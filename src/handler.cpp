#include "handler.hpp"
#include "parser.hpp"

Handler::Handler() {};

Handler::~Handler() {};

int Handler::handle(Conn &conn) {
  auto [state, n] = parser::parse(conn.msg_buf, conn.msg_buf.size(), this->cmd);
  if (state == parser::state::IN_COMPLETE) {
    return 0;
  }

  if (state == parser::state::IN_VALID) {
  }
}
