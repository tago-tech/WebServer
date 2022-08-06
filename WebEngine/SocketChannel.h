#pragma once
#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>


// 对操作系统底层的socket进行了封装，抽象。
// 记录了在该socket上发生了哪些方法，定义了发生读写事件时该怎样处理。
// handleEvents根据发生的Epoll事件，判断后，确定执行哪种回调函数。
class SocketChannel {
 public:
  SocketChannel(int fd); // 对应于一个底层的socket描述符
  ~SocketChannel();

 public:
  // 实际处理事件
  void handleEvents();
  // 下面设置三类事件的回调函数，std::function对象代表了一个可执行的函数，
  void setReadHandler(std::function<void()>&& read_handler) {
    in_call_back_ = read_handler;
  }
  void setConnHandler(std::function<void()>&& conn_handler) {
    conn_call_back_ = conn_handler;
  }
  void setWriteHandler(std::function<void()>&& write_handler) {
    out_call_back_ = write_handler;
  }
  void setErrorHandler(std::function<void()>&& error_handler) {
    error_call_back_ = error_handler;
  }
  // 设置当前socket channel上有哪些事件发生，好让handleEvents做判断。
  void setRevents(__uint32_t revents) { revents_ = revents; }
  // 没啥用？
  void setEvents(__uint32_t events) { events_ = events; }
  // 判断这次事件跟上次有没有不同
  bool equalAndUpdateLastEvents() {
    bool ret = (last_events_ == events_);
    last_events_ = events_;
    return ret;
  }
  __uint32_t getLastEvents() { return revents_; }
  __uint32_t& getEvents() { return events_; }
  int getFd() { return fd_; }

 private:
  void handleRead();
  void handleConn();
  void handleWrite();
  void handleError();

 private:
  int fd_;
  __uint32_t events_;
  __uint32_t last_events_;
  __uint32_t revents_;
  std::function<void()> in_call_back_; // 下面是四类回调函数对象
  std::function<void()> out_call_back_;
  std::function<void()> conn_call_back_;
  std::function<void()> error_call_back_;
};