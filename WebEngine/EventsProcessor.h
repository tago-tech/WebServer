#pragma once
#include <memory>

#include "EventsController.h"
#include "base/Thread.h"
#include "base/noncopyable.h"

// 事件处理器，继承自noncopyable，即创建的不允许拷贝。
// 一个事件处理器对应一个线程，对应一个事件控制器。
// 进程内可以有多个事件处理器同时工作，同时处理不同的事件处理器。
// 事件处理器内部的线程 被指定了 只执行 事件控制器EventsController的方法threadFunc
// 事件处理器其实主要是启动一个线程并让他执行事件控制器的process方法，其实对于各种事件的核心处理逻辑还是在事件控制器上。
class EventsProcessor : noncopyable {
 public:
  EventsProcessor(); // 事件处理器的初始化方法
  ~EventsProcessor();
  void startLoop(); // 事件处理器的启动方法，即开始让事件处理器开始工作
  std::shared_ptr<EventsController> getEventsController(); // 获得该事件控制器EventsProcessor所对应的事件控制器EventsController
  
 private:
  void threadFunc(); // 直接翻译为线程方法，即等会儿会把这个方法 注册到 线程上。

 private:
  std::shared_ptr<EventsController> loop_; // 对应的事件控制器，用shared_ptr包裹
  bool exiting_; // 标示该事件处理器是否已退出
  Thread thread_; // 事件处理器所对应的线程对象，一个进程内可以有多个线程，多个线程同时并行工作。
};