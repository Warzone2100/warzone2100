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
 * MultiMenu.h
 *
 * Definition for in game,multiplayer, interface.
 */
// 
#ifndef __INCLUDED_MULTIMENU__
#define __INCLUDED_MULTIMENU__

// requester
extern VOID		addMultiRequest(STRING *ToFind, UDWORD id,UBYTE mapCam, UBYTE numPlayers);
extern BOOL		multiRequestUp;
extern W_SCREEN *psRScreen;			// requester stuff.
extern BOOL		runMultiRequester(UDWORD id,UDWORD *contextmode, STRING *chosen,UDWORD *chosenValue);
extern void		displayRequestOption(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

// multimenu
extern VOID		intProcessMultiMenu		(UDWORD id);
extern BOOL		intRunMultiMenu			(VOID);
extern BOOL		intCloseMultiMenu		(VOID);
extern VOID		intCloseMultiMenuNoAnim	(VOID);
extern BOOL		intAddMultiMenu			(VOID);

extern BOOL		MultiMenuUp;
extern BOOL		ClosingMultiMenu;

//extern VOID		intDisplayMiniMultiMenu		(VOID);

#define MULTIMENU			10600
#define MULTIMENU_FORM		MULTIMENU

#endif
