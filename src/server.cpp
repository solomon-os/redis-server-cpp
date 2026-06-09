#include "server.hpp"
#include "handler.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <utility>

int make_epoll() {

  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd == -1) {
    throw std::runtime_error("epoll creation failed");
  }
  return epoll_fd;
}

int make_server_fd() {
  const int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    throw std::runtime_error("Creating socket failed");
  }
  return fd;
}

Server::Server(Handler handle)
    : epoll_fd{make_epoll()}, fd{make_server_fd()}, handler{handle} {
  int reuse = 1;
  if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt failed\n" << std::flush;
  }

  if (setsockopt(this->fd, IPPROTO_TCP, TCP_NODELAY, &reuse, sizeof(reuse))) {
    std::cerr << "setsockopt failed\n" << std::flush;
  }

  sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(6379);
  server_address.sin_addr.s_addr = INADDR_ANY;

  if (bind(this->fd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    throw std::runtime_error("Binding to socket port failed");
  }

  if (::listen(this->fd, 4069) < 0) {
    throw std::runtime_error("listning on socket failed");
  }

  // The accept() loop drains the queue until EAGAIN, which only works if the
  // listening socket is non-blocking — otherwise accept() blocks waiting for
  // the *next* connection and the event loop never resumes.
  if (fcntl(this->fd, F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error("fcntl server_socket O_NONBLOCK failed");
  }

  this->epoll_server_event.events = EPOLLIN;
  this->epoll_server_event.data.fd = this->fd;

  if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, this->fd,
                &this->epoll_server_event) == -1) {
    throw std::runtime_error("epoll_ctl failed");
  }

  std::cout << "listening on server" << std::endl;
}

Server::~Server() {
  this->conns.clear();
  close(this->epoll_fd);
  close(this->fd);
}

void Server::listen() {
  while (true) {
    const int nfds = epoll_wait(this->epoll_fd, this->epoll_client_events,
                                Server::max_events, -1);
    if (nfds == -1) {
      if (errno == EINTR) {
        continue;
      }

      throw std::runtime_error("epoll_wait failed: " +
                               std::string(std::strerror(errno)));
    }

    for (int n = 0; n < nfds; ++n) {
      if (this->epoll_client_events[n].data.fd ==
          this->epoll_server_event.data.fd) {
        this->accept();

      } else {
        this->handle(this->epoll_client_events[n].data.fd,
                     this->epoll_client_events[n]);
      }
    }
  }
}

void Server::accept() {
  sockaddr conn_addr;
  socklen_t addrlen = sizeof(conn_addr);

  while (true) {
    int conn_sock =
        ::accept(this->epoll_server_event.data.fd, &conn_addr, &addrlen);

    if (conn_sock == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }

      std::cerr << "accept client failed: " << conn_addr.sa_data << std::endl;
      continue;
    }

    std::unique_ptr<Conn> conn_ptr = std::make_unique<Conn>(conn_sock);

    this->epoll_server_event.events = EPOLLIN | EPOLLET;

    epoll_event ev{};
    ev.data.fd = conn_sock;
    ev.data.ptr = conn_ptr.get();

    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
      std::cerr << "epoll_ctl: conn_sock" << std::endl;
    }

    this->conns[conn_sock] = std::move(conn_ptr);
  }
}

void Server::handle(int client_fd, const epoll_event &client_epoll_event) {
  auto it = this->conns.find(client_fd);
  if (it == this->conns.end()) {
    return;
  }

  Conn &client = *it->second;

  if (client.read(this->msg_buf, Server::msg_buf_len) < 0) {
    this->conns.erase(client_fd);
    return;
  }

  this->handler.handle();
}
