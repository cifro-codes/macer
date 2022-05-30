#pragma once

#include <libusb-1.0/libusb.h>
#include <memory>
#include "expect.hpp"
#include "logger.hpp"

namespace usb
{
  struct context_exit
  {
    void operator()(libusb_context* ptr) const noexcept
    {
      if (ptr)
	libusb_exit(ptr);
    }
  };


  using context = std::unique_ptr<libusb_context, context_exit>;
  context make_context();


}
