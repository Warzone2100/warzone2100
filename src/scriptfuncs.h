/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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

extern BOOL scriptInit(void);
extern void scriptSetStartPos(int position, int x, int	y);
extern void scriptSetDerrickPos(int x, int y);

extern BOOL scrGetPlayer(void);
extern BOOL scrGetDerrick();
extern BOOL scrGetDifficulty(void);
extern BOOL scrScavengersActive(void);
extern BOOL scrGetPlayerStartPosition(void);
extern BOOL scrSafeDest(void);
extern BOOL scrThreatAt(void);
extern Vector2i getPlayerStartPosition(int player);
extern BOOL scrSetSunPosition(void);
extern BOOL scrSetSunIntensity(void);

// not used in scripts, but used in code.
extern  BOOL objectInRange(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range);

// Check for any player object being within a certain range of a position
extern BOOL scrObjectInRange(void);

// Check for a droid being within a certain range of a position
extern BOOL scrDroidInRange(void);

// Check for a struct being within a certain range of a position
extern BOOL scrStructInRange(void);

// return power of a player.
extern BOOL scrPlayerPower(void);

// Check for any player object being within a certain area
extern BOOL scrObjectInArea(void);

// Check for a droid being within a certain area
extern BOOL scrDroidInArea(void);

// Check for a struct being within a certain Area of a position
extern BOOL scrStructInArea(void);

// as above, but only visible structures.
extern BOOL scrSeenStructInArea(void);

// Check for a players structures but no walls being within a certain area
extern BOOL scrStructButNoWallsInArea(void);

// Count the number of player objects within a certain area
extern BOOL scrNumObjectsInArea(void);

// Count the number of player droids within a certain area
extern BOOL scrNumDroidsInArea(void);

// Count the number of player structures within a certain area
extern BOOL scrNumStructsInArea(void);

// Count the number of player structures but not walls within a certain area
extern BOOL scrNumStructsButNotWallsInArea(void);

// Count the number of structures in an area of a certain type
extern BOOL scrNumStructsByTypeInArea(void);

// Check for a droid having seen a certain object
extern BOOL scrDroidHasSeen(void);

// Enable a component to be researched
extern BOOL scrEnableComponent(void);

// Make a component available
extern BOOL scrMakeComponentAvailable(void);

//Enable a structure type to be built
extern BOOL	scrEnableStructure(void);

// true if structure is available.
extern BOOL scrIsStructureAvailable(void);

// Build a droid
extern BOOL scrAddDroid(void);

// Build a droid
extern BOOL scrAddDroidToMissionList(void);

//builds a droid in the specified factory//
extern BOOL scrBuildDroid(void);

//check for a building to have been destroyed
extern BOOL scrBuildingDestroyed(void);

// Add a reticule button to the interface
extern BOOL scrAddReticuleButton(void);

//Remove a reticule button from the interface
extern BOOL scrRemoveReticuleButton(void);

// add a message to the Intelligence Display
extern BOOL scrAddMessage(void);

// add a tutorial message to the Intelligence Display
//extern BOOL scrAddTutorialMessage(void);

//make the droid with the matching id the currently selected droid
extern BOOL scrSelectDroidByID(void);

// for a specified player, set the assembly point droids go to when built
extern BOOL	scrSetAssemblyPoint(void);

// test for structure is idle or not
extern BOOL	scrStructureIdle(void);

// sends a players droids to a location to attack
extern BOOL	scrAttackLocation(void);

// enumerate features;
extern BOOL scrInitGetFeature(void);
extern BOOL scrGetFeature(void);
extern BOOL scrGetFeatureB(void);

//Add a feature
extern BOOL scrAddFeature(void);

//Destroy a feature
extern BOOL scrDestroyFeature(void);

//Add a structure
extern BOOL scrAddStructure(void);

//Destroy a structure
extern BOOL scrDestroyStructure(void);

// enumerate structures
extern BOOL scrInitEnumStruct(void);
extern BOOL scrEnumStruct(void);
extern BOOL scrInitEnumStructB(void);
extern BOOL scrEnumStructB(void);

/*looks to see if a structure (specified by type) exists */
extern BOOL scrStructureBeingBuilt(void);

/* almost the same as above, but only for a specific struct*/
// pc multiplayer only for now.
extern BOOL scrStructureComplete(void);

/*looks to see if a structure (specified by type) exists and built*/
extern BOOL scrStructureBuilt(void);

/*centre theview on an object - can be droid/structure or feature */
extern BOOL scrCentreView(void);

/*centre the view on a position */
extern BOOL scrCentreViewPos(void);

// Get a pointer to a structure based on a stat - returns NULL if cannot find one
extern BOOL scrGetStructure(void);

// Get a pointer to a template based on a component stat - returns NULL if cannot find one
extern BOOL scrGetTemplate(void);

// Get a pointer to a droid based on a component stat - returns NULL if cannot find one
extern BOOL scrGetDroid(void);

// Sets all the scroll params for the map
extern BOOL scrSetScrollParams(void);

// Sets the scroll minX separately for the map
extern BOOL scrSetScrollMinX(void);

// Sets the scroll minY separately for the map
extern BOOL scrSetScrollMinY(void);

// Sets the scroll maxX separately for the map
extern BOOL scrSetScrollMaxX(void);

// Sets the scroll maxY separately for the map
extern BOOL scrSetScrollMaxY(void);

// Sets which sensor will be used as the default for a player
extern BOOL scrSetDefaultSensor(void);

// Sets which ECM will be used as the default for a player
extern BOOL scrSetDefaultECM(void);

// Sets which RepairUnit will be used as the default for a player
extern BOOL scrSetDefaultRepair(void);

// Sets the structure limits for a player
extern BOOL scrSetStructureLimits(void);

// Sets all structure limits for a player to a specified value
extern BOOL scrSetAllStructureLimits(void);


//multiplayer limit handler
extern BOOL scrApplyLimitSet(void);


// plays a sound for the specified player - only plays the sound if the
//specified player = selectedPlayer
extern BOOL scrPlaySound(void);

// plays a sound for the specified player - only plays the sound if the
// specified player = selectedPlayer - saves position
extern BOOL scrPlaySoundPos(void);

/* add a text message tothe top of the screen for the selected player*/
extern BOOL scrAddConsoleText(void);

// same as above - but it doesn't clear what's there and isn't permanent
extern	BOOL scrShowConsoleText(void);


/* Adds console text without clearing old */
extern BOOL scrTagConsoleText(void);


//demo functions for turning the power on
extern BOOL scrTurnPowerOff(void);

//demo functions for turning the power off
extern BOOL scrTurnPowerOn(void);

//flags when the tutorial is over so that console messages can be turned on again
extern BOOL scrTutorialEnd(void);

//function to play a full-screen video in the middle of the game for the selected player
extern BOOL scrPlayVideo(void);

//checks to see if there are any droids for the specified player
extern BOOL scrAnyDroidsLeft(void);

//checks to see if there are any structures (except walls) for the specified player
extern BOOL scrAnyStructButWallsLeft(void);

extern BOOL scrAnyFactoriesLeft(void);

//function to call when the game is over, plays a message.
extern BOOL scrGameOverMessage(void);

//function to call when the game is over
extern BOOL scrGameOver(void);

//defines the background audio to play
extern BOOL scrPlayBackgroundAudio(void);

// cd audio funcs
extern BOOL scrPlayIngameCDAudio(void);
extern BOOL scrStopCDAudio(void);
extern BOOL scrPauseCDAudio(void);
extern BOOL scrResumeCDAudio(void);

// set the retreat point for a player
extern BOOL scrSetRetreatPoint(void);

// set the retreat force level
extern BOOL scrSetRetreatForce(void);

// set the retreat leadership
extern BOOL scrSetRetreatLeadership(void);

// set the retreat point for a group
extern BOOL scrSetGroupRetreatPoint(void);

extern BOOL scrSetGroupRetreatForce(void);

// set the retreat leadership
extern BOOL scrSetGroupRetreatLeadership(void);

// set the retreat health level
BOOL scrSetRetreatHealth(void);
BOOL scrSetGroupRetreatHealth(void);

//start a Mission
extern BOOL scrStartMission(void);

//end a mission NO LONGER CALLED FROM SCRIPT
//extern BOOL scrEndMission(void);

//set Snow (enable disable snow)
extern BOOL scrSetSnow(void);

//set Rain (enable disable Rain)
extern BOOL scrSetRain(void);

//set Background Fog (replace fade out with fog)
extern BOOL scrSetBackgroundFog(void);

//set Depth Fog (gradual fog from mid range to edge of world)
extern BOOL scrSetDepthFog(void);

//set Mission Fog colour, may be modified by weather effects
extern BOOL scrSetFogColour(void);

// remove a message from the Intelligence Display
extern BOOL scrRemoveMessage(void);

// Pop up a message box with a number value in it
extern BOOL scrNumMB(void);

// Do an approximation to a square root
extern BOOL scrApproxRoot(void);

extern BOOL scrRefTest(void);

// is <player> human or a computer? (multiplayer)
extern BOOL	scrIsHumanPlayer(void);

// Set an alliance between two players
extern BOOL scrCreateAlliance(void);

extern BOOL scrOfferAlliance(void);

// Break an alliance between two players
extern BOOL scrBreakAlliance(void);

// push true if an alliance still exists.
extern BOOL scrAllianceExists(void);
extern BOOL scrAllianceExistsBetween(void);

// true if player is allied.
extern BOOL scrPlayerInAlliance(void);

// push true if group wins are allowed.
//extern BOOL scrAllianceState(void);

// push true if a single alliance is dominant.
extern BOOL scrDominatingAlliance(void);

// push true if human player is responsible for 'player'
extern BOOL	scrMyResponsibility(void);

/*checks to see if a structure of the type specified exists within the
specified range of an XY location */
extern BOOL scrStructureBuiltInRange(void);

// generate a random number
extern BOOL scrRandom(void);

// randomise the random number seed
extern BOOL scrRandomiseSeed(void);

//explicitly enables a research topic
extern BOOL scrEnableResearch(void);

//acts as if the research topic was completed - used to jump into the tree
extern BOOL scrCompleteResearch(void);

// start a reticule button flashing
extern BOOL scrFlashOn(void);

// stop a reticule button flashing
extern BOOL scrFlashOff(void);

//set the initial power level settings for a player
extern BOOL scrSetPowerLevel(void);

//add some power for a player
extern BOOL scrAddPower(void);

//set the landing Zone position for the map
extern BOOL scrSetLandingZone(void);

/*set the landing Zone position for the Limbo droids*/
extern BOOL scrSetLimboLanding(void);

//initialises all the no go areas
extern BOOL scrInitAllNoGoAreas(void);

//set a no go area for the map - landing zones for the enemy, or player 0
extern BOOL scrSetNoGoArea(void);

// set the zoom level for the radar
extern BOOL scrSetRadarZoom(void);

//set the time delay for reinforcements for an offworld mission
extern BOOL scrSetReinforcementTime(void);

//set how long an offworld mission can last -1 = no limit
extern BOOL scrSetMissionTime(void);

// this returns how long is left for the current mission time is 1/100th sec - same units as passed in
extern BOOL scrMissionTimeRemaining(void);

// clear all the console messages
extern BOOL scrFlushConsoleMessages(void);

// find and manipulate a position to build a structure.
extern BOOL scrPickStructLocation(void);
extern BOOL scrPickStructLocationB(void);
extern BOOL scrPickStructLocationC(void);

// establish the distance between two points in world coordinates - approximate bounded to 11% out
extern BOOL scrDistanceTwoPts( void );

// decides if a base object can see another - you can select whether walls matter to line of sight
extern BOOL	scrLOSTwoBaseObjects( void );

// destroys all structures of a certain type within a certain area and gives a gfx effect if you want it
extern BOOL	scrDestroyStructuresInArea( void );

// Estimates a threat from droids within a certain area
extern BOOL	scrThreatInArea( void );

// gets the nearest gateway to a list of points
extern BOOL scrGetNearestGateway( void );

// Lets the user specify which tile goes under water.
extern BOOL	scrSetWaterTile(void);

// lets the user specify which tile	is used for rubble on skyscraper destruction
extern BOOL	scrSetRubbleTile(void);

// Tells the game what campaign it's in
extern BOOL	scrSetCampaignNumber(void);

// tests whether a structure has a module. If structure is null, then any structure
extern BOOL	scrTestStructureModule(void);

// give a player a template from another player
extern BOOL scrAddTemplate(void);

// Sets the transporter entry and exit points for the map
extern BOOL scrSetTransporterExit(void);

// Fly transporters in at start of map
extern BOOL scrFlyTransporterIn(void);

// Add droid to transporter
extern BOOL scrAddDroidToTransporter(void);


extern	BOOL	scrDestroyUnitsInArea( void );

// Removes a droid from thr world without all the graphical hoo ha.
extern BOOL	scrRemoveDroid( void );

// Sets an object to be a certain percent damaged
extern BOOL	scrForceDamage( void );

extern BOOL scrGetGameStatus(void);

enum GAMESTATUS
{
	STATUS_ReticuleIsOpen,
	STATUS_BattleMapViewEnabled,
	STATUS_DeliveryReposInProgress
};

//get the colour number used by a player
extern BOOL scrGetPlayerColour(void);
extern BOOL scrGetPlayerColourName(void);

//set the colour number to use for a player
extern BOOL scrSetPlayerColour(void);

//set all droids in an area to belong to a different player
extern BOOL scrTakeOverDroidsInArea(void);

/*this takes over a single droid and passes a pointer back to the new one*/
extern BOOL scrTakeOverSingleDroid(void);

// set all droids in an area of a certain experience level or less to belong to
// a different player - returns the number of droids changed
extern BOOL scrTakeOverDroidsInAreaExp(void);

/*this takes over a single structure and passes a pointer back to the new one*/
extern BOOL scrTakeOverSingleStructure(void);

//set all structures in an area to belong to a different player - returns the number of droids changed
//will not work on factories for the selectedPlayer
extern BOOL scrTakeOverStructsInArea(void);

//set Flag for defining what happens to the droids in a Transporter
extern BOOL scrSetDroidsToSafetyFlag(void);

//set Flag for defining whether the coded countDown is called
extern BOOL scrSetPlayCountDown(void);

//get the number of droids currently onthe map for a player
extern BOOL scrGetDroidCount(void);

// fire a weapon stat at an object
extern BOOL scrFireWeaponAtObj(void);

// fire a weapon stat at a location
extern BOOL scrFireWeaponAtLoc(void);

extern BOOL	scrClearConsole(void);

// set the number of kills for a droid
extern BOOL scrSetDroidKills(void);

// get the number of kills for a droid
extern BOOL scrGetDroidKills(void);

// reset the visibility for a player
extern BOOL scrResetPlayerVisibility(void);

// set the vtol return pos for a player
extern BOOL scrSetVTOLReturnPos(void);

// skirmish function **NOT PSX**
extern BOOL scrIsVtol(void);

// init templates for tutorial.
extern BOOL scrTutorialTemplates(void);

//called via the script in a Limbo Expand level to set the level to plain ol' expand
extern BOOL scrResetLimboMission(void);

// skirmish lassat fire.
extern BOOL scrSkFireLassat(void);

//-----------------------------------------
//New functions
//-----------------------------------------

extern BOOL scrStrcmp(void);
extern BOOL scrConsole(void);
extern BOOL scrDbgMsgOn(void);
extern BOOL scrDbg(void);
extern BOOL scrMsg(void);
extern BOOL scrDebugFile(void);

extern BOOL scrActionDroidObj(void);
extern BOOL scrInitEnumDroids(void);
extern BOOL scrEnumDroid(void);
extern BOOL scrInitIterateGroupB(void);
extern BOOL scrIterateGroupB(void);
extern BOOL	scrFactoryGetTemplate(void);
extern BOOL scrNumTemplatesInProduction(void);
extern BOOL scrNumDroidsByComponent(void);
extern BOOL scrGetStructureLimit(void);
extern BOOL scrStructureLimitReached(void);
extern BOOL scrGetNumStructures(void);
extern BOOL scrGetUnitLimit(void);
extern BOOL scrMin(void);
extern BOOL scrMax(void);
extern BOOL scrFMin(void);
extern BOOL scrFMax(void);
extern BOOL scrFogTileInRange(void);
extern BOOL scrMapRevealedInRange(void);
extern BOOL scrMapTileVisible(void);
extern BOOL scrPursueResearch(void);
extern BOOL scrNumResearchLeft(void);
extern BOOL scrResearchCompleted(void);
extern BOOL scrResearchStarted(void);
extern BOOL scrThreatInRange(void);
extern BOOL scrNumEnemyWeapObjInRange(void);
extern BOOL scrNumEnemyWeapDroidsInRange(void);
extern BOOL scrNumEnemyWeapStructsInRange(void);
extern BOOL scrNumFriendlyWeapObjInRange(void);
extern BOOL scrNumFriendlyWeapDroidsInRange(void);
extern BOOL scrNumFriendlyWeapStructsInRange(void);
extern BOOL scrNumPlayerWeapDroidsInRange(void);
extern BOOL scrNumPlayerWeapStructsInRange(void);
extern BOOL scrNumPlayerWeapObjInRange(void);
extern BOOL scrNumEnemyObjInRange(void);
extern BOOL scrEnemyWeapObjCostInRange(void);
extern BOOL scrFriendlyWeapObjCostInRange(void);
extern BOOL scrNumStructsByStatInRange(void);
extern BOOL scrNumStructsByStatInArea(void);
extern BOOL scrNumStructsByTypeInRange(void);
extern BOOL scrNumFeatByTypeInRange(void);
extern BOOL scrNumStructsButNotWallsInRangeVis(void);
extern BOOL scrGetStructureVis(void);
extern BOOL scrChooseValidLoc(void);
extern BOOL scrGetClosestEnemy(void);
extern BOOL scrTransporterCapacity(void);
extern BOOL scrTransporterFlying(void);
extern BOOL scrUnloadTransporter(void);
extern BOOL scrHasGroup(void);
extern BOOL scrObjWeaponMaxRange(void);
extern BOOL scrObjHasWeapon(void);
extern BOOL scrObjectHasIndirectWeapon(void);
extern BOOL scrGetClosestEnemyDroidByType(void);
extern BOOL scrGetClosestEnemyStructByType(void);
extern BOOL scrSkDefenseLocationB(void);
extern BOOL scrCirclePerimPoint(void);

extern BOOL scrGiftRadar(void);
extern BOOL scrNumAllies(void);
extern BOOL scrNumAAinRange(void);
extern BOOL scrSelectDroid(void);
extern BOOL scrSelectGroup(void);
extern BOOL scrModulo(void);
extern BOOL scrPlayerLoaded(void);
extern BOOL scrRemoveBeacon(void);
extern BOOL scrDropBeacon(void);
extern BOOL scrClosestDamagedGroupDroid(void);
extern BOOL scrMsgBox(void);
extern BOOL scrGetStructureType(void);
extern BOOL scrGetPlayerName(void);
extern BOOL scrSetPlayerName(void);
extern BOOL scrStructInRangeVis(void);
extern BOOL scrDroidInRangeVis(void);

extern BOOL scrGetBit(void);
extern BOOL scrSetBit(void);
extern BOOL scrAlliancesLocked(void);
extern BOOL scrASSERT(void);
extern BOOL scrShowRangeAtPos(void);
extern BOOL scrToPow(void);
extern BOOL scrDebugMenu(void);
extern BOOL scrSetDebugMenuEntry(void);
extern BOOL scrProcessChatMsg(void);
extern BOOL scrGetChatCmdDescription(void);
extern BOOL	scrGetNumArgsInCmd(void);
extern BOOL	scrGetChatCmdParam(void);
extern BOOL scrChatCmdIsPlayerAddressed(void);
extern BOOL scrSetTileHeight(void);
extern BOOL scrGetTileStructure(void);
extern BOOL scrPrintCallStack(void);
extern BOOL scrDebugModeEnabled(void);
extern BOOL scrCalcDroidPower(void);
extern BOOL scrGetDroidLevel(void);
extern BOOL scrMoveDroidStopped(void);
extern BOOL scrUpdateVisibleTiles(void);
extern BOOL scrCheckVisibleTile(void);
extern BOOL scrAssembleWeaponTemplate(void);
extern BOOL scrWeaponShortHitUpgrade(void);
extern BOOL scrWeaponLongHitUpgrade(void);
extern BOOL scrWeaponDamageUpgrade(void);
extern BOOL scrWeaponFirePauseUpgrade(void);
extern BOOL scrIsComponentAvailable(void);
extern BOOL scrGetBodySize(void);
extern BOOL scrGettext(void);
extern BOOL scrGettext_noop(void);
extern BOOL scrPgettext(void);
extern BOOL scrPgettext_expr(void);
extern BOOL scrPgettext_noop(void);


extern BOOL beingResearchedByAlly(SDWORD resIndex, SDWORD player);
extern BOOL ThreatInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs);
extern BOOL skTopicAvail(UWORD inc, UDWORD player);
extern UDWORD numPlayerWeapDroidsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs);
extern UDWORD numPlayerWeapStructsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bFinished);
extern UDWORD playerWeapDroidsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs);
extern UDWORD playerWeapStructsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bFinished);
extern UDWORD numEnemyObjInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs, BOOL bFinished);
extern BOOL addBeaconBlip(SDWORD x, SDWORD y, SDWORD forPlayer, SDWORD sender, char * textMsg);
extern BOOL sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * beaconMsg);
extern MESSAGE * findBeaconMsg(UDWORD player, SDWORD sender);
extern SDWORD getNumRepairedBy(struct DROID *psDroidToCheck, SDWORD player);
extern BOOL objectInRangeVis(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range, SDWORD lookingPlayer);
extern SDWORD getPlayerFromString(char *playerName);
extern BOOL scrExp(void);
extern BOOL scrSqrt(void);
extern BOOL scrLog(void);

extern BOOL addBeaconBlip(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * textMsg);
extern VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY);

extern BOOL scrEnumUnbuilt(void);
extern BOOL scrIterateUnbuilt(void);

#endif // __INCLUDED_SRC_SCRIPTFUNCS_H__
