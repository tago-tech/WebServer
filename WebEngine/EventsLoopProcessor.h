#pragma once
#include <memory>

#include "EventsLoop.h"
#include "base/Thread.h"
#include "base/noncopyable.h"

class EventsLoopProcessor : noncopyable {
 public:
  EventsLoopProcessor();
  ~EventsLoopProcessor();
  void startLoop();
  std::shared_ptr<EventsLoop> getEventsLoop();
  
 private:
  void threadFunc();

 private:
  std::shared_ptr<EventsLoop> loop_;
  bool exiting_;
  Thread thread_;
};