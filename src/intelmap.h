/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 *  Functions for the display of the Intelligence Map
 */

#ifndef __INCLUDED_SRC_INTELMAP_H__
#define __INCLUDED_SRC_INTELMAP_H__

#include "messagedef.h"

/* Intelligence Map screen IDs */
#define IDINTMAP_FORM			6000	//The intelligence map base form
#define IDINTMAP_MSGVIEW		6002	//The message 3D view for the intelligence screen

/*dimensions for PIE view section relative to IDINTMAP_MSGVIEW*/

#define	INTMAP_PIEWIDTH			238
#define INTMAP_PIEHEIGHT		169


// The current message being displayed
extern MESSAGE			*psCurrentMsg;

/* Add the Intelligence Map widgets to the widget screen */
//extern bool intAddIntelMap(bool playCurrent);
extern bool intAddIntelMap(void);
/*Add the 3D world view for the current message */
extern bool intAddMessageView(MESSAGE *psMessage);
/* Remove the Message View from the Intelligence screen */
extern void intRemoveMessageView(bool animated);

/* Process return codes from the Intelligence Map */
extern void intProcessIntelMap(UDWORD id);
/* Process return code from the Message View for Tutorial Mode*/
//extern void intProcessMessageView(UDWORD id);

/* rotate the view so looking directly down if forward = true or
 back to previous view if forward = false */
//extern void intelMapView(bool forward);

/* Remove the Intelligence Map widgets from the screen */
extern void intRemoveIntelMap(void);

/* Remove the Intelligence Map widgets from the screen without animation*/
extern void intRemoveIntelMapNoAnim(void);

//initialise the text display stats for the current message
//extern void initTextDisplay(MESSAGE *psMessage, UDWORD fontID, UWORD fontColour);

/* scroll the text message from left to right - aka tickertape messages */
//extern void scrollMessage(char *pText, UDWORD startX, UDWORD endX, UDWORD y, UDWORD gap);

/*sets psCurrentMsg for the Intelligence screen*/
extern void setCurrentMsg(void);

/*sets which states need to be paused when the intelligence screen is up*/
extern void setIntelligencePauseState(void);
/*resets the pause states */
extern void resetIntelligencePauseState(void);

// tell the intelligence screen to play this message immediately
extern void displayImmediateMessage(MESSAGE *psMessage);

// return whether a message is immediate
extern bool messageIsImmediate(void);
/*sets the flag*/
extern void setMessageImmediate(bool state);

#endif	// __INCLUDED_SRC_INTELMAP_H__
