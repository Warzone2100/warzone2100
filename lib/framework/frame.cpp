/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
#include "wzapp.h"

#include <physfs.h>

#include "frameint.h"
#include "frameresource.h"
#include "input.h"
#include "physfs_ext.h"

#include "cursors.h"

/* Linux specific stuff */

/************************************************************************************
 *
 *	Player globals
 */

static bool mousewarp = false;

uint32_t selectedPlayer = 0;  /**< Current player */
uint32_t realSelectedPlayer = 0;

/* Global variables for the frame rate stuff */
static int frameCount = 0;
static uint64_t curFrames = 0; // Number of frames elapsed since start
static uint64_t lastFrames = 0;
static uint32_t curTicks = 0; // Number of ticks since execution started
static uint32_t lastTicks = 0;

/* InitFrameStuff - needs to be called once before frame loop commences */
static void InitFrameStuff( void )
{
	frameCount = 0.0;
	curFrames = 0;
	lastFrames = 0;
	curTicks = 0;
	lastTicks = 0;
}

int frameRate(void)
{
	return frameCount;
}

UDWORD	frameGetFrameNumber(void)
{
	return curFrames;
}

/*
 * frameInitialise
 *
 * Initialise the framework library. - PC version
 */
bool frameInitialise()
{
	/* Initialise the trig stuff */
	if (!trigInitialise())
	{
		return false;
	}

	if (!screenInitialise())
	{
		return false;
	}

	/* Initialise the input system */
	inputInitialise();

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
	curTicks = wzGetTicks();
	curFrames++;

	// Update the framerate only once per second
	if (curTicks >= lastTicks + 1000)
	{
		frameCount = curFrames - lastFrames;
		lastTicks = curTicks;
		lastFrames = curFrames;
	}
}


/*!
 * Cleanup framework
 */
void frameShutDown(void)
{
	debug(LOG_NEVER, "Screen shutdown!");
	screenShutDown();

	// Shutdown the resource stuff
	debug(LOG_NEVER, "No more resources!");
	resShutDown();
}

void setMouseWarp(bool value)
{
	mousewarp = value;
}

bool getMouseWarp()
{
	return mousewarp;
}

PHYSFS_file* openLoadFile(const char* fileName, bool hard_fail)
{
	PHYSFS_file* fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_NEVER, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
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

bool PHYSFS_printf(PHYSFS_file *file, const char *format, ...)
{
	char vaBuffer[PATH_MAX];
	va_list ap;

	va_start(ap, format);
	vssprintf(vaBuffer, format, ap);
	va_end(ap);

	return PHYSFS_write(file, vaBuffer, strlen(vaBuffer), 1);
}
