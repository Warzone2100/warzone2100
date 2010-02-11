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
/*****************************************************************************
*
* Copyright	(c)	Eidos Plc. 1997
*
* Contents:
*
*****************************************************************************
*
*
*
*****************************************************************************/
#ifndef	__STREAMER_H_INCLUDED__
#define	__STREAMER_H_INCLUDED__

#include <stddef.h>

#ifndef WIN32

#ifndef TYPEDEFS
#define TYPEDEFS

#ifndef FALSE
	#define FALSE 0
#endif

#ifndef TRUE
	#define TRUE 1
#endif

#define FAR
#define HUGE

typedef void					VOID;
typedef void *					LPVOID;

typedef unsigned int 			BOOL;

typedef signed char				CHAR;
typedef signed char	*			LPCHAR;
typedef signed char	*			LPCSTR;

typedef char					BYTE;
typedef char *					LPBYTE;

typedef unsigned char *			LPSTR;
typedef unsigned char  			UBYTE;
typedef unsigned char *			LPUBYTE;

typedef float					FLOAT;

typedef unsigned int			UINT;
typedef unsigned int *			LPUINT;

typedef signed int				INT;
typedef signed int *			LPINT;

typedef signed short			WORD;
typedef signed short *			LPWORD;

typedef signed long				LONG;
typedef signed long *			LPLONG;

typedef unsigned long			ULONG;
typedef unsigned long			DWORD;

typedef unsigned long *			LPULONG;
typedef unsigned short 			UWORD;

#endif

#endif

struct _VIDEOHANDLE;
struct _ALPHAHANDLE;
struct _MOVIEHANDLE;
struct _SOUNDHANDLE;

typedef struct _ALPHAHANDLE FAR *LPALPHAHANDLE;
typedef	struct _VIDEOHANDLE FAR *LPVIDEOHANDLE;
typedef	struct _SOUNDHANDLE FAR *LPSOUNDHANDLE;
typedef	struct _MOVIEHANDLE FAR *LPMOVIEHANDLE;

typedef	enum
{
	ESCAPEUNKNOWN,
	ESCAPE124=124,
	ESCAPE130=130

} CODECTYPE;

#define DFLAG_INVIEWPORT				1
#define DFLAG_OUTVIEWPORT				0
#define DFLAG_BANKEDVIDEO				2
#define DFLAG_LINEARVIDEO				0
#define DFLAG_CLEAROUTPUT				4
#define DFLAG_LEAVEOUTPUT				0
#define DFLAG_DOUBLED					8
#define DFLAG_NOTDOUBLED				0
#define DFLAG_DOUBLELUTS				16
#define DFLAG_EFFECTS					32
#define DFLAG_SOFTDOUBLED				64
#define DFLAG_INTERPOLATED				128

#define SIM_NONE						0
#define SIM_DATASTREAM					1

#define ALPHA_NONE						0
#define ALPHA_FROMALPHAIMAGE          (2+64)
#define ALPHA_NOT                     (3+64)
#define ALPHA_HALF                    (4+64)
#define ALPHA_GREY                    (128+64)
#define ALPHA_GREY_RED                (128+64+1)
#define ALPHA_GREY_GREEN              (128+64+2)
#define ALPHA_GREY_BLUE               (128+64+3)
#define ALPHA_GREY_REDGREEN           (128+64+4)
#define ALPHA_GREY_REDBLUE            (128+64+5)
#define ALPHA_GREY_GREENBLUE          (128+64+6)
#define ALPHA_GREY_HALF               (128+64+7)
#define ALPHA_GREY_NOT                (128+64+8)
#define ALPHA_GREY_RED_NOT            (128+64+9)
#define ALPHA_GREY_GREEN_NOT          (128+64+10)
#define ALPHA_GREY_BLUE_NOT           (128+64+11)
#define ALPHA_GREY_REDGREEN_NOT       (128+64+12)
#define ALPHA_GREY_REDBLUE_NOT        (128+64+13)
#define ALPHA_GREY_GREENBLUE_NOT      (128+64+14)
#define ALPHA_HOTANDCOLD			  (128+64+15)
#define ALPHA_EMBOSS                  (128+0)
#define ALPHA_AVERAGE                 (128+1)
#define ALPHA_REPLACE                 (128+2)
#define ALPHA_MIX                     (128+3)
#define ALPHA_MOSAIC                  (128+4)
#define ALPHA_MIX_PHASED			  (128+5)
#define RPL_VESA_MODE 0x110

#define DOS 1

#define DEFAULT_COLOUR_NONE                  0
#define DEFAULT_COLOUR_NOT                   1
#define DEFAULT_COLOUR_HALF                  2
#define DEFAULT_COLOUR_GREY                  3
#define DEFAULT_COLOUR_GREY_RED              4
#define DEFAULT_COLOUR_GREY_GREEN            5
#define DEFAULT_COLOUR_GREY_BLUE             6
#define DEFAULT_COLOUR_GREY_REDGREEN         7
#define DEFAULT_COLOUR_GREY_REDBLUE          8
#define DEFAULT_COLOUR_GREY_GREENBLUE        9
#define DEFAULT_COLOUR_GREY_HALF             10
#define DEFAULT_COLOUR_GREY_NOT              11
#define DEFAULT_COLOUR_GREY_RED_NOT          12
#define DEFAULT_COLOUR_GREY_GREEN_NOT        13
#define DEFAULT_COLOUR_GREY_BLUE_NOT         14
#define DEFAULT_COLOUR_GREY_REDGREEN_NOT     15
#define DEFAULT_COLOUR_GREY_REDBLUE_NOT      16
#define DEFAULT_COLOUR_GREY_GREENBLUE_NOT    17

#define SPF_BPP1		0x0001
#define SPF_BPP2		0x0002
#define SPF_BPP4		0x0004
#define SPF_BPP8		0x0008
#define SPF_BPP16		0x0010
#define SPF_BPP32		0x0020
#define SPF_BPPMASK		0x003F
#define SPF_PALETTISED  0x0040
#define SPF_COLOURKEYED 0x0080
#define SPF_TRANS       0x0100
#define SPF_ALPHA		0x0200

#define SSDM_IDLE										-1
#define SSDM_FIRSTBUFFER								0
#define SSDM_SECONDBUFFER								1

#define	STREAMER_TIMERFACTOR							1
#define	STREAMER_READSIZE								(1024<<3)
#define	STREAMER_ACCURACY								100
#define	STREAMER_FRAMERATE								0
#define	STREAMER_INTERRUPTFREQ							1000
#define	STREAMER_DOUBLEBUFFERSIZE						(1024<<4)
#define	STREAMER_BUFFERSIZE								(2<<20)
#define	STREAMER_TRANSFERFACTOR							4
#define	STREAMER_SOUND									FALSE
#define	STREAMER_TRANSFERSIZE							1024
#define	STREAMER_PREKEYPRESS							FALSE
#define	STREAMER_PREMESSAGE								FALSE
#define	STREAMER_VDM									FALSE
#define	STREAMER_PLAYKEYPRESS							FALSE
#define	STREAMER_PCM									FALSE
#define	STREAMER_PREMESSGAE								FALSE

#define	STREAMER_OK										0
#define	STREAMER_NO_SOUND_DRIVER						1
#define	STREAMER_NO_SOUND_IN_RPL						2
#define	STREAMER_BAD_ALLOC_SOUND_HANDLE					3
#define	STREAMER_BAD_ALLOC_DBUFFERS						4
#define	STREAMER_BAD_ALLOC_SAMPLE_HANDLE				5
#define	STREAMER_BAD_ALLOC_TIMER						6
#define	STREAMER_BAD_ALLOC_TIMER_HANDLE					7
#define	STREAMER_BAD_ALLOC_VIDEO_BUFFER					8
#define	STREAMER_BAD_ALLOC_GLCW_LUT						9
#define	STREAMER_BAD_ALLOC_LSCW_LUT						10
#define	STREAMER_BAD_ALLOC_GSCW_LUT						11
#define	STREAMER_BAD_ALLOC_VIDEO_HANDLE					12
#define	STREAMER_VESA_ERROR								13
#define	STREAMER_BAD_ALLOC_SB_OFFSETS					14
#define	STREAMER_BAD_ALLOC_MOVIE_HANDLE					15
#define	STREAMER_BAD_ALLOC_MOVIE_INDEX					16
#define	STREAMER_FILE_OPEN_ERROR						17
#define	STREAMER_BAD_PLAYBACK_STATE						18
#define	STREAMER_FILE_OPEN_ERROR_HIGH					19
#define	STREAMER_BAD_MOVIE_HEADER						20
#define	STREAMER_CHUNK_OUT_FILE_ERROR					21
#define	STREAMER_BAD_ALLOC_OPTIONS						22
#define	STREAMER_BAD_ALIGN_SEEK							23
#define	STREAMER_PRELOAD_KEY_PRESS						24
#define	STREAMER_FILE_READ_ERROR						25
#define	STREAMER_BAD_ALLOC_SCREEN_BUFFER				26
#define	STREAMER_BAD_SOUND_CALLBACK						27
#define STREAMER_BADTHREADCREATE						28
#define STREAMER_BADDATARATE							29
#define	STREAMER_BADMOVIEHANDLE							30
#define STREAMER_NOEFFECTS								31
#define STREAMER_FINISHEDVIDEO							32
#define STREAMER_FINISHEDAUDIO							33
#define STREAMER_BAD_ALLOC_SOUND_BUFFER					34
#define STREAMER_NOTINGRAPHICSMODE						35
#define STREAMER_BAD_MOVIEHANDLE						36
#define STREAMER_BAD_VIDEOHANDLE						37
#define STREAMER_BAD_SOUNDHANDLE						38
#define STREAMER_BADSOUNDBUFFERNO						39
#define STREAMER_NODATADATASTREAM                       40
#define STREAMER_BADMOVIEFORMAT							41
#define STREAMER_130CODECOPENERROR						42

typedef	LONG STRESULT;

#ifdef __cplusplus
extern "C"
{
#endif

LPSTR
Movie_GetAuthor( LPMOVIEHANDLE mhandle );

LONG
Movie_GetColourDepth( LPMOVIEHANDLE	mhandle );

LPSTR
Movie_GetCopyright( LPMOVIEHANDLE mhandle );

LONG
Movie_GetCurrentAudioChunk(	LPMOVIEHANDLE mhandle );

LONG
Movie_GetCurrentFrame( LPMOVIEHANDLE mhandle );

LONG
Movie_GetCurrentVideoChunk(	LPMOVIEHANDLE mhandle );

LONG
Movie_GetFormat( LPMOVIEHANDLE	mhandle );

FLOAT
Movie_GetFrameRate( LPMOVIEHANDLE mhandle);

LONG
Movie_GetFramesInBuffer( LPMOVIEHANDLE mhandle );

LONG
Movie_GetFramesPerChunk(LPMOVIEHANDLE mhandle );

LONG
Movie_GetMovieChunks( LPMOVIEHANDLE	mhandle );

LPSTR
Movie_GetName(	LPMOVIEHANDLE mhandle );

LONG
Movie_GetSoundChannels( LPMOVIEHANDLE mhandle );

LPSTR
Movie_GetSoundFormatString(	LPMOVIEHANDLE mhandle );

LONG
Movie_GetSoundPrecision( LPMOVIEHANDLE	mhandle );

LONG
Movie_GetSoundRate( LPMOVIEHANDLE mhandle);

LONG
Movie_GetSyncAdjust( LPSOUNDHANDLE shandle );

LONG
Movie_GetTotalFrames( LPMOVIEHANDLE	mhandle );

FLOAT
Movie_GetTotalTime( LPMOVIEHANDLE mhandle );

LONG
Movie_GetXSize( LPMOVIEHANDLE mhandle);

LONG
Movie_GetYSize( LPMOVIEHANDLE mhandle);

void
Movie_SetFrameRate( LPMOVIEHANDLE mhandle, FLOAT framerate);

void
Movie_SetSyncAdjust( LPMOVIEHANDLE mhandle, LPSOUNDHANDLE shandle, LONG adjust );





STRESULT
Streamer_AlphaApply(	LPVIDEOHANDLE vhandle,
						LPALPHAHANDLE ahandle,
						LPBYTE buffer);

STRESULT
Streamer_AlphaRemove(	LPVIDEOHANDLE vhandle,
						LPALPHAHANDLE ahandle,
						LPBYTE buffer);

STRESULT
Streamer_BeforeStreaming(	LPMOVIEHANDLE mhandle,
							LPVIDEOHANDLE vhandle);

STRESULT
Streamer_DoPreload( LPMOVIEHANDLE mhandle,
					LPSOUNDHANDLE shandle);

unsigned char *
Streamer_GetSoundBuffer(	LPSOUNDHANDLE	shandle,
							LONG			bufferno );
LONG
Streamer_GetSoundDecodeMode( LPSOUNDHANDLE	shandle );

STRESULT
Streamer_GetVideoPitch( LPVIDEOHANDLE vhandle , LONG *xpitch, LONG *ydepth);

STRESULT
Streamer_InitMovie(	LPMOVIEHANDLE	*handle,
					void			(*ProgressCallback)(),
					LONG			progressDelta,
					LPSTR			pathname,
					ULONG			buffersize,
					ULONG			properties);

STRESULT
		Streamer_InitSound(	void (*SoundBufferCallback)(LPSOUNDHANDLE ),
					LPSOUNDHANDLE *handle,
					UINT dbufsize,
					UINT compressionfactor,
					BOOL pcm,
					UINT transfersize,
					UINT channels);

STRESULT
Streamer_InitStreaming( LPMOVIEHANDLE mhandle,
						LPVIDEOHANDLE vhandle,
						LPSOUNDHANDLE shandle);

STRESULT
Streamer_InitVideo(	LPVIDEOHANDLE	*handle,
					LPMOVIEHANDLE	mhandle,
					UINT			moviexsize,
					UINT			movieysize,
					INT				videoleft,
					INT				videotop,
					INT				viewportleft,
					INT				viewporttop,
					UINT			viewportwidth,
					UINT			viewportheight,
					UINT			properties,
					LPLONG			bufferPixelWidth,
					LPLONG			bufferPixelDepth);

STRESULT
Streamer_MapVideo(	LPVIDEOHANDLE	vhandle,
					LPBYTE			buffer,
					LONG 			defop);

STRESULT
Streamer_NoMalloc_InitMovie(	LPMOVIEHANDLE	*handle,
							void	(*ProgressCallback)(),
							LONG	progressDelta,
							LPSTR	pathname,
							LPBYTE	memoryBuffer,
							ULONG	memoryBufferSize,
							ULONG	properties);

STRESULT
Streamer_NoMalloc_InitSound(	void (*SoundBufferCallback)(LPSOUNDHANDLE ),
							LPSOUNDHANDLE *handle,
							UINT dbufsize,
							UINT compressionfactor,
							BOOL pcm,
							UINT transfersize,
							UINT channels,
							LPBYTE memoryBuffer,
							LONG memoryBufferSize);

STRESULT
Streamer_NoMalloc_InitVideo(	LPVIDEOHANDLE	*handle,
							LPMOVIEHANDLE	mhandle,
							UINT		   	moviexsize,
							UINT		   	movieysize,
							INT				videoleft,
							INT				videotop,
							INT				viewportleft,
							INT				viewporttop,
							UINT		   	viewportwidth,
							UINT		   	viewportheight,
							UINT		   	properties,
							LPLONG			bufferPixelWidth,
							LPLONG			bufferPixelDepth,
							LPBYTE			memoryBuffer,
							LONG			memoryBufferSize);

LONG
Streamer_NoMalloc_MovieBufferRequired( void);

STRESULT
Streamer_NoMalloc_ShutDownMovie( LPMOVIEHANDLE * handle);

STRESULT
Streamer_NoMalloc_ShutDownSound(LPSOUNDHANDLE  *handle);

STRESULT
Streamer_NoMalloc_ShutDownVideo(LPVIDEOHANDLE *handle);

LONG
Streamer_NoMalloc_SoundBufferRequired( );

LONG
Streamer_NoMalloc_VideoBufferRequired(	UINT moviex,
									UINT moviey,
									LONG properties,
									LPLONG	bufferPixelWidth,
									LPLONG	bufferPixelDepth);

STRESULT
Streamer_SetPixelFormat(	LPVIDEOHANDLE	handle,
							WORD			flags,
							BYTE			ap,
							BYTE			ac,
							BYTE			rp,
							BYTE			rc,
							BYTE			gp,
							BYTE			gc,
							BYTE			bp,
							BYTE			bc);

STRESULT
Streamer_SetPreloadAmount(	LPMOVIEHANDLE mhandle,
							ULONG amount);

STRESULT
Streamer_SetVideoMappings(	LPVIDEOHANDLE	handle,
							INT				videoleft,
							INT				videotop,
							INT				viewportleft,
							INT				viewporttop,
							UINT			viewportwidth,
							UINT			viewportheight,
							UINT			properties);

STRESULT
Streamer_SetVideoPitch( LPVIDEOHANDLE vhandle, LONG xpitch, LONG ydepth);

STRESULT
Streamer_ShutDownMovie( LPMOVIEHANDLE * handle);

STRESULT
Streamer_ShutDownSound( LPSOUNDHANDLE *handle);

STRESULT
Streamer_ShutDownVideo( LPVIDEOHANDLE *handle);

STRESULT
Streamer_Stream(	LPMOVIEHANDLE	mhandle,
					LPVIDEOHANDLE	vhandle,
					LPSOUNDHANDLE	shandle,
					LPALPHAHANDLE	ahandle,
					LONG 			   framestoplay,
					LPBYTE 			videoFrameBuffer,
					LPBYTE 			dataFrameBuffer,
					LPBYTE 			ovlframe,
					LONG 			   ovlindex);


STRESULT
Streamer_SetSoundBuffer(	LPSOUNDHANDLE	shandle,
							LONG			bufferno,
							unsigned char *	soundbuffer);

STRESULT
Streamer_SetSoundDecodeMode(	LPSOUNDHANDLE	handle,
								LONG 			mode );

STRESULT
		Streamer_SwitchToGraphicsMode( void );

STRESULT
		Streamer_SwitchToTextMode( void );


STRESULT
Streamer_ResetMovie( LPMOVIEHANDLE	mhandle,
					 LPVIDEOHANDLE	vhandle,
					 LPSOUNDHANDLE	shandle);








LONG
Alpha_MapMemReq(	LPALPHAHANDLE	ahandle );

#if 0
LONG
Movie_GetVolume( LPSOUNDHANDLE shandle);
#endif


LPALPHAHANDLE
Alpha_CreateEffects( LONG Width, LONG Height );

void
Alpha_FreeEffects( LPALPHAHANDLE );

LPBYTE
Alpha_GetBuffer( LPALPHAHANDLE );

LONG
Alpha_GetBufferSize( LPALPHAHANDLE ahandle );

LONG
Alpha_GetHeight( LPALPHAHANDLE );

LONG
Alpha_GetWidth( LPALPHAHANDLE );

LONG
Alpha_GetXPos( LPALPHAHANDLE );

LONG
Alpha_GetYPos( LPALPHAHANDLE );

LONG
Alpha_SetXPos( LPALPHAHANDLE, LONG XPos );

LONG
Alpha_SetYPos( LPALPHAHANDLE, LONG YPos );





#ifdef __cplusplus
}
#endif



#ifdef DEBUG
void
heap_dump(char *caller );
#endif



#ifdef __WATCOMC__

#pragma aux (Streamer_InitSound, __cdecl);
#pragma aux (Streamer_ShutDownSound, __cdecl);
#pragma aux (Streamer_InitVideo, __cdecl);
#pragma aux (Streamer_ShutDownVideo, __cdecl);
#pragma aux (Streamer_InitMovie, __cdecl);
#pragma aux (Streamer_ShutDownMovie, __cdecl);
#pragma aux (Streamer_InitStreaming, __cdecl);
#pragma aux (Streamer_Stream, __cdecl);
#pragma aux (Streamer_AlphaApply, __cdecl);
#pragma aux (Streamer_AlphaRemove, __cdecl);
#pragma aux (Streamer_SetPixelFormat, __cdecl);
#pragma aux (Streamer_SwitchToGraphicsMode, __cdecl);
#pragma aux (Streamer_SwitchToTextMode, __cdecl);
#pragma aux (Streamer_MapVideo, __cdecl);
#pragma aux (Streamer_SetSoundDecodeMode, __cdecl);
#pragma aux (Streamer_ResetMovie, __cdecl);
#pragma aux (Streamer_SetVideoMappings, __cdecl);
#pragma aux (Streamer_GetSoundBuffer, __cdecl);
#pragma aux (Streamer_SetSoundBuffer, __cdecl);

#pragma aux (Alpha_CreateEffects, __cdecl);
#pragma aux (Alpha_FreeEffects, __cdecl);
#pragma aux (Alpha_GetXPos, __cdecl);
#pragma aux (Alpha_GetYPos, __cdecl);
#pragma aux (Alpha_SetXPos, __cdecl);
#pragma aux (Alpha_SetYPos, __cdecl);
#pragma aux (Alpha_GetWidth, __cdecl);
#pragma aux (Alpha_GetHeight, __cdecl);
#pragma aux (Alpha_GetBuffer, __cdecl);
#pragma aux (Alpha_GetBufferSize, __cdecl);
#pragma aux (Alpha_MapMemReq, __cdecl);

#pragma aux (Movie_GetName, __cdecl);
#pragma aux (Movie_GetSoundFormatString, __cdecl);
#pragma aux (Movie_GetCopyright, __cdecl);
#pragma aux (Movie_GetAuthor, __cdecl);
#pragma aux (Movie_GetFormat, __cdecl);
#pragma aux (Movie_GetColourDepth, __cdecl);
#pragma aux (Movie_GetXSize, __cdecl);
#pragma aux (Movie_GetYSize, __cdecl);
#pragma aux (Movie_GetSoundRate, __cdecl);
#pragma aux (Movie_GetSoundPrecision, __cdecl);
#pragma aux (Movie_GetSoundChannels, __cdecl);
#pragma aux (Movie_GetMovieChunks, __cdecl);
#pragma aux (Movie_GetFramesPerChunk, __cdecl);
#pragma aux (Movie_GetFrameRate, __cdecl);
#pragma aux (Movie_SetFrameRate, __cdecl);
#pragma aux (Movie_GetTotalFrames, __cdecl);
#pragma aux (Movie_GetCurrentFrame, __cdecl);
#pragma aux (Movie_GetFramesInBuffer, __cdecl);
#pragma aux (Movie_GetCurrentVideoChunk, __cdecl);
#pragma aux (Movie_GetCurrentAudioChunk, __cdecl);
#pragma aux (Movie_SetSyncAdjust, __cdecl);
#pragma aux (Movie_GetSyncAdjust, __cdecl);
#pragma aux (Movie_GetTotalTime, __cdecl);
#pragma aux (Movie_GetVolume, __cdecl);
#pragma aux ( Streamer_GetVideoPitch, __cdecl);
#pragma aux ( Streamer_SetVideoPitch, __cdecl);

#ifndef WIN32
#ifdef DEBUG
	#pragma aux (heap_dump, __cdecl);
#endif // DEBUG
#endif // WIN32
#endif
#endif
