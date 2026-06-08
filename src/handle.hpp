#pragma once

#include "conn.hpp"
#include <unordered_map>

class Handle {
public:
  Handle();

  ~Handle();

  std::unordered_map<int, Conn> conns;
};
