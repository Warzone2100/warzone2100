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
/** @file
 *  In game options screen
 */

#ifndef __INCLUDED_SRC_INGAMEOP_H__
#define __INCLUDED_SRC_INGAMEOP_H__

// functions
bool intAddInGameOptions();
void intReopenMenuWithoutUnPausing();
bool intCloseInGameOptions(bool bPutUpLoadSave, bool bResetMissionWidgets);
void intCloseInGameOptionsNoAnim();
void intProcessInGameOptions(UDWORD);
void intAddInGamePopup();

extern bool hostQuitConfirmation;

// status bools.
extern bool	InGameOpUp;
extern bool isInGamePopupUp;

// ////////////////////////////////////////////////////////////////////////////
// defines

// position info for window.

// game options

// initial options
#define INTINGAMEOP_W			150
#define INTINGAMEOP_H			148
#define INTINGAMEOP_HS			112

#define INTINGAMEOP_X			((320-(INTINGAMEOP_W/2))+D_W)
#define INTINGAMEOP_Y			((240-(INTINGAMEOP_H/2))+D_H)

#define INTINGAMEOP2_W			350
#define INTINGAMEOP2_H			160
#define INTINGAMEOP2_X			((320-(INTINGAMEOP2_W/2))+D_W)
#define INTINGAMEOP2_Y			((240-(INTINGAMEOP2_H/2))+D_H)

#define INTINGAMEOPLINE_H		17
#define INTINGAMEOPMARGIN_H		10
#define INTINGAMEOPAUTO_H(nlines)	((nlines)*INTINGAMEOPLINE_H+INTINGAMEOPMARGIN_H*2)
#define INTINGAMEOPAUTO_Y(nlines)	((240-(INTINGAMEOPAUTO_H(nlines)/2))+D_H)
#define INTINGAMEOPAUTO_Y_LINE(line)	(((line)-1)*INTINGAMEOPLINE_H+INTINGAMEOPMARGIN_H)

// Host quit confirmation.
#define INTINGAMEOP3_W			150
#define INTINGAMEOP3_H			65
#define INTINGAMEOP3_X			((320-(INTINGAMEOP3_W/2))+D_W)
#define INTINGAMEOP3_Y			((240-(INTINGAMEOP3_H/2))+D_H)

//Singleplayer quit confirmation
#define INTINGAMEOP4_W			400
#define INTINGAMEOP4_X			((320-(INTINGAMEOP4_W/2))+D_W)

#define PAUSEMESSAGE_YOFFSET (0)

// button sizes.
#define	INTINGAMEOP_OP_W		(INTINGAMEOP_W-10)
#define	INTINGAMEOP4_OP_W		(INTINGAMEOP4_W-15)
#define	INTINGAMEOP_SW_W		(INTINGAMEOP2_W - 15)
#define	INTINGAMEOP_OP_H		15

enum
{
	INTINGAMEOP = 10500,
	INTINGAMEPOPUP,
	INTINGAMEOP_HOSTQUIT,
	INTINGAMEOP_CONFIRM_QUIT,
	INTINGAMEOP_QUIT,               ///< The all important quit button
	INTINGAMEOP_RESUME,
	INTINGAMEOP_GO_BACK,
	INTINGAMEOP_LOAD_MISSION,
	INTINGAMEOP_LOAD_SKIRMISH,
	INTINGAMEOP_SAVE_MISSION,
	INTINGAMEOP_SAVE_SKIRMISH,
	INTINGAMEOP_OPTIONS,
	INTINGAMEOP_PAUSELABEL,                 ///< The paused message
	INTINGAMEOP_CENTRESCREEN,
	INTINGAMEOP_REPLAY,
	INTINGAMEOP_POPUP_QUIT,
	INTINGAMEOP_POPUP_MSG1,
	INTINGAMEOP_POPUP_MSG2,
	INTINGAMEOP_OPENGAMEGUIDE
};



// positions within option boxes.
#define INTINGAMEOP_1_X		5
#define INTINGAMEOP_2_X		10
#define INTINGAMEOP_MID		160
#define	INTINGAMEOP_1_Y		20
#define	INTINGAMEOP_2_Y		40

#define OPALIGN		(WBUT_PLAIN | WBUT_TXTCENTRE)

#endif // __INCLUDED_SRC_INGAMEOP_H__
