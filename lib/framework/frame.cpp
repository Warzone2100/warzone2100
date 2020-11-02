/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 */
#include "frame.h"
#include "file.h"
#include "wzapp.h"

#include <physfs.h>
#include "physfs_ext.h"

#include "frameresource.h"
#include "input.h"

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
static void InitFrameStuff()
{
	frameCount = 0;
	curFrames = 0;
	lastFrames = 0;
	curTicks = 0;
	lastTicks = 0;
}

int frameRate()
{
	return frameCount;
}

UDWORD	frameGetFrameNumber()
{
	return (UDWORD)curFrames;
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
void frameUpdate()
{
	curTicks = wzGetTicks();
	curFrames++;

	// Update the framerate only once per second
	if (curTicks >= lastTicks + 1000)
	{
		frameCount = static_cast<int>(curFrames - lastFrames);
		lastTicks = curTicks;
		lastFrames = curFrames;
	}
}


/*!
 * Cleanup framework
 */
void frameShutDown()
{
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

PHYSFS_file *openLoadFile(const char *fileName, bool hard_fail)
{
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_NEVER, "Reading...[directory: %s] %s", WZ_PHYSFS_getRealDir_String(fileName).c_str(), fileName);
	if (!fileHandle)
	{
		if (hard_fail)
		{
			ASSERT(!"unable to open file", "file %s could not be opened: %s", fileName, WZ_PHYSFS_getLastError());
		}
		else
		{
			debug(LOG_WZ, "optional file %s could not be opened: %s", fileName, WZ_PHYSFS_getLastError());
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
static bool loadFile2(const char *pFileName, char **ppFileData, UDWORD *pFileSize, bool AllocateMem, bool hard_fail)
{
	if (WZ_PHYSFS_isDirectory(pFileName))
	{
		return false;
	}

	PHYSFS_file *pfile = openLoadFile(pFileName, hard_fail);
	if (!pfile)
	{
		return false;
	}

	PHYSFS_sint64 filesize = PHYSFS_fileLength(pfile);
	if (filesize < 0)
	{
		return false;  // File size could not be determined. Is a directory?
	}
	ASSERT_OR_RETURN(false, filesize < static_cast<PHYSFS_sint64>(std::numeric_limits<PHYSFS_sint32>::max()), "\"%s\" filesize >= std::numeric_limits<PHYSFS_sint32>::max()", pFileName);
	ASSERT_OR_RETURN(false, static_cast<PHYSFS_uint64>(filesize) < static_cast<PHYSFS_uint64>(std::numeric_limits<size_t>::max()), "\"%s\" filesize >= std::numeric_limits<size_t>::max()", pFileName);

	if (AllocateMem)
	{
		// Allocate a buffer to store the data and a terminating zero
		*ppFileData = (char *)malloc(static_cast<size_t>(filesize + 1));
	}
	else
	{
		if (filesize > *pFileSize)
		{
			debug(LOG_ERROR, "No room for file %s, buffer is too small! Got: %d Need: %ld", pFileName, *pFileSize, (long)filesize);
			assert(false);
			return false;
		}
		assert(*ppFileData != nullptr);
	}

	/* Load the file data */
	PHYSFS_sint64 length_read = WZ_PHYSFS_readBytes(pfile, *ppFileData, static_cast<PHYSFS_uint32>(filesize));
	if (length_read != filesize)
	{
		if (AllocateMem)
		{
			free(*ppFileData);
			*ppFileData = nullptr;
		}

		debug(LOG_ERROR, "Reading %s short: %s", pFileName, WZ_PHYSFS_getLastError());
		assert(false);
		return false;
	}

	if (!PHYSFS_close(pfile))
	{
		if (AllocateMem)
		{
			free(*ppFileData);
			*ppFileData = nullptr;
		}

		debug(LOG_ERROR, "Error closing %s: %s", pFileName, WZ_PHYSFS_getLastError());
		assert(false);
		return false;
	}

	// Add the terminating zero
	*((*ppFileData) + filesize) = 0;

	// always set to correct size
	ASSERT(static_cast<PHYSFS_uint64>(filesize) <= static_cast<PHYSFS_uint64>(std::numeric_limits<UDWORD>::max()), "filesize exceeds std::numeric_limits<UDWORD>::max()");
	*pFileSize = static_cast<UDWORD>(filesize);

	return true;
}

PHYSFS_file *openSaveFile(const char *fileName)
{
	PHYSFS_file *fileHandle = PHYSFS_openWrite(fileName);
	if (!fileHandle)
	{
		const char *found = PHYSFS_getRealDir(fileName);

		debug(LOG_ERROR, "%s could not be opened: %s", fileName, WZ_PHYSFS_getLastError());
		if (found)
		{
			debug(LOG_ERROR, "%s found as %s", fileName, found);
		}

		assert(!"openSaveFile: couldn't open file for writing");
		return nullptr;
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
		ASSERT(false, "Couldn't save file %s (%s)?", pFileName, WZ_PHYSFS_getLastError());
		return false;
	}

	if (WZ_PHYSFS_writeBytes(pfile, pFileData, size) != size)
	{
		debug(LOG_ERROR, "%s could not write: %s", pFileName, WZ_PHYSFS_getLastError());
		assert(false);
		return false;
	}
	if (!PHYSFS_close(pfile))
	{
		debug(LOG_ERROR, "Error closing %s: %s", pFileName, WZ_PHYSFS_getLastError());
		assert(false);
		return false;
	}

	if (PHYSFS_getRealDir(pFileName) == nullptr)
	{
		// weird
		debug(LOG_ERROR, "PHYSFS_getRealDir(%s) returns NULL (%s)?!", pFileName, WZ_PHYSFS_getLastError());
	}
	else
	{
		debug(LOG_WZ, "Successfully wrote to %s%s with %d bytes", WZ_PHYSFS_getRealDir_String(pFileName).c_str(), pFileName, size);
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

Sha256 findHashOfFile(char const *realFileName)
{
	char *realFileData = nullptr;
	uint32_t realFileSize = 0;
	if (loadFile(realFileName, &realFileData, &realFileSize))
	{
		Sha256 realFileHash = sha256Sum(realFileData, realFileSize);
		free(realFileData);
		return realFileHash;
	}
	Sha256 zero;
	zero.setZero();
	return zero;
}

bool PHYSFS_printf(PHYSFS_file *file, const char *format, ...)
{
	char vaBuffer[PATH_MAX];
	va_list ap;

	va_start(ap, format);
	vssprintf(vaBuffer, format, ap);
	va_end(ap);

	return WZ_PHYSFS_writeBytes(file, vaBuffer, static_cast<PHYSFS_uint32>(strlen(vaBuffer)));
}

std::string video_backend_names[] =
{
	"opengl",
	"opengles",
	"vulkan",
#if defined(WZ_BACKEND_DIRECTX)
	"directx",
#endif
	"invalid" // Must be last!
};

static_assert((size_t)video_backend::num_backends == (sizeof(video_backend_names) / sizeof(std::string)) - 1, "video_backend_names must match video_backend enum");

bool video_backend_from_str(const char *str, video_backend &output_backend)
{
	for (size_t i = 0; i < (size_t)video_backend::num_backends; i++)
	{
		if (strcasecmp(video_backend_names[i].c_str(), str) == 0)
		{
			output_backend = (video_backend)i;
			return true;
		}
	}
	return false;
}

std::string to_string(video_backend backend)
{
	return video_backend_names[(size_t)backend];
}

std::string to_display_string(const video_backend& backend)
{
	switch (backend)
	{
		case video_backend::opengl:
			return "OpenGL";
		case video_backend::opengles:
			return "OpenGL ES";
		case video_backend::vulkan:
			return "Vulkan";
#if defined(WZ_BACKEND_DIRECTX)
		case video_backend::directx:
			return "DirectX (ANGLE)";
#endif
		case video_backend::num_backends:
			debug(LOG_FATAL, "Should never happen");
			break;
	}
	return "n/a";
}
