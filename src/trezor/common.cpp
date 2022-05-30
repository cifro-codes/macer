
#include "trezor/common.hpp"

#include "wire/protobuf.hpp"

namespace trezor
{
  void read_bytes(wire::protobuf_reader& source, failure& self)
  {
    wire::object(source, WIRE_FIELD(2, message));
  }

  WIRE_PROTOBUF_DEFINE_ENUM(button_request::type);
  void write_bytes(wire::protobuf_writer& dest, const button_ack&)
  {
    wire::object(dest);
  }
  void read_bytes(wire::protobuf_reader& source, button_request& self)
  {
    wire::object(source, WIRE_FIELD(1, code));
  }

  void write_bytes(wire::protobuf_writer& dest, const passphrase_ack& self)
  {
    wire::object(dest, WIRE_FIELD(1, passphrase));
  }
  void read_bytes(wire::protobuf_reader& source, const passphrase_request&)
  {
    wire::object(source);
  }

  void write_bytes(wire::protobuf_writer& dest, const pin_matrix_ack& self)
  {
    wire::object(dest, WIRE_FIELD(1, pin));
  }
  void read_bytes(wire::protobuf_reader& source, const pin_matrix_request&)
  {
    wire::object(source);
  }

  void read_bytes(wire::protobuf_reader& source, const success&)
  {
    wire::object(source);
  }
}
