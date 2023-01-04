/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once

#include "lib/framework/frame.h"
#include <physfs.h>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

class WZDecoder
{
public:
  WZDecoder() {};
  WZDecoder(const WZDecoder&)                 = delete;
  WZDecoder(WZDecoder&&)                      = delete;
  WZDecoder &operator=(const WZDecoder &)     = delete;
  WZDecoder &operator=(WZDecoder &&)          = delete;

  /** Decode all the data into a buffer.
   * Note that the API is different from oggvorbis: opus takes in, and gives out *samples* not bytes 
   * and we have to convert to little endian ourselves
   * 
   * \param [out] buffer: preallocated buffer to hold decoded data, little-endian. Will *not* be freed, even on error.
   * \param [in] bufferSize: takes buffer size in *bytes* as input, to be consistent with oggvorbis, 
   * \returns how much bytes have been placed into buffer, or nullopt  if error */
  virtual optional<size_t> decode(uint8_t*, size_t) = 0;
  virtual int64_t totalTime()       const = 0;
  virtual int     channels()        const = 0;

  /** Sampling rate of the bitstream */
  virtual size_t  frequency()       const = 0;
  virtual ~WZDecoder()                      = default;

};
