// Copyright (c) 2020, The Monero Project
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

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include "byte_slice.hpp"
#include "span.hpp"
#include "wire/error.hpp"
#include "wire/field.hpp"
#include "wire/traits.hpp"

namespace wire
{
  //! Interface for converting C/C++ objects to "wire" (byte) formats.
  struct writer
  {
    writer() = default;

    virtual ~writer() noexcept;

    virtual void integer(int) = 0;
    virtual void integer(std::intmax_t) = 0;

    virtual void unsigned_integer(unsigned) = 0;
    virtual void unsigned_integer(std::uintmax_t) = 0;

    virtual void real(double) = 0;

    virtual void string(span<const char> str) = 0;
    virtual void binary(span<const std::uint8_t> bytes) = 0;

    virtual void start_array(std::size_t) = 0;
    virtual void end_array() = 0;

    virtual void start_object(std::size_t) = 0;
    virtual void key(std::uintmax_t) = 0;
    virtual void key(const char*) = 0;
    virtual void key(unsigned, const char*) = 0;
    virtual void end_object() = 0;

  protected:
    writer(const writer&) = default;
    writer(writer&&) = default;
    writer& operator=(const writer&) = default;
    writer& operator=(writer&&) = default;
  };

  // leave in header, compiler can de-virtualize when final type is given

  template<typename W>
  inline void write_bytes(W& dest, const int source)
  {
    static_assert(std::is_same<W, void>::value, "protobuf_writer does not support signed integers");
    dest.integer(source);
  }
  template<typename W>
  inline void write_bytes(W& dest, const long source)
  {
    static_assert(std::is_same<W, void>::value, "protobuf_writer does not support signed integers");
    dest.integer(std::intmax_t(source));
  }
  template<typename W>
  inline void write_bytes(W& dest, const long long source)
  {
    static_assert(std::is_same<W, void>::value, "protobuf_writer does not support signed integers");
    dest.integer(std::intmax_t(source));
  }

  template<typename W>
  inline void write_bytes(W& dest, const unsigned source)
  {
    dest.unsigned_integer(source);
  }
  template<typename W>
  inline void write_bytes(W& dest, const unsigned long source)
  {
    dest.unsigned_integer(std::uintmax_t(source));
  }
  template<typename W>
  inline void write_bytes(W& dest, const unsigned long long source)
  {
    dest.unsigned_integer(std::uintmax_t(source));
  }

  template<typename W>
  inline void write_bytes(W& dest, const double source)
  {
    static_assert(std::is_same<W, void>::value, "protobuf_writer does not support doubles"); 
    dest.real(source);
  }

  template<typename W>
  inline void write_bytes(W& dest, const span<const char> source)
  {
    dest.string(source);
  }
  template<typename W>
  inline void write_bytes(W& dest, const std::string& source)
  {
    dest.string(to_span(source));
  }
  template<typename W>
  inline void write_bytes(W& dest, const char* source)
  {
    dest.string({source, std::strlen(source)});
  }

  template<typename W, typename T>
  inline enable_if<is_blob<T>::value> write_bytes(W& dest, const T& source)
  {
    dest.binary(as_byte_span(source));
  }

  template<typename W>
  inline void write_bytes(W& dest, const span<const std::uint8_t> source)
  {
    dest.binary(source);
  }
}

namespace wire_write
{
  /*! Don't add a function called `write_bytes` to this namespace, it will
      prevent ADL lookup. ADL lookup delays the function searching until the
      template is used instead of when its defined. This allows the unqualified
      calls to `write_bytes` in this namespace to "find" user functions that are
      declared after these functions. */

  template<typename W, typename T>
  inline void bytes(W& dest, const T& source)
  {
    write_bytes(dest, source); // ADL (searches every associated namespace)
  }

  template<typename W, typename T, typename U>
  inline std::error_code to_bytes(T& dest, const U& source)
  {
    try
    {
      W out{std::move(dest)};
      bytes(out, source);
      dest = out.take_sink();
    }
    catch (const wire::exception& e)
    {
      dest.clear();
      return e.code();
    }
    catch (...)
    {
      dest.clear();
      throw;
    }
    return {};
  }

  template<typename W, typename T>
  inline std::error_code to_bytes(byte_slice& dest, const T& source)
  {
    byte_stream sink{};
    const std::error_code error = wire_write::to_bytes<W>(sink, source);
    if (error)
    {
      dest = nullptr;
      return error;
    }
    dest = byte_slice{std::move(sink)};
    return {};
  }

  template<typename W, typename T>
  inline void array(W& dest, const T& source, const std::size_t count)
  {
    using value_type = typename T::value_type;
    static_assert(!std::is_same<value_type, char>::value, "write array of chars as binary");
    static_assert(!std::is_same<value_type, std::uint8_t>::value, "write array of unsigned chars as binary");

    dest.start_array(count);
    for (const auto& elem : source)
      bytes(dest, elem);
    dest.end_array();
  }

  template<typename W, typename T, unsigned Id>
  inline bool field(W& dest, const wire::field_<T, Id, true> elem)
  {
    dest.key(elem.id(), elem.name);
    bytes(dest, elem.get_value());
    return true;
  }

  template<typename W, typename T, unsigned Id>
  inline bool field(W& dest, const wire::field_<T, Id, false> elem)
  {
    if (bool(elem.get_value()))
    {
      dest.key(elem.id(), elem.name);
      bytes(dest, *elem.get_value());
    }
    return true;
  }

  template<typename W, typename... T>
  inline void object(W& dest, T... fields)
  {
    dest.start_object(wire::sum(std::size_t(wire::available(fields))...));
    const bool dummy[] = {field(dest, std::move(fields))...};
    dest.end_object();
  }
} // wire_write

namespace wire
{
  template<typename W, typename T>
  inline void array(W& dest, const T& source)
  {
    wire_write::array(dest, source, source.size());
  }
  template<typename W, typename T>
  inline enable_if<is_array<T>::value> write_bytes(W& dest, const T& source)
  {
    wire::array(dest, source);
  }

  template<typename W, typename... T>
  inline enable_if<std::is_base_of<writer, W>::value> object(W& dest, T... fields)
  {
    wire_write::object(dest, std::move(fields)...);
  }
}
