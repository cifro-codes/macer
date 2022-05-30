#pragma once

#include <cstdint>
#include "wire/traits.hpp"

namespace wire
{
  template<typename Tag, std::size_t N>
  struct fixed_bytes
  {
    char data[N];
  };

  template<typename Tag, std::size_t N>
  struct is_blob<fixed_bytes<Tag, N>>
    : std::true_type
  {};
}
