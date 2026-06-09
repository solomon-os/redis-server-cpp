#include "conn.hpp"
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <netinet/in.h>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>

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
      this->msg_in_buf.append(buf, n);
      continue;
    }

    if (n == 0) { // peer closed the connection
      return -1;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }
  }

  const std::string escaped = std::format("{:?}", this->msg_in_buf);
  std::cout << "recv: " << escaped << "\n" << std::flush;
  return 0;
}

int Conn::flush() {
  std::string_view out_buf{this->msg_out_buf};
  size_t sent = 0;
  ssize_t n = 0;

  while (sent < out_buf.size()) {
    auto remaining = out_buf.substr(sent, out_buf.size());
    n = ::send(this->fd, remaining.data(), remaining.size(), MSG_NOSIGNAL);
    if (n > 0) {
      sent += n;
      continue;
    }

    if (n == 0) {
      break;
    }

    if (n == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break; // bufer full finish later
      }
      return -1;
    }
    break;
  }
  this->msg_out_buf.erase(0, sent);
  return sent;
}
