#pragma once
#include <string>

namespace resp {
void error(std::string &out, const std::string_view msg);
void simple_string(std::string &out, const std::string_view msg);
} // namespace resp
