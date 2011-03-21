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
/**
 * @file configuration.c
 * Saves your favourite options to the Registry.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/configfile.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/mixer.h"
#include "lib/ivis_opengl/screen.h"

#include "advvis.h"
#include "ai.h"
// HACK bAllowDebugMode shouldn't be in clparse
#include "clparse.h"
#include "component.h"
#include "configuration.h"
#include "difficulty.h"
#include "display3d.h"
#include "hci.h"
#include "multiint.h"
#include "multiplay.h"
#include "radar.h"
#include "seqdisp.h"
#include "texture.h"
#include "warzoneconfig.h"	// renderMode

// ////////////////////////////////////////////////////////////////////////////

#define DEFAULTSCROLL	1000

#define MASTERSERVERPORT	9990
#define GAMESERVERPORT		2100

// ////////////////////////////////////////////////////////////////////////////
bool loadConfig(void)
{
	int val;
	char	sBuf[255];
	bool bad_resolution = false;

	if (!openWarzoneKey())
	{
		return false;
	}

	//  options screens.
	// //////////////////////////

	// //////////////////////////
	// voice vol
	if(getWarzoneKeyNumeric("voicevol", &val))
	{
		sound_SetUIVolume((float)val / 100.0);//was val
	}

	// //////////////////////////
	// fx vol
	if(getWarzoneKeyNumeric("fxvol", &val))
	{
		sound_SetEffectsVolume((float)val / 100.0);//was val
	}

	// //////////////////////////
	// cdvol
	if(getWarzoneKeyNumeric("cdvol", &val))
	{
		sound_SetMusicVolume((float)val / 100.0);
	}

	if (getWarzoneKeyNumeric("debugmode", &val))
	{
		bAllowDebugMode = val;
	}
	else
	{
#ifdef DEBUG
		bAllowDebugMode = true;
#else
		bAllowDebugMode = false;
#endif
		setWarzoneKeyNumeric("debugmode", bAllowDebugMode);
	}

	if (getWarzoneKeyNumeric("music_enabled", &val))
	{
		war_SetMusicEnabled(val);
	}

	if (getWarzoneKeyString("language", sBuf))
	{
		setLanguage(sBuf);
	}

	if (getWarzoneKeyNumeric("showFPS", &val))
	{
		showFPS = val;
	}
	else
	{
		showFPS = false;
		setWarzoneKeyNumeric("showFPS", false);
	}

	// //////////////////////////
	// scroll
	if(getWarzoneKeyNumeric("scroll", &val))
	{
		scroll_speed_accel = val;
	}
	else
	{
		scroll_speed_accel = DEFAULTSCROLL;
		setWarzoneKeyNumeric("scroll", DEFAULTSCROLL);
	}

	// //////////////////////////
	// screen shake
	if(getWarzoneKeyNumeric("shake", &val))
	{
		setShakeStatus(val);
	}
	else
	{
		setShakeStatus(false);
		setWarzoneKeyNumeric("shake", false);
	}

	// //////////////////////////
	// draw shadows
	if(getWarzoneKeyNumeric("shadows", &val))
	{
		setDrawShadows(val);
	}
	else
	{
		setDrawShadows(true);
		setWarzoneKeyNumeric("shadows", true);
	}

	// //////////////////////////
	// enable sound
	if(getWarzoneKeyNumeric("sound", &val))
	{
		war_setSoundEnabled( val );
	}
	else
	{
		war_setSoundEnabled( true );
		setWarzoneKeyNumeric( "sound", true );
	}

	// //////////////////////////
	// invert mouse
	if(getWarzoneKeyNumeric("mouseflip", &val))
	{
		setInvertMouseStatus(val);
	}
	else
	{
		setInvertMouseStatus(true);
		setWarzoneKeyNumeric("mouseflip", true);
	}

	// //////////////////////////
	// mouse buttons
	if (getWarzoneKeyNumeric("RightClickOrders", &val))
	{
		setRightClickOrders(val);
	}
	else
	{
		setRightClickOrders(false);
		setWarzoneKeyNumeric("RightClickOrders", false);
	}
	if (getWarzoneKeyNumeric("MiddleClickRotate", &val))
	{
		setMiddleClickRotate(val);
	}
	else
	{
		setMiddleClickRotate(false);
		setWarzoneKeyNumeric("MiddleClickRotate", false);
	}
	
	// //////////////////////////
	// rotate radar
	if(getWarzoneKeyNumeric("rotateRadar", &val))
	{
		rotateRadar = val;
	}
	else
	{
		rotateRadar = true;
		setWarzoneKeyNumeric("rotateRadar", rotateRadar);
	}

	if (getWarzoneKeyNumeric("PauseOnFocusLoss", &val))
	{
		war_SetPauseOnFocusLoss(val);
	}
	else
	{
		war_SetPauseOnFocusLoss(false);
		setWarzoneKeyNumeric("PauseOnFocusLoss", false);
	}

	if (getWarzoneKeyString("masterserver_name", sBuf))
	{
		NETsetMasterserverName(sBuf);
		if (strcasecmp(sBuf, "lobby.wz2100.net") != 0)
		{
			debug(LOG_ERROR, "We are not using lobby.wz2100.net, for the master server name, we are using %s instead?", sBuf);
		}
	}
	else
	{
		NETsetMasterserverName("lobby.wz2100.net");
		setWarzoneKeyString("masterserver_name", NETgetMasterserverName());
	}

	if (getWarzoneKeyString("fontname", sBuf) && strcmp(sBuf,"Lucida Grande"))
	{
		iV_font(sBuf, NULL, NULL);
	}
	else
	{
		iV_font("DejaVu Sans", NULL, NULL);
		setWarzoneKeyString("fontname", "DejaVu Sans");
	}

	if (getWarzoneKeyString("fontface", sBuf) && strcmp(sBuf,"Normal"))
	{
		iV_font(NULL, sBuf, NULL);
	}
	else
	{
		iV_font(NULL, "Book", NULL);
		setWarzoneKeyString("fontface", "Book");
	}

	if (getWarzoneKeyString("fontfacebold", sBuf))
	{
		iV_font(NULL, NULL, sBuf);
	}
	else
	{
		iV_font(NULL, NULL, "Bold");
		setWarzoneKeyString("fontfacebold", "Bold");
	}

	if (getWarzoneKeyNumeric("masterserver_port", &val))
	{
		NETsetMasterserverPort(val);
		if (val != MASTERSERVERPORT)
		{
			debug(LOG_ERROR, "We are not using port %d (which is the default Master server port), we are using %d?", MASTERSERVERPORT, val);
		}
	}
	else
	{
		NETsetMasterserverPort(MASTERSERVERPORT);
		setWarzoneKeyNumeric("masterserver_port", NETgetMasterserverPort());
	}

	if (getWarzoneKeyNumeric("gameserver_port", &val))
	{
		NETsetGameserverPort(val);
		if (val != GAMESERVERPORT)
		{
			debug(LOG_ERROR, "We are not using port %d (which is the default Game server port), we are using %d?", GAMESERVERPORT, val);
		}
	}
	else
	{
		NETsetGameserverPort(GAMESERVERPORT);
		setWarzoneKeyNumeric("gameserver_port", NETgetGameserverPort());
	}

	// //////////////////////////
	// sequences
	if (getWarzoneKeyNumeric("FMVmode", &val))
	{
		war_SetFMVmode((FMV_MODE)val);
	}
	else
	{
		war_SetFMVmode(FMV_FULLSCREEN);
	}

	// //////////////////////////
	// subtitles
	if(getWarzoneKeyNumeric("subtitles", &val))
	{
		seq_SetSubtitles(val);
	}
	else
	{
		seq_SetSubtitles(true);
	}

	// //////////////////////////
	// difficulty

	if(getWarzoneKeyNumeric("difficulty", &val))
	{
		setDifficultyLevel((DIFFICULTY_LEVEL)val);
	}
	else
	{
		setDifficultyLevel(DL_NORMAL);
		setWarzoneKeyNumeric("difficulty", DL_NORMAL);
	}

	// //////////////////////////
	// use vis fog
	if(getWarzoneKeyNumeric("visfog", &val))
	{
		if(val)
		{
			war_SetFog(false);
		}
		else
		{
			war_SetFog(true);
		}
	}
	else
	{
		war_SetFog(true);
		setWarzoneKeyNumeric("visfog", 0);
	}

	// //////////////////////////
	// favourite colour
	if(!bMultiPlayer)
	{
		if(getWarzoneKeyNumeric("colour", &val))
		{
			war_SetSPcolor(val);
		}
		else
		{
			war_SetSPcolor(0);	//default is green (0)
			setWarzoneKeyNumeric("colour", 0);
		}
	}


	// /////////////////////////
	//  multiplayer stuff.
	// /////////////////////////

	// game name
	if (getWarzoneKeyString("gameName", sBuf))
	{
		sstrcpy(game.name, sBuf);
	}
	else
	{
		setWarzoneKeyString("gameName", "My Game");
	}

	// player name
	// must _not_ be an empty string
	if (getWarzoneKeyString("playerName", sBuf)
	 && *sBuf != '\0')
	{
		sstrcpy(sPlayer, sBuf);
	}
	else
	{
		setWarzoneKeyString("playerName", _("Player"));
		sstrcpy(sPlayer, _("Player"));
	}

	// map name
	if(getWarzoneKeyString("mapName", sBuf))
	{
		/* FIXME: Get rid of storing the max-player count in the config
		 *        file. Instead we should parse the map *before*
		 *        showing the skirmish/multiplayer setup screen.
		 */
		if (getWarzoneKeyNumeric("maxPlayers", &val))
		{
			sstrcpy(game.map, sBuf);
			game.maxPlayers = val;
		}
		else
		{
			debug(LOG_WARNING, "Invalid or not found maxPlayers parameter for map %s", game.map);
			debug(LOG_WARNING, "Reseting map to default parameters.");
			// we don't have maxPlayers set, so fall back to defaults.
			game.maxPlayers = 4;	//4 is for the DEFAULTSKIRMISHMAP map (rush)
			sstrcpy(game.map, DEFAULTSKIRMISHMAP);
			setWarzoneKeyString("mapName", game.map);
			setWarzoneKeyNumeric("maxPlayers",game.maxPlayers);
		}
	}
	else
	{
		sstrcpy(game.map, DEFAULTSKIRMISHMAP);
		setWarzoneKeyString("mapName", game.map);
	}

	// power
	if(getWarzoneKeyNumeric("power", &val))
	{
		game.power = val;
	}
	else
	{
		game.power = LEV_MED;
		setWarzoneKeyNumeric("power", game.power);
	}

	// fog
	if(getWarzoneKeyNumeric("fog", &val))
	{
		game.fog= val;
	}
	else
	{
		game.fog= true;
		setWarzoneKeyNumeric("fog", game.fog);
	}

	//base
	if(getWarzoneKeyNumeric("base", &val))
	{
		game.base =(UBYTE)val;
	}
	else
	{
		game.base = CAMP_BASE;
		setWarzoneKeyNumeric("base", game.base);
	}

	//alliance
	if(getWarzoneKeyNumeric("alliance", &val))
	{
		game.alliance =(UBYTE)val;
	}
	else
	{
		game.alliance = NO_ALLIANCES;
		setWarzoneKeyNumeric("alliance", game.alliance);
	}

	// favourite phrases
	if(getWarzoneKeyString("phrase0", ingame.phrases[0]))
	{
		getWarzoneKeyString("phrase1", ingame.phrases[1]);
		getWarzoneKeyString("phrase2", ingame.phrases[2]);
		getWarzoneKeyString("phrase3", ingame.phrases[3]);
		getWarzoneKeyString("phrase4", ingame.phrases[4]);
	}
	else
	{
		memset(&ingame.phrases, 0, sizeof(ingame.phrases));
		setWarzoneKeyString("phrase0", ingame.phrases[0]);
		setWarzoneKeyString("phrase1", ingame.phrases[1]);
		setWarzoneKeyString("phrase2", ingame.phrases[2]);
		setWarzoneKeyString("phrase3", ingame.phrases[3]);
		setWarzoneKeyString("phrase4", ingame.phrases[4]);
	}

	// enemy/allies radar view
	if(getWarzoneKeyNumeric("radarObjectMode", &val))
	{
		bEnemyAllyRadarColor =(bool)val;
	} else {
		bEnemyAllyRadarColor = false;
		setWarzoneKeyNumeric("radarObjectMode", (SDWORD)bEnemyAllyRadarColor);
	}

	// mini-map terrain mode
	if(getWarzoneKeyNumeric("radarTerrainMode", &val))
	{
		radarDrawMode = (RADAR_DRAW_MODE)val;

		if(radarDrawMode >= NUM_RADAR_MODES)
		{
			ASSERT(!"wrong mini-map mode", "loadConfig: wrong mini-map mode: %d", radarDrawMode);
			radarDrawMode = RADAR_MODE_DEFAULT;
		}
	} else {
		radarDrawMode = RADAR_MODE_DEFAULT;
		setWarzoneKeyNumeric("radarTerrainMode", radarDrawMode);
	}

	// texture size
	if (getWarzoneKeyNumeric("textureSize", &val))
	{
		setTextureSize(val);
	} else {
		setWarzoneKeyNumeric("textureSize", getTextureSize());
	}

	// UPnP detection
	if (getWarzoneKeyNumeric("UPnP", &val))
	{
		NetPlay.isUPNP = val;
	}
	else
	{
		setWarzoneKeyNumeric("UPnP", 1);
		NetPlay.isUPNP = 1;
	}

	if (getWarzoneKeyNumeric("FSAA", &val))
	{
		war_setFSAA(val);
	}
	else
	{
		setWarzoneKeyNumeric("FSAA", war_getFSAA());
	}

	if( getWarzoneKeyNumeric("fullscreen", &val) ) {
		war_setFullscreen(val);
	} else {
		// If no setting is found go to fullscreen by default
		setWarzoneKeyNumeric("fullscreen", true);
		war_setFullscreen(true);
	}

	if (getWarzoneKeyNumeric("ColouredCursor", &val))
	{
		war_SetColouredCursor(val);
	}
	else
	{
#ifdef WZ_OS_MAC
		// Mac OS X doesn't support normal cursors
		war_SetColouredCursor(true);
		setWarzoneKeyNumeric("ColouredCursor", true);
#else
		war_SetColouredCursor(false);
		setWarzoneKeyNumeric("ColouredCursor", false);
#endif
	}

	if (getWarzoneKeyNumeric("trapCursor", &val))
	{
		war_SetTrapCursor(val);
	}
	else
	{
		war_SetTrapCursor(false);
	}

	if (getWarzoneKeyNumeric("vsync", &val))
	{
		war_SetVsync(val);
	}
	else
	{
		war_SetVsync(true);
	}

	// now load the desired res..
	// note that we only do this if we havent changed renderer..
	if (getWarzoneKeyNumeric("width", &val)
	 && val >= 640)
	{
		pie_SetVideoBufferWidth(val);
		war_SetWidth(val);
	}
	else
	{
		bad_resolution = true;
	}

	if (getWarzoneKeyNumeric("height", &val)
	 && val >= 400)
	{
		pie_SetVideoBufferHeight(val);
		war_SetHeight(val);
	}
	else
	{
		bad_resolution = true;
	}

	if (bad_resolution)
	{
		// If we have an invalid or incomplete resolution specified
		// fall back to the defaults.
		war_SetWidth(0);
		war_SetHeight(0);
	}

	if (getWarzoneKeyNumeric("bpp", &val))
	{
		pie_SetVideoBufferDepth(val);
	}

	if (getWarzoneKeyNumeric("framerate", &val))
	{
		setFramerateLimit(val);
	}
	else
	{
		setFramerateLimit(60);
	}

	return closeWarzoneKey();
}

// ////////////////////////////////////////////////////////////////////////////
bool saveConfig(void)
{
	debug( LOG_WZ, "Writing prefs to registry\n" );

	if(!openWarzoneKey())
	{
		return false;
	}

	// //////////////////////////
	// voicevol, fxvol and cdvol
	setWarzoneKeyNumeric("voicevol", (int)(sound_GetUIVolume() * 100.0));
	setWarzoneKeyNumeric("fxvol", (int)(sound_GetEffectsVolume() * 100.0));
	setWarzoneKeyNumeric("cdvol", (int)(sound_GetMusicVolume() * 100.0));
	setWarzoneKeyNumeric("music_enabled", war_GetMusicEnabled());

	setWarzoneKeyNumeric("width", war_GetWidth());
	setWarzoneKeyNumeric("height", war_GetHeight());
	setWarzoneKeyNumeric("bpp", pie_GetVideoBufferDepth());
	setWarzoneKeyNumeric("fullscreen", war_getFullscreen());

	setWarzoneKeyString("language", getLanguage());

	// dont save out the cheat mode.
	if(getDifficultyLevel()==DL_KILLER || getDifficultyLevel()== DL_TOUGH)
	{
		setDifficultyLevel(DL_NORMAL);
	}
	setWarzoneKeyNumeric("debugmode", bAllowDebugMode);
	setWarzoneKeyNumeric("framerate", (SDWORD)getFramerateLimit());
	setWarzoneKeyNumeric("showFPS", (SDWORD)showFPS);
	setWarzoneKeyNumeric("scroll",(SDWORD)scroll_speed_accel);		// scroll
	setWarzoneKeyNumeric("difficulty", getDifficultyLevel());		// level
	setWarzoneKeyNumeric("visfog",(SDWORD)(!war_GetFog()));			// fogtype
	setWarzoneKeyNumeric("shake",(SDWORD)(getShakeStatus()));		// screenshake
	setWarzoneKeyNumeric("mouseflip",(SDWORD)(getInvertMouseStatus()));	// flipmouse
	setWarzoneKeyNumeric("RightClickOrders",(SDWORD)(getRightClickOrders()));
	setWarzoneKeyNumeric("MiddleClickRotate",(SDWORD)(getMiddleClickRotate()));
	setWarzoneKeyNumeric("shadows",(SDWORD)(getDrawShadows()));	// shadows
	setWarzoneKeyNumeric("sound", (SDWORD)war_getSoundEnabled());
	setWarzoneKeyNumeric("FMVmode",(SDWORD)(war_GetFMVmode()));		// sequences
	setWarzoneKeyNumeric("subtitles",(SDWORD)(seq_GetSubtitles()));		// subtitles
	setWarzoneKeyNumeric("radarObjectMode",(SDWORD)bEnemyAllyRadarColor);    // enemy/allies radar view
	setWarzoneKeyNumeric("radarTerrainMode",(SDWORD)radarDrawMode);
	setWarzoneKeyNumeric("trapCursor", war_GetTrapCursor());
	setWarzoneKeyNumeric("vsync", war_GetVsync());
	setWarzoneKeyNumeric("textureSize", getTextureSize());
	setWarzoneKeyNumeric("rotateRadar", rotateRadar);
	setWarzoneKeyNumeric("PauseOnFocusLoss", war_GetPauseOnFocusLoss());
	setWarzoneKeyNumeric("ColouredCursor", war_GetColouredCursor());
	setWarzoneKeyString("masterserver_name", NETgetMasterserverName());
	setWarzoneKeyNumeric("masterserver_port", NETgetMasterserverPort());
	setWarzoneKeyNumeric("gameserver_port", NETgetGameserverPort());

	if(!bMultiPlayer)
	{
		setWarzoneKeyNumeric("colour",(SDWORD)getPlayerColour(0));			// favourite colour.
	}
	else
	{
		debug( LOG_NEVER, "Writing multiplay prefs to registry\n" );
		if (NetPlay.isHost && ingame.localJoiningInProgress)
		{
			if (bMultiPlayer && NetPlay.bComms)
			{
				setWarzoneKeyString("gameName", game.name);			//  last hosted game
			}
			setWarzoneKeyString("mapName", game.map);				//  map name
			setWarzoneKeyNumeric("maxPlayers",game.maxPlayers);		// maxPlayers
			setWarzoneKeyNumeric("power", game.power);				// power
			setWarzoneKeyNumeric("base", game.base);				// size of base
			setWarzoneKeyNumeric("fog", game.fog);					// fog 'o war
			setWarzoneKeyNumeric("alliance", game.alliance);		// allow alliances
		}
		setWarzoneKeyString("playerName",(char*)sPlayer);		// player name
		setWarzoneKeyString("phrase0", ingame.phrases[0]);		// phrases
		setWarzoneKeyString("phrase1", ingame.phrases[1]);
		setWarzoneKeyString("phrase2", ingame.phrases[2]);
		setWarzoneKeyString("phrase3", ingame.phrases[3]);
		setWarzoneKeyString("phrase4", ingame.phrases[4]);
	}

	return closeWarzoneKey();
}

// Saves and loads the relevant part of the config files for MP games
// Ensures that others' games don't change our own configuration settings
bool reloadMPConfig(void)
{
	int val;
	char	sBuf[255];

	debug( LOG_WZ, "Reloading prefs prefs to registry\n" );

	// If we're in-game, we already have our own configuration set, so no need to reload it.

	if (NetPlay.isHost && !ingame.localJoiningInProgress)
	{
		if (bMultiPlayer && !NetPlay.bComms && openWarzoneKey())
		{
			// one-player skirmish mode sets game name to "One Player Skirmish", so
			// reset the name
			if (getWarzoneKeyString("gameName", sBuf))
			{
				sstrcpy(game.name, sBuf);
			}
			return closeWarzoneKey();
		}
		return true;
	}

	if(!openWarzoneKey())
	{
		return false;
	}

	// If we're host and not in-game, we can safely save our settings and return.

	if (NetPlay.isHost && ingame.localJoiningInProgress)
	{
		if (bMultiPlayer && NetPlay.bComms)
		{
			setWarzoneKeyString("gameName", game.name);			//  last hosted game
		}
		else
		{
			// One-player skirmish mode sets game name to "One Player Skirmish", so
			// reset the name
			if (getWarzoneKeyString("gameName", sBuf))
			{
				sstrcpy(game.name, sBuf);
			}
		}
		setWarzoneKeyString("mapName", game.map);				//  map name
		setWarzoneKeyNumeric("maxPlayers",game.maxPlayers);		// maxPlayers
		setWarzoneKeyNumeric("power", game.power);				// power
		setWarzoneKeyNumeric("base", game.base);				// size of base
		setWarzoneKeyNumeric("fog", game.fog);					// fog 'o war
		setWarzoneKeyNumeric("alliance", game.alliance);		// allow alliances
		return closeWarzoneKey();
	}

	// We're not host, so let's get rid of the host's game settings and restore our own.

	// game name
	if (getWarzoneKeyString("gameName", sBuf))
	{
		sstrcpy(game.name, sBuf);
	}

	// map name
	if(getWarzoneKeyString("mapName", sBuf))
	{
		/* FIXME: Get rid of storing the max-player count in the config
		 *        file. Instead we should parse the map *before*
		 *        showing the skirmish/multiplayer setup screen.
		 */
		if (getWarzoneKeyNumeric("maxPlayers", &val))
		{
			sstrcpy(game.map, sBuf);
			game.maxPlayers = val;
		}
	}

	// power
	if(getWarzoneKeyNumeric("power", &val))
	{
		game.power = val;
	}

	// fog
	if(getWarzoneKeyNumeric("fog", &val))
	{
		game.fog= val;
	}

	//base
	if(getWarzoneKeyNumeric("base", &val))
	{
		game.base =(UBYTE)val;
	}

	//alliance
	if(getWarzoneKeyNumeric("alliance", &val))
	{
		game.alliance =(UBYTE)val;
	}

	return closeWarzoneKey();
}

void closeConfig( void )
{
	registry_clear();
}
