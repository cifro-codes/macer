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

#include <type_traits>

#include "wire/protobuf/base.hpp"
#include "wire/protobuf/error.hpp"
#include "wire/protobuf/read.hpp"
#include "wire/protobuf/write.hpp"

// Protobuf allows custom integers on the wire, and the C++ interface uses
// `std::size_t` for enumerations. Switch to runtime checks if another
// enum type is absolutely needed.
#define WIRE_PROTOBUF_DEFINE_ENUM(type_)				\
  static_assert(std::is_enum<type_>::value, "expected enum type");	\
  static_assert(0 <= std::numeric_limits<std::underlying_type<type_>::type>::min(), "underyling enum type is signed"); \
  static_assert(std::numeric_limits<std::underlying_type<type_>::type>::max() <= std::numeric_limits<std::size_t>::max(), "underlying enum type too small"); \
  void read_bytes(::wire::protobuf_reader& source, type_& dest)		\
  {									\
    dest = type_(source.enumeration({}));				\
  }									\
  void write_bytes(::wire::protobuf_writer& dest, const type_ source)	\
  {									\
   dest.enumeration(std::size_t(source), {});				\
  }

#define WIRE_PROOBUF_DEFINE_OBJECT(type, map)				\
  void read_bytes(::wire::protobuf_reader& source, type& dest)		\
  {                                                                     \
    map(source, dest);                                                  \
  }                                                                     \
  void write_bytes(::wire::protobuf_writer& dest, const type& source)	\
  {                                                                     \
    map(dest, source);                                                  \
  }

