#pragma once

#include "conn.hpp"
#include "handler.hpp"
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>

class Server {
public:
  Server(Handler handle);

  ~Server();

  void listen();
  void accept();

  void handle(int conn_fd);

  const int epoll_fd;
  const int fd;

  std::unordered_map<int, std::unique_ptr<Conn>> conns;

  static constexpr int msg_buf_len{1024};
  char msg_buf[msg_buf_len]; // read 1mb of message at a time

  static constexpr int max_events{1024};

  epoll_event epoll_server_event{}, epoll_client_events[max_events];

  Handler handler;
};
