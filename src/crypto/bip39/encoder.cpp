// Copyright (c) 2024, Cifro Codes
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

// Parts (the bit manipulation portion) is copyrighted by:
/**
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "encoder.hpp"
#include "byte_slice.hpp"
#include "byte_stream.hpp"
#include "crypto/sha256.h"
#include "error.hpp"
#include "wordlist.hpp"

namespace bip39
{
  expect<byte_slice> encode(byte_slice in)
  {
    if (in.size() < 16 || 32 < in.size() || in.size() % 8 != 0)
      return {common_error::hash_failure};

    static_assert(1 <= crypto_hash_sha256_BYTES, "unexpected hash size");
    {
      unsigned char hash[crypto_hash_sha256_BYTES] = {0};
      if (crypto_hash_sha256(hash, in.data(), in.size()))
        return {common_error::hash_failure};

      byte_stream temp;
      temp.write(to_span(in));
      temp.put(hash[0]);
      in = byte_slice{std::move(temp)};
    }

    byte_stream words;
    const unsigned word_count = ((in.size() - 1) * 3) / 4;
    for (unsigned i = 0; i < word_count; ++i)
    {
      // bit at a time, based on Trezor source
      unsigned index = 0; 
      for (unsigned j = 0; j < 11; j++) {
        index <<= 1;
        index += (in.data()[(i * 11 + j) / 8] & (1 << (7 - ((i * 11 + j) % 8)))) > 0;
      }

      char const * const word = word_list[index];
      words.write(word, strlen(word));

      if (i != word_count - 1)
      	words.put(' ');
    }

    return byte_slice{std::move(words)};
  }
}

