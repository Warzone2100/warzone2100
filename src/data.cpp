/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
/**
 * @file data.c
 *
 * Data loading functions used by the framework resource module.
 *
 */

#include <physfs.h>
#include "lib/framework/physfs_ext.h"

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"
#include "lib/framework/crc.h"
#include "lib/gamelib/parser.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/png_util.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"

#include "qtscript.h"
#include "data.h"
#include "droid.h"
#include "feature.h"
#include "mechanics.h"
#include "message.h"
#include "multiplay.h"
#include "research.h"
#include "scriptvals.h"
#include "stats.h"
#include "template.h"
#include "text.h"
#include "texture.h"

// whether a save game is currently being loaded
static bool saveFlag = false;

uint32_t	DataHash[DATA_MAXDATA] = {0};

/**
*	hashBuffer()
*	\param pData pointer to our buffer (always a text file)
*	\param size the size of the buffer
*	\return hash calculated from the buffer.
*
*	Note, this is obviously not a very complex hash routine.  This most likely has many collisions possible.
*	This is almost the same routine that Pumpkin had, minus the ugly bug :)
*	And minus the old algorithm and debugging trace, replaced with a simple CRC...
*/
static UDWORD	hashBuffer(const uint8_t *pData, uint32_t size)
{
	char nl = '\n';
	uint32_t crc = 0;
	uint32_t i, j;
	uint32_t lines = 0;
	uint32_t bytes = 0;
	for (i = 0; i < size; i = j + 1)
	{
		for (j = i; j < size && pData[j] != '\n' && pData[j] != '\r'; ++j)
		{}

		if (i != j)  // CRC non-empty lines only.
		{
			crc = crcSum(crc, pData + i, j - i);  // CRC the line.
			crc = crcSum(crc, &nl, 1);            // CRC the line ending.

			++lines;
			bytes += j - i + 1;
		}
	}
	debug(LOG_NET, "The size of the old buffer (%u bytes - %d stripped), New buffer size of %u bytes, %u non-empty lines.", size, size - bytes, bytes, lines);

	return ~crc;
}

// create the hash for that data block.
// Data should be converted to Network byte order
static void calcDataHash(const uint8_t *pBuffer, uint32_t size, uint32_t index)
{
	const uint32_t oldHash = DataHash[index];

	if (!bMultiPlayer)
	{
		return;
	}

	DataHash[index] += hashBuffer(pBuffer, size);

	if (!DataHash[index] && oldHash)
	{
		debug(LOG_NET, "The new hash is 0, the old hash was %u. We added the negated value!", oldHash);
	}

	debug(LOG_NET, "DataHash[%2u] = %08x", index, DataHash[index]);

	return;
}

static void calcDataHash(const WzConfig &ini, uint32_t index)
{
	std::string jsonDump = ini.compactStringRepresentation();
	calcDataHash(reinterpret_cast<const uint8_t *>(jsonDump.data()), jsonDump.size(), index);
}

void resetDataHash()
{
	UDWORD i;
	for (i = 0; i < DATA_MAXDATA; i++)
	{
		DataHash[i] = 0;
	}
	debug(LOG_NET, "== Hash is reset ==");
}

/**********************************************************/


void dataSetSaveFlag()
{
	saveFlag = true;
}
void dataClearSaveFlag()
{
	saveFlag = false;
}

/* Load the body stats */
static bool bufferSBODYLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SBODY);

	if (!loadBodyStats(ini) || !allocComponentList(COMP_BODY, numBodyStats))
	{
		return false;
	}
	*ppData = (void *)1;
	return true;
}

static void dataReleaseStats(WZ_DECL_UNUSED void *pData)
{
	// FIXME, huge hack, is called many times!
	freeComponentLists();
	statsShutDown();
}


/* Load the weapon stats */
static bool bufferSWEAPONLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SWEAPON);

	if (!loadWeaponStats(ini)
	    || !allocComponentList(COMP_WEAPON, numWeaponStats))
	{
		return false;
	}

	// not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the constructor stats */
static bool bufferSCONSTRLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SCONSTR);

	if (!loadConstructStats(ini)
	    || !allocComponentList(COMP_CONSTRUCT, numConstructStats))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the ECM stats */
static bool bufferSECMLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SECM);

	if (!loadECMStats(ini)
	    || !allocComponentList(COMP_ECM, numECMStats))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the Propulsion stats */
static bool bufferSPROPLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SPROP);

	if (!loadPropulsionStats(ini) || !allocComponentList(COMP_PROPULSION, numPropulsionStats))
	{
		return false;
	}

	//not interested in this value
	*ppData = (void *)1;
	return true;
}

static bool bufferSSENSORLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SSENSOR);

	if (!loadSensorStats(ini)
	    || !allocComponentList(COMP_SENSOR, numSensorStats))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the Repair stats */
static bool bufferSREPAIRLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SREPAIR);

	if (!loadRepairStats(ini) || !allocComponentList(COMP_REPAIRUNIT, numRepairStats))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the Brain stats */
static bool bufferSBRAINLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SBRAIN);

	if (!loadBrainStats(ini) || !allocComponentList(COMP_BRAIN, numBrainStats))
	{
		return false;
	}
	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the PropulsionType stats */
static bool bufferSPROPTYPESLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SPROPTY);

	if (!loadPropulsionTypes(ini))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the propulsion type sound stats */
static bool bufferSPROPSNDLoad(const char *fileName, void **ppData)
{
	if (!loadPropulsionSounds(fileName))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the STERRTABLE stats */
static bool bufferSTERRTABLELoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_STERRT);

	if (!loadTerrainTable(ini))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the body/propulsion IMDs stats -- FIXME, REMOVE IT, NOT USED */
static bool bufferSBPIMDLoad(const char *fileName, void **ppData)
{
	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the Weapon Effect modifier stats */
static bool bufferSWEAPMODLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SWEAPMOD);

	if (!loadWeaponModifiers(ini))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}


/* Load the Template stats */
static bool bufferSTEMPLLoad(const char *fileName, void **ppData)
{
	if (!loadDroidTemplates(fileName))
	{
		return false;
	}

	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return true;
}

// release the templates
static void dataSTEMPLRelease(WZ_DECL_UNUSED void *pData)
{
	//free the storage allocated to the droid templates
	droidTemplateShutDown();
}

/* Load the Structure stats */
static bool bufferSSTRUCTLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SSTRUCT);

	if (!loadStructureStats(ini))
	{
		return false;
	}

	if (!allocStructLists())
	{
		return false;
	}

	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return true;
}

// release the structure stats
static void dataSSTRUCTRelease(WZ_DECL_UNUSED void *pData)
{
	freeStructureLists();
	structureStatsShutDown();
}

/* Load the Structure strength modifier stats */
static bool bufferSSTRMODLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SSTRMOD);

	if (!loadStructureStrengthModifiers(ini))
	{
		return false;
	}

	//not interested in this value
	*ppData = nullptr;
	return true;
}

/* Load the Feature stats */
static bool bufferSFEATLoad(const char *fileName, void **ppData)
{
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_SFEAT);

	if (!loadFeatureStats(ini))
	{
		return false;
	}
	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return true;
}

// free the feature stats
static void dataSFEATRelease(WZ_DECL_UNUSED void *pData)
{
	featureStatsShutDown();
}

// release the research stats
static void dataRESCHRelease(WZ_DECL_UNUSED void *pData)
{
	//free the storage allocated to the stats
	ResearchShutDown();
}

/* Load the Research stats */
static bool bufferRESCHLoad(const char *fileName, void **ppData)
{
	//check to see if already loaded
	if (!asResearch.empty())
	{
		//release previous data before loading in the new
		dataRESCHRelease(nullptr);
	}

	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	calcDataHash(ini, DATA_RESCH);

	if (!loadResearch(ini))
	{
		return false;
	}

	return true;
}

/* Load the message viewdata */
static bool bufferSMSGLoad(const char *pBuffer, UDWORD size, void **ppData)
{
	WzString *ptr = loadViewData(pBuffer, size);
	if (!ptr)
	{
		return false;
	}

	// set the pointer so the release function gets called with it
	*ppData = (void *)ptr;
	return true;
}

/* Load research message viewdata */
static bool dataResearchMsgLoad(const char *fileName, void **ppData)
{
	WzString *ptr = loadResearchViewData(fileName);
	if (!ptr)
	{
		return false;
	}

	// set the pointer so the release function gets called with it
	*ppData = (void *)ptr;
	return true;
}

// release the message viewdata
static void dataSMSGRelease(void *pData)
{
	ASSERT(pData, "pData unexpectedly null");
	WzString *pFilename = static_cast<WzString *>(pData);
	viewDataShutDown(pFilename->toUtf8().c_str());
	delete pFilename;
}

/*!
 * Load an image from file
 */
static bool dataImageLoad(const char *fileName, void **ppData)
{
	iV_Image *psSprite = (iV_Image *)malloc(sizeof(iV_Image));
	if (!psSprite)
	{
		return false;
	}

	if (!iV_loadImage_PNG(fileName, psSprite))
	{
		debug(LOG_ERROR, "IMGPAGE load failed");
		free(psSprite);
		return false;
	}

	*ppData = psSprite;

	return true;
}


// Tertiles (terrain tiles) loader.
static bool dataTERTILESLoad(const char *fileName, void **ppData)
{
	bool status;

	status = texLoad(fileName);
	ASSERT_OR_RETURN(false, status, "Error loading tertiles!");
	debug(LOG_TEXTURE, "HW Tiles loaded");

	*ppData = nullptr;	// don't bother calling cleanup

	return true;
}

static bool dataIMGLoad(const char *fileName, void **ppData)
{
	*ppData = iV_LoadImageFile(fileName);
	if (*ppData == nullptr)
	{
		return false;
	}

	return true;
}


static void dataIMGRelease(void *pData)
{
	iV_FreeImageFile((IMAGEFILE *)pData);
}

/*!
 * Release an Image
 */
static void dataImageRelease(void *pData)
{
	iV_Image *psSprite = (iV_Image *) pData;

	if (psSprite)
	{
		free(psSprite);
	}
}


/* Load an audio file */
static bool dataAudioLoad(const char *fileName, void **ppData)
{
	if (audio_Disabled() == true)
	{
		*ppData = nullptr;
		// No error occurred (sound is just disabled), so we return true
		return true;
	}

	// Load the track from a file
	*ppData = sound_LoadTrackFromFile(fileName);

	return *ppData != nullptr;
}

/* Load an audio file */
static bool dataAudioCfgLoad(const char *fileName, void **ppData)
{
	bool success;
	PHYSFS_file *fileHandle;

	*ppData = nullptr;

	if (audio_Disabled())
	{
		return true;
	}
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
	fileHandle = PHYSFS_openRead(fileName);

	if (fileHandle == nullptr)
	{
		return false;
	}

	success = ParseResourceFile(fileHandle);

	PHYSFS_close(fileHandle);

	return success;
}

/* Load a string resource file */
static bool dataStrResLoad(const char *fileName, void **ppData)
{
	// recreate the string resource if it was freed by a WRF release
	if (psStringRes == nullptr)
	{
		if (!stringsInitialise())
		{
			return false;
		}
	}

	if (!strresLoad(psStringRes, fileName))
	{
		return false;
	}

	*ppData = psStringRes;
	return true;
}

static void dataStrResRelease(WZ_DECL_UNUSED void *pData)
{
	if (psStringRes != nullptr)
	{
		strresDestroy(psStringRes);
		psStringRes = nullptr;
	}
}


/* Load a script file */
// All scripts, binary or otherwise are now passed through this routine
static bool dataScriptLoad(const char *fileName, void **ppData)
{
	static const bool printHack = false;
	SCRIPT_CODE **psProg = (SCRIPT_CODE **)ppData;
	PHYSFS_file *fileHandle;
	uint8_t *pBuffer;
	PHYSFS_sint64 fileSize = 0;

	debug(LOG_WZ, "COMPILING SCRIPT ...%s", GetLastResourceFilename());

	fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
	if (fileHandle == nullptr)
	{
		return false;
	}

	// due to the changes in r2531 we must do this routine a bit different.
	fileSize = PHYSFS_fileLength(fileHandle);

	pBuffer = (uint8_t *)malloc(fileSize * sizeof(uint8_t));
	ASSERT_OR_RETURN(false, pBuffer, "Out of memory");

	WZ_PHYSFS_readBytes(fileHandle, pBuffer, fileSize);

	calcDataHash(pBuffer, fileSize, DATA_SCRIPT);

	free(pBuffer);

	PHYSFS_seek(fileHandle, 0);		//reset position

	*psProg = scriptCompile(fileHandle, SCRIPTTYPE);

	PHYSFS_close(fileHandle);

	if (!*psProg)		// see script.h
	{
		debug(LOG_ERROR, "Script %s did not compile", GetLastResourceFilename());
		return false;
	}

	if (printHack)
	{
		cpPrintProgram(*psProg);
	}

	return true;
}


static void dataScriptRelease(void *pData)
{
	SCRIPT_CODE *psCode = (SCRIPT_CODE *)pData;
	scriptFreeCode(psCode);
}

static bool jsLoad(const char *fileName, void **ppData)
{
	debug(LOG_WZ, "jsload: %s", fileName);
	*ppData = nullptr;
	return loadGlobalScript(fileName);
}

// Load a script variable values file
static bool dataScriptLoadVals(const char *fileName, void **ppData)
{
	bool success;
	PHYSFS_file *fileHandle;
	uint8_t *pBuffer;
	PHYSFS_sint64 fileSize = 0;

	*ppData = nullptr;

	// don't load anything if a saved game is being loaded
	if (saveFlag)
	{
		return true;
	}

	debug(LOG_WZ, "Loading script data %s", GetLastResourceFilename());

	fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
	if (fileHandle == nullptr)
	{
		return false;
	}
	// due to the changes in r2532 we must do this routine a bit different.
	fileSize = PHYSFS_fileLength(fileHandle);

	pBuffer = (uint8_t *)malloc(fileSize * sizeof(uint8_t));
	ASSERT_OR_RETURN(false, pBuffer, "Out of memory");

	WZ_PHYSFS_readBytes(fileHandle, pBuffer, fileSize);

	calcDataHash(pBuffer, fileSize, DATA_SCRIPTVAL);

	free(pBuffer);

	PHYSFS_seek(fileHandle, 0);		//reset position

	success = scrvLoad(fileHandle);

	if (!success)
	{
		debug(LOG_FATAL, "Script %s did not compile", GetLastResourceFilename());
	}

	PHYSFS_close(fileHandle);

	return success;
}

// New reduced resource type ... specially for PSX
// These are statically defined in data.c
// this is also defined in frameresource.c - needs moving to a .h file
// This basically matches the argument list of resAddBufferLoad in frameresource.c
struct RES_TYPE_MIN_BUF
{
	const char *aType;                      ///< points to the string defining the type (e.g. SCRIPT) - NULL indicates end of list
	RES_BUFFERLOAD buffLoad;                ///< routine to process the data for this type
	RES_FREE release;                       ///< routine to release the data (NULL indicates none)
};

static const RES_TYPE_MIN_BUF BufferResourceTypes[] =
{
	{"SMSG", bufferSMSGLoad, dataSMSGRelease},
	{"IMD", nullptr, nullptr}, // ignored
};

struct RES_TYPE_MIN_FILE
{
	const char *aType;                      ///< points to the string defining the type (e.g. SCRIPT) - NULL indicates end of list
	RES_FILELOAD fileLoad;                  ///< routine to process the data for this type
	RES_FREE release;                       ///< routine to release the data (NULL indicates none)
};

static const RES_TYPE_MIN_FILE FileResourceTypes[] =
{
	{"SFEAT", bufferSFEATLoad, dataSFEATRelease},                  //feature stats file
	{"STEMPL", bufferSTEMPLLoad, dataSTEMPLRelease},               //template and associated files
	{"WAV", dataAudioLoad, (RES_FREE)sound_ReleaseTrack},
	{"SWEAPON", bufferSWEAPONLoad, dataReleaseStats},
	{"SBPIMD", bufferSBPIMDLoad, dataReleaseStats},
	{"SBRAIN", bufferSBRAINLoad, dataReleaseStats},
	{"SSENSOR", bufferSSENSORLoad, dataReleaseStats},
	{"SECM", bufferSECMLoad, dataReleaseStats},
	{"SREPAIR", bufferSREPAIRLoad, dataReleaseStats},
	{"SCONSTR", bufferSCONSTRLoad, dataReleaseStats},
	{"SPROP", bufferSPROPLoad, dataReleaseStats},
	{"SPROPTYPES", bufferSPROPTYPESLoad, dataReleaseStats},
	{"STERRTABLE", bufferSTERRTABLELoad, dataReleaseStats},
	{"SBODY", bufferSBODYLoad, dataReleaseStats},
	{"SWEAPMOD", bufferSWEAPMODLoad, dataReleaseStats},
	{"SPROPSND", bufferSPROPSNDLoad, dataReleaseStats},
	{"AUDIOCFG", dataAudioCfgLoad, nullptr},
	{"IMGPAGE", dataImageLoad, dataImageRelease},
	{"TERTILES", dataTERTILESLoad, nullptr},
	{"IMG", dataIMGLoad, dataIMGRelease},
	{"TEXPAGE", nullptr, nullptr}, // ignored
	{"TCMASK", nullptr, nullptr}, // ignored
	{"SCRIPT", dataScriptLoad, dataScriptRelease},
	{"SCRIPTVAL", dataScriptLoadVals, nullptr},
	{"STR_RES", dataStrResLoad, dataStrResRelease},
	{"RESEARCHMSG", dataResearchMsgLoad, dataSMSGRelease },
	{"SSTRMOD", bufferSSTRMODLoad, nullptr},
	{"JAVASCRIPT", jsLoad, nullptr},
	{"SSTRUCT", bufferSSTRUCTLoad, dataSSTRUCTRelease},            //structure stats and associated files
	{"RESCH", bufferRESCHLoad, dataRESCHRelease},                  //research stats files
};

/* Pass all the data loading functions to the framework library */
bool dataInitLoadFuncs()
{
	// init the data integrity hash;
	resetDataHash();
	// Using iterator style: begin iterator (ResourceTypes),
	// end iterator (EndType), and current iterator (CurrentType)

	// iterate through buffer load functions
	{
		const RES_TYPE_MIN_BUF *CurrentType;
		// Points just past the last item in the list
		const RES_TYPE_MIN_BUF *const EndType = &BufferResourceTypes[sizeof(BufferResourceTypes) / sizeof(RES_TYPE_MIN_BUF)];

		for (CurrentType = BufferResourceTypes; CurrentType != EndType; ++CurrentType)
		{
			if (!resAddBufferLoad(CurrentType->aType, CurrentType->buffLoad, CurrentType->release))
			{
				return false; // error whilst adding a buffer load
			}
		}
	}

	// iterate through file load functions
	{
		const RES_TYPE_MIN_FILE *CurrentType;
		// Points just past the last item in the list
		const RES_TYPE_MIN_FILE *const EndType = &FileResourceTypes[sizeof(FileResourceTypes) / sizeof(RES_TYPE_MIN_BUF)];

		for (CurrentType = FileResourceTypes; CurrentType != EndType; ++CurrentType)
		{
			if (!resAddFileLoad(CurrentType->aType, CurrentType->fileLoad, CurrentType->release))
			{
				return false; // error whilst adding a file load
			}
		}
	}

	return true;
}
