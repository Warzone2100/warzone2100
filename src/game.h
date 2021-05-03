/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** @file
 *
 *  load and save game routines.
 *  Very likely to ALL change! All the headers are separately defined at the
 *  moment - they probably don't need to be - if no difference make into one.
 *  Also the struct definitions throughout the game could be re-ordered to contain
 *  the variables required for saving so that don't need to create a load more here!
 */

#ifndef __INCLUDED_SRC_GAME_H__
#define __INCLUDED_SRC_GAME_H__

#include "lib/framework/vector.h"
#include "gamedef.h"
#include "levels.h"

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/

bool loadGame(const char *pGameToLoad, bool keepObjects, bool freeMem, bool UserSaveGame);	// UserSaveGame is true when the save game is not a new level (User Save Game)

/*This just loads up the .gam file to determine which level data to set up - split up
so can be called in levLoadData when starting a game from a load save game*/
bool loadGameInit(const char *fileName);

bool loadMissionExtras(const char* pGameToLoad, LEVEL_TYPE levelType);

// load the script state given a .gam name
bool loadScriptState(char *pFileName);

/// Load the terrain types
bool loadTerrainTypeMap(const char *pFilePath);
bool loadTerrainTypeMapOverride(unsigned int tileSet);

bool saveGame(const char *aFileName, GAME_TYPE saveType);

// Get the campaign number for loadGameInit game
UDWORD getCampaign(const char *fileName);

/*returns the current type of save game being loaded*/
GAME_TYPE getSaveGameType();

void gameScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight);
void gameDisplayScaleFactorDidChange(float newDisplayScaleFactor);

#endif // __INCLUDED_SRC_GAME_H__
