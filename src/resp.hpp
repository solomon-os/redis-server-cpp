#pragma once
#include <string>
#include <string_view>

namespace resp {
void error(std::string &out, const std::string_view msg) noexcept;
void simple_string(std::string &out, const std::string_view msg) noexcept;

void bulk_string(std::string &out, const std::string_view msg) noexcept;
} // namespace resp
