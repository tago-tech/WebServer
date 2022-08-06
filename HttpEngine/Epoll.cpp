#include "Epoll.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <deque>
#include <iostream>
#include <iterator>
#include <memory>
#include <queue>

#include "HttpRequestContext.h"
#include "Util.h"

using namespace std;

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)),events_(EVENTSNUM) {
    assert(epollFd_ > 0);
}
Epoll::~Epoll() {}

void Epoll::epoll_add(std::shared_ptr<HttpRequestContext> request_context,
int timeout) {
    auto socket_channel  = request_context->getChannel();
    int fd = socket_channel->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = socket_channel->getEvents();
    socket_channel->equalAndUpdateLastEvents();
    fd2http_[fd] = request_context;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        std::cout << "epoll_ctl add fd" << fd << " error" << std::endl;
        fd2http_[fd].reset();
  }


  
}

void Epoll::epoll_mod(std::shared_ptr<SocketChannel> socket_channel,
                      int timeout) {
  
  int fd = socket_channel->getFd();
  
  if (!socket_channel->equalAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = socket_channel->getEvents(); 
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      std::cout << "epoll_ctl mod fd" << fd << " error" << std::endl;
      fd2http_[fd].reset();
    }
  }
}

void Epoll::epoll_del(std::shared_ptr<SocketChannel> socket_channel) {
  int fd = socket_channel->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = socket_channel->getLastEvents();
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    std::cout << "epoll_ctl del fd" << fd << " error" << std::endl;
  }
  fd2http_[fd].reset();
}


std::vector<std::shared_ptr<SocketChannel>> Epoll::poll() {
  while (true) {
    int event_count = 
        epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (event_count < 0) std::cout << "epoll wait error" << std::endl;
    std::vector<std::shared_ptr<SocketChannel>> req_data =
        getEventsRequest(event_count);
    if (req_data.size() > 0) return req_data;
  }
}

std::vector<std::shared_ptr<SocketChannel>> Epoll::getEventsRequest(
    int events_num) {
  std::vector<std::shared_ptr<SocketChannel>> req_data;
  for (int i = 0; i < events_num; ++i) {
    int fd = events_[i].data.fd;
    std::shared_ptr<SocketChannel> cur_req = fd2http_[fd]->getChannel();
    if (cur_req) {
      cur_req->setRevents(events_[i].events);
      cur_req->setEvents(0);
      req_data.push_back(cur_req);
    } else {
      std::cout << "SocketChannel cur_req is invalid" << std::endl;
    }
  }
  return req_data;
}