/*
 * Cursor.c
 *
 * A thread based cursor implementation that allows a cursor to update faster
 * than the program framerate.
 *
 */

#pragma warning (disable : 4201 4214 4115 4514)
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <ddraw.h>
#pragma warning (default : 4201 4214 4115)

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "surface.h"
#include "screen.h"
#include "input.h"
#include "dderror.h"
#include "dxinput.h"
#include "frameint.h"
#include "cursor.h"

// The size of the cursor save buffer
#define CURSOR_SAVEWIDTH		32
#define CURSOR_SAVEHEIGHT		32

// The video memory surface for the cursor bitmap
static LPDIRECTDRAWSURFACE4		psCursorSurface;
// The video memory surface for the save buffer
static LPDIRECTDRAWSURFACE4		psCursorSave;

// The size and number of frames of the current cursor
static SDWORD	cursorWidth,cursorHeight,cursorFrames;

// The length of the animation if any
static SDWORD	frameTime;

// The thread handle and thread id
static HANDLE	hCursorThread;
static DWORD	cursorThreadID;

// Whether the thread should exit
static BOOL		cursorExitThread;
//static BOOL		cursorThreadRunning;

// Whether the save buffer contains valid data
static BOOL		saveValid;

// Location of save buffer
static SDWORD	saveX,saveY;

// Critical section to control access to the surface
static CRITICAL_SECTION		sSurfaceCritical;

// The cursor thread update routine
static DWORD WINAPI cursorThreadUpdate(LPVOID param);

// Initialise the cursor system, specifying the maximum size of cursor bitmap
BOOL cursorInitialise(SDWORD width, SDWORD height)
{
	// Create the cursor surface
	if (!surfCreate(&psCursorSurface, width,height,
				    DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
					screenGetBackBufferPixelFormat(), FALSE, TRUE))
	{
		DBERROR(("cursorInitialise: couldn't create surface"));
		return FALSE;
	}

	// Create the save buffer
	if (!surfCreate(&psCursorSave, CURSOR_SAVEWIDTH, CURSOR_SAVEHEIGHT,
				    DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
					screenGetBackBufferPixelFormat(), FALSE, TRUE))
	{
		DBERROR(("cursorInitialise: couldn't create surface"));
		return FALSE;
	}

	cursorWidth = 0;
	cursorHeight = 0;
	cursorFrames = 0;
	cursorExitThread = FALSE;
	saveValid = FALSE;

	// Initialise the surface critical section
	InitializeCriticalSection(&sSurfaceCritical);

	// Create the Thread
	hCursorThread = CreateThread(NULL,1024,
								 cursorThreadUpdate,NULL,
								 0,//CREATE_SUSPENDED,
								 &cursorThreadID);
	if (hCursorThread == NULL)
	{
		DBERROR(("cursorInitialise: couldn't create thread"));
		return FALSE;
	}
	if (!SetThreadPriority(hCursorThread, THREAD_PRIORITY_TIME_CRITICAL))
	{
		DBERROR(("cursorInitialise: couldn't set thread priority"));
		return FALSE;
	}

/*	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST))
	{
		DBERROR(("cursorInitialise: couldn't set thread priority"));
		return FALSE;
	}*/


	return TRUE;
}


// Shutdown the cursor system
void cursorShutDown(void)
{
	// Tell the thread to exit
	cursorExitThread = TRUE;

	// Wait for a max of 3 seconds for the thread to terminate
	WaitForSingleObjectEx(hCursorThread, 3000, FALSE);

	// Free the cursor surface
	surfRelease(psCursorSurface);
	surfRelease(psCursorSave);

	DeleteCriticalSection(&sSurfaceCritical);
}


// Load the cursor image data
BOOL cursorLoad(CURSOR_LOAD *psLoadData)
{
	// ensure the thread isn't accessing the surface
	EnterCriticalSection(&sSurfaceCritical);

	// Load the data into the surface
	if (!surfLoadFrom8Bit(psCursorSurface,
						  psLoadData->imgWidth, psLoadData->imgHeight,
						  psLoadData->pImageData, psLoadData->psPalette))
	{
		return FALSE;
	}

	cursorWidth = psLoadData->cursorWidth;
	cursorHeight = psLoadData->cursorHeight;
	cursorFrames = psLoadData->numFrames;
	frameTime = psLoadData->frameTime;

	// allow the thread to access the surface again
	LeaveCriticalSection(&sSurfaceCritical);

	return TRUE;
}


// Set the screen and cursor surface rectangles
static void setRects(RECT *psScreen, RECT *psCursor,
					 SDWORD x, SDWORD y,
					 SDWORD width, SDWORD height)
{
	SDWORD		diff;

	psScreen->left = x;
	psScreen->top = y;
	psScreen->right = x + width;
	psScreen->bottom = y + height;

	psCursor->left = 0;
	psCursor->top = 0;
	psCursor->right = width;
	psCursor->bottom = height;

	if (psScreen->right >= (SDWORD)screenWidth)
	{
		diff = psScreen->right - (SDWORD)screenWidth + 1;
		psScreen->right -= diff;
		psCursor->right -= diff;
	}
	if (psScreen->bottom >= (SDWORD)screenHeight)
	{
		diff = psScreen->bottom - (SDWORD)screenHeight + 1;
		psScreen->bottom -= diff;
		psCursor->bottom -= diff;
	}
}

SDWORD		threadCount, threadStart;

// The cursor thread update routine
static DWORD WINAPI cursorThreadUpdate(LPVOID param)
{
	HRESULT		ddrval;
	RECT		sSaveRect, sScreenRect, sCursorRect;
	SDWORD		mx,my,buttons;//, diff;
	DDBLTFX		sBltFX;
	SDWORD		waitRet;
	SDWORD		currFrame, currTime;

	param = param;

//	DBPRINTF(("cursorThread start\n"));
	threadCount = 0;
	threadStart = GetTickCount();
	currFrame = 0;

	while (!cursorExitThread)
	{
		DInpGetMouseState(&mx,&my,&buttons);
		sSaveRect.left = 0;
		sSaveRect.top = 0;
		sSaveRect.right = CURSOR_SAVEWIDTH;
		sSaveRect.bottom = CURSOR_SAVEHEIGHT;

//		SleepEx(13);
		waitRet = WaitForSingleObjectEx(hScreenFlipSemaphore, 15, FALSE);
		if (waitRet == WAIT_TIMEOUT)
		{
			// Make sure we don't get a flip while showing the cursor
			EnterCriticalSection(&sScreenFlipCritical);

			// Restore the save buffer if necessary
			if (saveValid && screenFlipState == FLIP_IDLE)
			{
				setRects(&sScreenRect,&sSaveRect, saveX,saveY,
								CURSOR_SAVEWIDTH, CURSOR_SAVEHEIGHT);
				ddrval = psFront->lpVtbl->Blt(
								psFront, &sScreenRect,
								psCursorSave, &sSaveRect,
								DDBLT_WAIT, NULL);
				ASSERT((ddrval == DD_OK,
					"cursorThread: save buffer restore failed:\n%s",
					DDErrorToString(ddrval)));
			}

			LeaveCriticalSection(&sScreenFlipCritical);
		}
		
		// reset the flip state
		if (screenFlipState == FLIP_FINISHED)
		{
			screenFlipState = FLIP_IDLE;
		}

		// Store the current cursor location
		setRects(&sScreenRect,&sSaveRect, mx,my,
						CURSOR_SAVEWIDTH, CURSOR_SAVEHEIGHT);
		if (sScreenRect.left != sScreenRect.right &&
			sScreenRect.top != sScreenRect.bottom)
		{
			ddrval = psCursorSave->lpVtbl->Blt(
						psCursorSave, &sSaveRect,
						psFront, &sScreenRect,
						DDBLT_WAIT, NULL);
			ASSERT((ddrval == DD_OK,
				"cursorThread: save buffer copy failed:\n%s",
				DDErrorToString(ddrval)));
			saveX = mx;
			saveY = my;
			saveValid = TRUE;
		}
		else
		{
			saveValid = FALSE;
		}

		// Blit the cursor
		EnterCriticalSection(&sSurfaceCritical);
		if (cursorFrames > 0)
		{
			setRects(&sScreenRect,&sCursorRect, mx,my,
							cursorWidth, cursorHeight);

			// Choose the right frame if there is an animation
			if (cursorFrames > 1)
			{
				currTime = GetTickCount();
				currTime -= threadStart;
				currFrame = (currTime/frameTime) % cursorFrames;
				sCursorRect.left += currFrame * cursorWidth;
				sCursorRect.right += currFrame * cursorWidth;
			}
			if (sScreenRect.left != sScreenRect.right &&
				sScreenRect.top != sScreenRect.bottom)
			{
				memset(&sBltFX, 0, sizeof(DDBLTFX));
				sBltFX.dwSize = sizeof(DDBLTFX);
				sBltFX.ddckSrcColorkey.dwColorSpaceLowValue = 0;
				sBltFX.ddckSrcColorkey.dwColorSpaceHighValue = 0;

				// Wait for the V-Blank
//				ddrval = psDD->lpVtbl->WaitForVerticalBlank(psDD,DDWAITVB_BLOCKBEGIN, NULL);

				ddrval = psFront->lpVtbl->Blt(
								psFront, &sScreenRect,
								psCursorSurface, &sCursorRect,
								DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &sBltFX);
				ASSERT((ddrval == DD_OK,
					"cursorThread: cursor blit failed:\n%s",
					DDErrorToString(ddrval)));
			}
		}
		LeaveCriticalSection(&sSurfaceCritical);

		threadCount += 1;
	}

//	DBPRINTF(("cursorThreadExit\n"));

	return 0;
}

// Start displaying the cursor
BOOL cursorDisplay(void)
{

	return TRUE;
}


// Stop displaying the cursor
BOOL cursorHide(void)
{
	return TRUE;
}


