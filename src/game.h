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
/** @file
 *
 *  load and save game routines.
 *  Very likely to ALL change! All the headers are separately defined at the
 *  moment - they probably don't need to be - if no difference make into one.
 *  Also the struct defintions throughout the game could be re-ordered to contain
 *  the variables required for saving so that don't need to create a load more here!
 */

#ifndef __INCLUDED_SRC_GAME_H__
#define __INCLUDED_SRC_GAME_H__

#include "lib/framework/vector.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
//the version number used in save games
//#define VERSION_1				1				//demo save games
//#define VERSION_2				2				//names saved for components/structures/features
//#define VERSION_3				3				//changes to SAVE_GAME
//#define VERSION_4				4				//changes to SAVE_GAME
//#define VERSION_5				5				//different types of save game added
//#define VERSION_6				6				//level name added to a user save game
#define VERSION_7				7				//string ID names saved for comps an objects
#define VERSION_8				8				//research status saved out for user saved games
#define VERSION_9				9				//power and experience saved out for user saved games
#define VERSION_10				10				//includes gateways and zones in .map file.
#define VERSION_11				11				//newstyle save game with extending structure checked in 13 Nov.
#define VERSION_12				12				//mission and order stuff checked in 20 Nov.
//#define VERSION_13				13				//odds and ends to 24 Nov. and hashed scripts
#define VERSION_14				14				//
#define VERSION_15				15				//
#define VERSION_16				16				//beta save game
#define VERSION_17				17				//objId and new struct stats included
#define VERSION_18				18				//droid name savegame validity stamps
#define VERSION_19				19				//alliances, colours, radar zoom
#define VERSION_20				20				//MAX_NAME_ SIZE replaced by MAX_SAVE_NAME_SIZE
#define VERSION_21              21              //timeStartHold saved out for research facilities
#define VERSION_22              22              //asRundData
#define VERSION_23              23              //limbo droids and camstart droids saved properly, no cluster save
#define VERSION_24              24              //reinforceTime, droid move, droid resistance
#define VERSION_25              25              //limbo droid, research module hold fixed, cleaned by Alex
#define VERSION_26              26              //reArm pads
#define VERSION_27              27              //unit not the "d" word, experience and repair facility currentPtsAdded
#define VERSION_28              28              //rearm pads currentPtsAdded saved
#define VERSION_29              29              //mission scroll limits saved
#define VERSION_30              30              //script external variables saved
#define VERSION_31              31              //mission cheat time saved
#define VERSION_32              32              //factory secondary order saved
#define VERSION_33              33              //skirmish save
#define VERSION_34              34              //saves AI names for multiplayer (.gam file)
#define VERSION_35              35              //uses the (de)serialization API for saving/loading games and is big-endian instead of little-endian
#define VERSION_36              36              //saves beacon properly
//#define VERSION_37              37              //dpid changes; this had better be the last version
#define VERSION_38              38              //mod list!
#define VERSION_39		39		//lots of changes, breaking everything

#define CURRENT_VERSION_NUM     VERSION_39

//used in the loadGame
#define KEEPOBJECTS				true
#define FREEMEM					true

enum GAME_TYPE
{
	GTYPE_SCENARIO_START,	///< Initial scenario state.
	GTYPE_SCENARIO_EXPAND,	///< Scenario scroll area expansion.
	GTYPE_MISSION,		///< Stand alone mission.
	GTYPE_SAVE_START,	///< User saved game - at the start of a level.
	GTYPE_SAVE_MIDMISSION,	///< User saved game - in the middle of a level
};


struct VIS_SAVEHEADER
{
	char        aFileType[4];
	uint32_t    version;
};


/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/

extern bool loadGame(const char *pGameToLoad, bool keepObjects, bool freeMem, bool UserSaveGame);	// UserSaveGame is true when the save game is not a new level (User Save Game)

/*This just loads up the .gam file to determine which level data to set up - split up
so can be called in levLoadData when starting a game from a load save game*/
extern bool loadGameInit(const char* fileName);

extern bool loadMissionExtras(const char *pGameToLoad, SWORD levelType);

// load the script state given a .gam name
extern bool loadScriptState(char *pFileName);

/// Load the terrain types
extern bool loadTerrainTypeMap(const char *pFileData, UDWORD filesize);

extern bool saveGame(char *aFileName, GAME_TYPE saveType);

// Get the campaign number for loadGameInit game
extern UDWORD getCampaign(const char* fileName);

/*calls windows find file tree*/
extern bool getSaveGameName(char *pName);

/*returns the current type of save game being loaded*/
extern UDWORD getSaveGameType(void);

bool plotStructurePreview16(char *backDropSprite, Vector2i playeridpos[]);

#endif // __INCLUDED_SRC_GAME_H__
