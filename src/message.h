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
 *  Functions for the messages shown in the Intelligence Map View
 */

#ifndef __INCLUDED_SRC_MESSAGE_H__
#define __INCLUDED_SRC_MESSAGE_H__

#include "structure.h"
#include "messagedef.h"

#define NO_AUDIO_MSG		-1

/** The lists of messages allocated. */
extern MESSAGE		*apsMessages[MAX_PLAYERS];

/** The IMD to use for the proximity messages. */
extern iIMDShape	*pProximityMsgIMD;

/** The list of proximity displays allocated. */
extern PROXIMITY_DISPLAY *apsProxDisp[MAX_PLAYERS];

/** Allocates the viewdata heap. */
extern BOOL initViewData(void);

/** Initialise the message heaps. */
extern BOOL initMessage(void);

/** Release the message heaps. */
extern BOOL messageShutdown(void);

/** Add a messgae to the list. */
extern MESSAGE * addMessage(MESSAGE_TYPE msgType, BOOL proxPos, UDWORD player);

/** Remove a message. */
extern void removeMessage(MESSAGE *psDel, UDWORD player);

/** Remove all Messages. */
extern void freeMessages(void);

/** Removes all the proximity displays. */
extern void releaseAllProxDisp(void);

/** Load the view data for the messages from the file exported from the world editor. */
extern VIEWDATA* loadViewData(const char *pViewMsgData, UDWORD bufferSize);

/** Get the view data that contains the text message pointer passed in. */
extern VIEWDATA* getViewData(const char *pTextMsg);

/** Release the viewdata memory. */
extern void viewDataShutDown(VIEWDATA *psViewData);

extern PROXIMITY_DISPLAY * getProximityDisplay(MESSAGE *psMessage);

/** Looks through the players list of messages to find one with the same viewData
  * pointer and which is the same type of message - used in scriptFuncs. */
extern MESSAGE* findMessage(MSG_VIEWDATA *pViewdata, MESSAGE_TYPE type, UDWORD player);

/** 'Displays' a proximity display. */
extern void displayProximityMessage(PROXIMITY_DISPLAY *psProxDisp);

/** Add proximity messages for all untapped VISIBLE oil resources. */
extern void addOilResourceProximities(void);

#endif // __INCLUDED_SRC_MESSAGE_H__
