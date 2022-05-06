/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2022  Warzone 2100 Project

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
#include "codecs.h"
#include <opusfile.h>
#include <physfs.h>

class WZOpusDecoder final: public WZDecoder
{
public:
	static WZOpusDecoder* fromFilename(const char*);
	WZOpusDecoder(const WZOpusDecoder&)                 = delete;
	WZOpusDecoder(WZOpusDecoder&&)                      = delete;
	WZOpusDecoder &operator=(const WZOpusDecoder &)     = delete;
	WZOpusDecoder &operator=(WZOpusDecoder &&)          = delete;

	virtual optional<size_t> decode(uint8_t*, size_t) override;
	virtual int64_t totalTime()              const override { return m_duration; };
	virtual int     channels() 	             const override { return 2; };
	virtual size_t  frequency()              const override { return 48000l; };
	virtual ~WZOpusDecoder()
	{
		op_free(m_of);
		PHYSFS_close(m_file);
	}

private:
	WZOpusDecoder(PHYSFS_file *f, OggOpusFile* ovf, const OpusHead *head)
	: m_of(ovf), m_file(f)/*, m_head(head)*/
	{};
	OggOpusFile* m_of = nullptr;
	PHYSFS_file* m_file = nullptr;
	int64_t m_bufferSize = 0;
	// Because timestamps in Opus are fixed at 48 kHz, there is no need for a separate function to convert this to seconds
	// nb samples = nb seconds
	int64_t   m_duration = 0;
	// nb links. We do not supported chained files (which have > 1 link), because I am lazy
//	const int m_nlinks = 1;
//	const OpusHead *m_head = nullptr;
	const int m_nchannels = 2;
};
