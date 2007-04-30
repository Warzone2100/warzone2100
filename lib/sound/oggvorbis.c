/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#include "oggvorbis.h"

#ifndef WZ_NOOGG
#include <vorbis/vorbisfile.h>
#endif

#ifdef __BIG_ENDIAN__
#define OGG_ENDIAN 1
#else
#define OGG_ENDIAN 0
#endif

#ifdef WZ_OS_MAC
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

static char* soundDataBuffer = NULL; // Needed for sound_DecodeTrack, must be global, so it can be free'd on shutdown
static size_t DataBuffer_size = 16 * 1024;

typedef struct
{
    // Internal identifier towards PhysicsFS
    PHYSFS_file* fileHandle;

    // Wether to allow seeking or not
    BOOL         allowSeeking;
} wz_oggVorbis_fileInfo;

static size_t wz_oggVorbis_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    PHYSFS_file* fileHandle = ((wz_oggVorbis_fileInfo*)datasource)->fileHandle;
    return PHYSFS_read(fileHandle, ptr, 1, size*nmemb);
}

static int wz_oggVorbis_seek(void *datasource, ogg_int64_t offset, int whence) {
    PHYSFS_file* fileHandle = ((wz_oggVorbis_fileInfo*)datasource)->fileHandle;
    BOOL allowSeeking = ((wz_oggVorbis_fileInfo*)datasource)->allowSeeking;

    int curPos, fileSize, newPos;

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
            curPos = PHYSFS_tell(fileHandle);
            if (curPos == -1)
                return -1;

            newPos = curPos + offset;
            break;

        // Seek backwards from the end of the file
        case SEEK_END:
            fileSize = PHYSFS_fileLength(fileHandle);
            if (fileSize == -1)
                return -1;

            newPos = fileSize - 1 - offset;
            break;
    }

    // PHYSFS_seek return value of non-zero means success
    if (PHYSFS_seek(fileHandle, newPos) != 0)
        return newPos;   // success
    else
        return -1;  // failure
}

static int wz_oggVorbis_close(void *datasource) {
    return 0;
}

static long wz_oggVorbis_tell(void *datasource) {
    PHYSFS_file* fileHandle = ((wz_oggVorbis_fileInfo*)datasource)->fileHandle;
    return PHYSFS_tell(fileHandle);
}

static const ov_callbacks wz_oggVorbis_callbacks = {
    wz_oggVorbis_read,
    wz_oggVorbis_seek,
    wz_oggVorbis_close,
    wz_oggVorbis_tell
};

/** Decodes an opened OggVorbis file into an OpenAL buffer
 *  \param psTrack pointer to object which will contain the final buffer
 *  \param PHYSFS_fileHandle file handle given by PhysicsFS to the opened file
 *  \param allowSeeking whether we should allow seeking or not (seeking is best disabled for streaming files)
 *  \return on success the psTrack pointer, otherwise it will be free'd and a NULL pointer is returned instead
 */
TRACK* sound_DecodeOggVorbisTrack(TRACK *psTrack, PHYSFS_file* PHYSFS_fileHandle, BOOL allowSeeking)
{
	OggVorbis_File	ogg_stream;
	vorbis_info*	ogg_info;

	ALenum		format;
	ALsizei		freq;

	ALuint		buffer;
	size_t		size = 0;
	int		result, section;

	wz_oggVorbis_fileInfo fileHandle = { PHYSFS_fileHandle, allowSeeking };


	if (ov_open_callbacks(&fileHandle, &ogg_stream, NULL, 0, wz_oggVorbis_callbacks) < 0)
	{
		free(psTrack);
		return NULL;
	}

	// Aquire some info about the sound data
	ogg_info = ov_info(&ogg_stream, -1);

	// Determine PCM data format and sample rate in Hz
	format = (ogg_info->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	freq = ogg_info->rate;

	// Allocate an initial buffer to contain the decoded PCM data
	if (soundDataBuffer == NULL)
	{
		soundDataBuffer = malloc(DataBuffer_size);
	}

	// Decode PCM data into the buffer until there is nothing to decode left
	do
	{
		// If the PCM buffer has become to small increase it by reallocating double its previous size
		if (size == DataBuffer_size) {
			DataBuffer_size *= 2;
			soundDataBuffer = realloc(soundDataBuffer, DataBuffer_size);
		}

		// Decode
		result = ov_read(&ogg_stream, &soundDataBuffer[size], DataBuffer_size - size, OGG_ENDIAN, 2, 1, &section);

		if (result < 0)
		{
			debug(LOG_ERROR, "sound_DecodeTrack: error decoding from OggVorbis file; errorcode from ov_read: %d\n", result);
			free(psTrack);
			return NULL;
		}
		else
		{
			size += result;
		}

	} while( result != 0 || size == DataBuffer_size);

	// Create an OpenAL buffer and fill it with the decoded data
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, soundDataBuffer, size, freq);

	// save buffer name in track
	psTrack->iBufferName = buffer;

	// Close the OggVorbis decoding stream
	ov_clear(&ogg_stream);

	return psTrack;
}

/** Clean up all resources used by the decoder functions
 */
void sound_CleanupOggVorbisDecoder()
{
	free(soundDataBuffer);
	soundDataBuffer = NULL;
}
