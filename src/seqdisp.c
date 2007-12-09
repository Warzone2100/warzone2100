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
/*
 * SeqDisp.c		(Sequence Display)
 *
 * Functions for the display of the Escape Sequences
 *
 */
#include <string.h>
#include <SDL_timer.h>
#include <physfs.h>

#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
#include "lib/ivis_common/rendmode.h"
#include "seqdisp.h"
#include "lib/sequence/sequence.h"
#include "loop.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "hci.h"//for font
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "deliverance.h"
#include "warzoneconfig.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"

#include "multiplay.h"
#include "lib/gamelib/gtime.h"
#include "mission.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "design.h"
#include "wrappers.h"
#include "init.h" // For fileLoadBuffer

/***************************************************************************/
/*
 *	local Definitions
 */
/***************************************************************************/
#define INCLUDE_AUDIO
#define RPL_WIDTH 640
#define RPL_HEIGHT 480
#define RPL_DEPTH 2	//bytes, 16bit
#define RPL_BITS_555 15	//15bit
#define RPL_MASK_555 0x7fff	//15bit
#define RPL_FRAME_TIME frameDuration	//milliseconds
#define STD_FRAME_TIME 40 //milliseconds
#define VIDEO_PLAYBACK_WIDTH 640
#define VIDEO_PLAYBACK_HEIGHT 480
#define VIDEO_PLAYBACK_DELAY 0
#define MAX_TEXT_OVERLAYS 32
#define MAX_SEQ_LIST	  6
#define SUBTITLE_BOX_MIN 430
#define SUBTITLE_BOX_MAX 480


typedef struct {
	char pText[MAX_STR_LENGTH];
	UDWORD x;
	UDWORD y;
	UDWORD startFrame;
	UDWORD endFrame;
	BOOL	bSubtitle;
} SEQTEXT;

typedef struct {
	const char	*pSeq;						//name of the sequence to play
	const char	*pAudio;					//name of the wav to play
	BOOL		bSeqLoop;					//loop this sequence
	SDWORD		currentText;				//cuurent number of text messages for this seq
	SEQTEXT		aText[MAX_TEXT_OVERLAYS];	//text data to display for this sequence
} SEQLIST;
/***************************************************************************/
/*
 *	local Variables
 */
/***************************************************************************/

static BOOL bBackDropWasAlreadyUp;
static BOOL bSeqInit = FALSE;
static BOOL bSeqPlaying = FALSE;
static BOOL bAudioPlaying = FALSE;
static BOOL bHoldSeqForAudio = FALSE;
static BOOL bCDPath = FALSE;
static BOOL bHardPath = FALSE;
static BOOL bSeqSubtitles = TRUE;
static char aCDPath[MAX_STR_LENGTH];
static char aHardPath[MAX_STR_LENGTH];
static char aVideoName[MAX_STR_LENGTH];
static char aAudioName[MAX_STR_LENGTH];
static char aTextName[MAX_STR_LENGTH];
static char aSubtitleName[MAX_STR_LENGTH];
static char* pVideoBuffer = NULL;
static char* pVideoPalette = NULL;
static VIDEO_MODE videoMode;
static PERF_MODE perfMode = VIDEO_PERF_FULLSCREEN;
static SDWORD frameSkip = 1;
static SEQLIST aSeqList[MAX_SEQ_LIST];
static SDWORD currentSeq = -1;
static SDWORD currentPlaySeq = -1;
static SDWORD frameDuration = 40;
static BOOL g_bResumeInGame = FALSE;
static int videoFrameTime = 0;
static SDWORD frame = 0;

/***************************************************************************/
/*
 *	local ProtoTypes
 */
/***************************************************************************/

static void	seq_SetVideoPath(void);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

 /* Renders a video sequence specified by filename to a buffer*/
BOOL	seq_RenderVideoToBuffer( iSurface *pSurface, char *sequenceName, int time, int seqCommand)
{
	SDWORD	frameLag;
	int	videoTime;
	BOOL state = TRUE;

	if (seqCommand == SEQUENCE_KILL)
	{
		//stop the movie
		seq_ShutDown();
		bSeqPlaying = FALSE;
//		bSeqInit = FALSE;
		//return seq_ReleaseVideoBuffers();
		return TRUE;
	}

	if (!bSeqInit)
	{
		//this is done in HCI intInitialise() now
		//if ((bSeqInit = seq_SetupVideoBuffers()) == FALSE)
		//	return FALSE;
	}

	if (!bSeqPlaying)
	{
		//set a valid video path if there is one
		if(!bCDPath && !bHardPath)
		{
			seq_SetVideoPath();
		}

		if (bHardPath)//use this first
		{
			ASSERT( (strlen(sequenceName) + strlen(aHardPath))<MAX_STR_LENGTH,"sequence path+name greater than max string" );
			strcpy(aVideoName,aHardPath);
			strcat(aVideoName,sequenceName);

			// check it exists. If not then try CD.
			if ( !PHYSFS_exists( aVideoName ) && bCDPath)
			{
				ASSERT( (strlen(sequenceName) + strlen(aCDPath))<MAX_STR_LENGTH,"sequence path+name greater than max string" );
				strcpy(aVideoName,aCDPath);
				strcat(aVideoName,sequenceName);
			}
		}
		else if (bCDPath)
		{
			ASSERT( (strlen(sequenceName) + strlen(aCDPath))<MAX_STR_LENGTH,"sequence path+name greater than max string" );
			strcpy(aVideoName,aCDPath);
			strcat(aVideoName,sequenceName);
		}
		else
		{
			ASSERT( FALSE,"seq_StartFullScreenVideo: sequence path not found" );
			return FALSE;
		}

		iV_SetFont(font_regular);
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);

		videoMode = VIDEO_D3D_WINDOW;

//for new timing
		frame = 0;
		videoFrameTime = SDL_GetTicks();

		if ((bSeqPlaying = seq_SetSequenceForBuffer(aVideoName, videoFrameTime, perfMode)) == FALSE)
		{
			ASSERT( FALSE,"seq_RenderVideoToBuffer: unable to initialise sequence %s",aVideoName );
			return FALSE;
		}
		bSeqPlaying = TRUE;
		frameDuration =	seq_GetFrameTimeInClicks();
//jps 18 feb 99		seq_RenderOneFrameToBuffer(pVideoBuffer, 1);//skip frame if behind
	}

	if (frame != VIDEO_FINISHED)
	{
		//new call with timing
		//poll the sequence player while timing the video
		videoTime = SDL_GetTicks();
		while (videoTime < (videoFrameTime + RPL_FRAME_TIME))
		{
			seq_RefreshVideoBuffers();
			videoTime = SDL_GetTicks();
		}
		frameLag = videoTime - videoFrameTime;
		frameLag /= RPL_FRAME_TIME;// if were running slow frame lag will be greater than 1
		videoFrameTime += frameLag * RPL_FRAME_TIME;//frame Lag should be 1 (most of the time)
		//call sequence player to decode a frame
		frame = seq_RenderOneFrameToBuffer(pVideoBuffer, frameLag, 2, 0);//skip frame if behind
//old call
//		frame = seq_RenderOneFrameToBuffer(pVideoBuffer, 1);
	}

	if (frame == VIDEO_FINISHED)
	{
		if (seqCommand == SEQUENCE_LOOP)
		{
			bSeqPlaying = FALSE;
			state = TRUE;
		}
		if (seqCommand == SEQUENCE_HOLD)
		{
			//wait for call to stop video
			state = TRUE;
		}
		else
		{
			state = FALSE;
			seq_ShutDown();
			bSeqPlaying = FALSE;
		}
	}
	else if (frame < 0) //an ERROR
	{
		debug( LOG_WZ, "VIDEO FRAME ERROR %d\n", frame );
		state = FALSE;
		seq_ShutDown();
		bSeqPlaying = FALSE;
	}

	return state;
}

BOOL	seq_BlitBufferToScreen(char* screen, SDWORD screenStride, SDWORD xOffset, SDWORD yOffset)
{
	int i,j;
	char c8, *d8;
	UWORD c16, *p16;
	SDWORD width, height;
	seq_GetFrameSize(&width, &height);

	if (videoMode == VIDEO_SOFT_WINDOW)
	{
		d8 = screen + xOffset + yOffset * screenStride;
		p16 = (UWORD*)pVideoBuffer;

		for (j = 0; j < height; j++)
		{
			for (i = 0; i < width; i++)
			{
				c16 = p16[i];
				c16 &= RPL_MASK_555;
				c8 = pVideoPalette[c16];
				d8[i] = c8;
			}
			d8 += screenStride;
			p16 += width;
		}
	}
	else
	{
		/* pie_DownLoadBufferToScreen(pVideoBuffer, xOffset, yOffset,width,height,(2*width)); no longer exists */
	}
	return TRUE;
}


BOOL seq_ReleaseVideoBuffers(void)
{
	free(pVideoBuffer);
	free(pVideoPalette);
	pVideoBuffer = NULL;
	pVideoPalette = NULL;
	return TRUE;
}

BOOL seq_SetupVideoBuffers(void)
{
	SDWORD c,mallocSize;
	UBYTE r,g,b;
	//assume 320 * 240 * 16bit playback surface
	mallocSize = (RPL_WIDTH*RPL_HEIGHT*RPL_DEPTH);
	if ((pVideoBuffer = (char*)malloc(mallocSize)) == NULL)
	{
		return FALSE;
	}

	mallocSize = 1<<(RPL_BITS_555);//palette only used in 555mode
	if ((pVideoPalette = (char*)malloc(mallocSize)) == NULL)
	{
		return FALSE;
	}

	//Assume 555 RGB buffer for 8 bit rendering
	c = 0;
	for(r = 0 ; r < 32 ; r++)
	{
	LOADBARCALLBACK();	//	loadingScreenCallback();

		for(g = 0 ; g < 32 ; g++)
		{
			for(b = 0 ; b < 32 ; b++)
			{
				pVideoPalette[(SDWORD)c] = (char)pal_GetNearestColour((Uint8)(r<<3),(Uint8)(g<<3),(Uint8)(b<<3));
				c++;
			}
		}
	}

	return TRUE;
}

static void seq_SetVideoPath(void)
{
	// now set up the hard disc path /

	if (!bHardPath)
	{
		strcpy(aHardPath, "sequences/");
#ifdef WAS_WIN32_NOW_UNUSED
		fileHandle = FindFirstFile("sequences/*.rpl",&findData);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
//			bHardPath = FALSE;	//If it fails, then why say true?  Cause we *ALWAYS* need the
			bHardPath=TRUE;		//videos enabled.  Yes, we are playing novideo.rpl, but for
			return;			//windows, if we don't have that directory, then what?
		}				//then we run into a game stopping bug!
		else
		{
			bHardPath = TRUE;
			FindClose(fileHandle);
			return;
		}
#else
		bHardPath=TRUE;			//yes, always true, as it should be on windows ALSO.
#endif
	}
}



static BOOL SeqEndCallBack( void *psObj )
{
	bAudioPlaying = FALSE;
	debug(LOG_NEVER, "************* briefing ended **************");

	return TRUE;
}

//full screenvideo functions
static BOOL seq_StartFullScreenVideo(const char* videoName, const char* audioName)
{
	bHoldSeqForAudio = FALSE;

	frameSkip = 1;
	switch(war_GetSeqMode())
	{
		case SEQ_SMALL:
			perfMode = VIDEO_PERF_WINDOW;
			break;
		case SEQ_SKIP:
			frameSkip = 2;
			perfMode = VIDEO_PERF_SKIP_FRAMES;
			break;
		default:
		case SEQ_FULL:
			perfMode = VIDEO_PERF_FULLSCREEN;
			break;
	}



	//set a valid video path if there is one
	if(!bCDPath && !bHardPath)
	{
		seq_SetVideoPath();
	}

	if (bHardPath)//use this first
	{
		ASSERT( (strlen(videoName) + strlen(aHardPath))<MAX_STR_LENGTH,"sequence path+name greater than max string" );
		strcpy(aVideoName,aHardPath);
		strcat(aVideoName,videoName);

		// check it exists. If not then try CD.
		if ( !PHYSFS_exists( aVideoName ) && bCDPath)
		{
			ASSERT( (strlen(videoName) + strlen(aCDPath))<MAX_STR_LENGTH,"sequence path+name greater than max string" );
			strcpy(aVideoName,aCDPath);
			strcat(aVideoName,videoName);
		}
	}
	else if (bCDPath)
	{
		ASSERT( (strlen(videoName) + strlen(aCDPath)) < MAX_STR_LENGTH, "sequence path+name greater than max string" );
		strcpy(aVideoName, aCDPath);
		strcat(aVideoName, videoName);
	}
	else
	{
		ASSERT( FALSE,"seq_StartFullScreenVideo: sequence path not found" );
		return FALSE;
	}


	//set audio path
	if (audioName != NULL)
	{
		ASSERT( strlen(audioName) < MAX_STR_LENGTH, "sequence path+name greater than max string" );
		strcpy(aAudioName, "sequenceaudio/");
		strcat(aAudioName, audioName);
	}

	//start video mode
	if (loop_GetVideoMode() == 0)
	{
		cdAudio_Pause();
		loop_SetVideoPlaybackMode();
		iV_SetFont(font_regular);
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	}

	if (audioName != NULL)
	{
		ASSERT( strlen(audioName) < MAX_STR_LENGTH, "sequence path+name greater than max string" );
		strcpy(aAudioName, "sequenceaudio/");
		strcat(aAudioName, audioName);
	}


//for new timing
	frame = 0;
	videoFrameTime = SDL_GetTicks();

	if (!seq_SetSequence(aVideoName, videoFrameTime + VIDEO_PLAYBACK_DELAY, pVideoBuffer, perfMode))
	{
		seq_StopFullScreenVideo();
		ASSERT( FALSE,"seq_StartFullScreenVideo: unable to initialise sequence %s",aVideoName );
		return FALSE;
	}
	if (perfMode != VIDEO_PERF_SKIP_FRAMES)//JPS fix for video problems with some sound cards 9 may 99
	{
		frameDuration =	seq_GetFrameTimeInClicks();
	}
	else
	{
		frameDuration =	(seq_GetFrameTimeInClicks() * 112) / 100;//JPS fix for video problems with some sound cards 9 may 99
	}

	if (audioName == NULL)
	{
		bAudioPlaying = FALSE;
	}
	else
	{
		bAudioPlaying = audio_PlayStream( aAudioName, AUDIO_VOL_MAX, SeqEndCallBack );
		ASSERT( bAudioPlaying == TRUE,"seq_StartFullScreenVideo: unable to initialise sound %s",aAudioName );
	}

	return TRUE;
}

BOOL seq_UpdateFullScreenVideo(int *pbClear)
{
	SDWORD i, x, y, w, h;
	SDWORD frame, frameLag;
	UDWORD subMin, subMax;
	UDWORD videoTime, realFrame;
	static UDWORD videoFrameTime = 0, textFrame = 0;
	BOOL bMoreThanOneSequenceLine = FALSE;

	if (seq_GetCurrentFrame() == 0)
	{
//for new timing
		frame = 0;
		videoFrameTime = SDL_GetTicks();
		textFrame = 0;
	}

	seq_GetFrameSize(&w,&h);

	x = (pie_GetVideoBufferWidth() - w)/2;
	y = (pie_GetVideoBufferHeight() - h)/2;

	subMin = SUBTITLE_BOX_MAX + D_H;
	subMax = SUBTITLE_BOX_MIN + D_H;

	//get any text lines over bottom of the video
	realFrame = textFrame + 1;
	for(i=0;i<MAX_TEXT_OVERLAYS;i++)
	{
		if (aSeqList[currentPlaySeq].aText[i].pText[0] != 0)
		{
			if (aSeqList[currentPlaySeq].aText[i].bSubtitle == TRUE)
			{
				if ((realFrame >= aSeqList[currentPlaySeq].aText[i].startFrame) && (realFrame <= aSeqList[currentPlaySeq].aText[i].endFrame))
				{
					if (subMin > aSeqList[currentPlaySeq].aText[i].y)
					{
						if (aSeqList[currentPlaySeq].aText[i].y > SUBTITLE_BOX_MIN)
						{
							subMin = aSeqList[currentPlaySeq].aText[i].y;
						}
					}
					if (subMax < aSeqList[currentPlaySeq].aText[i].y)
					{
						subMax = aSeqList[currentPlaySeq].aText[i].y;
					}
				}
				else if (aSeqList[currentPlaySeq].bSeqLoop)//if its a looped video always draw the text
				{
					if (subMin >= aSeqList[currentPlaySeq].aText[i].y)
					{
						if (aSeqList[currentPlaySeq].aText[i].y > SUBTITLE_BOX_MIN)
						{
							subMin = aSeqList[currentPlaySeq].aText[i].y;
						}
					}
					if (subMax < aSeqList[currentPlaySeq].aText[i].y)
					{
						subMax = aSeqList[currentPlaySeq].aText[i].y;
					}
				}
			}
			if ((realFrame >= aSeqList[currentPlaySeq].aText[i].endFrame) && (realFrame < (aSeqList[currentPlaySeq].aText[i].endFrame + frameSkip)))
			{
				if (pbClear != NULL)
				{
					if (perfMode != VIDEO_PERF_FULLSCREEN)
					{
						*pbClear = CLEAR_BLACK;
					}
				}
			}
		}
	}

	subMin -= D_H;//adjust video window here because text is already ofset for big screens
	subMax -= D_H;

	if (subMin < SUBTITLE_BOX_MIN)
	{
		subMin = SUBTITLE_BOX_MIN;
	}
	if (subMax > SUBTITLE_BOX_MAX)
	{
		subMax = SUBTITLE_BOX_MAX;
	}

	if (subMax > subMin)
	{
		bMoreThanOneSequenceLine = TRUE;
	}

	{
		if (bHoldSeqForAudio == FALSE)
		{
			if (perfMode != VIDEO_PERF_SKIP_FRAMES)
			{
				//version 1.00 release code
				//poll the sequence player while timing the video
				videoTime = SDL_GetTicks();
				while (videoTime < (videoFrameTime + (RPL_FRAME_TIME * frameSkip)))
				{
					videoTime = SDL_GetTicks();
					seq_RefreshVideoBuffers();
				}
				frameLag = videoTime - videoFrameTime;
				frameLag /= RPL_FRAME_TIME;// if were running slow frame lag will be greater than 1
				videoFrameTime += frameLag * RPL_FRAME_TIME;//frame Lag should be 1 (most of the time)
				//call sequence player to decode a frame
				frame = seq_RenderOneFrame(frameLag, subMin, subMax);
			}
			else
			{
				//new version with timeing removed
				//poll the sequence player while timing the video
				videoTime = SDL_GetTicks();
				while (videoTime < (videoFrameTime + (RPL_FRAME_TIME * frameSkip)))
				{
					videoTime = SDL_GetTicks();
					seq_RefreshVideoBuffers();
				}
				videoFrameTime += frameSkip * RPL_FRAME_TIME;//frame Lag should be 1 (most of the time)
				//call sequence player to decode a frame
				frame = seq_RenderOneFrame(frameSkip, subMin, subMax);
			}
		}
		else
		{
			//call sequence player to download last frame
			frame = seq_RenderOneFrame(0, 2, 0);
		}
		//print any text over the video
		realFrame = textFrame + 1;
		for(i=0;i<MAX_TEXT_OVERLAYS;i++)
		{
			if (aSeqList[currentPlaySeq].aText[i].pText[0] != 0)
			{
				if ((realFrame >= aSeqList[currentPlaySeq].aText[i].startFrame) && (realFrame <= aSeqList[currentPlaySeq].aText[i].endFrame))
				{
					if (bMoreThanOneSequenceLine)
					{
						aSeqList[currentPlaySeq].aText[i].x = 20 + D_W;
					}
					iV_DrawText(&(aSeqList[currentPlaySeq].aText[i].pText[0]),
							aSeqList[currentPlaySeq].aText[i].x, aSeqList[currentPlaySeq].aText[i].y);
				}
				else if (aSeqList[currentPlaySeq].bSeqLoop)//if its a looped video always draw the text
				{
					if (bMoreThanOneSequenceLine)
					{
						aSeqList[currentPlaySeq].aText[i].x = 20 + D_W;
					}
					iV_DrawText(&(aSeqList[currentPlaySeq].aText[i].pText[0]),
							aSeqList[currentPlaySeq].aText[i].x, aSeqList[currentPlaySeq].aText[i].y);
				}

			}
		}
	}

	textFrame = frame * RPL_FRAME_TIME/STD_FRAME_TIME;

	if ((frame == VIDEO_FINISHED) || (bHoldSeqForAudio))
	{
#ifndef SEQ_LOOP
		if (bAudioPlaying)
#endif
		{
			if (aSeqList[currentPlaySeq].bSeqLoop)
			{
				seq_ClearMovie();
				if (!seq_SetSequence(aVideoName, SDL_GetTicks() + VIDEO_PLAYBACK_DELAY, pVideoBuffer, perfMode))
				{
					bHoldSeqForAudio = TRUE;
				}
				frameDuration =	seq_GetFrameTimeInClicks();
			}
			else
			{
				bHoldSeqForAudio = TRUE;
			}
			return TRUE;//should hold the video
		}
#ifndef SEQ_LOOP
		else
		{
			return FALSE;//should terminate the video
		}
#endif
	}
	else if (frame < 0) //an ERROR
	{
		debug( LOG_NEVER, "VIDEO FRAME ERROR %d\n", frame );
		return FALSE;
	}

	return TRUE;
}

BOOL seq_StopFullScreenVideo(void)
{
	if (!seq_AnySeqLeft())
	{
		loop_ClearVideoPlaybackMode();
	}

	seq_ShutDown();

	if (!seq_AnySeqLeft())
	{
		if ( g_bResumeInGame == TRUE )
		{
			resetDesignPauseState();
			intAddReticule();
			g_bResumeInGame = FALSE;
		}
	}

	return TRUE;
}

BOOL seq_GetVideoSize(SDWORD* pWidth, SDWORD* pHeight)
{
	return seq_GetFrameSize(pWidth, pHeight);
}

#define BUFFER_WIDTH 600
#define FOLLOW_ON_JUSTIFICATION 160
#define MIN_JUSTIFICATION 40

// add a string at x,y or add string below last line if x and y are 0
BOOL seq_AddTextForVideo(char* pText, SDWORD xOffset, SDWORD yOffset, SDWORD startFrame, SDWORD endFrame, SDWORD bJustify)
{
	SDWORD sourceLength, currentLength;
	char* currentText;
	SDWORD justification;
static SDWORD lastX;

	iV_SetFont(font_regular);

	ASSERT( aSeqList[currentSeq].currentText < MAX_TEXT_OVERLAYS,
		"seq_AddTextForVideo: too many text lines" );

	sourceLength = strlen(pText);
	currentLength = sourceLength;
	currentText = &(aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].pText[0]);

	//if the string is bigger than the buffer get the last end of the last fullword in the buffer
	if (currentLength >= MAX_STR_LENGTH)
	{
		currentLength = MAX_STR_LENGTH - 1;
		//get end of the last word
		while((pText[currentLength] != ' ') && (currentLength > 0))
		{
			currentLength--;
		}
		currentLength--;
	}

	memcpy(currentText,pText,currentLength);
	currentText[currentLength] = 0;//terminate the string what ever

	//check the string is shortenough to print
	//if not take a word of the end and try again
	while(iV_GetTextWidth(currentText) > BUFFER_WIDTH)
	{
		currentLength--;
		while((pText[currentLength] != ' ') && (currentLength > 0))
		{
			currentLength--;
		}
		currentText[currentLength] = 0;//terminate the string what ever
	}
	currentText[currentLength] = 0;//terminate the string what ever

	//check if x and y are 0 and put text on next line
	if (((xOffset == 0) && (yOffset == 0)) && (currentLength > 0))
	{
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x = lastX;
		//	aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText-1].x;
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].y = aSeqList[currentSeq].
			aText[aSeqList[currentSeq].currentText-1].y + iV_GetTextLineSize();
	}
	else
	{
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x = xOffset + D_W;
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].y = yOffset + D_H;
	}
	lastX = aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x;

	if ((bJustify) && (currentLength == sourceLength))
	{
		//justify this text
		justification = BUFFER_WIDTH - iV_GetTextWidth(currentText);
		if ((bJustify == SEQ_TEXT_JUSTIFY) && (justification > MIN_JUSTIFICATION))
		{
			aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x += (justification/2);
		}
		else if ((bJustify == SEQ_TEXT_FOLLOW_ON) && (justification > FOLLOW_ON_JUSTIFICATION))
		{

		}
	}


	//set start and finish times for the objects
	aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].startFrame = startFrame;
	aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].endFrame = endFrame;
	aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].bSubtitle = bJustify;

	aSeqList[currentSeq].currentText++;
	if (aSeqList[currentSeq].currentText >= MAX_TEXT_OVERLAYS)
	{
		aSeqList[currentSeq].currentText = 0;
	}

	//check text is okay on the screen
	if (currentLength < sourceLength)
	{
		//RECURSE x= 0 y = 0 for nextLine
		if (bJustify == SEQ_TEXT_JUSTIFY)
		{
			bJustify = SEQ_TEXT_POSITION;
		}
		seq_AddTextForVideo(&pText[currentLength + 1], 0, 0, startFrame, endFrame, bJustify);
	}
	return TRUE;
}

BOOL seq_ClearTextForVideo(void)
{
	SDWORD i, j;

	for (j=0; j < MAX_SEQ_LIST; j++)
	{
		for(i=0;i<MAX_TEXT_OVERLAYS;i++)
		{
			aSeqList[j].aText[i].pText[0] = 0;
			aSeqList[j].aText[i].x = 0;
			aSeqList[j].aText[i].y = 0;
			aSeqList[j].aText[i].startFrame = 0;
			aSeqList[j].aText[i].endFrame = 0;
			aSeqList[j].aText[i].bSubtitle = 0;
		}
		aSeqList[j].currentText = 0;
	}
	return TRUE;
}

static BOOL seq_AddTextFromFile(const char *pTextName, BOOL bJustify)
{
	char *pTextBuffer, *pCurrentLine, *pText;
	UDWORD fileSize;
//	HANDLE	fileHandle;
//	WIN32_FIND_DATA findData;
//	BOOL endOfFile = FALSE;
	SDWORD xOffset, yOffset, startFrame, endFrame;
	const char *seps = "\n";

	strcpy(aTextName,"sequenceaudio/");
	strcat(aTextName,pTextName);

/*
	fileHandle = FindFirstFile(aTextName,&findData);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	FindClose(fileHandle);
*/
	if (loadFileToBufferNoError(aTextName, fileLoadBuffer, FILE_LOAD_BUFFER_SIZE, &fileSize) == FALSE)  //Did I mention this is lame? -Q
	{
		return FALSE;
	}

	pTextBuffer = fileLoadBuffer;
	pCurrentLine = strtok(pTextBuffer,seps);
	while(pCurrentLine != NULL)
	{
		if (*pCurrentLine != '/')
		{
			if (sscanf(pCurrentLine,"%d %d %d %d", &xOffset, &yOffset, &startFrame, &endFrame) == 4)
			{
				//get the text
				pText = strrchr(pCurrentLine,'"');
				ASSERT( pText != NULL,"seq_AddTextFromFile error parsing text file" );
				if (pText != NULL)
				{
					*pText = (UBYTE)0;
				}
				pText = strchr(pCurrentLine,'"');
				ASSERT( pText != NULL,"seq_AddTextFromFile error parsing text file" );
				if (pText != NULL)
				{
					seq_AddTextForVideo(&pText[1], xOffset, yOffset, startFrame, endFrame, bJustify);
				}
			}
		}
		//get next line
		pCurrentLine = strtok(NULL,seps);
	}
	return TRUE;
}



//clear the sequence list
void seq_ClearSeqList(void)
{
	SDWORD i;

	seq_ClearTextForVideo();
	for(i=0;i<MAX_SEQ_LIST;i++)
	{
		aSeqList[i].pSeq = NULL;
	}
	currentSeq = -1;
	currentPlaySeq = -1;
}

//add a sequence to the list to be played
void seq_AddSeqToList(const char *pSeqName, const char *pAudioName, const char *pTextName, BOOL bLoop)
{
	SDWORD strLen;
	currentSeq++;


	if ((currentSeq) >=  MAX_SEQ_LIST)
	{
		ASSERT( FALSE, "seq_AddSeqToList: too many sequences" );
		return;
	}
#ifdef SEQ_LOOP
	bLoop = TRUE;
#endif

	//OK so add it to the list
	aSeqList[currentSeq].pSeq = pSeqName;
	aSeqList[currentSeq].pAudio = pAudioName;
	aSeqList[currentSeq].bSeqLoop = bLoop;
	if (pTextName != NULL)
	{
		seq_AddTextFromFile(pTextName, FALSE);//SEQ_TEXT_POSITION);//ordinary text not justified
	}

	if (bSeqSubtitles)
	{
		//check for a subtitle file
		strLen = strlen(pSeqName);
		ASSERT( strLen < MAX_STR_LENGTH,"seq_AddSeqToList: sequence name error" );
		strcpy(aSubtitleName,pSeqName);
		aSubtitleName[strLen - 4] = 0;
		strcat(aSubtitleName,".txt");
		seq_AddTextFromFile(aSubtitleName, TRUE);//SEQ_TEXT_JUSTIFY);//subtitles centre justified
	}
}

/*checks to see if there are any sequences left in the list to play*/
BOOL seq_AnySeqLeft(void)
{
	UBYTE		nextSeq;

	nextSeq = (UBYTE)(currentPlaySeq+1);

	//check haven't reached end
	if (nextSeq > MAX_SEQ_LIST)
	{
		return FALSE;
	}
	else if (aSeqList[nextSeq].pSeq)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void seqDispCDOK( void )
{
	BOOL	bPlayedOK;

	if ( bBackDropWasAlreadyUp == FALSE )
	{
		screen_StopBackDrop();
	}

	currentPlaySeq++;
	if (currentPlaySeq >= MAX_SEQ_LIST)
	{
		bPlayedOK = FALSE;
	}
	else
	{
		bPlayedOK = seq_StartFullScreenVideo( aSeqList[currentPlaySeq].pSeq, aSeqList[currentPlaySeq].pAudio );
	}

	if ( bPlayedOK == FALSE )
	{
		//don't do the callback if we're playing the win/lose video
		if (!getScriptWinLoseVideo())
		{
			debug(LOG_SCRIPT, "*** Called video quit trigger!");
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
		}
		else
		{
			displayGameOver(getScriptWinLoseVideo() == PLAY_WIN);
		}
	}
}

/*returns the next sequence in the list to play*/
void seq_StartNextFullScreenVideo(void)
{
		seqDispCDOK();
		return;
}

void seq_SetSubtitles(BOOL bNewState)
{
	bSeqSubtitles = bNewState;
}

BOOL seq_GetSubtitles(void)
{
	return bSeqSubtitles;
}
