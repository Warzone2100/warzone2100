/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/*! \file
 *  \brief Resource file processing functions
 */

#ifndef _frameresource_h
#define _frameresource_h

#include "lib/framework/frame.h"

/** Maximum number of characters in a resource type. */
#define RESTYPE_MAXCHAR		20


/** Function pointer for a function that loads from a memory buffer. */
typedef bool (*RES_BUFFERLOAD)(const char *pBuffer, UDWORD size, void **pData);

/** Function pointer for a function that loads from a filename. */
typedef bool (*RES_FILELOAD)(const char *pFile, void **pData);

/** Function pointer for releasing a resource loaded by the above functions. */
typedef void (*RES_FREE)(void *pData);

/** callback type for resload display callback. */
typedef void (*RESLOAD_CALLBACK)();

struct RES_DATA
{
	void		*pData;				// pointer to the acutal data
	SDWORD		blockID;			// which of the blocks is it in (so we can clear some of them...)

	UDWORD	HashedID;				// hashed version of the name of the id
	RES_DATA       *psNext;                         // next entry - most likely to be following on!
	UDWORD		usage; // Reference count

	// ID of the resource - filename from the .wrf - e.g. "TRON.PIE"
	const char *aID;
};


// New reduced resource type ... specially for PSX
// These types  are statically defined in data.c
struct RES_TYPE
{
	// type is still needed on psx ... strings are defined in source - data.c (yak!)
	char			aType[RESTYPE_MAXCHAR];		// type string (e.g. "PIE"	 - just for debug use only, only aplicable when loading from wrf (not wdg)

	RES_BUFFERLOAD buffLoad;	// routine to process the data for this type
	RES_FREE release;			// routine to release the data (NULL indicates none)

	// we must have a pointer to the data here so that we can do a resGetData();
	RES_DATA		*psRes;		// Linked list of data items of this type
	UDWORD	HashedType;				// hashed version of the name of the id - // a null hashedtype indicates end of list

	RES_FILELOAD	fileLoad;		// This isn't really used any more ?
	RES_TYPE       *psNext;
};


/** Set the function to call when loading files with resloadfile. */
void resSetLoadCallback(RESLOAD_CALLBACK funcToCall);

/** Initialise the resource module. */
bool resInitialise();

/** Shutdown the resource module. */
void resShutDown();

/** Set the base resource directory. */
WZ_DECL_NONNULL(1) void resSetBaseDir(const char *pResDir);
WZ_DECL_NONNULL(1) void resForceBaseDir(const char *pResDir);

/** Parse the res file. */
WZ_DECL_NONNULL(1) bool resLoad(const char *pResFile, SDWORD blockID);

/** Release all the resources currently loaded and the resource load functions. */
void resReleaseAll();

/** Release the data for a particular block ID. */
void resReleaseBlockData(SDWORD blockID);

/** Release all the resources currently loaded but keep the resource load functions. */
void resReleaseAllData();

/** Add a buffer load and release function for a file type. */
WZ_DECL_NONNULL(1) bool resAddBufferLoad(const char *pType, RES_BUFFERLOAD buffLoad, RES_FREE release);

/** Add a file name load and release function for a file type. */
WZ_DECL_NONNULL(1) bool resAddFileLoad(const char *pType, RES_FILELOAD fileLoad, RES_FREE release);

/** Call the load function for a file. */
WZ_DECL_NONNULL(1, 2) bool resLoadFile(const char *pType, const char *pFile);

/** Return the resource for a type and ID */
WZ_DECL_NONNULL(1) void *resGetDataFromHash(const char *pType, UDWORD HashedID);
WZ_DECL_NONNULL(1, 2) void *resGetData(const char *pType, const char *pID);
WZ_DECL_NONNULL(1, 2) bool resPresent(const char *pType, const char *pID);
WZ_DECL_NONNULL(1) void resToLower(char *pStr);

/** Return the HashedID string for a piece of data. */
WZ_DECL_NONNULL(1, 3) bool resGetHashfromData(const char *pType, const void *pData, UDWORD *pHash);

/** Retrieve the resource ID string
 *  \param type the resource type string (e.g. "IMG", "IMD", "TEXPAGE", "WAV", etc.)
 *  \param data the resource pointer to retrieve the ID string for
 *  \return the from the ID string (usually its filename without directory)
 *  \note passing a NULL pointer for either \c type or \c data is valid (the result will be an empty string though)
 */
const char *resGetNamefromData(const char *type, const void *data);

/** Return last imd resource */
const char *GetLastResourceFilename() WZ_DECL_PURE;

/** Set the resource name of the last resource file loaded. */
WZ_DECL_NONNULL(1) void SetLastResourceFilename(const char *pName);

#endif // _frameresource_h
