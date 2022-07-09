// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <functional>

#include "Util.h"
#include "base/Logging.h"


Server::Server(EventLoop *loop, int threadNum, int port)
    : loop_(loop),
      threadNum_(threadNum),
      eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),
      started_(false),
      acceptChannel_(new Channel(loop_)),
      port_(port),
      listenFd_(socket_bind_listen(port_)) {
  // listenFd_代表整个server的接受通道，负责监听某个端口的 连接 事件
  acceptChannel_->setFd(listenFd_);
  handle_for_sigpipe();
  // 设置为非阻塞模式
  if (setSocketNonBlocking(listenFd_) < 0) {
    perror("set socket non block failed");
    abort();
  }
}

void Server::start() {
  // eventLoopThreadPool_线程池中会初始化若干那个线程，每个线程又对应一个EventLoop
  eventLoopThreadPool_->start();
  // EPOLLIN 新连接达到，或者新数据到达
  // EPOLLET 边缘触发模式
  // acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
  // 为acceptChannel_ 绑定回调函数，当该channel上有read和conn事件发生时，执行被绑定的函数
  acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));
  acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));
  // 把acceptChannel_注册到 主EventLoop上
  loop_->addToPoller(acceptChannel_, 0);
  started_ = true;
}

// 当acceptChannel_上有 连接事件时，调用该函数
void Server::handNewConn() {
  // 存储client ipv4地址，并用0初始化该数据块
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  // 接受acceptChannel_上的新连接请求
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                             &client_addr_len)) > 0) {
    // accept_fd为新接受，已建立的连接的文件描述符
    // 从池中获取一个EventLoop对象
    EventLoop *loop = eventLoopThreadPool_->getNextLoop();
    LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
        << ntohs(client_addr.sin_port);
    // cout << "new connection" << endl;
    // cout << inet_ntoa(client_addr.sin_addr) << endl;
    // cout << ntohs(client_addr.sin_port) << endl;
    /*
    // TCP的保活机制默认是关闭的
    int optval = 0;
    socklen_t len_optval = 4;
    getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
    cout << "optval ==" << optval << endl;
    */
    // 限制服务器的最大并发连接数
    if (accept_fd >= MAXFDS) {
      close(accept_fd);
      continue;
    }
    // 设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0) {
      LOG << "Set non block failed!";
      // perror("Set non block failed!");
      return;
    }

    setSocketNodelay(accept_fd);
    // setSocketNoLinger(accept_fd);

    shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
    req_info->getChannel()->setHolder(req_info);
    loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));
  }
  // 当退出while循环后，此次acceptChannel_上的 新连接 事件处理完成
  // 重新设置下 acceptChannel_ 的监听事件的类型，若下次还有链接事件则该函数还会被再次回调
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}