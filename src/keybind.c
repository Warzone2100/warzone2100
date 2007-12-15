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
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "objects.h"
#include "basedef.h"
#include "map.h"
#include "warcam.h"
#include "warzoneconfig.h"
#include "console.h"
#include "objects.h"
#include "display.h"
#include "mapdisplay.h"
#include "display3d.h"
#include "edit3d.h"
#include "keybind.h"
#include "mechanics.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lighting.h"
#include "power.h"
#include "hci.h"
#include "oprint.h"
#include "wrappers.h"
#include "ingameop.h"
#include "effects.h"
#include "component.h"
#include "geometry.h"
#include "radar.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"


#include "cheat.h"
#include "e3demo.h"	// will this be on PSX?
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multimenu.h"
#include "atmos.h"
#include "raycast.h"
#include "advvis.h"
#include "game.h"
#include "difficulty.h"


#include "intorder.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/bar.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "order.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piestate.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"

#include "keymap.h"
#include "loop.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptextern.h"
#include "mission.h"
#include "mapgrid.h"
#include "order.h"
#include "drive.h"
#include "selection.h"
#include "difficulty.h"
#include "scriptcb.h"		/* for console callback */
#include "aiexperience.h"	/* for console commands */
#include "scriptfuncs.h"
#include "clparse.h"

#define	MAP_ZOOM_RATE	(1000)


#define PITCH_SCALING	(360*DEG_1)
#define	SECS_PER_PITCH	2
#define MAP_PITCH_RATE	(SPIN_SCALING/SECS_PER_SPIN)

#define MAX_TYPING_LENGTH	80



BOOL		bAllowOtherKeyPresses = TRUE;
STRUCTURE	*psOldRE = NULL;
extern		void shakeStop(void);
char	sTextToSend[MAX_CONSOLE_STRING_LENGTH];
extern char	ScreenDumpPath[];
char	sCurrentConsoleText[MAX_CONSOLE_STRING_LENGTH];			//remember what user types in console for beacon msg
char	beaconMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];		//beacon msg for each player

int fogCol = 0;//start in nicks mode

BOOL	processConsoleCommands( char *pName );

/* Support functions to minimise code size */
void	kfsf_SelectAllSameProp	( PROPULSION_TYPE propType );
void	kfsf_SelectAllSameName	( char *droidName );
void	kfsf_SetSelectedDroidsState( SECONDARY_ORDER sec, SECONDARY_STATE State );
/*
	KeyBind.c
	Holds all the functions that can be mapped to a key.
	All functions at the moment must be 'void func(void)'.
	Alex McLean, Pumpkin Studios, EIDOS Interactive.
*/

// --------------------------------------------------------------------------
void	kf_ToggleMissionTimer( void )
{
	setMissionCheatTime(!mission.cheatTime);
}
// --------------------------------------------------------------------------
void	kf_ToggleRadarJump( void )
{
	setRadarJump(!getRadarJumpStatus());
}
// --------------------------------------------------------------------------
void	kf_NoFaults( void )
{
	audio_QueueTrack( ID_SOUND_NOFAULTS );
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
//===================================================
void kf_ToggleSensorDisplay( void )
{
	if(rangeOnScreen) addConsoleMessage("Fine, sensor display is OFF!",LEFT_JUSTIFY);	//added this message... Yeah, its lame. :)
	else	addConsoleMessage("Lets us see what you see!",LEFT_JUSTIFY);			//added this message... Yeah, its lame. :)
	rangeOnScreen =~rangeOnScreen;							//toggle...  HMm  -Q 5-10-05
}
//===================================================
/* Halves all the heights of the map tiles */
void	kf_HalveHeights( void )
{
UDWORD	i,j;
MAPTILE	*psTile;

  		for (i=0; i<mapWidth; i++)
		{
			for (j=0; j<mapHeight; j++)
			{
				psTile = mapTile(i,j);
				psTile->height/=2;;
			}
		}
}

// --------------------------------------------------------------------------
void	kf_FaceNorth(void)
{
	player.r.y = 0;
	if(getWarCamStatus())
	{
		camToggleStatus();
	}
}
// --------------------------------------------------------------------------
void	kf_FaceSouth(void)
{
	player.r.y = DEG(180);
	if(getWarCamStatus())
	{
		camToggleStatus();
	}
}
// --------------------------------------------------------------------------
void	kf_FaceEast(void)
{
	player.r.y = DEG(90);
	if(getWarCamStatus())
	{
		camToggleStatus();
	}
}
// --------------------------------------------------------------------------
void	kf_FaceWest(void)
{
	player.r.y = DEG(270);
	if(getWarCamStatus())
	{
		camToggleStatus();
	}
}
// --------------------------------------------------------------------------

/* Writes out debug info about all the selected droids */
void	kf_DebugDroidInfo( void )
{
DROID	*psDroid;

	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
			{
				if (psDroid->selected)
				{
					printDroidInfo(psDroid);
				}
			}
}

// --------------------------------------------------------------------------
//
///* Prints out the date and time of the build of the game */
void	kf_BuildInfo( void )
{
 	CONPRINTF(ConsoleString,(ConsoleString,"Built at %s on %s",__TIME__,__DATE__));
}

// --------------------------------------------------------------------------
void	kf_ToggleConsoleDrop( void )
{
	if(!bInTutorial)
	{
		toggleConsoleDrop();
	}
}
// --------------------------------------------------------------------------
void	kf_SetKillerLevel( void )
{
	if(!bMultiPlayer || (NetPlay.bComms == 0))
	{
		setDifficultyLevel(DL_KILLER);
		addConsoleMessage("Hard as nails!!!",LEFT_JUSTIFY);
	}
}
// --------------------------------------------------------------------------
void	kf_SetEasyLevel( void )
{
	if(!bMultiPlayer|| (NetPlay.bComms == 0))
	{
		setDifficultyLevel(DL_EASY);
		addConsoleMessage("Takings thing easy!",LEFT_JUSTIFY);
	}
}

// --------------------------------------------------------------------------
void	kf_UpThePower( void )
{
	if(!bMultiPlayer|| (NetPlay.bComms == 0))
	{
		asPower[selectedPlayer]->currentPower+=1000;
		addConsoleMessage("1000 big ones!!!",LEFT_JUSTIFY);
	}
}

// --------------------------------------------------------------------------
void	kf_MaxPower( void )
{
	if(!bMultiPlayer|| (NetPlay.bComms == 0))
	{
		asPower[selectedPlayer]->currentPower = SDWORD_MAX / 2;
		addConsoleMessage("Power overwhelming",LEFT_JUSTIFY);
	}
}

// --------------------------------------------------------------------------
void	kf_SetNormalLevel( void )
{
	if(!bMultiPlayer|| (NetPlay.bComms == 0))
	{
		setDifficultyLevel(DL_NORMAL);
		addConsoleMessage("Back to normality!",LEFT_JUSTIFY);
	}
}
// --------------------------------------------------------------------------
void	kf_SetHardLevel( void )
{
	if(!bMultiPlayer|| (NetPlay.bComms == 0))
	{
		setDifficultyLevel(DL_HARD);
		addConsoleMessage("Getting tricky!",LEFT_JUSTIFY);
	}
}
// --------------------------------------------------------------------------
void	kf_SetToughUnitsLevel( void )
{
	if(!bMultiPlayer|| (NetPlay.bComms == 0))
	{
		setDifficultyLevel(DL_TOUGH);
		addConsoleMessage("Twice as nice!",LEFT_JUSTIFY);
	}
}
// --------------------------------------------------------------------------
void kf_ToggleFPS(void) //This shows *just FPS* and is always visable (when active) -Q.
{
	showFPS ^= 1;
	CONPRINTF(ConsoleString, (ConsoleString, "FPS display is %s", showFPS ? "Enabled" : "Disabled"));
}
/* Writes out the frame rate */
void	kf_FrameRate( void )
{
	CONPRINTF(ConsoleString,(ConsoleString,"FPS %d; FPS-Limit: %d; PIEs %d; polys %d; Terr. polys %d; States %d", frameGetAverageRate(), getFramerateLimit(), loopPieCount, loopPolyCount, loopTileCount, loopStateChanges));
	if (bMultiPlayer) {
			CONPRINTF(ConsoleString,(ConsoleString,
						"NETWORK:  Bytes: s-%d r-%d  Packets: s-%d r-%d",
						NETgetBytesSent(),
						NETgetBytesRecvd(),
						NETgetPacketsSent(),
						NETgetPacketsRecvd() ));

	}
	gameStats = !gameStats;

	CONPRINTF(ConsoleString, (ConsoleString,"Built at %s on %s",__TIME__,__DATE__));
//		addConsoleMessage("Game statistics display toggled",DEFAULT_JUSTIFY);
}

// --------------------------------------------------------------------------

// display the total number of objects in the world
void kf_ShowNumObjects( void )
{
	int droids, structures, features;

	objCount(&droids, &structures, &features);

	CONPRINTF(ConsoleString,(ConsoleString, "Num Droids: %d  Num Structures: %d  Num Features: %d",
				droids, structures, features));
}

// --------------------------------------------------------------------------

/* Toggles radar on off */
void	kf_ToggleRadar( void )
{
  		radarOnScreen = !radarOnScreen;
//		addConsoleMessage("Radar display toggled",DEFAULT_JUSTIFY);
}

// --------------------------------------------------------------------------

/* Toggles infinite power on/off */
void	kf_TogglePower( void )
{

#ifndef DEBUG
if(bMultiPlayer)
{
	return;
}
#endif

		powerCalculated = !powerCalculated;
		if (powerCalculated)
		{
			addConsoleMessage("Infinite power disabled",DEFAULT_JUSTIFY);
			powerCalc(TRUE);
		}
		else
		{
			addConsoleMessage("Infinite power enabled",DEFAULT_JUSTIFY);
		}
}

// --------------------------------------------------------------------------

/* Recalculates the lighting values for a tile */
void	kf_RecalcLighting( void )
{
        initLighting(0, 0, mapWidth, mapHeight);
		addConsoleMessage("Lighting values for all tiles recalculated",DEFAULT_JUSTIFY);
}

// --------------------------------------------------------------------------

/* Raises the 3dfx gamma value */
void	kf_RaiseGamma( void )
{
	gammaValue++;
	pie_SetGammaValue((float)gammaValue / 20.0f);
}

// --------------------------------------------------------------------------

/* Lowers the threedfx gamma value */
void	kf_LowerGamma( void )
{
	gammaValue--;
	pie_SetGammaValue((float)gammaValue / 20.0f);
}

// --------------------------------------------------------------------------

/* Sends the 3dfx screen buffer to disk */
void	kf_ScreenDump( void )
{
	//CONPRINTF(ConsoleString,(ConsoleString,"Screen dump written to working directory : %s", screenDumpToDisk()));
	screenDumpToDisk(ScreenDumpPath);
}

// --------------------------------------------------------------------------

/* Make all functions available */
void	kf_AllAvailable( void )
{
#ifndef DEBUG
if(bMultiPlayer && (NetPlay.bComms != 0) )
{
	return;
}
#endif
		addConsoleMessage("All items made available",DEFAULT_JUSTIFY);
		makeAllAvailable();
}

// --------------------------------------------------------------------------

/* Flips the cut of a tile */
void	kf_TriFlip( void )
{
	MAPTILE	*psTile;
	psTile = mapTile(mouseTileX,mouseTileY);
	TOGGLE_TRIFLIP(psTile);
//	addConsoleMessage("Triangle flip status toggled",DEFAULT_JUSTIFY);
}

// --------------------------------------------------------------------------

/* Debug info about a map tile */
void	kf_TileInfo(void)
{
	MAPTILE	*psTile = mapTile(mouseTileX, mouseTileY);

	debug(LOG_ERROR, "Tile position=(%d, %d) Terrain=%hhu Texture=%u Height=%hhu Illumination=%hhu",
	      mouseTileX, mouseTileY, terrainType(psTile), TileNumber_tile(psTile->texture), psTile->height,
	      psTile->illumination);
	addConsoleMessage("Tile info dumped into log", DEFAULT_JUSTIFY);
}

// --------------------------------------------------------------------------
void	kf_ToggleBackgroundFog( void )
{

	static BOOL bEnabled  = TRUE;//start in nicks mode

		if (bEnabled)//true, so go to false
		{
			bEnabled = FALSE;
			fogStatus &= FOG_FLAGS-FOG_BACKGROUND;//clear lowest bit of 3
			if (fogStatus == 0)
			{
				pie_SetFogStatus(FALSE);
				pie_EnableFog(FALSE);
			}
		}
		else
		{
			bEnabled = TRUE;
			if (fogStatus == 0)
			{
				pie_EnableFog(TRUE);
			}
			fogStatus |= FOG_BACKGROUND;//set lowest bit of 3
		}

}

extern void	kf_ToggleDistanceFog( void )
{

	static BOOL bEnabled  = TRUE;//start in nicks mode

		if (bEnabled)//true, so go to false
		{
			bEnabled = FALSE;
			fogStatus &= FOG_FLAGS-FOG_DISTANCE;//clear middle bit of 3
			if (fogStatus == 0)
			{
				pie_SetFogStatus(FALSE);
				pie_EnableFog(FALSE);
			}
		}
		else
		{
			bEnabled = TRUE;
			if (fogStatus == 0)
			{
				pie_EnableFog(TRUE);
			}
			fogStatus |= FOG_DISTANCE;//set lowest bit of 3
		}

}

void	kf_ToggleFog( void )
{
	static BOOL fogEnabled = FALSE;

		if (fogEnabled)
		{
			fogEnabled = FALSE;
			pie_SetFogStatus(FALSE);
			pie_EnableFog(fogEnabled);
			addConsoleMessage("Fog Off", DEFAULT_JUSTIFY);
		}
		else
		{
			fogEnabled = TRUE;
			pie_EnableFog(fogEnabled);
			addConsoleMessage("Fog On", DEFAULT_JUSTIFY);
		}
}

// --------------------------------------------------------------------------

/* Toggles fog on/off */
void	kf_ToggleWidgets( void )
{
	if(getWidgetsStatus())
	{
		setWidgetsStatus(FALSE);
	}
	else
	{
		setWidgetsStatus(TRUE);
	}
//	addConsoleMessage("Widgets display toggled",DEFAULT_JUSTIFY);
}

// --------------------------------------------------------------------------

/* Toggle camera on/off */
void	kf_ToggleCamera( void )
{
		if(getWarCamStatus() == FALSE) {
			shakeStop();	// Ensure screen shake stopped before starting camera mode.
			setDrivingStatus(FALSE);
		}
		camToggleStatus();
}

/* Toggle 'watch' window on/off */
void kf_ToggleWatchWindow( void )
{
	addConsoleMessage("WATCH WINDOW!", LEFT_JUSTIFY); // what is this? - per
	(void)addDebugMenu(!DebugMenuUp);
}

// --------------------------------------------------------------------------

/* Raises the tile under the mouse */
void	kf_RaiseTile( void )
{
	raiseTile(mouseTileX,mouseTileY);
}

// --------------------------------------------------------------------------

/* Lowers the tile under the mouse */
void	kf_LowerTile( void )
{
	lowerTile(mouseTileX,mouseTileY);
	// Not sure why the below was here of all places - Per
//	selNextSpecifiedBuilding(REF_FACTORY);
}

// --------------------------------------------------------------------------

/* Quick game exit */
void	kf_SystemClose( void )
{

}

// --------------------------------------------------------------------------
/* Zooms out from display */
void	kf_ZoomOut( void )
{
float	fraction;
float	zoomInterval;

	fraction = MAKEFRACT(frameTime2)/GAME_TICKS_PER_SEC;
	zoomInterval = fraction * MAP_ZOOM_RATE;
	distance += MAKEINT(zoomInterval);
	if(distance>MAXDISTANCE)
	{
		distance = MAXDISTANCE;
	}
	UpdateFogDistance(distance);
}

// --------------------------------------------------------------------------
void	kf_RadarZoomIn( void )
{
	if(RadarZoomLevel < MAX_RADARZOOM)
	{
		RadarZoomLevel++;
	   	SetRadarZoom(RadarZoomLevel);
		audio_PlayTrack( ID_SOUND_BUTTON_CLICK_5 );
   	}
	else	// at maximum already
	{
		audio_PlayTrack( ID_SOUND_BUILD_FAIL );
	}

}
// --------------------------------------------------------------------------
void	kf_RadarZoomOut( void )
{
	if(RadarZoomLevel)
	{
		RadarZoomLevel--;
	   	SetRadarZoom(RadarZoomLevel);
		audio_PlayTrack( ID_SOUND_BUTTON_CLICK_5 );
	}
	else	// at minimum already
	{
		audio_PlayTrack( ID_SOUND_BUILD_FAIL );
	}
}
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
/* Zooms in the map */
void	kf_ZoomIn( void )
{
float	fraction;
float	zoomInterval;

	fraction = MAKEFRACT(frameTime2)/GAME_TICKS_PER_SEC;
	zoomInterval = fraction * MAP_ZOOM_RATE;
	distance -= MAKEINT(zoomInterval);

	if( distance< MINDISTANCE)
	{
		distance = MINDISTANCE;
	}
	UpdateFogDistance(distance);
}

// --------------------------------------------------------------------------
void kf_MaxScrollLimits( void )
{
	scrollMinX = scrollMinY = 0;
	scrollMaxX = mapWidth;
	scrollMaxY = mapHeight;
}


// --------------------------------------------------------------------------
// Shrink the screen down
/*
void	kf_ShrinkScreen( void )
{
	// nearest multiple of 8 plus 1
	if (xOffset<73)
	{
		xOffset+=8;
  		distance+=170;
		if (yOffset<200)
		{
			yOffset+=8;
		}
	}
}
*/
// --------------------------------------------------------------------------
// Expand the screen
/*
void	kf_ExpandScreen( void )
{
	if(xOffset)
	{
   		if (distance>MAXDISTANCE)
   		{
   			distance-=170;
   		}
   		xOffset-=8;
   		if(yOffset)
   		{
   			yOffset-=8;
   		}
	}
}
*/
// --------------------------------------------------------------------------
/* Spins the world round left */
void	kf_RotateLeft( void )
{
float	fraction;
float	rotAmount;

	fraction = MAKEFRACT(frameTime2)/GAME_TICKS_PER_SEC;
	rotAmount = fraction * MAP_SPIN_RATE;
	player.r.y += MAKEINT(rotAmount);
}

// --------------------------------------------------------------------------
/* Spins the world right */
void	kf_RotateRight( void )
{
float	fraction;
float	rotAmount;

	fraction = MAKEFRACT(frameTime2)/GAME_TICKS_PER_SEC;
	rotAmount = fraction * MAP_SPIN_RATE;
	player.r.y -= MAKEINT(rotAmount);
	if(player.r.y<0)
	{
		player.r.y+= DEG(360);
	}
}

// --------------------------------------------------------------------------
/* Pitches camera back */
void	kf_PitchBack( void )
{
float	fraction;
float	pitchAmount;

//#ifdef ALEXM
//SDWORD	pitch;
//SDWORD	angConcern;
//#endif

	fraction = MAKEFRACT(frameTime2)/GAME_TICKS_PER_SEC;
	pitchAmount = fraction * MAP_PITCH_RATE;

//#ifdef ALEXM
//	pitch = getSuggestedPitch();
//	angConcern = DEG(360-pitch);
//
//	if(player.r.x < angConcern)
//	{
//#endif

	player.r.x+= MAKEINT(pitchAmount);

//#ifdef ALEXM
//	}
//#endif
//#ifdef ALEXM
//	if(getDebugMappingStatus() == FALSE)
//#endif

//	{
 	if(player.r.x>DEG(360+MAX_PLAYER_X_ANGLE))
  	{
   		player.r.x = DEG(360+MAX_PLAYER_X_ANGLE);
   	}
//	}
	setDesiredPitch(player.r.x/DEG_1);
}

// --------------------------------------------------------------------------
/* Pitches camera foward */
void	kf_PitchForward( void )
{
float	fraction;
float	pitchAmount;

	fraction = MAKEFRACT(frameTime2)/GAME_TICKS_PER_SEC;
	pitchAmount = fraction * MAP_PITCH_RATE;
	player.r.x-= MAKEINT(pitchAmount);
//#ifdef ALEXM
//	if(getDebugMappingStatus() == FALSE)
//#endif
//	{
	if(player.r.x <DEG(360+MIN_PLAYER_X_ANGLE))
	{
		player.r.x = DEG(360+MIN_PLAYER_X_ANGLE);
	}
//	}
	setDesiredPitch(player.r.x/DEG_1);
}

// --------------------------------------------------------------------------
/* Resets pitch to default */
void	kf_ResetPitch( void )
{
 	player.r.x = DEG(360-20);
	distance = START_DISTANCE;
}

// --------------------------------------------------------------------------
/* Dumps all the keyboard mappings to the console display */
void	kf_ShowMappings( void )
{
	keyShowMappings();
}

// --------------------------------------------------------------------------
/*If this is performed twice then it changes the productionPlayer*/
void	kf_SelectPlayer( void )
{
    UDWORD	playerNumber, prevPlayer;

#ifndef DEBUG
if(bMultiPlayer && (NetPlay.bComms != 0) )
{
	return;
}
#endif

    //store the current player
    prevPlayer = selectedPlayer;

	playerNumber = (getLastSubKey()-KEY_F1);
  	if(playerNumber >= 10)
	{
		selectedPlayer = 0;
	}
	else
	{
		selectedPlayer = playerNumber;
	}
   //	godMode = TRUE;

    if (prevPlayer == selectedPlayer)
    {
        changeProductionPlayer((UBYTE)selectedPlayer);
    }

}
// --------------------------------------------------------------------------

/* Selects the player's groups 1..9 */
void	kf_SelectGrouping( UDWORD	groupNumber)
{
	BOOL	bAlreadySelected;
	DROID	*psDroid;
	BOOL	Selected;

	bAlreadySelected = FALSE;
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid!=NULL; psDroid = psDroid->psNext)
	{
		/* Wipe out the ones in the wrong group */
		if(psDroid->selected && psDroid->group!=groupNumber)
		{
			psDroid->selected = FALSE;
		}
		/* Get the right ones */
		if(psDroid->group == groupNumber)
		{
			if(psDroid->selected)
			{
				bAlreadySelected = TRUE;
			}
		}
	}
	if(bAlreadySelected)
	{
		Selected = activateGroupAndMove(selectedPlayer,groupNumber);
	}
	else
	{
		Selected = activateGroup(selectedPlayer,groupNumber);
	}

	// Tell the driving system that the selection may have changed.
	driveSelectionChanged();
	/* play group audio but only if they wern't already selected - AM */
	if ( Selected && !bAlreadySelected)
	{
		audio_QueueTrack( ID_SOUND_GROUP_0+groupNumber );
		audio_QueueTrack( ID_SOUND_REPORTING );
		audio_QueueTrack( ID_SOUND_RADIOCLICK_1+(rand()%6) );
	}
}

// --------------------------------------------------------------------------

#define DEFINE_NUMED_KF(x) \
	void	kf_SelectGrouping_##x( void ) { \
		kf_SelectGrouping(x); \
	} \
	void	kf_AssignGrouping_##x( void ) { \
		assignDroidsToGroup(selectedPlayer, x); \
	} \
	void	kf_SelectCommander_##x( void ) { \
		selCommander(x); \
	}

DEFINE_NUMED_KF(0)
DEFINE_NUMED_KF(1)
DEFINE_NUMED_KF(2)
DEFINE_NUMED_KF(3)
DEFINE_NUMED_KF(4)
DEFINE_NUMED_KF(5)
DEFINE_NUMED_KF(6)
DEFINE_NUMED_KF(7)
DEFINE_NUMED_KF(8)
DEFINE_NUMED_KF(9)

// --------------------------------------------------------------------------
void	kf_SelectMoveGrouping( void )
{
UDWORD	groupNumber;


	groupNumber = (getLastSubKey()-KEY_1) + 1;

	activateGroupAndMove(selectedPlayer,groupNumber);
}
// --------------------------------------------------------------------------
void	kf_ToggleDroidInfo( void )
{
	camToggleInfo();
}
// --------------------------------------------------------------------------
void	kf_addInGameOptions( void )
{
		setWidgetsStatus(TRUE);
		intAddInGameOptions();
}
// --------------------------------------------------------------------------
/* Tell the scripts to start a mission*/
void	kf_AddMissionOffWorld( void )
{

#ifndef DEBUG
if(bMultiPlayer)
{
	return;
}
#endif

	game_SetValidityKey(VALIDITYKEY_CTRL_M);
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MISSION_START);
}
// --------------------------------------------------------------------------
/* Tell the scripts to end a mission*/
void	kf_EndMissionOffWorld( void )
{

#ifndef DEBUG
if(bMultiPlayer)
{
	return;
}
#endif

	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MISSION_END);
}
// --------------------------------------------------------------------------
/* Initialise the player power levels*/
void	kf_NewPlayerPower( void )
{

#ifndef DEBUG
if(bMultiPlayer)
{
	return;
}
#endif

	newGameInitPower();
}

// --------------------------------------------------------------------------
// Display multiplayer guff.
void	kf_addMultiMenu(void)
{

	if(bMultiPlayer)
	{
		intAddMultiMenu();
	}

}

// --------------------------------------------------------------------------
// start/stop capturing audio for multiplayer

void kf_multiAudioStart(void)
{
}

void kf_multiAudioStop(void)
{
}

// --------------------------------------------------------------------------

void	kf_JumpToMapMarker( void )
{

KEY_CODE	entry;
	if(!getRadarTrackingStatus())
	{
		entry = getLastSubKey();
//		CONPRINTF(ConsoleString,(ConsoleString,"Restoring map position %d:%d",getMarkerX(entry),getMarkerY(entry)));
		player.p.x = getMarkerX(entry);
		player.p.z = getMarkerY(entry);
		player.r.y = getMarkerSpin(entry);
		/* A fix to stop the camera continuing when marker code is called */
		if(getWarCamStatus())
		{
			camToggleStatus();
		}
	}

}


// --------------------------------------------------------------------------
/* Raises the G Offset */
void	kf_UpGeoOffset( void )
{

	geoOffset++;

}
// --------------------------------------------------------------------------
/* Lowers the geoOffset */
void	kf_DownGeoOffset( void )
{

	geoOffset--;

}

// --------------------------------------------------------------------------
/* Toggles the power bar display on and off*/
void	kf_TogglePowerBar( void )
{
	togglePowerBar();
}
// --------------------------------------------------------------------------
/* Toggles whether we process debug key mappings */
void	kf_ToggleDebugMappings( void )
{
#ifndef DEBUG
	// Prevent cheating in multiplayer when not compiled in debug mode
	if (bMultiPlayer && (NetPlay.bComms != 0))
	{
		return;
	}
#endif

	if (bAllowDebugMode)
	{
		if(getDebugMappingStatus())
		{
			processDebugMappings(FALSE);
			CONPRINTF(ConsoleString, (ConsoleString, "CHEATS DISABLED!"));
		}
		else
		{
			game_SetValidityKey(VALIDITYKEY_CHEAT_MODE);
			processDebugMappings(TRUE);
			CONPRINTF(ConsoleString, (ConsoleString, "CHEATS ENABLED!"));
		}

		if(bMultiPlayer)
		{
			sendTextMessage("Presses Debug. CHEAT",TRUE);
		}
	}
}
// --------------------------------------------------------------------------


void	kf_ToggleGodMode( void )
{

#ifndef DEBUG
if(bMultiPlayer && (NetPlay.bComms != 0) )
{
	return;
}
#endif


	if(godMode)
	{
		godMode = FALSE;
//		setDifficultyLevel(getDifficultyLevel());
		CONPRINTF(ConsoleString,(ConsoleString,"God Mode OFF"));
	}
	else
	{
		godMode = TRUE;
//		setModifiers(FRACTCONST(1000,100),FRACTCONST(100,1000));
		CONPRINTF(ConsoleString,(ConsoleString,"God Mode ON"));
	}

}
// --------------------------------------------------------------------------
/* Aligns the view to north - some people can't handle the world spinning */
void	kf_SeekNorth( void )
{
	player.r.y = 0;
	if(getWarCamStatus())
	{
		camToggleStatus();
	}
	CONPRINTF(ConsoleString,(ConsoleString,_("View Aligned to North")));
}

// --------------------------------------------------------------------------
void	kf_TogglePauseMode( void )
{


	if(bMultiPlayer && (NetPlay.bComms != 0) )
	{
		return;
	}


	/* Is the game running? */
	if(gamePaused() == FALSE)
	{
		/* Then pause it */
		setGamePauseStatus(TRUE);
		setConsolePause(TRUE);
		setScriptPause(TRUE);
		setAudioPause(TRUE);
		
		// If cursor trapping is enabled allow the cursor to leave the window
		if (war_GetTrapCursor())
		{
			SDL_WM_GrabInput(SDL_GRAB_OFF);
		}
		
		/* And stop the clock */
		gameTimeStop();
		addConsoleMessage(_("PAUSED"),CENTRE_JUSTIFY);

	}
	else
	{
		/* Else get it going again */
		setGamePauseStatus(FALSE);
		setConsolePause(FALSE);
		setScriptPause(FALSE);
		setAudioPause(FALSE);
		
		// Re-enable cursor trapping if it is enabled
		if (war_GetTrapCursor())
		{
			SDL_WM_GrabInput(SDL_GRAB_ON);
		}
				
		/* And start the clock again */
		gameTimeStart();
	}
}

// --------------------------------------------------------------------------
// finish all the research for the selected player
void	kf_FinishAllResearch(void)
{
	UDWORD	j;

	for (j = 0; j < numResearch; j++)
	{
		PLAYER_RESEARCH	*pPlayerRes = asPlayerResList[selectedPlayer];

		pPlayerRes += j; // select right tech
		if (IsResearchCompleted(pPlayerRes) == FALSE)
		{
			MakeResearchCompleted(pPlayerRes);
			researchResult(j, selectedPlayer, FALSE, NULL);
		}
	}
	CONPRINTF(ConsoleString, (ConsoleString, _("Researched EVERYTHING for you!")));
}

// --------------------------------------------------------------------------
// finish all the research for the selected player
void	kf_FinishResearch( void )
{
	STRUCTURE	*psCurr;

	for (psCurr=interfaceStructList(); psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_RESEARCH)
		{
			((RESEARCH_FACILITY *)psCurr->pFunctionality)->timeStarted = gameTime + 100000;
			//set power accrued to high value so that will trigger straight away
			((RESEARCH_FACILITY *)psCurr->pFunctionality)->powerAccrued = 10000;
		}
	}
}

// --------------------------------------------------------------------------
//void	kf_ToggleRadarAllign( void )
//{
//	toggleRadarAllignment();
//	addConsoleMessage("Radar allignment toggled",LEFT_JUSTIFY);
//}

// --------------------------------------------------------------------------
void	kf_ToggleEnergyBars( void )
{
	toggleEnergyBars();
	CONPRINTF(ConsoleString,(ConsoleString, _("Energy bars display toggled") ));
}

// --------------------------------------------------------------------------
void	kf_ToggleDemoMode( void )
{
	if(demoGetStatus() == FALSE)
	{
		/* Switch on demo mode */
		toggleDemoStatus();
		enableConsoleDisplay(TRUE);
	}
	else
	{
		toggleDemoStatus();
		flushConsoleMessages();
		setConsolePermanence(FALSE,TRUE);
		permitNewConsoleMessages(TRUE);
		addConsoleMessage("Demo Mode OFF - Returning to normal game mode",LEFT_JUSTIFY);
		if(getWarCamStatus())
		{
			camToggleStatus();
		}
	}

}

// --------------------------------------------------------------------------
void	kf_ChooseOptions( void )
{
	intResetScreen(TRUE);
	setWidgetsStatus(TRUE);
	intAddOptions();
}


// --------------------------------------------------------------------------
void	kf_ToggleBlips( void )
{
	setBlipDraw(!doWeDrawRadarBlips());
}
// --------------------------------------------------------------------------
void	kf_ToggleProximitys( void )
{
	setProximityDraw(!doWeDrawProximitys());
}
// --------------------------------------------------------------------------
void	kf_JumpToResourceExtractor( void )
{
STRUCTURE	*psStruct;
SDWORD	xJump,yJump;

	psStruct = getRExtractor(psOldRE);
	if(psStruct)
	{
		xJump = (psStruct->pos.x - ((visibleTiles.x/2)*TILE_UNITS));
		yJump = (psStruct->pos.y - ((visibleTiles.y/2)*TILE_UNITS));
		player.p.x = xJump;
		player.p.z = yJump;
		player.r.y = 0; // face north
		setViewPos(map_coord(psStruct->pos.x), map_coord(psStruct->pos.y), TRUE);
		psOldRE = psStruct;
	}
	else
	{
		addConsoleMessage(_("Unable to locate any resource extractors!"),LEFT_JUSTIFY);
	}

}
// --------------------------------------------------------------------------
void	kf_JumpToRepairUnits( void )
{
	selNextSpecifiedUnit( DROID_REPAIR );
}
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
void	kf_JumpToConstructorUnits( void )
{
	selNextSpecifiedUnit( DROID_CONSTRUCT );
}
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
void	kf_JumpToSensorUnits( void )
{
	selNextSpecifiedUnit( DROID_SENSOR );
}
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
void	kf_JumpToCommandUnits( void )
{
	selNextSpecifiedUnit( DROID_COMMAND );
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
void	kf_JumpToUnassignedUnits( void )
{
	selNextUnassignedUnit();
}
// --------------------------------------------------------------------------


void	kf_ToggleOverlays( void )
{
		/* Make sure they're both the same */
		/* Flip their states */
//		radarOnScreen = !radarOnScreen;

	if(getWidgetsStatus())
	{
		setWidgetsStatus(FALSE);
	}
	else
	{
		setWidgetsStatus(TRUE);
	}

}

void	kf_SensorDisplayOn( void )
{
	bDisplaySensorRange = TRUE;
}

void	kf_SensorDisplayOff( void )
{
	bDisplaySensorRange = FALSE;
}


// --------------------------------------------------------------------------
/*
#define IDRET_OPTIONS		2		// option button
#define IDRET_BUILD			3		// build button
#define IDRET_MANUFACTURE	4		// manufacture button
#define IDRET_RESEARCH		5		// research button
#define IDRET_INTEL_MAP		6		// intelligence map button
#define IDRET_DESIGN		7		// design droids button
#define IDRET_CANCEL		8		// central cancel button
*/

// --------------------------------------------------------------------------
void	kf_ChooseCommand( void )
{
	if (intCheckReticuleButEnabled(IDRET_COMMAND))
	{
		setKeyButtonMapping(IDRET_COMMAND);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseManufacture( void )
{
	if (intCheckReticuleButEnabled(IDRET_MANUFACTURE))
	{
		setKeyButtonMapping(IDRET_MANUFACTURE);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseResearch( void )
{
	if (intCheckReticuleButEnabled(IDRET_RESEARCH))
	{
		setKeyButtonMapping(IDRET_RESEARCH);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseBuild( void )
{
	if (intCheckReticuleButEnabled(IDRET_BUILD))
	{
		setKeyButtonMapping(IDRET_BUILD);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseDesign( void )
{
	if (intCheckReticuleButEnabled(IDRET_DESIGN))
	{
		setKeyButtonMapping(IDRET_DESIGN);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseIntelligence( void )
{
	if (intCheckReticuleButEnabled(IDRET_INTEL_MAP))
	{
		setKeyButtonMapping(IDRET_INTEL_MAP);
	}
}

// --------------------------------------------------------------------------

void	kf_ChooseCancel( void )
{
	setKeyButtonMapping(IDRET_CANCEL);
}

// --------------------------------------------------------------------------
void	kf_ToggleDrivingMode( void )
{
	addConsoleMessage("Toggle driver mode", LEFT_JUSTIFY); // what does this do? - per

	/* No point unless we're tracking */
	if(getWarCamStatus())
	{
		if(getDrivingStatus())
		{
			StopDriverMode();
			addConsoleMessage("DriverMode off", LEFT_JUSTIFY);
		}
		else
		{
			if(	(driveModeActive() == FALSE) &&	(demoGetStatus() == FALSE) && !bMultiPlayer)
			{
				StartDriverMode( NULL );
				addConsoleMessage("DriverMode on", LEFT_JUSTIFY);
			}
		}
	}
}

BOOL	bMovePause = FALSE;

// --------------------------------------------------------------------------
void	kf_MovePause( void )
{

	if(!bMultiPlayer)	// can't do it in multiplay
	{
		if(!bMovePause)
		{
			/* Then pause it */
			setGamePauseStatus(TRUE);
			setConsolePause(TRUE);
			setScriptPause(TRUE);
			setAudioPause(TRUE);
			/* And stop the clock */
			gameTimeStop();
			setWidgetsStatus(FALSE);
			radarOnScreen = FALSE;
			bMovePause = TRUE;
		}
		else
		{
			setWidgetsStatus(TRUE);
			radarOnScreen = TRUE;
			/* Else get it going again */
			setGamePauseStatus(FALSE);
			setConsolePause(FALSE);
			setScriptPause(FALSE);
			setAudioPause(FALSE);
			/* And start the clock again */
			gameTimeStart();
			bMovePause = FALSE;
		}
	}
}

// --------------------------------------------------------------------------
void	kf_MoveToLastMessagePos( void )
{
	SDWORD	iX, iY, iZ;

	if (!audio_GetPreviousQueueTrackPos( &iX, &iY, &iZ ))
	{
		return;
	}

// Should use requestRadarTrack but the camera gets jammed so use setViewpos - GJ
//		requestRadarTrack( iX, iY );
	setViewPos( map_coord(iX), map_coord(iY), TRUE );
}
// --------------------------------------------------------------------------
/* Makes it snow if it's not snowing and stops it if it is */
void	kf_ToggleWeather( void )
{
	if(atmosGetWeatherType() == WT_NONE)
	{
		atmosSetWeatherType(WT_SNOWING);
		addConsoleMessage("Oh, the weather outside is frightful... SNOW",LEFT_JUSTIFY);

	}
	else if(atmosGetWeatherType() == WT_SNOWING)
	{
		atmosSetWeatherType(WT_RAINING);
		addConsoleMessage("Singing in the rain, I'm singing in the rain... RAIN",LEFT_JUSTIFY);
	}
	else
	{
		atmosInitSystem();
		atmosSetWeatherType(WT_NONE);
		addConsoleMessage("Forecast : Clear skies for all areas... NO WEATHER",LEFT_JUSTIFY);
	}
}

// --------------------------------------------------------------------------
void	kf_SelectNextFactory(void)
{
	selNextSpecifiedBuilding(REF_FACTORY);
}
// --------------------------------------------------------------------------
void	kf_SelectNextResearch(void)
{
	selNextSpecifiedBuilding(REF_RESEARCH);
}
// --------------------------------------------------------------------------
void	kf_SelectNextPowerStation(void)
{
	selNextSpecifiedBuilding(REF_POWER_GEN);
}
// --------------------------------------------------------------------------
void	kf_SelectNextCyborgFactory(void)
{
	selNextSpecifiedBuilding(REF_CYBORG_FACTORY);
}
// --------------------------------------------------------------------------


void	kf_KillEnemy( void )
{
	UDWORD		player;
	DROID		*psCDroid,*psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;

	CONPRINTF(ConsoleString, (ConsoleString, _("Enemy destroyed by cheating!")));

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if(player!=selectedPlayer)
		{
		 	// wipe out all the droids
			for(psCDroid=apsDroidLists[player]; psCDroid; psCDroid=psNDroid)
			{
				psNDroid = psCDroid->psNext;
				destroyDroid(psCDroid);
			}
			// wipe out all their structures
		  	for(psCStruct=apsStructLists[player]; psCStruct; psCStruct=psNStruct)
		  	{
		  		psNStruct = psCStruct->psNext;
		  		destroyStruct(psCStruct);
		  	}
		}
	}
}

// kill all the selected objects
void kf_KillSelected(void)
{
	DROID		*psCDroid, *psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;

	for(psCDroid=apsDroidLists[selectedPlayer]; psCDroid; psCDroid=psNDroid)
	{
		psNDroid = psCDroid->psNext;
		if (psCDroid->selected)
		{
//		  	removeDroid(psCDroid);
			destroyDroid(psCDroid);
		}
	}
	for(psCStruct=apsStructLists[selectedPlayer]; psCStruct; psCStruct=psNStruct)
	{
		psNStruct = psCStruct->psNext;
		if (psCStruct->selected)
		{
			destroyStruct(psCStruct);
		}
	}
}


// --------------------------------------------------------------------------
// display the grid info for all the selected objects
void kf_ShowGridInfo(void)
{
	DROID		*psCDroid, *psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;

	for(psCDroid=apsDroidLists[selectedPlayer]; psCDroid; psCDroid=psNDroid)
	{
		psNDroid = psCDroid->psNext;
		if (psCDroid->selected)
		{
			gridDisplayCoverage((BASE_OBJECT *)psCDroid);
		}
	}
	for(psCStruct=apsStructLists[selectedPlayer]; psCStruct; psCStruct=psNStruct)
	{
		psNStruct = psCStruct->psNext;
		if (psCStruct->selected)
		{
			gridDisplayCoverage((BASE_OBJECT *)psCStruct);
		}
	}
}


// --------------------------------------------------------------------------
void kf_GiveTemplateSet(void)
{
	addTemplateSet(4,0);
	addTemplateSet(4,1);
	addTemplateSet(4,2);
	addTemplateSet(4,3);
}

// --------------------------------------------------------------------------
// Chat message. NOTE THIS FUNCTION CAN DISABLE ALL OTHER KEYPRESSES
void kf_SendTextMessage(void)
{
	char	ch;
	char tmp[MAX_CONSOLE_STRING_LENGTH + 100];
	SDWORD	i;

	if(bAllowOtherKeyPresses)									// just starting.
	{
		bAllowOtherKeyPresses = FALSE;
		strlcpy(sTextToSend, "", sizeof(sTextToSend));
		strlcpy(sCurrentConsoleText, "", sizeof(sTextToSend));							//for beacons
		inputClearBuffer();
	}

	ch = (char)inputGetKey();
	while (ch != 0)												// in progress
	{
		// Kill if they hit return - it maxes out console or it's more than one line long
	   	if ((ch == INPBUF_CR) || (strlen(sTextToSend)>=MAX_CONSOLE_STRING_LENGTH-16) // Prefixes with ERROR: and terminates with '?'
		 || iV_GetTextWidth(sTextToSend) > (pie_GetVideoBufferWidth()-64))// sendit
	   //	if((ch == INPBUF_CR) || (strlen(sTextToSend)==MAX_TYPING_LENGTH)
		{
			bAllowOtherKeyPresses = TRUE;
		 //	flushConsoleMessages();

			strlcpy(sCurrentConsoleText, "", sizeof(sCurrentConsoleText));		//reset beacon msg, since console is empty now

			// don't send empty lines to other players
			if(!strcmp(sTextToSend, ""))
				return;

			/* process console commands (only if skirmish or multiplayer, not a campaign) */
			if((game.type == SKIRMISH) || bMultiPlayer)
			{
				if(processConsoleCommands(sTextToSend))
				{
					return;	//it was a console command, so don't send
				}
			}

			//console callback message
			//--------------------------
			ConsolePlayer = selectedPlayer;
			strlcpy(ConsoleMsg, sTextToSend, sizeof(ConsoleMsg));
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_CONSOLE);


			if(bMultiPlayer && NetPlay.bComms)
			{
				sendTextMessage(sTextToSend,FALSE);
			}
			else
			{
				//show the message we sent on our local console as well (even in skirmish, to see console commands)
				//sprintf(tmp,"%d",selectedPlayer);

				strlcpy(tmp, getPlayerName(selectedPlayer), sizeof(tmp));
				strlcat(tmp, " : ", sizeof(tmp));        // seperator
				strlcat(tmp, sTextToSend, sizeof(tmp));  // add message
				addConsoleMessage(tmp,DEFAULT_JUSTIFY);

				//in skirmish send directly to AIs, for command and chat procesing
				for(i=0; i<game.maxPlayers; i++)		//don't use MAX_PLAYERS here, although possible
				{
					if(openchannels[i] && i != selectedPlayer)
					{
						sendAIMessage(sTextToSend, selectedPlayer, i);
					}
				}

				if (getDebugMappingStatus())
				{
					attemptCheatCode(sTextToSend);
				}
			}
			return;
		}
		else if(ch == INPBUF_BKSPACE )							// delete
		{
			if(sTextToSend[0] != '\0')							// cant delete nothing!
			{
				sTextToSend[strlen(sTextToSend)-1]= '\0';
				strlcpy(sCurrentConsoleText, sTextToSend, sizeof(sCurrentConsoleText));		//beacons
			}
		}
		else if(ch == INPBUF_ESC)								//abort.
		{
			bAllowOtherKeyPresses = TRUE;
			strlcpy(sCurrentConsoleText, "", sizeof(sCurrentConsoleText));
		 //	flushConsoleMessages();
			return;
		}
		else							 						// display
		{
			const char input_char[2] = { inputGetCharKey(), '\0' };

			strlcat(sTextToSend, input_char, sizeof(sTextToSend));

			strlcpy(sCurrentConsoleText, sTextToSend, sizeof(sCurrentConsoleText));
		}

		ch = (char)inputGetKey();
	}

	// macro store stuff
	if (keyPressed(KEY_F1))
	{
		if(keyDown(KEY_LCTRL))
		{
			strlcpy(ingame.phrases[0], sTextToSend, sizeof(ingame.phrases[0]));
		}
		else
		{
			strlcpy(sTextToSend, ingame.phrases[0], sizeof(sTextToSend));
			bAllowOtherKeyPresses = TRUE;
		 //	flushConsoleMessages();
			sendTextMessage(sTextToSend,FALSE);
			return;
		}
	}
	if (keyPressed(KEY_F2))
	{
		if(keyDown(KEY_LCTRL))
		{
			strlcpy(ingame.phrases[1], sTextToSend, sizeof(ingame.phrases[1]));
		}
		else
		{
			strlcpy(sTextToSend, ingame.phrases[1], sizeof(sTextToSend));
			bAllowOtherKeyPresses = TRUE;
		//	flushConsoleMessages();
			sendTextMessage(sTextToSend,FALSE);
			return;
		}
	}
	if (keyPressed(KEY_F3))
	{
		if(keyDown(KEY_LCTRL))
		{
			strlcpy(ingame.phrases[2], sTextToSend, sizeof(ingame.phrases[2]));
		}
		else
		{
			strlcpy(sTextToSend, ingame.phrases[2], sizeof(sTextToSend));
			bAllowOtherKeyPresses = TRUE;
		//	flushConsoleMessages();
			sendTextMessage(sTextToSend,FALSE);
			return;
		}
	}
	if (keyPressed(KEY_F4))
	{
		if(keyDown(KEY_LCTRL))
		{
			strlcpy(ingame.phrases[3], sTextToSend, sizeof(ingame.phrases[3]));
		}
		else
		{
			strlcpy(sTextToSend, ingame.phrases[3], sizeof(sTextToSend));
			bAllowOtherKeyPresses = TRUE;
		//	flushConsoleMessages();
			sendTextMessage(sTextToSend,FALSE);
			return;
		}
	}
	if (keyPressed(KEY_F5))
	{
		if(keyDown(KEY_LCTRL))
		{
			strlcpy(ingame.phrases[4], sTextToSend, sizeof(ingame.phrases[4]));
		}
		else
		{
			strlcpy(sTextToSend, ingame.phrases[4], sizeof(sTextToSend));
			bAllowOtherKeyPresses = TRUE;
		 //	flushConsoleMessages();
			sendTextMessage(sTextToSend,FALSE);
			return;
		}
	}

//	flushConsoleMessages();								//clear
//	addConsoleMessage(sTextToSend,DEFAULT_JUSTIFY);		//display
//	iV_DrawText(sTextToSend,16+D_W,RADTLY+D_H-16);
}
// --------------------------------------------------------------------------
void	kf_ToggleConsole( void )
{
	if(getConsoleDisplayStatus())
	{
		enableConsoleDisplay(FALSE);
	}
	else
	{
		enableConsoleDisplay(TRUE);
	}
}
// --------------------------------------------------------------------------
void	kf_SelectAllOnScreenUnits( void )
{

	selDroidSelection(selectedPlayer, DS_ALL_UNITS, DST_UNUSED, TRUE);

/*
DROID	*psDroid;
UDWORD	dX,dY;

	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (DrawnInLastFrame(psDroid->sDisplay.frameNumber)==TRUE)
		{
			dX = psDroid->sDisplay.screenX;
			dY = psDroid->sDisplay.screenY;
			if(dX>0 && dY>0 && dX<DISP_WIDTH && dY<DISP_HEIGHT)
			{
				psDroid->selected = TRUE;
			}
		}
	}
*/
}
// --------------------------------------------------------------------------
void	kf_SelectAllUnits( void )
{

	selDroidSelection(selectedPlayer, DS_ALL_UNITS, DST_UNUSED, FALSE);

/*
DROID	*psDroid;
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		psDroid->selected = TRUE;
	}
*/
}
// --------------------------------------------------------------------------
void	kf_SelectAllVTOLs( void )
{
  //	kfsf_SelectAllSameProp(LIFT);
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_VTOL,FALSE);
}
// --------------------------------------------------------------------------
void	kf_SelectAllHovers( void )
{
//	kfsf_SelectAllSameProp(HOVER);
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_HOVER,FALSE);
}
// --------------------------------------------------------------------------
void	kf_SelectAllWheeled( void )
{
//	kfsf_SelectAllSameProp(WHEELED);
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_WHEELED,FALSE);
}
// --------------------------------------------------------------------------
void	kf_SelectAllTracked( void )
{
//	kfsf_SelectAllSameProp(TRACKED);
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_TRACKED,FALSE);
}
// --------------------------------------------------------------------------
void	kf_SelectAllHalfTracked( void )
{
//	kfsf_SelectAllSameProp(HALF_TRACKED);
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_HALF_TRACKED,FALSE);
}

// --------------------------------------------------------------------------
void	kf_SelectAllDamaged( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_DAMAGED,FALSE);
/*
DROID	*psDroid;
UDWORD	damage;

	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		damage = PERCENT(psDroid->body,psDroid->originalBody);
		if(damage<REPAIRLEV_LOW)
		{
			psDroid->selected = TRUE;
		}
	}
*/
}
// --------------------------------------------------------------------------
void	kf_SelectAllCombatUnits( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_COMBAT,FALSE);

/*
DROID	*psDroid;
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->numWeaps)
		{
			psDroid->selected = TRUE;
		}
	}
*/
}
// --------------------------------------------------------------------------
void	kfsf_SelectAllSameProp( PROPULSION_TYPE propType )
{
	/*
PROPULSION_STATS	*psPropStats;
DROID	*psDroid;

	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if(!psDroid->selected)
		{
			psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
			ASSERT( psPropStats != NULL,
					"moveUpdateDroid: invalid propulsion stats pointer" );
			if ( psPropStats->propulsionType == propType )
			{
				psDroid->selected = TRUE;
			}
		}
	}
	*/
}
// --------------------------------------------------------------------------
// this is worst case (size of apsDroidLists[selectedPlayer] squared).
// --------------------------------------------------------------------------
void	kf_SelectAllSameType( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_SAME,FALSE);
	/*
DROID	*psDroid;
//PROPULSION_STATS	*psPropStats;

	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->selected)
		{
//			psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
//			kfsf_SelectAllSameProp(psPropStats->propulsionType);	// non optimal - multiple assertion!?
			kfsf_SelectAllSameName(psDroid->aName);
		}
	}
	*/
}
// --------------------------------------------------------------------------
void	kfsf_SelectAllSameName( char *droidName )
{
	/*
DROID	*psDroid;
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		// already selected - ignore
		if(!psDroid->selected)
		{
			if(!strcmp(droidName,psDroid->aName))
			{
				psDroid->selected = TRUE;
			}
		}
	}
	*/
}
// --------------------------------------------------------------------------
void	kf_SetDroidRangeShort( void )
{
	kfsf_SetSelectedDroidsState(DSO_ATTACK_RANGE,DSS_ARANGE_SHORT);
}
// --------------------------------------------------------------------------
void	kf_SetDroidRangeDefault( void )
{
	kfsf_SetSelectedDroidsState(DSO_ATTACK_RANGE,DSS_ARANGE_DEFAULT);
}
// --------------------------------------------------------------------------
void	kf_SetDroidRangeLong( void )
{
	kfsf_SetSelectedDroidsState(DSO_ATTACK_RANGE,DSS_ARANGE_LONG);
}

// --------------------------------------------------------------------------
void	kf_SetDroidRetreatMedium( void )
{
	kfsf_SetSelectedDroidsState(DSO_REPAIR_LEVEL,DSS_REPLEV_LOW);
}
// --------------------------------------------------------------------------
void	kf_SetDroidRetreatHeavy( void )
{
	kfsf_SetSelectedDroidsState(DSO_REPAIR_LEVEL,DSS_REPLEV_HIGH);
}
// --------------------------------------------------------------------------
void	kf_SetDroidRetreatNever( void )
{
	kfsf_SetSelectedDroidsState(DSO_REPAIR_LEVEL,DSS_REPLEV_NEVER);
}
// --------------------------------------------------------------------------
void	kf_SetDroidAttackAtWill( void )
{
	kfsf_SetSelectedDroidsState(DSO_ATTACK_LEVEL,DSS_ALEV_ALWAYS);
}
// --------------------------------------------------------------------------
void	kf_SetDroidAttackReturn( void )
{
	kfsf_SetSelectedDroidsState(DSO_ATTACK_LEVEL,DSS_ALEV_ATTACKED);
}
// --------------------------------------------------------------------------
void	kf_SetDroidAttackCease( void )
{
	kfsf_SetSelectedDroidsState(DSO_ATTACK_LEVEL,DSS_ALEV_NEVER);
}

// --------------------------------------------------------------------------
void	kf_SetDroidMoveHold( void )
{
	kfsf_SetSelectedDroidsState(DSO_HALTTYPE,DSS_HALT_HOLD);
}
// --------------------------------------------------------------------------
void	kf_SetDroidMovePursue( void )
{
	kfsf_SetSelectedDroidsState(DSO_HALTTYPE,DSS_HALT_PERSUE);	// ASK?
}
// --------------------------------------------------------------------------
void	kf_SetDroidMovePatrol( void )
{
	kfsf_SetSelectedDroidsState(DSO_PATROL,DSS_PATROL_SET);	// ASK
}
// --------------------------------------------------------------------------
void	kf_SetDroidReturnToBase( void )
{
	kfsf_SetSelectedDroidsState(DSO_RETURN_TO_LOC,DSS_RTL_BASE);
}
// --------------------------------------------------------------------------
void	kf_SetDroidGoForRepair( void )
{
	kfsf_SetSelectedDroidsState(DSO_RETURN_TO_LOC,DSS_RTL_REPAIR);
}
// --------------------------------------------------------------------------
void	kf_SetDroidRecycle( void )
{
	kfsf_SetSelectedDroidsState(DSO_RECYCLE,DSS_RECYCLE_SET);
}
// --------------------------------------------------------------------------
void	kf_ToggleVisibility( void )
{
	if(getRevealStatus())
	{
		setRevealStatus(FALSE);
	}
	else
	{
		setRevealStatus(TRUE);
	}

}
// --------------------------------------------------------------------------
void	kfsf_SetSelectedDroidsState( SECONDARY_ORDER sec, SECONDARY_STATE state )
{
DROID	*psDroid;

	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->selected)
		{
			secondarySetState(psDroid,sec,state);
			/* Kick him out of group if he's going for repair */	  // Now done in secondarySetState
		   //	if ((sec == DSO_RETURN_TO_LOC) && (state == DSS_RTL_REPAIR))
		   //	{
		   //		psDroid->group = UBYTE_MAX;
		   //		psDroid->selected = FALSE;
		   //	}
		}
	}
}
// --------------------------------------------------------------------------
void	kf_TriggerRayCast( void )
{
DROID	*psDroid;
BOOL	found;
DROID	*psOther;

	found = FALSE;
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid && !found;
		psDroid = psDroid->psNext)
		{
			if(psDroid->selected)
			{
				found = TRUE;
				psOther = psDroid;
			}
			/* NOP */
		}

	if(found)
	{
//		 getBlockHeightDirToEdgeOfGrid(UDWORD x, UDWORD y, UBYTE direction, UDWORD *height, UDWORD *dist)
   //		getBlockHeightDirToEdgeOfGrid(psOther->pos.x,psOther->pos.y,psOther->direction,&height,&dist);
//		getBlockHeightDirToEdgeOfGrid(mouseTileX*TILE_UNITS,mouseTileY*TILE_UNITS,getTestAngle(),&height,&dist);
	}
}
// --------------------------------------------------------------------------

void	kf_ScatterDroids( void )
{
	// to be written!
	addConsoleMessage("Scatter droids - not written yet!",LEFT_JUSTIFY);
}// --------------------------------------------------------------------------
void	kf_CentreOnBase( void )
{
STRUCTURE	*psStruct;
BOOL		bGotHQ;
UDWORD		xJump = 0, yJump = 0;

	/* Got through our buildings */
	for(psStruct = apsStructLists[selectedPlayer],bGotHQ = FALSE;	// start
	psStruct && !bGotHQ;											// terminate
	psStruct = psStruct->psNext)									// iteration
	{
		/* Have we got a HQ? */
		if(psStruct->pStructureType->type == REF_HQ)
		{
			bGotHQ = TRUE;
			xJump = (psStruct->pos.x - ((visibleTiles.x/2)*TILE_UNITS));
			yJump = (psStruct->pos.y - ((visibleTiles.y/2)*TILE_UNITS));
		}
	}

	/* If we found it, then jump to it! */
	if(bGotHQ)
	{
		addConsoleMessage(_("Centered on player HQ, direction NORTH"),LEFT_JUSTIFY);
		player.p.x = xJump;
		player.p.z = yJump;
		player.r.y = 0; // face north
		/* A fix to stop the camera continuing when marker code is called */
		if(getWarCamStatus())
		{
			camToggleStatus();
		}
	}
	else
	{
		addConsoleMessage(_("Unable to locate HQ!"),LEFT_JUSTIFY);
	}
}

// --------------------------------------------------------------------------

void kf_ToggleFormationSpeedLimiting( void )
{

	if(bMultiPlayer)
	{
		return;
	}

	if ( moveFormationSpeedLimitingOn() )
	{
		addConsoleMessage(_("Formation speed limiting OFF"),LEFT_JUSTIFY);
	}
	else
	{
		addConsoleMessage(_("Formation speed limiting ON"),LEFT_JUSTIFY);
	}
	moveToggleFormationSpeedLimiting();
}

// --------------------------------------------------------------------------
void	kf_RightOrderMenu( void )
{
DROID	*psDroid,*psGotOne = NULL;
BOOL	bFound;

	// if menu open, then close it!
	if (widgGetFromID(psWScreen,IDORDER_FORM) != NULL)
	{
		intRemoveOrder();	// close the screen.
		return;
	}


	for(psDroid = apsDroidLists[selectedPlayer],bFound = FALSE;
		psDroid && !bFound; psDroid = psDroid->psNext)
	{
		if(psDroid->selected)// && droidOnScreen(psDroid,0))
		{
			bFound = TRUE;
			psGotOne = psDroid;
		}
	}
	if(bFound)
	{
		intResetScreen(TRUE);
		intObjectSelected((BASE_OBJECT*)psGotOne);
	}
}

// --------------------------------------------------------------------------
void kf_TriggerShockWave( void )
{
	Vector3i pos;

	pos.x = mouseTileX*TILE_UNITS + TILE_UNITS/2;
	pos.z = mouseTileY*TILE_UNITS + TILE_UNITS/2;
	pos.y = map_Height(pos.x,pos.z) + SHOCK_WAVE_HEIGHT;

	addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SHOCKWAVE,FALSE,NULL,0);
}
// --------------------------------------------------------------------------
void	kf_ToggleMouseInvert( void )
{
	if(getInvertMouseStatus())
	{
		setInvertMouseStatus(FALSE);
	}
	else
	{
		setInvertMouseStatus(TRUE);
	}
}
// --------------------------------------------------------------------------
void	kf_ToggleShakeStatus( void )
{
	if(getShakeStatus())
	{
		setShakeStatus(FALSE);
	}
	else
	{
		setShakeStatus(TRUE);
	}

}
// --------------------------------------------------------------------------
void	kf_ToggleShadows( void )
{
	if(getDrawShadows())
	{
		setDrawShadows(FALSE);
	}
	else
	{
		setDrawShadows(TRUE);
	}

}
// --------------------------------------------------------------------------

float available_speed[] = {
	FRACTCONST(1, 8),
	FRACTCONST(1, 4),
	FRACTCONST(1, 2),
	FRACTCONST(3, 4),
	FRACTCONST(1, 1),
	FRACTCONST(3, 2),
	FRACTCONST(2, 1),
	FRACTCONST(5, 2),
	FRACTCONST(3, 1),
	FRACTCONST(10, 1),
	FRACTCONST(20, 1)
};
unsigned int nb_available_speeds = 11;

void kf_SpeedUp( void )
{
	float	mod;

	if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
		unsigned int i;

		// get the current modifier
		gameTimeGetMod(&mod);

		for (i = 1; i < nb_available_speeds; ++i) {
			if (mod < available_speed[i]) {
				mod = available_speed[i];

				if (mod == FRACTCONST(1, 1)) {
					CONPRINTF(ConsoleString,(ConsoleString,_("Game Speed Reset")));
				} else {
					CONPRINTF(ConsoleString,(ConsoleString,_("Game Speed Increased to %3.1f"),mod));
				}
				gameTimeSetMod(mod);
				break;
			}
		}
	}
}

void kf_SlowDown( void )
{
	float	mod;

	if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
		int i;

		// get the current modifier
		gameTimeGetMod(&mod);

		for (i = nb_available_speeds-2; i >= 0; --i) {
			if (mod > available_speed[i]) {
				mod = available_speed[i];

				if (mod == FRACTCONST(1, 1)) {
					CONPRINTF(ConsoleString,(ConsoleString,_("Game Speed Reset")));
				} else {
					CONPRINTF(ConsoleString,(ConsoleString,_("Game Speed Reduced to %3.1f"),mod));
				}
				gameTimeSetMod(mod);
				break;
			}
		}
	}
}

void kf_NormalSpeed( void )
{
	if ( (!bMultiPlayer || (NetPlay.bComms == 0)) && !bInTutorial)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("Game Speed Reset")));
		gameTimeResetMod();
	}
}

// --------------------------------------------------------------------------

void kf_ToggleReopenBuildMenu( void )
{
	intReopenBuild( !intGetReopenBuild() );

	if (intGetReopenBuild())
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("Build menu will reopen")));
	}
	else
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("Build menu will not reopen")));
	}
}

// --------------------------------------------------------------------------

void kf_ToggleRadarAllyEnemy(void)
{
	bEnemyAllyRadarColor = !bEnemyAllyRadarColor;
	resetRadarRedraw();
}

void kf_ToggleRadarTerrain(void)
{
	radarDrawMode = radarDrawMode + 1;

	if (radarDrawMode >= NUM_RADAR_MODES)
	{
		radarDrawMode = 0;
	}
	switch (radarDrawMode)
	{
		case RADAR_MODE_NO_TERRAIN:
		 	CONPRINTF(ConsoleString, (ConsoleString, "Radar showing only objects"));
			break;
		case RADAR_MODE_COMBINED:
		 	CONPRINTF(ConsoleString, (ConsoleString, "Radar blending terrain and height"));
			break;
		case RADAR_MODE_TERRAIN:
		 	CONPRINTF(ConsoleString, (ConsoleString, "Radar showing terrain"));
			break;
		case RADAR_MODE_HEIGHT_MAP:
		 	CONPRINTF(ConsoleString, (ConsoleString, "Radar showing height"));
			break;
		case NUM_RADAR_MODES:
			assert(false);
			break;
	}

	resetRadarRedraw();
}


//Returns TRUE if the engine should dofurther text processing, FALSE if just exit
BOOL	processConsoleCommands( char *pName )
{
#ifdef DEBUG
	BOOL	bFound = FALSE;
	SDWORD	i;

	if(strcmp(pName,"/loadai") == FALSE)
	{
		(void)LoadAIExperience(TRUE);
		return TRUE;
	}
	else if(strcmp(pName,"/saveai") == FALSE)
	{
		(void)SaveAIExperience(TRUE);
		return TRUE;
	}
	else if(strcmp(pName,"/maxplayers") == FALSE)
	{
		console("game.maxPlayers: &d", game.maxPlayers);
		return TRUE;
	}
	else if(strcmp(pName,"/bd") == FALSE)
	{
		BaseExperienceDebug(selectedPlayer);

		return TRUE;
	}
	else if(strcmp(pName,"/sm") == FALSE)
	{
		for(i=0; i<MAX_PLAYERS;i++)
		{
			console("%d - %d", i, game.skDiff[i]);
		}

		return TRUE;
	}
	else if(strcmp(pName,"/od") == FALSE)
	{
		OilExperienceDebug(selectedPlayer);

		return TRUE;
	}
	else
	{
		char tmpStr[255];



		/* saveai x */
		for(i=0;i<MAX_PLAYERS;i++)
		{
			sprintf(tmpStr,"/saveai %d", i);		//"saveai 0"
			if(strcmp(pName,tmpStr) == FALSE)
			{
				SavePlayerAIExperience(i, TRUE);
				return TRUE;
			}
		}

		/* loadai x */
		for(i=0;i<MAX_PLAYERS;i++)
		{
			sprintf(tmpStr,"/loadai %d", i);		//"loadai 0"
			if(strcmp(pName,tmpStr) == FALSE)
			{
				LoadPlayerAIExperience(i, TRUE);
				return TRUE;
			}
		}
	}
	return bFound;
#else
	return FALSE;
#endif
}

//Add a beacon (blip)
void	kf_AddHelpBlip( void )
{
	UDWORD 	worldX,worldY;
	UDWORD	i;
	char	tempStr[255];
	SDWORD	x,y;
	BOOL	mOverR=FALSE;

	/* not needed in campaign */
	if(!bMultiPlayer)
		return;

	debug(LOG_WZ,"Adding beacon='%s'",sCurrentConsoleText);

	/* check if clicked on radar */
	x = mouseX();
	y = mouseY();
	if(radarOnScreen && getHQExists(selectedPlayer))
	{
		if(CoordInRadar(x,y))
		{
			mOverR = TRUE;
			CalcRadarPosition(x,y,&worldX,&worldY);

			worldX = worldX*TILE_UNITS+TILE_UNITS/2;
			worldY = worldY*TILE_UNITS+TILE_UNITS/2;
			//printf_console("Radar, x: %d, y: %d", worldX, worldY);
		}
	}

	/* convert screen to world */
	if(!mOverR)
	{
		worldX = mouseTileX*TILE_UNITS+TILE_UNITS/2;
		worldY = mouseTileY*TILE_UNITS+TILE_UNITS/2;
	}

	//if chat message is empty, just send player name
	//if(!strcmp(sCurrentConsoleText, ""))
	//{
		strlcpy(tempStr, getPlayerName(selectedPlayer), sizeof(tempStr));		//temporary solution
	//}
	//else
	//{
	//	strlcpy(tempStr, sCurrentConsoleText, sizeof(tempStr));
	//}


	/* add beacon for the sender */
	strlcpy(beaconMsg[selectedPlayer], tempStr, sizeof(beaconMsg[selectedPlayer]));
	sendBeaconToPlayer(worldX, worldY, selectedPlayer, selectedPlayer, beaconMsg[selectedPlayer]);

	/* send beacon to other players */
	for(i=0;i<game.maxPlayers;i++)
	{
		if(openchannels[i] && (i != selectedPlayer))
		{
			strlcpy(beaconMsg[i], tempStr, sizeof(beaconMsg[i]));
			sendBeaconToPlayer(worldX, worldY, i, selectedPlayer, beaconMsg[i]);
		}
	}
}
