  
#pragma once

#define WIRE_PROTOBUF_DECLARE_ENUM(type)		\
  const char* get_string(type) noexcept;		\
  void read_bytes(::wire::protobuf_reader&, type&);	\
  void write_bytes(::wire::protobuf_writer&, type)

#define WIRE_PROTOBUF_DECLARE_OBJECT(type)		    \
  void read_bytes(::wire::protobuf_reader&, type&);         \
  void write_bytes(::wire::protobuf_writer&, const type&)

namespace wire
{
  struct protobuf;
  class protobuf_reader;
  class protobuf_writer;
}

