
#include "write.hpp"

#include <limits>
#include <stdexcept>

namespace
{
  constexpr const std::size_t max_object_depth = 100;
  constexpr const std::size_t max_size_t = std::numeric_limits<std::size_t>::max();

  template<typename T>
  void write_varint(byte_stream& out, T value)
  {
    static_assert(!std::numeric_limits<T>::is_signed, "unsigned values not allowed in write_varint");
    for (; 0x7f < value; value >>= 7)
    {
      out.put((value & 0x7f) | 0x80);
    }
    out.put(value);
  }
}

namespace wire
{
  void protobuf_writer::write_tag(protobuf::type type)
  {
    static_assert(std::numeric_limits<unsigned>::max() < std::numeric_limits<std::uint64_t>::max() >> 3, "not enough space in uint64");
    write_varint(objects_[index_].stream, (std::uint64_t(last_id_) << 3) | std::uint8_t(type));
  }

  protobuf_writer::protobuf_writer()
    : objects_(), index_(max_size_t), last_id_(0)
  {
    objects_.reset(new object_data[max_object_depth]);
  }

  protobuf_writer::~protobuf_writer() noexcept
  {}

  void protobuf_writer::integer(const int source)
  {
    throw std::runtime_error{"protobuf_writer::integer not implemented"};
  }
  void protobuf_writer::integer(const std::intmax_t source)
  {
    throw std::runtime_error{"protobuf_writer::integer not implemented"};
  }
  void protobuf_writer::unsigned_integer(const unsigned source)
  {
    assert(index_ < max_object_depth);
    write_tag(protobuf::type::varint);
    write_varint(objects_[index_].stream, source);
  }
  void protobuf_writer::unsigned_integer(const std::uintmax_t source)
  {
    assert(index_ < max_object_depth);
    write_tag(protobuf::type::varint);
    write_varint(objects_[index_].stream, source);
  }
  void protobuf_writer::real(const double source)
  {
    throw std::runtime_error{"protobuf_writer::real not implemented"};
  }

  void protobuf_writer::string(const span<const char> source)
  {
    assert(index_ <  max_object_depth);
    write_tag(protobuf::type::bytes);
    write_varint(objects_[index_].stream, source.size());
    objects_[index_].stream.write(source);
  }
  void protobuf_writer::binary(const span<const std::uint8_t> source)
  {
    string({reinterpret_cast<const char*>(source.data()), source.size()});
  }

  void protobuf_writer::enumeration(const std::size_t index, const span<char const* const>)
  {
    assert(index_ < max_object_depth);
    write_tag(protobuf::type::varint);
    write_varint(objects_[index_].stream, index);
  }

  void protobuf_writer::start_array(std::size_t)
  {}
  void protobuf_writer::end_array()
  {}

  void protobuf_writer::start_object(std::size_t)
  {
    assert(index_ == max_size_t || index_ < max_object_depth);
    if (index_ != max_size_t && max_object_depth - index_ <= 1)
      throw std::runtime_error{"protobuf_writer::start_object reached max depth"};
    ++index_;
    objects_[index_].id = last_id_;
  }
  void protobuf_writer::key(const char*)
  {
    throw std::logic_error{"protobuf_writer::key string key not supported"};
  }
  void protobuf_writer::key(const std::uintmax_t id)
  {
    if (std::numeric_limits<unsigned>::max() < id)
      throw std::logic_error{"protobuf_writer::key id must be less than unsigned type"};

    last_id_ = id;
  }
  void protobuf_writer::key(unsigned id, const char*)
  {
    key(id);
  }
  void protobuf_writer::end_object()
  {
    if (index_ != max_size_t)
      --index_;
    if (index_ != max_size_t)
    {
      last_id_ = objects_[index_ + 1].id;
      binary(to_span(objects_[index_ + 1].stream));
      objects_[index_ + 1].stream.clear(); // re-use allocated memory
    }
  }

  byte_slice protobuf_writer::take_bytes()
  {
    if (index_ != max_size_t)
      throw std::logic_error{"protobuf_writer::take_slice called on incomplete protobuf stream"};

    byte_slice out{std::move(objects_[0].stream)};
    objects_[0].stream.clear();
    return out;
  }
}
