#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "EventsController.h"
#include "EventsProcessor.h"
#include "base/Thread.h"

// Engine：引擎的意思，类名表示这是 负责web 或者 http的引擎对象。
// 主要有两个 public的 Start 和 Stop(未实现，出错时自动退出就行)
class Engine {
 public:
  Engine(int port, int thread_num);
  ~Engine(){};

 public:
  void Start();
  void Stop();

 private:
  std::shared_ptr<EventsProcessor> SelectOneEventsProcessors(); // 负责从若干个 事件处理器 中选择一个出来，因为要把请求均匀地发送到 各个线程中去，有各个线程 同时处理。

 private:
  int port_; // 表示要监听哪个端口
  int thread_num_; // 表示要开启多少个线程，同时并发 处理请求
  int listenFd_; // 负责监听端口的 listen_socket 的文件描述符，即代表 listen_socket
  int processor_index_; // 负责记录上次 循环到 哪个处理器了，下次 +1 即可
  std::vector<std::shared_ptr<EventsProcessor>> events_processores_; // std::vector, 可以理解成一个数组对象，里面存放 若干个指向 EventsProcessor（称为事件处理器） 的 shared_ptr
};