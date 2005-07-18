/*
 * messageDef.h
 *
 * Message structure definitions
 */
#ifndef _messageDef_h
#define _messageDef_h

#include "deliverance.h"
#include "pietypes.h"
	
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

	VIEW_TYPES,
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
	struct	iIMDShape	*pIMD;
	struct	iIMDShape	*pIMD2;				//allows base plates and turrets to be drawn as well
	STRING	sequenceName[MAX_STR_LENGTH];	//which windowed flic to display
	STRING	*pAudio;						/*name of audio track to play (for this seq)*/
	UWORD	numFrames;						/* On PSX if type is VIEW_RPL then 
											this is used as a number_of_frames_in_the_stream	
											count - NOT used on PC*/
} VIEW_RESEARCH;

typedef struct _seq_display
{
	STRING		sequenceName[MAX_STR_LENGTH];

	UBYTE		flag;			//flag data to control video playback 1 = loop till audio finish 
	UBYTE		numText;		//the number of textmessages associated with 
								//this sequence
	STRING		**ppTextMsg;	//Pointer to text messages - if any
	STRING		*pAudio;		/*name of audio track to play (for this seq)*/
	UWORD		numFrames;		/* On PSX if type is VIEW_RPL then 
								this is used as a number_of_frames_in_the_stream
								count - NOT used on PC*/
} SEQ_DISPLAY;

//info required to view a flic in Intelligence Screen
typedef struct _view_replay
{
	UBYTE		numSeq;
	SEQ_DISPLAY *pSeqList;
	//STRING		**ppSeqName;
	//UBYTE		numText;	//the number of textmessages associated with this sequence
	//STRING		**ppTextMsg;	//Pointer to text messages - if any
} VIEW_REPLAY;

// info required to view a proximity message
typedef struct _view_proximity
{
	UDWORD		x;			//world coords for position of Proximity message
	UDWORD		y;
	UDWORD		z;
	PROX_TYPE	proxType;
	SDWORD		audioID;	/*ID of the audio track to play - if any */
} VIEW_PROXIMITY;

typedef struct _viewdata
{
	STRING		*pName;		//name ID of the message - used for loading in and identifying
	VIEW_TYPE	type;		//the type of view
	UBYTE		numText;	//the number of textmessages associated with this data
	STRING		**ppTextMsg;	//Pointer to text messages - if any
	void*		pData;		/*the data required to view - either a 
							  VIEW_RESEARCH, VIEW_PROXIMITY or VIEW_REPLAY*/
} VIEWDATA;

typedef void* MSG_VIEWDATA;

//base structure for each message
typedef struct _message
{
	MESSAGE_TYPE	type;					//The type of message 
	UDWORD			id;						//ID number of the message
	//VIEWDATA		*pViewData;				//Pointer to view data - if any - should be some!
	MSG_VIEWDATA	*pViewData;				//Pointer to view data - if any - should be some!
	BOOL			read;					//flag to indicate whether message has been read
	UDWORD			player;					//which player this message belongs to

	struct _message *psNext;				//pointer to the next in the list
} MESSAGE;

//used to display the proximity messages
typedef struct _proximity_display
{
	POSITION_OBJ;
	MESSAGE			*psMessage;				//message associated with this 'button'
	UDWORD			radarX;					//Used to store the radar coords - if to be drawn
	UDWORD			radarY;
	UDWORD			timeLastDrawn;			//stores the time the 'button' was last drawn for animation
	UDWORD			strobe;					//id of image last used
	UDWORD			buttonID;				//id of the button for the interface
	struct _proximity_display	*psNext;	//pointer to the next in the list
} PROXIMITY_DISPLAY;

//used to display the text messages in 3D view of the Intel display
typedef struct _text_display
{
	UDWORD			totalFrames;			//number of frames for whole message to be displayed
	UDWORD			startTime;				//time started text display
	UDWORD			font;					//id of which font to use
	UWORD			fontColour;				//colour number
	STRING			text[MAX_STR_LENGTH-1];	//storage to hold the currently displayed text
} TEXT_DISPLAY;

typedef struct _viewData_list
{
	VIEWDATA				*psViewData;	//array of data
	UBYTE					numViewData;	//number in array
	struct _viewData_list	*psNext;		//next array of data
} VIEWDATA_LIST;

#endif	//messageDef.h
