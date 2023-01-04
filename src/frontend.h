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

#ifndef __INCLUDED_SRC_FRONTEND_H__
#define __INCLUDED_SRC_FRONTEND_H__

#include "lib/widget/widgbase.h"
#include "levels.h"

// determines which option screen to use. when in GS_TITLE_SCREEN mode.
enum tMode
{
	TITLE,			// 0 intro mode
	SINGLE,			// 1 single player menu
	MULTI,			// 2 multiplayer menu
	OPTIONS,		// 3 options menu
	GAME,			// 4
	TUTORIAL,		// 5  tutorial/fastplay
	TITLE_UNUSED,	// 6
	FORCESELECT,	// 7 MULTIPLAYER, Force design screen
	STARTGAME,		// 8 Fire up the game
	SHOWINTRO,		// 9 reshow the intro
	QUIT,			// 10 leaving game
	LOADSAVEGAME,	// 11 loading a save game
	KEYMAP,			// 12 keymap editor
	GRAPHICS_OPTIONS,       // 13 graphics options menu
	AUDIO_AND_ZOOM_OPTIONS, // 14 audio and zoom options menu
	VIDEO_OPTIONS,          // 15 video options menu
	MOUSE_OPTIONS,          // 16 mouse options menu
	CAMPAIGNS,              // 17 campaign selector
	MUSIC_MANAGER,			// 18 music manager
	MULTIPLAY_OPTIONS,		// 19 multiplay options menu
};

#define MAX_LEVEL_NAME_SIZE	(256)

extern char	aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.

extern bool	bLimiterLoaded;


void changeTitleMode(tMode mode);
bool runTitleMenu();
bool runSinglePlayerMenu();
bool runCampaignSelector();
bool runMultiPlayerMenu();
bool runGameOptionsMenu();
bool runMultiplayOptionsMenu();
bool runOptionsMenu();
bool runGraphicsOptionsMenu();
bool runAudioAndZoomOptionsMenu();
bool runVideoOptionsMenu();
bool runMouseOptionsMenu();
bool runTutorialMenu();
void runContinue();
void startTitleMenu();
void startTutorialMenu();
void startSinglePlayerMenu();
void startCampaignSelector();
void startMultiPlayerMenu();
void startOptionsMenu();
void startGraphicsOptionsMenu();
void startAudioAndZoomOptionsMenu();
void startVideoOptionsMenu();
void startMouseOptionsMenu();
void startGameOptionsMenu();
void startMultiplayOptionsMenu();
void refreshCurrentVideoOptionsValues();
void frontendIsShuttingDown();

void addTopForm(bool wide);
void addBottomForm(bool wide = false);
W_FORM *addBackdrop();
W_FORM *addBackdrop(const std::shared_ptr<W_SCREEN> &screen);
void addTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const std::string &txt, unsigned int style);
W_LABEL *addSideText(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt);
W_LABEL *addSideText(const std::shared_ptr<W_SCREEN> &screen, UDWORD formId, UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt);
void addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos);

void displayTextOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

bool CancelPressed();

/* Tell the frontend when the screen has been resized */
void frontendScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight);

// Graphics options, shared for in-game options menu use
char const *graphicsOptionsFmvmodeString();
char const *graphicsOptionsScanlinesString();
char const *graphicsOptionsSubtitlesString();
char const *graphicsOptionsShadowsString();
char const *graphicsOptionsRadarString();
char const *graphicsOptionsRadarJumpString();
char const *graphicsOptionsScreenShakeString();
void seqFMVmode();
void seqScanlineMode();

// Video options, shared for in-game options menu use
char const *videoOptionsDisplayScaleLabel();
char const *videoOptionsVsyncString();
std::string videoOptionsDisplayScaleString();
std::vector<unsigned int> availableDisplayScalesSorted();
void seqDisplayScale();
void seqVsyncMode();

// Mouse options, shared for in-game options menu use
char const *mouseOptionsMflipString();
char const *mouseOptionsTrapString();
char const *mouseOptionsMbuttonsString();
char const *mouseOptionsMmrotateString();
char const *mouseOptionsCursorModeString();
char const *mouseOptionsScrollEventString();
void seqScrollEvent();

struct DisplayTextOptionCache
{
	enum class OverflowBehavior
	{
		None,
		ShrinkFont
	};
	OverflowBehavior overflowBehavior = OverflowBehavior::None;
	int lastWidgetWidth = 0;
	WzText wzText;
};


// ////////////////////////////////////////////////////////////////////////////
// defines.

#define FRONTEND_TOPFORMX		50
#define FRONTEND_TOPFORMY		10
#define FRONTEND_TOPFORMW		540
#define FRONTEND_TOPFORMH		150


#define FRONTEND_TOPFORM_WIDEX	28
#define FRONTEND_TOPFORM_WIDEY	10
#define FRONTEND_TOPFORM_WIDEW	588
#define FRONTEND_TOPFORM_WIDEH	150


#define FRONTEND_BOTFORMX		FRONTEND_TOPFORMX
#define FRONTEND_BOTFORMY		170
#define FRONTEND_BOTFORMW		FRONTEND_TOPFORMW
#define FRONTEND_BOTFORMH		305				// keep Y+H < 480 (minimum display height)


#define FRONTEND_BOTFORM_WIDEX		30
#define FRONTEND_BOTFORM_WIDEY		FRONTEND_BOTFORMY
#define FRONTEND_BOTFORM_WIDEW		580
#define FRONTEND_BOTFORM_WIDEH		FRONTEND_BOTFORMH	// keep Y+H < 480 (minimum display height)


#define FRONTEND_BUTWIDTH		FRONTEND_BOTFORMW-40 // text button sizes.
#define FRONTEND_BUTHEIGHT		35

#define FRONTEND_BUTWIDTH_WIDE	FRONTEND_BOTFORM_WIDEW-40 // text button sizes.
#define FRONTEND_BUTHEIGHT_WIDE	FRONTEND_BUTHEIGHT

#define FRONTEND_POS1X			20				// button positions
#define FRONTEND_POS1Y			(0*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS1M			340

#define FRONTEND_POS2X			20
#define FRONTEND_POS2Y			(1*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS2M			340

#define FRONTEND_POS3X			20
#define FRONTEND_POS3Y			(2*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS3M			340

#define FRONTEND_POS4X			20
#define FRONTEND_POS4Y			(3*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS4M			340

#define FRONTEND_POS5X			20
#define FRONTEND_POS5Y			(4*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS5M			340

#define FRONTEND_POS6X			20
#define FRONTEND_POS6Y			(5*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS6M			340

#define FRONTEND_POS7X			20
#define FRONTEND_POS7Y			(6*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS7M			340

#define FRONTEND_POS8X			20
#define FRONTEND_POS8Y			(7*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS8M			340

#define FRONTEND_POS9X			-30				// special case for our hyperlink
#define FRONTEND_POS9Y			(8*FRONTEND_BUTHEIGHT)


#define FRONTEND_SIDEX			24
#define FRONTEND_SIDEY			FRONTEND_BOTFORMY

enum
{
	FRONTEND_BACKDROP		= 20000,
	FRONTEND_TOPFORM,
	FRONTEND_BOTFORM,
	FRONTEND_LOGO,
	FRONTEND_SIDETEXT,					// sideways text
	FRONTEND_MULTILINE_SIDETEXT,				// sideways text
	FRONTEND_SIDETEXT1,					// sideways text
	FRONTEND_SIDETEXT2,					// sideways text
	FRONTEND_SIDETEXT3,					// sideways text
	FRONTEND_SIDETEXT4,					// sideways text
	FRONTEND_PASSWORDFORM,
	FRONTEND_HYPERLINK,
	FRONTEND_UPGRDLINK,
	FRONTEND_DONATELINK,
	FRONTEND_CHATLINK,
	// begin menu
	FRONTEND_SINGLEPLAYER	= 20100,	// title screen
	FRONTEND_MULTIPLAYER,
	FRONTEND_TUTORIAL,
	FRONTEND_PLAYINTRO,
	FRONTEND_OPTIONS,
	FRONTEND_QUIT,
	FRONTEND_FASTPLAY,					//tutorial menu option
	FRONTEND_CONTINUE,
	FRONTEND_NEWGAME		= 20200,	// single player (menu)
	FRONTEND_LOADGAME_MISSION,
	FRONTEND_LOADGAME_SKIRMISH,
	FRONTEND_REPLAY,
	FRONTEND_SKIRMISH,
	FRONTEND_CHALLENGES,
	FRONTEND_HOST			= 20300,	//multiplayer menu options
	FRONTEND_JOIN,
	FE_P0,								// player 0 buton
	FE_P1,								// player 1 buton
	FE_P2,								// player 2 buton
	FE_P3,								// player 3 buton
	FE_P4,								// player 4 buton
	FE_P5,								// player 5 buton
	FE_P6,								// player 6 buton
	FE_P7,								// player 7 buton
	FE_MP_PR,  // Multiplayer player random button
	FE_MP_PMAX = FE_MP_PR + MAX_PLAYERS_IN_GUI,  // Multiplayer player blah button

	FRONTEND_CAMPAIGN_1,
	FRONTEND_CAMPAIGN_2,
	FRONTEND_CAMPAIGN_3,
	FRONTEND_CAMPAIGN_4,
	FRONTEND_CAMPAIGN_5,
	FRONTEND_CAMPAIGN_6,

	FRONTEND_GAMEOPTIONS = 21000,           // Game Options menu
	FRONTEND_LANGUAGE,
	FRONTEND_LANGUAGE_R,
	FRONTEND_COLOUR,
	FRONTEND_COLOUR_CAM,
	FRONTEND_COLOUR_MP,
	FRONTEND_DIFFICULTY,
	FRONTEND_DIFFICULTY_R,
	FRONTEND_CAMERASPEED,
	FRONTEND_CAMERASPEED_R,

	FRONTEND_GRAPHICSOPTIONS = 22000,       // Graphics Options Menu
	FRONTEND_FMVMODE,
	FRONTEND_FMVMODE_R,
	FRONTEND_SCANLINES,
	FRONTEND_SCANLINES_R,
	FRONTEND_SHADOWS,
	FRONTEND_SHADOWS_R,
	FRONTEND_FOG,
	FRONTEND_FOG_R,
	FRONTEND_RADAR,
	FRONTEND_RADAR_R,
	FRONTEND_RADAR_JUMP,
	FRONTEND_RADAR_JUMP_R,
	FRONTEND_LOD_DISTANCE,
	FRONTEND_LOD_DISTANCE_R,
	FRONTEND_SSHAKE,
	FRONTEND_SSHAKE_R,

	FRONTEND_AUDIO_AND_ZOOMOPTIONS = 23000,                 // Audio and Zoom Options Menu
	FRONTEND_3D_FX,						// 3d sound volume
	FRONTEND_FX,						// 2d (voice) sound volume
	FRONTEND_MUSIC,						// music volume
	FRONTEND_SUBTITLES,
	FRONTEND_SUBTITLES_R,
	FRONTEND_SOUND_HRTF,				// HRTF mode
	FRONTEND_MAP_ZOOM,					// map zoom
	FRONTEND_MAP_ZOOM_RATE,					// map zoom rate
	FRONTEND_RADAR_ZOOM,					// radar zoom rate
	FRONTEND_3D_FX_SL,
	FRONTEND_FX_SL,
	FRONTEND_MUSIC_SL,
	FRONTEND_SOUND_HRTF_R,
	FRONTEND_MAP_ZOOM_R,
	FRONTEND_MAP_ZOOM_RATE_R,
	FRONTEND_RADAR_ZOOM_R,

	FRONTEND_VIDEOOPTIONS = 24000,          // video Options Menu
	FRONTEND_WINDOWMODE,
	FRONTEND_WINDOWMODE_R,
	FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER,
	FRONTEND_RESOLUTION_READONLY_LABEL,
	FRONTEND_RESOLUTION_READONLY_CONTAINER,
	FRONTEND_RESOLUTION_READONLY,
	FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER,
	FRONTEND_RESOLUTION_DROPDOWN_LABEL,
	FRONTEND_RESOLUTION_DROPDOWN,
	FRONTEND_TEXTURESZ,
	FRONTEND_TEXTURESZ_R,
	FRONTEND_VSYNC,
	FRONTEND_VSYNC_R,
	FRONTEND_FSAA,
	FRONTEND_FSAA_R,
	FRONTEND_DISPLAYSCALE,
	FRONTEND_DISPLAYSCALE_R,
	FRONTEND_GFXBACKEND,
	FRONTEND_GFXBACKEND_R,
	FRONTEND_MINIMIZE_ON_FOCUS_LOSS,
	FRONTEND_MINIMIZE_ON_FOCUS_LOSS_DROPDOWN,
	FRONTEND_ALTENTER_TOGGLE_MODE,
	FRONTEND_ALTENTER_TOGGLE_MODE_DROPDOWN,

	FRONTEND_MOUSEOPTIONS = 25000,          // Mouse Options Menu
	FRONTEND_CURSORMODE,
	FRONTEND_CURSORMODE_R,
	FRONTEND_TRAP,
	FRONTEND_TRAP_R,
	FRONTEND_MFLIP,
	FRONTEND_MFLIP_R,
	FRONTEND_MBUTTONS,
	FRONTEND_MBUTTONS_R,
	FRONTEND_MMROTATE,
	FRONTEND_MMROTATE_R,
	FRONTEND_SCROLLEVENT,
	FRONTEND_SCROLLEVENT_R,
	FRONTEND_CURSORSCALE,
	FRONTEND_CURSORSCALE_DROPDOWN,

	FRONTEND_KEYMAP			= 26000,	// Keymap menu

	FRONTEND_MUSICMANAGER   = 27000,	// Music manager menu

	FRONTEND_MULTIPLAYOPTIONS = 28000,	// Multiplayer Options Menu
	FRONTEND_GAME_PORT,
	FRONTEND_GAME_PORT_R,
	FRONTEND_UPNP,
	FRONTEND_UPNP_R,
	FRONTEND_INACTIVITY_TIMEOUT,
	FRONTEND_INACTIVITY_TIMEOUT_DROPDOWN,
	FRONTEND_LAG_KICK,
	FRONTEND_LAG_KICK_DROPDOWN,
	FRONTEND_SPECTATOR_SLOTS,
	FRONTEND_SPECTATOR_SLOTS_DROPDOWN,
	FRONTEND_AUTORATING,
	FRONTEND_AUTORATING_R,

	FRONTEND_NOGAMESAVAILABLE = 31666	// Used when no games are available in lobby

};

void SPinit(LEVEL_TYPE gameType);

#endif // __INCLUDED_SRC_FRONTEND_H__
