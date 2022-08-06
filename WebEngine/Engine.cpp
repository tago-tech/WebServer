#include "Engine.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "EventsController.h"
#include "SocketChannel.h"
#include "Util.h"

Engine::Engine(int port, int thread_num)
    : port_(port), thread_num_(thread_num), processor_index_(0) {
  std::cout << "Init engine." << std::endl;
  // 注册进程当发生broken pipe异常后，应该怎样处理，即杀死进程，这是个系统调用，不用了解，不用看，直接写
  handle_for_sigpipe();
  // 创建一个listen—socket，并把它绑定到port_上，绑定成功后开始监听这个端口
  listenFd_ = socket_bind_listen(port_);
  // 创建若干个事件处理器，同时用shared_ptr指向他们，方法emplace_back的意思是把创建好的shared_ptr追加到数组末尾
  for (int i = 0; i < thread_num_; i++) {
    events_processores_.emplace_back(std::make_shared<EventsProcessor>());
  }
}

// 真正启动一个引擎，上面只是初始化，调用start之后，引擎才开始工作
void Engine::Start() {
  std::cout << "start engine." << std::endl;
  // 上面只是创建若干个事件处理器，这里把各个处理器启动起来
  for (const auto& processor : events_processores_) {
    processor->startLoop();
  }
  // 创建一个sockaddr_in的结构体，负责存储是手机/客户端 的 ip地址，端口，并用memset将其置为0
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  // 无限循环，从listenFd_（即listen_socket）上监听有无tcp链接的三次握手请求，如果有就进行三次握手，建立连接，
  // 建立连接的同时会生成一个新的socket（即accept_fd）来表示 服务端<-> 客户端 的tcp链接/socket，在该socket上将会产生 可读（in_buffer）、可写(out_buffer)的事件
  while ((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr,
                             &client_addr_len)) > 0) {
    // 这里硬限制下，最多建立10万链接，避免服务器内存/资源不够，导致进程崩溃
    if (accept_fd >= 99999) {
      // 如果超出了10w链接就直接close。
      close(accept_fd); // 对socket的close调用，意味着关闭这个socket/tcp链接，操作系统会执行四次挥手，断开这个tcp链接
      continue;
    }
    std::cout << "Accept connection from " << inet_ntoa(client_addr.sin_addr)
              << ":" << ntohs(client_addr.sin_port) << std::endl;
    // 选择一个事件处理器
    auto processor = SelectOneEventsProcessors();
    // 获取到这个事件处理器 所 对应的 事件控制器
    auto events_controller = processor->getEventsController();
    //设为非阻塞模式，即有可能会读到空的数据
    if (setSocketNonBlocking(accept_fd) < 0) {
      std::cout << "Set non block failed, exit engine!";
      return;
    }
    // socket/tcp链接，有事件/数据时立刻响应
    setSocketNodelay(accept_fd);
    // 初始化一个SocketChannel对象，这个对象是我们自定义的，上面定义了读写事件的处理方法，相当于是对操作系统的那个底层的socket做了一层封装。
    // 即不等同于操作系统的那个底层的socket，而是负责控制操作系统的那个底层socket的，一一对应的关系。
    auto socket_channel = std::make_shared<SocketChannel>(accept_fd);
    // 在这个socket channel上我们只关心 是否有数据进入（EPOLLIN），并用EPOLLET边缘触发模式（可以以后再了解，EPOLL的几种模式），设置完之后EPOLL只有在感知到 该socket上有可读事件发生后，才会告诉我们。其他事件不会。
    socket_channel->setEvents(EPOLLIN | EPOLLET);
    // 这段代码的意思是定义一个函数 对象，把events_controller、socket_channel传递到函数内部，入参为空，返回类型为void。
    // [events_controller, socket_channel]() -> void {
    //       events_controller->addToPoller(socket_channel); 意思是 让 events_controller事件控制器 把 该socket channel注册到所对应的 EPOLL对象上，将来该socket channel上若有事件发生可以通过该epoll对象感知。
    //     });
    // queueInLoop的意识时把这个函数，放到events_controller的待执行的函数列表中，将来events_controller事件控制器空闲时，执行这个函数对象。
    // 之所以不直接调用events_controller->addToPoller，是因为当前在主线程，而被选中的events_controller，已经在对应的事件处理器的 线程内运行呢，两个线程同时操作EPOLL对象会出问题。
    events_controller->queueInLoop(
        [events_controller, socket_channel]() -> void {
          events_controller->addToPoller(socket_channel);
        });
    // 这次握手已经结束，接下来主线程继续循环，继续等待新的tcp链接到来。
  }
  std::cout << "accept failed, exit engine.";
}

void Engine::Stop() {}

std::shared_ptr<EventsProcessor> Engine::SelectOneEventsProcessors() {
  auto processor = events_processores_.at(processor_index_ % thread_num_);
  processor_index_ += 1;
  return processor;
}