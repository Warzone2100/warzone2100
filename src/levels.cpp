/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * Levels.c
 *
 * Control the data loading for game levels
 *
 */

#include <ctype.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/listmacs.h"
#include "lib/framework/file.h"
#include "lib/framework/crc.h"
#include "lib/framework/physfs_ext.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include "init.h"
#include "objects.h"
#include "hci.h"
#include "levels.h"
#include "mission.h"
#include "levelint.h"
#include "game.h"
#include "lib/ivis_opengl/piestate.h"
#include "data.h"
#include "lib/ivis_opengl/ivi.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "research.h"
#include "lib/framework/lexer_input.h"
#include "effects.h"
#include "main.h"
#include "multiint.h"

extern int lev_get_lineno(void);
extern char* lev_get_text(void);
extern int lev_lex(void);
extern void lev_set_extra(YY_EXTRA_TYPE user_defined);
extern int lev_lex_destroy(void);

// block ID number start for the current level data (as opposed to a dataset)
#define CURRENT_DATAID		LEVEL_MAXFILES

static	char	currentLevelName[32] = { "main" };

// the current level descriptions
LEVEL_DATASET	*psLevels = NULL;

// the currently loaded data set
static LEVEL_DATASET	*psBaseData = NULL;
static LEVEL_DATASET	*psCurrLevel = NULL;

// dummy level data for single WRF loads
static LEVEL_DATASET	sSingleWRF = {0, 0, 0, 0, mod_clean, {0}, 0, 0, 0, NULL, {{0}}};

// return values from the lexer
char *pLevToken;
SDWORD levVal;
static GAME_TYPE levelLoadType;

// modes for the parser
enum LEVELPARSER_STATE
{
	LP_START,		// no input received
	LP_LEVEL,		// level token received
	LP_LEVELDONE,	// defined a level waiting for players/type/data
	LP_PLAYERS,		// players token received
	LP_TYPE,		// type token received
	LP_DATASET,		// dataset token received
	LP_WAITDATA,	// defining level data, waiting for data token
	LP_DATA,		// data token received
	LP_GAME,		// game token received
};


// initialise the level system
bool levInitialise(void)
{
	psLevels = NULL;
	psBaseData = NULL;
	psCurrLevel = NULL;

	return true;
}

SDWORD getLevelLoadType(void)
{
	return levelLoadType;
}

// shutdown the level system
void levShutDown(void)
{
	while (psLevels != NULL)
	{
		unsigned int i;
		LEVEL_DATASET * const toDelete = psLevels;

		psLevels = psLevels->psNext;

		for (i = 0; i < ARRAY_SIZE(toDelete->apDataFiles); ++i)
		{
			if (toDelete->apDataFiles[i] != NULL)
			{
				free(toDelete->apDataFiles[i]);
			}
		}

		free(toDelete->pName);
		free(toDelete->realFileName);
		free(toDelete);
	}
	psLevels = NULL;
}


// error report function for the level parser
void lev_error(const char* msg)
{
	debug(LOG_ERROR, "Level File parse error: `%s` at line `%d` text `%s`", msg, lev_get_lineno(), lev_get_text());
}

/** Find a level dataset with the given name.
 *  @param name the name of the dataset to search for.
 *  @return a dataset with associated with the given @c name, or NULL if none
 *          could be found.
 */
LEVEL_DATASET *levFindDataSet(char const *name, Sha256 const *hash)
{
	if (hash != NULL && hash->isZero())
	{
		hash = NULL;  // Don't check hash if it's just 0000000000000000000000000000000000000000000000000000000000000000. Assuming real map files probably won't have that particular SHA-256 hash.
	}

	for (LEVEL_DATASET *psNewLevel = psLevels; psNewLevel != NULL; psNewLevel = psNewLevel->psNext)
	{
		if (psNewLevel->pName != NULL && strcmp(psNewLevel->pName, name) == 0)
		{
			if (hash == NULL || levGetFileHash(psNewLevel) == *hash)
			{
				return psNewLevel;
			}
		}
	}

	return NULL;
}

Sha256 levGetFileHash(LEVEL_DATASET *level)
{
	if (level->realFileName != NULL && level->realFileHash.isZero())
	{
		level->realFileHash = findHashOfFile(level->realFileName);
		debug(LOG_WZ, "Hash of file \"%s\" is %s.", level->realFileName, level->realFileHash.toString().c_str());
	}
	return level->realFileHash;
}

Sha256 levGetMapNameHash(char const *mapName)
{
	LEVEL_DATASET *level = levFindDataSet(mapName, NULL);
	if (level == NULL)
	{
		debug(LOG_WARNING, "Couldn't find map \"%s\" to hash.", mapName);
		Sha256 zero;
		zero.setZero();
		return zero;
	}
	return levGetFileHash(level);
}

// parse a level description data file
// the ignoreWrf hack is for compatibility with old maps that try to link in various
// data files that we have removed
bool levParse(const char* buffer, size_t size, searchPathMode datadir, bool ignoreWrf, char const *realFileName)
{
	lexerinput_t input;
	LEVELPARSER_STATE state;
	int token, currData = -1;
	LEVEL_DATASET	*psDataSet = NULL;

	input.type = LEXINPUT_BUFFER;
	input.input.buffer.begin = buffer;
	input.input.buffer.end = &buffer[size];

	lev_set_extra(&input);

	state = LP_START;
	for (token = lev_lex(); token != 0; token = lev_lex())
	{
		switch (token)
		{
		case LTK_LEVEL:
		case LTK_CAMPAIGN:
		case LTK_CAMSTART:
		case LTK_CAMCHANGE:
		case LTK_EXPAND:
		case LTK_BETWEEN:
		case LTK_MKEEP:
		case LTK_MCLEAR:
		case LTK_EXPAND_LIMBO:
		case LTK_MKEEP_LIMBO:
			if (state == LP_START || state == LP_WAITDATA)
			{
				// start a new level data set
				psDataSet = (LEVEL_DATASET*)malloc(sizeof(LEVEL_DATASET));
				if (!psDataSet)
				{
					debug(LOG_FATAL, "Out of memory");
					abort();
					return false;
				}
				memset(psDataSet, 0, sizeof(LEVEL_DATASET));
				psDataSet->players = 1;
				psDataSet->game = -1;
				psDataSet->dataDir = datadir;
				psDataSet->realFileName = realFileName != NULL? strdup(realFileName) : NULL;
				psDataSet->realFileHash.setZero();  // The hash is only calculated on demand; for example, if the map name matches.
				LIST_APPEND(psLevels, psDataSet, LEVEL_DATASET);
				currData = 0;

				// set the dataset type
				switch (token)
				{
				case LTK_LEVEL:
					psDataSet->type = LDS_COMPLETE;
					break;
				case LTK_CAMPAIGN:
					psDataSet->type = LDS_CAMPAIGN;
					break;
				case LTK_CAMSTART:
					psDataSet->type = LDS_CAMSTART;
					break;
				case LTK_BETWEEN:
					psDataSet->type = LDS_BETWEEN;
					break;
				case LTK_MKEEP:
					psDataSet->type = LDS_MKEEP;
					break;
				case LTK_CAMCHANGE:
					psDataSet->type = LDS_CAMCHANGE;
					break;
				case LTK_EXPAND:
					psDataSet->type = LDS_EXPAND;
					break;
				case LTK_MCLEAR:
					psDataSet->type = LDS_MCLEAR;
					break;
				case LTK_EXPAND_LIMBO:
					psDataSet->type = LDS_EXPAND_LIMBO;
					break;
				case LTK_MKEEP_LIMBO:
					psDataSet->type = LDS_MKEEP_LIMBO;
					break;
				default:
					ASSERT( false,"eh?" );
					break;
				}
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			state = LP_LEVEL;
			break;
		case LTK_PLAYERS:
			if (state == LP_LEVELDONE &&
				(psDataSet->type == LDS_COMPLETE || psDataSet->type >= LDS_MULTI_TYPE_START))
			{
				state = LP_PLAYERS;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			break;
		case LTK_TYPE:
			if (state == LP_LEVELDONE && psDataSet->type == LDS_COMPLETE)
			{
				state = LP_TYPE;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			break;
		case LTK_INTEGER:
			if (state == LP_PLAYERS)
			{
				psDataSet->players = (SWORD)levVal;
			}
			else if (state == LP_TYPE)
			{
				if (levVal < LDS_MULTI_TYPE_START)
				{
					lev_error("invalid type number");
					return false;
				}

				psDataSet->type = (SWORD)levVal;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			state = LP_LEVELDONE;
			break;
		case LTK_DATASET:
			if (state == LP_LEVELDONE && psDataSet->type != LDS_COMPLETE)
			{
				state = LP_DATASET;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			break;
		case LTK_DATA:
			if (state == LP_WAITDATA)
			{
				state = LP_DATA;
			}
			else if (state == LP_LEVELDONE)
			{
				if (psDataSet->type == LDS_CAMSTART ||
					psDataSet->type == LDS_MKEEP
					||psDataSet->type == LDS_CAMCHANGE ||
					psDataSet->type == LDS_EXPAND ||
					psDataSet->type == LDS_MCLEAR ||
					psDataSet->type == LDS_EXPAND_LIMBO ||
					psDataSet->type == LDS_MKEEP_LIMBO
					)
				{
					lev_error("Missing dataset command");
					return false;
				}
				state = LP_DATA;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			break;
		case LTK_GAME:
			if ((state == LP_WAITDATA || state == LP_LEVELDONE) &&
				psDataSet->game == -1 && psDataSet->type != LDS_CAMPAIGN)
			{
				state = LP_GAME;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			break;
		case LTK_IDENT:
			if (state == LP_LEVEL)
			{
				if (psDataSet->type == LDS_CAMCHANGE)
				{
					// This is a campaign change dataset, we need to find the full data set.
					LEVEL_DATASET * const psFoundData = levFindDataSet(pLevToken);

					if (psFoundData == NULL)
					{
						lev_error("Cannot find full data set for camchange");
						return false;
					}

					if (psFoundData->type != LDS_CAMSTART)
					{
						lev_error("Invalid data set name for cam change");
						return false;
					}
					psFoundData->psChange = psDataSet;
				}
				// store the level name
				psDataSet->pName = strdup(pLevToken);
				if (psDataSet->pName == NULL)
				{
					debug(LOG_FATAL, "Out of memory!");
					abort();
					return false;
				}

				state = LP_LEVELDONE;
			}
			else if (state == LP_DATASET)
			{
				// find the dataset
				psDataSet->psBaseData = levFindDataSet(pLevToken);

				if (psDataSet->psBaseData == NULL)
				{
					lev_error("Unknown dataset");
					return false;
				}
				state = LP_WAITDATA;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			break;
		case LTK_STRING:
			if (state == LP_DATA || state == LP_GAME)
			{
				if (currData >= LEVEL_MAXFILES)
				{
					lev_error("Too many data files");
					return false;
				}

				// note the game index if necessary
				if (state == LP_GAME)
				{
					psDataSet->game = (SWORD)currData;
				}
				else if (ignoreWrf)
				{
					state = LP_WAITDATA;
					break;	// ignore this wrf line
				}

				// store the data name
				psDataSet->apDataFiles[currData] = strdup(pLevToken);
				if (psDataSet->apDataFiles[currData] == NULL)
				{
					debug(LOG_FATAL, "Out of memory!");
					abort();
					return false;
				}

				resToLower(pLevToken);

				currData += 1;
				state = LP_WAITDATA;
			}
			else
			{
				lev_error("Syntax Error");
				return false;
			}
			break;
		default:
			lev_error("Unexpected token");
			break;
		}
	}

	lev_lex_destroy();

	// Accept empty files when parsing (indicated by currData < 0)
	if (currData >= 0
	 && (state != LP_WAITDATA
	  || currData == 0))
	{
		lev_error("Unexpected end of file");
		return false;
	}

	return true;
}


// free the data for the current mission
bool levReleaseMissionData(void)
{
	SDWORD i;

	// release old data if any was loaded
	if (psCurrLevel != NULL)
	{
		if (!stageThreeShutDown())
		{
			return false;
		}

		// free up the old data
		for(i=LEVEL_MAXFILES-1; i >= 0; i--)
		{
			if (i == psCurrLevel->game)
			{
				if (psCurrLevel->psBaseData == NULL)
				{
					if (!stageTwoShutDown())
					{
						return false;
					}
				}
			}
			else// if (psCurrLevel->apDataFiles[i])
			{

				resReleaseBlockData(i + CURRENT_DATAID);
			}
		}
	}
	return true;
}


// free the currently loaded dataset
bool levReleaseAll(void)
{
	SDWORD i;

	// clear out old effect data first
	initEffectsSystem();
	// release old data if any was loaded
	if (psCurrLevel != NULL)
	{
		if (!levReleaseMissionData())
		{
			debug(LOG_ERROR, "Failed to unload mission data");
			return false;
		}

		// release the game data
		if (psCurrLevel->psBaseData != NULL)
		{
			if (!stageTwoShutDown())
			{
				debug(LOG_ERROR, "Failed stage two shutdown");
				return false;
			}
		}


		if (psCurrLevel->psBaseData)
		{
			for(i=LEVEL_MAXFILES-1; i >= 0; i--)
			{
				if (psCurrLevel->psBaseData->apDataFiles[i])
				{
					resReleaseBlockData(i);
				}
			}
		}

		if (!stageOneShutDown())
		{
			debug(LOG_ERROR, "Failed stage one shutdown");
			return false;
		}
	}

	psCurrLevel=NULL;

	return true;
}

// load up a single wrf file
static bool levLoadSingleWRF(const char* name)
{
	// free the old data
	if (!levReleaseAll())
	{
		return false;
	}

	// create the dummy level data
	if (sSingleWRF.pName)
	{
		free(sSingleWRF.pName);
	}

	memset(&sSingleWRF, 0, sizeof(LEVEL_DATASET));
	sSingleWRF.pName = strdup(name);

	// load up the WRF
	if (!stageOneInitialise())
	{
		return false;
	}

	// load the data
	debug(LOG_WZ, "Loading %s ...", name);
	if (!resLoad(name, 0))
	{
		return false;
	}

	if (!stageThreeInitialise())
	{
		return false;
	}

	psCurrLevel = &sSingleWRF;

	return true;
}


char *getLevelName( void )
{
	return(currentLevelName);
}


// load up the data for a level
bool levLoadData(char const *name, Sha256 const *hash, char *pSaveName, GAME_TYPE saveType)
{
	LEVEL_DATASET	*psNewLevel, *psBaseData, *psChangeLevel;
	SDWORD			i;
	bool            bCamChangeSaveGame;

	debug(LOG_WZ, "Loading level %s hash %s (%s, type %d)", name, hash == NULL? "builtin" : hash->toString().c_str(), pSaveName, (int)saveType);
	if (saveType == GTYPE_SAVE_START || saveType == GTYPE_SAVE_MIDMISSION)
	{
		if (!levReleaseAll())
		{
			debug(LOG_ERROR, "Failed to unload old data");
			return false;
		}
	}

	levelLoadType = saveType;

	// find the level dataset
	psNewLevel = levFindDataSet(name, hash);
	if (psNewLevel == NULL)
	{
		debug(LOG_INFO, "Dataset %s not found - trying to load as WRF", name);
		return levLoadSingleWRF(name);
	}
	debug(LOG_WZ, "** Data set found is %s type %d", psNewLevel->pName, (int)psNewLevel->type);

	/* Keep a copy of the present level name */
	sstrcpy(currentLevelName, name);

	bCamChangeSaveGame = false;
	if (pSaveName && saveType == GTYPE_SAVE_START)
	{
		if (psNewLevel->psChange != NULL)
		{
			bCamChangeSaveGame = true;
			debug(LOG_WZ, "** CAMCHANGE FOUND");
		}
	}

	// select the change dataset if there is one
	psChangeLevel = NULL;
	if (((psNewLevel->psChange != NULL) && (psCurrLevel != NULL)) || bCamChangeSaveGame)
	{
		//store the level name
		debug(LOG_WZ, "Found CAMCHANGE dataset");
		psChangeLevel = psNewLevel;
		psNewLevel = psNewLevel->psChange;
	}

	// ensure the correct dataset is loaded
	if (psNewLevel->type == LDS_CAMPAIGN)
	{
		debug(LOG_ERROR, "Cannot load a campaign dataset (%s)", psNewLevel->pName);
		return false;
	}
	else
	{
		if (psCurrLevel != NULL)
		{
			if ((psCurrLevel->psBaseData != psNewLevel->psBaseData) ||
				(psCurrLevel->type < LDS_NONE && psNewLevel->type  >= LDS_NONE) ||
				(psCurrLevel->type >= LDS_NONE && psNewLevel->type  < LDS_NONE))
			{
				// there is a dataset loaded but it isn't the correct one
				debug(LOG_WZ, "Incorrect base dataset loaded (%p != %p, %d - %d)",
				      psCurrLevel->psBaseData, psNewLevel->psBaseData, (int)psCurrLevel->type, (int)psNewLevel->type);
				if (!levReleaseAll())	// this sets psCurrLevel to NULL
				{
					debug(LOG_ERROR, "Failed to release old data");
					return false;
				}
			}
			else
			{
				debug(LOG_WZ, "Correct base dataset already loaded.");
			}
		}

		// setup the correct dataset to load if necessary
		if (psCurrLevel == NULL)
		{
			if (psNewLevel->psBaseData != NULL)
			{
				debug(LOG_WZ, "Setting base dataset to load: %s", psNewLevel->psBaseData->pName);
			}
			psBaseData = psNewLevel->psBaseData;
		}
		else
		{
			debug(LOG_WZ, "No base dataset to load");
			psBaseData = NULL;
		}
	}

	setCurrentMap(psNewLevel->realFileName, psNewLevel->players);
	if (!rebuildSearchPath(psNewLevel->dataDir, true))
	{
		debug(LOG_ERROR, "Failed to rebuild search path");
		return false;
	}

	// reset the old mission data if necessary
	if (psCurrLevel != NULL)
	{
		debug(LOG_WZ, "Reseting old mission data");
		if (!levReleaseMissionData())
		{
			debug(LOG_ERROR, "Failed to unload old mission data");
			return false;
		}
	}

	// need to free the current map and droids etc for a save game
	if ((psBaseData == NULL) &&
		(pSaveName != NULL))
	{
		if (!saveGameReset())
		{
			debug(LOG_ERROR, "Failed to saveGameReset()!");
			return false;
		}
	}

	// initialise if necessary
	if (psNewLevel->type == LDS_COMPLETE || //psNewLevel->type >= LDS_MULTI_TYPE_START ||
		psBaseData != NULL)
	{
		debug(LOG_WZ, "Calling stageOneInitialise!");
		if (!stageOneInitialise())
		{
			debug(LOG_ERROR, "Failed stageOneInitialise!");
			return false;
		}
	}

	// load up a base dataset if necessary
	if (psBaseData != NULL)
	{
		debug(LOG_WZ, "Loading base dataset %s", psBaseData->pName);
		for(i=0; i<LEVEL_MAXFILES; i++)
		{
			if (psBaseData->apDataFiles[i])
			{
				// load the data
				debug(LOG_WZ, "Loading [directory: %s] %s ...", PHYSFS_getRealDir(psBaseData->apDataFiles[i]), psBaseData->apDataFiles[i]);
				if (!resLoad(psBaseData->apDataFiles[i], i))
				{
					debug(LOG_ERROR, "Failed resLoad(%s)!", psBaseData->apDataFiles[i]);
					return false;
				}
			}
		}
	}
	if (psNewLevel->type == LDS_CAMCHANGE)
	{
		if (!campaignReset())
		{
			debug(LOG_ERROR, "Failed campaignReset()!");
			return false;
		}
	}
	if (psNewLevel->game == -1)  //no .gam file to load - BETWEEN missions (for Editor games only)
	{
		ASSERT( psNewLevel->type == LDS_BETWEEN,
			"levLoadData: only BETWEEN missions do not need a .gam file" );
		debug(LOG_WZ, "No .gam file for level: BETWEEN mission");
		if (pSaveName != NULL)
		{
			if (psBaseData != NULL)
			{
				if (!stageTwoInitialise())
				{
					debug(LOG_ERROR, "Failed stageTwoInitialise()!");
					return false;
				}
			}

			//set the mission type before the saveGame data is loaded
			if (saveType == GTYPE_SAVE_MIDMISSION)
			{
				debug(LOG_WZ, "Init mission stuff");
				if (!startMissionSave(psNewLevel->type))
				{
					debug(LOG_ERROR, "Failed startMissionSave(%d)!", psNewLevel->type);
					return false;
				}

				debug(LOG_NEVER, "dataSetSaveFlag");
				dataSetSaveFlag();
			}

			debug(LOG_NEVER, "Loading savegame: %s", pSaveName);
			if (!loadGame(pSaveName, false, true,true))
			{
				debug(LOG_ERROR, "Failed loadGame(%s)!", pSaveName);
				return false;
			}
		}

		if ((pSaveName == NULL) ||
			(saveType == GTYPE_SAVE_START))
		{
			debug(LOG_NEVER, "Start mission - no .gam");
			if (!startMission((LEVEL_TYPE)psNewLevel->type, NULL))
			{
				debug(LOG_ERROR, "Failed startMission(%d)!", psNewLevel->type);
				return false;
			}
		}
	}

	//we need to load up the save game data here for a camchange
	if (bCamChangeSaveGame)
	{
		if (pSaveName != NULL)
		{
			if (psBaseData != NULL)
			{
				if (!stageTwoInitialise())
				{
					debug(LOG_ERROR, "Failed stageTwoInitialise() [camchange]!");
					return false;
				}
			}

			debug(LOG_NEVER, "loading savegame: %s", pSaveName);
			if (!loadGame(pSaveName, false, true,true))
			{
				debug(LOG_ERROR, "Failed loadGame(%s)!", pSaveName);
				return false;
			}

			if (!campaignReset())
			{
				return false;
			}
		}
	}


	// load the new data
	debug(LOG_NEVER, "Loading mission dataset: %s", psNewLevel->pName);
	for(i=0; i < LEVEL_MAXFILES; i++)
	{
		if (psNewLevel->game == i)
		{
			// do some more initialising if necessary
			if (psNewLevel->type == LDS_COMPLETE || psNewLevel->type >= LDS_MULTI_TYPE_START || (psBaseData != NULL && !bCamChangeSaveGame))
			{
				if (!stageTwoInitialise())
				{
					debug(LOG_ERROR, "Failed stageTwoInitialise() [newdata]!");
					return false;
				}
			}

			// load a savegame if there is one - but not if already done so
			if (pSaveName != NULL && !bCamChangeSaveGame)
			{
				//set the mission type before the saveGame data is loaded
				if (saveType == GTYPE_SAVE_MIDMISSION)
				{
					debug(LOG_WZ, "Init mission stuff");
					if (!startMissionSave(psNewLevel->type))
					{
						debug(LOG_ERROR, "Failed startMissionSave(%d)!", psNewLevel->type);
						return false;
					}

					debug(LOG_NEVER, "dataSetSaveFlag");
					dataSetSaveFlag();
				}

				debug(LOG_NEVER, "Loading save game %s", pSaveName);
				if (!loadGame(pSaveName, false, true,true))
				{
					debug(LOG_ERROR, "Failed loadGame(%s)!", pSaveName);
					return false;
				}
			}

			if ((pSaveName == NULL) ||
				(saveType == GTYPE_SAVE_START))
			{
				// load the game
				debug(LOG_WZ, "Loading scenario file %s", psNewLevel->apDataFiles[i]);
				switch (psNewLevel->type)
				{
				case LDS_COMPLETE:
				case LDS_CAMSTART:
					debug(LOG_WZ, "LDS_COMPLETE / LDS_CAMSTART");
					if (!startMission(LDS_CAMSTART, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_CAMSTART, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;
				case LDS_BETWEEN:
					debug(LOG_WZ, "LDS_BETWEEN");
					if (!startMission(LDS_BETWEEN, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_BETWEEN, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;

				case LDS_MKEEP:
					debug(LOG_WZ, "LDS_MKEEP");
					if (!startMission(LDS_MKEEP, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_MKEEP, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;
				case LDS_CAMCHANGE:
					debug(LOG_WZ, "LDS_CAMCHANGE");
					if (!startMission(LDS_CAMCHANGE, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_CAMCHANGE, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;

				case LDS_EXPAND:
					debug(LOG_WZ, "LDS_EXPAND");
					if (!startMission(LDS_EXPAND, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_EXPAND, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;
				case LDS_EXPAND_LIMBO:
					debug(LOG_WZ, "LDS_LIMBO");
					if (!startMission(LDS_EXPAND_LIMBO, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_EXPAND_LIMBO, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;

				case LDS_MCLEAR:
					debug(LOG_WZ, "LDS_MCLEAR");
					if (!startMission(LDS_MCLEAR, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_MCLEAR, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;
				case LDS_MKEEP_LIMBO:
					debug(LOG_WZ, "LDS_MKEEP_LIMBO");
					if (!startMission(LDS_MKEEP_LIMBO, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s)!", LDS_MKEEP_LIMBO, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;
				default:
					ASSERT( psNewLevel->type >= LDS_MULTI_TYPE_START, "Unexpected mission type" );
					debug(LOG_WZ, "default (MULTIPLAYER)");
					if (!startMission(LDS_CAMSTART, psNewLevel->apDataFiles[i]))
					{
						debug(LOG_ERROR, "Failed startMission(%d, %s) (default)!", LDS_CAMSTART, psNewLevel->apDataFiles[i]);
						return false;
					}
					break;
				}
			}
		}
		else if (psNewLevel->apDataFiles[i])
		{
			// load the data
			debug(LOG_WZ, "Loading %s", psNewLevel->apDataFiles[i]);
			if (!resLoad(psNewLevel->apDataFiles[i], i + CURRENT_DATAID))
			{
				debug(LOG_ERROR, "Failed resLoad(%s, %d) (default)!", psNewLevel->apDataFiles[i], i + CURRENT_DATAID);
				return false;
			}
		}
	}

	if (pSaveName != NULL)
	{
		//load MidMission Extras
		if (!loadMissionExtras(pSaveName, psNewLevel->type))
		{
			debug(LOG_ERROR, "Failed loadMissionExtras(%s, %d)!", pSaveName, psNewLevel->type);
			return false;
		}
	}

	if (bMultiPlayer)
	{
		loadMultiScripts();
	}
	if (pSaveName != NULL && saveType == GTYPE_SAVE_MIDMISSION)
	{
		//load script stuff
		// load the event system state here for a save game
		debug(LOG_SAVE, "Loading script system state");
		if (!loadScriptState(pSaveName))
		{
			debug(LOG_ERROR, "Failed loadScriptState(%s)!", pSaveName);
			return false;
		}
	}

	if (!stageThreeInitialise())
	{
		debug(LOG_ERROR, "Failed stageThreeInitialise()!");
		return false;
	}

	dataClearSaveFlag();

	//this enables us to to start cam2/cam3 without going via a save game and get the extra droids
	//in from the script-controlled Transporters
	if (!pSaveName && psNewLevel->type == LDS_CAMSTART)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NO_REINFORCEMENTS_LEFT);
	}

	//restore the level name for comparisons on next mission load up
	if (psChangeLevel == NULL)
	{
		psCurrLevel = psNewLevel;
	}
	else
	{
		psCurrLevel = psChangeLevel;
	}

	{
		// Copy this info to be used by the crash handler for the dump file
		char buf[256];

		ssprintf(buf, "Current Level/map is %s", psCurrLevel->pName);
		addDumpInfo(buf);
	}


	return true;
}
