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
 * Frontend.h
 */
#ifndef _frontend_h
#define _frontend_h

#include "lib/widget/widgbase.h"

// determines which option screen to use. when in GS_TITLE_SCREEN mode.
typedef enum _title_mode {
	TITLE,			// 0 intro mode
	SINGLE,			// 1 single player menu
	MULTI,			// 2 multiplayer menu
	OPTIONS,		// 3 options menu
	GAME,			// 4
	TUTORIAL,		// 5  tutorial/fastplay
	CREDITS,		// 6  credits
	PROTOCOL,		// 7  MULTIPLAYER, select proto
	MULTIOPTION,	// 8 MULTIPLAYER, select game options
	FORCESELECT,	// 9 MULTIPLAYER, Force design screen
	GAMEFIND,		// 10 MULTIPLAYER, gamefinder.
	MULTILIMIT,		// 11 MULTIPLAYER, Limit the multistuff.
	STARTGAME,		// 12 Fire up the game
	SHOWINTRO,		// 13 reshow the intro
	QUIT,			// 14 leaving game
	LOADSAVEGAME,	// 15 loading a save game
	KEYMAP,			// 16 keymap editor
	GAME2,			// 17 second options menu.
	GAME3,			// 18 third options menu.
	GAME4,			// 19 fourth options menu.
//	GRAPHICS,
//	VIDEO,
//	DEMOMODE,		// demo mode. remove for release?
} tMode;

// This dos'nt compile on the PSX.
//typedef enum _titlemode tMode;	// define the type

extern tMode titleMode;					// the global case

#define DEFAULT_LEVEL	"CAM_1A"
#define TUTORIAL_LEVEL	"TUTORIAL3"

#define MAX_LEVEL_NAME_SIZE	(256)

extern char	aLevelName[MAX_LEVEL_NAME_SIZE+1];	//256];			// vital! the wrf file to use.

extern BOOL	bUsingKeyboard;	// to disable mouse pointer when using keys.
extern BOOL	bUsingSlider;

extern BOOL	bForceEditorLoaded;


extern void	changeTitleMode			(tMode mode);
extern BOOL startTitleMenu			(void);
extern BOOL runTitleMenu			(void);
extern BOOL runSinglePlayerMenu		(void);
extern BOOL runMultiPlayerMenu		(void);
extern BOOL runGameOptionsMenu		(void);
extern BOOL runDemoMenu				(void);
extern BOOL runOptionsMenu			(void);
extern BOOL runTutorialMenu			(void);

extern void processFrontendSnap		(BOOL bHideCursor);

extern void addTopForm				(void);
extern void addBottomForm			(void);
extern void addBackdrop				(void);
extern void	addTextButton			(UDWORD id,  UDWORD PosX, UDWORD PosY, const char *txt,BOOL bAlignLeft,BOOL bGrey);
extern void	addSideText				(UDWORD id,  UDWORD PosX, UDWORD PosY, const char *txt);
extern void addFESlider				(UDWORD id, UDWORD parent, UDWORD x,UDWORD y,UDWORD stops,UDWORD pos,UDWORD attachID );

extern void	displayLogo				(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
extern void	displayTextOption		(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

extern void loadOK					(void);
extern BOOL CancelPressed			(void);

BOOL		runGameOptions2Menu		(void);
BOOL		runGameOptions3Menu		(void);
BOOL		runGameOptions4Menu		(void);

// ////////////////////////////////////////////////////////////////////////////
// defines.

#define FRONTEND_BACKDROP		20000

#define FRONTEND_TOPFORM		20001

#define FRONTEND_TOPFORMX		80
#define FRONTEND_TOPFORMY		10
#define FRONTEND_TOPFORMW		480
#define FRONTEND_TOPFORMH		150


#define FRONTEND_TOPFORM_WIDEX	28
#define FRONTEND_TOPFORM_WIDEY	10
#define FRONTEND_TOPFORM_WIDEW	588
#define FRONTEND_TOPFORM_WIDEH	150


#define FRONTEND_BOTFORM		20002

#define FRONTEND_BOTFORMX		80
#define FRONTEND_BOTFORMY		170
#define FRONTEND_BOTFORMW		480
#define FRONTEND_BOTFORMH		300


#define FRONTEND_BUTWIDTH		FRONTEND_BOTFORMW-40 // text button sizes.
#define FRONTEND_BUTHEIGHT		30


#define FRONTEND_POS1X			20				// button positions
#define FRONTEND_POS1Y			10
#define FRONTEND_POS1M			290

#define FRONTEND_POS2X			20
#define FRONTEND_POS2Y			50
#define FRONTEND_POS2M			290

#define FRONTEND_POS3X			20
#define FRONTEND_POS3Y			90
#define FRONTEND_POS3M			290

#define FRONTEND_POS4X			20
#define FRONTEND_POS4Y			130
#define FRONTEND_POS4M			290

#define FRONTEND_POS5X			20
#define FRONTEND_POS5Y			170
#define FRONTEND_POS5M			290

#define FRONTEND_POS6X			20
#define FRONTEND_POS6Y			210
#define FRONTEND_POS6M			290

#define FRONTEND_POS7X			20
#define FRONTEND_POS7Y			250
#define FRONTEND_POS7M			290



#define FRONTEND_SINGLEPLAYER	20003		// title screen
#define FRONTEND_MULTIPLAYER	20004
#define FRONTEND_QUIT			20005
#define FRONTEND_OPTIONS		20006

#define FRONTEND_HOST			20007		//multiplayer screen
#define FRONTEND_JOIN			20008
#define FRONTEND_FORCEEDIT		20009
#define FRONTEND_SKIRMISH		20010

#define FRONTEND_NEWGAME		20011		// single player
#define FRONTEND_LOADGAME		20012
#define FRONTEND_PLAYINTRO		20013

//#define FRONTEND_MOUSESPEED		20014		// options
#define FRONTEND_SCROLLSPEED	20015
#define FRONTEND_3D_FX			20016
#define FRONTEND_FX			20017
#define FRONTEND_MUSIC			20018

#define FRONTEND_TUTORIAL		20019
#define FRONTEND_FASTPLAY		20020
//#define FRONTEND_FOGTYPE		20021

//#define FRONTEND_MOUSESPEED_SL	20021		// options sliders
#define FRONTEND_SCROLLSPEED_SL	20022
#define FRONTEND_3D_FX_SL		20023
#define FRONTEND_FX_SL			20024
#define FRONTEND_MUSIC_SL		20025

#define FRONTEND_SIDETEXT		20026		// side-ee-ways text
#define FRONTEND_SIDEX			44
#define FRONTEND_SIDEY			FRONTEND_BOTFORMY

#define FRONTEND_SIDETEXT1		20027		// side-ee-ways text
#define FRONTEND_SIDETEXT2		20028		// side-ee-ways text
#define FRONTEND_SIDETEXT3		20029		// side-ee-ways text
#define FRONTEND_SIDETEXT4		20030		// side-ee-ways text

#define FE_P0					20031
#define FE_P1					20032
#define FE_P2					20033
#define FE_P3					20034
#define FE_P4					20035
#define FE_P5					20036
#define FE_P6					20037
#define FE_P7					20038

#define FRONTEND_VIBRO			20039
#define FRONTEND_VIBRO_BT		20040
#define FRONTEND_CONTROL		20041
#define FRONTEND_CONTROL_BT		20042

//#define FRONTEND_VIDEO			20051
//#define FRONTEND_SOFTWARE		20052
//#define FRONTEND_DIRECTX		20053
//#define FRONTEND_OPENGL			20054
//#define FRONTEND_GLIDE			20055

#define FRONTEND_DEMO			20056
#define FRONTEND_DEMO1			20057
#define FRONTEND_DEMO2			20058
#define FRONTEND_DEMO3			20059
#define FRONTEND_DEMO4			20060
#define FRONTEND_DEMO5			20061

#define FRONTEND_LOGO			20062

#define FRONTEND_GAMEOPTIONS 	20063
//#define FRONTEND_GRAPHICS		20064
#define FRONTEND_TEXTURES		20065
#define FRONTEND_TEXTURES_R		20066
#define FRONTEND_EFFECTS		20067
#define FRONTEND_EFFECTS_R		20068
#define FRONTEND_SUBTITLES		20069
#define FRONTEND_SUBTITLES_R	20070
#define FRONTEND_SHADOWS	20071
#define FRONTEND_SHADOWS_R	20072

#define FRONTEND_COLOUR			20073

#define FRONTEND_DIFFICULTY		20074
#define FRONTEND_DIFFICULTY_R	20075

#define FRONTEND_LOGOW			248
#define FRONTEND_LOGOH			118

#define FRONTENDHELP_SELECT		20076
#define FRONTENDHELP_CANCEL		20077

#define FRONTEND_FOGTYPE		20078
#define FRONTEND_FOGTYPE_R		20079

#define FRONTEND_LOADCAM2		20080
#define FRONTEND_LOADCAM3		20081

#define FRONTEND_SCREENSHAKE	20082
#define FRONTEND_SCREENSHAKE_BT	20083
#define FRONTEND_CENTRESCREEN	20084
#define FRONTEND_PADUP			20085
#define FRONTEND_PADDOWN		20086
#define FRONTEND_PADLEFT		20087
#define FRONTEND_PADRIGHT		20088

#define FRONTEND_KEYMAP			20089

#define FRONTEND_CURSOR			20090
#define FRONTEND_CURSOR_SL		20091

#define FRONTEND_GAMEOPTIONS2 	20092
#define FRONTEND_SSHAKE			20093
#define FRONTEND_SSHAKE_R		20094
#define FRONTEND_MFLIP			20095
#define FRONTEND_MFLIP_R		20096

#define FRONTEND_GAMEOPTIONS3 	20099

#define FRONTEND_GAMEOPTIONS4	31415
#define FRONTEND_WINDOWMODE		31416
#define FRONTEND_WINDOWMODE_R	31417
#define FRONTEND_RESOLUTION		31418
#define FRONTEND_RESOLUTION_R	31419
#define FRONTEND_TRAP			31420
#define FRONTEND_TRAP_R			31421
#define FRONTEND_TEXTURESZ		31422
#define FRONTEND_TEXTURESZ_R	31423
#define FRONTEND_TAKESEFFECT	31424

#define FRONTEND_SEQUENCE		20097
#define FRONTEND_SEQUENCE_R		20098


#endif
