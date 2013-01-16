/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#ifndef WZ_NOSOUND
#  include <vorbis/vorbisfile.h>
#  include <vorbis/codec.h>
#endif

#ifdef __BIG_ENDIAN__
#define OGG_ENDIAN 1
#else
#define OGG_ENDIAN 0
#endif

#include "oggvorbis.h"

struct OggVorbisDecoderState
{
	// Internal identifier towards PhysicsFS
	PHYSFS_file* fileHandle;

	// Wether to allow seeking or not
	bool         allowSeeking;

#ifndef WZ_NOSOUND
	// Internal identifier towards libVorbisFile
	OggVorbis_File oggVorbis_stream;

	// Internal meta data
	vorbis_info* VorbisInfo;
#endif
};

#ifndef WZ_NOSOUND
static const char* wz_oggVorbis_getErrorStr(int error)
{
    switch(error)
    {
		case OV_FALSE:
			return "OV_FALSE";
		case OV_HOLE:
			return "OV_HOLE";
		case OV_EREAD:
			return "OV_EREAD";
		case OV_EFAULT:
			return "OV_EFAULT";
		case OV_EIMPL:
			return "OV_EIMPL";
		case OV_EINVAL:
			return "OV_EINVAL";
		case OV_ENOTVORBIS:
			return "OV_ENOTVORBIS";
		case OV_EBADHEADER:
			return "OV_EBADHEADER";
		case OV_EVERSION:
			return "OV_EVERSION";
		case OV_EBADLINK:
			return "OV_EBADLINK";
		case OV_ENOSEEK:
			return "OV_ENOSEEK";
		default:
			return "Unknown Ogg error.";
    }
}

static size_t wz_oggVorbis_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	PHYSFS_file* fileHandle;

	ASSERT(datasource != NULL, "NULL decoder passed!");

	fileHandle = ((struct OggVorbisDecoderState*)datasource)->fileHandle;
	ASSERT(fileHandle != NULL, "Bad PhysicsFS file handle passed in");

	return PHYSFS_read(fileHandle, ptr, 1, size*nmemb);
}

static int wz_oggVorbis_seek(void *datasource, ogg_int64_t offset, int whence)
{
	PHYSFS_file* fileHandle;
	bool allowSeeking;
	int newPos;

	ASSERT(datasource != NULL, "NULL decoder passed!");

	fileHandle = ((struct OggVorbisDecoderState*)datasource)->fileHandle;
	ASSERT(fileHandle != NULL, "Bad PhysicsFS file handle passed in");

	allowSeeking = ((struct OggVorbisDecoderState*)datasource)->allowSeeking;

	if (!allowSeeking)
		return -1;

	switch (whence)
	{
		// Seek to absolute position
		case SEEK_SET:
			newPos = offset;
			break;

		// Seek `offset` ahead
		case SEEK_CUR:
		{
			int curPos = PHYSFS_tell(fileHandle);
			if (curPos == -1)
				return -1;

			newPos = curPos + offset;
			break;
		}

		// Seek backwards from the end of the file
		case SEEK_END:
		{
			int fileSize = PHYSFS_fileLength(fileHandle);
			if (fileSize == -1)
				return -1;

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
		return newPos;   // success
	else
		return -1;  // failure
}

static int wz_oggVorbis_close(WZ_DECL_UNUSED void *datasource)
{
	return 0;
}

static long wz_oggVorbis_tell(void *datasource)
{
	PHYSFS_file* fileHandle;

	ASSERT(datasource != NULL, "NULL decoder passed!");

	fileHandle = ((struct OggVorbisDecoderState*)datasource)->fileHandle;
	ASSERT(fileHandle != NULL, "Bad PhysicsFS file handle passed in");

	return PHYSFS_tell(fileHandle);
}

static const ov_callbacks wz_oggVorbis_callbacks =
{
	wz_oggVorbis_read,
	wz_oggVorbis_seek,
	wz_oggVorbis_close,
	wz_oggVorbis_tell
};
#endif

struct OggVorbisDecoderState* sound_CreateOggVorbisDecoder(PHYSFS_file* PHYSFS_fileHandle, bool allowSeeking)
{
#ifndef WZ_NOSOUND
	int error;
#endif

	struct OggVorbisDecoderState* decoder = (struct OggVorbisDecoderState *)malloc(sizeof(struct OggVorbisDecoderState));
	if (decoder == NULL)
	{
		debug(LOG_FATAL, "Out of memory");
		abort();
		return NULL;
	}

	ASSERT(PHYSFS_fileHandle != NULL, "Bad PhysicsFS file handle passed in");

	decoder->fileHandle = PHYSFS_fileHandle;
	decoder->allowSeeking = allowSeeking;

#ifndef WZ_NOSOUND
	error = ov_open_callbacks(decoder, &decoder->oggVorbis_stream, NULL, 0, wz_oggVorbis_callbacks);
	if (error < 0)
	{
		debug(LOG_ERROR, "ov_open_callbacks failed with errorcode %s", wz_oggVorbis_getErrorStr(error));
		free(decoder);
		return NULL;
	}

	// Aquire some info about the sound data
	decoder->VorbisInfo = ov_info(&decoder->oggVorbis_stream, -1);
#endif

	return decoder;
}

void sound_DestroyOggVorbisDecoder(struct OggVorbisDecoderState* decoder)
{
	ASSERT(decoder != NULL, "NULL decoder passed!");

#ifndef WZ_NOSOUND
	// Close the OggVorbis decoding stream
	ov_clear(&decoder->oggVorbis_stream);
#endif

	free(decoder);
}

static inline unsigned int getSampleCount(struct OggVorbisDecoderState* decoder)
{
#ifndef WZ_NOSOUND
	int numSamples;

	ASSERT(decoder != NULL, "NULL decoder passed!");

	numSamples = ov_pcm_total(&decoder->oggVorbis_stream, -1);

	if (numSamples == OV_EINVAL)
		return 0;

	return numSamples;
#else
	return 0;
#endif
}

static inline unsigned int getCurrentSample(struct OggVorbisDecoderState* decoder)
{
#ifndef WZ_NOSOUND
	int samplePos;

	ASSERT(decoder != NULL, "NULL decoder passed!");

	samplePos = ov_pcm_tell(&decoder->oggVorbis_stream);

	if (samplePos == OV_EINVAL)
		return 0;

	return samplePos;
#else
	return 0;
#endif
}

soundDataBuffer* sound_DecodeOggVorbis(struct OggVorbisDecoderState* decoder, size_t bufferSize)
{
	size_t		size = 0;
#ifndef WZ_NOSOUND
	int		result;
#endif

	soundDataBuffer* buffer;

	ASSERT(decoder != NULL, "NULL decoder passed!");

#ifndef WZ_NOSOUND
	if (decoder->allowSeeking)
	{
		unsigned int sampleCount = getSampleCount(decoder);

		unsigned int sizeEstimate = sampleCount * decoder->VorbisInfo->channels * 2;

		if (((bufferSize == 0) || (bufferSize > sizeEstimate)) && (sizeEstimate != 0))
		{
			bufferSize = (sampleCount - getCurrentSample(decoder)) * decoder->VorbisInfo->channels * 2;
		}
	}

	// If we can't seek nor receive any suggested size for our buffer, just quit
	if (bufferSize == 0)
	{
		debug(LOG_ERROR, "can't find a proper buffer size");
		return NULL;
	}
#else
	bufferSize = 0;
#endif

	buffer = (soundDataBuffer *)malloc(bufferSize + sizeof(soundDataBuffer));
	if (buffer == NULL)
	{
		debug(LOG_ERROR, "couldn't allocate memory (%lu bytes requested)", (unsigned long) bufferSize + sizeof(soundDataBuffer));
		return NULL;
	}

	buffer->data = (char*)(buffer + 1);
	buffer->bufferSize = bufferSize;
	buffer->bitsPerSample = 16;

#ifndef WZ_NOSOUND
	buffer->channelCount = decoder->VorbisInfo->channels;
	buffer->frequency = decoder->VorbisInfo->rate;

	// Decode PCM data into the buffer until there is nothing to decode left
	do
	{
		// Decode
		int section;
		result = ov_read(&decoder->oggVorbis_stream, &buffer->data[size], bufferSize - size, OGG_ENDIAN, 2, 1, &section);

		if (result < 0)
		{
			debug(LOG_ERROR, "error decoding from OggVorbis file; errorcode from ov_read: %s", wz_oggVorbis_getErrorStr(result));
			free(buffer);
			return NULL;
		}
		else
		{
			size += result;
		}

	} while ((result != 0 && size < bufferSize));
#endif

	buffer->size = size;

	return buffer;
}
