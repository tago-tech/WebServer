#include "Engine.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "EventsLoop.h"
#include "SocketChannel.h"
#include "Util.h"

Engine::Engine(int port, int thread_num)
    : port_(port), thread_num_(thread_num), processor_index_(0) {
  std::cout << "Init engine." << std::endl;
  handle_for_sigpipe();
  listenFd_ = socket_bind_listen(port_);
  for (int i = 0; i < thread_num_; i++) {
    events_processores_.emplace_back(std::make_shared<EventsLoopProcessor>());
  }
}

void Engine::Start() {
  std::cout << "start engine." << std::endl;
  for (const auto& processor : events_processores_) {
    processor->startLoop();
  }
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  while ((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr,
                             &client_addr_len)) > 0) {
    if (accept_fd >= 99999) {
      close(accept_fd);
      continue;
    }
    std::cout << "Accept connection from " << inet_ntoa(client_addr.sin_addr)
              << ":" << ntohs(client_addr.sin_port) << std::endl;
    auto processor = SelectOneEventsProcessors();
    auto events_loop = processor->getEventsLoop();
    //设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0) {
      std::cout << "Set non block failed, exit engine!";
      return;
    }
    setSocketNodelay(accept_fd);
    auto socket_channel = std::make_shared<SocketChannel>(accept_fd);
    socket_channel->setEvents(EPOLLIN | EPOLLET);
    events_loop->queueInLoop([events_loop, socket_channel]() -> void {
      events_loop->addToPoller(socket_channel);
    });
  }
}

void Engine::Stop() {}

std::shared_ptr<EventsLoopProcessor> Engine::SelectOneEventsProcessors() {
  auto processor = events_processores_.at(processor_index_ % thread_num_);
  processor_index_ += 1;
  return processor;
}