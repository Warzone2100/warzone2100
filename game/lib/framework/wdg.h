/*
	WDG releated goodies
*/

#ifndef _wdg_h
#define _wdg_h

// list of all wrfs in the wdg
typedef struct
{
	UDWORD WRFname;	// Hashed wrf name
	UDWORD offset;	// offset to start of wrf info
	UDWORD filecount; // Number of files in wrf
} WDGINFO;

#define TPAGE_SOFTWARE (1)
#define TPAGE_HARDWARE (2)

// List of files in a wrf
typedef struct
{
	UDWORD type;		// Hashed type 
	UDWORD name;		// hashed file name
	UDWORD offset;		// offset from start of this wrf data
	UDWORD filesize;	// size of file in bytes
	// Extra info stored depending on type
	// - TEXPAGE info
	UBYTE SoftwareFlag;	// is texpage for software/hardware specifically TPAGE_xxx
	SBYTE TexturePage;	// what texture page slot is it for (-1 for none specified)
} WRFINFO;


#define MAXFILESINWRF (900)
#define MAXWRFINWDG (128)



/* Structures for .WDG data files */

#define WDGIDcount 4
#define WDG_NAMEMAX		16

#define WDG4_HEADER \
	char WDGid[WDGIDcount];	/* W,D,G,4 */ \
	int	NumWRFFiles			/* number of wrfs in this wdg */

#define WDG5_HEADER \
	WDG4_HEADER; \
	STRING		aName[WDG_NAMEMAX];	/* wdg name */ \
	UDWORD		sequence;	/* WDG sequence number */ \
	UDWORD		numDep		/* Number of WDG dependancies */

typedef struct
{
	WDG4_HEADER;
} WDG_HEADER;

typedef struct
{
	WDG5_HEADER;
} WDG_HEADER_V5;

// WDG dependancies
typedef struct
{
	STRING		aName[WDG_NAMEMAX];
	UDWORD		sequence;
} WDG_DEP;


typedef struct
{
	UBYTE	*pBufferStart;		// start of the memory buffer for the cache
	UDWORD	BufferSize;			// size of the cache
	UDWORD	OffsetInWDG;		// the start of the cache contains data from this offset in the WDG
	BOOL	IsCacheDataValid;
	BOOL 	IsCacheDataMalloced;	// has the data been malloced or is it using some non-allocated buffer somewhere (display buffer)
} CACHE;

BOOL WDG_SetCurrentWDG(char *filename);
BOOL loadFileFromWDG(STRING *pFileName, UBYTE **ppFileData, UDWORD *pFileSize, UBYTE MemAllocationMode);
struct _wdg_findfile;
BOOL loadFileFromWDGCache(struct _wdg_findfile *psFindFile, UBYTE **ppFileData, UDWORD *pFileSize, UBYTE MemAllocationMode);
BOOL LoadWRFCatalog(WDGINFO *CurrentWRF, FILE *pFileHandle);
BOOL WDG_ProcessWRF(char *WRFname,BOOL UseDataFromWDG );
BOOL FILE_InitialiseCache(SDWORD CacheSize);
BOOL FILE_ShutdownCache(void);
void FILE_InvalidateCache(void);
BOOL WDG_AllowWRFs(void);
BOOL WDG_AllowWDGs(void);
UBYTE *FILE_Retrieve(FILE *pFileHandle, UDWORD offsetInWDG, UDWORD filesize);
UBYTE *FILE_RetrieveSingle(FILE *pFileHandle, UDWORD offsetInWDG, UDWORD filesize);


// MemAllocationMode can be ....
#define	WDG_ALLOCATEMEM	(0)	// was TRUE - indicates that we allocate the memory for the file and return the size in pFileSize
#define	WDG_USESUPPLIED	(1)	// was FALSE - indicates that we dont allocate memory and copy the file into the buffer supplie by ppFileData
#define	WDG_RETURNCACHE	(2)	// new  - just returns as much of the file as we can in the cache

// set the current WDG and it's catalogs
void WDG_SetCurrentWDGCatalog(char *filename, UDWORD numWRF, WDGINFO *psWRFCatalog);

// set the current WRF file catalog
void WDG_SetCurrentWRFFileCatalog(UDWORD catOffset, WRFINFO *psFileCatalog);

// get the current WDG and it's catalogs
void WDG_GetCurrentWDGCatalog(char **ppFileName, UDWORD *pNumWRF, WDGINFO **ppsWRFCatalog);

// get the current WRF file catalog
void WDG_GetCurrentWFRFileCatalog(UDWORD *pCatOffset, WRFINFO **ppsFileCatalog);

#endif