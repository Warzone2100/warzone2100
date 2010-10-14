/*
	This file is part of Warzone 2100.
	Copyright (C) 2006-2010  Warzone 2100 Project

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

#include "lib/framework/frame.h"

#include "aiexperience.h"
#include "console.h"
#include "map.h"

#define	SAVE_FORMAT_VERSION			2

#define MAX_OIL_ENTRIES				600		//(Max number of derricks or oil resources) / 2
#define	MAX_OIL_LOCATIONS			300		//max number of oil locations to store

#define SAME_LOC_RANGE				8		//if within this range, consider it the same loc

//Return values of experience-loading routine
#define EXPERIENCE_LOAD_OK			0			//no problemens encountered
#define EXPERIENCE_LOAD_ERROR		1			//error while loading experience
#define EXPERIENCE_LOAD_NOSAVE		(-1)		//no experience exists

static PHYSFS_file* aiSaveFile[8];

SDWORD baseLocation[MAX_PLAYERS][MAX_PLAYERS][2];		//each player's visible enemy base x,y coords for each player (hence 3d)
static SDWORD oilLocation[MAX_PLAYERS][MAX_OIL_LOCATIONS][2];	//remembered oil locations

SDWORD baseDefendLocation[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS][2];
SDWORD baseDefendLocPrior[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS];		//Priority

SDWORD oilDefendLocation[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS][2];
SDWORD oilDefendLocPrior[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS];

void InitializeAIExperience(void)
{
	SDWORD i,j;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		for(j=0;j<MAX_PLAYERS;j++)
		{
			baseLocation[i][j][0] = -1;
			baseLocation[i][j][1] = -1;
		}

		for(j=0; j < MAX_BASE_DEFEND_LOCATIONS; j++)
		{
			baseDefendLocation[i][j][0] = -1;
			baseDefendLocation[i][j][1] = -1;

			baseDefendLocPrior[i][j] = -1;
		}

		for(j=0; j < MAX_OIL_DEFEND_LOCATIONS; j++)
		{
			oilDefendLocation[i][j][0] = -1;
			oilDefendLocation[i][j][1] = -1;

			oilDefendLocPrior[i][j] = -1;
		}

		//oil locations
		for(j=0; j < MAX_OIL_LOCATIONS; j++)
		{
			oilLocation[i][j][0] = -1;
			oilLocation[i][j][1] = -1;
		}
	}
}

void LoadAIExperience(BOOL bNotify)
{
	SDWORD player,status;

	for(player=0; player<game.maxPlayers; player++)
	{
		status = LoadPlayerAIExperience(player);

		//notify if needed
		if(bNotify)
		{
			if(status == EXPERIENCE_LOAD_NOSAVE)
			{
				console("No saved experience found for player %d", player);
			}
			else if(status == EXPERIENCE_LOAD_OK)
			{
				console("Successfully loaded experience for player %d", player);
			}
			else	//any error
			{
				console("Error while loading experience for player %d", player);
			}
		}
	}
}

BOOL SaveAIExperience(BOOL bNotify)
{
	SDWORD i;
	for(i=0;i<game.maxPlayers;i++)
	{
		(void)SavePlayerAIExperience(i, bNotify);
	}

	return true;
}

static bool SetUpInputFile(const unsigned int nPlayer)
{
	char* FileName;

	/* Create filename */
	sasprintf(&FileName, "multiplay/learndata/player%u/%s.lrn", nPlayer, game.map);

	aiSaveFile[nPlayer] = PHYSFS_openRead(FileName);

	if (aiSaveFile[nPlayer] == NULL)
	{
		debug(LOG_ERROR, "Couldn't open input file: [directory: %s] '%s' for player %u: %s", PHYSFS_getRealDir(FileName), FileName, nPlayer, PHYSFS_getLastError());
		return false;
	}

	return true;
}

static int ReadAISaveData(SDWORD nPlayer)
{
	FEATURE		*psFeature;
	SDWORD		NumEntries=0;		//How many derricks/oil resources will be saved
	UDWORD		PosXY[MAX_OIL_ENTRIES];		//Locations, 0=x,1=y,2=x etc
	UDWORD		i,j;
	BOOL		Found,bTileVisible;
	UDWORD		version;

	if(!SetUpInputFile(nPlayer))
	{
		//debug(LOG_ERROR,"No experience data loaded for %d",nPlayer);
		return EXPERIENCE_LOAD_NOSAVE;
	}
	else
	{
		/* Read data version */
		if (PHYSFS_read(aiSaveFile[nPlayer], &version, sizeof(NumEntries), 1 ) != 1 )
		{
			debug(LOG_ERROR, "Failed to read version number for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		// Check version, assume backward compatibility
		if(version > SAVE_FORMAT_VERSION)
		{
			debug(LOG_ERROR, "Incompatible version of the learn data (%d, expected %d) for player '%d'", version, SAVE_FORMAT_VERSION, nPlayer);
		}

		/************************/
		/*		Enemy bases		*/
		/************************/

		/* read max number of players (usually 8) */
		if (PHYSFS_read(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1 ) != 1 )
		{
			debug(LOG_ERROR, "Failed to read number of players for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/* read base locations of all players */
		if (PHYSFS_read(aiSaveFile[nPlayer], baseLocation[nPlayer], sizeof(SDWORD), NumEntries * 2 ) != (NumEntries * 2) )
		{
			debug(LOG_ERROR, "Failed to load baseLocation for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/************************************/
		/*		Base attack locations		*/
		/************************************/

		/* read MAX_BASE_DEFEND_LOCATIONS */
		if (PHYSFS_read(aiSaveFile[nPlayer], &NumEntries, sizeof(SDWORD), 1 ) != 1 )
		{
			debug(LOG_ERROR, "Failed to read MAX_BASE_DEFEND_LOCATIONS for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/* check it's the same as current MAX_BASE_DEFEND_LOCATIONS */
		if(NumEntries > MAX_BASE_DEFEND_LOCATIONS)
		{
			debug(LOG_ERROR, "saved MAX_BASE_DEFEND_LOCATIONS and current one don't match (%d / %d)", NumEntries, MAX_BASE_DEFEND_LOCATIONS);
			return EXPERIENCE_LOAD_ERROR;
		}

		//debug(LOG_ERROR,"num base attack loc: %d", NumEntries);

		/* read base defence locations */
		if (PHYSFS_read(aiSaveFile[nPlayer], baseDefendLocation[nPlayer], sizeof(SDWORD), NumEntries * 2 ) != (NumEntries * 2) )
		{
			debug(LOG_ERROR, "Failed to load baseDefendLocation for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/* read base defend priorities */
		if (PHYSFS_read(aiSaveFile[nPlayer], baseDefendLocPrior[nPlayer], sizeof(SDWORD), NumEntries * 2 ) != (NumEntries * 2) )
		{
			debug(LOG_ERROR, "Failed to load baseDefendLocPrior for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/************************************/
		/*		Oil attack locations		*/
		/************************************/

		/* read MAX_OIL_DEFEND_LOCATIONS */
		if (PHYSFS_read(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1 ) != 1 )
		{
			debug(LOG_ERROR, "Failed to read max number of oil attack locations for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/* check it's the same as current MAX_OIL_DEFEND_LOCATIONS */
		if(NumEntries > MAX_OIL_DEFEND_LOCATIONS)
		{
			debug(LOG_ERROR, "saved MAX_OIL_DEFEND_LOCATIONS and current one don't match (%d / %d)", NumEntries, MAX_OIL_DEFEND_LOCATIONS);
			return EXPERIENCE_LOAD_ERROR;
		}

		//debug(LOG_ERROR,"num oil attack loc: %d", NumEntries);

		/* read oil locations */
		if (PHYSFS_read(aiSaveFile[nPlayer], oilDefendLocation[nPlayer], sizeof(SDWORD), NumEntries * 2 ) != (NumEntries * 2) )
		{
			debug(LOG_ERROR, "Failed to load oilDefendLocation for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/* read oil location priority */
		if (PHYSFS_read(aiSaveFile[nPlayer], oilDefendLocPrior[nPlayer], sizeof(SDWORD), NumEntries * 2 ) != (NumEntries * 2) )
		{
			debug(LOG_ERROR, "Failed to load oilDefendLocPrior for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/****************************/
		/*		Oil Resources		*/
		/****************************/

		/* read MAX_OIL_LOCATIONS */
		if (PHYSFS_read(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1 ) != 1 )
		{
			debug(LOG_ERROR, "Failed to load MAX_OIL_LOCATIONS for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		/* check it's the same as current MAX_OIL_LOCATIONS */
		if(NumEntries > MAX_OIL_LOCATIONS)
		{
			debug(LOG_ERROR, "saved MAX_OIL_LOCATIONS and current one don't match (%d / %d)", NumEntries, MAX_OIL_LOCATIONS);
			return EXPERIENCE_LOAD_ERROR;
		}


		/* Read number of Oil Resources */
		if (PHYSFS_read(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1 ) != 1 )
		{
			debug(LOG_ERROR, "Failed to read Oil Resources count for player '%d'",nPlayer);
			return EXPERIENCE_LOAD_ERROR;
		}

		//debug(LOG_ERROR,"Num oil: %d", NumEntries);

		if(NumEntries > 0)	//any oil resources were saved?
		{
			/* Read Oil Resources coordinates */
			if (PHYSFS_read(aiSaveFile[nPlayer], PosXY, sizeof(UDWORD), NumEntries * 2 ) != (NumEntries * 2) )
			{
				debug(LOG_ERROR, "Failed to read Oil Resources coordinates for player '%d'",nPlayer);
				return EXPERIENCE_LOAD_ERROR;
			}

			for(i=0; i<NumEntries; i++)
			{
				Found = false;

				//re-read into remory
				if(i < MAX_OIL_LOCATIONS)	//didn't max out?
				{
					oilLocation[nPlayer][i][0] = PosXY[i * 2];		//x
					oilLocation[nPlayer][i][1] = PosXY[i * 2 + 1];	//y
				}

				/* Iterate through all Oil Resources and try to match coordinates */
				for(psFeature = apsFeatureLists[0]; psFeature != NULL; psFeature = psFeature->psNext)
				{
					if(psFeature->psStats->subType == FEAT_OIL_RESOURCE)	//Oil resource
					{
						if (!(psFeature->visible[nPlayer]))		//Not visible yet
						{
							if((PosXY[i * 2] == psFeature->pos.x) && (PosXY[i * 2 + 1] == psFeature->pos.y))	/* Found it */
							{
								//debug_console("Matched oil resource at x: %d y: %d", PosXY[i * 2]/128,PosXY[i * 2 + 1]/128);

								psFeature->visible[nPlayer] = true;		//Make visible for AI
								Found = true;
								break;
							}
						}

					}
				}

				//if(!Found)		//Couldn't find oil resource with this coords on the map
				//	debug_console("!!Failed to match oil resource #%d at x: %d y: %d", i,PosXY[i * 2]/128,PosXY[i * 2 + 1]/128);
			}

			/************************/
			/*		Fog of War		*/
			/************************/
			if(version >= 2)
			{
				for(i=0;i<mapWidth;i++)
				{
					for(j=0;j<mapWidth;j++)
					{
						if (PHYSFS_read(aiSaveFile[nPlayer], &bTileVisible, sizeof(BOOL), 1 ) != 1 )
						{
							debug(LOG_ERROR, "Failed to load tile visibility at tile %d-%d for player '%d'", i, j, nPlayer);
							return EXPERIENCE_LOAD_ERROR;
						}

						// Restore tile visibility
						if(bTileVisible)
						{
							SET_TILE_VISIBLE(selectedPlayer,mapTile(i,j));
						}
					}
				}
			}
		}
	}

	return PHYSFS_close(aiSaveFile[nPlayer]) ? EXPERIENCE_LOAD_OK : EXPERIENCE_LOAD_ERROR;
}

SDWORD LoadPlayerAIExperience(SDWORD nPlayer)
{
	if((nPlayer > -1) && (nPlayer < MAX_PLAYERS))
	{
		return ReadAISaveData(nPlayer);
	}

	return EXPERIENCE_LOAD_ERROR;
}

static bool SetUpOutputFile(unsigned int nPlayer)
{
	char* SaveDir;
	char* FileName;

	/* Assemble dir string */
	sasprintf(&SaveDir, "multiplay/learndata/player%u", nPlayer);

	/* Create dir on disk */
	if (!PHYSFS_mkdir(SaveDir))
	{
		debug(LOG_ERROR, "Failed to create directory \"%s\": %s", SaveDir, PHYSFS_getLastError() );
		return false;
	}

	/* Create filename */
	sasprintf(&FileName, "%s/%s.lrn", SaveDir, game.map);

	/* Open file */
	aiSaveFile[nPlayer] = PHYSFS_openWrite(FileName);
	if (aiSaveFile[nPlayer] == NULL)
	{
		debug(LOG_ERROR, "Couldn't open debugging output file: '%s' for player %u", FileName, nPlayer);
		return false;
	}

	return true;
}

static bool canRecallOilAt(SDWORD nPlayer, SDWORD x, SDWORD y)
{
	unsigned int i;

	/* go through all remembered oil and check */
	for(i = 0; i < MAX_OIL_LOCATIONS; ++i)
	{
		if (oilLocation[nPlayer][i][0] != x)
			continue;

		if (oilLocation[nPlayer][i][1] != y)
			continue;

		return true;		//yep, both matched
	}

	return false;			//no
}

static bool WriteAISaveData(SDWORD nPlayer)
{

	FEATURE					*psFeature;
	STRUCTURE				*psCurr;
	SDWORD					x=0,y=0;
	SDWORD					NumEntries=0;	//How many derricks/oil resources will be saved
	UDWORD					PosXY[MAX_OIL_ENTRIES];		//Locations, 0=x,1=y,2=x etc
	UDWORD					i,j;
	BOOL					bTileVisible;

	/* prepare experience file for the current map */
	if(!SetUpOutputFile(nPlayer))
	{
		debug(LOG_ERROR, "Failed to prepare experience file for player %d",nPlayer);
		return false;
	}

	if (aiSaveFile[nPlayer])
	{
		//debug(LOG_ERROR, "aiSaveFile ok");


		/* Version */
		NumEntries = SAVE_FORMAT_VERSION;		//Version
		if(PHYSFS_write(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1) != 1)
		{
			debug(LOG_ERROR, "failed to write version for player %d",nPlayer);
			return false;
		}

		//fwrite(&NumEntries,sizeof(NumEntries),1,aiSaveFile[nPlayer]);	//Version

		//debug(LOG_ERROR, "Version ok");

		/************************/
		/*		Enemy bases		*/
		/************************/
		NumEntries = MAX_PLAYERS;

		/* max num of players to store */
		if(PHYSFS_write(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1) != 1)			//num of players to store
		{
			debug(LOG_ERROR, "failed to write MAX_PLAYERS for player %d",nPlayer);
			return false;
		}

		//debug(LOG_ERROR, "MAX_PLAYERS ok");

		/* base locations of all players */
		if(PHYSFS_write(aiSaveFile[nPlayer], baseLocation[nPlayer], sizeof(SDWORD), MAX_PLAYERS * 2) != (MAX_PLAYERS * 2))			//num of players to store
		{
			debug(LOG_ERROR, "failed to write base locations for player %d",nPlayer);
			return false;
		}

		//debug(LOG_ERROR, "Enemy bases ok");

		/************************************/
		/*		Base attack locations		*/
		/************************************/
		NumEntries = MAX_BASE_DEFEND_LOCATIONS;

		/* write MAX_BASE_DEFEND_LOCATIONS */
		if(PHYSFS_write(aiSaveFile[nPlayer], &NumEntries, sizeof(SDWORD), 1) < 1)
		{
			debug(LOG_ERROR, "failed to write defence locations count for player %d",nPlayer);
			return false;
		}

		/* base defence locations */
		if(PHYSFS_write(aiSaveFile[nPlayer], baseDefendLocation[nPlayer], sizeof(SDWORD), MAX_BASE_DEFEND_LOCATIONS * 2) < (MAX_BASE_DEFEND_LOCATIONS * 2))
		{
			debug(LOG_ERROR, "failed to write defence locations for player %d",nPlayer);
			return false;
		}

		/* base defend priorities */
		if(PHYSFS_write(aiSaveFile[nPlayer], baseDefendLocPrior[nPlayer], sizeof(SDWORD), MAX_BASE_DEFEND_LOCATIONS * 2) < (MAX_BASE_DEFEND_LOCATIONS * 2))
		{
			debug(LOG_ERROR, "failed to write defence locations priority for player %d",nPlayer);
			return false;
		}

		//debug(LOG_ERROR, "Base attack locations ok");

		/************************************/
		/*		Oil attack locations		*/
		/************************************/
		NumEntries = MAX_OIL_DEFEND_LOCATIONS;

		/* MAX_OIL_DEFEND_LOCATIONS */
		if(PHYSFS_write(aiSaveFile[nPlayer], &NumEntries, sizeof(SDWORD), 1) < 1)
		{
			debug(LOG_ERROR, "failed to write oil defence locations count for player %d",nPlayer);
			return false;
		}

		/* oil locations */
		if(PHYSFS_write(aiSaveFile[nPlayer], oilDefendLocation[nPlayer], sizeof(SDWORD), MAX_OIL_DEFEND_LOCATIONS * 2) < (MAX_OIL_DEFEND_LOCATIONS * 2))
		{
			debug(LOG_ERROR, "failed to write oil defence locations for player %d",nPlayer);
			return false;
		}

		/* oil location priority */
		if(PHYSFS_write(aiSaveFile[nPlayer], oilDefendLocPrior[nPlayer], sizeof(SDWORD), MAX_OIL_DEFEND_LOCATIONS * 2) < (MAX_OIL_DEFEND_LOCATIONS * 2))
		{
			debug(LOG_ERROR, "failed to write oil defence locations priority for player %d",nPlayer);
			return false;
		}

		//debug(LOG_ERROR, "Oil attack locations ok");


		/****************************/
		/*		Oil Resources		*/
		/****************************/
		NumEntries = MAX_OIL_LOCATIONS;

		/* MAX_OIL_LOCATIONS */
		if(PHYSFS_write(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1) < 1)
		{
			debug(LOG_ERROR, "failed to write oil locations count for player %d",nPlayer);
			return false;
		}

		NumEntries = 0;		//Num of oil resources

		/* first save everything what we recalled from last load */
		for(i=0; i<MAX_OIL_LOCATIONS; i++)
		{
			if(oilLocation[nPlayer][i][0] <= 0 || oilLocation[nPlayer][i][1] <= 0)
				continue;		//skip to next one, since nothing stored

			PosXY[NumEntries * 2] = oilLocation[nPlayer][i][0];		//x
			PosXY[NumEntries * 2 + 1] = oilLocation[nPlayer][i][1];	//y

			NumEntries++;
		}

		/* now remember new ones that are not in memory yet (discovered this time) */
		for(psFeature = apsFeatureLists[0]; psFeature != NULL; psFeature = psFeature->psNext)
		{
			if(psFeature->psStats->subType == FEAT_OIL_RESOURCE)
			{
				if (psFeature->visible[nPlayer])	//|| godMode)
				{
					if(!canRecallOilAt(nPlayer, psFeature->pos.x, psFeature->pos.y))	//already stored?
					{
						/* Save X */
						PosXY[NumEntries * 2] = psFeature->pos.x;

						/* Save Y */
						PosXY[NumEntries * 2 + 1] = psFeature->pos.y;

						//debug_console("New oil visible x: %d y: %d. Storing.", PosXY[NumEntries * 2]/128,PosXY[NumEntries * 2 + 1]/128);

						NumEntries++;
					}
				}
			}
		}

		//Save Derricks as oil resources, since most of them will be unoccupied when experiance will be loaded
		for(i=0;i<MAX_PLAYERS;i=i+1)
		{
			for (psCurr = apsStructLists[i]; psCurr != NULL; psCurr = psCurr->psNext)
			{
				if (psCurr->pStructureType->type == REF_RESOURCE_EXTRACTOR)
				{
					if(psCurr->visible[nPlayer])	//if can see it
					{
						if(!canRecallOilAt(nPlayer, psCurr->pos.x, psCurr->pos.y))	//already stored?
						{
							//psResExtractor = (RES_EXTRACTOR *)psCurr->pFunctionality;

							x = psCurr->pos.x;
							y = psCurr->pos.y;

							//debug_console("Found derrick at x: %d, y: %d,, width: %d",psCurr->pos.x/128,psCurr->pos.y/128,mapWidth);

							// Save X //
							PosXY[NumEntries * 2] = psCurr->pos.x;

							// Save Y //
							PosXY[NumEntries * 2 + 1] = psCurr->pos.y;

							//debug_console("New derrick visible x: %d y: %d. Storing.", PosXY[NumEntries * 2]/128,PosXY[NumEntries * 2 + 1]/128);

							NumEntries++;
						}
					}
				}
			}
		}


		/* Write number of Oil Resources */
		if(PHYSFS_write(aiSaveFile[nPlayer], &NumEntries, sizeof(NumEntries), 1) < 1)
		{
			debug(LOG_ERROR, "failed to write stored oil locations count for player %d",nPlayer);
			return false;
		}

		//debug_console("Num Oil Resources: %d ****",NumEntries);

		/* Write Oil Resources coords */
		if(NumEntries > 0)		//Anything to save
		{
			if(PHYSFS_write(aiSaveFile[nPlayer], PosXY, sizeof(UDWORD), NumEntries * 2) < (NumEntries * 2))
			{
				debug(LOG_ERROR, "failed to write oil locations fir player %d",nPlayer);
				return false;
			}
		}

		/************************/
		/*		Fog of War		*/
		/************************/
		NumEntries = MAX_OIL_DEFEND_LOCATIONS;

		for(i=0;i<mapWidth;i++)
		{
			for(j=0;j<mapHeight;j++)
			{
				/* Write tile visibility */
				bTileVisible = TEST_TILE_VISIBLE(nPlayer, mapTile(i, j));
				if(PHYSFS_write(aiSaveFile[nPlayer], &bTileVisible, sizeof(BOOL), 1) < 1)
				{
					debug(LOG_ERROR, "failed to write fog of war at tile %d-%d for player %d", i, j, nPlayer);
					return false;
				}
			}
		}
	}
	else
	{
		debug(LOG_ERROR, "no output file for player %d",nPlayer);
		return false;
	}

	//debug_console("AI settings file written for player %d",nPlayer);

	return PHYSFS_close(aiSaveFile[nPlayer]);
}

BOOL SavePlayerAIExperience(SDWORD nPlayer, BOOL bNotify)
{
	if((nPlayer > -1) && (nPlayer < MAX_PLAYERS))
	{
		if(!WriteAISaveData(nPlayer))
		{
			debug(LOG_ERROR,"SavePlayerAIExperience - failed to save exper");

			//addConsoleMessage("Failed to save experience.",RIGHT_JUSTIFY,SYSTEM_MESSAGE);
			console("Failed to save experience for player %d.", nPlayer);
			return false;
		}
	}

	if(bNotify)
	{
		console("Experience for player %d saved successfully.", nPlayer);
	}

	return true;
}


//sort the priorities, placing the higher ones at the top
static bool SortBaseDefendLoc(SDWORD nPlayer)
{
	SDWORD i, prior, temp,LowestPrior,LowestIndex,SortBound;



	SortBound = MAX_BASE_DEFEND_LOCATIONS-1;	//nothing sorted yet, point at last elem

	while(SortBound >= 0)		//while didn't reach the top
	{
		LowestPrior = (SDWORD_MAX - 1);

		LowestIndex = -1;

		//find lowest element
		for(i=0; i <= SortBound; i++)
		{
			prior = baseDefendLocPrior[nPlayer][i];
			if(prior < LowestPrior)	//lower and isn't a flag meaning this one wasn't initialized with anything
			{
				LowestPrior = prior;
				LowestIndex = i;
			}
		}

		//huh, nothing found? (probably nothing set yet, no experience)
		if(LowestIndex < 0)
		{
			//debug(LOG_ERROR,"sortBaseDefendLoc() - No lowest elem found");
			return true;
		}

		//swap
		if(LowestPrior < baseDefendLocPrior[nPlayer][SortBound])	//need to swap? (might be the same elem)
		{
			//priority
			temp = baseDefendLocPrior[nPlayer][SortBound];
			baseDefendLocPrior[nPlayer][SortBound] = baseDefendLocPrior[nPlayer][LowestIndex];
			baseDefendLocPrior[nPlayer][LowestIndex] = temp;

			//x
			temp = baseDefendLocation[nPlayer][SortBound][0];
			baseDefendLocation[nPlayer][SortBound][0] = baseDefendLocation[nPlayer][LowestIndex][0];
			baseDefendLocation[nPlayer][LowestIndex][0] = temp;

			//y
			temp = baseDefendLocation[nPlayer][SortBound][1];
			baseDefendLocation[nPlayer][SortBound][1] = baseDefendLocation[nPlayer][LowestIndex][1];
			baseDefendLocation[nPlayer][LowestIndex][1] = temp;
		}

		SortBound--;		//in any case lower the boundry, even if didn't swap
	}

	return true;
}

//x and y are passed by script, find out if this loc is close to
//an already stored loc, if yes then increase its priority
BOOL StoreBaseDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer)
{
	SDWORD	i, index;
	BOOL	found;

	index = GetBaseDefendLocIndex(x, y, nPlayer);
	if(index < 0)			//this one is new
	{
		//find an empty element
		found = false;
		for(i=0; i < MAX_BASE_DEFEND_LOCATIONS; i++)
		{
			if(baseDefendLocation[nPlayer][i][0] < 0)	//not initialized yet
			{
				//addConsoleMessage("Base defense location - NEW LOCATION.", RIGHT_JUSTIFY,SYSTEM_MESSAGE);

				baseDefendLocation[nPlayer][i][0] = x;
				baseDefendLocation[nPlayer][i][1] = y;

				baseDefendLocPrior[nPlayer][i] = 1;

				found = true;

				return true;
			}
		}

		addConsoleMessage("Base defense location - NO SPACE LEFT.",RIGHT_JUSTIFY,SYSTEM_MESSAGE);
		return false;		//not enough space to store
	}
	else		//this one already stored
	{
		//addConsoleMessage("Base defense location - INCREASED PRIORITY.",SYSTEM_MESSAGE);

		baseDefendLocPrior[nPlayer][index]++;	//higher the priority

		if(baseDefendLocPrior[nPlayer][index] == SDWORD_MAX)
			baseDefendLocPrior[nPlayer][index] = 1;			//start all over

		SortBaseDefendLoc(nPlayer);				//now sort everything
	}

	return true;
}

SDWORD GetBaseDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer)
{
	UDWORD	i,range;

	range  = world_coord(SAME_LOC_RANGE);		//in world units

	for(i=0; i < MAX_BASE_DEFEND_LOCATIONS; i++)
	{
		if((baseDefendLocation[nPlayer][i][0] > 0) && (baseDefendLocation[nPlayer][i][1] > 0))		//if this one initialized
		{
			//check if very close to an already stored location
			if (hypotf(x - baseDefendLocation[nPlayer][i][0], y - baseDefendLocation[nPlayer][i][1]) < range)
			{
				return i;								//end here
			}
		}
	}

	return -1;
}

void BaseExperienceDebug(SDWORD nPlayer)
{
	SDWORD i;

	debug_console("-------------");
	for(i=0; i< MAX_BASE_DEFEND_LOCATIONS; i++)
	{
		debug_console("%d) %d - %d (%d)", i, map_coord(baseDefendLocation[nPlayer][i][0]), map_coord(baseDefendLocation[nPlayer][i][1]), baseDefendLocPrior[nPlayer][i]);
	}
	debug_console("-------------");
}

void OilExperienceDebug(SDWORD nPlayer)
{
	unsigned int i;

	debug_console("-------------");

	for(i = 0; i < MAX_OIL_DEFEND_LOCATIONS; ++i)
	{
		debug_console("%u) %d - %d (%d)", i, map_coord(oilDefendLocation[nPlayer][i][0]), map_coord(oilDefendLocation[nPlayer][i][1]), oilDefendLocPrior[nPlayer][i]);
	}

	debug_console("-------------");
}


//sort the priorities, placing the higher ones at the top
static bool SortOilDefendLoc(SDWORD nPlayer)
{
	int SortBound;

	//while didn't reach the top
	for (SortBound = MAX_OIL_DEFEND_LOCATIONS - 1; SortBound >= 0; --SortBound)
	{
		int LowestPrior = SDWORD_MAX - 1,
		    LowestIndex = -1;
		int i;

		//find lowest element
		for (i = 0; i <= SortBound; ++i)
		{
			const int prior = oilDefendLocPrior[nPlayer][i];
			if (prior < LowestPrior)	//lower and isn't a flag meaning this one wasn't initialized with anything
			{
				LowestPrior = prior;
				LowestIndex = i;
			}
		}

		//huh, nothing found? (probably nothing set yet, no experience)
		if (LowestIndex < 0)
		{
			//debug(LOG_ERROR,"sortBaseDefendLoc() - No lowest elem found");
			return true;
		}

		//swap
		if (LowestPrior < oilDefendLocPrior[nPlayer][SortBound])	//need to swap? (might be the same elem)
		{
			int temp;

			//priority
			temp = oilDefendLocPrior[nPlayer][SortBound];
			oilDefendLocPrior[nPlayer][SortBound] = oilDefendLocPrior[nPlayer][LowestIndex];
			oilDefendLocPrior[nPlayer][LowestIndex] = temp;

			//x
			temp = oilDefendLocation[nPlayer][SortBound][0];
			oilDefendLocation[nPlayer][SortBound][0] = oilDefendLocation[nPlayer][LowestIndex][0];
			oilDefendLocation[nPlayer][LowestIndex][0] = temp;

			//y
			temp = oilDefendLocation[nPlayer][SortBound][1];
			oilDefendLocation[nPlayer][SortBound][1] = oilDefendLocation[nPlayer][LowestIndex][1];
			oilDefendLocation[nPlayer][LowestIndex][1] = temp;
		}
	}

	return true;
}

//x and y are passed by script, find out if this loc is close to
//an already stored loc, if yes then increase its priority
BOOL StoreOilDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer)
{
	SDWORD	i, index;
	BOOL	found;

	index = GetOilDefendLocIndex(x, y, nPlayer);
	if(index < 0)			//this one is new
	{
		//find an empty element
		found = false;
		for(i=0; i < MAX_OIL_DEFEND_LOCATIONS; i++)
		{
			if(oilDefendLocation[nPlayer][i][0] < 0)	//not initialized yet
			{
				//addConsoleMessage("Oil defense location - NEW LOCATION.", SYSTEM_MESSAGE);

				oilDefendLocation[nPlayer][i][0] = x;
				oilDefendLocation[nPlayer][i][1] = y;

				oilDefendLocPrior[nPlayer][i] = 1;

				found = true;

				return true;
			}
		}

		addConsoleMessage("Oil defense location - NO SPACE LEFT.",RIGHT_JUSTIFY,SYSTEM_MESSAGE);
		return false;		//not enough space to store
	}
	else		//this one already stored
	{
		//addConsoleMessage("Oil defense location - INCREASED PRIORITY.",SYSTEM_MESSAGE);

		oilDefendLocPrior[nPlayer][index]++;	//higher the priority

		if(oilDefendLocPrior[nPlayer][index] == SDWORD_MAX)
			oilDefendLocPrior[nPlayer][index] = 1;			//start all over

		SortOilDefendLoc(nPlayer);				//now sort everything
	}

	return true;
}

int GetOilDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer)
{
	const unsigned int range = world_coord(SAME_LOC_RANGE);		//in world units
	int i;

	for(i = 0; i < MAX_OIL_DEFEND_LOCATIONS; ++i)
	{
		if (oilDefendLocation[nPlayer][i][0] > 0
		 && oilDefendLocation[nPlayer][i][1] > 0)		//if this one initialized
		{
			//check if very close to an already stored location
			if (hypotf(x - oilDefendLocation[nPlayer][i][0], y - oilDefendLocation[nPlayer][i][1]) < range)
			{
				return i;
			}
		}
	}

	return -1;
}

bool CanRememberPlayerBaseLoc(SDWORD lookingPlayer, SDWORD enemyPlayer)
{
	return lookingPlayer >= 0
	    && enemyPlayer >= 0
	    && lookingPlayer < MAX_PLAYERS
	    && enemyPlayer < MAX_PLAYERS
	    && baseLocation[lookingPlayer][enemyPlayer][0] > 0
	    && baseLocation[lookingPlayer][enemyPlayer][1] > 0;
}

bool CanRememberPlayerBaseDefenseLoc(SDWORD player, SDWORD index)
{
	return player >= 0
	    && player < MAX_PLAYERS
	    && index >= 0
	    && index < MAX_BASE_DEFEND_LOCATIONS
	    && baseDefendLocation[player][index][0] > 0
	    && baseDefendLocation[player][index][1] > 0;
}

bool CanRememberPlayerOilDefenseLoc(SDWORD player, SDWORD index)
{
	return player >= 0
	    && player < MAX_PLAYERS
	    && index >= 0
	    && index < MAX_BASE_DEFEND_LOCATIONS
	    && oilDefendLocation[player][index][0] > 0
	    && oilDefendLocation[player][index][1] > 0;
}
