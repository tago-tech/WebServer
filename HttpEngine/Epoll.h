#pragma once
#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "HttpRequestContext.h"
#include "SocketChannel.h"
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
  int getEpollFd() { return epollFd_; }

 private:
  static const int MAXFDS = 100000;
  int epollFd_;
  std::vector<epoll_event> events_;
  std::shared_ptr<HttpRequestContext> fd2http_[MAXFDS];
};