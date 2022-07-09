#include "EventsLoopProcessor.h"

#include <cassert>
#include <functional>
#include <memory>
#include <utility>

#include "base/Thread.h"

EventsLoopProcessor::EventsLoopProcessor()
    : loop_(std::make_shared<EventsLoop>()),
      exiting_(false),
      thread_(bind(&EventsLoopProcessor::threadFunc, this), "EventLoopThread") {
}

EventsLoopProcessor::~EventsLoopProcessor() {}

void EventsLoopProcessor::threadFunc() { loop_->process(); }

std::shared_ptr<EventsLoop> EventsLoopProcessor::getEventsLoop() {
  return loop_;
}

void EventsLoopProcessor::startLoop() {
  assert(!thread_.started());
  thread_.start();
  exiting_ = true;
}
