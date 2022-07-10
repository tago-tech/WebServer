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

// 初始化方法中用epoll_create1，创建一个操作系统级别的epoll，并返回一个int描述符，赋值给epollFd_
Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
  assert(epollFd_ > 0); // 判断一下是否出错了
}

Epoll::~Epoll() {}

void Epoll::epoll_add(std::shared_ptr<HttpRequestContext> request_context,
                      int timeout) {
  // HttpRequestContext上是可以获取到对应的socket channel的
  auto socket_channel = request_context->getChannel();
  int fd = socket_channel->getFd(); // 获取socket channel对应的那个底层的操作系统的int型socket 描述符
  struct epoll_event event;
  event.data.fd = fd;
  event.events = socket_channel->getEvents();
  socket_channel->equalAndUpdateLastEvents();
  fd2http_[fd] = request_context;
  // 把该文件描述符，加入到epoll的监听中去，监听event.events这类事件
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
    std::cout << "epoll_ctl add fd" << fd << " error" << std::endl;
    fd2http_[fd].reset(); // 监听失败，就重置fd2http_[fd]为空指针
  }
}

// 修改socket channel所感兴趣的事件
void Epoll::epoll_mod(std::shared_ptr<SocketChannel> socket_channel,
                      int timeout) {
  // add timer
  int fd = socket_channel->getFd();
  // 如果感兴趣的事件没有变化，就直接跳过了
  if (!socket_channel->equalAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = socket_channel->getEvents(); // 感兴趣的事件
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      std::cout << "epoll_ctl mod fd" << fd << " error" << std::endl;
      fd2http_[fd].reset();//重置fd2http_[fd]为空指针
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

// 返回活跃事件数
std::vector<std::shared_ptr<SocketChannel>> Epoll::poll() {
  while (true) {
    // 等待epoll上监听到事件并返回，监听到的 有关socket的个数，最长等待EPOLLWAIT_TIME秒，如果没事件发生就返回0，错误返回-1
    // 也就是会函数执行到这，会阻塞，也正是因为如此，上层的事件控制器需要增加一个唤醒功能。
    int event_count = 
        epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (event_count < 0) std::cout << "epoll wait error" << std::endl;
    // 获取发生事件的socket channel，并返回给事件控制器，事件控制器逐个遍历，处理。
    std::vector<std::shared_ptr<SocketChannel>> req_data =
        getEventsRequest(event_count);
    if (req_data.size() > 0) return req_data;
  }
}

// 分发处理函数
std::vector<std::shared_ptr<SocketChannel>> Epoll::getEventsRequest(
    int events_num) {
  std::vector<std::shared_ptr<SocketChannel>> req_data;
  for (int i = 0; i < events_num; ++i) {
    // 获取事件所对应的 底层socket描述符
    int fd = events_[i].data.fd;
    // 获取该底层socket所对应的socket channel
    std::shared_ptr<SocketChannel> cur_req = fd2http_[fd]->getChannel();
    // 如果不为空
    if (cur_req) {
      // 设置该socket上发生了这些事件
      cur_req->setRevents(events_[i].events);
      cur_req->setEvents(0);
      // 把该socket channle加入到数组req_data中。
      req_data.push_back(cur_req);
    } else {
      std::cout << "SocketChannel cur_req is invalid" << std::endl;
    }
  }
  return req_data;
}
