/***************************************************************************/
/*
 * Sequence.c
 *
 * Sequence setup and video control
 *
 * based on eidos example code
 *
 *
 */
/***************************************************************************/

// Standard include file
#include "frame.h"
#include "sequence.h"

#ifdef WIN32
// Direct Draw and Sound Include files
#include <ddraw.h>
#define SEQUENCE_SOUND
#ifdef SEQUENCE_SOUND
	#include <dsound.h>
#endif
#endif

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

//buffer render for software_window 3DFX_window and 3DFX_fullscreen modes
// SOFT_WINDOWED video size 16bit rgb 555 mode (convert to 8bit from the buffer)
// D3D_WINDOWED video size 16bit uses screen pixel mode
// 3DFX_WINDOWED video size 16bit BGR 565 mode
// 3DFX_FULLSCREEN 640 * 480 BGR 565 mode
BOOL seq_SetSequenceForBuffer(char* filename, VIDEO_MODE mode, LPDIRECTSOUND lpDS, int startTime, DDPIXELFORMAT	*DDPixelFormat, PERF_MODE perfMode)
{
	return TRUE;

}

/*
 * directX fullscreeen render uses local buffer to store previous frame data
 * directX 640 * 480 16bit rgb mode render through local buffer to back buffer
 */
BOOL seq_SetSequence(char* filename, LPDIRECTDRAWSURFACE4 lpDDSF, LPDIRECTSOUND lpDS, int startTime, char* lpBF, PERF_MODE perfMode)
{
	return TRUE;

}

int seq_ClearMovie(void)
{
	return TRUE;
}


/*
 * buffer render for software_window 3DFX_window and 3DFX_fullscreen modes
 * SOFT_WINDOWED video size 16bit rgb 555 mode (convert to 8bit from the buffer)
 * 3DFX_WINDOWED video size 16bit BGR 565 mode
 * 3DFX_FULLSCREEN 640 * 480 BGR 565 mode
 */
int	seq_RenderOneFrameToBuffer(char *lpSF, int skip, SDWORD subMin, SDWORD subMax)
{
	return VIDEO_FINISHED;
}


/*
 * render one frame to a direct draw surface (normally the back buffer)
 * directX 640 * 480 16bit rgb mode render through local buffer to back buffer
 */
int	seq_RenderOneFrame(LPDIRECTDRAWSURFACE4	lpDDSF, int skip, SDWORD subMin, SDWORD subMax)
{
	return VIDEO_FINISHED;
}

BOOL	seq_RefreshVideoBuffers(void)
{
	return TRUE;
}

BOOL	seq_ShutDown(void)
{
	return TRUE;
}

BOOL	seq_GetFrameSize(SDWORD *pWidth, SDWORD* pHeight)
{
	*pWidth = 0;
	*pHeight = 0;
	return FALSE;
}

int seq_GetCurrentFrame(void)
{
	return -1;
}

int seq_GetFrameTimeInClicks(void)
{
	return 40;

}

int seq_GetTotalFrames(void)
{
	return -1;
}

