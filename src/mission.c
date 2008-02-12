/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*
 * mission.c
 *
 * all the stuff relevant to a mission
 */
#include <stdio.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/ivis_common/textdraw.h"
#include "mission.h"
#include "lib/gamelib/gtime.h"
#include "game.h"
#include "projectile.h"
#include "power.h"
#include "structure.h"
#include "message.h"
#include "research.h"
#include "hci.h"
#include "order.h"
#include "action.h"
#include "display3d.h"
#include "lib/ivis_common/piestate.h"
#include "effects.h"
#include "radar.h"
#include "resource.h"		// for mousecursors
#include "transporter.h"
#include "group.h"
#include "frontend.h"		// for displaytextoption.
#include "intdisplay.h"
#include "main.h"
#include "display.h"
#include "loadsave.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "cmddroid.h"
#include "warcam.h"
#include "wrappers.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "console.h"
#include "droid.h"

#include "data.h"
#include "multiplay.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "environ.h"

#include "loop.h"
#include "levels.h"
#include "visibility.h"
#include "mapgrid.h"
#include "cluster.h"
#include "gateway.h"
#include "selection.h"
#include "scores.h"
#include "keymap.h"
#include "lib/ivis_common/bitimage.h"

#include "lib/sound/cdaudio.h"

//#include "texture.h"	   // ffs

#include "texture.h"


//DEFINES**************
//#define		IDTIMER_FORM			11000		// has to be in the header..boohoo
//#define		IDTIMER_DISPLAY			11001
//#define		IDMISSIONRES_FORM		11002		// has to be in the header..boohoo

#define		IDMISSIONRES_TXT		11004
#define     IDMISSIONRES_LOAD		11005
//#define     IDMISSIONRES_SAVE		11006
//#define     IDMISSIONRES_QUIT		11007			//has to be in the header. bummer.
#define     IDMISSIONRES_CONTINUE	11008

//#define		IDTRANSTIMER_FORM		11009		//form for transporter timer
//#define		IDTRANTIMER_DISPLAY		11010		//timer display for transporter timer
//#define		IDTRANTIMER_BUTTON		11012		//transporter button on timer display

#define		IDMISSIONRES_BACKFORM	11013
#define		IDMISSIONRES_TITLE		11014

/*mission timer position*/


#define		TIMER_X					(568 + E_W)
//#define		TIMER_Y					22
//#define		TIMER_WIDTH				38
//#define		TIMER_HEIGHT			20
#define		TIMER_LABELX			15
#define		TIMER_LABELY			0
/*Transporter Timer form position */
#define		TRAN_FORM_X				STAT_X
#define		TRAN_FORM_Y				TIMER_Y
#define		TRAN_FORM_WIDTH			75
#define		TRAN_FORM_HEIGHT		25



/*Transporter Timer position */
#define		TRAN_TIMER_X			4
#define		TRAN_TIMER_Y			TIMER_LABELY
#define		TRAN_TIMER_WIDTH		25
//#define		TRAN_TIMER_HEIGHT		TRAN_FORM_HEIGHT
/*Transporter Timer button position */
//#define		TRAN_BUTTON_X			(TRAN_TIMER_X + TRAN_TIMER_WIDTH + TRAN_TIMER_X)
//#define		TRAN_BUTTON_Y			TRAN_TIMER_Y
//#define		TRAN_BUTTON_WIDTH		20
//#define		TRAN_BUTTON_HEIGHT		20





#define		MISSION_1_X				5						// pos of text options in box.
#define		MISSION_1_Y				15
#define		MISSION_2_X				5
#define		MISSION_2_Y				35
#define		MISSION_3_X				5
#define		MISSION_3_Y				55

#define		MISSION_TEXT_W			MISSIONRES_W-10
#define		MISSION_TEXT_H			16



//these are used for mission countdown

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

BOOL		offWorldKeepLists;

// Set by scrFlyInTransporter. True if were currenly tracking the transporter.
BOOL		bTrackingTransporter = FALSE;

/*lists of droids that are held seperate over several missions. There should
only be selectedPlayer's droids but have possibility for MAX_PLAYERS -
also saves writing out list functions to cater for just one player*/
DROID       *apsLimboDroids[MAX_PLAYERS];


/**********TEST************/
//static  UDWORD      addCount = 0;

//STATICS***************
//Where the Transporter lands for player 0 (sLandingZone[0]), and the rest are
//a list of areas that cannot be built on, used for landing the enemy transporters
static LANDING_ZONE		sLandingZone[MAX_NOGO_AREAS];

//flag to indicate when the droids in a Transporter are flown to safety and not the next mission
static BOOL             bDroidsToSafety;

/* mission result holder */
static BOOL				g_bMissionResult;

// return positions for vtols
Vector2i asVTOLReturnPos[MAX_PLAYERS];


static UBYTE   missionCountDown;
//flag to indicate whether the coded mission countdown is played
static UBYTE   bPlayCountDown;


//FUNCTIONS**************
static void addLandingLights( UDWORD x, UDWORD y);
static BOOL startMissionOffClear(char *pGame);
static BOOL startMissionOffKeep(char *pGame);
static BOOL startMissionCampaignStart(char *pGame);
static BOOL startMissionCampaignChange(char *pGame);
static BOOL startMissionCampaignExpand(char *pGame);
static BOOL startMissionCampaignExpandLimbo(char *pGame);
static BOOL startMissionBetween(void);
static void endMissionCamChange(void);
static void endMissionOffClear(void);
static void endMissionOffKeep(void);
static void endMissionOffKeepLimbo(void);
static void endMissionExpandLimbo(void);

static void saveMissionData(void);
static void restoreMissionData(void);
static void saveCampaignData(void);
static void aiUpdateMissionStructure(STRUCTURE *psStructure);
static void missionResetDroids(void);
static void saveMissionLimboData(void);
static void restoreMissionLimboData(void);
static void processMissionLimbo(void);

static void intUpdateMissionTimer(WIDGET *psWidget, W_CONTEXT *psContext);
static BOOL intAddMissionTimer(void);
static void intUpdateTransporterTimer(WIDGET *psWidget, W_CONTEXT *psContext);
static void adjustMissionPower(void);
static void saveMissionPower(void);
static UDWORD getHomeLandingX(void);
static UDWORD getHomeLandingY(void);
void swapMissionPointers(void);
static void fillTimeDisplay(char	*psText, UDWORD time, BOOL bHours);
static void processPreviousCampDroids(void);
static BOOL intAddTransporterTimer(void);
static void clearCampaignUnits(void);

static void emptyTransporters(BOOL bOffWorld);

//result screen functions
BOOL intAddMissionResult(BOOL result, BOOL bPlaySucess);
//void intRemoveMissionResult			(void);
//void intRemoveMissionResultNoAnim	(void);
//void intRunMissionResult			(void);
//void intProcessMissionResult		(UDWORD id);

BOOL MissionResUp		= FALSE;
BOOL ClosingMissionRes	= FALSE;

static SDWORD		g_iReinforceTime = 0;
//static DROID_GROUP	*g_CurrentScriptGroup = NULL;

/* Which campaign are we dealing with? */
static	UDWORD	camNumber = 1;

//static iSprite *pMissionBackDrop; //pointer to backdrop piccy





//returns TRUE if on an off world mission
BOOL missionIsOffworld(void)
{
	return ((mission.type == LDS_MKEEP)
			|| (mission.type == LDS_MCLEAR)
			|| (mission.type == LDS_MKEEP_LIMBO)
			);
}

//returns TRUE if the correct type of mission for reinforcements
BOOL missionForReInforcements(void)
{
    if ( (mission.type == LDS_CAMSTART)
		|| missionIsOffworld()
		|| (mission.type == LDS_CAMCHANGE)
		)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

//returns TRUE if the correct type of mission and a reinforcement time has been set
BOOL missionCanReEnforce(void)
{
//	return missionIsOffworld() && (mission.ETA >= 0);
    if (mission.ETA >= 0)
    {
        //added CAMSTART for when load up a save game on Cam2A - AB 17/12/98
        //if (missionIsOffworld() || (mission.type == LDS_CAMCHANGE) ||
        //    (mission.type == LDS_CAMSTART))
        if (missionForReInforcements())
        {
            return TRUE;
        }
    }
    return FALSE;
}

//returns TRUE if the mission is a Limbo Expand mission
BOOL missionLimboExpand(void)
{
    if (mission.type == LDS_EXPAND_LIMBO)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


// mission initialisation game code
void initMission(void)
{
	UDWORD inc;

	debug(LOG_SAVEGAME, "*** Init Mission ***");
	mission.type = LDS_NONE;
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		mission.apsStructLists[inc] = NULL;
		mission.apsDroidLists[inc] = NULL;
		mission.apsFeatureLists[inc] = NULL;
		//mission.apsProxDisp[inc] = NULL;
		mission.apsFlagPosLists[inc] = NULL;
		apsLimboDroids[inc] = NULL;
	}
	offWorldKeepLists = FALSE;
	mission.time = -1;
	setMissionCountDown();

	mission.ETA = -1;
	mission.startTime = 0;
	mission.psGateways = NULL;
	mission.apRLEZones = NULL;
	mission.gwNumZones = 0;
	mission.mapHeight = 0;
	mission.mapWidth = 0;
	mission.aNumEquiv = NULL;
	mission.apEquivZones = NULL;
	mission.aZoneReachable = NULL;

	//init all the landing zones
	for (inc = 0; inc < MAX_NOGO_AREAS; inc++)
	{
		sLandingZone[inc].x1 = sLandingZone[inc].y1 = sLandingZone[inc].x2 = sLandingZone[inc].y2 = 0;
	}

	// init the vtol return pos
	memset(asVTOLReturnPos, 0, sizeof(Vector2i) * MAX_PLAYERS);

	bDroidsToSafety = FALSE;
	setPlayCountDown(TRUE);

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
    /*mission.apsDroidLists may contain some droids that have been transferred
    from one campaign to the next*/
	freeAllMissionDroids();

    /*apsLimboDroids may contain some droids that have been saved
    at the end of one mission and not yet used*/
	freeAllLimboDroids();
}

//called to shut down when mid-mission on an offWorld map
BOOL missionShutDown(void)
{
	UDWORD		inc;

	debug(LOG_SAVEGAME, "missionShutDown: called, mission is %s", 
	      missionIsOffworld() ? "off-world" : "main map");
	if (missionIsOffworld())
	{
		//clear out the audio
		audio_StopAll();

		freeAllDroids();
		freeAllStructs();
		freeAllFeatures();
		releaseAllProxDisp();
		gwShutDown();
		//flag positions go with structs
//		free(psMapTiles);
		//free(aMapLinePoints);

		for (inc = 0; inc < MAX_PLAYERS; inc++)
		{
			apsDroidLists[inc] = mission.apsDroidLists[inc];
			mission.apsDroidLists[inc] = NULL;
			apsStructLists[inc] = mission.apsStructLists[inc];
			mission.apsStructLists[inc] = NULL;
			apsFeatureLists[inc] = mission.apsFeatureLists[inc];
			mission.apsFeatureLists[inc] = NULL;
			//apsProxDisp[inc] = mission.apsProxDisp[inc];
			apsFlagPosLists[inc] = mission.apsFlagPosLists[inc];
			mission.apsFlagPosLists[inc] = NULL;
		}

		psMapTiles = mission.psMapTiles;
		aMapLinePoints = mission.aMapLinePoints;
		mapWidth = mission.mapWidth;
		mapHeight = mission.mapHeight;
		psGateways = mission.psGateways;
		apRLEZones = mission.apRLEZones;
		gwNumZones = mission.gwNumZones;
		aNumEquiv = mission.aNumEquiv;
		apEquivZones = mission.apEquivZones;
		aZoneReachable = mission.aZoneReachable;
	}

	// sorry if this breaks something - but it looks like it's what should happen - John
	mission.type = LDS_NONE;

	return TRUE;
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

    //need to init the countdown played each time the mission time is changed
    missionCountDown = NOT_PLAYED_ONE | NOT_PLAYED_TWO | NOT_PLAYED_THREE |
        NOT_PLAYED_FIVE | NOT_PLAYED_TEN | NOT_PLAYED_ACTIVATED;

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


BOOL startMission(LEVEL_TYPE missionType, char *pGame)
{
	BOOL	loaded = TRUE;

	debug(LOG_SAVEGAME, "startMission: type %d", (int)missionType);

	/* Player has (obviously) not failed at the start */
	setPlayerHasLost(FALSE);
	setPlayerHasWon(FALSE);

	/* and win/lose video is obvioulsy not playing */
	setScriptWinLoseVideo(PLAY_NONE);

	// this inits the flag so that 'reinforcements have arrived' message isn't played for the first transporter load
	initFirstTransporterFlag();

	if (mission.type != LDS_NONE)
	{
		/*mission type gets set to none when you have returned from a mission
		so don't want to go another mission when already on one! - so ignore*/
		debug(LOG_SAVEGAME, "startMission: Already on a mission");
		return TRUE;
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
		{
			if (!startMissionCampaignStart(pGame))
			{
				loaded = FALSE;
			}
			break;
		}

		case LDS_MKEEP:
		case LDS_MKEEP_LIMBO:
		{
			if (!startMissionOffKeep(pGame))
			{
				loaded = FALSE;
			}
			break;
		}
		case LDS_BETWEEN:
		{
			//do anything?
			if (!startMissionBetween())
			{
				loaded = FALSE;
			}
			break;
		}
		case LDS_CAMCHANGE:
		{
			if (!startMissionCampaignChange(pGame))
			{
				loaded = FALSE;
			}
			break;
		}
		case LDS_EXPAND:
		{
			if (!startMissionCampaignExpand(pGame))
			{
				loaded = FALSE;
			}
			break;
		}
		case LDS_EXPAND_LIMBO:
		{
			if (!startMissionCampaignExpandLimbo(pGame))
			{
				loaded = FALSE;
			}
			break;
		}
		case LDS_MCLEAR:
		{
			if (!startMissionOffClear(pGame))
			{
				loaded = FALSE;
			}
			break;
		}
		default:
		{
			//error!
			debug( LOG_ERROR, "Unknown Mission Type" );
			abort();
			loaded = FALSE;
		}
	}

	if (!loaded)
	{
		debug( LOG_ERROR, "Unable to load mission file" );
		abort();
		return FALSE;
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

	// add proximity messages for all untapped VISIBLE oil resources
	addOilResourceProximities();

	return TRUE;
}


// initialise the mission stuff for a save game
BOOL startMissionSave(SDWORD missionType)
{
	mission.type = missionType;

	return TRUE;
}


/*checks the time has been set and then adds the timer if not already on
the display*/
void addMissionTimerInterface(void)
{
	//don't add if the timer hasn't been set
	if (mission.time < 0)
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
	DROID	        *psDroid, *psTransporter;
	BOOL            bAddInterface = FALSE;
	W_CLICKFORM     *psForm;

	//check if reinforcements are allowed
	if (mission.ETA >= 0)
	{
		//check the player has at least one Transporter back at base
		psDroid = psTransporter = NULL;
		for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid !=
			NULL; psDroid = psDroid->psNext)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				psTransporter = psDroid;
				break;
			}
		}
		if (psDroid)
		{
			bAddInterface = TRUE;

	    		//check timer is not already on the screen
		    	if (!widgGetFromID(psWScreen, IDTRANTIMER_BUTTON))
    			{
	    			intAddTransporterTimer();
		    	}

			//set the data for the transporter timer
			widgSetUserData(psWScreen, IDTRANTIMER_DISPLAY, (void*)psTransporter);

			// lock the button if necessary
			if (transporterFlying(psTransporter))
			{
				// disable the form so can't add any more droids into the transporter
				psForm = (W_CLICKFORM*)widgGetFromID(psWScreen,IDTRANTIMER_BUTTON);
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

#define OFFSCREEN_RADIUS	600
#define OFFSCREEN_HEIGHT	600

#define	EDGE_SIZE	1

/* pick nearest map edge to point */
void missionGetNearestCorner( UWORD iX, UWORD iY, UWORD *piOffX, UWORD *piOffY )
{
	UDWORD	iMidX = (scrollMinX + scrollMaxX)/2,
			iMidY = (scrollMinY + scrollMaxY)/2;

	if (map_coord(iX) < iMidX)
	{
		*piOffX = (UWORD) (world_coord(scrollMinX) + (EDGE_SIZE * TILE_UNITS));
	}
	else
	{
		*piOffX = (UWORD) (world_coord(scrollMaxX) - (EDGE_SIZE * TILE_UNITS));
	}

	if (map_coord(iY) < iMidY)
	{
		*piOffY = (UWORD) (world_coord(scrollMinY) + (EDGE_SIZE * TILE_UNITS));
	}
	else
	{
		*piOffY = (UWORD) (world_coord(scrollMaxY) - (EDGE_SIZE * TILE_UNITS));
	}
}

/* fly in transporters at start of level */
void missionFlyTransportersIn( SDWORD iPlayer, BOOL bTrackTransporter )
{
	DROID	*psTransporter, *psNext;
	UWORD	iX, iY, iZ;
	SDWORD	iLandX, iLandY, iDx, iDy;
	double  fR;

	bTrackingTransporter = bTrackTransporter;

	iLandX = getLandingX(iPlayer);
	iLandY = getLandingY(iPlayer);
	missionGetTransporterEntry( iPlayer, &iX, &iY );
	iZ = (UWORD) (map_Height( iX, iY ) + OFFSCREEN_HEIGHT);

	psNext = NULL;
	//get the droids for the mission
	for (psTransporter = mission.apsDroidLists[iPlayer]; psTransporter !=
		NULL; psTransporter = psNext)
	{
		psNext = psTransporter->psNext;
		if (psTransporter->droidType == DROID_TRANSPORTER)
		{
            //check that this transporter actually contains some droids
            if (psTransporter->psGroup && psTransporter->psGroup->refCount > 1)
            {
			    //remove out of stored list and add to current Droid list
                if (droidRemove(psTransporter, mission.apsDroidLists))
                {
                    //don't want to add it unless managed to remove it from the previous list
			        addDroid(psTransporter, apsDroidLists);
                }

			    /* set start position */
			    psTransporter->pos.x = iX;
			    psTransporter->pos.y = iY;
			    psTransporter->pos.z = iZ;

			    /* set start direction */
			    iDx = iLandX - iX;
			    iDy = iLandY - iY;


			    fR = (double) atan2(iDx, iDy);
			    if ( fR < 0.0 )
			    {
			    	fR += (double) (2 * M_PI);
			    }
			    psTransporter->direction = RAD_TO_DEG(fR);


				// Camera track requested and it's the selected player.
			    if ( ( bTrackTransporter == TRUE ) && (iPlayer == (SDWORD)selectedPlayer) )
			    {
			    	/* deselect all droids */
			    	selDroidDeselect( selectedPlayer );

    		    	if ( getWarCamStatus() )
		        	{
			        	camToggleStatus();
			        }

	    	    	/* select transporter */
		        	psTransporter->selected = TRUE;
			        camToggleStatus();
			    }

                //little hack to ensure all Transporters are fully repaired by time enter world
                psTransporter->body = psTransporter->originalBody;

			    /* set fly-in order */
			    orderDroidLoc( psTransporter, DORDER_TRANSPORTIN,
			    				iLandX, iLandY );

				audio_PlayObjDynamicTrack( psTransporter, ID_SOUND_BLIMP_FLIGHT,
									moveCheckDroidMovingAndVisible );

                //only want to fly one transporter in at a time now - AB 14/01/99
                break;
            }
		}
	}
}

/* Saves the necessary data when moving from a home base Mission to an OffWorld mission */
void saveMissionData(void)
{
	UDWORD			inc;
	DROID			*psDroid;
	STRUCTURE		*psStruct, *psStructBeingBuilt;
	BOOL			bRepairExists;

	debug(LOG_SAVEGAME, "saveMissionData: called");

	//clear out the audio
	audio_StopAll();

	//save the mission data
	mission.psMapTiles = psMapTiles;
	mission.aMapLinePoints = aMapLinePoints;
	mission.mapWidth = mapWidth;
	mission.mapHeight = mapHeight;
	mission.scrollMinX = scrollMinX;
	mission.scrollMinY = scrollMinY;
	mission.scrollMaxX = scrollMaxX;
	mission.scrollMaxY = scrollMaxY;
	mission.psGateways = psGateways;
	psGateways = NULL;
	mission.apRLEZones = apRLEZones;
	apRLEZones = NULL;
	mission.gwNumZones = gwNumZones;
	gwNumZones = 0;
	mission.aNumEquiv = aNumEquiv;
	aNumEquiv = NULL;
	mission.apEquivZones = apEquivZones;
	apEquivZones = NULL;
	mission.aZoneReachable = aZoneReachable;
	aZoneReachable = NULL;
	// save the selectedPlayer's LZ
	mission.homeLZ_X = getLandingX(selectedPlayer);
	mission.homeLZ_Y = getLandingY(selectedPlayer);

	bRepairExists = FALSE;
	//set any structures currently being built to completed for the selected player
	for (psStruct = apsStructLists[selectedPlayer]; psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->status == SS_BEING_BUILT)
		{
			//find a droid working on it
			for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL;
				psDroid = psDroid->psNext)
			{
				if (orderStateObj(psDroid, DORDER_BUILD, (BASE_OBJECT **)&psStructBeingBuilt))
				{
					if (psStructBeingBuilt == psStruct)
					{
						// check there is enough power to complete
						inc = structPowerToBuild(psStruct) - psStruct->currentPowerAccrued;
						if (inc > 0)
						{
							// not accrued enough power, so check if there is enough available
							if (checkPower(selectedPlayer, inc, FALSE))
							{
								// enough - so use it and set to complete
								usePower(selectedPlayer, inc);
								buildingComplete(psStruct);
							}
						}
						else
						{
							// enough power or more than enough! - either way, set to complete
							buildingComplete(psStruct);
						}
						//don't bother looking for any other droids working on it
						break;
					}
				}
			}
		}
		//check if have a completed repair facility on home world
		if (psStruct->pStructureType->type == REF_REPAIR_FACILITY &&
			psStruct->status == SS_BUILT)
		{
			bRepairExists = TRUE;
		}
	}

	//repair all droids back at home base if have a repair facility
	if (bRepairExists)
	{
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL;
			psDroid = psDroid->psNext)
		{
			if (droidIsDamaged(psDroid))
			{
				psDroid->body = psDroid->originalBody;
			}
		}
	}

	//clear droid orders for all droids except constructors still building
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL;
			psDroid = psDroid->psNext)
	{
		if (orderStateObj(psDroid, DORDER_BUILD, (BASE_OBJECT **)&psStructBeingBuilt))
		{
			if (psStructBeingBuilt->status == SS_BUILT)
			{
				orderDroid(psDroid, DORDER_STOP);
			}
		}
		else
		{
			orderDroid(psDroid, DORDER_STOP);
		}
	}

	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		mission.apsStructLists[inc] = apsStructLists[inc];
		mission.apsDroidLists[inc] = apsDroidLists[inc];
		mission.apsFeatureLists[inc] = apsFeatureLists[inc];
		//mission.apsProxDisp[inc] = apsProxDisp[inc];
		mission.apsFlagPosLists[inc] = apsFlagPosLists[inc];
	}

	mission.playerX = player.p.x;
	mission.playerY = player.p.z;

	//save the power settings
	saveMissionPower();

	//reset before loading in the new game
	//resetFactoryNumFlag();

	//init before loading in the new game
	initFactoryNumFlag();

	//clear all the effects from the map
	initEffectsSystem();

	resetRadarRedraw();
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

	debug(LOG_SAVEGAME, "restoreMissionData: called");

	//clear out the audio
	audio_StopAll();

	//clear all the lists
	proj_FreeAllProjectiles();
	freeAllDroids();
	freeAllStructs();
	freeAllFeatures();
	gwShutDown();
	mapShutdown();
//	free(psMapTiles);
	//free(aMapLinePoints);
	//releaseAllProxDisp();
	//flag positions go with structs

	//restore the game pointers
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		apsDroidLists[inc] = mission.apsDroidLists[inc];
		mission.apsDroidLists[inc] = NULL;
		for(psObj = (BASE_OBJECT *)apsDroidLists[inc]; psObj; psObj=psObj->psNext)
		{
			psObj->died = FALSE;	//make sure the died flag is not set
			gridAddObject(psObj);
		}

		apsStructLists[inc] = mission.apsStructLists[inc];
		mission.apsStructLists[inc] = NULL;
		for(psObj = (BASE_OBJECT *)apsStructLists[inc]; psObj; psObj=psObj->psNext)
		{
			gridAddObject(psObj);
		}

		apsFeatureLists[inc] = mission.apsFeatureLists[inc];
		mission.apsFeatureLists[inc] = NULL;
		for(psObj = (BASE_OBJECT *)apsFeatureLists[inc]; psObj; psObj=psObj->psNext)
		{
			gridAddObject(psObj);
		}

		apsFlagPosLists[inc] = mission.apsFlagPosLists[inc];
		mission.apsFlagPosLists[inc] = NULL;
		//apsProxDisp[inc] = mission.apsProxDisp[inc];
		//mission.apsProxDisp[inc] = NULL;
		//asPower[inc]->usedPower = mission.usedPower[inc];
		//init the next structure to be powered
		asPower[inc]->psLastPowered = NULL;
	}
	//swap mission data over

	psMapTiles = mission.psMapTiles;

	aMapLinePoints = mission.aMapLinePoints;
	mapWidth = mission.mapWidth;
	mapHeight = mission.mapHeight;
	scrollMinX = mission.scrollMinX;
	scrollMinY = mission.scrollMinY;
	scrollMaxX = mission.scrollMaxX;
	scrollMaxY = mission.scrollMaxY;
	psGateways = mission.psGateways;
	apRLEZones = mission.apRLEZones;
	gwNumZones = mission.gwNumZones;
	aNumEquiv = mission.aNumEquiv;
	apEquivZones = mission.apEquivZones;
	aZoneReachable = mission.aZoneReachable;
	//and clear the mission pointers
	mission.psMapTiles	= NULL;
	mission.aMapLinePoints = NULL;
	mission.mapWidth	= 0;
	mission.mapHeight	= 0;
	mission.scrollMinX	= 0;
	mission.scrollMinY	= 0;
	mission.scrollMaxX	= 0;
	mission.scrollMaxY	= 0;
	mission.psGateways	= NULL;
	mission.apRLEZones	= NULL;
	mission.gwNumZones	= 0;
	mission.aNumEquiv	= NULL;
	mission.apEquivZones = NULL;
	mission.aZoneReachable = NULL;

	//reset the current structure lists
	setCurrentStructQuantity(FALSE);

	//initPlayerPower();

	initFactoryNumFlag();
	resetFactoryNumFlag();

	//terrain types? - hopefully not! otherwise we have to load in the terrain texture pages.

	//reset the game time
	//gameTimeReset(mission.startTime);

	offWorldKeepLists = FALSE;

	resetRadarRedraw();

	// reset the environ map back to the homebase settings
	environReset();

	//intSetMapPos(mission.playerX, mission.playerY);
}

/*Saves the necessary data when moving from one mission to a limbo expand Mission*/
void saveMissionLimboData(void)
{
	DROID           *psDroid, *psNext;
	STRUCTURE           *psStruct;

	debug(LOG_SAVEGAME, "saveMissionLimboData: called");

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
			holdProduction(psStruct);
		}
		else if (psStruct->pStructureType->type == REF_RESEARCH)
		{
			holdResearch(psStruct);
		}
	}
}

//this is called via a script function to place the Limbo droids once the mission has started
void placeLimboDroids(void)
{
	DROID           *psDroid, *psNext;
	UDWORD			droidX, droidY;
	PICKTILE		pickRes;

	debug(LOG_SAVEGAME, "placeLimboDroids: called");

	// Copy the droids across for the selected Player
	for (psDroid = apsLimboDroids[selectedPlayer]; psDroid != NULL; psDroid = psNext)
	{
        psNext = psDroid->psNext;
        if (droidRemove(psDroid, apsLimboDroids))
        {
    		addDroid(psDroid, apsDroidLists);
    		//KILL OFF TRANSPORTER - should never be one but....
	    	if (psDroid->droidType == DROID_TRANSPORTER)
		    {
			    vanishDroid(psDroid);
                continue;
		    }
            //set up location for each of the droids
	    	droidX = map_coord(getLandingX(LIMBO_LANDING));
		    droidY = map_coord(getLandingY(LIMBO_LANDING));
		    pickRes = pickHalfATile(&droidX, &droidY,LOOK_FOR_EMPTY_TILE);
		    if (pickRes == NO_FREE_TILE )
		    {
			    ASSERT( FALSE, "placeLimboUnits: Unable to find a free location" );
		    }
		    psDroid->pos.x = (UWORD)world_coord(droidX);
		    psDroid->pos.y = (UWORD)world_coord(droidY);
		    if (pickRes == HALF_FREE_TILE )
		    {
			    psDroid->pos.x += TILE_UNITS;
			    psDroid->pos.y += TILE_UNITS;
		    }
		    ASSERT(worldOnMap(psDroid->pos.x,psDroid->pos.y), "limbo droid is not on the map");
		    psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
		    updateDroidOrientation(psDroid);
		    //psDroid->lastTile = mapTile(map_coord(psDroid->pos.x),
		    //	map_coord(psDroid->pos.y));
		    psDroid->selected = FALSE;
            //this is mainly for VTOLs
			setDroidBase(psDroid, NULL);
		    psDroid->cluster = 0;
		    //initialise the movement data
		    initDroidMovement(psDroid);
            //make sure the died flag is not set
            psDroid->died = FALSE;
        }
        else
        {
            ASSERT( FALSE, "placeLimboUnits: Unable to remove unit from Limbo list" );
        }
	}
}

/*restores the necessary data on completion of a Limbo Expand mission*/
void restoreMissionLimboData(void)
{
	DROID           *psDroid, *psNext;

	debug(LOG_SAVEGAME, "restoreMissionLimboData: called");

    /*the droids stored in the mission droid list need to be added back
    into the current droid list*/
    for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL;
        psDroid = psNext)
    {
        psNext = psDroid->psNext;
		//remove out of stored list and add to current Droid list
		if (droidRemove(psDroid, mission.apsDroidLists))
        {
    		addDroid(psDroid, apsDroidLists);
	    	psDroid->cluster = 0;
            gridAddObject((BASE_OBJECT *)psDroid);
    		//reset droid orders
	    	orderDroid(psDroid, DORDER_STOP);
            //the location of the droid should be valid!
        }
    }
    ASSERT( mission.apsDroidLists[selectedPlayer] == NULL,
        "restoreMissionLimboData: list should be empty" );
}

/*Saves the necessary data when moving from one campaign to the start of the
next - saves out the list of droids for the selected player*/
void saveCampaignData(void)
{
	UBYTE		inc;
	DROID		*psDroid, *psNext, *psSafeDroid, *psNextSafe, *psCurr, *psCurrNext;

	debug(LOG_SAVEGAME, "saveCampaignData: called");

	// If the droids have been moved to safety then get any Transporters that exist
	if (getDroidsToSafetyFlag())
	{
		// Move any Transporters into the mission list
		psDroid = apsDroidLists[selectedPlayer];
		while(psDroid != NULL)
		{
			psNext = psDroid->psNext;
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				// Empty the transporter into the mission list
				ASSERT(psDroid->psGroup != NULL, "saveCampaignData: Transporter does not have a group");

				for (psCurr = psDroid->psGroup->psList; psCurr != NULL && psCurr != psDroid; psCurr = psCurrNext)
				{
					psCurrNext = psCurr->psGrpNext;
					// Remove it from the transporter group
					grpLeave( psDroid->psGroup, psCurr);
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
		while(psDroid != NULL)
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
        reverseObjectList((BASE_OBJECT**)&mission.apsDroidLists[selectedPlayer]);

        //find the *first* transporter
        for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL;
            psDroid = psDroid->psNext)
        {
            if (psDroid->droidType == DROID_TRANSPORTER)
            {
                //fill it with droids from the mission list
                for (psSafeDroid = mission.apsDroidLists[selectedPlayer]; psSafeDroid !=
                    NULL; psSafeDroid = psNextSafe)
                {
                    psNextSafe = psSafeDroid->psNext;
                    if (psSafeDroid != psDroid)
                    {
                        //add to the Transporter, checking for when full
	                    if (checkTransporterSpace(psDroid, psSafeDroid))
	                    {
		                    if (droidRemove(psSafeDroid, mission.apsDroidLists))
                            {
    	                        grpJoin(psDroid->psGroup, psSafeDroid);
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
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		psDroid = apsDroidLists[inc];

		while(psDroid != NULL)
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
BOOL startMissionOffClear(char *pGame)
{
	debug(LOG_SAVEGAME, "startMissionOffClear: called for %s", pGame);

	saveMissionData();

	//load in the new game clearing the lists
	if (!loadGame(pGame, !KEEPOBJECTS, !FREEMEM,FALSE))
	{
		return FALSE;
	}

	//call after everything has been loaded up - done on stageThreeInit
	//gridReset();

	offWorldKeepLists = FALSE;
	intResetPreviousObj();

	//this gets set when the timer is added in scriptFuncs
	//mission.startTime = gameTime;

	// The message should have been played at the between stage
	missionCountDown &= ~NOT_PLAYED_ACTIVATED;

	return TRUE;
}

//start an off world mission - keeping the object lists
BOOL startMissionOffKeep(char *pGame)
{
	debug(LOG_SAVEGAME, "startMissionOffKeep: called for %s", pGame);
	saveMissionData();

	//load in the new game clearing the lists
	if (!loadGame(pGame, !KEEPOBJECTS, !FREEMEM,FALSE))
	{
		return FALSE;
	}

	//call after everything has been loaded up - done on stageThreeInit
	//gridReset();

	offWorldKeepLists = TRUE;
	intResetPreviousObj();

	//this gets set when the timer is added in scriptFuncs
	//mission.startTime = gameTime;

	// The message should have been played at the between stage
	missionCountDown &= ~NOT_PLAYED_ACTIVATED;

	return TRUE;
}

BOOL startMissionCampaignStart(char *pGame)
{
	debug(LOG_SAVEGAME, "startMissionCampaignStart: called for %s", pGame);

	// Clear out all intelligence screen messages
	freeMessages();

	// Check no units left with any settings that are invalid
	clearCampaignUnits();

	// Load in the new game details
	if (!loadGame(pGame, !KEEPOBJECTS, FREEMEM, FALSE))
	{
		return FALSE;
	}

	offWorldKeepLists = FALSE;

	return TRUE;
}

BOOL startMissionCampaignChange(char *pGame)
{
	// Clear out all intelligence screen messages
	freeMessages();

	// Check no units left with any settings that are invalid
	clearCampaignUnits();

	// Clear out the production run between campaigns
	changeProductionPlayer((UBYTE)selectedPlayer);

	saveCampaignData();

	//load in the new game details
	if (!loadGame(pGame, !KEEPOBJECTS, !FREEMEM, FALSE))
	{
		return FALSE;
	}

	offWorldKeepLists = FALSE;
	intResetPreviousObj();

	return TRUE;
}

BOOL startMissionCampaignExpand(char *pGame)
{
	//load in the new game details
	if (!loadGame(pGame, KEEPOBJECTS, !FREEMEM, FALSE))
	{
		return FALSE;
	}

	//call after everything has been loaded up - done on stageThreeInit
	//gridReset();

	offWorldKeepLists = FALSE;
	return TRUE;
}

BOOL startMissionCampaignExpandLimbo(char *pGame)
{
	saveMissionLimboData();

	//load in the new game details
	if (!loadGame(pGame, KEEPOBJECTS, !FREEMEM, FALSE))
	{
		return FALSE;
	}

    	//call after everything has been loaded up - done on stageThreeInit
	//gridReset();

	offWorldKeepLists = FALSE;

	return TRUE;
}

BOOL startMissionBetween(void)
{
	offWorldKeepLists = FALSE;

	return TRUE;
}

//check no units left with any settings that are invalid
void clearCampaignUnits(void)
{
	DROID       *psDroid;

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		orderDroid(psDroid, DORDER_STOP);
		setDroidBase(psDroid, NULL);
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
		orderDroid(psDroid, DORDER_STOP);

		//remove out of stored list and add to current Droid list
		if (droidRemove(psDroid, apsDroidLists))
		{
			addDroid(psDroid, mission.apsDroidLists);
			droidX = getHomeLandingX();
			droidY = getHomeLandingY();
			// Swap the droid and map pointers
			swapMissionPointers();

			pickRes = pickHalfATile(&droidX, &droidY,LOOK_FOR_EMPTY_TILE);
			ASSERT(pickRes != NO_FREE_TILE, "processMission: Unable to find a free location" );
			psDroid->pos.x = (UWORD)world_coord(droidX);
			psDroid->pos.y = (UWORD)world_coord(droidY);
			if (pickRes == HALF_FREE_TILE )
			{
				psDroid->pos.x += TILE_UNITS;
				psDroid->pos.y += TILE_UNITS;
			}
			ASSERT(worldOnMap(psDroid->pos.x,psDroid->pos.y), "the droid is not on the map");
			psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
			updateDroidOrientation(psDroid);
			// Swap the droid and map pointers back again
			swapMissionPointers();
			psDroid->selected = FALSE;
			// This is mainly for VTOLs
			setDroidBase(psDroid, NULL);
			psDroid->cluster = 0;
			// Initialise the movement data
			initDroidMovement(psDroid);
		}
	}
}


#define MAXLIMBODROIDS (999)

/*This deals with droids at the end of an offworld Limbo mission*/
void processMissionLimbo(void)
{
	DROID			*psNext, *psDroid;
	UDWORD	numDroidsAddedToLimboList=0;

	//all droids (for selectedPlayer only) are placed into the limbo list
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		//KILL OFF TRANSPORTER - should never be one but....
		if (psDroid->droidType == DROID_TRANSPORTER)
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
					orderDroid(psDroid, DORDER_STOP);
					numDroidsAddedToLimboList++;
				}
			}
		}
	}
}

/*switch the pointers for the map and droid lists so that droid placement
 and orientation can occur on the map they will appear on*/
void swapMissionPointers(void)
{
	void		*pVoid;
	void		**ppVoid;
	UDWORD		udwTemp, inc;

	debug(LOG_SAVEGAME, "swapMissionPointers: called");

	// Swap psMapTiles
	pVoid = (void*)psMapTiles;
	psMapTiles = mission.psMapTiles;
	mission.psMapTiles = (MAPTILE *)pVoid;
	// Swap map sizes
	udwTemp = mapWidth;
	mapWidth = mission.mapWidth;
	mission.mapWidth = udwTemp;
	udwTemp = mapHeight;
	mapHeight = mission.mapHeight;
	mission.mapHeight = udwTemp;
	//swap gateway zones
	pVoid = (void*)psGateways;
	psGateways = mission.psGateways;
	mission.psGateways = (struct _gateway *)pVoid;
	ppVoid = (void**)apRLEZones;
	apRLEZones = mission.apRLEZones;
	mission.apRLEZones = (UBYTE **)ppVoid;
	udwTemp = (UDWORD)gwNumZones;
	gwNumZones = mission.gwNumZones;
	mission.gwNumZones = (SDWORD)udwTemp;
	pVoid = (void*)aNumEquiv;
	aNumEquiv = mission.aNumEquiv;
	mission.aNumEquiv = (UBYTE *)pVoid;
	ppVoid = (void**)apEquivZones;
	apEquivZones = mission.apEquivZones;
	mission.apEquivZones = (UBYTE **)ppVoid;
	pVoid = (void*)aZoneReachable;
	aZoneReachable = mission.aZoneReachable;
	mission.aZoneReachable = (UBYTE *)pVoid;
	// Swap scroll limits
	udwTemp = scrollMinX;
	scrollMinX = mission.scrollMinX;
	mission.scrollMinX = udwTemp;
	udwTemp = scrollMinY;
	scrollMinY = mission.scrollMinY;
	mission.scrollMinY = udwTemp;
	udwTemp = scrollMaxX;
	scrollMaxX = mission.scrollMaxX;
	mission.scrollMaxX = udwTemp;
	udwTemp = scrollMaxY;
	scrollMaxY = mission.scrollMaxY;
	mission.scrollMaxY = udwTemp;
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		pVoid = (void*)apsDroidLists[inc];
		apsDroidLists[inc] = mission.apsDroidLists[inc];
		mission.apsDroidLists[inc] = (DROID *)pVoid;
		pVoid = (void*)apsStructLists[inc];
		apsStructLists[inc] = mission.apsStructLists[inc];
		mission.apsStructLists[inc] = (STRUCTURE *)pVoid;
		pVoid = (void*)apsFeatureLists[inc];
		apsFeatureLists[inc] = mission.apsFeatureLists[inc];
		mission.apsFeatureLists[inc] = (FEATURE *)pVoid;
		pVoid = (void*)apsFlagPosLists[inc];
		apsFlagPosLists[inc] = mission.apsFlagPosLists[inc];
		mission.apsFlagPosLists[inc] = (FLAG_POSITION *)pVoid;
	}
}

void endMission(void)
{
	if (mission.type == LDS_NONE)
	{
		//can't go back any further!!
		debug(LOG_SAVEGAME, "endMission: Already returned from mission");
		return;
	}

	switch (mission.type)
	{
		case LDS_CAMSTART:
        {
            //any transporters that are flying in need to be emptied
            emptyTransporters(FALSE);
            //when loading in a save game mid cam2a or cam3a it is loaded as a camstart
            endMissionCamChange();
			/*
				This is called at the end of every campaign mission
			*/

			break;
        }
		case LDS_MKEEP:
		{
            //any transporters that are flying in need to be emptied
            emptyTransporters(TRUE);
			endMissionOffKeep();
			break;
		}
		case LDS_EXPAND:
		case LDS_BETWEEN:
		{
			/*
				This is called at the end of every campaign mission
			*/

			break;
		}
		case LDS_CAMCHANGE:
        {
            //any transporters that are flying in need to be emptied
            emptyTransporters(FALSE);
            endMissionCamChange();
            break;
        }
        /* left in so can skip the mission for testing...*/
        case LDS_EXPAND_LIMBO:
        {
            //shouldn't be any transporters on this mission but...who knows?
            endMissionExpandLimbo();
            break;
        }
		case LDS_MCLEAR:
		{
            //any transporters that are flying in need to be emptied
            emptyTransporters(TRUE);
			endMissionOffClear();
			break;
		}

		case LDS_MKEEP_LIMBO:
		{
            //any transporters that are flying in need to be emptied
            emptyTransporters(TRUE);
			endMissionOffKeepLimbo();
			break;
		}
		default:
		{
			//error!
			debug( LOG_ERROR, "Unknown Mission Type" );
			abort();
		}
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
    setPlayCountDown(TRUE);


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
	//called in setUpMission now - AB 6/4/98
	//intAddMissionResult(result);// ajl. fire up interface.
	restoreMissionData();

	//reset variables in Droids
	missionResetDroids();
}

void endMissionOffKeep(void)
{
	processMission();
	//called in setUpMission now - AB 6/4/98
	//intAddMissionResult(result);// ajl. fire up interface.
	restoreMissionData();

	//reset variables in Droids
	missionResetDroids();
}

/*In this case any droids remaining (for selectedPlayer) go into a limbo list
for use in a future mission (expand type) */
void endMissionOffKeepLimbo(void)
{
    //save any droids left 'alive'
	processMissionLimbo();

    //set the lists back to the home base lists
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

/* The AI update routine for all Structures left back at base during a Mission*/
void aiUpdateMissionStructure(STRUCTURE *psStructure)
{
	BASE_STATS			*pSubject = NULL;
	UDWORD				pointsToAdd;
	PLAYER_RESEARCH		*pPlayerRes = asPlayerResList[psStructure->player];
	UDWORD				structureMode = 0;
	DROID				*psNewDroid;
	UBYTE				Quantity;
	FACTORY				*psFactory;
	RESEARCH_FACILITY	*psResFacility;
#ifdef INCLUDE_FACTORYLISTS
	DROID_TEMPLATE		*psNextTemplate;
#endif

	ASSERT( psStructure != NULL,
		"aiUpdateMissionStructure: invalid Structure pointer" );

	ASSERT( (psStructure->pStructureType->type == REF_FACTORY  ||
		psStructure->pStructureType->type == REF_CYBORG_FACTORY  ||
		psStructure->pStructureType->type == REF_VTOL_FACTORY  ||
		psStructure->pStructureType->type == REF_RESEARCH),
		"aiUpdateMissionStructure: Structure is not a Factory or Research Facility" );

	//only interested if the Structure "does" something!
	if (psStructure->pFunctionality == NULL)
	{
		return;
	}

	//check if any power available
	if ((asPower[psStructure->player]->currentPower > POWER_PER_CYCLE) ||
		(!powerCalculated))
	{
		//check if this structure is due some power
		if (getLastPowered((BASE_OBJECT *)psStructure))
		{
			//get some power if necessary
			if (accruePower((BASE_OBJECT *)psStructure))
			{
				updateLastPowered((BASE_OBJECT *)psStructure, psStructure->player);
			}
		}
	}
	//determine the Subject
	switch (psStructure->pStructureType->type)
	{
		case REF_RESEARCH:
		{
			pSubject = ((RESEARCH_FACILITY*)psStructure->pFunctionality)->psSubject;
			structureMode = REF_RESEARCH;
			break;
		}
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		{
			pSubject = ((FACTORY*)psStructure->pFunctionality)->psSubject;
			structureMode = REF_FACTORY;
	        //check here to see if the factory's commander has died
            if (((FACTORY*)psStructure->pFunctionality)->psCommander &&
                ((FACTORY*)psStructure->pFunctionality)->psCommander->died)
            {
                //remove the commander from the factory
                assignFactoryCommandDroid(psStructure, NULL);
            }
		    break;
		}
		default:
			break;
	}

	if (pSubject != NULL)
	{
		//if subject is research...
		if (structureMode == REF_RESEARCH)
		{
			psResFacility = (RESEARCH_FACILITY*)psStructure->pFunctionality;

			//if on hold don't do anything
			if (psResFacility->timeStartHold)
			{
				return;
			}

            pPlayerRes += (pSubject->ref - REF_RESEARCH_START);
			//check research has not already been completed by another structure
			if (IsResearchCompleted(pPlayerRes)==0)
			{
				//check to see if enough power to research has accrued
				if (psResFacility->powerAccrued < ((RESEARCH *)pSubject)->researchPower)
				{
					//wait until enough power
					return;
				}

				if (psResFacility->timeStarted == ACTION_START_TIME)
				{
					//set the time started
					psResFacility->timeStarted = gameTime;
				}

				pointsToAdd = (psResFacility->researchPoints * (gameTime -
                    psResFacility->timeStarted)) / GAME_TICKS_PER_SEC;

				//check if Research is complete

				//if ((pointsToAdd + pPlayerRes->currentPoints) > psResFacility->
				//	timeToResearch)
                if ((pointsToAdd + pPlayerRes->currentPoints) > (
                    (RESEARCH *)pSubject)->researchPoints)

				{
					//store the last topic researched - if its the best
					if (psResFacility->psBestTopic == NULL)
					{
						psResFacility->psBestTopic = psResFacility->psSubject;
					}
					else
					{
						if (((RESEARCH *)psResFacility->psSubject)->researchPoints >
							((RESEARCH *)psResFacility->psBestTopic)->researchPoints)
						{
							psResFacility->psSubject = psResFacility->psSubject;
						}
					}
					psResFacility->psSubject = NULL;
					intResearchFinished(psStructure);
					researchResult(pSubject->ref - REF_RESEARCH_START,
						psStructure->player, TRUE, psStructure);
					//check if this result has enabled another topic
					intCheckResearchButton();
				}
			}
			else
			{
				//cancel this Structure's research since now complete
				psResFacility->psSubject = NULL;
				intResearchFinished(psStructure);
			}
		}
		//check for manufacture
		else if (structureMode == REF_FACTORY)
		{
			psFactory = (FACTORY *)psStructure->pFunctionality;
			Quantity = psFactory->quantity;

			//if on hold don't do anything
			if (psFactory->timeStartHold)
			{
				return;
			}

//			if (psFactory->timeStarted == ACTION_START_TIME)
//			{
//				// also need to check if a command droid's group is full
//				if ( ( psFactory->psCommander != NULL ) &&
//					 ( grpNumMembers( psFactory->psCommander->psGroup ) >=
//							cmdDroidMaxGroup( psFactory->psCommander ) ) )
//				{
//					return;
//				}
//			}

			if(CheckHaltOnMaxUnitsReached(psStructure) == TRUE) {
				return;
			}

			//check enough power has accrued to build the droid
			if (psFactory->powerAccrued < ((DROID_TEMPLATE *)pSubject)->
					powerPoints)
			{
				//wait until enough power
				return;
			}

			/*must be enough power so subtract that required to build*/
			if (psFactory->timeStarted == ACTION_START_TIME)
			{
				//set the time started
				psFactory->timeStarted = gameTime;
			}

			pointsToAdd = (gameTime - psFactory->timeStarted) /
				GAME_TICKS_PER_SEC;

			//check for manufacture to be complete
			if (pointsToAdd > psFactory->timeToBuild)
			{
				//build droid - store in mission list
				psNewDroid = buildMissionDroid((DROID_TEMPLATE *)pSubject,
					psStructure->pos.x, psStructure->pos.y, psStructure->player);

				if (!psNewDroid)
				{
					//if couldn't build then cancel the production
					Quantity = 0;
					psFactory->psSubject = NULL;
					intManufactureFinished(psStructure);
				}
				else
				{
					if (psStructure->player == selectedPlayer)
					{
						intRefreshScreen();	// update the interface.
					}

					setDroidBase(psNewDroid, psStructure);

					//reset the start time
					psFactory->timeStarted = ACTION_START_TIME;
					psFactory->powerAccrued = 0;

#ifdef INCLUDE_FACTORYLISTS
					//next bit for productionPlayer only
					if (productionPlayer == psStructure->player)
					{
						psNextTemplate = factoryProdUpdate(psStructure,
							(DROID_TEMPLATE *)pSubject);
						if (psNextTemplate)
						{
							structSetManufacture(psStructure, psNextTemplate,Quantity);
							return;
						}
						else
						{
							//nothing more to manufacture - reset the Subject and Tab on HCI Form
							psFactory->psSubject = NULL;
							intManufactureFinished(psStructure);
						}
					}
					else
#endif
					{
						//decrement the quantity to manufacture if not set to infinity
						if (Quantity != NON_STOP_PRODUCTION)
						{
							psFactory->quantity--;
							Quantity--;
						}

						// If quantity not 0 then kick of another manufacture.
						if(Quantity)
						{
							// Manufacture another.
							structSetManufacture(psStructure, (DROID_TEMPLATE*)pSubject,Quantity);
							return;
						}
						else
						{
							//when quantity = 0, reset the Subject and Tab on HCI Form
							psFactory->psSubject = NULL;
							intManufactureFinished(psStructure);
						}
					}
				}
			}
		}
	}
}

/* The update routine for all Structures left back at base during a Mission*/
void missionStructureUpdate(STRUCTURE *psBuilding)
{

	ASSERT( psBuilding != NULL,
		"structureUpdate: Invalid Structure pointer" );

	//update the manufacture/research of the building
//	if (psBuilding->pStructureType->type == REF_FACTORY ||
//		psBuilding->pStructureType->type == REF_CYBORG_FACTORY ||
//		psBuilding->pStructureType->type == REF_VTOL_FACTORY ||
//		psBuilding->pStructureType->type == REF_RESEARCH)
	if(StructIsFactory(psBuilding) ||
		psBuilding->pStructureType->type == REF_RESEARCH)
	{
		if (psBuilding->status == SS_BUILT)
		{
			aiUpdateMissionStructure(psBuilding);
		}
	}
}

/* The update routine for all droids left back at home base
Only interested in Transporters at present*/
void missionDroidUpdate(DROID *psDroid)
{
	ASSERT( psDroid != NULL,
		"unitUpdate: Invalid unit pointer" );

    /*This is required for Transporters that are moved offWorld so the
    saveGame doesn't try to set their position in the map - especially important
    for endCam2 where there isn't a valid map!*/
    if (psDroid->droidType == DROID_TRANSPORTER)
    {
	    psDroid->pos.x = INVALID_XY;
	    psDroid->pos.y = INVALID_XY;
    }

	//ignore all droids except Transporters
	if ( (psDroid->droidType != DROID_TRANSPORTER)           ||
		 !(orderState(psDroid, DORDER_TRANSPORTOUT)          ||
		   orderState(psDroid, DORDER_TRANSPORTIN)           ||
		   orderState(psDroid, DORDER_TRANSPORTRETURN))         )
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

//reset variables in Droids such as order and position
void missionResetDroids(void)
{
	UDWORD			player;
	DROID			*psDroid, *psNext;
	STRUCTURE		*psStruct;
	FACTORY			*psFactory = NULL;
//	UDWORD			mapX, mapY;
	BOOL			placed;
	UDWORD			x, y;
	PICKTILE		pickRes;

	debug(LOG_SAVEGAME, "missionResetDroids: called");

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psNext)
		{
			psNext = psDroid->psNext;

			//reset order - unless constructor droid that is mid-build
            //if (psDroid->droidType == DROID_CONSTRUCT && orderStateObj(psDroid,
            if ((psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType ==
                DROID_CYBORG_CONSTRUCT) && orderStateObj(psDroid,
                    DORDER_BUILD, (BASE_OBJECT **)&psStruct))
            {
                //need to set the action time to ignore the previous mission time
                psDroid->actionStarted = gameTime;
            }
            else
            {
			    orderDroid(psDroid, DORDER_STOP);
            }

			//KILL OFF TRANSPORTER
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				vanishDroid(psDroid);
			}
		}
	}

    //don't need to implement this hack now since using pickATile - oh what a wonderful routine...
	//only need to look through the selected players list
	/*need to put all the droids that were built whilst off on the mission into
	a temp list so can add them back into the world one by one so that the
	path blocking routines do not fail*/
	/*for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid =
		psNext)
	{
		psNext = psDroid->psNext;
		//for all droids that have never left home base
		if (psDroid->pos.x == INVALID_XY && psDroid->pos.y == INVALID_XY)
		{
			if (droidRemove(psDroid, apsDroidLists))
            {
			    addDroid(psDroid, mission.apsDroidLists);
            }
		}
	}*/

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid =
		psDroid->psNext)
	//for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL;
	//	psDroid = psNext)
	{
		psNext = psDroid->psNext;
		//for all droids that have never left home base
		if (psDroid->pos.x == INVALID_XY && psDroid->pos.y == INVALID_XY)
		{
            //this is now stored as the baseStruct when the droid is built...
			//find which factory produced the droid
			/*psFactory = NULL;
			for (psStruct = apsStructLists[psDroid->player]; psStruct !=
				NULL; psStruct = psStruct->psNext)
			{
				if(StructIsFactory(psStruct))
				{
					if (((FACTORY *)psStruct->pFunctionality)->psGroup ==
						psDroid->psGroup)
					{
						//found the right factory
						psFactory = (FACTORY *)psStruct->pFunctionality;
						break;
					}
				}
			}*/
            psStruct = psDroid->psBaseStruct;
            if (psStruct && StructIsFactory(psStruct))
            {
                psFactory = (FACTORY *)psStruct->pFunctionality;
            }
			placed = FALSE;
			//find a location next to the factory
			if (psStruct)
			{
				/*placed = placeDroid(psStruct, &x, &y);
				if (placed)
				{
					psDroid->pos.x = (UWORD)world_coord(x);
					psDroid->pos.y = (UWORD)world_coord(y);
				}
				else
				{
					psStruct = NULL;
				}*/
                //use pickATile now - yipeee!
                //use factory DP if one
                if (psFactory->psAssemblyPoint)
                {
                    x = (UWORD)map_coord(psFactory->psAssemblyPoint->coords.x);
                    y = (UWORD)map_coord(psFactory->psAssemblyPoint->coords.y);
                }
                else
                {
                    x = (UWORD)map_coord(psStruct->pos.x);
                    y = (UWORD)map_coord(psStruct->pos.y);
                }
		        pickRes = pickHalfATile(&x, &y,LOOK_FOR_EMPTY_TILE);
		        if (pickRes == NO_FREE_TILE )
		        {
			        ASSERT( FALSE, "missionResetUnits: Unable to find a free location" );
                    psStruct = NULL;
		        }
                else
                {
    		        psDroid->pos.x = (UWORD)world_coord(x);
	    	        psDroid->pos.y = (UWORD)world_coord(y);
		            if (pickRes == HALF_FREE_TILE )
		            {
			            psDroid->pos.x += TILE_UNITS;
			            psDroid->pos.y += TILE_UNITS;
		            }
                    placed = TRUE;
                }
			}
			//if couldn't find the factory - hmmm
			if (!psStruct)
			{
				//just stick them near the HQ
				for (psStruct = apsStructLists[psDroid->player]; psStruct !=
					NULL; psStruct = psStruct->psNext)
				{
					if (psStruct->pStructureType->type == REF_HQ)
					{
						/*psDroid->pos.x = (UWORD)(psStruct->pos.x + 128);
						psDroid->pos.y = (UWORD)(psStruct->pos.y + 128);
						placed = TRUE;
						break;*/
                        //use pickATile again...
                        x = (UWORD)map_coord(psStruct->pos.x);
                        y = (UWORD)map_coord(psStruct->pos.y);
		                pickRes = pickHalfATile(&x, &y,LOOK_FOR_EMPTY_TILE);
		                if (pickRes == NO_FREE_TILE )
		                {
			                ASSERT( FALSE, "missionResetUnits: Unable to find a free location" );
                            psStruct = NULL;
		                }
                        else
                        {
    		                psDroid->pos.x = (UWORD)world_coord(x);
	    	                psDroid->pos.y = (UWORD)world_coord(y);
		                    if (pickRes == HALF_FREE_TILE )
		                    {
			                    psDroid->pos.x += TILE_UNITS;
			                    psDroid->pos.y += TILE_UNITS;
		                    }
                            placed = TRUE;
                        }
                        break;
					}
				}
			}
			if (placed)
			{
				// Do all the things in build droid that never did when it was built!
				// check the droid is a reasonable distance from the edge of the map
				if (psDroid->pos.x <= TILE_UNITS || psDroid->pos.x >= (mapWidth *
				    TILE_UNITS) - TILE_UNITS ||	psDroid->pos.y <= TILE_UNITS ||
				    psDroid->pos.y >= (mapHeight * TILE_UNITS) - TILE_UNITS)
				{
					debug(LOG_SAVEGAME, "missionResetUnits: unit too close to edge of map - removing");
					vanishDroid(psDroid);
					continue;
				}
				// Set droid height
				psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);

				// People always stand upright
				if (psDroid->droidType != DROID_PERSON && !cyborgDroid(psDroid))
				{
					updateDroidOrientation(psDroid);
				}
				visTilesUpdate((BASE_OBJECT *)psDroid);
				// Reset the selected flag
				psDroid->selected = FALSE;
			}
			else
			{
				//can't put it down so get rid of this droid!!
				ASSERT( FALSE,"missionResetUnits: can't place unit - cancel to continue" );
				vanishDroid(psDroid);
			}
		}
	}
}

/*unloads the Transporter passed into the mission at the specified x/y
goingHome = TRUE when returning from an off World mission*/
void unloadTransporter(DROID *psTransporter, UDWORD x, UDWORD y, BOOL goingHome)
{
	DROID		*psDroid, *psNext;
	DROID		**ppCurrentList, **ppStoredList;
	UDWORD		droidX, droidY;
	UDWORD		iX, iY;
	DROID_GROUP	*psGroup;

	if (goingHome)
	{
		ppCurrentList = mission.apsDroidLists;
		ppStoredList = apsDroidLists;
	}
	else
	{
		ppCurrentList = apsDroidLists;
		ppStoredList = mission.apsDroidLists;
	}

	//look through the stored list of droids to see if there any transporters
	/*for (psTransporter = ppStoredList[selectedPlayer]; psTransporter != NULL; psTransporter =
		psTransporter->psNext)
	{
		if (psTransporter->droidType == DROID_TRANSPORTER)
		{
			//remove out of stored list and add to current Droid list
			removeDroid(psTransporter, ppStoredList);
			addDroid(psTransporter, ppCurrentList);
			//need to put the Transporter down at a specified location
			psTransporter->pos.x = x;
			psTransporter->pos.y = y;
		}
	}*/

	//unload all the droids from within the current Transporter
	if (psTransporter->droidType == DROID_TRANSPORTER)
	{
		// If the scripts asked for transporter tracking then clear the "tracking a transporter" flag
		// since the transporters landed and unloaded now.
		if(psTransporter->player == selectedPlayer) {
			bTrackingTransporter = FALSE;
		}

		// reset the transporter cluster
		psTransporter->cluster = 0;
		for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
				&& psDroid != psTransporter; psDroid = psNext)
		{
			psNext = psDroid->psGrpNext;

			//add it back into current droid lists
			addDroid(psDroid, ppCurrentList);
			//hack in the starting point!
			/*psDroid->pos.x = x;
			psDroid->pos.y = y;
			if (!goingHome)
			{
				//send the droids off to a starting location
				orderSelectedLoc(psDroid->player, x + 3*TILE_UNITS, y + 3*TILE_UNITS);
			}*/
			//starting point...based around the value passed in
			droidX = map_coord(x);
			droidY = map_coord(y);
			if (goingHome)
			{
				//swap the droid and map pointers
				swapMissionPointers();
			}
			if (!pickATileGen(&droidX, &droidY,LOOK_FOR_EMPTY_TILE,zonedPAT))
			{
				ASSERT( FALSE, "unloadTransporter: Unable to find a valid location" );
			}
			psDroid->pos.x = (UWORD)world_coord(droidX);
			psDroid->pos.y = (UWORD)world_coord(droidY);
			psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
			updateDroidOrientation(psDroid);
			// a commander needs to get it's group back
			if (psDroid->droidType == DROID_COMMAND)
			{
				if (grpCreate(&psGroup))
				{
					grpJoin(psGroup, psDroid);
				}
				clearCommandDroidFactory(psDroid);
			}

			//initialise the movement data
			initDroidMovement(psDroid);
			//reset droid orders
			orderDroid(psDroid, DORDER_STOP);
			gridAddObject((BASE_OBJECT *)psDroid);
			psDroid->selected = FALSE;
            //this is mainly for VTOLs
			setDroidBase(psDroid, NULL);
			psDroid->cluster = 0;
			if (goingHome)
			{
				//swap the droid and map pointers
				swapMissionPointers();
			}

            //inform all other players
            if (bMultiPlayer)
            {
                sendDroidDisEmbark(psDroid);
            }
		}

		/* trigger script callback detailing group about to disembark */
		transporterSetScriptCurrent( psTransporter );
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_TRANSPORTER_LANDED);
		transporterSetScriptCurrent( NULL );

		/* remove droids from transporter group if not already transferred to script group */
		for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
				&& psDroid != psTransporter; psDroid = psNext)
		{
			psNext = psDroid->psGrpNext;
			grpLeave( psTransporter->psGroup, psDroid );
		}
	}

    //don't do this in multiPlayer
    if (!bMultiPlayer)
    {
	    //send all transporter Droids back to home base if off world
	    if (!goingHome)
	    {
		    /* stop the camera following the transporter */
		    psTransporter->selected = FALSE;

		    /* send transporter offworld */
		    missionGetTransporterExit( psTransporter->player, &iX, &iY );
		    orderDroidLoc(psTransporter, DORDER_TRANSPORTRETURN, iX, iY );
            //set the launch time so the transporter doesn't just disappear for CAMSTART/CAMCHANGE
            transporterSetLaunchTime(gameTime);
        }
    }
}

void missionMoveTransporterOffWorld( DROID *psTransporter )
{
    W_CLICKFORM     *psForm;
    DROID           *psDroid;

	if (psTransporter->droidType == DROID_TRANSPORTER)
	{
    	/* trigger script callback */
	    transporterSetScriptCurrent( psTransporter );
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_TRANSPORTER_OFFMAP);
		transporterSetScriptCurrent( NULL );

		//gridRemoveObject( (BASE_OBJECT *) psTransporter ); - these happen in droidRemove()
		//clustRemoveObject( (BASE_OBJECT *) psTransporter );
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
    		widgSetUserData(psWScreen, IDTRANTIMER_DISPLAY, (void*)psTransporter);

            //make sure the button is enabled
            psForm = (W_CLICKFORM*)widgGetFromID(psWScreen,IDTRANTIMER_BUTTON);
            if (psForm)
            {
                formSetClickState(psForm, WCLICK_NORMAL);
            }

		}
        //need a callback for when all the selectedPlayers' reinforcements have been delivered
        if (psTransporter->player == selectedPlayer)
        {
            psDroid = NULL;
            for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid !=
                NULL; psDroid = psDroid->psNext)
            {
                if (psDroid->droidType != DROID_TRANSPORTER)
                {
                    break;
                }
            }
            if (psDroid == NULL)
            {
            	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NO_REINFORCEMENTS_LEFT);
            }
        }
	}
	else
	{
		debug(LOG_SAVEGAME, "missionMoveTransporterOffWorld: droid type not transporter!");
	}
}


//add the Mission timer into the top  right hand corner of the screen
BOOL intAddMissionTimer(void)
{
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;

	//check to see if it exists already
	if (widgGetFromID(psWScreen,IDTIMER_FORM) != NULL)
	{
		return TRUE;
	}

	// Add the background
	memset(&sFormInit, 0, sizeof(W_FORMINIT));

	sFormInit.formID = 0;
	sFormInit.id = IDTIMER_FORM;
	sFormInit.style = WFORM_PLAIN;

	sFormInit.width = iV_GetImageWidth(IntImages,IMAGE_MISSION_CLOCK);//TIMER_WIDTH;
	sFormInit.height = iV_GetImageHeight(IntImages,IMAGE_MISSION_CLOCK);//TIMER_HEIGHT;
	sFormInit.x = (SWORD)(RADTLX + RADWIDTH - sFormInit.width);
	sFormInit.y = (SWORD)TIMER_Y;
	sFormInit.UserData = PACKDWORD_TRI(0,IMAGE_MISSION_CLOCK,IMAGE_MISSION_CLOCK_UP);;
	sFormInit.pDisplay = intDisplayMissionClock;


	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}


	//add labels for the time display
	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.formID = IDTIMER_FORM;
	sLabInit.id = IDTIMER_DISPLAY;
	sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInit.x = TIMER_LABELX;
	sLabInit.y = TIMER_LABELY;
	sLabInit.width = sFormInit.width;//TIMER_WIDTH;
	sLabInit.height = sFormInit.height;//TIMER_HEIGHT;
	sLabInit.pText = "00:00:00";
	sLabInit.FontID = font_regular;
	sLabInit.pCallback = intUpdateMissionTimer;

	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}



	return TRUE;
}

//add the Transporter timer into the top left hand corner of the screen
BOOL intAddTransporterTimer(void)
{

	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;

    //make sure that Transporter Launch button isn't up as well
    intRemoveTransporterLaunch();

	//check to see if it exists already
	if (widgGetFromID(psWScreen, IDTRANTIMER_BUTTON) != NULL)
	{
		return TRUE;
	}

	// Add the button form - clickable
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = IDTRANTIMER_BUTTON;
	sFormInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;
	sFormInit.x = TRAN_FORM_X;
	sFormInit.y = TRAN_FORM_Y;
	sFormInit.width = iV_GetImageWidth(IntImages,IMAGE_TRANSETA_UP);
	sFormInit.height = iV_GetImageHeight(IntImages,IMAGE_TRANSETA_UP);
	sFormInit.pTip = _("Load Transport");
	sFormInit.pDisplay = intDisplayImageHilight;
	sFormInit.UserData = PACKDWORD_TRI(0,IMAGE_TRANSETA_DOWN,
		IMAGE_TRANSETA_UP);

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	//add labels for the time display
	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.formID = IDTRANTIMER_BUTTON;
	sLabInit.id = IDTRANTIMER_DISPLAY;
	sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInit.x = TRAN_TIMER_X;
	sLabInit.y = TRAN_TIMER_Y;
	sLabInit.width = TRAN_TIMER_WIDTH;
	sLabInit.height = sFormInit.height;
	sLabInit.FontID = font_regular;
	sLabInit.pCallback = intUpdateTransporterTimer;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}

	//add the capacity label
	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.formID = IDTRANTIMER_BUTTON;
	sLabInit.id = IDTRANS_CAPACITY;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = 65;
	sLabInit.y = 1;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "00/10";
	sLabInit.FontID = font_regular;
	sLabInit.pCallback = intUpdateTransCapacity;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}


	return TRUE;
}


/*
//add the Transporter timer into the top left hand corner of the screen
BOOL intAddTransporterTimer(void)
{
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;
	W_BUTINIT		sButInit;

	// Add the background - invisible since the button image caters for this
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = IDTRANSTIMER_FORM;
	sFormInit.style = WFORM_PLAIN | WFORM_INVISIBLE;
	sFormInit.x = TRAN_FORM_X;
	sFormInit.y = TRAN_FORM_Y;
	sFormInit.width = iV_GetImageWidth(IntImages,IMAGE_TRANSETA_UP);//TRAN_FORM_WIDTH;
	sFormInit.height = iV_GetImageHeight(IntImages,IMAGE_TRANSETA_UP);//TRAN_FORM_HEIGHT;
	//sFormInit.pDisplay = intDisplayPlainForm;

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	//add labels for the time display
	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.formID = IDTRANSTIMER_FORM;
	sLabInit.id = IDTRANTIMER_DISPLAY;
	sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInit.x = TRAN_TIMER_X;
	sLabInit.y = TRAN_TIMER_Y;
	sLabInit.width = TRAN_TIMER_WIDTH;
	sLabInit.height = sFormInit.height;//TRAN_TIMER_HEIGHT;
	//sLabInit.pText = "00.00";
	//if (mission.ETA < 0)
	//{
	//	sLabInit.pText = "00:00";
	//}
	//else
	//{
	//	fillTimeDisplay(sLabInit.pText, mission.ETA);
	}
	sLabInit.FontID = font_regular;
	sLabInit.pCallback = intUpdateTransporterTimer;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}

	//set up button data
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = IDTRANSTIMER_FORM;
	sButInit.id = IDTRANTIMER_BUTTON;
	sButInit.x = 0;//TRAN_FORM_X;
	sButInit.y = 0;//TRAN_FORM_Y;
	sButInit.width = sFormInit.width;
	sButInit.height = sFormInit.height;
	sButInit.FontID = font_regular;
	sButInit.style = WBUT_PLAIN;
	//sButInit.pText = "T";
	sButInit.pTip = _("Load Transport");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.pUserData = (void*)PACKDWORD_TRI(0,IMAGE_TRANSETA_DOWN,
		IMAGE_TRANSETA_UP);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}
	return TRUE;
}
*/

void missionSetReinforcementTime( UDWORD iTime )
{
	g_iReinforceTime = iTime;
}

UDWORD  missionGetReinforcementTime(void)
{
    return g_iReinforceTime;
}

//fills in a hours(if bHours = TRUE), minutes and seconds display for a given time in 1000th sec
void fillTimeDisplay(char *psText, UDWORD time, BOOL bHours)
{
	UDWORD		calcTime, inc;

    inc = 0;

    //this is only for the transporter timer - never have hours!
    if (time == LZ_COMPROMISED_TIME)
    {
        psText[inc++] = (UBYTE)('-');
        psText[inc++] = (UBYTE)('-');
	    //seperator
	    psText[inc++] = (UBYTE)(':');
        psText[inc++] = (UBYTE)('-');
        psText[inc++] = (UBYTE)('-');
	    //terminate the timer text
	    psText[inc] = '\0';
    }
    else
    {

	        if (bHours)
	        {
	            //hours
		        calcTime = time / (60*60*GAME_TICKS_PER_SEC);
		        psText[inc++] = (UBYTE)('0'+ calcTime / 10);
	    	    psText[inc++] = (UBYTE)('0'+ calcTime % 10);
		        time -= (calcTime * (60*60*GAME_TICKS_PER_SEC));
	    	    //seperator
		        psText[inc++] = (UBYTE)(':');
	        }
		    //minutes
		    calcTime = time / (60*GAME_TICKS_PER_SEC);
		    psText[inc++] = (UBYTE)('0'+ calcTime / 10);
		    psText[inc++] = (UBYTE)('0'+ calcTime % 10);
		    time -= (calcTime * (60*GAME_TICKS_PER_SEC));
		    //seperator
		    psText[inc++] = (UBYTE)(':');
		    //seconds
		    calcTime = time / GAME_TICKS_PER_SEC;
		    psText[inc++] = (UBYTE)('0'+ calcTime / 10);
		    psText[inc++] = (UBYTE)('0'+ calcTime % 10);
		    //terminate the timer text
		    psText[inc] = '\0';

    }
}


//update function for the mission timer
void intUpdateMissionTimer(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL		*Label = (W_LABEL*)psWidget;
	UDWORD		timeElapsed;//, calcTime;
	SDWORD		timeRemaining;

    //take into account cheating with the mission timer
    //timeElapsed = gameTime - mission.startTime;

    //if the cheatTime has been set, then don't want the timer to countdown until stop cheating
    if (mission.cheatTime)
    {
        timeElapsed = mission.cheatTime - mission.startTime;
    }
    else
    {
	    timeElapsed = gameTime - mission.startTime;
    }
	//check not gone over more than one hour - the mission should have been aborted?
	//if (timeElapsed > 60*60*GAME_TICKS_PER_SEC)
	//check not gone over more than 99 mins - the mission should have been aborted?
    //check not gone over more than 5 hours - arbitary number of hours
    if (timeElapsed > 5*60*60*GAME_TICKS_PER_SEC)
	{
		ASSERT( FALSE,"You've taken too long for this mission!" );
		return;
	}

	timeRemaining = mission.time - timeElapsed;
	if (timeRemaining < 0)
	{
		timeRemaining = 0;
	}

	fillTimeDisplay(Label->aText, timeRemaining, TRUE);

    //make sure its visible
    Label->style &= ~WIDG_HIDDEN;


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
	W_LABEL		*Label = (W_LABEL*)psWidget;
	DROID		*psTransporter;
	SDWORD		timeRemaining;
	SDWORD		ETA;

	ETA = mission.ETA;
	if(ETA < 0) {
		ETA = 0;
	}

	// Get the object associated with this widget.
	psTransporter = (DROID *)Label->pUserData;
	if (psTransporter != NULL)
	{
		ASSERT( psTransporter != NULL,
			"intUpdateTransporterTimer: invalid Droid pointer" );

		if (psTransporter->action == DACTION_TRANSPORTIN ||
			psTransporter->action == DACTION_TRANSPORTWAITTOFLYIN )
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
			    if (timeRemaining < TRANSPORTER_REINFORCE_LEADIN )
			    {
				    // arrived: tell the transporter to move to the new onworld
				    // location if not already doing so
				    if ( psTransporter->action == DACTION_TRANSPORTWAITTOFLYIN )
				    {
					    missionFlyTransportersIn( selectedPlayer, FALSE );
					    eventFireCallbackTrigger( (TRIGGER_TYPE)CALL_TRANSPORTER_REINFORCE );
				    }
			    }
            }
			fillTimeDisplay(Label->aText, timeRemaining, FALSE);
		} else {
			fillTimeDisplay(Label->aText, ETA, FALSE);
		}
	}
	else
	{
		if(missionCanReEnforce()) { // ((mission.type == LDS_MKEEP) || (mission.type == LDS_MCLEAR)) & (mission.ETA >= 0) ) {
			fillTimeDisplay(Label->aText, ETA, FALSE);
		} else {

			fillTimeDisplay(Label->aText, 0, FALSE);

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
	if(widgGetFromID(psWScreen,IDTIMER_FORM) != NULL)
	{
		//and remove it.
		widgDelete(psWScreen, IDTIMER_FORM);
	}
}

/* Remove the Transporter Timer widgets from the screen*/
void intRemoveTransporterTimer(void)
{

	//remove main screen
	if(widgGetFromID(psWScreen,IDTRANTIMER_BUTTON) != NULL)
	{
    	widgDelete(psWScreen, IDTRANTIMER_BUTTON);
    }
}



// ////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////
// mission result functions for the interface.



void intDisplayMissionBackDrop(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	scoreDataToScreen();
}



static void missionResetInGameState( void )
{
	//stop the game if in single player mode
	setMissionPauseState();

	// reset the input state
	resetInput();

	// Add the background
	// get rid of reticule etc..
	intResetScreen(FALSE);
	//intHidePowerBar();
	forceHidePowerBar();
	intRemoveReticule();
	intRemoveMissionTimer();


}

static BOOL _intAddMissionResult(BOOL result, BOOL bPlaySuccess)
{
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;
	W_BUTINIT		sButInit;
//	UDWORD			fileSize=0;

	missionResetInGameState();

	memset(&sFormInit, 0, sizeof(W_FORMINIT));

	// add some funky beats
	cdAudio_PlayTrack(playlist_frontend); // frontend music.

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
		return FALSE;
	}

	// TITLE
	sFormInit.formID		= IDMISSIONRES_BACKFORM;

	sFormInit.id			= IDMISSIONRES_TITLE;
	sFormInit.style			= WFORM_PLAIN;
	sFormInit.x				= MISSIONRES_TITLE_X;
	sFormInit.y				= MISSIONRES_TITLE_Y;
	sFormInit.width			= MISSIONRES_TITLE_W;
	sFormInit.height		= MISSIONRES_TITLE_H;
	sFormInit.disableChildren = TRUE;
	sFormInit.pDisplay		= intOpenPlainForm;	//intDisplayPlainForm;

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	// add form
	sFormInit.formID		= IDMISSIONRES_BACKFORM;

	sFormInit.id			= IDMISSIONRES_FORM;
	sFormInit.style			= WFORM_PLAIN;
	sFormInit.x				= MISSIONRES_X;
	sFormInit.y				= MISSIONRES_Y;
	sFormInit.width			= MISSIONRES_W;
	sFormInit.height		= MISSIONRES_H;
	sFormInit.disableChildren = TRUE;
	sFormInit.pDisplay		= intOpenPlainForm;	//intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	// description of success/fail
	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.formID = IDMISSIONRES_TITLE;
	sLabInit.id = IDMISSIONRES_TXT;
	sLabInit.style = WLAB_PLAIN | WLAB_ALIGNCENTRE;
	sLabInit.x = 0;
	sLabInit.y = 12;
	sLabInit.width = MISSIONRES_TITLE_W;
	sLabInit.height = 16;
	if(result)
	{

        //don't bother adding the text if haven't played the audio
        if (bPlaySuccess)
        {
		    sLabInit.pText = _("OBJECTIVE ACHIEVED");//"Objective Achieved";
        }

	}
	else
	{
	  	sLabInit.pText = _("OBJECTIVE FAILED");//"Objective Failed;
	}
	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}
	// options.
	memset(&sButInit,0,sizeof(W_BUTINIT));
	sButInit.formID		= IDMISSIONRES_FORM;
	sButInit.style		= WBUT_PLAIN | WBUT_TXTCENTRE;
	sButInit.width		= MISSION_TEXT_W;
	sButInit.height		= MISSION_TEXT_H;
	sButInit.FontID		= font_regular;
	sButInit.pTip		= NULL;
	sButInit.pDisplay	= displayTextOption;
    //if won or in debug mode
	if(result || getDebugMappingStatus() || bMultiPlayer)
	{
		//continue
		sButInit.x			= MISSION_2_X;
        // Won the game, so display "Quit to main menu"
		if(testPlayerHasWon() && !bMultiPlayer)
        {
			sButInit.id			= IDMISSIONRES_QUIT;
			sButInit.y			= MISSION_2_Y-8;
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

		/* Only add save option if in the game for real, ie, not fastplay.
        And the player hasn't just completed the whole game
        Don't add save option if just lost and in debug mode*/
		if (!bMultiPlayer && !testPlayerHasWon() && !(testPlayerHasLost() && getDebugMappingStatus()))
		{
			//save
			sButInit.id			= IDMISSIONRES_SAVE;
			sButInit.x			= MISSION_1_X;
			sButInit.y			= MISSION_1_Y;
			sButInit.pText		= _("Save Game");//"Save Game";
			widgAddButton(psWScreen, &sButInit);
		}
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
	MissionResUp = TRUE;

	/* play result audio */
	if ( result == TRUE && bPlaySuccess)
	{
		audio_QueueTrack( ID_SOUND_OBJECTIVE_ACCOMPLISHED );
	}

	return TRUE;
}


BOOL intAddMissionResult(BOOL result, BOOL bPlaySuccess)
{
	/* save result */
	g_bMissionResult = result;


	return _intAddMissionResult(result, bPlaySuccess);
}


void intRemoveMissionResultNoAnim(void)
{
	widgDelete(psWScreen, IDMISSIONRES_TITLE);
	widgDelete(psWScreen, IDMISSIONRES_FORM);
	widgDelete(psWScreen, IDMISSIONRES_BACKFORM);


	 cdAudio_Stop();


	MissionResUp	 	= FALSE;
	ClosingMissionRes   = FALSE;
	intMode				= INT_NORMAL;

	//reset the pauses
	resetMissionPauseState();


	// add back the reticule and power bar.
	intAddReticule();

	intShowPowerBar();

//	EnableMouseDraw(TRUE);
//	MouseMovement(TRUE);
}


void intRunMissionResult(void)
{
	frameSetCursorFromRes(IDC_DEFAULT);

	if(bLoadSaveUp)
	{
		if(runLoadSave(FALSE))// check for file name.
		{
			if(strlen(sRequestResult))
			{
				debug(LOG_SAVEGAME, "intRunMissionResult: Returned %s", sRequestResult);

				if (!bRequestLoad)
				{
					saveGame(sRequestResult,GTYPE_SAVE_START);
					addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY);
				}
			}
		}
	}
}

static void missionContineButtonPressed( void )
{
	if (nextMissionType == LDS_CAMSTART
		|| nextMissionType == LDS_BETWEEN
		|| nextMissionType == LDS_EXPAND
		|| nextMissionType == LDS_EXPAND_LIMBO)
	{
		launchMission();
	}
	widgDelete(psWScreen,IDMISSIONRES_FORM);	//close option box.

	if (bMultiPlayer)
	{
		intRemoveMissionResultNoAnim();
 	}
}

void intProcessMissionResult(UDWORD id)
{
	W_BUTINIT	sButInit;

	switch(id)
	{
	case IDMISSIONRES_LOAD:
		// throw up some filerequester
		addLoadSave(LOAD_MISSIONEND,SaveGamePath,"gam",_("Load Saved Game")/*"Load Game"*/);
		break;
	case IDMISSIONRES_SAVE:
		addLoadSave(SAVE_MISSIONEND,SaveGamePath,"gam",_("Save Game")/*"Save Game"*/);

		if (widgGetFromID(psWScreen, IDMISSIONRES_QUIT) == NULL)
		{
			//Add Quit Button now save has been pressed
			memset(&sButInit,0,sizeof(W_BUTINIT));
			sButInit.formID		= IDMISSIONRES_FORM;
			sButInit.style		= WBUT_PLAIN | WBUT_TXTCENTRE;
			sButInit.width		= MISSION_TEXT_W;
			sButInit.height		= MISSION_TEXT_H;
			sButInit.FontID		= font_regular;
			sButInit.pTip		= NULL;
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

		if(bLoadSaveUp)
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
DROID * buildMissionDroid(DROID_TEMPLATE *psTempl, UDWORD x, UDWORD y,
						  UDWORD player)
{
	DROID		*psNewDroid;

	psNewDroid = buildDroid(psTempl, world_coord(x), world_coord(y), player, TRUE);
	if (!psNewDroid)
	{
		return NULL;
	}
	addDroid(psNewDroid, mission.apsDroidLists);
	//set its x/y to impossible values so can detect when return from mission
	psNewDroid->pos.x = INVALID_XY;
	psNewDroid->pos.y = INVALID_XY;

	//set all the droids to selected from when return
	psNewDroid->selected = TRUE;

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
		debug(LOG_SAVEGAME, "launchMission: Start Mission has not been called");
	}
}


//sets up the game to start a new mission
BOOL setUpMission(UDWORD type)
{
	//close the interface
	intResetScreen(TRUE);

	//oldMission = mission.type;
	/*the last mission must have been successful otherwise endgame would have
	been called*/
	endMission();

	//release the level data for the previous mission
	if (!levReleaseMissionData())
	{
		return FALSE;
	}

	//if (type == MISSION_OFFCLEAR || type == MISSION_OFFKEEP)
	if ( type == LDS_CAMSTART )
	{

        //this cannot be called here since we need to be able to save the game at the end of cam1 and cam2
		/*CDrequired = getCDForCampaign( getCampaignNumber() );
		if ( cdspan_CheckCDPresent(CDrequired) )*/
		{
            //another one of those lovely hacks!!
            BOOL    bPlaySuccess = TRUE;

            //we don't want the 'mission accomplished' audio/text message at end of cam1
            if (getCampaignNumber() == 2)
            {
                bPlaySuccess = FALSE;
            }
    		//give the option of save/continue
	    	if (!intAddMissionResult(TRUE, bPlaySuccess))
		    {
			    return FALSE;
		    }
		    loopMissionState = LMS_SAVECONTINUE;
		}
	}
	else if (type == LDS_MKEEP
		|| type == LDS_MCLEAR
		|| type == LDS_MKEEP_LIMBO
		)
	{

		launchMission();

	}
	else
	{
		if(!getWidgetsStatus())
		{
			setWidgetsStatus(TRUE);
			intResetScreen(FALSE);
		}
		//give the option of save/continue
		if (!intAddMissionResult(TRUE, TRUE))
		{
			return FALSE;
		}
		loopMissionState = LMS_SAVECONTINUE;
	}

	//if current mission is 'between' then don't give option to save/continue again
	/*if (oldMission != MISSION_BETWEEN)
	{
		//give the option of save/continue
		if (!intAddMissionResult(TRUE))
		{
			return FALSE;
		}
	}*/

	return TRUE;
}

//save the power settings before loading in the new map data
void saveMissionPower(void)
{
	UDWORD	inc;

	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		//mission.asPower[inc].initialPower = asPower[inc]->initialPower;
		mission.asPower[inc].extractedPower = asPower[inc]->extractedPower;
		mission.asPower[inc].currentPower = asPower[inc]->currentPower;
	}
}

//add the power from the home base to the current power levels for the mission map
void adjustMissionPower(void)
{
	UDWORD	inc;

	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		//asPower[inc]->initialPower += mission.asPower[inc].initialPower;
		asPower[inc]->extractedPower += mission.asPower[inc].extractedPower;
		asPower[inc]->currentPower += mission.asPower[inc].currentPower;
	}
}

/*sets the appropriate pause states for when the interface is up but the
game needs to be paused*/
void setMissionPauseState(void)
{
	if (!bMultiPlayer)
	{
		gameTimeStop();
		setGameUpdatePause(TRUE);
		setAudioPause(TRUE);
		setScriptPause(TRUE);
		setConsolePause(TRUE);
	}
}

/*resets the pause states */
void resetMissionPauseState(void)
{

	if (!bMultiPlayer)
	{


		setGameUpdatePause(FALSE);
		setAudioPause(FALSE);
		setScriptPause(FALSE);
		setConsolePause(FALSE);
		gameTimeStart();


	}

}

//gets the coords for a no go area
LANDING_ZONE* getLandingZone(SDWORD i)
{
	ASSERT( (i >= 0) && (i < MAX_NOGO_AREAS), "getLandingZone out of range." );
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

//sets the coords for the Transporter to land (for player 0 - selectedPlayer)
void setLandingZone(UBYTE x1, UBYTE y1, UBYTE x2, UBYTE y2)
{
	//quick check that x2 > x1 and y2 > y1
 	if (x2 < x1)
	{
		sLandingZone[0].x1 = x2;
		sLandingZone[0].x2 = x1;
	}
	else
	{
		sLandingZone[0].x1 = x1;
		sLandingZone[0].x2 = x2;
	}
	if (y2 < y1)
	{
		sLandingZone[0].y1 = y2;
		sLandingZone[0].y2 = y1;
	}
	else
	{
		sLandingZone[0].y1 = y1;
		sLandingZone[0].y2 = y2;
	}


	addLandingLights(getLandingX(0)+64,getLandingY(0)+64);
}

//sets the coords for a no go area
void setNoGoArea(UBYTE x1, UBYTE y1, UBYTE x2, UBYTE y2, UBYTE area)
{
	//quick check that x2 > x1 and y2 > y1
	if (x2 < x1)
	{
		sLandingZone[area].x1 = x2;
		sLandingZone[area].x2 = x1;
	}
	else
	{
		sLandingZone[area].x1 = x1;
		sLandingZone[area].x2 = x2;
	}
	if (y2 < y1)
	{
		sLandingZone[area].y1 = y2;
		sLandingZone[area].y2 = y1;
	}
	else
	{
		sLandingZone[area].y1 = y1;
		sLandingZone[area].y2 = y2;
	}

	if (area == 0)
	{
		addLandingLights(getLandingX(area) + 64, getLandingY(area) + 64);
	}
}

static inline void addLandingLight(int x, int y, LAND_LIGHT_SPEC spec, BOOL lit)
{
	// The height the landing lights should be above the ground
	static const unsigned int AboveGround = 16;
	Vector3i pos;

	if ( x < 0 || y < 0 )
		return;

	pos.x = x;
	pos.z = y;
	pos.y = map_Height(x, y) + AboveGround;

	effectSetLandLightSpec(spec);

	addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LAND_LIGHT, FALSE, NULL, lit);
}

static void addLandingLights( UDWORD x, UDWORD y)
{
	addLandingLight(x, y, LL_MIDDLE, TRUE);                 // middle

	addLandingLight(x + 128, y + 128, LL_OUTER, FALSE);     // outer
	addLandingLight(x + 128, y - 128, LL_OUTER, FALSE);
	addLandingLight(x - 128, y + 128, LL_OUTER, FALSE);
	addLandingLight(x - 128, y - 128, LL_OUTER, FALSE);

	addLandingLight(x + 64, y + 64, LL_INNER, FALSE);       // inner
	addLandingLight(x + 64, y - 64, LL_INNER, FALSE);
	addLandingLight(x - 64, y + 64, LL_INNER, FALSE);
	addLandingLight(x - 64, y - 64, LL_INNER, FALSE);
}

/*	checks the x,y passed in are not within the boundary of any Landing Zone
	x and y in tile coords*/
BOOL withinLandingZone(UDWORD x, UDWORD y)
{
	UDWORD		inc;

	ASSERT( x < mapWidth, "withinLandingZone: x coord bigger than mapWidth" );
	ASSERT( y < mapHeight, "withinLandingZone: y coord bigger than mapHeight" );


	for (inc = 0; inc < MAX_NOGO_AREAS; inc++)
	{
		if ((x >= (UDWORD)sLandingZone[inc].x1 && x <= (UDWORD)sLandingZone[inc].x2) &&
			(y >= (UDWORD)sLandingZone[inc].y1 && y <= (UDWORD)sLandingZone[inc].y2))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//returns the x coord for where the Transporter can land (for player 0)
UWORD getLandingX( SDWORD iPlayer )
{
	ASSERT( iPlayer<MAX_NOGO_AREAS, "getLandingX: player %d out of range", iPlayer );
	return (UWORD)world_coord((sLandingZone[iPlayer].x1 + (sLandingZone[iPlayer].x2 -
		sLandingZone[iPlayer].x1)/2));
}

//returns the y coord for where the Transporter can land
UWORD getLandingY( SDWORD iPlayer )
{
	ASSERT( iPlayer<MAX_NOGO_AREAS, "getLandingY: player %d out of range", iPlayer );
	return (UWORD)world_coord((sLandingZone[iPlayer].y1 + (sLandingZone[iPlayer].y2 -
		sLandingZone[iPlayer].y1)/2));
}

//returns the x coord for where the Transporter can land back at home base
UDWORD getHomeLandingX(void)
{
	//return world_coord((mission.homeLZ.x1 + (mission.homeLZ.x2 -
	//	mission.homeLZ.x1)/2));
    return map_coord(mission.homeLZ_X);
}

//returns the y coord for where the Transporter can land back at home base
UDWORD getHomeLandingY(void)
{
	//return world_coord(mission.homeLZ.y1 + (mission.homeLZ.y2 -
	//	mission.homeLZ.y1)/2);
	return map_coord(mission.homeLZ_Y);
}

void missionSetTransporterEntry( SDWORD iPlayer, SDWORD iEntryTileX, SDWORD iEntryTileY )
{
	ASSERT( iPlayer<MAX_PLAYERS, "missionSetTransporterEntry: player %i too high", iPlayer );

	if( (iEntryTileX > scrollMinX) && (iEntryTileX < scrollMaxX) )
	{
		mission.iTranspEntryTileX[iPlayer] = (UWORD) iEntryTileX;
	}
	else
	{
		debug(LOG_SAVEGAME, "missionSetTransporterEntry: entry point x %i outside scroll limits %i->%i", iEntryTileX, scrollMinX, scrollMaxX);
		mission.iTranspEntryTileX[iPlayer] = (UWORD) (scrollMinX + EDGE_SIZE);
	}

	if( (iEntryTileY > scrollMinY) && (iEntryTileY < scrollMaxY) )
	{
		mission.iTranspEntryTileY[iPlayer] = (UWORD) iEntryTileY;
	}
	else
	{
		debug(LOG_SAVEGAME, "missionSetTransporterEntry: entry point y %i outside scroll limits %i->%i", iEntryTileY, scrollMinY, scrollMaxY);
		mission.iTranspEntryTileY[iPlayer] = (UWORD) (scrollMinY + EDGE_SIZE);
	}
}

void missionSetTransporterExit( SDWORD iPlayer, SDWORD iExitTileX, SDWORD iExitTileY )
{
	ASSERT( iPlayer<MAX_PLAYERS, "missionSetTransporterExit: player %i too high", iPlayer );

	if( (iExitTileX > scrollMinX) && (iExitTileX < scrollMaxX) )
	{
		mission.iTranspExitTileX[iPlayer] = (UWORD) iExitTileX;
	}
	else
	{
		debug(LOG_SAVEGAME, "missionSetTransporterExit: entry point x %i outside scroll limits %i->%i", iExitTileX, scrollMinX, scrollMaxX);
		mission.iTranspExitTileX[iPlayer] = (UWORD) (scrollMinX + EDGE_SIZE);
	}

	if( (iExitTileY > scrollMinY) && (iExitTileY < scrollMaxY) )
	{
		mission.iTranspExitTileY[iPlayer] = (UWORD) iExitTileY;
	}
	else
	{
		debug(LOG_SAVEGAME, "missionSetTransporterExit: entry point y %i outside scroll limits %i->%i", iExitTileY, scrollMinY, scrollMaxY);
		mission.iTranspExitTileY[iPlayer] = (UWORD) (scrollMinY + EDGE_SIZE);
	}
}

void missionGetTransporterEntry( SDWORD iPlayer, UWORD *iX, UWORD *iY )
{
	ASSERT( iPlayer<MAX_PLAYERS, "missionGetTransporterEntry: player %i too high", iPlayer );

	*iX = (UWORD) world_coord(mission.iTranspEntryTileX[iPlayer]);
	*iY = (UWORD) world_coord(mission.iTranspEntryTileY[iPlayer]);
}

void missionGetTransporterExit( SDWORD iPlayer, UDWORD *iX, UDWORD *iY )
{
	ASSERT( iPlayer<MAX_PLAYERS, "missionGetTransporterExit: player %i too high", iPlayer );

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
	    if (mission.time >= 0 ) //&& (
            //mission.type == LDS_MKEEP || mission.type == LDS_MKEEP_LIMBO ||
            //mission.type == LDS_MCLEAR || mission.type == LDS_BETWEEN))
	    {
		    //check if time is up
		    if ((SDWORD)(gameTime - mission.startTime) > mission.time)
		    {
			    //the script can call the end game cos have failed!
			    eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MISSION_TIME);
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

	debug(LOG_SAVEGAME, "missionDestroyObjects");
	proj_FreeAllProjectiles();
	for(Player = 0; Player < MAX_PLAYERS; Player++) {
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
				psDroid->died = FALSE;
				removeDroidBase(psDroid);
				psDroid = psNext;
			}
			mission.apsDroidLists[Player] = NULL;

			psStruct = apsStructLists[Player];

			while (psStruct != NULL)
			{
				STRUCTURE *psNext = psStruct->psNext;
				removeStruct(psStruct, TRUE);
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
		if (psDroid->psTarget && psDroid->psTarget->died)
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
				setStructureTarget(psStruct, NULL, i);
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
	//UDWORD			droidX, droidY;
    //BOOL            bPlaced;

    //see if any are left
    if (mission.apsDroidLists[selectedPlayer])
    {
        for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL;
            psDroid = psNext)
        {
            psNext = psDroid->psNext;
            //We want to kill off all droids now! - AB 27/01/99
		    //KILL OFF TRANSPORTER
    		//if (psDroid->droidType == DROID_TRANSPORTER)
	    	{
		    	if (droidRemove(psDroid, mission.apsDroidLists))
                {
		            addDroid(psDroid, apsDroidLists);
                    vanishDroid(psDroid);
                }
	    	}
            /*else
            {
		        //remove out of stored list and add to current Droid list
		        if (droidRemove(psDroid, mission.apsDroidLists))
                {
		            addDroid(psDroid, apsDroidLists);
		            //set the x/y
		            droidX = map_coord(getLandingX(psDroid->player));
		            droidY = map_coord(getLandingY(psDroid->player));
                    bPlaced = pickATileGen(&droidX, &droidY,LOOK_FOR_EMPTY_TILE,normalPAT);
                    if (!bPlaced)
		            {
			            ASSERT( FALSE, "processPreviousCampDroids: Unable to find a free location \
                            cancel to continue" );
                        vanishDroid(psDroid);
		            }
                    else
                    {
    		            psDroid->pos.x = (UWORD)world_coord(droidX);
	    	            psDroid->pos.y = (UWORD)world_coord(droidY);
		                psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
		                updateDroidOrientation(psDroid);
		                //psDroid->lastTile = mapTile(map_coord(psDroid->pos.x),
			            //    map_coord(psDroid->pos.y));

					    psDroid->selected = FALSE;
		                psDroid->cluster = 0;
		                gridAddObject((BASE_OBJECT *)psDroid);
		                //initialise the movement data
		                initDroidMovement(psDroid);
                    }
                }
            }*/
        }
    }
}

//access functions for droidsToSafety flag - so we don't have to end the mission when a Transporter fly's off world
void setDroidsToSafetyFlag(BOOL set)
{
    bDroidsToSafety = set;
}
BOOL getDroidsToSafetyFlag(void)
{
    return bDroidsToSafety;
}


//access functions for bPlayCountDown flag - TRUE = play coded mission count down
void setPlayCountDown(UBYTE set)
{
    bPlayCountDown = set;
}
BOOL getPlayCountDown(void)
{
    return bPlayCountDown;
}


//checks to see if the player has any droids (except Transporters left)
BOOL missionDroidsRemaining(UDWORD player)
{
    DROID   *psDroid;
    BOOL    bDroidsRemaining = FALSE;
    for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
    {
        if (psDroid->droidType != DROID_TRANSPORTER)
        {
            bDroidsRemaining = TRUE;
            //don't bother looking for more
            break;
        }
    }

    return bDroidsRemaining;
}

/*called when a Transporter gets to the edge of the world and the droids are
being flown to safety. The droids inside the Transporter are placed into the
mission list for later use*/
void moveDroidsToSafety(DROID *psTransporter)
{
    DROID       *psDroid, *psNext;

    ASSERT( psTransporter->droidType == DROID_TRANSPORTER,
        "moveUnitsToSafety: unit not a Transporter" );

    //move droids out of Transporter into mission list
	for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
			&& psDroid != psTransporter; psDroid = psNext)
	{
		psNext = psDroid->psGrpNext;
		grpLeave(psTransporter->psGroup, psDroid);
		//cam change add droid
		psDroid->pos.x = INVALID_XY;
		psDroid->pos.y = INVALID_XY;
		addDroid(psDroid, mission.apsDroidLists);
    }
    //move the transporter into the mission list also
    if (droidRemove(psTransporter, apsDroidLists))
    {
		//cam change add droid - done in missionDroidUpdate()
		//psDroid->pos.x = INVALID_XY;
		//psDroid->pos.y = INVALID_XY;
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
    //if not a reinforceable mission and a transporter exists, then add the launch button
    //if (!missionCanReEnforce())
    //check not a typical reinforcement mission
    else if (!missionForReInforcements())
    {
        for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid =
            psDroid->psNext)
        {
            if (psDroid->droidType == DROID_TRANSPORTER)
            {
                intAddTransporterLaunch(psDroid);
                break;
            }
        }
        /*if we got to the end without adding a transporter - there might be
        one sitting in the mission list which is waiting to come back in*/
        if (!psDroid)
        {
            for (psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != NULL;
                psDroid = psDroid->psNext)
            {
                if (psDroid->droidType == DROID_TRANSPORTER &&
                    psDroid->action == DACTION_TRANSPORTWAITTOFLYIN)
                {
                    intAddTransporterLaunch(psDroid);
                    break;
                }
            }
        }
    }

}

void	setCampaignNumber( UDWORD number )
{
	ASSERT( number<4,"Campaign Number too high!" );
	camNumber = number;
}

UDWORD	getCampaignNumber( void )
{
	return(camNumber);
}

/*deals with any selectedPlayer's transporters that are flying in when the
mission ends. bOffWorld is TRUE if the Mission is currenly offWorld*/
void emptyTransporters(BOOL bOffWorld)
{
    DROID       *psTransporter, *psDroid, *psNext, *psNextTrans;

    //see if there are any Transporters in the world
    for (psTransporter = apsDroidLists[selectedPlayer]; psTransporter != NULL;
        psTransporter = psNextTrans)
    {
        psNextTrans = psTransporter->psNext;
        if (psTransporter->droidType == DROID_TRANSPORTER)
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
                        grpLeave(psTransporter->psGroup, psDroid);
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
                        grpLeave(psTransporter->psGroup, psDroid);
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
        if (psTransporter->droidType == DROID_TRANSPORTER)
        {
            //for each droid within the transporter...
 		    for (psDroid = psTransporter->psGroup->psList; psDroid != NULL
				    && psDroid != psTransporter; psDroid = psNext)
		    {
			    psNext = psDroid->psGrpNext;
                //take it out of the Transporter group
                grpLeave(psTransporter->psGroup, psDroid);
			    //add it back into mission droid lists
			    addDroid(psDroid, mission.apsDroidLists);
            }
        }
        //don't need to destory the transporter here - it is dealt with by the endMission process
    }
}

/*bCheating = TRUE == start of cheat
bCheating = FALSE == end of cheat */
void setMissionCheatTime(BOOL bCheating)
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
