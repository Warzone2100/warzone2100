/*
 * multiWDG.c
 *
 * Handle multiple wdg files
 *
 */

#include "frame.h"
#include "wdg.h"
#include "multiwdg.h"

// the list of file catalogs for all the current WDGs
WDGCACHE	*psWDGCache;
WDGCACHE	*psWDGCacheRev;

// store the additional WDGs when the system is limited to warzone.WDG
WDGCACHE	*psWDGCacheStore;
WDGCACHE	*psWDGCacheStoreRev;

// static data areas used when loading the WDG catalogs at start up
static WDGINFO WRFCatalog[MAXWRFINWDG];		// static area for the catalog of wrf files in this wdg
static WRFINFO WRFfilesCatalog[MAXFILESINWRF];  // static area for list of files in the current wrf

BOOL wdgLoadCompleteCatalog(char *pWDGName);

#define DISK_ReadPos(filepos,buffer,bytescount,handle) \
	(fseek(handle,filepos,SEEK_SET), \
	 fread(buffer,1,bytescount,handle))


BOOL wdgMultiInit(void)
{
	ASSERT((psWDGCache == NULL,
		"wdgMultiInit: the WDG cache has not been reset"));
	psWDGCache = NULL;
	psWDGCacheRev = NULL;
	psWDGCacheStore = NULL;
	psWDGCacheStoreRev = NULL;

	// set the catalogs to the static arrays to load them
	WDG_SetCurrentWDGCatalog("", 0, WRFCatalog);
	WDG_SetCurrentWRFFileCatalog(0, WRFfilesCatalog);

	return TRUE;
}


void wdgMultiShutdown(void)
{
	WDGCACHE	*psNext;
	UDWORD		i;

	while (psWDGCache != NULL)
	{
		psNext = psWDGCache->psNext;

		// free the file catalogs
		for(i=0; i<psWDGCache->numWRF; i++)
		{
			if(psWDGCache->apsWRFFileCatalog[i])
			{
				FREE(psWDGCache->apsWRFFileCatalog[i]);
			}
		}
		FREE(psWDGCache->apsWRFFileCatalog);

		// free the WRF catalog
		FREE(psWDGCache->asWRFCatalog);
		FREE(psWDGCache);

		psWDGCache = psNext;
	}

	psWDGCacheRev = NULL;
}


// allow only the original warzone.wdg to be used
void wdgDisableAddonWDG(void)
{
	if (psWDGCache == NULL)
	{
		// no WDG loaded
		return;
	}

	if (strcmp(psWDGCache->aName, "warzone") == 0)
	{
		// only the main wdg loaded
		return;
	}

	ASSERT(( strcmp(psWDGCacheRev->aName, "warzone") == 0,
		"wdgDisableAddonWDG: cannot find warzone.wdg"));

	psWDGCacheStore = psWDGCache;
	psWDGCacheStoreRev = psWDGCacheRev->psPrev;

	psWDGCache = psWDGCacheRev;
	psWDGCache->psPrev = NULL;
}

// allow all the wdg to be used
void wdgEnableAddonWDG(void)
{
	if (psWDGCacheStore == NULL)
	{
		// nothing in the store
		return;
	}

	psWDGCache = psWDGCacheStore;
	psWDGCacheRev->psPrev = psWDGCacheStoreRev;

	psWDGCacheStore = NULL;
	psWDGCacheStoreRev = NULL;
}


// check the dependancies of all the WDG
BOOL wdgCheckDependancies(void)
{
	WDGCACHE	*psCheck, *psSearch;
	UDWORD		dep, i;
	STRING		aText[500];

	for(psCheck = psWDGCache; psCheck!=NULL; psCheck=psCheck->psNext)
	{
		for(dep = 0; dep < psCheck->numDep; dep++)
		{
			for(psSearch=psWDGCache; psSearch!=NULL; psSearch=psSearch->psNext)
			{
				if ((psSearch->sequence == psCheck->asDep[dep].sequence) &&
					(strcmp(psSearch->aName, psCheck->asDep[dep].aName) == 0))
				{
					break;
				}
			}

			if (psSearch == NULL)
			{
				sprintf(aText, "%s requires the following .wdg to load:\n\n", psCheck->aFileName);
				for(i=0; i<psCheck->numDep; i++)
				{
					sprintf(aText + strlen(aText), "    %s.wdg\n", psCheck->asDep[i].aName);
				}
				DBERROR((aText));
				return FALSE;
			}
		}
	}

	return TRUE;
}


// load all the WDG catalogs
BOOL wdgLoadAllWDGCatalogs(void)
{
	WIN32_FIND_DATA		sFindData;
	HANDLE				hFindHandle;

	// set the catalogs to the static arrays to load them
	WDG_SetCurrentWDGCatalog("", 0, WRFCatalog);
	WDG_SetCurrentWRFFileCatalog(0, WRFfilesCatalog);

	// now load the catalog for every WDG file in the current directory
	memset(&sFindData, 0, sizeof(WIN32_FIND_DATA));
	hFindHandle = FindFirstFile("*.wdg", &sFindData);
	while (hFindHandle != INVALID_HANDLE_VALUE)
	{
		if (!wdgLoadCompleteCatalog(sFindData.cFileName))
		{
			return FALSE;
		}

		if (!FindNextFile(hFindHandle, &sFindData))
		{
			hFindHandle = INVALID_HANDLE_VALUE;
		}
	}

	// now check the dependancies
	if (!wdgCheckDependancies())
	{
		return FALSE;
	}

	return TRUE;
}

// try to load the additional header info in addon WDGs
BOOL wdgLoadAdditionalHeader(FILE *pFileHandle, WDGCACHE *psCache)
{
	WDG_HEADER_V5	Header;
	UDWORD			BytesRead;

	// okay, the file exsists (because we succsesfully opened it), now read in the WDG header
	BytesRead=DISK_ReadPos(0,(UBYTE *)&Header,sizeof(WDG_HEADER_V5),pFileHandle);
	if (BytesRead!=sizeof(WDG_HEADER_V5))
	{
		DBERROR(("wdgLoadAdditionalHeader unable to read from %s\n",psCache->aFileName));
		return FALSE;	
	}
	
	// right, we read in the header now check the values to make sure its okay
	if (Header.WDGid[0]!='W' ||
		Header.WDGid[1]!='D' ||
		Header.WDGid[2]!='G')
	{
		DBERROR(("wdgLoadAdditionalHeader bad type of wdg file - %s\n",psCache->aFileName));
		return FALSE;	
	}

	if (Header.WDGid[3] == '4')
	{
		// got the original WDG
		psCache->sequence = 0;
		strcpy(psCache->aName, "warzone");
		psCache->numDep = 0;
		return TRUE;
	}

	// version 5 header
	psCache->sequence = Header.sequence;
	strcpy(psCache->aName, Header.aName);
	psCache->numDep = Header.numDep;
	BytesRead=DISK_ReadPos(sizeof(WDG_HEADER_V5),(UBYTE *)psCache->asDep,sizeof(WDG_DEP)*Header.numDep,pFileHandle);
	if (BytesRead!=sizeof(WDG_DEP)*Header.numDep)
	{
		DBERROR(("wdgLoadAdditionalHeader unable to read from %s\n",psCache->aFileName));
		return FALSE;	
	}

	return TRUE;
}


// load the catalogs in a wdg
// NOTE: assumes the wdg.c catalogs have been set to the static arrays
BOOL wdgLoadCompleteCatalog(char *pWDGName)
{
	WDGCACHE	*psNew, *psCurr, *psPrev;
	UDWORD		numWRF, i, offset;
	WDGINFO		*psCat;
	WRFINFO		*psFileCat;
	FILE		*pFileHandle;

	// allocate the cache structure
	psNew = MALLOC(sizeof(WDGCACHE));
	if (psNew == NULL)
	{
		DBERROR(("wdgLoadCompleteCatalog: out of memory"));
		return FALSE;
	}
	strcpy(psNew->aFileName, pWDGName);

	// load the WDGINFO
	if (!WDG_SetCurrentWDG(pWDGName))
	{
		DBERROR(("wdgLoadCompleteCatalog: could not set %s as Current WDG", pWDGName));
		return FALSE;
	}

	// get the WRF catalog
	WDG_GetCurrentWDGCatalog(&pWDGName, &numWRF, &psCat);
	psNew->numWRF = numWRF;
	psNew->asWRFCatalog = MALLOC( sizeof(WDGINFO) * numWRF);
	if (psNew->asWRFCatalog == NULL)
	{
		DBERROR(("wdgLoadCompleteCatalog: out of memory"));
		return FALSE;
	}
	memcpy(psNew->asWRFCatalog, psCat, sizeof(WDGINFO) * numWRF);
	psNew->apsWRFFileCatalog = MALLOC( sizeof(WRFINFO *) * numWRF );
	if (psNew->apsWRFFileCatalog == NULL)
	{
		DBERROR(("wdgLoadCompleteCatalog: out of memory"));
		return FALSE;
	}
	memset(psNew->apsWRFFileCatalog, 0, sizeof(WRFINFO *) * numWRF);

	// get the extra WDG info
	pFileHandle = fopen(pWDGName, "rb");
	if (pFileHandle == NULL)
	{
		DBERROR(("wdgLoadCompleteCatalog: could not open %s", pWDGName));
		return FALSE;
	}
	if (!wdgLoadAdditionalHeader(pFileHandle, psNew))
	{
		fclose(pFileHandle);
		return FALSE;
	}

	// now load the file catalogs
	for(i=0; i<numWRF; i++)
	{
		if (psCat[i].filecount == 0)
		{
			continue;
		}

		if (!LoadWRFCatalog(psCat + i, pFileHandle))
		{
			DBERROR(("wdgLoadCompleteCatalog: could not read file catalog for WRF number %d", i));
			fclose(pFileHandle);
			return FALSE;
		}
		psNew->apsWRFFileCatalog[i] = MALLOC( sizeof(WRFINFO) * psCat[i].filecount );
		if (psNew->apsWRFFileCatalog[i] == NULL)
		{
			DBERROR(("wdgLoadCompleteCatalog: out of memory"));
			fclose(pFileHandle);
			return FALSE;
		}

		WDG_GetCurrentWFRFileCatalog(&offset, &psFileCat);
		memcpy(psNew->apsWRFFileCatalog[i], psFileCat, sizeof(WRFINFO) * psCat[i].filecount);
	}

	if (fclose(pFileHandle) != 0)
	{
		DBERROR(("wdgLoadCompleteCatalog: failed to close WDG %s", pWDGName));
		fclose(pFileHandle);
		return FALSE;
	}

	// add the cache structure into the list
	psPrev = NULL;
	for(psCurr=psWDGCache; psCurr != NULL; psCurr=psCurr->psNext)
	{
		if (psCurr->sequence < psNew->sequence)
		{
			break;
		}

		psPrev = psCurr;
	}
	if (psPrev == NULL)
	{
		psNew->psNext = psWDGCache;
		psNew->psPrev = NULL;
		psWDGCache = psNew;
	}
	else
	{
		psNew->psNext = psPrev->psNext;
		psNew->psPrev = psPrev;
		psPrev->psNext = psNew;
	}

	if (psNew->psNext == NULL)
	{
		psWDGCacheRev = psNew;
	}
	else
	{
		psNew->psNext->psPrev = psNew;
	}

	return TRUE;
}


// Find the first instance of a WRF in the WDG catalogs
BOOL wdgFindFirstWRF(UDWORD WRFName, WDG_FINDWRF *psFindData)
{
	UDWORD	i;

	// initialise the find data
	memset(psFindData, 0, sizeof(WDG_FINDWRF));
	psFindData->WRFNameHash = WRFName;
	psFindData->psCurrCache = psWDGCache;

	if (psWDGCache == NULL)
	{
		return FALSE;
	}

	// see if the WRF is in the first WDG
	for(i=0; i<psWDGCache->numWRF; i++)
	{
		if (psWDGCache->asWRFCatalog[i].WRFname == WRFName)
		{
			psFindData->currWRFIndex = i;
			return TRUE;
		}
	}

	// search the remaining WDGs
	if (wdgFindNextWRF(psFindData))
	{
		return TRUE;
	}

	return FALSE;
}


// Find the next instance of a WRF in WDG catalogs
BOOL wdgFindNextWRF(WDG_FINDWRF *psFindData)
{
	WDGCACHE	*psCurrCache;
	UDWORD		i;

	if (psFindData->psCurrCache == NULL)
	{
		return FALSE;
	}

	// search each WDG cache
	psCurrCache=psFindData->psCurrCache->psNext;
	for(; psCurrCache != NULL; psCurrCache = psCurrCache->psNext)
	{
		for(i=0; i<psCurrCache->numWRF; i++)
		{
			if (psCurrCache->asWRFCatalog[i].WRFname == psFindData->WRFNameHash)
			{
				goto found;
			}
		}
	}

	// jump here if a WRF has been found
found:
	if (psCurrCache != NULL)
	{
		psFindData->psCurrCache = psCurrCache;
		psFindData->currWRFIndex = i;
		return TRUE;
	}

	psFindData->psCurrCache = NULL;
	psFindData->currWRFIndex = 0;

	return FALSE;
}


// Find the first instance of a file in a WRF
BOOL wdgFindFirstFile(UDWORD WRFName, UDWORD type, UDWORD FileName, WDG_FINDFILE *psFindData)
{
	WDGINFO	*psWRFData;
	WRFINFO	*asFileData;
	UDWORD	i;

	memset(psFindData, 0, sizeof(WDG_FINDFILE));
	psFindData->WRFNameHash = WRFName;
	psFindData->FileType = type;
	psFindData->FileNameHash = FileName;
	psFindData->psCurrCache = psWDGCache;

	if (psWDGCache == NULL)
	{
		return FALSE;
	}

	// find the first WRF
	if (!wdgFindFirstWRF(WRFName, (WDG_FINDWRF *)psFindData))
	{
		return FALSE;
	}

	// see if there is an instance of the file in this WRF
	psWRFData = psFindData->psCurrCache->asWRFCatalog + psFindData->currWRFIndex ;
	asFileData = psFindData->psCurrCache->apsWRFFileCatalog[ psFindData->currWRFIndex ];
	for(i=0; i<psWRFData->filecount; i++)
	{
		if ((asFileData[i].name == psFindData->FileNameHash) &&
			(asFileData[i].type == psFindData->FileType) &&
			(asFileData[i].offset != UDWORD_MAX))
		{
			psFindData->currFileIndex = i;
			return TRUE;
		}
	}

	// not in this WRF, try other ones
	if (wdgFindNextFile(psFindData))
	{
		return TRUE;
	}

	return FALSE;
}

// Find the next instance of a file in a WRF
BOOL wdgFindNextFile(WDG_FINDFILE *psFindData)
{
	WDGINFO	*psWRFData;
	WRFINFO	*asFileData;
	UDWORD	i;

	while (psFindData->psCurrCache != NULL)
	{
		// find the next WRF
		if (!wdgFindNextWRF((WDG_FINDWRF *)psFindData))
		{
			goto failed;
		}

		// see if there is an instance of the file in this WRF
		psWRFData = psFindData->psCurrCache->asWRFCatalog + psFindData->currWRFIndex ;
		asFileData = psFindData->psCurrCache->apsWRFFileCatalog[ psFindData->currWRFIndex ];
		for(i=0; i<psWRFData->filecount; i++)
		{
			if ((asFileData[i].name == psFindData->FileNameHash) &&
				(asFileData[i].type == psFindData->FileType) &&
				(asFileData[i].offset != UDWORD_MAX))
			{
				psFindData->currFileIndex = i;
				return TRUE;
			}
		}
	}

failed:
	psFindData->psCurrCache = NULL;
	psFindData->currWRFIndex = 0;
	psFindData->currFileIndex = 0;

	return FALSE;
}


// set the current WDG based on the found WRF
void wdgSetCurrentWDGFromFind(WDG_FINDWRF *psFindData)
{
	WDGCACHE	*psCache	= psFindData->psCurrCache;
	WDGINFO		*psWRFData	= psCache->asWRFCatalog + psFindData->currWRFIndex;

	WDG_SetCurrentWDGCatalog(psCache->aFileName,
							 psCache->numWRF,
							 psCache->asWRFCatalog);
	WDG_SetCurrentWRFFileCatalog(psWRFData->offset, psCache->apsWRFFileCatalog[ psFindData->currWRFIndex ]);
}


// load a file that is not stored in the current WDG
BOOL wdgGetFileFromOtherWDG(UDWORD WRFname, UDWORD type, UDWORD FileName,
							WRFINFO **ppsNewFile, UBYTE **ppFileBuf)
{
	WDG_FINDFILE	sFindFile;
	FILE			*pOtherWDG;
	WDGCACHE	*psCache;
	WDGINFO		*psWRFData;
	WRFINFO		*psFileData;
	UDWORD		FileOffset;

	// file is not in this WDG - find it in another
	if (!wdgFindFirstFile(WRFname, type, FileName, &sFindFile))
	{
		return FALSE;
	}

	// open the other WDG
	pOtherWDG=fopen(sFindFile.psCurrCache->aFileName, "rb");
	if (pOtherWDG==NULL)
	{
		DBERROR(("wdgGetFileFromOtherWDG: unable to open %s\n",sFindFile.psCurrCache->aFileName));
		return FALSE;	
	}

	psCache = sFindFile.psCurrCache;
	psWRFData = psCache->asWRFCatalog + sFindFile.currWRFIndex;
	psFileData = psCache->apsWRFFileCatalog[ sFindFile.currWRFIndex ] + sFindFile.currFileIndex;

	FileOffset=psWRFData->offset + sizeof(WRFINFO)*psWRFData->filecount + psFileData->offset;
	
	*ppFileBuf = FILE_RetrieveSingle(pOtherWDG,FileOffset,psFileData->filesize);
	*ppsNewFile = psFileData;

	if ((*ppFileBuf) == NULL)
	{
		return FALSE;
	}

	fclose(pOtherWDG);

	return TRUE;;
}


// Find the first instance of a WRF in the WDG catalogs
// This searches from the end to the begining
BOOL wdgFindFirstWRFRev(UDWORD WRFName, WDG_FINDWRF *psFindData)
{
	UDWORD	i;

	// initialise the find data
	memset(psFindData, 0, sizeof(WDG_FINDWRF));
	psFindData->WRFNameHash = WRFName;
	psFindData->psCurrCache = psWDGCacheRev;

	if (psWDGCacheRev == NULL)
	{
		return FALSE;
	}

	// see if the WRF is in the first WDG
	for(i=0; i<psWDGCacheRev->numWRF; i++)
	{
		if (psWDGCacheRev->asWRFCatalog[i].WRFname == WRFName)
		{
			psFindData->currWRFIndex = i;
			return TRUE;
		}
	}

	// search the remaining WDGs
	if (wdgFindNextWRFRev(psFindData))
	{
		return TRUE;
	}

	return FALSE;
}


// Find the next instance of a WRF in WDG catalogs
// This searches from the end to the begining
BOOL wdgFindNextWRFRev(WDG_FINDWRF *psFindData)
{
	WDGCACHE	*psCurrCache;
	UDWORD		i;

	if (psFindData->psCurrCache == NULL)
	{
		return FALSE;
	}

	// search each WDG cache
	psCurrCache=psFindData->psCurrCache->psPrev;
	for(; psCurrCache != NULL; psCurrCache = psCurrCache->psPrev)
	{
		for(i=0; i<psCurrCache->numWRF; i++)
		{
			if (psCurrCache->asWRFCatalog[i].WRFname == psFindData->WRFNameHash)
			{
				goto found;
			}
		}
	}

	// jump here if a WRF has been found
found:
	if (psCurrCache != NULL)
	{
		psFindData->psCurrCache = psCurrCache;
		psFindData->currWRFIndex = i;
		return TRUE;
	}

	psFindData->psCurrCache = NULL;
	psFindData->currWRFIndex = 0;

	return FALSE;
}


// Find the first instance of a file in a WRF
// This searches from the end to the begining
BOOL wdgFindFirstFileRev(UDWORD WRFName, UDWORD type, UDWORD FileName, WDG_FINDFILE *psFindData)
{
	WDGINFO	*psWRFData;
	WRFINFO	*asFileData;
	UDWORD	i;

	memset(psFindData, 0, sizeof(WDG_FINDFILE));
	psFindData->WRFNameHash = WRFName;
	psFindData->FileType = type;
	psFindData->FileNameHash = FileName;
	psFindData->psCurrCache = psWDGCacheRev;

	if (psWDGCacheRev == NULL)
	{
		return FALSE;
	}

	// find the first WRF
	if (!wdgFindFirstWRFRev(WRFName, (WDG_FINDWRF *)psFindData))
	{
		return FALSE;
	}

	// see if there is an instance of the file in this WRF
	psWRFData = psFindData->psCurrCache->asWRFCatalog + psFindData->currWRFIndex ;
	asFileData = psFindData->psCurrCache->apsWRFFileCatalog[ psFindData->currWRFIndex ];
	for(i=0; i<psWRFData->filecount; i++)
	{
		if ((asFileData[i].name == psFindData->FileNameHash) &&
			(asFileData[i].type == psFindData->FileType) &&
			(asFileData[i].offset != UDWORD_MAX))
		{
			psFindData->currFileIndex = i;
			return TRUE;
		}
	}

	// not in this WRF, try other ones
	if (wdgFindNextFileRev(psFindData))
	{
		return TRUE;
	}

	return FALSE;
}

// Find the next instance of a file in a WRF
// This searches from the end to the begining
BOOL wdgFindNextFileRev(WDG_FINDFILE *psFindData)
{
	WDGINFO	*psWRFData;
	WRFINFO	*asFileData;
	UDWORD	i;

	while (psFindData->psCurrCache != NULL)
	{
		// find the next WRF
		if (!wdgFindNextWRFRev((WDG_FINDWRF *)psFindData))
		{
			goto failed;
		}

		// see if there is an instance of the file in this WRF
		psWRFData = psFindData->psCurrCache->asWRFCatalog + psFindData->currWRFIndex ;
		asFileData = psFindData->psCurrCache->apsWRFFileCatalog[ psFindData->currWRFIndex ];
		for(i=0; i<psWRFData->filecount; i++)
		{
			if ((asFileData[i].name == psFindData->FileNameHash) &&
				(asFileData[i].type == psFindData->FileType) &&
				(asFileData[i].offset != UDWORD_MAX))
			{
				psFindData->currFileIndex = i;
				return TRUE;
			}
		}
	}

failed:
	psFindData->psCurrCache = NULL;
	psFindData->currWRFIndex = 0;
	psFindData->currFileIndex = 0;

	return FALSE;
}


