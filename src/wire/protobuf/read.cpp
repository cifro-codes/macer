
#include "read.hpp"

#include <algorithm>
#include <endian.h>
#include <limits>
#include <stdexcept>

#include "expect.hpp"
#include "wire/error.hpp"
#include "wire/protobuf/error.hpp"

namespace
{
  template<typename T>
  T protobuf_uvarint(span<const std::uint8_t>& source)
  {
    static constexpr auto bits = std::numeric_limits<T>::digits;
    static_assert(std::numeric_limits<T>::radix == 2, "only base2 type supported");
    static_assert(((bits / 7) + 1) * 7 <= std::numeric_limits<unsigned>::max(), "unsigned too small for shift amount");
    T value = 0;
    const std::uint8_t* bytes = source.begin();
    std::uint8_t const* const end = source.end();
    for (unsigned shift = 0; bytes < end && shift < bits; shift += 7, ++bytes)
    {
      const std::uint8_t raw = *bytes;
      const std::uint8_t next = raw & 0x7F;
      if (bits - shift < 8 && next >> (bits - shift))
	WIRE_DLOG_THROW(wire::error::schema::smaller_integer);
      if (!raw && shift)
	WIRE_DLOG_THROW(wire::error::protobuf::invalid_encoding, "unnecessary 0 byte in varint");

      value |= (T(next) << shift);
      if ((raw & 0x80) == 0)
      {
	source.remove_prefix((bytes - source.begin()) + 1);
	return value;
      }
    }
    WIRE_DLOG_THROW(wire::error::protobuf::invalid_encoding, (bytes == end ? "reached end of stream" : "exceeded expected size"));
  }

  template<typename T>
  [[noreturn]] T protobuf_varint(span<const std::uint8_t>&)
  {
    static_assert(std::numeric_limits<T>::is_signed, "use protobuf_uvarint");
    throw std::logic_error{"signed integer varints not implemented"};
  }

  template<typename T>
  T protobuf_fixed(span<const std::uint8_t>& source)
  {
    if (source.size() < sizeof(T))
      WIRE_DLOG_THROW(wire::error::protobuf::invalid_encoding, "not enough bytes for fixed value");

    T value;
    std::memcpy(std::addressof(value), source.data(), sizeof(value));
    source.remove_prefix(sizeof(value));

    static_assert(BYTE_ORDER == LITTLE_ENDIAN, "only little endian machines currently supported");
    return value;
  }

  span<const std::uint8_t> protobuf_bytes(span<const std::uint8_t>& source)
  {
    const std::size_t bytes = protobuf_uvarint<std::size_t>(source);
    if (source.size() < bytes)
      WIRE_DLOG_THROW(wire::error::protobuf::invalid_encoding, "not enough bytes");

    const std::uint8_t* start = source.data();
    return {start, source.remove_prefix(bytes)};
  }

  void protobuf_skip(span<const std::uint8_t>& source, const wire::protobuf::type type)
  {
    switch (type)
    {
    case wire::protobuf::type::bytes:
      protobuf_bytes(source);
      break;
    case wire::protobuf::type::fixed32:
      protobuf_fixed<std::uint32_t>(source);
      break;
    case wire::protobuf::type::fixed64:
      protobuf_fixed<std::uint64_t>(source);
      break;
    case wire::protobuf::type::varint:
      protobuf_uvarint<std::uintmax_t>(source);
      break;
    default:
      WIRE_DLOG_THROW(wire::error::protobuf::unrecognized_type);
    };
  }
} // anonymous

namespace wire
{
  void protobuf_reader::check_bounds(const char* function)
  {
    if (depth() < 1 || max_read_depth() < depth())
      throw std::logic_error{"array indexing out of bounds in protobuf_reader::" + std::string{function}};
  }

  protobuf_reader::protobuf_reader(byte_slice&& source)
    : reader(),
      source_(std::move(source)),
      objects_(new span<const std::uint8_t>[max_read_depth()])
  {
    objects_[0] = to_span(source_);
  }

  void protobuf_reader::check_complete() const
  {
    if (depth() || !objects_[0].empty())
      WIRE_DLOG_THROW(error::protobuf::invalid_encoding);
  }

  bool protobuf_reader::boolean()
  {
    check_bounds("boolean");
    return protobuf_uvarint<std::uint8_t>(objects_[depth() - 1]);
  }

  std::intmax_t protobuf_reader::integer()
  {
    check_bounds("integer");
    switch (last_type_)
    {
    default:
      WIRE_DLOG_THROW(error::schema::integer);
    case protobuf::type::fixed32:
      return protobuf_fixed<std::int32_t>(objects_[depth() - 1]);
    case protobuf::type::fixed64:
      return protobuf_fixed<std::int64_t>(objects_[depth() - 1]);
    case protobuf::type::varint:
      break;
    };
    return protobuf_varint<std::intmax_t>(objects_[depth() - 1]);
  }

  std::uintmax_t protobuf_reader::unsigned_integer()
  {
    check_bounds("unsigned_integer");
    switch (last_type_)
    {
    default:
      WIRE_DLOG_THROW(error::schema::integer);
    case protobuf::type::bytes:
      throw std::runtime_error{"protobuf_reader::unsigned_integer packed arrays not supported"};
    case protobuf::type::fixed32:
      return protobuf_fixed<std::uint32_t>(objects_[depth() - 1]);
    case protobuf::type::fixed64:
      return protobuf_fixed<std::uint64_t>(objects_[depth() - 1]);
    case protobuf::type::varint:
      break;
    };
    return protobuf_uvarint<std::uintmax_t>(objects_[depth() - 1]);
  }

  double protobuf_reader::real()
  {
    throw std::runtime_error{"protobuf_reader::real not implemented"};
  }

  std::string protobuf_reader::string()
  {
    check_bounds("string");
    const auto source = protobuf_bytes(objects_[depth() - 1]);
    return {reinterpret_cast<const char*>(source.data()), source.size()};
  }

  byte_slice protobuf_reader::binary()
  {
    check_bounds("binary");
    const auto source = protobuf_bytes(objects_[depth() - 1]);
    const std::size_t offset = source.data() - source_.data();
    return source_.get_slice(offset, offset + source.size());
  }

  void protobuf_reader::binary(const span<std::uint8_t> dest)
  {
    check_bounds("binary");
    const auto source = protobuf_bytes(objects_[depth() - 1]);
    if (source.size() != dest.size())
      WIRE_DLOG_THROW(error::schema::fixed_binary);
    std::memcpy(dest.data(), source.data(), dest.size());
  }

  std::size_t protobuf_reader::enumeration(const span<char const* const>)
  {
    check_bounds("enumeration");
    return protobuf_uvarint<std::size_t>(objects_[depth() - 1]);
  }

  std::size_t protobuf_reader::start_array()
  {
    increment_depth();
    check_bounds("start_array");
    if (last_type_ == protobuf::type::bytes)
      objects_[depth() - 1] = protobuf_bytes(objects_[depth() - 2]);
    else
      objects_[depth() - 1] = nullptr;
    return 0;
  }

  bool protobuf_reader::is_array_end(const std::size_t)
  {
    check_bounds("is_array_end");
    if (!objects_[depth() - 1].empty())
      return false;
    return true;
  }

  std::size_t protobuf_reader::start_object()
  {
    increment_depth();
    check_bounds("start_object");
    if (1 < depth())
      objects_[depth() - 1] = protobuf_bytes(objects_[depth() - 2]);
    return 0;
  }

  bool protobuf_reader::key(const span<const key_map> map, std::size_t& state, std::size_t& index)
  {
    check_bounds("key");

    span<const std::uint8_t>& source = objects_[depth() - 1];
    while (!source.empty())
    {
      const unsigned tag = protobuf_uvarint<unsigned>(source);
      last_type_ = protobuf::type(tag & 0x07);
      const unsigned id = tag >> 3;
      for (const key_map& entry : map)
      {
	if (id == entry.id)
	{
	  index = std::addressof(entry) - map.begin();
	  return true;
	}
      }
      protobuf_skip(source, last_type_);
   }
    return false;
  }
}
