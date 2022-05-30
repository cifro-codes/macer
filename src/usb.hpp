#pragma once

#include <chrono>
#include <cstdint>
#include <libusb-1.0/libusb.h>
#include <memory>
#include <system_error>
#include "byte_slice.hpp"
#include "expect.hpp"
#include "span.hpp"

#define TRELOCK_LIBUSB_DEFENSIVE(ptr)		\
  do						\
  {						\
    if (ptr == nullptr)				\
      return {common_error::invalid_argument};	\
  } while (0)

struct host_info;

namespace usb
{
  //! Wrapper for `libusb_error`
  enum class error : int {};

  //! \return Static string describing error `value`.
  inline const char* get_string(usb::error value) noexcept
  {
    return libusb_strerror(libusb_error(value));
  }

  //! \return Category for libusb generated errors.
  const std::error_category& error_category() noexcept;

  //! \return Error code with `value` and `usb::error_category()`.
  inline std::error_code make_error_code(usb::error value) noexcept
  {
    return std::error_code{int(value), error_category()};
  }

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

  class device;

  struct interface
  {
    int number;
    std::uint8_t in_endpoint;
    std::uint8_t out_endpoint;
  };

  
  expect<void> read(device& source, span<std::uint8_t> dest, std::chrono::milliseconds timeout);
  expect<void> write(device& dest, span<const std::uint8_t> source, std::chrono::milliseconds timeout);

  expect<byte_slice> run(libusb_context& ctx, const host_info& info);
}

namespace std
{
  template<>
  struct is_error_code_enum<usb::error>
    : true_type
  {};
}
