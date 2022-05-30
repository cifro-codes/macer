#pragma once

#include <string>
#include "expect.hpp"

bool is_cout_tty() noexcept;
expect<std::string> password_prompt(const char* message, bool confirm = false);
