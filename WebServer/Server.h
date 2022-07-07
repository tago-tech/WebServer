// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>
#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

/*
  http server实体，内部持有EventLoopThreadPool，负责 accepted socket的分发，
  并把这些已连接的 链接，注册到各个EventLoop上，即存在多个EPoll
*/
class Server {
 public:
  Server(EventLoop *loop, int threadNum, int port);
  ~Server() {}
  EventLoop *getLoop() const { return loop_; }
  void start();
  void handNewConn();
  void handThisConn() { loop_->updatePoller(acceptChannel_); }

 private:
  EventLoop *loop_;
  int threadNum_;
  std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
  bool started_;
  std::shared_ptr<Channel> acceptChannel_;
  int port_;
  int listenFd_;
  static const int MAXFDS = 100000;
};