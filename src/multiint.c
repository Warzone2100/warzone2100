/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#include <GLee.h>
#include "lib/framework/frame.h"

#include <time.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/frameint.h"
#include "lib/framework/file.h"
#include "lib/framework/stdio_ext.h"

/* Includes direct access to render library */
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/pieclip.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/rendmode.h"
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

#include "warzoneconfig.h"

#include "init.h"
#include "levels.h"

#if defined(WZ_OS_MAC)
#include <QuesoGLC/glc.h>
#else
#include <GL/glc.h>
#endif

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

// ////////////////////////////////////////////////////////////////////////////
// vars
extern char	MultiCustomMapsPath[PATH_MAX];
extern char	MultiPlayersPath[PATH_MAX];
extern char VersionString[80];		// from netplay.c
extern BOOL bSendingMap;			// used to indicate we are sending a map

extern void intDisplayTemplateButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
extern void NETsetGamePassword(const char *password);	// in netplay.c
extern void NETresetGamePassword(void);					// in netplay.c
extern void NETGameLocked(bool flag);					// in netplay.c
extern void NETsetGamePassword(const char *password);	// in netplay.c
extern void NETresetGamePassword(void);					// in netplay.c
extern void NETGameLocked(bool flag);					// in netplay.c
extern void NETsetGamePassword(const char *password);	// in netplay.c
extern void NETresetGamePassword(void);					// in netplay.c
extern void NETGameLocked(bool flag);					// in netplay.c

BOOL						bHosted			= false;				//we have set up a game
char						sPlayer[128];							// player name (to be used)
static int					colourChooserUp = -1;
static int					teamChooserUp = -1;
static BOOL				SettingsUp		= false;
static UBYTE				InitialProto	= 0;
static W_SCREEN				*psConScreen;
static SDWORD				dwSelectedGame	=0;						//player[] and games[] indexes
static UDWORD				gameNumber;								// index to games icons
static BOOL					safeSearch		= false;				// allow auto game finding.
static bool disableLobbyRefresh = false;	// if we allow lobby to be refreshed or not.
static UDWORD hideTime=0;
static bool EnablePasswordPrompt = false;	// if we need the password prompt
extern int NET_PlayerConnectionStatus;		// from src/display3d.c
LOBBY_ERROR_TYPES LobbyError = ERROR_NOERROR;
static BOOL allowChangePosition = true;
static char tooltipbuffer[MaxGames][256];
/// end of globals.
// ////////////////////////////////////////////////////////////////////////////
// Function protos

// widget functions
static BOOL addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, const char* tip, char tipres[128], UDWORD icon, UDWORD iconhi, UDWORD iconid);
static void addBlueForm					(UDWORD parent,UDWORD id, const char *txt,UDWORD x,UDWORD y,UDWORD w,UDWORD h);
static void drawReadyButton(UDWORD player);
static void displayPasswordEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours);

// Drawing Functions
void		displayChatEdit				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayMultiBut				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		intDisplayFeBox				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayRemoteGame			(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayPlayer				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayTeamChooser			(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayMultiEditBox			(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		setLockedTeamsMode			(void);

// find games
static void addGames				(void);
static void removeGames				(void);
void		runGameFind				(void);
void		startGameFind			(void);

// password form functions
static void hidePasswordForm(void);
static void showPasswordForm(void);

// Connection option functions
void		runConnectionScreen		(void);
BOOL		startConnectionScreen	(void);

// Game option functions
static	void	addGameOptions		(BOOL bRedo);				// options (rhs) boxV
UDWORD	addPlayerBox		(BOOL);				// players (mid) box
static	void	addChatBox			(void);
static	void	disableMultiButs	(void);
static	void	processMultiopWidgets(UDWORD);
static	void	SendFireUp			(void);

void			runMultiOptions		(void);
BOOL			startMultiOptions	(BOOL);
void			frontendMultiMessages(void);

static	UDWORD	bestPlayer			(UDWORD);
static	void	decideWRF			(void);

static void		closeColourChooser	(void);
static void		closeTeamChooser	(void);
static BOOL		SendColourRequest	(UBYTE player, UBYTE col);
static BOOL		SendPositionRequest	(UBYTE player, UBYTE chosenPlayer);
static BOOL		safeToUseColour		(UDWORD player,UDWORD col);
static BOOL		changeReadyStatus	(UBYTE player, BOOL bReady);
void			resetReadyStatus	(bool bSendOptions);
void			initTeams( void );
static	void stopJoining(void);
// ////////////////////////////////////////////////////////////////////////////
// map previews..

static int guessMapTilesetType(void)
{
	if (terrainTypes[0] == 1 && terrainTypes[1] == 0 && terrainTypes[2] == 2)
	{
		return TILESET_ARIZONA;
	}
	else if (terrainTypes[0] == 2 && terrainTypes[1] == 2 && terrainTypes[2] == 2)
	{
		return TILESET_URBAN;
	}
	else if (terrainTypes[0] == 0 && terrainTypes[1] == 0 && terrainTypes[2] == 2)
	{
		return TILESET_ROCKIES;
	}
	else
	{
		debug(LOG_MAP, "Custom level dataset: %u %u %u, using ARIZONA set",
			terrainTypes[0], terrainTypes[1], terrainTypes[2]);
		return TILESET_ARIZONA;
	}
}

/// This function is a HACK
/// Loads the entire map (including calculating gateways) just to show
/// a picture of it
void loadMapPreview(bool hideInterface)
{
	static char			aFileName[256], bFileName[256];
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
	ASSERT_OR_RETURN(, psLevel, "Could not find level dataset!");
	rebuildSearchPath(psLevel->dataDir, false);
	sstrcpy(aFileName,psLevel->apDataFiles[0]);
	aFileName[strlen(aFileName)-4] = '\0';
	sstrcat(aFileName, "/ttypes.ttp");
	pFileData = fileLoadBuffer;
	sstrcpy(bFileName, screen_getMapName());
	if (!sstrcmp(aFileName, bFileName))
	{
		if (hideInterface)
		{
			hideTime = gameTime;
		}
		return;
	}
	sstrcpy(bFileName, aFileName);
	if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_ERROR, "loadMapPreview: Failed to load terrain types file");
		return;
	}
	if (pFileData)
	{
		if (!loadTerrainTypeMap(pFileData, fileSize))
		{
			debug(LOG_ERROR, "loadMapPreview: Failed to load terrain types");
			return;
		}
	}

	// load the map data
	ptr = strrchr(aFileName, '/');
	ASSERT(ptr, "this string was supposed to contain a /");
	strcpy(ptr, "/game.map");
	if (!mapLoad(aFileName))
	{
		debug(LOG_ERROR, "loadMapPreview: Failed to load map");
		return;
	}
	gwShutDown();

	// set tileset colors
	switch (guessMapTilesetType())
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
			height = WTile->height;
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

	screen_enableMapPreview(bFileName, mapWidth, mapHeight, playerpos);

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

static BOOL OptionsInet(void)			//internet options
{
	W_EDBINIT		sEdInit;
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;
	W_CONTEXT		sContext;

	psConScreen = widgCreateScreen();
	widgSetTipFont(psConScreen,font_regular);

	memset(&sFormInit, 0, sizeof(W_FORMINIT));		//Connection Settings
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
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	sLabInit.formID = CON_SETTINGS;
	sLabInit.id		= CON_SETTINGS_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 10;
	sLabInit.width	= CON_SETTINGSWIDTH;
	sLabInit.height = 20;
	sLabInit.pText	= _("IP Address or Machine Name");
	sLabInit.FontID = font_regular;
	widgAddLabel(psConScreen, &sLabInit);


	memset(&sEdInit, 0, sizeof(W_EDBINIT));			// address
	sEdInit.formID = CON_SETTINGS;
	sEdInit.id = CON_IP;
	sEdInit.style = WEDB_PLAIN;
	sEdInit.x = CON_IPX;
	sEdInit.y = CON_IPY;
	sEdInit.width = CON_NAMEBOXWIDTH;
	sEdInit.height = CON_NAMEBOXHEIGHT;
	sEdInit.pText = "";									//_("IP Address or Machine Name");
	sEdInit.FontID = font_regular;
//	sEdInit.pUserData = (void*)PACKDWORD_TRI(0,IMAGE_DES_EDITBOXLEFTH , IMAGE_DES_EDITBOXLEFT);
//	sEdInit.pBoxDisplay = intDisplayButtonHilight;
	sEdInit.pBoxDisplay = intDisplayEditBox;
	if (!widgAddEditBox(psConScreen, &sEdInit))
	{
		return false;
	}
	// auto click in the text box
	sContext.psScreen	= psConScreen;
	sContext.psForm		= (W_FORM *)psConScreen->psForm;
	sContext.xOffset	= 0;
	sContext.yOffset	= 0;
	sContext.mx			= 0;
	sContext.my			= 0;
	editBoxClicked((W_EDITBOX*)widgGetFromID(psConScreen,CON_IP), &sContext);

	SettingsUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Draw the connections screen.
BOOL startConnectionScreen(void)
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


void runConnectionScreen(void )
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
			NETsetupTCPIP(""); //inet
			if (LobbyError != ERROR_CHEAT)
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

			if(SettingsUp == true)
			{
				widgReleaseScreen(psConScreen);
				SettingsUp = false;
			}

			NETsetupTCPIP(addr); //inet

			changeTitleMode(GAMEFIND);
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

// ////////////////////////////////////////////////////////////////////////////
// Game Chooser Screen.

static void addGames(void)
{
	UDWORD i,gcount=0;
	W_BUTINIT	sButInit;
	static const char *wrongVersionTip = "Your version of Warzone is incompatible with this game.";
	static const char *badModTip = "Your loaded mods are incompatible with this game. (Check mods/autoload/?)";

	memset(tooltipbuffer, 0, sizeof(tooltipbuffer));

	//count games to see if need two columns.
	for(i=0;i<MaxGames;i++)							// draw games
	{
		if( NetPlay.games[i].desc.dwSize !=0)
		{
			gcount++;
		}
	}
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.style = WBUT_PLAIN;
	sButInit.width = GAMES_GAMEWIDTH;
	sButInit.height = GAMES_GAMEHEIGHT;
	sButInit.FontID = font_regular;
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
				if (!NETgameIsCorrectVersion(&NetPlay.games[i]))
				{
					sButInit.pTip = wrongVersionTip;
				}
				else if (strcmp(NetPlay.games[i].modlist,getModList()) != 0)
				{
					sButInit.pTip = badModTip;
				}
				else
				{
					ssprintf(tooltipbuffer[i], "Map:%s, Game:%s, Hosted by %s ", NetPlay.games[i].mapname, NetPlay.games[i].name, NetPlay.games[i].hostname);
					sButInit.pTip = tooltipbuffer[i];
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
		// In other words, it happens all the time when people put their maps in the wrong directory. :P
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

		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = FRONTEND_BOTFORM;
		sButInit.id = FRONTEND_NOGAMESAVAILABLE;
		sButInit.x = 70;
		sButInit.y = 50;
		sButInit.style = WBUT_PLAIN | WBUT_TXTCENTRE;
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
			// TODO: Check whether this code is used at all in skirmish games, if not, remove it.
			// if skirmish, check it wont take the last slot
			// We also now check for the version string, to not allow people to join in the first place
			if ((bMultiPlayer
			 && !NetPlay.bComms
			 && NETgetGameFlagsUnjoined(gameNumber,1) == SKIRMISH
			 && (NetPlay.games[gameNumber].desc.dwCurrentPlayers >= NetPlay.games[gameNumber].desc.dwMaxPlayers - 1))
				|| (!NETgameIsCorrectVersion(&NetPlay.games[gameNumber]) != 0 ))
			{
				goto FAIL;
			}

			if (NetPlay.games[gameNumber].privateGame)
			{
				showPasswordForm();
			}
			else
			{
				ingame.localOptionsReceived = false;			// note we are awaiting options
				sstrcpy(game.name, NetPlay.games[gameNumber].name);		// store name

				if (joinCampaign(gameNumber,(char*)sPlayer))
				{
					changeTitleMode(MULTIOPTION);
				}
				else
				{
					if (!getLobbyError())
					{
						setLobbyError(ERROR_CONNECTION);
					}
					addGames();
				}
			}
		}

	}
	else if (id == CON_PASSWORDYES)
	{
		ingame.localOptionsReceived = false;			// note we are awaiting options
		sstrcpy(game.name, NetPlay.games[gameNumber].name);		// store name

		if (joinCampaign(gameNumber,(char*)sPlayer))
		{
			changeTitleMode(MULTIOPTION);
		}
		else
		{
			if (!getLobbyError())
			{
				setLobbyError(ERROR_CONNECTION);
			}
			hidePasswordForm();
		}
	}
	else if (id == CON_PASSWORDNO)
	{
		hidePasswordForm();
	}

FAIL:

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
	W_FORMINIT	sFormInit;
	W_EDBINIT	sEdInit;
	W_LABINIT	sLabInit;

	addBackdrop();										//background image

	// draws the background of the games listed
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
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
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	sLabInit.formID = FRONTEND_BACKDROP;
	sLabInit.id		= CON_PASSWORD_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 180;
	sLabInit.y		= 195;
	sLabInit.width	= CON_SETTINGSWIDTH;
	sLabInit.height = 20;
	sLabInit.pText	= _("Enter Password:");
	sLabInit.FontID = font_regular;
	sLabInit.pDisplay = showPasswordLabel;
	widgAddLabel(psWScreen, &sLabInit);

	// and finally draw the password entry box
	memset(&sEdInit, 0, sizeof(W_EDBINIT));
	sEdInit.formID = FRONTEND_BACKDROP;
	sEdInit.id = CON_PASSWORD;
	sEdInit.style = WEDB_PLAIN;
	sEdInit.x = 180;
	sEdInit.y = 200;
	sEdInit.width = 280;
	sEdInit.height = 20;
	sEdInit.pText = "";
	sEdInit.FontID = font_regular;
	sEdInit.pBoxDisplay = displayPasswordEditBox;

	widgAddEditBox(psWScreen, &sEdInit);

	addMultiBut(psWScreen, FRONTEND_BACKDROP,CON_PASSWORDYES,230,225,MULTIOP_OKW,MULTIOP_OKH,
	            _("OK"),IMAGE_OK,IMAGE_OK,true);
	addMultiBut(psWScreen, FRONTEND_BACKDROP,CON_PASSWORDNO,280,225,MULTIOP_OKW,MULTIOP_OKH,
	            _("Cancel"),IMAGE_NO,IMAGE_NO,true);

	// draws the background of the password box
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
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
	editBoxClicked((W_EDITBOX*)widgGetFromID(psWScreen,CON_PASSWORD), &sContext);
}


// ////////////////////////////////////////////////////////////////////////////
// Game Options Screen.

// ////////////////////////////////////////////////////////////////////////////

static void addBlueForm(UDWORD parent,UDWORD id, const char *txt,UDWORD x,UDWORD y,UDWORD w,UDWORD h)
{
	W_FORMINIT	sFormInit;
	W_LABINIT	sLabInit;

	memset(&sFormInit, 0, sizeof(W_FORMINIT));				// draw options box.
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
		memset(&sLabInit, 0, sizeof(W_LABINIT));
		sLabInit.formID = id;
		sLabInit.id		= id+1;
		sLabInit.style	= WLAB_PLAIN;
		sLabInit.x		= 3;
		sLabInit.y		= 4;
		sLabInit.width	= 80;
		sLabInit.height = 20;
		sLabInit.pText	= txt;
//		sLabInit.pDisplay = displayFeText;
		sLabInit.FontID = font_regular;
		widgAddLabel(psWScreen, &sLabInit);
	}
	return;
}


typedef struct
{
	char const *stat;
	char const *desc;
	int         icon;
} LimitIcon;
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

// FIX ME: bRedo is not used anymore since the removal of the forced screenClearFocus()
// need to check for side effects.
static void addGameOptions(BOOL bRedo)
{
	W_FORMINIT		sFormInit;

	widgDelete(psWScreen,MULTIOP_OPTIONS);  				// clear options list
	widgDelete(psWScreen,FRONTEND_SIDETEXT3);				// del text..

	iV_SetFont(font_regular);

	memset(&sFormInit, 0, sizeof(W_FORMINIT));				// draw options box.
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

	if (game.maxPlayers == 8)
	{
		widgSetButtonState(psWScreen, MULTIOP_SKIRMISH, WBUT_LOCK);
		widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_DISABLE);	// full, cannot enable scavenger player
		game.scavengers = false;
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

static BOOL safeToUseColour(UDWORD player,UDWORD col)
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

static void addColourChooser(UDWORD player)
{
	UDWORD i;

	// delete that players box,
	widgDelete(psWScreen,MULTIOP_PLAYER_START+player);

	// delete team chooser button
	widgDelete(psWScreen,MULTIOP_TEAMS_START+player);

	// delete 'ready' button
	widgDelete(psWScreen,MULTIOP_READY_FORM_ID+player);

	// remove colour chooser, if it's already up
	closeColourChooser();

	// add form.
	addBlueForm(MULTIOP_PLAYERS,MULTIOP_COLCHOOSER_FORM,"",
				7,
				((MULTIOP_PLAYERHEIGHT+5)*NetPlay.players[player].position)+4,
				MULTIOP_ROW_WIDTH,MULTIOP_PLAYERHEIGHT);

	// add the flags
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		addMultiBut(psWScreen,MULTIOP_COLCHOOSER_FORM, MULTIOP_COLCHOOSER+i,
			(i*(iV_GetImageWidth(FrontImages,IMAGE_PLAYER0) +5)+7) ,//x
			4,													  //y
			iV_GetImageWidth(FrontImages,IMAGE_PLAYER0),		  //w
			iV_GetImageHeight(FrontImages,IMAGE_PLAYER0),		  //h
			_("Player colour"), IMAGE_PLAYER0 + i, IMAGE_PLAYER0_HI + i, IMAGE_PLAYER0_HI + i);

			if( !safeToUseColour(selectedPlayer,i))
			{
				widgSetButtonState(psWScreen,MULTIOP_COLCHOOSER+i ,WBUT_DISABLE);
			}
	}

	// add a kick button
	if (player != selectedPlayer)
	{
		addMultiBut(psWScreen,MULTIOP_COLCHOOSER_FORM, MULTIOP_COLCHOOSER_KICK,
					(8*(iV_GetImageWidth(FrontImages,IMAGE_PLAYER0) +5)+7) ,//x
					4,													  //y
					iV_GetImageWidth(FrontImages,IMAGE_NOJOIN),		  //w
					iV_GetImageHeight(FrontImages,IMAGE_NOJOIN),		  //h
					_("Kick player"), IMAGE_NOJOIN, IMAGE_NOJOIN, IMAGE_NOJOIN);
	}

	//add the position chooser.
	for (i = 0; i < game.maxPlayers && allowChangePosition; i++)
	{
		addMultiBut(psWScreen,MULTIOP_COLCHOOSER_FORM, MULTIOP_PLAYCHOOSER+i,
					(i*(iV_GetImageWidth(FrontImages,IMAGE_PLAYER0) +5)+7),//x
					23,													  //y
					iV_GetImageWidth(FrontImages,IMAGE_WEE_GUY)+7,		  //w
					iV_GetImageHeight(FrontImages,IMAGE_WEE_GUY),		  //h
					_("Player number"), IMAGE_WEE_GUY, IMAGE_WEE_GUY, 10 + i);
	}

	if (!NetPlay.isHost)
	{
		for (i=0;i<game.maxPlayers;i++)
		{
			if (isHumanPlayer(i) && i!=selectedPlayer )
			{
				widgSetButtonState(psWScreen, MULTIOP_PLAYCHOOSER+NetPlay.players[i].position, WBUT_DISABLE);
			}
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
	NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);
	netPlayersUpdated = true;
}

static BOOL SendTeamRequest(UBYTE player, UBYTE chosenTeam)
{
	if(NetPlay.isHost)			// do or request the change.
	{
		changeTeam(player, chosenTeam);	// do the change, remember only the host can do this to avoid confusion.
	}
	else
	{
		NETbeginEncode(NET_TEAMREQUEST, NET_HOST_ONLY);

		NETuint8_t(&player);
		NETuint8_t(&chosenTeam);

		NETend();

	}
	return true;
}

BOOL recvTeamRequest()
{
	UBYTE	player, team;

	if(!NetPlay.isHost)			// only host should act
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(NET_TEAMREQUEST);
	NETuint8_t(&player);
	NETuint8_t(&team);
	NETend();

	if (player > MAX_PLAYERS || team > MAX_PLAYERS)
	{
		debug(LOG_NET, "NET_TEAMREQUEST invalid, player %d team, %d", (int) player, (int) team);
		debug(LOG_ERROR, "Invalid NET_TEAMREQUEST from player %d: Tried to change player %d (team %d)",
		      NETgetSource(), (int)player, (int)team);
		return false;
	}

	if (whosResponsible(player) != NetMsg.source)
	{
		HandleBadParam("NET_TEAMREQUEST given incorrect params.", player, NetMsg.source);
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

static BOOL SendReadyRequest(UBYTE player, BOOL bReady)
{
	if(NetPlay.isHost)			// do or request the change.
	{
		return changeReadyStatus(player, bReady);
	}
	else
	{
		NETbeginEncode(NET_READY_REQUEST, NET_HOST_ONLY);
			NETuint8_t(&player);
			NETbool(&bReady);
		NETend();
	}
	return true;
}

BOOL recvReadyRequest()
{
	UBYTE	player;
	BOOL	bReady;

	if(!NetPlay.isHost)					// only host should act
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(NET_READY_REQUEST);
		NETuint8_t(&player);
		NETbool(&bReady);
	NETend();

	if (player > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_READY_REQUEST from player %d: player id = %d",
		      NETgetSource(), (int)player);
		return false;
	}

	if (whosResponsible(player) != NetMsg.source)
	{
		HandleBadParam("NET_READY_REQUEST given incorrect params.", player, NetMsg.source);
		return false;
	}

	// do not allow players to select 'ready' if we are sending a map too them!
	// TODO: make a new icon to show this state?
	if (NetPlay.players[player].wzFile.isSending)
	{
		return false;
	}

	return changeReadyStatus(player, bReady);
}

static BOOL changeReadyStatus(UBYTE player, BOOL bReady)
{
	drawReadyButton(player);
	NetPlay.players[player].ready = bReady;
	NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);

	netPlayersUpdated = true;
	return true;
}

static BOOL changePosition(UBYTE player, UBYTE position)
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
			NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);
			netPlayersUpdated = true;
			return true;
		}
	}
	debug(LOG_ERROR, "Failed to swap positions for player %d, position %d", (int)player, (int)position);
	if (player < game.maxPlayers && position < game.maxPlayers)
	{
		debug(LOG_NET, "corrupted positions :player (%hhu) new position (%hhu) old position (%d)", player, position, NetPlay.players[player].position );
		// Positions were corrupted. Attempt to fix.
		NetPlay.players[player].position = position;
		NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

static BOOL changeColour(UBYTE player, UBYTE col)
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
			NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);
			netPlayersUpdated = true;
			return true;
		}
	}
	debug(LOG_ERROR, "Failed to swap colours for player %d, colour %d", (int)player, (int)col);
	if (player < game.maxPlayers && col < MAX_PLAYERS)
	{
		// Colours were corrupted. Attempt to fix.
		debug(LOG_NET, "corrupted colors :player (%hhu) new position (%hhu) old color (%d)", player, col, NetPlay.players[player].colour );
		setPlayerColour(player, col);
		NetPlay.players[player].colour = col;
		NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);
		netPlayersUpdated = true;
		return true;
	}
	return false;
}

static BOOL SendColourRequest(UBYTE player, UBYTE col)
{
	if(NetPlay.isHost)			// do or request the change
	{
		return changeColour(player, col);
	}
	else
	{
		// clients tell the host which color they want
		NETbeginEncode(NET_COLOURREQUEST, NET_HOST_ONLY);
			NETuint8_t(&player);
			NETuint8_t(&col);
		NETend();
	}
	return true;
}

static BOOL SendPositionRequest(UBYTE player, UBYTE position)
{
	if(NetPlay.isHost)			// do or request the change
	{
		return changePosition(player, position);
	}
	else
	{
		debug(LOG_NET, "Requesting the host to change our position. From %d to %d", player, position);
		// clients tell the host which position they want
		NETbeginEncode(NET_POSITIONREQUEST, NET_HOST_ONLY);
			NETuint8_t(&player);
			NETuint8_t(&position);
		NETend();
	}
	return true;
}

BOOL recvColourRequest()
{
	UBYTE	player, col;

	if(!NetPlay.isHost)				// only host should act
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(NET_COLOURREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&col);
	NETend();

	if (player > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_COLOURREQUEST from player %d: Tried to change player %d to colour %d",
		      NETgetSource(), (int)player, (int)col);
		return false;
	}

	if (whosResponsible(player) != NetMsg.source)
	{
		HandleBadParam("NET_COLOURREQUEST given incorrect params.", player, NetMsg.source);
		return false;
	}

	resetReadyStatus(false);

	return changeColour(player, col);
}

BOOL recvPositionRequest()
{
	UBYTE	player, position;

	if(!NetPlay.isHost)				// only host should act
	{
		ASSERT(false, "Host only routine detected for client!");
		return true;
	}

	NETbeginDecode(NET_POSITIONREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&position);
	NETend();
	debug(LOG_NET, "Host received position request from player %d to %d", player, position);

	if (player > MAX_PLAYERS || position > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_POSITIONREQUEST from player %d: Tried to change player %d to %d",
		      NETgetSource(), (int)player, (int)position);
		return false;
	}

	if (whosResponsible(player) != NetMsg.source)
	{
		HandleBadParam("NET_POSITIONREQUEST given incorrect params.", player, NetMsg.source);
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

	// delete colour chooser button
	closeColourChooser();

	// delete team chooser botton
	widgDelete(psWScreen,MULTIOP_TEAMS_START+player);

	// delete that players box
	widgDelete(psWScreen,MULTIOP_PLAYER_START+player);

	// delete 'ready' button
	widgDelete(psWScreen,MULTIOP_READY_FORM_ID+player);

	// add form.
	addBlueForm(MULTIOP_PLAYERS,MULTIOP_TEAMCHOOSER_FORM,"",
				7,
				((MULTIOP_TEAMSHEIGHT+5)*NetPlay.players[player].position)+4,
				MULTIOP_ROW_WIDTH,MULTIOP_TEAMSHEIGHT);

	// tally up the team counts
	for (i=0; i< game.maxPlayers ; i++)
	{
		inSlot[NetPlay.players[i].team]++;
	}

	// Make sure all players can't be on same team.
	if ( game.maxPlayers <= 2 )	// 2p game
	{
		disallow = player ? NetPlay.players[0].team : NetPlay.players[1].team;
	}
	else
		if ( game.maxPlayers > 2 && game.maxPlayers <= 8)	// 4 or 8p game
		{
			int maxslot = 0 , tmpslot =0 , range = 0;

			for(i=0; i < game.maxPlayers ; i++)
			{
				if( inSlot[i] >= tmpslot )
				{
					maxslot = i;
					tmpslot = inSlot[i];
				}
			}
			range = game.maxPlayers <= 4 ? 2 : 6 ;
			if ( inSlot[maxslot] <= range  || NetPlay.players[player].team == maxslot)
			{
				disallow = ANYENTRY;	// we can pick any slot
			}
			else
			{
				disallow = maxslot;		// can't pick this slot
			}
		}

	// add the teams, skipping the one we CAN'T be on (if applicable)
	for (i = 0; i < game.maxPlayers; i++)
	{
		if (i != disallow)
		{
			addMultiBut(psWScreen, MULTIOP_TEAMCHOOSER_FORM, MULTIOP_TEAMCHOOSER + i, i * (iV_GetImageWidth(FrontImages,
						IMAGE_TEAM0) + 3) + 3, 6, iV_GetImageWidth(FrontImages, IMAGE_TEAM0), iV_GetImageHeight(FrontImages,
						IMAGE_TEAM0), _("Team"), IMAGE_TEAM0 + i , IMAGE_TEAM0_HI + i, IMAGE_TEAM0_HI + i);
		}
		// may want to add some kind of 'can't do' icon instead of being blank?
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
				(UWORD)(( (MULTIOP_PLAYERHEIGHT+5)*NetPlay.players[player].position)+4),
				MULTIOP_READY_WIDTH,MULTIOP_READY_HEIGHT);

	// draw 'ready' button
	if (NetPlay.players[player].ready)
	{
		addMultiBut(psWScreen, MULTIOP_READY_FORM_ID+player,MULTIOP_READY_START+player,3,
			8,MULTIOP_READY_WIDTH,MULTIOP_READY_HEIGHT,
			_("Waiting for other players"),IMAGE_CHECK_ON,IMAGE_CHECK_ON,IMAGE_CHECK_ON_HI);
	}
	else
	{
		addMultiBut(psWScreen, MULTIOP_READY_FORM_ID+player,MULTIOP_READY_START+player,3, 
			8,MULTIOP_READY_WIDTH,MULTIOP_READY_HEIGHT,
			_("Click when ready"),IMAGE_CHECK_OFF,IMAGE_CHECK_OFF,IMAGE_CHECK_OFF_HI);
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

UDWORD addPlayerBox(BOOL players)
{
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	UDWORD			i=0;

	// if background isn't there, then return since were not ready to draw the box yet!
	if(widgGetFromID(psWScreen,FRONTEND_BACKDROP) == NULL)
	{
		return 0;
	}

	widgDelete(psWScreen,MULTIOP_PLAYERS);		// del player window
	widgDelete(psWScreen,FRONTEND_SIDETEXT2);	// del text too,

	memset(&sFormInit, 0, sizeof(W_FORMINIT));	// draw player window
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
		int numPlayers = 0, team = -1;
		bool allOnSameTeam = true;

		for (i=0;i<game.maxPlayers;i++)
		{
			if (game.skDiff[i] || isHumanPlayer(i))
			{
				numPlayers++;
				if (numPlayers > 2)
				{
					break; // We just need to know if we have enough to start a game
				}
			}
		}

		if (game.alliance != ALLIANCES_TEAMS)
		{
			allOnSameTeam = false;
		}
		else for (i=0; i<game.maxPlayers; i++)
		{
			if (game.skDiff[i] || isHumanPlayer(i))
			{
				if (team == -1)
				{
					team = NetPlay.players[i].team;
				}
				else if (NetPlay.players[i].team != team)
				{
					allOnSameTeam = false;
					break;
				}
			}
		}

		for(i=0;i<game.maxPlayers;i++)
		{
			if(ingame.localOptionsReceived)
			{
				//add team chooser
				memset(&sButInit, 0, sizeof(W_BUTINIT));
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_TEAMS_START+i;
				sButInit.style = WBUT_PLAIN;
				sButInit.x = 7;
				sButInit.y = (UWORD)(( (MULTIOP_TEAMSHEIGHT+5)*NetPlay.players[i].position)+4);
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
				sButInit.FontID = font_regular;
				sButInit.pDisplay = displayTeamChooser;
				sButInit.UserData = i;

				if (teamChooserUp == i && colourChooserUp < 0)
				{
					addTeamChooser(i);
				}
				else if(game.skDiff[i] && game.alliance == ALLIANCES_TEAMS)
				{
					// only if not disabled and in locked teams mode
					widgAddButton(psWScreen, &sButInit);
				}
			}

			if (ingame.localOptionsReceived && NetPlay.players[i].allocated)	// only draw if real player!
			{
				// add a 'ready' button
				if (numPlayers > 1 && !allOnSameTeam) // only if we have enough players to start
				{
					drawReadyButton(i);
				}

				// draw player info box
				memset(&sButInit, 0, sizeof(W_BUTINIT));
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_PLAYER_START+i;
				sButInit.style = WBUT_PLAIN;
				sButInit.x = 7 + MULTIOP_TEAMSWIDTH;
				sButInit.y = (UWORD)(( (MULTIOP_PLAYERHEIGHT+5)*NetPlay.players[i].position)+4);
				sButInit.width = MULTIOP_PLAYERWIDTH - MULTIOP_TEAMSWIDTH - MULTIOP_READY_WIDTH;
				sButInit.height = MULTIOP_PLAYERHEIGHT;
				if (selectedPlayer == i)
				{
					sButInit.pTip = _("Click to change player settings");
				}
				else
				{
					sButInit.pTip = NULL;
				}
				sButInit.FontID = font_regular;
				sButInit.pDisplay = displayPlayer;
				sButInit.UserData = i;

				if (teamChooserUp < 0 && i == colourChooserUp)
				{
					addColourChooser(i);
				}
				else if (i != teamChooserUp)	// Display player number/color only if not selecting team for this player
				{
					widgAddButton(psWScreen, &sButInit);
				}
			}
			else	// AI player
			{
				memset(&sFormInit, 0, sizeof(W_BUTINIT));
				sFormInit.formID = MULTIOP_PLAYERS;
				sFormInit.id = MULTIOP_PLAYER_START+i;
				sFormInit.style = WBUT_PLAIN;
				sFormInit.x = 7 + MULTIOP_TEAMSWIDTH;
				sFormInit.y = (UWORD)(( (MULTIOP_PLAYERHEIGHT+5)*NetPlay.players[i].position)+4);
				sFormInit.width = MULTIOP_PLAYERWIDTH - MULTIOP_TEAMSWIDTH + 1;
				sFormInit.height = MULTIOP_PLAYERHEIGHT;
				if (NetPlay.isHost && !challengeActive)
				{
					sFormInit.pTip = _("Click to adjust AI difficulty");
				}
				else
				{
					sFormInit.pTip = NULL;
				}
				sFormInit.pDisplay = displayPlayer;
				sFormInit.UserData = i;
				widgAddForm(psWScreen, &sFormInit);
				addFEAISlider(MULTIOP_SKSLIDE+i,sFormInit.id, 35,9, DIFF_SLIDER_STOPS,
					(game.skDiff[i] <= DIFF_SLIDER_STOPS ? game.skDiff[i] : DIFF_SLIDER_STOPS / 2));	//set to 50% (value of UBYTE_MAX == human player)
			}
		}
	}

	if (ingame.bHostSetup && !challengeActive) // if hosting.
	{
		sliderEnableDrag(true);
	}else{
		sliderEnableDrag(false);
	}

	return i;
}

/*
 * Notify all players of host launching the game
 */
static void SendFireUp(void)
{
	NETbeginEncode(NET_FIREUP, NET_ALL_PLAYERS);
		// no payload necessary
	NETend();
}

// host kicks a player from a game.
void kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type)
{
	// send a kick msg
	NETbeginEncode(NET_KICK, NET_ALL_PLAYERS);
		NETuint32_t(&player_id);
		NETstring( (char *) reason, MAX_KICK_REASON);
		NETenum(&type);
	NETend();

	debug(LOG_NET, "Kicking player %u (%s).",
		  (unsigned int)player_id, getPlayerName(player_id));

	NETplayerKicked(player_id);
}

static void addChatBox(void)
{
	W_FORMINIT		sFormInit;
	W_EDBINIT		sEdInit;

	if(widgGetFromID(psWScreen,FRONTEND_TOPFORM))
	{
		widgDelete(psWScreen,FRONTEND_TOPFORM);
	}

	if(widgGetFromID(psWScreen,MULTIOP_CHATBOX))
	{
		return;
	}

	memset(&sFormInit, 0, sizeof(W_FORMINIT));

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

	memset(&sEdInit, 0, sizeof(W_EDBINIT));							// add the edit box
	sEdInit.formID = MULTIOP_CHATBOX;
	sEdInit.id = MULTIOP_CHATEDIT;
	sEdInit.x = MULTIOP_CHATEDITX;
	sEdInit.y = MULTIOP_CHATEDITY;
	sEdInit.style = WEDB_PLAIN;
	sEdInit.width = MULTIOP_CHATEDITW;
	sEdInit.height = MULTIOP_CHATEDITH;
	sEdInit.FontID = font_regular;

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
			NETbeginEncode(NET_HOST_DROPPED, NET_ALL_PLAYERS);
			NETend();
			sendLeavingMsg();								// say goodbye
			NETclose();										// quit running game.
			bHosted = false;								// stop host mode.
			widgDelete(psWScreen,FRONTEND_BACKDROP);		// refresh options screen.
			startMultiOptions(true);
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
				selectedPlayer =0;
			}
			else
			{
				changeTitleMode(MULTI);
				selectedPlayer =0;
			}
			return;
		}
		debug(LOG_NET, "We have stopped joining.");
		changeTitleMode(lastTitleMode);
		selectedPlayer = 0;

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
//			sstrcpy(game.map,widgGetString(psWScreen, MULTIOP_MAP));
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
			NetPlay.maxPlayers = MIN(game.maxPlayers, 7);
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
			NetPlay.maxPlayers = game.maxPlayers;
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
			addGameOptions(false);

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions();
				disableMultiButs();
			}
			break;

		case MULTIOP_BASE:
			game.base = CAMP_BASE;
			addGameOptions(false);

			resetReadyStatus(false);

			if(bHosted)
			{
				disableMultiButs();
				sendOptions();
			}
			break;

		case MULTIOP_DEFENCE:
			game.base = CAMP_WALLS;
			addGameOptions(false);

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

		addGameOptions(false);									// update game options box.
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
	default:
		break;
	}

	if (id >= MULTIOP_TEAMS_START && id <= MULTIOP_TEAMS_END && !challengeActive)		// Clicked on a team chooser
	{
		int clickedMenuID = id - MULTIOP_TEAMS_START;

		//make sure team chooser is not up before adding new one for another player
		if (teamChooserUp < 0 && colourChooserUp < 0 && canChooseTeamFor(clickedMenuID))
		{
			addTeamChooser(clickedMenuID);
		}
	}

	//clicked on a team
	if((id >= MULTIOP_TEAMCHOOSER) && (id <= MULTIOP_TEAMCHOOSER_END))
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
	if((id >= MULTIOP_READY_START) && (id <= MULTIOP_READY_END))	// clicked on a player
	{
		UBYTE player = (UBYTE)(id-MULTIOP_READY_START);

		if (player == selectedPlayer)
		{
			SendReadyRequest(selectedPlayer, !NetPlay.players[player].ready);

			// if hosting try to start the game if everyone is ready
			if(NetPlay.isHost && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
				// reset flag in case people dropped/quit on join screen
				NET_PlayerConnectionStatus = 0;
			}
		}
	}
		

	if((id >= MULTIOP_PLAYER_START) && (id <= MULTIOP_PLAYER_END))	// clicked on a player
	{
		// options for kicking
		if(NetPlay.isHost && (id - MULTIOP_PLAYER_START != NET_HOST_ONLY) )	// can't kick self!
		{
			if(mouseDown(MOUSE_RMB)) // both buttons....
			{
				int victim = id - MULTIOP_PLAYER_START;		// who to kick out
				int j = 0;
				char *msg;
				
				while (j != victim && j < MAX_PLAYERS)
				{
					j++; // find out ID of player
				}
				sasprintf(&msg, _("The host has kicked %s from the game!"), getPlayerName(j));
				sendTextMessage(msg, true);
				kickPlayer(victim, "you are unwanted by the host.", ERROR_KICKED);
				resetReadyStatus(true);		//reset and send notification to all clients
			}
		}
		if (id - MULTIOP_PLAYER_START == selectedPlayer || (NetPlay.isHost && isHumanPlayer(id - MULTIOP_PLAYER_START)))
		{
			if (teamChooserUp < 0)		// not choosing team already
			{
				addColourChooser(id-MULTIOP_PLAYER_START);
			}
		}
	}

	if((id >= MULTIOP_SKSLIDE) && (id <=MULTIOP_SKSLIDE_END) && !challengeActive) // choseskirmish difficulty.
	{
		UDWORD newValue, oldValue;

		newValue = widgGetSliderPos(psWScreen,id);
		oldValue = (UDWORD)game.skDiff[id-MULTIOP_SKSLIDE];

		game.skDiff[id-MULTIOP_SKSLIDE] = (UBYTE)newValue;

		//Show/hide team chooser if player was enabled/disabled
		if((oldValue == 0 && newValue > 0) || (oldValue > 0 && newValue == 0) )
		{
			closeTeamChooser();
			addPlayerBox(  !ingame.bHostSetup || bHosted);	//restore initial options screen
		}

		resetReadyStatus(false);

		sendOptions();
	}

	// don't kill last player
	if((id >= MULTIOP_COLCHOOSER) && (id <= MULTIOP_COLCHOOSER_END)) // chose a new colour.
	{
		resetReadyStatus(false);		// will reset only locally if not a host

		SendColourRequest(colourChooserUp, id - MULTIOP_COLCHOOSER);
		closeColourChooser();
		addPlayerBox(  !ingame.bHostSetup || bHosted);
	}

	if (id == MULTIOP_COLCHOOSER_KICK)
	{
		char *msg;

		sasprintf(&msg, _("The host has kicked %s from the game!"), getPlayerName(colourChooserUp));
		sendTextMessage(msg, true);
		kickPlayer(colourChooserUp, "you are unwanted by the host.", ERROR_KICKED);
		resetReadyStatus(true);		//reset and send notification to all clients
		closeColourChooser();
	}

	// request a player number.
	if((id >= MULTIOP_PLAYCHOOSER) && (id <= MULTIOP_PLAYCHOOSER_END) && allowChangePosition) // chose a new starting position
	{
		resetReadyStatus(false);		// will reset only locally if not a host

		SendPositionRequest(colourChooserUp, id - MULTIOP_PLAYCHOOSER);
		closeColourChooser();
		addPlayerBox(  !ingame.bHostSetup || bHosted);
	}
}

/* Start a multiplayer or skirmish game */
void startMultiplayerGame(void)
{
	decideWRF();										// set up swrf & game.map
	bMultiPlayer = true;
	bMultiMessages = true;
	NET_PlayerConnectionStatus = 0; // reset disconnect conditions

	if (NetPlay.isHost)
	{
		// This sets the limits to whatever the defaults are for the limiter screen
		// If host sets limits, then we do not need to do the following routine.
		if (!bLimiterLoaded)
		{
			if (!resLoad("wrf/limiter_tex.wrf", 501))
			{
				debug(LOG_INFO, "Unable to load limiter_tex.  Defaults not set.");
			}
			debug(LOG_NET, "limiter was NOT activated, setting defaults");
			if (!resLoad("wrf/piestats.wrf", 502))
			{
				debug(LOG_INFO, "Unable to load limits.  Defaults not set.");
			}
			else if (!resLoad("wrf/limiter_data.wrf", 503))
			{
				debug(LOG_INFO, "Unable to load limits?");
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
		{
			char buf[255];
			NETlogEntry("FireUp called.", SYNC_FLAG, 0);
			snprintf(buf, sizeof(buf), "type:%u, scavs:%u, fog:%u, alliance:%u, MaxPlayers:%u", (unsigned int)game.type, (unsigned int)game.scavengers,
				(unsigned int)game.fog, (unsigned int)game.alliance, (unsigned int)game.maxPlayers);
			NETlogEntry(buf, SYNC_FLAG, 0);
			snprintf(buf, sizeof(buf), "base:%u, power:%u, map:%s, name:%s", (unsigned int)game.base, game.power, game.map, game.name );
			NETlogEntry(buf, SYNC_FLAG, 0);
		}
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
	uint8_t type;

	while(NETrecv(&type))
	{
		// Copy the message to the global one used by the new NET API
		switch(type)
		{
		case NET_FILE_REQUESTED:
			recvMapFileRequested();
			break;

		case NET_FILE_PAYLOAD:
			widgSetButtonState(psWScreen, MULTIOP_MAP_BUT, 1);	// turn preview button off
			if (recvMapFileData())
			{
				widgSetButtonState(psWScreen, MULTIOP_MAP_BUT, 0);	// turn it back on again
			}
			break;

		case NET_FILE_CANCELLED:					// host only routine
			{
				uint32_t reason;
				uint32_t victim;

				if(!NetPlay.isHost)				// only host should act
				{
					ASSERT(false, "Host only routine detected for client!");
					break;
				}

				NETbeginDecode(NET_FILE_CANCELLED);
					NETuint32_t(&victim);
					NETuint32_t(&reason);
				NETend();

				if (whosResponsible(victim) != NetMsg.source)
				{
					HandleBadParam("NET_FILE_CANCELLED given incorrect params.", victim, NetMsg.source);
					return;
				}

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
			recvOptions();
			ingame.localOptionsReceived = true;

			if(titleMode == MULTIOPTION)
			{
				addGameOptions(true);
				disableMultiButs();
				addChatBox();
			}
			break;

		case NET_ALLIANCE:
			recvAlliance(false);
			break;

		case NET_COLOURREQUEST:
			recvColourRequest();
			break;

		case NET_POSITIONREQUEST:
			recvPositionRequest();
			break;

		case NET_TEAMREQUEST:
			recvTeamRequest();
			break;

		case NET_READY_REQUEST:
			if (NetPlay.isHost)
			{
				recvReadyRequest();
				if (multiplayPlayersReady(false))
				{
					// if hosting try to start the game if everyone is ready
					startMultiplayerGame();
				}
			}
			break;

		case NET_PING:						// diagnostic ping msg.
			recvPing();
			break;

		case NET_PLAYER_DROPPED:		// remote player got disconnected
		{
			uint32_t player_id = MAX_PLAYERS;

			resetReadyStatus(false);

			NETbeginDecode(NET_PLAYER_DROPPED);
			{
				NETuint32_t(&player_id);
			}
			NETend();

			if (player_id >= MAX_PLAYERS)
			{
				debug(LOG_INFO, "** player %u has dropped - huh?", player_id);
				break;
			}

			if (whosResponsible(player_id) != NetMsg.source && NetMsg.source != NET_HOST_ONLY)
			{
				HandleBadParam("NET_PLAYER_DROPPED given incorrect params.", player_id, NetMsg.source);
				break;
			}

			debug(LOG_INFO,"** player %u has dropped!", player_id);

			MultiPlayerLeave(player_id);		// get rid of their stuff
			NET_InitPlayer(player_id, false);           // sets index player's array to false
			NET_PlayerConnectionStatus = 2;		//DROPPED_CONNECTION
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

			NETbeginDecode(NET_PLAYERRESPONDING);
				// the player that has just responded
				NETuint32_t(&player_id);
			NETend();

			ingame.JoiningInProgress[player_id] = false;
			ingame.DataIntegrity[player_id] = false;
			break;
		}
		case NET_FIREUP:					// campaign game started.. can fire the whole shebang up...
			if (NET_HOST_ONLY != NetMsg.source)
			{
				HandleBadParam("NET_FIREUP given incorrect params.", 255, NetMsg.source);
				break;
			}
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
				break;
			}

		case NET_KICK:						// player is forcing someone to leave
		{
			uint32_t player_id;
			char reason[MAX_KICK_REASON];
			LOBBY_ERROR_TYPES KICK_TYPE = ERROR_NOERROR;

			NETbeginDecode(NET_KICK);
				NETuint32_t(&player_id);
				NETstring( reason, MAX_KICK_REASON);
				NETenum(&KICK_TYPE);
			NETend();

			if (NetMsg.source != NET_HOST_ONLY)
			{
				char buf[250]= {'\0'};

				ssprintf(buf, "Player %d (%s : %s) tried to kick %u", (int) NetMsg.source, NetPlay.players[NetMsg.source].name, NetPlay.players[NetMsg.source].IPtextAddress, player_id);
				NETlogEntry(buf, SYNC_FLAG, 0);
				debug(LOG_ERROR, "%s", buf);
				if (NetPlay.isHost)
				{
					kickPlayer(NetMsg.source, reason, KICK_TYPE);
				} 
				break;
			}
			if (!NetPlay.isHost)
			{
				if (selectedPlayer == player_id)	// we've been told to leave.
				{
					setLobbyError(KICK_TYPE);
					stopJoining();
					//screen_RestartBackDrop();
					//changeTitleMode(TITLE);
					// maybe we want a custom 'kick' backdrop instead?
					pie_LoadBackDrop(SCREEN_RANDOMBDROP);
					debug(LOG_ERROR, "You have been kicked, because %s ", reason );
				}
				else
				{
					NETplayerKicked(player_id);
				}
			}
			else
			{
				debug(LOG_ERROR, "Player %d (%s : %s) tried to kick %u ?", (int) NetMsg.source, NetPlay.players[NetMsg.source].name, NetPlay.players[NetMsg.source].IPtextAddress, player_id);
			}
			break;
		}
		case NET_HOST_DROPPED:
			NETbeginDecode(NET_HOST_DROPPED);
			NETend();
			stopJoining();
			debug(LOG_NET, "The host has quit!");
			setLobbyError(ERROR_HOSTDROPPED);
			break;

		case NET_TEXTMSG:					// Chat message
			if(ingame.localOptionsReceived)
			{
				recvTextMessage();
			}
			break;
		}
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
		ingame.localOptionsReceived = true;		// when netPlayersUpdated is true, we need to redraw everything
		lastrefresh= gameTime;
		if (!multiRequestUp && (bHosted || ingame.localJoiningInProgress))
		{
			// store the slider settings if they are up,
			for(id=0;id<MAX_PLAYERS;id++)
			{
				if(widgGetFromID(psWScreen,MULTIOP_SKSLIDE+id))
				{
					value = widgGetSliderPos(psWScreen,MULTIOP_SKSLIDE+id);
					if(value != game.skDiff[id])
					{
						game.skDiff[id] = value;

						if (NetPlay.isHost)
						{
							sendOptions();
						}
					}
				}
			}


			addPlayerBox(true);				// update the player box.
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
			editBoxClicked((W_EDITBOX*)widgGetFromID(psWScreen,MULTIOP_CHATEDIT), &context);
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
				addGameOptions(false);
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
		changeTitleMode(lastTitleMode);
	}
}

BOOL startMultiOptions(BOOL bReenter)
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
		allowChangePosition = true;
		for(i=0;i<MAX_PLAYERS;i++)
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
			int		i;
			dictionary	*dict = iniparser_load(sRequestResult);

			resetReadyStatus(false);
			removeWildcards((char*)sPlayer);

			if (!hostCampaign((char*)game.name,(char*)sPlayer))
			{
				debug(LOG_ERROR, "Failed to host the challenge.");
				return false;
			}
			bHosted = true;

			sstrcpy(game.map, iniparser_getstring(dict, "challenge:Map", game.map));
			game.maxPlayers = iniparser_getint(dict, "challenge:MaxPlayers", game.maxPlayers);	// TODO, read from map itself, not here!!
			game.scavengers = iniparser_getboolean(dict, "challenge:Scavengers", game.scavengers);
			game.alliance = ALLIANCES_TEAMS;
			netPlayersUpdated = true;
			mapDownloadProgress = 100;
			game.power = iniparser_getint(dict, "challenge:Power", game.power);
			game.base = iniparser_getint(dict, "challenge:Bases", game.base + 1) - 1;		// count from 1 like the humans do
			allowChangePosition = iniparser_getboolean(dict, "challenge:AllowPositionChange", false);

			for (i = 0; i < MAX_PLAYERS; i++)
			{
				char key[64];

				ssprintf(key, "player_%d:team", i + 1);
				NetPlay.players[i].team = iniparser_getint(dict, key, NetPlay.players[i].team + 1) - 1;
				ssprintf(key, "player_%d:position", i + 1);
				if (iniparser_find_entry(dict, key))
				{
					changePosition(i, iniparser_getint(dict, key, NetPlay.players[i].position));
				}
				if (i != 0)
				{
					ssprintf(key, "player_%d:difficulty", i + 1);
					game.skDiff[i] = iniparser_getint(dict, key, game.skDiff[i]);
				}
			}

			iniparser_freedict(dict);

			ingame.localOptionsReceived = true;
			addGameOptions(false);									// update game options box.
			addChatBox();
			disableMultiButs();
			addPlayerBox(true);
		}
	}

	addPlayerBox(false);								// Players
	addGameOptions(false);

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
	BOOL Hilight = false;
	BOOL Down = false;
	UDWORD	i = psWidget->UserData;
	char	tmp[8], gamename[StringSize];
	unsigned int ping;

	if (LobbyError != ERROR_NOERROR)
	{
		return;
	}

	// collate info
	if( ((W_BUTTON*)psWidget)->state & (WBUTS_HILITE))
	{
		Hilight = true;
	}
	if( ((W_BUTTON*)psWidget)->state & (WBUT_LOCK |WBUT_CLICKLOCK)) //LOCK WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK))
	{
		Down = true;
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
	else if (NETgameIsCorrectVersion(&NetPlay.games[i]))
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


// ////////////////////////////////////////////////////////////////////////////
static UDWORD bestPlayer(UDWORD player)
{

	UDWORD i, myscore,  score, count=1;

	myscore =  getMultiStats(player).totalScore;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(isHumanPlayer(i) && i!=player )
		{
			score = getMultiStats(i).totalScore;

			if(score >= myscore)
			{
				count++;
			}
		}
	}

	return count;
}

void displayTeamChooser(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	BOOL		Hilight = false;
	UDWORD		i = psWidget->UserData;

	if( ((W_BUTTON*)psWidget)->state & (WBUTS_HILITE| WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK))
	{
		Hilight = true;
	}

	ASSERT(i < MAX_PLAYERS && NetPlay.players[i].team >= 0 && NetPlay.players[i].team < MAX_PLAYERS, "Team index out of bounds" );

	//bluboxes.
	drawBlueBox(x,y,psWidget->width,psWidget->height);							// right

	iV_DrawImage(FrontImages, IMAGE_TEAM0 + NetPlay.players[i].team, x + 2, y + 8);
}

// ////////////////////////////////////////////////////////////////////////////
void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	BOOL		Hilight = false;
	UDWORD		j = psWidget->UserData, eval;
	PLAYERSTATS stat;

	if( ((W_BUTTON*)psWidget)->state & (WBUTS_HILITE| WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK))
	{
		Hilight = true;
	}

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
		//bluboxes.
		drawBlueBox(x,y,psWidget->width,psWidget->height);							// right
		drawBlueBox(x,y,60,psWidget->height);
		drawBlueBox(x,y,31,psWidget->height);										// left.

		iV_SetFont(font_regular);											// font
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);

		// name
		while (iV_GetTextWidth(NetPlay.players[j].name) > psWidget->width - 68)		// clip name.
		{
			NetPlay.players[j].name[strlen(NetPlay.players[j].name) - 1] = '\0';
		}
		if (j == NET_HOST_ONLY && NetPlay.bComms)
		{
			iV_DrawText(NetPlay.players[j].name, x + 65, y + 18);
			iV_SetFont(font_small);
			iV_SetTextColour(WZCOL_TEXT_MEDIUM);
			iV_DrawText(_("HOST"), x + 65, y + 28);
			iV_SetFont(font_regular);
			iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		}
		else if (NetPlay.bComms && NetPlay.isHost)
		{
			char buf[250] = {'\0'};

			// show "actual" ping time
			iV_DrawText(NetPlay.players[j].name, x + 65, y + 18);
			iV_SetFont(font_small);
			iV_SetTextColour(WZCOL_TEXT_MEDIUM);
			ssprintf(buf, "Ping: %03d", ingame.PingTimes[j]);
			iV_DrawText(buf, x + 65, y + 28);
			iV_SetFont(font_regular);
			iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		}
		else
		{
			iV_DrawText(NetPlay.players[j].name, x + 65, y + 22);
		}

		// ping rating
		if (ingame.PingTimes[j] < PING_MED)
		{
			iV_DrawImage(FrontImages,IMAGE_LAMP_GREEN,x,y);
		}else
		if(ingame.PingTimes[j] >= PING_MED && ingame.PingTimes[j] < PING_HI)
		{
			iV_DrawImage(FrontImages,IMAGE_LAMP_AMBER,x,y);
		}else
		{
			iV_DrawImage(FrontImages,IMAGE_LAMP_RED,x,y);
		}

		// ranking against other players.
		eval = bestPlayer(j);
		switch (eval)
		{
		case 1:
			iV_DrawImage(IntImages,IMAGE_GN_1,x+5,y+3);
			break;
		case 2:
			iV_DrawImage(IntImages,IMAGE_GN_2,x+4,y+3);
			break;
		case 3:
			iV_DrawImage(IntImages,IMAGE_GN_3,x+4,y+3);
			break;
		case 4:
			iV_DrawImage(IntImages,IMAGE_GN_4,x+4,y+3);
			break;
		case 5:
			iV_DrawImage(IntImages,IMAGE_GN_5,x+4,y+3);
			break;
		case 6:
			iV_DrawImage(IntImages,IMAGE_GN_6,x+4,y+3);
			break;
		case 7:
			iV_DrawImage(IntImages,IMAGE_GN_7,x+4,y+3);
			break;
		default:
			break;
		}

		if(getMultiStats(j).played < 5)
		{
			iV_DrawImage(FrontImages,IMAGE_MEDAL_DUMMY,x+37,y+13);
		}
		else
		{
			stat = getMultiStats(j);

			// star 1 total droid kills
			eval = stat.totalKills;
			if(eval >600)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK1,x+37,y+3);
			}
			else if(eval >300)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK2,x+37,y+3);
			}
			else if(eval >150)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK3,x+37,y+3);
			}


			// star 2 games played
			eval = stat.played;
			if(eval >200)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK1,x+37,y+13);
			}
			else if(eval >100)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK2,x+37,y+13);
			}
			else if(eval >50)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK3,x+37,y+13);
			}


			// star 3 games won.
			eval = stat.wins;
			if(eval >80)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK1,x+37,y+23);
			}
			else if(eval >40)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK2,x+37,y+23);
			}
			else if(eval >10)
			{
				iV_DrawImage(FrontImages,IMAGE_MULTIRANK3,x+37,y+23);
			}

			// medals.
			if ((stat.wins >= 6) && (stat.wins > (2 * stat.losses))) // bronze requirement.
			{
				if ((stat.wins >= 12) && (stat.wins > (4 * stat.losses))) // silver requirement.
				{
					if ((stat.wins >= 24) && (stat.wins > (8 * stat.losses))) // gold requirement
					{
						iV_DrawImage(FrontImages,IMAGE_MEDAL_GOLD,x+49,y+11);
					}
					else
					{
						iV_DrawImage(FrontImages,IMAGE_MEDAL_SILVER,x+49,y+11);
					}
				}
				else
				{
					iV_DrawImage(FrontImages,IMAGE_MEDAL_BRONZE,x+49,y+11);
				}
			}
		}
		game.skDiff[j] = UBYTE_MAX;	// set AI difficulty to 0xFF (i.e. not an AI)
	}
	else
	{
		// AI
		drawBlueBox(x,y,31,psWidget->height);	// left.
	}
	// Draw for both AI and human players

	if (!NetPlay.players[j].wzFile.isSending)
	{
		// player number
		switch (NetPlay.players[j].position)
		{
		case 0:
			iV_DrawImage(IntImages,IMAGE_GN_0,x+4,y+29);
			break;
		case 1:
			iV_DrawImage(IntImages,IMAGE_GN_1,x+5,y+29);
			break;
		case 2:
			iV_DrawImage(IntImages,IMAGE_GN_2,x+4,y+29);
			break;
		case 3:
			iV_DrawImage(IntImages,IMAGE_GN_3,x+4,y+29);
			break;
		case 4:
			iV_DrawImage(IntImages,IMAGE_GN_4,x+4,y+29);
			break;
		case 5:
			iV_DrawImage(IntImages,IMAGE_GN_5,x+4,y+29);
			break;
		case 6:
			iV_DrawImage(IntImages,IMAGE_GN_6,x+4,y+29);
			break;
		case 7:
			iV_DrawImage(IntImages,IMAGE_GN_7,x+4,y+29);
			break;
		default:
			break;
		}

		if (game.skDiff[j]) // not disabled
		{
			switch (getPlayerColour(j))		// flag icon
			{
			case 0:
				iV_DrawImage(FrontImages,IMAGE_PLAYER0,x+7,y+9);
				break;
			case 1:
				iV_DrawImage(FrontImages,IMAGE_PLAYER1,x+7,y+9);
				break;
			case 2:
				iV_DrawImage(FrontImages,IMAGE_PLAYER2,x+7,y+9);
				break;
			case 3:
				iV_DrawImage(FrontImages,IMAGE_PLAYER3,x+7,y+9);
				break;
			case 4:
				iV_DrawImage(FrontImages,IMAGE_PLAYER4,x+7,y+9);
				break;
			case 5:
				iV_DrawImage(FrontImages,IMAGE_PLAYER5,x+7,y+9);
				break;
			case 6:
				iV_DrawImage(FrontImages,IMAGE_PLAYER6,x+7,y+9);
				break;
			case 7:
				iV_DrawImage(FrontImages,IMAGE_PLAYER7,x+7,y+9);
				break;
			default:
				break;
			}
		}
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
	BOOL	Hilight = false;
	UDWORD	Down = 0;
	UDWORD	Grey = 0;
	UWORD	im_norm = UNPACKDWORD_TRI_A((UDWORD)psWidget->UserData);
	UWORD	im_down = UNPACKDWORD_TRI_B((UDWORD)psWidget->UserData);
	UWORD	im_hili = UNPACKDWORD_TRI_C((UDWORD)psWidget->UserData);
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
			// This line spams a TON.  (Game options screen, hover mouse over one of the flag colors)
//			debug(LOG_WARNING, "no automatic multibut highlight for width = %d", iV_GetImageWidth(FrontImages, im_norm));
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


	// now display
	iV_DrawImage(FrontImages, im_norm, x, y);

	// hilights etc..
	if(Hilight && !Grey)
	{
		if (Down)
		{
			iV_DrawImage(FrontImages, im_down, x, y);
		}

		if (hiToUse)
		{
			iV_DrawImage(FrontImages, hiToUse, x, y);
		}

	}
	else if (Down)
	{
		iV_DrawImage(FrontImages, im_down, x, y);
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

static BOOL addMultiEditBox(UDWORD formid, UDWORD id, UDWORD x, UDWORD y, const char* tip, char tipres[128], UDWORD icon, UDWORD iconhi, UDWORD iconid)
{
	W_EDBINIT		sEdInit;

	memset(&sEdInit, 0, sizeof(W_EDBINIT));			// editbox
	sEdInit.formID = formid;
	sEdInit.id = id;
	sEdInit.style = WEDB_PLAIN;
	sEdInit.x = (short)x;
	sEdInit.y = (short)y;
	sEdInit.width = MULTIOP_EDITBOXW;
	sEdInit.height = MULTIOP_EDITBOXH;
	sEdInit.pText = tipres;
	sEdInit.FontID = font_regular;
	sEdInit.pBoxDisplay = displayMultiEditBox;
	if (!widgAddEditBox(psWScreen, &sEdInit))
	{
		return false;
	}

	addMultiBut(psWScreen, MULTIOP_OPTIONS, iconid, x + MULTIOP_EDITBOXW + 2, y + 2, MULTIOP_EDITBOXH, MULTIOP_EDITBOXH, tip, icon, iconhi, iconhi);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

BOOL addMultiBut(W_SCREEN *screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char* tipres, UDWORD norm, UDWORD down, UDWORD hi)
{
	W_BUTINIT		sButInit;

	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = formid;
	sButInit.id = id;
	sButInit.style = WFORM_PLAIN;
	sButInit.x = (short) x;
	sButInit.y = (short) y;
	sButInit.width = (unsigned short) width;
	sButInit.height= (unsigned short) height;
	sButInit.pTip = tipres;
	sButInit.FontID = font_regular;
	sButInit.pDisplay = displayMultiBut;
	sButInit.UserData = PACKDWORD_TRI(norm, down, hi);

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
