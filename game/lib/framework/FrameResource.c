/*
 * FrameResource.c
 *
 * Framework Resource file processing functions
 *
 */
// Now renamed to avoid clash with warzone windows resource.c
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "Frame.h"
#include "FrameResource.h"
#include "ResLY.h"

#include "wdg.h"
#include "multiwdg.h"


// control how data files are loaded
#ifdef WIN32   // I don't know what this is ... but it certainly doesn't work on the playstation  ... (it sounds like the .WDG cacheing to me!)
#define SINGLE_BUFFER_LOAD

#else
#include "cdPSX.h"

#endif

							  
extern void InitWDG(char *WdgFileName);

// Local prototypes
void ResetBinaryResourceTypeCount(void);
void AddBinaryResourceType(char *ResourceType);
static void ReleaseWRF(UBYTE **pBuffer);


static RES_TYPE *psResTypes=NULL;



// check to see if RES_TYPE entry is valie
// this is a NULL check on PC (linked list)
// and a NULL name check on PSX (static array)


#ifdef WIN32
#define resValidType(Type) (Type)
#define resNextType(Type)  (Type->psNext)
#define resGetResDataPointer(psRes) (psRes->pData)
#define resGetResBlockID(psRes) (psRes->blockID)
#else
#define resValidType(Type) (Type->HashedType)		// a null hashedtype indicates end of list
#define resNextType(Type)  (Type+1)
//// on the psx we need to unpack the data
#define resGetResDataPointer(psRes) ((void *)(((psRes->PackedDataID)&0x00ffffff)|0x80000000))
#define resGetResBlockID(psRes) ((UDWORD)(((psRes->PackedDataID)&0xff000000)>>24))
#endif







/* The initial resource directory and the current resource directory */
STRING	aResDir[FILE_MAXCHAR];
STRING	aCurrResDir[FILE_MAXCHAR];

// the current resource block ID
static SDWORD	resBlockID;

// buffer to load file data into
static UBYTE	*pFileBuffer = NULL;
static SDWORD	fileBufferSize = 0;

void ResetResourceFile(void);


// callback to resload screen.
static RESLOAD_CALLBACK resLoadCallback=NULL;
static RESPRELOAD_CALLBACK resPreLoadCallback=NULL;


void resSetPreLoadCallback(RESPRELOAD_CALLBACK funcToCall)
{
	resPreLoadCallback=funcToCall;
}



/* set the callback function for the res loader*/
VOID resSetLoadCallback(RESLOAD_CALLBACK funcToCall)
{
	resLoadCallback = funcToCall;
}

/* do the callback for the resload display function */
VOID resDoResLoadCallback()
{
	if(resLoadCallback)
	{
		resLoadCallback();
	}
}


/* Initialise the resource module */
BOOL resInitialise(void)
{
//	BOOL res;

	ASSERT((psResTypes == NULL,
		"resInitialise: resource module hasn't been shut down??"));
	psResTypes = NULL;

	resBlockID = 0;					  

	resLoadCallback=NULL;
	resPreLoadCallback=NULL;

	ResetResourceFile();

	ResetBinaryResourceTypeCount();		//Reset the binary types

#ifdef BINARY_PIES
	AddBinaryResourceType("IMD");
#endif
#ifdef BINARY_SCRIPTS
	AddBinaryResourceType("SCRIPT");
#endif

#ifdef PSX
	// we must now initialise the cache before checking the wdg
	//
	// This is because the cache setup code initialises the additional buffer for the wdg/wrf catalogs
	// which are loaded in WDG_SetCurrentWDG

		// if you allocate the cache to a size of zero then it will use the default area for the cache
		// on the PC this will be the display buffer	(NOT YET IMPLEMENTED)
		// on the PSX this will be the primative buffer

	FILE_InitialiseCache(0);		// 
	WDG_SetCurrentWDG("warzone.wdg");



#else	// the pc can handle it the old way
		FILE_InitialiseCache(2*1024*1024);		// set the cache to be 2meg for the time being ...!
//		WDG_SetCurrentWDG("warzone.wdg");
		if (!wdgMultiInit())
		{
			return FALSE;
		}

/*
	res=WDG_SetCurrentWDG("warzone.wdg");
	if (res==TRUE)	// only initialise the cache if the wdg is there!
	{
		// if you allocate the cache to a size of zero then it will use the default area for the cache

		// on the PC this will be the display buffer	(NOT YET IMPLEMENTED)

		// on the PSX this will be the primative buffer

		FILE_InitialiseCache(2*1024*1024);		// set the cache to be 2meg for the time being ...!
	}
*/
#endif



	
	return TRUE;
}


/* Shutdown the resource module */
void resShutDown(void)
{

	FILE_ShutdownCache();
	WDG_SetCurrentWDG(NULL);
	wdgMultiShutdown();

	if (psResTypes != NULL)
	{
		DBPRINTF(("resShutDown: warning resources still allocated"));
		resReleaseAll();
	}



}


// set the base resource directory
void resSetBaseDir(STRING *pResDir)
{
	strncpy(aResDir, pResDir, FILE_MAXCHAR - 1);
}

/* Parse the res file */
BOOL resLoad(STRING *pResFile, SDWORD blockID,
			 UBYTE *pLoadBuffer, SDWORD bufferSize,
			 BLOCK_HEAP *psMemHeap)
{
	UBYTE	*pBuffer;
	UDWORD	size;
	BLOCK_HEAP	*psOldHeap;
	BOOL bAllowWRFProcessing;

//	FILE_SetupCache(pLoadBuffer,bufferSize);

	strcpy(aCurrResDir, aResDir);

	// Note the buffer for file data
	pFileBuffer = pLoadBuffer;
	fileBufferSize = bufferSize;
	// Note the block id number
	resBlockID = blockID;


	bAllowWRFProcessing=WDG_AllowWRFs();


	// Do we want to check to see if the file is in the current WDG ?
	if (WDG_AllowWDGs()==TRUE)
	{
		BOOL bResult;

		psOldHeap = memGetBlockHeap();

		memSetBlockHeap(psMemHeap);

		bResult=WDG_ProcessWRF(pResFile, TRUE);	// we might need to return a little more than just a TRUE or FALSE here ... so that we know if the failed finding a wdg or at a later stage

		memSetBlockHeap(psOldHeap);

		// We we got all the files from the WDG we don't need to even consider the WRF
		if (bResult==TRUE) 
		{
			bAllowWRFProcessing=FALSE;
		}
	}

	if (bAllowWRFProcessing==TRUE)
	{

#ifndef FINALBUILD		// don't allow wrf's on final build
		// make sure the WRF doesn't get loaded into a block heap
		psOldHeap = memGetBlockHeap();
		memSetBlockHeap(NULL);


		// Load the RES file
		if (!LoadWRF(pResFile,&pBuffer,&size))
		{
			return FALSE;
		}



		// now set the memory system to use the block heap
		memSetBlockHeap(psMemHeap);

		// and parse it
		resSetInputBuffer(pBuffer, size);
		if (res_parse() != 0)
		{
			return FALSE;
		}

		// reset the memory system
		memSetBlockHeap(psOldHeap);

		ReleaseWRF(&pBuffer);
#else
		return FALSE;
#endif

	
	}

	return TRUE;
}

#ifdef WIN32

/* Allocate a RES_TYPE structure */
static BOOL resAlloc(STRING *pType, RES_TYPE **ppsFunc)
{
	RES_TYPE	*psT;

#ifdef DEBUG
	// Check for a duplicate type
	for(psT = psResTypes; psT; psT = psT->psNext)
	{
		ASSERT((strcmp(psT->aType, pType) != 0,
			"resAlloc: Duplicate function for type: %s", pType));
	}
#endif

	// Allocate the memory
	psT = (RES_TYPE *)MALLOC(sizeof(RES_TYPE));
	if (!psT)
	{
		DBERROR(("resAlloc: Out of memory"));
		return FALSE;
	}

	// setup the structure
	strncpy(psT->aType, pType, RESTYPE_MAXCHAR - 1);
	psT->aType[RESTYPE_MAXCHAR - 1] = 0;

	psT->HashedType=HashString(psT->aType);		// store a hased version for super speed !

	psT->psRes = NULL;

	*ppsFunc = psT;

	return TRUE;
}
#endif

#ifdef WIN32

/* Add a buffer load function for a file type */
BOOL resAddBufferLoad(STRING *pType, RES_BUFFERLOAD buffLoad,
					  RES_FREE release)
{
	RES_TYPE	*psT;

	if (!resAlloc(pType, &psT))
	{
		return FALSE;
	}

	psT->buffLoad = buffLoad;
	psT->fileLoad = NULL;
	psT->release = release;

	psT->psNext = psResTypes;
	psResTypes = psT;

	return TRUE;
}


/* Add a file name load function for a file type */
BOOL resAddFileLoad(STRING *pType, RES_FILELOAD fileLoad,
					RES_FREE release)
{
	RES_TYPE	*psT;

	if (!resAlloc(pType, &psT))
	{
		return FALSE;
	}

	psT->buffLoad = NULL;
	psT->fileLoad = fileLoad;
	psT->release = release;

	psT->psNext = psResTypes;
	psResTypes = psT;

	return TRUE;
}

#endif

// Make a string lower case
void resToLower(STRING *pStr)
{
	while (*pStr != 0)
	{
		if (isupper(*pStr))
		{
			*pStr = (STRING)(*pStr - (STRING)('A' - 'a'));
		}
		pStr += 1;
	}
}


static char LastResourceFilename[FILE_MAXCHAR];

// Returns the filename of the last resource file loaded
char *GetLastResourceFilename(void)
{
	return(LastResourceFilename);
}

// Set the resource name of the last resource file loaded
void SetLastResourceFilename(char *pName)
{
	strncpy(LastResourceFilename, pName, FILE_MAXCHAR-1);
	LastResourceFilename[FILE_MAXCHAR-1] = 0;
}


static UDWORD LastHashName;

// Returns the filename of the last resource file loaded
UDWORD GetLastHashName(void)
{
	return (LastHashName);
}

// Set the resource name of the last resource file loaded
void SetLastHashName(UDWORD HashName)
{
	LastHashName = HashName;
}


#ifdef WIN32

// load a file from disk into a fixed memory buffer
BOOL resLoadFromDisk(STRING *pFileName, UBYTE **ppBuffer, UDWORD *pSize)
{
	HANDLE	hFile;
	DWORD	bytesRead;
	BOOL	retVal;

	*ppBuffer = pFileBuffer;

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
	if (*pSize >= (UDWORD)fileBufferSize)
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

#endif


// Structure for each file currently in use in the resource  ... probably only going to be one ... but we will handle upto MAXLOADEDRESOURCE
typedef struct
{
  	UBYTE *pBuffer;	// a pointer to the data
	UDWORD size;	// number of bytes
	UBYTE	type;	// what type of resource is it
} RESOURCEFILE; 

#define RESFILETYPE_EMPTY (0)			// empty entry
#define RESFILETYPE_PC_SBL (1)			// Johns SBL stuff
#define RESFILETYPE_LOADED (2)			// Loaded from a file (!)
#define RESFILETYPE_WDGPTR (3)			// A pointer from the WDG cache


#define MAXLOADEDRESOURCES (6)
static RESOURCEFILE LoadedResourceFiles[MAXLOADEDRESOURCES];



// Clear out the resource list ... needs to be called during init.
void ResetResourceFile(void)
{
	UWORD i;

	for (i=0;i<MAXLOADEDRESOURCES;i++)
	{
		LoadedResourceFiles[i].type=RESFILETYPE_EMPTY;
	}  
}

// Returns an empty resource entry or -1 if none exsist
SDWORD FindEmptyResourceFile(void)
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
BOOL RetreiveResourceFile(char *ResourceName, RESOURCEFILE **NewResource)
{

	SDWORD ResID;
	RESOURCEFILE *ResData;
	UDWORD size;
	UBYTE *pBuffer;

	ResID=FindEmptyResourceFile();
	if (ResID==-1) return(FALSE);		// all resource files are full

	ResData= &LoadedResourceFiles[ResID];
	*NewResource=ResData;


#ifdef WIN32

	if (pFileBuffer &&
		resLoadFromDisk(ResourceName, &pBuffer, &size))
	{
		ResData->type=RESFILETYPE_PC_SBL;
		ResData->size=size;
		ResData->pBuffer=pBuffer;
		return(TRUE);
	}

#endif

	blockSuspendUsage();


	// This is needed for files that do not fit in the WDG cache ... (VAB file for example)
	if (!loadFile(ResourceName, &pBuffer, &size))
	{
		return FALSE;
	}

	blockUnsuspendUsage();

	ResData->type=RESFILETYPE_LOADED;
	ResData->size=size;
	ResData->pBuffer=pBuffer;
	return(TRUE);

}


// Free up the file depending on what type it is
void FreeResourceFile(RESOURCEFILE *OldResource)
{
	switch (OldResource->type)
	  {
		case RESFILETYPE_LOADED:
			FREE(OldResource->pBuffer);
	

			break;
	  }


	// Remove from the list
	OldResource->type=RESFILETYPE_EMPTY;  
}


/*
	Some routines to handle the loading of binary format files
	- any type of file that is added to the list using AddBinaryResourceType() will be presummed to be
	binary, and will be loaded from the directory called "olddirectoryname.bin"

	- This is currently only used for files of type IMD on the PSX (when ADD_PIEBINDIR is defined)

*/

#define MAXBINTYPENAME (6)	// Max number of letters that the type of name can be binary (i.e. SCRIPT = 6)
#define MAXBINTYPES (4)		// only four different types supported
char BinaryTypeNames[MAXBINTYPES][MAXBINTYPENAME+1];
UDWORD BinaryResourceTypeCount=0;

// Clear out the table of resource types that are stored as binary
void ResetBinaryResourceTypeCount(void)
{
	BinaryResourceTypeCount=0;
}


void AddBinaryResourceType(char *ResourceType)
{
	assert(strlen(ResourceType)<=MAXBINTYPENAME);		// Type name is too long (can only be 3 letters)
	assert(BinaryResourceTypeCount<=(MAXBINTYPES-1));	// too many types of binary file

	strcpy(BinaryTypeNames[BinaryResourceTypeCount],ResourceType);
	BinaryResourceTypeCount++;

}


void resDataInit(RES_DATA* psRes, STRING *DebugName, UDWORD DataIDHash, void *pData, UDWORD BlockID)
{
#ifdef WIN32

	psRes->pData = pData;
	psRes->blockID = resBlockID;

#else
	UDWORD PackedDataID=0;
	PackedDataID=((UDWORD)pData)&0x00ffffff;		// Just get lower 24 bits
	PackedDataID|=(BlockID<<24);					// Mask in the block ID
	psRes->PackedDataID=PackedDataID;

#endif
	psRes->HashedID=DataIDHash;

#ifdef DEBUG
#ifdef WIN32
		strcpy(psRes->aID,DebugName);
#endif
		psRes->usage = 0;
#endif
}


#ifndef FINALBUILD		// dont want this in final psx version

/* Call the load function for a file */
BOOL resLoadFile(STRING *pType, STRING *pFile)
{
	RES_TYPE	*psT;
//	UBYTE		*pBuffer;
//	UDWORD		size;
	void		*pData;
	RES_DATA	*psRes;
	STRING		aFileName[FILE_MAXCHAR];
	UDWORD		BinaryType;

	BOOL loadresource;



	loadresource=TRUE;
	if(resPreLoadCallback)
	{
		loadresource=resPreLoadCallback(pType,pFile,aCurrResDir);
	}

	
	
	if (loadresource==TRUE)
	{
		UDWORD HashedType=HashString(pType);

		for(psT = psResTypes; resValidType(psT); psT = resNextType(psT) )
		{
			if (psT->HashedType==HashedType)
			{
				break;
			}
		}

		if (psT == NULL)
		{
			DBERROR(("resLoadFile: Unknown type: %s", pType));
			return FALSE;
		}

//	#ifdef DEBUG
		{
			UDWORD HashedName=HashStringIgnoreCase(pFile);
			for (psRes = psT->psRes; psRes; psRes = psRes->psNext)
			{
				if(psRes->HashedID == HashedName)
				{
#ifdef WIN32
					DBPRINTF(("resLoadFile: Duplicate file name: %s (hash %x) for type %s",pFile, HashedName, psT->aType));
#else
					DBPRINTF(("resLoadFile: Duplicate file name: %s (hash %x) for type %x",pFile, HashedName, psT->HashedType));
#endif
					//assert(2+2==5);

					// assume that they are actually both the same and silently fail
					// lovely little hack to allow some files to be loaded from disk (believe it or not!).
					return TRUE;
				}
			}

		}



//	#endif

		// Create the file name
		if (strlen(aCurrResDir) + strlen(pFile) + 1 >= FILE_MAXCHAR)
		{
			DBERROR(("resLoadFile: Filename too long!!\n%s%s", aCurrResDir, pFile));
			return FALSE;
		}
		strcpy(aFileName, aCurrResDir);





	// Check to see if it's a type of file that is stored as BINARY , if it is then add .bin to the directory name
		for(BinaryType=0;BinaryType<BinaryResourceTypeCount;BinaryType++)
		{
			if (strcmp(pType,BinaryTypeNames[BinaryType])==0)
			{
	#ifdef DEBUG
	//			DBPRINTF(("BINARY TYPE FOUND - [%s]\n",pType));
	#endif
				aFileName[strlen(aFileName)-1]=0;	// shorten string by 1 character to remove slash
				strcat(aFileName,".BIN\\");			// add the binary directory extension		
				break;								// exit out of the loop
			}
		}


		strcat(aFileName, pFile);

		strcpy(LastResourceFilename,pFile);	// Save the filename in case any routines need it
		resToLower(LastResourceFilename);
		SetLastHashName(HashStringIgnoreCase(LastResourceFilename));

		// load the resource
		if (psT->buffLoad)
		{

			RESOURCEFILE *Resource;
			BOOL Result;

			Result=RetreiveResourceFile(aFileName,&Resource);
			if (Result==FALSE)
			{
				DBERROR(("resLoadFile: Unable to retreive resource - %d",aFileName));
				return(FALSE);
			}
			
	// Now process the buffer data
			if (!psT->buffLoad(Resource->pBuffer, Resource->size, &pData))
			{
				return FALSE;
			}

			FreeResourceFile(Resource);

			resDoResLoadCallback();		// do callback.

		}
	#ifdef NORESHASH //WIN32
		else if (psT->fileLoad)
		{
			if (!psT->fileLoad(aFileName, &pData))
			{
				return FALSE;
			}
		}
	#endif
		else
		{
			DBERROR(("resLoadFile:  No load functions for this type (%s)\n",pType));
			return FALSE;
		}

		// Set up the resource structure if there is something to store
		if (pData != NULL)
		{
			psRes = MALLOC(sizeof(RES_DATA));
			if (!psRes)
			{
				DBERROR(("resLoadFile: Out of memory"));
				psT->release(pData);
				return FALSE;
			}
			// LastResourceFilename may have been changed (e.g. by TEXPAGE loading)
			resDataInit(psRes,LastResourceFilename,HashStringIgnoreCase(LastResourceFilename),pData,resBlockID);


			// Add the resource to the list
			psRes->psNext = psT->psRes;
			psT->psRes = psRes;
		}
	}
		return TRUE;
}
#endif	// ifndef FINALBUILD


/* Return the resource for a type and hashedname */
void *resGetDataFromHash(STRING *pType, UDWORD HashedID)
{
	RES_TYPE	*psT;
	RES_DATA	*psRes;
//	STRING		aID[RESID_MAXCHAR];
	UDWORD HashedType;
	// Find the correct type

	HashedType=HashString(pType);	// la da la


	for(psT = psResTypes; resValidType(psT); psT = resNextType(psT) )
	{
		if (psT->HashedType==HashedType)
		{
			break;
		}
	}
	if (psT == NULL)
	{
#ifdef PSX
		DBPRINTF(("resGetData: Unknown type: %s\n", pType));
#endif
		ASSERT((FALSE, "resGetData: Unknown type: %s", pType));
		return NULL;
	}

	{
//		UDWORD HashedID=HashStringIgnoreCase(pID);
		for(psRes = psT->psRes; psRes; psRes = psRes->psNext)
		{
			if (psRes->HashedID==HashedID)
			{
				/* We found it */
				break;
			}
		}
	}



	if (psRes == NULL)
	{
#ifdef PSX
		DBPRINTF(("resGetDataFromHash: Unknown ID:"));
#endif
		ASSERT((FALSE, "resGetDataFromHash: Unknown ID:"));
		return NULL;
	}

#ifdef DEBUG
	psRes->usage += 1;
#endif

	return resGetResDataPointer(psRes);
}


/* Return the resource for a type and ID */
void *resGetData(STRING *pType, STRING *pID)
{
	RES_TYPE	*psT;
	RES_DATA	*psRes;
//	STRING		aID[RESID_MAXCHAR];
	UDWORD HashedType;
	// Find the correct type

	HashedType=HashString(pType);	// la da la


	for(psT = psResTypes; resValidType(psT); psT = resNextType(psT) )
	{
		if (psT->HashedType==HashedType)
		{
			break;
		}
	}
	if (psT == NULL)
	{
#ifdef PSX
		DBPRINTF(("resGetData: Unknown type: %s\n", pType));
#endif
		ASSERT((FALSE, "resGetData: Unknown type: %s", pType));
		return NULL;
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



	if (psRes == NULL)
	{
#ifdef PSX
		DBPRINTF(("resGetData: Unknown ID: %s\n", pID));
#endif
		ASSERT((FALSE, "resGetData: Unknown ID: %s", pID));
		return NULL;
	}

#ifdef DEBUG
	psRes->usage += 1;
#endif

	return resGetResDataPointer(psRes);
}


BOOL resGetHashfromData(STRING *pType, void *pData, UDWORD *pHash)
{
	RES_TYPE	*psT;
	RES_DATA	*psRes;

	// Find the correct type
	UDWORD	HashedType=HashString(pType);

	for(psT = psResTypes; resValidType(psT); psT = resNextType(psT) )
	{
		if (psT->HashedType==HashedType)
		{
			break;
		}
	}

	if (psT == NULL)
	{
		ASSERT((FALSE, "resGetHashfromData: Unknown type: %x", HashedType));
		return FALSE;
	}


	// Find the resource
	for(psRes = psT->psRes; psRes; psRes = psRes->psNext)
	{

		if (resGetResDataPointer(psRes) == pData)
		{
			break;
		}
	}

	if (psRes == NULL)
	{
		ASSERT((FALSE, "resGetHashfromData:: couldn't find data for type %x\n", HashedType));
		return FALSE;
	}

	*pHash = psRes->HashedID;

	return TRUE;
}



#if(0)
// return the ID string for a piece of data
// This now no longer works ... this is used for the savegame code ... sorry jeremey
// now use resGetHashfromData()  instead
BOOL resGetIDfromData(STRING *pType, void *pData, STRING **ppID)
{
#ifdef NORESHASH //WIN32
	RES_TYPE	*psT;
	RES_DATA	*psRes;

	// Find the correct type
	for(psT = psResTypes; resValidType(psT); psT = resNextType(psT) )
	{
		if (strcmp(psT->aType, pType) == 0)
		{
			break;
		}
	}
	if (psT == NULL)
	{
#ifdef PSX
		DBPRINTF(("resGetData: Unknown type: %s\n", pType));
#endif
		ASSERT((FALSE, "resGetData: Unknown type: %s", pType));
		return FALSE;
	}


	// Find the resource
	for(psRes = psT->psRes; psRes; psRes = psRes->psNext)
	{

		if (resGetResDataPointer(psRes) == pData)
		{
			break;
		}
	}

	if (psRes == NULL)
	{
#ifdef PSX
		DBPRINTF(("resGetIDfromData: couldn't find data for type %s\n", pType));
#endif
		ASSERT((FALSE, "resGetIDfromData: couldn't find data for type %s\n", pType));
		return FALSE;
	}

	*ppID = psRes->aID;

	return TRUE;
#else
	return FALSE;
#endif
}
#endif

/* Simply returns true if a resource is present */
BOOL resPresent(STRING *pType, STRING *pID)
{
	RES_TYPE	*psT;
	RES_DATA	*psRes;
//	STRING		aID[RESID_MAXCHAR];

	// Find the correct type
	UDWORD HashedType=HashString(pType);

	for(psT = psResTypes; resValidType(psT); psT = resNextType(psT) )
	{
		if (psT->HashedType==HashedType)
		{
			break;
		}
	}

	/* Bow out if unrecognised type */
	if (psT == NULL)
	{
//		ASSERT((FALSE, "resPresent: Unknown type"));
		return FALSE;
	}

	{
		UDWORD HashedID=HashStringIgnoreCase(pID);
//		DBPRINTF(("%x - %d\n",HashedID,pID));
		for(psRes = psT->psRes; psRes; psRes = psRes->psNext)
		{
//	DBPRINTF(("!= %x\n",psRes->HashedID));
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
		return (TRUE);
	}

	return (FALSE);
}


/* Release all the resources currently loaded and the resource load functions */
void resReleaseAll(void)
{
	RES_TYPE	*psT, *psNT;
	RES_DATA	*psRes, *psNRes;

	for(psT = psResTypes; resValidType(psT); psT = psNT)
	{
		for(psRes = psT->psRes; psRes; psRes = psNRes)
		{
#ifdef DEBUG
			if (psRes->usage == 0)
			{
#ifdef WIN32
				DBPRINTF(("%s resource: %s(%04x) not used\n", psT->aType, psRes->aID,psRes->HashedID));
#else
				DBPRINTF(("type %x resource: %x not used\n", psT->HashedType, psRes->HashedID));
#endif

			}
#endif
			if(psT->release != NULL) {
				psT->release( resGetResDataPointer(psRes) );
			} else {
				ASSERT((FALSE,"resReleaseAll: NULL release function"));
			}
			psNRes = psRes->psNext;
			FREE(psRes);
		}
		psNT = resNextType(psT);
#ifdef WIN32
		FREE(psT);
#endif
	}

	psResTypes = NULL;
}


// release the data for a particular block ID
void resReleaseBlockData(SDWORD blockID)
{
	RES_TYPE	*psT, *psNT;
	RES_DATA	*psPRes, *psRes, *psNRes;

	for(psT = psResTypes; resValidType(psT); psT = psNT)
	{
		psPRes = NULL;
		for(psRes = psT->psRes; psRes; psRes = psNRes)
		{
			ASSERT((psRes != NULL,"resReleaseBlockData: null pointer passed into loop"));

			if (resGetResBlockID(psRes) == blockID)
			{
#ifdef DEBUG
				if (psRes->usage == 0)
				{
#ifdef WIN32
					DBPRINTF(("%s resource: %x not used\n", psT->aType, psRes->HashedID));
#else
					DBPRINTF(("%x resource: %x not used\n", psT->HashedType, psRes->HashedID));
#endif
				}
#endif
				if(psT->release != NULL)
				{
					psT->release( resGetResDataPointer(psRes) );
				}
				else
				{
					ASSERT((FALSE,"resReleaseAllData: NULL release function"));
				}

				psNRes = psRes->psNext;
				FREE(psRes);

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
			ASSERT((psNRes != (RES_DATA *)0xdddddddd,"resReleaseBlockData: next data (next pointer) already freed"));
		}
		psNT = resNextType(psT);
		ASSERT((psNT != (RES_TYPE *)0xdddddddd,"resReleaseBlockData: next data (next pointer) already freed"));

	}
}


/* Release all the resources currently loaded but keep the resource load functions */
void resReleaseAllData(void)
{
	RES_TYPE	*psT, *psNT;
	RES_DATA	*psRes, *psNRes;

	for(psT = psResTypes; resValidType(psT); psT = psNT)
	{
		for(psRes = psT->psRes; psRes; psRes = psNRes)
		{
#ifdef DEBUG
			if (psRes->usage == 0)
			{
#ifdef WIN32
					DBPRINTF(("%s resource: %x not used\n", psT->aType, psRes->HashedID));
#else
					DBPRINTF(("%x resource: %x not used\n", psT->HashedType, psRes->HashedID));
#endif
			}
#endif

			if(psT->release != NULL) {
				psT->release( resGetResDataPointer(psRes) );
			} else {
				ASSERT((FALSE,"resReleaseAllData: NULL release function"));
			}

			psNRes = psRes->psNext;
			FREE(psRes);
		}
		psT->psRes = NULL;
		psNT = resNextType(psT);
	}
}





			

#ifndef FINALBUILD	// don't allow wrfs in final build



#define MAXWRFSIZE (64000)	// the max size of a wrf file (this amount is taken out of the cache space)
//								  	
//
// allocate memory for a wrf, and load it 
//
// Because we on use WRF's on non final version on the PSX we can malloc the memory for it now !
//    ... this should make the save game work (as if by magic)
//
//
BOOL LoadWRF(char *pResFile, UBYTE **pBuffer, UDWORD *size)
{

#ifdef PSX
	UBYTE WRFname[256];

/*

	UBYTE *WRFAddress;

	assert(GetPrimBufferAllocatedSize()==0);		// make sure that we arent using the prim buffer already

	WRFAddress=AllocInPrimBuffers(MAXWRFSIZE);		// load the wrf into this address
*/
	strcpy(WRFname,"WRF\\");
	strcat(WRFname,pResFile);
/*
	*pBuffer=WRFAddress;
	*size=MAXWRFSIZE;

	return (loadFile2(WRFname,pBuffer,size,FALSE));	  // we don't want loadfile to allocate any memory ... we are passing in the address to load the file
*/
	return (loadFile2(WRFname,pBuffer,size,TRUE));		// on the PC (and now the PSX) we just allocate the memory 
#else
	return (loadFile2(pResFile,pBuffer,size,TRUE));		// on the PC (and now the PSX) we just allocate the memory 
#endif


}


static void ReleaseWRF(UBYTE **pBuffer)
{
#if (0)	//PSX  // now the Psx frees the mem as well
	ResetPrimBuffers();	   // free up the mem used in the prim buffers for the wrf file
#else
// this is here for when we free up a old wrf

	// now free up the memory for the .wrf file
	FREE(*pBuffer);

#endif
}

#endif
		











static UDWORD CurrentFileNameHash;


// Define the table of resource load functions - PSX only
void resDefineLoadFuncTable(RES_TYPE*ResourceTypes)
{
	RES_TYPE *CurrRes;
	psResTypes=ResourceTypes;	

	// Generate the hashed names of the ascii types
	CurrRes=psResTypes;
	while(CurrRes->aType)
	{
//		DBPRINTF(("type = [%s]\n",CurrRes->aType));
		CurrRes->HashedType=HashString(CurrRes->aType);
		CurrRes++;
	}



}
					 



// Now call the correct routine based on the file type
BOOL FILE_ProcessFile(WRFINFO *CurrentFile, UBYTE *pRetreivedFile)
{
	RES_TYPE	*psT;
	void		*pData;	// return data from buffer loading ... is this used any more ?

	// first scan through all the types finding a matching one
	for(psT = psResTypes; resValidType(psT); psT = resNextType(psT) )
	{
		if (psT->HashedType== CurrentFile->type) 
		{
			break;
		}
	}
	if (psT==NULL)		// none found then error and panic
	{
		DBPRINTF(("Unknown resource type %x\n",CurrentFile->type));
		return FALSE;
	}

	// This is the normal hash value that is used to indentify the resource
	// it is normally just a hased version of the filename
	// however sometime the filename can change in buffload (e.g. when loading texture pages)
	// we need the ability to adjust the hash value as well! - absoulutly insane!
	CurrentFileNameHash=CurrentFile->name;

// Now process the buffer data by calling the relevant buffer command
	if (!psT->buffLoad(pRetreivedFile, CurrentFile->filesize, &pData))
	{
#ifdef WIN32
		DBPRINTF(("No buffer command for this type %s\n",psT->aType));
#else
		DBPRINTF(("No buffer command for this type %x\n",psT->HashedType));
#endif
		return FALSE;
	}


	if (pData != NULL)
	{
		RES_DATA *psRes;

		psRes = MALLOC(sizeof(RES_DATA));
		if (!psRes)
		{
			DBERROR(("resLoadFile: Out of memory"));
			psT->release(pData);
			return FALSE;
		}

		// LastResourceFilename may have been changed (e.g. by TEXPAGE loading)
		// So we need to adjust the hashed name as well

//		resDataInit(psRes,CurrentFile->name,pData,resBlockID);

		resDataInit(psRes,"a wdg file",CurrentFileNameHash,pData,resBlockID);

		// Add the resource to the list
		psRes->psNext = psT->psRes;
		psT->psRes = psRes;
	}

	return TRUE;		 
}

void SetLastResourceHash(char *fname)
{

	CurrentFileNameHash=HashStringIgnoreCase(fname);
}

