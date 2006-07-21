#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#endif //looks like we need this. --Qamly
#include "adpcm.h"
#include "lib/framework/frame.h"
#include "rpl_reader.h"
#include <physfs.h>

unsigned int rpl_decode_sound_none(RPL* rpl, short* buffer, unsigned int buffer_size);
unsigned int rpl_decode_sound_unknown(RPL* rpl, short* buffer, unsigned int buffer_size);
unsigned int rpl_decode_sound_raw(RPL* rpl, short* buffer, unsigned int buffer_size);
unsigned int rpl_decode_sound_adpcm(RPL* rpl, short* buffer, unsigned int buffer_size);

unsigned int rpl_decode_video_unknown(RPL* rpl, char* in, unsigned int in_size, char* out);
WZ_DEPRECATED unsigned int dec130_decode(RPL* rpl, char* in, unsigned int in_size, char* out);

//*************************************************************************************

char* data_buffer = NULL;
unsigned int data_buffer_size = 0;

void resize_data_buffer(unsigned int size) {
	if (size > data_buffer_size) {
		if (data_buffer != NULL) {
			free(data_buffer);
		}
		data_buffer = malloc(size);
		data_buffer_size = size;
	}
}

//*************************************************************************************

static char *readline(PHYSFS_file *f, char *linebuf, size_t len) {
	char c;
	size_t i = 0;

	for (i = 0; i < len-1; i++) {
		if (PHYSFS_read(f, &c, 1, 1) != 1) {
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

static int readint(PHYSFS_file *f, char *linebuf, size_t len) {
	int num;
	readline(f, linebuf, len);
	if (sscanf(linebuf, "%u", &num) < 1)
		num = 0;
	return num;
}

static float readfloat(PHYSFS_file *f, char *linebuf, size_t len) {
	float num;
	readline(f, linebuf, len);
	if (sscanf(linebuf, "%f", &num) < 1)
		num = 0.0;
	return num;
}

RPL*
rpl_open(char* filename) {
	PHYSFS_file* f;
	RPL* rpl;
	char buf[80];
	int tmp;
	size_t len = sizeof(buf);

	/* FIXME: we should just clean up our data */
	for (tmp = 0; tmp < strlen(filename); tmp++) {
		if (filename[tmp] == '\\')
			filename[tmp] = '/';
	}

	f = PHYSFS_openRead(filename);
	if (f == NULL) {
		debug( LOG_ERROR, "Error reading %s: %s",
			  filename, PHYSFS_getLastError() );
		return NULL;
	}

	/* going to do lots of small reads, so use a buffer */
	PHYSFS_setBuffer(f, 1024);

	rpl = (RPL*)malloc(sizeof(RPL));
	rpl->chunks = NULL;

	rpl->f = f;

	if (strcmp(readline(f, buf, len), "ARMovie") != 0)
		DBPRINTF(("%s missing RPL magic number\n", filename));
	readline(f, buf, len); /* discard filename */
	readline(f, buf, len); /* discard copyright */
	if (strcmp(readline(f, buf, len), "ESCAPE 2.0") != 0)
		/* This field is really "author", but.. */
		DBPRINTF(("%s not in \"ESCAPE 2.0\" format?\n", filename));


	tmp = readint(f, buf, len);
	switch (tmp) {
		case 130:
			rpl->video_decoder = dec130_decode;
			break;
		default:
			rpl->video_decoder = rpl_decode_video_unknown;
			printf("Unknown video format %i\n", tmp);
			break;
	}

	rpl->width = readint(f, buf, len);
	//printf("width : %i\n", rpl->width);

	rpl->height = readint(f, buf, len);
	//printf("height : %i\n", rpl->height);

	rpl->bpp = readint(f, buf, len);
	//printf("bpp : %i\n", rpl->bpp);

	rpl->fps = readfloat(f, buf, len);
	//printf("fps : %f\n\n", rpl->fps);
	rpl->current_video_frame = 0;

	tmp = readint(f, buf, len);
	switch (tmp) {
		case 0:
			rpl->sound_decoder = rpl_decode_sound_none;
			break;
		case 1:
			rpl->sound_decoder = rpl_decode_sound_raw;
			break;
		case 101:
			rpl->sound_decoder = rpl_decode_sound_adpcm;
			break;
		default:
			rpl->sound_decoder = rpl_decode_sound_unknown;
			printf("Unknown sound format %i\n", tmp);
			break;
	}
	rpl->current_sound_frame = 0;

	rpl->samples = readint(f, buf, len);
	//printf("samples : %i\n", rpl->samples);

	rpl->channels = readint(f, buf, len);
	//printf("channels : %i\n", rpl->channels);

	rpl->bps = readint(f, buf, len);
	//printf("bits per sample : %i\n\n", rpl->bps);

	rpl->fpc = readint(f, buf, len);
	//printf("frames per chunk : %i\n", rpl->fpc);

	rpl->nb_chunks = readint(f, buf, len) + 1;
	//printf("chunks : %i\n", rpl->nb_chunks);

	rpl->ocs = readint(f, buf, len);
	//printf("odd chunk size : %i\n", rpl->ocs);

	rpl->ecs = readint(f, buf, len);
	//printf("even chunk size : %i\n", rpl->ecs);

	rpl->otcc = readint(f, buf, len);
	//printf("offset to chunk cat : %i\n\n", rpl->otcc);

	rpl->ots = readint(f, buf, len);
	//printf("offset to sprite : %i\n", rpl->ots);

	rpl->sprite_size = readint(f, buf, len);
	//printf("size of sprite : %i\n\n", rpl->sprite_size);

	rpl->otkf = readint(f, buf, len);
	//printf("offset to key frames : %i\n", rpl->otkf);

	{
		unsigned int i;
		unsigned int max_video_size = 0;

		rpl->chunks = malloc(sizeof(RPL_chunk_info_t)*rpl->nb_chunks);
		PHYSFS_seek(f, rpl->otcc);

		for (i = 0; i < rpl->nb_chunks; ++i) {
			readline(f, buf, len);
			if (sscanf(buf, "%i,%i;%i", &rpl->chunks[i].offset, &rpl->chunks[i].video_size, &rpl->chunks[i].audio_size) != 3) {
				printf("Error in chunk catalog\n");
				goto error;
			}

			if (rpl->chunks[i].video_size > max_video_size) {
				max_video_size = rpl->chunks[i].video_size;
			}
		}

		resize_data_buffer(max_video_size);
	}


	return rpl;

error:
	free(rpl->chunks);
	free(rpl);
	return NULL;
}

unsigned int rpl_decode_sound_none(RPL* rpl, short* buffer, unsigned int buffer_size) {
	return 0;
}

unsigned int rpl_decode_sound_unknown(RPL* rpl, short* buffer, unsigned int buffer_size) {
	unsigned int i;
	unsigned total_audio_size = 0;
	char* audio_buffer;
	char* tmp;
	PHYSFS_file * out;

	printf("Saving unknown sound stream to file\n");
	for (i = 0; i < rpl->nb_chunks; ++i) {
		total_audio_size += rpl->chunks[i].audio_size;
	}

	audio_buffer = malloc(total_audio_size);
	tmp = audio_buffer;

	for (i = 0; i < rpl->nb_chunks; ++i) {
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

unsigned int rpl_decode_sound_raw(RPL* rpl, short* buffer, unsigned int buffer_size) {
	unsigned int size = 0;
	short* tmp = buffer;

	while (1) {
		unsigned int cf = rpl->current_sound_frame;
		unsigned int audio_frame_size = rpl->chunks[cf].audio_size;

		if (rpl->current_sound_frame >= rpl->nb_chunks) {
			break;
		}
		if (size + (audio_frame_size >> 1) > buffer_size) {
			break;
		}
		PHYSFS_seek(rpl->f, rpl->chunks[cf].offset+rpl->chunks[cf].video_size);
		PHYSFS_read(rpl->f, tmp, audio_frame_size, 1);
		tmp += audio_frame_size >> 1;
		size += audio_frame_size >> 1;
		rpl->current_sound_frame++;
	}

	return size;
}

unsigned int rpl_decode_sound_adpcm(RPL* rpl, short* buffer, unsigned int buffer_size) {
	static unsigned char* tmp_buffer = NULL;
	unsigned int tmp_buffer_size = 0;
	unsigned int size = 0;
	short* tmp = buffer;

	while (1) {
		unsigned int cf = rpl->current_sound_frame;
		unsigned int audio_frame_size = rpl->chunks[cf].audio_size;

		if (rpl->current_sound_frame >= rpl->nb_chunks) {
			break;
		}
		if (size + (audio_frame_size << 1) > buffer_size) {
			break;
		}
		if (audio_frame_size > tmp_buffer_size) {
			tmp_buffer_size = audio_frame_size << 1;
			free(tmp_buffer);
			tmp_buffer = malloc(tmp_buffer_size);
		}
		PHYSFS_seek(rpl->f, rpl->chunks[cf].offset+rpl->chunks[cf].video_size);
		PHYSFS_read(rpl->f, tmp_buffer, audio_frame_size, 1);
		adpcm_decode(tmp_buffer, audio_frame_size, &tmp);
		size += audio_frame_size << 1;
		rpl->current_sound_frame++;
	}

	return size;
}

unsigned int rpl_decode_sound(RPL* rpl, short* buffer, unsigned int buffer_size) {
	return rpl->sound_decoder(rpl, buffer, buffer_size);
}

unsigned int rpl_decode_video_unknown(RPL* rpl, char* in, unsigned int in_size, char* out) {
	unsigned int i, j;

	for (i = 0, j = 0; i < in_size; i += 2, j += 3) {
		out[j] = in[i];
		out[j+1] = in[i+1];
	}

	return 0;
}

int rpl_decode_next_image(RPL* rpl, char* buffer)
{
	unsigned int data_size;

	if (rpl->current_video_frame >= rpl->nb_chunks) {
		return -1;
	}

	data_size = rpl->chunks[rpl->current_video_frame].video_size;

	PHYSFS_seek(rpl->f, rpl->chunks[rpl->current_video_frame].offset);
	PHYSFS_read(rpl->f, data_buffer, data_size, 1);

	rpl->video_decoder(rpl, data_buffer, data_size, buffer);

	return rpl->current_video_frame++;
}

void
rpl_close(RPL* rpl) {
	if (rpl != NULL) {
		PHYSFS_close(rpl->f);
		free(rpl->chunks);
		free(rpl);
	}
}

