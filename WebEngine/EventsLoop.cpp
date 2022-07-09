#include "EventsLoop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>

#include "Epoll.h"
#include "SocketChannel.h"
#include "Util.h"
#include "unistd.h"

int createEventfd() {
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    std::cout << "Failed to create fd eventfd" << std::endl;
    abort();
  }
  return evtfd;
}

EventsLoop::EventsLoop()
    : looping_(false),
      epoller_(new Epoll()),
      wakeupFd_(createEventfd()),
      quit_(false),
      callingPendingFunctors_(false),
      pwakeupChannel_(new SocketChannel(wakeupFd_)) {
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
  pwakeupChannel_->setReadHandler(bind(&EventsLoop::handleRead, this));
  pwakeupChannel_->setConnHandler(bind(&EventsLoop::handleConn, this));
  epoller_->epoll_add(
      std::make_shared<HttpRequestContext>(this, wakeupFd_, pwakeupChannel_),
      0);
}

EventsLoop::~EventsLoop() { close(wakeupFd_); }

void EventsLoop::handleConn() { updatePoller(pwakeupChannel_, 0); }

void EventsLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = readn(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    std::cout << "EventLoop::handleRead() reads " << n << " bytes instead of 8"
              << std::endl;
  }
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

void EventsLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
  if (n != sizeof one) {
    std::cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8"
              << std::endl;
  }
}

void EventsLoop::queueInLoop(std::function<void()>&& cb) {
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }
  wakeup();
}

void EventsLoop::process() {
  assert(!looping_);
  looping_ = true;
  quit_ = false;
  std::cout << "EventLoop " << this << " start looping" << std::endl;
  std::vector<std::shared_ptr<SocketChannel>> ret;
  while (!quit_) {
    // cout << "doing" << endl;
    ret.clear();
    ret = epoller_->poll();
    eventHandling_ = true;
    for (auto& it : ret) it->handleEvents();
    eventHandling_ = false;
    doPendingFunctors();
  }
  looping_ = false;
}

void EventsLoop::doPendingFunctors() {
  std::vector<std::function<void()>> functors;
  callingPendingFunctors_ = true;
  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }
  for (size_t i = 0; i < functors.size(); ++i) functors[i]();
  callingPendingFunctors_ = false;
}