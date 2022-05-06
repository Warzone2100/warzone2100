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

#include "lib/framework/frame.h"
#include <physfs.h>
#include <limits>
#include "lib/framework/physfs_ext.h"
#include "oggvorbis.h"
#include "openal_callbacks.h"


#ifdef __BIG_ENDIAN__
#define OGG_ENDIAN 1
#else
#define OGG_ENDIAN 0
#endif

#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wshorten-64-to-32" // FIXME!!
#endif

#include "oggvorbis.h"

const ov_callbacks wz_oggVorbis_callbacks =
{
	wz_ogg_read,
	wz_ogg_seek,
	nullptr,
	wz_ogg_tell
};

WZVorbisDecoder* WZVorbisDecoder::fromFilename(const char* fileName)
{
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_WZ, "Reading...[directory: %s] %s", WZ_PHYSFS_getRealDir_String(fileName).c_str(), fileName);
	if (fileHandle == nullptr)
	{
		debug(LOG_ERROR, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, WZ_PHYSFS_getLastError());
		return nullptr;
	}
	OggVorbis_File* ovf = new OggVorbis_File();
	// https://xiph.org/vorbis/doc/vorbisfile/ov_open_callbacks.html
	const int error = ov_open_callbacks(fileHandle, ovf, nullptr, 0, wz_oggVorbis_callbacks);
	switch (error)
	{
	case OV_EREAD:
		debug(LOG_ERROR, "A read from media returned an error.");
		PHYSFS_close(fileHandle);
		return nullptr;
	case OV_ENOTVORBIS:
		debug(LOG_ERROR, "Bitstream does not contain any Vorbis data.");
		PHYSFS_close(fileHandle);
		return nullptr;
	case OV_EVERSION:
		debug(LOG_ERROR, "Vorbis version mismatch.");
		PHYSFS_close(fileHandle);
		return nullptr;
	case OV_EBADHEADER:
		debug(LOG_ERROR, "Invalid Vorbis bitstream header.");
		PHYSFS_close(fileHandle);
		return nullptr;
	case OV_EFAULT:
		debug(LOG_ERROR, "Internal logic fault; indicates a bug or heap/stack corruption.");
		PHYSFS_close(fileHandle);
		return nullptr;
	default:
		break;
	}
	ASSERT(ovf, "doesn't make sense");
	vorbis_info* info = ov_info(ovf, -1);
	if (!info)
	{
		debug(LOG_ERROR, "ov_info failed on %s", fileName);
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	
	long seekable = ov_seekable(ovf);
	if (!seekable)
	{
		debug(LOG_ERROR, "Expecting file to be seekable %s",  fileName);
		PHYSFS_close(fileHandle);
		ov_clear(ovf);
		return nullptr;
	}
	int64_t t = static_cast<int64_t>(ov_time_total(ovf, -1));
	if (t == OV_EINVAL)
	{
		debug(LOG_ERROR, "%s: the argument was invalid.The requested bitstream did not exist or the bitstream is nonseekable.", fileName);
		return nullptr;
	}
	const auto bitrate = ov_bitrate(ovf, -1);
	if (bitrate == OV_FALSE)
	{
		debug(LOG_ERROR, "bitrate failed");
		return nullptr;
	}
	WZVorbisDecoder* decoder = new WZVorbisDecoder(t, fileHandle, info, ovf);
	if (!decoder)
	{
		debug(LOG_ERROR, "Failed to allocate");
		PHYSFS_close(fileHandle);
		return nullptr;
	}

	return decoder;
}

int WZVorbisDecoder::decode(uint8_t* buffer, size_t bufferSize)
{
	size_t		bufferOffset = 0;
	int		    result;
	//unsigned int sampleCount = m_total_time;
	if (m_total_time == OV_EINVAL)
	{
		debug(LOG_ERROR, "--sampleCount was OV_EINVAL. The requested bitstream did not exist or the bitstream is nonseekable");
		return -1;
	}

	do
	{
		int section = 0;
		size_t spaceLeft = bufferSize - bufferOffset;
		ASSERT(spaceLeft <= static_cast<size_t>(std::numeric_limits<int>::max()), "spaceLeft (%zu) exceeds int::max", spaceLeft);
		result = ov_read(m_ovfile, (char*) buffer + bufferOffset, static_cast<int>(spaceLeft), OGG_ENDIAN, 2, 1, &section);
		switch (result)
		{
		case OV_HOLE:
			debug(LOG_ERROR, "error decoding from OggVorbis file: there was an interruption in the data, at %zu", bufferOffset);
			return -1;
		case OV_EBADLINK:
			debug(LOG_ERROR, "invalid stream section was supplied to libvorbisfile, or the requested link is corrupt, at %zu", bufferOffset);
			return -1;
		case OV_EINVAL:
			debug(LOG_ERROR, "initial file headers couldn't be read or are corrupt, or that the initial open call for vf failed, at %zu", bufferOffset);
			return -1;
		default:
			break;
		}
		ASSERT(result >= 0, "unexpected error while decoding: %i", result);
		bufferOffset += result;
	} while ((result != 0 && bufferOffset < bufferSize));
	m_bufferSize = bufferSize;
	return bufferOffset;
}
