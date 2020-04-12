/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2019  Warzone 2100 Project

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

#ifndef __INCLUDED_WZAPI_H__
#define __INCLUDED_WZAPI_H__

#include "lib/framework/frame.h"
#include <optional-lite/optional.hpp>

using nonstd::optional;
using nonstd::nullopt;

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace wzapi
{
#define WZAPI_NO_PARAMS const wzapi::execution_context& context
#define WZAPI_PARAMS(...) const wzapi::execution_context& context, __VA_ARGS__

	class execution_context
	{
	public:
		virtual ~execution_context() { };
	public:
		virtual int player() const = 0;
		virtual void throwError(const char *expr, int line, const char *function) const = 0;
		virtual playerCallbackFunc getNamedScriptCallback(const WzString& func) const = 0;
		virtual void hack_setMe(int player) const = 0;
		virtual void set_isReceivingAllEvents(bool value) const = 0;
		virtual bool get_isReceivingAllEvents() const = 0;
		virtual void doNotSaveGlobal(const std::string &global) const = 0;
	};

	struct droid_id_player
	{
		int id = -1;
		int player = -1;
	};

//	struct string_list
//	{
//		const char** strings = nullptr;
//		size_t count = 0;
//	};

	struct reservedParam
	{ };

	struct string_or_string_list
	{
		std::vector<std::string> strings;
	};

	struct va_list_treat_as_strings
	{
		std::vector<std::string> strings;
	};

//	struct va_list_of_string_or_string_list
//	{
//		std::vector<string_or_string_list> va_list;
//	};

	template<typename ContainedType>
	struct va_list
	{
		std::vector<ContainedType> va_list;
	};

	struct optional_position
	{
		bool valid;
		int x;
		int y;
	};

//	struct me
//	{
//		int player = -1;
//	};

	struct specified_player
	{
		int player = -1;
	};

	struct STRUCTURE_TYPE_or_statsName_string
	{
		STRUCTURE_TYPE type = NUM_DIFF_BUILDINGS;
		std::string statsName;
	};

	// retVals
	struct no_return_value
	{ };
	struct researchResult
	{
		const RESEARCH * psResearch;
		int player;
	};
	struct researchResults
	{
		std::vector<const RESEARCH *> resList;
		int player;
	};
	struct position_in_map_coords
	{
		int32_t x;
		int32_t y;
	};

	#define MULTIPLAY_SYNCREQUEST_REQUIRED
	#define MUTLIPLAY_UNSAFE
	#define WZAPI_DEPRECATED
	#define WZAPI_AI_UNSAFE

	std::string translate(WZAPI_PARAMS(std::string str));
	int32_t syncRandom(WZAPI_PARAMS(uint32_t limit));
	bool setAlliance(WZAPI_PARAMS(int player1, int player2, bool value));
	no_return_value sendAllianceRequest(WZAPI_PARAMS(int player2));
	bool orderDroid(WZAPI_PARAMS(DROID* psDroid, int order));
	bool orderDroidBuild(WZAPI_PARAMS(DROID* psDroid, int order, std::string statName, int x, int y, optional<float> direction));
	bool setAssemblyPoint(WZAPI_PARAMS(STRUCTURE *psStruct, int x, int y));
	bool setSunPosition(WZAPI_PARAMS(float x, float y, float z));
	bool setSunIntensity(WZAPI_PARAMS(float ambient_r, float ambient_g, float ambient_b, float diffuse_r, float diffuse_g, float diffuse_b, float specular_r, float specular_g, float specular_b));
	bool setWeather(WZAPI_PARAMS(int weather));
	bool setSky(WZAPI_PARAMS(std::string page, float wind, float scale));
	bool cameraSlide(WZAPI_PARAMS(float x, float y));
	bool cameraZoom(WZAPI_PARAMS(float z, float speed));
	bool cameraTrack(WZAPI_PARAMS(optional<DROID *> targetDroid));
	uint32_t addSpotter(WZAPI_PARAMS(int x, int y, int player, int range, bool radar, uint32_t expiry));
	bool removeSpotter(WZAPI_PARAMS(uint32_t id));
	bool syncRequest(WZAPI_PARAMS(int32_t req_id, int32_t x, int32_t y, optional<const BASE_OBJECT *> _psObj, optional<const BASE_OBJECT *> _psObj2));
	bool replaceTexture(WZAPI_PARAMS(std::string oldfile, std::string newfile));
	bool changePlayerColour(WZAPI_PARAMS(int player, int colour));
	bool setHealth(WZAPI_PARAMS(BASE_OBJECT* psObject, int health)); MULTIPLAY_SYNCREQUEST_REQUIRED
	bool useSafetyTransport(WZAPI_PARAMS(bool flag));
	bool restoreLimboMissionData(WZAPI_NO_PARAMS);
	uint32_t getMultiTechLevel(WZAPI_NO_PARAMS);
	bool setCampaignNumber(WZAPI_PARAMS(int num));

	bool setRevealStatus(WZAPI_PARAMS(bool status));
	bool autoSave(WZAPI_NO_PARAMS);

	// horrible hacks follow -- do not rely on these being present!
	no_return_value hackNetOff(WZAPI_NO_PARAMS);
	no_return_value hackNetOn(WZAPI_NO_PARAMS);
	no_return_value hackAddMessage(WZAPI_PARAMS(std::string message, int type, int player, bool immediate));
	no_return_value hackRemoveMessage(WZAPI_PARAMS(std::string message, int type, int player));
	const BASE_OBJECT * hackGetObj(WZAPI_PARAMS(int _type, int player, int id)) WZAPI_DEPRECATED;
	no_return_value hackChangeMe(WZAPI_PARAMS(int player));
	no_return_value hackAssert(WZAPI_PARAMS(bool condition, va_list_treat_as_strings message));
	bool receiveAllEvents(WZAPI_PARAMS(optional<bool> enabled));
	no_return_value hackDoNotSave(WZAPI_PARAMS(std::string name));
	no_return_value hackPlayIngameAudio(WZAPI_NO_PARAMS);
	no_return_value hackStopIngameAudio(WZAPI_NO_PARAMS);

	// General functions -- geared for use in AI scripts
	bool console(WZAPI_PARAMS(va_list_treat_as_strings strings));
	bool clearConsole(WZAPI_NO_PARAMS);
	bool structureIdle(WZAPI_PARAMS(const STRUCTURE *psStruct));
	std::vector<const STRUCTURE *> enumStruct(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking));
	std::vector<const STRUCTURE *> enumStructOffWorld(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking));
	std::vector<const DROID *> enumDroid(WZAPI_PARAMS(optional<int> _player, optional<int> _droidType, optional<int> _looking));
	std::vector<const FEATURE *> enumFeature(WZAPI_PARAMS(int looking, optional<std::string> _statsName));
	std::vector<Position> enumBlips(WZAPI_PARAMS(int player));
	std::vector<const BASE_OBJECT *> enumSelected(WZAPI_NO_PARAMS);
	researchResult getResearch(WZAPI_PARAMS(std::string resName, optional<int> _player));
	researchResults enumResearch(WZAPI_NO_PARAMS);
	std::vector<const BASE_OBJECT *> enumRange(WZAPI_PARAMS(int x, int y, int range, optional<int> _filter, optional<bool> _seen));
	bool pursueResearch(WZAPI_PARAMS(const STRUCTURE *psStruct, string_or_string_list research));
	researchResults findResearch(WZAPI_PARAMS(std::string resName, optional<int> _player));
	int32_t distBetweenTwoPoints(WZAPI_PARAMS(int32_t x1, int32_t y1, int32_t x2, int32_t y2));
	bool orderDroidLoc(WZAPI_PARAMS(DROID *psDroid, int order_, int x, int y));
	int32_t playerPower(WZAPI_PARAMS(int player));
	int queuedPower(WZAPI_PARAMS(int player));
	bool isStructureAvailable(WZAPI_PARAMS(std::string structName, optional<int> _player));
	optional<position_in_map_coords> pickStructLocation(WZAPI_PARAMS(DROID *psDroid, std::string statName, int startX, int startY, optional<int> _maxBlockingTiles));
	bool droidCanReach(WZAPI_PARAMS(const DROID *psDroid, int x, int y));
	bool propulsionCanReach(WZAPI_PARAMS(std::string propulsionName, int x1, int y1, int x2, int y2));
	int terrainType(WZAPI_PARAMS(int x, int y));
	bool orderDroidObj(WZAPI_PARAMS(DROID *psDroid, int _order, BASE_OBJECT *psObj));
	bool buildDroid(WZAPI_PARAMS(STRUCTURE *psFactory, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets));
	const DROID* addDroid(WZAPI_PARAMS(int player, int x, int y, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets)); MUTLIPLAY_UNSAFE
	std::unique_ptr<const DROID_TEMPLATE> makeTemplate(WZAPI_PARAMS(int player, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, va_list<string_or_string_list> turrets));
	bool addDroidToTransporter(WZAPI_PARAMS(droid_id_player transporter, droid_id_player droid));
	const FEATURE * addFeature(WZAPI_PARAMS(std::string featName, int x, int y)) MUTLIPLAY_UNSAFE;
	bool componentAvailable(WZAPI_PARAMS(std::string arg1, optional<std::string> arg2));
	bool isVTOL(WZAPI_PARAMS(const DROID *psDroid));
	bool safeDest(WZAPI_PARAMS(int player, int x, int y));
	bool activateStructure(WZAPI_PARAMS(STRUCTURE *psStruct, optional<BASE_OBJECT *> _psTarget));
	bool chat(WZAPI_PARAMS(int target, std::string message));
	bool addBeacon(WZAPI_PARAMS(int x, int y, int target, optional<std::string> _message));
	bool removeBeacon(WZAPI_PARAMS(int target));
	std::unique_ptr<const DROID> getDroidProduction(WZAPI_PARAMS(STRUCTURE *_psFactory));
	int getDroidLimit(WZAPI_PARAMS(optional<int> _player, optional<int> _unitType));
	int getExperienceModifier(WZAPI_PARAMS(int player));
	bool setDroidLimit(WZAPI_PARAMS(int player, int value, optional<int> _droidType));
	bool setExperienceModifier(WZAPI_PARAMS(int player, int percent));
	std::vector<const DROID *> enumCargo(WZAPI_PARAMS(const DROID *psDroid));

	// MARK: - Functions that operate on the current player only
	bool centreView(WZAPI_PARAMS(int x, int y));
	bool playSound(WZAPI_PARAMS(std::string sound, optional<int> _x, optional<int> _y, optional<int> _z));
	bool gameOverMessage(WZAPI_PARAMS(bool gameWon, optional<bool> _showBackDrop, optional<bool> _showOutro));

	// MARK: - Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)
	bool setStructureLimits(WZAPI_PARAMS(std::string building, int limit, optional<int> _player));
	bool applyLimitSet(WZAPI_NO_PARAMS);
	no_return_value setMissionTime(WZAPI_PARAMS(int _value));
	int getMissionTime(WZAPI_NO_PARAMS);
	no_return_value setReinforcementTime(WZAPI_PARAMS(int _value));
	no_return_value completeResearch(WZAPI_PARAMS(std::string researchName, optional<int> _player, optional<bool> _forceResearch));
	no_return_value completeAllResearch(WZAPI_PARAMS(optional<int> _player));
	bool enableResearch(WZAPI_PARAMS(std::string researchName, optional<int> _player));
	no_return_value setPower(WZAPI_PARAMS(int power, optional<int> _player)); WZAPI_AI_UNSAFE
	no_return_value setPowerModifier(WZAPI_PARAMS(int power, optional<int> _player)); WZAPI_AI_UNSAFE
	no_return_value setPowerStorageMaximum(WZAPI_PARAMS(int power, optional<int> _player)); WZAPI_AI_UNSAFE
	no_return_value extraPowerTime(WZAPI_PARAMS(int time, optional<int> _player));
	no_return_value setTutorialMode(WZAPI_PARAMS(bool tutorialMode));
	no_return_value setDesign(WZAPI_PARAMS(bool allowDesign));
	bool enableTemplate(WZAPI_PARAMS(std::string _templateName));
	bool removeTemplate(WZAPI_PARAMS(std::string _templateName));
	no_return_value setMiniMap(WZAPI_PARAMS(bool visible));
	no_return_value setReticuleButton(WZAPI_PARAMS(int buttonID, std::string tooltip, std::string filename, std::string filenameDown, optional<std::string> callbackFuncName));
	no_return_value setReticuleFlash(WZAPI_PARAMS(int button, bool flash));
	no_return_value showReticuleWidget(WZAPI_PARAMS(int button));
	no_return_value showInterface(WZAPI_NO_PARAMS);
	no_return_value hideInterface(WZAPI_NO_PARAMS);
	no_return_value enableStructure(WZAPI_PARAMS(std::string structureName, optional<int> _player));
	no_return_value enableComponent(WZAPI_PARAMS(std::string componentName, int player));
	no_return_value makeComponentAvailable(WZAPI_PARAMS(std::string componentName, int player));
	bool allianceExistsBetween(WZAPI_PARAMS(int player1, int player2));
	bool removeStruct(WZAPI_PARAMS(STRUCTURE *psStruct)); WZAPI_DEPRECATED
	bool removeObject(WZAPI_PARAMS(BASE_OBJECT *psObj, optional<bool> _sfx));
	no_return_value setScrollLimits(WZAPI_PARAMS(int x1, int y1, int x2, int y2));
	const STRUCTURE * addStructure(WZAPI_PARAMS(std::string structureName, int player, int x, int y));
	unsigned int getStructureLimit(WZAPI_PARAMS(std::string structureName, optional<int> _player));
	int countStruct(WZAPI_PARAMS(std::string structureName, optional<int> _player));
	int countDroid(WZAPI_PARAMS(optional<int> _type, optional<int> _player));
	no_return_value loadLevel(WZAPI_PARAMS(std::string levelName));
	no_return_value setDroidExperience(WZAPI_PARAMS(DROID *psDroid, double experience));
	bool donateObject(WZAPI_PARAMS(BASE_OBJECT *psObject, int toPlayer));
	bool donatePower(WZAPI_PARAMS(int amount, int toPlayer));
	no_return_value setNoGoArea(WZAPI_PARAMS(int x1, int y1, int x2, int y2, int player));
	no_return_value startTransporterEntry(WZAPI_PARAMS(int x, int y, int player));
	no_return_value setTransporterExit(WZAPI_PARAMS(int x, int y, int player));
	no_return_value setObjectFlag(WZAPI_PARAMS(BASE_OBJECT *psObj, int _flag, bool value)) MULTIPLAY_SYNCREQUEST_REQUIRED;
	no_return_value fireWeaponAtLoc(WZAPI_PARAMS(std::string weaponName, int x, int y, optional<int> _player));
	no_return_value fireWeaponAtObj(WZAPI_PARAMS(std::string weaponName, BASE_OBJECT *psObj, optional<int> _player));
	bool setUpgradeStats(WZAPI_PARAMS(int player, const std::string& name, int type, unsigned index, const nlohmann::json& newValue));
	nlohmann::json getUpgradeStats(WZAPI_PARAMS(int player, const std::string& name, int type, unsigned index));
}

enum Scrcb {
	SCRCB_FIRST = COMP_NUMCOMPONENTS,
	SCRCB_RES = SCRCB_FIRST,  // Research upgrade
	SCRCB_MODULE_RES,  // Research module upgrade
	SCRCB_REP,  // Repair upgrade
	SCRCB_POW,  // Power upgrade
	SCRCB_MODULE_POW,  // And so on...
	SCRCB_CON,
	SCRCB_MODULE_CON,
	SCRCB_REA,
	SCRCB_ARM,
	SCRCB_HEA,
	SCRCB_ELW,
	SCRCB_HIT,
	SCRCB_LIMIT,
	SCRCB_LAST = SCRCB_LIMIT
};

#endif
