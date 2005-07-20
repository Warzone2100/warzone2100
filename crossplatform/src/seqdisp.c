/*
 * SeqDisp.c		(Sequence Display)
 *
 * Functions for the display of the Escape Sequences
 *
 */
#include "frame.h"
#include "widget.h"
#include "rendmode.h"
#include "seqdisp.h"
#include "sequence.h"
#include "loop.h"
#include "piedef.h"
#include "piefunc.h"
#include "piestate.h"
#include "hci.h"//for font
#include "audio.h"
#include "cdaudio.h"
#include "deliverance.h"
#include "warzoneconfig.h"
#include "screen.h"

#include "multiplay.h"
#include "gtime.h"
#include "mission.h"
#include "script.h"
#include "scripttabs.h"
#include "design.h"
#include "wrappers.h"

/***************************************************************************/
/*
 *	local Definitions
 */
/***************************************************************************/
#define INCLUDE_AUDIO
#define DUMMY_VIDEO
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
	SDWORD x;
	SDWORD y;
	SDWORD startFrame;
	SDWORD endFrame;
	BOOL	bSubtitle;
} SEQTEXT; 

typedef struct {
	char		*pSeq;						//name of the sequence to play
	char		*pAudio;					//name of the wav to play
	BOOL		bSeqLoop;					//loop this sequence
	SDWORD		currentText;				//cuurent number of text messages for this seq
	SEQTEXT		aText[MAX_TEXT_OVERLAYS];	//text data to display for this sequence
} SEQLIST; 
/***************************************************************************/
/*
 *	local Variables
 */
/***************************************************************************/

static BOOL	bBackDropWasAlreadyUp;

BOOL bSeqInit = FALSE;
BOOL bSeqPlaying = FALSE;
BOOL bAudioPlaying = FALSE;
BOOL bHoldSeqForAudio = FALSE;
BOOL bSeq8Bit = TRUE;
BOOL bCDPath = FALSE;
BOOL bHardPath = FALSE;
BOOL bSeqSubtitles = TRUE;
char aCDPath[MAX_STR_LENGTH];
char aHardPath[MAX_STR_LENGTH];
char aVideoName[MAX_STR_LENGTH];
char aAudioName[MAX_STR_LENGTH];
char aTextName[MAX_STR_LENGTH];
char aSubtitleName[MAX_STR_LENGTH];
char* pVideoBuffer = NULL;
UWORD *p3DFXVideoBuffer = NULL; 
char* pVideoPalette = NULL;
VIDEO_MODE videoMode;
PERF_MODE perfMode = VIDEO_PERF_FULLSCREEN;
static SDWORD frameSkip = 1;
SEQLIST aSeqList[MAX_SEQ_LIST];
static SDWORD currentSeq = -1;
static SDWORD currentPlaySeq = -1;
static SDWORD frameDuration = 40;

static BOOL			g_bResumeInGame = FALSE;
static CD_INDEX		g_CDrequired = DISC_EITHER;

static int videoFrameTime = 0;
static	SDWORD frame = 0;

/***************************************************************************/
/*
 *	local ProtoTypes
 */
/***************************************************************************/

void	clearVideoBuffer(iSurface *surface);
void	seq_SetVideoPath(void);

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
	FILE	*pFileHandle;
	DDPIXELFORMAT *pDDPixelFormat;

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
			ASSERT(((strlen(sequenceName) + strlen(aHardPath))<MAX_STR_LENGTH,"sequence path+name greater than max string"));
			strcpy(aVideoName,aHardPath);
			strcat(aVideoName,sequenceName);
		
			// check it exists. If not then try CD.
			pFileHandle = fopen(aVideoName, "rb");
			if (pFileHandle == NULL)
			{
				if (bCDPath) {
					ASSERT(((strlen(sequenceName) + strlen(aCDPath))<MAX_STR_LENGTH,"sequence path+name greater than max string"));
					strcpy(aVideoName,aCDPath);
					strcat(aVideoName,sequenceName);
				}
			}
			else
			{
				fclose(pFileHandle);
			}
		}
		else if (bCDPath)
		{
			ASSERT(((strlen(sequenceName) + strlen(aCDPath))<MAX_STR_LENGTH,"sequence path+name greater than max string"));
			strcpy(aVideoName,aCDPath);
			strcat(aVideoName,sequenceName);
		}
		else
		{
			ASSERT((FALSE,"seq_StartFullScreenVideo: sequence path not found"));
			return FALSE;
		}
	

		iV_SetFont(WFont);
		iV_SetTextColour(-1);


		if (pie_GetRenderEngine() == ENGINE_4101)
		{
			videoMode = VIDEO_SOFT_WINDOW;
			pDDPixelFormat = NULL;
		}
		else if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			videoMode = VIDEO_3DFX_WINDOW;
			pDDPixelFormat = NULL;
		}
		else
		{
			videoMode = VIDEO_D3D_WINDOW;
			pDDPixelFormat = screenGetBackBufferPixelFormat();
		}

//for new timing
		frame = 0;
		videoFrameTime = GetTickCount();

#ifdef INCLUDE_AUDIO
		if ((bSeqPlaying = seq_SetSequenceForBuffer(aVideoName, videoMode, audio_GetDirectSoundObj(), videoFrameTime, pDDPixelFormat, perfMode)) == FALSE)
#else
		if ((bSeqPlaying = seq_SetSequenceForBuffer(aVideoName, videoMode, NULL, videoFrameTime, pDDPixelFormat, perfMode)) == FALSE)
#endif
		{
#ifdef DUMMY_VIDEO
			if ((bSeqPlaying = seq_SetSequenceForBuffer("noVideo.rpl", videoMode, NULL, time, pDDPixelFormat, perfMode)) == TRUE)
			{
				return TRUE;
			}
#endif
			ASSERT((FALSE,"seq_RenderVideoToBuffer: unable to initialise sequence %s",aVideoName));
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
		videoTime = GetTickCount();
		while (videoTime < (videoFrameTime + RPL_FRAME_TIME))
		{
			seq_RefreshVideoBuffers();
			videoTime = GetTickCount();
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
		DBPRINTF(("VIDEO FRAME ERROR %d\n",frame));
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
		pie_DownLoadBufferToScreen(pVideoBuffer, xOffset, yOffset,width,height,(2*width));
	}
	return TRUE;
}


void	clearVideoBuffer(iSurface *surface)
{
UDWORD	i;
UDWORD	*toClear;

	toClear = (UDWORD *)surface->buffer;
	for (i=0; i<(UDWORD)(surface->size/4); i++)
	{
		*toClear++ = (UDWORD)0xFCFCFCFC;
	}
}
 
BOOL seq_ReleaseVideoBuffers(void)
{
	FREE(pVideoBuffer);
	FREE(pVideoPalette);
	return TRUE;
}
 
BOOL seq_SetupVideoBuffers(void)
{
	SDWORD c,mallocSize;
	UBYTE r,g,b;
	//assume 320 * 240 * 16bit playback surface
	mallocSize = (RPL_WIDTH*RPL_HEIGHT*RPL_DEPTH);
	if ((pVideoBuffer = MALLOC(mallocSize)) == NULL)
	{
		return FALSE;
	}

	mallocSize = 1<<(RPL_BITS_555);//palette only used in 555mode
	if ((pVideoPalette = MALLOC(mallocSize)) == NULL)
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
				pVideoPalette[(SDWORD)c] = (char)pal_GetNearestColour((uint8)(r<<3),(uint8)(g<<3),(uint8)(b<<3));
				c++;
			}
		}
	}

	if(pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		p3DFXVideoBuffer = (UWORD*)pVideoBuffer;
	}

	return TRUE;
}

void seq_SetVideoPath(void)
{
	char	aCDDrive[256] = "";
#ifdef WIN32
	WIN32_FIND_DATA findData;
	HANDLE	fileHandle;
#endif
	/* set up the CD path */
	if (!bCDPath)
	{
		if ( cdspan_GetCDLetter( aCDDrive, g_CDrequired ) == TRUE )
		{
			if (strlen( aCDDrive ) <= (MAX_STR_LENGTH - 20))//leave enough space to add "\\warzone\\sequences\\"
			{
				strcpy(aCDPath, aCDDrive);
				strcat(aCDPath, "warzone\\sequences\\");
				bCDPath = TRUE;
			}
		}
	}
	/* now set up the hard disc path */
	if (!bHardPath)
	{
		strcpy(aHardPath, "sequences\\");
#ifdef WIN32
		fileHandle = FindFirstFile("sequences\\*.rpl",&findData);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			bHardPath = FALSE;
		}
		else
		{
			bHardPath = TRUE;
			FindClose(fileHandle);
		}
#else
		bHardPath=TRUE;
#endif
	}
}



BOOL SeqEndCallBack( AUDIO_SAMPLE *psSample )
{
//	psSample;
	bAudioPlaying = FALSE;
	dbg_printf("************* briefing ended **************\n");

	return TRUE;
}

//full screenvideo functions
BOOL seq_StartFullScreenVideo(char* videoName, char* audioName)
{
	SDWORD	i;
	FILE	*pFileHandle;
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
		ASSERT(((strlen(videoName) + strlen(aHardPath))<MAX_STR_LENGTH,"sequence path+name greater than max string"));
		strcpy(aVideoName,aHardPath);
		strcat(aVideoName,videoName);

		// check it exists. If not then try CD.
		pFileHandle = fopen(aVideoName, "rb");
		if (pFileHandle == NULL && bCDPath)
		{
			ASSERT(((strlen(videoName) + strlen(aCDPath))<MAX_STR_LENGTH,"sequence path+name greater than max string"));
			strcpy(aVideoName,aCDPath);
			strcat(aVideoName,videoName);
		}
		else if (pFileHandle != NULL)
		{
			fclose(pFileHandle);
		}
	}
	else if (bCDPath)
	{
		ASSERT(((strlen(videoName) + strlen(aCDPath))<MAX_STR_LENGTH,"sequence path+name greater than max string"));
		strcpy(aVideoName,aCDPath);
		strcat(aVideoName,videoName);
	}
	else
	{
		ASSERT((FALSE,"seq_StartFullScreenVideo: sequence path not found"));
		return FALSE;
	}

	
	//set audio path
	if (audioName != NULL)
	{
		ASSERT((strlen(audioName)<244,"sequence path+name greater than max string"));
		strcpy(aAudioName,"sequenceAudio\\");
		strcat(aAudioName,audioName);
	}

	//start video mode
	if (loop_GetVideoMode() == 0)
	{
		cdAudio_Pause();
		loop_SetVideoPlaybackMode();
		iV_SetFont(WFont);
		iV_SetTextColour(-1);
	}

	if (audioName != NULL)
	{
		ASSERT((strlen(audioName)<244,"sequence path+name greater than max string"));
		strcpy(aAudioName,"sequenceAudio\\");
		strcat(aAudioName,audioName);
	}


//for new timing
	frame = 0;
	videoFrameTime = GetTickCount();


	if(pie_GetRenderEngine() == ENGINE_GLIDE)
	{
//		p3DFXVideoBuffer = MALLOC(VIDEO_PLAYBACK_WIDTH * VIDEO_PLAYBACK_HEIGHT * sizeof(UWORD));
		if (p3DFXVideoBuffer != NULL)
		{
			for(i = 0; i < (VIDEO_PLAYBACK_WIDTH * VIDEO_PLAYBACK_HEIGHT); i++)
			{
				p3DFXVideoBuffer[i] = 0;
			}
		}
#ifdef INCLUDE_AUDIO
		if (!seq_SetSequenceForBuffer(aVideoName, VIDEO_3DFX_FULLSCREEN, audio_GetDirectSoundObj(), videoFrameTime + VIDEO_PLAYBACK_DELAY, NULL, perfMode))
#else
		if (!seq_SetSequenceForBuffer(aVideoName, VIDEO_3DFX_FULLSCREEN, NULL, videoFrameTime + VIDEO_PLAYBACK_DELAY, NULL, perfMode))
#endif
		{
#ifdef DUMMY_VIDEO
			if (seq_SetSequenceForBuffer("noVideo.rpl", VIDEO_3DFX_FULLSCREEN, NULL, videoFrameTime + VIDEO_PLAYBACK_DELAY, NULL, perfMode))
			{
				strcpy(aAudioName,"noVideo.wav");
				return TRUE;
			}
#endif
			seq_StopFullScreenVideo();
			DBERROR((FALSE,"Failed to initialise Escape video sequence %s",aVideoName));
			return FALSE;
		}
	}
	else
	{
#ifdef INCLUDE_AUDIO
		if (!seq_SetSequence(aVideoName,screenGetSurface(), audio_GetDirectSoundObj(), videoFrameTime + VIDEO_PLAYBACK_DELAY, pVideoBuffer, perfMode))
#else
		if (!seq_SetSequence(aVideoName,screenGetSurface(), NULL, videoFrameTime + VIDEO_PLAYBACK_DELAY, pVideoBuffer, perfMode))
#endif
		{
#ifdef DUMMY_VIDEO
			if (seq_SetSequence("noVideo.rpl",screenGetSurface(), NULL, videoFrameTime + VIDEO_PLAYBACK_DELAY, pVideoBuffer, perfMode))
			{
				strcpy(aAudioName,"noVideo.wav");
				return TRUE;
			}
#endif
			seq_StopFullScreenVideo();
//			ASSERT((FALSE,"seq_StartFullScreenVideo: unable to initialise sequence %s",aVideoName));
			return FALSE;
		}
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
		ASSERT((bAudioPlaying == TRUE,"seq_StartFullScreenVideo: unable to initialise sound %s",aAudioName));
	}

	return TRUE;
}

BOOL seq_UpdateFullScreenVideo(CLEAR_MODE *pbClear)
{
	SDWORD i, x, y, w, h;
	SDWORD	frame, frameLag, realFrame;
	SDWORD	subMin, subMax;
	int	videoTime;
	static int videoFrameTime = 0, textFrame = 0;
	LPDIRECTDRAWSURFACE4	lpDDSF;
	BOOL bMoreThanOneSequenceLine = FALSE;

	if (seq_GetCurrentFrame() == 0)
	{
//for new timing
		frame = 0;
		videoFrameTime = GetTickCount();
		textFrame = 0;
	}

	seq_GetFrameSize(&w,&h);

	x = (DISP_WIDTH - w)/2;
	y = (DISP_HEIGHT - h)/2;

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
			if (pie_GetRenderEngine() == ENGINE_GLIDE)
			{
				if ((realFrame >= aSeqList[currentPlaySeq].aText[i].endFrame + frameSkip) && (realFrame < (aSeqList[currentPlaySeq].aText[i].endFrame + frameSkip + frameSkip))) 
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

	if(pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		if (bHoldSeqForAudio == FALSE)
		{
			//poll the sequence player while timing the video
			videoTime = GetTickCount();
			while (videoTime < (videoFrameTime + RPL_FRAME_TIME * frameSkip))
			{
				videoTime = GetTickCount();
				seq_RefreshVideoBuffers();
			}
			frameLag = videoTime - videoFrameTime;
			frameLag /= RPL_FRAME_TIME;// if were running slow frame lag will be greater than 1
			videoFrameTime += frameLag * RPL_FRAME_TIME;//frame Lag should be 1 (most of the time)   
			//call sequence player to decode a frame
			lpDDSF = screenGetSurface();
			frame = seq_RenderOneFrameToBuffer((char*)p3DFXVideoBuffer, frameLag, subMin, subMax);//skip frame if behind
		}
		else
		{
			frame = seq_RenderOneFrameToBuffer((char*)p3DFXVideoBuffer, 0, 2, 0);//poll the video player
		}
		//3dfx blit the buffer to video
		pie_DownLoadBufferToScreen(p3DFXVideoBuffer,x,y,w,h,sizeof(UWORD)*w);
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
	else
	{
		if (bHoldSeqForAudio == FALSE)
		{
			if (perfMode != VIDEO_PERF_SKIP_FRAMES)
			{
				//version 1.00 release code
				//poll the sequence player while timing the video
				videoTime = GetTickCount();
				while (videoTime < (videoFrameTime + (RPL_FRAME_TIME * frameSkip)))
				{
					videoTime = GetTickCount();
					seq_RefreshVideoBuffers();
				}
				frameLag = videoTime - videoFrameTime;
				frameLag /= RPL_FRAME_TIME;// if were running slow frame lag will be greater than 1
				videoFrameTime += frameLag * RPL_FRAME_TIME;//frame Lag should be 1 (most of the time)   
				//call sequence player to decode a frame
				lpDDSF = screenGetSurface();
				frame = seq_RenderOneFrame(lpDDSF, frameLag, subMin, subMax);
			}
			else
			{
				//new version with timeing removed
				//poll the sequence player while timing the video
				videoTime = GetTickCount();
				while (videoTime < (videoFrameTime + (RPL_FRAME_TIME * frameSkip)))
				{
					videoTime = GetTickCount();
					seq_RefreshVideoBuffers();
				}
				videoFrameTime += frameSkip * RPL_FRAME_TIME;//frame Lag should be 1 (most of the time)   
				//call sequence player to decode a frame
				lpDDSF = screenGetSurface();
				frame = seq_RenderOneFrame(lpDDSF, frameSkip, subMin, subMax);

			}
		}
		else
		{
			//call sequence player to download last frame
			lpDDSF = screenGetSurface();
			frame = seq_RenderOneFrame(lpDDSF, 0, 2, 0);
		}
		//print any text over the video
		realFrame = textFrame + 1; 
		for(i=0;i<MAX_TEXT_OVERLAYS;i++)
		{
			if (aSeqList[currentPlaySeq].aText[i].pText[0] != 0)
			{
				if ((realFrame >= aSeqList[currentPlaySeq].aText[i].startFrame) && (realFrame <= aSeqList[currentPlaySeq].aText[i].endFrame))
				{
					lpDDSF = screenGetSurface();
					if (bMoreThanOneSequenceLine)
					{
						aSeqList[currentPlaySeq].aText[i].x = 20 + D_W;
					}
					pie_DrawTextToSurface(lpDDSF,&(aSeqList[currentPlaySeq].aText[i].pText[0]), 
							aSeqList[currentPlaySeq].aText[i].x, aSeqList[currentPlaySeq].aText[i].y);
				}
				else if (aSeqList[currentPlaySeq].bSeqLoop)//if its a looped video always draw the text
				{
					lpDDSF = screenGetSurface();
					if (bMoreThanOneSequenceLine)
					{
						aSeqList[currentPlaySeq].aText[i].x = 20 + D_W;
					}
					pie_DrawTextToSurface(lpDDSF,&(aSeqList[currentPlaySeq].aText[i].pText[0]), 
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
				if(pie_GetRenderEngine() == ENGINE_GLIDE)
				{
					if (!seq_SetSequenceForBuffer(aVideoName, VIDEO_3DFX_FULLSCREEN, NULL, GetTickCount() + VIDEO_PLAYBACK_DELAY, NULL, perfMode))
					{
						bHoldSeqForAudio = TRUE;
					}
				} 
				else
				{
					if (!seq_SetSequence(aVideoName,screenGetSurface(), NULL, GetTickCount() + VIDEO_PLAYBACK_DELAY, pVideoBuffer, perfMode))
					{
						bHoldSeqForAudio = TRUE;
					}
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
		DBPRINTF(("VIDEO FRAME ERROR %d\n",frame));
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
BOOL seq_AddTextForVideo(UBYTE* pText, SDWORD xOffset, SDWORD yOffset, SDWORD startFrame, SDWORD endFrame, SDWORD bJustify, UDWORD PSXSeqNumber)
{
	SDWORD sourceLength, currentLength;
	char* currentText;
	SDWORD justification;
static SDWORD lastX;

	iV_SetFont(WFont);

	ASSERT((aSeqList[currentSeq].currentText < MAX_TEXT_OVERLAYS,
		"seq_AddTextForVideo: too many text lines"));

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
		seq_AddTextForVideo(&pText[currentLength + 1], 0, 0, startFrame, endFrame, bJustify,0);
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

BOOL	seq_AddTextFromFile(STRING *pTextName, BOOL bJustify)
{
	UBYTE *pTextBuffer, *pCurrentLine, *pText;
	UDWORD fileSize;
//	HANDLE	fileHandle;
//	WIN32_FIND_DATA findData;
//	BOOL endOfFile = FALSE;
	SDWORD xOffset, yOffset, startFrame, endFrame;
	UBYTE* seps	= "\n";

	strcpy(aTextName,"sequenceAudio\\");
	strcat(aTextName,pTextName);
	
/*
	fileHandle = FindFirstFile(aTextName,&findData);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	FindClose(fileHandle);
*/	
	if (loadFileToBufferNoError(aTextName, DisplayBuffer, displayBufferSize, &fileSize) == FALSE)
	{
		return FALSE;
	}

	pTextBuffer = DisplayBuffer;
	pCurrentLine = strtok(pTextBuffer,seps);
	while(pCurrentLine != NULL)
	{
		if (*pCurrentLine != '/')
		{
			if (sscanf(pCurrentLine,"%d %d %d %d", &xOffset, &yOffset, &startFrame, &endFrame) == 4)
			{
				//get the text
				pText = strrchr(pCurrentLine,'"');
				ASSERT ((pText != NULL,"seq_AddTextFromFile error parsing text file"));
				if (pText != NULL)
				{
					*pText = (UBYTE)0;
				}
				pText = strchr(pCurrentLine,'"');
				ASSERT ((pText != NULL,"seq_AddTextFromFile error parsing text file"));
				if (pText != NULL)
				{
					seq_AddTextForVideo(&pText[1], xOffset, yOffset, startFrame, endFrame, bJustify,0);
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
void seq_AddSeqToList(STRING *pSeqName, STRING *pAudioName, STRING *pTextName, BOOL bLoop, UDWORD PSXSeqNumber)
{
	SDWORD strLen;
	currentSeq++;
	

	if ((currentSeq) >=  MAX_SEQ_LIST)
	{
		ASSERT((FALSE, "seq_AddSeqToList: too many sequences"));
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
		ASSERT((strLen < MAX_STR_LENGTH,"seq_AddSeqToList: sequence name error"));
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

void seqDispCDOK( void )
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
		bPlayedOK = seq_StartFullScreenVideo( aSeqList[currentPlaySeq].pSeq,
											  aSeqList[currentPlaySeq].pAudio );
	}

	if ( bPlayedOK == FALSE )
	{
        //don't do the callback if we're playing the win/lose video
        if (!getScriptWinLoseVideo())
        {
    		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
        }
        else
        {
            displayGameOver(getScriptWinLoseVideo() == PLAY_WIN);
        }
	}
}

void seqDispCDCancel( void )
{
}

/*returns the next sequence in the list to play*/
void seq_StartNextFullScreenVideo(void)
{
	if(bMultiPlayer)
	{
		seqDispCDOK();
		return;
	}

	/* check correct CD in drive */
	g_CDrequired = getCDForCampaign( getCampaignNumber() );
	if ( cdspan_CheckCDPresent( g_CDrequired ) )
	{
		seqDispCDOK();
	}
	else
	{
		/* check backdrop already up */
		if ( (void*)screen_GetBackDrop() == NULL )
		{
			bBackDropWasAlreadyUp = FALSE;
		}
		else
		{
			bBackDropWasAlreadyUp = TRUE;
		}
		intResetScreen(TRUE);
		forceHidePowerBar();
		intRemoveReticule();
		setDesignPauseState();
		if(!bMultiPlayer)
		{
			addCDChangeInterface( g_CDrequired, seqDispCDOK, seqDispCDCancel );
		}
		g_bResumeInGame = TRUE;
	}
}

void seq_SetSubtitles(BOOL bNewState)
{
	bSeqSubtitles = bNewState;
}

BOOL seq_GetSubtitles(void)
{
	return bSeqSubtitles;
}


/*play a video now and clear all other videos, front end use only*/
/*
BOOL seq_PlayVideo(char* pSeq, char* pAudio)
{
	seq_ClearSeqList();//other wise me might trigger these videos when we finish
	seq_StartFullScreenVideo(pSeq, pAudio);
	while (loop_GetVideoStatus())
	{
		videoLoop();
	}
	return TRUE;
}
*/
