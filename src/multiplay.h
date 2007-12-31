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
 * Multiplay.h
 *
 *Alex Lee 1997/98, Pumpkin Studios, Bath.
 */

#ifndef _multiplay_h
#define _multiplay_h
#include "group.h"

// Different Message Structures allowed to be sent between players.
// Each message must have type = to one of these.
typedef enum _msgtype
{
	NET_DROID,				//0 a new droid
	NET_DROIDINFO,			//1 update a droid order.
	NET_DROIDDEST,			//2 issue a droid destruction
	NET_DROIDMOVE,			//3 move a droid, don't change anything else though..
	NET_GROUPORDER,			//4 order a group of droids.
	NET_TEMPLATE,			//5 a new template
	NET_TEMPLATEDEST,		//6 remove template
	NET_FEATUREDEST,		//7 destroy a game feature.
	NET_PING,				//8 ping players.
	NET_CHECK_DROID,		//9 check & update bot position and damage.
	NET_CHECK_STRUCT,		//10 check & update struct damage.
	NET_CHECK_POWER,		//11 power levels for a player.
	NET_VERSION,			//12 VERSION data
	NET_BUILD,				//13 build a new structure
	NET_STRUCTDEST,			//14 specify a strucutre to destroy
	NET_BUILDFINISHED,		//15 a building is complete.
	NET_RESEARCH,			//16 Research has been completed.
	NET_TEXTMSG,			//17 A simple text message between machines.
	NET_LEAVING,			//18 A player is leaving, (nicely)
	NET_REQUESTDROID,		//19 a message has been recvd for an unknown droid, request the droid to resolve issues.

	// JOINING TYPES. these msgs are used when a player joins a game in progress.
	NET_PLAYERCOMPLETE,		//20 All Setup information about player x has been sent
	NET_REQUESTPLAYER,		//21 NOTUSED please send me info about a player
	NET_STRUCT,				//22 a complete structure
	NET_WHOLEDROID,			//23 a complete droid
	NET_FEATURES,			//24 information regarding features.
	NET_PLAYERRESPONDING,	//25 computer that sent this is now playing warzone!

	// RECENT TYPES
	NET_OPTIONS,			//26 welcome a player to a game.
	NET_KICK,				//27 kick a player .
	NET_SECONDARY,			//28 set a droids secondary order

	NET_FIREUP,				//29 campaign game has started, we can go too.. Shortcut message, not to be used in dmatch.
	NET_ALLIANCE,			//30 alliance data.
	NET_GIFT,				//31 a luvly gift between players.
	NET_DEMOLISH,			//32 a demolish is complete.
	NET_COLOURREQUEST,		//33 player requests a colour change.
	NET_ARTIFACTS,			//34 artifacts randomly placed.
	NET_DMATCHWIN,			//35 winner of a deathmatch. NOTUSED
	NET_SCORESUBMIT,		//36 submission of scores to host.
	NET_DESTROYXTRA,		//37 destroy droid with destroyer intact.
	NET_VTOL,				//38 vtol rearmed
	NET_UNUSED_39,			//39 unused

	NET_WHITEBOARD,			//40 whiteboard.
    NET_SECONDARY_ALL,      //41 complete secondary order.
    NET_DROIDEMBARK,        //42 droid embarked on a Transporter
    NET_DROIDDISEMBARK,     //43 droid disembarked from a Transporter

	NET_RESEARCHSTATUS,		//44 105, research state.
	NET_LASSAT,				//45 107, lassat firing.

	NET_REQUESTMAP,			//46 107 dont have map, please send it.
	NET_AITEXTMSG,			//chat between AIs
	NET_TEAMS_ON,
	NET_BEACONMSG,
	NET_SET_TEAMS,
	NET_TEAMREQUEST
} MESSAGE_TYPES;


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
//	uint32_t    techLevel;					// tech levels to use . 0= all levels.
	uint8_t		base;						// clean/base/base&defence
	uint8_t		alliance;					// no/yes/AIs vs Humans
	uint8_t		limit;						// limit no/time/frag
	uint16_t    bytesPerSec;				// maximum bitrate achieved before dropping checks.
	uint8_t		packetsPerSec;				// maximum packets to send before dropping checks.
	uint8_t		encryptKey;					// key to use for encryption.
//	uint8_t		skirmishPlayers[MAX_PLAYERS];// players to use in skirmish game.
	uint8_t		skDiff[MAX_PLAYERS];			// skirmish game difficulty settings.

} MULTIPLAYERGAME, *LPMULTIPLAYERGAME;

// info used inside games.
typedef struct {
	UDWORD		PingTimes[MAX_PLAYERS];				// store for pings.
	BOOL		localOptionsReceived;				// used to show if we have game options yet..
	BOOL		localJoiningInProgress;				// used before we know our player number.
	BOOL		JoiningInProgress[MAX_PLAYERS];
	BOOL		bHostSetup;
	UDWORD		startTime;
	UDWORD		modem;								// modem to use.
	UDWORD		numStructureLimits;					// number of limits
	UBYTE		*pStructureLimits;					// limits chunk.

	UDWORD		skScores[MAX_PLAYERS][2];			// score+kills for local skirmish players.

	char		phrases[5][255];					// 5 favourite text messages.
} MULTIPLAYERINGAME, *LPMULTIPLAYERINGAME;


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

// Max bit-rate/packet count, set for a 14.4KB modem (we have hardcore fans!)

#define MAX_BYTESPERSEC			1200
#define MAX_PACKETSPERSEC		6

#define ANYPLAYER				99
#define ONEPLAYER				98

//#define DMATCH					11			// to easily distinguish game types when joining.
#define CAMPAIGN				12
//#define TEAMPLAY				13

#define	SKIRMISH				14
#define MULTI_SKIRMISH2			18
#define MULTI_SKIRMISH3			19
//#define MULTI_SKIRMISHA			20

#define MULTI_CAMPAIGN2			15
#define MULTI_CAMPAIGN3			16
//#define MULTI_CAMPAIGNA			17


#define NOLIMIT					0			// limit options for dmatch.
#define FRAGLIMIT				1
#define TIMELIMIT				2

#define CAMP_CLEAN				0			// campaign subtypes
#define CAMP_BASE				1
#define CAMP_WALLS				2

#define	FORCEEDITPLAYER			0
#define DEATHMATCHTEMPLATES		4			// game templates are stored in player x.
#define CAMPAIGNTEMPLATES		5


#define PING_LO					0			// this ping is kickin'.
#define PING_MED				600			// this ping is crusin'.
#define PING_HI					1200		// this ping is crawlin'.
#define PING_LIMIT				2000		// if ping is bigger than this, then worry and panic.

#define LEV_LOW					100			// how many points to allocate for res levels???
#define LEV_MED					400
#define LEV_HI					700

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
extern char		playerName[MAX_PLAYERS][MAX_NAME_SIZE];	//Array to store all player names (humans and AIs)

extern BOOL	multiPlayerLoop		(void);							// for loop.c

extern BOOL recvMessage			(void);
extern BOOL sendTemplate		(DROID_TEMPLATE *t);
extern BOOL SendDestroyTemplate (DROID_TEMPLATE *t);
extern BOOL SendResearch		(UBYTE player,UDWORD index);
extern BOOL SendDestroyFeature  (FEATURE *pF);					// send a destruct feature message.
extern BOOL sendTextMessage		(const char *pStr,BOOL cast);		// send a text message
extern BOOL sendAIMessage		(char *pStr, SDWORD player, SDWORD to);	//send AI message

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
extern BOOL sendDestroyExtra	(BASE_OBJECT *psKilled,BASE_OBJECT *psKiller);
extern BOOL sendHappyVtol		(const DROID* psDroid);

// Startup. mulitopt
extern BOOL multiTemplateSetup	(void);
extern BOOL multiInitialise		(void);							// for Init.c
extern BOOL lobbyInitialise		(void);							// for Init.c
extern BOOL multiShutdown		(void);
extern BOOL sendLeavingMsg		(void);

extern BOOL hostCampaign		(char *sGame, char *sPlayer);
extern BOOL joinCampaign		(UDWORD gameNumber, char *playername);
//extern BOOL hostArena			(char *sGame, char *sPlayer);
//extern BOOL joinArena			(UDWORD gameNumber, char *playername);
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

extern BOOL	sendBeaconToPlayerNet(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char *pStr);
extern BOOL msgStackFireTop(void);

#endif
