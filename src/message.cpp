/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#include <map>
#include "lib/framework/frame.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"
#include "lib/framework/object_list_iteration.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/ivis_opengl/imd.h"

#include "console.h"
#include "hci.h"
#include "stats.h"
#include "text.h"
#include "qtscript.h"
#include "wzjsonhelpers.h"

static std::map<WzString, VIEWDATA *> apsViewData;

/* The id number for the next message allocated
 * Each message will have a unique id number irrespective of type
 */
static UDWORD	msgID = 0;

static int currentNumProxDisplays = 0;

PerPlayerMessageLists apsMessages;

/* The list of proximity displays allocated */
PerPlayerProximityDisplayLists apsProxDisp;

/* The IMD to use for the proximity messages */
iIMDBaseShape	*pProximityMsgIMD;

bool releaseObjectives = true;


/* Creating a new message
 * new is a pointer to a pointer to the new message
 * type is the type of the message
 */
static inline MESSAGE *createMessage(MESSAGE_TYPE msgType, UDWORD player)
{
	MESSAGE *newMsg;

	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN(nullptr, msgType < MSG_TYPES, "Bad message");

	// Allocate memory for the message, and on failure return a NULL pointer
	newMsg = new MESSAGE;
	newMsg->type = msgType;
	newMsg->dataType = MSG_DATA_DEFAULT;
	newMsg->id = (msgID << 3) | selectedPlayer;
	newMsg->player = player;
	msgID++;

	return newMsg;
}

/* Add the message to the BOTTOM of the list
 * list is a pointer to the message list
 * Order is now CAMPAIGN, MISSION, RESEARCH/PROXIMITY
 */
static inline void addMessageToList(PerPlayerMessageLists& list, MESSAGE *msg, UDWORD player)
{
	ASSERT_OR_RETURN(, msg != nullptr, "Invalid message pointer");
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player");

	// If there is no message list, create one
	if (list[player].empty())
	{
		list[player].emplace_front(msg);
		return;
	}

	switch (msg->type)
	{
	case MSG_CAMPAIGN:
	{
		/*add it before the first mission/research/prox message */
		const auto it = std::find_if(list[player].begin(), list[player].end(), [](MESSAGE* psCurr)
		{
			return psCurr->type == MSG_MISSION ||
				psCurr->type == MSG_RESEARCH ||
				psCurr->type == MSG_PROXIMITY;
		});
		list[player].emplace(it, msg);
		break;
	}
	case MSG_MISSION:
	{
		/*add it before the first research/prox message */
		const auto it = std::find_if(list[player].begin(), list[player].end(), [](MESSAGE* psCurr)
		{
			return psCurr->type == MSG_RESEARCH ||
				psCurr->type == MSG_PROXIMITY;
		});
		list[player].emplace(it, msg);
		break;
	}
	case MSG_RESEARCH:
	case MSG_PROXIMITY:
		/*add it to the bottom of the list */
		list[player].emplace_back(msg);
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
static inline void removeMessageFromList(PerPlayerMessageLists& list, MESSAGE *del, UDWORD player)
{
	ASSERT_OR_RETURN(, del != nullptr, "Invalid message pointer");
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player");

	auto it = std::find(list[player].begin(), list[player].end(), del);
	ASSERT_OR_RETURN(, it != list[player].end(), "Message %p not found in list", static_cast<void*>(del));
	list[player].erase(it);
	delete del;
}

static inline void releaseAllMessages(PerPlayerMessageLists& list)
{
	// Iterate through all players' message lists
	for (MessageList& pl : list)
	{
		// Iterate through all messages in list
		for (MESSAGE* psCurr : pl)
		{
			delete psCurr;
		}
		pl.clear();
	}
}

bool messageInitVars()
{
	int i;

	msgID = 0;
	currentNumProxDisplays = 0;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		apsMessages[i].clear();
		apsProxDisp[i].clear();
	}

	pProximityMsgIMD = nullptr;

	return true;
}

//allocates the viewdata heap
bool initViewData()
{
	return true;
}

/* Adds a beacon message. A wrapper for addMessage() */
MESSAGE *addBeaconMessage(MESSAGE_TYPE msgType, bool proxPos, UDWORD player)
{
	MESSAGE *psBeaconMsgToAdd = addMessage(msgType, proxPos, player);

	ASSERT_OR_RETURN(nullptr, psBeaconMsgToAdd, "createMessage failed");

	// remember we are storing beacon data in this message
	psBeaconMsgToAdd->dataType = MSG_DATA_BEACON;

	return psBeaconMsgToAdd;
}

/* adds a proximity display - holds variables that enable the message to be
 displayed in the Intelligence Screen*/
static void addProximityDisplay(MESSAGE *psMessage, bool proxPos, UDWORD player)
{
	PROXIMITY_DISPLAY *psToAdd;

	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player");
	debug(LOG_MSG, "Added prox display for player %u (proxPos=%d)", player, (int)proxPos);

	//create the proximity display
	psToAdd = new PROXIMITY_DISPLAY;

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
		apsProxDisp[player].emplace_front(psToAdd);
		currentNumProxDisplays++;
	}
	else
	{
		delete psToAdd;	// clean up
	}
}

/*Add a message to the list */
MESSAGE *addMessage(MESSAGE_TYPE msgType, bool proxPos, UDWORD player)
{
	//first create a message of the required type
	MESSAGE *psMsgToAdd = createMessage(msgType, player);

	debug(LOG_MSG, "adding message for player %d, type is %d, proximity is %d", player, msgType, proxPos);

	ASSERT(psMsgToAdd, "createMessage failed");
	if (!psMsgToAdd)
	{
		return nullptr;
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
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN(, psMessage != nullptr, "Bad message");

	if (apsProxDisp[player].empty())
	{
		return;	// no corresponding proximity display
	}

	//find the proximity display for this message
	auto it = std::find_if(apsProxDisp[player].begin(), apsProxDisp[player].end(), [psMessage](PROXIMITY_DISPLAY* psCurr)
	{
		return psCurr->psMessage == psMessage;
	});
	if (it != apsProxDisp[player].end())
	{
		PROXIMITY_DISPLAY* psDisp = *it;
		apsProxDisp[player].erase(it);
		intRemoveProximityButton(psDisp);
		delete psDisp;
	}
}

/*remove a message */
void removeMessage(MESSAGE *psDel, UDWORD player)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN(, psDel != nullptr, "Bad message");
	debug(LOG_MSG, "removing message for player %d", player);

	if (psDel->type == MSG_PROXIMITY)
	{
		removeProxDisp(psDel, player);
	}
	removeMessageFromList(apsMessages, psDel, player);
}

/* Remove all Messages*/
void freeMessages()
{
	releaseAllProxDisp();
	releaseAllMessages(apsMessages);
	jsDebugMessageUpdate();
}

/* removes all the proximity displays */
void releaseAllProxDisp()
{
	bool removedAMessage = false;

	for (size_t player = 0, end = apsProxDisp.size(); player < end; ++player)
	{
		mutating_list_iterate(apsProxDisp[player], [&removedAMessage, player](PROXIMITY_DISPLAY* psCurr)
		{
			//remove message associated with this display
			removeMessage(psCurr->psMessage, player);
			removedAMessage = true;

			return IterationResult::CONTINUE_ITERATION;
		});
		apsProxDisp[player].clear();
	}
	//re-initialise variables
	currentNumProxDisplays = 0;
	if (removedAMessage)
	{
		jsDebugMessageUpdate();
	}
}

/* Initialise the message heaps */
bool initMessage()
{
	//set up the imd used for proximity messages
	pProximityMsgIMD = modelGet("arrow.pie");
	if (pProximityMsgIMD == nullptr)
	{
		ASSERT(false, "Unable to load Proximity Message PIE");
		return false;
	}

	return true;
}

/* Return the number of newlines in a file buffer */
static unsigned numCR(const char *pFileBuffer, unsigned fileSize)
{
	unsigned lines = 0;

	while (fileSize-- > 0)
	{
		if (*pFileBuffer++ == '\n')
		{
			lines++;
		}
	}

	return lines;
}

/*load the view data for the messages from the file */
WzString *loadViewData(const char *pViewMsgData, UDWORD bufferSize)
{
	UDWORD			numData, count;
	int				dummy;
	VIEW_RESEARCH		*psViewRes;
	VIEW_REPLAY			*psViewReplay;
	char				name[MAX_STR_LENGTH], imdName[MAX_STR_LENGTH],
	                    string[MAX_STR_LENGTH],
	                    imdName2[MAX_STR_LENGTH];
	char				audioName[MAX_STR_LENGTH];
	SDWORD				LocX, LocY, LocZ, audioID;
	PROX_TYPE	proxType;
	int cnt;
	WzString *filename = new WzString(GetLastResourceFilename());

	numData = numCR(pViewMsgData, bufferSize);
	for (unsigned i = 0; i < numData; i++)
	{
		VIEWDATA *psViewData = new VIEWDATA;
		UDWORD numText;
		int readint;

		psViewData->pData = nullptr;
		psViewData->type = VIEW_SIZE;
		psViewData->fileName = *filename;
		name[0] = '\0';

		//read the data into the storage - the data is delimited using comma's
		sscanf(pViewMsgData, "%255[^,'\r\n],%u%n", name, &numText, &cnt);
		pViewMsgData += cnt;

		//allocate storage for the name
		psViewData->name = name;
		debug(LOG_MSG, "Loaded %s", psViewData->name.toUtf8().c_str());

		//read in the data for the text strings
		for (unsigned dataInc = 0; dataInc < numText; dataInc++)
		{
			name[0] = '\0';
			sscanf(pViewMsgData, ",%255[^,'\r\n]%n", name, &cnt);
			pViewMsgData += cnt;

			// Get the string from the ID string
			const char *str = strresGetString(psStringRes, name);
			ASSERT(str, "Cannot find the view data string with id \"%s\"", name);
			WzString wstr = WzString::fromUtf8(str);
			psViewData->textMsg.push_back(wstr);
		}

		sscanf(pViewMsgData, ",%d%n", &readint, &cnt);
		psViewData->type = (VIEW_TYPE)readint;
		pViewMsgData += cnt;

		//allocate data according to type
		switch (psViewData->type)
		{
		case VIEW_RES:
			psViewData->pData = new VIEW_RESEARCH;
			imdName[0] = '\0';
			imdName2[0] = '\0';
			string[0] = '\0';
			audioName[0] = '\0';
			sscanf(pViewMsgData, ",%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n],%d%n",
			       imdName, imdName2, string, audioName, &dummy, &cnt);
			pViewMsgData += cnt;
			psViewRes = (VIEW_RESEARCH *)psViewData->pData;
			psViewRes->pIMD = modelGet(imdName);
			if (psViewRes->pIMD == nullptr)
			{
				ASSERT(false, "Cannot find the PIE for message %s", name);
				delete psViewData;
				return nullptr;
			}
			if (strcmp(imdName2, "0"))
			{
				psViewRes->pIMD2 = modelGet(imdName2);
				if (psViewRes->pIMD2 == nullptr)
				{
					ASSERT(false, "Cannot find the 2nd PIE for message %s", name);
					delete psViewData;
					return nullptr;
				}
			}
			else
			{
				psViewRes->pIMD2 = nullptr;
			}
			psViewRes->sequenceName = string;
			//get the audio text string
			if (strcmp(audioName, "0"))
			{
				//allocate space
				psViewRes->audio = audioName;
			}
			break;
		case VIEW_RPL:
		case VIEW_RPLX:
			// This is now also used for the stream playing on the PSX
			// NOTE: on the psx the last entry (audioID) is used as the number of frames in the stream
			psViewData->pData = new VIEW_REPLAY;
			psViewReplay = (VIEW_REPLAY *)psViewData->pData;

			//read in number of sequences for this message
			sscanf(pViewMsgData, ",%u%n", &count, &cnt);
			pViewMsgData += cnt;

			psViewReplay->seqList.resize(count);

			//read in the data for the sequences
			for (unsigned dataInc = 0; dataInc < psViewReplay->seqList.size(); dataInc++)
			{
				int numSeqText = 0;

				name[0] = '\0';
				//load extradat for extended type only
				if (psViewData->type == VIEW_RPL)
				{
					sscanf(pViewMsgData, ",%255[^,'\r\n],%u%n", name, &count, &cnt);
					pViewMsgData += cnt;
					//set the flag to default
					psViewReplay->seqList[dataInc].flag = 0;
					numSeqText = count;
				}
				else //extended type
				{
					int count2;
					sscanf(pViewMsgData, ",%255[^,'\r\n],%u,%d%n", name, &count, &count2, &cnt);
					pViewMsgData += cnt;
					psViewReplay->seqList[dataInc].flag = (UBYTE)count;
					numSeqText = count2;
				}
				psViewReplay->seqList[dataInc].sequenceName = name;

				//get the text strings for this sequence - if any
				for (unsigned seqInc = 0; seqInc < numSeqText; seqInc++)
				{
					name[0] = '\0';
					sscanf(pViewMsgData, ",%255[^,'\r\n]%n", name, &cnt);
					pViewMsgData += cnt;

					// Get the string from the ID string
					const char *str = strresGetString(psStringRes, name);
					ASSERT(str, "Cannot find the view data string with id \"%s\"", name);
					WzString qstr = WzString::fromUtf8(str);
					psViewReplay->seqList[dataInc].textMsg.push_back(qstr);
				}
				//get the audio text string
				sscanf(pViewMsgData, ",%255[^,'\r\n],%d%n", audioName, &dummy, &cnt);
				pViewMsgData += cnt;

				if (strcmp(audioName, "0"))
				{
					//allocate space
					ASSERT(false, "Unexpected separate audio track provided (%s), for %s in %s - this is deprecated!", audioName, psViewReplay->seqList[dataInc].sequenceName.toUtf8().c_str(), (filename) ? filename->toUtf8().c_str() : "<unknown filename>");
					psViewReplay->seqList[dataInc].audio = audioName;
				}
			}
			psViewData->type = VIEW_RPL;//no longer need to know if it is extended type
			break;

		case VIEW_PROX:
			psViewData->pData = new VIEW_PROXIMITY;
			{
				int tmp;

				audioName[0] = '\0';
				sscanf(pViewMsgData, ", %d,%d,%d,%255[^,'\r\n],%d%n", &LocX, &LocY, &LocZ,
				       audioName, &tmp, &cnt);
				proxType = (PROX_TYPE)tmp;
			}
			pViewMsgData += cnt;

			//allocate audioID
			if (strcmp(audioName, "0") == 0)
			{
				audioID = NO_SOUND;
			}
			else
			{
				if ((audioID = audio_GetIDFromStr(audioName)) == NO_SOUND)
				{
					ASSERT(false, "couldn't get ID %d for weapon sound %s", audioID, audioName);
					return nullptr;
				}

				if ((audioID < 0
				     || audioID > ID_MAX_SOUND)
				    && audioID != NO_SOUND)
				{
					ASSERT(false, "Invalid Weapon Sound ID - %d for weapon %s", audioID, audioName);
					return nullptr;
				}
			}


			((VIEW_PROXIMITY *)psViewData->pData)->audioID = audioID;

			if (LocX < 0)
			{
				ASSERT(false, "Negative X coord for prox message - %s", name);
				return nullptr;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->x = (UDWORD)LocX;
			if (LocY < 0)
			{
				ASSERT(false, "Negative Y coord for prox message - %s", name);
				return nullptr;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->y = (UDWORD)LocY;
			if (LocZ < 0)
			{
				ASSERT(false, "Negative Z coord for prox message - %s", name);
				return nullptr;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->z = (UDWORD)LocZ;

			if (proxType > PROX_TYPES)
			{
				ASSERT(false, "Invalid proximity message sub type - %s", name);
				return nullptr;
			}
			((VIEW_PROXIMITY *)psViewData->pData)->proxType = proxType;
			break;
		default:
			ASSERT(false, "Unknown ViewData type");
			return nullptr;
		}
		//increment the pointer to the start of the next record
		pViewMsgData = strchr(pViewMsgData, '\n') + 1;

		apsViewData[psViewData->name] = psViewData;
	}

	return filename; // so that cleanup function will be called for correct data
}

WzString *loadResearchViewData(const char *fileName)
{
	ASSERT_OR_RETURN(nullptr, PHYSFS_exists(fileName), "%s not found", fileName);
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);
	std::vector<WzString> list = ini.childGroups();
	for (size_t i = 0; i < list.size(); ++i)
	{
		VIEWDATA *v = new VIEWDATA;
		VIEW_RESEARCH *r = new VIEW_RESEARCH;

		v->pData = nullptr;
		v->name = list[i];
		v->fileName = fileName;

		ini.beginGroup(list[i]);

		v->textMsg = ini.value("text").toWzStringList();
		for (size_t j = 0; j < v->textMsg.size(); j++)
		{
			v->textMsg[j].remove("\t");
			v->textMsg[j] = WzString::fromUtf8(_(v->textMsg[j].toUtf8().c_str()));
			v->textMsg[j].replace("%%", "%");
		}
		v->type = VIEW_RES;
		v->pData = r;
		if (ini.contains("imdName"))
		{
			r->pIMD = modelGet(ini.value("imdName").toWzString());
		}
		if (ini.contains("imdName2"))
		{
			r->pIMD2 = modelGet(ini.value("imdName2").toWzString());
		}
		if (ini.contains("sequenceName"))
		{
			r->sequenceName = ini.value("sequenceName").toWzString();
		}
		if (ini.contains("audioName"))
		{
			r->audio = ini.value("audioName").toWzString();
		}

		ini.endGroup();
		apsViewData[v->name] = v;
	}
	return new WzString(fileName); // so that cleanup function will be called on right data
}

WzString *loadProximityViewData(const char *fileName)
{
	ASSERT_OR_RETURN(nullptr, PHYSFS_exists(fileName), "%s not found", fileName);
	WzConfig ini(fileName, WzConfig::ReadOnlyAndRequired);

	// "type": "wz2100.proxmsgs.v1"
	if (!ini.contains("type"))
	{
		debug(LOG_ERROR, "Missing required \"type\" key: %s", fileName);
		return nullptr;
	}
	if (ini.value("type").toWzString() != "wz2100.proxmsgs.v1")
	{
		debug(LOG_ERROR, "Unexpected \"type\" - expecting \"wz2100.proxmsgs.v1\": %s", fileName);
		return nullptr;
	}

	// msgs
	if (!ini.contains("msgs"))
	{
		debug(LOG_ERROR, "Missing required \"msgs\" key: %s", fileName);
		return nullptr;
	}
	if (!ini.beginGroup("msgs"))
	{
		debug(LOG_ERROR, "Missing valid \"msgs\" value: %s", fileName);
		return nullptr;
	}
	std::vector<WzString> list = ini.childGroups();
	for (size_t i = 0; i < list.size(); ++i)
	{
		// Proximity viewdata init
		VIEWDATA *v = new VIEWDATA;
		VIEW_PROXIMITY *r = new VIEW_PROXIMITY;
		v->pData = r;
		v->name = list[i];
		v->fileName = fileName;
		v->type = VIEW_PROX;

		ini.beginGroup(list[i]);

		// Set the message string when this Proximity blip gets clicked on
		if (ini.contains("message"))
		{
			nlohmann::json array = ini.json("message");
			if (!array.is_null() && array.is_array())
			{
				for(auto &a : array)
				{
					if (!a.is_string())
					{
						debug(LOG_ERROR, "\"message\" value is not a string: %s", fileName);
						continue;
					}
					std::string msg = a.get<std::string>();
					const char *str = strresGetString(psStringRes, msg.c_str());
					ASSERT(str, "Cannot find the view data string with id \"%s\"", msg.c_str());
					v->textMsg.push_back(WzString::fromUtf8(str));
				}
			}
			else
			{
				std::string msg = ini.value("message").toWzString().toUtf8();
				const char *str = strresGetString(psStringRes, msg.c_str());
				ASSERT(str, "Cannot find the view data string with id \"%s\"", msg.c_str());
				v->textMsg.push_back(WzString::fromUtf8(str));
			}
		}

		// Read in the rest of the Proximity data
		if (ini.contains("x")) { r->x = ini.value("x").toUInt(); }
		if (ini.contains("y")) { r->y = ini.value("y").toUInt(); }
		if (ini.contains("z")) { r->z = ini.value("z").toUInt(); }
		if (ini.contains("type"))
		{
			unsigned int proxType = ini.value("type").toUInt();
			if (proxType > PROX_TYPES)
			{
				ASSERT(false, "Invalid proximity message sub type - %s", v->name.toUtf8().c_str());
				return nullptr;
			}
			r->proxType = static_cast<PROX_TYPE>(proxType);
		}
		if (ini.contains("audio"))
		{
			int audioID = 0;
			std::string audioName = ini.value("audio").toWzString().toUtf8();
			if (strcmp(audioName.c_str(), "0") == 0)
			{
				audioID = NO_SOUND;
			}
			else
			{
				if ((audioID = audio_GetIDFromStr(audioName.c_str())) == NO_SOUND)
				{
					ASSERT(false, "couldn't get ID %d for weapon sound %s", audioID, audioName.c_str());
					return nullptr;
				}
				if ((audioID < 0 || audioID > ID_MAX_SOUND) && audioID != NO_SOUND)
				{
					ASSERT(false, "Invalid Weapon Sound ID - %d for weapon %s", audioID, audioName.c_str());
					return nullptr;
				}
			}
			r->audioID = audioID;
		}

		ini.endGroup();
		apsViewData[v->name] = v;
	}
	ini.endGroup(); // "msgs"

	return new WzString(fileName); // so that cleanup function will be called on right data
}

inline void from_json(const nlohmann::json& j, SEQ_DISPLAY& v)
{
	v.sequenceName = WzString::fromUtf8(j["video"].get<std::string>());
	debug(LOG_WZ, "Sequence name: %s", v.sequenceName.toUtf8().c_str());
	v.flag = j["loop"].get<uint32_t>();
	debug(LOG_WZ, "Sequence loop: %d", v.flag);
	// Set the subtitle string for the sequence.
	const nlohmann::json& subtitles = j["subtitles"];
	if (!subtitles.is_null() && subtitles.is_array())
	{
		for (auto &a : subtitles)
		{
			std::string msg = a.get<std::string>();
			if (msg.length() != 0)
			{
				const char *str = strresGetString(psStringRes, msg.c_str());
				ASSERT(str, "Cannot find the view data string with id \"%s\"", msg.c_str());
				v.textMsg.push_back(WzString::fromUtf8(str));
				debug(LOG_WZ, "Sequence subtitle array: %s", msg.c_str());
			}
		}
	}
	else
	{
		std::string msg = subtitles.get<std::string>();
		if (msg.length() != 0)
		{
			const char *str = strresGetString(psStringRes, msg.c_str());
			ASSERT(str, "Cannot find the view data string with id \"%s\"", msg.c_str());
			v.textMsg.push_back(WzString::fromUtf8(str));
			debug(LOG_WZ, "Sequence subtitle string: %s", msg.c_str());
		}
	}
}

#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && !defined(WZ_CC_CLANG) && (7 <= __GNUC__) && (__GNUC__ < 10)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wnull-dereference" // GCC < 10 warning is buggy
#endif

WzString *loadFlicViewData(const char *fileName)
{
	ASSERT_OR_RETURN(nullptr, PHYSFS_exists(fileName), "%s not found", fileName);

	auto jsonObjOpt = wzLoadJsonObjectFromFile(fileName);
	if (!jsonObjOpt.has_value())
	{
		debug(LOG_ERROR, "Failed to load JSON: %s", fileName);
		return nullptr;
	}
	const auto& jsonObj = jsonObjOpt.value();

	// "type": "wz2100.briefs.v1"
	auto it = jsonObj.find("type");
	if (it == jsonObj.end())
	{
		debug(LOG_ERROR, "Missing required \"type\" key: %s", fileName);
		return nullptr;
	}
	if (!it.value().is_string() || it.value().get<std::string>() != "wz2100.briefs.v1")
	{
		debug(LOG_ERROR, "Unexpected \"type\" - expecting \"wz2100.briefs.v1\": %s", fileName);
		return nullptr;
	}

	// "briefs"
	it = jsonObj.find("briefs");
	if (it == jsonObj.end())
	{
		debug(LOG_ERROR, "Missing required \"briefs\" key: %s", fileName);
		return nullptr;
	}
	if (!it.value().is_object())
	{
		debug(LOG_ERROR, "\"briefs\" value is not an object: %s", fileName);
		return nullptr;
	}
	const auto& briefsObj = it.value();

	for (auto brief : briefsObj.items())
	{
		debug(LOG_WZ, "Sequence video set: %s", brief.key().c_str());

		const auto& briefObj = brief.value();
		if (!briefObj.is_object())
		{
			debug(LOG_ERROR, "\"briefs\"[%s] value is not an object: %s", brief.key().c_str(), fileName);
			continue;
		}

		auto briefIt = briefObj.find("sequences");
		if (briefIt == briefObj.end())
		{
			debug(LOG_ERROR, "\"briefs\"[%s] lacks a \"sequences\" property: %s", brief.key().c_str(), fileName);
			continue;
		}
		if (!briefIt.value().is_array())
		{
			debug(LOG_ERROR, "\"briefs\"[%s] \"sequences\" property must have an array value: %s", brief.key().c_str(), fileName);
			continue;
		}
		std::vector<SEQ_DISPLAY> seqList;
		try {
			seqList = briefObj["sequences"].get<std::vector<SEQ_DISPLAY>>();
		}
		catch (const std::exception&)
		{
			debug(LOG_ERROR, "Failed to parse \"briefs\"[%s] \"sequences\" property value: %s", brief.key().c_str(), fileName);
			continue;
		}
		debug(LOG_WZ, "Sequence list size: %zu", seqList.size());

		// Replay viewdata init
		VIEWDATA *v = new VIEWDATA;
		VIEW_REPLAY *r = new VIEW_REPLAY;
		v->pData = r;
		v->fileName = fileName;
		v->type = VIEW_RPL;

		v->name = WzString::fromUtf8(brief.key());
		r->seqList = std::move(seqList);

		apsViewData[v->name] = v;
	}
	return new WzString(fileName); // so that cleanup function will be called on right data
}

#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && !defined(WZ_CC_CLANG) && (7 <= __GNUC__) && (__GNUC__ < 10)
# pragma GCC diagnostic pop
#endif

/* Get the view data identified by the name */
VIEWDATA *getViewData(const WzString &name)
{
	std::map<WzString, VIEWDATA *>::iterator it = apsViewData.find(name);
	VIEWDATA *ptr = (it != apsViewData.end()) ? it->second : nullptr;
	if (!ptr) // dump for debugging
	{
		std::map<WzString, VIEWDATA *>::iterator iter = apsViewData.begin();
		while (iter != apsViewData.end())
		{
			VIEWDATA *psViewData = iter->second;
			debug(LOG_WZ, "\t%s", psViewData->name.toUtf8().c_str());
			++iter;
		}
	}
	ASSERT(ptr, "Message %s not found, run with --debug=wz to get a list of all known messages", name.toUtf8().c_str());
	return ptr;
}

std::vector<WzString> getViewDataKeys()
{
	std::vector<WzString> keys;
	keys.reserve(apsViewData.size());
	for(auto const& kvpair: apsViewData)
	{
		keys.push_back(kvpair.first);
	}
	return keys;
}

/* Release the message heaps */
bool messageShutdown()
{
	freeMessages();
	return true;
}

//check for any messages using this viewdata and remove them
static void checkMessages(VIEWDATA *psViewData)
{
	bool removedAMessage = false;
	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		mutating_list_iterate(apsMessages[i], [&removedAMessage, i, psViewData](MESSAGE* psCurr)
		{
			if (psCurr->pViewData == psViewData)
			{
				removeMessage(psCurr, i);
				removedAMessage = true;
			}
			return IterationResult::CONTINUE_ITERATION;
		});
	}

	if (removedAMessage)
	{
		jsDebugMessageUpdate();
	}
}

void releaseAllFlicMessages(PerPlayerMessageLists& list)
{
	// Iterate through all players' message lists
	for (unsigned int i = 0; i < MAX_PLAYERS; i++)
	{
		// Iterate through all messages in list
		mutating_list_iterate(list[i], [i](MESSAGE* psCurr)
		{
			if (psCurr->type == MSG_MISSION || psCurr->type == MSG_CAMPAIGN)
			{
				removeMessage(psCurr, i);
			}
			return IterationResult::CONTINUE_ITERATION;
		});
	}
}

/* Release the viewdata memory */
void viewDataShutDown(const char *fileName)
{
	debug(LOG_MSG, "calling shutdown for %s", fileName);
	std::map<WzString, VIEWDATA *>::iterator iter = apsViewData.begin();
	while (iter != apsViewData.end())
	{
		VIEWDATA *psViewData = iter->second;

		if (psViewData->fileName.compare(fileName) != 0)
		{
			++iter;
			continue; // do not delete this now
		}
		if (!releaseObjectives && (psViewData->type == VIEW_RPL || psViewData->type == VIEW_RPLX))
		{
			++iter;
			continue;
		}

		// check for any messages using this viewdata
		checkMessages(psViewData);
		delete psViewData->pData;
		delete psViewData;
		iter = apsViewData.erase(iter);
	}
}

/* Looks through the players list of messages to find one with the same viewData
pointer and which is the same type of message - used in scriptFuncs */
MESSAGE *findMessage(const VIEWDATA *pViewData, MESSAGE_TYPE type, UDWORD player)
{
	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN(nullptr , type < MSG_TYPES, "Bad message type");

	for (MESSAGE *psCurr : apsMessages[player])
	{
		if (psCurr->type == type && psCurr->pViewData == pViewData)
		{
			return psCurr;
		}
	}

	//not found the message so return NULL
	return nullptr;
}

MESSAGE *findMessage(const BASE_OBJECT *psObj, MESSAGE_TYPE type, UDWORD player)
{
	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "Bad player");
	ASSERT_OR_RETURN(nullptr , type < MSG_TYPES, "Bad message type");

	for (MESSAGE *psCurr : apsMessages[player])
	{
		if (psCurr->type == type && psCurr->psObj == psObj)
		{
			return psCurr;
		}
	}

	return nullptr;
}

/* 'displays' a proximity display*/
void displayProximityMessage(PROXIMITY_DISPLAY *psProxDisp)
{
	if (psProxDisp->type == POS_PROXDATA)
	{
		if (psProxDisp->psMessage->pViewData == nullptr)
		{
			return; // if no data - ignore message
		}

		const VIEWDATA *psViewData = psProxDisp->psMessage->pViewData;
		const VIEW_PROXIMITY *psViewProx = (VIEW_PROXIMITY *)psViewData->pData;

		//display text - if any
		if (!psViewData->textMsg.empty() && psViewData->type != VIEW_BEACON)
		{
			for (size_t i = 0; i < psViewData->textMsg.size(); ++i)
			{
				addConsoleMessage(psViewData->textMsg[i].toUtf8().c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			}
		}

		//play message - if any
		if (psViewProx->audioID != NO_AUDIO_MSG)
		{
			audio_QueueTrackPos(psViewProx->audioID, psViewProx->x, psViewProx->y, psViewProx->z);
		}
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		ASSERT_OR_RETURN(, psProxDisp->psMessage->psObj, "Invalid proxobj - null object");
		ASSERT_OR_RETURN(, psProxDisp->psMessage->psObj->type == OBJ_FEATURE, "Invalid proxobj - must be feature");
		const FEATURE *psFeature = (FEATURE *)psProxDisp->psMessage->psObj;

		if (psFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			//play default audio message for oil resource
			audio_QueueTrackPos(ID_SOUND_RESOURCE_HERE, psFeature->pos.x, psFeature->pos.y, psFeature->pos.z);
		}
		else if (psFeature->psStats->subType == FEAT_GEN_ARTE)
		{
			//play default audio message for artefact
			audio_QueueTrackPos(ID_SOUND_ARTIFACT, psFeature->pos.x, psFeature->pos.y, psFeature->pos.z);
		}
	}

	//set the read flag
	psProxDisp->psMessage->read = true;
}

void cleanupOldBeaconMessages()
{
	// Check if it's time to remove beacons
	bool removedAMessage = false;
	for (UDWORD i = 0; i < MAX_PLAYERS; i++)
	{
		/* Go through all the proximity Displays*/
		mutating_list_iterate(apsProxDisp[i], [&removedAMessage, i](PROXIMITY_DISPLAY* psProxDisp)
		{
			if (psProxDisp->psMessage->dataType == MSG_DATA_BEACON)
			{
				MESSAGE* psCurrMsg = psProxDisp->psMessage;
				VIEWDATA* pViewData = psCurrMsg->pViewData;

				ASSERT_OR_RETURN(IterationResult::CONTINUE_ITERATION, pViewData != nullptr, "Message without data!");

				if (pViewData->type == VIEW_BEACON)
				{
					ASSERT_OR_RETURN(IterationResult::CONTINUE_ITERATION, pViewData->pData != nullptr, "Help message without data!");
					if (pViewData->pData != nullptr && (((VIEW_PROXIMITY*)pViewData->pData)->timeAdded + 60000) <= gameTime)
					{
						debug(LOG_MSG, "blip timeout for %d, from %d", i, (((VIEW_PROXIMITY*)pViewData->pData)->sender));
						removeMessage(psCurrMsg, i);	//remove beacon
						removedAMessage = true;
						return IterationResult::BREAK_ITERATION; //there can only be 1 beacon per player
					}
				}
			}
			return IterationResult::CONTINUE_ITERATION;
		});
	}
	if (removedAMessage)
	{
		jsDebugMessageUpdate();
	}
}
