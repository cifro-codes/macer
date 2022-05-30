
#include "trezor/crypto.hpp"

#include "wire/protobuf.hpp"

namespace trezor
{
  void write_bytes(wire::protobuf_writer& dest, const identity& self)
  {
    wire::object(dest, WIRE_FIELD(1, protocol), WIRE_FIELD(2, user), WIRE_FIELD(3, host));
  }

  void write_bytes(wire::protobuf_writer& dest, const sign_identity& self)
  {
    wire::object(dest,
      WIRE_FIELD(1, ident),
      WIRE_FIELD(2, challenge_hidden),
      WIRE_FIELD(3, challenge_visual),
      WIRE_FIELD(4, curve_name)
    );
  }

  void read_bytes(wire::protobuf_reader& source, signed_identity& self)
  {
    wire::object(source, WIRE_FIELD(3, signature));
  }
}
