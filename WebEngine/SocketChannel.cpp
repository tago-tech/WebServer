#include "SocketChannel.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <iterator>

#include "Util.h"

SocketChannel::SocketChannel(int fd) : fd_(fd), events_(0), last_events_(0) {}

SocketChannel::~SocketChannel() { close(fd_); }

void SocketChannel::handleEvents() {
  events_ = 0;
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    events_ = 0;
    return;
  }
  if (revents_ & EPOLLERR) {
    if (error_call_back_) error_call_back_();
    events_ = 0;
    return;
  }
  if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    handleRead();
  }
  if (revents_ & EPOLLOUT) {
    handleWrite();
  }
  handleConn();
}

void SocketChannel::handleRead() {
  std::cout << "tigger read event " << fd_ << std::endl;
  if (in_call_back_) {
    in_call_back_();
  }
}

void SocketChannel::handleConn() {
  std::cout << "tigger conn event " << fd_ << std::endl;
  if (conn_call_back_) {
    conn_call_back_();
  }
}

void SocketChannel::handleWrite() {
  std::cout << "tigger write event " << fd_ << std::endl;
  if (out_call_back_) {
    out_call_back_();
  }
}

void SocketChannel::handleError() {
  if (error_call_back_) {
    error_call_back_();
  }
}