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
