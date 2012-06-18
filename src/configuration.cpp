/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * @file configuration.cpp
 * Saves your favourite options.
 *
 */

#include <QtCore/QSettings>
#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/input.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/mixer.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/framework/opengl.h"

#include "advvis.h"
#include "ai.h"
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

static const char *fileName = "config";

// ////////////////////////////////////////////////////////////////////////////
bool loadConfig()
{
	QSettings ini(PHYSFS_getWriteDir() + QString("/") + fileName, QSettings::IniFormat);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open configuration file \"%s\"", fileName);
		return false;
	}
	debug(LOG_WZ, "Reading configuration from %s", ini.fileName().toUtf8().constData());
	if (ini.contains("voicevol")) sound_SetUIVolume(ini.value("voicevol").toDouble() / 100.0);
	if (ini.contains("fxvol")) sound_SetEffectsVolume(ini.value("fxvol").toDouble() / 100.0);
	if (ini.contains("cdvol")) sound_SetMusicVolume(ini.value("cdvol").toDouble() / 100.0);
	if (ini.contains("music_enabled")) war_SetMusicEnabled(ini.value("music_enabled").toBool());
	if (ini.contains("language")) setLanguage(ini.value("language").toString().toUtf8().constData());
	if (ini.contains("nomousewarp")) setMouseWarp(ini.value("nomousewarp").toBool());
	if (ini.contains("notexturecompression")) wz_texture_compression = GL_RGBA;
	showFPS = ini.value("showFPS", false).toBool();
	scroll_speed_accel = ini.value("scroll", DEFAULTSCROLL).toInt();
	setShakeStatus(ini.value("shake", false).toBool());
	setDrawShadows(ini.value("shadows", true).toBool());
	war_setSoundEnabled(ini.value("sound", true).toBool());
	setInvertMouseStatus(ini.value("mouseflip", true).toBool());
	setRightClickOrders(ini.value("RightClickOrders", false).toBool());
	setMiddleClickRotate(ini.value("MiddleClickRotate", false).toBool());
	rotateRadar = ini.value("rotateRadar", true).toBool();
	war_SetPauseOnFocusLoss(ini.value("PauseOnFocusLoss", false).toBool());
	NETsetMasterserverName(ini.value("masterserver_name", "lobby.wz2100.net").toString().toUtf8().constData());
	iV_font(ini.value("fontname", "DejaVu Sans").toString().toUtf8().constData(),
		ini.value("fontface", "Book").toString().toUtf8().constData(),
		ini.value("fontfacebold", "Bold").toString().toUtf8().constData());
	NETsetMasterserverPort(ini.value("masterserver_port", MASTERSERVERPORT).toInt());
	NETsetGameserverPort(ini.value("gameserver_port", GAMESERVERPORT).toInt());
	war_SetFMVmode((FMV_MODE)ini.value("FMVmode", FMV_FULLSCREEN).toInt());
	war_setScanlineMode((SCANLINE_MODE)ini.value("scanlines", SCANLINES_OFF).toInt());
	seq_SetSubtitles(ini.value("subtitles", true).toBool());
	setDifficultyLevel((DIFFICULTY_LEVEL)ini.value("difficulty", DL_NORMAL).toInt());
	war_SetSPcolor(ini.value("colour", 0).toInt());	// default is green (0)
	war_setMPcolour(ini.value("colourMP", -1).toInt());  // default is random (-1)
	sstrcpy(game.name, ini.value("gameName", _("My Game")).toString().toUtf8().constData());
	sstrcpy(sPlayer, ini.value("playerName", _("Player")).toString().toUtf8().constData());
	if (ini.contains("mapName") && ini.contains("maxPlayers"))
	{
		sstrcpy(game.map, ini.value("mapName").toString().toUtf8().constData());
		game.hash.setZero();
		game.maxPlayers = ini.value("maxPlayers").toInt();	// FIXME: horrible kluge, MUST match map above
	}
	else
	{
		// Set a default map to prevent hosting games without a map.
		sstrcpy(game.map, "Sk-Rush");
		game.hash.setZero();
		game.maxPlayers = 4;
	}
	game.power = ini.value("power", LEV_MED).toInt();
	game.base = ini.value("base", CAMP_BASE).toInt();
	game.alliance = ini.value("alliance", NO_ALLIANCES).toInt();
	memset(&ingame.phrases, 0, sizeof(ingame.phrases));
	for (int i = 1; i < 5; i++)
	{
		QString key("phrase" + QString::number(i));
		if (ini.contains(key)) sstrcpy(ingame.phrases[i], ini.value(key).toString().toUtf8().constData());
	}
	bEnemyAllyRadarColor = ini.value("radarObjectMode").toBool();
	radarDrawMode = (RADAR_DRAW_MODE)ini.value("radarTerrainMode", RADAR_MODE_DEFAULT).toInt();
	radarDrawMode = (RADAR_DRAW_MODE)MIN(NUM_RADAR_MODES - 1, radarDrawMode); // restrict to allowed values
	if (ini.contains("textureSize")) setTextureSize(ini.value("textureSize").toInt());
	NetPlay.isUPNP = ini.value("UPnP", true).toBool();
	if (ini.contains("FSAA")) war_setFSAA(ini.value("FSAA").toInt());
	if (ini.contains("shaders")) war_SetShaders(ini.value("shaders").toInt());
	// Leave this to false, some system will fail and they can't see the system popup dialog!
	war_setFullscreen(ini.value("fullscreen", false).toBool());
	war_SetTrapCursor(ini.value("trapCursor", false).toBool());
	// this should be enabled on all systems by default
	war_SetVsync(ini.value("vsync", true).toBool());
	// 640x480 is minimum that we will support
	int width = ini.value("width", 640).toInt();
	int height = ini.value("height", 480).toInt();
	if (width < 640 || height < 480)	// sanity check
	{
		width = 640;
		height = 480;
	}
	pie_SetVideoBufferWidth(width);
	pie_SetVideoBufferHeight(height);
	war_SetWidth(width);
	war_SetHeight(height);

	if (ini.contains("bpp")) pie_SetVideoBufferDepth(ini.value("bpp").toInt());
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
bool saveConfig()
{
	QSettings ini(PHYSFS_getWriteDir() + QString("/") + fileName, QSettings::IniFormat);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open configuration file \"%s\"", fileName);
		return false;
	}
	debug(LOG_WZ, "Writing prefs to registry \"%s\"", ini.fileName().toUtf8().constData());

	// //////////////////////////
	// voicevol, fxvol and cdvol
	ini.setValue("voicevol", (int)(sound_GetUIVolume() * 100.0));
	ini.setValue("fxvol", (int)(sound_GetEffectsVolume() * 100.0));
	ini.setValue("cdvol", (int)(sound_GetMusicVolume() * 100.0));
	ini.setValue("music_enabled", war_GetMusicEnabled());
	ini.setValue("width", war_GetWidth());
	ini.setValue("height", war_GetHeight());
	ini.setValue("bpp", pie_GetVideoBufferDepth());
	ini.setValue("fullscreen", war_getFullscreen());
	ini.setValue("language", getLanguage());
	// dont save out the cheat mode.
	if (getDifficultyLevel() != DL_KILLER && getDifficultyLevel() != DL_TOUGH)
	{
		ini.setValue("difficulty", getDifficultyLevel());		// level
	}
	ini.setValue("showFPS", (SDWORD)showFPS);
	ini.setValue("scroll",(SDWORD)scroll_speed_accel);		// scroll
	ini.setValue("shake",(SDWORD)(getShakeStatus()));		// screenshake
	ini.setValue("mouseflip",(SDWORD)(getInvertMouseStatus()));	// flipmouse
	ini.setValue("nomousewarp", (SDWORD)getMouseWarp()); 		// mouse warp
	ini.setValue("RightClickOrders",(SDWORD)(getRightClickOrders()));
	ini.setValue("MiddleClickRotate",(SDWORD)(getMiddleClickRotate()));
	ini.setValue("showFPS", (SDWORD)showFPS);
	ini.setValue("shadows",(SDWORD)(getDrawShadows()));	// shadows
	ini.setValue("sound", (SDWORD)war_getSoundEnabled());
	ini.setValue("FMVmode",(SDWORD)(war_GetFMVmode()));		// sequences
	ini.setValue("scanlines", (SDWORD)war_getScanlineMode());
	ini.setValue("subtitles",(SDWORD)(seq_GetSubtitles()));		// subtitles
	ini.setValue("radarObjectMode",(SDWORD)bEnemyAllyRadarColor);    // enemy/allies radar view
	ini.setValue("radarTerrainMode",(SDWORD)radarDrawMode);
	ini.setValue("trapCursor", war_GetTrapCursor());
	ini.setValue("vsync", war_GetVsync());
	ini.setValue("shaders", war_GetShaders());
	ini.setValue("textureSize", getTextureSize());
	ini.setValue("FSAA", war_getFSAA());
	ini.setValue("UPnP", (SDWORD)NetPlay.isUPNP);
	ini.setValue("rotateRadar", rotateRadar);
	ini.setValue("PauseOnFocusLoss", war_GetPauseOnFocusLoss());
	ini.setValue("masterserver_name", NETgetMasterserverName());
	ini.setValue("masterserver_port", NETgetMasterserverPort());
	ini.setValue("gameserver_port", NETgetGameserverPort());
	if (!bMultiPlayer)
	{
		ini.setValue("colour", getPlayerColour(0));			// favourite colour.
	}
	else
	{
		if (NetPlay.isHost && ingame.localJoiningInProgress)
		{
			if (bMultiPlayer && NetPlay.bComms)
			{
				ini.setValue("gameName", game.name);			//  last hosted game
			}
			ini.setValue("mapName", game.map);				//  map name
			ini.setValue("maxPlayers", game.maxPlayers);		// maxPlayers
			ini.setValue("power", game.power);				// power
			ini.setValue("base", game.base);				// size of base
			ini.setValue("alliance", game.alliance);		// allow alliances
		}
		ini.setValue("playerName", (char*)sPlayer);		// player name
	}
	ini.setValue("colourMP", war_getMPcolour());
	ini.sync();
	return true;
}

// Saves and loads the relevant part of the config files for MP games
// Ensures that others' games don't change our own configuration settings
bool reloadMPConfig(void)
{
	QSettings ini(PHYSFS_getWriteDir() + QString("/") + fileName, QSettings::IniFormat);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open configuration file \"%s\"", fileName);
		return false;
	}
	debug(LOG_WZ, "Reloading prefs prefs to registry");

	// If we're in-game, we already have our own configuration set, so no need to reload it.
	if (NetPlay.isHost && !ingame.localJoiningInProgress)
	{
		if (bMultiPlayer && !NetPlay.bComms)
		{
			// one-player skirmish mode sets game name to "One Player Skirmish", so
			// reset the name
			if (ini.contains("gameName"))
			{
				sstrcpy(game.name, ini.value("gameName").toString().toUtf8().constData());
			}
		}
		return true;
	}

	// If we're host and not in-game, we can safely save our settings and return.
	if (NetPlay.isHost && ingame.localJoiningInProgress)
	{
		if (bMultiPlayer && NetPlay.bComms)
		{
			ini.setValue("gameName", game.name);			//  last hosted game
		}
		else
		{
			// One-player skirmish mode sets game name to "One Player Skirmish", so
			// reset the name
			if (ini.contains("gameName"))
			{
				sstrcpy(game.name, ini.value("gameName").toString().toUtf8().constData());
			}
		}
		ini.setValue("mapName", game.map);				//  map name
		ini.setValue("maxPlayers", game.maxPlayers);		// maxPlayers
		ini.setValue("power", game.power);				// power
		ini.setValue("base", game.base);				// size of base
		ini.setValue("alliance", game.alliance);		// allow alliances
		return true;
	}

	// We're not host, so let's get rid of the host's game settings and restore our own.

	// game name
	if (ini.contains("gameName"))
	{
		sstrcpy(game.name, ini.value("gameName").toString().toUtf8().constData());
	}
	if (ini.contains("mapName") && ini.contains("maxPlayers"))
	{
		sstrcpy(game.map, ini.value("mapName").toString().toUtf8().constData());
		game.hash.setZero();
		game.maxPlayers = ini.value("maxPlayers").toInt();	// FIXME: horrible kluge, MUST match map above
	}
	game.power = ini.value("power", LEV_MED).toInt();
	game.base = ini.value("base", CAMP_BASE).toInt();
	game.alliance = ini.value("alliance", NO_ALLIANCES).toInt();

	return true;
}

void closeConfig()
{
}
