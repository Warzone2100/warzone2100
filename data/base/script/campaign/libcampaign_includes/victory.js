
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
	if (__camNeedBonusTime)
	{
		const __POWER_TIME_REMAINING = getMissionTime();
		if (__POWER_TIME_REMAINING > 0)
		{
			let bonus = 110;
			if (difficulty === HARD)
			{
				bonus = 90; //Small bonus compared to the regular Hard extraction value.
			}
			else if (difficulty === INSANE)
			{
				bonus = 70; //Same value for oil extraction.
			}
			camTrace("Bonus time", __POWER_TIME_REMAINING);
			setPowerModifier(bonus); // Bonus percentage for completing fast
			extraPowerTime(__POWER_TIME_REMAINING);
			setPowerModifier(100);
		}
	}
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
//;;   * `reinforcements` Reinforcements interval, in seconds.
//;; For standard and offworld victory, some extra data parameters can be defined:
//;; * `callback` A function callback to check for extra win/loss conditions. Return values are interpreted as follows:
//;;   * `false` means instant defeat ("objective failed"),
//;;   * `true` means victory as long as other standard victory conditions are met,
//;;   * `undefined` means suppress other victory checks ("clearly not won yet").
//;; * `victoryVideo` Pass in the name of a video string here and it will be played before attempting to load the next level.
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
//;; Message(s) the mission script can set to further explain specific victory conditions.
//;; Allows a single string or an array of strings.
//;;
//;; @param {string|Object[]} message
//;; @returns {void}
//;;
function camSetExtraObjectiveMessage(message)
{
	__camExtraObjectiveMessage = message;
}

//;; ## camClearConsoleOnVictoryMessage(clear)
//;;
//;; If the script wants to allow `__camSetupConsoleForVictoryConditions()` to clear the console.
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

	if (__camNextLevel === "SUB_3_1S")
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
	else if (__camNextLevel === "CAM3A-D1")
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

		//Finish Beta-end early if they have no units and factories on Easy/Normal.
		if (dead && (difficulty <= MEDIUM) && (__camNextLevel === "CAM_3A"))
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
					const pos = camMakePos(lz);
					playSound(cam_sounds.lz.returnToLZ, pos.x, pos.y, 0);
					console(_("Return to LZ"));
				}
				++__camRTLZTicker;
			}
		}
	}
	if (enumArea(lz, ENEMIES, false).length > 0)
	{
		const __REMIND_COMPROMISED = 30; // every X seconds
		//Protect against early access to reinforcements GUI if it shouldn't be available yet
		if (__camVictoryData.reinforcements >= 0)
		{
			setReinforcementTime(LZ_COMPROMISED_TIME);
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
		setReinforcementTime(__camVictoryData.reinforcements);
		__camLZCompromisedTicker = 0;
		if (__camRTLZTicker === 0)
		{
			camUnmarkTiles(lz);
		}
	}
}

function __camSetupConsoleForVictoryConditions()
{
	// Console clears are only done when collecting artifacts or destroying bases.
	if (__camAllowVictoryMsgClear)
	{
		clearConsole();
	}

	queue("__camShowVictoryConditions", camSecondsToMilliseconds(0.5));
}

function __camShowBetaHint()
{
	return ((camDiscoverCampaign() === __CAM_BETA_CAMPAIGN_NUMBER) && (difficulty === HARD || difficulty === INSANE));
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

function __camShowVictoryConditions()
{
	if (!camDef(__camNextLevel))
	{
		return; // fastplay / tutorial. Should be a better identifier for this.
	}

	if (__camWinLossCallback === CAM_VICTORY_PRE_OFFWORLD)
	{
		if (__camShowBetaHint())
		{
			console(_("Hard / Insane difficulty hint:"));
			console(_("Fortify a strong base across the map to protect yourself from the Collective"));
		}
		return; // do not need this on these missions.
	}

	const __ANNIHILATE_MESSAGE = _("Destroy all enemy units and structures");

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

	console(__camNumArtifacts + "/" + Object.keys(__camArtifacts).length + " " + _("Artifacts collected"));
	console(__camNumEnemyBases + "/" + Object.keys(__camEnemyBases).length + " " + _("Bases destroyed"));
	console(unitsOnMap + " " + _("Enemy units remaining"));
	console(structuresOnMap + " " + _("Enemy structures remaining"));

	if (__camWinLossCallback === CAM_VICTORY_OFFWORLD)
	{
		if (camDef(__camVictoryData.retlz) && __camVictoryData.retlz)
		{
			console(_("Return to LZ required"));
		}

		if (camDef(__camVictoryData.annihilate) && __camVictoryData.annihilate)
		{
			console(__ANNIHILATE_MESSAGE);
		}

		if (camDef(__camVictoryData.eliminateBases) && __camVictoryData.eliminateBases)
		{
			console(_("Destroy all enemy units and bases"));
		}
	}
	else if (__camWinLossCallback === CAM_VICTORY_TIMEOUT)
	{
		console(_("Survive until the timer reaches zero"));
	}
	else if (__camWinLossCallback === CAM_VICTORY_STANDARD)
	{
		console(__ANNIHILATE_MESSAGE);
	}

	//More specific messages set through the mission scripts.
	if (camDef(__camExtraObjectiveMessage))
	{
		if (__camExtraObjectiveMessage instanceof Array)
		{
			for (let i = 0, len = __camExtraObjectiveMessage.length; i < len; ++i)
			{
				const __MES = __camExtraObjectiveMessage[i];
				console(__MES);
			}
		}
		else
		{
			console(__camExtraObjectiveMessage);
		}
	}
}
