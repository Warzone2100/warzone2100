
/*
 * Frame.c
 *
 * Initialisation and shutdown for the framework library.
 *
 * Includes a basic windows message loop.
 *
 */




// defines the inline functions in this module
#define DEFINE_INLINE

#pragma warning (disable : 4201 4214 4115 4514)
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#pragma warning (default : 4201 4214 4115)

#include <stdio.h>
#include <time.h>

// window focus messages 
//#define DEBUG_GROUP1
#include "frame.h"
#include "frameint.h"
#include "wdg.h"

#include "fractions.h"

#ifdef	 PSX
#include  "CtrlPSX.h"
#endif


#include <assert.h>

#define MAX_CURSORS 26 

typedef struct _cursor_resource
{
	WORD	resID;
	HCURSOR		hCursor;
} CURSOR_RESOURCE;


/* Program hInstance */
HINSTANCE	hInstance;

/* Handle for the main window */
HANDLE	hWndMain;

/* Are we running under glide? */
BOOL	bRunningUnderGlide = FALSE;


/* Flag if directdraw is active*/
static BOOL	bActiveDDraw;

// window class name
#define WINDOW_CLASS_NAME	"Framework"

/* Handle for the cursor */
static HCURSOR	hCursor;
/* Handle for the internal cursor */
static HCURSOR	hInternalCursor;

static WORD currentCursorResID = UWORD_MAX;
static SDWORD		nextCursor;
CURSOR_RESOURCE aCursors[MAX_CURSORS];

/* The default window function */
static DEFWINPROCTYPE frameWinProc;

/* Stores whether a windows quit message has been received */
static BOOL winQuit=FALSE;

typedef enum _focus_state
{
	FOCUS_OUT,		// Window does not have the focus
	FOCUS_SET,		// Just received WM_SETFOCUS
	FOCUS_IN,		// Window has got the focus
	FOCUS_KILL,		// Just received WM_KILLFOCUS
} FOCUS_STATE;

FOCUS_STATE		focusState, focusLast;

/* Whether the mouse is currently being displayed or not */
static BOOL mouseOn=TRUE;

/* Whether the mouse should be displayed in the app workspace */
static BOOL displayMouse=TRUE;

/* Graphics data for a default bitmap */
#define DEF_CURSOR_WIDTH	32
#define DEF_CURSOR_HEIGHT	32
#define DEF_CURSOR_X		0
#define DEF_CURSOR_Y		0
static UDWORD	aCursorData [DEF_CURSOR_HEIGHT] =
{
	0x00000000,
	0x00000000,
	0x00000040,
	0x00000060,
	0x00000070,
	0x00000078,
	0x0000007C,
	0x0000007E,
	0x0000007F,
	0x0000807F,
	0x0000007C,
	0x0000006C,
	0x00000046,
	0x00000006,
	0x00000003,
	0x00000003,
	0x00008001,
	0x00008001,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
static UDWORD	aCursorMask [DEF_CURSOR_HEIGHT] =
{
	0xffffff7f,
	0xffffff3f,
	0xffffff1f,
	0xffffff0f,
	0xffffff07,
	0xffffff03,
	0xffffff01,
	0xffffff00,
	0xffff7f00,
	0xffff3f00,
	0xffff1f00,
	0xffffff01,
	0xffffff10,
	0xffffff30,
	0xffff7f78,
	0xffff7ff8,
	0xffff3ffc,
	0xffff3ffc,
	0xffff7ffe,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
};

/************************************************************************************
 *
 * Alex's frame rate stuff
 */

/* Over how many seconds is the average required? */
#define	TIMESPAN	5

/* Initial filler value for the averages - arbitrary */
#define  IN_A_FRAME 70

/* Global variables for the frame rate stuff */
static SDWORD	FrameCounts[TIMESPAN];
static SDWORD	Frames;						// Number of frames elapsed since start
static SDWORD	LastFrames;					
static SDWORD	RecentFrames;				// Number of frames in last second
static SDWORD	PresSeconds;				// Number of seconds since execution started
static SDWORD	LastSeconds;
static SDWORD	FrameRate;					// Average frame rate since start
static SDWORD	FrameIndex;
static SDWORD	Total;
static SDWORD	RecentAverage;				// Average frame rate over last TIMSPAN seconds

/* InitFrameStuff - needs to be called once before frame loop commences */
static void	InitFrameStuff( void )
{
SDWORD i;

	for (i=0; i<TIMESPAN; i++)
		{
		FrameCounts[i] = IN_A_FRAME;
		}

	Frames = 0;
	LastFrames = 0;
	RecentFrames = 0;
	RecentAverage = 0;
	PresSeconds = 0;
	LastSeconds = 0;
	LastSeconds = PresSeconds;
	FrameIndex = 0;
}

/* MaintainFrameStuff - call this during completion of each frame loop */
static void	MaintainFrameStuff( void )
{
SDWORD i;
	
	PresSeconds = clock()/CLOCKS_PER_SEC;
	if (PresSeconds!=LastSeconds)
		{
		LastSeconds = PresSeconds;
		RecentFrames = Frames-LastFrames;
		LastFrames = Frames;
		FrameCounts[FrameIndex++] = RecentFrames;
		if (FrameIndex>=TIMESPAN)
		{
			FrameIndex = 0;
		}

		Total = 0;
		for (i=0; i<TIMESPAN; i++)
			{
			Total+=FrameCounts[i];
			}
		RecentAverage = Total/TIMESPAN;

		FrameRate = Frames / PresSeconds;
		}	
	Frames++;
}

/* Return the current frame rate */
UDWORD frameGetFrameRate(void)
{
	return RecentAverage;
}

/* Return the overall frame rate */
UDWORD frameGetOverallRate(void)
{
	return FrameRate;
}

/* Return the frame rate for the last second */
UDWORD frameGetRecentRate(void)
{
	return RecentFrames;
}

UDWORD	frameGetFrameNumber(void)
{
	return Frames;
}

/* Return the handle for the application window */
HANDLE	frameGetWinHandle(void)
{
	return hWndMain;
}

// string buffer for windows error string
static STRING winErrorString[255];

// Return a string for a windows error code
STRING *winErrorToString(SDWORD error)
{
#ifdef WIN32
	LPVOID lpMsgBuf;
 
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	// Copy the string.
	strncpy(winErrorString, lpMsgBuf, 254);
	winErrorString[255] = '0';

	// Free the buffer.
	LocalFree( lpMsgBuf );
#else
	winErrorString[0] = '0';
#endif

	return winErrorString;
}

/* If cursor on is TRUE the windows cursor will be displayed over the game window
 * (and in full screen mode).  If it is FALSE the cursor will not be displayed.
 */
void frameShowCursor(BOOL cursorOn)
{
	displayMouse = cursorOn;
}

/* Set the current cursor from a cursor handle */
void frameSetCursor(HCURSOR hNewCursor)
{
	if (hNewCursor == NULL)
	{
		ASSERT((FALSE,"frameSetCursor: NULL cursor handle"));
		return;
	}
	hCursor = hNewCursor;
	SetCursor(hCursor);
}

#ifdef WIN32	// not on PSX



/* Set the current cursor from a Resource ID */
void frameSetCursorFromRes(WORD resID)
{
	HCURSOR		hNewCursor = NULL;
	SDWORD		i;

	ASSERT((resID != 0, "frameSetCursorFromRes: null resource ID"));

	//If we are already using this cursor then  return
	if (resID != currentCursorResID)
	{
		//if its loaded then get cursor handle
		for(i = 0; (i < nextCursor && hNewCursor == NULL) ; i++)
		{
			if (aCursors[i].resID == resID)
			{
				hNewCursor = aCursors[i].hCursor;
			}
		}

		//if cursor wasnt loaded, load the cursor and add it to array
		if (hNewCursor == NULL)
		{

			ASSERT((nextCursor < MAX_CURSORS,"frameSetCursorFromRes: Attempting to load too many cursors\n"));

			if (nextCursor >= MAX_CURSORS)
			{
				nextCursor = MAX_CURSORS - 1;
			}
			hNewCursor = LoadCursor(hInstance, MAKEINTRESOURCE(resID));
			if (hNewCursor != NULL)
			{
				//store it
				aCursors[nextCursor].resID = resID;	
				aCursors[nextCursor].hCursor = hNewCursor;
				nextCursor++;
			}
		}

		ASSERT((hNewCursor != NULL, "frameSetCursorFromRes: LoadCursor failed:\n"));

		//if we got a new cursor set it
		if (hNewCursor != NULL)
		{
			frameSetCursor(hNewCursor);
			currentCursorResID = resID;
		}
	}
}




/*
 * Wndproc
 *
 * The windows message processing function.
 */
static long FAR PASCAL Wndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	SDWORD	res;

	if ((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST) ||
	    (message >= WM_KEYFIRST) && (message <= WM_KEYLAST))
	{
		inputProcessMessages(message, wParam, lParam);
		return 0;
	}
	else
	{
		switch( message )
		{
		case WM_SETFOCUS:
			DBP1(("WM_SETFOCUS\n"));
			if (focusState != FOCUS_IN)
			{
				DBP1(("FOCUS_SET\n"));
				focusState = FOCUS_SET;
				DInpMouseAcc(DINP_MOUSEACQUIRE);
			}
			return 0;
		case WM_KILLFOCUS:
			DBP1(("WM_KILLFOCUS\n"));
			if (focusState != FOCUS_OUT)
			{
				DBP1(("FOCUS_KILL\n"));
				focusState = FOCUS_KILL;
				DInpMouseAcc(DINP_MOUSERELEASE);
			}
			/* Have to tell the input system that we've lost focus */
			inputProcessMessages(message, wParam, lParam);
			return 0;
/* might need to catch this as well for direct input
	 - but I think it is covered by WM_ACTIVATEAPP

		case WM_ACTIVATE:
			res = LOWORD(wParam);
			if (res == WA_ACTIVE || res == WA_CLICKACTIVE)
			{
			}
			else
			{
			}
			break;*/
		case WM_ACTIVATEAPP:
			/* Surprise Surprise windows doesn't always sent a WM_SETFOCUS
			 * or WM_KILLFOCUS when the active application changes, so we've
			 * got to look at WM_ACTIVATEAPP as well.
			 */
			if ((BOOL)wParam)
			{
				/* We are being activated */
				DBP1(("WM_ACTIVATEAPP  TRUE\n"));
				PostMessage(hWndMain, WM_SETFOCUS, 0,0);
				if (bRunningUnderGlide)
				{
//				  	ShowWindow(hWndMain, SW_MAXIMIZE);
//				  	ShowWindow(hWndMain, SW_RESTORE);
//					ShowWindow(hWndMain, SW_SHOWMAXIMIZED);
//					UpdateWindow(hWndMain);
//					res = ShowOwnedPopups(hWndMain, TRUE);
					res = IsIconic(hWndMain);
					if (res)
					{
//						res = OpenIcon(hWndMain);
//						PostMessage(hWndMain, WM_SYSCOMMAND, SC_MAXIMIZE,0);
						ShowWindow(hWndMain, SW_SHOWMAXIMIZED);
					}
				}
			}
			else
			{
				/* We are being deactivated */
				DBP1(("WM_ACTIVATEAPP  FALSE\n"));
				PostMessage(hWndMain, WM_KILLFOCUS, 0,0);
				if (bRunningUnderGlide)
				{
//				  	ShowWindow(hWndMain, SW_MINIMIZE);
//					UpdateWindow(hWndMain);
//				  	ShowWindow(hWndMain, SW_HIDE);
//					res = ShowOwnedPopups(hWndMain, FALSE);
					res = IsIconic(hWndMain);
					if (!res)
					{
						CloseWindow(hWndMain);
					}
				}
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_SCREENSAVE)
			{
				return 0;
			}
			break;
		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT)
			{
				/* Turn off the cursor if necessary */
				if (!displayMouse && mouseOn)
				{
					res = ShowCursor(FALSE);
					if (res >= 0)
					{
						ASSERT((FALSE,"WM_SETCURSOR off: cursor count out of sync"));
						while (ShowCursor(FALSE) >= 0)
							;
					}
					mouseOn = FALSE;
				}
//				if (GetCursor() != hCursor)
				{
					SetCursor(hCursor);
				}
				return 0;
			}
			else if (LOWORD(lParam) == HTCAPTION)
			{
				if (!mouseOn)
				{
					res = ShowCursor(TRUE);
					if (res < 0)
					{
						ASSERT((FALSE,"WM_SETCURSOR on: cursor count out of sync"));
						while (ShowCursor(FALSE) < 0)
							;
					}
					mouseOn = TRUE;
				}
				SetCursor(hInternalCursor);
				return 0;
			}
			break;
		case WM_CLOSE:
			/* Request for the application to end */
			res = MessageBox(hWndMain, "Do you want to quit?", "Confirmation",
							 MB_ICONQUESTION | MB_YESNO);
			if (res == IDYES)
			{
				winQuit = TRUE;
			}
			return 0;
		case WM_DESTROY:
			/* Shut down the game and quit */
			winQuit = TRUE;
			return 0;
		case WM_SIZE:
			/* Just ignore this */
			return 0;
		case WM_ERASEBKGND:
			// Tell windows we have erased the background - cos we will when we draw next.
			return 1;
		default:
			break;
		}
	}

	/* Default behaviour for any messages not dealt with above */
	if (frameWinProc)
	{
	    return frameWinProc(hWnd, message, wParam, lParam);
	}

	// No extra window procedure set, use the default one
	return DefWindowProc(hWnd, message, wParam, lParam);
}


/* The default window procedure for the library.
 * This is initially set to the standard DefWindowProc, but can be changed
 * by this function
 */
extern void frameSetWindowProc(DEFWINPROCTYPE winProc)
{
	frameWinProc = winProc;
}


/*
 * winInitApp
 *
 * Do that Windows initialization thang...
 */


static BOOL WinInitGlide(HANDLE hInstance, char *name, int width, int height, BOOL maximize)
{
	WNDCLASS cls;
//	HWND hWndMainGlide;	// note that this doesn't go anywhere....

	cls.hInstance     = hInstance;//GetModuleHandle(NULL);
	cls.hIcon         = LoadIcon(cls.hInstance, IDI_APPLICATION);
	cls.hCursor       = LoadCursor(cls.hInstance, IDC_ARROW);
	cls.lpszMenuName  = NULL;
	cls.lpszClassName = WINDOW_CLASS_NAME;
	cls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);//GetStockObject(BLACK_BRUSH);
	cls.style         = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	cls.lpfnWndProc   = Wndproc;
	cls.cbClsExtra    = 0;
	cls.cbWndExtra    = 0;

	bRunningUnderGlide = TRUE;

	if (!RegisterClass(&cls))
	{
		return FALSE;
	}
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);
	// Initialise the window to be huge so it always covers the whole screen
#ifdef DEBUG
	hWndMain = CreateWindowEx(WS_EX_APPWINDOW, "Framework", name, WS_POPUP, 0, 0, width, height, NULL, NULL, cls.hInstance, NULL);
#else
	hWndMain = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_TOPMOST, "Framework", name, WS_POPUP, 0, 0, width, height, NULL, NULL, cls.hInstance, NULL);
#endif
//	width; height;
//	hWndMain = CreateWindowEx(WS_EX_APPWINDOW, "Framework", name, WS_POPUP, 0, 0, SDWORD_MAX, SDWORD_MAX, NULL, NULL, cls.hInstance, NULL);
	if(!hWndMain)
	{
		DBERROR(("Whoa! Cannot create the main window for the glide version"));
		return(FALSE);
	}

	ShowCursor(TRUE);
  	ShowWindow(hWndMain, maximize ? SW_MAXIMIZE : SW_NORMAL);
	UpdateWindow(hWndMain);

	/* Store the default window procedure */
	frameWinProc = NULL;

	return TRUE;
}

static BOOL winInitApp(HANDLE hInstance,	// Instance handle for the program
				STRING *pWindowName,	// The text to put on the window title bar
				UDWORD width,			// The window width
				UDWORD height)			// The window height
{
    WNDCLASS    wc;
    BOOL        rc;
	RECT		sWinSize;
	STRING		*pMsgBuf;

	/* Create the default cursor for the app - a simple arrow */
	hInternalCursor = CreateCursor(
				hInstance,
				DEF_CURSOR_X, DEF_CURSOR_Y,
				DEF_CURSOR_WIDTH,DEF_CURSOR_HEIGHT,
				aCursorMask, aCursorData);
	if (hInternalCursor == NULL)
	{
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &pMsgBuf,
			0,
			NULL);
		DBERROR(("Create Cursor failed:\n%s", pMsgBuf));
		// Free the message buffer.
		LocalFree( pMsgBuf );

		return FALSE;
	}
	hCursor = hInternalCursor;

	/* Create a windows class for the application */
    wc.style = CS_DBLCLKS |		// Want to get double click messages
			   CS_PARENTDC;		// Single DC for the window - should speed things
								// up a bit
    wc.lpfnWndProc = Wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( NULL, IDI_APPLICATION);
    wc.hCursor = NULL; //hCursor;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); //(COLOR_WINDOW+1); //GetStockObject( WHITE_BRUSH );
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    rc = RegisterClass( &wc );
    if( !rc )
    {
		DBERROR(("Failed to register windows class"));
        return FALSE;
    }
	
	/* Get the actual size of window we want (including the size of
	   title bars etc.) */
	(void)SetRect(&sWinSize, 0, 0, width, height);
	(void)AdjustWindowRectEx(&sWinSize,
					   WIN_STYLE,
                       FALSE,
                       WIN_EXSTYLE);

	/* The rectangle returned has values for the window edges relative to
	   the display area origin, i.e. left and top are negative - so we have
	   to adjust */
	sWinSize.right -= sWinSize.left;
	sWinSize.left = 0;
	sWinSize.bottom -= sWinSize.top;
	sWinSize.top = 0;
	
	/* Create the main window */
    hWndMain = CreateWindowEx(
		WIN_EXSTYLE,		// Extended window style, defined in WinMain.h
        "Framework",
        pWindowName,
		WIN_STYLE,			// Window style, defined in WinMain.h
        0, 0,				// Initial window location
        sWinSize.right,
		sWinSize.bottom,	// Initial window size
        NULL,
        NULL,
        hInstance,
        NULL );

    if( !hWndMain )
    {
		DBERROR(("Couldn't create main window."));
        return FALSE;
    }

	/* Store the default window procedure */
	frameWinProc = NULL;

    return TRUE;

} 


/*
 * frameInitialise
 *
 * Initialise the framework library. - PC version
 */
BOOL frameInitialise(HANDLE hInst,			// The windows application instance
					 STRING *pWindowName,	// The text to appear in the window title bar
					 UDWORD	width,			// The display width
					 UDWORD height,			// The display height
					 UDWORD bitDepth,		// The display bit depth
					 BOOL	fullScreen,		// Whether to start full screen or windowed
					 BOOL	bVidMem,	 	// Whether to put surfaces in video memory
					 BOOL	bGlide )		// Whether to create surfaces
{
	HWND	hWndPrev;

	/* exit if existing window with pWindowName name (i.e. only run one version) */
	if ( (hWndPrev = FindWindow( WINDOW_CLASS_NAME, pWindowName )) != NULL )
	{
		SetForegroundWindow( hWndPrev );
		return FALSE;
	}

	winQuit = FALSE;
	focusState = FOCUS_IN;
	focusLast = FOCUS_IN;
	if(!bGlide)
	{
		mouseOn = TRUE;
		displayMouse = TRUE;
	}
	else
	{
		mouseOn = FALSE;
		displayMouse = FALSE;
	}
	hInstance = hInst;
	bActiveDDraw = !bGlide;

//	/* Initialise the memory system */
//	if (!memInitialise())
//	{
//		return FALSE;
//	}

//	if (!blkInitialise())
//	{
//		return FALSE;
//	}

	/* Initialise the trig stuff */
	if (!trigInitialise())
	{
		return FALSE;
	}
//#ifdef ALEXM
	if(bGlide)
	{
		/* Initialise the windows stuff for Glide and open a window */
		if (!WinInitGlide(hInstance,pWindowName, width, height,TRUE))
		{
			return FALSE;
		}
		
	}
	else
	/* Initialise the windows stuff and open a window */
	if (!winInitApp(hInstance, pWindowName, width, height))
	{
		return FALSE;
	}

	if(bGlide)		  // Don't do this for Glide.
	{
		(void) screenInitialiseGlide(width,height,hWndMain);
	}
	else
	{
		/* Initialise the Direct Draw Buffers */
		if (!screenInitialise(width, height, bitDepth, fullScreen, bVidMem, bActiveDDraw, hWndMain))
		{
			return FALSE;							
		}
	}
//#else
#if 0
   	/* Initialise the windows stuff and open a window */
	if (!winInitApp(hInstance, pWindowName, width, height))
	{
		return FALSE;
	}

  	/* Initialise the Direct Draw Buffers */
	if (!screenInitialise(width, height, bitDepth, fullScreen, bVidMem, bActiveDDraw, hWndMain))
	{
		return FALSE;							
	}
#endif
	/* Initialise the input system */
	inputInitialise();
	/* Initialise the frame rate stuff */
	InitFrameStuff();



	// Initialise the resource stuff
	if (!resInitialise())
	{
		return FALSE;
	}

	dbg_SetMessageBoxCallback(NULL);
	dbg_SetErrorBoxCallback(NULL);
	dbg_SetAssertCallback(NULL);

	return TRUE;
}

/*
 * frameUpdate
 *
 * Call this each cycle to allow the framework to deal with
 * windows messages, and do general house keeping.
 *
 * Returns FRAME_STATUS.
 */
FRAME_STATUS frameUpdate(void)
{
	MSG				sMsg;
	FRAME_STATUS	retVal;

	/* Tell the input system about the start of another frame */
	inputNewFrame();

	/* Deal with any windows messages */
	while ( PeekMessage( &sMsg, NULL, 0, 0, PM_REMOVE ) )
	{
		if (sMsg.message == WM_QUIT)
		{
			break;
		}
		TranslateMessage(&sMsg);
		(void)DispatchMessage(&sMsg);
	}

	/* Now figure out what to return */
	retVal = FRAME_OK;
	if (winQuit)
	{
		retVal = FRAME_QUIT;
	}
	else if ((focusState == FOCUS_SET) && (focusLast == FOCUS_OUT))
	{
		DBP1(("Returning SETFOCUS\n"));
		focusState = FOCUS_IN;
		retVal = FRAME_SETFOCUS;
	}
	else if ((focusState == FOCUS_KILL) && (focusLast == FOCUS_IN))
	{
		DBP1(("Returning KILLFOCUS\n"));
		focusState = FOCUS_OUT;
		retVal = FRAME_KILLFOCUS;
	}

	if ((focusState == FOCUS_SET) || (focusState == FOCUS_KILL))
	{
		/* Got a SET or KILL when we were already in or out of
		   focus respectively */
		focusState = focusLast;
	}
	else if (focusLast != focusState)
	{
		DBP1(("focusLast changing from %d to %d\n", focusLast, focusState));
		focusLast = focusState;
	}

	/* If things are running normally, restore the surfaces and update the framerate */
	if ((!winQuit) && (focusState == FOCUS_IN))
	{
		/* Restore any surfaces that have been lost */
		if (bActiveDDraw)
		{
			screenRestoreSurfaces();
		}

		/* Update the frame rate stuff */
		MaintainFrameStuff();
	}

	return retVal;
}



void frameShutDown(void)
{
	if (bActiveDDraw)
	{
		surfShutDown();

		screenShutDown();
	}
	else
	{
		RELEASE(psDD);
	}

	/* Free the default cursor */
	DestroyCursor(hCursor);

	/* Destroy the Application window */
	DestroyWindow(hWndMain);

	/* shutdown the trig stuff */
	trigShutDown();

	// Shutdown the resource stuff
	resShutDown();

	// shutdown the block memory heap
	blkShutDown();

	/* Shutdown the memory system */
	memShutDown();

	//unregister the windows class (incase we want to recreate it
    UnregisterClass(WINDOW_CLASS_NAME, hInstance);


}

#else // ifdef WIN32 - here are the PSX version of the routines


void frameSetCursorFromRes(WORD resID)
{
//	SetPSXCursorFrame(resID);
}

/*
 * frameInitialise
 *
 * Initialise the framework library. - PSX Version
 */
BOOL frameInitialise(HANDLE hInst,			// The windows application instance
					 STRING *pWindowName,	// The text to appear in the window title bar
					 UDWORD	width,			// The display width
					 UDWORD height,			// The display height
					 UDWORD bitDepth,		// The display bit depth
					 BOOL	fullScreen,		// Whether to start full screen or windowed
					 BOOL	bVidMem,	 	// Whether to put surfaces in video memory
					 BOOL	bGlide )		// Whether to create surfaces
{
	winQuit = FALSE;
	focusState = FOCUS_IN;
	focusLast = FOCUS_IN;
	mouseOn = TRUE;
	displayMouse = TRUE;
	hInstance = hInst;

	if (!memInitialise())
	{
		return FALSE;
	}


	if (!blkInitialise())
	{
		return FALSE;
	}

	/* Initialise the windows stuff and open a window - windows stuff */
//	if (!winInitApp(hInstance, pWindowName, width, height))
//	{
//		return FALSE;
//	}



	/* Initialise the Direct Draw Buffers */
	if (!screenInitialise(width, height, bitDepth, fullScreen, hWndMain))
	{
		return FALSE;
	}

	/* Initialise the input system */
//	inputInitialise();

	/* Initialise the frame rate stuff */
	InitFrameStuff();


	/* Initialise the trig stuff ... must go after the PSX hard init (uses SQRT)*/
	if (!trigInitialise())
	{
		return FALSE;
	}

	// Initialise the resource stuff
	if (!resInitialise())
	{
		return FALSE;
	}




	return TRUE;
}





/*
 * frameUpdate
 *
 * Call this each cycle to allow the framework to deal with
 * windows messages, and do general house keeping.
 *
 * Returns FRAME_STATUS.
 */
FRAME_STATUS frameUpdate(void)
{
	/* Tell the input system about the start of another frame */
	inputNewFrame();

	MaintainFrameStuff();

	return NULL;
}



void frameShutDown(void)
{
}



#endif

BOOL loadFile(STRING *pFileName, UBYTE **ppFileData, UDWORD *pFileSize)
{
	return(loadFile2(pFileName,ppFileData,pFileSize,TRUE));
}


/* Load the file with name pointed to by pFileName into a memory buffer. */
// if allocate mem is true then the memory is allocated ... else it is already in ppFileData, and the max size is in pFileSize ... this is adjusted to the actual loaded file size
//   
BOOL loadFile2(STRING *pFileName, UBYTE **ppFileData, UDWORD *pFileSize, BOOL AllocateMem )
{
	FILE	*pFileHandle;
	UDWORD FileSize;

	BOOL res;

	// First we try to see if we can load the file from the freedata section of the current WDG

	
	res=loadFileFromWDG(pFileName, ppFileData, pFileSize,WDG_ALLOCATEMEM);	// loaded from WDG file, and allocate memory for it
	if (res==TRUE)	return TRUE;

	// Not in WDG so we try to load it the old fashion way !
	// This will never work on the final build of the PSX because we can *ONLY* load files
	// directly from CD, i.e. from the WDG's normal fopen/fread calls will never work!
#ifdef DEBUG
	DBPRINTF(("FOPEN ! %s\n",pFileName));
 #ifdef PSX_USECD
	assert(2+2==5);		// no fopens when using CD code !!!
 #endif

#endif


#ifdef FINALBUILD
	return FALSE;
#endif

// Not needed in a PSX FINALBUILD.
#if defined(WIN32) || !defined(FINALBUILD)
	pFileHandle = fopen(pFileName, "rb");
	if (pFileHandle == NULL)
	{
		DBERROR(("Couldn't open %s", pFileName));
		return FALSE;
	}

	/* Get the length of the file */
	if (fseek(pFileHandle, 0, SEEK_END) != 0)
	{
		DBERROR(("SEEK_END failed for %s", pFileName));
		return FALSE;
	}
	FileSize = ftell(pFileHandle);
	if (fseek(pFileHandle, 0, SEEK_SET) != 0)
	{
		DBERROR(("SEEK_SET failed for %s", pFileName));
		return FALSE;
	}


	if (AllocateMem==TRUE)
	{
		/* Allocate a buffer to store the data and a terminating zero */
// we don't want this popping up in the tools (makewdg)
//		DBPRINTF(("#############FILELOAD MALLOC - size=%d\n",(FileSize)+1));
		*ppFileData = (UBYTE *)MALLOC((FileSize) + 1);
		if (*ppFileData == NULL)
		{
			DBERROR(("Out of memory"));
			return FALSE;
		}
	}
	else
	{
		if (FileSize > *pFileSize)
		{
			DBERROR(("no room for file"));
			return FALSE;
		}
		assert(*ppFileData!=NULL);
	}
	/* Load the file data */
	if (fread(*ppFileData, 1, FileSize, pFileHandle) != FileSize)
	{
		DBERROR(("Read failed for %s", pFileName));
		return FALSE;
	}

	if (fclose(pFileHandle) != 0)
	{
		DBERROR(("Close failed for %s", pFileName));
		return FALSE;
	}

	// Add the terminating zero
	*((*ppFileData) + FileSize) = 0;
	*pFileSize=FileSize;	// always set to correct size
#endif
	return TRUE;
}

#ifdef WIN32

// load a file from disk into a fixed memory buffer
BOOL loadFileToBuffer(STRING *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	HANDLE	hFile;
	DWORD	bytesRead;
	BOOL	retVal;
	
	BOOL res;
	UDWORD FileSize;

	res=loadFileFromWDG(pFileName, &pFileBuffer, &FileSize,WDG_USESUPPLIED);	// loaded from WDG file, and allocate memory for it	if (res==TRUE)	return TRUE;
	if (res==TRUE)
	{
		*pSize=FileSize;
		return TRUE;
	}


	// try and open the file
	hFile = CreateFile(pFileName,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DBERROR(("Couldn't open %s\n%s",
			pFileName, winErrorToString(GetLastError())));
		return FALSE;
	}

	// get the size of the file
	*pSize = GetFileSize(hFile, NULL);
	if (*pSize >= (UDWORD)bufferSize)
	{
		DBERROR(("file too big !!:%s size %d\n", pFileName, *pSize));
		return FALSE;
	}

	// load the file into the buffer
	retVal = ReadFile(hFile, pFileBuffer, *pSize, &bytesRead, NULL);
	if (!retVal || *pSize != bytesRead)
	{
		DBERROR(("Couldn't read data from %s\n%s",
			pFileName, winErrorToString(GetLastError())));
		return FALSE;
	}
	pFileBuffer[*pSize] = 0;

	retVal = CloseHandle(hFile);
	if (!retVal)
	{
		DBERROR(("Couldn't close %s\n%s",
			pFileName, winErrorToString(GetLastError())));
		return FALSE;
	}

	return TRUE;
}

// as above but returns quietly if no file found
BOOL loadFileToBufferNoError(STRING *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	HANDLE	hFile;
	DWORD	bytesRead;
	BOOL	retVal;
	
	BOOL res;
	UDWORD FileSize;

	res=loadFileFromWDG(pFileName, &pFileBuffer, &FileSize,WDG_USESUPPLIED);	// loaded from WDG file, and allocate memory for it	if (res==TRUE)	return TRUE;
	if (res==TRUE)
	{
		*pSize=FileSize;
		return TRUE;
	}


	// try and open the file
	hFile = CreateFile(pFileName,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	// get the size of the file
	*pSize = GetFileSize(hFile, NULL);
	if (*pSize >= (UDWORD)bufferSize)
	{
		return FALSE;
	}

	// load the file into the buffer
	retVal = ReadFile(hFile, pFileBuffer, *pSize, &bytesRead, NULL);
	if (!retVal || *pSize != bytesRead)
	{
		return FALSE;
	}
	pFileBuffer[*pSize] = 0;

	retVal = CloseHandle(hFile);
	if (!retVal)
	{
		return FALSE;
	}

	return TRUE;
}
#endif



/* Save the data in the buffer into the given file */
BOOL saveFile(STRING *pFileName, UBYTE *pFileData, UDWORD fileSize)
{
	FILE	*pFile;

	/* open the file */
	pFile = fopen(pFileName, "wb");
	if (!pFile)
	{
		DBERROR(("Couldn't open %s", pFileName));
		return FALSE;
	}

	if (fwrite(pFileData, fileSize, 1, pFile) != 1)
	{
#ifdef WIN32	// ffs
		DBERROR(("Write failed for %s: %s", pFileName, winErrorToString(GetLastError()) ));
#else
		DBERROR(("Write failed for %s", pFileName ));
#endif	
		return FALSE;
	}

	if (fclose(pFile) != 0)
	{
		DBERROR(("Close failed for %s", pFileName));
		return FALSE;
	}
	
	return TRUE;
}





static UBYTE *WDGCacheStart=NULL;
static UDWORD WDGCacheSize=0;


#define MAXWDGDIRSIZE (7300)

static UBYTE WDGDirectory[MAXWDGDIRSIZE];	// Directory for current WDG file (MALLOCed)

static SDWORD WDGCacheStartPos=-1;
static SDWORD WDGCacheEndPos=-1;


/* next four used in HashPJW */
#define	BITS_IN_int		32
#define	THREE_QUARTERS	((UINT) ((BITS_IN_int * 3) / 4))
#define	ONE_EIGHTH		((UINT) (BITS_IN_int / 8))
#define	HIGH_BITS		( ~((UINT)(~0) >> ONE_EIGHTH ))      

//#define	HIGH_BITS		((UINT)(0xf0000000))
//#define	LOW_BITS		((UINT)(0x0fffffff))




///////////////////////////////////////////////////////////////////







/***************************************************************************/
/*
 * HashString
 *
 * Adaptation of Peter Weinberger's (PJW) generic hashing algorithm listed
 * in Binstock+Rex, "Practical Algorithms" p 69.
 *
 * Accepts string and returns hashed integer.
 */
/***************************************************************************/
UINT HashString( char *String )
{
	UINT	iHashValue, i;
	CHAR	*c = (CHAR *) String;

	assert(String!=NULL);
	assert(*String!=0x0);

	for ( iHashValue=0; *c; ++c )
	{
		iHashValue = ( iHashValue << ONE_EIGHTH ) + *c;

		if ( (i = iHashValue & HIGH_BITS) != 0 )
		{
			iHashValue = ( iHashValue ^ ( i >> THREE_QUARTERS ) ) &
							~HIGH_BITS;
		}
	}

	return iHashValue;
}

UINT HashStringIgnoreCase( char *String )
{
	UINT	iHashValue, i;
	CHAR	*c = (CHAR *) String;

	assert(String!=NULL);
	assert(*String!=0x0);

	for ( iHashValue=0; *c; ++c )
	{
		iHashValue = ( iHashValue << ONE_EIGHTH ) + ((*c)&(0xdf));

		if ( (i = iHashValue & HIGH_BITS) != 0 )
		{
			iHashValue = ( iHashValue ^ ( i >> THREE_QUARTERS ) ) &
							~HIGH_BITS;
		}
	}

	return iHashValue;
}



// Examine a filename for the last dot and slash
// and so giving the extension of the file and the directory
//
// PosOfDot and/of PosOfSlash can be NULL and then nothing will be stored
//
void ScanFilename(char *Fullname, int *PosOfDot, int *PosOfSlash)
{
	int Namelength;
	
	int DotPos=-1;
	int SlashPos=-1;
	int Pos;

	Namelength=strlen(Fullname);

	for (Pos=Namelength;Pos>=0;Pos--)
	{
		if (Fullname[Pos]=='.')
		{
			DotPos=Pos;
			break;
		}
	}

	for (Pos=Namelength;Pos>=0;Pos--)
	{
		if (Fullname[Pos]=='\\')
		{
			SlashPos=Pos;
			break;
		}
	}

	if (PosOfDot!=NULL)
		*PosOfDot=DotPos;

	if (PosOfSlash!=NULL)
		*PosOfSlash=SlashPos;
}

#ifdef DEBUG

SDWORD PercentFunc(char *File,UDWORD Line,SDWORD a,SDWORD b)
{
	if(b) {
		return (a*100)/b;
	}
	DBPRINTF(("Divide by 0 (PERCENT) in %s,line %d\n",File,Line));

	return 100;
}


SDWORD PerNumFunc(char *File,UDWORD Line,SDWORD range,SDWORD a,SDWORD b)
{
	if(b) {
		return (a*range)/b;
	}

	DBPRINTF(("Divide by 0 (PERNUM) in %s,line %d\n",File,Line));

	return range;
}

#endif
