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

#include <array>
#include <cstdint>
#include <string>
#include "trezor/common.hpp"
#include "wire/fixed_bytes.hpp"
#include "wire/protobuf/fwd.hpp"

namespace trezor
{
  namespace tag
  {
    struct ed25519_public;
    struct ed25519_signature;

    struct x25519_public;
    struct x25519_session;
  }
  using ed25519_public = wire::fixed_bytes<tag::ed25519_public, 32>;
  using ed25519_signature = wire::fixed_bytes<tag::ed25519_signature, 65>;

  using x25519_public = wire::fixed_bytes<tag::x25519_public, 33>;
  using x25519_session = wire::fixed_bytes<tag::x25519_session, 33>;

  static constexpr std::uint32_t hardened_path = 0x80000000;

  struct get_public_key
  {
    static constexpr message_id id() noexcept { return message_id::get_public_key; }

    std::string curve_name;
    std::array<std::uint32_t, 5> address_n;
  };
  void write_bytes(wire::protobuf_writer& dest, const get_public_key& source);

  struct public_key
  {
    struct hd_node
    {
      x25519_public public_key;
    } node;
  };
  void read_bytes(wire::protobuf_reader& source, public_key::hd_node& dest);
  void read_bytes(wire::protobuf_reader& source, public_key& dest);

  struct identity
  {
    std::string protocol;
    std::string user;
    std::string host;
  };
  void write_bytes(wire::protobuf_writer& dest, const identity& self);


  struct sign_identity
  {
    static constexpr message_id id() noexcept { return message_id::sign_identity; }

    identity ident;
    std::string challenge_hidden;
    std::string challenge_visual;
    std::string curve_name;
  };
  void write_bytes(wire::protobuf_writer& dest, const sign_identity&);

  struct signed_identity
  {
    ed25519_signature signature;
  };
  void read_bytes(wire::protobuf_reader& source, signed_identity& self);


  struct get_ecdh_session
  {
    static constexpr message_id id() noexcept { return message_id::get_ecdh_session; }

    identity ident;
    std::string curve_name;
    x25519_public peer_key;
  };
  void write_bytes(wire::protobuf_writer& dest, const get_ecdh_session& source);

  struct ecdh_session
  {
    x25519_session secret_key;
    x25519_public public_key;
  };
  void read_bytes(wire::protobuf_reader& source, ecdh_session& dest);
}
