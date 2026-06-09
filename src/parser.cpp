
#include "parser.hpp"
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <tuple>

using namespace std;
using namespace parser;

tuple<parser::state, size_t>
parser_array_resp(const string_view &msg, size_t size, parser::command &cmd) {
  uint32_t str_pointer = 0;
  ++str_pointer;

  size_t end_of_instruction = msg.find("\r\n", str_pointer);
  if (end_of_instruction == string::npos) {
    return {state::IN_COMPLETE, 0};
  }

  int total_array_elements = 0;
  auto [ptr, ec] =
      from_chars(msg.data() + str_pointer, msg.data() + end_of_instruction + 1,
                 total_array_elements);

  if (ec == errc{}) {
    return {state::IN_VALID, 0};
  }

  str_pointer += end_of_instruction + 4; // pointer at '\r'
  state parsed_state = state::IN_COMPLETE;
  int total_args = 0;

  while (str_pointer + 2 < size && parsed_state == state::IN_COMPLETE) {
    ++str_pointer; // advance to $
    if (msg[str_pointer] != '$') {
      parsed_state = state::IN_VALID;
      break;
    }
    ++str_pointer; // advance to the next char

    size_t end_of_instruction = msg.find("\r\n", str_pointer);
    if (end_of_instruction == string::npos) {
      parsed_state = state::IN_COMPLETE;
      break;
    }

    int string_count = 0;

    auto [_, ec] =
        from_chars(msg.data() + str_pointer,
                   msg.data() + end_of_instruction + 1, string_count);
    if (ec == errc{}) {
      parsed_state = state::IN_VALID;
      break;
    }

    // move to string pinter
    str_pointer += end_of_instruction + 5;
    if (str_pointer + string_count >= size) {
      parsed_state = state::IN_COMPLETE;
      break;
    }

    cmd.args.push_back(msg.substr(str_pointer, str_pointer + string_count + 1));
    ++total_args;
    if (total_args == total_array_elements) {
      parsed_state = state::VALID;
    }
  }

  return {parsed_state, total_args};
}

tuple<parser::state, size_t> parse_simple_string(const string_view &msg,
                                                 size_t size, command &cmd) {
  int idx = 0;
  ++idx;
  int edx = msg.find("\r\n");
  if (edx == string::npos) {
    return {state::IN_COMPLETE, 0};
  }

  auto str = msg.substr(idx, edx + 1);
  cmd.args.push_back(str);
  return {state::VALID, 1};
}

tuple<parser::state, size_t> parser::parse(const string_view &msg, size_t size,
                                           command &cmd) {

  if (size <= 1) {
    return {parser::state::IN_COMPLETE, 0};
  }

  switch (msg[0]) {
  case '*':
    return parser_array_resp(msg, size, cmd);
  case '+':
    return parse_simple_string(msg, size, cmd);
  };
  return {parser::state::IN_VALID, 0};
}
