
////////////////////////////////////////////////////////////////////////////////
// Victory celebration helpers.
////////////////////////////////////////////////////////////////////////////////

//;; ## camNextLevel(nextLevel)
//;;
//;; A wrapper around `loadLevel()`. Remembers to give bonus power for completing the mission faster.
//;;
//;; @param {string} nextLevel
//;; @returns {void}
//;;
function camNextLevel(nextLevel)
{
	if (tweakOptions.timerPowerBonus && __camNeedBonusTime)
	{
		const __POWER_TIME_INFINITE = camMinutesToSeconds(10); // A small and flat reward if using Infinite Time option.
		const __POWER_TIME_REMAINING = (tweakOptions.infiniteTime) ? __POWER_TIME_INFINITE : getMissionTime();
		const __TIME_UNDERFLOW = __POWER_TIME_REMAINING >= 86400; // Ended mission right when the timer ran out.
		if ((__POWER_TIME_REMAINING > 0) && !__TIME_UNDERFLOW)
		{
			let bonus = 110;
			if (difficulty === HARD)
			{
				bonus = 90; //Small bonus compared to the regular Hard extraction value.
			}
			else if (difficulty >= INSANE)
			{
				bonus = 70; //Same value for oil extraction.
			}
			camTrace("Bonus time", __POWER_TIME_REMAINING);
			setPowerModifier(bonus); // Bonus percentage for completing fast
			extraPowerTime(__POWER_TIME_REMAINING);
			setPowerModifier(100);
		}
	}
	__camRemoveNavGunSensorResearch();
	camBreakAlliances();
	//Set these limits again for the home map before exiting this mission
	setStructureLimits(cam_base_structures.commandCenter, 1, CAM_HUMAN_PLAYER);
	setStructureLimits(cam_base_structures.commandRelay, 1, CAM_HUMAN_PLAYER);
	loadLevel(nextLevel);
}

//;; ## camSetStandardWinLossConditions(kind, nextLevel, data)
//;;
//;; Set victory and defeat conditions to one of the common options. On victory, load nextLevel.
//;; The extra data parameter contains extra data required to define some of the victory conditions.
//;; The following options are available:
//;; * `CAM_VICTORY_STANDARD` Defeat if all ground factories and construction droids are lost, or on mission timeout.
//;;   Victory when all enemies are destroyed and all artifacts are recovered.
//;; * `CAM_VICTORY_PRE_OFFWORLD` Defeat on timeout. Victory on transporter launch, then load the sub-level.
//;; * `CAM_VICTORY_OFFWORLD` Defeat on timeout or all units lost.
//;;   Victory when all artifacts are recovered and either all enemies are dead (not just bases) or all droids are at the LZ.
//;;   Also automatically handles the "LZ compromised" message, which is why it needs to know reinforcement interval to restore.
//;;   The following data parameter fields are available:
//;;   * `area` The landing zone to return to.
//;;   * `message` The "Return to LZ" message ID. Optional.
//;;   * `playLzReminder` Play and show the "Return to LZ" message. Optional, enabled by default.
//;;   * `reinforcements` Reinforcements interval, in seconds.
//;; For standard and offworld victory, some extra data parameters can be defined:
//;; * `callback` A function callback to check for extra win/loss conditions. Return values are interpreted as follows:
//;;   * `false` means instant defeat ("objective failed"),
//;;   * `true` means victory as long as other standard victory conditions are met,
//;;   * `undefined` means suppress other victory checks ("clearly not won yet").
//;; For offworld victory, some more extra data parameters can be defined:
//;; * `retlz` Force the player to return to the LZ area:
//;;   * `false` mission does not require a LZ return,
//;;   * `true` mission requires all units to be at LZ to win.
//;; * `annihilate` Player must destroy every thing on map to win:
//;;   * `false` mission does not require everything destroyed,
//;;   * `true` mission requires total map annihilation.
//;; * `eliminateBases` Instant win when all enemy units and bases are destroyed:
//;;   * `false` does not require all bases to be destroyed,
//;;   * `true` requires all bases destroyed.
//;;
//;; @param {string} kind
//;; @param {string} nextLevel
//;; @param {Object} data
//;; @returns {void}
//;;
function camSetStandardWinLossConditions(kind, nextLevel, data)
{
	switch (kind)
	{
		case CAM_VICTORY_STANDARD:
			__camWinLossCallback = CAM_VICTORY_STANDARD;
			__camNeedBonusTime = true;
			__camDefeatOnTimeout = true;
			__camVictoryData = data;
			useSafetyTransport(false);
			break;
		case CAM_VICTORY_PRE_OFFWORLD:
			__camWinLossCallback = CAM_VICTORY_PRE_OFFWORLD;
			__camNeedBonusTime = false;
			__camDefeatOnTimeout = true;
			useSafetyTransport(false);
			break;
		case CAM_VICTORY_OFFWORLD:
			__camWinLossCallback = CAM_VICTORY_OFFWORLD;
			__camNeedBonusTime = true;
			__camDefeatOnTimeout = true;
			__camVictoryData = data;
			setReinforcementTime((__camNumTransporterExits > 0) ? __camVictoryData.reinforcements : -1);
			useSafetyTransport(false);
			break;
		case CAM_VICTORY_TIMEOUT:
			__camWinLossCallback = CAM_VICTORY_TIMEOUT;
			__camNeedBonusTime = false;
			__camDefeatOnTimeout = false;
			__camVictoryData = data;
			setReinforcementTime((__camVictoryData.reinforcements > -1) ? __camVictoryData.reinforcements : -1);
			useSafetyTransport(true);
			break;
		default:
			camDebug("Unknown standard victory condition", kind);
			break;
	}
	__camNextLevel = nextLevel;
	__camUpdateObjectives();
}

//;; ## camCheckExtraObjective()
//;;
//;; Checks for extra win conditions defined in level scripts being met, if any.
//;;
//;; @returns {boolean|undefined}
//;;
function camCheckExtraObjective()
{
	let extraObjMet = true;
	if (camDef(__camVictoryData) && camDef(__camVictoryData.callback))
	{
		const __RESULT = __camGlobalContext()[__camVictoryData.callback]();
		if (camDef(__RESULT))
		{
			if (!__RESULT)
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

//;; ## camSetExtraObjectiveMessage(message)
//;;
//;; Message(s) the mission script can set to further explain specific victory conditions: the mission's own objectives,
//;; listed before the library's (see `setObjective()`). Allows a single string or an array of strings. An item can
//;; also be an object `{id, text, marker}`, where `id` names it for `camSetObjectiveState()`, and `marker` names the
//;; proximity message that shows where it's done.
//;;
//;; @param {string|Object|Object[]} message
//;; @returns {void}
//;;
function camSetExtraObjectiveMessage(message)
{
	__camExtraObjectiveMessage = message;
	__camUpdateObjectives();
}

//;; ## camSetObjectiveState(id, state)
//;;
//;; Marks the mission's objective `id` (see `camSetExtraObjectiveMessage()`) "done", "failed" or "active" again, when the
//;; mission script knows it's met (or lost). Objectives start active.
//;;
//;; @param {string} id
//;; @param {string} state
//;; @returns {void}
//;;
function camSetObjectiveState(id, state)
{
	if (!camDef(__camObjectiveStates))
	{
		__camObjectiveStates = {}; // (for saves from before objectives existed)
	}
	if (__camObjectiveStates[id] === state)
	{
		return;
	}
	__camObjectiveStates[id] = state;
	__camUpdateObjectives();
}

//;; ## camClearConsoleOnVictoryMessage(clear)
//;;
//;; If the script wants to allow `__camSetupConsoleForVictoryConditions()` to clear the console.
//;; (No longer used: the victory conditions are objectives, and aren't printed when they change.)
//;;
//;; @param {boolean} clear
//;; @returns {void}
//;;
function camClearConsoleOnVictoryMessage(clear)
{
	__camAllowVictoryMsgClear = clear;
}

//////////// privates

function __camGameLostCB()
{
	gameOverMessage(false, false);
}

function __camGameLost()
{
	if (__camLevelEnded)
	{
		// Prevent losing after winning if they player managed to precisely time
		// their win right around when the mission timer ran out.
		return;
	}
	camCallOnce("__camGameLostCB");
}

function __camGameWon()
{
	__camLevelEnded = true;
	__camUpdateObjectives(true); // all met, even if the last hasn't been noticed yet (ex. the units being back at the LZ)
	if (camDef(__camNextLevel))
	{
		camTrace(__camNextLevel);
		if (__camNextLevel === CAM_GAMMA_OUT)
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

//Checks if the player is considered to be alive on all non-offworld missions.
//NOTE: Do not count cyborg factory here since combat engineer does not exist
//in campaign at the moment.
function __camPlayerDead()
{
	let dead = true;
	const __HAVE_FACTORIES = enumStruct(CAM_HUMAN_PLAYER, FACTORY).filter((obj) => (
		obj.status === BUILT
	)).length > 0;
	if (__HAVE_FACTORIES)
	{
		dead = false;
	}
	if (__camNextLevel === cam_levels.gamma2.pre)
	{
		//Check for any construction units.
		//NOTE: countDroid() will return the counts of construction units in
		//apsLimboDroids between Gamma 3 to Gamma 5.
		if (countDroid(DROID_CONSTRUCT) > 0)
		{
			dead = false;
		}
	}
	else if (enumDroid(CAM_HUMAN_PLAYER, DROID_CONSTRUCT).length > 0)
	{
		//A construction unit is currently on the map.
		dead = false;
	}
	else if (__camNextLevel === cam_levels.gamma7)
	{
		const __GAMMA_PLAYER = 1;

		//Care about all units and not just trucks at the start of cam3-c.
		if (allianceExistsBetween(__GAMMA_PLAYER, CAM_NEXUS) && enumDroid(CAM_HUMAN_PLAYER).length > 0)
		{
			dead = false;
		}
	}
	else
	{
		//Check the transporter.
		const transporter = enumDroid(CAM_HUMAN_PLAYER, DROID_SUPERTRANSPORTER);
		if (transporter.length > 0)
		{
			const cargoDroids = enumCargo(transporter[0]);
			for (let i = 0, len = cargoDroids.length; i < len; ++i)
			{
				const virDroid = cargoDroids[i];
				if (camDef(virDroid) && virDroid && virDroid.droidType === DROID_CONSTRUCT)
				{
					dead = false;
					break;
				}
			}
		}
	}
	if (__camWinLossCallback === CAM_VICTORY_TIMEOUT)
	{
		//Make the mission fail if no units are alive on map while having no factories.
		let droidCount = 0;
		enumDroid(CAM_HUMAN_PLAYER).forEach((obj) => {
			if (obj.droidType === DROID_SUPERTRANSPORTER)
			{
				//Don't count the transporter itself. This is for the case where
				//they have no units and no factories and have the transporter
				//sitting at base unable to launch.
				droidCount += enumCargo(obj).length;
			}
			else
			{
				droidCount += 1;
			}
		});
		dead = droidCount <= 0 && !__HAVE_FACTORIES;
		//Finish Beta-end early if they have no units and factories on lower difficulties.
		if (dead && (difficulty <= HARD) && (__camNextLevel === cam_levels.gamma1))
		{
			cam_eventMissionTimeout(); //Early victory trigger
			return false;
		}
	}
	return dead;
}

function __camTriggerLastAttack()
{
	if (!__camLastAttackTriggered)
	{
		// Do not order systems (sensor/trucks/repairs) to attack stuff.
		const enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).filter((obj) => (
			obj.type === DROID && !camIsTransporter(obj) && !camIsSystemDroid(obj)
		));
		camTrace(enemies.length, "enemy droids remaining");
		camManageGroup(camMakeGroup(enemies), CAM_ORDER_ATTACK);
		__camLastAttackTriggered = true;
	}
}

function __camVictoryStandard()
{
	const __EXTRA_OBJ = camCheckExtraObjective();
	// check if game is lost
	if (__camPlayerDead())
	{
		__camGameLost();
		return;
	}
	// check if game is won
	if (camAllArtifactsPickedUp() && camAllEnemyBasesEliminated() && __EXTRA_OBJ)
	{
		if (enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).length === 0)
		{
			__camGameWon();
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
	}
	// victory hooked from eventTransporterExit
}

function __camVictoryTimeout()
{
	if (__camPlayerDead())
	{
		__camGameLost();
	}
	// victory hooked from eventMissionTimeout
}

function __camVictoryOffworld()
{
	const lz = __camVictoryData.area;
	if (!camDef(lz))
	{
		camDebug("Landing zone area is required for OFFWORLD");
		return;
	}
	const __TOTAL = countDroid(DROID_ANY, CAM_HUMAN_PLAYER); // for future use
	if (__TOTAL === 0)
	{
		__camGameLost();
		return;
	}
	const __FORCE_LZ = camDef(__camVictoryData.retlz) ? __camVictoryData.retlz : false;
	const __DESTROY_ALL = camDef(__camVictoryData.annihilate) ? __camVictoryData.annihilate : false;
	const __ELIM_BASES = camDef(__camVictoryData.eliminateBases) ? __camVictoryData.eliminateBases : false;
	if (camCheckExtraObjective() && camAllArtifactsPickedUp())
	{
		if (__ELIM_BASES)
		{
			if (camAllEnemyBasesEliminated())
			{
				const enemyDroids = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).filter((obj) => (
					obj.type === DROID
				)).length;
				if (!enemyDroids)
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
		else
		{
			const __ENEMY_LEN = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).length;
			if (!__FORCE_LZ && !__ENEMY_LEN)
			{
				//if there are no more enemies, win instantly unless forced to go
				//back to the LZ.
				__camGameWon();
				return;
			}
			//Missions that are not won based on artifact count (see missions 2-1 and 3-2).
			//If either __FORCE_LZ or __DESTROY_ALL is true then ignore this.
			if (__camNumArtifacts === 0 && !__FORCE_LZ && !__DESTROY_ALL)
			{
				__camGameWon();
				return;
			}
			//Make sure to only count droids here.
			const __TOTAL_AT_LZ = enumArea(lz, CAM_HUMAN_PLAYER, false).filter((obj) => (
				obj.type === DROID && !camIsTransporter(obj)
			)).length;
			if (((!__FORCE_LZ && !__DESTROY_ALL) || (__FORCE_LZ && __DESTROY_ALL && !__ENEMY_LEN) || (__FORCE_LZ && !__DESTROY_ALL)) && (__TOTAL_AT_LZ === __TOTAL))
			{
				__camGameWon();
				return;
			}
			else
			{
				__camTriggerLastAttack();
			}
			if (!__DESTROY_ALL || (__FORCE_LZ && !__ENEMY_LEN))
			{
				const __REMIND_RETURN = 30; // every X seconds
				if (__camRTLZTicker === 0 && camDef(__camVictoryData.message))
				{
					camTrace("Return to LZ message displayed");
					camMarkTiles(lz);
					if (camDef(__camVictoryData.message))
					{
						hackAddMessage(__camVictoryData.message, PROX_MSG, CAM_HUMAN_PLAYER, false);
					}
				}
				if (__camRTLZTicker % __REMIND_RETURN === 0)
				{
					const __USED_REMIND = camDef(__camVictoryData.playLzReminder);
					if (!__USED_REMIND || (__USED_REMIND && __camVictoryData.playLzReminder))
					{
						const pos = camMakePos(lz);
						playSound(cam_sounds.lz.returnToLZ, pos.x, pos.y, 0);
					}
				}
				++__camRTLZTicker;
				if (__camRTLZTicker === 1)
				{
					__camUpdateObjectives(); // ("Return to LZ")
				}
			}
		}
	}
	if (enumArea(lz, ENEMIES, false).length > 0)
	{
		const __REMIND_COMPROMISED = 30; // every X seconds
		//Protect against early access to reinforcements GUI if it shouldn't be available yet
		if (__camVictoryData.reinforcements >= 0)
		{
			setReinforcementTime(LZ_COMPROMISED_TIME, false);
		}
		if (__camLZCompromisedTicker === 0)
		{
			camTrace("LZ compromised");
		}
		if (__camLZCompromisedTicker % __REMIND_COMPROMISED === 1)
		{
			const pos = camMakePos(lz);
			playSound(cam_sounds.lz.LZCompromised, pos.x, pos.y, 0);
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
		const pos = camMakePos(lz);
		playSound(cam_sounds.lz.LZClear, pos.x, pos.y, 0);
		setReinforcementTime(__camVictoryData.reinforcements, false);
		__camLZCompromisedTicker = 0;
		if (__camRTLZTicker === 0)
		{
			camUnmarkTiles(lz);
		}
	}
}

function __camSetupConsoleForVictoryConditions()
{
	// An artifact was collected or a base destroyed: update the objectives' counts
	__camUpdateObjectives();
}

function __camShowBetaHint()
{
	return ((camDiscoverCampaign() === __CAM_BETA_CAMPAIGN_NUMBER) && (difficulty >= INSANE));
}

function __camShowBetaHintEarly()
{
	if (!camDef(__camWinLossCallback) || (__camWinLossCallback !== CAM_VICTORY_PRE_OFFWORLD))
	{
		return;
	}
	if (__camShowBetaHint())
	{
		__camShowVictoryConditions();
	}
}

function __camShowBetaHintMessage()
{
	console(_("Insane difficulty hint:"));
	console(_("Fortify a strong base across the map to protect yourself from the Collective"));
}

// The mission's own objectives (camSetExtraObjectiveMessage()), as {id, text, marker} items
function __camExtraObjectiveItems()
{
	if (!camDef(__camExtraObjectiveMessage))
	{
		return [];
	}
	const messages = (__camExtraObjectiveMessage instanceof Array) ? __camExtraObjectiveMessage : [__camExtraObjectiveMessage];
	return messages.map((item) => ((typeof item === "string") ? {text: item} : item));
}

// A detected enemy base still standing, where "Show on map" goes
function __camEnemyBaseMarker()
{
	for (const baseLabel in __camEnemyBases)
	{
		const bi = __camEnemyBases[baseLabel];
		if (!bi.eliminated && bi.detected && camDef(bi.detectMsg))
		{
			return bi.detectMsg;
		}
	}
	return undefined;
}

// Sets the victory conditions as the player's objectives (see setObjective()): the mission's own, the artifacts and
// bases to count, what to destroy, surviving the timer, and returning to the LZ when needed. Called whenever any of
// these change. The engine shows them, or prints the changes to the console when the HUD doesn't. When the mission is
// `won`, its primary objectives are all marked done.
function __camUpdateObjectives(won)
{
	if (!camDef(__camNextLevel) || !camDef(__camWinLossCallback))
	{
		return; // fastplay / tutorial
	}
	if (__camLevelEnded && won !== true)
	{
		return; // the level is over: keep the objectives as they were at its end
	}
	const __KIND = __camWinLossCallback;
	const __DATA = camDef(__camVictoryData) ? __camVictoryData : {};
	const __REQUIRED = (__KIND === CAM_VICTORY_STANDARD || __KIND === CAM_VICTORY_OFFWORLD);
	const objectives = [];
	__camExtraObjectiveItems().forEach((item, index) => {
		const options = {order: 0};
		if (camDef(item.marker))
		{
			options.marker = item.marker;
		}
		if (camDef(item.id) && camDef(__camObjectiveStates) && camDef(__camObjectiveStates[item.id]))
		{
			options.state = __camObjectiveStates[item.id];
		}
		objectives.push(["cam_extra_" + (camDef(item.id) ? item.id : index), item.text, options]);
	});
	const __ARTIFACTS = Object.keys(__camArtifacts).length;
	if (__ARTIFACTS > 0)
	{
		objectives.push(["cam_artifacts", _("Artifacts collected"), {
			order: 1,
			primary: __REQUIRED,
			progress: [__camNumArtifacts, __ARTIFACTS],
			state: camAllArtifactsPickedUp() ? "done" : "active"
		}]);
	}
	const __BASES = Object.keys(__camEnemyBases).length;
	if (__BASES > 0)
	{
		const options = {
			order: 2,
			primary: (__KIND === CAM_VICTORY_STANDARD) || (__KIND === CAM_VICTORY_OFFWORLD && __DATA.eliminateBases === true),
			progress: [__camNumEnemyBases, __BASES],
			state: camAllEnemyBasesEliminated() ? "done" : "active"
		};
		const __MARKER = __camEnemyBaseMarker();
		if (camDef(__MARKER))
		{
			options.marker = __MARKER;
		}
		objectives.push(["cam_bases", _("Bases destroyed"), options]);
	}
	if (__KIND === CAM_VICTORY_STANDARD || (__KIND === CAM_VICTORY_OFFWORLD && __DATA.annihilate === true))
	{
		objectives.push(["cam_destroy", _("Destroy all enemy units and structures"), {order: 3}]);
	}
	else if (__KIND === CAM_VICTORY_OFFWORLD && __DATA.eliminateBases === true)
	{
		objectives.push(["cam_destroy", _("Destroy all enemy units and bases"), {order: 3}]);
	}
	else if (__KIND === CAM_VICTORY_TIMEOUT)
	{
		objectives.push(["cam_survive", _("Survive until the timer reaches zero"), {order: 3}]);
	}
	// Returning to the LZ: from the start when the mission requires it, otherwise once the rest is done
	if (__KIND === CAM_VICTORY_OFFWORLD && (__DATA.retlz === true || __camRTLZTicker > 0))
	{
		const options = {order: 4};
		if (camDef(__DATA.message))
		{
			options.marker = __DATA.message;
		}
		objectives.push(["cam_lz", _("Return to LZ"), options]);
	}
	if (won === true)
	{
		// All met, even if the last hasn't been noticed yet. One whose count is still short (ex. when cheating) stays undone
		objectives.forEach((objective) => {
			const options = objective[2];
			const __SHORT = camDef(options.progress) && options.progress[0] < options.progress[1];
			if (options.primary !== false && options.state !== "failed" && !__SHORT)
			{
				options.state = "done";
			}
		});
	}
	__camSetObjectives(objectives);
}

function __camSetObjectives(objectives)
{
	if (!camDef(__camObjectiveIds))
	{
		__camObjectiveIds = []; // (for saves from before objectives existed)
	}
	const __IDS = objectives.map((objective) => (objective[0]));
	__camObjectiveIds.forEach((id) => {
		if (!__IDS.includes(id))
		{
			removeObjective(id);
		}
	});
	objectives.forEach((objective) => {
		setObjective(objective[0], objective[1], objective[2]);
	});
	__camObjectiveIds = __IDS;
}

// Every few minutes (the Victory Hints option): prints the objectives to the console again, as they scroll away there
function __camShowVictoryConditions()
{
	if (camDef(tweakOptions.victoryHints) && !tweakOptions.victoryHints)
	{
		return; // Disabled.
	}
	if (!camDef(__camNextLevel))
	{
		return; // fastplay / tutorial. Should be a better identifier for this.
	}
	if (__camWinLossCallback === CAM_VICTORY_PRE_OFFWORLD)
	{
		if (__camShowBetaHint())
		{
			__camShowBetaHintMessage();
		}
		return; // do not need this on these missions.
	}
	__camUpdateObjectives();
	showObjectives(CAM_HUMAN_PLAYER, true);
}

// The "win info" chat command: the objectives, and what's left of the enemy
function __camShowVictoryInfo()
{
	if (!camDef(__camNextLevel) || __camWinLossCallback === CAM_VICTORY_PRE_OFFWORLD)
	{
		return;
	}
	let unitsOnMap = 0;
	let structuresOnMap = 0;
	enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).forEach((obj) => {
		if (obj.type === DROID)
		{
			++unitsOnMap;
		}
		else if (obj.type === STRUCTURE)
		{
			++structuresOnMap;
		}
	});
	console(unitsOnMap + " " + _("Enemy units remaining"));
	console(structuresOnMap + " " + _("Enemy structures remaining"));
	__camUpdateObjectives();
	showObjectives(CAM_HUMAN_PLAYER);
}
