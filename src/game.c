/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

/* Standard library headers */
#include <physfs.h>
#include <string.h>

/* Warzone src and library headers */
#include "lib/framework/endian_hack.h"
#include "lib/framework/file.h"
#include "lib/framework/frameint.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/strres.h"
#include "lib/framework/tagfile.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/netplay/netplay.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "modding.h"

#include "game.h"

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
#include "lighting.h"
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
#include "scriptextern.h"
#include "multistat.h"
#include "wrappers.h"
#include "scriptfuncs.h"
#include "challenge.h"

#define MAX_SAVE_NAME_SIZE_V19	40
#define MAX_SAVE_NAME_SIZE	60

#define NULL_ID UDWORD_MAX
#define MAX_BODY			SWORD_MAX
#define SAVEKEY_ONMISSION	0x100

/*!
 * FIXME
 * The code is reusing some pointers as normal integer values apparently. This
 * should be fixed!
 */
#define FIXME_CAST_ASSIGN(TYPE, lval, rval) { TYPE* __tmp = (TYPE*) &lval; *__tmp = (TYPE)rval; }

/// @note This represents a size internal to savegame files, so: DO NOT CHANGE THIS
#define MAX_GAME_STR_SIZE 20

static UDWORD RemapPlayerNumber(UDWORD OldNumber);

typedef struct _game_save_header
{
	char        aFileType[4];
	uint32_t    version;
} GAME_SAVEHEADER;

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

typedef struct _droid_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} DROID_SAVEHEADER;

typedef struct _struct_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} STRUCT_SAVEHEADER;

typedef struct _template_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} TEMPLATE_SAVEHEADER;

typedef struct _feature_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} FEATURE_SAVEHEADER;

/* Structure definitions for loading and saving map data */
typedef struct {
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} TILETYPE_SAVEHEADER;

/* Structure definitions for loading and saving map data */
typedef struct _compList_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} COMPLIST_SAVEHEADER;

/* Structure definitions for loading and saving map data */
typedef struct _structList_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} STRUCTLIST_SAVEHEADER;

typedef struct _research_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} RESEARCH_SAVEHEADER;

typedef struct _message_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} MESSAGE_SAVEHEADER;

typedef struct _proximity_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} PROXIMITY_SAVEHEADER;

typedef struct _flag_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} FLAG_SAVEHEADER;

typedef struct _production_save_header
{
	char aFileType[4];
	UDWORD version;
} PRODUCTION_SAVEHEADER;

typedef struct _structLimits_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} STRUCTLIMITS_SAVEHEADER;

typedef struct _command_save_header
{
	char aFileType[4];
	UDWORD version;
	UDWORD quantity;
} COMMAND_SAVEHEADER;

/* Sanity check definitions for the save struct file sizes */
#define GAME_HEADER_SIZE			8
#define DROID_HEADER_SIZE			12
#define DROIDINIT_HEADER_SIZE		12
#define STRUCT_HEADER_SIZE			12
#define TEMPLATE_HEADER_SIZE		12
#define FEATURE_HEADER_SIZE			12
#define TILETYPE_HEADER_SIZE		12
#define COMPLIST_HEADER_SIZE		12
#define STRUCTLIST_HEADER_SIZE		12
#define RESEARCH_HEADER_SIZE		12
#define MESSAGE_HEADER_SIZE			12
#define PROXIMITY_HEADER_SIZE		12
#define FLAG_HEADER_SIZE			12
#define PRODUCTION_HEADER_SIZE		8
#define STRUCTLIMITS_HEADER_SIZE	12
#define COMMAND_HEADER_SIZE			12


// general save definitions
#define MAX_LEVEL_SIZE 20

#define OBJECT_SAVE_V19 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	BOOL				inFire; \
	UDWORD				burnStart; \
	UDWORD				burnDamage

#define OBJECT_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	BOOL				inFire; \
	UDWORD				burnStart; \
	UDWORD				burnDamage


typedef struct _save_component_v19
{
	char				name[MAX_SAVE_NAME_SIZE_V19];
} SAVE_COMPONENT_V19;

typedef struct _save_component
{
	char				name[MAX_SAVE_NAME_SIZE];
} SAVE_COMPONENT;

typedef struct _save_weapon_v19
{
	char				name[MAX_SAVE_NAME_SIZE_V19];
	UDWORD				hitPoints;  // UNUSED: only here to keep struct size intact
	UDWORD				ammo;
	UDWORD				lastFired;
} SAVE_WEAPON_V19;

typedef struct _save_weapon
{
	char				name[MAX_SAVE_NAME_SIZE];
	UDWORD				hitPoints;  // UNUSED: only here to keep struct size intact
	UDWORD				ammo;
	UDWORD				lastFired;
} SAVE_WEAPON;

typedef struct _savePower
{
	uint32_t    currentPower;
	uint32_t    extractedPower; // UNUSED
} SAVE_POWER;

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
	 || !PHYSFS_writeSBE32(fileHandle, serializeMulti->fog)
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

	serializeMulti->fog = boolFog;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE8(fileHandle, &serializeMulti->skDiff[i]))
			return false;
	}

	return true;
}

static bool serializeSessionDesc(PHYSFS_file* fileHandle, const SESSIONDESC* serializeDesc)
{
	return (PHYSFS_writeSBE32(fileHandle, serializeDesc->dwSize)
	     && PHYSFS_writeSBE32(fileHandle, serializeDesc->dwFlags)
	     && PHYSFS_write(fileHandle, serializeDesc->host, 1, 16) == 16
	     && PHYSFS_writeSBE32(fileHandle, serializeDesc->dwMaxPlayers)
	     && PHYSFS_writeSBE32(fileHandle, serializeDesc->dwCurrentPlayers)
	     && PHYSFS_writeSBE32(fileHandle, serializeDesc->dwUserFlags[0])
	     && PHYSFS_writeSBE32(fileHandle, serializeDesc->dwUserFlags[1])
	     && PHYSFS_writeSBE32(fileHandle, serializeDesc->dwUserFlags[2])
	     && PHYSFS_writeSBE32(fileHandle, serializeDesc->dwUserFlags[3]));
}

static bool deserializeSessionDesc(PHYSFS_file* fileHandle, SESSIONDESC* serializeDesc)
{
	return (PHYSFS_readSBE32(fileHandle, &serializeDesc->dwSize)
	     && PHYSFS_readSBE32(fileHandle, &serializeDesc->dwFlags)
	     && PHYSFS_read(fileHandle, serializeDesc->host, 1, 16) == 16
	     && PHYSFS_readSBE32(fileHandle, &serializeDesc->dwMaxPlayers)
	     && PHYSFS_readSBE32(fileHandle, &serializeDesc->dwCurrentPlayers)
	     && PHYSFS_readSBE32(fileHandle, &serializeDesc->dwUserFlags[0])
	     && PHYSFS_readSBE32(fileHandle, &serializeDesc->dwUserFlags[1])
	     && PHYSFS_readSBE32(fileHandle, &serializeDesc->dwUserFlags[2])
	     && PHYSFS_readSBE32(fileHandle, &serializeDesc->dwUserFlags[3]));
}

static bool serializeGameStruct(PHYSFS_file* fileHandle, const GAMESTRUCT* serializeGame)
{
	return (PHYSFS_write(fileHandle, serializeGame->name, StringSize, 1) == 1
	     && serializeSessionDesc(fileHandle, &serializeGame->desc));
}

static bool deserializeGameStruct(PHYSFS_file* fileHandle, GAMESTRUCT* serializeGame)
{
	return (PHYSFS_read(fileHandle, serializeGame->name, StringSize, 1) == 1
	     && deserializeSessionDesc(fileHandle, &serializeGame->desc));
}

static bool serializePlayer(PHYSFS_file* fileHandle, const PLAYER* serializePlayer)
{
	return (PHYSFS_writeUBE32(fileHandle, serializePlayer->position)
	     && PHYSFS_write(fileHandle, serializePlayer->name, StringSize, 1) == 1
	     && PHYSFS_writeUBE32(fileHandle, serializePlayer->colour)
	     && PHYSFS_writeUBE32(fileHandle, serializePlayer->team));
}

static bool deserializePlayer(PHYSFS_file* fileHandle, PLAYER* serializePlayer)
{
	uint32_t position, colour, team;
	bool retval;

	retval = (PHYSFS_readUBE32(fileHandle, &position)
	          && PHYSFS_read(fileHandle, serializePlayer->name, StringSize, 1) == 1
	          && PHYSFS_readUBE32(fileHandle, &colour)
	          && PHYSFS_readUBE32(fileHandle, &team));

	serializePlayer->position = position;
	serializePlayer->colour = colour;
	serializePlayer->team = team;
	return retval;
}

static bool serializeNetPlay(PHYSFS_file* fileHandle, const NETPLAY* serializeNetPlay)
{
	unsigned int i;

	for (i = 0; i < MaxGames; ++i)
	{
		if (!serializeGameStruct(fileHandle, &serializeNetPlay->games[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializePlayer(fileHandle, &serializeNetPlay->players[i]))
			return false;
	}

	return (PHYSFS_writeUBE32(fileHandle, serializeNetPlay->bComms)
	     && PHYSFS_writeUBE32(fileHandle, serializeNetPlay->playercount)
	     && PHYSFS_writeUBE32(fileHandle, serializeNetPlay->hostPlayer)
	     && PHYSFS_writeUBE32(fileHandle, selectedPlayer)
	     && PHYSFS_writeUBE32(fileHandle, 0)
	     && PHYSFS_writeUBE32(fileHandle, 0)
	     && PHYSFS_writeUBE32(fileHandle, 0));
}

static bool deserializeNetPlay(PHYSFS_file* fileHandle, NETPLAY* serializeNetPlay)
{
	unsigned int i;
	uint32_t dummy;

	for (i = 0; i < MaxGames; ++i)
	{
		if (!deserializeGameStruct(fileHandle, &serializeNetPlay->games[i]))
			return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializePlayer(fileHandle, &serializeNetPlay->players[i]))
			return false;
	}

	serializeNetPlay->isHost = true;	// only host can load
	return (PHYSFS_readUBE32(fileHandle, &serializeNetPlay->bComms)
	        && PHYSFS_readUBE32(fileHandle, &serializeNetPlay->playercount)
	        && PHYSFS_readUBE32(fileHandle, &serializeNetPlay->hostPlayer)
	        && PHYSFS_readUBE32(fileHandle, &selectedPlayer)
	        && PHYSFS_readUBE32(fileHandle, &dummy)
	        && PHYSFS_readUBE32(fileHandle, &dummy)
	        && PHYSFS_readUBE32(fileHandle, &dummy));
}

#define GAME_SAVE_V7	\
	uint32_t    gameTime;	\
	uint32_t    GameType;		/* Type of game , one of the GTYPE_... enums. */ \
	int32_t     ScrollMinX;		/* Scroll Limits */ \
	int32_t     ScrollMinY; \
	uint32_t    ScrollMaxX; \
	uint32_t    ScrollMaxY; \
	char	levelName[MAX_LEVEL_SIZE]	//name of the level to load up when mid game

typedef struct save_game_v7
{
	GAME_SAVE_V7;
} SAVE_GAME_V7;

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

#define GAME_SAVE_V10	\
	GAME_SAVE_V7;		\
	SAVE_POWER	power[MAX_PLAYERS]

typedef struct save_game_v10
{
	GAME_SAVE_V10;
} SAVE_GAME_V10;

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

#define GAME_SAVE_V11	\
	GAME_SAVE_V10;		\
	iView currentPlayerPos

typedef struct save_game_v11
{
	GAME_SAVE_V11;
} SAVE_GAME_V11;

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

#define GAME_SAVE_V12	\
	GAME_SAVE_V11;		\
	uint32_t    missionTime;\
	uint32_t    saveKey

typedef struct save_game_v12
{
	GAME_SAVE_V12;
} SAVE_GAME_V12;

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

#define GAME_SAVE_V14			\
	GAME_SAVE_V12;				\
	int32_t     missionOffTime;		\
	int32_t     missionETA;			\
	uint16_t    missionHomeLZ_X;	\
	uint16_t    missionHomeLZ_Y;	\
	int32_t     missionPlayerX;		\
	int32_t     missionPlayerY;		\
	uint16_t    iTranspEntryTileX[MAX_PLAYERS];	\
	uint16_t    iTranspEntryTileY[MAX_PLAYERS];	\
	uint16_t    iTranspExitTileX[MAX_PLAYERS];	\
	uint16_t    iTranspExitTileY[MAX_PLAYERS];	\
	uint32_t    aDefaultSensor[MAX_PLAYERS];	\
	uint32_t    aDefaultECM[MAX_PLAYERS];		\
	uint32_t    aDefaultRepair[MAX_PLAYERS]

typedef struct save_game_v14
{
	GAME_SAVE_V14;
} SAVE_GAME_V14;

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

#define GAME_SAVE_V15			\
	GAME_SAVE_V14;				\
	BOOL        offWorldKeepLists;\
	uint8_t     aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];\
	uint32_t    RubbleTile;\
	uint32_t    WaterTile;\
	uint32_t    fogColour;\
	uint32_t    fogState

typedef struct save_game_v15
{
	GAME_SAVE_V15;
} SAVE_GAME_V15;

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
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->fogColour)
	     && PHYSFS_writeUBE32(fileHandle, serializeGame->fogState));
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

#define GAME_SAVE_V16			\
	GAME_SAVE_V15;				\
	LANDING_ZONE   sLandingZone[MAX_NOGO_AREAS]

typedef struct save_game_v16
{
	GAME_SAVE_V16;
} SAVE_GAME_V16;

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

#define GAME_SAVE_V17			\
	GAME_SAVE_V16;				\
	uint32_t    objId

typedef struct save_game_v17
{
	GAME_SAVE_V17;
} SAVE_GAME_V17;

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

#define GAME_SAVE_V18			\
	GAME_SAVE_V17;				\
	char        buildDate[MAX_STR_LENGTH];		\
	uint32_t    oldestVersion;	\
	uint32_t    validityKey

typedef struct save_game_v18
{
	GAME_SAVE_V18;
} SAVE_GAME_V18;

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

#define GAME_SAVE_V19			\
	GAME_SAVE_V18;				\
	uint8_t     alliances[MAX_PLAYERS][MAX_PLAYERS];\
	uint8_t     playerColour[MAX_PLAYERS];\
	uint8_t     radarZoom

typedef struct save_game_v19
{
	GAME_SAVE_V19;
} SAVE_GAME_V19;

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

#define GAME_SAVE_V20			\
	GAME_SAVE_V19;				\
	uint8_t     bDroidsToSafetyFlag;	\
	Vector2i    asVTOLReturnPos[MAX_PLAYERS]

typedef struct save_game_v20
{
	GAME_SAVE_V20;
} SAVE_GAME_V20;

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

#define GAME_SAVE_V22			\
	GAME_SAVE_V20;				\
	RUN_DATA	asRunData[MAX_PLAYERS]

typedef struct save_game_v22
{
	GAME_SAVE_V22;
} SAVE_GAME_V22;

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

#define GAME_SAVE_V24			\
	GAME_SAVE_V22;				\
	uint32_t    reinforceTime;		\
	uint8_t     bPlayCountDown;	\
	uint8_t     bPlayerHasWon;	\
	uint8_t     bPlayerHasLost;	\
	uint8_t     dummy3

typedef struct save_game_v24
{
	GAME_SAVE_V24;
} SAVE_GAME_V24;

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

/*
#define GAME_SAVE_V27		\
	GAME_SAVE_V24

typedef struct save_game_v27
{
	GAME_SAVE_V27;
} SAVE_GAME_V27;
*/
#define GAME_SAVE_V27			\
	GAME_SAVE_V24;				\
	uint16_t    awDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS]

typedef struct save_game_v27
{
	GAME_SAVE_V27;
} SAVE_GAME_V27;

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

#define GAME_SAVE_V29			\
	GAME_SAVE_V27;				\
	uint16_t    missionScrollMinX;  \
	uint16_t    missionScrollMinY;  \
	uint16_t    missionScrollMaxX;  \
	uint16_t    missionScrollMaxY

typedef struct save_game_v29
{
	GAME_SAVE_V29;
} SAVE_GAME_V29;

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

#define GAME_SAVE_V30			\
	GAME_SAVE_V29;				\
	int32_t     scrGameLevel;       \
	uint8_t     bExtraVictoryFlag;  \
	uint8_t     bExtraFailFlag;     \
	uint8_t     bTrackTransporter

typedef struct save_game_v30
{
	GAME_SAVE_V30;
} SAVE_GAME_V30;

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
#define GAME_SAVE_V31           \
	GAME_SAVE_V30;				\
	int32_t     missionCheatTime

typedef struct save_game_v31
{
	GAME_SAVE_V31;
} SAVE_GAME_V31;


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
#define GAME_SAVE_V33           \
	GAME_SAVE_V31;				\
	MULTIPLAYERGAME sGame;		\
	NETPLAY         sNetPlay;	\
	uint32_t        savePlayer;	\
	char            sPName[32];	\
	BOOL            multiPlayer;\
	uint32_t        sPlayerIndex[MAX_PLAYERS]

typedef struct save_game_v33
{
	GAME_SAVE_V33;
} SAVE_GAME_V33;

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

#define GAME_SAVE_V34           \
	GAME_SAVE_V33;				\
	char		sPlayerName[MAX_PLAYERS][StringSize]

//Now holds AI names for multiplayer
typedef struct save_game_v34
{
	GAME_SAVE_V34;
} SAVE_GAME_V34;


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
#define GAME_SAVE_V35           \
	GAME_SAVE_V34

typedef struct save_game_v35
{
	GAME_SAVE_V35;
} SAVE_GAME_V35;

static bool serializeSaveGameV35Data(PHYSFS_file* fileHandle, const SAVE_GAME_V35* serializeGame)
{
	return serializeSaveGameV34Data(fileHandle, (const SAVE_GAME_V34*) serializeGame);
}

static bool deserializeSaveGameV35Data(PHYSFS_file* fileHandle, SAVE_GAME_V35* serializeGame)
{
	return deserializeSaveGameV34Data(fileHandle, (SAVE_GAME_V34*) serializeGame);
}

// Store loaded mods in savegame
#define GAME_SAVE_V38			\
	GAME_SAVE_V35;					\
	char		modList[modlist_string_size]

typedef struct save_game_v38
{
	GAME_SAVE_V38;
} SAVE_GAME_V38;

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
typedef struct save_game
{
	GAME_SAVE_V38;
} SAVE_GAME;

static bool serializeSaveGameData(PHYSFS_file* fileHandle, const SAVE_GAME* serializeGame)
{
	return serializeSaveGameV38Data(fileHandle, (const SAVE_GAME_V38*) serializeGame);
}

static bool deserializeSaveGameData(PHYSFS_file* fileHandle, SAVE_GAME* serializeGame)
{
	return deserializeSaveGameV38Data(fileHandle, (SAVE_GAME_V38*) serializeGame);
}

#define TEMP_DROID_MAXPROGS	3
#define	SAVE_COMP_PROGRAM	8
#define SAVE_COMP_WEAPON	9

typedef struct _path_point
{
       UBYTE           x,y;
} PATH_POINT;

#define TRAVELSIZE	100

typedef struct _save_move_control
{
	UBYTE	Status;						// Inactive, Navigating or moving point to point status
	UBYTE	Position;	   				// Position in asPath
	UBYTE	numPoints;					// number of points in asPath
	PATH_POINT	asPath[TRAVELSIZE];		// Pointer to list of block X,Y coordinates.
	SDWORD	DestinationX;				// DestinationX,Y should match objects current X,Y
	SDWORD	DestinationY;				//		location for this movement to be complete.
	SDWORD	srcX,srcY,targetX,targetY;
	float	fx,fy;						// droid location as a fract
	float	speed;						// Speed of motion
	SWORD	boundX,boundY;				// Vector for the end of path boundary
	SWORD	moveDir;						// direction of motion (not the direction the droid is facing)
	SWORD	bumpDir;					// direction at last bump
	UDWORD	bumpTime;					// time of first bump with something
	UWORD	lastBump;					// time of last bump with a droid - relative to bumpTime
	UWORD	pauseTime;					// when MOVEPAUSE started - relative to bumpTime
	UWORD	bumpX,bumpY;				// position of last bump
	UDWORD	shuffleStart;				// when a shuffle started
	BOOL	isInFormation;                          // Indicates wether this droid is a member of a formation
	SWORD	iVertSpeed;
	UDWORD	iAttackRuns[DROID_MAXWEAPS];
	float   fz;
} SAVE_MOVE_CONTROL;


#define DROID_SAVE_V9		\
	OBJECT_SAVE_V19;			\
	SAVE_COMPONENT_V19	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UDWORD		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON_V19	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills

typedef struct _save_droid_v9
{
	DROID_SAVE_V9;
} SAVE_DROID_V9;

/*save DROID SAVE 11 */
#define DROID_SAVE_V11		\
	OBJECT_SAVE_V19;			\
	SAVE_COMPONENT_V19	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UBYTE		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON_V19	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills;	\
	UWORD	turretRotation;	\
	UWORD	turretPitch

typedef struct _save_droid_v11
{
	DROID_SAVE_V11;
} SAVE_DROID_V11;

#define DROID_SAVE_V12		\
	DROID_SAVE_V9;			\
	UWORD	turretRotation;	\
	UWORD	turretPitch;	\
	SDWORD	order;			\
	UWORD	orderX,orderY;	\
	UWORD	orderX2,orderY2;\
	UDWORD	timeLastHit;	\
	UDWORD	targetID;		\
	UDWORD	secondaryOrder;	\
	SDWORD	action;			\
	UDWORD	actionX,actionY;\
	UDWORD	actionTargetID;	\
	UDWORD	actionStarted;	\
	UDWORD	actionPoints;	\
	UWORD	actionHeight

typedef struct _save_droid_v12
{
	DROID_SAVE_V12;
} SAVE_DROID_V12;

#define DROID_SAVE_V14		\
	DROID_SAVE_V12;			\
	char	tarStatName[MAX_GAME_STR_SIZE];\
    UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission

typedef struct _save_droid_v14
{
	DROID_SAVE_V14;
} SAVE_DROID_V14;

//DROID_SAVE_18 replaces DROID_SAVE_14
#define DROID_SAVE_V18		\
	DROID_SAVE_V12;			\
	char	tarStatName[MAX_SAVE_NAME_SIZE_V19];\
    UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission

typedef struct _save_droid_v18
{
	DROID_SAVE_V18;
} SAVE_DROID_V18;

//DROID_SAVE_20 replaces all previous saves uses 60 character names
#define DROID_SAVE_V20		\
	OBJECT_SAVE_V20;			\
	SAVE_COMPONENT	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UDWORD		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills;	\
	UWORD	turretRotation;	\
	UWORD	turretPitch;	\
	SDWORD	order;			\
	UWORD	orderX,orderY;	\
	UWORD	orderX2,orderY2;\
	UDWORD	timeLastHit;	\
	UDWORD	targetID;		\
	UDWORD	secondaryOrder;	\
	SDWORD	action;			\
	UDWORD	actionX,actionY;\
	UDWORD	actionTargetID;	\
	UDWORD	actionStarted;	\
	UDWORD	actionPoints;	\
	UWORD	actionHeight;	\
	char	tarStatName[MAX_SAVE_NAME_SIZE];\
    UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission

typedef struct _save_droid_v20
{
	DROID_SAVE_V20;
} SAVE_DROID_V20;

#define DROID_SAVE_V21		\
	DROID_SAVE_V20;			\
	UDWORD	commandId

typedef struct _save_droid_v21
{
	DROID_SAVE_V21;
} SAVE_DROID_V21;

#define DROID_SAVE_V24		\
	DROID_SAVE_V21;			\
	SDWORD	resistance;		\
	SAVE_MOVE_CONTROL	sMove;	\
	SWORD		formationDir;	\
	SDWORD		formationX;	\
	SDWORD		formationY

typedef struct _save_droid_v24
{
	DROID_SAVE_V24;
} SAVE_DROID_V24;

//Watermelon: I need DROID_SAVE_V99...
#define DROID_SAVE_V99		\
	OBJECT_SAVE_V20;			\
	SAVE_COMPONENT	asBits[DROID_MAXCOMP]; \
	UDWORD		body;		\
	UBYTE		droidType;	\
	UDWORD		saveType;	\
	UDWORD		numWeaps;	\
	SAVE_WEAPON	asWeaps[TEMP_DROID_MAXPROGS];	\
	UDWORD		numKills;	\
	UWORD	turretRotation[DROID_MAXWEAPS];	\
	UWORD	turretPitch[DROID_MAXWEAPS];	\
	SDWORD	order;			\
	UWORD	orderX,orderY;	\
	UWORD	orderX2,orderY2;\
	UDWORD	timeLastHit;	\
	UDWORD	targetID;		\
	UDWORD	secondaryOrder;	\
	SDWORD	action;			\
	UDWORD	actionX,actionY;\
	UDWORD	actionTargetID;	\
	UDWORD	actionStarted;	\
	UDWORD	actionPoints;	\
	UWORD	actionHeight;	\
	char	tarStatName[MAX_SAVE_NAME_SIZE];\
	UDWORD	baseStructID;	\
	UBYTE	group;			\
	UBYTE	selected;		\
	UBYTE	cluster_unused;		\
	UBYTE	visible[MAX_PLAYERS];\
	UDWORD	died;			\
	UDWORD	lastEmission;         \
	UDWORD	commandId;            \
	SDWORD	resistance;           \
	SAVE_MOVE_CONTROL	sMove; \
	SWORD	formationDir;         \
	SDWORD	formationX;           \
	SDWORD	formationY

typedef struct _save_droid_v99
{
	DROID_SAVE_V99;
} SAVE_DROID_V99;

//Watermelon:V99 'test'
typedef struct _save_droid
{
	DROID_SAVE_V99;
} SAVE_DROID;


typedef struct _droidinit_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		quantity;
} DROIDINIT_SAVEHEADER;

typedef struct _save_droidinit
{
	OBJECT_SAVE_V19;
} SAVE_DROIDINIT;

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

typedef struct _save_structure_v2
{
	STRUCTURE_SAVE_V2;
} SAVE_STRUCTURE_V2;

#define STRUCTURE_SAVE_V12 \
	STRUCTURE_SAVE_V2; \
	UDWORD				factoryInc;			\
	UBYTE				loopsPerformed;		\
	UDWORD				powerAccrued;		\
	UDWORD				dummy2;			\
	UDWORD				droidTimeStarted;	\
	UDWORD				timeToBuild;		\
	UDWORD				timeStartHold

typedef struct _save_structure_v12
{
	STRUCTURE_SAVE_V12;
} SAVE_STRUCTURE_V12;

#define STRUCTURE_SAVE_V14 \
	STRUCTURE_SAVE_V12; \
	UBYTE	visible[MAX_PLAYERS]

typedef struct _save_structure_v14
{
	STRUCTURE_SAVE_V14;
} SAVE_STRUCTURE_V14;

#define STRUCTURE_SAVE_V15 \
	STRUCTURE_SAVE_V14; \
	char	researchName[MAX_SAVE_NAME_SIZE_V19]

typedef struct _save_structure_v15
{
	STRUCTURE_SAVE_V15;
} SAVE_STRUCTURE_V15;

#define STRUCTURE_SAVE_V17 \
	STRUCTURE_SAVE_V15;\
	SWORD				currentPowerAccrued

typedef struct _save_structure_v17
{
	STRUCTURE_SAVE_V17;
} SAVE_STRUCTURE_V17;

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

typedef struct _save_structure_v20
{
	STRUCTURE_SAVE_V20;
} SAVE_STRUCTURE_V20;

#define STRUCTURE_SAVE_V21 \
	STRUCTURE_SAVE_V20; \
	UDWORD				commandId

typedef struct _save_structure_v21
{
	STRUCTURE_SAVE_V21;
} SAVE_STRUCTURE_V21;

typedef struct _save_structure
{
	STRUCTURE_SAVE_V21;
} SAVE_STRUCTURE;


//PROGRAMS NEED TO BE REMOVED FROM DROIDS - 7/8/98
// multiPlayerID for templates needs to be saved - 29/10/98
#define TEMPLATE_SAVE_V2 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				ref; \
	UDWORD				player; \
	UBYTE				droidType; \
	char				asParts[DROID_MAXCOMP][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				numWeaps; \
	char				asWeaps[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				numProgs; \
	char				asProgs[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE_V19]

// multiPlayerID for templates needs to be saved - 29/10/98
#define TEMPLATE_SAVE_V14 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				ref; \
	UDWORD				player; \
	UBYTE				droidType; \
	char				asParts[DROID_MAXCOMP][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				numWeaps; \
	char				asWeaps[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE_V19]; \
	UDWORD				multiPlayerID

#define TEMPLATE_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UDWORD				ref; \
	UDWORD				player; \
	UBYTE				droidType; \
	char				asParts[DROID_MAXCOMP][MAX_SAVE_NAME_SIZE]; \
	UDWORD				numWeaps; \
	char				asWeaps[TEMP_DROID_MAXPROGS][MAX_SAVE_NAME_SIZE]; \
	UDWORD				multiPlayerID



typedef struct _save_template_v2
{
	TEMPLATE_SAVE_V2;
} SAVE_TEMPLATE_V2;

typedef struct _save_template_v14
{
	TEMPLATE_SAVE_V14;
} SAVE_TEMPLATE_V14;

typedef struct _save_template_v20
{
	TEMPLATE_SAVE_V20;
} SAVE_TEMPLATE_V20;

typedef struct _save_template
{
	TEMPLATE_SAVE_V20;
} SAVE_TEMPLATE;


#define FEATURE_SAVE_V2 \
	OBJECT_SAVE_V19

typedef struct _save_feature_v2
{
	FEATURE_SAVE_V2;
} SAVE_FEATURE_V2;

#define FEATURE_SAVE_V14 \
	FEATURE_SAVE_V2; \
	UBYTE	visible[MAX_PLAYERS]

typedef struct _save_feature_v14
{
	FEATURE_SAVE_V14;
} SAVE_FEATURE_V14;

#define FEATURE_SAVE_V20 \
	OBJECT_SAVE_V20; \
	UBYTE	visible[MAX_PLAYERS]

typedef struct _save_feature_v20
{
	FEATURE_SAVE_V20;
} SAVE_FEATURE_V20;

typedef struct _save_feature
{
	FEATURE_SAVE_V20;
} SAVE_FEATURE;


#define COMPLIST_SAVE_V6 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				type; \
	UBYTE				state; \
	UBYTE				player

#define COMPLIST_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				type; \
	UBYTE				state; \
	UBYTE				player


typedef struct _save_compList_v6
{
	COMPLIST_SAVE_V6;
} SAVE_COMPLIST_V6;

typedef struct _save_compList_v20
{
	COMPLIST_SAVE_V20;
} SAVE_COMPLIST_V20;

typedef struct _save_compList
{
	COMPLIST_SAVE_V20;
} SAVE_COMPLIST;





#define STRUCTLIST_SAVE_V6 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				state; \
	UBYTE				player

#define STRUCTLIST_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				state; \
	UBYTE				player

typedef struct _save_structList_v6
{
	STRUCTLIST_SAVE_V6;
} SAVE_STRUCTLIST_V6;

typedef struct _save_structList_v20
{
	STRUCTLIST_SAVE_V20;
} SAVE_STRUCTLIST_V20;

typedef struct _save_structList
{
	STRUCTLIST_SAVE_V20;
} SAVE_STRUCTLIST;


#define RESEARCH_SAVE_V8 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				possible[MAX_PLAYERS]; \
	UBYTE				researched[MAX_PLAYERS]; \
	UDWORD				currentPoints[MAX_PLAYERS]

#define RESEARCH_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				possible[MAX_PLAYERS]; \
	UBYTE				researched[MAX_PLAYERS]; \
	UDWORD				currentPoints[MAX_PLAYERS]


typedef struct _save_research_v8
{
	RESEARCH_SAVE_V8;
} SAVE_RESEARCH_V8;

typedef struct _save_research_v20
{
	RESEARCH_SAVE_V20;
} SAVE_RESEARCH_V20;

typedef struct _save_research
{
	RESEARCH_SAVE_V20;
} SAVE_RESEARCH;

typedef struct _save_message
{
	MESSAGE_TYPE	type;			//The type of message
	BOOL			bObj;
	char			name[MAX_GAME_STR_SIZE];
	UDWORD			objId;					//Id for Proximity messages!
	BOOL			read;					//flag to indicate whether message has been read
	UDWORD			player;					//which player this message belongs to

} SAVE_MESSAGE;

typedef struct _save_message_v36
{
	MESSAGE_TYPE	type;			//The type of message
	BOOL			bObj;
	char			name[MAX_GAME_STR_SIZE];
	UDWORD			objId;					//Id for Proximity messages!
	BOOL			read;					//flag to indicate whether message has been read
	UDWORD			player;					//which player this message belongs to
	MSG_DATA_TYPE	dataType;				//actual type of pViewData
	UDWORD			locX,locY;
	SDWORD			sender;					//sender of the message
} SAVE_MESSAGE_36;

typedef struct _save_flag_v18
{
	POSITION_TYPE	type;				/*the type of position obj - FlagPos or ProxDisp*/
	UDWORD			frameNumber;		/*when the Position was last drawn*/
	UDWORD			screenX;			/*screen coords and radius of Position imd */
	UDWORD			screenY;
	UDWORD			screenR;
	UDWORD			player;				/*which player the Position belongs to*/
	BOOL			selected;			/*flag to indicate whether the Position */
	Vector3i		coords;							//the world coords of the Position
	UBYTE		factoryInc;						//indicates whether the first, second etc factory
	UBYTE		factoryType;					//indicates whether standard, cyborg or vtol factory
	UBYTE		dummyNOTUSED;						//sub value. needed to order production points.
	UBYTE		dummyNOTUSED2;
} SAVE_FLAG_V18;

typedef struct _save_flag
{
	POSITION_TYPE	type;				/*the type of position obj - FlagPos or ProxDisp*/
	UDWORD			frameNumber;		/*when the Position was last drawn*/
	UDWORD			screenX;			/*screen coords and radius of Position imd */
	UDWORD			screenY;
	UDWORD			screenR;
	UDWORD			player;				/*which player the Position belongs to*/
	BOOL			selected;			/*flag to indicate whether the Position */
	Vector3i		coords;							//the world coords of the Position
	UBYTE		factoryInc;						//indicates whether the first, second etc factory
	UBYTE		factoryType;					//indicates whether standard, cyborg or vtol factory
	UBYTE		dummyNOTUSED;						//sub value. needed to order production points.
	UBYTE		dummyNOTUSED2;
	UDWORD		repairId;
} SAVE_FLAG;

//PRODUCTION_RUN		asProductionRun[NUM_FACTORY_TYPES][MAX_FACTORY][MAX_PROD_RUN];
typedef struct _save_production
{
	UBYTE						quantity;			//number to build
	UBYTE						built;				//number built on current run
	UDWORD						multiPlayerID;		//template to build
} SAVE_PRODUCTION;

#define STRUCTLIMITS_SAVE_V2 \
	char				name[MAX_SAVE_NAME_SIZE_V19]; \
	UBYTE				limit; \
	UBYTE				player

typedef struct _save_structLimits_v2
{
	STRUCTLIMITS_SAVE_V2;
} SAVE_STRUCTLIMITS_V2;

#define STRUCTLIMITS_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UBYTE				limit; \
	UBYTE				player

typedef struct _save_structLimits_v20
{
	STRUCTLIMITS_SAVE_V20;
} SAVE_STRUCTLIMITS_V20;

typedef struct _save_structLimits
{
	STRUCTLIMITS_SAVE_V20;
} SAVE_STRUCTLIMITS;


#define COMMAND_SAVE_V20 \
	UDWORD				droidID

typedef struct _save_command_v20
{
	COMMAND_SAVE_V20;
} SAVE_COMMAND_V20;

typedef struct _save_command
{
	COMMAND_SAVE_V20;
} SAVE_COMMAND;


/* The different types of droid */
typedef enum _droid_save_type
{
	DROID_NORMAL,	// Weapon droid
	DROID_ON_TRANSPORT,
} DROID_SAVE_TYPE;

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/
extern uint32_t unsynchObjID;  // unique ID creation thing..
extern uint32_t synchObjID;    // unique ID creation thing..

static UDWORD			saveGameVersion = 0;
static BOOL				saveGameOnMission = false;
static SAVE_GAME		saveGameData;
static UDWORD			oldestSaveGameVersion = CURRENT_VERSION_NUM;
static UDWORD			validityKey = 0;

static UDWORD	savedGameTime;
static UDWORD	savedObjId;

//static UDWORD			HashedName;
//static STRUCTURE *psStructList;
//static FEATURE *psFeatureList;
//static FLAG_POSITION **ppsCurrentFlagPosLists;
static SDWORD	startX, startY;
static UDWORD   width, height;
static UDWORD	gameType;
static BOOL IsScenario;
/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/
static bool gameLoadV7(PHYSFS_file* fileHandle);
static bool gameLoadV(PHYSFS_file* fileHandle, unsigned int version);
static bool writeGameFile(const char* fileName, SDWORD saveType);
static bool writeMapFile(const char* fileName);

static BOOL loadSaveDroidInitV2(char *pFileData, UDWORD filesize,UDWORD quantity);

static BOOL loadSaveDroidInit(char *pFileData, UDWORD filesize);

static BOOL loadSaveDroid(char *pFileData, UDWORD filesize, DROID **ppsCurrentDroidLists);
static BOOL loadSaveDroidV11(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists);
static BOOL loadSaveDroidV19(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists);
static BOOL loadSaveDroidV(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists);
static BOOL loadDroidSetPointers(void);
static BOOL writeDroidFile(char *pFileName, DROID **ppsCurrentDroidLists);

static BOOL loadSaveStructure(char *pFileData, UDWORD filesize);
static BOOL loadSaveStructureV7(char *pFileData, UDWORD filesize, UDWORD numStructures);
static BOOL loadSaveStructureV19(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version);
static BOOL loadSaveStructureV(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version);
static BOOL loadStructSetPointers(void);
static BOOL writeStructFile(char *pFileName);

static BOOL loadSaveTemplate(char *pFileData, UDWORD filesize);
static BOOL loadSaveTemplateV7(char *pFileData, UDWORD filesize, UDWORD numTemplates);
static BOOL loadSaveTemplateV14(char *pFileData, UDWORD filesize, UDWORD numTemplates);
static BOOL loadSaveTemplateV(char *pFileData, UDWORD filesize, UDWORD numTemplates);
static BOOL writeTemplateFile(char *pFileName);

static BOOL loadSaveFeature(char *pFileData, UDWORD filesize);
static BOOL loadSaveFeatureV14(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version);
static BOOL loadSaveFeatureV(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version);
static BOOL writeFeatureFile(char *pFileName);

static BOOL writeTerrainTypeMapFile(char *pFileName);

static BOOL loadSaveCompList(char *pFileData, UDWORD filesize);
static BOOL loadSaveCompListV9(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version);
static BOOL loadSaveCompListV(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version);
static BOOL writeCompListFile(char *pFileName);

static BOOL loadSaveStructTypeList(char *pFileData, UDWORD filesize);
static BOOL loadSaveStructTypeListV7(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL loadSaveStructTypeListV(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL writeStructTypeListFile(char *pFileName);

static BOOL loadSaveResearch(char *pFileData, UDWORD filesize);
static BOOL loadSaveResearchV8(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL loadSaveResearchV(char *pFileData, UDWORD filesize, UDWORD numRecords);
static BOOL writeResearchFile(char *pFileName);

static BOOL loadSaveMessage(char *pFileData, UDWORD filesize, SWORD levelType);
static BOOL loadSaveMessage36(char *pFileData, UDWORD filesize, UDWORD numMessages, UDWORD version, SWORD levelType);
static BOOL writeMessageFile(char *pFileName);

static BOOL loadSaveFlag(char *pFileData, UDWORD filesize);
static BOOL loadSaveFlagV(char *pFileData, UDWORD filesize, UDWORD numFlags, UDWORD version);
static BOOL writeFlagFile(char *pFileName);

static BOOL loadSaveProduction(char *pFileData, UDWORD filesize);
static BOOL loadSaveProductionV(char *pFileData, UDWORD filesize, UDWORD version);
static BOOL writeProductionFile(char *pFileName);

static BOOL loadSaveStructLimits(char *pFileData, UDWORD filesize);
static BOOL loadSaveStructLimitsV19(char *pFileData, UDWORD filesize, UDWORD numLimits);
static BOOL loadSaveStructLimitsV(char *pFileData, UDWORD filesize, UDWORD numLimits);
static BOOL writeStructLimitsFile(char *pFileName);

static BOOL readFiresupportDesignators(char *pFileName);
static BOOL writeFiresupportDesignators(char *pFileName);

static BOOL writeScriptState(char *pFileName);

static BOOL getNameFromComp(UDWORD compType, char *pDest, UDWORD compIndex);

//adjust the name depending on type of save game and whether resourceNames are used
static BOOL getSaveObjectName(char *pName);
static bool gameLoad(const char* fileName);

/* set the global scroll values to use for the save game */
static void setMapScroll(void);

static bool gameLoad(const char* fileName);

static char *getSaveStructNameV19(SAVE_STRUCTURE_V17 *psSaveStructure)
{
	return(psSaveStructure->name);
}

static char *getSaveStructNameV(SAVE_STRUCTURE *psSaveStructure)
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
BOOL loadMissionExtras(const char *pGameToLoad, SWORD levelType)
{
	char			aFileName[256];
	UDWORD			fileExten, fileSize;
	char			*pFileData = NULL;

	strcpy(aFileName, pGameToLoad);
	fileExten = strlen(pGameToLoad) - 3;
	aFileName[fileExten - 1] = '\0';
	strcat(aFileName, "/");

	if (saveGameVersion >= VERSION_11)
	{
		//if user save game then load up the messages AFTER any droids or structures are loaded
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "messtate.bjo");
			// Load in the chosen file data
			pFileData = fileLoadBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				//load the message status data
				if (pFileData)
				{
					if (!loadSaveMessage(pFileData, fileSize, levelType))
					{
						debug( LOG_NEVER, "loadMissionExtras: Fail 2\n" );
						return false;
					}
				}
			}

		}
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// UserSaveGame ... this is true when you are loading a players save game
BOOL loadGame(const char *pGameToLoad, BOOL keepObjects, BOOL freeMem, BOOL UserSaveGame)
{
	char			aFileName[256];
	UDWORD			fileExten, fileSize, pl;
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
			apsOilList[0] = NULL;
		}
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
		}
		mission.apsOilList[0] = NULL;
		mission.apsSensorList[0] = NULL;

		//JPS 25 feb
		//initialise upgrades
		//initialise the structure upgrade arrays
		memset(asStructureUpgrade, 0, sizeof(asStructureUpgrade));
		memset(asWallDefenceUpgrade, 0, sizeof(asWallDefenceUpgrade));
		memset(asResearchUpgrade, 0, sizeof(asResearchUpgrade));
		memset(asPowerUpgrade, 0, sizeof(asPowerUpgrade));
		memset(asRepairFacUpgrade, 0, sizeof(asRepairFacUpgrade));
		memset(asProductionUpgrade, 0, sizeof(asProductionUpgrade));
		memset(asReArmUpgrade, 0, sizeof(asReArmUpgrade));

		//initialise the upgrade structures
		memset(asWeaponUpgrade, 0, sizeof(asWeaponUpgrade));
		memset(asSensorUpgrade, 0, sizeof(asSensorUpgrade));
		memset(asECMUpgrade, 0, sizeof(asECMUpgrade));
		memset(asRepairUpgrade, 0, sizeof(asRepairUpgrade));
		memset(asBodyUpgrade, 0, sizeof(asBodyUpgrade));
		//JPS 25 feb
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
			PIELIGHT colour;

			offWorldKeepLists	= saveGameData.offWorldKeepLists;
			setRubbleTile(saveGameData.RubbleTile);
			setUnderwaterTile(saveGameData.WaterTile);
			if (saveGameData.fogState == 0)//no fog
			{
				pie_EnableFog(false);
				fogStatus = 0;
			}
			else if (saveGameData.fogState == 1)//fog using old code assume background and depth
			{
				if (war_GetFog())
				{
					pie_EnableFog(true);
				}
				else
				{
					pie_EnableFog(false);
				}
				fogStatus = FOG_BACKGROUND + FOG_DISTANCE;
			}
			else//version 18+ fog
			{
				if (war_GetFog())
				{
					pie_EnableFog(true);
				}
				else
				{
					pie_EnableFog(false);
				}
				fogStatus = saveGameData.fogState;
				fogStatus &= FOG_FLAGS;
			}
			colour.rgba = saveGameData.fogColour;
			pie_SetFogColour(colour);
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
/*			setPlayCountDown(saveGameData.bPlayCountDown);
			setPlayerHasWon(saveGameData.bPlayerHasWon);
			setPlayerHasLost(saveGameData.bPlayerHasLost);*/
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

			game			= saveGameData.sGame;
			productionPlayer= selectedPlayer;
			bMultiPlayer	= saveGameData.multiPlayer;
			bMultiMessages	= bMultiPlayer;
			cmdDroidMultiExpBoost(true);

			NetPlay.bComms = (saveGameData.sNetPlay).bComms;
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				strcpy((NetPlay.players[i]).name, ((saveGameData.sNetPlay).players[i]).name);
				if ((saveGameData.sGame).skDiff[i] == UBYTE_MAX || (game.type == CAMPAIGN && i == 0))
				{
					(NetPlay.players[i]).allocated = true;
				}
				else
				{
					(NetPlay.players[i]).allocated = false;
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

	//initialise the scroll values
	//startX = startY = width = height = 0;

	//before loading the data - turn power off so don't get any power low warnings
	powerCalculated = false;

	/* Load in the chosen file data */
	strcpy(aFileName, pGameToLoad);
	fileExten = strlen(aFileName) - 3;			// hack - !
	aFileName[fileExten - 1] = '\0';
	strcat(aFileName, "/");

	//the terrain type WILL only change with Campaign changes (well at the moment!)
	//if (freeMem) - this now works for Cam Start and Cam Change
	if (gameType != GTYPE_SCENARIO_EXPAND
	 || UserSaveGame)
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
		//NOT ANY MORE - use multiPlayerID (unique template id) to prevent duplicate's being loaded
		//Only want to clear the lists in the final version of the game

//#ifdef FINAL
		//first clear the templates
//		droidTemplateShutDown();
		// only free player 0 templates - keep all the others from the stats
//#define ALLOW_ACCESS_TEMPLATES
#ifndef ALLOW_ACCESS_TEMPLATES
		{
			DROID_TEMPLATE	*pTemplate, *pNext;
			for(pTemplate = apsDroidTemplates[0]; pTemplate != NULL;
				pTemplate = pNext)
			{
				pNext = pTemplate->psNext;
				free(pTemplate);
			}
			apsDroidTemplates[0] = NULL;
		}
#endif

		// In Multiplayer, clear templates out first.....
		if(	bMultiPlayer)
		{
			for(inc=0;inc<MAX_PLAYERS;inc++)
			{
				while(apsDroidTemplates[inc])				// clear the old template out.
				{
					DROID_TEMPLATE	*psTempl;
					psTempl = apsDroidTemplates[inc]->psNext;
					free(apsDroidTemplates[inc]);
					apsDroidTemplates[inc] = psTempl;
				}
			}
		}

//load in the templates
		aFileName[fileExten] = '\0';
		strcat(aFileName, "templ.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail20\n" );
			goto error;
		}
		//load the data into apsTemplates
		if (!loadSaveTemplate(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail22\n" );
			goto error;
		}
		//JPS 25 feb (reverse templates moved from here)
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
		if (!mapLoad(aFileName))
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
		strcat(aFileName, "mfeat.bjo");
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

		//load in the mission structures
		initStructLimits();
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


		if (bMultiPlayer)
		{
			for(pl=0;pl<MAX_PLAYERS;pl++)// ajl. must do for every player to stop multiplay/pc players going gaga.
			{
				//reverse the structure lists so the Research Facilities are in the same order as when saved
				reverseObjectList((BASE_OBJECT**)&apsStructLists[pl]);
			}
		}

		//load in the mission droids
		aFileName[fileExten] = '\0';

		if (saveGameVersion < VERSION_27)//V27
		{
			strcat(aFileName, "mdroid.bjo");
		}
		else
		{
			strcat(aFileName, "munit.bjo");
		}
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (loadFileToBufferNoError(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			//load the data into mission.apsDroidLists
			if (!loadSaveDroid(pFileData, fileSize, apsDroidLists))
			{
				debug( LOG_NEVER, "loadgame: Fail12\n" );
				goto error;
			}
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

		//load in the flag list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mflagstate.bjo");
		// Load in the chosen file data
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadMissionExtras: Fail 3\n" );
			return false;
		}


		//load the flag status data
		if (pFileData)
		{
			if (!loadSaveFlag(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadMissionExtras: Fail 4\n" );
				return false;
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
		if (!mapLoad(aFileName))
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
			strcat(aFileName, "fxstate.tag");

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
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		//load in the research list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "resstate.bjo");
		// Load in the chosen file data
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail32\n" );
			goto error;
		}

		//load the research status data
		if (pFileData)
		{
			if (!loadSaveResearch(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail33\n" );
				goto error;
			}
		}
	}

	if(IsScenario==true)
	{
		//load in the droid initialisation file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "dinit.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail8\n" );
			goto error;
		}


		if(!loadSaveDroidInit(pFileData,fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail10\n" );
			goto error;
		}
	}
	else
	{
		//load in the droids
		aFileName[fileExten] = '\0';
		if (saveGameVersion < VERSION_27)//V27
		{
			strcat(aFileName, "droid.bjo");
		}
		else
		{
			strcat(aFileName, "unit.bjo");
		}
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail11\n" );
			goto error;
		}

		//load the data into apsDroidLists
		if (!loadSaveDroid(pFileData, fileSize, apsDroidLists))
		{
			debug( LOG_NEVER, "loadgame: Fail12\n" );
			goto error;
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

		if (saveGameVersion >= 12)
		{
			if (!saveGameOnMission)
			{
				//load in the mission droids
				aFileName[fileExten] = '\0';
				if (saveGameVersion < VERSION_27)//V27
				{
					strcat(aFileName, "mdroid.bjo");
				}
				else
				{
					strcat(aFileName, "munit.bjo");
				}
				/* Load in the chosen file data */
				pFileData = fileLoadBuffer;
				if (loadFileToBufferNoError(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize)) {
					//load the data into mission.apsDroidLists
					if (!loadSaveDroid(pFileData, fileSize, mission.apsDroidLists))
					{
						debug( LOG_NEVER, "loadgame: Fail12\n" );
						goto error;
					}
				}

			}
		}
	}

	if (saveGameVersion >= VERSION_23)
	{
		//load in the limbo droids
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limbo.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (loadFileToBufferNoError(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			//load the data into apsDroidLists
			if (!loadSaveDroid(pFileData, fileSize, apsLimboDroids))
			{
				debug( LOG_NEVER, "loadgame: Fail12\n" );
				goto error;
			}
		}

	}

	//load in the features -do before the structures
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

	//load droid templates moved from here to BEFORE any structures loaded in

	//load in the structures
	initStructLimits();
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

	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		for(pl=0;pl<MAX_PLAYERS;pl++)	// ajl. must do for every player to stop multiplay/pc players going gaga.
		{
			//reverse the structure lists so the Research Facilities are in the same order as when saved
			reverseObjectList((BASE_OBJECT**)&apsStructLists[pl]);
		}
	}

	//if user save game then load up the current level for structs and components
	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		//load in the component list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "compl.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail26\n" );
			goto error;
		}


		//load the component list data
		if (pFileData)
		{
			if (!loadSaveCompList(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail28\n" );
				goto error;
			}
		}
		//load in the structure type list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "strtype.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail29\n" );
			goto error;
		}


		//load the structure type list data
		if (pFileData)
		{
			if (!loadSaveStructTypeList(pFileData, fileSize))
			{
				debug( LOG_NEVER, "loadgame: Fail31\n" );
				goto error;
			}
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

	if (saveGameVersion > VERSION_12)
	{
		//if user save game then load up the Visibility
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "prodstate.bjo");
			// Load in the chosen file data
			pFileData = fileLoadBuffer;
			if (loadFileToBufferNoError(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				//load the visibility data
				if (pFileData)
				{
					if (!loadSaveProduction(pFileData, fileSize))
					{
						debug( LOG_NEVER, "loadgame: Fail33\n" );
						goto error;
					}
				}
			}

		}
	}

	if (saveGameVersion >= VERSION_16)
	{
		//if user save game then load up the FX
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the message list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "score.tag");

			// Load the fx data from the chosen file
			if (!readScoreData(aFileName))
			{
				debug( LOG_NEVER, "loadgame: Fail33\n" );
				goto error;
			}
		}
	}

	if (saveGameVersion >= VERSION_12)
	{
		//if user save game then load up the flags AFTER any droids or structures are loaded
		if ((gameType == GTYPE_SAVE_START) ||
			(gameType == GTYPE_SAVE_MIDMISSION))
		{
			//load in the flag list file
			aFileName[fileExten] = '\0';
			strcat(aFileName, "flagstate.bjo");
			// Load in the chosen file data
			pFileData = fileLoadBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug( LOG_NEVER, "loadMissionExtras: Fail 3\n");
				return false;
			}


			//load the flag status data
			if (pFileData)
			{
				if (!loadSaveFlag(pFileData, fileSize))
				{
					debug( LOG_NEVER, "loadMissionExtras: Fail 4\n" );
					return false;
				}
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
			strcat(aFileName, "firesupport.tag");

			if (!readFiresupportDesignators(aFileName))
			{
				debug( LOG_NEVER, "loadMissionExtras: readFiresupportDesignators(%s) failed\n", aFileName );
				return false;
			}
		}
	}

	if ((saveGameVersion >= VERSION_15) && UserSaveGame)
	{
		//load in the mission structures
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limits.bjo");
		/* Load in the chosen file data */
		pFileData = fileLoadBuffer;
		if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail17\n" );
			goto error;
		}

		//load the data into apsStructLists
		if (!loadSaveStructLimits(pFileData, fileSize))
		{
			debug( LOG_NEVER, "loadgame: Fail19\n" );
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

	if ((gameType == GTYPE_SAVE_START) ||
		(gameType == GTYPE_SAVE_MIDMISSION))
	{
		for(pl=0;pl<MAX_PLAYERS;pl++)	// ajl. must do for every player to stop multiplay/pc players going gaga.
		{
			//reverse the structure lists so the Research Facilities are in the same order as when saved
			reverseTemplateList((DROID_TEMPLATE**)&apsDroidTemplates[pl]);
		}

		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&apsLimboDroids[pl]);
		}

		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&apsDroidLists[pl]);
		}

		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&mission.apsDroidLists[pl]);
		}

		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the struct lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&mission.apsStructLists[pl]);
		}

		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&apsFeatureLists[pl]);
		}

		for(pl=0;pl<MAX_PLAYERS;pl++)
		{
			//reverse the droid lists so selections occur in the same order
			reverseObjectList((BASE_OBJECT**)&mission.apsFeatureLists[pl]);
		}
	}

	//turn power on for rest of game
	powerCalculated = true;

	if (saveGameVersion > VERSION_12)
	{
		if (!keepObjects)//only reset the pointers if they were set
		{
			//reset the object pointers in the droid target lists
			loadDroidSetPointers();
		}
	}

	if (saveGameVersion > VERSION_20)
	{
		if (!keepObjects)//only reset the pointers if they were set
		{
			//reset the object pointers in the structure lists
			loadStructSetPointers();
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
	intCheckResearchButton();

	//set up the mission countdown flag
	setMissionCountDown();

	/* Start the game clock */
	gameTimeStart();

	//after the clock has been reset need to check if any res_extractors are active
	checkResExtractorsActive();

//	initViewPosition();

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
		debug( LOG_NEVER, "loadgame: ERROR\n" );

	/* Clear all the objects off the map and free up the map memory */
	freeAllDroids();
	freeAllStructs();
	freeAllFeatures();
	droidTemplateShutDown();
	if (psMapTiles)
	{
//		free(psMapTiles);
	}
	psMapTiles = NULL;

	/* Start the game clock */
	gameTimeStart();

	return false;
}
// -----------------------------------------------------------------------------------------

// Modified by AlexL , now takes a filename, with no popup....
BOOL saveGame(char *aFileName, GAME_TYPE saveType)
{
	UDWORD			fileExtension;
	DROID			*psDroid, *psNext;
	char			CurrentFileName[PATH_MAX] = {'\0'};

	ASSERT_OR_RETURN(false, aFileName && strlen(aFileName) > 4, "Bad savegame filename");
	sstrcpy(CurrentFileName, aFileName);
	debug(LOG_WZ, "saveGame: %s", CurrentFileName);

	fileExtension = strlen(CurrentFileName) - 3;
	gameTimeStop();

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

	//create the droids filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "unit.bjo");
	/*Write the current droid lists to the file*/
	if (!writeDroidFile(CurrentFileName,apsDroidLists))
	{
		debug(LOG_ERROR, "saveGame: writeDroidFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the structures filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "struct.bjo");
	/*Write the data to the file*/
	if (!writeStructFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the templates filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "templ.bjo");
	/*Write the data to the file*/
	if (!writeTemplateFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeTemplateFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the features filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "feat.bjo");
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
	strcat(CurrentFileName, "limits.bjo");
	/*Write the data to the file*/
	if (!writeStructLimitsFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructLimitsFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the component lists filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "compl.bjo");
	/*Write the data to the file*/
	if (!writeCompListFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeCompListFile(\"%s\") failed", CurrentFileName);
		goto error;
	}
	//create the structure type lists filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "strtype.bjo");
	/*Write the data to the file*/
	if (!writeStructTypeListFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructTypeListFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the research filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "resstate.bjo");
	/*Write the data to the file*/
	if (!writeResearchFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeResearchFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "messtate.bjo");
	/*Write the data to the file*/
	if (!writeMessageFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeMessageFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the proximity message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "proxstate.bjo");
	/*Write the data to the file*/
	if (!writeMessageFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeMessageFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "visstate.bjo");
	/*Write the data to the file*/
	if (!writeVisibilityData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeVisibilityData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "prodstate.bjo");
	/*Write the data to the file*/
	if (!writeProductionFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeProductionFile(\"%s\") failed", CurrentFileName);
		goto error;
	}


	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "fxstate.tag");
	/*Write the data to the file*/
	if (!writeFXData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeFXData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//added at V15 save
	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "score.tag");
	/*Write the data to the file*/
	if (!writeScoreData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeScoreData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "flagstate.bjo");
	/*Write the data to the file*/
	if (!writeFlagFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeFlagFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "firesupport.tag");
	/*Write the data to the file*/
	if (!writeFiresupportDesignators(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeFiresupportDesignators(\"%s\") failed", CurrentFileName);
		goto error;
	}

	// save the script state if necessary
	if (saveType == GTYPE_SAVE_MIDMISSION)
	{
		CurrentFileName[fileExtension-1] = '\0';
		strcat(CurrentFileName, ".es");
		/*Write the data to the file*/
		if (!writeScriptState(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeScriptState(\"%s\") failed", CurrentFileName);
			goto error;
		}
	}

	//create the droids filename
	CurrentFileName[fileExtension-1] = '\0';
	strcat(CurrentFileName, "/munit.bjo");
	/*Write the swapped droid lists to the file*/
	if (!writeDroidFile(CurrentFileName, mission.apsDroidLists))
	{
		debug(LOG_ERROR, "saveGame: writeDroidFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//21feb now done always
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
			orderDroid(psDroid, DORDER_STOP);
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
		strcat(CurrentFileName, "mstruct.bjo");
		/*Write the data to the file*/
		if (!writeStructFile(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeStructFile(\"%s\") failed", CurrentFileName);
			goto error;
		}

		//create the features filename
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "mfeat.bjo");
		/*Write the data to the file*/
		if (!writeFeatureFile(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeFeatureFile(\"%s\") failed", CurrentFileName);
			goto error;
		}

		//create the message filename
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "mflagstate.bjo");
		/*Write the data to the file*/
		if (!writeFlagFile(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeFlagFile(\"%s\") failed", CurrentFileName);
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
		for(i = 0; i < MaxGames; i++) {
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwSize);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwFlags);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwMaxPlayers);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwCurrentPlayers);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUserFlags[0]);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUserFlags[1]);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUserFlags[2]);
			endian_sdword(&psSaveGame->sNetPlay.games[i].desc.dwUserFlags[3]);
		}
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
		endian_udword(&psSaveGame->fogColour);
		endian_udword(&psSaveGame->fogState);
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

//	savedGameTime = saveGame.gameTime;

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
void game_SetValidityKey(UDWORD keys)
{
	validityKey = validityKey|keys;
	return;
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
		if (!levLoadData(aLevelName, saveGameName, gameType))
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
	char			date[MAX_STR_LENGTH];

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
	else if (version <= VERSION_37) // versions 35-37 use the same save game format
	{
		//if (PHYSFS_read(fileHandle, &saveGameData, sizeof(SAVE_GAME_V35), 1) != 1)
		if (!deserializeSaveGameV35Data(fileHandle, (SAVE_GAME_V35*) &saveGameData))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, PHYSFS_getLastError());

			return false;
		}
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
		PIELIGHT colour;

		offWorldKeepLists	= saveGameData.offWorldKeepLists;
		setRubbleTile(saveGameData.RubbleTile);
		setUnderwaterTile(saveGameData.WaterTile);
		if (saveGameData.fogState == 0)//no fog
		{
			pie_EnableFog(false);
			fogStatus = 0;
		}
		else if (saveGameData.fogState == 1)//fog using old code assume background and depth
		{
			if (war_GetFog())
			{
				pie_EnableFog(true);
			}
			else
			{
				pie_EnableFog(false);
			}
			fogStatus = FOG_BACKGROUND + FOG_DISTANCE;
		}
		else//version 18+ fog
		{
			if (war_GetFog())
			{
				pie_EnableFog(true);
			}
			else
			{
				pie_EnableFog(false);
			}
			fogStatus = saveGameData.fogState;
			fogStatus &= FOG_FLAGS;
		}
		colour.rgba = saveGameData.fogColour;
		pie_SetFogColour(colour);
	}

	if (version >= VERSION_17)
	{
		unsynchObjID = (saveGameData.objId + 1)/2;  // Make new object ID start at savedObjId*8.
		synchObjID   = saveGameData.objId*4;        // Make new object ID start at savedObjId*8.
		savedObjId = saveGameData.objId;
	}

	if (version >= VERSION_18)//version 18
	{
		validityKey = saveGameData.validityKey;
		oldestSaveGameVersion = saveGameData.oldestVersion;
		if (oldestSaveGameVersion > version)
		{
			oldestSaveGameVersion = version;
			validityKey = validityKey|VALIDITYKEY_VERSION;
		}
		else if (oldestSaveGameVersion < version)
		{
			validityKey = validityKey|VALIDITYKEY_VERSION;
		}

		strcpy(date,__DATE__);
		ASSERT( strlen(date)<MAX_STR_LENGTH,"BuildDate; String error" );
		if (strcmp(saveGameData.buildDate,date) != 0)
		{
//			ASSERT( gameType != GTYPE_SAVE_MIDMISSION,"Mid-game save out of date. Continue with caution." );
			debug( LOG_NEVER, "saveGame build date differs;\nsavegame %s\n build    %s\n", saveGameData.buildDate, date );
			validityKey = validityKey|VALIDITYKEY_DATE;
			if (gameType == GTYPE_SAVE_MIDMISSION)
			{
				validityKey = validityKey|VALIDITYKEY_MID_GAME;
			}
		}
	}
	else
	{
		debug( LOG_NEVER, "saveGame build date differs;\nsavegame pre-Version 18 (%s)\n build    %s\n", saveGameData.buildDate, date );
		oldestSaveGameVersion = 1;
		validityKey = VALIDITYKEY_DATE;
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
		}


		IsScenario = false;
		//copy the level name across
		sstrcpy(aLevelName, saveGameData.levelName);
		//load up the level dataset
		if (!levLoadData(aLevelName, saveGameName, gameType))
		{
			return false;
		}

		if (saveGameVersion >= VERSION_33)
		{
			PLAYERSTATS		playerStats;

			bMultiPlayer	= saveGameData.multiPlayer;
			bMultiMessages	= bMultiPlayer;
			productionPlayer = selectedPlayer;
			game			= saveGameData.sGame;
			cmdDroidMultiExpBoost(true);

			NetPlay.bComms = (saveGameData.sNetPlay).bComms;
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				strcpy((NetPlay.players[i]).name, ((saveGameData.sNetPlay).players[i]).name);
				if ((saveGameData.sGame).skDiff[i] == UBYTE_MAX || (game.type == CAMPAIGN && i == 0))
				{
					(NetPlay.players[i]).allocated = true;
				}
				else
				{
					(NetPlay.players[i]).allocated = false;
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
		saveGame.power[i].extractedPower = 0; // UNUSED
	}

	//camera position
	disp3d_getView(&(saveGame.currentPlayerPos));

	//mission data
//	saveGame.missionStartTime =		mission.startTime;
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
	saveGame.fogColour	= pie_GetFogColour().rgba;
	saveGame.fogState	= fogStatus;
	if(pie_GetFogEnabled())
	{
		saveGame.fogState	= fogStatus | FOG_ENABLED;
	}

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
	ASSERT(strlen(__DATE__) < MAX_STR_LENGTH, "BuildDate; String error" );
	strcpy(saveGame.buildDate, __DATE__);
	saveGame.oldestVersion = oldestSaveGameVersion;
	saveGame.validityKey = validityKey;

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
BOOL loadSaveDroidInit(char *pFileData, UDWORD filesize)
{
	DROIDINIT_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (DROIDINIT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'd' || psHeader->aFileType[1] != 'i' ||
		psHeader->aFileType[2] != 'n' || psHeader->aFileType[3] != 't')	{
		debug( LOG_ERROR, "loadSaveUnitInit: Incorrect file type" );

		return false;
	}

	/* DROIDINIT_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += DROIDINIT_HEADER_SIZE;

	debug(LOG_SAVE, "fileversion is %u ", psHeader->version);

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "UnitInit; unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveDroidInitV2(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported save format version %d", psHeader->version);

		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveDroidInitV2(char *pFileData, UDWORD filesize,UDWORD quantity)
{
	SAVE_DROIDINIT *pDroidInit;
	DROID_TEMPLATE *psTemplate;
	DROID *psDroid;
	UDWORD i;
	UDWORD NumberOfSkippedDroids = 0;

	pDroidInit = (SAVE_DROIDINIT*)pFileData;

	for(i=0; i<quantity; i++)
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
			ASSERT(psTemplate != NULL, "Invalid template pointer");
			psDroid = reallyBuildDroid(psTemplate, (pDroidInit->x & ~TILE_MASK) + TILE_UNITS/2, (pDroidInit->y  & ~TILE_MASK) + TILE_UNITS/2, pDroidInit->player, false);

			if (psDroid)
			{
				psDroid->id = pDroidInit->id;
				psDroid->rot.direction = DEG(pDroidInit->direction);
				addDroid(psDroid, apsDroidLists);
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
			return i;
		}
	}
	ASSERT(false, "Found no player position for player %d", (int)OldNumber);
	return 0;
}

// -----------------------------------------------------------------------------------------
/*This is *ALWAYS* called by a User Save Game */
BOOL loadSaveDroid(char *pFileData, UDWORD filesize, DROID **ppsCurrentDroidLists)
{
	DROID_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (DROID_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'd' || psHeader->aFileType[1] != 'r' ||
		psHeader->aFileType[2] != 'o' || psHeader->aFileType[3] != 'd')
	{
		debug( LOG_ERROR, "loadSaveUnit: Incorrect file type" );

		return false;
	}

	/* DROID_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += DROID_HEADER_SIZE;

	debug(LOG_SAVE, "fileversion is %u ", psHeader->version);

	/* Check the file version */
	if (psHeader->version < VERSION_9)
	{
		debug( LOG_ERROR, "UnitLoad; unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version == VERSION_11)
	{
		if (!loadSaveDroidV11(pFileData, filesize, psHeader->quantity, psHeader->version, ppsCurrentDroidLists))
		{
			return false;
		}
	}
	else if (psHeader->version <= VERSION_19)//old save name size
	{
		if (!loadSaveDroidV19(pFileData, filesize, psHeader->quantity, psHeader->version, ppsCurrentDroidLists))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveDroidV(pFileData, filesize, psHeader->quantity, psHeader->version, ppsCurrentDroidLists))
		{
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported droid save format version %u", psHeader->version);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
static DROID* buildDroidFromSaveDroidV11(SAVE_DROID_V11* psSaveDroid)
{
	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	BOOL					found;
	UDWORD					i;
	SDWORD					compInc;
	UDWORD					burnTime;

	psTemplate = &sTemplate;


	//set up the template
	//copy the values across

	sstrcpy(psTemplate->aName, psSaveDroid->name);
	//ignore the first comp - COMP_UNKNOWN
	found = true;
	for (i=1; i < DROID_MAXCOMP; i++)
	{

		compInc = getCompFromName(i, psSaveDroid->asBits[i].name);
		if (compInc < 0)
		{

			debug( LOG_ERROR, "This component no longer exists - %s, the droid will be deleted", psSaveDroid->asBits[i].name );

			found = false;
			break;//continue;
		}
		psTemplate->asParts[i] = (UDWORD)compInc;
	}
	if (!found)
	{
		//ignore this record
		return NULL;
	}
	psTemplate->numWeaps = psSaveDroid->numWeaps;
	for (i=0; i < psSaveDroid->numWeaps; i++)
	{
		int weapon = getCompFromName(COMP_WEAPON, psSaveDroid->asWeaps[i].name);
		ASSERT_OR_RETURN(false, weapon >= 0, "This component does not exist : %s", psSaveDroid->asWeaps[i].name);
		psTemplate->asWeaps[i] = weapon;
	}

	psTemplate->buildPoints = calcTemplateBuild(psTemplate);
	psTemplate->powerPoints = calcTemplatePower(psTemplate);
	psTemplate->droidType = psSaveDroid->droidType;

	/*create the Droid */

	// ignore brains for now
	psTemplate->asParts[COMP_BRAIN] = 0;

	psDroid = reallyBuildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y, psSaveDroid->player, false);

	//copy the droid's weapon stats
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			//only one weapon now
			psDroid->asWeaps[i].ammo = psSaveDroid->asWeaps[i].ammo;
			psDroid->asWeaps[i].lastFired = psSaveDroid->asWeaps[i].lastFired;
		}
	}
	//copy the values across
	psDroid->id = psSaveDroid->id;
	//are these going to ever change from the values set up with?
//			psDroid->pos.z = psSaveDroid->pos.z;		// use the correct map height value

	psDroid->rot.direction = DEG(psSaveDroid->direction);
	psDroid->body = psSaveDroid->body;
	if (psDroid->body > psDroid->originalBody)
	{
		psDroid->body = psDroid->originalBody;
	}

	psDroid->inFire = psSaveDroid->inFire;
	psDroid->burnDamage = psSaveDroid->burnDamage;
	burnTime = psSaveDroid->burnStart;
	psDroid->burnStart = burnTime;

	psDroid->experience = (float)psSaveDroid->numKills;
	//version 11
	for (i=0; i < psDroid->numWeaps; i++)
	{
		psDroid->asWeaps[i].rot.direction = DEG(psSaveDroid->turretRotation);
		psDroid->asWeaps[i].rot.pitch = DEG(psSaveDroid->turretPitch);
	}


	psDroid->psGroup = NULL;
	psDroid->psGrpNext = NULL;

	return psDroid;
}

// -----------------------------------------------------------------------------------------
static DROID* buildDroidFromSaveDroidV19(SAVE_DROID_V18* psSaveDroid, UDWORD version)
{
	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	SAVE_DROID_V14			*psSaveDroidV14;
	BOOL					found;
	UDWORD					i, id;
	SDWORD					compInc;
	UDWORD					burnTime;

	psTemplate = &sTemplate;

	psTemplate->pName = NULL;

	//set up the template
	//copy the values across

	sstrcpy(psTemplate->aName, psSaveDroid->name);

	//ignore the first comp - COMP_UNKNOWN
	found = true;
	for (i=1; i < DROID_MAXCOMP; i++)
	{

		compInc = getCompFromName(i, psSaveDroid->asBits[i].name);

		if (compInc < 0)
		{

			debug( LOG_ERROR, "This component no longer exists - %s, the droid will be deleted", psSaveDroid->asBits[i].name );

			found = false;
			break;//continue;
		}
		psTemplate->asParts[i] = (UDWORD)compInc;
	}
	ASSERT_OR_RETURN(NULL, found, "Failed to find weapon");
	psTemplate->numWeaps = psSaveDroid->numWeaps;

	if (psSaveDroid->numWeaps > 0)
	{
		for(i = 0;i < psTemplate->numWeaps;i++)
		{
			int weapon = getCompFromName(COMP_WEAPON, psSaveDroid->asWeaps[i].name);
			ASSERT_OR_RETURN(NULL, weapon >= 0, "This component does not exist : %s", psSaveDroid->asWeaps[i].name);
			psTemplate->asWeaps[i] = weapon;
		}
	}

	psTemplate->buildPoints = calcTemplateBuild(psTemplate);
	psTemplate->powerPoints = calcTemplatePower(psTemplate);
	psTemplate->droidType = psSaveDroid->droidType;

	/*create the Droid */


	// ignore brains for now
	// not any *$&!!! more - JOHN
//	psTemplate->asParts[COMP_BRAIN] = 0;

	if(psSaveDroid->x == INVALID_XY)
	{
		psDroid = reallyBuildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y, psSaveDroid->player, true);
	}
	else if(psSaveDroid->saveType == DROID_ON_TRANSPORT)
	{
		psDroid = reallyBuildDroid(psTemplate, 0, 0, psSaveDroid->player, true);
	}
	else
	{
		psDroid = reallyBuildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y, psSaveDroid->player, false);
	}

	ASSERT_OR_RETURN(NULL, psDroid != NULL, "Failed to build unit");

	//copy the droid's weapon stats
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			psDroid->asWeaps[i].ammo = psSaveDroid->asWeaps[i].ammo;
			psDroid->asWeaps[i].lastFired = psSaveDroid->asWeaps[i].lastFired;
		}
	}
	//copy the values across
	psDroid->id = psSaveDroid->id;
	//are these going to ever change from the values set up with?
//			psDroid->pos.z = psSaveDroid->pos.z;		// use the correct map height value

	psDroid->rot.direction = DEG(psSaveDroid->direction);
    psDroid->body = psSaveDroid->body;
	if (psDroid->body > psDroid->originalBody)
	{
		psDroid->body = psDroid->originalBody;
	}

	psDroid->inFire = psSaveDroid->inFire;
	psDroid->burnDamage = psSaveDroid->burnDamage;
	burnTime = psSaveDroid->burnStart;
	psDroid->burnStart = burnTime;

	psDroid->experience = (float)psSaveDroid->numKills;
	//version 14
	psDroid->resistance = droidResistance(psDroid);

	if (version >= VERSION_11)//version 11
	{
		//Watermelon:make it back-compatible with older versions of save
		for (i=0; i < psDroid->numWeaps; i++)
		{
			psDroid->asWeaps[i].rot.direction = DEG(psSaveDroid->turretRotation);
			psDroid->asWeaps[i].rot.pitch = DEG(psSaveDroid->turretPitch);
		}
	}
	if (version >= VERSION_12)//version 12
	{
		psDroid->order				= psSaveDroid->order;
		psDroid->orderX				= psSaveDroid->orderX;
		psDroid->orderY				= psSaveDroid->orderY;
		psDroid->orderX2				= psSaveDroid->orderX2;
		psDroid->orderY2				= psSaveDroid->orderY2;
		psDroid->timeLastHit			= psSaveDroid->timeLastHit;
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psTarget, psSaveDroid->targetID);
		psDroid->secondaryOrder		= psSaveDroid->secondaryOrder;
		psDroid->action				= psSaveDroid->action;
		psDroid->actionX				= psSaveDroid->actionX;
		psDroid->actionY				= psSaveDroid->actionY;
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psActionTarget[0], psSaveDroid->actionTargetID);
		psDroid->actionStarted		= psSaveDroid->actionStarted;
		psDroid->actionPoints		= psSaveDroid->actionPoints;
		//added for V14

		psDroid->psGroup = NULL;
		psDroid->psGrpNext = NULL;
	}
	if ((version >= VERSION_14) && (version < VERSION_18))//version 14
	{
		//warning V14 - v17 only
		//current Save Droid V18+ uses larger tarStatName
		//subsequent structure elements are not aligned between the two
		psSaveDroidV14 = (SAVE_DROID_V14*)psSaveDroid;
		if (psSaveDroidV14->tarStatName[0] == 0)
		{
			psDroid->psTarStats = NULL;
		}
		else
	 	{
			id = getStructStatFromName(psSaveDroidV14->tarStatName);
			if (id != (UDWORD)-1)
			{
				psDroid->psTarStats = (BASE_STATS*)&asStructureStats[id];
			}
			else
			{
				ASSERT( false,"loadUnit TargetStat not found" );
				psDroid->psTarStats = NULL;
                orderDroid(psDroid, DORDER_STOP);
			}
		}
		//warning V14 - v17 only
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psBaseStruct, psSaveDroidV14->baseStructID);
		psDroid->group = psSaveDroidV14->group;
		psDroid->selected = psSaveDroidV14->selected;
		psDroid->died = psSaveDroidV14->died;
		psDroid->lastEmission =	psSaveDroidV14->lastEmission;
		//warning V14 - v17 only
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psDroid->visible[i]	= psSaveDroidV14->visible[i];
		}
		//end warning V14 - v17 only
	}
	else if (version >= VERSION_18)//version 18
	{

		if (psSaveDroid->tarStatName[0] == 0)
		{
			psDroid->psTarStats = NULL;
		}
		else
		{
			id = getStructStatFromName(psSaveDroid->tarStatName);
			if (id != (UDWORD)-1)
			{
				psDroid->psTarStats = (BASE_STATS*)&asStructureStats[id];
			}
			else
			{
				ASSERT( false,"loadUnit TargetStat not found" );
				psDroid->psTarStats = NULL;
			}
		}
		//rebuild the object pointer from the ID
		FIXME_CAST_ASSIGN(UDWORD, psDroid->psBaseStruct, psSaveDroid->baseStructID);
		psDroid->group = psSaveDroid->group;
		psDroid->selected = psSaveDroid->selected;
		psDroid->died = psSaveDroid->died;
		psDroid->lastEmission =	psSaveDroid->lastEmission;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psDroid->visible[i]	= psSaveDroid->visible[i];
		}
	}

	return psDroid;
}

static void SaveDroidMoveControl(SAVE_DROID * const psSaveDroid, DROID const * const psDroid)
{
	unsigned int i;

	// Had to disable this one -- cannot figure out where it gets set wrong! - Per
	// ASSERT(droidOnMap(psDroid), "saved droid standing or moving off the map");

	// Copy over the endian neutral stuff (all UBYTE)
	psSaveDroid->sMove.Status    = psDroid->sMove.Status;
	psSaveDroid->sMove.Position  = psDroid->sMove.Position;
	psSaveDroid->sMove.numPoints = MIN(psDroid->sMove.numPoints, TRAVELSIZE);
	for (i = 0; i < MIN(psDroid->sMove.numPoints, TRAVELSIZE); i++)
	{
		psSaveDroid->sMove.asPath[i].x = map_coord(psDroid->sMove.asPath[i].x);
		psSaveDroid->sMove.asPath[i].y = map_coord(psDroid->sMove.asPath[i].y);
	}

	// Little endian SDWORDs
	psSaveDroid->sMove.DestinationX = PHYSFS_swapSLE32(psDroid->sMove.DestinationX);
	psSaveDroid->sMove.DestinationY = PHYSFS_swapSLE32(psDroid->sMove.DestinationY);
	psSaveDroid->sMove.srcX         = PHYSFS_swapSLE32(psDroid->sMove.srcX);
	psSaveDroid->sMove.srcY         = PHYSFS_swapSLE32(psDroid->sMove.srcY);
	psSaveDroid->sMove.targetX      = PHYSFS_swapSLE32(psDroid->sMove.targetX);
	psSaveDroid->sMove.targetY      = PHYSFS_swapSLE32(psDroid->sMove.targetY);

	// Little endian floats
	psSaveDroid->sMove.fx           = 0;
	psSaveDroid->sMove.fy           = 0;
	psSaveDroid->sMove.speed        = PHYSFS_swapSLE32(psDroid->sMove.speed);
	psSaveDroid->sMove.moveDir      = PHYSFS_swapSLE32(UNDEG(psDroid->sMove.moveDir));
	psSaveDroid->sMove.fz           = 0;

	// Little endian SWORDs
	psSaveDroid->sMove.boundX       = 0;
	psSaveDroid->sMove.boundY       = 0;
	psSaveDroid->sMove.bumpDir      = PHYSFS_swapSLE16(UNDEG(psDroid->sMove.bumpDir));
	psSaveDroid->sMove.iVertSpeed   = PHYSFS_swapSLE16(psDroid->sMove.iVertSpeed);

	// Little endian UDWORDs
	psSaveDroid->sMove.bumpTime     = PHYSFS_swapULE32(psDroid->sMove.bumpTime);
	psSaveDroid->sMove.shuffleStart = PHYSFS_swapULE32(psDroid->sMove.shuffleStart);

	// Array of little endian UDWORDS
	for (i = 0; i < sizeof(psDroid->sMove.iAttackRuns) / sizeof(psDroid->sMove.iAttackRuns[0]); ++i)
	{
		psSaveDroid->sMove.iAttackRuns[i] = PHYSFS_swapULE32(psDroid->sMove.iAttackRuns[i]);
	}

	// Little endian UWORDs
	psSaveDroid->sMove.lastBump     = PHYSFS_swapULE16(psDroid->sMove.lastBump);
	psSaveDroid->sMove.pauseTime    = PHYSFS_swapULE16(psDroid->sMove.pauseTime);
	psSaveDroid->sMove.bumpX        = PHYSFS_swapULE16(psDroid->sMove.bumpX);
	psSaveDroid->sMove.bumpY        = PHYSFS_swapULE16(psDroid->sMove.bumpY);

	psSaveDroid->sMove.isInFormation = false;
	psSaveDroid->formationDir = 0;
	psSaveDroid->formationX   = 0;
	psSaveDroid->formationY   = 0;
}

static void LoadDroidMoveControl(DROID * const psDroid, SAVE_DROID const * const psSaveDroid)
{
	unsigned int i;

	// Copy over the endian neutral stuff (all UBYTE)
	psDroid->sMove.Status      = psSaveDroid->sMove.Status;
	psDroid->sMove.Position    = psSaveDroid->sMove.Position;
	psDroid->sMove.numPoints   = psSaveDroid->sMove.numPoints;
	psDroid->sMove.asPath      = malloc(sizeof(*psDroid->sMove.asPath) * psDroid->sMove.numPoints);
	for (i = 0; i < psDroid->sMove.numPoints; i++)
	{
		psDroid->sMove.asPath[i].x = world_coord(psSaveDroid->sMove.asPath[i].x) + TILE_UNITS / 2;
		psDroid->sMove.asPath[i].y = world_coord(psSaveDroid->sMove.asPath[i].y) + TILE_UNITS / 2;
	}

	// Little endian SDWORDs
	psDroid->sMove.DestinationX = PHYSFS_swapSLE32(psSaveDroid->sMove.DestinationX);
	psDroid->sMove.DestinationY = PHYSFS_swapSLE32(psSaveDroid->sMove.DestinationY);
	psDroid->sMove.srcX         = PHYSFS_swapSLE32(psSaveDroid->sMove.srcX);
	psDroid->sMove.srcY         = PHYSFS_swapSLE32(psSaveDroid->sMove.srcY);
	psDroid->sMove.targetX      = PHYSFS_swapSLE32(psSaveDroid->sMove.targetX);
	psDroid->sMove.targetY      = PHYSFS_swapSLE32(psSaveDroid->sMove.targetY);

	// Little endian floats
	psDroid->sMove.speed        = PHYSFS_swapSLE32(psSaveDroid->sMove.speed);
	psDroid->sMove.moveDir      = DEG(PHYSFS_swapSLE32(psSaveDroid->sMove.moveDir));

	// Little endian SWORDs
	psDroid->sMove.bumpDir      = DEG(PHYSFS_swapSLE16(psSaveDroid->sMove.bumpDir));
	psDroid->sMove.iVertSpeed   = PHYSFS_swapSLE16(psSaveDroid->sMove.iVertSpeed);

	// Little endian UDWORDs
	psDroid->sMove.bumpTime     = PHYSFS_swapULE32(psSaveDroid->sMove.bumpTime);
	psDroid->sMove.shuffleStart = PHYSFS_swapULE32(psSaveDroid->sMove.shuffleStart);

	// Array of little endian UDWORDS
	for (i = 0; i < sizeof(psSaveDroid->sMove.iAttackRuns) / sizeof(psSaveDroid->sMove.iAttackRuns[0]); ++i)
	{
		psDroid->sMove.iAttackRuns[i] = PHYSFS_swapULE32(psSaveDroid->sMove.iAttackRuns[i]);
	}

	// Little endian UWORDs
	psDroid->sMove.lastBump     = PHYSFS_swapULE16(psSaveDroid->sMove.lastBump);
	psDroid->sMove.pauseTime    = PHYSFS_swapULE16(psSaveDroid->sMove.pauseTime);
	psDroid->sMove.bumpX        = PHYSFS_swapULE16(psSaveDroid->sMove.bumpX);
	psDroid->sMove.bumpY        = PHYSFS_swapULE16(psSaveDroid->sMove.bumpY);

	// Recreate path-finding jobs
	if (psDroid->sMove.Status == MOVEWAITROUTE)
	{
		psDroid->sMove.Status = MOVEINACTIVE;
		fpathDroidRoute(psDroid, psDroid->sMove.DestinationX, psDroid->sMove.DestinationY, FMT_MOVE);
		psDroid->sMove.Status = MOVEWAITROUTE;
	}
}

// -----------------------------------------------------------------------------------------
//version 20 + after names change
static DROID* buildDroidFromSaveDroid(SAVE_DROID* psSaveDroid, UDWORD version)
{
	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	BOOL					found;
	UDWORD					i, id;
	SDWORD					compInc;
	UDWORD					burnTime;

	psTemplate = &sTemplate;

	psTemplate->pName = NULL;

	//set up the template
	//copy the values across

	sstrcpy(psTemplate->aName, psSaveDroid->name);
	//ignore the first comp - COMP_UNKNOWN
	found = true;
	for (i=1; i < DROID_MAXCOMP; i++)
	{
		compInc = getCompFromName(i, psSaveDroid->asBits[i].name);

        //HACK to get the game to load when ECMs, Sensors or RepairUnits have been deleted
        if (compInc < 0 && (i == COMP_ECM || i == COMP_SENSOR || i == COMP_REPAIRUNIT))
        {
            //set the ECM to be the defaultECM ...
            if (i == COMP_ECM)
            {
                compInc = aDefaultECM[psSaveDroid->player];
            }
            else if (i == COMP_SENSOR)
            {
                compInc = aDefaultSensor[psSaveDroid->player];
            }
            else if (i == COMP_REPAIRUNIT)
            {
                compInc = aDefaultRepair[psSaveDroid->player];
            }
        }
		else if (compInc < 0)
		{
			ASSERT(compInc >= 0, "This component no longer exists - %s, the droid will be deleted", psSaveDroid->asBits[i].name);
			found = false;
			break;//continue;
		}
		psTemplate->asParts[i] = (UDWORD)compInc;
	}
	ASSERT_OR_RETURN(NULL, found, "Failed to find weapon");
	psTemplate->numWeaps = psSaveDroid->numWeaps;
	if (psSaveDroid->numWeaps > 0)
	{
		for(i = 0;i < psTemplate->numWeaps;i++)
		{
			int weapon = getCompFromName(COMP_WEAPON, psSaveDroid->asWeaps[i].name);
			ASSERT_OR_RETURN(NULL, weapon >= 0, "This component does not exist : %s", psSaveDroid->asWeaps[i].name);
			psTemplate->asWeaps[i] = weapon;
		}
	}

	psTemplate->buildPoints = calcTemplateBuild(psTemplate);
	psTemplate->powerPoints = calcTemplatePower(psTemplate);
	psTemplate->droidType = psSaveDroid->droidType;

	/*create the Droid */

	turnOffMultiMsg(true);

	if(psSaveDroid->x == INVALID_XY)
	{
		psDroid = reallyBuildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y, psSaveDroid->player, true);
	}
	else if(psSaveDroid->saveType == DROID_ON_TRANSPORT)
	{
		psDroid = reallyBuildDroid(psTemplate, 0, 0, psSaveDroid->player, true);
	}
	else
	{
		psDroid = reallyBuildDroid(psTemplate, psSaveDroid->x, psSaveDroid->y, psSaveDroid->player, false);
	}

	ASSERT_OR_RETURN(NULL, psDroid != NULL, "Failed to build unit");

	turnOffMultiMsg(false);


	//copy the droid's weapon stats
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			psDroid->asWeaps[i].ammo = psSaveDroid->asWeaps[i].ammo;
			psDroid->asWeaps[i].lastFired = psSaveDroid->asWeaps[i].lastFired;
		}
	}
	//copy the values across
	psDroid->id = psSaveDroid->id;
	//are these going to ever change from the values set up with?
//			psDroid->pos.z = psSaveDroid->pos.z;		// use the correct map height value

	psDroid->rot.direction = DEG(psSaveDroid->direction);
	psDroid->body = psSaveDroid->body;
	if (psDroid->body > psDroid->originalBody)
	{
		psDroid->body = psDroid->originalBody;
	}

	psDroid->inFire = psSaveDroid->inFire;
	psDroid->burnDamage = psSaveDroid->burnDamage;
	burnTime = psSaveDroid->burnStart;
	psDroid->burnStart = burnTime;

	psDroid->experience = (float)psSaveDroid->numKills;
	//version 14
	psDroid->resistance = droidResistance(psDroid);

	//version 11
	//Watermelon:make it back-compatible with older versions of save
	for (i=0; i < psDroid->numWeaps; i++)
	{
		if (version >= VERSION_24)
		{
			psDroid->asWeaps[i].rot.direction = DEG(psSaveDroid->turretRotation[i]);
			psDroid->asWeaps[i].rot.pitch = DEG(psSaveDroid->turretPitch[i]);
		}
		else
		{
			psDroid->asWeaps[i].rot.direction = DEG(psSaveDroid->turretRotation[0]);
			psDroid->asWeaps[i].rot.pitch = DEG(psSaveDroid->turretPitch[0]);
		}
	}
	//version 12
	psDroid->order				= psSaveDroid->order;
	psDroid->orderX				= psSaveDroid->orderX;
	psDroid->orderY				= psSaveDroid->orderY;
	psDroid->orderX2				= psSaveDroid->orderX2;
	psDroid->orderY2				= psSaveDroid->orderY2;
	psDroid->timeLastHit			= psSaveDroid->timeLastHit;
	//rebuild the object pointer from the ID
	FIXME_CAST_ASSIGN(UDWORD, psDroid->psTarget, psSaveDroid->targetID);
	psDroid->secondaryOrder		= psSaveDroid->secondaryOrder;
	psDroid->action				= psSaveDroid->action;
	psDroid->actionX				= psSaveDroid->actionX;
	psDroid->actionY				= psSaveDroid->actionY;
	//rebuild the object pointer from the ID
	FIXME_CAST_ASSIGN(UDWORD, psDroid->psActionTarget[0], psSaveDroid->actionTargetID);
	psDroid->actionStarted		= psSaveDroid->actionStarted;
	psDroid->actionPoints		= psSaveDroid->actionPoints;
	//added for V14


	//version 18
	if (psSaveDroid->tarStatName[0] == 0)
	{
		psDroid->psTarStats = NULL;
	}
	else
	{
		id = getStructStatFromName(psSaveDroid->tarStatName);
		if (id != (UDWORD)-1)
		{
			psDroid->psTarStats = (BASE_STATS*)&asStructureStats[id];
		}
		else
		{
			ASSERT( false,"loadUnit TargetStat not found" );
			psDroid->psTarStats = NULL;
		}
	}
	//rebuild the object pointer from the ID
	FIXME_CAST_ASSIGN(UDWORD, psDroid->psBaseStruct, psSaveDroid->baseStructID);
	psDroid->group = psSaveDroid->group;
	psDroid->selected = psSaveDroid->selected;
	psDroid->died = psSaveDroid->died;
	psDroid->lastEmission =	psSaveDroid->lastEmission;
	for (i=0; i < MAX_PLAYERS; i++)
	{
		psDroid->visible[i]	= psSaveDroid->visible[i];
	}

	if (version >= VERSION_21)//version 21
	{
		if ( (psDroid->droidType != DROID_TRANSPORTER) &&
						 (psDroid->droidType != DROID_COMMAND) )
		{
			//rebuild group from command id in loadDroidSetPointers
			FIXME_CAST_ASSIGN(UDWORD, psDroid->psGroup, psSaveDroid->commandId);
			FIXME_CAST_ASSIGN(UDWORD, psDroid->psGrpNext, NULL_ID);
		}
	}
	else
	{
		if ( (psDroid->droidType != DROID_TRANSPORTER) &&
						 (psDroid->droidType != DROID_COMMAND) )
		{
			//dont rebuild group from command id in loadDroidSetPointers
			psDroid->psGroup = NULL;
			psDroid->psGrpNext = NULL;
		}
	}

	if (version >= VERSION_24)//version 24
	{
		psDroid->resistance = (SWORD)psSaveDroid->resistance;
		LoadDroidMoveControl(psDroid, psSaveDroid);
	}
	return psDroid;
}


// -----------------------------------------------------------------------------------------
static BOOL loadDroidSetPointers(void)
{
	UDWORD		player,list;
	DROID		*psDroid, *psCommander;
	DROID		**ppsDroidLists[3], *psNext;

	ppsDroidLists[0] = apsDroidLists;
	ppsDroidLists[1] = mission.apsDroidLists;
	ppsDroidLists[2] = apsLimboDroids;

	for(list = 0; list<3; list++)
	{
		debug( LOG_NEVER, "List %d\n", list );
		for(player = 0; player<MAX_PLAYERS; player++)
		{
			psDroid=(DROID *)ppsDroidLists[list][player];
			while (psDroid)
			{
				UDWORD id;
				//Target rebuild the object pointer from the ID
				FIXME_CAST_ASSIGN(UDWORD, id, psDroid->psTarget);
				if (id != NULL_ID)
				{
					setSaveDroidTarget(psDroid, getBaseObjFromId(id));
					ASSERT(psDroid->psTarget != NULL, "Saved Droid psTarget getBaseObjFromId() failed");
					if (psDroid->psTarget == NULL)
					{
						psDroid->order = DORDER_NONE;
					}
				}
				else
				{
					setSaveDroidTarget(psDroid, NULL);
				}
				//ActionTarget rebuild the object pointer from the ID
				FIXME_CAST_ASSIGN(UDWORD, id, psDroid->psActionTarget[0]);
				if (id != NULL_ID)
				{
					setSaveDroidActionTarget(psDroid, getBaseObjFromId(id), 0);
					ASSERT( psDroid->psActionTarget[0] != NULL,"Saved Droid psActionTarget getBaseObjFromId() failed" );
					if (psDroid->psActionTarget[0] == NULL)
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else
				{
					setSaveDroidActionTarget(psDroid, NULL, 0);
				}
				//BaseStruct rebuild the object pointer from the ID
				FIXME_CAST_ASSIGN(UDWORD, id, psDroid->psBaseStruct);
				if (id != NULL_ID)
				{
					setSaveDroidBase(psDroid, (STRUCTURE*)getBaseObjFromId(id));
					ASSERT( psDroid->psBaseStruct != NULL,"Saved Droid psBaseStruct getBaseObjFromId() failed" );
					if (psDroid->psBaseStruct == NULL)
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else
				{
					setSaveDroidBase(psDroid, NULL);//psSaveDroid->targetID
				}
				if (saveGameVersion > VERSION_20)
				{
					UDWORD _tmpid;
					//rebuild group for droids in command group from the commander ID
					FIXME_CAST_ASSIGN(UDWORD, id, psDroid->psGrpNext);
					if (id == NULL_ID)
					{
						FIXME_CAST_ASSIGN(UDWORD, _tmpid, psDroid->psGroup);
						psDroid->psGroup = NULL;
						psDroid->psGrpNext = NULL;
						if (_tmpid != NULL_ID)
						{
							psCommander = (DROID*)getBaseObjFromId(_tmpid);
							ASSERT( psCommander != NULL,"Saved Droid psCommander getBaseObjFromId() failed" );
							if (psCommander != NULL)
							{
								cmdDroidAddDroid(psCommander,psDroid);
							}
						}
					}
				}
				psDroid = psDroid->psNext;
			}
		}
	}

	/* HACK: Make sure all cargo units are properly initialized! I am not sure why we need
	 * to do this, but the code in this file is too horrible to debug. - Per */
	for(list = 0; list<3; list++)
	{
		for(player = 0; player<MAX_PLAYERS; player++)
		{
			for (psNext = (DROID *)ppsDroidLists[list][player]; psNext; psNext = psNext->psNext)
			{
				if (psNext->droidType == DROID_TRANSPORTER)
				{
					DROID *psCargo, *psTemp;

					for (psCargo = psNext->psGroup->psList; psCargo; psCargo = psTemp)
					{
						UDWORD i;

						psTemp = psCargo->psGrpNext;
						setSaveDroidTarget(psCargo, NULL);
						for (i = 0; i < DROID_MAXWEAPS; i++)
						{
							setSaveDroidActionTarget(psCargo, NULL, i);
						}
						setSaveDroidBase(psCargo, NULL);
					}
				}
			}
		}
	}


	return true;
}

// -----------------------------------------------------------------------------------------
/* code specific to version 11 of a save droid */
BOOL loadSaveDroidV11(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists)
{
	SAVE_DROID_V11				*psSaveDroid, sSaveDroid;
//	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	DROID_GROUP				*psCurrentTransGroup;
	UDWORD					count;
	UDWORD					NumberOfSkippedDroids=0;
	UDWORD					sizeOfSaveDroid = 0;
	DROID_GROUP				*psGrp;
	UBYTE	i;

	psCurrentTransGroup = NULL;

	debug(LOG_SAVE, "fileversion is %u ", version);

	psSaveDroid = &sSaveDroid;
	if (version <= VERSION_10)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V9);
	}
	else if (version == VERSION_11)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V11);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V12);
	}

	if ((sizeOfSaveDroid * numDroids + DROID_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "unitLoad: unexpected end of file" );

		return false;
	}

	/* Load in the droid data */
	for (count = 0; count < numDroids; count ++, pFileData += sizeOfSaveDroid)
	{
		memcpy(psSaveDroid, pFileData, sizeOfSaveDroid);

		/* DROID_SAVE_V11 includes OBJECT_SAVE_V19, SAVE_WEAPON_V19 */
		endian_udword(&psSaveDroid->body);
		endian_udword(&psSaveDroid->numWeaps);
		endian_udword(&psSaveDroid->numKills);
		endian_uword(&psSaveDroid->turretRotation);
		endian_uword(&psSaveDroid->turretPitch);
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveDroid->id);
		endian_udword(&psSaveDroid->x);
		endian_udword(&psSaveDroid->y);
		endian_udword(&psSaveDroid->z);
		endian_udword(&psSaveDroid->direction);
		endian_udword(&psSaveDroid->player);
		endian_udword(&psSaveDroid->burnStart);
		endian_udword(&psSaveDroid->burnDamage);
		for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
			/* SAVE_WEAPON_V19 */
			endian_udword(&psSaveDroid->asWeaps[i].ammo);
			endian_udword(&psSaveDroid->asWeaps[i].lastFired);
		}

		// Here's a check that will allow us to load up save games on the playstation from the PC
		//  - It will skip data from any players after MAX_PLAYERS
		if (psSaveDroid->player >= MAX_PLAYERS)
		{
			NumberOfSkippedDroids++;
			psSaveDroid->player=MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
		}

		psDroid = buildDroidFromSaveDroidV11(psSaveDroid);

		if (psDroid == NULL)
		{
			debug( LOG_ERROR, "unitLoad: Template not found for unit\n" );
			return false;
		}
		else if (psSaveDroid->saveType == DROID_ON_TRANSPORT)
		{
   			//add the droid to the list
			setSaveDroidTarget(psDroid, NULL);
			for (i = 0; i < DROID_MAXWEAPS; i++)
			{
				setSaveDroidActionTarget(psDroid, NULL, i);
			}
			setSaveDroidBase(psDroid, NULL);
			ASSERT( psCurrentTransGroup != NULL,"loadSaveUnitV9; Transporter unit without group " );
			grpJoin(psCurrentTransGroup, psDroid);
		}
		else
		{
			//add the droid to the list
			addDroid(psDroid, ppsCurrentDroidLists);
		}

		if (psDroid != NULL)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				//set current TransPorter group
				if (!grpCreate(&psGrp))
				{
					debug( LOG_NEVER, "unit build: unable to create group\n" );
					return false;
				}
				grpJoin(psGrp, psDroid);
				psCurrentTransGroup = psDroid->psGroup;
			}
		}
	}
	if (NumberOfSkippedDroids>0)
	{
		debug( LOG_ERROR, "unitLoad: Bad Player number in %d unit(s)... assigned to the last player!\n", NumberOfSkippedDroids );
		return false;
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	return true;
}

// -----------------------------------------------------------------------------------------
/* code specific all versions upto from 12 to 19*/
BOOL loadSaveDroidV19(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists)
{
	SAVE_DROID_V18				*psSaveDroid, sSaveDroid;
//	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	DROID_GROUP				*psCurrentTransGroup;
	UDWORD					count;
	UDWORD					NumberOfSkippedDroids=0;
	UDWORD					sizeOfSaveDroid = 0;
	DROID_GROUP				*psGrp;
	UBYTE	i;

	psCurrentTransGroup = NULL;

	debug(LOG_SAVE, "fileversion is %u ", version);

	psSaveDroid = &sSaveDroid;
	if (version <= VERSION_10)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V9);
	}
	else if (version == VERSION_11)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V11);
	}
	else if (version == VERSION_12)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V12);
	}
	else if (version < VERSION_18)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V14);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V18);
	}

	if ((sizeOfSaveDroid * numDroids + DROID_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "unitLoad: unexpected end of file" );

		return false;
	}

	/* Load in the droid data */
	for (count = 0; count < numDroids; count ++, pFileData += sizeOfSaveDroid)
	{
		memcpy(psSaveDroid, pFileData, sizeOfSaveDroid);

		/* DROID_SAVE_V18 includes DROID_SAVE_V12*/
		endian_udword(&psSaveDroid->baseStructID);
		endian_udword(&psSaveDroid->died);
		endian_udword(&psSaveDroid->lastEmission);
		/* DROID_SAVE_V12 includes DROID_SAVE_V9 */
		endian_uword(&psSaveDroid->turretRotation);
		endian_uword(&psSaveDroid->turretPitch);
		endian_sdword(&psSaveDroid->order);
		endian_uword(&psSaveDroid->orderX);
		endian_uword(&psSaveDroid->orderY);
		endian_uword(&psSaveDroid->orderX2);
		endian_uword(&psSaveDroid->orderY2);
		endian_udword(&psSaveDroid->timeLastHit);
		endian_udword(&psSaveDroid->targetID);
		endian_udword(&psSaveDroid->secondaryOrder);
		endian_sdword(&psSaveDroid->action);
		endian_udword(&psSaveDroid->actionX);
		endian_udword(&psSaveDroid->actionY);
		endian_udword(&psSaveDroid->actionTargetID);
		endian_udword(&psSaveDroid->actionStarted);
		endian_udword(&psSaveDroid->actionPoints);
		endian_uword(&psSaveDroid->actionHeight);
		/* DROID_SAVE_V9 includes OBJECT_SAVE_V19, SAVE_WEAPON_V19 */
		endian_udword(&psSaveDroid->body);
		endian_udword(&psSaveDroid->saveType);
		endian_udword(&psSaveDroid->numWeaps);
		endian_udword(&psSaveDroid->numKills);
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveDroid->id);
		endian_udword(&psSaveDroid->x);
		endian_udword(&psSaveDroid->y);
		endian_udword(&psSaveDroid->z);
		endian_udword(&psSaveDroid->direction);
		endian_udword(&psSaveDroid->player);
		endian_udword(&psSaveDroid->burnStart);
		endian_udword(&psSaveDroid->burnDamage);
		for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
			/* SAVE_WEAPON_V19 */
			endian_udword(&psSaveDroid->asWeaps[i].ammo);
			endian_udword(&psSaveDroid->asWeaps[i].lastFired);
		}

		// Here's a check that will allow us to load up save games on the playstation from the PC
		//  - It will skip data from any players after MAX_PLAYERS
		if (psSaveDroid->player >= MAX_PLAYERS)
		{
			NumberOfSkippedDroids++;
			psSaveDroid->player=MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
		}

		psDroid = buildDroidFromSaveDroidV19((SAVE_DROID_V18*)psSaveDroid, version);

		if (psDroid == NULL)
		{
			ASSERT( psDroid != NULL,"unitLoad: Failed to build new unit\n" );
		}
		else if (psSaveDroid->saveType == DROID_ON_TRANSPORT)
		{
  			//add the droid to the list
			psDroid->order = DORDER_NONE;
			psDroid->action = DACTION_NONE;
			setSaveDroidTarget(psDroid, NULL);
			for (i = 0; i < DROID_MAXWEAPS; i++)
			{
				setSaveDroidActionTarget(psDroid, NULL, i);
			}
			setSaveDroidBase(psDroid, NULL);
			//add the droid to the list
			ASSERT( psCurrentTransGroup != NULL,"loadSaveUnitV9; Transporter unit without group " );
			grpJoin(psCurrentTransGroup, psDroid);
		}
		else
		{
			//add the droid to the list
			addDroid(psDroid, ppsCurrentDroidLists);
		}

		if (psDroid != NULL)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				//set current TransPorter group
				if (!grpCreate(&psGrp))
				{
					debug( LOG_NEVER, "unit build: unable to create group\n" );
					return false;
				}
				grpJoin(psGrp, psDroid);
				psCurrentTransGroup = psDroid->psGroup;
			}
		}
	}
	if (NumberOfSkippedDroids>0)
	{
		debug( LOG_ERROR, "unitLoad: Bad Player number in %d unit(s)... assigned to the last player!\n", NumberOfSkippedDroids );
		return false;
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	return true;
}

// -----------------------------------------------------------------------------------------
/* code for all versions after save name change v19*/
BOOL loadSaveDroidV(char *pFileData, UDWORD filesize, UDWORD numDroids, UDWORD version, DROID **ppsCurrentDroidLists)
{
	SAVE_DROID				sSaveDroid, *psSaveDroid = &sSaveDroid;
//	DROID_TEMPLATE			*psTemplate, sTemplate;
	DROID					*psDroid;
	DROID_GROUP				*psCurrentTransGroup = NULL;
	UDWORD					count;
	UDWORD					NumberOfSkippedDroids=0;
	UDWORD					sizeOfSaveDroid = 0;
//	DROID_GROUP				*psGrp;
	UBYTE	i;

	debug(LOG_SAVE, "fileversion is %u ", version);

	if (version <= VERSION_20)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V20);
	}
	else if (version <= VERSION_23)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID_V21);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveDroid = sizeof(SAVE_DROID);
	}

	if ((sizeOfSaveDroid * numDroids + DROID_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "unitLoad: unexpected end of file" );

		return false;
	}

	/* Load in the droid data */
	for (count = 0; count < numDroids; count ++, pFileData += sizeOfSaveDroid)
	{
		memcpy(psSaveDroid, pFileData, sizeOfSaveDroid);

		/* DROID_SAVE_V24 includes DROID_SAVE_V21,
		 * SAVE_MOVE_CONTROL */
		endian_sdword(&psSaveDroid->resistance);
		endian_sword(&psSaveDroid->formationDir);
		endian_sdword(&psSaveDroid->formationX);
		endian_sdword(&psSaveDroid->formationY);
		/* DROID_SAVE_V21 includes DROID_SAVE_V20 */
		endian_udword(&psSaveDroid->commandId);
		/* DROID_SAVE_V20 includes OBJECT_SAVE_V20, SAVE_WEAPON */
		endian_udword(&psSaveDroid->body);
		endian_udword(&psSaveDroid->saveType);
		endian_udword(&psSaveDroid->numWeaps);
		endian_udword(&psSaveDroid->numKills);
		//Watermelon:'hack' to make it read turretRotation,Pitch table properly
		if( version == CURRENT_VERSION_NUM )
		{
			for(i = 0;i < psSaveDroid->numWeaps;i++)
			{
				endian_uword(&psSaveDroid->turretRotation[i]);
				endian_uword(&psSaveDroid->turretPitch[i]);
			}
		}
		else
		{
			endian_uword(&psSaveDroid->turretRotation[0]);
			endian_uword(&psSaveDroid->turretPitch[0]);
		}
		endian_sdword(&psSaveDroid->order);
		endian_uword(&psSaveDroid->orderX);
		endian_uword(&psSaveDroid->orderY);
		endian_uword(&psSaveDroid->orderX2);
		endian_uword(&psSaveDroid->orderY2);
		endian_udword(&psSaveDroid->timeLastHit);
		endian_udword(&psSaveDroid->targetID);
		endian_udword(&psSaveDroid->secondaryOrder);
		endian_sdword(&psSaveDroid->action);
		endian_udword(&psSaveDroid->actionX);
		endian_udword(&psSaveDroid->actionY);
		endian_udword(&psSaveDroid->actionTargetID);
		endian_udword(&psSaveDroid->actionStarted);
		endian_udword(&psSaveDroid->actionPoints);
		endian_uword(&psSaveDroid->actionHeight);
		endian_udword(&psSaveDroid->baseStructID);
		endian_udword(&psSaveDroid->died);
		endian_udword(&psSaveDroid->lastEmission);
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveDroid->id);
		endian_udword(&psSaveDroid->x);
		endian_udword(&psSaveDroid->y);
		endian_udword(&psSaveDroid->z);
		endian_udword(&psSaveDroid->direction);
		endian_udword(&psSaveDroid->player);
		endian_udword(&psSaveDroid->burnStart);
		endian_udword(&psSaveDroid->burnDamage);
		for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
			/* SAVE_WEAPON */
			endian_udword(&psSaveDroid->asWeaps[i].ammo);
			endian_udword(&psSaveDroid->asWeaps[i].lastFired);
		}

		// Here's a check that will allow us to load up save games on the playstation from the PC
		//  - It will skip data from any players after MAX_PLAYERS
		if (psSaveDroid->player >= MAX_PLAYERS)
		{
			NumberOfSkippedDroids++;
			psSaveDroid->player=MAX_PLAYERS-1;	// now don't lose any droids ... force them to be the last player
		}

		psDroid = buildDroidFromSaveDroid(psSaveDroid, version);

		if (psDroid == NULL)
		{
			ASSERT( psDroid != NULL,"unitLoad: Failed to build new unit\n" );
		}
		else if (psSaveDroid->saveType == DROID_ON_TRANSPORT)
		{
  			//add the droid to the list
			psDroid->order = DORDER_NONE;
			psDroid->action = DACTION_NONE;
			setSaveDroidTarget(psDroid, NULL);
			for (i = 0; i < DROID_MAXWEAPS; i++)
			{
				setSaveDroidActionTarget(psDroid, NULL, i);
			}
			setSaveDroidBase(psDroid, NULL);
			//add the droid to the list
			psDroid->psGroup = NULL;
			psDroid->psGrpNext = NULL;
			ASSERT( psCurrentTransGroup != NULL,"loadSaveUnitV9; Transporter unit without group " );
			grpJoin(psCurrentTransGroup, psDroid);
		}
		else if (psDroid->droidType == DROID_TRANSPORTER)
		{
				//set current TransPorter group
			psCurrentTransGroup = psDroid->psGroup;
			addDroid(psDroid, ppsCurrentDroidLists);
		}
		else
		{
			//add the droid to the list
			addDroid(psDroid, ppsCurrentDroidLists);
		}
	}
	if (NumberOfSkippedDroids>0)
	{
		debug( LOG_ERROR, "unitLoad: Bad Player number in %d unit(s)... assigned to the last player!\n", NumberOfSkippedDroids );
		return false;
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	return true;
}

// -----------------------------------------------------------------------------------------
static BOOL buildSaveDroidFromDroid(SAVE_DROID* psSaveDroid, DROID* psCurr, DROID_SAVE_TYPE saveType)
{
	UDWORD				i;

			/*want to store the resource ID string for compatibilty with
			different versions of save game - NOT HAPPENING - the name saved is
			the translated name - old versions of save games should load because
			templates are loaded from Access AND the save game so they should all
			still exist*/

			sstrcpy(psSaveDroid->name, psCurr->aName);

			// not interested in first comp - COMP_UNKNOWN
			for (i=1; i < DROID_MAXCOMP; i++)
			{

				if (!getNameFromComp(i, psSaveDroid->asBits[i].name, psCurr->asBits[i].nStat))

				{
					//ignore this record
					break;
				}
			}
			psSaveDroid->body = psCurr->body;
			//Watermelon:loop thru all weapons
			psSaveDroid->numWeaps = psCurr->numWeaps;
			for(i = 0;i < psCurr->numWeaps;i++)
			{
				if (psCurr->asWeaps[i].nStat > 0)
				{
					//there is only one weapon now

					if (getNameFromComp(COMP_WEAPON, psSaveDroid->asWeaps[i].name, psCurr->asWeaps[i].nStat))

					{
	    				psSaveDroid->asWeaps[i].ammo = psCurr->asWeaps[i].ammo;
		    			psSaveDroid->asWeaps[i].lastFired = psCurr->asWeaps[i].lastFired;
					}
				}
				else
				{

					psSaveDroid->asWeaps[i].name[i] = '\0';
				}
			}

			//save out experience level
			psSaveDroid->numKills	= (UDWORD) psCurr->experience;
			//version 11
			//Watermelon:endian_udword for new save format
			for(i = 0;i < psCurr->numWeaps;i++)
			{
				psSaveDroid->turretRotation[i] = UNDEG(psCurr->asWeaps[i].rot.direction);
				psSaveDroid->turretPitch[i]	= UNDEG(psCurr->asWeaps[i].rot.pitch);
			}
			//version 12
			psSaveDroid->order			= psCurr->order;
			psSaveDroid->orderX			= psCurr->orderX;
			psSaveDroid->orderY			= psCurr->orderY;
			psSaveDroid->orderX2		= psCurr->orderX2;
			psSaveDroid->orderY2		= psCurr->orderY2;
			psSaveDroid->timeLastHit	= psCurr->timeLastHit;

			psSaveDroid->targetID = NULL_ID;
			if (psCurr->psTarget != NULL && psCurr->psTarget->died <= 1 && checkValidId(psCurr->psTarget->id))
			{
				psSaveDroid->targetID = psCurr->psTarget->id;
			}

			psSaveDroid->secondaryOrder = psCurr->secondaryOrder;
			psSaveDroid->action			= psCurr->action;
			psSaveDroid->actionX		= psCurr->actionX;
			psSaveDroid->actionY		= psCurr->actionY;

			psSaveDroid->actionTargetID = NULL_ID;
			if (psCurr->psActionTarget[0] != NULL && psCurr->psActionTarget[0]->died <= 1 && checkValidId( psCurr->psActionTarget[0]->id))
			{
				psSaveDroid->actionTargetID = psCurr->psActionTarget[0]->id;
			}

			psSaveDroid->actionStarted	= psCurr->actionStarted;
			psSaveDroid->actionPoints	= psCurr->actionPoints;
			psSaveDroid->actionHeight	= 0; // UNUSED

			//version 14
			if (psCurr->psTarStats != NULL)
			{
				ASSERT(strlen(psCurr->psTarStats->pName) < sizeof(psSaveDroid->tarStatName), "writeUnitFile; psTarStat pName Error");
				sstrcpy(psSaveDroid->tarStatName, psCurr->psTarStats->pName);
			}
			else
			{
				strcpy(psSaveDroid->tarStatName,"");
			}

			psSaveDroid->baseStructID = NULL_ID;
			if ((psCurr->psBaseStruct != NULL))
			{
				UDWORD _tmpid;
				FIXME_CAST_ASSIGN(UDWORD, _tmpid, psCurr->psBaseStruct);
				if (_tmpid != NULL_ID && psCurr->psBaseStruct->died <= 1 && checkValidId(psCurr->psBaseStruct->id))
				{
					psSaveDroid->baseStructID = psCurr->psBaseStruct->id;
				}
			}

			psSaveDroid->group = psCurr->group;
			psSaveDroid->selected = psCurr->selected;
			psSaveDroid->died = psCurr->died;
			psSaveDroid->lastEmission = psCurr->lastEmission;
			for (i=0; i < MAX_PLAYERS; i++)
			{
				psSaveDroid->visible[i] = psCurr->visible[i];
			}

			//version 21
			psSaveDroid->commandId = NULL_ID;
			if (hasCommander(psCurr))
			{
				if (((DROID*)psCurr->psGroup->psCommander)->died <= 1)
				{
					psSaveDroid->commandId = ((DROID*)psCurr->psGroup->psCommander)->id;
				}
				ASSERT( ((DROID*)psCurr->psGroup->psCommander)->died <= 1, "SaveUnit pcCommander died" );
				ASSERT( checkValidId(((DROID*)psCurr->psGroup->psCommander)->id), "SaveUnit pcCommander not found" );
			}

			//version 24
			psSaveDroid->resistance = psCurr->resistance;

			// Save the move control
			SaveDroidMoveControl(psSaveDroid, psCurr);

			psSaveDroid->id = psCurr->id;
			psSaveDroid->x = psCurr->pos.x;
			psSaveDroid->y = psCurr->pos.y;
			psSaveDroid->z = psCurr->pos.z;
			psSaveDroid->direction = UNDEG(psCurr->rot.direction);
			psSaveDroid->player = psCurr->player;
			psSaveDroid->inFire = psCurr->inFire;
			psSaveDroid->burnStart = psCurr->burnStart;
			psSaveDroid->burnDamage = psCurr->burnDamage;
			psSaveDroid->droidType = (UBYTE)psCurr->droidType;
			psSaveDroid->saveType = (UBYTE)saveType;//identifies special load cases

			/* SAVE_DROID is DROID_SAVE_V24 */
			/* DROID_SAVE_V24 includes DROID_SAVE_V21 */
			endian_sdword(&psSaveDroid->resistance);

			// psSaveDroid->formationDir, psSaveDroid->formationX and psSaveDroid->formationY are set by SaveDroidMoveControl
			// already, which also performs endian swapping, so we can (and should!) safely ignore those here.

			/* DROID_SAVE_V21 includes DROID_SAVE_V20 */
			endian_udword(&psSaveDroid->commandId);
			/* DROID_SAVE_V20 includes OBJECT_SAVE_V20 */
			endian_udword(&psSaveDroid->body);
			endian_udword(&psSaveDroid->saveType);
			for(i = 0; i < TEMP_DROID_MAXPROGS; i++) {
				endian_udword(&psSaveDroid->asWeaps[i].ammo);
				endian_udword(&psSaveDroid->asWeaps[i].lastFired);
			}
			endian_udword(&psSaveDroid->numKills);
			//Watermelon:endian_udword for new save format
			for(i = 0;i < psSaveDroid->numWeaps;i++)
			{
				endian_uword(&psSaveDroid->turretRotation[i]);
				endian_uword(&psSaveDroid->turretPitch[i]);
			}
			endian_udword(&psSaveDroid->numWeaps);
			endian_sdword(&psSaveDroid->order);
			endian_uword(&psSaveDroid->orderX);
			endian_uword(&psSaveDroid->orderY);
			endian_uword(&psSaveDroid->orderX2);
			endian_uword(&psSaveDroid->orderY2);
			endian_udword(&psSaveDroid->timeLastHit);
			endian_udword(&psSaveDroid->targetID);
			endian_udword(&psSaveDroid->secondaryOrder);
			endian_sdword(&psSaveDroid->action);
			endian_udword(&psSaveDroid->actionX);
			endian_udword(&psSaveDroid->actionY);
			endian_udword(&psSaveDroid->actionTargetID);
			endian_udword(&psSaveDroid->actionStarted);
			endian_udword(&psSaveDroid->actionPoints);
			endian_uword(&psSaveDroid->actionHeight);
			endian_udword(&psSaveDroid->baseStructID);
			endian_udword(&psSaveDroid->died);
			endian_udword(&psSaveDroid->lastEmission);
			/* OBJECT_SAVE_V20 */
			endian_udword(&psSaveDroid->id);
			endian_udword(&psSaveDroid->x);
			endian_udword(&psSaveDroid->y);
			endian_udword(&psSaveDroid->z);
			endian_udword(&psSaveDroid->direction);
			endian_udword(&psSaveDroid->player);
			endian_udword(&psSaveDroid->burnStart);
			endian_udword(&psSaveDroid->burnDamage);

			return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of droids for each player to a file
*/
BOOL writeDroidFile(char *pFileName, DROID **ppsCurrentDroidLists)
{
	char *pFileData = NULL;
	UDWORD				fileSize, player, totalDroids=0;
	DROID				*psCurr;
	DROID				*psTrans;
	DROID_SAVEHEADER	*psHeader;
	SAVE_DROID			*psSaveDroid;
	BOOL status = true;

	//total all the droids in the world
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurr = ppsCurrentDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			totalDroids++;
			// if transporter save any droids in the grp
			if (psCurr->droidType == DROID_TRANSPORTER)
			{
				psTrans = psCurr->psGroup->psList;
// Some MSVC specific debug code to check for dangling pointers
#if defined(WZ_CC_MSVC)
				{
					void* dangling_ptr;

					// Fill the memory with 0xcd, which MSVC initialises freshly
					// allocated memory with.
					memset(&dangling_ptr, 0xcd, sizeof(dangling_ptr));

					if (psTrans->psGrpNext == dangling_ptr)
					{
						debug( LOG_ERROR, "transporter ->psGrpNext not reset" );
						return false;
					}
				}
#endif

				for (psTrans = psTrans->psGrpNext; psTrans != NULL; psTrans = psTrans->psGrpNext)
				{
					totalDroids++;
				}
			}
		}
	}

	/* Allocate the data buffer */
	fileSize = DROID_HEADER_SIZE + totalDroids*sizeof(SAVE_DROID);
	pFileData = (char*)malloc(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	/* Put the file header on the file */
	psHeader = (DROID_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'd';
	psHeader->aFileType[1] = 'r';
	psHeader->aFileType[2] = 'o';
	psHeader->aFileType[3] = 'd';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalDroids;

	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "file version is %u for (%s) ", psHeader->version, pFileName);

	psSaveDroid = (SAVE_DROID*)(pFileData + DROID_HEADER_SIZE);

	/* Put the droid data into the buffer */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psCurr = ppsCurrentDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
            //always save transporter droids that are in the mission list with an INVALID_XY
            if (psCurr->droidType == DROID_TRANSPORTER &&
                ppsCurrentDroidLists[player] == mission.apsDroidLists[player])
            {
                psCurr->pos.x = INVALID_XY;
                psCurr->pos.y = INVALID_XY;
            }

			buildSaveDroidFromDroid(psSaveDroid, psCurr, DROID_NORMAL);

			psSaveDroid = (SAVE_DROID *)((char *)psSaveDroid + sizeof(SAVE_DROID));
			// if transporter save any droids in the grp
			if (psCurr->droidType == DROID_TRANSPORTER)
			{
				psTrans = psCurr->psGroup->psList;
				for(psTrans = psCurr->psGroup->psList; psTrans != NULL; psTrans = psTrans->psGrpNext)
				{
					if (psTrans->droidType != DROID_TRANSPORTER)
					{
						buildSaveDroidFromDroid(psSaveDroid, psTrans, DROID_ON_TRANSPORT);
						psSaveDroid = (SAVE_DROID *)((char *)psSaveDroid + sizeof(SAVE_DROID));
					}
				}
			}
		}
	}

	ppsCurrentDroidLists = NULL;//ensure it always gets set

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		free(pFileData);
		return status;
	}
	return false;
}


// -----------------------------------------------------------------------------------------
BOOL loadSaveStructure(char *pFileData, UDWORD filesize)
{
	STRUCT_SAVEHEADER		*psHeader;

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
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "StructLoad: unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version < VERSION_9)
	{
		if (!loadSaveStructureV7(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveStructureV19(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveStructureV(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported save format version %u", psHeader->version);
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
/* code specific to version 7 of a save structure */
BOOL loadSaveStructureV7(char *pFileData, UDWORD filesize, UDWORD numStructures)
{
	SAVE_STRUCTURE_V2		*psSaveStructure, sSaveStructure;
	STRUCTURE				*psStructure;
	REPAIR_FACILITY			*psRepair;
	STRUCTURE_STATS			*psStats = NULL;
	UDWORD					count, statInc;
	BOOL					found;
	UDWORD					NumberOfSkippedStructures=0;
	UDWORD					burnTime;

	psSaveStructure = &sSaveStructure;

	if ((sizeof(SAVE_STRUCTURE_V2) * numStructures + STRUCT_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "structureLoad: unexpected end of file" );

		return false;
	}

	/* Load in the structure data */
	for (count = 0; count < numStructures; count ++, pFileData += sizeof(SAVE_STRUCTURE_V2))
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


		if (!getSaveObjectName(psSaveStructure->name))
		{
			continue;
		}

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
			psStructure = getTileStructure(map_coord(psSaveStructure->x),
				map_coord(psSaveStructure->y));
			if (psStructure == NULL)
			{
				debug( LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->player );
				//ignore this module
				continue;
			}
		}

        //check not trying to build too near the edge
    	if (map_coord(psSaveStructure->x) < TOO_NEAR_EDGE
    	 || map_coord(psSaveStructure->x) > mapWidth - TOO_NEAR_EDGE)
        {
			debug( LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			//ignore this
            continue;
        }
    	if (map_coord(psSaveStructure->y) < TOO_NEAR_EDGE
    	 || map_coord(psSaveStructure->y) > mapHeight - TOO_NEAR_EDGE)
        {
			debug( LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			//ignore this
            continue;
        }

        psStructure = buildStructure(psStats, psSaveStructure->x, psSaveStructure->y,
			psSaveStructure->player,true);
		ASSERT(psStructure, "Unable to create structure");
		if (!psStructure) continue;

        /*The original code here didn't work and so the scriptwriters worked
        round it by using the module ID - so making it work now will screw up
        the scripts -so in ALL CASES overwrite the ID!*/
		//don't copy the module's id etc
		//if (IsStatExpansionModule(psStats)==false)
		{
			//copy the values across
			psStructure->id = psSaveStructure->id;
			//are these going to ever change from the values set up with?
			psStructure->rot.direction = DEG(psSaveStructure->direction);
		}

		psStructure->inFire = psSaveStructure->inFire;
		psStructure->burnDamage = psSaveStructure->burnDamage;
		burnTime = psSaveStructure->burnStart;
		psStructure->burnStart = burnTime;

		psStructure->status = psSaveStructure->status;
		if (psStructure->status ==SS_BUILT)
		{
			buildingComplete(psStructure);
		}

		if (psStructure->pStructureType->type == REF_HQ)
		{
			scriptSetStartPos(psSaveStructure->player, psStructure->pos.x, psStructure->pos.y);
		}

		//if not a save game, don't want to overwrite any of the stats so continue
		if (gameType != GTYPE_SAVE_START)
		{
			continue;
		}

		psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
		switch (psStructure->pStructureType->type)
		{
			case REF_FACTORY:
				//NOT DONE AT PRESENT
				((FACTORY *)psStructure->pFunctionality)->capacity = (UBYTE)psSaveStructure->
					capacity;
				//((FACTORY *)psStructure->pFunctionality)->productionOutput = psSaveStructure->
				//	output;
				//((FACTORY *)psStructure->pFunctionality)->quantity = psSaveStructure->
				//	quantity;
				//((FACTORY *)psStructure->pFunctionality)->timeStarted = gameTime -
				//	savedGameTime - (psSaveStructure->timeStarted);
				//((FACTORY*)psStructure->pFunctionality)->timeToBuild = ((DROID_TEMPLATE *)
				//	psSaveStructure->subjectInc)->buildPoints / ((FACTORY *)psStructure->pFunctionality)->
				//	productionOutput;

				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
					psStructure->sDisplay.imd = factoryModuleIMDs[psSaveStructure->
						capacity-1][0];
				}
				break;
			case REF_RESEARCH:
				((RESEARCH_FACILITY *)psStructure->pFunctionality)->capacity =
					psSaveStructure->capacity;
				((RESEARCH_FACILITY *)psStructure->pFunctionality)->researchPoints =
					psSaveStructure->output;
				((RESEARCH_FACILITY *)psStructure->pFunctionality)->timeStarted = (psSaveStructure->timeStarted);
				if (psSaveStructure->subjectInc != (UDWORD)-1)
				{
					((RESEARCH_FACILITY *)psStructure->pFunctionality)->psSubject = (BASE_STATS *)
						(asResearch + psSaveStructure->subjectInc);
					((RESEARCH_FACILITY*)psStructure->pFunctionality)->timeToResearch =
						(asResearch + psSaveStructure->subjectInc)->researchPoints /
						((RESEARCH_FACILITY *)psStructure->pFunctionality)->
						researchPoints;
				}
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
					psStructure->sDisplay.imd = researchModuleIMDs[psSaveStructure->
						capacity-1];
				}
				break;
			case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
				psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);
				psRepair->psDeliveryPoint = NULL;
				psRepair->psObj = NULL;
				psRepair->currentPtsAdded = 0;
				psRepair->droidQueue = 0;
				break;
			default:
				break;
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
static UDWORD getResearchIdFromName(char *pName)
{
	UDWORD inc;


	for (inc=0; inc < numResearch; inc++)
	{

		if (!strcmp(asResearch[inc].pName, pName))
		{
			return inc;
		}
	}

	debug( LOG_ERROR, "Unknown research - %s", pName );

	return NULL_ID;
}


// -----------------------------------------------------------------------------------------
/* code for version upto 19of a save structure */
BOOL loadSaveStructureV19(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version)
{
	SAVE_STRUCTURE_V17			*psSaveStructure, sSaveStructure;
	STRUCTURE				*psStructure;
	FACTORY					*psFactory;
	RESEARCH_FACILITY		*psResearch;
	REPAIR_FACILITY			*psRepair;
	STRUCTURE_STATS			*psStats = NULL;
	STRUCTURE_STATS			*psModule;
	UDWORD					capacity;
	UDWORD					count, statInc;
	BOOL					found;
	UDWORD					NumberOfSkippedStructures=0;
	UDWORD					burnTime;
	UDWORD					i;
	UDWORD					sizeOfSaveStruture;
	UDWORD					researchId;

	if (version < VERSION_12)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V2);
	}
	else if (version < VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V12);
	}
	else if (version <= VERSION_14)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V14);
	}
	else if (version <= VERSION_16)
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V15);
	}
	else
	{
		sizeOfSaveStruture = sizeof(SAVE_STRUCTURE_V17);
	}

	psSaveStructure = &sSaveStructure;

	if ((sizeOfSaveStruture * numStructures + STRUCT_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "structureLoad: unexpected end of file" );

		return false;
	}

	/* Load in the structure data */
	for (count = 0; count < numStructures; count ++, pFileData += sizeOfSaveStruture)
	{
		memcpy(psSaveStructure, pFileData, sizeOfSaveStruture);

		/* SAVE_STRUCTURE_V17 is STRUCTURE_SAVE_V17 */
		/* STRUCTURE_SAVE_V17 includes STRUCTURE_SAVE_V15 */
		endian_sword(&psSaveStructure->currentPowerAccrued);
		/* STRUCTURE_SAVE_V15 includes STRUCTURE_SAVE_V14 */
		/* STRUCTURE_SAVE_V14 includes STRUCTURE_SAVE_V12 */
		endian_udword(&psSaveStructure->factoryInc);
		endian_udword(&psSaveStructure->powerAccrued);
		endian_udword(&psSaveStructure->dummy2);
		endian_udword(&psSaveStructure->droidTimeStarted);
		endian_udword(&psSaveStructure->timeToBuild);
		endian_udword(&psSaveStructure->timeStartHold);
		/* STRUCTURE_SAVE_V12 includes STRUCTURE_SAVE_V2 */
		endian_sdword(&psSaveStructure->currentBuildPts);
		endian_udword(&psSaveStructure->body);
		endian_udword(&psSaveStructure->armour);
		endian_udword(&psSaveStructure->resistance);
		endian_udword(&psSaveStructure->dummy1);
		endian_udword(&psSaveStructure->subjectInc);
		endian_udword(&psSaveStructure->timeStarted);
		endian_udword(&psSaveStructure->output);
		endian_udword(&psSaveStructure->capacity);
		/* STRUCTURE_SAVE_V2 includes OBJECT_SAVE_V19 */
		endian_udword(&psSaveStructure->id);
		endian_udword(&psSaveStructure->x);
		endian_udword(&psSaveStructure->y);
		endian_udword(&psSaveStructure->z);
		endian_udword(&psSaveStructure->direction);
		endian_udword(&psSaveStructure->player);
		endian_udword(&psSaveStructure->burnStart);
		endian_udword(&psSaveStructure->burnDamage);

		if (psSaveStructure->player >= MAX_PLAYERS)
		{
			psSaveStructure->player=MAX_PLAYERS-1;
			NumberOfSkippedStructures++;

		}
		//get the stats for this structure
		found = false;


		if (!getSaveObjectName(psSaveStructure->name))
		{
			continue;
		}

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
			debug( LOG_ERROR, "This structure no longer exists - %s", getSaveStructNameV19(psSaveStructure) );
			//ignore this
			continue;
		}
		/*create the Structure */
		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			psStructure = getTileStructure(map_coord(psSaveStructure->x),
				map_coord(psSaveStructure->y));
			if (psStructure == NULL)
			{
				debug( LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV19(psSaveStructure), psSaveStructure->player );
				//ignore this module
				continue;
			}
		}
        //check not trying to build too near the edge
    	if (map_coord(psSaveStructure->x) < TOO_NEAR_EDGE
    	 || map_coord(psSaveStructure->x) > mapWidth - TOO_NEAR_EDGE)
        {
			debug( LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			//ignore this
            continue;
        }
    	if (map_coord(psSaveStructure->y) < TOO_NEAR_EDGE
    	 || map_coord(psSaveStructure->y) > mapHeight - TOO_NEAR_EDGE)
        {
			debug( LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17*)psSaveStructure), psSaveStructure->id );
			//ignore this
            continue;
        }

	psStructure = buildStructure(psStats, psSaveStructure->x, psSaveStructure->y, psSaveStructure->player,true);
		ASSERT(psStructure, "Unable to create structure");
		if (!psStructure) continue;

        /*The original code here didn't work and so the scriptwriters worked
        round it by using the module ID - so making it work now will screw up
        the scripts -so in ALL CASES overwrite the ID!*/
		//don't copy the module's id etc
		//if (IsStatExpansionModule(psStats)==false)
		{
			//copy the values across
			psStructure->id = psSaveStructure->id;
			//are these going to ever change from the values set up with?
//			psStructure->pos.z = (UWORD)psSaveStructure->pos.z;
			psStructure->rot.direction = DEG(psSaveStructure->direction);
		}

		psStructure->inFire = psSaveStructure->inFire;
		psStructure->burnDamage = psSaveStructure->burnDamage;
		burnTime = psSaveStructure->burnStart;
		psStructure->burnStart = burnTime;
		if (version >= VERSION_14)
		{
			for (i=0; i < MAX_PLAYERS; i++)
			{
				psStructure->visible[i] = psSaveStructure->visible[i];
			}
		}
		psStructure->status = psSaveStructure->status;
		if (psStructure->status ==SS_BUILT)
		{
			buildingComplete(psStructure);
		}

		if (psStructure->pStructureType->type == REF_HQ)
		{
			scriptSetStartPos(psSaveStructure->player, psStructure->pos.x, psStructure->pos.y);
		}

		//if not a save game, don't want to overwrite any of the stats so continue
		if ((gameType != GTYPE_SAVE_START) &&
			(gameType != GTYPE_SAVE_MIDMISSION))
		{
			continue;
		}

		if (version <= VERSION_16)
		{
			if (psSaveStructure->currentBuildPts > SWORD_MAX)//old MAX_BODY
			{
				psSaveStructure->currentBuildPts = SWORD_MAX;
			}
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
			psStructure->currentPowerAccrued = 0;
		}
		else
		{
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
			psStructure->currentPowerAccrued = psSaveStructure->currentPowerAccrued;
		}
//		psStructure->repair = (UWORD)psSaveStructure->repair;
		switch (psStructure->pStructureType->type)
		{
			case REF_FACTORY:
            case REF_VTOL_FACTORY:
            case REF_CYBORG_FACTORY:
				//if factory save the current build info
				if (version >= VERSION_12)
				{
					psFactory = ((FACTORY *)psStructure->pFunctionality);
					psFactory->capacity = 0;//capacity reset during module build (UBYTE)psSaveStructure->capacity;
                    //this is set up during module build - if the stats have changed it will also set up with the latest value
					//psFactory->productionOutput = (UBYTE)psSaveStructure->output;
					psFactory->productionLoops = (UBYTE)psSaveStructure->quantity;
					psFactory->timeStarted = psSaveStructure->droidTimeStarted;
					psFactory->powerAccrued = psSaveStructure->powerAccrued;
					psFactory->timeToBuild = psSaveStructure->timeToBuild;
					psFactory->timeStartHold = psSaveStructure->timeStartHold;

					//adjust the module structures IMD
					if (psSaveStructure->capacity)
					{
					    psModule = getModuleStat(psStructure);
						capacity = psSaveStructure->capacity;
						//build the appropriate number of modules
						while (capacity)
						{
		                    buildStructure(psModule, psStructure->pos.x, psStructure->pos.y,
                                psStructure->player, false);
                            capacity--;
						}

					}
				// imd set by building modules
				//	psStructure->sDisplay.imd = factoryModuleIMDs[psSaveStructure->capacity-1][0];

				//if factory reset the delivery points
					//this trashes the flag pos pointer but flag pos list is cleared when flags load
					//assemblyCheck
					FIXME_CAST_ASSIGN(UDWORD, psFactory->psAssemblyPoint, psSaveStructure->factoryInc);
					//if factory was building find the template from the unique ID
					if (psSaveStructure->subjectInc == NULL_ID)
					{
						psFactory->psSubject = NULL;
					}
					else
					{
						psFactory->psSubject = (BASE_STATS*)
                            getTemplateFromMultiPlayerID(psSaveStructure->subjectInc);
                        //if the build has started set the powerAccrued =
                        //powerRequired to sync the interface
                        if (psFactory->timeStarted != ACTION_START_TIME &&
                            psFactory->psSubject)
                        {
                            psFactory->powerAccrued = ((DROID_TEMPLATE *)psFactory->
                                psSubject)->powerPoints;
                        }
					}
				}
				break;
			case REF_RESEARCH:
				psResearch = ((RESEARCH_FACILITY *)psStructure->pFunctionality);
				psResearch->capacity = 0;//capacity set when module loaded psSaveStructure->capacity;
                //this is set up during module build - if the stats have changed it will also set up with the latest value
                //psResearch->researchPoints = psSaveStructure->output;
				psResearch->powerAccrued = psSaveStructure->powerAccrued;
				//clear subject
				psResearch->psSubject = NULL;
				psResearch->timeToResearch = 0;
				psResearch->timeStarted = 0;
				//set the subject
				if (saveGameVersion >= VERSION_15)
				{
					if (psSaveStructure->subjectInc != NULL_ID)
					{
						researchId = getResearchIdFromName(psSaveStructure->researchName);
						if (researchId != NULL_ID)
						{
							psResearch->psSubject = (BASE_STATS *)(asResearch + researchId);
							psResearch->timeToResearch = (asResearch + researchId)->researchPoints / psResearch->researchPoints;
							psResearch->timeStarted = psSaveStructure->timeStarted;
						}
					}
					else
					{
						psResearch->psSubject = NULL;
						psResearch->timeToResearch = 0;
						psResearch->timeStarted = 0;
					}
				}
				else
				{
					psResearch->timeStarted = (psSaveStructure->timeStarted);
					if (psSaveStructure->subjectInc != NULL_ID)
					{
						psResearch->psSubject = (BASE_STATS *)(asResearch + psSaveStructure->subjectInc);
						psResearch->timeToResearch = (asResearch + psSaveStructure->subjectInc)->researchPoints / psResearch->researchPoints;
					}

				}
                //if started research, set powerAccrued = powerRequired
                if (psResearch->timeStarted != ACTION_START_TIME && psResearch->
                    psSubject)
                {
                    psResearch->powerAccrued = ((RESEARCH *)psResearch->
                        psSubject)->researchPower;
                }
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
				    psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
					buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
				}
				break;
			case REF_POWER_GEN:
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
				    psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
					buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				break;
			case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
				if (version >= VERSION_19)
				{
					psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);

					psRepair->power = ((REPAIR_DROID_FUNCTION *) psStructure->pStructureType->asFuncList[0])->repairPoints;
					psRepair->timeStarted = psSaveStructure->droidTimeStarted;
					psRepair->powerAccrued = psSaveStructure->powerAccrued;
                    psRepair->currentPtsAdded = 0;

					//if repair facility reset the delivery points
					//this trashes the flag pos pointer but flag pos list is cleared when flags load
					//assemblyCheck
					psRepair->psDeliveryPoint = NULL;
					//if factory was building find the template from the unique ID
					if (psSaveStructure->subjectInc == NULL_ID)
					{
						psRepair->psObj = NULL;
					}
					else
					{
						psRepair->psObj = getBaseObjFromId(psSaveStructure->subjectInc);
                        //if the build has started set the powerAccrued =
                        //powerRequired to sync the interface
                        if (psRepair->timeStarted != ACTION_START_TIME &&
                            psRepair->psObj)
                        {
                            psRepair->powerAccrued = powerReqForDroidRepair((DROID*)psRepair->psObj);
                        }
					}
				}
				else
				{
					psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);
                    //init so setAssemplyPoint check will re-allocate one
					psRepair->psObj = NULL;
                    psRepair->psDeliveryPoint = NULL;
				}
				break;
			default:
				break;
		}
		//get the base body points
		psStructure->body = (UWORD)structureBody(psStructure);
		if (psSaveStructure->body < psStructure->body)
		{
			psStructure->body = (UWORD)psSaveStructure->body;
		}
		//set the build status from the build points
		psStructure->currentBuildPts = (SWORD)psStructure->pStructureType->buildPoints;
		if (psSaveStructure->currentBuildPts < psStructure->currentBuildPts)
		{
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
            psStructure->status = SS_BEING_BUILT;
		}
		else
		{
            psStructure->status = SS_BUILT;
//            buildingComplete(psStructure);//replaced by following switch
			switch (psStructure->pStructureType->type)
			{
				case REF_POWER_GEN:
					checkForResExtractors(psStructure);
					if(selectedPlayer == psStructure->player)
					{
						audio_PlayObjStaticTrack( (void *) psStructure, ID_SOUND_POWER_HUM );
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
				case REF_RESEARCH:
					break;
				default:
					//do nothing for factories etc
					break;
			}
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
/* code for versions after version 20 of a save structure */
BOOL loadSaveStructureV(char *pFileData, UDWORD filesize, UDWORD numStructures, UDWORD version)
{
	SAVE_STRUCTURE			*psSaveStructure, sSaveStructure;
	STRUCTURE				*psStructure;
	FACTORY					*psFactory;
	RESEARCH_FACILITY		*psResearch;
	REPAIR_FACILITY			*psRepair;
	REARM_PAD				*psReArmPad;
	STRUCTURE_STATS			*psStats = NULL;
	STRUCTURE_STATS			*psModule;
	UDWORD					capacity;
	UDWORD					count, statInc;
	BOOL					found;
	UDWORD					NumberOfSkippedStructures=0;
	UDWORD					burnTime;
	UDWORD					i;
	UDWORD					sizeOfSaveStructure = 0;
	UDWORD					researchId;

	debug(LOG_SAVE, "fileversion is %u ", version);

	if (version <= VERSION_20)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE_V20);
	}
	else if (version <= CURRENT_VERSION_NUM)
	{
		sizeOfSaveStructure = sizeof(SAVE_STRUCTURE);
	}

	psSaveStructure = &sSaveStructure;

	if ((sizeOfSaveStructure * numStructures + STRUCT_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "structureLoad: unexpected end of file" );

		return false;
	}

	/* Load in the structure data */
	for (count = 0; count < numStructures; count ++, pFileData += sizeOfSaveStructure)
	{
		memcpy(psSaveStructure, pFileData, sizeOfSaveStructure);

		/* SAVE_STRUCTURE is STRUCTURE_SAVE_V21 */
		/* STRUCTURE_SAVE_V21 includes STRUCTURE_SAVE_V20 */
		endian_udword(&psSaveStructure->commandId);
		/* STRUCTURE_SAVE_V20 includes OBJECT_SAVE_V20 */
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
		endian_udword(&psSaveStructure->factoryInc);
		endian_udword(&psSaveStructure->powerAccrued);
		endian_udword(&psSaveStructure->dummy2);
		endian_udword(&psSaveStructure->droidTimeStarted);
		endian_udword(&psSaveStructure->timeToBuild);
		endian_udword(&psSaveStructure->timeStartHold);
		endian_sword(&psSaveStructure->currentPowerAccrued);
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveStructure->id);
		endian_udword(&psSaveStructure->x);
		endian_udword(&psSaveStructure->y);
		endian_udword(&psSaveStructure->z);
		endian_udword(&psSaveStructure->direction);
		endian_udword(&psSaveStructure->player);
		endian_udword(&psSaveStructure->burnStart);
		endian_udword(&psSaveStructure->burnDamage);

		if (psSaveStructure->player >= MAX_PLAYERS)
		{
			psSaveStructure->player=MAX_PLAYERS-1;
			NumberOfSkippedStructures++;

		}
		//get the stats for this structure
		found = false;


		if (!getSaveObjectName(psSaveStructure->name))
		{
			continue;
		}

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
			debug( LOG_ERROR, "This structure no longer exists - %s", getSaveStructNameV(psSaveStructure) );
			//ignore this
			continue;
		}
		/*create the Structure */
		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			psStructure = getTileStructure(map_coord(psSaveStructure->x),
			                               map_coord(psSaveStructure->y));
			if (psStructure == NULL)
			{
				debug( LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV(psSaveStructure), psSaveStructure->player );
				//ignore this module
				continue;
			}
		}
        //check not trying to build too near the edge
    	if (map_coord(psSaveStructure->x) < TOO_NEAR_EDGE
    	 || map_coord(psSaveStructure->x) > mapWidth - TOO_NEAR_EDGE)
        {
			debug( LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV((SAVE_STRUCTURE*)psSaveStructure), psSaveStructure->id );
			//ignore this
            continue;
        }
    	if (map_coord(psSaveStructure->y) < TOO_NEAR_EDGE
    	 || map_coord(psSaveStructure->y) > mapHeight - TOO_NEAR_EDGE)
        {
			debug( LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV((SAVE_STRUCTURE*)psSaveStructure), psSaveStructure->id );
			//ignore this
            continue;
        }

	psStructure = buildStructure(psStats, psSaveStructure->x, psSaveStructure->y, psSaveStructure->player, true);
		ASSERT(psStructure, "Unable to create structure");
		if (!psStructure) continue;

        /*The original code here didn't work and so the scriptwriters worked
        round it by using the module ID - so making it work now will screw up
        the scripts -so in ALL CASES overwrite the ID!*/
		//don't copy the module's id etc
		//if (IsStatExpansionModule(psStats)==false)
		{
			//copy the values across
			psStructure->id = psSaveStructure->id;
			//are these going to ever change from the values set up with?
//			psStructure->pos.z = (UWORD)psSaveStructure->pos.z;
			psStructure->rot.direction = DEG(psSaveStructure->direction);
		}

		psStructure->inFire = psSaveStructure->inFire;
		psStructure->burnDamage = psSaveStructure->burnDamage;
		burnTime = psSaveStructure->burnStart;
		psStructure->burnStart = burnTime;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psStructure->visible[i] = psSaveStructure->visible[i];
		}

		psStructure->status = psSaveStructure->status;
		if (psStructure->status ==SS_BUILT)
		{
			buildingComplete(psStructure);
		}

		if (psStructure->pStructureType->type == REF_HQ)
		{
			scriptSetStartPos(psSaveStructure->player, psStructure->pos.x, psStructure->pos.y);
		}

		//if not a save game, don't want to overwrite any of the stats so continue
		if ((gameType != GTYPE_SAVE_START) &&
			(gameType != GTYPE_SAVE_MIDMISSION))
		{
			continue;
		}

		psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
		psStructure->currentPowerAccrued = psSaveStructure->currentPowerAccrued;
		psStructure->resistance = (SWORD)psSaveStructure->resistance;
//		psStructure->repair = (UWORD)psSaveStructure->repair;
		switch (psStructure->pStructureType->type)
		{
			case REF_FACTORY:
			case REF_VTOL_FACTORY:
			case REF_CYBORG_FACTORY:
				//if factory save the current build info
				psFactory = ((FACTORY *)psStructure->pFunctionality);
				psFactory->capacity = 0;//capacity reset during module build (UBYTE)psSaveStructure->capacity;
				//this is set up during module build - if the stats have changed it will also set up with the latest value
				//psFactory->productionOutput = (UBYTE)psSaveStructure->output;
				psFactory->productionLoops = (UBYTE)psSaveStructure->quantity;
				psFactory->timeStarted = psSaveStructure->droidTimeStarted;
				psFactory->powerAccrued = psSaveStructure->powerAccrued;
				psFactory->timeToBuild = psSaveStructure->timeToBuild;
				psFactory->timeStartHold = psSaveStructure->timeStartHold;

				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
					psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
					while (capacity)
					{
						buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
						capacity--;
					}

				}
			// imd set by building modules
			//	psStructure->sDisplay.imd = factoryModuleIMDs[psSaveStructure->capacity-1][0];

			//if factory reset the delivery points
				//this trashes the flag pos pointer but flag pos list is cleared when flags load
				//assemblyCheck
				FIXME_CAST_ASSIGN(UDWORD, psFactory->psAssemblyPoint, psSaveStructure->factoryInc);
				//if factory was building find the template from the unique ID
				if (psSaveStructure->subjectInc == NULL_ID)
				{
					psFactory->psSubject = NULL;
				}
				else
				{
					psFactory->psSubject = (BASE_STATS*)
                        getTemplateFromMultiPlayerID(psSaveStructure->subjectInc);
                    //if the build has started set the powerAccrued =
                    //powerRequired to sync the interface
                    if (psFactory->timeStarted != ACTION_START_TIME &&
                        psFactory->psSubject)
                    {
                        psFactory->powerAccrued = ((DROID_TEMPLATE *)psFactory->
                            psSubject)->powerPoints;
                    }
				}
				if (version >= VERSION_21)//version 21
				{
					//reset command id in loadStructSetPointers
					FIXME_CAST_ASSIGN(UDWORD, psFactory->psCommander, psSaveStructure->commandId);
				}
                //secondary order added - AB 22/04/99
                if (version >= VERSION_32)
                {
                    psFactory->secondaryOrder = psSaveStructure->dummy2;
                }
				break;
			case REF_RESEARCH:
				psResearch = ((RESEARCH_FACILITY *)psStructure->pFunctionality);
				psResearch->capacity = 0;//capacity set when module loaded psSaveStructure->capacity;
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
					psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
					buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
				}
				//this is set up during module build - if the stats have changed it will also set up with the latest value
				psResearch->powerAccrued = psSaveStructure->powerAccrued;
				//clear subject
				psResearch->psSubject = NULL;
				psResearch->timeToResearch = 0;
				psResearch->timeStarted = 0;
				psResearch->timeStartHold = 0;
				//set the subject
				if (psSaveStructure->subjectInc != NULL_ID)
				{
					researchId = getResearchIdFromName(psSaveStructure->researchName);
					if (researchId != NULL_ID)
					{
						psResearch->psSubject = (BASE_STATS *)(asResearch + researchId);
						psResearch->timeToResearch = (asResearch + researchId)->researchPoints / psResearch->researchPoints;
						psResearch->timeStarted = psSaveStructure->timeStarted;
						if (saveGameVersion >= VERSION_20)
						{
							psResearch->timeStartHold = psSaveStructure->timeStartHold;
						}
					}
				}
                //if started research, set powerAccrued = powerRequired
                if (psResearch->timeStarted != ACTION_START_TIME && psResearch->
                    psSubject)
                {
                    psResearch->powerAccrued = ((RESEARCH *)psResearch->
                        psSubject)->researchPower;
                }
				break;
			case REF_POWER_GEN:
				//adjust the module structures IMD
				if (psSaveStructure->capacity)
				{
				    psModule = getModuleStat(psStructure);
					capacity = psSaveStructure->capacity;
					//build the appropriate number of modules
					buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				break;
			case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
				psRepair = ((REPAIR_FACILITY *)psStructure->pFunctionality);

				psRepair->power = ((REPAIR_DROID_FUNCTION *) psStructure->pStructureType->asFuncList[0])->repairPoints;
				psRepair->timeStarted = psSaveStructure->droidTimeStarted;
				psRepair->powerAccrued = psSaveStructure->powerAccrued;

				//if repair facility reset the delivery points
				//this trashes the flag pos pointer but flag pos list is cleared when flags load
				//assemblyCheck
				psRepair->psDeliveryPoint = NULL;
				//if  repair facility was  repairing find the object later
				FIXME_CAST_ASSIGN(UDWORD, psRepair->psObj, psSaveStructure->subjectInc);
                if (version < VERSION_27)
                {
                    psRepair->currentPtsAdded = 0;
                }
                else
                {
                    psRepair->currentPtsAdded = psSaveStructure->dummy2;
                }
				break;
			case REF_REARM_PAD:
				if (version >= VERSION_26)//version 26
				{
					psReArmPad = ((REARM_PAD *)psStructure->pFunctionality);
					psReArmPad->reArmPoints = psSaveStructure->output;//set in build structure ?
					psReArmPad->timeStarted = psSaveStructure->droidTimeStarted;
					//if  ReArm Pad was  rearming find the object later
					FIXME_CAST_ASSIGN(UDWORD, psReArmPad->psObj, psSaveStructure->subjectInc);
                    if (version < VERSION_28)
                    {
                        psReArmPad->timeLastUpdated = 0;
                    }
                    else
                    {
                        psReArmPad->timeLastUpdated = psSaveStructure->dummy2;
                    }
				}
				else
				{
                    psReArmPad = ((REARM_PAD *)psStructure->pFunctionality);
					psReArmPad->timeStarted = 0;
				}
				break;
			default:
				break;
		}
		//get the base body points
		psStructure->body = (UWORD)structureBody(psStructure);
		if (psSaveStructure->body < psStructure->body)
		{
			psStructure->body = (UWORD)psSaveStructure->body;
		}
		//set the build status from the build points
		psStructure->currentPowerAccrued = psSaveStructure->currentPowerAccrued;//22feb
		psStructure->currentBuildPts = (SWORD)psStructure->pStructureType->buildPoints;
		if (psSaveStructure->currentBuildPts < psStructure->currentBuildPts)
		{
			psStructure->currentBuildPts = (SWORD)psSaveStructure->currentBuildPts;
            psStructure->status = SS_BEING_BUILT;
		}
		else
		{
            psStructure->status = SS_BUILT;
//            buildingComplete(psStructure);//replaced by following switch
			switch (psStructure->pStructureType->type)
			{
				case REF_POWER_GEN:
					checkForResExtractors(psStructure);
					if(selectedPlayer == psStructure->player)
					{
						audio_PlayObjStaticTrack( (void *) psStructure, ID_SOUND_POWER_HUM );
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
				case REF_RESEARCH:
					break;
				default:
					//do nothing for factories etc
					break;
			}
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
/*
Writes the linked list of structure for each player to a file
*/
BOOL writeStructFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD				fileSize, player, i, totalStructs=0;
	STRUCTURE			*psCurr;
	DROID_TEMPLATE		*psSubjectTemplate;
	FACTORY				*psFactory;
	REPAIR_FACILITY		*psRepair;
	REARM_PAD				*psReArmPad;
	STRUCT_SAVEHEADER *psHeader;
	SAVE_STRUCTURE		*psSaveStruct;
	FLAG_POSITION		*psFlag;
	UDWORD				researchId;
	BOOL status = true;

	//total all the structures in the world
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			totalStructs++;
		}
	}

	/* Allocate the data buffer */
	fileSize = STRUCT_HEADER_SIZE + totalStructs*sizeof(SAVE_STRUCTURE);
	pFileData = (char*)malloc(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	/* Put the file header on the file */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 's';
	psHeader->aFileType[1] = 't';
	psHeader->aFileType[2] = 'r';
	psHeader->aFileType[3] = 'u';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalStructs;

	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "file version is %u for %s ", psHeader->version, pFileName);

	psSaveStruct = (SAVE_STRUCTURE*)(pFileData + STRUCT_HEADER_SIZE);

	/* Put the structure data into the buffer */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{

			strcpy(psSaveStruct->name, psCurr->pStructureType->pName);

			psSaveStruct->id = psCurr->id;


			psSaveStruct->x = psCurr->pos.x;
			psSaveStruct->y = psCurr->pos.y;
			psSaveStruct->z = psCurr->pos.z;

			psSaveStruct->direction = UNDEG(psCurr->rot.direction);
			psSaveStruct->player = psCurr->player;
			psSaveStruct->inFire = psCurr->inFire;
			psSaveStruct->burnStart = psCurr->burnStart;
			psSaveStruct->burnDamage = psCurr->burnDamage;
			//version 14
			for (i=0; i < MAX_PLAYERS; i++)
			{
				psSaveStruct->visible[i] = psCurr->visible[i];
			}
			//psSaveStruct->structureInc = psCurr->pStructureType - asStructureStats;
			psSaveStruct->status = psCurr->status;
			//check if body at max
			if (psCurr->body >= structureBody(psCurr))
			{
				psSaveStruct->body = MAX_BODY;
			}
			else
			{
				psSaveStruct->body = psCurr->body;
			}
			//check if buildpts at max
			if (psCurr->currentBuildPts >= (SWORD)psCurr->pStructureType->buildPoints)
			{
				psSaveStruct->currentBuildPts = MAX_BODY;
			}
			else
			{
				psSaveStruct->currentBuildPts = psCurr->currentBuildPts;
			}
			// no need to check power at max because it would be being built
			psSaveStruct->currentPowerAccrued = psCurr->currentPowerAccrued;

			psSaveStruct->armour = psCurr->armour[0][0]; // advanced armour not supported yet
			psSaveStruct->resistance = psCurr->resistance;
			psSaveStruct->subjectInc = NULL_ID;
			psSaveStruct->timeStarted = 0;
			psSaveStruct->output = 0;
			psSaveStruct->capacity = 0;
			psSaveStruct->quantity = 0;
			if (psCurr->pFunctionality)
			{
				switch (psCurr->pStructureType->type)
				{
				case REF_FACTORY:
				case REF_CYBORG_FACTORY:
				case REF_VTOL_FACTORY:
					psFactory = ((FACTORY *)psCurr->pFunctionality);
					psSaveStruct->capacity	= psFactory->capacity;
					psSaveStruct->quantity                  = psFactory->productionLoops;
					psSaveStruct->droidTimeStarted	= psFactory->timeStarted;
					psSaveStruct->powerAccrued		= psFactory->powerAccrued;
					psSaveStruct->timeToBuild		= psFactory->timeToBuild;
					psSaveStruct->timeStartHold		= psFactory->timeStartHold;

    				if (psFactory->psSubject != NULL)
					{
						psSubjectTemplate = (DROID_TEMPLATE *)psFactory->psSubject;
						psSaveStruct->subjectInc = psSubjectTemplate->multiPlayerID;
					}
					else
					{
						psSaveStruct->subjectInc = NULL_ID;
					}

					psFlag = ((FACTORY *)psCurr->pFunctionality)->psAssemblyPoint;
					if (psFlag != NULL)
					{
						psSaveStruct->factoryInc = psFlag->factoryInc;
					}
					else
					{
						psSaveStruct->factoryInc = NULL_ID;
					}
					//version 21
					if (psFactory->psCommander)
					{
						psSaveStruct->commandId = psFactory->psCommander->id;
					}
					else
					{
						psSaveStruct->commandId = NULL_ID;
					}
					//secondary order added - AB 22/04/99
					psSaveStruct->dummy2 = psFactory->secondaryOrder;

					break;
				case REF_RESEARCH:
					psSaveStruct->capacity = ((RESEARCH_FACILITY *)psCurr->
						pFunctionality)->capacity;
					psSaveStruct->powerAccrued = ((RESEARCH_FACILITY *)psCurr->
						pFunctionality)->powerAccrued;
					psSaveStruct->timeStartHold	= ((RESEARCH_FACILITY *)psCurr->
						pFunctionality)->timeStartHold;
					if (((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject)
					{
						psSaveStruct->subjectInc = 0;
						researchId = ((RESEARCH_FACILITY *)psCurr->pFunctionality)->
							psSubject->ref - REF_RESEARCH_START;
						ASSERT(strlen(asResearch[researchId].pName) < sizeof(psSaveStruct->researchName), "writeStructData: research name too long");
						sstrcpy(psSaveStruct->researchName, asResearch[researchId].pName);
						psSaveStruct->timeStarted = ((RESEARCH_FACILITY *)psCurr->
							pFunctionality)->timeStarted;
					}
					else
					{
						psSaveStruct->subjectInc = NULL_ID;
						psSaveStruct->researchName[0] = 0;
						psSaveStruct->timeStarted = 0;
					}
					psSaveStruct->timeToBuild = 0;
					break;
				case REF_POWER_GEN:
					psSaveStruct->capacity = ((POWER_GEN *)psCurr->
						pFunctionality)->capacity;
					break;
				case REF_RESOURCE_EXTRACTOR:
					psSaveStruct->output = 1;
					break;
				case REF_REPAIR_FACILITY: //CODE THIS SOMETIME
					psRepair = ((REPAIR_FACILITY *)psCurr->pFunctionality);
					psSaveStruct->droidTimeStarted = psRepair->timeStarted;
					psSaveStruct->powerAccrued = psRepair->powerAccrued;
					psSaveStruct->dummy2 = psRepair->currentPtsAdded;

					if (psRepair->psObj != NULL)
					{
						psSaveStruct->subjectInc = psRepair->psObj->id;
					}
					else
					{
						psSaveStruct->subjectInc = NULL_ID;
					}

					psFlag = psRepair->psDeliveryPoint;
					if (psFlag != NULL)
					{
						psSaveStruct->factoryInc = psFlag->factoryInc;
					}
					else
					{
						psSaveStruct->factoryInc = NULL_ID;
					}
					break;
				case REF_REARM_PAD:
					psReArmPad = ((REARM_PAD *)psCurr->pFunctionality);
					psSaveStruct->output = psReArmPad->reArmPoints;
					psSaveStruct->droidTimeStarted = psReArmPad->timeStarted;
					psSaveStruct->dummy2 = psReArmPad->timeLastUpdated;
					if (psReArmPad->psObj != NULL)
					{
						psSaveStruct->subjectInc = psReArmPad->psObj->id;
					}
					else
					{
						psSaveStruct->subjectInc = NULL_ID;
					}
					break;
				default: //CODE THIS SOMETIME
					ASSERT( false,"Structure facility not saved" );
					break;
				}
			}

			/* SAVE_STRUCTURE is STRUCTURE_SAVE_V21 */
			/* STRUCTURE_SAVE_V21 includes STRUCTURE_SAVE_V20 */
			endian_udword(&psSaveStruct->commandId);
			/* STRUCTURE_SAVE_V20 includes OBJECT_SAVE_V20 */
			endian_sdword(&psSaveStruct->currentBuildPts);
			endian_udword(&psSaveStruct->body);
			endian_udword(&psSaveStruct->armour);
			endian_udword(&psSaveStruct->resistance);
			endian_udword(&psSaveStruct->dummy1);
			endian_udword(&psSaveStruct->subjectInc);
			endian_udword(&psSaveStruct->timeStarted);
			endian_udword(&psSaveStruct->output);
			endian_udword(&psSaveStruct->capacity);
			endian_udword(&psSaveStruct->quantity);
			endian_udword(&psSaveStruct->factoryInc);
			endian_udword(&psSaveStruct->powerAccrued);
			endian_udword(&psSaveStruct->dummy2);
			endian_udword(&psSaveStruct->droidTimeStarted);
			endian_udword(&psSaveStruct->timeToBuild);
			endian_udword(&psSaveStruct->timeStartHold);
			endian_sword(&psSaveStruct->currentPowerAccrued);
			/* OBJECT_SAVE_V20 */
			endian_udword(&psSaveStruct->id);
			endian_udword(&psSaveStruct->x);
			endian_udword(&psSaveStruct->y);
			endian_udword(&psSaveStruct->z);
			endian_udword(&psSaveStruct->direction);
			endian_udword(&psSaveStruct->player);
			endian_udword(&psSaveStruct->burnStart);
			endian_udword(&psSaveStruct->burnDamage);

			psSaveStruct = (SAVE_STRUCTURE *)((char *)psSaveStruct + sizeof(SAVE_STRUCTURE));
		}
	}

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		free(pFileData);
		return status;
	}
	return false;
}

// -----------------------------------------------------------------------------------------
BOOL loadStructSetPointers(void)
{
	UDWORD		player,list;
	FACTORY		*psFactory;
	REPAIR_FACILITY	*psRepair;
	REARM_PAD	*psReArmPad;
	STRUCTURE	*psStruct;
	DROID		*psCommander;
	STRUCTURE	**ppsStructLists[2];

	ppsStructLists[0] = apsStructLists;
	ppsStructLists[1] = mission.apsStructLists;

	for(list = 0; list<2; list++)
	{
		for(player = 0; player<MAX_PLAYERS; player++)
		{
			psStruct=(STRUCTURE *)ppsStructLists[list][player];
			while (psStruct)
			{
				if (psStruct->pFunctionality)
				{
					UDWORD _tmpid;
					switch (psStruct->pStructureType->type)
					{
					case REF_FACTORY:
					case REF_CYBORG_FACTORY:
					case REF_VTOL_FACTORY:
						psFactory = ((FACTORY *)psStruct->pFunctionality);
						FIXME_CAST_ASSIGN(UDWORD, _tmpid, psFactory->psCommander);
						//there is a commander then has been temporarily removed
						//so put it back
						if (_tmpid != NULL_ID)
						{
							psCommander = (DROID*)getBaseObjFromId(_tmpid);
							psFactory->psCommander = NULL;
							ASSERT( psCommander != NULL,"loadStructSetPointers psCommander getBaseObjFromId() failed" );
							if (psCommander == NULL)
							{
								psFactory->psCommander = NULL;
							}
							else
							{
                                if (list == 1) //ie offWorld
                                {
                                    //don't need to worry about the Flag
                                    ((FACTORY *)psStruct->pFunctionality)->psCommander =
                                        psCommander;
                                }
                                else
                                {
								    assignFactoryCommandDroid(psStruct, psCommander);
                                }
							}
						}
						else
						{
							psFactory->psCommander = NULL;
						}
						break;
					case REF_REPAIR_FACILITY:
						psRepair = ((REPAIR_FACILITY *)psStruct->pFunctionality);
						FIXME_CAST_ASSIGN(UDWORD, _tmpid, psRepair->psObj);
						if (_tmpid == NULL_ID)
						{
							psRepair->psObj = NULL;
						}
						else
						{
							psRepair->psObj = getBaseObjFromId(_tmpid);
							ASSERT(psRepair->psObj->type == OBJ_DROID, "%s cannot repair %s", 
							       objInfo((BASE_OBJECT *)psStruct), objInfo(psRepair->psObj));
							//if the build has started set the powerAccrued =
							//powerRequired to sync the interface
							if (psRepair->timeStarted != ACTION_START_TIME && psRepair->psObj
							    && psRepair->psObj->type == OBJ_DROID)
							{
								psRepair->powerAccrued = powerReqForDroidRepair((DROID*)psRepair->psObj);
							}
						}
						break;
					case REF_REARM_PAD:
						psReArmPad = ((REARM_PAD *)psStruct->pFunctionality);
						if (saveGameVersion >= VERSION_26)//version 26
						{
							FIXME_CAST_ASSIGN(UDWORD, _tmpid, psReArmPad->psObj);
							if (_tmpid == NULL_ID)
							{
								psReArmPad->psObj = NULL;
							}
							else
							{
								psReArmPad->psObj = getBaseObjFromId(_tmpid);
							}
						}
						else
						{
							psReArmPad->psObj = NULL;
						}
					default:
						break;
					}
				}
				psStruct = psStruct->psNext;
			}
		}
	}
	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveFeature(char *pFileData, UDWORD filesize)
{
	FEATURE_SAVEHEADER		*psHeader;

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

	debug(LOG_SAVE, "file version is %u ", psHeader->version);

	//increment to the start of the data
	pFileData += FEATURE_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "FeatLoad: unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveFeatureV14(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveFeatureV(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported save format version %u", psHeader->version);
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
/* code for all version 8 - 14 save features */
BOOL loadSaveFeatureV14(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version)
{
	SAVE_FEATURE_V14			*psSaveFeature;
	FEATURE					*pFeature;
	UDWORD					count, i, statInc;
	FEATURE_STATS			*psStats = NULL;
	BOOL					found;
	UDWORD					sizeOfSaveFeature;

	if (version < VERSION_14)
	{
		sizeOfSaveFeature = sizeof(SAVE_FEATURE_V2);
	}
	else
	{
		sizeOfSaveFeature = sizeof(SAVE_FEATURE_V14);
	}


	if ((sizeOfSaveFeature * numFeatures + FEATURE_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "featureLoad: unexpected end of file" );

		return false;
	}

	/* Load in the feature data */
	for (count = 0; count < numFeatures; count ++,
									pFileData += sizeOfSaveFeature)
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

		/*if (psSaveFeature->featureInc > numFeatureStats)
		{
			DBERROR(("Invalid Feature Type - unable to load save game"));
			goto error;
		}*/
		//get the stats for this feature
		found = false;


		if (!getSaveObjectName(psSaveFeature->name))
		{
			continue;
		}

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
		//restore values
		pFeature->id = psSaveFeature->id;
		pFeature->rot.direction = DEG(psSaveFeature->direction);
		pFeature->inFire = psSaveFeature->inFire;
		pFeature->burnDamage = psSaveFeature->burnDamage;
		if (version >= VERSION_14)
		{
			for (i=0; i < MAX_PLAYERS; i++)
			{
				pFeature->visible[i] = psSaveFeature->visible[i];
			}
		}

	}

	return true;
}

// -----------------------------------------------------------------------------------------
/* code for all post version 7 save features */
BOOL loadSaveFeatureV(char *pFileData, UDWORD filesize, UDWORD numFeatures, UDWORD version)
{
	SAVE_FEATURE			*psSaveFeature;
	FEATURE					*pFeature;
	UDWORD					count, i, statInc;
	FEATURE_STATS			*psStats = NULL;
	BOOL					found;
	UDWORD					sizeOfSaveFeature;

//	version;

	sizeOfSaveFeature = sizeof(SAVE_FEATURE);

	if ((sizeOfSaveFeature * numFeatures + FEATURE_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "featureLoad: unexpected end of file" );

		return false;
	}

	/* Load in the feature data */
	for (count = 0; count < numFeatures; count ++,
									pFileData += sizeOfSaveFeature)
	{
		psSaveFeature = (SAVE_FEATURE*) pFileData;

		/* FEATURE_SAVE is FEATURE_SAVE_V20 */
		/* FEATURE_SAVE_V20 is OBJECT_SAVE_V20 */
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveFeature->id);
		endian_udword(&psSaveFeature->x);
		endian_udword(&psSaveFeature->y);
		endian_udword(&psSaveFeature->z);
		endian_udword(&psSaveFeature->direction);
		endian_udword(&psSaveFeature->player);
		endian_udword(&psSaveFeature->burnStart);
		endian_udword(&psSaveFeature->burnDamage);

		/*if (psSaveFeature->featureInc > numFeatureStats)
		{
			DBERROR(("Invalid Feature Type - unable to load save game"));
			goto error;
		}*/
		//get the stats for this feature
		found = false;


		if (!getSaveObjectName(psSaveFeature->name))
		{
			continue;
		}

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
		//restore values
		pFeature->id = psSaveFeature->id;
		pFeature->rot.direction = DEG(psSaveFeature->direction);
		pFeature->inFire = psSaveFeature->inFire;
		pFeature->burnDamage = psSaveFeature->burnDamage;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			pFeature->visible[i] = psSaveFeature->visible[i]	;
		}
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of features to a file
*/
BOOL writeFeatureFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD				fileSize, i, totalFeatures=0;
	FEATURE				*psCurr;
	FEATURE_SAVEHEADER	*psHeader;
	SAVE_FEATURE		*psSaveFeature;
	BOOL status = true;

	//total all the features in the world
	for (psCurr = apsFeatureLists[0]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		totalFeatures++;
	}

	/* Allocate the data buffer */
	fileSize = FEATURE_HEADER_SIZE + totalFeatures * sizeof(SAVE_FEATURE);
	pFileData = (char*)malloc(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	/* Put the file header on the file */
	psHeader = (FEATURE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'f';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 'a';
	psHeader->aFileType[3] = 't';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalFeatures;

	psSaveFeature = (SAVE_FEATURE*)(pFileData + FEATURE_HEADER_SIZE);

	/* Put the feature data into the buffer */
	for(psCurr = apsFeatureLists[0]; psCurr != NULL; psCurr = psCurr->psNext)
	{

		strcpy(psSaveFeature->name, psCurr->psStats->pName);

		psSaveFeature->id = psCurr->id;

		psSaveFeature->x = psCurr->pos.x;
		psSaveFeature->y = psCurr->pos.y;
		psSaveFeature->z = psCurr->pos.z;

		psSaveFeature->direction = UNDEG(psCurr->rot.direction);
		psSaveFeature->inFire = psCurr->inFire;
		psSaveFeature->burnDamage = psCurr->burnDamage;
		for (i=0; i < MAX_PLAYERS; i++)
		{
			psSaveFeature->visible[i] = psCurr->visible[i];
		}

		/* SAVE_FEATURE is FEATURE_SAVE_V20 */
		/* FEATURE_SAVE_V20 includes OBJECT_SAVE_V20 */
		/* OBJECT_SAVE_V20 */
		endian_udword(&psSaveFeature->id);
		endian_udword(&psSaveFeature->x);
		endian_udword(&psSaveFeature->y);
		endian_udword(&psSaveFeature->z);
		endian_udword(&psSaveFeature->direction);
		endian_udword(&psSaveFeature->player);
		endian_udword(&psSaveFeature->burnStart);
		endian_udword(&psSaveFeature->burnDamage);

		psSaveFeature = (SAVE_FEATURE *)((char *)psSaveFeature + sizeof(SAVE_FEATURE));
	}

	/* FEATURE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		free(pFileData);
		return status;
	}
	return false;
}


// -----------------------------------------------------------------------------------------
BOOL loadSaveTemplate(char *pFileData, UDWORD filesize)
{
	TEMPLATE_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (TEMPLATE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 't' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 'm' || psHeader->aFileType[3] != 'p')
	{
		debug( LOG_ERROR, "loadSaveTemplate: Incorrect file type" );

		return false;
	}

	/* TEMPLATE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "fileversion is %u ", psHeader->version);

	//increment to the start of the data
	pFileData += TEMPLATE_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "TemplateLoad: unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version < VERSION_14)
	{
		if (!loadSaveTemplateV7(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveTemplateV14(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveTemplateV(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported template save format version %u", psHeader->version);
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/* code specific to version 7 of a save template */
BOOL loadSaveTemplateV7(char *pFileData, UDWORD filesize, UDWORD numTemplates)
{
	SAVE_TEMPLATE_V2		*psSaveTemplate, sSaveTemplate;
	DROID_TEMPLATE			*psTemplate;
	UDWORD					count, i;
	SDWORD					compInc;
	BOOL					found;

	psSaveTemplate = &sSaveTemplate;

	if ((sizeof(SAVE_TEMPLATE_V2) * numTemplates + TEMPLATE_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "templateLoad: unexpected end of file" );

		return false;
	}

	/* Load in the template data */
	for (count = 0; count < numTemplates; count ++, pFileData += sizeof(SAVE_TEMPLATE_V2))
	{
		memcpy(psSaveTemplate, pFileData, sizeof(SAVE_TEMPLATE_V2));

		/* SAVE_TEMPLATE_V2 is TEMPLATE_SAVE_V2 */
		/* TEMPLATE_SAVE_V2 */
		endian_udword(&psSaveTemplate->ref);
		endian_udword(&psSaveTemplate->player);
		endian_udword(&psSaveTemplate->numWeaps);
		endian_udword(&psSaveTemplate->numProgs);

		if (psSaveTemplate->player != 0)
		{
			// only load player 0 templates - the rest come from stats
			continue;
		}

		//create the Template
		psTemplate = malloc(sizeof(DROID_TEMPLATE));
		if (psTemplate == NULL)
		{
			debug(LOG_FATAL, "loadSaveTemplateV7: Out of memory");
			abort();
			goto error;
		}
		//copy the values across

		psTemplate->pName = NULL;
		sstrcpy(psTemplate->aName, psSaveTemplate->name);

		psTemplate->ref = psSaveTemplate->ref;
		psTemplate->droidType = psSaveTemplate->droidType;
		found = true;
		//for (i=0; i < DROID_MAXCOMP; i++) - not intestested in the first comp - COMP_UNKNOWN
		for (i=1; i < DROID_MAXCOMP; i++)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (i == SAVE_COMP_PROGRAM)
			{
				break;
			}

			compInc = getCompFromName(i, psSaveTemplate->asParts[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This component no longer exists - %s, the template will be deleted", psSaveTemplate->asParts[i] );

				found = false;
				//continue;
				break;
			}
			psTemplate->asParts[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			free(psTemplate);
			continue;
		}
		psTemplate->numWeaps = psSaveTemplate->numWeaps;
		found = true;
		for (i=0; i < psTemplate->numWeaps; i++)
		{

			compInc = getCompFromName(COMP_WEAPON, psSaveTemplate->asWeaps[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This weapon no longer exists - %s, the template will be deleted", psSaveTemplate->asWeaps[i] );

				found = false;
				//continue;
				break;
			}
			psTemplate->asWeaps[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			free(psTemplate);
			continue;
		}

		// ignore brains and programs for now
		psTemplate->asParts[COMP_BRAIN] = 0;
		psTemplate->prefab = false;		// not AI template

		//calculate the total build points
		psTemplate->buildPoints = calcTemplateBuild(psTemplate);
		psTemplate->powerPoints = calcTemplatePower(psTemplate);

		//store it in the apropriate player' list
		psTemplate->psNext = apsDroidTemplates[psSaveTemplate->player];
		apsDroidTemplates[psSaveTemplate->player] = psTemplate;
	}

	return true;

error:
	droidTemplateShutDown();
	return false;
}

// -----------------------------------------------------------------------------------------
/* none specific version of a save template */
BOOL loadSaveTemplateV14(char *pFileData, UDWORD filesize, UDWORD numTemplates)
{
	SAVE_TEMPLATE_V14			*psSaveTemplate, sSaveTemplate;
	DROID_TEMPLATE			*psTemplate, *psDestTemplate;
	UDWORD					count, i;
	SDWORD					compInc;
	BOOL					found;

	psSaveTemplate = &sSaveTemplate;

	if ((sizeof(SAVE_TEMPLATE_V14) * numTemplates + TEMPLATE_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "templateLoad: unexpected end of file" );

		return false;
	}

	/* Load in the template data */
	for (count = 0; count < numTemplates; count ++, pFileData += sizeof(SAVE_TEMPLATE_V14))
	{
		memcpy(psSaveTemplate, pFileData, sizeof(SAVE_TEMPLATE_V14));

		/* SAVE_TEMPLATE_V14 is TEMPLATE_SAVE_V14 */
		endian_udword(&psSaveTemplate->ref);
		endian_udword(&psSaveTemplate->player);
		endian_udword(&psSaveTemplate->numWeaps);
		endian_udword(&psSaveTemplate->multiPlayerID);

		//AT SOME POINT CHECK THE multiPlayerID TO SEE IF ALREADY EXISTS - IGNORE IF IT DOES

		if (psSaveTemplate->player != 0)
		{
			// only load player 0 templates - the rest come from stats
			continue;
		}

		//create the Template
		psTemplate = malloc(sizeof(DROID_TEMPLATE));
		if (psTemplate == NULL)
		{
			debug(LOG_ERROR, "loadSaveTemplateV14: Out of memory");

			goto error;
		}
		//copy the values across

		psTemplate->pName = NULL;
		sstrcpy(psTemplate->aName, psSaveTemplate->name);

		psTemplate->ref = psSaveTemplate->ref;
		psTemplate->droidType = psSaveTemplate->droidType;
		psTemplate->multiPlayerID = psSaveTemplate->multiPlayerID;
		found = true;
		//for (i=0; i < DROID_MAXCOMP; i++) - not intestested in the first comp - COMP_UNKNOWN
		for (i=1; i < DROID_MAXCOMP; i++)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (i == SAVE_COMP_PROGRAM)
			{
				break;
			}

			compInc = getCompFromName(i, psSaveTemplate->asParts[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This component no longer exists - %s, the template will be deleted", psSaveTemplate->asParts[i] );

				found = false;
				//continue;
				break;
			}
			psTemplate->asParts[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			free(psTemplate);
			continue;
		}
		psTemplate->numWeaps = psSaveTemplate->numWeaps;
		found = true;
		for (i=0; i < psTemplate->numWeaps; i++)
		{

			compInc = getCompFromName(COMP_WEAPON, psSaveTemplate->asWeaps[i]);

			if (compInc < 0)
			{
				debug( LOG_ERROR, "This weapon no longer exists - %s, the template will be deleted", psSaveTemplate->asWeaps[i] );

				found = false;
				//continue;
				break;
			}
			psTemplate->asWeaps[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			free(psTemplate);
			continue;
		}

		// ignore brains and programs for now
		psTemplate->asParts[COMP_BRAIN] = 0;
		psTemplate->prefab = false;		// not AI template

		//calculate the total build points
		psTemplate->buildPoints = calcTemplateBuild(psTemplate);
		psTemplate->powerPoints = calcTemplatePower(psTemplate);

		//store it in the apropriate player' list
		//if a template with the same multiplayerID exists overwrite it
		//else add this template to the top of the list
		psDestTemplate = apsDroidTemplates[psSaveTemplate->player];
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
			memcpy(psDestTemplate,psTemplate,sizeof(DROID_TEMPLATE));
		}
		else
		{
			//add it to the top of the list
			psTemplate->psNext = apsDroidTemplates[psSaveTemplate->player];
			apsDroidTemplates[psSaveTemplate->player] = psTemplate;
		}




	}

	return true;

error:
	droidTemplateShutDown();
	return false;
}

// -----------------------------------------------------------------------------------------
/* none specific version of a save template */
BOOL loadSaveTemplateV(char *pFileData, UDWORD filesize, UDWORD numTemplates)
{
	SAVE_TEMPLATE			*psSaveTemplate, sSaveTemplate;
	DROID_TEMPLATE			*psTemplate, *psDestTemplate;
	UDWORD					count, i;
	SDWORD					compInc;
	BOOL					found;

	psSaveTemplate = &sSaveTemplate;

	if ((sizeof(SAVE_TEMPLATE) * numTemplates + TEMPLATE_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "templateLoad: unexpected end of file" );

		return false;
	}

	/* Load in the template data */
	for (count = 0; count < numTemplates; count ++, pFileData += sizeof(SAVE_TEMPLATE))
	{
		memcpy(psSaveTemplate, pFileData, sizeof(SAVE_TEMPLATE));

		/* SAVE_TEMPLATE is TEMPLATE_SAVE_V20 */
		/* TEMPLATE_SAVE_V20 */
		endian_udword(&psSaveTemplate->ref);
		endian_udword(&psSaveTemplate->player);
		endian_udword(&psSaveTemplate->numWeaps);
		endian_udword(&psSaveTemplate->multiPlayerID);

		//AT SOME POINT CHECK THE multiPlayerID TO SEE IF ALREADY EXISTS - IGNORE IF IT DOES

		if (psSaveTemplate->player != 0 && !bMultiPlayer)
		{
			// only load player 0 templates - the rest come from stats
			continue;
		}

		//create the Template
		psTemplate = malloc(sizeof(DROID_TEMPLATE));
		if (psTemplate == NULL)
		{
			debug(LOG_FATAL, "loadSaveTemplateV: Out of memory");
			abort();
			goto error;
		}
		//copy the values across

		psTemplate->pName = NULL;
		sstrcpy(psTemplate->aName, psSaveTemplate->name);

		psTemplate->ref = psSaveTemplate->ref;
		psTemplate->droidType = psSaveTemplate->droidType;
		psTemplate->multiPlayerID = psSaveTemplate->multiPlayerID;
		found = true;
		//for (i=0; i < DROID_MAXCOMP; i++) - not intestested in the first comp - COMP_UNKNOWN
		for (i=1; i < DROID_MAXCOMP; i++)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (i == SAVE_COMP_PROGRAM)
			{
				break;
			}

			compInc = getCompFromName(i, psSaveTemplate->asParts[i]);

            //HACK to get the game to load when ECMs, Sensors or RepairUnits have been deleted
            if (compInc < 0 && (i == COMP_ECM || i == COMP_SENSOR || i == COMP_REPAIRUNIT))
            {
                //set the ECM to be the defaultECM ...
                if (i == COMP_ECM)
                {
                    compInc = aDefaultECM[psSaveTemplate->player];
                }
                else if (i == COMP_SENSOR)
                {
                    compInc = aDefaultSensor[psSaveTemplate->player];
                }
                else if (i == COMP_REPAIRUNIT)
                {
                    compInc = aDefaultRepair[psSaveTemplate->player];
                }
            }
			else if (compInc < 0)
			{

				debug( LOG_ERROR, "This component no longer exists - %s, the template will be deleted", psSaveTemplate->asParts[i] );

				found = false;
				//continue;
				break;
			}
			psTemplate->asParts[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			free(psTemplate);
			continue;
		}
		psTemplate->numWeaps = psSaveTemplate->numWeaps;
		found = true;
		for (i=0; i < psTemplate->numWeaps; i++)
		{

			compInc = getCompFromName(COMP_WEAPON, psSaveTemplate->asWeaps[i]);

			if (compInc < 0)
			{

				debug( LOG_ERROR, "This weapon no longer exists - %s, the template will be deleted", psSaveTemplate->asWeaps[i] );

				found = false;
				//continue;
				break;
			}
			psTemplate->asWeaps[i] = (UDWORD)compInc;
		}
		if (!found)
		{
			//ignore this record
			free(psTemplate);
			continue;
		}

		//no! put brains back in 10Feb //ignore brains and programs for now
		//psTemplate->asParts[COMP_BRAIN] = 0;
		psTemplate->prefab = false;		// not AI template

		//calculate the total build points
		psTemplate->buildPoints = calcTemplateBuild(psTemplate);
		psTemplate->powerPoints = calcTemplatePower(psTemplate);

		//store it in the apropriate player' list
		//if a template with the same multiplayerID exists overwrite it
		//else add this template to the top of the list
		psDestTemplate = apsDroidTemplates[psSaveTemplate->player];
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
			memcpy(psDestTemplate,psTemplate,sizeof(DROID_TEMPLATE));
		}
		else
		{
			//add it to the top of the list
			psTemplate->psNext = apsDroidTemplates[psSaveTemplate->player];
			apsDroidTemplates[psSaveTemplate->player] = psTemplate;
		}
	}

	return true;

error:
	droidTemplateShutDown();
	return false;
}

// -----------------------------------------------------------------------------------------
/*
Writes the linked list of templates for each player to a file
*/
BOOL writeTemplateFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD				fileSize, player, totalTemplates=0;
	DROID_TEMPLATE		*psCurr;
	TEMPLATE_SAVEHEADER	*psHeader;
	SAVE_TEMPLATE		*psSaveTemplate;
	UDWORD				i;
	BOOL status = true;

	//total all the droids in the world
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			totalTemplates++;
		}
	}

	/* Allocate the data buffer */
	fileSize = TEMPLATE_HEADER_SIZE + totalTemplates*sizeof(SAVE_TEMPLATE);
	pFileData = (char*)malloc(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	/* Put the file header on the file */
	psHeader = (TEMPLATE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 't';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 'm';
	psHeader->aFileType[3] = 'p';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalTemplates;

	psSaveTemplate = (SAVE_TEMPLATE*)(pFileData + TEMPLATE_HEADER_SIZE);

	/* Put the template data into the buffer */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			sstrcpy(psSaveTemplate->name, psCurr->aName);

			psSaveTemplate->ref = psCurr->ref;
			psSaveTemplate->player = player;
			psSaveTemplate->droidType = (UBYTE)psCurr->droidType;
			psSaveTemplate->multiPlayerID = psCurr->multiPlayerID;

			// not interested in first comp - COMP_UNKNOWN
			for (i=1; i < DROID_MAXCOMP; i++)
			{

				if (!getNameFromComp(i, psSaveTemplate->asParts[i], psCurr->asParts[i]))

				{
					//ignore this record
					break;
					//continue;
				}
			}
			psSaveTemplate->numWeaps = psCurr->numWeaps;
			for (i=0; i < psCurr->numWeaps; i++)
			{

				if (!getNameFromComp(COMP_WEAPON, psSaveTemplate->asWeaps[i], psCurr->asWeaps[i]))

				{
					//ignore this record
					//continue;
					break;
				}
			}

			/* SAVE_TEMPLATE is TEMPLATE_SAVE_V20 */
			/* TEMPLATE_SAVE_V20 */
			endian_udword(&psSaveTemplate->ref);
			endian_udword(&psSaveTemplate->player);
			endian_udword(&psSaveTemplate->numWeaps);
			endian_udword(&psSaveTemplate->multiPlayerID);

			psSaveTemplate = (SAVE_TEMPLATE *)((char *)psSaveTemplate + sizeof(SAVE_TEMPLATE));
		}
	}

	/* TEMPLATE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	/* Write the data to the file */
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		free(pFileData);
		return status;
	}
	return false;
}

// -----------------------------------------------------------------------------------------
// load up a terrain tile type map file
BOOL loadTerrainTypeMap(const char *pFileData, UDWORD filesize)
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
static BOOL writeTerrainTypeMapFile(char *pFileName)
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
// load up component list file
BOOL loadSaveCompList(char *pFileData, UDWORD filesize)
{
	COMPLIST_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (COMPLIST_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'c' || psHeader->aFileType[1] != 'm' ||
		psHeader->aFileType[2] != 'p' || psHeader->aFileType[3] != 'l')
	{
		debug( LOG_ERROR, "loadSaveCompList: Incorrect file type" );

		return false;
	}

	/* COMPLIST_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "file version is %u ", psHeader->version);

	//increment to the start of the data
	pFileData += COMPLIST_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "CompLoad: unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveCompListV9(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveCompListV(pFileData, filesize, psHeader->quantity, psHeader->version))
		{
			return false;
		}
	}
	else
	{
		debug( LOG_ERROR, "Unsupported component save format version %u", psHeader->version);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveCompListV9(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version)
{
	SAVE_COMPLIST_V6		*psSaveCompList;
	UDWORD				i;
	SDWORD				compInc;

	if ((sizeof(SAVE_COMPLIST_V6) * numRecords + COMPLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "CompListLoad: unexpected end of file" );

		return false;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_COMPLIST_V6))
	{
		psSaveCompList = (SAVE_COMPLIST_V6 *) pFileData;

		if (version < VERSION_9)
		{
			//DROID_MAXCOMP has changed to remove COMP_PROGRAM so hack here to load old save games!
			if (psSaveCompList->type == SAVE_COMP_PROGRAM)
			{
				//ignore this record
				continue;
			}
			if (psSaveCompList->type == SAVE_COMP_WEAPON)
			{
				//this typeNum has to be reset for lack of COMP_PROGRAM
				psSaveCompList->type = COMP_WEAPON;
			}
		}

		if (psSaveCompList->type > COMP_NUMCOMPONENTS)
		{
			//ignore this record
			continue;
		}

		compInc = getCompFromName(psSaveCompList->type, psSaveCompList->name);

		if (compInc < 0)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->state != UNAVAILABLE && psSaveCompList->state !=
			AVAILABLE && psSaveCompList->state != FOUND && psSaveCompList->state != REDUNDANT)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apCompLists[psSaveCompList->player][psSaveCompList->type][compInc] =
			psSaveCompList->state;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveCompListV(char *pFileData, UDWORD filesize, UDWORD numRecords, UDWORD version)
{
	SAVE_COMPLIST		*psSaveCompList;
	UDWORD				i;
	SDWORD				compInc;

	if ((sizeof(SAVE_COMPLIST) * numRecords + COMPLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "CompListLoad: unexpected end of file" );

		return false;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_COMPLIST))
	{
		psSaveCompList = (SAVE_COMPLIST *) pFileData;

		if (psSaveCompList->type > COMP_NUMCOMPONENTS)
		{
			//ignore this record
			continue;
		}

		compInc = getCompFromName(psSaveCompList->type, psSaveCompList->name);

		if (compInc < 0)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->state != UNAVAILABLE && psSaveCompList->state !=
			AVAILABLE && psSaveCompList->state != FOUND && psSaveCompList->state != REDUNDANT)
		{
			//ignore this record
			continue;
		}
		if (psSaveCompList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apCompLists[psSaveCompList->player][psSaveCompList->type][compInc] =
			psSaveCompList->state;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Comp lists per player
static BOOL writeCompListFile(char *pFileName)
{
	COMPLIST_SAVEHEADER		*psHeader;
	char *pFileData;
	SAVE_COMPLIST			*psSaveCompList;
	UDWORD					fileSize, totalComp, player, i;
	COMPONENT_STATS			*psStats;

	// Calculate the file size
	totalComp = (numBodyStats + numWeaponStats + numConstructStats + numECMStats +
		numPropulsionStats + numSensorStats + numRepairStats + numBrainStats) * MAX_PLAYERS;
	fileSize = COMPLIST_HEADER_SIZE + (sizeof(SAVE_COMPLIST) * totalComp);
	//allocate the buffer space
	pFileData = (char*)malloc(fileSize);
	if (!pFileData)
	{
		debug( LOG_FATAL, "writeCompListFile: Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (COMPLIST_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'c';
	psHeader->aFileType[1] = 'm';
	psHeader->aFileType[2] = 'p';
	psHeader->aFileType[3] = 'l';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalComp;

	psSaveCompList = (SAVE_COMPLIST *) (pFileData + COMPLIST_HEADER_SIZE);

	//save each type of comp
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(i = 0; i < numBodyStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asBodyStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_BODY;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_BODY][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numWeaponStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asWeaponStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_WEAPON;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_WEAPON][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numConstructStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asConstructStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_CONSTRUCT;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_CONSTRUCT][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numECMStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asECMStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_ECM;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_ECM][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numPropulsionStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asPropulsionStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_PROPULSION;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_PROPULSION][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numSensorStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asSensorStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_SENSOR;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_SENSOR][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numRepairStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asRepairStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_REPAIRUNIT;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_REPAIRUNIT][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
		for(i = 0; i < numBrainStats; i++)
		{
			psStats = (COMPONENT_STATS *)(asBrainStats + i);

			strcpy(psSaveCompList->name, psStats->pName);

			psSaveCompList->type = COMP_BRAIN;
			psSaveCompList->player = (UBYTE)player;
			psSaveCompList->state = apCompLists[player][COMP_BRAIN][i];
			psSaveCompList = (SAVE_COMPLIST *)((char *)psSaveCompList + sizeof(SAVE_COMPLIST));
		}
	}

	/* COMPLIST_SAVEHEADER */
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
// load up structure type list file
BOOL loadSaveStructTypeList(char *pFileData, UDWORD filesize)
{
	STRUCTLIST_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (STRUCTLIST_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
		psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'l')
	{
		debug( LOG_ERROR, "loadSaveStructTypeList: Incorrect file type" );

		return false;
	}

	/* STRUCTLIST_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "file version is %u ", psHeader->version);

	//increment to the start of the data
	pFileData += STRUCTLIST_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_7)
	{
		debug( LOG_ERROR, "StructTypeLoad: unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveStructTypeListV7(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveStructTypeListV(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported struct type save format version %u", psHeader->version);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveStructTypeListV7(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_STRUCTLIST_V6		*psSaveStructList;
	UDWORD				i, statInc;
	STRUCTURE_STATS		*psStats;
	BOOL				found;

	if ((sizeof(SAVE_STRUCTLIST_V6) * numRecords + STRUCTLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "StructListLoad: unexpected end of file" );

		return false;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_STRUCTLIST_V6))
	{
		psSaveStructList = (SAVE_STRUCTLIST_V6 *) pFileData;

		found = false;

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructList->name))

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
		if (psSaveStructList->state != UNAVAILABLE && psSaveStructList->state !=
			AVAILABLE && psSaveStructList->state != FOUND && psSaveStructList->state != REDUNDANT)
		{
			//ignore this record
			continue;
		}
		if (psSaveStructList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apStructTypeLists[psSaveStructList->player][statInc] =
			psSaveStructList->state;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveStructTypeListV(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_STRUCTLIST		*psSaveStructList;
	UDWORD				i, statInc;
	STRUCTURE_STATS		*psStats;
	BOOL				found;

	if ((sizeof(SAVE_STRUCTLIST) * numRecords + STRUCTLIST_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "StructListLoad: unexpected end of file" );

		return false;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_STRUCTLIST))
	{
		psSaveStructList = (SAVE_STRUCTLIST *) pFileData;

		found = false;

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveStructList->name))

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
		if (psSaveStructList->state != UNAVAILABLE && psSaveStructList->state !=
			AVAILABLE && psSaveStructList->state != FOUND && psSaveStructList->state != REDUNDANT)
		{
			//ignore this record
			continue;
		}
		if (psSaveStructList->player > MAX_PLAYERS)
		{
			//ignore this record
			continue;
		}
		//date is valid so set the state
		apStructTypeLists[psSaveStructList->player][statInc] =
			psSaveStructList->state;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Struct Type List per player
static BOOL writeStructTypeListFile(char *pFileName)
{
	STRUCTLIST_SAVEHEADER	*psHeader;
	SAVE_STRUCTLIST			*psSaveStructList;
	char *pFileData;
	UDWORD					fileSize, player, i;
	STRUCTURE_STATS			*psStats;

	// Calculate the file size
	fileSize = STRUCTLIST_HEADER_SIZE + (sizeof(SAVE_STRUCTLIST) *
		numStructureStats * MAX_PLAYERS);

	//allocate the buffer space
	pFileData = (char*)malloc(fileSize);
	if (!pFileData)
	{
		debug( LOG_FATAL, "writeStructTypeListFile: Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (STRUCTLIST_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 's';
	psHeader->aFileType[1] = 't';
	psHeader->aFileType[2] = 'r';
	psHeader->aFileType[3] = 'l';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numStructureStats * MAX_PLAYERS;

	psSaveStructList = (SAVE_STRUCTLIST *) (pFileData + STRUCTLIST_HEADER_SIZE);
	//save each type of struct type
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psStats = asStructureStats;
		for(i = 0; i < numStructureStats; i++, psStats++)
		{

			strcpy(psSaveStructList->name, psStats->pName);
			psSaveStructList->state = apStructTypeLists[player][i];
			psSaveStructList->player = (UBYTE)player;
			psSaveStructList = (SAVE_STRUCTLIST *)((char *)psSaveStructList +
				sizeof(SAVE_STRUCTLIST));
		}
	}

	/* STRUCT_SAVEHEADER */
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
// load up saved research file
BOOL loadSaveResearch(char *pFileData, UDWORD filesize)
{
	RESEARCH_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (RESEARCH_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'r' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 's' || psHeader->aFileType[3] != 'h')
	{
		debug( LOG_ERROR, "loadSaveResearch: Incorrect file type" );

		return false;
	}

	/* RESEARCH_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "file version is %u ", psHeader->version);

	//increment to the start of the data
	pFileData += RESEARCH_HEADER_SIZE;

	/* Check the file version */
	if (psHeader->version < VERSION_8)
	{
		debug( LOG_ERROR, "ResearchLoad: unsupported save format version %d", psHeader->version );

		return false;
	}
	else if (psHeader->version <= VERSION_19)
	{
		if (!loadSaveResearchV8(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveResearchV(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else
	{
		debug( LOG_ERROR, "Unsupported research save format version %u", psHeader->version);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveResearchV8(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_RESEARCH_V8		*psSaveResearch;
	UDWORD				i, statInc;
	RESEARCH			*psStats;
	BOOL				found;
	UBYTE				playerInc;

	if ((sizeof(SAVE_RESEARCH_V8) * numRecords + RESEARCH_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveResearch: unexpected end of file" );

		return false;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_RESEARCH_V8))
	{
		psSaveResearch = (SAVE_RESEARCH_V8 *) pFileData;

		/* SAVE_RESEARCH_V8 is RESEARCH_SAVE_V8 */
		/* RESEARCH_SAVE_V8 */
		for(playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
			endian_udword(&psSaveResearch->currentPoints[playerInc]);

		found = false;

		for (statInc = 0; statInc < numResearch; statInc++)
		{
			psStats = asResearch + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveResearch->name))

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


		for (playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
		{
			PLAYER_RESEARCH *psPlRes;

			psPlRes=&asPlayerResList[playerInc][statInc];

			// Copy the research status
			psPlRes->ResearchStatus=	(UBYTE)(psSaveResearch->researched[playerInc] & RESBITS);

			if (psSaveResearch->possible[playerInc]!=0)
				MakeResearchPossible(psPlRes);

			psPlRes->currentPoints = psSaveResearch->currentPoints[playerInc];

			//for any research that has been completed - perform so that upgrade values are set up
			if (psSaveResearch->researched[playerInc] == RESEARCHED)
			{
				researchResult(statInc, playerInc, false, NULL, false);
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveResearchV(char *pFileData, UDWORD filesize, UDWORD numRecords)
{
	SAVE_RESEARCH		*psSaveResearch;
	UDWORD				i, statInc;
	RESEARCH			*psStats;
	BOOL				found;
	UBYTE				playerInc;

	if ((sizeof(SAVE_RESEARCH) * numRecords + RESEARCH_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveResearch: unexpected end of file" );

		return false;
	}

	// Load the data
	for (i = 0; i < numRecords; i++, pFileData += sizeof(SAVE_RESEARCH))
	{
		psSaveResearch = (SAVE_RESEARCH *) pFileData;

		/* SAVE_RESEARCH is RESEARCH_SAVE_V20 */
		/* RESEARCH_SAVE_V20 */
		for(playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
			endian_udword(&psSaveResearch->currentPoints[playerInc]);

		found = false;

		for (statInc = 0; statInc < numResearch; statInc++)
		{
			psStats = asResearch + statInc;
			//loop until find the same name

			if (!strcmp(psStats->pName, psSaveResearch->name))

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


		for (playerInc = 0; playerInc < MAX_PLAYERS; playerInc++)
		{


			PLAYER_RESEARCH *psPlRes;

			psPlRes=&asPlayerResList[playerInc][statInc];

			// Copy the research status
			psPlRes->ResearchStatus=	(UBYTE)(psSaveResearch->researched[playerInc] & RESBITS);

			if (psSaveResearch->possible[playerInc]!=0)
				MakeResearchPossible(psPlRes);



			psPlRes->currentPoints = psSaveResearch->currentPoints[playerInc];

			//for any research that has been completed - perform so that upgrade values are set up
			if (psSaveResearch->researched[playerInc] == RESEARCHED)
			{
				researchResult(statInc, playerInc, false, NULL, false);
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Research per player
static BOOL writeResearchFile(char *pFileName)
{
	RESEARCH_SAVEHEADER		*psHeader;
	SAVE_RESEARCH			*psSaveResearch;
	char *pFileData;
	UDWORD					fileSize, player, i;
	RESEARCH				*psStats;

	// Calculate the file size
	fileSize = RESEARCH_HEADER_SIZE + (sizeof(SAVE_RESEARCH) *
		numResearch);

	//allocate the buffer space
	pFileData = (char*)malloc(fileSize);
	if (!pFileData)
	{
		debug( LOG_FATAL, "writeResearchFile: Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (RESEARCH_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'r';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 's';
	psHeader->aFileType[3] = 'h';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numResearch;

	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	psSaveResearch = (SAVE_RESEARCH *) (pFileData + RESEARCH_HEADER_SIZE);
	//save each type of reesearch
	psStats = asResearch;
	for(i = 0; i < numResearch; i++, psStats++)
	{

		strcpy(psSaveResearch->name, psStats->pName);

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			psSaveResearch->possible[player] = IsResearchPossible(&asPlayerResList[player][i]);
			psSaveResearch->researched[player] = (UBYTE)(asPlayerResList[player][i].ResearchStatus&RESBITS);
			psSaveResearch->currentPoints[player] = asPlayerResList[player][i].currentPoints;
		}

		for(player = 0; player < MAX_PLAYERS; player++)
			endian_udword(&psSaveResearch->currentPoints[player]);

		psSaveResearch = (SAVE_RESEARCH *)((char *)psSaveResearch +
			sizeof(SAVE_RESEARCH));
	}

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return false;
	}
	free(pFileData);

	return true;
}


// -----------------------------------------------------------------------------------------
// load up saved message file
BOOL loadSaveMessage(char *pFileData, UDWORD filesize, SWORD levelType)
{
	MESSAGE_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (MESSAGE_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'm' || psHeader->aFileType[1] != 'e' ||
		psHeader->aFileType[2] != 's' || psHeader->aFileType[3] != 's')
	{
		debug( LOG_ERROR, "loadSaveMessage: Incorrect file type" );

		return false;
	}

	/* MESSAGE_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += MESSAGE_HEADER_SIZE;

	/* Check the file version */
	if(psHeader->version <= VERSION_35)
	{
		debug(LOG_ERROR, "Unsupported message save format %d", (int)psHeader->version);
		return false;
	}
	else
	{
		if (!loadSaveMessage36(pFileData, filesize, psHeader->quantity, psHeader->version, levelType))
		{
			return false;
		}
	}
	return true;
}

BOOL loadSaveMessage36(char *pFileData, UDWORD filesize, UDWORD numMessages, UDWORD version, SWORD levelType)
{
	SAVE_MESSAGE_36	*psSaveMessage;
	MESSAGE			*psMessage;
	UDWORD			i, height;

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

	//check file
	if ((sizeof(SAVE_MESSAGE_36) * numMessages + MESSAGE_HEADER_SIZE) > filesize)
	{
		debug(LOG_ERROR, "loadSaveMessage: unexpected end of file");
		return false;
	}

	// Load the data
	for (i = 0; i < numMessages; i++, pFileData += sizeof(SAVE_MESSAGE_36))
	{
		psSaveMessage = (SAVE_MESSAGE_36 *) pFileData;

		/* SAVE_MESSAGE_36 */
		endian_sdword((SDWORD*)&psSaveMessage->type);	/* FIXME: enum may not be this type! */
		endian_sdword((SDWORD*)&psSaveMessage->dataType);
		endian_udword(&psSaveMessage->objId);
		endian_udword(&psSaveMessage->player);

		if (psSaveMessage->type == MSG_PROXIMITY)
		{
			//only load proximity if a mid-mission save game
			if (gameType == GTYPE_SAVE_MIDMISSION)
			{
				if (psSaveMessage->bObj)
				{
					// Proximity object so create get the obj from saved idy
					psMessage = addMessage(psSaveMessage->type, true, psSaveMessage->player);
					if (psMessage)
					{
						psMessage->pViewData = (MSG_VIEWDATA *)getBaseObjFromId(psSaveMessage->objId);
					}
					else
					{
						debug(LOG_ERROR, "Proximity object could not be created (type=%d, player=%d)",
					              (int)psSaveMessage->type, (int)psSaveMessage->player);
					}
				}
				else
				{
					VIEWDATA	*psViewData = NULL;

					// Proximity position so get viewdata pointer from the name
					psMessage = addMessage(psSaveMessage->type, false, psSaveMessage->player);

					if (psMessage)
					{
						if (psSaveMessage->dataType == MSG_DATA_BEACON)
						{
							UDWORD locX, locY;
							SDWORD sender;

							endian_udword(&psSaveMessage->locX);
							locX = psSaveMessage->locX;
							endian_udword(&psSaveMessage->locY);
							locY = psSaveMessage->locY;
							endian_sdword(&psSaveMessage->sender);
							sender = psSaveMessage->sender;

							psViewData = CreateBeaconViewData(sender, locX, locY);
							if (psViewData == NULL)
							{
								// Skip this message
								debug(LOG_ERROR, "Failed to create view data for beacon - skipping");
								removeMessage(psMessage, psSaveMessage->player);
								continue;
							}
						}
						else if (psSaveMessage->name[0] != '\0')
						{
							psViewData = (VIEWDATA *)getViewData(psSaveMessage->name);
							if (psViewData == NULL)
							{
								// Skip this message
								debug(LOG_ERROR, "Failed to find view data for proximity position - skipping");
								removeMessage(psMessage, psSaveMessage->player);
								continue;
							}
						}
						else
						{
							debug(LOG_ERROR, "Proximity position with empty name skipped");
							removeMessage(psMessage, psSaveMessage->player);
							continue;
						}

						psMessage->pViewData = (MSG_VIEWDATA *)psViewData;
						// Check the z value is at least the height of the terrain
						height = map_Height(((VIEW_PROXIMITY *)psViewData->pData)->x, ((VIEW_PROXIMITY *)psViewData->pData)->y);
						if (((VIEW_PROXIMITY *)psViewData->pData)->z < height)
						{
							((VIEW_PROXIMITY *)psViewData->pData)->z = height;
						}
					}
					else
					{
						debug(LOG_ERROR, "Proximity position could not be created (type=%d, player=%d)",
					              (int)psSaveMessage->type, (int)psSaveMessage->player);
					}
				}
			}
		}
		else
		{
			// Only load Campaign/Mission if a mid-mission save game
			if (psSaveMessage->type == MSG_CAMPAIGN || psSaveMessage->type == MSG_MISSION)
			{
				if (gameType == GTYPE_SAVE_MIDMISSION)
				{
					// Research message // Campaign message // Mission Report messages
					psMessage = addMessage(psSaveMessage->type, false, psSaveMessage->player);
					if (psMessage)
					{
						psMessage->pViewData = (MSG_VIEWDATA *)getViewData(psSaveMessage->name);
					}
				}
			}
			else
			{
				// Research message
				psMessage = addMessage(psSaveMessage->type, false, psSaveMessage->player);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)getViewData(psSaveMessage->name);
				}
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current messages per player
static BOOL writeMessageFile(char *pFileName)
{
	MESSAGE_SAVEHEADER		*psHeader;
	SAVE_MESSAGE_36			*psSaveMessage;
	char *pFileData;
	UDWORD					fileSize, player;
	MESSAGE					*psMessage;
	PROXIMITY_DISPLAY		*psProx;
	BASE_OBJECT				*psObj;
	UDWORD					numMessages = 0;
	VIEWDATA				*pViewData;

	// Calculate the file size
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psMessage = apsMessages[player]; psMessage != NULL;psMessage = psMessage->psNext)
		{
			numMessages++;
		}

	}

	fileSize = MESSAGE_HEADER_SIZE + (sizeof(SAVE_MESSAGE_36) *
		numMessages);


	//allocate the buffer space
	pFileData = (char*)malloc(fileSize);
	if (!pFileData)
	{
		debug( LOG_FATAL, "writeMessageFile: Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (MESSAGE_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'm';
	psHeader->aFileType[1] = 'e';
	psHeader->aFileType[2] = 's';
	psHeader->aFileType[3] = 's';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numMessages;

	psSaveMessage = (SAVE_MESSAGE_36 *) (pFileData + MESSAGE_HEADER_SIZE);
	//save each type of reesearch
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psMessage = apsMessages[player];
		for(psMessage = apsMessages[player]; psMessage != NULL;psMessage = psMessage->psNext)
		{
			psSaveMessage->type = psMessage->type;	//The type of message
			psSaveMessage->dataType = psMessage->dataType;

			if (psMessage->type == MSG_PROXIMITY)
			{
				//get the matching proximity message
				for(psProx = apsProxDisp[player]; psProx != NULL; psProx = psProx->psNext)
				{
					//compare the pointers
					if (psProx->psMessage == psMessage)
					{
						break;
					}
				}
				ASSERT( psProx != NULL,"Save message; proximity display not found for message" );

				if (psProx->type == POS_PROXDATA)
				{
					//message has viewdata so store the name
					psSaveMessage->bObj = false;
					pViewData = (VIEWDATA*)psMessage->pViewData;
					ASSERT( strlen(pViewData->pName) < MAX_GAME_STR_SIZE,"writeMessageFile; viewdata pName Error" );
					strcpy(psSaveMessage->name,pViewData->pName);	//Pointer to view data - if any - should be some!

					// save beacon data
					if(psMessage->dataType == MSG_DATA_BEACON)
					{
						VIEW_PROXIMITY * viewData = (VIEW_PROXIMITY *)((VIEWDATA *)psMessage->pViewData)->pData;

						psSaveMessage->locX =  viewData->x;
						endian_udword(&psSaveMessage->locX);
						psSaveMessage->locY =  viewData->y;
						endian_udword(&psSaveMessage->locY);
						psSaveMessage->sender = viewData->sender;
						endian_sdword(&psSaveMessage->sender);
					}
				}
				else
				{
					//message has object so store ObjectId
					psSaveMessage->bObj = true;
					psObj = (BASE_OBJECT*)psMessage->pViewData;
					psSaveMessage->objId = psObj->id;//should be unique for these objects
				}
			}
			else
			{
				psSaveMessage->bObj = false;
				pViewData = (VIEWDATA*)psMessage->pViewData;
				ASSERT( strlen(pViewData->pName) < MAX_GAME_STR_SIZE,"writeMessageFile; viewdata pName Error" );
				strcpy(psSaveMessage->name,pViewData->pName);	//Pointer to view data - if any - should be some!
			}
			psSaveMessage->read = psMessage->read;			//flag to indicate whether message has been read
			psSaveMessage->player = psMessage->player;		//which player this message belongs to

			endian_sdword((SDWORD*)&psSaveMessage->type); /* FIXME: enum may be different type! */
			endian_sdword((SDWORD*)&psSaveMessage->dataType);
			endian_udword(&psSaveMessage->objId);
			endian_udword(&psSaveMessage->player);

			psSaveMessage = (SAVE_MESSAGE_36 *)((char *)psSaveMessage + 	sizeof(SAVE_MESSAGE_36));
		}
	}

	/* MESSAGE_SAVEHEADER */
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
// load up saved flag file
BOOL loadSaveFlag(char *pFileData, UDWORD filesize)
{
	FLAG_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (FLAG_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'f' || psHeader->aFileType[1] != 'l' ||
		psHeader->aFileType[2] != 'a' || psHeader->aFileType[3] != 'g')
	{
		debug( LOG_ERROR, "loadSaveflag: Incorrect file type" );

		return false;
	}

	/* FLAG_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	//increment to the start of the data
	pFileData += FLAG_HEADER_SIZE;

	/* Check the file version */
	if (!loadSaveFlagV(pFileData, filesize, psHeader->quantity, psHeader->version))
	{
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveFlagV(char *pFileData, UDWORD filesize, UDWORD numflags, UDWORD version)
{
	SAVE_FLAG		*psSaveflag;
	FLAG_POSITION	*psflag;
	UDWORD			i;
	STRUCTURE*		psStruct;
	UDWORD			factoryToFind = 0;
	UDWORD			sizeOfSaveFlag;

	//clear any flags put in during level loads
	freeAllFlagPositions();
	initFactoryNumFlag();//clear the factory masks, we will find the factories from their assembly points


	//check file
	if (version <= VERSION_18)
	{
		sizeOfSaveFlag = sizeof(SAVE_FLAG_V18);
	}
	else
	{
		sizeOfSaveFlag = sizeof(SAVE_FLAG);
	}

	if ((sizeOfSaveFlag * numflags + FLAG_HEADER_SIZE) > filesize)
	{
		debug( LOG_ERROR, "loadSaveflag: unexpected end of file" );

		return false;
	}

	// Load the data
	for (i = 0; i < numflags; i++, pFileData += sizeOfSaveFlag)
	{
		psSaveflag = (SAVE_FLAG *) pFileData;

		/* SAVE_FLAG */
		endian_sdword((SDWORD*) &psSaveflag->type); /* FIXME: enum may not be this type! */
		endian_udword(&psSaveflag->frameNumber);
		endian_udword(&psSaveflag->screenX);
		endian_udword(&psSaveflag->screenY);
		endian_udword(&psSaveflag->screenR);
		endian_udword(&psSaveflag->player);
		endian_sdword(&psSaveflag->coords.x);
		endian_sdword(&psSaveflag->coords.y);
		endian_sdword(&psSaveflag->coords.z);
		endian_udword(&psSaveflag->repairId);

		createFlagPosition(&psflag, psSaveflag->player);
		psflag->type = psSaveflag->type;				//The type of flag
		psflag->frameNumber = psSaveflag->frameNumber;	//when the Position was last drawn
		psflag->screenX = psSaveflag->screenX;			//screen coords and radius of Position imd
		psflag->screenY = psSaveflag->screenY;
		psflag->screenR = psSaveflag->screenR;
		psflag->player = psSaveflag->player;	// which player the position belongs to
		psflag->selected = psSaveflag->selected;		//flag to indicate whether the Position
		psflag->coords = psSaveflag->coords;			//the world coords of the Position
		psflag->factoryInc = psSaveflag->factoryInc;	//indicates whether the first, second etc factory
		psflag->factoryType = psSaveflag->factoryType;	//indicates whether standard, cyborg or vtol factory

		if ((psflag->type == POS_DELIVERY) || (psflag->type == POS_TEMPDELIVERY))
		{
			if (psflag->factoryType == FACTORY_FLAG)
			{
				factoryToFind = REF_FACTORY;
			}
			else if (psflag->factoryType == CYBORG_FLAG)
			{
				factoryToFind = REF_CYBORG_FACTORY;
			}
			else if (psflag->factoryType == VTOL_FLAG)
			{
				factoryToFind = REF_VTOL_FACTORY;
			}
			else if (psflag->factoryType == REPAIR_FLAG)
			{
				factoryToFind = REF_REPAIR_FACILITY;
			}
			else
			{
				ASSERT( false,"loadSaveFlagV delivery flag type not recognised?" );
			}

			if (factoryToFind == REF_REPAIR_FACILITY)
			{
				if (version > VERSION_18)
				{
					psStruct = (STRUCTURE*)getBaseObjFromId(psSaveflag->repairId);
					if (psStruct != NULL)
					{
						if (psStruct->type != OBJ_STRUCTURE)
						{
							ASSERT( false,"loadFlag found duplicate Id for repair facility" );
						}
						else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
						{
							if (psStruct->pFunctionality != NULL)
							{
								//this is the one so set it
								((REPAIR_FACILITY *)psStruct->pFunctionality)->psDeliveryPoint = psflag;
							}
						}
					}
				}
			}
			else
			{
				//okay find the player factory with this inc and set this as the assembly point
				for (psStruct = apsStructLists[psflag->player]; psStruct != NULL; psStruct = psStruct->psNext)
				{
					if (psStruct->pStructureType->type == factoryToFind)
					{
						if ((UDWORD)((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint == psflag->factoryInc)
						{
							//this is the one so set it
							((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint = psflag;
						}
					}
				}
			}
		}

		if (psflag->type == POS_DELIVERY)//dont add POS_TEMPDELIVERYs
		{
			addFlagPosition(psflag);
		}
		else if (psflag->type == POS_TEMPDELIVERY)//but make them real flags
		{
			psflag->type = POS_DELIVERY;
		}
	}

	resetFactoryNumFlag();//set the new numbers into the masks

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current flags per player
static BOOL writeFlagFile(char *pFileName)
{
	FLAG_SAVEHEADER		*psHeader;
	SAVE_FLAG			*psSaveflag;
	STRUCTURE			*psStruct;
	FACTORY				*psFactory;
	char				*pFileData;
	UDWORD				fileSize, player;
	FLAG_POSITION		*psflag;
	UDWORD				numflags = 0;


	// Calculate the file size
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psflag = apsFlagPosLists[player]; psflag != NULL;psflag = psflag->psNext)
		{
			numflags++;
		}

	}
	//and add the delivery points not in the list
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psStruct = apsStructLists[player]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (psStruct->pFunctionality)
			{
				switch (psStruct->pStructureType->type)
				{
				case REF_FACTORY:
				case REF_CYBORG_FACTORY:
				case REF_VTOL_FACTORY:
					psFactory = ((FACTORY *)psStruct->pFunctionality);
					//if there is a commander then has been temporarily removed
					//so put it back
					if (psFactory->psCommander)
					{
						psflag = psFactory->psAssemblyPoint;
						if (psflag != NULL)
						{
							numflags++;
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}

	fileSize = FLAG_HEADER_SIZE + (sizeof(SAVE_FLAG) *
		numflags);


	//allocate the buffer space
	pFileData = (char*)malloc(fileSize);
	if (!pFileData)
	{
		debug( LOG_FATAL, "writeflagFile: Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (FLAG_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'f';
	psHeader->aFileType[1] = 'l';
	psHeader->aFileType[2] = 'a';
	psHeader->aFileType[3] = 'g';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = numflags;

	psSaveflag = (SAVE_FLAG *) (pFileData + FLAG_HEADER_SIZE);
	//save each type of reesearch
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psflag = apsFlagPosLists[player];
		for(psflag = apsFlagPosLists[player]; psflag != NULL;psflag = psflag->psNext)
		{
			psSaveflag->type = psflag->type;				//The type of flag
			psSaveflag->frameNumber = psflag->frameNumber;	//when the Position was last drawn
			psSaveflag->screenX = psflag->screenX;			//screen coords and radius of Position imd
			psSaveflag->screenY = psflag->screenY;
			psSaveflag->screenR = psflag->screenR;
			psSaveflag->player = psflag->player;			//which player the Position belongs to
			psSaveflag->selected = psflag->selected;		//flag to indicate whether the Position
			psSaveflag->coords = psflag->coords;			//the world coords of the Position
			psSaveflag->factoryInc = psflag->factoryInc;	//indicates whether the first, second etc factory
			psSaveflag->factoryType = psflag->factoryType;	//indicates whether standard, cyborg or vtol factory
			if (psflag->factoryType == REPAIR_FLAG)
			{
				//get repair facility id
				psSaveflag->repairId = getRepairIdFromFlag(psflag);
			}

			/* SAVE_FLAG */
			endian_sdword((SDWORD*)&psSaveflag->type); /* FIXME: enum may be different type! */
			endian_udword(&psSaveflag->frameNumber);
			endian_udword(&psSaveflag->screenX);
			endian_udword(&psSaveflag->screenY);
			endian_udword(&psSaveflag->screenR);
			endian_udword(&psSaveflag->player);
			endian_sdword(&psSaveflag->coords.x);
			endian_sdword(&psSaveflag->coords.y);
			endian_sdword(&psSaveflag->coords.z);
			endian_udword(&psSaveflag->repairId);

			psSaveflag = (SAVE_FLAG *)((char *)psSaveflag + 	sizeof(SAVE_FLAG));
		}
	}
	//and add the delivery points not in the list
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(psStruct = apsStructLists[player]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (psStruct->pFunctionality)
			{
				switch (psStruct->pStructureType->type)
				{
				case REF_FACTORY:
				case REF_CYBORG_FACTORY:
				case REF_VTOL_FACTORY:
					psFactory = ((FACTORY *)psStruct->pFunctionality);
					//if there is a commander then has been temporarily removed
					//so put it back
					if (psFactory->psCommander)
					{
						psflag = psFactory->psAssemblyPoint;
						if (psflag != NULL)
						{
							psSaveflag->type = POS_TEMPDELIVERY;				//The type of flag
							psSaveflag->frameNumber = psflag->frameNumber;	//when the Position was last drawn
							psSaveflag->screenX = psflag->screenX;			//screen coords and radius of Position imd
							psSaveflag->screenY = psflag->screenY;
							psSaveflag->screenR = psflag->screenR;
							psSaveflag->player = psflag->player;			//which player the Position belongs to
							psSaveflag->selected = psflag->selected;		//flag to indicate whether the Position
							psSaveflag->coords = psflag->coords;			//the world coords of the Position
							psSaveflag->factoryInc = psflag->factoryInc;	//indicates whether the first, second etc factory
							psSaveflag->factoryType = psflag->factoryType;	//indicates whether standard, cyborg or vtol factory
							if (psflag->factoryType == REPAIR_FLAG)
							{
								//get repair facility id
								psSaveflag->repairId = getRepairIdFromFlag(psflag);
							}

							/* SAVE_FLAG */
							endian_sdword((SDWORD*)&psSaveflag->type); /* FIXME: enum may be different type! */
							endian_udword(&psSaveflag->frameNumber);
							endian_udword(&psSaveflag->screenX);
							endian_udword(&psSaveflag->screenY);
							endian_udword(&psSaveflag->screenR);
							endian_udword(&psSaveflag->player);
							endian_sdword(&psSaveflag->coords.x);
							endian_sdword(&psSaveflag->coords.y);
							endian_sdword(&psSaveflag->coords.z);
							endian_udword(&psSaveflag->repairId);

							psSaveflag = (SAVE_FLAG *)((char *)psSaveflag + 	sizeof(SAVE_FLAG));
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}

	/* FLAG_SAVEHEADER */
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
BOOL loadSaveProduction(char *pFileData, UDWORD filesize)
{
	PRODUCTION_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (PRODUCTION_SAVEHEADER*)pFileData;
	if (psHeader->aFileType[0] != 'p' || psHeader->aFileType[1] != 'r' ||
		psHeader->aFileType[2] != 'o' || psHeader->aFileType[3] != 'd')
	{
		debug( LOG_ERROR, "loadSaveProduction: Incorrect file type" );

		return false;
	}

	/* PRODUCTION_SAVEHEADER */
	endian_udword(&psHeader->version);

	//increment to the start of the data
	pFileData += PRODUCTION_HEADER_SIZE;

	/* Check the file version */
	if (!loadSaveProductionV(pFileData, filesize, psHeader->version))
	{
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveProductionV(char *pFileData, UDWORD filesize, UDWORD version)
{
	SAVE_PRODUCTION	*psSaveProduction;
	PRODUCTION_RUN	*psCurrentProd;
	UDWORD			factoryType,factoryNum,runNum;

	//check file
	if ((sizeof(SAVE_PRODUCTION) * NUM_FACTORY_TYPES * MAX_FACTORY * MAX_PROD_RUN + PRODUCTION_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveProduction: unexpected end of file" );

		return false;
	}

	//save each production run
	for (factoryType = 0; factoryType < NUM_FACTORY_TYPES; factoryType++)
	{
		for (factoryNum = 0; factoryNum < MAX_FACTORY; factoryNum++)
		{
			for (runNum = 0; runNum < MAX_PROD_RUN; runNum++)
			{
				psSaveProduction = (SAVE_PRODUCTION *)pFileData;
				psCurrentProd = &asProductionRun[factoryType][factoryNum][runNum];

				/* SAVE_PRODUCTION */
				endian_udword(&psSaveProduction->multiPlayerID);

				psCurrentProd->quantity = psSaveProduction->quantity;
				psCurrentProd->built = psSaveProduction->built;
				if (psSaveProduction->multiPlayerID != NULL_ID)
				{
					psCurrentProd->psTemplate = getTemplateFromMultiPlayerID(psSaveProduction->multiPlayerID);
				}
				else
				{
					psCurrentProd->psTemplate = NULL;
				}
				pFileData += sizeof(SAVE_PRODUCTION);
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current production figures for factories
static BOOL writeProductionFile(char *pFileName)
{
	PRODUCTION_SAVEHEADER	*psHeader;
	SAVE_PRODUCTION			*psSaveProduction;
	char				*pFileData;
	UDWORD				fileSize;
	PRODUCTION_RUN	*psCurrentProd;
	UDWORD				factoryType,factoryNum,runNum;

	fileSize = PRODUCTION_HEADER_SIZE + (sizeof(SAVE_PRODUCTION) *
		NUM_FACTORY_TYPES * MAX_FACTORY * MAX_PROD_RUN);

	//allocate the buffer space
	pFileData = (char*)malloc(fileSize);
	if (!pFileData)
	{
		debug( LOG_FATAL, "writeProductionFile: Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (PRODUCTION_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'p';
	psHeader->aFileType[1] = 'r';
	psHeader->aFileType[2] = 'o';
	psHeader->aFileType[3] = 'd';
	psHeader->version = CURRENT_VERSION_NUM;

	psSaveProduction = (SAVE_PRODUCTION *) (pFileData + PRODUCTION_HEADER_SIZE);
	//save each production run
	for (factoryType = 0; factoryType < NUM_FACTORY_TYPES; factoryType++)
	{
		for (factoryNum = 0; factoryNum < MAX_FACTORY; factoryNum++)
		{
			for (runNum = 0; runNum < MAX_PROD_RUN; runNum++)
			{
				psCurrentProd = &asProductionRun[factoryType][factoryNum][runNum];
				psSaveProduction->quantity = psCurrentProd->quantity;
				psSaveProduction->built = psCurrentProd->built;
				psSaveProduction->multiPlayerID = NULL_ID;
				if (psCurrentProd->psTemplate != NULL)
				{
					psSaveProduction->multiPlayerID = psCurrentProd->psTemplate->multiPlayerID;
				}

				/* SAVE_PRODUCTION */
				endian_udword(&psSaveProduction->multiPlayerID);

				psSaveProduction = (SAVE_PRODUCTION *)((char *)psSaveProduction + 	sizeof(SAVE_PRODUCTION));
			}
		}
	}

	/* PRODUCTION_SAVEHEADER */
	endian_udword(&psHeader->version);

	if (!saveFile(pFileName, pFileData, fileSize))
	{
		return false;
	}
	free(pFileData);

	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveStructLimits(char *pFileData, UDWORD filesize)
{
	STRUCTLIMITS_SAVEHEADER		*psHeader;

	// Check the file type
	psHeader = (STRUCTLIMITS_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'l' || psHeader->aFileType[1] != 'm' ||
		psHeader->aFileType[2] != 't' || psHeader->aFileType[3] != 's')
	{
		debug( LOG_ERROR, "loadSaveStructLimits: Incorrect file type" );

		return false;
	}

	//increment to the start of the data
	pFileData += STRUCTLIMITS_HEADER_SIZE;

	/* STRUCTLIMITS_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	debug(LOG_SAVE, "fileversion is %u ", psHeader->version);

	// Check the file version
	if ((psHeader->version >= VERSION_15) && (psHeader->version <= VERSION_19))
	{
		if (!loadSaveStructLimitsV19(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else if (psHeader->version <= CURRENT_VERSION_NUM)
	{
		if (!loadSaveStructLimitsV(pFileData, filesize, psHeader->quantity))
		{
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported limits file format version %u", psHeader->version);
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/* code specific to version 2 of saveStructLimits */
BOOL loadSaveStructLimitsV19(char *pFileData, UDWORD filesize, UDWORD numLimits)
{
	SAVE_STRUCTLIMITS_V2	*psSaveLimits;
	UDWORD					count, statInc;
	BOOL					found;
	STRUCTURE_STATS			*psStats;
	int SkippedRecords=0;

	psSaveLimits = (SAVE_STRUCTLIMITS_V2 *) malloc(sizeof(SAVE_STRUCTLIMITS_V2));
	if (!psSaveLimits)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	if ((sizeof(SAVE_STRUCTLIMITS_V2) * numLimits + STRUCTLIMITS_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveStructLimits: unexpected end of file" );

		return false;
	}

	// Load in the data
	for (count = 0; count < numLimits; count ++,
									pFileData += sizeof(SAVE_STRUCTLIMITS_V2))
	{
		memcpy(psSaveLimits, pFileData, sizeof(SAVE_STRUCTLIMITS_V2));

		//get the stats for this structure
		found = false;
		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name
			if (!strcmp(psStats->pName, psSaveLimits->name))
			{
				found = true;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "The structure no longer exists. The limits have not been set! - %s", psSaveLimits->name );

			continue;
		}

		if (psSaveLimits->player < MAX_PLAYERS)
		{
			asStructLimits[psSaveLimits->player][statInc].limit = psSaveLimits->limit;
		}
		else
		{
			SkippedRecords++;
		}

	}

	if (SkippedRecords>0)
	{
		debug( LOG_ERROR, "Skipped %d records in structure limits due to bad player number\n", SkippedRecords );
		free(psSaveLimits);
		return false;
	}
	free(psSaveLimits);
	return true;
}

// -----------------------------------------------------------------------------------------
BOOL loadSaveStructLimitsV(char *pFileData, UDWORD filesize, UDWORD numLimits)
{
	SAVE_STRUCTLIMITS		*psSaveLimits;
	UDWORD					count, statInc;
	BOOL					found;
	STRUCTURE_STATS			*psStats;
	int SkippedRecords=0;

	psSaveLimits = (SAVE_STRUCTLIMITS*) malloc(sizeof(SAVE_STRUCTLIMITS));
	if (!psSaveLimits)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	if ((sizeof(SAVE_STRUCTLIMITS) * numLimits + STRUCTLIMITS_HEADER_SIZE) >
		filesize)
	{
		debug( LOG_ERROR, "loadSaveStructLimits: unexpected end of file" );

		return false;
	}

	// Load in the data
	for (count = 0; count < numLimits; count ++,
									pFileData += sizeof(SAVE_STRUCTLIMITS))
	{
		memcpy(psSaveLimits, pFileData, sizeof(SAVE_STRUCTLIMITS));

		//get the stats for this structure
		found = false;
		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name
			if (!strcmp(psStats->pName, psSaveLimits->name))
			{
				found = true;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug( LOG_ERROR, "The structure no longer exists. The limits have not been set! - %s", psSaveLimits->name );

			continue;
		}

		if (psSaveLimits->player < MAX_PLAYERS)
		{
			asStructLimits[psSaveLimits->player][statInc].limit = psSaveLimits->limit;
		}
		else
		{
			SkippedRecords++;
		}

	}

	if (SkippedRecords>0)
	{
		debug( LOG_ERROR, "Skipped %d records in structure limits due to bad player number\n", SkippedRecords );
		free(psSaveLimits);
		return false;
	}
	free(psSaveLimits);
	return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes the list of structure limits to a file
*/
BOOL writeStructLimitsFile(char *pFileName)
{
	char *pFileData = NULL;
	UDWORD						fileSize, totalLimits=0, i, player;
	STRUCTLIMITS_SAVEHEADER		*psHeader;
	SAVE_STRUCTLIMITS			*psSaveLimit;
	STRUCTURE_STATS				*psStructStats;
	BOOL status = true;

	totalLimits = numStructureStats * MAX_PLAYERS;

	// Allocate the data buffer
	fileSize = STRUCTLIMITS_HEADER_SIZE + (totalLimits * (sizeof(SAVE_STRUCTLIMITS)));
	pFileData = (char*)malloc(fileSize);
	if (pFileData == NULL)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	// Put the file header on the file
	psHeader = (STRUCTLIMITS_SAVEHEADER *)pFileData;
	psHeader->aFileType[0] = 'l';
	psHeader->aFileType[1] = 'm';
	psHeader->aFileType[2] = 't';
	psHeader->aFileType[3] = 's';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->quantity = totalLimits;

	psSaveLimit = (SAVE_STRUCTLIMITS*)(pFileData + STRUCTLIMITS_HEADER_SIZE);

	// Put the data into the buffer
	for (player = 0; player < MAX_PLAYERS ; player++)
	{
		psStructStats = asStructureStats;
		for(i = 0; i < numStructureStats; i++, psStructStats++)
		{
			strcpy(psSaveLimit->name, psStructStats->pName);
			psSaveLimit->limit = asStructLimits[player][i].limit;
			psSaveLimit->player = (UBYTE)player;
			psSaveLimit = (SAVE_STRUCTLIMITS *)((char *)psSaveLimit + sizeof(SAVE_STRUCTLIMITS));
		}
	}

	/* STRUCTLIMITS_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->quantity);

	// Write the data to the file
	if (pFileData != NULL) {
		status = saveFile(pFileName, pFileData, fileSize);
		free(pFileData);
		return status;
	}
	return false;
}

static const char FireSupport_tag_definition[] = "tagdefinitions/savegame/firesupport.def";
static const char FireSupport_file_identifier[] = "FIRESUPPORT";

/*!
 * Load the current fire-support designated commanders (the one who has fire-support enabled)
 */
BOOL readFiresupportDesignators(char *pFileName)
{
	unsigned int numPlayers, player;
	char formatIdentifier[12] = "";

	if (!tagOpenRead(FireSupport_tag_definition, pFileName))
	{
		debug(LOG_ERROR, "readFiresupportDesignators: Failed to open savegame %s", pFileName);
		return false;
	}
	debug(LOG_MAP, "Reading tagged savegame %s with definition %s:", pFileName, FireSupport_tag_definition);

	tagReadString(0x01, 12, formatIdentifier);
	if (strcmp(formatIdentifier, FireSupport_file_identifier) != 0)
	{
		debug(LOG_ERROR, "readFiresupportDesignators: Incompatble %s, 'FIRESUPPORT' expected", pFileName);
		return false;
	}

	numPlayers = tagReadEnter(0x02);
	for (player = 0; player < numPlayers; player++)
	{
		uint32_t id = tagRead(0x01);
		if (id != NULL_ID)
		{
			cmdDroidSetDesignator((DROID*)getBaseObjFromId(id));
		}
		tagReadNext();
	}
	tagReadLeave(0x02);

	tagClose();

	return true;
}


/*!
 * Save the current fire-support designated commanders (the one who has fire-support enabled)
 */
BOOL writeFiresupportDesignators(char *pFileName)
{
	unsigned int player;

	if (!tagOpenWrite(FireSupport_tag_definition, pFileName))
	{
		debug(LOG_ERROR, "writeFiresupportDesignators: Failed to create savegame %s", pFileName);
		return false;
	}
	debug(LOG_MAP, "Creating tagged savegame %s with definition %s:", pFileName, FireSupport_tag_definition);

	tagWriteString(0x01, FireSupport_file_identifier);
	tagWriteEnter(0x02, MAX_PLAYERS);
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		DROID * psDroid = cmdDroidGetDesignator(player);
		if (psDroid != NULL)
		{
			tagWrite(0x01, psDroid->id);
		}
		else
		{
			tagWrite(0x01, NULL_ID);
		}
		tagWriteNext();
	}
	tagWriteLeave(0x02);

	tagClose();

	return true;
}


// -----------------------------------------------------------------------------------------
// write the event state to a file on disk
static BOOL	writeScriptState(char *pFileName)
{
	static const int32_t current_event_version = 4;

	char	*pBuffer;
	UDWORD	fileSize;

	if (!eventSaveState(current_event_version, &pBuffer, &fileSize))
	{
		return false;
	}

	if (!saveFile(pFileName, pBuffer, fileSize))
	{
		return false;
	}
	free(pBuffer);
	return true;
}

// -----------------------------------------------------------------------------------------
// load the script state given a .gam name
BOOL loadScriptState(char *pFileName)
{
	char	*pFileData;
	UDWORD	fileSize;
	BOOL bHashed = false;

	// change the file extension
	pFileName[strlen(pFileName)-4] = (char)0;
	strcat(pFileName, ".es");

	pFileData = fileLoadBuffer;
	if (!loadFileToBuffer(pFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug( LOG_ERROR, "loadScriptState: couldn't load %s", pFileName );

		return false;
	}

	if (saveGameVersion > VERSION_12)
	{
		bHashed = true;
	}

	if (!eventLoadState(pFileData, fileSize, bHashed))
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
BOOL getSaveObjectName(char *pName)
{
	return true;
}

// -----------------------------------------------------------------------------------------
/*returns the current type of save game being loaded*/
UDWORD getSaveGameType(void)
{
	return gameType;
}


// -----------------------------------------------------------------------------------------
//copies a Stat name into a destination string for a given stat type and index
static BOOL getNameFromComp(UDWORD compType, char *pDest, UDWORD compIndex)
{
	BASE_STATS	*psStats;

	//allocate the stats pointer
	switch (compType)
	{
	case COMP_BODY:
		ASSERT_OR_RETURN( false, compIndex < numBodyStats, "Invalid range referenced for numBodyStats, %d > %d", compIndex, numBodyStats);
		psStats = (BASE_STATS *)(asBodyStats + compIndex);
		break;
	case COMP_BRAIN:
		ASSERT_OR_RETURN( false, compIndex < numBrainStats, "Invalid range referenced for numBrainStats, %d > %d", compIndex, numBrainStats);
		psStats = (BASE_STATS *)(asBrainStats + compIndex);
		break;
	case COMP_PROPULSION:
		ASSERT_OR_RETURN( false, compIndex < numPropulsionStats, "Invalid range referenced for numPropulsionStats, %d > %d", compIndex, numPropulsionStats);
		psStats = (BASE_STATS *)(asPropulsionStats + compIndex);
		break;
	case COMP_REPAIRUNIT:
		ASSERT_OR_RETURN( false, compIndex < numRepairStats, "Invalid range referenced for numRepairStats, %d > %d", compIndex, numRepairStats);
		psStats = (BASE_STATS*)(asRepairStats  + compIndex);
		break;
	case COMP_ECM:
		ASSERT_OR_RETURN( false, compIndex < numECMStats, "Invalid range referenced for numECMStats, %d > %d", compIndex, numECMStats);
		psStats = (BASE_STATS*)(asECMStats + compIndex);
		break;
	case COMP_SENSOR:
		ASSERT_OR_RETURN( false, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = (BASE_STATS*)(asSensorStats + compIndex);
		break;
	case COMP_CONSTRUCT:
		ASSERT_OR_RETURN( false, compIndex < numConstructStats, "Invalid range referenced for numConstructStats, %d > %d", compIndex, numConstructStats);
		psStats = (BASE_STATS*)(asConstructStats + compIndex);
		break;
	/*case COMP_PROGRAM:
		psStats = (BASE_STATS*)(asProgramStats + compIndex);
		break;*/
	case COMP_WEAPON:
		ASSERT_OR_RETURN( false, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
		psStats = (BASE_STATS*)(asWeaponStats + compIndex);
		break;
	default:
		debug( LOG_ERROR, "Invalid component type - game.c" );

		return false;
	}

	//copy the name into the destination string
	strcpy(pDest, psStats->pName);
	return true;
}
// -----------------------------------------------------------------------------------------
// END

/**
 * \param[out] backDropSprite The premade map texture.
 * \param scale               Scale of the map texture.
 * \param[out] playeridpos    Will contain the position on the map where the player's HQ are located.
 *
 * Reads the current map and colours the map preview for any structures
 * present. Additionally we load the player's HQ location into playeridpos so
 * we know the player's starting location.
 */
BOOL plotStructurePreview16(char *backDropSprite, Vector2i playeridpos[])
{
	SAVE_STRUCTURE sSave;  // close eyes now.
	// assumes save_struct is larger than all previous ones...
	SAVE_STRUCTURE_V2 *psSaveStructure2 = (SAVE_STRUCTURE_V2*)&sSave;
	SAVE_STRUCTURE_V20 *psSaveStructure20= (SAVE_STRUCTURE_V20*)&sSave;
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
			color.byte.b = 0xff;
			color.byte.g = 0;
			color.byte.r = 0xff;
		}

		// and now we blit the color to the texture
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx)] = color.byte.r;
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 1] = color.byte.g;
		backDropSprite[3 * ((yy * BACKDROP_HACK_WIDTH) + xx) + 2] = color.byte.b;
	}

	// NOTE: would do fallback if FBO is not available here.
	return true;
}
