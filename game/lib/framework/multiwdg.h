/*
 * multiWDG.h
 *
 * Handle multiple wdg files
 *
 */

#ifndef _multiwdg_h
#define _multiwdg_h

#include "wdg.h"

// maximum number of characters in the wdg name
#define WDG_NAMEMAX		16
#define WDG_FILEMAX		256
#define WDG_MAXDEP		20

// store the catalog info for a WDG
typedef struct _wdgcache
{
	STRING		aFileName[WDG_FILEMAX];
	STRING		aName[WDG_NAMEMAX];
	UDWORD		sequence;

	UDWORD		numWRF;
	WDGINFO		*asWRFCatalog;
	WRFINFO		**apsWRFFileCatalog;

	// store the dependancies
	UDWORD		numDep;
	WDG_DEP		asDep[WDG_MAXDEP];

	struct _wdgcache *psNext;
	struct _wdgcache *psPrev;
} WDGCACHE;


// function prototypes
BOOL wdgMultiInit(void);
void wdgMultiShutdown(void);
BOOL wdgLoadAllWDGCatalogs(void);
void wdgDisableAddonWDG(void);
void wdgEnableAddonWDG(void);

// structures for the find WRF / file functions

#define WDG_FINDWRF_BASE \
	/* the wrf that is being searched for */ \
	UDWORD		WRFNameHash; \
	/* the current WDG that the WRF was found in */ \
	WDGCACHE	*psCurrCache; \
	UDWORD		currWRFIndex

typedef struct _wdg_findwrf
{
	WDG_FINDWRF_BASE;
} WDG_FINDWRF;


typedef struct _wdg_findfile
{
	WDG_FINDWRF_BASE;

	// the file that is being searched for
	UDWORD		FileType;
	UDWORD		FileNameHash;

	// the index of a found file
	UDWORD		currFileIndex;
} WDG_FINDFILE;


// Find the first instance of a WRF in the WDG catalogs
BOOL wdgFindFirstWRF(UDWORD WRFName, WDG_FINDWRF *psFindData);
// Find the next instance of a WRF in WDG catalogs
BOOL wdgFindNextWRF(WDG_FINDWRF *psFindData);

// Find the first instance of a file in a WRF
BOOL wdgFindFirstFile(UDWORD WRFName, UDWORD type, UDWORD FileName, WDG_FINDFILE *psFindData);
// Find the next instance of a file in a WRF
BOOL wdgFindNextFile(WDG_FINDFILE *psFindData);

// set the current WDG based on the found WRF
void wdgSetCurrentWDGFromFind(WDG_FINDWRF *psFindData);

// load a file that is not stored in the current WDG
BOOL wdgGetFileFromOtherWDG(UDWORD WRFname, UDWORD type, UDWORD FileName,
							WRFINFO **ppsNewFile, UBYTE **ppFileBuf);

// Rev versions of these functions all search from the end to the begining

// Find the first instance of a WRF in the WDG catalogs
BOOL wdgFindFirstWRFRev(UDWORD WRFName, WDG_FINDWRF *psFindData);
// Find the next instance of a WRF in WDG catalogs
BOOL wdgFindNextWRFRev(WDG_FINDWRF *psFindData);
// Find the first instance of a file in a WRF
BOOL wdgFindFirstFileRev(UDWORD WRFName, UDWORD type, UDWORD FileName, WDG_FINDFILE *psFindData);
// Find the next instance of a file in a WRF
BOOL wdgFindNextFileRev(WDG_FINDFILE *psFindData);

#endif
