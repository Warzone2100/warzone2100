/*
 * Multiplay.h
 *
 *Alex Lee 1997/98, Pumpkin Studios, Bath.
 */
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
	NET_VTOLREARM,			//39 vtol rearm

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
	NET_SET_TEAMS

} MESSAGE_TYPES;

// /////////////////////////////////////////////////////////////////////////////////////////////////
// holder for gamespy info.
typedef struct {
	BOOL	bGameSpy;

} GAMESPY, *LPGAMESPY;

extern GAMESPY	gameSpy;


// /////////////////////////////////////////////////////////////////////////////////////////////////
// Game Options Structure. Enough info to completely describe the static stuff in amultiplay game.
typedef struct {
	UBYTE		type;						// DMATCH/CAMPAIGN/SKIRMISH/TEAMPLAY etc...
	STRING		map[128];					// name of multiplayer map being used.
	char		version[8];					// version of warzone
	UBYTE		maxPlayers;					// max players to allow
	STRING		name[128];					// game name   (to be used)
	BOOL		bComputerPlayers;			// Allow computer players
	BOOL		fog;
	UDWORD		power;						// power level for arena game
//	UDWORD		techLevel;					// tech levels to use . 0= all levels.
	UBYTE		base;						// clean/base/base&defence
	UBYTE		alliance;					// none/groupwin/nogroupwin.
	UBYTE		limit;						// limit no/time/frag
	UWORD		bytesPerSec;				// maximum bitrate achieved before dropping checks.
	UBYTE		packetsPerSec;				// maximum packets to send before dropping checks.
	UBYTE		encryptKey;					// key to use for encryption.
//	UBYTE		skirmishPlayers[MAX_PLAYERS];// players to use in skirmish game.
	UBYTE		skDiff[MAX_PLAYERS];			// skirmish game difficulty settings.

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

	CHAR		phrases[5][255];					// 5 favourite text messages.
} MULTIPLAYERINGAME, *LPMULTIPLAYERINGAME;


// ////////////////////////////////////////////////////////////////////////////
// Game Options and stats. 
extern MULTIPLAYERGAME		game;						// the game description.
extern MULTIPLAYERINGAME	ingame;						// the game description.

extern BOOL					bMultiPlayer;				// true when more than 1 player.
extern UDWORD				selectedPlayer;
extern DWORD				player2dpid[MAX_PLAYERS];	// note this is of type DPID, not DWORD
extern BOOL					openchannels[MAX_PLAYERS];
extern UBYTE				bDisplayMultiJoiningStatus;	// draw load progress?

// ////////////////////////////////////////////////////////////////////////////
// defines

// permissable bitrates for connection types. NOTE MUST DIFFER!
#define DEFAULTBYTESPERSEC		1000		// 1   Ks-1
#define DEFAULTPACKETS			5

#define MODEMBYTESPERSEC		1200	
#define MODEMPACKETS			8

#define INETBYTESPERSEC			1201
#define INETPACKETS				5

#define IPXBYTESPERSEC			2000	
#define IPXPACKETS				10

#define CABLEBYTESPERSEC		1202
#define CABLEPACKETS			8


#define S_WARZONEGUID "{48AB0B01-FEC0-11d1-980C-00A0243870A8}"

#define ANYPLAYER				99
#define ONEPLAYER				98

//#define DMATCH					11			// to easily distinguish game types when joining.
#define CAMPAIGN				12
#define TEAMPLAY				13

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

// ////////////////////////////////////////////////////////////////////////////
// macros For handling net messages, just copy things in & out of the msg buffer
/*
#define NetAdd(m,pos,thing) \
	memcpy(&(m.body[pos]),&(thing),sizeof(thing)) 

#define NetAdd2(m,pos,thing) \
	memcpy( &((*m).body[pos]), &(thing), sizeof(thing)) 

#define NetAddSt(m,pos,stri) \
	strcpy(&(m.body[pos]),stri)

#define NetGet(m,pos,thing) \
	memcpy(&(thing),&(m->body[pos]),sizeof(thing))

#define NetGetSt(m,pos,stri) \
	strcpy(stri,&(m->body[pos]))
*/
// ////////////////////////////////////////////////////////////////////////////
// functions

extern BASE_OBJECT		*IdToPointer(UDWORD id,UDWORD player);
extern STRUCTURE		*IdToStruct	(UDWORD id,UDWORD player);
extern BOOL				IdToDroid	(UDWORD id, UDWORD player, DROID **psDroid);
extern FEATURE			*IdToFeature(UDWORD id,UDWORD player);
extern DROID_TEMPLATE	*IdToTemplate(UDWORD tempId,UDWORD player);
extern DROID_TEMPLATE	*NameToTemplate(CHAR *sName,UDWORD player);

extern STRING *getPlayerName	(UDWORD player);
extern BOOL isHumanPlayer		(UDWORD player);				//to tell if the player is a computer or not.
extern BOOL myResponsibility	(UDWORD player);
extern BOOL responsibleFor		(UDWORD player, UDWORD playerinquestion);
extern UDWORD whosResponsible	(UDWORD player);
extern iVector cameraToHome		(UDWORD player,BOOL scroll);

extern BOOL	multiPlayerLoop		(VOID);							// for loop.c						

extern BOOL recvMessage			(VOID);	
extern BOOL sendTemplate		(DROID_TEMPLATE *t);				
extern BOOL SendDestroyTemplate (DROID_TEMPLATE *t);
extern BOOL SendResearch		(UBYTE player,UDWORD index);
extern BOOL SendDestroyFeature  (FEATURE *pF);					// send a destruct feature message.      
extern BOOL sendTextMessage		(char *pStr,BOOL cast);			// send a text message
extern BOOL sendAIMessage		(char *pStr, SDWORD player, SDWORD to);	//send AI message

extern BOOL turnOffMultiMsg		(BOOL bDoit);

extern UBYTE sendMap			(void);

extern BOOL multiplayerWinSequence(BOOL firstCall);

/////////////////////////////////////////////////////////
// definitions of functions in multiplay's other c files.

// Buildings . multistruct
extern BOOL	sendBuildStarted	(STRUCTURE *psStruct,DROID *psDroid);
extern BOOL SendDestroyStructure(STRUCTURE *s);
extern BOOL	SendBuildFinished	(STRUCTURE *psStruct);
extern BOOL sendLasSat			(UBYTE player,STRUCTURE *psStruct, BASE_OBJECT *psObj);


// droids . multibot
extern BOOL SendDroid			(DROID_TEMPLATE *pTemplate, UDWORD x, UDWORD y, UBYTE player, UDWORD id);
extern BOOL SendDestroyDroid	(DROID *d);
extern BOOL SendDemolishFinished(STRUCTURE *psS,DROID *psD);	
extern BOOL SendDroidInfo		(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, BASE_OBJECT *psObj);
extern BOOL SendDroidMove		(DROID *psDroid, UDWORD x, UDWORD y,BOOL bFormation);
extern BOOL SendGroupOrderSelected	(UBYTE player, UDWORD x, UDWORD y, BASE_OBJECT *psObj);
extern BOOL SendCmdGroup		(DROID_GROUP *psGroup, UWORD x, UWORD y, BASE_OBJECT *psObj);

extern BOOL SendGroupOrderGroup(DROID_GROUP *psGroup, DROID_ORDER order,UDWORD x,UDWORD y,BASE_OBJECT *psObj);


//extern BOOL SendDroidWaypoint	(UBYTE player, UDWORD	x, UDWORD y);
//extern BOOL SendSingleDroidWaypoint(DROID *psDroid, UDWORD x,UDWORD y);
extern BOOL sendDroidSecondary	(DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state);
extern BOOL sendDroidSecondaryAll(DROID *psDroid);
extern BOOL sendDroidEmbark     (DROID *psDroid);
extern BOOL sendDroidDisEmbark  (DROID *psDroid);
extern BOOL sendDestroyExtra	(BASE_OBJECT *psKilled,BASE_OBJECT *psKiller);
extern BOOL sendHappyVtol		(DROID *psDroid);
extern BOOL sendVtolRearm		(DROID *psDroid,STRUCTURE *psStruct, UBYTE chosen);

// Startup. mulitopt
extern BOOL multiTemplateSetup	(VOID);
extern BOOL multiInitialise		(VOID);							// for Init.c
extern BOOL lobbyInitialise		(VOID);							// for Init.c
extern BOOL multiShutdown		(VOID);	
extern BOOL sendLeavingMsg		(VOID);

extern BOOL hostCampaign		(STRING *sGame, STRING *sPlayer);
extern BOOL joinCampaign		(UDWORD gameNumber, STRING *playername);
//extern BOOL hostArena			(STRING *sGame, STRING *sPlayer);
//extern BOOL joinArena			(UDWORD gameNumber, STRING *playername);
extern VOID	playerResponding	(VOID);
extern BOOL multiGameInit		(VOID);
extern BOOL multiGameShutdown	(VOID);
extern BOOL copyTemplateSet		(UDWORD from,UDWORD to);
extern BOOL addTemplateSet		(UDWORD from,UDWORD to);
extern BOOL addTemplate			(UDWORD	player,DROID_TEMPLATE *psNew);

// syncing.
extern BOOL sendCheck			(VOID);							//send/recv  check info
extern BOOL sendScoreCheck		(VOID);							//score check only(frontend)
extern BOOL sendPowerCheck		(BOOL now);
extern BOOL sendPing			(VOID);							// allow game to request pings.
extern UDWORD averagePing		(VOID);

// multijoin
extern VOID modifyResources		(POWER_GEN_FUNCTION* psFunction);

extern BOOL sendReseachStatus	(STRUCTURE *psBuilding ,UDWORD index, UBYTE player, BOOL bStart);

extern void displayAIMessage	(STRING *pStr, SDWORD from, SDWORD to); //make AI process a message


/* for multiplayer message stack */
extern  UDWORD	msgStackPush(SDWORD CBtype, SDWORD plFrom, SDWORD plTo, STRING *tStr, SDWORD x, SDWORD y);
extern	BOOL	isMsgStackEmpty();
extern	BOOL	msgStackGetFrom(SDWORD  *psVal);
extern	BOOL	msgStackGetTo(SDWORD  *psVal);
extern	BOOL	msgStackGetMsg(STRING  *psVal);
extern	BOOL	msgStackPop();
extern	SDWORD	msgStackGetCount();
extern	void	msgStackReset(void);

extern BOOL	sendBeaconToPlayerNet(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char *pStr);
extern BOOL msgStackFireTop();