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
#ifndef __RPL_READER_H__
#define __RPL_READER_H__

#include "lib/framework/frame.h"

#include <stdio.h>
#include <physfs.h>

typedef struct RPL_chunk_info_t
{
	int offset;
	unsigned int video_size;
	int audio_size;
} RPL_chunk_info_t;

typedef struct RPL
{
	PHYSFS_file*		f;

	// Video attributes
	int			width;
	int			height;
	int			bpp;
	float			fps;
	unsigned int		(*video_decoder)(struct RPL*, char*, unsigned int, char*);
	unsigned int		current_video_frame;

	// Audio attributes
	unsigned int soundCodecID;
	int			samples;
	int			channels;
	int			bps;
	int			fpc;
	unsigned int		(*sound_decoder)(struct RPL*, int16_t*, unsigned int);
	unsigned int		current_sound_frame;
	void*       sound_decoder_data;

	// Chunk-related info
	unsigned int		nb_chunks;
	RPL_chunk_info_t*	chunks;
	int			ocs;
	int			ecs;
	int			otcc;

	int			ots;
	int			sprite_size;

	int			otkf;
} RPL;

extern RPL*		rpl_open(const char* filename);
extern unsigned int	rpl_decode_sound(RPL* rpl, int16_t* buffer, unsigned int buffer_size);
extern int		rpl_decode_next_image(RPL* rpl, char* buffer);
extern void		rpl_close(RPL* rpl);

#endif // __RPL_READER_H__
