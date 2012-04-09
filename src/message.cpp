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
/*
 * Message.c
 *
 * Functions for the messages shown in the Intelligence Map
 *
 */

#include <QtCore/QMap>
#include "lib/framework/frame.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "console.h"
#include "hci.h"
#include "stats.h"
#include "text.h"

static QMap<QString, VIEWDATA *> apsViewData;

/* The id number for the next message allocated
 * Each message will have a unique id number irrespective of type
 */
static UDWORD	msgID = 0;

static int currentNumProxDisplays;
/* The list of messages allocated */
MESSAGE		*apsMessages[MAX_PLAYERS];

/* The list of proximity displays allocated */
PROXIMITY_DISPLAY *apsProxDisp[MAX_PLAYERS];

/* The IMD to use for the proximity messages */
iIMDShape	*pProximityMsgIMD;


/* Creating a new message
 * new is a pointer to a pointer to the new message
 * type is the type of the message
 */
static inline MESSAGE* createMessage(MESSAGE_TYPE msgType, UDWORD player)
{
	MESSAGE *newMsg;

	ASSERT_OR_RETURN(NULL, player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN(NULL, msgType < MSG_TYPES, "Bad message");
	if (player >= MAX_PLAYERS || msgType >= MSG_TYPES)
	{
		return NULL;
	}

	// Allocate memory for the message, and on failure return a NULL pointer
	newMsg = (MESSAGE *)malloc(sizeof(MESSAGE));
	newMsg->type = msgType;
	newMsg->dataType = MSG_DATA_DEFAULT;
	newMsg->id = (msgID << 3) | selectedPlayer;
	newMsg->pViewData = NULL;
	newMsg->read = false;
	newMsg->player = player;
	msgID++;

	return newMsg;
}

/* Add the message to the BOTTOM of the list
 * list is a pointer to the message list
 * Order is now CAMPAIGN, MISSION, RESEARCH/PROXIMITY
 */
static inline void addMessageToList(MESSAGE *list[MAX_PLAYERS], MESSAGE *msg, UDWORD player)
{
	MESSAGE *psCurr = NULL, *psPrev = NULL;

	ASSERT_OR_RETURN( , msg != NULL, "Invalid message pointer");
	ASSERT_OR_RETURN( , player < MAX_PLAYERS, "Bad player");

	// If there is no message list, create one
	if (list[player] == NULL)
	{
		list[player] = msg;
		msg->psNext = NULL;

		return;
	}

	switch (msg->type)
		{
		case MSG_CAMPAIGN:
			/*add it before the first mission/research/prox message */
			for(psCurr = list[player]; psCurr != NULL; psCurr = psCurr->psNext)
			{
				if (psCurr->type == MSG_MISSION ||
					psCurr->type == MSG_RESEARCH ||
					psCurr->type == MSG_PROXIMITY)
					break;

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
				if (psCurr->type == MSG_RESEARCH ||
				    psCurr->type == MSG_PROXIMITY)
					break;

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

			// Iterate to the last item in the list
			for (psCurr = list[player]; psCurr->psNext != NULL; psCurr = psCurr->psNext) {}

			// Append the new message to the end of the list
			psCurr->psNext = msg;
			msg->psNext = NULL;

			break;
		default:
			debug(LOG_ERROR, "unknown message type");
			break;
	}
}



/* Remove a message from the list
 * list is a pointer to the message list
 * del is a pointer to the message to remove
*/
static inline void removeMessageFromList(MESSAGE *list[], MESSAGE *del, UDWORD player)
{
	MESSAGE *psPrev = NULL, *psCurr;

	ASSERT_OR_RETURN( , del != NULL, "Invalid message pointer");
	ASSERT_OR_RETURN( , player < MAX_PLAYERS, "Bad player");

	// If the message to remove is the first one in the list then mark the next one as the first
	if (list[player] == del)
	{
		list[player] = list[player]->psNext;
		free(del);
		return;
	}

	// Iterate through the list and find the item before the message to delete
	for(psCurr = list[player]; (psCurr != del) && (psCurr != NULL);	psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}

	ASSERT(psCurr != NULL, "message not found in list");

	if (psCurr != NULL)
	{
		// Modify the "next" pointer of the previous item to
		// point to the "next" item of the item to delete.
		psPrev->psNext = psCurr->psNext;
		free(del);
	}
}

static inline void releaseAllMessages(MESSAGE *list[])
{
	UDWORD	i;
	MESSAGE	*psCurr, *psNext;

	// Iterate through all players' message lists
	for(i=0; i < MAX_PLAYERS; i++)
	{
		// Iterate through all messages in list
		for(psCurr = list[i]; psCurr != NULL; psCurr = psNext)
		{
			psNext = psCurr->psNext;
			free(psCurr);
		}
		list[i] = NULL;
	}
}

bool messageInitVars(void)
{
	int i;

	msgID = 0;
	currentNumProxDisplays = 0;

	for(i=0; i<MAX_PLAYERS; i++) {
		apsMessages[i] = NULL;
		apsProxDisp[i] = NULL;
	}

	pProximityMsgIMD = NULL;

	return true;
}

//allocates the viewdata heap
bool initViewData(void)
{
	return true;
}

/* Adds a beacon message. A wrapper for addMessage() */
MESSAGE * addBeaconMessage(MESSAGE_TYPE msgType, bool proxPos, UDWORD player)
{
	MESSAGE* psBeaconMsgToAdd = addMessage(msgType, proxPos, player);

	ASSERT_OR_RETURN(NULL, psBeaconMsgToAdd, "createMessage failed");

	// remember we are storing beacon data in this message
	psBeaconMsgToAdd->dataType = MSG_DATA_BEACON;

	return psBeaconMsgToAdd;
}

/* adds a proximity display - holds variables that enable the message to be
 displayed in the Intelligence Screen*/
static void addProximityDisplay(MESSAGE *psMessage, bool proxPos, UDWORD player)
{
	PROXIMITY_DISPLAY *psToAdd;

	ASSERT_OR_RETURN( , player < MAX_PLAYERS, "Bad player");
	debug(LOG_MSG, "Added prox display for player %u (proxPos=%d)", player, (int)proxPos);

	//create the proximity display
	psToAdd = (PROXIMITY_DISPLAY*)malloc(sizeof(PROXIMITY_DISPLAY));

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
	psToAdd->selected = false;
	psToAdd->strobe = 0;

	//add a button to the interface
	if (intAddProximityButton(psToAdd, currentNumProxDisplays))
	{
		// Now add it to the top of the list. Be aware that this
		// check means that messages and proximity displays can
		// become out of sync - but this should never happen.
		psToAdd->psNext = apsProxDisp[player];
		apsProxDisp[player] = psToAdd;
		currentNumProxDisplays++;
	}
	else
	{
		free(psToAdd);	// clean up
	}
}

/*Add a message to the list */
MESSAGE * addMessage(MESSAGE_TYPE msgType, bool proxPos, UDWORD player)
{
	//first create a message of the required type
	MESSAGE* psMsgToAdd = createMessage(msgType, player);

	debug(LOG_MSG, "adding message for player %d, type is %d, proximity is %d", player, msgType, proxPos);

	ASSERT(psMsgToAdd, "createMessage failed");
	if (!psMsgToAdd)
	{
		return NULL;
	}
	//then add to the players' list
	addMessageToList(apsMessages, psMsgToAdd, player);

	//add a proximity display
	if (msgType == MSG_PROXIMITY)
	{
		addProximityDisplay(psMsgToAdd, proxPos, player);
	}

	return psMsgToAdd;
}

/* remove a proximity display */
static void removeProxDisp(MESSAGE *psMessage, UDWORD player)
{
	PROXIMITY_DISPLAY		*psCurr, *psPrev;

	ASSERT_OR_RETURN( , player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN( , psMessage != NULL, "Bad message");

	if (!apsProxDisp[player])
	{
		return;	// no corresponding proximity display
	}

	//find the proximity display for this message
	if (apsProxDisp[player]->psMessage == psMessage)
	{
		psCurr = apsProxDisp[player];

		apsProxDisp[player] = apsProxDisp[player]->psNext;
		intRemoveProximityButton(psCurr);
		free(psCurr);
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
				free(psCurr);
				break;
			}
			psPrev = psCurr;
		}
	}
}

/*remove a message */
void removeMessage(MESSAGE *psDel, UDWORD player)
{
	ASSERT_OR_RETURN( , player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN( , psDel != NULL, "Bad message");
	debug(LOG_MSG, "removing message for player %d", player);

	if (psDel->type == MSG_PROXIMITY)
	{
		removeProxDisp(psDel, player);
	}
	removeMessageFromList(apsMessages, psDel, player);
}

/* Remove all Messages*/
void freeMessages(void)
{
	releaseAllProxDisp();
	releaseAllMessages(apsMessages);
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
		}
		apsProxDisp[player] = NULL;
	}
	//re-initialise variables
	currentNumProxDisplays = 0;
}

/* Initialise the message heaps */
bool initMessage(void)
{
	//set up the imd used for proximity messages
	pProximityMsgIMD = (iIMDShape *)resGetData("IMD", "arrow.pie");
	if (pProximityMsgIMD == NULL)
	{
		ASSERT(false, "Unable to load Proximity Message PIE");
		return false;
	}

	return true;
}

/*load the view data for the messages from the file */
const char *loadViewData(const char *pViewMsgData, UDWORD bufferSize)
{
	UDWORD				i, dataInc, seqInc, dummy, numData, count, count2;
	VIEW_RESEARCH		*psViewRes;
	VIEW_REPLAY			*psViewReplay;
	char				name[MAX_STR_LENGTH], imdName[MAX_STR_LENGTH],
						string[MAX_STR_LENGTH],
						imdName2[MAX_STR_LENGTH];
	char				audioName[MAX_STR_LENGTH];
	SDWORD				LocX,LocY,LocZ, audioID;
	PROX_TYPE	proxType;
	int cnt;
	const char *filename = strdup(GetLastResourceFilename());

	numData = numCR(pViewMsgData, bufferSize);
	for (i=0; i < numData; i++)
	{
		VIEWDATA *psViewData = new VIEWDATA;
		UDWORD numText;
		int readint;

		psViewData->pData = NULL;
		psViewData->pName = NULL;
		psViewData->type = VIEW_SIZE;
		psViewData->fileName = filename;
		name[0] = '\0';

		//read the data into the storage - the data is delimeted using comma's
		sscanf(pViewMsgData,"%255[^,'\r\n],%d%n",name, &numText,&cnt);
		pViewMsgData += cnt;

		//check not loading up too many text strings
		ASSERT(numText <= MAX_DATA, "Too many text strings for %s", name);

		//allocate storage for the name
		psViewData->pName = strdup(name);
		debug(LOG_MSG, "Loaded %s", psViewData->pName);

		//read in the data for the text strings
		for (dataInc = 0; dataInc < numText; dataInc++)
		{
			name[0] = '\0';
			sscanf(pViewMsgData,",%255[^,'\r\n]%n",name,&cnt);
			pViewMsgData += cnt;

			// Get the string from the ID string
			const char *str = strresGetString(psStringRes, name);
			ASSERT(str, "Cannot find the view data string with id \"%s\"", name);
			QString qstr = QString::fromUtf8(str);
			psViewData->textMsg.push_back(qstr);
		}

		sscanf(pViewMsgData, ",%d%n", &readint, &cnt);
		psViewData->type = (VIEW_TYPE)readint;
		pViewMsgData += cnt;

		//allocate data according to type
		switch (psViewData->type)
		{
		case VIEW_RES:
			psViewData->pData = (VIEW_RESEARCH *) malloc(sizeof(VIEW_RESEARCH));
			imdName[0] = '\0';
			imdName2[0] = '\0';
			string[0] = '\0';
			audioName[0] = '\0';
			sscanf(pViewMsgData,",%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n],%d%n",
				imdName, imdName2, string, audioName, &dummy, &cnt);
			pViewMsgData += cnt;
			psViewRes = (VIEW_RESEARCH *)psViewData->pData;
			psViewRes->pIMD = (iIMDShape *) resGetData("IMD", imdName);
			if (psViewRes->pIMD == NULL)
			{
				ASSERT(false, "Cannot find the PIE for message %s", name);
				delete psViewData;
				return NULL;
			}
			if (strcmp(imdName2, "0"))
			{
				psViewRes->pIMD2 = (iIMDShape *) resGetData("IMD", imdName2);
				if (psViewRes->pIMD2 == NULL)
				{
					ASSERT(false, "Cannot find the 2nd PIE for message %s", name);
					delete psViewData;
					return NULL;
				}
			}
			else
			{
				psViewRes->pIMD2 = NULL;
			}
			sstrcpy(psViewRes->sequenceName, string);
			//get the audio text string
			if (strcmp(audioName, "0"))
			{
				//allocate space
				psViewRes->pAudio = strdup(audioName);
			}
			else
			{
				psViewRes->pAudio = NULL;
			}
			break;
		case VIEW_RPL:
		case VIEW_RPLX:
			// This is now also used for the stream playing on the PSX
			// NOTE: on the psx the last entry (audioID) is used as the number of frames in the stream
			psViewData->pData = (VIEW_REPLAY *) malloc(sizeof(VIEW_REPLAY));
			psViewReplay = (VIEW_REPLAY *)psViewData->pData;

			//read in number of sequences for this message
			sscanf(pViewMsgData, ",%d%n", &count,&cnt);
			pViewMsgData += cnt;

			ASSERT(count <= MAX_DATA, "Too many text strings for %s", psViewData->pName);

			psViewReplay->numSeq = (UBYTE)count;

			//allocate space for the sequences
			psViewReplay->pSeqList = new SEQ_DISPLAY[psViewReplay->numSeq];

			//read in the data for the sequences
			for (dataInc = 0; dataInc < psViewReplay->numSeq; dataInc++)
			{
				int numText = 0;

				name[0] = '\0';
				//load extradat for extended type only
				if (psViewData->type == VIEW_RPL)
				{
					sscanf(pViewMsgData, ",%255[^,'\r\n],%d%n", name, &count,&cnt);
					pViewMsgData += cnt;
					ASSERT(count <= MAX_DATA, "Too many text strings for %s", psViewData->pName);
					//set the flag to default
					psViewReplay->pSeqList[dataInc].flag = 0;
					numText = count;
				}
				else //extended type
				{
					sscanf(pViewMsgData, ",%255[^,'\r\n],%d,%d%n", name, &count,	&count2,&cnt);
					pViewMsgData += cnt;
					ASSERT(count <= MAX_DATA, "Invalid flag for %s", psViewData->pName);
					psViewReplay->pSeqList[dataInc].flag = (UBYTE)count;
					//check not loading up too many text strings
					ASSERT(count2 <= MAX_DATA, "Too many text strings for %s", psViewData->pName);
					numText = count2;
				}
				strcpy(psViewReplay->pSeqList[dataInc].sequenceName,name);

				//get the text strings for this sequence - if any
				for (seqInc = 0; seqInc < numText; seqInc++)
				{
					name[0] = '\0';
					sscanf(pViewMsgData,",%255[^,'\r\n]%n", name,&cnt);
					pViewMsgData += cnt;

					// Get the string from the ID string
					const char *str = strresGetString(psStringRes, name);
					ASSERT(str, "Cannot find the view data string with id \"%s\"", name);
					QString qstr = QString::fromUtf8(str);
					psViewReplay->pSeqList[dataInc].textMsg.push_back(qstr);
				}
				//get the audio text string
				sscanf(pViewMsgData,",%255[^,'\r\n],%d%n", audioName, &dummy, &cnt);
				pViewMsgData += cnt;

				if (strcmp(audioName, "0"))
				{
					//allocate space
					psViewReplay->pSeqList[dataInc].pAudio = strdup(audioName);
				}
				else
				{
					psViewReplay->pSeqList[dataInc].pAudio = NULL;
				}
			}
			psViewData->type = VIEW_RPL;//no longer need to know if it is extended type
			break;

		case VIEW_PROX:
			psViewData->pData = (VIEW_PROXIMITY *) malloc(sizeof(VIEW_PROXIMITY));
			{
				int tmp;

				audioName[0] = '\0';
				sscanf( pViewMsgData, ", %d,%d,%d,%255[^,'\r\n],%d%n", &LocX, &LocY, &LocZ,
						audioName, &tmp, &cnt);
				proxType = (PROX_TYPE)tmp;
			}
			pViewMsgData += cnt;

			//allocate audioID
			if ( strcmp( audioName, "0" ) == 0 )
			{
				audioID = NO_SOUND;
			}
			else
			{
				if ( (audioID = audio_GetIDFromStr( audioName )) == NO_SOUND )
				{
					ASSERT(false, "couldn't get ID %d for weapon sound %s", audioID, audioName);
					return NULL;
				}

				if ((audioID < 0
				  || audioID > ID_MAX_SOUND)
				 && audioID != NO_SOUND)
				{
					ASSERT(false, "Invalid Weapon Sound ID - %d for weapon %s", audioID, audioName);
					return NULL;
				}
			}


			((VIEW_PROXIMITY *)psViewData->pData)->audioID = audioID;

			if (LocX < 0)
			{
				ASSERT(false, "Negative X coord for prox message - %s",name);
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->x = (UDWORD)LocX;
			if (LocY < 0)
			{
				ASSERT(false, "Negative Y coord for prox message - %s",name);
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->y = (UDWORD)LocY;
			if (LocZ < 0)
			{
				ASSERT(false, "Negative Z coord for prox message - %s",name);
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->z = (UDWORD)LocZ;

			if (proxType > PROX_TYPES)
			{
				ASSERT(false, "Invalid proximity message sub type - %s", name);
				return NULL;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->proxType = proxType;
			break;
		default:
			ASSERT(false, "Unknown ViewData type");
			return NULL;
		}
		//increment the pointer to the start of the next record
		pViewMsgData = strchr(pViewMsgData,'\n') + 1;

		apsViewData.insert(psViewData->pName, psViewData);
	}

	return filename; // so that cleanup function will be called for correct data
}

const char *loadResearchViewData(const char* fileName)
{
	ASSERT_OR_RETURN(NULL, PHYSFS_exists(fileName), "%s not found", fileName);
	WzConfig ini(fileName);
	ASSERT_OR_RETURN(NULL, ini.status() == QSettings::NoError, "%s not loaded", fileName);

	const char *filedup = strdup(fileName);
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		VIEWDATA *v = new VIEWDATA;
		VIEW_RESEARCH *r = (VIEW_RESEARCH *)malloc(sizeof(*r));

		v->pData = NULL;
		v->pName = strdup(list[i].toUtf8().constData());
		v->fileName = filedup;
		memset(r, 0, sizeof(*r));

		ini.beginGroup(list[i]);

		v->textMsg = ini.value("text").toStringList();
		for (int j = 0; j < v->textMsg.size(); j++)
		{
			v->textMsg[j].remove('\t');
			v->textMsg[j].remove(0, 2); // initial _(
			v->textMsg[j].remove(v->textMsg[j].length() - 1, 1); // final )
		}
		v->type = VIEW_RES;
		v->pData = r;
		if (ini.contains("imdName"))
		{
			r->pIMD = (iIMDShape *) resGetData("IMD", ini.value("imdName").toString().toUtf8().constData());
		}
		if (ini.contains("imdName2"))
		{
			r->pIMD2 = (iIMDShape *) resGetData("IMD", ini.value("imdName2").toString().toUtf8().constData());
		}
		if (ini.contains("sequenceName"))
		{
			sstrcpy(r->sequenceName, ini.value("sequenceName").toString().toUtf8().constData());
		}
		if (ini.contains("audioName"))
		{
			r->pAudio = strdup(ini.value("audioName").toString().toUtf8().constData());
		}

		ini.endGroup();
		apsViewData.insert(v->pName, v);
	}
	return filedup; // so that cleanup function will be called on rigth data
}

/* Get the view data identified by the name */
VIEWDATA *getViewData(const char *pName)
{
	VIEWDATA *ptr = apsViewData.value(pName);
	if (!ptr) // dump for debugging
	{
		QMap<QString, VIEWDATA *>::iterator iter = apsViewData.begin();
		while (iter != apsViewData.constEnd())
		{
			VIEWDATA *psViewData = iter.value();
			debug(LOG_WZ, "\t%s", psViewData->pName);
			++iter;
		}
	}
	ASSERT(ptr, "Message %s not found, run with --debug=wz to get a list of all known messages", pName);
	return ptr;
}

/* Release the message heaps */
bool messageShutdown(void)
{
	freeMessages();
	apsViewData.clear();
	return true;
}

//check for any messages using this viewdata and remove them
static void checkMessages(MSG_VIEWDATA *psViewData)
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

/* Release the viewdata memory */
void viewDataShutDown(const char *fileName)
{
	debug(LOG_MSG, "calling shutdown for %s", fileName);
	QMap<QString, VIEWDATA *>::iterator iter = apsViewData.begin();
	char *filedup = NULL;
	while (iter != apsViewData.constEnd())
	{
		VIEWDATA *psViewData = iter.value();

		if (strcmp(psViewData->fileName, fileName) != 0)
		{
			++iter;
			continue; // do not delete this now
		}

		filedup = (char *)psViewData->fileName;

		// check for any messages using this viewdata
		checkMessages((MSG_VIEWDATA *)psViewData);

		free(psViewData->pName);

		// free the space allocated for multiple sequences
		if (psViewData->type == VIEW_RPL)
		{
			VIEW_REPLAY* const psViewReplay = (VIEW_REPLAY *)psViewData->pData;
			if (psViewReplay->numSeq)
			{
				unsigned int seqInc;
				for (seqInc = 0; seqInc < psViewReplay->numSeq; ++seqInc)
				{
					if (psViewReplay->pSeqList[seqInc].pAudio)
					{
						free(psViewReplay->pSeqList[seqInc].pAudio);
					}
				}
				delete[] psViewReplay->pSeqList;
			}
		}
		else if (psViewData->type == VIEW_RES)
		{
			VIEW_RESEARCH* const psViewRes = (VIEW_RESEARCH *)psViewData->pData;
			if (psViewRes->pAudio)
			{
				free(psViewRes->pAudio);
			}
		}
		free(psViewData->pData);
		delete psViewData;
		iter = apsViewData.erase(iter);
	}
	free(filedup);
}

/* Looks through the players list of messages to find one with the same viewData
pointer and which is the same type of message - used in scriptFuncs */
MESSAGE * findMessage(MSG_VIEWDATA *pViewData, MESSAGE_TYPE type, UDWORD player)
{
	MESSAGE					*psCurr;

	ASSERT_OR_RETURN(NULL, player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN(NULL , type < MSG_TYPES, "Bad message type");

	for (psCurr = apsMessages[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->type == type && psCurr->pViewData == pViewData)
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
	if (psProxDisp->type == POS_PROXDATA)
	{
		VIEWDATA	*psViewData = (VIEWDATA *)psProxDisp->psMessage->pViewData;
		VIEW_PROXIMITY	*psViewProx = (VIEW_PROXIMITY *)psViewData->pData;

		//display text - if any
		if (psViewData->textMsg.size() > 0 && psViewData->type != VIEW_BEACON)
		{
			addConsoleMessage(psViewData->textMsg[0].toAscii().constData(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}

		//play message - if any
		if ( psViewProx->audioID != NO_AUDIO_MSG )
		{
			audio_QueueTrackPos(psViewProx->audioID, psViewProx->x, psViewProx->y, psViewProx->z);
		}
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		FEATURE	*psFeature = (FEATURE *)psProxDisp->psMessage->pViewData;

		ASSERT_OR_RETURN( , ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->type ==
			OBJ_FEATURE, "invalid feature" );

		if (psFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			//play default audio message for oil resource
			audio_QueueTrackPos( ID_SOUND_RESOURCE_HERE, psFeature->pos.x,
								 psFeature->pos.y, psFeature->pos.z );
		}
		else if (psFeature->psStats->subType == FEAT_GEN_ARTE)
		{
			//play default audio message for artefact
			audio_QueueTrackPos( ID_SOUND_ARTIFACT, psFeature->pos.x,
								 psFeature->pos.y, psFeature->pos.z );
		}
	}

	//set the read flag
	psProxDisp->psMessage->read = true;
}
