/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
 *  Definitions for messages.
 */

#ifndef __INCLUDED_MESSAGEDEF_H__
#define __INCLUDED_MESSAGEDEF_H__

#include <vector>
#include "lib/framework/wzstring.h"
#include "positiondef.h"
#include "stringdef.h"

struct iIMDShape;
struct BASE_OBJECT;

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

struct VIEW_BASE
{
	virtual ~VIEW_BASE() = default; // will cause destructors to be called for subclasses
};

// info required to view an object in Intelligence screen
struct VIEW_RESEARCH : VIEW_BASE
{
	iIMDShape	*pIMD = nullptr;
	iIMDShape	*pIMD2 = nullptr;	// allows base plates and turrets to be drawn as well
	WzString		sequenceName;	// which windowed flic to display
	WzString		audio;			// name of audio track to play (for this seq)
};

struct SEQ_DISPLAY
{
	WzString				sequenceName;
	UBYTE					flag;		//flag data to control video playback 1 = loop till audio finish
	std::vector<WzString>	textMsg;	//Text messages - if any
	WzString				audio;		// name of audio track to play (for this seq)
};

//info required to view a flic in Intelligence Screen
struct VIEW_REPLAY : VIEW_BASE
{
	std::vector<SEQ_DISPLAY> seqList;
};

// info required to view a proximity message
struct VIEW_PROXIMITY : VIEW_BASE
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
	WzString				name;			//name ID of the message - used for loading in and identifying
	VIEW_TYPE				type;			//the type of view
	std::vector<WzString>	textMsg;        //Text messages, if any
	VIEW_BASE				*pData = nullptr; // the data required to view - either VIEW_RESEARCH, VIEW_PROXIMITY or VIEW_REPLAY
	WzString				fileName;       // file it came from, for piecemeal destruction (pretty lame reason)
};

enum MSG_DATA_TYPE
{
	MSG_DATA_DEFAULT,
	MSG_DATA_BEACON,
};

//base structure for each message
struct MESSAGE
{
	MESSAGE_TYPE    type;                                   // The type of message
	UDWORD          id;                                     // ID number of the message
	VIEWDATA        *pViewData = nullptr;                   // Pointer to view data - if any - should be some!
	BASE_OBJECT     *psObj = nullptr;
	bool            read = false;                           // flag to indicate whether message has been read
	UDWORD		player;                                 // which player this message belongs to
	MSG_DATA_TYPE	dataType;

	MESSAGE         *psNext = nullptr;                       // pointer to the next in the list
};

//used to display the proximity messages
struct PROXIMITY_DISPLAY : public OBJECT_POSITION
{
	MESSAGE            *psMessage = nullptr;   // message associated with this 'button'
	UDWORD             timeLastDrawn = 0;     // stores the time the 'button' was last drawn for animation
	UDWORD             strobe = 0;            // id of image last used
	UDWORD             buttonID = 0;          // id of the button for the interface
	PROXIMITY_DISPLAY  *psNext = nullptr;      // pointer to the next in the list
};

#endif // __INCLUDED_MESSAGEDEF_H__
