#pragma once
#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "HttpRequestContext.h"
#include "SocketChannel.h"
// #include "Timer.h"

/*
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
class Epoll {
 public:
  Epoll();
  ~Epoll();
  void epoll_add(std::shared_ptr<HttpRequestContext> request_context,
                 int timeout);
  void epoll_mod(std::shared_ptr<SocketChannel> socket_channel, int timeout);
  void epoll_del(std::shared_ptr<SocketChannel> socket_channel);
  std::vector<std::shared_ptr<SocketChannel>> poll();
  std::vector<std::shared_ptr<SocketChannel>> getEventsRequest(int events_num);
  //   void add_timer(std::shared_ptr<SocketChannel> request_data, int timeout);
  int getEpollFd() { return epollFd_; }

 private:
  static const int MAXFDS = 100000;
  int epollFd_;
  std::vector<epoll_event> events_;
  std::shared_ptr<HttpRequestContext> fd2http_[MAXFDS];
};