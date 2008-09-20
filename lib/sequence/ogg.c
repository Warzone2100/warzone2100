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

// this source uses parts from www.theora.org sample playback code, and 
// freespace2 source code project http://scp.indiegames.us/
//#include <SDL/SDL.h>

#include <theora/theora.h>
#include <vorbis/codec.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "lib/ivis_opengl/GLee.h"
#include "lib/framework/frame.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/framework/frameint.h"
#include "lib/ivis_common/piestate.h"
#include "lib/gamelib/gtime.h"
#include "lib/framework/physfs_ext.h"

#include "ogg.h"

#ifndef WIN32
#include <unistd.h>
#endif

#include <time.h>
#include "timer.h"

/// Buffers to hold sound data.
ALuint buffer1 = 0, buffer2 = 0;

/// Sources are points of emitting sound.
ALuint source = 0;
ALint sourcestate = 0;
int totbufstarted = 0;

//FILE *infile;
PHYSFS_file*  fpInfile = NULL;
PHYSFS_sint64 fsize = 0, fsize2 = 0;

static BOOL playing = 0;
//======================================
/* never forget that globals are a one-way ticket to Hell */
/* Ogg and codec state for demux/decode */
ogg_sync_state oy;
ogg_page og;
ogg_stream_state vo;
ogg_stream_state to;
theora_info ti;
theora_comment tc;
theora_state td;
vorbis_info vi;
vorbis_dsp_state vd;
vorbis_block vb;
vorbis_comment vc;

int theora_p = 0;
int vorbis_p = 0;
int stateflag = 0;

/* single frame video buffering */
int videobuf_ready = 0;
ogg_int64_t videobuf_granulepos = -1;
double videobuf_time = 0;

/* single audio fragment audio buffering */
int audiobuf_fill = 0;
int audiobuf_ready = 0;
ogg_int16_t*  audiobuf = NULL;
ogg_int64_t audiobuf_granulepos = 0;	/* time position of last sample */
int audiofd_fragsize;
char*  RGBAframe = NULL;

long audiofd_totalsize = 0;
double audioTime = 0;
double sampletimeOffset = 0;
double basetime = -1;
double last_time;
double timer_expire;
bool timer_started = 0;
static GLuint backDropTexture2 = ~0;

int frames = 0;
int dropped = 0;
int OGGCurrentFrame = 0;
int ScrnvidXsize = 0;
int ScrnvidYsize = 0;
int ScrnvidXpos = 0;
int ScrnvidYpos = 0;
//======================================
extern char datadir[PATH_MAX];
//prototypes
int OGG_buffer_data ( PHYSFS_file *in, ogg_sync_state *oy );
static void open_audio ( void );
double getTimeNow ( void );
double getRelativeTime ( void );
void seq_InitOgg ( void );
static void audio_write ( void );
void OGG_timer_do_wait ( void );
void OGG_SetFrameCounter( int frame);
//**********************************************************************************
// Helper; just grab some more compressed bitstream and sync it for page extraction
int OGG_buffer_data( PHYSFS_file *in, ogg_sync_state *oy )
{
	// read in 4K chunks
	char *buffer = ogg_sync_buffer( oy, 4096 );
	int bytes = PHYSFS_read( in, buffer, 1, 4096 );

	ogg_sync_wrote( oy, bytes );
	return( bytes );
}
//**********************************************************************************
/** helper: push a page into the appropriate steam
	this can be done blindly; a stream won't accept a page
	that doesn't belong to it
*/
static int OGG_queue_page( ogg_page *page )
{
	if( theora_p )
	{
		ogg_stream_pagein( &to, page );
	}

	if( vorbis_p )
	{
		ogg_stream_pagein( &vo, page );
	}

	return 0;
}
//**********************************************************************************
// sets the OGGCurrentFrame number we are on
void OGG_SetFrameCounter( int frame)
{
	OGGCurrentFrame = frame;
}
//**********************************************************************************
// Just telling the world what frame # we are on.
int OGG_GetFrameCounter( void )
{
	return OGGCurrentFrame;
}
void OGG_SetSize( int sizeX, int sizeY, int posX, int posY)
{
	ScrnvidXsize = sizeX;
	ScrnvidYsize = sizeY;
	ScrnvidXpos = posX;
	ScrnvidYpos = posY;
}
//**********************************************************************************
static void open_audio( void )
{
	audiofd_fragsize = ( ((vi.channels * 16) / 8) * vi.rate );
	audiobuf = malloc( audiofd_fragsize );

	// openal
	alGenSources( 1, &source );

	// Create an OpenAL buffer and fill it with the decoded data
	alGenBuffers( 1, &buffer1 );
	alGenBuffers( 1, &buffer2 );

	// Clear Error Codes
	alGetError();
	totbufstarted = 0;
}
//**********************************************************************************
static void audio_close( void )
{
	audiobuf_fill = 0;
	audiobuf_granulepos = 0;
	audiofd_fragsize = 0;
	source = 0;
	buffer1 = buffer2 = 0;
	if( audiobuf )
	{
		free( audiobuf );
	}
}
//**********************************************************************************
// Retrieves the current time with millisecond accuracy
double getTimeNow( void )
{
	return Timer_getElapsedMilliSecs();
}
//**********************************************************************************
// get relative time since beginning playback, compensating for A/V drift 
double getRelativeTime( void )
{
	if( basetime == -1 )					// check to see if this is first time run
	{
		basetime = getTimeNow();
		timer_expire = Timer_getElapsedMicroSecs();
		timer_expire += ( int ) ( (videobuf_time - getTimeNow()) * 1000000.0 );
		timer_started = true;
	}
	return( (getTimeNow() - basetime) * .001 );
}
//**********************************************************************************
// sync the time if we must.
void OGG_timer_do_wait( void )
{
	int tv, ts, ts2;

	if( !timer_started )
	{
		getRelativeTime();
	}

	tv = Timer_getElapsedMicroSecs();

	if( tv <= timer_expire )
	{
		ts = timer_expire - tv;
		ts2 = ts / 1000;
		printf( "sleeping for %d\n", ts2 );
#ifdef WIN32
		Sleep( ts2 );
#else
		sleep( ts2 );
#endif
	}

	timer_expire +=  ( (videobuf_time - getRelativeTime()) * 1000000.0 );
}
//**********************************************************************************
static void OGG_Allocate_videoFrame( void )
{
	RGBAframe = malloc( ti.frame_width * ti.frame_height * 4 );
}

#define Vclip( x )	( (x > 0) ? ((x < 255) ? x : 255) : 0 )
//**********************************************************************************
static void video_write( BOOL update )
{
	int x = 0, y = 0;
	yuv_buffer yuv;

	if( update )
	{
		theora_decode_YUVout( &td, &yuv );

		// fill the RGBA buffer
		for( y = 0; y < ti.frame_height; y++ )
		{
			for( x = 0; x < ti.frame_width; x++ )
			{
				int Y = yuv.y[x + y * yuv.y_stride];
				int U = yuv.u[x / 2 + ( y / 2 ) * yuv.uv_stride];
				int V = yuv.v[x / 2 + ( y / 2 ) * yuv.uv_stride];

				int C = Y - 16;
				int D = U - 128;
				int E = V - 128;

				int R = Vclip( (298 * C + 409 * E + 128) >> 8 );
				int G = Vclip( (298 * C - 100 * D - 208 * E + 128) >> 8 );
				int B = Vclip( (298 * C + 516 * D + 128) >> 8 );

				RGBAframe[x * 4 + y * ti.frame_width * 4 + 0] = R;
				RGBAframe[x * 4 + y * ti.frame_width * 4 + 1] = G;
				RGBAframe[x * 4 + y * ti.frame_width * 4 + 2] = B;
				RGBAframe[x * 4 + y * ti.frame_width * 4 + 3] = 0xFF;
			}
		}

		if( backDropTexture2 != ~0 )
		{
			glDeleteTextures( 1, &backDropTexture2 );
		}
// NOTE: should we have 2 paths, 1 for cards that support NPOT textures or not?
/*
		glGenTextures( 1, &backDropTexture );

		glBindTexture( GL_TEXTURE_2D, backDropTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, ti.frame_width, ti.frame_height, 0, GL_RGBA,
					  GL_UNSIGNED_BYTE, RGBAframe );

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
*/
// fallback routine for cards that don't support NPOT textures [FORCE TESTING--for now]
	glGenTextures( 1, &backDropTexture2);
	glEnable( GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, backDropTexture2);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, ti.frame_width, ti.frame_height, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, RGBAframe );

	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameterf( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
	glDisable( GL_TEXTURE_RECTANGLE_ARB );

	}

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );

	// Make sure the current texture page is reloaded after we are finished
	// Otherwise WZ will think it is still loaded and not load it again
	pie_SetTexturePage( -1 );

// NOTE: should we have 2 paths, 1 for cards that support NPOT textures or not?
/*
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, backDropTexture );
	glColor3f( 1, 1, 1 );

	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2f( 0, 0 );
	glVertex2f( 0, 0 );
	glTexCoord2f( 255, 0 );
	glVertex2f( screenWidth, 0 );
	glTexCoord2f( 0, 255 );
	glVertex2f( 0, screenHeight );
	glTexCoord2f( 255, 255 );
	glVertex2f( screenWidth, screenHeight );
	glEnd();
*/
// fallback routine for cards that don't support NPOT textures  [FORCE TESTING--for now]
	if(!GLEE_ARB_texture_rectangle)
	{
		debug(LOG_ERROR, "You got some really crappy hardware! GL_TEXTURE_RECTANGLE_ARB not supported! :P");
		debug(LOG_ERROR, "Video will not show!");
	}
	glPushMatrix();
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, backDropTexture2);

	// NOTE: 255 * width | height, because texture matrix is set up with a
	// call to glScalef(1/256.0, 1/256.0, 1) ... so don't blame me. :P
	glTranslatef(ScrnvidXpos,ScrnvidYpos,0.0f);
	glBegin( GL_TRIANGLE_STRIP );
	glTexCoord2f( 0, 0 );
	glVertex2f( 0, 0 );
	glTexCoord2f( 255* ti.frame_width, 0 );
	glVertex2f( ScrnvidXsize, 0 );//screenWidth
	glTexCoord2f( 0, 255* ti.frame_height );
	glVertex2f( 0, ScrnvidYsize );//screenHeight
	glTexCoord2f( 255*ti.frame_width, 255* ti.frame_height );
	glVertex2f( ScrnvidXsize, ScrnvidYsize);//screenWidth,screenHeight
	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glPopMatrix();
}
//**********************************************************************************
// dump the theora (or vorbis) comment header 
static void dump_comments( theora_comment *tc )
{

	int i = 0, len = 0;
	char *value = NULL;

	debug( LOG_VIDEO, "Encoded by %s", tc->vendor );
	if( tc->comments )
	{
		debug( LOG_VIDEO, "theora comment header:" );
		for( i = 0; i < tc->comments; i++ )
		{
			if( tc->user_comments[i] )
			{
				len = tc->comment_lengths[i];
				value = malloc( len + 1 );
				memcpy( value, tc->user_comments[i], len );
				value[len] = '\0';
				debug( LOG_VIDEO, "\t%s", value );
				free( value );
			}
		}
	}
}
//**********************************************************************************
/** Report the encoder-specified colorspace for the video, if any.
   We don't actually make use of the information in this example;
   a real player should attempt to perform color correction for
   whatever display device it supports. */
static void report_colorspace( theora_info *ti )
{
	switch( ti->colorspace ) {
		case OC_CS_UNSPECIFIED:
			/* nothing to report */
			break;
		case OC_CS_ITU_REC_470M:
			debug( LOG_VIDEO, "  encoder specified ITU Rec 470M (NTSC) color.\n" );
			break;
		case OC_CS_ITU_REC_470BG:
			debug( LOG_VIDEO, "  encoder specified ITU Rec 470BG (PAL) color.\n" );
			break;
		default:
			debug( LOG_WARNING, "encoder specified unknown colorspace (%d).\n", ti->colorspace );
			break;
	}
}
//**********************************************************************************
// FIXME: perhaps we should use wz's routine for audio?
// loads up the audio buffers, and calculates audio sync time.
static void audio_write( void )
{
	ALint processed;
	ALint queued;

	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	alGetSourcei( source, AL_BUFFERS_QUEUED, &queued );
	if( (totbufstarted < 2 || processed) && audiobuf_fill > 0 )
	{
		// we have audiobuf_fill bytes of data
		ALuint oldbuffer;

		if( totbufstarted == 0 )
		{
			oldbuffer = buffer1;
		}
		else
		if( totbufstarted == 1 )
		{
			oldbuffer = buffer2;
		}
		else
		{
			ALint buffer_size;
			ogg_int64_t current_sample;

			alSourceUnqueueBuffers( source, 1, &oldbuffer );
			alGetBufferi( oldbuffer, AL_SIZE, &buffer_size );
			// audio time sync
			audioTime += ( double ) buffer_size / ( vi.rate * vi.channels );
			debug(LOG_VIDEO, "Audio sync");
			current_sample = audiobuf_granulepos - audiobuf_fill / 2 / vi.channels;
			sampletimeOffset -= getTimeNow() - 1000 * current_sample / vi.rate;
		}

		alBufferData( oldbuffer, (vi.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16),
					  audiobuf, audiobuf_fill, vi.rate );

		alSourceQueueBuffers( source, 1, &oldbuffer );
		totbufstarted++;
		if( totbufstarted > 2 )
		{
			totbufstarted = 2;
		}

		if( sourcestate != AL_PLAYING )
		{
			debug( LOG_VIDEO, "seq_UpdateOgg: starting source\n" );
			alSourcePlay( source );
		}

		audiobuf_ready = 0;
		audiobuf_fill = 0;
	}
}
//**********************************************************************************
// rint hack, since windows don't have rint!
double rint( double x )
{
	int a = 0;

	if( ceil( x + 0.5 ) == floor( x + 0.5 ) )
	{
		a = ( int ) ceil( x );
		if( a % 2 == 0 )
		{
			return ceil( x );
		}
		else
		{
			return floor( x );
		}
	}
	else
	{
		return floor( x + 0.5 );
	}
}
//**********************************************************************************
/**
 * Display the next frame and play the sound.
 * \return FALSE if the end of the video is reached.
 */
int seq_UpdateOgg( void )
{
	ogg_packet op;
	int i, j;
	int ret;
	float **  pcm;
	int count, maxsamples;

	/* we want a video and audio frame ready to go at all times.  If
		   we have to buffer incoming, buffer the compressed data (ie, let
		   ogg do the buffering) */
	if( !playing )
	{
		debug( LOG_WARNING, "seq_UpdateOgg: no movie playing" );
		return false;
	}

	while( vorbis_p && !audiobuf_ready )
	{
		/* if there's pending, decoded audio, grab it */
		if( (ret = vorbis_synthesis_pcmout( &vd, &pcm )) > 0 )
		{
			// we now have float pcm data in pcm
			// going to convert that to int pcm in audiobuf
			count = audiobuf_fill / 2;
			maxsamples = ( audiofd_fragsize - audiobuf_fill ) / 2 / vi.channels;

			for( i = 0; i < ret && i < maxsamples; i++ )
			{
				for( j = 0; j < vi.channels; j++ )
				{
					int val = rint( pcm[j][i] * 32767.f );

					if( val > 32767 )
					{
						val = 32767;
					}
					else if( val < -32768 )
					{
						val = -32768;
					}
					audiobuf[count++] = val;
				}
			}

			vorbis_synthesis_read( &vd, i );
			audiobuf_fill += i * vi.channels * 2;

			if( audiobuf_fill == audiofd_fragsize )
			{
				audiobuf_ready = true;
			}

			if( vd.granulepos >= 0 )
			{
				audiobuf_granulepos = vd.granulepos - ret + i;
			}
			else
			{
				audiobuf_granulepos += i;
			}
		}
		else
		{
			/* no pending audio; is there a pending packet to decode? */
			if( ogg_stream_packetout( &vo, &op ) > 0 )
			{
				if( vorbis_synthesis( &vb, &op ) == 0 )
				{	/* test for success! */
					vorbis_synthesis_blockin( &vd, &vb );
				}
			}
			else
			{	/* we need more data; break out to suck in another page */
				break;
			}
		}
	}

	while( theora_p && !videobuf_ready )
	{
		/* theora is one in, one out... */
		if( ogg_stream_packetout( &to, &op ) > 0 )
		{
			double now_time = 0;
			double delay = 0;

			theora_decode_packetin( &td, &op );
			videobuf_granulepos = td.granulepos;
			videobuf_time = theora_granule_time( &td, videobuf_granulepos );

			now_time = getRelativeTime();
			delay = videobuf_time - getRelativeTime();

			if( (delay >= 0.0f) || (now_time - last_time >= 1.0f) )
			{
				videobuf_ready = true;
				OGG_SetFrameCounter(frames++);
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

	alGetSourcei( source, AL_SOURCE_STATE, &sourcestate );

	if( !videobuf_ready && !audiobuf_ready && PHYSFS_eof( fpInfile ) && sourcestate != AL_PLAYING &&
		audiobuf_fill == 0 )
	{
		video_write( false );
		seq_ShutdownOgg();
		debug( LOG_VIDEO, "seq_UpdateOgg: video finished" );
		return false;
	}

	if( !videobuf_ready || !audiobuf_ready )
	{
		/* no data yet for somebody.  Grab another page */
		ret = OGG_buffer_data( fpInfile, &oy );
		while( ogg_sync_pageout( &oy, &og ) > 0 )
		{
			OGG_queue_page( &og );
		}
	}

	/* If playback has begun, top audio buffer off immediately. */
	if( vorbis_p && stateflag )
	{
		//debug(LOG_VIDEO, "seq_UpdateOgg: starting playback\n");
		// play the data in pcm
		audio_write();
	}

	/* are we at or past time for this video frame? */
	if( stateflag && videobuf_ready && (videobuf_time <= getRelativeTime()) )
	{
		video_write( true );
		last_time = getRelativeTime();
		videobuf_ready = false;
	}
	else
	if( stateflag )
	{
		video_write( false );
	}

	/* if our buffers either don't exist or are ready to go,
		   we can begin playback */
	if( (!theora_p || videobuf_ready) && (!vorbis_p || audiobuf_ready) && !stateflag )
	{
		debug( LOG_VIDEO, "seq_UpdateOgg: all buffers ready" );
		stateflag = true;
	}

	//debug(LOG_VIDEO, "theora_p: %i, videobuf_ready: %i, vorbis_p: %i, audiobuf_ready: %i", theora_p, videobuf_ready, vorbis_p, audiobuf_ready);
	/* same if we've run out of input */
	if( PHYSFS_eof( fpInfile ) )
	{
		stateflag = true;
	}

	if( videobuf_ready )
	{
		OGG_timer_do_wait();
	}

	return true;
}
//**********************************************************************************
void seq_InitOgg( void )
{
	debug( LOG_VIDEO, "seq_InitOgg" );

	ASSERT((ScrnvidXsize && ScrnvidYsize ),"Screen dimensions not specified!");

	stateflag = 0;
	theora_p = 0;
	vorbis_p = 0;
	playing = false;

	/* single frame video buffering */
	videobuf_ready = 0;
	videobuf_granulepos = -1;
	videobuf_time = 0;
	OGGCurrentFrame = 0;
	/* single audio fragment audio buffering */
	audiobuf_fill = 0;
	audiobuf_ready = 0;

	audiobuf_granulepos = 0;	/* time position of last sample */

	audioTime = 0;
	sampletimeOffset = 0;

	/* start up Ogg stream synchronization layer */
	ogg_sync_init( &oy );

	/* init supporting Vorbis structures needed in header parsing */
	vorbis_info_init( &vi );
	vorbis_comment_init( &vc );

	/* init supporting Theora structuretotbufstarteds needed in header parsing */
	theora_comment_init( &tc );
	theora_info_init( &ti );
	Timer_Init();
	Timer_start();
}
//**********************************************************************************
BOOL seq_PlayOgg( char *filename)
{
	char kludge[512];
	int len = 0;
	int pp_level_max = 0;
	int pp_level = 0;
	int pp_inc = 0;
	ogg_packet op;

	debug( LOG_VIDEO, "seq_PlayOgg(\"%s\")", filename );

	if( playing )
	{
		debug( LOG_WARNING, "seq_PlayOgg: previous movie is not yet finished" );
		seq_ShutdownOgg();
	}

	seq_InitOgg();
	strcpy( kludge, filename );
	len = strlen( filename );
	kludge[len - 3] = 'o';
	kludge[len - 2] = 'g';
	kludge[len - 1] = 'g';
	fpInfile = PHYSFS_openRead( kludge );
	if( fpInfile == NULL )
	{
		debug( LOG_ERROR, "seq_PlayOgg: unable to open '%s' for playback [%s]", kludge, datadir );
		return false;
	}

	theora_p = 0;
	vorbis_p = 0;

	/* Ogg file open; parse the headers */
	/* Only interested in Vorbis/Theora streams */
	while( !stateflag )
	{
		int ret = OGG_buffer_data( fpInfile, &oy );

		if( ret == 0 )
		{
			break;
		}

		while( ogg_sync_pageout( &oy, &og ) > 0 )
		{
			ogg_stream_state test;

			/* is this a mandated initial header? If not, stop parsing */
			if( !ogg_page_bos( &og ) )
			{
				/* don't leak the page; get it into the appropriate stream */
				OGG_queue_page( &og );
				stateflag = 1;
				break;
			}

			ogg_stream_init( &test, ogg_page_serialno( &og ) );
			ogg_stream_pagein( &test, &og );
			ogg_stream_packetout( &test, &op );

			/* identify the codec: try theora */
			if( !theora_p && theora_decode_header( &ti, &tc, &op ) >= 0 )
			{
				/* it is theora */
				memcpy( &to, &test, sizeof(test) );
				theora_p = 1;
			}
			else
			if( !vorbis_p && vorbis_synthesis_headerin( &vi, &vc, &op ) >= 0 )
			{
				/* it is vorbis */
				memcpy( &vo, &test, sizeof(test) );
				vorbis_p = 1;
			}
			else
			{
				/* whatever it is, we don't care about it */
				ogg_stream_clear( &test );
			}
		}
		/* fall through to non-bos page parsing */
	}

	/* we're expecting more header packets. */
	while( (theora_p && theora_p < 3) || (vorbis_p && vorbis_p < 3) )
	{
		int ret;

		/* look for further theora headers */
		while( theora_p && (theora_p < 3) && (ret = ogg_stream_packetout( &to, &op )) )
		{
			if( ret < 0 )
			{
				debug( LOG_ERROR, "Error parsing Theora stream headers; corrupt stream?\n" );
				return false;
			}

			if( theora_decode_header( &ti, &tc, &op ) )
			{
				debug( LOG_ERROR, "Error parsing Theora stream headers; corrupt stream?\n" );
				return false;
			}

			theora_p++;

			//if (theora_p == 3)
			//{
			//	break;
			//}
		}

		/* look for more vorbis header packets */
		while( vorbis_p && (vorbis_p < 3) && (ret = ogg_stream_packetout( &vo, &op )) )
		{
			if( ret < 0 )
			{
				debug( LOG_ERROR, "Error parsing Vorbis stream headers; corrupt stream?\n" );
				return false;
			}

			if( vorbis_synthesis_headerin( &vi, &vc, &op ) )
			{
				debug( LOG_ERROR, "Error parsing Vorbis stream headers; corrupt stream?\n" );
				return false;
			}

			vorbis_p++;

			//if (vorbis_p == 3)
			//{
			//	break;
			//}
		}

		/* The header pages/packets will arrive before anything else we
				care about, or the stream is not obeying spec */
		if( ogg_sync_pageout( &oy, &og ) > 0 )
		{
			OGG_queue_page( &og );	/* demux into the appropriate stream */
		}
		else
		{
			int ret = OGG_buffer_data( fpInfile, &oy ); /* someone needs more data */

			if( ret == 0 )
			{
				debug( LOG_ERROR, "End of file while searching for codec headers.\n" );
				return false;
			}
		}
	}

	/* and now we have it all.  initialize decoders */
	if( theora_p )
	{
		theora_decode_init( &td, &ti );
		debug( LOG_VIDEO, "Ogg logical stream %x is Theora %dx%d %.02f fps video",
			   ( unsigned int ) to.serialno, ( int ) ti.width, ( int ) ti.height,
			   ( double ) ti.fps_numerator / ti.fps_denominator );
		if( ti.width != ti.frame_width || ti.height != ti.frame_height )
		{
			debug( LOG_VIDEO, "  Frame content is %dx%d with offset (%d,%d)", ti.frame_width,
				   ti.frame_height, ti.offset_x, ti.offset_y );
		}

		report_colorspace( &ti );
		dump_comments( &tc );

		// hmm
		theora_control( &td, TH_DECCTL_GET_PPLEVEL_MAX, &pp_level_max, sizeof(pp_level_max) );
		pp_level = pp_level_max;
		theora_control( &td, TH_DECCTL_SET_PPLEVEL, &pp_level, sizeof(pp_level) );
		pp_inc = 0;
	}
	else
	{
		/* tear down the partial theora setup */
		theora_info_clear( &ti );
		theora_comment_clear( &tc );
	}

	if( vorbis_p )
	{
		vorbis_synthesis_init( &vd, &vi );
		vorbis_block_init( &vd, &vb );
		debug( LOG_VIDEO, "Ogg logical stream %x is Vorbis %d channel %d Hz audio",
			   ( unsigned int ) vo.serialno, vi.channels, ( int ) vi.rate );
	}
	else
	{
		/* tear down the partial vorbis setup */
		vorbis_info_clear( &vi );
		vorbis_comment_clear( &vc );
	}

	/* open audio */
	if( vorbis_p )
	{
		open_audio();
	}

	/* open video */
	if( theora_p )
	{
		OGG_Allocate_videoFrame();
	}

	/* on to the main decode loop.  We assume in this example that audio
		and video start roughly together, and don't begin playback until
		we have a start frame for both.  This is not necessarily a valid
		assumption in Ogg A/V streams! It will always be true of the
		example_encoder (and most streams) though. */
	sampletimeOffset = getTimeNow();
	playing = true;
	return true;
}
//**********************************************************************************
void seq_ShutdownOgg( void )
{
	/* tear it all down */
	debug( LOG_VIDEO, "seq_ShutdownOgg" );

	if( !playing )
	{
		debug( LOG_WARNING, "seq_ShutdownOgg: movie is not playing" );
		return;
	}

	if( vorbis_p )
	{
		ogg_stream_clear( &vo );
		vorbis_block_clear( &vb );
		vorbis_dsp_clear( &vd );
		vorbis_comment_clear( &vc );
		vorbis_info_clear( &vi );

		alDeleteSources( 1, &source );
		alDeleteBuffers( 1, &buffer1 );
		alDeleteBuffers( 1, &buffer2 );

		audio_close();
	}

	if( theora_p )
	{
		ogg_stream_clear( &to );
		theora_clear( &td );
		theora_comment_clear( &tc );
		theora_info_clear( &ti );

	}

	ogg_sync_clear( &oy );

	if( fpInfile )
	{
		PHYSFS_close( fpInfile );
	}

	if( RGBAframe)
	{
		free(RGBAframe);
	}
	playing = false;
	Timer_stop();

	audioTime = sampletimeOffset = last_time = timer_expire = timer_started = 0;
	basetime = -1;
	pie_SetTexturePage(-1);
	debug( LOG_VIDEO, " **** frames = %d dropped = %d ****", frames, dropped );
}
