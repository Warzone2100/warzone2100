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
 * @file qtscriptfuncs.cpp
 *
 * New scripting system -- script functions
 */

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-copy" // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

// **NOTE: Qt headers _must_ be before platform specific headers so we don't get conflicts.
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtScript/QScriptSyntaxCheckResult>
#include <QtScript/QScriptContextInfo>
#include <QtCore/QStringList>
#include <QtCore/QJsonArray>
#include <QtGui/QStandardItemModel>
#include <QtCore/QPointer>
#include <QtCore/QFileInfo>
#include <QtCore/QVariant>

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic pop // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/fixedpoint.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/netplay/netplay.h"
#include "qtscriptfuncs.h"
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


#include <unordered_set>
#include "lib/framework/file.h"
#include <unordered_map>
#include "3rdparty/integer_sequence.hpp"

void to_json(nlohmann::json& j, const QVariant& value); // forward-declare

class qtscript_scripting_instance;
static std::map<QScriptEngine*, qtscript_scripting_instance *> engineToInstanceMap;

class qtscript_scripting_instance : public wzapi::scripting_instance
{
public:
	qtscript_scripting_instance(int player, const std::string& scriptName)
	: scripting_instance(player, scriptName)
	{
		engine = new QScriptEngine();
		// Set processEventsInterval to -1 because the interpreter should *never* call
		// QCoreApplication::processEvents() (or SDL builds will break in various ways).
		engine->setProcessEventsInterval(-1);

		engineToInstanceMap.insert(std::pair<QScriptEngine*, qtscript_scripting_instance*>(engine, this));
	}
	virtual ~qtscript_scripting_instance()
	{
		engineToInstanceMap.erase(engine);
		delete engine;
	}
	bool loadScript(const WzString& path, int player, int difficulty);
	bool readyInstanceForExecution() override;

private:
	bool registerFunctions(const QString& scriptName);

public:
	// save / restore state
	virtual bool saveScriptGlobals(nlohmann::json &result) override;
	virtual bool loadScriptGlobals(const nlohmann::json &result) override;

	virtual nlohmann::json saveTimerFunction(uniqueTimerID timerID, std::string timerName, timerAdditionalData* additionalParam) override;

	// recreates timer functions (and additional userdata) based on the information saved by the saveTimerFunction() method
	virtual std::tuple<TimerFunc, timerAdditionalData *> restoreTimerFunction(const nlohmann::json& savedTimerFuncData) override;

public:
	// get state for debugging
	nlohmann::json debugGetAllScriptGlobals() override;

	bool debugEvaluateCommand(const std::string &text) override;

public:

	void updateGameTime(uint32_t gameTime) override;
	void updateGroupSizes(int group, int size) override;

	void setSpecifiedGlobalVariables(const nlohmann::json& variables, wzapi::GlobalVariableFlags flags = wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave) override;

	void setSpecifiedGlobalVariable(const std::string& name, const nlohmann::json& value, wzapi::GlobalVariableFlags flags = wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave) override;

private:
	inline QScriptValue::PropertyFlags toQScriptPropertyFlags(wzapi::GlobalVariableFlags flags)
	{
		QScriptValue::PropertyFlags propertyFlags = QScriptValue::Undeletable;
		if (((flags & wzapi::GlobalVariableFlags::ReadOnly) == wzapi::GlobalVariableFlags::ReadOnly)
			|| ((flags & wzapi::GlobalVariableFlags::ReadOnlyUpdatedFromApp) == wzapi::GlobalVariableFlags::ReadOnlyUpdatedFromApp))
		{
			propertyFlags |= QScriptValue::ReadOnly; // for QtScript, this is apparently just "advisory", and doesn't actually prevent the app from updating / resetting the value
		}
		return propertyFlags;
	}

public:
	QScriptEngine * getQScriptEngine() { return engine; }

	void doNotSaveGlobal(const std::string &global);

private:
	QScriptEngine *engine = nullptr;
	QString m_source;
	QString m_path;
	/// Remember what names are used internally in the scripting engine, we don't want to save these to the savegame
	std::unordered_set<std::string> internalNamespace;
	/// Separate event namespaces for libraries
public: // temporary
	QStringList eventNamespaces;

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

	//__
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
	//__An event that is run when current user picks a body in the design menu.
	//__
	virtual bool handle_eventDesignBody() override;

	//__ ## eventDesignPropulsion()
	//__
	//__An event that is run when current user picks a propulsion in the design menu.
	//__
	virtual bool handle_eventDesignPropulsion() override;

	//__ ## eventDesignWeapon()
	//__
	//__An event that is run when current user picks a weapon in the design menu.
	//__
	virtual bool handle_eventDesignWeapon() override;

	//__ ## eventDesignCommand()
	//__
	//__An event that is run when current user picks a command turret in the design menu.
	//__
	virtual bool handle_eventDesignCommand() override;

	//__ ## eventDesignSystem()
	//__
	//__An event that is run when current user picks a system other than command turret in the design menu.
	//__
	virtual bool handle_eventDesignSystem() override;

	//__ ## eventDesignQuit()
	//__
	//__An event that is run when current user leaves the design menu.
	//__
	virtual bool handle_eventDesignQuit() override;

	//__ ## eventMenuBuildSelected()
	//__
	//__An event that is run when current user picks something new in the build menu.
	//__
	virtual bool handle_eventMenuBuildSelected(/*BASE_OBJECT *psObj*/) override;

	//__ ## eventMenuResearchSelected()
	//__
	//__An event that is run when current user picks something new in the research menu.
	//__
	virtual bool handle_eventMenuResearchSelected(/*BASE_OBJECT *psObj*/) override;

	//__ ## eventMenuBuild()
	//__
	//__An event that is run when current user opens the build menu.
	//__
	virtual bool handle_eventMenuBuild() override;

	//__ ## eventMenuResearch()
	//__
	//__An event that is run when current user opens the research menu.
	//__
	virtual bool handle_eventMenuResearch() override;


	virtual bool handle_eventMenuDesign() override;

	//__ ## eventMenuManufacture()
	//__An event that is run when current user opens the manufacture menu.
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

	//__ ## eventPlayerLeft(player index)
	//__
	//__ An event that is run after a player has left the game.
	//__
	virtual bool handle_eventPlayerLeft(int id) override;

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
	virtual bool handle_eventDroidBuilt(const DROID *psDroid, const STRUCTURE *psFactory) override;

	//__ ## eventStructureBuilt(structure[, droid])
	//__
	//__ An event that is run every time a structure is produced. The droid parameter is set
	//__ if the structure was built by a droid. It is not triggered for building theft
	//__ (check ```eventObjectTransfer``` for that).
	//__
	virtual bool handle_eventStructureBuilt(const STRUCTURE *psStruct, const DROID *psDroid) override;

	//__ ## eventStructureDemolish(structure[, droid])
	//__
	//__ An event that is run every time a structure begins to be demolished. This does
	//__ not trigger again if the structure is partially demolished.
	//__
	virtual bool handle_eventStructureDemolish(const STRUCTURE *psStruct, const DROID *psDroid) override;

	//__ ## eventStructureReady(structure)
	//__
	//__ An event that is run every time a structure is ready to perform some
	//__ special ability. It will only fire once, so if the time is not right,
	//__ register your own timer to keep checking.
	//__
	virtual bool handle_eventStructureReady(const STRUCTURE *psStruct) override;

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
	virtual bool handle_eventResearched(const wzapi::researchResult& research, const STRUCTURE *psStruct, int player) override;

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

	//__ ## eventBeacon(x, y, from, to[, message])
	//__
	//__ An event that is run whenever a beacon message is received. The ```from``` parameter is the
	//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
	//__ Message may be undefined.
	//__
	virtual bool handle_eventBeacon(int x, int y, int from, int to, const char *message) override;

	//__ ## eventBeaconRemoved(from, to)
	//__
	//__ An event that is run whenever a beacon message is removed. The ```from``` parameter is the
	//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
	//__
	virtual bool handle_eventBeaconRemoved(int from, int to) override;

	//__ ## eventGroupLoss(object, group id, new size)
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
	//__ eventArea + the name of the label.
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

	//__ ## eventKeyPressed(meta, key)
	//__
	//__ An event that is called whenever user presses a key in the game, not counting chat
	//__ or other pop-up user interfaces. The key values are currently undocumented.
	virtual bool handle_eventKeyPressed(int meta, int key) override;
};

#define ALL_PLAYERS -1
#define ALLIES -2
#define ENEMIES -3

// private qtscript bureaucracy

/// Assert for scripts that give useful backtraces and other info.
#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
			context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed in " + QString(__FUNCTION__) + " at line " + QString::number(__LINE__)); \
			return QScriptValue::NullValue; } } while (0)

#define SCRIPT_ASSERT_PLAYER(_context, _player) \
	SCRIPT_ASSERT(_context, _player >= 0 && _player < MAX_PLAYERS, "Invalid player index %d", _player);

#define WzStringToQScriptValue(_engine, _wzstring) \
	QScriptValue(_engine, QString::fromUtf8(_wzstring.toUtf8().c_str()))

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

QScriptValue mapJsonToQScriptValue(QScriptEngine *engine, const nlohmann::json &instance, QScriptValue::PropertyFlags flags); // forward-declare

static QScriptValue mapJsonObjectToQScriptValue(QScriptEngine *engine, const nlohmann::json &obj, QScriptValue::PropertyFlags flags)
{
	QScriptValue value = engine->newObject();
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		value.setProperty(QString::fromUtf8(it.key().c_str()), mapJsonToQScriptValue(engine, it.value(), flags), flags);
	}
	return value;
}

static QScriptValue mapJsonArrayToQScriptValue(QScriptEngine *engine, const nlohmann::json &array, QScriptValue::PropertyFlags flags)
{
	QScriptValue value = engine->newArray(array.size());
	for (int i = 0; i < array.size(); i++)
	{
		value.setProperty(i, mapJsonToQScriptValue(engine, array.at(i), flags), flags);
	}
	return value;
}

QScriptValue mapJsonToQScriptValue(QScriptEngine *engine, const nlohmann::json &instance, QScriptValue::PropertyFlags flags)
{
	switch (instance.type())
	{
		// IMPORTANT: To match the prior behavior of loading a QVariant from the JSON value and using engine->toScriptValue(QVariant)
		//			  to convert to a QScriptValue, "null" JSON values *MUST* map to QScriptValue::UndefinedValue.
		//
		//			  If they are set to QScriptValue::NullValue, it causes issues for libcampaign.js. (As the values become "defined".)
		//
		case json::value_t::null : return QScriptValue::UndefinedValue;
		case json::value_t::boolean : return engine->toScriptValue(instance.get<bool>());
		case json::value_t::number_integer: return engine->toScriptValue(instance.get<int>());
		case json::value_t::number_unsigned: return engine->toScriptValue(instance.get<unsigned>());
		case json::value_t::number_float: return engine->toScriptValue(instance.get<double>());
		case json::value_t::string	: return engine->toScriptValue(QString::fromUtf8(instance.get<WzString>().toUtf8().c_str()));
		case json::value_t::array : return mapJsonArrayToQScriptValue(engine, instance, flags);
		case json::value_t::object : return mapJsonObjectToQScriptValue(engine, instance, flags);
		case json::value_t::binary :
			debug(LOG_ERROR, "Unexpected binary value type");
			return QScriptValue::UndefinedValue;
		case json::value_t::discarded : return QScriptValue::UndefinedValue;
	}
	return QScriptValue::UndefinedValue; // should never be reached
}

// Forward-declare
QScriptValue convDroid(const DROID *psDroid, QScriptEngine *engine);
QScriptValue convStructure(const STRUCTURE *psStruct, QScriptEngine *engine);
QScriptValue convObj(const BASE_OBJECT *psObj, QScriptEngine *engine);
QScriptValue convFeature(const FEATURE *psFeature, QScriptEngine *engine);
QScriptValue convMax(const BASE_OBJECT *psObj, QScriptEngine *engine);
QScriptValue convTemplate(const DROID_TEMPLATE *psTemplate, QScriptEngine *engine);
QScriptValue convResearch(const RESEARCH *psResearch, QScriptEngine *engine, int player);

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
//;;
QScriptValue convResearch(const RESEARCH *psResearch, QScriptEngine *engine, int player)
{
	if (psResearch == nullptr)
	{
		return QScriptValue::NullValue;
	}
	QScriptValue value = engine->newObject();
	value.setProperty("power", (int)psResearch->researchPower);
	value.setProperty("points", (int)psResearch->researchPoints);
	bool started = false;
	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (aiCheckAlliances(player, i) || player == i)
		{
			int bits = asPlayerResList[i][psResearch->index].ResearchStatus;
			started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING) || (bits & RESBITS_PENDING_ONLY);
		}
	}
	value.setProperty("started", started); // including whether an ally has started it
	value.setProperty("done", IsResearchCompleted(&asPlayerResList[player][psResearch->index]));
	value.setProperty("fullname", WzStringToQScriptValue(engine, psResearch->name)); // temporary
	value.setProperty("name", WzStringToQScriptValue(engine, psResearch->id)); // will be changed to contain fullname
	value.setProperty("id", WzStringToQScriptValue(engine, psResearch->id));
	value.setProperty("type", SCRIPT_RESEARCH);
	value.setProperty("results", mapJsonToQScriptValue(engine, psResearch->results, QScriptValue::ReadOnly | QScriptValue::Undeletable));
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
//;; * ```stattype``` The stattype defines the type of structure. It will be one of ```HQ```, ```FACTORY```, ```POWER_GEN```,
//;; ```RESOURCE_EXTRACTOR```, ```LASSAT```, ```DEFENSE```, ```WALL```, ```RESEARCH_LAB```, ```REPAIR_FACILITY```,
//;; ```CYBORG_FACTORY```, ```VTOL_FACTORY```, ```REARM_PAD```, ```SAT_UPLINK```, ```GATE``` and ```COMMAND_CONTROL```.
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
QScriptValue convStructure(const STRUCTURE *psStruct, QScriptEngine *engine)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	for (int i = 0; i < psStruct->numWeaps; i++)
	{
		if (psStruct->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = &asWeaponStats[psStruct->asWeaps[i].nStat];
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(psWeap, psStruct->player), range);
		}
	}
	QScriptValue value = convObj(psStruct, engine);
	value.setProperty("isCB", structCBSensor(psStruct), QScriptValue::ReadOnly);
	value.setProperty("isSensor", structStandardSensor(psStruct), QScriptValue::ReadOnly);
	value.setProperty("canHitAir", aa, QScriptValue::ReadOnly);
	value.setProperty("canHitGround", ga, QScriptValue::ReadOnly);
	value.setProperty("hasIndirect", indirect, QScriptValue::ReadOnly);
	value.setProperty("isRadarDetector", objRadarDetector(psStruct), QScriptValue::ReadOnly);
	value.setProperty("range", range, QScriptValue::ReadOnly);
	value.setProperty("status", (int)psStruct->status, QScriptValue::ReadOnly);
	value.setProperty("health", 100 * psStruct->body / MAX(1, structureBody(psStruct)), QScriptValue::ReadOnly);
	value.setProperty("cost", psStruct->pStructureType->powerToBuild, QScriptValue::ReadOnly);
	switch (psStruct->pStructureType->type) // don't bleed our source insanities into the scripting world
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
		value.setProperty("stattype", (int)REF_WALL, QScriptValue::ReadOnly);
		break;
	case REF_GENERIC:
	case REF_DEFENSE:
		value.setProperty("stattype", (int)REF_DEFENSE, QScriptValue::ReadOnly);
		break;
	default:
		value.setProperty("stattype", (int)psStruct->pStructureType->type, QScriptValue::ReadOnly);
		break;
	}
	if (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	    || psStruct->pStructureType->type == REF_VTOL_FACTORY
	    || psStruct->pStructureType->type == REF_RESEARCH
	    || psStruct->pStructureType->type == REF_POWER_GEN)
	{
		value.setProperty("modules", psStruct->capacity, QScriptValue::ReadOnly);
	}
	else
	{
		value.setProperty("modules", QScriptValue::NullValue);
	}
	QScriptValue weaponlist = engine->newArray(psStruct->numWeaps);
	for (int j = 0; j < psStruct->numWeaps; j++)
	{
		QScriptValue weapon = engine->newObject();
		const WEAPON_STATS *psStats = asWeaponStats + psStruct->asWeaps[j].nStat;
		weapon.setProperty("fullname", WzStringToQScriptValue(engine, psStats->name), QScriptValue::ReadOnly);
		weapon.setProperty("name", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly); // will be changed to contain full name
		weapon.setProperty("id", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly);
		weapon.setProperty("lastFired", psStruct->asWeaps[j].lastFired, QScriptValue::ReadOnly);
		weaponlist.setProperty(j, weapon, QScriptValue::ReadOnly);
	}
	value.setProperty("weapons", weaponlist, QScriptValue::ReadOnly);
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
QScriptValue convFeature(const FEATURE *psFeature, QScriptEngine *engine)
{
	QScriptValue value = convObj(psFeature, engine);
	const FEATURE_STATS *psStats = psFeature->psStats;
	value.setProperty("health", 100 * psStats->body / MAX(1, psFeature->body), QScriptValue::ReadOnly);
	value.setProperty("damageable", psStats->damageable, QScriptValue::ReadOnly);
	value.setProperty("stattype", psStats->subType, QScriptValue::ReadOnly);
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
QScriptValue convDroid(const DROID *psDroid, QScriptEngine *engine)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	const BODY_STATS *psBodyStats = &asBodyStats[psDroid->asBits[COMP_BODY]];

	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = &asWeaponStats[psDroid->asWeaps[i].nStat];
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(psWeap, psDroid->player), range);
		}
	}
	DROID_TYPE type = psDroid->droidType;
	QScriptValue value = convObj(psDroid, engine);
	value.setProperty("action", (int)psDroid->action, QScriptValue::ReadOnly);
	if (range >= 0)
	{
		value.setProperty("range", range, QScriptValue::ReadOnly);
	}
	else
	{
		value.setProperty("range", QScriptValue::NullValue);
	}
	value.setProperty("order", (int)psDroid->order.type, QScriptValue::ReadOnly);
	value.setProperty("cost", calcDroidPower(psDroid), QScriptValue::ReadOnly);
	value.setProperty("hasIndirect", indirect, QScriptValue::ReadOnly);
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
	value.setProperty("bodySize", psBodyStats->size, QScriptValue::ReadOnly);
	if (isTransporter(psDroid))
	{
		value.setProperty("cargoCapacity", TRANSPORTER_CAPACITY, QScriptValue::ReadOnly);
		value.setProperty("cargoLeft", calcRemainingCapacity(psDroid), QScriptValue::ReadOnly);
		value.setProperty("cargoCount", psDroid->psGroup != nullptr? psDroid->psGroup->getNumMembers() : 0, QScriptValue::ReadOnly);
	}
	value.setProperty("isRadarDetector", objRadarDetector(psDroid), QScriptValue::ReadOnly);
	value.setProperty("isCB", cbSensorDroid(psDroid), QScriptValue::ReadOnly);
	value.setProperty("isSensor", standardSensorDroid(psDroid), QScriptValue::ReadOnly);
	value.setProperty("canHitAir", aa, QScriptValue::ReadOnly);
	value.setProperty("canHitGround", ga, QScriptValue::ReadOnly);
	value.setProperty("isVTOL", isVtolDroid(psDroid), QScriptValue::ReadOnly);
	value.setProperty("droidType", (int)type, QScriptValue::ReadOnly);
	value.setProperty("experience", (double)psDroid->experience / 65536.0, QScriptValue::ReadOnly);
	value.setProperty("health", 100.0 / (double)psDroid->originalBody * (double)psDroid->body, QScriptValue::ReadOnly);
	value.setProperty("body", WzStringToQScriptValue(engine, asBodyStats[psDroid->asBits[COMP_BODY]].id), QScriptValue::ReadOnly);
	value.setProperty("propulsion", WzStringToQScriptValue(engine, asPropulsionStats[psDroid->asBits[COMP_PROPULSION]].id), QScriptValue::ReadOnly);
	value.setProperty("armed", 0.0, QScriptValue::ReadOnly); // deprecated!
	QScriptValue weaponlist = engine->newArray(psDroid->numWeaps);
	for (int j = 0; j < psDroid->numWeaps; j++)
	{
		int armed = droidReloadBar(psDroid, &psDroid->asWeaps[j], j);
		QScriptValue weapon = engine->newObject();
		const WEAPON_STATS *psStats = asWeaponStats + psDroid->asWeaps[j].nStat;
		weapon.setProperty("fullname", WzStringToQScriptValue(engine, psStats->name), QScriptValue::ReadOnly);
		weapon.setProperty("id", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly); // will be changed to full name
		weapon.setProperty("name", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly);
		weapon.setProperty("lastFired", psDroid->asWeaps[j].lastFired, QScriptValue::ReadOnly);
		weapon.setProperty("armed", armed, QScriptValue::ReadOnly);
		weaponlist.setProperty(j, weapon, QScriptValue::ReadOnly);
	}
	value.setProperty("weapons", weaponlist, QScriptValue::ReadOnly);
	value.setProperty("cargoSize", transporterSpaceRequired(psDroid), QScriptValue::ReadOnly);
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
QScriptValue convObj(const BASE_OBJECT *psObj, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psObj, "No object for conversion");
	value.setProperty("id", psObj->id, QScriptValue::ReadOnly);
	value.setProperty("x", map_coord(psObj->pos.x), QScriptValue::ReadOnly);
	value.setProperty("y", map_coord(psObj->pos.y), QScriptValue::ReadOnly);
	value.setProperty("z", map_coord(psObj->pos.z), QScriptValue::ReadOnly);
	value.setProperty("player", psObj->player, QScriptValue::ReadOnly);
	value.setProperty("armour", objArmour(psObj, WC_KINETIC), QScriptValue::ReadOnly);
	value.setProperty("thermal", objArmour(psObj, WC_HEAT), QScriptValue::ReadOnly);
	value.setProperty("type", psObj->type, QScriptValue::ReadOnly);
	value.setProperty("selected", psObj->selected, QScriptValue::ReadOnly);
	value.setProperty("name", objInfo(psObj), QScriptValue::ReadOnly);
	value.setProperty("born", psObj->born, QScriptValue::ReadOnly);
	scripting_engine::GROUPMAP *psMap = scripting_engine::instance().getGroupMap(engineToInstanceMap.at(engine));
	if (psMap != nullptr && psMap->map().count(psObj) > 0) // FIXME:
	{
		int group = psMap->map().at(psObj); // FIXME:
		value.setProperty("group", group, QScriptValue::ReadOnly);
	}
	else
	{
		value.setProperty("group", QScriptValue::NullValue);
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
QScriptValue convTemplate(const DROID_TEMPLATE *psTempl, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psTempl, "No object for conversion");
	value.setProperty("fullname", WzStringToQScriptValue(engine, psTempl->name), QScriptValue::ReadOnly);
	value.setProperty("name", WzStringToQScriptValue(engine, psTempl->id), QScriptValue::ReadOnly);
	value.setProperty("id", WzStringToQScriptValue(engine, psTempl->id), QScriptValue::ReadOnly);
	value.setProperty("points", calcTemplateBuild(psTempl), QScriptValue::ReadOnly);
	value.setProperty("power", calcTemplatePower(psTempl), QScriptValue::ReadOnly); // deprecated, use cost below
	value.setProperty("cost", calcTemplatePower(psTempl), QScriptValue::ReadOnly);
	value.setProperty("droidType", psTempl->droidType, QScriptValue::ReadOnly);
	value.setProperty("body", WzStringToQScriptValue(engine, (asBodyStats + psTempl->asParts[COMP_BODY])->id), QScriptValue::ReadOnly);
	value.setProperty("propulsion", WzStringToQScriptValue(engine, (asPropulsionStats + psTempl->asParts[COMP_PROPULSION])->id), QScriptValue::ReadOnly);
	value.setProperty("brain", WzStringToQScriptValue(engine, (asBrainStats + psTempl->asParts[COMP_BRAIN])->id), QScriptValue::ReadOnly);
	value.setProperty("repair", WzStringToQScriptValue(engine, (asRepairStats + psTempl->asParts[COMP_REPAIRUNIT])->id), QScriptValue::ReadOnly);
	value.setProperty("ecm", WzStringToQScriptValue(engine, (asECMStats + psTempl->asParts[COMP_ECM])->id), QScriptValue::ReadOnly);
	value.setProperty("sensor", WzStringToQScriptValue(engine, (asSensorStats + psTempl->asParts[COMP_SENSOR])->id), QScriptValue::ReadOnly);
	value.setProperty("construct", WzStringToQScriptValue(engine, (asConstructStats + psTempl->asParts[COMP_CONSTRUCT])->id), QScriptValue::ReadOnly);
	QScriptValue weaponlist = engine->newArray(psTempl->numWeaps);
	for (int j = 0; j < psTempl->numWeaps; j++)
	{
		weaponlist.setProperty(j, WzStringToQScriptValue(engine, (asWeaponStats + psTempl->asWeaps[j])->id), QScriptValue::ReadOnly);
	}
	value.setProperty("weapons", weaponlist);
	return value;
}

QScriptValue convMax(const BASE_OBJECT *psObj, QScriptEngine *engine)
{
	if (!psObj)
	{
		return QScriptValue::NullValue;
	}
	switch (psObj->type)
	{
	case OBJ_DROID: return convDroid((const DROID *)psObj, engine);
	case OBJ_STRUCTURE: return convStructure((const STRUCTURE *)psObj, engine);
	case OBJ_FEATURE: return convFeature((const FEATURE *)psObj, engine);
	default: ASSERT(false, "No such supported object type"); return convObj(psObj, engine);
	}
}

BASE_OBJECT *IdToObject(OBJECT_TYPE type, int id, int player)
{
	switch (type)
	{
	case OBJ_DROID: return IdToDroid(id, player);
	case OBJ_FEATURE: return IdToFeature(id, player);
	case OBJ_STRUCTURE: return IdToStruct(id, player);
	default: return nullptr;
	}
}

// Call a function by name
static QScriptValue callFunction(QScriptEngine *engine, const QString &function, const QScriptValueList &args, bool event = true)
{
	if (event)
	{
		const auto instance = engineToInstanceMap.at(engine);
		// recurse into variants, if any
		for (const QString &s : instance->eventNamespaces)
		{
			const QScriptValue &value = engine->globalObject().property(s + function);
			if (value.isValid() && value.isFunction())
			{
				callFunction(engine, s + function, args, event);
			}
		}
	}
	code_part level = event ? LOG_SCRIPT : LOG_ERROR;
	QScriptValue value = engine->globalObject().property(function);
	if (!value.isValid() || !value.isFunction())
	{
		// not necessarily an error, may just be a trigger that is not defined (ie not needed)
		// or it could be a typo in the function name or ...
		debug(level, "called function (%s) not defined", function.toUtf8().constData());
		return false;
	}

	QScriptValue result;
	scripting_engine::instance().executeWithPerformanceMonitoring(engineToInstanceMap.at(engine), function.toStdString(), [&result, &value, &args](){
		result = value.call(QScriptValue(), args);
	});

	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		QStringList bt = engine->uncaughtExceptionBacktrace();
		for (int i = 0; i < bt.size(); i++)
		{
			debug(LOG_ERROR, "%d : %s", i, bt.at(i).toUtf8().constData());
		}
		ASSERT(false, "Uncaught exception calling function \"%s\" at line %d: %s",
		       function.toUtf8().constData(), line, result.toString().toUtf8().constData());
		engine->clearExceptions();
		return QScriptValue();
	}
	return result;
}

// ----------------------------------------------------------------------------------------

	class qtscript_execution_context : public wzapi::execution_context
	{
	private:
		QScriptContext *context = nullptr;
		QScriptEngine *engine = nullptr;
	public:
		qtscript_execution_context(QScriptContext *context, QScriptEngine *engine)
		: context(context), engine(engine)
		{ }
		~qtscript_execution_context() { }
	public:
		virtual wzapi::scripting_instance* currentInstance() const override
		{
			return engineToInstanceMap.at(engine);
		}

		virtual void throwError(const char *expr, int line, const char *function) const override
		{
			context->throwError(QScriptContext::ReferenceError, QString(expr) +  " failed in " + QString(function) + " at line " + QString::number(line));
		}

		virtual playerCallbackFunc getNamedScriptCallback(const WzString& func) const override
		{
			QScriptEngine *pEngine = engine;
			return [pEngine, func](const int player) {
				QScriptValueList args;
				args += QScriptValue(player);
				callFunction(pEngine, QString::fromUtf8(func.toUtf8().c_str()), args);
			};
		}

		virtual void doNotSaveGlobal(const std::string &name) const override
		{
			engineToInstanceMap.at(engine)->doNotSaveGlobal(name);
		}
	};

	/// Assert for scripts that give useful backtraces and other info.
	#define UNBOX_SCRIPT_ASSERT(context, expr, ...) \
		do { bool _wzeval = (expr); \
			if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
				context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed when converting argument " + QString::number(idx) + " for " + QString(function)); \
				break; } } while (0)

	namespace
	{
		template<typename T>
		struct unbox {
			//T operator()(size_t& idx, QScriptContext *context) const;
		};

		template<>
		struct unbox<int>
		{
			int operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toInt32();
			}
		};

		template<>
		struct unbox<unsigned int>
		{
			unsigned int operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toUInt32();
			}
		};

		template<>
		struct unbox<bool>
		{
			bool operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return 	context->argument(idx++).toBool();
			}
		};



		template<>
		struct unbox<float>
		{
			float operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toNumber();
			}
		};

		template<>
		struct unbox<double>
		{
			double operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toNumber();
			}
		};

		template<>
		struct unbox<DROID*>
		{
			DROID* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				QScriptValue droidVal = context->argument(idx++);
				int id = droidVal.property("id").toInt32();
				int player = droidVal.property("player").toInt32();
				DROID *psDroid = IdToDroid(id, player);
				UNBOX_SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
				return psDroid;
			}
		};

		template<>
		struct unbox<const DROID*>
		{
			const DROID* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				return unbox<DROID*>()(idx, context, engine, function);
			}
		};

		template<>
		struct unbox<STRUCTURE*>
		{
			STRUCTURE* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				QScriptValue structVal = context->argument(idx++);
				int id = structVal.property("id").toInt32();
				int player = structVal.property("player").toInt32();
				STRUCTURE *psStruct = IdToStruct(id, player);
				UNBOX_SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
				return psStruct;
			}
		};

		template<>
		struct unbox<const STRUCTURE*>
		{
			const STRUCTURE* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				return unbox<STRUCTURE*>()(idx, context, engine, function);
			}
		};

		template<>
		struct unbox<BASE_OBJECT*>
		{
			BASE_OBJECT* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				QScriptValue objVal = context->argument(idx++);
				int oid = objVal.property("id").toInt32();
				int oplayer = objVal.property("player").toInt32();
				OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
				BASE_OBJECT* psObj = IdToObject(otype, oid, oplayer);
				UNBOX_SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
				return psObj;
			}
		};

		template<>
		struct unbox<const BASE_OBJECT*>
		{
			const BASE_OBJECT* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				return unbox<BASE_OBJECT*>()(idx, context, engine, function);
			}
		};

		template<>
		struct unbox<std::string>
		{
			std::string operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toString().toStdString();
			}
		};

		template<>
		struct unbox<wzapi::STRUCTURE_TYPE_or_statsName_string>
		{
			wzapi::STRUCTURE_TYPE_or_statsName_string operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				wzapi::STRUCTURE_TYPE_or_statsName_string result;
				if (context->argumentCount() <= idx)
					return result;
				QScriptValue val = context->argument(idx++);
				if (val.isNumber())
				{
					result.type = (STRUCTURE_TYPE)val.toInt32();
				}
				else
				{
					result.statsName = val.toString().toStdString();
				}
				return result;
			}
		};

		template<typename OptionalType>
		struct unbox<optional<OptionalType>>
		{
			optional<OptionalType> operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return optional<OptionalType>(unbox<OptionalType>()(idx, context, engine, function));
			}
		};

		template<>
		struct unbox<wzapi::reservedParam>
		{
			wzapi::reservedParam operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				// just ignore parameter value, and increment idx
				idx++;
				return {};
			}
		};

		template<>
		struct unbox<wzapi::game_object_identifier>
		{
			wzapi::game_object_identifier operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() < idx)
					return {};
				QScriptValue objVal = context->argument(idx++);
				wzapi::game_object_identifier result;
				result.id = objVal.property("id").toInt32();
				result.player = objVal.property("player").toInt32();
				result.type = objVal.property("type").toInt32();
				return result;
			}
		};

		template<>
		struct unbox<wzapi::string_or_string_list>
		{
			wzapi::string_or_string_list operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				wzapi::string_or_string_list strings;

				QScriptValue list_or_string = context->argument(idx++);
				if (list_or_string.isArray())
				{
					int length = list_or_string.property("length").toInt32();
					for (int k = 0; k < length; k++)
					{
						QString resName = list_or_string.property(k).toString();
						strings.strings.push_back(resName.toStdString());
					}
				}
				else
				{
					QString resName = list_or_string.toString();
					strings.strings.push_back(resName.toStdString());
				}
				return strings;
			}
		};

		template<>
		struct unbox<wzapi::va_list_treat_as_strings>
		{
			wzapi::va_list_treat_as_strings operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				wzapi::va_list_treat_as_strings strings;
				for (; idx < context->argumentCount(); idx++)
				{
					QString s = context->argument(idx).toString();
					if (context->state() == QScriptContext::ExceptionState)
					{
						break;
					}
					strings.strings.push_back(s.toStdString());
				}
				return strings;
			}
		};

		template<typename ContainedType>
		struct unbox<wzapi::va_list<ContainedType>>
		{
			wzapi::va_list<ContainedType> operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				wzapi::va_list<ContainedType> result;
				for (; idx < context->argumentCount(); idx++)
				{
					result.va_list.push_back(unbox<ContainedType>()(idx, context, engine, function));
				}
				return result;
			}
		};

		template<>
		struct unbox<scripting_engine::area_by_values_or_area_label_lookup>
		{
			scripting_engine::area_by_values_or_area_label_lookup operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};

				if (context->argument(idx).isString())
				{
					std::string label = context->argument(idx).toString().toStdString();
					idx += 1;
					return scripting_engine::area_by_values_or_area_label_lookup(label);
				}
				else if ((context->argumentCount() - idx) >= 4)
				{
					int x1, y1, x2, y2;
					x1 = context->argument(idx).toInt32();
					y1 = context->argument(idx + 1).toInt32();
					x2 = context->argument(idx + 2).toInt32();
					y2 = context->argument(idx + 3).toInt32();
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
			generic_script_object operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};

				QScriptValue qval = context->argument(idx++);
				int type = qval.property("type").toInt32();

				QScriptValue triggered = qval.property("triggered");
				if (triggered.isNumber())
				{
//					UNBOX_SCRIPT_ASSERT(context, type != SCRIPT_POSITION, "Cannot assign a trigger to a position");
					ASSERT(false, "Not currently handling triggered property - does anything use this?");
				}

				if (type == SCRIPT_RADIUS)
				{
					return generic_script_object::fromRadius(
						qval.property("x").toInt32(),
						qval.property("y").toInt32(),
						qval.property("radius").toInt32()
					);
				}
				else if (type == SCRIPT_AREA)
				{
					return generic_script_object::fromArea(
						qval.property("x").toInt32(),
						qval.property("y").toInt32(),
						qval.property("x2").toInt32(),
						qval.property("y2").toInt32()
					);
				}
				else if (type == SCRIPT_POSITION)
				{
					return generic_script_object::fromPosition(
						qval.property("x").toInt32(),
						qval.property("y").toInt32()
					);
				}
				else if (type == SCRIPT_GROUP)
				{
					return generic_script_object::fromGroup(
						qval.property("id").toInt32()
					);
				}
				else if (type == OBJ_DROID || type == OBJ_STRUCTURE || type == OBJ_FEATURE)
				{
					int id = qval.property("id").toInt32();
					int player = qval.property("player").toInt32();
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
			wzapi::object_request operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};

				if ((context->argumentCount() - idx) >= 3) // get by ID case (3 parameters)
				{
					OBJECT_TYPE type = (OBJECT_TYPE)context->argument(idx++).toInt32();
					int player = context->argument(idx++).toInt32();
					int id = context->argument(idx++).toInt32();
					return wzapi::object_request(type, player, id);
				}
				else if ((context->argumentCount() - idx) >= 2) // get at position case (2 parameters)
				{
					int x = context->argument(idx++).toInt32();
					int y = context->argument(idx++).toInt32();
					return wzapi::object_request(x, y);
				}
				else
				{
					// get by label case (1 parameter)
					std::string label = context->argument(idx++).toString().toStdString();
					return wzapi::object_request(label);
				}
			}
		};

		template<>
		struct unbox<wzapi::label_or_position_values>
		{
			wzapi::label_or_position_values operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if ((context->argumentCount() - idx) >= 4) // square area
				{
					int x1 = context->argument(idx++).toInt32();
					int y1 = context->argument(idx++).toInt32();
					int x2 = context->argument(idx++).toInt32();
					int y2 = context->argument(idx++).toInt32();
					return wzapi::label_or_position_values(x1, y1, x2, y2);
				}
				else if ((context->argumentCount() - idx) >= 2) // single tile
				{
					int x = context->argument(idx++).toInt32();
					int y = context->argument(idx++).toInt32();
					return wzapi::label_or_position_values(x, y);
				}
				else if ((context->argumentCount() - idx) >= 1) // label
				{
					std::string label = context->argument(idx++).toString().toStdString();
					return wzapi::label_or_position_values(label);
				}
				return wzapi::label_or_position_values();
			}
		};

		template<typename T>
		QScriptValue box(T a, QScriptEngine*)
		{
			return QScriptValue(a);
		}

		QScriptValue box(const char* str, QScriptEngine* engine)
		{
			// The redundant QString cast is a workaround for a Qt5 bug, the QScriptValue(char const *) constructor interprets as Latin1 instead of UTF-8!
			return QScriptValue(QString::fromUtf8(str));
		}

		QScriptValue box(std::string str, QScriptEngine* engine)
		{
			// The redundant QString cast is a workaround for a Qt5 bug, the QScriptValue(char const *) constructor interprets as Latin1 instead of UTF-8!
			return QScriptValue(QString::fromUtf8(str.c_str()));
		}

		QScriptValue box(wzapi::no_return_value, QScriptEngine* engine)
		{
			return QScriptValue();
		}

		QScriptValue box(const BASE_OBJECT * psObj, QScriptEngine* engine)
		{
			if (!psObj)
			{
				return QScriptValue::NullValue;
			}
			return convMax(psObj, engine);
		}

		QScriptValue box(const STRUCTURE * psStruct, QScriptEngine* engine)
		{
			if (!psStruct)
			{
				return QScriptValue::NullValue;
			}
			return convStructure(psStruct, engine);
		}

		QScriptValue box(const DROID * psDroid, QScriptEngine* engine)
		{
			if (!psDroid)
			{
				return QScriptValue::NullValue;
			}
			return convDroid(psDroid, engine);
		}

		QScriptValue box(const FEATURE * psFeat, QScriptEngine* engine)
		{
			if (!psFeat)
			{
				return QScriptValue::NullValue;
			}
			return convFeature(psFeat, engine);
		}

		QScriptValue box(const DROID_TEMPLATE * psTemplate, QScriptEngine* engine)
		{
			if (!psTemplate)
			{
				return QScriptValue::NullValue;
			}
			return convTemplate(psTemplate, engine);
		}

//		// deliberately not defined
//		// use `wzapi::researchResult` instead of `const RESEARCH *`
//		template<>
//		QScriptValue box(const RESEARCH * psResearch, QScriptEngine* engine);

		QScriptValue box(const scr_radius& r, QScriptEngine* engine)
		{
			QScriptValue ret = engine->newObject();
			ret.setProperty("type", SCRIPT_RADIUS, QScriptValue::ReadOnly);
			ret.setProperty("x", r.x, QScriptValue::ReadOnly);
			ret.setProperty("y", r.y, QScriptValue::ReadOnly);
			ret.setProperty("radius", r.radius, QScriptValue::ReadOnly);
			return ret;
		}

		QScriptValue box(const scr_area& r, QScriptEngine* engine)
		{
			QScriptValue ret = engine->newObject();
			ret.setProperty("type", SCRIPT_AREA, QScriptValue::ReadOnly);
			ret.setProperty("x", r.x1, QScriptValue::ReadOnly); // TODO: Rename scr_area x1 to x
			ret.setProperty("y", r.y1, QScriptValue::ReadOnly);
			ret.setProperty("x2", r.x2, QScriptValue::ReadOnly);
			ret.setProperty("y2", r.y2, QScriptValue::ReadOnly);
			return ret;
		}

		QScriptValue box(const scr_position& p, QScriptEngine* engine)
		{
			QScriptValue ret = engine->newObject();
			ret.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly);
			ret.setProperty("x", p.x, QScriptValue::ReadOnly);
			ret.setProperty("y", p.y, QScriptValue::ReadOnly);
			return ret;
		}

		QScriptValue box(const generic_script_object& p, QScriptEngine* engine)
		{
			int type = p.getType();
			switch (type)
			{
			case SCRIPT_RADIUS:
				return box(p.getRadius(), engine);
				break;
			case SCRIPT_AREA:
				return box(p.getArea(), engine);
				break;
			case SCRIPT_POSITION:
				return box(p.getPosition(), engine);
				break;
			case SCRIPT_GROUP:
			{
				QScriptValue ret = engine->newObject();
				ret.setProperty("type", type, QScriptValue::ReadOnly);
				ret.setProperty("id", p.getGroupId(), QScriptValue::ReadOnly);
				return ret;
			}
				break;
			case OBJ_DROID:
			case OBJ_FEATURE:
			case OBJ_STRUCTURE:
			{
				BASE_OBJECT* psObj = p.getObject();
				return convMax(psObj, engine);
			}
				break;
			default:
				if (p.isNull())
				{
					return QScriptValue::NullValue;
				}
				debug(LOG_SCRIPT, "Unsupported object label type: %d", type);
				break;
			}

			return QScriptValue::NullValue;
		}

		QScriptValue box(GATEWAY* psGateway, QScriptEngine* engine)
		{
			QScriptValue v = engine->newObject();
			v.setProperty("x1", psGateway->x1, QScriptValue::ReadOnly);
			v.setProperty("y1", psGateway->y1, QScriptValue::ReadOnly);
			v.setProperty("x2", psGateway->x2, QScriptValue::ReadOnly);
			v.setProperty("y2", psGateway->y2, QScriptValue::ReadOnly);
			return v;
		}

		template<typename VectorType>
		QScriptValue box(const std::vector<VectorType>& value, QScriptEngine* engine)
		{
			QScriptValue result = engine->newArray(value.size());
			for (int i = 0; i < value.size(); i++)
			{
				VectorType item = value.at(i);
				result.setProperty(i, box(item, engine));
			}
			return result;
		}

		template<typename ContainedType>
		QScriptValue box(const std::list<ContainedType>& value, QScriptEngine* engine)
		{
			QScriptValue result = engine->newArray(value.size());
			size_t i = 0;
			for (auto item : value)
			{
				result.setProperty(i, box(item, engine));
				i++;
			}
			return result;
		}

		QScriptValue box(const wzapi::researchResult& result, QScriptEngine* engine)
		{
			if (result.psResearch == nullptr)
			{
				return QScriptValue::NullValue;
			}
			return convResearch(result.psResearch, engine, result.player);
		}

		QScriptValue box(const wzapi::researchResults& results, QScriptEngine* engine)
		{
			QScriptValue result = engine->newArray(results.resList.size());
			for (int i = 0; i < results.resList.size(); i++)
			{
				const RESEARCH *psResearch = results.resList.at(i);
				result.setProperty(i, convResearch(psResearch, engine, results.player));
			}
			return result;
		}

		template<typename OptionalType>
		QScriptValue box(const optional<OptionalType>& result, QScriptEngine* engine)
		{
			if (result.has_value())
			{
				return box(result.value(), engine);
			}
			else
			{
				return QScriptValue(); // "undefined" variable
				//return QScriptValue::NullValue;
			}
		}

		template<typename PtrType>
		QScriptValue box(std::unique_ptr<PtrType> result, QScriptEngine* engine)
		{
			if (result)
			{
				return box(result.get(), engine);
			}
			else
			{
				return QScriptValue::NullValue;
			}
		}

		QScriptValue box(nlohmann::json results, QScriptEngine* engine)
		{
			return mapJsonToQScriptValue(engine, results, QScriptValue::PropertyFlags());
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
		struct UnboxTupleIndex<tl::index_sequence<I...>, T...>
		{
		public:
			typedef std::tuple<const wzapi::execution_context&, T...> tuple_type;
			typedef std::tuple<const wzapi::execution_context&, T&...> tuple_ref_type;

			UnboxTupleIndex(const wzapi::execution_context& execution_context, size_t& idx, QScriptContext *ctx, QScriptEngine *engine, const char *function)
				: impl(idx, ctx, engine, function),
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
				UnboxedValueWrapper(size_t& idx, WZ_DECL_UNUSED QScriptContext *ctx, WZ_DECL_UNUSED QScriptEngine *engine, WZ_DECL_UNUSED const char *function)
					: value(unbox<UnboxedType>{}(idx, ctx, engine, function))
				{ }
			};

			// The UnboxedImpl class derives from all of the value wrappers
			// (Base classes are guaranteed to be constructed in-order)
			struct UnboxedImpl : UnboxedValueWrapper<I, T>...
			{
				UnboxedImpl(size_t& idx, WZ_DECL_UNUSED QScriptContext *ctx, WZ_DECL_UNUSED QScriptEngine *engine, WZ_DECL_UNUSED const char *function)
					: UnboxedValueWrapper<I, T>(idx, ctx, engine, function)...
				{}
			};

			UnboxedImpl impl;
			tuple_ref_type value;
		};

		template<typename...T> using UnboxTuple = UnboxTupleIndex<tl::make_index_sequence<sizeof...(T)>, T...>;

		MSVC_PRAGMA(warning( push )) // see matching "pop" below
		MSVC_PRAGMA(warning( disable : 4189 )) // disable "warning C4189: 'idx': local variable is initialized but not referenced"
		
		template<typename R, typename...Args>
		QScriptValue wrap__(R(*f)(const wzapi::execution_context&, Args...), WZ_DECL_UNUSED const char *wrappedFunctionName, QScriptContext *context, QScriptEngine *engine)
		{
			size_t idx WZ_DECL_UNUSED = 0; // unused when Args... is empty
			qtscript_execution_context execution_context(context, engine);
			return box(apply(f, UnboxTuple<Args...>(execution_context, idx, context, engine, wrappedFunctionName)()), engine);
		}

		template<typename R, typename...Args>
		QScriptValue wrap__(R(*f)(), WZ_DECL_UNUSED const char *wrappedFunctionName, QScriptContext *context, QScriptEngine *engine)
		{
			return box(f(), engine);
		}

		MSVC_PRAGMA(warning( pop ))

		#define wrap_(wzapi_function, context, engine) \
		wrap__(wzapi_function, #wzapi_function, context, engine)

		#define JS_FUNC_IMPL_NAME(func_name) js_##func_name

		#define IMPL_JS_FUNC(func_name, wrapped_func) \
			static QScriptValue JS_FUNC_IMPL_NAME(func_name)(QScriptContext *context, QScriptEngine *engine) \
			{ \
				return wrap_(wrapped_func, context, engine); \
			}

		#define IMPL_JS_FUNC_DEBUGMSGUPDATE(func_name, wrapped_func) \
			static QScriptValue JS_FUNC_IMPL_NAME(func_name)(QScriptContext *context, QScriptEngine *engine) \
			{ \
				QScriptValue retVal = wrap_(wrapped_func, context, engine); \
				jsDebugMessageUpdate(); \
				return retVal; \
			}

		template <typename T>
		void append_value_list(QScriptValueList &list, T t, QScriptEngine* engine) { list += box(std::forward<T>(t), engine); }

		template <typename... Args>
		bool wrap_event_handler__(const std::string &functionName, QScriptEngine* engine, Args&&... args)
		{
			QScriptValueList args_list;
			using expander = int[];
//			WZ_DECL_UNUSED int dummy[] = { 0, ((void) append_value_list(args_list, std::forward<Args>(args), engine),0)... };
			// Left-most void to avoid `expression result unused [-Wunused-value]`
			(void)expander{ 0, ((void) append_value_list(args_list, std::forward<Args>(args), engine),0)... };
			/*QScriptValue result =*/ callFunction(engine, QString::fromStdString(functionName), args_list);
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
			bool qtscript_scripting_instance::handle_##fun(MAKE_PARAMS(__VA_ARGS__)) { \
				return wrap_event_handler__(STRINGIFY(fun), engine, MAKE_ARGS(__VA_ARGS__)); \
			}

		#define IMPL_EVENT_HANDLER_NO_PARAMS(fun) \
		bool qtscript_scripting_instance::handle_##fun() { \
			return wrap_event_handler__(STRINGIFY(fun), engine); \
		}

	}

// Wraps a QScriptEngine instance

//-- ## profile(function[, arguments])
//-- Calls a function with given arguments, measures time it took to evaluate the function,
//-- and adds this time to performance monitor statistics. Transparently returns the
//-- function's return value. The function to run is the first parameter, and it
//-- _must be quoted_. (3.2+ only)
//--
static QScriptValue js_profile(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Profiled functions must be quoted");
	QString funcName = context->argument(0).toString();
	QScriptValueList args;
	for (int i = 1; i < context->argumentCount(); ++i)
	{
		args.push_back(context->argument(i));
	}
	return callFunction(engine, funcName, args);
}

//-- ## include(file)
//-- Includes another source code file at this point. You should generally only specify the filename,
//-- not try to specify its path, here.
//--
static QScriptValue js_include(QScriptContext *context, QScriptEngine *engine)
{
	QString basePath = engine->globalObject().property("scriptPath").toString();
	QFileInfo basename(context->argument(0).toString());
	QString path = basePath + "/" + basename.fileName();
	// allow users to use subdirectories too
	if (PHYSFS_exists(basename.filePath().toUtf8().constData()))
	{
		path = basename.filePath(); // use this path instead (from read-only dir)
	}
	else if (PHYSFS_exists(QString("scripts/" + basename.filePath()).toUtf8().constData()))
	{
		path = "scripts/" + basename.filePath(); // use this path instead (in user write dir)
	}
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().constData(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read include file \"%s\" (path=%s, name=%s)",
		      path.toUtf8().constData(), basePath.toUtf8().constData(), basename.filePath().toUtf8().constData());
		return QScriptValue(false);
	}
	QString source = QString::fromUtf8(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in include %s line %d: %s",
		      path.toUtf8().constData(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
		return QScriptValue(false);
	}
	context->setActivationObject(engine->globalObject());
	context->setThisObject(engine->globalObject());
	QScriptValue result = engine->evaluate(source, path);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Uncaught exception at line %d, include file %s: %s",
		      line, path.toUtf8().constData(), result.toString().toUtf8().constData());
		return QScriptValue(false);
	}
	debug(LOG_SCRIPT, "Included new script file %s", path.toUtf8().constData());
	return QScriptValue(true);
}

class qtscript_timer_additionaldata : public timerAdditionalData
{
public:
	QString stringArg;
	qtscript_timer_additionaldata(const QString& _stringArg)
	: stringArg(_stringArg)
	{ }
};

//-- ## setTimer(function, milliseconds[, object])
//--
//-- Set a function to run repeated at some given time interval. The function to run
//-- is the first parameter, and it _must be quoted_, otherwise the function will
//-- be inlined. The second parameter is the interval, in milliseconds. A third, optional
//-- parameter can be a **game object** to pass to the timer function. If the **game object**
//-- dies, the timer stops running. The minimum number of milliseconds is 100, but such
//-- fast timers are strongly discouraged as they may deteriorate the game performance.
//--
//-- ```javascript
//--   function conDroids()
//--   {
//--      ... do stuff ...
//--   }
//--   // call conDroids every 4 seconds
//--   setTimer("conDroids", 4000);
//-- ```
//--
static QScriptValue js_setTimer(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Timer functions must be quoted");
	QString funcName = context->argument(0).toString();
	QScriptValue ms = context->argument(1);
	int player = engine->globalObject().property("me").toInt32();

	QScriptValue value = engine->globalObject().property(funcName); // check existence
	SCRIPT_ASSERT(context, value.isValid() && value.isFunction(), "No such function: %s",
	              funcName.toUtf8().constData());

	QString stringArg;
	BASE_OBJECT *psObj = nullptr;
	if (context->argumentCount() == 3)
	{
		QScriptValue obj = context->argument(2);
		if (obj.isString())
		{
			stringArg = obj.toString();
		}
		else // is game object
		{
			int baseobj = obj.property("id").toInt32();
			OBJECT_TYPE baseobjtype = (OBJECT_TYPE)obj.property("type").toInt32();
			psObj = IdToObject(baseobjtype, baseobj, player);
		}
	}

	scripting_engine::instance().setTimer(engineToInstanceMap.at(engine)
	  // timerFunc
	, [engine, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
		qtscript_timer_additionaldata* pData = static_cast<qtscript_timer_additionaldata*>(additionalParams);
		QScriptValueList args;
		if (baseObject != nullptr)
		{
			args += convMax(baseObject, engine);
		}
		else if (pData && !(pData->stringArg.isEmpty()))
		{
			args += pData->stringArg;
		}
		callFunction(engine, funcName, args, true);
	}
	, player, ms.toInt32(), funcName.toStdString(), psObj, TIMER_REPEAT
	// additionalParams
	, new qtscript_timer_additionaldata(stringArg));

	return QScriptValue();
}

//-- ## removeTimer(function)
//--
//-- Removes an existing timer. The first parameter is the function timer to remove,
//-- and its name _must be quoted_.
//--
static QScriptValue js_removeTimer(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Timer functions must be quoted");
	std::string function = context->argument(0).toString().toStdString();
	int player = engine->globalObject().property("me").toInt32();
	std::vector<uniqueTimerID> removedTimerIDs = scripting_engine::instance().removeTimersIf(
		[engine, function, player](const scripting_engine::timerNode& node)
	{
		return (node.instance == engineToInstanceMap.at(engine)) && (node.timerName == function) && (node.player == player);
	});
	if (removedTimerIDs.empty())
	{
		// Friendly warning
		QString warnName = QString::fromStdString(function).left(15) + "...";
		debug(LOG_ERROR, "Did not find timer %s to remove", warnName.toUtf8().constData());
	}
	return QScriptValue();
}

//-- ## queue(function[, milliseconds[, object]])
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
static QScriptValue js_queue(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Queued functions must be quoted");
	QString funcName = context->argument(0).toString();
	QScriptValue value = engine->globalObject().property(funcName); // check existence
	SCRIPT_ASSERT(context, value.isValid() && value.isFunction(), "No such function: %s",
	              funcName.toUtf8().constData());
	int ms = 0;
	if (context->argumentCount() > 1)
	{
		ms = context->argument(1).toInt32();
	}
	int player = engine->globalObject().property("me").toInt32();

	QString stringArg;
	BASE_OBJECT *psObj = nullptr;
	if (context->argumentCount() == 3)
	{
		QScriptValue obj = context->argument(2);
		if (obj.isString())
		{
			stringArg = obj.toString();
		}
		else // is game object
		{
			int baseobj = obj.property("id").toInt32();
			OBJECT_TYPE baseobjtype = (OBJECT_TYPE)obj.property("type").toInt32();
			psObj = IdToObject(baseobjtype, baseobj, player);
		}
	}

	scripting_engine::instance().setTimer(engineToInstanceMap.at(engine)
	  // timerFunc
	, [engine, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
		qtscript_timer_additionaldata* pData = static_cast<qtscript_timer_additionaldata*>(additionalParams);
		QScriptValueList args;
		if (baseObject != nullptr)
		{
			args += convMax(baseObject, engine);
		}
		else if (pData && !(pData->stringArg.isEmpty()))
		{
			args += pData->stringArg;
		}
		callFunction(engine, funcName, args, true);
	}
	, player, ms, funcName.toStdString(), psObj, TIMER_ONESHOT_READY
	// additionalParams
	, new qtscript_timer_additionaldata(stringArg));
	return QScriptValue();
}

//-- ## namespace(prefix)
//-- Registers a new event namespace. All events can now have this prefix. This is useful for
//-- code libraries, to implement event that do not conflict with events in main code. This
//-- function should be called from global; do not (for hopefully obvious reasons) put it
//-- inside an event.
//--
static QScriptValue js_namespace(QScriptContext *context, QScriptEngine *engine)
{
	QString prefix(context->argument(0).toString());
	engineToInstanceMap.at(engine)->eventNamespaces.append(prefix);
	return QScriptValue(true);
}

static QScriptValue debugGetCallerFuncObject(QScriptContext *context, QScriptEngine *engine)
{
	QScriptContext *psContext = context;
	size_t levels = 0;
	for(; levels < 2; psContext = psContext->parentContext())
	{
		if (psContext == NULL)
		{
			return context->throwError(QScriptContext::ReferenceError, "Unable to get caller function object");
		}
		levels++;
	}
	return psContext->callee();
}

//-- ## debugGetCallerFuncName()
//-- Returns the function name of the caller of the current context as a string (if available).
//-- ex.
//-- ```javascript
//--   function FuncA() {
//--     var callerFuncName = debugGetCallerFuncName();
//--     debug(callerFuncName);
//--   }
//--   function FuncB() {
//--     FuncA();
//--   }
//--   FuncB();
//-- ```
//-- Will output: "FuncB"
//-- Useful for debug logging.
//--
static QScriptValue debugGetCallerFuncName(QScriptContext *context, QScriptEngine *engine)
{
	return debugGetCallerFuncObject(context, engine).property("name").toString();
}

static QScriptValue debugGetBacktrace(QScriptContext *context, QScriptEngine *engine)
{
	QStringList list;
	QScriptContext *psContext = context;
	while (psContext)
	{
		QScriptContextInfo contextInfo(psContext);
		QScriptValue name = contextInfo.functionName();
		if (name.isValid() && name.isString())
		{
			list.append(name.toString());
		}
		else
		{
			list.append("<anonymous>");
		}
		psContext = psContext->parentContext();
	}
	return qScriptValueFromSequence(engine, list);
}

ScriptMapData runMapScript_QtScript(WzString const &path, uint64_t seed, bool preview)
{
	ScriptMapData data;
	data.valid = false;
	data.mt = MersenneTwister(seed);

	auto engine = std::unique_ptr<QScriptEngine>(new QScriptEngine());
	//auto engine = std::make_unique<QScriptEngine>();
	engine->setProcessEventsInterval(-1);
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().c_str(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toUtf8().c_str());
		return data;
	}
	QString source = QString::fromUtf8(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	ASSERT_OR_RETURN(data, syntax.state() == QScriptSyntaxCheckResult::Valid, "Syntax error in %s line %d: %s",
	                 path.toUtf8().c_str(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
	engine->globalObject().setProperty("preview", preview, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("XFLIP", TILE_XFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("YFLIP", TILE_YFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ROTMASK", TILE_ROTMASK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ROTSHIFT", TILE_ROTSHIFT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("TRIFLIP", TILE_TRIFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//engine->globalObject().setProperty("players", players, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("gameRand", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
		auto &data = *(ScriptMapData *)_data;
		uint32_t num = data.mt.u32();
		uint32_t mod = context->argument(0).toUInt32();
		return mod? num%mod : num;
	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("log", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
		auto &data = *(ScriptMapData *)_data;
		(void)data;
		auto str = context->argument(0).toString();
		debug(LOG_INFO, "game.js: \"%s\"", str.toUtf8().constData());
		return {};
	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("setMapData", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
		auto &data = *(ScriptMapData *)_data;
		data.valid = false;
		auto mapWidth = context->argument(0);
		auto mapHeight = context->argument(1);
		auto texture = context->argument(2);
		auto height = context->argument(3);
		auto structures = context->argument(4);
		auto droids = context->argument(5);
		auto features = context->argument(6);
		ASSERT_OR_RETURN(false, mapWidth.isNumber(), "mapWidth must be number");
		ASSERT_OR_RETURN(false, mapHeight.isNumber(), "mapHeight must be number");
		ASSERT_OR_RETURN(false, texture.isArray(), "texture must be array");
		ASSERT_OR_RETURN(false, height.isArray(), "height must be array");
		ASSERT_OR_RETURN(false, structures.isArray(), "structures must be array");
		ASSERT_OR_RETURN(false, droids.isArray(), "droids must be array");
		ASSERT_OR_RETURN(false, features.isArray(), "features must be array");
		data.mapWidth = mapWidth.toInt32();
		data.mapHeight = mapHeight.toInt32();
		ASSERT_OR_RETURN(false, data.mapWidth > 1 && data.mapHeight > 1 && (uint64_t)data.mapWidth*data.mapHeight <= 65536, "Map size out of bounds");
		size_t N = (size_t)data.mapWidth*data.mapHeight;
		data.texture.resize(N);
		data.height.resize(N);
		for (size_t n = 0; n < N; ++n)
		{
			data.texture[n] = texture.property(n).toUInt16();
			data.height[n] = height.property(n).toInt32();
		}
		uint16_t structureCount = structures.property("length").toUInt16();
		for (unsigned i = 0; i < structureCount; ++i) {
			auto structure = structures.property(i);
			auto position = structure.property("position");
			ScriptMapData::Structure sd;
			sd.name = structure.property("name").toString().toUtf8().constData();
			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
			sd.direction = structure.property("direction").toInt32();
			sd.modules = structure.property("modules").toUInt32();
			sd.player = structure.property("player").toInt32();
			if (sd.player < -1 || sd.player >= MAX_PLAYERS) {
				ASSERT(false, "Invalid player");
				continue;
			}
			data.structures.push_back(std::move(sd));
		}
		uint16_t droidCount = droids.property("length").toUInt16();
		for (unsigned i = 0; i < droidCount; ++i) {
			auto droid = droids.property(i);
			auto position = droid.property("position");
			ScriptMapData::Droid sd;
			sd.name = droid.property("name").toString().toUtf8().constData();
			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
			sd.direction = droid.property("direction").toInt32();
			sd.player = droid.property("player").toInt32();
			if (sd.player < -1 || sd.player >= MAX_PLAYERS) {
				ASSERT(false, "Invalid player");
				continue;
			}
			data.droids.push_back(std::move(sd));
		}
		uint16_t featureCount = features.property("length").toUInt16();
		for (unsigned i = 0; i < featureCount; ++i) {
			auto feature = features.property(i);
			auto position = feature.property("position");
			ScriptMapData::Feature sd;
			sd.name = feature.property("name").toString().toUtf8().constData();
			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
			sd.direction = feature.property("direction").toInt32();
			data.features.push_back(std::move(sd));
		}
		data.valid = true;
		return true;
	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	QScriptValue result = engine->evaluate(source, QString::fromUtf8(path.toUtf8().c_str()));
	ASSERT_OR_RETURN(data, !engine->hasUncaughtException(), "Uncaught exception at line %d, file %s: %s",
	                 engine->uncaughtExceptionLineNumber(), path.toUtf8().c_str(), result.toString().toUtf8().constData());

	return data;
}

wzapi::scripting_instance* createQtScriptInstance(const WzString& path, int player, int difficulty)
{
	QFileInfo basename(QString::fromUtf8(path.toUtf8().c_str()));
	qtscript_scripting_instance* pNewInstance = new qtscript_scripting_instance(player, basename.baseName().toStdString());
	if (!pNewInstance->loadScript(path, player, difficulty))
	{
		delete pNewInstance;
		return nullptr;
	}
	return pNewInstance;
}

bool qtscript_scripting_instance::loadScript(const WzString& path, int player, int difficulty)
{
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().c_str(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toUtf8().c_str());
		return false;
	}
	/*QString*/ m_source = QString::fromUtf8(bytes, size);
	m_path = QString::fromUtf8(path.toUtf8().c_str());
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(m_source);
	ASSERT_OR_RETURN(false, syntax.state() == QScriptSyntaxCheckResult::Valid, "Syntax error in %s line %d: %s",
					 path.toUtf8().c_str(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
	// Special functions
	engine->globalObject().setProperty("setTimer", engine->newFunction(js_setTimer)); // JS-specific implementation
	engine->globalObject().setProperty("queue", engine->newFunction(js_queue)); // JS-specific implementation
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer)); // JS-specific implementation
	engine->globalObject().setProperty("profile", engine->newFunction(js_profile)); // JS-specific implementation
	engine->globalObject().setProperty("include", engine->newFunction(js_include)); // backend-specific (a scripting_instance can't directly include a different type of script)
	engine->globalObject().setProperty("namespace", engine->newFunction(js_namespace)); // JS-specific implementation
	engine->globalObject().setProperty("debugGetCallerFuncObject", engine->newFunction(debugGetCallerFuncObject)); // backend-specific
	engine->globalObject().setProperty("debugGetCallerFuncName", engine->newFunction(debugGetCallerFuncName)); // backend-specific
	engine->globalObject().setProperty("debugGetBacktrace", engine->newFunction(debugGetBacktrace)); // backend-specific

	// Regular functions
	QFileInfo basename(QString::fromUtf8(path.toUtf8().c_str()));
	registerFunctions(basename.baseName());

	// Remember internal, reserved names
	QScriptValueIterator it(engine->globalObject());
	while (it.hasNext())
	{
		it.next();
		internalNamespace.insert(it.name().toStdString());
	}

	return true;
}

bool qtscript_scripting_instance::readyInstanceForExecution()
{
	QScriptValue result = engine->evaluate(m_source, m_path);
	ASSERT_OR_RETURN(false, !engine->hasUncaughtException(), "Uncaught exception at line %d, file %s: %s",
					 engine->uncaughtExceptionLineNumber(), m_path.toUtf8().constData(), result.toString().toUtf8().constData());

	return true;
}

bool qtscript_scripting_instance::saveScriptGlobals(nlohmann::json &result)
{
	QScriptValueIterator it(engine->globalObject());
	// we save 'scriptName' and 'me' implicitly
	while (it.hasNext())
	{
		it.next();
		std::string nameStr = it.name().toStdString();
		if (internalNamespace.count(nameStr) == 0 && !it.value().isFunction()
			&& !it.value().equals(engine->globalObject()))
		{
			result[nameStr] = it.value().toVariant();
		}
	}
	return true;
}

bool qtscript_scripting_instance::loadScriptGlobals(const nlohmann::json &result)
{
	ASSERT_OR_RETURN(false, result.is_object(), "Can't load script globals from non-json-object");
	for (auto it : result.items())
	{
		// IMPORTANT: "null" JSON values *MUST* map to QScriptValue::UndefinedValue.
		//			  If they are set to QScriptValue::NullValue, it causes issues for libcampaign.js. (As the values become "defined".)
		//			  (mapJsonToQScriptValue handles this properly.)
		engine->globalObject().setProperty(QString::fromStdString(it.key()), mapJsonToQScriptValue(engine, it.value(), QScriptValue::PropertyFlags()));
	}
	return true;
}

nlohmann::json qtscript_scripting_instance::saveTimerFunction(uniqueTimerID timerID, std::string timerName, timerAdditionalData* additionalParam)
{
	nlohmann::json result = nlohmann::json::object();
	result["function"] = timerName;
	qtscript_timer_additionaldata* pData = static_cast<qtscript_timer_additionaldata*>(additionalParam);
	if (pData)
	{
		result["stringArg"] = (pData->stringArg).toStdString();
	}
	return result;
}

// recreates timer functions (and additional userdata) based on the information saved by the saveTimer() method
std::tuple<TimerFunc, timerAdditionalData *> qtscript_scripting_instance::restoreTimerFunction(const nlohmann::json& savedTimerFuncData)
{
	QString funcName = QString::fromStdString(json_getValue(savedTimerFuncData, WzString::fromUtf8("function")).toWzString().toStdString());
	if (funcName.isEmpty())
	{
		throw std::runtime_error("Invalid timer restore data");
	}
	QString stringArg = QString::fromStdString(json_getValue(savedTimerFuncData, WzString::fromUtf8("stringArg")).toWzString().toStdString());

	QScriptEngine* pEngine = engine;

	return std::tuple<TimerFunc, timerAdditionalData *>{
		// timerFunc
		[pEngine, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
			qtscript_timer_additionaldata* pData = static_cast<qtscript_timer_additionaldata*>(additionalParams);
			QScriptValueList args;
			if (baseObject != nullptr)
			{
				args += convMax(baseObject, pEngine);
			}
			else if (pData && !(pData->stringArg.isEmpty()))
			{
				args += pData->stringArg;
			}
			callFunction(pEngine, funcName, args, true);
		}
		// additionalParams
		, new qtscript_timer_additionaldata(stringArg)
	};
}

nlohmann::json qtscript_scripting_instance::debugGetAllScriptGlobals()
{
	nlohmann::json globals = nlohmann::json::object();
	QScriptValueIterator it(engine->globalObject());
	while (it.hasNext())
	{
		it.next();
		if ((internalNamespace.count(it.name().toStdString()) == 0 && !it.value().isFunction()
			 && !it.value().equals(engine->globalObject()))
			|| it.name() == "Upgrades" || it.name() == "Stats")
		{
			globals[it.name().toStdString()] = (it.value().toVariant()); // uses to_json QVariant implementation
		}
	}
	return globals;
}

bool qtscript_scripting_instance::debugEvaluateCommand(const std::string &_text)
{
	QString text = QString::fromStdString(_text);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(text);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in %s: %s",
		      text.toUtf8().constData(), syntax.errorMessage().toUtf8().constData());
		return false;
	}
	QScriptValue result = engine->evaluate(text);
	if (engine->hasUncaughtException())
	{
		debug(LOG_ERROR, "Uncaught exception in %s: %s",
		      text.toUtf8().constData(), result.toString().toUtf8().constData());
		return false;
	}
	console("%s", result.toString().toUtf8().constData());
	return true;
}

void qtscript_scripting_instance::updateGameTime(uint32_t gameTime)
{
	engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
}

void qtscript_scripting_instance::updateGroupSizes(int groupId, int size)
{
	QScriptValue groupMembers = engine->globalObject().property("groupSizes");
	groupMembers.setProperty(groupId, size, QScriptValue::ReadOnly);
}

void qtscript_scripting_instance::setSpecifiedGlobalVariables(const nlohmann::json& variables, wzapi::GlobalVariableFlags flags /*= wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave*/)
{
	if (!variables.is_object())
	{
		ASSERT(false, "setSpecifiedGlobalVariables expects a JSON object");
		return;
	}
	QScriptValue::PropertyFlags propertyFlags = toQScriptPropertyFlags(flags);
	bool markGlobalAsInternal = (flags & wzapi::GlobalVariableFlags::DoNotSave) == wzapi::GlobalVariableFlags::DoNotSave;
	for (auto it : variables.items())
	{
		ASSERT(!it.key().empty(), "Empty key");
		engine->globalObject().setProperty(
			QString::fromStdString(it.key()),
			mapJsonToQScriptValue(engine, it.value(), propertyFlags),
			propertyFlags
		);
		if (markGlobalAsInternal)
		{
			internalNamespace.insert(it.key());
		}
	}
}

void qtscript_scripting_instance::setSpecifiedGlobalVariable(const std::string& name, const nlohmann::json& value, wzapi::GlobalVariableFlags flags /*= GlobalVariableFlags::ReadOnly | GlobalVariableFlags::DoNotSave*/)
{
	ASSERT(!name.empty(), "Empty key");
	QScriptValue::PropertyFlags propertyFlags = toQScriptPropertyFlags(flags);
	bool markGlobalAsInternal = (flags & wzapi::GlobalVariableFlags::DoNotSave) == wzapi::GlobalVariableFlags::DoNotSave;
	engine->globalObject().setProperty(
		QString::fromStdString(name),
		mapJsonToQScriptValue(engine, value, propertyFlags),
		propertyFlags
	);
	if (markGlobalAsInternal)
	{
		internalNamespace.insert(name);
	}
}

void qtscript_scripting_instance::doNotSaveGlobal(const std::string &global)
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
IMPL_EVENT_HANDLER(eventDroidBuilt, const DROID *, const STRUCTURE *)
IMPL_EVENT_HANDLER(eventStructureBuilt, const STRUCTURE *, const DROID *)
IMPL_EVENT_HANDLER(eventStructureDemolish, const STRUCTURE *, const DROID *)
IMPL_EVENT_HANDLER(eventStructureReady, const STRUCTURE *)
IMPL_EVENT_HANDLER(eventAttacked, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventResearched, const wzapi::researchResult&, const STRUCTURE *, int)
IMPL_EVENT_HANDLER(eventDestroyed, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventPickup, const FEATURE *, const DROID *)
IMPL_EVENT_HANDLER(eventObjectSeen, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventGroupSeen, const BASE_OBJECT *, int)
IMPL_EVENT_HANDLER(eventObjectTransfer, const BASE_OBJECT *, int)
IMPL_EVENT_HANDLER(eventChat, int, int, const char *)
IMPL_EVENT_HANDLER(eventBeacon, int, int, int, int, const char *)
IMPL_EVENT_HANDLER(eventBeaconRemoved, int, int)
IMPL_EVENT_HANDLER(eventGroupLoss, const BASE_OBJECT *, int, int)
bool qtscript_scripting_instance::handle_eventArea(const std::string& label, const DROID *psDroid)
{
	QScriptValueList args;
	args += convDroid(psDroid, engine);
	QString funcname = QString("eventArea" + QString::fromStdString(label));
	debug(LOG_SCRIPT, "Triggering %s for %s", funcname.toUtf8().constData(),
		  engine->globalObject().property("scriptName").toString().toUtf8().constData());
	callFunction(engine, funcname, args);
	return true;
}
IMPL_EVENT_HANDLER(eventDesignCreated, const DROID_TEMPLATE *)
IMPL_EVENT_HANDLER(eventAllianceOffer, uint8_t, uint8_t)
IMPL_EVENT_HANDLER(eventAllianceAccepted, uint8_t, uint8_t)
IMPL_EVENT_HANDLER(eventAllianceBroken, uint8_t, uint8_t)

// MARK: Special input events
IMPL_EVENT_HANDLER(eventSyncRequest, int, int, int, int, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventKeyPressed, int, int)

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
static QScriptValue js_enumTemplates(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	QScriptValue result = engine->newArray(droidTemplates[player].size());
	int count = 0;
	for (auto &keyvaluepair : droidTemplates[player])
	{
		result.setProperty(count, convTemplate(keyvaluepair.second, engine));
		count++;
	}
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

//-- ## removeReticuleButton(button type)
//--
//-- Remove reticule button. DO NOT USE FOR ANYTHING.
//--
static QScriptValue js_removeReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue();
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

static QScriptValue js_removeBeacon(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::removeBeacon, context, engine);
	if (retVal.isBool() && retVal.toBool())
	{
		jsDebugMessageUpdate();
	}
	return retVal;
}

IMPL_JS_FUNC(chat, wzapi::chat)
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
IMPL_JS_FUNC(changePlayerColour, wzapi::changePlayerColour)
IMPL_JS_FUNC(getMultiTechLevel, wzapi::getMultiTechLevel)
IMPL_JS_FUNC(setCampaignNumber, wzapi::setCampaignNumber)
IMPL_JS_FUNC(getMissionType, wzapi::getMissionType)
IMPL_JS_FUNC(getRevealStatus, wzapi::getRevealStatus)
IMPL_JS_FUNC(setRevealStatus, wzapi::setRevealStatus)


QScriptValue js_stats(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue callee = context->callee();
	int type = callee.property("type").toInt32();
	int player = callee.property("player").toInt32();
	unsigned index = callee.property("index").toUInt32();
	QString name = callee.property("name").toString();
	qtscript_execution_context execution_context(context, engine);
	if (context->argumentCount() == 1) // setter
	{
		wzapi::setUpgradeStats(execution_context, player, name.toStdString(), type, index, context->argument(0).toVariant());
	}
	// Now read value and return it
	return mapJsonToQScriptValue(engine, wzapi::getUpgradeStats(execution_context, player, name.toStdString(), type, index), QScriptValue::ReadOnly | QScriptValue::Undeletable);
}

static void setStatsFunc(QScriptValue &base, QScriptEngine *engine, const QString& name, int player, int type, unsigned index)
{
	QScriptValue v = engine->newFunction(js_stats);
	base.setProperty(name, v, QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
	v.setProperty("player", player, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("type", type, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("index", index, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("name", name, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
}

QScriptValue constructUpgradesQScriptValue(QScriptEngine *engine)
{
	auto upgradesObj = wzapi::getUpgradesObject();

	QScriptValue upgrades = engine->newArray(MAX_PLAYERS);
	for (auto& playerUpgrades : upgradesObj)
	{
		QScriptValue node = engine->newObject();

		for (const auto& gameEntityClass : playerUpgrades)
		{
			const std::string& gameEntityClassName = gameEntityClass.first;
			QScriptValue entityBase = engine->newObject();
			for (const auto& gameEntity : gameEntityClass.second)
			{
				const std::string& gameEntityName = gameEntity.first;
				const auto& gameEntityRules = gameEntity.second;
				QScriptValue v = engine->newObject();
				for (const auto& property : gameEntityRules)
				{
					setStatsFunc(v, engine, QString::fromUtf8(property.first.c_str()), gameEntityRules.getPlayer(), property.second, gameEntityRules.getIndex());
				}
				entityBase.setProperty(QString::fromUtf8(gameEntityName.c_str()), v, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			}
			node.setProperty(QString::fromUtf8(gameEntityClassName.c_str()), entityBase, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}

		// Finally
		ASSERT(playerUpgrades.getPlayer() >= 0 && playerUpgrades.getPlayer() < MAX_PLAYERS, "Invalid player index");
		upgrades.setProperty(playerUpgrades.getPlayer(), node, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}

	return upgrades;
}

#define JS_REGISTER_FUNC(js_func_name) \
	engine->globalObject().setProperty(#js_func_name, engine->newFunction(JS_FUNC_IMPL_NAME(js_func_name)));

#define JS_REGISTER_FUNC_NAME(js_func_name, full_impl_handler_func_name) \
	engine->globalObject().setProperty(#js_func_name, engine->newFunction(full_impl_handler_func_name));

bool qtscript_scripting_instance::registerFunctions(const QString& scriptName)
{
	debug(LOG_WZ, "Loading functions for engine %p, script %s", static_cast<void *>(engine), scriptName.toUtf8().constData());

	//== * ```Upgrades``` A special array containing per-player rules information for game entity types,
	//== which can be written to in order to implement upgrades and other dynamic rules changes. Each item in the
	//== array contains a subset of the sparse array of rules information in the ```Stats``` global.
	//== These values are defined:
	QScriptValue upgrades = constructUpgradesQScriptValue(engine);
	engine->globalObject().setProperty("Upgrades", upgrades, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Register functions to the script engine here
	JS_REGISTER_FUNC_NAME(_, JS_FUNC_IMPL_NAME(translate)); // WZAPI
	JS_REGISTER_FUNC(dump);
	JS_REGISTER_FUNC(syncRandom); // WZAPI
	JS_REGISTER_FUNC_NAME(label, JS_FUNC_IMPL_NAME(getObject)); // deprecated // scripting_engine
	JS_REGISTER_FUNC(getObject); // scripting_engine
	JS_REGISTER_FUNC(addLabel); // scripting_engine
	JS_REGISTER_FUNC(removeLabel); // scripting_engine
	JS_REGISTER_FUNC(getLabel); // scripting_engine
	JS_REGISTER_FUNC(enumLabels); // scripting_engine
	JS_REGISTER_FUNC(enumGateways); // WZAPI
	JS_REGISTER_FUNC(enumTemplates);
	JS_REGISTER_FUNC(makeTemplate); // WZAPI
	JS_REGISTER_FUNC(setAlliance); // WZAPI
	JS_REGISTER_FUNC(sendAllianceRequest); // WZAPI
	JS_REGISTER_FUNC(setAssemblyPoint); // WZAPI
	JS_REGISTER_FUNC(setSunPosition); // WZAPI
	JS_REGISTER_FUNC(setSunIntensity); // WZAPI
	JS_REGISTER_FUNC(setWeather); // WZAPI
	JS_REGISTER_FUNC(setSky); // WZAPI
	JS_REGISTER_FUNC(cameraSlide); // WZAPI
	JS_REGISTER_FUNC(cameraTrack); // WZAPI
	JS_REGISTER_FUNC(cameraZoom); // WZAPI
	JS_REGISTER_FUNC_NAME(resetArea, JS_FUNC_IMPL_NAME(resetLabel)); // deprecated // scripting_engine
	JS_REGISTER_FUNC(resetLabel); // scripting_engine
	JS_REGISTER_FUNC(addSpotter); // WZAPI
	JS_REGISTER_FUNC(removeSpotter); // WZAPI
	JS_REGISTER_FUNC(syncRequest); // WZAPI
	JS_REGISTER_FUNC(replaceTexture); // WZAPI
	JS_REGISTER_FUNC(changePlayerColour); // WZAPI
	JS_REGISTER_FUNC(setHealth); // WZAPI
	JS_REGISTER_FUNC(useSafetyTransport); // WZAPI
	JS_REGISTER_FUNC(restoreLimboMissionData); // WZAPI
	JS_REGISTER_FUNC(getMultiTechLevel); // WZAPI
	JS_REGISTER_FUNC(setCampaignNumber); // WZAPI
	JS_REGISTER_FUNC(getMissionType); // WZAPI
	JS_REGISTER_FUNC(getRevealStatus); // WZAPI
	JS_REGISTER_FUNC(setRevealStatus); // WZAPI
	JS_REGISTER_FUNC(autoSave); // WZAPI

	// horrible hacks follow -- do not rely on these being present!
	JS_REGISTER_FUNC(hackNetOff); // WZAPI
	JS_REGISTER_FUNC(hackNetOn); // WZAPI
	JS_REGISTER_FUNC(hackAddMessage); // WZAPI
	JS_REGISTER_FUNC(hackRemoveMessage); // WZAPI
	JS_REGISTER_FUNC(hackGetObj); // WZAPI
	JS_REGISTER_FUNC(hackAssert); // WZAPI
	JS_REGISTER_FUNC(hackMarkTiles); // WZAPI
	JS_REGISTER_FUNC(receiveAllEvents); // WZAPI
	JS_REGISTER_FUNC(hackDoNotSave); // WZAPI
	JS_REGISTER_FUNC(hackPlayIngameAudio); // WZAPI
	JS_REGISTER_FUNC(hackStopIngameAudio); // WZAPI

	// General functions -- geared for use in AI scripts
	JS_REGISTER_FUNC(debug);
	JS_REGISTER_FUNC(console); // WZAPI
	JS_REGISTER_FUNC(clearConsole); // WZAPI
	JS_REGISTER_FUNC(structureIdle); // WZAPI
	JS_REGISTER_FUNC(enumStruct); // WZAPI
	JS_REGISTER_FUNC(enumStructOffWorld); // WZAPI
	JS_REGISTER_FUNC(enumDroid); // WZAPI
	JS_REGISTER_FUNC(enumGroup); // scripting_engine
	JS_REGISTER_FUNC(enumFeature); // WZAPI
	JS_REGISTER_FUNC(enumBlips); // WZAPI
	JS_REGISTER_FUNC(enumSelected); // WZAPI
	JS_REGISTER_FUNC(enumResearch); // WZAPI
	JS_REGISTER_FUNC(enumRange); // WZAPI
	JS_REGISTER_FUNC(enumArea); // scripting_engine
	JS_REGISTER_FUNC(getResearch); // WZAPI
	JS_REGISTER_FUNC(pursueResearch); // WZAPI
	JS_REGISTER_FUNC(findResearch); // WZAPI
	JS_REGISTER_FUNC(distBetweenTwoPoints); // WZAPI
	JS_REGISTER_FUNC(newGroup); // scripting_engine
	JS_REGISTER_FUNC(groupAddArea); // scripting_engine
	JS_REGISTER_FUNC(groupAddDroid); // scripting_engine
	JS_REGISTER_FUNC(groupAdd); // scripting_engine
	JS_REGISTER_FUNC(groupSize); // scripting_engine
	JS_REGISTER_FUNC(orderDroidLoc); // WZAPI
	JS_REGISTER_FUNC(playerPower); // WZAPI
	JS_REGISTER_FUNC(queuedPower); // WZAPI
	JS_REGISTER_FUNC(isStructureAvailable); // WZAPI
	JS_REGISTER_FUNC(pickStructLocation); // WZAPI
	JS_REGISTER_FUNC(droidCanReach); // WZAPI
	JS_REGISTER_FUNC(propulsionCanReach); // WZAPI
	JS_REGISTER_FUNC(terrainType); // WZAPI
	JS_REGISTER_FUNC(orderDroidBuild); // WZAPI
	JS_REGISTER_FUNC(orderDroidObj); // WZAPI
	JS_REGISTER_FUNC(orderDroid); // WZAPI
	JS_REGISTER_FUNC(buildDroid); // WZAPI
	JS_REGISTER_FUNC(addDroid); // WZAPI
	JS_REGISTER_FUNC(addDroidToTransporter); // WZAPI
	JS_REGISTER_FUNC(addFeature); // WZAPI
	JS_REGISTER_FUNC(componentAvailable); // WZAPI
	JS_REGISTER_FUNC(isVTOL); // WZAPI
	JS_REGISTER_FUNC(safeDest); // WZAPI
	JS_REGISTER_FUNC(activateStructure); // WZAPI
	JS_REGISTER_FUNC(chat); // WZAPI
	JS_REGISTER_FUNC(addBeacon); // WZAPI
	JS_REGISTER_FUNC(removeBeacon); // WZAPI
	JS_REGISTER_FUNC(getDroidProduction); // WZAPI
	JS_REGISTER_FUNC(getDroidLimit); // WZAPI
	JS_REGISTER_FUNC(getExperienceModifier); // WZAPI
	JS_REGISTER_FUNC(setDroidLimit); // WZAPI
	JS_REGISTER_FUNC(setCommanderLimit); // deprecated!!
	JS_REGISTER_FUNC(setConstructorLimit); // deprecated!!
	JS_REGISTER_FUNC(setExperienceModifier); // WZAPI
	JS_REGISTER_FUNC(getWeaponInfo); // WZAPI // deprecated!!
	JS_REGISTER_FUNC(enumCargo); // WZAPI

	// Functions that operate on the current player only
	JS_REGISTER_FUNC(centreView); // WZAPI
	JS_REGISTER_FUNC(playSound); // WZAPI
	JS_REGISTER_FUNC(gameOverMessage); // WZAPI

	// Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)
	JS_REGISTER_FUNC(setStructureLimits); // WZAPI
	JS_REGISTER_FUNC(applyLimitSet); // WZAPI
	JS_REGISTER_FUNC(setMissionTime); // WZAPI
	JS_REGISTER_FUNC(getMissionTime); // WZAPI
	JS_REGISTER_FUNC(setReinforcementTime); // WZAPI
	JS_REGISTER_FUNC(completeResearch); // WZAPI
	JS_REGISTER_FUNC(completeAllResearch); // WZAPI
	JS_REGISTER_FUNC(enableResearch); // WZAPI
	JS_REGISTER_FUNC(setPower); // WZAPI
	JS_REGISTER_FUNC(setPowerModifier); // WZAPI
	JS_REGISTER_FUNC(setPowerStorageMaximum); // WZAPI
	JS_REGISTER_FUNC(extraPowerTime); // WZAPI
	JS_REGISTER_FUNC(setTutorialMode); // WZAPI
	JS_REGISTER_FUNC(setDesign); // WZAPI
	JS_REGISTER_FUNC(enableTemplate); // WZAPI
	JS_REGISTER_FUNC(removeTemplate); // WZAPI
	JS_REGISTER_FUNC(setMiniMap); // WZAPI
	JS_REGISTER_FUNC(setReticuleButton); // WZAPI
	JS_REGISTER_FUNC(setReticuleFlash); // WZAPI
	JS_REGISTER_FUNC(showReticuleWidget); // WZAPI
	JS_REGISTER_FUNC(showInterface); // WZAPI
	JS_REGISTER_FUNC(hideInterface); // WZAPI
	JS_REGISTER_FUNC_NAME(addReticuleButton, JS_FUNC_IMPL_NAME(removeReticuleButton)); // deprecated!!
	JS_REGISTER_FUNC(removeReticuleButton); // deprecated!!
	JS_REGISTER_FUNC(enableStructure); // WZAPI
	JS_REGISTER_FUNC(makeComponentAvailable); // WZAPI
	JS_REGISTER_FUNC(enableComponent); // WZAPI
	JS_REGISTER_FUNC(allianceExistsBetween); // WZAPI
	JS_REGISTER_FUNC(removeStruct); // WZAPI // deprecated!!
	JS_REGISTER_FUNC(removeObject); // WZAPI
	JS_REGISTER_FUNC_NAME(setScrollParams, JS_FUNC_IMPL_NAME(setScrollLimits)); // deprecated!!
	JS_REGISTER_FUNC(setScrollLimits); // WZAPI
	JS_REGISTER_FUNC(getScrollLimits); // WZAPI
	JS_REGISTER_FUNC(addStructure); // WZAPI
	JS_REGISTER_FUNC(getStructureLimit); // WZAPI
	JS_REGISTER_FUNC(countStruct); // WZAPI
	JS_REGISTER_FUNC(countDroid); // WZAPI
	JS_REGISTER_FUNC(loadLevel); // WZAPI
	JS_REGISTER_FUNC(setDroidExperience); // WZAPI
	JS_REGISTER_FUNC(donateObject); // WZAPI
	JS_REGISTER_FUNC(donatePower); // WZAPI
	JS_REGISTER_FUNC(setNoGoArea); // WZAPI
	JS_REGISTER_FUNC(startTransporterEntry); // WZAPI
	JS_REGISTER_FUNC(setTransporterExit); // WZAPI
	JS_REGISTER_FUNC(setObjectFlag); // WZAPI
	JS_REGISTER_FUNC(fireWeaponAtLoc); // WZAPI
	JS_REGISTER_FUNC(fireWeaponAtObj); // WZAPI

	return true;
}

// Enable JSON support for custom types

// QVariant
void to_json(nlohmann::json& j, const QVariant& value) {
	// IMPORTANT: This largely follows the Qt documentation on QJsonValue::fromVariant
	// See: http://doc.qt.io/qt-5/qjsonvalue.html#fromVariant
	//
	// The main change is that numeric types are independently handled (instead of
	// converting everything to `double`), because nlohmann::json handles them a bit
	// differently.

	// Note: Older versions of Qt 5.x (5.6?) do not define QMetaType::Nullptr,
	//		 so check value.isNull() instead.
	if (value.isNull())
	{
		j = nlohmann::json(); // null value
		return;
	}

	switch (value.userType())
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
		case QMetaType::Nullptr:
			j = nlohmann::json(); // null value
			break;
#endif
		case QMetaType::Bool:
			j = value.toBool();
			break;
		case QMetaType::Int:
			j = value.toInt();
			break;
		case QMetaType::UInt:
			j = value.toUInt();
			break;
		case QMetaType::LongLong:
			j = value.toLongLong();
			break;
		case QMetaType::ULongLong:
			j = value.toULongLong();
			break;
		case QMetaType::Float:
		case QVariant::Double:
			j = value.toDouble();
			break;
		case QMetaType::QString:
		{
			QString qstring = value.toString();
			j = json(qstring.toUtf8().constData());
			break;
		}
		case QMetaType::QStringList:
		case QMetaType::QVariantList:
		{
			// an array
			j = nlohmann::json::array();
			QList<QVariant> list = value.toList();
			for (QVariant& list_variant : list)
			{
				nlohmann::json list_variant_json_value;
				to_json(list_variant_json_value, list_variant);
				j.push_back(list_variant_json_value);
			}
			break;
		}
		case QMetaType::QVariantMap:
		{
			// an object
			j = nlohmann::json::object();
			QMap<QString, QVariant> map = value.toMap();
			for (QMap<QString, QVariant>::const_iterator i = map.constBegin(); i != map.constEnd(); ++i)
			{
				j[i.key().toUtf8().constData()] = i.value();
			}
			break;
		}
		case QMetaType::QVariantHash:
		{
			// an object
			j = nlohmann::json::object();
			QHash<QString, QVariant> hash = value.toHash();
			for (QHash<QString, QVariant>::const_iterator i = hash.constBegin(); i != hash.constEnd(); ++i)
			{
				j[i.key().toUtf8().constData()] = i.value();
			}
			break;
		}
		default:
			// For all other QVariant types a conversion to a QString will be attempted.
			QString qstring = value.toString();
			// If the returned string is empty, a Null QJsonValue will be stored, otherwise a String value using the returned QString.
			if (qstring.isEmpty())
			{
				j = nlohmann::json(); // null value
			}
			else
			{
				j = std::string(qstring.toUtf8().constData());
			}
	}
}

