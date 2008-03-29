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
/** @file
 *  In game options screen
 */

#ifndef __INCLUDED_SRC_INGAMEOP_H__
#define __INCLUDED_SRC_INGAMEOP_H__

// functions
extern BOOL intAddInGameOptions			(void);
extern BOOL intCloseInGameOptions		(BOOL bPutUpLoadSave, BOOL bResetMissionWidgets);
extern void intCloseInGameOptionsNoAnim	(BOOL bResetMissionWidgets);
extern BOOL intRunInGameOptions			(void);
extern void intProcessInGameOptions		(UDWORD);

// status bools.
extern BOOL	ClosingInGameOp;
extern BOOL	InGameOpUp;

// ////////////////////////////////////////////////////////////////////////////
// defines

// the screen itself.
#define INTINGAMEOP				10500

//the all important quit button
#define INTINGAMEOP_QUIT_CONFIRM	INTINGAMEOP+2

// position info for window.

// game options

// initial options
#define INTINGAMEOP_W			120
#define INTINGAMEOP_H			124
#define INTINGAMEOP_HS			88

#define INTINGAMEOP_X			((320-(INTINGAMEOP_W/2))+D_W)
#define INTINGAMEOP_Y			((240-(INTINGAMEOP_H/2))+D_H)

#define INTINGAMEOP2_W			290
#define INTINGAMEOP2_H			100
#define INTINGAMEOP2_X			((320-(INTINGAMEOP2_W/2))+D_W)
#define INTINGAMEOP2_Y			((240-(INTINGAMEOP2_H/2))+D_H)

// quit confirmation.
#define INTINGAMEOP3_W			120
#define INTINGAMEOP3_H			65
#define INTINGAMEOP3_X			((320-(INTINGAMEOP3_W/2))+D_W)
#define INTINGAMEOP3_Y			((240-(INTINGAMEOP3_H/2))+D_H)

#define PAUSEMESSAGE_YOFFSET (0)

#define INTINGAMEOP_PAUSEX			RET_X
#define INTINGAMEOP_PAUSEY			10
#define INTINGAMEOP_PAUSEWIDTH		60
#define INTINGAMEOP_PAUSEHEIGHT		20

// button sizes.
#define	INTINGAMEOP_OP_W		(INTINGAMEOP_W-10)
#define	INTINGAMEOP_OP_H		10

#define INTINGAMEOP_QUIT			(INTINGAMEOP+1)
//#define INTINGAMEOP_QUIT_CONFIRM	(INTINGAMEOP+2)
#define	INTINGAMEOP_RESUME			(INTINGAMEOP+3)	//resume

#define INTINGAMEOP_LOAD			(INTINGAMEOP+4)
#define INTINGAMEOP_SAVE			(INTINGAMEOP+5)
#define INTINGAMEOP_OPTIONS			(INTINGAMEOP+6)
#define INTINGAMEOP_FXVOL			(INTINGAMEOP+7)
#define INTINGAMEOP_FXVOL_S			(INTINGAMEOP+8)
#define INTINGAMEOP_3DFXVOL			(INTINGAMEOP+9)
#define INTINGAMEOP_3DFXVOL_S			(INTINGAMEOP+10)
#define INTINGAMEOP_CDVOL			(INTINGAMEOP+11)
#define INTINGAMEOP_CDVOL_S			(INTINGAMEOP+12)
#define INTINGAMEOP_CONTROL			(INTINGAMEOP+13)
#define INTINGAMEOP_CONTROL_BT		(INTINGAMEOP+14)
#define INTINGAMEOP_VIBRATION		(INTINGAMEOP+15)
#define INTINGAMEOP_VIBRATION_BT	(INTINGAMEOP+16)
#define	INTINGAMEOP_PAUSELABEL		(INTINGAMEOP+17)	//The paused message
#define INTINGAMEOP_SCREENSHAKE		(INTINGAMEOP+18)
#define INTINGAMEOP_SCREENSHAKE_BT	(INTINGAMEOP+19)
#define INTINGAMEOP_CENTRESCREEN	(INTINGAMEOP+20)
//#define INTINGAMEOP_REPLAY			(INTINGAMEOP+21)
#define INTINGAMEOP_CURSOR_S 		(INTINGAMEOP+22)
#define INTINGAMEOP_SUBTITLES		(INTINGAMEOP+23)
#define INTINGAMEOP_SUBTITLES_BT	(INTINGAMEOP+24)


// positions within option boxes.
#define INTINGAMEOP_1_X		5
#define INTINGAMEOP_MID		100
#define	INTINGAMEOP_1_Y		20
#define	INTINGAMEOP_2_Y		40
#define	INTINGAMEOP_3_Y		60
#define	INTINGAMEOP_4_Y		80
#define	INTINGAMEOP_5_Y		100
#define	INTINGAMEOP_6_Y		120

#define OPALIGN		(WBUT_PLAIN | WBUT_TXTCENTRE)

#endif // __INCLUDED_SRC_INGAMEOP_H__
