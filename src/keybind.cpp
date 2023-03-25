/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include <cstring>

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/rational.h"
#include "lib/framework/physfs_ext.h"
#include "objects.h"
#include "levels.h"
#include "basedef.h"
#include "map.h"
#include "warcam.h"
#include "warzoneconfig.h"
#include "console.h"
#include "display.h"
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
#include "ingameop.h"
#include "effects.h"
#include "component.h"
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

#include "intorder.h"
#include "lib/widget/listwidget.h"
#include "order.h"
#include "lib/ivis_opengl/piestate.h"
// FIXME Direct iVis implementation include!
#include "lib/framework/fixedpoint.h"

#include "loop.h"
#include "mission.h"
#include "selection.h"
#include "difficulty.h"
#include "research.h"
#include "template.h"
#include "qtscript.h"
#include "multigifts.h"
#include "loadsave.h"
#include "game.h"
#include "droid.h"
#include "spectatorwidgets.h"

#include "activity.h"

/*
	KeyBind.c
	Holds all the functions that can be mapped to a key.
	All functions at the moment must be 'void func()'.
	Alex McLean, Pumpkin Studios, EIDOS Interactive.
*/

extern char	ScreenDumpPath[];

bool	bMovePause = false;
bool		bAllowOtherKeyPresses = true;
char	beaconMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];		//beacon msg for each player

static STRUCTURE	*psOldRE = nullptr;
static char	sCurrentConsoleText[MAX_CONSOLE_STRING_LENGTH];			//remember what user types in console for beacon msg

#define QUICKSAVE_CAM_FOLDER "savegames/campaign/QuickSave"
#define QUICKSAVE_SKI_FOLDER "savegames/skirmish/QuickSave"
#define QUICKSAVE_CAM_FILENAME "savegames/campaign/QuickSave.gam"
#define QUICKSAVE_SKI_FILENAME "savegames/skirmish/QuickSave.gam"
#define QUICKSAVE_CAM_JSON_FILENAME QUICKSAVE_CAM_FOLDER "/gam.json"
#define QUICKSAVE_SKI_JSON_FILENAME QUICKSAVE_SKI_FOLDER "/gam.json"

#define SPECTATOR_NO_OP() do { if (selectedPlayer >= MAX_PLAYERS || NetPlay.players[selectedPlayer].isSpectator) { return; } } while (0)

/* Support functions to minimise code size */
static void kfsf_SetSelectedDroidsState(SECONDARY_ORDER sec, SECONDARY_STATE State);

/** A function to determine whether we're running a multiplayer game, not just a
 *  single player campaign or a skirmish game.
 *  \return false if this is a skirmish or single player game, true if it is a
 *          multiplayer game.
 */
bool runningMultiplayer()
{
	return bMultiPlayer && NetPlay.bComms;
}

static void noMPCheatMsg()
{
	addConsoleMessage(_("Sorry, that cheat is disabled in multiplayer games."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------
void kf_AutoGame()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif
	if (game.type == LEVEL_TYPE::CAMPAIGN)
	{
		CONPRINTF("%s", "Not possible with the campaign!");
		return;
	}
	// Notify all human players that we are trying to enable autogame
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (NetPlay.players[i].allocated)
		{
			sendGift(AUTOGAME_GIFT, i);
		}
	}

	CONPRINTF("%s", "autogame request has been sent to all players. AI script *must* support this command!");
}

void	kf_ToggleMissionTimer()
{
	addConsoleMessage(_("Warning! This cheat is buggy.  We recommend to NOT use it."), DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);
	setMissionCheatTime(!mission.cheatTime);
}

void	kf_ToggleShowGateways()
{
	addConsoleMessage(_("Gateways toggled."), DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);
	showGateways = !showGateways;
}

void	kf_ToggleShowPath()
{
	addConsoleMessage(_("Path display toggled."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	showPath = !showPath;
}

void kf_PerformanceSample()
{
	wzPerfStart();
}

// --------------------------------------------------------------------------
void	kf_ToggleRadarJump()
{
	war_SetRadarJump(!war_GetRadarJump());
}

// --------------------------------------------------------------------------

void kf_ForceDesync()
{
	syncDebug("Oh no!!! I went out of sync!!!");
}

void	kf_PowerInfo()
{
	int i;

	for (i = 0; i < game.maxPlayers; i++)
	{
		console(_("Player %d: %d power"), i, (int)getPower(i));
	}
}

void kf_DamageMe()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			int val = psDroid->body - ((psDroid->originalBody / 100) * 20);
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
	for (STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->selected)
		{
			int val = psStruct->body - ((structureBody(psStruct) / 100) * 20);
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

void	kf_TraceObject()
{
	DROID		*psCDroid, *psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}

	for (psCDroid = apsDroidLists[selectedPlayer]; psCDroid; psCDroid = psNDroid)
	{
		psNDroid = psCDroid->psNext;
		if (psCDroid->selected)
		{
			objTraceEnable(psCDroid->id);
			CONPRINTF("Tracing droid %d", (int)psCDroid->id);
			return;
		}
	}
	for (psCStruct = apsStructLists[selectedPlayer]; psCStruct; psCStruct = psNStruct)
	{
		psNStruct = psCStruct->psNext;
		if (psCStruct->selected)
		{
			objTraceEnable(psCStruct->id);
			CONPRINTF("Tracing structure %d", (int)psCStruct->id);
			return;
		}
	}
	objTraceDisable();
	CONPRINTF("%s", "No longer tracing anything.");
}

//===================================================
void kf_ToggleSensorDisplay()
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
	{
		addConsoleMessage(_("Lets us see what you see!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);    //added this message... Yeah, its lame. :)
	}
	else
	{
		addConsoleMessage(_("Fine, weapon & sensor display is off!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);    //added this message... Yeah, its lame. :)
	}
}
//===================================================
/* Halves all the heights of the map tiles */
void	kf_HalveHeights()
{
	MAPTILE	*psTile;

	for (int i = 0; i < mapWidth; ++i)
	{
		for (int j = 0; j < mapHeight; ++j)
		{
			psTile = mapTile(i, j);
			psTile->height /= 2;
		}
	}
}

// --------------------------------------------------------------------------
void	kf_FaceNorth()
{
	kf_SeekNorth();
}
// --------------------------------------------------------------------------
void	kf_FaceSouth()
{
	playerPos.r.y = DEG(180);
	if (getWarCamStatus())
	{
		camToggleStatus();
	}
	CONPRINTF("%s", _("View Aligned to South"));
}
// --------------------------------------------------------------------------
void	kf_FaceEast()
{
	playerPos.r.y = DEG(90);
	if (getWarCamStatus())
	{
		camToggleStatus();
	}
	CONPRINTF("%s", _("View Aligned to East"));
}
// --------------------------------------------------------------------------
void	kf_FaceWest()
{
	playerPos.r.y = DEG(270);
	if (getWarCamStatus())
	{
		camToggleStatus();
	}
	CONPRINTF("%s", _("View Aligned to West"));
}
// --------------------------------------------------------------------------

/* Writes out debug info about all the selected droids */
void	kf_DebugDroidInfo()
{
	DROID	*psDroid;

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			printDroidInfo(psDroid);
		}
	}
}

void kf_CloneSelected(int limit)
{
	DROID_TEMPLATE	*sTemplate = nullptr;
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}

	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			enumerateTemplates(selectedPlayer, [psDroid, &sTemplate](DROID_TEMPLATE * psTempl) {
				if (psTempl->name.compare(psDroid->aName) == 0)
				{
					sTemplate = psTempl;
					return false; // stop enumerating
				}
				return true;
			});

			if (!sTemplate)
			{
				debug(LOG_ERROR, "Cloning vat has been destroyed. We can't find the template for this droid: %s, id:%u, type:%d!", psDroid->aName, psDroid->id, psDroid->droidType);
				return;
			}

			// create a new droid army
			for (int i = 0; i < limit; i++)
			{
				Vector2i pos = psDroid->pos.xy() + iSinCosR(40503 * i, iSqrt(50 * 50 * i));  // 40503 = 65536/φ
				DROID *psNewDroid = buildDroid(sTemplate, pos.x, pos.y, psDroid->player, false, nullptr);
				if (psNewDroid)
				{
					addDroid(psNewDroid, apsDroidLists);
					triggerEventDroidBuilt(psNewDroid, nullptr);
				}
				else if (!bMultiMessages)
				{
					debug(LOG_ERROR, "Cloning has failed for template:%s id:%d", getID(sTemplate), sTemplate->multiPlayerID);
				}
			}
			std::string msg = astringf(_("Player %u is cheating a new droid army of: %d × %s."), selectedPlayer, limit, psDroid->aName);
			sendInGameSystemMessage(msg.c_str());
			Cheated = true;
			audio_PlayTrack(ID_SOUND_NEXUS_LAUGH1);
			return;
		}
		debug(LOG_INFO, "Nothing was selected?");
	}
}

void kf_MakeMeHero()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}

	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected && psDroid->droidType == DROID_COMMAND)
		{
			psDroid->experience = 8 * 65536 * 128;
		} 
		else if (psDroid->selected)
		{
			psDroid->experience = 4 * 65536 * 128;
		}
	}
}

void kf_TeachSelected()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}

	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			psDroid->experience += 4 * 65536;
		}
	}
}

void kf_Unselectable()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}

	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			psDroid->flags.set(OBJECT_FLAG_UNSELECTABLE, true);
			psDroid->selected = false;
		}
	}
	for (STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->selected)
		{
			psStruct->flags.set(OBJECT_FLAG_UNSELECTABLE, true);
			psStruct->selected = false;
		}
	}
}

// --------------------------------------------------------------------------
//
///* Prints out the date and time of the build of the game */
void	kf_BuildInfo()
{
	CONPRINTF("Built: %s %s", getCompileDate(), __TIME__);
}

// --------------------------------------------------------------------------
void	kf_ToggleConsoleDrop()
{
	if (!bInTutorial)
	{
		setHistoryMode(false);
		toggleConsoleDrop();
	}
}

void kf_ToggleTeamChat()
{
	if (!bInTutorial)
	{
		setHistoryMode(true);
		toggleConsoleDrop();
	}
}
// --------------------------------------------------------------------------
void	kf_BifferBaker()
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	// player deals far more damage, and the enemy far less
	setDamageModifiers(999, 1);
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("Hard as nails!!!"));
	sendInGameSystemMessage(cmsg.c_str());
}
// --------------------------------------------------------------------------
void	kf_SetEasyLevel()
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
void	kf_UpThePower()
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
	addPower(selectedPlayer, 1000);
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("1000 big ones!!!"));
	sendInGameSystemMessage(cmsg.c_str());
}

// --------------------------------------------------------------------------
void	kf_MaxPower()
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
	setPower(selectedPlayer, 100000);
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("Power overwhelming"));
	sendInGameSystemMessage(cmsg.c_str());
}

// --------------------------------------------------------------------------
void	kf_SetNormalLevel()
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
void	kf_SetHardLevel()
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
void	kf_DoubleUp()
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
	setDamageModifiers(100, 50); // enemy damage halved
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("Twice as nice!"));
	sendInGameSystemMessage(cmsg.c_str());
}
// --------------------------------------------------------------------------
void kf_ToggleFPS() //This shows *just FPS* and is always visible (when active) -Q.
{
	// Toggle the boolean value of showFPS
	showFPS = !showFPS;

	if (showFPS)
	{
		CONPRINTF("%s", _("FPS display is enabled."));
	}
	else
	{
		CONPRINTF("%s", _("FPS display is disabled."));
	}
}
void kf_ToggleUnitCount()		// Display units built / lost / produced counter
{
	// Toggle the boolean value of showUNITCOUNT
	showUNITCOUNT = !showUNITCOUNT;

	if (showUNITCOUNT)
	{
		CONPRINTF("%s", _("Unit count display is enabled."));
	}
	else
	{
		CONPRINTF("%s", _("Unit count display is disabled."));
	}
}
void kf_ToggleSamples() //Displays number of sound sample in the sound queues & lists.
{
	// Toggle the boolean value of showSAMPLES
	showSAMPLES = !showSAMPLES;

	CONPRINTF("Sound Samples displayed is %s", showSAMPLES ? "Enabled" : "Disabled");
}

void kf_ToggleOrders()	// Displays orders & action of currently selected unit.
{
	// Toggle the boolean value of showORDERS
	showORDERS = !showORDERS;
	CONPRINTF("Unit Order/Action displayed is %s", showORDERS ? "Enabled" : "Disabled");
}

/* Writes out the frame rate */
void	kf_FrameRate()
{
	CONPRINTF("FPS %d; PIEs %zu; polys %zu",
	                          frameRate(), loopPieCount, loopPolyCount);
	if (runningMultiplayer())
	{
		CONPRINTF("NETWORK:  Bytes: s-%zu r-%zu  Uncompressed Bytes: s-%zu r-%zu  Packets: s-%zu r-%zu",
		                          NETgetStatistic(NetStatisticRawBytes, true),
		                          NETgetStatistic(NetStatisticRawBytes, false),
		                          NETgetStatistic(NetStatisticUncompressedBytes, true),
		                          NETgetStatistic(NetStatisticUncompressedBytes, false),
		                          NETgetStatistic(NetStatisticPackets, true),
		                          NETgetStatistic(NetStatisticPackets, false));
	}
	gameStats = !gameStats;
	CONPRINTF("Built: %s %s", getCompileDate(), __TIME__);
}

// --------------------------------------------------------------------------

// display the total number of objects in the world
void kf_ShowNumObjects()
{
	int droids, structures, features;

	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}

	objCount(&droids, &structures, &features);
	std::string cmsg = astringf(_("(Player %u) is using a cheat :Num Droids: %d  Num Structures: %d  Num Features: %d"),
	          selectedPlayer, droids, structures, features);
	sendInGameSystemMessage(cmsg.c_str());
}


void kf_ListDroids()
{
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	debug(LOG_INFO, "list droids:");
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		for (DROID *psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			const auto x = map_coord(psDroid->pos.x);
			const auto y = map_coord(psDroid->pos.y);
			debug(LOG_INFO, "droid %i;%s;%i;%i;%i", i, psDroid->aName, psDroid->droidType, x, y);
		}
	}
	
}


// --------------------------------------------------------------------------

/* Toggles radar on off */
void	kf_ToggleRadar()
{
	radarOnScreen = !radarOnScreen;
}

// --------------------------------------------------------------------------

/* Toggles infinite power on/off */
void	kf_TogglePower()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	powerCalculated = !powerCalculated;
	if (powerCalculated)
	{
		powerCalc(true);
	}

	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, powerCalculated ? _("Infinite power disabled") : _("Infinite power enabled"));
	sendInGameSystemMessage(cmsg.c_str());
}

// --------------------------------------------------------------------------

/* Recalculates the lighting values for a tile */
void	kf_RecalcLighting()
{
	initLighting(0, 0, mapWidth, mapHeight);
	addConsoleMessage(_("Lighting values for all tiles recalculated"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------

/* Sends the screen buffer to disk */
void	kf_ScreenDump()
{
	screenDumpToDisk(ScreenDumpPath, getLevelName());
}

// --------------------------------------------------------------------------

/* Make all functions available */
void	kf_AllAvailable()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	makeAllAvailable();
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("All items made available"));
	sendInGameSystemMessage(cmsg.c_str());
}

// --------------------------------------------------------------------------

/* Flips the cut of a tile */
void	kf_TriFlip()
{
	MAPTILE	*psTile;
	psTile = mapTile(mouseTileX, mouseTileY);
	TOGGLE_TRIFLIP(psTile);
//	addConsoleMessage(_("Triangle flip status toggled"),DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------

/* Debug info about a map tile */
void	kf_TileInfo()
{
	MAPTILE	*psTile = mapTile(mouseTileX, mouseTileY);

	debug(LOG_ERROR, "Tile position=(%d, %d) Terrain=%d Texture=%u Height=%d Illumination=%u",
	      mouseTileX, mouseTileY, (int)terrainType(psTile), TileNumber_tile(psTile->texture), psTile->height,
	      psTile->illumination);
	addConsoleMessage(_("Tile info dumped into log"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}

/* Toggles fog on/off */
void	kf_ToggleFog()
{
	if (pie_GetFogEnabled())
	{
		pie_SetFogStatus(false);
		pie_EnableFog(false);
	}
	else
	{
		pie_EnableFog(true);
	}
	std::string cmsg = pie_GetFogEnabled() ? _("Fog on") : _("Fog off");
	sendInGameSystemMessage(cmsg.c_str());
}

// --------------------------------------------------------------------------

/* Toggle camera on/off */
void	kf_ToggleCamera()
{
	if (!getWarCamStatus())
	{
		shakeStop(); // Ensure screen shake stopped before starting camera mode.
	}

	camToggleStatus();
}

void kf_RevealMapAtPos()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	if (selectedPlayer >= MAX_PLAYERS) { return; }

	addSpotter(mouseTileX, mouseTileY, selectedPlayer, 1024, false, gameTime + 2000);
}

// --------------------------------------------------------------------------

void kf_MapCheck()
{
#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent desyncs)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	DROID		*psDroid;
	STRUCTURE	*psStruct;
	FLAG_POSITION	*psCurrFlag;

	if (selectedPlayer >= MAX_PLAYERS) { return; }

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
void	kf_RaiseTile()
{
	if (runningMultiplayer())
	{
		return;  // Don't desynch if pressing 'W'...
	}

	raiseTile(mouseTileX, mouseTileY);
}

// --------------------------------------------------------------------------

/* Lowers the tile under the mouse */
void	kf_LowerTile()
{
	if (runningMultiplayer())
	{
		return;  // Don't desynch if pressing 'A'...
	}

	lowerTile(mouseTileX, mouseTileY);
}

// --------------------------------------------------------------------------
/* Zooms in/out from display */
MappableFunction kf_Zoom(const int multiplier)
{
	return [multiplier]() {
		incrementViewDistance(war_GetMapZoomRate() * multiplier);
	};
}

/* Zooms in/out from radar */
MappableFunction kf_RadarZoom(const int multiplier)
{
	return [multiplier]() {
		const uint8_t oldZoomLevel = GetRadarZoom();
		uint8_t newZoomLevel = oldZoomLevel + (RADARZOOM_STEP * multiplier);
		newZoomLevel = (newZoomLevel > MAX_RADARZOOM ? MAX_RADARZOOM : newZoomLevel);
		newZoomLevel = (newZoomLevel < MIN_RADARZOOM ? MIN_RADARZOOM : newZoomLevel);

		if (newZoomLevel != oldZoomLevel)
		{
			CONPRINTF(_("Setting radar zoom to %u"), newZoomLevel);
			SetRadarZoom(newZoomLevel);
			audio_PlayTrack(ID_SOUND_BUTTON_CLICK_5);
		}
	};
}

// --------------------------------------------------------------------------
void kf_MaxScrollLimits()
{
	scrollMinX = scrollMinY = 0;
	scrollMaxX = mapWidth;
	scrollMaxY = mapHeight;
}

// --------------------------------------------------------------------------
/* Spins the world round left */
void	kf_RotateLeft()
{
	int rotAmount = static_cast<int>(realTimeAdjustedIncrement(MAP_SPIN_RATE));

	playerPos.r.y += rotAmount;
}

// --------------------------------------------------------------------------
/* Spins the world right */
void	kf_RotateRight()
{
	int rotAmount = static_cast<int>(realTimeAdjustedIncrement(MAP_SPIN_RATE));

	playerPos.r.y -= rotAmount;
	if (playerPos.r.y < 0)
	{
		playerPos.r.y += DEG(360);
	}
}

// --------------------------------------------------------------------------
/* Rotate editing building direction clockwise */
void kf_RotateBuildingCW()
{
	incrementBuildingDirection(DEG(-90));
}

// --------------------------------------------------------------------------
/* Rotate editing building direction anticlockwise */
void kf_RotateBuildingACW()
{
	incrementBuildingDirection(DEG(90));
}

// --------------------------------------------------------------------------
/* Pitches camera back */
void	kf_PitchBack()
{
	int pitchAmount = static_cast<int>(realTimeAdjustedIncrement(MAP_PITCH_RATE));

	playerPos.r.x += pitchAmount;

	if (playerPos.r.x > DEG(360 + MAX_PLAYER_X_ANGLE))
	{
		playerPos.r.x = DEG(360 + MAX_PLAYER_X_ANGLE);
	}
}

// --------------------------------------------------------------------------
/* Pitches camera forward */
void	kf_PitchForward()
{
	int pitchAmount = static_cast<int>(realTimeAdjustedIncrement(MAP_PITCH_RATE));

	playerPos.r.x -= pitchAmount;
	if (playerPos.r.x < DEG(360 + MIN_PLAYER_X_ANGLE))
	{
		playerPos.r.x = DEG(360 + MIN_PLAYER_X_ANGLE);
	}
}

// --------------------------------------------------------------------------
/* Resets pitch to default */
void	kf_ResetPitch()
{
	playerPos.r.x = DEG(360 - 20);
	setViewDistance(STARTDISTANCE);
}

// --------------------------------------------------------------------------
/* Quickly access the in-game keymap */
void kf_ShowMappings()
{
	if (!InGameOpUp && !isInGamePopupUp)
	{
		kf_addInGameOptions();
		intProcessInGameOptions(INTINGAMEOP_OPTIONS);
		intProcessInGameOptions(INTINGAMEOP_KEYMAP);
	}
}

// --------------------------------------------------------------------------

/* Selects the player's groups 1..9 */
void kf_SelectGrouping(UDWORD groupNumber)
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	bool	bAlreadySelected;
	DROID	*psDroid;
	bool	Selected;

	bAlreadySelected = false;
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		/* Wipe out the ones in the wrong group */
		if (psDroid->selected && psDroid->group != groupNumber)
		{
			psDroid->selected = false;
		}
		/* Get the right ones */
		if (psDroid->group == groupNumber)
		{
			if (psDroid->selected)
			{
				bAlreadySelected = true;
			}
		}
	}
	if (bAlreadySelected)
	{
		Selected = activateGroupAndMove(selectedPlayer, groupNumber);
	}
	else
	{
		Selected = activateGroup(selectedPlayer, groupNumber);
	}

	/* play group audio but only if they weren't already selected - AM */
	if (Selected && !bAlreadySelected)
	{
		audio_QueueTrack(ID_SOUND_GROUP_0 + groupNumber);
		audio_QueueTrack(ID_SOUND_REPORTING);
		audio_QueueTrack(ID_SOUND_RADIOCLICK_1 + (rand() % 6));
	}
	triggerEventSelected();
}

// --------------------------------------------------------------------------

MappableFunction kf_SelectGrouping_N(const unsigned int n)
{
	return [n]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		kf_SelectGrouping(n);
	};
}

MappableFunction kf_AssignGrouping_N(const unsigned int n)
{
	return [n]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		assignDroidsToGroup(selectedPlayer, n, true);
	};
}

MappableFunction kf_AddGrouping_N(const unsigned int n)
{
	return [n]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		assignDroidsToGroup(selectedPlayer, n, false);
	};
}

MappableFunction kf_RemoveFromGrouping()
{
	return []() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		removeDroidsFromGroup(selectedPlayer);
	};
}

MappableFunction kf_SelectCommander_N(const unsigned int n)
{
	return [n]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		selCommander(n);
	};
}

// --------------------------------------------------------------------------
void	kf_ToggleDroidInfo()
{
	camToggleInfo();
}

void	kf_addInGameOptions()
{
	setWidgetsStatus(true);
	if (!isInGamePopupUp)	// they can *only* quit when popup is up.
	{
		intResetScreen(false);
		intAddInGameOptions();
	}
}

// --------------------------------------------------------------------------
// Display multiplayer guff.
void	kf_addMultiMenu()
{
	if (bMultiPlayer)
	{
		intAddMultiMenu();
	}
}

// --------------------------------------------------------------------------

MappableFunction kf_JumpToMapMarker(const unsigned int x, const unsigned int z, const int yaw)
{
	return [x, z, yaw]() {
		if (!getRadarTrackingStatus())
		{
			playerPos.p.x = x;
			playerPos.p.z = z;
			playerPos.r.y = yaw;

			/* A fix to stop the camera continuing when marker code is called */
			if (getWarCamStatus())
			{
				camToggleStatus();
			}
		}
	};
}

// --------------------------------------------------------------------------
/* Toggles the power bar display on and off*/
void	kf_TogglePowerBar()
{
	togglePowerBar();
}
// --------------------------------------------------------------------------
/* Toggles whether we process debug key mappings */
void kf_ToggleDebugMappings()
{
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	sendProcessDebugMappings(!dbgInputManager.getPlayerWantsDebugMappings(selectedPlayer));
}

/* Toggles the local debug mapping context prioritization status */
void kf_PrioritizeDebugMappings()
{
	DebugInputManager& dbgInputManager = gInputManager.debugManager();
	const auto status = dbgInputManager.toggleDebugMappingPriority()
		? "TRUE"
		: "FALSE";
	CONPRINTF("%s%s", _("Toggling debug mapping priority: "), status);
}

void kf_ToggleLevelEditor()
{
	ContextManager& contexts = gInputManager.contexts();
	const bool bIsActive = contexts.isActive(InputContext::DEBUG_LEVEL_EDITOR);

	if (bIsActive)
	{
		CONPRINTF("%s", _("Disabling level editor"));
		contexts.set(InputContext::DEBUG_LEVEL_EDITOR, InputContext::State::INACTIVE);
	}
	else
	{
		CONPRINTF("%s", _("Enabling level editor"));
		contexts.set(InputContext::DEBUG_LEVEL_EDITOR, InputContext::State::ACTIVE);
	}
}
// --------------------------------------------------------------------------

void enableGodMode()
{
	if (godMode)
	{
		return;
	}

	// Bail out if we're running a _true_ multiplayer game and we aren't a spectator (to prevent MP cheating)
	if (runningMultiplayer() && !NetPlay.players[selectedPlayer].isSpectator)
	{
		noMPCheatMsg();
		return;
	}

	godMode = true; // view all structures and droids
	revealAll(selectedPlayer);
	setRevealStatus(true); // view the entire map
	radarPermitted = true; //add minimap without CC building

	preProcessVisibility();
}

void	kf_ToggleGodMode()
{
	static bool pastReveal = true;

	/* not supported if a spectator - since they automatically get full vis */
	SPECTATOR_NO_OP();

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	if (godMode)
	{
		ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "Cannot disable godMode for spectator-only slots");

		FEATURE	*psFeat = apsFeatureLists[0];

		godMode = false;
		setRevealStatus(pastReveal);
		// now hide the features
		while (psFeat)
		{
			psFeat->visible[selectedPlayer] = 0;
			psFeat = psFeat->psNext;
		}

		// and the structures
		for (unsigned playerId = 0; playerId < MAX_PLAYERS; ++playerId)
		{
			if (playerId != selectedPlayer)
			{
				STRUCTURE *psStruct = apsStructLists[playerId];

				while (psStruct)
				{
					psStruct->visible[selectedPlayer] = 0;
					psStruct = psStruct->psNext;
				}
			}
		}
		// remove all proximity messages
		releaseAllProxDisp();
		radarPermitted = structureExists(selectedPlayer, REF_HQ, true, false) || structureExists(selectedPlayer, REF_HQ, true, true);
	}
	else
	{
		pastReveal = getRevealStatus();
		enableGodMode();
	}

	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, godMode ? _("God Mode ON") : _("God Mode OFF"));
	sendInGameSystemMessage(cmsg.c_str());
}
// --------------------------------------------------------------------------
/* Aligns the view to north - some people can't handle the world spinning */
void	kf_SeekNorth()
{
	playerPos.r.y = 0;
	if (getWarCamStatus())
	{
		camToggleStatus();
	}
	CONPRINTF("%s", _("View Aligned to North"));
}

MappableFunction kf_ScrollCamera(const int horizontal, const int vertical)
{
	return [horizontal, vertical]() {
		scrollDirLeftRight += horizontal;
		scrollDirUpDown += vertical;
	};
}

void kf_toggleTrapCursor()
{
	bool trap = !war_GetTrapCursor();
	war_SetTrapCursor(trap);
	(trap ? wzGrabMouse : wzReleaseMouse)();
	std::string msg = astringf(_("Trap cursor %s"), trap ? "ON" : "OFF");
	addConsoleMessage(msg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}


// --------------------------------------------------------------------------
void	kf_TogglePauseMode()
{
	// Bail out if we're running a _true_ multiplayer game (which cannot be paused)
	if (runningMultiplayer())
	{
		return;
	}

	/* Is the game running? */
	if (!gamePaused())
	{
		/* Then pause it */
		setGamePauseStatus(true);
		setConsolePause(true);
		setScriptPause(true);
		setAudioPause(true);
		setScrollPause(true);

		// If cursor trapping is enabled allow the cursor to leave the window
		if (war_GetTrapCursor())
		{
			wzReleaseMouse();
		}

		/* And stop the clock */
		gameTimeStop();
		addConsoleMessage(_("PAUSED"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);


		// check if campaign and display gamemode

		bool campaignTrue = false;

		auto gameMode = ActivityManager::instance().getCurrentGameMode();

		switch (gameMode)
		{
			case ActivitySink::GameMode::CAMPAIGN:
				addConsoleMessage(_("CAMPAIGN"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
				campaignTrue = true;
				break;
			case ActivitySink::GameMode::CHALLENGE:
				addConsoleMessage(_("CHALLENGE"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
				break;
			case ActivitySink::GameMode::SKIRMISH:
				addConsoleMessage(_("SKIRMISH"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
				break;
			default:
				break;
		}


		// display level info
		addConsoleMessage(_(getLevelName()), CENTRE_JUSTIFY, SYSTEM_MESSAGE);

		// if we are playing campaign, display difficulty
		if (campaignTrue)
		{
			DIFFICULTY_LEVEL lev = getDifficultyLevel();

			switch (lev)
			{
				case DL_SUPER_EASY:
					addConsoleMessage(_("DIFFICULTY: SUPER EASY"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
					break;
				case DL_EASY:
					addConsoleMessage(_("DIFFICULTY: EASY"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
					break;
				case DL_NORMAL:
					addConsoleMessage(_("DIFFICULTY: NORMAL"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
					break;
				case DL_HARD:
					addConsoleMessage(_("DIFFICULTY: HARD"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
					break;
				case DL_INSANE:
					addConsoleMessage(_("DIFFICULTY: INSANE"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
					break;
			}
		}

		// display cheats status
		const DebugInputManager& dbgInputManager = gInputManager.debugManager();
		if (dbgInputManager.debugMappingsAllowed())
		{
			addConsoleMessage(_("CHEATS: ENABLED"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		} else {
			addConsoleMessage(_("CHEATS: DISABLED"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		}

	}
	else
	{
		/* Else get it going again */
		setGamePauseStatus(false);
		setConsolePause(false);
		setScriptPause(false);
		setAudioPause(false);
		setScrollPause(false);

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
void	kf_FinishAllResearch()
{
	UDWORD	j;

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

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
		if (!IsResearchCompleted(&asPlayerResList[selectedPlayer][j]))
		{
			if (bMultiMessages)
			{
				SendResearch(selectedPlayer, j, false);
				// Wait for our message before doing anything.
			}
			else
			{
				MakeResearchCompleted(&asPlayerResList[selectedPlayer][j]);
				researchResult(j, selectedPlayer, false, nullptr, false);
			}
		}
	}
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("Researched EVERYTHING for you!"));
	sendInGameSystemMessage(cmsg.c_str());
}

void kf_Reload()
{
	STRUCTURE	*psCurr;

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	for (psCurr = interfaceStructList(); psCurr; psCurr = psCurr->psNext)
	{
		if (isLasSat(psCurr->pStructureType) && psCurr->selected)
		{
			unsigned int firePause = weaponFirePause(&asWeaponStats[psCurr->asWeaps[0].nStat], psCurr->player);

			psCurr->asWeaps[0].lastFired -= firePause;
			CONPRINTF("%s", _("Selected buildings instantly recharged!"));
		}
	}
}

// --------------------------------------------------------------------------
// finish all the research for the selected player
void	kf_FinishResearch()
{
	STRUCTURE	*psCurr;

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	for (psCurr = interfaceStructList(); psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_RESEARCH)
		{
			BASE_STATS	*pSubject;

			// find out what we are researching here
			pSubject = ((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject;
			if (pSubject)
			{
				int rindex = ((RESEARCH *)pSubject)->index;
				if (bMultiMessages)
				{
					SendResearch(selectedPlayer, rindex, true);
					// Wait for our message before doing anything.
				}
				else
				{
					researchResult(rindex, selectedPlayer, true, psCurr, true);
				}
				std::string cmsg = astringf(_("(Player %u) is using cheat :%s %s"), selectedPlayer, _("Researched"), getStatsName(pSubject));
				sendInGameSystemMessage(cmsg.c_str());
				intResearchFinished(psCurr);
			}
		}
	}
}

// --------------------------------------------------------------------------
void	kf_ToggleEnergyBars()
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
void	kf_ChooseOptions()
{
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("Debug menu is Open"));
	sendInGameSystemMessage(cmsg.c_str());
	jsShowDebug();
}

// --------------------------------------------------------------------------
void	kf_ToggleProximitys()
{
	setProximityDraw(!doWeDrawProximitys());
}
// --------------------------------------------------------------------------
void	kf_JumpToResourceExtractor()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

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
		playerPos.r.y = 0; // face north
		setViewPos(map_coord(psOldRE->pos.x), map_coord(psOldRE->pos.y), true);
	}
	else
	{
		addConsoleMessage(_("Unable to locate any oil derricks!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}

}

// --------------------------------------------------------------------------
MappableFunction kf_JumpToUnits(const DROID_TYPE droidType)
{
	return [droidType]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		selNextSpecifiedUnit(droidType);
	};
}

// --------------------------------------------------------------------------
void	kf_JumpToUnassignedUnits()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	selNextUnassignedUnit();
}
// --------------------------------------------------------------------------


void	kf_ToggleOverlays()
{
	if (getWidgetsStatus() && !isChatUp() && !gamePaused())
	{
		setWidgetsStatus(false);
	}
	else
	{
		setWidgetsStatus(true);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseCommand()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	if (intCheckReticuleButEnabled(IDRET_COMMAND))
	{
		setKeyButtonMapping(IDRET_COMMAND);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseManufacture()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	if (intCheckReticuleButEnabled(IDRET_MANUFACTURE))
	{
		setKeyButtonMapping(IDRET_MANUFACTURE);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseResearch()
{
	if (intCheckReticuleButEnabled(IDRET_RESEARCH))
	{
		setKeyButtonMapping(IDRET_RESEARCH);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseBuild()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	if (intCheckReticuleButEnabled(IDRET_BUILD))
	{
		setKeyButtonMapping(IDRET_BUILD);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseDesign()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	if (intCheckReticuleButEnabled(IDRET_DESIGN))
	{
		setKeyButtonMapping(IDRET_DESIGN);
	}
}

// --------------------------------------------------------------------------
void	kf_ChooseIntelligence()
{
	if (intCheckReticuleButEnabled(IDRET_INTEL_MAP))
	{
		setKeyButtonMapping(IDRET_INTEL_MAP);
	}
}

// --------------------------------------------------------------------------

void	kf_ChooseCancel()
{
	setKeyButtonMapping(IDRET_CANCEL);
}

// --------------------------------------------------------------------------
void	kf_MovePause()
{
	if (!bMultiPlayer)	// can't do it in multiplay
	{
		if (!bMovePause)
		{
			/* Then pause it */
			setGamePauseStatus(true);
			setConsolePause(true);
			setScriptPause(true);
			setAudioPause(true);
			/* And stop the clock */
			gameTimeStop();
			setWidgetsStatus(false);
			bMovePause = true;
		}
		else
		{
			setWidgetsStatus(true);
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
void	kf_MoveToLastMessagePos()
{
	int iX, iY, iZ;

	if (!audio_GetPreviousQueueTrackPos(&iX, &iY, &iZ))
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
void	kf_ToggleWeather()
{
	if (atmosGetWeatherType() == WT_NONE)
	{
		atmosSetWeatherType(WT_SNOWING);
		addConsoleMessage(_("Oh, the weather outside is frightful... SNOW"), LEFT_JUSTIFY, SYSTEM_MESSAGE);

	}
	else if (atmosGetWeatherType() == WT_SNOWING)
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
MappableFunction kf_SelectNextFactory(const STRUCTURE_TYPE factoryType, const bool bJumpToSelected)
{
	static const std::vector<STRUCTURE_TYPE> FACTORY_TYPES = {
		STRUCTURE_TYPE::REF_FACTORY,
		STRUCTURE_TYPE::REF_CYBORG_FACTORY,
		STRUCTURE_TYPE::REF_VTOL_FACTORY,
	};

	return [factoryType, bJumpToSelected]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		selNextSpecifiedBuilding(factoryType, bJumpToSelected);

		STRUCTURE* psCurrent;

		//deselect factories of other types
		for (psCurrent = apsStructLists[selectedPlayer]; psCurrent; psCurrent = psCurrent->psNext)
		{
			const STRUCTURE_TYPE currentType = psCurrent->pStructureType->type;
			const bool bIsAnotherTypeOfFactory = currentType != factoryType && std::any_of(FACTORY_TYPES.begin(), FACTORY_TYPES.end(), [currentType](const STRUCTURE_TYPE type) {
				return currentType == type;
			});
			if (psCurrent->selected && bIsAnotherTypeOfFactory)
			{
				psCurrent->selected = false;
			}
		}

		if (intCheckReticuleButEnabled(IDRET_MANUFACTURE))
		{
			setKeyButtonMapping(IDRET_MANUFACTURE);
		}
		triggerEventSelected();
	};
}

// --------------------------------------------------------------------------
MappableFunction kf_SelectNextResearch(const bool bJumpToSelected)
{
	return [bJumpToSelected]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		selNextSpecifiedBuilding(REF_RESEARCH, bJumpToSelected);
		if (intCheckReticuleButEnabled(IDRET_RESEARCH))
		{
			setKeyButtonMapping(IDRET_RESEARCH);
		}
		triggerEventSelected();
	};
}

// --------------------------------------------------------------------------
MappableFunction kf_SelectNextPowerStation(const bool bJumpToSelected)
{
	return [bJumpToSelected]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		selNextSpecifiedBuilding(REF_POWER_GEN, bJumpToSelected);
		triggerEventSelected();
	};
}

// --------------------------------------------------------------------------
void	kf_KillEnemy()
{
	DROID		*psCDroid, *psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	if (selectedPlayer >= MAX_PLAYERS) { return; }

	debug(LOG_DEATH, "Destroying enemy droids and structures");
	CONPRINTF("%s", _("Warning! This can have drastic consequences if used incorrectly in missions."));
	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("All enemies destroyed by cheating!"));
	sendInGameSystemMessage(cmsg.c_str());
	Cheated = true;

	for (int playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		if (playerId != selectedPlayer)
		{
			// wipe out all the droids
			for (psCDroid = apsDroidLists[playerId]; psCDroid; psCDroid = psNDroid)
			{
				psNDroid = psCDroid->psNext;
				SendDestroyDroid(psCDroid);
			}
			// wipe out all their structures
			for (psCStruct = apsStructLists[playerId]; psCStruct; psCStruct = psNStruct)
			{
				psNStruct = psCStruct->psNext;
				SendDestroyStructure(psCStruct);
			}
		}
	}
}

// kill all the selected objects
void kf_KillSelected()
{
	DROID		*psCDroid, *psNDroid;
	STRUCTURE	*psCStruct, *psNStruct;

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

#ifndef DEBUG
	// Bail out if we're running a _true_ multiplayer game (to prevent MP cheating)
	if (runningMultiplayer())
	{
		noMPCheatMsg();
		return;
	}
#endif

	std::string cmsg = astringf(_("(Player %u) is using cheat :%s"),
	          selectedPlayer, _("Destroying selected droids and structures!"));
	sendInGameSystemMessage(cmsg.c_str());

	debug(LOG_DEATH, "Destroying selected droids and structures");
	audio_PlayTrack(ID_SOUND_COLL_DIE);
	Cheated = true;

	for (psCDroid = apsDroidLists[selectedPlayer]; psCDroid; psCDroid = psNDroid)
	{
		psNDroid = psCDroid->psNext;
		if (psCDroid->selected)
		{
			SendDestroyDroid(psCDroid);
		}
	}
	for (psCStruct = apsStructLists[selectedPlayer]; psCStruct; psCStruct = psNStruct)
	{
		psNStruct = psCStruct->psNext;
		if (psCStruct->selected)
		{
			SendDestroyStructure(psCStruct);
		}
	}
}

// --------------------------------------------------------------------------
// Chat message. NOTE THIS FUNCTION CAN DISABLE ALL OTHER KEYPRESSES
void kf_SendTeamMessage()
{
	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	if (!getWidgetsStatus())
	{
		return;
	}

	if (bAllowOtherKeyPresses && !gamePaused())  // just starting.
	{
		bAllowOtherKeyPresses = false;
		sstrcpy(sCurrentConsoleText, "");			//for beacons
		inputClearBuffer();
		chatDialog(CHAT_TEAM);						// throw up the dialog
	}
	else
	{
		bAllowOtherKeyPresses = true;
	}
}

// Chat message. NOTE THIS FUNCTION CAN DISABLE ALL OTHER KEYPRESSES
void kf_SendGlobalMessage()
{
	if (!getWidgetsStatus())
	{
		return;
	}

	if (bAllowOtherKeyPresses && !gamePaused())  // just starting.
	{
		bAllowOtherKeyPresses = false;
		sstrcpy(sCurrentConsoleText, "");			//for beacons
		inputClearBuffer();
		chatDialog(CHAT_GLOB);						// throw up the dialog
	}
	else
	{
		bAllowOtherKeyPresses = true;
	}
}

// --------------------------------------------------------------------------
void	kf_ToggleConsole()
{
	if (getConsoleDisplayStatus())
	{
		enableConsoleDisplay(false);
	}
	else
	{
		enableConsoleDisplay(true);
	}
}

// --------------------------------------------------------------------------
MappableFunction kf_SelectUnits(const SELECTIONTYPE selectionType, const SELECTION_CLASS selectionClass, const bool bOnScreen)
{
	return [selectionClass, selectionType, bOnScreen]() {
		selDroidSelection(selectedPlayer, selectionClass, selectionType, bOnScreen);
	};
}

// --------------------------------------------------------------------------
MappableFunction kf_SelectNoGroupUnits(const SELECTIONTYPE selectionType, const SELECTION_CLASS selectionClass, const bool bOnScreen)
{
	return [selectionClass, selectionType, bOnScreen]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		activateNoGroup(selectedPlayer, selectionType, selectionClass, bOnScreen);
	};
}

// --------------------------------------------------------------------------
MappableFunction kf_SetDroid(const SECONDARY_ORDER order, const SECONDARY_STATE state)
{
	return [order, state]() {
		kfsf_SetSelectedDroidsState(order, state);
	};
}

// --------------------------------------------------------------------------
MappableFunction kf_OrderDroid(const DroidOrderType order)
{
	return [order]() {
		/* not supported if a spectator */
		SPECTATOR_NO_OP();

		for (DROID* psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->selected)
			{
				orderDroid(psDroid, order, ModeQueue);
			}
		}
		intRefreshOrder();
	};
}

// --------------------------------------------------------------------------
void	kf_ToggleVisibility()
{
	if (getRevealStatus())
	{
		CONPRINTF("%s", _("Reveal OFF"));
		setRevealStatus(false);
	}
	else
	{
		CONPRINTF("%s", _("Reveal ON"));
		setRevealStatus(true);
	}
}

// --------------------------------------------------------------------------
static void kfsf_SetSelectedDroidsState(SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	DROID	*psDroid;

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	// NOT A CHEAT CODE
	// This is a function to set unit orders via keyboard shortcuts. It should
	// _not_ be disallowed in multiplayer games.

	// This code is similar to SetSecondaryState() in intorder.cpp. Unfortunately, it seems hard to un-duplicate the code.
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		// Only set the state if it's not a transporter.
		if (psDroid->selected && !isTransporter(psDroid))
		{
			secondarySetState(psDroid, sec, state);
		}
	}
	intRefreshOrder();
}

// --------------------------------------------------------------------------
void	kf_TriggerRayCast()
{
	DROID	*psDroid;
	bool	found;

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	found = false;
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid && !found;
	     psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			found = true;
		}
		/* NOP */
	}

	if (found)
	{
//		getBlockHeightDirToEdgeOfGrid(UDWORD x, UDWORD y, UBYTE direction, UDWORD *height, UDWORD *dist)
//		getBlockHeightDirToEdgeOfGrid(psOther->pos.x,psOther->pos.y,psOther->direction,&height,&dist);
//		getBlockHeightDirToEdgeOfGrid(mouseTileX*TILE_UNITS,mouseTileY*TILE_UNITS,getTestAngle(),&height,&dist);
	}
}

// --------------------------------------------------------------------------
void	kf_CentreOnBase()
{
	STRUCTURE	*psStruct;
	bool		bGotHQ;
	UDWORD		xJump = 0, yJump = 0;

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	/* Got through our buildings */
	for (psStruct = apsStructLists[selectedPlayer], bGotHQ = false;	// start
	     psStruct && !bGotHQ;											// terminate
	     psStruct = psStruct->psNext)									// iteration
	{
		/* Have we got a HQ? */
		if (psStruct->pStructureType->type == REF_HQ)
		{
			bGotHQ = true;
			xJump = psStruct->pos.x;
			yJump = psStruct->pos.y;
		}
	}

	/* If we found it, then jump to it! */
	if (bGotHQ)
	{
		addConsoleMessage(_("Centered on player HQ, direction NORTH"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		playerPos.p.x = xJump;
		playerPos.p.z = yJump;
		playerPos.r.y = 0; // face north
		/* A fix to stop the camera continuing when marker code is called */
		if (getWarCamStatus())
		{
			camToggleStatus();
		}
	}
	else
	{
		addConsoleMessage(_("Unable to locate HQ!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

// --------------------------------------------------------------------------
void kf_ToggleFormationSpeedLimiting()
{
	addConsoleMessage(_("Formation speed limiting has been removed from the game due to bugs."), LEFT_JUSTIFY, SYSTEM_MESSAGE);
}

// --------------------------------------------------------------------------
void	kf_RightOrderMenu()
{
	DROID	*psDroid, *psGotOne = nullptr;
	bool	bFound;

	// if menu open, then close it!
	if (widgGetFromID(psWScreen, IDORDER_FORM) != nullptr)
	{
		intRemoveOrder();	// close the screen.
		return;
	}

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	for (psDroid = apsDroidLists[selectedPlayer], bFound = false;
	     psDroid && !bFound; psDroid = psDroid->psNext)
	{
		if (psDroid->selected) // && droidOnScreen(psDroid,0))
		{
			bFound = true;
			psGotOne = psDroid;
		}
	}
	if (bFound)
	{
		intResetScreen(true);
		intObjectSelected((BASE_OBJECT *)psGotOne);
	}
}

// --------------------------------------------------------------------------
void	kf_ToggleMouseInvert()
{
	if (getInvertMouseStatus())
	{
		setInvertMouseStatus(false);
		CONPRINTF("%s", _("Vertical rotation direction: Normal"));
	}
	else
	{
		setInvertMouseStatus(true);
		CONPRINTF("%s", _("Vertical rotation direction: Flipped"));
	}
}

// --------------------------------------------------------------------------
void	kf_ToggleShakeStatus(void)
{
	if (getShakeStatus())
	{
		setShakeStatus(false);
		CONPRINTF("%s", _("Screen shake when things die: Off"));
	}
	else
	{
		setShakeStatus(true);
		CONPRINTF("%s", _("Screen shake when things die: On"));
	}
}

// --------------------------------------------------------------------------
void	kf_ToggleShadows()
{
	if (getDrawShadows())
	{
		setDrawShadows(false);
	}
	else
	{
		setDrawShadows(true);
	}
}
// --------------------------------------------------------------------------

static const Rational available_speed[] =
{
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
	if (runningMultiplayer() || bInTutorial)
	{
		if (!bInTutorial)
		{
			addConsoleMessage(_("Sorry, but game speed cannot be changed in multiplayer."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
		return;
	}

	// only in debug/cheat mode do we enable all time compression speeds.
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!dbgInputManager.debugMappingsAllowed() && (newMod > 2 || newMod <= 0))  // 2 = max officially allowed time compression
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
		CONPRINTF("%s", _("Game Speed Reset"));
	}
	else if (newMod > oldMod)
	{
		CONPRINTF(_("Game Speed Increased to %s"), modString);
	}
	else
	{
		CONPRINTF(_("Game Speed Reduced to %s"), modString);
	}
	gameTimeSetMod(newMod);
}

void kf_SpeedUp()
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

void kf_SlowDown()
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

void kf_NormalSpeed()
{
	// Bail out if we're running a _true_ multiplayer game or are playing a tutorial
	if (runningMultiplayer() || bInTutorial)
	{
		if (!bInTutorial)
		{
			noMPCheatMsg();
		}
		return;
	}

	CONPRINTF("%s", _("Game Speed Reset"));
	gameTimeResetMod();
}

// --------------------------------------------------------------------------

void kf_ToggleRadarAllyEnemy()
{
	bEnemyAllyRadarColor = !bEnemyAllyRadarColor;

	if (bEnemyAllyRadarColor)
	{
		CONPRINTF("%s", _("Radar showing friend-foe colors"));
	}
	else
	{
		CONPRINTF("%s", _("Radar showing player colors"));
	}
	resizeRadar();
}

void kf_ToggleRadarTerrain()
{
	radarDrawMode = (RADAR_DRAW_MODE)(radarDrawMode + 1);
	if (radarDrawMode >= NUM_RADAR_MODES)
	{
		radarDrawMode = (RADAR_DRAW_MODE)0;
	}
	switch (radarDrawMode)
	{
	case RADAR_MODE_NO_TERRAIN:
		CONPRINTF("%s", _("Radar showing only objects"));
		break;
	case RADAR_MODE_COMBINED:
		CONPRINTF("%s", _("Radar blending terrain and height"));
		break;
	case RADAR_MODE_TERRAIN:
		CONPRINTF("%s", _("Radar showing terrain"));
		break;
	case RADAR_MODE_HEIGHT_MAP:
		CONPRINTF("%s", _("Radar showing height"));
		break;
	case NUM_RADAR_MODES:
		assert(false);
	}
}

//Add a beacon (blip)
void	kf_AddHelpBlip()
{
	int		worldX = -1, worldY = -1;
	UDWORD	i;
	char	tempStr[255];
	SDWORD	x, y;

	/* not needed in campaign */
	if (!bMultiPlayer)
	{
		return;
	}

	/* not supported if a spectator */
	SPECTATOR_NO_OP();

	debug(LOG_WZ, "Adding beacon='%s'", sCurrentConsoleText);

	/* check if clicked on radar */
	x = mouseX();
	y = mouseY();
	if (isMouseOverRadar())
	{
		CalcRadarPosition(x, y, &worldX, &worldY);
		worldX = worldX * TILE_UNITS + TILE_UNITS / 2;
		worldY = worldY * TILE_UNITS + TILE_UNITS / 2;
	}
	/* convert screen to world */
	else
	{
		worldX = mouseTileX * TILE_UNITS + TILE_UNITS / 2;
		worldY = mouseTileY * TILE_UNITS + TILE_UNITS / 2;
	}

	sstrcpy(tempStr, getPlayerName(selectedPlayer));		//temporary solution

	/* add beacon for the sender */
	sstrcpy(beaconMsg[selectedPlayer], tempStr);
	sendBeaconToPlayer(worldX, worldY, selectedPlayer, selectedPlayer, beaconMsg[selectedPlayer]);

	/* send beacon to other players */
	for (i = 0; i < game.maxPlayers; i++)
	{
		if (openchannels[i] && (i != selectedPlayer))
		{
			sstrcpy(beaconMsg[i], tempStr);
			sendBeaconToPlayer(worldX, worldY, i, selectedPlayer, beaconMsg[i]);
		}
	}
}

void kf_NoAssert()
{
	debugDisableAssert();
	console(_("Asserts turned off"));
	debug(LOG_ERROR, "Asserts turned off");
}

// routine to decrement the tab-scroll 'buttons'
void kf_BuildPrevPage()
{
	ASSERT_OR_RETURN(, psWScreen != nullptr, " Invalid screen pointer!");
	ListTabWidget *psTForm = (ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM);
	if (psTForm == nullptr)
	{
		return;
	}

	if ((psTForm->currentPage() == 0) || !psTForm->setCurrentPage(psTForm->currentPage() - 1))
	{
		audio_PlayBuildFailedOnce();
		return;
	}

	audio_PlayTrack(ID_SOUND_BUTTON_CLICK_5);
}

// routine to advance the tab-scroll 'buttons'
void kf_BuildNextPage()
{
	ASSERT_OR_RETURN(, psWScreen != nullptr, " Invalid screen pointer!");
	ListTabWidget *psTForm = (ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM);
	if (psTForm == nullptr)
	{
		return;
	}

	if (!psTForm->setCurrentPage(psTForm->currentPage() + 1))
	{
		// went over max
		audio_PlayBuildFailedOnce();
		return;
	}

	audio_PlayTrack(ID_SOUND_BUTTON_CLICK_5);
}

void kf_QuickSave()
{
	// Bail out if we're running a _true_ multiplayer game or are playing a tutorial
	if (runningMultiplayer() || bInTutorial)
	{
		console(_("QuickSave not allowed for multiplayer or tutorial games"));
		return;
	}
	if (InGameOpUp || isInGamePopupUp)
	{
		return;
	}

	const char *filename = bMultiPlayer ? QUICKSAVE_SKI_FILENAME : QUICKSAVE_CAM_FILENAME;
	const char *quickSaveFolder = bMultiPlayer ? QUICKSAVE_SKI_FOLDER : QUICKSAVE_CAM_FOLDER;
	if (WZ_PHYSFS_isDirectory(quickSaveFolder))
	{
		deleteSaveGame(quickSaveFolder);
	}
	if (saveGame(filename, GTYPE_SAVE_MIDMISSION)) // still expects a .gam filename... TODO: FIX
	{
		console(_("QuickSave"));
	}
	else
	{
		console(_("QuickSave failed"));
	}
}

void kf_QuickLoad()
{
	// Bail out if we're running a _true_ multiplayer game or are playing a tutorial
	if (runningMultiplayer() || bInTutorial)
	{
		console(_("QuickLoad not allowed for multiplayer or tutorial games"));
		return;
	}
	if (InGameOpUp || isInGamePopupUp)
	{
		return;
	}

	const char *filename = bMultiPlayer ? QUICKSAVE_SKI_FILENAME : QUICKSAVE_CAM_FILENAME;
	// check for .json version, because that's what going to be loaded anyway
	if (PHYSFS_exists(filename) || PHYSFS_exists(bMultiPlayer ? QUICKSAVE_SKI_JSON_FILENAME : QUICKSAVE_CAM_JSON_FILENAME))
	{
		console(_("QuickLoad"));
		audio_StopAll();
		//clear out any mission widgets - timers etc that may be on the screen
		clearMissionWidgets();
		setWidgetsStatus(true);
		intResetScreen(false);
		wzSetCursor(CURSOR_DEFAULT);
		int campaign = getCampaign(filename);
		setCampaignNumber(campaign);

		loopMissionState = LMS_LOADGAME;
		sstrcpy(saveGameName, filename);
	}
	else
	{
		console(_("QuickSave file does not exist yet"));
	}
}

void kf_ToggleFullscreen()
{
	war_setWindowMode(wzAltEnterToggleFullscreen());
}

void kf_ToggleSpecOverlays()
{
	if (realSelectedPlayer >= NetPlay.players.size() || !NetPlay.players[realSelectedPlayer].isSpectator)
	{
		return;
	}
	specToggleOverlays();
}

void keybindShutdown()
{
	psOldRE = nullptr;
}
