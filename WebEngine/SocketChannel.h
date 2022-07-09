#pragma once
#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class SocketChannel {
 public:
  SocketChannel(int fd);
  ~SocketChannel();

 public:
  void handleEvents();
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
  void setRevents(__uint32_t revents) { revents_ = revents; }
  void setEvents(__uint32_t events) { events_ = events; }
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
  std::function<void()> in_call_back_;
  std::function<void()> out_call_back_;
  std::function<void()> conn_call_back_;
  std::function<void()> error_call_back_;
};