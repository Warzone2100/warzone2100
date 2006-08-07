
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

#include <stdio.h>
#include <time.h>
#include <SDL/SDL.h>
#include <physfs.h>
#include "ignorecase.h"

// window focus messages
//#define DEBUG_GROUP1
#include "frame.h"
#include "frameint.h"

#include "fractions.h"
#include <assert.h>
#include "cursors.h"

#include "SDL_framerate.h"

#define IGNORE_FOCUS

/* Linux specific stuff */

static FPSmanager wzFPSmanager;


char *unix_path(const char *path)
{
	static char returnval[512];
	unsigned int i;

	for (i = 0; path[i] != '\0'; ++i) {
		if (path[i] >= 'A' && path[i] <= 'Z') {
			returnval[i] = path[i]-'A'+'a';
		} else if (path[i] == '\\') {
			returnval[i] = '/';
		} else {
			returnval[i] = path[i];
		}
	}
	for(;returnval[i-1] == '/'; --i);
	returnval[i] = '\0';

	return returnval;
}

#ifndef WIN32

FILE *unix_fopen(const char *filename, const char *mode)
{
// ridiculous kludge because we redefine fopen to unix_fopen
#undef fopen
	return fopen(unix_path(filename), mode);
#define fopen unix_fopen
}

#endif



/* Handle for the main window */
HANDLE	hWndMain;

/* Program hInstance */
HINSTANCE       hInstance;

/* Flag if directdraw is active*/
static BOOL	bActiveDDraw;

static WORD currentCursorResID = UWORD_MAX;
SDL_Cursor *aCursors[MAX_CURSORS];


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

		if (PresSeconds > 0)
			FrameRate = Frames / PresSeconds;
		}
	Frames++;
}

/* replacement for win32 function */	//Check this.  [test] --Qamly
#ifndef  _MSC_VER		//was WIN32, but gcc is OK with this?			//Note, I vote for name change, since we are using SDL now right? --Qamly
DWORD GetTickCount()
{
        return (DWORD) SDL_GetTicks();
}
#endif
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
}



/* Set the current cursor from a Resource ID */
void frameSetCursorFromRes(WORD resID)
{
	ASSERT((resID >= CURSOR_OFFSET, "frameSetCursorFromRes: bad resource ID"));
	ASSERT((resID < CURSOR_OFFSET + MAX_CURSORS, "frameSetCursorFromRes: bad resource ID"));

	//If we are already using this cursor then  return
	if (resID != currentCursorResID)
        {
		SDL_SetCursor(aCursors[resID - CURSOR_OFFSET]);
		currentCursorResID = resID;
        }
}




/*
 * Wndproc
 *
 * The windows message processing function.
 */
static void processEvent(SDL_Event *event)
{
	switch(event->type)
	{
#ifndef IGNORE_FOCUS
		case SDL_ACTIVEEVENT:
			if (event->active.state == SDL_APPINPUTFOCUS || event->active.state == SDL_APPACTIVE)
			{
				if (event->active.gain == 1)
				{
					DBP1(("WM_SETFOCUS\n"));
					if (focusState != FOCUS_IN)
					{
						DBP1(("FOCUS_SET\n"));
						focusState = FOCUS_SET;
					}
				}
				else
				{
					DBP1(("WM_KILLFOCUS\n"));
					if (focusState != FOCUS_OUT)
					{
						DBP1(("FOCUS_KILL\n"));
						focusState = FOCUS_KILL;
					}
					/* Have to tell the input system that we've lost focus */
					inputProcessEvent(event);
				}
			}
			break;
#endif
		case SDL_KEYUP:
		case SDL_KEYDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEMOTION:
			inputProcessEvent(event);
			break;
	}
}





static void initCursors()
{
        aCursors[CURSOR_ARROW - CURSOR_OFFSET] = init_system_cursor(cursor_arrow);
        aCursors[CURSOR_DEST - CURSOR_OFFSET] = init_system_cursor(cursor_dest);
        aCursors[CURSOR_SIGHT - CURSOR_OFFSET] = init_system_cursor(cursor_sight);
        aCursors[CURSOR_TARGET - CURSOR_OFFSET] = init_system_cursor(cursor_target);
        aCursors[CURSOR_LARROW - CURSOR_OFFSET] = init_system_cursor(cursor_larrow);
        aCursors[CURSOR_RARROW - CURSOR_OFFSET] = init_system_cursor(cursor_rarrow);
        aCursors[CURSOR_DARROW - CURSOR_OFFSET] = init_system_cursor(cursor_darrow);
        aCursors[CURSOR_UARROW - CURSOR_OFFSET] = init_system_cursor(cursor_uarrow);
        aCursors[CURSOR_DEFAULT - CURSOR_OFFSET] = init_system_cursor(cursor_default);
        aCursors[CURSOR_EDGEOFMAP - CURSOR_OFFSET] = init_system_cursor(cursor_default);
        aCursors[CURSOR_ATTACH - CURSOR_OFFSET] = init_system_cursor(cursor_attach);
        aCursors[CURSOR_ATTACK - CURSOR_OFFSET] = init_system_cursor(cursor_attack);
        aCursors[CURSOR_BOMB - CURSOR_OFFSET] = init_system_cursor(cursor_bomb);
        aCursors[CURSOR_BRIDGE - CURSOR_OFFSET] = init_system_cursor(cursor_bridge);
        aCursors[CURSOR_BUILD - CURSOR_OFFSET] = init_system_cursor(cursor_build);
        aCursors[CURSOR_EMBARK - CURSOR_OFFSET] = init_system_cursor(cursor_embark);
        aCursors[CURSOR_FIX - CURSOR_OFFSET] = init_system_cursor(cursor_fix);
        aCursors[CURSOR_GUARD - CURSOR_OFFSET] = init_system_cursor(cursor_guard);
        aCursors[CURSOR_JAM - CURSOR_OFFSET] = init_system_cursor(cursor_jam);
        aCursors[CURSOR_LOCKON - CURSOR_OFFSET] = init_system_cursor(cursor_lockon);
        aCursors[CURSOR_MENU - CURSOR_OFFSET] = init_system_cursor(cursor_menu);
        aCursors[CURSOR_MOVE - CURSOR_OFFSET] = init_system_cursor(cursor_move);
        aCursors[CURSOR_NOTPOSSIBLE - CURSOR_OFFSET] = init_system_cursor(cursor_notpossible);
        aCursors[CURSOR_PICKUP - CURSOR_OFFSET] = init_system_cursor(cursor_pickup);
        aCursors[CURSOR_SEEKREPAIR - CURSOR_OFFSET] = init_system_cursor(cursor_seekrepair);
        aCursors[CURSOR_SELECT - CURSOR_OFFSET] = init_system_cursor(cursor_select);
}


static void freeCursors()
{
	unsigned int i = 0;
	for( ; i < MAX_CURSORS; i++ )
	{
		SDL_FreeCursor( aCursors[i] );
	}
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
					 BOOL	bVidMem)	 	// Whether to put surfaces in video memory
{
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CDROM) != 0)
        {
		printf("Error: Could not initialise SDL (%s).\n", SDL_GetError());
		return FALSE;
        }

        SDL_WM_SetCaption(pWindowName, NULL);

	winQuit = FALSE;
	focusState = FOCUS_IN;
	focusLast = FOCUS_IN;
	mouseOn = TRUE;
	displayMouse = TRUE;
//	hInstance = hInst;
	bActiveDDraw = TRUE;

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

        /* initialise all cursors */
        initCursors();

        /* Initialise the Direct Draw Buffers */
        if (!screenInitialise(width, height, bitDepth, fullScreen, bVidMem, bActiveDDraw, hWndMain))
        {
                return FALSE;
        }

	/* Initialise the input system */
	inputInitialise();
	/* Initialise the frame rate stuff */
	InitFrameStuff();

	SDL_initFramerate( &wzFPSmanager );
	SDL_setFramerate( &wzFPSmanager, 60 );

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
	SDL_Event event;
	FRAME_STATUS	retVal;

	/* Tell the input system about the start of another frame */
	inputNewFrame();

	/* Deal with any windows messages */
	while ( SDL_PollEvent( &event ) != 0)
	{
		if (event.type == SDL_QUIT)
		{
			break;
		}
		processEvent(&event);
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

	/* If things are running normally update the framerate */
	if ((!winQuit) && (focusState == FOCUS_IN))
	{
		/* Update the frame rate stuff */
		MaintainFrameStuff();
		SDL_framerateDelay( &wzFPSmanager );
	}

	return retVal;
}



void frameShutDown(void)
{
	screenShutDown();

	/* Free the default cursor */
	// DestroyCursor(hCursor);

	/* Free all cursors */
	freeCursors();

	/* Destroy the Application window */
	SDL_Quit();

	/* shutdown the trig stuff */
	trigShutDown();

	// Shutdown the resource stuff
	resShutDown();

	// shutdown the block memory heap
	blkShutDown();

	/* Shutdown the memory system */
	memShutDown();
}

/***************************************************************************
  Load the file with name pointed to by pFileName into a memory buffer.
  If AllocateMem is true then the memory is allocated ... else it is
  already in allocated in ppFileData, and the max size is in pFileSize
  ... this is adjusted to the actual loaded file size.

  If hard_fail is true, we will assert and report on failures.
***************************************************************************/
static BOOL loadFile2(char *pFileName, UBYTE **ppFileData, UDWORD *pFileSize,
                      BOOL AllocateMem, BOOL hard_fail)
{
	PHYSFS_file *pfile;
	PHYSFS_sint64 filesize;
	PHYSFS_sint64 length_read;

	if (PHYSFSEXT_locateCorrectCase(pFileName)) {
		if (hard_fail) {
			debug(LOG_ERROR, "loadFile2: %s not found", pFileName);
			assert(FALSE);
		}
		return FALSE;
	}
	pfile = PHYSFS_openRead(pFileName);
	if (!pfile) {
		debug(LOG_ERROR, "loadFile2: %s could not be opened: %s", pFileName,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}
	filesize = PHYSFS_fileLength(pfile);

	//debug(LOG_WZ, "loadFile2: %s opened, size %i", pFileName, filesize);

	if (AllocateMem == TRUE) {
		// Allocate a buffer to store the data and a terminating zero
		*ppFileData = (UBYTE*)MALLOC(filesize + 1);
		if (*ppFileData == NULL) {
			debug(LOG_ERROR, "loadFile2: Out of memory loading %s", pFileName);
			assert(FALSE);
			return FALSE;
		}
	} else {
		if (filesize > *pFileSize) {
			debug(LOG_ERROR, "loadFile2: No room for file %s", pFileName);
			assert(FALSE);
			return FALSE;
		}
		assert(*ppFileData != NULL);
	}

	/* Load the file data */
	length_read = PHYSFS_read(pfile, *ppFileData, 1, filesize);
	if (length_read != filesize) {
		FREE( *ppFileData );
		debug(LOG_ERROR, "loadFile2: Reading %s short: %s",
		      pFileName, PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}
	if (!PHYSFS_close(pfile)) {
		FREE( *ppFileData );
		debug(LOG_ERROR, "loadFile2: Error closing %s: %s", pFileName,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}

	// Add the terminating zero
	*((*ppFileData) + filesize) = 0;

	// always set to correct size
	*pFileSize = filesize;

	return TRUE;
}

/***************************************************************************
	Save the data in the buffer into the given file.
***************************************************************************/
BOOL saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize)
{
	char filename[200]; // pFileName in lowercase
	PHYSFS_file *pfile;
	PHYSFS_uint32 size = fileSize;

	strncpy(filename, unix_path(pFileName), sizeof(filename));
	debug(LOG_WZ, "We are to write (%s) as (%s) of size %d", pFileName,
	      filename, fileSize);
	pfile = PHYSFS_openWrite(filename);
	if (!pfile) {
		const char *found = PHYSFS_getRealDir(filename);

		debug(LOG_ERROR, "saveFile: %s could not be opened: %s", filename,
		      PHYSFS_getLastError());
		if (found) {
			debug(LOG_ERROR, "saveFile: %s found as %s", filename, found);
		}
		assert(FALSE);
		return FALSE;
	}
	if (PHYSFS_write(pfile, pFileData, 1, size) != size) {
		debug(LOG_ERROR, "saveFile: %s could not write: %s", filename,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}
	if (!PHYSFS_close(pfile)) {
		debug(LOG_ERROR, "saveFile: Error closing %s: %s", filename,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}

	if (PHYSFS_getRealDir(filename) == NULL) {
		// weird
		debug(LOG_ERROR, "saveFile: PHYSFS_getRealDir(%s) returns NULL?!",
		      filename);
	} else {
	  debug(LOG_WZ, "Successfully wrote to %s%s%s with %d bytes",
		      PHYSFS_getRealDir(filename), PHYSFS_getDirSeparator(),
		      filename, size);
	}
	return TRUE;
}

BOOL loadFile(const char *pFileName, UBYTE **ppFileData, UDWORD *pFileSize)
{
	return loadFile2(unix_path(pFileName), ppFileData, pFileSize, TRUE, TRUE);
}

// load a file from disk into a fixed memory buffer
BOOL loadFileToBuffer(char *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	*pSize = bufferSize;
	return loadFile2(unix_path(pFileName), &pFileBuffer, pSize, FALSE, TRUE);
}

// as above but returns quietly if no file found
BOOL loadFileToBufferNoError(char *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	*pSize = bufferSize;
	return loadFile2(unix_path(pFileName), &pFileBuffer, pSize, FALSE, FALSE);
}



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
//	printf("%%%%%%%% String:%s Hash:%0x\n",String,iHashValue);
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
//	printf("%%%%%%%% (Ignorcase) String:%s Hash:%0x\n",String,iHashValue);
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
