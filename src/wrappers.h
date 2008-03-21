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

#ifndef __INCLUDED_SRC_WRAPPERS_H__
#define __INCLUDED_SRC_WRAPPERS_H__

typedef enum {
	TITLECODE_CONTINUE,
	TITLECODE_STARTGAME,
	TITLECODE_QUITGAME,
	TITLECODE_SHOWINTRO,
	TITLECODE_SAVEGAMELOAD,
} TITLECODE;

//used to set the scriptWinLoseVideo variable
#define PLAY_NONE   0
#define PLAY_WIN    1
#define PLAY_LOSE   2


extern BOOL			frontendInitVars	    ( void );
extern TITLECODE	titleLoop			    ( void );

extern void			clearTitle	 		    ( void );
extern void			displayTitleScreen 	    ( void );

extern void			initLoadingScreen		( BOOL drawbdrop );
extern void			closeLoadingScreen	    ( void );
extern void			loadingScreenCallback   ( void );

extern void			startCreditsScreen	    ( void );

extern BOOL			displayGameOver		    ( BOOL success);
extern void			setPlayerHasLost	    ( BOOL val );
extern BOOL			testPlayerHasLost	    ( void );
extern BOOL			testPlayerHasWon  	    ( void );
extern void			setPlayerHasWon		    ( BOOL val );
extern void         setScriptWinLoseVideo   ( UBYTE val );
extern UBYTE        getScriptWinLoseVideo   ( void );


// PC version calls the loading bar code directly.
//#define LOADBARCALLBACK() loadingScreenCallback()
#define LOADBARCALLBACK()

#endif // __INCLUDED_SRC_WRAPPERS_H__
