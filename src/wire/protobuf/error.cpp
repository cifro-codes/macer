
#include "wire/protobuf/error.hpp"

#include <string>

namespace wire
{
namespace error
{
  const char* get_string(const protobuf value) noexcept
  {

    switch(value)
    {
    default:
      break;
    }
    return "Unknown protobuf parser error";
  }

  const std::error_category& protobuf_category() noexcept
  {
    struct category final : std::error_category
    {
      virtual const char* name() const noexcept override final
      {
        return "wire::error::rapidjson_category()";
      }

      virtual std::string message(int value) const override final
      {
        return get_string(protobuf(value));
      }
    };
    static const category instance{};
    return instance;
  }
} // error
} // wire
