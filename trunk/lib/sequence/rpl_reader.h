#ifndef __RPL_READER_H__
#define __RPL_READER_H__

#include <stdio.h>
#include <physfs.h>

typedef struct RPL_chunk_info_t {
	int offset;
	int video_size;
	int audio_size;
} RPL_chunk_info_t;

typedef struct RPL {
	PHYSFS_file*		f;

	// Video attributes
	int			width;
	int			height;
	int			bpp;
	float			fps;
	unsigned int		(*video_decoder)(struct RPL*, char*, unsigned int, char*);
	unsigned int		current_video_frame;

	// Audio attributes
	int			samples;
	int			channels;
	int			bps;
	int			fpc;
	unsigned int		(*sound_decoder)(struct RPL*, short*, unsigned int);
	unsigned int		current_sound_frame;

	// Chunk-related info
	int			nb_chunks;
	RPL_chunk_info_t*	chunks;
	int			ocs;
	int			ecs;
	int			otcc;

	int			ots;
	int			sprite_size;

	int			otkf;
} RPL;

extern RPL*		rpl_open(char* filename);
extern unsigned int	rpl_decode_sound(RPL* rpl, short* buffer, unsigned int buffer_size);
extern int		rpl_decode_next_image(RPL* rpl, char* buffer);
extern void		rpl_close(RPL* rpl);

#endif

