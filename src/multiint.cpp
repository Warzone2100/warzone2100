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
/*
 * MultiInt.c
 *
 * Alex Lee, 98. Pumpkin Studios, Bath.
 * Functions to display and handle the multiplayer interface screens,
 * along with connection and game options.
 */

#include "lib/framework/frame.h"

#include <time.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/frameint.h"
#include "lib/framework/file.h"
#include "lib/framework/stdio_ext.h"

/* Includes direct access to render library */
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piematrix.h"			// for setgeometricoffset
#include "lib/ivis_opengl/screen.h"

#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "lib/script/script.h"
#include "lib/widget/editbox.h"
#include "lib/widget/button.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"

#include "lib/iniparser/iniparser.h"

#include "challenge.h"
#include "main.h"
#include "objects.h"
#include "display.h"// pal stuff
#include "display3d.h"
#include "objmem.h"
#include "gateway.h"

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

#define MAP_PREVIEW_DISPLAY_TIME 2500	// number of milliseconds to show map in preview

// ////////////////////////////////////////////////////////////////////////////
// tertile dependant colors for map preview

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

static const unsigned gnImage[] = {IMAGE_GN_0, IMAGE_GN_1, IMAGE_GN_2, IMAGE_GN_3, IMAGE_GN_4, IMAGE_GN_5, IMAGE_GN_6, IMAGE_GN_7, IMAGE_GN_8, IMAGE_GN_9, IMAGE_GN_10, IMAGE_GN_11, IMAGE_GN_12, IMAGE_GN_13, IMAGE_GN_14, IMAGE_GN_15};

// ////////////////////////////////////////////////////////////////////////////
// vars
extern char	MultiCustomMapsPath[PATH_MAX];
extern char	MultiPlayersPath[PATH_MAX];
extern char VersionString[80];		// from netplay.c
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
static SDWORD				dwSelectedGame	=0;						//player[] and games[] indexes
static UDWORD				gameNumber;								// index to games icons
static bool					safeSearch		= false;				// allow auto game finding.
static bool disableLobbyRefresh = false;	// if we allow lobby to be refreshed or not.
static UDWORD hideTime=0;
static bool EnablePasswordPrompt = false;	// if we need the password prompt
LOBBY_ERROR_TYPES LobbyError = ERROR_NOERROR;
static bool allowChangePosition = true;
static char tooltipbuffer[256] ={'\0'};
/// end of globals.
// ////////////////////////////////////////////////////////////////////////////
// Function protos

// widget functions
static bool addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, char const *tip, char const *tipres, UDWORD icon, UDWORD iconhi, UDWORD iconid);
static void addBlueForm					(UDWORD parent,UDWORD id, const char *txt,UDWORD x,UDWORD y,UDWORD w,UDWORD h);
static void drawReadyButton(UDWORD player);
static void displayPasswordEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours);

// Drawing Functions
static void displayChatEdit     (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayMultiBut     (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void intDisplayFeBox     (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayRemoteGame   (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayPlayer       (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayPosition     (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayColour       (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayTeamChooser  (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayAi           (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayDifficulty   (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayMultiEditBox (WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void setLockedTeamsMode  (void);

// find games
static void addGames				(void);
static void removeGames				(void);

// password form functions
static void hidePasswordForm(void);
static void showPasswordForm(void);

// Game option functions
static	void	addGameOptions();
static	void	addChatBox			(void);
static	void	disableMultiButs	(void);
static	void	processMultiopWidgets(UDWORD);
static	void	SendFireUp			(void);

static	void	decideWRF			(void);

static void		closeColourChooser	(void);
static void		closeTeamChooser	(void);
static void		closePositionChooser	(void);
static void		closeAiChooser		(void);
static void		closeDifficultyChooser	(void);
static bool		SendColourRequest	(UBYTE player, UBYTE col);
static bool		SendPositionRequest	(UBYTE player, UBYTE chosenPlayer);
static bool		safeToUseColour		(UDWORD player,UDWORD col);
static bool		changeReadyStatus	(UBYTE player, bool bReady);
static	void stopJoining(void);
static int difficultyIcon(int difficulty);
// ////////////////////////////////////////////////////////////////////////////
// map previews..

static const char *difficultyList[] = { "Easy", "Medium", "Hard", "Insane" };
static const int difficultyValue[] = { 1, 10, 15, 20 };

struct AIDATA
{
	char name[MAX_LEN_AI_NAME];
	char slo[MAX_LEN_AI_NAME];
	char vlo[MAX_LEN_AI_NAME];
	char tip[255];
	int assigned;	///< How many AIs have we assigned of this type
};
static std::vector<AIDATA> aidata;

const char *getAIName(int player)
{
	if (NetPlay.players[player].ai >= 0)
	{
		return aidata[NetPlay.players[player].ai].name;
	}
	else
	{
		return "";
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
				return i;	// found right player
			}
			match--;		// find next of this type
		}
	}
	return AI_NOT_FOUND;
}

void loadMultiScripts()
{
	// TODO: Only load AI scripts (not vals) once.

	// Reset assigned counter
	std::vector<AIDATA>::iterator it;
	for (it = aidata.begin(); it < aidata.end(); it++)
	{
		(*it).assigned = 0;
	}

	// Load multiplayer rules
	resForceBaseDir("messages/");
	resLoadFile("SMSG", "multiplay.txt");
	resForceBaseDir("multiplay/skirmish/");
	resLoadFile("SCRIPT", "rules.slo");
	resLoadFile("SCRIPTVAL", "rules.vlo");

	// Load extra tech levels
	if (current_tech == 2)
	{
		resLoadFile("SCRIPT", "sktech.slo");
		resLoadFile("SCRIPTVAL", "sk2tech.vlo");
	}
	else if (current_tech == 3)
	{
		resLoadFile("SCRIPT", "sktech.slo");
		resLoadFile("SCRIPTVAL", "sk3tech.vlo");
	}

	// Backup data hashes, since AI and scavenger scripts aren't run on all clients.
	uint32_t oldHash1 = DataHash[DATA_SCRIPT];
	uint32_t oldHash2 = DataHash[DATA_SCRIPTVAL];

	// Load AI players
	resForceBaseDir("multiplay/skirmish/");
	for (int i = 0; i < game.maxPlayers; i++)
	{
		// The i == selectedPlayer hack is to enable autogames
		if (bMultiPlayer && game.type == SKIRMISH && (!NetPlay.players[i].allocated || i == selectedPlayer)
		    && NetPlay.players[i].ai >= 0 && myResponsibility(i))
		{
			resLoadFile("SCRIPT", aidata[NetPlay.players[i].ai].slo);
			resLoadFile("SCRIPTVAL", aidata[NetPlay.players[i].ai].vlo);
		}
	}

	// Load scavengers
	resForceBaseDir("multiplay/script/");
	if (game.scavengers && myResponsibility(scavengerPlayer()))
	{
		resLoadFile("SCRIPT", "scavfact.slo");
		resLoadFile("SCRIPTVAL", "scavfact.vlo");
	}

	// Restore data hashes, since AI and scavenger scripts aren't run on all clients.
	DataHash[DATA_SCRIPT]    = oldHash1;  // Not all players load the same AI scripts.
	DataHash[DATA_SCRIPTVAL] = oldHash2;

	// Reset resource path, otherwise things break down the line
	resForceBaseDir("");
}

static int guessMapTilesetType(LEVEL_DATASET *psLevel)
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

/// Loads the entire map just to show a picture of it
void loadMapPreview(bool hideInterface)
{
	static char		aFileName[256];
	UDWORD			fileSize;
	char			*pFileData = NULL;
	LEVEL_DATASET	*psLevel = NULL;
	PIELIGHT		plCliffL, plCliffH, plWater, plRoadL, plRoadH, plGroundL, plGroundH;

	UDWORD			x, y, height;
	UBYTE			col;
	MAPTILE			*psTile,*WTile;
	UDWORD oursize;
	Vector2i playerpos[MAX_PLAYERS];	// Will hold player positions
	char  *ptr = NULL, *imageData = NULL;

	if(psMapTiles)
	{
		mapShutdown();
	}

	// load the terrain types
	psLevel = levFindDataSet(game.map);
	ASSERT_OR_RETURN(, psLevel != NULL, "Could not find level dataset!");
	rebuildSearchPath(psLevel->dataDir, false);
	sstrcpy(aFileName,psLevel->apDataFiles[0]);
	aFileName[strlen(aFileName)-4] = '\0';
	sstrcat(aFileName, "/ttypes.ttp");
	pFileData = fileLoadBuffer;

	if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_ERROR, "Failed to load terrain types file");
		return;
	}
	if (pFileData)
	{
		if (!loadTerrainTypeMap(pFileData, fileSize))
		{
			debug(LOG_ERROR, "Failed to load terrain types");
			return;
		}
	}

	// load the map data
	ptr = strrchr(aFileName, '/');
	ASSERT(ptr, "this string was supposed to contain a /");
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
	imageData = (char*)malloc(oursize * 3);		// used for the texture
	if( !imageData )
	{
		debug(LOG_FATAL,"Out of memory for texture!");
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
			char * const p = imageData + (3 * (y * BACKDROP_HACK_WIDTH + x));
			height = WTile->height / ELEVATION_SCALE;
			col = height;

			switch (terrainType(WTile))
			{
				case TER_CLIFFFACE:
					p[0] = plCliffL.byte.r + (plCliffH.byte.r-plCliffL.byte.r) * col / 256;
					p[1] = plCliffL.byte.g + (plCliffH.byte.g-plCliffL.byte.g) * col / 256;
					p[2] = plCliffL.byte.b + (plCliffH.byte.b-plCliffL.byte.b) * col / 256;
					break;
				case TER_WATER:
					p[0] = plWater.byte.r;
					p[1] = plWater.byte.g;
					p[2] = plWater.byte.b;
					break;
				case TER_ROAD:
					p[0] = plRoadL.byte.r + (plRoadH.byte.r-plRoadL.byte.r) * col / 256;
					p[1] = plRoadL.byte.g + (plRoadH.byte.g-plRoadL.byte.g) * col / 256;
					p[2] = plRoadL.byte.b + (plRoadH.byte.b-plRoadL.byte.b) * col / 256;
					break;
				default:
					p[0] = plGroundL.byte.r + (plGroundH.byte.r-plGroundL.byte.r) * col / 256;
					p[1] = plGroundL.byte.g + (plGroundH.byte.g-plGroundL.byte.g) * col / 256;
					p[2] = plGroundL.byte.b + (plGroundH.byte.b-plGroundL.byte.b) * col / 256;
					break;
			}
			WTile += 1;
		}
		psTile += mapWidth;
	}
	// Slight hack to init array with a special value used to determine how many players on map
	memset(playerpos,0x77,sizeof(playerpos));
	// color our texture with clancolors @ correct position
	plotStructurePreview16(imageData, playerpos);

	screen_enableMapPreview(aFileName, mapWidth, mapHeight, playerpos);

	screen_Upload(imageData, true);

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
	std::vector<AIDATA>::iterator it;
	int i = 0;

	if (name[0] == '\0')
	{
		return AI_CLOSED;
	}
	for (it = aidata.begin(); it < aidata.end(); it++, i++)
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

	strcpy(basepath, sSearchPath);
	files = PHYSFS_enumerateFiles(basepath);
	for (i = files; *i != NULL; ++i)
	{
		char path[PATH_MAX];
		// See if this filename contains the extension we're looking for
		if (!strstr(*i, ".ai"))
		{
			continue;
		}

		sstrcpy(path, basepath);
		sstrcat(path, *i);
		inifile *inif = inifile_load(path);
		if (!inif)
		{
			debug(LOG_ERROR, "Failed to open AI %s", path);
			continue;
		}
		AIDATA ai;
		inifile_set_current_section(inif, "AI");
		sstrcpy(ai.name, inifile_get(inif, "name", "error"));
		sstrcpy(ai.slo, inifile_get(inif, "slo", "error"));
		sstrcpy(ai.vlo, inifile_get(inif, "vlo", "error"));
		sstrcpy(ai.tip, inifile_get(inif, "tip", "Click to choose this AI"));
		if (strcmp(ai.name, "Nexus") == 0)
		{
			std::vector<AIDATA>::iterator it;
			it = aidata.begin();
			aidata.insert(it, ai);
		}
		else
		{
			aidata.push_back(ai);
		}
		inifile_delete(inif);
	}
	PHYSFS_freeList(files);
}

//sets sWRFILE form game.map
static void decideWRF(void)
{
	// try and load it from the maps directory first,
	sstrcpy(aLevelName, MultiCustomMapsPath);
	sstrcat(aLevelName, game.map);
	sstrcat(aLevelName, ".wrf");
	debug(LOG_WZ, "decideWRF: %s", aLevelName);
	//if the file exists in the downloaded maps dir then use that one instead.
	// FIXME: Try to incorporate this into physfs setup somehow for sane paths
	if ( !PHYSFS_exists(aLevelName) )
	{
		sstrcpy(aLevelName, game.map);		// doesn't exist, must be a predefined one.
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Connection Options Screen.

static bool OptionsInet(void)			//internet options
{
	psConScreen = widgCreateScreen();
	widgSetTipFont(psConScreen,font_regular);

	W_FORMINIT sFormInit;           //Connection Settings
	sFormInit.formID = 0;
	sFormInit.id = CON_SETTINGS;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = CON_SETTINGSX;
	sFormInit.y = CON_SETTINGSY;
	sFormInit.width = CON_SETTINGSWIDTH;
	sFormInit.height = CON_SETTINGSHEIGHT;
	sFormInit.pDisplay = intDisplayFeBox;
	widgAddForm(psConScreen, &sFormInit);

	addMultiBut(psConScreen, CON_SETTINGS,CON_OK,CON_OKX,CON_OKY,MULTIOP_OKW,MULTIOP_OKH,
				_("Accept Settings"),IMAGE_OK,IMAGE_OK,true);
	addMultiBut(psConScreen, CON_SETTINGS,CON_IP_CANCEL,CON_OKX+MULTIOP_OKW+10,CON_OKY,MULTIOP_OKW,MULTIOP_OKH,
				_("Cancel"),IMAGE_NO,IMAGE_NO,true);

	//label.
	W_LABINIT sLabInit;
	sLabInit.formID = CON_SETTINGS;
	sLabInit.id		= CON_SETTINGS_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 10;
	sLabInit.width	= CON_SETTINGSWIDTH;
	sLabInit.height = 20;
	sLabInit.pText	= _("IP Address or Machine Name");
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
	sContext.psScreen	= psConScreen;
	sContext.psForm		= (W_FORM *)psConScreen->psForm;
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
bool startConnectionScreen(void)
{
	addBackdrop();										//background
	addTopForm();										// logo
	addBottomForm();

	SettingsUp		= false;
	InitialProto	= 0;
	safeSearch		= false;

	// don't pretend we are running a network game. Really do it!
	NetPlay.bComms = true; // use network = true

	addSideText(FRONTEND_SIDETEXT,  FRONTEND_SIDEX, FRONTEND_SIDEY,_("CONNECTION"));

	addMultiBut(psWScreen,FRONTEND_BOTFORM,CON_CANCEL,10,10,MULTIOP_OKW,MULTIOP_OKH,
		_("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);	// goback buttpn levels

	addTextButton(CON_TYPESID_START+0,FRONTEND_POS2X,FRONTEND_POS2Y, _("Lobby"), WBUT_TXTCENTRE);
	addTextButton(CON_TYPESID_START+1,FRONTEND_POS3X,FRONTEND_POS3Y, _("IP"), WBUT_TXTCENTRE);

	return true;
}

void runConnectionScreen(void)
{
	UDWORD id;
	static char addr[128];

	if(SettingsUp == true)
	{
		id = widgRunScreen(psConScreen);				// Run the current set of widgets
	}
	else
	{
		id = widgRunScreen(psWScreen);					// Run the current set of widgets
	}

	switch(id)
	{
		case CON_CANCEL: //cancel
			changeTitleMode(MULTI);
			bMultiPlayer = false;
			bMultiMessages = false;
			break;
		case CON_TYPESID_START+0: // Lobby button
			if ((LobbyError != ERROR_KICKED) && (LobbyError != ERROR_CHEAT))
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

			if(SettingsUp == true)
			{
				widgReleaseScreen(psConScreen);
				SettingsUp = false;
			}

			joinGame(addr, 0);
			break;
		case CON_IP_CANCEL:
			if (SettingsUp == true)
			{
				widgReleaseScreen(psConScreen);
				SettingsUp = false;
			}
			break;
	}

	widgDisplayScreen(psWScreen);							// show the widgets currently running
	if(SettingsUp == true)
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
LOBBY_ERROR_TYPES getLobbyError(void)
{
	return LobbyError;
}

void setLobbyError (LOBBY_ERROR_TYPES error_type)
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
bool joinGame(const char* host, uint32_t port)
{
	PLAYERSTATS	playerStats;
	LOBBY_ERROR_TYPES oldError;
	UDWORD id;

	if(ingame.localJoiningInProgress)
	{
		return false;
	}

	if (!NETjoinGame(host, port, (char*)sPlayer))	// join
	{
		// Save the error.
		oldError = getLobbyError();
		if (!oldError)
		{
			oldError = ERROR_CONNECTION;
		}
		else if (oldError == ERROR_WRONGPASSWORD)
		{
			// Change to GAMEFIND, screen with a password dialog.
			changeTitleMode(GAMEFIND);
			showPasswordForm();

			id = widgRunScreen(psWScreen); // Run the current set of widgets

			if (id != CON_PASSWORD)
			{
				return false;
			}

			NETsetGamePassword(widgGetString(psWScreen, CON_PASSWORD));
		}

		// Hide a (maybe shown) password form.
		hidePasswordForm();

		// Change to GAMEFIND, screen.
		changeTitleMode(GAMEFIND);

		// Reset the error to the saved one.
		setLobbyError(oldError);

		// Shows the lobby error.
		addGames();

		return false;
	}
	ingame.localJoiningInProgress	= true;

	loadMultiStats(sPlayer,&playerStats);
	setMultiStats(selectedPlayer, playerStats, false);
	setMultiStats(selectedPlayer, playerStats, true);

	changeTitleMode(MULTIOPTION);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Game Chooser Screen.

static void addGames(void)
{
	UDWORD i,gcount=0;
	static const char *wrongVersionTip = "Your version of Warzone is incompatible with this game.";
	static const char *badModTip = "Your loaded mods are incompatible with this game. (Check mods/autoload/?)";

	//count games to see if need two columns.
	for(i=0;i<MaxGames;i++)							// draw games
	{
		if( NetPlay.games[i].desc.dwSize !=0)
		{
			gcount++;
		}
	}
	W_BUTINIT sButInit;
	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.width = GAMES_GAMEWIDTH;
	sButInit.height = GAMES_GAMEHEIGHT;
	sButInit.pDisplay = displayRemoteGame;

	// we want the old games deleted, and only list games when we should
	if (getLobbyError() || !gcount)
	{
		for(i = 0; i<MaxGames; i++)
		{
			widgDelete(psWScreen, GAMES_GAMESTART+i);	// remove old widget
		}
		gcount = 0;
	}
	// in case they refresh, and a game becomes available.
	widgDelete(psWScreen,FRONTEND_NOGAMESAVAILABLE);

	// only have to do this if we have any games available.
	if (!getLobbyError() && gcount)
	{
		for (i=0; i<MaxGames; i++)							// draw games
		{
			widgDelete(psWScreen, GAMES_GAMESTART+i);	// remove old icon.
			if (NetPlay.games[i].desc.dwSize !=0)
			{

				sButInit.id = GAMES_GAMESTART+i;

				if (gcount < 9)							// only center column needed.
				{
					sButInit.x = 165;
					sButInit.y = (UWORD)(30+((5+GAMES_GAMEHEIGHT)*i) );
				}
				else
				{
					if (i<9)		//column 1
					{
						sButInit.x = 50;
						sButInit.y = (UWORD)(30+((5+GAMES_GAMEHEIGHT)*i) );
					}
					else		//column 2
					{
						sButInit.x = 60+GAMES_GAMEWIDTH;
						sButInit.y = (UWORD)(30+((5+GAMES_GAMEHEIGHT)*(i-9) ) );
					}
				}
				// display the correct tooltip message.
				if (!NETisCorrectVersion(NetPlay.games[i].game_version_minor, NetPlay.games[i].game_version_major))
				{
					sButInit.pTip = wrongVersionTip;
				}
				else if (strcmp(NetPlay.games[i].modlist,getModList()) != 0)
				{
					sButInit.pTip = badModTip;
				}
				else
				{
					ssprintf(tooltipbuffer, "Map:%s, Game:%s, Hosted by %s ", NetPlay.games[i].mapname, NetPlay.games[i].name, NetPlay.games[i].hostname);
					sButInit.pTip = tooltipbuffer;
				}
				sButInit.UserData = i;

				widgAddButton(psWScreen, &sButInit);
			}
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
			txt = _("No games are available");
			break;
		case ERROR_FULL:
			txt = _("Game is full");
			break;
		case ERROR_KICKED:
		case ERROR_CHEAT:
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
		widgDelete(psWScreen,FRONTEND_NOGAMESAVAILABLE);

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
		sButInit.FontID = font_large;
		sButInit.pText = txt;

		widgAddButton(psWScreen, &sButInit);
	}
}

static void removeGames(void)
{
	int i;
	for (i = 0; i<MaxGames; i++)
	{
		widgDelete(psWScreen, GAMES_GAMESTART+i);	// remove old widget
	}
	widgDelete(psWScreen,FRONTEND_NOGAMESAVAILABLE);
}

void runGameFind(void )
{
	UDWORD id;
	static UDWORD lastupdate=0;
	static char game_password[64];		// check if StringSize is available

	if (lastupdate > gameTime) lastupdate = 0;
	if (gameTime-lastupdate > 6000 && !EnablePasswordPrompt)
	{
		lastupdate = gameTime;
		if(safeSearch)
		{
			NETfindGame();						// find games synchronously
		}
		addGames();									//redraw list
	}

	id = widgRunScreen(psWScreen);						// Run the current set of widgets

	if(id == CON_CANCEL)								// ok
	{
		changeTitleMode(PROTOCOL);
	}

	if(id == MULTIOP_REFRESH)
	{
		ingame.localOptionsReceived = true;
		NETfindGame();								// find games synchronously
		addGames();										//redraw list.
	}
	if (id == CON_PASSWORD)
	{
		sstrcpy(game_password, widgGetString(psWScreen, CON_PASSWORD));
		NETsetGamePassword(game_password);
	}

	// below is when they hit a game box to connect to--ideally this would be where
	// we would want a modal password entry box.
	if (id >= GAMES_GAMESTART && id<=GAMES_GAMEEND)
	{
		gameNumber = id-GAMES_GAMESTART;

		if (!(NetPlay.games[gameNumber].desc.dwFlags & SESSION_JOINDISABLED)) // if still joinable
		{
			if (NetPlay.games[gameNumber].privateGame)
			{
				showPasswordForm();
			}
			else
			{
				ingame.localOptionsReceived = false;			// note we are awaiting options
				sstrcpy(game.name, NetPlay.games[gameNumber].name);		// store name

				joinGame(NetPlay.games[gameNumber].desc.host, 0);
			}
		}

	}
	else if (id == CON_PASSWORDYES)
	{
		ingame.localOptionsReceived = false;			// note we are awaiting options
		sstrcpy(game.name, NetPlay.games[gameNumber].name);		// store name

		joinGame(NetPlay.games[gameNumber].desc.host, 0);
	}
	else if (id == CON_PASSWORDNO)
	{
		hidePasswordForm();
	}
	
	widgDisplayScreen(psWScreen);								// show the widgets currently running
	if(safeSearch)
	{
		iV_SetFont(font_large);
		iV_DrawText(_("Searching"), D_W+260, D_H+460);
	}

	if (CancelPressed())
	{
		changeTitleMode(PROTOCOL);
	}
}

// Used to draw the password box for the lobby screen
static void displayPasswordEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UDWORD	w = psWidget->width;
	UDWORD  h = psWidget->height;

	pie_BoxFill(x, y, x + w, y + h, WZCOL_MENU_BORDER);
	pie_BoxFill(x + 1, y + 1, x + w - 1, y + h - 1, WZCOL_MENU_BACKGROUND);
}

static void showPasswordLabel( WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	SDWORD	fx,fy;
	W_LABEL	*psLab;

	psLab = (W_LABEL *)psWidget;

	fx = xOffset + psWidget->x;
	fy = yOffset + psWidget->y;

	iV_SetFont(font_large);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	iV_DrawText(psLab->aText, fx, fy);
	iV_SetTextColour(WZCOL_TEXT_MEDIUM);
}

// This is what starts the lobby screen
void startGameFind(void)
{
	addBackdrop();										//background image

	// draws the background of the games listed
	W_FORMINIT sFormInit;
	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = FRONTEND_BOTFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = MULTIOP_OPTIONSX;
	sFormInit.y = MULTIOP_OPTIONSY;
	sFormInit.width = MULTIOP_CHATBOXW;
	sFormInit.height = 460;
	sFormInit.pDisplay = intOpenPlainForm;
	sFormInit.disableChildren = true;

	widgAddForm(psWScreen, &sFormInit);

	addSideText(FRONTEND_SIDETEXT,  MULTIOP_OPTIONSX-3, MULTIOP_OPTIONSY,_("GAMES"));

	// cancel
	addMultiBut(psWScreen,FRONTEND_BOTFORM,CON_CANCEL,10,5,MULTIOP_OKW,MULTIOP_OKH,_("Return To Previous Screen"),
		IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	//refresh
	addMultiBut(psWScreen,FRONTEND_BOTFORM,MULTIOP_REFRESH, MULTIOP_CHATBOXW-MULTIOP_OKW-5,5,MULTIOP_OKW,MULTIOP_OKH,
	            _("Refresh Games List"),IMAGE_REFRESH,IMAGE_REFRESH,false);			// Find Games button
	if (safeSearch || disableLobbyRefresh)
	{
		widgHide(psWScreen, MULTIOP_REFRESH);
	}

	NETfindGame();
	addGames();	// now add games.

	// Password stuff. Hidden by default.

	// password label.
	W_LABINIT sLabInit;
	sLabInit.formID = FRONTEND_BACKDROP;
	sLabInit.id		= CON_PASSWORD_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 180;
	sLabInit.y		= 195;
	sLabInit.width	= CON_SETTINGSWIDTH;
	sLabInit.height = 20;
	sLabInit.pText	= _("Enter Password:");
	sLabInit.pDisplay = showPasswordLabel;
	widgAddLabel(psWScreen, &sLabInit);

	// and finally draw the password entry box
	W_EDBINIT sEdInit;
	sEdInit.formID = FRONTEND_BACKDROP;
	sEdInit.id = CON_PASSWORD;
	sEdInit.x = 180;
	sEdInit.y = 200;
	sEdInit.width = 280;
	sEdInit.height = 20;
	sEdInit.pText = "";
	sEdInit.pBoxDisplay = displayPasswordEditBox;

	widgAddEditBox(psWScreen, &sEdInit);

	addMultiBut(psWScreen, FRONTEND_BACKDROP,CON_PASSWORDYES,230,225,MULTIOP_OKW,MULTIOP_OKH,
	            _("OK"),IMAGE_OK,IMAGE_OK,true);
	addMultiBut(psWScreen, FRONTEND_BACKDROP,CON_PASSWORDNO,280,225,MULTIOP_OKW,MULTIOP_OKH,
	            _("Cancel"),IMAGE_NO,IMAGE_NO,true);

	// draws the background of the password box
	sFormInit = W_FORMINIT();
	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = FRONTEND_PASSWORDFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = FRONTEND_BOTFORMX;
	sFormInit.y = 160;
	sFormInit.width = FRONTEND_TOPFORMW;
	sFormInit.height = FRONTEND_TOPFORMH-40;
	sFormInit.pDisplay = intOpenPlainForm;
	sFormInit.disableChildren = true;

	widgAddForm(psWScreen, &sFormInit);

	widgHide(psWScreen, FRONTEND_PASSWORDFORM);
	widgHide(psWScreen, CON_PASSWORD_LABEL);
	widgHide(psWScreen, CON_PASSWORD);
	widgHide(psWScreen, CON_PASSWORDYES);
	widgHide(psWScreen, CON_PASSWORDNO);

	EnablePasswordPrompt = false;
}

static void hidePasswordForm(void)
{
	EnablePasswordPrompt = false;

	widgHide(psWScreen, FRONTEND_PASSWORDFORM);
	widgHide(psWScreen, CON_PASSWORD_LABEL);
	widgHide(psWScreen, CON_PASSWORD);
	widgHide(psWScreen, CON_PASSWORDYES);
	widgHide(psWScreen, CON_PASSWORDNO);

	widgReveal(psWScreen, FRONTEND_SIDETEXT);
	widgReveal(psWScreen, FRONTEND_BOTFORM);
	widgReveal(psWScreen, CON_CANCEL);
	if (!safeSearch && (!disableLobbyRefresh))
	{
		widgReveal(psWScreen, MULTIOP_REFRESH);
	}
	addGames();
}

static void showPasswordForm(void)
{
	W_CONTEXT sContext;
	EnablePasswordPrompt = true;

	widgHide(psWScreen, FRONTEND_SIDETEXT);
	widgHide(psWScreen, FRONTEND_BOTFORM);
	widgHide(psWScreen, CON_CANCEL);
	widgHide(psWScreen, MULTIOP_REFRESH);
	removeGames();

	widgReveal(psWScreen, FRONTEND_PASSWORDFORM);
	widgReveal(psWScreen, CON_PASSWORD_LABEL);
	widgReveal(psWScreen, CON_PASSWORD);
	widgReveal(psWScreen, CON_PASSWORDYES);
	widgReveal(psWScreen, CON_PASSWORDNO);

	// auto click in the password box
	sContext.psScreen	= psWScreen;
	sContext.psForm		= (W_FORM *)psWScreen->psForm;
	sContext.xOffset	= 0;
	sContext.yOffset	= 0;
	sContext.mx			= 0;
	sContext.my			= 0;
	widgGetFromID(psWScreen, CON_PASSWORD)->clicked(&sContext);
}


// ////////////////////////////////////////////////////////////////////////////
// Game Options Screen.

// ////////////////////////////////////////////////////////////////////////////

static void addBlueForm(UDWORD parent,UDWORD id, const char *txt,UDWORD x,UDWORD y,UDWORD w,UDWORD h)
{
	W_FORMINIT sFormInit;                  // draw options box.
	sFormInit.formID= parent;
	sFormInit.id	= id;
	sFormInit.x		=(UWORD) x;
	sFormInit.y		=(UWORD) y;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = (UWORD)w;//190;
	sFormInit.height= (UWORD)h;//27;
	sFormInit.pDisplay =  intDisplayFeBox;
	widgAddForm(psWScreen, &sFormInit);

	if(strlen(txt)>0)
	{
		W_LABINIT sLabInit;
		sLabInit.formID = id;
		sLabInit.id		= id+1;
		sLabInit.x		= 3;
		sLabInit.y		= 4;
		sLabInit.width	= 80;
		sLabInit.height = 20;
		sLabInit.pText	= txt;
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
	{"A0VTolFactory1",  N_("VTOLs disabled."),   IMAGE_NO_VTOL}
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
		bool disabled = asStructLimits[0] != NULL && stat >= 0 && asStructLimits[0][stat].limit == 0;
		flags |= disabled<<i;
	}

	ingame.flags = flags;
}

// need to check for side effects.
static void addGameOptions()
{
	widgDelete(psWScreen,MULTIOP_OPTIONS);  				// clear options list
	widgDelete(psWScreen,FRONTEND_SIDETEXT3);				// del text..

	iV_SetFont(font_regular);

	W_FORMINIT sFormInit;                          // draw options box.
	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = MULTIOP_OPTIONS;
	sFormInit.x = MULTIOP_OPTIONSX;
	sFormInit.y = MULTIOP_OPTIONSY;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = MULTIOP_OPTIONSW;
	sFormInit.height = MULTIOP_OPTIONSH;
	sFormInit.pDisplay = intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

	addSideText(FRONTEND_SIDETEXT3, MULTIOP_OPTIONSX-3 , MULTIOP_OPTIONSY,_("OPTIONS"));

	// game name box
	if (!NetPlay.bComms)
	{
		addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_GNAME, MCOL0, MROW2, _("Select Game Name"), _("One-Player Skirmish"), IMAGE_EDIT_GAME, IMAGE_EDIT_GAME_HI, MULTIOP_GNAME_ICON);
		// disable for one-player skirmish
		widgSetButtonState(psWScreen, MULTIOP_GNAME, WEDBS_DISABLE);
		widgSetButtonState(psWScreen, MULTIOP_GNAME_ICON, WBUT_DISABLE);
	}
	else
	{
		addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_GNAME, MCOL0, MROW2, _("Select Game Name"), game.name, IMAGE_EDIT_GAME, IMAGE_EDIT_GAME_HI, MULTIOP_GNAME_ICON);
	}
	// map chooser
	addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_MAP  , MCOL0, MROW3, _("Select Map"), game.map, IMAGE_EDIT_MAP, IMAGE_EDIT_MAP_HI, MULTIOP_MAP_ICON);
	// disable for challenges
	if (challengeActive)
	{
		widgSetButtonState(psWScreen, MULTIOP_MAP, WEDBS_DISABLE);
		widgSetButtonState(psWScreen, MULTIOP_MAP_ICON, WBUT_DISABLE);
	}
	// password box
	addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_PASSWORD_EDIT  , MCOL0, MROW4, _("Click to set Password"), NetPlay.gamePassword, IMAGE_UNLOCK_BLUE, IMAGE_LOCK_BLUE , MULTIOP_PASSWORD_BUT);
	// disable for one-player skirmish
	if (!NetPlay.bComms)
	{
		widgSetButtonState(psWScreen, MULTIOP_PASSWORD_EDIT, WEDBS_DISABLE);
		widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT, WBUT_DISABLE);
	}
	// buttons.
	
	// game type
	addBlueForm(MULTIOP_OPTIONS,MULTIOP_GAMETYPE,_("Scavengers"),MCOL0,MROW5,MULTIOP_BLUEFORMW,27);
	addMultiBut(psWScreen, MULTIOP_GAMETYPE, MULTIOP_CAMPAIGN, MCOL1, 2, MULTIOP_BUTW, MULTIOP_BUTH, _("Scavengers"), 
	            IMAGE_SCAVENGERS_ON, IMAGE_SCAVENGERS_ON_HI, true);
	addMultiBut(psWScreen, MULTIOP_GAMETYPE, MULTIOP_SKIRMISH, MCOL2, 2, MULTIOP_BUTW, MULTIOP_BUTH, _("No Scavengers"), 
	            IMAGE_SCAVENGERS_OFF, IMAGE_SCAVENGERS_OFF_HI, true);

	widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN,	0);
	widgSetButtonState(psWScreen, MULTIOP_SKIRMISH,	0);

	if (game.scavengers)
	{
		widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_LOCK);
		if (challengeActive)
		{
			widgSetButtonState(psWScreen, MULTIOP_SKIRMISH, WBUT_DISABLE);
		}
	}
	else
	{
		widgSetButtonState(psWScreen, MULTIOP_SKIRMISH, WBUT_LOCK);
		if (challengeActive)
		{
			widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_DISABLE);
		}
	}

	if (game.maxPlayers == MAX_PLAYERS || !foundScavengerPlayerInMap)
	{
		widgSetButtonState(psWScreen, MULTIOP_SKIRMISH, WBUT_LOCK);
		widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_DISABLE);	// full, cannot enable scavenger player
	}

	//just display the game options.
	addMultiEditBox(MULTIOP_OPTIONS, MULTIOP_PNAME, MCOL0, MROW1, _("Select Player Name"), (char*) sPlayer, IMAGE_EDIT_PLAYER, IMAGE_EDIT_PLAYER_HI, MULTIOP_PNAME_ICON);

	// Fog type
	addBlueForm(MULTIOP_OPTIONS,MULTIOP_FOG,_("Fog"),MCOL0,MROW6,MULTIOP_BLUEFORMW,27);

	addMultiBut(psWScreen,MULTIOP_FOG,MULTIOP_FOG_ON ,MCOL1,2,MULTIOP_BUTW,MULTIOP_BUTH, _("Fog Of War"), IMAGE_FOG_OFF, IMAGE_FOG_OFF_HI,true);//black stuff
	addMultiBut(psWScreen,MULTIOP_FOG,MULTIOP_FOG_OFF,MCOL2,2,MULTIOP_BUTW,MULTIOP_BUTH, _("Distance Fog"),IMAGE_FOG_ON,IMAGE_FOG_ON_HI,true);
	if(game.fog)
	{
		widgSetButtonState(psWScreen, MULTIOP_FOG_ON,WBUT_LOCK);
	}
	else
	{
		widgSetButtonState(psWScreen, MULTIOP_FOG_OFF,WBUT_LOCK);
	}

		// alliances
		addBlueForm(MULTIOP_OPTIONS, MULTIOP_ALLIANCES, _("Alliances"), MCOL0, MROW7, MULTIOP_BLUEFORMW, 27);

		addMultiBut(psWScreen,MULTIOP_ALLIANCES,MULTIOP_ALLIANCE_N,MCOL1,2,MULTIOP_BUTW,MULTIOP_BUTH,
				_("No Alliances"),IMAGE_NOALLI,IMAGE_NOALLI_HI,true);
		addMultiBut(psWScreen,MULTIOP_ALLIANCES,MULTIOP_ALLIANCE_Y,MCOL2,2,MULTIOP_BUTW,MULTIOP_BUTH,
				_("Allow Alliances"),IMAGE_ALLI,IMAGE_ALLI_HI,true);

		//add 'Locked Teams' button
		addMultiBut(psWScreen,MULTIOP_ALLIANCES,MULTIOP_ALLIANCE_TEAMS,MCOL3,2,MULTIOP_BUTW,MULTIOP_BUTH,
		            _("Locked Teams"), IMAGE_ALLI_TEAMS, IMAGE_ALLI_TEAMS_HI, true);

		widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N,0);				//hilight correct entry
		widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y,0);
		widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_TEAMS,0);
		if (challengeActive)
		{
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N, WBUT_DISABLE);
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y, WBUT_DISABLE);
		}

		switch(game.alliance)
		{
		case NO_ALLIANCES:
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N,WBUT_LOCK);
			break;
		case ALLIANCES:
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y,WBUT_LOCK);
			break;
		case ALLIANCES_TEAMS:
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_TEAMS,WBUT_LOCK);
			break;
		}

		addBlueForm(MULTIOP_OPTIONS, MULTIOP_POWER, _("Power"), MCOL0, MROW8, MULTIOP_BLUEFORMW, 27);
		addMultiBut(psWScreen,MULTIOP_POWER,MULTIOP_POWLEV_LOW,MCOL1,2,MULTIOP_BUTW,MULTIOP_BUTH,
			_("Low Power Levels"),IMAGE_POWLO,IMAGE_POWLO_HI,true);
		addMultiBut(psWScreen,MULTIOP_POWER,MULTIOP_POWLEV_MED,MCOL2,2,MULTIOP_BUTW,MULTIOP_BUTH,
			_("Medium Power Levels"),IMAGE_POWMED,IMAGE_POWMED_HI,true);
		addMultiBut(psWScreen,MULTIOP_POWER,MULTIOP_POWLEV_HI, MCOL3, 2,MULTIOP_BUTW,MULTIOP_BUTH,
			_("High Power Levels"),IMAGE_POWHI,IMAGE_POWHI_HI,true);
		widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,0);		//hilight correct entry
		widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,0);
		widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI ,0);
		if (game.power <= LEV_LOW)
		{
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,WBUT_LOCK);
			if (challengeActive)
			{
				widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED, WBUT_DISABLE);
				widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI, WBUT_DISABLE);
			}
		}
		else if (game.power <= LEV_MED)
		{
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,WBUT_LOCK);
			if (challengeActive)
			{
				widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW, WBUT_DISABLE);
				widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI, WBUT_DISABLE);
			}
		}
		else
		{
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI,WBUT_LOCK);
			if (challengeActive)
			{
				widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW, WBUT_DISABLE);
				widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED, WBUT_DISABLE);
			}
		}

		addBlueForm(MULTIOP_OPTIONS, MULTIOP_BASETYPE, _("Base"), MCOL0, MROW9, MULTIOP_BLUEFORMW, 27);
		addMultiBut(psWScreen,MULTIOP_BASETYPE,MULTIOP_CLEAN,MCOL1,2,MULTIOP_BUTW,MULTIOP_BUTH,
				_("Start with No Bases"), IMAGE_NOBASE,IMAGE_NOBASE_HI,true);
		addMultiBut(psWScreen,MULTIOP_BASETYPE,MULTIOP_BASE,MCOL2,2,MULTIOP_BUTW,MULTIOP_BUTH,
				_("Start with Bases"),IMAGE_SBASE,IMAGE_SBASE_HI,true);
		addMultiBut(psWScreen,MULTIOP_BASETYPE,MULTIOP_DEFENCE,MCOL3,2,MULTIOP_BUTW,MULTIOP_BUTH,
				_("Start with Advanced Bases"),IMAGE_LBASE,IMAGE_LBASE_HI,true);
		widgSetButtonState(psWScreen, MULTIOP_CLEAN,0);						//hilight correct entry
		widgSetButtonState(psWScreen, MULTIOP_BASE,0);
		widgSetButtonState(psWScreen, MULTIOP_DEFENCE,0);
		switch(game.base)
		{
		case 0:
			widgSetButtonState(psWScreen, MULTIOP_CLEAN,WBUT_LOCK);
			if (challengeActive)
			{
				widgSetButtonState(psWScreen, MULTIOP_BASE, WBUT_DISABLE);
				widgSetButtonState(psWScreen, MULTIOP_DEFENCE, WBUT_DISABLE);
			}
			break;
		case 1:
			widgSetButtonState(psWScreen, MULTIOP_BASE,WBUT_LOCK);
			if (challengeActive)
			{
				widgSetButtonState(psWScreen, MULTIOP_CLEAN, WBUT_DISABLE);
				widgSetButtonState(psWScreen, MULTIOP_DEFENCE, WBUT_DISABLE);
			}
			break;
		case 2:
			widgSetButtonState(psWScreen, MULTIOP_DEFENCE,WBUT_LOCK);
			if (challengeActive)
			{
				widgSetButtonState(psWScreen, MULTIOP_CLEAN, WBUT_DISABLE);
				widgSetButtonState(psWScreen, MULTIOP_BASE, WBUT_DISABLE);
			}
			break;
		}

	addBlueForm(MULTIOP_OPTIONS, MULTIOP_MAP_PREVIEW, _("Map Preview"), MCOL0, MROW10, MULTIOP_BLUEFORMW, 27);
	addMultiBut(psWScreen,MULTIOP_MAP_PREVIEW, MULTIOP_MAP_BUT, MCOL2, 2, MULTIOP_BUTW, MULTIOP_BUTH, 
	            _("Click to see Map"), IMAGE_FOG_OFF, IMAGE_FOG_OFF_HI, true);
	widgSetButtonState(psWScreen, MULTIOP_MAP_BUT,0); //1 = OFF  0=ON 

	// cancel
	addMultiBut(psWScreen,MULTIOP_OPTIONS,CON_CANCEL,
		MULTIOP_CANCELX,MULTIOP_CANCELY,
		iV_GetImageWidth(FrontImages,IMAGE_RETURN),
		iV_GetImageHeight(FrontImages,IMAGE_RETURN),
		_("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	// host Games button
	if(ingame.bHostSetup && !bHosted && !challengeActive)
	{
		addMultiBut(psWScreen,MULTIOP_OPTIONS,MULTIOP_HOST,MULTIOP_HOSTX,MULTIOP_HOSTY,35,28,
					_("Start Hosting Game"), IMAGE_HOST, IMAGE_HOST_HI, IMAGE_HOST_HI);
	}

	// hosted or hosting.
	// limits button.
	if (ingame.bHostSetup)
	{
		addMultiBut(psWScreen,MULTIOP_OPTIONS,MULTIOP_STRUCTLIMITS,MULTIOP_STRUCTLIMITSX,MULTIOP_STRUCTLIMITSY,
		            35, 28, challengeActive ? _("Show Structure Limits") : _("Set Structure Limits"), 
		            IMAGE_SLIM, IMAGE_SLIM_HI, IMAGE_SLIM_HI);
	}

	// Add any relevant factory disabled icons.
	updateLimitFlags();
	{
		int i;
		int y = 2;
		bool skip = false;

		for (i = 0; i < ARRAY_SIZE(limitIcons); ++i)
		{
			if ((ingame.flags & 1<<i) != 0)
			{
				if (!skip)
				{	// only add this once.
					addBlueForm(MULTIOP_OPTIONS, MULTIOP_NO_SOMETHING, "", MULTIOP_HOSTX, MULTIOP_NO_SOMETHINGY, 41, 90 );
				}

				addMultiBut(psWScreen, MULTIOP_NO_SOMETHING, MULTIOP_NO_SOMETHINGY + i, MULTIOP_NO_SOMETHINGX, y,
				            35, 28, _(limitIcons[i].desc),
				            limitIcons[i].icon, limitIcons[i].icon, limitIcons[i].icon);
				y += 28 + 3;
				skip = true;
			}
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Colour functions

static bool safeToUseColour(UDWORD player,UDWORD col)
{
	UDWORD i;

	// if already using it, or we're the host
	if( col == getPlayerColour(player) || NetPlay.isHost )
	{
		return true;						// already using it.
	}

	// player wants to be colour. check no other player to see if it is using that colour.....
	for(i=0;i<MAX_PLAYERS;i++)
	{
		// if no human (except us) is using it
		if( (i!=player) && isHumanPlayer(i) && (getPlayerColour(i) == col) )
		{
			return false;
		}
	}

	return true;
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

	W_FORMINIT sFormInit;
	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = MULTIOP_AI_FORM;	// reuse
	sFormInit.x = MULTIOP_PLAYERSX;
	sFormInit.y = MULTIOP_PLAYERSY;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = MULTIOP_PLAYERSW;
	sFormInit.height = MULTIOP_PLAYERSH;
	sFormInit.pDisplay = intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

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
		sButInit.pTip = NULL;
		switch (i)
		{
		case 0: sButInit.pTip = _("Less aggressive and starts with less units"); break;
		case 1: sButInit.pTip = _("Plays nice"); break;
		case 2: sButInit.pTip = _("No holds barred"); break;
		case 3: sButInit.pTip = _("Starts with advantages and gets twice as much oil from derricks"); break;
		}
		sButInit.pDisplay = displayDifficulty;
		sButInit.UserData = i;
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

	W_FORMINIT sFormInit;
	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = MULTIOP_AI_FORM;
	sFormInit.x = MULTIOP_PLAYERSX;
	sFormInit.y = MULTIOP_PLAYERSY;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = MULTIOP_PLAYERSW;
	sFormInit.height = MULTIOP_PLAYERSH;
	sFormInit.pDisplay = intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

	addSideText(FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX - 3, MULTIOP_PLAYERSY, _("CHOOSE AI"));

	W_BUTINIT sButInit;
	sButInit.formID = MULTIOP_AI_FORM;
	sButInit.x = 7;
	sButInit.width = MULTIOP_PLAYERWIDTH + 1;
	sButInit.height = MULTIOP_PLAYERHEIGHT;
	sButInit.pDisplay = displayAi;

	int y = 4;
	const int step = (MULTIOP_PLAYERHEIGHT + 5);

	// Open button
	if (NetPlay.bComms)
	{
		sButInit.id = MULTIOP_AI_OPEN;
		sButInit.pTip = _("Allow human players to join in this slot");
		sButInit.UserData = (UDWORD)AI_OPEN;
		sButInit.y = y;
		y += step;
		widgAddButton(psWScreen, &sButInit);
	}

	// Closed button
	sButInit.pTip = _("Leave this slot unused");
	sButInit.id = MULTIOP_AI_CLOSED;
	sButInit.UserData = (UDWORD)AI_CLOSED;
	sButInit.y = y;
	y += step + 8;
	widgAddButton(psWScreen, &sButInit);

	for (int i = 0; i < aidata.size(); i++)
	{
		sButInit.y = y;
		y += step;
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
	int gap = MULTIOP_PLAYERSH - 4 - 3 - MULTIOP_TEAMSHEIGHT*game.maxPlayers;
	int gapDiv = game.maxPlayers - 1;
	gap = std::min(gap, 5*gapDiv);
	STATIC_ASSERT(MULTIOP_TEAMSHEIGHT == MULTIOP_PLAYERHEIGHT);  // Why are these different defines?
	return (MULTIOP_TEAMSHEIGHT*gapDiv + gap)*NetPlay.players[player].position/gapDiv + 4;
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
	UDWORD i;

	initChooser(player);

	// add form.
	addBlueForm(MULTIOP_PLAYERS,MULTIOP_COLCHOOSER_FORM,"",
				7,
				playerBoxHeight(player),
				MULTIOP_ROW_WIDTH,MULTIOP_PLAYERHEIGHT);

	// add the flags
	int flagW = iV_GetImageWidth(FrontImages, IMAGE_PLAYERN);
	int flagH = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);
	int space = MULTIOP_ROW_WIDTH - 7 - flagW*MAX_PLAYERS_IN_GUI;
	int spaceDiv = MAX_PLAYERS_IN_GUI;
	space = std::min(space, 5*spaceDiv);
	for (i = 0; i < MAX_PLAYERS_IN_GUI; i++)
	{
		addMultiBut(psWScreen,MULTIOP_COLCHOOSER_FORM, MULTIOP_COLCHOOSER+i,
			i*(flagW*spaceDiv + space)/spaceDiv + 7,  4,  // x, y
			flagW, flagH,  // w, h
			getPlayerColourName(i), IMAGE_PLAYERN, IMAGE_PLAYERN_HI, IMAGE_PLAYERN_HI, i);

			if( !safeToUseColour(selectedPlayer,i))
			{
				widgSetButtonState(psWScreen,MULTIOP_COLCHOOSER+i ,WBUT_DISABLE);
			}
	}

	colourChooserUp = player;
}

static void closeColourChooser(void)
{
	colourChooserUp = -1;
	widgDelete(psWScreen,MULTIOP_COLCHOOSER_FORM);
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
	if(NetPlay.isHost)			// do or request the change.
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
		return true;
	}

	NETbeginDecode(queue, NET_TEAMREQUEST);
	NETuint8_t(&player);
	NETuint8_t(&team);
	NETend();

	if (player > MAX_PLAYERS || team > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_TEAMREQUEST from player %d: Tried to change player %d (team %d)",
		      queue.index, (int)player, (int)team);
		return false;
	}

	if (NetPlay.players[player].team != team)
	{
		resetReadyStatus(false);
	}
	changeTeam(player, team); // we do this regardless, in case of sync issues

	return true;
}

static bool SendReadyRequest(UBYTE player, bool bReady)
{
	if(NetPlay.isHost)			// do or request the change.
	{
		return changeReadyStatus(player, bReady);
	}
	else
	{
		NETbeginEncode(NETbroadcastQueue(), NET_READY_REQUEST);
			NETuint8_t(&player);
			NETbool(&bReady);
		NETend();
	}
	return true;
}

bool recvReadyRequest(NETQUEUE queue)
{
	UBYTE	player;
	bool	bReady;

	if (!NetPlay.isHost || !bHosted)  // Only host should act, and only if the game hasn't started yet.
	{
		return true;
	}

	NETbeginDecode(queue, NET_READY_REQUEST);
		NETuint8_t(&player);
		NETbool(&bReady);
	NETend();

	if (player > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_READY_REQUEST from player %d: player id = %d",
		      queue.index, (int)player);
		return false;
	}

	// do not allow players to select 'ready' if we are sending a map too them!
	// TODO: make a new icon to show this state?
	if (NetPlay.players[player].wzFile.isSending)
	{
		return false;
	}

	return changeReadyStatus((UBYTE)player, bReady);
}

static bool changeReadyStatus(UBYTE player, bool bReady)
{
	drawReadyButton(player);
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
			NetPlay.players[i].position = NetPlay.players[player].position;
			NetPlay.players[player].position = position;
			NETBroadcastTwoPlayerInfo(player, i);
			netPlayersUpdated = true;
			return true;
		}
	}
	debug(LOG_ERROR, "Failed to swap positions for player %d, position %d", (int)player, (int)position);
	if (player < game.maxPlayers && position < game.maxPlayers)
	{
		// Positions were corrupted. Attempt to fix.
		NetPlay.players[player].position = position;
		NETBroadcastPlayerInfo(player);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

bool changeColour(UBYTE player, UBYTE col)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (getPlayerColour(i) == col)
		{
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
	if(NetPlay.isHost)			// do or request the change
	{
		return changeColour(player, col);
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
	if(NetPlay.isHost)			// do or request the change
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
		return true;
	}

	NETbeginDecode(queue, NET_COLOURREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&col);
	NETend();

	if (player > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_COLOURREQUEST from player %d: Tried to change player %d to colour %d",
		      queue.index, (int)player, (int)col);
		return false;
	}

	resetReadyStatus(false);

	return changeColour(player, col);
}

bool recvPositionRequest(NETQUEUE queue)
{
	UBYTE	player, position;

	if (!NetPlay.isHost || !bHosted)  // Only host should act, and only if the game hasn't started yet.
	{
		return true;
	}

	NETbeginDecode(queue, NET_POSITIONREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&position);
	NETend();
	debug(LOG_NET, "Host received position request from player %d to %d", player, position);
	if (player > MAX_PLAYERS || position > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_POSITIONREQUEST from player %d: Tried to change player %d to %d",
		      queue.index, (int)player, (int)position);
		return false;
	}

	resetReadyStatus(false);

	return changePosition(player, position);
}

#define ANYENTRY 0xFF		// used to allow any team slot to be used.
/*
 * Opens a menu for a player to choose a team
 * 'player' is a player id of the player who will get a new team assigned
 */
static void addTeamChooser(UDWORD player)
{
	UDWORD i;
	int disallow = ANYENTRY;
	SDWORD inSlot[MAX_PLAYERS] = {0};

	debug(LOG_NET, "Opened team chooser for %d, current team: %d", player, NetPlay.players[player].team);

	initChooser(player);

	// add form.
	addBlueForm(MULTIOP_PLAYERS, MULTIOP_TEAMCHOOSER_FORM, "", 7, playerBoxHeight(player), MULTIOP_ROW_WIDTH, MULTIOP_TEAMSHEIGHT);

	// tally up the team counts
	for (i=0; i< game.maxPlayers ; i++)
	{
		if (i != player)
		{
			inSlot[NetPlay.players[i].team]++;
			if (inSlot[NetPlay.players[i].team] >= game.maxPlayers - 1)
			{
				// Make sure all players can't be on same team.
				disallow = NetPlay.players[i].team;
			}
		}
	}

	int teamW = iV_GetImageWidth(FrontImages, IMAGE_TEAM0);
	int teamH = iV_GetImageHeight(FrontImages, IMAGE_TEAM0);
	int space = MULTIOP_ROW_WIDTH - 7 - teamW*(game.maxPlayers + 1);
	int spaceDiv = game.maxPlayers;
	space = std::min(space, 3*spaceDiv);

	// add the teams, skipping the one we CAN'T be on (if applicable)
	for (i = 0; i < game.maxPlayers; i++)
	{
		if (i != disallow)
		{
			addMultiBut(psWScreen, MULTIOP_TEAMCHOOSER_FORM, MULTIOP_TEAMCHOOSER + i, i * (teamW*spaceDiv + space)/spaceDiv + 3, 6, teamW, teamH, _("Team"), IMAGE_TEAM0 + i , IMAGE_TEAM0_HI + i, IMAGE_TEAM0_HI + i);
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
static void closeTeamChooser(void)
{
	teamChooserUp = -1;
	widgDelete(psWScreen,MULTIOP_TEAMCHOOSER_FORM);	//only once!
}

static void drawReadyButton(UDWORD player)
{
	// delete 'ready' botton form
	widgDelete(psWScreen, MULTIOP_READY_FORM_ID + player);

	// add form to hold 'ready' botton
	addBlueForm(MULTIOP_PLAYERS,MULTIOP_READY_FORM_ID + player,"",
				8 + MULTIOP_PLAYERWIDTH - MULTIOP_READY_WIDTH,
				playerBoxHeight(player),
				MULTIOP_READY_WIDTH,MULTIOP_READY_HEIGHT);

	if (!NetPlay.players[player].allocated && NetPlay.players[player].ai >= 0)
	{
		int icon = difficultyIcon(NetPlay.players[player].difficulty);
		addMultiBut(psWScreen, MULTIOP_READY_FORM_ID + player, MULTIOP_DIFFICULTY_INIT_START + player, 6, 4, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
		            challengeActive ? _("You cannot change difficulty in a challenge") : NetPlay.isHost? _("Click to change difficulty") : NULL, icon, icon, icon);
		return;
	}
	else if (!NetPlay.players[player].allocated)
	{
		return;	// closed or open
	}

	// draw 'ready' button
	if (NetPlay.players[player].ready)
	{
		addMultiBut(psWScreen, MULTIOP_READY_FORM_ID + player, MULTIOP_READY_START + player, 3, 8, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
		            _("Waiting for other players"), IMAGE_CHECK_ON, IMAGE_CHECK_ON, IMAGE_CHECK_ON_HI);
	}
	else
	{
		addMultiBut(psWScreen, MULTIOP_READY_FORM_ID + player, MULTIOP_READY_START + player, 3, 8, MULTIOP_READY_WIDTH, MULTIOP_READY_HEIGHT,
		            _("Click when ready"), IMAGE_CHECK_OFF, IMAGE_CHECK_OFF, IMAGE_CHECK_OFF_HI);
	}

	addText(MULTIOP_READY_START+MAX_PLAYERS+player, 0,10,
	        _("READY?"), MULTIOP_READY_FORM_ID + player);
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
	if(widgGetFromID(psWScreen,FRONTEND_BACKDROP) == NULL)
	{
		return;
	}

	widgDelete(psWScreen,MULTIOP_PLAYERS);		// del player window
	widgDelete(psWScreen,FRONTEND_SIDETEXT2);	// del text too,

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

	W_FORMINIT sFormInit;                           // draw player window
	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = MULTIOP_PLAYERS;
	sFormInit.x = MULTIOP_PLAYERSX;
	sFormInit.y = MULTIOP_PLAYERSY;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = MULTIOP_PLAYERSW;
	sFormInit.height = MULTIOP_PLAYERSH;
	sFormInit.pDisplay = intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

	addSideText(FRONTEND_SIDETEXT2, MULTIOP_PLAYERSX-3, MULTIOP_PLAYERSY,_("PLAYERS"));

	if(players)
	{
		int  team = -1;
		bool allOnSameTeam = true;

		for (int i = 0; i < game.maxPlayers; i++)
		{
			if (game.skDiff[i] || isHumanPlayer(i))
			{
				int myTeam = game.alliance != ALLIANCES_TEAMS? NetPlay.players[i].team : i;
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
				sButInit.id = MULTIOP_TEAMS_START+i;
				sButInit.x = 7;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_TEAMSWIDTH;
				sButInit.height = MULTIOP_TEAMSHEIGHT;
				if (canChooseTeamFor(i))
				{
					sButInit.pTip = _("Choose Team");
				}
				else
				{
					sButInit.pTip = NULL;
				}
				sButInit.pDisplay = displayTeamChooser;
				sButInit.UserData = i;

				if (teamChooserUp == i && colourChooserUp < 0)
				{
					addTeamChooser(i);
				}
				else if (game.alliance == ALLIANCES_TEAMS)
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
			else
			{
				sColInit.pTip = NULL;
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
				sButInit.id = MULTIOP_PLAYER_START+i;
				sButInit.x = 7 + MULTIOP_TEAMSWIDTH + MULTIOP_COLOUR_WIDTH;
				sButInit.y = playerBoxHeight(i);
				sButInit.width = MULTIOP_PLAYERWIDTH - MULTIOP_TEAMSWIDTH - MULTIOP_READY_WIDTH - MULTIOP_COLOUR_WIDTH;
				sButInit.height = MULTIOP_PLAYERHEIGHT;
				sButInit.pTip = NULL;
				if ((selectedPlayer == i || NetPlay.isHost) && NetPlay.players[i].allocated)
				{
					sButInit.pTip = _("Click to change player position");
				}
				else if (!NetPlay.players[i].allocated && NetPlay.isHost)
				{
					if (!challengeActive)
					{
						sButInit.pTip = _("Click to change AI");
					}
					else
					{
						sButInit.pTip = _("You cannot change AI in a challenge");
					}
				}
				sButInit.pDisplay = displayPlayer;
				sButInit.UserData = i;
				widgAddButton(psWScreen, &sButInit);
			}
		}
	}
}

/*
 * Notify all players of host launching the game
 */
static void SendFireUp(void)
{
	NETbeginEncode(NETbroadcastQueue(), NET_FIREUP);
		// no payload necessary
	NETend();
}

// host kicks a player from a game.
void kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type)
{
	// send a kick msg
	NETbeginEncode(NETbroadcastQueue(), NET_KICK);
		NETuint32_t(&player_id);
		NETstring( (char *) reason, MAX_KICK_REASON);
		NETenum(&type);
	NETend();
	NETflush();
	debug(LOG_NET, "Kicking player %u (%s).",
	      (unsigned int)player_id, getPlayerName(player_id));

	NETplayerKicked(player_id);
}

static void addChatBox(void)
{
	if(widgGetFromID(psWScreen,FRONTEND_TOPFORM))
	{
		widgDelete(psWScreen,FRONTEND_TOPFORM);
	}

	if(widgGetFromID(psWScreen,MULTIOP_CHATBOX))
	{
		return;
	}

	W_FORMINIT sFormInit;

	sFormInit.formID = FRONTEND_BACKDROP;							// add the form
	sFormInit.id = MULTIOP_CHATBOX;
	sFormInit.x = MULTIOP_CHATBOXX;
	sFormInit.y = MULTIOP_CHATBOXY;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = MULTIOP_CHATBOXW;
	sFormInit.height = MULTIOP_CHATBOXH;
	sFormInit.disableChildren = true;								// wait till open!
	sFormInit.pDisplay = intOpenPlainForm;//intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

	addSideText(FRONTEND_SIDETEXT4,MULTIOP_CHATBOXX-3,MULTIOP_CHATBOXY,_("CHAT"));

	flushConsoleMessages();											// add the chatbox.
	initConsoleMessages();
	enableConsoleDisplay(true);
	setConsoleBackdropStatus(false);
	setDefaultConsoleJust(LEFT_JUSTIFY);
	setConsoleSizePos(MULTIOP_CHATBOXX+4+D_W, MULTIOP_CHATBOXY+14+D_H, MULTIOP_CHATBOXW-4);
	setConsolePermanence(true,true);
	setConsoleLineInfo(5);											// use x lines on chat window

	W_EDBINIT sEdInit;                                                                                      // add the edit box
	sEdInit.formID = MULTIOP_CHATBOX;
	sEdInit.id = MULTIOP_CHATEDIT;
	sEdInit.x = MULTIOP_CHATEDITX;
	sEdInit.y = MULTIOP_CHATEDITY;
	sEdInit.width = MULTIOP_CHATEDITW;
	sEdInit.height = MULTIOP_CHATEDITH;

	sEdInit.pUserData = NULL;
	sEdInit.pBoxDisplay = displayChatEdit;

	widgAddEditBox(psWScreen, &sEdInit);

	if (*getModList())
	{
		char modListMessage[WIDG_MAXSTR] = "";
		sstrcat(modListMessage, _("Mod: "));
		sstrcat(modListMessage, getModList());
		addConsoleMessage(modListMessage,DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		if (NetPlay.bComms)
		{
			addConsoleMessage(_("All players need to have the same mods to join your game."),DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
	}

	return;
}

// ////////////////////////////////////////////////////////////////////////////
static void disableMultiButs(void)
{

	// edit box icons.
	widgSetButtonState(psWScreen, MULTIOP_GNAME_ICON, WBUT_DISABLE);
	widgSetButtonState(psWScreen, MULTIOP_MAP_ICON, WBUT_DISABLE);
	if (NetPlay.GamePassworded)
	{
		// force the state down if a locked game
		// FIXME: It don't seem to be locking it into the 2nd state?
		widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT, WBUT_LOCK);
	}
	widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT, WBUT_DISABLE);

	// edit boxes
	widgSetButtonState(psWScreen,MULTIOP_GNAME,WEDBS_DISABLE);
	widgSetButtonState(psWScreen,MULTIOP_MAP,WEDBS_DISABLE);

	if (!NetPlay.isHost)
	{
		if( game.fog) widgSetButtonState(psWScreen,MULTIOP_FOG_OFF ,WBUT_DISABLE);		//fog
		if(!game.fog) widgSetButtonState(psWScreen,MULTIOP_FOG_ON ,WBUT_DISABLE);

			if(game.base != CAMP_CLEAN)	widgSetButtonState(psWScreen,MULTIOP_CLEAN ,WBUT_DISABLE);	// camapign subtype.
			if(game.base != CAMP_BASE)	widgSetButtonState(psWScreen,MULTIOP_BASE ,WBUT_DISABLE);
			if(game.base != CAMP_WALLS)	widgSetButtonState(psWScreen,MULTIOP_DEFENCE,WBUT_DISABLE);

			if(game.power != LEV_LOW)	widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,WBUT_DISABLE);		// pow levels
			if(game.power != LEV_MED)	widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,WBUT_DISABLE);
			if(game.power != LEV_HI )	widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI,WBUT_DISABLE);

			if(game.alliance != NO_ALLIANCES)	widgSetButtonState(psWScreen,MULTIOP_ALLIANCE_N ,WBUT_DISABLE);	//alliance settings.
			if(game.alliance != ALLIANCES)	widgSetButtonState(psWScreen,MULTIOP_ALLIANCE_Y ,WBUT_DISABLE);
			if(game.alliance != ALLIANCES_TEAMS)	widgSetButtonState(psWScreen,MULTIOP_ALLIANCE_TEAMS ,WBUT_DISABLE);
	}
}

////////////////////////////////////////////////////////////////////////////
static void stopJoining(void)
{
	dwSelectedGame	 = 0;
	reloadMPConfig(); // reload own settings

	debug(LOG_NET,"player %u (Host is %s) stopping.", selectedPlayer, NetPlay.isHost ? "true" : "false");

		if(bHosted)											// cancel a hosted game.
		{
			// annouce we are leaving...
			debug(LOG_NET, "Host is quitting game...");
			NETbeginEncode(NETbroadcastQueue(), NET_HOST_DROPPED);
			NETend();
			sendLeavingMsg();								// say goodbye
			NETclose();										// quit running game.
			bHosted = false;								// stop host mode.
			widgDelete(psWScreen,FRONTEND_BACKDROP);		// refresh options screen.
			startMultiOptions(false);
			ingame.localJoiningInProgress = false;
			NETremRedirects();
			return;
		}
		else if(ingame.localJoiningInProgress)				// cancel a joined game.
		{
			debug(LOG_NET, "Canceling game...");
			sendLeavingMsg();								// say goodbye
			NETclose();										// quit running game.

			// if we were in a midle of transfering a file, then close the file handle
			if (NetPlay.pMapFileHandle)
			{
				debug(LOG_NET, "closing aborted file");		// no need to delete it, we do size check on (map) file
				PHYSFS_close(NetPlay.pMapFileHandle);
				NetPlay.pMapFileHandle = NULL;
			}
			ingame.localJoiningInProgress = false;			// reset local flags
			ingame.localOptionsReceived = false;
			if(!ingame.bHostSetup && NetPlay.isHost)			// joining and host was transfered.
			{
				NetPlay.isHost = false;
			}

			if(NetPlay.bComms)	// not even connected.
			{
				changeTitleMode(GAMEFIND);
			}
			else
			{
				changeTitleMode(MULTI);
			}
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

/*
 * Process click events on the multiplayer/skirmish options screen
 * 'id' is id of the button that was pressed
 */
static void processMultiopWidgets(UDWORD id)
{
	PLAYERSTATS playerStats;

	// host, who is setting up the game
	if((ingame.bHostSetup && !bHosted))
	{
		switch(id)												// Options buttons
		{
		case MULTIOP_GNAME:										// we get this when nec.
			sstrcpy(game.name,widgGetString(psWScreen, MULTIOP_GNAME));
			break;

		case MULTIOP_MAP:
			widgSetString(psWScreen, MULTIOP_MAP,game.map);
			break;

		case MULTIOP_GNAME_ICON:
			break;

		case MULTIOP_MAP_ICON:
			widgDelete(psWScreen,MULTIOP_PLAYERS);
			widgDelete(psWScreen,FRONTEND_SIDETEXT2);					// del text too,

			debug(LOG_WZ, "processMultiopWidgets[MULTIOP_MAP_ICON]: %s.wrf", MultiCustomMapsPath);
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, current_numplayers);
			break;

		case MULTIOP_PASSWORD_BUT:
			{
				char game_password[64];
				char buf[255];
				int32_t result = 0;

				result = widgGetButtonState(psWScreen, MULTIOP_PASSWORD_BUT);
				debug(LOG_NET, "Password button hit, %d", result);
				if (result == 0)
				{
					sstrcpy(game_password, widgGetString(psWScreen, MULTIOP_PASSWORD_EDIT));
					NETsetGamePassword(game_password);
					widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT, WBUT_CLICKLOCK);
					widgSetButtonState(psWScreen, MULTIOP_PASSWORD_EDIT, WEDBS_DISABLE);
					// say password is now required to join games?
					ssprintf(buf, _("*** password [%s] is now required! ***"), NetPlay.gamePassword);
					addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
					NETGameLocked(true);
				}
				else
				{
					widgSetButtonState(psWScreen, MULTIOP_PASSWORD_BUT , 0);
					widgSetButtonState(psWScreen, MULTIOP_PASSWORD_EDIT, 0);
					ssprintf(buf, _("*** password is NOT required! ***"));
					addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
					NETresetGamePassword();
					NETGameLocked(false);
					break;
				}

			}
			break;

		case MULTIOP_MAP_BUT:
			loadMapPreview(true);
			break;
		}
	}

	// host who is setting up or has hosted
	if(ingame.bHostSetup)// || NetPlay.isHost) // FIXME Was: if(ingame.bHostSetup);{} ??? Note the ; !
	{
		switch(id)
		{
		case MULTIOP_CAMPAIGN:									// turn on campaign game
			widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_SKIRMISH,0);
			game.scavengers = true;
			resetReadyStatus(false);
			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_SKIRMISH:
			widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN,0 );
			widgSetButtonState(psWScreen, MULTIOP_SKIRMISH,WBUT_LOCK);
			game.scavengers = false;
			resetReadyStatus(false);
			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_FOG_ON:
			widgSetButtonState(psWScreen, MULTIOP_FOG_ON,WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_FOG_OFF,0);
			game.fog = true;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_FOG_OFF:
			widgSetButtonState(psWScreen, MULTIOP_FOG_ON,0);
			widgSetButtonState(psWScreen, MULTIOP_FOG_OFF,WBUT_LOCK);
			game.fog = false;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_CLEAN:
			game.base = CAMP_CLEAN;
			addGameOptions();

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
				disableMultiButs();
			}
			break;

		case MULTIOP_BASE:
			game.base = CAMP_BASE;
			addGameOptions();

			resetReadyStatus(false);

			if(bHosted)
			{
				disableMultiButs();
				sendOptions();
			}
			break;

		case MULTIOP_DEFENCE:
			game.base = CAMP_WALLS;
			addGameOptions();

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
				disableMultiButs();
			}
			break;

		case MULTIOP_ALLIANCE_N:
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N,WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y,0);

			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_TEAMS,0);
			// Host wants NO alliance, so reset all teams back to default
			game.alliance = NO_ALLIANCES;	//0;

			resetReadyStatus(false);
			netPlayersUpdated = true;

			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_ALLIANCE_Y:
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N,0);
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_TEAMS,0);

			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y,WBUT_LOCK);

			game.alliance = ALLIANCES;	//1;

			resetReadyStatus(false);
			netPlayersUpdated = true;

			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_ALLIANCE_TEAMS:	//locked teams
			resetReadyStatus(false);
			setLockedTeamsMode();
			break;

		case MULTIOP_POWLEV_LOW:								// set power level to low
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,0);
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI ,0);
			game.power = LEV_LOW;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_POWLEV_MED:								// set power to med
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,0);
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI ,0);
			game.power = LEV_MED;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
			}
			break;

		case MULTIOP_POWLEV_HI:									// set power to hi
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,0);
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,0);
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI ,WBUT_LOCK);
			game.power = LEV_HI;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
			}
			break;
		}
	}

	// these work all the time.
	switch(id)
	{
	case MULTIOP_STRUCTLIMITS:
		changeTitleMode(MULTILIMIT);
		break;

	case MULTIOP_PNAME:
		sstrcpy(sPlayer,widgGetString(psWScreen, MULTIOP_PNAME));

		// chop to 15 chars..
		while(strlen(sPlayer) > 15)	// clip name.
		{
			sPlayer[strlen(sPlayer)-1]='\0';
		}

		// update string.
		widgSetString(psWScreen, MULTIOP_PNAME,sPlayer);


		removeWildcards((char*)sPlayer);

		printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);

		NETchangePlayerName(selectedPlayer, (char*)sPlayer);			// update if joined.
		loadMultiStats((char*)sPlayer,&playerStats);
		setMultiStats(selectedPlayer,playerStats,false);
		setMultiStats(selectedPlayer,playerStats,true);
		netPlayersUpdated = true;
		break;

	case MULTIOP_PNAME_ICON:
		widgDelete(psWScreen,MULTIOP_PLAYERS);
		widgDelete(psWScreen,FRONTEND_SIDETEXT2);					// del text too,

		addMultiRequest(MultiPlayersPath, ".sta", MULTIOP_PNAME, 0, 0);
		break;

	case MULTIOP_HOST:
		debug(LOG_NET, "MULTIOP_HOST enabled");
		sstrcpy(game.name, widgGetString(psWScreen, MULTIOP_GNAME));	// game name
		sstrcpy(sPlayer, widgGetString(psWScreen, MULTIOP_PNAME));	// pname
		sstrcpy(game.map, widgGetString(psWScreen, MULTIOP_MAP));		// add the name

		resetReadyStatus(false);
		resetDataHash();
		removeWildcards((char*)sPlayer);

		if (!hostCampaign((char*)game.name,(char*)sPlayer))
		{
			addConsoleMessage(_("Sorry! Failed to host the game."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			break;
		}
		bHosted = true;

		widgDelete(psWScreen,MULTIOP_REFRESH);
		widgDelete(psWScreen,MULTIOP_HOST);

		ingame.localOptionsReceived = true;

		addGameOptions();									// update game options box.
		addChatBox();

		disableMultiButs();

		addPlayerBox(!ingame.bHostSetup || bHosted);	//to make sure host can't skip player selection menu (sets game.skdiff to UBYTE_MAX for humans)
		break;

	case MULTIOP_CHATEDIT:

		// don't send empty lines to other players in the lobby
		if(!strcmp(widgGetString(psWScreen, MULTIOP_CHATEDIT), ""))
			break;

		sendTextMessage(widgGetString(psWScreen, MULTIOP_CHATEDIT),true);					//send
		widgSetString(psWScreen, MULTIOP_CHATEDIT, "");										// clear box
		break;

	case CON_CANCEL:
		if (!challengeActive)
		{
			NETGameLocked(false);		// reset status on a cancel
			stopJoining();
		}
		else
		{
			NETclose();
			widgDelete(psWScreen, FRONTEND_BACKDROP);
			challengeActive = false;
			bHosted = false;
			ingame.localJoiningInProgress = false;
			changeTitleMode(TITLE);
		}
		break;
	case MULTIOP_MAP_BUT:
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

	if (id >= MULTIOP_DIFFICULTY_CHOOSE_START && id <= MULTIOP_DIFFICULTY_CHOOSE_END)
	{
		int idx = id - MULTIOP_DIFFICULTY_CHOOSE_START;
		NetPlay.players[difficultyChooserUp].difficulty = idx;
		game.skDiff[difficultyChooserUp] = difficultyValue[idx];
		NETBroadcastPlayerInfo(difficultyChooserUp);
		closeDifficultyChooser();
		addPlayerBox(!ingame.bHostSetup || bHosted);
	}

	if (id >= MULTIOP_AI_START && id <= MULTIOP_AI_END)
	{
		int idx = id - MULTIOP_AI_START;
		NetPlay.players[aiChooserUp].ai = idx;
		game.skDiff[aiChooserUp] = difficultyValue[NetPlay.players[aiChooserUp].difficulty];    // set difficulty, in case re-enabled
		NETBroadcastPlayerInfo(aiChooserUp);
		closeAiChooser();
		addPlayerBox(!ingame.bHostSetup || bHosted);
	}

	STATIC_ASSERT(MULTIOP_TEAMS_START + MAX_PLAYERS - 1 <= MULTIOP_TEAMS_END);
	if (id >= MULTIOP_TEAMS_START && id <= MULTIOP_TEAMS_START + MAX_PLAYERS - 1 && !challengeActive)  // Clicked on a team chooser
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
	if(id >= MULTIOP_TEAMCHOOSER && id <= MULTIOP_TEAMCHOOSER + MAX_PLAYERS - 1)
	{
		ASSERT(teamChooserUp >= 0, "teamChooserUp < 0");
		ASSERT(id >= MULTIOP_TEAMCHOOSER
		    && (id - MULTIOP_TEAMCHOOSER) < MAX_PLAYERS, "processMultiopWidgets: wrong id - MULTIOP_TEAMCHOOSER value (%d)", id - MULTIOP_TEAMCHOOSER);

		resetReadyStatus(false);		// will reset only locally if not a host

		SendTeamRequest(teamChooserUp, (UBYTE)id - MULTIOP_TEAMCHOOSER);

		debug(LOG_WZ, "Changed team for player %d to %d", teamChooserUp, NetPlay.players[teamChooserUp].team);

		closeTeamChooser();
		addPlayerBox(  !ingame.bHostSetup || bHosted);	//restore initial options screen

		//enable locked teams mode
		//-----------------------------
		if(game.alliance != ALLIANCES_TEAMS && bHosted)		//only if host
		{
			resetReadyStatus(false);
			setLockedTeamsMode();		//update GUI

			addConsoleMessage(_("'Locked Teams' mode enabled"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
	}

	// 'ready' button
	if(id >= MULTIOP_READY_START && id <= MULTIOP_READY_END)  // clicked on a player
	{
		UBYTE player = (UBYTE)(id-MULTIOP_READY_START);

		if (player == selectedPlayer && teamChooserUp < 0 && positionChooserUp < 0)
		{
			SendReadyRequest(selectedPlayer, !NetPlay.players[player].ready);

			// if hosting try to start the game if everyone is ready
			if(NetPlay.isHost && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
				// reset flag in case people dropped/quit on join screen
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_NORMAL, NET_ALL_PLAYERS);
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
	if (id >= MULTIOP_PLAYER_START && id <= MULTIOP_PLAYER_START + MAX_PLAYERS - 1 && (id - MULTIOP_PLAYER_START == selectedPlayer || NetPlay.isHost || (positionChooserUp >= 0 && !isHumanPlayer(id - MULTIOP_PLAYER_START))))
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
		else if (!NetPlay.players[player].allocated && !challengeActive && NetPlay.isHost
		         && positionChooserUp < 0 && teamChooserUp < 0 && colourChooserUp < 0)
		{
			addAiChooser(player);
		}
	}

	if (id >= MULTIOP_DIFFICULTY_INIT_START && id <= MULTIOP_DIFFICULTY_INIT_END
	    && !challengeActive && NetPlay.isHost && positionChooserUp < 0 && teamChooserUp < 0 && colourChooserUp < 0)
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
		sendTextMessage(msg, true);
		kickPlayer(teamChooserUp, "you are unwanted by the host.", ERROR_KICKED);
		resetReadyStatus(true);		//reset and send notification to all clients
		closeTeamChooser();
	}
}

/* Start a multiplayer or skirmish game */
void startMultiplayerGame(void)
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

	if (NetPlay.isHost)
	{
		// This sets the limits to whatever the defaults are for the limiter screen
		// If host sets limits, then we do not need to do the following routine.
		if (!bLimiterLoaded)
		{
			debug(LOG_NET, "limiter was NOT activated, setting defaults");

			// NOTE: master <-> 2.3 difference, we don't load limiter_tex!
			if (!resLoad("wrf/limiter_tex.wrf", 501))
			{
				debug(LOG_INFO, "Unable to load limiter_tex.  Defaults not set.");
			}
			if (!resLoad("wrf/piestats.wrf", 502))
			{
				debug(LOG_INFO, "Unable to load piestats.  Defaults not set.");
			}
			else if (!resLoad("wrf/limiter_data.wrf", 503))
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
		debug(LOG_NET,"sending our options to all clients");
		sendOptions(); 
		NEThaltJoining();							// stop new players entering.
		ingame.TimeEveryoneIsInGame = 0;
		ingame.isAllPlayersDataOK = false;
		memset(&ingame.DataIntegrity, 0x0, sizeof(ingame.DataIntegrity));	//clear all player's array
		SendFireUp();								//bcast a fireup message
	}

	// set the fog correctly..
	setRevealStatus(game.fog);
	war_SetFog(!game.fog);

	debug(LOG_NET, "title mode STARTGAME is set--Starting Game!"); 
	changeTitleMode(STARTGAME);

	bHosted = false;

	if (NetPlay.isHost)
	{
		sendTextMessage(_("Host is Starting Game"),true);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Net message handling

void frontendMultiMessages(void)
{
	NETQUEUE queue;
	uint8_t type;

	while (NETrecvNet(&queue, &type))
	{
		// Copy the message to the global one used by the new NET API
		switch(type)
		{
		case NET_FILE_REQUESTED:
			recvMapFileRequested(queue);
			break;

		case NET_FILE_PAYLOAD:
			widgSetButtonState(psWScreen, MULTIOP_MAP_BUT, 1);	// turn preview button off
			if (recvMapFileData(queue))
			{
				widgSetButtonState(psWScreen, MULTIOP_MAP_BUT, 0);	// turn it back on again
			}
			break;

		case NET_FILE_CANCELLED:					// host only routine
			{
				uint32_t reason;
				uint32_t victim;

				NETbeginDecode(queue, NET_FILE_CANCELLED);
					NETuint32_t(&victim);
					NETuint32_t(&reason);
				NETend();
				switch (reason)
				{
					case STUCK_IN_FILE_LOOP:
						debug(LOG_WARNING, "Received file cancel request from player %u, They are stuck in a loop?", victim);
						kickPlayer(victim, "the host couldn't send a file for some reason.", ERROR_UNKNOWNFILEISSUE);
						NetPlay.players[victim].wzFile.isCancelled = true;
						NetPlay.players[victim].wzFile.isSending = false;
						break;

					case ALREADY_HAVE_FILE:
					default:
						debug(LOG_WARNING, "Received file cancel request from player %u, they already have the file.", victim);
						NetPlay.players[victim].wzFile.isCancelled = true;
						NetPlay.players[victim].wzFile.isSending = false;
						break;
				}
			}
			break;

		case NET_OPTIONS:					// incoming options file.
			recvOptions(queue);
			ingame.localOptionsReceived = true;

			if(titleMode == MULTIOPTION)
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

			debug(LOG_INFO,"** player %u has dropped!", player_id);

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
			debug(LOG_NET, "NET_FIREUP was received ..."); 
			if(ingame.localOptionsReceived)
			{
				debug(LOG_NET, "& local Options Received (MP game)");
				ingame.TimeEveryoneIsInGame = 0;			// reset time
				resetDataHash();
				decideWRF();

				// set the fog correctly..
				setRevealStatus(game.fog);
				war_SetFog(!game.fog);

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
				debug(LOG_ERROR, "someone tried to kick the host--check your netplay logs!");
				break;
			}

			if (selectedPlayer == player_id)	// we've been told to leave.
			{
				setLobbyError(KICK_TYPE);
				stopJoining();
				//screen_RestartBackDrop();
				//changeTitleMode(TITLE);
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
			break;

		case NET_TEXTMSG:					// Chat message
			if(ingame.localOptionsReceived)
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

void runMultiOptions(void)
{
	static UDWORD	lastrefresh = 0;
	UDWORD			id, value, i;
	char			sTemp[128];
	PLAYERSTATS		playerStats;
	W_CONTEXT		context;

	KEY_CODE		k;
	char			str[3];

	frontendMultiMessages();

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		// send it for each player that needs it
		if (NetPlay.players[i].wzFile.isSending)
		{
			sendMap();
		}
	}

	// update boxes?
	if (netPlayersUpdated || (NetPlay.isHost && mouseDown(MOUSE_LMB) && gameTime-lastrefresh>500))
	{
		netPlayersUpdated = false;
		lastrefresh= gameTime;
		if (!multiRequestUp && (bHosted || ingame.localJoiningInProgress))
		{
			addPlayerBox(true);				// update the player box.
			loadMapPreview(false);
		}
	}


	// update scores and pings if far enough into the game
	if(ingame.localOptionsReceived && ingame.localJoiningInProgress)
	{
		sendScoreCheck();
		sendPing();
	}

	// if typing and not in an edit box then jump to chat box.
	k = getQwertyKey();
	if(	k && psWScreen->psFocus == NULL)
	{
		context.psScreen	= psWScreen;
		context.psForm		= (W_FORM *)psWScreen->psForm;
		context.xOffset		= 0;
		context.yOffset		= 0;
		context.mx			= mouseX();
		context.my			= mouseY();

		keyScanToString(k,(char*)&str,3);
		if(widgGetFromID(psWScreen,MULTIOP_CHATEDIT))
		{
			widgSetString(psWScreen, MULTIOP_CHATEDIT, (char*)&str);	// start it up!
			widgGetFromID(psWScreen, MULTIOP_CHATEDIT)->clicked(&context);
		}
	}

	// chat box handling
	if(widgGetFromID(psWScreen,MULTIOP_CHATBOX))
	{
		while(getNumberConsoleMessages() >getConsoleLineInfo())
		{
			removeTopConsoleMessage();
		}
		updateConsoleMessages();								// run the chatbox
	}

	// widget handling

	if(multiRequestUp)
	{
		id = widgRunScreen(psRScreen);						// a requester box is up.
		if( runMultiRequester(id,&id,(char*)&sTemp,&value))
		{
			switch(id)
			{
			case MULTIOP_PNAME:
				sstrcpy(sPlayer, sTemp);
				widgSetString(psWScreen,MULTIOP_PNAME,sTemp);

				removeWildcards((char*)sPlayer);

				printConsoleNameChange(NetPlay.players[selectedPlayer].name, sPlayer);

				NETchangePlayerName(selectedPlayer, (char*)sPlayer);
				loadMultiStats((char*)sPlayer,&playerStats);
				setMultiStats(selectedPlayer,playerStats,false);
				setMultiStats(selectedPlayer,playerStats,true);
				netPlayersUpdated = true;
				break;
			case MULTIOP_MAP:
				sstrcpy(game.map, sTemp);
				game.maxPlayers =(UBYTE) value;
				loadMapPreview(true);

				widgSetString(psWScreen,MULTIOP_MAP,sTemp);
				addGameOptions();
				break;
			default:
				break;
			}
			addPlayerBox( !ingame.bHostSetup );
		}
	}
	else
	{
		if(hideTime != 0)
		{
			// we abort the 'hidetime' on press of a mouse button.
			if(gameTime-hideTime < MAP_PREVIEW_DISPLAY_TIME && !mousePressed(MOUSE_LMB) && !mousePressed(MOUSE_RMB))
			{
				return;
			}
			inputLooseFocus();	// remove the mousepress from the input stream.
			hideTime = 0;
		}

		id = widgRunScreen(psWScreen);								// run the widgets.
		processMultiopWidgets(id);
	}

	widgDisplayScreen(psWScreen);									// show the widgets currently running

	if(multiRequestUp)
	{
		widgDisplayScreen(psRScreen);								// show the Requester running
	}

	if(widgGetFromID(psWScreen,MULTIOP_CHATBOX))
	{
		iV_SetFont(font_regular);											// switch to small font.
		displayConsoleMessages();									// draw the chatbox
	}

	if (CancelPressed())
	{
		if (multiRequestUp)
		{
			changeTitleMode(lastTitleMode);
		}
		else
		{
			processMultiopWidgets(CON_CANCEL);  // "Press" the cancel button to clean up net connections and stuff.
		}
	}
}

bool startMultiOptions(bool bReenter)
{
	PLAYERSTATS		nullStats;
	UBYTE i;

	netPlayersUpdated = true;

	addBackdrop();
	addTopForm();

	if (getLobbyError() != ERROR_CHEAT)
	{
		setLobbyError(ERROR_NOERROR);
	}

	// free limiter structure
	// challengeActive can be removed when challenge will set the limits from some config file
	if(!bReenter || challengeActive)
	{
		if(ingame.numStructureLimits)
		{
			ingame.numStructureLimits = 0;
			free(ingame.pStructureLimits);
			ingame.pStructureLimits = NULL;
		}
	}
	
	if(!bReenter)
	{
		teamChooserUp = -1;
		aiChooserUp = -1;
		difficultyChooserUp = -1;
		positionChooserUp = -1;
		colourChooserUp = -1;
		allowChangePosition = true;
		for(i=0; i < MAX_PLAYERS; i++)
		{
			game.skDiff[i] = (DIFF_SLIDER_STOPS / 2);	// reset AI (turn it on again)
			setPlayerColour(i,i);						//reset all colors as well
		}

		if(!NetPlay.bComms)			// force skirmish if no comms.
		{
			game.type = SKIRMISH;
			game.scavengers = false;
			sstrcpy(game.map, DEFAULTSKIRMISHMAP);
			game.maxPlayers = 4;
		}

		ingame.localOptionsReceived = false;

		loadMultiStats((char*)sPlayer,&nullStats);

		if (challengeActive)
		{
			inifile	*inif = inifile_load(sRequestResult);
			inifile_set_current_section(inif, "challenge");

			resetReadyStatus(false);
			removeWildcards((char*)sPlayer);

			if (!hostCampaign((char*)game.name,(char*)sPlayer))
			{
				debug(LOG_ERROR, "Failed to host the challenge.");
				return false;
			}
			bHosted = true;

			sstrcpy(game.map, inifile_get(inif, "Map", game.map));
			game.maxPlayers = inifile_get_as_int(inif, "MaxPlayers", game.maxPlayers);	// TODO, read from map itself, not here!!
			game.scavengers = inifile_get_as_bool(inif, "Scavengers", game.scavengers);
			game.alliance = ALLIANCES_TEAMS;
			netPlayersUpdated = true;
			mapDownloadProgress = 100;
			game.power = inifile_get_as_int(inif, "challenge:Power", game.power);
			game.base = inifile_get_as_int(inif, "challenge:Bases", game.base + 1) - 1;		// count from 1 like the humans do
			allowChangePosition = inifile_get_as_bool(inif, "challenge:AllowPositionChange", false);

			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				char key[64];
				const char *value;

				ssprintf(key, "player_%d:team", i + 1);
				NetPlay.players[i].team = inifile_get_as_int(inif, key, NetPlay.players[i].team + 1) - 1;
				ssprintf(key, "player_%d:position", i + 1);
				if (inifile_key_exists(inif, key))
				{
					changePosition(i, inifile_get_as_int(inif, key, NetPlay.players[i].position));
				}
				if (i != 0)	// player index zero is always the challenger
				{
					ssprintf(key, "player_%d:difficulty", i + 1);
					value = inifile_get(inif, key, "Medium");
					for (int j = 0; j < ARRAY_SIZE(difficultyList); j++)
					{
						if (strcasecmp(difficultyList[j], value) == 0)
						{
							game.skDiff[i] = difficultyValue[j];
						}
					}
				}
			}

			inifile_delete(inif);

			ingame.localOptionsReceived = true;
			addGameOptions();									// update game options box.
			addChatBox();
			disableMultiButs();
			addPlayerBox(true);
		}
	}

	addPlayerBox(false);								// Players
	addGameOptions();

	addChatBox();

	// going back to multiop after setting limits up..
	if(bReenter && bHosted)
	{
		disableMultiButs();
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Drawing functions

void displayChatEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y -4;			// 4 is the magic number.

	// draws the line at the bottom of the multiplayer join dialog separating the chat
	// box from the input box
	iV_Line(x, y, x + psWidget->width, y, WZCOL_MENU_SEPARATOR);

	return;
}

// ////////////////////////////////////////////////////////////////////////////
void displayRemoteGame(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	UDWORD	i = psWidget->UserData;
	char tmp[80], gamename[StringSize];
	unsigned int ping;

	if (LobbyError != ERROR_NOERROR)
	{
		return;
	}

	// Draw blue boxes.
	drawBlueBox(x,y,psWidget->width,psWidget->height);
	drawBlueBox(x,y,94,psWidget->height);
	drawBlueBox(x,y,55,psWidget->height);

	//draw game info
	iV_SetFont(font_regular);													// font

	// get game info.
	// TODO: Check whether this code is used at all in skirmish games, if not, remove it.
	if ((NetPlay.games[i].desc.dwFlags & SESSION_JOINDISABLED)
		|| strcmp(NetPlay.games[i].modlist,getModList()) != 0
		|| (bMultiPlayer
			&& !NetPlay.bComms
			&& NETgetGameFlagsUnjoined(gameNumber,1) == SKIRMISH                                  // the LAST bug...
			&& NetPlay.games[gameNumber].desc.dwCurrentPlayers >= NetPlay.games[gameNumber].desc.dwMaxPlayers - 1))
	{
		iV_SetTextColour(WZCOL_TEXT_MEDIUM);
		// FIXME: We should really use another way to indicate that the game is full than our current big fat cross.
		// need some sort of closed thing here!
		iV_DrawImage(FrontImages,IMAGE_NOJOIN,x+18,y+11);
	}
	else if (!NETisCorrectVersion(NetPlay.games[i].game_version_minor, NetPlay.games[i].game_version_major))
	{
		if (NetPlay.games[gameNumber].desc.dwCurrentPlayers >= NetPlay.games[gameNumber].desc.dwMaxPlayers)
		{
			iV_SetTextColour(WZCOL_TEXT_MEDIUM);
		}
		else
		{
			iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		}
		iV_DrawText(_("Players"), x + 5, y + 18);
		ssprintf(tmp, "%d/%d", NetPlay.games[i].desc.dwCurrentPlayers, NetPlay.games[i].desc.dwMaxPlayers);
		iV_DrawText(tmp, x + 17, y + 33);
	}
	else
	{	//don't allow people to join games frome a different version of the game.
		iV_SetTextColour(WZCOL_TEXT_MEDIUM);
		// FIXME: Need a Wrong version icon!
		iV_DrawImage(FrontImages,IMAGE_NOJOIN,x+18,y+11);
	}
	
	//draw type overlay.
	if( NETgetGameFlagsUnjoined(i,1) == CAMPAIGN)
	{
		iV_DrawImage(FrontImages, IMAGE_ARENA_OVER, x + 59, y + 3);
	}
	else if (NetPlay.games[i].privateGame)	// check to see if it is a private game
	{
		iV_DrawImage(FrontImages, IMAGE_LOCKED_NOBG, x+62, y+3);	// lock icon
	}
	else
	{
		iV_DrawImage(FrontImages, IMAGE_SKIRMISH_OVER, x+62, y+3);	// SKIRMISH
	}

	// ping rating
	ping = NETgetGameFlagsUnjoined(i, 2);
	if (ping < PING_MED)
	{
		iV_DrawImage(FrontImages,IMAGE_LAMP_GREEN,x+70,y+26);
	}
	else if (ping >= PING_MED && ping < PING_HI)
	{
		iV_DrawImage(FrontImages,IMAGE_LAMP_AMBER,x+70,y+26);
	}
	else
	{
		iV_DrawImage(FrontImages,IMAGE_LAMP_RED,x+70,y+26);
	}

	//draw game name
	sstrcpy(gamename, NetPlay.games[i].name);
	while(iV_GetTextWidth(gamename) > (psWidget->width-110) )
	{
		gamename[strlen(gamename)-1]='\0';
	}
	iV_DrawText(gamename, x + 100, y + 18);	// name

	iV_SetFont(font_small);											// font
	iV_DrawText(NetPlay.games[i].versionstring, x + 100, y + 32);	// version
}

void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	UDWORD		i = psWidget->UserData;

	ASSERT(i < MAX_PLAYERS && NetPlay.players[i].team >= 0 && NetPlay.players[i].team < MAX_PLAYERS, "Team index out of bounds" );

	drawBlueBox(x,y,psWidget->width,psWidget->height);							// right

	if (game.skDiff[i])
	{
		iV_DrawImage(FrontImages, IMAGE_TEAM0 + NetPlay.players[i].team, x + 2, y + 8);
	}
}

void displayPosition(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	const int x = xOffset + psWidget->x;
	const int y = yOffset + psWidget->y;
	const int i = psWidget->UserData;
	char text[80];

	drawBlueBox(x, y, psWidget->width, psWidget->height);
	iV_SetFont(font_regular);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	ssprintf(text, "Click to take player slot %d", NetPlay.players[i].position);
	iV_DrawText(text, x + 10, y + 22);
}

static void displayAi(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	const int x = xOffset + psWidget->x;
	const int y = yOffset + psWidget->y;
	const int j = psWidget->UserData;
	const char *commsText[] = { "Open", "Closed" };

	drawBlueBox(x, y, psWidget->width, psWidget->height);
	iV_SetFont(font_regular);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawText((j >= 0) ? aidata[j].name : commsText[j + 2], x + 10, y + 22);
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

static void displayDifficulty(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	const int x = xOffset + psWidget->x;
	const int y = yOffset + psWidget->y;
	const int j = psWidget->UserData;

	ASSERT_OR_RETURN(, j < ARRAY_SIZE(difficultyList), "Bad difficulty found: %d", j);
	drawBlueBox(x, y, psWidget->width, psWidget->height);
	iV_SetFont(font_regular);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawImage(FrontImages, difficultyIcon(j), x + 5, y + 5);
	iV_DrawText(difficultyList[j], x + 42, y + 22);
}

// ////////////////////////////////////////////////////////////////////////////
void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	UDWORD		j = psWidget->UserData, eval;
	PLAYERSTATS stat;

	const int nameX = 32;

	//bluboxes.
	drawBlueBox(x,y,psWidget->width,psWidget->height);							// right
	if (NetPlay.isHost && NetPlay.players[j].wzFile.isSending)
	{
		static char mapProgressString[MAX_STR_LENGTH] = {'\0'};
		int progress = (NetPlay.players[j].wzFile.currPos * 100) / NetPlay.players[j].wzFile.fileSize_32;

		snprintf(mapProgressString, MAX_STR_LENGTH, _("Sending Map: %d%% "), progress);
		iV_SetFont(font_regular); // font
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		iV_DrawText(mapProgressString, x + 15, y + 22);
	}
	else if (mapDownloadProgress != 100 && j == selectedPlayer)
	{
		static char mapProgressString[MAX_STR_LENGTH] = {'\0'};
		snprintf(mapProgressString, MAX_STR_LENGTH, _("Map: %d%% downloaded"), mapDownloadProgress);
		iV_SetFont(font_regular); // font
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		iV_DrawText(mapProgressString, x + 5, y + 22);
		return;
	}
	else if (ingame.localOptionsReceived && NetPlay.players[j].allocated)					// only draw if real player!
	{
		drawBlueBox(x,y,psWidget->width,psWidget->height);							// right

		iV_SetFont(font_regular);											// font
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);

		// name
		std::string name = NetPlay.players[j].name;
		if (iV_GetTextWidth(name.c_str()) > psWidget->width - nameX)
		{
			while (!name.empty() && iV_GetTextWidth((name + "...").c_str()) > psWidget->width - nameX)
			{
				name.resize(name.size() - 1);  // Clip name.
			}
			name += "...";
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
			ssprintf(buf, "%s%s: %03d", subText.empty()? "" : ", ", _("Ping"), ingame.PingTimes[j]);
			subText += buf;
		}
		iV_DrawText(name.c_str(), x + nameX, y + (subText.empty()? 22 : 18));
		if (!subText.empty())
		{
			iV_SetFont(font_small);
			iV_SetTextColour(WZCOL_TEXT_MEDIUM);
			iV_DrawText(subText.c_str(), x + nameX, y + 28);
			iV_SetFont(font_regular);
			iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		}
		
		if(getMultiStats(j).played < 5)
		{
			iV_DrawImage(FrontImages, IMAGE_MEDAL_DUMMY, x + 4, y + 13);
		}
		else
		{
			stat = getMultiStats(j);

			// star 1 total droid kills
			eval = stat.totalKills;
			if(eval >600)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK1, x + 4, y + 3);
			}
			else if(eval >300)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK2, x + 4, y + 3);
			}
			else if(eval >150)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK3, x + 4, y + 3);
			}

			// star 2 games played
			eval = stat.played;
			if(eval >200)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK1, x + 4, y + 13);
			}
			else if(eval >100)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK2, x + 4, y + 13);
			}
			else if(eval >50)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK3, x + 4, y + 13);
			}

			// star 3 games won.
			eval = stat.wins;
			if(eval >80)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK1, x + 4, y + 23);
			}
			else if(eval >40)
			{
				iV_DrawImage(FrontImages, IMAGE_MULTIRANK2, x + 4, y + 23);
			}
			else if(eval >10)
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
		iV_SetFont(font_regular);
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		ASSERT_OR_RETURN(, NetPlay.players[j].ai < (int)aidata.size(), "Uh-oh, AI index out of bounds");
		switch (NetPlay.players[j].ai)
		{
		case AI_OPEN: sstrcpy(aitext, "Open"); break;
		case AI_CLOSED: sstrcpy(aitext, "Closed"); break;
		default: sstrcpy(aitext, aidata[NetPlay.players[j].ai].name); break;
		}
		iV_DrawText(aitext, x + nameX, y + 22);
	}
}

void displayColour(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	const int x = xOffset + psWidget->x;
	const int y = yOffset + psWidget->y;
	const int j = psWidget->UserData;

	drawBlueBox(x, y, psWidget->width, psWidget->height);
	if (!NetPlay.players[j].wzFile.isSending && game.skDiff[j])
	{
		int player = getPlayerColour(j);
		STATIC_ASSERT(MAX_PLAYERS <= 16);
		iV_DrawImageTc(FrontImages, IMAGE_PLAYERN, IMAGE_PLAYERN_TC, x + 7, y + 9, pal_GetTeamColour(player));
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Display blue box
void intDisplayFeBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UDWORD	w = psWidget->width;
	UDWORD	h = psWidget->height;

	drawBlueBox(x,y,w,h);
}

// ////////////////////////////////////////////////////////////////////////////
// Display edit box
void displayMultiEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;

	drawBlueBox(x,y,psWidget->width,psWidget->height);

	if( ((W_EDITBOX*)psWidget)->state & WEDBS_DISABLE)					// disabled
	{
		PIELIGHT colour;

		colour.byte.r = FILLRED;
		colour.byte.b = FILLBLUE;
		colour.byte.g = FILLGREEN;
		colour.byte.a = FILLTRANS;
		pie_UniTransBoxFill(x, y, x + psWidget->width + psWidget->height, y+psWidget->height, colour);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Display one of two images depending on if the widget is hilighted by the mouse.
void displayMultiBut(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	bool	Hilight = false;
	UDWORD	Down = 0;
	UDWORD	Grey = 0;
	UWORD	im_norm = UNPACKDWORD_QUAD_A((UDWORD)psWidget->UserData);
	UWORD	im_down = UNPACKDWORD_QUAD_B((UDWORD)psWidget->UserData);
	UWORD	im_hili = UNPACKDWORD_QUAD_C((UDWORD)psWidget->UserData);
	unsigned tc     = UNPACKDWORD_QUAD_D(psWidget->UserData);
	UWORD	hiToUse = im_hili;

	// FIXME: This seems to be a way to conserve space, so you can use a
	// transparent icon with these edit boxes.
	// hack for multieditbox
	if (im_norm == IMAGE_EDIT_MAP || im_norm == IMAGE_EDIT_GAME || im_norm == IMAGE_EDIT_PLAYER
		|| im_norm == IMAGE_LOCK_BLUE || im_norm == IMAGE_UNLOCK_BLUE)
	{
		drawBlueBox(x - 2, y - 2, psWidget->height, psWidget->height);	// box on end.
	}

	// evaluate auto-frame
	if (((W_BUTTON*)psWidget)->state & WBUTS_HILITE)
	{
		Hilight = true;
	}

	// evaluate auto-frame
	if (im_hili == 1 && Hilight && im_norm != IMAGE_WEE_GUY)
	{
		Hilight = true;
		switch(iV_GetImageWidth(FrontImages, im_norm))			//pick a hilight.
		{
		case 30:
			hiToUse = IMAGE_HI34;
			break;
		case 60:
			hiToUse = IMAGE_HI64;
			break;
		case 19:
			hiToUse = IMAGE_HI23;
			break;
		case 27:
			hiToUse = IMAGE_HI31;
			break;
		case 35:
			hiToUse = IMAGE_HI39;
			break;
		case 37:
			hiToUse = IMAGE_HI41;
			break;
		case 56:
			hiToUse = IMAGE_HI56;
			break;
		default:
			hiToUse = 0;
			break;
		}
	}

	if (im_norm == IMAGE_WEE_GUY)
	{
		// fugly hack for adding player number to the wee guy (whoever that is)
		iV_DrawImage(IntImages, IMAGE_ASCII48 - 10 + im_hili, x + 11, y + 8);
		Hilight = false;
	}

	if( ((W_BUTTON*)psWidget)->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK))
	{
		Down = 1;
	}

	if( ((W_BUTTON*)psWidget)->state & WBUTS_GREY)
	{
		Grey = 1;
	}

	int toDraw[3];
	int numToDraw = 0;

	// now display
	toDraw[numToDraw++] = im_norm;

	// hilights etc..
	if (Down)
	{
		toDraw[numToDraw++] = im_down;
	}
	if(Hilight && !Grey && hiToUse)
	{
		toDraw[numToDraw++] = hiToUse;
	}

	for (int n = 0; n < numToDraw; ++n)
	{
		if (tc == MAX_PLAYERS)
		{
			iV_DrawImage(FrontImages, toDraw[n], x, y);
		}
		else
		{
			iV_DrawImageTc(FrontImages, toDraw[n], toDraw[n] + 1, x, y, pal_GetTeamColour(tc));
		}
	}

	if (Grey)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x, y, x + psWidget->width, y + psWidget->height);
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

bool addMultiBut(W_SCREEN *screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char* tipres, UDWORD norm, UDWORD down, UDWORD hi, unsigned tc)
{
	W_BUTINIT sButInit;
	sButInit.formID = formid;
	sButInit.id = id;
	sButInit.x = (short) x;
	sButInit.y = (short) y;
	sButInit.width = (unsigned short) width;
	sButInit.height= (unsigned short) height;
	sButInit.pTip = tipres;
	sButInit.pDisplay = displayMultiBut;
	sButInit.UserData = PACKDWORD_QUAD(norm, down, hi, tc);

	return widgAddButton(screen, &sButInit);
}

/*
 * Set skirmish/multiplayer alliance mode to 'Locked teams',
 * update GUI accordingly and notify other players
 */
void setLockedTeamsMode(void)
{
	widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N,0);
	widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y,0);
	widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_TEAMS,WBUT_LOCK);
	game.alliance = ALLIANCES_TEAMS;		//2
	netPlayersUpdated = true;
	if(bHosted)
	{
		sendOptions();
	}
}

/* Returns true if all human players clicked on the 'ready' button */
bool multiplayPlayersReady(bool bNotifyStatus)
{
	unsigned int	player,playerID;
	bool			bReady;

	bReady = true;

	for(player = 0; player < game.maxPlayers; player++)
	{
		// check if this human player is ready, ignore AIs
		if (NetPlay.players[player].allocated && !NetPlay.players[player].ready)
		{
			if(bNotifyStatus)
			{
				for (playerID = 0; playerID <= game.maxPlayers && playerID != player; playerID++) ;

				console("%s is not ready", getPlayerName(playerID));
			}

			bReady = false;
		}
	}

	return bReady;
}
