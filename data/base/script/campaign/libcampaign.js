//;; # libcampaign.js documentation
//;;
//;; ```libcampaign.js``` is a JavaScript library supplied with the game,
//;; which contains reusable code for campaign scenarios. It is designed to
//;; make scenario development as high-level and declarative as possible.
//;; It also contains a few simple convenient wrappers.
//;; Public API functions of ```libcampaign.js``` are prefixed with
//;; ```cam```. To use ```libcampaign.js```, add the following include
//;; into your scenario code:
//;;
//;; ```javascript
//;; include("script/campaign/libcampaign.js");
//;; ```
//;;
//;; Also, most of the ```libcampaign.js``` features require some of the
//;; game events handled by the library. Transparent JavaScript pre-hooks are
//;; therefore injected into your global event handlers upon include. For
//;; example, if ```camSetArtifacts()``` was called to let
//;; ```libcampaign.js``` manage scenario artifacts, then
//;; ```eventPickup()``` will be first handled by the library, and only then
//;; your handler will be called, if any.
//;; All of this happens automagically and does not normally require
//;; your attention.
//;;

/*
	Private vars and functions are prefixed with `__cam'.
	Please do not use private stuff in scenario code, use only public API.

	We CANNOT put our private vars into an anonymous namespace, even though
	it's a common JS trick -

		(function(global) {
			var __camPrivateVar; // something like that
		})(this);

	because they would break on savegame-loadgame. So let's just agree
	that we'd never use them anywhere outside the file, so that it'd be easier
	to change them, and also think twice before we change the public API.

	The lib is split into sections, each section is separated with a slash line:

////////////////////////////////////////////////////////////////////////////////
// Section name.
////////////////////////////////////////////////////////////////////////////////

	yeah, like that. Also, it's exactly 80 characters wide.

	In each section, public stuff is on TOP, and private stuff
	is below, split from the public stuff with:

//////////// privates

	, for easier reading (all the useful stuff on top).

	Please leave camDebug() around if something that should never happen
	occurs, indicating an actual bug in campaign. Then a sensible message
	should be helpful as well. But normally, no warnings should ever be
	printed.

	In cheat mode, more warnings make sense, explaining what's going on
	under the hood. Use camTrace() for such warnings - they'd auto-hide
	outside cheat mode.

	Lines prefixed with // followed by ;; are docstrings for JavaScript API
	documentation.
*/

////////////////////////////////////////////////////////////////////////////////
// Library initialization.
////////////////////////////////////////////////////////////////////////////////

// Registers a private event namespace for this library, to avoid collisions with
// any event handling in code using this library. Make sure no other library uses
// the same namespace, or strange things will happen. After this, we can name our
// event handlers with the registered prefix, and they will still get called.
namespace("cam_");

////////////////////////////////////////////////////////////////////////////////
// Misc useful stuff.
////////////////////////////////////////////////////////////////////////////////

//These are campaign player numbers.
const CAM_HUMAN_PLAYER = 0;
const NEW_PARADIGM = 1;
const THE_COLLECTIVE = 2;
const NEXUS = 3;

const CAM_MAX_PLAYERS = 8;
const CAM_TICKS_PER_FRAME = 100;
const AI_POWER = 999999;

//;; ## camDef(something)
//;;
//;; Returns false if something is JavaScript-undefined, true otherwise.
//;;
function camDef(something)
{
	return typeof(something) !== "undefined";
}

//;; ## camIsString(something)
//;;
//;; Returns true if something is a string, false otherwise.
//;;
function camIsString(something)
{
	return typeof(something) === "string";
}

//;; ## camRand(max)
//;;
//;; A non-synchronous random integer in range [0, max - 1].
//;;
function camRand(max)
{
	if (max > 0)
	{
		return Math.floor(Math.random() * max);
	}
	camDebug("Max should be positive");
}

//;; ## camCallOnce(function name)
//;;
//;; Call a function by name, but only if it has not been called yet.
//;;
function camCallOnce(callback)
{
	if (camDef(__camCalledOnce[callback]) && __camCalledOnce[callback])
	{
		return;
	}
	__camCalledOnce[callback] = true;
	__camGlobalContext()[callback]();
}

//;; ## camSafeRemoveObject(obj[, special effects?])
//;;
//;; Remove a game object (by value or label) if it exists, do nothing otherwise.
//;;
function camSafeRemoveObject(obj, flashy)
{
	if (__camLevelEnded)
	{
		return;
	}
	if (camIsString(obj))
	{
		obj = getObject(obj);
	}
	if (camDef(obj) && obj)
	{
		removeObject(obj, flashy);
	}
}

//;; ## camMakePos(x, y | label | object)
//;;
//;; Make a POSITION-like object, unless already done. Often useful
//;; for making functions that would accept positions in both xx,yy and {x:xx,y:yy} forms.
//;; Also accepts labels. If label of AREA is given, returns the center of the area.
//;; If an existing object or label of such is given, returns a safe JavaScript
//;; object containing its x, y and id.
//;;
function camMakePos(xx, yy)
{
	if (camDef(yy))
	{
		return { x : xx, y : yy };
	}
	if (!camDef(xx))
	{
		return undefined;
	}
	var obj = xx;
	if (camIsString(xx))
	{
		obj = getObject(xx);
	}
	if (!camDef(obj) || !obj)
	{
		camDebug("Failed at", xx);
		return undefined;
	}
	switch (obj.type)
	{
		case DROID:
		case STRUCTURE:
		case FEATURE:
			// store ID for those as well.
			return { x: obj.x, y: obj.y, id: obj.id };
		case POSITION:
		case RADIUS:
			return obj;
		case AREA:
			return {
				x: Math.floor((obj.x + obj.x2) / 2),
				y: Math.floor((obj.y + obj.y2) / 2)
			};
		case GROUP:
		default:
			// already a pos-like object?
			if (camDef(obj.x) && camDef(obj.y))
			{
				return { x: obj.x, y: obj.y };
			}
			camDebug("Not implemented:", obj.type);
			return undefined;
	}
}

//;; ## camDist(x1, y1, x2, y2 | pos1, x2, y2 | x1, y1, pos2 | pos1, pos2)
//;;
//;; A wrapper for ```distBetweenTwoPoints()```.
//;;
function camDist(x1, y1, x2, y2)
{
	if (camDef(y2)) // standard
	{
		return distBetweenTwoPoints(x1, y1, x2, y2);
	}
	var pos2 = camMakePos(x2, y2);
	if (!camDef(pos2)) // pos1, pos2
	{
		return distBetweenTwoPoints(x1.x, x1.y, y1.x, y1.y);
	}
	if (camDef(pos2.x)) // x2 is pos2
	{
		return distBetweenTwoPoints(x1, y1, pos2.x, pos2.y);
	}
	else // pos1, x2, y2
	{
		return distBetweenTwoPoints(x1.x, x1.y, y1, x2);
	}
}

//;; ## camPlayerMatchesFilter(player, filter)
//;;
//;; A function to handle player filters in a way similar to
//;; how JS API functions (eg. ```enumDroid(filter, ...)```) handle them.
//;;
function camPlayerMatchesFilter(player, filter)
{
	switch(filter) {
		case ALL_PLAYERS:
			return true;
		case ALLIES:
			return player === CAM_HUMAN_PLAYER;
		case ENEMIES:
			return player >= 0 && player < CAM_MAX_PLAYERS
			                   && player !== CAM_HUMAN_PLAYER;
		default:
			return player === filter;
	}
}

//;; ## camRemoveDuplicates(array)
//;;
//;; Remove duplicate items from an array.
//;;
function camRemoveDuplicates(array)
{
	var prims = {"boolean":{}, "number":{}, "string":{}};
	var objs = [];

	return array.filter(function(item) {
		var type = typeof item;
		if (type in prims)
		{
			return prims[type].hasOwnProperty(item) ? false : (prims[type][item] = true);
		}
		else
		{
			return objs.indexOf(item) >= 0 ? false : objs.push(item);
		}
	});
}

//;; ## camCountStructuresInArea(label, [player])
//;;
//;; Mimics wzscript's numStructsButNotWallsInArea().
//;;
function camCountStructuresInArea(lab, player)
{
	if (!camDef(player))
	{
		player = CAM_HUMAN_PLAYER;
	}
	var list = enumArea(lab, player, false);
	var ret = 0;
	for (var i = 0, l = list.length; i < l; ++i)
	{
		if (list[i].type === STRUCTURE && list[i].stattype !== WALL
		                               && list[i].status === BUILT)
		{
			++ret;
		}
	}
	return ret;
}

//;; ## camChangeOnDiff(numeric value, [bool])
//;;
//;; Change a numeric value based on campaign difficulty. If the second option is defined
//;; then the opposite effect will occur on that value.
//;;
function camChangeOnDiff(num, invert)
{
	var modifier = 0;
	if(!camDef(invert))
	{
		invert = false;
	}

	switch(difficulty)
	{
		case EASY:
			modifier = (invert === false) ? 1.25 : 0.67;
			break;
		case MEDIUM:
			modifier = 1.00;
			break;
		case HARD:
			modifier = (invert === false) ? 0.80 : 1.50;
			break;
		case INSANE:
			modifier = (invert === false) ? 0.67 : 2.50;
			break;
		default:
			modifier = 1.00;
			break;
	}

	return Math.floor(num * modifier);
}

//;; ## camIsSystemDroid(game object)
//;;
//;; Determine if the passed in object is a non-weapon based droid.
//;;
function camIsSystemDroid(obj)
{
	if(!camDef(obj) || !obj)
	{
		return false;
	}

	if (obj.type !== DROID)
	{
		camTrace("Non-droid: " + obj.type + " pl: " + obj.name);
		return false;
	}

	return (obj.droidType === DROID_SENSOR
		|| obj.droidType === DROID_CONSTRUCT
		|| obj.droidType === DROID_REPAIR
	);
}

//;; ## camMakeGroup(what, filter)
//;;
//;; Make a new group out of array of droids, single game object,
//;; or label string, with fuzzy auto-detection of argument type.
//;; Only droids would be added to the group. ```filter``` can be one of
//;; a player index, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```;
//;; defaults to ```ENEMIES```.
//;;
function camMakeGroup(what, filter)
{
	if (!camDef(filter))
	{
		filter = ENEMIES;
	}
	var array;
	var obj;
	if (camIsString(what)) // label
	{
		obj = getObject(what);
	}
	else if (camDef(what.length)) // array
	{
		array = what;
	}
	else if (camDef(what.type)) // object
	{
		obj = what;
	}
	if (camDef(obj))
	{
		switch(obj.type) {
			case POSITION:
				obj = getObject(obj.x, obj.y);
				// fall-through
			case DROID:
			case STRUCTURE:
			case FEATURE:
				array = [ obj ];
				break;
			case AREA:
				array = enumArea(obj.x, obj.y, obj.x2, obj.y2,
				                 ALL_PLAYERS, false);
				break;
			case RADIUS:
				array = enumRange(obj.x, obj.y, obj.radius,
				                 ALL_PLAYERS, false);
				break;
			case GROUP:
				array = enumGroup(obj.id);
				break;
			default:
				camDebug("Unknown object type", obj.type);
		}
	}
	if (camDef(array))
	{
		var group = camNewGroup();
		for (var i = 0, l = array.length; i < l; ++i)
		{
			var o = array[i];
			if (!camDef(o) || !o)
			{
				camDebug("Trying to add", o);
				continue;
			}
			if (o.type === DROID && camPlayerMatchesFilter(o.player, filter))
			{
				groupAdd(group, o);
			}
		}
		return group;
	}
	camDebug("Cannot parse", what);
}

//;; ## camBreakAlliances()
//;;
//;; Break alliances between all players.
//;;
function camBreakAlliances()
{
	for (var i = 0; i < CAM_MAX_PLAYERS; ++i)
	{
		for (var c = 0; c < CAM_MAX_PLAYERS; ++c)
		{
			if (i !== c && allianceExistsBetween(i, c) === true)
			{
				setAlliance(i, c, false);
			}
		}
	}
}

//////////// privates

var __camCalledOnce = {};

function __camGlobalContext()
{
	return Function('return this')();
}

////////////////////////////////////////////////////////////////////////////////
// Research and structure related functions.
////////////////////////////////////////////////////////////////////////////////

//;; ## camEnableRes(list, player)
//;;
//;; Grants research from the given list to player
//;;
function camEnableRes(list, player)
{
	for(var i = 0, l = list.length; i < l; ++i)
	{
		enableResearch(list[i], player);
		completeResearch(list[i], player);
	}
}

//;; ## camCompleteRequiredResearch(items, player)
//;;
//;; Grants research from the given list to player and also researches
//;; the required research for that item.
//;;
function camCompleteRequiredResearch(items, player)
{
	dump("\n*Player " + player + " requesting accelerated research.");

	for (var i = 0, l = items.length; i < l; ++i)
	{
		dump("Searching for required research of item: " + items[i]);
		var reqRes = findResearch(items[i], player).reverse();

		if (reqRes.length === 0)
		{
			//HACK: autorepair like upgrades don't work after mission transition.
			if (items[i] === "R-Sys-NEXUSrepair")
			{
				completeResearch("R-Sys-NEXUSrepair", player, true);
			}
			continue;
		}

		reqRes = camRemoveDuplicates(reqRes);
		for (var s = 0, r = reqRes.length; s < r; ++s)
		{
			dump("	Found: " + reqRes[s].name);
			enableResearch(reqRes[s].name, player);
			completeResearch(reqRes[s].name, player);
		}
	}
}

//privates

//granted shortly after mission start to give enemy players instant droid production.
function __camGrantSpecialResearch()
{
	for (var i = 1; i < CAM_MAX_PLAYERS; ++i)
	{
		if (!allianceExistsBetween(CAM_HUMAN_PLAYER, i) && (countDroid(DROID_ANY, i) || enumStruct(i).length))
		{
			//Boost AI production to produce all droids within a factory throttle
			completeResearch("R-Struc-Factory-Upgrade-AI", i);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Debugging helpers.
////////////////////////////////////////////////////////////////////////////////

//;; ## camMarkTiles(label | array of labels)
//;;
//;; Mark area on the map by label(s), but only if debug mode is enabled.
//;; Otherwise, remember what to mark in case it is going to be.
//;;
function camMarkTiles(label)
{
	if (camIsString(label))
	{
		__camMarkedTiles[label] = true;
	}
	else
	{
		for (var i = 0, l = label.length; i < l; ++i)
		{
			__camMarkedTiles[label[i]] = true;
		}
	}
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; ## camUnmarkTiles(label | array of labels)
//;;
//;; No longer mark area(s) with given label(s) in debug mode.
//;;
function camUnmarkTiles(label)
{
	if (camIsString(label))
	{
		delete __camMarkedTiles[label];
	}
	else
	{
		for (var i = 0, l = label.length; i < l; ++i)
		{
			delete __camMarkedTiles[label[i]];
		}
	}
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; ## camDebug(string...)
//;;
//;; Pretty debug prints - a wrapper around ```debug()```.
//;; Prints a function call stack and the argument message,
//;; prefixed with "DEBUG". Only use this function to indicate
//;; actual bugs in the scenario script, because game shouldn't
//;; print things when nothing is broken. If you want to keep
//;; some prints around to make debugging easier without distracting
//;; the user, use ```camTrace()```.
//;;
function camDebug()
{
	__camGenericDebug("DEBUG",
	                  arguments.callee.caller.name,
	                  arguments,
	                  true,
	                  __camBacktrace());
}

//;; ## camDebugOnce(string...)
//;;
//;; Same as ```camDebug()```, but prints each message only once
//;; during script lifetime.
//;;
function camDebugOnce()
{
	var str = arguments.callee.caller.name
		+ ": " + Array.prototype.join.call(arguments, " ");
	if (camDef(__camDebuggedOnce[str]))
	{
		return;
	}
	__camDebuggedOnce[str] = true;
	__camGenericDebug("DEBUG",
	                  arguments.callee.caller.name,
	                  arguments,
	                  true,
	                  __camBacktrace());
}

//;; ## camTrace(string...)
//;;
//;; Same as ```camDebug()```, but only warns in cheat mode.
//;; Prefixed with "TRACE". It's safe and natural to keep ```camTrace()```
//;; calls in your code for easier debugging.
//;;
function camTrace()
{
	if (!__camCheatMode)
	{
		return;
	}
	__camGenericDebug("TRACE",
	                  arguments.callee.caller.name,
	                  arguments);
}

//;; ## camTraceOnce(string...)
//;;
//;; Same as ```camTrace()```, but prints each message only once
//;; during script lifetime.
//;;
function camTraceOnce()
{
	if (!__camCheatMode)
	{
		return;
	}
	var str = arguments.callee.caller.name
		+ ": " + Array.prototype.join.call(arguments, " ");
	if (camDef(__camTracedOnce[str]))
	{
		return;
	}
	__camTracedOnce[str] = true;
	__camGenericDebug("TRACE",
	                  arguments.callee.caller.name,
	                  arguments);
}

//;; ## isCheating()
//;;
//;; Check if the player is in cheat mode.
//;;
function isCheating()
{
	return __camCheatMode;
}

//////////// privates

var __camMarkedTiles = {};
var __camCheatMode = false;
var __camDebuggedOnce = {};
var __camTracedOnce = {};

function __camUpdateMarkedTiles()
{
	hackMarkTiles();
	if (__camCheatMode && camDef(__camMarkedTiles))
	{
		for (var label in __camMarkedTiles)
		{
			hackMarkTiles(label);
		}
	}
}

function __camLetMeWin()
{
	__camLetMeWinArtifacts();
	hackMarkTiles(); // clear marked tiles, as they may actually remain
	__camGameWon();
}

function __camWinInfo()
{
	console("Picked up", __camNumArtifacts,
	        "artifacts out of", Object.keys(__camArtifacts).length);
	console("Eliminated", __camNumEnemyBases,
	        "enemy bases out of", Object.keys(__camEnemyBases).length);
}

function __camGenericDebug(flag, func, args, err, bt)
{
	if (camDef(bt) && bt)
	{
		for (var i = bt.length - 1; i >= 0; --i)
		{
			debug("STACK: from", [bt[i]]);
		}
	}
	if (!func)
	{
		func = "<anonymous>";
	}
	var str = flag + ": " + func + ": " + Array.prototype.join.call(args, " ");
	debug(str);
	if (camDef(err) && err)
	{
		dump(str);
	}
}

function __camBacktrace()
{
	var func = arguments.callee.caller;
	var list = [];
	while (camDef(func) && func)
	{
		if (func.name)
		{
			list.push(func.name);
		}
		else
		{
			list.push("<anonymous>");
		}
		func = func.caller;
	}
	return list;
}


////////////////////////////////////////////////////////////////////////////////
// Artifacts management.
////////////////////////////////////////////////////////////////////////////////

//;; ## camSetArtifacts(artifacts)
//;;
//;; Tell ```libcampaign.js``` to manage a certain set of artifacts.
//;; The argument is a JavaScript map from object labels to artifact
//;; description. If the label points to a game object, artifact will be
//;; placed when this object is destroyed; if the label is a position, the
//;; artifact will be placed instantly. Artifact description is a JavaScript
//;; object with the following fields:
//;;
//;; * ```tech``` The technology to grant when the artifact is recovered.
//;; On __let me win__ cheat, all technologies stored in the artifacts
//;; managed by this function are automatically granted.
//;; Additionally, this function would call special event callbacks if they are
//;; defined in your script, which should be named as follows,
//;; where LABEL is the artifact label:
//;; * ```camArtifactPickup_LABEL``` Called when the player picks up
//;; 	the artifact.
//;;
function camSetArtifacts(artifacts)
{
	__camArtifacts = artifacts;
	for (var alabel in __camArtifacts)
	{
		var ai = __camArtifacts[alabel];
		var pos = camMakePos(alabel);
		if (camDef(pos.id))
		{
			// will place when object with this id is destroyed
			ai.id = "" + pos.id;
			ai.placed = false;
		}
		else
		{
			// received position or area, place immediately
			var acrate = addFeature("Crate", pos.x, pos.y);
			addLabel(acrate, __camGetArtifactLabel(alabel));
			ai.placed = true;
		}
	}
}

//;; ## camAllArtifactsPickedUp()
//;;
//;; Returns true if all artifacts managed by ```libcampaign.js```
//;; were picked up.
//;;
function camAllArtifactsPickedUp()
{
	// FIXME: O(n) lookup here
	return __camNumArtifacts === Object.keys(__camArtifacts).length;
}

//Returns the label of all existing artifacts
function camGetArtifacts()
{
	var camArti = [];
	for (var alabel in __camArtifacts)
	{
		if (camDef(__camArtifacts[alabel]))
		{
			if (getObject(__camGetArtifactLabel(alabel)))
			{
				camArti.push(__camGetArtifactLabel(alabel));
			}
			else
			{
				camArti.push(alabel);
			}
		}
	}
	return camArti;
}

//////////// privates

var __camArtifacts;
var __camNumArtifacts;

function __camGetArtifactLabel(alabel)
{
	return alabel + "_artifact_label";
}

function __camGetArtifactKey(objlabel)
{
	return objlabel.replace("_artifact_label", "");
}

// called from eventDestroyed
function __camCheckPlaceArtifact(obj)
{
	// FIXME: O(n) lookup here
	var alabel = getLabel(obj);
	if (!camDef(alabel) || !alabel)
	{
		return;
	}
	var ai = __camArtifacts[alabel];
	if (!camDef(ai))
	{
		return;
	}
	if (ai.placed)
	{
		camDebug("Object to which artifact", alabel, "is bound has died twice");
		return;
	}
	camTrace("Placing", ai.tech);
	var acrate = addFeature("Crate", obj.x, obj.y);
	addLabel(acrate, __camGetArtifactLabel(alabel));
	ai.placed = true;
}

function __camPickupArtifact(artifact)
{
	if (artifact.stattype !== ARTIFACT)
	{
		camDebug("Not an artifact");
		return;
	}
	// FIXME: O(n) lookup here
	var alabel = __camGetArtifactKey(getLabel(artifact));
	var ai = __camArtifacts[alabel];
	if (!camDef(alabel) || !alabel || !camDef(ai))
	{
		camTrace("Artifact", artifact.id, "is not managed");
		return;
	}

	camTrace("Picked up", ai.tech);
	playSound("pcv352.ogg", artifact.x, artifact.y, artifact.z);
	// artifacts are not self-removing...
	camSafeRemoveObject(artifact);
	enableResearch(ai.tech);
	// bump counter before the callback, so that it was
	// actual during the callback
	++__camNumArtifacts;
	var callback = __camGlobalContext()["camArtifactPickup_" + alabel];
	if (camDef(callback))
	{
		callback();
	}
}

function __camLetMeWinArtifacts()
{
	for (var alabel in __camArtifacts)
	{
		var ai = __camArtifacts[alabel];
		if (ai.placed)
		{
			var label = __camGetArtifactLabel(alabel);
			var artifact = getObject(label);
			if (!camDef(artifact) || !artifact)
			{
				continue;
			}
			__camPickupArtifact(artifact);
		}
		else
		{
			enableResearch(ai.tech);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Enemy base management.
////////////////////////////////////////////////////////////////////////////////

const CAM_REINFORCE_NONE = 0;
const CAM_REINFORCE_GROUND = 1;
const CAM_REINFORCE_TRANSPORT = 2;

//;; ## camSetEnemyBases(bases)
//;;
//;; Tell ```libcampaign.js``` to manage a certain set of enemy bases.
//;; Management assumes auto-cleanup of leftovers on destruction, and also
//;; counting how many bases have been successfully destroyed by the player.
//;; The argument is a JavaScript map from group labels to base descriptions.
//;; Each label points to a group of vital base structures. If no group label
//;; with this name is defined, a group is created automatically
//;; based on ```cleanup``` area and labeled. Base description
//;; is a JavaScript object with the following optional fields:
//;;
//;; * ```cleanup``` An area label to clean up features in once base is
//;; 	destroyed. If base id is not a group label, this field is required
//;; 	in order to auto-create the group of stuff in the area which doesn't
//;; 	qualify as a valid leftover.
//;; * ```detectMsg``` A ```PROX_MSG``` message id to play when the base is detected.
//;; * ```detectSnd``` A sound file to play when the base is detected.
//;; * ```eliminateSnd``` A sound file to play when the base is eliminated.
//;; 	The sound is played in the center of the cleanup area,
//;; 	which needs to be defined.
//;; * ```player``` If base is detected by cleanup area, only objects
//;; 	matching this player filter would be added to the base group or
//;; 	cleaned up. Note that this most likely disables feature cleanup.
//;; Additionally, this function would call special event callbacks if they are
//;; defined in your script, which should be named as follows,
//;; where LABEL is the label of the base group:
//;; * ```camEnemyBaseDetected_LABEL``` Called when the player sees an object from
//;; 	the enemy base group for the first time.
//;; * ```camEnemyBaseEliminated_LABEL``` Called when the base is eliminated,
//;; 	right after leftovers were cleaned up.
//;;
function camSetEnemyBases(bases)
{
	var reload = !camDef(bases);
	if (!reload)
	{
		__camEnemyBases = bases;
		__camNumEnemyBases = 0;
	}
	// convert label strings to groups and store
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		var obj = getObject(blabel);
		if (camDef(obj) && obj) // group already defined
		{
			if (!camDef(bi.group))
			{
				bi.group = obj.id;
			}
			else
			{
				var structures = enumGroup(bi.group);
				addLabel({ type: GROUP, id: bi.group }, blabel);
				for (var i = 0, l = structures.length; i < l; ++i)
				{
					var s = structures[i];
					if (s.type !== STRUCTURE || __camIsValidLeftover(s))
					{
						continue;
					}
					if (!camDef(bi.player) || camPlayerMatchesFilter(s.player, bi.player))
					{
						camTrace("Auto-adding", s.id, "to base", blabel);
						groupAdd(bi.group, s);
					}
				}
			}
			if (!camDef(bi.cleanup)) // auto-detect cleanup area
			{
				var objs = enumGroup(bi.group);
				if (objs.length > 0)
				{
					var a = {
						type: AREA,
						x: mapWidth, y: mapHeight,
						x2: 0, y2: 0
					};
					// smallest rectangle to contain all objects
					for (var i = 0, l = objs.length; i < l; ++i)
					{
						var o = objs[i];
						if (o.x < a.x) a.x = o.x;
						if (o.y < a.y) a.y = o.y;
						if (o.x > a.x2) a.x2 = o.x;
						if (o.y > a.y2) a.y2 = o.y;
					}
					// but a bit wider
					a.x -= 2; a.y -= 2; a.x2 += 2; a.y2 += 2;
					camTrace("Auto-detected cleanup area for", blabel, ":",
					         a.x, a.y,
					         a.x2, a.y2);
					bi.cleanup = "__cam_enemy_base_cleanup__" + blabel;
					addLabel(a, bi.cleanup);
				}
			}
		}
		else // define a group automatically
		{
			if (!camDef(bi.cleanup))
			{
				camDebug("Neither group nor cleanup area found for", blabel);
				continue;
			}
			bi.group = camNewGroup();
			addLabel({ type: GROUP, id: bi.group }, blabel);
			var structs = enumArea(bi.cleanup, ENEMIES, false);
			for (var i = 0, l = structs.length; i < l; ++i)
			{
				var s = structs[i];
				if (s.type !== STRUCTURE || __camIsValidLeftover(s))
				{
					continue;
				}
				if (!camDef(bi.player) || camPlayerMatchesFilter(s.player, bi.player))
				{
					camTrace("Auto-adding", s.id, "to base", blabel);
					groupAdd(bi.group, s);
				}
			}
		}
		if (groupSize(bi.group) === 0)
		{
			camDebug("Base", blabel, "defined as empty group");
		}
		if(!reload)
		{
			bi.detected = false;
			bi.eliminated = false;
		}
		camTrace("Resetting label", blabel);
		resetLabel(blabel, CAM_HUMAN_PLAYER); // subscribe for eventGroupSeen
	}
}

//;; ## camDetectEnemyBase(base label)
//;;
//;; Plays the "enemy base detected" message and places a beacon
//;; for the enemy base defined by the label, as if the base
//;; was actually found by the player.
//;;
function camDetectEnemyBase(blabel)
{
	var bi = __camEnemyBases[blabel];
	if (bi.detected || bi.eliminated)
	{
		return;
	}
	camTrace("Enemy base", blabel, "detected");
	bi.detected = true;
	if (camDef(bi.detectSnd))
	{
		var pos = camMakePos(bi.cleanup);
		if (!camDef(pos)) // auto-detect sound position by group object pos
		{
			var objs = enumGroup(bi.group);
			if (objs.length > 0)
			{
				pos = camMakePos(objs[0]);
			}
		}
		if (camDef(pos))
		{
			playSound(bi.detectSnd, pos.x, pos.y, 0);
		}
	}
	if (camDef(bi.detectMsg))
	{
		hackAddMessage(bi.detectMsg, PROX_MSG, 0, false);
	}
	var callback = __camGlobalContext()["camEnemyBaseDetected_" + blabel];
	if (camDef(callback))
	{
		callback();
	}
}

//;; ## camAllEnemyBasesEliminated()
//;;
//;; Returns true if all enemy bases managed by ```libcampaign.js```
//;; are destroyed.
//;;
function camAllEnemyBasesEliminated()
{
	// FIXME: O(n) lookup here
	return __camNumEnemyBases === Object.keys(__camEnemyBases).length;
}

//;; ## camSendReinforcement(player, position, droids, kind, data)
//;;
//;; Give a single bunch of droids (template list) for a player at
//;; a position label. Kind can be one of:
//;;
//;; * ```CAM_REINFORCE_GROUND``` Reinforcements magically appear
//;; 	on the ground.
//;; * ```CAM_REINFORCE_TRANSPORT``` Reinforcements are unloaded from
//;; 	a transporter.
//;; 	__NOTE:__ the game engine doesn't seem to support two simultaneous
//;; 	incoming transporters for the same player. Avoid this at all costs!
//;; 	The following data fields are required:
//;;   * ```entry``` Transporter entry position.
//;;   * ```exit``` Transporter exit position.
//;;   * ```message``` ```PROX_MSG``` to display when transport is landing.
//;;   * ```order``` Order to give to newly landed droids
//;;   * ```data``` Order data.
//;; 	 __NOTE:__ the game engine doesn't seem to support two simultaneous
//;; 	incoming transporters for the same player. If a transporter is already
//;; 	on map, it will be correctly queued up and sent later.
//;;
function camSendReinforcement(player, position, list, kind, data)
{
	var pos = camMakePos(position);
	var order = CAM_ORDER_ATTACK;
	var order_data = { regroup: false, count: -1 };
	if (camDef(data) && camDef(data.order))
	{
		order = data.order;
		if (camDef(data.data))
		{
			order_data = data.data;
		}
	}
	switch(kind)
	{
		case CAM_REINFORCE_GROUND:
			var droids = [];
			for (var i = 0, l = list.length; i < l; ++i)
			{
				var prop = __camChangePropulsionOnDiff(list[i].prop);
				droids[droids.length] = addDroid(player, pos.x, pos.y,
				                        "Reinforcement", list[i].body,
				                        prop, "", "", list[i].weap);
			}
			camManageGroup(camMakeGroup(droids), order, order_data);
			break;
		case CAM_REINFORCE_TRANSPORT:
			__camTransporterQueue[__camTransporterQueue.length] = {
				player: player,
				position: position,
				list: list,
				data: data,
				order: order,
				order_data: order_data
			};
			__camDispatchTransporterSafe();
			break;
		default:
			camTrace("Unknown reinforcement type");
			break;
	}
}

//;; ## camSetBaseReinforcements(base label, interval, callback, kind, data)
//;;
//;; Periodically brings reinforcements to an enemy base, until the base is
//;; eliminated. Interval is the pause, in milliseconds, between reinforcements.
//;; Callback is name of a function that returns a list of droid templates to spawn,
//;; which may be different every time. Kind and data work similarly to
//;; ```camSendReinforcement()```.
//;; Use CAM_REINFORCE_NONE as kind to disable previously set reinforcements.
//;;
function camSetBaseReinforcements(blabel, interval, callback, kind, data)
{
	if (!camIsString(callback))
	{
		camDebug("Callback name must be a string (received", callback, ")");
	}
	var bi = __camEnemyBases[blabel];
	bi.reinforce_kind = kind;
	bi.reinforce_interval = interval;
	bi.reinforce_callback = callback;
	bi.reinforce_data = data;
}

//;; ## camRemoveEnemyTransporterBlip()
//;;
//;; Removes the last blip that an enemy transporter left behind, if any.
//;;
function camRemoveEnemyTransporterBlip()
{
	if (camDef(__camTransporterMessage))
	{
		hackRemoveMessage(__camTransporterMessage, PROX_MSG, CAM_HUMAN_PLAYER);
		__camTransporterMessage = undefined;
	}
}

//////////// privates

var __camEnemyBases;
var __camNumEnemyBases;
var __camPlayerTransports;
var __camIncomingTransports;
var __camTransporterQueue;
var __camTransporterMessage;

// returns true if transporter was launched,
// false if transporter is already on map,
// throws error if queue is empty.
function __camDispatchTransporterUnsafe()
{
	if (__camTransporterQueue.length === 0)
	{
		camDebug("Transporter queue empty!");
		return false;
	}
	var args = __camTransporterQueue[0];
	var player = args.player;
	var pos = args.position;
	var list = args.list;
	var data = args.data;
	if (camDef(__camIncomingTransports[player]))
	{
		camTrace("Transporter already on map for player", player + ", delaying.");
		return false;
	}
	__camTransporterQueue.shift(); // what could possibly go wrong?
	if (!camDef(__camPlayerTransports[player]))
	{
		camTrace("Creating a transporter for player", player);
		__camPlayerTransports[player] = addDroid(player, -1, -1,
		                                         "Transporter",
		                                         "TransporterBody",
		                                         "V-Tol", "", "",
		                                         "MG3-VTOL");
	}
	var trans = __camPlayerTransports[player];
	var droids = [];
	for (var i = 0, l = list.length; i < l; ++i)
	{
		var prop = __camChangePropulsionOnDiff(list[i].prop);
		var droid = addDroid(player, -1, -1,
		                     "Reinforcement", list[i].body,
		                     prop, "", "", list[i].weap);
		droids.push(droid);
		addDroidToTransporter(trans, droid);
	}
	__camIncomingTransports[player] = {
		droids: droids,
		message: args.data.message,
		order: args.order,
		data: args.order_data,
	};
	camTrace("Incoming transport with", droids.length,
	         "droids for player", player +
	         ", queued transports", __camTransporterQueue.length);

	setNoGoArea(pos.x - 2, pos.y - 2, pos.x + 2, pos.y + 2, player);

	//Delete previous enemy reinforcement transport blip
	if (player !== CAM_HUMAN_PLAYER)
	{
		camRemoveEnemyTransporterBlip();
	}

	if(player !== CAM_HUMAN_PLAYER)
	{
		playSound("pcv381.ogg"); //Enemy transport detected.
	}

	setTransporterExit(data.exit.x, data.exit.y, player);
	// will guess which transporter to start, automagically
	startTransporterEntry(data.entry.x, data.entry.y, player);
	return true;
}

function __camDispatchTransporterSafe(player, position, list, data)
{
	if (!profile("__camDispatchTransporterUnsafe"))
	{
		queue("__camDispatchTransporterSafe", 1000);
	}
}

function __camLandTransporter(player, pos)
{
	var ti = __camIncomingTransports[player];
	if (!camDef(ti))
	{
		camDebug("Unhandled transporter for player", player);
		return;
	}
	if (camDef(ti.message))
	{
		__camTransporterMessage = ti.message;
		hackAddMessage(ti.message, PROX_MSG, 0, false);
	}
	else
	{
		__camTransporterMessage = undefined;
	}
	camTrace("Landing transport for player", player);
	playSound("pcv395.ogg", pos.x, pos.y, 0); //Incoming enemy transport.
	camManageGroup(camMakeGroup(ti.droids), ti.order, ti.data);
}

function __camCheckBaseSeen(seen)
{
	var group = seen; // group?
	if (camDef(seen.group)) // object?
	{
		group = seen.group;
	}
	if (!camDef(group) || !group)
	{
		return;
	}
	// FIXME: O(n) lookup here
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.group !== group)
		{
			continue;
		}
		camDetectEnemyBase(blabel);
	}
}

function __camIsValidLeftover(obj)
{
	if (camPlayerMatchesFilter(obj.player, ENEMIES))
	{
		if (obj.type === STRUCTURE && obj.stattype === WALL)
		{
			return true;
		}
	}
	if (obj.type === FEATURE)
	{
		if (obj.stattype === BUILDING)
		{
			return true;
		}
	}
	return false;
}

function __camCheckBaseEliminated(group)
{
	// FIXME: O(n) lookup here
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.eliminated || (bi.group !== group))
		{
			continue;
		}
		if (enumGroup(bi.group).length !== 0)
		{
			return; //still has something in the base group
		}
		if (camDef(bi.cleanup))
		{
			var leftovers = enumArea(bi.cleanup, ENEMIES, false).filter(function(obj) {
				return (obj.type === STRUCTURE
					&& obj.status === BUILT
					&& __camIsValidLeftover(obj)
					&& (!camDef(bi.player)
					|| (camDef(bi.player) && camPlayerMatchesFilter(obj.player, bi.player)))
				);
			});
			for (var i = 0, l = leftovers.length; i < l; i++)
			{
				// remove with special effect
				camSafeRemoveObject(leftovers[i], true);
			}
			if (camDef(bi.eliminateSnd))
			{
				// play sound
				var pos = camMakePos(bi.cleanup);
				playSound(bi.eliminateSnd, pos.x, pos.y, 0);
			}
		}
		else
		{
			camDebug("All bases must have a cleanup area : " + blabel);
			continue;
		}
		if (camDef(bi.detectMsg) && bi.detected) // remove the beacon
		{
			hackRemoveMessage(bi.detectMsg, PROX_MSG, CAM_HUMAN_PLAYER);
		}
		camTrace("Enemy base", blabel, "eliminated");
		bi.eliminated = true;
		// bump counter before the callback, so that it was
		// actual during the callback
		++__camNumEnemyBases;
		var callback = __camGlobalContext()["camEnemyBaseEliminated_" + blabel];
		if (camDef(callback))
		{
			callback();
		}
	}
}

function __camBasesTick()
{
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.eliminated || !camDef(bi.reinforce_kind))
		{
			continue;
		}
		if (gameTime - bi.reinforce_last < bi.reinforce_interval)
		{
			continue;
		}
		if (!camDef(bi.player))
		{
			camDebug("Enemy base player needs to be set for", blabel);
			return;
		}
		if (!camDef(bi.cleanup))
		{
			camDebug("Enemy base cleanup area needs to be set for", blabel);
			return;
		}
		bi.reinforce_last = gameTime;
		var list = profile(bi.reinforce_callback);
		var pos = camMakePos(bi.cleanup);
		camSendReinforcement(bi.player, pos, list,
		                     bi.reinforce_kind, bi.reinforce_data);
	}
}

////////////////////////////////////////////////////////////////////////////////
// AI droid movement automation.
////////////////////////////////////////////////////////////////////////////////

const CAM_ORDER_ATTACK = 0;
const CAM_ORDER_DEFEND = 1;
const CAM_ORDER_PATROL = 2;
const CAM_ORDER_COMPROMISE = 3;
const CAM_ORDER_FOLLOW = 4;

//;; ## camManageGroup(group, order, data)
//;;
//;; Tell ```libcampaign.js``` to manage a certain group. The group
//;; would be permanently managed depending on the high-level orders given.
//;; For each order, data parameter is a JavaScript object that controls
//;; different aspects of behavior. The order parameter is one of:
//;;
//;; * ```CAM_ORDER_ATTACK``` Pursue human player, preferably around
//;; 	the given position. The following optional data object fields are
//;; 	available, none of which is required:
//;;   * ```pos``` Position or list of positions to attack. If pos is a list,
//;; 		first positions in the list will be attacked first.
//;;   * ```radius``` Circle radius around ```pos``` to scan for targets.
//;;   * ```fallback``` Position to retreat.
//;;   * ```morale``` An integer from 1 to 100. If that high percentage
//;; 		of the original group dies, fall back to the fallback position.
//;; 		If new droids are added to the group, it can recover and attack
//;; 		again.
//;;   * ```count``` Override size of the original group. If unspecified,
//;; 		number of droids in the group at call time. Retreat on low morale
//;; 		and regroup is calculated against this value.
//;;   * ```repair``` Health percentage to fall back to repair facility,
//;; 		if any.
//;;   * ```regroup``` If set to true, the group will not move forward unless
//;; 		it has at least ```count``` droids in its biggest cluster.
//;; 		If ```count``` is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; * ```CAM_ORDER_DEFEND``` Protect the given position. If too far, retreat
//;; 	back there ignoring fire. The following data object fields are
//;; 	available:
//;;   * ```pos``` Position to defend.
//;;   * ```radius``` Circle radius around ```pos``` to scan for targets.
//;;   * ```count``` Override size of the original group. If unspecified,
//;; 		number of droids in the group at call time. Regroup is calculated
//;; 		against this value.
//;;   * ```repair``` Health percentage to fall back to repair facility,
//;; 		if any.
//;;   * ```regroup``` If set to true, the group will not move forward unless
//;; 		it has at least ```count``` droids in its biggest cluster.
//;; 		If ```count``` is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; * ```CAM_ORDER_PATROL``` Move droids randomly between a given list of
//;; 	positions. The following data object fields are available:
//;;   * ```pos``` An array of positions to patrol between.
//;;   * ```interval``` Change positions every this many milliseconds.
//;;   * ```count``` Override size of the original group. If unspecified,
//;; 		number of droids in the group at call time. Regroup is calculated
//;; 		against this value.
//;;   * ```repair``` Health percentage to fall back to repair facility,
//;; 		if any.
//;;   * ```regroup``` If set to true, the group will not move forward unless
//;; 		it has at least ```count``` droids in its biggest cluster.
//;; 		If ```count``` is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; * ```CAM_ORDER_COMPROMISE``` Same as CAM_ORDER_ATTACK, just stay near the
//;; 	last (or only) attack position instead of looking for the player
//;; 	around the whole map. Useful for offworld missions,
//;; 	with player's LZ as the final position. The following data object fields
//;; 	are available:
//;;   * ```pos``` Position or list of positions to compromise. If pos is a list,
//;; 		first positions in the list will be compromised first.
//;;   * ```radius``` Circle radius around ```pos``` to scan for targets.
//;;   * ```count``` Override size of the original group. If unspecified,
//;; 		number of droids in the group at call time. Regroup is calculated
//;; 		against this value.
//;;   * ```repair``` Health percentage to fall back to repair facility,
//;; 		if any.
//;;   * ```regroup``` If set to true, the group will not move forward unless
//;; 		it has at least ```count``` droids in its biggest cluster.
//;; 		If ```count``` is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; * ```CAM_ORDER_FOLLOW``` Assign the group to commander. The sub-order
//;; 	is defined to be given to the commander. When commander dies,
//;; 	the group continues to execute the sub-order. The following data object
//;; 	fields are available:
//;;   * ```droid``` Commander droid label.
//;;   * ```order``` The order to give to the commander.
//;;   * ```data``` Data of the commander's order.
//;;   * ```repair``` Health percentage to fall back to repair facility, if any.
//;;
function camManageGroup(group, order, data)
{
	var saneData = data;
	if (!camDef(saneData))
	{
		saneData = {};
	}
	if (camDef(saneData.pos)) // sanitize pos now to make ticks faster
	{
		if (camIsString(saneData.pos)) // single label?
		{
			saneData.pos = [ saneData.pos ];
		}
		else if (!camDef(saneData.pos.length)) // single position object?
		{
			saneData.pos = [ saneData.pos ];
		}
		for (var i = 0, l = saneData.pos.length; i < l; ++i) // array of labels?
		{
			saneData.pos[i] = camMakePos(saneData.pos[i]);
		}
	}
	if (camDef(__camGroupInfo[group]) && order !== __camGroupInfo[group].order)
	{
		camTrace("Group", group, "receives a new order:", camOrderToString(order));
	}
	__camGroupInfo[group] = {
		target: undefined,
		order: order,
		data: saneData,
		count: camDef(saneData.count) ? saneData.count : groupSize(group)
	};
	if (order === CAM_ORDER_FOLLOW)
	{
		var commanderGroup = camMakeGroup([getObject(data.droid)]);
		camManageGroup(commanderGroup, data.order, data.data);
	}
	// apply orders instantly
	queue("__camTacticsTickForGroup", CAM_TICKS_PER_FRAME, group);
}

//;; ## camStopManagingGroup(group)
//;;
//;; Tell ```libcampaign.js``` to stop managing a certain group.
//;;
function camStopManagingGroup(group)
{
	if (!camDef(__camGroupInfo[group]))
	{
		camTrace("Not managing", group, "anyway");
		return;
	}
	camTrace("Cease managing", group);
	delete __camGroupInfo[group];
}

//;; ## camManageTrucks(player)
//;;
//;; Manage trucks for an AI player. This assumes recapturing oils and
//;; rebuilding destroyed trucks in factories, the latter is implemented
//;; via ```camQueueDroidProduction()``` mechanism.
//;;
function camManageTrucks(player)
{
	__camTruckInfo[player] = { enabled: 1, queue: [] };
}

//;; ## camQueueBuilding(player, stat[, pos])
//;;
//;; Assuming truck management is enabled for the player, issue an order
//;; to build a specific building near a certain position. The order
//;; would be issued once as soon as a free truck becomes available. It will
//;; not be re-issued in case the truck is destroyed before the building
//;; is finished. If position is unspecified, the building would be built
//;; near the first available truck. Otherwise, position may be a label
//;; or a POSITION-like object.
//;;
function camQueueBuilding(player, stat, pos)
{
	var ti = __camTruckInfo[player];
	ti.queue[ti.queue.length] = { stat: stat, pos: camMakePos(pos) };
}

//;; ## camOrderToString(order)
//;;
//;; Print campaign order as string, useful for debugging.
//;;
function camOrderToString(order)
{
	switch(order)
	{
		case CAM_ORDER_ATTACK:
			return "ATTACK";
		case CAM_ORDER_DEFEND:
			return "DEFEND";
		case CAM_ORDER_PATROL:
			return "PATROL";
		case CAM_ORDER_COMPROMISE:
			return "COMPROMISE";
		case CAM_ORDER_FOLLOW:
			return "FOLLOW";
		default:
			return "UNKNOWN";
	}
}

//;; ## camSortByHealth(object 1, object 2)
//;;
//;; Use this to sort an array of objects by health value.
//;;
function camSortByHealth(a, b)
{
	return (a.health - b.health);
}

//////////// privates

var __camGroupInfo;
var __camTruckInfo;

const __CAM_TARGET_TRACKING_RADIUS = 5;
const __CAM_PLAYER_BASE_RADIUS = 20;
const __CAM_DEFENSE_RADIUS = 4;
const __CAM_CLOSE_RADIUS = 2;
const __CAM_CLUSTER_SIZE = 4;
const __CAM_FALLBACK_TIME_ON_REGROUP = 5000;

function __camFindClusters(list, size)
{
	// The good old cluster analysis algorithm taken from NullBot AI.
	var ret = { clusters: [], xav: [], yav: [], maxIdx: 0, maxCount: 0 };
	for (var i = list.length - 1; i >= 0; --i)
	{
		var x = list[i].x;
		var y = list[i].y;
		var found = false;
		for (var j = 0; j < ret.clusters.length; ++j)
		{
			if (camDist(ret.xav[j], ret.yav[j], x, y) < size)
			{
				var n = ret.clusters[j].length;
				ret.clusters[j][n] = list[i];
				ret.xav[j] = Math.floor((n * ret.xav[j] + x) / (n + 1));
				ret.yav[j] = Math.floor((n * ret.yav[j] + y) / (n + 1));
				if (ret.clusters[j].length > ret.maxCount)
				{
					ret.maxIdx = j;
					ret.maxCount = ret.clusters[j].length;
				}
				found = true;
				break;
			}
		}
		if (!found)
		{
			var n = ret.clusters.length;
			ret.clusters[n] = [list[i]];
			ret.xav[n] = x;
			ret.yav[n] = y;
			if (1 > ret.maxCount)
			{
				ret.maxIdx = n;
				ret.maxCount = 1;
			}
		}
	}
	return ret;
}

function __camPickTarget(group)
{
	var targets = [];
	var gi = __camGroupInfo[group];
	var droids = enumGroup(group);
	switch(gi.order)
	{
		case CAM_ORDER_ATTACK:
			if (camDef(gi.target))
			{
				targets = enumRange(gi.target.x, gi.target.y,
				                    __CAM_TARGET_TRACKING_RADIUS,
				                    CAM_HUMAN_PLAYER, false);
			}
			// fall-through! we just don't track targets on COMPROMISE
		case CAM_ORDER_COMPROMISE:
			if (camDef(gi.data.pos))
			{
				for (var i = 0; i < gi.data.pos.length && !targets.length; ++i)
				{
					var radius = gi.data.radius;
					if (!camDef(radius))
					{
						radius = __CAM_PLAYER_BASE_RADIUS;
					}
					targets = enumRange(gi.data.pos[i].x,
					                    gi.data.pos[i].y,
					                    radius,
					                    CAM_HUMAN_PLAYER, false);
				}
			}
			if (gi.order === CAM_ORDER_COMPROMISE && !targets.length)
			{
				if (!camDef(gi.data.pos))
				{
					camDebug("'pos' is required for COMPROMISE order");
					return undefined;
				}
				else
				{
					targets = [ gi.data.pos[gi.data.pos.length - 1] ];
				}
			}
			var dr = droids[0];
			targets = targets.filter(function(obj) {
				return propulsionCanReach(dr.propulsion, dr.x, dr.y, obj.x, obj.y);
			});
			if (!targets.length)
			{
				targets = enumStruct(CAM_HUMAN_PLAYER).filter(function(obj) {
					return propulsionCanReach(dr.propulsion, dr.x, dr.y, obj.x, obj.y);
				});
				if (!targets.length)
				{
					targets = enumDroid(CAM_HUMAN_PLAYER).filter(function(obj) {
						return propulsionCanReach(dr.propulsion, dr.x, dr.y, obj.x, obj.y);
					});
				}
			}
			break;
		case CAM_ORDER_DEFEND:
			if (!camDef(gi.data.pos))
			{
				camDebug("'pos' is required for DEFEND order");
				return undefined;
			}
			var radius = gi.data.radius;
			if (!camDef(radius))
			{
				radius = __CAM_DEFENSE_RADIUS;
			}
			if (camDef(gi.target) && camDist(gi.target, gi.data.pos[0]) < radius)
			{
				targets = enumRange(gi.target.x, gi.target.y,
				                    __CAM_TARGET_TRACKING_RADIUS,
				                    CAM_HUMAN_PLAYER, false);
			}
			if (!targets.length)
			{
				targets = enumRange(gi.data.pos[0].x, gi.data.pos[0].y,
				                    radius, CAM_HUMAN_PLAYER, false);
			}
			if (!targets.length)
			{
				targets = [ gi.data.pos[0] ];
			}
			break;
		default:
			camDebug("Unsupported group order", gi.order, camOrderToString(gi.order));
			break;
	}
	if (!targets.length)
	{
		return undefined;
	}
	var target = targets[0];
	if (camDef(target) && camDef(target.type) && target.type === DROID && camIsTransporter(target))
	{
		return undefined;
	}
	__camGroupInfo[group].target = { x: target.x, y: target.y };
	return __camGroupInfo[group].target;
}

function __camTacticsTick()
{
	var dt = CAM_TICKS_PER_FRAME;
	for (var group in __camGroupInfo)
	{
		//Remove groups with no droids.
		if (groupSize(group) === 0)
		{
			var remove = true;
			//Useful if the group has manual management (seen in cam1-3 script).
			if (camDef(__camGroupInfo[group].data.removable) && __camGroupInfo[group].data.removable === false)
			{
				remove = false;
			}
			if (remove === true)
			{
				camStopManagingGroup(group);
				break;
			}
		}
		queue("__camTacticsTickForGroup", dt, group);
		dt += CAM_TICKS_PER_FRAME;
	}
	queue("__camTacticsTick", dt);
}

//Return the range (in tiles) a droid will scout for stuff to attack around it.
function __camScanRange(order, drType)
{
	var rng = __CAM_TARGET_TRACKING_RADIUS; //default
	switch (order)
	{
		case CAM_ORDER_ATTACK:
		case CAM_ORDER_DEFEND:
		case CAM_ORDER_FOLLOW:
			rng = __CAM_TARGET_TRACKING_RADIUS;
			break;
		case CAM_ORDER_PATROL:
			rng = 4;
			break;
		case CAM_ORDER_COMPROMISE:
			rng = 2; //very small so they don't track stuff too far
			break;
		default:
			camDebug("Unsupported group order", order, camOrderToString(order));
	}

	if (drType === DROID_SENSOR)
	{
		rng = Math.floor(rng * 1.5);
	}

	return rng;
}

function __camCountHQ(player)
{
	var count = 0;
	const HQ_TYPE = [
		"A0CommandCentre", "A0CommandCentreNP", "A0CommandCentreCO", "A0CommandCentreNE",
	];
	for (var i = 0, l = HQ_TYPE.length; i < l; ++i)
	{
		count = count + countStruct(HQ_TYPE[i], player);
	}

	return count;
}

function __camTacticsTickForGroup(group)
{
	var gi = __camGroupInfo[group];
	if (!camDef(gi))
	{
		return;
	}
	var rawDroids = enumGroup(group);
	if (rawDroids.length === 0)
	{
		return;
	}
	var healthyDroids = [];
	var hasRepairFacility = (countStruct("A0RepairCentre3", rawDroids[0].player) > 0);
	var hasRTB = __camCountHQ(rawDroids[0].player);
	var hasRepairPos = camDef(gi.data.repairPos);
	var repairPercent = (camDef(gi.data.repair) ? gi.data.repair : 66);

	// Handle the case when we need to repair
	if (rawDroids.length > 0 && (hasRepairFacility || hasRepairPos))
	{
		for (var i = 0, l = rawDroids.length; i < l; ++i)
		{
			var droid = rawDroids[i];
			var repairLikeAction = false;
			//has a repair facility so use it
			if (hasRepairFacility && camDef(gi.data.repair) && droid.order !== DORDER_RTR)
			{
				if (droid.health < gi.data.repair)
				{
					orderDroid(droid, DORDER_RTR);
					repairLikeAction = true;
				}
			}
			//Or they have auto-repair and run to some position for a while
			else if (!hasRepairFacility && hasRepairPos && droid.order !== DORDER_RTR)
			{
				if (droid.health < repairPercent)
				{
					var loc = camMakePos(gi.data.repairPos);
					orderDroidLoc(droid, DORDER_MOVE, loc.x, loc.y);
					repairLikeAction = true;
				}
			}

			if (!repairLikeAction)
			{
				healthyDroids.push(droid);
			}
		}
	}
	else
	{
		healthyDroids = rawDroids;
	}


	if (camDef(gi.data.regroup) && gi.data.regroup && healthyDroids.length > 0)
	{
		var ret = __camFindClusters(healthyDroids, __CAM_CLUSTER_SIZE);
		var groupX = ret.xav[ret.maxIdx];
		var groupY = ret.yav[ret.maxIdx];
		var droids = ret.clusters[ret.maxIdx].filter(function(obj) {
			return (obj.type === DROID && obj.droidType !== DROID_CONSTRUCT);
		});
		for (var i = 0; i < ret.clusters.length; ++i)
		{
			if (i !== ret.maxIdx) // move other droids towards main cluster
			{
				for (var j = 0; j < ret.clusters[i].length; ++j)
				{
					var droid = ret.clusters[i][j];
					if (droid.type === DROID && droid.order !== DORDER_RTR)
					{
						orderDroidLoc(droid, DORDER_MOVE, groupX, groupY);
					}
				}
			}
		}
		var hitRecently = (gameTime - gi.lastHit < __CAM_FALLBACK_TIME_ON_REGROUP);
		// not enough droids grouped?
		if (gi.count < 0 ? (ret.maxCount < groupSize(group) * 0.66) : (ret.maxCount < gi.count))
		{
			for (var i = 0, l = droids.length; i < l; ++i)
			{
				var droid = droids[i];
				if (droid.order !== DORDER_RTR)
				{
					if (hitRecently && hasRTB)
					{
						orderDroid(droid, DORDER_RTB);
					}
					else if (droid.order !== DORDER_HOLD)
					{
						orderDroid(droid, DORDER_HOLD);
					}
				}
			}
			return;
		}
	}

	var droids = healthyDroids.filter(function(dr) {
		return (dr.type === DROID
			&& dr.droidType !== DROID_CONSTRUCT
			&& dr.order !== DORDER_RTR
		);
	});

	//target is only used for attack/defend/compromise
	var target;
	var patrolPos;
	switch (gi.order)
	{
		case CAM_ORDER_ATTACK:
		case CAM_ORDER_DEFEND:
		case CAM_ORDER_COMPROMISE:
			target = profile("__camPickTarget", group);
			if (!camDef(target))
			{
				return;
			}
			break;
		case CAM_ORDER_PATROL:
		case CAM_ORDER_FOLLOW:
			//do nothing here
			break;
		default:
			camDebug("Unknown group order given: " + gi.order);
			return;
	}

	for (var i = 0, l = droids.length; i < l; ++i)
	{
		var droid = droids[i];
		if (droid.player === CAM_HUMAN_PLAYER)
		{
			camDebug("Controlling a human player's droid");
		}
		//Rearm vtols.
		if (droid.type === DROID && isVTOL(droid))
		{
			var arm = Math.floor(droid.weapons[0].armed);
			var isRearming = droid.order === DORDER_REARM;
			var health = Math.floor(droid.health);
			if ((arm < 1) || (isRearming && (arm < 100 || health < 100)))
			{
				var pads = countStruct("A0VtolPad", droid.player);
				if (pads && !isRearming)
				{
					orderDroid(droid, DORDER_REARM);
				}
				continue; //Rearming. Try not to attack stuff.
			}
		}

		if (gi.order === CAM_ORDER_FOLLOW)
		{
			var commander = getObject(gi.data.droid);
			if (commander === null)
			{
				// the commander is dead? let the group execute his last will.
				camManageGroup(group, gi.data.order, gi.data.data);
				return;
			}
			if (droid.droidType !== DROID_COMMAND && droid.order !== DORDER_COMMANDERSUPPORT)
			{
				orderDroidObj(droid, DORDER_COMMANDERSUPPORT, commander);
				continue;
			}
		}

		if (gi.order === CAM_ORDER_DEFEND)
		{
			// fall back to defense position
			var dist = camDist(droid, gi.data.pos);
			var radius = gi.data.radius;
			if (!camDef(radius))
			{
				radius = __CAM_DEFENSE_RADIUS;
			}
			if (dist > radius)
			{
				orderDroidLoc(droid, DORDER_MOVE, gi.data.pos.x, gi.data.pos.y);
				continue;
			}
		}

		//As of this refactor patrol groups can react to stuff within the
		//tile scan limits in the attack code while waiting
		var patrol = (gi.order === CAM_ORDER_PATROL);
		if (patrol)
		{
			if (!camDef(gi.data.interval))
			{
				gi.data.interval = 60000;
			}
			if (!camDef(gi.lastmove) || !camDef(gi.lastspot))
			{
				gi.lastspot = 0;
				gi.lastmove = 0;
			}
			else
			{
				if (gameTime - gi.lastmove > gi.data.interval)
				{
					// find random new position to visit
					var list = [];
					for (var i = 0, l = gi.data.pos.length; i < l; ++i)
					{
						if (i !== gi.lastspot)
						{
							list[list.length] = i;
						}
					}
					gi.lastspot = list[camRand(list.length)];
					gi.lastmove = gameTime;
				}
			}
			patrolPos = gi.data.pos[gi.lastspot];
			//I will leave this here for clarity so that it don't look
			//like patrol picks a target.
			if (camDef(patrolPos))
			{
				target = camMakePos(patrolPos);
			}
		}

		if (patrol || (camDef(target) && camDist(droid, target) >= __CAM_CLOSE_RADIUS))
		{
			const SCAN_RANGE = __camScanRange(gi.order, droid.droidType);
			var defending = (gi.order === CAM_ORDER_DEFEND);

			var closeBy = enumRange(droid.x, droid.y, SCAN_RANGE, CAM_HUMAN_PLAYER, false);
			closeBy = closeBy.sort(function(obj1, obj2) {
				var temp1 = distBetweenTwoPoints(droid.x, droid.y, obj1.x, obj1.y);
				var temp2 = distBetweenTwoPoints(droid.x, droid.y, obj2.x, obj2.y);
				return (temp1 - temp2);
			});

			if (!defending && camDef(closeBy[0]))
			{
				if (droid.droidType === DROID_SENSOR)
				{
					orderDroidObj(droid, DORDER_OBSERVE, closeBy[0]);
				}
				else
				{
					orderDroidObj(droid, DORDER_ATTACK, closeBy[0]);
				}
			}
			else
			{
				if (!defending)
				{
					orderDroidLoc(droid, DORDER_SCOUT, target.x, target.y);
				}
				else
				{
					orderDroidLoc(droid, DORDER_MOVE, target.x, target.y);
				}
			}
		}
	}
}

function __camCheckGroupMorale(group)
{
	var gi = __camGroupInfo[group];
	if (!camDef(gi.data.morale))
	{
		return;
	}
	// morale is %.
	var msize = Math.floor((100 - gi.data.morale) * gi.count / 100);
	var gsize = groupSize(group);
	switch (gi.order)
	{
		case CAM_ORDER_ATTACK:
			if (gsize > msize)
			{
				break;
			}
			camTrace("Group", group, "falls back");
			gi.order = CAM_ORDER_DEFEND;
			// swap pos and fallback
			var temp = gi.data.pos;
			gi.data.pos = [ camMakePos(gi.data.fallback) ];
			gi.data.fallback = temp;
			// apply orders instantly
			queue("__camTacticsTickForGroup", CAM_TICKS_PER_FRAME, group);
			break;
		case CAM_ORDER_DEFEND:
			if (gsize <= msize)
			{
				break;
			}
			camTrace("Group", group, "restores");
			gi.order = CAM_ORDER_ATTACK;
			// swap pos and fallback
			var temp = gi.data.pos;
			gi.data.pos = gi.data.fallback;
			gi.data.fallback = temp[0];
			// apply orders instantly
			queue("__camTacticsTickForGroup", CAM_TICKS_PER_FRAME, group);
			break;
		default:
			camDebug("Group order doesn't support morale", camOrderToString(gi.order));
			break;
	}
}

//Reset AI power back to highest storage possible.
function __camAiPowerReset()
{
	for (var i = 1; i < CAM_MAX_PLAYERS; ++i)
	{
		setPower(AI_POWER, i);
	}
}

function __camEnumFreeTrucks(player)
{
	var rawDroids = enumDroid(player, DROID_CONSTRUCT);
	var droids = [];
	for (var i = 0, l = rawDroids.length; i < l; ++i)
	{
		var droid = rawDroids[i];
		if (droid.order !== DORDER_BUILD && droid.order !== DORDER_HELPBUILD
		                                 && droid.order !== DORDER_LINEBUILD)
		{
			droids.push(droid);
		}
	}
	return droids;
}

function __camGetClosestTruck(player, pos)
{
	var droids = __camEnumFreeTrucks(player);
	if (droids.length <= 0)
	{
		return undefined;
	}

	// Find out which one is the closest.
	var minDroid = droids[0];
	var minDist = camDist(minDroid, pos);
	for (var i = 1, l = droids.length; i < l; ++i)
	{
		var droid = droids[i];
		if (!droidCanReach(droid, pos.x, pos.y))
			continue;
		var dist = camDist(droid, pos);
		if (dist < minDist)
		{
			minDist = dist;
			minDroid = droid;
		}
	}
	return minDroid;
}

function __camTruckTick()
{
	// Issue truck orders for each player.
	// See comments inside the loop to understand priority.
	for (var player in __camTruckInfo)
	{
		var ti = __camTruckInfo[player];

		// First, build things that were explicitly ordered.
		while (ti.queue.length > 0)
		{
			var qi = ti.queue[0];
			var pos = qi.pos;
			var truck;
			if (camDef(pos))
			{
				// Find the truck most suitable for the job.
				truck = __camGetClosestTruck(player, pos);
				if (!camDef(truck))
				{
					break;
				}
			}
			else
			{
				// Build near any truck if pos was not specified.
				var droids = __camEnumFreeTrucks(player);
				if (droids.length <= 0)
				{
					break;
				}
				truck = droids[0];
				pos = truck;
			}

			enableStructure(qi.stat, player);
			var loc = pickStructLocation(truck, qi.stat, pos.x, pos.y);
			if (camDef(loc) && camDef(truck))
			{
				if (orderDroidBuild(truck, DORDER_BUILD, qi.stat, loc.x, loc.y))
				{
					ti.queue.shift(); // consider it handled
				}
			}
		}

		// Then, capture free oils.
		var oils = enumFeature(-1, "OilResource");
		if (oils.length === 0)
		{
			continue;
		}
		var oil = oils[0];
		var truck = __camGetClosestTruck(player, oil);
		if (camDef(truck))
		{
			enableStructure("A0ResourceExtractor", player);
			orderDroidBuild(truck, DORDER_BUILD, "A0ResourceExtractor", oil.x, oil.y);
			continue;
		}
	}
}

// called from eventDestroyed
function __camCheckDeadTruck(obj)
{
	if (camDef(__camTruckInfo[obj.player]))
	{
		// multi-turret templates are not supported yet
		// cyborg engineers are not supported yet
		// cannot use obj.weapons[] because spade is not a weapon
		camQueueDroidProduction(obj.player, {
			body: obj.body,
			prop: obj.propulsion,
			weap: "Spade1Mk1"
		});
	}
}

////////////////////////////////////////////////////////////////////////////////
// Factory production management.
////////////////////////////////////////////////////////////////////////////////
// To use this feature, call camSetFactories() with a list of factories.
// This assumes tactical management of groups of units produced from them.
// Factories won't start production immediately; call camEnableFactory()
// to turn them on.

//;; ## camSetFactories(factories)
//;;
//;; Tell ```libcampaign.js``` to manage a certain set of enemy factories.
//;; Management assumes producing droids, packing them into groups and
//;; executing orders once the group becomes large-enough.
//;; The argument is a JavaScript map from group labels to factory descriptions.
//;; Each label points to a factory object. Factory description
//;;
//;; is a JavaScript object with the following fields:
//;; * ```assembly``` A rally point position label, where the group would
//;; 	gather.
//;; * ```order``` An order to execute for every group produced in the
//;; 	factory. Same as the order parameter for ```camManageGroup()```.
//;; * ```data``` Order data. Same as the data parameter for
//;; 	```camManageGroup()```.
//;; * ```groupSize``` Number of droids to produce before executing the order.
//;; 	Also, if order is ```CAM_ORDER_ATTACK```, data.count defaults to this value.
//;; * ```maxSize``` Halt production when reaching that many droids in the
//;; 	factory group. Resume when some droids die. Unlimited if unspecified.
//;; * ```throttle``` If defined, produce droids only every that many
//;; 	milliseconds, and keep the factory idle between ticks.
//;; * ```group``` If defined, make the factory manage this group,
//;; 	otherwise create a new empty group to manage.
//;; 	Droids produced in the factory would automatically be
//;; 	added to the group, and order and data parameters
//;; 	would be applied to this group.
//;; * ```templates``` List of droid templates to produce in the factory.
//;; 	Each template is a JavaScript object with the following fields:
//;;   * ```body``` Body stat name.
//;;   * ```prop``` Propulsion stat name.
//;;   * ```weap``` Weapon stat name. Only single-turret droids are
//;; 		currently supported.
//;; 	Note that all template components are automatically made available
//;; 	to the factory owner.
//;; Factories won't start production immediately; call
//;; ```camEnableFactory()``` to turn them on when necessary.
//;;
function camSetFactories(factories)
{
	for (var flabel in factories)
	{
		camSetFactoryData(flabel, factories[flabel]);
	}
}

//;; ## camSetFactoryData(factory label, factory description)
//;;
//;; Similar to ```camSetFactories()```, but one factory at a time.
//;; If the factory was already managing a group of droids, it keeps
//;; managing it. If a new group is specified in the description,
//;; the old group is merged into it. NOTE: This function disables the
//;; factory. You would need to call ```camEnableFactory()``` again.
//;;
function camSetFactoryData(flabel, fdata)
{
	var structure = getObject(flabel);
	if (!camDef(structure) || !structure)
	{
		// Not an error! It's ok if the factory is already destroyed
		// when its data was updated.
		camTrace("Factory", flabel, "not found");
		return;
	}
	// remember the old factory group, if any
	var droids = [];
	if (camDef(__camFactoryInfo[flabel]))
	{
		droids = enumGroup(__camFactoryInfo[flabel].group);
	}
	__camFactoryInfo[flabel] = fdata;
	var fi = __camFactoryInfo[flabel];
	if (!camDef(fi.data))
	{
		fi.data = {};
	}
	fi.enabled = false;
	fi.state = 0;
	if (!camDef(fi.group))
	{
		fi.group = camNewGroup();
	}
	for (var i = 0, l = droids.length; i < l; ++i)
	{
		groupAdd(fi.group, droids[i]);
	}
	if (!camDef(fi.data.count))
	{
		fi.data.count = fi.groupSize;
	}
}

//;; ## camEnableFactory(factory label)
//;;
//;; Enable a managed factory by the given label. Once the factory is enabled,
//;; it starts producing units and executing orders as given.
//;;
function camEnableFactory(flabel)
{
	var fi = __camFactoryInfo[flabel];
	if (!camDef(fi) || !fi)
	{
		camDebug("Factory not managed", flabel);
		return;
	}
	if (fi.enabled)
	{
		// safe, no error
		camTrace("Factory", flabel, "enabled again");
		return;
	}
	camTrace("Enabling", flabel);
	fi.enabled = true;
	var obj = getObject(flabel);
	if (!camDef(obj) || !obj)
	{
		camTrace("Factory", flabel, "not found, probably already dead");
		return;
	}
	__camContinueProduction(flabel);
	__camFactoryUpdateTactics(flabel);
}

//;; ## camQueueDroidProduction(player, template)
//;;
//;; Queues up an extra droid template for production. It would be produced
//;; in the first factory that is capable of producing it, at the end of
//;; its production loop, first queued first served.
//;;
function camQueueDroidProduction(player, template)
{
	if (!camDef(__camFactoryQueue[player]))
	{
		__camFactoryQueue[player] = [];
	}
	__camFactoryQueue[player][__camFactoryQueue[player].length] = template;
}

//;; ## camSetPropulsionTypeLimit(number)
//;;
//;; On hard and insane the propulsion type can be limited with this. For type II
//;; pass in 2, and for type III pass in 3. Hard defaults to type II and
//;; insane defaults to type III. If nothing is passed in then the type
//;; limit will match what is in templates.json.
//;;
function camSetPropulsionTypeLimit(num)
{
	if (!camDef(num))
	{
		__camPropulsionTypeLimit = "NO_USE";
	}
	else if (num === 2)
	{
		__camPropulsionTypeLimit = "02";
	}
	else if (num === 3)
	{
		__camPropulsionTypeLimit = "03";
	}
}

//////////// privates

var __camFactoryInfo;
var __camFactoryQueue;
var __camPropulsionTypeLimit;

function __camFactoryUpdateTactics(flabel)
{
	var fi = __camFactoryInfo[flabel];
	if (!fi.enabled)
	{
		camDebug("Factory", flabel, "was not enabled");
		return;
	}
	var droids = enumGroup(fi.group);
	if (droids.length >= fi.groupSize)
	{
		camManageGroup(fi.group, fi.order, fi.data);
		fi.group = camNewGroup();
	}
	else
	{
		var pos = camMakePos(fi.assembly);
		if (!camDef(pos))
		{
			pos = camMakePos(flabel);
		}
		camManageGroup(fi.group, CAM_ORDER_DEFEND, { pos: pos });
	}
}

function __camAddDroidToFactoryGroup(droid, structure)
{
	// don't manage trucks in this function
	if (droid.droidType === DROID_CONSTRUCT)
	{
		return;
	}
	// FIXME: O(n) lookup here
	var flabel = getLabel(structure);
	if (!camDef(flabel) || !flabel)
	{
		return;
	}
	var fi = __camFactoryInfo[flabel];
	groupAdd(fi.group, droid);
	if (camDef(fi.assembly))
	{
		// this is necessary in case droid is regrouped manually
		// in the scenario code, and thus DORDER_DEFEND for assembly
		// will not be applied in __camFactoryUpdateTactics()
		var pos = camMakePos(fi.assembly);
		orderDroidLoc(droid, DORDER_MOVE, pos.x, pos.y);
	}
	__camFactoryUpdateTactics(flabel);
}

function __camChangePropulsionOnDiff(propulsion)
{
	if (difficulty === EASY || difficulty === MEDIUM)
	{
		return propulsion;
	}
	if (camDef(__camPropulsionTypeLimit) && __camPropulsionTypeLimit === "NO_USE")
	{
		return propulsion; //this mission don't want this feature then
	}

	var name = propulsion;
	var typeModifier = difficulty === HARD ? "02" : "03";
	const VALID_PROPS = [
		"CyborgLegs", "HalfTrack", "V-Tol", "hover", "tracked", "wheeled",
	];

	var lastTwo = name.substring(name.length - 2);
	if (lastTwo === "01" || lastTwo === "02" || lastTwo === "03")
	{
		name = name.substring(0, name.length - 2)
	}

	for (var i = 0, l = VALID_PROPS.length; i < l; ++i)
	{
		var currentProp = VALID_PROPS[i];
		if (name === currentProp)
		{
			//if hard difficulty and a future template has a type III then this will
			//ensure it stays type III.
			if (difficulty === HARD && lastTwo === "02")
			{
				typeModifier = "03";
			}
			//maybe a mission wants to set a limit on the highest propulsion type
			if (camDef(__camPropulsionTypeLimit))
			{
				typeModifier = __camPropulsionTypeLimit;
			}
			//return a stronger propulsion based on difficulty
			return currentProp.concat(typeModifier);
		}
	}

	//If all else fails then return the propulsion that came with the template
	return propulsion;
}

function __camBuildDroid(template, structure)
{
	if (!camDef(structure))
	{
		return false;
	}
	//if not a normal factory and the template is a constructor then keep it in the
	//queue until a factory can deal with it.
	if (template.weap === "Spade1Mk1" && structure.stattype !== FACTORY)
	{
		return false;
	}
	var prop = __camChangePropulsionOnDiff(template.prop);
	makeComponentAvailable(template.body, structure.player);
	makeComponentAvailable(prop, structure.player);
	makeComponentAvailable(template.weap, structure.player);
	var n = [ structure.name, structure.id, template.body, prop, template.weap ].join(" ");
	// multi-turret templates are not supported yet
	return buildDroid(structure, n, template.body, prop, "", "", template.weap);
}

//Check if an enabled factory can begin manufacturing something. Doing this
//by timer has the perk of not breaking production if something went wrong in
//cam_eventDroidBuilt (or the mere act of reloading saves).
function __checkEnemyFactoryProductionTick()
{
	for (var flabel in __camFactoryInfo)
	{
		if (getObject(flabel) !== null && __camFactoryInfo[flabel].enabled === true)
		{
			__camContinueProduction(flabel);
		}
	}
}

function __camContinueProduction(structure)
{
	var flabel;
	var struct;
	if (camIsString(structure))
	{
		flabel = structure;
		struct = getObject(flabel);
		if (!camDef(struct) || !struct)
		{
			camTrace("Factory not found");
			return;
		}
	}
	else
	{
		// FIXME: O(n) lookup here
		flabel = getLabel(structure);
		struct = structure;
	}
	if (!camDef(flabel) || !flabel)
	{
		return;
	}
	if (!structureIdle(struct))
	{
		return;
	}
	var fi = __camFactoryInfo[flabel];
	if (camDef(fi.maxSize) && groupSize(fi.group) >= fi.maxSize)
	{
		// retry later
		return;
	}
	if (camDef(fi.throttle) && camDef(fi.lastprod))
	{
		var throttle = gameTime - fi.lastprod;
		if (throttle < fi.throttle)
		{
			// do throttle
			return;
		}
	}
	// check factory queue after every loop
	if (fi.state === -1)
	{
		fi.state = 0;
		var p = struct.player;
		if (camDef(__camFactoryQueue[p]) && __camFactoryQueue[p].length > 0)
		{
			if (__camBuildDroid(__camFactoryQueue[p][0], struct))
			{
				__camFactoryQueue[p].shift();
				return;
			}
		}
	}
	__camBuildDroid(fi.templates[fi.state], struct);
	// loop through templates
	++fi.state;
	if (fi.state >= fi.templates.length)
	{
		fi.state = -1;
	}
	fi.lastprod = gameTime;
}


////////////////////////////////////////////////////////////////////////////////
// Victory celebration helpers.
////////////////////////////////////////////////////////////////////////////////

//;; ## camNextLevel(next level)
//;;
//;; A wrapper around ```loadLevel()```. Remembers to give bonus power
//;; for completing the mission faster.
//;;
function camNextLevel(nextLevel)
{
	if (__camNeedBonusTime)
	{
		var bonusTime = getMissionTime();
		if (difficulty === EASY || difficulty === MEDIUM)
		{
			bonusTime = Math.floor(bonusTime * 0.75);
		}
		if (bonusTime > 0)
		{
			var bonus = 110;
			if (difficulty === HARD)
			{
				bonus = 105;
			}
			else if (difficulty === INSANE)
			{
				bonus = 100;
			}
			camTrace("Bonus time", bonusTime);
			setPowerModifier(bonus); // Bonus percentage for completing fast
			extraPowerTime(bonusTime);
			setPowerModifier(100);
		}
	}
	camBreakAlliances();
	//Set these limits again for the home map before exiting this mission
	setStructureLimits("A0CommandCentre", 1, CAM_HUMAN_PLAYER);
	setStructureLimits("A0ComDroidControl", 1, CAM_HUMAN_PLAYER);

	loadLevel(nextLevel);
}

const CAM_VICTORY_STANDARD = 0;
const CAM_VICTORY_PRE_OFFWORLD = 1;
const CAM_VICTORY_OFFWORLD = 2;
const CAM_VICTORY_TIMEOUT = 3;

//;; ## camSetStandardWinLossConditions(kind, nextLevel, data)
//;;
//;; Set victory and defeat conditions to one of the common
//;; options. On victory, load nextLevel. The extra data parameter
//;; contains extra data required to define some of the victory
//;; conditions. The following options are available:
//;;
//;; * ```CAM_VICTORY_STANDARD``` Defeat if all ground factories
//;; 	and construction droids are lost, or on mission timeout.
//;; 	Victory when all enemies are destroyed and all artifacts
//;; 	are recovered.
//;; * ```CAM_VICTORY_PRE_OFFWORLD``` Defeat on timeout. Victory on
//;; 	transporter launch, then load the sub-level.
//;; * ```CAM_VICTORY_OFFWORLD``` Defeat on timeout or all units lost.
//;; 	Victory when all artifacts are recovered and either all enemies
//;; 	are dead (not just bases) or all droids are at the LZ.
//;; 	Also automatically handles the "LZ compromised" message,
//;; 	which is why it needs to know reinforcement interval to restore.
//;; 	The following data parameter fields are available:
//;;   * ```area``` The landing zone to return to.
//;;   * ```message``` The "Return to LZ" message ID. Optional.
//;;   * ```reinforcements``` Reinforcements interval, in seconds.
//;; For standard and offworld victory, some extra data parameters can be defined:
//;; * ```callback``` A function callback to check for extra win/loss
//;; 		conditions. Return values are interpreted as follows:
//;;   * __false__ means instant defeat ("objective failed"),
//;;   * __true__ means victory as long as other standard victory
//;; 		conditions are met,
//;;   * __undefined__ means suppress
//;; 		other victory checks ("clearly not won yet").
//;; * ```victoryVideo``` Pass in the name of a video string here
//;;			and it will be played before attempting to load the next level.
//;; For offworld victory, some more extra data parameters can be defined:
//;; * ```retlz``` Force the player to return to the LZ area:
//;;   * __false__ mission does not require a LZ return,
//;;   * __true__ mission requires all units to be at LZ to win.
//;; * ```annihilate``` Player must destroy every thing on map to win:
//;;   * __false__ mission does not require everything destroyed,
//;;   * __true__ mission requires total map annihilation.
//;; * ```eliminateBases``` Instant win when all enemy units and bases are destroyed:
//;;   * __false__ does not require all bases to be destroyed,
//;;   * __true__ requires all bases destroyed.
//;;
function camSetStandardWinLossConditions(kind, nextLevel, data)
{
	switch(kind)
	{
		case CAM_VICTORY_STANDARD:
			__camWinLossCallback = "__camVictoryStandard";
			__camNeedBonusTime = true;
			__camDefeatOnTimeout = true;
			__camVictoryData = data;
			useSafetyTransport(false);
			break;
		case CAM_VICTORY_PRE_OFFWORLD:
			__camWinLossCallback = "__camVictoryPreOffworld";
			__camNeedBonusTime = false;
			__camDefeatOnTimeout = true;
			useSafetyTransport(false);
			break;
		case CAM_VICTORY_OFFWORLD:
			__camWinLossCallback = "__camVictoryOffworld";
			__camNeedBonusTime = true;
			__camDefeatOnTimeout = true;
			__camVictoryData = data;
			setReinforcementTime(__camVictoryData.reinforcements);
			queue("__camSetOffworldLimits", 100);
			useSafetyTransport(false);
			break;
		case CAM_VICTORY_TIMEOUT:
			__camWinLossCallback = "__camVictoryTimeout";
			__camNeedBonusTime = false;
			__camDefeatOnTimeout = false;
			__camVictoryData = data;
			setReinforcementTime(__camVictoryData.reinforcements);
			useSafetyTransport(true);
			break;
		default:
			camDebug("Unknown standard victory condition", kind);
			break;
	}
	__camNextLevel = nextLevel;
}

//Checks for extra win conditions defined in level scripts, if any.
function camCheckExtraObjective()
{
	var extraObjMet = true;
	if (camDef(__camVictoryData) && camDef(__camVictoryData.callback))
	{
		var result = __camGlobalContext()[__camVictoryData.callback]();
		if (camDef(result))
		{
			if (!result)
			{
				__camGameLost();
				return;
			}
		}
		else
		{
			extraObjMet = false;
		}
	}

	return extraObjMet;
}

//////////// privates

var __camWinLossCallback;
var __camNextLevel;
var __camNeedBonusTime;
var __camDefeatOnTimeout;
var __camVictoryData;
var __camRTLZTicker;
var __camLZCompromisedTicker;
var __camLastAttackTriggered;
var __camLevelEnded;

function __camSetOffworldLimits()
{
	// These are the only structures that do not get
	// auto-disabled by the engine in off-world missions.
	setStructureLimits("A0CommandCentre", 0, 0);
	setStructureLimits("A0ComDroidControl", 0, 0);
}

function __camGameLostCB()
{
	gameOverMessage(false, false);
}

function __camGameLost()
{
	//HACK: Allows the player to survive in Gamma 3-A. If they did not
	//bring in a construction droid then they lose.
	if(__camNextLevel === "SUB_3_1S" && getMissionTime() > (camChangeOnDiff(7200) - 60))
	{
		return; // Campaign 3A needs to get units setup first.
	}
	camCallOnce("__camGameLostCB");
}

function __camGameWon()
{
	__camLevelEnded = true;
	if (camDef(__camVictoryData) && camDef(__camVictoryData.victoryVideo))
	{
		camPlayVideos(__camVictoryData.victoryVideo);
	}

	if (camDef(__camNextLevel))
	{
		camTrace(__camNextLevel);
		if (__camNextLevel === "GAMMA_OUT")
		{
			gameOverMessage(true, false, true);
			return;
		}
		camNextLevel(__camNextLevel);
	}
	else
	{
		//If nothing to load, go to main menu.
		gameOverMessage(true, false);
	}
}

//NOTE: Do not count cyborg factory here since combat engineer does not exist
//in campaign at the moment.
function __camPlayerDead()
{
	if (countStruct("A0LightFactory") > 0)
	{
		return false;
	}
	return (countDroid(DROID_CONSTRUCT) === 0);
}

function __camTriggerLastAttack()
{
	if (!__camLastAttackTriggered)
	{
		var enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false);
		// Do not order systems (sensor/trucks/repairs) to attack stuff.
		enemies = enemies.filter(function(obj) {
			return ((obj.type === DROID)
				&& !camIsTransporter(obj)
				&& !camIsSystemDroid(obj)
			);
		});
		camTrace(enemies.length, "enemy droids remaining");
		camManageGroup(camMakeGroup(enemies), CAM_ORDER_ATTACK);
		__camLastAttackTriggered = true;
	}
}

function __camVictoryStandard()
{
	var extraObj = camCheckExtraObjective();
	// check if game is lost
	if (__camPlayerDead())
	{
		__camGameLost();
		return;
	}
	// check if game is won
	if (camAllArtifactsPickedUp() && camAllEnemyBasesEliminated() && extraObj)
	{
		if (enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).length === 0)
		{
			__camGameWon();
			return;
		}
		else
		{
			__camTriggerLastAttack();
		}
	}
}

function __camVictoryPreOffworld()
{
	if (__camPlayerDead())
	{
		__camGameLost();
		return;
	}
	// victory hooked from eventTransporterExit
}

function __camVictoryTimeout()
{
	if (__camPlayerDead())
	{
		__camGameLost();
		return;
	}
	// victory hooked from eventMissionTimeout
}

function __camVictoryOffworld()
{
	var lz = __camVictoryData.area;
	if (!camDef(lz))
	{
		camDebug("Landing zone area is required for OFFWORLD");
		return;
	}
	var total = countDroid(DROID_ANY, CAM_HUMAN_PLAYER); // for future use
	if (total === 0)
	{
		__camGameLost();
		return;
	}
	var forceLZ = camDef(__camVictoryData.retlz) ? __camVictoryData.retlz : false;
	var destroyAll = camDef(__camVictoryData.annihilate) ? __camVictoryData.annihilate : false;
	var elimBases = camDef(__camVictoryData.eliminateBases) ? __camVictoryData.eliminateBases : false;

	if (camCheckExtraObjective() && camAllArtifactsPickedUp())
	{
		if (elimBases)
		{
			var enemyDroids = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).filter(function(obj) {
				return obj.type === DROID;
			}).length;
			if (!enemyDroids && camAllEnemyBasesEliminated())
			{
				__camGameWon();
			}
		}
		else
		{
			var enemyLen = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).length;
			if (!forceLZ && !enemyLen)
			{
				//if there are no more enemies, win instantly unless forced to go
				//back to the LZ.
				__camGameWon();
				return;
			}

			//Missions that are not won based on artifact count (see missions 2-1 and 3-2).
			//If either forceLZ or destroyAll is true then ignore this.
			var arti = camGetArtifacts();
			if (arti.length === 0 && !forceLZ && !destroyAll)
			{
				__camGameWon();
				return;
			}

			//Make sure to only count droids here.
			var atlz = enumArea(lz, CAM_HUMAN_PLAYER, false).filter(function(obj) {
				return (obj.type === DROID && !camIsTransporter(obj));
			}).length;
			if (((!forceLZ && !destroyAll)
				|| (forceLZ && destroyAll && !enemyLen)
				|| (forceLZ && !destroyAll))
				&& (atlz === total))
			{
				__camGameWon();
				return;
			}
			else
			{
				__camTriggerLastAttack();
			}

			if (!destroyAll || (forceLZ && destroyAll && !enemyLen))
			{
				if (__camRTLZTicker === 0 && camDef(__camVictoryData.message))
				{
					camTrace("Return to LZ message displayed");
					camMarkTiles(lz);
					if (camDef(__camVictoryData.message))
						hackAddMessage(__camVictoryData.message, PROX_MSG,
						               	CAM_HUMAN_PLAYER, false);
				}
				if (__camRTLZTicker % 30 === 0) // every 30 seconds
				{
					var pos = camMakePos(lz);
					playSound("pcv427.ogg", pos.x, pos.y, 0);
					console(_("Return to LZ"));
				}
				++__camRTLZTicker;
			}
		}
	}
	if (enumArea(lz, ENEMIES, false).length > 0)
	{
		if (__camLZCompromisedTicker === 0)
		{
			camTrace("LZ compromised");
		}
		if (__camLZCompromisedTicker % 30 === 1) // every 30 seconds
		{
			var pos = camMakePos(lz);
			playSound("pcv445.ogg", pos.x, pos.y, 0);
			setReinforcementTime(LZ_COMPROMISED_TIME);
		}
		++__camLZCompromisedTicker;
		if (__camRTLZTicker === 0)
		{
			camMarkTiles(lz);
		}
	}
	else if (__camLZCompromisedTicker > 0)
	{
		camTrace("LZ clear");
		var pos = camMakePos(lz);
		playSound("lz-clear.ogg", pos.x, pos.y, 0);
		setReinforcementTime(__camVictoryData.reinforcements);
		__camLZCompromisedTicker = 0;
		if (__camRTLZTicker === 0)
		{
			camUnmarkTiles(lz);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Transporter management.
////////////////////////////////////////////////////////////////////////////////

//;; ## camIsTransporter(game object)
//;;
//;; Determine if the object is a transporter.
//;;
function camIsTransporter(object)
{
	if (!camDef(object) || !object)
	{
		return false;
	}

	if (object.type !== DROID)
	{
		camTrace("Attempted to check if a non-droid object is a transporter.");
		return false;
	}

	return ((object.droidType === DROID_TRANSPORTER)
		|| (object.droidType === DROID_SUPERTRANSPORTER)
	);
}

//;; ## camSetupTransport(place x, place y, exit x, exit y)
//;;
//;; A convenient function for placing the standard campaign transport
//;; for loading in pre-away missions. The exit point for the transport
//;; is set up as well.
//;;
function camSetupTransporter(x, y, x1, y1)
{
	addDroid(CAM_HUMAN_PLAYER, x, y, "Transport", "TransporterBody", "V-Tol", "", "", "MG3-VTOL");
	setTransporterExit(x1, y1, CAM_HUMAN_PLAYER);
}

////////////////////////////////////////////////////////////////////////////////
// VTOL management.
// These functions create the hit and run vtols for the given player.
// Vtol rearming is handled in group management.
////////////////////////////////////////////////////////////////////////////////

//Setup hit and runner VTOLs. Passing in obj is optional and will automatically
//disable the vtol spawner once that object is dead.
function camSetVtolData(player, startPos, exitPos, templates, timer, obj, extras)
{
	__camVtolPlayer = player;
	__camVtolStartPosition = startPos;
	__camVtolExitPosition = camMakePos(exitPos);
	__camVtolTemplates = templates;
	__camVtolTimer = timer;
	__camVtolSpawnStopObject = obj;
	__camVtolExtras = extras;

	camSetVtolSpawn(true);
	__checkVtolSpawnObject();
	__camSpawnVtols();
	__camRetreatVtols();
}

function camSetVtolSpawn(value)
{
	__camVtolSpawnActive = value;
}

// privates
var __camVtolPlayer;
var __camVtolStartPosition;
var __camVtolTemplates;
var __camVtolExitPosition;
var __camVtolTimer;
var __camVtolSpawnActive;
var __camVtolSpawnStopObject;
var __camVtolExtras;

function __checkVtolSpawnObject()
{
	if (camDef(__camVtolSpawnStopObject))
	{
		if (getObject(__camVtolSpawnStopObject) === null)
		{
			camSetVtolSpawn(false); //Deactive hit and runner VTOLs.
		}
		else
		{
			queue("__checkVtolSpawnObject", 5000);
		}
	}
}

function __camSpawnVtols()
{
	if (__camVtolSpawnActive === false)
	{
		return;
	}

	var amount = 5 + camRand(2);
	var droids = [];
	var pos;

	//Make sure to catch multiple start positions also.
	if(__camVtolStartPosition instanceof Array)
	{
		pos = __camVtolStartPosition[camRand(__camVtolStartPosition.length)];
	}
	else
	{
		pos = __camVtolStartPosition;
	}

	if (!camDef(__camVtolExtras))
	{
		//Pick some droids randomly.
		for (var i = 0; i < amount; ++i)
		{
			droids.push(__camVtolTemplates[camRand(__camVtolTemplates.length)]);
		}
	}
	else
	{
		var lim = amount;
		var alternate = false;
		if (camDef(__camVtolExtras.alternate))
		{
			alternate = __camVtolExtras.alternate; //Only use one template type
		}
		if (!camDef(__camVtolExtras.altIdx))
		{
			__camVtolExtras.altIdx = 0;
		}
		if (camDef(__camVtolExtras.limit))
		{
			//support an array of limits for each template
			if (__camVtolExtras.limit instanceof Array)
			{
				lim = __camVtolExtras.limit[__camVtolExtras.altIdx]; //max templates to use
			}
			else
			{
				lim = __camVtolExtras.limit;
			}
		}

		for (var i = 0; i < lim; ++i)
		{
			if (!alternate)
			{
				droids.push(__camVtolTemplates[camRand(__camVtolTemplates.length)]);
			}
			else
			{
				droids.push(__camVtolTemplates[__camVtolExtras.altIdx]);
			}
		}

		if (__camVtolExtras.altIdx < __camVtolTemplates.length)
		{
			++__camVtolExtras.altIdx;
		}
		else
		{
			__camVtolExtras.altIdx = 0;
		}
	}

	//...And send them.
	camSendReinforcement(__camVtolPlayer, camMakePos(pos), droids, CAM_REINFORCE_GROUND, {
		order: CAM_ORDER_ATTACK,
		data: { regroup: false, count: -1 }
	});

	queue("__camSpawnVtols", __camVtolTimer);
}

function __camRetreatVtols()
{
	if(countStruct("A0VtolPad", __camVtolPlayer))
	{
		return; //Got to destroy those rearming pads now.
	}

	var vtols = enumDroid(__camVtolPlayer).filter(function(obj) {
		return isVTOL(obj);
	});

	for (var i = 0, l = vtols.length; i < l; ++i)
	{
		var vt = vtols[i];
		if (!camDef(vt))
		{
			continue;
		}
		for (var c = 0, d = vt.weapons.length; c < d; ++c)
		{
			if ((vt.order === DORDER_RTB) || (vt.weapons[c].armed < 1) || (vt.health < 40))
			{
				orderDroidLoc(vt, DORDER_MOVE, __camVtolExitPosition.x, __camVtolExitPosition.y);
				break;
			}
		}
	}

	queue("__camRetreatVtols", 800);
}

////////////////////////////////////////////////////////////////////////////////
// Nexus related functionality.
////////////////////////////////////////////////////////////////////////////////

const DEFENSE_ABSORBED = "defabsrd.ogg";
const DEFENSE_NEUTRALIZE = "defnut.ogg";
const LAUGH1 = "laugh1.ogg";
const LAUGH2 = "laugh2.ogg";
const LAUGH3 = "laugh3.ogg";
const PRODUCTION_COMPLETE = "pordcomp.ogg";
const RES_ABSORBED = "resabsrd.ogg";
const STRUCTURE_ABSORBED = "strutabs.ogg";
const STRUCTURE_NEUTRALIZE = "strutnut.ogg";
const SYNAPTICS_ACTIVATED = "synplnk.ogg";
const UNIT_ABSORBED = "untabsrd.ogg";
const UNIT_NEUTRALIZE = "untnut.ogg";

//Play a random laugh.
function camNexusLaugh()
{
	if (camRand(101) < 45)
	{
		var rnd = camRand(3) + 1;
		var snd;

		if (rnd === 0)
		{
			snd = LAUGH1;
		}
		else if (rnd === 1)
		{
			snd = LAUGH2;
		}
		else
		{
			snd = LAUGH3;
		}

		playSound(snd);
	}
}

function camAbsorbPlayer(who, to)
{
     if (!camDef(who))
     {
          who = CAM_HUMAN_PLAYER;
     }
     if (!camDef(to))
     {
          to = NEXUS;
     }

     var units = enumDroid(who);
	for (var i = 0, l = units.length; i < l; i++)
	{
		if (!donateObject(units[i], to))
		{
			camSafeRemoveObject(units[i], false);
		}
	}

    var structs = enumStruct(who);
    for (var i = 0, l = structs.length; i < l; i++)
    {
         if (!donateObject(structs[i], to))
	    {
		    camSafeRemoveObject(structs[i], false);
	    }
    }

    camTrace("Player " + who + " has been absorbed by player" + to);
    changePlayerColour(who, to);
}

//Steal a droid or structure from a player.
function camHackIntoPlayer(player, to)
{
	if (__camNexusActivated === false)
	{
		return;
	}

     if (!camDef(player))
     {
          player = CAM_HUMAN_PLAYER;
     }
     if (!camDef(to))
     {
          to = NEXUS;
     }
     if (!camDef(__camLastNexusAttack))
     {
          lastNexusHit = 0;
     }

	//HACK: Seems HQ can not be donated yet so destroy it for now.
     var tmp = camRand(2) ? enumDroid(player) : enumStruct(player).filter(function(s) { return (s.status === BUILT); });
	if (!__camLastNexusAttack || (gameTime > (__camLastNexusAttack + 5000)) && (camRand(101) <= 25))
     {
		var obj = !__camLastNexusAttack ? enumStruct(player, HQ)[0] : tmp[camRand(tmp.length)];
		__camLastNexusAttack = gameTime;
		if (!camDef(obj))
		{
			return;
		}
		camTrace("stealing object: " + obj.name);
		if (obj.type === STRUCTURE && obj.stattype === HQ)
		{
			if (!donateObject(obj, to))
			{
				camSafeRemoveObject(obj, false);
			}
			if (player === CAM_HUMAN_PLAYER)
			{
				setMiniMap(false);
				setDesign(false);
			}
		}
		else
		{
			if (!donateObject(obj, to))
			{
				camSafeRemoveObject(obj, true); //Explode it then.
			}
		}
     }
}

function camSetNexusState(flag)
{
	__camNexusActivated = flag;
}

function camGetNexusState()
{
	return __camNexusActivated;
}

// privates
var __camLastNexusAttack;
var __camNexusActivated;

////////////////////////////////////////////////////////////////////////////////
// Group functionality
////////////////////////////////////////////////////////////////////////////////

//;; ## camNewGroup()
//;;
//;; A saveload safe version of newGroup() so as not to create group ID clashes.
//;;
function camNewGroup()
{
	if (!camDef(__camNewGroupCounter))
	{
		__camNewGroupCounter = 0;
	}
	++__camNewGroupCounter;
	return __camNewGroupCounter;
}

//;; ## camInNeverGroup(droid)
//;;
//;; check if this droid is forced to never group.
//;;
function camInNeverGroup(droid)
{
	if (droid.type !== DROID)
	{
		camDebug("Non-droid in camInNeverGroup.");
		return false;
	}
	for (var i = 0, l = __camNeverGroupDroids.length; i < l; ++i)
	{
		if (droid.id === __camNeverGroupDroids[i])
		{
			return true;
		}
	}

	return false;
}

//;; ## camNeverGroupDroid(what, [filter])
//;;
//;; A means to not auto group some droids.
//;;
function camNeverGroupDroid(what, filter)
{
	if (!camDef(filter))
	{
		filter = ENEMIES;
	}
	var array;
	var obj;
	if (camIsString(what)) // label
	{
		obj = getObject(what);
	}
	else if (camDef(what.length)) // array
	{
		array = what;
	}
	else if (camDef(what.type)) // object
	{
		obj = what;
	}
	if (camDef(obj))
	{
		switch (obj.type) {
			case POSITION:
				obj = getObject(obj.x, obj.y);
				// fall-through
			case DROID:
			case STRUCTURE:
			case FEATURE:
				array = [ obj ];
				break;
			case AREA:
				array = enumArea(obj.x, obj.y, obj.x2, obj.y2, ALL_PLAYERS, false);
				break;
			case RADIUS:
				array = enumRange(obj.x, obj.y, obj.radius, ALL_PLAYERS, false);
				break;
			case GROUP:
				array = enumGroup(obj.id);
				break;
			default:
				camDebug("Unknown object type", obj.type);
		}
	}
	if (camDef(array))
	{
		for (var i = 0, l = array.length; i < l; ++i)
		{
			var o = array[i];
			if (!camDef(o) || !o)
			{
				continue;
			}
			if (o.type === DROID && camPlayerMatchesFilter(o.player, filter))
			{
				__camNeverGroupDroids.push(o.id);
			}
		}
		return;
	}
	camDebug("Cannot parse", what);
}

// privates
var __camNewGroupCounter;
var __camNeverGroupDroids;

////////////////////////////////////////////////////////////////////////////////
// Video playback related logic
////////////////////////////////////////////////////////////////////////////////

// Be aware that eventVideoDone will be triggered for each video
// and will be received by the mission script's eventVideoDone, should it exist.

//;; ## camPlayVideos(videos)
//;;
//;; If videos is an array, queue up all of them for immediate playing. This
//;; function will play one video sequence should one be provided.
//;;
function camPlayVideos(videos)
{
	if (videos instanceof Array)
	{
		__camVideoSequences = videos;
		__camEnqueueVideos();
	}
	else
	{
		if (!camIsString(videos))
		{
			camDebug("Non string passed in to play a video. Skipping.");
			return;
		}
		hackAddMessage(videos, MISS_MSG, CAM_HUMAN_PLAYER, true);
	}
}

// privates
var __camVideoSequences;

// Play all stored videos one after the other.
// cam_eventVideoDone will call this function repeatedly.
function __camEnqueueVideos()
{
	if (!__camVideoSequences.length)
	{
		return; //Nothing to play
	}

	var what = __camVideoSequences[0];
	if (!camIsString(what))
	{
		camDebug("Non string found in __camVideoSequences. Stopping playback.");
		return;
	}

	hackAddMessage(what, MISS_MSG, CAM_HUMAN_PLAYER, true);
	__camVideoSequences.shift();
}

////////////////////////////////////////////////////////////////////////////////
// Event hooks magic. This makes all the library catch all the necessary events
// without distracting the script authors from handling them on their own.
// It assumes that script authors do not implement another similar mechanism,
// or something BAD would happen.
////////////////////////////////////////////////////////////////////////////////

//;; ## camAreaEvent(label, function(droid))
//;;
//;; Implement eventArea<label> in a debugging-friendly way. The function
//;; marks the area until the event is triggered, and traces entering the area
//;; in the TRACE log.
//;;
function camAreaEvent(label, code)
{
	var eventName = "eventArea" + label;
	camMarkTiles(label);
	__camPreHookEvent(eventName, function(droid)
	{
		if (camDef(droid))
		{
			camTrace("Player", droid.player, "enters", label);
		}
		camUnmarkTiles(label);
		code(droid);
	});
}

//////////// privates

var __camOriginalEvents = {};

// A hack to make sure we do not put this variable into the savegame. It is
// called from top level, because we need to call it again every time we load
// scripts. But other than this one, you should in general never call game
// functions from toplevel, since all game state may not be fully initialized
// yet at the time scripts are loaded. (Yes, function name needs to be quoted.)
hackDoNotSave("__camOriginalEvents");


function __camPreHookEvent(eventname, hookcode)
{
	// store the original event handler
	if (camDef(__camGlobalContext()[eventname]))
	{
		__camOriginalEvents[eventname] = __camGlobalContext()[eventname];
	}

	__camGlobalContext()[eventname] = function()
	{
		// Don't trigger hooks after level end.
		// No, this should't say "if (__camLevelEnded) return;".
		// Because we are not hooking all possible events,
		// and we are not documenting which exactly events are hooked.
		// We should not intervene with level events in unobvious ways.
		if (!__camLevelEnded)
		{
			hookcode.apply(__camGlobalContext(), arguments);
		}
		// otherwise, call the original event if any
		if (camDef(__camOriginalEvents[eventname]))
		{
			__camOriginalEvents[eventname].apply(__camGlobalContext(), arguments);
		}
	};
}

/* Called every second after eventStartLevel(). */
function __camTick()
{
	if (camDef(__camWinLossCallback))
	{
		__camGlobalContext()[__camWinLossCallback]();
	}
	__camBasesTick();
}

var __camLastHitTime = 0;
const __CAM_EVENT_ATTACKED_INTENSITY = 5000;

function cam_eventPickup(feature, droid)
{
	if (feature.stattype === ARTIFACT)
	{
		__camPickupArtifact(feature);
	}
}

var __camSaveLoading;
function cam_eventGroupLoss(obj, group, newsize)
{
	if (__camSaveLoading === true)
	{
		return;
	}
	if (newsize === 0)
	{
		__camCheckBaseEliminated(group);
	}
	if (camDef(__camGroupInfo[group]))
	{
		profile("__camCheckGroupMorale", group);
	}
}

function cam_eventCheatMode(entered)
{
	if (entered)
	{
		__camCheatMode = true;
		camTrace("Cheats enabled!");
	}
	else
	{
		camTrace("Cheats disabled!");
		__camCheatMode = false;
	}
	__camUpdateMarkedTiles();
}

function cam_eventChat(from, to, message)
{
	if (!__camCheatMode)
	{
		return;
	}
	camTrace(from, to, message);
	if (message === "let me win" && __camNextLevel !== "SUB_1_1")
	{
		__camLetMeWin();
	}
	if (message === "win info")
	{
		__camWinInfo();
	}
	if (message === "make cc")
	{
		setMiniMap(true);
		setDesign(true);
	}
	if (message.lastIndexOf("ascend ", 0) === 0)
	{
		__camNextLevel = message.substring(7).toUpperCase().replace(/-/g, "_");
		__camLetMeWin();
	}
	if (message === "deity")
	{
		for (var blabel in __camEnemyBases)
		{
			camDetectEnemyBase(blabel);
		}
	}
}

function cam_eventStartLevel()
{
	isReceivingAllEvents = true;
	// Variables initialized here are the ones that should not be
	// re-initialized on save-load. Otherwise, they are initialized
	// on the global scope (or wherever necessary).
	__camGroupInfo = {};
	__camFactoryInfo = {};
	__camFactoryQueue = {};
	__camTruckInfo = {};
	__camNeedBonusTime = false;
	__camDefeatOnTimeout = false;
	__camRTLZTicker = 0;
	__camLZCompromisedTicker = 0;
	__camLastAttackTriggered = false;
	__camLevelEnded = false;
	__camPlayerTransports = {};
	__camIncomingTransports = {};
	__camTransporterQueue = [];
	__camNumArtifacts = 0;
	__camArtifacts = {};
	__camNumEnemyBases = 0;
	__camEnemyBases = {};
	__camVtolPlayer = 0;
	__camVtolStartPosition = {};
	__camVtolTemplates = {};
	__camVtolExitPosition = {};
	__camVtolTimer = 0;
	__camVtolSpawnActive = false;
	__camVtolExtras = {};
	__camLastNexusAttack = 0;
	__camNexusActivated = false;
	__camNewGroupCounter = 0;
	__camVideoSequences = [];
	__camSaveLoading = false;
	__camNeverGroupDroids = [];
	camSetPropulsionTypeLimit(); //disable the propulsion changer by default
	__camAiPowerReset(); //grant power to the AI
	setTimer("__checkEnemyFactoryProductionTick", 800);
	setTimer("__camTick", 1000); // campaign pollers
	setTimer("__camTruckTick", 40100); // some slower campaign pollers
	setTimer("__camAiPowerReset", 180000); //reset AI power every so often
	queue("__camTacticsTick", 100); // would re-queue itself
	queue("__camGrantSpecialResearch", 6000);
}

function cam_eventDroidBuilt(droid, structure)
{
	if (!camDef(structure)) // "clone wars" cheat
	{
		return;
	}
	if (!camPlayerMatchesFilter(structure.player, ENEMIES))
	{
		return;
	}
	if (!camPlayerMatchesFilter(droid.player, ENEMIES))
	{
		return;
	}
	if (!camDef(__camFactoryInfo))
	{
		return;
	}
	__camAddDroidToFactoryGroup(droid, structure);
}

function cam_eventDestroyed(obj)
{
	__camCheckPlaceArtifact(obj);
	if (obj.type === DROID && obj.droidType === DROID_CONSTRUCT)
	{
		__camCheckDeadTruck(obj);
	}

	//Nexus neutralize sounds.
	if (__camNexusActivated === true && camRand(101) < 75 && obj.player === CAM_HUMAN_PLAYER)
	{
		var snd;
		if (obj.type === STRUCTURE)
		{
			if (obj.stattype === DEFENSE)
			{
				snd = DEFENSE_NEUTRALIZE;
			}
			else
			{
				snd = STRUCTURE_NEUTRALIZE;
			}
		}
		else if (obj.type === DROID)
		{
			snd = UNIT_NEUTRALIZE;
		}

		if (camDef(snd))
		{
			playSound(snd);
		}
		queue("camNexusLaugh", 1500);
	}
}

function cam_eventObjectSeen(viewer, seen)
{
	__camCheckBaseSeen(seen);
}

function cam_eventGroupSeen(viewer, group)
{
	__camCheckBaseSeen(group);
}

function cam_eventTransporterExit(transport)
{
	camTrace("Transporter for player", transport.player + " has exited");

	if (transport.player !== CAM_HUMAN_PLAYER
		|| (__camWinLossCallback === "__camVictoryStandard"
		&& transport.player === CAM_HUMAN_PLAYER))
	{
		// allow the next transport to enter
		if (camDef(__camIncomingTransports[transport.player]))
		{
			delete __camIncomingTransports[transport.player];
		}
	}
	else if (__camWinLossCallback === "__camVictoryPreOffworld")
	{
		camTrace("Transporter is away.");
		__camGameWon();
		return;
	}
}

function cam_eventTransporterLanded(transport)
{
	if (transport.player !== CAM_HUMAN_PLAYER)
	{
		__camLandTransporter(transport.player, camMakePos(transport));
	}
}

function cam_eventMissionTimeout()
{
	if (__camDefeatOnTimeout)
	{
		camTrace("0 minutes remaining.");
		__camGameLost();
	}
	else
	{
		var won = camCheckExtraObjective();
		if (!won)
		{
			__camGameLost();
			return;
		}
		__camGameWon();
	}
}

function cam_eventAttacked(victim, attacker)
{
	if (camDef(victim) && victim && victim.type === DROID)
	{
		if (victim.player !== CAM_HUMAN_PLAYER && !allianceExistsBetween(CAM_HUMAN_PLAYER, victim.player))
		{
			//Try dynamically creating a group of nearby droids not part
			//of a group. Only supports those who can hit ground units.
			if (victim.group === null)
			{
				const DEFAULT_RADIUS = 6;
				var loc = {x: victim.x, y: victim.y};
				var droids = enumRange(loc.x, loc.y, DEFAULT_RADIUS, victim.player, false).filter(function(obj) {
					return (obj.type === DROID
						&& obj.group === null
						&& (obj.canHitGround || obj.isSensor)
						&& obj.droidType !== DROID_CONSTRUCT
						&& !camIsTransporter(obj)
						&& !camInNeverGroup(obj)
					);
				});
				if (droids.length === 0)
				{
					return;
				}
				camManageGroup(camMakeGroup(droids), CAM_ORDER_ATTACK, {
					count: -1,
					regroup: false,
					repair: 70
				});
			}

			if (camDef(__camGroupInfo[victim.group]))
			{
				__camGroupInfo[victim.group].lastHit = gameTime;
			}
		}
	}
}

//Work around some things that break on save-load.
function cam_eventGameLoaded()
{
	isReceivingAllEvents = true;
	__camSaveLoading = true;
	const SCAV_KEVLAR_MISSIONS = [
		"CAM_1CA", "SUB_1_4AS", "SUB_1_4A", "SUB_1_5S", "SUB_1_5",
		"CAM_1A-C", "SUB_1_7S", "SUB_1_7", "SUB_1_DS", "CAM_1END", "SUB_2_5S"
	];

	//Need to set the scavenger kevlar vests when loading a save from later Alpha
	//missions or else it reverts to the original texture.
	for (var i = 0, l = SCAV_KEVLAR_MISSIONS.length; i < l; ++i)
	{
		if (__camNextLevel === SCAV_KEVLAR_MISSIONS[i])
		{
			if (tilesetType === "ARIZONA")
			{
				replaceTexture("page-7-barbarians-arizona.png",
							"page-7-barbarians-kevlar.png");
			}
			else if (tilesetType === "URBAN")
			{
				replaceTexture("page-7-barbarians-arizona.png",
							"page-7-barbarians-urban.png");
			}
			break;
		}
	}

	//Subscribe to eventGroupSeen again.
	camSetEnemyBases();
	__camSaveLoading = false;
}

//Plays Nexus sounds if nexusActivated is true.
function cam_eventObjectTransfer(obj, from)
{
	if (from === CAM_HUMAN_PLAYER && __camNexusActivated === true)
	{
		var loc = {x: obj.x, y: obj.y, z: obj.z};
		var snd;
		if (obj.type === STRUCTURE)
		{
			if (obj.stattype === DEFENSE)
			{
				snd = DEFENSE_ABSORBED;
			}
			else if (obj.stattype === RESEARCH_LAB)
			{
				snd = RES_ABSORBED;
			}
			else
			{
				snd = STRUCTURE_ABSORBED;
			}

			if (obj.stattype === FACTORY
				|| obj.stattype === CYBORG_FACTORY
				|| obj.stattype === VTOL_FACTORY)
			{
				//TODO: add to the factory list.
			}
		}
		else if (obj.type === DROID)
		{
			snd = UNIT_ABSORBED;
		}

		if (camDef(snd))
		{
			playSound(snd);
		}
		queue("camNexusLaugh", 1500);
	}
}

function cam_eventVideoDone()
{
	__camEnqueueVideos(); //Play any remaining videos automatically.
}
