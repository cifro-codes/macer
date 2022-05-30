#include "usb/base.hpp"

#include "usb/error.hpp"


namespace usb
{


  device_list get_device_list(libusb_context& ctx)
  {
    libusb_device** handle = nullptr;
    TRELOCK_LIBUSB_CHECK(nullptr, libusb_get_device_list(std::addressof(ctx), std::addressof(handle)));
    return device_list{handle};
  }

  device open(libusb_device& dev)
  {
    libusb_device_handle* handle = nullptr;
    TRELOCK_LIBUSB_CHECK(nullptr, libusb_open(std::addressof(dev), std::addressof(handle)));
    return device{handle};
  }

  expect<libusb_device_descriptor> get_descriptor(libusb_device& dev)
  {
    return descriptor;
  }
}
