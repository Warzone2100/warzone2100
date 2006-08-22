/*
 * Message.c
 *
 * Functions for the messages shown in the Intelligence Map
 *
 */
#include <stdio.h>

#include "lib/framework/frame.h"
#include "message.h"
#include "stats.h"
#include "text.h"
#include "console.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "hci.h"
#include "lib/ivis_common/piedef.h"
#include "objmem.h"
#include "map.h"

#include "multiplay.h"

/* Allocation sizes for the message heaps */
#define MESSAGE_INIT		20
#define MESSAGE_EXT			5
#define VIEWDATA_INIT		5		// was 2 ... but that wasn't enough
#define VIEWDATA_EXT		1

/* Allocation sizes for the proximity display heaps - this should coincide with
the number of Proximity Messages for a mission*/
#define PROXDISP_INIT		10
#define PROXDISP_EXT		5

//max number of text strings or sequences for viewdata
#define MAX_DATA		4

//VIEWDATA			*asViewData;
//UDWORD			numViewData;

//array of pointers for the view data
VIEWDATA_LIST			*apsViewData;

/* The memory heaps for the messages and viewData*/
OBJ_HEAP		*psMsgHeap;
OBJ_HEAP		*psViewDataHeap;

/* The memory heap for the proximity displays */
OBJ_HEAP		*psProxDispHeap;

/* The id number for the next message allocated
 * Each message will have a unique id number irrespective of type
 */
UDWORD	msgID = 0;

static int currentNumProxDisplays;
/* The list of messages allocated */
MESSAGE		*apsMessages[MAX_PLAYERS];

/* The list of proximity displays allocated */
PROXIMITY_DISPLAY *apsProxDisp[MAX_PLAYERS];

/* The current tutorial message - there is only ever one at a time. They are displayed
when called by the script. They are not to be re-displayed*/
//MESSAGE		tutorialMessage;

/* The IMD to use for the proximity messages */
iIMDShape	*pProximityMsgIMD;

//function declarations
static void addProximityDisplay(MESSAGE *psMessage, BOOL proxPos, UDWORD player);
static void removeProxDisp(MESSAGE *psMessage, UDWORD player);
//static void checkMessages(VIEWDATA *psViewData);
static void checkMessages(MSG_VIEWDATA *psViewData);


/* Creating a new message
 * new is a pointer to a pointer to the new message
 * type is the type of the message
 */
// ajl modified for netgames
extern UDWORD selectedPlayer;

#define CREATE_MSG(heap, new, msgType) \
	if (HEAP_ALLOC(heap, ((void*) new))) \
	{ \
		(*(new))->type = msgType; \
		(*(new))->id = (msgID<<3)|selectedPlayer; \
		msgID++; \
	}


/* Add the message to the BOTTOM of the list
 * list is a pointer to the message list
 * Order is now CAMPAIGN, MISSION, RESEARCH/PROXIMITY
 */
#define ADD_MSG(list, msg, player) \
	ASSERT((PTRVALID((msg), sizeof(MESSAGE)), \
		"addMessage: Invalid message pointer")); \
	if (list[player] == NULL) \
	{ \
		list[player] = msg; \
	} \
	else \
	{ \
		MESSAGE *psCurr, *psPrev; \
        switch (msg->type) \
        { \
            case MSG_CAMPAIGN: \
                /*add to bottom of the list*/ \
    		    for(psCurr = list[player]; psCurr->psNext != NULL; \
	    		    psCurr = psCurr->psNext) \
		        { \
		        } \
		        psCurr->psNext = msg; \
                msg->psNext = NULL; \
                break; \
            case MSG_MISSION: \
                /*add it before the first campaign message */ \
    		    for(psCurr = list[player]; psCurr->psNext != NULL AND psCurr->type == MSG_CAMPAIGN; \
	    		    psCurr = psCurr->psNext) \
		        { \
                    psPrev = psCurr; \
		        } \
                psPrev->psNext = msg; \
                msg->psNext = psCurr; \
                break; \
            case MSG_RESEARCH: \
            case MSG_PROXIMITY: \
                /*add it before the first mission message */ \
    		    for(psCurr = list[player]; psCurr->psNext != NULL AND psCurr->type == MSG_MISSION; \
	    		    psCurr = psCurr->psNext) \
		        { \
                   psPrev = psCurr; \
		        } \
		        psPrev->psNext = msg; \
                msg->psNext = psCurr; \
                break; \
        } \
	}

void add_msg(MESSAGE *list[MAX_PLAYERS], MESSAGE *msg, UDWORD player)
{
	ASSERT((PTRVALID((msg), sizeof(MESSAGE)),
		"addMessage: Invalid message pointer"));
	if (list[player] == NULL)
	{
		list[player] = msg;
        msg->psNext = NULL;
	}
	else
	{
		MESSAGE *psCurr, *psPrev;

        psCurr = psPrev = NULL;
        switch (msg->type)
        {
            case MSG_CAMPAIGN:
                /*add it before the first mission/research/prox message */
    		    for(psCurr = list[player]; psCurr != NULL; psCurr = psCurr->psNext)
		        {
                    if (psCurr->type == MSG_MISSION OR
                        psCurr->type == MSG_RESEARCH OR
                        psCurr->type == MSG_PROXIMITY)
                    {
                        break;
                    }
                    psPrev = psCurr;
		        }
                if (psPrev)
                {
                    psPrev->psNext = msg;
                    msg->psNext = psCurr;
                }
                else
                {
                    //must be top of list
                    psPrev = list[player];
                    list[player] = msg;
                    msg->psNext = psPrev;
                }
                break;
            case MSG_MISSION:
                /*add it before the first research/prox message */
    		    for(psCurr = list[player]; psCurr != NULL; psCurr = psCurr->psNext)
		        {
                    if (psCurr->type == MSG_RESEARCH OR
                        psCurr->type == MSG_PROXIMITY)
                    {
                        break;
                    }
                    psPrev = psCurr;
		        }
                if (psPrev)
                {
                    psPrev->psNext = msg;
                    msg->psNext = psCurr;
                }
                else
                {
                    //must be top of list
                    psPrev = list[player];
                    list[player] = msg;
                    msg->psNext = psPrev;
                }
                break;
            case MSG_RESEARCH:
            case MSG_PROXIMITY:
                /*add it to the bottom of the list */
    		    for(psCurr = list[player]; psCurr->psNext != NULL;
                psCurr = psCurr->psNext)
		        {
		        }
		        psCurr->psNext = msg;
                msg->psNext = NULL;
                break;
	default:
		break;
        }
	}
}



/* Remove a message from the list
 * list is a pointer to the message list
 * del is a pointer to the message to remove
*/
#define REMOVEMSG(list, heap, del, player) \
	ASSERT((PTRVALID(del, sizeof(MESSAGE)), \
		"removeMessage: Invalid message pointer")); \
	if (list[player] == del) \
	{ \
		list[player] = list[player]->psNext; \
		HEAP_FREE(heap, del); \
	} \
	else \
	{ \
		MESSAGE *psPrev = NULL, *psCurr; \
		for(psCurr = list[player]; (psCurr != del) && (psCurr != NULL); \
			psCurr = psCurr->psNext) \
		{ \
			psPrev = psCurr; \
		} \
		ASSERT((psCurr != NULL, \
			"removeMessage: message not found")); \
		if (psCurr != NULL) \
		{ \
			psPrev->psNext = psCurr->psNext; \
			HEAP_FREE(heap, del); \
		} \
	}

#define RELEASEALLMSG(list, heap) \
	{ \
		UDWORD	i; \
		MESSAGE	*psCurr, *psNext; \
		for(i=0; i<MAX_PLAYERS; i++) \
		{ \
			for(psCurr = list[i]; psCurr != NULL; psCurr = psNext) \
			{ \
		 		psNext = psCurr->psNext; \
				HEAP_FREE(heap, psCurr); \
			} \
			list[i] = NULL; \
		} \
	}


BOOL messageInitVars(void)
{
	int i;

	psMsgHeap = NULL;
	psProxDispHeap = NULL;
	psViewDataHeap = NULL;
	msgID = 0;
	currentNumProxDisplays = 0;

	for(i=0; i<MAX_PLAYERS; i++) {
		apsMessages[i] = NULL;
		apsProxDisp[i] = NULL;
	}

	pProximityMsgIMD = NULL;

	return TRUE;
}

//allocates the viewdata heap
BOOL initViewData(void)
{
	//initialise the viewData heap - needs to be done before the data is loaded
	if (!HEAP_CREATE(&psViewDataHeap, sizeof(VIEWDATA), VIEWDATA_INIT, VIEWDATA_EXT))
	{
		return FALSE;
	}
	return TRUE;
}

//destroys the viewdata heap
void viewDataHeapShutDown(void)
{
	HEAP_DESTROY(psViewDataHeap);
}

/*Add a message to the list */
 MESSAGE * addMessage(UDWORD msgType, BOOL proxPos, UDWORD player)
 {
	 MESSAGE *psMsgToAdd = NULL;

	 //first create a message of the required type
	 CREATE_MSG(psMsgHeap, &psMsgToAdd, msgType);
	 if (!psMsgToAdd)
	 {
		 return NULL;
	 }
	 //then add to the players' list
	 //ADD_MSG(apsMessages, psMsgToAdd, player);
     add_msg(apsMessages, psMsgToAdd, player);

	 //initialise the message data
	 psMsgToAdd->player = player;
	 psMsgToAdd->pViewData = NULL;
	 //psMsgToAdd->frameNumber = 0;
	 psMsgToAdd->read = FALSE;

	 //add a proximity display
	 if (msgType == MSG_PROXIMITY)
	 {
		 addProximityDisplay(psMsgToAdd, proxPos, player);
	 }
//	 else
//	 {
//		 //make the reticule button flash as long as not prox msg or multiplayer game.
//#ifndef PSX
//		if (player == selectedPlayer && !bMultiPlayer)
//#else
//		if (player == selectedPlayer )
//#endif
//		{
//			flashReticuleButton(IDRET_INTEL_MAP);
//		}
//	 }

	 return psMsgToAdd;
}

/* adds a proximity display - holds varaibles that enable the message to be
 displayed in the Intelligence Screen*/
void addProximityDisplay(MESSAGE *psMessage, BOOL proxPos, UDWORD player)
{
	PROXIMITY_DISPLAY *psToAdd;

	//create the proximity display
	if (HEAP_ALLOC(psProxDispHeap, (void*) &psToAdd))
	{
		if (proxPos)
		{
			psToAdd->type = POS_PROXOBJ;
		}
		else
		{
			psToAdd->type = POS_PROXDATA;
		}
		psToAdd->psMessage = psMessage;
		psToAdd->screenX = 0;
		psToAdd->screenY = 0;
		psToAdd->screenR = 0;
		psToAdd->player = player;
		psToAdd->timeLastDrawn = 0;
		psToAdd->frameNumber = 0;
		psToAdd->selected = FALSE;
		psToAdd->strobe = 0;
	}

	//now add it to the top of the list
	psToAdd->psNext = apsProxDisp[player];
	apsProxDisp[player] = psToAdd;

	//add a button to the interface
	intAddProximityButton(psToAdd, currentNumProxDisplays);
	currentNumProxDisplays++;
}

 /*remove a message */
void removeMessage(MESSAGE *psDel, UDWORD player)
{
	if (psDel->type == MSG_PROXIMITY)
	{
		removeProxDisp(psDel, player);
	}
	REMOVEMSG(apsMessages, psMsgHeap, psDel, player);
}

/* remove a proximity display */
void removeProxDisp(MESSAGE *psMessage, UDWORD player)
{
	PROXIMITY_DISPLAY		*psCurr, *psPrev;

	//find the proximity display for this message
	if (apsProxDisp[player]->psMessage == psMessage)
	{
		psCurr = apsProxDisp[player];
		apsProxDisp[player] = apsProxDisp[player]->psNext;
		intRemoveProximityButton(psCurr);
		HEAP_FREE(psProxDispHeap, psCurr);
	}
	else
	{
		psPrev = apsProxDisp[player];
		for(psCurr = apsProxDisp[player]; psCurr != NULL; psCurr =
			psCurr->psNext)
		{
			//compare the pointers
			if (psCurr->psMessage == psMessage)
			{
				psPrev->psNext = psCurr->psNext;
				intRemoveProximityButton(psCurr);
				HEAP_FREE(psProxDispHeap, psCurr);
				break;
			}
			psPrev = psCurr;
		}
	}
}

/* Remove all Messages*/
void freeMessages(void)
{
	releaseAllProxDisp();
	RELEASEALLMSG(apsMessages, psMsgHeap);
}

/* removes all the proximity displays */
void releaseAllProxDisp(void)
{
	UDWORD				player;
	PROXIMITY_DISPLAY	*psCurr, *psNext;

	for(player=0; player<MAX_PLAYERS; player++)
	{
		for(psCurr = apsProxDisp[player]; psCurr != NULL; psCurr = psNext)
		{
	 		psNext = psCurr->psNext;
			//remove message associated with this display
			removeMessage(psCurr->psMessage, player);
			//HEAP_FREE(psProxDispHeap, psCurr);
		}
		apsProxDisp[player] = NULL;
	}
	//re-initialise variables
	currentNumProxDisplays = 0;
}

/* Initialise the message heaps */
BOOL initMessage(void)
{
#ifdef VIDEO_TEST
	MESSAGE *psMessage;
#endif

	//set up the imd used for proximity messages
	pProximityMsgIMD = (iIMDShape *)resGetData("IMD", "arrow.pie");
	if (pProximityMsgIMD == NULL)
	{
		debug( LOG_ERROR, "Unable to load Proximity Message PIE" );
		abort();
		return FALSE;
	}

	//initialise the tutorial message - only used by scripts
	/*tutorialMessage.id = msgID;
	tutorialMessage.type = MSG_TUTORIAL;
	tutorialMessage.pViewData = NULL;
	tutorialMessage.read = FALSE;
	tutorialMessage.player = MAX_PLAYERS + 1;
	tutorialMessage.psNext = NULL;*/

	if (!HEAP_CREATE(&psMsgHeap, sizeof(MESSAGE), MESSAGE_INIT, MESSAGE_EXT))
	{
		return FALSE;
	}

	//initialise the proximity display heap
	if (!HEAP_CREATE(&psProxDispHeap, sizeof(PROXIMITY_DISPLAY), PROXDISP_INIT, PROXDISP_EXT))
	{
		return FALSE;
	}

	//JPS add message to get on screen video
#ifdef VIDEO_TEST
    //mission
	psMessage = addMessage(MSG_MISSION, FALSE, 0);
	if (psMessage)
	{
		psMessage->pViewData = (MSG_VIEWDATA *)getViewData("MB1A_MSG");
	}
    //campaign
	psMessage = addMessage(MSG_CAMPAIGN, FALSE, 0);
	if (psMessage)
	{
		psMessage->pViewData = (MSG_VIEWDATA *)getViewData("CMB1_MSG");
	}
#endif

	return TRUE;
}

BOOL addToViewDataList(VIEWDATA *psViewData, UBYTE numData)
{
	VIEWDATA_LIST		*psAdd;

	if (HEAP_ALLOC(psViewDataHeap, (void**)&psAdd))
	{
		psAdd->psViewData = psViewData;
		psAdd->numViewData = numData;
		//add to top of list
		psAdd->psNext = apsViewData;
		apsViewData = psAdd;
		return TRUE;
	}
	return FALSE;
}

/*load the view data for the messages from the file */
VIEWDATA *loadViewData(char *pViewMsgData, UDWORD bufferSize)
{
	UDWORD				i, id, dataInc, seqInc, numFrames, numData, count, count2;
	VIEWDATA			*psViewData, *pData;
	VIEW_RESEARCH		*psViewRes;
	VIEW_REPLAY			*psViewReplay;
	STRING				name[MAX_STR_LENGTH], imdName[MAX_NAME_SIZE],
						string[MAX_STR_LENGTH],
						imdName2[MAX_NAME_SIZE];
	STRING				audioName[MAX_STR_LENGTH];
	SDWORD				LocX,LocY,LocZ, proxType, audioID;
        int cnt;
	//keep the start so we release it at the end
	//pData = pViewMsgData;

	numData = numCR(pViewMsgData, bufferSize);
	if (numData > UBYTE_MAX)
	{
		debug( LOG_ERROR, "loadViewData: Didn't expect 256 viewData messages!" );
		abort();
		return NULL;
	}

	//allocate space for the data
	psViewData = (VIEWDATA *)MALLOC(numData * sizeof(VIEWDATA));
	if (psViewData == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for viewdata" );
		abort();
		return NULL;
	}

	//add to array list
	addToViewDataList(psViewData, (UBYTE)numData);

	//psViewData = asViewData;
	//save so can pass the value back
	pData = psViewData;

	for (i=0; i < numData; i++)
	{
		UDWORD numText;

		memset(psViewData, 0, sizeof(VIEWDATA));

		name[0] = '\0';

		//read the data into the storage - the data is delimeted using comma's
		sscanf(pViewMsgData,"%[^','],%d%n",name, &numText,&cnt);
                pViewMsgData += cnt;

		//check not loading up too many text strings
		if (numText > MAX_DATA)
		{
			debug( LOG_ERROR, "loadViewData: too many text strings for %s", psViewData->pName );
			abort();
			return NULL;
		}
		psViewData->numText=(UBYTE)numText;

		//allocate storage for the name
 		psViewData->pName = (STRING *)MALLOC((strlen(name))+1);
		if (psViewData->pName == NULL)
		{
			debug( LOG_ERROR, "ViewData Name - Out of memory" );
			abort();
			return NULL;
		}
		strcpy(psViewData->pName,name);

		//allocate space for text strings
		if (psViewData->numText)
		{
			psViewData->ppTextMsg = (STRING **) MALLOC(psViewData->numText *
				sizeof(STRING *));
		}

		//read in the data for the text strings
		for (dataInc = 0; dataInc < psViewData->numText; dataInc++)
		{
			name[0] = '\0';
			//sscanf(pViewMsgData,"%[^',']", &name);
			sscanf(pViewMsgData,",%[^',']%n",name,&cnt);
                        pViewMsgData += cnt;


			//get the ID for the string
			if (!strresGetIDNum(psStringRes, name, &id))
			{
				debug( LOG_ERROR, "Cannot find the view data string id %s ", name );
				abort();
				return NULL;
			}
			//get the string from the id
			psViewData->ppTextMsg[dataInc] = strresGetString(psStringRes, id);

		}

		//sscanf(pViewMsgData,"%d", &psViewData->type);
		sscanf(pViewMsgData,",%d%n", (int*) &psViewData->type,&cnt);
                pViewMsgData += cnt;

		//allocate data according to type
		switch (psViewData->type)
		{
		case VIEW_RES:
			psViewData->pData = (VIEW_RESEARCH *) MALLOC(sizeof(VIEW_RESEARCH));
			if (psViewData->pData == NULL)
			{
				debug( LOG_ERROR, "Unable to allocate memory" );
				abort();
				return NULL;
			}
			imdName[0] = '\0';
			imdName2[0] = '\0';
			string[0] = '\0';
			audioName[0] = '\0';
			//sscanf(pViewMsgData, "%[^','],%[^','],%[^','],%[^','],%d",
			//	&imdName, &imdName2, &string, &audioName, &numFrames);
			sscanf(pViewMsgData,",%[^','],%[^','],%[^','],%[^','],%d%n",
				imdName, imdName2, string, audioName, &numFrames,&cnt);
                        pViewMsgData += cnt;
			psViewRes = (VIEW_RESEARCH *)psViewData->pData;
			psViewRes->pIMD = (iIMDShape *) resGetData("IMD", imdName);
			if (psViewRes->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the PIE for message %s", name );
				abort();
				return NULL;
			}
			if (strcmp(imdName2, "0"))
			{
				psViewRes->pIMD2 = (iIMDShape *) resGetData("IMD", imdName2);
				if (psViewRes->pIMD2 == NULL)
				{
					debug( LOG_ERROR, "Cannot find the 2nd PIE for message %s", name );
					abort();
					return NULL;
				}
			}
			else
			{
				psViewRes->pIMD2 = NULL;
			}
			strcpy(psViewRes->sequenceName, string);
			//get the audio text string
			if (strcmp(audioName, "0"))
			{
				//allocate space
				psViewRes->pAudio = (STRING *) MALLOC(strlen(audioName) + 1);
				if (psViewRes->pAudio == NULL)
				{
					debug( LOG_ERROR, "loadViewData - Out of memory" );
					abort();
					return NULL;
				}
				strcpy(psViewRes->pAudio, audioName);
			}
			else
			{
				psViewRes->pAudio = NULL;
			}
			//this is for the PSX only
			psViewRes->numFrames = (UWORD)numFrames;
			break;
		case VIEW_RPL:
		case VIEW_RPLX:
			// This is now also used for the stream playing on the PSX
			// NOTE: on the psx the last entry (audioID) is used as the number of frames in the stream
			psViewData->pData = (VIEW_REPLAY *) MALLOC(sizeof(VIEW_REPLAY));
			if (psViewData->pData == NULL)
			{
				debug( LOG_ERROR, "Unable to allocate memory" );
				abort();
				return NULL;
			}
			psViewReplay = (VIEW_REPLAY *)psViewData->pData;

			//read in number of sequences for this message
			//sscanf(pViewMsgData, "%d", &psViewReplay->numSeq);
			sscanf(pViewMsgData, ",%d%n", &count,&cnt);
                        pViewMsgData += cnt;

			if (count > MAX_DATA)
			{
				debug( LOG_ERROR, "loadViewData: too many sequence for %s", psViewData->pName );
				abort();
				return NULL;
			}

			psViewReplay->numSeq = (UBYTE)count;

			//allocate space for the sequences
			psViewReplay->pSeqList = (SEQ_DISPLAY*) MALLOC(psViewReplay->numSeq *
				sizeof(SEQ_DISPLAY));

			//read in the data for the sequences
			for (dataInc = 0; dataInc < psViewReplay->numSeq; dataInc++)
			{
				name[0] = '\0';
				//load extradat for extended type only
				if (psViewData->type == VIEW_RPL)
				{
					sscanf(pViewMsgData, ",%[^','],%d%n", name, &count,&cnt);
                                        pViewMsgData += cnt;
					if (count > MAX_DATA)
					{
						debug( LOG_ERROR, "loadViewData: too many strings for %s", psViewData->pName );
						abort();
						return NULL;
					}
					psViewReplay->pSeqList[dataInc].numText = (UBYTE)count;
					//set the flag to default
					psViewReplay->pSeqList[dataInc].flag = 0;
				}
				else //extended type
				{
					sscanf(pViewMsgData, ",%[^','],%d,%d%n", name, &count,	&count2,&cnt);
                                        pViewMsgData += cnt;
					if (count > MAX_DATA)
					{
						debug( LOG_ERROR, "loadViewData: invalid video playback flag %s", psViewData->pName );
						abort();
						return NULL;
					}
					psViewReplay->pSeqList[dataInc].flag = (UBYTE)count;
					//check not loading up too many text strings
					if (count2 > MAX_DATA)
					{
						debug( LOG_ERROR, "loadViewData: too many text strings for seq for %s", psViewData->pName );
						abort();
						return NULL;
					}
					psViewReplay->pSeqList[dataInc].numText = (UBYTE)count2;
				}
				strcpy(psViewReplay->pSeqList[dataInc].sequenceName,name);

				//get the text strings for this sequence - if any
				//allocate space for text strings
				if (psViewReplay->pSeqList[dataInc].numText)
				{
					psViewReplay->pSeqList[dataInc].ppTextMsg = (STRING **) MALLOC(
						psViewReplay->pSeqList[dataInc].numText * sizeof(STRING *));
				}
				//read in the data for the text strings
				for (seqInc = 0; seqInc < psViewReplay->pSeqList[dataInc].numText;
					seqInc++)
				{
					name[0] = '\0';
					sscanf(pViewMsgData,",%[^',']%n", name,&cnt);
                                        pViewMsgData += cnt;
					//get the ID for the string
					if (!strresGetIDNum(psStringRes, name, &id))
					{
						debug( LOG_ERROR, "Cannot find the view data string id %s ", name );
						abort();
						return NULL;
					}

					//get the string from the id
					psViewReplay->pSeqList[dataInc].ppTextMsg[seqInc] = strresGetString(psStringRes, id);




				}
				//get the audio text string
				sscanf(pViewMsgData,",%[^','],%d%n", audioName, &count,&cnt);
                                pViewMsgData += cnt;

				ASSERT((count < UWORD_MAX, "loadViewData: numFrames too high for %s", name));

				psViewReplay->pSeqList[dataInc].numFrames = (UWORD)count;

				if (strcmp(audioName, "0"))
				{
					//allocate space
					psViewReplay->pSeqList[dataInc].pAudio = (STRING *) MALLOC(
						strlen(audioName) + 1);
					if (psViewReplay->pSeqList[dataInc].pAudio == NULL)
					{
						debug( LOG_ERROR, "loadViewData - Out of memory" );
						abort();
						return NULL;
					}
					strcpy(psViewReplay->pSeqList[dataInc].pAudio, audioName);
				}
				else
				{
					psViewReplay->pSeqList[dataInc].pAudio = NULL;
				}
			}
			psViewData->type = VIEW_RPL;//no longer need to know if it is extended type
			break;

		case VIEW_PROX:
			psViewData->pData = (VIEW_PROXIMITY *) MALLOC(sizeof(VIEW_PROXIMITY));
			if (psViewData->pData == NULL)
			{
				debug( LOG_ERROR, "Unable to allocate memory" );
				abort();
				return NULL;
			}


			audioName[0] = '\0';
			sscanf(pViewMsgData, ",%d,%d,%d,%[^','],%d%n", &LocX, &LocY, &LocZ,
				audioName,&proxType,&cnt);
                        pViewMsgData += cnt;

			//allocate audioID
			if ( strcmp( audioName, "0" ) == 0 )
			{
				audioID = NO_SOUND;
			}
			else
			{
				if ( audioID_GetIDFromStr( audioName, &audioID ) == FALSE )
				{
					debug( LOG_ERROR, "loadViewData: couldn't get ID %d for weapon sound %s", audioID, audioName );
					abort();
					return FALSE;
				}

				if ( ((audioID < 0) || (audioID >= ID_MAX_SOUND)) &&
					 (audioID != NO_SOUND) )
				{
					debug( LOG_ERROR, "Invalid Weapon Sound ID - %d for weapon %s", audioID, audioName );
					abort();
					return FALSE;
				}
			}


			((VIEW_PROXIMITY *)psViewData->pData)->audioID = audioID;

			if (LocX < 0)
			{
				ASSERT((FALSE,
					"loadViewData: Negative X coord for prox message - %s",name));
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->x = (UDWORD)LocX;
			if (LocY < 0)
			{
				ASSERT((FALSE,
					"loadViewData: Negative Y coord for prox message - %s",name));
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->y = (UDWORD)LocY;
			if (LocZ < 0)
			{
				ASSERT((FALSE,
					"loadViewData: Negative Z coord for prox message - %s",name));
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->z = (UDWORD)LocZ;

			if (proxType > PROX_TYPES)
			{
//printf("proxType %d > %d\n",proxType,PROX_TYPES);
				ASSERT((FALSE, "Invalid proximity message sub type - %s", name));
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->proxType = proxType;
			break;
		default:
			debug( LOG_ERROR, "Unknown ViewData type" );
			abort();
			return NULL;
		}
		//increment the pointer to the start of the next record
		pViewMsgData = strchr(pViewMsgData,'\n') + 1;
		//increment the list to the start of the next storage block
		psViewData++;
	}
//	FREE(pData);

	//return TRUE;
	return pData;
}

/*get the view data identified by the name */
VIEWDATA * getViewData(STRING *pName)
{
	//VIEWDATA		*psViewData;// = asViewData;
	VIEWDATA_LIST	*psList;
	//UDWORD			i;
	UBYTE			i;

	ASSERT((strlen(pName)< MAX_STR_SIZE,"getViewData: verbose message name"));

	for (psList = apsViewData; psList != NULL; psList = psList->psNext)
	{
		for (i = 0; i < psList->numViewData; i++)
		{
			//compare the strings
			if (!strcmp(psList->psViewData[i].pName, pName))
			{
				//return psViewData;
				return &psList->psViewData[i];
			}
		}
	}

	debug( LOG_ERROR, "Unable to find viewdata for message %s", pName );
	abort();
	return NULL;
}

/* Release the message heaps */
BOOL messageShutdown(void)
{
	freeMessages();
	HEAP_DESTROY(psMsgHeap);
	HEAP_DESTROY(psProxDispHeap);

	return TRUE;
}

/* Release the viewdata memory */
void viewDataShutDown(VIEWDATA *psViewData)
{
	VIEWDATA_LIST	*psList, *psPrev;
	//VIEWDATA		*psData;// = asViewData;
	//UDWORD		inc, numData;
	UDWORD			seqInc;
	VIEW_REPLAY		*psViewReplay;
	VIEW_RESEARCH	*psViewRes;
	UBYTE			i;

	psPrev = apsViewData;

	for (psList = apsViewData; psList != NULL; psList = psList->psNext)
	{
		if (psList->psViewData == psViewData)
		{
			for (i = 0; i < psList->numViewData; i++)
			{
				psViewData = &psList->psViewData[i];

				//check for any messages using this viewdata
				checkMessages((MSG_VIEWDATA *)psViewData);

				FREE(psViewData->pName);
				//free the space allocated for the text messages
				if (psViewData->numText)
				{
					FREE(psViewData->ppTextMsg);
				}

				//free the space allocated for multiple sequences
				if (psViewData->type == VIEW_RPL)
				{
					psViewReplay = (VIEW_REPLAY *)psViewData->pData;
					if (psViewReplay->numSeq)
					{
						for (seqInc = 0; seqInc < psViewReplay->numSeq; seqInc++)
						{
							//free the space allocated for the text messages
							if (psViewReplay->pSeqList[seqInc].numText)
							{
								FREE(psViewReplay->pSeqList[seqInc].ppTextMsg);
							}
							if (psViewReplay->pSeqList[seqInc].pAudio)
							{
								FREE(psViewReplay->pSeqList[seqInc].pAudio);
							}
						}
						FREE(psViewReplay->pSeqList);
					}
				}
				else if (psViewData->type == VIEW_RES)
				{
					psViewRes = (VIEW_RESEARCH *)psViewData->pData;
					if (psViewRes->pAudio)
					{
						FREE(psViewRes->pAudio);
					}
				}
				FREE(psViewData->pData);
			}
			FREE(psList->psViewData);
			//remove viewData list from the heap
			if (psList == apsViewData)
			{
				apsViewData = psList->psNext;
				HEAP_FREE(psViewDataHeap, psList);
			}
			else
			{
				psPrev->psNext = psList->psNext;
				HEAP_FREE(psViewDataHeap, psList);
			}
			break;
		}
	}
	psPrev = psList;
}

/* Looks through the players list of messages to find one with the same viewData
pointer and which is the same type of message - used in scriptFuncs */
MESSAGE * findMessage(MSG_VIEWDATA *pViewData, MESSAGE_TYPE type, UDWORD player)
{
	MESSAGE					*psCurr;

	for (psCurr = apsMessages[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->type == type AND psCurr->pViewData == pViewData)
		{
			return psCurr;
		}
	}

	//not found the message so return NULL
	return NULL;
}

/* 'displays' a proximity display*/
void displayProximityMessage(PROXIMITY_DISPLAY *psProxDisp)
{
	FEATURE			*psFeature;
	VIEWDATA		*psViewData;
	VIEW_PROXIMITY	*psViewProx;

	if (psProxDisp->type == POS_PROXDATA)
	{
		psViewData = (VIEWDATA *) psProxDisp->psMessage->pViewData;

		//display text - if any
		if (psViewData->ppTextMsg)
		{
			addConsoleMessage( psViewData->ppTextMsg[0], DEFAULT_JUSTIFY );
		}

		//play message - if any
		psViewProx = (VIEW_PROXIMITY *) psViewData->pData;
		if ( psViewProx->audioID != NO_AUDIO_MSG )
		{
			audio_QueueTrackPos( psViewProx->audioID, psViewProx->x,
									psViewProx->y, psViewProx->z );
		}
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		ASSERT(( ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->type ==
			OBJ_FEATURE, "displayProximityMessage: invalid feature" ));

		psFeature = (FEATURE *)psProxDisp->psMessage->pViewData;
		if (psFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			//play default audio message for oil resource
			audio_QueueTrackPos( ID_SOUND_RESOURCE_HERE, psFeature->x,
								 psFeature->y, psFeature->z );
		}
		else if (psFeature->psStats->subType == FEAT_GEN_ARTE)
		{
			//play default audio message for artefact
			audio_QueueTrackPos( ID_SOUND_ARTIFACT, psFeature->x,
								 psFeature->y, psFeature->z );
		}
	}

	//set the read flag
	psProxDisp->psMessage->read = TRUE;

}

/*void storeProximityScreenCoords(MESSAGE *psMessage, SDWORD x, SDWORD y)
{
	PROXIMITY_DISPLAY		*psProxDisp = NULL;

	psProxDisp = getProximityDisplay(psMessage);
	if (psProxDisp)
	{
		psProxDisp->screenX = x;
		psProxDisp->screenY = y;
	}
	else
	{
		ASSERT((FALSE, "Unable to find proximity display"));
	}
}*/

PROXIMITY_DISPLAY * getProximityDisplay(MESSAGE *psMessage)
{
	PROXIMITY_DISPLAY	*psCurr;

	if (apsProxDisp[psMessage->player]->psMessage == psMessage)
	{
		return apsProxDisp[psMessage->player];
	}
	else
	{
		for(psCurr = apsProxDisp[psMessage->player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if (psCurr->psMessage == psMessage)
			{
				return psCurr;
			}
		}
	}
	return NULL;
}

//check for any messages using this viewdata and remove them
//void checkMessages(VIEWDATA *psViewData)
void checkMessages(MSG_VIEWDATA *psViewData)
{
	MESSAGE			*psCurr, *psNext;
	UDWORD			i;

	for (i=0; i < MAX_PLAYERS; i++)
	{
		for (psCurr = apsMessages[i]; psCurr != NULL; psCurr = psNext)
		{
			psNext = psCurr->psNext;
			if (psCurr->pViewData == psViewData)
			{
				removeMessage(psCurr, i);
			}
		}
	}
}

//add proximity messages for all untapped VISIBLE oil resources
void addOilResourceProximities(void)
{
    FEATURE     *psFeat;
    MESSAGE     *psMessage;

    //look thru the features to find oil resources
    for (psFeat = apsFeatureLists[0]; psFeat != NULL; psFeat = psFeat->psNext)
    {
        if (psFeat->psStats->subType == FEAT_OIL_RESOURCE)
        {
            //check to see if the feature is visible to the selected player
            if (psFeat->visible[selectedPlayer])
            {
                //if there isn't an oil derrick built on it
				if(!TILE_HAS_STRUCTURE(mapTile(psFeat->x >> TILE_SHIFT,
					psFeat->y >> TILE_SHIFT)))
				{
                    //add a proximity message
					psMessage = addMessage(MSG_PROXIMITY, TRUE, selectedPlayer);
					if (psMessage)
					{
						psMessage->pViewData = (MSG_VIEWDATA *)psFeat;
					}
                }
            }
        }
    }
}






