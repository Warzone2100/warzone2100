/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 * Frame.c
 *
 * Initialisation and shutdown for the framework library.
 *
 * Includes a basic windows message loop.
 *
 */
#include "frame.h"
#include "file.h"

#include <SDL.h>
#include <physfs.h>

#include "frameint.h"
#include "frameresource.h"
#include "input.h"
#include "lib/widget/scrap.h"
#include "SDL_framerate.h"
#include "physfs_ext.h"

#include "cursors.h"

static const enum CURSOR_TYPE cursor_type =
	CURSOR_32;

/* Linux specific stuff */

static CURSOR currentCursor = CURSOR_MAX;
static SDL_Cursor* aCursors[CURSOR_MAX];

bool selfTest = false;

/************************************************************************************
 *
 *	Player globals
 */

uint32_t selectedPlayer = 0;  /**< Current player */
uint32_t realSelectedPlayer = 0;


/************************************************************************************
 *
 * Alex's frame rate stuff
 */

/* Over how many seconds is the average required? */
#ifdef _DEBUG
# define	TIMESPAN	1
#else
# define	TIMESPAN	5
#endif

/* Initial filler value for the averages - arbitrary */
#define  IN_A_FRAME 70

/* Global variables for the frame rate stuff */
static Uint32	FrameCounts[TIMESPAN] = { 0 };
static Uint32	FrameIndex = 0;
static Uint64	curFrames = 0; // Number of frames elapsed since start
static Uint64	lastFrames = 0;
static Uint32	curTicks = 0; // Number of ticks since execution started
static Uint32	lastTicks = 0;
static FPSmanager wzFPSmanager;
static bool	initFPSmanager = false;

void setFramerateLimit(int fpsLimit)
{
	if (!initFPSmanager)
	{
		/* Initialize framerate handler */
		SDL_initFramerate(&wzFPSmanager);
		initFPSmanager = true;
	}
	SDL_setFramerate(&wzFPSmanager, fpsLimit);
}


int getFramerateLimit(void)
{
	return SDL_getFramerate(&wzFPSmanager);
}

/* InitFrameStuff - needs to be called once before frame loop commences */
static void InitFrameStuff( void )
{
	UDWORD i;

	for (i=0; i<TIMESPAN; i++)
	{
		FrameCounts[i] = IN_A_FRAME;
	}

	FrameIndex = 0;
	curFrames = 0;
	lastFrames = 0;
	curTicks = 0;
	lastTicks = 0;
}

/* MaintainFrameStuff - call this during completion of each frame loop */
static void MaintainFrameStuff( void )
{
	curTicks = SDL_GetTicks();
	curFrames++;

	// Update the framerate only once per second
	if ( curTicks >= lastTicks + 1000 )
	{
		// TODO Would have to be normalized to be correct for < 1 fps:
		// FrameCounts[FrameIndex++] = 1000 * (curFrames - lastFrames) / (curTicks - lastTicks);
		FrameCounts[FrameIndex++] = curFrames - lastFrames;
		if ( FrameIndex >= TIMESPAN )
		{
			FrameIndex = 0;
		}
		lastTicks = curTicks;
		lastFrames = curFrames;
	}
}


UDWORD frameGetAverageRate(void)
{
	SDWORD averageFrames = 0, i = 0;
	for ( i = 0; i < TIMESPAN; i++ )
		averageFrames += FrameCounts[i];
	averageFrames /= TIMESPAN;

	return averageFrames;
}


UDWORD	frameGetFrameNumber(void)
{
	return curFrames;
}


/** Set the current cursor from a Resource ID
 */
void frameSetCursor(CURSOR cur)
{
	ASSERT(cur < CURSOR_MAX, "frameSetCursorFromRes: bad resource ID" );

	//If we are already using this cursor then  return
	if (cur != currentCursor)
        {
		SDL_SetCursor(aCursors[cur]);
		currentCursor = cur;
        }
}


static void initCursors(void)
{
	aCursors[CURSOR_ARROW]       = init_system_cursor(CURSOR_ARROW, cursor_type);
	aCursors[CURSOR_DEST]        = init_system_cursor(CURSOR_DEST, cursor_type);
	aCursors[CURSOR_SIGHT]       = init_system_cursor(CURSOR_SIGHT, cursor_type);
	aCursors[CURSOR_TARGET]      = init_system_cursor(CURSOR_TARGET, cursor_type);
	aCursors[CURSOR_LARROW]      = init_system_cursor(CURSOR_LARROW, cursor_type);
	aCursors[CURSOR_RARROW]      = init_system_cursor(CURSOR_RARROW, cursor_type);
	aCursors[CURSOR_DARROW]      = init_system_cursor(CURSOR_DARROW, cursor_type);
	aCursors[CURSOR_UARROW]      = init_system_cursor(CURSOR_UARROW, cursor_type);
	aCursors[CURSOR_DEFAULT]     = init_system_cursor(CURSOR_DEFAULT, cursor_type);
	aCursors[CURSOR_EDGEOFMAP]   = init_system_cursor(CURSOR_EDGEOFMAP, cursor_type);
	aCursors[CURSOR_ATTACH]      = init_system_cursor(CURSOR_ATTACH, cursor_type);
	aCursors[CURSOR_ATTACK]      = init_system_cursor(CURSOR_ATTACK, cursor_type);
	aCursors[CURSOR_BOMB]        = init_system_cursor(CURSOR_BOMB, cursor_type);
	aCursors[CURSOR_BRIDGE]      = init_system_cursor(CURSOR_BRIDGE, cursor_type);
	aCursors[CURSOR_BUILD]       = init_system_cursor(CURSOR_BUILD, cursor_type);
	aCursors[CURSOR_EMBARK]      = init_system_cursor(CURSOR_EMBARK, cursor_type);
	aCursors[CURSOR_DISEMBARK]   = init_system_cursor(CURSOR_DISEMBARK, cursor_type);
	aCursors[CURSOR_FIX]         = init_system_cursor(CURSOR_FIX, cursor_type);
	aCursors[CURSOR_GUARD]       = init_system_cursor(CURSOR_GUARD, cursor_type);
	aCursors[CURSOR_JAM]         = init_system_cursor(CURSOR_JAM, cursor_type);
	aCursors[CURSOR_LOCKON]      = init_system_cursor(CURSOR_LOCKON, cursor_type);
	aCursors[CURSOR_SCOUT]       = init_system_cursor(CURSOR_SCOUT, cursor_type);
	aCursors[CURSOR_MENU]        = init_system_cursor(CURSOR_MENU, cursor_type);
	aCursors[CURSOR_MOVE]        = init_system_cursor(CURSOR_MOVE, cursor_type);
	aCursors[CURSOR_NOTPOSSIBLE] = init_system_cursor(CURSOR_NOTPOSSIBLE, cursor_type);
	aCursors[CURSOR_PICKUP]      = init_system_cursor(CURSOR_PICKUP, cursor_type);
	aCursors[CURSOR_SEEKREPAIR]  = init_system_cursor(CURSOR_SEEKREPAIR, cursor_type);
	aCursors[CURSOR_SELECT]      = init_system_cursor(CURSOR_SELECT, cursor_type);
}


static void freeCursors(void)
{
	unsigned int i;
	for(i = 0 ; i < ARRAY_SIZE(aCursors); ++i)
	{
		SDL_FreeCursor(aCursors[i]);
	}
}

/*
 * frameInitialise
 *
 * Initialise the framework library. - PC version
 */
bool frameInitialise(
					const char *pWindowName,// The text to appear in the window title bar
					UDWORD width,			// The display width
					UDWORD height,			// The display height
					UDWORD bitDepth,		// The display bit depth
					unsigned int fsaa,      // FSAA anti aliasing level
					bool fullScreen,		// Whether to start full screen or windowed
					bool vsync)				// If to sync to vblank or not
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		debug( LOG_ERROR, "Error: Could not initialise SDL (%s).\n", SDL_GetError() );
		return false;
	}

	SDL_WM_SetCaption(pWindowName, NULL);

	/* Initialise the trig stuff */
	if (!trigInitialise())
	{
		return false;
	}

	/* initialise all cursors */
	initCursors();

	if (!screenInitialise(width, height, bitDepth, fsaa, fullScreen, vsync))
	{
		if (fullScreen)
		{
			info("Trying windowed mode now.");
			if (!screenInitialise(width, height, bitDepth, fsaa, false, vsync))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	/* Initialise the input system */
	inputInitialise();

	// Initialise clipboard stuff.
	init_scrap();

	/* Initialise the frame rate stuff */
	InitFrameStuff();

	// Initialise the resource stuff
	if (!resInitialise())
	{
		return false;
	}

	return true;
}


/*!
 * Call this each cycle to do general house keeping.
 */
void frameUpdate(void)
{
	/* Tell the input system about the start of another frame */
	inputNewFrame();

	/* Update the frame rate stuff */
	MaintainFrameStuff();

	SDL_framerateDelay(&wzFPSmanager);
}


/*!
 * Cleanup framework
 */
void frameShutDown(void)
{
	screenShutDown();

	/* Free all cursors */
	freeCursors();

	/* Destroy the Application window */
	SDL_Quit();

	// Shutdown the resource stuff
	resShutDown();
}

PHYSFS_file* openLoadFile(const char* fileName, bool hard_fail)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
	if (!fileHandle)
	{
		if (hard_fail)
		{
			ASSERT(!"unable to open file", "file %s could not be opened: %s", fileName, PHYSFS_getLastError());
		}
		else
		{
			debug(LOG_WZ, "optional file %s could not be opened: %s", fileName, PHYSFS_getLastError());
		}
	}

	return fileHandle;
}

/***************************************************************************
  Load the file with name pointed to by pFileName into a memory buffer.
  If AllocateMem is true then the memory is allocated ... else it is
  already in allocated in ppFileData, and the max size is in pFileSize
  ... this is adjusted to the actual loaded file size.

  If hard_fail is true, we will assert and report on failures.
***************************************************************************/
static bool loadFile2(const char *pFileName, char **ppFileData, UDWORD *pFileSize,
                      bool AllocateMem, bool hard_fail)
{
	PHYSFS_file *pfile;
	PHYSFS_sint64 filesize;
	PHYSFS_sint64 length_read;

	pfile = openLoadFile(pFileName, hard_fail);
	if (!pfile)
	{
		return false;
	}

	filesize = PHYSFS_fileLength(pfile);

	//debug(LOG_WZ, "loadFile2: %s opened, size %i", pFileName, filesize);

	if (AllocateMem)
	{
		// Allocate a buffer to store the data and a terminating zero
		*ppFileData = (char*)malloc(filesize + 1);
		if (*ppFileData == NULL)
		{
			debug(LOG_ERROR, "loadFile2: Out of memory loading %s", pFileName);
			assert(false);
			return false;
		}
	}
	else
	{
		if (filesize > *pFileSize)
		{
			debug(LOG_ERROR, "loadFile2: No room for file %s, buffer is too small! Got: %d Need: %ld", pFileName, *pFileSize, (long)filesize);
			assert(false);
			return false;
		}
		assert(*ppFileData != NULL);
	}

	/* Load the file data */
	length_read = PHYSFS_read(pfile, *ppFileData, 1, filesize);
	if (length_read != filesize)
	{
		if (AllocateMem)
		{
			free(*ppFileData);
			*ppFileData = NULL;
		}

		debug(LOG_ERROR, "Reading %s short: %s", pFileName, PHYSFS_getLastError());
		assert(false);
		return false;
	}

	if (!PHYSFS_close(pfile))
	{
		if (AllocateMem)
		{
			free(*ppFileData);
			*ppFileData = NULL;
		}

		debug(LOG_ERROR, "Error closing %s: %s", pFileName, PHYSFS_getLastError());
		assert(false);
		return false;
	}

	// Add the terminating zero
	*((*ppFileData) + filesize) = 0;

	// always set to correct size
	*pFileSize = filesize;

	return true;
}

PHYSFS_file* openSaveFile(const char* fileName)
{
	PHYSFS_file* fileHandle = PHYSFS_openWrite(fileName);
	if (!fileHandle)
	{
		const char *found = PHYSFS_getRealDir(fileName);

		debug(LOG_ERROR, "%s could not be opened: %s", fileName, PHYSFS_getLastError());
		if (found)
		{
			debug(LOG_ERROR, "%s found as %s", fileName, found);
		}

		assert(!"openSaveFile: couldn't open file for writing");
		return NULL;
	}

	return fileHandle;
}

/***************************************************************************
	Save the data in the buffer into the given file.
***************************************************************************/
bool saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize)
{
	PHYSFS_file *pfile;
	PHYSFS_uint32 size = fileSize;

	debug(LOG_WZ, "We are to write (%s) of size %d", pFileName, fileSize);
	pfile = openSaveFile(pFileName);
	if (!pfile)
	{
		ASSERT(false, "Couldn't save file %s (%s)?", pFileName, PHYSFS_getLastError());
		return false;
	}

	if (PHYSFS_write(pfile, pFileData, 1, size) != size) 
	{
		debug(LOG_ERROR, "%s could not write: %s", pFileName, PHYSFS_getLastError());
		assert(false);
		return false;
	}
	if (!PHYSFS_close(pfile))
	{
		debug(LOG_ERROR, "Error closing %s: %s", pFileName, PHYSFS_getLastError());
		assert(false);
		return false;
	}

	if (PHYSFS_getRealDir(pFileName) == NULL) 
	{
		// weird
		debug(LOG_ERROR, "PHYSFS_getRealDir(%s) returns NULL (%s)?!", pFileName, PHYSFS_getLastError());
	}
	else
	{
		debug(LOG_WZ, "Successfully wrote to %s%s%s with %d bytes", PHYSFS_getRealDir(pFileName), PHYSFS_getDirSeparator(), pFileName, size);
	}
	return true;
}

bool loadFile(const char *pFileName, char **ppFileData, UDWORD *pFileSize)
{
	return loadFile2(pFileName, ppFileData, pFileSize, true, true);
}

// load a file from disk into a fixed memory buffer
bool loadFileToBuffer(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	*pSize = bufferSize;
	return loadFile2(pFileName, &pFileBuffer, pSize, false, true);
}

// as above but returns quietly if no file found
bool loadFileToBufferNoError(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	*pSize = bufferSize;
	return loadFile2(pFileName, &pFileBuffer, pSize, false, false);
}


/* next four used in HashPJW */
#define	BITS_IN_int		32
#define	THREE_QUARTERS	((UDWORD) ((BITS_IN_int * 3) / 4))
#define	ONE_EIGHTH		((UDWORD) (BITS_IN_int / 8))
#define	HIGH_BITS		( ~((UDWORD)(~0) >> ONE_EIGHTH ))


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
UDWORD HashString( const char *c )
{
	UDWORD	iHashValue;

	assert(c != NULL);
	assert(*c != 0x0);

	for (iHashValue = 0; *c; ++c)
	{
		unsigned int i;
		iHashValue = ( iHashValue << ONE_EIGHTH ) + *c;

		i = iHashValue & HIGH_BITS;
		if ( i != 0 )
		{
			iHashValue = ( iHashValue ^ ( i >> THREE_QUARTERS ) ) &
							~HIGH_BITS;
		}
	}
	return iHashValue;
}

/* Converts lower case ASCII characters into upper case characters
 * \param c the character to convert
 * \return an upper case ASCII character
 */
static inline char upcaseASCII(char c)
{
	// If this is _not_ a lower case character simply return
	if (c < 'a' || c > 'z')
		return c;
	// Otherwise substract 32 to make the lower case character an upper case one
	else
		return c - 32;
}

UDWORD HashStringIgnoreCase( const char *c )
{
	UDWORD	iHashValue;

	assert(c != NULL);
	assert(*c != 0x0);

	for (iHashValue=0; *c; ++c)
	{
		unsigned int i;
		iHashValue = ( iHashValue << ONE_EIGHTH ) + upcaseASCII(*c);

		i = iHashValue & HIGH_BITS;
		if ( i != 0 )
		{
			iHashValue = ( iHashValue ^ ( i >> THREE_QUARTERS ) ) &
							~HIGH_BITS;
		}
	}
	return iHashValue;
}

#if defined(WZ_OS_WIN)
/**
 * The difference between the FAT32 and Unix epoch.
 *
 * The FAT32 epoch starts at 1 January 1601 while the Unix epoch starts at 1
 * January 1970. And apparantly we gained 3.25 days in that time period.
 *
 * Thus the amount of micro seconds passed between these dates can be computed
 * as follows:
 * \f[((1970 - 1601) \cdot 365.25 + 3.25) \cdot 86400 \cdot 1000000\f]
 *
 * Use 1461 and 13 instead of 365.25 and 3.25 respectively because we can't use
 * floating point math here.
 */
static const uint64_t usecs_between_fat32_and_unix_epoch = (uint64_t)((1970 - 1601) * 1461 + 13) * (uint64_t)86400 / (uint64_t)4 * (uint64_t)1000000;

int gettimeofday(struct timeval* tv, struct timezone* tz)
{
	ASSERT(tz == NULL, "This gettimeofday implementation doesn't provide timezone info.");

	if (tv)
	{
		FILETIME ft;
		uint64_t systime, usec;

		/* Retrieve the current time expressed as 100 nano-second
		 * intervals since 1 January 1601 (UTC).
		 */
		GetSystemTimeAsFileTime(&ft);
		systime = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

		// Convert to micro seconds since 1 January 1970 (UTC).
		usec = systime / 10 - usecs_between_fat32_and_unix_epoch;

		tv->tv_sec  = usec / (uint64_t)1000000;
		tv->tv_usec = usec % (uint64_t)1000000;
	}

	return 0;
}
#endif

bool PHYSFS_printf(PHYSFS_file *file, const char *format, ...)
{
	char vaBuffer[PATH_MAX];
	va_list ap;

	va_start(ap, format);
	vssprintf(vaBuffer, format, ap);
	va_end(ap);

	return PHYSFS_write(file, vaBuffer, strlen(vaBuffer), 1);
}
