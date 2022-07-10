#include "EventsProcessor.h"

#include <cassert>
#include <functional>
#include <memory>
#include <utility>

#include "base/Thread.h"

EventsProcessor::EventsProcessor()
    : loop_(std::make_shared<EventsController>()), // 用这种方式在初始化方法中构造一个事件控制器对象。
      exiting_(false),
      thread_(bind(&EventsProcessor::threadFunc, this), "EventLoopThread") {
        // 这段代码的意思是，创建一个线程对象，并把它赋值给thread_，线程名叫做EventLoopThread
        // thread_(bind(&EventsProcessor::threadFunc, this), "EventLoopThread")
        // bind(&EventsProcessor::threadFunc, this)的意识是把 这个对象的EventsProcessor::threadFunc方法绑定到该线程上
        // 即该线程 从创建时就被指定，将来要运行 这个方法。
}

EventsProcessor::~EventsProcessor() {} // 什么也不做

// 线程thread_要执行的方法，只要是调用事件处理器EventsController的process方法
// process是个无限循环，该无限循环主要是一直处理 该事件控制器EventsController上发生的各种事件。
// 无限循环也让该线程thread_一直工作，一直不退出，跟main线程同时工作，分摊烦恼
void EventsProcessor::threadFunc() { loop_->process(); }

std::shared_ptr<EventsController> EventsProcessor::getEventsController() {
  return loop_;
}

// 启动事件控制器，
void EventsProcessor::startLoop() {
  assert(!thread_.started()); // 如果这个线程已经启动了，进程就会崩溃退出，这里是检查一下
  thread_.start(); // 启动线程对象，启动之后，线程将执行 初始化时被绑定的方法，即threadFunc方法。
  exiting_ = false; // 写错了应该是false，你有空自己改吧，不影响，没啥用
}
