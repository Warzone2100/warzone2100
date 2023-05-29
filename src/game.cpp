/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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

/* Standard library headers */
#include <physfs.h>
#include <string.h>

/* Warzone src and library headers */
#include "lib/framework/endian_hack.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/wztime.h"

#include <wzmaplib/map_terrain_types.h>

#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "modding.h"
#include "main.h"
#include "game.h"
#include "qtscript.h"
#include "fpath.h"
#include "difficulty.h"
#include "map.h"
#include "move.h"
#include "droid.h"
#include "order.h"
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
#include "multistat.h"
#include "multiint.h"
#include "wrappers.h"
#include "challenge.h"
#include "combat.h"
#include "template.h"
#include "version.h"
#include "lib/ivis_opengl/screen.h"
#include <ctime>
#include "multimenu.h"
#include "console.h"
#include "multigifts.h"
#include "wzscriptdebug.h"
#include "build_tools/autorevision.h"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wcast-align"	// TODO: FIXME!
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wcast-align"	// TODO: FIXME!
#endif

#define UNUSED(x) (void)(x)

// Ignore unused functions for now (until full cleanup of old binary savegame .gam writing)
#if defined( _MSC_VER )
# pragma warning( disable : 4505 ) // warning C4505: unreferenced function with internal linkage has been removed
#endif
#if defined(__clang__)
# pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wunused-function"
#endif

bool saveJSONToFile(const nlohmann::json& obj, const char* pFileName)
{
	std::ostringstream stream;
	try {
		stream << obj.dump(4) << std::endl;
	}
	catch (const std::exception &e) {
		ASSERT(false, "Failed to save JSON to %s with error: %s", pFileName, e.what());
		return false;
	}
	std::string jsonString = stream.str();
	debug(LOG_SAVE, "%s %s", "Saving", pFileName);
	return saveFile(pFileName, jsonString.c_str(), jsonString.size());
}

void gameScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	intScreenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	loadSaveScreenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	challengesScreenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	multiMenuScreenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	display3dScreenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	consoleScreenDidChangeSize(oldWidth, oldHeight, newWidth, newHeight);
	frontendScreenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	widgOverlaysScreenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight); // must be last!
}

void gameDisplayScaleFactorDidChange(float newDisplayScaleFactor)
{
	// The text subsystem requires the game -> renderer scale factor, which potentially differs from
	// the display scale factor.
	unsigned int horizGameToRendererScaleFactor = 0, vertGameToRendererScaleFactor = 0;
	wzGetGameToRendererScaleFactorInt(&horizGameToRendererScaleFactor, &vertGameToRendererScaleFactor);
	iV_TextUpdateScaleFactor(horizGameToRendererScaleFactor, vertGameToRendererScaleFactor);
}

static unsigned int currentGameVersion = 410;

#define MAX_SAVE_NAME_SIZE_V19	40
#define MAX_SAVE_NAME_SIZE	60

static const UDWORD NULL_ID = UDWORD_MAX;
#define SAVEKEY_ONMISSION	0x100
static UDWORD RemapPlayerNumber(UDWORD OldNumber);
bool writeGameInfo(const char *pFileName);
static nonstd::optional<nlohmann::json> readGamJson(const char*);
/** struct used to store the data for retreating. */
struct RUN_DATA
{
	Vector2i sPos = Vector2i(0, 0); ///< position to where units should flee to.
	uint8_t forceLevel = 0;  ///< number of units below which others might flee.
	uint8_t healthLevel = 0; ///< health percentage value below which it might flee. This value is used for groups only.
	uint8_t leadership = 0;  ///< basic value that will be used on calculations of the flee probability.
};

// return positions for vtols, at one time.
Vector2i asVTOLReturnPos[MAX_PLAYERS];

struct GAME_SAVEHEADER
{
	char        aFileType[4];
	uint32_t    version;
};

static bool serializeSaveGameHeader(PHYSFS_file *fileHandle, const GAME_SAVEHEADER *serializeHeader)
{
	if (WZ_PHYSFS_writeBytes(fileHandle, serializeHeader->aFileType, 4) != 4)
	{
		return false;
	}

	// Write version numbers below version 35 as little-endian, and those above as big-endian
	if (serializeHeader->version < VERSION_35)
	{
		return PHYSFS_writeULE32(fileHandle, serializeHeader->version);
	}
	else
	{
		return PHYSFS_writeUBE32(fileHandle, serializeHeader->version);
	}
}

static bool deserializeSaveGameHeader(PHYSFS_file *fileHandle, GAME_SAVEHEADER *serializeHeader)
{
	// Read in the header from the file
	if (WZ_PHYSFS_readBytes(fileHandle, serializeHeader->aFileType, 4) != 4
	    || WZ_PHYSFS_readBytes(fileHandle, &serializeHeader->version, sizeof(uint32_t)) != sizeof(uint32_t))
	{
		return false;
	}

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
	UDWORD				periodicalDamageStart; \
	UDWORD				periodicalDamage

#define OBJECT_SAVE_V20 \
	char				name[MAX_SAVE_NAME_SIZE]; \
	UDWORD				id; \
	UDWORD				x,y,z; \
	UDWORD				direction; \
	UDWORD				player; \
	int32_t		inFire; \
	UDWORD				periodicalDamageStart; \
	UDWORD				periodicalDamage

struct SAVE_POWER
{
	uint32_t    currentPower;
	uint32_t    extractedPower; // used for hacks
};
static void serializeSavePowerData_json(nlohmann::json &o, const SAVE_POWER *serializePower)
{
	o["currentPower"] = serializePower->currentPower;
	o["extractedPower"] = serializePower->extractedPower;
}
static bool serializeSavePowerData(PHYSFS_file *fileHandle, const SAVE_POWER *serializePower)
{
	return (PHYSFS_writeUBE32(fileHandle, serializePower->currentPower)
	        && PHYSFS_writeUBE32(fileHandle, serializePower->extractedPower));
}
static void deserializeSavePowerData_json(const nlohmann::json &o, SAVE_POWER *serializePower)
{
	serializePower->currentPower = o.at("currentPower").get<uint32_t>();
	serializePower->extractedPower = o.at("extractedPower").get<uint32_t>();
}
static bool deserializeSavePowerData(PHYSFS_file *fileHandle, SAVE_POWER *serializePower)
{
	return (PHYSFS_readUBE32(fileHandle, &serializePower->currentPower)
	        && PHYSFS_readUBE32(fileHandle, &serializePower->extractedPower));
}
static void serializeVector3i_json(nlohmann::json &o, const Vector3i *serializeVector)
{
	o["x"] = serializeVector->x;
	o["y"] = serializeVector->y;
	o["z"] = serializeVector->z;
}
static bool serializeVector3i(PHYSFS_file *fileHandle, const Vector3i *serializeVector)
{
	return (PHYSFS_writeSBE32(fileHandle, serializeVector->x)
	        && PHYSFS_writeSBE32(fileHandle, serializeVector->y)
	        && PHYSFS_writeSBE32(fileHandle, serializeVector->z));
}

static void deserializeVector3i_json(const nlohmann::json &o, Vector3i *serializeVector)
{
	serializeVector->x = o.at("x").get<int32_t>();
	serializeVector->y = o.at("y").get<int32_t>();
	serializeVector->z = o.at("z").get<int32_t>();
}

static bool deserializeVector3i(PHYSFS_file *fileHandle, Vector3i *serializeVector)
{
	int32_t x, y, z;

	if (!PHYSFS_readSBE32(fileHandle, &x)
	    || !PHYSFS_readSBE32(fileHandle, &y)
	    || !PHYSFS_readSBE32(fileHandle, &z))
	{
		return false;
	}

	serializeVector-> x = x;
	serializeVector-> y = y;
	serializeVector-> z = z;

	return true;
}
static void serializeVector2i_json(nlohmann::json &o,  const Vector2i *serializeVector)
{
	o["x"] = serializeVector->x;
	o["y"] = serializeVector->y;
}

static bool serializeVector2i(PHYSFS_file *fileHandle, const Vector2i *serializeVector)
{
	return (PHYSFS_writeSBE32(fileHandle, serializeVector->x)
	        && PHYSFS_writeSBE32(fileHandle, serializeVector->y));
}

static void deserializeVector2i_json(const nlohmann::json &o, Vector2i *serializeVector)
{
	serializeVector->x = o.at("x").get<int32_t>();
	serializeVector->y = o.at("y").get<int32_t>();
}

static bool deserializeVector2i(PHYSFS_file *fileHandle, Vector2i *serializeVector)
{
	int32_t x, y;

	if (!PHYSFS_readSBE32(fileHandle, &x)
	    || !PHYSFS_readSBE32(fileHandle, &y))
	{
		return false;
	}

	serializeVector-> x = x;
	serializeVector-> y = y;

	return true;
}
static void serializeiViewData_json(nlohmann::json &o, const iView *serializeView)
{
	auto viewP = nlohmann::json::object();
	serializeVector3i_json(viewP, &serializeView->p);
	o["viewDataP"] = viewP;
	auto viewR = nlohmann::json::object();
	serializeVector3i_json(viewR, &serializeView->r);
	o["viewDataR"] = viewR;
}

static bool serializeiViewData(PHYSFS_file *fileHandle, const iView *serializeView)
{
	return (serializeVector3i(fileHandle, &serializeView->p)
	        && serializeVector3i(fileHandle, &serializeView->r));
}

static void deserializeiViewData_json(const nlohmann::json &o, iView *serializeView)
{
	deserializeVector3i_json(o.at("viewDataP"), &serializeView->p);
	deserializeVector3i_json(o.at("viewDataR"), &serializeView->r);
}

static bool deserializeiViewData(PHYSFS_file *fileHandle, iView *serializeView)
{
	return (deserializeVector3i(fileHandle, &serializeView->p)
	        && deserializeVector3i(fileHandle, &serializeView->r));
}
static void serializeRunData_json(nlohmann::json &o, const RUN_DATA *serializeRun)
{
	serializeVector2i_json(o, &serializeRun->sPos);
	o["forceLevel"] = serializeRun->forceLevel;
	o["healthLevel"] = serializeRun->healthLevel;
	o["leadership"] = serializeRun->leadership;
}
static bool serializeRunData(PHYSFS_file *fileHandle, const RUN_DATA *serializeRun)
{
	return (serializeVector2i(fileHandle, &serializeRun->sPos)
	        && PHYSFS_writeUBE8(fileHandle, serializeRun->forceLevel)
	        && PHYSFS_writeUBE8(fileHandle, serializeRun->healthLevel)
	        && PHYSFS_writeUBE8(fileHandle, serializeRun->leadership));
}
static void deserializeRunData_json(const nlohmann::json &o, RUN_DATA *serializeRun)
{
	deserializeVector2i_json(o, &serializeRun->sPos);
	serializeRun->forceLevel = o.at("forceLevel").get<uint8_t>();
	serializeRun->healthLevel = o.at("healthLevel").get<uint8_t>();
	serializeRun->leadership = o.at("leadership").get<uint8_t>();
}
static bool deserializeRunData(PHYSFS_file *fileHandle, RUN_DATA *serializeRun)
{
	return (deserializeVector2i(fileHandle, &serializeRun->sPos)
	        && PHYSFS_readUBE8(fileHandle, &serializeRun->forceLevel)
	        && PHYSFS_readUBE8(fileHandle, &serializeRun->healthLevel)
	        && PHYSFS_readUBE8(fileHandle, &serializeRun->leadership));
}
static void serializeLandingZoneData_json(nlohmann::json &o, const LANDING_ZONE *serializeLandZone)
{
	o["x1"] = serializeLandZone->x1;
	o["y1"] = serializeLandZone->y1;
	o["x2"] = serializeLandZone->x2;
	o["y2"] = serializeLandZone->y2;
}
static bool serializeLandingZoneData(PHYSFS_file *fileHandle, const LANDING_ZONE *serializeLandZone)
{
	return (PHYSFS_writeUBE8(fileHandle,    serializeLandZone->x1)
	        && PHYSFS_writeUBE8(fileHandle, serializeLandZone->y1)
	        && PHYSFS_writeUBE8(fileHandle, serializeLandZone->x2)
	        && PHYSFS_writeUBE8(fileHandle, serializeLandZone->y2));
}

static void deserializeLandingZoneData_json(const nlohmann::json &o, LANDING_ZONE *serializeLandZone)
{
	serializeLandZone->x1 = o.at("x1").get<uint8_t>();
	serializeLandZone->y1 = o.at("y1").get<uint8_t>();
	serializeLandZone->x2 = o.at("x2").get<uint8_t>();
	serializeLandZone->y2 = o.at("y2").get<uint8_t>();
}

static bool deserializeLandingZoneData(PHYSFS_file *fileHandle, LANDING_ZONE *serializeLandZone)
{
	return (PHYSFS_readUBE8(fileHandle, &serializeLandZone->x1)
	        && PHYSFS_readUBE8(fileHandle, &serializeLandZone->y1)
	        && PHYSFS_readUBE8(fileHandle, &serializeLandZone->x2)
	        && PHYSFS_readUBE8(fileHandle, &serializeLandZone->y2));
}
static void serializeMultiplayerGame_json(nlohmann::json &o, const MULTIPLAYERGAME *serializeMulti)
{
	o["multiType"] = serializeMulti->type;
	o["multiMapName"] = serializeMulti->map;
	o["multiMaxPlayers"] = serializeMulti->maxPlayers;
	o["multiGameName"] = serializeMulti->name;
	o["multiPower"] = serializeMulti->power;
	o["multiBase"] = serializeMulti->base;
	o["multiAlliance"] = serializeMulti->alliance;
	o["multiHashBytes"] = 32; // serializeMulti->hash.Bytes
	o["multiHash"] = serializeMulti->hash.toString();
	// skip more dummy

}
static bool serializeMultiplayerGame(PHYSFS_file *fileHandle, const MULTIPLAYERGAME *serializeMulti)
{
	const char *dummy8c = "DUMMYSTRING";

	if (!PHYSFS_writeUBE8(fileHandle, static_cast<uint8_t>(serializeMulti->type))
	    || WZ_PHYSFS_writeBytes(fileHandle, serializeMulti->map, 128) != 128
	    || WZ_PHYSFS_writeBytes(fileHandle, dummy8c, 8) != 8
	    || !PHYSFS_writeUBE8(fileHandle, serializeMulti->maxPlayers)
	    || WZ_PHYSFS_writeBytes(fileHandle, serializeMulti->name, 128) != 128
	    || !PHYSFS_writeSBE32(fileHandle, 0)
	    || !PHYSFS_writeUBE32(fileHandle, serializeMulti->power)
	    || !PHYSFS_writeUBE8(fileHandle, serializeMulti->base)
	    || !PHYSFS_writeUBE8(fileHandle, serializeMulti->alliance)
	    || !PHYSFS_writeUBE8(fileHandle, serializeMulti->hash.Bytes)
	    || !WZ_PHYSFS_writeBytes(fileHandle, serializeMulti->hash.bytes, serializeMulti->hash.Bytes)
	    || !PHYSFS_writeUBE16(fileHandle, 0)	// dummy, was bytesPerSec
	    || !PHYSFS_writeUBE8(fileHandle, 0)	// dummy, was packetsPerSec
	    || !PHYSFS_writeUBE8(fileHandle, challengeActive))	// reuse available field, was encryptKey
	{
		return false;
	}

	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		// dummy, was `skDiff` for each player
		if (!PHYSFS_writeUBE8(fileHandle, 0))
		{
			return false;
		}
	}

	return true;
}
static void deserializeMultiplayerGame_json(const nlohmann::json &o, MULTIPLAYERGAME *serializeMulti)
{
	serializeMulti->type = static_cast<LEVEL_TYPE>(o.at("multiType").get<uint8_t>());
	sstrcpy(serializeMulti->map,  o.at("multiMapName").get<std::string>().c_str());
	serializeMulti->maxPlayers = o.at("multiMaxPlayers").get<uint8_t>();
	sstrcpy(serializeMulti->name, o.at("multiGameName").get<std::string>().c_str());
	serializeMulti->power = o.at("multiPower").get<uint32_t>();
	serializeMulti->base = o.at("multiBase").get<uint8_t>();
	serializeMulti->alliance = o.at("multiAlliance").get<uint8_t>();
	Sha256 sha256;
	sha256.fromString(o.at("multiHash").get<std::string>());
	serializeMulti->hash = sha256;
}
static bool deserializeMultiplayerGame(PHYSFS_file *fileHandle, MULTIPLAYERGAME *serializeMulti)
{
	int32_t boolFog;
	uint8_t dummy8;
	uint16_t dummy16;
	char dummy8c[8];
	uint8_t hashSize;

	serializeMulti->hash.setZero();

	if (!PHYSFS_readUBE8(fileHandle, reinterpret_cast<uint8_t*>(&serializeMulti->type))
	    || WZ_PHYSFS_readBytes(fileHandle, serializeMulti->map, 128) != 128
	    || WZ_PHYSFS_readBytes(fileHandle, dummy8c, 8) != 8
	    || !PHYSFS_readUBE8(fileHandle, &serializeMulti->maxPlayers)
	    || WZ_PHYSFS_readBytes(fileHandle, serializeMulti->name, 128) != 128
	    || !PHYSFS_readSBE32(fileHandle, &boolFog)
	    || !PHYSFS_readUBE32(fileHandle, &serializeMulti->power)
	    || !PHYSFS_readUBE8(fileHandle, &serializeMulti->base)
	    || !PHYSFS_readUBE8(fileHandle, &serializeMulti->alliance)
	    || !PHYSFS_readUBE8(fileHandle, &hashSize)
	    || (hashSize == serializeMulti->hash.Bytes && !WZ_PHYSFS_readBytes(fileHandle, serializeMulti->hash.bytes, serializeMulti->hash.Bytes))
	    || !PHYSFS_readUBE16(fileHandle, &dummy16)	// dummy, was bytesPerSec
	    || !PHYSFS_readUBE8(fileHandle, &dummy8)	// dummy, was packetsPerSec
	    || !PHYSFS_readUBE8(fileHandle, &dummy8))	// reused for challenge, was encryptKey
	{
		return false;
	}
	challengeActive = dummy8;	// hack

	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		// dummy, was `skDiff` for each player
		if (!PHYSFS_readUBE8(fileHandle, &dummy8))
		{
			return false;
		}
	}

	return true;
}
static void serializePlayer_json(nlohmann::json &o, const PLAYER *serializePlayer, int player)
{
	o["position"] = serializePlayer->position;
	o["name"] = serializePlayer->name;
	o["aiName"] = getAIName(player);
	o["difficulty"] = static_cast<int8_t>(serializePlayer->difficulty);
	o["allocated"] = (uint8_t)serializePlayer->allocated;
	o["colour"] = serializePlayer->colour;
	o["team"] = serializePlayer->team;
}

static bool serializePlayer(PHYSFS_file *fileHandle, const PLAYER *serializePlayer, int player)
{
	return (PHYSFS_writeUBE32(fileHandle, serializePlayer->position)
	        && WZ_PHYSFS_writeBytes(fileHandle, serializePlayer->name, StringSize) == StringSize
	        && WZ_PHYSFS_writeBytes(fileHandle, getAIName(player), MAX_LEN_AI_NAME) == MAX_LEN_AI_NAME
	        && PHYSFS_writeSBE8(fileHandle, static_cast<int8_t>(serializePlayer->difficulty))
	        && PHYSFS_writeUBE8(fileHandle, (uint8_t)serializePlayer->allocated)
	        && PHYSFS_writeUBE32(fileHandle, serializePlayer->colour)
	        && PHYSFS_writeUBE32(fileHandle, serializePlayer->team));
}
static void deserializePlayer_json(const nlohmann::json &o, PLAYER *serializePlayer, int player)
{
	char aiName[MAX_LEN_AI_NAME] = { "THEREISNOAI" };
	ASSERT(o.is_object(), "unexpected type, wanted object");
	sstrcpy(serializePlayer->name, o.at("name").get<std::string>().c_str());
	sstrcpy(aiName, o.at("aiName").get<std::string>().c_str());
	serializePlayer->difficulty = static_cast<AIDifficulty>(o.at("difficulty").get<int8_t>());
	serializePlayer->allocated = o.at("allocated").get<uint8_t>();
	if (player < game.maxPlayers)
	{
		serializePlayer->ai = matchAIbyName(aiName);
		ASSERT(serializePlayer->ai != AI_NOT_FOUND, "AI \"%s\" not found -- script loading will fail (player %d / %d)",
				aiName, player, game.maxPlayers);
	}
	serializePlayer->position = o.at("position").get<uint32_t>();
	serializePlayer->colour = o.at("colour").get<uint32_t>();
	serializePlayer->team = o.at("team").get<uint32_t>();
}
static bool deserializePlayer(PHYSFS_file *fileHandle, PLAYER *serializePlayer, int player)
{
	char aiName[MAX_LEN_AI_NAME] = { "THEREISNOAI" };
	uint32_t position = 0, colour = 0, team = 0;
	bool retval = false;
	uint8_t allocated = 0;

	retval = (PHYSFS_readUBE32(fileHandle, &position)
	          && WZ_PHYSFS_readBytes(fileHandle, serializePlayer->name, StringSize) == StringSize
	          && WZ_PHYSFS_readBytes(fileHandle, aiName, MAX_LEN_AI_NAME) == MAX_LEN_AI_NAME
	          && PHYSFS_readSBE8(fileHandle, reinterpret_cast<int8_t*>(&serializePlayer->difficulty))
	          && PHYSFS_readUBE8(fileHandle, &allocated)
	          && PHYSFS_readUBE32(fileHandle, &colour)
	          && PHYSFS_readUBE32(fileHandle, &team));

	serializePlayer->allocated = allocated;
	if (player < game.maxPlayers)
	{
		serializePlayer->ai = matchAIbyName(aiName);
		ASSERT(serializePlayer->ai != AI_NOT_FOUND, "AI \"%s\" not found -- script loading will fail (player %d / %d)",
		       aiName, player, game.maxPlayers);
	}
	serializePlayer->position = position;
	serializePlayer->colour = colour;
	serializePlayer->team = team;
	return retval;
}
static void serializeNetPlay_json(nlohmann::json &o, const NETPLAY *serializeNetPlay)
{
	auto arr = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		auto tmp = nlohmann::json::object();
		serializePlayer_json(tmp, &serializeNetPlay->players[i], i);
		arr.insert(arr.end(), tmp);
	}
	o["netbComms"] = serializeNetPlay->bComms;
	o["netPlayerCount"] = serializeNetPlay->playercount;
	o["netHostPlayer"] = serializeNetPlay->hostPlayer;
	o["netSelectedPlayer"] = selectedPlayer;
	o["netScavengers"] = game.scavengers;
	// skip dummy
	o["netPlayers"] = arr;
}
static bool serializeNetPlay(PHYSFS_file *fileHandle, const NETPLAY *serializeNetPlay)
{
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializePlayer(fileHandle, &serializeNetPlay->players[i], i))
		{
			return false;
		}
	}

	return (PHYSFS_writeUBE32(fileHandle, (uint32_t)serializeNetPlay->bComms)
	        && PHYSFS_writeUBE32(fileHandle, serializeNetPlay->playercount)
	        && PHYSFS_writeUBE32(fileHandle, serializeNetPlay->hostPlayer)
	        && PHYSFS_writeUBE32(fileHandle, selectedPlayer)
	        && PHYSFS_writeUBE32(fileHandle, (uint32_t)game.scavengers)
	        && PHYSFS_writeUBE32(fileHandle, 0)
	        && PHYSFS_writeUBE32(fileHandle, 0));
}

static void deserializeNetPlay_json(const nlohmann::json &o, NETPLAY *serializeNetPlay)
{
	const auto players = o.at("netPlayers");
	ASSERT_OR_RETURN(, players.is_array(), "unexpected type, wanted array");
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		deserializePlayer_json(players.at(i), &serializeNetPlay->players[i], i);
	}
	serializeNetPlay->isHost = true; // only host can load
	serializeNetPlay->playercount = o.at("netPlayerCount").get<uint32_t>();
	serializeNetPlay->bComms = o.at("netbComms").get<bool>();
	selectedPlayer = o.at("netSelectedPlayer").get<uint32_t>();
	game.scavengers = o.at("netScavengers").get<uint8_t>();
}
static bool deserializeNetPlay(PHYSFS_file *fileHandle, NETPLAY *serializeNetPlay)
{
	unsigned int i;
	bool retv;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializePlayer(fileHandle, &serializeNetPlay->players[i], i))
		{
			return false;
		}
	}

	uint32_t dummy, bComms = serializeNetPlay->bComms, scavs = game.scavengers;

	serializeNetPlay->isHost = true;	// only host can load
	retv = (PHYSFS_readUBE32(fileHandle, &bComms)
	        && PHYSFS_readUBE32(fileHandle, &serializeNetPlay->playercount)
	        && PHYSFS_readUBE32(fileHandle, &serializeNetPlay->hostPlayer)
	        && PHYSFS_readUBE32(fileHandle, &selectedPlayer)
	        && PHYSFS_readUBE32(fileHandle, &scavs)
	        && PHYSFS_readUBE32(fileHandle, &dummy)
	        && PHYSFS_readUBE32(fileHandle, &dummy));
	serializeNetPlay->bComms = bComms;
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

static void serializeSaveGameV7Data_json(nlohmann::json &o, const SAVE_GAME_V7 *serializeGame)
{
	o["gameTime"] = serializeGame->gameTime;
	o["GameType"] = serializeGame->GameType;
	o["ScrollMinX"] = serializeGame->ScrollMinX;
	o["ScrollMinY"] = serializeGame->ScrollMinY;
	o["ScrollMaxX"] = serializeGame->ScrollMaxX;
	o["ScrollMaxY"] = serializeGame->ScrollMaxY;
	o["levelName"] = serializeGame->levelName;
}

static bool serializeSaveGameV7Data(PHYSFS_file *fileHandle, const SAVE_GAME_V7 *serializeGame)
{
	return (PHYSFS_writeUBE32(fileHandle, serializeGame->gameTime)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->GameType)
	        && PHYSFS_writeSBE32(fileHandle, serializeGame->ScrollMinX)
	        && PHYSFS_writeSBE32(fileHandle, serializeGame->ScrollMinY)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->ScrollMaxX)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->ScrollMaxY)
	        && WZ_PHYSFS_writeBytes(fileHandle, serializeGame->levelName, MAX_LEVEL_SIZE) == MAX_LEVEL_SIZE);
}
static void deserializeSaveGameV7Data_json(const nlohmann::json &o, SAVE_GAME_V7 *serializeGame)
{
	serializeGame->gameTime = o.at("gameTime").get<uint32_t>();
	serializeGame->GameType = o.at("GameType").get<uint32_t>();
	serializeGame->ScrollMinX = o.at("ScrollMinX").get<int32_t>();
	serializeGame->ScrollMinY = o.at("ScrollMinY").get<int32_t>();
	serializeGame->ScrollMaxX = o.at("ScrollMaxX").get<uint32_t>();
	serializeGame->ScrollMaxY = o.at("ScrollMaxY").get<uint32_t>();
	sstrcpy(serializeGame->levelName, o.at("levelName").get<std::string>().c_str());
}

static bool deserializeSaveGameV7Data(PHYSFS_file *fileHandle, SAVE_GAME_V7 *serializeGame)
{
	return (PHYSFS_readUBE32(fileHandle, &serializeGame->gameTime)
	        && PHYSFS_readUBE32(fileHandle, &serializeGame->GameType)
	        && PHYSFS_readSBE32(fileHandle, &serializeGame->ScrollMinX)
	        && PHYSFS_readSBE32(fileHandle, &serializeGame->ScrollMinY)
	        && PHYSFS_readUBE32(fileHandle, &serializeGame->ScrollMaxX)
	        && PHYSFS_readUBE32(fileHandle, &serializeGame->ScrollMaxY)
	        && WZ_PHYSFS_readBytes(fileHandle, serializeGame->levelName, MAX_LEVEL_SIZE) == MAX_LEVEL_SIZE);
}

struct SAVE_GAME_V10 : public SAVE_GAME_V7
{
	SAVE_POWER  power[MAX_PLAYERS];
};

static void serializeSaveGameV10Data_json(nlohmann::json &o, const SAVE_GAME_V10 *serializeGame)
{
	serializeSaveGameV7Data_json(o, (const SAVE_GAME_V7 *) serializeGame);
	auto arr = nlohmann::json::array();
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		auto tmp = nlohmann::json::object();
		serializeSavePowerData_json(tmp, &serializeGame->power[i]);
		arr.insert(arr.end(), tmp);
	}
	o["power"] = arr;
}
static bool serializeSaveGameV10Data(PHYSFS_file *fileHandle, const SAVE_GAME_V10 *serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV7Data(fileHandle, (const SAVE_GAME_V7 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializeSavePowerData(fileHandle, &serializeGame->power[i]))
		{
			return false;
		}
	}

	return true;
}
static void deserializeSaveGameV10Data_json(const nlohmann::json &o, SAVE_GAME_V10 *serializeGame)
{
	deserializeSaveGameV7Data_json(o,  (SAVE_GAME_V7 *) serializeGame);
	nlohmann::json power= o.at("power");
	ASSERT(power.is_array(), "unexpected type");
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		deserializeSavePowerData_json(power.at(i), &serializeGame->power[i]);
	}
}
static bool deserializeSaveGameV10Data(PHYSFS_file *fileHandle, SAVE_GAME_V10 *serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV7Data(fileHandle, (SAVE_GAME_V7 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializeSavePowerData(fileHandle, &serializeGame->power[i]))
		{
			return false;
		}
	}

	return true;
}

struct SAVE_GAME_V11 : public SAVE_GAME_V10
{
	iView currentPlayerPos;
};

static void serializeSaveGameV11Data_json(nlohmann::json &o, const SAVE_GAME_V11 *serializeGame)
{
	serializeSaveGameV10Data_json(o, (const SAVE_GAME_V10 *) serializeGame);
	serializeiViewData_json(o, &serializeGame->currentPlayerPos);

}

static bool serializeSaveGameV11Data(PHYSFS_file *fileHandle, const SAVE_GAME_V11 *serializeGame)
{
	return (serializeSaveGameV10Data(fileHandle, (const SAVE_GAME_V10 *) serializeGame)
	        && serializeiViewData(fileHandle, &serializeGame->currentPlayerPos));
}
static void deserializeSaveGameV11Data_json(const nlohmann::json &o, SAVE_GAME_V11 *serializeGame)
{
	deserializeSaveGameV10Data_json(o, (SAVE_GAME_V10 *) serializeGame);
	deserializeiViewData_json(o, &serializeGame->currentPlayerPos);
}
static bool deserializeSaveGameV11Data(PHYSFS_file *fileHandle, SAVE_GAME_V11 *serializeGame)
{
	return (deserializeSaveGameV10Data(fileHandle, (SAVE_GAME_V10 *) serializeGame)
	        && deserializeiViewData(fileHandle, &serializeGame->currentPlayerPos));
}

struct SAVE_GAME_V12 : public SAVE_GAME_V11
{
	uint32_t    missionTime;
	uint32_t    saveKey;
};
static void serializeSaveGameV12Data_json(nlohmann::json &o, const SAVE_GAME_V12 *serializeGame)
{
	serializeSaveGameV11Data_json(o,  (const SAVE_GAME_V11 *) serializeGame);
	o["missionTime"] = serializeGame->missionTime;
	o["saveKey"] = serializeGame->saveKey;
}
static bool serializeSaveGameV12Data(PHYSFS_file *fileHandle, const SAVE_GAME_V12 *serializeGame)
{
	return (serializeSaveGameV11Data(fileHandle, (const SAVE_GAME_V11 *) serializeGame)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->missionTime)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->saveKey));
}

static void deserializeSaveGameV12Data_json(const nlohmann::json &o, SAVE_GAME_V12 *serializeGame)
{
	deserializeSaveGameV11Data_json(o, (SAVE_GAME_V11 *) serializeGame);
	serializeGame->missionTime = o.at("missionTime").get<uint32_t>();
	serializeGame->saveKey = o.at("saveKey").get<uint32_t>();
}

static bool deserializeSaveGameV12Data(PHYSFS_file *fileHandle, SAVE_GAME_V12 *serializeGame)
{
	return (deserializeSaveGameV11Data(fileHandle, (SAVE_GAME_V11 *) serializeGame)
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
static void serializeSaveGameV14Data_json(nlohmann::json &o, const SAVE_GAME_V14 *serializeGame)
{
	serializeSaveGameV12Data_json(o, (const SAVE_GAME_V12 *) serializeGame);
	o["missionOffTime"] = serializeGame->missionOffTime;
	o["missionETA"] = serializeGame->missionETA;
	o["missionHomeLZ_X"] = serializeGame->missionHomeLZ_X;
	o["missionHomeLZ_Y"] = serializeGame->missionHomeLZ_Y;
	o["missionPlayerX"] = serializeGame->missionPlayerX;
	o["missionPlayerY"] = serializeGame->missionPlayerY;
	auto arr = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		auto tmp = nlohmann::json::object();
		tmp["iTranspEntryTileX"] =  serializeGame->iTranspEntryTileX[i];
		tmp["iTranspEntryTileY"] =  serializeGame->iTranspEntryTileY[i];
		tmp["iTranspExitTileX"] = serializeGame->iTranspExitTileX[i];
		tmp["iTranspExitTileY"] = serializeGame->iTranspExitTileY[i];
		tmp["aDefaultSensor"] = serializeGame->aDefaultSensor[i];
		tmp["aDefaultECM"] = serializeGame->aDefaultECM[i];
		tmp["aDefaultRepair"] = serializeGame->aDefaultRepair[i];
		arr.insert(arr.end(), tmp);
	}
	o["data"] = arr;
}
static bool serializeSaveGameV14Data(PHYSFS_file *fileHandle, const SAVE_GAME_V14 *serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV12Data(fileHandle, (const SAVE_GAME_V12 *) serializeGame)
	    || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionOffTime)
	    || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionETA)
	    || !PHYSFS_writeUBE16(fileHandle, serializeGame->missionHomeLZ_X)
	    || !PHYSFS_writeUBE16(fileHandle, serializeGame->missionHomeLZ_Y)
	    || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionPlayerX)
	    || !PHYSFS_writeSBE32(fileHandle, serializeGame->missionPlayerY))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspEntryTileX[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspEntryTileY[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspExitTileX[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE16(fileHandle, serializeGame->iTranspExitTileY[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->aDefaultSensor[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->aDefaultECM[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->aDefaultRepair[i]))
		{
			return false;
		}
	}

	return true;
}
static void deserializeSaveGameV14Data_json(const nlohmann::json &o, SAVE_GAME_V14 *serializeGame)
{
	deserializeSaveGameV12Data_json(o, (SAVE_GAME_V12 *) serializeGame);
	 serializeGame->missionOffTime = o.at("missionOffTime").get<int32_t>();
	 serializeGame->missionETA = o.at("missionETA").get<int32_t>();
	 serializeGame->missionHomeLZ_X = o.at("missionHomeLZ_X").get<uint32_t>();
	 serializeGame->missionHomeLZ_Y = o.at("missionHomeLZ_Y").get<uint32_t>();
	 serializeGame->missionPlayerX = o.at("missionPlayerX").get<int32_t>();
	 serializeGame->missionPlayerY = o.at("missionPlayerY").get<int32_t>();
	nlohmann::json arr = o.at("data");
	ASSERT_OR_RETURN(, arr.is_array(), "unexpected type, wanted array");
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		serializeGame->iTranspEntryTileX[i] = o.at("data").at(i).at("iTranspEntryTileX").get<uint16_t>();
		serializeGame->iTranspEntryTileY[i] = o.at("data").at(i).at("iTranspEntryTileY").get<uint16_t>();
		serializeGame->iTranspExitTileX[i] = o.at("data").at(i).at("iTranspExitTileX").get<uint16_t>();
		serializeGame->iTranspExitTileY[i] = o.at("data").at(i).at("iTranspExitTileY").get<uint16_t>();
		serializeGame->aDefaultSensor[i] = o.at("data").at(i).at("aDefaultSensor").get<uint32_t>();
		serializeGame->aDefaultECM[i] = o.at("data").at(i).at("aDefaultECM").get<uint32_t>();
		serializeGame->aDefaultRepair[i] = o.at("data").at(i).at("aDefaultRepair").get<uint32_t>();
	}
}
static bool deserializeSaveGameV14Data(PHYSFS_file *fileHandle, SAVE_GAME_V14 *serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV12Data(fileHandle, (SAVE_GAME_V12 *) serializeGame)
	    || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionOffTime)
	    || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionETA)
	    || !PHYSFS_readUBE16(fileHandle, &serializeGame->missionHomeLZ_X)
	    || !PHYSFS_readUBE16(fileHandle, &serializeGame->missionHomeLZ_Y)
	    || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionPlayerX)
	    || !PHYSFS_readSBE32(fileHandle, &serializeGame->missionPlayerY))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspEntryTileX[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspEntryTileY[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspExitTileX[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE16(fileHandle, &serializeGame->iTranspExitTileY[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->aDefaultSensor[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->aDefaultECM[i]))
		{
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->aDefaultRepair[i]))
		{
			return false;
		}
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

static void serializeSaveGameV15Data_json(nlohmann::json &o, const SAVE_GAME_V15 *serializeGame)
{
	serializeSaveGameV14Data_json(o, (const SAVE_GAME_V14 *) serializeGame);
	o["offWorldKeepLists"] = serializeGame->offWorldKeepLists;
	o["RubbleTile"] = serializeGame->RubbleTile;
	o["WaterTile"] = serializeGame->WaterTile;

}
static bool serializeSaveGameV15Data(PHYSFS_file *fileHandle, const SAVE_GAME_V15 *serializeGame)
{
	unsigned int i, j;

	if (!serializeSaveGameV14Data(fileHandle, (const SAVE_GAME_V14 *) serializeGame)
	    || !PHYSFS_writeSBE32(fileHandle, serializeGame->offWorldKeepLists))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			if (!PHYSFS_writeUBE8(fileHandle, 0)) // no longer saved in binary form
			{
				return false;
			}
		}
	}

	return (PHYSFS_writeUBE32(fileHandle, serializeGame->RubbleTile)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->WaterTile)
	        && PHYSFS_writeUBE32(fileHandle, 0)
	        && PHYSFS_writeUBE32(fileHandle, 0));
}
static void deserializeSaveGameV15Data_json(const nlohmann::json &o, SAVE_GAME_V15 *serializeGame)
{
	deserializeSaveGameV14Data_json(o, (SAVE_GAME_V14 *) serializeGame);
	serializeGame->offWorldKeepLists = o.at("offWorldKeepLists").get<int32_t>();
	serializeGame->RubbleTile = o.at("RubbleTile").get<uint32_t>();
	serializeGame->WaterTile = o.at("WaterTile").get<uint32_t>();
	serializeGame->fogColour = 0;
	serializeGame->fogState = 0;
}
static bool deserializeSaveGameV15Data(PHYSFS_file *fileHandle, SAVE_GAME_V15 *serializeGame)
{
	unsigned int i, j;
	int32_t boolOffWorldKeepLists;

	if (!deserializeSaveGameV14Data(fileHandle, (SAVE_GAME_V14 *) serializeGame)
	    || !PHYSFS_readSBE32(fileHandle, &boolOffWorldKeepLists))
	{
		return false;
	}

	serializeGame->offWorldKeepLists = boolOffWorldKeepLists;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			uint8_t tmp;
			if (!PHYSFS_readUBE8(fileHandle, &tmp))
			{
				return false;
			}
			if (tmp > 0)
			{
				add_to_experience_queue(i, tmp * 65536);
			}
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

static void serializeSaveGameV16Data_json(nlohmann::json &o, const SAVE_GAME_V16 *serializeGame)
{
	serializeSaveGameV15Data_json(o, (const SAVE_GAME_V15 *) serializeGame);
	auto arr = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		auto tmp = nlohmann::json::object();
		serializeLandingZoneData_json(tmp, &serializeGame->sLandingZone[i]);
		arr.insert(arr.end(), tmp);
	}
	o["landingZones"] = arr;
}

static bool serializeSaveGameV16Data(PHYSFS_file *fileHandle, const SAVE_GAME_V16 *serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV15Data(fileHandle, (const SAVE_GAME_V15 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		if (!serializeLandingZoneData(fileHandle, &serializeGame->sLandingZone[i]))
		{
			return false;
		}
	}

	return true;
}

static void deserializeSaveGameV16Data_json(const nlohmann::json &o, SAVE_GAME_V16 *serializeGame)
{
	deserializeSaveGameV15Data_json(o, (SAVE_GAME_V15 *) serializeGame);
	for (unsigned i = 0; i < MAX_NOGO_AREAS; ++i)
	{		
		deserializeLandingZoneData_json(o.at("landingZones").at(i), &serializeGame->sLandingZone[i]);
	}
}

static bool deserializeSaveGameV16Data(PHYSFS_file *fileHandle, SAVE_GAME_V16 *serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV15Data(fileHandle, (SAVE_GAME_V15 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		if (!deserializeLandingZoneData(fileHandle, &serializeGame->sLandingZone[i]))
		{
			return false;
		}
	}

	return true;
}

struct SAVE_GAME_V17 : public SAVE_GAME_V16
{
	uint32_t    objId;
};

static void serializeSaveGameV17Data_json(nlohmann::json &o, const SAVE_GAME_V17 *serializeGame)
{
	serializeSaveGameV16Data_json(o, (const SAVE_GAME_V16 *) serializeGame);
	o["objId"] = serializeGame->objId;
}

static bool serializeSaveGameV17Data(PHYSFS_file *fileHandle, const SAVE_GAME_V17 *serializeGame)
{
	return (serializeSaveGameV16Data(fileHandle, (const SAVE_GAME_V16 *) serializeGame)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->objId));
}

static void deserializeSaveGameV17Data_json(const nlohmann::json &o, SAVE_GAME_V17 *serializeGame)
{
	deserializeSaveGameV16Data_json(o, (SAVE_GAME_V16 *) serializeGame);
	serializeGame->objId = o.at("objId").get<uint32_t>();
}

static bool deserializeSaveGameV17Data(PHYSFS_file *fileHandle, SAVE_GAME_V17 *serializeGame)
{
	return (deserializeSaveGameV16Data(fileHandle, (SAVE_GAME_V16 *) serializeGame)
	        && PHYSFS_readUBE32(fileHandle, &serializeGame->objId));
}

struct SAVE_GAME_V18 : public SAVE_GAME_V17
{
	char        buildDate[MAX_STR_LENGTH];
	uint32_t    oldestVersion;
	uint32_t    validityKey;
};
static void serializeSaveGameV18Data_json(nlohmann::json &o, const SAVE_GAME_V18 *serializeGame)
{
	serializeSaveGameV17Data_json(o, (const SAVE_GAME_V17 *) serializeGame);
	o["buildDate"] = serializeGame->buildDate;
	o["oldestVersion"] = serializeGame->oldestVersion;
	o["validityKey"] = serializeGame->validityKey;
}
static bool serializeSaveGameV18Data(PHYSFS_file *fileHandle, const SAVE_GAME_V18 *serializeGame)
{
	return (serializeSaveGameV17Data(fileHandle, (const SAVE_GAME_V17 *) serializeGame)
	        && WZ_PHYSFS_writeBytes(fileHandle, serializeGame->buildDate, MAX_STR_LENGTH) == MAX_STR_LENGTH
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->oldestVersion)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->validityKey));
}

static void deserializeSaveGameV18Data_json(const nlohmann::json &o, SAVE_GAME_V18 *serializeGame)
{
	deserializeSaveGameV17Data_json(o, (SAVE_GAME_V17 *) serializeGame);
	sstrcpy(serializeGame->buildDate, o.at("buildDate").get<std::string>().c_str());
	serializeGame->oldestVersion = o.at("oldestVersion").get<uint32_t>();
	serializeGame->validityKey = o.at("validityKey").get<uint32_t>();
}

static bool deserializeSaveGameV18Data(PHYSFS_file *fileHandle, SAVE_GAME_V18 *serializeGame)
{
	return (deserializeSaveGameV17Data(fileHandle, (SAVE_GAME_V17 *) serializeGame)
	        && WZ_PHYSFS_readBytes(fileHandle, serializeGame->buildDate, MAX_STR_LENGTH) == MAX_STR_LENGTH
	        && PHYSFS_readUBE32(fileHandle, &serializeGame->oldestVersion)
	        && PHYSFS_readUBE32(fileHandle, &serializeGame->validityKey));
}

struct SAVE_GAME_V19 : public SAVE_GAME_V18
{
	uint8_t     alliances[MAX_PLAYERS][MAX_PLAYERS];
	uint8_t     playerColour[MAX_PLAYERS];
	uint8_t     radarZoom;
};

static void serializeSaveGameV19Data_json(nlohmann::json &o, const SAVE_GAME_V19 *serializeGame)
{
	serializeSaveGameV18Data_json(o, (const SAVE_GAME_V18 *) serializeGame);
	auto alliancesArray = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		for (unsigned j = 0; j < MAX_PLAYERS; ++j)
		{
			alliancesArray.insert(alliancesArray.end(), serializeGame->alliances[i][j]);
		}
	}
	o["alliances"] = alliancesArray;

	auto colours = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		colours.insert(colours.end(), serializeGame->playerColour[i]);
	}
	o["colours"] = colours;
	o["radarZoom"] = serializeGame->radarZoom;
}

static bool serializeSaveGameV19Data(PHYSFS_file *fileHandle, const SAVE_GAME_V19 *serializeGame)
{
	unsigned int i, j;

	if (!serializeSaveGameV18Data(fileHandle, (const SAVE_GAME_V18 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_PLAYERS; ++j)
		{
			if (!PHYSFS_writeUBE8(fileHandle, serializeGame->alliances[i][j]))
			{
				return false;
			}
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE8(fileHandle, serializeGame->playerColour[i]))
		{
			return false;
		}
	}

	return PHYSFS_writeUBE8(fileHandle, serializeGame->radarZoom);
}

static void deserializeSaveGameV19Data_json(const nlohmann::json &o, SAVE_GAME_V19 *serializeGame)
{
	deserializeSaveGameV18Data_json(o, (SAVE_GAME_V18 *) serializeGame);
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		for (unsigned j = 0; j < MAX_PLAYERS; ++j)
		{
			serializeGame->alliances[i][j] = o.at("alliances").at(i * MAX_PLAYERS + j).get<uint8_t>();
		}
		serializeGame->playerColour[i] = o.at("colours").at(i).get<uint8_t>();
	}
	serializeGame->radarZoom = o.at("radarZoom").get<uint8_t>();
}

static bool deserializeSaveGameV19Data(PHYSFS_file *fileHandle, SAVE_GAME_V19 *serializeGame)
{
	unsigned int i, j;

	if (!deserializeSaveGameV18Data(fileHandle, (SAVE_GAME_V18 *) serializeGame))
	{
		return false;
	}

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
		{
			return false;
		}
	}

	return PHYSFS_readUBE8(fileHandle, &serializeGame->radarZoom);
}

struct SAVE_GAME_V20 : public SAVE_GAME_V19
{
	uint8_t     bDroidsToSafetyFlag;
	Vector2i    asVTOLReturnPos[MAX_PLAYERS];
};
static void serializeSaveGameV20Data_json(nlohmann::json &o, const SAVE_GAME_V20 *serializeGame)
{
	serializeSaveGameV19Data_json(o, (const SAVE_GAME_V19 *) serializeGame);
	o["bDroidsToSafetyFlag"] = serializeGame->bDroidsToSafetyFlag;
	auto arr = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		auto tmp = nlohmann::json::object();
		serializeVector2i_json(tmp, &serializeGame->asVTOLReturnPos[i]);
		arr.insert(arr.end(), tmp);
	}
	o["VTOLReturnPos"] = arr;
}
static bool serializeSaveGameV20Data(PHYSFS_file *fileHandle, const SAVE_GAME_V20 *serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV19Data(fileHandle, (const SAVE_GAME_V19 *) serializeGame)
	    || !PHYSFS_writeUBE8(fileHandle, serializeGame->bDroidsToSafetyFlag))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializeVector2i(fileHandle, &serializeGame->asVTOLReturnPos[i]))
		{
			return false;
		}
	}

	return true;
}

static void deserializeSaveGameV20Data_json(const nlohmann::json &o, SAVE_GAME_V20 *serializeGame)
{
	deserializeSaveGameV19Data_json(o, (SAVE_GAME_V19 *) serializeGame);
	serializeGame->bDroidsToSafetyFlag = o.at("bDroidsToSafetyFlag").get<uint8_t>();

	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		deserializeVector2i_json(o.at("VTOLReturnPos").at(i), &serializeGame->asVTOLReturnPos[i]);
	}
}

static bool deserializeSaveGameV20Data(PHYSFS_file *fileHandle, SAVE_GAME_V20 *serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV19Data(fileHandle, (SAVE_GAME_V19 *) serializeGame)
	    || !PHYSFS_readUBE8(fileHandle, &serializeGame->bDroidsToSafetyFlag))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializeVector2i(fileHandle, &serializeGame->asVTOLReturnPos[i]))
		{
			return false;
		}
	}

	return true;
}

struct SAVE_GAME_V22 : public SAVE_GAME_V20
{
	RUN_DATA asRunData[MAX_PLAYERS];
};
static void serializeSaveGameV22Data_json(nlohmann::json &o, const SAVE_GAME_V22 *serializeGame)
{
	serializeSaveGameV20Data_json(o, (const SAVE_GAME_V20 *) serializeGame);
	auto arr = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		auto tmp = nlohmann::json::object();
		serializeRunData_json(tmp, &serializeGame->asRunData[i]);
		arr.insert(arr.end(), tmp);
	}
	o["runData"] = arr;
}
static bool serializeSaveGameV22Data(PHYSFS_file *fileHandle, const SAVE_GAME_V22 *serializeGame)
{
	unsigned int i;

	if (!serializeSaveGameV20Data(fileHandle, (const SAVE_GAME_V20 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!serializeRunData(fileHandle, &serializeGame->asRunData[i]))
		{
			return false;
		}
	}

	return true;
}

static void deserializeSaveGameV22Data_json(const nlohmann::json &o, SAVE_GAME_V22 *serializeGame)
{
	deserializeSaveGameV20Data_json(o, (SAVE_GAME_V20 *) serializeGame);
	ASSERT_OR_RETURN(, o.at("runData").is_array(), "unexpected type, wanted array");
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		const nlohmann::json runData = o.at("runData");
		ASSERT_OR_RETURN(, runData.is_array(), "unexpected type, wanted array");
		deserializeRunData_json(runData.at(i), &serializeGame->asRunData[i]);
		
	}
}

static bool deserializeSaveGameV22Data(PHYSFS_file *fileHandle, SAVE_GAME_V22 *serializeGame)
{
	unsigned int i;

	if (!deserializeSaveGameV20Data(fileHandle, (SAVE_GAME_V20 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!deserializeRunData(fileHandle, &serializeGame->asRunData[i]))
		{
			return false;
		}
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

static void serializeSaveGameV24Data_json(nlohmann::json &o, const SAVE_GAME_V24 *serializeGame)
{
	serializeSaveGameV22Data_json(o, (const SAVE_GAME_V22 *) serializeGame);
	o["reinforceTime"] = serializeGame->reinforceTime;
	o["bPlayCountDown"] = serializeGame->bPlayCountDown;
	o["bPlayerHasWon"] = serializeGame->bPlayerHasWon;
	o["bPlayerHasLost"] = serializeGame->bPlayerHasLost;
}
static bool serializeSaveGameV24Data(PHYSFS_file *fileHandle, const SAVE_GAME_V24 *serializeGame)
{
	return (serializeSaveGameV22Data(fileHandle, (const SAVE_GAME_V22 *) serializeGame)
	        && PHYSFS_writeUBE32(fileHandle, serializeGame->reinforceTime)
	        && PHYSFS_writeUBE8(fileHandle, serializeGame->bPlayCountDown)
	        && PHYSFS_writeUBE8(fileHandle, serializeGame->bPlayerHasWon)
	        && PHYSFS_writeUBE8(fileHandle, serializeGame->bPlayerHasLost)
	        && PHYSFS_writeUBE8(fileHandle, serializeGame->dummy3));
}
static void deserializeSaveGameV24Data_json(const nlohmann::json &o, SAVE_GAME_V24 *serializeGame)
{
	deserializeSaveGameV22Data_json(o, (SAVE_GAME_V22 *) serializeGame);
	serializeGame->reinforceTime = o.at("reinforceTime").get<uint32_t>();
	serializeGame->bPlayCountDown = o.at("bPlayCountDown").get<uint8_t>();
	serializeGame->bPlayerHasWon = o.at("bPlayerHasWon").get<uint8_t>();
	serializeGame->bPlayerHasLost = o.at("bPlayerHasLost").get<uint8_t>();
}
static bool deserializeSaveGameV24Data(PHYSFS_file *fileHandle, SAVE_GAME_V24 *serializeGame)
{
	return (deserializeSaveGameV22Data(fileHandle, (SAVE_GAME_V22 *) serializeGame)
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

static void serializeSaveGameV27Data_json(nlohmann::json &o,const SAVE_GAME_V27 *serializeGame)
{
	serializeSaveGameV24Data_json(o, (const SAVE_GAME_V24 *) serializeGame);
}
static bool serializeSaveGameV27Data(PHYSFS_file *fileHandle, const SAVE_GAME_V27 *serializeGame)
{
	unsigned int i, j;

	if (!serializeSaveGameV24Data(fileHandle, (const SAVE_GAME_V24 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			if (!PHYSFS_writeUBE16(fileHandle, 0))
			{
				return false;
			}
		}
	}

	return true;
}
static void deserializeSaveGameV27Data_json(const nlohmann::json &o, SAVE_GAME_V27 *serializeGame)
{
	deserializeSaveGameV24Data_json(o, (SAVE_GAME_V24 *) serializeGame);
}
static bool deserializeSaveGameV27Data(PHYSFS_file *fileHandle, SAVE_GAME_V27 *serializeGame)
{
	unsigned int i, j;

	if (!deserializeSaveGameV24Data(fileHandle, (SAVE_GAME_V24 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		for (j = 0; j < MAX_RECYCLED_DROIDS; ++j)
		{
			uint16_t tmp;
			if (!PHYSFS_readUBE16(fileHandle, &tmp))
			{
				return false;
			}
			if (tmp > 0)
			{
				add_to_experience_queue(i, tmp * 65536);
			}
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
static void serializeSaveGameV29Data_json(nlohmann::json &o, const SAVE_GAME_V29 *serializeGame)
{
	serializeSaveGameV27Data_json(o, (const SAVE_GAME_V27 *) serializeGame);
	o["missionScrollMinX"] = serializeGame->missionScrollMinX;
	o["missionScrollMinY"] = serializeGame->missionScrollMinY;
	o["missionScrollMaxX"] = serializeGame->missionScrollMaxX;
	o["missionScrollMaxY"] = serializeGame->missionScrollMaxY;
}
static bool serializeSaveGameV29Data(PHYSFS_file *fileHandle, const SAVE_GAME_V29 *serializeGame)
{
	return (serializeSaveGameV27Data(fileHandle, (const SAVE_GAME_V27 *) serializeGame)
	        && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMinX)
	        && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMinY)
	        && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMaxX)
	        && PHYSFS_writeUBE16(fileHandle, serializeGame->missionScrollMaxY));
}

static void deserializeSaveGameV29Data_json(const nlohmann::json &o, SAVE_GAME_V29 *serializeGame)
{
	deserializeSaveGameV27Data_json(o, (SAVE_GAME_V27 *) serializeGame);
	serializeGame->missionScrollMinX = o.at("missionScrollMinX").get<uint16_t>();
	serializeGame->missionScrollMinY = o.at("missionScrollMinY").get<uint16_t>();
	serializeGame->missionScrollMaxX = o.at("missionScrollMaxX").get<uint16_t>();
	serializeGame->missionScrollMaxY = o.at("missionScrollMaxY").get<uint16_t>();
}

static bool deserializeSaveGameV29Data(PHYSFS_file *fileHandle, SAVE_GAME_V29 *serializeGame)
{
	return (deserializeSaveGameV27Data(fileHandle, (SAVE_GAME_V27 *) serializeGame)
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
static void serializeSaveGameV30Data_json(nlohmann::json &o, const SAVE_GAME_V30 *serializeGame)
{
	serializeSaveGameV29Data_json(o, (const SAVE_GAME_V29 *) serializeGame);
	o["scrGameLevel"] = serializeGame->scrGameLevel;
	o["bExtraVictoryFlag"] = serializeGame->bExtraVictoryFlag;
	o["bExtraFailFlag"] = serializeGame->bExtraFailFlag;
	o["bTrackTransporter"] = serializeGame->bTrackTransporter;
}
static bool serializeSaveGameV30Data(PHYSFS_file *fileHandle, const SAVE_GAME_V30 *serializeGame)
{
	return (serializeSaveGameV29Data(fileHandle, (const SAVE_GAME_V29 *) serializeGame)
	        && PHYSFS_writeSBE32(fileHandle, serializeGame->scrGameLevel)
	        && PHYSFS_writeUBE8(fileHandle, serializeGame->bExtraVictoryFlag)
	        && PHYSFS_writeUBE8(fileHandle, serializeGame->bExtraFailFlag)
	        && PHYSFS_writeUBE8(fileHandle, serializeGame->bTrackTransporter));
}
static void deserializeSaveGameV30Data_json(const nlohmann::json &o, SAVE_GAME_V30 *serializeGame)
{
	deserializeSaveGameV29Data_json(o, (SAVE_GAME_V29 *) serializeGame);
	serializeGame->scrGameLevel = o.at("scrGameLevel").get<int32_t>();
	serializeGame->bExtraVictoryFlag = o.at("bExtraVictoryFlag").get<uint8_t>();
	serializeGame->bExtraFailFlag = o.at("bExtraFailFlag").get<uint8_t>();
	serializeGame->bTrackTransporter = o.at("bTrackTransporter").get<uint8_t>();
}
static bool deserializeSaveGameV30Data(PHYSFS_file *fileHandle, SAVE_GAME_V30 *serializeGame)
{
	return (deserializeSaveGameV29Data(fileHandle, (SAVE_GAME_V29 *) serializeGame)
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

static void serializeSaveGameV31Data_json(nlohmann::json &o, const SAVE_GAME_V31 *serializeGame)
{
	serializeSaveGameV30Data_json(o, (const SAVE_GAME_V30 *) serializeGame);
	o["missionCheatTime"] = serializeGame->missionCheatTime;
}
static bool serializeSaveGameV31Data(PHYSFS_file *fileHandle, const SAVE_GAME_V31 *serializeGame)
{
	return (serializeSaveGameV30Data(fileHandle, (const SAVE_GAME_V30 *) serializeGame)
	        && PHYSFS_writeSBE32(fileHandle, serializeGame->missionCheatTime));
}
static void deserializeSaveGameV31Data_json(const nlohmann::json &o, SAVE_GAME_V31 *serializeGame)
{
	deserializeSaveGameV30Data_json(o, (SAVE_GAME_V30 *) serializeGame);
	serializeGame->missionCheatTime = o.at("missionCheatTime").get<int32_t>();
}
static bool deserializeSaveGameV31Data(PHYSFS_file *fileHandle, SAVE_GAME_V31 *serializeGame)
{
	return (deserializeSaveGameV30Data(fileHandle, (SAVE_GAME_V30 *) serializeGame)
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
static void serializeSaveGameV33Data_json(nlohmann::json &o, const SAVE_GAME_V33 *serializeGame)
{
	serializeSaveGameV31Data_json(o, (const SAVE_GAME_V31 *) serializeGame);
	serializeMultiplayerGame_json(o, &serializeGame->sGame);
	serializeNetPlay_json(o, &serializeGame->sNetPlay);
	o["sPname"] = serializeGame->sPName;
	o["multiPlayer"] = serializeGame->multiPlayer;
	o["savePlayer"] = serializeGame->savePlayer;
	auto arr = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		arr.insert(arr.end(), serializeGame->sPlayerIndex[i]);
	}
	o["sPlayerIndices"] = arr;
}
static bool serializeSaveGameV33Data(PHYSFS_file *fileHandle, const SAVE_GAME_V33 *serializeGame)
{
	if (!serializeSaveGameV31Data(fileHandle, (const SAVE_GAME_V31 *) serializeGame)
	    || !serializeMultiplayerGame(fileHandle, &serializeGame->sGame)
	    || !serializeNetPlay(fileHandle, &serializeGame->sNetPlay)
	    || !PHYSFS_writeUBE32(fileHandle, serializeGame->savePlayer)
	    || WZ_PHYSFS_writeBytes(fileHandle, serializeGame->sPName, 32) != 32
	    || !PHYSFS_writeSBE32(fileHandle, serializeGame->multiPlayer))
	{
		return false;
	}

	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_writeUBE32(fileHandle, serializeGame->sPlayerIndex[i]))
		{
			return false;
		}
	}

	return true;
}

static void deserializeSaveGameV33Data_json(const nlohmann::json &o, SAVE_GAME_V33 *serializeGame)
{
	deserializeSaveGameV31Data_json(o, (SAVE_GAME_V31 *) serializeGame);
	deserializeMultiplayerGame_json(o, &serializeGame->sGame);
	serializeGame->savePlayer = o.at("savePlayer").get<uint32_t>();
	deserializeNetPlay_json(o, &serializeGame->sNetPlay);
	sstrcpy(serializeGame->sPName, o.at("sPname").get<std::string>().c_str());
	serializeGame->multiPlayer = o.at("multiPlayer").get<int32_t>();
	const auto playerIndices = o.at("sPlayerIndices");
	ASSERT_OR_RETURN(, playerIndices.is_array(), "unexpected type, wanted array");
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		serializeGame->sPlayerIndex[i] = playerIndices.at(i).get<uint32_t>();
	}
}

static bool deserializeSaveGameV33Data(PHYSFS_file *fileHandle, SAVE_GAME_V33 *serializeGame)
{
	unsigned int i;
	int32_t boolMultiPlayer;

	if (!deserializeSaveGameV31Data(fileHandle, (SAVE_GAME_V31 *) serializeGame)
	    || !deserializeMultiplayerGame(fileHandle, &serializeGame->sGame)
	    || !deserializeNetPlay(fileHandle, &serializeGame->sNetPlay)
	    || !PHYSFS_readUBE32(fileHandle, &serializeGame->savePlayer)
	    || WZ_PHYSFS_readBytes(fileHandle, serializeGame->sPName, 32) != 32
	    || !PHYSFS_readSBE32(fileHandle, &boolMultiPlayer))
	{
		return false;
	}

	serializeGame->multiPlayer = boolMultiPlayer;

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!PHYSFS_readUBE32(fileHandle, &serializeGame->sPlayerIndex[i]))
		{
			return false;
		}
	}

	return true;
}

//Now holds AI names for multiplayer
struct SAVE_GAME_V34 : public SAVE_GAME_V33
{
	char sPlayerName[MAX_PLAYERS][StringSize];
};

static void serializeSaveGameV34Data_json(nlohmann::json &o, const SAVE_GAME_V34 *serializeGame)
{
	serializeSaveGameV33Data_json(o, (const SAVE_GAME_V33 *) serializeGame);
	auto arr = nlohmann::json::array();
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		arr.insert(arr.end(), std::string(serializeGame->sPlayerName[i]));
	}
	o["playerNames"] = arr;
}
static bool serializeSaveGameV34Data(PHYSFS_file *fileHandle, const SAVE_GAME_V34 *serializeGame)
{
	unsigned int i;
	if (!serializeSaveGameV33Data(fileHandle, (const SAVE_GAME_V33 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (WZ_PHYSFS_writeBytes(fileHandle, serializeGame->sPlayerName[i], StringSize) != StringSize)
		{
			return false;
		}
	}

	return true;
}

static void deserializeSaveGameV34Data_json(const nlohmann::json &o, SAVE_GAME_V34 *serializeGame)
{
	deserializeSaveGameV33Data_json(o, (SAVE_GAME_V33 *) serializeGame);
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		sstrcpy(serializeGame->sPlayerName[i], o.at("playerNames").at(i).get<std::string>().c_str());
	}
}

static bool deserializeSaveGameV34Data(PHYSFS_file *fileHandle, SAVE_GAME_V34 *serializeGame)
{
	unsigned int i;
	if (!deserializeSaveGameV33Data(fileHandle, (SAVE_GAME_V33 *) serializeGame))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, serializeGame->sPlayerName[i], StringSize) != StringSize)
		{
			return false;
		}
	}

	return true;
}

// First version to utilize (de)serialization API and first to be big-endian (instead of little-endian)
struct SAVE_GAME_V35 : public SAVE_GAME_V34
{
};
static void serializeSaveGameV35Data_json(nlohmann::json &o, const SAVE_GAME_V35 *serializeGame)
{
	return serializeSaveGameV34Data_json(o, (const SAVE_GAME_V34 *) serializeGame);
}
static bool serializeSaveGameV35Data(PHYSFS_file *fileHandle, const SAVE_GAME_V35 *serializeGame)
{
	return serializeSaveGameV34Data(fileHandle, (const SAVE_GAME_V34 *) serializeGame);
}
static void deserializeSaveGameV35Data_json(const nlohmann::json &o, SAVE_GAME_V35 *serializeGame)
{
	return deserializeSaveGameV34Data_json(o, (SAVE_GAME_V34 *) serializeGame);
}
static bool deserializeSaveGameV35Data(PHYSFS_file *fileHandle, SAVE_GAME_V35 *serializeGame)
{
	return deserializeSaveGameV34Data(fileHandle, (SAVE_GAME_V34 *) serializeGame);
}

// Store loaded mods in savegame
struct SAVE_GAME_V38 : public SAVE_GAME_V35
{
	char modList[modlist_string_size];
};

static void serializeSaveGameV38Data_json(nlohmann::json &o, const SAVE_GAME_V38 *serializeGame)
{
	serializeSaveGameV35Data_json(o, (const SAVE_GAME_V35 *) serializeGame);
	o["modList"] = std::string(serializeGame->modList);
}

static bool serializeSaveGameV38Data(PHYSFS_file *fileHandle, const SAVE_GAME_V38 *serializeGame)
{
	if (!serializeSaveGameV35Data(fileHandle, (const SAVE_GAME_V35 *) serializeGame))
	{
		return false;
	}
	
	if (WZ_PHYSFS_writeBytes(fileHandle, serializeGame->modList, modlist_string_size) != modlist_string_size)
	{
		return false;
	}

	return true;
}

static void deserializeSaveGameV38Data_json(const nlohmann::json &o, SAVE_GAME_V38 *serializeGame)
{
	deserializeSaveGameV35Data_json(o, (SAVE_GAME_V35 *) serializeGame);
	sstrcpy(serializeGame->modList, o.at("modList").get<std::string>().c_str());
}

static bool deserializeSaveGameV38Data(PHYSFS_file *fileHandle, SAVE_GAME_V38 *serializeGame)
{
	if (!deserializeSaveGameV35Data(fileHandle, (SAVE_GAME_V35 *) serializeGame))
	{
		return false;
	}

	if (WZ_PHYSFS_readBytes(fileHandle, serializeGame->modList, modlist_string_size) != modlist_string_size)
	{
		return false;
	}

	return true;
}

// Current save game version
typedef SAVE_GAME_V38 SAVE_GAME;
static void serializeSaveGameData_json(nlohmann::json &o, nlohmann::json &saveinfo, const char *saveName, const SAVE_GAME *serializeGame)
{
	serializeSaveGameV38Data_json(o, (const SAVE_GAME_V38 *) serializeGame);
	// not sure whether its 38, 39 or 40... different .cpp files are using different numbers
	o["version"] = VERSION_39;
	
	// This file lists saved games, and their build info
	// one per savegame directory
	const auto tagResult = version_extractVersionNumberFromTag(version_getLatestTag());
	ASSERT(tagResult.has_value(), "No extractable latest tag?? - Please try re-downloading the latest official source bundle");
	const TagVer tag = tagResult.value_or(TagVer());
	saveinfo["latestTagArray"] = nlohmann::json::array({tag.version[0], tag.version[1], tag.version[2]});
	const auto epoch  = std::chrono::system_clock::now().time_since_epoch();
	saveinfo["epoch"] = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
	if (saveinfo.contains(saveName))
	{
		saveinfo.erase(saveName);
	}	
	char ourtime[20];
	const time_t curtime = time(nullptr);
	struct tm timeinfo = getLocalTime(curtime);
	// "YYYY-MM-DD HH:MM:SS"
	strftime(ourtime, sizeof(ourtime), "%F %T", &timeinfo);
	saveinfo["time"] = ourtime;
}
static bool serializeSaveGameData(PHYSFS_file *fileHandle, const SAVE_GAME *serializeGame)
{
	return serializeSaveGameV38Data(fileHandle, (const SAVE_GAME_V38 *) serializeGame);
}
static bool deserializeSaveGameData_json(const nlohmann::json &o, SAVE_GAME *serializeGame)
{
	try
	{
		deserializeSaveGameV38Data_json(o, (SAVE_GAME_V38 *) serializeGame);
		return true;
	} catch (nlohmann::json::exception &e)
	{
		debug(LOG_ERROR, "%s", e.what());
		return false;
	}
	
}
static bool deserializeSaveGameData(PHYSFS_file *fileHandle, SAVE_GAME *serializeGame)
{
	return deserializeSaveGameV38Data(fileHandle, (SAVE_GAME_V38 *) serializeGame);
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

static UDWORD		savedGameTime;
static UDWORD		savedObjId;
static SDWORD		startX, startY;
static UDWORD		width, height;
static GAME_TYPE	gameType;
static bool IsScenario;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/
static bool gameLoadV7(PHYSFS_file *fileHandle, nonstd::optional<nlohmann::json>&);
static bool gameLoadV(PHYSFS_file *fileHandle, unsigned int version, nonstd::optional<nlohmann::json>&);
static bool loadMainFile(const std::string &fileName);
static bool loadMainFileFinal(const std::string &fileName);
static bool writeMainFile(const std::string &fileName, SDWORD saveType);
static bool writeGameFile(const char *fileName, SDWORD saveType);
static bool writeMapFile(const char *fileName);

static bool loadWzMapDroidInit(WzMap::Map &wzMap);

static bool loadSaveDroid(const char *pFileName, DROID **ppsCurrentDroidLists);
static bool loadSaveDroidPointers(const WzString &pFileName, DROID **ppsCurrentDroidLists);
static bool writeDroidFile(const char *pFileName, DROID **ppsCurrentDroidLists);

static bool loadSaveStructure(char *pFileData, UDWORD filesize);
static bool loadSaveStructure2(const char *pFileName);
static bool loadWzMapStructure(WzMap::Map& wzMap);
static bool loadSaveStructurePointers(const WzString& filename, STRUCTURE **ppList);
static bool writeStructFile(const char *pFileName);

static bool loadSaveTemplate(const char *pFileName);
static bool writeTemplateFile(const char *pFileName);

static bool loadSaveFeature(char *pFileData, UDWORD filesize);
static bool writeFeatureFile(const char *pFileName);
static bool loadSaveFeature2(const char *pFileName);
static bool loadWzMapFeature(WzMap::Map &wzMap);

static bool writeTerrainTypeMapFile(char *pFileName);

static bool loadSaveCompList(const char *pFileName);
static bool writeCompListFile(const char *pFileName);

static bool loadSaveStructTypeList(const char *pFileName);
static bool writeStructTypeListFile(const char *pFileName);

static bool loadSaveResearch(const char *pFileName);
static bool writeResearchFile(char *pFileName);

static bool loadSaveMessage(const char* pFileName, LEVEL_TYPE levelType);
static bool writeMessageFile(const char *pFileName);

static bool loadSaveLimits(const char *pFileName);
static bool writeStructLimitsFile(const char *pFileName);

static bool readFiresupportDesignators(const char *pFileName);
static bool writeFiresupportDesignators(const char *pFileName);

static bool writeScriptState(const char *pFileName);

static bool gameLoad(const char *fileName);

/* set the global scroll values to use for the save game */
static void setMapScroll();

static char *getSaveStructNameV19(SAVE_STRUCTURE_V17 *psSaveStructure)
{
	return (psSaveStructure->name);
}

/*This just loads up the .gam file to determine which level data to set up - split up
so can be called in levLoadData when starting a game from a load save game*/

// -----------------------------------------------------------------------------------------
bool loadGameInit(const char *fileName)
{
	ASSERT_OR_RETURN(false, fileName != nullptr, "fileName is null??");

	if (strEndsWith(fileName, ".wzrp"))
	{
		SetGameMode(GS_TITLE_SCREEN); // hack - the caller sets this to GS_NORMAL but we actually want to proceed with normal startGameLoop

		// if it ends in .wzrp, try to load the replay!
		WZGameReplayOptionsHandler optionsHandler;
		if (!NETloadReplay(fileName, optionsHandler))
		{
			return false;
		}

		bMultiPlayer = true;
		bMultiMessages = true;
		changeTitleMode(STARTGAME);
	}
	else if (!gameLoad(fileName))
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
bool loadMissionExtras(const char* pGameToLoad, LEVEL_TYPE levelType)
{
	char			aFileName[256];
	size_t			fileExten;

	sstrcpy(aFileName, pGameToLoad);
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
			strcat(aFileName, "messtate.json");
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
	for (int player = 0; player < game.maxPlayers; player++)
	{
		for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
		{
			orderCheckList(psDroid);
			actionSanity(psDroid);
		}
	}
}

static void getIniBaseObject(WzConfig &ini, WzString const &key, BASE_OBJECT *&object)
{
	object = nullptr;
	if (ini.contains(key + "/id"))
	{
		int tid = ini.value(key + "/id", -1).toInt();
		int tplayer = ini.value(key + "/player", -1).toInt();
		OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value(key + "/type", 0).toInt();
		ASSERT_OR_RETURN(, tid >= 0 && tplayer >= 0, "Bad ID");
		object = getBaseObjFromData(tid, tplayer, ttype);
		ASSERT(object != nullptr, "Failed to find target");
	}
}

static void getIniStructureStats(WzConfig &ini, WzString const &key, STRUCTURE_STATS *&stats)
{
	stats = nullptr;
	if (ini.contains(key))
	{
		WzString statName = ini.value(key).toWzString();
		int tid = getStructStatFromName(statName);
		ASSERT_OR_RETURN(, tid >= 0, "Target stats not found %s", statName.toUtf8().c_str());
		stats = &asStructureStats[tid];
	}
}

static void getIniDroidOrder(WzConfig &ini, WzString const &key, DroidOrder &order)
{
	order.type = (DroidOrderType)ini.value(key + "/type", DORDER_NONE).toInt();
	order.pos = ini.vector2i(key + "/pos");
	order.pos2 = ini.vector2i(key + "/pos2");
	order.direction = ini.value(key + "/direction").toInt();
	getIniBaseObject(ini, key + "/obj", order.psObj);
	getIniStructureStats(ini, key + "/stats", order.psStats);
}

static void setIniBaseObject(nlohmann::json &json, WzString const &key, BASE_OBJECT const *object)
{
	if (object != nullptr && object->died <= 1)
	{
		const auto& keyStr = key.toStdString();
		json[keyStr + "/id"] = object->id;
		json[keyStr + "/player"] = object->player;
		json[keyStr + "/type"] = object->type;
#ifdef DEBUG
		//ini.setValue(key + "/debugfunc", WzString::fromUtf8(psCurr->targetFunc));
		//ini.setValue(key + "/debugline", psCurr->targetLine);
#endif
	}
}

static inline void setIniStructureStats(nlohmann::json &jsonObj, WzString const &key, STRUCTURE_STATS const *stats)
{
	if (stats != nullptr)
	{
		jsonObj[key.toStdString()] = stats->id;
	}
}

static inline void setIniDroidOrder(nlohmann::json &jsonObj, WzString const &key, DroidOrder const &order)
{
	const auto& keyStr = key.toStdString();
	jsonObj[keyStr + "/type"] = order.type;
	jsonObj[keyStr + "/pos"] = order.pos;
	jsonObj[keyStr + "/pos2"] = order.pos2;
	jsonObj[keyStr + "/direction"] = order.direction;
	setIniBaseObject(jsonObj, key + "/obj", order.psObj);
	setIniStructureStats(jsonObj, key + "/stats", order.psStats);
}

static void allocatePlayers()
{
	DebugInputManager& dbgInputManager = gInputManager.debugManager();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		NetPlay.players[i].team = saveGameData.sNetPlay.players[i].team;
		NetPlay.players[i].ai = saveGameData.sNetPlay.players[i].ai;
		NetPlay.players[i].difficulty = saveGameData.sNetPlay.players[i].difficulty;
//		NetPlay.players[i].faction; // read and initialized by loadMainFile
		setPlayerName(i, saveGameData.sNetPlay.players[i].name);
		NetPlay.players[i].position = saveGameData.sNetPlay.players[i].position;
		if (NetPlay.players[i].difficulty == AIDifficulty::HUMAN || (game.type == LEVEL_TYPE::CAMPAIGN && i == 0))
		{
			NetPlay.players[i].allocated = true;
			//processDebugMappings ensures game does not start in DEBUG mode
			dbgInputManager.setPlayerWantsDebugMappings(i, false);
		}
		else
		{
			NetPlay.players[i].allocated = false;
		}
	}
}

static void getPlayerNames()
{
	/* Get human and AI players names */
	if (saveGameVersion >= VERSION_34)
	{
		for (unsigned i = 0; i < MAX_PLAYERS; ++i)
		{
			(void)setPlayerName(i, saveGameData.sPlayerName[i]);
		}
	}
}

static WzMap::MapType getWzMapType(bool UserSaveGame)
{
	if (UserSaveGame)
	{
		return WzMap::MapType::SAVEGAME;
	}
	else
	{
		return (game.type == LEVEL_TYPE::CAMPAIGN) ? WzMap::MapType::CAMPAIGN : WzMap::MapType::SKIRMISH;
	}
}

// -----------------------------------------------------------------------------------------
// UserSaveGame ... this is true when you are loading a players save game
bool loadGame(const char *pGameToLoad, bool keepObjects, bool freeMem, bool UserSaveGame)
{
	std::shared_ptr<WzMap::Map> data;
	std::map<WzString, DROID **> droidMap;
	std::map<WzString, STRUCTURE **> structMap;
	char			aFileName[256];
	size_t			fileExten;
	UDWORD			fileSize;
	char			*pFileData = nullptr;
	UDWORD			player, inc, i, j;
	DROID           *psCurr;
	UWORD           missionScrollMinX = 0, missionScrollMinY = 0,
	                missionScrollMaxX = 0, missionScrollMaxY = 0;
	uint32_t        mapSeed = 0;

	/* Stop the game clock */
	gameTimeStop();

	if ((gameType == GTYPE_SAVE_START) ||
	    (gameType == GTYPE_SAVE_MIDMISSION))
	{
		gameTimeReset(savedGameTime);//added 14 may 98 JPS to solve kev's problem with no firing droids
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
			apsDroidLists[player] = nullptr;
			apsStructLists[player] = nullptr;
			apsFeatureLists[player] = nullptr;
			apsFlagPosLists[player] = nullptr;
			//clear all the messages?
			apsProxDisp[player] = nullptr;
			apsSensorList[0] = nullptr;
			apsExtractorLists[player] = nullptr;
		}
		apsOilList[0] = nullptr;
		initFactoryNumFlag();
	}

	if (UserSaveGame)//always !keepObjects
	{
		//initialise the lists
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			apsLimboDroids[player] = nullptr;
			mission.apsDroidLists[player] = nullptr;
			mission.apsStructLists[player] = nullptr;
			mission.apsFeatureLists[player] = nullptr;
			mission.apsFlagPosLists[player] = nullptr;
			mission.apsExtractorLists[player] = nullptr;
		}
		mission.apsOilList[0] = nullptr;
		mission.apsSensorList[0] = nullptr;

		// Stuff added after level load to avoid being reset or initialised during load
		// always !keepObjects

		if (saveGameVersion >= VERSION_33)
		{
			bMultiPlayer = saveGameData.multiPlayer;
		}

		if (saveGameVersion >= VERSION_12)
		{
			mission.startTime = saveGameData.missionTime;
		}

		//set the scroll variables
		if (saveGameData.ScrollMinX >= 0 && static_cast<uint32_t>(saveGameData.ScrollMinX) > saveGameData.ScrollMaxX)
		{
			debug(LOG_ERROR, "ScrollMinX (%" PRIi32 ") exceeds ScrollMaxX %" PRIu32, saveGameData.ScrollMinX, saveGameData.ScrollMaxX);
			uint32_t newMax = static_cast<uint32_t>(saveGameData.ScrollMinX);
			saveGameData.ScrollMinX = static_cast<int32_t>(saveGameData.ScrollMaxX);
			saveGameData.ScrollMaxX = newMax;
		}
		if (saveGameData.ScrollMinY >= 0 && static_cast<uint32_t>(saveGameData.ScrollMinY) > saveGameData.ScrollMaxY)
		{
			debug(LOG_ERROR, "ScrollMinY (%" PRIi32 ") exceeds ScrollMaxY %" PRIu32, saveGameData.ScrollMinY, saveGameData.ScrollMaxY);
			uint32_t newMax = static_cast<uint32_t>(saveGameData.ScrollMinY);
			saveGameData.ScrollMinY = static_cast<int32_t>(saveGameData.ScrollMaxY);
			saveGameData.ScrollMaxY = newMax;
		}
		startX = saveGameData.ScrollMinX;
		startY = saveGameData.ScrollMinY;
		width = saveGameData.ScrollMaxX - saveGameData.ScrollMinX;
		height = saveGameData.ScrollMaxY - saveGameData.ScrollMinY;
		gameType = static_cast<GAME_TYPE>(saveGameData.GameType);

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
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				for (j = 0; j < MAX_PLAYERS; j++)
				{
					alliances[i][j] = saveGameData.alliances[i][j];
					if (i == j)
					{
						alliances[i][j] = ALLIANCE_FORMED;    // hack to fix old savegames
					}
					if (bMultiPlayer && alliancesSharedVision(game.alliance) && alliances[i][j] == ALLIANCE_FORMED)
					{
						alliancebits[i] |= 1 << j;
					}
				}
			}
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				setPlayerColour(i, saveGameData.playerColour[i]);
			}
			SetRadarZoom(saveGameData.radarZoom);
		}

		if (saveGameVersion >= VERSION_20)//V21
		{
			setDroidsToSafetyFlag(saveGameData.bDroidsToSafetyFlag);
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
			uint32_t scav = game.scavengers;

			game			= saveGameData.sGame;
			game.scavengers = scav;	// ok, so this is butt ugly. but i'm just getting inspiration from the rest of the code around here. ok? - per
			productionPlayer = selectedPlayer;
			bMultiMessages	= bMultiPlayer;

			NetPlay.bComms = saveGameData.sNetPlay.bComms;

			allocatePlayers();

			if (bMultiPlayer)
			{
				loadMultiStats(saveGameData.sPName, &playerStats);				// stats stuff
				setMultiStats(selectedPlayer, playerStats, false);
				setMultiStats(selectedPlayer, playerStats, true);
			}
		}
	}

	getPlayerNames();

	//clear the player Power structs
	if ((gameType != GTYPE_SAVE_START) && (gameType != GTYPE_SAVE_MIDMISSION) &&
	    (!keepObjects))
	{
		clearPlayerPower();
	}

	//before loading the data - turn power off so don't get any power low warnings
	powerCalculated = false;

	/* Load in the chosen file data */
	sstrcpy(aFileName, pGameToLoad);
	fileExten = strlen(aFileName) - 3;			// hack - !
	aFileName[fileExten - 1] = '\0';
	strcat(aFileName, "/");

	//the terrain type WILL only change with Campaign changes (well at the moment!)
	if (gameType != GTYPE_SCENARIO_EXPAND || UserSaveGame)
	{
		//load in the terrain type map
		aFileName[fileExten] = '\0';
		strcat(aFileName, "ttypes.ttp");
		//load the terrain type data
		if (!loadTerrainTypeMap(aFileName))
		{
			debug(LOG_ERROR, "Failed with: %s", aFileName);
			goto error;
		}
	}

	/* decide if we have to create teams, ONLY in multiplayer mode!*/
	if (bMultiPlayer && alliancesSharedVision(game.alliance))
	{
		createTeamAlliances();

		/* Update ally vision for pre-placed structures and droids */
		for (unsigned int pl = 0; pl < MAX_PLAYERS; pl++)
		{
			if (pl != selectedPlayer)
			{
				/* Structures */
				for (STRUCTURE *psStr = apsStructLists[pl]; psStr; psStr = psStr->psNext)
				{
					if (selectedPlayer < MAX_PLAYERS && aiCheckAlliances(psStr->player, selectedPlayer))
					{
						visTilesUpdate((BASE_OBJECT *)psStr);
					}
				}

				/* Droids */
				for (DROID *psDroid = apsDroidLists[pl]; psDroid; psDroid = psDroid->psNext)
				{
					if (selectedPlayer < MAX_PLAYERS && aiCheckAlliances(psDroid->player, selectedPlayer))
					{
						visTilesUpdate((BASE_OBJECT *)psDroid);
					}
				}
			}
		}
	}

	//load up the Droid Templates BEFORE any structures are loaded
	if (IsScenario == false)
	{
		if (bMultiPlayer)
		{
			droidTemplateShutDown();
		}
		else
		{
			clearTemplates(0);
			localTemplates.clear();
		}

		aFileName[fileExten] = '\0';
		strcat(aFileName, "templates.json");
		if (!loadSaveTemplate(aFileName))
		{
			debug(LOG_ERROR, "Failed with: %s", aFileName);
			goto error;
		}
	}

	if ((saveGameVersion >= VERSION_15) && UserSaveGame)
	{
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limits.json");

		//load the data into apsStructLists
		if (!loadSaveLimits(aFileName))
		{
			debug(LOG_ERROR, "failed to load %s", aFileName);
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
		if (!mapLoad(aFileName))
		{
			debug(LOG_ERROR, "Failed with: %s", aFileName);
			return false;
		}

		//load in the visibility file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "misvis.bjo");

		// Load in the visibility data from the chosen file
		if (!readVisibilityData(aFileName))
		{
			debug(LOG_ERROR, "Failed with: %s", aFileName);
			goto error;
		}

		// reload the objects that were in the mission list
		//load in the features -do before the structures
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mfeature.json");

		//load the data into apsFeatureLists
		if (!loadSaveFeature2(aFileName))
		{
			aFileName[fileExten] = '\0';
			strcat(aFileName, "mfeat.bjo");
			/* Load in the chosen file data */
			pFileData = fileLoadBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug(LOG_ERROR, "Failed with: %s", aFileName);
				goto error;
			}
			if (!loadSaveFeature(pFileData, fileSize))
			{
				debug(LOG_ERROR, "Failed with: %s", aFileName);
				goto error;
			}
		}

		initStructLimits();
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mstruct.json");

		//load in the mission structures
		if (!loadSaveStructure2(aFileName))
		{
			aFileName[fileExten] = '\0';
			strcat(aFileName, "mstruct.bjo");
			/* Load in the chosen file data */
			pFileData = fileLoadBuffer;
			if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug(LOG_ERROR, "Failed with: %s", aFileName);
				goto error;
			}
			//load the data into apsStructLists
			if (!loadSaveStructure(pFileData, fileSize))
			{
				debug(LOG_ERROR, "Failed with: %s", aFileName);
				goto error;
			}
		}
		else
		{
			structMap[aFileName] = mission.apsStructLists;	// we swap pointers below
		}

		// load in the mission droids, if any
		aFileName[fileExten] = '\0';
		strcat(aFileName, "mdroid.json");
		if (loadSaveDroid(aFileName, apsDroidLists))
		{
			droidMap[aFileName] = mission.apsDroidLists; // need to swap here to read correct list later
		}

		/* after we've loaded in the units we need to redo the orientation because
		 * the direction may have been saved - we need to do it outside of the loop
		 * whilst the current map is valid for the units
		 */
		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			for (psCurr = apsDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
			{
				if (psCurr->droidType != DROID_PERSON
				    // && psCurr->droidType != DROID_CYBORG
				    && !cyborgDroid(psCurr)
				    && (!isTransporter(psCurr))
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

	// construct the WzMap object for loading map data
	aFileName[fileExten] = '\0';
	mapSeed = gameRandU32();
	data = WzMap::Map::loadFromPath(aFileName, getWzMapType(UserSaveGame), game.maxPlayers, mapSeed, std::make_shared<WzMapDebugLogger>(), std::make_shared<WzMapPhysFSIO>());

	if (data && data->wasScriptGenerated())
	{
		// Log the random seed used to generate this instance of the map
		debug(LOG_INFO, "Loaded script-generated map \"%s\" with random seed: %" PRIu32, aFileName, mapSeed);
	}

	//if Campaign Expand then don't load in another map
	if (gameType != GTYPE_SCENARIO_EXPAND)
	{
		psMapTiles = nullptr;
		// load in the map file
		if (!data)
		{
			debug(LOG_ERROR, "Failed to load map from path: %s", aFileName);
			return false;
		}
		auto mapData = data->mapData();
		if (!mapData)
		{
			debug(LOG_ERROR, "Failed to load map data from path: %s", aFileName);
			return false;
		}
		if (data->wasScriptGenerated())
		{
			syncDebug("mapSize = [%d, %d]", mapData->width, mapData->height);
			syncDebug("crc(maptiles) = 0x%08x", mapData->crcSumMapTiles(0));
			syncDebug("crc(structures) = 0x%08x", data->crcSumStructures(0));
			syncDebug("crc(droids) = 0x%08x", data->crcSumDroids(0));
			syncDebug("crc(features) = 0x%08x", data->crcSumFeatures(0));
		}
		if (!mapLoadFromWzMapData(mapData))
		{
			debug(LOG_ERROR, "Failed to process map data from path: %s", aFileName);
			return false;
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
			strcat(aFileName, "fxstate.json");

			// load the fx data from the file
			if (!readFXData(aFileName))
			{
				debug(LOG_ERROR, "Failed with: %s", aFileName);
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

	//if user save game then load up the research BEFORE any droids or structures are loaded
	if (gameType == GTYPE_SAVE_START || gameType == GTYPE_SAVE_MIDMISSION)
	{
		//load in the research list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "resstate.json");
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
		strcat(aFileName, "droid.json");

		//load the data into apsDroidLists
		if (!UserSaveGame)
		{
			ASSERT(data != nullptr, "Expecting WzMap::Map instance");
			if (loadWzMapDroidInit(*(data.get())))
			{
				debug(LOG_SAVE, "Loaded new style droids");
				droidMap[aFileName] = apsDroidLists;	// load pointers later
			}
			else
			{
				aFileName[fileExten] = '\0';
				debug(LOG_ERROR, "Failed to load map droid init from map directory: %s", aFileName);
				goto error;
			}
		}
		else
		{
			if (loadSaveDroid(aFileName, apsDroidLists))
			{
				debug(LOG_SAVE, "Loaded new style droids");
				droidMap[aFileName] = apsDroidLists;	// load pointers later
			}
		}
	}
	else
	{
		//load in the droids
		aFileName[fileExten] = '\0';
		strcat(aFileName, "droid.json");

		//load the data into apsDroidLists
		if (!loadSaveDroid(aFileName, apsDroidLists))
		{
			debug(LOG_ERROR, "failed to load %s", aFileName);
			goto error;
		}
		droidMap[aFileName] = apsDroidLists;	// load pointers later

		/* after we've loaded in the units we need to redo the orientation because
		 * the direction may have been saved - we need to do it outside of the loop
		 * whilst the current map is valid for the units
		 */
		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			for (psCurr = apsDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
			{
				if (psCurr->droidType != DROID_PERSON
				    && !cyborgDroid(psCurr)
				    && (!isTransporter(psCurr))
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
			strcat(aFileName, "mdroid.json");

			// load the data into mission.apsDroidLists, if any
			if (loadSaveDroid(aFileName, mission.apsDroidLists))
			{
				droidMap[aFileName] = mission.apsDroidLists;
			}
		}
	}

	if (saveGameVersion >= VERSION_23)
	{
		// load in the limbo droids, if any
		aFileName[fileExten] = '\0';
		strcat(aFileName, "limbo.json");
		if (loadSaveDroid(aFileName, apsLimboDroids))
		{
			droidMap[aFileName] = apsLimboDroids;
		}
	}

	//load in the features -do before the structures
	aFileName[fileExten] = '\0';
	if (!UserSaveGame)
	{
		ASSERT(data != nullptr, "Expecting WzMap::Map instance");
		if (!loadWzMapFeature(*(data.get())))
		{
			debug(LOG_ERROR, "Failed to load map feature init from map directory: %s", aFileName);
			goto error;
		}
	}
	else
	{
		aFileName[fileExten] = '\0';
		strcat(aFileName, "feature.json");
		if (!loadSaveFeature2(aFileName))
		{
			debug(LOG_ERROR, "Failed with: %s", aFileName);
			goto error;
		}
	}

	//load in the structures
	initStructLimits();
	aFileName[fileExten] = '\0';
	strcat(aFileName, "struct.json");
	if (!UserSaveGame)
	{
		ASSERT(data != nullptr, "Expecting WzMap::Map instance");
		if (game.type != LEVEL_TYPE::CAMPAIGN)
		{
			freeAllFlagPositions();		//clear any flags put in during level loads
		}
		if (!loadWzMapStructure(*(data.get())))
		{
			aFileName[fileExten] = '\0';
			debug(LOG_ERROR, "Failed to load map structure init from map directory: %s", aFileName);
			goto error;
		}
		if (game.type != LEVEL_TYPE::CAMPAIGN)
		{
			resetFactoryNumFlag();	//reset flags into the masks
		}
	}
	else if (!loadSaveStructure2(aFileName))
	{
		debug(LOG_ERROR, "Failed with: %s", aFileName);
		goto error;
	}
	structMap[aFileName] = apsStructLists;

	//if user save game then load up the current level for structs and components
	if (gameType == GTYPE_SAVE_START || gameType == GTYPE_SAVE_MIDMISSION)
	{
		//load in the component list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "complist.json");
		if (!loadSaveCompList(aFileName))
		{
			debug(LOG_ERROR, "failed to load %s", aFileName);
			goto error;
		}
		//load in the structure type list file
		aFileName[fileExten] = '\0';
		strcat(aFileName, "strtype.json");
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
				debug(LOG_ERROR, "Failed with: %s", aFileName);
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
			strcat(aFileName, "score.json");

			// Load the fx data from the chosen file
			if (!readScoreData(aFileName))
			{
				debug(LOG_ERROR, "Failed with: %s", aFileName);
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
			strcat(aFileName, "firesupport.json");

			if (!readFiresupportDesignators(aFileName))
			{
				debug(LOG_ERROR, "Failed with: %s", aFileName);
				goto error;
			}
		}
	}

	if ((saveGameVersion >= VERSION_15) && UserSaveGame)
	{
		setCurrentStructQuantity(false);
	}
	else
	{
		setCurrentStructQuantity(true);
	}

	//check that delivery points haven't been put down in invalid location
	checkDeliveryPoints(saveGameVersion);

	//turn power on for rest of game
	powerCalculated = true;

	if (!keepObjects)//only reset the pointers if they were set
	{
		// Reset the object pointers in the droid target lists
		for (auto it = droidMap.begin(); it != droidMap.end(); ++it)
		{
			const WzString& key = it->first;
			DROID **pList = it->second;
			loadSaveDroidPointers(key, pList);
		}
		for (auto it = structMap.begin(); it != structMap.end(); ++it)
		{
			const WzString& key = it->first;
			STRUCTURE **pList = it->second;
			loadSaveStructurePointers(key, pList);
		}
	}

	// Load labels
	aFileName[fileExten] = '\0';
	strcat(aFileName, "labels.json");
	loadLabels(aFileName);

	//if user save game then reset the time - BEWARE IF YOU USE IT
	if ((gameType == GTYPE_SAVE_START) ||
	    (gameType == GTYPE_SAVE_MIDMISSION))
	{
		ASSERT(gameTime == savedGameTime, "loadGame; game time modified during load");
		gameTimeReset(savedGameTime);//added 14 may 98 JPS to solve kev's problem with no firing droids

		//reset the objId for new objects
		if (saveGameVersion >= VERSION_17)
		{
			unsynchObjID = (savedObjId + 1) / 2; // Make new object ID start at savedObjId*8.
			synchObjID   = savedObjId * 4;      // Make new object ID start at savedObjId*8.
		}

		if (getDroidsToSafetyFlag())
		{
			//The droids lists are "reversed" as they are loaded in loadSaveDroid().
			//Which later causes issues in saveCampaignData() which tries to extract
			//the first transporter group sent off at Beta-end by reversing this very list.
			ASSERT(selectedPlayer < MAX_PLAYERS, "selectedPlayer is out of bounds: %" PRIu32 "", selectedPlayer);
			reverseObjectList(&mission.apsDroidLists[selectedPlayer]);
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
		ASSERT(selectedPlayer < MAX_PLAYERS, "selectedPlayer is out of bounds: %" PRIu32 "", selectedPlayer);
		if (mission.apsDroidLists[selectedPlayer] == nullptr)
		{
			//set the mission type
			startMissionSave(LEVEL_TYPE::LDS_EXPAND);
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

	debug(LOG_NEVER, "Done loading");

	return true;

error:
	debug(LOG_ERROR, "Game load failed for %s, FS:%s, params=%s,%s,%s", pGameToLoad, WZ_PHYSFS_getRealDir_String(pGameToLoad).c_str(),
	      keepObjects ? "true" : "false", freeMem ? "true" : "false", UserSaveGame ? "true" : "false");

	/* Clear all the objects off the map and free up the map memory */
	freeAllDroids();
	freeAllStructs();
	freeAllFeatures();
	droidTemplateShutDown();
	psMapTiles = nullptr;

	/* Start the game clock */
	gameTimeStart();

	return false;
}
// -----------------------------------------------------------------------------------------

bool saveGame(const char *aFileName, GAME_TYPE saveType)
{
	size_t			fileExtension;
	DROID			*psDroid, *psNext;
	char			CurrentFileName[PATH_MAX] = {'\0'};

	triggerEvent(TRIGGER_GAME_SAVING);

	ASSERT_OR_RETURN(false, aFileName && strlen(aFileName) > 4, "Bad savegame filename");
	sstrcpy(CurrentFileName, aFileName);
	debug(LOG_WZ, "saveGame: %s", CurrentFileName);

	fileExtension = strlen(CurrentFileName) - 3;
	gameTimeStop();
	sanityUpdate();

	/* Write the data to the file */
	if (!writeGameFile(CurrentFileName, saveType))
	{
		debug(LOG_ERROR, "writeGameFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//remove the file extension
	CurrentFileName[strlen(CurrentFileName) - 4] = '\0';

	//create dir will fail if directory already exists but don't care!
	(void) PHYSFS_mkdir(CurrentFileName);

	writeMainFile(std::string(CurrentFileName) + "/main.json", saveType);

	//save the map file
	strcat(CurrentFileName, "/game.map");
	/* Write the data to the file */
	if (!writeMapFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeMapFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	// Save some game info
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "gameinfo.json");
	writeGameInfo(CurrentFileName);

	// Save labels
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "labels.json");
	writeLabels(CurrentFileName);

	//create the droids filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "droid.json");
	/*Write the current droid lists to the file*/
	if (!writeDroidFile(CurrentFileName, apsDroidLists))
	{
		debug(LOG_ERROR, "writeDroidFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the structures filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "struct.json");
	/*Write the data to the file*/
	if (!writeStructFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the templates filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "templates.json");
	/*Write the data to the file*/
	if (!writeTemplateFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeTemplateFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the features filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "feature.json");
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
	strcat(CurrentFileName, "limits.json");
	/*Write the data to the file*/
	if (!writeStructLimitsFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructLimitsFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the component lists filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "complist.json");
	/*Write the data to the file*/
	if (!writeCompListFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeCompListFile(\"%s\") failed", CurrentFileName);
		goto error;
	}
	//create the structure type lists filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "strtype.json");
	/*Write the data to the file*/
	if (!writeStructTypeListFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeStructTypeListFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the research filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "resstate.json");
	/*Write the data to the file*/
	if (!writeResearchFile(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeResearchFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//create the message filename
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "messtate.json");
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
	strcat(CurrentFileName, "fxstate.json");
	/*Write the data to the file*/
	if (!writeFXData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeFXData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	//added at V15 save
	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "score.json");
	/*Write the data to the file*/
	if (!writeScoreData(CurrentFileName))
	{
		debug(LOG_ERROR, "saveGame: writeScoreData(\"%s\") failed", CurrentFileName);
		goto error;
	}

	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "firesupport.json");
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
	strcat(CurrentFileName, "mdroid.json");
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
		ASSERT(selectedPlayer < MAX_PLAYERS, "selectedPlayer is out of bounds: %" PRIu32 "", selectedPlayer);
		for (psDroid = apsLimboDroids[selectedPlayer]; psDroid != nullptr; psDroid = psNext)
		{
			psNext = psDroid->psNext;
			//limbo list invalidate XY
			psDroid->pos.x = INVALID_XY;
			psDroid->pos.y = INVALID_XY;
			//this is mainly for VTOLs
			setSaveDroidBase(psDroid, nullptr);
			orderDroid(psDroid, DORDER_STOP, ModeImmediate);
		}
	}

	CurrentFileName[fileExtension] = '\0';
	strcat(CurrentFileName, "limbo.json");
	/*Write the swapped droid lists to the file*/
	if (!writeDroidFile(CurrentFileName, apsLimboDroids))
	{
		debug(LOG_ERROR, "saveGame: writeDroidFile(\"%s\") failed", CurrentFileName);
		goto error;
	}

	if (saveGameOnMission)
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
		strcat(CurrentFileName, "mstruct.json");
		/*Write the data to the file*/
		if (!writeStructFile(CurrentFileName))
		{
			debug(LOG_ERROR, "saveGame: writeStructFile(\"%s\") failed", CurrentFileName);
			goto error;
		}

		//create the features filename
		CurrentFileName[fileExtension] = '\0';
		strcat(CurrentFileName, "mfeature.json");
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
	CurrentFileName[fileExtension - 1] = '\0';

	/* Start the game clock */
	triggerEvent(TRIGGER_GAME_SAVED);
	gameTimeStart();
	return true;

error:
	/* Start the game clock */
	gameTimeStart();

	return false;
}

// -----------------------------------------------------------------------------------------
static bool writeMapFile(const char *fileName)
{
	ASSERT_OR_RETURN(false, fileName != nullptr, "filename is null");

	/* Get the save data */
	WzMap::MapData mapData;
	bool status = mapSaveToWzMapData(mapData);
	if (!status)
	{
		return false;
	}

	/* Write out the map data */
	WzMapPhysFSIO mapIO;
	WzMapDebugLogger debugLoggerInstance;
	status = WzMap::writeMapData(mapData, fileName, mapIO, WzMap::LatestOutputFormat, &debugLoggerInstance);

	return status;
}

// -----------------------------------------------------------------------------------------
static bool gameLoad(const char *fileName)
{
	char CurrentFileName[PATH_MAX];
	strcpy(CurrentFileName, fileName);
	GAME_SAVEHEADER fileHeader = {};
	auto gamJsonSave = readGamJson(fileName);
	debug(LOG_INFO, "loading: %s", fileName);
	PHYSFS_file *fileHandle = openLoadFile(fileName, false);
	if (!gamJsonSave.has_value() && fileHandle)
	{
		// haven't converted .gam to .json yet!
		// Read the header from the file
		if (!deserializeSaveGameHeader(fileHandle, &fileHeader))
		{
			debug(LOG_ERROR, "gameLoad: error while reading header from file (%s): %s", fileName, WZ_PHYSFS_getLastError());
			PHYSFS_close(fileHandle);
			return false;
		}
	}
	else if (gamJsonSave.has_value())
	{
		// be compatible with .gam logic
		fileHeader.version = gamJsonSave.value().at("version").get<uint32_t>();
		fileHeader.aFileType[0] = 'g';
		fileHeader.aFileType[1] = 'a';
		fileHeader.aFileType[2] = 'm';
		fileHeader.aFileType[3] = 'e';
	}
	// exit if no .gam neither .json was found
	if (!fileHandle && !gamJsonSave.has_value())
	{
		// Failure to open the file is a failure to load the specified savegame
		return false;
	}
	debug(LOG_WZ, "gameLoad");

	// Check the header to see if we've been given a file of the right type
	if (!(fileHeader.aFileType[0] == 'g' && fileHeader.aFileType[1] == 'a' && fileHeader.aFileType[2] == 'm' && fileHeader.aFileType[3] == 'e'))
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

	// Prior to getting here, the directory structure has been set to wherever the
	// map or savegame is loaded from, so we will get the right ruleset file.
	if (!PHYSFS_exists("ruleset.json"))
	{
		debug(LOG_ERROR, "ruleset.json not found! User generated data will not work.");
		memset(rulesettag, 0, sizeof(rulesettag));
	}
	else
	{
		WzConfig ruleset(WzString::fromUtf8("ruleset.json"), WzConfig::ReadOnly);
		if (!ruleset.contains("tag"))
		{
			debug(LOG_ERROR, "ruleset tag not found in ruleset.json!"); // fall-through
		}
		WzString tag = ruleset.value("tag", "[]").toWzString();
		sstrcpy(rulesettag, tag.toUtf8().c_str());
		if (strspn(rulesettag, "abcdefghijklmnopqrstuvwxyz") != strlen(rulesettag)) // for safety
		{
			debug(LOG_ERROR, "ruleset.json userdata tag contains invalid characters!");
			debug(LOG_ERROR, "User generated data will not work.");
			memset(rulesettag, 0, sizeof(rulesettag));
		}
	}

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
		bool retVal = gameLoadV7(fileHandle, gamJsonSave);
		PHYSFS_close(fileHandle);
		return retVal;
	}
	else if (fileHeader.version <= CURRENT_VERSION_NUM)
	{
		//The in-game load menu was clearing level data AFTER loading in some savegame data.
		//Hacking this in here so things make a little more sense and so that data loaded
		//in from main.json is somewhat safe from being reset by mistake.
		if (!levReleaseAll())
		{
			debug(LOG_ERROR, "Failed to unload old data. Attempting to load anyway");
		}

		//remove the file extension
		CurrentFileName[strlen(CurrentFileName) - 4] = '\0';
		loadMainFile(std::string(CurrentFileName) + "/main.json");

		bool retVal = gameLoadV(fileHandle, fileHeader.version, gamJsonSave);
		PHYSFS_close(fileHandle);

		loadMainFileFinal(std::string(CurrentFileName) + "/main.json");

		challengeFileName = "";
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
static void endian_SaveGameV(SAVE_GAME *psSaveGame, UDWORD version)
{
	unsigned int i;
	/* SAVE_GAME is GAME_SAVE_V33 */
	/* GAME_SAVE_V33 includes GAME_SAVE_V31 */
	if (version >= VERSION_33)
	{
		endian_udword(&psSaveGame->sGame.power);
		endian_udword(&psSaveGame->sNetPlay.playercount);
		endian_udword(&psSaveGame->savePlayer);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			endian_udword(&psSaveGame->sPlayerIndex[i]);
		}
	}
	/* GAME_SAVE_V31 includes GAME_SAVE_V30 */
	if (version >= VERSION_31)
	{
		endian_sdword(&psSaveGame->missionCheatTime);
	}
	/* GAME_SAVE_V30 includes GAME_SAVE_V29 */
	if (version >= VERSION_30)
	{
		endian_sdword(&psSaveGame->scrGameLevel);
	}
	/* GAME_SAVE_V29 includes GAME_SAVE_V27 */
	if (version >= VERSION_29)
	{
		endian_uword(&psSaveGame->missionScrollMinX);
		endian_uword(&psSaveGame->missionScrollMinY);
		endian_uword(&psSaveGame->missionScrollMaxX);
		endian_uword(&psSaveGame->missionScrollMaxY);
	}
	/* GAME_SAVE_V24 includes GAME_SAVE_V22 */
	if (version >= VERSION_24)
	{
		endian_udword(&psSaveGame->reinforceTime);
	}
	/* GAME_SAVE_V19 includes GAME_SAVE_V18 */
	if (version >= VERSION_19)
	{
	}
	/* GAME_SAVE_V18 includes GAME_SAVE_V17 */
	if (version >= VERSION_18)
	{
		endian_udword(&psSaveGame->oldestVersion);
		endian_udword(&psSaveGame->validityKey);
	}
	/* GAME_SAVE_V17 includes GAME_SAVE_V16 */
	if (version >= VERSION_17)
	{
		endian_udword(&psSaveGame->objId);
	}
	/* GAME_SAVE_V16 includes GAME_SAVE_V15 */
	if (version >= VERSION_16)
	{
	}
	/* GAME_SAVE_V15 includes GAME_SAVE_V14 */
	if (version >= VERSION_15)
	{
		endian_udword(&psSaveGame->RubbleTile);
		endian_udword(&psSaveGame->WaterTile);
	}
	/* GAME_SAVE_V14 includes GAME_SAVE_V12 */
	if (version >= VERSION_14)
	{
		endian_sdword(&psSaveGame->missionOffTime);
		endian_sdword(&psSaveGame->missionETA);
		endian_uword(&psSaveGame->missionHomeLZ_X);
		endian_uword(&psSaveGame->missionHomeLZ_Y);
		endian_sdword(&psSaveGame->missionPlayerX);
		endian_sdword(&psSaveGame->missionPlayerY);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
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
	if (version >= VERSION_12)
	{
		endian_udword(&psSaveGame->missionTime);
		endian_udword(&psSaveGame->saveKey);
	}
	/* GAME_SAVE_V11 includes GAME_SAVE_V10 */
	if (version >= VERSION_11)
	{
		endian_sdword(&psSaveGame->currentPlayerPos.p.x);
		endian_sdword(&psSaveGame->currentPlayerPos.p.y);
		endian_sdword(&psSaveGame->currentPlayerPos.p.z);
		endian_sdword(&psSaveGame->currentPlayerPos.r.x);
		endian_sdword(&psSaveGame->currentPlayerPos.r.y);
		endian_sdword(&psSaveGame->currentPlayerPos.r.z);
	}
	/* GAME_SAVE_V10 includes GAME_SAVE_V7 */
	if (version >= VERSION_10)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			endian_udword(&psSaveGame->power[i].currentPower);
			endian_udword(&psSaveGame->power[i].extractedPower);
		}
	}
	/* GAME_SAVE_V7 */
	if (version >= VERSION_7)
	{
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
static UDWORD getCampaignV(PHYSFS_file *fileHandle, unsigned int version, nonstd::optional<nlohmann::json> &gamJson)
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
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGame, sizeof(SAVE_GAME_V14)) != sizeof(SAVE_GAME_V14))
		{
			debug(LOG_ERROR, "getCampaignV: error while reading file: %s", WZ_PHYSFS_getLastError());

			return 0;
		}

		// Convert from little-endian to native byte-order
		endian_SaveGameV((SAVE_GAME *)&saveGame, VERSION_14);
	}
	else
 	if (version <= CURRENT_VERSION_NUM)
	{
		if (gamJson.has_value())
		{
			debug(LOG_SAVEGAME, "loading campaign from gam json");
			deserializeSaveGameV14Data_json(gamJson.value(), &saveGame);
		}
		else
		{
			if (!deserializeSaveGameV14Data(fileHandle, &saveGame))
			{
				debug(LOG_ERROR, "getCampaignV: error while reading file: %s", WZ_PHYSFS_getLastError());

				return 0;
			}
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
/// 2021-11: This appears to be useless, as scripts set the campaign number... Probably can & should remove this completely?
UDWORD getCampaign(const char *fileName)
{
	GAME_SAVEHEADER fileHeader;
	auto gamJson = readGamJson(fileName);
	PHYSFS_file *fileHandle = openLoadFile(fileName, false);
	if (!fileHandle)
	{
		// Failure to open the file *may NOT be* a failure to load the specified savegame
		// TODO: If this really is needed, we need to add the new JSON format parsing here... but this whole function appears to be useless??
		return false;
	}

	debug(LOG_WZ, "getCampaign: %s", fileName);

	// Read the header from the file
	if (!deserializeSaveGameHeader(fileHandle, &fileHeader))
	{
		debug(LOG_ERROR, "getCampaign: error while reading header from file (%s): %s", fileName, WZ_PHYSFS_getLastError());
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
	// don't check skirmish saves.
	if (fileHeader.version <= CURRENT_VERSION_NUM)
	{
		UDWORD retVal = getCampaignV(fileHandle, fileHeader.version, gamJson);
		PHYSFS_close(fileHandle);
		return retVal;
	}
	else
	{
		debug(LOG_ERROR, "getCampaign: undefined save format version %d", fileHeader.version);
		PHYSFS_close(fileHandle);

		return 0;
	}
}

// -----------------------------------------------------------------------------------------
/* code specific to version 7 of a save game */
bool gameLoadV7(PHYSFS_file *fileHandle, nonstd::optional<nlohmann::json> &gamJson)
{
	SAVE_GAME_V7 saveGame;
	if (gamJson.has_value())
	{
		// this seems to be still used by maps/mission loading routines
		deserializeSaveGameV7Data_json(gamJson.value(), &saveGame);
	}
	else
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGame, sizeof(saveGame)) != sizeof(saveGame))
		{
			debug(LOG_ERROR, "gameLoadV7: error while reading file: %s", WZ_PHYSFS_getLastError());

			return false;
		}

		/* GAME_SAVE_V7 */
		endian_udword(&saveGame.gameTime);
		endian_udword(&saveGame.GameType);
		endian_sdword(&saveGame.ScrollMinX);
		endian_sdword(&saveGame.ScrollMinY);
		endian_udword(&saveGame.ScrollMaxX);
		endian_udword(&saveGame.ScrollMaxY);
	}

	savedGameTime = saveGame.gameTime;

	//set the scroll variables
	if (saveGame.ScrollMinX >= 0 && static_cast<uint32_t>(saveGame.ScrollMinX) > saveGame.ScrollMaxX)
	{
		debug(LOG_ERROR, "ScrollMinX (%" PRIi32 ") exceeds ScrollMaxX %" PRIu32, saveGame.ScrollMinX, saveGame.ScrollMaxX);
		uint32_t newMax = static_cast<uint32_t>(saveGame.ScrollMinX);
		saveGame.ScrollMinX = static_cast<int32_t>(saveGame.ScrollMaxX);
		saveGame.ScrollMaxX = newMax;
	}
	if (saveGame.ScrollMinY >= 0 && static_cast<uint32_t>(saveGame.ScrollMinY) > saveGame.ScrollMaxY)
	{
		debug(LOG_ERROR, "ScrollMinY (%" PRIi32 ") exceeds ScrollMaxY %" PRIu32, saveGame.ScrollMinY, saveGame.ScrollMaxY);
		uint32_t newMax = static_cast<uint32_t>(saveGame.ScrollMinY);
		saveGame.ScrollMinY = static_cast<int32_t>(saveGame.ScrollMaxY);
		saveGame.ScrollMaxY = newMax;
	}
	startX = saveGame.ScrollMinX;
	startY = saveGame.ScrollMinY;
	width = saveGame.ScrollMaxX - saveGame.ScrollMinX;
	height = saveGame.ScrollMaxY - saveGame.ScrollMinY;
	gameType = static_cast<GAME_TYPE>(saveGame.GameType);
	//set IsScenario to true if not a user saved game
	if (gameType == GTYPE_SAVE_START)
	{
		LEVEL_DATASET *psNewLevel;

		IsScenario = false;
		//copy the level name across
		sstrcpy(aLevelName, saveGame.levelName);
		//load up the level dataset
		if (!levLoadData(aLevelName, nullptr, saveGameName, (GAME_TYPE)gameType))
		{
			return false;
		}
		// find the level dataset
		psNewLevel = levFindDataSet(aLevelName);
		if (psNewLevel == nullptr)
		{
			debug(LOG_ERROR, "gameLoadV7: couldn't find level data");

			return false;
		}
		//check to see whether mission automatically starts
		//shouldn't be able to be any other value at the moment!
		if (psNewLevel->type == LEVEL_TYPE::LDS_CAMSTART
		    || psNewLevel->type == LEVEL_TYPE::LDS_BETWEEN
		    || psNewLevel->type == LEVEL_TYPE::LDS_EXPAND
		    || psNewLevel->type == LEVEL_TYPE::LDS_EXPAND_LIMBO)
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
bool gameLoadV(PHYSFS_file *fileHandle, unsigned int version, nonstd::optional<nlohmann::json> &gamJson)
{
	unsigned int i, j;
	static	SAVE_POWER	powerSaved[MAX_PLAYERS];
	UDWORD			player;

	debug(LOG_WZ, "gameLoadV: version %u", version);

	// Version 7 and earlier are loaded separately in gameLoadV7

	//size is now variable so only check old save games
	if (version <= VERSION_10)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V10)) != sizeof(SAVE_GAME_V10))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version == VERSION_11)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V11)) != sizeof(SAVE_GAME_V11))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_12)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V12)) != sizeof(SAVE_GAME_V12))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_14)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V14)) != sizeof(SAVE_GAME_V14))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_15)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V15)) != sizeof(SAVE_GAME_V15))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_16)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V16)) != sizeof(SAVE_GAME_V16))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_17)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V17)) != sizeof(SAVE_GAME_V17))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_18)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V18)) != sizeof(SAVE_GAME_V18))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_19)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V19)) != sizeof(SAVE_GAME_V19))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_21)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V20)) != sizeof(SAVE_GAME_V20))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_23)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V22)) != sizeof(SAVE_GAME_V22))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_26)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V24)) != sizeof(SAVE_GAME_V24))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_28)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V27)) != sizeof(SAVE_GAME_V27))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_29)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V29)) != sizeof(SAVE_GAME_V29))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_30)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V30)) != sizeof(SAVE_GAME_V30))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_32)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V31)) != sizeof(SAVE_GAME_V31))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_33)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V33)) != sizeof(SAVE_GAME_V33))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

			return false;
		}
	}
	else if (version <= VERSION_34)
	{
		if (WZ_PHYSFS_readBytes(fileHandle, &saveGameData, sizeof(SAVE_GAME_V34)) != sizeof(SAVE_GAME_V34))
		{
			debug(LOG_ERROR, "gameLoadV: error while reading file (with version number %u): %s", version, WZ_PHYSFS_getLastError());

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
		if (gamJson.has_value())
		{
			debug(LOG_SAVEGAME, "gam json found, loading that.");
			if (!deserializeSaveGameData_json(gamJson.value(), &saveGameData))
			{
				debug(LOG_ERROR, "failed to load gamjson");
				return false;
			}
		} 
		else
		{
			debug(LOG_SAVEGAME, "no gam json found, falling back to .gam");
			if (!deserializeSaveGameData(fileHandle, &saveGameData))
			{
				debug(LOG_ERROR, "gameLoadV: error while reading data from file for deserialization (with version number %u): %s", version, WZ_PHYSFS_getLastError());
				return false;
			}
		}
	}
	else
	{
		debug(LOG_ERROR, "Unsupported version number (%u) for savegame", version);

		return false;
	}

	debug(LOG_SAVE, "Savegame is of type: %u", static_cast<uint8_t>(saveGameData.sGame.type));
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

	//set the scroll variables
	if (saveGameData.ScrollMinX >= 0 && static_cast<uint32_t>(saveGameData.ScrollMinX) > saveGameData.ScrollMaxX)
	{
		debug(LOG_ERROR, "ScrollMinX (%" PRIi32 ") exceeds ScrollMaxX %" PRIu32, saveGameData.ScrollMinX, saveGameData.ScrollMaxX);
		uint32_t newMax = static_cast<uint32_t>(saveGameData.ScrollMinX);
		saveGameData.ScrollMinX = static_cast<int32_t>(saveGameData.ScrollMaxX);
		saveGameData.ScrollMaxX = newMax;
	}
	if (saveGameData.ScrollMinY >= 0 && static_cast<uint32_t>(saveGameData.ScrollMinY) > saveGameData.ScrollMaxY)
	{
		debug(LOG_ERROR, "ScrollMinY (%" PRIi32 ") exceeds ScrollMaxY %" PRIu32, saveGameData.ScrollMinY, saveGameData.ScrollMaxY);
		uint32_t newMax = static_cast<uint32_t>(saveGameData.ScrollMinY);
		saveGameData.ScrollMinY = static_cast<int32_t>(saveGameData.ScrollMaxY);
		saveGameData.ScrollMaxY = newMax;
	}
	startX = saveGameData.ScrollMinX;
	startY = saveGameData.ScrollMinY;
	width = saveGameData.ScrollMaxX - saveGameData.ScrollMinX;
	height = saveGameData.ScrollMaxY - saveGameData.ScrollMinY;
	gameType = static_cast<GAME_TYPE>(saveGameData.GameType);

	if (version >= VERSION_11)
	{
		//camera position
		disp3d_setView(&saveGameData.currentPlayerPos);
	}
	else
	{
		disp3d_oldView();
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
		unsynchObjID = (saveGameData.objId + 1) / 2; // Make new object ID start at savedObjId*8.
		synchObjID   = saveGameData.objId * 4;      // Make new object ID start at savedObjId*8.
		savedObjId = saveGameData.objId;
	}

	if (version >= VERSION_19)//version 19
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				alliances[i][j] = saveGameData.alliances[i][j];
			}
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			setPlayerColour(i, saveGameData.playerColour[i]);
		}
	}

	if (version >= VERSION_20)//version 20
	{
		setDroidsToSafetyFlag(saveGameData.bDroidsToSafetyFlag);
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

	if (saveGameVersion >= VERSION_31)
	{
		mission.cheatTime = saveGameData.missionCheatTime;
	}

	droidInit();

	//set IsScenario to true if not a user saved game
	if ((gameType == GTYPE_SAVE_START) ||
	    (gameType == GTYPE_SAVE_MIDMISSION))
	{
		for (i = 0; i < MAX_PLAYERS; ++i)
		{
			powerSaved[i].currentPower = saveGameData.power[i].currentPower;
			powerSaved[i].extractedPower = saveGameData.power[i].extractedPower;
		}

		allocatePlayers();

		IsScenario = false;
		//copy the level name across
		sstrcpy(aLevelName, saveGameData.levelName);
		//load up the level dataset
		// Not sure what aLevelName is, in relation to game.map. But need to use aLevelName here, to be able to start the right map for campaign, and need game.hash, to start the right non-campaign map, if there are multiple identically named maps.
		if (!levLoadData(aLevelName, &saveGameData.sGame.hash, saveGameName, (GAME_TYPE)gameType))
		{
			return false;
		}

		if (saveGameVersion >= VERSION_33)
		{
			PLAYERSTATS		playerStats;
			uint32_t scav = game.scavengers; // loaded earlier, keep it over struct copy below

			bMultiPlayer	= saveGameData.multiPlayer;
			bMultiMessages	= bMultiPlayer;
			productionPlayer = selectedPlayer;
			game			= saveGameData.sGame;  // why do this again????
			game.scavengers = scav;
			NetPlay.bComms = saveGameData.sNetPlay.bComms;
			if (bMultiPlayer)
			{
				loadMultiStats(saveGameData.sPName, &playerStats);				// stats stuff
				setMultiStats(selectedPlayer, playerStats, false);
				setMultiStats(selectedPlayer, playerStats, true);
			}
		}
	}
	else
	{
		IsScenario = true;
	}

	getPlayerNames();

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
// Load main game data from JSON. Only implement stuff here that we actually use instead of
// the binary blobbery.
static bool loadMainFile(const std::string &fileName)
{
	WzConfig save(WzString::fromUtf8(fileName), WzConfig::ReadOnly);

	if (save.contains("gameType"))
	{
		game.type = static_cast<LEVEL_TYPE>(save.value("gameType").toInt());
	}
	if (save.contains("scavengers"))
	{
		auto saveScavValue = save.value("scavengers").toUInt();
		if (saveScavValue <= ULTIMATE_SCAVENGERS)
		{
			game.scavengers = static_cast<uint8_t>(saveScavValue);
		}
		else
		{
			debug(LOG_ERROR, "Invalid scavengers value: %u", saveScavValue);
		}
	}
	if (save.contains("maxPlayers"))
	{
		game.maxPlayers = save.value("maxPlayers").toUInt();
	}
	if (save.contains("mapHasScavengers"))
	{
		game.mapHasScavengers = save.value("mapHasScavengers").toBool();
	}
	if (save.contains("playerBuiltHQ"))
	{
		playerBuiltHQ = save.value("playerBuiltHQ").toBool();
	}
	if (save.contains("challengeFileName"))
	{
		challengeFileName = save.string("challengeFileName");
	}
	if (save.contains("challengeActive"))
	{
		challengeActive = save.value("challengeActive").toBool();
	}
	if (save.contains("builtInMap"))
	{
		builtInMap = save.value("builtInMap").toBool();
	}
	if (save.contains("inactivityMinutes"))
	{
		game.inactivityMinutes = save.value("inactivityMinutes").toUInt();
	}

	save.beginArray("players");
	while (save.remainingArrayItems() > 0)
	{
		int index = save.value("index").toInt();
		if (!(index >= 0 && index < MAX_PLAYERS))
		{
			debug(LOG_ERROR, "Invalid player index: %d", index);
			save.nextArrayItem();
			continue;
		}
		unsigned int FactionValue = save.value("faction", static_cast<uint8_t>(FACTION_NORMAL)).toUInt();
		NetPlay.players[index].faction = static_cast<FactionID>(FactionValue);
		save.nextArrayItem();
	}
	save.endArray();

	return true;
}

static bool loadMainFileFinal(const std::string &fileName)
{
	WzConfig save(WzString::fromUtf8(fileName), WzConfig::ReadOnly);

	if (save.contains("techLevel"))
	{
		game.techLevel = save.value("techLevel").toInt();
	}

	save.beginArray("players");
	while (save.remainingArrayItems() > 0)
	{
		int index = save.value("index").toInt();
		if (!(index >= 0 && index < MAX_PLAYERS))
		{
			debug(LOG_ERROR, "Invalid player index: %d", index);
			save.nextArrayItem();
			continue;
		}
		auto value = save.value("recycled_droids").jsonValue();
		for (const auto &v : value)
		{
			add_to_experience_queue(index, json_variant(v).toInt());
		}
		setMultiPlayRecentScore(index, save.value("recentScore", 0).toUInt());
		setMultiPlayUnitsKilled(index, save.value("recentKills", 0).toUInt());
		save.nextArrayItem();
	}
	save.endArray();

	return true;
}

// -----------------------------------------------------------------------------------------
// Save main game data to JSON. We save more here than we need to, and duplicate some of the
// binary blobbery, for future usage.
static bool writeMainFile(const std::string &fileName, SDWORD saveType)
{
	ASSERT(saveType == GTYPE_SAVE_START || saveType == GTYPE_SAVE_MIDMISSION, "invalid save type");

	WzConfig save(WzString::fromUtf8(fileName), WzConfig::ReadAndWrite);

	uint32_t saveKey = getCampaignNumber();
	if (missionIsOffworld())
	{
		saveKey |= SAVEKEY_ONMISSION;
		saveGameOnMission = true;
	}
	else
	{
		saveGameOnMission = false;
	}

	/* Put the save game data into the buffer */
	save.setValue("version", currentGameVersion); // game version save was made on
	save.setValue("versionFile", 2); // version of this file. Bump for significant changes
	save.setValue("saveKey", saveKey);
	save.setValue("gameTime", gameTime);
	save.setValue("missionTime", mission.startTime);
	save.setVector2i("scrollMin", Vector2i(scrollMinX, scrollMinY));
	save.setVector2i("scrollMax", Vector2i(scrollMaxX, scrollMaxY));
	save.setValue("saveType", saveType);
	ASSERT_OR_RETURN(false, strlen(aLevelName) < MAX_LEVEL_SIZE, "Unable to save level name - too long (max %d) - %s", (int)MAX_LEVEL_SIZE, aLevelName);
	save.setValue("levelName", aLevelName);
	save.setValue("radarPermitted", radarPermitted);
	save.setValue("allowDesign", allowDesign);
	save.setValue("missionOffTime", mission.time);
	save.setValue("missionETA", mission.ETA);
	save.setValue("missionCheatTime", mission.cheatTime);
	save.setVector2i("missionHomeLZ", Vector2i(mission.homeLZ_X, mission.homeLZ_Y));
	save.setVector2i("missionPlayerPos", Vector2i(mission.playerX, mission.playerY));
	save.setVector2i("missionScrollMin", Vector2i(mission.scrollMinX, mission.scrollMinY));
	save.setVector2i("missionScrollMax", Vector2i(mission.scrollMaxX, mission.scrollMaxY));
	save.setValue("offWorldKeepLists", offWorldKeepLists);
	save.setValue("rubbleTile", getRubbleTileNum());
	save.setValue("waterTile", getWaterTileNum());
	save.setValue("objId", MAX(unsynchObjID * 2, (synchObjID + 3) / 4));
	save.setValue("radarZoom", GetRadarZoom());
	save.setValue("droidsToSafetyFlag", getDroidsToSafetyFlag());
	save.setValue("reinforceTime", missionGetReinforcementTime());
	save.setValue("playCountDown", getPlayCountDown());

	save.beginArray("players");
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		save.setValue("index", i);
		save.setValue("power", getPower(i));
		save.setVector2i("iTranspEntryTile", Vector2i(mission.iTranspEntryTileX[i], mission.iTranspEntryTileY[i]));
		save.setVector2i("iTranspExitTile", Vector2i(mission.iTranspExitTileX[i], mission.iTranspExitTileY[i]));
		save.setValue("aDefaultSensor", aDefaultSensor[i]);
		save.setValue("aDefaultECM", aDefaultECM[i]);
		save.setValue("aDefaultRepair", aDefaultRepair[i]);

		std::priority_queue<int> experience = copy_experience_queue(i);
		nlohmann::json recycled_droids = nlohmann::json::array();
		while (!experience.empty())
		{
			recycled_droids.push_back(experience.top());
			experience.pop();
		}
		save.set("recycled_droids", recycled_droids);

		nlohmann::json allies = nlohmann::json::array();
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			allies.push_back(alliances[i][j]);
		}
		save.set("alliances", allies);
		save.setValue("difficulty", NetPlay.players[i].difficulty);
		save.setValue("position", NetPlay.players[i].position);
		save.setValue("colour", getPlayerColour(i));
		save.setValue("allocated", NetPlay.players[i].allocated);
		save.setValue("faction", NetPlay.players[i].faction);
		save.setValue("team", NetPlay.players[i].team);
		save.setValue("ai", NetPlay.players[i].ai);
		save.setValue("autoGame", NetPlay.players[i].autoGame);
		save.setValue("ip", NetPlay.players[i].IPtextAddress);
		save.setValue("name", getPlayerName(i));
		save.setValue("nameAI", getAIName(i));
		save.setValue("recentScore", getMultiPlayRecentScore(i));
		save.setValue("recentKills", getMultiPlayUnitsKilled(i));

		save.nextArrayItem();
	}
	save.endArray();

	iView currPlayerPos;
	disp3d_getView(&currPlayerPos);
	save.setVector3i("camera_position", currPlayerPos.p);
	save.setVector3i("camera_rotation", currPlayerPos.r);

	save.beginArray("landing_zones");
	for (int i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		LANDING_ZONE *psLandingZone = getLandingZone(i);
		save.setVector2i("start", Vector2i(psLandingZone->x1, psLandingZone->y1));
		save.setVector2i("end", Vector2i(psLandingZone->x2, psLandingZone->y2));
		save.nextArrayItem();
	}
	save.endArray();

	save.setValue("playerHasWon", testPlayerHasWon());
	save.setValue("playerHasLost", testPlayerHasLost());
	save.setValue("gameType", game.type);
	save.setValue("scavengers", game.scavengers);
	save.setValue("mapName", game.map);
	save.setValue("maxPlayers", game.maxPlayers);
	save.setValue("gameName", game.name);
	save.setValue("powerSetting", game.power);
	save.setValue("baseSetting", game.base);
	save.setValue("allianceSetting", game.alliance);
	save.setValue("mapHasScavengers", game.mapHasScavengers);
	save.setValue("mapMod", game.isMapMod);
	save.setValue("mapRandom", game.isRandom);
	save.setValue("techLevel", game.techLevel);
	save.setValue("gameHash", game.hash.toString());
	save.setValue("selectedPlayer", selectedPlayer);
	save.setValue("multiplayer", bMultiPlayer);
	save.setValue("playerCount", NetPlay.playercount);
	save.setValue("hostPlayer", NetPlay.hostPlayer);
	save.setValue("bComms", NetPlay.bComms);
	save.setValue("modList", getModList().c_str());
	save.setValue("playerBuiltHQ", playerBuiltHQ);
	save.setValue("challengeFileName", challengeFileName.toUtf8().c_str());
	save.setValue("challengeActive", challengeActive);
	save.setValue("builtInMap", builtInMap);
	save.setValue("inactivityMinutes", game.inactivityMinutes);

	return true;
}

//-- Write gam.json
//--
static bool writeGameFile(const char *fileName, SDWORD saveType)
{
	GAME_SAVEHEADER fileHeader;
	SAVE_GAME       saveGame;
	unsigned int    i, j;

	fileHeader.aFileType[0] = 'g';
	fileHeader.aFileType[1] = 'a';
	fileHeader.aFileType[2] = 'm';
	fileHeader.aFileType[3] = 'e';

	fileHeader.version = CURRENT_VERSION_NUM;

	debug(LOG_SAVE, "fileversion is %u, (%s) ", fileHeader.version, fileName);

	ASSERT(saveType == GTYPE_SAVE_START || saveType == GTYPE_SAVE_MIDMISSION, "invalid save type");
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
	saveGame.missionScrollMinX = (UWORD)mission.scrollMinX;
	saveGame.missionScrollMinY = (UWORD)mission.scrollMinY;
	saveGame.missionScrollMaxX = (UWORD)mission.scrollMaxX;
	saveGame.missionScrollMaxY = (UWORD)mission.scrollMaxY;

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
	}

	for (i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		LANDING_ZONE *psLandingZone = getLandingZone(i);
		saveGame.sLandingZone[i].x1	= psLandingZone->x1; // in case struct changes
		saveGame.sLandingZone[i].x2	= psLandingZone->x2;
		saveGame.sLandingZone[i].y1	= psLandingZone->y1;
		saveGame.sLandingZone[i].y2	= psLandingZone->y2;
	}

	//version 17
	saveGame.objId = MAX(unsynchObjID * 2, (synchObjID + 3) / 4);

	//version 18
	memset(saveGame.buildDate, 0, sizeof(saveGame.buildDate));
	saveGame.oldestVersion = 0;
	saveGame.validityKey = 0;

	//version 19
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			saveGame.alliances[i][j] = alliances[i][j];
		}
	}
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		saveGame.playerColour[i] = getPlayerColour(i);
	}
	saveGame.radarZoom = (UBYTE)GetRadarZoom();

	//version 20
	saveGame.bDroidsToSafetyFlag = (UBYTE)getDroidsToSafetyFlag();

	//version 24
	saveGame.reinforceTime = missionGetReinforcementTime();
	saveGame.bPlayCountDown = (UBYTE)getPlayCountDown();
	saveGame.bPlayerHasWon = (UBYTE)testPlayerHasWon();
	saveGame.bPlayerHasLost = (UBYTE)testPlayerHasLost();

	//version 30
	saveGame.scrGameLevel = 0;
	saveGame.bExtraFailFlag = 0;
	saveGame.bExtraVictoryFlag = 0;
	saveGame.bTrackTransporter = 0;

	// version 33
	saveGame.sGame		= game;
	saveGame.savePlayer	= selectedPlayer;
	saveGame.multiPlayer = bMultiPlayer;
	saveGame.sNetPlay	= NetPlay;
	sstrcpy(saveGame.sPName, getPlayerName(selectedPlayer));
	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		saveGame.sPlayerIndex[i] = i;
	}

	//version 34
	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		sstrcpy(saveGame.sPlayerName[i], (!challengeActive && NetPlay.players[i].ai >= 0 && !NetPlay.players[i].allocated) ? getAIName(i) : getPlayerName(i));
	}

	//version 38
	sstrcpy(saveGame.modList, getModList().c_str());
	// Attempt to see if we have a corrupted game structure in campaigns.
	if (saveGame.sGame.type == LEVEL_TYPE::CAMPAIGN)
	{
		// player 0 is always a human in campaign games
		for (i = 1; i < MAX_PLAYERS; i++)
		{
			if (saveGame.sNetPlay.players[i].difficulty == AIDifficulty::HUMAN)
			{
				ASSERT(!"savegame corruption!", "savegame corruption!");
				debug(LOG_ERROR, "Savegame corruption detected, trying to salvage.  Please Report this issue @ wz2100.net");
				debug(LOG_ERROR, "players[i].difficulty was %d, level %s / %s, ", (int) static_cast<int8_t>(saveGame.sNetPlay.players[i].difficulty), saveGame.levelName, saveGame.sGame.map);
				saveGame.sNetPlay.players[i].difficulty = AIDifficulty::DISABLED;
			}
		}
	}
	const std::string fileNameStr(fileName);
	const auto len = fileNameStr.size();
	ASSERT(strcmp(fileName + (len - 4), ".gam") == 0, "hmm... not .gam?");
	
	const auto lastSep = fileNameStr.rfind("/");
	// TODO: convert argument "const char* filename" to a manageable struct with path/filename/extension
	//		 and remove this mess...
	const std::string pathToCommonSaveDir(fileNameStr, 0, lastSep + 1);
	const std::string gameName(fileNameStr, lastSep + 1, len - pathToCommonSaveDir.size() - 4);
	const std::string pathToThisSaveDir = pathToCommonSaveDir + gameName + "/";
	if (!PHYSFS_exists(pathToThisSaveDir.c_str()))
	{
		PHYSFS_mkdir(pathToThisSaveDir.c_str());
	}
	const std::string saveInfoJsonFilename = pathToThisSaveDir + "save-info.json";
	const std::string jsonFileName = pathToThisSaveDir + "gam.json";
	auto gamJson = nlohmann::json::object();

	nonstd::optional<nlohmann::json> saveInfoJsonOpt = nullopt;
	if (PHYSFS_exists(saveInfoJsonFilename.c_str()))
	{
		saveInfoJsonOpt = parseJsonFile(saveInfoJsonFilename.c_str());
		ASSERT(saveInfoJsonOpt.has_value() && saveInfoJsonOpt.value().is_object(), "save-info.json looks broken, wanted an object");
	}
	if (!saveInfoJsonOpt.has_value() || !saveInfoJsonOpt.value().is_object())
	{
		saveInfoJsonOpt = nlohmann::json::object();
	}
	auto saveInfoJson = saveInfoJsonOpt.value();
	// new .json format
	serializeSaveGameData_json(gamJson, saveInfoJson, gameName.c_str(), &saveGame);
	if (!saveJSONToFile(gamJson, jsonFileName.c_str()))
	{
		debug(LOG_ERROR, "Failed to save: %s", jsonFileName.c_str());
		return false;
	}
	if (!saveJSONToFile(saveInfoJson, saveInfoJsonFilename.c_str()))
	{
		debug(LOG_ERROR, "Failed to save: %s", saveInfoJsonFilename.c_str());
		return false;
	}
	debug(LOG_SAVEGAME, "saved game into %s", jsonFileName.c_str());

	// Return our success status with writing out the file!
	return true;
}

static nonstd::optional<nlohmann::json> readGamJson(const char* filenameWithGamExtension)
{
	// TODO: use SaveGamePath_t instead of char* ...
	// but that's a lot of change, so not doing it now
	unsigned len = strlen(filenameWithGamExtension);
	ASSERT(strcmp(filenameWithGamExtension + (len - 4), ".gam") == 0, "hmm... not .gam?");
	const std::string filenameWithGamExtensionStr(filenameWithGamExtension);
	const auto lastSep = filenameWithGamExtensionStr.rfind("/");
	ASSERT(lastSep > 0, "unexpected filename format: '%s'", filenameWithGamExtension);
	const std::string lastSegment(filenameWithGamExtension, lastSep + 1, filenameWithGamExtensionStr.size() - lastSep - 5);
	const std::string commonPath(filenameWithGamExtension, 0, filenameWithGamExtensionStr.size() - lastSegment.size() - 4);
	const std::string containingFolder = commonPath + lastSegment + "/";
	// "gam.json" in the containingFolder
	const std::string gamJson = containingFolder + "gam.json";
	auto result = parseJsonFile(gamJson.c_str());
	if (result.has_value())
	{
		debug(LOG_SAVEGAME, "last segment was %s, common path %s, save-info %s", lastSegment.c_str(), commonPath.c_str(), gamJson.c_str());
	}
	else
	{
		// FALLBACK for older saves: prior behavior was to save "<lastSegment>.json" in the containingFolder
		const std::string oldGamJsonPath = containingFolder + lastSegment + ".json";
		debug(LOG_SAVEGAME, "last segment was %s, common path %s, save-info %s", lastSegment.c_str(), commonPath.c_str(), oldGamJsonPath.c_str());
		result = parseJsonFile(oldGamJsonPath.c_str());
	}
	return result;
}

nonstd::optional<nlohmann::json> parseJsonFile(const char *filename)
{
	UDWORD pFileSize;
	char *ppFileData = nullptr;
	debug(LOG_SAVEGAME, "starting deserialize %s", filename);
	if (!loadFile(filename, &ppFileData, &pFileSize, false))
	{
		debug(LOG_SAVE, "No %s found, sad", filename);
		return nullopt;
	}
	nonstd::optional<nlohmann::json> result;
	try {
		result = nlohmann::json::parse(ppFileData);
	}
	catch (const std::exception &e) {
		ASSERT(false, "JSON document from %s is invalid: %s", filename, e.what());
		result = nullopt;
	}
	free(ppFileData);
	return result;
}

static uint32_t RemapWzMapPlayerNumber(int8_t oldNumber)
{
	if (oldNumber < 0)
	{
		game.mapHasScavengers = true;
		return static_cast<uint32_t>(scavengerSlot());
	}

	if (game.type == LEVEL_TYPE::CAMPAIGN)		// don't remap for SP games
	{
		return oldNumber;
	}

	for (uint32_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (oldNumber == NetPlay.players[i].position)
		{
			game.mapHasScavengers = game.mapHasScavengers || i == scavengerSlot();
			return i;
		}
	}
	ASSERT(false, "Found no player position for player %d", (int)oldNumber);
	return 0;
}

static bool loadWzMapDroidInit(WzMap::Map &wzMap)
{
	uint32_t NumberOfSkippedDroids = 0;
	auto pDroids = wzMap.mapDroids();
	ASSERT_OR_RETURN(false, pDroids.get() != nullptr, "No data.");

	for (auto &droid : *pDroids)
	{
		unsigned player = RemapWzMapPlayerNumber(droid.player);
		if (player >= MAX_PLAYERS)
		{
			player = MAX_PLAYERS - 1;	// now don't lose any droids ... force them to be the last player
			NumberOfSkippedDroids++;
		}
		auto psTemplate = getTemplateFromTranslatedNameNoPlayer(droid.name.c_str());
		if (psTemplate == nullptr)
		{
			debug(LOG_ERROR, "Unable to find template for %s for player %d -- unit skipped", droid.name.c_str(), player);
			continue;
		}
		turnOffMultiMsg(true);
		DROID *psDroid = nullptr;
		if (droid.id.has_value() && droid.id.value() > 0)
		{
			psDroid = reallyBuildDroid(psTemplate, Position(droid.position.x, droid.position.y, 0), player, false, {droid.direction, 0, 0}, droid.id.value());
		} else
		{
			psDroid = reallyBuildDroid(psTemplate, Position(droid.position.x, droid.position.y, 0), player, false, {droid.direction, 0, 0});
		}
		turnOffMultiMsg(false);
		if (psDroid == nullptr)
		{
			debug(LOG_ERROR, "Failed to build unit %s", droid.name.c_str());
			continue;
		}
		ASSERT(psDroid->id != 0, "Droid ID should never be zero here");

		// HACK!!
		Vector2i startpos = getPlayerStartPosition(player);
		if (psDroid->droidType == DROID_CONSTRUCT && startpos.x == 0 && startpos.y == 0)
		{
			scriptSetStartPos(psDroid->player, psDroid->pos.x, psDroid->pos.y);	// set map start position, FIXME - save properly elsewhere!
		}

		addDroid(psDroid, apsDroidLists);
	}
	if (NumberOfSkippedDroids)
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

	if (game.type == LEVEL_TYPE::CAMPAIGN)		// don't remap for SP games
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
		json_variant result = ini.value("player");
		if (result.toWzString().startsWith("scavenger"))
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

static inline void setPlayerJSON(nlohmann::json &jsonObj, int player)
{
	if (scavengerSlot() == player)
	{
		jsonObj["player"] = "scavenger";
	}
	else
	{
		jsonObj["player"] = player;
	}
}

static bool loadSaveDroidPointers(const WzString &pFileName, DROID **ppsCurrentDroidLists)
{
	WzConfig ini(pFileName, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();

	for (size_t i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID *psDroid;
		int id = ini.value("id", -1).toInt();
		int player = getPlayer(ini);

		if (id <= 0)
		{
			ini.endGroup();
			continue; // special hack for campaign missions, cannot have targets
		}

		for (psDroid = ppsCurrentDroidLists[player]; psDroid && psDroid->id != id; psDroid = psDroid->psNext)
		{
			if (isTransporter(psDroid) && psDroid->psGroup != nullptr)  // Check for droids in the transporter.
			{
				for (DROID *psTrDroid = psDroid->psGroup->psList; psTrDroid != nullptr; psTrDroid = psTrDroid->psGrpNext)
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
			if (psDroid)
			{
				debug(LOG_ERROR, "Droid %s (%d) was in wrong file/list (was in %s)...", objInfo(psDroid), id, pFileName.toUtf8().c_str());
			}
		}
		ASSERT_OR_RETURN(false, psDroid, "Droid %d not found", id);
		psDroid->listSize = clip(ini.value("orderList/size", 0).toInt(), 0, 10000);
		psDroid->asOrderList.resize(psDroid->listSize);  // Must resize before setting any orders, and must set in-place, since pointers are updated later.
		for (int droidIdx = 0; droidIdx < psDroid->listSize; ++droidIdx)
		{
			getIniDroidOrder(ini, "orderList/" + WzString::number(droidIdx), psDroid->asOrderList[droidIdx]);
		}
		psDroid->listPendingBegin = 0;
		for (int j = 0; j < MAX_WEAPONS; j++)
		{
			objTrace(psDroid->id, "weapon %d, nStat %d", j, psDroid->asWeaps[j].nStat);
			getIniBaseObject(ini, "actionTarget/" + WzString::number(j), psDroid->psActionTarget[j]);
		}
		if (ini.contains("baseStruct/id"))
		{
			int tid = ini.value("baseStruct/id", -1).toInt();
			int tplayer = ini.value("baseStruct/player", -1).toInt();
			OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value("baseStruct/type", 0).toInt();
			ASSERT(tid >= 0 && tplayer >= 0, "Bad ID");
			BASE_OBJECT *psObj = getBaseObjFromData(tid, tplayer, ttype);
			ASSERT(psObj, "Failed to find droid base structure");
			ASSERT(!psObj || psObj->type == OBJ_STRUCTURE, "Droid base structure not a structure");
			setSaveDroidBase(psDroid, (STRUCTURE *)psObj);
		}
		if (ini.contains("commander"))
		{
			int tid = ini.value("commander", -1).toInt();
			DROID *psCommander = (DROID *)getBaseObjFromData(tid, psDroid->player, OBJ_DROID);
			ASSERT(psCommander, "Failed to find droid commander");
			cmdDroidAddDroid(psCommander, psDroid);
		}
		getIniDroidOrder(ini, "order", psDroid->order);
		ini.endGroup();
	}
	return true;
}

static int healthValue(WzConfig &ini, int defaultValue)
{
	WzString health = ini.value("health").toWzString();
	if (health.isEmpty() || defaultValue == 0)
	{
		return defaultValue;
	}
	else if (health.contains(WzUniCodepoint::fromASCII('%')))
	{
		int perc = health.replace("%", "").toInt();
		return MAX(defaultValue * perc / 100, 1); //hp not supposed to be 0
	}
	else
	{
		return MIN(health.toInt(), defaultValue);
	}
}

static void loadSaveObject(WzConfig &ini, BASE_OBJECT *psObj)
{
	psObj->died = ini.value("died", 0).toInt();
	memset(psObj->visible, 0, sizeof(psObj->visible));
	for (int j = 0; j < game.maxPlayers; j++)
	{
		psObj->visible[j] = ini.value("visible/" + WzString::number(j), 0).toInt();
	}
	psObj->periodicalDamage = ini.value("periodicalDamage", 0).toInt();
	psObj->periodicalDamageStart = ini.value("periodicalDamageStart", 0).toInt();
	psObj->timeAnimationStarted = ini.value("timeAnimationStarted", 0).toInt();
	psObj->animationEvent = ini.value("animationEvent", 0).toInt();
	psObj->timeLastHit = ini.value("timeLastHit", UDWORD_MAX).toInt();
	psObj->lastEmission = ini.value("lastEmission", 0).toInt();
	psObj->selected = ini.value("selected", false).toBool();
	psObj->born = ini.value("born", 2).toInt();
}

static void writeSaveObject(WzConfig &ini, BASE_OBJECT *psObj)
{
	ini.setValue("id", psObj->id);
	setPlayer(ini, psObj->player);
	ini.setValue("health", psObj->body);
	ini.setVector3i("position", psObj->pos);
	ini.setVector3i("rotation", toVector(psObj->rot));
	if (psObj->timeAnimationStarted)
	{
		ini.setValue("timeAnimationStarted", psObj->timeAnimationStarted);
	}
	if (psObj->animationEvent)
	{
		ini.setValue("animationEvent", psObj->animationEvent);
	}
	ini.setValue("selected", psObj->selected);	// third kind of group
	if (psObj->lastEmission)
	{
		ini.setValue("lastEmission", psObj->lastEmission);
	}
	if (psObj->periodicalDamageStart > 0)
	{
		ini.setValue("periodicalDamageStart", psObj->periodicalDamageStart);
	}
	if (psObj->periodicalDamage > 0)
	{
		ini.setValue("periodicalDamage", psObj->periodicalDamage);
	}
	ini.setValue("born", psObj->born);
	if (psObj->died > 0)
	{
		ini.setValue("died", psObj->died);
	}
	if (psObj->timeLastHit != UDWORD_MAX)
	{
		ini.setValue("timeLastHit", psObj->timeLastHit);
	}
	if (psObj->selected)
	{
		ini.setValue("selected", psObj->selected);
	}
	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (psObj->visible[i])
		{
			ini.setValue("visible/" + WzString::number(i), psObj->visible[i]);
		}
	}
}

static void writeSaveObjectJSON(nlohmann::json &jsonObj, BASE_OBJECT *psObj)
{
	jsonObj["id"] = psObj->id;
	setPlayerJSON(jsonObj, psObj->player);
	jsonObj["health"] = psObj->body;
	jsonObj["position"] = psObj->pos;
	jsonObj["rotation"] = toVector(psObj->rot);
	if (psObj->timeAnimationStarted)
	{
		jsonObj["timeAnimationStarted"] = psObj->timeAnimationStarted;
	}
	if (psObj->animationEvent)
	{
		jsonObj["animationEvent"] = psObj->animationEvent;
	}
	jsonObj["selected"] = psObj->selected;	// third kind of group
	if (psObj->lastEmission)
	{
		jsonObj["lastEmission"] = psObj->lastEmission;
	}
	if (psObj->periodicalDamageStart > 0)
	{
		jsonObj["periodicalDamageStart"] = psObj->periodicalDamageStart;
	}
	if (psObj->periodicalDamage > 0)
	{
		jsonObj["periodicalDamage"] = psObj->periodicalDamage;
	}
	jsonObj["born"] = psObj->born;
	if (psObj->died > 0)
	{
		jsonObj["died"] = psObj->died;
	}
	if (psObj->timeLastHit != UDWORD_MAX)
	{
		jsonObj["timeLastHit"] = psObj->timeLastHit;
	}
	if (psObj->selected)
	{
		jsonObj["selected"] = psObj->selected;
	}
	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (psObj->visible[i])
		{
			jsonObj["visible/" + WzString::number(i).toStdString()] = psObj->visible[i];
		}
	}
}

template<typename T>
inline T getCompFromName_NullCompOnFail(COMPONENT_TYPE compType, const WzString &name)
{
	int index = getCompFromName(compType, name);
	return (index >= 0) ? static_cast<T>(index) : 0; // 0 to reference the null weapon / body / etc
}

static bool loadSaveDroid(const char *pFileName, DROID **ppsCurrentDroidLists)
{
	if (!PHYSFS_exists(pFileName))
	{
		debug(LOG_SAVE, "No %s found -- use fallback method", pFileName);
		return false;	// try to use fallback method
	}
	WzString fName = WzString::fromUtf8(pFileName);
	WzConfig ini(fName, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();
	// Sort list so transports are loaded first, since they must be loaded before the droids they contain.
	std::vector<std::pair<int, WzString>> sortedList;
	bool missionList = fName.compare("mdroid");
	for (size_t i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID_TYPE droidType = (DROID_TYPE)ini.value("droidType").toInt();
		int priority = 0;
		switch (droidType)
		{
		case DROID_TRANSPORTER:
			++priority; // fallthrough
		case DROID_SUPERTRANSPORTER:
			++priority; // fallthrough
		case DROID_COMMAND:
			//Don't care about sorting commanders in the mission list for safety missions. They
			//don't have a group to command and it messes up the order of the list sorting them
			//which causes problems getting the first transporter group for Gamma-1.
			if (!missionList || (missionList && !getDroidsToSafetyFlag()))
			{
				++priority;
			}
		default:
			break;
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
		int id = ini.value("id", -1).toInt();
		Position pos = ini.vector3i("position");
		Rotation rot = ini.vector3i("rotation");
		bool onMission = ini.value("onMission", false).toBool();
		DROID_TEMPLATE templ;
		const DROID_TEMPLATE *psTemplate = nullptr;

		if (ini.contains("template"))
		{
			// Use real template (for maps)
			WzString templName(ini.value("template").toWzString());
			psTemplate = getTemplateFromTranslatedNameNoPlayer(templName.toUtf8().c_str());
			if (psTemplate == nullptr)
			{
				debug(LOG_ERROR, "Unable to find template for %s for player %d -- unit skipped", templName.toUtf8().c_str(), player);
				ini.endGroup();
				continue;
			}
		}
		else
		{
			// Create fake template
			templ.name = ini.string("name", "UNKNOWN");
			templ.droidType = (DROID_TYPE)ini.value("droidType").toInt();
			templ.numWeaps = ini.value("weapons", 0).toInt();
			ini.beginGroup("parts");	// the following is copy-pasted from loadSaveTemplate() -- fixme somehow
			templ.asParts[COMP_BODY] = getCompFromName_NullCompOnFail<uint8_t>(COMP_BODY, ini.value("body", "ZNULLBODY").toWzString());
			templ.asParts[COMP_BRAIN] = getCompFromName_NullCompOnFail<uint8_t>(COMP_BRAIN, ini.value("brain", "ZNULLBRAIN").toWzString());
			templ.asParts[COMP_PROPULSION] = getCompFromName_NullCompOnFail<uint8_t>(COMP_PROPULSION, ini.value("propulsion", "ZNULLPROP").toWzString());
			templ.asParts[COMP_REPAIRUNIT] = getCompFromName_NullCompOnFail<uint8_t>(COMP_REPAIRUNIT, ini.value("repair", "ZNULLREPAIR").toWzString());
			templ.asParts[COMP_ECM] = getCompFromName_NullCompOnFail<uint8_t>(COMP_ECM, ini.value("ecm", "ZNULLECM").toWzString());
			templ.asParts[COMP_SENSOR] = getCompFromName_NullCompOnFail<uint8_t>(COMP_SENSOR, ini.value("sensor", "ZNULLSENSOR").toWzString());
			templ.asParts[COMP_CONSTRUCT] = getCompFromName_NullCompOnFail<uint8_t>(COMP_CONSTRUCT, ini.value("construct", "ZNULLCONSTRUCT").toWzString());
			templ.asWeaps[0] = getCompFromName_NullCompOnFail<uint32_t>(COMP_WEAPON, ini.value("weapon/1", "ZNULLWEAPON").toWzString());
			templ.asWeaps[1] = getCompFromName_NullCompOnFail<uint32_t>(COMP_WEAPON, ini.value("weapon/2", "ZNULLWEAPON").toWzString());
			templ.asWeaps[2] = getCompFromName_NullCompOnFail<uint32_t>(COMP_WEAPON, ini.value("weapon/3", "ZNULLWEAPON").toWzString());
			ini.endGroup();
			psTemplate = &templ;
		}

		// If droid is on a mission, calling with the saved position might cause an assertion. Or something like that.
		if (!onMission)
		{
			pos.x = clip(pos.x, world_coord(1), world_coord(mapWidth - 1));
			pos.y = clip(pos.y, world_coord(1), world_coord(mapHeight - 1));
		}

		/* Create the Droid */
		turnOffMultiMsg(true);
		if (id > 0)
		{
			psDroid = reallyBuildDroid(psTemplate, pos, player, onMission, rot, id);
		} else
		{
			// will generate a new id
			psDroid = reallyBuildDroid(psTemplate, pos, player, onMission, rot);
		}
		ASSERT_OR_RETURN(false, psDroid != nullptr, "Failed to build unit %s", sortedList[i].second.toUtf8().c_str());
		turnOffMultiMsg(false);
		ASSERT(id != 0, "Droid ID should never be zero here");
		// conditional check so that existing saved games don't break
		if (ini.contains("originalBody"))
		{
			// we need to set "originalBody" before setting "body", otherwise CHECK_DROID throws assertion errors
			// we cannot use droidUpgradeBody here to calculate "originalBody", because upgrades aren't loaded yet
			// so it's much simplier just store/retrieve originalBody value
			psDroid->originalBody = ini.value("originalBody").toInt();
		}
		psDroid->body = healthValue(ini, psDroid->originalBody);
		ASSERT(psDroid->body != 0, "%s : %d has zero hp!", pFileName, i);
		psDroid->experience = ini.value("experience", 0).toInt();
		psDroid->kills = ini.value("kills", 0).toInt();
		psDroid->secondaryOrder = ini.value("secondaryOrder", psDroid->secondaryOrder).toInt();
		psDroid->secondaryOrderPending = psDroid->secondaryOrder;
		psDroid->action = (DROID_ACTION)ini.value("action", DACTION_NONE).toInt();
		psDroid->actionPos = ini.vector2i("action/pos");
		psDroid->actionStarted = ini.value("actionStarted", 0).toInt();
		psDroid->actionPoints = ini.value("actionPoints", 0).toInt();
		psDroid->resistance = ini.value("resistance", 0).toInt(); // zero resistance == no electronic damage
		psDroid->lastFrustratedTime = ini.value("lastFrustratedTime", 0).toInt();

		// common BASE_OBJECT info
		loadSaveObject(ini, psDroid);

		// copy the droid's weapon stats
		for (int j = 0; j < psDroid->numWeaps; j++)
		{
			if (psDroid->asWeaps[j].nStat > 0)
			{
				psDroid->asWeaps[j].ammo = ini.value("ammo/" + WzString::number(j)).toInt();
				psDroid->asWeaps[j].lastFired = ini.value("lastFired/" + WzString::number(j)).toInt();
				psDroid->asWeaps[j].shotsFired = ini.value("shotsFired/" + WzString::number(j)).toInt();
				psDroid->asWeaps[j].rot = ini.vector3i("rotation/" + WzString::number(j));
			}
		}

		psDroid->group = ini.value("group", UBYTE_MAX).toInt();
		int aigroup = ini.value("aigroup", -1).toInt();
		if (aigroup >= 0)
		{
			DROID_GROUP *psGroup = grpFind(aigroup);
			psGroup->add(psDroid);
			if (psGroup->type == GT_TRANSPORTER)
			{
				psDroid->selected = false;  // Droid should be visible in the transporter interface.
				visRemoveVisibility(psDroid); // should not have visibility data when in a transporter
			}
		}
		else
		{
			if (isTransporter(psDroid) || psDroid->droidType == DROID_COMMAND)
			{
				DROID_GROUP *psGroup = grpCreate();
				psGroup->add(psDroid);
			}
			else
			{
				psDroid->psGroup = nullptr;
			}
		}

		psDroid->sMove.Status = (MOVE_STATUS)ini.value("moveStatus", 0).toInt();
		psDroid->sMove.pathIndex = ini.value("pathIndex", 0).toInt();
		const int numPoints = ini.value("pathLength", 0).toInt();
		psDroid->sMove.asPath.resize(numPoints);
		for (int j = 0; j < numPoints; j++)
		{
			psDroid->sMove.asPath[j] = ini.vector2i("pathNode/" + WzString::number(j));
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
		for (int j = 0; j < MAX_WEAPONS; ++j)
		{
			psDroid->asWeaps[j].usedAmmo = ini.value("attackRun/" + WzString::number(j)).toInt();
		}
		psDroid->sMove.lastBump = ini.value("lastBump").toInt();
		psDroid->sMove.pauseTime = ini.value("pauseTime").toInt();
		Vector2i tmp = ini.vector2i("bumpPosition");
		psDroid->sMove.bumpPos = Vector3i(tmp.x, tmp.y, 0);

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

		if (psDroid->psGroup == nullptr || psDroid->psGroup->type != GT_TRANSPORTER || isTransporter(psDroid))  // do not add to list if on a transport, then the group list is used instead
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
static nlohmann::json writeDroid(DROID *psCurr, bool onMission, int &counter)
{
	nlohmann::json droidObj = nlohmann::json::object();
	droidObj["name"] = psCurr->aName;
	droidObj["originalBody"] = psCurr->originalBody;
	// write common BASE_OBJECT info
	writeSaveObjectJSON(droidObj, psCurr);

	for (unsigned i = 0; i < psCurr->numWeaps; i++)
	{
		if (psCurr->asWeaps[i].nStat > 0)
		{
			auto numberWzStr = WzString::number(i);
			const std::string& numStr = numberWzStr.toStdString();
			droidObj["ammo/" + numStr] = psCurr->asWeaps[i].ammo;
			droidObj["lastFired/" + numStr] = psCurr->asWeaps[i].lastFired;
			droidObj["shotsFired/" + numStr] = psCurr->asWeaps[i].shotsFired;
			droidObj["rotation/" + numStr] = toVector(psCurr->asWeaps[i].rot);
		}
	}
	for (unsigned i = 0; i < MAX_WEAPONS; i++)
	{
		setIniBaseObject(droidObj, "actionTarget/" + WzString::number(i), psCurr->psActionTarget[i]);
	}
	if (psCurr->lastFrustratedTime > 0)
	{
		droidObj["lastFrustratedTime"] = psCurr->lastFrustratedTime;
	}
	if (psCurr->experience > 0)
	{
		droidObj["experience"] = psCurr->experience;
	}
	if (psCurr->kills > 0)
	{
		droidObj["kills"] = psCurr->kills;
	}

	setIniDroidOrder(droidObj, "order", psCurr->order);
	droidObj["orderList/size"] = psCurr->listSize;
	for (int i = 0; i < psCurr->listSize; ++i)
	{
		setIniDroidOrder(droidObj, "orderList/" + WzString::number(i), psCurr->asOrderList[i]);
	}
	if (psCurr->timeLastHit != UDWORD_MAX)
	{
		droidObj["timeLastHit"] = psCurr->timeLastHit;
	}
	droidObj["secondaryOrder"] = psCurr->secondaryOrder;
	droidObj["action"] = psCurr->action;
	droidObj["actionString"] = getDroidActionName(psCurr->action); // future-proofing
	droidObj["action/pos"] = psCurr->actionPos;
	droidObj["actionStarted"] = psCurr->actionStarted;
	droidObj["actionPoints"] = psCurr->actionPoints;
	if (psCurr->psBaseStruct != nullptr)
	{
		droidObj["baseStruct/id"] = psCurr->psBaseStruct->id;
		droidObj["baseStruct/player"] = psCurr->psBaseStruct->player;	// always ours, but for completeness
		droidObj["baseStruct/type"] = psCurr->psBaseStruct->type;		// always a building, but for completeness
	}
	if (psCurr->psGroup)
	{
		droidObj["aigroup"] = psCurr->psGroup->id;	// AI and commander/transport group
		droidObj["aigroup/type"] = psCurr->psGroup->type;
	}
	droidObj["group"] = psCurr->group;	// different kind of group. of course.
	if (hasCommander(psCurr) && psCurr->psGroup->psCommander->died <= 1)
	{
		droidObj["commander"] = psCurr->psGroup->psCommander->id;
	}
	if (psCurr->resistance > 0)
	{
		droidObj["resistance"] = psCurr->resistance;
	}
	droidObj["droidType"] = psCurr->droidType;
	droidObj["weapons"] = psCurr->numWeaps;
	nlohmann::json partsObj = nlohmann::json::object();
	partsObj["body"] = (asBodyStats + psCurr->asBits[COMP_BODY])->id;
	partsObj["propulsion"] = (asPropulsionStats + psCurr->asBits[COMP_PROPULSION])->id;
	partsObj["brain"] = (asBrainStats + psCurr->asBits[COMP_BRAIN])->id;
	partsObj["repair"] = (asRepairStats + psCurr->asBits[COMP_REPAIRUNIT])->id;
	partsObj["ecm"] = (asECMStats + psCurr->asBits[COMP_ECM])->id;
	partsObj["sensor"] = (asSensorStats + psCurr->asBits[COMP_SENSOR])->id;
	partsObj["construct"] = (asConstructStats + psCurr->asBits[COMP_CONSTRUCT])->id;
	for (int j = 0; j < psCurr->numWeaps; j++)
	{
		partsObj["weapon/" + WzString::number(j + 1).toStdString()] = (asWeaponStats + psCurr->asWeaps[j].nStat)->id;
	}
	droidObj["parts"] = partsObj;
	droidObj["moveStatus"] = psCurr->sMove.Status;
	droidObj["pathIndex"] = psCurr->sMove.pathIndex;
	droidObj["pathLength"] = psCurr->sMove.asPath.size();
	for (unsigned i = 0; i < psCurr->sMove.asPath.size(); i++)
	{
		droidObj["pathNode/" + WzString::number(i).toStdString()] = psCurr->sMove.asPath[i];
	}
	droidObj["moveDestination"] = psCurr->sMove.destination;
	droidObj["moveSource"] = psCurr->sMove.src;
	droidObj["moveTarget"] = psCurr->sMove.target;
	droidObj["moveSpeed"] = psCurr->sMove.speed;
	droidObj["moveDirection"] = psCurr->sMove.moveDir;
	droidObj["bumpDir"] = psCurr->sMove.bumpDir;
	droidObj["vertSpeed"] = psCurr->sMove.iVertSpeed;
	droidObj["bumpTime"] = psCurr->sMove.bumpTime;
	droidObj["shuffleStart"] = psCurr->sMove.shuffleStart;
	for (int i = 0; i < MAX_WEAPONS; ++i)
	{
		droidObj["attackRun/" + WzString::number(i).toStdString()] = psCurr->asWeaps[i].usedAmmo;
	}
	droidObj["lastBump"] = psCurr->sMove.lastBump;
	droidObj["pauseTime"] = psCurr->sMove.pauseTime;
	droidObj["bumpPosition"] = psCurr->sMove.bumpPos.xy();
	droidObj["onMission"] = onMission;
	return droidObj;
}

static bool writeDroidFile(const char *pFileName, DROID **ppsCurrentDroidLists)
{
	nlohmann::json mRoot = nlohmann::json::object();
	int counter = 0;
	bool onMission = (ppsCurrentDroidLists[0] == mission.apsDroidLists[0]);

	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		for (DROID *psCurr = ppsCurrentDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			auto droidKey = "droid_" + (WzString::number(counter++).leftPadToMinimumLength(WzUniCodepoint::fromASCII('0'), 10));  // Zero padded so that alphabetical sort works.
			mRoot[droidKey.toStdString()] = writeDroid(psCurr, onMission, counter);
			if (isTransporter(psCurr))	// if transporter save any droids in the grp
			{
				for (DROID *psTrans = psCurr->psGroup->psList; psTrans != nullptr; psTrans = psTrans->psGrpNext)
				{
					if (psTrans != psCurr)
					{
						droidKey = "droid_" + (WzString::number(counter++).leftPadToMinimumLength(WzUniCodepoint::fromASCII('0'), 10));  // Zero padded so that alphabetical sort works.
						mRoot[droidKey.toStdString()] = writeDroid(psTrans, onMission, counter);
					}
				}
				//always save transporter droids that are in the mission list with an invalid value
				if (ppsCurrentDroidLists[player] == mission.apsDroidLists[player])
				{
					mRoot[droidKey.toStdString()]["position"] = Vector3i(INVALID_XY, INVALID_XY, -1); // Must be INVALID_XY or else unit placement could get messed up in missionResetDroids().
				}
			}
		}
	}

	return saveJSONToFile(mRoot, pFileName);
}


// -----------------------------------------------------------------------------------------
bool loadSaveStructure(char *pFileData, UDWORD filesize)
{
	STRUCT_SAVEHEADER		*psHeader;
	SAVE_STRUCTURE_V2		*psSaveStructure, sSaveStructure;
	STRUCTURE			*psStructure;
	STRUCTURE_STATS			*psStats = nullptr;
	UDWORD				count, statInc;
	int32_t				found;
	UDWORD				NumberOfSkippedStructures = 0;
	UDWORD				periodicalDamageTime;

	/* Check the file type */
	psHeader = (STRUCT_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 's' || psHeader->aFileType[1] != 't' ||
	    psHeader->aFileType[2] != 'r' || psHeader->aFileType[3] != 'u')
	{
		debug(LOG_ERROR, "loadSaveStructure: Incorrect file type");

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
		debug(LOG_ERROR, "StructLoad: unsupported save format version %d", psHeader->version);

		return false;
	}

	psSaveStructure = &sSaveStructure;

	if ((sizeof(SAVE_STRUCTURE_V2) * psHeader->quantity + STRUCT_HEADER_SIZE) > filesize)
	{
		debug(LOG_ERROR, "structureLoad: unexpected end of file");
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
		endian_udword(&psSaveStructure->periodicalDamageStart);
		endian_udword(&psSaveStructure->periodicalDamage);

		psSaveStructure->player = RemapPlayerNumber(psSaveStructure->player);

		if (psSaveStructure->player >= MAX_PLAYERS)
		{
			psSaveStructure->player = MAX_PLAYERS - 1;
			NumberOfSkippedStructures++;
		}
		//get the stats for this structure
		found = false;

		for (statInc = 0; statInc < numStructureStats; statInc++)
		{
			psStats = asStructureStats + statInc;
			//loop until find the same name

			if (psStats->id.compare(psSaveStructure->name) == 0)
			{
				found = true;
				break;
			}
		}
		//if haven't found the structure - ignore this record!
		if (!found)
		{
			debug(LOG_ERROR, "This structure no longer exists - %s", getSaveStructNameV19((SAVE_STRUCTURE_V17 *)psSaveStructure));
			//ignore this
			continue;
		}

		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			psStructure = getTileStructure(map_coord(psSaveStructure->x), map_coord(psSaveStructure->y));
			if (psStructure == nullptr)
			{
				debug(LOG_ERROR, "No owning structure for module - %s for player - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17 *)psSaveStructure), psSaveStructure->player);
				//ignore this module
				continue;
			}
		}

		//check not trying to build too near the edge
		if (map_coord(psSaveStructure->x) < TOO_NEAR_EDGE || map_coord(psSaveStructure->x) > mapWidth - TOO_NEAR_EDGE)
		{
			debug(LOG_ERROR, "Structure %s, x coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17 *)psSaveStructure), psSaveStructure->id);
			//ignore this
			continue;
		}
		if (map_coord(psSaveStructure->y) < TOO_NEAR_EDGE || map_coord(psSaveStructure->y) > mapHeight - TOO_NEAR_EDGE)
		{
			debug(LOG_ERROR, "Structure %s, y coord too near the edge of the map. id - %d", getSaveStructNameV19((SAVE_STRUCTURE_V17 *)psSaveStructure), psSaveStructure->id);
			//ignore this
			continue;
		}

		psStructure = buildStructureDir(psStats, psSaveStructure->x, psSaveStructure->y, DEG(psSaveStructure->direction), psSaveStructure->player, true);
		ASSERT(psStructure, "Unable to create structure");
		if (!psStructure)
		{
			continue;
		}

		psStructure->periodicalDamage = psSaveStructure->periodicalDamage;
		periodicalDamageTime = psSaveStructure->periodicalDamageStart;
		psStructure->periodicalDamageStart = periodicalDamageTime;
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

	if (NumberOfSkippedStructures > 0)
	{
		debug(LOG_ERROR, "structureLoad: invalid player number in %d structures ... assigned to the last player!\n\n", NumberOfSkippedStructures);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//return id of a research topic based on the name
static UDWORD getResearchIdFromName(const WzString &name)
{
	for (size_t inc = 0; inc < asResearch.size(); inc++)
	{
		if (asResearch[inc].id.compare(name) == 0)
		{
			return inc;
		}
	}
	debug(LOG_ERROR, "Unknown research - %s", name.toUtf8().c_str());
	return NULL_ID;
}

static bool loadWzMapStructure(WzMap::Map& wzMap)
{
	uint32_t NumberOfSkippedStructures = 0;
	auto pStructures = wzMap.mapStructures();
	if (!pStructures)
	{
		return false;
	}

	for (auto &structure : *pStructures) {
		auto psStats = std::find_if(asStructureStats, asStructureStats + numStructureStats, [&](STRUCTURE_STATS &stat) { return stat.id.compare(structure.name.c_str()) == 0; });
		if (psStats == asStructureStats + numStructureStats)
		{
			debug(LOG_ERROR, "Structure type \"%s\" unknown", structure.name.c_str());
			continue;  // ignore this
		}
		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			STRUCTURE *psStructure = getTileStructure(map_coord(structure.position.x), map_coord(structure.position.y));
			if (psStructure == nullptr)
			{
				debug(LOG_ERROR, "No owning structure for module - %s for player - %d", structure.name.c_str(), structure.player);
				continue; // ignore this module
			}
		}
		//check not trying to build too near the edge
		if (map_coord(structure.position.x) < TOO_NEAR_EDGE || map_coord(structure.position.x) > mapWidth - TOO_NEAR_EDGE
		 || map_coord(structure.position.y) < TOO_NEAR_EDGE || map_coord(structure.position.y) > mapHeight - TOO_NEAR_EDGE)
		{
			debug(LOG_ERROR, "Structure %s, coord too near the edge of the map", structure.name.c_str());
			continue; // skip it
		}
		auto player = RemapWzMapPlayerNumber(structure.player);
		if (player >= MAX_PLAYERS)
		{
			player = MAX_PLAYERS - 1;
			NumberOfSkippedStructures++;
		}
		STRUCTURE *psStructure = nullptr;
		debug(LOG_NEVER, "trying to build structure %i;%i;%s;%i;%i", (structure.id.has_value()) ? structure.id.value() : -1, player, 
				structure.name.c_str(), map_coord(structure.position.x), map_coord(structure.position.y));
		if (structure.id.has_value() && structure.id.value() > 0)
		{
			psStructure = buildStructureDir(psStats, structure.position.x, structure.position.y, structure.direction, player, true, structure.id.value());
		} else
		{
			// generate new synchronised id
			psStructure = buildStructureDir(psStats, structure.position.x, structure.position.y, structure.direction, player, true);
		}
		
		if (psStructure == nullptr)
		{
			debug(LOG_ERROR, "Structure %s couldn't be built (probably on top of another structure).", structure.name.c_str());
			continue;
		}
		// Previously, we would override building's ID with module's ID
		// now, "id" is const, we can't do that. 
		// this may break some mods which look up structures by theirs module id.
		if (structure.modules > 0)
		{
			auto moduleStat = getModuleStat(psStructure);
			if (moduleStat == nullptr)
			{
				debug(LOG_ERROR, "Structure %s can't have modules.", structure.name.c_str());
				continue;
			}
			for (int i = 0; i < structure.modules; ++i)
			{
				buildStructure(moduleStat, structure.position.x, structure.position.y, player, true);
			}
		}
		buildingComplete(psStructure);
		if (psStructure->pStructureType->type == REF_HQ)
		{
			scriptSetStartPos(player, psStructure->pos.x, psStructure->pos.y);
		}
		else if (psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			scriptSetDerrickPos(psStructure->pos.x, psStructure->pos.y);
		}
	}

	if (NumberOfSkippedStructures > 0)
	{
		debug(LOG_ERROR, "structureLoad: invalid player number in %d structures ... assigned to the last player!\n\n", NumberOfSkippedStructures);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
/* code for versions after version 20 of a save structure */
static bool loadSaveStructure2(const char *pFileName)
{
	if (!PHYSFS_exists(pFileName))
	{
		debug(LOG_SAVE, "No %s found -- use fallback method", pFileName);
		return false;	// try to use fallback method
	}
	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadOnly);

	freeAllFlagPositions();		//clear any flags put in during level loads

	std::vector<WzString> list = ini.childGroups();
	for (size_t i = 0; i < list.size(); ++i)
	{
		FACTORY *psFactory;
		RESEARCH_FACILITY *psResearch;
		REPAIR_FACILITY *psRepair;
		REARM_PAD *psReArmPad;
		STRUCTURE_STATS *psModule;
		int capacity, researchId;
		STRUCTURE *psStructure;

		ini.beginGroup(list[i]);
		int player = getPlayer(ini);
		int id = ini.value("id", -1).toInt();
		Position pos = ini.vector3i("position");
		Rotation rot = ini.vector3i("rotation");
		WzString name = ini.string("name");

		//get the stats for this structure
		auto psStats = std::find_if(asStructureStats, asStructureStats + numStructureStats, [&](STRUCTURE_STATS &stat) { return stat.id == name; });
		//if haven't found the structure - ignore this record!
		ASSERT(psStats != asStructureStats + numStructureStats, "This structure no longer exists - %s", name.toUtf8().c_str());
		if (psStats == asStructureStats + numStructureStats)
		{
			ini.endGroup();
			continue;	// ignore this
		}
		/*create the Structure */
		//for modules - need to check the base structure exists
		if (IsStatExpansionModule(psStats))
		{
			STRUCTURE *psTileStructure = getTileStructure(map_coord(pos.x), map_coord(pos.y));
			if (psTileStructure == nullptr)
			{
				debug(LOG_ERROR, "No owning structure for module - %s for player - %d", name.toUtf8().c_str(), player);
				ini.endGroup();
				continue; // ignore this module
			}
		}
		//check not trying to build too near the edge
		if (map_coord(pos.x) < TOO_NEAR_EDGE || map_coord(pos.x) > mapWidth - TOO_NEAR_EDGE
		    || map_coord(pos.y) < TOO_NEAR_EDGE || map_coord(pos.y) > mapHeight - TOO_NEAR_EDGE)
		{
			debug(LOG_ERROR, "Structure %s (%s), coord too near the edge of the map", name.toUtf8().c_str(), list[i].toUtf8().c_str());
			ini.endGroup();
			continue; // skip it
		}
		if (id <= 0)
		{
			id = generateSynchronisedObjectId();
		}
		debug(LOG_NEVER, "trying to build structure %i;%i;%s;%i;%i", id, player, psStats->name.toUtf8().c_str(), map_coord(pos.y), map_coord(pos.y));
		psStructure = buildStructureDir(psStats, pos.x, pos.y, rot.direction, player, true, id);
		ASSERT(psStructure, "Unable to create structure");
		if (!psStructure)
		{
			ini.endGroup();
			continue;
		}
		// common BASE_OBJECT info
		loadSaveObject(ini, psStructure);

		if (psStructure->pStructureType->type == REF_HQ)
		{
			scriptSetStartPos(player, psStructure->pos.x, psStructure->pos.y);
		}
		psStructure->resistance = ini.value("resistance", psStructure->resistance).toInt();
		capacity = ini.value("modules", 0).toInt();
		psStructure->capacity = 0; // increased when modules are built
		switch (psStructure->pStructureType->type)
		{
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
		case REF_CYBORG_FACTORY:
			//if factory save the current build info
			psFactory = ((FACTORY *)psStructure->pFunctionality);
			psFactory->productionLoops = ini.value("Factory/productionLoops", psFactory->productionLoops).toUInt();
			psFactory->timeStarted = ini.value("Factory/timeStarted", psFactory->timeStarted).toInt();
			psFactory->buildPointsRemaining = ini.value("Factory/buildPointsRemaining", psFactory->buildPointsRemaining).toInt();
			psFactory->timeStartHold = ini.value("Factory/timeStartHold", psFactory->timeStartHold).toInt();
			psFactory->loopsPerformed = ini.value("Factory/loopsPerformed", psFactory->loopsPerformed).toInt();
			// statusPending and pendingCount belong to the GUI, not the game state.
			psFactory->secondaryOrder = ini.value("Factory/secondaryOrder", psFactory->secondaryOrder).toInt();
			//adjust the module structures IMD
			if (capacity)
			{
				psModule = getModuleStat(psStructure);
				//build the appropriate number of modules
				for (int moduleIdx = 0; moduleIdx < capacity; moduleIdx++)
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
			if (ini.contains("Factory/assemblyPoint/number"))
			{
				psFactory->psAssemblyPoint->factoryInc = ini.value("Factory/assemblyPoint/number", 42).toInt();
			}
			if (player == productionPlayer)
			{
				for (int runNum = 0; runNum < ini.value("Factory/productionRuns", 0).toInt(); runNum++)
				{
					ProductionRunEntry currentProd;
					currentProd.quantity = ini.value("Factory/Run/" + WzString::number(runNum) + "/quantity").toInt();
					currentProd.built = ini.value("Factory/Run/" + WzString::number(runNum) + "/built").toInt();
					if (ini.contains("Factory/Run/" + WzString::number(runNum) + "/template"))
					{
						int tid = ini.value("Factory/Run/" + WzString::number(runNum) + "/template").toInt();
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
			}
			break;
		case REF_RESEARCH:
			psResearch = ((RESEARCH_FACILITY *)psStructure->pFunctionality);
			//adjust the module structures IMD
			if (capacity)
			{
				psModule = getModuleStat(psStructure);
				buildStructure(psModule, psStructure->pos.x, psStructure->pos.y, psStructure->player, true);
			}
			//clear subject
			psResearch->psSubject = nullptr;
			psResearch->timeStartHold = 0;
			//set the subject
			if (ini.contains("Research/target"))
			{
				researchId = getResearchIdFromName(ini.value("Research/target").toWzString());
				if (researchId != NULL_ID)
				{
					psResearch->psSubject = &asResearch[researchId];
					psResearch->timeStartHold = ini.value("Research/timeStartHold").toInt();
				}
				else
				{
					debug(LOG_ERROR, "Failed to look up research target %s", ini.value("Research/target").toWzString().toUtf8().c_str());
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
			if (ini.contains("Repair/deliveryPoint/pos"))
			{
				Position point = ini.vector3i("Repair/deliveryPoint/pos");
				setAssemblyPoint(psRepair->psDeliveryPoint, point.x, point.y, player, true);
				psRepair->psDeliveryPoint->selected = ini.value("Repair/deliveryPoint/selected", false).toBool();
			}
			break;
		case REF_REARM_PAD:
			psReArmPad = ((REARM_PAD *)psStructure->pFunctionality);
			psReArmPad->timeStarted = ini.value("Rearm/timeStarted", psReArmPad->timeStarted).toInt();
			psReArmPad->timeLastUpdated = ini.value("Rearm/timeLastUpdated", psReArmPad->timeLastUpdated).toInt();
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
		psStructure->currentBuildPts = ini.value("currentBuildPts", structureBuildPointsToCompletion(*psStructure)).toInt();
		if (psStructure->status == SS_BUILT)
		{
			switch (psStructure->pStructureType->type)
			{
			case REF_POWER_GEN:
				checkForResExtractors(psStructure);
				if (selectedPlayer == psStructure->player)
				{
					audio_PlayObjStaticTrack(psStructure, ID_SOUND_POWER_HUM);
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				checkForPowerGen(psStructure);
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
				psStructure->asWeaps[j].ammo = ini.value("ammo/" + WzString::number(j)).toInt();
				psStructure->asWeaps[j].lastFired = ini.value("lastFired/" + WzString::number(j)).toInt();
				psStructure->asWeaps[j].shotsFired = ini.value("shotsFired/" + WzString::number(j)).toInt();
				psStructure->asWeaps[j].rot = ini.vector3i("rotation/" + WzString::number(j));
			}
		}
		psStructure->status = (STRUCT_STATES)ini.value("status", SS_BUILT).toInt();
		if (psStructure->status == SS_BUILT)
		{
			buildingComplete(psStructure);
		}
		ini.endGroup();
	}
	resetFactoryNumFlag();	//reset flags into the masks

	return true;
}

// -----------------------------------------------------------------------------------------
/*
Writes some version info
*/
bool writeGameInfo(const char *pFileName)
{
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();

	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadAndWrite);
	char ourtime[100] = {'\0'};
	const time_t currentTime = time(nullptr);
	std::string time(ctime(&currentTime));

	ini.beginGroup("GameProperties");
	ini.setValue("current_time", time.data());
	getAsciiTime(ourtime, graphicsTime);
	ini.setValue("graphics_time", ourtime);
	getAsciiTime(ourtime, gameTime);
	ini.setValue("game_time", ourtime);
	getAsciiTime(ourtime, gameTime - missionData.missionStarted);
	ini.setValue("playing_time", ourtime);
	ini.setValue("version", version_getVersionString());
	ini.setValue("full_version", version_getFormattedVersionString());
	ini.setValue("cheated", Cheated);
	ini.setValue("debug", dbgInputManager.debugMappingsAllowed());
	ini.setValue("level/map", getLevelName());
	ini.setValue("mods", !getModList().empty() ? getModList().c_str() : "None");
	auto backendInfo = gfx_api::context::get().getBackendGameInfo();
	for (auto& kv : backendInfo)
	{
		ini.setValue(WzString::fromUtf8(kv.first), WzString::fromUtf8(kv.second));
	}
	ini.endGroup();
	return true;
}

/*
Writes the linked list of structure for each player to a file
*/
bool writeStructFile(const char *pFileName)
{
	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadAndWrite);
	int counter = 0;

	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		for (STRUCTURE *psCurr = apsStructLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			if (!psCurr->pStructureType)
			{
				ASSERT(psCurr->pStructureType, "Structure has null pStructureType??");
				continue;
			}
			ini.beginGroup("structure_" + (WzString::number(counter++).leftPadToMinimumLength(WzUniCodepoint::fromASCII('0'), 10)));  // Zero padded so that alphabetical sort works.
			ini.setValue("name", psCurr->pStructureType->id);

			writeSaveObject(ini, psCurr);

			if (psCurr->resistance > 0)
			{
				ini.setValue("resistance", psCurr->resistance);
			}
			if (psCurr->status != SS_BUILT)
			{
				ini.setValue("status", psCurr->status);
			}
			ini.setValue("weapons", psCurr->numWeaps);
			for (unsigned j = 0; j < psCurr->numWeaps; j++)
			{
				ini.setValue("parts/weapon/" + WzString::number(j + 1), (asWeaponStats + psCurr->asWeaps[j].nStat)->id);
				if (psCurr->asWeaps[j].nStat > 0)
				{
					ini.setValue("ammo/" + WzString::number(j), psCurr->asWeaps[j].ammo);
					ini.setValue("lastFired/" + WzString::number(j), psCurr->asWeaps[j].lastFired);
					ini.setValue("shotsFired/" + WzString::number(j), psCurr->asWeaps[j].shotsFired);
					ini.setVector3i("rotation/" + WzString::number(j), toVector(psCurr->asWeaps[j].rot));
				}
			}
			for (unsigned i = 0; i < psCurr->numWeaps; i++)
			{
				if (psCurr->psTarget[i] && !psCurr->psTarget[i]->died)
				{
					ini.setValue("target/" + WzString::number(i) + "/id", psCurr->psTarget[i]->id);
					ini.setValue("target/" + WzString::number(i) + "/player", psCurr->psTarget[i]->player);
					ini.setValue("target/" + WzString::number(i) + "/type", psCurr->psTarget[i]->type);
#ifdef DEBUG
					ini.setValue("target/" + WzString::number(i) + "/debugfunc", WzString::fromUtf8(psCurr->targetFunc[i]));
					ini.setValue("target/" + WzString::number(i) + "/debugline", psCurr->targetLine[i]);
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
					ini.setValue("modules", psCurr->capacity);
					ini.setValue("Factory/productionLoops", psFactory->productionLoops);
					ini.setValue("Factory/timeStarted", psFactory->timeStarted);
					ini.setValue("Factory/buildPointsRemaining", psFactory->buildPointsRemaining);
					ini.setValue("Factory/timeStartHold", psFactory->timeStartHold);
					ini.setValue("Factory/loopsPerformed", psFactory->loopsPerformed);
					// statusPending and pendingCount belong to the GUI, not the game state.
					ini.setValue("Factory/secondaryOrder", psFactory->secondaryOrder);

					if (psFactory->psSubject != nullptr)
					{
						ini.setValue("Factory/template", psFactory->psSubject->multiPlayerID);
					}
					FLAG_POSITION *psFlag = ((FACTORY *)psCurr->pFunctionality)->psAssemblyPoint;
					if (psFlag != nullptr)
					{
						ini.setVector3i("Factory/assemblyPoint/pos", psFlag->coords);
						if (psFlag->selected)
						{
							ini.setValue("Factory/assemblyPoint/selected", psFlag->selected);
						}
						ini.setValue("Factory/assemblyPoint/number", psFlag->factoryInc);
					}
					if (psFactory->psCommander)
					{
						ini.setValue("Factory/commander/id", psFactory->psCommander->id);
						ini.setValue("Factory/commander/player", psFactory->psCommander->player);
					}
					ini.setValue("Factory/secondaryOrder", psFactory->secondaryOrder);
					if (player == productionPlayer)
					{
						ProductionRun emptyRun;
						bool haveRun = psFactory->psAssemblyPoint->factoryInc < asProductionRun[psFactory->psAssemblyPoint->factoryType].size();
						ProductionRun const &productionRun = haveRun ? asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc] : emptyRun;
						ini.setValue("Factory/productionRuns", (int)productionRun.size());
						for (size_t runNum = 0; runNum < productionRun.size(); runNum++)
						{
							ProductionRunEntry psCurrentProd = productionRun.at(runNum);
							ini.setValue("Factory/Run/" + WzString::number(runNum) + "/quantity", psCurrentProd.quantity);
							ini.setValue("Factory/Run/" + WzString::number(runNum) + "/built", psCurrentProd.built);
							if (psCurrentProd.psTemplate) ini.setValue("Factory/Run/" + WzString::number(runNum) + "/template",
										psCurrentProd.psTemplate->multiPlayerID);
						}
					}
					else
					{
						ini.setValue("Factory/productionRuns", 0);
					}
				}
				else if (psCurr->pStructureType->type == REF_RESEARCH)
				{
					ini.setValue("modules", psCurr->capacity);
					ini.setValue("Research/timeStartHold", ((RESEARCH_FACILITY *)psCurr->pFunctionality)->timeStartHold);
					if (((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject)
					{
						ini.setValue("Research/target", ((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject->id);
					}
				}
				else if (psCurr->pStructureType->type == REF_POWER_GEN)
				{
					ini.setValue("modules", psCurr->capacity);
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
						if (psFlag->selected)
						{
							ini.setValue("Repair/deliveryPoint/selected", psFlag->selected);
						}
					}
				}
				else if (psCurr->pStructureType->type == REF_REARM_PAD)
				{
					REARM_PAD *psReArmPad = ((REARM_PAD *)psCurr->pFunctionality);
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
bool loadSaveStructurePointers(const WzString& filename, STRUCTURE **ppList)
{
	WzConfig ini(filename, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();

	for (size_t i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		STRUCTURE *psStruct;
		int player = getPlayer(ini);
		int id = ini.value("id", -1).toInt();
		for (psStruct = ppList[player]; psStruct && psStruct->id != id; psStruct = psStruct->psNext) { }
		if (!psStruct)
		{
			ini.endGroup();
			continue;	// it is not unusual for a structure to 'disappear' like this; it can happen eg because of module upgrades
		}
		for (int j = 0; j < MAX_WEAPONS; j++)
		{
			objTrace(psStruct->id, "weapon %d, nStat %d", j, psStruct->asWeaps[j].nStat);
			if (ini.contains("target/" + WzString::number(j) + "/id"))
			{
				int tid = ini.value("target/" + WzString::number(j) + "/id", -1).toInt();
				int tplayer = ini.value("target/" + WzString::number(j) + "/player", -1).toInt();
				OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value("target/" + WzString::number(j) + "/type", 0).toInt();
				if (tid >= 0 && tplayer >= 0)
				{
					setStructureTarget(psStruct, getBaseObjFromData(tid, tplayer, ttype), j, ORIGIN_UNKNOWN);
					ASSERT(psStruct->psTarget[j], "Failed to find target");
				}
				else
				{
					ASSERT(tid >= 0 && tplayer >= 0, "Bad ID");
				}
			}
		}
		if (ini.contains("Factory/commander/id")) {
				ASSERT(psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
				       || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Bad type");
				FACTORY *psFactory = (FACTORY *)psStruct->pFunctionality;
				OBJECT_TYPE ttype = OBJ_DROID;
				int tid = ini.value("Factory/commander/id", -1).toInt();
				int tplayer = ini.value("Factory/commander/player", -1).toInt();
				if (tid >= 0 && tplayer >= 0)
				{
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
				else
				{
					ASSERT(tid >= 0 && tplayer >= 0, "Bad commander ID %d for player %d for building %d", tid, tplayer, id);
				}
		}
		if (ini.contains("Repair/target/id")){
				ASSERT(psStruct->pStructureType->type == REF_REPAIR_FACILITY, "Bad type");
				REPAIR_FACILITY *psRepair = ((REPAIR_FACILITY *)psStruct->pFunctionality);
				OBJECT_TYPE ttype = (OBJECT_TYPE)ini.value("Repair/target/type", OBJ_DROID).toInt();
				int tid = ini.value("Repair/target/id", -1).toInt();
				int tplayer = ini.value("Repair/target/player", -1).toInt();
				if (tid >= 0 && tplayer >= 0)
				{
					psRepair->psObj = getBaseObjFromData(tid, tplayer, ttype);
					ASSERT(psRepair->psObj, "Repair target %d not found for building %d", tid, id);
				}
				else
				{
					ASSERT(tid >= 0 && tplayer >= 0, "Bad repair ID %d for player %d for building %d", tid, tplayer, id);
					psRepair->psObj = nullptr;
				}
		}
		if (ini.contains("Rearm/target/id")) {
				ASSERT(psStruct->pStructureType->type == REF_REARM_PAD, "Bad type");
				REARM_PAD *psReArmPad = ((REARM_PAD *)psStruct->pFunctionality);
				OBJECT_TYPE ttype = OBJ_DROID; // always, for now
				int tid = ini.value("Rearm/target/id", -1).toInt();
				int tplayer = ini.value("Rearm/target/player", -1).toInt();
				if (tid >= 0 && tplayer >= 0)
				{
					psReArmPad->psObj = getBaseObjFromData(tid, tplayer, ttype);
					ASSERT(psReArmPad->psObj, "Rearm target %d not found for building %d", tid, id);
				}
				else
				{
					ASSERT(tid >= 0 && tplayer >= 0, "Bad rearm ID %d for player %d for building %d", tid, tplayer, id);
					psReArmPad->psObj = nullptr;
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
	FEATURE_STATS			*psStats = nullptr;
	bool					found;
	UDWORD					sizeOfSaveFeature;

	/* Check the file type */
	psHeader = (FEATURE_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'f' || psHeader->aFileType[1] != 'e' ||
	    psHeader->aFileType[2] != 'a' || psHeader->aFileType[3] != 't')
	{
		debug(LOG_ERROR, "loadSaveFeature: Incorrect file type");
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
		debug(LOG_ERROR, "featureLoad: unexpected end of file");
		return false;
	}

	/* Load in the feature data */
	for (count = 0; count < psHeader->quantity; count ++, pFileData += sizeOfSaveFeature)
	{
		psSaveFeature = (SAVE_FEATURE_V14 *) pFileData;

		/* FEATURE_SAVE_V14 is FEATURE_SAVE_V2 */
		/* FEATURE_SAVE_V2 is OBJECT_SAVE_V19 */
		/* OBJECT_SAVE_V19 */
		endian_udword(&psSaveFeature->id);
		endian_udword(&psSaveFeature->x);
		endian_udword(&psSaveFeature->y);
		endian_udword(&psSaveFeature->z);
		endian_udword(&psSaveFeature->direction);
		endian_udword(&psSaveFeature->player);
		endian_udword(&psSaveFeature->periodicalDamageStart);
		endian_udword(&psSaveFeature->periodicalDamage);

		//get the stats for this feature
		found = false;

		for (statInc = 0; statInc < numFeatureStats; statInc++)
		{
			psStats = asFeatureStats + statInc;
			//loop until find the same name
			if (psStats->id.compare(psSaveFeature->name) == 0)
			{
				found = true;
				break;
			}
		}
		//if haven't found the feature - ignore this record!
		if (!found)
		{
			debug(LOG_ERROR, "This feature no longer exists - %s;%i", psSaveFeature->name, psSaveFeature->id);
			//ignore this
			continue;
		}
		//create the Feature
		pFeature = buildFeature(psStats, psSaveFeature->x, psSaveFeature->y, true, psSaveFeature->id);
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
		pFeature->rot.direction = DEG(psSaveFeature->direction);
		pFeature->periodicalDamage = psSaveFeature->periodicalDamage;
		if (psHeader->version >= VERSION_14)
		{
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				pFeature->visible[i] = psSaveFeature->visible[i];
			}
		}
	}

	return true;
}

static bool loadWzMapFeature(WzMap::Map &wzMap)
{
	auto pFeatures = wzMap.mapFeatures();
	if (!pFeatures)
	{
		return false;
	}

	for (auto &feature : *pFeatures)
	{
		auto psStats = std::find_if(asFeatureStats, asFeatureStats + numFeatureStats, [&](FEATURE_STATS &stat) { return stat.id.compare(feature.name.c_str()) == 0; });
		if (psStats == asFeatureStats + numFeatureStats)
		{
			debug(LOG_ERROR, "Feature type \"%s\" unknown", feature.name.c_str());
			continue;  // ignore this
		}
		// Create the Feature
		FEATURE *pFeature = nullptr;
		//restore values && create Feature
		if (feature.id.has_value())
		{
			pFeature = buildFeature(psStats, feature.position.x, feature.position.y, true, feature.id.value());
		} else
		{
			pFeature = buildFeature(psStats, feature.position.x, feature.position.y, true);
		}
		if (!pFeature)
		{
			debug(LOG_ERROR, "Unable to create feature %s", feature.name.c_str());
			continue;
		}
		if (pFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			scriptSetDerrickPos(pFeature->pos.x, pFeature->pos.y);
		}

		pFeature->rot.direction = feature.direction;
		pFeature->player = (feature.player.has_value()) ? feature.player.value() : PLAYER_FEATURE;
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
	WzConfig ini(pFileName, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();
	debug(LOG_SAVE, "Loading new style features (%zu found)", list.size());

	for (size_t i = 0; i < list.size(); ++i)
	{
		FEATURE *pFeature;
		ini.beginGroup(list[i]);
		WzString name = ini.string("name");
		Position pos = ini.vector3i("position");
		int statInc;
		bool found = false;
		FEATURE_STATS *psStats = nullptr;

		//get the stats for this feature
		for (statInc = 0; statInc < numFeatureStats; statInc++)
		{
			psStats = asFeatureStats + statInc;
			//loop until find the same name
			if (psStats->id.compare(name) == 0)
			{
				found = true;
				break;
			}
		}
		//if haven't found the feature - ignore this record!
		if (!found)
		{
			debug(LOG_ERROR, "This feature no longer exists - %s", name.toUtf8().c_str());
			//ignore this
			continue;
		}
		//create the Feature
		int id = ini.value("id", -1).toInt();
		if (id > 0)
		{
			pFeature = buildFeature(psStats, pos.x, pos.y, true, id);
		}
		else
		{
			pFeature = buildFeature(psStats, pos.x, pos.y, true);
		}
		if (!pFeature)
		{
			debug(LOG_ERROR, "Unable to create feature %s", name.toUtf8().c_str());
			continue;
		}
		if (pFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			scriptSetDerrickPos(pFeature->pos.x, pFeature->pos.y);
		}
		//restore values
		pFeature->rot = ini.vector3i("rotation");
		pFeature->player = ini.value("player", PLAYER_FEATURE).toInt();

		// common BASE_OBJECT info
		loadSaveObject(ini, pFeature);

		pFeature->body = healthValue(ini, pFeature->psStats->body);

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
	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadAndWrite);
	int counter = 0;

	for (FEATURE *psCurr = apsFeatureLists[0]; psCurr != nullptr; psCurr = psCurr->psNext)
	{
		ini.beginGroup("feature_" + (WzString::number(counter++).leftPadToMinimumLength(WzUniCodepoint::fromASCII('0'), 10)));  // Zero padded so that alphabetical sort works.
		ini.setValue("name", psCurr->psStats->id);
		writeSaveObject(ini, psCurr);
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
bool loadSaveTemplate(const char *pFileName)
{
	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();

	auto loadTemplate = [&]() {
		DROID_TEMPLATE t;
		if (!loadTemplateCommon(ini, t))
		{
			debug(LOG_ERROR, "Stored template \"%s\" contains an unknown component.", ini.string("name").toUtf8().c_str());
		}
		t.name = ini.string("name");
		t.multiPlayerID = ini.value("multiPlayerID", generateNewObjectId()).toInt();
		t.enabled = ini.value("enabled", false).toBool();
		t.stored = ini.value("stored", false).toBool();
		t.prefab = ini.value("prefab", false).toBool();
		ini.nextArrayItem();
		return t;
	};

	int version = ini.value("version", 0).toInt();
	if (version == 0)
	{
		return false;
	}
	for (size_t i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		int player = getPlayer(ini);
		ini.beginArray("templates");
		while (ini.remainingArrayItems() > 0)
		{
			addTemplate(player, std::unique_ptr<DROID_TEMPLATE>(new DROID_TEMPLATE(loadTemplate())));
		}
		ini.endArray();
		ini.endGroup();
	}

	if (ini.contains("localTemplates"))
	{
		ini.beginArray("localTemplates");
		while (ini.remainingArrayItems() > 0)
		{
			localTemplates.emplace_back(loadTemplate());
		}
		ini.endArray();
	}
	else
	{
		// Old savegame compatibility, should remove this branch sometime.
		enumerateTemplates(selectedPlayer, [](DROID_TEMPLATE * psTempl) {
			localTemplates.push_back(*psTempl);
			return true;
		});
	}

	return true;
}

static nlohmann::json convGameTemplateToJSON(DROID_TEMPLATE *psCurr)
{
	nlohmann::json templateObj = saveTemplateCommon(psCurr);
	templateObj["ref"] = psCurr->ref;
	templateObj["multiPlayerID"] = psCurr->multiPlayerID;
	templateObj["enabled"] = psCurr->enabled;
	templateObj["stored"] = psCurr->stored;
	templateObj["prefab"] = psCurr->prefab;
	return templateObj;
}

bool writeTemplateFile(const char *pFileName)
{
	nlohmann::json mRoot = nlohmann::json::object();

	mRoot["version"] = 1;
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		if (!apsDroidLists[player] && !apsStructLists[player])	// only write out templates of players that are still 'alive'
		{
			continue;
		}
		nlohmann::json playerTemplatesObj = nlohmann::json::object();
		setPlayerJSON(playerTemplatesObj, player);
		nlohmann::json templates_array = nlohmann::json::array();
		enumerateTemplates(player, [&templates_array](DROID_TEMPLATE* psTemplate) {
			templates_array.push_back(convGameTemplateToJSON(psTemplate));
			return true;
		});
		playerTemplatesObj["templates"] = std::move(templates_array);
		auto playerKey = "player_" + WzString::number(player);
		mRoot[playerKey.toUtf8()] = std::move(playerTemplatesObj);
	}
	nlohmann::json localtemplates_array = nlohmann::json::array();
	for (auto &psCurr : localTemplates)
	{
		localtemplates_array.push_back(convGameTemplateToJSON(&psCurr));
	}
	mRoot["localTemplates"] = std::move(localtemplates_array);

	return saveJSONToFile(mRoot, pFileName);
}

// -----------------------------------------------------------------------------------------
// load up a terrain tile type map file
bool loadTerrainTypeMap(const char *pFilePath)
{
	ASSERT_OR_RETURN(false, pFilePath, "Null pFilePath");
	WzMapDebugLogger logger;
	WzMapPhysFSIO mapIO;
	auto result = WzMap::loadTerrainTypes(pFilePath, mapIO, &logger);
	if (!result)
	{
		// Failed to load terrain type map data
		return false;
	}

	// reset the terrain table
	memset(terrainTypes, 0, sizeof(terrainTypes));

	size_t quantity = result->terrainTypes.size();
	if (quantity >= MAX_TILE_TEXTURES)
	{
		// Workaround for fugly map editor bug, since we can't fix the map editor
		quantity = MAX_TILE_TEXTURES - 1;
	}
	for (size_t i = 0; i < quantity; i++)
	{
		auto& type = result->terrainTypes[i];
		if (type > TER_MAX)
		{
			debug(LOG_ERROR, "loadTerrainTypeMap: terrain type out of range");
			return false;
		}

		terrainTypes[i] = static_cast<UBYTE>(type);
	}

	return true;
}

bool loadTerrainTypeMapOverride(MAP_TILESET tileSet)
{
	resForceBaseDir("/data/base/");
	WzString iniName = "tileset/tileTypes.json";
	if (!PHYSFS_exists(iniName.toUtf8().c_str()))
	{
		return false;
	}

	WzConfig ini(iniName, WzConfig::ReadOnly);
	WzString tileTypeKey;

	switch (tileSet)
	{
		case MAP_TILESET::ARIZONA:
			tileTypeKey = "Arizona";
			break;
		case MAP_TILESET::URBAN:
			tileTypeKey = "Urban";
			break;
		case MAP_TILESET::ROCKIES:
			tileTypeKey = "Rockies";
			break;
//		default:
//			debug(LOG_ERROR, "Unknown tile type");
//			resForceBaseDir("");
//			return false;
	}

	std::vector<WzString> list = ini.childGroups();
	for (size_t i = 0; i < list.size(); ++i)
	{
		if (list[i].compare(tileTypeKey) == 0)
		{
			ini.beginGroup(list[i]);

			debug(LOG_TERRAIN, "Looking at tileset type: %s", tileTypeKey.toUtf8().c_str());
			unsigned int counter = 0;

			std::vector<WzString> keys = ini.childKeys();
			for (size_t j = 0; j < keys.size(); ++j)
			{
				unsigned int tileType = ini.value(keys.at(j)).toUInt();

				if (tileType > TER_MAX)
				{
					debug(LOG_ERROR, "loadTerrainTypeMapOverride: terrain type out of range");
					resForceBaseDir("");
					return false;
				}
				// Workaround for fugly map editor bug, since we can't fix the map editor
				if (counter > (MAX_TILE_TEXTURES - 1))
				{
					debug(LOG_ERROR, "loadTerrainTypeMapOverride: too many textures!");
					resForceBaseDir("");
					return false;
				}
				// Log the output for the override value.
				if (terrainTypes[counter] != tileType)
				{
					debug(LOG_TERRAIN, "Upgrading map tile %d (type %d) to type %d", counter, terrainTypes[counter], tileType);
				}
				terrainTypes[counter] = tileType;
				++counter;
				debug(LOG_TERRAIN, "Tile %d at value: %d", counter - 1, tileType);
			}
			ini.endGroup();
		}
	}

	resForceBaseDir("");

	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the terrain type map
static bool writeTerrainTypeMapFile(char *pFileName)
{
	ASSERT_OR_RETURN(false, pFileName != nullptr, "pFileName is null");

	WzMap::TerrainTypeData ttypeData;
	ttypeData.terrainTypes.reserve(MAX_TILE_TEXTURES);
	for (size_t i = 0; i < MAX_TILE_TEXTURES; i++)
	{
		UBYTE &tType = terrainTypes[i];
		if (tType > TER_MAX)
		{
			debug(LOG_ERROR, "Terrain type exceeds TER_MAX: %" PRIu8 "", tType);
		}
		ttypeData.terrainTypes.push_back(static_cast<TYPE_OF_TERRAIN>(tType));
	}

	/* Write out the map data */
	WzMapPhysFSIO mapIO;
	WzMapDebugLogger debugLoggerInstance;
	return WzMap::writeTerrainTypes(ttypeData, pFileName, mapIO, WzMap::LatestOutputFormat, &debugLoggerInstance);
}

bool isComponentStateValid(int state)
{
    return state == UNAVAILABLE 
        || state == AVAILABLE 
        || state == FOUND 
        || state == REDUNDANT
        || state == REDUNDANT_FOUND
        || state == REDUNDANT_UNAVAILABLE;
}

// -----------------------------------------------------------------------------------------
bool loadSaveCompList(const char *pFileName)
{
	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadOnly);

	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + WzString::number(player));
		std::vector<WzString> list = ini.childKeys();
		for (size_t i = 0; i < list.size(); ++i)
		{
			WzString name = list[i];
			int state = ini.value(name, UNAVAILABLE).toInt();
			COMPONENT_STATS *psComp = getCompStatsFromName(name);
			ASSERT_OR_RETURN(false, psComp, "Bad component %s", name.toUtf8().c_str());
			ASSERT_OR_RETURN(false, psComp->compType >= 0 && psComp->compType != COMP_NUMCOMPONENTS, "Bad type %d", psComp->compType);
			ASSERT_OR_RETURN(false, isComponentStateValid(state), "Bad state %d for %s", state, name.toUtf8().c_str());
			apCompLists[player][psComp->compType][psComp->index] = state;
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Comp lists per player
static bool writeCompListFile(const char *pFileName)
{
	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadAndWrite);

	// Save each type of struct type
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + WzString::number(player));
		for (int i = 0; i < numBodyStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asBodyStats + i);
			const int state = apCompLists[player][COMP_BODY][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		for (int i = 0; i < numWeaponStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asWeaponStats + i);
			const int state = apCompLists[player][COMP_WEAPON][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		for (int i = 0; i < numConstructStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asConstructStats + i);
			const int state = apCompLists[player][COMP_CONSTRUCT][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		for (int i = 0; i < numECMStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asECMStats + i);
			const int state = apCompLists[player][COMP_ECM][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		for (int i = 0; i < numPropulsionStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asPropulsionStats + i);
			const int state = apCompLists[player][COMP_PROPULSION][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		for (int i = 0; i < numSensorStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asSensorStats + i);
			const int state = apCompLists[player][COMP_SENSOR][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		for (int i = 0; i < numRepairStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asRepairStats + i);
			const int state = apCompLists[player][COMP_REPAIRUNIT][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		for (int i = 0; i < numBrainStats; i++)
		{
			COMPONENT_STATS *psStats = (COMPONENT_STATS *)(asBrainStats + i);
			const int state = apCompLists[player][COMP_BRAIN][i];
			if (state != UNAVAILABLE)
			{
				ini.setValue(psStats->id, state);
			}
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// load up structure type list file
static bool loadSaveStructTypeList(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnly);

	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + WzString::number(player));
		std::vector<WzString> list = ini.childKeys();
		for (size_t i = 0; i < list.size(); ++i)
		{
			WzString name = list[i];
			int state = ini.value(name, UNAVAILABLE).toInt();
			int statInc;

			ASSERT_OR_RETURN(false, isComponentStateValid(state), "Bad state %d for %s", state, name.toUtf8().c_str());
			for (statInc = 0; statInc < numStructureStats; statInc++) // loop until find the same name
			{
				STRUCTURE_STATS *psStats = asStructureStats + statInc;

				if (name.compare(psStats->id) == 0)
				{
					apStructTypeLists[player][statInc] = state;
					break;
				}
			}
			ASSERT_OR_RETURN(false, statInc != numStructureStats, "Did not find structure %s", name.toUtf8().c_str());
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current state of the Struct Type List per player
static bool writeStructTypeListFile(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadAndWrite);

	// Save each type of struct type
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		ini.beginGroup("player_" + WzString::number(player));
		STRUCTURE_STATS *psStats = asStructureStats;
		for (int i = 0; i < numStructureStats; i++, psStats++)
		{
			if (apStructTypeLists[player][i] != UNAVAILABLE)
			{
				ini.setValue(psStats->id, apStructTypeLists[player][i]);
			}
		}
		ini.endGroup();
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// load up saved research file
bool loadSaveResearch(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnly);
	const int players = game.maxPlayers;
	std::vector<WzString> list = ini.childGroups();
	for (size_t i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		bool found = false;
		WzString name = ini.value("name").toWzString();
		int statInc;
		for (statInc = 0; statInc < asResearch.size(); statInc++)
		{
			RESEARCH *psStats = &asResearch[statInc];
			//loop until find the same name
			if (psStats->id.compare(name) == 0)

			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			//ignore this record
			debug(LOG_SAVE, "Skipping unknown research named '%s'", name.toStdString().c_str());
			ini.endGroup();
			continue;
		}
		auto researchedList = ini.value("researched").jsonValue();
		auto possiblesList = ini.value("possible").jsonValue();
		auto pointsList = ini.value("currentPoints").jsonValue();
		ASSERT(researchedList.is_array(), "Bad (non-array) researched list for %s", name.toUtf8().c_str());
		ASSERT(possiblesList.is_array(), "Bad (non-array) possible list for %s", name.toUtf8().c_str());
		ASSERT(pointsList.is_array(), "Bad (non-array) points list for %s", name.toUtf8().c_str());
		ASSERT(researchedList.size() == players, "Bad researched list for %s", name.toUtf8().c_str());
		ASSERT(possiblesList.size() == players, "Bad possible list for %s", name.toUtf8().c_str());
		ASSERT(pointsList.size() == players, "Bad points list for %s", name.toUtf8().c_str());
		for (int plr = 0; plr < players; plr++)
		{
			PLAYER_RESEARCH *psPlRes;
			int researched = json_getValue(researchedList, plr).toInt();
			int possible = json_getValue(possiblesList, plr).toInt();
			int points = json_getValue(pointsList, plr).toInt();

			psPlRes = &asPlayerResList[plr][statInc];
			// Copy the research status
			psPlRes->ResearchStatus = (researched & RESBITS_ALL);
			SetResearchPossible(psPlRes, possible);
			psPlRes->currentPoints = points;
			//for any research that has been completed - perform so that upgrade values are set up
			if (researched == RESEARCHED)
			{
				researchResult(statInc, plr, false, nullptr, false);
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
	WzConfig ini(WzString::fromUtf8(pFileName), WzConfig::ReadAndWrite);

	for (size_t i = 0; i < asResearch.size(); ++i)
	{
		RESEARCH *psStats = &asResearch[i];
		bool valid = false;
		std::vector<WzString> possibles, researched, points;
		for (int player = 0; player < game.maxPlayers; player++)
		{
			possibles.push_back(WzString::number(GetResearchPossible(&asPlayerResList[player][i])));
			researched.push_back(WzString::number(asPlayerResList[player][i].ResearchStatus & RESBITS_ALL));
			points.push_back(WzString::number(asPlayerResList[player][i].currentPoints));
			if (IsResearchPossible(&asPlayerResList[player][i]) || (asPlayerResList[player][i].ResearchStatus & RESBITS_ALL) || asPlayerResList[player][i].currentPoints)
			{
				valid = true;	// write this entry
			}
		}
		if (valid)
		{
			ini.beginGroup("research_" + WzString::number(i));
			ini.setValue("name", psStats->id);
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
bool loadSaveMessage(const char* pFileName, LEVEL_TYPE levelType)
{
	// Only clear the messages if its a mid save game
	if (gameType == GTYPE_SAVE_MIDMISSION)
	{
		freeMessages();
	}
	else if (gameType == GTYPE_SAVE_START)
	{
		// If we are loading in a CamStart or a CamChange then we are not interested in any saved messages
		if (levelType == LEVEL_TYPE::LDS_CAMSTART || levelType == LEVEL_TYPE::LDS_CAMCHANGE)
		{
			return true;
		}
	}

	WzConfig ini(pFileName, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();
	for (size_t i = 0; i < list.size(); ++i)
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
						psMessage->psObj = getBaseObjFromData(objId, objPlayer, objType);
						ASSERT(psMessage->psObj, "Viewdata object id %d not found for message %d", objId, id);
					}
					else
					{
						debug(LOG_ERROR, "Proximity object could not be created (type=%d, player=%d, message=%d)", type, player, id);
					}
				}
				else
				{
					VIEWDATA* psViewData = nullptr;

					// Proximity position so get viewdata pointer from the name
					MESSAGE* psMessage = addMessage(type, false, player);

					if (psMessage)
					{
						if (dataType == MSG_DATA_BEACON)
						{
							//See addBeaconMessage(). psMessage->dataType is wrong here because
							//addMessage() calls createMessage() which defaults dataType to MSG_DATA_DEFAULT.
							//Later when findBeaconMsg() attempts to find a placed beacon it can't because
							//the dataType is wrong.
							psMessage->dataType = MSG_DATA_BEACON;
							Vector2i pos = ini.vector2i("position");
							int sender = ini.value("sender").toInt();
							psViewData = CreateBeaconViewData(sender, pos.x, pos.y);
							ASSERT(psViewData, "Could not create view data for message %d", id);
							if (psViewData == nullptr)
							{
								// Skip this message
								removeMessage(psMessage, player);
								continue;
							}
						}
						else if (ini.contains("name"))
						{
							psViewData = getViewData(ini.value("name").toWzString());
							ASSERT(psViewData, "Failed to find view data for proximity position - skipping message %d", id);
							if (psViewData == nullptr)
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

						psMessage->pViewData = psViewData;
						// Check the z value is at least the height of the terrain
						const int terrainHeight = map_Height(((VIEW_PROXIMITY*)psViewData->pData)->x, ((VIEW_PROXIMITY*)psViewData->pData)->y);
						if (((VIEW_PROXIMITY*)psViewData->pData)->z < terrainHeight)
						{
							((VIEW_PROXIMITY*)psViewData->pData)->z = terrainHeight;
						}
					}
					else
					{
						debug(LOG_ERROR, "Proximity position could not be created (type=%d, player=%d, message=%d)", type, player, id);
					}
				}
			}
		}
		else
		{
			// Only load Campaign/Mission messages if a mid-mission save game; always load research messages
			if (type == MSG_RESEARCH || gameType == GTYPE_SAVE_MIDMISSION)
			{
				MESSAGE* psMessage = addMessage(type, false, player);
				ASSERT(psMessage, "Could not create message %d", id);
				if (psMessage)
				{
					psMessage->pViewData = getViewData(ini.value("name").toWzString());
					ASSERT(psMessage->pViewData, "Failed to find view data for message %d", id);
				}
			}
		}
		ini.endGroup();
	}
	jsDebugMessageUpdate();
	return true;
}

// -----------------------------------------------------------------------------------------
// Write out the current messages per player
static bool writeMessageFile(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadAndWrite);
	int numMessages = 0;

	// save each type of research
	for (int player = 0; player < game.maxPlayers; player++)
	{
		ASSERT(player < MAX_PLAYERS, "player out of bounds: %d", player);
		for (MESSAGE *psMessage = apsMessages[player]; psMessage != nullptr; psMessage = psMessage->psNext)
		{
			ini.beginGroup("message_" + WzString::number(numMessages++));
			ini.setValue("id", numMessages - 1);	// for future use
			ini.setValue("player", player);
			ini.setValue("type", psMessage->type);
			ini.setValue("dataType", psMessage->dataType);
			if (psMessage->type == MSG_PROXIMITY)
			{
				//get the matching proximity message
				PROXIMITY_DISPLAY *psProx = nullptr;
				for (psProx = apsProxDisp[player]; psProx != nullptr; psProx = psProx->psNext)
				{
					//compare the pointers
					if (psProx->psMessage == psMessage)
					{
						break;
					}
				}
				ASSERT(psProx != nullptr, "Save message; proximity display not found for message");
				if (psProx && psProx->type == POS_PROXDATA)
				{
					//message has viewdata so store the name
					VIEWDATA *pViewData = psMessage->pViewData;
					ini.setValue("name", pViewData->name);

					// save beacon data
					if (psMessage->dataType == MSG_DATA_BEACON)
					{
						VIEW_PROXIMITY *viewData = (VIEW_PROXIMITY *)psMessage->pViewData->pData;
						ini.setVector2i("position", Vector2i(viewData->x, viewData->y));
						ini.setValue("sender", viewData->sender);
					}
				}
				else
				{
					// message has object so store Object Id
					const BASE_OBJECT *psObj = psMessage->psObj;
					if (psObj)
					{
						ini.setValue("obj/id", psObj->id);
						ini.setValue("obj/player", psObj->player);
						ini.setValue("obj/type", psObj->type);
					}
					else
					{
						ASSERT(false, "Message type has no object data to save ?");
					}
				}
			}
			else
			{
				const VIEWDATA *pViewData = psMessage->pViewData;
				ini.setValue("name", pViewData != nullptr ? pViewData->name : "NULL");
			}
			ini.setValue("read", psMessage->read);	// flag to indicate whether message has been read; not that this is/was _not_ read by loading code!?
			ASSERT(player == psMessage->player, "Bad player number (%d == %d)", player, psMessage->player);
			ini.endGroup();
		}
	}
	return true;
}

// -----------------------------------------------------------------------------------------
bool loadSaveLimits(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnly);

	for (int player = 0; player < game.maxPlayers; player++)
	{
		ini.beginGroup("player_" + WzString::number(player));
		std::vector<WzString> list = ini.childKeys();
		for (size_t i = 0; i < list.size(); ++i)
		{
			WzString name = list[i];
			int limit = ini.value(name, 0).toInt();

			if (name.compare("@Droid") == 0)
			{
				setMaxDroids(player, limit);
			}
			else if (name.compare("@Commander") == 0)
			{
				setMaxCommanders(player, limit);
			}
			else if (name.compare("@Constructor") == 0)
			{
				setMaxConstructors(player, limit);
			}
			else
			{
				unsigned statInc;
				for (statInc = 0; statInc < numStructureStats; ++statInc)
				{
					STRUCTURE_STATS *psStats = asStructureStats + statInc;
					if (name.compare(psStats->id) == 0)
					{
						asStructureStats[statInc].upgrade[player].limit = limit != 255 ? limit : LOTS_OF;
						break;
					}
				}
				ASSERT_OR_RETURN(false, statInc != numStructureStats, "Did not find structure %s", name.toUtf8().c_str());
			}
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
	WzConfig ini(pFileName, WzConfig::ReadAndWrite);

	// Save each type of struct type
	for (int player = 0; player < game.maxPlayers; player++)
	{
		ini.beginGroup("player_" + WzString::number(player));

		ini.setValue("@Droid", getMaxDroids(player));
		ini.setValue("@Commander", getMaxCommanders(player));
		ini.setValue("@Constructor", getMaxConstructors(player));

		STRUCTURE_STATS *psStats = asStructureStats;
		for (int i = 0; i < numStructureStats; i++, psStats++)
		{
			const int limit = MIN(asStructureStats[i].upgrade[player].limit, 255);
			if (limit != 255)
			{
				ini.setValue(psStats->id, limit);
			}
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
	WzConfig ini(pFileName, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();

	for (size_t i = 0; i < list.size(); ++i)
	{
		uint32_t id = ini.value("Player_" + WzString::number(i) + "/id", NULL_ID).toInt();
		if (id != NULL_ID)
		{
			cmdDroidSetDesignator((DROID *)getBaseObjFromId(id));
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
	WzConfig ini(pFileName, WzConfig::ReadAndWrite);

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		DROID *psDroid = cmdDroidGetDesignator(player);
		if (psDroid != nullptr)
		{
			ini.setValue("Player_" + WzString::number(player) + "/id", psDroid->id);
		}
	}
	return true;
}


// -----------------------------------------------------------------------------------------
// write the event state to a file on disk
static bool	writeScriptState(const char *pFileName)
{
	char	jsFilename[PATH_MAX], *ext;

	// The below belongs to the new javascript stuff
	sstrcpy(jsFilename, pFileName);
	ext = strrchr(jsFilename, '/');
	*ext = '\0';
	strcat(jsFilename, "/scriptstate.json");
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
	sstrcpy(jsFilename, pFileName);
	strcat(jsFilename, "/scriptstate.json");
	loadScriptStates(jsFilename);

	// change the file extension
	strcat(pFileName, "/scriptstate.es");

	return true;
}


// -----------------------------------------------------------------------------------------
/* set the global scroll values to use for the save game */
static void setMapScroll()
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
		debug(LOG_NEVER, "scrollMaxX was too big It has been set to map width");
	}
	if (scrollMaxY > (SDWORD)mapHeight)
	{
		scrollMaxY = mapHeight;
		debug(LOG_NEVER, "scrollMaxY was too big It has been set to map height");
	}
}


// -----------------------------------------------------------------------------------------
/*returns the current type of save game being loaded*/
GAME_TYPE getSaveGameType()
{
	return gameType;
}

const char *savegameWithoutExtension(const char *name)
{
	static char displaySavegameName[256] = {'\0'};
	sstrcpy(displaySavegameName, name);
	displaySavegameName[std::max<size_t>(strlen(displaySavegameName), 4) - 4] = '\0'; //axe the .gam

	return displaySavegameName;
}
