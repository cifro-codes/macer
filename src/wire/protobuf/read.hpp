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
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "expect.hpp"
#include "span.hpp"
#include "wire/field.hpp"
#include "wire/protobuf/base.hpp"
#include "wire/read.hpp"
#include "wire/traits.hpp"

namespace wire
{
  //! Reads protobufs elements at a time for DOMless parsing
  class protobuf_reader : public reader
  {
    byte_slice source_;
    std::unique_ptr<span<const std::uint8_t>[]> objects_;
    protobuf::type last_type_;

    void check_bounds(const char* function);

  public:
    explicit protobuf_reader(byte_slice&& source);

    //! \throw wire::exception if protubuf parsing is incomplete.
    void check_complete() const override final;

    //! \throw wire::exception if next token not a boolean.
    bool boolean() override final;

    //! \throw wire::expception if next token not an integer.
    std::intmax_t integer() override final;

    //! \throw wire::exception if next token not an unsigned integer.
    std::uintmax_t unsigned_integer() override final;

    //! \throw wire::exception if next token not a valid real number
    double real() override final;

    //! \throw wire::exception if next token not a string
    std::string string() override final;

    //! \throw wire::exception if next token cannot be read as hex
    byte_slice binary() override final;

    //! \throw wire::exception if next token cannot be read as hex into `dest`.
    void binary(span<std::uint8_t> dest) override final;

    //! \throw wire::exception if next token not `[`.
    std::size_t start_array() override final;

    //! Skips whitespace to next token. \return True if next token is eof or ']'.
    bool is_array_end(std::size_t count) override final;


    //! \throw wire::exception if next token not `{`.
    std::size_t start_object() override final;

    /*! \throw wire::exception if next token not key or `}`.
        \param[out] index of key match within `map`.
        \return True if another value to read. */
    bool key(span<const key_map> map, std::size_t&, std::size_t& index) override final;
  };

  template<typename T>
  expect<T> protobuf::from_bytes(byte_slice&& bytes)
  {
    return wire_read::from_bytes<input_type, T>(std::move(bytes));
  }
} // wire

