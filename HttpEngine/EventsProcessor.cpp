#include "EventsProcessor.h"

#include <cassert>
#include <functional>
#include <memory>
#include <utility>

#include "base/Thread.h"

EventsProcessor::EventsProcessor()
    : loop_(std::make_shared<
            EventsController>()),  
      exiting_(false),
      thread_(bind(&EventsProcessor::threadFunc, this), "EventLoopThread") {}

EventsProcessor::~EventsProcessor() {}

void EventsProcessor::threadFunc() { loop_->process(); }

std::shared_ptr<EventsController> EventsProcessor::getEventsController() {
  return loop_;
}

void EventsProcessor::startLoop() {
  assert(!thread_.started());
  thread_.start();
  exiting_ = false;
}