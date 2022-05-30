
#pragma once

#include <system_error>

namespace wire
{
namespace error
{
  //! Type wrapper to "grab" rapidjson errors
  enum class protobuf : int
  {
    invalid_encoding = 0,
    unrecognized_type
  };

  //! \return Static string describing error `value`.
  const char* get_string(protobuf value) noexcept;

  //! \return Category for protobuf generated errors.
  const std::error_category& protobuf_category() noexcept;

  //! \return Error code with `value` and `protobuf_category()`.
  inline std::error_code make_error_code(protobuf value) noexcept
  {
    return std::error_code{int(value), protobuf_category()};
  }
}
}

namespace std
{
  template<>
  struct is_error_code_enum<wire::error::protobuf>
    : true_type
  {};
}
