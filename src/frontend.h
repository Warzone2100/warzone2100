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
	TUTORIAL,		// 3  tutorial/fastplay
	TITLE_UNUSED,	// 4
	FORCESELECT,	// 5 MULTIPLAYER, Force design screen
	STARTGAME,		// 6 Fire up the game
	SHOWINTRO,		// 7 reshow the intro
	QUIT,			// 8 leaving game
	LOADSAVEGAME,	// 9 loading a save game
	CAMPAIGNS,      // 10 campaign selector
};

#define MAX_LEVEL_NAME_SIZE	(256)

extern char	aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.

extern bool	bLimiterLoaded;


void changeTitleMode(tMode mode);
bool runTitleMenu();
bool runSinglePlayerMenu();
bool runMultiPlayerMenu();
bool runTutorialMenu();
void runContinue();
void startTitleMenu();
void startTutorialMenu();
void startSinglePlayerMenu();
void startMultiPlayerMenu();
void frontendIsShuttingDown();

void notifyAboutMissingVideos();

std::shared_ptr<IMAGEFILE> getFlagsImages();

void addTopForm(bool wide);
void addBottomForm(bool wide = false);
W_FORM *addBackdrop();
W_FORM *addBackdrop(const std::shared_ptr<W_SCREEN> &screen);
std::shared_ptr<W_BUTTON> addTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const std::string &txt, unsigned int style);
W_BUTTON * addSmallTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, unsigned int style);
std::shared_ptr<W_LABEL> addSideText(WIDGET* psParent, UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt);
W_LABEL *addSideText(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt);
W_LABEL *addSideText(const std::shared_ptr<W_SCREEN> &screen, UDWORD formId, UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt);
std::shared_ptr<W_SLIDER> addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos);

void displayTextOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

bool CancelPressed();

/* Tell the frontend when the screen has been resized */
void frontendScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight);

std::vector<unsigned int> availableDisplayScalesSorted();

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

#define FRONTEND_BUTHEIGHT_LIST_SPACER		(FRONTEND_BUTHEIGHT + 15)

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

	FRONTEND_KEYMAP			= 26000,	// Keymap menu

	FRONTEND_MUSICMANAGER   = 27000,	// Music manager menu

	FRONTEND_NOGAMESAVAILABLE = 31666	// Used when no games are available in lobby

};

void SPinit(LEVEL_TYPE gameType);

#endif // __INCLUDED_SRC_FRONTEND_H__
