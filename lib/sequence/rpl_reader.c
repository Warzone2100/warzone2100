#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#endif //looks like we need this. --Qamly
#include "adpcm.h"
#include "frame.h"
#include "rpl_reader.h"

unsigned int rpl_decode_sound_none(RPL* rpl, short* buffer, unsigned int buffer_size);
unsigned int rpl_decode_sound_unknown(RPL* rpl, short* buffer, unsigned int buffer_size);
unsigned int rpl_decode_sound_raw(RPL* rpl, short* buffer, unsigned int buffer_size);
unsigned int rpl_decode_sound_adpcm(RPL* rpl, short* buffer, unsigned int buffer_size);

unsigned int rpl_decode_video_unknown(RPL* rpl, char* in, unsigned int in_size, char* out);
unsigned int dec130_decode(RPL* rpl, char* in, unsigned int in_size, char* out);

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

RPL*
rpl_open(char* filename) {
	FILE* f = fopen(filename, "rb");						//Hmm...open from .wz or not?
	RPL* rpl;														//it is only looking for novideo.rpl for right now.
	char buffer[512];											//The only issue I see, if we use a memory buffer,
	int tmp;															//and the video is too large, some system may
																		//run out of memory. So we may have to redo this
	if (f == NULL) {												//routine when we change vid formats. -Q
		return NULL;
	}

	rpl = (RPL*)malloc(sizeof(RPL));
	rpl->chunks = NULL;

	rpl->f = f;

	fgets(buffer, 511, rpl->f);
	fgets(buffer, 511, rpl->f);
	fgets(buffer, 511, rpl->f);
	fgets(buffer, 511, rpl->f);

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &tmp);
	switch (tmp) {
		case 130:
			rpl->video_decoder = dec130_decode;
			break;
		default:
			rpl->video_decoder = rpl_decode_video_unknown;
			printf("Unknown sound format %i\n", tmp);
			break;
	}
	
	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->width);
	//printf("width : %i\n", rpl->width); 
	
	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->height);
	//printf("height : %i\n", rpl->height); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->bpp);
	//printf("bpp : %i\n", rpl->bpp); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%f", &rpl->fps);
	//printf("fps : %f\n\n", rpl->fps); 
	rpl->current_video_frame = 0;

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &tmp);
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
	
	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->samples);
	//printf("samples : %i\n", rpl->samples); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->channels);
	//printf("channels : %i\n", rpl->channels); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->bps);
	//printf("bits per sample : %i\n\n", rpl->bps); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->fpc);
	//printf("frames per chunk : %i\n", rpl->fpc); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->nb_chunks);
	rpl->nb_chunks++;
	//printf("chunks : %i\n", rpl->nb_chunks); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->ocs);
	//printf("odd chunk size : %i\n", rpl->ocs); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->ecs);
	//printf("even chunk size : %i\n", rpl->ecs); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->otcc);
	//printf("offset to chunk cat : %i\n\n", rpl->otcc); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->ots);
	//printf("offset to sprite : %i\n", rpl->ots); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->sprite_size);
	//printf("size of sprite : %i\n\n", rpl->sprite_size); 

	fgets(buffer, 511, rpl->f);
	sscanf(buffer, "%i", &rpl->otkf);
	//printf("offset to key frames : %i\n", rpl->otkf); 

	{
		unsigned int i;
		unsigned int max_video_size = 0;

		rpl->chunks = malloc(sizeof(RPL_chunk_info_t)*rpl->nb_chunks);
		fseek(rpl->f, rpl->otcc, SEEK_SET);

		for (i = 0; i < rpl->nb_chunks; ++i) {
			fgets(buffer, 511, rpl->f);
			if (sscanf(buffer, "%i,%i;%i", &rpl->chunks[i].offset, &rpl->chunks[i].video_size, &rpl->chunks[i].audio_size) != 3) {
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
	FILE* out;

	printf("Saving unknown sound stream to file\n");
	for (i = 0; i < rpl->nb_chunks; ++i) {
		total_audio_size += rpl->chunks[i].audio_size;
	}

	audio_buffer = malloc(total_audio_size);
	tmp = audio_buffer;

	for (i = 0; i < rpl->nb_chunks; ++i) {
		fseek(rpl->f, rpl->chunks[i].offset+rpl->chunks[i].video_size, SEEK_SET);
		fread(tmp, rpl->chunks[i].audio_size, 1, rpl->f);
		tmp += rpl->chunks[i].audio_size;
	}

	out = fopen("unknown_sound", "wb");
	fwrite(audio_buffer, total_audio_size, 1, out);
	fclose(out);

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
		fseek(rpl->f, rpl->chunks[cf].offset+rpl->chunks[cf].video_size, SEEK_SET);
		fread(tmp, audio_frame_size, 1, rpl->f);
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
		fseek(rpl->f, rpl->chunks[cf].offset+rpl->chunks[cf].video_size, SEEK_SET);
		fread(tmp_buffer, audio_frame_size, 1, rpl->f);
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
	unsigned int data_size = rpl->chunks[rpl->current_video_frame].video_size;

	if (rpl->current_video_frame >= rpl->nb_chunks) {
		return -1;
	}

//	unsigned int data_size = rpl->chunks[rpl->current_video_frame].video_size;		//can't have this here in .c rules that is. ;)

	fseek(rpl->f, rpl->chunks[rpl->current_video_frame].offset, SEEK_SET);
	fread(data_buffer, data_size, 1, rpl->f);

	rpl->video_decoder(rpl, data_buffer, data_size, buffer);

	return rpl->current_video_frame++;
}

void
rpl_close(RPL* rpl) {
	if (rpl != NULL) {
		fclose(rpl->f);
		free(rpl->chunks);
		free(rpl);
	}
}

