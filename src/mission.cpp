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
/**
 * @file mission.c
 *
 * All the stuff relevant to a mission.
 */
#include <time.h>

#include "mission.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/gamelib/gtime.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/cdaudio.h"
#include "lib/widget/label.h"
#include "lib/widget/widget.h"

#include "game.h"
#include "challenge.h"
#include "projectile.h"
#include "power.h"
#include "structure.h"
#include "message.h"
#include "research.h"
#include "hci.h"
#include "order.h"
#include "action.h"
#include "display3d.h"
#include "effects.h"
#include "radar.h"
#include "transporter.h"
#include "group.h"
#include "frontend.h"		// for displaytextoption.
#include "intdisplay.h"
#include "main.h"
#include "display.h"
#include "loadsave.h"
#include "scripttabs.h"
#include "cmddroid.h"
#include "warcam.h"
#include "wrappers.h"
#include "console.h"
#include "droid.h"
#include "data.h"
#include "multiplay.h"
#include "loop.h"
#include "visibility.h"
#include "mapgrid.h"
#include "cluster.h"
#include "gateway.h"
#include "selection.h"
#include "scores.h"
#include "keymap.h"
#include "texture.h"
#include "warzoneconfig.h"
#include "combat.h"
#include "qtscript.h"

#define		IDMISSIONRES_TXT		11004
#define		IDMISSIONRES_LOAD		11005
#define		IDMISSIONRES_CONTINUE		11008
#define		IDMISSIONRES_BACKFORM		11013
#define		IDMISSIONRES_TITLE		11014

/* Mission timer label position */
#define		TIMER_LABELX			15
#define		TIMER_LABELY			0

/* Transporter Timer form position */
#define		TRAN_FORM_X			STAT_X
#define		TRAN_FORM_Y			TIMER_Y

/* Transporter Timer position */
#define		TRAN_TIMER_X			4
#define		TRAN_TIMER_Y			TIMER_LABELY
#define		TRAN_TIMER_WIDTH		25

#define		MISSION_1_X			5	// pos of text options in box.
#define		MISSION_1_Y			15
#define		MISSION_2_X			5
#define		MISSION_2_Y			35
#define		MISSION_3_X			5
#define		MISSION_3_Y			55

#define		MISSION_TEXT_W			MISSIONRES_W-10
#define		MISSION_TEXT_H			16

// These are used for mission countdown
#define TEN_MINUTES         (10 * 60 * GAME_TICKS_PER_SEC)
#define FIVE_MINUTES        (5 * 60 * GAME_TICKS_PER_SEC)
#define FOUR_MINUTES        (4 * 60 * GAME_TICKS_PER_SEC)
#define THREE_MINUTES       (3 * 60 * GAME_TICKS_PER_SEC)
#define TWO_MINUTES         (2 * 60 * GAME_TICKS_PER_SEC)
#define ONE_MINUTE          (1 * 60 * GAME_TICKS_PER_SEC)
#define NOT_PLAYED_ONE          0x01
#define NOT_PLAYED_TWO          0x02
#define NOT_PLAYED_THREE        0x04
#define NOT_PLAYED_FIVE         0x08
#define NOT_PLAYED_TEN          0x10
#define NOT_PLAYED_ACTIVATED    0x20

MISSION		mission;

bool		offWorldKeepLists;

/*lists of droids that are held seperate over several missions. There should
only be selectedPlayer's droids but have possibility for MAX_PLAYERS -
also saves writing out list functions to cater for just one player*/
DROID       *apsLimboDroids[MAX_PLAYERS];

//Where the Transporter lands for player 0 (sLandingZone[0]), and the rest are
//a list of areas that cannot be built on, used for landing the enemy transporters
static LANDING_ZONE		sLandingZone[MAX_NOGO_AREAS];

//flag to indicate when the droids in a Transporter are flown to safety and not the next mission
static bool             bDroidsToSafety;

// return positions for vtols
Vector2i asVTOLReturnPos[MAX_PLAYERS];

static UBYTE   missionCountDown;
//flag to indicate whether the coded mission countdown is played
static UBYTE   bPlayCountDown;

//FUNCTIONS**************
static void addLandingLights(UDWORD x, UDWORD y);
static bool startMissionOffClear(char *pGame);
static bool startMissionOffKeep(char *pGame);
static bool startMissionCampaignStart(char *pGame);
static bool startMissionCampaignChange(char *pGame);
static bool startMissionCampaignExpand(char *pGame);
static bool startMissionCampaignExpandLimbo(char *pGame);
static bool startMissionBetween(void);
static void endMissionCamChange(void);
static void endMissionOffClear(void);
static void endMissionOffKeep(void);
static void endMissionOffKeepLimbo(void);
static void endMissionExpandLimbo(void);

static void saveMissionData(void);
static void restoreMissionData(void);
static void saveCampaignData(void);
static void missionResetDroids(void);
static void saveMissionLimboData(void);
static void restoreMissionLimboData(void);
static void processMissionLimbo(void);

static void intUpdateMissionTimer(WIDGET *psWidget, W_CONTEXT *psContext);
static bool intAddMissionTimer(void);
static void intUpdateTransporterTimer(WIDGET *psWidget, W_CONTEXT *psContext);
static void adjustMissionPower(void);
static void saveMissionPower(void);
static UDWORD getHomeLandingX(void);
static UDWORD getHomeLandingY(void);
static void processPreviousCampDroids(void);
static bool intAddTransporterTimer(void);
static void clearCampaignUnits(void);
static void emptyTransporters(bool bOffWorld);

bool MissionResUp	= false;

static SDWORD		g_iReinforceTime = 0;

/* Which campaign are we dealing with? */
static	UDWORD	camNumber = 1;


//returns true if on an off world mission
bool missionIsOffworld(void)
{
	return ((mission.type == LDS_MKEEP)
	        || (mission.type == LDS_MCLEAR)
	        || (mission.type == LDS_MKEEP_LIMBO)
	       );
}

//returns true if the correct type of mission for reinforcements
bool missionForReInforcements(void)
{
	if (mission.type == LDS_CAMSTART || missionIsOffworld() || mission.type == LDS_CAMCHANGE)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//returns true if the correct type of mission and a reinforcement time has been set
bool missionCanReEnforce(void)
{
	if (mission.ETA >= 0)
	{
		if (missionForReInforcements())
		{
			return true;
		}
	}
	return false;
}

//returns true if the mission is a Limbo Expand mission
bool missionLimboExpand(void)
{
	return (mission.type == LDS_EXPAND_LIMBO);
}

// mission initialisation game code
void initMission(void)
{
	debug(LOG_SAVE, "*** Init Mission ***");
	mission.type = LDS_NONE;
	for (int inc = 0; inc < MAX_PLAYERS; inc++)
	{
		mission.apsStructLists[inc] = NULL;
		mission.apsDroidLists[inc] = NULL;
		mission.apsFeatureLists[inc] = NULL;
		mission.apsFlagPosLists[inc] = NULL;
		mission.apsExtractorLists[inc] = NULL;
		apsLimboDroids[inc] = NULL;
	}
	mission.apsSensorList[0] = NULL;
	mission.apsOilList[0] = NULL;
	offWorldKeepLists = false;
	mission.time = -1;
	setMissionCountDown();

	mission.ETA = -1;
	mission.startTime = 0;
	mission.psGateways = NULL;
	mission.mapHeight = 0;
	mission.mapWidth = 0;
	for (int i = 0; i < ARRAY_SIZE(mission.psBlockMap); ++i)
	{
		mission.psBlockMap[i] = NULL;
	}
	for (int i = 0; i < ARRAY_SIZE(mission.psAuxMap); ++i)
	{
		mission.psAuxMap[i] = NULL;
	}

	//init all the landing zones
	for (int inc = 0; inc < MAX_NOGO_AREAS; inc++)
	{
		sLandingZone[inc].x1 = sLandingZone[inc].y1 = sLandingZone[inc].x2 = sLandingZone[inc].y2 = 0;
	}

	// init the vtol return pos
	memset(asVTOLReturnPos, 0, sizeof(Vector2i) * MAX_PLAYERS);

	bDroidsToSafety = false;
	setPlayCountDown(true);

	//start as not cheating!
	mission.cheatTime = 0;
}

// reset the vtol landing pos
void resetVTOLLandingPos(void)
{
	memset(asVTOLReturnPos, 0, sizeof(Vector2i)*MAX_PLAYERS);
}

//this is called everytime the game is quit
void releaseMission(void)
{
	/* mission.apsDroidLists may contain some droids that have been transferred from one campaign to the next */
	freeAllMissionDroids();

	/* apsLimboDroids may contain some droids that have been saved at the end of one mission and not yet used */
	freeAllLimboDroids();
}

//called to shut down when mid-mission on an offWorld map
bool missionShutDown(void)
{
	debug(LOG_SAVE, "called, mission is %s", missionIsOffworld() ? "off-world" : "main map");
	if (missionIsOffworld())
	{
		//clear out the audio
		audio_StopAll();

		freeAllDroids();
		freeAllStructs();
		freeAllFeatures();
		releaseAllProxDisp();
		gwShutDown();

		for (int inc = 0; inc < MAX_PLAYERS; inc++)
		{
			apsDroidLists[inc] = mission.apsDroidLists[inc];
			mission.apsDroidLists[inc] = NULL;
			apsStructLists[inc] = mission.apsStructLists[inc];
			mission.apsStructLists[inc] = NULL;
			apsFeatureLists[inc] = mission.apsFeatureLists[inc];
			mission.apsFeatureLists[inc] = NULL;
			apsFlagPosLists[inc] = mission.apsFlagPosLists[inc];
			mission.apsFlagPosLists[inc] = NULL;
			apsExtractorLists[inc] = mission.apsExtractorLists[inc];
			mission.apsExtractorLists[inc] = NULL;
		}
		apsSensorList[0] = mission.apsSensorList[0];
		apsOilList[0] = mission.apsOilList[0];
		mission.apsSensorList[0] = NULL;
		mission.apsOilList[0] = NULL;

		psMapTiles = mission.psMapTiles;
		mapWidth = mission.mapWidth;
		mapHeight = mission.mapHeight;
		for (int i = 0; i < ARRAY_SIZE(mission.psBlockMap); ++i)
		{
			free(psBlockMap[i]);
			psBlockMap[i] = mission.psBlockMap[i];
			mission.psBlockMap[i] = NULL;
		}
		for (int i = 0; i < ARRAY_SIZE(mission.psAuxMap); ++i)
		{
			free(psAuxMap[i]);
			psAuxMap[i] = mission.psAuxMap[i];
			mission.psAuxMap[i] = NULL;
		}
		gwSetGateways(mission.psGateways);
	}

	// sorry if this breaks something - but it looks like it's what should happen - John
	mission.type = LDS_NONE;

	return true;
}


/*on the PC - sets the countdown played flag*/
void setMissionCountDown(void)
{
	SDWORD		timeRemaining;

	timeRemaining = mission.time - (gameTime - mission.startTime);
	if (timeRemaining < 0)
	{
		timeRemaining = 0;
	}

	// Need to init the countdown played each time the mission time is changed
	missionCountDown = NOT_PLAYED_ONE | NOT_PLAYED_TWO | NOT_PLAYED_THREE | NOT_PLAYED_FIVE | NOT_PLAYED_TEN | NOT_PLAYED_ACTIVATED;

	if (timeRemaining < TEN_MINUTES - 1)
	{
		missionCountDown &= ~NOT_PLAYED_TEN;
	}
	if (timeRemaining < FIVE_MINUTES - 1)
	{
		missionCountDown &= ~NOT_PLAYED_FIVE;
	}
	if (timeRemaining < THREE_MINUTES - 1)
	{
		missionCountDown &= ~NOT_PLAYED_THREE;
	}
	if (timeRemaining < TWO_MINUTES - 1)
	{
		missionCountDown &= ~NOT_PLAYED_TWO;
	}
	if (timeRemaining < ONE_MINUTE - 1)
	{
		missionCountDown &= ~NOT_PLAYED_ONE;
	}
}


bool startMission(LEVEL_TYPE missionType, char *pGame)
{
	bool	loaded = true;

	debug(LOG_SAVE, "type %d", (int)missionType);

	/* Player has (obviously) not failed at the start */
	setPlayerHasLost(false);
	setPlayerHasWon(false);

	/* and win/lose video is obvioulsy not playing */
	setScriptWinLoseVideo(PLAY_NONE);

	// this inits the flag so that 'reinforcements have arrived' message isn't played for the first transporter load
	initFirstTransporterFlag();

	if (mission.type != LDS_NONE)
	{
		/*mission type gets set to none when you have returned from a mission
		so don't want to go another mission when already on one! - so ignore*/
		debug(LOG_SAVE, "Already on a mission");
		return true;
	}

	// reset the cluster stuff
	clustInitialise();
	initEffectsSystem();

	//load the game file for all types of mission except a Between Mission
	if (missionType != LDS_BETWEEN)
	{
		loadGameInit(pGame);
	}

	//all proximity messages are removed between missions now
	releaseAllProxDisp();

	switch (missionType)
	{
	case LDS_CAMSTART:
		if (!startMissionCampaignStart(pGame))
		{
			loaded = false;
		}
		break;
	case LDS_MKEEP:
	case LDS_MKEEP_LIMBO:
		if (!startMissionOffKeep(pGame))
		{
			loaded = false;
		}
		break;
	case LDS_BETWEEN:
		//do anything?
		if (!startMissionBetween())
		{
			loaded = false;
		}
		break;
	case LDS_CAMCHANGE:
		if (!startMissionCampaignChange(pGame))
		{
			loaded = false;
		}
		break;
	case LDS_EXPAND:
		if (!startMissionCampaignExpand(pGame))
		{
			loaded = false;
		}
		break;
	case LDS_EXPAND_LIMBO:
		if (!startMissionCampaignExpandLimbo(pGame))
		{
			loaded = false;
		}
		break;
	case LDS_MCLEAR:
		if (!startMissionOffClear(pGame))
		{
			loaded = false;
		}
		break;
	default:
		//error!
		debug(LOG_ERROR, "Unknown Mission Type");

		loaded = false;
	}

	if (!loaded)
	{
		debug(LOG_ERROR, "Failed to start mission, missiontype = %d, game, %s", (int)missionType, pGame);
		return false;
	}

	mission.type = missionType;

	if (missionIsOffworld())
	{
		//add what power have got from the home base
		adjustMissionPower();
	}

	if (missionCanReEnforce())
	{
		//add mission timer - if necessary
		addMissionTimerInterface();

		//add Transporter Timer
		addTransporterTimerInterface();
	}

	scoreInitSystem();

	return true;
}


// initialise the mission stuff for a save game
bool startMissionSave(SDWORD missionType)
{
	mission.type = missionType;

	return true;
}


/*checks the time has been set and then adds the timer if not already on
the display*/
void addMissionTimerInterface(void)
{
	//don't add if the timer hasn't been set
	if (mission.time < 0 && !challengeActive)
	{
		return;
	}

	//check timer is not already on the screen
	if (!widgGetFromID(psWScreen, IDTIMER_FORM))
	{
		intAddMissionTimer();
	}
}

/*checks that the timer has been set and that a Transporter exists before
adding the timer button*/
void addTransporterTimerInterface(void)
{
	DROID           *psTransporter = NULL;
	bool            bAddInterface = false;
	W_CLICKFORM     *psForm;

	//check if reinforcements are allowed
	if (mission.ETA >= 0)
	{
		//check the player has at least one Transporter back at base
		for (DROID *psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
			{
				psTransporter = psDroid;
				break;
			}
		}
		if (psTransporter)
		{
			bAddInterface = true;

			//check timer is not already on the screen
			if (!widgGetFromID(psWScreen, IDTRANTIMER_BUTTON))
			{
				intAddTransporterTimer();
			}

			//set the data for the transporter timer
			widgSetUserData(psWScreen, IDTRANTIMER_DISPLAY, (void *)psTransporter);

			// lock the button if necessary
			if (transporterFlying(psTransporter))
			{
				// disable the form so can't add any more droids into the transporter
				psForm = (W_CLICKFORM *)widgGetFromID(psWScreen, IDTRANTIMER_BUTTON);
				if (psForm)
				{
					formSetClickState(psForm, WBUT_LOCK);
				}
			}
		}
	}
	// if criteria not met
	if (!bAddInterface)
	{
		// make sure its not there!
		intRemoveTransporterTimer();
	}
}

#define OFFSCREEN_HEIGHT	600
#define	EDGE_SIZE	1

/* pick nearest map edge to point */
void missionGetNearestCorner(UWORD iX, UWORD iY, UWORD *piOffX, UWORD *piOffY)
{
	UDWORD	iMidX = (scrollMinX + scrollMaxX) / 2,
	        iMidY = (scrollMinY + scrollMaxY) / 2;

	if (map_coord(iX) < iMidX)
	{
		*piOffX = (UWORD)(world_coord(scrollMinX) + (EDGE_SIZE * TILE_UNITS));
	}
	else
	{
		*piOffX = (UWORD)(world_coord(scrollMaxX) - (EDGE_SIZE * TILE_UNITS));
	}

	if (map_coord(iY) < iMidY)
	{
		*piOffY = (UWORD)(world_coord(scrollMinY) + (EDGE_SIZE * TILE_UNITS));
	}
	else
	{
		*piOffY = (UWORD)(world_coord(scrollMaxY) - (EDGE_SIZE * TILE_UNITS));
	}
}

/* fly in transporters at start of level */
void missionFlyTransportersIn(SDWORD iPlayer, bool bTrackTransporter)
{
	DROID	*psTransporter, *psNext;
	UWORD	iX, iY, iZ;
	SDWORD	iLandX, iLandY, iDx, iDy;

	ASSERT_OR_RETURN(, iPlayer < MAX_PLAYERS, "Flying nonexistent player %d's transporters in", iPlayer);

	iLandX = getLandingX(iPlayer);
	iLandY = getLandingY(iPlayer);
	missionGetTransporterEntry(iPlayer, &iX, &iY);
	iZ = (UWORD)(map_Height(iX, iY) + OFFSCREEN_HEIGHT);

	psNext = NULL;
	//get the droids for the mission
	for (psTransporter = mission.apsDroidLists[iPlayer]; psTransporter != NULL; psTransporter = psNext)
	{
		psNext = psTransporter->psNext;
		// FIXME: When we convert campaign scripts to use DROID_SUPERTRANSPORTER
		if (psTransporter->droidType == DROID_TRANSPORTER)
		{
			// Check that this transporter actually contains some droids
			if (psTransporter->psGroup && psTransporter->psGroup->refCount > 1)
			{
				// Remove map information from previous map
				free(psTransporter->watchedTiles);
				psTransporter->watchedTiles = NULL;
				psTransporter->numWatchedTiles = 0;

				// Remove out of stored list and add to current Droid list
				if (droidRemove(psTransporter, mission.apsDroidLists))
				{
					// Do not want to add it unless managed to remove it from the previous list
					addDroid(psTransporter, apsDroidLists);
				}

				/* set start position */
				psTransporter->pos.x = iX;
				psTransporter->pos.y = iY;
				psTransporter->pos.z = iZ;

				/* set start direction */
				iDx = iLandX - iX;
				iDy = iLandY - iY;

				psTransporter->rot.direction = iAtan2(iDx, iDy);

				// Camera track requested and it's the selected player.
				if ((bTrackTransporter == true) && (iPlayer == (SDWORD)selectedPlayer))
				{
					/* deselect all droids */
					selDroidDeselect(selectedPlayer);

					if (getWarCamStatus())
					{
						camToggleStatus();
					}

					/* select transporter */
					psTransporter->selected = true;
					camToggleStatus();
				}

				//little hack to ensure all Transporters are fully repaired by time enter world
				psTransporter->body = psTransporter->originalBody;

				/* set fly-in order */
				orderDroidLoc(psTransporter, DORDER_TRANSPORTIN, iLandX, iLandY, ModeImmediate);

				audio_PlayObjDynamicTrack(psTransporter, ID_SOUND_BLIMP_FLIGHT, moveCheckDroidMovingAndVisible);

				//only want to fly one transporter in at a time now - AB 14/01/99
				break;
			}
		}
	}
}

/* Saves the necessary data when moving from a home base Mission to an OffWorld mission */
static void saveMissionData(void)
{
	UDWORD			inc;
	DROID			*psDroid;
	STRUCTURE		*psStruct, *psStructBeingBuilt;
	bool			bRepairExists;

	debug(LOG_SAVE, "called");

	//clear out the audio
	audio_StopAll();

	//save the mission data
	mission.psMapTiles = psMapTiles;
	mission.mapWidth = mapWidth;
	mission.mapHeight = mapHeight;
	for (int i = 0; i < ARRAY_SIZE(mission.psBlockMap); ++i)
	{
		mission.psBlockMap[i] = psBlockMap[i];
	}
	for (int i = 0; i < ARRAY_SIZE(mission.psAuxMap); ++i)
	{
		mission.psAuxMap[i] = psAuxMap[i];
	}
	mission.scrollMinX = scrollMinX;
	mission.scrollMinY = scrollMinY;
	mission.scrollMaxX = scrollMaxX;
	mission.scrollMaxY = scrollMaxY;
	mission.psGateways = gwGetGateways();
	gwSetGateways(NULL);
	// save the selectedPlayer's LZ
	mission.homeLZ_X = getLandingX(selectedPlayer);
	mission.homeLZ_Y = getLandingY(selectedPlayer);

	bRepairExists = false;
	//set any structures currently being built to completed for the selected player
	for (psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->status == SS_BEING_BUILT)
		{
			//find a droid working on it
			for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
			{
				if ((psStructBeingBuilt = (STRUCTURE *)orderStateObj(psDroid, DORDER_BUILD))
				    && psStructBeingBuilt == psStruct)
				{
					// just give it all its build points
					structureBuild(psStruct, NULL, psStruct->pStructureType->buildPoints);
					//don't bother looking for any other droids working on it
					break;
				}
			}
		}
		//check if have a completed repair facility on home world
		if (psStruct->pStructureType->type == REF_REPAIR_FACILITY && psStruct->status == SS_BUILT)
		{
			bRepairExists = true;
		}
	}

	//repair all droids back at home base if have a repair facility
	if (bRepairExists)
	{
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if (droidIsDamaged(psDroid))
			{
				psDroid->body = psDroid->originalBody;
			}
		}
	}

	//clear droid orders for all droids except constructors still building
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if ((psStructBeingBuilt = (STRUCTURE *)orderStateObj(psDroid, DORDER_BUILD)))
		{
			if (psStructBeingBuilt->status == SS_BUILT)
			{
				orderDroid(psDroid, DORDER_STOP, ModeImmediate);
			}
		}
		else
		{
			orderDroid(psDroid, DORDER_STOP, ModeImmediate);
		}
	}

	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		mission.apsStructLists[inc] = apsStructLists[inc];
		mission.apsDroidLists[inc] = apsDroidLists[inc];
		mission.apsFeatureLists[inc] = apsFeatureLists[inc];
		mission.apsFlagPosLists[inc] = apsFlagPosLists[inc];
		mission.apsExtractorLists[inc] = apsExtractorLists[inc];
	}
	mission.apsSensorList[0] = apsSensorList[0];
	mission.apsOilList[0] = apsOilList[0];

	mission.playerX = player.p.x;
	mission.playerY = player.p.z;

	//save the power settings
	saveMissionPower();

	//init before loading in the new game
	initFactoryNumFlag();

	//clear all the effects from the map
	initEffectsSystem();

	resizeRadar();
}

/*
	This routine frees the memory for the offworld mission map (in the call to mapShutdown)

	- so when this routine is called we must still be set to the offworld map data
	i.e. We shoudn't have called SwapMissionPointers()

*/
void restoreMissionData(void)
{
	UDWORD		inc;
	BASE_OBJECT	*psObj;

	debug(LOG_SAVE, "called");

	//clear out the audio
	audio_StopAll();

	//clear all the lists
	proj_FreeAllProjectiles();
	freeAllDroids();
	freeAllStructs();
	freeAllFeatures();
	gwShutDown();
	if (game.type != CAMPAIGN)
	{
		ASSERT(false, "game type isn't campaign, but we are in a campaign game!");
		game.type = CAMPAIGN;	// fix the issue, since it is obviously a bug
	}
	//restore the game pointers
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		apsDroidLists[inc] = mission.apsDroidLists[inc];
		mission.apsDroidLists[inc] = NULL;
		for (psObj = (BASE_OBJECT *)apsDroidLists[inc]; psObj; psObj = psObj->psNext)
		{
			psObj->died = false;	//make sure the died flag is not set
		}

		apsStructLists[inc] = mission.apsStructLists[inc];
		mission.apsStructLists[inc] = NULL;

		apsFeatureLists[inc] = mission.apsFeatureLists[inc];
		mission.apsFeatureLists[inc] = NULL;

		apsFlagPosLists[inc] = mission.apsFlagPosLists[inc];
		mission.apsFlagPosLists[inc] = NULL;

		apsExtractorLists[inc] = mission.apsExtractorLists[inc];
		mission.apsExtractorLists[inc] = NULL;
	}
	apsSensorList[0] = mission.apsSensorList[0];
	apsOilList[0] = mission.apsOilList[0];
	mission.apsSensorList[0] = NULL;
	//swap mission data over

	psMapTiles = mission.psMapTiles;

	mapWidth = mission.mapWidth;
	mapHeight = mission.mapHeight;
	for (int i = 0; i < ARRAY_SIZE(mission.psBlockMap); ++i)
	{
		psBlockMap[i] = mission.psBlockMap[i];
		mission.psBlockMap[i] = NULL;
	}
	for (int i = 0; i < ARRAY_SIZE(mission.psAuxMap); ++i)
	{
		psAuxMap[i] = mission.psAuxMap[i];
		mission.psAuxMap[i] = NULL;
	}
	scrollMinX = mission.scrollMinX;
	scrollMinY = mission.scrollMinY;
	scrollMaxX = mission.scrollMaxX;
	scrollMaxY = mission.scrollMaxY;
	gwSetGateways(mission.psGateways);
	//and clear the mission pointers
	mission.psMapTiles	= NULL;
	mission.mapWidth	= 0;
	mission.mapHeight	= 0;
	mission.scrollMinX	= 0;
	mission.scrollMinY	= 0;
	mission.scrollMaxX	= 0;
	mission.scrollMaxY	= 0;
	mission.psGateways	= NULL;

	//reset the current structure lists
	setCurrentStructQuantity(false);

	initFactoryNumFlag();
	resetFactoryNumFlag();

	offWorldKeepLists = false;

	resizeRadar();
}

/*Saves the necessary data when moving from one mission to a limbo expand Mission*/
void saveMissionLimboData(void)
{
	DROID           *psDroid, *psNext;
	STRUCTURE           *psStruct;

	debug(LOG_SAVE, "called");

	//clear out the audio
	audio_StopAll();

	// before copy the pointers over check selectedPlayer's mission.droids since
	// there might be some from the previous camapign
	processPreviousCampDroids();

	// move droids properly - does all the clean up code
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		if (droidRemove(psDroid, apsDroidLists))
		{
			addDroid(psDroid, mission.apsDroidLists);
		}
	}
	apsDroidLists[selectedPlayer] = NULL;

	// any selectedPlayer's factories/research need to be put on holdProduction/holdresearch
	for (psStruct = apsStructLists[selectedPlayer]; psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (StructIsFactory(psStruct))
		{
			holdProduction(psStruct, ModeImmediate);
		}
		else if (psStruct->pStructureType->type == REF_RESEARCH)
		{
			holdResearch(psStruct, ModeImmediate);
		}
	}
}

//this is called via a script function to place the Limbo droids once the mission has started
void placeLimboDroids(void)
{
	DROID           *psDroid, *psNext;
	UDWORD			droidX, droidY;
	PICKTILE		pickRes;

	debug(LOG_SAVE, "called");

	// Copy the droids across for the selected Player
	for (psDroid = apsLimboDroids[selectedPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		if (droidRemove(psDroid, apsLimboDroids))
		{
			addDroid(psDroid, apsDroidLists);
			//KILL OFF TRANSPORTER - should never be one but....
			if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
			{
				vanishDroid(psDroid);
				continue;
			}
			//set up location for each of the droids
			droidX = map_coord(getLandingX(LIMBO_LANDING));
			droidY = map_coord(getLandingY(LIMBO_LANDING));
			pickRes = pickHalfATile(&droidX, &droidY, LOOK_FOR_EMPTY_TILE);
			if (pickRes == NO_FREE_TILE)
			{
				ASSERT(false, "placeLimboUnits: Unable to find a free location");
			}
			psDroid->pos.x = (UWORD)world_coord(droidX);
			psDroid->pos.y = (UWORD)world_coord(droidY);
			ASSERT(worldOnMap(psDroid->pos.x, psDroid->pos.y), "limbo droid is not on the map");
			psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
			updateDroidOrientation(psDroid);
			psDroid->selected = false;
			//this is mainly for VTOLs
			setDroidBase(psDroid, NULL);
			psDroid->cluster = 0;
			//initialise the movement data
			initDroidMovement(psDroid);
			//make sure the died flag is not set
			psDroid->died = false;
		}
		else
		{
			ASSERT(false, "placeLimboUnits: Unable to remove unit from Limbo list");
		}
	}
}

/*restores the necessary data on completion of a Limbo Expand mission*/
void restoreMissionLimboData(void)
{
	DROID           *psDroid, *psNext;

	debug(LOG_SAVE, "called");

	/*the droids stored in the mission droid list need to be added back
	into the current droid list*/
	for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		//remove out of stored list and add to current Droid list
		if (droidRemove(psDroid, mission.apsDroidLists))
		{
			addDroid(psDroid, apsDroidLists);
			psDroid->cluster = 0;
			//reset droid orders
			orderDroid(psDroid, DORDER_STOP, ModeImmediate);
			//the location of the droid should be valid!
		}
	}
	ASSERT(mission.apsDroidLists[selectedPlayer] == NULL, "list should be empty");
}

/*Saves the necessary data when moving from one campaign to the start of the
next - saves out the list of droids for the selected player*/
void saveCampaignData(void)
{
	DROID		*psDroid, *psNext, *psSafeDroid, *psNextSafe, *psCurr, *psCurrNext;

	debug(LOG_SAVE, "called");

	// If the droids have been moved to safety then get any Transporters that exist
	if (getDroidsToSafetyFlag())
	{
		// Move any Transporters into the mission list
		psDroid = apsDroidLists[selectedPlayer];
		while (psDroid != NULL)
		{
			psNext = psDroid->psNext;
			if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
			{
				// Empty the transporter into the mission list
				ASSERT(psDroid->psGroup != NULL, "saveCampaignData: Transporter does not have a group");

				for (psCurr = psDroid->psGroup->psList; psCurr != NULL && psCurr != psDroid; psCurr = psCurrNext)
				{
					psCurrNext = psCurr->psGrpNext;
					// Remove it from the transporter group
					psDroid->psGroup->remove(psCurr);
					// Cam change add droid
					psCurr->pos.x = INVALID_XY;
					psCurr->pos.y = INVALID_XY;
					// Add it back into current droid lists
					addDroid(psCurr, mission.apsDroidLists);
				}
				// Remove the transporter from the current list
				if (droidRemove(psDroid, apsDroidLists))
				{
					//cam change add droid
					psDroid->pos.x = INVALID_XY;
					psDroid->pos.y = INVALID_XY;
					addDroid(psDroid, mission.apsDroidLists);
				}
			}
			psDroid = psNext;
		}
	}
	else
	{
		// Reserve the droids for selected player for start of next campaign
		mission.apsDroidLists[selectedPlayer] = apsDroidLists[selectedPlayer];
		apsDroidLists[selectedPlayer] = NULL;
		psDroid = mission.apsDroidLists[selectedPlayer];
		while (psDroid != NULL)
		{
			//cam change add droid
			psDroid->pos.x = INVALID_XY;
			psDroid->pos.y = INVALID_XY;
			psDroid = psDroid->psNext;
		}
	}

	//if the droids have been moved to safety then get any Transporters that exist
	if (getDroidsToSafetyFlag())
	{
		/*now that every unit for the selected player has been moved into the
		mission list - reverse it and fill the transporter with the first ten units*/
		reverseObjectList(&mission.apsDroidLists[selectedPlayer]);

		//find the *first* transporter
		for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
			{
				//fill it with droids from the mission list
				for (psSafeDroid = mission.apsDroidLists[selectedPlayer]; psSafeDroid; psSafeDroid = psNextSafe)
				{
					psNextSafe = psSafeDroid->psNext;
					if (psSafeDroid != psDroid)
					{
						//add to the Transporter, checking for when full
						if (checkTransporterSpace(psDroid, psSafeDroid))
						{
							if (droidRemove(psSafeDroid, mission.apsDroidLists))
							{
								psDroid->psGroup->add(psSafeDroid);
							}
						}
						else
						{
							//setting this will cause the loop to end
							psNextSafe = NULL;
						}
					}
				}
				//only want to fill one transporter
				break;
			}
		}
	}

	//clear all other droids
	for (int inc = 0; inc < MAX_PLAYERS; inc++)
	{
		psDroid = apsDroidLists[inc];

		while (psDroid != NULL)
		{
			psNext = psDroid->psNext;
			vanishDroid(psDroid);
			psDroid = psNext;
		}
	}

	//clear out the audio
	audio_StopAll();

	//clear all other memory
	freeAllStructs();
	freeAllFeatures();
}


//start an off world mission - clearing the object lists
bool startMissionOffClear(char *pGame)
{
	debug(LOG_SAVE, "called for %s", pGame);

	saveMissionData();

	//load in the new game clearing the lists
	if (!loadGame(pGame, !KEEPOBJECTS, !FREEMEM, false))
	{
		return false;
	}

	offWorldKeepLists = false;
	intResetPreviousObj();

	// The message should have been played at the between stage
	missionCountDown &= ~NOT_PLAYED_ACTIVATED;

	return true;
}

//start an off world mission - keeping the object lists
bool startMissionOffKeep(char *pGame)
{
	debug(LOG_SAVE, "called for %s", pGame);
	saveMissionData();

	//load in the new game clearing the lists
	if (!loadGame(pGame, !KEEPOBJECTS, !FREEMEM, false))
	{
		return false;
	}

	offWorldKeepLists = true;
	intResetPreviousObj();

	// The message should have been played at the between stage
	missionCountDown &= ~NOT_PLAYED_ACTIVATED;

	return true;
}

bool startMissionCampaignStart(char *pGame)
{
	debug(LOG_SAVE, "called for %s", pGame);

	// Clear out all intelligence screen messages
	freeMessages();

	// Check no units left with any settings that are invalid
	clearCampaignUnits();

	// Load in the new game details
	if (!loadGame(pGame, !KEEPOBJECTS, FREEMEM, false))
	{
		return false;
	}

	offWorldKeepLists = false;

	return true;
}

bool startMissionCampaignChange(char *pGame)
{
	// Clear out all intelligence screen messages
	freeMessages();

	// Check no units left with any settings that are invalid
	clearCampaignUnits();

	// Clear out the production run between campaigns
	changeProductionPlayer((UBYTE)selectedPlayer);

	saveCampaignData();

	//load in the new game details
	if (!loadGame(pGame, !KEEPOBJECTS, !FREEMEM, false))
	{
		return false;
	}

	offWorldKeepLists = false;
	intResetPreviousObj();

	return true;
}

bool startMissionCampaignExpand(char *pGame)
{
	//load in the new game details
	if (!loadGame(pGame, KEEPOBJECTS, !FREEMEM, false))
	{
		return false;
	}

	offWorldKeepLists = false;
	return true;
}

bool startMissionCampaignExpandLimbo(char *pGame)
{
	saveMissionLimboData();

	//load in the new game details
	if (!loadGame(pGame, KEEPOBJECTS, !FREEMEM, false))
	{
		return false;
	}

	offWorldKeepLists = false;

	return true;
}

static bool startMissionBetween(void)
{
	offWorldKeepLists = false;

	return true;
}

//check no units left with any settings that are invalid
static void clearCampaignUnits(void)
{
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		orderDroid(psDroid, DORDER_STOP, ModeImmediate);
		setDroidBase(psDroid, NULL);
		visRemoveVisibilityOffWorld((BASE_OBJECT *)psDroid);
		CHECK_DROID(psDroid);
	}
}

/*This deals with droids at the end of an offworld mission*/
static void processMission(void)
{
	DROID			*psNext;
	DROID			*psDroid;
	UDWORD			droidX, droidY;
	PICKTILE		pickRes;

	//and the rest on the mission map  - for now?
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		//reset order - do this to all the droids that are returning from offWorld
		orderDroid(psDroid, DORDER_STOP, ModeImmediate);
		// clean up visibility
		visRemoveVisibility((BASE_OBJECT *)psDroid);
		//remove out of stored list and add to current Droid list
		if (droidRemove(psDroid, apsDroidLists))
		{
			int	x, y;

			addDroid(psDroid, mission.apsDroidLists);
			droidX = getHomeLandingX();
			droidY = getHomeLandingY();
			// Swap the droid and map pointers
			swapMissionPointers();

			pickRes = pickHalfATile(&droidX, &droidY, LOOK_FOR_EMPTY_TILE);
			ASSERT(pickRes != NO_FREE_TILE, "processMission: Unable to find a free location");
			x = (UWORD)world_coord(droidX);
			y = (UWORD)world_coord(droidY);
			droidSetPosition(psDroid, x, y);
			ASSERT(worldOnMap(psDroid->pos.x, psDroid->pos.y), "the droid is not on the map");
			updateDroidOrientation(psDroid);
			// Swap the droid and map pointers back again
			swapMissionPointers();
			psDroid->selected = false;
			// This is mainly for VTOLs
			setDroidBase(psDroid, NULL);
			psDroid->cluster = 0;
		}
	}
}


#define MAXLIMBODROIDS (999)

/*This deals with droids at the end of an offworld Limbo mission*/
void processMissionLimbo(void)
{
	DROID			*psNext, *psDroid;
	UDWORD	numDroidsAddedToLimboList = 0;

	//all droids (for selectedPlayer only) are placed into the limbo list
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		//KILL OFF TRANSPORTER - should never be one but....
		if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
		{
			vanishDroid(psDroid);
		}
		else
		{
			if (numDroidsAddedToLimboList >= MAXLIMBODROIDS)		// any room in limbo list
			{
				vanishDroid(psDroid);
			}
			else
			{
				// Remove out of stored list and add to current Droid list
				if (droidRemove(psDroid, apsDroidLists))
				{
					// Limbo list invalidate XY
					psDroid->pos.x = INVALID_XY;
					psDroid->pos.y = INVALID_XY;
					addDroid(psDroid, apsLimboDroids);
					// This is mainly for VTOLs
					setDroidBase(psDroid, NULL);
					psDroid->cluster = 0;
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
					numDroidsAddedToLimboList++;
				}
			}
		}
	}
}

/*switch the pointers for the map and droid lists so that droid placement
 and orientation can occur on the map they will appear on*/
// NOTE: This is one huge hack for campaign games!
// Pay special attention on what is getting swapped!
void swapMissionPointers(void)
{
	debug(LOG_SAVE, "called");

	std::swap(psMapTiles, mission.psMapTiles);
	std::swap(mapWidth,   mission.mapWidth);
	std::swap(mapHeight,  mission.mapHeight);
	for (int i = 0; i < ARRAY_SIZE(mission.psBlockMap); ++i)
	{
		std::swap(psBlockMap[i], mission.psBlockMap[i]);
	}
	for (int i = 0; i < ARRAY_SIZE(mission.psAuxMap); ++i)
	{
		std::swap(psAuxMap[i],   mission.psAuxMap[i]);
	}
	//swap gateway zones
	GATEWAY *gateway = gwGetGateways();
	gwSetGateways(mission.psGateways);
	mission.psGateways = gateway;
	std::swap(scrollMinX, mission.scrollMinX);
	std::swap(scrollMinY, mission.scrollMinY);
	std::swap(scrollMaxX, mission.scrollMaxX);
	std::swap(scrollMaxY, mission.scrollMaxY);
	for (unsigned inc = 0; inc < MAX_PLAYERS; inc++)
	{
		std::swap(apsDroidLists[inc],     mission.apsDroidLists[inc]);
		std::swap(apsStructLists[inc],    mission.apsStructLists[inc]);
		std::swap(apsFeatureLists[inc],   mission.apsFeatureLists[inc]);
		std::swap(apsFlagPosLists[inc],   mission.apsFlagPosLists[inc]);
		std::swap(apsExtractorLists[inc], mission.apsExtractorLists[inc]);
	}
	std::swap(apsSensorList[0], mission.apsSensorList[0]);
	std::swap(apsOilList[0],    mission.apsOilList[0]);
}

void endMission(void)
{
	if (mission.type == LDS_NONE)
	{
		//can't go back any further!!
		debug(LOG_SAVE, "Already returned from mission");
		return;
	}

	switch (mission.type)
	{
	case LDS_CAMSTART:
		//any transporters that are flying in need to be emptied
		emptyTransporters(false);
		//when loading in a save game mid cam2a or cam3a it is loaded as a camstart
		endMissionCamChange();
		/*
			This is called at the end of every campaign mission
		*/
		break;
	case LDS_MKEEP:
		//any transporters that are flying in need to be emptied
		emptyTransporters(true);
		endMissionOffKeep();
		break;
	case LDS_EXPAND:
	case LDS_BETWEEN:
		/*
			This is called at the end of every campaign mission
		*/

		break;
	case LDS_CAMCHANGE:
		//any transporters that are flying in need to be emptied
		emptyTransporters(false);
		endMissionCamChange();
		break;
		/* left in so can skip the mission for testing...*/
	case LDS_EXPAND_LIMBO:
		//shouldn't be any transporters on this mission but...who knows?
		endMissionExpandLimbo();
		break;
	case LDS_MCLEAR:
		//any transporters that are flying in need to be emptied
		emptyTransporters(true);
		endMissionOffClear();
		break;
	case LDS_MKEEP_LIMBO:
		//any transporters that are flying in need to be emptied
		emptyTransporters(true);
		endMissionOffKeepLimbo();
		break;
	default:
		//error!
		debug(LOG_FATAL, "Unknown Mission Type");
		abort();
	}

	if (missionCanReEnforce()) //mission.type == LDS_MCLEAR || mission.type == LDS_MKEEP)
	{
		intRemoveMissionTimer();
		intRemoveTransporterTimer();
	}

	//at end of mission always do this
	intRemoveTransporterLaunch();

	//and this...
	//make sure the cheat time is not set for the next mission
	mission.cheatTime = 0;


	//reset the bSetPlayCountDown flag
	setPlayCountDown(true);


	//mission.type = MISSION_NONE;
	mission.type = LDS_NONE;

	// reset the transporters
	initTransporters();
}

void endMissionCamChange(void)
{
	//get any droids remaining from the previous campaign
	processPreviousCampDroids();
}

void endMissionOffClear(void)
{
	processMission();
	restoreMissionData();

	//reset variables in Droids
	missionResetDroids();
}

void endMissionOffKeep(void)
{
	processMission();
	restoreMissionData();

	//reset variables in Droids
	missionResetDroids();
}

/*In this case any droids remaining (for selectedPlayer) go into a limbo list
for use in a future mission (expand type) */
void endMissionOffKeepLimbo(void)
{
	// Save any droids left 'alive'
	processMissionLimbo();

	// Set the lists back to the home base lists
	restoreMissionData();

	//reset variables in Droids
	missionResetDroids();
}

//This happens MID_MISSION now! but is left here in case the scripts fail but somehow get here...?
/*The selectedPlayer's droids which were separated at the start of the
mission need to merged back into the list*/
void endMissionExpandLimbo(void)
{
	restoreMissionLimboData();
}


//this is called mid Limbo mission via the script
void resetLimboMission(void)
{
	//add the units that were moved into the mission list at the start of the mission
	restoreMissionLimboData();
	//set the mission type to plain old expand...
	mission.type = LDS_EXPAND;
}

/* The update routine for all droids left back at home base
Only interested in Transporters at present*/
void missionDroidUpdate(DROID *psDroid)
{
	ASSERT(psDroid != NULL, "unitUpdate: Invalid unit pointer");

	/*This is required for Transporters that are moved offWorld so the
	saveGame doesn't try to set their position in the map - especially important
	for endCam2 where there isn't a valid map!*/
	if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
	{
		psDroid->pos.x = INVALID_XY;
		psDroid->pos.y = INVALID_XY;
	}

	//ignore all droids except Transporters
	if ((psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)
	    || !(orderState(psDroid, DORDER_TRANSPORTOUT)  ||
	            orderState(psDroid, DORDER_TRANSPORTIN)     ||
	            orderState(psDroid, DORDER_TRANSPORTRETURN)))
	{
		return;
	}

	// NO ai update droid

	// update the droids order
	orderUpdateDroid(psDroid);

	// update the action of the droid
	actionUpdateDroid(psDroid);

	//NO move update
}

// Reset variables in Droids such as order and position
static void missionResetDroids(void)
{
	debug(LOG_SAVE, "called");

	for (unsigned int player = 0; player < MAX_PLAYERS; player++)
	{
		for (DROID *psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			// Reset order - unless constructor droid that is mid-build
			if ((psDroid->droidType == DROID_CONSTRUCT
			     || psDroid->droidType == DROID_CYBORG_CONSTRUCT)
			    && orderStateObj(psDroid, DORDER_BUILD))
			{
				// Need to set the action time to ignore the previous mission time
				psDroid->actionStarted = gameTime;
			}
			else
			{
				orderDroid(psDroid, DORDER_STOP, ModeImmediate);
			}

			//KILL OFF TRANSPORTER
			if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
			{
				vanishDroid(psDroid);
			}
		}
	}

	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		bool	placed = false;

		//for all droids that have never left home base
		if (psDroid->pos.x == INVALID_XY && psDroid->pos.y == INVALID_XY)
		{
			STRUCTURE	*psStruct = psDroid->psBaseStruct;
			FACTORY		*psFactory = NULL;

			if (psStruct && StructIsFactory(psStruct))
			{
				psFactory = (FACTORY *)psStruct->pFunctionality;
			}
			//find a location next to the factory
			if (psStruct)
			{
				PICKTILE	pickRes;
				UDWORD		x, y;

				// Use factory DP if one
				if (psFactory->psAssemblyPoint)
				{
					x = map_coord(psFactory->psAssemblyPoint->coords.x);
					y = map_coord(psFactory->psAssemblyPoint->coords.y);
				}
				else
				{
					x = map_coord(psStruct->pos.x);
					y = map_coord(psStruct->pos.y);
				}
				pickRes = pickHalfATile(&x, &y, LOOK_FOR_EMPTY_TILE);
				if (pickRes == NO_FREE_TILE)
				{
					ASSERT(false, "missionResetUnits: Unable to find a free location");
					psStruct = NULL;
				}
				else
				{
					int wx = world_coord(x);
					int wy = world_coord(y);

					droidSetPosition(psDroid, wx, wy);
					placed = true;
				}
			}
			else // if couldn't find the factory - try to place near HQ instead
			{
				for (psStruct = apsStructLists[psDroid->player]; psStruct != NULL; psStruct = psStruct->psNext)
				{
					if (psStruct->pStructureType->type == REF_HQ)
					{
						UDWORD		x = map_coord(psStruct->pos.x);
						UDWORD		y = map_coord(psStruct->pos.y);
						PICKTILE	pickRes = pickHalfATile(&x, &y, LOOK_FOR_EMPTY_TILE);

						if (pickRes == NO_FREE_TILE)
						{
							ASSERT(false, "missionResetUnits: Unable to find a free location");
							psStruct = NULL;
						}
						else
						{
							int wx = world_coord(x);
							int wy = world_coord(y);

							droidSetPosition(psDroid, wx, wy);
							placed = true;
						}
						break;
					}
				}
			}
			if (placed)
			{
				// Do all the things in build droid that never did when it was built!
				// check the droid is a reasonable distance from the edge of the map
				if (psDroid->pos.x <= world_coord(EDGE_SIZE) ||
				    psDroid->pos.y <= world_coord(EDGE_SIZE) ||
				    psDroid->pos.x >= world_coord(mapWidth - EDGE_SIZE) ||
				    psDroid->pos.y >= world_coord(mapHeight - EDGE_SIZE))
				{
					debug(LOG_ERROR, "missionResetUnits: unit too close to edge of map - removing");
					vanishDroid(psDroid);
					continue;
				}

				// People always stand upright
				if (psDroid->droidType != DROID_PERSON && !cyborgDroid(psDroid))
				{
					updateDroidOrientation(psDroid);
				}
				// Reset the selected flag
				psDroid->selected = false;
			}
			else
			{
				//can't put it down so get rid of this droid!!
				ASSERT(false, "missionResetUnits: can't place unit - cancel to continue");
				vanishDroid(psDroid);
			}
		}
	}
}

/*unloads the Transporter passed into the mission at the specified x/y
goingHome = true when returning from an off World mission*/
void unloadTransporter(DROID *psTransporter, UDWORD x, UDWORD y, bool goingHome)
{
	DROID		*psDroid, *psNext;
	DROID		**ppCurrentList;
	UDWORD		droidX, droidY;
	UDWORD		iX, iY;
	DROID_GROUP	*psGroup;

	if (goingHome)
	{
		ppCurrentList = mission.apsDroidLists;
	}
	else
	{
		ppCurrentList = apsDroidLists;
	}

	//unload all the droids from within the current Transporter
	if (psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER)
	{
		// reset the transporter cluster
		psTransporter->cluster = 0;
		for (psDroid = psTransporter->psGroup->psList; psDroid != NULL && psDroid != psTransporter; psDroid = psNext)
		{
			psNext = psDroid->psGrpNext;
			//add it back into current droid lists
			addDroid(psDroid, ppCurrentList);

			//starting point...based around the value passed in
			droidX = map_coord(x);
			droidY = map_coord(y);
			if (goingHome)
			{
				//swap the droid and map pointers
				swapMissionPointers();
			}
			if (!pickATileGen(&droidX, &droidY, LOOK_FOR_EMPTY_TILE, zonedPAT))
			{
				ASSERT(false, "unloadTransporter: Unable to find a valid location");
			}
			droidSetPosition(psDroid, world_coord(droidX), world_coord(droidY));
			updateDroidOrientation(psDroid);
			// a commander needs to get it's group back
			if (psDroid->droidType == DROID_COMMAND)
			{
				psGroup = grpCreate();
				psGroup->add(psDroid);
				clearCommandDroidFactory(psDroid);
			}

			//reset droid orders
			orderDroid(psDroid, DORDER_STOP, ModeImmediate);
			psDroid->selected = false;
			if (!bMultiPlayer)
			{
				// So VTOLs don't try to rearm on another map
				setDroidBase(psDroid, NULL);
			}
			psDroid->cluster = 0;
			if (goingHome)
			{
				//swap the droid and map pointers
				swapMissionPointers();
			}
		}

		/* trigger script callback detailing group about to disembark */
		transporterSetScriptCurrent(psTransporter);
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_TRANSPORTER_LANDED);
		triggerEvent(TRIGGER_TRANSPORTER_LANDED, psTransporter);
		transporterSetScriptCurrent(NULL);

		/* remove droids from transporter group if not already transferred to script group */
		for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
		     && psDroid != psTransporter; psDroid = psNext)
		{
			psNext = psDroid->psGrpNext;
			psTransporter->psGroup->remove(psDroid);
		}
	}

	// Don't do this in multiPlayer
	if (!bMultiPlayer)
	{
		// Send all transporter Droids back to home base if off world
		if (!goingHome)
		{
			/* Stop the camera following the transporter */
			psTransporter->selected = false;

			/* Send transporter offworld */
			missionGetTransporterExit(psTransporter->player, &iX, &iY);
			orderDroidLoc(psTransporter, DORDER_TRANSPORTRETURN, iX, iY, ModeImmediate);

			// Set the launch time so the transporter doesn't just disappear for CAMSTART/CAMCHANGE
			transporterSetLaunchTime(gameTime);
		}
	}
}

void missionMoveTransporterOffWorld(DROID *psTransporter)
{
	W_CLICKFORM     *psForm;
	DROID           *psDroid;
	// FIXME: When we convert campaign scripts, use DROID_SUPERTRANSPORTER
	if (psTransporter->droidType == DROID_TRANSPORTER)
	{
		/* trigger script callback */
		transporterSetScriptCurrent(psTransporter);
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_TRANSPORTER_OFFMAP);
		triggerEvent(TRIGGER_TRANSPORTER_EXIT, psTransporter);
		transporterSetScriptCurrent(NULL);

		if (droidRemove(psTransporter, apsDroidLists))
		{
			addDroid(psTransporter, mission.apsDroidLists);
		}

		//stop the droid moving - the moveUpdate happens AFTER the orderUpdate and can cause problems if the droid moves from one tile to another
		moveReallyStopDroid(psTransporter);

		//if offworld mission, then add the timer
		//if (mission.type == LDS_MKEEP || mission.type == LDS_MCLEAR)
		if (missionCanReEnforce() && psTransporter->player == selectedPlayer)
		{
			addTransporterTimerInterface();
			//set the data for the transporter timer label
			widgSetUserData(psWScreen, IDTRANTIMER_DISPLAY, (void *)psTransporter);

			//make sure the button is enabled
			psForm = (W_CLICKFORM *)widgGetFromID(psWScreen, IDTRANTIMER_BUTTON);
			if (psForm)
			{
				formSetClickState(psForm, WCLICK_NORMAL);
			}

		}
		//need a callback for when all the selectedPlayers' reinforcements have been delivered
		if (psTransporter->player == selectedPlayer)
		{
			psDroid = NULL;
			for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
			{
				if (psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)
				{
					break;
				}
			}
			if (psDroid == NULL)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NO_REINFORCEMENTS_LEFT);
				triggerEvent(TRIGGER_TRANSPORTER_DONE, psTransporter);
			}
		}
	}
	else
	{
		debug(LOG_SAVE, "droid type not transporter!");
	}
}


//add the Mission timer into the top  right hand corner of the screen
bool intAddMissionTimer(void)
{
	//check to see if it exists already
	if (widgGetFromID(psWScreen, IDTIMER_FORM) != NULL)
	{
		return true;
	}

	// Add the background
	W_FORMINIT sFormInit;

	sFormInit.formID = 0;
	sFormInit.id = IDTIMER_FORM;
	sFormInit.style = WFORM_PLAIN;

	sFormInit.width = iV_GetImageWidth(IntImages, IMAGE_MISSION_CLOCK); //TIMER_WIDTH;
	sFormInit.height = iV_GetImageHeight(IntImages, IMAGE_MISSION_CLOCK); //TIMER_HEIGHT;
	sFormInit.x = (SWORD)(RADTLX + RADWIDTH - sFormInit.width);
	sFormInit.y = (SWORD)TIMER_Y;
	sFormInit.UserData = PACKDWORD_TRI(0, IMAGE_MISSION_CLOCK, IMAGE_MISSION_CLOCK_UP);;
	sFormInit.pDisplay = intDisplayMissionClock;

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	//add labels for the time display
	W_LABINIT sLabInit;
	sLabInit.formID = IDTIMER_FORM;
	sLabInit.id = IDTIMER_DISPLAY;
	sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInit.x = TIMER_LABELX;
	sLabInit.y = TIMER_LABELY;
	sLabInit.width = sFormInit.width;//TIMER_WIDTH;
	sLabInit.height = sFormInit.height;//TIMER_HEIGHT;
	sLabInit.pText = "00:00:00";
	sLabInit.pCallback = intUpdateMissionTimer;

	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	return true;
}

//add the Transporter timer into the top left hand corner of the screen
bool intAddTransporterTimer(void)
{
	// Make sure that Transporter Launch button isn't up as well
	intRemoveTransporterLaunch();

	//check to see if it exists already
	if (widgGetFromID(psWScreen, IDTRANTIMER_BUTTON) != NULL)
	{
		return true;
	}

	// Add the button form - clickable
	W_FORMINIT sFormInit;
	sFormInit.formID = 0;
	sFormInit.id = IDTRANTIMER_BUTTON;
	sFormInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;
	sFormInit.x = TRAN_FORM_X;
	sFormInit.y = TRAN_FORM_Y;
	sFormInit.width = iV_GetImageWidth(IntImages, IMAGE_TRANSETA_UP);
	sFormInit.height = iV_GetImageHeight(IntImages, IMAGE_TRANSETA_UP);
	sFormInit.pTip = _("Load Transport");
	sFormInit.pDisplay = intDisplayImageHilight;
	sFormInit.UserData = PACKDWORD_TRI(0, IMAGE_TRANSETA_DOWN, IMAGE_TRANSETA_UP);

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	//add labels for the time display
	W_LABINIT sLabInit;
	sLabInit.formID = IDTRANTIMER_BUTTON;
	sLabInit.id = IDTRANTIMER_DISPLAY;
	sLabInit.style = WIDG_HIDDEN;
	sLabInit.x = TRAN_TIMER_X;
	sLabInit.y = TRAN_TIMER_Y;
	sLabInit.width = TRAN_TIMER_WIDTH;
	sLabInit.height = sFormInit.height;
	sLabInit.pCallback = intUpdateTransporterTimer;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	//add the capacity label
	sLabInit = W_LABINIT();
	sLabInit.formID = IDTRANTIMER_BUTTON;
	sLabInit.id = IDTRANS_CAPACITY;
	sLabInit.x = 65;
	sLabInit.y = 1;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "00/10";
	sLabInit.pCallback = intUpdateTransCapacity;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	return true;
}

void missionSetReinforcementTime(UDWORD iTime)
{
	g_iReinforceTime = iTime;
}

UDWORD  missionGetReinforcementTime(void)
{
	return g_iReinforceTime;
}

//fills in a hours(if bHours = true), minutes and seconds display for a given time in 1000th sec
static void fillTimeDisplay(char *psText, UDWORD time, bool bHours)
{
	//this is only for the transporter timer - never have hours!
	if (time == LZ_COMPROMISED_TIME)
	{
		strcpy(psText, "--:--");
	}
	else
	{
		time_t secs = time / GAME_TICKS_PER_SEC;
		struct tm *tmp = localtime(&secs);
		strftime(psText, WIDG_MAXSTR, bHours ? "%H:%M:%S" : "%H:%M", tmp);
	}
}


//update function for the mission timer
void intUpdateMissionTimer(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL		*Label = (W_LABEL *)psWidget;
	UDWORD		timeElapsed;
	SDWORD		timeRemaining;

	// If the cheatTime has been set, then don't want the timer to countdown until stop cheating
	if (mission.cheatTime)
	{
		timeElapsed = mission.cheatTime - mission.startTime;
	}
	else
	{
		timeElapsed = gameTime - mission.startTime;
	}

	if (!challengeActive)
	{
		timeRemaining = mission.time - timeElapsed;
		if (timeRemaining < 0)
		{
			timeRemaining = 0;
		}
	}
	else
	{
		timeRemaining = timeElapsed;
	}

	fillTimeDisplay(Label->aText, timeRemaining, true);
	Label->style &= ~WIDG_HIDDEN;	// Make sure its visible

	if (challengeActive)
	{
		return;	// all done
	}

	//make timer flash if time remaining < 5 minutes
	if (timeRemaining < FIVE_MINUTES)
	{
		flashMissionButton(IDTIMER_FORM);
	}
	//stop timer from flashing when gets to < 4 minutes
	if (timeRemaining < FOUR_MINUTES)
	{
		stopMissionButtonFlash(IDTIMER_FORM);
	}
	//play audio the first time the timed mission is started
	if (timeRemaining && (missionCountDown & NOT_PLAYED_ACTIVATED))
	{
		audio_QueueTrack(ID_SOUND_MISSION_TIMER_ACTIVATED);
		missionCountDown &= ~NOT_PLAYED_ACTIVATED;
	}
	//play some audio for mission countdown - start at 10 minutes remaining
	if (getPlayCountDown() && timeRemaining < TEN_MINUTES)
	{
		if (timeRemaining < TEN_MINUTES && (missionCountDown & NOT_PLAYED_TEN))
		{
			audio_QueueTrack(ID_SOUND_10_MINUTES_REMAINING);
			missionCountDown &= ~NOT_PLAYED_TEN;
		}
		else if (timeRemaining < FIVE_MINUTES && (missionCountDown & NOT_PLAYED_FIVE))
		{
			audio_QueueTrack(ID_SOUND_5_MINUTES_REMAINING);
			missionCountDown &= ~NOT_PLAYED_FIVE;
		}
		else if (timeRemaining < THREE_MINUTES && (missionCountDown & NOT_PLAYED_THREE))
		{
			audio_QueueTrack(ID_SOUND_3_MINUTES_REMAINING);
			missionCountDown &= ~NOT_PLAYED_THREE;
		}
		else if (timeRemaining < TWO_MINUTES && (missionCountDown & NOT_PLAYED_TWO))
		{
			audio_QueueTrack(ID_SOUND_2_MINUTES_REMAINING);
			missionCountDown &= ~NOT_PLAYED_TWO;
		}
		else if (timeRemaining < ONE_MINUTE && (missionCountDown & NOT_PLAYED_ONE))
		{
			audio_QueueTrack(ID_SOUND_1_MINUTE_REMAINING);
			missionCountDown &= ~NOT_PLAYED_ONE;
		}
	}
}

#define	TRANSPORTER_REINFORCE_LEADIN	10*GAME_TICKS_PER_SEC

//update function for the transporter timer
void intUpdateTransporterTimer(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL		*Label = (W_LABEL *)psWidget;
	DROID		*psTransporter;
	SDWORD		timeRemaining;
	SDWORD		ETA;

	ETA = mission.ETA;
	if (ETA < 0)
	{
		ETA = 0;
	}

	// Get the object associated with this widget.
	psTransporter = (DROID *)Label->pUserData;
	if (psTransporter != NULL)
	{
		ASSERT(psTransporter != NULL,
		       "intUpdateTransporterTimer: invalid Droid pointer");

		if (psTransporter->action == DACTION_TRANSPORTIN ||
		    psTransporter->action == DACTION_TRANSPORTWAITTOFLYIN)
		{
			if (mission.ETA == LZ_COMPROMISED_TIME)
			{
				timeRemaining = LZ_COMPROMISED_TIME;
			}
			else
			{
				timeRemaining = mission.ETA - (gameTime - g_iReinforceTime);
				if (timeRemaining < 0)
				{
					timeRemaining = 0;
				}
				if (timeRemaining < TRANSPORTER_REINFORCE_LEADIN)
				{
					// arrived: tell the transporter to move to the new onworld
					// location if not already doing so
					if (psTransporter->action == DACTION_TRANSPORTWAITTOFLYIN)
					{
						missionFlyTransportersIn(selectedPlayer, false);
						eventFireCallbackTrigger((TRIGGER_TYPE)CALL_TRANSPORTER_REINFORCE);
						triggerEvent(TRIGGER_TRANSPORTER_ARRIVED, psTransporter);
					}
				}
			}
			fillTimeDisplay(Label->aText, timeRemaining, false);
		}
		else
		{
			fillTimeDisplay(Label->aText, ETA, false);
		}
	}
	else
	{
		if (missionCanReEnforce())  // ((mission.type == LDS_MKEEP) || (mission.type == LDS_MCLEAR)) & (mission.ETA >= 0) ) {
		{
			fillTimeDisplay(Label->aText, ETA, false);
		}
		else
		{

			fillTimeDisplay(Label->aText, 0, false);

		}
	}

	//minutes
	/*calcTime = timeRemaining / (60*GAME_TICKS_PER_SEC);
	Label->aText[0] = (UBYTE)('0'+ calcTime / 10);
	Label->aText[1] = (UBYTE)('0'+ calcTime % 10);
	timeElapsed -= calcTime * (60*GAME_TICKS_PER_SEC);
	//seperator
	Label->aText[3] = (UBYTE)(':');
	//seconds
	calcTime = timeRemaining / GAME_TICKS_PER_SEC;
	Label->aText[3] = (UBYTE)('0'+ calcTime / 10);
	Label->aText[4] = (UBYTE)('0'+ calcTime % 10);*/

	Label->style &= ~WIDG_HIDDEN;
}

/* Remove the Mission Timer widgets from the screen*/
void intRemoveMissionTimer(void)
{
	// Check it's up.
	if (widgGetFromID(psWScreen, IDTIMER_FORM) != NULL)
	{
		//and remove it.
		widgDelete(psWScreen, IDTIMER_FORM);
	}
}

/* Remove the Transporter Timer widgets from the screen*/
void intRemoveTransporterTimer(void)
{

	//remove main screen
	if (widgGetFromID(psWScreen, IDTRANTIMER_BUTTON) != NULL)
	{
		widgDelete(psWScreen, IDTRANTIMER_BUTTON);
	}
}



// ////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////
// mission result functions for the interface.



static void intDisplayMissionBackDrop(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	scoreDataToScreen();
}

static void missionResetInGameState(void)
{
	//stop the game if in single player mode
	setMissionPauseState();

	// reset the input state
	resetInput();

	// Add the background
	// get rid of reticule etc..
	intResetScreen(false);
	forceHidePowerBar();
	intRemoveReticule();
	intRemoveMissionTimer();
}

static bool _intAddMissionResult(bool result, bool bPlaySuccess)
{
	missionResetInGameState();

	W_FORMINIT sFormInit;

	// add some funky beats
	cdAudio_PlayTrack(SONG_FRONTEND);

	pie_LoadBackDrop(SCREEN_MISSIONEND);

	sFormInit.formID		= 0;
	sFormInit.id			= IDMISSIONRES_BACKFORM;
	sFormInit.style			= WFORM_PLAIN;
	sFormInit.x				= (SWORD)(0 + D_W);
	sFormInit.y				= (SWORD)(0 + D_H);
	sFormInit.width			= 640;
	sFormInit.height		= 480;
	sFormInit.pDisplay		= intDisplayMissionBackDrop;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// TITLE
	sFormInit.formID		= IDMISSIONRES_BACKFORM;

	sFormInit.id			= IDMISSIONRES_TITLE;
	sFormInit.style			= WFORM_PLAIN;
	sFormInit.x				= MISSIONRES_TITLE_X;
	sFormInit.y				= MISSIONRES_TITLE_Y;
	sFormInit.width			= MISSIONRES_TITLE_W;
	sFormInit.height		= MISSIONRES_TITLE_H;
	sFormInit.disableChildren = true;
	sFormInit.pDisplay		= intOpenPlainForm;	//intDisplayPlainForm;

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// add form
	sFormInit.formID		= IDMISSIONRES_BACKFORM;

	sFormInit.id			= IDMISSIONRES_FORM;
	sFormInit.style			= WFORM_PLAIN;
	sFormInit.x				= MISSIONRES_X;
	sFormInit.y				= MISSIONRES_Y;
	sFormInit.width			= MISSIONRES_W;
	sFormInit.height		= MISSIONRES_H;
	sFormInit.disableChildren = true;
	sFormInit.pDisplay		= intOpenPlainForm;	//intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// description of success/fail
	W_LABINIT sLabInit;
	sLabInit.formID = IDMISSIONRES_TITLE;
	sLabInit.id = IDMISSIONRES_TXT;
	sLabInit.style = WLAB_ALIGNCENTRE;
	sLabInit.x = 0;
	sLabInit.y = 12;
	sLabInit.width = MISSIONRES_TITLE_W;
	sLabInit.height = 16;
	if (result)
	{

		//don't bother adding the text if haven't played the audio
		if (bPlaySuccess)
		{
			sLabInit.pText = Cheated ? _("OBJECTIVE ACHIEVED by cheating!") : _("OBJECTIVE ACHIEVED");//"Objective Achieved";
		}

	}
	else
	{
		sLabInit.pText = Cheated ? _("OBJECTIVE FAILED--and you cheated!") : _("OBJECTIVE FAILED"); //"Objective Failed;
	}
	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}
	// options.
	W_BUTINIT sButInit;
	sButInit.formID		= IDMISSIONRES_FORM;
	sButInit.style		= WBUT_TXTCENTRE;
	sButInit.width		= MISSION_TEXT_W;
	sButInit.height		= MISSION_TEXT_H;
	sButInit.pTip		= NULL;
	sButInit.pDisplay	= displayTextOption;
	// If won or in debug mode
	if (result || getDebugMappingStatus() || bMultiPlayer)
	{
		//continue
		sButInit.x			= MISSION_2_X;
		// Won the game, so display "Quit to main menu"
		if (testPlayerHasWon() && !bMultiPlayer)
		{
			sButInit.id			= IDMISSIONRES_QUIT;
			sButInit.y			= MISSION_2_Y - 8;
			sButInit.pText		= _("Quit To Main Menu");
			widgAddButton(psWScreen, &sButInit);
		}
		else
		{
			// Finished the mission, so display "Continue Game"
			sButInit.y			= MISSION_2_Y;
			sButInit.id			= IDMISSIONRES_CONTINUE;
			sButInit.pText		= _("Continue Game");//"Continue Game";
			widgAddButton(psWScreen, &sButInit);
		}

		// FIXME, We got serious issues with savegames at the *END* of some missions, and while they
		// will load, they don't have the correct state information or other settings.
		// See transition from CAM2->CAM3 for a example.
		/* Only add save option if in the game for real, ie, not fastplay.
		* And the player hasn't just completed the whole game
		* Don't add save option if just lost and in debug mode.
		if (!bMultiPlayer && !testPlayerHasWon() && !(testPlayerHasLost() && getDebugMappingStatus()))
		{
			//save
			sButInit.id			= IDMISSIONRES_SAVE;
			sButInit.x			= MISSION_1_X;
			sButInit.y			= MISSION_1_Y;
			sButInit.pText		= _("Save Game");//"Save Game";
			widgAddButton(psWScreen, &sButInit);

			// automatically save the game to be able to restart a mission
			saveGame((char *)"savegames/Autosave.gam", GTYPE_SAVE_START);
		}
		*/
	}
	else
	{
		//load
		sButInit.id			= IDMISSIONRES_LOAD;
		sButInit.x			= MISSION_1_X;
		sButInit.y			= MISSION_1_Y;
		sButInit.pText		= _("Load Saved Game");//"Load Saved Game";
		widgAddButton(psWScreen, &sButInit);
		//quit
		sButInit.id			= IDMISSIONRES_QUIT;
		sButInit.x			= MISSION_2_X;
		sButInit.y			= MISSION_2_Y;
		sButInit.pText		= _("Quit To Main Menu");//"Quit to Main Menu";
		widgAddButton(psWScreen, &sButInit);
	}

	intMode		= INT_MISSIONRES;
	MissionResUp = true;

	/* play result audio */
	if (result == true && bPlaySuccess)
	{
		audio_QueueTrack(ID_SOUND_OBJECTIVE_ACCOMPLISHED);
	}

	return true;
}


bool intAddMissionResult(bool result, bool bPlaySuccess)
{
	return _intAddMissionResult(result, bPlaySuccess);
}

void intRemoveMissionResultNoAnim(void)
{
	widgDelete(psWScreen, IDMISSIONRES_TITLE);
	widgDelete(psWScreen, IDMISSIONRES_FORM);
	widgDelete(psWScreen, IDMISSIONRES_BACKFORM);

	cdAudio_Stop();

	MissionResUp	 	= false;
	intMode				= INT_NORMAL;

	//reset the pauses
	resetMissionPauseState();

	// add back the reticule and power bar.
	intAddReticule();

	intShowPowerBar();
}

void intRunMissionResult(void)
{
	wzSetCursor(CURSOR_DEFAULT);

	if (bLoadSaveUp)
	{
		if (runLoadSave(false)) // check for file name.
		{
			if (strlen(sRequestResult))
			{
				debug(LOG_SAVE, "Returned %s", sRequestResult);

				if (!bRequestLoad)
				{
					char msg[256] = {'\0'};

					saveGame(sRequestResult, GTYPE_SAVE_START);
					sstrcpy(msg, _("GAME SAVED :"));
					sstrcat(msg, sRequestResult);
					addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);
				}
			}
		}
	}
}

static void missionContineButtonPressed(void)
{
	if (nextMissionType == LDS_CAMSTART
	    || nextMissionType == LDS_BETWEEN
	    || nextMissionType == LDS_EXPAND
	    || nextMissionType == LDS_EXPAND_LIMBO)
	{
		launchMission();
	}
	widgDelete(psWScreen, IDMISSIONRES_FORM);	//close option box.

	if (bMultiPlayer)
	{
		intRemoveMissionResultNoAnim();
	}
}

void intProcessMissionResult(UDWORD id)
{
	switch (id)
	{
	case IDMISSIONRES_LOAD:
		// throw up some filerequester
		addLoadSave(LOAD_MISSIONEND, _("Load Saved Game"));
		break;
	case IDMISSIONRES_SAVE:
		addLoadSave(SAVE_MISSIONEND, _("Save Game"));
		if (widgGetFromID(psWScreen, IDMISSIONRES_QUIT) == NULL)
		{
			//Add Quit Button now save has been pressed
			W_BUTINIT sButInit;
			sButInit.formID		= IDMISSIONRES_FORM;
			sButInit.style		= WBUT_TXTCENTRE;
			sButInit.width		= MISSION_TEXT_W;
			sButInit.height		= MISSION_TEXT_H;
			sButInit.pDisplay	= displayTextOption;
			sButInit.id			= IDMISSIONRES_QUIT;
			sButInit.x			= MISSION_3_X;
			sButInit.y			= MISSION_3_Y;
			sButInit.pText		= _("Quit To Main Menu");
			widgAddButton(psWScreen, &sButInit);
		}
		break;
	case IDMISSIONRES_QUIT:
		// catered for by hci.c.
		break;
	case IDMISSIONRES_CONTINUE:
		if (bLoadSaveUp)
		{
			closeLoadSave();				// close save interface if it's up.
		}
		missionContineButtonPressed();
		break;

	default:
		break;
	}
}

// end of interface stuff.
// ////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////

/*builds a droid back at the home base whilst on a mission - stored in a list made
available to the transporter interface*/
DROID *buildMissionDroid(DROID_TEMPLATE *psTempl, UDWORD x, UDWORD y,
        UDWORD player)
{
	DROID		*psNewDroid;

	psNewDroid = buildDroid(psTempl, world_coord(x), world_coord(y), player, true, NULL);
	if (!psNewDroid)
	{
		return NULL;
	}
	addDroid(psNewDroid, mission.apsDroidLists);
	//set its x/y to impossible values so can detect when return from mission
	psNewDroid->pos.x = INVALID_XY;
	psNewDroid->pos.y = INVALID_XY;

	//set all the droids to selected from when return
	psNewDroid->selected = true;

	// Set died parameter correctly
	psNewDroid->died = NOT_CURRENT_LIST;

	return psNewDroid;
}

//this causes the new mission data to be loaded up - only if startMission has been called
void launchMission(void)
{
	//if (mission.type == MISSION_NONE)
	if (mission.type == LDS_NONE)
	{
		// tell the loop that a new level has to be loaded up
		loopMissionState = LMS_NEWLEVEL;
	}
	else
	{
		debug(LOG_SAVE, "Start Mission has not been called");
	}
}


//sets up the game to start a new mission
bool setUpMission(UDWORD type)
{
	// Close the interface
	intResetScreen(true);

	/* The last mission must have been successful otherwise endgame would have been called */
	endMission();

	//release the level data for the previous mission
	if (!levReleaseMissionData())
	{
		return false;
	}

	if (type == LDS_CAMSTART)
	{
		// Another one of those lovely hacks!!
		bool    bPlaySuccess = true;

		// We don't want the 'mission accomplished' audio/text message at end of cam1
		if (getCampaignNumber() == 2)
		{
			bPlaySuccess = false;
		}
		// Give the option of save/continue
		if (!intAddMissionResult(true, bPlaySuccess))
		{
			return false;
		}
		loopMissionState = LMS_SAVECONTINUE;
	}
	else if (type == LDS_MKEEP
	        || type == LDS_MCLEAR
	        || type == LDS_MKEEP_LIMBO)
	{
		launchMission();
	}
	else
	{
		if (!getWidgetsStatus())
		{
			setWidgetsStatus(true);
			intResetScreen(false);
		}

		// Give the option of save / continue
		if (!intAddMissionResult(true, true))
		{
			return false;
		}
		loopMissionState = LMS_SAVECONTINUE;
	}

	return true;
}

//save the power settings before loading in the new map data
void saveMissionPower(void)
{
	UDWORD	inc;

	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		mission.asCurrentPower[inc] = getPower(inc);
	}
}

//add the power from the home base to the current power levels for the mission map
void adjustMissionPower(void)
{
	UDWORD	inc;

	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		addPower(inc, mission.asCurrentPower[inc]);
	}
}

/*sets the appropriate pause states for when the interface is up but the
game needs to be paused*/
void setMissionPauseState(void)
{
	if (!bMultiPlayer)
	{
		gameTimeStop();
		setGameUpdatePause(true);
		setAudioPause(true);
		setScriptPause(true);
		setConsolePause(true);
	}
}

/*resets the pause states */
void resetMissionPauseState(void)
{
	if (!bMultiPlayer)
	{
		setGameUpdatePause(false);
		setAudioPause(false);
		setScriptPause(false);
		setConsolePause(false);
		gameTimeStart();
	}
}

//gets the coords for a no go area
LANDING_ZONE *getLandingZone(SDWORD i)
{
	ASSERT((i >= 0) && (i < MAX_NOGO_AREAS), "getLandingZone out of range.");
	return &sLandingZone[i];
}

/*Initialises all the nogo areas to 0 - DOESN'T INIT THE LIMBO AREA because we
have to set this up in the mission BEFORE*/
void initNoGoAreas(void)
{
	UBYTE	i;

	for (i = 0; i < MAX_NOGO_AREAS; i++)
	{
		if (i != LIMBO_LANDING)
		{
			sLandingZone[i].x1 = sLandingZone[i].y1 = sLandingZone[i].x2 =
			        sLandingZone[i].y2 = 0;
		}
	}
}

//sets the coords for a no go area
void setNoGoArea(UBYTE x1, UBYTE y1, UBYTE x2, UBYTE y2, UBYTE area)
{
	// make sure that x2 > x1 and y2 > y1
	if (x2 < x1)
	{
		std::swap(x1, x2);
	}
	if (y2 < y1)
	{
		std::swap(y1, y2);
	}

	sLandingZone[area].x1 = x1;
	sLandingZone[area].x2 = x2;
	sLandingZone[area].y1 = y1;
	sLandingZone[area].y2 = y2;

	if (area == 0 && x1 && y1)
	{
		addLandingLights(getLandingX(area) + 64, getLandingY(area) + 64);
	}
}

static inline void addLandingLight(int x, int y, LAND_LIGHT_SPEC spec, bool lit)
{
	// The height the landing lights should be above the ground
	static const unsigned int AboveGround = 16;
	Vector3i pos;

	if (x < 0 || y < 0)
	{
		return;
	}

	pos.x = x;
	pos.z = y;
	pos.y = map_Height(x, y) + AboveGround;

	effectSetLandLightSpec(spec);

	addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LAND_LIGHT, false, NULL, lit);
}

static void addLandingLights(UDWORD x, UDWORD y)
{
	addLandingLight(x, y, LL_MIDDLE, true);                 // middle

	addLandingLight(x + 128, y + 128, LL_OUTER, false);     // outer
	addLandingLight(x + 128, y - 128, LL_OUTER, false);
	addLandingLight(x - 128, y + 128, LL_OUTER, false);
	addLandingLight(x - 128, y - 128, LL_OUTER, false);

	addLandingLight(x + 64, y + 64, LL_INNER, false);       // inner
	addLandingLight(x + 64, y - 64, LL_INNER, false);
	addLandingLight(x - 64, y + 64, LL_INNER, false);
	addLandingLight(x - 64, y - 64, LL_INNER, false);
}

/*	checks the x,y passed in are not within the boundary of any Landing Zone
	x and y in tile coords*/
bool withinLandingZone(UDWORD x, UDWORD y)
{
	UDWORD		inc;

	ASSERT(x < mapWidth, "withinLandingZone: x coord bigger than mapWidth");
	ASSERT(y < mapHeight, "withinLandingZone: y coord bigger than mapHeight");


	for (inc = 0; inc < MAX_NOGO_AREAS; inc++)
	{
		if ((x >= (UDWORD)sLandingZone[inc].x1 && x <= (UDWORD)sLandingZone[inc].x2) &&
		    (y >= (UDWORD)sLandingZone[inc].y1 && y <= (UDWORD)sLandingZone[inc].y2))
		{
			return true;
		}
	}
	return false;
}

//returns the x coord for where the Transporter can land (for player 0)
UWORD getLandingX(SDWORD iPlayer)
{
	ASSERT_OR_RETURN(0, iPlayer < MAX_NOGO_AREAS, "getLandingX: player %d out of range", iPlayer);
	return (UWORD)world_coord((sLandingZone[iPlayer].x1 + (sLandingZone[iPlayer].x2 -
	        sLandingZone[iPlayer].x1) / 2));
}

//returns the y coord for where the Transporter can land
UWORD getLandingY(SDWORD iPlayer)
{
	ASSERT_OR_RETURN(0, iPlayer < MAX_NOGO_AREAS, "getLandingY: player %d out of range", iPlayer);
	return (UWORD)world_coord((sLandingZone[iPlayer].y1 + (sLandingZone[iPlayer].y2 -
	        sLandingZone[iPlayer].y1) / 2));
}

//returns the x coord for where the Transporter can land back at home base
UDWORD getHomeLandingX(void)
{
	return map_coord(mission.homeLZ_X);
}

//returns the y coord for where the Transporter can land back at home base
UDWORD getHomeLandingY(void)
{
	return map_coord(mission.homeLZ_Y);
}

void missionSetTransporterEntry(SDWORD iPlayer, SDWORD iEntryTileX, SDWORD iEntryTileY)
{
	ASSERT(iPlayer < MAX_PLAYERS, "missionSetTransporterEntry: player %i too high", iPlayer);

	if ((iEntryTileX > scrollMinX) && (iEntryTileX < scrollMaxX))
	{
		mission.iTranspEntryTileX[iPlayer] = (UWORD) iEntryTileX;
	}
	else
	{
		debug(LOG_SAVE, "entry point x %i outside scroll limits %i->%i", iEntryTileX, scrollMinX, scrollMaxX);
		mission.iTranspEntryTileX[iPlayer] = (UWORD)(scrollMinX + EDGE_SIZE);
	}

	if ((iEntryTileY > scrollMinY) && (iEntryTileY < scrollMaxY))
	{
		mission.iTranspEntryTileY[iPlayer] = (UWORD) iEntryTileY;
	}
	else
	{
		debug(LOG_SAVE, "entry point y %i outside scroll limits %i->%i", iEntryTileY, scrollMinY, scrollMaxY);
		mission.iTranspEntryTileY[iPlayer] = (UWORD)(scrollMinY + EDGE_SIZE);
	}
}

void missionSetTransporterExit(SDWORD iPlayer, SDWORD iExitTileX, SDWORD iExitTileY)
{
	ASSERT(iPlayer < MAX_PLAYERS, "missionSetTransporterExit: player %i too high", iPlayer);

	if ((iExitTileX > scrollMinX) && (iExitTileX < scrollMaxX))
	{
		mission.iTranspExitTileX[iPlayer] = (UWORD) iExitTileX;
	}
	else
	{
		debug(LOG_SAVE, "entry point x %i outside scroll limits %i->%i", iExitTileX, scrollMinX, scrollMaxX);
		mission.iTranspExitTileX[iPlayer] = (UWORD)(scrollMinX + EDGE_SIZE);
	}

	if ((iExitTileY > scrollMinY) && (iExitTileY < scrollMaxY))
	{
		mission.iTranspExitTileY[iPlayer] = (UWORD) iExitTileY;
	}
	else
	{
		debug(LOG_SAVE, "entry point y %i outside scroll limits %i->%i", iExitTileY, scrollMinY, scrollMaxY);
		mission.iTranspExitTileY[iPlayer] = (UWORD)(scrollMinY + EDGE_SIZE);
	}
}

void missionGetTransporterEntry(SDWORD iPlayer, UWORD *iX, UWORD *iY)
{
	ASSERT(iPlayer < MAX_PLAYERS, "missionGetTransporterEntry: player %i too high", iPlayer);

	*iX = (UWORD) world_coord(mission.iTranspEntryTileX[iPlayer]);
	*iY = (UWORD) world_coord(mission.iTranspEntryTileY[iPlayer]);
}

void missionGetTransporterExit(SDWORD iPlayer, UDWORD *iX, UDWORD *iY)
{
	ASSERT(iPlayer < MAX_PLAYERS, "missionGetTransporterExit: player %i too high", iPlayer);

	*iX = world_coord(mission.iTranspExitTileX[iPlayer]);
	*iY = world_coord(mission.iTranspExitTileY[iPlayer]);
}

/*update routine for mission details */
void missionTimerUpdate(void)
{
	//don't bother with the time check if have 'cheated'
	if (!mission.cheatTime)
	{
		//Want a mission timer on all types of missions now - AB 26/01/99
		//only interested in off world missions (so far!) and if timer has been set
		if (mission.time >= 0)  //&& (
			//mission.type == LDS_MKEEP || mission.type == LDS_MKEEP_LIMBO ||
			//mission.type == LDS_MCLEAR || mission.type == LDS_BETWEEN))
		{
			//check if time is up
			if ((SDWORD)(gameTime - mission.startTime) > mission.time)
			{
				//the script can call the end game cos have failed!
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MISSION_TIME);
				triggerEvent(TRIGGER_MISSION_TIMEOUT);
			}
		}
	}
}


// Remove any objects left ie walls,structures and droids that are not the selected player.
//
void missionDestroyObjects(void)
{
	DROID *psDroid;
	STRUCTURE *psStruct;
	UBYTE Player, i;

	debug(LOG_SAVE, "called");
	proj_FreeAllProjectiles();
	for (Player = 0; Player < MAX_PLAYERS; Player++)
	{
		if (Player != selectedPlayer)
		{
			// AI player, clear out old data

			psDroid = apsDroidLists[Player];

			while (psDroid != NULL)
			{
				DROID *psNext = psDroid->psNext;
				removeDroidBase(psDroid);
				psDroid = psNext;
			}

			//clear out the mission lists as well to make sure no Tranporters exist
			apsDroidLists[Player] = mission.apsDroidLists[Player];
			psDroid = apsDroidLists[Player];

			while (psDroid != NULL)
			{
				DROID *psNext = psDroid->psNext;

				//make sure its died flag is not set since we've swapped the apsDroidList pointers over
				psDroid->died = false;
				removeDroidBase(psDroid);
				psDroid = psNext;
			}
			mission.apsDroidLists[Player] = NULL;

			psStruct = apsStructLists[Player];

			while (psStruct != NULL)
			{
				STRUCTURE *psNext = psStruct->psNext;
				removeStruct(psStruct, true);
				psStruct = psNext;
			}
		}
	}

	// human player, check that we do not reference the cleared out data
	Player = selectedPlayer;

	psDroid = apsDroidLists[Player];
	while (psDroid != NULL)
	{

		if (psDroid->psBaseStruct && psDroid->psBaseStruct->died)
		{
			setDroidBase(psDroid, NULL);
		}
		for (i = 0; i < DROID_MAXWEAPS; i++)
		{
			if (psDroid->psActionTarget[i] && psDroid->psActionTarget[i]->died)
			{
				setDroidActionTarget(psDroid, NULL, i);
				// Clear action too if this requires a valid first action target
				if (i == 0
				    && psDroid->action != DACTION_MOVEFIRE
				    && psDroid->action != DACTION_TRANSPORTIN
				    && psDroid->action != DACTION_TRANSPORTOUT)
				{
					psDroid->action = DACTION_NONE;
				}
			}
		}
		if (psDroid->order.psObj && psDroid->order.psObj->died)
		{
			setDroidTarget(psDroid, NULL);
		}
		psDroid = psDroid->psNext;
	}

	psStruct = apsStructLists[Player];
	while (psStruct != NULL)
	{
		for (i = 0; i < STRUCT_MAXWEAPS; i++)
		{
			if (psStruct->psTarget[i] && psStruct->psTarget[i]->died)
			{
				setStructureTarget(psStruct, NULL, i, ORIGIN_UNKNOWN);
			}
		}
		psStruct = psStruct->psNext;
	}

	// FIXME: check that orders do not reference anything bad?

	gameTime++;	// Wonderful hack to ensure objects destroyed above get free'ed up by objmemUpdate.
	objmemUpdate();	// Actually free objects removed above
}

void processPreviousCampDroids(void)
{
	DROID           *psDroid, *psNext;

	// See if any are left
	if (mission.apsDroidLists[selectedPlayer])
	{
		for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psNext)
		{
			psNext = psDroid->psNext;
			// We want to kill off all droids now! - AB 27/01/99
			// KILL OFF TRANSPORTER
			if (droidRemove(psDroid, mission.apsDroidLists))
			{
				addDroid(psDroid, apsDroidLists);
				vanishDroid(psDroid);
			}
		}
	}
}

//access functions for droidsToSafety flag - so we don't have to end the mission when a Transporter fly's off world
void setDroidsToSafetyFlag(bool set)
{
	bDroidsToSafety = set;
}

bool getDroidsToSafetyFlag(void)
{
	return bDroidsToSafety;
}


//access functions for bPlayCountDown flag - true = play coded mission count down
void setPlayCountDown(UBYTE set)
{
	bPlayCountDown = set;
}

bool getPlayCountDown(void)
{
	return bPlayCountDown;
}


//checks to see if the player has any droids (except Transporters left)
bool missionDroidsRemaining(UDWORD player)
{
	for (DROID *psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if (psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)
		{
			return true;
		}
	}
	return false;
}

/*called when a Transporter gets to the edge of the world and the droids are
being flown to safety. The droids inside the Transporter are placed into the
mission list for later use*/
void moveDroidsToSafety(DROID *psTransporter)
{
	DROID       *psDroid, *psNext;

	ASSERT((psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER), "unit not a Transporter");

	//move droids out of Transporter into mission list
	for (psDroid = psTransporter->psGroup->psList; psDroid != NULL && psDroid != psTransporter; psDroid = psNext)
	{
		psNext = psDroid->psGrpNext;
		psTransporter->psGroup->remove(psDroid);
		//cam change add droid
		psDroid->pos.x = INVALID_XY;
		psDroid->pos.y = INVALID_XY;
		addDroid(psDroid, mission.apsDroidLists);
	}
	//move the transporter into the mission list also
	if (droidRemove(psTransporter, apsDroidLists))
	{
		addDroid(psTransporter, mission.apsDroidLists);
	}
}

void clearMissionWidgets(void)
{
	//remove any widgets that are up due to the missions
	if (mission.time > 0)
	{
		intRemoveMissionTimer();
	}

	if (missionCanReEnforce())
	{
		intRemoveTransporterTimer();
	}

	intRemoveTransporterLaunch();
}

void resetMissionWidgets(void)
{
	DROID       *psDroid;

	//add back any widgets that should be up due to the missions
	if (mission.time > 0)
	{
		intAddMissionTimer();
		//make sure its not flashing when added
		stopMissionButtonFlash(IDTIMER_FORM);
	}

	if (missionCanReEnforce())
	{
		addTransporterTimerInterface();
	}
	//check not a typical reinforcement mission
	else if (!missionForReInforcements())
	{
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
			{
				intAddTransporterLaunch(psDroid);
				break;
			}
		}
		/*if we got to the end without adding a transporter - there might be
		one sitting in the mission list which is waiting to come back in*/
		if (!psDroid)
		{
			for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
			{
				if ((psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER) &&
				    psDroid->action == DACTION_TRANSPORTWAITTOFLYIN)
				{
					intAddTransporterLaunch(psDroid);
					break;
				}
			}
		}
	}

}

void	setCampaignNumber(UDWORD number)
{
	ASSERT(number < 4, "Campaign Number too high!");
	camNumber = number;
}

UDWORD	getCampaignNumber(void)
{
	return(camNumber);
}

/*deals with any selectedPlayer's transporters that are flying in when the
mission ends. bOffWorld is true if the Mission is currenly offWorld*/
void emptyTransporters(bool bOffWorld)
{
	DROID       *psTransporter, *psDroid, *psNext, *psNextTrans;

	//see if there are any Transporters in the world
	for (psTransporter = apsDroidLists[selectedPlayer]; psTransporter != NULL; psTransporter = psNextTrans)
	{
		psNextTrans = psTransporter->psNext;
		if (psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER)
		{
			//if flying in, empty the contents
			if (orderState(psTransporter, DORDER_TRANSPORTIN))
			{
				/* if we're offWorld, all we need to do is put the droids into the apsDroidList
				and processMission() will assign them a location etc */
				if (bOffWorld)
				{
					for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
					     && psDroid != psTransporter; psDroid = psNext)
					{
						psNext = psDroid->psGrpNext;
						//take it out of the Transporter group
						psTransporter->psGroup->remove(psDroid);
						//add it back into current droid lists
						addDroid(psDroid, apsDroidLists);
					}
				}
				/* we're not offWorld so add to mission.apsDroidList to be
				processed by the endMission function */
				else
				{
					for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
					     && psDroid != psTransporter; psDroid = psNext)
					{
						psNext = psDroid->psGrpNext;
						//take it out of the Transporter group
						psTransporter->psGroup->remove(psDroid);
						//add it back into current droid lists
						addDroid(psDroid, mission.apsDroidLists);
					}
				}
				//now kill off the Transporter
				vanishDroid(psDroid);
			}
		}
	}
	//deal with any transporters that are waiting to come over
	for (psTransporter = mission.apsDroidLists[selectedPlayer]; psTransporter !=
	     NULL; psTransporter = psTransporter->psNext)
	{
		if (psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER)
		{
			//for each droid within the transporter...
			for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
			     && psDroid != psTransporter; psDroid = psNext)
			{
				psNext = psDroid->psGrpNext;
				//take it out of the Transporter group
				psTransporter->psGroup->remove(psDroid);
				//add it back into mission droid lists
				addDroid(psDroid, mission.apsDroidLists);
			}
		}
		//don't need to destory the transporter here - it is dealt with by the endMission process
	}
}

/*bCheating = true == start of cheat
bCheating = false == end of cheat */
void setMissionCheatTime(bool bCheating)
{
	if (bCheating)
	{
		mission.cheatTime = gameTime;
	}
	else
	{
		//adjust the mission start time for the duration of the cheat!
		mission.startTime += gameTime - mission.cheatTime;
		mission.cheatTime = 0;
	}
}
