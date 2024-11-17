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

#include <algorithm>
#include <cstdint>
#include <cstring>
#include "crypto/bip39/encoder.hpp"
#include "host_info.hpp"
#include "logger.hpp"
#include "password.hpp"
#include "usb.hpp"

namespace
{
  enum class format : std::uint8_t { none = 0, legacy, binary, bip39_12, bip39_18, bip39_24 };
  struct program
  {
    host_info info;
    format fmt;
    bool existing;
    bool password;
    bool failed;
  };
  typedef const char**(*argument_handler)(program&, const char*[]);
  
  struct argument
  {
    const argument_handler handler;
    const char* full;
    const char* description;
    const char truncated;
  };

  const char** basic_handler(program& prog, std::string& out, const char* name, const char* argv[])
  {
    if (!argv || !argv[0])
    {
      prog.failed = true;
      fprintf(stderr, "Mssing argument for --%s\n", name);
      return nullptr;
    }
    if (!out.empty())
    {
      prog.failed = true;
      fprintf(stderr, "Argument --%s listed twice\n", name);
      return nullptr;
    }
    out = argv[0];
    return ++argv;
  }

  const char** handle_existing(program& prog, const char* argv[])
  {
    prog.existing = true;
    return argv;
  }
  const char** handle_format(program& prog, const char* argv[])
  {
    if (!argv || !argv[0])
    {
      prog.failed = true;
      fprintf(stderr, "Missing argument for --format\n");
      return nullptr;
    }

    if (prog.fmt != format::none)
    {
      prog.failed = true;
      fprintf(stderr, "Argument --format listed twice\n");
      return nullptr;
    }

    if (strcmp("legacy", argv[0]) == 0)
    {
      prog.fmt = format::legacy;
      return ++argv;
    }

    if (!prog.info.message.empty())
    {
      prog.failed = true;
      fprintf(stderr, "Cannot use --message with new --format scheme\n");
      return nullptr;
    }

    prog.info.message = "GENERATE PASSWORD";
    if (strcmp("binary", argv[0]) == 0)
      prog.fmt = format::binary;
    else if (strcmp("bip39-12", argv[0]) == 0)
      prog.fmt = format::bip39_12;
    else if (strcmp("bip39-18", argv[0]) == 0)
      prog.fmt = format::bip39_18;
    else if (strcmp("bip39-24", argv[0]) == 0)
      prog.fmt = format::bip39_24;
    else
    {
      prog.failed = true;
      fprintf(stderr, "Invalid --format value\n");
      return nullptr;
    }
    return ++argv;
  }
  const char** handle_host(program& prog, const char* argv[])
  {
    return basic_handler(prog, prog.info.host, "host", argv);
  }
  const char** handle_user(program& prog, const char* argv[])
  {
    return basic_handler(prog, prog.info.user, "user", argv);
  }
  const char** handle_message(program& prog, const char* argv[])
  {
    if (prog.fmt != format::none && prog.fmt != format::legacy)
    {
      prog.failed = true;
      fprintf(stderr, "Cannot use --message with new --format scheme\n");
      return nullptr;
    }
    return basic_handler(prog, prog.info.message, "message", argv);
  }
  const char** handle_password(program& prog, const char* argv[])
  {
    prog.password = true;
    return argv;
  }
  
  constexpr const argument process_args[] =
  {
    {nullptr, "help", "\t\tList help", 'h'},
    {handle_existing, "existing", "\t\tPrompt for existing LUKS password for adding new key", 'e'},
    {handle_format, "format", "[format]\tOutput format/strength -> legacy | binary | bip39-12 | bip39-18 | bip39-24", 'f'},
    {handle_host, "host", "[hostname]\tIdentity hostname for password", 't'},
    {handle_user, "user", "[user]\t\tIdentity username for password", 'u'},
    {handle_message, "message", "[message]\tMessage to display on device (legacy format only)", 'm'},
    {handle_password, "password", "\t\tPrompt for local only password to append to stdout (more entropy)", 'p'}
  };

  template<typename F>
  const argument* find_argument(F f)
  {
    return std::find_if(std::begin(process_args), std::end(process_args), f);
  }

  const char** process_argument(program& prog, const char* argv[])
  {
    if (!argv || !argv[0])
      return nullptr;

    const argument* current = std::end(process_args);
    if (std::strncmp("--", argv[0], 2) == 0)
      current = find_argument([argv] (const argument& arg) { return std::strcmp(arg.full, argv[0] + 2) == 0; });
    else if (std::strncmp("-", argv[0], 1) == 0)
      current = find_argument([argv] (const argument& arg) { return std::strlen(argv[0]) == 2 && argv[0][1] == arg.truncated; });

    if (current == std::end(process_args) || !current->handler)
    {
      prog.failed = true;
      if (current == std::end(process_args))
	fprintf(stderr, "No such argument %s\n", argv[0]);
      for (const argument& arg : process_args)
	fprintf(stderr, "\t--%s, -%c\t%s\n", arg.full, arg.truncated, arg.description);
      return nullptr;
    }

    ++argv;
    return current->handler(prog, argv);
  }
}

int main(int, const char* argv[])
{
  if (!argv)
  {
    fprintf(stderr, "No process name\n");
    return -1;
  }

  ++argv;
  program prog{};
  while (argv = process_argument(prog, argv));
  if (prog.failed)
    return -1;

  if (is_cout_tty())
  {
    fprintf(stderr, "stdout should not be connected to tty. Pipe output to another process to run.\n");
    return -1;
  }
	
  if (prog.info.host.empty())
  {
    fprintf(stderr, "--host argument required\n");
    return -1;
  }

  if (prog.fmt == format::none)
  {
    fprintf(stderr, "--format argument required\n");
    return -1;
  }

  if (prog.existing)
  {
    const expect<std::string> existing = password_prompt("Enter current password");
    if (!existing)
    {
      MACER_LOG_ERROR(existing.error());
      return -1;
    }
    fwrite(existing->data(), 1, existing->size(), stdout);
    fprintf(stdout, "\n"); // tells cryptsetup about existing password
  }

  const usb::context ctx = usb::make_context();
  if (!ctx)
    return -1;

  expect<byte_slice> secret{common_error::invalid_argument};
  while (true)
  {
    secret = usb::run(*ctx, prog.info, prog.fmt == format::legacy);
    if (!secret)
    {
      MACER_LOG_ERROR(secret.error());
      return -1;
    }
    if (!secret->empty())
      break;

    fprintf(stderr, "Attach compatible device  (press any key when ready)...\n");
    if (getchar() == EOF)
    {
      fprintf(stderr, "No input available, quitting\n");
      return -1;
    }
  }

  bool bip39_output = true;
  unsigned pass_size = 64;
  switch (prog.fmt)
  {
  default:
  case format::legacy:
    bip39_output = false;
    break;
  case format::binary:
    bip39_output = false;
    pass_size = 32;
    break;
  case format::bip39_12:
    pass_size = 16;
    break;
  case format::bip39_18:
    pass_size = 24;
    break;
  case format::bip39_24:
    pass_size = 32;
    break;
  }

  if (secret->size() < pass_size)
  {
    fprintf(stderr, "Internal Error on Password Generation\n");
    return -1;
  }

  secret = secret->get_slice(0, pass_size);
  if (bip39_output)
    secret = bip39::encode(std::move(*secret)).value();
  

  expect<std::string> local{common_error::invalid_argument};
  if (prog.password)
  {
    local = password_prompt("Local passphrase", prog.existing);
    if (!local)
    {
      MACER_LOG_ERROR(local.error());
      return -1;
    }
  }

  assert(secret.has_value());
  fwrite(secret->data(), 1, secret->size(), stdout);
  if (local)
    fwrite(local->data(), 1, local->size(), stdout);
  return 0;
}
