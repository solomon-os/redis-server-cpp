#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace parser {

enum class state { IN_COMPLETE, VALID, IN_VALID };

struct command {
  std::string_view name;
  std::vector<std::string_view> args;
};

std::tuple<state, size_t> parse(const std::string_view &msg, size_t size,
                                command &cmd);
} // namespace parser
