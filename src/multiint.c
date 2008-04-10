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
 * MultiInt.c
 *
 * Alex Lee, 98. Pumpkin Studios, Bath.
 * Functions to display and handle the multiplayer interface screens,
 * along with connection and game options.
 */

#include "lib/framework/frame.h"

#include <time.h>
#include <SDL_opengl.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/frameint.h"
#include "lib/framework/file.h"

/* Includes direct access to render library */
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/pieclip.h"
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

#include "multiplay.h"
#include "multiint.h"
#include "multijoin.h"
#include "multistat.h"
#include "multirecv.h"
#include "multimenu.h"

#include "warzoneconfig.h"

#include "init.h"
#include "levels.h"

#define MAP_PREVIEW_DISPLAY_TIME 2500	// number of milliseconds to show map in preview
// ////////////////////////////////////////////////////////////////////////////
// vars
extern char	MultiCustomMapsPath[PATH_MAX];
extern char	MultiPlayersPath[PATH_MAX];

extern BOOL				bSendingMap;

extern void intDisplayTemplateButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

BOOL						bHosted			= false;				//we have set up a game
char						sPlayer[128];							// player name (to be used)
char						buildTime[8]	 = "67HGDV3"; //RODZ was __TIME__ ;
static BOOL					bColourChooserUp= false;
static BOOL					bTeamChooserUp[MAX_PLAYERS]= {false,false,false,false,false,false,false,false};
SDWORD						playerTeamGUI[MAX_PLAYERS] = {0,1,2,3,4,5,6,7};		//team each player belongs to (in skirmish setup screen)
SDWORD						playerTeam[MAX_PLAYERS] = {-1,-1,-1,-1,-1,-1,-1,-1};		//team each player belongs to (in the game)
static BOOL				SettingsUp		= false;
static UBYTE				InitialProto	= 0;
static W_SCREEN				*psConScreen;
static SDWORD				dwSelectedGame	=0;						//player[] and games[] indexes
static UDWORD				gameNumber;								// index to games icons
static BOOL					safeSearch		= false;				// allow auto game finding.

static UDWORD hideTime=0;

#define DEFAULTCAMPAIGNMAP	"Rush"
#define DEFAULTSKIRMISHMAP	"Sk-Rush"


/// end of globals.
// ////////////////////////////////////////////////////////////////////////////
// Function protos

// widget functions
static BOOL addMultiEditBox(UDWORD formid,UDWORD id,UDWORD x, UDWORD y, const char* tip, char tipres[128],UDWORD icon,UDWORD iconid);
static void addBlueForm					(UDWORD parent,UDWORD id, const char *txt,UDWORD x,UDWORD y,UDWORD w,UDWORD h);

// Drawing Functions
void		displayChatEdit				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayMultiBut				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		intDisplayFeBox				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayRemoteGame			(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayPlayer				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayTeamChooser			(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayMultiReady			(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		displayMultiEditBox			(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		setLockedTeamsMode			(void);

// find games
static void addGames				(void);
void		runGameFind				(void);
void		startGameFind			(void);

// Connection option functions
static BOOL	OptionsInet				(UDWORD);
static void addConnections			(UDWORD);
void		runConnectionScreen		(void);
BOOL		startConnectionScreen	(void);

// Game option functions
static	void	addGameOptions		(BOOL bRedo);				// options (rhs) boxV
UDWORD	addPlayerBox		(BOOL);				// players (mid) box
static	void	addChatBox			(void);
static	void	disableMultiButs	(void);
static	void	processMultiopWidgets(UDWORD);
static	void	SendFireUp			(void);
void	kickPlayer			(UDWORD dpid);
void			runMultiOptions		(void);
BOOL			startMultiOptions	(BOOL);
void			frontendMultiMessages(void);

static	UDWORD	bestPlayer			(UDWORD);
static	void	decideWRF			(void);

static void		closeColourChooser	(void);
static void		closeTeamChooser	(void);
static BOOL		SendColourRequest	(UBYTE player, UBYTE col,UBYTE chosenPlayer);
static BOOL		safeToUseColour		(UDWORD player,UDWORD col);
BOOL			chooseColour		(UDWORD);
static BOOL		changeReadyStatus	(UBYTE player, BOOL bReady);
void			resetReadyStatus	(bool bSendOptions);

// ////////////////////////////////////////////////////////////////////////////
// map previews..

/// This function is a HACK
/// Loads the entire map (including calculating gateways) just to show
/// a picture of it
void loadMapPreview(void)
{
	char			aFileName[256];
	UDWORD			fileSize;
	char			*pFileData = NULL;
	LEVEL_DATASET	*psLevel;

	UDWORD			i, j, x, y, height, offX2, offY2;
	UBYTE			scale,col;
	MAPTILE			*psTile,*WTile;
	UDWORD oursize;
	char  *ptr, *imageData;

	if(psMapTiles)
	{
		mapShutdown();
	}

	// load the terrain types
	levFindDataSet(game.map, &psLevel);
	rebuildSearchPath(psLevel->dataDir, false);
	strcpy(aFileName,psLevel->apDataFiles[0]);
	aFileName[strlen(aFileName)-4] = '\0';
	strcat(aFileName, "/ttypes.ttp");
	pFileData = fileLoadBuffer;
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
	pFileData = fileLoadBuffer;
	if (!loadFileToBuffer(aFileName, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_ERROR, "loadMapPreview: Failed to load map file");
		return;
	}
	if (!mapLoad(pFileData, fileSize))
	{
		debug(LOG_ERROR, "loadMapPreview: Failed to load map");
		return;
	}
	gwShutDown();

	scale =1;
	if((mapHeight <  240)&&(mapWidth < 320))
	{
		scale = 2;
	}
	if((mapHeight <  120)&&(mapWidth < 160))
	{
		scale = 3;
	}
	if((mapHeight <  60)&&(mapWidth < 80))
	{
		scale = 4;
	}
	if((mapHeight <  30)&&(mapWidth < 40))
	{
		scale = 5;
	}
	oursize = sizeof(char) * BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT;
	imageData = (char*)malloc(oursize * 3);
	ptr = imageData;
	memset(ptr, 0x45, sizeof(char) * BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT * 3); //dunno about background color
	psTile = psMapTiles;
	offX2 = (BACKDROP_HACK_WIDTH / 2) - ((scale * mapWidth) / 2);
	offY2 = (BACKDROP_HACK_HEIGHT / 2 )  - ((scale * mapHeight) / 2);

	for (i = 0; i < mapHeight; i++)
	{
		WTile = psTile;
		for (j = 0; j < mapWidth; j++)
		{
			height = WTile->height;
			col = height;

			for (x = (j * scale); x < (j * scale) + scale; x++)
			{
				for (y = (i * scale); y < (i * scale) + scale; y++)
				{
					imageData[3 * ((offY2 + y) * BACKDROP_HACK_WIDTH + (x + offX2))] = col;
					imageData[3 * ((offY2 + y) * BACKDROP_HACK_WIDTH + (x + offX2)) + 1] = col;
					imageData[3 * ((offY2 + y) * BACKDROP_HACK_WIDTH + (x + offX2)) + 2] = col;
				}
			}
			WTile += 1;
		}
		psTile += mapWidth;
	}
	plotStructurePreview16(imageData, scale, offX2, offY2);
	screen_Upload(imageData);
	free(imageData);

	hideTime = gameTime;
	mapShutdown();
}

// ////////////////////////////////////////////////////////////////////////////
// helper func

//sets sWRFILE form game.map
static void decideWRF(void)
{
	// try and load it from the maps directory first,
	strlcpy(aLevelName, MultiCustomMapsPath, sizeof(aLevelName));
	strlcat(aLevelName, game.map, sizeof(aLevelName));
	strlcat(aLevelName, ".wrf", sizeof(aLevelName));
	debug(LOG_WZ, "decideWRF: %s", aLevelName);
	//if the file exists in the downloaded maps dir then use that one instead.
	// FIXME: Try to incorporate this into physfs setup somehow for sane paths
	if ( !PHYSFS_exists(aLevelName) )
	{
		strlcpy(aLevelName, game.map, sizeof(aLevelName));		// doesn't exist, must be a predefined one.
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Connection Options Screen.

static BOOL OptionsInet(UDWORD parentID)			//internet options
{
	W_EDBINIT		sEdInit;
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;

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
		_("Return To Previous Screen"),IMAGE_RETURN,IMAGE_RETURN_HI,true);	// goback buttpn levels

	addConnections(0);

	return true;
}

// add connections
static void addConnections(UDWORD begin)
{
	addTextButton(CON_TYPESID_START+begin+0,FRONTEND_POS2X,FRONTEND_POS2Y, _("Lobby"),false,false);
	addTextButton(CON_TYPESID_START+begin+1,FRONTEND_POS3X,FRONTEND_POS3Y, _("IP"),   false,false);
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
			break;
		case CON_TYPESID_MORE:
			widgDelete(psWScreen,FRONTEND_BOTFORM);

			SettingsUp = false;
			InitialProto +=5;

			addBottomForm();
			addMultiBut(psWScreen,FRONTEND_BOTFORM,CON_CANCEL,10,10,MULTIOP_OKW,MULTIOP_OKH,
			_("Return To Previous Screen"),IMAGE_RETURN,IMAGE_RETURN_HI,true);	// goback buttpn levels

			addConnections(InitialProto);
			break;
		case CON_TYPESID_START+0: // Lobby button
			NETsetupTCPIP(""); //inet
			changeTitleMode(GAMEFIND);
			break;
		case CON_TYPESID_START+1: // IP button
			OptionsInet(id);
			break;
		case CON_IP: // ip entered
			strcpy(addr, widgGetString(psConScreen, CON_IP));
			break;
		case CON_OK:
			if(SettingsUp == true)
			{
				widgReleaseScreen(psConScreen);
				SettingsUp = false;
			}

			NETsetupTCPIP(addr); //inet

			changeTitleMode(GAMEFIND);
			break;
	}

	widgDisplayScreen(psWScreen);							// show the widgets currently running
	if(SettingsUp == true)
	{
		widgDisplayScreen(psConScreen);						// show the widgets currently running
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Game Chooser Screen.

static void addGames(void)
{
	UDWORD i,gcount=0;
	W_BUTINIT	sButInit;

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

	for(i=0;i<MaxGames;i++)							// draw games
	{
		widgDelete(psWScreen, GAMES_GAMESTART+i);	// remove old icon.
		if( NetPlay.games[i].desc.dwSize !=0)
		{

			sButInit.id = GAMES_GAMESTART+i;

			if(gcount < 6)							// only center column needed.
			{
				sButInit.x = 125;
				sButInit.y = (UWORD)(30+((5+GAMES_GAMEHEIGHT)*i) );
			}
			else
			{
				if(i<6)		//column 1
				{
					sButInit.x = 10;
					sButInit.y = (UWORD)(30+((5+GAMES_GAMEHEIGHT)*i) );
				}
				else		//column 2
				{
					sButInit.x = 20+GAMES_GAMEWIDTH;
					sButInit.y = (UWORD)(30+((5+GAMES_GAMEHEIGHT)*(i-6) ) );
				}
			}

			sButInit.pTip = NetPlay.games[i].name;

			sButInit.UserData = i;
			widgAddButton(psWScreen, &sButInit);
		}
	}

}

void runGameFind(void )
{
	UDWORD id;
	static UDWORD lastupdate=0;

	if(lastupdate> gameTime)lastupdate = 0;
	if(gameTime-lastupdate >6000)
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
		NETfindGame();								// find games synchronously
		addGames();										//redraw list.
	}


	if(id >= GAMES_GAMESTART && id<=GAMES_GAMEEND)
	{
		gameNumber = id-GAMES_GAMESTART;

		if( ( NetPlay.games[gameNumber].desc.dwCurrentPlayers < NetPlay.games[gameNumber].desc.dwMaxPlayers)
			&& !(NetPlay.games[gameNumber].desc.dwFlags & SESSION_JOINDISABLED) ) // if still joinable
		{
			// TODO: Check whether this code is used at all in skirmish games, if not, remove it.
			// if skirmish, check it wont take the last slot
			if (bMultiPlayer
			 && !NetPlay.bComms
			 && NETgetGameFlagsUnjoined(gameNumber,1) == SKIRMISH
			 && (NetPlay.games[gameNumber].desc.dwCurrentPlayers >= NetPlay.games[gameNumber].desc.dwMaxPlayers - 1))
			{
				goto FAIL;
			}


			ingame.localOptionsReceived = false;			// note we are awaiting options
			strcpy(game.name, NetPlay.games[gameNumber].name);		// store name

			joinCampaign(gameNumber,(char*)sPlayer);

			changeTitleMode(MULTIOPTION);
		}

	}

FAIL:

	widgDisplayScreen(psWScreen);								// show the widgets currently running
	if(safeSearch)
	{
		iV_SetFont(font_large);
		iV_DrawText(_("Searching"), D_W+260, D_H+460);
	}
}


void startGameFind(void)
{
	addBackdrop();										//background
	addTopForm();										// logo
	addBottomForm();

	addSideText(FRONTEND_SIDETEXT,  FRONTEND_SIDEX, FRONTEND_SIDEY,_("GAMES"));

	// cancel
	addMultiBut(psWScreen,FRONTEND_BOTFORM,CON_CANCEL,10,5,MULTIOP_OKW,MULTIOP_OKH,_("Return To Previous Screen"),
		IMAGE_RETURN,IMAGE_RETURN_HI,true);

	if(!safeSearch)
	{
		//refresh
		addMultiBut(psWScreen,FRONTEND_BOTFORM,MULTIOP_REFRESH ,480-MULTIOP_OKW-5 ,5,MULTIOP_OKW,MULTIOP_OKH,
					_("Refresh Games List"),IMAGE_REFRESH,IMAGE_REFRESH,false);			// Find Games button
	}

	NETfindGame();
	addGames();	// now add games.
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



static void addGameOptions(BOOL bRedo)
{
	W_FORMINIT		sFormInit;

	if(bRedo)
	{
		// stop any editing going on
		screenClearFocus(psWScreen);
	}

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

	addMultiEditBox(MULTIOP_OPTIONS,MULTIOP_GNAME,MCOL0,MROW2,_("Select Game Name"), game.name ,IMAGE_EDIT_GAME,MULTIOP_GNAME_ICON);
	addMultiEditBox(MULTIOP_OPTIONS,MULTIOP_MAP  ,MCOL0,MROW3,_("Select Map"), game.map ,IMAGE_EDIT_MAP,MULTIOP_MAP_ICON);

	// buttons.

	// game type
	addBlueForm(MULTIOP_OPTIONS,MULTIOP_GAMETYPE,_("Game"),MCOL0,MROW4,MULTIOP_BLUEFORMW,27);
	addMultiBut(psWScreen,MULTIOP_GAMETYPE,MULTIOP_CAMPAIGN,MCOL1, 2 , MULTIOP_BUTW,MULTIOP_BUTH,
				_("Mayhem"), IMAGE_ARENA, IMAGE_ARENA_HI, true);	//camp
	addMultiBut(psWScreen,MULTIOP_GAMETYPE,MULTIOP_SKIRMISH,MCOL2, 2 , MULTIOP_BUTW,MULTIOP_BUTH,
				_("Skirmish"),IMAGE_SKIRMISH,IMAGE_SKIRMISH_HI,true);	//skirmish

	widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN,	0);
	widgSetButtonState(psWScreen, MULTIOP_SKIRMISH,	0);

	switch(game.type)
	{
	case CAMPAIGN:
		widgSetButtonState(psWScreen,MULTIOP_CAMPAIGN,WBUT_LOCK);
		break;
	case SKIRMISH:
		widgSetButtonState(psWScreen,MULTIOP_SKIRMISH,WBUT_LOCK);
		break;
	}

	if(!NetPlay.bComms)
	{
		widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_DISABLE);
	}

	//just display the game options.
	addMultiEditBox(MULTIOP_OPTIONS,MULTIOP_PNAME,MCOL0,MROW1, _("Select Player Name"),(char*) sPlayer,IMAGE_EDIT_PLAYER,MULTIOP_PNAME_ICON);

	// Fog type
	addBlueForm(MULTIOP_OPTIONS,MULTIOP_FOG,_("Fog"),MCOL0,MROW5,MULTIOP_BLUEFORMW,27);

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

	if (game.type != CAMPAIGN)
	{
		// alliances
		addBlueForm(MULTIOP_OPTIONS, MULTIOP_ALLIANCES, _("Alliances"), MCOL0, MROW6, MULTIOP_BLUEFORMW, 27);

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
	}

	if(game.type == CAMPAIGN || game.type == SKIRMISH)
	{
		// pow levels
		if (game.type == CAMPAIGN)
		{
			addBlueForm(MULTIOP_OPTIONS,MULTIOP_POWER,_("Power"),MCOL0,MROW6,MULTIOP_BLUEFORMW,27);
		}
		else
		{
			addBlueForm(MULTIOP_OPTIONS,MULTIOP_POWER,_("Power"),MCOL0,MROW7,MULTIOP_BLUEFORMW,27);
		}
		addMultiBut(psWScreen,MULTIOP_POWER,MULTIOP_POWLEV_LOW,MCOL1,2,MULTIOP_BUTW,MULTIOP_BUTH,
			_("Low Power Levels"),IMAGE_POWLO,IMAGE_POWLO_HI,true);
		addMultiBut(psWScreen,MULTIOP_POWER,MULTIOP_POWLEV_MED,MCOL2,2,MULTIOP_BUTW,MULTIOP_BUTH,
			_("Medium Power Levels"),IMAGE_POWMED,IMAGE_POWMED_HI,true);
		addMultiBut(psWScreen,MULTIOP_POWER,MULTIOP_POWLEV_HI, MCOL3, 2,MULTIOP_BUTW,MULTIOP_BUTH,
			_("High Power Levels"),IMAGE_POWHI,IMAGE_POWHI_HI,true);
		widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,0);		//hilight correct entry
		widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,0);
		widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI ,0);
		switch(game.power)
		{
		case LEV_LOW:
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,WBUT_LOCK);
			break;
		case LEV_MED:
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,WBUT_LOCK);
			break;
		case LEV_HI:
			widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI,WBUT_LOCK);
			break;
		}

		//type clean/base/defence
		if (game.type == CAMPAIGN)
		{
			addBlueForm(MULTIOP_OPTIONS, MULTIOP_BASETYPE, _("Base"), MCOL0, MROW7, MULTIOP_BLUEFORMW, 27);
		}
		else
		{
			addBlueForm(MULTIOP_OPTIONS, MULTIOP_BASETYPE, _("Base"), MCOL0, MROW8, MULTIOP_BLUEFORMW, 27);
		}
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
			break;
		case 1:
			widgSetButtonState(psWScreen, MULTIOP_BASE,WBUT_LOCK);
			break;
		case 2:
			widgSetButtonState(psWScreen, MULTIOP_DEFENCE,WBUT_LOCK);
			break;
		}
	}
	addBlueForm(MULTIOP_OPTIONS, MULTIOP_MAP_PREVIEW, _("Map Preview"), MCOL0, MROW9, MULTIOP_BLUEFORMW, 27);
						//hilight correct entry
	addMultiBut(psWScreen,MULTIOP_MAP_PREVIEW,MULTIOP_MAP_BUT,MCOL4-8,2,MULTIOP_BUTW,MULTIOP_BUTH,
				_("Click to see Map"), IMAGE_FOG_OFF,IMAGE_FOG_OFF_HI,true);
			widgSetButtonState(psWScreen, MULTIOP_MAP_BUT,0); //1 = OFF  0=ON 

	// cancel
	addMultiBut(psWScreen,MULTIOP_OPTIONS,CON_CANCEL,
		MULTIOP_CANCELX,MULTIOP_CANCELY,
		iV_GetImageWidth(FrontImages,IMAGE_RETURN),
		iV_GetImageHeight(FrontImages,IMAGE_RETURN),
		_("Return To Previous Screen"),IMAGE_RETURN,IMAGE_RETURN_HI,true);

	// host Games button
	if(ingame.bHostSetup && !bHosted)
	{
		addMultiBut(psWScreen,MULTIOP_OPTIONS,MULTIOP_HOST,MULTIOP_HOSTX,MULTIOP_HOSTY,35,28,
					_("Start Hosting Game"),IMAGE_HOST,IMAGE_HOST,false);
	}

	// hosted or hosting.
	// limits button.
	if(ingame.bHostSetup )
	{
		addMultiBut(psWScreen,MULTIOP_OPTIONS,MULTIOP_STRUCTLIMITS,MULTIOP_STRUCTLIMITSX,MULTIOP_STRUCTLIMITSY,
					35,28, _("Set Structure Limits"),IMAGE_SLIM,IMAGE_SLIM_HI,false);
	}

	return;
}

// ////////////////////////////////////////////////////////////////////////////
// Colour functions

static BOOL safeToUseColour(UDWORD player,UDWORD col)
{
	UDWORD i;

	// if already using it.
	if( col == getPlayerColour(player) )
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

BOOL chooseColour(UDWORD player)
{
	UDWORD col;

	for(col =0;col<MAX_PLAYERS;col++)
	{
		if(safeToUseColour(player,col))
		{
			setPlayerColour(player,col);
			return true;
		}
	}
	return true;
}

/*
 * Checks if a team chooser dialog is up
 * returns index of the player who was choosing a team or -1 if none open
 */
static SDWORD teamChooserUp(void)
{
	UDWORD i;
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(bTeamChooserUp[i])
			return i;
	}

	return -1;		//none
}
static void addColourChooser(UDWORD player)
{
	UDWORD i;//,j;

	// delete that players box,
	widgDelete(psWScreen,MULTIOP_PLAYER_START+player);

	// detele team chooser botton
	widgDelete(psWScreen,MULTIOP_TEAMS_START+player);

	// detele 'ready' button
	widgDelete(psWScreen,MULTIOP_READY_START+player);

	// add form.
	addBlueForm(MULTIOP_PLAYERS,MULTIOP_COLCHOOSER_FORM,"",
				10,
				((MULTIOP_PLAYERHEIGHT+5)*player)+4,
				MULTIOP_ROW_WIDTH,MULTIOP_PLAYERHEIGHT);

	// add the flags
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		addMultiBut(psWScreen,MULTIOP_COLCHOOSER_FORM, MULTIOP_COLCHOOSER+i,
			(i*(iV_GetImageWidth(FrontImages,IMAGE_PLAYER0) +5)+7) ,//x
			4,/*9,*/													  //y
			iV_GetImageWidth(FrontImages,IMAGE_PLAYER0),		  //w
			iV_GetImageHeight(FrontImages,IMAGE_PLAYER0),		  //h
			"", IMAGE_PLAYER0+i, IMAGE_PLAYER0+i,false);

			if( !safeToUseColour(selectedPlayer,i))
			{
				widgSetButtonState(psWScreen,MULTIOP_COLCHOOSER+i ,WBUT_DISABLE);
			}
	}

	//add the position chooser.
	for(i=0;i<game.maxPlayers;i++)
	{

		addMultiBut(psWScreen,MULTIOP_COLCHOOSER_FORM, MULTIOP_PLAYCHOOSER+i,
			(i*(iV_GetImageWidth(FrontImages,IMAGE_PLAYER0) +5)+7),//x
			23,													  //y
			iV_GetImageWidth(FrontImages,IMAGE_WEE_GUY)+7,		  //w
			iV_GetImageHeight(FrontImages,IMAGE_WEE_GUY),		  //h
			"",IMAGE_WEE_GUY, IMAGE_WEE_GUY,10+i);

			if(isHumanPlayer(i) && i!=selectedPlayer)
			{
				widgSetButtonState(psWScreen,MULTIOP_PLAYCHOOSER+i ,WBUT_DISABLE);
			}
	}



	bColourChooserUp = true;
}

static void closeColourChooser(void)
{
	bColourChooserUp = false;

	widgDelete(psWScreen,MULTIOP_COLCHOOSER_FORM);
}

static void changeTeam(UBYTE player, UBYTE team)
{
	playerTeamGUI[player] = team;
	debug(LOG_WZ, "set %d as new team for player %d", team, player);
	sendOptions(player2dpid[player], player);	// tell everyone && update requesting player.
}

static BOOL SendTeamRequest(UBYTE player, UBYTE chosenTeam)
{
	if(NetPlay.bHost)			// do or request the change.
	{
		changeTeam(player, chosenTeam);	// do the change, remember only the host can do this to avoid confusion.
	}
	else
	{
		NETbeginEncode(NET_TEAMREQUEST, NET_ALL_PLAYERS);

		NETuint8_t(&player);
		NETuint8_t(&chosenTeam);

		NETend();

	}
	return true;
}

BOOL recvTeamRequest()
{
	UBYTE	player, team;

	if(!NetPlay.bHost)							//only host should act.
	{
		return true;
	}

	NETbeginDecode(NET_TEAMREQUEST);
	NETuint8_t(&player);
	NETuint8_t(&team);
	NETend();

	if (player > MAX_PLAYERS || team > MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_TEAMREQUEST from player %d: Tried to change player %d (team %d)",
		      NETgetSource(), (int)player, (int)team);
		return false;
	}

	resetReadyStatus(false);

	changeTeam(player, team);

	return true;
}

static BOOL SendReadyRequest(UBYTE player, BOOL bReady)
{
	if(NetPlay.bHost)			// do or request the change.
	{
		return changeReadyStatus(player, bReady);
	}
	else
	{
		NETbeginEncode(NET_READY_REQUEST, NET_ALL_PLAYERS);
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

	if(!NetPlay.bHost)							//only host should act.
	{
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

	return changeReadyStatus((UBYTE)player, bReady);
}

static BOOL changeReadyStatus(UBYTE player, BOOL bReady)
{
	unsigned int playerGUI_ID;

	for(playerGUI_ID=0;(playerGUI_ID <= game.maxPlayers) &&
		(player2dpid[player] != NetPlay.players[playerGUI_ID].dpid);playerGUI_ID++);

	bPlayerReadyGUI[playerGUI_ID] = bReady;
	sendOptions(player2dpid[player], player);	// tell everyone && update requesting player.

	return true;
}

static BOOL changeColour(UBYTE player, UBYTE col, UBYTE chosenPlayer)
{
	if(chosenPlayer == UBYTE_MAX)
	{	// colour change.
		if(!safeToUseColour(player,col))
		{
			return false;
		}

		setPlayerColour(player, col);
		sendOptions(player2dpid[player], player);	// tell everyone && update requesting player.
	}
	else // base change.
	{
		UBYTE oldcol;
		UDWORD dpid;

		if(isHumanPlayer(chosenPlayer))
		{
			return false;
		}

		dpid = player2dpid[player];
		player2dpid[player] = 0;					// remove player,
		ingame.JoiningInProgress[player] = false;	// if they never joined, reset the flag

		oldcol = getPlayerColour(chosenPlayer);
		setPlayerColour(chosenPlayer, getPlayerColour(player));// retain player colour
		setPlayerColour(player, oldcol);			// reset old colour
//		chooseColour(chosenPlayer);				// pick an unused colour.
		setupNewPlayer(dpid, chosenPlayer);		// setup all the guff for that player.
		sendOptions(dpid, chosenPlayer);

		NETplayerInfo();				// bring netplay up to date with changes.

		if(player == selectedPlayer)			// if host changing
		{
			selectedPlayer = chosenPlayer;
		}
	}
	return true;
}

static BOOL SendColourRequest(UBYTE player, UBYTE col,UBYTE chosenPlayer)
{
	if(NetPlay.bHost)			// do or request the change.
	{
		return changeColour(player, col, chosenPlayer);
	}
	else
	{
		NETbeginEncode(NET_COLOURREQUEST, NET_ALL_PLAYERS);
			NETuint8_t(&player);
			NETuint8_t(&col);
			NETuint8_t(&chosenPlayer);
		NETend();
	}
	return true;
}

BOOL recvColourRequest()
{
	UBYTE	player, col, chosenPlayer;

	if(!NetPlay.bHost)							//only host should act.
	{
		return true;
	}

	NETbeginDecode(NET_COLOURREQUEST);
		NETuint8_t(&player);
		NETuint8_t(&col);
		NETuint8_t(&chosenPlayer);
	NETend();

	if (player > MAX_PLAYERS || (chosenPlayer > MAX_PLAYERS && chosenPlayer != UBYTE_MAX))
	{
		debug(LOG_ERROR, "Invalid NET_COLOURREQUEST from player %d: Tried to change player %d to %d",
		      NETgetSource(), (int)player, (int)chosenPlayer);
		return false;
	}

	resetReadyStatus(false);

	return changeColour(player, col, chosenPlayer);
}


/*
 * Opens a menu for a player to choose a team
 * 'player' is a player id of the player who will get a new team assigned
 */
static void addTeamChooser(UDWORD player)
{
	UDWORD i;

	debug(LOG_WZ, "opened team chooser for %d, current team: %d", player, playerTeamGUI[player]);

	// delete team chooser botton
	widgDelete(psWScreen,MULTIOP_TEAMS_START+player);

	// delete that players box
	widgDelete(psWScreen,MULTIOP_PLAYER_START+player);

	// delete 'ready' button
	widgDelete(psWScreen,MULTIOP_READY_START+player);

	// add form.
	addBlueForm(MULTIOP_PLAYERS,MULTIOP_TEAMCHOOSER_FORM,"",
				10,
				((MULTIOP_TEAMSHEIGHT+5)*player)+4,
				MULTIOP_ROW_WIDTH,MULTIOP_TEAMSHEIGHT);

	// add the teams
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		W_BUTINIT		sButInit;

		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = MULTIOP_TEAMCHOOSER_FORM;
		sButInit.id = MULTIOP_TEAMCHOOSER+i;
		sButInit.style = WFORM_PLAIN;
		sButInit.x = (short) (i*(iV_GetImageWidth(FrontImages,IMAGE_TEAM0) + 3)+3);
		sButInit.y = (short) 6;
		sButInit.width = (unsigned short) iV_GetImageWidth(FrontImages,IMAGE_TEAM0);
		sButInit.height= (unsigned short) iV_GetImageHeight(FrontImages,IMAGE_TEAM0);

		sButInit.pTip = "Team";

		sButInit.FontID = font_regular;
		sButInit.pDisplay = displayMultiBut;
		sButInit.UserData = PACKDWORD_TRI(false,IMAGE_TEAM0+i , IMAGE_TEAM0+i);
		sButInit.pText = "Team0";

		if (!widgAddButton(psWScreen, &sButInit))
		{
			ASSERT(false,"addTeamChooser: widgAddButton() failed");
		}
	}

	bTeamChooserUp[player] = true;
}

/*
 * Closes Team Chooser dialog box, if there was any open
 */
static void closeTeamChooser(void)
{
	UDWORD i;
	for(i=0; i<MAX_PLAYERS;i++)
	{
		if(bTeamChooserUp[i])
		{
			debug(LOG_WZ, "closed team chooser for %d, current team: %d", i, playerTeamGUI[i]);

			bTeamChooserUp[i] = false;

			ASSERT(teamChooserUp() < 0, "closeTeamChooser: more than one team chooser open (%d)", teamChooserUp());
		}
	}

	widgDelete(psWScreen,MULTIOP_TEAMCHOOSER_FORM);	//only once!
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

	if(bHosted || ingame.localJoiningInProgress)
	{
		NETplayerInfo();
	}
	else
	{
		NETplayerInfo();			// get player info.
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
		for(i=0;i<game.maxPlayers;i++)
		{
			if(ingame.localOptionsReceived && game.type == SKIRMISH)	// skirmish only
			{
				//add team chooser
				memset(&sButInit, 0, sizeof(W_BUTINIT));
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_TEAMS_START+i;
				sButInit.style = WBUT_PLAIN;
				sButInit.x = 10;
				sButInit.y = (UWORD)(( (MULTIOP_TEAMSHEIGHT+5)*i)+4);
				sButInit.width = MULTIOP_TEAMSWIDTH;
				sButInit.height = MULTIOP_TEAMSHEIGHT;
				sButInit.pTip = "Choose team";	//Players[i].name;
				sButInit.FontID = font_regular;
				sButInit.pDisplay = displayTeamChooser;//intDisplayButtonHilight;
				sButInit.UserData = i;

				if(bTeamChooserUp[i] && !bColourChooserUp )
				{
					addTeamChooser(i);
				}
				else if(game.skDiff[i])	//only if not disabled
				{
					widgAddButton(psWScreen, &sButInit);
				}
			}

			if(ingame.localOptionsReceived && NetPlay.players[i].dpid)					// only draw if real player!
			{
				memset(&sButInit, 0, sizeof(W_BUTINIT));
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_PLAYER_START+i;
				sButInit.style = WBUT_PLAIN;
				sButInit.x = 10 + MULTIOP_TEAMSWIDTH;
				sButInit.y = (UWORD)(( (MULTIOP_PLAYERHEIGHT+5)*i)+4);
				sButInit.width = MULTIOP_PLAYERWIDTH - MULTIOP_TEAMSWIDTH - MULTIOP_READY_WIDTH;
				sButInit.height = MULTIOP_PLAYERHEIGHT;
				sButInit.pTip = NULL;//Players[i].name;
				sButInit.FontID = font_regular;
				sButInit.pDisplay = displayPlayer;//intDisplayButtonHilight;
				sButInit.UserData = i;

				if(bColourChooserUp && (teamChooserUp() < 0)
					&& NetPlay.players[i].dpid == player2dpid[selectedPlayer])
				{
					addColourChooser(i);
				}
				else if(i != teamChooserUp())	//display player number/color only if not selecting team for this player
				{
					widgAddButton(psWScreen, &sButInit);
				}

				// add 'ready' button
				memset(&sButInit, 0, sizeof(W_BUTINIT));
				sButInit.formID = MULTIOP_PLAYERS;
				sButInit.id = MULTIOP_READY_START+i;
				sButInit.style = WBUT_PLAIN;
				sButInit.x = 10 + MULTIOP_PLAYERWIDTH - MULTIOP_READY_WIDTH;
				sButInit.y = (UWORD)(( (MULTIOP_PLAYERHEIGHT+5)*i)+4);
				sButInit.width = MULTIOP_READY_WIDTH;
				sButInit.height = MULTIOP_READY_HEIGHT;
				sButInit.FontID = font_regular;
				sButInit.pDisplay = displayMultiReady;
				sButInit.UserData = i;

				if(bPlayerReadyGUI[i])
				{
					sButInit.pTip = "Ready";
				}
				else
				{
					sButInit.pTip = "Click when ready";
				}

				widgAddButton(psWScreen, &sButInit);
			}
			else if(game.type == SKIRMISH)	// skirmish player
			{
				memset(&sFormInit, 0, sizeof(W_BUTINIT));
				sFormInit.formID = MULTIOP_PLAYERS;
				sFormInit.id = MULTIOP_PLAYER_START+i;
				sFormInit.style = WBUT_PLAIN;
				sFormInit.x = 10;
				sFormInit.y = (UWORD)(( (MULTIOP_PLAYERHEIGHT+5)*i)+4);
				sFormInit.width = MULTIOP_ROW_WIDTH;
				sFormInit.height = MULTIOP_PLAYERHEIGHT;
				sFormInit.pTip = NULL;//Players[i].name;
				sFormInit.pDisplay = displayPlayer;//intDisplayButtonHilight;
				sFormInit.UserData = i;
				widgAddForm(psWScreen, &sFormInit);
				addFESlider(MULTIOP_SKSLIDE+i,sFormInit.id, 43,9, DIFF_SLIDER_STOPS,
					(game.skDiff[i] <= DIFF_SLIDER_STOPS ? game.skDiff[i] : DIFF_SLIDER_STOPS / 2), 0);	//set to 50% (value of UBYTE_MAX == human player)
			}
		}
	}

	if(ingame.bHostSetup) // if hosting.
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

// host kick a player from a game.
void kickPlayer(uint32_t player_id)
{
	// send a kick msg
	NETbeginEncode(NET_KICK, NET_ALL_PLAYERS);
		NETuint32_t(&player_id);
	NETend();
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
	setConsoleSizePos(MULTIOP_CHATBOXX+4+D_W, MULTIOP_CHATBOXY+10+D_H, MULTIOP_CHATBOXW-4);
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

	return;
}

// ////////////////////////////////////////////////////////////////////////////
static void disableMultiButs(void)
{

	// edit box icons.
	widgSetButtonState(psWScreen,MULTIOP_GNAME_ICON,WBUT_DISABLE);
	widgSetButtonState(psWScreen,MULTIOP_MAP_ICON ,WBUT_DISABLE);

	// edit boxes
	widgSetButtonState(psWScreen,MULTIOP_GNAME,WEDBS_DISABLE);
	widgSetButtonState(psWScreen,MULTIOP_MAP,WEDBS_DISABLE);

	if(game.type != CAMPAIGN)	widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_DISABLE);
	if(game.type != SKIRMISH)	widgSetButtonState(psWScreen, MULTIOP_SKIRMISH, WBUT_DISABLE);

	if(! NetPlay.bHost)
	{
		if( game.fog) widgSetButtonState(psWScreen,MULTIOP_FOG_OFF ,WBUT_DISABLE);		//fog
		if(!game.fog) widgSetButtonState(psWScreen,MULTIOP_FOG_ON ,WBUT_DISABLE);

		if(	game.type == CAMPAIGN || game.type == SKIRMISH)
		{
			if(game.base != CAMP_CLEAN)	widgSetButtonState(psWScreen,MULTIOP_CLEAN ,WBUT_DISABLE);	// camapign subtype.
			if(game.base != CAMP_BASE)	widgSetButtonState(psWScreen,MULTIOP_BASE ,WBUT_DISABLE);
			if(game.base != CAMP_WALLS)	widgSetButtonState(psWScreen,MULTIOP_DEFENCE,WBUT_DISABLE);

			if(game.power != LEV_LOW)	widgSetButtonState(psWScreen, MULTIOP_POWLEV_LOW,WBUT_DISABLE);		// pow levels
			if(game.power != LEV_MED)	widgSetButtonState(psWScreen, MULTIOP_POWLEV_MED,WBUT_DISABLE);
			if(game.power != LEV_HI )	widgSetButtonState(psWScreen, MULTIOP_POWLEV_HI,WBUT_DISABLE);
		}

		if (game.type == SKIRMISH)
		{
			if(game.alliance != NO_ALLIANCES)	widgSetButtonState(psWScreen,MULTIOP_ALLIANCE_N ,WBUT_DISABLE);	//alliance settings.
			if(game.alliance != ALLIANCES)	widgSetButtonState(psWScreen,MULTIOP_ALLIANCE_Y ,WBUT_DISABLE);
			if(game.alliance != ALLIANCES_TEAMS)	widgSetButtonState(psWScreen,MULTIOP_ALLIANCE_TEAMS ,WBUT_DISABLE);
		}
	}
}


////////////////////////////////////////////////////////////////////////////
static void stopJoining(void)
{
	dwSelectedGame	 = 0;
	saveConfig();

	{
		if(bHosted)											// cancel a hosted game.
		{
			sendLeavingMsg();								// say goodbye
			NETclose();										// quit running game.
			bHosted = false;								// stop host mode.
			widgDelete(psWScreen,FRONTEND_BACKDROP);		// refresh options screen.
//			startMultiOptions(false);
			startMultiOptions(true);
			ingame.localJoiningInProgress = false;
			return;
		}
		else if(ingame.localJoiningInProgress)				// cancel a joined game.
		{
			sendLeavingMsg();								// say goodbye
			NETclose();										// quit running game.

			ingame.localJoiningInProgress = false;			// reset local flags
			ingame.localOptionsReceived = false;
			if(!ingame.bHostSetup && NetPlay.bHost)			// joining and host was transfered.
			{
				NetPlay.bHost = false;
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

		changeTitleMode(MULTI);
		selectedPlayer = 0;

		if (ingame.bHostSetup)
		{
				pie_LoadBackDrop(SCREEN_RANDOMBDROP);
		}
	}
}

////////////////////////////////////////////////////////////////////////////
static void chooseSkirmishColours(void)
{
	UDWORD col,i,k;
	BOOL taken;

	col=0;
	for(i=0;i<MAX_PLAYERS;i++)	// assign each pc player a colour.
	{
		if(!isHumanPlayer(i))	// pick a colour for this player.
		{
			taken = true;		// go to next unused colour.
			while(taken)
			{
				taken = false;
				for(k=0;k<MAX_PLAYERS;k++)
				{
					if(isHumanPlayer(k) && getPlayerColour(k) == col)
					{
						taken = true;// already taken.
					}
				}
				if(taken)
				{
					col++;
				}
			}
			setPlayerColour(i,col);
			col++;
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
	UDWORD i;
	char	tmp[255];

	// host, who is setting up the game
	if((ingame.bHostSetup && !bHosted))
	{
		switch(id)												// Options buttons
		{

		case MULTIOP_GNAME:										// we get this when nec.
			strcpy(game.name,widgGetString(psWScreen, MULTIOP_GNAME));
			break;

		case MULTIOP_MAP:
			widgSetString(psWScreen, MULTIOP_MAP,game.map);
//			strcpy(game.map,widgGetString(psWScreen, MULTIOP_MAP));
			break;

		case MULTIOP_GNAME_ICON:
			break;

		case MULTIOP_MAP_ICON:
			widgDelete(psWScreen,MULTIOP_PLAYERS);
			widgDelete(psWScreen,FRONTEND_SIDETEXT2);					// del text too,

			debug(LOG_WZ, "processMultiopWidgets[MULTIOP_MAP_ICON]: %s.wrf", MultiCustomMapsPath);
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 1, 2);
			break;

		case MULTIOP_CAMPAIGN:									// turn on campaign game
			widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN, WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_SKIRMISH,0);
			game.type = CAMPAIGN;
			widgSetString(psWScreen, MULTIOP_MAP, DEFAULTCAMPAIGNMAP);
			strcpy(game.map,widgGetString(psWScreen, MULTIOP_MAP));
			game.alliance = NO_ALLIANCES;
			addGameOptions(false);
			break;

		case MULTIOP_SKIRMISH:
			widgSetButtonState(psWScreen, MULTIOP_CAMPAIGN,0 );
			widgSetButtonState(psWScreen, MULTIOP_SKIRMISH,WBUT_LOCK);
			game.type = SKIRMISH;
			widgSetString(psWScreen, MULTIOP_MAP, DEFAULTSKIRMISHMAP);
			strcpy(game.map,widgGetString(psWScreen, MULTIOP_MAP));
			addGameOptions(false);
			break;
		case MULTIOP_MAP_BUT:
			loadMapPreview();
			break;
		}
	}

	// host who is setting up or has hosted
	if(ingame.bHostSetup)// || NetPlay.bHost) // FIXME Was: if(ingame.bHostSetup);{} ??? Note the ; !
	{
		switch(id)
		{
		case MULTIOP_FOG_ON:
			widgSetButtonState(psWScreen, MULTIOP_FOG_ON,WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_FOG_OFF,0);
			game.fog = true;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions(0,0);
			}
			break;

		case MULTIOP_FOG_OFF:
			widgSetButtonState(psWScreen, MULTIOP_FOG_ON,0);
			widgSetButtonState(psWScreen, MULTIOP_FOG_OFF,WBUT_LOCK);
			game.fog = false;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions(0,0);
			}
			break;

		case MULTIOP_CLEAN:
			game.base = CAMP_CLEAN;
			addGameOptions(false);

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions(0,0);
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
				sendOptions(0,0);
			}
			break;

		case MULTIOP_DEFENCE:
			game.base = CAMP_WALLS;
			addGameOptions(false);

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions(0,0);
				disableMultiButs();
			}
			break;

		case MULTIOP_ALLIANCE_N:
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N,WBUT_LOCK);
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y,0);

			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_TEAMS,0);

			game.alliance = NO_ALLIANCES;	//0;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions(0,0);
			}
			break;

		case MULTIOP_ALLIANCE_Y:
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_N,0);
			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_TEAMS,0);

			widgSetButtonState(psWScreen, MULTIOP_ALLIANCE_Y,WBUT_LOCK);

			game.alliance = ALLIANCES;	//1;

			resetReadyStatus(false);

			if(bHosted)
			{
				sendOptions(0,0);
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
				sendOptions(0,0);
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
				sendOptions(0,0);
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
				sendOptions(0,0);
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
		strcpy(sPlayer,widgGetString(psWScreen, MULTIOP_PNAME));

		// chop to 15 chars..
		while(strlen(sPlayer) > 15)	// clip name.
		{
			sPlayer[strlen(sPlayer)-1]='\0';
		}

		// update string.
		widgSetString(psWScreen, MULTIOP_PNAME,sPlayer);


		removeWildcards((char*)sPlayer);

		sprintf(tmp,"-> %s",sPlayer);
		sendTextMessage(tmp,true);

		NETchangePlayerName(NetPlay.dpidPlayer, (char*)sPlayer);			// update if joined.
		loadMultiStats((char*)sPlayer,&playerStats);
		setMultiStats(NetPlay.dpidPlayer,playerStats,false);
		setMultiStats(NetPlay.dpidPlayer,playerStats,true);
		break;

	case MULTIOP_PNAME_ICON:
		widgDelete(psWScreen,MULTIOP_PLAYERS);
		widgDelete(psWScreen,FRONTEND_SIDETEXT2);					// del text too,

		addMultiRequest(MultiPlayersPath, ".sta", MULTIOP_PNAME, 0, 0);
		break;

	case MULTIOP_HOST:
		strcpy((char*)game.name,widgGetString(psWScreen, MULTIOP_GNAME));	// game name
		strcpy((char*)sPlayer,widgGetString(psWScreen, MULTIOP_PNAME));	// pname
		strcpy((char*)game.map,widgGetString(psWScreen, MULTIOP_MAP));		// add the name

		resetReadyStatus(false);

		removeWildcards((char*)sPlayer);

		hostCampaign((char*)game.name,(char*)sPlayer);
		bHosted = true;

		widgDelete(psWScreen,MULTIOP_REFRESH);
		widgDelete(psWScreen,MULTIOP_HOST);

		ingame.localOptionsReceived = true;

		addGameOptions(false);									// update game options box.
		addChatBox();

		disableMultiButs();

		// Ensure that Skirmish games have at least one AI player
		if (game.type == SKIRMISH)
		{
			// Search for the last AI player we can find and make sure that it is enabled
			for (i = game.maxPlayers - 1; i != ~0; --i)
			{
				if (!isHumanPlayer(i))
				{
					if (game.skDiff[i] == 0)
					{
						game.skDiff[i] = (DIFF_SLIDER_STOPS / 2);
					}

					break;
				}
			}
		}

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
		stopJoining();
		break;
	case MULTIOP_MAP_BUT:
		loadMapPreview();
		break;
	default:
		break;
	}

	//pop-up team chooser
	if((id >= MULTIOP_TEAMS_START) && (id <= MULTIOP_TEAMS_END))	// clicked on a team chooser
	{
		UDWORD clickedMenuID = id-MULTIOP_TEAMS_START,playerNetID;
		BOOL	bClickedOnMe = (NetPlay.players[clickedMenuID].dpid == player2dpid[selectedPlayer]);

		//find net player id
		for(playerNetID=0;(playerNetID <= MAX_PLAYERS) && (player2dpid[selectedPlayer] != NetPlay.players[playerNetID].dpid);playerNetID++);

		ASSERT(playerNetID < MAX_PLAYERS, "processMultiopWidgets: failed to find playerNetID for player %d", selectedPlayer);

		//make sure team chooser is not up before adding new one for another player
		if(teamChooserUp() < 0 && !bColourChooserUp)
		{
			if(bClickedOnMe		//player selecting team for himself	(clicked on his menu)
				|| (NetPlay.bHost							//local player=host
				&& !((NetPlay.players[clickedMenuID].dpid > 0) && !bClickedOnMe)	//human and not me
				))
				addTeamChooser(clickedMenuID);
		}
	}

	//clicked on a team
	if((id >= MULTIOP_TEAMCHOOSER) && (id <= MULTIOP_TEAMCHOOSER_END))
	{
		char msg[255];

		ASSERT(teamChooserUp() >= 0, "processMultiopWidgets: teamChooserUp() < 0");
		ASSERT((id - MULTIOP_TEAMCHOOSER) >= 0
			&& (id - MULTIOP_TEAMCHOOSER) < MAX_PLAYERS, "processMultiopWidgets: wrong id - MULTIOP_TEAMCHOOSER value (%d)", id - MULTIOP_TEAMCHOOSER);

		resetReadyStatus(false);		// will reset only locally if not a host

		SendTeamRequest(teamChooserUp(),(UBYTE)id-MULTIOP_TEAMCHOOSER);

		//playerTeamGUI[teamChooserUp()] = id - MULTIOP_TEAMCHOOSER;

		debug(LOG_WZ, "Changed team for player %d to %d", teamChooserUp(), playerTeamGUI[teamChooserUp()]);

		closeTeamChooser();
		addPlayerBox(  !ingame.bHostSetup || bHosted);	//restore initial options screen

		//enable locked teams mode
		//-----------------------------
		if(game.alliance != ALLIANCES_TEAMS && bHosted)		//only if host
		{
			resetReadyStatus(false);
			setLockedTeamsMode();		//update GUI

			sprintf( msg,"'%s' mode enabled", _("Locked Teams") );
			addConsoleMessage(msg,DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
	}

	// 'ready' button
	if((id >= MULTIOP_READY_START) && (id <= MULTIOP_READY_END))	// clicked on a player
	{
		UBYTE player = (UBYTE)(id-MULTIOP_READY_START);
		if(NetPlay.players[player].dpid == player2dpid[selectedPlayer] )
		{
			SendReadyRequest(selectedPlayer, !bPlayerReadyGUI[player]);

			// if hosting try to start the game if everyone is ready
			if(NetPlay.bHost && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
			}
		}
	}
		

	if((id >= MULTIOP_PLAYER_START) && (id <= MULTIOP_PLAYER_END))	// clicked on a player
	{
		if(NetPlay.players[id-MULTIOP_PLAYER_START].dpid == player2dpid[selectedPlayer] )
		{
			if(teamChooserUp() < 0)		//not choosing team already
				addColourChooser(id-MULTIOP_PLAYER_START);
		}
		else // options for kick out, etc..
		{
			if(NetPlay.bHost)
			{
				if(mouseDown(MOUSE_RMB)) // both buttons....
				{
					int victim = NetPlay.players[id - MULTIOP_PLAYER_START].dpid;	// who to kick out
					int j = 0;
					char *msg;

					while (player2dpid[j] != victim && j < MAX_PLAYERS)
					{
						j++; // find out ID of player
					}
					sasprintf(&msg, _("The host has kicked %s from the game!"), getPlayerName(j));
					sendTextMessage(msg, true);
					kickPlayer(victim);

					resetReadyStatus(true);		//reset and send notification to all clients
				}
			}
		}
	}

	if((id >= MULTIOP_SKSLIDE) && (id <=MULTIOP_SKSLIDE_END)) // choseskirmish difficulty.
	{
		UDWORD newValue, oldValue;

		if(		(id-MULTIOP_SKSLIDE == game.maxPlayers-1)
			&& (widgGetSliderPos(psWScreen,id) == 0)
			)
		{
			if((NetPlay.bComms == 0) || (NetPlay.playercount == 1))	//allow to disable all AIs in an mp game
			{
				game.skDiff[id-MULTIOP_SKSLIDE] = 1;
				widgSetSliderPos(psWScreen,id,1);
			}
		}

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

		sendOptions(0,0);
	}

	// don't kill last player
	if((id >= MULTIOP_COLCHOOSER) && (id <= MULTIOP_COLCHOOSER_END)) // chose a new colour.
	{
		resetReadyStatus(false);		// will reset only locally if not a host

		SendColourRequest(selectedPlayer,id-MULTIOP_COLCHOOSER,UBYTE_MAX);
		closeColourChooser();
		addPlayerBox(  !ingame.bHostSetup || bHosted);
	}

	// request a player number.
	if((id >= MULTIOP_PLAYCHOOSER) && (id <= MULTIOP_PLAYCHOOSER_END)) // chose a new colour.
	{
		resetReadyStatus(false);		// will reset only locally if not a host

		SendColourRequest(selectedPlayer,UBYTE_MAX,id-MULTIOP_PLAYCHOOSER);
		closeColourChooser();
		addPlayerBox(  !ingame.bHostSetup || bHosted);
	}
}

/* Start a multiplayer or skirmish game */
void startMultiplayerGame(void)
{
	decideWRF();										// set up swrf & game.map
	bMultiPlayer = true;

	if(NetPlay.bHost)
	{
		if(game.type == SKIRMISH)
		{
			chooseSkirmishColours();
			sendOptions(0,0);
		}

		NEThaltJoining();							// stop new players entering.
		SendFireUp();								//bcast a fireup message
	}

	// set the fog correctly..
	setRevealStatus(game.fog);
	war_SetFog(!game.fog);

	changeTitleMode(STARTGAME);

	bHosted = false;

	if(NetPlay.bHost)
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
		case NET_REQUESTMAP:
			recvMapFileRequested();
			break;
		case FILEMSG:
			recvMapFileData();
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

		case NET_TEAMREQUEST:
			recvTeamRequest();
			break;

		case NET_READY_REQUEST:
			recvReadyRequest();

			// if hosting try to start the game if everyone is ready
			if(NetPlay.bHost && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
			}
			break;

		case NET_PING:						// diagnostic ping msg.
			recvPing();
			break;

		case NET_LEAVING:					// remote player leaving.
		{
			BOOL host;
			uint32_t player_id;

			resetReadyStatus(false);

			NETbeginDecode(NET_LEAVING);
			{
				NETuint32_t(&player_id);
				NETbool(&host);
			}
			NETend();
			MultiPlayerLeave(player_id);
			if (host)					// host has quit, need to quit too.
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
			break;
		}
		case NET_FIREUP:					// campaign game started.. can fire the whole shebang up...
			if(ingame.localOptionsReceived)
			{
				decideWRF();

				// set the fog correctly..
				setRevealStatus(game.fog);
				war_SetFog(!game.fog);

				bMultiPlayer = true;
				changeTitleMode(STARTGAME);
				bHosted = false;
				break;
			}

		case NET_KICK:						// player is forcing someone to leave
		{
			uint32_t player_id;

			NETbeginDecode(NET_KICK);
				NETuint32_t(&player_id);
			NETend();

			if (NetPlay.dpidPlayer == player_id)	// we've been told to leave.
			{
				stopJoining();
			}
			break;
		}
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
	static UDWORD	lastrefresh=0;
	UDWORD			id,value;//,i;
	char			sTemp[128];
	PLAYERSTATS		playerStats;
	W_CONTEXT		context;

	KEY_CODE		k;
	char			str[3];

	frontendMultiMessages();


	// keep sending the map if required.
	if(bSendingMap)
	{
		sendMap();
	}

	// update boxes?
	if(lastrefresh > gameTime)lastrefresh= 0;
	if ((gameTime - lastrefresh) >2000)
	{
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

						if(value == 0 && (id == game.maxPlayers-1)  )
						{
							game.skDiff[id] = 1;
							widgSetSliderPos(psWScreen,id+MULTIOP_SKSLIDE,1);
						}
						else
						{
							game.skDiff[id] = value;
						}


						if(NetPlay.bHost)
						{
							sendOptions(0,0);
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
				strcpy((char*)sPlayer,sTemp);
				widgSetString(psWScreen,MULTIOP_PNAME,sTemp);

				sprintf(sTemp," -> %s",sPlayer);
				sendTextMessage(sTemp,true);

				NETchangePlayerName(NetPlay.dpidPlayer, (char*)sPlayer);
				loadMultiStats((char*)sPlayer,&playerStats);
				setMultiStats(NetPlay.dpidPlayer,playerStats,false);
				setMultiStats(NetPlay.dpidPlayer,playerStats,true);
				break;
			case MULTIOP_MAP:
				strcpy(game.map,sTemp);
				game.maxPlayers =(UBYTE) value;
				loadMapPreview();

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
}

// ////////////////////////////////////////////////////////////////////////////
BOOL startMultiOptions(BOOL bReenter)
{
	PLAYERSTATS		nullStats;
	UBYTE i;

	addBackdrop();
	addTopForm();

	if(!bReenter)
	{
		initPlayerColours();			 // force a colour clearout.

		for(i=0;i<MAX_PLAYERS;i++)
		{
//			game.skirmishPlayers[i] = 1; // clear out skirmish setting
//			game.skDiff[i] = (rand()%19)+1;	//1-20
			game.skDiff[i] = (DIFF_SLIDER_STOPS / 2);
		}

		if(!NetPlay.bComms)			// firce skirmish if no comms.
		{
			game.type				= SKIRMISH;
			strcpy(game.map, DEFAULTSKIRMISHMAP);
			game.maxPlayers = 4;
		}

		strlcpy(game.version, buildTime, sizeof(game.version));		// note buildtime.

		ingame.localOptionsReceived = false;
		if(ingame.numStructureLimits)
		{
			ingame.numStructureLimits = 0;
			free(ingame.pStructureLimits);
			ingame.pStructureLimits = NULL;
		}

		loadMultiStats((char*)sPlayer,&nullStats);
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
	char	tmp[8];
	UDWORD png;

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
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	//draw type overlay.
	if( NETgetGameFlagsUnjoined(i,1) == CAMPAIGN)
	{
		iV_DrawImage(FrontImages, IMAGE_ARENA_OVER, x + 59, y + 3);
	}
	else
	{
		iV_DrawImage(FrontImages,IMAGE_SKIRMISH_OVER,x+62,y+3);	// SKIRMISH
	}

	// ping rating
	png = NETgetGameFlagsUnjoined(i,2);
	if(png >= PING_LO && png < PING_MED)
	{
		iV_DrawImage(FrontImages,IMAGE_LAMP_GREEN,x+70,y+26);
	}
	else if(png >= PING_MED && png < PING_HI)
	{
		iV_DrawImage(FrontImages,IMAGE_LAMP_AMBER,x+70,y+26);
	}else
	{
		iV_DrawImage(FrontImages,IMAGE_LAMP_RED,x+70,y+26);
	}

	//draw game name
	while(iV_GetTextWidth(NetPlay.games[i].name) > (psWidget->width-110) )
	{
		NetPlay.games[i].name[strlen(NetPlay.games[i].name)-1]='\0';
	}
	iV_DrawText(NetPlay.games[i].name, x + 100, y + 24);	// name

	// get game info.
	// TODO: Check whether this code is used at all in skirmish games, if not, remove it.
	if ((NetPlay.games[i].desc.dwFlags & SESSION_JOINDISABLED)
	 || NetPlay.games[i].desc.dwCurrentPlayers >= NetPlay.games[i].desc.dwMaxPlayers        // if not joinable
	 || (bMultiPlayer
	  && !NetPlay.bComms
	  && NETgetGameFlagsUnjoined(gameNumber,1) == SKIRMISH                                  // the LAST bug...
	  && NetPlay.games[gameNumber].desc.dwCurrentPlayers >= NetPlay.games[gameNumber].desc.dwMaxPlayers - 1))
	{
		// FIXME: We should really use another way to indicate that the game is full than our current big fat cross.
		// need some sort of closed thing here!
		iV_DrawImage(FrontImages,IMAGE_NOJOIN,x+18,y+11);
	}
	else
	{
		iV_DrawText(_("Players"), x + 5, y + 18);
		sprintf(tmp,"%d/%d",NetPlay.games[i].desc.dwCurrentPlayers,NetPlay.games[i].desc.dwMaxPlayers);
		iV_DrawText(tmp, x + 17, y + 33);
	}

}


// ////////////////////////////////////////////////////////////////////////////
static UDWORD bestPlayer(UDWORD player)
{

	UDWORD i, myscore,  score, count=1;

	myscore =  getMultiStats(player,false).totalScore;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(isHumanPlayer(i) && i!=player )
		{
			score = getMultiStats(i, false).totalScore;

			if(score >= myscore)
			{
				count++;
			}
		}
	}

	return count;
}

void displayMultiReady(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	BOOL		Hilight = false;

	if( ((W_BUTTON*)psWidget)->state & (WBUTS_HILITE| WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK))
	{
		Hilight = true;
	}

	//bluboxes.
	drawBlueBox(x,y,psWidget->width,psWidget->height);							// right
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

	ASSERT(playerTeamGUI[i] >= 0 && playerTeamGUI[i] < MAX_PLAYERS,
		"displayTeamChooser: playerTeamGUI out of bounds" );

	//bluboxes.
	drawBlueBox(x,y,psWidget->width,psWidget->height);							// right

	iV_DrawImage(FrontImages,IMAGE_TEAM0 + playerTeamGUI[i],x+3,y+6);
}

// ////////////////////////////////////////////////////////////////////////////
void displayPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	BOOL		Hilight = false;
	UDWORD		j;
	UDWORD		i = psWidget->UserData, eval;
	PLAYERSTATS stat;

	if( ((W_BUTTON*)psWidget)->state & (WBUTS_HILITE| WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK))
	{
		Hilight = true;
	}

	//bluboxes.
	drawBlueBox(x,y,psWidget->width,psWidget->height);							// right

	if(ingame.localOptionsReceived && NetPlay.players[i].dpid)					// only draw if real player!
	{
		//bluboxes.
		drawBlueBox(x,y,psWidget->width,psWidget->height);							// right
		drawBlueBox(x,y,60,psWidget->height);
		drawBlueBox(x,y,31,psWidget->height);										// left.

		for(j=0; player2dpid[j] != NetPlay.players[i].dpid && j<MAX_PLAYERS; j++);// get the in game playernumber.

		iV_SetFont(font_regular);														// font
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);

		// name
		while(iV_GetTextWidth(NetPlay.players[i].name) > psWidget->width -68)	// clip name.
		{
			NetPlay.players[i].name[strlen(NetPlay.players[i].name)-1]='\0';
		}
		iV_DrawText(NetPlay.players[i].name, x + 65, y + 22);

		// ping rating
		if(ingame.PingTimes[j] >= PING_LO && ingame.PingTimes[j] < PING_MED)
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


		// player number
//		iV_DrawImage(FrontImages,IMAGE_WEE_GUY,x,y+23);
		switch(j)
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

		if(getMultiStats(j,false).played < 5)
		{
			iV_DrawImage(FrontImages,IMAGE_MEDAL_DUMMY,x+37,y+13);
		}
		else
		{
			stat = getMultiStats(j,false);

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
			if((stat.losses > 2) && (stat.wins > 2) && (stat.wins > (2 * stat.losses)))// bronze requirement.
			{
				if( stat.wins > (4 * stat.losses))								// silver requirement.
				{
					if(stat.wins > (8 * stat.losses))							// gold requirement
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

		switch(getPlayerColour(j))		//flag icon
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
		//unknown bugfix
//		game.skirmishPlayers[i] =true;	// don't clear this one!
		game.skDiff[i]=UBYTE_MAX;	// don't clear this one!

		// draw 'ready' button state
		if(bPlayerReadyGUI[i])
		{
			iV_DrawImage(FrontImages,IMAGE_OK,x+MULTIOP_READY_IMG_OFFSET_X,
				y+MULTIOP_READY_IMG_OFFSET_Y);
		}
		else
		{
			iV_DrawImage(FrontImages,IMAGE_NO,x+MULTIOP_READY_IMG_OFFSET_X,
				y+MULTIOP_READY_IMG_OFFSET_Y);
		}
	}
	else
	{
		//bluboxes.
		drawBlueBox(x,y,psWidget->width,psWidget->height);							// right
		//drawBlueBox(x,y,31,psWidget->height);										// left.

		//if(game.type == SKIRMISH && game.skDiff[i])
		//{
		//	iV_DrawImage(FrontImages,IMAGE_PLAYER_PC,x+2,y+9);
		//}
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
	UWORD	im = psWidget->UserData;

	drawBlueBox(x,y,psWidget->width,psWidget->height);
	drawBlueBox(x+psWidget->width,y,psWidget->height,psWidget->height);	// box on end.

	iV_DrawImage(FrontImages,im,x+psWidget->width+2,y+4);			//icon descriptor.

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
	UWORD	hiToUse = 0;
	UDWORD	Grey = 0;
	UWORD	im = UNPACKDWORD_TRI_B((UDWORD)psWidget->UserData);
	UWORD	im2= UNPACKDWORD_TRI_C((UDWORD)psWidget->UserData);
	BOOL	usehl = UNPACKDWORD_TRI_A((UDWORD)psWidget->UserData);

	//evaluate
	if( (usehl==1) && ((W_BUTTON*)psWidget)->state & WBUTS_HILITE)
	{
		Hilight = true;
		switch(iV_GetImageWidth(FrontImages,im))			//pick a hilight.
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
//			DBPRINTF(("no multibut highlight for width = %d",iV_GetImageWidth(FrontImages,im)));
			break;
		}

		if(im == IMAGE_RETURN)
		{
			hiToUse = IMAGE_RETURN_HI;
		}

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
	iV_DrawImage(FrontImages,im,x,y);

	// hilight with a number...just for player selector
	if(usehl >=10)
	{
		iV_DrawImage(IntImages,IMAGE_ASCII48-10+usehl,x+11,y+8);
	}

	// hilights etc..
	if(Hilight && !Grey)
	{
		if(Down)
		{
			iV_DrawImage(FrontImages,im2,x,y);
		}

		if(hiToUse)
		{
			iV_DrawImage(FrontImages,hiToUse,x,y);
		}

	}
	else if(Down)
	{
		iV_DrawImage(FrontImages,im2,x,y);
	}


	if (Grey) {
		// disabled, render something over it!
		iV_TransBoxFill(x,y,x+psWidget->width,y+psWidget->height);
	}
}



/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// common widgets

static BOOL addMultiEditBox(UDWORD formid,UDWORD id,UDWORD x, UDWORD y, const char* tip, char tipres[128],UDWORD icon,UDWORD iconid)
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
	sEdInit.UserData = icon;
	sEdInit.pBoxDisplay = displayMultiEditBox;
	if (!widgAddEditBox(psWScreen, &sEdInit))
	{
		return false;
	}

	// note drawing is done by the editbox draw tho...

	addMultiBut(psWScreen,MULTIOP_OPTIONS ,iconid,(x+MULTIOP_EDITBOXW+2),(y+2) ,
				 MULTIOP_EDITBOXH, MULTIOP_EDITBOXH, tip,
				 icon,icon,false);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

BOOL addMultiBut(W_SCREEN *screen, UDWORD formid, UDWORD id, UDWORD x, UDWORD y, UDWORD width, UDWORD height, const char* tipres, UDWORD norm, UDWORD hi, BOOL hiIt)
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
	sButInit.UserData = PACKDWORD_TRI(hiIt,norm , hi);

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
	if(bHosted)
	{
		sendOptions(0,0);
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
		if(NetPlay.players[player].dpid && !bPlayerReadyGUI[player])
		{
			if(bNotifyStatus)
			{
				for(playerID=0;(playerID <= game.maxPlayers) && (player2dpid[playerID] != NetPlay.players[player].dpid);playerID++);

				console("%s is not ready", getPlayerName(playerID));
			}

			bReady = false;
		}
	}

	return bReady;
}
