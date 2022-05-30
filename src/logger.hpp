
#pragma once
#include <system_error>

#define TRELOCK_LOG_ERROR(code, ...)				\
  ::print_error(code, __FILE__, __LINE__,  ## __VA_ARGS__ )

void print_error(std::error_code code, const char* file, int line, const char* msg = nullptr);

