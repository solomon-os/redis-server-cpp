#include "resp.hpp"
#include <charconv>
#include <cstddef>
#include <string_view>

using namespace std;
using namespace string_view_literals;

void append_int_to_string(string &str, size_t num) noexcept {

  char buf[20];
  auto result = to_chars(buf, buf + sizeof(buf), num);
  str.append(buf, result.ptr - buf);
}
void resp::error(string &out, const string_view msg) noexcept {
  out.append("-ERR "sv).append(msg).append("\r\n");
}

void resp::simple_string(string &out, const string_view msg) noexcept {
  out.append("+").append(msg).append("\r\n");
}

void resp::bulk_string(string &out, const string_view msg) noexcept {

  out.append("$");
  append_int_to_string(out, msg.length());

  out.append("\r\n").append(msg).append("\r\n");
}
