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
/*! \file frameresource.h
 * \brief Resource file processing functions
 */

#ifndef _resource_h
#define _resource_h

#include "lib/framework/frame.h"

/* Maximum number of characters in a resource type */
#define RESTYPE_MAXCHAR		20

/* Maximum number of characters in a resource ID */
#define RESID_MAXCHAR		40

/* Function pointer for a function that loads from a memory buffer */
typedef BOOL (*RES_BUFFERLOAD)(const char *pBuffer, UDWORD size, void **pData);
/* Function pointer for a function that loads from a filename */
typedef BOOL (*RES_FILELOAD)(const char *pFile, void **pData);

/* Function pointer for releasing a resource loaded by the above functions */
typedef void (*RES_FREE)(void *pData);

/* callback type for resload display callback*/
typedef void (*RESLOAD_CALLBACK)(void);


typedef struct res_data
{
	void		*pData;				// pointer to the acutal data
	SDWORD		blockID;			// which of the blocks is it in (so we can clear some of them...)

	UDWORD	HashedID;				// hashed version of the name of the id
	struct	res_data *psNext;		// next entry - most likely to be following on!
	UDWORD		usage; // Reference count

	// ID of the resource - filename from the .wrf - e.g. "TRON.PIE"
	const char* aID;
} RES_DATA;


// New reduced resource type ... specially for PSX
// These types  are statically defined in data.c
typedef struct _res_type
{
	// type is still needed on psx ... strings are defined in source - data.c (yak!)
	char			aType[RESTYPE_MAXCHAR];		// type string (e.g. "PIE"	 - just for debug use only, only aplicable when loading from wrf (not wdg)

	RES_BUFFERLOAD buffLoad;	// routine to process the data for this type
	RES_FREE release;			// routine to release the data (NULL indicates none)

	// we must have a pointer to the data here so that we can do a resGetData();
	RES_DATA		*psRes;		// Linked list of data items of this type
	UDWORD	HashedType;				// hashed version of the name of the id - // a null hashedtype indicates end of list

	RES_FILELOAD	fileLoad;		// This isn't really used any more ?
	struct _res_type	*psNext;
} RES_TYPE;


/* set the function to call when loading files with resloadfile*/
extern void resSetLoadCallback(RESLOAD_CALLBACK funcToCall);

/* Initialise the resource module */
extern BOOL resInitialise(void);

/* Shutdown the resource module */
extern void resShutDown(void);

// set the base resource directory
extern void resSetBaseDir(const char* pResDir);

/* Parse the res file */
BOOL resLoad(const char *pResFile, SDWORD blockID);

/* Release all the resources currently loaded and the resource load functions */
extern void resReleaseAll(void);

// release the data for a particular block ID
extern void resReleaseBlockData(SDWORD blockID);

/* Release all the resources currently loaded but keep the resource load functions */
extern void resReleaseAllData(void);

/* Add a buffer load and release function for a file type */
extern BOOL	resAddBufferLoad(const char *pType, RES_BUFFERLOAD buffLoad,
							 RES_FREE release);

/* Add a file name load and release function for a file type */
extern BOOL	resAddFileLoad(const char *pType, RES_FILELOAD fileLoad,
						   RES_FREE release);

/* Call the load function for a file */
extern BOOL resLoadFile(const char *pType, const char *pFile);

// Add data to the resource system
extern BOOL resAddData(char *pType, char *pID, void *pData);

/* Return the resource for a type and ID */
extern void *resGetDataFromHash(const char *pType, UDWORD HashedID);
extern void *resGetData(const char *pType, const char *pID);
extern BOOL resPresent(const char *pType, const char *pID);
void resToLower(char *pStr);

// return the HashedID string for a piece of data
extern BOOL resGetHashfromData(const char *pType, const void *pData, UDWORD *pHash);

//return last imd resource
const char *GetLastResourceFilename(void) WZ_DECL_PURE;
// Set the resource name of the last resource file loaded
void SetLastResourceFilename(const char *pName);

#endif
