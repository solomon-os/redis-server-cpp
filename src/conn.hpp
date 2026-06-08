#pragma once
#include <cstddef>
#include <fcntl.h>
#include <string>
#include <unistd.h>

// Owns a connected client socket fd (RAII: closed in the destructor).
class Conn {
public:
  explicit Conn(int fd);
  ~Conn();
  int read(char *buf, size_t cap);
  // Owns an fd, so forbid copying (would double-close). Held via unique_ptr,
  // so no move ops are needed.
  Conn(const Conn &) = delete;
  Conn &operator=(const Conn &) = delete;

  inline int register_non_blocking();

  const int fd;
  std::string message_buf;
};
