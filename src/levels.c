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
/*
 * Levels.c
 *
 * Control the data loading for game levels
 *
 */

#include <ctype.h>
#include <string.h>

// levLoadData printf's
#define DEBUG_GROUP0
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/listmacs.h"
#include "init.h"
#include "objects.h"
#include "hci.h"
#include "levels.h"
#include "mission.h"
#include "levelint.h"
#include "game.h"
#include "lighting.h"
#include "lib/ivis_common/piestate.h"
#include "data.h"
#include "lib/ivis_common/ivi.h"
#include "lib/script/script.h"
#include "scripttabs.h"

// minimum type number for a type instruction
#define MULTI_TYPE_START	10

// block ID number start for the current level data (as opposed to a dataset)
#define CURRENT_DATAID		LEVEL_MAXFILES

static	char	currentLevelName[32];

// the current level descriptions
LEVEL_DATASET	*psLevels;

// the currently loaded data set
static LEVEL_DATASET	*psBaseData;
static LEVEL_DATASET	*psCurrLevel;

// dummy level data for single WRF loads
static LEVEL_DATASET	sSingleWRF;

// return values from the lexer
char *pLevToken;
SDWORD levVal;
SDWORD levelLoadType;
// modes for the parser
enum
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
BOOL levInitialise(void)
{
	psLevels = NULL;
	psBaseData = NULL;
	psCurrLevel = NULL;

	return TRUE;
}

SDWORD getLevelLoadType(void)
{
	return levelLoadType;
}

// shutdown the level system
void levShutDown(void)
{
	LEVEL_DATASET	*psNext;
	SDWORD			i;

	while (psLevels)
	{
		free(psLevels->pName);
		for(i=0; i<LEVEL_MAXFILES; i++)
		{
			if (psLevels->apDataFiles[i] != NULL)
			{
				free(psLevels->apDataFiles[i]);
			}
		}
		psNext = psLevels->psNext;
		free(psLevels);
		psLevels = psNext;
	}
}


// error report function for the level parser
void levError(const char *pError)
{
	char	*pText;
	int		line;

	levGetErrorData(&line, &pText);

#ifdef DEBUG
	ASSERT( FALSE, "Level File parse error:\n%s at line %d text %s\n", pError, line, pText );
#else
	debug( LOG_ERROR, "Level File parse error:\n%s at line %d text %s\n", pError, line, pText );
#endif
}

// find the level dataset
BOOL levFindDataSet(char *pName, LEVEL_DATASET **ppsDataSet)
{
	LEVEL_DATASET	*psNewLevel;

	for(psNewLevel = psLevels; psNewLevel; psNewLevel = psNewLevel->psNext)
	{
		if (psNewLevel->pName != NULL)
		{
			if (strcmp(psNewLevel->pName, pName) == 0)
			{
				*ppsDataSet = psNewLevel;
				return TRUE;
			}
		}
	}

	return FALSE;
}

// parse a level description data file
BOOL levParse(char *pBuffer, SDWORD size, searchPathMode datadir)
{
	SDWORD			token, state, currData=0;
	LEVEL_DATASET	*psDataSet = NULL;
	LEVEL_DATASET	*psFoundData;

	levSetInputBuffer(pBuffer, size);

	state = LP_START;
	token = lev_lex();
	while (token != 0)
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
					levError("Out of memory");
					return FALSE;
				}
				memset(psDataSet, 0, sizeof(LEVEL_DATASET));
				psDataSet->players = 1;
				psDataSet->game = -1;
				psDataSet->dataDir = datadir;
				LIST_ADDEND(psLevels, psDataSet, LEVEL_DATASET);
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
					ASSERT( FALSE,"eh?" );
					break;
				}
			}
			else
			{
				levError("Syntax Error");
				return FALSE;
			}
			state = LP_LEVEL;
			break;
		case LTK_PLAYERS:
			if (state == LP_LEVELDONE &&
				(psDataSet->type == LDS_COMPLETE || psDataSet->type >= MULTI_TYPE_START))
			{
				state = LP_PLAYERS;
			}
			else
			{
				levError("Syntax Error");
				return FALSE;
			}
			break;
		case LTK_TYPE:
			if (state == LP_LEVELDONE && psDataSet->type == LDS_COMPLETE)
			{
				state = LP_TYPE;
			}
			else
			{
				levError("Syntax Error");
				return FALSE;
			}
			break;
		case LTK_INTEGER:
			if (state == LP_PLAYERS)
			{
				psDataSet->players = (SWORD)levVal;
			}
			else if (state == LP_TYPE)
			{
				if (levVal < MULTI_TYPE_START)
				{
					levError("invalid type number");
					return FALSE;
				}

				psDataSet->type = (SWORD)levVal;
			}
			else
			{
				levError("Syntax Error");
				return FALSE;
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
				levError("Syntax Error");
				return FALSE;
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
					levError("Missing dataset command");
					return FALSE;
				}
				state = LP_DATA;
			}
			else
			{
				levError("Syntax Error");
				return FALSE;
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
				levError("Syntax Error");
				return FALSE;
			}
			break;
		case LTK_IDENT:
			if (state == LP_LEVEL)
			{
				if (psDataSet->type == LDS_CAMCHANGE)
				{
					// campaign change dataset - need to find the full data set
					if (!levFindDataSet(pLevToken, &psFoundData))
					{
						levError("Cannot find full data set for camchange");
						return FALSE;
					}

					if (psFoundData->type != LDS_CAMSTART)
					{
						levError("Invalid data set name for cam change");
						return FALSE;
					}
					psFoundData->psChange = psDataSet;
				}
				// store the level name
				psDataSet->pName = strdup(pLevToken);
				if (psDataSet->pName == NULL)
				{
					debug(LOG_ERROR, "levParse: Out of memory!");
					levError("Out of memory");
					abort();
					return FALSE;
				}

				state = LP_LEVELDONE;
			}
			else if (state == LP_DATASET)
			{
				// find the dataset
				if (!levFindDataSet(pLevToken, &psDataSet->psBaseData))
				{
					levError("Unknown dataset");
					return FALSE;
				}
				state = LP_WAITDATA;
			}
			else
			{
				levError("Syntax Error");
				return FALSE;
			}
			break;
		case LTK_STRING:
			if (state == LP_DATA || state == LP_GAME)
			{
				if (currData >= LEVEL_MAXFILES)
				{
					levError("Too many data files");
					return FALSE;
				}

				// note the game index if necessary
				if (state == LP_GAME)
				{
					psDataSet->game = (SWORD)currData;
				}

				// store the data name
				psDataSet->apDataFiles[currData] = strdup(pLevToken);
				if (psDataSet->apDataFiles[currData] == NULL)
				{
					debug(LOG_ERROR, "levParse: Out of memory!");
					levError("Out of memory");
					abort();
					return FALSE;
				}

				resToLower(pLevToken);

				currData += 1;
				state = LP_WAITDATA;
			}
			else
			{
				levError("Syntax Error");
				return FALSE;
			}
			break;
		default:
			levError("Unexpected token");
			break;
		}

		// get the next token
		token = lev_lex();
	}

	if (state != LP_WAITDATA || currData == 0)
	{
		levError("Unexpected end of file");
		return FALSE;
	}

	return TRUE;
}


// free the data for the current mission
BOOL levReleaseMissionData(void)
{
	SDWORD i;

	// release old data if any was loaded
	if (psCurrLevel != NULL)
	{
		if (!stageThreeShutDown())
		{
			return FALSE;
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
						return FALSE;
					}
				}
			}
			else// if (psCurrLevel->apDataFiles[i])
			{

				resReleaseBlockData(i + CURRENT_DATAID);
			}
		}
	}
	return TRUE;
}


// free the currently loaded dataset
BOOL levReleaseAll(void)
{
	SDWORD i;

	// release old data if any was loaded
	if (psCurrLevel != NULL)
	{
		if (!levReleaseMissionData())
		{
			return FALSE;
		}

		// release the game data
		if (psCurrLevel->psBaseData != NULL)
		{
			if (!stageTwoShutDown())
			{
				return FALSE;
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
			return FALSE;
		}
	}

	psCurrLevel=NULL;

	return TRUE;
}

// load up a single wrf file
static BOOL levLoadSingleWRF(char *pName)
{
	// free the old data
	levReleaseAll();

	// create the dummy level data
	memset(&sSingleWRF, 0, sizeof(LEVEL_DATASET));
	sSingleWRF.pName = pName;

	// load up the WRF
	if (!stageOneInitialise())
	{
		return FALSE;
	}

	// load the data
	debug(LOG_WZ, "levLoadSingleWRF: Loading %s ...", pName);
	if (!resLoad(pName, 0))
	{
		return FALSE;
	}

	if (!stageThreeInitialise())
	{
		return FALSE;
	}

	psCurrLevel = &sSingleWRF;

	return TRUE;
}


char *getLevelName( void )
{
	return(currentLevelName);
}


// load up the data for a level
BOOL levLoadData(char *pName, char *pSaveName, SDWORD saveType)
{
	LEVEL_DATASET	*psNewLevel, *psBaseData, *psChangeLevel;
	SDWORD			i;
	BOOL            bCamChangeSaveGame;

	debug(LOG_WZ, "Loading level %s (%s)", pName, pSaveName);

	levelLoadType = saveType;

	// find the level dataset
	if (!levFindDataSet(pName, &psNewLevel))
	{
		debug( LOG_NEVER, "levLoadData: dataset %s not found - trying to load as WRF", pName );
		return levLoadSingleWRF(pName);
	}

	/* Keep a copy of the present level name */
	strlcpy(currentLevelName, pName, sizeof(currentLevelName));

	bCamChangeSaveGame = FALSE;
	if (pSaveName && saveType == GTYPE_SAVE_START)
	{
		if (psNewLevel->psChange != NULL)
		{
			bCamChangeSaveGame = TRUE;
		}
	}

	// select the change dataset if there is one
	psChangeLevel = NULL;
	if (((psNewLevel->psChange != NULL) && (psCurrLevel != NULL)) || bCamChangeSaveGame)
	{
		//store the level name
		debug( LOG_WZ, "levLoadData: Found CAMCHANGE dataset\n" );
		psChangeLevel = psNewLevel;
		psNewLevel = psNewLevel->psChange;
	}

	// ensure the correct dataset is loaded
	if (psNewLevel->type == LDS_CAMPAIGN)
	{
		debug( LOG_ERROR, "levLoadData: Cannot load a campaign dataset (%s)", psNewLevel->pName );
		return FALSE;
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
				debug(LOG_WZ, "levLoadData: Incorrect base dataset loaded - levReleaseAll()");
				levReleaseAll();	// this sets psCurrLevel to NULL
			}
			else
			{
				debug(LOG_WZ, "levLoadData: Correct base dataset already loaded.");
			}
		}

		// setup the correct dataset to load if necessary
		if (psCurrLevel == NULL)
		{
			if (psNewLevel->psBaseData != NULL)
			{
				debug(LOG_WZ, "levLoadData: Setting base dataset to load: %s", psNewLevel->psBaseData->pName);
			}
			psBaseData = psNewLevel->psBaseData;
		}
		else
		{
			debug(LOG_WZ, "levLoadData: No base dataset to load");
			psBaseData = NULL;
		}
	}

	rebuildSearchPath(psNewLevel->dataDir, FALSE);

	// reset the old mission data if necessary
	if (psCurrLevel != NULL)
	{
		debug(LOG_WZ, "levLoadData: reseting old mission data");
		if (!levReleaseMissionData())
		{
			return FALSE;
		}
	}

	// need to free the current map and droids etc for a save game
	if ((psBaseData == NULL) &&
		(pSaveName != NULL))
	{
		if (!saveGameReset())
		{
			return FALSE;
		}
	}

	// initialise if necessary
	if (psNewLevel->type == LDS_COMPLETE || //psNewLevel->type >= MULTI_TYPE_START ||
		psBaseData != NULL)
	{
		debug(LOG_WZ, "levLoadData: Calling stageOneInitialise!");
		if (!stageOneInitialise())
		{
			return FALSE;
		}
	}

	// load up a base dataset if necessary
	if (psBaseData != NULL)
	{
		debug( LOG_NEVER, "levLoadData: loading base dataset %s\n", psBaseData->pName );
		for(i=0; i<LEVEL_MAXFILES; i++)
		{
			if (psBaseData->apDataFiles[i])
			{
				// load the data
				debug(LOG_WZ, "levLoadData: Loading %s ...", psBaseData->apDataFiles[i]);
				if (!resLoad(psBaseData->apDataFiles[i], i))
				{
					return FALSE;
				}
			}
		}
	}
	if (psNewLevel->type == LDS_CAMCHANGE)
	{
		if (!campaignReset())
		{
			return FALSE;
		}
	}
	if (psNewLevel->game == -1)  //no .gam file to load - BETWEEN missions (for Editor games only)
	{
		ASSERT( psNewLevel->type == LDS_BETWEEN,
			"levLoadData: only BETWEEN missions do not need a .gam file" );
		debug( LOG_NEVER, "levLoadData: no .gam file for level: BETWEEN mission\n" );
		if (pSaveName != NULL)
		{
			if (psBaseData != NULL)
			{
				if (!stageTwoInitialise())
				{
					return FALSE;
				}
			}

			//set the mission type before the saveGame data is loaded
			if (saveType == GTYPE_SAVE_MIDMISSION)
			{
				debug( LOG_NEVER, "levLoadData: init mission stuff\n" );
				if (!startMissionSave(psNewLevel->type))
				{
					return FALSE;
				}

				debug( LOG_NEVER, "levLoadData: dataSetSaveFlag\n" );
				dataSetSaveFlag();
			}

			debug( LOG_NEVER, "levLoadData: loading savegame: %s\n", pSaveName );
			if (!loadGame(pSaveName, FALSE, TRUE,TRUE))
			{
				return FALSE;
			}
		}

		if ((pSaveName == NULL) ||
			(saveType == GTYPE_SAVE_START))
		{
			debug( LOG_NEVER, "levLoadData: start mission - no .gam\n" );
			if (!startMission(psNewLevel->type, NULL))
			{
				return FALSE;
			}
		}
	}

	//we need to load up the save game data here for a camchange
	if (bCamChangeSaveGame)
	{
		debug( LOG_NEVER, "levLoadData: no .gam file for level: BETWEEN mission\n" );
		if (pSaveName != NULL)
		{
			if (psBaseData != NULL)
			{
				if (!stageTwoInitialise())
				{
					return FALSE;
				}
			}

			debug( LOG_NEVER, "levLoadData: loading savegame: %s\n", pSaveName );
			if (!loadGame(pSaveName, FALSE, TRUE,TRUE))
			{
				return FALSE;
			}

			if (!campaignReset())
			{
				return FALSE;
			}
		}
	}


	// load the new data
	debug( LOG_NEVER, "levLoadData: loading mission dataset: %s\n", psNewLevel->pName );
	for(i=0; i < LEVEL_MAXFILES; i++)
	{
		if (psNewLevel->game == i)
		{
			// do some more initialising if necessary
			if (psNewLevel->type == LDS_COMPLETE || psNewLevel->type >= MULTI_TYPE_START || (psBaseData != NULL && !bCamChangeSaveGame))
			{
				if (!stageTwoInitialise())
				{
					return FALSE;
				}
			}

			// load a savegame if there is one - but not if already done so
			if (pSaveName != NULL && !bCamChangeSaveGame)
			{
				//set the mission type before the saveGame data is loaded
				if (saveType == GTYPE_SAVE_MIDMISSION)
				{
					debug( LOG_NEVER, "levLoadData: init mission stuff\n" );
					if (!startMissionSave(psNewLevel->type))
					{
						return FALSE;
					}

					debug( LOG_NEVER, "levLoadData: dataSetSaveFlag\n" );
					dataSetSaveFlag();
				}

				debug( LOG_NEVER, "levLoadData: loading save game %s\n", pSaveName );
				if (!loadGame(pSaveName, FALSE, TRUE,TRUE))
				{
					return FALSE;
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
					debug(LOG_WZ, "levLoadData: LDS_COMPLETE / LDS_CAMSTART");
					if (!startMission(LDS_CAMSTART, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;
				case LDS_BETWEEN:
					debug(LOG_WZ, "levLoadData: LDS_BETWEEN");
					if (!startMission(LDS_BETWEEN, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;

				case LDS_MKEEP:
					debug(LOG_WZ, "levLoadData: LDS_MKEEP");
					if (!startMission(LDS_MKEEP, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;
				case LDS_CAMCHANGE:
					debug(LOG_WZ, "levLoadData: LDS_CAMCHANGE");
					if (!startMission(LDS_CAMCHANGE, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;

				case LDS_EXPAND:
					debug(LOG_WZ, "levLoadData: LDS_EXPAND");
					if (!startMission(LDS_EXPAND, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;
				case LDS_EXPAND_LIMBO:
					debug(LOG_WZ, "levLoadData: LDS_LIMBO");
					if (!startMission(LDS_EXPAND_LIMBO, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;

				case LDS_MCLEAR:
					debug(LOG_WZ, "levLoadData: LDS_MCLEAR");
					if (!startMission(LDS_MCLEAR, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;
				case LDS_MKEEP_LIMBO:
					debug(LOG_WZ, "levLoadData: LDS_MKEEP_LIMBO");
					debug( LOG_NEVER, "MKEEP_LIMBO\n" );
					if (!startMission(LDS_MKEEP_LIMBO, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;
				default:
					ASSERT( psNewLevel->type >= MULTI_TYPE_START,
						"levLoadData: Unexpected mission type" );
					debug(LOG_WZ, "levLoadData: default (MULTIPLAYER)");
					if (!startMission(LDS_CAMSTART, psNewLevel->apDataFiles[i]))
					{
						return FALSE;
					}
					break;
				}
			}
		}
		else if (psNewLevel->apDataFiles[i])
		{
			// load the data
			debug(LOG_WZ, "levLoadData: Loading %s", psNewLevel->apDataFiles[i]);
			if (!resLoad(psNewLevel->apDataFiles[i], i + CURRENT_DATAID))
			{
				return FALSE;
			}
		}
	}

	dataClearSaveFlag();

	if (pSaveName != NULL)
	{
		//load MidMission Extras
		if (!loadMissionExtras(pSaveName, psNewLevel->type))
		{
			return FALSE;
		}
	}

	if (pSaveName != NULL && saveType == GTYPE_SAVE_MIDMISSION)
	{
		//load script stuff
		// load the event system state here for a save game
		debug( LOG_NEVER, "levLoadData: loading script system state\n" );
		if (!loadScriptState(pSaveName))
		{
			return FALSE;
		}
	}

	if (!stageThreeInitialise())
	{
		return FALSE;
	}

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


	return TRUE;
}
