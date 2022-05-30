
#include "trezor/error.hpp"

#include <string>

namespace trezor
{
  const char* get_string(const error value) noexcept
  {
    switch(value)
    {
    case error::device_failure:
      return "Device returned failure";
    case error::invalid_encoding:
      return "Invalid byte encoding over USB";
    case error::unsupported_message:
      return "Unsupported Trezor message";
    default:
      break;
    }
    return "Unknown trezor error";
  }

  const std::error_category& error_category() noexcept
  {
    struct category final : std::error_category
    {
      virtual const char* name() const noexcept override final
      {
        return "trezor::error_category()";
      }

      virtual std::string message(int value) const override final
      {
        return get_string(error(value));
      }
    };
    static const category instance{};
    return instance;
  }
} // trezor
