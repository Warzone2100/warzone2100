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
/*
 * MultiInt.c
 *
 * Alex Lee, 98. Pumpkin Studios, Bath.
 * Functions to display and handle the multiplayer interface screens,
 * along with connection and game options.
 */

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"

#include <time.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/file.h"
#include "lib/framework/stdio_ext.h"

/* Includes direct access to render library */
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/screen.h"

#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "lib/script/script.h"
#include "lib/widget/editbox.h"
#include "lib/widget/button.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"

#include "challenge.h"
#include "main.h"
#include "levels.h"
#include "objects.h"
#include "display.h"// pal stuff
#include "display3d.h"
#include "objmem.h"
#include "gateway.h"
#include "clparse.h"
#include "configuration.h"
#include "intdisplay.h"
#include "design.h"
#include "hci.h"
#include "power.h"
#include "loadsave.h"			// for blueboxes.
#include "component.h"
#include "map.h"
#include "console.h"			// chat box stuff
#include "frend.h"
#include "advvis.h"
#include "frontend.h"
#include "data.h"
#include "keymap.h"
#include "game.h"
#include "warzoneconfig.h"
#include "modding.h"
#include "qtscript.h"
#include "random.h"

#include "multiplay.h"
#include "multiint.h"
#include "multijoin.h"
#include "multistat.h"
#include "multirecv.h"
#include "multimenu.h"
#include "multilimit.h"
#include "multigifts.h"

#include "warzoneconfig.h"

#include "init.h"
#include "levels.h"
#include "wrappers.h"

#ifndef WZ_OS_MAC
	#include <QFileInfo> // used to strip path of challenge AI values
#else // WZ_OS_MAC
	#include <QtCore/QFileInfo> // used to strip path of challenge AI values
#endif

#define MAP_PREVIEW_DISPLAY_TIME 2500	// number of milliseconds to show map in preview

// ////////////////////////////////////////////////////////////////////////////
// tertile dependent colors for map preview

// C1 - Arizona type
#define WZCOL_TERC1_CLIFF_LOW   pal_Colour(0x68, 0x3C, 0x24)
#define WZCOL_TERC1_CLIFF_HIGH  pal_Colour(0xE8, 0x84, 0x5C)
#define WZCOL_TERC1_WATER       pal_Colour(0x3F, 0x68, 0x9A)
#define WZCOL_TERC1_ROAD_LOW    pal_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC1_ROAD_HIGH   pal_Colour(0xB2, 0x9A, 0x66)
#define WZCOL_TERC1_GROUND_LOW  pal_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC1_GROUND_HIGH pal_Colour(0xCC, 0xB2, 0x80)
// C2 - Urban type
#define WZCOL_TERC2_CLIFF_LOW   pal_Colour(0x3C, 0x3C, 0x3C)
#define WZCOL_TERC2_CLIFF_HIGH  pal_Colour(0x84, 0x84, 0x84)
#define WZCOL_TERC2_WATER       WZCOL_TERC1_WATER
#define WZCOL_TERC2_ROAD_LOW    pal_Colour(0x00, 0x00, 0x00)
#define WZCOL_TERC2_ROAD_HIGH   pal_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC2_GROUND_LOW  pal_Colour(0x1F, 0x1F, 0x1F)
#define WZCOL_TERC2_GROUND_HIGH pal_Colour(0xB2, 0xB2, 0xB2)
// C3 - Rockies type
#define WZCOL_TERC3_CLIFF_LOW   pal_Colour(0x3C, 0x3C, 0x3C)
#define WZCOL_TERC3_CLIFF_HIGH  pal_Colour(0xFF, 0xFF, 0xFF)
#define WZCOL_TERC3_WATER       WZCOL_TERC1_WATER
#define WZCOL_TERC3_ROAD_LOW    pal_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC3_ROAD_HIGH   pal_Colour(0x3D, 0x21, 0x0A)
#define WZCOL_TERC3_GROUND_LOW  pal_Colour(0x00, 0x1C, 0x0E)
#define WZCOL_TERC3_GROUND_HIGH WZCOL_TERC3_CLIFF_HIGH

// ////////////////////////////////////////////////////////////////////////////
// vars
extern char	MultiCustomMapsPath[PATH_MAX];
extern char	MultiPlayersPath[PATH_MAX];
extern bool bSendingMap;			// used to indicate we are sending a map

bool						bHosted			= false;				//we have set up a game
char						sPlayer[128];							// player name (to be used)
static int					colourChooserUp = -1;
static int					teamChooserUp = -1;
static int					aiChooserUp = -1;
static int					difficultyChooserUp = -1;
static int					positionChooserUp = -1;
static bool				SettingsUp		= false;
static UBYTE				InitialProto	= 0;
static W_SCREEN				*psConScreen;
static SDWORD				dwSelectedGame	= 0;						//player[] and games[] indexes
static UDWORD				gameNumber;								// index to games icons
static bool					safeSearch		= false;				// allow auto game finding.
static bool disableLobbyRefresh = false;	// if we allow lobby to be refreshed or not.
static UDWORD hideTime = 0;
static bool EnablePasswordPrompt = false;	// if we need the password prompt
LOBBY_ERROR_TYPES LobbyError = ERROR_NOERROR;
static char tooltipbuffer[MaxGames][256] = {{'\0'}};
static bool toggleFilter = true;	// Used to show all games or only games that are of the same version
/// end of globals.
// ////////////////////////////////////////////////////////////////////////////
// Function protos

// widget functions
static bool addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid);
static void addBlueForm(UDWORD parent, UDWORD id, const char *txt, UDWORD x, UDWORD y, UDWORD w, UDWORD h);
static void drawReadyButton(UDWORD player);

// Drawing Functions
static void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void intDisplayFeBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayRemoteGame(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayPosition(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayDifficulty(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayMultiEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static Image getFrontHighlightImage(Image image);

// pUserData structures used by drawing functions
struct DisplayPlayerCache {
	std::string	fullMainText;	// the “full” main text (used for storing the full player name when displaying a player)
	WzText		wzMainText;		// the main text

	WzText		wzSubText;		// the sub text (used for players)
};
struct DisplayPositionCache {
	WzText wzPositionText;
};
struct DisplayAICache {
	WzText wzText;
};
struct DisplayDifficultyCache {
	WzText wzDifficultyText;
};
struct DisplayRemoteGameHeaderCache
{
	WzText wzHeaderText_GameName;
	WzText wzHeaderText_MapName;
	WzText wzHeaderText_Players;
	WzText wzHeaderText_Status;
};
struct DisplayRemoteGameCache
{
	WzText wzText_CurrentVsMaxNumPlayers;
	WidthLimitedWzText wzText_GameName;
	WidthLimitedWzText wzText_MapName;
	WidthLimitedWzText wzText_ModNames;
	WidthLimitedWzText wzText_VersionString;
};

static DisplayRemoteGameHeaderCache remoteGameListHeaderCache;

// find games
static void addGames();
static void removeGames();

// password form functions
static void hidePasswordForm();
static void showPasswordForm();

// Game option functions
static	void	addGameOptions();
static void addChatBox(bool preserveOldChat = false);
static void		addConsoleBox();
static	void	disableMultiButs();
static	void	processMultiopWidgets(UDWORD);
static	void	SendFireUp();

static	void	decideWRF();

static void		closeColourChooser();
static void		closeTeamChooser();
static void		closePositionChooser();
static void		closeAiChooser();
static void		closeDifficultyChooser();
static bool		SendColourRequest(UBYTE player, UBYTE col);
static bool		SendPositionRequest(UBYTE player, UBYTE chosenPlayer);
static bool		safeToUseColour(UDWORD player, UDWORD col);
static bool		changeReadyStatus(UBYTE player, bool bReady);
static	void stopJoining();
static int difficultyIcon(int difficulty);
// ////////////////////////////////////////////////////////////////////////////
// map previews..

static const char *difficultyList[] = { N_("Easy"), N_("Medium"), N_("Hard"), N_("Insane") };
static const int difficultyValue[] = { 1, 10, 15, 20 };
static struct
{
	bool scavengers;
	bool alliances;
	bool teams;
	bool power;
	bool difficulty;
	bool ai;
	bool position;
	bool bases;
} locked;

struct AIDATA
{
	AIDATA() : assigned(0) {}
	char name[MAX_LEN_AI_NAME];
	char slo[MAX_LEN_AI_NAME];
	char vlo[MAX_LEN_AI_NAME];
	char js[MAX_LEN_AI_NAME];
	char tip[255];
	int assigned;	///< How many AIs have we assigned of this type
};
static std::vector<AIDATA> aidata;

struct WzMultiButton : public W_BUTTON
{
	WzMultiButton(WIDGET *parent) : W_BUTTON(parent) {}

	void display(int xOffset, int yOffset);

	Image imNormal;
	Image imDown;
	unsigned doHighlight;
	unsigned tc;
};

const std::vector<WzString> getAINames()
{
	std::vector<WzString> l;
	for (const auto &i : aidata)
	{
		if (i.js[0] != '\0')
		{
			l.push_back(WzString::fromUtf8(i.js));
		}
	}
	return l;
}

const char *getAIName(int player)
{
	if (NetPlay.players[player].ai >= 0 && NetPlay.players[player].ai != AI_CUSTOM)
	{
		return aidata[NetPlay.players[player].ai].name;
	}
	else
	{
		return _("Commander");
	}
}

int getNextAIAssignment(const char *name)
{
	int ai = matchAIbyName(name);
	int match = aidata[ai].assigned;

	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (ai == NetPlay.players[i].ai)
		{
			if (match == 0)
			{
				aidata[ai].assigned++;
				debug(LOG_SAVE, "wzscript assigned to player %d", i);
				return i;	// found right player
			}
			match--;		// find next of this type
		}
	}
	return AI_NOT_FOUND;
}

void setupChallengeAIs()
{
	if(challengeActive)
	{
		WzConfig ini(sRequestResult, WzConfig::ReadOnly);
		for (unsigned i = 0; i < MAX_PLAYERS; ++i)
		{
			// set ai property for all players, so that getAIName() can name them accordingly
			ini.beginGroup("player_" + WzString::number(i));
			if (ini.contains("ai"))
			{
				WzString val = ini.value("ai").toWzString();
				ini.endGroup();
				if (val.compare("null") == 0)
				{
					continue; // no AI
				}

				// strip given path down to filename
				WzString filename(QFileInfo(QString::fromUtf8(val.toUtf8().c_str())).fileName().toUtf8().constData());

				// look up AI value in vector of known skirmish AIs
				for (int ai = 0; ai < aidata.size(); ++ai)
				{
					if (filename == aidata[ai].js || filename == aidata[ai].slo || filename == aidata[ai].vlo)
					{
						NetPlay.players[i].ai = ai;
						break;
					}
					else
					{
						NetPlay.players[i].ai = AI_CUSTOM; // for custom AIs
					}
				}
				continue;
			}
			ini.endGroup();
		}
	}
}

void loadMultiScripts()
{
	bool defaultRules = true;
	char aFileName[256];
	char aPathName[256];
	LEVEL_DATASET *psLevel = levFindDataSet(game.map, &game.hash);
	ASSERT_OR_RETURN(, psLevel, "No level found for %s", game.map);
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game]);
	aFileName[strlen(aFileName) - 4] = '\0';
	sstrcpy(aPathName, aFileName);
	sstrcat(aFileName, ".json");
	sstrcat(aPathName, "/");
	WzString ininame = challengeActive ? sRequestResult : aFileName;
	WzString path = challengeActive ? "challenges/" : aPathName;

	if (hostlaunch == 2)
	{
		ininame = "tests/" + WzString::fromUtf8(wz_skirmish_test());
		path = "tests/";
	}

	// Reset assigned counter
	for (auto it = aidata.begin(); it < aidata.end(); ++it)
	{
		(*it).assigned = 0;
	}

	// Load map scripts
	if (PHYSFS_exists(ininame.toUtf8().c_str()))
	{
		WzConfig ini(ininame, WzConfig::ReadOnly);
		debug(LOG_SAVE, "Loading map scripts");
		if (ini.beginGroup("scripts"))
		{
			if (ini.contains("extra"))
			{
				loadGlobalScript(path + ini.value("extra").toWzString());
			}
			if (ini.contains("rules"))
			{
				loadGlobalScript(path + ini.value("rules").toWzString());
				defaultRules = false;
			}
		}
		ini.endGroup();
	}

	// Load multiplayer rules
	resForceBaseDir("messages/");
	resLoadFile("SMSG", "multiplay.txt");
	if (defaultRules)
	{
		debug(LOG_SAVE, "Loading default rules");
		loadGlobalScript("multiplay/skirmish/rules.js");
	}

	// Backup data hashes, since AI and scavenger scripts aren't run on all clients.
	uint32_t oldHash1 = DataHash[DATA_SCRIPT];
	uint32_t oldHash2 = DataHash[DATA_SCRIPTVAL];

	// Load AI players
	resForceBaseDir("multiplay/skirmish/");
	for (unsigned i = 0; i < game.maxPlayers; i++)
	{
		if (NetPlay.players[i].ai < 0 && i == selectedPlayer)
		{
			NetPlay.players[i].ai = 0;  // For autogames.
		}
		// The i == selectedPlayer hack is to enable autogames
		if (bMultiPlayer && game.type == SKIRMISH && (!NetPlay.players[i].allocated || i == selectedPlayer)
		    && (NetPlay.players[i].ai >= 0 || hostlaunch == 2) && myResponsibility(i))
		{
			if (PHYSFS_exists(ininame.toUtf8().c_str())) // challenge file may override AI
			{
				WzConfig ini(ininame, WzConfig::ReadOnly);
				ini.beginGroup("player_" + WzString::number(i));
				if (ini.contains("ai"))
				{
					WzString val = ini.value("ai").toWzString();
					ini.endGroup();
					if (val.compare("null") == 0)
					{
						continue; // no AI
					}
					loadPlayerScript(val, i, NetPlay.players[i].difficulty);

					debug(LOG_WZ, "AI %s loaded for player %u", val.toUtf8().c_str(), i);
					continue;
				}
				ini.endGroup();
			}
			if (aidata[NetPlay.players[i].ai].slo[0] != '\0')
			{
				debug(LOG_SAVE, "Loading wzscript AI for player %d", i);
				resLoadFile("SCRIPT", aidata[NetPlay.players[i].ai].slo);
				resLoadFile("SCRIPTVAL", aidata[NetPlay.players[i].ai].vlo);
			}
			// autogames are to be implemented differently for qtscript, do not start for human players yet
			if (!NetPlay.players[i].allocated && aidata[NetPlay.players[i].ai].js[0] != '\0')
			{
				debug(LOG_SAVE, "Loading javascript AI for player %d", i);
				loadPlayerScript(WzString("multiplay/skirmish/") + aidata[NetPlay.players[i].ai].js, i, NetPlay.players[i].difficulty);
			}
		}
	}

	// Load scavengers
	if (game.scavengers && myResponsibility(scavengerPlayer()))
	{
		debug(LOG_SAVE, "Loading scavenger AI for player %d", scavengerPlayer());
		loadPlayerScript("multiplay/script/scavfact.js", scavengerPlayer(), DIFFICULTY_EASY);
	}

	// Restore data hashes, since AI and scavenger scripts aren't run on all clients.
	DataHash[DATA_SCRIPT]    = oldHash1;  // Not all players load the same AI scripts.
	DataHash[DATA_SCRIPTVAL] = oldHash2;

	// Reset resource path, otherwise things break down the line
	resForceBaseDir("");
}

static MAP_TILESET_TYPE guessMapTilesetType(LEVEL_DATASET *psLevel)
{
	unsigned t = 0, c = 0;

	if (psLevel->psBaseData && psLevel->psBaseData->pName)
	{
		if (sscanf(psLevel->psBaseData->pName, "MULTI_CAM_%u", &c) != 1)
		{
			sscanf(psLevel->psBaseData->pName, "MULTI_T%u_C%u", &t, &c);
		}
	}

	switch (c)
	{
	case 1:
		return TILESET_ARIZONA;
		break;
	case 2:
		return TILESET_URBAN;
		break;
	case 3:
		return TILESET_ROCKIES;
		break;
	}

	debug(LOG_MAP, "Could not guess map tileset, using ARIZONA.");
	return TILESET_ARIZONA;
}

static void loadEmptyMapPreview()
{
	// No map is available to preview, so improvise.
	char *imageData = (char *)malloc(BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3);
	memset(imageData, 0, BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3); //dunno about background color
	int const ex = 100, ey = 100, bx = 5, by = 8;
	for (unsigned n = 0; n < 125; ++n)
	{
		int sx = rand() % (ex - bx), sy = rand() % (ey - by);
		char col[3] = {char(rand() % 256), char(rand() % 256), char(rand() % 256)};
		for (unsigned y = 0; y < by; ++y)
			for (unsigned x = 0; x < bx; ++x)
				if (("\2\1\261\11\6"[x] >> y & 1) == 1) // ?
				{
					memcpy(imageData + 3 * (sx + x + BACKDROP_HACK_WIDTH * (sy + y)), col, 3);
				}
	}

	// Slight hack to init array with a special value used to determine how many players on map
	Vector2i playerpos[MAX_PLAYERS];
	for (size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		playerpos[i] = Vector2i(0x77777777, 0x77777777);
	}

	screen_enableMapPreview(ex, ey, playerpos);

	screen_Upload(imageData);
	free(imageData);
}

/// Loads the entire map just to show a picture of it
void loadMapPreview(bool hideInterface)
{
	static char		aFileName[256];
	UDWORD			fileSize;
	char			*pFileData = nullptr;
	LEVEL_DATASET	*psLevel = nullptr;
	PIELIGHT		plCliffL, plCliffH, plWater, plRoadL, plRoadH, plGroundL, plGroundH;
	UDWORD			x, y, height;
	UBYTE			col;
	MAPTILE			*psTile, *WTile;
	UDWORD oursize;
	Vector2i playerpos[MAX_PLAYERS];	// Will hold player positions
	char  *ptr = nullptr, *imageData = nullptr;

	// absurd hack, since there is a problem with updating this crap piece of info, we're setting it to
	// true by default for now, like it used to be
	game.mapHasScavengers = true; // this is really the wrong place for it, but this is where it has to be

	if (psMapTiles)
	{
		mapShutdown();
	}

	// load the terrain types
	psLevel = levFindDataSet(game.map, &game.hash);
	if (psLevel == nullptr)
	{
		debug(LOG_INFO, "Could not find level dataset \"%s\" %s. We %s waiting for a download.", game.map, game.hash.toString().c_str(), !NetPlay.wzFiles.empty() ? "are" : "aren't");
		loadEmptyMapPreview();
		return;
	}
	if (psLevel->realFileName == nullptr)
	{
		debug(LOG_WZ, "Loading map preview: \"%s\" builtin t%d", psLevel->pName, psLevel->dataDir);
	}
	else
	{
		debug(LOG_WZ, "Loading map preview: \"%s\" in (%s)\"%s\"  %s t%d", psLevel->pName, PHYSFS_getRealDir(psLevel->realFileName), psLevel->realFileName, psLevel->realFileHash.toString().c_str(), psLevel->dataDir);
	}
	rebuildSearchPath(psLevel->dataDir, false, psLevel->realFileName);
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game]);
	aFileName[strlen(aFileName) - 4] = '\0';
	sstrcat(aFileName, "/ttypes.ttp");
	pFileData = fileLoadBuffer;

	if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_ERROR, "Failed to load terrain types file: [%s]", aFileName);
		return;
	}
	if (!loadTerrainTypeMap(pFileData, fileSize))
	{
		debug(LOG_ERROR, "Failed to load terrain types");
		return;
	}

	// load the map data
	ptr = strrchr(aFileName, '/');
	ASSERT_OR_RETURN(, ptr, "this string was supposed to contain a /");
	strcpy(ptr, "/game.map");
	if (!mapLoad(aFileName, true))
	{
		debug(LOG_ERROR, "Failed to load map");
		return;
	}
	gwShutDown();

	// set tileset colors
	switch (guessMapTilesetType(psLevel))
	{
	case TILESET_ARIZONA:
		plCliffL = WZCOL_TERC1_CLIFF_LOW;
		plCliffH = WZCOL_TERC1_CLIFF_HIGH;
		plWater = WZCOL_TERC1_WATER;
		plRoadL = WZCOL_TERC1_ROAD_LOW;
		plRoadH = WZCOL_TERC1_ROAD_HIGH;
		plGroundL = WZCOL_TERC1_GROUND_LOW;
		plGroundH = WZCOL_TERC1_GROUND_HIGH;
		break;
	case TILESET_URBAN:
		plCliffL = WZCOL_TERC2_CLIFF_LOW;
		plCliffH = WZCOL_TERC2_CLIFF_HIGH;
		plWater = WZCOL_TERC2_WATER;
		plRoadL = WZCOL_TERC2_ROAD_LOW;
		plRoadH = WZCOL_TERC2_ROAD_HIGH;
		plGroundL = WZCOL_TERC2_GROUND_LOW;
		plGroundH = WZCOL_TERC2_GROUND_HIGH;
		break;
	case TILESET_ROCKIES:
		plCliffL = WZCOL_TERC3_CLIFF_LOW;
		plCliffH = WZCOL_TERC3_CLIFF_HIGH;
		plWater = WZCOL_TERC3_WATER;
		plRoadL = WZCOL_TERC3_ROAD_LOW;
		plRoadH = WZCOL_TERC3_ROAD_HIGH;
		plGroundL = WZCOL_TERC3_GROUND_LOW;
		plGroundH = WZCOL_TERC3_GROUND_HIGH;
		break;
	}

	oursize = sizeof(char) * BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT;
	imageData = (char *)malloc(oursize * 3);		// used for the texture
	if (!imageData)
	{
		debug(LOG_FATAL, "Out of memory for texture!");
		abort();	// should be a fatal error ?
		return;
	}
	ptr = imageData;
	memset(ptr, 0, sizeof(char) * BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3); //dunno about background color
	psTile = psMapTiles;

	for (y = 0; y < mapHeight; y++)
	{
		WTile = psTile;
		for (x = 0; x < mapWidth; x++)
		{
			char *const p = imageData + (3 * (y * BACKDROP_HACK_WIDTH + x));
			height = WTile->height / ELEVATION_SCALE;
			col = height;

			switch (terrainType(WTile))
			{
			case TER_CLIFFFACE:
				p[0] = plCliffL.byte.r + (plCliffH.byte.r - plCliffL.byte.r) * col / 256;
				p[1] = plCliffL.byte.g + (plCliffH.byte.g - plCliffL.byte.g) * col / 256;
				p[2] = plCliffL.byte.b + (plCliffH.byte.b - plCliffL.byte.b) * col / 256;
				break;
			case TER_WATER:
				p[0] = plWater.byte.r;
				p[1] = plWater.byte.g;
				p[2] = plWater.byte.b;
				break;
			case TER_ROAD:
				p[0] = plRoadL.byte.r + (plRoadH.byte.r - plRoadL.byte.r) * col / 256;
				p[1] = plRoadL.byte.g + (plRoadH.byte.g - plRoadL.byte.g) * col / 256;
				p[2] = plRoadL.byte.b + (plRoadH.byte.b - plRoadL.byte.b) * col / 256;
				break;
			default:
				p[0] = plGroundL.byte.r + (plGroundH.byte.r - plGroundL.byte.r) * col / 256;
				p[1] = plGroundL.byte.g + (plGroundH.byte.g - plGroundL.byte.g) * col / 256;
				p[2] = plGroundL.byte.b + (plGroundH.byte.b - plGroundL.byte.b) * col / 256;
				break;
			}
			WTile += 1;
		}
		psTile += mapWidth;
	}
	// Slight hack to init array with a special value used to determine how many players on map
	for (size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		playerpos[i] = Vector2i(0x77777777, 0x77777777);
	}
	// color our texture with clancolors @ correct position
	plotStructurePreview16(imageData, playerpos);

	screen_enableMapPreview(mapWidth, mapHeight, playerpos);

	screen_Upload(imageData);

	free(imageData);

	if (hideInterface)
	{
		hideTime = gameTime;
	}
	mapShutdown();
}

// ////////////////////////////////////////////////////////////////////////////
// helper func

int matchAIbyName(const char *name)
{
	int i = 0;

	if (name[0] == '\0')
	{
		return AI_CLOSED;
	}
	for (auto it = aidata.cbegin(); it < aidata.cend(); ++it, i++)
	{
		if (strncasecmp(name, (*it).name, MAX_LEN_AI_NAME) == 0)
		{
			return i;
		}
	}
	return AI_NOT_FOUND;
}

void readAIs()
{
	char basepath[PATH_MAX];
	const char *sSearchPath = "multiplay/skirmish/";
	char **i, **files;

	aidata.clear();

	sstrcpy(basepath, sSearchPath);
	files = PHYSFS_enumerateFiles(basepath);
	for (i = files; *i != nullptr; ++i)
	{
		char path[PATH_MAX];
		// See if this filename contains the extension we're looking for
		if (!strstr(*i, ".json"))
		{
			continue;
		}
		sstrcpy(path, basepath);
		sstrcat(path, *i);
		WzConfig aiconf(path, WzConfig::ReadOnly);
		AIDATA ai;
		aiconf.beginGroup("AI");
		sstrcpy(ai.name, aiconf.value("name", "error").toWzString().toUtf8().c_str());
		sstrcpy(ai.slo, aiconf.value("slo", "").toWzString().toUtf8().c_str());
		sstrcpy(ai.vlo, aiconf.value("vlo", "").toWzString().toUtf8().c_str());
		sstrcpy(ai.js, aiconf.value("js", "").toWzString().toUtf8().c_str());
		sstrcpy(ai.tip, aiconf.value("tip", "Click to choose this AI").toWzString().toUtf8().c_str());
		if (strcmp(ai.name, "Nullbot") == 0)
		{
			std::vector<AIDATA>::iterator it;
			it = aidata.begin();
			aidata.insert(it, ai);
		}
		else
		{
			aidata.push_back(ai);
		}
		aiconf.endGroup();
	}
	PHYSFS_freeList(files);
}

//sets sWRFILE form game.map
static void decideWRF()
{
	// try and load it from the maps directory first,
	sstrcpy(aLevelName, MultiCustomMapsPath);
	sstrcat(aLevelName, game.map);
	sstrcat(aLevelName, ".wrf");
	debug(LOG_WZ, "decideWRF: %s", aLevelName);
	//if the file exists in the downloaded maps dir then use that one instead.
	// FIXME: Try to incorporate this into physfs setup somehow for sane paths
	if (!PHYSFS_exists(aLevelName))
	{
		sstrcpy(aLevelName, game.map);		// doesn't exist, must be a predefined one.
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Connection Options Screen.

void multiOptionsScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	if (psConScreen == nullptr) return;
	psConScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

static bool OptionsInet()			//internet options
{
	psConScreen = new W_SCREEN;

	W_FORMINIT sFormInit;           //Connection Settings
	sFormInit.formID = 0;
	sFormInit.id = CON_SETTINGS;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(CON_SETTINGSX, CON_SETTINGSY, CON_SETTINGSWIDTH, CON_SETTINGSHEIGHT);
	});
	sFormInit.pDisplay = intDisplayFeBox;
	widgAddForm(psConScreen, &sFormInit);

	addMultiBut(psConScreen, CON_SETTINGS, CON_OK, CON_OKX, CON_OKY, MULTIOP_OKW, MULTIOP_OKH,
	            _("Accept Settings"), IMAGE_OK, IMAGE_OK, true);
	addMultiBut(psConScreen, CON_SETTINGS, CON_IP_CANCEL, CON_OKX + MULTIOP_OKW + 10, CON_OKY, MULTIOP_OKW, MULTIOP_OKH,
	            _("Cancel"), IMAGE_NO, IMAGE_NO, true);

	//label.
	W_LABINIT sLabInit;
	sLabInit.formID = CON_SETTINGS;
	sLabInit.id		= CON_SETTINGS_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 10;
	sLabInit.width	= CON_SETTINGSWIDTH;
	sLabInit.height = 20;
	sLabInit.pText	= WzString::fromUtf8(_("IP Address or Machine Name"));
	widgAddLabel(psConScreen, &sLabInit);


	W_EDBINIT sEdInit;             // address
	sEdInit.formID = CON_SETTINGS;
	sEdInit.id = CON_IP;
	sEdInit.x = CON_IPX;
	sEdInit.y = CON_IPY;
	sEdInit.width = CON_NAMEBOXWIDTH;
	sEdInit.height = CON_NAMEBOXHEIGHT;
	sEdInit.pText = "";									//_("IP Address or Machine Name");
	sEdInit.pBoxDisplay = intDisplayEditBox;
	if (!widgAddEditBox(psConScreen, &sEdInit))
	{
		return false;
	}
	// auto click in the text box
	W_CONTEXT sContext;
	sContext.xOffset	= 0;
	sContext.yOffset	= 0;
	sContext.mx			= 0;
	sContext.my			= 0;
	widgGetFromID(psConScreen, CON_IP)->clicked(&sContext);

	SettingsUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Draw the connections screen.
bool startConnectionScreen()
{
	addBackdrop();										//background
	addTopForm();										// logo
	addBottomForm();

	SettingsUp		= false;
	InitialProto	= 0;
	safeSearch		= false;

	// don't pretend we are running a network game. Really do it!
	NetPlay.bComms = true; // use network = true

	addSideText(FRONTEND_SIDETEXT,  FRONTEND_SIDEX, FRONTEND_SIDEY, _("CONNECTION"));

	addMultiBut(psWScreen, FRONTEND_BOTFORM, CON_CANCEL, 10, 10, MULTIOP_OKW, MULTIOP_OKH,
	            _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);	// goback buttpn levels

	addTextButton(CON_TYPESID_START + 0, FRONTEND_POS2X, FRONTEND_POS2Y, _("Lobby"), WBUT_TXTCENTRE);
	addTextButton(CON_TYPESID_START + 1, FRONTEND_POS3X, FRONTEND_POS3Y, _("IP"), WBUT_TXTCENTRE);

	return true;
}

void runConnectionScreen()
{
	static char addr[128];

	W_SCREEN *curScreen = SettingsUp ? psConScreen : psWScreen;
	WidgetTriggers const &triggers = widgRunScreen(curScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case CON_CANCEL: //cancel
		changeTitleMode(MULTI);
		bMultiPlayer = false;
		bMultiMessages = false;
		break;
	case CON_TYPESID_START+0: // Lobby button
		if (LobbyError != ERROR_INVALID)
		{
			setLobbyError(ERROR_NOERROR);
		}
		changeTitleMode(GAMEFIND);
		break;
	case CON_TYPESID_START+1: // IP button
		OptionsInet();
		break;
	case CON_OK:
		sstrcpy(addr, widgGetString(psConScreen, CON_IP));
		if (addr[0] == '\0')
		{
			sstrcpy(addr, "127.0.0.1");  // Default to localhost.
		}

		if (SettingsUp == true)
		{
			delete psConScreen;
			psConScreen = nullptr;
			SettingsUp = false;
		}

		joinGame(addr, 0);
		break;
	case CON_IP_CANCEL:
		if (SettingsUp == true)
		{
			delete psConScreen;
			psConScreen = nullptr;
			SettingsUp = false;
		}
		break;
	}

	widgDisplayScreen(psWScreen);							// show the widgets currently running
	if (SettingsUp == true)
	{
		widgDisplayScreen(psConScreen);						// show the widgets currently running
	}

	if (CancelPressed())
	{
		changeTitleMode(MULTI);
	}
}

// ////////////////////////////////////////////////////////////////////////
// Lobby error reading
LOBBY_ERROR_TYPES getLobbyError()
{
	return LobbyError;
}

void setLobbyError(LOBBY_ERROR_TYPES error_type)
{
	LobbyError = error_type;
	if (LobbyError <= ERROR_FULL)
	{
		disableLobbyRefresh = false;
	}
	else
	{
		disableLobbyRefresh = true;
	}
}

/**
 * Try connecting to the given host, show
 * the multiplayer screen or a error.
 */
bool joinGame(const char *host, uint32_t port)
{
	PLAYERSTATS	playerStats;

	if (ingame.localJoiningInProgress)
	{
		return false;
	}

	if (!NETjoinGame(host, port, (char *)sPlayer))	// join
	{
		switch (getLobbyError())
		{
		case ERROR_HOSTDROPPED:
			setLobbyError(ERROR_NOERROR);
			break;
		case ERROR_WRONGPASSWORD:
			{
				// Change to GAMEFIND, screen with a password dialog.
				changeTitleMode(GAMEFIND);
				showPasswordForm();
				WidgetTriggers const &triggers = widgRunScreen(psWScreen);
				unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

				if (id != CON_PASSWORD) // Run the current set of widgets
				{
					return false;
				}
				NETsetGamePassword(widgGetString(psWScreen, CON_PASSWORD));
				break;
			}
		default:
			break;
		}

		// Hide a (maybe shown) password form.
		hidePasswordForm();

		// Change to GAMEFIND, screen.
		changeTitleMode(GAMEFIND);

		// Shows the lobby error.
		addConsoleBox();
		addGames();
		updateConsoleMessages();
		displayConsoleMessages();
		return false;
	}
	ingame.localJoiningInProgress	= true;

	loadMultiStats(sPlayer, &playerStats);
	setMultiStats(selectedPlayer, playerStats, false);
	setMultiStats(selectedPlayer, playerStats, true);

	changeTitleMode(MULTIOPTION);

	if (war_getMPcolour() >= 0)
	{
		SendColourRequest(selectedPlayer, war_getMPcolour());
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Game Chooser Screen.

static void addGames()
{
	int i, gcount = 0, added = 0;
	static const char *wrongVersionTip = _("Your version of Warzone is incompatible with this game.");

	//count games to see if need two columns.
	for (i = 0; i < MaxGames; i++)
	{
		if (NetPlay.games[i].desc.dwSize != 0)
		{
			gcount++;
		}
	}

	W_BUTINIT sButInit;
	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.width = GAMES_GAMEWIDTH;
	sButInit.height = GAMES_GAMEHEIGHT;
	sButInit.pDisplay = displayRemoteGame;
	sButInit.initPUserDataFunc = []() -> void * { return new DisplayRemoteGameCache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayRemoteGameCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	// we want the old games deleted, and only list games when we should
	if (getLobbyError() || !gcount)
	{
		for (i = 0; i < MaxGames; i++)
		{
			widgDelete(psWScreen, GAMES_GAMESTART + i);	// remove old widget
		}
		gcount = 0;
	}
	// in case they refresh, and a game becomes available.
	widgDelete(psWScreen, FRONTEND_NOGAMESAVAILABLE);

	// only have to do this if we have any games available.
	if (!getLobbyError() && gcount)
	{
		for (i = 0; i < MaxGames; i++)							// draw games
		{
			widgDelete(psWScreen, GAMES_GAMESTART + i);	// remove old icon.

			if (NetPlay.games[i].desc.dwSize != 0)
			{
				// either display all games, or games that are the client's specific version
				if (toggleFilter)
				{
					if ((NetPlay.games[i].game_version_major != (unsigned)NETGetMajorVersion()) || (NetPlay.games[i].game_version_minor != (unsigned)NETGetMinorVersion()))
					{
						continue;
					}
				}
				added++;
				sButInit.id = GAMES_GAMESTART + i;
				sButInit.x = 20;
				sButInit.y = (UWORD)(45 + ((5 + GAMES_GAMEHEIGHT) * i));

				// display the correct tooltip message.
				if (!NETisCorrectVersion(NetPlay.games[i].game_version_major, NetPlay.games[i].game_version_minor))
				{
					sButInit.pTip = wrongVersionTip;
				}
				else
				{
					WzString flags;
					if (NetPlay.games[i].privateGame)
					{
						flags += " "; flags += _("[Password required]");
					}
					if (NetPlay.games[i].limits & NO_TANKS)
					{
						flags += " "; flags += _("[No Tanks]");
					}
					if (NetPlay.games[i].limits & NO_BORGS)
					{
						flags += " "; flags += _("[No Cyborgs]");
					}
					if (NetPlay.games[i].limits & NO_VTOLS)
					{
						flags += " "; flags += _("[No VTOLs]");
					}
					if (flags.isEmpty())
					{
						ssprintf(tooltipbuffer[i], _("Hosted by %s"), NetPlay.games[i].hostname);
					}
					else
					{
						ssprintf(tooltipbuffer[i], _("Hosted by %s —%s"), NetPlay.games[i].hostname, flags.toUtf8().c_str());
					}
					sButInit.pTip = tooltipbuffer[i];
				}
				sButInit.UserData = i;

				widgAddButton(psWScreen, &sButInit);
			}
		}
		if (!added)
		{
			sButInit = W_BUTINIT();
			sButInit.formID = FRONTEND_BOTFORM;
			sButInit.id = FRONTEND_NOGAMESAVAILABLE;
			sButInit.x = 70;
			sButInit.y = 378;
			sButInit.style = WBUT_TXTCENTRE;
			sButInit.width = FRONTEND_BUTWIDTH;
			sButInit.UserData = 0; // store disable state
			sButInit.height = FRONTEND_BUTHEIGHT;
			sButInit.pDisplay = displayTextOption;
			sButInit.pUserData = new DisplayTextOptionCache();
			sButInit.onDelete = [](WIDGET *psWidget) {
				assert(psWidget->pUserData != nullptr);
				delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
				psWidget->pUserData = nullptr;
			};
			sButInit.FontID = font_large;
			sButInit.pText = _("Can't find any games for your version.");

			widgAddButton(psWScreen, &sButInit);
		}
	}
	else
	{
		// display lobby message based on results.
		// This is a 'button', not text so it can be hilighted/centered.
		const char *txt;
		W_BUTINIT sButInit;

		switch (getLobbyError())
		{
		case ERROR_NOERROR:
			if (NetPlay.HaveUpgrade)
			{
				txt = _("There appears to be a game update available!");
			}
			else
			{
				txt = _("No games are available for your version");
			}
			break;
		case ERROR_FULL:
			txt = _("Game is full");
			break;
		case ERROR_KICKED:
		case ERROR_INVALID:
			txt = _("You were kicked!");
			break;
		case ERROR_WRONGVERSION:
			txt = _("Wrong Game Version!");
			break;
		case ERROR_WRONGDATA:
			txt = _("You have an incompatible mod.");
			break;
		// AFAIK, the only way this can really happy is if the Host's file is named wrong, or a client side error.
		case ERROR_UNKNOWNFILEISSUE:
			txt = _("Host couldn't send file?");
			debug(LOG_POPUP, "Warzone couldn't complete a file request.\n\nPossibly, Host's file is incorrect. Check your logs for more details.");
			break;
		case ERROR_WRONGPASSWORD:
			txt = _("Incorrect Password!");
			break;
		case ERROR_HOSTDROPPED:
			txt = _("Host has dropped connection!");
			break;
		case ERROR_CONNECTION:
		default:
			txt = _("Connection Error");
			break;
		}

		// delete old widget if necessary
		widgDelete(psWScreen, FRONTEND_NOGAMESAVAILABLE);

		sButInit = W_BUTINIT();
		sButInit.formID = FRONTEND_BOTFORM;
		sButInit.id = FRONTEND_NOGAMESAVAILABLE;
		sButInit.x = 70;
		sButInit.y = 50;
		sButInit.style = WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
		sButInit.UserData = 0; // store disable state
		sButInit.height = FRONTEND_BUTHEIGHT;
		sButInit.pDisplay = displayTextOption;
		sButInit.pUserData = new DisplayTextOptionCache();
		sButInit.onDelete = [](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		};
		sButInit.FontID = font_medium;
		sButInit.pText = txt;

		widgAddButton(psWScreen, &sButInit);
	}
	if (strlen(NetPlay.MOTD))
	{
		permitNewConsoleMessages(true);
		addConsoleMessage(NetPlay.MOTD, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	setConsolePermanence(false, false);
	updateConsoleMessages();
	displayConsoleMessages();
}

static void removeGames()
{
	int i;
	for (i = 0; i < MaxGames; i++)
	{
		widgDelete(psWScreen, GAMES_GAMESTART + i);	// remove old widget
	}
	widgDelete(psWScreen, FRONTEND_NOGAMESAVAILABLE);
}

void runGameFind()
{
	static UDWORD lastupdate = 0;
	static char game_password[StringSize];

	if (lastupdate > gameTime)
	{
		lastupdate = 0;
	}
	if (gameTime - lastupdate > 6000 && !EnablePasswordPrompt)
	{
		lastupdate = gameTime;
		addConsoleBox();
		if (safeSearch)
		{
			if (!NETfindGame())						// find games synchronously
			{
				pie_LoadBackDrop(SCREEN_RANDOMBDROP);
			}
		}
		addGames();									//redraw list
	}

	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	if (id == CON_CANCEL)								// ok
	{
		changeTitleMode(PROTOCOL);
	}

	if (id == MULTIOP_REFRESH || id == MULTIOP_FILTER_TOGGLE)
	{
		if (id == MULTIOP_FILTER_TOGGLE)
		{
			toggleFilter = !toggleFilter;
			toggleFilter ? widgSetButtonState(psWScreen, MULTIOP_FILTER_TOGGLE, WBUT_CLICKLOCK) : widgSetButtonState(psWScreen, MULTIOP_FILTER_TOGGLE, 0);
		}
		else
		{
			widgSetButtonState(psWScreen, MULTIOP_FILTER_TOGGLE, 0);
		}
		ingame.localOptionsReceived = true;
		addConsoleBox();
		if (!NETfindGame())							// find games synchronously
		{
			pie_LoadBackDrop(SCREEN_RANDOMBDROP);
		}
		addConsoleMessage(_("Refreshing..."), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		addGames();									//redraw list.
	}
	if (id == CON_PASSWORD)
	{
		sstrcpy(game_password, widgGetString(psWScreen, CON_PASSWORD));
		NETsetGamePassword(game_password);
	}

	// below is when they hit a game box to connect to--ideally this would be where
	// we would want a modal password entry box.
	if (id >= GAMES_GAMESTART && id <= GAMES_GAMEEND)
	{
		gameNumber = id - GAMES_GAMESTART;

		if (NetPlay.games[gameNumber].privateGame)
		{
			showPasswordForm();
		}
		else
		{
			ingame.localOptionsReceived = false;					// note, we are awaiting options
			sstrcpy(game.name, NetPlay.games[gameNumber].name);		// store name
			joinGame(NetPlay.games[gameNumber].desc.host, 0);
		}
	}
	else if (id == CON_PASSWORDYES)
	{
		ingame.localOptionsReceived = false;					// note, we are awaiting options
		sstrcpy(game.name, NetPlay.games[gameNumber].name);		// store name
		joinGame(NetPlay.games[gameNumber].desc.host, 0);
	}
	else if (id == CON_PASSWORDNO)
	{
		hidePasswordForm();
	}

	widgDisplayScreen(psWScreen);								// show the widgets currently running
	if (safeSearch)
	{
		iV_DrawText(_("Searching"), D_W + 260, D_H + 460, font_large);
	}

	if (CancelPressed())
	{
		changeTitleMode(PROTOCOL);
	}

	// console box handling
	if (widgGetFromID(psWScreen, MULTIOP_CONSOLEBOX))
	{
		while (getNumberConsoleMessages() > getConsoleLineInfo())
		{
			removeTopConsoleMessage();
		}
		updateConsoleMessages();
	}
	displayConsoleMessages();
}

// This is what starts the lobby screen
void startGameFind()
{
	addBackdrop();										//background image

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	// draws the background of the games listed
	IntFormAnimated *botForm = new IntFormAnimated(parent);
	botForm->id = FRONTEND_BOTFORM;
	botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_OPTIONSX, MULTIOP_OPTIONSY, MULTIOP_CHATBOXW, 415);  // FIXME: Add box at bottom for server messages
	}));

	addSideText(FRONTEND_SIDETEXT,  MULTIOP_OPTIONSX - 3, MULTIOP_OPTIONSY, _("GAMES"));

	// cancel
	addMultiBut(psWScreen, FRONTEND_BOTFORM, CON_CANCEL, 10, 5, MULTIOP_OKW, MULTIOP_OKH, _("Return To Previous Screen"),
	            IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	//refresh
	addMultiBut(psWScreen, FRONTEND_BOTFORM, MULTIOP_REFRESH, MULTIOP_CHATBOXW - MULTIOP_OKW - 5, 5, MULTIOP_OKW, MULTIOP_OKH,
	            _("Refresh Games List"), IMAGE_RELOAD_HI, IMAGE_RELOAD, IMAGE_RELOAD);
	//filter toggle
	addMultiBut(psWScreen, FRONTEND_BOTFORM, MULTIOP_FILTER_TOGGLE, MULTIOP_CHATBOXW - MULTIOP_OKW - 45, 5, MULTIOP_OKW, MULTIOP_OKH,
	            _("Filter Games List"), IMAGE_FILTER, IMAGE_FILTER_ON, IMAGE_FILTER_ON);

	if (safeSearch || disableLobbyRefresh)
	{
		widgHide(psWScreen, MULTIOP_REFRESH);
		widgHide(psWScreen, MULTIOP_FILTER_TOGGLE);
	}

	addConsoleBox();
	if (!NETfindGame())
	{
		pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	}

	addGames();	// now add games.
	displayConsoleMessages();

	// Password stuff. Hidden by default.

	// draws the background of the password box
	IntFormAnimated *passwordForm = new IntFormAnimated(parent);
	passwordForm->id = FRONTEND_PASSWORDFORM;
	passwordForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(FRONTEND_BOTFORMX, 160, FRONTEND_TOPFORMW, FRONTEND_TOPFORMH - 40);
	}));

	// password label.
	W_LABEL *enterPasswordLabel = new W_LABEL(passwordForm);
	enterPasswordLabel->setTextAlignment(WLAB_ALIGNCENTRE);
	enterPasswordLabel->setGeometry(130, 0, 280, 40);
	enterPasswordLabel->setFont(font_large, WZCOL_FORM_TEXT);
	enterPasswordLabel->setString(_("Enter Password:"));

	// and finally draw the password entry box
	W_EDITBOX *passwordBox = new W_EDITBOX(passwordForm);
	passwordBox->id = CON_PASSWORD;
	passwordBox->setGeometry(130, 40, 280, 20);
	passwordBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);

	W_BUTTON *buttonYes = new W_BUTTON(passwordForm);
	buttonYes->id = CON_PASSWORDYES;
	buttonYes->setImages(Image(FrontImages, IMAGE_OK), Image(FrontImages, IMAGE_OK), getFrontHighlightImage(Image(FrontImages, IMAGE_OK)));
	buttonYes->move(180, 65);
	buttonYes->setTip(_("OK"));
	W_BUTTON *buttonNo = new W_BUTTON(passwordForm);
	buttonNo->id = CON_PASSWORDNO;
	buttonNo->setImages(Image(FrontImages, IMAGE_NO), Image(FrontImages, IMAGE_NO), getFrontHighlightImage(Image(FrontImages, IMAGE_NO)));
	buttonNo->move(230, 65);
	buttonNo->setTip(_("Cancel"));

	passwordForm->hide();

	EnablePasswordPrompt = false;
}

static void hidePasswordForm()
{
	EnablePasswordPrompt = false;

	if (widgGetFromID(psWScreen, FRONTEND_PASSWORDFORM))
	{
		widgHide(psWScreen, FRONTEND_PASSWORDFORM);
	}

	widgReveal(psWScreen, FRONTEND_SIDETEXT);
	widgReveal(psWScreen, FRONTEND_BOTFORM);
	widgReveal(psWScreen, CON_CANCEL);
	if (!safeSearch && (!disableLobbyRefresh))
	{
		if (widgGetFromID(psWScreen, MULTIOP_REFRESH))
		{
			widgReveal(psWScreen, MULTIOP_REFRESH);
		}
		if (widgGetFromID(psWScreen, MULTIOP_FILTER_TOGGLE))
		{
			widgReveal(psWScreen, MULTIOP_FILTER_TOGGLE);
		}
	}
	addConsoleBox();
	addGames();
}

static void showPasswordForm()
{
	W_CONTEXT sContext;
	EnablePasswordPrompt = true;

	widgHide(psWScreen, FRONTEND_SIDETEXT);
	widgHide(psWScreen, FRONTEND_BOTFORM);
	widgHide(psWScreen, CON_CANCEL);
	widgHide(psWScreen, MULTIOP_REFRESH);
	widgHide(psWScreen, MULTIOP_FILTER_TOGGLE);

	removeGames();

	widgReveal(psWScreen, FRONTEND_PASSWORDFORM);

	// auto click in the password box
	sContext.xOffset	= 0;
	sContext.yOffset	= 0;
	sContext.mx			= 0;
	sContext.my			= 0;
	widgGetFromID(psWScreen, CON_PASSWORD)->clicked(&sContext);
}


// ////////////////////////////////////////////////////////////////////////////
// Game Options Screen.

// ////////////////////////////////////////////////////////////////////////////

MultibuttonWidget::MultibuttonWidget(WIDGET *parent, int value)
	: W_FORM(parent)
	, label(nullptr)
	, currentValue_(value)
	, disabled(false)
	, gap_(3)
	, lockCurrent(false)
{
}

void MultibuttonWidget::display(int xOffset, int yOffset)
{
	iV_ShadowBox(xOffset + x(), yOffset + y(), xOffset + x() + width() - 1, yOffset + y() + height() - 1, 0, WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
}

void MultibuttonWidget::geometryChanged()
{
	int s = width() - gap_;
	for (std::vector<std::pair<W_BUTTON *, int> >::const_reverse_iterator i = buttons.rbegin(); i != buttons.rend(); ++i)
	{
		i->first->move(s - i->first->width(), (height() - i->first->height()) / 2);
		s -= i->first->width() + gap_;
	}
	if (label != nullptr)
	{
		label->setGeometry(gap_, 0, s - gap_, height());
	}
}

void MultibuttonWidget::setLabel(char const *text)
{
	delete label;
	label = new W_LABEL(this);
	label->setString(text);

	geometryChanged();
}

void MultibuttonWidget::addButton(int value, Image image, Image imageDown, char const *tip)
{
	W_BUTTON *button = new W_BUTTON(this);
	button->setImages(image, imageDown, getFrontHighlightImage(image));
	button->setTip(tip);
	button->setState(value == currentValue_ && lockCurrent ? WBUT_LOCK : disabled ? WBUT_DISABLE : 0);
	buttons.push_back(std::make_pair(button, value));

	button->addOnClickHandler([value](W_BUTTON& button) {
		MultibuttonWidget* pParent = static_cast<MultibuttonWidget*>(button.parent());
		assert(pParent != nullptr);
		pParent->choose(value);
	});

	geometryChanged();
}

void MultibuttonWidget::enable(bool enabled)
{
	if (!enabled == disabled)
	{
		return;
	}

	disabled = !enabled;
	stateChanged();
}

void MultibuttonWidget::setGap(int gap)
{
	if (gap == gap_)
	{
		return;
	}

	gap_ = gap;
	geometryChanged();
}

void MultibuttonWidget::choose(int value)
{
	if (value == currentValue_ && lockCurrent)
	{
		return;
	}

	currentValue_ = value;
	stateChanged();

	/* Call all onChoose event handlers */
	for (auto it = onChooseHandlers.begin(); it != onChooseHandlers.end(); it++)
	{
		auto onChoose = *it;
		if (onChoose)
		{
			onChoose(*this, currentValue_);
		}
	}

	screenPointer->setReturn(this);
}

void MultibuttonWidget::stateChanged()
{
	for (std::vector<std::pair<W_BUTTON *, int> >::const_iterator i = buttons.begin(); i != buttons.end(); ++i)
	{
		i->first->setState(i->second == currentValue_ && lockCurrent ? WBUT_LOCK : disabled ? WBUT_DISABLE : 0);
	}
}

MultichoiceWidget::MultichoiceWidget(WIDGET *parent, int value)
	: MultibuttonWidget(parent, value)
{
	lockCurrent = true;
}

static void addBlueForm(UDWORD parent, UDWORD id, const char *txt, UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	W_FORMINIT sFormInit;                  // draw options box.
	sFormInit.formID = parent;
	sFormInit.id	= id;
	sFormInit.x		= (UWORD) x;
	sFormInit.y		= (UWORD) y;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = (UWORD)w;//190;
	sFormInit.height = (UWORD)h; //27;
	sFormInit.pDisplay =  intDisplayFeBox;
	widgAddForm(psWScreen, &sFormInit);

	if (strlen(txt) > 0)
	{
		W_LABINIT sLabInit;
		sLabInit.formID = id;
		sLabInit.id		= id + 1;
		sLabInit.x		= 3;
		sLabInit.y		= 4;
		sLabInit.width	= 80;
		sLabInit.height = 20;
		sLabInit.pText	= WzString::fromUtf8(txt);
		widgAddLabel(psWScreen, &sLabInit);
	}
	return;
}


struct LimitIcon
{
	char const *stat;
	char const *desc;
	int         icon;
};
static const LimitIcon limitIcons[] =
{
	{"A0LightFactory",  N_("Tanks disabled!!"),  IMAGE_NO_TANK},
	{"A0CyborgFactory", N_("Cyborgs disabled."), IMAGE_NO_CYBORG},
	{"A0VTolFactory1",  N_("VTOLs disabled."),   IMAGE_NO_VTOL},
	{"A0Sat-linkCentre", N_("Satellite Uplink disabled."), IMAGE_NO_UPLINK},
	{"A0LasSatCommand",  N_("Laser Satellite disabled."),  IMAGE_NO_LASSAT},
};

void updateLimitFlags()
{
	unsigned i;
	unsigned flags = 0;

	if (!ingame.bHostSetup)
	{
		return;  // The host works out the flags.
	}

	for (i = 0; i < ARRAY_SIZE(limitIcons); ++i)
	{
		int stat = getStructStatFromName(limitIcons[i].stat);
		bool disabled = stat >= 0 && asStructureStats[stat].upgrade[0].limit == 0;
		flags |= disabled << i;
	}

	ingame.flags = flags;
}

// need to check for side effects.
static void addGameOptions()
{
	widgDelete(psWScreen, MULTIOP_OPTIONS);  				// clear options list
	widgDelete(psWScreen, FRONTEND_SIDETEXT3);				// del text..

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	// draw options box.
	IntFormAnimated *optionsForm = new IntFormAnimated(parent, false);
	optionsForm->id = MULTIOP_OPTIONS;
	optionsForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_OPTIONSX, MULTIOP_OPTIONSY, MULTIOP_OPTIONSW, MULTIOP_OPTIONSH);
	}));

	addSideText(FRONTEND_SIDETEXT3, MULTIOP_OPTIONSX - 3 , MULTIOP_OPTIONSY, _("OPTIONS"));

	// game name box
	if (!NetPlay.bComms)
	{
		addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_GNAME, MCOL0, MROW2, _("Game Name"),
		                challengeActive ? game.name : _("One-Player Skirmish"), IMAGE_EDIT_GAME,
		                IMAGE_EDIT_GAME_HI, MULTIOP_GNAME_ICON);
		// disable for one-player skirmish
		widgSetButtonState(psWScreen, MULTIOP_GNAME, WEDBS_DISABLE);
	}
	else
	{
		addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_GNAME, MCOL0, MROW2, _("Select Game Name"), game.name, IMAGE_EDIT_GAME, IMAGE_EDIT_GAME_HI, MULTIOP_GNAME_ICON);
	}
	widgSetButtonState(psWScreen, MULTIOP_GNAME_ICON, WBUT_DISABLE);

	// map chooser

	addBlueForm(MULTIOP_OPTIONS, MULTIOP_MAP, game.map, MCOL0, MROW3, MULTIOP_BLUEFORMW, 29);
	addMultiBut(psWScreen, MULTIOP_MAP, MULTIOP_MAP_ICON, MCOL4, 2, 20, MULTIOP_BUTH, _("Select Map"), IMAGE_EDIT_MAP, IMAGE_EDIT_MAP_HI, true);
	addMultiBut(psWScreen, MULTIOP_MAP, MULTIOP_MAP_MOD, MCOL3 + 11, 10, 12, 12, _("Map-Mod!"), IMAGE_LAMP_RED, IMAGE_LAMP_AMBER, false);
	if (!game.isMapMod)
	{
		widgHide(psWScreen, MULTIOP_MAP_MOD);
	}
	// disable for challenges
	if (challengeActive)
	{
		widgSetButtonState(psWScreen, MULTIOP_MAP_ICON, WBUT_DISABLE);
	}
	// password box
	if (ingame.bHostSetup && NetPlay.bComms)
	{
		addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_PASSWORD_EDIT, MCOL0, MROW4, _("Click to set Password"), NetPlay.gamePassword, IMAGE_UNLOCK_BLUE, IMAGE_LOCK_BLUE, MULTIOP_PASSWORD_BUT);
		if (NetPlay.GamePassworded)
		{
			widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT, WBUT_CLICKLOCK);
			widgSetButtonState(psWScreen, MULTIOP_PASSWORD_EDIT, WEDBS_DISABLE);
		}
	}

	//just display the game options.
	addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_PNAME, MCOL0, MROW1, _("Select Player Name"), (char *) sPlayer, IMAGE_EDIT_PLAYER, IMAGE_EDIT_PLAYER_HI, MULTIOP_PNAME_ICON);

	ListWidget *optionsList = new ListWidget(optionsForm);
	optionsList->setChildSize(MULTIOP_BLUEFORMW, 29);
	optionsList->setChildSpacing(2, 2);
	optionsList->setGeometry(MCOL0, MROW5, MULTIOP_BLUEFORMW, optionsForm->height() - MROW5);

	MultichoiceWidget *scavengerChoice = new MultichoiceWidget(optionsList, game.scavengers);
	scavengerChoice->id = MULTIOP_GAMETYPE;
	scavengerChoice->setLabel(_("Scavengers"));
	if (game.mapHasScavengers)
	{
		scavengerChoice->addButton(true, Image(FrontImages, IMAGE_SCAVENGERS_ON), Image(FrontImages, IMAGE_SCAVENGERS_ON_HI), _("Scavengers"));
	}
	scavengerChoice->addButton(false, Image(FrontImages, IMAGE_SCAVENGERS_OFF), Image(FrontImages, IMAGE_SCAVENGERS_OFF_HI), _("No Scavengers"));
	scavengerChoice->enable(!locked.scavengers);
	optionsList->addWidgetToLayout(scavengerChoice);

	MultichoiceWidget *allianceChoice = new MultichoiceWidget(optionsList, game.alliance);
	allianceChoice->id = MULTIOP_ALLIANCES;
	allianceChoice->setLabel(_("Alliances"));
	allianceChoice->addButton(NO_ALLIANCES, Image(FrontImages, IMAGE_NOALLI), Image(FrontImages, IMAGE_NOALLI_HI), _("No Alliances"));
	allianceChoice->addButton(ALLIANCES, Image(FrontImages, IMAGE_ALLI), Image(FrontImages, IMAGE_ALLI_HI), _("Allow Alliances"));
	allianceChoice->addButton(ALLIANCES_UNSHARED, Image(FrontImages, IMAGE_ALLI_UNSHARED), Image(FrontImages, IMAGE_ALLI_UNSHARED_HI), _("Locked Teams, No Shared Research"));
	allianceChoice->addButton(ALLIANCES_TEAMS, Image(FrontImages, IMAGE_ALLI_TEAMS), Image(FrontImages, IMAGE_ALLI_TEAMS_HI), _("Locked Teams"));
	allianceChoice->enable(!locked.alliances);
	optionsList->addWidgetToLayout(allianceChoice);

	MultichoiceWidget *powerChoice = new MultichoiceWidget(optionsList, game.power);
	powerChoice->id = MULTIOP_POWER;
	powerChoice->setLabel(_("Power"));
	powerChoice->addButton(LEV_LOW, Image(FrontImages, IMAGE_POWLO), Image(FrontImages, IMAGE_POWLO_HI), _("Low Power Levels"));
	powerChoice->addButton(LEV_MED, Image(FrontImages, IMAGE_POWMED), Image(FrontImages, IMAGE_POWMED_HI), _("Medium Power Levels"));
	powerChoice->addButton(LEV_HI, Image(FrontImages, IMAGE_POWHI), Image(FrontImages, IMAGE_POWHI_HI), _("High Power Levels"));
	powerChoice->enable(!locked.power);
	optionsList->addWidgetToLayout(powerChoice);

	MultichoiceWidget *baseTypeChoice = new MultichoiceWidget(optionsList, game.base);
	baseTypeChoice->id = MULTIOP_BASETYPE;
	baseTypeChoice->setLabel(_("Base"));
	baseTypeChoice->addButton(CAMP_CLEAN, Image(FrontImages, IMAGE_NOBASE), Image(FrontImages, IMAGE_NOBASE_HI), _("Start with No Bases"));
	baseTypeChoice->addButton(CAMP_BASE, Image(FrontImages, IMAGE_SBASE), Image(FrontImages, IMAGE_SBASE_HI), _("Start with Bases"));
	baseTypeChoice->addButton(CAMP_WALLS, Image(FrontImages, IMAGE_LBASE), Image(FrontImages, IMAGE_LBASE_HI), _("Start with Advanced Bases"));
	baseTypeChoice->enable(!locked.bases);
	optionsList->addWidgetToLayout(baseTypeChoice);

	MultibuttonWidget *mapPreviewButton = new MultibuttonWidget(optionsList);
	mapPreviewButton->id = MULTIOP_MAP_PREVIEW;
	mapPreviewButton->setLabel(_("Map Preview"));
	mapPreviewButton->addButton(0, Image(FrontImages, IMAGE_FOG_OFF), Image(FrontImages, IMAGE_FOG_OFF_HI), _("Click to see Map"));
	optionsList->addWidgetToLayout(mapPreviewButton);

	if (ingame.bHostSetup)
	{
		MultibuttonWidget *structLimitsButton = new MultibuttonWidget(optionsList);
		structLimitsButton->id = MULTIOP_STRUCTLIMITS;
		structLimitsButton->setLabel(challengeActive ? _("Show Structure Limits") : _("Set Structure Limits"));
		structLimitsButton->addButton(0, Image(FrontImages, IMAGE_SLIM), Image(FrontImages, IMAGE_SLIM_HI), challengeActive ? _("Show Structure Limits") : _("Set Structure Limits"));
		optionsList->addWidgetToLayout(structLimitsButton);
	}

	if (ingame.bHostSetup && !bHosted && !challengeActive)
	{
		MultibuttonWidget *hostButton = new MultibuttonWidget(optionsList);
		hostButton->id = MULTIOP_HOST;
		hostButton->setLabel(_("Start Hosting Game"));
		hostButton->addButton(0, Image(FrontImages, IMAGE_HOST), Image(FrontImages, IMAGE_HOST_HI), _("Start Hosting Game"));
		optionsList->addWidgetToLayout(hostButton);
	}

	// cancel
	addMultiBut(psWScreen, MULTIOP_OPTIONS, CON_CANCEL,
	            MULTIOP_CANCELX, MULTIOP_CANCELY,
	            iV_GetImageWidth(FrontImages, IMAGE_RETURN),
	            iV_GetImageHeight(FrontImages, IMAGE_RETURN),
	            _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	// Add any relevant factory disabled icons.
	updateLimitFlags();

	int y = 2;
	bool skip = false;
	for (int i = 0; i < ARRAY_SIZE(limitIcons); ++i)
	{
		if ((ingame.flags & 1 << i) != 0)
		{
			if (!skip)
			{
				// only add this once.
				addBlueForm(MULTIOP_OPTIONS, MULTIOP_NO_SOMETHING, "", MULTIOP_HOSTX, MULTIOP_NO_SOMETHINGY, 41, 152);
			}

			addMultiBut(psWScreen, MULTIOP_NO_SOMETHING, MULTIOP_NO_SOMETHINGY + i, MULTIOP_NO_SOMETHINGX, y,
			            35, 28, _(limitIcons[i].desc),
			            limitIcons[i].icon, limitIcons[i].icon, limitIcons[i].icon);
			y += 28 + 3;
			skip = true;
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Colour functions

static bool safeToUseColour(unsigned player, unsigned otherPlayer)
{
	// Player wants to take the colour from otherPlayer. May not take from a human otherPlayer, unless we're the host.
	return player == otherPlayer || NetPlay.isHost || !isHumanPlayer(otherPlayer);
}

static void initChooser(int player)
{
	// delete that players box,
	widgDelete(psWScreen, MULTIOP_PLAYER_START + player);

	// delete team chooser button
	widgDelete(psWScreen, MULTIOP_TEAMS_START + player);

	// delete 'ready' button
	widgDelete(psWScreen, MULTIOP_READY_FORM_ID + player);

	// delete 'colour' button
	widgDelete(psWScreen, MULTIOP_COLOUR_START + player);

	// remove any choosers already up
	closeColourChooser();
	closeTeamChooser();
}

static void addDifficultyChooser(int player)
{
	closeColourChooser();
	closeTeamChooser();
	widgDelete(psWScreen, MULTIOP_AI_FORM);
	widgDelete(psWScreen, MULTIOP_PLAYERS);
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);
	difficultyChooserUp = player;

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *aiForm = new IntFormAnimated(parent, false);
	aiForm->id = MULTIOP_AI_FORM;
	aiForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_PLAYERSX, MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
	}));

	addSideText(FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, _("DIFFICULTY"));

	for (int i = 0; i < 4; i++)
	{
		W_BUTINIT sButInit;
		sButInit.formID = MULTIOP_AI_FORM;
		sButInit.id = MULTIOP_DIFFICULTY_CHOOSE_START + i;
		sButInit.x = 7;
		sButInit.y = (MULTIOP_PLAYERHEIGHT + 5) * i + 4;
		sButInit.width = MULTIOP_PLAYERWIDTH + 1;
		sButInit.height = MULTIOP_PLAYERHEIGHT;
		switch (i)
		{
		case 0: sButInit.pTip = _("Starts disadvantaged"); break;
		case 1: sButInit.pTip = _("Plays nice"); break;
		case 2: sButInit.pTip = _("No holds barred"); break;
		case 3: sButInit.pTip = _("Starts with advantages"); break;
		}
		sButInit.pDisplay = displayDifficulty;
		sButInit.UserData = i;
		sButInit.pUserData = new DisplayDifficultyCache();
		sButInit.onDelete = [](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayDifficultyCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		};
		widgAddButton(psWScreen, &sButInit);
	}
}

static void addAiChooser(int player)
{
	closeColourChooser();
	closeTeamChooser();
	widgDelete(psWScreen, MULTIOP_AI_FORM);
	widgDelete(psWScreen, MULTIOP_PLAYERS);
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);
	aiChooserUp = player;

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *aiForm = new IntFormAnimated(parent, false);
	aiForm->id = MULTIOP_AI_FORM;
	aiForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_PLAYERSX, MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
	}));

	addSideText(FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, _("CHOOSE AI"));

	W_BUTINIT sButInit;
	sButInit.formID = MULTIOP_AI_FORM;
	sButInit.x = 7;
	sButInit.width = MULTIOP_PLAYERWIDTH + 1;
	// Try to fit as many as possible, just got to make sure text fits in the 'box'.
	// NOTE: Correct way would be to get the actual font size, render the text, and see what fits.
	if (aidata.size() > 8)
	{
		sButInit.height = MULTIOP_PLAYERHEIGHT - 7;
	}
	else
	{
		sButInit.height = MULTIOP_PLAYERHEIGHT;
	}
	sButInit.pDisplay = displayAi;
	sButInit.initPUserDataFunc = []() -> void * { return new DisplayAICache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayAICache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	// only need this button in (true) mp games
	int mpbutton = NetPlay.bComms ? 1 : 0;
	// cap AI's that are shown, since it looks a bit ugly.  *FIXME*
	int capAIs = aidata.size();
	if (aidata.size() > 9)
	{
		debug(LOG_INFO, "You have too many AI's loaded for the GUI to handle.  Only the first 10 will be shown.");
		addConsoleMessage("You have too many AI's loaded for the GUI to handle.  Only the first 10 will be shown.", DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		capAIs = 10;
	}

	// button height * how many AI + possible buttons (openclosed)
	int gap = MULTIOP_PLAYERSH - ((sButInit.height) * (capAIs + 1 + mpbutton));
	int gapDiv = capAIs - 1;
	gap = std::min(gap, 5 * gapDiv);

	// Open button
	if (mpbutton)
	{
		sButInit.id = MULTIOP_AI_OPEN;
		sButInit.pTip = _("Allow human players to join in this slot");
		sButInit.UserData = (UDWORD)AI_OPEN;
		sButInit.y = 3;	//Top most position
		widgAddButton(psWScreen, &sButInit);
	}

	// Closed button
	sButInit.pTip = _("Leave this slot unused");
	sButInit.id = MULTIOP_AI_CLOSED;
	sButInit.UserData = (UDWORD)AI_CLOSED;
	if (mpbutton)
	{
		sButInit.y = sButInit.height;
	}
	else
	{
		sButInit.y = 0; //since we don't have the lone mpbutton, we can start at position 0
	}
	widgAddButton(psWScreen, &sButInit);

	for (int i = 0; i < capAIs; i++)
	{
		sButInit.y = (sButInit.height * gapDiv + gap) * (i + 1 + mpbutton) / gapDiv; // +1 for 'closed', and possible +1 more for 'open' for MP games)
		sButInit.pTip = aidata[i].tip;
		sButInit.id = MULTIOP_AI_START + i;
		sButInit.UserData = i;
		widgAddButton(psWScreen, &sButInit);
	}
}

static void closeAiChooser()
{
	widgDelete(psWScreen, MULTIOP_AI_FORM);
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);
	aiChooserUp = -1;
}

static void closeDifficultyChooser()
{
	widgDelete(psWScreen, MULTIOP_AI_FORM);
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);
	difficultyChooserUp = -1;
}

static void closePositionChooser()
{
	positionChooserUp = -1;
}

static int playerBoxHeight(int player)
{
	int gap = MULTIOP_PLAYERSH - MULTIOP_TEAMSHEIGHT * game.maxPlayers;
	int gapDiv = game.maxPlayers - 1;
	gap = std::min(gap, 5 * gapDiv);
	STATIC_ASSERT(MULTIOP_TEAMSHEIGHT == MULTIOP_PLAYERHEIGHT);  // Why are these different defines?
	return (MULTIOP_TEAMSHEIGHT * gapDiv + gap) * NetPlay.players[player].position / gapDiv;
}

static void addPositionChooser(int player)
{
	closeColourChooser();
	closeTeamChooser();
	closePositionChooser();
	closeAiChooser();
	closeDifficultyChooser();
	positionChooserUp = player;
	addPlayerBox(true);
}

static void addColourChooser(UDWORD player)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player number");
	initChooser(player);

	// add form.
	addBlueForm(MULTIOP_PLAYERS, MULTIOP_COLCHOOSER_FORM, "",
	            8,
	            playerBoxHeight(player),
	            MULTIOP_ROW_WIDTH, MULTIOP_PLAYERHEIGHT);

	// add the flags
	int flagW = iV_GetImageWidth(FrontImages, IMAGE_PLAYERN);
	int flagH = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);
	int space = MULTIOP_ROW_WIDTH - 0 - flagW * MAX_PLAYERS_IN_GUI;
	int spaceDiv = MAX_PLAYERS_IN_GUI;
	space = std::min(space, 5 * spaceDiv);
	for (unsigned int i = 0; i < MAX_PLAYERS_IN_GUI; i++)
	{
		addMultiBut(psWScreen, MULTIOP_COLCHOOSER_FORM, MULTIOP_COLCHOOSER + getPlayerColour(i),
		            i * (flagW * spaceDiv + space) / spaceDiv + 7,  4, // x, y
		            flagW, flagH,  // w, h
		            getPlayerColourName(i), IMAGE_PLAYERN, IMAGE_PLAYERN_HI, IMAGE_PLAYERN_HI, getPlayerColour(i));

		if (!safeToUseColour(selectedPlayer, i))
		{
			widgSetButtonState(psWScreen, MULTIOP_COLCHOOSER + getPlayerColour(i), WBUT_DISABLE);
		}
	}

	colourChooserUp = player;
}

static void closeColourChooser()
{
	colourChooserUp = -1;
	widgDelete(psWScreen, MULTIOP_COLCHOOSER_FORM);
}

static void changeTeam(UBYTE player, UBYTE team)
{
	NetPlay.players[player].team = team;
	debug(LOG_WZ, "set %d as new team for player %d", team, player);
	NETBroadcastPlayerInfo(player);
	netPlayersUpdated = true;
}

static bool SendTeamRequest(UBYTE player, UBYTE chosenTeam)
{
	if (NetPlay.isHost)			// do or request the change.
	{
		changeTeam(player, chosenTeam);	// do the change, remember only the host can do this to avoid confusion.
	}
	else
	{
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_TEAMREQUEST);

		NETuint8_t(&player);
		NETuint8_t(&chosenTeam);

		NETend();

	}
	return true;
}

bool recvTeamRequest(NETQUEUE queue)
{
	UBYTE	player, team;

	if (!NetPlay.isHost || !bHosted)  // Only host should act, and only if the game hasn't started yet.
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(queue, NET_TEAMREQUEST);
	NETuint8_t(&player);
	NETuint8_t(&team);
	NETend();

	if (player >= MAX_PLAYERS || team >= MAX_PLAYERS)
	{
		debug(LOG_NET, "NET_TEAMREQUEST invalid, player %d team, %d", (int) player, (int) team);
		debug(LOG_ERROR, "Invalid NET_TEAMREQUEST from player %d: Tried to change player %d (team %d)",
		      queue.index, (int)player, (int)team);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_TEAMREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	if (NetPlay.players[player].team != team)
	{
		resetReadyStatus(false);
	}
	debug(LOG_NET, "%s is now part of team: %d", NetPlay.players[player].name, (int) team);
	changeTeam(player, team); // we do this regardless, in case of sync issues

	return true;
}

static bool SendReadyRequest(UBYTE player, bool bReady)
{
	if (NetPlay.isHost)			// do or request the change.
	{
		return changeReadyStatus(player, bReady);
	}
	else
	{
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_READY_REQUEST);
		NETuint8_t(&player);
		NETbool(&bReady);
		NETend();
	}
	return true;
}

bool recvReadyRequest(NETQUEUE queue)
{
	UBYTE	player;
	bool	bReady = false;

	if (!NetPlay.isHost || !bHosted)  // Only host should act, and only if the game hasn't started yet.
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(queue, NET_READY_REQUEST);
	NETuint8_t(&player);
	NETbool(&bReady);
	NETend();

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_READY_REQUEST from player %d: player id = %d",
		      queue.index, (int)player);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_READY_REQUEST given incorrect params.", player, queue.index);
		return false;
	}

	// do not allow players to select 'ready' if we are sending a map too them!
	// TODO: make a new icon to show this state?
	if (!NetPlay.players[player].wzFiles.empty())
	{
		return false;
	}

	return changeReadyStatus((UBYTE)player, bReady);
}

static bool changeReadyStatus(UBYTE player, bool bReady)
{
	NetPlay.players[player].ready = bReady;
	NETBroadcastPlayerInfo(player);
	netPlayersUpdated = true;

	return true;
}

static bool changePosition(UBYTE player, UBYTE position)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (NetPlay.players[i].position == position)
		{
			debug(LOG_NET, "Swapping positions between players %d(%d) and %d(%d)",
			      player, NetPlay.players[player].position, i, NetPlay.players[i].position);
			std::swap(NetPlay.players[i].position, NetPlay.players[player].position);
			std::swap(NetPlay.players[i].team, NetPlay.players[player].team);
			NETBroadcastTwoPlayerInfo(player, i);
			netPlayersUpdated = true;
			return true;
		}
	}
	debug(LOG_ERROR, "Failed to swap positions for player %d, position %d", (int)player, (int)position);
	if (player < game.maxPlayers && position < game.maxPlayers)
	{
		debug(LOG_NET, "corrupted positions: player (%u) new position (%u) old position (%d)", player, position, NetPlay.players[player].position);
		// Positions were corrupted. Attempt to fix.
		NetPlay.players[player].position = position;
		NETBroadcastPlayerInfo(player);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

bool changeColour(unsigned player, int col, bool isHost)
{
	if (col < 0 || col >= MAX_PLAYERS_IN_GUI)
	{
		return true;
	}

	if (getPlayerColour(player) == col)
	{
		return true;  // Nothing to do.
	}

	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (getPlayerColour(i) == col)
		{
			if (!isHost && NetPlay.players[i].allocated)
			{
				return true;  // May not swap.
			}

			debug(LOG_NET, "Swapping colours between players %d(%d) and %d(%d)",
			      player, getPlayerColour(player), i, getPlayerColour(i));
			setPlayerColour(i, getPlayerColour(player));
			NetPlay.players[i].colour = getPlayerColour(player);
			setPlayerColour(player, col);
			NetPlay.players[player].colour = col;
			NETBroadcastTwoPlayerInfo(player, i);
			netPlayersUpdated = true;
			return true;
		}
	}
	debug(LOG_ERROR, "Failed to swap colours for player %d, colour %d", (int)player, (int)col);
	if (player < game.maxPlayers && col < MAX_PLAYERS)
	{
		// Colours were corrupted. Attempt to fix.
		debug(LOG_NET, "corrupted colours: player (%u) new colour (%u) old colour (%d)", player, col, NetPlay.players[player].colour);
		setPlayerColour(player, col);
		NetPlay.players[player].colour = col;
		NETBroadcastPlayerInfo(player);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

static bool SendColourRequest(UBYTE player, UBYTE col)
{
	if (NetPlay.isHost)			// do or request the change
	{
		return changeColour(player, col, true);
	}
	else
	{
		// clients tell the host which color they want
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_COLOURREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&col);
		NETend();
	}
	return true;
}

static bool SendPositionRequest(UBYTE player, UBYTE position)
{
	if (NetPlay.isHost)			// do or request the change
	{
		return changePosition(player, position);
	}
	else
	{
		debug(LOG_NET, "Requesting the host to change our position. From %d to %d", player, position);
		// clients tell the host which position they want
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_POSITIONREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&position);
		NETend();
	}
	return true;
}

bool recvColourRequest(NETQUEUE queue)
{
	UBYTE	player, col;

	if (!NetPlay.isHost || !bHosted)  // Only host should act, and only if the game hasn't started yet.
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(queue, NET_COLOURREQUEST);
	NETuint8_t(&player);
	NETuint8_t(&col);
	NETend();

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_COLOURREQUEST from player %d: Tried to change player %d to colour %d",
		      queue.index, (int)player, (int)col);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_COLOURREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	resetReadyStatus(false);

	return changeColour(player, col, false);
}

bool recvPositionRequest(NETQUEUE queue)
{
	UBYTE	player, position;

	if (!NetPlay.isHost || !bHosted)  // Only host should act, and only if the game hasn't started yet.
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(queue, NET_POSITIONREQUEST);
	NETuint8_t(&player);
	NETuint8_t(&position);
	NETend();
	debug(LOG_NET, "Host received position request from player %d to %d", player, position);

	if (player >= MAX_PLAYERS || position >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_POSITIONREQUEST from player %d: Tried to change player %d to %d",
		      queue.index, (int)player, (int)position);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_POSITIONREQUEST given incorrect params.", player, queue.index);
		return false;
	}

	resetReadyStatus(false);

	return changePosition(player, position);
}

static int getPlayerTeam(int i)
{
	return alliancesSetTeamsBeforeGame(game.alliance) ? NetPlay.players[i].team : i;
}

// If so, return that team; if not, return -1; if there are no players, return team MAX_PLAYERS.
static int allPlayersOnSameTeam(int except)
{
	int minTeam = MAX_PLAYERS, maxTeam = 0, numPlayers = 0;
	for (int i = 0; i < game.maxPlayers; ++i)
	{
		if (i != except && (NetPlay.players[i].allocated || NetPlay.players[i].ai >= 0))
		{
			int team = getPlayerTeam(i);
			minTeam = std::min(minTeam, team);
			maxTeam = std::max(maxTeam, team);
			++numPlayers;
		}
	}
	if (minTeam == MAX_PLAYERS || minTeam == maxTeam)
	{
		return minTeam;  // Players all on same team.
	}
	return -1;  // Players not all on same team.
}

/*
 * Opens a menu for a player to choose a team
 * 'player' is a player id of the player who will get a new team assigned
 */
static void addTeamChooser(UDWORD player)
{
	UDWORD i;
	int disallow = allPlayersOnSameTeam(player);

	debug(LOG_NET, "Opened team chooser for %d, current team: %d", player, NetPlay.players[player].team);

	initChooser(player);

	// add form.
	addBlueForm(MULTIOP_PLAYERS, MULTIOP_TEAMCHOOSER_FORM, "", 8, playerBoxHeight(player), MULTIOP_ROW_WIDTH, MULTIOP_TEAMSHEIGHT);

	int teamW = iV_GetImageWidth(FrontImages, IMAGE_TEAM0);
	int teamH = iV_GetImageHeight(FrontImages, IMAGE_TEAM0);
	int space = MULTIOP_ROW_WIDTH - 4 - teamW * (game.maxPlayers + 1);
	int spaceDiv = game.maxPlayers;
	space = std::min(space, 3 * spaceDiv);

	// add the teams, skipping the one we CAN'T be on (if applicable)
	for (i = 0; i < game.maxPlayers; i++)
	{
		if (i != disallow)
		{
			addMultiBut(psWScreen, MULTIOP_TEAMCHOOSER_FORM, MULTIOP_TEAMCHOOSER + i, i * (teamW * spaceDiv + space) / spaceDiv + 3, 6, teamW, teamH, _("Team"), IMAGE_TEAM0 + i , IMAGE_TEAM0_HI + i, IMAGE_TEAM0_HI + i);
		}
		// may want to add some kind of 'can't do' icon instead of being blank?
	}

	// add a kick button
	if (player != selectedPlayer && NetPlay.bComms && NetPlay.isHost && NetPlay.players[player].allocated)
	{
		const int imgwidth = iV_GetImageWidth(FrontImages, IMAGE_NOJOIN);
		const int imgheight = iV_GetImageHeight(FrontImages, IMAGE_NOJOIN);
		addMultiBut(psWScreen, MULTIOP_TEAMCHOOSER_FORM, MULTIOP_TEAMCHOOSER_KICK, MULTIOP_ROW_WIDTH - imgwidth - 4, 8, imgwidth, imgheight,
		            ("Kick player"), IMAGE_NOJOIN, IMAGE_NOJOIN, IMAGE_NOJOIN);
	}

	teamChooserUp = player;
}

/*
 * Closes Team Chooser dialog box, if there was any open
 */
static void closeTeamChooser()
{
	teamChooserUp = -1;
	widgDelete(psWScreen, MULTIOP_TEAMCHOOSER_FORM);	//only once!
}

static void drawReadyButton(UDWORD player)
{
	int disallow = allPlayersOnSameTeam(-1);

	// delete 'ready' botton form
	widgDelete(psWScreen, MULTIOP_READY_FORM_ID + player);

	// add form to hold 'ready' botton
	addBlueForm(MULTIOP_PLAYERS, MULTIOP_READY_FORM_ID + player, "",
	            7 + MULTIOP_PLAYERWIDTH - MULTIOP_READY_WIDTH,
	            playerBoxHeight(player),
	            MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT);

	WIDGET *parent = widgGetFromID(psWScreen, MULTIOP_READY_FORM_ID + player);


	if (!NetPlay.players[player].allocated && NetPlay.players[player].ai >= 0)
	{
		int icon = difficultyIcon(NetPlay.players[player].difficulty);
		addMultiBut(psWScreen, MULTIOP_READY_FORM_ID + player, MULTIOP_DIFFICULTY_INIT_START + player, 6, 4, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
		            locked.difficulty ? _("Difficulty locked") : (NetPlay.isHost ? _("Click to change difficulty") : nullptr), icon, icon, icon);
		return;
	}
	else if (!NetPlay.players[player].allocated)
	{
		return;	// closed or open
	}

	if (disallow != -1)
	{
		return;
	}

	bool isMe = player == selectedPlayer;
	int isReady = NETgetDownloadProgress(player) != 100 ? 2 : NetPlay.players[player].ready ? 1 : 0;
	char const *const toolTips[2][3] = {{_("Waiting for player"), _("Player is ready"), _("Player is downloading")}, {_("Click when ready"), _("Waiting for other players"), _("Waiting for download")}};
	unsigned images[2][3] = {{IMAGE_CHECK_OFF, IMAGE_CHECK_ON, IMAGE_CHECK_DOWNLOAD}, {IMAGE_CHECK_OFF_HI, IMAGE_CHECK_ON_HI, IMAGE_CHECK_DOWNLOAD_HI}};

	// draw 'ready' button
	addMultiBut(psWScreen, MULTIOP_READY_FORM_ID + player, MULTIOP_READY_START + player, 3, 10, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
	            toolTips[isMe][isReady], images[0][isReady], images[0][isReady], images[isMe][isReady]);

	W_LABEL *label = new W_LABEL(parent);
	label->id = MULTIOP_READY_START + MAX_PLAYERS + player;
	label->setGeometry(0, 0, MULTIOP_READY_WIDTH, 17);
	label->setTextAlignment(WLAB_ALIGNBOTTOM);
	label->setFont(font_small, WZCOL_TEXT_BRIGHT);
	label->setString(_("READY?"));
}

static bool canChooseTeamFor(int i)
{
	return (i == selectedPlayer || NetPlay.isHost);
}

// ////////////////////////////////////////////////////////////////////////////
// box for players.

void addPlayerBox(bool players)
{
	// if background isn't there, then return since were not ready to draw the box yet!
	if (widgGetFromID(psWScreen, FRONTEND_BACKDROP) == nullptr)
	{
		return;
	}

	widgDelete(psWScreen, MULTIOP_PLAYERS);		// del player window
	widgDelete(psWScreen, FRONTEND_SIDETEXT2);	// del text too,

	if (aiChooserUp >= 0)
	{
		addAiChooser(aiChooserUp);
		return;
	}
	else if (difficultyChooserUp >= 0)
	{
		addDifficultyChooser(difficultyChooserUp);
		return;
	}

	// draw player window
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *playersForm = new IntFormAnimated(parent, false);
	playersForm->id = MULTIOP_PLAYERS;
	playersForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_PLAYERSX, MULTIOP_PLAYERSY, MULTIOP_PLAYERSW, MULTIOP_PLAYERSH);
	}));

	addSideText(FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, _("PLAYERS"));

	if (players)
	{
		int  team = -1;
		bool allOnSameTeam = true;

		for (int i = 0; i < game.maxPlayers; i++)
		{
			if (game.skDiff[i] || isHumanPlayer(i))
			{
				int myTeam = getPlayerTeam(i);
				if (team == -1)
				{
					team = myTeam;
				}
				else if (myTeam != team)
				{
					allOnSameTeam = false;
					break;  // We just need to know if we have enough to start a game
				}
			}
		}

		for (int i = 0; i < game.maxPlayers; i++)
		{
			if (positionChooserUp >= 0 && positionChooserUp != i && (NetPlay.isHost || !isHumanPlayer(i)))
			{
				W_BUTINIT sButInit;
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_PLAYER_START + i;
				sButInit.x = 7;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_PLAYERWIDTH + 1;
				sButInit.height = MULTIOP_PLAYERHEIGHT;
				sButInit.pTip = _("Click to change to this slot");
				sButInit.pDisplay = displayPosition;
				sButInit.UserData = i;
				sButInit.pUserData = new DisplayPositionCache();
				sButInit.onDelete = [](WIDGET *psWidget) {
					assert(psWidget->pUserData != nullptr);
					delete static_cast<DisplayPositionCache *>(psWidget->pUserData);
					psWidget->pUserData = nullptr;
				};
				widgAddButton(psWScreen, &sButInit);
				continue;
			}
			else if (i == colourChooserUp)
			{
				addColourChooser(i);
				continue;
			}
			else if (i == teamChooserUp)
			{
				addTeamChooser(i);
				continue;
			}
			else if (ingame.localOptionsReceived)
			{
				//add team chooser
				W_BUTINIT sButInit;
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_TEAMS_START + i;
				sButInit.x = 7;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_TEAMSWIDTH;
				sButInit.height = MULTIOP_TEAMSHEIGHT;
				if (canChooseTeamFor(i) && !locked.teams)
				{
					sButInit.pTip = _("Choose Team");
				}
				else if (locked.teams)
				{
					sButInit.pTip = _("Teams locked");
				}
				sButInit.pDisplay = displayTeamChooser;
				sButInit.UserData = i;

				if (teamChooserUp == i && colourChooserUp < 0)
				{
					addTeamChooser(i);
				}
				else if (alliancesSetTeamsBeforeGame(game.alliance))
				{
					// only if not disabled and in locked teams mode
					widgAddButton(psWScreen, &sButInit);
				}
			}

			// draw player colour
			W_BUTINIT sColInit;
			sColInit.formID = MULTIOP_PLAYERS;
			sColInit.id = MULTIOP_COLOUR_START + i;
			sColInit.x = 7 + MULTIOP_TEAMSWIDTH;
			sColInit.y = playerBoxHeight(i);
			sColInit.width = MULTIOP_COLOUR_WIDTH;
			sColInit.height = MULTIOP_PLAYERHEIGHT;
			if (selectedPlayer == i || NetPlay.isHost)
			{
				sColInit.pTip = _("Click to change player colour");
			}
			sColInit.pDisplay = displayColour;
			sColInit.UserData = i;
			widgAddButton(psWScreen, &sColInit);

			if (ingame.localOptionsReceived)
			{
				if (!allOnSameTeam)
				{
					drawReadyButton(i);
				}

				// draw player info box
				W_BUTINIT sButInit;
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_PLAYER_START + i;
				sButInit.x = 7 + MULTIOP_TEAMSWIDTH + MULTIOP_COLOUR_WIDTH;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_PLAYERWIDTH - MULTIOP_TEAMSWIDTH - MULTIOP_READY_WIDTH - MULTIOP_COLOUR_WIDTH;
				sButInit.height = MULTIOP_PLAYERHEIGHT;
				if ((selectedPlayer == i || NetPlay.isHost) && NetPlay.players[i].allocated && !locked.position)
				{
					sButInit.pTip = _("Click to change player position");
				}
				else if (!NetPlay.players[i].allocated && NetPlay.isHost && !locked.ai)
				{
					sButInit.pTip = _("Click to change AI");
				}
				if (NetPlay.players[i].allocated && !getMultiStats(i).identity.empty())
				{
					if (!sButInit.pTip.empty())
					{
						sButInit.pTip += "\n";
					}
					EcKey::Key bytes = getMultiStats(i).identity.toBytes(EcKey::Public);
					sButInit.pTip += _("Player ID: ");
					if (!bytes.empty())
					{
						sButInit.pTip += sha256Sum(&bytes[0], bytes.size()).toString().substr(0, 20).c_str();
					}
					else
					{
						sButInit.pTip += _("(none)");
					}
				}
				sButInit.pDisplay = displayPlayer;
				sButInit.UserData = i;
				sButInit.pUserData = new DisplayPlayerCache();
				sButInit.onDelete = [](WIDGET *psWidget) {
					assert(psWidget->pUserData != nullptr);
					delete static_cast<DisplayPlayerCache *>(psWidget->pUserData);
					psWidget->pUserData = nullptr;
				};
				widgAddButton(psWScreen, &sButInit);
			}
		}
	}
}

/*
 * Notify all players of host launching the game
 */
static void SendFireUp()
{
	uint32_t randomSeed = rand();  // Pick a random random seed for the synchronised random number generator.

	NETbeginEncode(NETbroadcastQueue(), NET_FIREUP);
	NETuint32_t(&randomSeed);
	NETend();
	printSearchPath();
	gameSRand(randomSeed);  // Set the seed for the synchronised random number generator. The clients will use the same seed.
}

// host kicks a player from a game.
void kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type)
{
	// send a kick msg
	NETbeginEncode(NETbroadcastQueue(), NET_KICK);
	NETuint32_t(&player_id);
	NETstring(reason, MAX_KICK_REASON);
	NETenum(&type);
	NETend();
	NETflush();
	wzDelay(300);
	debug(LOG_NET, "Kicking player %u (%s).",
	      (unsigned int)player_id, getPlayerName(player_id));

	NETplayerKicked(player_id);
}

static void addChatBox(bool preserveOldChat)
{
	if (widgGetFromID(psWScreen, FRONTEND_TOPFORM))
	{
		widgDelete(psWScreen, FRONTEND_TOPFORM);
	}

	if (widgGetFromID(psWScreen, MULTIOP_CHATBOX))
	{
		return;
	}

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *chatBox = new IntFormAnimated(parent);
	chatBox->id = MULTIOP_CHATBOX;
	chatBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_CHATBOXX, MULTIOP_CHATBOXY, MULTIOP_CHATBOXW, MULTIOP_CHATBOXH);
	}));

	addSideText(FRONTEND_SIDETEXT4, MULTIOP_CHATBOXX - 3, MULTIOP_CHATBOXY, _("CHAT"));

	if (!preserveOldChat)
	{
		flushConsoleMessages();  // add the chatbox.
		initConsoleMessages();
		setConsoleBackdropStatus(false);
		setConsoleCalcLayout([]() {
			setConsoleSizePos(MULTIOP_CHATBOXX + 4 + D_W, MULTIOP_CHATBOXY + 14 + D_H, MULTIOP_CHATBOXW - 4);
		});
		setConsolePermanence(true, true);
		setConsoleLineInfo(5);  // use x lines on chat window
	}
	enableConsoleDisplay(true);

	W_EDBINIT sEdInit;                                                                                      // add the edit box
	sEdInit.formID = MULTIOP_CHATBOX;
	sEdInit.id = MULTIOP_CHATEDIT;
	sEdInit.x = MULTIOP_CHATEDITX;
	sEdInit.y = MULTIOP_CHATEDITY;
	sEdInit.width = MULTIOP_CHATEDITW;
	sEdInit.height = MULTIOP_CHATEDITH;

	sEdInit.pUserData = nullptr;
	sEdInit.pBoxDisplay = displayChatEdit;

	widgAddEditBox(psWScreen, &sEdInit);

	if (!getModList().empty())
	{
		WzString modListMessage = _("Mod: ");
		modListMessage += getModList().c_str();
		addConsoleMessage(modListMessage.toUtf8().c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}

	return;
}

static void addConsoleBox()
{
	if (widgGetFromID(psWScreen, FRONTEND_TOPFORM))
	{
		widgDelete(psWScreen, FRONTEND_TOPFORM);
	}

	if (widgGetFromID(psWScreen, MULTIOP_CONSOLEBOX))
	{
		return;
	}

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *consoleBox = new IntFormAnimated(parent);
	consoleBox->id = MULTIOP_CONSOLEBOX;
	consoleBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_CONSOLEBOXX, MULTIOP_CONSOLEBOXY, MULTIOP_CONSOLEBOXW, MULTIOP_CONSOLEBOXH);
	}));

	flushConsoleMessages();											// add the chatbox.
	initConsoleMessages();
	enableConsoleDisplay(true);
	setConsoleBackdropStatus(false);
	setConsoleCalcLayout([]() {
		setConsoleSizePos(MULTIOP_CONSOLEBOXX + 4 + D_W, MULTIOP_CONSOLEBOXY + 14 + D_H, MULTIOP_CONSOLEBOXW - 4);
	});
	setConsolePermanence(true, true);
	setConsoleLineInfo(3);											// use x lines on chat window

	addConsoleMessage(_("Connecting to the lobby server..."), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
	displayConsoleMessages();
	return;
}

// ////////////////////////////////////////////////////////////////////////////
static void disableMultiButs()
{
	if (!NetPlay.isHost)
	{
		// edit box icons.
		widgSetButtonState(psWScreen, MULTIOP_GNAME_ICON, WBUT_DISABLE);
		widgSetButtonState(psWScreen, MULTIOP_MAP_ICON, WBUT_DISABLE);
		
		// edit boxes
		widgSetButtonState(psWScreen, MULTIOP_GNAME, WEDBS_DISABLE);
		
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_GAMETYPE))->disable();  // Scavengers.
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_BASETYPE))->disable();  // camapign subtype.
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_POWER))->disable();  // pow levels
		((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_ALLIANCES))->disable();
	}
}

////////////////////////////////////////////////////////////////////////////
static void stopJoining()
{
	dwSelectedGame	 = 0;
	reloadMPConfig(); // reload own settings

	debug(LOG_NET, "player %u (Host is %s) stopping.", selectedPlayer, NetPlay.isHost ? "true" : "false");

	if (bHosted)											// cancel a hosted game.
	{
		// annouce we are leaving...
		debug(LOG_NET, "Host is quitting game...");
		NETbeginEncode(NETbroadcastQueue(), NET_HOST_DROPPED);
		NETend();
		sendLeavingMsg();								// say goodbye
		NETclose();										// quit running game.
		bHosted = false;								// stop host mode.
		widgDelete(psWScreen, FRONTEND_BACKDROP);		// refresh options screen.
		startMultiOptions(false);
		ingame.localJoiningInProgress = false;
		return;
	}
	else if (ingame.localJoiningInProgress)				// cancel a joined game.
	{
		debug(LOG_NET, "Canceling game...");
		sendLeavingMsg();								// say goodbye
		NETclose();										// quit running game.

		// if we were in a midle of transferring a file, then close the file handle
		for (auto const &file : NetPlay.wzFiles)
		{
			debug(LOG_NET, "closing aborted file");		// no need to delete it, we do size check on (map) file
			PHYSFS_close(file.handle);
		}
		NetPlay.wzFiles.clear();
		ingame.localJoiningInProgress = false;			// reset local flags
		ingame.localOptionsReceived = false;
		if (!ingame.bHostSetup && NetPlay.isHost)			// joining and host was transferred.
		{
			NetPlay.isHost = false;
		}

		changeTitleMode(MULTI);

		selectedPlayer = 0;
		realSelectedPlayer = 0;
		return;
	}
	debug(LOG_NET, "We have stopped joining.");
	changeTitleMode(lastTitleMode);
	selectedPlayer = 0;
	realSelectedPlayer = 0;

	if (ingame.bHostSetup)
	{
		pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	}
}

static void loadMapSettings1()
{
	char aFileName[256];
	LEVEL_DATASET *psLevel = levFindDataSet(game.map, &game.hash);

	ASSERT_OR_RETURN(, psLevel, "No level found for %s", game.map);
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game]);
	aFileName[std::max<size_t>(strlen(aFileName), 4) - 4] = '\0';
	sstrcat(aFileName, ".json");

	WzString ininame = challengeActive ? sRequestResult : aFileName;
	if (hostlaunch == 2)
	{
		ininame = "tests/" + WzString::fromUtf8(wz_skirmish_test());
	}
	if (!PHYSFS_exists(ininame.toUtf8().c_str()))
	{
		return;
	}
	WzConfig ini(ininame, WzConfig::ReadOnly);

	ini.beginGroup("locked"); // GUI lockdown
	locked.power = ini.value("power", challengeActive).toBool();
	locked.alliances = ini.value("alliances", challengeActive).toBool();
	locked.teams = ini.value("teams", challengeActive).toBool();
	locked.difficulty = ini.value("difficulty", challengeActive).toBool();
	locked.ai = ini.value("ai", challengeActive).toBool();
	locked.scavengers = ini.value("scavengers", challengeActive).toBool();
	locked.position = ini.value("position", challengeActive).toBool();
	locked.bases = ini.value("bases", challengeActive).toBool();
	ini.endGroup();

	ini.beginGroup("defaults");
	game.scavengers = ini.value("scavengers", game.scavengers).toBool();
	game.base = ini.value("bases", game.base).toInt();
	game.alliance = ini.value("alliances", game.alliance).toInt();
	if (ini.contains("powerLevel"))
	{
		game.power = ini.value("powerLevel", game.power).toInt();
	}
	ini.endGroup();
}

static void loadMapSettings2()
{
	char aFileName[256];
	LEVEL_DATASET *psLevel = levFindDataSet(game.map, &game.hash);

	ASSERT_OR_RETURN(, psLevel, "No level found for %s", game.map);
	sstrcpy(aFileName, psLevel->apDataFiles[psLevel->game]);
	aFileName[std::max<size_t>(strlen(aFileName), 4) - 4] = '\0';
	sstrcat(aFileName, ".json");

	WzString ininame = challengeActive ? sRequestResult : aFileName;
	if (hostlaunch == 2)
	{
		ininame = "tests/" + WzString::fromUtf8(wz_skirmish_test());
	}
	if (!PHYSFS_exists(ininame.toUtf8().c_str()))
	{
		return;
	}
	WzConfig ini(ininame, WzConfig::ReadOnly);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		ini.beginGroup("player_" + WzString::number(i));
		if (ini.contains("team"))
		{
			NetPlay.players[i].team = ini.value("team").toInt();
		}
		else if (challengeActive) // team is a required key for challenges
		{
			NetPlay.players[i].ai = AI_CLOSED;
		}
		if (ini.contains("name"))
		{
			sstrcpy(NetPlay.players[i].name, ini.value("name").toWzString().toUtf8().c_str());
		}
		NetPlay.players[i].position = MAX_PLAYERS;  // Invalid value, fix later.
		if (ini.contains("position"))
		{
			NetPlay.players[i].position = std::min(std::max(ini.value("position").toInt(), 0), MAX_PLAYERS);
		}
		if (ini.contains("difficulty"))
		{
			WzString value = ini.value("difficulty", "Medium").toWzString();
			for (unsigned j = 0; j < ARRAY_SIZE(difficultyList); ++j)
			{
				if (strcasecmp(difficultyList[j], value.toUtf8().c_str()) == 0)
				{
					game.skDiff[i] = difficultyValue[j];
					NetPlay.players[i].difficulty = j;
				}
			}
		}
		ini.endGroup();
	}

	// Fix duplicate or unset player positions.
	PlayerMask havePosition = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (NetPlay.players[i].position < MAX_PLAYERS)
		{
			PlayerMask old = havePosition;
			havePosition |= PlayerMask(1) << NetPlay.players[i].position;
			if (havePosition == old)
			{
				ASSERT(false, "Duplicate position %d", NetPlay.players[i].position);
				NetPlay.players[i].position = MAX_PLAYERS;
			}
		}
	}
	int pos = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (NetPlay.players[i].position >= MAX_PLAYERS)
		{
			while ((havePosition & (PlayerMask(1) << pos)) != 0)
			{
				++pos;
			}
			NetPlay.players[i].position = pos;
			++pos;
		}
	}
}

/*
 * Process click events on the multiplayer/skirmish options screen
 * 'id' is id of the button that was pressed
 */
static void processMultiopWidgets(UDWORD id)
{
	PLAYERSTATS playerStats;

	// host, who is setting up the game
	if (ingame.bHostSetup)
	{
		switch (id)												// Options buttons
		{
		case MULTIOP_GNAME:										// we get this when nec.
			sstrcpy(game.name, widgGetString(psWScreen, MULTIOP_GNAME));
			removeWildcards(game.name);
			widgSetString(psWScreen, MULTIOP_GNAME, game.name);
			
			if (NetPlay.isHost && NetPlay.bComms)
			{
				NETsetLobbyOptField(game.name, NET_LOBBY_OPT_FIELD::GNAME);
				sendOptions();
				NETregisterServer(WZ_SERVER_UPDATE);
			
				addConsoleMessage(_("Game Name Updated."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			}
			break;

		case MULTIOP_GNAME_ICON:
			break;

		case MULTIOP_MAP:
			widgDelete(psWScreen, MULTIOP_PLAYERS);
			widgDelete(psWScreen, FRONTEND_SIDETEXT2);  // del text too,

			debug(LOG_WZ, "processMultiopWidgets[MULTIOP_MAP_ICON]: %s.wrf", MultiCustomMapsPath);
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 0, widgGetString(psWScreen, MULTIOP_MAP));

			widgSetString(psWScreen, MULTIOP_MAP + 1 , game.map); //What a horrible hack! FIX ME! (See addBlueForm())
			widgReveal(psWScreen, MULTIOP_MAP_MOD);
			break;

		case MULTIOP_MAP_ICON:
			widgDelete(psWScreen, MULTIOP_PLAYERS);
			widgDelete(psWScreen, FRONTEND_SIDETEXT2);					// del text too,

			debug(LOG_WZ, "processMultiopWidgets[MULTIOP_MAP_ICON]: %s.wrf", MultiCustomMapsPath);
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, current_numplayers);

			if (NetPlay.isHost && bHosted && NetPlay.bComms)
			{
				sendOptions();
			
				NETsetLobbyOptField(game.map, NET_LOBBY_OPT_FIELD::MAPNAME);
				NETregisterServer(WZ_SERVER_UPDATE);
			}
			break;

		case MULTIOP_MAP_PREVIEW:
			loadMapPreview(true);
			break;
		}
	}

	// host who is setting up or has hosted
	if (ingame.bHostSetup) // || NetPlay.isHost) // FIXME Was: if(ingame.bHostSetup);{} ??? Note the ; !
	{
		switch (id)
		{
		case MULTIOP_GAMETYPE:
			game.scavengers = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_GAMETYPE))->currentValue();
			resetReadyStatus(false);
			if (bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_BASETYPE:
			game.base = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_BASETYPE))->currentValue();
			addGameOptions();

			resetReadyStatus(false);

			if (bHosted)
			{
				sendOptions();
				disableMultiButs();
			}
			break;

		case MULTIOP_ALLIANCES:
			game.alliance = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_ALLIANCES))->currentValue();

			resetReadyStatus(false);
			netPlayersUpdated = true;

			if (bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_POWER:  // set power level
			game.power = ((MultichoiceWidget *)widgGetFromID(psWScreen, MULTIOP_POWER))->currentValue();

			resetReadyStatus(false);

			if (bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_PASSWORD_EDIT:
			{
				unsigned result = widgGetButtonState(psWScreen, MULTIOP_PASSWORD_BUT);
				if (result != 0)
				{
					break;
				}
			}
			// fallthrough
		case MULTIOP_PASSWORD_BUT:
			{
				char buf[255];

				bool willSet = widgGetButtonState(psWScreen, MULTIOP_PASSWORD_BUT) == 0;
				debug(LOG_NET, "Password button hit, %d", (int)willSet);
				widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT,  willSet ? WBUT_CLICKLOCK : 0);
				widgSetButtonState(psWScreen, MULTIOP_PASSWORD_EDIT, willSet ? WEDBS_DISABLE  : 0);
				if (willSet)
				{
					char game_password[64];
					sstrcpy(game_password, widgGetString(psWScreen, MULTIOP_PASSWORD_EDIT));
					NETsetGamePassword(game_password);
					// say password is now required to join games?
					ssprintf(buf, _("*** password [%s] is now required! ***"), NetPlay.gamePassword);
					addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
				}
				else
				{
					NETresetGamePassword();
					ssprintf(buf, "%s", _("*** password is NOT required! ***"));
					addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
				}
				NETGameLocked(willSet);
			}
			break;
		}
	}

	// these work all the time.
	switch (id)
	{
	case MULTIOP_MAP_MOD:
		char buf[256];
		ssprintf(buf, "%s", _("This is a map-mod, it can change your playing experience!"));
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		break;

	case MULTIOP_STRUCTLIMITS:
		changeTitleMode(MULTILIMIT);
		break;

	case MULTIOP_PNAME:
		sstrcpy(sPlayer, widgGetString(psWScreen, MULTIOP_PNAME));

		// chop to 15 chars..
		while (strlen(sPlayer) > 15)	// clip name.
		{
			sPlayer[strlen(sPlayer) - 1] = '\0';
		}
		removeWildcards(sPlayer);
		// update string.
		widgSetString(psWScreen, MULTIOP_PNAME, sPlayer);
		printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);

		NETchangePlayerName(selectedPlayer, (char *)sPlayer);			// update if joined.
		loadMultiStats((char *)sPlayer, &playerStats);
		setMultiStats(selectedPlayer, playerStats, false);
		setMultiStats(selectedPlayer, playerStats, true);
		netPlayersUpdated = true;

		if (NetPlay.isHost && bHosted && NetPlay.bComms)
		{
			sendOptions();
			NETsetLobbyOptField(NetPlay.players[selectedPlayer].name, NET_LOBBY_OPT_FIELD::HOSTNAME);
			NETregisterServer(WZ_SERVER_UPDATE);
		}

		break;

	case MULTIOP_PNAME_ICON:
		widgDelete(psWScreen, MULTIOP_PLAYERS);
		widgDelete(psWScreen, FRONTEND_SIDETEXT2);					// del text too,

		addMultiRequest(MultiPlayersPath, ".sta", MULTIOP_PNAME, 0, 0);
		break;

	case MULTIOP_HOST:
		debug(LOG_NET, "MULTIOP_HOST enabled");
		sstrcpy(game.name, widgGetString(psWScreen, MULTIOP_GNAME));	// game name
		sstrcpy(sPlayer, widgGetString(psWScreen, MULTIOP_PNAME));	// pname

		resetReadyStatus(false);
		resetDataHash();
		removeWildcards((char *)sPlayer);

		if (!hostCampaign((char *)game.name, (char *)sPlayer))
		{
			addConsoleMessage(_("Sorry! Failed to host the game."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			break;
		}
		bHosted = true;
		loadMapSettings2();

		widgDelete(psWScreen, MULTIOP_REFRESH);
		widgDelete(psWScreen, MULTIOP_HOST);
		widgDelete(psWScreen, MULTIOP_FILTER_TOGGLE);

		ingame.localOptionsReceived = true;

		addGameOptions();									// update game options box.
		addChatBox();

		disableMultiButs();

		addPlayerBox(!ingame.bHostSetup || bHosted);	//to make sure host can't skip player selection menu (sets game.skdiff to UBYTE_MAX for humans)
		break;

	case MULTIOP_CHATEDIT:

		// don't send empty lines to other players in the lobby
		if (!strcmp(widgGetString(psWScreen, MULTIOP_CHATEDIT), ""))
		{
			break;
		}

		sendTextMessage(widgGetString(psWScreen, MULTIOP_CHATEDIT), true);					//send
		widgSetString(psWScreen, MULTIOP_CHATEDIT, "");										// clear box
		break;

	case CON_CANCEL:
		pie_LoadBackDrop(SCREEN_RANDOMBDROP);
		if (!challengeActive)
		{
			NETGameLocked(false);		// reset status on a cancel
			stopJoining();
		}
		else
		{
			NETclose();
			bHosted = false;
			ingame.localJoiningInProgress = false;
			changeTitleMode(SINGLE);
			addChallenges();
		}
		break;
	case MULTIOP_MAP_PREVIEW:
		loadMapPreview(true);
		break;
	case MULTIOP_AI_CLOSED:
		NetPlay.players[aiChooserUp].ai = AI_CLOSED;
		break;
	case MULTIOP_AI_OPEN:
		NetPlay.players[aiChooserUp].ai = AI_OPEN;
		break;
	default:
		break;
	}

	if (id == MULTIOP_AI_CLOSED || id == MULTIOP_AI_OPEN)		// common code
	{
		game.skDiff[aiChooserUp] = 0;   // disable AI for this slot
		NETBroadcastPlayerInfo(aiChooserUp);
		closeAiChooser();
		addPlayerBox(!ingame.bHostSetup || bHosted);
	}

	if (id >= MULTIOP_DIFFICULTY_CHOOSE_START && id <= MULTIOP_DIFFICULTY_CHOOSE_END && difficultyChooserUp != -1)
	{
		int idx = id - MULTIOP_DIFFICULTY_CHOOSE_START;
		NetPlay.players[difficultyChooserUp].difficulty = idx;
		game.skDiff[difficultyChooserUp] = difficultyValue[idx];
		NETBroadcastPlayerInfo(difficultyChooserUp);
		closeDifficultyChooser();
		addPlayerBox(!ingame.bHostSetup || bHosted);
	}

	if (id >= MULTIOP_AI_START && id <= MULTIOP_AI_END && aiChooserUp != -1)
	{
		int idx = id - MULTIOP_AI_START;
		NetPlay.players[aiChooserUp].ai = idx;
		sstrcpy(NetPlay.players[aiChooserUp].name, getAIName(aiChooserUp));
		game.skDiff[aiChooserUp] = difficultyValue[NetPlay.players[aiChooserUp].difficulty];    // set difficulty, in case re-enabled
		NETBroadcastPlayerInfo(aiChooserUp);
		closeAiChooser();
		addPlayerBox(!ingame.bHostSetup || bHosted);
	}

	STATIC_ASSERT(MULTIOP_TEAMS_START + MAX_PLAYERS - 1 <= MULTIOP_TEAMS_END);
	if (id >= MULTIOP_TEAMS_START && id <= MULTIOP_TEAMS_START + MAX_PLAYERS - 1 && !locked.teams)  // Clicked on a team chooser
	{
		int clickedMenuID = id - MULTIOP_TEAMS_START;

		//make sure team chooser is not up before adding new one for another player
		if (teamChooserUp < 0 && colourChooserUp < 0 && canChooseTeamFor(clickedMenuID) && positionChooserUp < 0)
		{
			addTeamChooser(clickedMenuID);
		}
	}

	//clicked on a team
	STATIC_ASSERT(MULTIOP_TEAMCHOOSER + MAX_PLAYERS - 1 <= MULTIOP_TEAMCHOOSER_END);
	if (id >= MULTIOP_TEAMCHOOSER && id <= MULTIOP_TEAMCHOOSER + MAX_PLAYERS - 1)
	{
		ASSERT(teamChooserUp >= 0, "teamChooserUp < 0");
		ASSERT(id >= MULTIOP_TEAMCHOOSER
		       && (id - MULTIOP_TEAMCHOOSER) < MAX_PLAYERS, "processMultiopWidgets: wrong id - MULTIOP_TEAMCHOOSER value (%d)", id - MULTIOP_TEAMCHOOSER);

		resetReadyStatus(false);		// will reset only locally if not a host

		SendTeamRequest(teamChooserUp, (UBYTE)id - MULTIOP_TEAMCHOOSER);

		debug(LOG_WZ, "Changed team for player %d to %d", teamChooserUp, NetPlay.players[teamChooserUp].team);

		closeTeamChooser();
		addPlayerBox(!ingame.bHostSetup || bHosted);	//restore initial options screen
	}

	// 'ready' button
	if (id >= MULTIOP_READY_START && id <= MULTIOP_READY_END) // clicked on a player
	{
		UBYTE player = (UBYTE)(id - MULTIOP_READY_START);

		if (player == selectedPlayer && teamChooserUp < 0 && positionChooserUp < 0)
		{
			SendReadyRequest(selectedPlayer, !NetPlay.players[player].ready);

			// if hosting try to start the game if everyone is ready
			if (NetPlay.isHost && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
				// reset flag in case people dropped/quit on join screen
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);
			}
		}

		if (NetPlay.isHost && !alliancesSetTeamsBeforeGame(game.alliance))
		{
			if (mouseDown(MOUSE_RMB) && player != NetPlay.hostPlayer) // both buttons....
			{
				char *msg;

				sasprintf(&msg, _("The host has kicked %s from the game!"), getPlayerName(player));
				sendTextMessage(msg, true);
				kickPlayer(player, "you are unwanted by the host.", ERROR_KICKED);
				resetReadyStatus(true);		//reset and send notification to all clients
			}
		}
	}

	if (id >= MULTIOP_COLOUR_START && id <= MULTIOP_COLOUR_END && (id - MULTIOP_COLOUR_START == selectedPlayer || NetPlay.isHost))
	{
		if (teamChooserUp < 0 && positionChooserUp < 0 && colourChooserUp < 0)		// not choosing something else already
		{
			addColourChooser(id - MULTIOP_COLOUR_START);
		}
	}

	// clicked on a player
	STATIC_ASSERT(MULTIOP_PLAYER_START + MAX_PLAYERS - 1 <= MULTIOP_PLAYER_END);
	if (id >= MULTIOP_PLAYER_START && id <= MULTIOP_PLAYER_START + MAX_PLAYERS - 1
	    && !locked.position
	    && (id - MULTIOP_PLAYER_START == selectedPlayer || NetPlay.isHost
	        || (positionChooserUp >= 0 && !isHumanPlayer(id - MULTIOP_PLAYER_START))))
	{
		int player = id - MULTIOP_PLAYER_START;
		if ((player == selectedPlayer || (NetPlay.players[player].allocated && NetPlay.isHost))
		    && positionChooserUp < 0 && teamChooserUp < 0 && colourChooserUp < 0)
		{
			addPositionChooser(player);
		}
		else if (positionChooserUp == player)
		{
			closePositionChooser();	// changed his mind
			addPlayerBox(!ingame.bHostSetup || bHosted);
		}
		else if (positionChooserUp >= 0)
		{
			// Switch player
			resetReadyStatus(false);		// will reset only locally if not a host
			SendPositionRequest(positionChooserUp, NetPlay.players[player].position);
			closePositionChooser();
			addPlayerBox(!ingame.bHostSetup || bHosted);
		}
		else if (!NetPlay.players[player].allocated && !locked.ai && NetPlay.isHost
		         && positionChooserUp < 0 && teamChooserUp < 0 && colourChooserUp < 0)
		{
			addAiChooser(player);
		}
	}

	if (id >= MULTIOP_DIFFICULTY_INIT_START && id <= MULTIOP_DIFFICULTY_INIT_END
	    && !locked.difficulty && NetPlay.isHost && positionChooserUp < 0 && teamChooserUp < 0 && colourChooserUp < 0)
	{
		addDifficultyChooser(id - MULTIOP_DIFFICULTY_INIT_START);
		addPlayerBox(!ingame.bHostSetup || bHosted);
	}

	STATIC_ASSERT(MULTIOP_COLCHOOSER + MAX_PLAYERS - 1 <= MULTIOP_COLCHOOSER_END);
	if (id >= MULTIOP_COLCHOOSER && id < MULTIOP_COLCHOOSER + MAX_PLAYERS - 1)  // chose a new colour.
	{
		resetReadyStatus(false);		// will reset only locally if not a host
		SendColourRequest(colourChooserUp, id - MULTIOP_COLCHOOSER);
		closeColourChooser();
		addPlayerBox(!ingame.bHostSetup || bHosted);
	}

	if (id == MULTIOP_TEAMCHOOSER_KICK)
	{
		char *msg;

		sasprintf(&msg, _("The host has kicked %s from the game!"), getPlayerName(teamChooserUp));
		kickPlayer(teamChooserUp, "you are unwanted by the host.", ERROR_KICKED);
		sendTextMessage(msg, true);
		resetReadyStatus(true);		//reset and send notification to all clients
		closeTeamChooser();
	}
}

/* Start a multiplayer or skirmish game */
void startMultiplayerGame()
{
	if (!bHosted)
	{
		debug(LOG_ERROR, "Multiple start requests received when we already started.");
		return;
	}
	decideWRF();										// set up swrf & game.map
	bMultiPlayer = true;
	bMultiMessages = true;
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);  // reset disconnect conditions
	initLoadingScreen(true);
	if (NetPlay.isHost)
	{
		// This sets the limits to whatever the defaults are for the limiter screen
		// If host sets limits, then we do not need to do the following routine.
		if (!bLimiterLoaded)
		{
			debug(LOG_NET, "limiter was NOT activated, setting defaults");

			if (!resLoad("wrf/limiter_data.wrf", 503))
			{
				debug(LOG_INFO, "Unable to load limiter_data.");
			}
		}
		else
		{
			debug(LOG_NET, "limiter was activated");
		}

		resetDataHash();	// need to reset it, since host's data has changed.
		createLimitSet();
		debug(LOG_NET, "sending our options to all clients");
		sendOptions();
		NEThaltJoining();							// stop new players entering.
		ingame.TimeEveryoneIsInGame = 0;
		ingame.isAllPlayersDataOK = false;
		memset(&ingame.DataIntegrity, 0x0, sizeof(ingame.DataIntegrity));	//clear all player's array
		SendFireUp();								//bcast a fireup message
	}

	debug(LOG_NET, "title mode STARTGAME is set--Starting Game!");
	changeTitleMode(STARTGAME);

	bHosted = false;

	if (NetPlay.isHost)
	{
		sendTextMessage(_("Host is Starting Game"), true);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Net message handling

void frontendMultiMessages()
{
	NETQUEUE queue;
	uint8_t type;

	while (NETrecvNet(&queue, &type))
	{
		// Copy the message to the global one used by the new NET API
		switch (type)
		{
		case NET_FILE_REQUESTED:
			recvMapFileRequested(queue);
			break;

		case NET_FILE_PAYLOAD:
			{
				bool done = recvMapFileData(queue);
				((MultibuttonWidget *)widgGetFromID(psWScreen, MULTIOP_MAP_PREVIEW))->enable(done);  // turn preview button on or off
				break;
			}

		case NET_FILE_CANCELLED:					// host only routine
			{
				if (!NetPlay.isHost)				// only host should act
				{
					ASSERT(false, "Host only routine detected for client!");
					break;
				}

				Sha256 hash;
				hash.setZero();

				NETbeginDecode(queue, NET_FILE_CANCELLED);
				NETbin(hash.bytes, hash.Bytes);
				NETend();

				debug(LOG_WARNING, "Received file cancel request from player %u, they weren't expecting the file.", queue.index);
				auto &wzFiles = NetPlay.players[queue.index].wzFiles;
				wzFiles.erase(std::remove_if(wzFiles.begin(), wzFiles.end(), [&](WZFile const &file) { return file.hash == hash; }), wzFiles.end());
			}
			break;

		case NET_OPTIONS:					// incoming options file.
			recvOptions(queue);
			ingame.localOptionsReceived = true;

			if (titleMode == MULTIOPTION)
			{
				addGameOptions();
				disableMultiButs();
				addChatBox();
			}
			break;

		case GAME_ALLIANCE:
			recvAlliance(queue, false);
			break;

		case NET_COLOURREQUEST:
			recvColourRequest(queue);
			break;

		case NET_POSITIONREQUEST:
			recvPositionRequest(queue);
			break;

		case NET_TEAMREQUEST:
			recvTeamRequest(queue);
			break;

		case NET_READY_REQUEST:
			recvReadyRequest(queue);

			// If hosting and game not yet started, try to start the game if everyone is ready.
			if (NetPlay.isHost && bHosted && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
			}
			break;

		case NET_PING:						// diagnostic ping msg.
			recvPing(queue);
			break;

		case NET_PLAYER_DROPPED:		// remote player got disconnected
			{
				uint32_t player_id = MAX_PLAYERS;

				resetReadyStatus(false);

				NETbeginDecode(queue, NET_PLAYER_DROPPED);
				{
					NETuint32_t(&player_id);
				}
				NETend();

				if (player_id >= MAX_PLAYERS)
				{
					debug(LOG_INFO, "** player %u has dropped - huh?", player_id);
					break;
				}

				if (whosResponsible(player_id) != queue.index && queue.index != NET_HOST_ONLY)
				{
					HandleBadParam("NET_PLAYER_DROPPED given incorrect params.", player_id, queue.index);
					break;
				}

				debug(LOG_INFO, "** player %u has dropped!", player_id);

				MultiPlayerLeave(player_id);		// get rid of their stuff
				NET_InitPlayer(player_id, false);           // sets index player's array to false
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, player_id);
				if (player_id == NetPlay.hostPlayer || player_id == selectedPlayer)	// if host quits or we quit, abort out
				{
					stopJoining();
				}
				break;
			}
		case NET_PLAYERRESPONDING:			// remote player is now playing.
			{
				uint32_t player_id;

				resetReadyStatus(false);

				NETbeginDecode(queue, NET_PLAYERRESPONDING);
				// the player that has just responded
				NETuint32_t(&player_id);
				NETend();

				ingame.JoiningInProgress[player_id] = false;
				ingame.DataIntegrity[player_id] = false;
				break;
			}
		case NET_FIREUP:					// campaign game started.. can fire the whole shebang up...
			if (NET_HOST_ONLY != queue.index)
			{
				HandleBadParam("NET_FIREUP given incorrect params.", 255, queue.index);
				break;
			}
			debug(LOG_NET, "NET_FIREUP was received ...");
			if (ingame.localOptionsReceived)
			{
				uint32_t randomSeed = 0;
				NETbeginDecode(queue, NET_FIREUP);
				NETuint32_t(&randomSeed);
				NETend();

				gameSRand(randomSeed);  // Set the seed for the synchronised random number generator, using the seed given by the host.

				debug(LOG_NET, "& local Options Received (MP game)");
				ingame.TimeEveryoneIsInGame = 0;			// reset time
				resetDataHash();
				decideWRF();

				bMultiPlayer = true;
				bMultiMessages = true;
				changeTitleMode(STARTGAME);
				bHosted = false;

				// Start the game before processing more messages.
				NETpop(queue);
				return;
			}
			ASSERT(false, "NET_FIREUP was received, but !ingame.localOptionsReceived.");
			break;

		case NET_KICK:						// player is forcing someone to leave
			{
				uint32_t player_id;
				char reason[MAX_KICK_REASON];
				LOBBY_ERROR_TYPES KICK_TYPE = ERROR_NOERROR;

				NETbeginDecode(queue, NET_KICK);
				NETuint32_t(&player_id);
				NETstring(reason, MAX_KICK_REASON);
				NETenum(&KICK_TYPE);
				NETend();

				if (player_id == NET_HOST_ONLY)
				{
					char buf[250] = {'\0'};

					ssprintf(buf, "*Player %d (%s : %s) tried to kick %u", (int) queue.index, NetPlay.players[queue.index].name, NetPlay.players[queue.index].IPtextAddress, player_id);
					NETlogEntry(buf, SYNC_FLAG, 0);
					debug(LOG_ERROR, "%s", buf);
					if (NetPlay.isHost)
					{
						NETplayerKicked((unsigned int) queue.index);
					}
					break;
				}

				if (selectedPlayer == player_id)	// we've been told to leave.
				{
					setLobbyError(KICK_TYPE);
					stopJoining();
					//screen_RestartBackDrop();
					changeTitleMode(GAMEFIND);
					pie_LoadBackDrop(SCREEN_RANDOMBDROP);
					debug(LOG_ERROR, "You have been kicked, because %s ", reason);
				}
				else
				{
					NETplayerKicked(player_id);
				}
				break;
			}
		case NET_HOST_DROPPED:
			NETbeginDecode(queue, NET_HOST_DROPPED);
			NETend();
			stopJoining();
			debug(LOG_NET, "The host has quit!");
			setLobbyError(ERROR_HOSTDROPPED);
			changeTitleMode(GAMEFIND);
			break;

		case NET_TEXTMSG:					// Chat message
			if (ingame.localOptionsReceived)
			{
				recvTextMessage(queue);
			}
			break;

		default:
			debug(LOG_ERROR, "Didn't handle %s message!", messageTypeToString(type));
			break;
		}

		NETpop(queue);
	}
}

void runMultiOptions()
{
	static UDWORD	lastrefresh = 0;
	char                    oldGameMap[128];
	int                     oldMaxPlayers;
	PLAYERSTATS		playerStats;
	W_CONTEXT		context;

	frontendMultiMessages();
	if (NetPlay.isHost)
	{
		// send it for each player that needs it
		sendMap();
	}

	// update boxes?
	if (netPlayersUpdated || (NetPlay.isHost && mouseDown(MOUSE_LMB) && gameTime - lastrefresh > 500))
	{
		netPlayersUpdated = false;
		lastrefresh = gameTime;
		if (!multiRequestUp && (bHosted || ingame.localJoiningInProgress))
		{
			addPlayerBox(true);				// update the player box.
			loadMapPreview(false);
		}
	}


	// update scores and pings if far enough into the game
	if (ingame.localOptionsReceived && ingame.localJoiningInProgress)
	{
		sendScoreCheck();
		sendPing();
	}

	// if we don't have the focus, then autoclick in the chatbox.
	if (!psWScreen->psFocus)
	{
		context.xOffset = 	context.yOffset = 0;
		context.mx			= mouseX();
		context.my			= mouseY();

		if (widgGetFromID(psWScreen, MULTIOP_CHATEDIT))
		{
			widgGetFromID(psWScreen, MULTIOP_CHATEDIT)->clicked(&context);
		}
	}

	// chat box handling
	if (widgGetFromID(psWScreen, MULTIOP_CHATBOX))
	{
		while (getNumberConsoleMessages() > getConsoleLineInfo())
		{
			removeTopConsoleMessage();
		}
		updateConsoleMessages();								// run the chatbox
	}

	// widget handling

	if (multiRequestUp)
	{
		WidgetTriggers const &triggers = widgRunScreen(psRScreen);
		unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

		LEVEL_DATASET *mapData;
		bool isHoverPreview;
		WzString sTemp;
		if (runMultiRequester(id, &id, &sTemp, &mapData, &isHoverPreview))
		{
			Sha256 oldGameHash;

			switch (id)
			{
			case MULTIOP_PNAME:
				sstrcpy(sPlayer, sTemp.toUtf8().c_str());
				widgSetString(psWScreen, MULTIOP_PNAME, sTemp.toUtf8().c_str());

				removeWildcards((char *)sPlayer);

				printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);

				NETchangePlayerName(selectedPlayer, (char *)sPlayer);
				loadMultiStats((char *)sPlayer, &playerStats);
				setMultiStats(selectedPlayer, playerStats, false);
				setMultiStats(selectedPlayer, playerStats, true);
				netPlayersUpdated = true;
				if (NetPlay.isHost && bHosted && NetPlay.bComms)
				{
					sendOptions();
					NETsetLobbyOptField(NetPlay.players[selectedPlayer].name, NET_LOBBY_OPT_FIELD::HOSTNAME);
					NETregisterServer(WZ_SERVER_UPDATE);
				}
				break;
			case MULTIOP_MAP:
				{
					if (NetPlay.bComms && bHosted && !isHoverPreview && NET_numHumanPlayers() > mapData->players)
					{
						addConsoleMessage(_("Cannot change to a map with too few slots for all players."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
						break;
					}
					sstrcpy(oldGameMap, game.map);
					oldGameHash = game.hash;
					oldMaxPlayers = game.maxPlayers;

					sstrcpy(game.map, mapData->pName);
					game.hash = levGetFileHash(mapData);
					game.maxPlayers = mapData->players;
					game.isMapMod = CheckForMod(mapData->realFileName);
					loadMapPreview(!isHoverPreview);

					if (isHoverPreview)
					{
						sstrcpy(game.map, oldGameMap);
						game.hash = oldGameHash;
						game.maxPlayers = oldMaxPlayers;
					}
					else
					{
						loadMapSettings1();
					}

					//Reset player slots if it's a smaller map.
					if (NetPlay.isHost && NetPlay.bComms && bHosted && !isHoverPreview && oldMaxPlayers > game.maxPlayers)
					{
						const std::vector<uint8_t> &Humans = NET_getHumanPlayers();

						//This is pretty ugly tbh, but seems like the easiest way to achieve the goal to my tired mind.
						for (uint8_t SlotInc = 0, PlayerInc = 0; SlotInc < game.maxPlayers && PlayerInc < Humans.size(); ++SlotInc, ++PlayerInc)
						{
							changePosition(Humans[PlayerInc], SlotInc);
						}
					}
							
					widgSetString(psWScreen, MULTIOP_MAP + 1, mapData->pName); //What a horrible, horrible way to do this! FIX ME! (See addBlueForm)
					addGameOptions();

					if (NetPlay.isHost && bHosted && NetPlay.bComms && !isHoverPreview)
					{
						sendOptions();
						NETsetLobbyOptField(game.map, NET_LOBBY_OPT_FIELD::MAPNAME);
						NETregisterServer(WZ_SERVER_UPDATE);
					}

					break;
				}
			default:
				loadMapPreview(false);  // Restore the preview of the old map.
				break;
			}
			if (!isHoverPreview)
			{
				addPlayerBox(!ingame.bHostSetup);
			}
		}
	}
	else
	{
		if (hideTime != 0)
		{
			// we abort the 'hidetime' on press of a mouse button.
			if (gameTime - hideTime < MAP_PREVIEW_DISPLAY_TIME && !mousePressed(MOUSE_LMB) && !mousePressed(MOUSE_RMB))
			{
				return;
			}
			inputLoseFocus();	// remove the mousepress from the input stream.
			hideTime = 0;
		}

		WidgetTriggers const &triggers = widgRunScreen(psWScreen);
		unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

		processMultiopWidgets(id);
	}

	widgDisplayScreen(psWScreen);									// show the widgets currently running

	if (multiRequestUp)
	{
		widgDisplayScreen(psRScreen);								// show the Requester running
	}

	if (widgGetFromID(psWScreen, MULTIOP_CHATBOX))
	{
		displayConsoleMessages();									// draw the chatbox
	}

	if (CancelPressed())
	{
		processMultiopWidgets(CON_CANCEL);  // "Press" the cancel button to clean up net connections and stuff.
	}
	if (!NetPlay.isHostAlive && !ingame.bHostSetup)
	{
		changeTitleMode(GAMEFIND);
		screen_RestartBackDrop();
	}
}

static bool loadSettings(const WzString &filename)
{
	WzConfig ini(filename, WzConfig::ReadOnlyAndRequired);
	ini.beginGroup("challenge");
	sstrcpy(game.map, ini.value("map", game.map).toString().toUtf8().constData());
	game.hash = levGetMapNameHash(game.map);
	game.maxPlayers = ini.value("maxPlayers", game.maxPlayers).toInt();	// TODO, read from map itself, not here!!
	game.scavengers = ini.value("scavengers", game.scavengers).toBool();
	game.alliance = ini.value("alliances", ALLIANCES_TEAMS).toInt();
	game.power = ini.value("powerLevel", game.power).toInt();
	game.base = ini.value("bases", game.base + 1).toInt() - 1;		// count from 1 like the humans do
	sstrcpy(game.name, ini.value("name").toString().toUtf8().constData());
	locked.position = !ini.value("allowPositionChange", !locked.position).toBool();
	ini.endGroup();
	return true;
}

bool startMultiOptions(bool bReenter)
{
	PLAYERSTATS		nullStats;
	UBYTE i;

	netPlayersUpdated = true;

	addBackdrop();
	loadMapPreview(false);
	addTopForm();

	if (getLobbyError() != ERROR_INVALID)
	{
		setLobbyError(ERROR_NOERROR);
	}

	// free limiter structure
	if (!bReenter || challengeActive)
	{
		if (ingame.numStructureLimits)
		{
			ingame.numStructureLimits = 0;
			free(ingame.pStructureLimits);
			ingame.pStructureLimits = nullptr;
		}
	}

	if (!bReenter)
	{
		memset(&locked, 0, sizeof(locked)); // nothing is locked by default

		teamChooserUp = -1;
		aiChooserUp = -1;
		difficultyChooserUp = -1;
		positionChooserUp = -1;
		colourChooserUp = -1;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			game.skDiff[i] = (DIFF_SLIDER_STOPS / 2);	// reset AI (turn it on again)
			setPlayerColour(i, i);						//reset all colors as well
		}
		game.isMapMod = false;			// reset map-mod status
		game.mapHasScavengers = true; // FIXME, should default to false
		if (!NetPlay.bComms)			// force skirmish if no comms.
		{
			game.type = SKIRMISH;
			sstrcpy(game.map, DEFAULTSKIRMISHMAP);
			game.hash = levGetMapNameHash(game.map);
			game.maxPlayers = DEFAULTSKIRMISHMAPMAXPLAYERS;
		}

		ingame.localOptionsReceived = false;

		loadMultiStats((char *)sPlayer, &nullStats);
	}
	if (!bReenter && challengeActive)
	{
		resetReadyStatus(false);
		removeWildcards((char *)sPlayer);
		if (!hostCampaign((char *)game.name, (char *)sPlayer))
		{
			debug(LOG_ERROR, "Failed to host the challenge.");
			return false;
		}
		bHosted = true;

		loadMapSettings1();
		loadMapSettings2();

		loadSettings(sRequestResult);
		netPlayersUpdated = true;

		ingame.localOptionsReceived = true;
		addGameOptions();									// update game options box.
		addChatBox();
		disableMultiButs();
		addPlayerBox(true);
	}
	else
	{
		addPlayerBox(false);								// Players
		addGameOptions();
		addChatBox(bReenter);

		if (ingame.bHostSetup)
		{
			char buf[512] = {'\0'};
			if (NetPlay.bComms)
			{
				if (NetPlay.isUPNP)
				{
					if (NetPlay.isUPNP_CONFIGURED)
					{
						ssprintf(buf, "%s", _("UPnP has been enabled."));
					}
					else
					{
						if (NetPlay.isUPNP_ERROR)
						{
							ssprintf(buf, "%s", _("UPnP detection failed. You must manually configure router yourself."));
						}
						else
						{
							ssprintf(buf, "%s", _("UPnP detection is in progress..."));
						}
					}
					addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
				}
				else
				{
					ssprintf(buf, "%s", _("UPnP detection disabled by user. Autoconfig of port 2100 will not happen."));
					addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
				}
			}
			if (challengeActive)
			{
				ssprintf(buf, "%s", _("Hit the ready box to begin your challenge!"));
			}
			else if (!bHosted)
			{
				ssprintf(buf, "%s", _("Press the start hosting button to begin hosting a game."));
			}
			addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		}
	}
	// going back to multiop after setting limits up..
	if (bReenter && bHosted)
	{
		disableMultiButs();
	}

	loadMapPreview(false);

	if (autogame_enabled())
	{
		if (!ingame.localJoiningInProgress)
		{
			processMultiopWidgets(MULTIOP_HOST);
		}
		SendReadyRequest(selectedPlayer, true);
		if (hostlaunch == 2)
		{
			loadSettings("tests/" + WzString::fromUtf8(wz_skirmish_test()));
			startMultiplayerGame();
			// reset flag in case people dropped/quit on join screen
			NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Drawing functions

void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y() - 4;  // 4 is the magic number.

	// draws the line at the bottom of the multiplayer join dialog separating the chat
	// box from the input box
	iV_Line(x, y, x + psWidget->width(), y, WZCOL_MENU_SEPARATOR);

	return;
}

// ////////////////////////////////////////////////////////////////////////////
void displayRemoteGame(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UDWORD	gameID = psWidget->UserData;
	char tmp[80], name[StringSize];

	// Any widget using displayRemoteGame must have its pUserData initialized to a (DisplayRemoteGameCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayRemoteGameCache& cache = *static_cast<DisplayRemoteGameCache*>(psWidget->pUserData);

	if ((LobbyError != ERROR_NOERROR) && (bMultiPlayer && !NetPlay.bComms))
	{
		addConsoleMessage(_("Can't connect to lobby server!"), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		return;
	}

	// Draw blue boxes for games (buttons) & headers
	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	drawBlueBox(x, y, GAMES_STATUS_START - 4 , psWidget->height());
	drawBlueBox(x, y, GAMES_PLAYERS_START - 4 , psWidget->height());
	drawBlueBox(x, y, GAMES_MAPNAME_START - 4, psWidget->height());

	int lamp = IMAGE_LAMP_RED;
	int statusStart = IMAGE_NOJOIN;
	bool disableButton = true;
	PIELIGHT textColor = WZCOL_TEXT_DARK;

	// As long as they got room, and mods are the same then we process the button(s)
	if (NETisCorrectVersion(NetPlay.games[gameID].game_version_major, NetPlay.games[gameID].game_version_minor))
	{
		if (NetPlay.games[gameID].desc.dwCurrentPlayers >= NetPlay.games[gameID].desc.dwMaxPlayers)
		{
			// If game is full.
			statusStart = IMAGE_NOJOIN_FULL;
		}
		else
		{
			// Game is ok to join!
			textColor = WZCOL_FORM_TEXT;
			statusStart = IMAGE_SKIRMISH_OVER;
			lamp = IMAGE_LAMP_GREEN;
			disableButton = false;

			if (NetPlay.games[gameID].privateGame)  // check to see if it is a private game
			{
				statusStart = IMAGE_LOCKED_NOBG;
				lamp = IMAGE_LAMP_AMBER;
			}
			else if (NetPlay.games[gameID].modlist[0] != '\0')
			{
				statusStart = IMAGE_MOD_OVER;
			}
		}

		ssprintf(tmp, "%d/%d", NetPlay.games[gameID].desc.dwCurrentPlayers, NetPlay.games[gameID].desc.dwMaxPlayers);
		cache.wzText_CurrentVsMaxNumPlayers.setText(tmp, font_regular);
		cache.wzText_CurrentVsMaxNumPlayers.render(x + GAMES_PLAYERS_START + 4 , y + 18, textColor);

		// see what host limits are... then draw them.
		if (NetPlay.games[gameID].limits)
		{
			if (NetPlay.games[gameID].limits & NO_TANKS)
			{
				iV_DrawImage(FrontImages, IMAGE_NO_TANK, x + GAMES_STATUS_START + 37, y + 2);
			}
			if (NetPlay.games[gameID].limits & NO_BORGS)
			{
				iV_DrawImage(FrontImages, IMAGE_NO_CYBORG, x + GAMES_STATUS_START + (37 * 2), y + 2);
			}
			if (NetPlay.games[gameID].limits & NO_VTOLS)
			{
				iV_DrawImage(FrontImages, IMAGE_NO_VTOL, x + GAMES_STATUS_START + (37 * 3) , y + 2);
			}
		}
	}
	// Draw type overlay.
	iV_DrawImage(FrontImages, statusStart, x + GAMES_STATUS_START, y + 3);
	iV_DrawImage(FrontImages, lamp, x - 14, y + 8);
	if (disableButton)
	{
		widgSetButtonState(psWScreen, psWidget->id, WBUT_DISABLE);
	}

	//draw game name, chop when we get a too long name
	sstrcpy(name, NetPlay.games[gameID].name);
	cache.wzText_GameName.setTruncatableText(name, font_regular, (GAMES_MAPNAME_START - GAMES_GAMENAME_START - 4));
	cache.wzText_GameName.render(x + GAMES_GAMENAME_START, y + 12, textColor);

	if (NetPlay.games[gameID].pureMap)
	{
		textColor = WZCOL_RED;
	}
	else
	{
		textColor = WZCOL_FORM_TEXT;
	}
	// draw map name, chop when we get a too long name
	sstrcpy(name, NetPlay.games[gameID].mapname);
	cache.wzText_MapName.setTruncatableText(name, font_regular, (GAMES_PLAYERS_START - GAMES_MAPNAME_START - 4));
	cache.wzText_MapName.render(x + GAMES_MAPNAME_START, y + 12, textColor);

	textColor = WZCOL_FORM_TEXT;
	// draw mod name (if any)
	if (strlen(NetPlay.games[gameID].modlist))
	{
		// FIXME: we really don't have enough space to list all mods
		char tmp[300];
		sprintf(tmp, _("Mods: %s"), NetPlay.games[gameID].modlist);
		tmp[StringSize] = '\0';
		sstrcpy(name, tmp);
	}
	else
	{
		sstrcpy(name, _("Mods: None!"));
	}
	cache.wzText_ModNames.setTruncatableText(name, font_small, (GAMES_PLAYERS_START - GAMES_MAPNAME_START - 8));
	cache.wzText_ModNames.render(x + GAMES_MODNAME_START, y + 24, textColor);

	// draw version string
	sprintf(name, _("Version: %s"), NetPlay.games[gameID].versionstring);
	cache.wzText_VersionString.setTruncatableText(name, font_small, (GAMES_MAPNAME_START - 6 - GAMES_GAMENAME_START - 4));
	cache.wzText_VersionString.render(x + GAMES_GAMENAME_START + 6, y + 24, textColor);

	// crappy hack to only draw this once for the header.  TODO fix GUI
	if (gameID == 0)
	{
		// make the 'header' for the table...
		drawBlueBox(x , y - 12 , GAMES_GAMEWIDTH, 12);

		remoteGameListHeaderCache.wzHeaderText_GameName.setText(_("Game Name"), font_small);
		remoteGameListHeaderCache.wzHeaderText_GameName.render(x - 2 + GAMES_GAMENAME_START + 48, y - 3, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_MapName.setText(_("Map Name"), font_small);
		remoteGameListHeaderCache.wzHeaderText_MapName.render(x - 2 + GAMES_MAPNAME_START + 48, y - 3, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_Players.setText(_("Players"), font_small);
		remoteGameListHeaderCache.wzHeaderText_Players.render(x - 2 + GAMES_PLAYERS_START, y - 3, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_Status.setText(_("Status"), font_small);
		remoteGameListHeaderCache.wzHeaderText_Status.render(x - 2 + GAMES_STATUS_START + 48, y - 3, WZCOL_YELLOW);
	}
}

void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UDWORD		i = psWidget->UserData;

	ASSERT_OR_RETURN(, i < MAX_PLAYERS && NetPlay.players[i].team >= 0 && NetPlay.players[i].team < MAX_PLAYERS, "Team index out of bounds");

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	if (game.skDiff[i])
	{
		iV_DrawImage(FrontImages, IMAGE_TEAM0 + NetPlay.players[i].team, x + 2, y + 8);
	}
}

void displayPosition(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayPosition must have its pUserData initialized to a (DisplayPositionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayPositionCache& cache = *static_cast<DisplayPositionCache *>(psWidget->pUserData);

	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int i = psWidget->UserData;
	char text[80];

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	ssprintf(text, _("Click to take player slot %d"), NetPlay.players[i].position);
	cache.wzPositionText.setText(text, font_regular);
	cache.wzPositionText.render(x + 10, y + 22, WZCOL_FORM_TEXT);
}

static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayAi must have its pUserData initialized to a (DisplayAICache*)
	assert(psWidget->pUserData != nullptr);
	DisplayAICache& cache = *static_cast<DisplayAICache *>(psWidget->pUserData);

	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;
	const char *commsText[] = { N_("Open"), N_("Closed") };

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	cache.wzText.setText((j >= 0) ? aidata[j].name : gettext(commsText[j + 2]), font_regular);
	cache.wzText.render(x + 10, y + 22, WZCOL_FORM_TEXT);
}

static int difficultyIcon(int difficulty)
{
	switch (difficulty)
	{
	case 0: return IMAGE_EASY;
	case 1: return IMAGE_MEDIUM;
	case 2: return IMAGE_HARD;
	case 3: return IMAGE_INSANE;
	default: return IMAGE_NO;	/// what??
	}
}

static void displayDifficulty(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayDifficulty must have its pUserData initialized to a (DisplayDifficultyCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayDifficultyCache& cache = *static_cast<DisplayDifficultyCache *>(psWidget->pUserData);

	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	ASSERT_OR_RETURN(, j < ARRAY_SIZE(difficultyList), "Bad difficulty found: %d", j);
	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	iV_DrawImage(FrontImages, difficultyIcon(j), x + 5, y + 5);
	cache.wzDifficultyText.setText(gettext(difficultyList[j]), font_regular);
	cache.wzDifficultyText.render(x + 42, y + 22, WZCOL_FORM_TEXT);
}

static bool isKnownPlayer(std::map<std::string, EcKey::Key> const &knownPlayers, std::string const &name, EcKey const &key)
{
	if (key.empty())
	{
		return false;
	}
	std::map<std::string, EcKey::Key>::const_iterator i = knownPlayers.find(name);
	return i != knownPlayers.end() && key.toBytes(EcKey::Public) == i->second;
}

// ////////////////////////////////////////////////////////////////////////////
void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayPlayer must have its pUserData initialized to a (DisplayPlayerCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayPlayerCache& cache = *static_cast<DisplayPlayerCache *>(psWidget->pUserData);

	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UDWORD		j = psWidget->UserData, eval;

	const int nameX = 32;

	int downloadProgress = NETgetDownloadProgress(j);

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	if (downloadProgress != 100)
	{
		char progressString[MAX_STR_LENGTH];
		ssprintf(progressString, j != selectedPlayer ? _("Sending Map: %d%% ") : _("Map: %d%% downloaded"), downloadProgress);
		cache.wzMainText.setText(progressString, font_regular);
		cache.wzMainText.render(x + 5, y + 22, WZCOL_FORM_TEXT);
		return;
	}
	else if (ingame.localOptionsReceived && NetPlay.players[j].allocated)					// only draw if real player!
	{
		std::string name = NetPlay.players[j].name;

		drawBlueBox(x, y, psWidget->width(), psWidget->height());

		std::map<std::string, EcKey::Key> serverPlayers;  // TODO Fill this with players known to the server (needs implementing on the server, too). Currently useless.

		PIELIGHT colour;
		if (ingame.PingTimes[j] >= PING_LIMIT)
		{
			colour = WZCOL_FORM_PLAYER_NOPING;
		}
		else if (isKnownPlayer(serverPlayers, name, getMultiStats(j).identity))
		{
			colour = WZCOL_FORM_PLAYER_KNOWN_BY_SERVER;
		}
		else if (isKnownPlayer(getKnownPlayers(), name, getMultiStats(j).identity))
		{
			colour = WZCOL_FORM_PLAYER_KNOWN;
		}
		else
		{
			colour = WZCOL_FORM_PLAYER_UNKNOWN;
		}

		// name
		if (cache.fullMainText != name)
		{
			if ((int)iV_GetTextWidth(name.c_str(), font_regular) > psWidget->width() - nameX)
			{
				while (!name.empty() && (int)iV_GetTextWidth((name + "...").c_str(), font_regular) > psWidget->width() - nameX)
				{
					name.resize(name.size() - 1);  // Clip name.
				}
				name += "...";
			}
			cache.wzMainText.setText(name, font_regular);
			cache.fullMainText = name;
		}
		std::string subText;
		if (j == NET_HOST_ONLY && NetPlay.bComms)
		{
			subText += _("HOST");
		}
		if (NetPlay.bComms && j != selectedPlayer)
		{
			char buf[250] = {'\0'};

			// show "actual" ping time
			ssprintf(buf, "%s%s: ", subText.empty() ? "" : ", ", _("Ping"));
			subText += buf;
			if (ingame.PingTimes[j] < PING_LIMIT)
			{
				ssprintf(buf, "%03d", ingame.PingTimes[j]);
			}
			else
			{
				ssprintf(buf, "%s", "∞");  // Player has ping of somewhat questionable quality.
			}
			subText += buf;
		}
		cache.wzMainText.render(x + nameX, y + (subText.empty() ? 22 : 18), colour);
		if (!subText.empty())
		{
			cache.wzSubText.setText(subText, font_small);
			cache.wzSubText.render(x + nameX, y + 28, WZCOL_TEXT_MEDIUM);
		}

		PLAYERSTATS stat = getMultiStats(j);
		if (stat.wins + stat.losses < 5)
		{
			iV_DrawImage(FrontImages, IMAGE_MEDAL_DUMMY, x + 4, y + 13);
		}
		else
		{
			PLAYERSTATS stat = getMultiStats(j);

			// star 1 total droid kills
			eval = stat.totalKills;
			if (eval > 600)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK1, x + 4, y + 3);
			}
			else if (eval > 300)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK2, x + 4, y + 3);
			}
			else if (eval > 150)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK3, x + 4, y + 3);
			}

			// star 2 games played (Cannot use stat.played, since that's just the number of times the player exited via the game menu, not the number of games played.)
			eval = stat.wins + stat.losses;
			if (eval > 200)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK1, x + 4, y + 13);
			}
			else if (eval > 100)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK2, x + 4, y + 13);
			}
			else if (eval > 50)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK3, x + 4, y + 13);
			}

			// star 3 games won.
			eval = stat.wins;
			if (eval > 80)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK1, x + 4, y + 23);
			}
			else if (eval > 40)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK2, x + 4, y + 23);
			}
			else if (eval > 10)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK3, x + 4, y + 23);
			}

			// medals.
			if ((stat.wins >= 6) && (stat.wins > (2 * stat.losses))) // bronze requirement.
			{
				if ((stat.wins >= 12) && (stat.wins > (4 * stat.losses))) // silver requirement.
				{
					if ((stat.wins >= 24) && (stat.wins > (8 * stat.losses))) // gold requirement
					{
						iV_DrawImage(FrontImages, IMAGE_MEDAL_GOLD, x + 16, y + 11);
					}
					else
					{
						iV_DrawImage(FrontImages, IMAGE_MEDAL_SILVER, x + 16, y + 11);
					}
				}
				else
				{
					iV_DrawImage(FrontImages, IMAGE_MEDAL_BRONZE, x + 16, y + 11);
				}
			}
		}
		game.skDiff[j] = UBYTE_MAX;	// set AI difficulty to 0xFF (i.e. not an AI)
	}
	else	// AI
	{
		char aitext[80];

		if (NetPlay.players[j].ai >= 0)
		{
			iV_DrawImage(FrontImages, IMAGE_PLAYER_PC, x, y + 11);
		}

		// challenges may use custom AIs that are not in aidata and set to 127
		if (!challengeActive)
		{
		    ASSERT_OR_RETURN(, NetPlay.players[j].ai < (int)aidata.size(), "Uh-oh, AI index out of bounds");
		}

		switch (NetPlay.players[j].ai)
		{
		case AI_OPEN: sstrcpy(aitext, _("Open")); break;
		case AI_CLOSED: sstrcpy(aitext, _("Closed")); break;
		default: sstrcpy(aitext, NetPlay.players[j].name ); break;
		}
		cache.wzMainText.setText(aitext, font_regular);
		cache.wzMainText.render(x + nameX, y + 22, WZCOL_FORM_TEXT);
	}
}

void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	const int x = xOffset + psWidget->x();
	const int y = yOffset + psWidget->y();
	const int j = psWidget->UserData;

	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	if (NetPlay.players[j].wzFiles.empty() && game.skDiff[j])
	{
		int player = getPlayerColour(j);
		STATIC_ASSERT(MAX_PLAYERS <= 16);
		iV_DrawImageTc(FrontImages, IMAGE_PLAYERN, IMAGE_PLAYERN_TC, x + 7, y + 9, pal_GetTeamColour(player));
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Display blue box
void intDisplayFeBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();

	drawBlueBox(x, y, w, h);
}

// ////////////////////////////////////////////////////////////////////////////
// Display edit box
void displayMultiEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	drawBlueBox(x, y, psWidget->width(), psWidget->height());

	if (((W_EDITBOX *)psWidget)->state & WEDBS_DISABLE)					// disabled
	{
		PIELIGHT colour;

		colour.byte.r = FILLRED;
		colour.byte.b = FILLBLUE;
		colour.byte.g = FILLGREEN;
		colour.byte.a = FILLTRANS;
		pie_UniTransBoxFill(x, y, x + psWidget->width() + psWidget->height(), y + psWidget->height(), colour);
	}
}

static Image getFrontHighlightImage(Image image)
{
	if (image.isNull())
	{
		return Image();
	}
	switch (image.width())
	{
	case 30: return Image(FrontImages, IMAGE_HI34);
	case 60: return Image(FrontImages, IMAGE_HI64);
	case 19: return Image(FrontImages, IMAGE_HI23);
	case 27: return Image(FrontImages, IMAGE_HI31);
	case 35: return Image(FrontImages, IMAGE_HI39);
	case 37: return Image(FrontImages, IMAGE_HI41);
	case 56: return Image(FrontImages, IMAGE_HI56);
	}
	return Image();
}

void WzMultiButton::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	Image hiToUse(nullptr, 0);

	// FIXME: This seems to be a way to conserve space, so you can use a
	// transparent icon with these edit boxes.
	// hack for multieditbox
	if (imNormal.id == IMAGE_EDIT_MAP || imNormal.id == IMAGE_EDIT_GAME || imNormal.id == IMAGE_EDIT_PLAYER
	    || imNormal.id == IMAGE_LOCK_BLUE || imNormal.id == IMAGE_UNLOCK_BLUE)
	{
		drawBlueBox(x0 - 2, y0 - 2, height(), height());  // box on end.
	}

	// evaluate auto-frame
	bool highlight = (getState() & WBUT_HIGHLIGHT) != 0;

	// evaluate auto-frame
	if (doHighlight == 1 && highlight)
	{
		hiToUse = getFrontHighlightImage(imNormal);
	}

	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool grey = (getState() & WBUT_DISABLE) != 0;

	Image toDraw[3];
	int numToDraw = 0;

	// now display
	toDraw[numToDraw++] = imNormal;

	// hilights etc..
	if (down)
	{
		toDraw[numToDraw++] = imDown;
	}
	if (highlight && !grey && hiToUse.images != nullptr)
	{
		toDraw[numToDraw++] = hiToUse;
	}

	for (int n = 0; n < numToDraw; ++n)
	{
		Image tcImage(toDraw[n].images, toDraw[n].id + 1);
		if (tc == MAX_PLAYERS)
		{
			iV_DrawImage(toDraw[n], x0, y0);
		}
		else if (tc == MAX_PLAYERS + 1)
		{
			const int scale = 4000;
			int f = realTime % scale;
			PIELIGHT mix;
			mix.byte.r = 128 + iSinR(65536 * f / scale + 65536 * 0 / 3, 127);
			mix.byte.g = 128 + iSinR(65536 * f / scale + 65536 * 1 / 3, 127);
			mix.byte.b = 128 + iSinR(65536 * f / scale + 65536 * 2 / 3, 127);
			mix.byte.a = 255;
			iV_DrawImageTc(toDraw[n], tcImage, x0, y0, mix);
		}
		else
		{
			iV_DrawImageTc(toDraw[n], tcImage, x0, y0, pal_GetTeamColour(tc));
		}
	}

	if (grey)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// common widgets

static bool addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid)
{
	W_EDBINIT sEdInit;                           // editbox
	sEdInit.formID = formid;
	sEdInit.id = id;
	sEdInit.x = (short)x;
	sEdInit.y = (short)y;
	sEdInit.width = MULTIOP_EDITBOXW;
	sEdInit.height = MULTIOP_EDITBOXH;
	sEdInit.pText = tipres;
	sEdInit.pBoxDisplay = displayMultiEditBox;
	if (!widgAddEditBox(psWScreen, &sEdInit))
	{
		return false;
	}

	addMultiBut(psWScreen, MULTIOP_OPTIONS, iconid, x + MULTIOP_EDITBOXW + 2, y + 2, MULTIOP_EDITBOXH, MULTIOP_EDITBOXH, tip, icon, iconhi, iconhi);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool addMultiBut(W_SCREEN *screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char *tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc)
{
	WzMultiButton *button = new WzMultiButton(widgGetFromID(screen, formid));
	button->id = id;
	button->setGeometry(x, y, width, height);
	button->setTip((tipres != nullptr) ? std::string(tipres) : std::string());
	button->imNormal = Image(FrontImages, norm);
	button->imDown = Image(FrontImages, down);
	button->doHighlight = hi;
	button->tc = tc;
	return true;
}

/* Returns true if all human players clicked on the 'ready' button */
bool multiplayPlayersReady(bool bNotifyStatus)
{
	unsigned int	player, playerID;
	bool			bReady;

	bReady = true;

	for (player = 0; player < game.maxPlayers; player++)
	{
		// check if this human player is ready, ignore AIs
		if (NetPlay.players[player].allocated && (!NetPlay.players[player].ready || ingame.PingTimes[player] >= PING_LIMIT))
		{
			if (bNotifyStatus)
			{
				for (playerID = 0; playerID <= game.maxPlayers && playerID != player; playerID++) ;

				console("%s is not ready", getPlayerName(playerID));
			}

			bReady = false;
		}
	}

	return bReady;
}
