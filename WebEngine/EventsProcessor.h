#pragma once
#include <memory>

#include "EventsController.h"
#include "base/Thread.h"
#include "base/noncopyable.h"

class EventsProcessor : noncopyable {
 public:
  EventsProcessor();
  ~EventsProcessor();
  void startLoop();
  std::shared_ptr<EventsController> getEventsController();
  
 private:
  void threadFunc();

 private:
  std::shared_ptr<EventsController> loop_;
  bool exiting_;
  Thread thread_;
};