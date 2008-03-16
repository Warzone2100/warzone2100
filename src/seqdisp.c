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
/**
 * @file seqdisp.c
 *
 * Functions for the display of the Escape Sequences (FMV).
 *
 */
#include "lib/framework/frame.h"

#include <string.h>
#include <SDL_timer.h>
#include <physfs.h>

#include "lib/framework/file.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/sequence/sequence.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/script/script.h"

#include "seqdisp.h"

#include "warzoneconfig.h"
#include "hci.h"//for font
#include "loop.h"
#include "scripttabs.h"
#include "design.h"
#include "wrappers.h"
#include "init.h" // For fileLoadBuffer
#include "drive.h"

/***************************************************************************/
/*
 *	local Definitions
 */
/***************************************************************************/
#define STD_FRAME_TIME 40 //milliseconds
#define VIDEO_PLAYBACK_WIDTH 640
#define VIDEO_PLAYBACK_HEIGHT 480
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
static BOOL bAudioPlaying = FALSE;
static BOOL bHoldSeqForAudio = FALSE;
static BOOL bCDPath = FALSE;
static BOOL bHardPath = FALSE;
static BOOL bSeqSubtitles = TRUE;
static char aHardPath[MAX_STR_LENGTH];
static char aVideoName[MAX_STR_LENGTH];
static SEQLIST aSeqList[MAX_SEQ_LIST];
static SDWORD currentSeq = -1;
static SDWORD currentPlaySeq = -1;
static BOOL g_bResumeInGame = FALSE;

static unsigned int time_started = 0;

/***************************************************************************/
/*
 *	local ProtoTypes
 */
/***************************************************************************/

static void	seq_SetVideoPath(void);
static BOOL seq_StartFullScreenVideo(const char* videoName, const char* audioName);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

 /* Renders a video sequence specified by filename to a buffer*/
BOOL	seq_RenderVideoToBuffer( iSurface *pSurface, char *sequenceName, int time, int seqCommand)
{
	if (seqCommand == SEQUENCE_KILL)
	{
		//stop the movie
		seq_Shutdown();
		return TRUE;
	}

	seq_StartFullScreenVideo(sequenceName, NULL);

	return TRUE;
}

static void seq_SetVideoPath(void)
{
	// now set up the hard disc path /
	if (!bHardPath)
	{
		strcpy(aHardPath, "sequences/");
		bHardPath=TRUE;			//yes, always true, as it should be on windows ALSO.
	}
}

//full screenvideo functions
static BOOL seq_StartFullScreenVideo(const char* videoName, const char* audioName)
{
	const char* aAudioName;
	bHoldSeqForAudio = FALSE;

	//set a valid video path if there is one
	if(!bCDPath && !bHardPath)
	{
		seq_SetVideoPath();
	}

	ASSERT( (strlen(videoName) + strlen(aHardPath))<MAX_STR_LENGTH,"sequence path+name greater than max string" );
	snprintf(aVideoName, sizeof(aVideoName), "%s%s", aHardPath, videoName);

	//set audio path
	if (audioName != NULL)
	{
		ASSERT( strlen(audioName) < MAX_STR_LENGTH, "sequence path+name greater than max string" );
		sasprintf((char**)&aAudioName, "sequenceaudio/%s", audioName);
	}

	//start video mode
	if (loop_GetVideoMode() == 0)
	{
		cdAudio_Pause();
		loop_SetVideoPlaybackMode();
		iV_SetFont(font_regular);
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	}

	if (!seq_Play(aVideoName))
	{
		// don't stop or cancel it as we may have text to display
		debug( LOG_WARNING,"seq_StartFullScreenVideo: unable to initialise sequence %s",aVideoName );
	}

	if (audioName == NULL)
	{
		bAudioPlaying = FALSE;
	}
	else
	{
		static const float maxVolume = 1.f;

		bAudioPlaying = audio_PlayStream(aAudioName, maxVolume, NULL, NULL) ? TRUE : FALSE;
		ASSERT(bAudioPlaying == TRUE, "seq_StartFullScreenVideo: unable to initialise sound %s", aAudioName);
	}

	time_started = SDL_GetTicks();

	return TRUE;
}

BOOL seq_UpdateFullScreenVideo(int *pbClear)
{
	SDWORD i, x, y, w = 640, h = 480;
	UDWORD subMin, subMax;
	UDWORD realFrame;
	BOOL bMoreThanOneSequenceLine = FALSE;
	BOOL stillText;

	x = (pie_GetVideoBufferWidth() - w)/2;
	y = (pie_GetVideoBufferHeight() - h)/2;

	subMin = SUBTITLE_BOX_MAX + D_H;
	subMax = SUBTITLE_BOX_MIN + D_H;

	stillText = FALSE;
	//get any text lines over bottom of the video
	realFrame = (SDL_GetTicks()-time_started)/40; // standard frame is 40 msec
	for(i=0;i<MAX_TEXT_OVERLAYS;i++)
	{
		if (aSeqList[currentPlaySeq].aText[i].pText[0] != 0)
		{
			if (aSeqList[currentPlaySeq].aText[i].bSubtitle == TRUE)
			{
				if(realFrame <= aSeqList[currentPlaySeq].aText[i].endFrame)
				{
					stillText = TRUE;
				}
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

	//call sequence player to download last frame
	if(seq_Playing())
	{
		seq_Update();
	}

	//print any text over the video
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

	if ((!seq_Playing() && !stillText) || (bHoldSeqForAudio))
	{
		if (bAudioPlaying)
		{
			if (aSeqList[currentPlaySeq].bSeqLoop)
			{
				seq_Shutdown();
				if (!seq_Play(aVideoName))
				{
					bHoldSeqForAudio = TRUE;
				}
			}
			else
			{
				bHoldSeqForAudio = TRUE;
			}
			return TRUE;//should hold the video
		}
		else
		{
			return FALSE;//should terminate the video
		}
	}
	return TRUE;
}

BOOL seq_StopFullScreenVideo(void)
{
	StopDriverMode();
	if (!seq_AnySeqLeft())
	{
		loop_ClearVideoPlaybackMode();
	}

	seq_Shutdown();

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
	char aTextName[MAX_STR_LENGTH];
	char *pTextBuffer, *pCurrentLine, *pText;
	UDWORD fileSize;
	SDWORD xOffset, yOffset, startFrame, endFrame;
	const char *seps = "\n";

	snprintf(aTextName, sizeof(aTextName), "sequenceaudio/%s", pTextName);

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
		char aSubtitleName[MAX_STR_LENGTH];

		//check for a subtitle file
		strLen = strlen(pSeqName);
		ASSERT( strLen < MAX_STR_LENGTH,"seq_AddSeqToList: sequence name error" );
		strlcpy(aSubtitleName, pSeqName, sizeof(aSubtitleName));
		aSubtitleName[strLen - 4] = 0;
		strlcat(aSubtitleName, ".txt", sizeof(aSubtitleName));
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
