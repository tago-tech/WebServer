#pragma once
#include <iostream>
#include <string>
#include "EventsController.h"
#include "EventsProcessor.h"
#include "base/Thread.h"
#include <pthread.h>
#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

class Engine {
 public:
  Engine(int port,int thread_num);
  ~Engine(){};

 public:
  void Start();
  void Stop();

 private:
  std::shared_ptr<EventsProcessor> SelectOneEventsProcessors();

 private:
  int port_;
  int thread_num_;
  int listenFd_;
  int processor_index_;

  std::vector<std::shared_ptr<EventsProcessor>> events_processores_;
};
