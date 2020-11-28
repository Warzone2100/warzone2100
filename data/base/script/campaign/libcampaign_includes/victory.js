
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
			setReinforcementTime(__camVictoryData.reinforcements);
			__camSetOffworldLimits();
			useSafetyTransport(false);
			break;
		case CAM_VICTORY_TIMEOUT:
			__camWinLossCallback = CAM_VICTORY_TIMEOUT;
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

//Message(s) the mission script can set to further explain specific victory conditions.
//Allows a single string or an array of strings.
function camSetExtraObjectiveMessage(message)
{
	__camExtraObjectiveMessage = message;
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

//Checks if the player is considered to be alive on all non-offworld missions.
//NOTE: Do not count cyborg factory here since combat engineer does not exist
//in campaign at the moment.
function __camPlayerDead()
{
	var dead = true;
	var haveFactories = enumStruct(CAM_HUMAN_PLAYER, FACTORY).filter(function(obj) {
		return obj.status === BUILT;
	}).length > 0;

	if (haveFactories)
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
		const GAMMA_PLAYER = 1;

		//Care about all units and not just trucks at the start of cam3-c.
		if (allianceExistsBetween(GAMMA_PLAYER, NEXUS) && enumDroid(CAM_HUMAN_PLAYER).length > 0)
		{
			dead = false;
		}
	}
	else
	{
		//Check the transporter.
		var transporter = enumDroid(CAM_HUMAN_PLAYER, DROID_SUPERTRANSPORTER);
		if (transporter.length > 0)
		{
			var cargoDroids = enumCargo(transporter[0]);
			for (var i = 0, len = cargoDroids.length; i < len; ++i)
			{
				var virDroid = cargoDroids[i];
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
		var droidCount = 0;
		enumDroid(CAM_HUMAN_PLAYER).forEach(function(obj) {
			droidCount += 1;
			if (obj.droidType === DROID_SUPERTRANSPORTER)
			{
				droidCount += enumCargo(obj).length;
			}
		});
		dead = droidCount <= 0 && !haveFactories;
	}

	return dead;
}

function __camTriggerLastAttack()
{
	if (!__camLastAttackTriggered)
	{
		var enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false);
		// Do not order systems (sensor/trucks/repairs) to attack stuff.
		enemies = enemies.filter(function(obj) {
			return ((obj.type === DROID) && !camIsTransporter(obj) && !camIsSystemDroid(obj));
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
			if (camAllEnemyBasesEliminated())
			{
				var enemyDroids = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).filter(function(obj) {
					return obj.type === DROID;
				}).length;


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
			if (__camNumArtifacts === 0 && !forceLZ && !destroyAll)
			{
				__camGameWon();
				return;
			}

			//Make sure to only count droids here.
			var atlz = enumArea(lz, CAM_HUMAN_PLAYER, false).filter(function(obj) {
				return (obj.type === DROID && !camIsTransporter(obj));
			}).length;
			if (((!forceLZ && !destroyAll) || (forceLZ && destroyAll && !enemyLen) || (forceLZ && !destroyAll)) && (atlz === total))
			{
				__camGameWon();
				return;
			}
			else
			{
				__camTriggerLastAttack();
			}

			if (!destroyAll || (forceLZ && !enemyLen))
			{
				const REMIND_RETURN = 30; // every X seconds
				if (__camRTLZTicker === 0 && camDef(__camVictoryData.message))
				{
					camTrace("Return to LZ message displayed");
					camMarkTiles(lz);
					if (camDef(__camVictoryData.message))
					{
						hackAddMessage(__camVictoryData.message, PROX_MSG, CAM_HUMAN_PLAYER, false);
					}
				}
				if (__camRTLZTicker % REMIND_RETURN === 0)
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
		const REMIND_COMPROMISED = 30; // every X seconds
		if (__camLZCompromisedTicker === 0)
		{
			camTrace("LZ compromised");
		}
		if (__camLZCompromisedTicker % REMIND_COMPROMISED === 1)
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

function __camShowVictoryConditions(forceMessage)
{
	if (!camDef(forceMessage))
	{
		if (__camVictoryMessageThrottle + camSecondsToMilliseconds(10) > gameTime)
		{
			return;
		}
		if (!camDef(__camNextLevel))
		{
			return; // fastplay / tutorial. Should be a better identifier for this.
		}
		if (__camWinLossCallback === CAM_VICTORY_PRE_OFFWORLD)
		{
			return; // do not need this on these missions.
		}
	}

	const ANNIHILATE_MESSAGE = _("Destroy all enemy units and structures");

	var unitsOnMap = 0;
	var structuresOnMap = 0;

	enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).forEach(function(obj) {
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
			console(ANNIHILATE_MESSAGE);
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
		console(ANNIHILATE_MESSAGE);
	}

	//More specific messages set through the mission scripts.
	if (camDef(__camExtraObjectiveMessage))
	{
		if (__camExtraObjectiveMessage instanceof Array)
		{
			for (var i = 0, len = __camExtraObjectiveMessage.length; i < len; ++i)
			{
				var mes = __camExtraObjectiveMessage[i];
				console(mes);
			}
		}
		else
		{
			console(__camExtraObjectiveMessage);
		}
	}

	__camVictoryMessageThrottle = gameTime;
}
