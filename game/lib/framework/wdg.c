

 /*******************************************************************
 *
 *    DESCRIPTION:	 Routines to handle format 4 .WDG files for PSX & PC
 *
 *    AUTHOR:		 Tim ... at first
 *
 *    HISTORY:       
 *
 *******************************************************************/

/** include files **/

#include <assert.h>

#include "Frame.h"
#include "wdg.h"
#include "MultiWDG.h"

#ifdef PSX
#include "cdpsx.h"
#include "file_psx.h"
#endif

/** local definitions **/

#ifdef PSX
//#define	PRIMCATALOG		// define this if we want to use the primative buffer to store the wdg catalogs - frees up 29k (!)
#else
//#define	PRIMCATALOG		// define this if we want to use the primative buffer to store the wdg catalogs - frees up 29k (!)
#endif



#ifndef PSX_USECD

#define MAX_STR (256)

typedef FILE DISK_FILE;

#define DISK_OpenFile(name)							   fopen(name,"rb")
#define DISK_Close(handle)							   fclose(handle)
#define DISK_ReadPos(filepos,buffer,bytescount,handle) freadpos(filepos,buffer,bytescount,handle)

int freadpos(UDWORD filepos,UBYTE *buffer, UDWORD bytescount, FILE *handle)
{
	fseek(handle,filepos,SEEK_SET);
	return (fread(buffer,1,bytescount,handle));
}

#else
// This is for PSX CD code only 
typedef UDWORD DISK_FILE;		// this isn't used !

#define DISK_OpenFile(name)								OpenCDfile(name)
#define DISK_ReadPos(filepos,buffer,bytecount,handle)	ReadCDdata(filepos,buffer,bytecount)	
#define DISK_Close(handle)								CloseCDfile()

#define MAX_STR (32)
#endif



/* default settings */

/** external functions **/
BOOL FILE_ProcessFile(WRFINFO *CurrentFile, UBYTE *pRetreivedFile);
void SetLastFnameExtra(UBYTE SoftwareFlag, SBYTE TexturePage);

/** external data **/

/** internal functions **/
UBYTE *FILE_Retrieve(FILE *pFileHandle, UDWORD offsetInWDG, UDWORD filesize);
void FILE_InvalidateCache(void);
BOOL FILE_RestoreCache(void);

/** public data **/

/** private data **/

// used for putting the data into the primative buffer

#ifdef PRIMCATALOG

#define CHECK1 (0x12341234)
#define CHECK2 (0x56785678)

typedef struct
{
	UDWORD	Check1;				// check to make sure that nothing has zapped the data
	WDGINFO	primWRFCatalog[MAXWRFINWDG];
	WRFINFO primWRFfilesCatalog[MAXFILESINWRF];
	UDWORD	Check2;				// check to make sure that nothing has zapped the data
} PRIMBUFFERCATALOG;

PRIMBUFFERCATALOG *PrimBufferCatalog=NULL;		// NULL indicates invalid data

#else
//static WDGINFO WRFCatalog[MAXWRFINWDG];		// static area for the catalog of wrf files in this wdg
//static WRFINFO WRFfilesCatalog[MAXFILESINWRF];  // static area for list of files in the current wrf

// change this to a pointer so that it can be set by the multi WDG
static WDGINFO *WRFCatalog;
static WRFINFO *WRFfilesCatalog;
#endif


static UDWORD NumberOfWRFfiles=0;
static UDWORD LastCatalogLoadedOffset=0;


CACHE Cache={ NULL,0,0,FALSE };

/** public functions **/

char CurrentWDGname[MAX_STR];		// name of the current wdg

// set the current WDG and it's catalogs
void WDG_SetCurrentWDGCatalog(char *filename, UDWORD numWRF, WDGINFO *psWRFCatalog)
{
	strcpy(CurrentWDGname,filename);
	NumberOfWRFfiles = numWRF;
	WRFCatalog = psWRFCatalog;

	// now invalidate the caches
	LastCatalogLoadedOffset=0;
	FILE_InvalidateCache();
}

// set the current WRF file catalog
void WDG_SetCurrentWRFFileCatalog(UDWORD catOffset, WRFINFO *psFileCatalog)
{
	LastCatalogLoadedOffset = catOffset;
	WRFfilesCatalog = psFileCatalog;

	FILE_InvalidateCache();
}

// get the current WDG and it's catalogs
void WDG_GetCurrentWDGCatalog(char **ppFileName, UDWORD *pNumWRF, WDGINFO **ppsWRFCatalog)
{
	*ppFileName = CurrentWDGname;
	*pNumWRF = NumberOfWRFfiles;
	*ppsWRFCatalog = WRFCatalog;
}

// get the current WRF file catalog
void WDG_GetCurrentWFRFileCatalog(UDWORD *pCatOffset, WRFINFO **ppsFileCatalog)
{
	*pCatOffset = LastCatalogLoadedOffset;
	*ppsFileCatalog = WRFfilesCatalog;
}

/*
 ** WDG_SetCurrentWDG
 *
 *  FILENAME: C:\Deliverance\libpsx\Framework\wdg.c
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION: This routine checks that the requested WDG exists and loads up the WRF directory for it
 *
 *  RETURNS:
 *
 */
BOOL WDG_SetCurrentWDG(char *filename)
{
	DISK_FILE *pFileHandle;
	WDG_HEADER		NewHeader;
	WDG_HEADER_V5	HeaderV5;
	UDWORD BytesRead, headersize, size;

	if (filename==NULL)
	{
		NumberOfWRFfiles=0;
		CurrentWDGname[0]=0;
		return TRUE;
	}
	pFileHandle=DISK_OpenFile(filename);	// tries to open the WDG on the HD (pc) or CD  (psx)
	if (pFileHandle==NULL)
	{
		DBPRINTF(("WDG_SetCurrentWDG unable to open %s\n",filename));
		return FALSE;	
	}

	// okay, the file exsists (because we succsesfully opened it), now read in the WDG header
	BytesRead=DISK_ReadPos(0,(UBYTE *)&NewHeader,sizeof(WDG_HEADER),pFileHandle);
	if (BytesRead!=sizeof(WDG_HEADER))
	{
		DBPRINTF(("WDG_SetCurrentWDG unable to read from %s\n",filename));
		DISK_Close(pFileHandle);  
		return FALSE;	
	}
	
	// right, we read in the header now check the values to make sure its okay
	if (NewHeader.WDGid[0]!='W' ||
		NewHeader.WDGid[1]!='D' ||
		NewHeader.WDGid[2]!='G')
	{
		DBPRINTF(("WDG_SetCurrentWDG bad type of wdg file - %s\n",filename));
		DISK_Close(pFileHandle);  
		return FALSE;	
	}

	NumberOfWRFfiles=NewHeader.NumWRFFiles;

	if (NewHeader.WDGid[3]=='5')
	{
		// got new format WDG file
		BytesRead=DISK_ReadPos(0,(UBYTE *)&HeaderV5,sizeof(WDG_HEADER_V5),pFileHandle);
		if (BytesRead!=sizeof(WDG_HEADER_V5))
		{
			DBPRINTF(("WDG_SetCurrentWDG unable to read from %s\n",filename));
			DISK_Close(pFileHandle);  
			return FALSE;	
		}

		headersize = sizeof(WDG_HEADER_V5) + HeaderV5.numDep * sizeof(WDG_DEP);
	}
	else
	{
		headersize = sizeof(WDG_HEADER);
	}

	assert(NumberOfWRFfiles<=MAXWRFINWDG);	// make sure that we don't have too many wrf's


//	prnt(1,"Setcurrentwdg\n",0,0);

	// now we read in the catalog of all the WRF files in this WDG
#ifdef PRIMCATALOG
	assert(PrimBufferCatalog);	// make sure the cache is valid

	assert(PrimBufferCatalog->Check1==CHECK1);
	assert(PrimBufferCatalog->Check2==CHECK2);


	BytesRead=DISK_ReadPos(headersize,PrimBufferCatalog->primWRFCatalog,sizeof(WDGINFO)*NumberOfWRFfiles,pFileHandle);
#else
	BytesRead=DISK_ReadPos(headersize,(UBYTE *)WRFCatalog,sizeof(WDGINFO)*NumberOfWRFfiles,pFileHandle);
#endif
	size = sizeof(WDGINFO)*NumberOfWRFfiles;
	if (BytesRead!=size)
	{
		DBPRINTF(("WDG_SetCurrentWDG unable to read from %s\n",filename));
		DISK_Close(pFileHandle);  
		return FALSE;	
	}
	DISK_Close(pFileHandle);  
	strcpy(CurrentWDGname,filename);


	return TRUE;
}



static BOOL CheckCurrentWDGforWRF(char *wrfname,WDGINFO ** WRFentry)
{
	BOOL		FoundWRF;
	WDGINFO		*CurrentWRF;	// Pointer to the current WRF file in the WDG
	UDWORD		WRFHash,WRF;

	WRFHash=HashStringIgnoreCase(wrfname);

#ifdef PRIMCATALOG
	assert(PrimBufferCatalog);	// make sure the cache is valid
	assert(PrimBufferCatalog->Check1==CHECK1);
	assert(PrimBufferCatalog->Check2==CHECK2);
	CurrentWRF=PrimBufferCatalog->primWRFCatalog;
#else
	CurrentWRF=WRFCatalog;	
#endif

	FoundWRF=FALSE;


	for (WRF=0;WRF<NumberOfWRFfiles;WRF++)
	{
//		char t[256];
//		sprintf(t,"wrf %d) %s %x - %x\n",WRF,wrfname,WRFHash,CurrentWRF->WRFname);
//		prnt(1,t,0,0);

		if (WRFHash==CurrentWRF->WRFname)		// match the hash
		{
			FoundWRF=TRUE;
			*WRFentry=CurrentWRF;
			break;
		}
		CurrentWRF++;
	}
	return FoundWRF;
}

BOOL LoadWRFCatalog(WDGINFO *CurrentWRF, FILE *pFileHandle)
{
	UDWORD BytesRead;


//	UBYTE string[64];
//	sprintf(string,"loadwrf [%x]\n",CurrentWRF->offset);
	// now load the directory of the wrf

//	prnt(1,string,0,0);

	// okay, the file exsists (because we succsesfully opened it), now read in the WDG header
	// CurrentWRF->filecount		// contains the number of files in the WRF


#ifdef PRIMCATALOG
	assert(PrimBufferCatalog);	// make sure the cache is valid
	assert(PrimBufferCatalog->Check1==CHECK1);
	assert(PrimBufferCatalog->Check2==CHECK2);
	assert(CurrentWRF->filecount < MAXFILESINWRF);
	BytesRead=DISK_ReadPos(CurrentWRF->offset,PrimBufferCatalog->primWRFfilesCatalog,sizeof(WRFINFO)*CurrentWRF->filecount,pFileHandle);
#else

// only reload when needed
// what the hell it's the PC so do it all the time ...
//	if (LastCatalogLoadedOffset!=CurrentWRF->offset)
	{

		BytesRead=DISK_ReadPos(CurrentWRF->offset,(UBYTE *)WRFfilesCatalog,sizeof(WRFINFO)*CurrentWRF->filecount,pFileHandle);
		if (BytesRead!=sizeof(WRFINFO)*CurrentWRF->filecount)
		{
			DBPRINTF(("WDG_ProcessWRF unable to read from %s\n",CurrentWDGname));
			return FALSE;	
		}
		LastCatalogLoadedOffset=CurrentWRF->offset;
	}
#endif

	return TRUE;
}


#define MAXSKIPAMOUNT (2048)	// largest amount of file to skip by reading

/*
 ** WDG_ProcessWRF
 *
 *  FILENAME: C:\Deliverance\libpsx\Framework\wdg.c
 *
 *  PARAMETERS: UseDataFromWDG is true when we wish to load the data from a WDG in preference to direct loading it from the HD
 *
 *  DESCRIPTION:  Loads up all the files in the WRF ... looks in the current WDG for the WRF 
 *					Also handles directly loading of files if now WDG found
 *  RETURNS:
 *
 */

// We need to support the primative buffer cache on the playstation
//  ... this allows us to load up as much of the file as memory holds 

// It means that we don't want the files in each wrf to be spaced out on a 2048 byte boundary  ... needs fixing in MAKEWDG




/*

						

*/


BOOL WDG_ProcessWRF(char *WRFname,BOOL UseDataFromWDG )
{
	WDGINFO *CurrentWRF;	// Pointer to the current WRF file in the WDG
//	BOOL FoundWRF;			// Have we had a match for the WRF
	BOOL CatalogLoadedOK;
	FILE *pFileHandle;
	WRFINFO	*CurrentFile;  //=WRFfilesCatalog;
	UDWORD File;
	UDWORD pos;
	UDWORD CurrentWRF_HeaderSize;
	char wrfentryname[32];
	WDG_FINDWRF	sFindWRF;


// THIS WORKS !!!!!!
//	prnt(1,"hello\n",0,0);
//	prnt(1,WRFname,0,0);

	// First check that we have a valid WDG open
	if (NumberOfWRFfiles==0)		// = indicates that we have not initialisied a WDG
	{
		return FALSE;
	}

	// remove any preceeded directory stuff
	pos=strlen(WRFname);
	while (pos>0)
	{
		if (WRFname[pos]=='\\')
		{
			pos++;		// we need to skip the "\"
			break;
		}

		pos--;
	}
	strcpy(wrfentryname,&WRFname[pos]);


	// Make sure the correct WDG is loaded
	memset(&sFindWRF, 0, sizeof(WDG_FINDWRF));
	if (!wdgFindFirstWRF(HashStringIgnoreCase(wrfentryname), &sFindWRF))
	{
		return FALSE;
	}
	wdgSetCurrentWDGFromFind(&sFindWRF);
	CurrentWRF = WRFCatalog + sFindWRF.currWRFIndex;

	// if the cache has become invalid then reload it
	//
	// The cache also holds the wrf/wdg catalog infomation ... this needs to be reloaded before we can continue to load a wrf
//	prnt(1,"CHECK\n",0,0);

#ifdef PRIMCATALOG			// if not primcatalog we always must restore
	if ( FILE_IsCatalogValid()==FALSE)
	{
//		prnt(1,"a)RELOADING CATALOG INFOMATION !! \n",0,0);
		FILE_RestoreCache();
	}
#endif


// now check that the requested wrf is in the current wdg
/*	FoundWRF=CheckCurrentWDGforWRF(wrfentryname,&CurrentWRF);
	// If the wrf is not in the wdg lets shout and scream and moan about it...
	if (FoundWRF==FALSE)
	{
//#ifdef PSX
//		char t[32];
//		sprintf(t,"bad wrf %s\n",wrfentryname);
//		prnt(1,t,0,0);
//#endif


 		DBPRINTF(("Unable to find %s in WDG\n",wrfentryname));
		return FALSE;//error			UseDataFromWDG=FALSE;
	}*/

	pFileHandle=DISK_OpenFile(CurrentWDGname);	// tries to open the WDG on the HD (pc) or CD  (psx)
	if (pFileHandle==NULL)
	{
		DBPRINTF(("WDG_ProcessWRF unable to open %s\n",CurrentWDGname));
		return FALSE;	
	}

	CatalogLoadedOK=LoadWRFCatalog(CurrentWRF,pFileHandle);
	if (CatalogLoadedOK==FALSE)
	{
		DISK_Close(pFileHandle);
		return FALSE;			// error ... problem loading WRF catalog
	}


	// This is the size of the WRF headers the data starts directly after this
	CurrentWRF_HeaderSize=(sizeof(WRFINFO)*CurrentWRF->filecount);

	// now we need to process each file in the wrf and call routines in data.c for each one

#ifdef PRIMCATALOG
	assert(PrimBufferCatalog);	// make sure the cache is valid
	assert(PrimBufferCatalog->Check1==CHECK1);
	assert(PrimBufferCatalog->Check2==CHECK2);
	CurrentFile=PrimBufferCatalog->primWRFfilesCatalog;
#else
	CurrentFile=WRFfilesCatalog;
#endif
/*
	We don't want any of this disk_ bollox within this loop 

		- we need to support the disk cache if it exists 

		.. this way we can load up the entire WDG if we have the memory for it


	what about being able to retreive a range within a file 


*/
	for (File=0;File<CurrentWRF->filecount;File++)
	{
		UDWORD FileOffset;

		UBYTE *pRetreivedFile;

#ifdef PSX
extern UDWORD MouseIn2;

	if (MouseIn2 == 1)
	{
		zrintf("file =%d",File);
	}
#endif
		CurrentFile = WRFfilesCatalog + File;

		if (CurrentFile->offset == UDWORD_MAX)
		{
			if (!wdgGetFileFromOtherWDG(CurrentWRF->WRFname, CurrentFile->type, CurrentFile->name,
							&CurrentFile, &pRetreivedFile))
			{
				return FALSE;
			}
		}
		else
		{
			// file is in this WDG - load normally
			FileOffset=CurrentWRF->offset + CurrentWRF_HeaderSize + CurrentFile->offset;

			pRetreivedFile=FILE_Retrieve(pFileHandle,FileOffset,CurrentFile->filesize);	// Somehow get from the wdg the required data and return a pointer to it
		}

//		DBPRINTF(("%d) %p\n",File,pRetreivedFile));

//		{
//			char t[32];
//			sprintf(t,"%d) %p\n",File,pRetreivedFile);
//			prnt(1,t,0,0);
//		}


		SetLastFnameExtra(CurrentFile->SoftwareFlag, CurrentFile->TexturePage);

		// John has written this function in frameresource.c
		SetLastHashName(CurrentFile->name);		// set the hashed name up so that the script stuff will work

									
		if (FILE_ProcessFile(CurrentFile,pRetreivedFile)==FALSE) return FALSE;


		resDoResLoadCallback();		// do callback.

	}
	DISK_Close(pFileHandle);
	return TRUE;
}



// These functions should be checking the games system registry options
// so that we can selectively turn on or  off the loading from WRFs & WDGs
BOOL WDG_AllowWRFs(void)
{
	return TRUE;
}


BOOL WDG_AllowWDGs(void)
{
	return TRUE;
}


/** private functions **/
BOOL FILE_ShutdownCache(void)
{
	if (Cache.pBufferStart!=NULL)
	{
		if (Cache.IsCacheDataMalloced==TRUE)
		{
			Cache.IsCacheDataValid=FALSE;
			FREE(Cache.pBufferStart);
			Cache.pBufferStart=NULL;
		}
		Cache.IsCacheDataMalloced=FALSE;
	}
#if defined(WIN32) && defined(PRIMCATALOG)

	if (PrimBufferCatalog)
	{
		FREE(PrimBufferCatalog);
		PrimBufferCatalog=NULL;
	}

#endif

	return TRUE;
}




//static UDWORD CacheInvalidCounter=0;

// Call this when you are zapping the primative buffer mem. so that next time it tries to load from a wdg it reloads the cache.
void FILE_InvalidateCache(void)
{
//	prnt(1,"cache invalidated1!\n",0,0);
//	DBPRINTF(("Cache invalidated !!!"));
//	{
//		UBYTE buf[32];
//		sprintf(buf,"cache invalid\n");
//#ifdef PSX
//		prnt(1,buf,0,0);
//#endif
//	}
	Cache.IsCacheDataValid=FALSE;	// This is the actual data in the cache 
#ifdef PRIMCATALOG
	PrimBufferCatalog=NULL;			// invalidate the wrf/wdg catalogs in the primative buffer
#endif
}

void FILE_SetupCache(UBYTE *CacheStart, UDWORD CacheSize)
{

DBPRINTF(("\n\n\n\n\n\n\n%dv =  cache size\n\n\n\n\n\n",CacheSize));
	Cache.pBufferStart=CacheStart;
	Cache.BufferSize=CacheSize;
	FILE_InvalidateCache();		// setup everything up as invalid
}

BOOL FILE_IsCatalogValid(void)
{
#ifdef PRIMCATALOG
	if (PrimBufferCatalog)
	{
		if (PrimBufferCatalog->Check1==CHECK1 && PrimBufferCatalog->Check2==CHECK2)
		{
//#ifdef PSX
//			prnt(1,"CataValid\n",0,0);
//#endif
			return TRUE;
		}
	}
//#ifdef PSX	
//	prnt(1,"CataInvalid\n",0,0);
//#endif
	return FALSE;
#else
	return TRUE;
#endif
}


// restore the cache to the state it was before it was invalidated
BOOL FILE_RestoreCache(void)
{
	FILE_InitialiseCache(0);
#ifdef PRIMCATALOG
	WDG_SetCurrentWDG(CurrentWDGname);
#endif

	return TRUE;
}

BOOL FILE_InitialiseCache(SDWORD CacheSize)
{
	UBYTE *CacheBuffer;

	if (CacheSize == 0 && Cache.IsCacheDataMalloced)
	{
		Cache.OffsetInWDG=0;
		Cache.IsCacheDataValid=FALSE;	// data not yet loaded

		return TRUE;
	}
	else if (CacheSize==0)
	{
#ifdef PSX
		UDWORD BufferUsed;
#endif

		UBYTE *CacheStart;
		UDWORD CacheSize;

		Cache.IsCacheDataMalloced=FALSE;

#ifdef PSX

	// Calculate where the cache is on the playstation
		BufferUsed=GetPrimBufferAllocatedSize();
		DBPRINTF(("InitialiseCache - %d bytes already allocated in the primative buffer\n",BufferUsed));


		GetPrimBufferMem(&CacheStart, &CacheSize);
		// adjust the values by the amount allocated
		CacheStart+=BufferUsed;
		CacheSize-=BufferUsed;
#endif

// if we are loading the catalog data from the primative buffer add in the size and setup the pointers now
#ifdef	PRIMCATALOG

		PrimBufferCatalog=CacheStart;


		PrimBufferCatalog->Check1=CHECK1;			// sanity check (even in release - only 8 bytes)
		PrimBufferCatalog->Check2=CHECK2;


		CacheStart+=sizeof(PRIMBUFFERCATALOG);
		CacheSize-=sizeof(PRIMBUFFERCATALOG);
		
		assert(CacheSize>0);
						
		DBPRINTF(("Catalog address = %p\n",PrimBufferCatalog));
#endif

	DBPRINTF(("\n\n\n\n\n\n\nold cache = %d\n\n\n\n",Cache.BufferSize));
		DBPRINTF(("adjusted to %p size=%d\n",CacheStart,CacheSize));

		Cache.pBufferStart=CacheStart;
		Cache.BufferSize=CacheSize;
		Cache.OffsetInWDG=0;
		Cache.IsCacheDataValid=FALSE;	// data not yet loaded

		return TRUE;
	}

	blockSuspendUsage();
#ifdef	PRIMCATALOG
	if (PrimBufferCatalog==NULL)
	{
		PrimBufferCatalog=MALLOC(sizeof(PRIMBUFFERCATALOG));

		PrimBufferCatalog->Check1=CHECK1;			// sanity check (even in release - only 8 bytes)
		PrimBufferCatalog->Check2=CHECK2;
			
	}
#endif
	CacheBuffer=MALLOC(CacheSize);
	if (CacheBuffer==NULL)
	{
		DBPRINTF(("No memory for the file cache ... !\n"));
		blockUnsuspendUsage();
		return FALSE;
	}

	Cache.pBufferStart=CacheBuffer;
	Cache.BufferSize=CacheSize;
	Cache.OffsetInWDG=0;
	Cache.IsCacheDataValid=FALSE;	// data not yet loaded
	Cache.IsCacheDataMalloced=TRUE;
	
	blockUnsuspendUsage();

	return TRUE;
}


static SDWORD DataPendingSize=0;
static UDWORD DataPendingOffset;

SDWORD FILE_AmountPending(void)
{
	return DataPendingSize;
}

// Get a pointer to a file in the cache ... if its not in the cache then fill up the cache
UBYTE *FILE_Retrieve(FILE *pFileHandle, UDWORD offsetInWDG, UDWORD filesize)
{
	UDWORD BytesRead;
	// Check to see if the file we want is in the cache 

//DBPRINTF(("\n\n\n\n\n\n\nfile_ret  - cache size = %d\n\n\n\n\n\n\n\n",Cache.BufferSize));

	if (Cache.pBufferStart!=NULL)
	{
		if (Cache.IsCacheDataValid==TRUE)
		{
			if (Cache.OffsetInWDG<offsetInWDG)	// is the requested data after the start of the cache area ?
			{
				if (offsetInWDG+filesize <= Cache.OffsetInWDG+Cache.BufferSize)	// is the end of the data before the end of the cache
				{
					// if it is then just return the pointer in the cache
					return (Cache.pBufferStart + (offsetInWDG-Cache.OffsetInWDG));	// return cache pointer
				}
			}
		}
		

		// if its not then fill up the cache with data from the start of the requested WDG


//		DBPRINTF(("filesize=%d cachesize=%d\n",filesize,Cache.BufferSize));



//		assert(filesize<=Cache.BufferSize);	// make sure that the file will fit the the cache !!!!


		BytesRead=DISK_ReadPos(offsetInWDG,Cache.pBufferStart,Cache.BufferSize,pFileHandle);


		DataPendingSize=0;
		if (filesize > Cache.BufferSize)
		{
			// we can't load all of the file into the cache at once
			// see we load what we can and the rest is loaded via a FILE_RetreivePending();

			DataPendingSize=filesize-Cache.BufferSize;		// amount pending
			DataPendingOffset=offsetInWDG+Cache.BufferSize;	// start of pending data
		}


		Cache.OffsetInWDG=offsetInWDG;	// Set the cache offset position
		
		Cache.IsCacheDataValid=TRUE;	// Data is now valid !

		// and return a pointer to the start of the buffer
		return (Cache.pBufferStart);

	}

	return NULL;		// error condition ... cache not set up
}


// load a single file into the cache buffer.  NB - this invalidates the cache
UBYTE *FILE_RetrieveSingle(FILE *pFileHandle, UDWORD offsetInWDG, UDWORD filesize)
{
	UDWORD BytesRead;

	BytesRead=DISK_ReadPos(offsetInWDG,Cache.pBufferStart,filesize,pFileHandle);

	if (BytesRead != filesize)
	{
		return NULL;
	}

	Cache.IsCacheDataValid=FALSE;	// Data is not valid for subsequent reads

	// and return a pointer to the start of the buffer
	return (Cache.pBufferStart);
}


// Get any 
UBYTE *FILE_RetreivePending( UDWORD *SizeLoaded)
{
	UDWORD BytesRead;
	FILE *pFileHandle;

	*SizeLoaded=0;
	if (DataPendingSize<=0) return NULL;		// nothing to do

	if ((UDWORD)DataPendingSize>Cache.BufferSize)
	{
		*SizeLoaded=Cache.BufferSize;
	}
	else
	{
		*SizeLoaded=DataPendingSize;
	}

	// Now read in the catalog for the MISCDATA section
	// If it's already loaded then we shouldn't really re-load it  ... but what the fuck it's saturday
	pFileHandle=DISK_OpenFile(CurrentWDGname);	// tries to open the WDG on the HD (pc) or CD  (psx)
	if (pFileHandle==NULL)
	{
		DBPRINTF(("WDG_ProcessWRF unable to open %s\n",CurrentWDGname));
		return FALSE;	
	}

	BytesRead=DISK_ReadPos(DataPendingOffset,Cache.pBufferStart,Cache.BufferSize,pFileHandle);

	DISK_Close(pFileHandle);

	DataPendingSize-=BytesRead;
	Cache.OffsetInWDG=DataPendingOffset;	// Set the cache offset position
	DataPendingOffset+=BytesRead;

	DBPRINTF(("Bytes read = %d remaining=%d\n",BytesRead,DataPendingSize));
	
	Cache.IsCacheDataValid=TRUE;	// Data is now valid !

	// and return a pointer to the start of the buffer
	return (Cache.pBufferStart);


	

}

/*

	Get specific infomation from a text string

  - This routine scans through a text string and retreives very specific 
  infomation from it ...

 It searchs for the words "soft" or "hard" which implies that the resource that the string
 is releated to is specifically for software or hardware

 Also it scans through for a texture page number which is in the format page-XX

*/



void ScanString(char *InString, UBYTE * pSoftwareFlag,SBYTE * pTexturePage)
{

	UBYTE SoftwareFlag=0;
	SBYTE TexturePage=-1;	// no page


	char String[64];

	strcpy(String,InString);

	// Is the word "soft" in the string
	// then its a software texture page
	if (strstr(String,"soft") != NULL)//and this is a software textpage
	{
		SoftwareFlag|=TPAGE_SOFTWARE;
	}

	if (strstr(String,"hard") != NULL)//and this is a software textpage
	{
		SoftwareFlag|=TPAGE_HARDWARE;
	}


	if (strncmp(String, "page-", 5) == 0)
	{
		UDWORD i;
		UDWORD Value=0;

		for(i=5; i<(SDWORD)strlen(String); i++)
		{
			if (isdigit(String[i]))
			{
				Value*=10;
				Value+= String[i]-'0';
			}
			else
			{

				break;
			}
		}
		TexturePage=Value;
	}

	*pSoftwareFlag=SoftwareFlag;
	*pTexturePage=TexturePage;

	return;

}


// creates a filename that will work with all the getlastresourcename() calls
void SetLastFnameExtra(UBYTE SoftwareFlag, SBYTE TexturePage)
{
	char CreatedFilename[256];

	// maps to TPAGE_SOFT & TPAGE_HARD
	char TypeString[]= {"    "};


	if (SoftwareFlag&TPAGE_SOFTWARE) strcpy(TypeString,"soft");

	if (SoftwareFlag&TPAGE_HARDWARE) strcpy(TypeString,"hard");
	

	if (TexturePage>=0)
	{
		sprintf(CreatedFilename,"page-%d %s\n",TexturePage,TypeString);
		SetLastResourceFilename(CreatedFilename);
	}
	else
	{
		SetLastResourceFilename("Unknown");
	}
}




/*
		loadFileFromWDG

	Trys to load a file directly from the current WDG
	will return TRUE if load was sucessful or FALSE if it fails

	if AllocateMem is TRUE then the memory is allocated from the current block, and returned through ppFileData & pFileSize
	if AllocateMem is FALSE then the file is loaded into *ppFileData

	// currently we always either allocate memory for the file or pass into this routine a block of memory
	// we could if we wanted to just return a pointer in the cache that contains the file...
	//   ... this is not yet supported

	The filename can contain directories (e.g.   "wrf\cam1\sub1-2\game.map" is valid)


	We will look in the current WDG for a MISCDATA section, this is not releated to any particual WRF file (unlike all the other sections)
	This will hold all the extra data not connected to a .wrf file

MemAllocationMode can be ...

	WDG_ALLOCATEMEM		// was TRUE - indicates that we allocate the memory for the file and return the size in pFileSize
	WDG_USESUPPLIED		// was FALSE - indicates that we dont allocate memory and copy the file into the buffer supplie by ppFileData
	WDG_RETURNCACHE		// new  - just returns as much of the file as we can in the cache

*/
//BOOL loadFileFromWDG(STRING *pFileName, UBYTE **ppFileData, UDWORD *pFileSize, BOOL AllocateMem)
BOOL loadFileFromWDGCache(WDG_FINDFILE *psFindFile, UBYTE **ppFileData, UDWORD *pFileSize, UBYTE MemAllocationMode)
{
	WDGINFO *CurrentWRF;
//	BOOL FoundWRF;
//	BOOL CatalogLoadedOK;
//	BOOL FoundFileInWRF;
	WRFINFO	*CurrentFile;  //=WRFfilesCatalog;
	FILE *pFileHandle;
//	UDWORD RequiredFileHash,File;
	UDWORD FileOffset;
	UBYTE *pRetreivedFile;
	UDWORD CurrentWRF_HeaderSize;
//	char name[32];
//	UDWORD pos;

/*	THIS IS ALL DONE BY THE MULTIWDG STUFF NOW - JOHN

	if (CurrentWDGname[0]==0) return FALSE;			// no wdg open

	DBPRINTF(("loadfilefromwdg = %s %d\n",pFileName,MemAllocationMode));

//	prnt(1,pFileName,0,0);

	// if the cache has become invalid then reload it
	//
	// The cache also holds the wrf/wdg catalog infomation ... this needs to be reloaded before we can continue to load a wrf
	if ( FILE_IsCatalogValid()==FALSE)
	{
////#ifdef PSX
//		prnt(1,"RELOADING CATALOG INFOMATION !! \n",0,0);
//#endif
		FILE_RestoreCache();
	}


	FoundWRF=CheckCurrentWDGforWRF("MISCDATA",&CurrentWRF);

	if (FoundWRF==FALSE)
	{
	
		// There is no MISCDATA section in the current WDG
		return FALSE;
	}


	// Now read in the catalog for the MISCDATA section
	// If it's already loaded then we shouldn't really re-load it  ... but what the fuck it's saturday
	pFileHandle=DISK_OpenFile(CurrentWDGname);	// tries to open the WDG on the HD (pc) or CD  (psx)
	if (pFileHandle==NULL)
	{
		DBPRINTF(("WDG_ProcessWRF unable to open %s\n",CurrentWDGname));
		return FALSE;	
	}

	CatalogLoadedOK=LoadWRFCatalog(CurrentWRF,pFileHandle);
	if (CatalogLoadedOK==FALSE)
	{
		DISK_Close(pFileHandle);
		return FALSE;			// error ... problem loading WRF catalog
	}


	
	// Generate the Hash value for the required file
	RequiredFileHash=HashStringIgnoreCase(pFileName);
	
	// Now we'll just go through all the files in the MISCDATA section looking for the required file
	FoundFileInWRF=FALSE;
#ifdef PRIMCATALOG
	assert(PrimBufferCatalog);	// make sure the cache is valid
	assert(PrimBufferCatalog->Check1==CHECK1);
	assert(PrimBufferCatalog->Check2==CHECK2);
	CurrentFile=PrimBufferCatalog->primWRFfilesCatalog;	// Start at the begining of the catalog
#else
	CurrentFile=WRFfilesCatalog;	// Start at the begining of the catalog
#endif

	for (File=0;File<CurrentWRF->filecount;File++)
	{
		if (CurrentFile->name==RequiredFileHash)
		{
			FoundFileInWRF=TRUE;
			break;
		}

		CurrentFile++;		
	}

	if (FoundFileInWRF==FALSE)
	{
		// The file we want is not stored in the MISCDATA section of the WDG
		DISK_Close(pFileHandle);
		return FALSE;
	}
	THIS IS ALL DONE BY THE MULTIWDG STUFF NOW - JOHN */

	// get the file offset from the find file structure
	CurrentWRF = psFindFile->psCurrCache->asWRFCatalog + psFindFile->currWRFIndex;
	CurrentFile = psFindFile->psCurrCache->apsWRFFileCatalog[ psFindFile->currWRFIndex ] + psFindFile->currFileIndex ;

	// This is the size of the WRF headers the data starts directly after this
	CurrentWRF_HeaderSize=(sizeof(WRFINFO)*CurrentWRF->filecount);

	FileOffset=CurrentWRF->offset + CurrentWRF_HeaderSize + CurrentFile->offset;

	FILE_InvalidateCache();

	pFileHandle=DISK_OpenFile(psFindFile->psCurrCache->aFileName);	// tries to open the WDG on the HD (pc) or CD  (psx)
	if (pFileHandle==NULL)
	{
		DBPRINTF(("WDG_ProcessWRF unable to open %s\n",CurrentWDGname));
		return FALSE;	
	}

	pRetreivedFile=FILE_Retrieve(pFileHandle,FileOffset,CurrentFile->filesize);	// Somehow get from the wdg the required data and return a pointer to it
	
	if (pRetreivedFile==NULL)
	{
		// Unable to load the file in (perhaps to big for the cache?)
		DISK_Close(pFileHandle);
		return FALSE;

	}														   


	if (MemAllocationMode==WDG_RETURNCACHE)
	{
		// This will return a pointer in the cache ! ... not currently implemented correctly
		//   ... it will only work if the whole file fits in the cache (?)
		*ppFileData=pRetreivedFile;

		if (FILE_AmountPending() <= 0)
		{
			*pFileSize=CurrentFile->filesize; 	// we got all the file
		}
		else
		{	
			*pFileSize=Cache.BufferSize;	// we got all we could but there is still some left
			//prnt(1,"DATA STILL PENDING !! \n",0,0); ffs TC
			DBPRINTF(("size still pending = %d\n",FILE_AmountPending()));

		}
		DISK_Close(pFileHandle);
		return TRUE;

	}
	// The file was loaded successfully ... now we must find out if we need to allocate memory for it
	else if (MemAllocationMode==WDG_ALLOCATEMEM)
	{
		// we must allocate memory for the file
		*ppFileData=MALLOC(CurrentFile->filesize);
		if (*ppFileData==NULL)
		{
			// no mem for file
			DBPRINTF(("alloc failed - no mem for file size %d\n",CurrentFile->filesize));
			DISK_Close(pFileHandle);
			return FALSE;
		}
	}

	// otherwise WDG_USESUPPLIED which will already have setup the pointers
	assert(*ppFileData!=NULL);

	memcpy(*ppFileData,pRetreivedFile,CurrentFile->filesize);
	*pFileSize=CurrentFile->filesize;

	DISK_Close(pFileHandle);
	return TRUE;

}

BOOL loadFileFromWDG(STRING *pFileName, UBYTE **ppFileData, UDWORD *pFileSize, UBYTE AllocateMem)
{
	WDG_FINDFILE	sFindFile;

	// find the file
	if (!wdgFindFirstFile(HashStringIgnoreCase("MISCDATA"), HashString("MISCDATA"), HashStringIgnoreCase(pFileName),
						&sFindFile))
	{
		return FALSE;
	}

	return loadFileFromWDGCache(&sFindFile, ppFileData, pFileSize, AllocateMem);
}