#pragma once
#include "conn.hpp"
#include "parser.hpp"

class Handler {

public:
  Handler();

  ~Handler();

  void handle(Conn &conn);

  parser::command cmd;
};
