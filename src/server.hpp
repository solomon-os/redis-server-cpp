#pragma once

#include "conn.hpp"
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>

class Server {
public:
  Server();

  ~Server();

  void listen();
  void accept();

  void handle_conn(int client_fd, const epoll_event &client_epoll_event);

  const int epoll_fd;
  const int fd;
  std::unordered_map<int, std::unique_ptr<Conn>> conns;
  char msg_buf[1024]; // read 1mb of message at a time
  static constexpr int max_events{1024};
  epoll_event epoll_server_event{}, epoll_client_events[max_events];
};
