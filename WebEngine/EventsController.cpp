#include "EventsController.h"

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

// 创建一个虚拟的，负责唤醒Epoll的 socket
int createEventfd() {
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    std::cout << "Failed to create fd eventfd" << std::endl;
    abort();
  }
  return evtfd;
}

EventsController::EventsController()
    : looping_(false),
      epoller_(new Epoll()),
      wakeupFd_(createEventfd()),
      quit_(false),
      callingPendingFunctors_(false),
      pwakeupChannel_(new SocketChannel(wakeupFd_)) {
  // 设置唤醒的socket channel上感兴趣的事件，这里是可读事件
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
  // 向pwakeupChannel_注册可读事件的回调，即当pwakeupChannel_上发生读事件时，执行this.handleRead，下同，是注册链接事件的
  pwakeupChannel_->setReadHandler(bind(&EventsController::handleRead, this));
  pwakeupChannel_->setConnHandler(bind(&EventsController::handleConn, this));
  // 将该唤醒socket channel注册到epoll对象上，即将来该socket上发生事件后，epoll也会监听到，并反馈回来，正是利用这一机制，我们可以随时唤醒epoll
  epoller_->epoll_add(
      std::make_shared<HttpRequestContext>(this, wakeupFd_, pwakeupChannel_),
      0);
}

EventsController::~EventsController() { 
  close(wakeupFd_);//关闭唤醒socket
 }

// 当唤醒socket channle上有链接事件发生后，执行这个方法，不过好像也没执行过，因为只对唤醒channel监听了可读事件
void EventsController::handleConn() { updatePoller(pwakeupChannel_, 0); }

// 当唤醒socket channle上有可读事件发生后，执行这个方法，
void EventsController::handleRead() {
  // 从wakeupFd_这个socket上读出来buff，之后啥也不做，因为这玩意存在的目的就是为了唤醒epoll，
  // 因为唤醒时向wakeupFd_写入了数据，这里就读出来，否则内存会满
  uint64_t one = 1;
  ssize_t n = readn(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    std::cout << "EventLoop::handleRead() reads " << n << " bytes instead of 8"
              << std::endl;
  }
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET); // 设置一下读事件
}

// 唤醒Epoll，就像wakeupFd_上随便写点数据，epoll就会感知到，然后从epoll.epoll_wait方法上返回回来
void EventsController::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
  if (n != sizeof one) {
    std::cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8"
              << std::endl;
  }
}

void EventsController::queueInLoop(std::function<void()>&& cb) {
  // 这里先加锁，把cb对象放到待执行 列表的末尾
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }
  // 因为这里新建入了一个 待执行函数， 避免epoll_wait阻塞，这里主动唤醒一下。
  wakeup();
}

// 核心的处理逻辑，正是事件处理器线程要执行的方法
void EventsController::process() {
  assert(!looping_); // 检查一下是否重复启动了
  looping_ = true;
  quit_ = false;
  std::cout << "EventLoop " << this << " start looping" << std::endl;
  std::vector<std::shared_ptr<SocketChannel>> ret; // 定义一个存储SocketChannel的数组
  // 无限循环，一直处理各种事件
  while (!quit_) {
    // cout << "doing" << endl;
    ret.clear(); // 清空数组，避免干扰。
    ret = epoller_->poll(); // 等待Epoll对象上监听事件，任何Socket channel上发生感兴趣的事件后，就会返回，如果没有事件发生线程就一直等待着。
    eventHandling_ = true;
    // 遍历 发生事件的 socket channel，并处理事件
    for (auto& it : ret) it->handleEvents(); 
    eventHandling_ = false;
    // 执行 待执行的函数
    doPendingFunctors();
  }
  looping_ = false;
}

void EventsController::doPendingFunctors() {
  std::vector<std::function<void()>> functors;
  callingPendingFunctors_ = true;
  // 这里也需要加锁，是为了避免主线程在同时操控pendingFunctors_，两个线程同时写，就会出问题
  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }
  for (size_t i = 0; i < functors.size(); ++i) functors[i]();
  callingPendingFunctors_ = false;
}