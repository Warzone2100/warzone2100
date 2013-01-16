/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
extern bool intAddInGameOptions			(void);
extern bool intCloseInGameOptions		(bool bPutUpLoadSave, bool bResetMissionWidgets);
extern void intCloseInGameOptionsNoAnim	(bool bResetMissionWidgets);
extern bool intRunInGameOptions			(void);
extern void intProcessInGameOptions		(UDWORD);
extern void intAddInGamePopup(void);

// status bools.
extern bool	ClosingInGameOp;
extern bool	InGameOpUp;
extern bool isInGamePopupUp;
// ////////////////////////////////////////////////////////////////////////////
// defines

// the screen itself.
#define INTINGAMEOP				10500
#define INTINGAMEPOPUP			77335

// position info for window.

// game options

// initial options
#define INTINGAMEOP_W			150
#define INTINGAMEOP_H			124
#define INTINGAMEOP_HS			88

#define INTINGAMEOP_X			((320-(INTINGAMEOP_W/2))+D_W)
#define INTINGAMEOP_Y			((240-(INTINGAMEOP_H/2))+D_H)

#define INTINGAMEOP2_W			350
#define INTINGAMEOP2_H			120
#define INTINGAMEOP2_X			((320-(INTINGAMEOP2_W/2))+D_W)
#define INTINGAMEOP2_Y			((240-(INTINGAMEOP2_H/2))+D_H)

// quit confirmation.
#define INTINGAMEOP3_W			150
#define INTINGAMEOP3_H			65
#define INTINGAMEOP3_X			((320-(INTINGAMEOP3_W/2))+D_W)
#define INTINGAMEOP3_Y			((240-(INTINGAMEOP3_H/2))+D_H)

#define PAUSEMESSAGE_YOFFSET (0)

// button sizes.
#define	INTINGAMEOP_OP_W		(INTINGAMEOP_W-10)
#define	INTINGAMEOP_SW_W		(INTINGAMEOP2_W - 15)
#define	INTINGAMEOP_OP_H		10

enum
{
	INTINGAMEOP_QUIT = INTINGAMEOP + 1,
	INTINGAMEOP_QUIT_CONFIRM,               ///< The all important quit button
	INTINGAMEOP_RESUME,
	INTINGAMEOP_LOAD_MISSION,
	INTINGAMEOP_LOAD_SKIRMISH,
	INTINGAMEOP_SAVE_MISSION,
	INTINGAMEOP_SAVE_SKIRMISH,
	INTINGAMEOP_OPTIONS,
	INTINGAMEOP_FXVOL,
	INTINGAMEOP_FXVOL_S,
	INTINGAMEOP_3DFXVOL,
	INTINGAMEOP_3DFXVOL_S,
	INTINGAMEOP_CDVOL,
	INTINGAMEOP_CDVOL_S,
	INTINGAMEOP_CONTROL,
	INTINGAMEOP_CONTROL_BT,
	INTINGAMEOP_VIBRATION,
	INTINGAMEOP_VIBRATION_BT,
	INTINGAMEOP_PAUSELABEL,                 ///< The paused message
	INTINGAMEOP_SCREENSHAKE,
	INTINGAMEOP_SCREENSHAKE_BT,
	INTINGAMEOP_CENTRESCREEN,
	INTINGAMEOP_REPLAY,
	INTINGAMEOP_CURSOR_S,
	INTINGAMEOP_SUBTITLES,
	INTINGAMEOP_SUBTITLES_BT,
	INTINGAMEOP_TUI_TARGET_ORIGIN_SW,		///< Switch
	INTINGAMEOP_POPUP_QUIT,
	INTINGAMEOP_POPUP_MSG1,
	INTINGAMEOP_POPUP_MSG2,
	INTINGAMEOP_POPUP_MSG3
};



// positions within option boxes.
#define INTINGAMEOP_1_X		5
#define INTINGAMEOP_2_X		10
#define INTINGAMEOP_MID		160
#define	INTINGAMEOP_1_Y		20
#define	INTINGAMEOP_2_Y		40
#define	INTINGAMEOP_3_Y		60
#define	INTINGAMEOP_4_Y		80
#define	INTINGAMEOP_5_Y		100
#define	INTINGAMEOP_6_Y		120

#define OPALIGN		(WBUT_PLAIN | WBUT_TXTCENTRE)

#endif // __INCLUDED_SRC_INGAMEOP_H__
