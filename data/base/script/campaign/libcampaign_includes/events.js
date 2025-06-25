
////////////////////////////////////////////////////////////////////////////////
// Campaign library events.
////////////////////////////////////////////////////////////////////////////////

function cam_eventPickup(feature, droid)
{
	if (feature.stattype === ARTIFACT)
	{
		__camPickupArtifact(feature);
	}
}

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
	if (message === "win info")
	{
		__camShowVictoryConditions();
	}
	if (message.lastIndexOf("rank ", 0) === 0)
	{
		camSetExpLevel(Number(message.substring(5)));
		camSetOnMapEnemyUnitExp();
	}
	if (message.lastIndexOf("prop ", 0) === 0)
	{
		camSetPropulsionTypeLimit(Number(message.substring(5)));
	}
	if (!camIsCheating())
	{
		return;
	}
	camTrace(from, to, message);
	if (message === "let me win" && __camNextLevel !== cam_levels.alpha3.offWorld)
	{
		__camLetMeWin();
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
		for (const baseLabel in __camEnemyBases)
		{
			camDetectEnemyBase(baseLabel);
		}
	}
	if (message === "research available")
	{
		while (true) // eslint-disable-line no-constant-condition
		{
			const research = enumResearch();
			if (research.length === 0)
			{
				break;
			}
			for (let i = 0, len = research.length; i < len; ++i)
			{
				const __RESEARCH_NAME = research[i].name;
				completeResearch(__RESEARCH_NAME, CAM_HUMAN_PLAYER);
			}
		}
	}
}

function cam_eventStartLevel()
{
	receiveAllEvents(true);
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
	__camVtolDataSystem = [];
	__camLastNexusAttack = 0;
	__camNexusActivated = false;
	__camNewGroupCounter = 0;
	__camVideoSequences = [];
	__camSaveLoading = false;
	__camNeverGroupDroids = [];
	__camNumTransporterExits = 0;
	__camAllowVictoryMsgClear = true;
	__camExpLevel = 0;
	camSetPropulsionTypeLimit(); //disable the propulsion changer by default
	__camAiPowerReset(); //grant power to the AI
	setTimer("__camSpawnVtols", camSecondsToMilliseconds(0.5));
	setTimer("__camRetreatVtols", camSecondsToMilliseconds(0.9));
	setTimer("__checkVtolSpawnObject", camSecondsToMilliseconds(5));
	setTimer("__checkEnemyFactoryProductionTick", camSecondsToMilliseconds(0.8));
	setTimer("__camTick", camSecondsToMilliseconds(1)); // campaign pollers
	setTimer("__camTruckTick", camSecondsToMilliseconds(10) + camSecondsToMilliseconds(0.1)); // some slower campaign pollers
	setTimer("__camAiPowerReset", camMinutesToMilliseconds(3)); //reset AI power every so often
	setTimer("__camShowVictoryConditions", camMinutesToMilliseconds(5));
	setTimer("__camTacticsTick", camSecondsToMilliseconds(0.1));
	queue("__camShowBetaHintEarly", camSecondsToMilliseconds(4));
	queue("__camGrantSpecialResearch", camSecondsToMilliseconds(0.5));
	queue("__camEnableGuideTopics", camSecondsToMilliseconds(0.1)); // delayed to handle when mission scripts add research
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
	if (camGetNexusState() && droid.player === CAM_NEXUS && __camNextLevel === cam_levels.gamma6 && camRand(100) < 7)
	{
		// Occasionally hint that NEXUS is producing units on Gamma 5.
		playSound(cam_sounds.nexus.productionCompleted);
	}
	if (droid.player === CAM_HUMAN_PLAYER)
	{
		// handling guide topics for built units
		if (droid.isVTOL)
		{
			camCallOnce("__camDoAddVTOLUseTopics");
		}
		else if (droid.droidType === DROID_COMMAND)
		{
			camCallOnce("__camDoAddCommanderUseTopics");
		}
	}
	if (!camDef(__camFactoryInfo))
	{
		return;
	}
	camSetDroidExperience(droid);
	__camAddDroidToFactoryGroup(droid, structure);
}

function cam_eventDestroyed(obj)
{
	if (obj.type === FEATURE && obj.stattype === ARTIFACT)
	{
		return;
	}
	__camCheckPlaceArtifact(obj);
	if (obj.type === DROID)
	{
		if (obj.droidType === DROID_CONSTRUCT)
		{
			__camCheckDeadTruck(obj);
		}
		else if (camIsTransporter(obj))
		{
			__camRemoveIncomingTransporter(obj.player);
			if (obj.player === CAM_HUMAN_PLAYER)
			{
				// Player will lose if their transporter gets destroyed
				__camGameLost();
				return;
			}
			if (camDef(__camPlayerTransports[obj.player]))
			{
				delete __camPlayerTransports[obj.player];
			}
		}
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
	if (transport.player === CAM_HUMAN_PLAYER)
	{
		__camNumTransporterExits += 1;
		//Audio cue to let the player know they can bring in reinforcements. This
		//assumes the player can bring in reinforcements immediately after the first
		//transporter leaves the map. Mission scripts can handle special situations.
		if (__camNumTransporterExits === 1 &&
			((__camWinLossCallback === CAM_VICTORY_OFFWORLD &&
			__camVictoryData.reinforcements > -1) ||
			__camWinLossCallback === CAM_VICTORY_STANDARD))
		{
			playSound(cam_sounds.reinforcementsAreAvailable);
			//Show the transporter reinforcement timer when it leaves for the first time.
			if (__camWinLossCallback === CAM_VICTORY_OFFWORLD)
			{
				setReinforcementTime(__camVictoryData.reinforcements);
			}
		}
		// Show how long until the transporter comes back on Beta End.
		if (__camWinLossCallback === CAM_VICTORY_TIMEOUT)
		{
			setReinforcementTime(__camVictoryData.reinforcements);
		}
	}
	if (transport.player !== CAM_HUMAN_PLAYER ||
		(__camWinLossCallback === CAM_VICTORY_STANDARD &&
		transport.player === CAM_HUMAN_PLAYER))
	{
		__camRemoveIncomingTransporter(transport.player);
	}
	else if (__camWinLossCallback === CAM_VICTORY_PRE_OFFWORLD)
	{
		camTrace("Transporter is away.");
		__camGameWon();
	}
}

function cam_eventTransporterLanded(transport)
{
	if (transport.player !== CAM_HUMAN_PLAYER)
	{
		__camLandTransporter(transport.player, camMakePos(transport));
	}
	else
	{
		// Make the transporter timer on Beta End disappear, since the transporter has arrived.
		if (__camWinLossCallback === CAM_VICTORY_TIMEOUT)
		{
			setReinforcementTime(-1);
		}
		// Handle enabling guide topics relevant to units potentially "gifted" by libcampaign
		__camEnableGuideTopicsForTransport(transport);
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
		const __WON = camCheckExtraObjective();
		if (!__WON)
		{
			__camGameLost();
			return;
		}
		__camGameWon();
	}
}

function cam_eventAttacked(victim, attacker)
{
	if (camDef(victim) && victim)
	{
		if (victim.type === DROID && victim.player !== CAM_HUMAN_PLAYER && !allianceExistsBetween(CAM_HUMAN_PLAYER, victim.player))
		{
			//Try dynamically creating a group of nearby droids not part
			//of a group. Only supports those who can hit ground units.
			if (victim.group === null)
			{
				const __DEFAULT_RADIUS = 6;
				const loc = {x: victim.x, y: victim.y};
				const droids = enumRange(loc.x, loc.y, __DEFAULT_RADIUS, victim.player, false).filter((obj) => (
					obj.type === DROID &&
					obj.group === null &&
					(obj.canHitGround || obj.isSensor) &&
					obj.droidType !== DROID_CONSTRUCT &&
					!camIsTransporter(obj) &&
					!camInNeverGroup(obj)
				));
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
				if (__camGroupInfo[victim.group].order === CAM_ORDER_PATROL)
				{
					if (camDef(__camGroupInfo[victim.group].data) &&
						camDef(__camGroupInfo[victim.group].data.reactToAttack) &&
						__camGroupInfo[victim.group].data.reactToAttack)
					{
						__camGroupInfo[victim.group].order = CAM_ORDER_ATTACK;
					}
				}
			}
		}
		if (victim.player === CAM_HUMAN_PLAYER && camDef(attacker) && attacker && attacker.player !== CAM_HUMAN_PLAYER)
		{
			if (attacker.type === DROID && attacker.isVTOL)
			{
				camCallOnce("__camDoAddVTOLDefenseTopics");
			}
		}
	}
}

//Work around some things that break on save-load.
function cam_eventGameLoaded()
{
	receiveAllEvents(true);
	__camSaveLoading = true;
	const scavKevlarMissions = [
		cam_levels.alpha7, cam_levels.alpha8.pre, cam_levels.alpha8.offWorld,
		cam_levels.alpha9.pre, cam_levels.alpha9.offWorld, cam_levels.alpha10,
		cam_levels.alpha11.pre, cam_levels.alpha11.offWorld, cam_levels.alpha12.pre,
		cam_levels.alphaEnd, cam_levels.beta6.pre
	];
	//Need to set the scavenger kevlar vests when loading a save from later Alpha
	//missions or else it reverts to the original texture.
	for (let i = 0, l = scavKevlarMissions.length; i < l; ++i)
	{
		const __MISSION = scavKevlarMissions[i];
		if (__camNextLevel === __MISSION)
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
	if (__camWinLossCallback === CAM_VICTORY_TIMEOUT &&
		enumDroid(CAM_HUMAN_PLAYER, DROID_SUPERTRANSPORTER).length === 0)
	{
		// If the transport is gone on Beta End, put a timer up to show when it'll be back
		setReinforcementTime(__camVictoryData.reinforcements);
	}
	//Subscribe to eventGroupSeen again.
	camSetEnemyBases();
	// Ensure appropriate guide topics are displayed
	__camEnableGuideTopics();
	//Reset any vars
	__camCheatMode = false;
	__camSaveLoading = false;
}

//Plays Nexus sounds if nexusActivated is true.
function cam_eventObjectTransfer(obj, from)
{
	if (camGetNexusState() && from === CAM_HUMAN_PLAYER && obj.player === CAM_NEXUS)
	{
		let snd;
		if (obj.type === STRUCTURE)
		{
			if (obj.stattype === DEFENSE)
			{
				snd = cam_sounds.nexus.defensesAbsorbed;
			}
			else if (obj.stattype === RESEARCH_LAB)
			{
				snd = cam_sounds.nexus.researchAbsorbed;
			}
			else
			{
				snd = cam_sounds.nexus.structureAbsorbed;
			}
		}
		else if (obj.type === DROID)
		{
			snd = cam_sounds.nexus.unitAbsorbed;
		}
		if (camDef(snd))
		{
			playSound(snd);
		}
		queue("camNexusLaugh", camSecondsToMilliseconds(1.5));
	}
}

function cam_eventVideoDone()
{
	__camEnqueueVideos(); //Play any remaining videos automatically.
}

function cam_eventDroidRankGained(droid, rankNum)
{
	if (droid.player === CAM_HUMAN_PLAYER)
	{
		addGuideTopic("wz2100::units::experience", SHOWTOPIC_FIRSTADD);
	}
}

function cam_eventResearched(research, structure, player)
{
	if (player !== CAM_HUMAN_PLAYER)
	{
		return;
	}
	const __RESEARCHED_BY_STRUCT = (camDef(structure) && structure);
	if (!__RESEARCHED_BY_STRUCT)
	{
		return; // for now, return - don't think we need to process if researched by API call here?
	}
	// only pass the research in if it was completed by a structure (not if given by an API call, in which structure would be null)
	//__camProcessResearchGatedGuideTopics((__RESEARCHED_BY_STRUCT) ? research : null);
	__camProcessResearchGatedGuideTopics(research);
}
