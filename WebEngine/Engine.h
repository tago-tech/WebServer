#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "EventsLoop.h"
#include "EventsLoopProcessor.h"
#include "base/Thread.h"

class Engine {
 public:
  Engine(int port, int thread_num);
  ~Engine(){};

 public:
  void Start();
  void Stop();

 private:
  std::shared_ptr<EventsLoopProcessor> SelectOneEventsProcessors();

 private:
  int port_;
  int thread_num_;
  int listenFd_;
  int processor_index_;
  std::vector<std::shared_ptr<EventsLoopProcessor>> events_processores_;
};