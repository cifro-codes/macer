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
#include "wire/protobuf/fwd.hpp"

namespace trezor
{
  enum class message_id : std::uint32_t
  {
    success = 2,
    failure = 3,
    pin_matrix_request = 18,
    pin_matrix_ack = 19,
    button_request = 26,
    button_ack = 27,
    passphrase_request = 41,
    passphrase_ack = 42,
    sign_identity = 53,
    signed_identity = 54
  };
  
  struct failure
  {
    std::string message;
  };
  void read_bytes(wire::protobuf_reader& source, failure& self);


  struct button_ack
  {
    static constexpr message_id id() noexcept { return message_id::button_ack; }
  };
  void write_bytes(wire::protobuf_writer&, const button_ack&);

  struct button_request
  {
    enum class type : std::size_t
    {
      other = 1,
      fee_over_threshold,
      confirm_output,
      reset_device,
      confirm_word,
      wipe_device,
      protect_call,
      sign_tx,
      firmware_check,
      address,
      public_key,
      mnemonic_word_count,
      mnemonic_input,
      deprecated,
      unknown_derivation_path,
      recovery_homepage,
      success,
      warning,
      passphrase_entry,
      pin_entry
    };

    type code;
  };
  WIRE_PROTOBUF_DECLARE_ENUM(button_request::type);
  void read_bytes(wire::protobuf_reader&, button_request&);


  struct passphrase_ack
  {
    static constexpr message_id id() noexcept { return message_id::passphrase_ack; }
    std::string passphrase;
  };
  void write_bytes(wire::protobuf_writer& dest, const passphrase_ack& self);

  struct passphrase_request {};
  void read_bytes(wire::protobuf_reader& source, const passphrase_request&);


  struct pin_matrix_ack
  {
    static constexpr message_id id() noexcept { return message_id::pin_matrix_ack; }
    std::string pin;
  };
  void write_bytes(wire::protobuf_writer& dest, const pin_matrix_ack& self);

  struct pin_matrix_request {};
  void read_bytes(wire::protobuf_reader&, const pin_matrix_request&);


  struct success {};
  void read_bytes(wire::protobuf_reader&, const success&);
}
