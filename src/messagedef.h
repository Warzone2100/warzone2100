/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#include "lib/ivis_common/pietypes.h"
#include "lib/ivis_common/ivisdef.h"
#include "positiondef.h"
#include "stringdef.h"

/// max number of text strings or sequences for VIEWDATA
static const unsigned int MAX_DATA = 4;

typedef enum _message_type
{
	MSG_RESEARCH,		// Research message
	MSG_CAMPAIGN,		// Campaign message
	MSG_MISSION,		// Mission Report messages
	MSG_PROXIMITY,		// Proximity message

	MSG_TYPES,
} MESSAGE_TYPE;

typedef enum _view_type
{
	VIEW_RES,			// research view
	VIEW_RPL,			// full screen view sequence - flic
	VIEW_PROX,			// proximity view - no view really!
	VIEW_RPLX,			// full screen view sequence - flic.	extended format

	VIEW_BEACON,			// Beacon message
} VIEW_TYPE;

typedef enum _prox_type
{
	PROX_ENEMY,				//enemy proximity message
	PROX_RESOURCE,			//resource proximity message
	PROX_ARTEFACT,			//artefact proximity message

	PROX_TYPES,
} PROX_TYPE;

// info required to view an object in Intelligence screen
typedef struct _view_research
{
	iIMDShape	*pIMD;
	iIMDShape	*pIMD2;				//allows base plates and turrets to be drawn as well
	char	sequenceName[MAX_STR_LENGTH];	//which windowed flic to display
	char	*pAudio;						/*name of audio track to play (for this seq)*/
} VIEW_RESEARCH;

typedef struct _seq_display
{
	char		sequenceName[MAX_STR_LENGTH];

	UBYTE		flag;			//flag data to control video playback 1 = loop till audio finish
	UBYTE		numText;		//the number of textmessages associated with
								//this sequence
	const char**    ppTextMsg;	//Pointer to text messages - if any
	char		*pAudio;		/*name of audio track to play (for this seq)*/
} SEQ_DISPLAY;

//info required to view a flic in Intelligence Screen
typedef struct _view_replay
{
	UBYTE		numSeq;
	SEQ_DISPLAY *pSeqList;
	//char		**ppSeqName;
	//UBYTE		numText;	//the number of textmessages associated with this sequence
	//char		**ppTextMsg;	//Pointer to text messages - if any
} VIEW_REPLAY;

// info required to view a proximity message
typedef struct _view_proximity
{
	UDWORD		x;			//world coords for position of Proximity message
	UDWORD		y;
	UDWORD		z;
	PROX_TYPE	proxType;
	SDWORD		audioID;	/*ID of the audio track to play - if any */
	SDWORD		sender;		//user who sent this msg
	SDWORD		timeAdded;	//remember when was added, so can remove after certain period of time
} VIEW_PROXIMITY;

typedef struct _viewdata
{
	char		*pName;		//name ID of the message - used for loading in and identifying
	VIEW_TYPE	type;		//the type of view
	UBYTE		numText;	//the number of textmessages associated with this data
	const char**    ppTextMsg;	//Pointer to text messages - if any
	void*		pData;		/*the data required to view - either a
							  VIEW_RESEARCH, VIEW_PROXIMITY or VIEW_REPLAY*/
} VIEWDATA;

typedef void* MSG_VIEWDATA;

typedef enum _msg_data_type
{
	MSG_DATA_DEFAULT,		// Message's pViewData has a BASE_OBJECT stored
	MSG_DATA_BEACON,		// Message's pViewData has beacon data stored
} MSG_DATA_TYPE;

//base structure for each message
typedef struct _message
{
	MESSAGE_TYPE	type;					//The type of message
	UDWORD			id;						//ID number of the message
	MSG_VIEWDATA	*pViewData;				//Pointer to view data - if any - should be some!
	BOOL			read;					//flag to indicate whether message has been read
	UDWORD			player;					//which player this message belongs to
	MSG_DATA_TYPE	dataType;				//stores actual type of data pViewData points to
											//only relevant for messages of type MSG_PROXIMITY

	struct _message *psNext;				//pointer to the next in the list
} MESSAGE;

//used to display the proximity messages
struct PROXIMITY_DISPLAY : public OBJECT_POSITION
{
	MESSAGE			*psMessage;				//message associated with this 'button'
	UDWORD			timeLastDrawn;			//stores the time the 'button' was last drawn for animation
	UDWORD			strobe;					//id of image last used
	UDWORD			buttonID;				//id of the button for the interface
	PROXIMITY_DISPLAY *     psNext;                         //pointer to the next in the list
};

//used to display the text messages in 3D view of the Intel display
typedef struct _text_display
{
	UDWORD			totalFrames;			//number of frames for whole message to be displayed
	UDWORD			startTime;				//time started text display
	UDWORD			font;					//id of which font to use
	UWORD			fontColour;				//colour number
	char			text[MAX_STR_LENGTH-1];	//storage to hold the currently displayed text
} TEXT_DISPLAY;

typedef struct _viewData_list
{
	VIEWDATA				*psViewData;	//array of data
	UBYTE					numViewData;	//number in array
	struct _viewData_list	*psNext;		//next array of data
} VIEWDATA_LIST;

#endif // __INCLUDED_MESSAGEDEF_H__
