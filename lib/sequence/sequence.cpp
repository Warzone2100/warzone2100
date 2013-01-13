/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2012  Warzone 2100 Project

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

/* This file is derived from the SDL player example as found in the OggTheora
 * software codec source code. In particular this is examples/player_example.c
 * as found in OggTheora 1.0beta3.
 *
 * The copyright to this file was originally owned by and licensed as follows.
 * Please note, however, that *this* file, i.e. the one you are currently
 * reading is not licensed as such anymore.
 *
 * Copyright (C) 2002-2007 Xiph.org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lib/framework/frame.h"
#include "lib/framework/frameint.h"
#include "lib/framework/opengl.h"
#include "sequence.h"
#include "timer.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/sound/audio.h"
#include "lib/sound/openal_error.h"
#include "lib/sound/mixer.h"

#include <theora/theora.h>
#include <physfs.h>

#if !defined(WZ_NOSOUND)
# include <vorbis/codec.h>

# if defined( WZ_OS_MAC)
#  include <OpenAL/al.h>
# else
#  include <AL/al.h>
# endif

// stick this in sequence.h perhaps?
struct AudioData
{
	ALuint buffer1;			// buffer 1
	ALuint buffer2;			// buffer 2
	ALuint source;			// source
	int totbufstarted;		// number of buffers started
	int audiofd_fragsize;	// audio fragment size, used to calculate how big audiobuf is
	int audiobuf_fill;		// how full our audio buffer is
};

#endif

struct VideoData
{
	ogg_sync_state oy;		// ogg sync state
	ogg_page og;			// ogg page
	ogg_stream_state vo;	// ogg stream state
	ogg_stream_state to;	// ogg stream state
	theora_info ti;			// theora info
	theora_comment tc;		// theora comment
	theora_state td;		// theora state
#if !defined(WZ_NOSOUND)
	vorbis_info vi;			// vorbis info
	vorbis_dsp_state vd;	// vorbis display state
	vorbis_block vb;		// vorbis block
	vorbis_comment vc;		// vorbis comment
#endif
};
// stick that in sequence.h perhaps?

#if !defined(WZ_NOSOUND)
// for our audio structure
static AudioData audiodata;
static ALint sourcestate = 0;		//Source state information
#endif

// for our video structure
static VideoData videodata;

static int theora_p = 0;

#if !defined(WZ_NOSOUND)
static int vorbis_p = 0;
#endif

static bool stateflag = false;
static bool videoplaying = false;
static bool videobuf_ready = false;		// single frame video buffer ready for processing

#if !defined(WZ_NOSOUND)
static bool audiobuf_ready = false;		// single 'frame' audio buffer ready for processing
#endif

// file handle
static PHYSFS_file* fpInfile = NULL;

static uint32_t* RGBAframe = NULL;					// texture buffer

#if !defined(WZ_NOSOUND)
static ogg_int16_t* audiobuf = NULL;			// audio buffer
#endif


// For timing
#if !defined(WZ_NOSOUND)
static double audioTime = 0;
#endif

static double videobuf_time = 0;
static double sampletimeOffset = 0;
static double basetime = -1;
static double last_time;
static double timer_expire;
static bool timer_started = false;

#if !defined(WZ_NOSOUND)
static ogg_int64_t audiobuf_granulepos = 0;	// time position of last sample
#endif

static ogg_int64_t videobuf_granulepos = -1;	// time position of last video frame

// frame & dropped frame counter
static int frames = 0;
static int dropped = 0;

// Screen dimensions
#define NUM_VERTICES 4
static GLuint buffers[VBO_COUNT];
static GLfloat vertices[NUM_VERTICES][2];
static GLfloat Scrnvidpos[3];

static SCANLINE_MODE use_scanlines;

// Helper; just grab some more compressed bitstream and sync it for page extraction
static int buffer_data(PHYSFS_file* in, ogg_sync_state* oy)
{
	// read in 256K chunks
	const int size = 262144;
	char *buffer = ogg_sync_buffer(oy, size);
	int bytes = PHYSFS_read(in, buffer, 1, size);

	ogg_sync_wrote(oy, bytes);
	return(bytes);
}

/** helper: push a page into the appropriate stream
	this can be done blindly; a stream won't accept a page
	that doesn't belong to it
*/
static int queue_page(ogg_page *page)
{
	if (theora_p)
	{
		ogg_stream_pagein(&videodata.to, page);
	}

#if !defined(WZ_NOSOUND)
	if (vorbis_p)
	{
		ogg_stream_pagein(&videodata.vo, page);
	}
#endif

	return 0;
}

// sets the frames number we are on
static void seq_SetFrameNumber(int frame)
{
	frames = frame;
}

#if !defined(WZ_NOSOUND)
/// @TODO FIXME:  This routine can & will fail when sources are used up!
static void open_audio(void)
{
	float volume = 1.0;

	audiodata.audiofd_fragsize = (((videodata.vi.channels * 16) / 8) * videodata.vi.rate);
	audiobuf = (ogg_int16_t *)malloc(audiodata.audiofd_fragsize);

	// FIX ME:  This call will fail, since we have, most likely, already
	// used up all available sources late in the game!
	// openal
	alGenSources(1, &audiodata.source);
	sound_GetError();	// on error, we *need* to free another source!

	// Create an OpenAL buffer and fill it with the decoded data
	alGenBuffers(1, &audiodata.buffer1);
	sound_GetError();
	alGenBuffers(1, &audiodata.buffer2);
	sound_GetError();	// Log and clear error codes

	audiodata.totbufstarted = 0;

	// set the volume of the FMV based on the user's preferences
	volume = sound_GetUIVolume();
	alSourcef(audiodata.source, AL_GAIN, volume);
}

/** Cleans up audio sources & buffers
 */
static void audio_close(void)
{
	// NOTE: sources & buffers deleted in seq_Shutdown()
	audiobuf_granulepos = 0;
//	clear struct
//	memset(&audiodata,0x0,sizeof(audiodata));
	audiodata.audiobuf_fill = 0;
	audiodata.audiofd_fragsize = 0;
	audiodata.source = 0;
	audiodata.buffer1 = audiodata.buffer2 = 0;
	if (audiobuf)
	{
		free(audiobuf);
		audiobuf = NULL;
	}
}
#endif

// Retrieves the current time with millisecond accuracy
static double getTimeNow(void)
{
	return Timer_getElapsedMilliSecs();
}

// get relative time since beginning playback, compensating for A/V drift
static double getRelativeTime(void)
{
	if (basetime == -1)					// check to see if this is first time run
	{
		basetime = getTimeNow();
		timer_expire = Timer_getElapsedMicroSecs();
		timer_expire += (int)((videobuf_time - getTimeNow()) * 1000000.0);
		timer_started = true;
	}
	return((getTimeNow() - basetime) * .001);
}

const GLfloat texture_width = 1024.0f;
const GLfloat texture_height = 1024.0f;
static GLuint video_texture;

/** Allocates memory to hold the decoded video frame
 */
static void Allocate_videoFrame(void)
{
	int size = videodata.ti.frame_width * videodata.ti.frame_height * 4;
	if (use_scanlines)
		size *= 2;

	RGBAframe = (uint32_t *)malloc(size);
	memset(RGBAframe, 0, size);
	glGenTextures(1, &video_texture);
}

static void deallocateVideoFrame(void)
{
	if (RGBAframe)
		free(RGBAframe);
	glDeleteTextures(1, &video_texture);
}

#ifndef __BIG_ENDIAN__
const int Rshift = 0;
const int Gshift = 8;
const int Bshift = 16;
const int Ashift = 24;
// RGBmask is used only after right-shifting, so ignore the leftmost bit of each byte
const int RGBmask = 0x007f7f7f;
const int Amask = 0xff000000;
#else
const int Rshift = 24;
const int Gshift = 16;
const int Bshift = 8;
const int Ashift = 0;
const int RGBmask = 0x7f7f7f00;
const int Amask = 0x000000ff;
#endif
#define Vclip( x )	( (x > 0) ? ((x < 255) ? x : 255) : 0 )
// main routine to display video on screen.
static void video_write(bool update)
{
	unsigned int x = 0, y = 0;
	const int video_width = videodata.ti.frame_width;
	const int video_height = videodata.ti.frame_height;
	// when using scanlines we need to double the height
	const int height_factor = (use_scanlines ? 2 : 1);
	yuv_buffer yuv;

	glErrors();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, video_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (update)
	{
		int rgb_offset = 0;
		int y_offset = 0;
		int uv_offset = 0;
		const int half_width = video_width / 2;

		theora_decode_YUVout(&videodata.td, &yuv);

		// fill the RGBA buffer
		for (y = 0; y < video_height; y++)
		{
			y_offset = y * yuv.y_stride;
			uv_offset = (y >> 1) * yuv.uv_stride;

			for (x = 0; x < half_width; x++)
			{
				int Y = yuv.y[y_offset++] - 16;
				const int U = yuv.u[uv_offset] - 128;
				const int V = yuv.v[uv_offset++] - 128;

				int A = 298 * Y;
				const int C = 409 * V;

				int R = Vclip((A + C + 128) >> 8);
				int G = Vclip((A - 100 * U - (C >> 1) + 128) >> 8);
				int B = Vclip((A + 516 * U + 128) >> 8);

				uint32_t rgba = (R << Rshift) | (G << Gshift) | (B << Bshift) | (0xFF << Ashift);

				RGBAframe[rgb_offset] = rgba;
				if (use_scanlines == SCANLINES_50)
				{
					// halve the rgb values for a dimmed scanline
					RGBAframe[rgb_offset + video_width] = (rgba >> 1 & RGBmask) | Amask;
				}
				else if (use_scanlines == SCANLINES_BLACK)
				{
					RGBAframe[rgb_offset + video_width] = Amask;
				}
				rgb_offset++;

				// second pixel, U and V (and thus C) are the same as before.
				Y = yuv.y[y_offset++] - 16;
				A = 298 * Y;

				R = Vclip((A + C + 128) >> 8);
				G = Vclip((A - 100 * U - (C >> 1) + 128) >> 8);
				B = Vclip((A + 516 * U + 128) >> 8);

				rgba = (R << Rshift) | (G << Gshift) | (B << Bshift) | (0xFF << Ashift);
				RGBAframe[rgb_offset] = rgba;
				if (use_scanlines == SCANLINES_50)
				{
					// halve the rgb values for a dimmed scanline
					RGBAframe[rgb_offset + video_width] = (rgba >> 1 & RGBmask) | Amask;
				}
				else if (use_scanlines == SCANLINES_BLACK)
				{
					RGBAframe[rgb_offset + video_width] = Amask;
				}
				rgb_offset++;
			}
			if (use_scanlines)
				rgb_offset += video_width;
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width,
				video_height * height_factor, GL_RGBA, GL_UNSIGNED_BYTE, RGBAframe);
		glErrors();
	}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glPushMatrix();
	glTranslatef(Scrnvidpos[0], Scrnvidpos[1], Scrnvidpos[2]);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[VBO_TEXCOORD]); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[VBO_VERTEX]); glVertexPointer(2, GL_FLOAT, 0, NULL);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPopMatrix();
	glErrors();

	// Make sure the current texture page is reloaded after we are finished
	// Otherwise WZ will think it is still loaded and not load it again
	pie_SetTexturePage(TEXPAGE_NONE);
}

#if !defined(WZ_NOSOUND)
// FIXME: perhaps we should use wz's routine for audio?
// loads up the audio buffers, and calculates audio sync time.
static void audio_write(void)
{
	ALint processed = 0;
	ALint queued = 0;

	alGetSourcei(audiodata.source, AL_BUFFERS_PROCESSED, &processed);
	alGetSourcei(audiodata.source, AL_BUFFERS_QUEUED, &queued);
	if ((audiodata.totbufstarted < 2 || processed) && audiodata.audiobuf_fill > 0)
	{
		// we have audiobuf_fill bytes of data
		ALuint oldbuffer = 0;

		if (audiodata.totbufstarted == 0)
		{
			oldbuffer = audiodata.buffer1;
		}
		else if (audiodata.totbufstarted == 1)
		{
			oldbuffer = audiodata.buffer2;
		}
		else
		{
			ALint buffer_size = 0;
			ogg_int64_t current_sample = 0;

			alSourceUnqueueBuffers(audiodata.source, 1, &oldbuffer);
			alGetBufferi(oldbuffer, AL_SIZE, &buffer_size);
			// audio time sync
			audioTime += (double) buffer_size / (videodata.vi.rate * videodata.vi.channels);
			debug(LOG_VIDEO, "Audio sync");
			current_sample = audiobuf_granulepos - audiodata.audiobuf_fill / 2 / videodata.vi.channels;
			sampletimeOffset -= getTimeNow() - 1000 * current_sample / videodata.vi.rate;
		}

		alBufferData(oldbuffer, (videodata.vi.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16),
		        audiobuf, audiodata.audiobuf_fill, videodata.vi.rate);

		alSourceQueueBuffers(audiodata.source, 1, &oldbuffer);
		audiodata.totbufstarted++;
		if (audiodata.totbufstarted > 2)
		{
			audiodata.totbufstarted = 2;
		}

		if (sourcestate != AL_PLAYING)
		{
			debug(LOG_VIDEO, "starting source\n");
			alSourcePlay(audiodata.source);
		}

		audiobuf_ready = 0;
		audiodata.audiobuf_fill = 0;
	}
}
#endif

static void seq_InitOgg(void)
{
	debug(LOG_VIDEO, "seq_InitOgg");

	stateflag = false;
	theora_p = 0;

#if !defined(WZ_NOSOUND)
	vorbis_p = 0;
#endif

	videoplaying = false;

	/* single frame video buffering */
	videobuf_ready = false;
	videobuf_granulepos = -1;
	videobuf_time = 0;
	frames = 0;
	dropped = 0;

#if !defined(WZ_NOSOUND)
	/* single audio fragment audio buffering */
	audiodata.audiobuf_fill = 0;
	audiobuf_ready = false;

	audiobuf_granulepos = 0;	/* time position of last sample */

	audioTime = 0;
#endif

	sampletimeOffset = 0;

	/* start up Ogg stream synchronization layer */
	ogg_sync_init(&videodata.oy);

#if !defined(WZ_NOSOUND)
	/* init supporting Vorbis structures needed in header parsing */
	vorbis_info_init(&videodata.vi);
	vorbis_comment_init(&videodata.vc);
#endif

	/* init supporting Theora structuretotbufstarteds needed in header parsing */
	theora_comment_init(&videodata.tc);
	theora_info_init(&videodata.ti);
	Timer_Init();
	Timer_start();
}

bool seq_Play(const char* filename)
{
	int pp_level_max = 0;
	int pp_level = 0;
	ogg_packet op;

	debug(LOG_VIDEO, "starting playback of: %s", filename);

	if (videoplaying)
	{
		debug(LOG_VIDEO, "previous movie is not yet finished");
		seq_Shutdown();
	}

	seq_InitOgg();

	fpInfile = PHYSFS_openRead(filename);
	if (fpInfile == NULL)
	{
		info("unable to open '%s' for playback", filename);

		fpInfile = PHYSFS_openRead("novideo.ogg");
		if (fpInfile == NULL)
		{
			return false;
		}
	}

	theora_p = 0;

#if !defined(WZ_NOSOUND)
	vorbis_p = 0;
#endif

	/* Ogg file open; parse the headers */
	/* Only interested in Vorbis/Theora streams */
	while (!stateflag)
	{
		int ret = buffer_data(fpInfile, &videodata.oy);

		if (ret == 0)
		{
			break;
		}

		while (ogg_sync_pageout(&videodata.oy, &videodata.og) > 0)
		{
			ogg_stream_state test;

			/* is this a mandated initial header? If not, stop parsing */
			if (!ogg_page_bos(&videodata.og))
			{
				/* don't leak the page; get it into the appropriate stream */
				queue_page(&videodata.og);
				stateflag = 1;
				break;
			}

			ogg_stream_init(&test, ogg_page_serialno(&videodata.og));
			ogg_stream_pagein(&test, &videodata.og);
			ogg_stream_packetout(&test, &op);

			/* identify the codec: try theora */
			if (!theora_p && theora_decode_header(&videodata.ti, &videodata.tc, &op) >= 0)
			{
				/* it is theora */
				memcpy(&videodata.to, &test, sizeof(test));
				theora_p = 1;
			}
#if !defined(WZ_NOSOUND)
			else if (!vorbis_p && vorbis_synthesis_headerin(&videodata.vi, &videodata.vc, &op) >= 0)
			{
				/* it is vorbis */
				memcpy(&videodata.vo, &test, sizeof(test));
				vorbis_p = 1;
			}
#endif
			else
			{
				/* whatever it is, we don't care about it */
				ogg_stream_clear(&test);
			}
		}
		/* fall through to non-bos page parsing */
	}

	/* we're expecting more header packets. */
	while ((theora_p && theora_p < 3)
#if !defined(WZ_NOSOUND)
	    || (vorbis_p && vorbis_p < 3)
#endif
	    )
	{
		int ret;

		/* look for further theora headers */
		while (theora_p && (theora_p < 3) && (ret = ogg_stream_packetout(&videodata.to, &op)))
		{
			if (ret < 0)
			{
				debug(LOG_ERROR, "Error parsing Theora stream headers; corrupt stream?\n");
				return false;
			}

			if (theora_decode_header(&videodata.ti, &videodata.tc, &op))
			{
				debug(LOG_ERROR, "Error parsing Theora stream headers; corrupt stream?\n");
				return false;
			}

			theora_p++;
		}

#if !defined(WZ_NOSOUND)
		/* look for more vorbis header packets */
		while (vorbis_p && (vorbis_p < 3) && (ret = ogg_stream_packetout(&videodata.vo, &op)))
		{
			if (ret < 0)
			{
				debug(LOG_ERROR, "Error parsing Vorbis stream headers; corrupt stream?\n");
				return false;
			}

			if (vorbis_synthesis_headerin(&videodata.vi, &videodata.vc, &op))
			{
				debug(LOG_ERROR, "Error parsing Vorbis stream headers; corrupt stream?\n");
				return false;
			}

			vorbis_p++;
		}
#endif

		/* The header pages/packets will arrive before anything else we
				care about, or the stream is not obeying spec */
		if (ogg_sync_pageout(&videodata.oy, &videodata.og) > 0)
		{
			queue_page(&videodata.og);	/* demux into the appropriate stream */
		}
		else
		{
			int ret = buffer_data(fpInfile, &videodata.oy);   /* someone needs more data */

			if (ret == 0)
			{
				debug(LOG_ERROR, "End of file while searching for codec headers.\n");
				return false;
			}
		}
	}

	/* and now we have it all.  initialize decoders */
	if (theora_p)
	{
		theora_decode_init(&videodata.td, &videodata.ti);
		debug(LOG_VIDEO, "Ogg logical stream %x is Theora %dx%d %.02f fps video",
		      (unsigned int) videodata.to.serialno, (int) videodata.ti.width, (int) videodata.ti.height,
		      (double) videodata.ti.fps_numerator / videodata.ti.fps_denominator);
		if (videodata.ti.width != videodata.ti.frame_width || videodata.ti.height != videodata.ti.frame_height)
		{
			debug(LOG_VIDEO, "  Frame content is %dx%d with offset (%d,%d)", videodata.ti.frame_width,
			      videodata.ti.frame_height, videodata.ti.offset_x, videodata.ti.offset_y);
		}

		// hmm
		theora_control(&videodata.td, TH_DECCTL_GET_PPLEVEL_MAX, &pp_level_max, sizeof(pp_level_max));
		pp_level = pp_level_max;
		theora_control(&videodata.td, TH_DECCTL_SET_PPLEVEL, &pp_level, sizeof(pp_level));
	}
	else
	{
		/* tear down the partial theora setup */
		theora_info_clear(&videodata.ti);
		theora_comment_clear(&videodata.tc);
	}

#if !defined(WZ_NOSOUND)
	if (vorbis_p)
	{
		vorbis_synthesis_init(&videodata.vd, &videodata.vi);
		vorbis_block_init(&videodata.vd, &videodata.vb);
		debug(LOG_VIDEO, "Ogg logical stream %x is Vorbis %d channel %d Hz audio",
		      (unsigned int) videodata.vo.serialno, videodata.vi.channels, (int) videodata.vi.rate);
	}
	else
	{
		/* tear down the partial vorbis setup */
		vorbis_info_clear(&videodata.vi);
		vorbis_comment_clear(&videodata.vc);
	}

	/* open audio */
	if (vorbis_p)
	{
		open_audio();
	}
#endif

	/* open video */
	glGenBuffers(VBO_MINIMAL, buffers);
	glErrors();
	if (theora_p)
	{
		if (videodata.ti.frame_width > texture_width || videodata.ti.frame_height > texture_height)
		{
			debug(LOG_ERROR, "Video size too large, must be below %.gx%.g!",
					texture_width, texture_height);
			glDeleteBuffers(VBO_MINIMAL, buffers);
			return false;
		}
		if (videodata.ti.pixelformat != OC_PF_420)
		{
			debug(LOG_ERROR, "Video not in YUV420 format!");
			glDeleteBuffers(VBO_MINIMAL, buffers);
			return false;
		}
		char *blackframe = (char *)calloc(1, texture_width * texture_height * 4);

		// disable scanlines if the video is too large for the texture or shown too small
		if (videodata.ti.frame_height * 2 > texture_height || vertices[3][1] < videodata.ti.frame_height * 2)
			use_scanlines = SCANLINES_OFF;

		Allocate_videoFrame();

		glBindTexture(GL_TEXTURE_2D, video_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height,
				0, GL_RGBA, GL_UNSIGNED_BYTE, blackframe);
		free(blackframe);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		// when using scanlines we need to double the height
		const int height_factor = (use_scanlines ? 2 : 1);
		const GLfloat vtwidth = (float)videodata.ti.frame_width / texture_width;
		const GLfloat vtheight = (float)videodata.ti.frame_height * height_factor / texture_height;
		GLfloat texcoords[NUM_VERTICES * 2] = { 0.0f, 0.0f, vtwidth, 0.0f, 0.0f, vtheight, vtwidth, vtheight };
		glBindBuffer(GL_ARRAY_BUFFER, buffers[VBO_TEXCOORD]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[VBO_VERTEX]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glErrors();
	}

	/* on to the main decode loop.  We assume in this example that audio
		and video start roughly together, and don't begin playback until
		we have a start frame for both.  This is not necessarily a valid
		assumption in Ogg A/V streams! It will always be true of the
		example_encoder (and most streams) though. */
	sampletimeOffset = getTimeNow();
	videoplaying = true;
	return true;
}

bool seq_Playing()
{
	return videoplaying;
}

/**
 * Display the next frame and play the sound.
 * \return false if the end of the video is reached.
 */
bool seq_Update()
{
	ogg_packet op;
	int ret;
#if !defined(WZ_NOSOUND)
	int i, j;
	float **  pcm;
	int count, maxsamples;
#endif

	/* we want a video and audio frame ready to go at all times.  If
		   we have to buffer incoming, buffer the compressed data (ie, let
		   ogg do the buffering) */
	if (!videoplaying)
	{
		debug(LOG_VIDEO, "no movie playing");
		return false;
	}

#if !defined(WZ_NOSOUND)
	while (vorbis_p && !audiobuf_ready)
	{
		/* if there's pending, decoded audio, grab it */
		if ((ret = vorbis_synthesis_pcmout(&videodata.vd, &pcm)) > 0)
		{
			// we now have float pcm data in pcm
			// going to convert that to int pcm in audiobuf
			count = audiodata.audiobuf_fill / 2;
			maxsamples = (audiodata.audiofd_fragsize - audiodata.audiobuf_fill) / 2 / videodata.vi.channels;

			for (i = 0; i < ret && i < maxsamples; i++)
			{
				for (j = 0; j < videodata.vi.channels; j++)
				{
					int val = nearbyint(pcm[j][i] * 32767.f);

					if (val > 32767)
					{
						val = 32767;
					}
					else if (val < -32768)
					{
						val = -32768;
					}
					audiobuf[count++] = val;
				}
			}

			vorbis_synthesis_read(&videodata.vd, i);
			audiodata.audiobuf_fill += i * videodata.vi.channels * 2;

			if (audiodata.audiobuf_fill == audiodata.audiofd_fragsize)
			{
				audiobuf_ready = true;
			}

			if (videodata.vd.granulepos >= 0)
			{
				audiobuf_granulepos = videodata.vd.granulepos - ret + i;
			}
			else
			{
				audiobuf_granulepos += i;
			}
		}
		else
		{
			/* no pending audio; is there a pending packet to decode? */
			if (ogg_stream_packetout(&videodata.vo, &op) > 0)
			{
				if (vorbis_synthesis(&videodata.vb, &op) == 0)
				{	/* test for success! */
					vorbis_synthesis_blockin(&videodata.vd, &videodata.vb);
				}
			}
			else
			{	/* we need more data; break out to suck in another page */
				break;
			}
		}
	}
#endif

	while (theora_p && !videobuf_ready)
	{
		/* theora is one in, one out... */
		if (ogg_stream_packetout(&videodata.to, &op) > 0)
		{
			double now_time = 0;
			double delay = 0;

			theora_decode_packetin(&videodata.td, &op);
			videobuf_granulepos = videodata.td.granulepos;
			videobuf_time = theora_granule_time(&videodata.td, videobuf_granulepos);

			now_time = getRelativeTime();
			delay = videobuf_time - getRelativeTime();

			if ((delay >= 0.0f) || (now_time - last_time >= 1.0f))
			{
				videobuf_ready = true;
				seq_SetFrameNumber(seq_GetFrameNumber() + 1);
			}
			else
			{
				// running slow, so we skip this frame
				dropped++;
			}
		}
		else
		{
			break;
		}
	}

#if !defined(WZ_NOSOUND)
	alGetSourcei(audiodata.source, AL_SOURCE_STATE, &sourcestate);
#endif

	if (PHYSFS_eof(fpInfile)
		&& !videobuf_ready
#if !defined(WZ_NOSOUND)
		&& ((!audiobuf_ready && (audiodata.audiobuf_fill == 0)) || audio_Disabled())
		&& sourcestate != AL_PLAYING
#endif
	 )
	{
		video_write(false);
		seq_Shutdown();
		debug(LOG_VIDEO, "video finished");
		return false;
	}

	if (!videobuf_ready
#if !defined(WZ_NOSOUND)
	 || !audiobuf_ready
#endif
	 )
	{
		/* no data yet for somebody.  Grab another page */
		ret = buffer_data(fpInfile, &videodata.oy);
		while (ogg_sync_pageout(&videodata.oy, &videodata.og) > 0)
		{
			queue_page(&videodata.og);
		}
	}

#if !defined(WZ_NOSOUND)
	/* If playback has begun, top audio buffer off immediately. */
	if (vorbis_p
	 && stateflag
	// FIXME : it is possible to crash if people are playing with no sound.
	 && !audio_Disabled())
	{
		// play the data in pcm
		audio_write();
	}
#endif

	/* are we at or past time for this video frame? */
	if (stateflag && videobuf_ready && (videobuf_time <= getRelativeTime()))
	{
		video_write(true);
		last_time = getRelativeTime();
		videobuf_ready = false;
	}
	else if (stateflag)
	{
		video_write(false);
	}

	/* if our buffers either don't exist or are ready to go,
		   we can begin playback */
	if ((!theora_p || videobuf_ready)
#if !defined(WZ_NOSOUND)
	 && (!vorbis_p || audiobuf_ready)
#endif
	 && !stateflag)
	{
		debug(LOG_VIDEO, "all buffers ready");
		stateflag = true;
	}

	/* same if we've run out of input */
	if (PHYSFS_eof(fpInfile))
	{
		stateflag = true;
	}

	return true;
}

void seq_Shutdown()
{
	/* tear it all down */
	debug(LOG_VIDEO, "seq_Shutdown");

	if (!videoplaying)
	{
		debug(LOG_VIDEO, "movie is not playing");
		return;
	}
	glDeleteBuffers(VBO_MINIMAL, buffers);
	glErrors();

#if !defined(WZ_NOSOUND)
	if (vorbis_p)
	{
		ogg_stream_clear(&videodata.vo);
		vorbis_block_clear(&videodata.vb);
		vorbis_dsp_clear(&videodata.vd);
		vorbis_comment_clear(&videodata.vc);
		vorbis_info_clear(&videodata.vi);

		alDeleteSources(1, &audiodata.source);
		alDeleteBuffers(1, &audiodata.buffer1);
		alDeleteBuffers(1, &audiodata.buffer2);

		audio_close();
	}
#endif

	if (theora_p)
	{
		ogg_stream_clear(&videodata.to);
		theora_clear(&videodata.td);
		theora_comment_clear(&videodata.tc);
		theora_info_clear(&videodata.ti);
		deallocateVideoFrame();
	}

	ogg_sync_clear(&videodata.oy);

	if (fpInfile)
	{
		PHYSFS_close(fpInfile);
	}

	videoplaying = false;
	Timer_stop();

#if !defined(WZ_NOSOUND)
	audioTime = 0;
#endif
	sampletimeOffset = last_time = timer_expire = timer_started = 0;
	basetime = -1;
	pie_SetTexturePage(-1);
	debug(LOG_VIDEO, " **** frames = %d dropped = %d ****", frames, dropped);
}

int seq_GetFrameNumber()
{
	return frames;
}

double seq_GetFrameTime()
{
	return videobuf_time;
}

// this controls the size of the video to display on screen
void seq_SetDisplaySize(int sizeX, int sizeY, int posX, int posY)
{
	vertices[0][0] = 0.0f;
	vertices[0][1] = 0.0f;
	vertices[1][0] = sizeX;
	vertices[1][1] = 0.0f;
	vertices[2][0] = 0.0f;
	vertices[2][1] = sizeY;
	vertices[3][0] = sizeX;
	vertices[3][1] = sizeY;

	if (sizeX > 640 || sizeY > 480)
	{
		const float aspect = screenWidth / (float)screenHeight, videoAspect = 4 / (float)3;

		if (aspect > videoAspect) // x offset
		{
			int offset = (screenWidth - screenHeight * videoAspect) / 2;
			vertices[0][0] += offset;
			vertices[3][0] += offset;
			vertices[1][0] -= offset;
			vertices[2][0] -= offset;
		}
		else // y offset
		{
			int offset = (screenHeight - screenWidth / videoAspect) / 2;
			vertices[0][1] += offset;
			vertices[1][1] += offset;
			vertices[2][1] -= offset;
			vertices[3][1] -= offset;
		}
	}

	Scrnvidpos[0] = posX;
	Scrnvidpos[1] = posY;
	Scrnvidpos[2] = 0.0f;
}

void seq_setScanlineMode(SCANLINE_MODE mode)
{
    use_scanlines = mode;
}

SCANLINE_MODE seq_getScanlineMode(void)
{
    return use_scanlines;
}
