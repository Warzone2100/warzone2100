/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>
#include <physfs.h>
#include <memory>

class WZVorbisDecoder final: public WZDecoder
{
public:
	static WZVorbisDecoder* fromFilename(const char*);
	WZVorbisDecoder(const WZVorbisDecoder&)                 = delete;
	WZVorbisDecoder(WZVorbisDecoder&&)                      = delete;
	WZVorbisDecoder &operator=(const WZVorbisDecoder &)     = delete;
	WZVorbisDecoder &operator=(WZVorbisDecoder &&)          = delete;

	virtual optional<size_t> decode(uint8_t*, size_t) override;
	virtual int64_t totalTime()                    const override { return m_total_time; };
	virtual int     channels() 	                   const override { return m_info->channels;   };
	virtual size_t  frequency()                    const override { return m_info->rate;  };

	/** Returns the total pcm samples of the physical bitstream.  */
	int64_t totalSamples()                         const { return ov_pcm_total(m_ovfile.get(), -1);}
	virtual ~WZVorbisDecoder()
	{
		if (m_ovfile)
		{
			ov_clear(m_ovfile.get());
			m_ovfile.reset();
		}
		PHYSFS_close(m_file);
	}
private:
	WZVorbisDecoder(int64_t totalTime, PHYSFS_file *f, vorbis_info* info, std::unique_ptr<OggVorbis_File> ovf)
	: m_total_time(totalTime), m_info(info), m_file(f), m_ovfile(std::move(ovf))
	{};
	int64_t m_total_time = 0;
	int64_t m_bufferSize = 0;
	vorbis_info* m_info = nullptr;
	PHYSFS_file* m_file = nullptr;
	std::unique_ptr<OggVorbis_File> m_ovfile;

};
