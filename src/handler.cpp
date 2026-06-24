#include "handler.hpp"
#include "parser.hpp"
#include "resp.hpp"
#include <cctype>
#include <format>
#include <iostream>

using namespace std::string_view_literals;
using namespace std;

int handle_echo(Conn &conn, parser::command &cmd, size_t total_read) noexcept {
  if (cmd.args.size() < 2) {
    resp::simple_string(conn.msg_out_buf, "Invalid command arguments"sv);
    auto n = conn.flush(total_read);
    cmd.args.clear();
  }
  resp::bulk_string(conn.msg_out_buf, cmd.args[1]);

  cout << format("{:?}", conn.msg_out_buf) << endl;

  auto n = conn.flush(total_read);
  cmd.args.clear();
  return n;
}

int handle_set(Conn &conn, parser::command &cmd, size_t total_read) noexcept {
  if (cmd.args.size() < 2) {
    resp::simple_string(conn.msg_out_buf, "Invalid command arguments"sv);
    auto n = conn.flush(total_read);
    cmd.args.clear();
    return n;
  }
}
int Handler::handle(Conn &conn) noexcept {
  auto [state, args, total_read] =
      parser::parse(conn.msg_in_buf, conn.msg_in_buf.size(), this->cmd);
  if (state == parser::state::IN_COMPLETE) {
    return 0;
  }

  if (state == parser::state::IN_VALID) {
    resp::simple_string(conn.msg_out_buf, "Command not supported"sv);
    conn.msg_in_buf.clear();
    return conn.flush(0);
  }

  auto &command = cmd.args[0];

  if (cmd.args[0] == "PING") {
    resp::simple_string(conn.msg_out_buf, "PONG"sv);
    cmd.args.clear();
    return conn.flush(total_read);
  }

  if (command == "ECHO") {
    return handle_echo(conn, cmd, total_read);
  }

  resp::simple_string(conn.msg_out_buf, "Command not supported"sv);
  cmd.args.clear();
  conn.msg_in_buf.clear();
  return conn.flush(0);
}
