#pragma once
#include "conn.hpp"
#include "parser.hpp"

class Handler {

public:
  Handler();

  ~Handler();

  int handle(Conn &conn);

  parser::command cmd;
};
