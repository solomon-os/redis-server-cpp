#include "handler.hpp"
#include "parser.hpp"
#include "resp.hpp"
#include <cctype>

using namespace std::string_view_literals;
using namespace std;

Handler::Handler() {};

Handler::~Handler() {};

int Handler::handle(Conn &conn) {
  auto [state, args, total_read] =
      parser::parse(conn.msg_in_buf, conn.msg_in_buf.size(), this->cmd);
  if (state == parser::state::IN_COMPLETE) {
    return 0;
  }

  if (state == parser::state::IN_VALID) {
    resp::simple_string(conn.msg_out_buf, "Command not supported"sv);
    conn.msg_in_buf.clear();
    return conn.flush();
  }

  if (cmd.args[0] == "PING") {
    resp::simple_string(conn.msg_out_buf, "PONG"sv);
    conn.flush();
    conn.msg_in_buf.erase(0, total_read);
    return 0;
  }

  resp::simple_string(conn.msg_out_buf, "Command not supported"sv);
  conn.msg_in_buf.clear();
  return conn.flush();
}
