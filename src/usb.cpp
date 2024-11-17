#include "usb.hpp"

#include <algorithm>
#include <limits>
#include "logger.hpp"
#include "trezor/usb.hpp"

#define MACER_LIBUSB_CHECK(error_return, ...)			\
  do								\
  {								\
    const auto rc = __VA_ARGS__ ;				\
    if (rc < 0)							\
    {								\
      const std::error_code code{usb::error(rc)};		\
      MACER_LOG_ERROR(code);					\
      return error_return;					\
    }								\
  } while (0)

namespace
{
  struct device_list_free
  {
    void operator()(libusb_device** ptr) const noexcept
    {
      if (ptr)
	libusb_free_device_list(ptr, true);
    }
  };

  struct device_close
  {
    void operator()(libusb_device_handle* ptr) const noexcept
    {
      if (ptr)
	libusb_close(ptr);
    }
  };

  using device_ptr = std::unique_ptr<libusb_device_handle, device_close>;
} // anonymous

namespace usb
{
  class device
  {
    device_ptr ptr_;
    std::uint8_t in_;
    std::uint8_t out_;

  public:
    explicit device(device_ptr ptr, const std::uint8_t in, const std::uint8_t out) noexcept
      : ptr_(std::move(ptr)), in_(in), out_(out)
    {}

    std::uint8_t in() const noexcept { return in_; }
    std::uint8_t out() const noexcept { return out_; }
    libusb_device_handle* get() const noexcept { return ptr_.get(); }
  };
} // usb

namespace
{
  template<typename T>
  expect<byte_slice> open_and_run(libusb_context& ctx, libusb_device& dev, const host_info& info, const bool legacy, const bool og_firmware)
  {
    device_ptr handle;
    {
      libusb_device_handle* temp = nullptr;
      MACER_LIBUSB_CHECK(code, libusb_open(std::addressof(dev), std::addressof(temp)));
      MACER_LIBUSB_DEFENSIVE(temp);
      handle.reset(temp);
    }
    libusb_set_auto_detach_kernel_driver(handle.get(), true);
    expect<usb::interface> selected = usb::interface{};
    {
      libusb_config_descriptor* descriptor = nullptr;
      MACER_LIBUSB_CHECK(code, libusb_get_active_config_descriptor(std::addressof(dev), std::addressof(descriptor)));
      MACER_LIBUSB_DEFENSIVE(descriptor);
      MACER_LIBUSB_DEFENSIVE(descriptor->interface);
      
      selected = T::select({descriptor->interface, descriptor->bNumInterfaces}, og_firmware);
      if (!selected)
	return selected.error();
    }

    MACER_LIBUSB_CHECK(code, libusb_claim_interface(handle.get(), selected->number));
    usb::device real{std::move(handle), selected->in_endpoint, selected->out_endpoint};
    return T::run(real, info, legacy);
  }

  expect<void> transfer(libusb_device_handle* dev, const std::uint8_t endpoint, span<std::uint8_t> bytes, const std::chrono::milliseconds timeout)
  {
    static_assert(std::numeric_limits<int>::max() <= std::numeric_limits<std::size_t>::max(), "unexpected size_t max");
    static constexpr const std::size_t max_send = std::numeric_limits<int>::max();

    assert(dev != nullptr);
    const std::size_t total = bytes.size();
    while (!bytes.empty())
    {
      int actual = 0;
      const std::size_t this_send = std::min(max_send, bytes.size());
      MACER_LIBUSB_CHECK(
        code, libusb_interrupt_transfer(
	  dev, endpoint, bytes.data(), this_send, std::addressof(actual), timeout.count()
        )
      );
      bytes.remove_prefix(actual);
    }
    return success();
  }
} // anonymous

namespace usb
{
  const std::error_category& error_category() noexcept
  {
    struct category final : std::error_category
    {
      virtual const char* name() const noexcept override final
      {
        return "usb::error_category()";
      }

      virtual std::string message(int value) const override final
      {
        return get_string(usb::error(value));
      }
    };
    static const category instance{};
    return instance;
  }
  
  context make_context()
  {
    libusb_context* handle = nullptr;
    MACER_LIBUSB_CHECK(nullptr, libusb_init(std::addressof(handle)));
    return context{handle};
  }

  expect<void> read(device& dev, span<std::uint8_t> dest, const std::chrono::milliseconds timeout)
  {
    return transfer(dev.get(), dev.in(), dest, timeout);
  }
  expect<void> write(device& dev, span<const std::uint8_t> source, const std::chrono::milliseconds timeout)
  {
    return transfer(dev.get(), dev.out(), {const_cast<std::uint8_t*>(source.data()), source.size()}, timeout);
  }

  expect<byte_slice> run(libusb_context& ctx, const host_info& info, const bool legacy)
  {
    std::unique_ptr<libusb_device*[], device_list_free> list;
    {
      libusb_device** temp = nullptr;
      MACER_LIBUSB_CHECK(code, libusb_get_device_list(std::addressof(ctx), std::addressof(temp)));
      list.reset(temp);
    }
     
    for (libusb_device** i = list.get(); i && *i; ++i)
    {
      libusb_device_descriptor descriptor{};
      MACER_LIBUSB_CHECK(code, libusb_get_device_descriptor(*i, std::addressof(descriptor)));
      if (descriptor.idVendor == trezor::vendor_id || descriptor.idVendor == trezor::vendor_id_og)
      {
	const bool og_firmware = descriptor.idVendor == trezor::vendor_id_og;
	span<const std::uint16_t> devices{trezor::devices};
	if (og_firmware)
	  devices = span<const std::uint16_t>{trezor::devices_og};

	if (std::binary_search(devices.begin(), devices.end(), descriptor.idProduct))
	  return open_and_run<trezor::usb>(ctx, **i, info, legacy, og_firmware);
      }
    }
    return byte_slice{};
  }
}
