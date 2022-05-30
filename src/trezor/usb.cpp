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

#include "trezor/usb.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>
#include "error.hpp"
#include "host_info.hpp"
#include "logger.hpp"
#include "password.hpp"
#include "../usb.hpp"
#include "trezor/common.hpp"
#include "trezor/crypto.hpp"
#include "wire/protobuf.hpp"

namespace
{
  expect<void> read_buffer(usb::device& dev, span<std::uint8_t> dest)
  {
    return usb::read(dev, dest, std::chrono::seconds{0});
  }

  expect<void> send_buffer(usb::device& dev, byte_slice& bytes, span<std::uint8_t> buffer, const std::size_t offset)
  {
    assert(offset < buffer.size());
    const std::size_t next = std::min(bytes.size(), buffer.size() - offset);
    std::memcpy(buffer.data() + offset, bytes.data(), next);
    TRELOCK_CHECK(usb::write(dev, to_span(buffer), std::chrono::seconds{1}));
    bytes.remove_prefix(next);
    return success();
  }

  template<typename T>
  expect<void> send_message(usb::device& dev, const T& message)
  {
    byte_slice bytes = wire::protobuf::to_bytes(message);
    TRELOCK_PRECOND(bytes.size() <= std::numeric_limits<std::uint32_t>::max());

    std::uint8_t buffer[64] = {'?', '#', '#', 0};

    buffer[3] = std::uint16_t(message.id()) >> 8;
    buffer[4] = std::uint16_t(message.id()) & 0xFF;

    buffer[5] = (bytes.size() >> 24) & 0xFF;
    buffer[6] = (bytes.size() >> 16) & 0xFF;
    buffer[7] = (bytes.size() >> 8) & 0xFF;
    buffer[8] = bytes.size() & 0xFF;

    TRELOCK_CHECK(send_buffer(dev, bytes, buffer, 9));
    while (!bytes.empty())
      TRELOCK_CHECK(send_buffer(dev, bytes, buffer, 1));
    return success();
  }

  template<typename T>
  expect<void> send_password(usb::device& dev, const char* prompt)
  {
    expect<std::string> pass = password_prompt(prompt);
    if (!pass)
      return pass.error();
    return send_message(dev, T{std::move(*pass)});
  }

  expect<byte_slice> handle_failure(usb::device&, byte_slice&& bytes)
  {
    const auto message = wire::protobuf::from_bytes<trezor::failure>(std::move(bytes));
    if (!message)
      return message.error();
    fprintf(stderr, "Trezor failure: %s\n", message->message.c_str());
    return {trezor::error::device_failure};
  }
  expect<byte_slice> handle_pin(usb::device& dev, byte_slice&& bytes)
  {
    fprintf(stderr, "  7 8 9\n");
    fprintf(stderr, "  4 5 6\n");
    fprintf(stderr, "  1 2 3\n");
    TRELOCK_CHECK(send_password<trezor::pin_matrix_ack>(dev, "Trezor Pin"));
    return byte_slice{};
  }
  expect<byte_slice> handle_button(usb::device& dev, byte_slice&& bytes)
  {
    fprintf(stderr, "Check Trezor\n");
    TRELOCK_CHECK(send_message(dev, trezor::button_ack{}));
    return byte_slice{};
  }
  expect<byte_slice> handle_passphrase(usb::device& dev, byte_slice&& bytes)
  {
    TRELOCK_CHECK(send_password<trezor::passphrase_ack>(dev, "Trezor Passphrase:"));
    return byte_slice{};
  }
  expect<byte_slice> handle_signature(usb::device&, byte_slice&& bytes)
  {
    const auto message = wire::protobuf::from_bytes<trezor::signed_identity>(std::move(bytes));
    static_assert(sizeof(message->signature) == 65, "unexpected signature size");
    /* Trezor returns 65 byte signatures even though ed25519 produces 64-byte
       signature. The first byte is random garbage due to a bug in the Trezor v1
       firmware. */
    if (!message)
      return message.error();
    auto sig = as_byte_span(message->signature);
    sig.remove_prefix(1);
    return byte_slice{sig};
  }

  struct message_map
  {
    typedef expect<byte_slice>(*handler_func)(usb::device&, byte_slice&&);
    const handler_func handler;
    const trezor::message_id id;
  };
  bool operator<(const message_map& lhs, const trezor::message_id rhs) noexcept
  {
    return lhs.id < rhs;
  }

  constexpr const message_map handlers[] =
  {
    {handle_failure, trezor::message_id::failure},
    {handle_pin, trezor::message_id::pin_matrix_request},
    {handle_button, trezor::message_id::button_request},
    {handle_passphrase, trezor::message_id::passphrase_request},
    {handle_signature, trezor::message_id::signed_identity}
  };

  expect<byte_slice> read_message(usb::device& dev)
  {
    std::uint8_t buffer[64];
    TRELOCK_CHECK(read_buffer(dev, buffer));
    if (buffer[0] != '?' || buffer[1] != '#' || buffer[2] != '#')
      return {trezor::error::invalid_encoding};

    std::uint16_t id = std::uint16_t(buffer[3]) << 8;
    id |= buffer[4] & 0xFF;

    std::uint32_t remaining = std::uint32_t(buffer[5]) << 24;
    remaining |= std::uint32_t(buffer[6]) << 16;
    remaining |= std::uint32_t(buffer[7]) << 8;
    remaining |= buffer[8];

    byte_stream unpacked{};

    std::uint32_t next = std::min(std::uint32_t(sizeof(buffer) - 9), remaining);
    unpacked.write({buffer + 9, next});
    remaining -= next;

    while (remaining)
    {
      TRELOCK_CHECK(read_buffer(dev, buffer));
      if (buffer[0] != '?')
	return {trezor::error::invalid_encoding};

      next = std::min(std::uint32_t(sizeof(buffer) - 1), remaining);
      unpacked.write({buffer + 1, next});
      remaining -= next;
    }

    const auto found = std::lower_bound(std::begin(handlers), std::end(handlers), trezor::message_id(id));
    if (found == std::end(handlers))
      return {trezor::error::unsupported_message};

    return found->handler(dev, byte_slice{std::move(unpacked)});
  }
}

namespace trezor
{
  expect<::usb::interface> usb::select(span<const libusb_interface> interfaces)
  {
    for (const libusb_interface& intf : interfaces)
    {
      TRELOCK_LIBUSB_DEFENSIVE(intf.altsetting);
      for (unsigned i = 0; i < intf.num_altsetting; ++i)
      {
	const libusb_interface_descriptor& descriptor = intf.altsetting[i];
	if (descriptor.bInterfaceClass == LIBUSB_CLASS_HID)
	{
	  TRELOCK_LIBUSB_DEFENSIVE(descriptor.endpoint);
	  std::uint8_t in = 0;
	  std::uint8_t out = 0;
	  for (unsigned j = 0; j < descriptor.bNumEndpoints; ++j)
	  {
	    const libusb_endpoint_descriptor& endpoint = descriptor.endpoint[j];
	    if (endpoint.bmAttributes & LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT)
	    {
	      if ((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
		in = endpoint.bEndpointAddress;
	      else
		out = endpoint.bEndpointAddress;
	    }
	  }
	  return ::usb::interface{descriptor.bInterfaceNumber, in, out};
	}
      }
    }
    return {common_error::invalid_argument};
  }

  expect<byte_slice> usb::run(::usb::device& dev, const host_info& info)
  {
    // send request first
    {
      sign_identity request{
	{"macer", info.user, info.host}, "macer_luks_drive", info.message, "ed25519"
      };

      TRELOCK_CHECK(send_message(dev, request));
    }

    // process messages until signed response is received
    while (true)
    {
      expect<byte_slice> secret = read_message(dev);
      if (!secret || !secret->empty())
	return secret;
    }
    // unreachable;
  }
}
