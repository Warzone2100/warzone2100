/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
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
#include "lib/framework/frame.h"

#include "adpcm.h"
#include "rpl_reader.h"
#include "dec130.h"

static unsigned int rpl_decode_sound_none(RPL* rpl, int16_t* buffer, unsigned int buffer_size);
static unsigned int rpl_decode_sound_unknown(RPL* rpl, int16_t* buffer, unsigned int buffer_size);
static unsigned int rpl_decode_sound_raw(RPL* rpl, int16_t* buffer, unsigned int buffer_size);
static unsigned int rpl_decode_sound_adpcm(RPL* rpl, int16_t* buffer, unsigned int buffer_size);

static unsigned int rpl_decode_video_unknown(RPL* rpl, char* in, unsigned int in_size, char* out);

//*************************************************************************************

static char* data_buffer = NULL;

//*************************************************************************************

static char *readline(PHYSFS_file *f, char *linebuf, size_t len)
{
	char c;
	size_t i = 0;

	for (i = 0; i < len-1; i++)
	{
		if (PHYSFS_read(f, &c, 1, 1) != 1)
		{
			debug( LOG_ERROR, "Error reading from sequence file: %s\n",
				 PHYSFS_getLastError() );
			break;
		}

		if (c == '\n' || !isprint(c))
			break;

		linebuf[i] = c;
	}

	linebuf[i] = '\0';

	return linebuf;
}

static int readint(PHYSFS_file *f, char *linebuf, size_t len)
{
	int num;
	readline(f, linebuf, len);
	if (sscanf(linebuf, "%u", &num) < 1)
		num = 0;

	return num;
}

static float readfloat(PHYSFS_file *f, char *linebuf, size_t len)
{
	float num;
	readline(f, linebuf, len);
	if (sscanf(linebuf, "%f", &num) < 1)
		num = 0.0;

	return num;
}

RPL* rpl_open(const char* filename)
{
	PHYSFS_file* f;
	RPL* rpl;
	char buf[80];
	int tmp;
	size_t len = sizeof(buf);

	f = PHYSFS_openRead(filename);
	if (f == NULL)
	{
		debug(LOG_ERROR, "Error reading %s: %s", filename, PHYSFS_getLastError());
		return NULL;
	}

	/* going to do lots of small reads, so use a buffer */
	PHYSFS_setBuffer(f, 1024);

	rpl = (RPL*)malloc(sizeof(RPL));
	rpl->chunks = NULL;

	rpl->f = f;

	if (strcmp(readline(f, buf, len), "ARMovie") != 0)
	{
		debug(LOG_NEVER, "%s missing RPL magic number\n", filename);
	}
	readline(f, buf, len); /* discard filename */
	readline(f, buf, len); /* discard copyright */
	if (strcmp(readline(f, buf, len), "ESCAPE 2.0") != 0)
	{
		/* This field is really "author", but.. */
		debug(LOG_NEVER, "%s not in \"ESCAPE 2.0\" format?\n", filename);
	}


	tmp = readint(f, buf, len);
	switch (tmp)
	{
		case 130:
			rpl->video_decoder = dec130_decode;
			break;
		default:
			rpl->video_decoder = rpl_decode_video_unknown;
			debug( LOG_NEVER, "Unknown video format %i\n", tmp);
			break;
	}

	rpl->width = readint(f, buf, len);
	rpl->height = readint(f, buf, len);
	rpl->bpp = readint(f, buf, len);
	rpl->fps = readfloat(f, buf, len);
	rpl->current_video_frame = 0;

	rpl->soundCodecID = readint(f, buf, len);
	switch (rpl->soundCodecID)
	{
		case 0:
			rpl->sound_decoder = rpl_decode_sound_none;
			break;

		case 1:
			rpl->sound_decoder = rpl_decode_sound_raw;
			break;

		case 101:
			rpl->sound_decoder = rpl_decode_sound_adpcm;
			rpl->sound_decoder_data = adpcm_init();
			break;

		default:
			rpl->sound_decoder = rpl_decode_sound_unknown;
			debug(LOG_SOUND, "Unknown sound format %i\n", tmp);
			break;
	}

	rpl->current_sound_frame = 0;
	rpl->samples = readint(f, buf, len);
	rpl->channels = readint(f, buf, len);
	rpl->bps = readint(f, buf, len);
	rpl->fpc = readint(f, buf, len);
	rpl->nb_chunks = readint(f, buf, len) + 1;
	rpl->ocs = readint(f, buf, len);
	rpl->ecs = readint(f, buf, len);
	rpl->otcc = readint(f, buf, len);
	rpl->ots = readint(f, buf, len);
	rpl->sprite_size = readint(f, buf, len);
	rpl->otkf = readint(f, buf, len);

	{
		unsigned int i;
		unsigned int max_video_size = 0;

		rpl->chunks = (RPL_chunk_info_t*)malloc(sizeof(RPL_chunk_info_t)*rpl->nb_chunks);
		PHYSFS_seek(f, rpl->otcc);

		for (i = 0; i < rpl->nb_chunks; ++i)
		{
			readline(f, buf, len);
			if (sscanf(buf, "%i,%i;%i", &rpl->chunks[i].offset, &rpl->chunks[i].video_size, &rpl->chunks[i].audio_size) != 3)
			{
				debug( LOG_SOUND, "Error in chunk catalog\n" );
				goto error;
			}

			if (rpl->chunks[i].video_size > max_video_size)
			{
				max_video_size = rpl->chunks[i].video_size;
			}
		}

		ASSERT(data_buffer == NULL, "Memory buffer not empty; memory loss in rpl_reader.c!");
		data_buffer = malloc(max_video_size);
		ASSERT(data_buffer != NULL, "Out of memory allocating %d bytes", (int)max_video_size);
	}

	return rpl;

error:
	free(rpl->chunks);
	free(rpl);
	return NULL;
}

static unsigned int rpl_decode_sound_none(RPL* rpl, int16_t* buffer, unsigned int buffer_size)
{
	return 0;
}

static unsigned int rpl_decode_sound_unknown(RPL* rpl, int16_t* buffer, unsigned int buffer_size)
{
	unsigned int i;
	unsigned total_audio_size = 0;
	char* audio_buffer;
	char* tmp;
	PHYSFS_file * out;

	debug( LOG_SOUND, "Saving unknown sound stream to file\n" );
	for (i = 0; i < rpl->nb_chunks; ++i)
	{
		total_audio_size += rpl->chunks[i].audio_size;
	}

	audio_buffer = (char*)malloc(total_audio_size);
	tmp = audio_buffer;

	for (i = 0; i < rpl->nb_chunks; ++i)
	{
		PHYSFS_seek(rpl->f, rpl->chunks[i].offset+rpl->chunks[i].video_size);
		PHYSFS_read(rpl->f, tmp, rpl->chunks[i].audio_size, 1);
		tmp += rpl->chunks[i].audio_size;
	}

	out = PHYSFS_openWrite("unknown_sound");
	PHYSFS_write(out, audio_buffer, total_audio_size, 1);
	PHYSFS_close(out);

	free(audio_buffer);

	return 0;
}

static unsigned int rpl_decode_sound_raw(RPL* rpl, int16_t* buffer, unsigned int buffer_size)
{
	unsigned int size = 0;
	short* tmp = buffer;

	while (1)
	{
		unsigned int cf = rpl->current_sound_frame;
		unsigned int audio_frame_size = rpl->chunks[cf].audio_size;

		if (rpl->current_sound_frame >= rpl->nb_chunks)
			break;

		if (size + (audio_frame_size >> 1) > buffer_size)
			break;

		PHYSFS_seek(rpl->f, rpl->chunks[cf].offset+rpl->chunks[cf].video_size);
		PHYSFS_read(rpl->f, tmp, audio_frame_size, 1);
		tmp += audio_frame_size >> 1;
		size += audio_frame_size >> 1;
		rpl->current_sound_frame++;
	}

	return size;
}

static unsigned int rpl_decode_sound_adpcm(RPL* rpl, int16_t* buffer, unsigned int buffer_size)
{
	static unsigned char* tmp_buffer = NULL;
	unsigned int tmp_buffer_size = 0;
	unsigned int size = 0;

	while (1)
	{
		unsigned int cf = rpl->current_sound_frame;
		unsigned int audio_frame_size = rpl->chunks[cf].audio_size;

		if (rpl->current_sound_frame >= rpl->nb_chunks)
			break;

		if (size + (audio_frame_size << 1) > buffer_size)
			break;

		if (audio_frame_size > tmp_buffer_size) {
			tmp_buffer_size = audio_frame_size << 1;
			free(tmp_buffer);
			tmp_buffer = (unsigned char*)malloc(tmp_buffer_size);
		}

		PHYSFS_seek(rpl->f, rpl->chunks[cf].offset+rpl->chunks[cf].video_size);
		PHYSFS_read(rpl->f, tmp_buffer, audio_frame_size, 1);
		adpcm_decode(rpl->sound_decoder_data, tmp_buffer, audio_frame_size, buffer);
		size += audio_frame_size << 1;
		rpl->current_sound_frame++;
	}

	return size;
}

unsigned int rpl_decode_sound(RPL* rpl, int16_t* buffer, unsigned int buffer_size)
{
	return rpl->sound_decoder(rpl, buffer, buffer_size);
}

static unsigned int rpl_decode_video_unknown(RPL* rpl, char* in, unsigned int in_size, char* out)
{
	unsigned int i, j;

	for (i = 0, j = 0; i < in_size; i += 2, j += 3)
	{
		out[j] = in[i];
		out[j+1] = in[i+1];
	}

	return 0;
}

int rpl_decode_next_image(RPL* rpl, char* buffer)
{
	unsigned int data_size;

	if (rpl->current_video_frame >= rpl->nb_chunks)
	{
		return -1;
	}

	data_size = rpl->chunks[rpl->current_video_frame].video_size;

	PHYSFS_seek(rpl->f, rpl->chunks[rpl->current_video_frame].offset);
	PHYSFS_read(rpl->f, data_buffer, data_size, 1);

	rpl->video_decoder(rpl, data_buffer, data_size, buffer);

	return rpl->current_video_frame++;
}

void rpl_close(RPL* rpl)
{
	if (rpl == NULL)
	{
		return;
	}

	switch (rpl->soundCodecID)
	{
		case 101:
			adpcm_finish(rpl->sound_decoder_data);
			break;
	}

	PHYSFS_close(rpl->f);
	free(data_buffer);
	data_buffer = NULL;
	free(rpl->chunks);
	free(rpl);
}
