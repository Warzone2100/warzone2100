/*
 * Multiplay.c
 *
 * Alex Lee, Sep97, Pumpkin Studios
 *
 * Contains the day to day networking stuff, and received message handler.
 */

#include "lib/framework/frame.h"
#include "map.h"

#include "stats.h"									// for templates.
#include "game.h"									// for loading maps
#include "hci.h"

#include <time.h>									// for recording ping times.
#include "research.h"
#include "display3d.h"								// for changing the viewpoint
#include "console.h"								// for screen messages
#include "power.h"
#include "text.h"
#include "cmddroid.h"								//  for commanddroidupdatekills
#include "wrappers.h"								// for game over
#include "component.h"
#include "frontend.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "levels.h"
#include "selection.h"

#include "init.h"
#include "warcam.h"	// these 4 for fireworks
#include "effects.h"
#include "lib/gamelib/gtime.h"
#include "keybind.h"

#include "lib/script/script.h"				//Because of "ScriptTabs.h"
#include "scripttabs.h"			//because of CALL_AI_MSG
#include "scriptcb.h"			//for console callback

#include "lib/netplay/netplay.h"								// the netplay library.
#include "multiplay.h"								// warzone net stuff.
#include "multijoin.h"								// player management stuff.
#include "multirecv.h"								// incoming messages stuff
#include "multistat.h"
#include "multigifts.h"								// gifts and alliances.

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// globals.
BOOL						bMultiPlayer				= FALSE;	// true when more than 1 player.
STRING						sForceName[256]				= "Default";
DWORD						player2dpid[MAX_PLAYERS]	={0,0,0,0,0,0,0,0};		//stores dpids of each player. FILTHY HACK (ASSUMES 8playerS)
//UDWORD						arenaPlayersReceived=0;
BOOL						openchannels[MAX_PLAYERS]={TRUE};
UBYTE						bDisplayMultiJoiningStatus;

GAMESPY						gameSpy;

MULTIPLAYERGAME				game;									//info to describe game.
MULTIPLAYERINGAME			ingame;

BOOL						bSendingMap					= FALSE;	// map broadcasting.

STRING						tempString[12];
STRING						beaconReceiveMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];	//beacon msg for each player
BOOL						recvBeacon(NETMSG *pMsg);

/////////////////////////////////////
/* multiplayer message stack stuff */
/////////////////////////////////////
#define MAX_MSG_STACK	100
#define MAX_STR			255

char msgStr[MAX_MSG_STACK][MAX_STR];

SDWORD msgPlFrom[MAX_MSG_STACK];
SDWORD msgPlTo[MAX_MSG_STACK];

SDWORD callbackType[MAX_MSG_STACK];
SDWORD locx[MAX_MSG_STACK];
SDWORD locy[MAX_MSG_STACK];

SDWORD msgStackPos = -1;				//top element pointer

// ////////////////////////////////////////////////////////////////////////////
// Remote Prototypes
extern BOOL MultiPlayValidTemplate(DROID_TEMPLATE *psTempl);	// for templates.

extern RESEARCH*			asResearch;							//list of possible research items.
extern PLAYER_RESEARCH*		asPlayerResList[MAX_PLAYERS];

// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

BOOL turnOffMultiMsg(BOOL bDoit);

BOOL		multiPlayerLoop(VOID);
BOOL		IdToDroid	(UDWORD id, UDWORD player, DROID **psDroid);
STRUCTURE	*IdToStruct	(UDWORD id,UDWORD player);
BASE_OBJECT *IdToPointer(UDWORD id,UDWORD player);
FEATURE		*IdToFeature(UDWORD id,UDWORD player);
DROID_TEMPLATE *IdToTemplate(UDWORD tempId,UDWORD player);
DROID_TEMPLATE *NameToTemplate(CHAR *sName,UDWORD player);

STRING *getPlayerName		(UDWORD player);
BOOL	isHumanPlayer		(UDWORD player);				// determine if human
BOOL	myResponsibility	(UDWORD player);				// this pc has comms responsibility
BOOL	responsibleFor		(UDWORD player,UDWORD player2);	// has player responsibility for player2
UDWORD	whosResponsible		(UDWORD player);				// returns player responsible for 'player'
iVector	cameraToHome		(UDWORD player,BOOL scroll);
BOOL	DirectPlaySystemMessageHandler(LPVOID msg);			// interpret DP messages
BOOL	recvMessage			(VOID);							// process an incoming message
BOOL	SendResearch		(UBYTE player,UDWORD index);	// send/recv Research issues
BOOL	recvResearch		(NETMSG *pMsg);
BOOL	sendTextMessage		(char *pStr,BOOL bcast);		// send/recv a text message

BOOL	sendAIMessage		(char *pStr, SDWORD player, SDWORD to);	//send AI message
void	displayAIMessage	(STRING *pStr, SDWORD from, SDWORD to);
BOOL	recvTextMessageAI	(NETMSG *pMsg);					//AI multiplayer message

BOOL	recvTextMessage		(NETMSG *pMsg);
BOOL	sendTemplate		(DROID_TEMPLATE *t);			// send/recv Template information
BOOL	recvTemplate		(NETMSG *pMsg);
BOOL	SendDestroyTemplate	(DROID_TEMPLATE *t);			// send/recv Template destruction info
BOOL	recvDestroyTemplate	(NETMSG *pMsg);
BOOL	SendDestroyFeature	(FEATURE *pF);					// send a destruct feature message.
BOOL	recvDestroyFeature	(NETMSG *m);					// process a destroy feature msg.
BOOL	sendDestroyExtra	(BASE_OBJECT *psKilled,BASE_OBJECT *psKiller);
BOOL	recvDestroyExtra	(NETMSG *pMsg);
BOOL	recvAudioMsg		(NETMSG *pMsg);

BOOL	recvMapFileRequested	(NETMSG *pMsg);
UBYTE	sendMap				(void);
BOOL	recvMapFileData			(NETMSG *pMsg);
BOOL	addHelpBlip(SDWORD x, SDWORD y, SDWORD forPlayer, SDWORD sender, STRING * textMsg);


// ////////////////////////////////////////////////////////////////////////////
// temporarily disable multiplayer mode.
BOOL turnOffMultiMsg(BOOL bDoit)
{
	static BOOL bTemp;

	if(bDoit)	// turn off msgs.
	{
		if(bTemp == TRUE)
		{
			debug( LOG_NEVER, "\nturnoffmultimsg: multiple calls to turn off msging.\n" );
		}
		if(bMultiPlayer)
		{
			bMultiPlayer = FALSE;
			bTemp = TRUE;
		}
	}
	else	// turn on msgs.
	{
		if(bTemp)
		{
			bMultiPlayer = TRUE;
			bTemp = FALSE;
		}
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// throw a pary when you win!
BOOL multiplayerWinSequence(BOOL firstCall)
{
	static		iVector pos;
	iVector		pos2;
	static UDWORD last=0;
	FRACT		fraction;
	FRACT		rotAmount;
	STRUCTURE	*psStruct;

	if(firstCall)
	{
		pos  = cameraToHome(selectedPlayer,TRUE);			// pan the camera to home if not already doing so
		last =0;

		// stop all research
		CancelAllResearch(selectedPlayer);

		// stop all manufacture.
		for(psStruct=apsStructLists[selectedPlayer];psStruct;psStruct = psStruct->psNext)
		{
			if (StructIsFactory(psStruct))
			{
				if (((FACTORY *)psStruct->pFunctionality)->psSubject)//check if active
				{
					cancelProduction(psStruct);
				}
			}
		}
	}

	// rotate world
	if(!getWarCamStatus())
	{
		fraction = MAKEFRACT(frameTime)/GAME_TICKS_PER_SEC;
		rotAmount = fraction * MAP_SPIN_RATE/12;
		player.r.y += MAKEINT(rotAmount);
	}

	if(last > gameTime)last= 0;
	if((gameTime-last) < 500 )							// only  if not done recently.
	{
		return TRUE;
	}
	last = gameTime;

	if(rand()%3 == 0)
	{
		pos2=pos;
		pos2.x +=  (rand()%(8<<TILE_SHIFT))-(4<<TILE_SHIFT);
		pos2.z +=  (rand()%(8<<TILE_SHIFT))-(4<<TILE_SHIFT);
		if(pos2.x <0) pos2.x =128;
		if(pos2.x >mapWidth<<TILE_SHIFT) pos2.x =mapWidth<<TILE_SHIFT;
		if(pos2.z <0) pos2.z =128;
		if(pos2.z >mapHeight<<TILE_SHIFT) pos2.z =mapHeight<<TILE_SHIFT;

		addEffect(&pos2,EFFECT_FIREWORK,FIREWORK_TYPE_LAUNCHER,FALSE,NULL,0);	// throw up some fire works.
	}

	// show the score..


	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// MultiPlayer main game loop code.
BOOL multiPlayerLoop(VOID)
{
	UDWORD		i;
	UBYTE		joinCount;

	sendCheck();						// send some checking info if possible
	processMultiPlayerArtifacts();		// process artifacts

//	if( (game.type == DMATCH) && !ingame.localJoiningInProgress)
//	{
//		deathmatchCheck();
//	}

//	if (game.type != DMATCH)
//	{
		joinCount =0;
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(isHumanPlayer(i) && ingame.JoiningInProgress[i] )
			{
				joinCount++;
			}
		}
		if(joinCount)
		{
			setWidgetsStatus(FALSE);
			bDisplayMultiJoiningStatus = joinCount;	// someone is still joining! say So

			// deselect anything selected.
			selDroidDeselect(selectedPlayer);

			if(keyPressed(KEY_ESC) )// check for cancel
			{
				bDisplayMultiJoiningStatus = 0;
				setWidgetsStatus(TRUE);
				setPlayerHasLost(TRUE);
			}
		}
		else		//everyone is in the game now!
		{
			if(bDisplayMultiJoiningStatus)
			{
				sendVersionCheck();
				bDisplayMultiJoiningStatus = 0;
				setWidgetsStatus(TRUE);
			}
		}
//	}

	// process network audio
	if(game.bytesPerSec==IPXBYTESPERSEC)
	{
		NETprocessAudioCapture();		// manage the capture buffer
	}

	recvMessage();						// get queued messages


	// if player has won then process the win effects...
	if(testPlayerHasWon())
	{
		multiplayerWinSequence(FALSE);
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// quikie functions.

// to get droids ...
BOOL IdToDroid(UDWORD id, UDWORD player, DROID **psDroid)
{
	UDWORD i;
	DROID *d;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)			// find the droid to order form them all
		{
			d = apsDroidLists[i];
			while((d != NULL )&&(d->id !=id) )d=d->psNext;
			if(d)
			{
				*psDroid = d;
				return TRUE;
			}
		}
		return FALSE;
	}
	else									// find the droid, given player
	{
		d = apsDroidLists[player];
		while( (d != NULL ) && (d->id !=id))d=d->psNext;
		if(d)
		{
			*psDroid = d;
			return TRUE;
		}
		return FALSE;
	}
}

// ////////////////////////////////////////////////////////////////////////////
// find a structure
STRUCTURE *IdToStruct(UDWORD id,UDWORD player)
{
	STRUCTURE	*psStr = NULL;
	UDWORD		i;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for(psStr=apsStructLists[i];( (psStr != NULL) && (psStr->id != id)); psStr=psStr->psNext);
			if(psStr)
			{
				return psStr;
			}
		}
	}
	else
	{
		for(psStr=apsStructLists[player];((psStr != NULL )&&(psStr->id != id) );psStr=psStr->psNext);
	}
	return psStr;
}

// ////////////////////////////////////////////////////////////////////////////
// find a feature
FEATURE *IdToFeature(UDWORD id,UDWORD player)
{
	FEATURE	*psF =NULL;
	UDWORD	i;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for(psF=apsFeatureLists[i];( (psF != NULL) && (psF->id != id)); psF=psF->psNext);
			if(psF)
			{
				return psF;
			}
		}
	}
	else
	{
		for(psF=apsFeatureLists[player];((psF != NULL )&&(psF->id != id) );psF=psF->psNext);
	}
	return psF;
}

// ////////////////////////////////////////////////////////////////////////////

DROID_TEMPLATE *IdToTemplate(UDWORD tempId,UDWORD player)
{
	DROID_TEMPLATE *psTempl = NULL;
	UDWORD		i;
	if(player != ANYPLAYER)
	{
		for (psTempl = apsDroidTemplates[player];			// follow templates
		(psTempl && (psTempl->multiPlayerID != tempId ));
		 psTempl = psTempl->psNext);
	}
	else
	{
		// REALLY DANGEROUS!!! ID's are NOT assumed to be unique for TEMPLATES.
		debug( LOG_NEVER, "Really Dodgy Check performed for a template" );
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for (psTempl = apsDroidTemplates[i];			// follow templates
			(psTempl && (psTempl->multiPlayerID != tempId ));
			 psTempl = psTempl->psNext);
			if(psTempl)
			{
				return psTempl;
			}
		}
	}
	return psTempl;
}

// the same as above, but only checks names in similarity.
DROID_TEMPLATE *NameToTemplate(CHAR *sName,UDWORD player)
{
	DROID_TEMPLATE *psTempl = NULL;

	for (psTempl = apsDroidTemplates[player];			// follow templates
		(psTempl && (strcmp(psTempl->aName,sName) != 0) );
		 psTempl = psTempl->psNext);

	 return psTempl;
}
/////////////////////////////////////////////////////////////////////////////////
//  Returns a pointer to base object, given an id and optionally a player.
BASE_OBJECT *IdToPointer(UDWORD id,UDWORD player)
{
	DROID		*pD;
	STRUCTURE	*pS;
	FEATURE		*pF;
	// droids.
	if (IdToDroid(id,player,&pD))
	{
		return (BASE_OBJECT*)pD;
	}

	// structures
	pS = IdToStruct(id,player);
	if(pS)
	{
		return (BASE_OBJECT*)pS;
	}

	// features
	pF = IdToFeature(id,player);
	if(pF)
	{
		return (BASE_OBJECT*)pF;
	}

	return NULL;
}


// ////////////////////////////////////////////////////////////////////////////
// return a players name.
STRING *getPlayerName(UDWORD player)
{
	UDWORD i;
//	STRING tempString[2];
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(player2dpid[player] == NetPlay.players[i].dpid)
		{
			if(strcmp(NetPlay.players[i].name,"") == 0)
			{
				// make up a name for this player.
				switch(getPlayerColour(player))
				{
				case 0:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_GREEN));
					break;
				case 1:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_ORANGE));
					break;
				case 2:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_GREY));
					break;
				case 3:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_BLACK));
					break;
				case 4:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_RED));
					break;
				case 5:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_BLUE));
					break;
				case 6:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_PINK));
					break;
				case 7:
					strcpy(tempString,strresGetString(psStringRes, STR_FE_CYAN));
					break;
				}
//				sprintf(tempString,"%d",player);
				return tempString;
			}

			return (STRING*)&NetPlay.players[i].name;
		}
	}

	return NetPlay.players[0].name;
}

// ////////////////////////////////////////////////////////////////////////////
// to determine human/computer players and responsibilities of each..
BOOL isHumanPlayer(UDWORD player)
{
	BOOL val = (player2dpid[player] != 0);
	return val;
}

// returns player responsible for 'player'
UDWORD  whosResponsible(UDWORD player)
{
	UDWORD c;
	SDWORD i;

    c = ANYPLAYER;
	if (isHumanPlayer(player))
	{
		c = player;
	}

	else if(player == selectedPlayer)
	{
		c = player;
	}

	else
	{
		// crawl down array to find a responsible fellow,
		for(i=player; i>=0; i--)
		{
			if(isHumanPlayer(i))
			{
				c = i;
			}
		}
		// else crawl up to find a responsible fellow
		if(c == ANYPLAYER)
		{
			for(i=player; i<MAX_PLAYERS; i++)
			{
				if(isHumanPlayer(i))
				{
					c = i;
				}
			}
		}

	}
	if(c == ANYPLAYER)
	{
		debug( LOG_NEVER, "failed to find a player for %d \n", player );
	}
	return c;
}

//returns true if selected player is responsible for 'player'
BOOL myResponsibility(UDWORD player)
{
	if(whosResponsible(player) == selectedPlayer)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//returns true if 'player' is responsible for 'playerinquestion'
BOOL responsibleFor(UDWORD player, UDWORD playerinquestion)
{

	if(whosResponsible(playerinquestion) == player)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// ////////////////////////////////////////////////////////////////////////////
// probably temporary. Places the camera on the players 1st droid or struct.
iVector cameraToHome(UDWORD player,BOOL scroll)
{
	iVector res;
	UDWORD x,y;
	STRUCTURE	*psBuilding;

	for (psBuilding = apsStructLists[player];
		 psBuilding && (psBuilding->pStructureType->type != REF_HQ);
		 psBuilding= psBuilding->psNext);

	if(psBuilding)
	{
		x= (psBuilding->x  >> TILE_SHIFT);
		y= (psBuilding->y  >> TILE_SHIFT);
	}
	else if (apsDroidLists[player])				// or first droid
	{
		 x= (apsDroidLists[player]->x >> TILE_SHIFT);
		 y=	(apsDroidLists[player]->y >> TILE_SHIFT);
	}
	else if (apsStructLists[player])							// center on first struct
	{
		x= (apsStructLists[player]->x  >> TILE_SHIFT);
		y= (apsStructLists[player]->y  >> TILE_SHIFT);
	}
	else														//or map center.
	{
		x= mapWidth/2;
		y= mapHeight/2;
	}


	if(scroll)
	{
		requestRadarTrack(x<<TILE_SHIFT,y<<TILE_SHIFT);
	}
	else
	{
		setViewPos(x,y,TRUE);
	}

	res.x = x<<TILE_SHIFT;
	res.y = map_TileHeight(x,y);
	res.z = y<<TILE_SHIFT;
	return res;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Required by the net library. It's the system message handler..
BOOL DirectPlaySystemMessageHandler(LPVOID mg)
{

	return (TRUE);
}



// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Recv Messages. Get a message and dispatch to relevant function.
BOOL recvMessage(VOID)
{
	NETMSG msg;
	DPID dp;
	UDWORD a;

	while(NETrecv(&msg) == TRUE)			// for all incoming messages.
	{
		// messages only in game.
		if(!ingame.localJoiningInProgress)
		{
			switch(msg.type)
			{
			case AUDIOMSG:
				recvAudioMsg(&msg);
			break;

			case NET_DROID:						// new droid of known type
				recvDroid(&msg);
				break;
			case NET_DROIDINFO:					//droid update info
				recvDroidInfo(&msg);
				break;
			case NET_DROIDDEST:					// droid destroy
				recvDestroyDroid(&msg);
				break;
			case NET_DESTROYXTRA:
				recvDestroyExtra(&msg);			// a generic destroy, complete wiht killer info.
				break;
			case NET_DROIDMOVE:					// move a droid to x,y command.
				recvDroidMove(&msg);
				break;
			case NET_GROUPORDER:				// an order for more than 1 droid.
				recvGroupOrder(&msg);
				break;
			case NET_CHECK_DROID:				// droid damage and position checks
				recvDroidCheck(&msg);
				break;
			case NET_CHECK_STRUCT:				// structure damage checks.
				recvStructureCheck(&msg);
				break;
			case NET_CHECK_POWER:				// Power level syncing.
				recvPowerCheck(&msg);
				break;
			case NET_TEXTMSG:					// simple text message
				recvTextMessage(&msg);
				break;

			case NET_AITEXTMSG:					//multiplayer AI text message
				recvTextMessageAI(&msg);
				break;
			case NET_BEACONMSG:					//beacon (blip) message
				recvBeacon(&msg);
				break;

			case NET_BUILD:						// a build order has been sent.
				recvBuildStarted(&msg);
				break;
			case NET_BUILDFINISHED:				// a building is complete
				recvBuildFinished(&msg);
				break;
			case NET_STRUCTDEST:				// structure destroy
				recvDestroyStructure(&msg);
				break;

//			case NET_WAYPOINT:					// add waypoint to droids.
//				recvDroidWaypoint(&msg);
//				break;
			case NET_SECONDARY:					// set a droids secondary order level.
				recvDroidSecondary(&msg);
				break;
            case NET_SECONDARY_ALL:					// set a droids secondary order level.
				recvDroidSecondaryAll(&msg);
				break;
            case NET_DROIDEMBARK:
                recvDroidEmbark(&msg);              //droid has embarked on a Transporter
                break;
            case NET_DROIDDISEMBARK:
                recvDroidDisEmbark(&msg);           //droid has disembarked from a Transporter
                break;
			case NET_REQUESTDROID:				// player requires a droid that they dont have.
				recvRequestDroid(&msg);
				break;
//			case NET_REQUESTPLAYER:				// a new player requires information
//				multiPlayerRequest(&msg);
//				break;
			case NET_GIFT:						// an alliance gift from one player to another.
				recvGift(&msg);
				break;
			case NET_SCORESUBMIT:				//  a score update from another player
				recvScoreSubmission(&msg);
				break;
//			case NET_DMATCHWIN:					// someone has won!.
//				recvdMatchWinner(&msg);
//				break;
			case NET_VTOL:
				recvHappyVtol(&msg);
				break;
			case NET_VTOLREARM:
				recvVtolRearm(&msg);
				break;
			case NET_LASSAT:
				recvLasSat(&msg);
				break;

			default:
				break;
			}
		}
/*
		// messages only during setup.
		if(ingame.localJoiningInProgress)
		{
			switch(msg.type)
			{
			case NET_PLAYERCOMPLETE:			// don't care about this!
				arenaPlayersReceived++;

				if (arenaPlayersReceived == MAX_PLAYERS)
				{
					ProcessDroidOrders();						// set droid orders....
					bMultiPlayer = TRUE;						// reinit mulitplay messaging

					useTheForce(FALSE);							// use the force.
					playerResponding();							// say hi.
					addDMatchDroid(1);							// each player adds a newdroid point.
				}
				else
				{
					// request the next player
					NetAdd(msg,0,NetPlay.dpidPlayer);				//our id
					NetAdd(msg,4,arenaPlayersReceived);				//player we require.
					msg.size = 8;
					msg.type = NET_REQUESTPLAYER;
					NETbcast(msg,TRUE);
				}
				break;

			case NET_FEATURES:								// feature set
				recvFeatures(&msg);							// **biggest message of them all!**
				break;

//			case NET_STRUCT:								// structure set.
//				receiveWholeStructure(&msg);
//				break;

			default:
				break;
			}
		}
*/
		// messages usable all the time
		switch(msg.type)
		{
		case NET_TEMPLATE:					// new template
			recvTemplate(&msg);
			break;
		case NET_TEMPLATEDEST:				// template destroy
			recvDestroyTemplate(&msg);
			break;
		case NET_FEATUREDEST:				// feature destroy
			recvDestroyFeature(&msg);
			break;
		case NET_PING:						// diagnostic ping msg.
			recvPing(&msg);
			break;
		case NET_DEMOLISH:					// structure demolished.
			recvDemolishFinished(&msg);
			break;
		case NET_RESEARCH:					// some research has been done.
			recvResearch(&msg);
			break;
		case NET_LEAVING:					// player leaving nicely
			NetGet((&msg),0,dp);
			MultiPlayerLeave(dp);
			break;
		case NET_WHOLEDROID:				// a complete droid description has arrived.
			receiveWholeDroid(&msg);
			break;
		case NET_OPTIONS:
			recvOptions(&msg);
			break;
		case NET_VERSION:
			recvVersionCheck(&msg);
			break;
		case NET_PLAYERRESPONDING:			// remote player is now playing
			NetGet((&msg),0,a);
			ingame.JoiningInProgress[a] = FALSE;	// player is with us!
			break;
		case NET_COLOURREQUEST:
			recvColourRequest(&msg);
			break;
		case NET_ARTIFACTS:
			recvMultiPlayerRandomArtifacts(&msg);
			break;
		case NET_ALLIANCE:
			recvAlliance(&msg,TRUE);
			break;
		case NET_KICK:
			NetGet((&msg),0,dp);
			if(NetPlay.dpidPlayer == dp)	// we've been told to leave.
			{
				setPlayerHasLost(TRUE);
			}
			break;

		case NET_FIREUP:				// frontend only
			break;

		case NET_RESEARCHSTATUS:
			recvResearchStatus(&msg);
			break;

		default:
//			NetGet((&msg),4,a);
//			DBPRINTF(("wierdy message arrived, type:%d size:%d msg:%d ",msg.type,msg.size,a));
//			return FALSE;
			break;
		}
	}

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Research Stuff. Nat games only send the result of research procedures.

BOOL SendResearch(UBYTE player,UDWORD index)
{
	NETMSG m;
	UBYTE i;
	PLAYER_RESEARCH *pPlayerRes;


	NetAdd(m,0,player);						// player researching
	NetAdd(m,1,index);								// reference into topic.

	m.size =5;
	m.type = NET_RESEARCH;

	// teamplay games share research....

	if(game.type == TEAMPLAY || game.type == SKIRMISH)
	{
		pPlayerRes = asPlayerResList[player];
		pPlayerRes += index;
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(alliances[i][player] == ALLIANCE_FORMED)
			{
				pPlayerRes = asPlayerResList[i];
				pPlayerRes += index;
				if(IsResearchCompleted(pPlayerRes)==FALSE)
				{
					MakeResearchCompleted(pPlayerRes);		// do the research for that player
					researchResult(index, i,FALSE);
				}
			}
		}
	}

	return( NETbcast(&m,FALSE) );
}

// recv a research topic that is now complete.
BOOL recvResearch(NETMSG *m)
{
	UBYTE			player,i;
	UDWORD			index;
	PLAYER_RESEARCH *pPlayerRes;
	RESEARCH		*pResearch;

	player = m->body[0];
	NetGet(m,1,index);								// get the index

	pPlayerRes = asPlayerResList[player];
	pPlayerRes += index;
	if(IsResearchCompleted(pPlayerRes)==FALSE)
	{
		MakeResearchCompleted(pPlayerRes);
		researchResult(index, player,FALSE);

		//take off the power if available.
		pResearch = asResearch + index;
		usePower(player, pResearch->researchPower);
	}

	// teamplay games share research....
	if(game.type == TEAMPLAY || game.type == SKIRMISH)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(alliances[i][player] == ALLIANCE_FORMED)
			{
				pPlayerRes = asPlayerResList[i];
				pPlayerRes += index;
				if(IsResearchCompleted(pPlayerRes)==FALSE)
				{
					MakeResearchCompleted(pPlayerRes);		// do the research for that player
					researchResult(index, i,FALSE);
				}
			}
		}
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// New research stuff, so you can see what others are up to!

// inform others, I'm researching this.

BOOL sendReseachStatus(STRUCTURE *psBuilding ,UDWORD index, UBYTE player, BOOL bStart)
{
	NETMSG m;
	UDWORD nil =0;
	UWORD start = (UBYTE)bStart;
	if(!myResponsibility(player) || gameTime <5 )
	{
		return TRUE;
	}

	NetAdd(m,0,player);				// player researching
	NetAdd(m,1,start);	// start stop..
	if(psBuilding)
	{
		NetAdd(m,2,psBuilding->id);	// res lab.
	}
	else
	{
		NetAdd(m,2,nil);			// res lab.
	}
	NetAdd(m,6,index);				// topic.

	m.size = 10;
	m.type = NET_RESEARCHSTATUS;

	return( NETbcast(&m,FALSE) );
}


BOOL recvResearchStatus(NETMSG *pMsg)
{
	STRUCTURE			*psBuilding;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH_FACILITY	*psResFacilty;
	RESEARCH			*pResearch;
	UBYTE				player,bStart;
	UDWORD				index,buildingId;

	NetGet(pMsg,0,player);				// player researching
	NetGet(pMsg,1,bStart);				// start stop..
	NetGet(pMsg,2,buildingId);			// res lab.
	NetGet(pMsg,6,index);				// topic.

	pPlayerRes = asPlayerResList[player] + index;

	// psBuilding may be null if finishing.
	if(bStart)							//starting research
	{
		psBuilding = IdToStruct( buildingId, player);
		if(psBuilding)					// set that facility to research
		{

			psResFacilty =	(RESEARCH_FACILITY*)psBuilding->pFunctionality;
			if(psResFacilty->psSubject != NULL)
			{
				cancelResearch(psBuilding);
			}
			pResearch				= (asResearch+index);
			psResFacilty->psSubject = (BASE_STATS*)pResearch;		  //set the subject up

			if (IsResearchCancelled(pPlayerRes))
			{
				psResFacilty->powerAccrued	= pResearch->researchPower;//set up as if all power available for cancelled topics
			}
			else
			{
				psResFacilty->powerAccrued	= 0;
			}

			MakeResearchStarted(pPlayerRes);
			psResFacilty->timeStarted		= ACTION_START_TIME;
			psResFacilty->timeStartHold		= 0;
			psResFacilty->timeToResearch	= pResearch->researchPoints / 	psResFacilty->researchPoints;
			if (psResFacilty->timeToResearch == 0)
			{
				psResFacilty->timeToResearch = 1;
			}
		}

	}
	else								// finished/cancelled research
	{
		if(buildingId == 0)				// find the centre doing this research.(set building
		{
			// go through the structs to find the centre that's doing this topic)
			for(psBuilding = apsStructLists[player];psBuilding;psBuilding = psBuilding->psNext)
			{
				if(   psBuilding->pStructureType->type == REF_RESEARCH
				   && psBuilding->status == SS_BUILT
				   && ((RESEARCH_FACILITY *)psBuilding->pFunctionality)->psSubject
				   && ((RESEARCH_FACILITY *)psBuilding->pFunctionality)->psSubject->ref - REF_RESEARCH_START == index
				  )
				{
					break;
				}
			}
		}
		else
		{
			psBuilding = IdToStruct( buildingId, player);
		}

		if(IsResearchCompleted(pPlayerRes))
		{
			return TRUE;
		}

		// stop that facility doing any research.
		if(psBuilding)
		{
			cancelResearch(psBuilding);
		}
	}
	return TRUE;

}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Text Messaging between players. proceed string with players to send to.
// eg "123 hi there" sends "hi there" to players 1,2 and 3.
BOOL sendTextMessage(char *pStr,BOOL all)
{
	NETMSG	m;
	BOOL	normal = TRUE;
	BOOL	sendto[MAX_PLAYERS];
	UDWORD	i;
	CHAR	display[MAX_CONSOLE_STRING_LENGTH];
	BOOL	bEncrypting;



	if(!ingame.localOptionsReceived)
	{
		return TRUE;
	}

	bEncrypting = NetPlay.bEncryptAllPackets;
	NetPlay.bEncryptAllPackets = FALSE;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		sendto[i]=0;
	}

	for(i=0; ((pStr[i] >= '0' ) && (pStr[i] <= '8' )) ; i++ )		// for each numeric char encountered..
	{
		sendto[ pStr[i]-'0' ] = TRUE;
		normal = FALSE;
	}

	NetAdd(m,0,NetPlay.dpidPlayer);
	memcpy(&(m.body[4]),&(pStr[i]), strlen( &(pStr[i]) )+1);		// copy message in.
	m.size = (UWORD)( strlen( &(pStr[i]) )+5);						// package the message up and send it.
	m.type = NET_TEXTMSG;

	if(all)
	{
		NETbcast(&m,FALSE);
	}
	else if(normal)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(openchannels[i])
			{
				if(isHumanPlayer(i) )
				{
					NETsend(&m,player2dpid[i],FALSE);
				}
				else	//also send to AIs now (non-humans), needed for AI
				{
					sendAIMessage(&(m.body[4]), selectedPlayer, i);
				}
			}
		}
	}
	else
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(sendto[i])
			{
				if(isHumanPlayer(i))
				{
					NETsend(&m,player2dpid[i],FALSE);
				}
				else	//also send to AIs now (non-humans), needed for AI
				{
					sendAIMessage(&(m.body[4]), selectedPlayer, i);
				}
			}
		}
	}

	NetPlay.bEncryptAllPackets = bEncrypting;

	for(i = 0; NetPlay.players[i].dpid != NetPlay.dpidPlayer; i++);	//findplayer
	strcpy(display,NetPlay.players[i].name);						// name
	strcat(display," : ");											// seperator
	strcat(display,pStr);											// add message
	addConsoleMessage(display,DEFAULT_JUSTIFY);						// display

	return TRUE;
}

//AI multiplayer message, send from a certain player index to another player index
BOOL sendAIMessage(char *pStr, SDWORD player, SDWORD to)
{
	NETMSG	m;
	SDWORD	sendPlayer;
	BOOL	bEncrypting;

	//check if this is one of the local players, don't need net send then
	if(to == selectedPlayer || myResponsibility(to))	//(the only) human on this machine or AI on this machine
	{
		//Just show him the message
		displayAIMessage(pStr, player, to);

		//Received a console message from a player callback
		//store and call later
		//-------------------------------------------------
		if(!msgStackPush(CALL_AI_MSG,player,to,pStr,-1,-1))
		{
			debug(LOG_ERROR, "sendAIMessage() - msgStackPush - stack failed");
			return FALSE;
		}
	}
	else		//not a local player (use multiplayer mode)
	{
		if(!ingame.localOptionsReceived)
		{
			return TRUE;
		}

		bEncrypting = NetPlay.bEncryptAllPackets;
		NetPlay.bEncryptAllPackets = FALSE;

		NetAdd(m,0,player);			//save the actual sender

		//save the actual player that is to get this msg on the source machine (source can host many AIs)
		NetAdd(m,4,to);			//save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)

		memcpy(&(m.body[8]),&(pStr[0]), strlen( &(pStr[0]) )+1);		// copy message in.

		m.size = (UWORD)( strlen( &(pStr[0]) )+9);						// package the message up and send it.
		m.type = NET_AITEXTMSG;		//new type

		//find machine that is hosting this human or AI
		sendPlayer = whosResponsible(to);

		if(sendPlayer >= MAX_PLAYERS)
		{
			debug(LOG_ERROR, "sendAIMessage() - sendPlayer >= MAX_PLAYERS");
			return FALSE;
		}

		if(!isHumanPlayer(sendPlayer))		//NETsend can't send to non-humans
		{
			debug(LOG_ERROR, "sendAIMessage() - player is not human.");
			return FALSE;
		}

		NETsend(&m,player2dpid[sendPlayer],FALSE);		//send to the player who is hosting 'to' player (might be himself if human and not AI)

		NetPlay.bEncryptAllPackets = bEncrypting;
	}

	return TRUE;
}

BOOL sendBeaconToPlayerNet(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char *pStr)
{
	NETMSG	m;
	SDWORD	sendPlayer;
	BOOL	bEncrypting;

	//debug(LOG_WZ, "sendBeaconToPlayerNet: '%s'",pStr);

	bEncrypting = NetPlay.bEncryptAllPackets;
	NetPlay.bEncryptAllPackets = FALSE;

	NetAdd(m,0,sender);			//save the actual sender

	//save the actual player that is to get this msg on the source machine (source can host many AIs)
	NetAdd(m,4,forPlayer);			//save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)

	//save location
	NetAdd(m,8,locX);
	NetAdd(m,12,locY);

	memcpy(&(m.body[16]),&(pStr[0]), strlen( &(pStr[0]) )+1);		// copy message in.

	m.size = (UWORD)( strlen( &(pStr[0]) )+17);						// package the message up and send it.
	m.type = NET_BEACONMSG;		//new type

	//find machine that is hosting this human or AI
	sendPlayer = whosResponsible(forPlayer);

	if(sendPlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "sendAIMessage() - whosResponsible() failed.");
		return FALSE;
	}

	NETsend(&m,player2dpid[sendPlayer],FALSE);		//send to the player who is hosting 'to' player (might be himself if human and not AI)

	NetPlay.bEncryptAllPackets = bEncrypting;

	return TRUE;
}

void displayAIMessage(STRING *pStr, SDWORD from, SDWORD to)
{
	STRING tmp[255];

	if(isHumanPlayer(to))		//display text only if receiver is the (human) host machine itself
	{
		//addConsoleMessage(pStr,DEFAULT_JUSTIFY);
		//console("%d: %s", from, pStr);
		sprintf(tmp,"%d",from);
		strcat(tmp," : ");											// seperator
		strcat(tmp,pStr);											// add message
		addConsoleMessage(tmp,DEFAULT_JUSTIFY);
	}
}

// Write a message to the console.
BOOL recvTextMessage(NETMSG *pMsg)
{
	DPID	dpid;
	UDWORD	i;
	STRING	msg[MAX_CONSOLE_STRING_LENGTH];
	UDWORD  player,j;		//console callback - player who sent the message

	NetGet(pMsg,0,dpid);
	for(i = 0; NetPlay.players[i].dpid != dpid; i++);		//findplayer

	//console callback - find real number of the player
	for(j = 0; i<MAX_PLAYERS;j++)
	{
		if(dpid == player2dpid[j])
		{
			player = j;
			break;
		}
	}

	//sprintf(msg, "%d", i);
	strcpy(msg,NetPlay.players[i].name);
	strcat(msg," : ");								// seperator
	//strcat(msg, &(pMsg->body[4]));					// add message
	strncat(msg, &(pMsg->body[4]), MAX_CONSOLE_STRING_LENGTH);					// add message
	addConsoleMessage((char *)&msg,DEFAULT_JUSTIFY);

	//multiplayer message callback
	//Received a console message from a player, save
	//----------------------------------------------
	MultiMsgPlayerFrom = player;
	MultiMsgPlayerTo = selectedPlayer;

	strcpy(MultiplayMsg,&(pMsg->body[4]));
	eventFireCallbackTrigger(CALL_AI_MSG);

	// make some noise!
	if(titleMode == MULTIOPTION || titleMode == MULTILIMIT)
	{
		audio_PlayTrack(FE_AUDIO_MESSAGEEND);
	}
	else if(!ingame.localJoiningInProgress)
	{
		audio_PlayTrack(ID_SOUND_MESSAGEEND);
	}

	return TRUE;
}

//AI multiplayer message - received message from AI (from scripts)
BOOL recvTextMessageAI(NETMSG *pMsg)
{
	SDWORD	sender, receiver;

	STRING	msg[MAX_CONSOLE_STRING_LENGTH];

	NetGet(pMsg,0,sender);
	NetGet(pMsg,4,receiver);

	strcpy(msg, &(pMsg->body[8]));
	strcat(msg,NetPlay.players[sender].name);		// name

	//Display the message and make the script callback
	displayAIMessage(msg, sender, receiver);

	//Received a console message from a player callback
	//store and call later
	//-------------------------------------------------
	if(!msgStackPush(CALL_AI_MSG,sender,receiver,msg,-1,-1))
	{
		debug(LOG_ERROR, "recvTextMessageAI() - msgStackPush - stack failed");
		return FALSE;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Templates

// send a newly created template to other players
BOOL sendTemplate(DROID_TEMPLATE *pTempl)
{
	NETMSG m;

	if(pTempl == NULL)
	{
#ifdef DEBUG
		// FIXME sendTemplate: TELL ALEXL NOW!!!THIS IS THE BUG THAT ISNT FIXED!!!
		debug( LOG_ERROR, "sendTemplate: TELL ALEXL NOW!!!THIS IS THE BUG THAT ISNT FIXED!!!" );
		abort();
#endif
		return TRUE;
	}

	m.body[0] = (UBYTE)selectedPlayer;						//player to attach template to
	memcpy(&(m.body[1]),pTempl,	sizeof(DROID_TEMPLATE));			//the template itself
	m.size = (UWORD)(sizeof(DROID_TEMPLATE)+1);
	m.type = NET_TEMPLATE;
	return(  NETbcast(&m,FALSE)	);
}

// receive a template created by another player
BOOL recvTemplate(NETMSG * m)
{
	UBYTE			player;
	DROID_TEMPLATE	*psTempl;
	DROID_TEMPLATE	t;

	player = (UBYTE)(m->body[0]);

	ASSERT( player < MAX_PLAYERS, "recvtemplate: invalid player size: %d", player );

	if(m->size < sizeof(DROID_TEMPLATE))
	{
#ifdef DEBUG
		// FIXME recvTemplate: invalid template recvd. THIS IS THE BUG THAT ISNT FIXED!!!
		debug( LOG_ERROR, "recvTemplate: invalid template recvd. THIS IS THE BUG THAT ISNT FIXED!!!" );
		abort();
#endif
		return TRUE;
	}

	memcpy(&t,&(m->body[1]),sizeof(DROID_TEMPLATE));

	psTempl = IdToTemplate(t.multiPlayerID,player);
	if(psTempl)												// already exists.
	{
		t.psNext = psTempl->psNext;
		memcpy(psTempl, &t, sizeof(DROID_TEMPLATE));
	}
	else
	{
		addTemplate(player,&t);
		apsDroidTemplates[player]->ref = REF_TEMPLATE_START;	// templates are the odd one out!

/*		if (!HEAP_ALLOC(psTemplateHeap, &psTempl))
		{
			return FALSE;
		}
		memcpy(psTempl, &t, sizeof(DROID_TEMPLATE));
		psTempl->ref = REF_TEMPLATE_START;					// templates are the odd one out!
		if (apsDroidTemplates[player])						// Add it to the list
		{
			for(psCurr = apsDroidTemplates[player];
				psCurr->psNext != NULL;
				psCurr = psCurr->psNext
				);

			psCurr->psNext = psTempl;
			psTempl->psNext = NULL;
		}
		else
		{
			apsDroidTemplates[player]=psTempl;
			psTempl->psNext = NULL;
		}
*/
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// inform others that you no longer have a template

BOOL SendDestroyTemplate(DROID_TEMPLATE *t)
{
	NETMSG m;

	m.body[0] = (char) selectedPlayer;			// send player number

	// send id of template to destroy
	NetAdd(m,1,(t->multiPlayerID));
	m.size = 5;

	m.type=NET_TEMPLATEDEST;

	return( NETbcast(&m,FALSE)	);
}

// acknowledge another player no longer has a template
BOOL recvDestroyTemplate(NETMSG * m)
{
	UDWORD			player,targetref;
	DROID_TEMPLATE	*psTempl, *psTempPrev;

	player	= m->body[0];								// decode the message
	NetGet(m,1,targetref);

	psTempPrev = NULL;									// first find it.
	for(psTempl = apsDroidTemplates[player]; psTempl;psTempl = psTempl->psNext)
	{
		if( psTempl->multiPlayerID == targetref )
		{
			break;
		}
		psTempPrev = psTempl;
	}

	if (psTempl)										// if we found itthen delete it.
	{
		if(psTempPrev)									// Update list pointers.
		{
			psTempPrev->psNext = psTempl->psNext;		// It's down the list somewhere ?
		} else
		{
			apsDroidTemplates[player] = psTempl->psNext;// It's at the root ?
		}
		HEAP_FREE(psTemplateHeap, psTempl);									// Delete the template.
	}
	return (TRUE);
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Features

// send a destruct feature message.
BOOL SendDestroyFeature(FEATURE *pF)
{
	NETMSG m;

	// only send the destruction if the feature is our responsibility
	NetAdd(m,0,pF->id);
	m.size = sizeof(pF->id);
	m.type = NET_FEATUREDEST;

	return( NETbcast(&m,TRUE) );
}

// process a destroy feature msg.
BOOL recvDestroyFeature(NETMSG *pMsg)
{
	FEATURE *pF;
	UDWORD	id;

	NetGet(pMsg,0,id);														// get feature id

	//	for(pF = apsFeatureLists[0]; pF && (pF->id  != id);	pF = pF->psNext);	// find the feature
	pF = IdToFeature(id,ANYPLAYER);

	if(  (pF == NULL)  )													// if already a gonner.
	{
		return FALSE;														// feature wasnt found anyway.
	}

	bMultiPlayer = FALSE;													// remove, don't broadcast.
	removeFeature(pF);
	bMultiPlayer = TRUE;

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// a generic destroy function, with killer info included.
BOOL sendDestroyExtra(BASE_OBJECT *psKilled,BASE_OBJECT *psKiller)
{
	NETMSG		m;
	UDWORD		n=0;

/*	if(psKilled != NULL)
	{
		NetAdd(m,4,psKilled->id);	// id of thing killed
	}
	else
	{
		NetAdd(m,4,n);
	}
*/
	if(psKiller != NULL)
	{
		NetAdd(m,0,psKiller->id);	// id of killer.
	}
	else
	{
		NetAdd(m,0,n);
	}


	m.type = NET_DESTROYXTRA;
	m.size = 4;

	return 	NETbcast(&m,FALSE);
}

// ////////////////////////////////////////////////////////////////////////////
BOOL recvDestroyExtra(NETMSG *pMsg)
{
//	BASE_OBJECT	*psKilled;
//	UDWORD		 killedId;

	BASE_OBJECT	*psSrc;
	UDWORD		srcId;

	DROID		*psKiller;
/*
	NetGet(pMsg,0,killedId);					// remove as normal
	if(killedId !=0)
	{
		psKilled = IdToPointer(killedId,ANYPLAYER);
		if(psKilled)
		{
			switch(psKilled->type)
			{
			case OBJ_DROID:
				recvDestroyDroid(pMsg);
				break;
			case OBJ_STRUCTURE:
				recvDestroyStructure(pMsg);
				break;
			case OBJ_FEATURE:
				recvDestroyFeature(pMsg);
				break;
			}
		}
	}
*/
	NetGet(pMsg,0,srcId);
	if(srcId != 0)
	{
		psSrc = IdToPointer(srcId,ANYPLAYER);

		if(psSrc && (psSrc->type == OBJ_DROID) )		// process extra bits.
		{
			psKiller = 	(DROID*)psSrc;
			if(psKiller)
			{
				psKiller->numKills++;
			}
			cmdDroidUpdateKills(psKiller);
			return TRUE;
		}
	}
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////////
// Network Audio packet processor.
BOOL recvAudioMsg(NETMSG *pMsg)
{
	NETplayIncomingAudio(pMsg);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Network File packet processor.
BOOL recvMapFileRequested(NETMSG *pMsg)
{
	char mapStr[256],mapName[256],fixedname[256];

//	pMsg;

	// another player is requesting the map
	if(!NetPlay.bHost)
	{
		return TRUE;
	}

//#ifndef DISABLEMAPSEND
	// start sending the map to the other players.
	if(!bSendingMap)
	{
		memset(mapStr,0,256);
		memset(mapName,0,256);
		memset(fixedname,0,256);
		bSendingMap = TRUE;
		addConsoleMessage("SENDING MAP!",DEFAULT_JUSTIFY);
		addConsoleMessage("FIX FOR LINUX!",DEFAULT_JUSTIFY);

		strcpy(mapName,game.map);
		// chop off the -T1
		mapName[strlen(game.map)-3] = 0;		// chop off the -T1 etc..

		// chop off the sk- if required.
		if(strncmp(mapName,"Sk-",3) == 0)
		{
			strcpy(mapStr,&(mapName[3]));
			strcpy(mapName,mapStr);
		}

		sprintf(mapStr,"%dc-%s",game.maxPlayers,mapName);
		strcat(mapStr,".wz");//.wdg
		sprintf(fixedname,"maps\\%s",mapStr);		//We know maps are in /maps dir...now. fix for linux -Q
		memcpy(mapStr,fixedname,256);
		NETsendFile(TRUE,mapStr,0);
	}
//#endif
	return TRUE;
}

// continue sending the map
UBYTE sendMap(void)
{
	UBYTE done;
	static UDWORD lastCall;


	if(lastCall > gameTime)lastCall= 0;
	if ( (gameTime - lastCall) <200)
	{
		return 0;
	}
	lastCall = gameTime;


	done = NETsendFile(FALSE,game.map,0);

	if(done == 100)
	{
		addConsoleMessage("MAP SENT!",DEFAULT_JUSTIFY);
		bSendingMap = FALSE;
	}

	return done;
}

// another player is broadcasting a map, recv a chunk
BOOL recvMapFileData(NETMSG *pMsg)
{
	UBYTE done;

	done =  NETrecvFile(pMsg);
	if(done == 100)
	{
		addConsoleMessage("MAP DOWNLOADED!",DEFAULT_JUSTIFY);
		sendTextMessage("MAP DOWNLOADED",TRUE);					//send

		// clear out the old level list.
		levShutDown();
		levInitialise();
		if (!buildMapList()) {
			return FALSE;
		}
	}
	else
	{
		flushConsoleMessages();
		CONPRINTF(ConsoleString,(ConsoleString,"MAP:%d%%",done));
	}

	return TRUE;
}


//------------------------------------------------------------------------------------------------//

/* multiplayer message stack */
void msgStackReset(void)
{
	msgStackPos = -1;		//Beginning of the stack
}

UDWORD msgStackPush(SDWORD CBtype, SDWORD plFrom, SDWORD plTo, STRING *tStr, SDWORD x, SDWORD y)
{
	if (msgStackPos >= MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPush() - stack full");
		return FALSE;
	}

	//make point to the last valid element
	msgStackPos++;

	//remember values
	msgPlFrom[msgStackPos] = plFrom;
	msgPlTo[msgStackPos] = plTo;

	callbackType[msgStackPos] = CBtype;
	locx[msgStackPos] = x;
	locy[msgStackPos] = y;

	strcpy(msgStr[msgStackPos], tStr);

	return TRUE;
}

BOOL isMsgStackEmpty()
{
	if(msgStackPos == (-1)) return TRUE;
	return FALSE;
}

BOOL msgStackGetFrom(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetFrom: msgStackPos < 0");
		return FALSE;
	}

	*psVal = msgPlFrom[0];

	return TRUE;
}

BOOL msgStackGetTo(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetTo: msgStackPos < 0");
		return FALSE;
	}

	*psVal = msgPlTo[0];

	return TRUE;
}

BOOL msgStackGetCallbackType(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetCallbackType: msgStackPos < 0");
		return FALSE;
	}

	*psVal = callbackType[0];

	return TRUE;
}

BOOL msgStackGetXY(SDWORD  *psValx, SDWORD  *psValy)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetXY: msgStackPos < 0");
		return FALSE;
	}

	*psValx = locx[0];
	*psValy = locy[0];

	return TRUE;
}


BOOL msgStackGetMsg(STRING  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetMsg: msgStackPos < 0");
		return FALSE;
	}

	strcpy(psVal, msgStr[0]);
	//*psVal = msgPlTo[msgStackPos];

	return TRUE;
}

BOOL msgStackSort()
{
	SDWORD i;

	//go through all-1 elements (bottom-top)
	for(i=0;i<msgStackPos;i++)
	{
		msgPlFrom[i] = msgPlFrom[i+1];
		msgPlTo[i] = msgPlTo[i+1];

		callbackType[i] = callbackType[i+1];
		locx[i] = locx[i+1];
		locy[i] = locy[i+1];

		strcpy(msgStr[i], msgStr[i+1]);
	}

	//erase top element
	msgPlFrom[msgStackPos] = -2;
	msgPlTo[msgStackPos] = -2;

	callbackType[msgStackPos] = -2;
	locx[msgStackPos] = -2;
	locy[msgStackPos] = -2;

	strcpy(msgStr[msgStackPos], "ERROR STRING!!!!!!!!");

	msgStackPos--;		//since removed the top element

	return TRUE;
}

BOOL msgStackPop()
{
	if(msgStackPos < 0 || msgStackPos >= MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPop: wrong msgStackPos index");
		return FALSE;
	}

	return msgStackSort();		//move all elements 1 pos lower
}

SDWORD msgStackGetCount()
{
	return msgStackPos + 1;
}

BOOL msgStackFireTop()
{
	SDWORD		_callbackType;
	STRING		msg[255];

	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackFireTop: msgStackPos < 0");
		return FALSE;
	}

	if(!msgStackGetCallbackType(&_callbackType))
		return FALSE;

	switch(_callbackType)
	{
		case CALL_VIDEO_QUIT:
			debug(LOG_SCRIPT, "msgStackFireTop: popped CALL_VIDEO_QUIT");
			eventFireCallbackTrigger(CALL_VIDEO_QUIT);
			break;
		
		case CALL_BEACON:

			if(!msgStackGetXY(&beaconX, &beaconY))
				return FALSE;

			if(!msgStackGetFrom(&MultiMsgPlayerFrom))
				return FALSE;

			if(!msgStackGetTo(&MultiMsgPlayerTo))
				return FALSE;

			if(!msgStackGetMsg(msg))
				return FALSE;

			strcpy(MultiplayMsg, msg);

			eventFireCallbackTrigger(CALL_BEACON);
			break;

		case CALL_AI_MSG:
			if(!msgStackGetFrom(&MultiMsgPlayerFrom))
				return FALSE;

			if(!msgStackGetTo(&MultiMsgPlayerTo))
				return FALSE;

			if(!msgStackGetMsg(msg))
				return FALSE;

			strcpy(MultiplayMsg, msg);

			eventFireCallbackTrigger(CALL_AI_MSG);
			break;

		default:
			debug(LOG_ERROR, "msgStackFireTop: unknown callback type");
			return FALSE;
			break;
	}

	if(!msgStackPop())
		return FALSE;

	return TRUE;
}

BOOL recvBeacon(NETMSG *pMsg)
{
	SDWORD	sender, receiver,locX, locY;
	
	STRING	msg[MAX_CONSOLE_STRING_LENGTH];

	NetGet(pMsg,0,sender);
	NetGet(pMsg,4,receiver);
	NetGet(pMsg,8,locX);
	NetGet(pMsg,12,locY);
	
	debug(LOG_WZ, "Received beacon for player: %d, from: %d",receiver, sender);

	strcpy(msg, &(pMsg->body[16]));
	strcat(msg, NetPlay.players[sender].name);		// name
	
	strcpy(beaconReceiveMsg[sender], msg);

	return addHelpBlip(locX,locY,receiver,sender,beaconReceiveMsg[sender]);
}