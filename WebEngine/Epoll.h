#pragma once
#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "HttpRequestContext.h"
#include "SocketChannel.h"
// #include "Timer.h"

/*
   Epoll上有这几类事件
   EPOLLIN ： 表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
   EPOLLOUT： 表示对应的文件描述符可以写；
   EPOLLPRI：
   表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
   EPOLLERR： 表示对应的文件描述符发生错误；
   EPOLLHUP： 表示对应的文件描述符被挂断；
   EPOLLET： 将 EPOLL设为边缘触发(Edge
   Triggered)模式（默认为水平触发），这是相对于水平触发(Level Triggered)来说的。
   EPOLLONESHOT：
   只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
*/
// 这里就是基于操作系统的epoll.h，做了一层封装，封装出来一些易用的方法
class Epoll {
 public:
  Epoll();
  ~Epoll();
  // 将HttpRequestContext中的SocketChannel注册到该epoll对象上
  void epoll_add(std::shared_ptr<HttpRequestContext> request_context,
                 int timeout);
  // 修改socket channel感兴趣的事件
  void epoll_mod(std::shared_ptr<SocketChannel> socket_channel, int timeout);
  // epoll对象不再监听该socket channel
  void epoll_del(std::shared_ptr<SocketChannel> socket_channel);
  // 等待所监听的任何socket channel上发生事件，然后返回发生事件的socket channel，返回结果是个数组
  std::vector<std::shared_ptr<SocketChannel>> poll();
  std::vector<std::shared_ptr<SocketChannel>> getEventsRequest(int events_num);
  // epoll也有自己的文件描述符，用int表示
  int getEpollFd() { return epollFd_; }

 private:
  static const int MAXFDS = 100000;
  int epollFd_;
  std::vector<epoll_event> events_; // 存放发生的所有事件
  std::shared_ptr<HttpRequestContext> fd2http_[MAXFDS]; // 记录操作系统级别socket 与 我们定义的HttpRequestContext 对应关系，因为操作系统的epoll.h只认识操作系统级别的socket(即int)
};