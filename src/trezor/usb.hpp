#pragma once

#include <cstdint>
#include "byte_slice.hpp"
#include "expect.hpp"
#include "../usb.hpp"

struct host_info;
struct libusb_interface;

namespace trezor
{
  constexpr const std::uint16_t vendor_id = 0x534c;
  constexpr const std::uint16_t device_v1_id = 0x01;
  constexpr const std::uint16_t device_v2_id = 0x02;
  constexpr const std::uint16_t devices[] = {device_v1_id, device_v2_id};

  struct usb
  {
    static expect<::usb::interface> select(span<const libusb_interface> interfaces);
    static expect<byte_slice> run(::usb::device& dev, const host_info& info);
  };
}
