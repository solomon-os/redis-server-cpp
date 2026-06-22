#pragma once
#include "conn.hpp"
#include "parser.hpp"

class Handler {

public:
  int handle(Conn &conn) noexcept;

  parser::command cmd;
};
