/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
/** @file
 *  All the C functions callable from the script code
 */

#ifndef __INCLUDED_SRC_SCRIPTFUNCS_H__
#define __INCLUDED_SRC_SCRIPTFUNCS_H__

#include "lib/framework/vector.h"
#include "messagedef.h"			//for VIEWDATA

// AI won't build there if there are more than
// MAX_BLOCKING_TILES on some location
#define MAX_BLOCKING_TILES		1

/// Forward declarations to allow pointers to these types
struct BASE_OBJECT;
struct DROID;

bool scriptInit();
void scriptSetStartPos(int position, int x, int	y);
void scriptSetDerrickPos(int x, int y);

bool scrGetPlayer();
bool scrGetDerrick();
bool scrGetDifficulty();
bool scrScavengersActive();
bool scrGetPlayerStartPosition();
bool scrSafeDest();
bool scrThreatAt();
Vector2i getPlayerStartPosition(int player);
bool scrSetSunPosition();
bool scrSetSunIntensity();

// not used in scripts, but used in code.
bool objectInRange(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range);

// Check for any player object being within a certain range of a position
bool scrObjectInRange();

// Check for a droid being within a certain range of a position
bool scrDroidInRange();

// Check for a struct being within a certain range of a position
bool scrStructInRange();

// return power of a player.
bool scrPlayerPower();

// Check for any player object being within a certain area
bool scrObjectInArea();

// Check for a droid being within a certain area
bool scrDroidInArea();

// Check for a struct being within a certain Area of a position
bool scrStructInArea();

// as above, but only visible structures.
bool scrSeenStructInArea();

// Check for a players structures but no walls being within a certain area
bool scrStructButNoWallsInArea();

// Count the number of player objects within a certain area
bool scrNumObjectsInArea();

// Count the number of player droids within a certain area
bool scrNumDroidsInArea();

// Count the number of player structures within a certain area
bool scrNumStructsInArea();

// Count the number of player structures but not walls within a certain area
bool scrNumStructsButNotWallsInArea();

// Count the number of structures in an area of a certain type
bool scrNumStructsByTypeInArea();

// Check for a droid having seen a certain object
bool scrDroidHasSeen();

// Enable a component to be researched
bool scrEnableComponent();

// Make a component available
bool scrMakeComponentAvailable();

//Enable a structure type to be built
bool	scrEnableStructure();

// true if structure is available.
bool scrIsStructureAvailable();

// Build a droid
bool scrAddDroid();

// Build a droid
bool scrAddDroidToMissionList();

//builds a droid in the specified factory//
bool scrBuildDroid();

//check for a building to have been destroyed
bool scrBuildingDestroyed();

// Add a reticule button to the interface
bool scrAddReticuleButton();

//Remove a reticule button from the interface
bool scrRemoveReticuleButton();

// add a message to the Intelligence Display
bool scrAddMessage();

// add a tutorial message to the Intelligence Display
//bool scrAddTutorialMessage();

//make the droid with the matching id the currently selected droid
bool scrSelectDroidByID();

// for a specified player, set the assembly point droids go to when built
bool scrSetAssemblyPoint();

// test for structure is idle or not
bool scrStructureIdle();

// sends a players droids to a location to attack
bool scrAttackLocation();

// enumerate features;
bool scrInitGetFeature();
bool scrGetFeature();
bool scrGetFeatureB();

//Add a feature
bool scrAddFeature();

//Destroy a feature
bool scrDestroyFeature();

//Add a structure
bool scrAddStructure();

//Destroy a structure
bool scrDestroyStructure();

// enumerate structures
bool scrInitEnumStruct();
bool scrEnumStruct();
bool scrInitEnumStructB();
bool scrEnumStructB();

/*looks to see if a structure (specified by type) exists */
bool scrStructureBeingBuilt();

/* almost the same as above, but only for a specific struct*/
// pc multiplayer only for now.
bool scrStructureComplete();

/*looks to see if a structure (specified by type) exists and built*/
bool scrStructureBuilt();

/*centre theview on an object - can be droid/structure or feature */
bool scrCentreView();

/*centre the view on a position */
bool scrCentreViewPos();

// Get a pointer to a structure based on a stat - returns NULL if cannot find one
bool scrGetStructure();

// Get a pointer to a template based on a component stat - returns NULL if cannot find one
bool scrGetTemplate();

// Get a pointer to a droid based on a component stat - returns NULL if cannot find one
bool scrGetDroid();

// Sets all the scroll params for the map
bool scrSetScrollParams();

// Sets the scroll minX separately for the map
bool scrSetScrollMinX();

// Sets the scroll minY separately for the map
bool scrSetScrollMinY();

// Sets the scroll maxX separately for the map
bool scrSetScrollMaxX();

// Sets the scroll maxY separately for the map
bool scrSetScrollMaxY();

// Sets which sensor will be used as the default for a player
bool scrSetDefaultSensor();

// Sets which ECM will be used as the default for a player
bool scrSetDefaultECM();

// Sets which RepairUnit will be used as the default for a player
bool scrSetDefaultRepair();

// Sets the structure limits for a player
bool scrSetStructureLimits();

// Sets all structure limits for a player to a specified value
bool scrSetAllStructureLimits();

//multiplayer limit handler
bool scrApplyLimitSet();

// plays a sound for the specified player - only plays the sound if the
//specified player = selectedPlayer
bool scrPlaySound();

// plays a sound for the specified player - only plays the sound if the
// specified player = selectedPlayer - saves position
bool scrPlaySoundPos();

/* add a text message tothe top of the screen for the selected player*/
bool scrAddConsoleText();

// same as above - but it doesn't clear what's there and isn't permanent
bool scrShowConsoleText();

/* Adds console text without clearing old */
bool scrTagConsoleText();

//demo functions for turning the power on
bool scrTurnPowerOff();

//demo functions for turning the power off
bool scrTurnPowerOn();

//flags when the tutorial is over so that console messages can be turned on again
bool scrTutorialEnd();

//function to play a full-screen video in the middle of the game for the selected player
bool scrPlayVideo();

//checks to see if there are any droids for the specified player
bool scrAnyDroidsLeft();

//checks to see if there are any structures (except walls) for the specified player
bool scrAnyStructButWallsLeft();

bool scrAnyFactoriesLeft();

//function to call when the game is over, plays a message.
bool scrGameOverMessage();

//function to call when the game is over
bool scrGameOver();

//defines the background audio to play
bool scrPlayBackgroundAudio();

// cd audio funcs
bool scrPlayIngameCDAudio();
bool scrStopCDAudio();
bool scrPauseCDAudio();
bool scrResumeCDAudio();

// set the retreat point for a player
bool scrSetRetreatPoint();

// set the retreat force level
bool scrSetRetreatForce();

// set the retreat leadership
bool scrSetRetreatLeadership();

// set the retreat point for a group
bool scrSetGroupRetreatPoint();

bool scrSetGroupRetreatForce();

// set the retreat leadership
bool scrSetGroupRetreatLeadership();

// set the retreat health level
bool scrSetRetreatHealth();
bool scrSetGroupRetreatHealth();

//start a Mission
bool scrStartMission();

//set Snow (enable disable snow)
bool scrSetSnow();

//set Rain (enable disable Rain)
bool scrSetRain();

//set Background Fog (replace fade out with fog)
bool scrSetBackgroundFog();

//set Depth Fog (gradual fog from mid range to edge of world)
bool scrSetDepthFog();

//set Mission Fog colour, may be modified by weather effects
bool scrSetFogColour();

// remove a message from the Intelligence Display
bool scrRemoveMessage();

// Pop up a message box with a number value in it
bool scrNumMB();

// Do an approximation to a square root
bool scrApproxRoot();

bool scrRefTest();

// is <player> human or a computer? (multiplayer)
bool	scrIsHumanPlayer();

// Set an alliance between two players
bool scrCreateAlliance();

bool scrOfferAlliance();

// Break an alliance between two players
bool scrBreakAlliance();

// push true if an alliance still exists.
bool scrAllianceExists();
bool scrAllianceExistsBetween();

// true if player is allied.
bool scrPlayerInAlliance();

// push true if group wins are allowed.
//bool scrAllianceState();

// push true if a single alliance is dominant.
bool scrDominatingAlliance();

// push true if human player is responsible for 'player'
bool scrMyResponsibility();

/*checks to see if a structure of the type specified exists within the
specified range of an XY location */
bool scrStructureBuiltInRange();

// generate a random number
bool scrRandom();

// randomise the random number seed
bool scrRandomiseSeed();

//explicitly enables a research topic
bool scrEnableResearch();

//acts as if the research topic was completed - used to jump into the tree
bool scrCompleteResearch();

// start a reticule button flashing
bool scrFlashOn();

// stop a reticule button flashing
bool scrFlashOff();

//set the initial power level settings for a player
bool scrSetPowerLevel();

//add some power for a player
bool scrAddPower();

//set the landing Zone position for the map
bool scrSetLandingZone();

/*set the landing Zone position for the Limbo droids*/
bool scrSetLimboLanding();

//initialises all the no go areas
bool scrInitAllNoGoAreas();

//set a no go area for the map - landing zones for the enemy, or player 0
bool scrSetNoGoArea();

// set the zoom level for the radar
bool scrSetRadarZoom();

//set the time delay for reinforcements for an offworld mission
bool scrSetReinforcementTime();

//set how long an offworld mission can last -1 = no limit
bool scrSetMissionTime();

// this returns how long is left for the current mission time is 1/100th sec - same units as passed in
bool scrMissionTimeRemaining();

// clear all the console messages
bool scrFlushConsoleMessages();

// find and manipulate a position to build a structure.
bool scrPickStructLocation();
bool scrPickStructLocationB();
bool scrPickStructLocationC();

// establish the distance between two points in world coordinates - approximate bounded to 11% out
bool scrDistanceTwoPts();

// decides if a base object can see another - you can select whether walls matter to line of sight
bool scrLOSTwoBaseObjects();

// destroys all structures of a certain type within a certain area and gives a gfx effect if you want it
bool scrDestroyStructuresInArea();

// Estimates a threat from droids within a certain area
bool scrThreatInArea();

// gets the nearest gateway to a list of points
bool scrGetNearestGateway();

// Lets the user specify which tile goes under water.
bool scrSetWaterTile();

// lets the user specify which tile	is used for rubble on skyscraper destruction
bool scrSetRubbleTile();

// Tells the game what campaign it's in
bool scrSetCampaignNumber();

// tests whether a structure has a module. If structure is null, then any structure
bool scrTestStructureModule();

// give a player a template from another player
bool scrAddTemplate();

// Sets the transporter entry and exit points for the map
bool scrSetTransporterExit();

// Fly transporters in at start of map
bool scrFlyTransporterIn();

// Add droid to transporter
bool scrAddDroidToTransporter();

bool scrDestroyUnitsInArea();

// Removes a droid from thr world without all the graphical hoo ha.
bool scrRemoveDroid();

// Sets an object to be a certain percent damaged
bool scrForceDamage();

bool scrGetGameStatus();

enum GAMESTATUS
{
	STATUS_ReticuleIsOpen,
	STATUS_BattleMapViewEnabled,
	STATUS_DeliveryReposInProgress
};

//get the colour number used by a player
bool scrGetPlayerColour();
bool scrGetPlayerColourName();

//set the colour number to use for a player
bool scrSetPlayerColour();

//set all droids in an area to belong to a different player
bool scrTakeOverDroidsInArea();

/*this takes over a single droid and passes a pointer back to the new one*/
bool scrTakeOverSingleDroid();

// set all droids in an area of a certain experience level or less to belong to
// a different player - returns the number of droids changed
bool scrTakeOverDroidsInAreaExp();

/*this takes over a single structure and passes a pointer back to the new one*/
bool scrTakeOverSingleStructure();

//set all structures in an area to belong to a different player - returns the number of droids changed
//will not work on factories for the selectedPlayer
bool scrTakeOverStructsInArea();

//set Flag for defining what happens to the droids in a Transporter
bool scrSetDroidsToSafetyFlag();

//set Flag for defining whether the coded countDown is called
bool scrSetPlayCountDown();

//get the number of droids currently onthe map for a player
bool scrGetDroidCount();

// fire a weapon stat at an object
bool scrFireWeaponAtObj();

// fire a weapon stat at a location
bool scrFireWeaponAtLoc();

bool scrClearConsole();

// set the number of kills for a droid
bool scrSetDroidKills();

// get the number of kills for a droid
bool scrGetDroidKills();

// reset the visibility for a player
bool scrResetPlayerVisibility();

// set the vtol return pos for a player
bool scrSetVTOLReturnPos();

// skirmish function **NOT PSX**
bool scrIsVtol();

// init templates for tutorial.
bool scrTutorialTemplates();

//called via the script in a Limbo Expand level to set the level to plain ol' expand
bool scrResetLimboMission();

// skirmish lassat fire.
bool scrSkFireLassat();

//-----------------------------------------
//New functions
//-----------------------------------------

bool scrStrcmp();
bool scrConsole();
bool scrDbgMsgOn();
bool scrDbg();
bool scrMsg();
bool scrDebugFile();

bool scrActionDroidObj();
bool scrInitEnumDroids();
bool scrEnumDroid();
bool scrInitIterateGroupB();
bool scrIterateGroupB();
bool scrFactoryGetTemplate();
bool scrNumTemplatesInProduction();
bool scrNumDroidsByComponent();
bool scrGetStructureLimit();
bool scrStructureLimitReached();
bool scrGetNumStructures();
bool scrGetUnitLimit();
bool scrMin();
bool scrMax();
bool scrFMin();
bool scrFMax();
bool scrFogTileInRange();
bool scrMapRevealedInRange();
bool scrMapTileVisible();
bool scrPursueResearch();
bool scrNumResearchLeft();
bool scrResearchCompleted();
bool scrResearchStarted();
bool scrThreatInRange();
bool scrNumEnemyWeapObjInRange();
bool scrNumEnemyWeapDroidsInRange();
bool scrNumEnemyWeapStructsInRange();
bool scrNumFriendlyWeapObjInRange();
bool scrNumFriendlyWeapDroidsInRange();
bool scrNumFriendlyWeapStructsInRange();
bool scrNumPlayerWeapDroidsInRange();
bool scrNumPlayerWeapStructsInRange();
bool scrNumPlayerWeapObjInRange();
bool scrNumEnemyObjInRange();
bool scrEnemyWeapObjCostInRange();
bool scrFriendlyWeapObjCostInRange();
bool scrNumStructsByStatInRange();
bool scrNumStructsByStatInArea();
bool scrNumStructsByTypeInRange();
bool scrNumFeatByTypeInRange();
bool scrNumStructsButNotWallsInRangeVis();
bool scrGetStructureVis();
bool scrChooseValidLoc();
bool scrGetClosestEnemy();
bool scrTransporterCapacity();
bool scrTransporterFlying();
bool scrUnloadTransporter();
bool scrHasGroup();
bool scrObjWeaponMaxRange();
bool scrObjHasWeapon();
bool scrObjectHasIndirectWeapon();
bool scrGetClosestEnemyDroidByType();
bool scrGetClosestEnemyStructByType();
bool scrSkDefenseLocationB();
bool scrCirclePerimPoint();

bool scrGiftRadar();
bool scrNumAllies();
bool scrNumAAinRange();
bool scrSelectDroid();
bool scrSelectGroup();
bool scrModulo();
bool scrPlayerLoaded();
bool scrRemoveBeacon();
bool scrDropBeacon();
bool scrClosestDamagedGroupDroid();
bool scrMsgBox();
bool scrGetStructureType();
bool scrGetPlayerName();
bool scrSetPlayerName();
bool scrStructInRangeVis();
bool scrDroidInRangeVis();

bool scrGetBit();
bool scrSetBit();
bool scrAlliancesLocked();
bool scrASSERT();
bool scrShowRangeAtPos();
bool scrToPow();
bool scrDebugMenu();
bool scrSetDebugMenuEntry();
bool scrProcessChatMsg();
bool scrGetChatCmdDescription();
bool scrGetNumArgsInCmd();
bool scrGetChatCmdParam();
bool scrChatCmdIsPlayerAddressed();
bool scrSetTileHeight();
bool scrGetTileStructure();
bool scrPrintCallStack();
bool scrDebugModeEnabled();
bool scrCalcDroidPower();
bool scrGetDroidLevel();
bool scrMoveDroidStopped();
bool scrUpdateVisibleTiles();
bool scrCheckVisibleTile();
bool scrAssembleWeaponTemplate();
bool scrWeaponLongHitUpgrade();
bool scrWeaponDamageUpgrade();
bool scrWeaponFirePauseUpgrade();
bool scrIsComponentAvailable();
bool scrGetBodySize();
bool scrGettext();
bool scrGettext_noop();
bool scrPgettext();
bool scrPgettext_expr();
bool scrPgettext_noop();

bool beingResearchedByAlly(SDWORD resIndex, SDWORD player);
bool ThreatInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs);
bool skTopicAvail(UWORD inc, UDWORD player);
UDWORD numPlayerWeapDroidsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs);
UDWORD numPlayerWeapStructsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bFinished);
UDWORD playerWeapDroidsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs);
UDWORD playerWeapStructsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bFinished);
UDWORD numEnemyObjInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs, bool bFinished);
bool addBeaconBlip(SDWORD x, SDWORD y, SDWORD forPlayer, SDWORD sender, const char *textMsg);
bool sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, const char *beaconMsg);
MESSAGE *findBeaconMsg(UDWORD player, SDWORD sender);
SDWORD getNumRepairedBy(struct DROID *psDroidToCheck, SDWORD player);
bool objectInRangeVis(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range, SDWORD lookingPlayer);
SDWORD getPlayerFromString(char *playerName);
bool scrExp();
bool scrSqrt();
bool scrLog();

VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY);

bool scrEnumUnbuilt();
bool scrIterateUnbuilt();

#endif // __INCLUDED_SRC_SCRIPTFUNCS_H__
