#pragma once

#include <system_error>

namespace trezor
{
  //! Type wrapper to "grab" rapidjson errors
  enum class error : int
  {
    success = 0, // per expect<T> requirements
    device_failure,
    invalid_encoding,
    unsupported_message
  };

  //! \return Static string describing error `value`.
  const char* get_string(error value) noexcept;

  //! \return Category for trezor generated errors.
  const std::error_category& error_category() noexcept;

  //! \return Error code with `value` and `error_category()`.
  inline std::error_code make_error_code(error value) noexcept
  {
    return std::error_code{int(value), error_category()};
  }
}

namespace std
{
  template<>
  struct is_error_code_enum<trezor::error>
    : true_type
  {};
}
