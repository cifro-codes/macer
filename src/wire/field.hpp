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

#include <functional>
#include <utility>

#include "wire/traits.hpp"

//! A required field has the same key name and C/C++ name
#define WIRE_FIELD(id, name)				\
  ::wire::field<id>( #name , std::ref( self . name ))

//! A required field has the same key name and C/C++ name AND is cheap to copy (faster output).
#define WIRE_FIELD_COPY(id, name)		\
  ::wire::field<id>( #name , self . name )

//! The optional field has the same key name and C/C++ name
#define WIRE_OPTIONAL_FIELD(id, name)				\
  ::wire::optional_field<id>( #name , std::ref( self . name ))

namespace wire
{
  template<typename T>
  struct unwrap_reference
  {
    using type = T;
  };

  template<typename T>
  struct unwrap_reference<std::reference_wrapper<T>>
  {
    using type = T;
  };


  //! Links `name` to a `value` for object serialization.
  template<typename T, unsigned Id, bool Required>
  struct field_
  {
    using value_type = typename unwrap_reference<T>::type;
    static constexpr bool is_required() noexcept { return Required; }
    static constexpr unsigned id() noexcept { return Id; }

    const char* name;
    T value;

    //! \return `value` with `std::reference_wrapper` removed.
    constexpr const value_type& get_value() const noexcept
    {
      return value;
    }

    //! \return `value` with `std::reference_wrapper` removed.
    value_type& get_value() noexcept
    {
      return value;
    }
  };

  //! Links `name` to `value`. Use `std::ref` if de-serializing.
  template<unsigned Id, typename T>
  constexpr inline field_<T, Id, true> field(const char* name, T value)
  {
    return {name, std::move(value)};
  }

  //! Links `name` to `value`. Use `std::ref` if de-serializing.
  template<unsigned Id, typename T>
  constexpr inline field_<T, Id, false> optional_field(const char* name, T value)
  {
    return {name, std::move(value)};
  }


  template<typename T, unsigned Id>
  inline constexpr bool available(const field_<T, Id, true>&) noexcept
  {
    return true;
  }
  template<typename T, unsigned Id>
  inline bool available(const field_<T, Id, false>& elem)
  {
    return bool(elem.get_value());
  }


  // example usage : `wire::sum(std::size_t(wire::available(fields))...)`

  inline constexpr int sum() noexcept
  {
    return 0;
  }
  template<typename T, typename... U>
  inline constexpr T sum(const T head, const U... tail) noexcept
  {
    return head + sum(tail...);
  }
}

