#pragma once
#include <iostream>
#include <string>

class Engine {
 public:
  Engine(int port);
  ~Engine(){};

 public:
  void Start();

private:
  int port_;
  int listenFd_;
};