
#include "logger.hpp"

#include <cstring>
#include <mutex>
#include <string>

namespace
{
  std::mutex log_sync;
}

void print_error(const std::error_code code, const char* file, const int line, const char* additional)
{
  const char* separator = " : ";
  if (!file)
    file = "unknown file";
  if (!additional)
    separator = "";

  const char* filename = std::strrchr(file, '/');
  if (filename)
    ++filename;
  else
    filename = file;

  const std::string msg = code.message();
  const std::lock_guard<std::mutex> lock{log_sync};
  fprintf(stderr, "(%s:%i) %s%s%s\n", filename, line, msg.c_str(), separator, additional);
}
