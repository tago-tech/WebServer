#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "Epoll.h"
#include "SocketChannel.h"
#include "Util.h"
#include "base/MutexLock.h"
using namespace std;

// 事件控制器，进程内各类事件的核心处理逻辑
// 其内部持有一个Epoll对象，可以通过事件控制器 操纵Epoll对象，负责向Epoll上添加、删除、修改 socket channel的监听事件。
// process方法负责处理各类事件。
class EventsController : enable_shared_from_this<EventsController> {
 public:
  EventsController();
  ~EventsController();
  void process();
  void quit();
  // 把一个函数对象放到待执行队列里，将来空闲时候执行这个函数
  void queueInLoop(std::function<void()>&& cb);
  // 关闭一个socket channel，例如该socket channel上发生什么异常时候，就会调用该方法，可以理解成安全地关闭该socket，tcp挥手
  void shutdown(std::shared_ptr<SocketChannel> channel) {
    shutDownWR(channel->getFd());
  }
  // 将一个已在该Epoll对象上注册过的socket channel取消注册，即Epoll将来不再监听该socket channel
  // 一般是在 socket 关闭，或者请求结束时使用。
  void removeFromPoller(shared_ptr<SocketChannel> channel) {
    // shutDownWR(channel->getFd());
    epoller_->epoll_del(channel);
  }
  // 更新 socket channel感兴趣的事件
  void updatePoller(shared_ptr<SocketChannel> channel, int timeout = 0) {
    epoller_->epoll_mod(channel, timeout);
  }
    // 将一个新建立链接的的socket channel注册到该Epoll对象上，channel里定义了感兴趣的事件，一般为可读事件。
  void addToPoller(shared_ptr<SocketChannel> channel, int timeout = 0) {
    // 初始化一个Http请求上下文对象，该对象与socket channel一一对应，记录发生在该socket channel上的动作，处理到那一步了，下一步该执行什么了。
    // 一个SocketChannel，一个HttpRequestContext
    // 其中HttpRequestContext里面也实现了怎么处理SocketChannel上发生的读写事件
    auto request_context =
        std::make_shared<HttpRequestContext>(this, channel->getFd(), channel);
    // socket channel上有链接事件发生后执行 request_context->handleConn();，即调用HttpRequestContext的handleConn，下面的意义也一样，
    // 其实这样也看出来实际对SocketChannel上读写事件的处理逻辑，还是在HttpRequestContext中实现的。
    channel->setConnHandler(
        [request_context]() -> void { request_context->handleConn(); });
    // 注册读事件的回调函数，当读事件发生后执行request_context->handleRead();
    channel->setReadHandler(
        [request_context]() -> void { request_context->handleRead(); });
    // 注册可写事件的回调函数，同上
    channel->setWriteHandler(
        [request_context]() -> void { request_context->handleWrite(); });
    // 将该channle & request_context，一起注册到Epoll对象中去
    epoller_->epoll_add(request_context, timeout); // 正式因为Epoll对象不能被多个线程同时调用，才让EventsController的实现这么复杂
  }

 private:
  // 声明顺序 wakeupFd_ > pwakeupChannel_
  bool looping_; // 标示是否在工作
  shared_ptr<Epoll> epoller_; // 对应的Epoll对象，可以让该Epoll对象关闭 很多个SocketChannel，某个SocketChannel有感兴趣的事件发生后，可以通过Epoll函数得知，这正是该项目高性能的关键。
  int wakeupFd_;  // 一个唤醒socket，负责有意地唤醒 Epoll对象，因为当Epoll监听的任何一个Socket没有事件发生时，Epoll就会阻塞着不再执行，有时候我们想唤醒Epoll，就往这个虚拟的socket上发消息，这样Epoll就感知到了，也就被唤醒了
  bool quit_; // 标示 事件控制器 是否已退出
  bool eventHandling_; // 是否在处理各类事件
  mutable MutexLock mutex_; // 一把锁，避免多个线程同时操控pendingFunctors_，那样的话，可能会数据错乱，这把锁会让他们先来后到。这个项目里只有主线程会和该事件控制器对应的事件处理线程，争抢这把锁。
  std::vector<std::function<void()>> pendingFunctors_; // 一个数据，存储多个待执行的函数对象。
  bool callingPendingFunctors_; // 是否在调用待执行的函数对象
  std::shared_ptr<SocketChannel> pwakeupChannel_; // 对应于wakeupFd_的socket channel，我们向其定义感兴趣的事件，并把它注册到需要唤醒的Epoll上

  void wakeup(); // 唤醒Epoll的方法
  void handleRead(); // 仅仅处理pwakeupChannel_上的可读事件
  void doPendingFunctors(); // 循环调用
  void handleConn(); // 仅仅处理pwakeupChannel_上的链接事件pendingFunctors_中的待执行函数
};