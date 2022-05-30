
#pragma once

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
  }
  using ed25519_public = wire::fixed_bytes<tag::ed25519_public, 32>;
  using ed25519_signature = wire::fixed_bytes<tag::ed25519_signature, 65>;

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
}
