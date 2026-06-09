#include "handler.hpp"
#include "parser.hpp"

Handler::Handler() {};

Handler::~Handler() {};

void Handler::handle(Conn &conn) {
  auto [state, n] = parser::parse(conn.msg_buf, conn.msg_buf.size(), this->cmd);
}
