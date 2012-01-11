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
/*
 *  Alex Lee 1997/98, Pumpkin Studios, Bath.
 */

#ifndef __INCLUDED_SRC_MULTIPLAY_H__
#define __INCLUDED_SRC_MULTIPLAY_H__

#include "featuredef.h"
#include "group.h"
#include "featuredef.h"
#include "droid.h"  // For INITIAL_DROID_ORDERS.

// /////////////////////////////////////////////////////////////////////////////////////////////////
// Game Options Structure. Enough info to completely describe the static stuff in a multiplayer game.
// Also used for skirmish games. And sometimes for campaign. Should be moved somewhere else.
struct MULTIPLAYERGAME
{
	uint8_t		type;						// DMATCH/CAMPAIGN/SKIRMISH/TEAMPLAY etc...
	bool		scavengers;					// whether scavengers are on or off
	char		map[128];					// name of multiplayer map being used.
	uint8_t		maxPlayers;					// max players to allow
	char		name[128];					// game name   (to be used)
	uint32_t    power;						// power level for arena game
	uint8_t		base;						// clean/base/base&defence
	uint8_t		alliance;					// no/yes/AIs vs Humans
	uint8_t		skDiff[MAX_PLAYERS];		// skirmish game difficulty settings. 0x0=OFF 0xff=HUMAN
	bool		mapHasScavengers;
};

struct MULTISTRUCTLIMITS
{
	uint32_t        id;
	uint32_t        limit;
};

// info used inside games.
struct MULTIPLAYERINGAME
{
	UDWORD				PingTimes[MAX_PLAYERS];				// store for pings.
	bool				localOptionsReceived;				// used to show if we have game options yet..
	bool				localJoiningInProgress;				// used before we know our player number.
	bool				JoiningInProgress[MAX_PLAYERS];
	bool				DataIntegrity[MAX_PLAYERS];
	bool				bHostSetup;
	int32_t				TimeEveryoneIsInGame;
	bool				isAllPlayersDataOK;
	UDWORD				startTime;
	UDWORD				numStructureLimits;					// number of limits
	MULTISTRUCTLIMITS	*pStructureLimits;					// limits chunk.
	uint8_t				flags;  ///< Bitmask, shows which structures are disabled.
	uint8_t				SPcolor;	//
	UDWORD		skScores[MAX_PLAYERS][2];			// score+kills for local skirmish players.
	char		phrases[5][255];					// 5 favourite text messages.
};

enum STRUCTURE_INFO
{
	STRUCTUREINFO_MANUFACTURE,
	STRUCTUREINFO_CANCELPRODUCTION,
	STRUCTUREINFO_HOLDPRODUCTION,
	STRUCTUREINFO_RELEASEPRODUCTION,
	STRUCTUREINFO_HOLDRESEARCH,
	STRUCTUREINFO_RELEASERESEARCH
};

struct PACKAGED_CHECK
{
	uint32_t gameTime;  ///< Game time that this synch check was made. Not touched by NETauto().
	uint8_t player;
	uint32_t droidID;
	int32_t order;
	uint32_t secondaryOrder;
	uint32_t body;
	uint32_t experience;
	Position pos;
	Rotation rot;
	uint32_t targetID;  ///< Defined iff order == DORDER_ATTACK.
	uint16_t orderX;    ///< Defined iff order == DORDER_MOVE.
	uint16_t orderY;    ///< Defined iff order == DORDER_MOVE.
};

// ////////////////////////////////////////////////////////////////////////////
// Game Options and stats.
extern MULTIPLAYERGAME		game;						// the game description.
extern MULTIPLAYERINGAME	ingame;						// the game description.

extern bool					bMultiPlayer;				// true when more than 1 player.
extern bool					bMultiMessages;				// == bMultiPlayer unless multi messages are disabled
extern bool					openchannels[MAX_PLAYERS];
extern UBYTE				bDisplayMultiJoiningStatus;	// draw load progress?

// ////////////////////////////////////////////////////////////////////////////
// defines

// NOTE: MaxMsgSize is currently set to 16K.  When MAX_BYTESPERSEC has been reached (sent + recv!), then we do NOT
//       do the sync code checks anymore(!), needless to say, this can and does cause issues.
// FIXME: We should define this externally so people with dial-up modems can configure this
// FIXME: Use possible compression on the packets.
// NOTE: Remember, we (now) allow 450 units max * 7 (1 human, 6 AI possible for Host) to send to the other player.

#define MAX_BYTESPERSEC			14336

// Bitmask for client lobby

#define NO_VTOLS  1
#define NO_TANKS 2
#define NO_BORGS 4


#define ANYPLAYER				99

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

//#define PING_LO                               0                       // this ping is kickin'.
#define PING_MED				200			// this ping is crawlin'
#define PING_HI					400			// this ping just plain sucks :P
#define PING_LIMIT				1000		// if ping is bigger than this, then worry and panic.

#define LEV_LOW					400			// how many points to allocate for res levels???
#define LEV_MED					700
#define LEV_HI					1000

#define DIFF_SLIDER_STOPS		20			//max number of stops for the multiplayer difficulty slider

#define MAX_KICK_REASON			80			// max array size for the reason your kicking someone
// functions

extern WZ_DECL_WARN_UNUSED_RESULT BASE_OBJECT		*IdToPointer(UDWORD id,UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT STRUCTURE		*IdToStruct(UDWORD id,UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT DROID			*IdToDroid(UDWORD id, UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT FEATURE		*IdToFeature(UDWORD id,UDWORD player);
extern WZ_DECL_WARN_UNUSED_RESULT DROID_TEMPLATE	*IdToTemplate(UDWORD tempId,UDWORD player);

extern const char* getPlayerName(int player);
extern bool setPlayerName(int player, const char *sName);
extern const char* getPlayerColourName(int player);
extern bool isHumanPlayer(int player);				//to tell if the player is a computer or not.
extern bool myResponsibility(int player);
extern bool responsibleFor(int player, int playerinquestion);
extern int whosResponsible(int player);
int scavengerSlot();    // Returns the player number that scavengers would have if they were enabled.
int scavengerPlayer();  // Returns the player number that the scavengers have, or -1 if disabled.
extern Vector3i cameraToHome		(UDWORD player,bool scroll);
extern char		playerName[MAX_PLAYERS][MAX_STR_LENGTH];	//Array to store all player names (humans and AIs)

extern bool	multiPlayerLoop		(void);							// for loop.c

extern bool recvMessage			(void);
bool sendTemplate(uint32_t player, DROID_TEMPLATE *t);
extern bool SendDestroyTemplate(DROID_TEMPLATE *t, uint8_t player);
extern bool SendResearch(uint8_t player, uint32_t index, bool trigger);
extern bool SendDestroyFeature  (FEATURE *pF);					// send a destruct feature message.
extern bool sendTextMessage		(const char *pStr,bool cast);		// send a text message
extern bool sendAIMessage		(char *pStr, UDWORD player, UDWORD to);	//send AI message
void printConsoleNameChange(const char *oldName, const char *newName);  ///< Print message to console saying a name changed.

extern void turnOffMultiMsg		(bool bDoit);

extern void sendMap(void);
extern bool multiplayerWinSequence(bool firstCall);

/////////////////////////////////////////////////////////
// definitions of functions in multiplay's other c files.

// Buildings . multistruct
extern bool SendDestroyStructure(STRUCTURE *s);
extern bool	SendBuildFinished	(STRUCTURE *psStruct);
extern bool sendLasSat			(UBYTE player, STRUCTURE *psStruct, BASE_OBJECT *psObj);
void sendStructureInfo                  (STRUCTURE *psStruct, STRUCTURE_INFO structureInfo, DROID_TEMPLATE *psTempl);

// droids . multibot
extern bool SendDroid                   (const DROID_TEMPLATE* pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id, const INITIAL_DROID_ORDERS *initialOrders);
extern bool SendDestroyDroid	(const DROID* psDroid);
extern bool SendDemolishFinished(STRUCTURE *psS,DROID *psD);
void sendQueuedDroidInfo(void);  ///< Actually sends the droid orders which were queued by SendDroidInfo.
void sendDroidInfo(DROID *psDroid, DroidOrder const &order, bool add);
extern bool SendCmdGroup		(DROID_GROUP *psGroup, UWORD x, UWORD y, BASE_OBJECT *psObj);


extern bool sendDroidSecondary	(const DROID* psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state);
extern bool sendDroidEmbark     (const DROID* psDroid, const DROID* psTransporter);
bool sendDroidDisembark(DROID const *psTransporter, DROID const *psDroid);

// Startup. mulitopt
extern bool multiTemplateSetup	(void);
extern bool multiShutdown		(void);
extern bool sendLeavingMsg		(void);

extern bool hostCampaign		(char *sGame, char *sPlayer);
extern bool joinGame			(const char* host, uint32_t port);
extern void	playerResponding	(void);
extern bool multiGameInit		(void);
extern bool multiGameShutdown	(void);
extern bool addTemplate			(UDWORD	player,DROID_TEMPLATE *psNew);
extern bool addTemplateToList(DROID_TEMPLATE *psNew, DROID_TEMPLATE **ppList);
void addTemplateBack(unsigned player, DROID_TEMPLATE *psNew);

// syncing.
void sendCheck();  //send/recv  check info
extern bool sendScoreCheck		(void);							//score check only(frontend)
extern bool sendPing			(void);							// allow game to request pings.

extern bool ForceDroidSync(const DROID *droidToSend);
// multijoin
extern bool sendResearchStatus  (STRUCTURE *psBuilding, UDWORD index, UBYTE player, bool bStart);

extern void displayAIMessage	(char *pStr, SDWORD from, SDWORD to); //make AI process a message


/* for multiplayer message stack */
extern  UDWORD	msgStackPush(SDWORD CBtype, SDWORD plFrom, SDWORD plTo, const char *tStr, SDWORD x, SDWORD y, DROID *psDroid);
extern	bool	isMsgStackEmpty(void);
extern	bool	msgStackGetFrom(SDWORD  *psVal);
extern	bool	msgStackGetTo(SDWORD  *psVal);
extern	bool	msgStackGetMsg(char  *psVal);
extern	bool	msgStackPop(void);
extern	SDWORD	msgStackGetCount(void);
extern	void	msgStackReset(void);
extern bool msgStackGetDroid(DROID **ppsDroid);

extern bool sendBeacon(int32_t locX, int32_t locY, int32_t forPlayer, int32_t sender, const char* pStr);
extern bool msgStackFireTop(void);

extern	bool multiplayPlayersReady	(bool bNotifyStatus);
extern	void startMultiplayerGame	(void);
extern	void resetReadyStatus		(bool bSendOptions);

extern	bool bPlayerReadyGUI[MAX_PLAYERS];

#endif // __INCLUDED_SRC_MULTIPLAY_H__
