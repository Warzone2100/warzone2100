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

// Documentation stuff follows. The build system will parse the comment prefixes
// and sort the comments into the appropriate Markdown documentation files.

//== # Globals
//==
//== This section describes global variables (or 'globals' for short) that are
//== available from all scripts. You typically cannot write to these variables,
//== they are read-only.
//==
//__ # Events
//__
//__ This section describes event callbacks (or 'events' for short) that are
//__ called from the game when something specific happens. Which scripts
//__ receive them is usually filtered by player. Call ```receiveAllEvents(true)```
//__ to start receiving all events unfiltered.
//__
//-- # Functions
//--
//-- This section describes functions that can be called from scripts to make
//-- things happen in the game (usually called our script 'API').
//--
//;; # Game objects
//;;
//;; This section describes various **game objects** defined by the script interface,
//;; and which are both accepted by functions and returned by them. Changing the
//;; fields of a **game object** has no effect on the game before it is passed to a
//;; function that does something with the **game object**.
//;;

#include "lib/framework/frame.h"
#include <optional-lite/optional.hpp>
#include "basedef.h"
#include "structuredef.h"
#include "featuredef.h"
#include "lib/framework/wzconfig.h"
#include "hci.h"
#include "gateway.h"

using nonstd::optional;
using nonstd::nullopt;

#include <string>
#include <vector>
#include <memory>
#include <functional>

typedef uint64_t uniqueTimerID;
class timerAdditionalData
{
public:
	virtual ~timerAdditionalData() { }
public:
	virtual void onTimerDelete(uniqueTimerID, BASE_OBJECT*) { };
};

typedef std::function<void (uniqueTimerID, BASE_OBJECT*, timerAdditionalData *)> TimerFunc;

// NOTES:
// - All position value types (scr_position, scr_area, etc) passed to/from scripts expect map coordinates

enum timerType
{
	TIMER_REPEAT, TIMER_ONESHOT_READY, TIMER_ONESHOT_DONE
};

struct scr_radius
{
	int32_t x; int32_t y; int32_t radius;
};

struct scr_area
{
	int32_t x1; int32_t y1; int32_t x2; int32_t y2;
};

struct scr_position
{
	int32_t x; int32_t y;
};

class QScriptEngine;

namespace wzapi
{
#define WZAPI_NO_PARAMS_NO_CONTEXT
#define WZAPI_NO_PARAMS const wzapi::execution_context& context
#define WZAPI_PARAMS(...) const wzapi::execution_context& context, __VA_ARGS__

#define SCRIPTING_EVENT_NON_REQUIRED { return false; }

	struct researchResult; // forward-declare

	class scripting_event_handling_interface
	{
	public:
		virtual ~scripting_event_handling_interface() { };
	public:
		// MARK: General events

		//__ ## eventGameInit()
		//__
		//__ An event that is run once as the game is initialized. Not all game state may have been
		//__ properly initialized by this time, so use this only to initialize script state.
		//__
		virtual bool handle_eventGameInit() = 0;

		//__ ## eventStartLevel()
		//__
		//__ An event that is run once the game has started and all game data has been loaded.
		//__
		virtual bool handle_eventStartLevel() = 0;

		//__ ## eventMissionTimeout()
		//__
		//__ An event that is run when the mission timer has run out.
		//__
		virtual bool handle_eventMissionTimeout() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventVideoDone()
		//__
		//__ An event that is run when a video show stopped playing.
		//__
		virtual bool handle_eventVideoDone() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventGameLoaded()
		//__
		//__ An event that is run when game is loaded from a saved game. There is usually no need to use this event.
		//__
		virtual bool handle_eventGameLoaded() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventGameSaving()
		//__
		//__ An event that is run before game is saved. There is usually no need to use this event.
		//__
		virtual bool handle_eventGameSaving() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventGameSaved()
		//__
		//__ An event that is run after game is saved. There is usually no need to use this event.
		//__
		virtual bool handle_eventGameSaved() SCRIPTING_EVENT_NON_REQUIRED

	public:
		// MARK: Transporter events

		//__
		//__ ## eventTransporterLaunch(transport)
		//__
		//__ An event that is run when the mission transporter has been ordered to fly off.
		//__
		virtual bool handle_eventLaunchTransporter() SCRIPTING_EVENT_NON_REQUIRED // DEPRECATED!
		virtual bool handle_eventTransporterLaunch(const BASE_OBJECT *psTransport) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventTransporterArrived(transport)
		//__
		//__ An event that is run when the mission transporter has arrived at the map edge with reinforcements.
		//__
		virtual bool handle_eventReinforcementsArrived() SCRIPTING_EVENT_NON_REQUIRED // DEPRECATED!
		virtual bool handle_eventTransporterArrived(const BASE_OBJECT *psTransport) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventTransporterExit(transport)
		//__
		//__ An event that is run when the mission transporter has left the map.
		//__
		virtual bool handle_eventTransporterExit(const BASE_OBJECT *psObj) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventTransporterDone(transport)
		//__
		//__ An event that is run when the mission transporter has no more reinforcements to deliver.
		//__
		virtual bool handle_eventTransporterDone(const BASE_OBJECT *psTransport) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventTransporterLanded(transport)
		//__
		//__ An event that is run when the mission transporter has landed with reinforcements.
		//__
		virtual bool handle_eventTransporterLanded(const BASE_OBJECT *psTransport) SCRIPTING_EVENT_NON_REQUIRED

	public:
		// MARK: UI-related events (intended for the tutorial)

		//__ ## eventDeliveryPointMoving()
		//__
		//__ An event that is run when the current player starts to move a delivery point.
		//__
		virtual bool handle_eventDeliveryPointMoving(const BASE_OBJECT *psStruct) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventDeliveryPointMoved()
		//__
		//__ An event that is run after the current player has moved a delivery point.
		//__
		virtual bool handle_eventDeliveryPointMoved(const BASE_OBJECT *psStruct) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventDesignBody()
		//__
		//__An event that is run when current user picks a body in the design menu.
		//__
		virtual bool handle_eventDesignBody() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventDesignPropulsion()
		//__
		//__An event that is run when current user picks a propulsion in the design menu.
		//__
		virtual bool handle_eventDesignPropulsion() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventDesignWeapon()
		//__
		//__An event that is run when current user picks a weapon in the design menu.
		//__
		virtual bool handle_eventDesignWeapon() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventDesignCommand()
		//__
		//__An event that is run when current user picks a command turret in the design menu.
		//__
		virtual bool handle_eventDesignCommand() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventDesignSystem()
		//__
		//__An event that is run when current user picks a system other than command turret in the design menu.
		//__
		virtual bool handle_eventDesignSystem() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventDesignQuit()
		//__
		//__An event that is run when current user leaves the design menu.
		//__
		virtual bool handle_eventDesignQuit() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventMenuBuildSelected()
		//__
		//__An event that is run when current user picks something new in the build menu.
		//__
		virtual bool handle_eventMenuBuildSelected(/*BASE_OBJECT *psObj*/) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventMenuResearchSelected()
		//__
		//__An event that is run when current user picks something new in the research menu.
		//__
		virtual bool handle_eventMenuResearchSelected(/*BASE_OBJECT *psObj*/) SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventMenuBuild()
		//__
		//__An event that is run when current user opens the build menu.
		//__
		virtual bool handle_eventMenuBuild() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventMenuResearch()
		//__
		//__An event that is run when current user opens the research menu.
		//__
		virtual bool handle_eventMenuResearch() SCRIPTING_EVENT_NON_REQUIRED


		virtual bool handle_eventMenuDesign() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventMenuManufacture()
		//__An event that is run when current user opens the manufacture menu.
		//__
		virtual bool handle_eventMenuManufacture() SCRIPTING_EVENT_NON_REQUIRED

		//__ ## eventSelectionChanged(objects)
		//__
		//__ An event that is triggered whenever the host player selects one or more game objects.
		//__ The ```objects``` parameter contains an array of the currently selected game objects.
		//__ Keep in mind that the player may drag and drop select many units at once, select one
		//__ unit specifically, or even add more selections to a current selection one at a time.
		//__ This event will trigger once for each user action, not once for each selected or
		//__ deselected object. If all selected game objects are deselected, ```objects``` will
		//__ be empty.
		//__
		virtual bool handle_eventSelectionChanged(const std::vector<const BASE_OBJECT *>& objects) SCRIPTING_EVENT_NON_REQUIRED

	public:
		// MARK: Game state-change events

		//__ ## eventObjectRecycled()
		//__
		//__ An event that is run when an object (ex. droid, structure) is recycled.
		//__
		virtual bool handle_eventObjectRecycled(const BASE_OBJECT *psObj) = 0;

		//__ ## eventPlayerLeft(player index)
		//__
		//__ An event that is run after a player has left the game.
		//__
		virtual bool handle_eventPlayerLeft(int id) = 0;

		//__ ## eventCheatMode(entered)
		//__
		//__ Game entered or left cheat/debug mode.
		//__ The entered parameter is true if cheat mode entered, false otherwise.
		//__
		virtual bool handle_eventCheatMode(bool entered) = 0;

		//__ ## eventDroidIdle(droid)
		//__
		//__ A droid should be given new orders.
		//__
		virtual bool handle_eventDroidIdle(const DROID *psDroid) = 0;

		//__ ## eventDroidBuilt(droid[, structure])
		//__
		//__ An event that is run every time a droid is built. The structure parameter is set
		//__ if the droid was produced in a factory. It is not triggered for droid theft or
		//__ gift (check ```eventObjectTransfer``` for that).
		//__
		virtual bool handle_eventDroidBuilt(const DROID *psDroid, const STRUCTURE *psFactory) = 0;

		//__ ## eventStructureBuilt(structure[, droid])
		//__
		//__ An event that is run every time a structure is produced. The droid parameter is set
		//__ if the structure was built by a droid. It is not triggered for building theft
		//__ (check ```eventObjectTransfer``` for that).
		//__
		virtual bool handle_eventStructBuilt(const STRUCTURE *psStruct, const DROID *psDroid) = 0;

		//__ ## eventStructureDemolish(structure[, droid])
		//__
		//__ An event that is run every time a structure begins to be demolished. This does
		//__ not trigger again if the structure is partially demolished.
		//__
		virtual bool handle_eventStructDemolish(const STRUCTURE *psStruct, const DROID *psDroid) = 0;

		//__ ## eventStructureReady(structure)
		//__
		//__ An event that is run every time a structure is ready to perform some
		//__ special ability. It will only fire once, so if the time is not right,
		//__ register your own timer to keep checking.
		//__
		virtual bool handle_eventStructureReady(const STRUCTURE *psStruct) = 0;

		//__ ## eventAttacked(victim, attacker)
		//__
		//__ An event that is run when an object belonging to the script's controlling player is
		//__ attacked. The attacker parameter may be either a structure or a droid.
		//__
		virtual bool handle_eventAttacked(const BASE_OBJECT *psVictim, const BASE_OBJECT *psAttacker) = 0;

		//__ ## eventResearched(research, structure, player)
		//__
		//__ An event that is run whenever a new research is available. The structure
		//__ parameter is set if the research comes from a research lab owned by the
		//__ current player. If an ally does the research, the structure parameter will
		//__ be set to null. The player parameter gives the player it is called for.
		//__
		virtual bool handle_eventResearched(const researchResult& research, const STRUCTURE *psStruct, int player) = 0;

		//__ ## eventDestroyed(object)
		//__
		//__ An event that is run whenever an object is destroyed. Careful passing
		//__ the parameter object around, since it is about to vanish!
		//__
		virtual bool handle_eventDestroyed(const BASE_OBJECT *psVictim) = 0;

		//__ ## eventPickup(feature, droid)
		//__
		//__ An event that is run whenever a feature is picked up. It is called for
		//__ all players / scripts.
		//__ Careful passing the parameter object around, since it is about to vanish! (3.2+ only)
		//__
		virtual bool handle_eventPickup(const FEATURE *psFeat, const DROID *psDroid) = 0;

		//__ ## eventObjectSeen(viewer, seen)
		//__
		//__ An event that is run sometimes when an object, which was marked by an object label,
		//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
		//__ An event that is run sometimes when an objectm  goes from not seen to seen.
		//__ First parameter is **game object** doing the seeing, the next the game
		//__ object being seen.
		virtual bool handle_eventObjectSeen(const BASE_OBJECT *psViewer, const BASE_OBJECT *psSeen) = 0;

		//__
		//__ ## eventGroupSeen(viewer, group)
		//__
		//__ An event that is run sometimes when a member of a group, which was marked by a group label,
		//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
		//__ First parameter is **game object** doing the seeing, the next the id of the group
		//__ being seen.
		//__
		virtual bool handle_eventGroupSeen(const BASE_OBJECT *psViewer, int groupId) = 0;

		//__ ## eventObjectTransfer(object, from)
		//__
		//__ An event that is run whenever an object is transferred between players,
		//__ for example due to a Nexus Link weapon. The event is called after the
		//__ object has been transferred, so the target player is in object.player.
		//__ The event is called for both players.
		//__
		virtual bool handle_eventObjectTransfer(const BASE_OBJECT *psObj, int from) = 0;

		//__ ## eventChat(from, to, message)
		//__
		//__ An event that is run whenever a chat message is received. The ```from``` parameter is the
		//__ player sending the chat message. For the moment, the ```to``` parameter is always the script
		//__ player.
		//__
		virtual bool handle_eventChat(int from, int to, const char *message) = 0;

		//__ ## eventBeacon(x, y, from, to[, message])
		//__
		//__ An event that is run whenever a beacon message is received. The ```from``` parameter is the
		//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
		//__ Message may be undefined.
		//__
		virtual bool handle_eventBeacon(int x, int y, int from, int to, const char *message) = 0;

		//__ ## eventBeaconRemoved(from, to)
		//__
		//__ An event that is run whenever a beacon message is removed. The ```from``` parameter is the
		//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
		//__
		virtual bool handle_eventBeaconRemoved(int from, int to) = 0;

		//__ ## eventGroupLoss(object, group id, new size)
		//__
		//__ An event that is run whenever a group becomes empty. Input parameter
		//__ is the about to be killed object, the group's id, and the new group size.
		//__
//		// Since groups are entities local to one context, we do not iterate over them here.
		virtual bool handle_eventGroupLoss(const BASE_OBJECT *psObj, int group, int size) = 0;

		//__ ## eventArea<label>(droid)
		//__
		//__ An event that is run whenever a droid enters an area label. The area is then
		//__ deactived. Call resetArea() to reactivate it. The name of the event is
		//__ eventArea + the name of the label.
		//__
		virtual bool handle_eventArea(const std::string& label, const DROID *psDroid) = 0;

		//__ ## eventDesignCreated(template)
		//__
		//__ An event that is run whenever a new droid template is created. It is only
		//__ run on the client of the player designing the template.
		//__
		virtual bool handle_eventDesignCreated(const DROID_TEMPLATE *psTemplate) = 0;

		//__ ## eventAllianceOffer(from, to)
		//__
		//__ An event that is called whenever an alliance offer is requested.
		//__
		virtual bool handle_eventAllianceOffer(uint8_t from, uint8_t to) = 0;

		//__ ## eventAllianceAccepted(from, to)
		//__
		//__ An event that is called whenever an alliance is accepted.
		//__
		virtual bool handle_eventAllianceAccepted(uint8_t from, uint8_t to) = 0;

		//__ ## eventAllianceBroken(from, to)
		//__
		//__ An event that is called whenever an alliance is broken.
		//__
		virtual bool handle_eventAllianceBroken(uint8_t from, uint8_t to) = 0;

	public:
		// MARK: Special input events

		//__ ## eventSyncRequest(req_id, x, y, obj_id, obj_id2)
		//__
		//__ An event that is called from a script and synchronized with all other scripts and hosts
		//__ to prevent desync from happening. Sync requests must be carefully validated to prevent
		//__ cheating!
		//__
		virtual bool handle_eventSyncRequest(int from, int req_id, int x, int y, const BASE_OBJECT *psObj, const BASE_OBJECT *psObj2) = 0;

		//__ ## eventKeyPressed(meta, key)
		//__
		//__ An event that is called whenever user presses a key in the game, not counting chat
		//__ or other pop-up user interfaces. The key values are currently undocumented.
		virtual bool handle_eventKeyPressed(int meta, int key) SCRIPTING_EVENT_NON_REQUIRED
	};

	enum class GlobalVariableFlags
	{
		None = 0,
		ReadOnly = 1 << 0, // binary 0001
		DoNotSave = 1 << 1, // binary 0010
	};
	inline GlobalVariableFlags operator | (GlobalVariableFlags lhs, GlobalVariableFlags rhs)
	{
		using T = std::underlying_type<GlobalVariableFlags>::type;
		return static_cast<GlobalVariableFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
	}
	inline GlobalVariableFlags& operator |= (GlobalVariableFlags& lhs, GlobalVariableFlags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}
	inline GlobalVariableFlags operator& (GlobalVariableFlags lhs, GlobalVariableFlags rhs)
	{
		using T = std::underlying_type<GlobalVariableFlags>::type;
		return static_cast<GlobalVariableFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
	}
	inline GlobalVariableFlags& operator&= (GlobalVariableFlags& lhs, GlobalVariableFlags rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	class scripting_instance : public scripting_event_handling_interface
	{
	public:
		scripting_instance(int player, const std::string& scriptName);
		virtual ~scripting_instance();

	public:
		virtual bool readyInstanceForExecution() = 0;

	public:
		const std::string& scriptName() const { return m_scriptName; }
		int player() const { return m_player; }
//		virtual const wzapi::execution_context& getContext() = 0;

	public:
		virtual bool isReceivingAllEvents() const { return false; }

	public:
		// event handling
		// - see `scripting_event_handling_interface`

	public:
		// save / restore state
		virtual bool saveScriptGlobals(nlohmann::json &result) = 0;
		virtual bool loadScriptGlobals(const nlohmann::json &result) = 0;

		virtual nlohmann::json saveTimerFunction(uniqueTimerID timerID, std::string timerName, timerAdditionalData* additionalParam) = 0;

		// recreates timer functions (and additional userdata) based on the information saved by the saveTimerFunction() method
		virtual std::tuple<TimerFunc, timerAdditionalData *> restoreTimerFunction(const nlohmann::json& savedTimerFuncData) = 0;

	public:
		// get state for debugging
		virtual nlohmann::json debugGetAllScriptGlobals() = 0;

		virtual bool debugEvaluateCommand(const std::string &text) = 0;

	public:
		// output to debug log file
		void dumpScriptLog(const std::string &info);
		void dumpScriptLog(const std::string &info, int me);

	public:
		virtual void updateGameTime(uint32_t gameTime) = 0;
		virtual void updateGroupSizes(int group, int size) = 0;

		// set "global" variables
		//
		// expects: a json object (keys ("variable names") -> values)
		//
		// as appropriate for this scripting_instance, modifies "global variables" that scripts can access
		// for each key in the json object, it sets the appropriate "global variable" to the associated value
		//
		// only modifies global variables for keys in the json object - if other global variables already exist
		// in this scripting_instance (ex. from a prior call to this function), they are maintained
		//
		// flags: - GlobalVariableFlags::ReadOnly - if supported by the scripting instance, should set constant / read-only variables
		//          that the script(s) themselves cannot modify (but may be updated by WZ via future calls to this function)
		//        - GlobalVariableFlags::DoNotSave - indicates that the global variable(s) should not be saved by saveScriptGlobals()
		virtual void setSpecifiedGlobalVariables(const nlohmann::json& variables, GlobalVariableFlags flags = GlobalVariableFlags::ReadOnly | GlobalVariableFlags::DoNotSave) = 0;

		virtual void setSpecifiedGlobalVariable(const std::string& name, const nlohmann::json& value, GlobalVariableFlags flags = GlobalVariableFlags::ReadOnly | GlobalVariableFlags::DoNotSave) = 0;

	private:
		int m_player;
		std::string m_scriptName;
	};

	class execution_context
	{
	public:
		virtual ~execution_context() { };
	public:
		virtual  wzapi::scripting_instance* currentInstance() const = 0;
		virtual int player() const = 0;
		virtual void throwError(const char *expr, int line, const char *function) const = 0;
		virtual playerCallbackFunc getNamedScriptCallback(const WzString& func) const = 0;
		virtual void hack_setMe(int player) const = 0;
		virtual void set_isReceivingAllEvents(bool value) const = 0;
		virtual bool get_isReceivingAllEvents() const = 0;
		virtual void doNotSaveGlobal(const std::string &global) const = 0;
	};

	struct game_object_identifier
	{
		game_object_identifier() { }
		game_object_identifier(const BASE_OBJECT* psObj)
		: type(psObj->type)
		, id(psObj->id)
		, player(psObj->player)
		{ }
		int type;
		int id = -1;
		int player = -1;
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

	struct object_request
	{
	public:
		object_request();
		object_request(const std::string& label);
		object_request(int x, int y);
		object_request(OBJECT_TYPE type, int player, int id);
	public:
		enum class RequestType
		{
			INVALID_REQUEST,
			LABEL_REQUEST,
			MAPPOS_REQUEST,
			OBJECTID_REQUEST
		};
	public:
		const std::string& getLabel() const;
		/*std::tuple<int, int>*/ scr_position getMapPosition() const;
		std::tuple<OBJECT_TYPE, int, int> getObjectIDRequest() const;
	public:
		const RequestType requestType;
	private:
		const std::string str;
		const int val1 = -1;
		const int val2 = -1;
		const int val3 = -1;
	};

	struct label_or_position_values
	{
	private:
		const int VERY_LOW_INVALID_POS_VALUE = -2;
	public:
		label_or_position_values() { }
		label_or_position_values(const std::string &label)
		: type(Type::Label_Request)
		, label(label)
		{ }
		label_or_position_values(int32_t x1, int32_t y1, optional<int32_t> x2 = nullopt, optional<int32_t> y2 = nullopt)
		: type(Type::Position_Values_Request)
		, x1(x1), y1(y1), x2(x2), y2(y2)
		{ }
	public:
		inline bool isValid() { return type != Type::Invalid_Request; }
		inline bool isLabel() { return type == Type::Label_Request; }
		inline bool isPositionValues() { return type == Type::Position_Values_Request; }
	public:
		enum Type
		{
			Invalid_Request,
			Label_Request,
			Position_Values_Request
		};
		Type type = Invalid_Request;

		int32_t x1 = VERY_LOW_INVALID_POS_VALUE;
		int32_t y1 = VERY_LOW_INVALID_POS_VALUE;
		optional<int32_t> x2;
		optional<int32_t> y2;
		std::string label;
	};

	// retVals
	struct no_return_value
	{ };
	struct researchResult
	{
		researchResult() { }
		researchResult(const RESEARCH * psResearch, int player)
		: psResearch(psResearch)
		, player(player)
		{ }
		const RESEARCH * psResearch = nullptr;
		int player;
	};
	struct researchResults
	{
		std::vector<const RESEARCH *> resList;
		int player;
	};

	class GameEntityRules
	{
	public:
		typedef std::map<std::string, int> NameToTypeMap;
		GameEntityRules(int player, unsigned index, const NameToTypeMap& nameToTypeMap)
		: player(player)
		, index(index)
		, propertyNameToTypeMap(nameToTypeMap)
		{ }
		//GameEntityRules() { }
	public:
		using value_type = nlohmann::json;
		value_type getPropertyValue(const wzapi::execution_context& context, const std::string& name) const;
		value_type setPropertyValue(const wzapi::execution_context& context, const std::string& name, const value_type& newValue);
	public:
		inline NameToTypeMap::const_iterator begin() const
		{
			return propertyNameToTypeMap.cbegin();
		}
		inline NameToTypeMap::const_iterator end() const
		{
			return propertyNameToTypeMap.cend();
		}
		inline int getPlayer() const { return player; }
		inline int getIndex() const { return index; }
	private:
		// context
		int player = -1;
		unsigned index = 0;
		NameToTypeMap propertyNameToTypeMap;
	};

	class GameEntityRuleContainer
	{
	public:
		typedef std::string GameEntityName;
	public:
		GameEntityRuleContainer()
		{ }
		inline void addRules(const GameEntityName& statsName, GameEntityRules&& entityRules)
		{
			rules.emplace(statsName, std::move(entityRules));
		}
	public:
		inline GameEntityRules& operator[](const GameEntityName& statsName)
		{
			return rules.at(statsName);
		}
		inline std::unordered_map<GameEntityName, GameEntityRules>::const_iterator begin() const
		{
			return rules.cbegin();
		}
		inline std::unordered_map<GameEntityName, GameEntityRules>::const_iterator end() const
		{
			return rules.cend();
		}
	private:
		std::unordered_map<GameEntityName, GameEntityRules> rules;
	};

	class PerPlayerUpgrades
	{
	public:
		typedef std::string GameEntityClass;
	public:
		PerPlayerUpgrades(int player)
		: player(player)
		{ }
		inline void addGameEntity(const GameEntityClass& entityClass, GameEntityRuleContainer&& rulesContainer)
		{
			upgrades.emplace(entityClass, std::move(rulesContainer));
		}
	public:
		inline GameEntityRuleContainer& operator[](const GameEntityClass& entityClass)
		{
			return upgrades[entityClass];
		}
		inline int getPlayer() const { return player; }
		inline std::map<GameEntityClass, GameEntityRuleContainer>::const_iterator begin() const
		{
			return upgrades.cbegin();
		}
		inline std::map<GameEntityClass, GameEntityRuleContainer>::const_iterator end() const
		{
			return upgrades.cend();
		}
	private:
		std::map<GameEntityClass, GameEntityRuleContainer> upgrades;
		int player = 0;
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
	int32_t getMissionType(WZAPI_NO_PARAMS);
	bool getRevealStatus(WZAPI_NO_PARAMS);
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
	no_return_value hackMarkTiles(WZAPI_PARAMS(optional<label_or_position_values> _tilePosOrArea));

	// General functions -- geared for use in AI scripts
	no_return_value dump(WZAPI_PARAMS(va_list_treat_as_strings strings));
	no_return_value debugOutputStrings(WZAPI_PARAMS(va_list_treat_as_strings strings));
	bool console(WZAPI_PARAMS(va_list_treat_as_strings strings));
	bool clearConsole(WZAPI_NO_PARAMS);
	bool structureIdle(WZAPI_PARAMS(const STRUCTURE *psStruct));
	std::vector<const STRUCTURE *> enumStruct(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking));
	std::vector<const STRUCTURE *> enumStructOffWorld(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking));
	std::vector<const DROID *> enumDroid(WZAPI_PARAMS(optional<int> _player, optional<int> _droidType, optional<int> _looking));
	std::vector<const FEATURE *> enumFeature(WZAPI_PARAMS(int looking, optional<std::string> _statsName));
	std::vector<scr_position> enumBlips(WZAPI_PARAMS(int player));
	std::vector<const BASE_OBJECT *> enumSelected(WZAPI_NO_PARAMS_NO_CONTEXT);
	GATEWAY_LIST enumGateways(WZAPI_NO_PARAMS);
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
	optional<scr_position> pickStructLocation(WZAPI_PARAMS(const DROID *psDroid, std::string statName, int startX, int startY, optional<int> _maxBlockingTiles));
	bool droidCanReach(WZAPI_PARAMS(const DROID *psDroid, int x, int y));
	bool propulsionCanReach(WZAPI_PARAMS(std::string propulsionName, int x1, int y1, int x2, int y2));
	int terrainType(WZAPI_PARAMS(int x, int y));
	bool orderDroidObj(WZAPI_PARAMS(DROID *psDroid, int _order, BASE_OBJECT *psObj));
	bool buildDroid(WZAPI_PARAMS(STRUCTURE *psFactory, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets));
	const DROID* addDroid(WZAPI_PARAMS(int player, int x, int y, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets)); MUTLIPLAY_UNSAFE
	std::unique_ptr<const DROID_TEMPLATE> makeTemplate(WZAPI_PARAMS(int player, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, va_list<string_or_string_list> turrets));
	bool addDroidToTransporter(WZAPI_PARAMS(game_object_identifier transporter, game_object_identifier droid));
	const FEATURE * addFeature(WZAPI_PARAMS(std::string featName, int x, int y)) MUTLIPLAY_UNSAFE;
	bool componentAvailable(WZAPI_PARAMS(std::string arg1, optional<std::string> arg2));
	bool isVTOL(WZAPI_PARAMS(const DROID *psDroid));
	bool safeDest(WZAPI_PARAMS(int player, int x, int y));
	bool activateStructure(WZAPI_PARAMS(STRUCTURE *psStruct, optional<BASE_OBJECT *> _psTarget));
	bool chat(WZAPI_PARAMS(int target, std::string message));
	bool addBeacon(WZAPI_PARAMS(int x, int y, int target, optional<std::string> _message));
	bool removeBeacon(WZAPI_PARAMS(int target));
	std::unique_ptr<const DROID> getDroidProduction(WZAPI_PARAMS(const STRUCTURE *_psFactory));
	int getDroidLimit(WZAPI_PARAMS(optional<int> _player, optional<int> _unitType));
	int getExperienceModifier(WZAPI_PARAMS(int player));
	bool setDroidLimit(WZAPI_PARAMS(int player, int value, optional<int> _droidType));
	bool setCommanderLimit(WZAPI_PARAMS(int player, int value));
	bool setConstructorLimit(WZAPI_PARAMS(int player, int value));
	bool setExperienceModifier(WZAPI_PARAMS(int player, int percent));
	std::vector<const DROID *> enumCargo(WZAPI_PARAMS(const DROID *psDroid));

	nlohmann::json getWeaponInfo(WZAPI_PARAMS(std::string weaponID)) WZAPI_DEPRECATED;

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
	bool removeStruct(WZAPI_PARAMS(STRUCTURE *psStruct)) WZAPI_DEPRECATED;
	bool removeObject(WZAPI_PARAMS(BASE_OBJECT *psObj, optional<bool> _sfx));
	no_return_value setScrollLimits(WZAPI_PARAMS(int x1, int y1, int x2, int y2));
	scr_area getScrollLimits(WZAPI_NO_PARAMS);
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

	// MARK: - Used for retrieving information to set up script instance environments
	nlohmann::json constructStatsObject();
	nlohmann::json getUsefulConstants();
	nlohmann::json constructStaticPlayerData();
	std::vector<PerPlayerUpgrades> getUpgradesObject();
}

#endif
