#pragma once
#include <sys/epoll.h>

#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>

#include "Epoll.h"
#include "HttpRequestContext.h"
#include "SocketChannel.h"
#include "Util.h"
#include "base/MutexLock.h"
using namespace std;

class EventsController : enable_shared_from_this<EventsController> {
 public:
  EventsController();
  ~EventsController();
  void process();
  void quit();
  void queueInLoop(std::function<void()>&& cb);
  void shutdown(std::shared_ptr<SocketChannel> channel) {
    shutDownWR(channel->getFd());
  }
  void removeFromPoller(shared_ptr<SocketChannel> channel) {
    epoller_->epoll_del(channel);
  }
  void updatePoller(shared_ptr<SocketChannel> channel, int timeout = 0) {
    epoller_->epoll_mod(channel, timeout);
  }
  void addToPoller(shared_ptr<SocketChannel> channel, int timeout = 0) {
    auto request_context =
        std::make_shared<HttpRequestContext>(this, channel->getFd(), channel);

    channel->setConnHandler(
        [request_context]() -> void { request_context->handleConn(); });
    channel->setReadHandler(
        [request_context]() -> void { request_context->handleRead(); });
    channel->setWriteHandler(
        [request_context]() -> void { request_context->handleWrite(); });
    epoller_->epoll_add(request_context, timeout);
  }

 private:
  bool looping_;
  shared_ptr<Epoll> epoller_;
  int wakeupFd_;
  bool quit_;
  bool eventHandling_;
  mutable MutexLock mutex_;
  std::vector<std::function<void()>> pendingFunctors_;
  bool callingPendingFunctors_;
  std::shared_ptr<SocketChannel> pwakeupChannel_;

  void wakeup();
  void handleRead();
  void doPendingFunctors();
  void handleConn();
};