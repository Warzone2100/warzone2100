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


#ifdef __BIG_ENDIAN__
#define OGG_ENDIAN 1
#else
#define OGG_ENDIAN 0
#endif

#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wshorten-64-to-32" // FIXME!!
#endif

#include "oggvorbis.h"

/**
 *
 * \param	_stream	    The stream to read from.
 * \param 	[out]	 _ptr	The buffer to store the data in.
 * \param _nbytes	    The maximum number of bytes to read.
	This function may return fewer, though it will not return zero unless it reaches end-of-file.
*/
static size_t wz_ogg_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	PHYSFS_file *fileHandle;

	ASSERT(datasource != nullptr, "NULL decoder passed!");

	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");

	size_t readLen = size * nmemb;
	ASSERT(readLen <= static_cast<size_t>(std::numeric_limits<PHYSFS_uint32>::max()), "readLen (%zu) exceeds PHYSFS_uint32::max", readLen);
	return WZ_PHYSFS_readBytes(fileHandle, ptr, static_cast<PHYSFS_uint32>(readLen));
}

static int wz_ogg_seek(void *datasource, int64_t offset, int whence)
{
	PHYSFS_file *fileHandle;
	int64_t newPos;
	ASSERT(datasource != nullptr, "NULL decoder passed!");
	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");

	switch (whence)
	{
	// Seek to absolute position
	case SEEK_SET:
		newPos = offset;
		break;

	// Seek `offset` ahead
	case SEEK_CUR:
		{
			int64_t curPos = PHYSFS_tell(fileHandle);
			if (curPos == -1)
			{
				return -1;
			}

			newPos = curPos + offset;
			break;
		}

	// Seek backwards from the end of the file
	case SEEK_END:
		{
			int64_t fileSize = PHYSFS_fileLength(fileHandle);
			if (fileSize == -1)
			{
				return -1;
			}

			newPos = fileSize - 1 - offset;
			break;
		}

	// unrecognized seek instruction
	default:
		// indicate failure
		return -1;
	}

	// PHYSFS_seek return value of non-zero means success
	if (PHYSFS_seek(fileHandle, newPos) != 0)
	{
		// success is NON-ZERO!
		return newPos;
	}
	else
	{
		// indicate failure
		return -1;    // failure
	}
}

static long wz_ogg_tell(void *datasource)
{
	PHYSFS_file *fileHandle;
	ASSERT(datasource != nullptr, "NULL decoder passed!");

	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");
	const int64_t out = PHYSFS_tell(fileHandle);
	return out;
}

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
	std::unique_ptr<OggVorbis_File> ovf = std::unique_ptr<OggVorbis_File>(new OggVorbis_File());
	// https://xiph.org/vorbis/doc/vorbisfile/ov_open_callbacks.html
	const int error = ov_open_callbacks(fileHandle, ovf.get(), nullptr, 0, wz_oggVorbis_callbacks);
	if (error != 0)
	{
		switch (error)
		{
		case OV_EREAD:
			debug(LOG_ERROR, "A read from media returned an error.");
			break;
		case OV_ENOTVORBIS:
			debug(LOG_ERROR, "Bitstream does not contain any Vorbis data.");
			break;
		case OV_EVERSION:
			debug(LOG_ERROR, "Vorbis version mismatch.");
			break;
		case OV_EBADHEADER:
			debug(LOG_ERROR, "Invalid Vorbis bitstream header.");
			break;
		case OV_EFAULT:
			debug(LOG_ERROR, "Internal logic fault; indicates a bug or heap/stack corruption.");
			break;
		default:
			break;
		}
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	ASSERT(ovf, "doesn't make sense");
	vorbis_info* info = ov_info(ovf.get(), -1);
	if (!info)
	{
		debug(LOG_ERROR, "ov_info failed on %s", fileName);
		ov_clear(ovf.get());
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	
	long seekable = ov_seekable(ovf.get());
	if (!seekable)
	{
		debug(LOG_ERROR, "Expecting file to be seekable %s",  fileName);
		ov_clear(ovf.get());
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	int64_t t = static_cast<int64_t>(ov_time_total(ovf.get(), -1));
	if (t == OV_EINVAL)
	{
		debug(LOG_ERROR, "%s: the argument was invalid.The requested bitstream did not exist or the bitstream is nonseekable.", fileName);
		ov_clear(ovf.get());
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	const auto bitrate = ov_bitrate(ovf.get(), -1);
	if (bitrate == OV_FALSE)
	{
		debug(LOG_ERROR, "bitrate failed");
		ov_clear(ovf.get());
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	WZVorbisDecoder* decoder = new WZVorbisDecoder(t, fileHandle, info, std::move(ovf));
	if (!decoder)
	{
		debug(LOG_ERROR, "Failed to allocate");
		if (ovf)
		{
			ov_clear(ovf.get());
		}
		PHYSFS_close(fileHandle);
		return nullptr;
	}

	return decoder;
}

optional<size_t> WZVorbisDecoder::decode(uint8_t* buffer, size_t bufferSize)
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
		result = ov_read(m_ovfile.get(), (char*) buffer + bufferOffset, static_cast<int>(spaceLeft), OGG_ENDIAN, 2, 1, &section);
		switch (result)
		{
		case OV_HOLE:
			debug(LOG_ERROR, "error decoding from OggVorbis file: there was an interruption in the data, at %zu", bufferOffset);
			return nullopt;
		case OV_EBADLINK:
			debug(LOG_ERROR, "invalid stream section was supplied to libvorbisfile, or the requested link is corrupt, at %zu", bufferOffset);
			return nullopt;
		case OV_EINVAL:
			debug(LOG_ERROR, "initial file headers couldn't be read or are corrupt, or that the initial open call for vf failed, at %zu", bufferOffset);
			return nullopt;
		default:
			break;
		}
		ASSERT_OR_RETURN(nullopt, result >= 0, "unexpected error while decoding: %i", result);
		bufferOffset += static_cast<size_t>(result);
	} while ((result != 0 && bufferOffset < bufferSize));
	m_bufferSize = bufferSize;
	return bufferOffset;
}
