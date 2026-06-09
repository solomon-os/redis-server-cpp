#include "resp.hpp"

using namespace std;
using namespace string_view_literals;
void resp::error(string &out, const string_view msg) {
  out.append("-ERR "sv).append(msg).append("\r\n");
}

void resp::simple_string(string &out, const string_view msg) {
  out.append("+").append(msg).append("\r\n");
}
