
#pragma once

#include <cstdint>
#include <string>

#include "byte_slice.hpp"
#include "expect.hpp"
#include "wire/protobuf/fwd.hpp"

namespace wire
{
  struct protobuf
  {
    enum class type : std::uint8_t
    {
      bytes = 2,
      fixed32 = 5,
      fixed64 = 1,
      varint = 0
    };

    using input_type = protobuf_reader;
    using output_type = protobuf_writer;

    template<typename T>
    static expect<T> from_bytes(byte_slice&& source);

    template<typename T>
    static byte_slice to_bytes(const T& source);
  };
}
