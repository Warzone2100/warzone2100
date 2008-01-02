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
#include <string.h>
#include <SDL.h>
#include <physfs.h>

// window focus messages
//#define DEBUG_GROUP1
#include "frame.h"
#include "frameint.h"
#include "frameresource.h"
#include "input.h"

#include "fractions.h"
#include <assert.h>
#ifdef __APPLE__
#include "cursors16.h"
#else
#include "cursors.h"
#endif

/* Linux specific stuff */

static UWORD currentCursorResID = UWORD_MAX;
SDL_Cursor *aCursors[MAX_CURSORS];

FOCUS_STATE focusState = FOCUS_IN;

/************************************************************************************
 *
 *	Player globals
 */

UDWORD		selectedPlayer = 0; 	/**< Current player */


/************************************************************************************
 *
 * Alex's frame rate stuff
 */

/* Over how many seconds is the average required? */
#define	TIMESPAN	5

/* Initial filler value for the averages - arbitrary */
#define  IN_A_FRAME 70

/* Global variables for the frame rate stuff */
static Uint32	FrameCounts[TIMESPAN] = { 0 };
static Uint32	FrameIndex = 0;
static Uint64	curFrames = 0; // Number of frames elapsed since start
static Uint64	lastFrames = 0;
static Uint32	curTicks = 0; // Number of ticks since execution started
static Uint32	lastTicks = 0;

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


/** Set the current cursor from a Resource ID */
void frameSetCursorFromRes(SWORD resID)
{
	ASSERT( resID >= CURSOR_OFFSET, "frameSetCursorFromRes: bad resource ID" );
	ASSERT( resID < CURSOR_OFFSET + MAX_CURSORS, "frameSetCursorFromRes: bad resource ID" );

	//If we are already using this cursor then  return
	if (resID != currentCursorResID)
        {
		SDL_SetCursor(aCursors[resID - CURSOR_OFFSET]);
		currentCursorResID = resID;
        }
}


static void initCursors(void)
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


static void freeCursors(void)
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
BOOL frameInitialise(
					const char *pWindowName,// The text to appear in the window title bar
					UDWORD width,			// The display width
					UDWORD height,			// The display height
					UDWORD bitDepth,		// The display bit depth
					BOOL fullScreen		// Whether to start full screen or windowed
					)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		debug( LOG_ERROR, "Error: Could not initialise SDL (%s).\n", SDL_GetError() );
		return FALSE;
	}

	SDL_WM_SetCaption(pWindowName, NULL);

	/* Initialise the trig stuff */
	if (!trigInitialise())
	{
		return FALSE;
	}

		/* initialise all cursors */
		initCursors();

		/* Initialise the Direct Draw Buffers */
		if (!screenInitialise(width, height, bitDepth, fullScreen))
		{
				return FALSE;
		}

	/* Initialise the input system */
	inputInitialise();
	/* Initialise the frame rate stuff */
	InitFrameStuff();

	// Initialise the resource stuff
	if (!resInitialise())
	{
		return FALSE;
	}

	return TRUE;
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

	/* shutdown the trig stuff */
	trigShutDown();

	// Shutdown the resource stuff
	resShutDown();
}

PHYSFS_file* openLoadFile(const char* fileName, bool hard_fail)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(fileName);
	if (!fileHandle)
	{
		if (hard_fail)
		{
			ASSERT(!"unable to open file", "openLoadFile: file %s could not be opened: %s", fileName, PHYSFS_getLastError());
		}
		else
		{
			debug(LOG_WARNING, "openLoadFile: optional file %s could not be opened: %s", fileName, PHYSFS_getLastError());
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
static BOOL loadFile2(const char *pFileName, char **ppFileData, UDWORD *pFileSize,
                      BOOL AllocateMem, BOOL hard_fail)
{
	PHYSFS_file *pfile;
	PHYSFS_sint64 filesize;
	PHYSFS_sint64 length_read;

	pfile = openLoadFile(pFileName, hard_fail);
	if (!pfile)
	{
		return FALSE;
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
			assert(FALSE);
			return FALSE;
		}
	}
	else
	{
		if (filesize > *pFileSize)
		{
			debug(LOG_ERROR, "loadFile2: No room for file %s, buffer is too small! Got: %d Need: %ld", pFileName, *pFileSize, (long)filesize);
			assert(FALSE);
			return FALSE;
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

		debug(LOG_ERROR, "loadFile2: Reading %s short: %s",
		      pFileName, PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}

	if (!PHYSFS_close(pfile))
	{
		if (AllocateMem)
		{
			free(*ppFileData);
			*ppFileData = NULL;
		}

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

PHYSFS_file* openSaveFile(const char* fileName)
{
	PHYSFS_file* fileHandle = PHYSFS_openWrite(fileName);
	if (!fileHandle)
	{
		const char *found = PHYSFS_getRealDir(fileName);

		debug(LOG_ERROR, "saveFile: %s could not be opened: %s", fileName, PHYSFS_getLastError());
		if (found)
		{
			debug(LOG_ERROR, "saveFile: %s found as %s", fileName, found);
		}

		assert(!"openSaveFile: couldn't open file for writing");
	}

	return fileHandle;
}

/***************************************************************************
	Save the data in the buffer into the given file.
***************************************************************************/
BOOL saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize)
{
	PHYSFS_file *pfile;
	PHYSFS_uint32 size = fileSize;

	debug(LOG_WZ, "We are to write (%s) of size %d", pFileName, fileSize);
	pfile = openSaveFile(pFileName);
	if (!pfile)
	{
		return FALSE;
	}

	if (PHYSFS_write(pfile, pFileData, 1, size) != size) {
		debug(LOG_ERROR, "saveFile: %s could not write: %s", pFileName,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}
	if (!PHYSFS_close(pfile)) {
		debug(LOG_ERROR, "saveFile: Error closing %s: %s", pFileName,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}

	if (PHYSFS_getRealDir(pFileName) == NULL) {
		// weird
		debug(LOG_ERROR, "saveFile: PHYSFS_getRealDir(%s) returns NULL?!",
		      pFileName);
	} else {
	  debug(LOG_WZ, "Successfully wrote to %s%s%s with %d bytes",
		      PHYSFS_getRealDir(pFileName), PHYSFS_getDirSeparator(),
		      pFileName, size);
	}
	return TRUE;
}

BOOL loadFile(const char *pFileName, char **ppFileData, UDWORD *pFileSize)
{
	return loadFile2(pFileName, ppFileData, pFileSize, TRUE, TRUE);
}

// load a file from disk into a fixed memory buffer
BOOL loadFileToBuffer(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	*pSize = bufferSize;
	return loadFile2(pFileName, &pFileBuffer, pSize, FALSE, TRUE);
}

// as above but returns quietly if no file found
BOOL loadFileToBufferNoError(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize)
{
	*pSize = bufferSize;
	return loadFile2(pFileName, &pFileBuffer, pSize, FALSE, FALSE);
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
