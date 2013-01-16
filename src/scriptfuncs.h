/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include "messagedef.h"			//for VIEWDATA

// AI won't build there if there are more than
// MAX_BLOCKING_TILES on some location
#define MAX_BLOCKING_TILES		1

/// Forward declarations to allow pointers to these types
struct BASE_OBJECT;
struct DROID;

extern bool scriptInit(void);
extern void scriptSetStartPos(int position, int x, int	y);
extern void scriptSetDerrickPos(int x, int y);

extern bool scrGetPlayer(void);
extern bool scrGetDerrick();
extern bool scrGetDifficulty(void);
extern bool scrScavengersActive(void);
extern bool scrGetPlayerStartPosition(void);
extern bool scrSafeDest(void);
extern bool scrThreatAt(void);
extern Vector2i getPlayerStartPosition(int player);
extern bool scrSetSunPosition(void);
extern bool scrSetSunIntensity(void);

// not used in scripts, but used in code.
extern  bool objectInRange(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range);

// Check for any player object being within a certain range of a position
extern bool scrObjectInRange(void);

// Check for a droid being within a certain range of a position
extern bool scrDroidInRange(void);

// Check for a struct being within a certain range of a position
extern bool scrStructInRange(void);

// return power of a player.
extern bool scrPlayerPower(void);

// Check for any player object being within a certain area
extern bool scrObjectInArea(void);

// Check for a droid being within a certain area
extern bool scrDroidInArea(void);

// Check for a struct being within a certain Area of a position
extern bool scrStructInArea(void);

// as above, but only visible structures.
extern bool scrSeenStructInArea(void);

// Check for a players structures but no walls being within a certain area
extern bool scrStructButNoWallsInArea(void);

// Count the number of player objects within a certain area
extern bool scrNumObjectsInArea(void);

// Count the number of player droids within a certain area
extern bool scrNumDroidsInArea(void);

// Count the number of player structures within a certain area
extern bool scrNumStructsInArea(void);

// Count the number of player structures but not walls within a certain area
extern bool scrNumStructsButNotWallsInArea(void);

// Count the number of structures in an area of a certain type
extern bool scrNumStructsByTypeInArea(void);

// Check for a droid having seen a certain object
extern bool scrDroidHasSeen(void);

// Enable a component to be researched
extern bool scrEnableComponent(void);

// Make a component available
extern bool scrMakeComponentAvailable(void);

//Enable a structure type to be built
extern bool	scrEnableStructure(void);

// true if structure is available.
extern bool scrIsStructureAvailable(void);

// Build a droid
extern bool scrAddDroid(void);

// Build a droid
extern bool scrAddDroidToMissionList(void);

//builds a droid in the specified factory//
extern bool scrBuildDroid(void);

//check for a building to have been destroyed
extern bool scrBuildingDestroyed(void);

// Add a reticule button to the interface
extern bool scrAddReticuleButton(void);

//Remove a reticule button from the interface
extern bool scrRemoveReticuleButton(void);

// add a message to the Intelligence Display
extern bool scrAddMessage(void);

// add a tutorial message to the Intelligence Display
//extern bool scrAddTutorialMessage(void);

//make the droid with the matching id the currently selected droid
extern bool scrSelectDroidByID(void);

// for a specified player, set the assembly point droids go to when built
extern bool	scrSetAssemblyPoint(void);

// test for structure is idle or not
extern bool	scrStructureIdle(void);

// sends a players droids to a location to attack
extern bool	scrAttackLocation(void);

// enumerate features;
extern bool scrInitGetFeature(void);
extern bool scrGetFeature(void);
extern bool scrGetFeatureB(void);

//Add a feature
extern bool scrAddFeature(void);

//Destroy a feature
extern bool scrDestroyFeature(void);

//Add a structure
extern bool scrAddStructure(void);

//Destroy a structure
extern bool scrDestroyStructure(void);

// enumerate structures
extern bool scrInitEnumStruct(void);
extern bool scrEnumStruct(void);
extern bool scrInitEnumStructB(void);
extern bool scrEnumStructB(void);

/*looks to see if a structure (specified by type) exists */
extern bool scrStructureBeingBuilt(void);

/* almost the same as above, but only for a specific struct*/
// pc multiplayer only for now.
extern bool scrStructureComplete(void);

/*looks to see if a structure (specified by type) exists and built*/
extern bool scrStructureBuilt(void);

/*centre theview on an object - can be droid/structure or feature */
extern bool scrCentreView(void);

/*centre the view on a position */
extern bool scrCentreViewPos(void);

// Get a pointer to a structure based on a stat - returns NULL if cannot find one
extern bool scrGetStructure(void);

// Get a pointer to a template based on a component stat - returns NULL if cannot find one
extern bool scrGetTemplate(void);

// Get a pointer to a droid based on a component stat - returns NULL if cannot find one
extern bool scrGetDroid(void);

// Sets all the scroll params for the map
extern bool scrSetScrollParams(void);

// Sets the scroll minX separately for the map
extern bool scrSetScrollMinX(void);

// Sets the scroll minY separately for the map
extern bool scrSetScrollMinY(void);

// Sets the scroll maxX separately for the map
extern bool scrSetScrollMaxX(void);

// Sets the scroll maxY separately for the map
extern bool scrSetScrollMaxY(void);

// Sets which sensor will be used as the default for a player
extern bool scrSetDefaultSensor(void);

// Sets which ECM will be used as the default for a player
extern bool scrSetDefaultECM(void);

// Sets which RepairUnit will be used as the default for a player
extern bool scrSetDefaultRepair(void);

// Sets the structure limits for a player
extern bool scrSetStructureLimits(void);

// Sets all structure limits for a player to a specified value
extern bool scrSetAllStructureLimits(void);


//multiplayer limit handler
extern bool scrApplyLimitSet(void);


// plays a sound for the specified player - only plays the sound if the
//specified player = selectedPlayer
extern bool scrPlaySound(void);

// plays a sound for the specified player - only plays the sound if the
// specified player = selectedPlayer - saves position
extern bool scrPlaySoundPos(void);

/* add a text message tothe top of the screen for the selected player*/
extern bool scrAddConsoleText(void);

// same as above - but it doesn't clear what's there and isn't permanent
extern	bool scrShowConsoleText(void);


/* Adds console text without clearing old */
extern bool scrTagConsoleText(void);


//demo functions for turning the power on
extern bool scrTurnPowerOff(void);

//demo functions for turning the power off
extern bool scrTurnPowerOn(void);

//flags when the tutorial is over so that console messages can be turned on again
extern bool scrTutorialEnd(void);

//function to play a full-screen video in the middle of the game for the selected player
extern bool scrPlayVideo(void);

//checks to see if there are any droids for the specified player
extern bool scrAnyDroidsLeft(void);

//checks to see if there are any structures (except walls) for the specified player
extern bool scrAnyStructButWallsLeft(void);

extern bool scrAnyFactoriesLeft(void);

//function to call when the game is over, plays a message.
extern bool scrGameOverMessage(void);

//function to call when the game is over
extern bool scrGameOver(void);

//defines the background audio to play
extern bool scrPlayBackgroundAudio(void);

// cd audio funcs
extern bool scrPlayIngameCDAudio(void);
extern bool scrStopCDAudio(void);
extern bool scrPauseCDAudio(void);
extern bool scrResumeCDAudio(void);

// set the retreat point for a player
extern bool scrSetRetreatPoint(void);

// set the retreat force level
extern bool scrSetRetreatForce(void);

// set the retreat leadership
extern bool scrSetRetreatLeadership(void);

// set the retreat point for a group
extern bool scrSetGroupRetreatPoint(void);

extern bool scrSetGroupRetreatForce(void);

// set the retreat leadership
extern bool scrSetGroupRetreatLeadership(void);

// set the retreat health level
bool scrSetRetreatHealth(void);
bool scrSetGroupRetreatHealth(void);

//start a Mission
extern bool scrStartMission(void);

//end a mission NO LONGER CALLED FROM SCRIPT
//extern bool scrEndMission(void);

//set Snow (enable disable snow)
extern bool scrSetSnow(void);

//set Rain (enable disable Rain)
extern bool scrSetRain(void);

//set Background Fog (replace fade out with fog)
extern bool scrSetBackgroundFog(void);

//set Depth Fog (gradual fog from mid range to edge of world)
extern bool scrSetDepthFog(void);

//set Mission Fog colour, may be modified by weather effects
extern bool scrSetFogColour(void);

// remove a message from the Intelligence Display
extern bool scrRemoveMessage(void);

// Pop up a message box with a number value in it
extern bool scrNumMB(void);

// Do an approximation to a square root
extern bool scrApproxRoot(void);

extern bool scrRefTest(void);

// is <player> human or a computer? (multiplayer)
extern bool	scrIsHumanPlayer(void);

// Set an alliance between two players
extern bool scrCreateAlliance(void);

extern bool scrOfferAlliance(void);

// Break an alliance between two players
extern bool scrBreakAlliance(void);

// push true if an alliance still exists.
extern bool scrAllianceExists(void);
extern bool scrAllianceExistsBetween(void);

// true if player is allied.
extern bool scrPlayerInAlliance(void);

// push true if group wins are allowed.
//extern bool scrAllianceState(void);

// push true if a single alliance is dominant.
extern bool scrDominatingAlliance(void);

// push true if human player is responsible for 'player'
extern bool	scrMyResponsibility(void);

/*checks to see if a structure of the type specified exists within the
specified range of an XY location */
extern bool scrStructureBuiltInRange(void);

// generate a random number
extern bool scrRandom(void);

// randomise the random number seed
extern bool scrRandomiseSeed(void);

//explicitly enables a research topic
extern bool scrEnableResearch(void);

//acts as if the research topic was completed - used to jump into the tree
extern bool scrCompleteResearch(void);

// start a reticule button flashing
extern bool scrFlashOn(void);

// stop a reticule button flashing
extern bool scrFlashOff(void);

//set the initial power level settings for a player
extern bool scrSetPowerLevel(void);

//add some power for a player
extern bool scrAddPower(void);

//set the landing Zone position for the map
extern bool scrSetLandingZone(void);

/*set the landing Zone position for the Limbo droids*/
extern bool scrSetLimboLanding(void);

//initialises all the no go areas
extern bool scrInitAllNoGoAreas(void);

//set a no go area for the map - landing zones for the enemy, or player 0
extern bool scrSetNoGoArea(void);

// set the zoom level for the radar
extern bool scrSetRadarZoom(void);

//set the time delay for reinforcements for an offworld mission
extern bool scrSetReinforcementTime(void);

//set how long an offworld mission can last -1 = no limit
extern bool scrSetMissionTime(void);

// this returns how long is left for the current mission time is 1/100th sec - same units as passed in
extern bool scrMissionTimeRemaining(void);

// clear all the console messages
extern bool scrFlushConsoleMessages(void);

// find and manipulate a position to build a structure.
extern bool scrPickStructLocation(void);
extern bool scrPickStructLocationB(void);
extern bool scrPickStructLocationC(void);

// establish the distance between two points in world coordinates - approximate bounded to 11% out
extern bool scrDistanceTwoPts( void );

// decides if a base object can see another - you can select whether walls matter to line of sight
extern bool	scrLOSTwoBaseObjects( void );

// destroys all structures of a certain type within a certain area and gives a gfx effect if you want it
extern bool	scrDestroyStructuresInArea( void );

// Estimates a threat from droids within a certain area
extern bool	scrThreatInArea( void );

// gets the nearest gateway to a list of points
extern bool scrGetNearestGateway( void );

// Lets the user specify which tile goes under water.
extern bool	scrSetWaterTile(void);

// lets the user specify which tile	is used for rubble on skyscraper destruction
extern bool	scrSetRubbleTile(void);

// Tells the game what campaign it's in
extern bool	scrSetCampaignNumber(void);

// tests whether a structure has a module. If structure is null, then any structure
extern bool	scrTestStructureModule(void);

// give a player a template from another player
extern bool scrAddTemplate(void);

// Sets the transporter entry and exit points for the map
extern bool scrSetTransporterExit(void);

// Fly transporters in at start of map
extern bool scrFlyTransporterIn(void);

// Add droid to transporter
extern bool scrAddDroidToTransporter(void);


extern	bool	scrDestroyUnitsInArea( void );

// Removes a droid from thr world without all the graphical hoo ha.
extern bool	scrRemoveDroid( void );

// Sets an object to be a certain percent damaged
extern bool	scrForceDamage( void );

extern bool scrGetGameStatus(void);

enum GAMESTATUS
{
	STATUS_ReticuleIsOpen,
	STATUS_BattleMapViewEnabled,
	STATUS_DeliveryReposInProgress
};

//get the colour number used by a player
extern bool scrGetPlayerColour(void);
extern bool scrGetPlayerColourName(void);

//set the colour number to use for a player
extern bool scrSetPlayerColour(void);

//set all droids in an area to belong to a different player
extern bool scrTakeOverDroidsInArea(void);

/*this takes over a single droid and passes a pointer back to the new one*/
extern bool scrTakeOverSingleDroid(void);

// set all droids in an area of a certain experience level or less to belong to
// a different player - returns the number of droids changed
extern bool scrTakeOverDroidsInAreaExp(void);

/*this takes over a single structure and passes a pointer back to the new one*/
extern bool scrTakeOverSingleStructure(void);

//set all structures in an area to belong to a different player - returns the number of droids changed
//will not work on factories for the selectedPlayer
extern bool scrTakeOverStructsInArea(void);

//set Flag for defining what happens to the droids in a Transporter
extern bool scrSetDroidsToSafetyFlag(void);

//set Flag for defining whether the coded countDown is called
extern bool scrSetPlayCountDown(void);

//get the number of droids currently onthe map for a player
extern bool scrGetDroidCount(void);

// fire a weapon stat at an object
extern bool scrFireWeaponAtObj(void);

// fire a weapon stat at a location
extern bool scrFireWeaponAtLoc(void);

extern bool	scrClearConsole(void);

// set the number of kills for a droid
extern bool scrSetDroidKills(void);

// get the number of kills for a droid
extern bool scrGetDroidKills(void);

// reset the visibility for a player
extern bool scrResetPlayerVisibility(void);

// set the vtol return pos for a player
extern bool scrSetVTOLReturnPos(void);

// skirmish function **NOT PSX**
extern bool scrIsVtol(void);

// init templates for tutorial.
extern bool scrTutorialTemplates(void);

//called via the script in a Limbo Expand level to set the level to plain ol' expand
extern bool scrResetLimboMission(void);

// skirmish lassat fire.
extern bool scrSkFireLassat(void);

//-----------------------------------------
//New functions
//-----------------------------------------

extern bool scrStrcmp(void);
extern bool scrConsole(void);
extern bool scrDbgMsgOn(void);
extern bool scrDbg(void);
extern bool scrMsg(void);
extern bool scrDebugFile(void);

extern bool scrActionDroidObj(void);
extern bool scrInitEnumDroids(void);
extern bool scrEnumDroid(void);
extern bool scrInitIterateGroupB(void);
extern bool scrIterateGroupB(void);
extern bool	scrFactoryGetTemplate(void);
extern bool scrNumTemplatesInProduction(void);
extern bool scrNumDroidsByComponent(void);
extern bool scrGetStructureLimit(void);
extern bool scrStructureLimitReached(void);
extern bool scrGetNumStructures(void);
extern bool scrGetUnitLimit(void);
extern bool scrMin(void);
extern bool scrMax(void);
extern bool scrFMin(void);
extern bool scrFMax(void);
extern bool scrFogTileInRange(void);
extern bool scrMapRevealedInRange(void);
extern bool scrMapTileVisible(void);
extern bool scrPursueResearch(void);
extern bool scrNumResearchLeft(void);
extern bool scrResearchCompleted(void);
extern bool scrResearchStarted(void);
extern bool scrThreatInRange(void);
extern bool scrNumEnemyWeapObjInRange(void);
extern bool scrNumEnemyWeapDroidsInRange(void);
extern bool scrNumEnemyWeapStructsInRange(void);
extern bool scrNumFriendlyWeapObjInRange(void);
extern bool scrNumFriendlyWeapDroidsInRange(void);
extern bool scrNumFriendlyWeapStructsInRange(void);
extern bool scrNumPlayerWeapDroidsInRange(void);
extern bool scrNumPlayerWeapStructsInRange(void);
extern bool scrNumPlayerWeapObjInRange(void);
extern bool scrNumEnemyObjInRange(void);
extern bool scrEnemyWeapObjCostInRange(void);
extern bool scrFriendlyWeapObjCostInRange(void);
extern bool scrNumStructsByStatInRange(void);
extern bool scrNumStructsByStatInArea(void);
extern bool scrNumStructsByTypeInRange(void);
extern bool scrNumFeatByTypeInRange(void);
extern bool scrNumStructsButNotWallsInRangeVis(void);
extern bool scrGetStructureVis(void);
extern bool scrChooseValidLoc(void);
extern bool scrGetClosestEnemy(void);
extern bool scrTransporterCapacity(void);
extern bool scrTransporterFlying(void);
extern bool scrUnloadTransporter(void);
extern bool scrHasGroup(void);
extern bool scrObjWeaponMaxRange(void);
extern bool scrObjHasWeapon(void);
extern bool scrObjectHasIndirectWeapon(void);
extern bool scrGetClosestEnemyDroidByType(void);
extern bool scrGetClosestEnemyStructByType(void);
extern bool scrSkDefenseLocationB(void);
extern bool scrCirclePerimPoint(void);

extern bool scrGiftRadar(void);
extern bool scrNumAllies(void);
extern bool scrNumAAinRange(void);
extern bool scrSelectDroid(void);
extern bool scrSelectGroup(void);
extern bool scrModulo(void);
extern bool scrPlayerLoaded(void);
extern bool scrRemoveBeacon(void);
extern bool scrDropBeacon(void);
extern bool scrClosestDamagedGroupDroid(void);
extern bool scrMsgBox(void);
extern bool scrGetStructureType(void);
extern bool scrGetPlayerName(void);
extern bool scrSetPlayerName(void);
extern bool scrStructInRangeVis(void);
extern bool scrDroidInRangeVis(void);

extern bool scrGetBit(void);
extern bool scrSetBit(void);
extern bool scrAlliancesLocked(void);
extern bool scrASSERT(void);
extern bool scrShowRangeAtPos(void);
extern bool scrToPow(void);
extern bool scrDebugMenu(void);
extern bool scrSetDebugMenuEntry(void);
extern bool scrProcessChatMsg(void);
extern bool scrGetChatCmdDescription(void);
extern bool	scrGetNumArgsInCmd(void);
extern bool	scrGetChatCmdParam(void);
extern bool scrChatCmdIsPlayerAddressed(void);
extern bool scrSetTileHeight(void);
extern bool scrGetTileStructure(void);
extern bool scrPrintCallStack(void);
extern bool scrDebugModeEnabled(void);
extern bool scrCalcDroidPower(void);
extern bool scrGetDroidLevel(void);
extern bool scrMoveDroidStopped(void);
extern bool scrUpdateVisibleTiles(void);
extern bool scrCheckVisibleTile(void);
extern bool scrAssembleWeaponTemplate(void);
extern bool scrWeaponShortHitUpgrade(void);
extern bool scrWeaponLongHitUpgrade(void);
extern bool scrWeaponDamageUpgrade(void);
extern bool scrWeaponFirePauseUpgrade(void);
extern bool scrIsComponentAvailable(void);
extern bool scrGetBodySize(void);
extern bool scrGettext(void);
extern bool scrGettext_noop(void);
extern bool scrPgettext(void);
extern bool scrPgettext_expr(void);
extern bool scrPgettext_noop(void);


extern bool beingResearchedByAlly(SDWORD resIndex, SDWORD player);
extern bool ThreatInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs);
extern bool skTopicAvail(UWORD inc, UDWORD player);
extern UDWORD numPlayerWeapDroidsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs);
extern UDWORD numPlayerWeapStructsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bFinished);
extern UDWORD playerWeapDroidsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs);
extern UDWORD playerWeapStructsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bFinished);
extern UDWORD numEnemyObjInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs, bool bFinished);
extern bool addBeaconBlip(SDWORD x, SDWORD y, SDWORD forPlayer, SDWORD sender, char * textMsg);
extern bool sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * beaconMsg);
extern MESSAGE * findBeaconMsg(UDWORD player, SDWORD sender);
extern SDWORD getNumRepairedBy(struct DROID *psDroidToCheck, SDWORD player);
extern bool objectInRangeVis(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range, SDWORD lookingPlayer);
extern SDWORD getPlayerFromString(char *playerName);
extern bool scrExp(void);
extern bool scrSqrt(void);
extern bool scrLog(void);

extern bool addBeaconBlip(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * textMsg);
extern VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY);

extern bool scrEnumUnbuilt(void);
extern bool scrIterateUnbuilt(void);

#endif // __INCLUDED_SRC_SCRIPTFUNCS_H__
