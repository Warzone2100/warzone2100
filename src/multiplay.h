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
/*
 *  Alex Lee 1997/98, Pumpkin Studios, Bath.
 */

#ifndef __INCLUDED_SRC_MULTIPLAY_H__
#define __INCLUDED_SRC_MULTIPLAY_H__

#include "group.h"

// /////////////////////////////////////////////////////////////////////////////////////////////////
// Game Options Structure. Enough info to completely describe the static stuff in amultiplay game.
typedef struct {
	uint8_t		type;						// DMATCH/CAMPAIGN/SKIRMISH/TEAMPLAY etc...
	char		map[128];					// name of multiplayer map being used.
	char		version[8];					// version of warzone
	uint8_t		maxPlayers;					// max players to allow
	char		name[128];					// game name   (to be used)
	BOOL		fog;
	uint32_t    power;						// power level for arena game
	uint8_t		base;						// clean/base/base&defence
	uint8_t		alliance;					// no/yes/AIs vs Humans
	uint8_t		skDiff[MAX_PLAYERS];			// skirmish game difficulty settings.
} MULTIPLAYERGAME;

typedef struct
{
	UBYTE		id;
	UBYTE		limit;
} MULTISTRUCTLIMITS;

// info used inside games.
typedef struct {
	UDWORD				PingTimes[MAX_PLAYERS];				// store for pings.
	BOOL				localOptionsReceived;				// used to show if we have game options yet..
	BOOL				localJoiningInProgress;				// used before we know our player number.
	BOOL				JoiningInProgress[MAX_PLAYERS];
	BOOL				bHostSetup;
	UDWORD				startTime;
	UDWORD				numStructureLimits;					// number of limits
	MULTISTRUCTLIMITS	*pStructureLimits;					// limits chunk.
	UDWORD		skScores[MAX_PLAYERS][2];			// score+kills for local skirmish players.
	char		phrases[5][255];					// 5 favourite text messages.
} MULTIPLAYERINGAME;


// ////////////////////////////////////////////////////////////////////////////
// Game Options and stats.
extern MULTIPLAYERGAME		game;						// the game description.
extern MULTIPLAYERINGAME	ingame;						// the game description.

extern BOOL					bMultiPlayer;				// true when more than 1 player.
extern UDWORD				selectedPlayer;
extern SDWORD				player2dpid[MAX_PLAYERS];	// note this is of type DPID, not DWORD
extern BOOL					openchannels[MAX_PLAYERS];
extern UBYTE				bDisplayMultiJoiningStatus;	// draw load progress?

// ////////////////////////////////////////////////////////////////////////////
// defines

// Max bit-rate, set for a 28.8KB/s modem (we have hardcore fans!)

#define MAX_BYTESPERSEC			3400

#define ANYPLAYER				99
#define ONEPLAYER				98

#define CAMPAIGN				12
#define	SKIRMISH				14
#define MULTI_SKIRMISH2			18
#define MULTI_SKIRMISH3			19

#define MULTI_CAMPAIGN2			15
#define MULTI_CAMPAIGN3			16

#define CAMP_CLEAN				0			// campaign subtypes
#define CAMP_BASE				1
#define CAMP_WALLS				2

#define DEATHMATCHTEMPLATES		4			// game templates are stored in player x.
#define CAMPAIGNTEMPLATES		5

#define PING_LO					0			// this ping is kickin'.
#define PING_MED				600			// this ping is crusin'.
#define PING_HI					1200		// this ping is crawlin'.
#define PING_LIMIT				2000		// if ping is bigger than this, then worry and panic.

#define LEV_LOW					400			// how many points to allocate for res levels???
#define LEV_MED					700
#define LEV_HI					1000

#define DIFF_SLIDER_STOPS		20			//max number of stops for the multiplayer difficulty slider

// functions

extern WZ_DECL_WARN_UNUSED_RESULT BASE_OBJECT		*IdToPointer(UDWORD id,UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT STRUCTURE		*IdToStruct(UDWORD id,UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT BOOL			IdToDroid(UDWORD id, UDWORD player, DROID **psDroid);
extern WZ_DECL_WARN_UNUSED_RESULT FEATURE		*IdToFeature(UDWORD id,UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT DROID_TEMPLATE	*IdToTemplate(UDWORD tempId,UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT DROID_TEMPLATE	*NameToTemplate(const char *sName,UDWORD player);

extern char *getPlayerName	(UDWORD player);
extern BOOL setPlayerName		(UDWORD player, const char *sName);
extern char *getPlayerColourName(SDWORD player);
extern BOOL isHumanPlayer		(UDWORD player);				//to tell if the player is a computer or not.
extern BOOL myResponsibility	(UDWORD player);
extern BOOL responsibleFor		(UDWORD player, UDWORD playerinquestion);
extern UDWORD whosResponsible	(UDWORD player);
extern Vector3i cameraToHome		(UDWORD player,BOOL scroll);
extern SDWORD dpidToPlayer		(SDWORD dpid);
extern char		playerName[MAX_PLAYERS][MAX_STR_LENGTH];	//Array to store all player names (humans and AIs)

extern BOOL	multiPlayerLoop		(void);							// for loop.c

extern BOOL recvMessage			(void);
extern BOOL sendTemplate		(DROID_TEMPLATE *t);
extern BOOL SendDestroyTemplate (DROID_TEMPLATE *t);
extern BOOL SendResearch		(UBYTE player,UDWORD index);
extern BOOL SendDestroyFeature  (FEATURE *pF);					// send a destruct feature message.
extern BOOL sendTextMessage		(const char *pStr,BOOL cast);		// send a text message
extern BOOL sendAIMessage		(char *pStr, UDWORD player, UDWORD to);	//send AI message

extern BOOL turnOffMultiMsg		(BOOL bDoit);

extern UBYTE sendMap			(void);

extern BOOL multiplayerWinSequence(BOOL firstCall);

/////////////////////////////////////////////////////////
// definitions of functions in multiplay's other c files.

// Buildings . multistruct
extern BOOL	sendBuildStarted	(STRUCTURE *psStruct, DROID *psDroid);
extern BOOL SendDestroyStructure(STRUCTURE *s);
extern BOOL	SendBuildFinished	(STRUCTURE *psStruct);
extern BOOL sendLasSat			(UBYTE player, STRUCTURE *psStruct, BASE_OBJECT *psObj);


// droids . multibot
extern BOOL SendDroid			(const DROID_TEMPLATE* pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id);
extern BOOL SendDestroyDroid	(const DROID* psDroid);
extern BOOL SendDemolishFinished(STRUCTURE *psS,DROID *psD);
extern BOOL SendDroidInfo		(const DROID* psDroid, DROID_ORDER order, uint32_t x, uint32_t y, const BASE_OBJECT* psObj);
extern BOOL SendDroidMove		(const DROID* psDroid, uint32_t x, uint32_t y, BOOL formation);
extern BOOL SendGroupOrderSelected(uint8_t player, uint32_t x, uint32_t y, const BASE_OBJECT* psObj);
extern BOOL SendCmdGroup		(DROID_GROUP *psGroup, UWORD x, UWORD y, BASE_OBJECT *psObj);

extern BOOL SendGroupOrderGroup(const DROID_GROUP* psGroup, DROID_ORDER order, uint32_t x, uint32_t y, const BASE_OBJECT* psObj);


extern BOOL sendDroidSecondary	(const DROID* psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state);
extern BOOL sendDroidSecondaryAll(const DROID* psDroid);
extern BOOL sendDroidEmbark     (const DROID* psDroid);
extern BOOL sendDroidDisEmbark  (const DROID* psDroid);
extern BOOL sendHappyVtol		(const DROID* psDroid);

// Startup. mulitopt
extern BOOL multiTemplateSetup	(void);
extern BOOL multiShutdown		(void);
extern BOOL sendLeavingMsg		(void);

extern BOOL hostCampaign		(char *sGame, char *sPlayer);
extern BOOL joinCampaign		(UDWORD gameNumber, char *playername);
extern void	playerResponding	(void);
extern BOOL multiGameInit		(void);
extern BOOL multiGameShutdown	(void);
extern BOOL copyTemplateSet		(UDWORD from,UDWORD to);
extern BOOL addTemplateSet		(UDWORD from,UDWORD to);
extern BOOL addTemplate			(UDWORD	player,DROID_TEMPLATE *psNew);

// syncing.
extern BOOL sendCheck			(void);							//send/recv  check info
extern BOOL sendScoreCheck		(void);							//score check only(frontend)
extern BOOL sendPing			(void);							// allow game to request pings.

// multijoin
extern void modifyResources		(POWER_GEN_FUNCTION* psFunction);

extern BOOL sendReseachStatus	(STRUCTURE *psBuilding ,UDWORD index, UBYTE player, BOOL bStart);

extern void displayAIMessage	(char *pStr, SDWORD from, SDWORD to); //make AI process a message


/* for multiplayer message stack */
extern  UDWORD	msgStackPush(SDWORD CBtype, SDWORD plFrom, SDWORD plTo, const char *tStr, SDWORD x, SDWORD y, DROID *psDroid);
extern	BOOL	isMsgStackEmpty(void);
extern	BOOL	msgStackGetFrom(SDWORD  *psVal);
extern	BOOL	msgStackGetTo(SDWORD  *psVal);
extern	BOOL	msgStackGetMsg(char  *psVal);
extern	BOOL	msgStackPop(void);
extern	SDWORD	msgStackGetCount(void);
extern	void	msgStackReset(void);
extern BOOL msgStackGetDroid(DROID **ppsDroid);

extern BOOL sendBeacon(int32_t locX, int32_t locY, int32_t forPlayer, int32_t sender, const char* pStr);
extern BOOL msgStackFireTop(void);

#endif // __INCLUDED_SRC_MULTIPLAY_H__
