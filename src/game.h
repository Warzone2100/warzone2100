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
#include <nlohmann/json_fwd.hpp>
#include <nonstd/optional.hpp>
#include <sstream>

namespace WzMap {
	class Map;
	class MapPackage;
	class LoggingProtocol;
	struct GamInfo;
}

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/

struct GameLoadDetails
{
public:
	enum class GameLoadType
	{
		UserSaveGame,
		Level,
		MapPackage
	};
protected:
	GameLoadDetails(GameLoadType loadType, const std::string& filePath);
public:
	static GameLoadDetails makeUserSaveGameLoad(const std::string& saveGame);
	static GameLoadDetails makeMapPackageLoad(const std::string& mapPackageFilePath);
	static GameLoadDetails makeLevelFileLoad(const std::string& levelFileName);
	GameLoadDetails& setLogger(const std::shared_ptr<WzMap::LoggingProtocol>& logger);
public:
	std::string getMapFolderPath() const;
	std::shared_ptr<WzMap::Map> getMap(uint32_t mapSeed) const;
	const WzMap::GamInfo* getGamInfoFromPackage() const;
private:
	std::shared_ptr<WzMap::MapPackage> getMapPackage() const;
public:
	GameLoadType loadType;
	std::string filePath;
private:
	std::shared_ptr<WzMap::LoggingProtocol> m_logger;
	mutable std::shared_ptr<WzMap::MapPackage> m_loadedPackage;
};

bool loadGame(const GameLoadDetails& gameToLoad, bool keepObjects, bool freeMem);

/*This just loads up the .gam file to determine which level data to set up - split up
so can be called in levLoadData when starting a game from a load save game*/
bool loadGameInit(const GameLoadDetails& gameToLoad);

bool loadMissionExtras(const char* pGameToLoad, LEVEL_TYPE levelType);

// load the script state given a .gam name
bool loadScriptState(char *pFileName);

bool saveGame(const char *aFileName, GAME_TYPE saveType, bool isAutoSave = false);

// Get the campaign number for loadGameInit game
UDWORD getCampaign(const char *fileName);

/*returns the current type of save game being loaded*/
GAME_TYPE getSaveGameType();

// Removes .gam from a save for display purposes
const char *savegameWithoutExtension(const char *name);

void gameScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight);
void gameDisplayScaleFactorDidChange(float newDisplayScaleFactor);
nonstd::optional<nlohmann::json> parseJsonFile(const char *filename);
bool saveJSONToFile(const nlohmann::json& obj, const char* pFileName);

#if defined(__EMSCRIPTEN__)
void wz_emscripten_did_finish_render(unsigned int browserRenderDelta);
#endif

#endif // __INCLUDED_SRC_GAME_H__
