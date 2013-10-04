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
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/stdio_ext.h"
#include "lib/framework/utf.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/rational.h"
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
#include "structure.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"

#include "cheat.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multimenu.h"
#include "atmos.h"
#include "advvis.h"
#include "difficulty.h"

#include "intorder.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/bar.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/listwidget.h"
#include "order.h"
#include "lib/ivis_opengl/piestate.h"
// FIXME Direct iVis implementation include!
#include "lib/framework/fixedpoint.h"
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
#include "scriptfuncs.h"
#include "clparse.h"
#include "research.h"
#include "template.h"
#include "qtscript.h"
#include "multigifts.h"

/*
	KeyBind.c
	Holds all the functions that can be mapped to a key.
	All functions at the moment must be 'void func(void)'.
	Alex McLean, Pumpkin Studios, EIDOS Interactive.
*/

#define	MAP_ZOOM_RATE	(1000)
#define MAP_PITCH_RATE	(SPIN_SCALING/SECS_PER_SPIN)

extern char	ScreenDumpPath[];

bool	bMovePause = false;
bool		bAllowOtherKeyPresses = true;
char	sTextToSend[MAX_CONSOLE_STRING_LENGTH];
char	beaconMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];		//beacon msg for each player

static STRUCTURE	*psOldRE = NULL;
static char	sCurrentConsoleText[MAX_CONSOLE_STRING_LENGTH];			//remember what user types in console for beacon msg

/* Support functions to minimise code size */
static void kfsf_SetSelectedDroidsState( SECONDARY_ORDER sec, SECONDARY_STATE State );

/** A function to determine wether we're running a multiplayer game, not just a
 *  single player campaign or a skirmish game.
 *  \return false if this is a skirmish or single player game, true if it is a
 *          multiplayer game.
 */
bool runningMultiplayer(void)
{
	if (!bMultiPlayer || !NetPlay.bComms)
		return false;

	return true;
}

static void noMPCheatMsg(void)
{
	addConsoleMessage(_("Sorry, that cheat is disabled in multiplayer games."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------
void kf_AutoGame(void)
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif
	if (game.type == CAMPAIGN)
	{
		CONPRINTF(ConsoleString, (ConsoleString, "Not possible with the campaign!"));
		return;
	}
	// Notify all human players that we are trying to enable autogame
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if(NetPlay.players[i].allocated)
		{
			sendGift(AUTOGAME_GIFT, i);
		}
	}

	CONPRINTF(ConsoleString, (ConsoleString, "autogame request has been sent to all players. AI script *must* support this command!"));
}

void	kf_ToggleMissionTimer( void )
{
	addConsoleMessage(_("Warning! This cheat is buggy.  We recommend to NOT use it."), DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);
	setMissionCheatTime(!mission.cheatTime);
}

void	kf_ToggleShowGateways(void)
{
	addConsoleMessage("Gateways toggled.", DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);
	showGateways = !showGateways;
}

void	kf_ToggleShowPath(void)
{
	addConsoleMessage("Path display toggled.", DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	showPath = !showPath;
}

// --------------------------------------------------------------------------
void	kf_ToggleRadarJump( void )
{
	setRadarJump(!getRadarJumpStatus());
}

// --------------------------------------------------------------------------

void kf_ForceDesync(void)
{
	syncDebug("Oh no!!! I went out of sync!!!");
}

void	kf_PowerInfo( void )
{
	int i;

	for (i = 0; i < game.maxPlayers; i++)
	{
		console("Player %d: %d power", i, (int)getPower(i));
	}
}

void kf_DamageMe(void)
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif
	for(DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			int val = psDroid->body - ((psDroid->originalBody / 100) *20);
			if (val > 0)
			{
				psDroid->body = val;
				addConsoleMessage(_("Ouch! Droid's health is down 20%!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
			}
			else
			{
				psDroid->body = 0;
			}
		}
	}
	for(STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->selected)
		{
			int val = psStruct->body - ((structureBody(psStruct) / 100) *20);
			if (val > 0)
			{
				psStruct->body = val;
				addConsoleMessage(_("Ouch! Structure's health is down 20%!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
			}
			else
			{
				psStruct->body = 0;
			}
		}
	}
}

void	kf_TraceObject( void )
{
	DROID		*psCDroid, *psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;

	for(psCDroid = apsDroidLists[selectedPlayer]; psCDroid; psCDroid = psNDroid)
	{
		psNDroid = psCDroid->psNext;
		if (psCDroid->selected)
		{
			objTraceEnable(psCDroid->id);
			CONPRINTF(ConsoleString, (ConsoleString, "Tracing droid %d", (int)psCDroid->id));
			return;
		}
	}
	for(psCStruct = apsStructLists[selectedPlayer]; psCStruct; psCStruct = psNStruct)
	{
		psNStruct = psCStruct->psNext;
		if (psCStruct->selected)
		{
			objTraceEnable(psCStruct->id);
			CONPRINTF(ConsoleString, (ConsoleString, "Tracing structure %d", (int)psCStruct->id));
			return;
		}
	}
	objTraceDisable();
	CONPRINTF(ConsoleString, (ConsoleString, "No longer tracing anything."));
}

//===================================================
void kf_ToggleSensorDisplay( void )
{

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	rangeOnScreen = !rangeOnScreen;

	if (rangeOnScreen)
		addConsoleMessage(_("Lets us see what you see!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);        //added this message... Yeah, its lame. :)
	else
		addConsoleMessage(_("Fine, weapon & sensor display is off!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);     //added this message... Yeah, its lame. :)
}
//===================================================
/* Halves all the heights of the map tiles */
void	kf_HalveHeights( void )
{
UDWORD	i,j;
MAPTILE	*psTile;

	for (i=0; i < mapWidth; i++)
	{
		for (j=0; j < mapHeight; j++)
		{
			psTile = mapTile(i,j);
			psTile->height/=2;
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

void	kf_CloneSelected( void )
{
	DROID_TEMPLATE	*sTemplate = NULL;
	const int	limit = 10;	// make 10 clones
	const char *msg;
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
	{
		if (psDroid->selected)
		{
			for (DROID_TEMPLATE *psTempl = apsDroidTemplates[selectedPlayer]; psTempl; psTempl = psTempl->psNext)
			{
				if (psTempl->name.compare(psDroid->aName) == 0)
				{
					sTemplate = psTempl;
					break;
				}
			}

			if (!sTemplate)
			{
				debug(LOG_ERROR, "Cloning vat has been destoryed. We can't find the template for this droid: %s, id:%u, type:%d!", psDroid->aName, psDroid->id, psDroid->droidType);
				return;
			}

			// create a new droid army
			for (int i = 0; i < limit; i++)
			{
				DROID *psNewDroid = buildDroid(sTemplate, psDroid->pos.x + (i*12), psDroid->pos.y + (i*14), psDroid->player, false, NULL);
				if (psNewDroid)
				{
					addDroid(psNewDroid, apsDroidLists);
					psScrCBNewDroid = psNewDroid;
					psScrCBNewDroidFact = NULL;
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NEWDROID);	// notify scripts so it will get assigned jobs
					psScrCBNewDroid = NULL;
					triggerEventDroidBuilt(psNewDroid, NULL);
				}
				else
				{
					debug(LOG_ERROR, "Cloning has failed for template:%s id:%d", getID(sTemplate), sTemplate->multiPlayerID);
				}
			}
			sasprintf((char**)&msg, _("Player %u is cheating a new droid army of: %s."), selectedPlayer, psDroid->aName);
			sendTextMessage(msg, true);
			Cheated = true;
			audio_PlayTrack(ID_SOUND_NEXUS_LAUGH1);
			return;
		}
		debug(LOG_INFO, "Nothing was selected?");
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
	const char* cmsg;

	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	setDifficultyLevel(DL_KILLER);
	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("Hard as nails!!!"));
	sendTextMessage(cmsg, true);
}
// --------------------------------------------------------------------------
void	kf_SetEasyLevel( void )
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	setDifficultyLevel(DL_EASY);
	addConsoleMessage(_("Takings thing easy!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------
void	kf_UpThePower( void )
{
	const char* cmsg;

	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
	addPower(selectedPlayer, 1000);
	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("1000 big ones!!!"));
	sendTextMessage(cmsg, true);
}

// --------------------------------------------------------------------------
void	kf_MaxPower( void )
{
	const char* cmsg;

	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
	setPower(selectedPlayer, 100000);
	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("Power overwhelming"));
	sendTextMessage(cmsg, true);
}

// --------------------------------------------------------------------------
void	kf_SetNormalLevel( void )
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	setDifficultyLevel(DL_NORMAL);
	addConsoleMessage(_("Back to normality!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
}
// --------------------------------------------------------------------------
void	kf_SetHardLevel( void )
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	setDifficultyLevel(DL_HARD);
	addConsoleMessage(_("Getting tricky!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
}
// --------------------------------------------------------------------------
void	kf_SetToughUnitsLevel( void )
{
	const char* cmsg;

	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	setDifficultyLevel(DL_TOUGH);
	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("Twice as nice!"));
	sendTextMessage(cmsg, true);
}
// --------------------------------------------------------------------------
void kf_ToggleFPS(void) //This shows *just FPS* and is always visable (when active) -Q.
{
	// Toggle the boolean value of showFPS
	showFPS = !showFPS;

	if (showFPS)
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("FPS display is enabled.")));
	}
	else
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("FPS display is disabled.")));
	}
}
void kf_ToggleSamples(void) //Displays number of sound sample in the sound queues & lists.
{
	// Toggle the boolean value of showSAMPLES
	showSAMPLES = !showSAMPLES;

	CONPRINTF(ConsoleString, (ConsoleString, "Sound Samples displayed is %s", showSAMPLES ? "Enabled" : "Disabled"));
}

void kf_ToggleOrders(void)	// Displays orders & action of currently selected unit.
{
		// Toggle the boolean value of showORDERS
		showORDERS = !showORDERS;
		CONPRINTF(ConsoleString, (ConsoleString, "Unit Order/Action displayed is %s", showORDERS ? "Enabled" : "Disabled"));
}

void kf_ToggleLevelName(void) // toggles level name 
{
	showLevelName = !showLevelName;
}

/* Writes out the frame rate */
void	kf_FrameRate( void )
{
	CONPRINTF(ConsoleString,(ConsoleString, "FPS %d; PIEs %d; polys %d; States %d",
	          frameRate(), loopPieCount, loopPolyCount, loopStateChanges));
	if (runningMultiplayer())
	{
		CONPRINTF(ConsoleString, (ConsoleString, "NETWORK:  Bytes: s-%d r-%d  Uncompressed Bytes: s-%d r-%d  Packets: s-%d r-%d",
		                          NETgetStatistic(NetStatisticRawBytes, true),
		                          NETgetStatistic(NetStatisticRawBytes, false),
		                          NETgetStatistic(NetStatisticUncompressedBytes, true),
		                          NETgetStatistic(NetStatisticUncompressedBytes, false),
		                          NETgetStatistic(NetStatisticPackets, true),
		                          NETgetStatistic(NetStatisticPackets, false)));
	}
	gameStats = !gameStats;
	CONPRINTF(ConsoleString, (ConsoleString,"Built at %s on %s",__TIME__,__DATE__));
}

// --------------------------------------------------------------------------

// display the total number of objects in the world
void kf_ShowNumObjects( void )
{
	int droids, structures, features;
	const char* cmsg;

	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	objCount(&droids, &structures, &features);
	sasprintf((char**)&cmsg, _("(Player %u) is using a cheat :Num Droids: %d  Num Structures: %d  Num Features: %d"),
				selectedPlayer, droids, structures, features);
	sendTextMessage(cmsg, true);
}

// --------------------------------------------------------------------------

/* Toggles radar on off */
void	kf_ToggleRadar( void )
{
		radarOnScreen = !radarOnScreen;
}

// --------------------------------------------------------------------------

/* Toggles infinite power on/off */
void	kf_TogglePower( void )
{
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	powerCalculated = !powerCalculated;
	if (powerCalculated)
	{
		powerCalc(true);
	}

	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
		selectedPlayer, powerCalculated ? _("Infinite power disabled"): _("Infinite power enabled") );
	sendTextMessage(cmsg, true);
}

// --------------------------------------------------------------------------

/* Recalculates the lighting values for a tile */
void	kf_RecalcLighting( void )
{
		initLighting(0, 0, mapWidth, mapHeight);
		addConsoleMessage("Lighting values for all tiles recalculated",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------

/* Sends the screen buffer to disk */
void	kf_ScreenDump( void )
{
	//CONPRINTF(ConsoleString,(ConsoleString,"Screen dump written to working directory : %s", screenDumpToDisk()));
	screenDumpToDisk(ScreenDumpPath);
}

// --------------------------------------------------------------------------

/* Make all functions available */
void	kf_AllAvailable( void )
{
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	makeAllAvailable();
	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("All items made available"));
	sendTextMessage(cmsg, true);
}

// --------------------------------------------------------------------------

/* Flips the cut of a tile */
void	kf_TriFlip( void )
{
	MAPTILE	*psTile;
	psTile = mapTile(mouseTileX,mouseTileY);
	TOGGLE_TRIFLIP(psTile);
//	addConsoleMessage("Triangle flip status toggled",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------

/* Debug info about a map tile */
void	kf_TileInfo(void)
{
	MAPTILE	*psTile = mapTile(mouseTileX, mouseTileY);

	debug(LOG_ERROR, "Tile position=(%d, %d) Terrain=%d Texture=%u Height=%d Illumination=%u",
	      mouseTileX, mouseTileY, (int)terrainType(psTile), TileNumber_tile(psTile->texture), psTile->height,
	      psTile->illumination);
	addConsoleMessage("Tile info dumped into log", DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------
void	kf_ToggleBackgroundFog( void )
{
	static bool bEnabled  = true;//start in nicks mode

	if (bEnabled)//true, so go to false
	{
		bEnabled = false;
		fogStatus &= FOG_FLAGS-FOG_BACKGROUND;//clear lowest bit of 3
		if (fogStatus == 0)
		{
			pie_SetFogStatus(false);
			pie_EnableFog(false);
		}
	}
	else
	{
		bEnabled = true;
		if (fogStatus == 0)
		{
			pie_EnableFog(true);
		}
		fogStatus |= FOG_BACKGROUND;//set lowest bit of 3
	}
}

extern void	kf_ToggleDistanceFog( void )
{
	static bool bEnabled  = true;//start in nicks mode

	if (bEnabled)//true, so go to false
	{
		bEnabled = false;
		fogStatus &= FOG_FLAGS-FOG_DISTANCE;//clear middle bit of 3
		if (fogStatus == 0)
		{
			pie_SetFogStatus(false);
			pie_EnableFog(false);
		}
	}
	else
	{
		bEnabled = true;
		if (fogStatus == 0)
		{
			pie_EnableFog(true);
		}
		fogStatus |= FOG_DISTANCE;//set lowest bit of 3
	}
}
/* Toggles fog on/off */
void	kf_ToggleFog( void )
{
	static bool fogEnabled = false;
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	if (fogEnabled)
	{
		fogEnabled = false;
		pie_SetFogStatus(false);
		pie_EnableFog(fogEnabled);
	}
	else
	{
		fogEnabled = true;
		pie_EnableFog(fogEnabled);
	}

	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
		selectedPlayer, fogEnabled ? _("Fog on") : _("Fog off") );
	sendTextMessage(cmsg, true);
}

// --------------------------------------------------------------------------

void	kf_ToggleWidgets( void )
{
	if(getWidgetsStatus())
	{
		setWidgetsStatus(false);
	}
	else
	{
		setWidgetsStatus(true);
	}
//	addConsoleMessage("Widgets display toggled",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------

/* Toggle camera on/off */
void	kf_ToggleCamera( void )
{
	if(getWarCamStatus() == false)
	{
		shakeStop();	// Ensure screen shake stopped before starting camera mode.
		setDrivingStatus(false);
	}
	camToggleStatus();
}

void kf_RevealMapAtPos()
{
	addSpotter(mouseTileX, mouseTileY, selectedPlayer, 1024, 0, gameTime + 2000);
}

// --------------------------------------------------------------------------

void kf_MapCheck(void)
{
	DROID		*psDroid;
	STRUCTURE	*psStruct;
	FLAG_POSITION	*psCurrFlag;

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
	}

	for (psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		alignStructure(psStruct);
	}

	for (psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
	{
		psCurrFlag->coords.z = map_Height(psCurrFlag->coords.x, psCurrFlag->coords.y) + ASSEMBLY_POINT_Z_PADDING;
	}
}

/* Raises the tile under the mouse */
void	kf_RaiseTile( void )
{
	if (runningMultiplayer())
	{
		return;  // Don't desynch if pressing 'W'...
	}

	raiseTile(mouseTileX, mouseTileY);
}

// --------------------------------------------------------------------------

/* Lowers the tile under the mouse */
void	kf_LowerTile( void )
{
	if (runningMultiplayer())
	{
		return;  // Don't desynch if pressing 'A'...
	}

	lowerTile(mouseTileX, mouseTileY);
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
	if (getDebugMappingStatus())
	{
		setViewDistance(getViewDistance() + realTimeAdjustedIncrement(MAP_ZOOM_RATE));
	}
	else
	{
		setViewDistance(std::min<int>(getViewDistance() + realTimeAdjustedIncrement(MAP_ZOOM_RATE), MAXDISTANCE));
	}
	UpdateFogDistance(getViewDistance());
}

void kf_ZoomOutStep(void)
{
	if (getDebugMappingStatus())
	{
		setViewDistance(getViewDistance() + MAP_ZOOM_RATE / 3);
	}
	else
	{
		setViewDistance(std::min<int>(getViewDistance() + MAP_ZOOM_RATE / 3, MAXDISTANCE));
	}
	UpdateFogDistance(getViewDistance());
}

// --------------------------------------------------------------------------
void	kf_RadarZoomIn( void )
{
	uint8_t RadarZoomLevel = GetRadarZoom();

	if(RadarZoomLevel < MAX_RADARZOOM)
	{
		RadarZoomLevel += RADARZOOM_STEP;
		SetRadarZoom(RadarZoomLevel);
		audio_PlayTrack( ID_SOUND_BUTTON_CLICK_5 );
	}
}
// --------------------------------------------------------------------------
void	kf_RadarZoomOut( void )
{
	uint8_t RadarZoomLevel = GetRadarZoom();

	if (RadarZoomLevel > MIN_RADARZOOM)
	{
		RadarZoomLevel -= RADARZOOM_STEP;
		SetRadarZoom(RadarZoomLevel);
		audio_PlayTrack( ID_SOUND_BUTTON_CLICK_5 );
	}
}
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
/* Zooms in the map */
void	kf_ZoomIn( void )
{
	if (getDebugMappingStatus())
	{
		setViewDistance(getViewDistance() - realTimeAdjustedIncrement(MAP_ZOOM_RATE));
	}
	else
	{
		setViewDistance(std::max<int>(getViewDistance() - realTimeAdjustedIncrement(MAP_ZOOM_RATE), MINDISTANCE));
	}
	UpdateFogDistance(getViewDistance());
}

void kf_ZoomInStep(void)
{
	if (getDebugMappingStatus())
	{
		setViewDistance(getViewDistance() - MAP_ZOOM_RATE / 3);
	}
	else
	{
		setViewDistance(std::max<int>(getViewDistance() - MAP_ZOOM_RATE / 3, MINDISTANCE));
	}
	UpdateFogDistance(getViewDistance());
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
	float rotAmount = realTimeAdjustedIncrement(MAP_SPIN_RATE);

	player.r.y += rotAmount;
}

// --------------------------------------------------------------------------
/* Spins the world right */
void	kf_RotateRight( void )
{
	float rotAmount = realTimeAdjustedIncrement(MAP_SPIN_RATE);

	player.r.y -= rotAmount;
	if (player.r.y < 0)
	{
		player.r.y += DEG(360);
	}
}

// --------------------------------------------------------------------------
/* Pitches camera back */
void	kf_PitchBack( void )
{
	float pitchAmount = realTimeAdjustedIncrement(MAP_PITCH_RATE);

	player.r.x += pitchAmount;

	if(player.r.x>DEG(360+MAX_PLAYER_X_ANGLE))
	{
		player.r.x = DEG(360+MAX_PLAYER_X_ANGLE);
	}
	setDesiredPitch(player.r.x/DEG_1);
}

// --------------------------------------------------------------------------
/* Pitches camera foward */
void	kf_PitchForward( void )
{
	float pitchAmount = realTimeAdjustedIncrement(MAP_PITCH_RATE);

	player.r.x -= pitchAmount;
	if (player.r.x < DEG(360 + MIN_PLAYER_X_ANGLE))
	{
		player.r.x = DEG(360 + MIN_PLAYER_X_ANGLE);
	}
	setDesiredPitch(player.r.x / DEG_1);
}

// --------------------------------------------------------------------------
/* Resets pitch to default */
void	kf_ResetPitch( void )
{
	player.r.x = DEG(360-20);
	setViewDistance(START_DISTANCE);
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
	// Bail out if we're running a _true_ multiplayer game (to prevent MP
	// cheating which could even result in undefined behaviour)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
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
	realSelectedPlayer = selectedPlayer;
	//	godMode = true;

	if (prevPlayer == selectedPlayer)
	{
		changeProductionPlayer((UBYTE)selectedPlayer);
	}
}
// --------------------------------------------------------------------------

/* Selects the player's groups 1..9 */
void	kf_SelectGrouping( UDWORD	groupNumber)
{
	bool	bAlreadySelected;
	DROID	*psDroid;
	bool	Selected;

	bAlreadySelected = false;
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid!=NULL; psDroid = psDroid->psNext)
	{
		/* Wipe out the ones in the wrong group */
		if(psDroid->selected && psDroid->group!=groupNumber)
		{
			psDroid->selected = false;
		}
		/* Get the right ones */
		if(psDroid->group == groupNumber)
		{
			if(psDroid->selected)
			{
				bAlreadySelected = true;
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
	triggerEventSelected();
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
		setWidgetsStatus(true);
		if (!isInGamePopupUp)	// they can *only* quit when popup is up.
		{
		intAddInGameOptions();
		}
}

// --------------------------------------------------------------------------
/* Tell the scripts to start a mission*/
void	kf_AddMissionOffWorld( void )
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MISSION_START);
}

// --------------------------------------------------------------------------
/* Tell the scripts to end a mission*/
void	kf_EndMissionOffWorld( void )
{
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	sasprintf((char**)&cmsg, _("Warning!  This cheat can cause dire problems later on! [%s]"), _("Ending Mission."));
	sendTextMessage(cmsg, true);

	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MISSION_END);
}

// --------------------------------------------------------------------------
// Display multiplayer guff.
void	kf_addMultiMenu(void)
{
	if (bMultiPlayer)
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
/* Toggles the power bar display on and off*/
void	kf_TogglePowerBar( void )
{
	togglePowerBar();
}
// --------------------------------------------------------------------------
/* Toggles whether we process debug key mappings */
void	kf_ToggleDebugMappings( void )
{
	sendProcessDebugMappings(!getWantedDebugMappingStatus(selectedPlayer));
}
// --------------------------------------------------------------------------

void	kf_ToggleGodMode( void )
{
	const char* cmsg;
	static bool pastReveal = true;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	if(godMode)
	{
		FEATURE	*psFeat = apsFeatureLists[0];
		int player;

		godMode = false;
		setRevealStatus(pastReveal);
		// now hide the features
		while (psFeat)
		{
			psFeat->visible[selectedPlayer] = 0;
			psFeat = psFeat->psNext;
		}

		// and the structures
		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			if (player != selectedPlayer)
			{
				STRUCTURE* psStruct = apsStructLists[player];
				
				while (psStruct)
				{
					psStruct->visible[selectedPlayer] = 0;
					psStruct = psStruct->psNext;
				}
			}
		}
		// remove all proximity messages
		releaseAllProxDisp();
	}
	else
	{
		godMode = true; // view all structures and droids
		revealAll(selectedPlayer);
		pastReveal = getRevealStatus();
		setRevealStatus(true); // view the entire map
	}

	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
		selectedPlayer, godMode ? _("God Mode ON") : _("God Mode OFF"));
	sendTextMessage(cmsg, true);
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

void kf_toggleTrapCursor(void)
{
	const char *msg;
	bool trap = !war_GetTrapCursor();
	war_SetTrapCursor(trap);
	(trap? wzGrabMouse : wzReleaseMouse)();
	sasprintf((char**)&msg, _("Trap cursor %s"), trap ? "ON" : "OFF");
	addConsoleMessage(msg, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}


// --------------------------------------------------------------------------
void	kf_TogglePauseMode( void )
{
	// Bail out if we're running a _true_ multiplayer game (which cannot be paused)
	if (runningMultiplayer())
	{
		return;
	}

	/* Is the game running? */
	if(gamePaused() == false)
	{
		/* Then pause it */
		setGamePauseStatus(true);
		setConsolePause(true);
		setScriptPause(true);
		setAudioPause(true);

		// If cursor trapping is enabled allow the cursor to leave the window
		if (war_GetTrapCursor())
		{
			wzReleaseMouse();
		}

		/* And stop the clock */
		gameTimeStop();
		addConsoleMessage(_("PAUSED"),CENTRE_JUSTIFY, SYSTEM_MESSAGE);

	}
	else
	{
		/* Else get it going again */
		setGamePauseStatus(false);
		setConsolePause(false);
		setScriptPause(false);
		setAudioPause(false);

		// Re-enable cursor trapping if it is enabled
		if (war_GetTrapCursor())
		{
			wzGrabMouse();
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
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	for (j = 0; j < asResearch.size(); j++)
	{
		if (IsResearchCompleted(&asPlayerResList[selectedPlayer][j]) == false)
		{
			if (bMultiMessages)
			{
				SendResearch(selectedPlayer, j, false);
				// Wait for our message before doing anything.
			}
			else
			{
				MakeResearchCompleted(&asPlayerResList[selectedPlayer][j]);
				researchResult(j, selectedPlayer, false, NULL, false);
			}
		}
	}
	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("Researched EVERYTHING for you!"));
	sendTextMessage(cmsg, true);
}

void kf_Reload(void)
{
	STRUCTURE	*psCurr;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	for (psCurr=interfaceStructList(); psCurr; psCurr = psCurr->psNext)
	{
		if (isLasSat(psCurr->pStructureType) && psCurr->selected)
		{
			unsigned int firePause = weaponFirePause(&asWeaponStats[psCurr->asWeaps[0].nStat], psCurr->player);

			psCurr->asWeaps[0].lastFired -= firePause;
			console("Selected buildings instantly recharged!");
		}
	}
}

// --------------------------------------------------------------------------
// finish all the research for the selected player
void	kf_FinishResearch( void )
{
	STRUCTURE	*psCurr;
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	for (psCurr=interfaceStructList(); psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_RESEARCH)
		{
			BASE_STATS	*pSubject = NULL;

			// find out what we are researching here
			pSubject = ((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject;
			if (pSubject)
			{
				int rindex = ((RESEARCH*)pSubject)->index;
				if (bMultiMessages)
				{
					SendResearch(selectedPlayer, rindex, true);
					// Wait for our message before doing anything.
				}
				else
				{
					researchResult(rindex, selectedPlayer, true, psCurr, true);
				}
				sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s %s"), selectedPlayer, _("Researched"), getName(pSubject));
				sendTextMessage(cmsg, true);
				intResearchFinished(psCurr);
			}
		}
	}
}

// --------------------------------------------------------------------------
//void	kf_ToggleRadarAllign( void )
//{
//	toggleRadarAllignment();
//	addConsoleMessage("Radar allignment toggled",LEFT_JUSTIFY, SYSTEM_MESSAGE);
//}

// --------------------------------------------------------------------------
void	kf_ToggleEnergyBars( void )
{
	switch (toggleEnergyBars())
	{
	case BAR_SELECTED:
		addConsoleMessage(_("Only displaying energy bars when selected"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		break;
	case BAR_DROIDS:
		addConsoleMessage(_("Always displaying energy bars for units"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		break;
	case BAR_DROIDS_AND_STRUCTURES:
		addConsoleMessage(_("Always displaying energy bars for units and structures"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		break;
	default:
		ASSERT(false, "Bad energy bar status");
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseOptions( void )
{
	const char* cmsg;

	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
			selectedPlayer, _("Debug menu is Open") );
	sendTextMessage(cmsg, true);
	intResetScreen(true);
	setWidgetsStatus(true);
	intAddOptions();
}

// --------------------------------------------------------------------------
void	kf_ToggleProximitys( void )
{
	setProximityDraw(!doWeDrawProximitys());
}
// --------------------------------------------------------------------------
void	kf_JumpToResourceExtractor( void )
{
	int xJump, yJump;

	if (psOldRE && (STRUCTURE *)psOldRE->psNextFunc)
	{
		psOldRE = psOldRE->psNextFunc;
	}
	else
	{
		psOldRE = apsExtractorLists[selectedPlayer];
	}

	if (psOldRE)
	{
		xJump = psOldRE->pos.x;
		yJump = psOldRE->pos.y;
		player.p.x = xJump;
		player.p.z = yJump;
		player.r.y = 0; // face north
		setViewPos(map_coord(psOldRE->pos.x), map_coord(psOldRE->pos.y), true);
	}
	else
	{
		addConsoleMessage(_("Unable to locate any oil derricks!"),LEFT_JUSTIFY, SYSTEM_MESSAGE);
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
		setWidgetsStatus(false);
	}
	else
	{
		setWidgetsStatus(true);
	}

}

void	kf_SensorDisplayOn( void )
{
	// Now a placeholder for future stuff
}

void	kf_SensorDisplayOff( void )
{
	// Now a placeholder for future stuff
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
	addConsoleMessage("Toggle driver mode", LEFT_JUSTIFY, SYSTEM_MESSAGE); // what does this do? - per

	/* No point unless we're tracking */
	if(getWarCamStatus())
	{
		if(getDrivingStatus())
		{
			StopDriverMode();
			addConsoleMessage("DriverMode off", LEFT_JUSTIFY, SYSTEM_MESSAGE);
		}
		else
		{	// removed the MP check for this, so you can now play with in in MP games.
			if (!driveModeActive())
			{
				StartDriverMode( NULL );
				addConsoleMessage("DriverMode on", LEFT_JUSTIFY, SYSTEM_MESSAGE);
			}
		}
	}
	else
	{
		addConsoleMessage("DriverMode disabled. Must be in tracking mode. Hit space bar with a unit selected.", LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

// --------------------------------------------------------------------------
void	kf_MovePause( void )
{

	if(!bMultiPlayer)	// can't do it in multiplay
	{
		if(!bMovePause)
		{
			/* Then pause it */
			setGamePauseStatus(true);
			setConsolePause(true);
			setScriptPause(true);
			setAudioPause(true);
			/* And stop the clock */
			gameTimeStop();
			setWidgetsStatus(false);
			radarOnScreen = false;
			bMovePause = true;
		}
		else
		{
			setWidgetsStatus(true);
			radarOnScreen = true;
			/* Else get it going again */
			setGamePauseStatus(false);
			setConsolePause(false);
			setScriptPause(false);
			setAudioPause(false);
			/* And start the clock again */
			gameTimeStart();
			bMovePause = false;
		}
	}
}

// --------------------------------------------------------------------------
void	kf_MoveToLastMessagePos( void )
{
	int iX, iY, iZ;

	if (!audio_GetPreviousQueueTrackPos( &iX, &iY, &iZ ))
	{
		return;
	}
	if (iX != 0 && iY != 0)
	{
		setViewPos(map_coord(iX), map_coord(iY), true);
	}
}

// --------------------------------------------------------------------------
/* Makes it snow if it's not snowing and stops it if it is */
void	kf_ToggleWeather( void )
{
	if(atmosGetWeatherType() == WT_NONE)
	{
		atmosSetWeatherType(WT_SNOWING);
		addConsoleMessage(_("Oh, the weather outside is frightful... SNOW"), LEFT_JUSTIFY, SYSTEM_MESSAGE);

	}
	else if(atmosGetWeatherType() == WT_SNOWING)
	{
		atmosSetWeatherType(WT_RAINING);
		addConsoleMessage(_("Singing in the rain, I'm singing in the rain... RAIN"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
	else
	{
		atmosInitSystem();
		atmosSetWeatherType(WT_NONE);
		addConsoleMessage(_("Forecast : Clear skies for all areas... NO WEATHER"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

// --------------------------------------------------------------------------
void	kf_SelectNextFactory(void)
{
STRUCTURE	*psCurr;

	selNextSpecifiedBuilding(REF_FACTORY);

	//deselect factories of other types
	for(psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if( psCurr->selected && 
		    ( (psCurr->pStructureType->type == REF_CYBORG_FACTORY) ||
			(psCurr->pStructureType->type == REF_VTOL_FACTORY) ) )
		{
		  psCurr->selected = false;
		}
	}

	if (intCheckReticuleButEnabled(IDRET_MANUFACTURE))
	{
		setKeyButtonMapping(IDRET_MANUFACTURE);
	}
	triggerEventSelected();
}

// --------------------------------------------------------------------------
void	kf_SelectNextResearch(void)
{
	selNextSpecifiedBuilding(REF_RESEARCH);
	if (intCheckReticuleButEnabled(IDRET_RESEARCH))
	{
		setKeyButtonMapping(IDRET_RESEARCH);
	}
	triggerEventSelected();
}

// --------------------------------------------------------------------------
void	kf_SelectNextPowerStation(void)
{
	selNextSpecifiedBuilding(REF_POWER_GEN);
	triggerEventSelected();
}

// --------------------------------------------------------------------------
void	kf_SelectNextCyborgFactory(void)
{
STRUCTURE	*psCurr;

	selNextSpecifiedBuilding(REF_CYBORG_FACTORY);

	//deselect factories of other types
	for(psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if( psCurr->selected && 
		    ( (psCurr->pStructureType->type == REF_FACTORY) ||
			(psCurr->pStructureType->type == REF_VTOL_FACTORY) ) )
		{
		  psCurr->selected = false;
		}
	}

	if (intCheckReticuleButEnabled(IDRET_MANUFACTURE))
	{
		setKeyButtonMapping(IDRET_MANUFACTURE);
	}
	triggerEventSelected();
}

// --------------------------------------------------------------------------


void	kf_KillEnemy( void )
{
	UDWORD		player;
	DROID		*psCDroid,*psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	debug(LOG_DEATH, "Destroying enemy droids and structures");
	CONPRINTF(ConsoleString, (ConsoleString,
		_("Warning! This can have drastic consequences if used incorrectly in missions.")));
	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("All enemies destroyed by cheating!"));
	sendTextMessage(cmsg, true);
	Cheated = true;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if(player!=selectedPlayer)
		{
		 	// wipe out all the droids
			for(psCDroid=apsDroidLists[player]; psCDroid; psCDroid=psNDroid)
			{
				psNDroid = psCDroid->psNext;
				SendDestroyDroid(psCDroid);
			}
			// wipe out all their structures
		  	for(psCStruct=apsStructLists[player]; psCStruct; psCStruct=psNStruct)
		  	{
		  		psNStruct = psCStruct->psNext;
				SendDestroyStructure(psCStruct);
		  	}
		}
	}
}

// kill all the selected objects
void kf_KillSelected(void)
{
	DROID		*psCDroid, *psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;
	const char* cmsg;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	sasprintf((char**)&cmsg, _("(Player %u) is using cheat :%s"),
				selectedPlayer, _("Destroying selected droids and structures!"));
	sendTextMessage(cmsg, true);

	debug(LOG_DEATH, "Destroying selected droids and structures");
	audio_PlayTrack(ID_SOUND_COLL_DIE);
	Cheated = true;

	for(psCDroid=apsDroidLists[selectedPlayer]; psCDroid; psCDroid=psNDroid)
	{
		psNDroid = psCDroid->psNext;
		if (psCDroid->selected)
		{
//			removeDroid(psCDroid);
			SendDestroyDroid(psCDroid);
		}
	}
	for(psCStruct=apsStructLists[selectedPlayer]; psCStruct; psCStruct=psNStruct)
	{
		psNStruct = psCStruct->psNext;
		if (psCStruct->selected)
		{
			SendDestroyStructure(psCStruct);
		}
	}
}

#if 0  // There's no gridDisplayCoverage anymore.
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
#endif

// --------------------------------------------------------------------------
// Chat message. NOTE THIS FUNCTION CAN DISABLE ALL OTHER KEYPRESSES
void kf_SendTextMessage(void)
{
	UDWORD	ch;
	utf_32_char unicode;

	if(bAllowOtherKeyPresses)									// just starting.
	{
		bAllowOtherKeyPresses = false;
		sstrcpy(sTextToSend, "");
		sstrcpy(sCurrentConsoleText, "");							//for beacons
		inputClearBuffer();
	}

	ch = inputGetKey(&unicode);
	while (ch != 0)												// in progress
	{
		// FIXME: Why are we using duplicate defines? INPBUF_CR == KEY_RETURN == SDLK_RETURN

		// Kill if they hit return or keypad enter or it maxes out console or it's more than one line long
		if ((ch == INPBUF_CR) || (ch == KEY_KPENTER) || (strlen(sTextToSend)>=MAX_CONSOLE_STRING_LENGTH-16) // Prefixes with ERROR: and terminates with '?'
		 || iV_GetTextWidth(sTextToSend) > (pie_GetVideoBufferWidth()-64))// sendit
		{
			bAllowOtherKeyPresses = true;
			//	flushConsoleMessages();

			sstrcpy(sCurrentConsoleText, "");		//reset beacon msg, since console is empty now

			// don't send empty lines to other players
			if(!strcmp(sTextToSend, ""))
				return;

			//console callback message
			//--------------------------
			ConsolePlayer = selectedPlayer;
			sstrcpy(ConsoleMsg, sTextToSend);
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_CONSOLE);

			sendTextMessage(sTextToSend,false);
			attemptCheatCode(sTextToSend);
			return;
		}
		else if(ch == INPBUF_BKSPACE )							// delete
		{
			if(sTextToSend[0] != '\0')							// cant delete nothing!
			{
				size_t newlen = strlen(sTextToSend) - 1;
				while(newlen > 0 && (sTextToSend[newlen]&0xC0) == 0x80)
				    --newlen;  // Don't delete half a unicode character.
				sTextToSend[newlen]= '\0';
				sstrcpy(sCurrentConsoleText, sTextToSend);		//beacons
			}
		}
		else if(ch == INPBUF_ESC)								//abort.
		{
			bAllowOtherKeyPresses = true;
			sstrcpy(sCurrentConsoleText, "");
			//	flushConsoleMessages();
			return;
		}
		else							 						// display
		{
			const utf_32_char input_char[2] = { unicode, '\0' };
			char *utf = UTF32toUTF8(input_char, NULL);
			sstrcat(sTextToSend, utf);
			free(utf);
			sstrcpy(sCurrentConsoleText, sTextToSend);
		}

		ch = inputGetKey(&unicode);
	}

	// macro store stuff
	if (keyPressed(KEY_F1))
	{
		if(keyDown(KEY_LCTRL))
		{
			sstrcpy(ingame.phrases[0], sTextToSend);
		}
		else
		{
			sstrcpy(sTextToSend, ingame.phrases[0]);
			bAllowOtherKeyPresses = true;
			//	flushConsoleMessages();
			sendTextMessage(sTextToSend,false);
			return;
		}
	}
	if (keyPressed(KEY_F2))
	{
		if(keyDown(KEY_LCTRL))
		{
			sstrcpy(ingame.phrases[1], sTextToSend);
		}
		else
		{
			sstrcpy(sTextToSend, ingame.phrases[1]);
			bAllowOtherKeyPresses = true;
			//	flushConsoleMessages();
			sendTextMessage(sTextToSend,false);
			return;
		}
	}
	if (keyPressed(KEY_F3))
	{
		if(keyDown(KEY_LCTRL))
		{
			sstrcpy(ingame.phrases[2], sTextToSend);
		}
		else
		{
			sstrcpy(sTextToSend, ingame.phrases[2]);
			bAllowOtherKeyPresses = true;
			//	flushConsoleMessages();
			sendTextMessage(sTextToSend,false);
			return;
		}
	}
	if (keyPressed(KEY_F4))
	{
		if(keyDown(KEY_LCTRL))
		{
			sstrcpy(ingame.phrases[3], sTextToSend);
		}
		else
		{
			sstrcpy(sTextToSend, ingame.phrases[3]);
			bAllowOtherKeyPresses = true;
			//	flushConsoleMessages();
			sendTextMessage(sTextToSend,false);
			return;
		}
	}
	if (keyPressed(KEY_F5))
	{
		if(keyDown(KEY_LCTRL))
		{
			sstrcpy(ingame.phrases[4], sTextToSend);
		}
		else
		{
			sstrcpy(sTextToSend, ingame.phrases[4]);
			bAllowOtherKeyPresses = true;
			 //	flushConsoleMessages();
			sendTextMessage(sTextToSend,false);
			return;
		}
	}

//	flushConsoleMessages();								//clear
//	addConsoleMessage(sTextToSend,DEFAULT_JUSTIFY, SYSTEM_MESSAGE);		//display
//	iV_DrawText(sTextToSend,16+D_W,RADTLY+D_H-16);
}
// --------------------------------------------------------------------------
void	kf_ToggleConsole( void )
{
	if(getConsoleDisplayStatus())
	{
		enableConsoleDisplay(false);
	}
	else
	{
		enableConsoleDisplay(true);
	}
}

// --------------------------------------------------------------------------
void	kf_SelectAllOnScreenUnits( void )
{

	selDroidSelection(selectedPlayer, DS_ALL_UNITS, DST_UNUSED, true);
}

// --------------------------------------------------------------------------
void	kf_SelectAllUnits( void )
{

	selDroidSelection(selectedPlayer, DS_ALL_UNITS, DST_UNUSED, false);
}

// --------------------------------------------------------------------------
void	kf_SelectAllVTOLs( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_VTOL,false);
}

void kf_SelectAllArmedVTOLs()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_VTOL_ARMED, false);
}

// --------------------------------------------------------------------------
void	kf_SelectAllHovers( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_HOVER,false);
}

// --------------------------------------------------------------------------
void	kf_SelectAllWheeled( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_WHEELED,false);
}

// --------------------------------------------------------------------------
void	kf_SelectAllTracked( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_TRACKED,false);
}

// --------------------------------------------------------------------------
void	kf_SelectAllHalfTracked( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_HALF_TRACKED,false);
}

// --------------------------------------------------------------------------
void kf_SelectAllCyborgs()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_CYBORG, false);
}

// --------------------------------------------------------------------------
void kf_SelectAllEngineers()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_ENGINEER, false);
}

// --------------------------------------------------------------------------
void kf_SelectAllMechanics()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_MECHANIC, false);
}

// --------------------------------------------------------------------------
void kf_SelectAllTransporters()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_TRANSPORTER, false);
}

// --------------------------------------------------------------------------
void kf_SelectAllRepairTanks()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_REPAIR_TANK, false);
}

// --------------------------------------------------------------------------
void kf_SelectAllSensorUnits()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_SENSOR, false);
}

// --------------------------------------------------------------------------
void kf_SelectAllTrucks()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_TRUCK, false);
}

// --------------------------------------------------------------------------
void	kf_SelectAllDamaged( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_DAMAGED,false);
}

// --------------------------------------------------------------------------
void	kf_SelectAllCombatUnits( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_COMBAT,false);
}

// --------------------------------------------------------------------------
void kf_SelectAllLandCombatUnits()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_ALL_COMBAT_LAND, false);
}

// --------------------------------------------------------------------------
void kf_SelectAllCombatCyborgs()
{
	selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_ALL_COMBAT_CYBORG, false);
}

// --------------------------------------------------------------------------
// this is worst case (size of apsDroidLists[selectedPlayer] squared).
// --------------------------------------------------------------------------
void	kf_SelectAllSameType( void )
{
	selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_SAME,false);
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
void	kf_SetDroidOrderHold()
{
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			orderDroid(psDroid, DORDER_HOLD, ModeQueue);
		}
	}
}

// --------------------------------------------------------------------------
void	kf_SetDroidOrderStop()
{
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			orderDroid(psDroid, DORDER_HOLD, ModeQueue);
		}
	}
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
void	kf_SetDroidGoToTransport( void )
{
	kfsf_SetSelectedDroidsState(DSO_RETURN_TO_LOC,DSS_RTL_TRANSPORT);
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
		console("Reveal OFF");
		setRevealStatus(false);
	}
	else
	{
		console("Reveal ON");
		setRevealStatus(true);
	}
}

// --------------------------------------------------------------------------
static void kfsf_SetSelectedDroidsState( SECONDARY_ORDER sec, SECONDARY_STATE state )
{
	DROID	*psDroid;

	// NOT A CHEAT CODE
	// This is a function to set unit orders via keyboard shortcuts. It should
	// _not_ be disallowed in multiplayer games.

	// This code is similar to SetSecondaryState() in intorder.cpp. Unfortunately, it seems hard to un-duplicate the code.
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		// Only set the state if it's not a transporter.
		if (psDroid->selected && psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)
		{
			secondarySetState(psDroid,sec,state);
		}
	}
}

// --------------------------------------------------------------------------
void	kf_TriggerRayCast( void )
{
DROID	*psDroid;
bool	found;

	found = false;
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid && !found;
		psDroid = psDroid->psNext)
		{
			if(psDroid->selected)
			{
				found = true;
			}
			/* NOP */
		}

	if(found)
	{
//		getBlockHeightDirToEdgeOfGrid(UDWORD x, UDWORD y, UBYTE direction, UDWORD *height, UDWORD *dist)
//		getBlockHeightDirToEdgeOfGrid(psOther->pos.x,psOther->pos.y,psOther->direction,&height,&dist);
//		getBlockHeightDirToEdgeOfGrid(mouseTileX*TILE_UNITS,mouseTileY*TILE_UNITS,getTestAngle(),&height,&dist);
	}
}

// --------------------------------------------------------------------------
void	kf_ScatterDroids( void )
{
	// to be written!
	addConsoleMessage("Scatter droids - not written yet!",LEFT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------
void	kf_CentreOnBase( void )
{
STRUCTURE	*psStruct;
bool		bGotHQ;
UDWORD		xJump = 0, yJump = 0;

	/* Got through our buildings */
	for(psStruct = apsStructLists[selectedPlayer],bGotHQ = false;	// start
	psStruct && !bGotHQ;											// terminate
	psStruct = psStruct->psNext)									// iteration
	{
		/* Have we got a HQ? */
		if(psStruct->pStructureType->type == REF_HQ)
		{
			bGotHQ = true;
			xJump = psStruct->pos.x;
			yJump = psStruct->pos.y;
		}
	}

	/* If we found it, then jump to it! */
	if(bGotHQ)
	{
		addConsoleMessage(_("Centered on player HQ, direction NORTH"),LEFT_JUSTIFY, SYSTEM_MESSAGE);
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
		addConsoleMessage(_("Unable to locate HQ!"),LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

// --------------------------------------------------------------------------
void kf_ToggleFormationSpeedLimiting( void )
{
	addConsoleMessage(_("Formation speed limiting has been removed from the game due to bugs."), LEFT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------
void	kf_RightOrderMenu( void )
{
DROID	*psDroid,*psGotOne = NULL;
bool	bFound;

	// if menu open, then close it!
	if (widgGetFromID(psWScreen,IDORDER_FORM) != NULL)
	{
		intRemoveOrder();	// close the screen.
		return;
	}

	for(psDroid = apsDroidLists[selectedPlayer],bFound = false;
		psDroid && !bFound; psDroid = psDroid->psNext)
	{
		if(psDroid->selected)// && droidOnScreen(psDroid,0))
		{
			bFound = true;
			psGotOne = psDroid;
		}
	}
	if(bFound)
	{
		intResetScreen(true);
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

	addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SHOCKWAVE,false,NULL,0);
}
// --------------------------------------------------------------------------
void	kf_ToggleMouseInvert( void )
{
	if(getInvertMouseStatus())
	{
		setInvertMouseStatus(false);
		CONPRINTF(ConsoleString,(ConsoleString,_("Vertical rotation direction: Normal")));
	}
	else
	{
		setInvertMouseStatus(true);
		CONPRINTF(ConsoleString,(ConsoleString,_("Vertical rotation direction: Flipped")));
	}
}
// --------------------------------------------------------------------------
void	kf_ToggleShakeStatus( void )
{
	if(getShakeStatus())
	{
		setShakeStatus(false);
		CONPRINTF(ConsoleString,(ConsoleString,_("Screen shake when things die: Off")));
	}
	else
	{
		setShakeStatus(true);
		CONPRINTF(ConsoleString,(ConsoleString,_("Screen shake when things die: On")));
	}
}
// --------------------------------------------------------------------------
void	kf_ToggleShadows( void )
{
	if(getDrawShadows())
	{
		setDrawShadows(false);
	}
	else
	{
		setDrawShadows(true);
	}
}
// --------------------------------------------------------------------------

static const Rational available_speed[] = {
// p = pumpkin allowed, n = new entries allowed in debug mode only.
// Since some of these values can ruin a SP game, we disallow them in normal mode.
	Rational(0),     // n
	Rational(1, 1000),// n
	Rational(1, 100),// n
	Rational(1, 40), // n
	Rational(1, 8),  // n
	Rational(1, 5),  // n
	Rational(1, 3),  // p
	Rational(1, 2),
	Rational(3, 4),  // p
	Rational(1),     // p
	Rational(5, 4),  // p
	Rational(3, 2),  // p
	Rational(2),     // p (in debug mode only)
	Rational(5, 2),  // n
	Rational(3),     // n
	Rational(10),    // n
	Rational(20),    // n
	Rational(30),    // n
	Rational(60),    // n
	Rational(100),   // n
};
const unsigned nb_available_speeds = ARRAY_SIZE(available_speed);

static void tryChangeSpeed(Rational newMod, Rational oldMod)
{
	// Bail out if we're running a _true_ multiplayer game or are playing a tutorial
	if ((runningMultiplayer() && !getDebugMappingStatus()) || bInTutorial)
	{
		if (!bInTutorial)
		{
			addConsoleMessage(_("Sorry, but game speed cannot be changed in multiplayer."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
		return;
	}

	// only in debug/cheat mode do we enable all time compression speeds.
	if (!getDebugMappingStatus() && (newMod >= 2 || newMod <= 0))  // 2 = max officially allowed time compression
	{
		return;
	}

	char modString[30];
	if (newMod.d != 1)
	{
		const float speed = float(newMod.n) / newMod.d;
		if (speed > 0.4)
		{
			ssprintf(modString, "%.1f", speed);
		}
		else if (speed > 0.15)
		{
			ssprintf(modString, "%.2f", speed);
		}
		else
		{
			ssprintf(modString, "%.3f", speed);
		}
	}
	else
	{
		ssprintf(modString, "%d", newMod.n);
	}

	if (newMod == 1)
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("Game Speed Reset")));
	}
	else if (newMod > oldMod)
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("Game Speed Increased to %s"), modString));
	}
	else
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("Game Speed Reduced to %s"), modString));
	}
	gameTimeSetMod(newMod);
}

void kf_SpeedUp( void )
{
	// get the current modifier
	Rational mod = gameTimeGetMod();

	Rational const *newMod = std::upper_bound(available_speed, available_speed + nb_available_speeds, mod);
	if (newMod == available_speed + nb_available_speeds)
	{
		return;  // Already at maximum speed.
	}

	tryChangeSpeed(*newMod, mod);
}

void kf_SlowDown( void )
{
	// get the current modifier
	Rational mod = gameTimeGetMod();

	Rational const *newMod = std::lower_bound(available_speed, available_speed + nb_available_speeds, mod);
	if (newMod == available_speed)
	{
		return;  // Already at minimum speed.
	}
	--newMod;  // Point to lower speed instead of current speed.

	tryChangeSpeed(*newMod, mod);
}

void kf_NormalSpeed( void )
{
	// Bail out if we're running a _true_ multiplayer game or are playing a tutorial
	if (runningMultiplayer() || bInTutorial)
	{
		if (!bInTutorial)
			noMPCheatMsg();
		return;
	}

	CONPRINTF(ConsoleString,(ConsoleString,_("Game Speed Reset")));
	gameTimeResetMod();
}

// --------------------------------------------------------------------------

void kf_ToggleRadarAllyEnemy(void)
{
	bEnemyAllyRadarColor = !bEnemyAllyRadarColor;

	if(bEnemyAllyRadarColor)
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("Radar showing friend-foe colors")));
	}
	else
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("Radar showing player colors")));
	}
	resizeRadar();
}

void kf_ToggleRadarTerrain(void)
{
	radarDrawMode = (RADAR_DRAW_MODE)(radarDrawMode + 1);
	if (radarDrawMode >= NUM_RADAR_MODES)
	{
		radarDrawMode = (RADAR_DRAW_MODE)0;
	}
	switch (radarDrawMode)
	{
		case RADAR_MODE_NO_TERRAIN:
			CONPRINTF(ConsoleString, (ConsoleString, _("Radar showing only objects")));
			break;
		case RADAR_MODE_COMBINED:
			CONPRINTF(ConsoleString, (ConsoleString, _("Radar blending terrain and height")));
			break;
		case RADAR_MODE_TERRAIN:
			CONPRINTF(ConsoleString, (ConsoleString, _("Radar showing terrain")));
			break;
		case RADAR_MODE_HEIGHT_MAP:
			CONPRINTF(ConsoleString, (ConsoleString, _("Radar showing height")));
			break;
		case NUM_RADAR_MODES:
			assert(false);
			break;
	}
}

//Add a beacon (blip)
void	kf_AddHelpBlip( void )
{
	int		worldX, worldY;
	UDWORD	i;
	char	tempStr[255];
	SDWORD	x,y;
	bool	mOverR=false;

	/* not needed in campaign */
	if(!bMultiPlayer)
		return;

	debug(LOG_WZ,"Adding beacon='%s'",sCurrentConsoleText);

	/* check if clicked on radar */
	x = mouseX();
	y = mouseY();
	if (radarOnScreen && radarPermitted)
	{
		if(CoordInRadar(x,y))
		{
			mOverR = true;
			CalcRadarPosition(x,y,&worldX,&worldY);

			CLIP(worldX, 0, mapWidth - 1);	// temporary hack until CalcRadarPosition is fixed
			CLIP(worldY, 0, mapHeight- 1);
			worldX = worldX*TILE_UNITS+TILE_UNITS/2;
			worldY = worldY*TILE_UNITS+TILE_UNITS/2;
		}
	}

	/* convert screen to world */
	if(!mOverR)
	{
		worldX = mouseTileX*TILE_UNITS+TILE_UNITS/2;
		worldY = mouseTileY*TILE_UNITS+TILE_UNITS/2;
	}

	sstrcpy(tempStr, getPlayerName(selectedPlayer));		//temporary solution

	/* add beacon for the sender */
	sstrcpy(beaconMsg[selectedPlayer], tempStr);
	sendBeaconToPlayer(worldX, worldY, selectedPlayer, selectedPlayer, beaconMsg[selectedPlayer]);

	/* send beacon to other players */
	for(i=0;i<game.maxPlayers;i++)
	{
		if(openchannels[i] && (i != selectedPlayer))
		{
			sstrcpy(beaconMsg[i], tempStr);
			sendBeaconToPlayer(worldX, worldY, i, selectedPlayer, beaconMsg[i]);
		}
	}
}

void kf_NoAssert()
{
	debugDisableAssert();
	console("Asserts turned off");
	debug(LOG_ERROR, "Asserts turned off");
}

// rotuine to decrement the tab-scroll 'buttons'
void kf_BuildPrevPage()
{
	ASSERT_OR_RETURN( , psWScreen != NULL, " Invalid screen pointer!");
	ListTabWidget *psTForm = (ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM);
	if (psTForm == NULL)
	{
		return;
	}

	if (!psTForm->setCurrentPage(psTForm->currentPage() - 1))
	{
		audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		return;
	}

	audio_PlayTrack(ID_SOUND_BUTTON_CLICK_5);
}

// rotuine to advance the tab-scroll 'buttons'
void kf_BuildNextPage()
{
	ASSERT_OR_RETURN( , psWScreen != NULL, " Invalid screen pointer!");
	ListTabWidget *psTForm = (ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM);
	if (psTForm == NULL)
	{
		return;
	}

	if (!psTForm->setCurrentPage(psTForm->currentPage() + 1))
	{
		// went over max
		audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		return;
	}

	audio_PlayTrack(ID_SOUND_BUTTON_CLICK_5);
}
