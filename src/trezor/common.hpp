
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
