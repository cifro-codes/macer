// Copyright (c) 2022, Cifro Codes
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cstdint>

#include "byte_stream.hpp"
#include "span.hpp"
#include "wire/field.hpp"
#include "wire/protobuf/base.hpp"
#include "wire/traits.hpp"
#include "wire/write.hpp"

namespace wire
{
  //! Writes protobuf tags one-at-a-time for DOMless output.
  class protobuf_writer final : public writer
  {
    struct object_data
    {
      object_data()
	: stream(), id()
      {}

      byte_stream stream;
      unsigned id;
    };
    std::unique_ptr<object_data[]> objects_;
    std::size_t index_;
    unsigned last_id_;

    void write_tag(protobuf::type);

  public:
    protobuf_writer();

    protobuf_writer(const protobuf_writer&) = delete;
    virtual ~protobuf_writer() noexcept;
    protobuf_writer& operator=(const protobuf_writer&) = delete;

    void integer(int) override final;
    void integer(std::intmax_t) override final;

    void unsigned_integer(unsigned) override final;
    void unsigned_integer(std::uintmax_t) override final;

    void real(double) override final;

    void string(span<const char> source) override final;
    void binary(span<const std::uint8_t> source) override final;

    void enumeration(std::size_t index, span<char const* const>) override final;

    void start_array(std::size_t) override final;
    void end_array() override final;

    void start_object(std::size_t) override final;
    void key(std::uintmax_t) override final;
    void key(const char*) override final;
    void key(unsigned, const char*) override final;
    void end_object() override final;

    byte_slice take_bytes();
  };

  template<typename T>
  byte_slice protobuf::to_bytes(const T& source)
  {
    return wire_write::to_bytes<protobuf_writer>(source);
  }

  template<typename T, typename F = identity_>
  inline void array(protobuf_writer& dest, const T& source, F filter = F{})
  {
    // works with "lazily" computed ranges
    wire_write::array(dest, source, 0, std::move(filter));
  }
  template<typename T, typename F>
  inline void write_bytes(protobuf_writer& dest, as_array_<T, F> source)
  {
    wire::array(dest, source.get_value(), std::move(source.filter));
  }
  template<typename T>
  inline enable_if<is_array<T>::value> write_bytes(protobuf_writer& dest, const T& source)
  {
    wire::array(dest, source);
  }

  template<typename T, typename F = identity_, typename G = identity_>
  inline void dynamic_object(protobuf_writer& dest, const T& source, F key_filter = F{}, G value_filter = G{})
  {
    // works with "lazily" computed ranges
    wire_write::dynamic_object(dest, source, 0, std::move(key_filter), std::move(value_filter));
  }
  template<typename T, typename F, typename G>
  inline void write_bytes(protobuf_writer& dest, as_object_<T, F, G> source)
  {
    wire::dynamic_object(dest, source.get_map(), std::move(source.key_filter), std::move(source.value_filter));
  }

  template<typename... T>
  inline void object(protobuf_writer& dest, T... fields)
  {
    wire_write::object(dest, std::move(fields)...);
  }
}
