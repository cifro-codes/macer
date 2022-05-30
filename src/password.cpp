// Copyright (c) 2014-2020, The Monero Project
// 
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "password.hpp"

#include <cstdio>
#include <termios.h>
#include <unistd.h>

#define EOT 0x4

namespace
{
  constexpr const std::size_t max_password_size = 1000;

  bool is_tty(FILE* file) noexcept
  {
    return 0 != isatty(fileno(file));
  }

  expect<std::string> do_password(const char* message)
  {
    static constexpr const char BACKSPACE = 127;

    struct reenable
    {
      termios old;
      ~reenable() noexcept { tcsetattr(STDIN_FILENO, TCSANOW, &old); }
    };
    
    if (!is_tty(stdin))
      return {common_error::invalid_argument};
    
    fprintf(stderr, "%s:", message);
    
    std::string aPass;
    aPass.reserve(max_password_size);
    
    reenable terminal{};
    tcgetattr(STDIN_FILENO, &terminal.old);
    
    struct termios tty_new;
    tty_new = terminal.old;
    tty_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty_new);
    
    while (aPass.size() < max_password_size)
    {
      const int ch = getchar();
      if (EOF == ch || ch == EOT)
	return {common_error::invalid_argument};
      else if (ch == '\n' || ch == '\r')
	break;
      else if (ch == BACKSPACE && !aPass.empty())
	aPass.pop_back();
      else
	aPass.push_back(ch);
    }
    fprintf(stderr, "\n");
    return aPass;
  }
}

bool is_cout_tty() noexcept
{
  return is_tty(stdout);
}

expect<std::string> password_prompt(const char* message, const bool confirm)
{
  expect<std::string> secret = do_password(message);
  if (secret && confirm && secret != do_password("Confirm"))
    return {common_error::invalid_argument};
  return secret;
}
