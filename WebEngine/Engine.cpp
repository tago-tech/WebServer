#include "Engine.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>

#include "Util.h"

Engine::Engine(int port) : port_(port) {
  std::cout << "Init engine." << std::endl;
  handle_for_sigpipe();
  listenFd_ = socket_bind_listen(port_);
}

void Engine::Start() {
  std::cout << "start engine." << std::endl;
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                             &client_addr_len)) > 0) {
    std::cout << "Accept connection from " << inet_ntoa(client_addr.sin_addr)
              << ":" << ntohs(client_addr.sin_port) << std::endl;
    //设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0) {
      std::cout << "Set non block failed, break engine!";
      return;
    }

    setSocketNodelay(accept_fd);
    bool zero = false;
    std::string inBuffer_;
    int read_num = readn(accept_fd, inBuffer_, zero);
    std::cout << "read count: " << read_num << ", recv:----\n"
              << inBuffer_ << "\n-----" << std::endl;
  }
}