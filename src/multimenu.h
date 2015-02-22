/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
 *  Definition for in game,multiplayer, interface.
 */

#ifndef __INCLUDED_SRC_MULTIMENU__
#define __INCLUDED_SRC_MULTIMENU__

#include "lib/widget/widgbase.h"
#include "stringdef.h"

// requester
void addMultiRequest(const char* searchDir, const char* fileExtension, UDWORD id,UBYTE mapCam, UBYTE numPlayers, std::string const &searchString = std::string());
extern bool		multiRequestUp;
extern W_SCREEN *psRScreen;			// requester stuff.
bool runMultiRequester(UDWORD id, UDWORD *mode, QString *chosen, LEVEL_DATASET **chosenValue, bool *isHoverPreview);
void displayRequestOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

// multimenu
extern void		intProcessMultiMenu		(UDWORD id);
extern bool		intRunMultiMenu			(void);
extern bool		intCloseMultiMenu		(void);
extern void		intCloseMultiMenuNoAnim	(void);
extern bool		intAddMultiMenu			(void);

extern bool		MultiMenuUp;

extern UDWORD		current_numplayers;
extern UDWORD		current_tech;

#define MULTIMENU				10600
#define MULTIMENU_FORM			MULTIMENU

#endif // __INCLUDED_SRC_MULTIMENU__
