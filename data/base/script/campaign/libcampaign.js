
//;; \section{libcampaign.js documentation}
//;;
//;; \texttt{libcampaign.js} is a JavaScript library supplied with the game,
//;; which contains reusable code for campaign scenarios. It is designed to
//;; make scenario development as high-level and declarative as possible.
//;; It also contains a few simple convenient wrappers.
//;; Public API functions of \texttt{libcampaign.js} are prefixed with
//;; `\texttt{cam}'. To use \texttt{libcampaign.js}, add the following include
//;; into your scenario code:
//;;
//;; \begin{lstlisting}
//;; include("script/campaign/libcampaign.js");
//;; \end{lstlisting}
//;;
//;; Also, most of the \texttt{libcampaign.js} features require some of the
//;; game events handled by the library. Transparent JavaScript pre-hooks are
//;; therefore injected into your global event handlers upon include. For
//;; example, if \texttt{camSetArtifacts()} was called to let
//;; \texttt{libcampaign.js} manage scenario artifacts, then
//;; \texttt{eventPickup()} will be first handled by the library, and only then
//;; your handler will be called, if any.
//;; All of this happens automagically and does not normally require
//;; your attention.

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

	Please leave camDebug() around iff something that should never happen
	occurs, indicating an actual bug in campaign. Then a sensible message
	should be helpful as well. But normally, no warnings should ever be
	printed.

	In cheat mode, more warnings make sense, explaining what's going on
	under the hood. Use camTrace() for such warnings - they'd auto-hide
	outside cheat mode.

	Lines prefixed with // followed by ;; are docstrings for JavaScript API
	documentation. Visit
		http://buildbot.wz2100.net/files/javascript/javascript.html
	to see how it looks. The documentation is coded in TeX; but please
	use only very simple TeX, because it would then be compiled to
	HTML by HeVeA, which fails to support most of the complicated stuff.
*/


////////////////////////////////////////////////////////////////////////////////
// Misc useful stuff.
////////////////////////////////////////////////////////////////////////////////

const CAM_HUMAN_PLAYER = 0;
const CAM_MAX_PLAYERS = 8;
const CAM_TICKS_PER_FRAME = 100;

//;; \subsection{camDef(something)}
//;; Returns false if something is JavaScript-undefined, true otherwise.
function camDef(something)
{
	return typeof(something) !== "undefined";
}

//;; \subsection{camIsString(something)}
//;; Returns true if something is a string, false otherwise.
function camIsString(something)
{
	return typeof(something) === "string";
}

//;; \subsection{camRand(max)}
//;; A non-synchronous random integer in range [0, max - 1].
function camRand(max)
{
	if (max > 0)
		return Math.floor(Math.random() * max);
	camDebug("Max should be positive");
}

//;; \subsection{camCallOnce(function name)}
//;; Call a function by name, but only if it has not been called yet.
function camCallOnce(callback)
{
	if (camDef(__camCalledOnce[callback]) && __camCalledOnce[callback])
		return;
	__camCalledOnce[callback] = true;
	__camGlobalContext()[callback]();
}

//;; \subsection{camSafeRemoveObject(obj[, special effects?])}
//;; Remove a game object (by value or label) if it exists, do nothing otherwise.
function camSafeRemoveObject(obj, flashy)
{
	if (__camLevelEnded)
		return;
	if (camIsString(obj))
		obj = getObject(obj);
	if (camDef(obj) && obj)
		removeObject(obj, flashy);
}

//;; \subsection{camMakePos(x, y | label | object)}
//;; Make a POSITION-like object, unless already done. Often useful
//;; for making functions that would accept positions in both xx,yy and {x:xx,y:yy} forms.
//;; Also accepts labels. If label of AREA is given, returns the center of the area.
//;; If an existing object or label of such is given, returns a safe JavaScript
//;; object containing its x, y and id.
function camMakePos(xx, yy)
{
	if (camDef(yy))
		return { x : xx, y : yy };
	if (!camDef(xx))
		return undefined;
	var obj = xx;
	if (camIsString(xx))
		obj = getObject(xx);
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
				return { x: obj.x, y: obj.y };
			camDebug("Not implemented:", obj.type);
			return undefined;
	}
}

//;; \subsection{camDist(x1, y1, x2, y2 | pos1, x2, y2 | x1, y1, pos2 | pos1, pos2)}
//;; A wrapper for \texttt{distBetweenTwoPoints()}.
function camDist(x1, y1, x2, y2)
{
	if (camDef(y2)) // standard
		return distBetweenTwoPoints(x1, y1, x2, y2);
	var pos2 = camMakePos(x2, y2);
	if (!camDef(pos2)) // pos1, pos2
		return distBetweenTwoPoints(x1.x, x1.y, y1.x, y1.y);
	if (camDef(pos2.x)) // x2 is pos2
		return distBetweenTwoPoints(x1, y1, pos2.x, pos2.y);
	else // pos1, x2, y2
		return distBetweenTwoPoints(x1.x, x1.y, y1, x2);
}

//;; \subsection{camPlayerMatchesFilter(player, filter)}
//;; A function to handle player filters in a way similar to
//;; how JS API functions (eg. \texttt{enumDroid(filter, ...)}) handle them.
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

//;; \subsection{camMakeGroup(what, filter)}
//;; Make a new group out of array of droids, single game object,
//;; or label string, with fuzzy auto-detection of argument type.
//;; Only droids would be added to the group. \textbf{filter} can be one of
//;; a player index, ALL_PLAYERS, ALLIES or ENEMIES; defaults to ENEMIES.
function camMakeGroup(what, filter)
{
	if (!camDef(filter))
		filter = ENEMIES;
	var array;
	var obj;
	if (camIsString(what)) // label
		obj = getObject(what);
	else if (camDef(what.length)) // array
		array = what;
	else if (camDef(what.type)) // object
		obj = what;
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
		var group = newGroup();
		for (var i = 0; i < array.length; ++i)
		{
			var o = array[i];
			if (!camDef(o) || !o)
			{
				camDebug("Trying to add", o);
				continue;
			}
			if (o.type === DROID && camPlayerMatchesFilter(o.player, filter))
				groupAdd(group, o);
		}
		return group;
	}
	camDebug("Cannot parse", what);
}

//////////// privates

var __camCalledOnce = {};

function __camGlobalContext()
{
	return Function('return this')();
}

////////////////////////////////////////////////////////////////////////////////
// Debugging helpers.
////////////////////////////////////////////////////////////////////////////////

//;; \subsection{camMarkTiles(label | array of labels)}
//;; Mark area on the map by label(s), but only if debug mode is enabled.
//;; Otherwise, remember what to mark in case it is going to be.
function camMarkTiles(label)
{
	if (camIsString(label))
		__camMarkedTiles[label] = true;
	else
		for (var i = 0; i < label.length; ++i)
			__camMarkedTiles[label[i]] = true;
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; \subsection{camUnmarkTiles(label | array of labels)}
//;; No longer mark area(s) with given label(s) in debug mode.
function camUnmarkTiles(label)
{
	if (camIsString(label))
		delete __camMarkedTiles[label];
	else
		for (var i = 0; i < label.length; ++i)
			delete __camMarkedTiles[label[i]];
	// apply instantly
	__camUpdateMarkedTiles();
}

//;; \subsection{camDebug(string...)}
//;; Pretty debug prints - a wrapper around \texttt{debug()}.
//;; Prints a function call stack and the argument message,
//;; prefixed with "DEBUG". Only use this function to indicate
//;; actual bugs in the scenario script, because game shouldn't
//;; print things when nothing is broken. If you want to keep
//;; some prints around to make debugging easier without distracting
//;; the user, use \texttt{camTrace()}.
function camDebug()
{
	__camGenericDebug("DEBUG",
	                  arguments.callee.caller.name,
	                  arguments,
	                  true,
	                  __camBacktrace());
}

//;; \subsection{camDebugOnce(string...)}
//;; Same as \texttt{camDebug()}, but prints each message only once
//;; during script lifetime.
function camDebugOnce()
{
	var str = arguments.callee.caller.name
		+ ": " + Array.prototype.join.call(arguments, " ");
	if (camDef(__camDebuggedOnce[str]))
		return;
	__camDebuggedOnce[str] = true;
	__camGenericDebug("DEBUG",
	                  arguments.callee.caller.name,
	                  arguments,
	                  true,
	                  __camBacktrace());
}

//;; \subsection{camTrace(string...)}
//;; Same as \texttt{camDebug()}, but only warns in cheat mode.
//;; Prefixed with "TRACE". It's safe and natural to keep \texttt{camTrace()}
//;; calls in your code for easier debugging.
function camTrace()
{
	if (!__camCheatMode)
		return;
	__camGenericDebug("TRACE",
	                  arguments.callee.caller.name,
	                  arguments);
}

//;; \subsection{camTraceOnce(string...)}
//;; Same as \texttt{camTrace()}, but prints each message only once
//;; during script lifetime.
function camTraceOnce()
{
	if (!__camCheatMode)
		return;
	var str = arguments.callee.caller.name
		+ ": " + Array.prototype.join.call(arguments, " ");
	if (camDef(__camTracedOnce[str]))
		return;
	__camTracedOnce[str] = true;
	__camGenericDebug("TRACE",
	                  arguments.callee.caller.name,
	                  arguments);
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
		for (var label in __camMarkedTiles)
			hackMarkTiles(label);
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
		for (var i = bt.length - 1; i >= 0; --i)
			debug("STACK: from", [bt[i]]);
	if (!func)
		func = "<anonymous>";
	var str = flag + ": " + func + ": " +
	      Array.prototype.join.call(args, " ");
	debug(str);
	if (camDef(err) && err)
		dump(str);
}

function __camBacktrace()
{
	var func = arguments.callee.caller;
	var list = [];
	while (camDef(func) && func)
	{
		if (func.name)
			list[list.length] = func.name;
		else
			list[list.length] = "<anonymous>";
		func = func.caller;
	}
	return list;
}


////////////////////////////////////////////////////////////////////////////////
// Artifacts management.
////////////////////////////////////////////////////////////////////////////////

//;; \subsection{camSetArtifacts(artifacts)}
//;; Tell \texttt{libcampaign.js} to manage a certain set of artifacts.
//;; The argument is a JavaScript map from object labels to artifact
//;; description. If the label points to a game object, artifact will be
//;; placed when this object is destroyed; if the label is a position, the
//;; artifact will be placed instantly. Artifact description is a JavaScript
//;; object with the following fields:
//;; \begin{description}
//;; 	\item[tech] The technology to grant when the artifact is recovered.
//;; \end{description}
//;; On \textbf{let me win} cheat, all technologies stored in the artifacts
//;; managed by this function are automatically granted.
//;; Additionally, this function would call special event callbacks if they are
//;; defined in your script, which should be named as follows,
//;; where LABEL is the artifact label:
//;; \begin{description}
//;; 	\item[camArtifactPickup_LABEL] Called when the player picks up
//;; 	the artifact.
//;; \end{description}
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

//;; \subsection{camAllArtifactsPickedUp()}
//;; Returns true if all artifacts managed by \texttt{libcampaign.js}
//;; were picked up.
function camAllArtifactsPickedUp()
{
	// FIXME: O(n) lookup here
	return __camNumArtifacts === Object.keys(__camArtifacts).length;
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
		return;
	var ai = __camArtifacts[alabel];
	if (!camDef(ai))
		return;
	if (ai.placed)
	{
		camDebug("Object to which artifact", alabel,
			"is bound has died twice");
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
		callback();
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
				continue;
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

//;; \subsection{camSetEnemyBases(bases)}
//;; Tell \texttt{libcampaign.js} to manage a certain set of enemy bases.
//;; Management assumes auto-cleanup of leftovers on destruction, and also
//;; counting how many bases have been successfully destroyed by the player.
//;; The argument is a JavaScript map from group labels to base descriptions.
//;; Each label points to a group of vital base structures. If no group label
//;; with this name is defined, a group is created automatically
//;; based on \textbf{cleanup} area and labeled. Base description
//;; is a JavaScript object with the following optional fields:
//;; \begin{description}
//;; 	\item[cleanup] An area label to clean up features in once base is
//;; 	destroyed. If base id is not a group label, this field is required
//;; 	in order to auto-create the group of stuff in the area which doesn't
//;; 	qualify as a valid leftover.
//;; 	\item[detectMsg] A PROX_MSG message id to play when the base is detected.
//;; 	\item[detectSnd] A sound file to play when the base is detected.
//;; 	\item[eliminateSnd] A sound file to play when the base is eliminated.
//;; 	The sound is played in the center of the cleanup area,
//;; 	which needs to be defined.
//;; 	\item{player} If base is detected by cleanup area, only objects
//;; 	matching this player filter would be added to the base group or
//;; 	cleaned up. Note that this most likely disables feature cleanup.
//;; \end{description}
//;; Additionally, this function would call special event callbacks if they are
//;; defined in your script, which should be named as follows,
//;; where LABEL is the label of the base group:
//;; \begin{description}
//;; 	\item[camEnemyBaseDetected_LABEL] Called when the player sees an object from
//;; 	the enemy base group for the first time.
//;; 	\item[camEnemyBaseEliminated_LABEL] Called when the base is eliminated,
//;; 	right after leftovers were cleaned up.
//;; \end{description}
function camSetEnemyBases(bases)
{
	__camEnemyBases = bases;
	__camNumEnemyBases = 0;
	// convert label strings to groups and store
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		var obj = getObject(blabel);
		if (camDef(obj) && obj) // group already defined
		{
			bi.group = obj.id;
			if (!camDef(bi.cleanup)) // auto-detect cleanup area
			{
				var objs = enumGroup(bi.group);
				if (objs.length > 0) {
					var a = {
						type: AREA,
						x: mapWidth, y: mapHeight,
						x2: 0, y2: 0
					};
					// smallest rectangle to contain all objects
					for (var i = 0; i < objs.length; ++i)
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
			bi.group = newGroup();
			addLabel({ type: GROUP, id: bi.group }, blabel);
			var structs = enumArea(bi.cleanup, ENEMIES, false);
			for (var i = 0; i < structs.length; ++i)
			{
				var s = structs[i];
				if (s.type !== STRUCTURE || __camIsValidLeftover(s))
					continue;
				if (!camDef(bi.player) ||
				    camPlayerMatchesFilter(s.player, bi.player))
				{
					camTrace("Auto-adding", s.id, "to base", blabel);
					groupAdd(bi.group, s);
				}
			}
		}
		if (groupSize(bi.group) === 0)
			camDebug("Base", blabel, "defined as empty group");
		bi.detected = false;
		bi.eliminated = false;
		camTrace("Resetting label", blabel);
		resetLabel(blabel, CAM_HUMAN_PLAYER); // subscribe for eventGroupSeen
	}
}

//;; \subsection{camDetectEnemyBase(base label)}
//;; Plays the "enemy base detected" message and places a beacon
//;; for the enemy base defined by the label, as if the base
//;; was actually found by the player.
function camDetectEnemyBase(blabel)
{
	var bi = __camEnemyBases[blabel];
	if (bi.detected || bi.eliminated)
		return;
	camTrace("Enemy base", blabel, "detected");
	bi.detected = true;
	if (camDef(bi.detectSnd))
	{
		var pos = camMakePos(bi.cleanup);
		if (!camDef(pos)) // auto-detect sound position by group object pos
		{
			var objs = enumGroup(bi.group);
			if (objs.length > 0)
				pos = camMakePos(objs[0]);
		}
		if (camDef(pos))
			playSound(bi.detectSnd, pos.x, pos.y, 0);
	}
	if (camDef(bi.detectMsg))
		hackAddMessage(bi.detectMsg, PROX_MSG, 0, false);
	var callback = __camGlobalContext()["camEnemyBaseDetected_" + blabel];
	if (camDef(callback))
		callback();
}

//;; \subsection{camAllEnemyBasesEliminated()}
//;; Returns true if all enemy bases managed by \texttt{libcampaign.js}
//;; are destroyed.
function camAllEnemyBasesEliminated()
{
	// FIXME: O(n) lookup here
	return __camNumEnemyBases === Object.keys(__camEnemyBases).length;
}

//;; \subsection{camSendReinforcement(player, position, droids, kind, data)}
//;; Give a single bunch of droids (template list) for a player at
//;; a position label. Kind can be one of:
//;; \begin{description}
//;; 	\item[CAM_REINFORCE_GROUND] Reinforcements magically appear
//;; 	on the ground,
//;; 	No data fields are currently supported.
//;; 	\item[CAM_REINFORCE_TRANSPORT] Reinforcements are unloaded from
//;; 	a transporter.
//;; 	\textbf{NOTE:} the game engine doesn't seem to support two simultaneous
//;; 	incoming transporters for the same player. Avoid this at all costs!
//;; 	The following data fields are required:
//;; 	\begin{description}
//;; 		\item[entry] Transporter entry position.
//;; 		\item[exit] Transporter exit position.
//;; 		\item[message] PROX_MSG to display when transport is landing.
//;; 		\item[order] Order to give to newly landed droids
//;; 		\item[data] Order data.
//;; 	\end{description}
//;; 	 \textbf{NOTE:} the game engine doesn't seem to support two simultaneous
//;; 	incoming transporters for the same player. If a transporter is already
//;; 	on map, it will be correctly queued up and sent later.
//;; \end{description}
function camSendReinforcement(player, position, list, kind, data)
{
	var pos = camMakePos(position);
	switch(kind)
	{
		case CAM_REINFORCE_GROUND:
			var droids = [];
			for (var i = 0; i < list.length; ++i)
			{
				droids[droids.length] = addDroid(player, pos.x, pos.y,
				                        "Reinforcement", list[i].body,
				                        list[i].prop, null, null, list[i].weap);
			}
			camManageGroup(camMakeGroup(droids), CAM_ORDER_ATTACK, {
				regroup: true,
				count: -1
			});
			break;
		case CAM_REINFORCE_TRANSPORT:
			var order = CAM_ORDER_ATTACK, order_data = {
				regroup: true,
				count: -1
			};
			if (camDef(data) && camDef(data.order))
			{
				order = data.order;
				if (camDef(data) && camDef(data.data))
				{
					order_data = data.data;
				}
			}
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
			break;
	}
}

//;; \subsection{camSetBaseReinforcements(base label, interval, callback, kind, data)}
//;; Periodically brings reinforcements to an enemy base, until the base is
//;; eliminated. Interval is the pause, in milliseconds, between reinforcements.
//;; Callback is name of a function that returns a list of droid templates to spawn,
//;; which may be different every time. Kind and data work similarly to
//;; \texttt{camSendReinforcement()}.
//;; Use CAM_REINFORCE_NONE as kind to disable previously set reinforcements.
function camSetBaseReinforcements(blabel, interval, callback, kind, data)
{
	if (!camIsString(callback))
		camDebug("Callback name must be a string (received", callback, ")");
	var bi = __camEnemyBases[blabel];
	bi.reinforce_kind = kind;
	bi.reinforce_interval = interval;
	bi.reinforce_callback = callback;
	bi.reinforce_data = data;
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
	args = __camTransporterQueue[0];
	var player = args.player;
	var pos = args.position;
	var list = args.list;
	var data = args.data;
	if (camDef(__camIncomingTransports[player]))
	{
		camTrace("Transporter already on map for player",
		         player + ", delaying.");
		return false;
	}
	__camTransporterQueue.shift(); // what could possibly go wrong?
	if (!camDef(__camPlayerTransports[player]))
	{
		camTrace("Creating a transporter for player", player);
		__camPlayerTransports[player] = addDroid(player, -1, -1,
		                                         "Transporter",
		                                         "TransporterBody",
		                                         "V-Tol", null, null,
		                                         "MG3-VTOL");
	}
	var trans = __camPlayerTransports[player];
	var droids = [];
	for (var i = 0; i < list.length; ++i)
	{
		var droid = addDroid(player, -1, -1,
		                     "Reinforcement", list[i].body,
		                     list[i].prop, null, null, list[i].weap);
		droids[droids.length] = droid;
		addDroidToTransporter(trans, droid);
	}
	__camIncomingTransports[player] = {
		droids: droids,
		message: args.data.message,
		order: args.order,
		data: args.order_data,
	}
	camTrace("Incoming transport with", droids.length,
	         "droids for player", player + 
	         ", queued transports", __camTransporterQueue.length);
	setNoGoArea(pos.x - 2, pos.y - 2, pos.x + 2, pos.y + 2, player);
	setTransporterExit(data.exit.x,
	                   data.exit.y, player);
	// will guess which transporter to start, automagically
	startTransporterEntry(data.entry.x, data.entry.y, player);
	if (camDef(__camTransporterMessage))
	{
		hackRemoveMessage(__camTransporterMessage, PROX_MSG, 0);
		__camTransporterMessage = undefined;
	}
	return true;
}

function __camDispatchTransporterSafe(player, position, list, data)
{
	if (!profile("__camDispatchTransporterUnsafe"))
		queue("__camDispatchTransporterSafe", 1000);
}

function __camCheckBaseSeen(seen)
{
	var group = seen; // group?
	if (camDef(seen.group)) // object?
		group = seen.group;
	if (!camDef(group) || !group)
		return;
	// FIXME: O(n) lookup here
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.group !== group)
			continue;
		camDetectEnemyBase(blabel);
	}
}

function __camIsValidLeftover(obj)
{
	if (camPlayerMatchesFilter(obj.player, ENEMIES))
		if (obj.type === STRUCTURE && obj.stattype === WALL)
			return true;
	if (obj.type === FEATURE)
		if (obj.stattype === BUILDING)
			return true;
	return false;
}

function __camCheckBaseEliminated(group)
{
	// FIXME: O(n) lookup here
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.group !== group)
			continue;
		camTrace("Enemy base", blabel, "eliminated");
		bi.eliminated = true;
		if (camDef(bi.cleanup))
		{
			var leftovers = enumArea(bi.cleanup);
			for (var i = 0; i < leftovers.length; i++)
			{
				var obj = leftovers[i];
				if (__camIsValidLeftover(obj))
				{
					// remove with special effect
					camSafeRemoveObject(obj, true);
				}
			}
			if (camDef(bi.eliminateSnd))
			{
				// play sound
				var pos = camMakePos(bi.cleanup);
				playSound(bi.eliminateSnd, pos.x, pos.y, 0);
			}
		}
		if (camDef(bi.detectMsg) && bi.detected) // remove the beacon
			hackRemoveMessage(bi.detectMsg, PROX_MSG, 0);
		// bump counter before the callback, so that it was
		// actual during the callback
		++__camNumEnemyBases;
		var callback = __camGlobalContext()["camEnemyBaseEliminated_" + blabel];
		if (camDef(callback))
			callback();
		break;
	}
}

function __camBasesTick()
{
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.eliminated || !camDef(bi.reinforce_kind))
			continue;
		if (gameTime - bi.reinforce_last < bi.reinforce_interval)
			continue;
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
	playSound("pcv395.ogg", pos.x, pos.y, 0);
	camManageGroup(camMakeGroup(ti.droids), ti.order, ti.data);
}


////////////////////////////////////////////////////////////////////////////////
// AI droid movement automation.
////////////////////////////////////////////////////////////////////////////////

const CAM_ORDER_ATTACK = 0;
const CAM_ORDER_DEFEND = 1;
const CAM_ORDER_PATROL = 2;
const CAM_ORDER_COMPROMISE = 3;
const CAM_ORDER_FOLLOW = 4;

//;; \subsection{camManageGroup(group, order, data)}
//;; Tell \texttt{libcampaign.js} to manage a certain group. The group
//;; would be permanently managed depending on the high-level orders given.
//;; For each order, data parameter is a JavaScript object that controls
//;; different aspects of behavior. The order parameter is one of:
//;; \begin{description}
//;; 	\item[CAM_ORDER_ATTACK] Pursue human player, preferably around
//;; 	the given position. The following optional data object fields are
//;; 	available, none of which is required:
//;; 	\begin{description}
//;; 		\item[pos] Position or list of positions to attack. If pos is a list,
//;; 		first positions in the list will be attacked first.
//;; 		\item[radius] Circle radius around \textbf{pos} to scan for targets.
//;; 		\item[fallback] Position to retreat.
//;; 		\item[morale] An integer from 1 to 100. If that high percentage
//;; 		of the original group dies, fall back to the fallback position.
//;; 		If new droids are added to the group, it can recover and attack
//;; 		again.
//;; 		\item[count] Override size of the original group. If unspecified,
//;; 		number of doids in the group at call time. Retreat on low morale
//;; 		and regroup is calculated against this value.
//;; 		\item[repair] Health percentage to fall back to repair facility,
//;; 		if any.
//;; 		\item[regroup] If set to true, the group will not move forward unless
//;; 		it has at least \textbf{count} droids in its biggest cluster.
//;; 		If \textbf{count} is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; 	\end{description}
//;; 	\item[CAM_ORDER_DEFEND] Protect the given position. If too far, retreat
//;; 	back there ignoring fire. The following data object fields are
//;; 	available:
//;; 	\begin{description}
//;; 		\item[pos] Position to defend.
//;; 		\item[radius] Circle radius around \textbf{pos} to scan for targets.
//;; 		\item[count] Override size of the original group. If unspecified,
//;; 		number of doids in the group at call time. Regroup is calculated
//;; 		against this value.
//;; 		\item[repair] Health percentage to fall back to repair facility,
//;; 		if any.
//;; 		\item[regroup] If set to true, the group will not move forward unless
//;; 		it has at least \textbf{count} droids in its biggest cluster.
//;; 		If \textbf{count} is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; 	\end{description}
//;; 	\item[CAM_ORDER_PATROL] Move droids randomly between a given list of
//;; 	positions. The following data object fields are available:
//;; 	\begin{description}
//;; 		\item[pos] An array of positions to patrol between.
//;; 		\item[interval] Change positions every this many milliseconds.
//;; 		\item[count] Override size of the original group. If unspecified,
//;; 		number of doids in the group at call time. Regroup is calculated
//;; 		against this value.
//;; 		\item[repair] Health percentage to fall back to repair facility,
//;; 		if any.
//;; 		\item[regroup] If set to true, the group will not move forward unless
//;; 		it has at least \textbf{count} droids in its biggest cluster.
//;; 		If \textbf{count} is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; 	\end{description}
//;; 	\item[CAM_ORDER_COMPROMISE] Same as CAM_ORDER_ATTACK, just stay near the
//;; 	last (or only) attack position instead of looking for the player
//;; 	around the whole map. Useful for offworld missions,
//;; 	with player's LZ as the final position. The following data object fields
//;; 	are available:
//;; 	\begin{description}
//;; 		\item[pos] Position or list of positions to compromise. If pos is a list,
//;; 		first positions in the list will be compromised first.
//;; 		\item[radius] Circle radius around \textbf{pos} to scan for targets.
//;; 		\item[count] Override size of the original group. If unspecified,
//;; 		number of doids in the group at call time. Regroup is calculated
//;; 		against this value.
//;; 		\item[repair] Health percentage to fall back to repair facility,
//;; 		if any.
//;; 		\item[regroup] If set to true, the group will not move forward unless
//;; 		it has at least \textbf{count} droids in its biggest cluster.
//;; 		If \textbf{count} is set to -1, at least 2/3 of group's droids should be in
//;; 		the biggest cluster.
//;; 	\end{description}
//;; 	\item[CAM_ORDER_FOLLOW] Assign the group to commander. The sub-order
//;; 	is defined to be given to the commander. When commander dies,
//;; 	the group continues to execute the sub-order. The following data object
//;; 	fields are available:
//;; 	\begin{description}
//;; 		\item[droid] Commander droid label.
//;; 		\item[order] The order to give to the commander.
//;; 		\item[data] Data of the commander's order.
//;; 		\item[repair] Health percentage to fall back to repair facility,
//;; 		if any.
//;; 	\end{description}
//;; \end{description} 
function camManageGroup(group, order, data)
{
	var saneData = data;
	if (!camDef(saneData))
		saneData = {};
	if (camDef(saneData.pos)) // sanitize pos now to make ticks faster
	{
		if (camIsString(saneData.pos)) // single label?
			saneData.pos = [ saneData.pos ];
		else if (!camDef(saneData.pos.length)) // single position object?
			saneData.pos = [ saneData.pos ];
		for (var i = 0; i < saneData.pos.length; ++i) // array of labels?
			saneData.pos[i] = camMakePos(saneData.pos[i]);
	}
	if (camDef(__camGroupInfo[group])
	    && order !== __camGroupInfo[group].order)
	{
		camTrace("Group", group, "receives a new order:",
		         camOrderToString(order));
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

//;; \subsection{camStopManagingGroup(group)}
//;; Tell \texttt{libcampaign.js} to stop managing a certain group.
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

//;; \subsection{camManageTrucks(player)}
//;; Manage trucks for an AI player. This assumes recapturing oils and
//;; rebuilding destroyed trucks in factories, the latter is implemented
//;; via \texttt{camQueueDroidProduction()} mechanism.
function camManageTrucks(player)
{
	__camTruckInfo[player] = { enabled: 1, queue: [] };
}

//;; \subsection{camQueueBuilding(player, stat[, pos])}
//;; Assuming truck management is enabled for the player, issue an order
//;; to build a specific building near a certain position. The order
//;; would be issued once as soon as a free truck becomes available. It will
//;; not be re-issued in case the truck is destroyed before the building
//;; is finished. If position is unspecified, the building would be built
//;; near the first available truck. Otherwise, position may be a label
//;; or a POSITION-like object.
function camQueueBuilding(player, stat, pos)
{
	var ti = __camTruckInfo[player];
	ti.queue[ti.queue.length] = { stat: stat, pos: camMakePos(pos) };
}

//;; \subsection{camOrderToString(order)}
//;; Print campaign order as string, useful for debugging.
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
		var x = list[i].x, y = list[i].y;
		var found = false;
		for (var j = 0; j < ret.clusters.length; ++j) {
			if (camDist(ret.xav[j], ret.yav[j], x, y) < size)
			{
				var n = ret.clusters[j].length;
				ret.clusters[j][n] = list[i];
				ret.xav[j] = (n * ret.xav[j] + x) / (n + 1);
				ret.yav[j] = (n * ret.yav[j] + y) / (n + 1);
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
				ret.maxCount = 1
			}
		}
	}
	return ret;
}

function __camPickTarget(group)
{
	var targets = [];
	var gi = __camGroupInfo[group];
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
						radius = __CAM_PLAYER_BASE_RADIUS;
					targets = enumRange(gi.data.pos[i].x,
					                    gi.data.pos[i].y,
					                    radius,
					                    CAM_HUMAN_PLAYER, false);
				}
			}
			if (gi.order === CAM_ORDER_COMPROMISE && !targets.length)
			{
				if (!camDef(gi.data.pos))
					camDebug("`pos' is required for COMPROMISE order")
				else
					targets = [ gi.data.pos[gi.data.pos.length - 1] ];
			}
			if (!targets.length)
				targets = enumStruct(CAM_HUMAN_PLAYER);
			if (!targets.length)
				targets = enumDroid(CAM_HUMAN_PLAYER);
			break;
		case CAM_ORDER_DEFEND:
			if (!camDef(gi.data.pos))
				camDebug("`pos' is required for DEFEND order")
			var radius = gi.data.radius;
			if (!camDef(radius))
				radius = __CAM_DEFENSE_RADIUS;
			if (camDef(gi.target)
				&& camDist(gi.target, gi.data.pos[0]) < radius)
			{
				targets = enumRange(gi.target.x, gi.target.y,
				                    __CAM_TARGET_TRACKING_RADIUS,
				                    CAM_HUMAN_PLAYER, false);
			}
			if (!targets.length)
				targets = enumRange(gi.data.pos[0].x, gi.data.pos[0].y,
				                    radius, CAM_HUMAN_PLAYER, false);
			if (!targets.length)
				targets = [ gi.data.pos[0] ];
			break;
		default:
			camDebug("Unsupported group order", gi.order,
			         camOrderToString(gi.order));
			break;
	}
	if (!targets.length)
		return undefined;
	var target = targets[camRand(targets.length)];
	// sanitize data to make sure no bad things happen when object dies
	target = { x : target.x, y : target.y };
	__camGroupInfo[group].target = target;
	return target;
}

function __camTacticsTick()
{
	var dt = CAM_TICKS_PER_FRAME;
	for (var group in __camGroupInfo)
	{
		queue("__camTacticsTickForGroup", dt, group);
		dt += CAM_TICKS_PER_FRAME;
	}
	queue("__camTacticsTick", dt);
}

function __camTacticsTickForGroup(group)
{
	var gi = __camGroupInfo[group];
	if (!camDef(gi))
		return;
	var rawDroids = enumGroup(group);
	if (rawDroids.length === 0)
		return;
	var healthyDroids = [];

	// Handle the case when we need to repair
	if (rawDroids.length > 0 && camDef(gi.data.repair)
	            && countStruct("A0RepairCentre3", rawDroids[0].player))
	{
		for (var i = 0; i < rawDroids.length; ++i)
		{
			var droid = rawDroids[i];
			if (droid.order === DORDER_RTR)
				continue;
			if (droid.health < gi.data.repair)
			{
				orderDroid(droid, DORDER_RTR);
				continue;
			}
			healthyDroids[healthyDroids.length] = droid;
		}
	}
	else
	{
		healthyDroids = rawDroids;
	}

	// Handle regrouping
	var droids = healthyDroids;
	if (camDef(gi.data.regroup) && gi.data.regroup
	              && healthyDroids.length > 0)
	{
		var ret = __camFindClusters(healthyDroids, __CAM_CLUSTER_SIZE);
		var groupX = ret.xav[ret.maxIdx];
		var groupY = ret.yav[ret.maxIdx];
		droids = ret.clusters[ret.maxIdx];
		for (var i = 0; i < ret.clusters.length; ++i)
			if (i != ret.maxIdx) // move other droids towards main cluster
				for (var j = 0; j < ret.clusters[i].length; ++j)
				{
					var droid = ret.clusters[i][j];
					orderDroidLoc(droid, DORDER_MOVE, groupX, groupY);
				}
		var back = (gameTime - gi.lastHit < __CAM_FALLBACK_TIME_ON_REGROUP);
		// not enough droids grouped?
		if (gi.count < 0
		    ? (ret.maxCount < groupSize(group) * 0.66)
		    : (ret.maxCount < gi.count))
		{
			for (var i = 0; i < droids.length; ++i)
			{
				var droid = droids[i];
				if (back)
					orderDroid(droid, DORDER_RTB);
				else if (droid.order !== DORDER_MOVE) // to assembly point?
					orderDroid(droid, DORDER_STOP);
			}
			return;
		}
	}

	if (droids.length === 0)
		return;

	switch(gi.order)
	{
		case CAM_ORDER_ATTACK:
		case CAM_ORDER_DEFEND:
		case CAM_ORDER_COMPROMISE:
			var target = profile("__camPickTarget", group);
			if (!camDef(target))
				return;
			for (var i = 0; i < droids.length; ++i)
			{
				var droid = droids[i];
				if (droid.player === CAM_HUMAN_PLAYER)
				{
					camDebug("Controlling a human player's droid");
				}
				var done = false;
				if (gi.order === CAM_ORDER_DEFEND)
				{
					// fall back to defense position
					var dist = camDist(droid, gi.data.pos);
					var radius = gi.data.radius;
					if (!camDef(radius))
						radius = __CAM_DEFENSE_RADIUS;
					if (dist > radius)
					{
						orderDroidLoc(droid, DORDER_MOVE,
						              gi.data.pos.x, gi.data.pos.y);
						done = true;
					}
				}
				if (!done && droid.order !== DORDER_SCOUT
					&& camDist(droid, target) >= __CAM_CLOSE_RADIUS)
				{
					orderDroidLoc(droid, DORDER_SCOUT,
					              target.x, target.y);
				}
			}
			break;
		case CAM_ORDER_PATROL:
			if (!camDef(gi.lastmove) || !camDef(gi.lastspot))
			{
				gi.lastspot = 0;
			}
			else
			{
				if (gameTime - gi.lastmove < gi.data.interval)
					return;
				// find random new position to visit
				var list = [];
				for (var i = 0; i < gi.data.pos.length; ++i)
					if (i !== gi.lastpos)
						list[list.length] = i;
				gi.lastspot = list[camRand(list.length)];
			}
			gi.lastmove = gameTime;
			var pos = gi.data.pos[gi.lastspot];
			for (var i = 0; i < droids.length; ++i)
			{
				var droid = droids[i];
				orderDroidLoc(droid, DORDER_MOVE, pos.x, pos.y);
			}
			break;
		case CAM_ORDER_FOLLOW:
			var commander = getObject(gi.data.droid);
			if (!camDef(commander) || !commander)
			{
				// the commander is dead? let the group execute
				// his last will.
				camManageGroup(group, gi.data.order, gi.data.data);
				break;
			}
			for (var i = 0; i < droids.length; ++i)
			{
				var droid = droids[i];
				if (droid.order !== DORDER_COMMANDERSUPPORT)
					orderDroidObj(droid, DORDER_COMMANDERSUPPORT,
					              commander);
			}
			break;
		default:
			camDebug("Unknown group order", gi.order);
			break;
	}
}

function __camCheckGroupMorale(group)
{
	var gi = __camGroupInfo[group];
	if (!camDef(gi.data.morale))
		return;
	// morale is %.
	var msize = Math.floor((100 - gi.data.morale) * gi.count / 100);
	var gsize = groupSize(group);
	switch(gi.order)
	{
		case CAM_ORDER_ATTACK:
			if (gsize > msize)
				break;
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
				break;
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
			camDebug("Group order doesn't support morale",
			         camOrderToString(gi.order));
			break;
	}
}

function __camEnumFreeTrucks(player)
{
	var rawDroids = enumDroid(player, DROID_CONSTRUCT);
	var droids = [];
	for (var i = 0; i < rawDroids.length; ++i)
	{
		var droid = rawDroids[i];
		if (droid.order !== DORDER_BUILD && droid.order !== DORDER_HELPBUILD
		                                 && droid.order !== DORDER_LINEBUILD)
		{
			droids[droids.length] = droid;
		}
	}
	return droids;
}

function __camGetClosestTruck(player, pos)
{
	var droids = __camEnumFreeTrucks(player);
	if (droids.length <= 0)
		return undefined;

	// Find out which one is the closest.
	var minDroid = droids[0];
	var minDist = camDist(minDroid, pos);
	for (var i = 1; i < droids.length; ++i)
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
	for (player in __camTruckInfo)
	{
		var ti = __camTruckInfo[player];

		// First, build things that were explicitly ordered.
		if (ti.queue.length > 0)
		{
			var qi = ti.queue[0];
			var pos = qi.pos;
			var truck;
			if (camDef(pos))
			{
				// Find the truck most suitable for the job.
				truck = __camGetClosestTruck(player, pos);
				if (!camDef(truck))
					continue;
			}
			else
			{
				// Build near any truck if pos was not specified.
				var droids = __camEnumFreeTrucks(player);
				if (droids.length <= 0)
					continue;
				truck = droids[0];
				pos = truck;
			}

			enableStructure(qi.stat, player);
			var loc = pickStructLocation(truck, qi.stat, pos.x, pos.y);
			if (camDef(loc))
			{
				if (orderDroidBuild(truck, DORDER_BUILD,
				                    qi.stat, loc.x, loc.y))
				{
					ti.queue.shift(); // consider it handled
					continue;
				}
			}
		}

		// Then, capture free oils.
		var oils = enumFeature(-1, "OilResource");
		if (oils.length === 0)
			continue;
		var oil = oils[0];
		var truck = __camGetClosestTruck(player, oil);
		if (camDef(truck))
		{
			enableStructure("A0ResourceExtractor", player);
			orderDroidBuild(minDroid, DORDER_BUILD, "A0ResourceExtractor",
			                oil.x, oil.y);
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
		// cannot use obj.weapons[] because spade aint a weapon
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

//;; \subsection{camSetFactories(factories)}
//;; Tell \texttt{libcampaign.js} to manage a certain set of enemy factories.
//;; Management assumes producing droids, packing them into groups and
//;; executing orders once the group becomes large-enough.
//;; The argument is a JavaScript map from group labels to factory descriptions.
//;; Each label points to a factory object. Factory description
//;; is a JavaScript object with the following fields:
//;; \begin{description}
//;; 	\item[assembly] A rally point position label, where the group would
//;; 	gather.
//;; 	\item[order] An order to execute for every group produced in the
//;; 	factory. Same as the order parameter for \texttt{camManageGroup()}.
//;; 	\item[data] Order data. Same as the data parameter for
//;; 	\texttt{camManageGroup()}.
//;; 	\item[groupSize] Number of droids to produce before executing the order.
//;; 	Also, if order is CAM_ORDER_ATTACK, data.count defaults to this value.
//;; 	\item[maxSize] Halt production when reaching that many droids in the
//;; 	factory group. Resume when some droids die. Unlimited if unspecified.
//;; 	\item[throttle] If defined, produce droids only every that many
//;; 	milliseconds, and keep the factory idle between ticks.
//;; 	\item[group] If defined, make the factory manage this group,
//;; 	otherwise create a new empty group to manage.
//;; 	Droids produced in the factory would automatically be
//;; 	added to the group, and order and data parameters
//;; 	would be applied to this group.
//;; 	\item[templates] List of droid templates to produce in the factory.
//;; 	Each template is a JavaScript object with the following fields:
//;; 	\begin{description}
//;; 		\item[body] Body stat name.
//;; 		\item[prop] Propulsion stat name.
//;; 		\item[weap] Weapon stat name. Only single-turret droids are
//;; 		currently supported.
//;; 	\end{description}
//;; 	Note that all template components are automatically made available
//;; 	to the factory owner.
//;; \end{description}
//;; Factories won't start production immediately; call
//;; \texttt{camEnableFactory()} to turn them on when necessary.
function camSetFactories(factories)
{
	for (var flabel in factories)
		camSetFactoryData(flabel, factories[flabel]);
}

//;; \subsection{camSetFactoryData(factory label, factory description)}
//;; Similar to \texttt{camSetFactories()}, but one factory at a time.
//;; If the factory was already managing a group of droids, it keeps
//;; managing it. If a new group is specified in the description,
//;; the old group is merged into it. NOTE: This function disables the
//;; factory. You would need to call \texttt{camEnableFactory()} again.
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
		droids = enumGroup(__camFactoryInfo[flabel].group);
	__camFactoryInfo[flabel] = fdata;
	var fi = __camFactoryInfo[flabel];
	if (!camDef(fi.data))
		fi.data = {};
	fi.enabled = false;
	fi.state = 0;
	if (!camDef(fi.group))
		fi.group = newGroup();
	for (var i = 0; i < droids.length; ++i)
		groupAdd(fi.group, droids[i]);
	if (!camDef(fi.data.count))
		fi.data.count = fi.groupSize;
}

//;; \subsection{camEnableFactory(factory label)}
//;; Enable a managed factory by the given label. Once the factory is enabled,
//;; it starts producing units and executing orders as given.
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
		camTrace("Factory", flabel, "enabled twice");
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

//;; \subsection{camQueueDroidProduction(player, template)}
//;; Queues up an extra droid template for production. It would be produced
//;; in the first factory that is capable of producing it, at the end of
//;; its production loop, first queued first served.
function camQueueDroidProduction(player, template)
{
	if (!camDef(__camFactoryQueue[player]))
		__camFactoryQueue[player] = [];
	__camFactoryQueue[player][__camFactoryQueue[player].length] = template;
}

//////////// privates

var __camFactoryInfo;
var __camFactoryQueue;

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
		fi.group = newGroup();
	}
	else
	{
		var pos = camMakePos(fi.assembly);
		if (!camDef(pos))
			pos = camMakePos(flabel);
		camManageGroup(fi.group, CAM_ORDER_DEFEND, { pos: pos });
	}
}

function __camAddDroidToFactoryGroup(droid, structure)
{
	// don't manage trucks in this function
	if (droid.droidType === DROID_CONSTRUCT)
		return;
	// FIXME: O(n) lookup here
	var flabel = getLabel(structure);
	if (!camDef(flabel) || !flabel)
		return;
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

function __camBuildDroid(template, structure)
{
	makeComponentAvailable(template.body, structure.player);
	makeComponentAvailable(template.prop, structure.player);
	makeComponentAvailable(template.weap, structure.player);
	var n = [ structure.name, structure.id,
	          template.body, template.prop, template.weap ].join(" ");
	// multi-turret templates are not supported yet
	return buildDroid(structure, n, template.body, template.prop,
	                          null, null, template.weap);
}

function __camContinueProduction(structure)
{
	var flabel, struct;
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
	if (!structureIdle(struct))
	{
		camDebug("Already enabled?");
		return;
	}
	if (!camDef(flabel) || !flabel)
		return;
	var fi = __camFactoryInfo[flabel];
	if (camDef(fi.maxSize) && groupSize(fi.group) >= fi.maxSize)
	{
		// retry in 5 seconds
		queue("__camContinueProduction", 5000, flabel);
		return;
	}
	if (camDef(fi.throttle) && camDef(fi.lastprod))
	{
		var throttle = gameTime - fi.lastprod;
		if (throttle < fi.throttle)
		{
			// do throttle
			queue("__camContinueProduction",
			      fi.throttle - throttle, flabel);
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
		fi.state = -1;
	fi.lastprod = gameTime;
}


////////////////////////////////////////////////////////////////////////////////
// Victory celebration helpers.
////////////////////////////////////////////////////////////////////////////////

//;; \subsection{camNextLevel(next level)}
//;; A wrapper around \texttt{loadLevel()}. Remembers to give bonus power
//;; for completing the mission faster - 25 percent more than what you could have
//;; received by milking this time limit down.
function camNextLevel(nextLevel)
{
	if (__camNeedBonusTime)
	{
		var bonusTime = getMissionTime();
		if (bonusTime > 0)
		{
			camTrace("Bonus time", bonusTime);
			setPowerModifier(125); // 25% bonus to completing fast
			extraPowerTime(bonusTime);
			setPowerModifier(100);
		}
	}
	loadLevel(nextLevel);
}

const CAM_VICTORY_STANDARD = 0;
const CAM_VICTORY_PRE_OFFWORLD = 1;
const CAM_VICTORY_OFFWORLD = 2;

//;; \subsection{camSetStandardWinLossConditions(kind, nextLevel, data)}
//;; Set victory and defeat conditions to one of the common
//;; options. On victory, load nextLevel. The extra data parameter
//;; contains extra data required to define some of the victory
//;; conditions. The following options are available:
//;; \begin{description}
//;; 	\item[CAM_VICTORY_STANDARD] Defeat if all ground factories
//;; 	and construction droids are lost, or on mission timeout.
//;; 	Victory when all enemies are destroyed and all artifacts
//;; 	are recovered.
//;; 	The following data parameter fields are available:
//;; 	\begin{description}
//;; 		\item[calllback] A function callback to check for extra win/loss
//;; 		conditions. Return values are interpreted as follows:
//;; 		\textbf{false} means instant defeat ("objective failed"),
//;; 		\textbf{true} means victory as long as other standard victory
//;; 		conditions are met, \textbf{undefined} means suppress
//;; 		other victory checks ("clearly not won yet").
//;; 	\end{description}
//;; 	\item[CAM_VICTORY_PRE_OFFWORLD] Defeat on timeout. Victory on
//;; 	transporter launch, then load the sub-level.
//;; 	\item[CAM_VICTORY_OFFWORLD] Defeat on timeout or all units lost.
//;; 	Victory when all artifacts are recovered and either all enemies
//;; 	are dead (not just bases) or all droids are at the LZ.
//;; 	Also automatically handles the "LZ compromised" message,
//;; 	which is why it needs to know reinforcement interval to restore.
//;; 	The following data parameter fields are available:
//;; 	\begin{description}
//;; 		\item[area] The landing zone to return to.
//;; 		\item[message] The "Return to LZ" message ID. Optional.
//;; 		\item[reinforcements] Reinforcements interval, in seconds.
//;; 	\end{description}
//;; \end{description}
function camSetStandardWinLossConditions(kind, nextLevel, data)
{
	switch(kind)
	{
		case CAM_VICTORY_STANDARD:
			__camWinLossCallback = "__camVictoryStandard";
			__camNeedBonusTime = true;
			__camDefeatOnTimeout = true;
			__camVictoryData = data;
			break;
		case CAM_VICTORY_PRE_OFFWORLD:
			__camWinLossCallback = "__camVictoryPreOffworld";
			__camNeedBonusTime = false;
			__camDefeatOnTimeout = true;
			break;
		case CAM_VICTORY_OFFWORLD:
			__camWinLossCallback = "__camVictoryOffworld";
			__camNeedBonusTime = true;
			__camDefeatOnTimeout = true;
			__camVictoryData = data;
			setReinforcementTime(__camVictoryData.reinforcements);
			queue("__camSetOffworldLimits", 100);
			break;
		default:
			camDebug("Unknown standard victory condition", kind);
			break;
	}
	__camNextLevel = nextLevel;
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
	// That's the only structure that doesn't get
	// auto-disabled by the engine in off-world missions.
	setStructureLimits("A0CommandCentre", 0, 0);
}

function __camGameLostCB()
{
	camTrace();
	gameOverMessage(false);
}

function __camGameLost()
{
	camCallOnce("__camGameLostCB");
}

function __camGameWon()
{
	__camLevelEnded = true;
	camTrace(__camNextLevel);
	if (camDef(__camNextLevel))
		camNextLevel(__camNextLevel);
}

function __camPlayerDead()
{
	if (countStruct("A0LightFactory") + countStruct("A0CyborgFactory") > 0)
		return false;
	return (countDroid(DROID_CONSTRUCT) === 0);
}

function __camTriggerLastAttack()
{
	if (!__camLastAttackTriggered)
	{
		var enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false);
		camTrace(enemies.length, "enemy droids remaining");
		camManageGroup(camMakeGroup(enemies), CAM_ORDER_ATTACK);
		__camLastAttackTriggered = true;
	}
}

function __camVictoryStandard()
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
	// check if game is lost
	if (__camPlayerDead())
	{
		__camGameLost()
		return;
	}
	// check if game is won
	if (camAllArtifactsPickedUp() && camAllEnemyBasesEliminated() && extraObjMet)
	{
		if (enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).length === 0)
		{
			__camGameWon();
			return;
		}
		else
			__camTriggerLastAttack();
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

function __camVictoryOffworld()
{
	var lz = __camVictoryData.area;
	if (!camDef(lz))
	{
		camDebug("Landing zone area is required for OFFWORLD")
		return;
	}
	var total = countDroid(DROID_ANY, CAM_HUMAN_PLAYER); // for future use
	if (total === 0)
		__camGameLost();
	if (camAllArtifactsPickedUp())
	{
		if (enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).length === 0)
		{
			// if there are no more enemies, win instantly
			__camGameWon();
			return;
		}

		var atlz = enumArea(lz, CAM_HUMAN_PLAYER, false).length;
		if (atlz === total)
		{
			__camGameWon();
			return;
		}
		else
			__camTriggerLastAttack();
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
			console("Return to LZ");
		}
		++__camRTLZTicker;
	}
	if (enumArea(lz, ENEMIES, false).length > 0)
	{
		if (__camLZCompromisedTicker === 0)
			camTrace("LZ compromised");
		if (__camLZCompromisedTicker % 30 === 1) // every 30 seconds
		{
			var pos = camMakePos(lz);
			playSound("pcv445.ogg", pos.x, pos.y, 0);
			setReinforcementTime(LZ_COMPROMISED_TIME);
		}
		++__camLZCompromisedTicker;
		if (__camRTLZTicker === 0)
			camMarkTiles(lz);
	}
	else if (__camLZCompromisedTicker > 0)
	{
		camTrace("LZ clear");
		var pos = camMakePos(lz);
		playSound("lz-clear.ogg", pos.x, pos.y, 0);
		setReinforcementTime(__camVictoryData.reinforcements);
		__camLZCompromisedTicker = 0;
		if (__camRTLZTicker === 0)
			camUnmarkTiles(lz);
	}
}


////////////////////////////////////////////////////////////////////////////////
// Transporter management.
////////////////////////////////////////////////////////////////////////////////

//;; \subsection{camSetupTransport(place x, place y, exit x, exit y)}
//;; A convenient function for placing the standard campaign transport
//;; for loading in pre-away missions. The exit point for the transport
//;; is set up as well.
function camSetupTransporter(x, y, x1, y1)
{
	addDroid(CAM_HUMAN_PLAYER, x, y, "Transport",
	         "TransporterBody", "V-Tol", null, null, "MG3-VTOL");
	setTransporterExit(x1, y1, CAM_HUMAN_PLAYER);
}


////////////////////////////////////////////////////////////////////////////////
// Event hooks magic. This makes all the library catch all the necessary events
// without distracting the script authors from handling them on their own.
// It assumes that script authors do not implement another similar mechanism,
// or something BAD would happen.
////////////////////////////////////////////////////////////////////////////////

//;; \subsection{camAreaEvent(label, function(droid))}
//;; Implement eventArea<label> in a debugging-friendly way. The function
//;; marks the area until the event is triggered, and traces entering the area
//;; in the TRACE log.
function camAreaEvent(label, code)
{
	var eventName = "eventArea" + label;
	camMarkTiles(label);
	__camPreHookEvent(eventName, function(droid)
	{
		if (camDef(droid))
			camTrace("Player", droid.player, "enters", label);
		camUnmarkTiles(label);
		code(droid);
	});
}

//////////// privates

var __camOriginalEvents = {};

function __camPreHookEvent(eventname, hookcode)
{
	// store the original event handler
	if (camDef(__camGlobalContext()[eventname]))
		__camOriginalEvents[eventname] = __camGlobalContext()[eventname];
	__camGlobalContext()[eventname] = function()
	{
		// Don't trigger hooks after level end.
		// No, this should't say "if (__camLevelEnded) return;".
		// Because we are not hooking all possible events,
		// and we are not documenting which exactly events are hooked.
		// We should not intervene with level events in unobvious ways.
		if (!__camLevelEnded)
			hookcode.apply(__camGlobalContext(), arguments);
		// otherwise, call the original event if any
		if (camDef(__camOriginalEvents[eventname]))
			__camOriginalEvents[eventname].apply(__camGlobalContext(), arguments);
	};
}

/* Called every second after eventStartLevel(). */
function __camTick()
{
	if (camDef(__camWinLossCallback))
		__camGlobalContext()[__camWinLossCallback]();
	__camBasesTick();
}

var __camLastHitTime = 0;

const __CAM_EVENT_ATTACKED_INTENSITY = 5000;

__camPreHookEvent("eventPickup", function(feature, droid)
{
	if (feature.stattype === ARTIFACT)
	{
		__camPickupArtifact(feature);
	}
});

__camPreHookEvent("eventGroupLoss", function(obj, group, newsize)
{
	if (newsize === 0)
		__camCheckBaseEliminated(group);
	if (camDef(__camGroupInfo[group]))
		profile("__camCheckGroupMorale", group);
});

__camPreHookEvent("eventCheatMode", function(entered)
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
});

__camPreHookEvent("eventChat", function(from, to, message)
{
	if (!__camCheatMode)
		return;
	camTrace(from, to, message);
	if (message === "let me win")
		__camLetMeWin();
	if (message === "win info")
		__camWinInfo();
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
});

__camPreHookEvent("eventStartLevel", function()
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
	setTimer("__camTick", 1000); // campaign pollers
	setTimer("__camTruckTick", 150100); // some slower campaign pollers
	queue("__camTacticsTick", 100); // would re-queue itself
});

__camPreHookEvent("eventDroidBuilt", function(droid, structure)
{
	if (!camDef(structure)) // "clone wars" cheat
		return;
	if (!camPlayerMatchesFilter(structure.player, ENEMIES))
		return;
	if (!camPlayerMatchesFilter(droid.player, ENEMIES))
		return;
	if (!camDef(__camFactoryInfo))
		return;
	__camContinueProduction(structure);
	__camAddDroidToFactoryGroup(droid, structure);
});

__camPreHookEvent("eventDestroyed", function(obj)
{
	__camCheckPlaceArtifact(obj);
	if (obj.type === DROID && obj.droidType === DROID_CONSTRUCT)
		__camCheckDeadTruck(obj);
});

__camPreHookEvent("eventObjectSeen", function(viewer, seen)
{
	__camCheckBaseSeen(seen);
});

__camPreHookEvent("eventGroupSeen", function(viewer, group)
{
	__camCheckBaseSeen(group);
});

__camPreHookEvent("eventTransporterExit", function(transport)
{
	if (transport.player !== CAM_HUMAN_PLAYER)
	{
		// allow the next transport to enter
		if (camDef(__camIncomingTransports[transport.player]))
			delete __camIncomingTransports[transport.player];
	}
	else if (__camWinLossCallback === "__camVictoryPreOffworld")
	{
		camTrace("Transporter is away.");
		__camGameWon();
		return;
	}
});

__camPreHookEvent("eventTransporterLanded", function(transport)
{
	if (transport.player !== CAM_HUMAN_PLAYER)
		__camLandTransporter(transport.player, camMakePos(transport));
});

__camPreHookEvent("eventMissionTimeout", function()
{
	if (__camDefeatOnTimeout)
	{
		camTrace("0 minutes remaining.");
		__camGameLost();
	}
});

__camPreHookEvent("eventAttacked", function(victim, attacker)
{
	if (camDef(victim) && victim &&
	    victim.type === DROID && camDef(__camGroupInfo[victim.group]))
	{
		__camGroupInfo[victim.group].lastHit = gameTime;
	}
});
