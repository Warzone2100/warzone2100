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
/*
 * FrameResource.c
 *
 * Framework Resource file processing functions
 *
 */
#include "frameresource.h"

#include "string_ext.h"
#include "stdio_ext.h"

#include "file.h"
#include "resly.h"

// Local prototypes
static RES_TYPE *psResTypes=NULL;

/* The initial resource directory and the current resource directory */
char aResDir[PATH_MAX];
char aCurrResDir[PATH_MAX];

// the current resource block ID
static SDWORD resBlockID;

// prototypes
static void ResetResourceFile(void);

// callback to resload screen.
static RESLOAD_CALLBACK resLoadCallback=NULL;


/* set the callback function for the res loader*/
void resSetLoadCallback(RESLOAD_CALLBACK funcToCall)
{
	resLoadCallback = funcToCall;
}

/* do the callback for the resload display function */
static inline void resDoResLoadCallback(void)
{
	if(resLoadCallback)
	{
		resLoadCallback();
	}
}


/* Initialise the resource module */
bool resInitialise(void)
{
	ASSERT( psResTypes == NULL,
		"resInitialise: resource module hasn't been shut down??" );
	psResTypes = NULL;
	resBlockID = 0;
	resLoadCallback = NULL;

	ResetResourceFile();

	return true;
}


/* Shutdown the resource module */
void resShutDown(void)
{
	if (psResTypes != NULL) {
		debug(LOG_WZ, "resShutDown: warning resources still allocated");
		resReleaseAll();
	}
}

// force set the base resource directory
void resForceBaseDir(const char* pResDir)
{
	sstrcpy(aResDir, pResDir);
	sstrcpy(aCurrResDir, pResDir);
}

// set the base resource directory
void resSetBaseDir(const char* pResDir)
{
	sstrcpy(aResDir, pResDir);
}

/* Parse the res file */
bool resLoad(const char *pResFile, SDWORD blockID)
{
	bool retval = true;
	lexerinput_t input;

	sstrcpy(aCurrResDir, aResDir);

	// Note the block id number
	resBlockID = blockID;

	debug(LOG_WZ, "resLoad: loading [directory: %s] %s", PHYSFS_getRealDir(pResFile), pResFile);

	// Load the RES file; allocate memory for a wrf, and load it
	input.type = LEXINPUT_PHYSFS;
	input.input.physfsfile = openLoadFile(pResFile, true);
	if (!input.input.physfsfile)
	{
		debug(LOG_FATAL, "Could not open file %s", pResFile);
		return false;
	}

	// and parse it
	res_set_extra(&input);
	if (res_parse() != 0)
	{
		debug(LOG_FATAL, "Failed to parse %s", pResFile);
		retval = false;
	}

	res_lex_destroy();
	PHYSFS_close(input.input.physfsfile);

	return retval;
}


/* Allocate a RES_TYPE structure */
static RES_TYPE* resAlloc(const char *pType)
{
	RES_TYPE	*psT;

#ifdef DEBUG
	// Check for a duplicate type
	for(psT = psResTypes; psT; psT = psT->psNext)
	{
		ASSERT(strcmp(psT->aType, pType) != 0, "Duplicate function for type: %s", pType);
	}
#endif

	// setup the structure
	psT = (RES_TYPE *)malloc(sizeof(RES_TYPE));
	sstrcpy(psT->aType, pType);
	psT->HashedType = HashString(psT->aType); // store a hased version for super speed !
	psT->psRes = NULL;

	return psT;
}


/* Add a buffer load function for a file type */
bool resAddBufferLoad(const char *pType, RES_BUFFERLOAD buffLoad, RES_FREE release)
{
	RES_TYPE	*psT = resAlloc(pType);

	psT->buffLoad = buffLoad;
	psT->fileLoad = NULL;
	psT->release = release;

	psT->psNext = psResTypes;
	psResTypes = psT;

	return true;
}


/* Add a file name load function for a file type */
bool resAddFileLoad(const char *pType, RES_FILELOAD fileLoad, RES_FREE release)
{
	RES_TYPE	*psT = resAlloc(pType);

	psT->buffLoad = NULL;
	psT->fileLoad = fileLoad;
	psT->release = release;

	psT->psNext = psResTypes;
	psResTypes = psT;

	return true;
}

// Make a string lower case
void resToLower(char *pStr)
{
	while (*pStr != 0)
	{
		if (isupper(*pStr))
		{
			*pStr = (char)(*pStr - (char)('A' - 'a'));
		}
		pStr += 1;
	}
}


static char LastResourceFilename[PATH_MAX];

/*!
 * Returns the filename of the last resource file loaded
 * The filename is always null terminated
 */
const char *GetLastResourceFilename(void)
{
	return LastResourceFilename;
}

/*!
 * Set the resource name of the last resource file loaded
 */
void SetLastResourceFilename(const char *pName)
{
	sstrcpy(LastResourceFilename, pName);
}


// Structure for each file currently in use in the resource  ... probably only going to be one ... but we will handle upto MAXLOADEDRESOURCE
struct RESOURCEFILE
{
  	char *pBuffer;	// a pointer to the data
	UDWORD size;	// number of bytes
	UBYTE	type;	// what type of resource is it
};

#define RESFILETYPE_EMPTY (0)			// empty entry
#define RESFILETYPE_LOADED (2)			// Loaded from a file (!)


#define MAXLOADEDRESOURCES (6)
static RESOURCEFILE LoadedResourceFiles[MAXLOADEDRESOURCES];



// Clear out the resource list ... needs to be called during init.
static void ResetResourceFile(void)
{
	UWORD i;

	for (i=0;i<MAXLOADEDRESOURCES;i++)
	{
		LoadedResourceFiles[i].type=RESFILETYPE_EMPTY;
	}
}

// Returns an empty resource entry or -1 if none exsist
static SDWORD FindEmptyResourceFile(void)
{
	UWORD i;
	for (i=0;i<MAXLOADEDRESOURCES ;i++ )
	  {
		if (LoadedResourceFiles[i].type==RESFILETYPE_EMPTY)
			return(i);

	  }
	return(-1);			// ERROR
}


// Get a resource data file ... either loads it or just returns a pointer
static bool RetreiveResourceFile(char *ResourceName, RESOURCEFILE **NewResource)
{
	SDWORD ResID;
	RESOURCEFILE *ResData;
	UDWORD size;
	char *pBuffer;

	ResID=FindEmptyResourceFile();
	if (ResID==-1) return(false);		// all resource files are full

	ResData= &LoadedResourceFiles[ResID];
	*NewResource=ResData;

	// This is needed for files that do not fit in the WDG cache ... (VAB file for example)
	if (!loadFile(ResourceName, &pBuffer, &size))
	{
		return false;
	}

	ResData->type=RESFILETYPE_LOADED;
	ResData->size=size;
	ResData->pBuffer=pBuffer;
	return(true);
}


// Free up the file depending on what type it is
static void FreeResourceFile(RESOURCEFILE *OldResource)
{
	switch (OldResource->type)
	{
		case RESFILETYPE_LOADED:
			free(OldResource->pBuffer);
			OldResource->pBuffer = NULL;
			break;

		default:
			debug(LOG_WARNING, "resource not freed");
	}


	// Remove from the list
	OldResource->type=RESFILETYPE_EMPTY;
}


static inline RES_DATA* resDataInit(const char *DebugName, UDWORD DataIDHash, void *pData, UDWORD BlockID)
{
	char* resID;

	// Allocate memory to hold the RES_DATA structure plus the identifying string
	RES_DATA *psRes = (RES_DATA *)malloc(sizeof(RES_DATA) + strlen(DebugName) + 1);
	if (!psRes)
	{
		debug(LOG_ERROR, "resDataInit: Out of memory");
		return NULL;
	}

	// Initialize the pointer for our ID string
	resID = (char*)(psRes + 1);

	// Copy over the identifying string
	strcpy(resID, DebugName);
	psRes->aID = resID;

	psRes->pData = pData;
	psRes->blockID = BlockID;
	psRes->HashedID = DataIDHash;

	psRes->usage = 0;

	return psRes;
}


/*!
 * check if given file exists in a locale dependend subdir
 * if so, modify given fileName to hold the locale dep. file,
 * else do not change given fileName
 * \param[in,out] fileName must be at least PATH_MAX bytes large
 * \param maxlen indicates the maximum buffer size of \c fileName
 */
static void makeLocaleFile(char* fileName, size_t maxlen)  // given string must have PATH_MAX size
{
#ifdef ENABLE_NLS
	const char * language = getLanguage();
	char localeFile[PATH_MAX];

	if ( language[0] == '\0' || // could not get language
		 strlen(fileName) + strlen(language) + 1 >= PATH_MAX )
	{
		return;
	}

	snprintf(localeFile, sizeof(localeFile), "locale/%s/%s", language, fileName);

	if ( PHYSFS_exists(localeFile) )
	{
		strlcpy(fileName, localeFile, maxlen);
		debug(LOG_WZ, "Found translated file: %s", fileName);
	}
#endif // ENABLE_NLS

	return;
}


/*!
 * Call the load function (registered in data.c)
 * for this filetype
 */
bool resLoadFile(const char *pType, const char *pFile)
{
	RES_TYPE	*psT = NULL;
	void		*pData = NULL;
	RES_DATA	*psRes = NULL;
	char		aFileName[PATH_MAX];
	UDWORD HashedName, HashedType = HashString(pType);

	// Find the resource-type
	for(psT = psResTypes; psT != NULL; psT = psT->psNext )
	{
		if (psT->HashedType == HashedType)
		{
			ASSERT(strcmp(psT->aType, pType) == 0, "Hash collision \"%s\" vs \"%s\"", psT->aType, pType);
			break;
		}
	}

	if (psT == NULL)
	{
		debug(LOG_WZ, "resLoadFile: Unknown type: %s", pType);
		return false;
	}

	// Check for duplicates
	HashedName = HashStringIgnoreCase(pFile);
	for (psRes = psT->psRes; psRes; psRes = psRes->psNext)
	{
		if(psRes->HashedID == HashedName)
		{
			ASSERT(strcasecmp(psRes->aID, pFile) == 0, "Hash collision \"%s\" vs \"%s\"", psRes->aID, pFile);
			debug(LOG_WZ, "Duplicate file name: %s (hash %x) for type %s",
			      pFile, HashedName, psT->aType);
			// assume that they are actually both the same and silently fail
			// lovely little hack to allow some files to be loaded from disk (believe it or not!).
			return true;
		}
	}

	// Create the file name
	if (strlen(aCurrResDir) + strlen(pFile) + 1 >= PATH_MAX)
	{
		debug(LOG_ERROR, "resLoadFile: Filename too long!! %s%s", aCurrResDir, pFile);
		return false;
	}
	sstrcpy(aFileName, aCurrResDir);
	sstrcat(aFileName, pFile);

	makeLocaleFile(aFileName, sizeof(aFileName));  // check for translated file

	SetLastResourceFilename(pFile); // Save the filename in case any routines need it

	// load the resource
	if (psT->buffLoad)
	{
		RESOURCEFILE *Resource;

		// Load the file in a buffer
		if (!RetreiveResourceFile(aFileName, &Resource))
		{
			debug(LOG_ERROR, "resLoadFile: Unable to retreive resource - %s", aFileName);
			return false;
		}

		// Now process the buffer data
		if (!psT->buffLoad(Resource->pBuffer, Resource->size, &pData))
		{
			ASSERT(false, "The load function for resource type \"%s\" failed for file \"%s\"", pType, pFile);
			FreeResourceFile(Resource);
			if (psT->release != NULL)
			{
				psT->release(pData);
			}
			return false;
		}

		FreeResourceFile(Resource);
	}
	else if(psT->fileLoad)
	{
		// Process data directly from file
		if (!psT->fileLoad(aFileName, &pData))
		{
			ASSERT(false, "The load function for resource type \"%s\" failed for file \"%s\"", pType, pFile);
			if (psT->release != NULL)
			{
				psT->release(pData);
			}
			return false;
		}
	}

	resDoResLoadCallback();		// do callback.

	// Set up the resource structure if there is something to store
	if (pData != NULL)
	{
		// LastResourceFilename may have been changed (e.g. by TEXPAGE loading)
		psRes = resDataInit( GetLastResourceFilename(), HashStringIgnoreCase(GetLastResourceFilename()), pData, resBlockID );
		if (!psRes)
		{
			if (psT->release != NULL)
			{
				psT->release(pData);
			}
			return false;
		}

		// Add the resource to the list
		psRes->psNext = psT->psRes;
		psT->psRes = psRes;
	}
	return true;
}

/* Return the resource for a type and hashedname */
void *resGetDataFromHash(const char *pType, UDWORD HashedID)
{
	RES_TYPE	*psT = NULL;
	RES_DATA	*psRes = NULL;
	// Find the correct type
	UDWORD HashedType = HashString(pType);

	for(psT = psResTypes; psT != NULL; psT = psT->psNext )
	{
		if (psT->HashedType==HashedType)
		{
			/* We found it */
			break;
		}
	}

	ASSERT( psT != NULL, "resGetDataFromHash: Unknown type: %s", pType );
	if (psT == NULL)
	{
		return NULL;
	}

	for(psRes = psT->psRes; psRes != NULL; psRes = psRes->psNext)
	{
		if (psRes->HashedID == HashedID)
		{
			/* We found it */
			break;
		}
	}

	ASSERT( psRes != NULL, "resGetDataFromHash: Unknown ID: %0x Type: %s", HashedID, pType );
	if (psRes == NULL)
	{
		return NULL;
	}

	psRes->usage += 1;

	return psRes->pData;
}


/* Return the resource for a type and ID */
void *resGetData(const char *pType, const char *pID)
{
	void * data = resGetDataFromHash(pType, HashStringIgnoreCase(pID));
	ASSERT(data != NULL, "resGetData: Unable to find data for %s type %s", pID, pType);
	return data;
}


bool resGetHashfromData(const char *pType, const void *pData, UDWORD *pHash)
{
	RES_TYPE	*psT;
	RES_DATA	*psRes;

	// Find the correct type
	UDWORD	HashedType=HashString(pType);

	for(psT = psResTypes; psT != NULL; psT = psT->psNext )
	{
		if (psT->HashedType==HashedType)
		{
			break;
		}
	}

	if (psT == NULL)
	{
		ASSERT( false, "resGetHashfromData: Unknown type: %x", HashedType );
		return false;
	}

	// Find the resource
	for(psRes = psT->psRes; psRes; psRes = psRes->psNext)
	{

		if (psRes->pData == pData)
		{
			break;
		}
	}

	if (psRes == NULL)
	{
		ASSERT( false, "resGetHashfromData:: couldn't find data for type %x\n", HashedType );
		return false;
	}

	*pHash = psRes->HashedID;

	return true;
}

const char* resGetNamefromData(const char* type, const void *data)
{
	RES_TYPE	*psT;
	RES_DATA	*psRes;
	UDWORD   HashedType;

	if (type == NULL || data == NULL)
	{
		return "";
	}

	// Find the correct type
	HashedType = HashString(type);

	// Find the resource table for the given type
	for (psT = psResTypes; psT != NULL; psT = psT->psNext)
	{
		if (psT->HashedType == HashedType)
		{
			break;
		}
	}

	if (psT == NULL)
	{
		ASSERT( false, "resGetHashfromData: Unknown type: %x", HashedType );
		return "";
	}

	// Find the resource in the resource table
	for(psRes = psT->psRes; psRes; psRes = psRes->psNext)
	{
		if (psRes->pData == data)
		{
			break;
		}
	}

	if (psRes == NULL)
	{
		ASSERT( false, "resGetHashfromData:: couldn't find data for type %x\n", HashedType );
		return "";
	}

	return psRes->aID;
}

/* Simply returns true if a resource is present */
bool resPresent(const char *pType, const char *pID)
{
	RES_TYPE	*psT;
	RES_DATA	*psRes;

	// Find the correct type
	UDWORD HashedType=HashString(pType);

	for(psT = psResTypes; psT != NULL; psT = psT->psNext )
	{
		if (psT->HashedType==HashedType)
		{
			break;
		}
	}

	/* Bow out if unrecognised type */
	ASSERT(psT != NULL, "resPresent: Unknown type");
	if (psT == NULL)
	{
		return false;
	}

	{
		UDWORD HashedID=HashStringIgnoreCase(pID);

		for(psRes = psT->psRes; psRes; psRes = psRes->psNext)
		{
			if (psRes->HashedID==HashedID)
			{
				/* We found it */
				break;
			}
		}
	}

	/* Did we find it? */
	if (psRes != NULL)
	{
		return (true);
	}

	return (false);
}


/* Release all the resources currently loaded and the resource load functions */
void resReleaseAll(void)
{
	RES_TYPE *psT, *psNT;

	resReleaseAllData();

	for(psT = psResTypes; psT != NULL; psT = psNT)
	{
		psNT = psT->psNext;
		free(psT);
	}

	psResTypes = NULL;
}


/* Release all the resources currently loaded but keep the resource load functions */
void resReleaseAllData(void)
{
	RES_TYPE *psT;
	RES_DATA *psRes, *psNRes;

	for (psT = psResTypes; psT != NULL; psT = psT->psNext)
	{
		for(psRes = psT->psRes; psRes != NULL; psRes = psNRes)
		{
			if (psRes->usage == 0)
			{
				debug(LOG_NEVER, "resReleaseAllData: %s resource: %s(%04x) not used", psT->aType, psRes->aID, psRes->HashedID);
			}

			if (psT->release != NULL)
			{
				psT->release(psRes->pData);
			}

			psNRes = psRes->psNext;
			free(psRes);
		}

		psT->psRes = NULL;
	}
}


// release the data for a particular block ID
void resReleaseBlockData(SDWORD blockID)
{
	RES_TYPE	*psT, *psNT;
	RES_DATA	*psPRes, *psRes, *psNRes;

	for(psT = psResTypes; psT != NULL; psT = psNT)
	{
		psPRes = NULL;
		for(psRes = psT->psRes; psRes; psRes = psNRes)
		{
			ASSERT(psRes != NULL, "resReleaseBlockData: null pointer passed into loop");

			if (psRes->blockID == blockID)
			{
				if (psRes->usage == 0)
				{
					debug(LOG_NEVER, "resReleaseBlockData: %s resource: %s(%04x) not used", psT->aType, psRes->aID,
					      psRes->HashedID);
				}
				if(psT->release != NULL)
				{
					psT->release( psRes->pData );
				}

				psNRes = psRes->psNext;
				free(psRes);

				if (psPRes == NULL)
				{
					psT->psRes = psNRes;
				}
				else
				{
					psPRes->psNext = psNRes;
				}
			}
			else
			{
				psPRes = psRes;
				psNRes = psRes->psNext;
			}
		}

		psNT = psT->psNext;
	}
}
