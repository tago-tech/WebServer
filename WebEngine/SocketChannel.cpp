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
    // 如果socket挂起EPOLLHUP，并且没有可读事件EPOLLIN，就认为没啥问题
    // 跟http中的keep alive有关，不过咱们项目里没事件，代码我也没删除
    events_ = 0;
    return;
  }
  if (revents_ & EPOLLERR) {
    // 如果Epoll发现了该socket错误，EPOLLERR
    // 如果error_call_back_对象不为空，就执行这个函数对象，意思是发生错误时执行的函数，一般为关闭链接
    if (error_call_back_) error_call_back_();
    events_ = 0;
    return;
  }
  if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    // 如果有可读事件发生，就执行 可读事件的回调函数
    handleRead();
  }
  if (revents_ & EPOLLOUT) {
    // 如果有可写事件发生，就执行可写事件的回调函数
    handleWrite();
  }
  // 剩下就是链接相关的事件，跟http keep alive相关，我没仔细看，也没支持，没啥大问题
  handleConn();
}

// 下面都是如果不为空就执行某个函数对象。
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