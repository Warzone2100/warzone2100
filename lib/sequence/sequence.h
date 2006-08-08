/***************************************************************************/
/*
 * sequence.h
 *
 * video streaming to game surfaces.
 *
 */
/***************************************************************************/

#ifndef _sequence_h
#define _sequence_h

/***************************************************************************/



/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

typedef enum _video_mode
{
	VIDEO_SOFT_WINDOW,			// video size 16bit rgb 555 mode (convert to 8bit from the buffer)
	VIDEO_SOFT_FULLSCREEN,		// directX 640 * 480 16bit rgb mode render through local buffer to back buffer
	VIDEO_D3D_WINDOW,			// video 16bit screen pixel mode
	VIDEO_D3D_FULLSCREEN,		// 640 * 480 screen pixel mode
	VIDEO_3DFX_WINDOW,			// video 16bit BGR 565 mode
	VIDEO_3DFX_FULLSCREEN		// 640 * 480 BGR 565 mode
} VIDEO_MODE;

typedef enum _perf_mode
{
	VIDEO_PERF_FULLSCREEN,		// Normal alternate line video
	VIDEO_PERF_WINDOW,			// 320 240 centred
	VIDEO_PERF_SKIP_FRAMES		// 320 240 centred and display every 4th frame
} PERF_MODE;

#define MAX_BAD_DATA 20

#define FRAMES_TO_SKIP 4

#define VIDEO_FINISHED -1
#define VIDEO_FRAME_WAIT -2
#define VIDEO_FRAME_ERROR -3
#define VIDEO_SOUND_ERROR -4
#define VIDEO_SURFACE_ERROR -5
/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
//buffer render for software_window 3DFX_window and 3DFX_fullscreen modes
extern BOOL	seq_SetSequenceForBuffer(char* filename, VIDEO_MODE mode, LPDIRECTSOUND	lpDS, int startTime, PERF_MODE perfMode);
extern int	seq_RenderOneFrameToBuffer(char* lpSF, int skip, SDWORD boxMin, SDWORD boxMax);

//directX fullscreeen render uses local buffer to store previous frame data
extern BOOL	seq_SetSequence(char* filename, LPDIRECTDRAWSURFACE4	lpDDSF, LPDIRECTSOUND lpDS, int startTime, char* lpBF, PERF_MODE perfMode);
extern int	seq_RenderOneFrame(LPDIRECTDRAWSURFACE4	lpDDSF, int skip, SDWORD boxMin, SDWORD boxMax);

extern int seq_ClearMovie(void);

//setup monitoring and control
extern BOOL	seq_RefreshVideoBuffers(void);
extern BOOL	seq_ShutDown(void);
extern BOOL	seq_GetFrameSize(SDWORD *pWidth, SDWORD* pHeight);
extern int	seq_GetCurrentFrame(void);
extern int	seq_GetFrameTimeInClicks(void);
extern int	seq_GetTotalFrames(void);
//extern int	seq_ResetMovie(void);

#endif // _sequence_h
