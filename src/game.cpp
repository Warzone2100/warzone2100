/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
#include "lib/framework/wzapp.h"
#include <QtCore/QMap>

/* Standard library headers */
#include <physfs.h>
#include <string.h>

/* Warzone src and library headers */
#include "lib/framework/endian_hack.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/file.h"
#include "lib/framework/frameint.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/strres.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/netplay/netplay.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "modding.h"

#include "game.h"
#include "qtscript.h"
#include "fpath.h"
#include "map.h"
#include "droid.h"
#include "action.h"
#include "research.h"
#include "power.h"
#include "projectile.h"
#include "loadsave.h"
#include "text.h"
#include "message.h"
#include "hci.h"
#include "display.h"
#include "display3d.h"
#include "map.h"
#include "effects.h"
#include "init.h"
#include "mission.h"
#include "scores.h"
#include "anim_id.h"
#include "design.h"
#include "component.h"
#include "radar.h"
#include "cmddroid.h"
#include "warzoneconfig.h"
#include "multiplay.h"
#include "frontend.h"
#include "levels.h"
#include "mission.h"
#include "geometry.h"
#include "gateway.h"
#include "scripttabs.h"
#include "scriptvals.h"
#include "scriptextern.h"
#include "multistat.h"
#include "multiint.h"
#include "wrappers.h"
#include "scriptfuncs.h"
#include "challenge.h"
#include "combat.h"
#include "template.h"

#define MAX_SAVE_NAME_SIZE_V19	40
#define MAX_SAVE_NAME_SIZE	60

static const UDWORD NULL_ID = UDWORD_MAX;
#define MAX_BODY			SWORD_MAX
#define SAVEKEY_ONMISSION	0x100

/*!
 * FIXME
 * The code is reusing some pointers as normal integer values apparently. This
 * should be fixed!
 */
#define FIXME_CAST_ASSIGN(TYPE, lval, rval) memcpy(&lval, &rval, MIN(sizeof(lval), sizeof(rval)))

/// @note This represents a size internal to savegame files, so: DO NOT CHANGE THIS
#define MAX_GAME_STR_SIZE 20

static UDWORD RemapPlayerNumber(UDWORD OldNumber);
static void plotFeature(char *backDropSprite);

struct GAME_SAVEHEADER
{
	char        aFileType[4];
	uint32_t    version;
};

static bool serializeSaveGameHeader(PHYSFS_file* fileHandle, const GAME_SAVEHEADER* serializeHeader)
{
	if (PHYSFS_write(fileHandle, serializeHeader->aFileType, 4, 1) != 1)
		return false;

	// Write version numbers below version 35 as little-endian, and those above as big-endian
	if (serializeHeader->version < VERSION_35)
		return PHYSFS_writeULE32(fileHandle, serializeHeader->version);
	else
		return PHYSFS_writeUBE32(fileHandle, serializeHeader->version);
}

static bool deserializeSaveGameHeader(PHYSFS_file* fileHandle, GAME_SAVEHEADER* serializeHeader)
{
	// Read in the header from the file
	if (PHYSFS_read(fileHandle, serializeHeader->aFileType, 4, 1) != 1
	 || PHYSFS_read(fileHandle, &serializeHeader->version, sizeof(uint32_t), 1) != 1)
		return false;

	// All save game file versions below version 35 (i.e. _not_ version 35 itself)
	// have their version numbers stored as little endian. Versions from 35 and
	// onward use big-endian. This basically means that, because of endian
	// swapping, numbers from 35 and onward will be ridiculously high if a
	// little-endian byte-order is assumed.

	// Convert from little endian to native byte-order and check if we get a
	// ridiculously high number
	endian_udword(&serializeHeader->version);

	if (serializeHeader->version <= VERSION_34)
	{
		// Apparently we don't get a ridiculously high number if we assume
		// little-endian, so lets assume our version number is 34 at max and return
		debug(LOG_SAVE, "Version = %u (little-endian)", serializeHeader->version);

		return true;
	}
	else
	{
		// Apparently we get a larger number than expected if using little-endian.
		// So assume we have a version of 35 and onward

		// Reverse the little-endian decoding
		endian_udword(&serializeHeader->version);
	}

	// Considering that little-endian didn't work we now use big-endian instead
	serializeHeader->version = PHYSFS_swapUBE32(serializeHeader->version);
	debug(LOG_SAVE, "Version %u = (big-endian)", serializeHeader->version);

	return true;
}

struct STRUCT_SAVEHEADER : public GAME_SAVEHEADER
{
	UDWORD		quantity;
};

struct FEATURE_SAVEHEADER : public GAME_SAVEHEADER
{
	UDWORD		quantity;
};

/* Structure definitions for loading and saving map data */
struct TILETYPE_SAVEHEADER : public GAME_SAVEHEADER
{
	UDWORD quantity;
};

/* Sanity check definitions for the save struct file sizes */
#define GAME_HEADER_SIZE			8
#define DROIDINIT_HEADER_SIZE		12
#define STRUCT_HEADER_SIZE			12
#define FEATURE_HEADER_SIZE			12
#define TILETYPE_HEADER_SIZE		12

// general save definitions
#define MAX_LEVEL_SIZE 20

#define OBJECT_SAVE_V19 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	int32_t				inFire; \
	UDWORD				burnStart; \
	UDWORD				burnDamage

#define OBJECT_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	int32_t		inFire; \
	UDWORD				burnStart; \
	UDWORD				burnDamage

struct SAVE_POWER
{
	uint32_t    currentPower;
	uint32_t    extractedPower; // used for hacks
};

static bool serializeSavePowerData(PHYSFS_file* fileHandle, const SAVE_POWER* serializePower)
{
	return (PHYSFS_writeUBE32(fileHandle, serializePower->currentPower)
	     && PHYSFS_writeUBE32(fileHandle, serializePower->extractedPower));
}

static bool deserializeSavePowerData(PHYSFS_file* fileHandle, SAVE_POWER* serializePower)
{
	return (PHYSFS_readUBE32(fileHandle, &serializePower->currentPower)
	     && PHYSFS_readUBE32(fileHandle, &serializePower->extractedPower));
}

static bool serializeVector3i(PHYSFS_file* fileHandle, const Vector3i* serializeVector)
{
	return (PHYSFS_writeSBE32(fileHandle, serializeVector->x)
	     && PHYSFS_writeSBE32(fileHandle, serializeVector->y)
	     && PHYSFS_writeSBE32(fileHandle, serializeVector->z));
}

static bool deserializeVector3i(PHYSFS_file* fileHandle, Vector3i* serializeVector)
{
	int32_t x, y, z;

	if (!PHYSFS_readSBE32(fileHandle, &x)
	 || !PHYSFS_readSBE32(fileHandle, &y)
	 || !PHYSFS_readSBE32(fileHandle, &z))
		return false;

	serializeVector-> x = x;
	serializeVector-> y = y;
	serializeVector-> z = z;

	return true;
}

static bool serializeVector2i(PHYSFS_file* fileHandle, const Vector2i* serializeVector)
{
	return (PHYSFS_writeSBE32(fileHandle, serializeVector->x)
	     && PHYSFS_writeSBE32(fileHandle, serializeVector->y));
}

static bool deserializeVector2i(PHYSFS_file* fileHandle, Vector2i* serializeVector)
{
	int32_t x, y;

	if (!PHYSFS_readSBE32(fileHandle, &x)
	 || !PHYSFS_readSBE32(fileHandle, &y))
		return false;

	serializeVector-> x = x;
	serializeVector-> y = y;

	return true;
}

static bool serializeiViewData(PHYSFS_file* fileHandle, const iView* serializeView)
{
	return (serializeVector3i(fileHandle, &serializeView->p)
	     && serializeVector3i(fileHandle, &serializeView->r));
}

static bool deserializeiViewData(PHYSFS_file* fileHandle, iView* serializeView)
{
	return (deserializeVector3i(fileHandle, &serializeView->p)
	     && deserializeVector3i(fileHandle, &serializeView->r));
}

static bool serializeRunData(PHYSFS_file* fileHandle, const RUN_DATA* serializeRun)
{
	return (serializeVector2i(fileHandle, &serializeRun->sPos)
	     && PHYSFS_writeUBE8(fileHandle, serializeRun->forceLevel)
	     && PHYSFS_writeUBE8(fileHandle, serializeRun->healthLevel)
	     && PHYSFS_writeUBE8(fileHandle, serializeRun->leadership));
}

static bool deserializeRunData(PHYSFS_file* fileHandle, RUN_DATA* serializeRun)
{
	return (deserializeVector2i(fileHandle, &serializeRun->sPos)
	     && PHYSFS_readUBE8(fileHandle, &serializeRun->forceLevel)
	     && PHYSFS_readUBE8(fileHandle, &serializeRun->healthLevel)
	     && PHYSFS_readUBE8(fileHandle, &serializeRun->leadership));
}

static bool serializeLandingZoneData(PHYSFS_file* fileHandle, const LANDING_ZONE* serializeLandZone)
{
	return (PHYSFS_writeUBE8(fileHandle, serializeLandZone->x1)
	     && PHYSFS_writeUBE8(fileHandle, serializeLandZone->y1)
	     && PHYSFS_writeUBE8(fileHandle, serializeLandZone->x2)
	     && PHYSFS_writeUBE8(fileHandle, serializeLandZone->y2));
}

static bool deserializeLandingZoneData(PHYSFS_file* fileHandle, LANDING_ZONE* serializeLandZone)
{
	return (PHYSFS_readUBE8(fileHandle, &serializeLandZone->x1)
	     && PHYSFS_readUBE8(fileHandle, &serializeLandZone->y1)
	     && PHYSFS_readUBE8(fileHandle, &serializeLandZone->x2)
	     && PHYSFS_readUBE8(fileHandle, &serializeLandZone->y2));
}

static bool serializeMultiplayerGame(PHYSFS_file* fileHandle, const MULTIPLAYERGAME* serializeMulti)
{
	const char *dummy8c = "DUMMYSTRING";
	unsigned int i;

	if (!PHYSFS_writeUBE8(fileHandle, serializeMulti->type)
	 || PHYSFS_write(fileHandle, serializeMulti->map, 1, 128) != 128
	 || PHYSFS_write(fileHandle, dummy8c, 1, 8) != 8
	 || !PHYSFS_writeUBE8(fileHandle, serializeMulti->maxPlayers)
	 || PHYSFS_write(fileHandle, serializeMulti->name, 1, 128) != 128
	 || !PHYSFS_writeSBE32(fileHandle, 0)
	 || !PHYSFS_writeUBE32(fileHandle, serializeMulti->power)
	 || !PHYSFS_writeUBE8(fileHandle, serializeMulti->base)
	 || !PHYSFS_writeUBE8(fileHandle, serializeMulti->alliance)
	 || !PHYSFS_writeUBE8(fileHandle, 0)
	 || !PHYSFS_writeUBE16(fileHandle, 0)	// dummy, was bytesPerSec
	 || !PHYSFS_writeUBE8(fileHandle, 0)	// dummy, was packetsPerSec
	 || !PHYSFS_writeUBE8(fileHandle, challengeActive))	// reuse available field, was encryptKey
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE8(fileHandle, serializeMulti->skDiff[i]))
			return false;
	}

	return true;
}

static bool deserializeMultiplayerGame(PHYSFS_file* fileHandle, MULTIPLAYERGAME* serializeMulti)
{
	unsigned int i;
	int32_t boolFog;
	uint8_t dummy8;
	uint16_t dummy16;
	char dummy8c[8];

	if (!PHYSFS_readUBE8(fileHandle, &serializeMulti->type)
	 || PHYSFS_read(fileHandle, serializeMulti->map, 1, 128) != 128
	 || PHYSFS_read(fileHandle, dummy8c, 1, 8) != 8
	 || !PHYSFS_readUBE8(fileHandle, &serializeMulti->maxPlayers)
	 || PHYSFS_read(fileHandle, serializeMulti->name, 1, 128) != 128
	 || !PHYSFS_readSBE32(fileHandle, &boolFog)
	 || !PHYSFS_readUBE32(fileHandle, &serializeMulti->power)
	 || !PHYSFS_readUBE8(fileHandle, &serializeMulti->base)
	 || !PHYSFS_readUBE8(fileHandle, &serializeMulti->alliance)
	 || !PHYSFS_readUBE8(fileHandle, &dummy8)
	 || !PHYSFS_readUBE16(fileHandle, &dummy16)	// dummy, was bytesPerSec
	 || !PHYSFS_readUBE8(fileHandle, &dummy8)	// dummy, was packetsPerSec
	 || !PHYSFS_readUBE8(fileHandle, &dummy8))	// reused for challenge, was encryptKey
	{
		return false;
	}
	challengeActive = dummy8;	// hack

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE8(fileHandle, &serializeMulti->skDiff[i]))
			return false;
	}

	return true;
}

static bool serializePlayer(PHYSFS_file* fileHandle, const PLAYER* serializePlayer, int player)
{
	return (PHYSFS_writeUBE32(fileHandle, serializePlayer->position)
	     && PHYSFS_write(fileHandle, serializePlayer->name, StringSize, 1) == 1
	     && PHYSFS_write(fileHandle, getAIName(player), MAX_LEN_AI_NAME, 1) == 1
	     && PHYSFS_writeSBE8(fileHandle, serializePlayer->difficulty)
	     && PHYSFS_writeUBE8(fileHandle, (uint8_t)serializePlayer->allocated)
	     && PHYSFS_writeUBE32(fileHandle, serializePlayer->colour)
	     && PHYSFS_writeUBE32(fileHandle, serializePlayer->team));
}

static bool deserializePlayer(PHYSFS_file* fileHandle, PLAYER* serializePlayer, int player)
{
	char aiName[MAX_LEN_AI_NAME];
	uint32_t position, colour, team;
	bool retval;
	uint8_t allocated;

	retval = (PHYSFS_readUBE32(fileHandle, &position)
	          && PHYSFS_read(fileHandle, serializePlayer->name, StringSize, 1) == 1
	          && PHYSFS_read(fileHandle, aiName, MAX_LEN_AI_NAME, 1) == 1
	          && PHYSFS_readSBE8(fileHandle, &serializePlayer->difficulty)
	          && PHYSFS_readUBE8(fileHandle, &allocated)
	          && PHYSFS_readUBE32(fileHandle, &colour)
	          && PHYSFS_readUBE32(fileHandle, &team));

	serializePlayer->allocated = allocated;
	if (player < game.maxPlayers)
	{
		serializePlayer->ai = matchAIbyName(aiName);
		if (serializePlayer->ai == AI_NOT_FOUND)
		{
			debug(LOG_ERROR, "AI \"%s\" not found -- script loading will fail (player %d / %d)", aiName, player, game.maxPlayers);
		}
	}
	serializePlayer->position = position;
	serializePlayer->colour = colour;
	serializePlayer->team = team;
	return retval;
}

static bool serializeNetPlay(PHYSFS_file* fileHandle, const NETPLAY* serializeNetPlay)
{
	unsigned int i;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializePlayer(fileHandle, &serializeNetPlay->players[i], i))
			return false;
	}

	return (PHYSFS_writeUBE32(fileHandle, serializeNetPlay->bComms)
	     && PHYSFS_writeUBE32(fileHandle, serializeNetPlay->playercount)
	     && PHYSFS_writeUBE32(fileHandle, serializeNetPlay->hostPlayer)
	     && PHYSFS_writeUBE32(fileHandle, selectedPlayer)
	     && PHYSFS_writeUBE32(fileHandle, (uint32_t)game.scavengers)
	     && PHYSFS_writeUBE32(fileHandle, 0)
	     && PHYSFS_writeUBE32(fileHandle, 0));
}

static bool deserializeNetPlay(PHYSFS_file* fileHandle, NETPLAY* serializeNetPlay)
{
	unsigned int i;
	uint32_t dummy, scavs = game.scavengers;
	bool retv;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializePlayer(fileHandle, &serializeNetPlay->players[i], i))
			return false;
	}

	serializeNetPlay->isHost = true;	// only host can load
	retv = (PHYSFS_readUBE32(fileHandle, &serializeNetPlay->bComms)
	        && PHYSFS_readUBE32(fileHandle, &serializeNetPlay->playercount)
	        && PHYSFS_readUBE32(fileHandle, &serializeNetPlay->hostPlayer)
	        && PHYSFS_readUBE32(fileHandle, &selectedPlayer)
	        && PHYSFS_readUBE32(fileHandle, &scavs)
	        && PHYSFS_readUBE32(fileHandle, &dummy)
	        && PHYSFS_readUBE32(fileHandle, &dummy));
	game.scavengers = scavs;
	return retv;
}

struct SAVE_GAME_V7
{
	uint32_t    gameTime;
	uint32_t    GameType;                   /* Type of game , one of the GTYPE_... enums. */
	int32_t     ScrollMinX;                 /* Scroll Limits */
	int32_t     ScrollMinY;
	uint32_t    ScrollMaxX;
	uint32_t    ScrollMaxY;
	char        levelName[MAX_LEVEL_SIZE];  //name of the level to load up when mid game
};

static bool serializeSaveGameV7Data(PHYSFS_file* fileHandle, const SAVE_GAME_V7* serializeGame)
{
	return (PHYSFS_writeUBE32(fileHandle, serializeGame->gameTime)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->GameType)
	     && PHYSFS_writeSBE32(fileHandle, serializeGame->ScrollMinX)
	     && PHYSFS_writeSBE32(fileHandle, serializeGame->ScrollMinY)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->ScrollMaxX)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->ScrollMaxY)
	     && PHYSFS_write(fileHandle, serializeGame->levelName, MAX_LEVEL_SIZE, 1) == 1);
}

static bool deserializeSaveGameV7Data(PHYSFS_file* fileHandle, SAVE_GAME_V7* serializeGame)
{
	return (PHYSFS_readUBE32(fileHandle, &serializeGame->gameTime)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->GameType)
	     && PHYSFS_readSBE32(fileHandle, &serializeGame->ScrollMinX)
	     && PHYSFS_readSBE32(fileHandle, &serializeGame->ScrollMinY)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->ScrollMaxX)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->ScrollMaxY)
	     && PHYSFS_read(fileHandle, serializeGame->levelName, MAX_LEVEL_SIZE, 1) == 1);
}

struct SAVE_GAME_V10 : public SAVE_GAME_V7
{
	SAVE_POWER  power[MAX_PLAYERS];
};

static bool serializeSaveGameV10Data(PHYSFS_file* fileHandle, const SAVE_GAME_V10* serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV7Data(fileHandle, (const SAVE_GAME_V7*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializeSavePowerData(fileHandle, &serializeGame->power[i]))
			return false;
	}

	return true;
}

static bool deserializeSaveGameV10Data(PHYSFS_file* fileHandle, SAVE_GAME_V10* serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV7Data(fileHandle, (SAVE_GAME_V7*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializeSavePowerData(fileHandle, &serializeGame->power[i]))
			return false;
	}

	return true;
}

struct SAVE_GAME_V11 : public SAVE_GAME_V10
{
	iView currentPlayerPos;
};

static bool serializeSaveGameV11Data(PHYSFS_file* fileHandle, const SAVE_GAME_V11* serializeGame)
{
	return (serializeSaveGameV10Data(fileHandle, (const SAVE_GAME_V10*) serializeGame)
	     && serializeiViewData(fileHandle, &serializeGame->currentPlayerPos));
}

static bool deserializeSaveGameV11Data(PHYSFS_file* fileHandle, SAVE_GAME_V11* serializeGame)
{
	return (deserializeSaveGameV10Data(fileHandle, (SAVE_GAME_V10*) serializeGame)
	     && deserializeiViewData(fileHandle, &serializeGame->currentPlayerPos));
}

struct SAVE_GAME_V12 : public SAVE_GAME_V11
{
	uint32_t    missionTime;
	uint32_t    saveKey;
};

static bool serializeSaveGameV12Data(PHYSFS_file* fileHandle, const SAVE_GAME_V12* serializeGame)
{
	return (serializeSaveGameV11Data(fileHandle, (const SAVE_GAME_V11*) serializeGame)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->missionTime)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->saveKey));
}

static bool deserializeSaveGameV12Data(PHYSFS_file* fileHandle, SAVE_GAME_V12* serializeGame)
{
	return (deserializeSaveGameV11Data(fileHandle, (SAVE_GAME_V11*) serializeGame)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->missionTime)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->saveKey));
}

struct SAVE_GAME_V14 : public SAVE_GAME_V12
{
	int32_t     missionOffTime;
	int32_t     missionETA;
	uint16_t    missionHomeLZ_X;
	uint16_t    missionHomeLZ_Y;
	int32_t     missionPlayerX;
	int32_t     missionPlayerY;
	uint16_t    iTranspEntryTileX[MAX_PLAYERS];
	uint16_t    iTranspEntryTileY[MAX_PLAYERS];
	uint16_t    iTranspExitTileX[MAX_PLAYERS];
	uint16_t    iTranspExitTileY[MAX_PLAYERS];
	uint32_t    aDefaultSensor[MAX_PLAYERS];
	uint32_t    aDefaultECM[MAX_PLAYERS];
	uint32_t    aDefaultRepair[MAX_PLAYERS];
};

static bool serializeSaveGameV14Data(PHYSFS_file* fileHandle, const SAVE_GAME_V14* serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV12Data(fileHandle, (const SAVE_GAME_V12*) serializeGame)
	 || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionOffTime)
	 || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionETA)
	 || !PHYSFS_writeUBE16(fileHandle, serializeGame->missionHomeLZ_X)
	 || !PHYSFS_writeUBE16(fileHandle, serializeGame->missionHomeLZ_Y)
	 || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionPlayerX)
	 || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionPlayerY))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspEntryTileX[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspEntryTileY[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspExitTileX[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspExitTileY[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->aDefaultSensor[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->aDefaultECM[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->aDefaultRepair[i]))
			return false;
	}

	return true;
}

static bool deserializeSaveGameV14Data(PHYSFS_file* fileHandle, SAVE_GAME_V14* serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV12Data(fileHandle, (SAVE_GAME_V12*) serializeGame)
	 || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionOffTime)
	 || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionETA)
	 || !PHYSFS_readUBE16(fileHandle, &serializeGame->missionHomeLZ_X)
	 || !PHYSFS_readUBE16(fileHandle, &serializeGame->missionHomeLZ_Y)
	 || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionPlayerX)
	 || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionPlayerY))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspEntryTileX[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspEntryTileY[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspExitTileX[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspExitTileY[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->aDefaultSensor[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->aDefaultECM[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->aDefaultRepair[i]))
			return false;
	}

	return true;
}

struct SAVE_GAME_V15 : public SAVE_GAME_V14
{
	int32_t    offWorldKeepLists;	// was BOOL (which was a int)
	uint8_t     aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];
	uint32_t    RubbleTile;
	uint32_t    WaterTile;
	uint32_t    fogColour;
	uint32_t    fogState;
};

static bool serializeSaveGameV15Data(PHYSFS_file* fileHandle, const SAVE_GAME_V15* serializeGame)
{
	unsigned int i, j;

	if (!serializeSaveGameV14Data(fileHandle, (const SAVE_GAME_V14*) serializeGame)
	 || !PHYSFS_writeSBE32(fileHandle, serializeGame->offWorldKeepLists))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			if (!PHYSFS_writeUBE8(fileHandle, serializeGame->aDroidExperience[i][j]))
				return false;
		}
	}

	return (PHYSFS_writeUBE32(fileHandle, serializeGame->RubbleTile)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->WaterTile)
	     && PHYSFS_writeUBE32(fileHandle, 0)
	     && PHYSFS_writeUBE32(fileHandle, 0));
}

static bool deserializeSaveGameV15Data(PHYSFS_file* fileHandle, SAVE_GAME_V15* serializeGame)
{
	unsigned int i, j;
	int32_t boolOffWorldKeepLists;

	if (!deserializeSaveGameV14Data(fileHandle, (SAVE_GAME_V14*) serializeGame)
	 || !PHYSFS_readSBE32(fileHandle, &boolOffWorldKeepLists))
		return false;

	serializeGame->offWorldKeepLists = boolOffWorldKeepLists;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			if (!PHYSFS_readUBE8(fileHandle, &serializeGame->aDroidExperience[i][j]))
				return false;
		}
	}

	return (PHYSFS_readUBE32(fileHandle, &serializeGame->RubbleTile)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->WaterTile)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->fogColour)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->fogState));
}

struct SAVE_GAME_V16 : public SAVE_GAME_V15
{
	LANDING_ZONE   sLandingZone[MAX_NOGO_AREAS];
};

static bool serializeSaveGameV16Data(PHYSFS_file* fileHandle, const SAVE_GAME_V16* serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV15Data(fileHandle, (const SAVE_GAME_V15*) serializeGame))
		return false;

	for (i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		if (!serializeLandingZoneData(fileHandle, &serializeGame->sLandingZone[i]))
			return false;
	}

	return true;
}

static bool deserializeSaveGameV16Data(PHYSFS_file* fileHandle, SAVE_GAME_V16* serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV15Data(fileHandle, (SAVE_GAME_V15*) serializeGame))
		return false;

	for (i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		if (!deserializeLandingZoneData(fileHandle, &serializeGame->sLandingZone[i]))
			return false;
	}

	return true;
}

struct SAVE_GAME_V17 : public SAVE_GAME_V16
{
	uint32_t    objId;
};

static bool serializeSaveGameV17Data(PHYSFS_file* fileHandle, const SAVE_GAME_V17* serializeGame)
{
	return (serializeSaveGameV16Data(fileHandle, (const SAVE_GAME_V16*) serializeGame)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->objId));
}

static bool deserializeSaveGameV17Data(PHYSFS_file* fileHandle, SAVE_GAME_V17* serializeGame)
{
	return (deserializeSaveGameV16Data(fileHandle, (SAVE_GAME_V16*) serializeGame)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->objId));
}

struct SAVE_GAME_V18 : public SAVE_GAME_V17
{
	char        buildDate[MAX_STR_LENGTH];
	uint32_t    oldestVersion;
	uint32_t    validityKey;
};

static bool serializeSaveGameV18Data(PHYSFS_file* fileHandle, const SAVE_GAME_V18* serializeGame)
{
	return (serializeSaveGameV17Data(fileHandle, (const SAVE_GAME_V17*) serializeGame)
	     && PHYSFS_write(fileHandle, serializeGame->buildDate, MAX_STR_LENGTH, 1) == 1
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->oldestVersion)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->validityKey));
}

static bool deserializeSaveGameV18Data(PHYSFS_file* fileHandle, SAVE_GAME_V18* serializeGame)
{
	return (deserializeSaveGameV17Data(fileHandle, (SAVE_GAME_V17*) serializeGame)
	     && PHYSFS_read(fileHandle, serializeGame->buildDate, MAX_STR_LENGTH, 1) == 1
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->oldestVersion)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->validityKey));
}

struct SAVE_GAME_V19 : public SAVE_GAME_V18
{
	uint8_t     alliances[MAX_PLAYERS][MAX_PLAYERS];
	uint8_t     playerColour[MAX_PLAYERS];
	uint8_t     radarZoom;
};

static bool serializeSaveGameV19Data(PHYSFS_file* fileHandle, const SAVE_GAME_V19* serializeGame)
{
	unsigned int i, j;

	if (!serializeSaveGameV18Data(fileHandle, (const SAVE_GAME_V18*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_PLAYERS; ++j)
		{
			if (!PHYSFS_writeUBE8(fileHandle, serializeGame->alliances[i][j]))
				return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE8(fileHandle, serializeGame->playerColour[i]))
			return false;
	}

	return PHYSFS_writeUBE8(fileHandle, serializeGame->radarZoom);
}

static bool deserializeSaveGameV19Data(PHYSFS_file* fileHandle, SAVE_GAME_V19* serializeGame)
{
	unsigned int i, j;

	if (!deserializeSaveGameV18Data(fileHandle, (SAVE_GAME_V18*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_PLAYERS; ++j)
		{
			if (!PHYSFS_readUBE8(fileHandle, &serializeGame->alliances[i][j]))
			{
				return false;
			}
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE8(fileHandle, &serializeGame->playerColour[i]))
			return false;
	}

	return PHYSFS_readUBE8(fileHandle, &serializeGame->radarZoom);
}

struct SAVE_GAME_V20 : public SAVE_GAME_V19
{
	uint8_t     bDroidsToSafetyFlag;
	Vector2i    asVTOLReturnPos[MAX_PLAYERS];
};

static bool serializeSaveGameV20Data(PHYSFS_file* fileHandle, const SAVE_GAME_V20* serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV19Data(fileHandle, (const SAVE_GAME_V19*) serializeGame)
	 || !PHYSFS_writeUBE8(fileHandle, serializeGame->bDroidsToSafetyFlag))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializeVector2i(fileHandle, &serializeGame->asVTOLReturnPos[i]))
			return false;
	}

	return true;
}

static bool deserializeSaveGameV20Data(PHYSFS_file* fileHandle, SAVE_GAME_V20* serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV19Data(fileHandle, (SAVE_GAME_V19*) serializeGame)
	 || !PHYSFS_readUBE8(fileHandle, &serializeGame->bDroidsToSafetyFlag))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializeVector2i(fileHandle, &serializeGame->asVTOLReturnPos[i]))
			return false;
	}

	return true;
}

struct SAVE_GAME_V22 : public SAVE_GAME_V20
{
	RUN_DATA asRunData[MAX_PLAYERS];
};

static bool serializeSaveGameV22Data(PHYSFS_file* fileHandle, const SAVE_GAME_V22* serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV20Data(fileHandle, (const SAVE_GAME_V20*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializeRunData(fileHandle, &serializeGame->asRunData[i]))
			return false;
	}

	return true;
}

static bool deserializeSaveGameV22Data(PHYSFS_file* fileHandle, SAVE_GAME_V22* serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV20Data(fileHandle, (SAVE_GAME_V20*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializeRunData(fileHandle, &serializeGame->asRunData[i]))
			return false;
	}

	return true;
}

struct SAVE_GAME_V24 : public SAVE_GAME_V22
{
	uint32_t    reinforceTime;
	uint8_t     bPlayCountDown;
	uint8_t     bPlayerHasWon;
	uint8_t     bPlayerHasLost;
	uint8_t     dummy3;
};

static bool serializeSaveGameV24Data(PHYSFS_file* fileHandle, const SAVE_GAME_V24* serializeGame)
{
	return (serializeSaveGameV22Data(fileHandle, (const SAVE_GAME_V22*) serializeGame)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->reinforceTime)
	     && PHYSFS_writeUBE8(fileHandle, serializeGame->bPlayCountDown)
	     && PHYSFS_writeUBE8(fileHandle, serializeGame->bPlayerHasWon)
	     && PHYSFS_writeUBE8(fileHandle, serializeGame->bPlayerHasLost)
	     && PHYSFS_writeUBE8(fileHandle, serializeGame->dummy3));
}

static bool deserializeSaveGameV24Data(PHYSFS_file* fileHandle, SAVE_GAME_V24* serializeGame)
{
	return (deserializeSaveGameV22Data(fileHandle, (SAVE_GAME_V22*) serializeGame)
	     && PHYSFS_readUBE32(fileHandle, &serializeGame->reinforceTime)
	     && PHYSFS_readUBE8(fileHandle, &serializeGame->bPlayCountDown)
	     && PHYSFS_readUBE8(fileHandle, &serializeGame->bPlayerHasWon)
	     && PHYSFS_readUBE8(fileHandle, &serializeGame->bPlayerHasLost)
	     && PHYSFS_readUBE8(fileHandle, &serializeGame->dummy3));
}

struct SAVE_GAME_V27 : public SAVE_GAME_V24
{
	uint16_t    awDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];
};

static bool serializeSaveGameV27Data(PHYSFS_file* fileHandle, const SAVE_GAME_V27* serializeGame)
{
	unsigned int i, j;

	if (!serializeSaveGameV24Data(fileHandle, (const SAVE_GAME_V24*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			if (!PHYSFS_writeUBE16(fileHandle, serializeGame->awDroidExperience[i][j]))
				return false;
		}
	}

	return true;
}

static bool deserializeSaveGameV27Data(PHYSFS_file* fileHandle, SAVE_GAME_V27* serializeGame)
{
	unsigned int i, j;

	if (!deserializeSaveGameV24Data(fileHandle, (SAVE_GAME_V24*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			if (!PHYSFS_readUBE16(fileHandle, &serializeGame->awDroidExperience[i][j]))
				return false;
		}
	}

	return true;
}

struct SAVE_GAME_V29 : public SAVE_GAME_V27
{
	uint16_t    missionScrollMinX;
	uint16_t    missionScrollMinY;
	uint16_t    missionScrollMaxX;
	uint16_t    missionScrollMaxY;
};

static bool serializeSaveGameV29Data(PHYSFS_file* fileHandle, const SAVE_GAME_V29* serializeGame)
{
	return (serializeSaveGameV27Data(fileHandle, (const SAVE_GAME_V27*) serializeGame)
	     && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMinX)
	     && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMinY)
	     && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMaxX)
	     && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMaxY));
}

static bool deserializeSaveGameV29Data(PHYSFS_file* fileHandle, SAVE_GAME_V29* serializeGame)
{
	return (deserializeSaveGameV27Data(fileHandle, (SAVE_GAME_V27*) serializeGame)
	     && PHYSFS_readUBE16(fileHandle, &serializeGame->missionScrollMinX)
	     && PHYSFS_readUBE16(fileHandle, &serializeGame->missionScrollMinY)
	     && PHYSFS_readUBE16(fileHandle, &serializeGame->missionScrollMaxX)
	     && PHYSFS_readUBE16(fileHandle, &serializeGame->missionScrollMaxY));
}

struct SAVE_GAME_V30 : public SAVE_GAME_V29
{
	int32_t     scrGameLevel;
	uint8_t     bExtraVictoryFlag;
	uint8_t     bExtraFailFlag;
	uint8_t     bTrackTransporter;
};

static bool serializeSaveGameV30Data(PHYSFS_file* fileHandle, const SAVE_GAME_V30* serializeGame)
{
	return (serializeSaveGameV29Data(fileHandle, (const SAVE_GAME_V29*) serializeGame)
	     && PHYSFS_writeSBE32(fileHandle, serializeGame->scrGameLevel)
	     && PHYSFS_writeUBE8(fileHandle, serializeGame->bExtraVictoryFlag)
	     && PHYSFS_writeUBE8(fileHandle, serializeGame->bExtraFailFlag)
	     && PHYSFS_writeUBE8(fileHandle, serializeGame->bTrackTransporter));
}

static bool deserializeSaveGameV30Data(PHYSFS_file* fileHandle, SAVE_GAME_V30* serializeGame)
{
	return (deserializeSaveGameV29Data(fileHandle, (SAVE_GAME_V29*) serializeGame)
	     && PHYSFS_readSBE32(fileHandle, &serializeGame->scrGameLevel)
	     && PHYSFS_readUBE8(fileHandle, &serializeGame->bExtraVictoryFlag)
	     && PHYSFS_readUBE8(fileHandle, &serializeGame->bExtraFailFlag)
	     && PHYSFS_readUBE8(fileHandle, &serializeGame->bTrackTransporter));
}

//extra code for the patch - saves out whether cheated with the mission timer
struct SAVE_GAME_V31 : public SAVE_GAME_V30
{
	int32_t     missionCheatTime;
};


static bool serializeSaveGameV31Data(PHYSFS_file* fileHandle, const SAVE_GAME_V31* serializeGame)
{
	return (serializeSaveGameV30Data(fileHandle, (const SAVE_GAME_V30*) serializeGame)
	     && PHYSFS_writeSBE32(fileHandle, serializeGame->missionCheatTime));
}

static bool deserializeSaveGameV31Data(PHYSFS_file* fileHandle, SAVE_GAME_V31* serializeGame)
{
	return (deserializeSaveGameV30Data(fileHandle, (SAVE_GAME_V30*) serializeGame)
	     && PHYSFS_readSBE32(fileHandle, &serializeGame->missionCheatTime));
}

// alexl. skirmish saves
struct SAVE_GAME_V33 : public SAVE_GAME_V31
{
	MULTIPLAYERGAME sGame;
	NETPLAY         sNetPlay;
	uint32_t        savePlayer;
	char            sPName[32];
	int32_t         multiPlayer;	// was BOOL (int) ** see warning about conversion
	uint32_t        sPlayerIndex[MAX_PLAYERS];
};

static bool serializeSaveGameV33Data(PHYSFS_file* fileHandle, const SAVE_GAME_V33* serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV31Data(fileHandle, (const SAVE_GAME_V31*) serializeGame)
	 || !serializeMultiplayerGame(fileHandle, &serializeGame->sGame)
	 || !serializeNetPlay(fileHandle, &serializeGame->sNetPlay)
	 || !PHYSFS_writeUBE32(fileHandle, serializeGame->savePlayer)
	 || !PHYSFS_write(fileHandle, serializeGame->sPName, 1, 32) == 32
	 || !PHYSFS_writeSBE32(fileHandle, serializeGame->multiPlayer))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->sPlayerIndex[i]))
			return false;
	}

	return true;
}

static bool deserializeSaveGameV33Data(PHYSFS_file* fileHandle, SAVE_GAME_V33* serializeGame)
{
	unsigned int i;
	int32_t boolMultiPlayer;

	if (!deserializeSaveGameV31Data(fileHandle, (SAVE_GAME_V31*) serializeGame)
	 || !deserializeMultiplayerGame(fileHandle, &serializeGame->sGame)
	 || !deserializeNetPlay(fileHandle, &serializeGame->sNetPlay)
	 || !PHYSFS_readUBE32(fileHandle, &serializeGame->savePlayer)
	 || PHYSFS_read(fileHandle, serializeGame->sPName, 1, 32) != 32
	 || !PHYSFS_readSBE32(fileHandle, &boolMultiPlayer))
		return false;

	serializeGame->multiPlayer = boolMultiPlayer;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->sPlayerIndex[i]))
			return false;
	}

	return true;
}

//Now holds AI names for multiplayer
struct SAVE_GAME_V34 : public SAVE_GAME_V33
{
	char sPlayerName[MAX_PLAYERS][StringSize];
};


static bool serializeSaveGameV34Data(PHYSFS_file* fileHandle, const SAVE_GAME_V34* serializeGame)
{
	unsigned int i;
	if (!serializeSaveGameV33Data(fileHandle, (const SAVE_GAME_V33*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (PHYSFS_write(fileHandle, serializeGame->sPlayerName[i], StringSize, 1) != 1)
			return false;
	}

	return true;
}

static bool deserializeSaveGameV34Data(PHYSFS_file* fileHandle, SAVE_GAME_V34* serializeGame)
{
	unsigned int i;
	if (!deserializeSaveGameV33Data(fileHandle, (SAVE_GAME_V33*) serializeGame))
		return false;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (PHYSFS_read(fileHandle, serializeGame->sPlayerName[i], StringSize, 1) != 1)
			return false;
	}

	return true;
}

// First version to utilize (de)serialization API and first to be big-endian (instead of little-endian)
struct SAVE_GAME_V35 : public SAVE_GAME_V34
{
};

static bool serializeSaveGameV35Data(PHYSFS_file* fileHandle, const SAVE_GAME_V35* serializeGame)
{
	return serializeSaveGameV34Data(fileHandle, (const SAVE_GAME_V34*) serializeGame);
}

static bool deserializeSaveGameV35Data(PHYSFS_file* fileHandle, SAVE_GAME_V35* serializeGame)
{
	return deserializeSaveGameV34Data(fileHandle, (SAVE_GAME_V34*) serializeGame);
}

// Store loaded mods in savegame
struct SAVE_GAME_V38 : public SAVE_GAME_V35
{
	char modList[modlist_string_size];
};

static bool serializeSaveGameV38Data(PHYSFS_file* fileHandle, const SAVE_GAME_V38* serializeGame)
{
	if (!serializeSaveGameV35Data(fileHandle, (const SAVE_GAME_V35*) serializeGame))
		return false;

	if (PHYSFS_write(fileHandle, serializeGame->modList, modlist_string_size, 1) != 1)
		return false;

	return true;
}

static bool deserializeSaveGameV38Data(PHYSFS_file* fileHandle, SAVE_GAME_V38* serializeGame)
{
	if (!deserializeSaveGameV35Data(fileHandle, (SAVE_GAME_V35*) serializeGame))
		return false;

	if (PHYSFS_read(fileHandle, serializeGame->modList, modlist_string_size, 1) != 1)
		return false;

	return true;
}

// Current save game version
typedef SAVE_GAME_V38 SAVE_GAME;

static bool serializeSaveGameData(PHYSFS_file* fileHandle, const SAVE_GAME* serializeGame)
{
	return serializeSaveGameV38Data(fileHandle, (const SAVE_GAME_V38*) serializeGame);
}

static bool deserializeSaveGameData(PHYSFS_file* fileHandle, SAVE_GAME* serializeGame)
{
	return deserializeSaveGameV38Data(fileHandle, (SAVE_GAME_V38*) serializeGame);
}

struct DROIDINIT_SAVEHEADER : public GAME_SAVEHEADER
{
	UDWORD		quantity;
};

struct SAVE_DROIDINIT
{
	OBJECT_SAVE_V19;
};

/*
 *	STRUCTURE Definitions
 */

#define STRUCTURE_SAVE_V2 \
	OBJECT_SAVE_V19; \
	UBYTE				status; \
	SDWORD				currentBuildPts; \
	UDWORD				body; \
	UDWORD				armour; \
	UDWORD				resistance; \
	UDWORD				dummy1; \
	UDWORD				subjectInc;  /*research inc or factory prod id*/\
	UDWORD				timeStarted; \
	UDWORD				output; \
	UDWORD				capacity; \
	UDWORD				quantity

struct SAVE_STRUCTURE_V2
{
	STRUCTURE_SAVE_V2;
};

#define STRUCTURE_SAVE_V12 \
	STRUCTURE_SAVE_V2; \
	UDWORD				factoryInc;			\
	UBYTE				loopsPerformed;		\
	UDWORD				powerAccrued;		\
	UDWORD				dummy2;			\
	UDWORD				droidTimeStarted;	\
	UDWORD				timeToBuild;		\
	UDWORD				timeStartHold

struct SAVE_STRUCTURE_V12
{
	STRUCTURE_SAVE_V12;
};

#define STRUCTURE_SAVE_V14 \
	STRUCTURE_SAVE_V12; \
	UBYTE	visible[MAX_PLAYERS]

struct SAVE_STRUCTURE_V14
{
	STRUCTURE_SAVE_V14;
};

#define STRUCTURE_SAVE_V15 \
	STRUCTURE_SAVE_V14; \
	char	researchName[MAX_SAVE_NAME_SIZE_V19]

struct SAVE_STRUCTURE_V15
{
	STRUCTURE_SAVE_V15;
};

#define STRUCTURE_SAVE_V17 \
	STRUCTURE_SAVE_V15;\
	SWORD				currentPowerAccrued

struct SAVE_STRUCTURE_V17
{
	STRUCTURE_SAVE_V17;
};

#define STRUCTURE_SAVE_V20 \
	OBJECT_SAVE_V20; \
	UBYTE				status; \
	SDWORD				currentBuildPts; \
	UDWORD				body; \
	UDWORD				armour; \
	UDWORD				resistance; \
	UDWORD				dummy1; \
	UDWORD				subjectInc;  /*research inc or factory prod id*/\
	UDWORD				timeStarted; \
	UDWORD				output; \
	UDWORD				capacity; \
	UDWORD				quantity; \
	UDWORD				factoryInc;			\
	UBYTE				loopsPerformed;		\
	UDWORD				powerAccrued;		\
	UDWORD				dummy2;			\
	UDWORD				droidTimeStarted;	\
	UDWORD				timeToBuild;		\
	UDWORD				timeStartHold; \
	UBYTE				visible[MAX_PLAYERS]; \
	char				researchName[MAX_SAVE_NAME_SIZE]; \
	SWORD				currentPowerAccrued

struct SAVE_STRUCTURE_V20
{
	STRUCTURE_SAVE_V20;
};

#define STRUCTURE_SAVE_V21 \
	STRUCTURE_SAVE_V20; \
	UDWORD				commandId

struct SAVE_STRUCTURE_V21
{
	STRUCTURE_SAVE_V21;
};

struct SAVE_STRUCTURE
{
	STRUCTURE_SAVE_V21;
};

#define FEATURE_SAVE_V2 \
	OBJECT_SAVE_V19

struct SAVE_FEATURE_V2
{
	FEATURE_SAVE_V2;
};

#define FEATURE_SAVE_V14 \
	FEATURE_SAVE_V2; \
	UBYTE	visible[MAX_PLAYERS]

struct SAVE_FEATURE_V14
{
	FEATURE_SAVE_V14;
};

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/
extern uint32_t unsynchObjID;  // unique ID creation thing..
extern uint32_t synchObjID;    // unique ID creation thing..

static UDWORD			saveGameVersion = 0;
static bool				saveGameOnMission = false;
static SAVE_GAME		saveGameData;

static UDWORD	savedGameTime;
static UDWORD	savedObjId;
static SDWORD	startX, startY;
static UDWORD   width, height;
static UDWORD	gameType;
static bool IsScenario;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/
static bool gameLoadV7(PHYSFS_file* fileHandle);
static bool gameLoadV(PHYSFS_file* fileHandle, unsigned int version);
static bool writeGameFile(const char* fileName, SDWORD saveType);
static bool writeMapFile(const char* fileName);

static bool loadSaveDroidInit(char *pFileData, UDWORD filesize);

static bool loadSaveDroid(const char *pFileName, DROID **ppsCurrentDroidLists);
static bool loadSaveDroidPointers(const QString &pFileName, DROID **ppsCurrentDroidLists);
static bool writeDroidFile(const char *pFileName, DROID **ppsCurrentDroidLists);

static bool loadSaveStructure(char *pFileData, UDWORD filesize);
static bool loadSaveStructure2(const char *pFileName, STRUCTURE **ppList);
static bool loadSaveStructurePointers(QString filename, STRUCTURE **ppList);
static bool writeStructFile(const char *pFileName);

static bool loadSaveTemplate(const char *pFileName);
static bool writeTemplateFile(const char *pFileName);

static bool loadSaveFeature(char *pFileData, UDWORD filesize);
static bool writeFeatureFile(const char *pFileName);
static bool loadSaveFeature2(const char *pFileName);

static bool writeTerrainTypeMapFile(char *pFileName);

static bool loadSaveCompList(const char *pFileName);
static bool writeCompListFile(const char *pFileName);

static bool loadSaveStructTypeList(const char *pFileName);
static bool writeStructTypeListFile(const char *pFileName);

static bool loadSaveResearch(const char *pFileName);
static bool writeResearchFile(char *pFileName);

static bool loadSaveMessage(const char *pFileName, SWORD levelType);
static bool writeMessageFile(const char *pFileName);

static bool loadSaveStructLimits(const char *pFileName);
static bool writeStructLimitsFile(const char *pFileName);

static bool readFiresupportDesignators(const char *pFileName);
static bool writeFiresupportDesignators(const char *pFileName);

static bool writeScriptState(const char *pFileName);

static bool gameLoad(const char* fileName);

/* set the global scroll values to use for the save game */
static void setMapScroll(void);

static bool gameLoad(const char* fileName);

static char *getSaveStructNameV19(SAVE_STRUCTURE_V17 *psSaveStructure)
{
	return(psSaveStructure->name);
}

/*This just loads up the .gam file to determine which level data to set up - split up
so can be called in levLoadData when starting a game from a load save game*/

// -----------------------------------------------------------------------------------------
bool loadGameInit(const char* fileName)
{
	if (!gameLoad(fileName))
	{
		debug(LOG_ERROR, "Corrupted / unsupported savegame file %s, Unable to load!", fileName);
		// NOTE: why do we start the game clock on a *failed* load?
		// Start the game clock
		gameTimeStart();

		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// Load a file from a save game into the psx.
// This is divided up into 2 parts ...
//
// if it is a level loaded up from CD then UserSaveGame will by false
// UserSaveGame ... Extra stuff to load after scripts
bool loadMissionExtras(const char *pGameToLoad, SWORD levelType)
{
	char			aFileName[256];
	UDWORD			fileExten;

	strcpy(aFileName, pGameToLoad);
	fileExten = strlen(pGameToLoad) - 3;
	aFileName[fileExten - 1] = '\0';
	strcat(aFileName, "/");

	if (saveGameVersion >= VERSION_11)
	{
		//if user save game then load up the messages AFTER any droids or structures are loaded
		if (gameType == GTYPE_SAVE_START || gameType == GTYPE_SAVE_MIDMISSION)
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "messtate.ini");
			if (!loadSaveMessage(aFileName, levelType))
			{
				debug(LOG_ERROR, "Failed to load mission extras from %s", aFileName);
				return false;
			}
		}
	}

	return true;
}

static void sanityUpdate()
{
	scrvUpdateBasePointers();	// update the script object pointers
	for (int player = 0; player < game.maxPlayers; player++)
	{
		for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
		{
			orderCheckList(psDroid);
			actionSanity(psDroid);
		}
	}
}

static void getIniBaseObject(WzConfig &ini, QString const &key, BASE_OBJECT *&object)
{
	object = NULL;
	if (ini.contains(key + "/id"))
	{
		int tid = ini.value(key + "/id", -1).toInt();
		int tplayer = ini.value(key + "/player", -1).toInt();
		OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value(key + "/type", 0).toInt();
		ASSERT_OR_RETURN(, tid >= 0 && tplayer >= 0, "Bad ID");
		object = getBaseObjFromData(tid, tplayer, ttype);
		ASSERT(object != NULL, "Failed to find target");
	}
}

static void getIniStructureStats(WzConfig &ini, QString const &key, STRUCTURE_STATS *&stats)
{
	stats = NULL;
	if (ini.contains(key))
	{
		QString statName = ini.value(key).toString();
		int tid = getStructStatFromName(statName.toUtf8().constData());
		ASSERT_OR_RETURN(, tid >= 0, "Target stats not found %s", statName.toUtf8().constData());
		stats = &asStructureStats[tid];
	}
}

static void getIniDroidOrder(WzConfig &ini, QString const &key, DroidOrder &order)
{
	order.type = (DroidOrderType)ini.value(key + "/type", DORDER_NONE).toInt();
	order.pos = ini.vector2i(key + "/pos");
	order.pos2 = ini.vector2i(key + "/pos2");
	order.direction = ini.value(key + "/direction").toInt();
	getIniBaseObject(ini, key + "/obj", order.psObj);
	getIniStructureStats(ini, key + "/stats", order.psStats);
}

static void setIniBaseObject(WzConfig &ini, QString const &key, BASE_OBJECT const *object)
{
	if (object != NULL && object->died <= 1)
	{
		ini.setValue(key + "/id", object->id);
		ini.setValue(key + "/player", object->player);
		ini.setValue(key + "/type", object->type);
#ifdef DEBUG
		//ini.setValue(key + "/debugfunc", QString::fromUtf8(psCurr->targetFunc));
		//ini.setValue(key + "/debugline", psCurr->targetLine);
#endif
	}
}

static void setIniStructureStats(WzConfig &ini, QString const &key, STRUCTURE_STATS const *stats)
{
	if (stats != NULL)
	{
		ini.setValue(key, stats->pName);
	}
}

static void setIniDroidOrder(WzConfig &ini, QString const &key, DroidOrder const &order)
{
	ini.setValue(key + "/type", order.type);
	ini.setVector2i(key + "/pos", order.pos);
	ini.setVector2i(key + "/pos2", order.pos2);
	ini.setValue(key + "/direction", order.direction);
	setIniBaseObject(ini, key + "/obj", order.psObj);
	setIniStructureStats(ini, key + "/stats", order.psStats);
}

// -----------------------------------------------------------------------------------------
// UserSaveGame ... this is true when you are loading a players save game
bool loadGame(const char *pGameToLoad, bool keepObjects, bool freeMem, bool UserSaveGame)
{
	QMap<QString, DROID **> droidMap;
	QMap<QString, STRUCTURE **> structMap;
	char			aFileName[256];
	UDWORD			fileExten, fileSize;
	char			*pFileData = NULL;
	UDWORD			player, inc, i, j;
	DROID           *psCurr;
	UWORD           missionScrollMinX = 0, missionScrollMinY = 0,
	                missionScrollMaxX = 0, missionScrollMaxY = 0;

	/* Stop the game clock */
	gameTimeStop();

	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		gameTimeReset(savedGameTime);//added 14 may 98 JPS to solve kev's problem with no firing droids
		//need to reset the event timer too - AB 14/01/99
		eventTimeReset(savedGameTime/SCR_TICKRATE);
	}

	/* Clear all the objects off the map and free up the map memory */
	proj_FreeAllProjectiles();	//always clear this
	if (freeMem)
	{
		//clear out the audio
		audio_StopAll();

		freeAllDroids();
		freeAllStructs();
		freeAllFeatures();

		//clear all the messages?
		releaseAllProxDisp();
	}

	if (!keepObjects)
	{
		//initialise the lists
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			apsDroidLists[player] = NULL;
			apsStructLists[player] = NULL;
			apsFeatureLists[player] = NULL;
			apsFlagPosLists[player] = NULL;
			//clear all the messages?
			apsProxDisp[player] = NULL;
			apsSensorList[0] = NULL;
			apsExtractorLists[player] = NULL;
		}
		apsOilList[0] = NULL;
		initFactoryNumFlag();
	}

	if (UserSaveGame)//always !keepObjects
	{
		//initialise the lists
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			apsLimboDroids[player] = NULL;
			mission.apsDroidLists[player] = NULL;
			mission.apsStructLists[player] = NULL;
			mission.apsFeatureLists[player] = NULL;
			mission.apsFlagPosLists[player] = NULL;
			mission.apsExtractorLists[player] = NULL;
		}
		mission.apsOilList[0] = NULL;
		mission.apsSensorList[0] = NULL;
	}

	if (saveGameVersion >= VERSION_11)
	{
		//camera position
		disp3d_setView(&(saveGameData.currentPlayerPos));
	}
	else
	{
		disp3d_resetView();
	}

	//Stuff added after level load to avoid being reset or initialised during load
	if (UserSaveGame)//always !keepObjects
	{
		if (saveGameVersion >= VERSION_12)
		{
			mission.startTime = saveGameData.missionTime;
		}

		//set the scroll varaibles
		startX = saveGameData.ScrollMinX;
		startY = saveGameData.ScrollMinY;
		width = saveGameData.ScrollMaxX - saveGameData.ScrollMinX;
		height = saveGameData.ScrollMaxY - saveGameData.ScrollMinY;
		gameType = saveGameData.GameType;

		if (saveGameVersion >= VERSION_14)
		{
			//mission data
			mission.time		=	saveGameData.missionOffTime;
			mission.ETA			=	saveGameData.missionETA;
			mission.homeLZ_X	=	saveGameData.missionHomeLZ_X;
			mission.homeLZ_Y	=	saveGameData.missionHomeLZ_Y;
			mission.playerX		=	saveGameData.missionPlayerX;
			mission.playerY		=	saveGameData.missionPlayerY;
			//mission data
			for (player = 0; player < MAX_PLAYERS; player++)
			{
				aDefaultSensor[player]				= saveGameData.aDefaultSensor[player];
				aDefaultECM[player]					= saveGameData.aDefaultECM[player];
				aDefaultRepair[player]				= saveGameData.aDefaultRepair[player];
				//check for self repair having been set
				if (aDefaultRepair[player] != 0
				 && asRepairStats[aDefaultRepair[player]].location == LOC_DEFAULT)
				{
					enableSelfRepair((UBYTE)player);
				}
				mission.iTranspEntryTileX[player]	= saveGameData.iTranspEntryTileX[player];
				mission.iTranspEntryTileY[player]	= saveGameData.iTranspEntryTileY[player];
				mission.iTranspExitTileX[player]	= saveGameData.iTranspExitTileX[player];
				mission.iTranspExitTileY[player]	= saveGameData.iTranspExitTileY[player];
			}
		}

		if (saveGameVersion >= VERSION_15)//V21
		{
			offWorldKeepLists	= saveGameData.offWorldKeepLists;
			setRubbleTile(saveGameData.RubbleTile);
			setUnderwaterTile(saveGameData.WaterTile);
		}
		if (saveGameVersion >= VERSION_19)//V21
		{
			for(i=0; i<MAX_PLAYERS; i++)
			{
		                alliancebits[i] |= 0;
				for(j=0; j<MAX_PLAYERS; j++)
				{
					alliances[i][j] = saveGameData.alliances[i][j];
					if (i == j) alliances[i][j] = ALLIANCE_FORMED;	// hack to fix old savegames
					if (game.alliance == ALLIANCES_TEAMS && alliances[i][j] == ALLIANCE_FORMED)
					{
			                	alliancebits[i] |= 1 << j;
					}
				}
			}
			for(i=0; i<MAX_PLAYERS; i++)
			{
				setPlayerColour(i,saveGameData.playerColour[i]);
			}
			SetRadarZoom(saveGameData.radarZoom);
		}

		if (saveGameVersion >= VERSION_20)//V21
		{
			setDroidsToSafetyFlag(saveGameData.bDroidsToSafetyFlag);
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				memcpy(&asVTOLReturnPos[inc], &(saveGameData.asVTOLReturnPos[inc]), sizeof(Vector2i));
			}
		}

		if (saveGameVersion >= VERSION_22)//V22
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				memcpy(&asRunData[inc], &(saveGameData.asRunData[inc]), sizeof(RUN_DATA));
			}
		}

		if (saveGameVersion >= VERSION_24)//V24
		{
			missionSetReinforcementTime(saveGameData.reinforceTime);
			// horrible hack to catch savegames that were saving garbage into these fields
			if (saveGameData.bPlayCountDown <= 1)
			{
				setPlayCountDown(saveGameData.bPlayCountDown);
			}
			if (saveGameData.bPlayerHasWon <= 1)
			{
				setPlayerHasWon(saveGameData.bPlayerHasWon);
			}
			if (saveGameData.bPlayerHasLost <= 1)
			{
				setPlayerHasLost(saveGameData.bPlayerHasLost);
			}
		}

		if (saveGameVersion >= VERSION_27)//V27
		{
			for (player = 0; player < MAX_PLAYERS; player++)
			{
				for (inc = 0; inc < MAX_RECYCLED_DROIDS; inc++)
				{
					aDroidExperience[player][inc]	= saveGameData.awDroidExperience[player][inc];
				}
			}
		}
		else
		{
			for (player = 0; player < MAX_PLAYERS; player++)
			{
				for (inc = 0; inc < MAX_RECYCLED_DROIDS; inc++)
				{
					aDroidExperience[player][inc]	= saveGameData.aDroidExperience[player][inc];
				}
			}
		}
		if (saveGameVersion >= VERSION_30)
		{
			scrGameLevel = saveGameData.scrGameLevel;
			bExtraVictoryFlag = saveGameData.bExtraVictoryFlag;
			bExtraFailFlag = saveGameData.bExtraFailFlag;
			bTrackTransporter = saveGameData.bTrackTransporter;
		}

		//extra code added for the first patch (v1.1) to save out if mission time is not being counted
		if (saveGameVersion >= VERSION_31)
		{
			//mission data
			mission.cheatTime = saveGameData.missionCheatTime;
		}

		// skirmish saves.
		if (saveGameVersion >= VERSION_33)
		{
			PLAYERSTATS		playerStats;
			bool scav = game.scavengers;

			game			= saveGameData.sGame;
			game.scavengers = scav;	// ok, so this is butt ugly. but i'm just getting inspiration from the rest of the code around here. ok? - per
			productionPlayer= selectedPlayer;
			bMultiPlayer	= saveGameData.multiPlayer;
			bMultiMessages	= bMultiPlayer;
			cmdDroidMultiExpBoost(true);

			NetPlay.bComms = (saveGameData.sNetPlay).bComms;
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				NetPlay.players[i].ai = saveGameData.sNetPlay.players[i].ai;
				NetPlay.players[i].difficulty = saveGameData.sNetPlay.players[i].difficulty;
				strcpy(NetPlay.players[i].name, saveGameData.sNetPlay.players[i].name);
				if (saveGameData.sGame.skDiff[i] == UBYTE_MAX || (game.type == CAMPAIGN && i == 0))
				{
					NetPlay.players[i].allocated = true;
				}
				else
				{
					NetPlay.players[i].allocated = false;
				}
			}
			
			if(bMultiPlayer)
			{
				loadMultiStats(saveGameData.sPName,&playerStats);				// stats stuff
				setMultiStats(selectedPlayer, playerStats, false);
				setMultiStats(selectedPlayer, playerStats, true);
			}
		}
	}

	/* Get human and AI players names */
	if (saveGameVersion >= VERSION_34)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			(void)setPlayerName(i, saveGameData.sPlayerName[i]);
		}
	}

	//clear the player Power structs
	if ((gameType != GTYPE_SAVE_START) && (gameType != GTYPE_SAVE_MIDMISSION) &&
		(!keepObjects))
	{
		clearPlayerPower();
	}

	//before loading the data - turn power off so don't get any power low warnings
	powerCalculated = false;

	/* Load in the chosen file data */
	strcpy(aFileName, pGameToLoad);
	fileExten = strlen(aFileName) - 3;			// hack - !
	aFileName[fileExten - 1] = '\0';
	strcat(aFileName, "/");

	// Load labels
	aFileName[fileExten] = '\0';
	strcat(aFileName, "labels.ini");
	loadLabels(aFileName);

	//the terrain type WILL only change with Campaign changes (well at the moment!)
	if (gameType != GTYPE_SCENARIO_EXPAND || UserSaveGame)
	{
		//load in the terrain type map
		aFileName[fileExten] = '\0';
		strcat(aFileName, "ttypes.ttp");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail23\n" );
			goto error;
		}


		//load the terrain type data
		if (pFileData)
		{
			if (!loadTerrainTypeMap(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail25\n" );
				goto error;
			}
		}
	}

	//load up the Droid Templates BEFORE any structures are loaded
	if (IsScenario==false)
	{
		{
			DROID_TEMPLATE	*pTemplate, *pNext;
			for(pTemplate = apsDroidTemplates[0]; pTemplate != NULL;
				pTemplate = pNext)
			{
				pNext = pTemplate->psNext;
				delete pTemplate;
			}
			apsDroidTemplates[0] = NULL;
		}

		// In Multiplayer, clear templates out first.....
		if (bMultiPlayer)
		{
			for(inc=0;inc<MAX_PLAYERS;inc++)
			{
				while(apsDroidTemplates[inc])				// clear the old template out.
				{
					DROID_TEMPLATE	*psTempl;
					psTempl = apsDroidTemplates[inc]->psNext;
					delete apsDroidTemplates[inc];
					apsDroidTemplates[inc] = psTempl;
				}
			}
		}
		for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
		{
			free(i->pName);
		}
		localTemplates.clear();

		//load in the templates
		aFileName[fileExten] = '\0';
		strcat(aFileName, "templates.ini");
		//load the data into apsTemplates
		if (!loadSaveTemplate(aFileName))
		{
			debug(LOG_NEVER, "Failed to load templates from %s", aFileName);
			goto error;
		}
	}

	if (saveGameOnMission && UserSaveGame)
	{
		//the scroll limits for the mission map have already been written
		if (saveGameVersion >= VERSION_29)
		{
			missionScrollMinX = (UWORD)mission.scrollMinX;
			missionScrollMinY = (UWORD)mission.scrollMinY;
			missionScrollMaxX = (UWORD)mission.scrollMaxX;
			missionScrollMaxY = (UWORD)mission.scrollMaxY;
		}

		//load the map and the droids then swap pointers

		//load in the map file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mission.map");
		if (!mapLoad(aFileName, false))
		{
			debug(LOG_ERROR, "Failed to load map");
			return false;
		}

		//load in the visibility file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "misvis.bjo");

		// Load in the visibility data from the chosen file
		if (!readVisibilityData(aFileName))
		{
			debug( LOG_NEVER, "loadgame: Fail33\n" );
			goto error;
		}

		// reload the objects that were in the mission list
		//load in the features -do before the structures
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mfeature.ini");

		//load the data into apsFeatureLists
		if (!loadSaveFeature2(aFileName))
		{
			aFileName[fileExten] = '\0';
			strcat(aFileName, "mfeat.bjo");
			/* Load in the chosen file data */
			pFileData = fileLoadBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail14\n" );
				goto error;
			}
			if (!loadSaveFeature(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail16\n" );
				goto error;
			}
		}

		initStructLimits();
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mstruct.ini");

		//load in the mission structures
		if (!loadSaveStructure2(aFileName, apsStructLists))
		{
			aFileName[fileExten] = '\0';
			strcat(aFileName, "mstruct.bjo");
			/* Load in the chosen file data */
			pFileData = fileLoadBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail17\n" );
				goto error;
			}
			//load the data into apsStructLists
			if (!loadSaveStructure(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail19\n" );
				goto error;
			}
		}
		else
		{
			structMap.insert(aFileName, mission.apsStructLists);	// we swap pointers below
		}

		// load in the mission droids, if any
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mdroid.ini");
		if (loadSaveDroid(aFileName, apsDroidLists))
		{
			droidMap.insert(aFileName, mission.apsDroidLists); // need to swap here to read correct list later
		}

		/* after we've loaded in the units we need to redo the orientation because
		 * the direction may have been saved - we need to do it outside of the loop
		 * whilst the current map is valid for the units
		 */
		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
			{
				if (psCurr->droidType != DROID_PERSON
				// && psCurr->droidType != DROID_CYBORG
				 && !cyborgDroid(psCurr)
				 && psCurr->droidType != DROID_TRANSPORTER
				 && psCurr->pos.x != INVALID_XY)
				{
					updateDroidOrientation(psCurr);
				}
			}
		}

		swapMissionPointers();

		//once the mission map has been loaded reset the mission scroll limits
		if (saveGameVersion >= VERSION_29)
		{
			mission.scrollMinX = missionScrollMinX;
			mission.scrollMinY = missionScrollMinY;
			mission.scrollMaxX = missionScrollMaxX;
			mission.scrollMaxY = missionScrollMaxY;
		}
	}

	//if Campaign Expand then don't load in another map
	if (gameType != GTYPE_SCENARIO_EXPAND)
	{
		psMapTiles = NULL;
		//load in the map file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "game.map");
		if (!mapLoad(aFileName, false))
		{
			debug( LOG_NEVER, "loadgame: Fail7\n" );
			return(false);
		}
	}

	// FIXME THIS FILE IS A HUGE MESS, this code should probably appear at another position...
	if (saveGameVersion > VERSION_12)
	{
		//if user save game then load up the FX
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "fxstate.ini");

			// load the fx data from the file
			if (!readFXData(aFileName))
			{
				debug(LOG_ERROR, "loadgame: Fail33");
				goto error;
			}
		}
	}

	//save game stuff added after map load

	if (saveGameVersion >= VERSION_16)
	{
		for (inc = 0; inc < MAX_NOGO_AREAS; inc++)
		{
			setNoGoArea(saveGameData.sLandingZone[inc].x1, saveGameData.sLandingZone[inc].y1,
						saveGameData.sLandingZone[inc].x2, saveGameData.sLandingZone[inc].y2, (UBYTE)inc);
		}
	}

	//adjust the scroll range for the new map or the expanded map
	setMapScroll();

	//initialise the Templates' build and power points before loading in any droids
	initTemplatePoints();

	//if user save game then load up the research BEFORE any droids or structures are loaded
	if (gameType == GTYPE_SAVE_START || gameType == GTYPE_SAVE_MIDMISSION)
	{
		//load in the research list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "resstate.ini");
		if (!loadSaveResearch(aFileName))
		{
			debug(LOG_ERROR, "Failed to load research data from %s", aFileName);
			goto error;
		}
	}

	if (IsScenario)
	{
		//load in the droids
		aFileName[fileExten] = '\0';
		strcat(aFileName, "droid.ini");

		//load the data into apsDroidLists
		if (loadSaveDroid(aFileName, apsDroidLists))
		{
			debug(LOG_SAVE, "Loaded new style droids");
			droidMap.insert(aFileName, apsDroidLists);	// load pointers later
		}
		else
		{
			// load in the old style droid initialisation file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "dinit.bjo");
			/* Load in the chosen file data */
			pFileData = fileLoadBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail8\n" );
				goto error;
			}
			if (!loadSaveDroidInit(pFileData,fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail10\n" );
				goto error;
			}
			debug(LOG_SAVE, "Loaded old style droids");
		}
	}
	else
	{
		//load in the droids
		aFileName[fileExten] = '\0';
		strcat(aFileName, "droid.ini");

		//load the data into apsDroidLists
		if (!loadSaveDroid(aFileName, apsDroidLists))
		{
			debug(LOG_ERROR, "failed to load %s", aFileName);
			goto error;
		}
		droidMap.insert(aFileName, apsDroidLists);	// load pointers later

		/* after we've loaded in the units we need to redo the orientation because
		 * the direction may have been saved - we need to do it outside of the loop
		 * whilst the current map is valid for the units
		 */
		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
			{
				if (psCurr->droidType != DROID_PERSON
				 && !cyborgDroid(psCurr)
				 && psCurr->droidType != DROID_TRANSPORTER
				 && psCurr->pos.x != INVALID_XY)
				{
					updateDroidOrientation(psCurr);
				}
			}
		}
		if (!saveGameOnMission)
		{
			//load in the mission droids
			aFileName[fileExten] = '\0';
			strcat(aFileName, "mdroid.ini");

			// load the data into mission.apsDroidLists, if any
			if (loadSaveDroid(aFileName, mission.apsDroidLists))
			{
				droidMap.insert(aFileName, mission.apsDroidLists);
			}
		}
	}

	if (saveGameVersion >= VERSION_23)
	{
		// load in the limbo droids, if any
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limbo.ini");
		if (loadSaveDroid(aFileName, apsLimboDroids))
		{
			droidMap.insert(aFileName, apsLimboDroids);
		}
	}

	//load in the features -do before the structures
	aFileName[fileExten] = '\0';
	strcat(aFileName, "feature.ini");
	if (!loadSaveFeature2(aFileName))
	{
		aFileName[fileExten] = '\0';
		strcat(aFileName, "feat.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail14\n" );
			goto error;
		}

		//load the data into apsFeatureLists
		if (!loadSaveFeature(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail16\n" );
			goto error;
		}
	}

	//load in the structures
	initStructLimits();
	aFileName[fileExten] = '\0';
	strcat(aFileName, "struct.ini");
	if (!loadSaveStructure2(aFileName, apsStructLists))
	{
		aFileName[fileExten] = '\0';
		strcat(aFileName, "struct.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail17\n" );
			goto error;
		}
		//load the data into apsStructLists
		if (!loadSaveStructure(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail19\n" );
			goto error;
		}
	}
	else
	{
		structMap.insert(aFileName, apsStructLists);
	}

	//if user save game then load up the current level for structs and components
	if (gameType == GTYPE_SAVE_START || gameType == GTYPE_SAVE_MIDMISSION)
	{
		//load in the component list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "complist.ini");
		if (!loadSaveCompList(aFileName))
		{
			debug(LOG_ERROR, "failed to load %s", aFileName);
			goto error;
		}
		//load in the structure type list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "strtype.ini");
		if (!loadSaveStructTypeList(aFileName))
		{
			debug(LOG_ERROR, "failed to load %s", aFileName);
			goto error;
		}
	}

	if (saveGameVersion >= VERSION_11)
	{
		//if user save game then load up the Visibility
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the visibility file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "visstate.bjo");

			// Load in the visibility data from the chosen file
			if (!readVisibilityData(aFileName))
			{
				debug( LOG_NEVER, "loadgame: Fail33\n" );
				goto error;
			}
		}
	}

	if (saveGameVersion >= VERSION_16)
	{
		//if user save game then load up the FX
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			aFileName[fileExten] = '\0';
			strcat(aFileName, "score.ini");

			// Load the fx data from the chosen file
			if (!readScoreData(aFileName))
			{
				debug( LOG_NEVER, "loadgame: Fail33\n" );
				goto error;
			}
		}
	}

	if (saveGameVersion >= VERSION_21)
	{
		//rebuild the apsCommandDesignation AFTER all droids and structures are loaded
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the command list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "firesupport.ini");

			if (!readFiresupportDesignators(aFileName))
			{
				debug( LOG_NEVER, "loadMissionExtras: readFiresupportDesignators(%s) failed\n", aFileName );
				goto error;
			}
		}
	}

	if ((saveGameVersion >= VERSION_15) && UserSaveGame)
	{
		//load in the mission structures
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limits.ini");

		//load the data into apsStructLists
		if (!loadSaveStructLimits(aFileName))
		{
			debug(LOG_ERROR, "failed to load %s", aFileName);
			goto error;
		}

		//set up the structure Limits
		setCurrentStructQuantity(false);
	}
	else
	{
		//set up the structure Limits
		setCurrentStructQuantity(true);
	}

	//check that delivery points haven't been put down in invalid location
	checkDeliveryPoints(saveGameVersion);

	//turn power on for rest of game
	powerCalculated = true;

	if (!keepObjects)//only reset the pointers if they were set
	{
		// Reset the object pointers in the droid target lists
		QStringList keys = droidMap.keys();
		for (int i = 0; i < keys.size(); i++)
		{
			QString key = keys.at(i);
			DROID **pList = droidMap.value(key);
			loadSaveDroidPointers(key, pList);
		}
		keys = structMap.keys();
		for (int i = 0; i < keys.size(); i++)
		{
			QString key = keys.at(i);
			STRUCTURE **pList = structMap.value(key);
			loadSaveStructurePointers(key, pList);
		}
	}

	//if user save game then reset the time - THIS SETS BOTH TIMERS - BEWARE IF YOU USE IT
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		ASSERT( gameTime == savedGameTime,"loadGame; game time modified during load" );
		gameTimeReset(savedGameTime);//added 14 may 98 JPS to solve kev's problem with no firing droids
		//need to reset the event timer too - AB 14/01/99
		eventTimeReset(savedGameTime/SCR_TICKRATE);

		//reset the objId for new objects
		if (saveGameVersion >= VERSION_17)
		{
			unsynchObjID = (savedObjId + 1)/2;  // Make new object ID start at savedObjId*8.
			synchObjID   = savedObjId*4;        // Make new object ID start at savedObjId*8.
		}
	}

	//check the research button isn't flashing unnecessarily
	//cancel first
	stopReticuleButtonFlash(IDRET_RESEARCH);
	//then see if needs to be set
	intNotifyResearchButton(0);

	//set up the mission countdown flag
	setMissionCountDown();

	/* Start the game clock */
	gameTimeStart();

	//check if limbo_expand mission has changed to an expand mission for user save game (mid-mission)
	if (gameType == GTYPE_SAVE_MIDMISSION && missionLimboExpand())
	{
		/* when all the units have moved from the mission.apsDroidList then the
		 * campaign has been reset to an EXPAND type - OK so there should have
		 * been another flag to indicate this state has changed but its late in
		 * the day excuses...excuses...excuses
		 */
		if (mission.apsDroidLists[selectedPlayer] == NULL)
		{
			//set the mission type
			startMissionSave(LDS_EXPAND);
		}
	}

	//set this if come into a save game mid mission
	if (gameType == GTYPE_SAVE_MIDMISSION)
	{
		setScriptWinLoseVideo(PLAY_NONE);
	}

	//need to clear before setting up
	clearMissionWidgets();
	//put any widgets back on for the missions
	resetMissionWidgets();

	debug( LOG_NEVER, "loadGame: done\n" );

	return true;

error:
	debug(LOG_ERROR, "Game load failed");

	/* Clear all the objects off the map and free up the map memory */
	freeAllDroids();
	freeAllStructs();
	freeAllFeatures();
	droidTemplateShutDown();
	psMapTiles = NULL;

	/* Start the game clock */
	gameTimeStart();

	return false;
}
// -----------------------------------------------------------------------------------------

// Modified by AlexL , now takes a filename, with no popup....
bool saveGame(char *aFileName, GAME_TYPE saveType)
{
	UDWORD			fileExtension;
	DROID			*psDroid, *psNext;
	char			CurrentFileName[PATH_MAX] = {'\0'};

	ASSERT_OR_RETURN(false, aFileName && strlen(aFileName) > 4, "Bad savegame filename");
	sstrcpy(CurrentFileName, aFileName);
	debug(LOG_WZ, "saveGame: %s", CurrentFileName);

	fileExtension = strlen(CurrentFileName) - 3;
	gameTimeStop();
	sanityUpdate();

	/* Write the data to the file */
	if (!writeGameFile(CurrentFileName, saveType))
	{
		debug(LOG_ERROR, "saveGame: writeGameFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//remove the file extension
	CurrentFileName[strlen(CurrentFileName) - 4] = '\0';

	//create dir will fail if directory already exists but don't care!
	(void) PHYSFS_mkdir(CurrentFileName);

	//save the map file
	strcat(CurrentFileName, "/game.map");
	/* Write the data to the file */
	if (!writeMapFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeMapFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	// Save labels
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "labels.ini");
	writeLabels(CurrentFileName);

	//create the droids filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "droid.ini");
	/*Write the current droid lists to the file*/
	if (!writeDroidFile(CurrentFileName, apsDroidLists))
	{
		debug(LOG_ERROR, "writeDroidFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the structures filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "struct.ini");
	/*Write the data to the file*/
	if (!writeStructFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the templates filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "templates.ini");
	/*Write the data to the file*/
	if (!writeTemplateFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeTemplateFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the features filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "feature.ini");
	/*Write the data to the file*/
	if (!writeFeatureFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeFeatureFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the terrain types filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "ttypes.ttp");
	/*Write the data to the file*/
	if (!writeTerrainTypeMapFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeTerrainTypeMapFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the strucutLimits filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "limits.ini");
	/*Write the data to the file*/
	if (!writeStructLimitsFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructLimitsFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the component lists filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "complist.ini");
	/*Write the data to the file*/
	if (!writeCompListFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeCompListFile(\"%s\") failed", CurrentFileName);
		goto error;
	}
	//create the structure type lists filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "strtype.ini");
	/*Write the data to the file*/
	if (!writeStructTypeListFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructTypeListFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the research filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "resstate.ini");
	/*Write the data to the file*/
	if (!writeResearchFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeResearchFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "messtate.ini");
	/*Write the data to the file*/
	if (!writeMessageFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeMessageFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "visstate.bjo");
	/*Write the data to the file*/
	if (!writeVisibilityData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeVisibilityData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "fxstate.ini");
	/*Write the data to the file*/
	if (!writeFXData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeFXData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//added at V15 save
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "score.ini");
	/*Write the data to the file*/
	if (!writeScoreData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeScoreData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "firesupport.ini");
	/*Write the data to the file*/
	if (!writeFiresupportDesignators(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeFiresupportDesignators(\"%s\") failed", CurrentFileName);
		goto error;
	}

	// save the script state if necessary
	if (saveType == GTYPE_SAVE_MIDMISSION)
	{
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "scriptstate.es");
		/*Write the data to the file*/
		if (!writeScriptState(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeScriptState(\"%s\") failed", CurrentFileName);
			goto error;
		}
	}

	//create the droids filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "mdroid.ini");
	/*Write the swapped droid lists to the file*/
	if (!writeDroidFile(CurrentFileName, mission.apsDroidLists))
	{
		debug(LOG_ERROR, "writeDroidFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the limbo filename
	//clear the list
	if (saveGameVersion < VERSION_25)
	{
		for (psDroid = apsLimboDroids[selectedPlayer]; psDroid != NULL; psDroid = psNext)
		{
			psNext = psDroid->psNext;
			//limbo list invalidate XY
			psDroid->pos.x = INVALID_XY;
			psDroid->pos.y = INVALID_XY;
			//this is mainly for VTOLs
			setSaveDroidBase(psDroid, NULL);
			psDroid->cluster = 0;
			orderDroid(psDroid, DORDER_STOP, ModeImmediate);
		}
	}

	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "limbo.bjo");
	/*Write the swapped droid lists to the file*/
	if (!writeDroidFile(CurrentFileName, apsLimboDroids))
	{
		debug(LOG_ERROR, "saveGame: writeDroidFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	if (saveGameOnMission )
	{
		//mission save swap the mission pointers and save the changes
		swapMissionPointers();
		//now save the map and droids

		//save the map file
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "mission.map");
		/* Write the data to the file */
		if (!writeMapFile(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeMapFile(\"%s\") failed", CurrentFileName);
			goto error;
		}

		//save the map file
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "misvis.bjo");
		/* Write the data to the file */
		if (!writeVisibilityData(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeVisibilityData(\"%s\") failed", CurrentFileName);
			goto error;
		}

		//create the structures filename
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "mstruct.ini");
		/*Write the data to the file*/
		if (!writeStructFile(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeStructFile(\"%s\") failed", CurrentFileName);
			goto error;
		}

		//create the features filename
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "mfeature.ini");
		/*Write the data to the file*/
		if (!writeFeatureFile(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeFeatureFile(\"%s\") failed", CurrentFileName);
			goto error;
		}

		//mission save swap back so we can restart the game
		swapMissionPointers();
	}

	// strip the last filename
	CurrentFileName[fileExtension-1] = '\0';

	/* Start the game clock */
	gameTimeStart();
	return true;

error:
	/* Start the game clock */
	gameTimeStart();

	return false;
}

// -----------------------------------------------------------------------------------------
static bool writeMapFile(const char* fileName)
{
	char* pFileData = NULL;
	UDWORD fileSize;

	/* Get the save data */
	bool status = mapSave(&pFileData, &fileSize);

	if (status)
	{
		/* Write the data to the file */
		status = saveFile(fileName, pFileData, fileSize);
	}

	if (pFileData != NULL)
	{
		free(pFileData);
	}

	return status;
}

// -----------------------------------------------------------------------------------------
static bool gameLoad(const char* fileName)
{
	GAME_SAVEHEADER fileHeader;

	PHYSFS_file* fileHandle = openLoadFile(fileName, true);
	if (!fileHandle)
	{
		// Failure to open the file is a failure to load the specified savegame
		return true;
	}
	initLoadingScreen(true);
	debug(LOG_WZ, "gameLoad");

	// Read the header from the file
	if (!deserializeSaveGameHeader(fileHandle, &fileHeader))
	{
		debug(LOG_ERROR, "gameLoad: error while reading header from file (%s): %s", fileName, PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return false;
	}

	// Check the header to see if we've been given a file of the right type
	if (fileHeader.aFileType[0] != 'g'
	 || fileHeader.aFileType[1] != 'a'
	 || fileHeader.aFileType[2] != 'm'
	 || fileHeader.aFileType[3] != 'e')
	{
		debug(LOG_ERROR, "gameLoad: Weird file type found? Has header letters - '%c' '%c' '%c' '%c' (should be 'g' 'a' 'm' 'e')",
		      fileHeader.aFileType[0],
		      fileHeader.aFileType[1],
		      fileHeader.aFileType[2],
		      fileHeader.aFileType[3]);

		PHYSFS_close(fileHandle);

		return false;
	}

	debug(LOG_NEVER, "gl .gam file is version %u\n", fileHeader.version);

	//set main version Id from game file
	saveGameVersion = fileHeader.version;
	debug(LOG_SAVE, "file version is %u, (%s)", fileHeader.version, fileName);
	/* Check the file version */
	if (fileHeader.version < VERSION_7)
	{
		debug(LOG_ERROR, "gameLoad: unsupported save format version %d", fileHeader.version);
		PHYSFS_close(fileHandle);

		return false;
	}
	else if (fileHeader.version < VERSION_9)
	{
		bool retVal = gameLoadV7(fileHandle);
		PHYSFS_close(fileHandle);
		return retVal;
	}
	else if (fileHeader.version <= CURRENT_VERSION_NUM)
	{
		bool retVal = gameLoadV(fileHandle, fileHeader.version);
		PHYSFS_close(fileHandle);
		return retVal;
	}
	else
	{
		debug(LOG_ERROR, "Unsupported main save format version %u", fileHeader.version);
		PHYSFS_close(fileHandle);

		return false;
	}
}

// Fix endianness of a savegame
static void endian_SaveGameV(SAVE_GAME* psSaveGame, UDWORD version)
{
	unsigned int i, j;
	/* SAVE_GAME is GAME_SAVE_V33 */
	/* GAME_SAVE_V33 includes GAME_SAVE_V31 */
	if(version >= VERSION_33)
	{
		endian_udword(&psSaveGame->sGame.power);
		endian_udword(&psSaveGame->sNetPlay.playercount);
		endian_udword(&psSaveGame->savePlayer);
		for(i = 0; i < MAX_PLAYERS; i++)
			endian_udword(&psSaveGame->sPlayerIndex[i]);
	}
	/* GAME_SAVE_V31 includes GAME_SAVE_V30 */
	if(version >= VERSION_31) {
		endian_sdword(&psSaveGame->missionCheatTime);
	}
	/* GAME_SAVE_V30 includes GAME_SAVE_V29 */
	if(version >= VERSION_30) {
		endian_sdword(&psSaveGame->scrGameLevel);
	}
	/* GAME_SAVE_V29 includes GAME_SAVE_V27 */
	if(version >= VERSION_29) {
		endian_uword(&psSaveGame->missionScrollMinX);
		endian_uword(&psSaveGame->missionScrollMinY);
		endian_uword(&psSaveGame->missionScrollMaxX);
		endian_uword(&psSaveGame->missionScrollMaxY);
	}
	/* GAME_SAVE_V27 includes GAME_SAVE_V24 */
	if(version >= VERSION_27) {
		for(i = 0; i < MAX_PLAYERS; i++)
			for(j = 0; j < MAX_RECYCLED_DROIDS; j++)
				endian_uword(&psSaveGame->awDroidExperience[i][j]);
	}
	/* GAME_SAVE_V24 includes GAME_SAVE_V22 */
	if(version >= VERSION_24) {
		endian_udword(&psSaveGame->reinforceTime);
	}
	/* GAME_SAVE_V22 includes GAME_SAVE_V20 */
	if(version >= VERSION_22) {
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_sdword(&psSaveGame->asRunData[i].sPos.x);
			endian_sdword(&psSaveGame->asRunData[i].sPos.y);
		}
	}
	/* GAME_SAVE_V20 includes GAME_SAVE_V19 */
	if(version >= VERSION_20) {
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_sdword(&psSaveGame->asVTOLReturnPos[i].x);
			endian_sdword(&psSaveGame->asVTOLReturnPos[i].y);
		}
	}
	/* GAME_SAVE_V19 includes GAME_SAVE_V18 */
	if(version >= VERSION_19) {
	}
	/* GAME_SAVE_V18 includes GAME_SAVE_V17 */
	if(version >= VERSION_18) {
		endian_udword(&psSaveGame->oldestVersion);
		endian_udword(&psSaveGame->validityKey);
	}
	/* GAME_SAVE_V17 includes GAME_SAVE_V16 */
	if(version >= VERSION_17) {
		endian_udword(&psSaveGame->objId);
	}
	/* GAME_SAVE_V16 includes GAME_SAVE_V15 */
	if(version >= VERSION_16) {
	}
	/* GAME_SAVE_V15 includes GAME_SAVE_V14 */
	if(version >= VERSION_15) {
		endian_udword(&psSaveGame->RubbleTile);
		endian_udword(&psSaveGame->WaterTile);
	}
	/* GAME_SAVE_V14 includes GAME_SAVE_V12 */
	if(version >= VERSION_14) {
		endian_sdword(&psSaveGame->missionOffTime);
		endian_sdword(&psSaveGame->missionETA);
		endian_uword(&psSaveGame->missionHomeLZ_X);
		endian_uword(&psSaveGame->missionHomeLZ_Y);
		endian_sdword(&psSaveGame->missionPlayerX);
		endian_sdword(&psSaveGame->missionPlayerY);
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_uword(&psSaveGame->iTranspEntryTileX[i]);
			endian_uword(&psSaveGame->iTranspEntryTileY[i]);
			endian_uword(&psSaveGame->iTranspExitTileX[i]);
			endian_uword(&psSaveGame->iTranspExitTileY[i]);
			endian_udword(&psSaveGame->aDefaultSensor[i]);
			endian_udword(&psSaveGame->aDefaultECM[i]);
			endian_udword(&psSaveGame->aDefaultRepair[i]);
		}
	}
	/* GAME_SAVE_V12 includes GAME_SAVE_V11 */
	if(version >= VERSION_12) {
		endian_udword(&psSaveGame->missionTime);
		endian_udword(&psSaveGame->saveKey);
	}
	/* GAME_SAVE_V11 includes GAME_SAVE_V10 */
	if(version >= VERSION_11) {
		endian_sdword(&psSaveGame->currentPlayerPos.p.x);
		endian_sdword(&psSaveGame->currentPlayerPos.p.y);
		endian_sdword(&psSaveGame->currentPlayerPos.p.z);
		endian_sdword(&psSaveGame->currentPlayerPos.r.x);
		endian_sdword(&psSaveGame->currentPlayerPos.r.y);
		endian_sdword(&psSaveGame->currentPlayerPos.r.z);
	}
	/* GAME_SAVE_V10 includes GAME_SAVE_V7 */
	if(version >= VERSION_10) {
		for(i = 0; i < MAX_PLAYERS; i++) {
			endian_udword(&psSaveGame->power[i].currentPower);
			endian_udword(&psSaveGame->power[i].extractedPower);
		}
	}
	/* GAME_SAVE_V7 */
	if(version >= VERSION_7) {
		endian_udword(&psSaveGame->gameTime);
		endian_udword(&psSaveGame->GameType);
		endian_sdword(&psSaveGame->ScrollMinX);
		endian_sdword(&psSaveGame->ScrollMinY);
		endian_udword(&psSaveGame->ScrollMaxX);
		endian_udword(&psSaveGame->ScrollMaxY);
	}
}

// -----------------------------------------------------------------------------------------
// Get campaign number stuff is not needed in this form on the PSX (thank you very much)
static UDWORD getCampaignV(PHYSFS_file* fileHandle, unsigned int version)
{
	SAVE_GAME_V14 saveGame;

	debug(LOG_SAVE, "getCampaignV: version = %u", version);

	if (version < VERSION_14)
	{
		return 0;
	}
	// We only need VERSION 12 data (saveGame.saveKey)
	else if (version <= VERSION_34)
	{
		if (PHYSFS_read(fileHandle, &saveGame, sizeof(SAVE_GAME_V14), 1) != 1)
		{
			debug(LOG_ERROR, "getCampaignV: error while reading file: %s", PHYSFS_getLastError());

			return 0;
		}

		// Convert from little-endian to native byte-order
		endian_SaveGameV((SAVE_GAME*)&saveGame, VERSION_14);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		if (!deserializeSaveGameV14Data(fileHandle, &saveGame))
		{
			debug(LOG_ERROR, "getCampaignV: error while reading file: %s", PHYSFS_getLastError());

			return 0;
		}
	}
	else
	{
		debug(LOG_ERROR, "Bad savegame version %u", version);
		return 0;
	}

	return saveGame.saveKey & (SAVEKEY_ONMISSION - 1);
}

// -----------------------------------------------------------------------------------------
// Returns the campaign number  --- apparently this is does alot less than it look like
    /// it now does even less than it looks like on the psx ... cause its pc only
UDWORD getCampaign(const char* fileName)
{
	GAME_SAVEHEADER fileHeader;

	PHYSFS_file* fileHandle = openLoadFile(fileName, true);
	if (!fileHandle)
	{
		// Failure to open the file is a failure to load the specified savegame
		return false;
	}

	debug(LOG_WZ, "getCampaign: %s", fileName);

	// Read the header from the file
	if (!deserializeSaveGameHeader(fileHandle, &fileHeader))
	{
		debug(LOG_ERROR, "getCampaign: error while reading header from file (%s): %s", fileName, PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return false;
	}

	// Check the header to see if we've been given a file of the right type
	if (fileHeader.aFileType[0] != 'g'
	 || fileHeader.aFileType[1] != 'a'
	 || fileHeader.aFileType[2] != 'm'
	 || fileHeader.aFileType[3] != 'e')
	{
		debug(LOG_ERROR, "getCampaign: Weird file type found? Has header letters - '%c' '%c' '%c' '%c' (should be 'g' 'a' 'm' 'e')",
		      fileHeader.aFileType[0],
		      fileHeader.aFileType[1],
		      fileHeader.aFileType[2],
		      fileHeader.aFileType[3]);

		PHYSFS_close(fileHandle);

		return false;
	}

	debug(LOG_NEVER, "gl .gam file is version %d\n", fileHeader.version);

	//set main version Id from game file
	saveGameVersion = fileHeader.version;

	debug(LOG_SAVE, "fileversion is %u, (%s) ", fileHeader.version, fileName);
	/* Check the file version */
	if (fileHeader.version < VERSION_14)
	{
		PHYSFS_close(fileHandle);
		return 0;
	}

	// what the arse bollocks is this
			// the campaign number is fine prior to saving
			// you save it out in a skirmish save and
			// then don't bother putting it back in again
			// when loading so it screws loads of stuff?!?
	// dont check skirmish saves.
	if (fileHeader.version <= CURRENT_VERSION_NUM)
	{
		UDWORD retVal = getCampaignV(fileHandle, fileHeader.version);
		PHYSFS_close(fileHandle);
		return retVal;
	}
	else
	{
		debug(LOG_ERROR, "getCampaign: undefined save format version %d", fileHeader.version);
		PHYSFS_close(fileHandle);

		return 0;
	}

	PHYSFS_close(fileHandle);
	return 0;
}

// -----------------------------------------------------------------------------------------
/* code specific to version 7 of a save game */
bool gameLoadV7(PHYSFS_file* fileHandle)
{
	SAVE_GAME_V7 saveGame;

	if (PHYSFS_read(fileHandle, &saveGame, sizeof(saveGame), 1) != 1)
	{
		debug(LOG_ERROR, "gameLoadV7: error while reading file: %s", PHYSFS_getLastError());

		return false;
	}

	/* GAME_SAVE_V7 */
	endian_udword(&saveGame.gameTime);
	endian_udword(&saveGame.GameType);
	endian_sdword(&saveGame.ScrollMinX);
	endian_sdword(&saveGame.ScrollMinY);
	endian_udword(&saveGame.ScrollMaxX);
	endian_udword(&saveGame.ScrollMaxY);

	savedGameTime = saveGame.gameTime;

	//set the scroll varaibles
	startX = saveGame.ScrollMinX;
	startY = saveGame.ScrollMinY;
	width = saveGame.ScrollMaxX - saveGame.ScrollMinX;
	height = saveGame.ScrollMaxY - saveGame.ScrollMinY;
	gameType = saveGame.GameType;
	//set IsScenario to true if not a user saved game
	if (gameType == GTYPE_SAVE_START)
	{
		LEVEL_DATASET* psNewLevel;

		IsScenario = false;
		//copy the level name across
		sstrcpy(aLevelName, saveGame.levelName);
		//load up the level dataset
		if (!levLoadData(aLevelName, saveGameName, (GAME_TYPE)gameType))
		{
			return false;
		}
		// find the level dataset
		psNewLevel = levFindDataSet(aLevelName);
		if (psNewLevel == NULL)
		{
			debug( LOG_ERROR, "gameLoadV7: couldn't find level data" );

			return false;
		}
		//check to see whether mission automatically starts
		//shouldn't be able to be any other value at the moment!
		if (psNewLevel->type == LDS_CAMSTART
		 || psNewLevel->type == LDS_BETWEEN
		 || psNewLevel->type == LDS_EXPAND
		 || psNewLevel->type == LDS_EXPAND_LIMBO)
		{
			launchMission();
		}

	}
	else
	{
		IsScenario = true;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
/* non specific version of a save game */
bool gameLoadV(PHYSFS_file* fileHandle, unsigned int version)
{
	unsigned int i, j;
	static	SAVE_POWER	powerSaved[MAX_PLAYERS];
	UDWORD			player;

	debug(LOG_WZ, "gameLoadV: version %u", version);

	// Version 7 and earlier are loaded separately in gameLoadV7

	//size is now variable so only check old save games
	if (version <= VERSION_10)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V10), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version == VERSION_11)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V11), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_12)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V12), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_14)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V14), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_15)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V15), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_16)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V16), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_17)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V17), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_18)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V18), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_19)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V19), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_21)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V20), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_23)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V22), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_26)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V24), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_28)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V27), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_29)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V29), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_30)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V30), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_32)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V31), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_33)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V33), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_34)
	{
		if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V34), 1) != 1)
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else if (version < VERSION_39)
	{
		debug(LOG_ERROR, "Unsupported savegame version");
		return false;
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		if (!deserializeSaveGameData(fileHandle, &saveGameData))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading data from file for deserialization (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported version number (%u) for savegame", version);

		return false;
	}

	debug(LOG_SAVE, "Savegame is of type: %u", saveGameData.sGame.type);

	// Campaign games are fine, only skirmish games are broken in v36
	if (saveGameData.sGame.type != CAMPAIGN && version == VERSION_36)
	{
		debug(LOG_ERROR, "Skirmish savegames of version %u are not supported in this release.", version);
		return false;
	}
	game.type = saveGameData.sGame.type;
	/* Test mod list */
	if (version >= VERSION_38)
	{
		setOverrideMods(saveGameData.modList);
	}

	// All savegames from version 34 or before are little endian so swap them. All
	// from version 35, and onward, are already swapped to the native byte-order
	// by the (de)serialization API
	if (version <= VERSION_34)
	{
		endian_SaveGameV(&saveGameData, version);
	}

	savedGameTime = saveGameData.gameTime;

	if (version >= VERSION_12)
	{
		mission.startTime = saveGameData.missionTime;
		if (saveGameData.saveKey & SAVEKEY_ONMISSION)
		{
			saveGameOnMission = true;
		}
		else
		{
			saveGameOnMission = false;
		}

	}
	else
	{
		saveGameOnMission = false;
	}
	//set the scroll varaibles
	startX = saveGameData.ScrollMinX;
	startY = saveGameData.ScrollMinY;
	width = saveGameData.ScrollMaxX - saveGameData.ScrollMinX;
	height = saveGameData.ScrollMaxY - saveGameData.ScrollMinY;
	gameType = saveGameData.GameType;

	if (version >= VERSION_11)
	{
		//camera position
		disp3d_setView(&saveGameData.currentPlayerPos);
	}
	else
	{
		disp3d_resetView();
	}

	//load mission data from save game these values reloaded after load game

	if (version >= VERSION_14)
	{
		//mission data
		mission.time     = saveGameData.missionOffTime;
		mission.ETA      = saveGameData.missionETA;
		mission.homeLZ_X = saveGameData.missionHomeLZ_X;
		mission.homeLZ_Y = saveGameData.missionHomeLZ_Y;
		mission.playerX  = saveGameData.missionPlayerX;
		mission.playerY  = saveGameData.missionPlayerY;

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			mission.iTranspEntryTileX[player]	= saveGameData.iTranspEntryTileX[player];
			mission.iTranspEntryTileY[player]	= saveGameData.iTranspEntryTileY[player];
			mission.iTranspExitTileX[player]	= saveGameData.iTranspExitTileX[player];
			mission.iTranspExitTileY[player]	= saveGameData.iTranspExitTileY[player];
			aDefaultSensor[player]				= saveGameData.aDefaultSensor[player];
			aDefaultECM[player]					= saveGameData.aDefaultECM[player];
			aDefaultRepair[player]				= saveGameData.aDefaultRepair[player];
		}
	}

	if (version >= VERSION_15)
	{
		offWorldKeepLists	= saveGameData.offWorldKeepLists;
		setRubbleTile(saveGameData.RubbleTile);
		setUnderwaterTile(saveGameData.WaterTile);
	}

	if (version >= VERSION_17)
	{
		unsynchObjID = (saveGameData.objId + 1)/2;  // Make new object ID start at savedObjId*8.
		synchObjID   = saveGameData.objId*4;        // Make new object ID start at savedObjId*8.
		savedObjId = saveGameData.objId;
	}

	if (version >= VERSION_19)//version 19
	{
		for(i=0; i<MAX_PLAYERS; i++)
		{
			for(j=0; j<MAX_PLAYERS; j++)
			{
				alliances[i][j] = saveGameData.alliances[i][j];
			}
		}
		for(i=0; i<MAX_PLAYERS; i++)
		{
			setPlayerColour(i,saveGameData.playerColour[i]);
		}
		SetRadarZoom(saveGameData.radarZoom);
	}

	if (version >= VERSION_20)//version 20
	{
		setDroidsToSafetyFlag(saveGameData.bDroidsToSafetyFlag);
		for (i = 0; i < MAX_PLAYERS; ++i)
		{
			memcpy(&asVTOLReturnPos[i], &saveGameData.asVTOLReturnPos[i], sizeof(Vector2i));
		}
	}

	if (version >= VERSION_22)//version 22
	{
		for (i = 0; i < MAX_PLAYERS; ++i)
		{
			memcpy(&asRunData[i], &saveGameData.asRunData[i], sizeof(RUN_DATA));
		}
	}

	if (saveGameVersion >= VERSION_24)//V24
	{
		missionSetReinforcementTime(saveGameData.reinforceTime);

		// horrible hack to catch savegames that were saving garbage into these fields
		if (saveGameData.bPlayCountDown <= 1)
		{
			setPlayCountDown(saveGameData.bPlayCountDown);
		}
		if (saveGameData.bPlayerHasWon <= 1)
		{
			setPlayerHasWon(saveGameData.bPlayerHasWon);
		}
		if (saveGameData.bPlayerHasLost <= 1)
		{
			setPlayerHasLost(saveGameData.bPlayerHasLost);
		}
	}

	if (saveGameVersion >= VERSION_29)
	{
		mission.scrollMinX = saveGameData.missionScrollMinX;
		mission.scrollMinY = saveGameData.missionScrollMinY;
		mission.scrollMaxX = saveGameData.missionScrollMaxX;
		mission.scrollMaxY = saveGameData.missionScrollMaxY;
	}

	if (saveGameVersion >= VERSION_30)
	{
		scrGameLevel = saveGameData.scrGameLevel;
		bExtraVictoryFlag = saveGameData.bExtraVictoryFlag;
		bExtraFailFlag = saveGameData.bExtraFailFlag;
		bTrackTransporter = saveGameData.bTrackTransporter;
	}

	if (saveGameVersion >= VERSION_31)
	{
		mission.cheatTime = saveGameData.missionCheatTime;
	}

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (i = 0; i < MAX_RECYCLED_DROIDS; ++i)
		{
			aDroidExperience[player][i]	= 0;//clear experience before
		}
	}

	//set IsScenario to true if not a user saved game
	if ((gameType == GTYPE_SAVE_START) ||
	    (gameType == GTYPE_SAVE_MIDMISSION))
	{
		for (i = 0; i < MAX_PLAYERS; ++i)
		{
			powerSaved[i].currentPower = saveGameData.power[i].currentPower;
			powerSaved[i].extractedPower = saveGameData.power[i].extractedPower;
		}

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			for (i = 0; i < MAX_RECYCLED_DROIDS; ++i)
			{
				aDroidExperience[player][i] = 0;//clear experience before building saved units
			}
			NetPlay.players[player].ai = saveGameData.sNetPlay.players[player].ai;
			NetPlay.players[player].difficulty = saveGameData.sNetPlay.players[player].difficulty;
			strcpy(NetPlay.players[player].name, saveGameData.sNetPlay.players[player].name);
			if ((saveGameData.sGame.skDiff[player] == UBYTE_MAX || game.type == CAMPAIGN) && player == 0)
			{
				NetPlay.players[player].allocated = true;
			}
			else
			{
				NetPlay.players[player].allocated = false;
			}
		}

		IsScenario = false;
		//copy the level name across
		sstrcpy(aLevelName, saveGameData.levelName);
		//load up the level dataset
		if (!levLoadData(aLevelName, saveGameName, (GAME_TYPE)gameType))
		{
			return false;
		}

		if (saveGameVersion >= VERSION_33)
		{
			PLAYERSTATS		playerStats;
			bool scav = game.scavengers; // loaded earlier, keep it over struct copy below

			bMultiPlayer	= saveGameData.multiPlayer;
			bMultiMessages	= bMultiPlayer;
			productionPlayer = selectedPlayer;
			game			= saveGameData.sGame;  // why do this again????
			game.scavengers = scav;
			cmdDroidMultiExpBoost(true);
			NetPlay.bComms = (saveGameData.sNetPlay).bComms;
			if(bMultiPlayer)
			{
				loadMultiStats(saveGameData.sPName,&playerStats);				// stats stuff
				setMultiStats(selectedPlayer, playerStats, false);
				setMultiStats(selectedPlayer, playerStats, true);
			}
		}
	}
	else
	{
		IsScenario = true;
	}

	/* Get human and AI players names */
	if (saveGameVersion >= VERSION_34)
	{
		for(i=0;i<MAX_PLAYERS;i++)
			(void)setPlayerName(i, saveGameData.sPlayerName[i]);
	}

	clearPlayerPower();
	//don't adjust any power if a camStart (gameType is set to GTYPE_SCENARIO_START when a camChange saveGame is loaded)
	if (gameType != GTYPE_SCENARIO_START)
	{
		//set the players power
		for (i = 0; i < MAX_PLAYERS; ++i)
		{
			//only overwrite selectedPlayer's power on a startMission save game
			if (gameType == GTYPE_SAVE_MIDMISSION || i == selectedPlayer)
			{
				setPower(i, powerSaved[i].currentPower);
			}
		}
	}
	radarPermitted = (bool)powerSaved[0].extractedPower; // nice hack, eh? don't want to break savegames now...
	allowDesign = (bool)powerSaved[1].extractedPower; // nice hack, eh? don't want to break savegames now...

	return true;
}


// -----------------------------------------------------------------------------------------
/*
Writes the game specifics to a file
*/
static bool writeGameFile(const char* fileName, SDWORD saveType)
{
	GAME_SAVEHEADER fileHeader;
	SAVE_GAME       saveGame;
	bool            status;
	unsigned int    i, j;

	PHYSFS_file* fileHandle = openSaveFile(fileName);
	if (!fileHandle)
	{
		debug(LOG_ERROR, "writeGameFile: openSaveFile(\"%s\") failed", fileName);
		return false;
	}

	fileHeader.aFileType[0] = 'g';
	fileHeader.aFileType[1] = 'a';
	fileHeader.aFileType[2] = 'm';
	fileHeader.aFileType[3] = 'e';

	fileHeader.version = CURRENT_VERSION_NUM;

	debug(LOG_SAVE, "fileversion is %u, (%s) ", fileHeader.version, fileName);

	if (!serializeSaveGameHeader(fileHandle, &fileHeader))
	{
		debug(LOG_ERROR, "game.c:writeGameFile: could not write header to %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return false;
	}

	ASSERT( saveType == GTYPE_SAVE_START ||
			saveType == GTYPE_SAVE_MIDMISSION,
			"writeGameFile: invalid save type" );

	// saveKeymissionIsOffworld
	saveGame.saveKey = getCampaignNumber();
	if (missionIsOffworld())
	{
		saveGame.saveKey |= SAVEKEY_ONMISSION;
		saveGameOnMission = true;
	}
	else
	{
		saveGameOnMission = false;
	}


	/* Put the save game data into the buffer */
	saveGame.gameTime = gameTime;
	saveGame.missionTime = mission.startTime;

	//put in the scroll data
	saveGame.ScrollMinX = scrollMinX;
	saveGame.ScrollMinY = scrollMinY;
	saveGame.ScrollMaxX = scrollMaxX;
	saveGame.ScrollMaxY = scrollMaxY;

	saveGame.GameType = saveType;

	//save the current level so we can load up the STARTING point of the mission
	ASSERT_OR_RETURN(false, strlen(aLevelName) < MAX_LEVEL_SIZE, "Unable to save level name - too long (max %d) - %s",
	                 (int)MAX_LEVEL_SIZE, aLevelName);
	sstrcpy(saveGame.levelName, aLevelName);

	//save out the players power
	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		saveGame.power[i].currentPower = getPower(i);
	}
	saveGame.power[0].extractedPower = radarPermitted; // hideous hack, don't want to break savegames now...
	saveGame.power[1].extractedPower = allowDesign; // hideous hack, don't want to break savegames now...

	//camera position
	disp3d_getView(&(saveGame.currentPlayerPos));

	//mission data
	saveGame.missionOffTime =		mission.time;
	saveGame.missionETA =			mission.ETA;
	saveGame.missionCheatTime =		mission.cheatTime;
	saveGame.missionHomeLZ_X =		mission.homeLZ_X;
	saveGame.missionHomeLZ_Y =		mission.homeLZ_Y;
	saveGame.missionPlayerX =		mission.playerX;
	saveGame.missionPlayerY =		mission.playerY;
	saveGame.missionScrollMinX =     (UWORD)mission.scrollMinX;
	saveGame.missionScrollMinY =     (UWORD)mission.scrollMinY;
	saveGame.missionScrollMaxX =     (UWORD)mission.scrollMaxX;
	saveGame.missionScrollMaxY =     (UWORD)mission.scrollMaxY;

	saveGame.offWorldKeepLists = offWorldKeepLists;
	saveGame.RubbleTile	= getRubbleTileNum();
	saveGame.WaterTile	= getWaterTileNum();

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		saveGame.iTranspEntryTileX[i] = mission.iTranspEntryTileX[i];
		saveGame.iTranspEntryTileY[i] = mission.iTranspEntryTileY[i];
		saveGame.iTranspExitTileX[i]  = mission.iTranspExitTileX[i];
		saveGame.iTranspExitTileY[i]  = mission.iTranspExitTileY[i];
		saveGame.aDefaultSensor[i]    = aDefaultSensor[i];
		saveGame.aDefaultECM[i]       = aDefaultECM[i];
		saveGame.aDefaultRepair[i]    = aDefaultRepair[i];

		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			saveGame.awDroidExperience[i][j]	= aDroidExperience[i][j];
		}
	}

	for (i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		LANDING_ZONE* psLandingZone = getLandingZone(i);
		saveGame.sLandingZone[i].x1	= psLandingZone->x1; // in case struct changes
		saveGame.sLandingZone[i].x2	= psLandingZone->x2;
		saveGame.sLandingZone[i].y1	= psLandingZone->y1;
		saveGame.sLandingZone[i].y2	= psLandingZone->y2;
	}

	//version 17
	saveGame.objId = MAX(unsynchObjID*2, (synchObjID + 3)/4);

	//version 18
	memset(saveGame.buildDate, 0, sizeof(saveGame.buildDate));
	saveGame.oldestVersion = 0;
	saveGame.validityKey = 0;

	//version 19
	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(j=0; j<MAX_PLAYERS; j++)
		{
			saveGame.alliances[i][j] = alliances[i][j];
		}
	}
	for(i=0; i<MAX_PLAYERS; i++)
	{
		saveGame.playerColour[i] = getPlayerColour(i);
	}
	saveGame.radarZoom = (UBYTE)GetRadarZoom();

	//version 20
	saveGame.bDroidsToSafetyFlag = (UBYTE)getDroidsToSafetyFlag();
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		memcpy(&saveGame.asVTOLReturnPos[i], &asVTOLReturnPos[i], sizeof(Vector2i));
	}

	//version 22
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		memcpy(&saveGame.asRunData[i], &asRunData[i], sizeof(RUN_DATA));
	}

	//version 24
	saveGame.reinforceTime = missionGetReinforcementTime();
	saveGame.bPlayCountDown = (UBYTE)getPlayCountDown();
	saveGame.bPlayerHasWon =  (UBYTE)testPlayerHasWon();
	saveGame.bPlayerHasLost = (UBYTE)testPlayerHasLost();

	//version 30
	saveGame.scrGameLevel = scrGameLevel;
	saveGame.bExtraFailFlag = (UBYTE)bExtraFailFlag;
	saveGame.bExtraVictoryFlag = (UBYTE)bExtraVictoryFlag;
	saveGame.bTrackTransporter = (UBYTE)bTrackTransporter;

	// version 33
	saveGame.sGame		= game;
	saveGame.savePlayer	= selectedPlayer;
	saveGame.multiPlayer = bMultiPlayer;
	saveGame.sNetPlay	= NetPlay;
	strcpy(saveGame.sPName, getPlayerName(selectedPlayer));
	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		saveGame.sPlayerIndex[i] = i;
	}

	//version 34
	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		strcpy(saveGame.sPlayerName[i], getPlayerName(i));
	}

	//version 38
	sstrcpy(saveGame.modList, getModList());

	status = serializeSaveGameData(fileHandle, &saveGame);

	// Close the file
	PHYSFS_close(fileHandle);

	// Return our success status with writing out the file!
	return status;
}

// -----------------------------------------------------------------------------------------
// Process the droid initialisation file (dinit.bjo). Creates droids for
// the scenario being loaded. This is *NEVER* called for a user save game
//
bool loadSaveDroidInit(char *pFileData, UDWORD filesize)
{
	DROIDINIT_SAVEHEADER		*psHeader;
	SAVE_DROIDINIT *pDroidInit;
	DROID_TEMPLATE *psTemplate;
	DROID *psDroid;
	UDWORD i;
	UDWORD NumberOfSkippedDroids = 0;

	/* Check the file type */
	psHeader = (DROIDINIT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'd' || psHeader->aFileType[1] != 'i' || psHeader->aFileType[2] != 'n' || psHeader->aFileType[3] != 't')
	{
		debug(LOG_ERROR, "Incorrect file type");
		return false;
	}

	/* DROIDINIT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += DROIDINIT_HEADER_SIZE;

	debug(LOG_SAVE, "fileversion is %u ", psHeader->version);

	pDroidInit = (SAVE_DROIDINIT*)pFileData;

	for (i = 0; i < psHeader->quantity; i++)
	{
		/* SAVE_DROIDINIT is OBJECT_SAVE_V19 */
		/* OBJECT_SAVE_V19 */
		endian_udword(&pDroidInit->id);
		endian_udword(&pDroidInit->x);
		endian_udword(&pDroidInit->y);
		endian_udword(&pDroidInit->z);
		endian_udword(&pDroidInit->direction);
		endian_udword(&pDroidInit->player);
		endian_udword(&pDroidInit->burnStart);
		endian_udword(&pDroidInit->burnDamage);

		pDroidInit->player = RemapPlayerNumber(pDroidInit->player);
		if (pDroidInit->player >= MAX_PLAYERS)
		{
			pDroidInit->player = MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
			NumberOfSkippedDroids++;
		}

		psTemplate = getTemplateFromTranslatedNameNoPlayer(pDroidInit->name);
		if (psTemplate == NULL)
		{
			debug(LOG_ERROR, "Unable to find template for %s for player %d", pDroidInit->name, pDroidInit->player);
		}
		else
		{
			psDroid = reallyBuildDroid(psTemplate, Position((pDroidInit->x & ~TILE_MASK) + TILE_UNITS/2, (pDroidInit->y  & ~TILE_MASK) + TILE_UNITS/2, 0), pDroidInit->player, false);
			if (psDroid)
			{
				Vector2i startpos = getPlayerStartPosition(psDroid->player);

				psDroid->id = pDroidInit->id > 0 ? pDroidInit->id : 0xFEDBCA98;	// hack to remove droid id zero
				psDroid->rot.direction = DEG(pDroidInit->direction);
				addDroid(psDroid, apsDroidLists);
				if (psDroid->droidType == DROID_CONSTRUCT && startpos.x == 0 && startpos.y == 0)
				{
					scriptSetStartPos(psDroid->player, psDroid->pos.x, psDroid->pos.y);
				}
			}
			else
			{
				debug(LOG_ERROR, "This droid cannot be built - %s", pDroidInit->name);
				return false;
			}
		}
		pDroidInit++;
	}
	if(NumberOfSkippedDroids)
	{
		debug(LOG_ERROR, "Bad Player number in %d unit(s)... assigned to the last player!", NumberOfSkippedDroids);
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Remaps old player number based on position on map to new owner
static UDWORD RemapPlayerNumber(UDWORD OldNumber)
{
	int i;

	if (game.type == CAMPAIGN)		// don't remap for SP games
	{
		return OldNumber;
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (OldNumber == NetPlay.players[i].position)
		{
			game.mapHasScavengers = game.mapHasScavengers || i == scavengerSlot();
			return i;
		}
	}
	ASSERT(false, "Found no player position for player %d", (int)OldNumber);
	return 0;
}

static int getPlayer(WzConfig &ini)
{
	if (ini.contains("player"))
	{
		QVariant result = ini.value("player");
		if (result.toString().startsWith("scavenger"))
		{
			game.mapHasScavengers = true;
			return scavengerSlot();
		}
		return result.toInt();
	}
	else if (ini.contains("startpos"))
	{
		int position = ini.value("startpos").toInt();
		for (int i = 0; i < game.maxPlayers; i++)
		{
			if (NetPlay.players[i].position == position)
			{
				return i;
			}
		}
	}
	ASSERT(false, "No player info found!");
	return 0;
}

static void setPlayer(WzConfig &ini, int player)
{
	if (scavengerSlot() == player)
	{
		ini.setValue("player", "scavenger");
	}
	else
	{
		ini.setValue("player", player);
	}
}

static bool loadSaveDroidPointers(const QString &pFileName, DROID **ppsCurrentDroidLists)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName.toUtf8().constData());
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID *psDroid;
		int player = getPlayer(ini);
		int id = ini.value("id").toInt();
		ASSERT(id >= 0, "Negative droid ID in %s", pFileName.toUtf8().constData());

		for (psDroid = ppsCurrentDroidLists[player]; psDroid && psDroid->id != id; psDroid = psDroid->psNext)
		{
			if (psDroid->droidType == DROID_TRANSPORTER && psDroid->psGroup != NULL)  // Check for droids in the transporter.
			{
				for (DROID *psTrDroid = psDroid->psGroup->psList; psTrDroid != NULL; psTrDroid = psTrDroid->psGrpNext)
				{
					if (psTrDroid->id == id)
					{
						psDroid = psTrDroid;
						goto foundDroid;
					}
				}
			}
		}
		foundDroid:
		if (!psDroid)
		{
			for (psDroid = mission.apsDroidLists[player]; psDroid && psDroid->id != id; psDroid = psDroid->psNext) {}
			// FIXME
			if (psDroid) debug(LOG_ERROR, "Droid %s (%d) was in wrong file/list (was in %s)...", objInfo(psDroid), id, pFileName.toUtf8().constData());
		}
		ASSERT_OR_RETURN(false, psDroid, "Droid %d not found", id);

		getIniDroidOrder(ini, "order", psDroid->order);
		psDroid->listSize = clip(ini.value("orderList/size", 0).toInt(), 0, 10000);
		psDroid->asOrderList.resize(psDroid->listSize);  // Must resize before setting any orders, and must set in-place, since pointers are updated later.
		for (int i = 0; i < psDroid->listSize; ++i)
		{
			getIniDroidOrder(ini, "orderList/" + QString::number(i), psDroid->asOrderList[i]);
		}
		psDroid->listPendingBegin = 0;
		for (int j = 0; j < DROID_MAXWEAPS; j++)
		{
			objTrace(psDroid->id, "weapon %d, nStat %d", j, psDroid->asWeaps[j].nStat);
			getIniBaseObject(ini, "actionTarget/" + QString::number(j), psDroid->psActionTarget[j]);
		}
		if (ini.contains("baseStruct/id"))
		{
			int tid = ini.value("baseStruct/id", -1).toInt();
			int tplayer = ini.value("baseStruct/player", -1).toInt();
			OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value("baseStruct/type", 0).toInt();
			ASSERT(tid >= 0 && tplayer >= 0, "Bad ID");
			BASE_OBJECT *psObj = getBaseObjFromData(tid, tplayer, ttype);
			ASSERT(psObj, "Failed to find droid base structure");
			ASSERT(psObj->type == OBJ_STRUCTURE, "Droid base structure not a structure");
			setSaveDroidBase(psDroid, (STRUCTURE *)psObj);
		}
		if (ini.contains("commander"))
		{
			int tid = ini.value("commander", -1).toInt();
			DROID *psCommander = (DROID *)getBaseObjFromData(tid, psDroid->player, OBJ_DROID);
			ASSERT(psCommander, "Failed to find droid commander");
			psCommander->psGroup = NULL;
			psCommander->psGrpNext = NULL;
			cmdDroidAddDroid(psCommander, psDroid);
		}
		ini.endGroup();
	}
	return true;
}

static int healthValue(QSettings &ini, int defaultValue)
{
	QString health = ini.value("health").toString();
	if (health.isEmpty() || defaultValue == 0)
	{
		return defaultValue;
	}
	else if (health.contains('%'))
	{
		int perc = health.remove('%').toInt();
		return MAX(defaultValue * perc / 100, 1); //hp not supposed to be 0
	}
	else
	{
		return MIN(health.toInt(), defaultValue);
	}
}

static bool loadSaveDroid(const char *pFileName, DROID **ppsCurrentDroidLists)
{
	if (!PHYSFS_exists(pFileName))
	{
		debug(LOG_SAVE, "No %s found -- use fallback method", pFileName);
		return false;	// try to use fallback method
	}
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	QStringList list = ini.childGroups();
	// Sort list so transports are loaded first, since they must be loaded before the droids they contain.
	std::vector<std::pair<int, QString> > sortedList;
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID_TYPE droidType = (DROID_TYPE)ini.value("droidType").toInt();
		int priority = 0;
		switch (droidType)
		{
			case DROID_TRANSPORTER: ++priority;
			case DROID_COMMAND:     ++priority;
			default:                break;
		}
		sortedList.push_back(std::make_pair(-priority, list[i]));
		ini.endGroup();
	}
	std::sort(sortedList.begin(), sortedList.end());

	for (unsigned i = 0; i < sortedList.size(); ++i)
	{
		ini.beginGroup(sortedList[i].second);
		DROID *psDroid;
		int player = getPlayer(ini);
		int id = ini.value("id").toInt();
		Position pos = ini.vector3i("position");
		Rotation rot = ini.vector3i("rotation");
		Vector2i tmp;
		bool onMission = ini.value("onMission", false).toBool();
		DROID_TEMPLATE templ, *psTemplate = &templ;

		if (ini.contains("template"))
		{
			// Use real template (for maps)
			QString templName(ini.value("template").toString());
			psTemplate = getTemplateFromTranslatedNameNoPlayer(templName.toUtf8().constData());
			if (psTemplate == NULL)
			{
				debug(LOG_ERROR, "Unable to find template for %s for player %d -- unit skipped", templName.toUtf8().constData(), player);
				ini.endGroup();
				continue;
			}
		}
		else
		{
			// Create fake template
			psTemplate->pName = NULL;
			sstrcpy(templ.aName, ini.value("name", "UNKNOWN").toString().toUtf8().constData());
			psTemplate->droidType = (DROID_TYPE)ini.value("droidType").toInt();
			psTemplate->numWeaps = ini.value("weapons", 0).toInt();
			ini.beginGroup("parts");	// the following is copy-pasted from loadSaveTemplate() -- fixme somehow
			psTemplate->asParts[COMP_BODY] = getCompFromName(COMP_BODY, ini.value("body", QString("ZNULLBODY")).toString().toUtf8().constData());
			psTemplate->asParts[COMP_BRAIN] = getCompFromName(COMP_BRAIN, ini.value("brain", QString("ZNULLBRAIN")).toString().toUtf8().constData());
			psTemplate->asParts[COMP_PROPULSION] = getCompFromName(COMP_PROPULSION, ini.value("propulsion", QString("ZNULLPROP")).toString().toUtf8().constData());
			psTemplate->asParts[COMP_REPAIRUNIT] = getCompFromName(COMP_REPAIRUNIT, ini.value("repair", QString("ZNULLREPAIR")).toString().toUtf8().constData());
			psTemplate->asParts[COMP_ECM] = getCompFromName(COMP_ECM, ini.value("ecm", QString("ZNULLECM")).toString().toUtf8().constData());
			psTemplate->asParts[COMP_SENSOR] = getCompFromName(COMP_SENSOR, ini.value("sensor", QString("ZNULLSENSOR")).toString().toUtf8().constData());
			psTemplate->asParts[COMP_CONSTRUCT] = getCompFromName(COMP_CONSTRUCT, ini.value("construct", QString("ZNULLCONSTRUCT")).toString().toUtf8().constData());
			psTemplate->asWeaps[0] = getCompFromName(COMP_WEAPON, ini.value("weapon/1", QString("ZNULLWEAPON")).toString().toUtf8().constData());
			psTemplate->asWeaps[1] = getCompFromName(COMP_WEAPON, ini.value("weapon/2", QString("ZNULLWEAPON")).toString().toUtf8().constData());
			psTemplate->asWeaps[2] = getCompFromName(COMP_WEAPON, ini.value("weapon/3", QString("ZNULLWEAPON")).toString().toUtf8().constData());
			ini.endGroup();
			psTemplate->buildPoints = calcTemplateBuild(psTemplate);
			psTemplate->powerPoints = calcTemplatePower(psTemplate);
		}

		// If droid is on a mission, calling with the saved position might cause an assertion. Or something like that.
		pos.x = clip(pos.x, world_coord(1), world_coord(mapWidth - 1));
		pos.y = clip(pos.y, world_coord(1), world_coord(mapHeight - 1));

		/* Create the Droid */
		turnOffMultiMsg(true);
		psDroid = reallyBuildDroid(psTemplate, pos, player, onMission, rot);
		ASSERT_OR_RETURN(NULL, psDroid != NULL, "Failed to build unit %d", id);
		turnOffMultiMsg(false);

		// Copy the values across
		if (id > 0)
		{
			psDroid->id = id; // force correct ID, unless ID is set to eg -1, in which case we should keep new ID (useful for starting units in campaign)
		}
		ASSERT(id != 0, "Droid ID should never be zero here");
		psDroid->body = healthValue(ini, psDroid->originalBody);
		psDroid->burnDamage = ini.value("burnDamage", 0).toInt();
		psDroid->burnStart = ini.value("burnStart", 0).toInt();
		psDroid->experience = ini.value("experience", 0).toInt();
		psDroid->timeLastHit = ini.value("timeLastHit", UDWORD_MAX).toInt();
		psDroid->secondaryOrder = ini.value("secondaryOrder", DSS_NONE).toInt();
		psDroid->secondaryOrderPending = psDroid->secondaryOrder;
		psDroid->action = (DROID_ACTION)ini.value("action", DACTION_NONE).toInt();
		psDroid->actionPos = ini.vector2i("action/pos");
		psDroid->actionStarted = ini.value("actionStarted", 0).toInt();
		psDroid->actionPoints = ini.value("actionPoints", 0).toInt();
		psDroid->resistance = ini.value("resistance", 0).toInt(); // zero resistance == no electronic damage

		// copy the droid's weapon stats
		for (int j = 0; j < psDroid->numWeaps; j++)
		{
			if (psDroid->asWeaps[j].nStat > 0)
			{
				psDroid->asWeaps[j].ammo = ini.value("ammo/" + QString::number(j)).toInt();
				psDroid->asWeaps[j].lastFired = ini.value("lastFired/" + QString::number(j)).toInt();
				psDroid->asWeaps[j].shotsFired = ini.value("shotsFired/" + QString::number(j)).toInt();
				psDroid->asWeaps[j].rot = ini.vector3i("rotation/" + QString::number(j));
			}
		}

		psDroid->group = ini.value("group", UBYTE_MAX).toInt();
		psDroid->selected = ini.value("selected", false).toBool();
		int aigroup = ini.value("aigroup", -1).toInt();
		if (aigroup >= 0)
		{
			DROID_GROUP *psGroup = grpFind(aigroup);
			psGroup->add(psDroid);
			if (psGroup->type == GT_TRANSPORTER)
			{
				psDroid->selected = false;  // Droid should be visible in the transporter interface.
			}
		}
		else
		{
			psDroid->psGroup = NULL;
		}
		psDroid->died = ini.value("died", 0).toInt();
		psDroid->lastEmission = ini.value("lastEmission", 0).toInt();
		memset(psDroid->visible, 0, sizeof(psDroid->visible));
		for (int j = 0; j < game.maxPlayers; j++)
		{
			psDroid->visible[j] = ini.value("visible/" + QString::number(j), 0).toInt();
		}

		psDroid->sMove.Status = (MOVE_STATUS)ini.value("moveStatus", 0).toInt();
		psDroid->sMove.pathIndex = ini.value("pathIndex", 0).toInt();
		psDroid->sMove.numPoints = ini.value("pathLength", 0).toInt();
		psDroid->sMove.asPath = (Vector2i *)malloc(sizeof(*psDroid->sMove.asPath) * psDroid->sMove.numPoints);
		for (int j = 0; j < psDroid->sMove.numPoints; j++)
		{
			psDroid->sMove.asPath[j] = ini.vector2i("pathNode/" + QString::number(j));
		}
		psDroid->sMove.destination = ini.vector2i("moveDestination");
		psDroid->sMove.src = ini.vector2i("moveSource");
		psDroid->sMove.target = ini.vector2i("moveTarget");
		psDroid->sMove.speed = ini.value("moveSpeed").toInt();
		psDroid->sMove.moveDir = ini.value("moveDirection").toInt();
		psDroid->sMove.bumpDir = ini.value("bumpDir").toInt();
		psDroid->sMove.iVertSpeed = ini.value("vertSpeed").toInt();
		psDroid->sMove.bumpTime = ini.value("bumpTime").toInt();
		psDroid->sMove.shuffleStart = ini.value("shuffleStart").toInt();
		for (int j = 0; j < sizeof(psDroid->sMove.iAttackRuns) / sizeof(psDroid->sMove.iAttackRuns[0]); ++j)
		{
			psDroid->sMove.iAttackRuns[j] = ini.value("attackRun/" + QString::number(j)).toInt();
		}
		psDroid->sMove.lastBump = ini.value("lastBump").toInt();
		psDroid->sMove.pauseTime = ini.value("pauseTime").toInt();
		tmp = ini.vector2i("bumpPosition");
		psDroid->sMove.bumpX = tmp.x;
		psDroid->sMove.bumpY = tmp.y;
		psDroid->born = ini.value("born", 2).toInt();

		// Recreate path-finding jobs
		if (psDroid->sMove.Status == MOVEWAITROUTE)
		{
			psDroid->sMove.Status = MOVEINACTIVE;
			fpathDroidRoute(psDroid, psDroid->sMove.destination.x, psDroid->sMove.destination.y, FMT_MOVE);
			psDroid->sMove.Status = MOVEWAITROUTE;

			// Droid might be on a mission, so finish pathfinding now, in case pointers swap and map size changes.
			FPATH_RETVAL dr = fpathDroidRoute(psDroid, psDroid->sMove.destination.x, psDroid->sMove.destination.y, FMT_MOVE);
			if (dr == FPR_OK)
			{
				psDroid->sMove.Status = MOVENAVIGATE;
				psDroid->sMove.pathIndex = 0;
			}
			else // if (retVal == FPR_FAILED)
			{
				psDroid->sMove.Status = MOVEINACTIVE;
				actionDroid(psDroid, DACTION_SULK);
			}
			ASSERT(dr != FPR_WAIT, " ");
		}

		// HACK!!
		Vector2i startpos = getPlayerStartPosition(player);
		if (psDroid->droidType == DROID_CONSTRUCT && startpos.x == 0 && startpos.y == 0)
		{
			scriptSetStartPos(psDroid->player, psDroid->pos.x, psDroid->pos.y);	// set map start position, FIXME - save properly elsewhere!
		}

		if (psDroid->psGroup == NULL || psDroid->psGroup->type != GT_TRANSPORTER || psDroid->droidType == DROID_TRANSPORTER)  // do not add to list if on a transport, then the group list is used instead
		{
			addDroid(psDroid, ppsCurrentDroidLists);
		}

		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of droids for each player to a file
*/
static bool writeDroid(WzConfig &ini, DROID *psCurr, bool onMission, int &counter)
{
	ini.beginGroup("droid_" + QString("%1").arg(counter++, 10, 10, QLatin1Char('0')));  // Zero padded so that alphabetical sort works.
	ini.setValue("id", psCurr->id);
	setPlayer(ini, psCurr->player);
	ini.setValue("name", psCurr->aName);
	ini.setVector3i("position", psCurr->pos);
	ini.setVector3i("rotation", psCurr->rot);
	ini.setValue("health", psCurr->body);
	for (int i = 0; i < psCurr->numWeaps; i++)
	{
		if (psCurr->asWeaps[i].nStat > 0)
		{
			ini.setValue("ammo/" + QString::number(i), psCurr->asWeaps[i].ammo);
			ini.setValue("lastFired/" + QString::number(i), psCurr->asWeaps[i].lastFired);
			ini.setValue("shotsFired/" + QString::number(i), psCurr->asWeaps[i].shotsFired);
			ini.setVector3i("rotation/" + QString::number(i), psCurr->asWeaps[i].rot);
		}
	}
	for (int i = 0; i < DROID_MAXWEAPS; i++)
	{
		setIniBaseObject(ini, "actionTarget/" + QString::number(i), psCurr->psActionTarget[i]);
	}
	ini.setValue("born", psCurr->born);
	if (psCurr->lastFrustratedTime > 0) ini.setValue("lastFrustratedTime", psCurr->lastFrustratedTime);
	if (psCurr->experience > 0) ini.setValue("experience", psCurr->experience);
	setIniDroidOrder(ini, "order", psCurr->order);
	ini.setValue("orderList/size", psCurr->listSize);
	for (int i = 0; i < psCurr->listSize; ++i)
	{
		setIniDroidOrder(ini, "orderList/" + QString::number(i), psCurr->asOrderList[i]);
	}
	if (psCurr->timeLastHit != UDWORD_MAX) ini.setValue("timeLastHit", psCurr->timeLastHit);
	ini.setValue("secondaryOrder", psCurr->secondaryOrder);
	ini.setValue("action", psCurr->action);
	ini.setVector2i("action/pos", psCurr->actionPos);
	ini.setValue("actionStarted", psCurr->actionStarted);
	ini.setValue("actionPoints", psCurr->actionPoints);
	if (psCurr->psBaseStruct != NULL)
	{
		ini.setValue("baseStruct/id", psCurr->psBaseStruct->id);
		ini.setValue("baseStruct/player", psCurr->psBaseStruct->player);	// always ours, but for completeness
		ini.setValue("baseStruct/type", psCurr->psBaseStruct->type);		// always a building, but for completeness
	}
	if (psCurr->psGroup)
	{
		ini.setValue("aigroup", psCurr->psGroup->id);	// AI and commander/transport group
		ini.setValue("aigroup/type", psCurr->psGroup->type);
	}
	ini.setValue("group", psCurr->group);	// different kind of group. of course.
	ini.setValue("selected", psCurr->selected);	// third kind of group
	if (psCurr->lastEmission)
	{
		ini.setValue("lastEmission", psCurr->lastEmission);
	}
	for (int i = 0; i < game.maxPlayers; i++)
	{
		ini.setValue("visible/" + QString::number(i), psCurr->visible[i]);
	}
	if (hasCommander(psCurr) && psCurr->psGroup->psCommander->died <= 1)
	{
		ini.setValue("commander", psCurr->psGroup->psCommander->id);
	}
	if (psCurr->died > 0) ini.setValue("died", psCurr->died);
	if (psCurr->resistance > 0) ini.setValue("resistance", psCurr->resistance);
	if (psCurr->burnStart > 0) ini.setValue("burnStart", psCurr->burnStart);
	if (psCurr->burnDamage > 0) ini.setValue("burnDamage", psCurr->burnDamage);
	ini.setValue("droidType", psCurr->droidType);
	ini.setValue("weapons", psCurr->numWeaps);
	ini.setValue("parts/body", (asBodyStats + psCurr->asBits[COMP_BODY].nStat)->pName);
	ini.setValue("parts/propulsion", (asPropulsionStats + psCurr->asBits[COMP_PROPULSION].nStat)->pName);
	ini.setValue("parts/brain", (asBrainStats + psCurr->asBits[COMP_BRAIN].nStat)->pName);
	ini.setValue("parts/repair", (asRepairStats + psCurr->asBits[COMP_REPAIRUNIT].nStat)->pName);
	ini.setValue("parts/ecm", (asECMStats + psCurr->asBits[COMP_ECM].nStat)->pName);
	ini.setValue("parts/sensor", (asSensorStats + psCurr->asBits[COMP_SENSOR].nStat)->pName);
	ini.setValue("parts/construct", (asConstructStats + psCurr->asBits[COMP_CONSTRUCT].nStat)->pName);
	for (int j = 0; j < psCurr->numWeaps; j++)
	{
		ini.setValue("parts/weapon/" + QString::number(j + 1), (asWeaponStats + psCurr->asWeaps[j].nStat)->pName);
	}
	ini.setValue("moveStatus", psCurr->sMove.Status);
	ini.setValue("pathIndex", psCurr->sMove.pathIndex);
	ini.setValue("pathLength", psCurr->sMove.numPoints);
	for (int i = 0; i < psCurr->sMove.numPoints; i++)
	{
		ini.setVector2i("pathNode/" + QString::number(i), psCurr->sMove.asPath[i]);
	}
	ini.setVector2i("moveDestination", psCurr->sMove.destination);
	ini.setVector2i("moveSource", psCurr->sMove.src);
	ini.setVector2i("moveTarget", psCurr->sMove.target);
	ini.setValue("moveSpeed", psCurr->sMove.speed);
	ini.setValue("moveDirection", psCurr->sMove.moveDir);
	ini.setValue("bumpDir", psCurr->sMove.bumpDir);
	ini.setValue("vertSpeed", psCurr->sMove.iVertSpeed);
	ini.setValue("bumpTime", psCurr->sMove.bumpTime);
	ini.setValue("shuffleStart", psCurr->sMove.shuffleStart);
	for (int i = 0; i < sizeof(psCurr->sMove.iAttackRuns) / sizeof(psCurr->sMove.iAttackRuns[0]); ++i)
	{
		ini.setValue("attackRun/" + QString::number(i), psCurr->sMove.iAttackRuns[i]);
	}
	ini.setValue("lastBump", psCurr->sMove.lastBump);
	ini.setValue("pauseTime", psCurr->sMove.pauseTime);
	ini.setVector2i("bumpPosition", Vector2i(psCurr->sMove.bumpX, psCurr->sMove.bumpY));
	ini.setValue("onMission", onMission);
	ini.endGroup();
	return true;
}

static bool writeDroidFile(const char *pFileName, DROID **ppsCurrentDroidLists)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	int counter = 0;
	bool onMission = (ppsCurrentDroidLists[0] == mission.apsDroidLists[0]);
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		for (DROID *psCurr = ppsCurrentDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			writeDroid(ini, psCurr, onMission, counter);
			if (psCurr->droidType == DROID_TRANSPORTER)	// if transporter save any droids in the grp
			{
				for (DROID *psTrans = psCurr->psGroup->psList; psTrans != NULL; psTrans = psTrans->psGrpNext)
				{
					if (psTrans != psCurr)
					{
						writeDroid(ini, psTrans, onMission, counter);
					}
				}
				//always save transporter droids that are in the mission list with an invalid value
				if (ppsCurrentDroidLists[player] == mission.apsDroidLists[player])
				{
					ini.setVector3i("position", Vector3i(-1, -1, -1)); // was INVALID_XY
				}
			}
		}
	}
	return true;
}


// -----------------------------------------------------------------------------------------
bool loadSaveStructure(char *pFileData, UDWORD filesize)
{
	STRUCT_SAVEHEADER		*psHeader;
	SAVE_STRUCTURE_V2		*psSaveStructure, sSaveStructure;
	STRUCTURE			*psStructure;
	STRUCTURE_STATS			*psStats = NULL;
	UDWORD				count, statInc;
	int32_t				found;
	UDWORD				NumberOfSkippedStructures=0;
	UDWORD				burnTime;

	/* Check the file type */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'u')
	{
		debug( LOG_ERROR, "loadSaveStructure: Incorrect file type" );

		return false;
	}

	/* STRUCT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += STRUCT_HEADER_SIZE;

	debug(LOG_SAVE, "file version is %u ", psHeader->version);

	/* Check the file version */
	if (psHeader->version < VERSION_7 || psHeader->version > VERSION_8)
	{
		debug( LOG_ERROR, "StructLoad: unsupported save format version %d", psHeader->version );

		return false;
	}

	psSaveStructure = &sSaveStructure;

	if ((sizeof(SAVE_STRUCTURE_V2) * psHeader->quantity + STRUCT_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "structureLoad: unexpected end of file" );
		return false;
	}

	/* Load in the structure data */
	for (count = 0; count < psHeader->quantity; count ++, pFileData += sizeof(SAVE_STRUCTURE_V2))
	{
		memcpy(psSaveStructure, pFileData, sizeof(SAVE_STRUCTURE_V2));

		/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
		endian_sdword(&psSaveStructure->currentBuildPts);
		endian_udword(&psSaveStructure->body);
		endian_udword(&psSaveStructure->armour);
		endian_udword(&psSaveStructure->resistance);
		endian_udword(&psSaveStructure->dummy1);
		endian_udword(&psSaveStructure->subjectInc);
		endian_udword(&psSaveStructure->timeStarted);
		endian_udword(&psSaveStructure->output);
		endian_udword(&psSaveStructure->capacity);
		endian_udword(&psSaveStructure->quantity);
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveStructure->id);
		endian_udword(&psSaveStructure->x);
		endian_udword(&psSaveStructure->y);
		endian_udword(&psSaveStructure->z);
		endian_udword(&psSaveStructure->direction);
		endian_udword(&psSaveStructure->player);
		endian_udword(&psSaveStructure->burnStart);
		endian_udword(&psSaveStructure->burnDamage);

		psSaveStructure->player=RemapPlayerNumber(psSaveStructure->player);

		if (psSaveStructure->player >= MAX_PLAYERS)
		{
			psSaveStructure->player=MAX_PLAYERS-1;
			NumberOfSkippedStructures++;
		}
		//get the stats for this structure
		found = false;

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructure->name))
			{
				found = true;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "This structure no longer exists - %s", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure) );
			//ignore this
			continue;
		}

		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			psStructure = getTileStructure(map_coord(psSaveStructure->x), map_coord(psSaveStructure->y));
			if (psStructure == NULL)
			{
				debug( LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->player );
				//ignore this module
				continue;
			}
		}

		//check not trying to build too near the edge
		if (map_coord(psSaveStructure->x) < TOO_NEAR_EDGE || map_coord(psSaveStructure->x) > mapWidth - TOO_NEAR_EDGE)
		{
			debug( LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			//ignore this
			continue;
		}
		if (map_coord(psSaveStructure->y) < TOO_NEAR_EDGE || map_coord(psSaveStructure->y) > mapHeight - TOO_NEAR_EDGE)
		{
			debug( LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			//ignore this
			continue;
		}

		psStructure = buildStructureDir(psStats, psSaveStructure->x, psSaveStructure->y, DEG(psSaveStructure->direction), psSaveStructure->player, true);
		ASSERT(psStructure, "Unable to create structure");
		if (!psStructure) continue;
		// The original code here didn't work and so the scriptwriters worked round it by using the module ID - so making it work now will screw up
		// the scripts -so in ALL CASES overwrite the ID!
		psStructure->id = psSaveStructure->id > 0 ? psSaveStructure->id : 0xFEDBCA98; // hack to remove struct id zero
		psStructure->burnDamage = psSaveStructure->burnDamage;
		burnTime = psSaveStructure->burnStart;
		psStructure->burnStart = burnTime;
		psStructure->status = (STRUCT_STATES)psSaveStructure->status;
		if (psStructure->status == SS_BUILT)
		{
			buildingComplete(psStructure);
		}
		if (psStructure->pStructureType->type == REF_HQ)
		{
			scriptSetStartPos(psSaveStructure->player, psStructure->pos.x, psStructure->pos.y);
		}
		else if (psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			scriptSetDerrickPos(psStructure->pos.x, psStructure->pos.y);
		}
	}

	if (NumberOfSkippedStructures>0)
	{
		debug( LOG_ERROR, "structureLoad: invalid player number in %d structures ... assigned to the last player!\n\n", NumberOfSkippedStructures );
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//return id of a research topic based on the name
static UDWORD getResearchIdFromName(const char *pName)
{
	for (int inc = 0; inc < asResearch.size(); inc++)
	{
		if (!strcmp(asResearch[inc].pName, pName))
		{
			return inc;
		}
	}
	debug(LOG_ERROR, "Unknown research - %s", pName);
	return NULL_ID;
}

// -----------------------------------------------------------------------------------------
/* code for versions after version 20 of a save structure */
static bool loadSaveStructure2(const char *pFileName, STRUCTURE **ppList)
{
	if (!PHYSFS_exists(pFileName))
	{
		debug(LOG_SAVE, "No %s found -- use fallback method", pFileName);
		return false;	// try to use fallback method
	}
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		FACTORY *psFactory;
		RESEARCH_FACILITY *psResearch;
		REPAIR_FACILITY *psRepair;
		REARM_PAD *psReArmPad;
		STRUCTURE_STATS *psStats = NULL, *psModule;
		int statInc, capacity, found, researchId;
		STRUCTURE *psStructure;

		ini.beginGroup(list[i]);
		int player = getPlayer(ini);
		int id = ini.value("id").toInt();
		Position pos = ini.vector3i("position");
		Rotation rot = ini.vector3i("rotation");
		QString name = ini.value("name").toString();

		//get the stats for this structure
		found = false;
		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name
			if (name.compare(psStats->pName) == 0)
			{
				found = true;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		ASSERT(found, "This structure no longer exists - %s", name.toUtf8().constData());
		if (!found)
		{
			ini.endGroup();
			continue;	// ignore this
		}
		/*create the Structure */
		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			STRUCTURE *psStructure = getTileStructure(map_coord(pos.x), map_coord(pos.y));
			if (psStructure == NULL)
			{
				debug(LOG_ERROR, "No owning structure for module - %s for player - %d", name.toUtf8().constData(), player);
				ini.endGroup();
				continue; // ignore this module
			}
		}
		//check not trying to build too near the edge
		if (map_coord(pos.x) < TOO_NEAR_EDGE || map_coord(pos.x) > mapWidth - TOO_NEAR_EDGE
		    || map_coord(pos.y) < TOO_NEAR_EDGE || map_coord(pos.y) > mapHeight - TOO_NEAR_EDGE)
		{
			debug(LOG_ERROR, "Structure %s, coord too near the edge of the map. id - %d", name.toUtf8().constData(), id);
			ini.endGroup();
			continue; // skip it
		}
		psStructure = buildStructureDir(psStats, pos.x, pos.y, rot.direction, player, true);
		ASSERT(psStructure, "Unable to create structure");
		if (!psStructure)
		{
			ini.endGroup();
			continue;
		}
		if (id > 0)
		{
			psStructure->id = id;	// force correct ID
		}
		psStructure->burnDamage = ini.value("burnDamage", 0).toInt();
		psStructure->burnStart = ini.value("burnStart", 0).toInt();
		memset(psStructure->visible, 0, sizeof(psStructure->visible));
		for (int i = 0; i < game.maxPlayers; i++)
		{
			psStructure->visible[i] = ini.value("visible/" + QString::number(i), 0).toInt();
		}
		if (psStructure->pStructureType->type == REF_HQ)
		{
			scriptSetStartPos(player, psStructure->pos.x, psStructure->pos.y);
		}
		psStructure->resistance = ini.value("resistance", psStructure->resistance).toInt();
		capacity = ini.value("modules", 0).toInt();
		switch (psStructure->pStructureType->type)
		{
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
		case REF_CYBORG_FACTORY:
			//if factory save the current build info
			psFactory = ((FACTORY *)psStructure->pFunctionality);
			psFactory->capacity = 0; // increased when built
			psFactory->productionLoops = ini.value("Factory/productionLoops", psFactory->productionLoops).toInt();
			psFactory->timeStarted = ini.value("Factory/timeStarted", psFactory->timeStarted).toInt();
			psFactory->buildPointsRemaining = ini.value("Factory/buildPointsRemaining", psFactory->buildPointsRemaining).toInt();
			psFactory->timeStartHold = ini.value("Factory/timeStartHold", psFactory->timeStartHold).toInt();
			psFactory->loopsPerformed = ini.value("Factory/loopsPerformed", psFactory->loopsPerformed).toInt();
			psFactory->productionOutput = ini.value("Factory/productionOutput", psFactory->productionOutput).toInt();
			// statusPending and pendingCount belong to the GUI, not the game state.
			psFactory->secondaryOrder = ini.value("Factory/secondaryOrder", psFactory->secondaryOrder).toInt();
			//adjust the module structures IMD
			if (capacity)
			{
				psModule = getModuleStat(psStructure);
				//build the appropriate number of modules
				for (int i = 0; i < capacity; i++)
				{
					buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
				}
			}
			if (ini.contains("Factory/template"))
			{
				int templId(ini.value("Factory/template").toInt());
				psFactory->psSubject = getTemplateFromMultiPlayerID(templId);
			}
			if (ini.contains("Factory/assemblyPoint/pos"))
			{
				Position point = ini.vector3i("Factory/assemblyPoint/pos");
				setAssemblyPoint(psFactory->psAssemblyPoint, point.x, point.y, player, true);
				psFactory->psAssemblyPoint->selected = ini.value("Factory/assemblyPoint/selected", false).toBool();
			}
			for (int runNum = 0; runNum < ini.value("Factory/productionRuns", 0).toInt(); runNum++)
			{
				ProductionRunEntry currentProd;
				currentProd.quantity = ini.value("Factory/Run/" + QString::number(runNum) + "/quantity").toInt();
				currentProd.built = ini.value("Factory/Run/" + QString::number(runNum) + "/built").toInt();
				if (ini.contains("Factory/Run/" + QString::number(runNum) + "/template"))
				{
					int tid = ini.value("Factory/Run/" + QString::number(runNum) + "/template").toInt();
					DROID_TEMPLATE *psTempl = getTemplateFromMultiPlayerID(tid);
					currentProd.psTemplate = psTempl;
					ASSERT(psTempl, "No template found for template ID %d for %s (%d)", tid, objInfo(psStructure), id);
				}
				if (psFactory->psAssemblyPoint->factoryInc >= asProductionRun[psFactory->psAssemblyPoint->factoryType].size())
				{
					asProductionRun[psFactory->psAssemblyPoint->factoryType].resize(psFactory->psAssemblyPoint->factoryInc + 1);
				}
				asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc].push_back(currentProd);
			}
			break;
		case REF_RESEARCH:
			psResearch = ((RESEARCH_FACILITY *)psStructure->pFunctionality);
			psResearch->capacity = 0; // increased when built
			//adjust the module structures IMD
			if (capacity)
			{
				psModule = getModuleStat(psStructure);
				//build the appropriate number of modules
				buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
			}
			//clear subject
			psResearch->psSubject = NULL;
			psResearch->timeStartHold = 0;
			//set the subject
			if (ini.contains("Research/target"))
			{
				researchId = getResearchIdFromName(ini.value("Research/target").toString().toUtf8().constData());
				if (researchId != NULL_ID)
				{
					psResearch->psSubject = &asResearch[researchId];
					psResearch->timeStartHold = ini.value("Research/timeStartHold").toInt();
				}
				else
				{
					debug(LOG_ERROR, "Failed to look up research target %s", ini.value("Research/target").toString().toUtf8().constData());
				}
			}
			break;
		case REF_POWER_GEN:
			// adjust the module structures IMD
			if (capacity)
			{
				psModule = getModuleStat(psStructure);
				buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
			}
			break;
		case REF_RESOURCE_EXTRACTOR:
			break;
		case REF_REPAIR_FACILITY:
			psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);
			psRepair->power = ((REPAIR_DROID_FUNCTION *) psStructure->pStructureType->asFuncList[0])->repairPoints;
			if (ini.contains("Repair/deliveryPoint/pos"))
			{
				Position point = ini.vector3i("Repair/deliveryPoint/pos");
				setAssemblyPoint(psRepair->psDeliveryPoint, point.x, point.y, player, true);
				psRepair->psDeliveryPoint->selected = ini.value("Repair/deliveryPoint/selected", false).toBool();
			}
			break;
		case REF_REARM_PAD:
			psReArmPad = ((REARM_PAD *)psStructure->pFunctionality);
			psReArmPad->reArmPoints = ini.value("Rearm/reArmPoints").toInt();
			psReArmPad->timeStarted = ini.value("Rearm/timeStarted").toInt();
			psReArmPad->timeLastUpdated = ini.value("Rearm/timeLastUpdated").toInt();
			break;
		case REF_WALL:
		case REF_GATE:
			psStructure->pFunctionality->wall.type = ini.value("Wall/type").toInt();
			psStructure->sDisplay.imd = psStructure->pStructureType->pIMD[std::min<unsigned>(psStructure->pFunctionality->wall.type, psStructure->pStructureType->pIMD.size() - 1)];
			break;
		default:
			break;
		}
		psStructure->body = healthValue(ini, structureBody(psStructure));
		psStructure->currentBuildPts = ini.value("currentBuildPts", psStructure->pStructureType->buildPoints).toInt();
		if (psStructure->status == SS_BUILT)
		{
			switch (psStructure->pStructureType->type)
			{
			case REF_POWER_GEN:
				checkForResExtractors(psStructure);
				if(selectedPlayer == psStructure->player)
				{
					audio_PlayObjStaticTrack(psStructure, ID_SOUND_POWER_HUM);
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				checkForPowerGen(psStructure);
				/* GJ HACK! - add anim to deriks */
				if (psStructure->psCurAnim == NULL)
				{
					psStructure->psCurAnim = animObj_Add(psStructure, ID_ANIM_DERIK, 0, 0);
				}
				break;
			default:
				//do nothing for factories etc
				break;
			}
		}
		// weapons
		for (int j = 0; j < psStructure->pStructureType->numWeaps; j++)
		{
			if (psStructure->asWeaps[j].nStat > 0)
			{
				psStructure->asWeaps[j].ammo = ini.value("ammo/" + QString::number(j)).toInt();
				psStructure->asWeaps[j].lastFired = ini.value("lastFired/" + QString::number(j)).toInt();
				psStructure->asWeaps[j].shotsFired = ini.value("shotsFired/" + QString::number(j)).toInt();
				psStructure->asWeaps[j].rot = ini.vector3i("rotation/" + QString::number(j));
			}
		}
		psStructure->selected = ini.value("selected", false).toBool();
		psStructure->died = ini.value("died", 0).toInt();
		psStructure->lastEmission = ini.value("lastEmission", 0).toInt();
		psStructure->timeLastHit = ini.value("timeLastHit", UDWORD_MAX).toInt();
		psStructure->status = (STRUCT_STATES)ini.value("status", SS_BUILT).toInt();
		if (psStructure->status == SS_BUILT)
		{
			buildingComplete(psStructure);
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of structure for each player to a file
*/
bool writeStructFile(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	int counter = 0;
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		for (STRUCTURE *psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			ini.beginGroup("structure_" + QString("%1").arg(counter++, 10, 10, QLatin1Char('0')));  // Zero padded so that alphabetical sort works.
			ini.setValue("id", psCurr->id);
			setPlayer(ini, psCurr->player);
			ini.setValue("name", psCurr->pStructureType->pName);
			ini.setVector3i("position", psCurr->pos);
			ini.setVector3i("rotation", psCurr->rot);
			ini.setValue("health", psCurr->body);
			ini.setValue("born", psCurr->born);
			if (psCurr->timeLastHit != UDWORD_MAX) ini.setValue("timeLastHit", psCurr->timeLastHit);
			if (psCurr->selected) ini.setValue("selected", psCurr->selected);
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				if (psCurr->visible[i]) ini.setValue("visible/" + QString::number(i), psCurr->visible[i]);
			}
			if (psCurr->died > 0) ini.setValue("died", psCurr->died);
			if (psCurr->resistance > 0) ini.setValue("resistance", psCurr->resistance);
			if (psCurr->burnStart > 0) ini.setValue("burnStart", psCurr->burnStart);
			if (psCurr->burnDamage > 0) ini.setValue("burnDamage", psCurr->burnDamage);
			if (psCurr->status != SS_BUILT) ini.setValue("status", psCurr->status);
			ini.setValue("weapons", psCurr->numWeaps);
			for (int j = 0; j < psCurr->numWeaps; j++)
			{
				ini.setValue("parts/weapon/" + QString::number(j + 1), (asWeaponStats + psCurr->asWeaps[j].nStat)->pName);
				if (psCurr->asWeaps[j].nStat > 0)
				{
					ini.setValue("ammo/" + QString::number(j), psCurr->asWeaps[j].ammo);
					ini.setValue("lastFired/" + QString::number(j), psCurr->asWeaps[j].lastFired);
					ini.setValue("shotsFired/" + QString::number(j), psCurr->asWeaps[j].shotsFired);
					ini.setVector3i("rotation/" + QString::number(j), psCurr->asWeaps[j].rot);
				}
			}
			for (int i = 0; i < psCurr->numWeaps; i++)
			{
				if (psCurr->psTarget[i])
				{
					ini.setValue("target/" + QString::number(i) + "/id", psCurr->psTarget[i]->id);
					ini.setValue("target/" + QString::number(i) + "/player", psCurr->psTarget[i]->player);
					ini.setValue("target/" + QString::number(i) + "/type", psCurr->psTarget[i]->type);
#ifdef DEBUG
					ini.setValue("target/" + QString::number(i) + "/debugfunc", QString::fromUtf8(psCurr->targetFunc[i]));
					ini.setValue("target/" + QString::number(i) + "/debugline", psCurr->targetLine[i]);
#endif
				}
			}
			ini.setValue("currentBuildPts", psCurr->currentBuildPts);
			if (psCurr->pFunctionality)
			{
				if (psCurr->pStructureType->type == REF_FACTORY || psCurr->pStructureType->type == REF_CYBORG_FACTORY
				    || psCurr->pStructureType->type == REF_VTOL_FACTORY)
				{
					FACTORY *psFactory = (FACTORY *)psCurr->pFunctionality;
					ini.setValue("modules", psFactory->capacity);
					ini.setValue("Factory/productionLoops", psFactory->productionLoops);
					ini.setValue("Factory/timeStarted", psFactory->timeStarted);
					ini.setValue("Factory/buildPointsRemaining", psFactory->buildPointsRemaining);
					ini.setValue("Factory/timeStartHold", psFactory->timeStartHold);
					ini.setValue("Factory/loopsPerformed", psFactory->loopsPerformed);
					ini.setValue("Factory/productionOutput", psFactory->productionOutput);
					// statusPending and pendingCount belong to the GUI, not the game state.
					ini.setValue("Factory/secondaryOrder", psFactory->secondaryOrder);

					if (psFactory->psSubject != NULL) ini.setValue("Factory/template", psFactory->psSubject->multiPlayerID);
					FLAG_POSITION *psFlag = ((FACTORY *)psCurr->pFunctionality)->psAssemblyPoint;
					if (psFlag != NULL)
					{
						ini.setVector3i("Factory/assemblyPoint/pos", psFlag->coords);
						if (psFlag->selected) ini.setValue("Factory/assemblyPoint/selected", psFlag->selected);
					}
					if (psFactory->psCommander)
					{
						ini.setValue("Factory/commander/id", psFactory->psCommander->id);
						ini.setValue("Factory/commander/player", psFactory->psCommander->player);
					}
					ini.setValue("Factory/secondaryOrder", psFactory->secondaryOrder);
					ProductionRun emptyRun;
					bool haveRun = psFactory->psAssemblyPoint->factoryInc < asProductionRun[psFactory->psAssemblyPoint->factoryType].size();
					ProductionRun const &productionRun = haveRun? asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc] : emptyRun;
					ini.setValue("Factory/productionRuns", (int)productionRun.size());
					for (int runNum = 0; runNum < productionRun.size(); runNum++)
					{
						ProductionRunEntry psCurrentProd = productionRun.at(runNum);
						ini.setValue("Factory/Run/" + QString::number(runNum) + "/quantity", psCurrentProd.quantity);
						ini.setValue("Factory/Run/" + QString::number(runNum) + "/built", psCurrentProd.built);
						if (psCurrentProd.psTemplate) ini.setValue("Factory/Run/" + QString::number(runNum) + "/template",
											   psCurrentProd.psTemplate->multiPlayerID);
					}
				}
				else if (psCurr->pStructureType->type == REF_RESEARCH)
				{
					ini.setValue("modules", ((RESEARCH_FACILITY *)psCurr->pFunctionality)->capacity);
					ini.setValue("Research/timeStartHold", ((RESEARCH_FACILITY *)psCurr->pFunctionality)->timeStartHold);
					if (((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject)
					{
						ini.setValue("Research/target", ((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject->pName);
					}
				}
				else if (psCurr->pStructureType->type == REF_POWER_GEN)
				{
					ini.setValue("modules", ((POWER_GEN *)psCurr->pFunctionality)->capacity);
				}
				else if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
				{
					REPAIR_FACILITY *psRepair = ((REPAIR_FACILITY *)psCurr->pFunctionality);
					if (psRepair->psObj)
					{
						ini.setValue("Repair/target/id", psRepair->psObj->id);
						ini.setValue("Repair/target/player", psRepair->psObj->player);
						ini.setValue("Repair/target/type", psRepair->psObj->type);
					}
					FLAG_POSITION *psFlag = psRepair->psDeliveryPoint;
					if (psFlag)
					{
						ini.setVector3i("Repair/deliveryPoint/pos", psFlag->coords);
						if (psFlag->selected) ini.setValue("Repair/deliveryPoint/selected", psFlag->selected);
					}
				}
				else if (psCurr->pStructureType->type == REF_REARM_PAD)
				{
					REARM_PAD *psReArmPad = ((REARM_PAD *)psCurr->pFunctionality);
					ini.setValue("Rearm/reArmPoints", psReArmPad->reArmPoints);
					ini.setValue("Rearm/timeStarted", psReArmPad->timeStarted);
					ini.setValue("Rearm/timeLastUpdated", psReArmPad->timeLastUpdated);
					if (psReArmPad->psObj)
					{
						ini.setValue("Rearm/target/id", psReArmPad->psObj->id);
						ini.setValue("Rearm/target/player", psReArmPad->psObj->player);
						ini.setValue("Rearm/target/type", psReArmPad->psObj->type);
					}
				}
				else if (psCurr->pStructureType->type == REF_WALL || psCurr->pStructureType->type == REF_GATE)
				{
					ini.setValue("Wall/type", psCurr->pFunctionality->wall.type);
				}
			}
			ini.endGroup();
		}
	}
	return true;
}

// -----------------------------------------------------------------------------------------
bool loadSaveStructurePointers(QString filename, STRUCTURE **ppList)
{
	WzConfig ini(filename);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", filename.toUtf8().constData());
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		STRUCTURE *psStruct;
		int player = getPlayer(ini);
		int id = ini.value("id").toInt();
		for (psStruct = ppList[player]; psStruct && psStruct->id != id; psStruct = psStruct->psNext) { }
		if (!psStruct)
		{
			ini.endGroup();
			continue;	// it is not unusual for a structure to 'disappear' like this; it can happen eg because of module upgrades
		}
		for (int j = 0; j < STRUCT_MAXWEAPS; j++)
		{
			objTrace(psStruct->id, "weapon %d, nStat %d", j, psStruct->asWeaps[j].nStat);
			if (ini.contains("target/" + QString::number(j) + "/id"))
			{
				int tid = ini.value("target/" + QString::number(j) + "/id", -1).toInt();
				int tplayer = ini.value("target/" + QString::number(j) + "/player", -1).toInt();
				OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value("target/" + QString::number(j) + "/type", 0).toInt();
				ASSERT(tid >= 0 && tplayer >= 0, "Bad ID");
				setStructureTarget(psStruct, getBaseObjFromData(tid, tplayer, ttype), j, ORIGIN_UNKNOWN);
				ASSERT(psStruct->psTarget[j], "Failed to find target");
			}
			if (ini.contains("Factory/commander/id"))
			{
				ASSERT(psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
				       || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Bad type");
				FACTORY *psFactory = (FACTORY *)psStruct->pFunctionality;
				OBJECT_TYPE ttype = OBJ_DROID;
				int tid = ini.value("Factory/commander/id", -1).toInt();
				int tplayer = ini.value("Factory/commander/player", -1).toInt();
				ASSERT(tid >= 0 && tplayer >= 0, "Bad commander ID %d for player %d for building %d", tid, tplayer, id);
				DROID *psCommander = (DROID *)getBaseObjFromData(tid, tplayer, ttype);
				ASSERT(psCommander, "Commander %d not found for building %d", tid, id);
				if (ppList == mission.apsStructLists)
				{
					psFactory->psCommander = psCommander;
				}
				else
				{
					assignFactoryCommandDroid(psStruct, psCommander);
				}
			}
			if (ini.contains("Repair/target/id"))
			{
				ASSERT(psStruct->pStructureType->type == REF_REPAIR_FACILITY, "Bad type");
				REPAIR_FACILITY *psRepair = ((REPAIR_FACILITY *)psStruct->pFunctionality);
				OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value("Repair/target/type", OBJ_DROID).toInt();
				int tid = ini.value("Repair/target/id", -1).toInt();
				int tplayer = ini.value("Repair/target/player", -1).toInt();
				ASSERT(tid >= 0 && tplayer >= 0, "Bad repair ID %d for player %d for building %d", tid, tplayer, id);
				psRepair->psObj = getBaseObjFromData(tid, tplayer, ttype);
				ASSERT(psRepair->psObj, "Repair target %d not found for building %d", tid, id);
			}
			if (ini.contains("Rearm/target/id"))
			{
				ASSERT(psStruct->pStructureType->type == REF_REARM_PAD, "Bad type");
				REARM_PAD *psReArmPad = ((REARM_PAD *)psStruct->pFunctionality);
				OBJECT_TYPE ttype = OBJ_DROID; // always, for now
				int tid = ini.value("Rearm/target/id", -1).toInt();
				int tplayer = ini.value("Rearm/target/player", -1).toInt();
				ASSERT(tid >= 0 && tplayer >= 0, "Bad rearm ID %d for player %d for building %d", tid, tplayer, id);
				psReArmPad->psObj = getBaseObjFromData(tid, tplayer, ttype);
				ASSERT(psReArmPad->psObj, "Rearm target %d not found for building %d", tid, id);
			}
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
bool loadSaveFeature(char *pFileData, UDWORD filesize)
{
	FEATURE_SAVEHEADER		*psHeader;
	SAVE_FEATURE_V14			*psSaveFeature;
	FEATURE					*pFeature;
	UDWORD					count, i, statInc;
	FEATURE_STATS			*psStats = NULL;
	bool					found;
	UDWORD					sizeOfSaveFeature;

	/* Check the file type */
	psHeader = (FEATURE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'f' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 'a' || psHeader->aFileType[3] != 't')
	{
		debug( LOG_ERROR, "loadSaveFeature: Incorrect file type" );
		return false;
	}

	/* FEATURE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "Feature file version is %u ", psHeader->version);

	//increment to the start of the data
	pFileData += FEATURE_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7 || psHeader->version > VERSION_19)
	{
		debug(LOG_ERROR, "Unsupported save format version %u", psHeader->version);
		return false;
	}
	if (psHeader->version < VERSION_14)
	{
		sizeOfSaveFeature = sizeof(SAVE_FEATURE_V2);
	}
	else
	{
		sizeOfSaveFeature = sizeof(SAVE_FEATURE_V14);
	}
	if ((sizeOfSaveFeature * psHeader->quantity + FEATURE_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "featureLoad: unexpected end of file" );
		return false;
	}

	/* Load in the feature data */
	for (count = 0; count < psHeader->quantity; count ++, pFileData += sizeOfSaveFeature)
	{
		psSaveFeature = (SAVE_FEATURE_V14*) pFileData;

		/* FEATURE_SAVE_V14 is FEATURE_SAVE_V2 */
		/* FEATURE_SAVE_V2 is OBJECT_SAVE_V19 */
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveFeature->id);
		endian_udword(&psSaveFeature->x);
		endian_udword(&psSaveFeature->y);
		endian_udword(&psSaveFeature->z);
		endian_udword(&psSaveFeature->direction);
		endian_udword(&psSaveFeature->player);
		endian_udword(&psSaveFeature->burnStart);
		endian_udword(&psSaveFeature->burnDamage);

		//get the stats for this feature
		found = false;

		for (statInc = 0; statInc < numFeatureStats; statInc++)
		{
			psStats = asFeatureStats + statInc;
			//loop until find the same name
			if (!strcmp(psStats->pName, psSaveFeature->name))
			{
				found = true;
				break;
			}
		}
		//if haven't found the feature - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "This feature no longer exists - %s", psSaveFeature->name );
			//ignore this
			continue;
		}
		//create the Feature
		pFeature = buildFeature(psStats, psSaveFeature->x, psSaveFeature->y,true);
		if (!pFeature)
		{
			debug(LOG_ERROR, "Unable to create feature %s", psSaveFeature->name);
			continue;
		}
		if (pFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			scriptSetDerrickPos(pFeature->pos.x, pFeature->pos.y);
		}
		//restore values
		pFeature->id = psSaveFeature->id;
		pFeature->rot.direction = DEG(psSaveFeature->direction);
		pFeature->burnDamage = psSaveFeature->burnDamage;
		if (psHeader->version >= VERSION_14)
		{
			for (i=0; i < MAX_PLAYERS; i++)
			{
				pFeature->visible[i] = psSaveFeature->visible[i];
			}
		}
	}

	return true;
}

bool loadSaveFeature2(const char *pFileName)
{
	if (!PHYSFS_exists(pFileName))
	{
		debug(LOG_SAVE, "No %s found -- use fallback method", pFileName);
		return false;
	}
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	QStringList list = ini.childGroups();
	debug(LOG_SAVE, "Loading new style features (%d found)", list.size());
	for (int i = 0; i < list.size(); ++i)
	{
		FEATURE *pFeature;
		ini.beginGroup(list[i]);
		QString name = ini.value("name").toString();
		Position pos = ini.vector3i("position");
		int statInc;
		bool found = false;
		FEATURE_STATS *psStats = NULL;

		//get the stats for this feature
		for (statInc = 0; statInc < numFeatureStats; statInc++)
		{
			psStats = asFeatureStats + statInc;
			//loop until find the same name
			if (!strcmp(psStats->pName, name.toUtf8().constData()))
			{
				found = true;
				break;
			}
		}
		//if haven't found the feature - ignore this record!
		if (!found)
		{
			debug(LOG_ERROR, "This feature no longer exists - %s", name.toUtf8().constData());
			//ignore this
			continue;
		}
		//create the Feature
		pFeature = buildFeature(psStats, pos.x, pos.y, true);
		if (!pFeature)
		{
			debug(LOG_ERROR, "Unable to create feature %s", name.toUtf8().constData());
			continue;
		}
		if (pFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			scriptSetDerrickPos(pFeature->pos.x, pFeature->pos.y);
		}
		//restore values
		pFeature->id = ini.value("id").toInt();
		pFeature->rot = ini.vector3i("rotation");
		pFeature->burnDamage = ini.value("burnDamage", 0).toInt();
		pFeature->burnStart = ini.value("burnStart", 0).toInt();
		pFeature->born = ini.value("born", 2).toInt();
		pFeature->timeLastHit = ini.value("timeLastHit", UDWORD_MAX).toInt();
		pFeature->selected = ini.value("selected", false).toBool();
		pFeature->body = healthValue(ini, pFeature->psStats->body);
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			pFeature->visible[i] = ini.value("visible/" + QString::number(i), 0).toInt();
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of features to a file
*/
bool writeFeatureFile(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	int counter = 0;
	for(FEATURE *psCurr = apsFeatureLists[0]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		ini.beginGroup("feature_" + QString("%1").arg(counter++, 10, 10, QLatin1Char('0')));  // Zero padded so that alphabetical sort works.
		ini.setValue("id", psCurr->id);
		ini.setValue("name", psCurr->psStats->pName);
		ini.setVector3i("position", psCurr->pos);
		ini.setVector3i("rotation", psCurr->rot);
		ini.setValue("burnDamage", psCurr->burnDamage);
		ini.setValue("burnStart", psCurr->burnStart);
		ini.setValue("health", psCurr->body);
		ini.setValue("born", psCurr->born);
		if (psCurr->selected) ini.setValue("selected", psCurr->selected);
		if (psCurr->timeLastHit != UDWORD_MAX) ini.setValue("timeLastHit", psCurr->timeLastHit);
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			if (psCurr->visible[i]) ini.setValue("visible/" + QString::number(i), psCurr->visible[i]);
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
bool loadSaveTemplate(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		int player = getPlayer(ini);
		DROID_TEMPLATE *psTemplate = new DROID_TEMPLATE;
		psTemplate->pName = NULL;
		sstrcpy(psTemplate->aName, ini.value("name").toString().toUtf8().constData());
		psTemplate->ref = ini.value("ref").toInt();
		psTemplate->droidType = (DROID_TYPE)ini.value("droidType").toInt();
		psTemplate->multiPlayerID = ini.value("multiPlayerID").toInt();
		psTemplate->asParts[COMP_BODY] = getCompFromName(COMP_BODY, ini.value("body", QString("ZNULLBODY")).toString().toUtf8().constData());
		psTemplate->asParts[COMP_BRAIN] = getCompFromName(COMP_BRAIN, ini.value("brain", QString("ZNULLBRAIN")).toString().toUtf8().constData());
		psTemplate->asParts[COMP_PROPULSION] = getCompFromName(COMP_PROPULSION, ini.value("propulsion", QString("ZNULLPROP")).toString().toUtf8().constData());
		psTemplate->asParts[COMP_REPAIRUNIT] = getCompFromName(COMP_REPAIRUNIT, ini.value("repair", QString("ZNULLREPAIR")).toString().toUtf8().constData());
		psTemplate->asParts[COMP_ECM] = getCompFromName(COMP_ECM, ini.value("ecm", QString("ZNULLECM")).toString().toUtf8().constData());
		psTemplate->asParts[COMP_SENSOR] = getCompFromName(COMP_SENSOR, ini.value("sensor", QString("ZNULLSENSOR")).toString().toUtf8().constData());
		psTemplate->asParts[COMP_CONSTRUCT] = getCompFromName(COMP_CONSTRUCT, ini.value("construct", QString("ZNULLCONSTRUCT")).toString().toUtf8().constData());
		psTemplate->asWeaps[0] = getCompFromName(COMP_WEAPON, ini.value("weapon/1", QString("ZNULLWEAPON")).toString().toUtf8().constData());
		psTemplate->asWeaps[1] = getCompFromName(COMP_WEAPON, ini.value("weapon/2", QString("ZNULLWEAPON")).toString().toUtf8().constData());
		psTemplate->asWeaps[2] = getCompFromName(COMP_WEAPON, ini.value("weapon/3", QString("ZNULLWEAPON")).toString().toUtf8().constData());
		psTemplate->numWeaps = ini.value("weapons").toInt();
		psTemplate->enabled = ini.value("enabled").toBool();
		psTemplate->prefab = false;		// not AI template

		//calculate the total build points
		psTemplate->buildPoints = calcTemplateBuild(psTemplate);
		psTemplate->powerPoints = calcTemplatePower(psTemplate);

		//store it in the apropriate player' list
		//if a template with the same multiplayerID exists overwrite it
		//else add this template to the top of the list
		DROID_TEMPLATE *psDestTemplate = apsDroidTemplates[player];
		while (psDestTemplate != NULL)
		{
			if (psTemplate->multiPlayerID == psDestTemplate->multiPlayerID)
			{
				//whooh get rid of this one
				break;
			}
			psDestTemplate = psDestTemplate->psNext;
		}

		if (psDestTemplate != NULL)
		{
			psTemplate->psNext = psDestTemplate->psNext;//preserve the list
			*psDestTemplate = *psTemplate;
		}
		else
		{
			//add it to the top of the list
			psTemplate->psNext = apsDroidTemplates[player];
			apsDroidTemplates[player] = psTemplate;
		}
		ini.endGroup();
	}

	for (DROID_TEMPLATE *t = apsDroidTemplates[selectedPlayer]; t != NULL; t = t->psNext)
	{
		localTemplates.push_front(*t);
	}

	return true;
}

bool writeTemplateFile(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		for (DROID_TEMPLATE *psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			ini.beginGroup("template_" + QString::number(psCurr->multiPlayerID));
			ini.setValue("name", psCurr->aName);
			ini.setValue("ref", psCurr->ref);
			ini.setValue("droidType", psCurr->droidType);
			ini.setValue("multiPlayerID", psCurr->multiPlayerID);
			setPlayer(ini, player);
			ini.setValue("body", (asBodyStats + psCurr->asParts[COMP_BODY])->pName);
			ini.setValue("propulsion", (asPropulsionStats + psCurr->asParts[COMP_PROPULSION])->pName);
			ini.setValue("brain", (asBrainStats + psCurr->asParts[COMP_BRAIN])->pName);
			ini.setValue("repair", (asRepairStats + psCurr->asParts[COMP_REPAIRUNIT])->pName);
			ini.setValue("ecm", (asECMStats + psCurr->asParts[COMP_ECM])->pName);
			ini.setValue("sensor", (asSensorStats + psCurr->asParts[COMP_SENSOR])->pName);
			ini.setValue("construct", (asConstructStats + psCurr->asParts[COMP_CONSTRUCT])->pName);
			ini.setValue("weapons", psCurr->numWeaps);
			ini.setValue("enabled", psCurr->enabled);
			for (int j = 0; j < psCurr->numWeaps; j++)
			{
				ini.setValue("weapon/" + QString::number(j + 1), (asWeaponStats + psCurr->asWeaps[j])->pName);
			}
			ini.endGroup();
		}
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// load up a terrain tile type map file
bool loadTerrainTypeMap(const char *pFileData, UDWORD filesize)
{
	TILETYPE_SAVEHEADER	*psHeader;
	UDWORD				i;
	UWORD				*pType;

	if (filesize < TILETYPE_HEADER_SIZE)
	{
		debug( LOG_ERROR, "loadTerrainTypeMap: file too small" );
		return false;
	}

	// Check the header
	psHeader = (TILETYPE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 't' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'y' || psHeader->aFileType[3] != 'p')
	{
		debug( LOG_ERROR, "loadTerrainTypeMap: Incorrect file type" );

		return false;
	}

	/* TILETYPE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	// reset the terrain table
	memset(terrainTypes, 0, sizeof(terrainTypes));

	// Load the terrain type mapping
	pType = (UWORD *)(pFileData + TILETYPE_HEADER_SIZE);
	endian_uword(pType);
	if (psHeader->quantity >= MAX_TILE_TEXTURES)
	{
		// Workaround for fugly map editor bug, since we can't fix the map editor
		psHeader->quantity = MAX_TILE_TEXTURES - 1;
	}
	for(i = 0; i < psHeader->quantity; i++)
	{
		if (*pType > TER_MAX)
		{
			debug( LOG_ERROR, "loadTerrainTypeMap: terrain type out of range" );

			return false;
		}

		terrainTypes[i] = (UBYTE)*pType;
		pType++;
		endian_uword(pType);
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the terrain type map
static bool writeTerrainTypeMapFile(char *pFileName)
{
	TILETYPE_SAVEHEADER		*psHeader;
	char *pFileData;
	UDWORD					fileSize, i;
	UWORD					*pType;

	// Calculate the file size
	fileSize = TILETYPE_HEADER_SIZE + sizeof(UWORD) * MAX_TILE_TEXTURES;
	pFileData = (char*)malloc(fileSize);
	if (!pFileData)
	{
		debug( LOG_FATAL, "writeTerrainTypeMapFile: Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (TILETYPE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 't';
	psHeader->aFileType[1] = 't';
	psHeader->aFileType[2] = 'y';
	psHeader->aFileType[3] = 'p';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = MAX_TILE_TEXTURES;

	pType = (UWORD *)(pFileData + TILETYPE_HEADER_SIZE);
	for(i=0; i<MAX_TILE_TEXTURES; i++)
	{
		*pType = terrainTypes[i];
		endian_uword(pType);
		pType += 1;
	}

	/* TILETYPE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return false;
	}
	free(pFileData);

	return true;
}

// -----------------------------------------------------------------------------------------
bool loadSaveCompList(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + QString::number(player));
		QStringList list = ini.childKeys();
		for (int i = 0; i < list.size(); ++i)
		{
			QString name = list[i];
			int state = ini.value(name, UNAVAILABLE).toInt();
			int type = -1;
			int compInc = -1;
			for (int j = COMP_BODY; j < COMP_NUMCOMPONENTS && compInc == -1; j++)
			{
				// this is very inefficient, but I am so not giving in to the deranged nature of the components code
				// and convoluting the new savegame format for its sake
				compInc = getCompFromName(j, name.toUtf8().constData());
				type = j;
			}
			ASSERT(compInc >= 0, "Bad component %d", compInc);
			ASSERT(type >= 0 && type != COMP_NUMCOMPONENTS, "Bad type %d", type);
			ASSERT_OR_RETURN(false, state == UNAVAILABLE || state == AVAILABLE || state == FOUND || state == REDUNDANT,
					 "Bad state %d for %s", state, name.toUtf8().constData());
			apCompLists[player][type][compInc] = state;
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Comp lists per player
static bool writeCompListFile(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}

	// Save each type of struct type
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + QString::number(player));
		for (int i = 0; i < numBodyStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asBodyStats + i);
			const int state = apCompLists[player][COMP_BODY][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		for (int i = 0; i < numWeaponStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asWeaponStats + i);
			const int state = apCompLists[player][COMP_WEAPON][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		for (int i = 0; i < numConstructStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asConstructStats + i);
			const int state = apCompLists[player][COMP_CONSTRUCT][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		for (int i = 0; i < numECMStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asECMStats + i);
			const int state = apCompLists[player][COMP_ECM][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		for (int i = 0; i < numPropulsionStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asPropulsionStats + i);
			const int state = apCompLists[player][COMP_PROPULSION][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		for (int i = 0; i < numSensorStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asSensorStats + i);
			const int state = apCompLists[player][COMP_SENSOR][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		for (int i = 0; i < numRepairStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asRepairStats + i);
			const int state = apCompLists[player][COMP_REPAIRUNIT][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		for (int i = 0; i < numBrainStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asBrainStats + i);
			const int state = apCompLists[player][COMP_BRAIN][i];
			if (state != UNAVAILABLE) ini.setValue(psStats->pName, state);
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// load up structure type list file
static bool loadSaveStructTypeList(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + QString::number(player));
		QStringList list = ini.childKeys();
		for (int i = 0; i < list.size(); ++i)
		{
			QString name = list[i];
			int state = ini.value(name, UNAVAILABLE).toInt();
			int statInc;

			ASSERT_OR_RETURN(false, state == UNAVAILABLE || state == AVAILABLE || state == FOUND || state == REDUNDANT,
					 "Bad state %d for %s", state, name.toUtf8().constData());
			for (statInc = 0; statInc < numStructureStats; statInc++) // loop until find the same name
			{
				STRUCTURE_STATS *psStats = asStructureStats + statInc;

				if (name.compare(psStats->pName) == 0)
				{
					apStructTypeLists[player][statInc] = state;
					break;
				}
			}
			ASSERT_OR_RETURN(false, statInc != numStructureStats, "Did not find structure %s", name.toUtf8().constData());
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Struct Type List per player
static bool writeStructTypeListFile(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}

	// Save each type of struct type
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + QString::number(player));
		STRUCTURE_STATS *psStats = asStructureStats;
		for (int i = 0; i < numStructureStats; i++, psStats++)
		{
			if (apStructTypeLists[player][i] != UNAVAILABLE) ini.setValue(psStats->pName, apStructTypeLists[player][i]);
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// load up saved research file
bool loadSaveResearch(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	const int players = game.maxPlayers;
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		bool found = false;
		char name[MAX_SAVE_NAME_SIZE];
		sstrcpy(name, ini.value("name").toString().toUtf8().constData());
		int statInc;
		for (statInc = 0; statInc < asResearch.size(); statInc++)
		{
			RESEARCH *psStats = &asResearch[statInc];
			//loop until find the same name
			if (!strcmp(psStats->pName, name))

			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			//ignore this record
			continue;
		}
		QStringList researchedList = ini.value("researched").toStringList();
		QStringList possiblesList = ini.value("possible").toStringList();
		QStringList pointsList = ini.value("currentPoints").toStringList();
		ASSERT(researchedList.size() == players, "Bad researched list for %s", name);
		ASSERT(possiblesList.size() == players, "Bad possible list for %s", name);
		ASSERT(pointsList.size() == players, "Bad points list for %s", name);
		for (int plr = 0; plr < players; plr++)
		{
			PLAYER_RESEARCH *psPlRes;
			int researched = researchedList.at(plr).toInt();
			int possible = possiblesList.at(plr).toInt();
			int points = pointsList.at(plr).toInt();

			psPlRes = &asPlayerResList[plr][statInc];
			// Copy the research status
			psPlRes->ResearchStatus = (researched & RESBITS);
			if (possible != 0)
			{
				MakeResearchPossible(psPlRes);
			}
			psPlRes->currentPoints = points;
			//for any research that has been completed - perform so that upgrade values are set up
			if (researched == RESEARCHED)
			{
				researchResult(statInc, plr, false, NULL, false);
			}
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Research per player
static bool writeResearchFile(char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	RESEARCH *psStats = &asResearch[0];
	for (int i = 0; i < asResearch.size(); i++, psStats = &asResearch[i])
	{
		bool valid = false;
		QStringList possibles, researched, points;
		for (int player = 0; player < game.maxPlayers; player++)
		{
			possibles.push_back(QString::number(IsResearchPossible(&asPlayerResList[player][i])));
			researched.push_back(QString::number(asPlayerResList[player][i].ResearchStatus & RESBITS));
			points.push_back(QString::number(asPlayerResList[player][i].currentPoints));
			if (IsResearchPossible(&asPlayerResList[player][i]) || (asPlayerResList[player][i].ResearchStatus & RESBITS) || asPlayerResList[player][i].currentPoints)
			{
				valid = true;	// write this entry
			}
		}
		if (valid)
		{
			ini.beginGroup("research_" + QString::number(i));
			ini.setValue("name", psStats->pName);
			ini.setValue("possible", possibles);
			ini.setValue("researched", researched);
			ini.setValue("currentPoints", points);
			ini.endGroup();
		}
	}
	return true;
}


// -----------------------------------------------------------------------------------------
// load up saved message file
bool loadSaveMessage(const char *pFileName, SWORD levelType)
{
	// Only clear the messages if its a mid save game
	if (gameType == GTYPE_SAVE_MIDMISSION)
	{
		freeMessages();
	}
	else if (gameType == GTYPE_SAVE_START)
	{
		// If we are loading in a CamStart or a CamChange then we are not interested in any saved messages
		if (levelType == LDS_CAMSTART || levelType == LDS_CAMCHANGE)
		{
			return true;
		}
	}

	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		MESSAGE_TYPE type = (MESSAGE_TYPE)ini.value("type").toInt();
		bool bObj = ini.contains("obj/id");
		int player = ini.value("player").toInt();
		int id = ini.value("id").toInt();
		int dataType = ini.value("dataType").toInt();

		if (type == MSG_PROXIMITY)
		{
			//only load proximity if a mid-mission save game
			if (gameType == GTYPE_SAVE_MIDMISSION)
			{
				if (bObj)
				{
					// Proximity object so create get the obj from saved idy
					int objId = ini.value("obj/id").toInt();
					int objPlayer = ini.value("obj/player").toInt();
					OBJECT_TYPE objType = (OBJECT_TYPE)ini.value("obj/type").toInt();
					MESSAGE *psMessage = addMessage(type, true, player);
					if (psMessage)
					{
						psMessage->pViewData = (MSG_VIEWDATA *)getBaseObjFromData(objId, objPlayer, objType);
						ASSERT(psMessage->pViewData, "Viewdata object id %d not found for message %d", objId, id);
					}
					else
					{
						debug(LOG_ERROR, "Proximity object could not be created (type=%d, player=%d, message=%d)",
						      type, player, id);
					}
				}
				else
				{
					VIEWDATA *psViewData = NULL;

					// Proximity position so get viewdata pointer from the name
					MESSAGE *psMessage = addMessage(type, false, player);

					if (psMessage)
					{
						if (dataType == MSG_DATA_BEACON)
						{
							Vector2i pos = ini.vector2i("position");
							int sender = ini.value("sender").toInt();
							psViewData = CreateBeaconViewData(sender, pos.x, pos.y);
							ASSERT(psViewData, "Could not create view data for message %d", id);
							if (psViewData == NULL)
							{
								// Skip this message
								removeMessage(psMessage, player);
								continue;
							}
						}
						else if (ini.contains("name"))
						{
							psViewData = (VIEWDATA *)getViewData(ini.value("name").toString().toUtf8().constData());
							ASSERT(psViewData, "Failed to find view data for proximity position - skipping message %d", id);
							if (psViewData == NULL)
							{
								// Skip this message
								removeMessage(psMessage, player);
								continue;
							}
						}
						else
						{
							debug(LOG_ERROR, "Proximity position with empty name skipped (message %d)", id);
							removeMessage(psMessage, player);
							continue;
						}

						psMessage->pViewData = (MSG_VIEWDATA *)psViewData;
						// Check the z value is at least the height of the terrain
						const int height = map_Height(((VIEW_PROXIMITY *)psViewData->pData)->x, ((VIEW_PROXIMITY *)psViewData->pData)->y);
						if (((VIEW_PROXIMITY *)psViewData->pData)->z < height)
						{
							((VIEW_PROXIMITY *)psViewData->pData)->z = height;
						}
					}
					else
					{
						debug(LOG_ERROR, "Proximity position could not be created (type=%d, player=%d, message=%d)",
						      type, player, id);
					}
				}
			}
		}
		else
		{
			// Only load Campaign/Mission messages if a mid-mission save game; always load research messages
			if (type == MSG_RESEARCH || gameType == GTYPE_SAVE_MIDMISSION)
			{
				MESSAGE *psMessage = addMessage(type, false, player);
				ASSERT(psMessage, "Could not create message %d", id);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)getViewData(ini.value("name").toString().toUtf8().constData());
					ASSERT(psMessage->pViewData, "Failed to find view data for message %d", id);
				}
			}
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current messages per player
static bool writeMessageFile(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	int numMessages = 0;

	// save each type of research
	for (int player = 0; player < game.maxPlayers; player++)
	{
		for (MESSAGE *psMessage = apsMessages[player]; psMessage != NULL;psMessage = psMessage->psNext)
		{
			ini.beginGroup("message_" + QString::number(numMessages++));
			ini.setValue("id", numMessages - 1);	// for future use
			ini.setValue("player", player);
			ini.setValue("type", psMessage->type);
			ini.setValue("dataType", psMessage->dataType);
			if (psMessage->type == MSG_PROXIMITY)
			{
				//get the matching proximity message
				PROXIMITY_DISPLAY *psProx = NULL;
				for (psProx = apsProxDisp[player]; psProx != NULL; psProx = psProx->psNext)
				{
					//compare the pointers
					if (psProx->psMessage == psMessage)
					{
						break;
					}
				}
				ASSERT(psProx != NULL,"Save message; proximity display not found for message");
				if (psProx->type == POS_PROXDATA)
				{
					//message has viewdata so store the name
					VIEWDATA *pViewData = (VIEWDATA*)psMessage->pViewData;
					ini.setValue("name", pViewData->pName);

					// save beacon data
					if (psMessage->dataType == MSG_DATA_BEACON)
					{
						VIEW_PROXIMITY *viewData = (VIEW_PROXIMITY *)((VIEWDATA *)psMessage->pViewData)->pData;
						ini.setVector2i("position", Vector2i(viewData->x, viewData->y));
						ini.setValue("sender", viewData->sender);
					}
				}
				else
				{
					// message has object so store Object Id
					BASE_OBJECT *psObj = (BASE_OBJECT*)psMessage->pViewData;
					ini.setValue("obj/id", psObj->id);
					ini.setValue("obj/player", psObj->player);
					ini.setValue("obj/type", psObj->type);
				}
			}
			else
			{
				VIEWDATA *pViewData = (VIEWDATA*)psMessage->pViewData;
				ini.setValue("name", pViewData->pName);
			}
			ini.setValue("read", psMessage->read);	// flag to indicate whether message has been read; not that this is/was _not_ read by loading code!?
			ASSERT(player == psMessage->player, "Bad player number (%d == %d)", player, psMessage->player);
			ini.endGroup();
		}
	}
	return true;
}

// -----------------------------------------------------------------------------------------
bool loadSaveStructLimits(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	for (int player = 0; player < game.maxPlayers; player++)
	{
		ini.beginGroup("player_" + QString::number(player));
		QStringList list = ini.childKeys();
		for (int i = 0; i < list.size(); ++i)
		{
			QString name = list[i];
			int limit = ini.value(name, 0).toInt();
			int statInc;

			for (statInc = 0; statInc < numStructureStats; statInc++)
			{
				STRUCTURE_STATS *psStats = asStructureStats + statInc;
				if (name.compare(psStats->pName) == 0)
				{
					asStructLimits[player][statInc].limit = limit != 255? limit : LOTS_OF;
					break;
				}
			}
			ASSERT_OR_RETURN(false, statInc != numStructureStats, "Did not find structure %s", name.toUtf8().constData());
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes the list of structure limits to a file
*/
bool writeStructLimitsFile(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}

	// Save each type of struct type
	for (int player = 0; player < game.maxPlayers; player++)
	{
		ini.beginGroup("player_" + QString::number(player));
		STRUCTURE_STATS *psStats = asStructureStats;
		for (int i = 0; i < numStructureStats; i++, psStats++)
		{
			const int limit = MIN(asStructLimits[player][i].limit, 255);
			if (limit != 255) ini.setValue(psStats->pName, limit);
		}
		ini.endGroup();
	}
	return true;
}

/*!
 * Load the current fire-support designated commanders (the one who has fire-support enabled)
 */
bool readFiresupportDesignators(const char *pFileName)
{
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		uint32_t id = ini.value("Player_" + QString::number(i) + "/id", NULL_ID).toInt();
		if (id != NULL_ID)
		{
			cmdDroidSetDesignator((DROID*)getBaseObjFromId(id));
		}
	}
	return true;
}

/*!
 * Save the current fire-support designated commanders (the one who has fire-support enabled)
 */
bool writeFiresupportDesignators(const char *pFileName)
{
	int player;

	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
		return false;
	}
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		DROID *psDroid = cmdDroidGetDesignator(player);
		if (psDroid != NULL)
		{
			ini.setValue("Player_" + QString::number(player) + "/id", psDroid->id);
		}
	}
	return true;
}


// -----------------------------------------------------------------------------------------
// write the event state to a file on disk
static bool	writeScriptState(const char *pFileName)
{
	char	jsFilename[PATH_MAX], *ext;

	if (!eventSaveState(pFileName))
	{
		return false;
	}

	// The below belongs to the new javascript stuff
	strcpy(jsFilename, pFileName);
	ext = strrchr(jsFilename, '/');
	*ext = '\0';
	strcat(jsFilename, "/scriptstate.ini");
	saveScriptStates(jsFilename);

	return true;
}

// -----------------------------------------------------------------------------------------
// load the script state given a .gam name
bool loadScriptState(char *pFileName)
{
	char	jsFilename[PATH_MAX];

	pFileName[strlen(pFileName) - 4] = '\0';

	// The below belongs to the new javascript stuff
	strcpy(jsFilename, pFileName);
	strcat(jsFilename, "/scriptstate.ini");
	loadScriptStates(jsFilename);

	// change the file extension
	strcat(pFileName, "/scriptstate.es");

	if (!eventLoadState(pFileName))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
/* set the global scroll values to use for the save game */
static void setMapScroll(void)
{
	//if loading in a pre version5 then scroll values will not have been set up so set to max poss
	if (width == 0 && height == 0)
	{
		scrollMinX = 0;
		scrollMaxX = mapWidth;
		scrollMinY = 0;
		scrollMaxY = mapHeight;
		return;
	}
	scrollMinX = startX;
	scrollMinY = startY;
	scrollMaxX = startX + width;
	scrollMaxY = startY + height;
	//check not going beyond width/height of map
	if (scrollMaxX > (SDWORD)mapWidth)
	{
		scrollMaxX = mapWidth;
		debug( LOG_NEVER, "scrollMaxX was too big It has been set to map width" );
	}
	if (scrollMaxY > (SDWORD)mapHeight)
	{
		scrollMaxY = mapHeight;
		debug( LOG_NEVER, "scrollMaxY was too big It has been set to map height" );
	}
}


// -----------------------------------------------------------------------------------------
/*returns the current type of save game being loaded*/
UDWORD getSaveGameType(void)
{
	return gameType;
}


/**
 * \param[out] backDropSprite The premade map texture.
 * \param scale               Scale of the map texture.
 * \param[out] playeridpos    Will contain the position on the map where the player's HQ are located.
 *
 * Reads the current map and colours the map preview for any structures
 * present. Additionally we load the player's HQ location into playeridpos so
 * we know the player's starting location.
 */
bool plotStructurePreview16(char *backDropSprite, Vector2i playeridpos[])
{
	union { SAVE_STRUCTURE_V2 v2; SAVE_STRUCTURE_V20 v20; } sSave;  // close eyes now.
	SAVE_STRUCTURE_V2 *psSaveStructure2   = &sSave.v2;
	SAVE_STRUCTURE_V20 *psSaveStructure20 = &sSave.v20;
	// ok you can open them again..

	STRUCT_SAVEHEADER *psHeader;
	char aFileName[256];
	UDWORD xx, yy, count, fileSize, sizeOfSaveStructure = sizeof(SAVE_STRUCTURE);
	UDWORD playerid = 0;
	char *pFileData = NULL;
	LEVEL_DATASET *psLevel;
	PIELIGHT color = WZCOL_BLACK ;
	bool HQ = false;

	psLevel = levFindDataSet(game.map);
	strcpy(aFileName, psLevel->apDataFiles[0]);
	aFileName[strlen(aFileName) - 4] = '\0';
	strcat(aFileName, "/struct.bjo");

	if (!PHYSFS_exists(aFileName))	// use new version of structure data
	{
		strcpy(aFileName, psLevel->apDataFiles[0]);
		aFileName[strlen(aFileName) - 4] = '\0';
		strcat(aFileName, "/struct.ini");
		WzConfig ini(aFileName);
		if (ini.status() != QSettings::NoError)
		{
			debug(LOG_ERROR, "Could not open %s", aFileName);
			return false;
		}
		QStringList list = ini.childGroups();
		for (int i = 0; i < list.size(); ++i)
		{
			ini.beginGroup(list[i]);
			QString name = ini.value("name").toString();
			Position pos = ini.vector3i("position");
			playerid = ini.value("startpos", scavengerSlot()).toInt();  // No conversion should be going on, this is the map makers position when player X should be.

			if (name.startsWith("A0CommandCentre"))
			{
				HQ = true;
				xx = playeridpos[playerid].x = map_coord(pos.x);
				yy = playeridpos[playerid].y = map_coord(pos.y);
			}
			else
			{
				HQ = false;
				xx = map_coord(pos.x);
				yy = map_coord(pos.y);
			}
			playerid = getPlayerColour(RemapPlayerNumber(playerid));
			// kludge to fix black, so you can see it on some maps.
			if (playerid == 3)	// in this case 3 = palette entry for black.
			{
				color = WZCOL_GREY;
			}
			else
			{
				color.rgba = clanColours[playerid].rgba;
			}
			if (HQ)
			{	// This shows where the HQ is on the map in a special color.
				// We could do the same for anything else (oil/whatever) also.
				// Possible future enhancement?
				color = WZCOL_MAP_PREVIEW_HQ;
			}
			// and now we blit the color to the texture
			backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx)] = color.byte.r;
			backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 1] = color.byte.g;
			backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 2] = color.byte.b;
			ini.endGroup();
		}
		// And now we need to show features.
		plotFeature(backDropSprite);

		return true;
	}


	pFileData = fileLoadBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_NEVER, "Failed to load file to buffer.");
	}

	/* Check the file type */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
	    psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'u')
	{
		debug(LOG_ERROR, "Invalid file type.");
		return false;
	}

	/* STRUCT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += STRUCT_HEADER_SIZE;

	if (psHeader->version < VERSION_12)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V2);
	}
	else if (psHeader->version < VERSION_14)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V12);
	}
	else if (psHeader->version <= VERSION_14)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V14);
	}
	else if (psHeader->version <= VERSION_16)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V15);
	}
	else if (psHeader->version <= VERSION_19)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V17);
	}
	else if (psHeader->version <= VERSION_20)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V20);
	}

	/* Load in the structure data */
	for (count = 0; count < psHeader->quantity; count++, pFileData += sizeOfSaveStructure)
	{
		// we are specifically looking for the HQ, and it seems this is the only way to
		// find it via parsing map.
		// We store the coordinates of the structure, into a array for as many players as are on the map.

		if (psHeader->version <= VERSION_19)
		{
			// All versions up to 19 are compatible with V2.
			memcpy(psSaveStructure2, pFileData, sizeof(SAVE_STRUCTURE_V2));

			endian_udword(&psSaveStructure2->x);
			endian_udword(&psSaveStructure2->y);
			endian_udword(&psSaveStructure2->player);
			playerid = psSaveStructure2->player;
			if (strncmp(psSaveStructure2->name, "A0CommandCentre", 15) == 0)
			{
				HQ = true;
				xx = playeridpos[playerid].x = map_coord(psSaveStructure2->x);
				yy = playeridpos[playerid].y = map_coord(psSaveStructure2->y);
			}
			else
			{
				HQ = false;
				xx = map_coord(psSaveStructure2->x);
				yy = map_coord(psSaveStructure2->y);
			}
		}
		else
		{
			// All newer versions are compatible with V20.
			memcpy(psSaveStructure20, pFileData, sizeof(SAVE_STRUCTURE_V20));

			endian_udword(&psSaveStructure20->x);
			endian_udword(&psSaveStructure20->y);
			endian_udword(&psSaveStructure20->player);
			playerid = psSaveStructure20->player;

			if (strncmp(psSaveStructure20->name, "A0CommandCentre", 15) == 0)
			{
				HQ = true;
				xx = playeridpos[playerid].x = map_coord(psSaveStructure20->x);
				yy = playeridpos[playerid].y = map_coord(psSaveStructure20->y);
			}
			else
			{
				HQ = false;
				xx = map_coord(psSaveStructure20->x);
				yy = map_coord(psSaveStructure20->y);
			}
		}
		playerid = getPlayerColour(RemapPlayerNumber(playerid));
		// kludge to fix black, so you can see it on some maps.
		if (playerid == 3)	// in this case 3 = palette entry for black.
		{
			color = WZCOL_GREY;
		}
		else
		{
			color.rgba = clanColours[playerid].rgba;
		}
		
		if (HQ)
		{	// This shows where the HQ is on the map in a special color.
			// We could do the same for anything else (oil/whatever) also.
			// Possible future enhancement?
			color = WZCOL_MAP_PREVIEW_HQ;
		}

		// and now we blit the color to the texture
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx)] = color.byte.r;
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 1] = color.byte.g;
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 2] = color.byte.b;
	}

	// And now we need to show features.
	plotFeature(backDropSprite);
	return true;
}

// Show location of (at this time) oil on the map preview
static void plotFeature(char *backDropSprite)
{
	FEATURE_SAVEHEADER	*psHeader;
	SAVE_FEATURE_V2	*psSaveFeature;
	LEVEL_DATASET *psLevel;
	UDWORD xx, yy, count, fileSize;
	UDWORD sizeOfSaveFeature = 0;
	char *pFileData = NULL;
	char aFileName[256];
	const PIELIGHT color = WZCOL_MAP_PREVIEW_OIL;

	psLevel = levFindDataSet(game.map);
	strcpy(aFileName, psLevel->apDataFiles[0]);
	aFileName[strlen(aFileName) - 4] = '\0';
	strcat(aFileName, "/feat.bjo");
	if (!PHYSFS_exists(aFileName))	// use new version of feature data
	{
		strcpy(aFileName, psLevel->apDataFiles[0]);
		aFileName[strlen(aFileName) - 4] = '\0';
		strcat(aFileName, "/feature.ini");
		WzConfig ini(aFileName);
		if (ini.status() != QSettings::NoError)
		{
			debug(LOG_ERROR, "Could not open %s", aFileName);
			return;
		}
		QStringList list = ini.childGroups();
		for (int i = 0; i < list.size(); ++i)
		{
			ini.beginGroup(list[i]);
			QString name = ini.value("name").toString();
			Position pos = ini.vector3i("position");

			// we only care about oil
			if (name.startsWith("OilResource"))
			{
				// and now we blit the color to the texture
				xx = map_coord(pos.x);
				yy = map_coord(pos.y);
				backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx)] = color.byte.r;
				backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 1] = color.byte.g;
				backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 2] = color.byte.b;
			}
			ini.endGroup();
		}
		return;
	}

	// Load in the chosen file data/
	pFileData = fileLoadBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug( LOG_ERROR, "Unable to load file %s?", aFileName);
		return;
	}

	// Check the file type
	psHeader = (FEATURE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'f' || psHeader->aFileType[1] != 'e' || psHeader->aFileType[2] != 'a' || psHeader->aFileType[3] != 't')
	{
		debug( LOG_ERROR, "Incorrect file type, looking at %s", aFileName);
		return;
	}

	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += FEATURE_HEADER_SIZE;

	sizeOfSaveFeature = sizeof(SAVE_FEATURE_V2);

	if ((sizeOfSaveFeature * psHeader->quantity + FEATURE_HEADER_SIZE) > fileSize)
	{
		debug( LOG_ERROR, "Unexpected end of file ?" );
		return;
	}

	// Load in the feature data
	for (count = 0; count < psHeader->quantity; count++, pFileData += sizeOfSaveFeature)
	{
		// All versions up to 19 are compatible with V2.
		psSaveFeature = (SAVE_FEATURE_V2*) pFileData;

		// we only care about oil
		if (strncmp(psSaveFeature->name, "OilResource", 11) == 0)
		{
			endian_udword(&psSaveFeature->x);
			endian_udword(&psSaveFeature->y);
			xx = map_coord(psSaveFeature->x);
			yy = map_coord(psSaveFeature->y);
		}
		else
		{
			continue;
		}
		// and now we blit the color to the texture
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx)] = color.byte.r;
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 1] = color.byte.g;
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 2] = color.byte.b;
	}
}
