/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
/** \file
 *  Defintions for messages.
 */

#ifndef __INCLUDED_MESSAGEDEF_H__
#define __INCLUDED_MESSAGEDEF_H__

#include <QtCore/QStringList>
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "positiondef.h"
#include "stringdef.h"

/// max number of text strings or sequences for VIEWDATA
static const unsigned int MAX_DATA = 4;

enum MESSAGE_TYPE
{
	MSG_RESEARCH,		// Research message
	MSG_CAMPAIGN,		// Campaign message
	MSG_MISSION,		// Mission Report messages
	MSG_PROXIMITY,		// Proximity message

	MSG_TYPES,
};

enum VIEW_TYPE
{
	VIEW_RES,			// research view
	VIEW_RPL,			// full screen view sequence - flic
	VIEW_PROX,			// proximity view - no view really!
	VIEW_RPLX,			// full screen view sequence - flic.	extended format

	VIEW_BEACON,			// Beacon message
	VIEW_SIZE
};

enum PROX_TYPE
{
	PROX_ENEMY,				//enemy proximity message
	PROX_RESOURCE,			//resource proximity message
	PROX_ARTEFACT,			//artefact proximity message

	PROX_TYPES,
};

// info required to view an object in Intelligence screen
struct VIEW_RESEARCH
{
	iIMDShape	*pIMD;
	iIMDShape	*pIMD2;				//allows base plates and turrets to be drawn as well
	char	sequenceName[MAX_STR_LENGTH];	//which windowed flic to display
	char	*pAudio;						/*name of audio track to play (for this seq)*/
};

struct SEQ_DISPLAY
{
	char		sequenceName[MAX_STR_LENGTH];

	UBYTE		flag;			//flag data to control video playback 1 = loop till audio finish
	QStringList     textMsg;	//Text messages - if any
	char		*pAudio;		/*name of audio track to play (for this seq)*/
};

//info required to view a flic in Intelligence Screen
struct VIEW_REPLAY
{
	UBYTE		numSeq;
	SEQ_DISPLAY *pSeqList;
};

// info required to view a proximity message
struct VIEW_PROXIMITY
{
	UDWORD		x;			//world coords for position of Proximity message
	UDWORD		y;
	UDWORD		z;
	PROX_TYPE	proxType;
	SDWORD		audioID;	/*ID of the audio track to play - if any */
	SDWORD		sender;		//user who sent this msg
	SDWORD		timeAdded;	//remember when was added, so can remove after certain period of time
};

struct VIEWDATA
{
	char		*pName;		//name ID of the message - used for loading in and identifying
	VIEW_TYPE	type;		//the type of view
	QStringList     textMsg;        //Text messages, if any
	void*		pData;		/*the data required to view - either a
							  VIEW_RESEARCH, VIEW_PROXIMITY or VIEW_REPLAY*/
};

typedef void* MSG_VIEWDATA;

enum MSG_DATA_TYPE
{
	MSG_DATA_DEFAULT,		// Message's pViewData has a BASE_OBJECT stored
	MSG_DATA_BEACON,		// Message's pViewData has beacon data stored
};

//base structure for each message
struct MESSAGE
{
	MESSAGE_TYPE	type;					//The type of message
	UDWORD			id;						//ID number of the message
	MSG_VIEWDATA	*pViewData;				//Pointer to view data - if any - should be some!
	bool			read;					//flag to indicate whether message has been read
	UDWORD			player;					//which player this message belongs to
	MSG_DATA_TYPE	dataType;				//stores actual type of data pViewData points to
											//only relevant for messages of type MSG_PROXIMITY

	MESSAGE *       psNext;                                 //pointer to the next in the list
};

//used to display the proximity messages
struct PROXIMITY_DISPLAY : public OBJECT_POSITION
{
	MESSAGE			*psMessage;				//message associated with this 'button'
	UDWORD			timeLastDrawn;			//stores the time the 'button' was last drawn for animation
	UDWORD			strobe;					//id of image last used
	UDWORD			buttonID;				//id of the button for the interface
	PROXIMITY_DISPLAY *     psNext;                         //pointer to the next in the list
};

#endif // __INCLUDED_MESSAGEDEF_H__
