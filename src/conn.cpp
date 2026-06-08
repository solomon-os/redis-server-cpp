#pragma once
#include "conn.hpp"
#include <cstddef>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>

Conn::Conn(int fd) : fd{fd} {}

Conn::~Conn() { close(this->fd); }

int Conn::register_non_blocking() {
  int fcntl_flags = fcntl(fd, F_GETFL, 0);
  if (fcntl_flags == -1) {
    std::cerr << "fcntl F_GETFL failed" << std::endl;
    return -1;
  }
  if (fcntl(fd, F_SETFL, fcntl_flags | O_NONBLOCK) == -1) {
    std::cerr << "fcntl F_SETFL failed" << std::endl;
    return -1;
  }
  return 0;
}

int Conn::read(char *buf, size_t cap) {
  while (true) {

    ssize_t n = recv(this->fd, buf, cap, 0);

    if (n > 0) {
      this->message_buf.append(buf, n);
      continue;
    }

    if (n == 0) { // peer closed the connection
      return -1;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }
  }

  const std::string escaped = std::format("{:?}", this->message_buf);
  std::cout << "recv: " << escaped << "\n" << std::flush;
  return 0;
}
