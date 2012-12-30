/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#ifndef _LIBSOUND_OGGVORBIS_H_
#define _LIBSOUND_OGGVORBIS_H_

#include "lib/framework/frame.h"
#include <physfs.h>

struct soundDataBuffer
{
	// the size of the data contained in *data (NOTE: this is *NOT* the size of *data itself)
	size_t size;

	// the size of the buffer *data points to plus sizeof(soundDataBuffer)
	size_t bufferSize;

	unsigned int bitsPerSample;
	unsigned int channelCount;
	unsigned int frequency;

	// the raw PCM data
	char *data;
};

// Forward declaration so we can take pointers to this type
struct OggVorbisDecoderState;

struct OggVorbisDecoderState *sound_CreateOggVorbisDecoder(PHYSFS_file *PHYSFS_fileHandle, bool allowSeeking);
void sound_DestroyOggVorbisDecoder(struct OggVorbisDecoderState *decoder);

soundDataBuffer *sound_DecodeOggVorbis(struct OggVorbisDecoderState *decoder, size_t bufferSize);

#endif // _LIBSOUND_OGGVORBIS_H_
