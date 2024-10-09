/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2020  Warzone 2100 Project

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
/**
 * @file quickjs_backend.cpp
 *
 * New scripting system -- script functions
 */

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/wzpaths.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/string_ext.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_opengl/tex.h"

#include "action.h"
#include "clparse.h"
#include "combat.h"
#include "console.h"
#include "design.h"
#include "display3d.h"
#include "map.h"
#include "mission.h"
#include "move.h"
#include "order.h"
#include "transporter.h"
#include "message.h"
#include "display3d.h"
#include "intelmap.h"
#include "hci.h"
#include "wrappers.h"
#include "challenge.h"
#include "research.h"
#include "multilimit.h"
#include "multigifts.h"
#include "multimenu.h"
#include "template.h"
#include "lighting.h"
#include "radar.h"
#include "random.h"
#include "frontend.h"
#include "loop.h"
#include "gateway.h"
#include "mapgrid.h"
#include "lighting.h"
#include "atmos.h"
#include "warcam.h"
#include "projectile.h"
#include "component.h"
#include "seqdisp.h"
#include "ai.h"
#include "advvis.h"
#include "loadsave.h"
#include "wzapi.h"
#include "qtscript.h"
#include "featuredef.h"
#include "data.h"


#include <unordered_set>
#include "lib/framework/file.h"
#include <unordered_map>
#include <limits>

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
MSVC_PRAGMA(warning( push ))
MSVC_PRAGMA(warning( disable : 4191 )) // disable "warning C4191: 'type cast': unsafe conversion from 'JSCFunctionMagic (__cdecl *)' to 'JSCFunction (__cdecl *)'"
#include "quickjs.h"
#include "quickjs-debugger.h"
#include "quickjs-limitedcontext.h"
MSVC_PRAGMA(warning( pop ))
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif
#include "3rdparty/gsl_finally.h"
#include <utility>

// Alternatives for C++ - can't use the JS_CFUNC_DEF / JS_CGETSET_DEF / etc defines
// #define JS_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }
static inline JSCFunctionListEntry QJS_CFUNC_DEF(const char *name, uint8_t length, JSCFunction *func1)
{
	JSCFunctionListEntry entry;
	entry.name = name;
	entry.prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE;
	entry.def_type = JS_DEF_CFUNC;
	entry.magic = 0;
	entry.u.func.length = length;
	entry.u.func.cproto = JS_CFUNC_generic;
	entry.u.func.cfunc.generic = func1;
	return entry;
}
// #define JS_CGETSET_DEF(name, fgetter, fsetter) { name, JS_PROP_CONFIGURABLE, JS_DEF_CGETSET, 0, .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }
typedef JSValue JSCFunctionGetter(JSContext *ctx, JSValueConst this_val);
typedef JSValue JSCFunctionSetter(JSContext *ctx, JSValueConst this_val, JSValueConst val);
static inline JSCFunctionListEntry QJS_CGETSET_DEF(const char *name, JSCFunctionGetter *fgetter, JSCFunctionSetter *fsetter)
{
	JSCFunctionListEntry entry;
	entry.name = name;
	entry.prop_flags = JS_PROP_CONFIGURABLE;
	entry.def_type = JS_DEF_CGETSET;
	entry.magic = 0;
	entry.u.getset.get.getter = fgetter;
	entry.u.getset.set.setter = fsetter;
	return entry;
}

struct JSContextValue {
	JSContext *ctx;
	JSValue value;
	bool skip_constructors;
};
void to_json(nlohmann::json& j, const JSContextValue& value); // forward-declare

bool QuickJS_EnumerateObjectProperties(JSContext *ctx, JSValue obj, const std::function<void (const char *key, JSAtom& atom)>& func, bool enumerableOnly = true); // forward-declare

class quickjs_scripting_instance;
static std::map<JSContext*, quickjs_scripting_instance *> engineToInstanceMap;

static void QJSRuntimeFree_LeakHandler_Error(const char* msg)
{
	debug(LOG_ERROR, "QuickJS FreeRuntime leak: %s", msg);
}

class quickjs_scripting_instance : public wzapi::scripting_instance
{
public:
	quickjs_scripting_instance(int player, const std::string& scriptName, const std::string& scriptPath)
	: scripting_instance(player, scriptName, scriptPath)
	{
		rt = JS_NewRuntime();
		ASSERT(rt != nullptr, "JS_NewRuntime failed?");

		JSLimitedContextOptions ctxOptions = { };
		ctxOptions.baseObjects = true;
		ctxOptions.dateObject = true;
		ctxOptions.eval = (game.type == LEVEL_TYPE::CAMPAIGN); // allow "eval" only for campaign (which currently has lots of implicit eval usage)
		ctxOptions.stringNormalize = true;
		ctxOptions.regExp = true;
		ctxOptions.json = true;
		ctxOptions.proxy = true;
		ctxOptions.mapSet = true;
		ctxOptions.typedArrays = true;
		ctxOptions.promise = false; // disable promise, async, await
		ctxOptions.bigInt = false;
		ctx = JS_NewLimitedContext(rt, &ctxOptions);
		ASSERT(ctx != nullptr, "JS_NewContext failed?");

		global_obj = JS_GetGlobalObject(ctx);

		engineToInstanceMap.insert(std::pair<JSContext*, quickjs_scripting_instance*>(ctx, this));
	}
	virtual ~quickjs_scripting_instance()
	{
		engineToInstanceMap.erase(ctx);

		debug(LOG_INFO, "Destroying [%d]:%s/%s", player(), scriptPath().c_str(), scriptName().c_str());

		if (!(JS_IsUninitialized(compiledScriptObj)))
		{
			JS_FreeValue(ctx, compiledScriptObj);
			compiledScriptObj = JS_UNINITIALIZED;
		}

		JS_FreeValue(ctx, global_obj);
		ASSERT(ctx != nullptr, "context is null??");
		if (ctx)
		{
			JS_FreeContext(ctx);
			ctx = nullptr;
		}
		ASSERT(rt != nullptr, "runtime is null??");
		if (rt)
		{
			JS_FreeRuntime2(rt, QJSRuntimeFree_LeakHandler_Error);
			rt = nullptr;
		}
	}
	bool loadScript(const WzString& path, int player, int difficulty);
	bool readyInstanceForExecution() override;

private:
	bool registerFunctions(const std::string& scriptName);

public:
	// save / restore state
	virtual bool saveScriptGlobals(nlohmann::json &result) override;
	virtual bool loadScriptGlobals(const nlohmann::json &result) override;

	virtual nlohmann::json saveTimerFunction(uniqueTimerID timerID, std::string timerName, const timerAdditionalData* additionalParam) override;

	// recreates timer functions (and additional userdata) based on the information saved by the saveTimerFunction() method
	virtual std::tuple<TimerFunc, std::unique_ptr<timerAdditionalData>> restoreTimerFunction(const nlohmann::json& savedTimerFuncData) override;

public:
	// get state for debugging
	nlohmann::json debugGetAllScriptGlobals() override;
	std::unordered_map<std::string, wzapi::scripting_instance::DebugSpecialStringType> debugGetScriptGlobalSpecialStringValues() override;

	bool debugEvaluateCommand(const std::string &text) override;

public:

	void updateGameTime(uint32_t gameTime) override;
	void updateGroupSizes(int group, int size) override;

	void setSpecifiedGlobalVariables(const nlohmann::json& variables, wzapi::GlobalVariableFlags flags = wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave) override;

	void setSpecifiedGlobalVariable(const std::string& name, const nlohmann::json& value, wzapi::GlobalVariableFlags flags = wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave) override;

private:
	inline int toQuickJSPropertyFlags(wzapi::GlobalVariableFlags flags)
	{
		int propertyFlags = 0;
		if (((flags & wzapi::GlobalVariableFlags::ReadOnly) != wzapi::GlobalVariableFlags::ReadOnly)
			|| ((flags & wzapi::GlobalVariableFlags::ReadOnlyUpdatedFromApp) == wzapi::GlobalVariableFlags::ReadOnlyUpdatedFromApp))
		{
			// NOTE: QuickJS prevents even the app - not just scripts - from updating unwritable / read-only variables
			// So if ReadOnlyUpdatedFromApp is set, we must set the variable to writable to support updates from WZ
			propertyFlags |= JS_PROP_WRITABLE;
		}
		return propertyFlags;
	}

public:

	void doNotSaveGlobal(const std::string &global);

private:
	JSRuntime *rt;
    JSContext *ctx;
	JSValue global_obj;

	JSValue compiledScriptObj = JS_UNINITIALIZED;
	std::string m_path;
	/// Remember what names are used internally in the scripting engine, we don't want to save these to the savegame
	std::unordered_set<std::string> internalNamespace;
	/// Separate event namespaces for libraries
public: // temporary
	std::vector<std::string> eventNamespaces;
	JSValue Get_Global_Obj() const { return global_obj; }

public:
	// MARK: General events

	//__ ## eventGameInit()
	//__
	//__ An event that is run once as the game is initialized. Not all game state may have been
	//__ properly initialized by this time, so use this only to initialize script state.
	//__
	virtual bool handle_eventGameInit() override;

	//__ ## eventStartLevel()
	//__
	//__ An event that is run once the game has started and all game data has been loaded.
	//__
	virtual bool handle_eventStartLevel() override;

	//__ ## eventMissionTimeout()
	//__
	//__ An event that is run when the mission timer has run out.
	//__
	virtual bool handle_eventMissionTimeout() override;

	//__ ## eventVideoDone()
	//__
	//__ An event that is run when a video show stopped playing.
	//__
	virtual bool handle_eventVideoDone() override;

	//__ ## eventGameLoaded()
	//__
	//__ An event that is run when game is loaded from a saved game. There is usually no need to use this event.
	//__
	virtual bool handle_eventGameLoaded() override;

	//__ ## eventGameSaving()
	//__
	//__ An event that is run before game is saved. There is usually no need to use this event.
	//__
	virtual bool handle_eventGameSaving() override;

	//__ ## eventGameSaved()
	//__
	//__ An event that is run after game is saved. There is usually no need to use this event.
	//__
	virtual bool handle_eventGameSaved() override;

public:
	// MARK: Transporter events

	//__ ## eventTransporterLaunch(transport)
	//__
	//__ An event that is run when the mission transporter has been ordered to fly off.
	//__
	virtual bool handle_eventLaunchTransporter() override; // DEPRECATED!
	virtual bool handle_eventTransporterLaunch(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterArrived(transport)
	//__
	//__ An event that is run when the mission transporter has arrived at the map edge with reinforcements.
	//__
	virtual bool handle_eventReinforcementsArrived() override; // DEPRECATED!
	virtual bool handle_eventTransporterArrived(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterExit(transport)
	//__
	//__ An event that is run when the mission transporter has left the map.
	//__
	virtual bool handle_eventTransporterExit(const BASE_OBJECT *psObj) override;

	//__ ## eventTransporterDone(transport)
	//__
	//__ An event that is run when the mission transporter has no more reinforcements to deliver.
	//__
	virtual bool handle_eventTransporterDone(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterLanded(transport)
	//__
	//__ An event that is run when the mission transporter has landed with reinforcements.
	//__
	virtual bool handle_eventTransporterLanded(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterEmbarked(transport)
	//__
	//__ An event that is run when a unit embarks into a transporter.
	//__
	virtual bool handle_eventTransporterEmbarked(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterDisembarked(transport)
	//__
	//__ An event that is run when a unit disembarks from a transporter.
	//__
	virtual bool handle_eventTransporterDisembarked(const BASE_OBJECT *psTransport) override;

public:
	// MARK: UI-related events (intended for the tutorial)

	//__ ## eventDeliveryPointMoving()
	//__
	//__ An event that is run when the current player starts to move a delivery point.
	//__
	virtual bool handle_eventDeliveryPointMoving(const BASE_OBJECT *psStruct) override;

	//__ ## eventDeliveryPointMoved()
	//__
	//__ An event that is run after the current player has moved a delivery point.
	//__
	virtual bool handle_eventDeliveryPointMoved(const BASE_OBJECT *psStruct) override;

	//__ ## eventDesignBody()
	//__
	//__ An event that is run when current user picks a body in the design menu.
	//__
	virtual bool handle_eventDesignBody() override;

	//__ ## eventDesignPropulsion()
	//__
	//__ An event that is run when current user picks a propulsion in the design menu.
	//__
	virtual bool handle_eventDesignPropulsion() override;

	//__ ## eventDesignWeapon()
	//__
	//__ An event that is run when current user picks a weapon in the design menu.
	//__
	virtual bool handle_eventDesignWeapon() override;

	//__ ## eventDesignCommand()
	//__
	//__ An event that is run when current user picks a command turret in the design menu.
	//__
	virtual bool handle_eventDesignCommand() override;

	//__ ## eventDesignSystem()
	//__
	//__ An event that is run when current user picks a system other than command turret in the design menu.
	//__
	virtual bool handle_eventDesignSystem() override;

	//__ ## eventDesignQuit()
	//__
	//__ An event that is run when current user leaves the design menu.
	//__
	virtual bool handle_eventDesignQuit() override;

	//__ ## eventMenuBuildSelected()
	//__
	//__ An event that is run when current user picks something new in the build menu.
	//__
	virtual bool handle_eventMenuBuildSelected(/*BASE_OBJECT *psObj*/) override;

	//__ ## eventMenuResearchSelected()
	//__
	//__ An event that is run when current user picks something new in the research menu.
	//__
	virtual bool handle_eventMenuResearchSelected(/*BASE_OBJECT *psObj*/) override;

	//__ ## eventMenuBuild()
	//__
	//__ An event that is run when current user opens the build menu.
	//__
	virtual bool handle_eventMenuBuild() override;

	//__ ## eventMenuResearch()
	//__
	//__ An event that is run when current user opens the research menu.
	//__
	virtual bool handle_eventMenuResearch() override;


	virtual bool handle_eventMenuDesign() override;

	//__ ## eventMenuManufacture()
	//__
	//__ An event that is run when current user opens the manufacture menu.
	//__
	virtual bool handle_eventMenuManufacture() override;

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
	virtual bool handle_eventSelectionChanged(const std::vector<const BASE_OBJECT *>& objects) override;

public:
	// MARK: Game state-change events

	//__ ## eventObjectRecycled()
	//__
	//__ An event that is run when an object (ex. droid, structure) is recycled.
	//__
	virtual bool handle_eventObjectRecycled(const BASE_OBJECT *psObj) override;

	//__ ## eventPlayerLeft(player)
	//__
	//__ An event that is run after a player has left the game.
	//__
	virtual bool handle_eventPlayerLeft(int player) override;

	//__ ## eventCheatMode(entered)
	//__
	//__ Game entered or left cheat/debug mode.
	//__ The entered parameter is true if cheat mode entered, false otherwise.
	//__
	virtual bool handle_eventCheatMode(bool entered) override;

	//__ ## eventDroidIdle(droid)
	//__
	//__ A droid should be given new orders.
	//__
	virtual bool handle_eventDroidIdle(const DROID *psDroid) override;

	//__ ## eventDroidBuilt(droid[, structure])
	//__
	//__ An event that is run every time a droid is built. The structure parameter is set
	//__ if the droid was produced in a factory. It is not triggered for droid theft or
	//__ gift (check ```eventObjectTransfer``` for that).
	//__
	virtual bool handle_eventDroidBuilt(const DROID *psDroid, optional<const STRUCTURE *> psFactory) override;

	//__ ## eventStructureBuilt(structure[, droid])
	//__
	//__ An event that is run every time a structure is produced. The droid parameter is set
	//__ if the structure was built by a droid. It is not triggered for building theft
	//__ (check ```eventObjectTransfer``` for that).
	//__
	virtual bool handle_eventStructureBuilt(const STRUCTURE *psStruct, optional<const DROID *> psDroid) override;

	//__ ## eventStructureDemolish(structure[, droid])
	//__
	//__ An event that is run every time a structure begins to be demolished. This does
	//__ not trigger again if the structure is partially demolished.
	//__
	virtual bool handle_eventStructureDemolish(const STRUCTURE *psStruct, optional<const DROID *> psDroid) override;

	//__ ## eventStructureReady(structure)
	//__
	//__ An event that is run every time a structure is ready to perform some
	//__ special ability. It will only fire once, so if the time is not right,
	//__ register your own timer to keep checking.
	//__
	virtual bool handle_eventStructureReady(const STRUCTURE *psStruct) override;

	//__ ## eventStructureUpgradeStarted(structure)
	//__
	//__ An event that is run every time a structure starts to be upgraded.
	//__
	virtual bool handle_eventStructureUpgradeStarted(const STRUCTURE *psStruct) override;

	//__ ## eventDroidRankGained(droid, rankNum)
	//__
	//__ An event that is run whenever a droid gains a rank.
	//__
	virtual bool handle_eventDroidRankGained(const DROID *psDroid, int rankNum) override;

	//__ ## eventAttacked(victim, attacker)
	//__
	//__ An event that is run when an object belonging to the script's controlling player is
	//__ attacked. The attacker parameter may be either a structure or a droid.
	//__
	virtual bool handle_eventAttacked(const BASE_OBJECT *psVictim, const BASE_OBJECT *psAttacker) override;

	//__ ## eventResearched(research, structure, player)
	//__
	//__ An event that is run whenever a new research is available. The structure
	//__ parameter is set if the research comes from a research lab owned by the
	//__ current player. If an ally does the research, the structure parameter will
	//__ be set to null. The player parameter gives the player it is called for.
	//__
	virtual bool handle_eventResearched(const wzapi::researchResult& research, wzapi::event_nullable_ptr<const STRUCTURE> psStruct, int player) override;

	//__ ## eventDestroyed(object)
	//__
	//__ An event that is run whenever an object is destroyed. Careful passing
	//__ the parameter object around, since it is about to vanish!
	//__
	virtual bool handle_eventDestroyed(const BASE_OBJECT *psVictim) override;

	//__ ## eventPickup(feature, droid)
	//__
	//__ An event that is run whenever a feature is picked up. It is called for
	//__ all players / scripts.
	//__ Careful passing the parameter object around, since it is about to vanish! (3.2+ only)
	//__
	virtual bool handle_eventPickup(const FEATURE *psFeat, const DROID *psDroid) override;

	//__ ## eventObjectSeen(viewer, seen)
	//__
	//__ An event that is run sometimes when an object, which was marked by an object label,
	//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
	//__ An event that is run sometimes when an objectm  goes from not seen to seen.
	//__ First parameter is **game object** doing the seeing, the next the game
	//__ object being seen.
	virtual bool handle_eventObjectSeen(const BASE_OBJECT *psViewer, const BASE_OBJECT *psSeen) override;

	//__
	//__ ## eventGroupSeen(viewer, group)
	//__
	//__ An event that is run sometimes when a member of a group, which was marked by a group label,
	//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
	//__ First parameter is **game object** doing the seeing, the next the id of the group
	//__ being seen.
	//__
	virtual bool handle_eventGroupSeen(const BASE_OBJECT *psViewer, int groupId) override;

	//__ ## eventObjectTransfer(object, from)
	//__
	//__ An event that is run whenever an object is transferred between players,
	//__ for example due to a Nexus Link weapon. The event is called after the
	//__ object has been transferred, so the target player is in object.player.
	//__ The event is called for both players.
	//__
	virtual bool handle_eventObjectTransfer(const BASE_OBJECT *psObj, int from) override;

	//__ ## eventChat(from, to, message)
	//__
	//__ An event that is run whenever a chat message is received. The ```from``` parameter is the
	//__ player sending the chat message. For the moment, the ```to``` parameter is always the script
	//__ player.
	//__
	virtual bool handle_eventChat(int from, int to, const char *message) override;

	//__ ## eventQuickChat(from, to, messageEnum)
	//__
	//__ An event that is run whenever a quick chat message is received. The ```from``` parameter is the
	//__ player sending the chat message. For the moment, the ```to``` parameter is always the script
	//__ player. ```messageEnum``` is the WzQuickChatMessage value (see the WzQuickChatMessages global
	//__ object for constants to match with it). The ```teamSpecific``` parameter is true if this message
	//__ was sent only to teammates, false otherwise.
	//__
	virtual bool handle_eventQuickChat(int from, int to, int messageEnum, bool teamSpecific) override;

	//__ ## eventBeacon(x, y, from, to[, message])
	//__
	//__ An event that is run whenever a beacon message is received. The ```from``` parameter is the
	//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
	//__ Message may be undefined.
	//__
	virtual bool handle_eventBeacon(int x, int y, int from, int to, optional<const char *> message) override;

	//__ ## eventBeaconRemoved(from, to)
	//__
	//__ An event that is run whenever a beacon message is removed. The ```from``` parameter is the
	//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
	//__
	virtual bool handle_eventBeaconRemoved(int from, int to) override;

	//__ ## eventGroupLoss(object, groupId, newSize)
	//__
	//__ An event that is run whenever a group becomes empty. Input parameter
	//__ is the about to be killed object, the group's id, and the new group size.
	//__
//		// Since groups are entities local to one context, we do not iterate over them here.
	virtual bool handle_eventGroupLoss(const BASE_OBJECT *psObj, int group, int size) override;

	//__ ## eventArea<label>(droid)
	//__
	//__ An event that is run whenever a droid enters an area label. The area is then
	//__ deactived. Call resetArea() to reactivate it. The name of the event is
	//__ `eventArea${label}`.
	//__
	virtual bool handle_eventArea(const std::string& label, const DROID *psDroid) override;

	//__ ## eventDesignCreated(template)
	//__
	//__ An event that is run whenever a new droid template is created. It is only
	//__ run on the client of the player designing the template.
	//__
	virtual bool handle_eventDesignCreated(const DROID_TEMPLATE *psTemplate) override;

	//__ ## eventAllianceOffer(from, to)
	//__
	//__ An event that is called whenever an alliance offer is requested.
	//__
	virtual bool handle_eventAllianceOffer(uint8_t from, uint8_t to) override;

	//__ ## eventAllianceAccepted(from, to)
	//__
	//__ An event that is called whenever an alliance is accepted.
	//__
	virtual bool handle_eventAllianceAccepted(uint8_t from, uint8_t to) override;

	//__ ## eventAllianceBroken(from, to)
	//__
	//__ An event that is called whenever an alliance is broken.
	//__
	virtual bool handle_eventAllianceBroken(uint8_t from, uint8_t to) override;

public:
	// MARK: Special input events

	//__ ## eventSyncRequest(req_id, x, y, obj_id, obj_id2)
	//__
	//__ An event that is called from a script and synchronized with all other scripts and hosts
	//__ to prevent desync from happening. Sync requests must be carefully validated to prevent
	//__ cheating!
	//__
	virtual bool handle_eventSyncRequest(int from, int req_id, int x, int y, const BASE_OBJECT *psObj, const BASE_OBJECT *psObj2) override;
};

// private QuickJS bureaucracy

/// Assert for scripts that give useful backtraces and other info.
#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
			JS_ThrowReferenceError(context, "%s failed in %s at line %d", #expr, __FUNCTION__, __LINE__); \
			return JS_NULL; } } while (0)


// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

JSValue mapJsonToQuickJSValue(JSContext *ctx, const nlohmann::json &instance, uint8_t prop_flags); // forward-declare

static JSValue mapJsonObjectToQuickJSValue(JSContext *ctx, const nlohmann::json &obj, uint8_t prop_flags)
{
	JSValue value = JS_NewObject(ctx);
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		JSValue prop_val = mapJsonToQuickJSValue(ctx, it.value(), prop_flags);
		JS_DefinePropertyValueStr(ctx, value, it.key().c_str(), prop_val, prop_flags);
	}
	return value;
}

static JSValue mapJsonArrayToQuickJSValue(JSContext *ctx, const nlohmann::json &array, uint8_t prop_flags)
{
	JSValue value = JS_NewArray(ctx);
	for (int i = 0; i < array.size(); i++)
	{
		JS_DefinePropertyValueUint32(ctx, value, i, mapJsonToQuickJSValue(ctx, array.at(i), prop_flags), prop_flags);
	}
	return value;
}

JSValue mapJsonToQuickJSValue(JSContext *ctx, const nlohmann::json &instance, uint8_t prop_flags)
{
	switch (instance.type())
	{
		// IMPORTANT: To match the prior behavior of loading a QVariant from the JSON value and using engine->toScriptValue(QVariant)
		//			  to convert to a QScriptValue, "null" JSON values *MUST* map to QScriptValue::UndefinedValue.
		//
		//			  If they are set to QScriptValue::NullValue, it causes issues for libcampaign.js. (As the values become "defined".)
		//
		case json::value_t::null : return JS_UNDEFINED;
		case json::value_t::boolean : return JS_NewBool(ctx, instance.get<bool>());
		case json::value_t::number_integer: return JS_NewInt32(ctx, instance.get<int32_t>());
		case json::value_t::number_unsigned: return JS_NewUint32(ctx, instance.get<uint32_t>());
		case json::value_t::number_float: return JS_NewFloat64(ctx, instance.get<double>());
		case json::value_t::string	: return JS_NewString(ctx, instance.get<WzString>().toUtf8().c_str());
		case json::value_t::array : return mapJsonArrayToQuickJSValue(ctx, instance, prop_flags);
		case json::value_t::object : return mapJsonObjectToQuickJSValue(ctx, instance, prop_flags);
		case json::value_t::binary :
			debug(LOG_ERROR, "Unexpected binary value type");
			return JS_UNDEFINED;
		case json::value_t::discarded : return JS_UNDEFINED;
	}
	return JS_UNDEFINED; // should never be reached
}

int JS_DeletePropertyStr(JSContext *ctx, JSValueConst this_obj,
                          const char *prop)
{
    JSAtom atom = JS_NewAtom(ctx, prop);
    int ret = JS_DeleteProperty(ctx, this_obj, atom, 0);
    JS_FreeAtom(ctx, atom);
    return ret;
}

// Forward-declare
JSValue convDroid(const DROID *psDroid, JSContext *ctx);
JSValue convStructure(const STRUCTURE *psStruct, JSContext *ctx);
JSValue convObj(const BASE_OBJECT *psObj, JSContext *ctx);
JSValue convFeature(const FEATURE *psFeature, JSContext *ctx);
JSValue convMax(const BASE_OBJECT *psObj, JSContext *ctx);
JSValue convTemplate(const DROID_TEMPLATE *psTemplate, JSContext *ctx);
JSValue convResearch(const RESEARCH *psResearch, JSContext *ctx, int player);

static int QuickJS_DefinePropertyValue(JSContext *ctx, JSValueConst this_obj, const char* prop, JSValue val, int flags)
{
	JSAtom prop_name = JS_NewAtom(ctx, prop);
	int ret = JS_DefinePropertyValue(ctx, this_obj, prop_name, val, flags);
	JS_FreeAtom(ctx, prop_name);
	return ret;
}

//;; ## Research
//;;
//;; Describes a research item. The following properties are defined:
//;;
//;; * ```power``` Number of power points needed for starting the research.
//;; * ```points``` Number of research points needed to complete the research.
//;; * ```started``` A boolean saying whether or not this research has been started by current player or any of its allies.
//;; * ```done``` A boolean saying whether or not this research has been completed.
//;; * ```name``` A string containing the full name of the research.
//;; * ```id``` A string containing the index name of the research.
//;; * ```type``` The type will always be ```RESEARCH_DATA```.
//;; * ```results``` An array of objects of research upgrades (defined in "research.json").
//;;
JSValue convResearch(const RESEARCH *psResearch, JSContext *ctx, int player)
{
	if (psResearch == nullptr)
	{
		return JS_NULL;
	}
	JSValue value = JS_NewObject(ctx);
	QuickJS_DefinePropertyValue(ctx, value, "power", JS_NewInt32(ctx, (int)psResearch->researchPower), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "points", JS_NewInt32(ctx, (int)psResearch->researchPoints), JS_PROP_ENUMERABLE);
	bool started = false;
	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (aiCheckAlliances(player, i) || player == i)
		{
			int bits = asPlayerResList[i][psResearch->index].ResearchStatus;
			started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING) || (bits & RESBITS_PENDING_ONLY);
		}
	}
	QuickJS_DefinePropertyValue(ctx, value, "started", JS_NewBool(ctx, started), JS_PROP_ENUMERABLE); // including whether an ally has started it
	QuickJS_DefinePropertyValue(ctx, value, "done", JS_NewBool(ctx, IsResearchCompleted(&asPlayerResList[player][psResearch->index])), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "fullname", JS_NewString(ctx, psResearch->name.toUtf8().c_str()), JS_PROP_ENUMERABLE); // temporary
	QuickJS_DefinePropertyValue(ctx, value, "name", JS_NewString(ctx, psResearch->id.toUtf8().c_str()), JS_PROP_ENUMERABLE); // will be changed to contain fullname
	QuickJS_DefinePropertyValue(ctx, value, "id", JS_NewString(ctx, psResearch->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "type", JS_NewInt32(ctx, SCRIPT_RESEARCH), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "results", mapJsonToQuickJSValue(ctx, psResearch->results, JS_PROP_ENUMERABLE), JS_PROP_ENUMERABLE);
	return value;
}

//;; ## Structure
//;;
//;; Describes a structure (building). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;;
//;; * ```status``` The completeness status of the structure. It will be one of ```BEING_BUILT``` and ```BUILT```.
//;; * ```type``` The type will always be ```STRUCTURE```.
//;; * ```cost``` What it would cost to build this structure. (3.2+ only)
//;; * ```direction``` The direction the structure is facing. (4.5+ only)
//;; * ```stattype``` The stattype defines the type of structure. It will be one of ```HQ```, ```FACTORY```, ```POWER_GEN```,
//;; ```RESOURCE_EXTRACTOR```, ```LASSAT```, ```DEFENSE```, ```WALL```, ```RESEARCH_LAB```, ```REPAIR_FACILITY```,
//;; ```CYBORG_FACTORY```, ```VTOL_FACTORY```, ```REARM_PAD```, ```SAT_UPLINK```, ```GATE```, ```STRUCT_GENERIC```, and ```COMMAND_CONTROL```.
//;; * ```modules``` If the stattype is set to one of the factories, ```POWER_GEN``` or ```RESEARCH_LAB```, then this property is set to the
//;; number of module upgrades it has.
//;; * ```canHitAir``` True if the structure has anti-air capabilities. (3.2+ only)
//;; * ```canHitGround``` True if the structure has anti-ground capabilities. (3.2+ only)
//;; * ```isSensor``` True if the structure has sensor ability. (3.2+ only)
//;; * ```isCB``` True if the structure has counter-battery ability. (3.2+ only)
//;; * ```isRadarDetector``` True if the structure has radar detector ability. (3.2+ only)
//;; * ```range``` Maximum range of its weapons. (3.2+ only)
//;; * ```hasIndirect``` One or more of the structure's weapons are indirect. (3.2+ only)
//;;
JSValue convStructure(const STRUCTURE *psStruct, JSContext *ctx)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	for (int i = 0; i < psStruct->numWeaps; i++)
	{
		if (psStruct->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = psStruct->getWeaponStats(i);
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(*psWeap, psStruct->player), range);
		}
	}
	JSValue value = convObj(psStruct, ctx);
	QuickJS_DefinePropertyValue(ctx, value, "isCB", JS_NewBool(ctx, structCBSensor(psStruct)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "isSensor", JS_NewBool(ctx, structStandardSensor(psStruct)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "canHitAir", JS_NewBool(ctx, aa), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "canHitGround", JS_NewBool(ctx, ga), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "hasIndirect", JS_NewBool(ctx, indirect), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "isRadarDetector", JS_NewBool(ctx, objRadarDetector(psStruct)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "range", JS_NewInt32(ctx, range), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "status", JS_NewInt32(ctx, (int)psStruct->status), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "health", JS_NewInt32(ctx, 100 * psStruct->body / MAX(1, psStruct->structureBody())), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "cost", JS_NewInt32(ctx, psStruct->pStructureType->powerToBuild), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "direction", JS_NewInt32(ctx, static_cast<int32_t>(UNDEG(psStruct->rot.direction))), JS_PROP_ENUMERABLE);
	int stattype = 0;
	switch (psStruct->pStructureType->type) // don't bleed our source insanities into the scripting world
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
		stattype = (int)REF_WALL;
		break;
	case REF_FORTRESS:
	case REF_DEFENSE:
		stattype = (int)REF_DEFENSE;
		break;
	default:
		stattype = (int)psStruct->pStructureType->type;
		break;
	}
	QuickJS_DefinePropertyValue(ctx, value, "stattype", JS_NewInt32(ctx, stattype), JS_PROP_ENUMERABLE);
	if (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	    || psStruct->pStructureType->type == REF_VTOL_FACTORY
	    || psStruct->pStructureType->type == REF_RESEARCH
	    || psStruct->pStructureType->type == REF_POWER_GEN)
	{
		QuickJS_DefinePropertyValue(ctx, value, "modules", JS_NewUint32(ctx, psStruct->capacity), JS_PROP_ENUMERABLE);
	}
	else
	{
		QuickJS_DefinePropertyValue(ctx, value, "modules", JS_NULL, JS_PROP_ENUMERABLE);
	}
	JSValue weaponlist = JS_NewArray(ctx);
	for (int j = 0; j < psStruct->numWeaps; j++)
	{
		JSValue weapon = JS_NewObject(ctx);
		const WEAPON_STATS *psStats = psStruct->getWeaponStats(j);
		QuickJS_DefinePropertyValue(ctx, weapon, "fullname", JS_NewString(ctx, psStats->name.toUtf8().c_str()), JS_PROP_ENUMERABLE);
		QuickJS_DefinePropertyValue(ctx, weapon, "name", JS_NewString(ctx, psStats->id.toUtf8().c_str()), JS_PROP_ENUMERABLE); // will be changed to contain full name
		QuickJS_DefinePropertyValue(ctx, weapon, "id", JS_NewString(ctx, psStats->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
		QuickJS_DefinePropertyValue(ctx, weapon, "lastFired", JS_NewUint32(ctx, psStruct->asWeaps[j].lastFired), JS_PROP_ENUMERABLE);
		JS_DefinePropertyValueUint32(ctx, weaponlist, j, weapon, JS_PROP_ENUMERABLE);
	}
	QuickJS_DefinePropertyValue(ctx, value, "weapons", weaponlist, JS_PROP_ENUMERABLE);
	return value;
}

//;; ## Feature
//;;
//;; Describes a feature (a **game object** not owned by any player). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;; * ```type``` It will always be ```FEATURE```.
//;; * ```stattype``` The type of feature. Defined types are ```OIL_RESOURCE```, ```OIL_DRUM``` and ```ARTIFACT```.
//;; * ```damageable``` Can this feature be damaged?
//;;
JSValue convFeature(const FEATURE *psFeature, JSContext *ctx)
{
	JSValue value = convObj(psFeature, ctx);
	const FEATURE_STATS *psStats = psFeature->psStats;
	QuickJS_DefinePropertyValue(ctx, value, "health", JS_NewUint32(ctx, 100 * psStats->body / MAX(1, psFeature->body)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "damageable", JS_NewBool(ctx, psStats->damageable), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "stattype", JS_NewInt32(ctx, psStats->subType), JS_PROP_ENUMERABLE);
	return value;
}

//;; ## Droid
//;;
//;; Describes a droid. It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;;
//;; * ```type``` It will always be ```DROID```.
//;; * ```order``` The current order of the droid. This is its plan. The following orders are defined:
//;;   * ```DORDER_ATTACK``` Order a droid to attack something.
//;;   * ```DORDER_MOVE``` Order a droid to move somewhere.
//;;   * ```DORDER_SCOUT``` Order a droid to move somewhere and stop to attack anything on the way.
//;;   * ```DORDER_BUILD``` Order a droid to build something.
//;;   * ```DORDER_HELPBUILD``` Order a droid to help build something.
//;;   * ```DORDER_LINEBUILD``` Order a droid to build something repeatedly in a line.
//;;   * ```DORDER_REPAIR``` Order a droid to repair something.
//;;   * ```DORDER_PATROL``` Order a droid to patrol.
//;;   * ```DORDER_DEMOLISH``` Order a droid to demolish something.
//;;   * ```DORDER_EMBARK``` Order a droid to embark on a transport.
//;;   * ```DORDER_DISEMBARK``` Order a transport to disembark its units at the given position.
//;;   * ```DORDER_FIRESUPPORT``` Order a droid to fire at whatever the target sensor is targeting. (3.2+ only)
//;;   * ```DORDER_COMMANDERSUPPORT``` Assign the droid to a commander. (3.2+ only)
//;;   * ```DORDER_STOP``` Order a droid to stop whatever it is doing. (3.2+ only)
//;;   * ```DORDER_RTR``` Order a droid to return for repairs. (3.2+ only)
//;;   * ```DORDER_RTB``` Order a droid to return to base. (3.2+ only)
//;;   * ```DORDER_HOLD``` Order a droid to hold its position. (3.2+ only)
//;;   * ```DORDER_REARM``` Order a VTOL droid to rearm. If given a target, will go to specified rearm pad. If not, will go to nearest rearm pad. (3.2+ only)
//;;   * ```DORDER_OBSERVE``` Order a droid to keep a target in sensor view. (3.2+ only)
//;;   * ```DORDER_RECOVER``` Order a droid to pick up something. (3.2+ only)
//;;   * ```DORDER_RECYCLE``` Order a droid to factory for recycling. (3.2+ only)
//;; * ```action``` The current action of the droid. This is how it intends to carry out its plan. The
//;; C++ code may change the action frequently as it tries to carry out its order. You never want to set
//;; the action directly, but it may be interesting to look at what it currently is.
//;; * ```droidType``` The droid's type. The following types are defined:
//;;   * ```DROID_CONSTRUCT``` Trucks and cyborg constructors.
//;;   * ```DROID_WEAPON``` Droids with weapon turrets, except cyborgs.
//;;   * ```DROID_PERSON``` Non-cyborg two-legged units, like scavengers.
//;;   * ```DROID_REPAIR``` Units with repair turret, including repair cyborgs.
//;;   * ```DROID_SENSOR``` Units with sensor turret.
//;;   * ```DROID_ECM``` Unit with ECM jammer turret.
//;;   * ```DROID_CYBORG``` Cyborgs with weapons.
//;;   * ```DROID_TRANSPORTER``` Cyborg transporter.
//;;   * ```DROID_SUPERTRANSPORTER``` Droid transporter.
//;;   * ```DROID_COMMAND``` Commanders.
//;; * ```group``` The group this droid is member of. This is a numerical ID. If not a member of any group, will be set to \emph{null}.
//;; * ```armed``` The percentage of weapon capability that is fully armed. Will be \emph{null} for droids other than VTOLs.
//;; * ```experience``` Amount of experience this droid has, based on damage it has dealt to enemies.
//;; * ```cost``` What it would cost to build the droid. (3.2+ only)
//;; * ```isVTOL``` True if the droid is VTOL. (3.2+ only)
//;; * ```isFlying``` True if the droid is currently flying. (4.5.4+ only)
//;; * ```canHitAir``` True if the droid has anti-air capabilities. (3.2+ only)
//;; * ```canHitGround``` True if the droid has anti-ground capabilities. (3.2+ only)
//;; * ```isSensor``` True if the droid has sensor ability. (3.2+ only)
//;; * ```isCB``` True if the droid has counter-battery ability. (3.2+ only)
//;; * ```isRadarDetector``` True if the droid has radar detector ability. (3.2+ only)
//;; * ```hasIndirect``` One or more of the droid's weapons are indirect. (3.2+ only)
//;; * ```range``` Maximum range of its weapons. (3.2+ only)
//;; * ```body``` The body component of the droid. (3.2+ only)
//;; * ```propulsion``` The propulsion component of the droid. (3.2+ only)
//;; * ```weapons``` The weapon components of the droid, as an array. Contains 'name', 'id', 'armed' percentage and 'lastFired' properties. (3.2+ only)
//;; * ```cargoCapacity``` Defined for transporters only: Total cargo capacity (number of items that will fit may depend on their size). (3.2+ only)
//;; * ```cargoSpace``` Defined for transporters only: Cargo capacity left. (3.2+ only)
//;; * ```cargoCount``` Defined for transporters only: Number of individual \emph{items} in the cargo hold. (3.2+ only)
//;; * ```cargoSize``` The amount of cargo space the droid will take inside a transport. (3.2+ only)
//;;
JSValue convDroid(const DROID *psDroid, JSContext *ctx)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	const BODY_STATS *psBodyStats = psDroid->getBodyStats();

	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat)
		{
			ASSERT(psDroid->asWeaps[i].nStat < asWeaponStats.size(), "Invalid nStat (%d) referenced for asWeaps[%d]; numWeaponStats (%zu); droid: \"%s\" (numWeaps: %u)", psDroid->asWeaps[i].nStat, i, asWeaponStats.size(), psDroid->aName, psDroid->numWeaps);
			WEAPON_STATS *psWeap = psDroid->getWeaponStats(i);
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(*psWeap, psDroid->player), range);
		}
	}
	DROID_TYPE type = psDroid->droidType;
	JSValue value = convObj(psDroid, ctx);
	QuickJS_DefinePropertyValue(ctx, value, "action", JS_NewInt32(ctx, (int)psDroid->action), JS_PROP_ENUMERABLE);
	if (range >= 0)
	{
		QuickJS_DefinePropertyValue(ctx, value, "range", JS_NewInt32(ctx, range), JS_PROP_ENUMERABLE);
	}
	else
	{
		QuickJS_DefinePropertyValue(ctx, value, "range", JS_NULL, JS_PROP_ENUMERABLE);
	}
	QuickJS_DefinePropertyValue(ctx, value, "order", JS_NewInt32(ctx, (int)psDroid->order.type), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "cost", JS_NewUint32(ctx, calcDroidPower(psDroid)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "hasIndirect", JS_NewBool(ctx, indirect), JS_PROP_ENUMERABLE);
	switch (psDroid->droidType) // hide some engine craziness
	{
	case DROID_CYBORG_CONSTRUCT:
		type = DROID_CONSTRUCT; break;
	case DROID_CYBORG_SUPER:
		type = DROID_CYBORG; break;
	case DROID_DEFAULT:
		type = DROID_WEAPON; break;
	case DROID_CYBORG_REPAIR:
		type = DROID_REPAIR; break;
	default:
		break;
	}
	QuickJS_DefinePropertyValue(ctx, value, "bodySize", JS_NewInt32(ctx, psBodyStats->size), JS_PROP_ENUMERABLE);
	if (psDroid->isTransporter())
	{
		QuickJS_DefinePropertyValue(ctx, value, "cargoCapacity", JS_NewInt32(ctx, TRANSPORTER_CAPACITY), JS_PROP_ENUMERABLE);
		QuickJS_DefinePropertyValue(ctx, value, "cargoLeft", JS_NewInt32(ctx, calcRemainingCapacity(psDroid)), JS_PROP_ENUMERABLE);
		QuickJS_DefinePropertyValue(ctx, value, "cargoCount", JS_NewUint32(ctx, psDroid->psGroup != nullptr? psDroid->psGroup->getNumMembers() : 0), JS_PROP_ENUMERABLE);
	}
	QuickJS_DefinePropertyValue(ctx, value, "isRadarDetector", JS_NewBool(ctx, objRadarDetector(psDroid)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "isCB", JS_NewBool(ctx, cbSensorDroid(psDroid)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "isSensor", JS_NewBool(ctx, standardSensorDroid(psDroid)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "canHitAir", JS_NewBool(ctx, aa), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "canHitGround", JS_NewBool(ctx, ga), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "isVTOL", JS_NewBool(ctx, psDroid->isVtol()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "isFlying", JS_NewBool(ctx, psDroid->isFlying()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "droidType", JS_NewInt32(ctx, (int)type), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "experience", JS_NewFloat64(ctx, (double)psDroid->experience / 65536.0), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "health", JS_NewFloat64(ctx, 100.0 / (double)psDroid->originalBody * (double)psDroid->body), JS_PROP_ENUMERABLE);

	QuickJS_DefinePropertyValue(ctx, value, "body", JS_NewString(ctx, psDroid->getBodyStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "propulsion", JS_NewString(ctx, psDroid->getPropulsionStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "armed", JS_NewFloat64(ctx, 0.0), JS_PROP_ENUMERABLE); // deprecated!

	JSValue weaponlist = JS_NewArray(ctx);
	for (int j = 0; j < psDroid->numWeaps; j++)
	{
		int armed = droidReloadBar(psDroid, &psDroid->asWeaps[j], j);
		JSValue weapon = JS_NewObject(ctx);
		const WEAPON_STATS *psStats = psDroid->getWeaponStats(j);
		QuickJS_DefinePropertyValue(ctx, weapon, "fullname", JS_NewString(ctx, psStats->name.toUtf8().c_str()), JS_PROP_ENUMERABLE);
		QuickJS_DefinePropertyValue(ctx, weapon, "name", JS_NewString(ctx, psStats->id.toUtf8().c_str()), JS_PROP_ENUMERABLE); // will be changed to contain full name
		QuickJS_DefinePropertyValue(ctx, weapon, "id", JS_NewString(ctx, psStats->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
		QuickJS_DefinePropertyValue(ctx, weapon, "lastFired", JS_NewUint32(ctx, psDroid->asWeaps[j].lastFired), JS_PROP_ENUMERABLE);
		QuickJS_DefinePropertyValue(ctx, weapon, "armed", JS_NewInt32(ctx, armed), JS_PROP_ENUMERABLE);
		JS_DefinePropertyValueUint32(ctx, weaponlist, j, weapon, JS_PROP_ENUMERABLE);
	}
	QuickJS_DefinePropertyValue(ctx, value, "weapons", weaponlist, JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "cargoSize", JS_NewInt32(ctx, transporterSpaceRequired(psDroid)), JS_PROP_ENUMERABLE);
	return value;
}

//;; ## Base Object
//;;
//;; Describes a basic object. It will always be a droid, structure or feature, but sometimes the
//;; difference does not matter, and you can treat any of them simply as a basic object. These
//;; fields are also inherited by the droid, structure and feature objects.
//;; The following properties are defined:
//;;
//;; * ```type``` It will be one of ```DROID```, ```STRUCTURE``` or ```FEATURE```.
//;; * ```id``` The unique ID of this object.
//;; * ```x``` X position of the object in tiles.
//;; * ```y``` Y position of the object in tiles.
//;; * ```z``` Z (height) position of the object in tiles.
//;; * ```player``` The player owning this object.
//;; * ```selected``` A boolean saying whether 'selectedPlayer' has selected this object.
//;; * ```name``` A user-friendly name for this object.
//;; * ```health``` Percentage that this object is damaged (where 100 means not damaged at all).
//;; * ```armour``` Amount of armour points that protect against kinetic weapons.
//;; * ```thermal``` Amount of thermal protection that protect against heat based weapons.
//;; * ```born``` The game time at which this object was produced or came into the world. (3.2+ only)
//;;
JSValue convObj(const BASE_OBJECT *psObj, JSContext *ctx)
{
	JSValue value = JS_NewObject(ctx);
	ASSERT_OR_RETURN(value, psObj, "No object for conversion");
	QuickJS_DefinePropertyValue(ctx, value, "id", JS_NewUint32(ctx, psObj->id), 0);
	QuickJS_DefinePropertyValue(ctx, value, "x", JS_NewInt32(ctx, map_coord(psObj->pos.x)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "y", JS_NewInt32(ctx, map_coord(psObj->pos.y)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "z", JS_NewInt32(ctx, map_coord(psObj->pos.z)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "player", JS_NewUint32(ctx, psObj->player), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "armour", JS_NewInt32(ctx, objArmour(psObj, WC_KINETIC)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "thermal", JS_NewInt32(ctx, objArmour(psObj, WC_HEAT)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "type", JS_NewInt32(ctx, psObj->type), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "selected", JS_NewUint32(ctx, psObj->selected), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "name", JS_NewString(ctx, objInfo(psObj)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "born", JS_NewUint32(ctx, psObj->born), JS_PROP_ENUMERABLE);
	scripting_engine::GROUPMAP *psMap = scripting_engine::instance().getGroupMap(engineToInstanceMap.at(ctx));
	if (psMap != nullptr && psMap->map().count(psObj) > 0) // FIXME:
	{
		int group = psMap->map().at(psObj); // FIXME:
		QuickJS_DefinePropertyValue(ctx, value, "group", JS_NewInt32(ctx, group), JS_PROP_ENUMERABLE);
	}
	else
	{
		QuickJS_DefinePropertyValue(ctx, value, "group", JS_NULL, JS_PROP_ENUMERABLE);
	}
	return value;
}

//;; ## Template
//;;
//;; Describes a template type. Templates are droid designs that a player has created.
//;; The following properties are defined:
//;;
//;; * ```id``` The ID of this object.
//;; * ```name``` Name of the template.
//;; * ```cost``` The power cost of the template if put into production.
//;; * ```droidType``` The type of droid that would be created.
//;; * ```body``` The name of the body type.
//;; * ```propulsion``` The name of the propulsion type.
//;; * ```brain``` The name of the brain type.
//;; * ```repair``` The name of the repair type.
//;; * ```ecm``` The name of the ECM (electronic counter-measure) type.
//;; * ```construct``` The name of the construction type.
//;; * ```weapons``` An array of weapon names attached to this template.
JSValue convTemplate(const DROID_TEMPLATE *psTempl, JSContext *ctx)
{
	JSValue value = JS_NewObject(ctx);
	ASSERT_OR_RETURN(value, psTempl, "No object for conversion");
	QuickJS_DefinePropertyValue(ctx, value, "fullname", JS_NewString(ctx, psTempl->name.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "name", JS_NewString(ctx, psTempl->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "id", JS_NewString(ctx, psTempl->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "points", JS_NewUint32(ctx, calcTemplateBuild(psTempl)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "power", JS_NewUint32(ctx, calcTemplatePower(psTempl)), JS_PROP_ENUMERABLE); // deprecated, use cost below
	QuickJS_DefinePropertyValue(ctx, value, "cost", JS_NewUint32(ctx, calcTemplatePower(psTempl)), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "droidType", JS_NewInt32(ctx, psTempl->droidType), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "body", JS_NewString(ctx, psTempl->getBodyStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "propulsion", JS_NewString(ctx, psTempl->getPropulsionStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "brain", JS_NewString(ctx, psTempl->getBrainStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "repair", JS_NewString(ctx, psTempl->getRepairStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "ecm", JS_NewString(ctx, psTempl->getECMStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "sensor", JS_NewString(ctx, psTempl->getSensorStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	QuickJS_DefinePropertyValue(ctx, value, "construct", JS_NewString(ctx, psTempl->getConstructStats()->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	JSValue weaponlist = JS_NewArray(ctx);
	for (int j = 0; j < psTempl->numWeaps; j++)
	{
		JS_DefinePropertyValueUint32(ctx, weaponlist, j, JS_NewString(ctx, psTempl->getWeaponStats(j)->id.toUtf8().c_str()), JS_PROP_ENUMERABLE);
	}
	QuickJS_DefinePropertyValue(ctx, value, "weapons", weaponlist, JS_PROP_ENUMERABLE);
	return value;
}

JSValue convMax(const BASE_OBJECT *psObj, JSContext *ctx)
{
	if (!psObj)
	{
		return JS_NULL;
	}
	switch (psObj->type)
	{
	case OBJ_DROID: return convDroid((const DROID *)psObj, ctx);
	case OBJ_STRUCTURE: return convStructure((const STRUCTURE *)psObj, ctx);
	case OBJ_FEATURE: return convFeature((const FEATURE *)psObj, ctx);
	default: ASSERT(false, "No such supported object type: %d", static_cast<int>(psObj->type)); return convObj(psObj, ctx);
	}
}

// Call a function by name
static JSValue callFunction(JSContext *ctx, const std::string &function, std::vector<JSValue> &args, bool event = true)
{
	const auto instance = engineToInstanceMap.at(ctx);
	JSValue global_obj = instance->Get_Global_Obj();
	if (event)
	{
		// recurse into variants, if any
		for (const std::string &s : instance->eventNamespaces)
		{
			std::string funcName = s + function;
			JSValue value = JS_GetPropertyStr(ctx, global_obj, funcName.c_str());
			if (JS_IsFunction(ctx, value))
			{
				callFunction(ctx, funcName, args, event);
			}
			JS_FreeValue(ctx, value);
		}
	}
	code_part level = event ? LOG_SCRIPT : LOG_ERROR;
	JSValue value = JS_GetPropertyStr(ctx, global_obj, function.c_str());
	auto free_func_ref = gsl::finally([ctx, value] { JS_FreeValue(ctx, value); });  // establish exit action
	if (!JS_IsFunction(ctx, value))
	{
		// not necessarily an error, may just be a trigger that is not defined (ie not needed)
		// or it could be a typo in the function name or ...
		debug(level, "called function (%s) not defined", function.c_str());
		return JS_FALSE; // ?? Shouldn't this be "undefined?"
	}

	JSValue result;
	scripting_engine::instance().executeWithPerformanceMonitoring(instance, function, [ctx, &result, value, &args](){
		result = JS_Call(ctx, value, JS_UNDEFINED, (int)args.size(), args.data());
	});

	if (JS_IsException(result))
	{
		JSValue err = JS_GetException(ctx);
		bool isError = JS_IsError(ctx, err);
		std::string result_str;
		if (isError)
		{
			JSValue name = JS_GetPropertyStr(ctx, err, "name");
			const char* errorname_str = JS_ToCString(ctx, name);
			result_str = (errorname_str) ? errorname_str : "<unknown error name>";
			result_str += " - ";
			JS_FreeCString(ctx, errorname_str);
			JS_FreeValue(ctx, name);
			JSValue message = JS_GetPropertyStr(ctx, err, "message");
			const char* message_str = JS_ToCString(ctx, message);
			result_str += (message_str) ? std::string("\"") + message_str + "\"" : "<no message>";
			result_str += ":: \n";
			JS_FreeCString(ctx, message_str);
			JS_FreeValue(ctx, message);
			JSValue stack = JS_GetPropertyStr(ctx, err, "stack");
			if (!JS_IsUndefined(stack)) {
                const char* stack_str = JS_ToCString(ctx, stack);
                if (stack_str) {
					result_str += stack_str;
                    JS_FreeCString(ctx, stack_str);
                }
            }
            JS_FreeValue(ctx, stack);
		}
		JS_FreeValue(ctx, err);
		ASSERT(false, "Uncaught exception calling function \"%s\": %s",
		       function.c_str(), result_str.c_str());
		return JS_UNDEFINED;
	}
	return result;
}

// ----------------------------------------------------------------------------------------

	class quickjs_execution_context : public wzapi::execution_context
	{
	private:
		JSContext *ctx = nullptr;
	public:
		quickjs_execution_context(JSContext *ctx)
		: ctx(ctx)
		{ }
		~quickjs_execution_context() { }
	public:
		virtual wzapi::scripting_instance* currentInstance() const override
		{
			return engineToInstanceMap.at(ctx);
		}

		virtual void throwError(const char *expr, int line, const char *function) const override
		{
			JS_ThrowReferenceError(ctx, "%s failed in %s at line %d", expr, function, line);
		}

		virtual playerCallbackFunc getNamedScriptCallback(const WzString& func) const override
		{
			JSContext *pCtx = ctx;
			return [pCtx, func](const int player) {
				std::vector<JSValue> args;
				args.push_back(JS_NewInt32(pCtx, player));
				callFunction(pCtx, func.toUtf8(), args);
				std::for_each(args.begin(), args.end(), [pCtx](JSValue& val) { JS_FreeValue(pCtx, val); });
			};
		}

		virtual void doNotSaveGlobal(const std::string &name) const override
		{
			engineToInstanceMap.at(ctx)->doNotSaveGlobal(name);
		}
	};

	/// Assert for scripts that give useful backtraces and other info.
	#define UNBOX_SCRIPT_ASSERT(context, expr, ...) \
		do { bool _wzeval = (expr); \
			if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
				JS_ThrowReferenceError(ctx, "%s failed when converting argument %zu for %s", #expr, idx, function); \
				break; } } while (0)

	namespace
	{
		template<typename T>
		struct unbox {
			//T operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function) const;
		};

		template<>
		struct unbox<int>
		{
			int operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				int32_t value = -1;
				if (JS_ToInt32(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return value;
			}
		};

		template<>
		struct unbox<unsigned int>
		{
			unsigned int operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				uint32_t value = -1;
				if (JS_ToUint32(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return value;
			}
		};

		template<>
		struct unbox<bool>
		{
			bool operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				return JS_ToBool(ctx, argv[idx++]);
			}
		};



		template<>
		struct unbox<float>
		{
			float operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				double value = -1;
				if (JS_ToFloat64(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return static_cast<float>(value);
			}
		};

		template<>
		struct unbox<double>
		{
			double operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				double value = -1;
				if (JS_ToFloat64(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return value;
			}
		};

		static inline int32_t JSValueToInt32(JSContext *ctx, JSValue &value)
		{
			int intVal = -1;
			if (JS_ToInt32(ctx, &intVal, value))
			{
				// failed
				ASSERT(false, "Failed"); // TODO:
			}
			return intVal;
		}

		static int32_t QuickJS_GetInt32(JSContext *ctx, JSValueConst this_obj, const char *prop)
		{
			JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
			int32_t result = JSValueToInt32(ctx, val);
			JS_FreeValue(ctx, val);
			return result;
		}

		static inline uint32_t JSValueToUint32(JSContext *ctx, JSValue &value)
		{
			uint32_t uintVal = 0;
			if (JS_ToUint32(ctx, &uintVal, value))
			{
				// failed
				ASSERT(false, "Failed"); // TODO:
			}
			return uintVal;
		}

		static uint32_t QuickJS_GetUint32(JSContext *ctx, JSValueConst this_obj, const char *prop)
		{
			JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
			uint32_t result = JSValueToUint32(ctx, val);
			JS_FreeValue(ctx, val);
			return result;
		}

		static inline std::string JSValueToStdString(JSContext *ctx, JSValue &value)
		{
			const char* pStr = JS_ToCString(ctx, value);
			std::string result;
			if (pStr) result = pStr;
			JS_FreeCString(ctx, pStr);
			return result;
		}

		static std::string QuickJS_GetStdString(JSContext *ctx, JSValueConst this_obj, const char *prop)
		{
			JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
			std::string result = JSValueToStdString(ctx, val);
			JS_FreeValue(ctx, val);
			return result;
		}

		bool QuickJS_GetArrayLength(JSContext* ctx, JSValueConst arr, uint64_t& len)
		{
			len = 0;

			if (!JS_IsArray(ctx, arr))
				return false;

			JSValue len_val = JS_GetPropertyStr(ctx, arr, "length");
			if (JS_IsException(len_val))
				return false;

			if (JS_ToIndex(ctx, &len, len_val)) {
				JS_FreeValue(ctx, len_val);
				return false;
			}

			JS_FreeValue(ctx, len_val);
			return true;
		}

		template<>
		struct unbox<DROID*>
		{
			DROID* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				JSValue droidVal = argv[idx++];
				int id = QuickJS_GetInt32(ctx, droidVal, "id");
				int player = QuickJS_GetInt32(ctx, droidVal, "player");
				DROID *psDroid = IdToDroid(id, player);
				UNBOX_SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
				return psDroid;
			}
		};

		template<>
		struct unbox<const DROID*>
		{
			const DROID* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				return unbox<DROID*>()(idx, ctx, argc, argv, function);
			}
		};

		template<>
		struct unbox<STRUCTURE*>
		{
			STRUCTURE* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				JSValue structVal = argv[idx++];
				int id = QuickJS_GetInt32(ctx, structVal, "id");
				int player = QuickJS_GetInt32(ctx, structVal, "player");
				STRUCTURE *psStruct = IdToStruct(id, player);
				UNBOX_SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
				return psStruct;
			}
		};

		template<>
		struct unbox<const STRUCTURE*>
		{
			const STRUCTURE* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				return unbox<STRUCTURE*>()(idx, ctx, argc, argv, function);
			}
		};

		template<>
		struct unbox<BASE_OBJECT*>
		{
			BASE_OBJECT* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				JSValue objVal = argv[idx++];
				int oid = QuickJS_GetInt32(ctx, objVal, "id");
				int oplayer = QuickJS_GetInt32(ctx, objVal, "player");
				OBJECT_TYPE otype = (OBJECT_TYPE)QuickJS_GetInt32(ctx, objVal, "type");
				BASE_OBJECT* psObj = IdToObject(otype, oid, oplayer);
				UNBOX_SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
				return psObj;
			}
		};

		template<>
		struct unbox<const BASE_OBJECT*>
		{
			const BASE_OBJECT* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				return unbox<BASE_OBJECT*>()(idx, ctx, argc, argv, function);
			}
		};

		template<>
		struct unbox<std::string>
		{
			std::string operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				return JSValueToStdString(ctx, argv[idx++]);
			}
		};

		template<>
		struct unbox<wzapi::STRUCTURE_TYPE_or_statsName_string>
		{
			wzapi::STRUCTURE_TYPE_or_statsName_string operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				wzapi::STRUCTURE_TYPE_or_statsName_string result;
				if (argc <= idx)
					return result;
				JSValue val = argv[idx++];
				if (JS_IsNumber(val))
				{
					result.type = (STRUCTURE_TYPE)JSValueToInt32(ctx, val);
				}
				else
				{
					result.statsName = JSValueToStdString(ctx, val);
				}
				return result;
			}
		};

		template<typename OptionalType>
		struct unbox<optional<OptionalType>>
		{
			optional<OptionalType> operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				// TODO: Might have to add a check here for whether argv[idx] == JS_UNDEFINED? (depends on how QuickJS handles missing optional arguments of functions)
				return optional<OptionalType>(unbox<OptionalType>()(idx, ctx, argc, argv, function));
			}
		};

		template<>
		struct unbox<wzapi::reservedParam>
		{
			wzapi::reservedParam operator()(size_t& idx, JSContext *ctx, int argc, WZ_DECL_UNUSED JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				// just ignore parameter value, and increment idx
				idx++;
				return {};
			}
		};

		template<>
		struct unbox<wzapi::game_object_identifier>
		{
			wzapi::game_object_identifier operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc < idx)
					return {};
				JSValue objVal = argv[idx++];
				wzapi::game_object_identifier result;
				result.id = QuickJS_GetInt32(ctx, objVal, "id");
				result.player = QuickJS_GetInt32(ctx, objVal, "player");
				result.type = QuickJS_GetInt32(ctx, objVal, "type");
				return result;
			}
		};

		template<>
		struct unbox<wzapi::string_or_string_list>
		{
			wzapi::string_or_string_list operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				wzapi::string_or_string_list strings;

				JSValue list_or_string = argv[idx++];
				if (JS_IsArray(ctx, list_or_string))
				{
					uint64_t length = 0;
					if (QuickJS_GetArrayLength(ctx, list_or_string, length))
					{
						for (uint64_t k = 0; k < length; k++) // TODO: uint64_t isn't correct here, as we call GetUint32...
						{
							JSValue resVal = JS_GetPropertyUint32(ctx, list_or_string, k);
							strings.strings.push_back(JSValueToStdString(ctx, resVal));
							JS_FreeValue(ctx, resVal);
						}
					}
				}
				else
				{
					strings.strings.push_back(JSValueToStdString(ctx, list_or_string));
				}
				return strings;
			}
		};

		template<>
		struct unbox<wzapi::va_list_treat_as_strings>
		{
			wzapi::va_list_treat_as_strings operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				wzapi::va_list_treat_as_strings strings;
				for (; idx < argc; idx++)
				{
					const char* pStr = JS_ToCString(ctx, argv[idx]);
					if (!pStr) // (context->state() == QScriptContext::ExceptionState)
					{
						break;
					}
					strings.strings.push_back(std::string(pStr));
					JS_FreeCString(ctx, pStr);
				}
				return strings;
			}
		};

		template<typename ContainedType>
		struct unbox<wzapi::va_list<ContainedType>>
		{
			wzapi::va_list<ContainedType> operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				wzapi::va_list<ContainedType> result;
				size_t before_idx = idx;
				for (; idx < argc; )
				{
					before_idx = idx;
					result.va_list.push_back(unbox<ContainedType>()(idx, ctx, argc, argv, function));
					if (before_idx == idx)
					{
						idx++;
					}
				}
				return result;
			}
		};

		template<>
		struct unbox<scripting_engine::area_by_values_or_area_label_lookup>
		{
			scripting_engine::area_by_values_or_area_label_lookup operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};

				if (JS_IsString(argv[idx]))
				{
					return scripting_engine::area_by_values_or_area_label_lookup(JSValueToStdString(ctx, argv[idx++]));
				}
				else if ((argc - idx) >= 4)
				{
					int x1, y1, x2, y2;
					x1 = JSValueToInt32(ctx, argv[idx]);
					y1 = JSValueToInt32(ctx, argv[idx + 1]);
					x2 = JSValueToInt32(ctx, argv[idx + 2]);
					y2 = JSValueToInt32(ctx, argv[idx + 3]);
					idx += 4;
					return scripting_engine::area_by_values_or_area_label_lookup(x1, y1, x2, y2);
				}
				else
				{
					// could log an error here
				}
				return {};
			}
		};

		template<>
		struct unbox<generic_script_object>
		{
			generic_script_object operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};

				JSValue qval = argv[idx++];
				int type = QuickJS_GetInt32(ctx, qval, "type");

				JSValue triggered = JS_GetPropertyStr(ctx, qval, "triggered");
				if (JS_IsNumber(triggered))
				{
//					UNBOX_SCRIPT_ASSERT(context, type != SCRIPT_POSITION, "Cannot assign a trigger to a position");
					ASSERT(false, "Not currently handling triggered property - does anything use this?");
				}

				if (type == SCRIPT_RADIUS)
				{
					return generic_script_object::fromRadius(
						QuickJS_GetInt32(ctx, qval, "x"),
						QuickJS_GetInt32(ctx, qval, "y"),
						QuickJS_GetInt32(ctx, qval, "radius")
					);
				}
				else if (type == SCRIPT_AREA)
				{
					return generic_script_object::fromArea(
						QuickJS_GetInt32(ctx, qval, "x"),
						QuickJS_GetInt32(ctx, qval, "y"),
						QuickJS_GetInt32(ctx, qval, "x2"),
						QuickJS_GetInt32(ctx, qval, "y2")
					);
				}
				else if (type == SCRIPT_POSITION)
				{
					return generic_script_object::fromPosition(
						QuickJS_GetInt32(ctx, qval, "x"),
						QuickJS_GetInt32(ctx, qval, "y")
					);
				}
				else if (type == SCRIPT_GROUP)
				{
					return generic_script_object::fromGroup(
						QuickJS_GetInt32(ctx, qval, "id")
					);
				}
				else if (type == OBJ_DROID || type == OBJ_STRUCTURE || type == OBJ_FEATURE)
				{
					int id = QuickJS_GetInt32(ctx, qval, "id");
					int player = QuickJS_GetInt32(ctx, qval, "player");
					BASE_OBJECT *psObj = IdToObject((OBJECT_TYPE)type, id, player);
					UNBOX_SCRIPT_ASSERT(context, psObj, "Object id %d not found belonging to player %d", id, player); // TODO: fail out
					return generic_script_object::fromObject(psObj);
				}
				return {};
			}
		};

		template<>
		struct unbox<wzapi::object_request>
		{
			wzapi::object_request operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};

				if ((argc - idx) >= 3) // get by ID case (3 parameters)
				{
					OBJECT_TYPE type = (OBJECT_TYPE)JSValueToInt32(ctx, argv[idx++]);
					int player = JSValueToInt32(ctx, argv[idx++]);
					int id = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::object_request(type, player, id);
				}
				else if ((argc - idx) >= 2) // get at position case (2 parameters)
				{
					int x = JSValueToInt32(ctx, argv[idx++]);
					int y = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::object_request(x, y);
				}
				else
				{
					// get by label case (1 parameter)
					std::string label = JSValueToStdString(ctx, argv[idx++]);
					return wzapi::object_request(label);
				}
			}
		};

		template<>
		struct unbox<wzapi::label_or_position_values>
		{
			wzapi::label_or_position_values operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if ((argc - idx) >= 4) // square area
				{
					int x1 = JSValueToInt32(ctx, argv[idx++]);
					int y1 = JSValueToInt32(ctx, argv[idx++]);
					int x2 = JSValueToInt32(ctx, argv[idx++]);
					int y2 = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::label_or_position_values(x1, y1, x2, y2);
				}
				else if ((argc - idx) >= 2) // single tile
				{
					int x = JSValueToInt32(ctx, argv[idx++]);
					int y = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::label_or_position_values(x, y);
				}
				else if ((argc - idx) >= 1) // label
				{
					std::string label = JSValueToStdString(ctx, argv[idx++]);
					return wzapi::label_or_position_values(label);
				}
				return wzapi::label_or_position_values();
			}
		};

		template<typename T>
		JSValue box(T a, JSContext *);

		JSValue box(unsigned char val, JSContext* ctx)
		{
			return JS_NewUint32(ctx, val);
		}

		JSValue box(bool val, JSContext* ctx)
		{
			return JS_NewBool(ctx, val);
		}

		JSValue box(int val, JSContext* ctx)
		{
			return JS_NewInt32(ctx, val);
		}

		JSValue box(unsigned int val, JSContext* ctx)
		{
			return JS_NewUint32(ctx, val);
		}

//		JSValue box(double val, JSContext* ctx)
//		{
//			return JS_NewFloat64(ctx, val);
//		}

		JSValue box(const char* str, JSContext* ctx)
		{
			return JS_NewString(ctx, str);
		}

		JSValue box(std::string str, JSContext* ctx)
		{
			return JS_NewStringLen(ctx, str.c_str(), str.length());
		}

		JSValue box(wzapi::no_return_value, JSContext* ctx)
		{
			return JS_UNDEFINED;
		}

		JSValue box(const BASE_OBJECT * psObj, JSContext* ctx)
		{
			if (!psObj)
			{
				return JS_NULL;
			}
			return convMax(psObj, ctx);
		}

		JSValue box(const STRUCTURE * psStruct, JSContext* ctx)
		{
			if (!psStruct)
			{
				return JS_NULL;
			}
			return convStructure(psStruct, ctx);
		}

		JSValue box(const DROID * psDroid, JSContext* ctx)
		{
			if (!psDroid)
			{
				return JS_NULL;
			}
			return convDroid(psDroid, ctx);
		}

		JSValue box(const FEATURE * psFeat, JSContext* ctx)
		{
			if (!psFeat)
			{
				return JS_NULL;
			}
			return convFeature(psFeat, ctx);
		}

		JSValue box(const DROID_TEMPLATE * psTemplate, JSContext* ctx)
		{
			if (!psTemplate)
			{
				return JS_NULL;
			}
			return convTemplate(psTemplate, ctx);
		}

//		// deliberately not defined
//		// use `wzapi::researchResult` instead of `const RESEARCH *`
//		template<>
//		JSValue box(const RESEARCH * psResearch, JSContext* ctx);

		JSValue box(const scr_radius& r, JSContext* ctx)
		{
			JSValue ret = JS_NewObject(ctx);
			QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, SCRIPT_RADIUS), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "x", JS_NewInt32(ctx, r.x), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "y", JS_NewInt32(ctx, r.y), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "radius", JS_NewInt32(ctx, r.radius), JS_PROP_C_W_E);
			return ret;
		}

		JSValue box(const scr_area& r, JSContext* ctx)
		{
			JSValue ret = JS_NewObject(ctx);
			QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, SCRIPT_AREA), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "x", JS_NewInt32(ctx, r.x1), JS_PROP_C_W_E); // TODO: Rename scr_area x1 to x
			QuickJS_DefinePropertyValue(ctx, ret, "y", JS_NewInt32(ctx, r.y1), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "x2", JS_NewInt32(ctx, r.x2), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "y2", JS_NewInt32(ctx, r.y2), JS_PROP_C_W_E);
			return ret;
		}

		JSValue box(const scr_position& p, JSContext* ctx)
		{
			JSValue ret = JS_NewObject(ctx);
			QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, SCRIPT_POSITION), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "x", JS_NewInt32(ctx, p.x), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "y", JS_NewInt32(ctx, p.y), JS_PROP_C_W_E);
			return ret;
		}

		JSValue box(const generic_script_object& p, JSContext* ctx)
		{
			int type = p.getType();
			switch (type)
			{
			case SCRIPT_RADIUS:
				return box(p.getRadius(), ctx);
				break;
			case SCRIPT_AREA:
				return box(p.getArea(), ctx);
				break;
			case SCRIPT_POSITION:
				return box(p.getPosition(), ctx);
				break;
			case SCRIPT_GROUP:
			{
				JSValue ret = JS_NewObject(ctx);
				QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, type), JS_PROP_C_W_E);
				QuickJS_DefinePropertyValue(ctx, ret, "id", JS_NewInt32(ctx, p.getGroupId()), JS_PROP_C_W_E);
				return ret;
			}
				break;
			case OBJ_DROID:
			case OBJ_FEATURE:
			case OBJ_STRUCTURE:
			{
				BASE_OBJECT* psObj = p.getObject();
				return convMax(psObj, ctx);
			}
				break;
			default:
				if (p.isNull())
				{
					return JS_NULL;
				}
				debug(LOG_SCRIPT, "Unsupported object label type: %d", type);
				break;
			}

			return JS_NULL;
		}

		JSValue box(GATEWAY* psGateway, JSContext* ctx)
		{
			JSValue ret = JS_NewObject(ctx);
			QuickJS_DefinePropertyValue(ctx, ret, "x1", JS_NewInt32(ctx, psGateway->x1), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "y1", JS_NewInt32(ctx, psGateway->y1), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "x2", JS_NewInt32(ctx, psGateway->x2), JS_PROP_C_W_E);
			QuickJS_DefinePropertyValue(ctx, ret, "y2", JS_NewInt32(ctx, psGateway->y2), JS_PROP_C_W_E);
			return ret;
		}

		template<typename VectorType>
		JSValue box(const std::vector<VectorType>& value, JSContext* ctx)
		{
			JSValue result = JS_NewArray(ctx);
			for (uint32_t i = 0; i < value.size(); i++)
			{
				int ret = JS_DefinePropertyValueUint32(ctx, result, i, box(value.at(i), ctx), JS_PROP_C_W_E);
				if (ret != 1)
				{
					// Failed to define property value??
					debug(LOG_ERROR, "Failed to define property value vector[%" PRIu32 "]", i);
				}
			}
			return result;
		}

		template<typename ContainedType>
		JSValue box(const std::list<ContainedType>& value, JSContext* ctx)
		{
			JSValue result = JS_NewArray(ctx);
			uint32_t i = 0;
			for (auto item : value)
			{
				int ret = JS_DefinePropertyValueUint32(ctx, result, i, box(item, ctx), JS_PROP_C_W_E);
				if (ret != 1)
				{
					// Failed to define property value??
					debug(LOG_ERROR, "Failed to define property value list[%" PRIu32 "]", i);
				}
				i++;
			}
			return result;
		}

		JSValue box(const wzapi::researchResult& result, JSContext* ctx)
		{
			if (result.psResearch == nullptr)
			{
				return JS_NULL;
			}
			return convResearch(result.psResearch, ctx, result.player);
		}

		JSValue box(const wzapi::researchResults& results, JSContext* ctx)
		{
			JSValue result = JS_NewArray(ctx);
			for (uint32_t i = 0; i < results.resList.size(); i++)
			{
				const RESEARCH *psResearch = results.resList.at(i);
				JS_DefinePropertyValueUint32(ctx, result, i, convResearch(psResearch, ctx, results.player), JS_PROP_C_W_E); // TODO: Check return value?
			}
			return result;
		}

		template<typename OptionalType>
		JSValue box(const optional<OptionalType>& result, JSContext* ctx)
		{
			if (result.has_value())
			{
				return box(result.value(), ctx);
			}
			else
			{
				return JS_UNDEFINED;
			}
		}

		template<typename PtrType>
		JSValue box(wzapi::event_nullable_ptr<PtrType> result, JSContext* ctx)
		{
			if (result)
			{
				return box<PtrType *>(result, ctx);
			}
			else
			{
				return JS_NULL;
			}
		}

		template<typename PtrType>
		JSValue box(wzapi::returned_nullable_ptr<PtrType> result, JSContext* ctx)
		{
			if (result)
			{
				return box<PtrType *>(result, ctx);
			}
			else
			{
				return JS_NULL;
			}
		}

		template<typename PtrType>
		JSValue box(std::unique_ptr<PtrType> result, JSContext* ctx)
		{
			if (result)
			{
				return box(result.get(), ctx);
			}
			else
			{
				return JS_NULL;
			}
		}

		JSValue box(nlohmann::json results, JSContext* ctx)
		{
			return mapJsonToQuickJSValue(ctx, results, JS_PROP_C_W_E);
		}

		#include <cstddef>
		#include <tuple>
		#include <type_traits>
		#include <utility>

		template<size_t N>
		struct Apply {
			template<typename F, typename T, typename... A>
			static inline auto apply(F && f, T && t, A &&... a)
				-> decltype(Apply<N-1>::apply(
					::std::forward<F>(f), ::std::forward<T>(t),
					::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
				))
			{
				return Apply<N-1>::apply(::std::forward<F>(f), ::std::forward<T>(t),
					::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
				);
			}
		};

		template<>
		struct Apply<0> {
			template<typename F, typename T, typename... A>
			static inline auto apply(F && f, T &&, A &&... a)
				-> decltype(::std::forward<F>(f)(::std::forward<A>(a)...))
			{
				return ::std::forward<F>(f)(::std::forward<A>(a)...);
			}
		};

		template<typename F, typename T>
		inline auto apply(F && f, T && t)
			-> decltype(Apply< ::std::tuple_size<
				typename ::std::decay<T>::type
			>::value>::apply(::std::forward<F>(f), ::std::forward<T>(t)))
		{
			return Apply< ::std::tuple_size<
				typename ::std::decay<T>::type
			>::value>::apply(::std::forward<F>(f), ::std::forward<T>(t));
		}

		template<typename...T> struct UnboxTupleIndex;
		template<size_t...I, typename...T>
		struct UnboxTupleIndex<std::index_sequence<I...>, T...>
		{
		public:
			typedef std::tuple<const wzapi::execution_context&, T...> tuple_type;
			typedef std::tuple<const wzapi::execution_context&, T&...> tuple_ref_type;

			UnboxTupleIndex(const wzapi::execution_context& execution_context, size_t& idx, JSContext* ctx, int argc, JSValueConst* argv, const char* function)
				: impl(idx, ctx, argc, argv, function),
				value(execution_context, (static_cast<UnboxedValueWrapper<I, T>&>(impl)).value...)
			{ }

			explicit operator tuple_type() const { return value; }
			tuple_ref_type operator()() const { return value; }

		private:
			// Index differentiates wrappers of the same UnboxedType from one another
			template<size_t Index, typename UnboxedType>
			struct UnboxedValueWrapper
			{
				UnboxedType value;
				UnboxedValueWrapper(size_t& idx, WZ_DECL_UNUSED JSContext* ctx, WZ_DECL_UNUSED int argc, WZ_DECL_UNUSED JSValueConst* argv, WZ_DECL_UNUSED const char* function)
					: value(unbox<UnboxedType>{}(idx, ctx, argc, argv, function))
				{ }
			};

			// The UnboxedImpl class derives from all of the value wrappers
			// (Base classes are guaranteed to be constructed in-order)
			struct UnboxedImpl : UnboxedValueWrapper<I, T>...
			{
				UnboxedImpl(size_t& idx, WZ_DECL_UNUSED JSContext* ctx, WZ_DECL_UNUSED int argc, WZ_DECL_UNUSED JSValueConst* argv, WZ_DECL_UNUSED const char* function)
					: UnboxedValueWrapper<I, T>(idx, ctx, argc, argv, function)...
				{}
			};

			UnboxedImpl impl;
			tuple_ref_type value;
		};

		template<typename...T> using UnboxTuple = UnboxTupleIndex<std::make_index_sequence<sizeof...(T)>, T...>;

		MSVC_PRAGMA(warning( push )) // see matching "pop" below
		MSVC_PRAGMA(warning( disable : 4189 )) // disable "warning C4189: 'idx': local variable is initialized but not referenced"

		template<typename R, typename...Args>
		JSValue wrap__(R(*f)(const wzapi::execution_context&, Args...), WZ_DECL_UNUSED const char *wrappedFunctionName, JSContext *context, WZ_DECL_UNUSED int argc, WZ_DECL_UNUSED JSValueConst *argv)
		{
			size_t idx WZ_DECL_UNUSED = 0; // unused when Args... is empty
			quickjs_execution_context execution_context(context);
			return box(apply(f, UnboxTuple<Args...>(execution_context, idx, context, argc, argv, wrappedFunctionName)()), context);
		}

		template<typename R, typename...Args>
		JSValue wrap__(R(*f)(), WZ_DECL_UNUSED const char *wrappedFunctionName, JSContext *context, WZ_DECL_UNUSED int argc, WZ_DECL_UNUSED JSValueConst *argv)
		{
			return box(f(), context);
		}

		MSVC_PRAGMA(warning( pop ))

		#define wrap_(wzapi_function, context, argc, argv) \
		wrap__(wzapi_function, #wzapi_function, context, argc, argv)

		#define JS_FUNC_IMPL_NAME(func_name) js_##func_name

		#define IMPL_JS_FUNC(func_name, wrapped_func) \
			static JSValue JS_FUNC_IMPL_NAME(func_name)(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) \
			{ \
				return wrap_(wrapped_func, ctx, argc, argv); \
			}

		#define IMPL_JS_FUNC_DEBUGMSGUPDATE(func_name, wrapped_func) \
			static JSValue JS_FUNC_IMPL_NAME(func_name)(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) \
			{ \
				JSValue retVal = wrap_(wrapped_func, ctx, argc, argv); \
				jsDebugMessageUpdate(); \
				return retVal; \
			}

		template <typename T>
		void append_value_list(std::vector<JSValue> &list, T t, JSContext *context) { list.push_back(box(std::forward<T>(t), context)); }

		template <typename... Args>
		bool wrap_event_handler__(const std::string &functionName, JSContext *context, Args&&... args)
		{
			std::vector<JSValue> args_list;
			using expander = int[];
//			WZ_DECL_UNUSED int dummy[] = { 0, ((void) append_value_list(args_list, std::forward<Args>(args), engine),0)... };
			// Left-most void to avoid `expression result unused [-Wunused-value]`
			(void)expander{ 0, ((void) append_value_list(args_list, std::forward<Args>(args), context),0)... };
			/*JSValue result =*/ callFunction(context, functionName, args_list);
			std::for_each(args_list.begin(), args_list.end(), [context](JSValue& val) { JS_FreeValue(context, val); });
			return true; //nlohmann::json(result.toVariant());
		}

		/* PP_NARG returns the number of arguments that have been passed to it.
		 * (Expand these macros as necessary. Does not support 0 arguments.)
		 */
		#define WRAP_EXPAND(x) x
        #define VARGS_COUNT_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
        #define VARGS_COUNT(...) WRAP_EXPAND(VARGS_COUNT_N(__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0))

		static_assert(VARGS_COUNT(A) == 1, "PP_NARG() failed for 1 argument");
		static_assert(VARGS_COUNT(A,B) == 2, "PP_NARG() failed for 2 arguments");
		static_assert(VARGS_COUNT(A,B,C,D,E,F,G,H,I,J) == 10, "PP_NARG() failed for 12 arguments");

		#define MAKE_PARAMS_1(t) t arg1
		#define MAKE_PARAMS_2(t1, t2) t1 arg1, t2 arg2
		#define MAKE_PARAMS_3(t1, t2, t3) t1 arg1, t2 arg2, t3 arg3
		#define MAKE_PARAMS_4(t1, t2, t3, t4) t1 arg1, t2 arg2, t3 arg3, t4 arg4
		#define MAKE_PARAMS_5(t1, t2, t3, t4, t5) t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5
		#define MAKE_PARAMS_6(t1, t2, t3, t4, t5, t6) t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, t6 arg6

		#define MAKE_ARGS_1(t) arg1
		#define MAKE_ARGS_2(t1, t2) arg1, arg2
		#define MAKE_ARGS_3(t1, t2, t3) arg1, arg2, arg3
		#define MAKE_ARGS_4(t1, t2, t3, t4) arg1, arg2, arg3, arg4
		#define MAKE_ARGS_5(t1, t2, t3, t4, t5) arg1, arg2, arg3, arg4, arg5
		#define MAKE_ARGS_6(t1, t2, t3, t4, t5, t6) arg1, arg2, arg3, arg4, arg5, arg6

		#define MAKE_PARAMS_COUNT(COUNT, ...) WRAP_EXPAND(MAKE_PARAMS_##COUNT(__VA_ARGS__))
		#define MAKE_PARAMS_WITH_COUNT(COUNT, ...) MAKE_PARAMS_COUNT(COUNT, __VA_ARGS__)
		#define MAKE_PARAMS(...) MAKE_PARAMS_WITH_COUNT(VARGS_COUNT(__VA_ARGS__), __VA_ARGS__)

		#define MAKE_ARGS_COUNT(COUNT, ...) WRAP_EXPAND(MAKE_ARGS_##COUNT(__VA_ARGS__))
		#define MAKE_ARGS_WITH_COUNT(COUNT, ...) MAKE_ARGS_COUNT(COUNT, __VA_ARGS__)
		#define MAKE_ARGS(...) MAKE_ARGS_WITH_COUNT(VARGS_COUNT(__VA_ARGS__), __VA_ARGS__)

		#define STRINGIFY_EXPAND(tok) #tok
		#define STRINGIFY(tok) STRINGIFY_EXPAND(tok)

		#define IMPL_EVENT_HANDLER(fun, ...) \
			bool quickjs_scripting_instance::handle_##fun(MAKE_PARAMS(__VA_ARGS__)) { \
				return wrap_event_handler__(STRINGIFY(fun), ctx, MAKE_ARGS(__VA_ARGS__)); \
			}

		#define IMPL_EVENT_HANDLER_NO_PARAMS(fun) \
		bool quickjs_scripting_instance::handle_##fun() { \
			return wrap_event_handler__(STRINGIFY(fun), ctx); \
		}

	}

// Wraps a QuickJS instance

//-- ## profile(functionName[, arguments])
//--
//-- Calls a function with given arguments, measures time it took to evaluate the function,
//-- and adds this time to performance monitor statistics. Transparently returns the
//-- function's return value. The function to run is the first parameter, and it
//-- _must be quoted_. (3.2+ only)
//--
static JSValue js_profile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc >= 1, "Must have at least one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Profiled functions must be quoted");
	std::string functionName = JSValueToStdString(ctx, argv[0]);
	std::vector<JSValue> args;
	for (int i = 1; i < argc; ++i)
	{
		args.push_back(argv[i]);
	}
	return callFunction(ctx, functionName, args);
}

static std::string QuickJS_DumpObject(JSContext *ctx, JSValue obj)
{
	std::string result;
    const char *str = JS_ToCString(ctx, obj);
    if (str)
	{
		result = str;
        JS_FreeCString(ctx, str);
    }
	else
	{
        result = "[failed to convert object to string]";
    }
	return result;
}

static std::string QuickJS_DumpError(JSContext *ctx)
{
	std::string result;
    JSValue exception_val = JS_GetException(ctx);
	bool isError = JS_IsError(ctx, exception_val);
	result = QuickJS_DumpObject(ctx, exception_val);
	if (isError)
	{
		JSValue stack = JS_GetPropertyStr(ctx, exception_val, "stack");
		if (!JS_IsUndefined(stack))
		{
			result += "\n" + QuickJS_DumpObject(ctx, stack);
		}
		JS_FreeValue(ctx, stack);
	}
    JS_FreeValue(ctx, exception_val);
	return result;
}

//-- ## include(filePath)
//--
//-- Includes another source code file at this point. You should generally only specify the filename,
//-- not try to specify its path, here.
//-- However, *if* you specify sub-paths / sub-folders, the path separator should **always** be forward-slash ("/").
//--
static JSValue js_include(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must specify a file to include");
	std::string filePath = JSValueToStdString(ctx, argv[0]);
	SCRIPT_ASSERT(ctx, strEndsWith(filePath, ".js"), "Include file must end in .js");
	const auto instance = engineToInstanceMap.at(ctx);
	UDWORD size;
	char *bytes = nullptr;
	std::string loadedFilePath;
	if (!instance->loadFileForInclude(filePath.c_str(), loadedFilePath, &bytes, &size, wzapi::scripting_instance::LoadFileSearchOptions::All_BackwardsCompat))
	{
		debug(LOG_ERROR, "Failed to read include file \"%s\"", filePath.c_str());
		JS_ThrowReferenceError(ctx, "Failed to read include file \"%s\"", filePath.c_str());
		return JS_FALSE;
	}
	JSValue compiledFuncObj = JS_Eval_BypassLimitedContext(ctx, bytes, size, loadedFilePath.c_str(), JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
	free(bytes);
	if (JS_IsException(compiledFuncObj))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(LOG_ERROR, "Syntax error in include file %s: %s", loadedFilePath.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, compiledFuncObj);
		compiledFuncObj = JS_UNINITIALIZED;
		return JS_FALSE;
	}
	JSValue result = JS_EvalFunction(ctx, compiledFuncObj);
	compiledFuncObj = JS_UNINITIALIZED;
	if (JS_IsException(result))
	{
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(LOG_ERROR, "Uncaught exception in include file %s: %s", loadedFilePath.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, result);
		return JS_FALSE;
    }
    JS_FreeValue(ctx, result);
	debug(LOG_SCRIPT, "Included new script file %s", loadedFilePath.c_str());
	return JS_TRUE;
}

//-- ## includeJSON(filePath)
//--
//-- Reads a JSON file and returns an object. You should generally only specify the filename,
//-- However, *if* you specify sub-paths / sub-folders, the path separator should **always** be forward-slash ("/").
//--
static JSValue js_includeJSON(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must specify a file to include");
	std::string filePath = JSValueToStdString(ctx, argv[0]);
	SCRIPT_ASSERT(ctx, strEndsWith(filePath, ".json"), "Include file must end in .json");
	const auto instance = engineToInstanceMap.at(ctx);
	UDWORD size;
	char *bytes = nullptr;
	std::string loadedFilePath;
	if (!instance->loadFileForInclude(filePath.c_str(), loadedFilePath, &bytes, &size, wzapi::scripting_instance::LoadFileSearchOptions::ScriptPath))
	{
		debug(LOG_ERROR, "Failed to read include file \"%s\"", filePath.c_str());
		JS_ThrowReferenceError(ctx, "Failed to read include file \"%s\"", filePath.c_str());
		return JS_FALSE;
	}
	JSValue r = JS_ParseJSON(ctx, bytes, size, loadedFilePath.c_str());
	free(bytes);
	debug(LOG_SCRIPT, "Included new JSON file %s", loadedFilePath.c_str());
	return r;
}

class quickjs_timer_additionaldata : public timerAdditionalData
{
public:
	std::string stringArg;
	quickjs_timer_additionaldata(const std::string& _stringArg)
	: stringArg(_stringArg)
	{ }
};

static uniqueTimerID SetQuickJSTimer(JSContext *ctx, int player, const std::string& funcName, int32_t ms, const std::string& stringArg, BASE_OBJECT *psObj, timerType type)
{
	return scripting_engine::instance().setTimer(engineToInstanceMap.at(ctx)
	  // timerFunc
	, [ctx, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
		quickjs_timer_additionaldata* pData = static_cast<quickjs_timer_additionaldata*>(additionalParams);
		std::vector<JSValue> args;
		if (baseObject != nullptr)
		{
			args.push_back(convMax(baseObject, ctx));
		}
		else if (pData && !(pData->stringArg.empty()))
		{
			args.push_back(JS_NewStringLen(ctx, pData->stringArg.c_str(), pData->stringArg.length()));
		}
		callFunction(ctx, funcName, args, true);
		std::for_each(args.begin(), args.end(), [ctx](JSValue& val) { JS_FreeValue(ctx, val); });
	}
	, player, ms, funcName, psObj, type
	// additionalParams
	, std::make_unique<quickjs_timer_additionaldata>(stringArg));
}

//-- ## setTimer(functionName, milliseconds[, object])
//--
//-- Set a function to run repeated at some given time interval. The function to run
//-- is the first parameter, and it _must be quoted_, otherwise the function will
//-- be inlined. The second parameter is the interval, in milliseconds. A third, optional
//-- parameter can be a **game object** to pass to the timer function. If the **game object**
//-- dies, the timer stops running. The minimum number of milliseconds is 100, but such
//-- fast timers are strongly discouraged as they may deteriorate the game performance.
//--
//-- ```js
//-- function conDroids()
//-- {
//--   ... do stuff ...
//-- }
//-- // call conDroids every 4 seconds
//-- setTimer("conDroids", 4000);
//-- ```
//--
static JSValue js_setTimer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc >= 2, "Must have at least two parameters");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Timer functions must be quoted");
	std::string functionName = JSValueToStdString(ctx, argv[0]);
	int32_t ms = JSValueToInt32(ctx, argv[1]);

	JSValue global_obj = JS_GetGlobalObject(ctx);
	auto free_global_obj = gsl::finally([ctx, global_obj] { JS_FreeValue(ctx, global_obj); });  // establish exit action
	int player = QuickJS_GetInt32(ctx, global_obj, "me");

	JSValue funcObj = JS_GetPropertyStr(ctx, global_obj, functionName.c_str()); // check existence
	SCRIPT_ASSERT(ctx, JS_IsFunction(ctx, funcObj), "No such function: %s", functionName.c_str());
	JS_FreeValue(ctx, funcObj);

	std::string stringArg;
	BASE_OBJECT *psObj = nullptr;
	if (argc == 3)
	{
		JSValue obj = argv[2];
		if (JS_IsString(obj))
		{
			stringArg = JSValueToStdString(ctx, obj);
		}
		else // is game object
		{
			int baseobj = QuickJS_GetInt32(ctx, obj, "id");
			OBJECT_TYPE baseobjtype = (OBJECT_TYPE)QuickJS_GetInt32(ctx, obj, "type");
			psObj = IdToObject(baseobjtype, baseobj, player);
		}
	}

	SetQuickJSTimer(ctx, player, functionName, ms, stringArg, psObj, TIMER_REPEAT);

	return JS_TRUE;
}

//-- ## removeTimer(functionName)
//--
//-- Removes an existing timer. The first parameter is the function timer to remove,
//-- and its name _must be quoted_.
//--
static JSValue js_removeTimer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must have one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Timer functions must be quoted");
	std::string functionName = JSValueToStdString(ctx, argv[0]);

	JSValue global_obj = JS_GetGlobalObject(ctx);
	auto free_global_obj = gsl::finally([ctx, global_obj] { JS_FreeValue(ctx, global_obj); });  // establish exit action
	int player = QuickJS_GetInt32(ctx, global_obj, "me");

	wzapi::scripting_instance* instance = engineToInstanceMap.at(ctx);
	std::vector<uniqueTimerID> removedTimerIDs = scripting_engine::instance().removeTimersIf(
		[instance, functionName, player](const scripting_engine::timerNode& node)
	{
		return node.instance == instance && node.timerName == functionName && node.player == player;
	});
	if (removedTimerIDs.empty())
	{
		// Friendly warning
		std::string warnName = functionName;
		debug(LOG_ERROR, "Did not find timer %s to remove", warnName.c_str());
		return JS_FALSE;
	}
	return JS_TRUE;
}

//-- ## queue(functionName[, milliseconds[, object]])
//--
//-- Queues up a function to run at a later game frame. This is useful to prevent
//-- stuttering during the game, which can happen if too much script processing is
//-- done at once.  The function to run is the first parameter, and it
//-- _must be quoted_, otherwise the function will be inlined.
//-- The second parameter is the delay in milliseconds, if it is omitted or 0,
//-- the function will be run at a later frame.  A third optional
//-- parameter can be a **game object** to pass to the queued function. If the **game object**
//-- dies before the queued call runs, nothing happens.
//--
// TODO, check if an identical call is already queued up - and in this case,
// do not add anything.
static JSValue js_queue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc >= 1, "Must have at least one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Queued functions must be quoted");
	std::string functionName = JSValueToStdString(ctx, argv[0]);

	JSValue global_obj = JS_GetGlobalObject(ctx);
	auto free_global_obj = gsl::finally([ctx, global_obj] { JS_FreeValue(ctx, global_obj); });  // establish exit action

	JSValue funcObj = JS_GetPropertyStr(ctx, global_obj, functionName.c_str()); // check existence
	SCRIPT_ASSERT(ctx, JS_IsFunction(ctx, funcObj), "No such function: %s", functionName.c_str());
	JS_FreeValue(ctx, funcObj);

	int32_t ms = 0;
	if (argc > 1)
	{
		ms = JSValueToInt32(ctx, argv[1]);
	}
	int player = QuickJS_GetInt32(ctx, global_obj, "me");

	std::string stringArg;
	BASE_OBJECT *psObj = nullptr;
	if (argc == 3)
	{
		JSValue obj = argv[2];
		if (JS_IsString(obj))
		{
			stringArg = JSValueToStdString(ctx, obj);
		}
		else // is game object
		{
			int baseobj = QuickJS_GetInt32(ctx, obj, "id");
			OBJECT_TYPE baseobjtype = (OBJECT_TYPE)QuickJS_GetInt32(ctx, obj, "type");
			psObj = IdToObject(baseobjtype, baseobj, player);
		}
	}

	SetQuickJSTimer(ctx, player, functionName, ms, stringArg, psObj, TIMER_ONESHOT_READY);

	return JS_TRUE;
}

//-- ## namespace(prefix)
//--
//-- Registers a new event namespace. All events can now have this prefix. This is useful for
//-- code libraries, to implement event that do not conflict with events in main code. This
//-- function should be called from global; do not (for hopefully obvious reasons) put it
//-- inside an event.
//--
static JSValue js_namespace(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must have one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Must provide a string namespace prefix");
	std::string prefix = JSValueToStdString(ctx, argv[0]);
	auto instance = engineToInstanceMap.at(ctx);
	instance->eventNamespaces.push_back(prefix);
	return JS_TRUE;
}

static JSValue debugGetCallerFuncObject(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	return js_debugger_get_caller_funcObject(ctx);
}

//-- ## debugGetCallerFuncName()
//--
//-- Returns the function name of the caller of the current context as a string (if available).
//-- ex.
//-- ```js
//-- function funcA() {
//--   const callerFuncName = debugGetCallerFuncName();
//--   debug(callerFuncName);
//-- }
//-- function funcB() {
//--   funcA();
//-- }
//-- funcB();
//-- ```
//-- Will output: "funcB"
//-- Useful for debug logging.
//--
static JSValue debugGetCallerFuncName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	return js_debugger_get_caller_name(ctx);
}

static JSValue debugGetBacktrace(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	return js_debugger_build_backtrace(ctx, nullptr);
}

wzapi::scripting_instance* createQuickJSScriptInstance(const WzString& path, int player, int difficulty)
{
	WzPathInfo basename = WzPathInfo::fromPlatformIndependentPath(path.toUtf8());
	quickjs_scripting_instance* pNewInstance = new quickjs_scripting_instance(player, basename.baseName(), basename.path());
	if (!pNewInstance->loadScript(path, player, difficulty))
	{
		delete pNewInstance;
		return nullptr;
	}
	return pNewInstance;
}

bool QuickJS_EnumerateObjectProperties(JSContext *ctx, JSValue obj, const std::function<void (const char *key, JSAtom& atom)>& func, bool enumerableOnly /*= true*/)
{
	ASSERT_OR_RETURN(false, JS_IsObject(obj), "obj is not a JS object");
	JSPropertyEnum * properties = nullptr;
    uint32_t count = 0;
	int32_t flags = (JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK);
	if (enumerableOnly)
	{
		flags |= JS_GPN_ENUM_ONLY;
	}
	if (JS_GetOwnPropertyNames(ctx, &properties, &count, obj, flags) != 0)
	{
		// JS_GetOwnPropertyNames failed?
		debug(LOG_ERROR, "JS_GetOwnPropertyNames failed");
		return false;
	}

	for(uint32_t i = 0; i < count; i++)
    {
        JSAtom atom = properties[i].atom;

        const char *key = JS_AtomToCString(ctx, atom);

		if (key)
		{
			func(key, atom);
			JS_FreeCString(ctx, key);
		}
		else
		{
			debug(LOG_INFO, "JS_AtomToCString returned null?");
		}
    }
	for (int i = 0; i < count; i++)
	{
		JS_FreeAtom(ctx, properties[i].atom);
	}
	js_free(ctx, properties);
	return true;
}

static const JSCFunctionListEntry js_builtin_funcs[] = {
	QJS_CFUNC_DEF("setTimer", 2, js_setTimer ), // JS-specific implementation
	QJS_CFUNC_DEF("queue", 1, js_queue ), // JS-specific implementation
	QJS_CFUNC_DEF("removeTimer", 1, js_removeTimer ), // JS-specific implementation
	QJS_CFUNC_DEF("profile", 1, js_profile ), // JS-specific implementation
	QJS_CFUNC_DEF("include", 1, js_include ), // backend-specific (a scripting_instance can't directly include a different type of script)
	QJS_CFUNC_DEF("includeJSON", 1, js_includeJSON ), // JS-specific JSON loading
	QJS_CFUNC_DEF("namespace", 1, js_namespace ), // JS-specific implementation
	QJS_CFUNC_DEF("debugGetCallerFuncObject", 0, debugGetCallerFuncObject ), // backend-specific
	QJS_CFUNC_DEF("debugGetCallerFuncName", 0, debugGetCallerFuncName ), // backend-specific
	QJS_CFUNC_DEF("debugGetBacktrace", 0, debugGetBacktrace ) // backend-specific
};

bool quickjs_scripting_instance::loadScript(const WzString& path, int player, int difficulty)
{
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().c_str(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toUtf8().c_str());
		return false;
	}
	if (!isHostAI())
	{
		calcDataHash(reinterpret_cast<const uint8_t *>(bytes), size, DATA_SCRIPT);
	}
	m_path = path.toUtf8();
	compiledScriptObj = JS_Eval_BypassLimitedContext(ctx, bytes, size, path.toUtf8().c_str(), JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
	free(bytes);
	if (JS_IsException(compiledScriptObj))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(LOG_ERROR, "Syntax / compilation error in %s: %s",
			  path.toUtf8().c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, compiledScriptObj);
		compiledScriptObj = JS_UNINITIALIZED;
		return false;
	}

	// Special functions
	JS_SetPropertyFunctionList(ctx, global_obj, js_builtin_funcs, sizeof(js_builtin_funcs) / sizeof(js_builtin_funcs[0]));

	// Regular functions
	WzPathInfo basename = WzPathInfo::fromPlatformIndependentPath(path.toUtf8());
	registerFunctions(basename.baseName());
	// Remember internal, reserved names
	std::unordered_set<std::string>& internalNamespaceRef = internalNamespace;
	QuickJS_EnumerateObjectProperties(ctx, global_obj, [&internalNamespaceRef](const char *key, JSAtom &) {
		if (!key) { return; }
		internalNamespaceRef.insert(key);
	}, false);

	return true;
}

bool quickjs_scripting_instance::readyInstanceForExecution()
{
	ASSERT_OR_RETURN(false, !JS_IsUninitialized(compiledScriptObj), "compiledScriptObj is uninitialized");
	JSValue result = JS_EvalFunction(ctx, compiledScriptObj);
	compiledScriptObj = JS_UNINITIALIZED;
	if (JS_IsException(result))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(LOG_ERROR, "Syntax / compilation error in %s: %s",
			  m_path.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, result);
		return false;
	}

	JS_FreeValue(ctx, result);
	return true;
}

bool quickjs_scripting_instance::saveScriptGlobals(nlohmann::json &result)
{
	// we save 'scriptName' and 'me' implicitly
	QuickJS_EnumerateObjectProperties(ctx, global_obj, [this, &result](const char *key, JSAtom &atom) {
        JSValue jsVal = JS_GetProperty(ctx, global_obj, atom);
		std::string nameStr = key;
		if (!JS_IsException(jsVal))
		{
			if (internalNamespace.count(nameStr) == 0 && !JS_IsFunction(ctx, jsVal)
				&& !JS_IsConstructor(ctx, jsVal)
				)//&& !it.value().equals(engine->globalObject()))
			{
				result[nameStr] = JSContextValue{ctx, jsVal, true};
			}
		}
		else
		{
			debug(LOG_INFO, "Got an exception trying to get the value of \"%s\"?", nameStr.c_str());
		}
        JS_FreeValue(ctx, jsVal);
	});
	return true;
}

bool quickjs_scripting_instance::loadScriptGlobals(const nlohmann::json &result)
{
	ASSERT_OR_RETURN(false, result.is_object(), "Can't load script globals from non-json-object");
	for (auto it : result.items())
	{
		// IMPORTANT: "null" JSON values *MUST* map to JS_UNDEFINED.
		//			  If they are set to JS_NULL, it causes issues for libcampaign.js. (As the values become "defined".)
		//			  (mapJsonToQuickJSValue handles this properly.)
		// NOTE: Properties created on the JS global object (as "global variables") should be non-configurable.
		//		 However *their* properties should probably be C_W_E.
		int ret = JS_DefinePropertyValueStr(ctx, global_obj, it.key().c_str(), mapJsonToQuickJSValue(ctx, it.value(), JS_PROP_C_W_E), JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_THROW);
		if (ret != 1)
		{
			// Failed to load script global
			if (ret == -1)
			{
				// threw exception
				debug(LOG_SCRIPT, "[script: %s] Failed to load script global: \"%s\"; %s", scriptName().c_str(), it.key().c_str(), QuickJS_DumpError(ctx).c_str());
			}
			else
			{
				// other failure
				debug(LOG_SCRIPT, "[script: %s] Failed to load script global: \"%s\"", scriptName().c_str(), it.key().c_str());
			}
		}
	}
	return true;
}

nlohmann::json quickjs_scripting_instance::saveTimerFunction(uniqueTimerID timerID, std::string timerName, const timerAdditionalData* additionalParam)
{
	nlohmann::json result = nlohmann::json::object();
	result["function"] = timerName;
	const quickjs_timer_additionaldata* pData = static_cast<const quickjs_timer_additionaldata*>(additionalParam);
	if (pData)
	{
		result["stringArg"] = pData->stringArg;
	}
	return result;
}

// recreates timer functions (and additional userdata) based on the information saved by the saveTimer() method
std::tuple<TimerFunc, std::unique_ptr<timerAdditionalData>> quickjs_scripting_instance::restoreTimerFunction(const nlohmann::json& savedTimerFuncData)
{
	std::string funcName = json_getValue(savedTimerFuncData, WzString::fromUtf8("function")).toWzString().toStdString();
	if (funcName.empty())
	{
		throw std::runtime_error("Invalid timer restore data");
	}
	std::string stringArg = json_getValue(savedTimerFuncData, WzString::fromUtf8("stringArg")).toWzString().toStdString();

	JSContext* pContext = ctx;

	return std::tuple<TimerFunc, std::unique_ptr<timerAdditionalData>>{
		// timerFunc
		[pContext, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
			quickjs_timer_additionaldata* pData = static_cast<quickjs_timer_additionaldata*>(additionalParams);
			std::vector<JSValue> args;
			if (baseObject != nullptr)
			{
				args.push_back(convMax(baseObject, pContext));
			}
			else if (pData && !(pData->stringArg.empty()))
			{
				args.push_back(JS_NewStringLen(pContext, pData->stringArg.c_str(), pData->stringArg.length()));
			}
			callFunction(pContext, funcName, args, true);
			std::for_each(args.begin(), args.end(), [pContext](JSValue& val) { JS_FreeValue(pContext, val); });
		}
		// additionalParams
		, std::make_unique<quickjs_timer_additionaldata>(stringArg)
	};
}

nlohmann::json quickjs_scripting_instance::debugGetAllScriptGlobals()
{
	nlohmann::json globals = nlohmann::json::object();

	QuickJS_EnumerateObjectProperties(ctx, global_obj, [this, &globals](const char *key, JSAtom &atom) {
        JSValue jsVal = JS_GetProperty(ctx, global_obj, atom);
		std::string nameStr = key;
		if ((internalNamespace.count(nameStr) == 0 && !JS_IsFunction(ctx, jsVal)
			/*&& !it.value().equals(engine->globalObject())*/)
			|| nameStr == "Upgrades" || nameStr == "Stats")
		{
			globals[nameStr] = JSContextValue{ctx, jsVal, false}; // uses to_json JSContextValue implementation
		}
        JS_FreeValue(ctx, jsVal);
	});
	return globals;
}

std::unordered_map<std::string, wzapi::scripting_instance::DebugSpecialStringType> quickjs_scripting_instance::debugGetScriptGlobalSpecialStringValues()
{
	std::unordered_map<std::string, wzapi::scripting_instance::DebugSpecialStringType> result;
	result["<constructor>"] = wzapi::scripting_instance::DebugSpecialStringType::TYPE_DESCRIPTION;
	return result;
}

bool quickjs_scripting_instance::debugEvaluateCommand(const std::string &text)
{
	JSValue compiledFuncObj = JS_Eval_BypassLimitedContext(ctx, text.c_str(), text.length(), "<debug_evaluate_command>", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
	if (JS_IsException(compiledFuncObj))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(LOG_ERROR, "Syntax error in %s: %s",
			  text.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, compiledFuncObj);
		compiledFuncObj = JS_UNINITIALIZED;
		return false;
	}
	JSValue result = JS_EvalFunction(ctx, compiledFuncObj);
	compiledFuncObj = JS_UNINITIALIZED;
	if (JS_IsException(result))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(LOG_ERROR, "Uncaught exception in %s: %s",
			  text.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, result);
		return false;
	}
	std::string resultStr = JSValueToStdString(ctx, result);
	console("%s", resultStr.c_str());
	JS_FreeValue(ctx, result);
	return true;
}

void quickjs_scripting_instance::updateGameTime(uint32_t newGameTime)
{
	int ret = JS_DefinePropertyValueStr(ctx, global_obj, "gameTime", JS_NewUint32(ctx, newGameTime), JS_PROP_WRITABLE | JS_PROP_ENUMERABLE);
	ASSERT(ret >= 1, "Failed to update gameTime");
}

void quickjs_scripting_instance::updateGroupSizes(int groupId, int size)
{
	JSValue groupMembersObj = JS_GetPropertyStr(ctx, global_obj, "groupSizes");
	JS_DefinePropertyValueStr(ctx, groupMembersObj, std::to_string(groupId).c_str(), JS_NewInt32(ctx, size), JS_PROP_C_W_E);
	JS_FreeValue(ctx, groupMembersObj);
}

void quickjs_scripting_instance::setSpecifiedGlobalVariables(const nlohmann::json& variables, wzapi::GlobalVariableFlags flags /*= wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave*/)
{
	if (!variables.is_object())
	{
		ASSERT(false, "setSpecifiedGlobalVariables expects a JSON object");
		return;
	}
	int propertyFlags = toQuickJSPropertyFlags(flags) | JS_PROP_ENUMERABLE;
	bool markGlobalAsInternal = (flags & wzapi::GlobalVariableFlags::DoNotSave) == wzapi::GlobalVariableFlags::DoNotSave;
	for (auto it : variables.items())
	{
		ASSERT(!it.key().empty(), "Empty key");
		JS_DefinePropertyValueStr(ctx, global_obj, it.key().c_str(), mapJsonToQuickJSValue(ctx, it.value(), propertyFlags), propertyFlags);
		if (markGlobalAsInternal)
		{
			internalNamespace.insert(it.key());
		}
	}
}

void quickjs_scripting_instance::setSpecifiedGlobalVariable(const std::string& name, const nlohmann::json& value, wzapi::GlobalVariableFlags flags /*= wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave*/)
{
	ASSERT(!name.empty(), "Empty key");
	int propertyFlags = toQuickJSPropertyFlags(flags) | JS_PROP_ENUMERABLE;
	bool markGlobalAsInternal = (flags & wzapi::GlobalVariableFlags::DoNotSave) == wzapi::GlobalVariableFlags::DoNotSave;
	JS_DefinePropertyValueStr(ctx, global_obj, name.c_str(), mapJsonToQuickJSValue(ctx, value, propertyFlags), propertyFlags);
	if (markGlobalAsInternal)
	{
		internalNamespace.insert(name);
	}
}

void quickjs_scripting_instance::doNotSaveGlobal(const std::string &global)
{
	internalNamespace.insert(global);
}


IMPL_EVENT_HANDLER_NO_PARAMS(eventGameInit)
IMPL_EVENT_HANDLER_NO_PARAMS(eventStartLevel)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMissionTimeout)
IMPL_EVENT_HANDLER_NO_PARAMS(eventVideoDone)
IMPL_EVENT_HANDLER_NO_PARAMS(eventGameLoaded)
IMPL_EVENT_HANDLER_NO_PARAMS(eventGameSaving)
IMPL_EVENT_HANDLER_NO_PARAMS(eventGameSaved)

// MARK: Transporter events
IMPL_EVENT_HANDLER_NO_PARAMS(eventLaunchTransporter) // DEPRECATED!
IMPL_EVENT_HANDLER(eventTransporterLaunch, const BASE_OBJECT *)
IMPL_EVENT_HANDLER_NO_PARAMS(eventReinforcementsArrived) // DEPRECATED!
IMPL_EVENT_HANDLER(eventTransporterArrived, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterExit, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterDone, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterLanded, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterEmbarked, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterDisembarked, const BASE_OBJECT *)

// MARK: UI-related events (intended for the tutorial)
IMPL_EVENT_HANDLER(eventDeliveryPointMoving, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventDeliveryPointMoved, const BASE_OBJECT *)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignBody)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignPropulsion)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignWeapon)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignCommand)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignSystem)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignQuit)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuBuildSelected)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuResearchSelected)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuBuild)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuResearch)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuDesign)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuManufacture)
IMPL_EVENT_HANDLER(eventSelectionChanged, const std::vector<const BASE_OBJECT *>&)

// MARK: Game state-change events
IMPL_EVENT_HANDLER(eventObjectRecycled, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventPlayerLeft, int)
IMPL_EVENT_HANDLER(eventCheatMode, bool)
IMPL_EVENT_HANDLER(eventDroidIdle, const DROID *)
IMPL_EVENT_HANDLER(eventDroidBuilt, const DROID *, optional<const STRUCTURE *>)
IMPL_EVENT_HANDLER(eventStructureBuilt, const STRUCTURE *, optional<const DROID *>)
IMPL_EVENT_HANDLER(eventStructureDemolish, const STRUCTURE *, optional<const DROID *>)
IMPL_EVENT_HANDLER(eventStructureReady, const STRUCTURE *)
IMPL_EVENT_HANDLER(eventStructureUpgradeStarted, const STRUCTURE *)
IMPL_EVENT_HANDLER(eventDroidRankGained, const DROID *, int)
IMPL_EVENT_HANDLER(eventAttacked, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventResearched, const wzapi::researchResult&, wzapi::event_nullable_ptr<const STRUCTURE>, int)
IMPL_EVENT_HANDLER(eventDestroyed, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventPickup, const FEATURE *, const DROID *)
IMPL_EVENT_HANDLER(eventObjectSeen, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventGroupSeen, const BASE_OBJECT *, int)
IMPL_EVENT_HANDLER(eventObjectTransfer, const BASE_OBJECT *, int)
IMPL_EVENT_HANDLER(eventChat, int, int, const char *)
IMPL_EVENT_HANDLER(eventQuickChat, int, int, int, bool)
IMPL_EVENT_HANDLER(eventBeacon, int, int, int, int, optional<const char *>)
IMPL_EVENT_HANDLER(eventBeaconRemoved, int, int)
IMPL_EVENT_HANDLER(eventGroupLoss, const BASE_OBJECT *, int, int)
bool quickjs_scripting_instance::handle_eventArea(const std::string& label, const DROID *psDroid)
{
	std::vector<JSValue> args;
	args.push_back(convDroid(psDroid, ctx));
	std::string funcname = std::string("eventArea") + label;
	debug(LOG_SCRIPT, "Triggering %s for %s", funcname.c_str(), scriptName().c_str());
	callFunction(ctx, funcname, args);
	std::for_each(args.begin(), args.end(), [this](JSValue& val) { JS_FreeValue(ctx, val); });
	return true;
}
IMPL_EVENT_HANDLER(eventDesignCreated, const DROID_TEMPLATE *)
IMPL_EVENT_HANDLER(eventAllianceOffer, uint8_t, uint8_t)
IMPL_EVENT_HANDLER(eventAllianceAccepted, uint8_t, uint8_t)
IMPL_EVENT_HANDLER(eventAllianceBroken, uint8_t, uint8_t)

// MARK: Special input events
IMPL_EVENT_HANDLER(eventSyncRequest, int, int, int, int, const BASE_OBJECT *, const BASE_OBJECT *)

// ----------------------------------------------------------------------------------------
// Script functions
//
// All script functions should be prefixed with "js_" then followed by same name as in script.

IMPL_JS_FUNC(getWeaponInfo, wzapi::getWeaponInfo)
IMPL_JS_FUNC(resetLabel, scripting_engine::resetLabel)
IMPL_JS_FUNC(enumLabels, scripting_engine::enumLabels)
IMPL_JS_FUNC(addLabel, scripting_engine::addLabel)
IMPL_JS_FUNC(removeLabel, scripting_engine::removeLabel)
IMPL_JS_FUNC(getLabel, scripting_engine::getLabelJS)
IMPL_JS_FUNC(getObject, scripting_engine::getObject)

IMPL_JS_FUNC(enumBlips, wzapi::enumBlips)
IMPL_JS_FUNC(enumSelected, wzapi::enumSelected)
IMPL_JS_FUNC(enumGateways, wzapi::enumGateways)

//-- ## enumTemplates(player)
//--
//-- Return an array containing all the buildable templates for the given player. (3.2+ only)
//--
static JSValue js_enumTemplates(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must have one parameter");
	SCRIPT_ASSERT(ctx, JS_IsNumber(argv[0]), "Supplied parameter must be a player number");
	int player = JSValueToInt32(ctx, argv[0]);

	JSValue result = JS_NewArray(ctx); //engine->newArray(droidTemplates[player].size());
	uint32_t count = 0;
	enumerateTemplates(player, [ctx, &result, &count](DROID_TEMPLATE* psTemplate) {
		JS_DefinePropertyValueUint32(ctx, result, count, convTemplate(psTemplate, ctx), 0); // TODO: Check return value?
		count++;
		return true;
	});
	return result;
}

IMPL_JS_FUNC(enumGroup, scripting_engine::enumGroup)
IMPL_JS_FUNC(newGroup, scripting_engine::newGroup)
IMPL_JS_FUNC(groupAddArea, scripting_engine::groupAddArea)
IMPL_JS_FUNC(groupAddDroid, scripting_engine::groupAddDroid)
IMPL_JS_FUNC(groupAdd, scripting_engine::groupAdd)
IMPL_JS_FUNC(groupSize, scripting_engine::groupSize)

IMPL_JS_FUNC(activateStructure, wzapi::activateStructure)
IMPL_JS_FUNC(findResearch, wzapi::findResearch)
IMPL_JS_FUNC(pursueResearch, wzapi::pursueResearch)
IMPL_JS_FUNC(getResearch, wzapi::getResearch)
IMPL_JS_FUNC(enumResearch, wzapi::enumResearch)
IMPL_JS_FUNC(componentAvailable, wzapi::componentAvailable)
IMPL_JS_FUNC(addFeature, wzapi::addFeature)
IMPL_JS_FUNC(addDroid, wzapi::addDroid)
IMPL_JS_FUNC(addDroidToTransporter, wzapi::addDroidToTransporter)
IMPL_JS_FUNC(makeTemplate, wzapi::makeTemplate)
IMPL_JS_FUNC(buildDroid, wzapi::buildDroid)
IMPL_JS_FUNC(enumStruct, wzapi::enumStruct)
IMPL_JS_FUNC(enumStructOffWorld, wzapi::enumStructOffWorld)
IMPL_JS_FUNC(enumFeature, wzapi::enumFeature)
IMPL_JS_FUNC(enumCargo, wzapi::enumCargo)
IMPL_JS_FUNC(enumDroid, wzapi::enumDroid)
IMPL_JS_FUNC(dump, wzapi::dump)
IMPL_JS_FUNC(debug, wzapi::debugOutputStrings)
IMPL_JS_FUNC(pickStructLocation, wzapi::pickStructLocation)
IMPL_JS_FUNC(structureIdle, wzapi::structureIdle)
IMPL_JS_FUNC(removeStruct, wzapi::removeStruct)
IMPL_JS_FUNC(removeObject, wzapi::removeObject)
IMPL_JS_FUNC(clearConsole, wzapi::clearConsole)
IMPL_JS_FUNC(console, wzapi::console)
IMPL_JS_FUNC(distBetweenTwoPoints, wzapi::distBetweenTwoPoints)
IMPL_JS_FUNC(droidCanReach, wzapi::droidCanReach)
IMPL_JS_FUNC(propulsionCanReach, wzapi::propulsionCanReach)
IMPL_JS_FUNC(terrainType, wzapi::terrainType)
IMPL_JS_FUNC(tileIsBurning, wzapi::tileIsBurning)
IMPL_JS_FUNC(orderDroid, wzapi::orderDroid)
IMPL_JS_FUNC(orderDroidObj, wzapi::orderDroidObj)
IMPL_JS_FUNC(orderDroidBuild, wzapi::orderDroidBuild)
IMPL_JS_FUNC(orderDroidLoc, wzapi::orderDroidLoc)
IMPL_JS_FUNC(setMissionTime, wzapi::setMissionTime)
IMPL_JS_FUNC(getMissionTime, wzapi::getMissionTime)
IMPL_JS_FUNC(setTransporterExit, wzapi::setTransporterExit)
IMPL_JS_FUNC(startTransporterEntry, wzapi::startTransporterEntry)
IMPL_JS_FUNC(useSafetyTransport, wzapi::useSafetyTransport)
IMPL_JS_FUNC(restoreLimboMissionData, wzapi::restoreLimboMissionData)
IMPL_JS_FUNC(setReinforcementTime, wzapi::setReinforcementTime)
IMPL_JS_FUNC(setStructureLimits, wzapi::setStructureLimits)
IMPL_JS_FUNC(centreView, wzapi::centreView)
IMPL_JS_FUNC(hackPlayIngameAudio, wzapi::hackPlayIngameAudio)
IMPL_JS_FUNC(hackStopIngameAudio, wzapi::hackStopIngameAudio)
IMPL_JS_FUNC(playSound, wzapi::playSound)
IMPL_JS_FUNC_DEBUGMSGUPDATE(gameOverMessage, wzapi::gameOverMessage)
IMPL_JS_FUNC(completeResearch, wzapi::completeResearch)
IMPL_JS_FUNC(completeAllResearch, wzapi::completeAllResearch)
IMPL_JS_FUNC(enableResearch, wzapi::enableResearch)
IMPL_JS_FUNC(extraPowerTime, wzapi::extraPowerTime)
IMPL_JS_FUNC(setPower, wzapi::setPower)
IMPL_JS_FUNC(setPowerModifier, wzapi::setPowerModifier)
IMPL_JS_FUNC(setPowerStorageMaximum, wzapi::setPowerStorageMaximum)
IMPL_JS_FUNC(enableStructure, wzapi::enableStructure)
IMPL_JS_FUNC(setTutorialMode, wzapi::setTutorialMode)
IMPL_JS_FUNC(setMiniMap, wzapi::setMiniMap)
IMPL_JS_FUNC(setDesign, wzapi::setDesign)
IMPL_JS_FUNC(enableTemplate, wzapi::enableTemplate)
IMPL_JS_FUNC(removeTemplate, wzapi::removeTemplate)
IMPL_JS_FUNC(setReticuleButton, wzapi::setReticuleButton)
IMPL_JS_FUNC(showReticuleWidget, wzapi::showReticuleWidget)
IMPL_JS_FUNC(setReticuleFlash, wzapi::setReticuleFlash)
IMPL_JS_FUNC(showInterface, wzapi::showInterface)
IMPL_JS_FUNC(hideInterface, wzapi::hideInterface)
IMPL_JS_FUNC(addGuideTopic, wzapi::addGuideTopic)

//-- ## removeReticuleButton(buttonId)
//--
//-- Remove reticule button. DO NOT USE FOR ANYTHING.
//--
static JSValue js_removeReticuleButton(JSContext *, JSValueConst, int, JSValueConst *)
{
	return JS_UNDEFINED;
}

IMPL_JS_FUNC(applyLimitSet, wzapi::applyLimitSet)
IMPL_JS_FUNC(enableComponent, wzapi::enableComponent)
IMPL_JS_FUNC(makeComponentAvailable, wzapi::makeComponentAvailable)
IMPL_JS_FUNC(allianceExistsBetween, wzapi::allianceExistsBetween)
IMPL_JS_FUNC(translate, wzapi::translate)
IMPL_JS_FUNC(playerPower, wzapi::playerPower)
IMPL_JS_FUNC(queuedPower, wzapi::queuedPower)
IMPL_JS_FUNC(isStructureAvailable, wzapi::isStructureAvailable)
IMPL_JS_FUNC(isVTOL, wzapi::isVTOL)
IMPL_JS_FUNC(hackGetObj, wzapi::hackGetObj)
IMPL_JS_FUNC(receiveAllEvents, wzapi::receiveAllEvents)
IMPL_JS_FUNC(hackAssert, wzapi::hackAssert)
IMPL_JS_FUNC(setDroidExperience, wzapi::setDroidExperience)
IMPL_JS_FUNC(donateObject, wzapi::donateObject)
IMPL_JS_FUNC(donatePower, wzapi::donatePower)
IMPL_JS_FUNC(safeDest, wzapi::safeDest)
IMPL_JS_FUNC(addStructure, wzapi::addStructure)
IMPL_JS_FUNC(getStructureLimit, wzapi::getStructureLimit)
IMPL_JS_FUNC(countStruct, wzapi::countStruct)
IMPL_JS_FUNC(countDroid, wzapi::countDroid)
IMPL_JS_FUNC(setNoGoArea, wzapi::setNoGoArea)
IMPL_JS_FUNC(setScrollLimits, wzapi::setScrollLimits)
IMPL_JS_FUNC(getScrollLimits, wzapi::getScrollLimits)
IMPL_JS_FUNC(loadLevel, wzapi::loadLevel)
IMPL_JS_FUNC(autoSave, wzapi::autoSave)
IMPL_JS_FUNC(enumRange, wzapi::enumRange)
IMPL_JS_FUNC(enumArea, scripting_engine::enumAreaJS)
IMPL_JS_FUNC(addBeacon, wzapi::addBeacon)

//-- ## removeBeacon(playerFilter)
//--
//-- Remove a beacon message sent to target player. Target may also be ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
static JSValue js_removeBeacon(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	JSValue retVal = wrap_(wzapi::removeBeacon, ctx, argc, argv);
	if (JS_IsBool(retVal) && JS_ToBool(ctx, retVal))
	{
		jsDebugMessageUpdate();
	}
	return retVal;
}

IMPL_JS_FUNC(chat, wzapi::chat)
IMPL_JS_FUNC(quickChat, wzapi::quickChat)
IMPL_JS_FUNC(getDroidPath, wzapi::getDroidPath)
IMPL_JS_FUNC(setAlliance, wzapi::setAlliance)
IMPL_JS_FUNC(sendAllianceRequest, wzapi::sendAllianceRequest)
IMPL_JS_FUNC(setAssemblyPoint, wzapi::setAssemblyPoint)
IMPL_JS_FUNC(hackNetOff, wzapi::hackNetOff)
IMPL_JS_FUNC(hackNetOn, wzapi::hackNetOn)
IMPL_JS_FUNC(getDroidProduction, wzapi::getDroidProduction)
IMPL_JS_FUNC(getDroidLimit, wzapi::getDroidLimit)
IMPL_JS_FUNC(getExperienceModifier, wzapi::getExperienceModifier)
IMPL_JS_FUNC(setExperienceModifier, wzapi::setExperienceModifier)
IMPL_JS_FUNC(setDroidLimit, wzapi::setDroidLimit)
IMPL_JS_FUNC(setCommanderLimit, wzapi::setCommanderLimit)
IMPL_JS_FUNC(setConstructorLimit, wzapi::setConstructorLimit)
IMPL_JS_FUNC_DEBUGMSGUPDATE(hackAddMessage, wzapi::hackAddMessage)
IMPL_JS_FUNC_DEBUGMSGUPDATE(hackRemoveMessage, wzapi::hackRemoveMessage)
IMPL_JS_FUNC(setSunPosition, wzapi::setSunPosition)
IMPL_JS_FUNC(setSunIntensity, wzapi::setSunIntensity)
IMPL_JS_FUNC(setFogColour, wzapi::setFogColour)
IMPL_JS_FUNC(setWeather, wzapi::setWeather)
IMPL_JS_FUNC(setSky, wzapi::setSky)
IMPL_JS_FUNC(hackDoNotSave, wzapi::hackDoNotSave)
IMPL_JS_FUNC(hackMarkTiles, wzapi::hackMarkTiles)
IMPL_JS_FUNC(cameraSlide, wzapi::cameraSlide)
IMPL_JS_FUNC(cameraZoom, wzapi::cameraZoom)
IMPL_JS_FUNC(cameraTrack, wzapi::cameraTrack)
IMPL_JS_FUNC(setHealth, wzapi::setHealth)
IMPL_JS_FUNC(setObjectFlag, wzapi::setObjectFlag)
IMPL_JS_FUNC(addSpotter, wzapi::addSpotter)
IMPL_JS_FUNC(removeSpotter, wzapi::removeSpotter)
IMPL_JS_FUNC(syncRandom, wzapi::syncRandom)
IMPL_JS_FUNC(syncRequest, wzapi::syncRequest)
IMPL_JS_FUNC(replaceTexture, wzapi::replaceTexture)
IMPL_JS_FUNC(fireWeaponAtLoc, wzapi::fireWeaponAtLoc)
IMPL_JS_FUNC(fireWeaponAtObj, wzapi::fireWeaponAtObj)
IMPL_JS_FUNC(transformPlayerToSpectator, wzapi::transformPlayerToSpectator)
IMPL_JS_FUNC(isSpectator, wzapi::isSpectator)
IMPL_JS_FUNC(changePlayerColour, wzapi::changePlayerColour)
IMPL_JS_FUNC(getMultiTechLevel, wzapi::getMultiTechLevel)
IMPL_JS_FUNC(setCampaignNumber, wzapi::setCampaignNumber)
IMPL_JS_FUNC(getMissionType, wzapi::getMissionType)
IMPL_JS_FUNC(getRevealStatus, wzapi::getRevealStatus)
IMPL_JS_FUNC(setRevealStatus, wzapi::setRevealStatus)
IMPL_JS_FUNC(setGameStoryLogPlayerDataValue, wzapi::setGameStoryLogPlayerDataValue)

static JSValue js_stats_get(JSContext *ctx, JSValueConst this_val)
{
	JSValue currentFuncObj = js_debugger_get_current_funcObject(ctx);
	int type = QuickJS_GetInt32(ctx, currentFuncObj, "type");
	int player = QuickJS_GetInt32(ctx, currentFuncObj, "player");
	unsigned index = QuickJS_GetUint32(ctx, currentFuncObj, "index");
	std::string name = QuickJS_GetStdString(ctx, currentFuncObj, "name");
	JS_FreeValue(ctx, currentFuncObj);
	quickjs_execution_context execution_context(ctx);
	return mapJsonToQuickJSValue(ctx, wzapi::getUpgradeStats(execution_context, player, name, type, index), JS_PROP_C_W_E);
}

static JSValue js_stats_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)
{
	JSValue currentFuncObj = js_debugger_get_current_funcObject(ctx);
	int type = QuickJS_GetInt32(ctx, currentFuncObj, "type");
	int player = QuickJS_GetInt32(ctx, currentFuncObj, "player");
	unsigned index = QuickJS_GetUint32(ctx, currentFuncObj, "index");
	std::string name = QuickJS_GetStdString(ctx, currentFuncObj, "name");
	JS_FreeValue(ctx, currentFuncObj);
	quickjs_execution_context execution_context(ctx);
	wzapi::setUpgradeStats(execution_context, player, name, type, index, JSContextValue{ctx, val, true});
	// Now read value and return it
	return mapJsonToQuickJSValue(ctx, wzapi::getUpgradeStats(execution_context, player, name, type, index), JS_PROP_C_W_E);
}



static void setStatsFunc(JSValue &base, JSContext *ctx, const std::string& name, int player, int type, unsigned index)
{
	const JSCFunctionListEntry js_stats_getter_setter_func[] = {
		QJS_CGETSET_DEF(name.c_str(), js_stats_get, js_stats_set)
	};

	JSValue getter = JS_UNDEFINED;
	JSValue setter = JS_UNDEFINED;
	char buf[64];

	JSAtom atom = JS_NewAtom(ctx, name.c_str());

	snprintf(buf, sizeof(buf), "get %s", name.c_str());
	getter = JS_NewCFunction2(ctx, js_stats_getter_setter_func[0].u.getset.get.generic,
							  buf, 0, JS_CFUNC_getter,
							  0);
	snprintf(buf, sizeof(buf), "set %s", name.c_str());
	setter = JS_NewCFunction2(ctx, js_stats_getter_setter_func[0].u.getset.set.generic,
							  buf, 1, JS_CFUNC_setter,
							  0);

	JSValue funcObjects[2] = {getter, setter};
	for (size_t i = 0; i < 2; i++)
	{
		QuickJS_DefinePropertyValue(ctx, funcObjects[i], "player", JS_NewInt32(ctx, player), 0);
		QuickJS_DefinePropertyValue(ctx, funcObjects[i], "type", JS_NewInt32(ctx, type), 0);
		QuickJS_DefinePropertyValue(ctx, funcObjects[i], "index", JS_NewUint32(ctx, index), 0);
		QuickJS_DefinePropertyValue(ctx, funcObjects[i], "name", JS_NewStringLen(ctx, name.c_str(), name.length()), 0);
	}

	JS_DefinePropertyGetSet(ctx, base, atom, getter, setter, js_stats_getter_setter_func[0].prop_flags | JS_PROP_WRITABLE | JS_PROP_ENUMERABLE);

	JS_FreeAtom(ctx, atom);
}

JSValue constructUpgradesQuickJSValue(JSContext *ctx)
{
	auto upgradesObj = wzapi::getUpgradesObject();

	JSValue upgrades = JS_NewArray(ctx);
	for (auto& playerUpgrades : upgradesObj)
	{
		JSValue node = JS_NewObject(ctx);

		for (const auto& gameEntityClass : playerUpgrades)
		{
			const std::string& gameEntityClassName = gameEntityClass.first;
			JSValue entityBase = JS_NewObject(ctx);
			for (const auto& gameEntity : gameEntityClass.second)
			{
				const std::string& gameEntityName = gameEntity.first;
				const auto& gameEntityRules = gameEntity.second;
				JSValue v = JS_NewObject(ctx);
				for (const auto& property : gameEntityRules)
				{
					setStatsFunc(v, ctx, property.first, gameEntityRules.getPlayer(), property.second, gameEntityRules.getIndex());
				}
				QuickJS_DefinePropertyValue(ctx, entityBase, gameEntityName.c_str(), v, JS_PROP_ENUMERABLE);
			}
			QuickJS_DefinePropertyValue(ctx, node, gameEntityClassName.c_str(), entityBase, JS_PROP_ENUMERABLE);
		}

		// Finally
		ASSERT(playerUpgrades.getPlayer() >= 0 && playerUpgrades.getPlayer() < MAX_PLAYERS, "Invalid player index");
//		QuickJS_DefinePropertyValue(ctx, upgrades, std::to_string(playerUpgrades.getPlayer()).c_str(), node, 0);
		JS_DefinePropertyValueUint32(ctx, upgrades, static_cast<uint32_t>(playerUpgrades.getPlayer()), node, JS_PROP_ENUMERABLE);
	}

	return upgrades;
}

#define JS_REGISTER_FUNC(js_func_name, num_parameters) \
	JS_SetPropertyStr(ctx, global_obj, #js_func_name, \
		JS_NewCFunction(ctx, JS_FUNC_IMPL_NAME(js_func_name), #js_func_name, num_parameters));

#define JS_REGISTER_FUNC2(js_func_name, min_num_parameters, max_num_parameters) \
	JS_SetPropertyStr(ctx, global_obj, #js_func_name, \
		JS_NewCFunction(ctx, JS_FUNC_IMPL_NAME(js_func_name), #js_func_name, min_num_parameters));

#define JS_REGISTER_FUNC_NAME(js_func_name, num_parameters, full_impl_handler_func_name) \
	JS_SetPropertyStr(ctx, global_obj, #js_func_name, \
		JS_NewCFunction(ctx, full_impl_handler_func_name, #js_func_name, num_parameters));

#define JS_REGISTER_FUNC_NAME2(js_func_name, min_num_parameters, max_num_parameters, full_impl_handler_func_name) \
	JS_SetPropertyStr(ctx, global_obj, #js_func_name, \
		JS_NewCFunction(ctx, full_impl_handler_func_name, #js_func_name, min_num_parameters));

#define MAX_JS_VARARGS 20

bool quickjs_scripting_instance::registerFunctions(const std::string& scriptName)
{
	debug(LOG_WZ, "Loading functions for context %p, script %s", static_cast<void *>(ctx), scriptName.c_str());

	//== * ```Upgrades``` A special array containing per-player rules information for game entity types,
	//== which can be written to in order to implement upgrades and other dynamic rules changes. Each item in the
	//== array contains a subset of the sparse array of rules information in the ```Stats``` global.
	//== These values are defined:
	JSValue upgrades = constructUpgradesQuickJSValue(ctx);
	JS_DefinePropertyValueStr(ctx, global_obj, "Upgrades", upgrades, JS_PROP_WRITABLE | JS_PROP_ENUMERABLE);

	// Register functions to the script engine here
	JS_REGISTER_FUNC_NAME(_, 1, JS_FUNC_IMPL_NAME(translate)); // WZAPI
	JS_REGISTER_FUNC2(dump, 1, MAX_JS_VARARGS);
	JS_REGISTER_FUNC(syncRandom, 1); // WZAPI
	JS_REGISTER_FUNC_NAME2(label, 1, 3, JS_FUNC_IMPL_NAME(getObject)); // deprecated // scripting_engine
	JS_REGISTER_FUNC2(getObject, 1, 3); // scripting_engine
	JS_REGISTER_FUNC2(addLabel, 2, 3); // scripting_engine
	JS_REGISTER_FUNC(removeLabel, 1); // scripting_engine
	JS_REGISTER_FUNC(getLabel, 1); // scripting_engine
	JS_REGISTER_FUNC(enumLabels, 1); // scripting_engine
	JS_REGISTER_FUNC(enumGateways, 0); // WZAPI
	JS_REGISTER_FUNC(enumTemplates, 1);
	JS_REGISTER_FUNC2(makeTemplate, 6, 6 + MAX_JS_VARARGS); // WZAPI
	JS_REGISTER_FUNC(setAlliance, 3); // WZAPI
	JS_REGISTER_FUNC(sendAllianceRequest, 1); // WZAPI
	JS_REGISTER_FUNC(setAssemblyPoint, 3); // WZAPI
	JS_REGISTER_FUNC(setSunPosition, 3); // WZAPI
	JS_REGISTER_FUNC(setSunIntensity, 9); // WZAPI
	JS_REGISTER_FUNC(setFogColour, 3); // WZAPI
	JS_REGISTER_FUNC(setWeather, 1); // WZAPI
	JS_REGISTER_FUNC(setSky, 3); // WZAPI
	JS_REGISTER_FUNC(cameraSlide, 2); // WZAPI
	JS_REGISTER_FUNC(cameraTrack, 1); // WZAPI
	JS_REGISTER_FUNC(cameraZoom, 2); // WZAPI
	JS_REGISTER_FUNC_NAME2(resetArea, 1, 2, JS_FUNC_IMPL_NAME(resetLabel)); // deprecated // scripting_engine
	JS_REGISTER_FUNC2(resetLabel, 1, 2); // scripting_engine
	JS_REGISTER_FUNC(addSpotter, 6); // WZAPI
	JS_REGISTER_FUNC(removeSpotter, 1); // WZAPI
	JS_REGISTER_FUNC2(syncRequest, 3, 5); // WZAPI
	JS_REGISTER_FUNC(replaceTexture, 2); // WZAPI
	JS_REGISTER_FUNC(changePlayerColour, 2); // WZAPI
	JS_REGISTER_FUNC(setHealth, 2); // WZAPI
	JS_REGISTER_FUNC(useSafetyTransport, 1); // WZAPI
	JS_REGISTER_FUNC(restoreLimboMissionData, 0); // WZAPI
	JS_REGISTER_FUNC(getMultiTechLevel, 0); // WZAPI
	JS_REGISTER_FUNC(setCampaignNumber, 1); // WZAPI
	JS_REGISTER_FUNC(getMissionType, 0); // WZAPI
	JS_REGISTER_FUNC(getRevealStatus, 0); // WZAPI
	JS_REGISTER_FUNC(setRevealStatus, 1); // WZAPI
	JS_REGISTER_FUNC(autoSave, 0); // WZAPI

	// horrible hacks follow -- do not rely on these being present!
	JS_REGISTER_FUNC(hackNetOff, 0); // WZAPI
	JS_REGISTER_FUNC(hackNetOn, 0); // WZAPI
	JS_REGISTER_FUNC(hackAddMessage, 4); // WZAPI
	JS_REGISTER_FUNC(hackRemoveMessage, 3); // WZAPI
	JS_REGISTER_FUNC(hackGetObj, 3); // WZAPI
	JS_REGISTER_FUNC2(hackAssert, 2, 2 + MAX_JS_VARARGS); // WZAPI
	JS_REGISTER_FUNC2(hackMarkTiles, 1, 4); // WZAPI
	JS_REGISTER_FUNC2(receiveAllEvents, 0, 1); // WZAPI
	JS_REGISTER_FUNC(hackDoNotSave, 1); // WZAPI
	JS_REGISTER_FUNC(hackPlayIngameAudio, 0); // WZAPI
	JS_REGISTER_FUNC(hackStopIngameAudio, 0); // WZAPI

	// General functions -- geared for use in AI scripts
	JS_REGISTER_FUNC2(debug, 1, 1 + MAX_JS_VARARGS);
	JS_REGISTER_FUNC2(console, 1, 1 + MAX_JS_VARARGS); // WZAPI
	JS_REGISTER_FUNC(clearConsole, 0); // WZAPI
	JS_REGISTER_FUNC(structureIdle, 1); // WZAPI
	JS_REGISTER_FUNC2(enumStruct, 0, 3); // WZAPI
	JS_REGISTER_FUNC2(enumStructOffWorld, 0, 3); // WZAPI
	JS_REGISTER_FUNC2(enumDroid, 0, 3); // WZAPI
	JS_REGISTER_FUNC(enumGroup, 1); // scripting_engine
	JS_REGISTER_FUNC2(enumFeature, 1, 2); // WZAPI
	JS_REGISTER_FUNC(enumBlips, 1); // WZAPI
	JS_REGISTER_FUNC(enumSelected, 0); // WZAPI
	JS_REGISTER_FUNC(enumResearch, 0); // WZAPI
	JS_REGISTER_FUNC2(enumRange, 3, 5); // WZAPI
	JS_REGISTER_FUNC2(enumArea, 1, 6); // scripting_engine
	JS_REGISTER_FUNC2(getResearch, 1, 2); // WZAPI
	JS_REGISTER_FUNC(pursueResearch, 2); // WZAPI
	JS_REGISTER_FUNC2(findResearch, 1, 2); // WZAPI
	JS_REGISTER_FUNC(distBetweenTwoPoints, 4); // WZAPI
	JS_REGISTER_FUNC(newGroup, 0); // scripting_engine
	JS_REGISTER_FUNC(groupAddArea, 5); // scripting_engine
	JS_REGISTER_FUNC(groupAddDroid, 2); // scripting_engine
	JS_REGISTER_FUNC(groupAdd, 2); // scripting_engine
	JS_REGISTER_FUNC(groupSize, 1); // scripting_engine
	JS_REGISTER_FUNC(orderDroidLoc, 4); // WZAPI
	JS_REGISTER_FUNC(playerPower, 1); // WZAPI
	JS_REGISTER_FUNC(queuedPower, 1); // WZAPI
	JS_REGISTER_FUNC2(isStructureAvailable, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(pickStructLocation, 4, 5); // WZAPI
	JS_REGISTER_FUNC(droidCanReach, 3); // WZAPI
	JS_REGISTER_FUNC(propulsionCanReach, 5); // WZAPI
	JS_REGISTER_FUNC(terrainType, 2); // WZAPI
	JS_REGISTER_FUNC(tileIsBurning, 2); // WZAPI
	JS_REGISTER_FUNC2(orderDroidBuild, 5, 6); // WZAPI
	JS_REGISTER_FUNC(orderDroidObj, 3); // WZAPI
	JS_REGISTER_FUNC(orderDroid, 2); // WZAPI
	JS_REGISTER_FUNC2(buildDroid, 7, 7 + MAX_JS_VARARGS); // WZAPI
	JS_REGISTER_FUNC2(addDroid, 9, 9 + MAX_JS_VARARGS); // WZAPI
	JS_REGISTER_FUNC(addDroidToTransporter, 2); // WZAPI
	JS_REGISTER_FUNC(addFeature, 3); // WZAPI
	JS_REGISTER_FUNC2(componentAvailable, 1, 2); // WZAPI
	JS_REGISTER_FUNC(isVTOL, 1); // WZAPI
	JS_REGISTER_FUNC(safeDest, 3); // WZAPI
	JS_REGISTER_FUNC2(activateStructure, 1, 2); // WZAPI
	JS_REGISTER_FUNC(chat, 2); // WZAPI
	JS_REGISTER_FUNC(quickChat, 2); // WZAPI
	JS_REGISTER_FUNC(getDroidPath, 1); // WZAPI
	JS_REGISTER_FUNC2(addBeacon, 3, 4); // WZAPI
	JS_REGISTER_FUNC(removeBeacon, 1); // WZAPI
	JS_REGISTER_FUNC(getDroidProduction, 1); // WZAPI
	JS_REGISTER_FUNC2(getDroidLimit, 0, 2); // WZAPI
	JS_REGISTER_FUNC(getExperienceModifier, 1); // WZAPI
	JS_REGISTER_FUNC2(setDroidLimit, 2, 3); // WZAPI
	JS_REGISTER_FUNC(setCommanderLimit, 2); // deprecated!!
	JS_REGISTER_FUNC(setConstructorLimit, 2); // deprecated!!
	JS_REGISTER_FUNC(setExperienceModifier, 2); // WZAPI
	JS_REGISTER_FUNC(getWeaponInfo, 1); // WZAPI // deprecated!!
	JS_REGISTER_FUNC(enumCargo, 1); // WZAPI

	// Functions that operate on the current player only
	JS_REGISTER_FUNC(centreView, 2); // WZAPI
	JS_REGISTER_FUNC2(playSound, 1, 4); // WZAPI
	JS_REGISTER_FUNC2(gameOverMessage, 1, 3); // WZAPI
	JS_REGISTER_FUNC2(addGuideTopic, 1, 3); // WZAPI

	// Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)
	JS_REGISTER_FUNC2(setStructureLimits, 2, 3); // WZAPI
	JS_REGISTER_FUNC(applyLimitSet, 0); // WZAPI
	JS_REGISTER_FUNC(setMissionTime, 1); // WZAPI
	JS_REGISTER_FUNC(getMissionTime, 0); // WZAPI
	JS_REGISTER_FUNC2(setReinforcementTime, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(completeResearch, 1, 3); // WZAPI
	JS_REGISTER_FUNC2(completeAllResearch, 0, 1); // WZAPI
	JS_REGISTER_FUNC2(enableResearch, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(setPower, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(setPowerModifier, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(setPowerStorageMaximum, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(extraPowerTime, 1, 2); // WZAPI
	JS_REGISTER_FUNC(setTutorialMode, 1); // WZAPI
	JS_REGISTER_FUNC(setDesign, 1); // WZAPI
	JS_REGISTER_FUNC(enableTemplate, 1); // WZAPI
	JS_REGISTER_FUNC(removeTemplate, 1); // WZAPI
	JS_REGISTER_FUNC(setMiniMap, 1); // WZAPI
	JS_REGISTER_FUNC2(setReticuleButton, 4, 5); // WZAPI
	JS_REGISTER_FUNC(setReticuleFlash, 2); // WZAPI
	JS_REGISTER_FUNC(showReticuleWidget, 1); // WZAPI
	JS_REGISTER_FUNC(showInterface, 0); // WZAPI
	JS_REGISTER_FUNC(hideInterface, 0); // WZAPI
	JS_REGISTER_FUNC_NAME(addReticuleButton, 1, JS_FUNC_IMPL_NAME(removeReticuleButton)); // deprecated!!
	JS_REGISTER_FUNC(removeReticuleButton, 1); // deprecated!!
	JS_REGISTER_FUNC2(enableStructure, 1, 2); // WZAPI
	JS_REGISTER_FUNC(makeComponentAvailable, 2); // WZAPI
	JS_REGISTER_FUNC(enableComponent, 2); // WZAPI
	JS_REGISTER_FUNC(allianceExistsBetween, 2); // WZAPI
	JS_REGISTER_FUNC(removeStruct, 1); // WZAPI // deprecated!!
	JS_REGISTER_FUNC2(removeObject, 1, 2); // WZAPI
	JS_REGISTER_FUNC_NAME(setScrollParams, 4, JS_FUNC_IMPL_NAME(setScrollLimits)); // deprecated!!
	JS_REGISTER_FUNC(setScrollLimits, 4); // WZAPI
	JS_REGISTER_FUNC(getScrollLimits, 0); // WZAPI
	JS_REGISTER_FUNC(addStructure, 4); // WZAPI
	JS_REGISTER_FUNC2(getStructureLimit, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(countStruct, 1, 2); // WZAPI
	JS_REGISTER_FUNC2(countDroid, 0, 2); // WZAPI
	JS_REGISTER_FUNC(loadLevel, 1); // WZAPI
	JS_REGISTER_FUNC(setDroidExperience, 2); // WZAPI
	JS_REGISTER_FUNC(donateObject, 2); // WZAPI
	JS_REGISTER_FUNC(donatePower, 2); // WZAPI
	JS_REGISTER_FUNC(setNoGoArea, 5); // WZAPI
	JS_REGISTER_FUNC(startTransporterEntry, 3); // WZAPI
	JS_REGISTER_FUNC(setTransporterExit, 3); // WZAPI
	JS_REGISTER_FUNC(setObjectFlag, 3); // WZAPI
	JS_REGISTER_FUNC2(fireWeaponAtLoc, 3, 4); // WZAPI
	JS_REGISTER_FUNC2(fireWeaponAtObj, 2, 3); // WZAPI
	JS_REGISTER_FUNC(transformPlayerToSpectator, 1); // WZAPI
	JS_REGISTER_FUNC(isSpectator, 1); // WZAPI
	JS_REGISTER_FUNC(setGameStoryLogPlayerDataValue, 3); // WZAPI

	return true;
}

// Enable JSON support for custom types

// JSContextValue
void to_json(nlohmann::json& j, const JSContextValue& v) {
	// IMPORTANT: This largely follows the Qt documentation on QJsonValue::fromVariant
	// See: http://doc.qt.io/qt-5/qjsonvalue.html#fromVariant
	//
	// The main change is that numeric types are independently handled (instead of
	// converting everything to `double`), because nlohmann::json handles them a bit
	// differently.

	// Note: Older versions of Qt 5.x (5.6?) do not define QMetaType::Nullptr,
	//		 so check value.isNull() instead.
	if (JS_IsNull(v.value))
	{
		j = nlohmann::json(); // null value
		return;
	}

	if (JS_IsArray(v.ctx, v.value))
	{
		j = nlohmann::json::array();
		uint64_t length = 0;
		if (QuickJS_GetArrayLength(v.ctx, v.value, length))
		{
			for (uint64_t k = 0; k < length; k++) // TODO: uint64_t isn't correct here, as we call GetUint32...
			{
				JSValue jsVal = JS_GetPropertyUint32(v.ctx, v.value, k);
				nlohmann::json jsonValue;
				to_json(jsonValue, JSContextValue{v.ctx, jsVal, v.skip_constructors});
				j.push_back(jsonValue);
				JS_FreeValue(v.ctx, jsVal);
			}
		}
		return;
	}
	if (JS_IsObject(v.value))
	{
		if (JS_IsConstructor(v.ctx, v.value))
		{
			j = (!v.skip_constructors) ? "<constructor>" : nlohmann::json() /* null value */;
			return;
		}
		j = nlohmann::json::object();
		QuickJS_EnumerateObjectProperties(v.ctx, v.value, [v, &j](const char *key, JSAtom &atom) {
			JSValue jsVal = JS_GetProperty(v.ctx, v.value, atom);
			std::string nameStr = key;
			if (!JS_IsException(jsVal))
			{
				if (!JS_IsConstructor(v.ctx, jsVal))
				{
					j[nameStr] = JSContextValue{v.ctx, jsVal, v.skip_constructors};
				}
				else if (!v.skip_constructors)
				{
					j[nameStr] = "<constructor>";
				}
			}
			else
			{
				debug(LOG_INFO, "Got an exception trying to get the value of \"%s\"?", nameStr.c_str());
			}
			JS_FreeValue(v.ctx, jsVal);
		}, false);
		return;
	}

	int tag = JS_VALUE_GET_NORM_TAG(v.value);
	switch (tag)
	{
		case JS_TAG_BOOL:
			j = static_cast<bool>(JS_ToBool(v.ctx, v.value) != 0);
			break;
		case JS_TAG_INT:
		{
			int32_t intVal = 0;
			if (JS_ToInt32(v.ctx, &intVal, v.value))
			{
				// Failed
				debug(LOG_SCRIPT, "Failed to convert to int32_t");
			}
			j = intVal;
			break;
		}
		case JS_TAG_FLOAT64:
		{
			double dblVal = 0.0;
			if (JS_ToFloat64(v.ctx, &dblVal, v.value))
			{
				// Failed
				debug(LOG_SCRIPT, "Failed to convert to double");
			}
			j = dblVal;
			break;
		}
		case JS_TAG_UNDEFINED:
			j = nlohmann::json(); // null value // ???
			break;
		case JS_TAG_STRING:
		{
			const char* pStr = JS_ToCString(v.ctx, v.value);
			j = json(pStr ? pStr : "");
			JS_FreeCString(v.ctx, pStr);
			break;
		}
		default:
		{
			// In every other case, a conversion to a string will be attempted
			const char* pStr = JS_ToCString(v.ctx, v.value);
			// If the returned string is empty, a Null QJsonValue will be stored, otherwise a String value using the returned QString.
			if (pStr)
			{
				std::string str(pStr);
				if (str.empty())
				{
					j = nlohmann::json(); // null value
				}
				else
				{
					j = str;
				}
				JS_FreeCString(v.ctx, pStr);
			}
			else
			{
				j = nlohmann::json(); // null value
			}
		}
	}
}
