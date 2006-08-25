/*
 * MultiOpt.c
 *
 * Alex Lee,97/98, Pumpkin Studios
 *
 * Routines for setting the game options and starting the init process.
 */

#include "lib/framework/frame.h"			// for everything
#include "map.h"
#include "game.h"			// for loading maps
#include "message.h"		// for clearing messages.
#include "winmain.h"
#include "display3d.h"		// for changing the viewpoint
#include "power.h"
#include "lib/widget/widget.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "hci.h"
#include "configuration.h"			// lobby cfg.
#include "clparse.h"
#include "lib/ivis_common/piestate.h"

#include "component.h"
#include "console.h"
#include "multiplay.h"
#include "lib/sound/audio.h"
#include "multijoin.h"
#include "frontend.h"
#include "levels.h"
#include "multistat.h"
#include "multiint.h"
#include "multilimit.h"
#include "multigifts.h"
// ////////////////////////////////////////////////////////////////////////////
// GUID for warzone lobby and MPATH stuff.  i hate this stuff.
#ifdef WIN32		//Not really (going to be) used. -Qamly
#include <initguid.h>
//old guid {7B706E40-5A7E-11d1-94F6-006097B8260B}"
DEFINE_GUID(WARZONEGUID,0x48ab0b01,0xfec0,0x11d1,0x98,0xc,0x0,0xa0,0x24,0x38,0x70,0xa8);
// also change S_WARZONEGUID in multiplay.h
#endif

// ////////////////////////////////////////////////////////////////////////////
// External Variables

extern char	MultiForcesPath[255];

extern char	buildTime[8];
extern VOID	stopJoining(void);

// ////////////////////////////////////////////////////////////////////////////
// Local Functions

VOID		sendOptions			(DPID dest,UDWORD player);
VOID		recvOptions			(NETMSG *pMsg);
//static BOOL dMatchInit			(VOID);
static BOOL campInit			(VOID);
BOOL		hostCampaign		(STRING *sGame,		STRING *sPlayer);
BOOL		joinCampaign		(UDWORD gameNumber, STRING *playername);
//BOOL		hostArena			(STRING *sGame,		STRING *sPlayer);
//BOOL		joinArena			(UDWORD gameNumber, STRING *playername);
BOOL		LobbyLaunched		(VOID);
VOID		playerResponding	(VOID);
BOOL		multiInitialise		(VOID);		//only once.
BOOL		lobbyInitialise		(VOID);		//only once.
BOOL		sendLeavingMsg		(VOID);
BOOL		multiShutdown		(VOID);
BOOL		addTemplate			(UDWORD player, DROID_TEMPLATE *psNew);
BOOL		addTemplateSet		(UDWORD from,UDWORD to);
BOOL		copyTemplateSet		(UDWORD from,UDWORD to);
BOOL		multiGameInit		(VOID);		// every game
BOOL		multiGameShutdown	(VOID);

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

// send complete game info set!
// dpid == 0 for no new players.
VOID sendOptions(DPID dest,UDWORD play)
{
	NETMSG m;
	UBYTE checkval;

	NetAdd(m,0,game);
	m.size = sizeof(game);

	NetAdd(m,m.size,player2dpid);			//add dpid array
	m.size += sizeof(player2dpid);

	NetAdd(m,m.size,ingame.JoiningInProgress);
	m.size += sizeof(ingame.JoiningInProgress);

	checkval = NEThashVal(NetPlay.cryptKey[0]);	// exe's hash val. DONT SEND THE VAL ITSELF!
	NetAdd(m,m.size,checkval);
	m.size += sizeof(checkval);

	NetAdd(m,m.size,dest);
	m.size += sizeof(dest);

	NetAdd(m,m.size,play);
	m.size += sizeof(play);

	NetAdd(m,m.size,PlayerColour);
	m.size += sizeof(PlayerColour);

	NetAdd(m,m.size,alliances);
	m.size += sizeof(alliances);

	NetAdd(m,m.size,ingame.numStructureLimits);
	m.size += sizeof(ingame.numStructureLimits);
	if(ingame.numStructureLimits)
	{
		memcpy(&(m.body[m.size]),ingame.pStructureLimits, ingame.numStructureLimits * (sizeof(UBYTE)+sizeof(UDWORD)) );
		m.size = (UWORD)(m.size + ingame.numStructureLimits * (sizeof(UBYTE)+sizeof(UDWORD)) );
	}

	//
	// now add the wdg files that are being used.
	//

	m.type = NET_OPTIONS;				// send it.
	NETbcast(&m,TRUE);

}

// ////////////////////////////////////////////////////////////////////////////
BOOL checkGameWdg(CHAR *nm)
{
	LEVEL_DATASET *lev;

	//
	// now check the wdg files that are being used.
	//

	// game.map must be available in xxx list.

	lev = psLevels;
	while(lev)
	{
		if( strcmp(lev->pName, nm) == 0)
		{
			return TRUE;
		}
		lev=lev->psNext;
	}

	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////////
// options for a game. (usually recvd in frontend)
void recvOptions(NETMSG *pMsg)
{
	UDWORD	pos=0,play,id;
	DPID	newPl;
	UBYTE	checkval;


	NetGet(pMsg,0,game);									// get details.
	pos += sizeof(game);
	if(strncmp((CHAR*)game.version,buildTime,8) != 0)
	{

#ifndef DEBUG
		debug( LOG_ERROR, "Host is running a different version of Warzone2100." );
		abort();
#endif
	}
	if(ingame.numStructureLimits)							// free old limits.
	{
			ingame.numStructureLimits = 0;
			FREE(ingame.pStructureLimits);
	}

	NetGet(pMsg,pos,player2dpid);
	pos += sizeof(player2dpid);

	NetGet(pMsg,pos,ingame.JoiningInProgress);
	pos += sizeof(ingame.JoiningInProgress);

	NetGet(pMsg,pos,checkval);
	pos += sizeof(checkval);
/*
	// This was set to a fixed value in earlier versions of post-Pumpkin code.
	// Commenting out to avoid confusion. Should probably be removed. - Per
	if(checkval != NEThashVal(NetPlay.cryptKey[0]))
	{

		DBERROR(("Host Binary is different from this one. Cheating?"));
	}
*/
	NetGet(pMsg,pos,newPl);
	pos += sizeof(newPl);

	NetGet(pMsg,pos,play);
	pos += sizeof(play);

	NetGet(pMsg,pos,PlayerColour);
	pos += sizeof(PlayerColour);

	NetGet(pMsg,pos,alliances);
	pos += sizeof(alliances);

	NetGet(pMsg,pos,ingame.numStructureLimits);
	pos += sizeof(ingame.numStructureLimits);
	if(ingame.numStructureLimits)
	{
		ingame.pStructureLimits = MALLOC(ingame.numStructureLimits*(sizeof(UDWORD)+sizeof(UBYTE)));	// malloc some room
		memcpy(ingame.pStructureLimits, &(pMsg->body[pos]) ,ingame.numStructureLimits*(sizeof(UDWORD)+sizeof(UBYTE)));
	}

	// process
	if(newPl != 0)
	{
		if(newPl == NetPlay.dpidPlayer)
		{
			// it's us thats new
			selectedPlayer = play;							// select player
			NETplayerInfo();							// get player info
			powerCalculated = FALSE;						// turn off any power requirements.
		}
		else
		{
			// someone else is joining.
			setupNewPlayer( newPl, play);
		}
	}


	// do the skirmish slider settings if they are up,
	for(id=0;id<MAX_PLAYERS;id++)
	{
		if(widgGetFromID(psWScreen,MULTIOP_SKSLIDE+id))
		{
			widgSetSliderPos(psWScreen,MULTIOP_SKSLIDE+id,game.skDiff[id]);
		}
	}

	if(!checkGameWdg(game.map) )
	{
		// request the map from the host. NET_REQUESTMAP
		{
			NETMSG m;
			NetAdd(m,0,NetPlay.dpidPlayer);
			m.type = NET_REQUESTMAP;
			m.size =4;
			NETbcast(&m,TRUE);
			addConsoleMessage("MAP REQUESTED!",DEFAULT_JUSTIFY);
		}
	}
	else
	{
		loadMapPreview();
	}

}






// ////////////////////////////////////////////////////////////////////////////
// Host Campaign.
BOOL hostCampaign(STRING *sGame, STRING *sPlayer)
{
	PLAYERSTATS playerStats;
	UDWORD		pl,numpl,i,j;

	freeMessages();
	if(!NetPlay.bLobbyLaunched)
	{
		NEThostGame(sGame,sPlayer,game.type,0,0,0,game.maxPlayers); // temporary stuff
	}
	else
	{
		NETsetGameFlags(1,game.type);
		// 2 is average ping
		NETsetGameFlags(3,0);
		NETsetGameFlags(4,0);
	}

	for(i=0;i<MAX_PLAYERS;i++)
	{
		player2dpid[i] =0;
	}


	pl = rand()%game.maxPlayers;						//pick a player

	player2dpid[pl] = NetPlay.dpidPlayer;				// add ourselves to the array.
	selectedPlayer = pl;

	ingame.localJoiningInProgress = TRUE;
	ingame.JoiningInProgress[selectedPlayer] = TRUE;
	bMultiPlayer = TRUE;								// enable messages

	loadMultiStats(sPlayer,&playerStats);				// stats stuff
	setMultiStats(NetPlay.dpidPlayer,playerStats,FALSE);
	setMultiStats(NetPlay.dpidPlayer,playerStats,TRUE);

	if(!NetPlay.bComms)
	{
		NETplayerInfo();
		strcpy(NetPlay.players[0].name,sPlayer);
		numpl = 1;
	}
	else
	{
		numpl = NETplayerInfo();
	}

	// may be more than one player already. check and resolve!
	if(numpl >1)
	{
		for(j = 0;j<MAX_PLAYERS;j++)
		{
			if(NetPlay.players[j].dpid && (NetPlay.players[j].dpid != NetPlay.dpidPlayer))
			{
				for(i = 0;player2dpid[i] != 0;i++);
				player2dpid[i] = NetPlay.players[j].dpid;
			}
		}
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Join Campaign

BOOL joinCampaign(UDWORD gameNumber, STRING *sPlayer)
{
	PLAYERSTATS	playerStats;

	if(!ingame.localJoiningInProgress)
	{
//		game.type = CAMPAIGN;
		if(!NetPlay.bLobbyLaunched)
		{
			NETjoinGame(gameNumber, sPlayer);	// join
		}
		ingame.localJoiningInProgress	= TRUE;

		loadMultiStats(sPlayer,&playerStats);
		setMultiStats(NetPlay.dpidPlayer,playerStats,FALSE);
		setMultiStats(NetPlay.dpidPlayer,playerStats,TRUE);
		return FALSE;
	}

	bMultiPlayer = TRUE;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// HostArena
/*
BOOL hostArena(STRING *sGame, STRING *sPlayer)
{
	PLAYERSTATS playerStats;
	UDWORD		numpl,i,j,pl;

	game.type = DMATCH;

	freeMessages();
	if(!NetPlay.bLobbyLaunched)
	{
		NEThostGame(sGame,sPlayer,DMATCH,0,0,0,game.maxPlayers);			// temporary stuff
	}
	else
	{
		NETsetGameFlags(1,DMATCH);
	// 2 is average ping
		NETsetGameFlags(3,0);
		NETsetGameFlags(4,0);
	}

	bMultiPlayer = TRUE;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		player2dpid[i] =0;
	}

	//pick a player
#if 0
	pl =0;
#else
	pl = rand()%game.maxPlayers;
#endif

	player2dpid[pl] = NetPlay.dpidPlayer;							// add ourselves to the array.
	selectedPlayer = pl;

	ingame.localJoiningInProgress = TRUE;
	ingame.JoiningInProgress[selectedPlayer] = TRUE;

	loadMultiStats(sPlayer,&playerStats);						// load player stats!
	setMultiStats(NetPlay.dpidPlayer,playerStats,FALSE);
	setMultiStats(NetPlay.dpidPlayer,playerStats,TRUE);

	numpl = NETplayerInfo(NULL);

	// may be more than one player already. check and resolve!
	if(numpl >1)
	{
		for(j = 0;j<MAX_PLAYERS;j++)
		{
			if(NetPlay.players[j].dpid && (NetPlay.players[j].dpid != NetPlay.dpidPlayer))
			{
				for(i = 0;player2dpid[i] != 0;i++);
				player2dpid[i] = NetPlay.players[j].dpid;
				ingame.JoiningInProgress[i] = TRUE;
			}
		}
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// JoinArena.
BOOL joinArena(UDWORD gameNumber, STRING *playerName)
{
	PLAYERSTATS	playerStats;

	if(!ingame.localJoiningInProgress)
	{
		game.type = DMATCH;
		if(!NetPlay.bLobbyLaunched)
		{
			NETjoinGame(NetPlay.games[gameNumber].desc.guidInstance,playerName);	// join
		}
		ingame.localJoiningInProgress	= TRUE;

		loadMultiStats(playerName,&playerStats);
		setMultiStats(NetPlay.dpidPlayer,playerStats,FALSE);
		setMultiStats(NetPlay.dpidPlayer,playerStats,TRUE);
		return FALSE;
	}

	bMultiPlayer = TRUE;
	return TRUE;
}
*/

// ////////////////////////////////////////////////////////////////////////////
// Lobby launched. fires the correct routine when the game was lobby launched.
BOOL LobbyLaunched(VOID)
{
	UDWORD			i;
	PLAYERSTATS		pl={0};

	// set the player info as soon as possible to avoid screwy scores appearing elsewhere.
	NETplayerInfo();
	NETfindGame(TRUE);

	for(i = 0; (i< MAX_PLAYERS)&& (NetPlay.players[i].dpid != NetPlay.dpidPlayer);i++);

	if(!loadMultiStats(NetPlay.players[i].name,&pl) )
	{
		return FALSE;									// cheating was detected, so fail.
	}

	setMultiStats(NetPlay.dpidPlayer,pl,FALSE);
	setMultiStats(NetPlay.dpidPlayer,pl,TRUE);

	// setup text boxes on multiplay screen.
	strcpy((STRING*) sPlayer,	NetPlay.players[i].name);
	strcpy((STRING*) game.name, NetPlay.games[0].name);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Init and shutdown routines
BOOL lobbyInitialise(VOID)
{

	if(!NETinit(TRUE))								// initialise, may change guid.
	{
		return FALSE;
	}
	// setup the encryption key

	// RODZ : hashing the file is no more an option.
	// hash the file to get the key.and catch out the exe patchers.
//#ifdef DEBUG
	NETsetKey( 0xdaf456 ,0xb72a5, 0x114d0, 0x2a17);
//#else
//	NETsetKey(NEThashFile("warzone.exe"), 0xb72a5, 0x114d0, 0x2a7);
//#endif

	if(NetPlay.bLobbyLaunched)									// now check for lobby launching..
	{
		if(!LobbyLaunched())
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL multiInitialise(VOID)
{
	// NET AUDIO CAPTURE
	NETinitAudioCapture();
#ifdef WIN32			//Disabled for now.  (returns FALSE always anyway) --Qamly
//	NETinitPlaybackBuffer(audio_GetDirectSoundObj());			// pass in a dsound pointer to use.
#endif

	return TRUE;  // use the menus dumbass.
}



// ////////////////////////////////////////////////////////////////////////////
// say goodbye to everyone else
BOOL sendLeavingMsg(VOID)
{
	NETMSG m;
        UBYTE bHost = (UBYTE)NetPlay.bHost;
	// send a leaving message, This resolves a problem with tcpip which
	// occasionally doesn't automatically notice a player leaving.
	NetAdd(m,0,player2dpid[selectedPlayer]);
	NetAdd(m,4,bHost);
	m.size = 5;
	m.type = NET_LEAVING ;
	NETbcast(&m,TRUE);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// called in Init.c to shutdown the whole netgame gubbins.
BOOL multiShutdown(VOID)
{
	FORCE_MEMBER *pF;

	debug(LOG_MAIN, "shutting down audio capture");
	NETshutdownAudioCapture();

	debug(LOG_MAIN, "shutting down audio playback");
	NETshutdownAudioPlayback();

	// shut down netplay lib.
	debug(LOG_MAIN, "shutting down networking");
  	NETshutdown();

	// clear any force we may have.
	debug(LOG_MAIN, "free game data (forces)");
	while(Force.pMembers)
	{
		pF = Force.pMembers;
		Force.pMembers = pF->psNext;
		FREE(pF);
	}

	debug(LOG_MAIN, "free game data (structure limits)");
	if(ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		FREE(ingame.pStructureLimits);
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// copy tempates from one player to another.


BOOL addTemplate(UDWORD player, DROID_TEMPLATE *psNew)
{
	DROID_TEMPLATE	*psTempl;

	if (!HEAP_ALLOC(psTemplateHeap, (void*) &psTempl))
	{
		return FALSE;
	}
	memcpy(psTempl, psNew, sizeof(DROID_TEMPLATE));

	psTempl->pName = (CHAR*)&psTempl->aName;
	strncpy(psTempl->aName, psNew->aName,DROID_MAXNAME);
	psTempl->pName[DROID_MAXNAME-1]=0;


	psTempl->psNext = apsDroidTemplates[player];
	apsDroidTemplates[player] = psTempl;

	return TRUE;
}

BOOL addTemplateSet(UDWORD from,UDWORD to)
{
	DROID_TEMPLATE	*psCurr;

	if(from == to)
	{
		return TRUE;
	}

	for(psCurr = apsDroidTemplates[from];psCurr;psCurr= psCurr->psNext)
	{
		addTemplate(to, psCurr);
	}

	return TRUE;
}

BOOL copyTemplateSet(UDWORD from,UDWORD to)
{
	DROID_TEMPLATE	*psTempl;

	if(from == to)
	{
		return TRUE;
	}

	while(apsDroidTemplates[to])				// clear the old template out.
	{
		psTempl = apsDroidTemplates[to]->psNext;
		HEAP_FREE(psTemplateHeap, apsDroidTemplates[to]);
		apsDroidTemplates[to] = psTempl;
	}

	return 	addTemplateSet(from,to);
}

// ////////////////////////////////////////////////////////////////////////////
// setup templates
BOOL multiTemplateSetup()
{
	UDWORD player, pcPlayer = 0;
	CHAR			sTemp[256];


	if(game.type == CAMPAIGN && game.base == CAMP_WALLS)
	{
		strcpy(sTemp, MultiForcesPath);
		strcat(sTemp, sForceName);
		strcat(sTemp,".for");

		loadForce(sTemp);
//		useTheForce(TRUE);
	}


	switch (game.type)
	{
//	case DMATCH:
//		for(player=0;player<MAX_PLAYERS;player++)
//		{
//			 copyTemplateSet(DEATHMATCHTEMPLATES,player);
//		}
//		break;

	case TEAMPLAY:
	case CAMPAIGN:
		for(player=0;player<game.maxPlayers;player++)
		{
			copyTemplateSet(CAMPAIGNTEMPLATES,player);
		}
		break;

	case SKIRMISH:
		// create the pc player list in deathmatch set.
		addTemplateSet(CAMPAIGNTEMPLATES,DEATHMATCHTEMPLATES);
		addTemplateSet(6,DEATHMATCHTEMPLATES);
		addTemplateSet(2,DEATHMATCHTEMPLATES);

		//choose which way to do this.
		if(isHumanPlayer(CAMPAIGNTEMPLATES))
		{
			//pc first
			for(player=0;player<game.maxPlayers;player++)
			{
				if(!isHumanPlayer(player))
				{
					copyTemplateSet(DEATHMATCHTEMPLATES,player);
				}
			}
			//now players.
			for(player=0;player<game.maxPlayers;player++)
			{
				if(isHumanPlayer(player))
				{
					copyTemplateSet(CAMPAIGNTEMPLATES,player);
				}
			}
		}
		else
		{
			// ensure a copy of pc templates to a pc player.
			if(isHumanPlayer(DEATHMATCHTEMPLATES))
			{

				for(player=0;player<MAX_PLAYERS && isHumanPlayer(player);player++);
				if(!isHumanPlayer(player))
				{
					pcPlayer = player;
					copyTemplateSet(DEATHMATCHTEMPLATES,pcPlayer);
				}
			}
			else
			{
				pcPlayer = DEATHMATCHTEMPLATES;
			}
			//players first
			for(player=0;player<game.maxPlayers;player++)
			{
				if(isHumanPlayer(player))
				{
					copyTemplateSet(CAMPAIGNTEMPLATES,player);
				}
			}
			//now pc
			for(player=0;player<game.maxPlayers;player++)
			{
				if(!isHumanPlayer(player))
				{
					copyTemplateSet(pcPlayer,player);
				}
			}
		}
		break;

	default:
		break;
	}

	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////////
// remove structures from map before campaign play.
BOOL cleanMap(UDWORD player)
{
	DROID		*psD,*psD2;
	STRUCTURE	*psStruct;
	BOOL		firstFact,firstRes;

	bMultiPlayer = FALSE;

	firstFact = TRUE;
	firstRes = TRUE;

	// reverse so we always remove the last object. re-reverse afterwards.
//	reverseObjectList((BASE_OBJECT**)&apsStructLists[player]);


	switch(game.base)
	{
	case CAMP_CLEAN:									//clean map
		while(apsStructLists[player])					//strip away structures.
		{
			removeStruct(apsStructLists[player], TRUE);
		}
		psD = apsDroidLists[player];					// remove all but construction droids.
		while(psD)
		{
			psD2=psD->psNext;
			//if(psD->droidType != DROID_CONSTRUCT)
            if (!(psD->droidType == DROID_CONSTRUCT OR
                psD->droidType == DROID_CYBORG_CONSTRUCT))
			{
				killDroid(psD);
			}
			psD = psD2;
		}
		break;

	case CAMP_BASE:												//just structs, no walls
		psStruct = apsStructLists[player];
		while(psStruct)
		{
			if ( (psStruct->pStructureType->type == REF_WALL)
			   ||(psStruct->pStructureType->type == REF_WALLCORNER)
			   ||(psStruct->pStructureType->type == REF_DEFENSE)
			   ||(psStruct->pStructureType->type == REF_BLASTDOOR)
			   ||(psStruct->pStructureType->type == REF_CYBORG_FACTORY)
			   ||(psStruct->pStructureType->type == REF_COMMAND_CONTROL)
			   )
			{
				removeStruct(psStruct, TRUE);
				psStruct= apsStructLists[player];			//restart,(list may have changed).
			}

			else if( (psStruct->pStructureType->type == REF_FACTORY)
				   ||(psStruct->pStructureType->type == REF_RESEARCH)
				   ||(psStruct->pStructureType->type == REF_POWER_GEN))
			{
				if(psStruct->pStructureType->type == REF_FACTORY )
				{
					if(firstFact == TRUE)
					{
						firstFact = FALSE;
						removeStruct(psStruct, TRUE);
						psStruct= apsStructLists[player];
					}
					else	// don't delete, just rejig!
					{
						if(((FACTORY*)psStruct->pFunctionality)->capacity != 0)
						{
							((FACTORY*)psStruct->pFunctionality)->capacity = 0;
							((FACTORY*)psStruct->pFunctionality)->productionOutput = (UBYTE)((PRODUCTION_FUNCTION*)psStruct->pStructureType->asFuncList[0])->productionOutput;

							psStruct->sDisplay.imd	= psStruct->pStructureType->pIMD;
							psStruct->body			= (UWORD)(structureBody(psStruct));

						}
						psStruct				= psStruct->psNext;
					}
				}
				else if(psStruct->pStructureType->type == REF_RESEARCH)
				{
					if(firstRes == TRUE)
					{
						firstRes = FALSE;
						removeStruct(psStruct, TRUE);
						psStruct= apsStructLists[player];
					}
					else
					{
						if(((RESEARCH_FACILITY*)psStruct->pFunctionality)->capacity != 0)
						{	// downgrade research
							((RESEARCH_FACILITY*)psStruct->pFunctionality)->capacity = 0;
							((RESEARCH_FACILITY*)psStruct->pFunctionality)->researchPoints = ((RESEARCH_FUNCTION*)psStruct->pStructureType->asFuncList[0])->researchPoints;
							psStruct->sDisplay.imd	= psStruct->pStructureType->pIMD;
							psStruct->body			= (UWORD)(structureBody(psStruct));
						}
						psStruct=psStruct->psNext;
					}
				}
				else if(psStruct->pStructureType->type == REF_POWER_GEN)
				{
						if(((POWER_GEN*)psStruct->pFunctionality)->capacity != 0)
						{	// downgrade powergen.
							((POWER_GEN*)psStruct->pFunctionality)->capacity = 0;
							((POWER_GEN*)psStruct->pFunctionality)->power = ((POWER_GEN_FUNCTION*)psStruct->pStructureType->asFuncList[0])->powerOutput;
							((POWER_GEN*)psStruct->pFunctionality)->multiplier += ((POWER_GEN_FUNCTION*)psStruct->pStructureType->asFuncList[0])->powerMultiplier;

							psStruct->sDisplay.imd	= psStruct->pStructureType->pIMD;
							psStruct->body			= (UWORD)(structureBody(psStruct));
						}
						psStruct=psStruct->psNext;
				}
			}

			else
			{
				psStruct=psStruct->psNext;
			}
		}
		break;


	case CAMP_WALLS:												//everything.
		break;
	default:
		debug( LOG_ERROR, "Unknown Campaign Style" );
		abort();
		break;
	}

	// rerev list to get back to normal.
//	reverseObjectList((BASE_OBJECT**)&apsStructLists[player]);

	bMultiPlayer = TRUE;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// setup a deathmatch game.
/*
static BOOL dMatchInit()
{
	UDWORD	i;
	NETMSG	msg;
	UDWORD	player;
	BOOL	resourceFound = FALSE;
	CHAR	sTemp[256];

	turnOffMultiMsg(TRUE);

	for(i =0 ; i<MAX_PLAYERS;i++)
	{
		setPower(i,LEV_HI);							// set deathmatch power to hi.
	}

	strcpy(sTemp, "multiplay/forces/");
	strcat(sTemp, sForceName);
	strcat(sTemp,".for");
	loadForce( sTemp);


	freeMessages();									// CLEAR MESSAGES

	if(NetPlay.bHost)								// it's new.
	{

	}
	else
	{
		for(player=0;player<MAX_PLAYERS;player++)			// remove droids.
		{
			while(apsDroidLists[player])
			{
				killDroid(apsDroidLists[player]);
			}

			while(apsFeatureLists[player])
			{
				removeFeature(apsFeatureLists[player]);
			}

			while(apsStructLists[player])
			{
				removeStruct(apsStructLists[player]);
			}
		}

	}
	turnOffMultiMsg(FALSE);

	if(NetPlay.bHost)
	{
														// dont do anything.
	}
	else
	{
		NetAdd(msg,0,NetPlay.dpidPlayer);				// start to request info.
		NetAdd(msg,4,arenaPlayersReceived);				// player we require.
		msg.size = 8;
		msg.type = NET_REQUESTPLAYER;
		NETbcast(msg,TRUE);
	}

	return TRUE;
}
*/
// ////////////////////////////////////////////////////////////////////////////
// setup a campaign game
static BOOL campInit()
{
	UDWORD			player;
	UBYTE		newPlayerArray[MAX_PLAYERS];
	UDWORD		i,j;

// if this is from a savegame, stop here!
	if((getSaveGameType() == GTYPE_SAVE_START)
	|| (getSaveGameType() == GTYPE_SAVE_MIDMISSION)	)
	{
		// these two lines are the biggest hack in the world.
		// the reticule seems to get detached from 'reticuleup'
		// this forces it back in sync...
		intRemoveReticule();
		intAddReticule();

		return TRUE;
	}



	// for each player, if it's a skirmish then assign a player or clear it off.
	if(game.type == SKIRMISH)
	{
		memset(newPlayerArray,1,MAX_PLAYERS);
		j=0;
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(game.skDiff[i] == 0)						// no player.
			{
				// find a non human player and strip the lot.
				for(;isHumanPlayer(j) && j<MAX_PLAYERS;j++);
				if(j != MAX_PLAYERS)
				{
					clearPlayer(j,TRUE,FALSE);
					newPlayerArray[j] = 0;
					j++; // dont do this one again.
				}
			}
			else if(game.skDiff[i] == UBYTE_MAX)		// human player.
			{
				// do nothing.
			}
			else
			{
				newPlayerArray[j] = game.skDiff[i];		// skirmish player.
				j++;
			}
		}
		memcpy(game.skDiff,newPlayerArray,MAX_PLAYERS);
	}


	for(player = 0;player<game.maxPlayers;player++)			// clean up only to the player limit for this map..
	{
		if( (!isHumanPlayer(player)) && game.type != SKIRMISH)	// strip away unused players
		{
			clearPlayer(player,TRUE,TRUE);
		}

		cleanMap(player);
	}

	// optionally remove other computer players.
	if(  ( (game.type == TEAMPLAY || game.type == CAMPAIGN) && !game.bComputerPlayers )
	   ||  (game.type == SKIRMISH)
	  )
	{
		for(player=game.maxPlayers;player<MAX_PLAYERS;player++)
		{
			clearPlayer(player,TRUE,FALSE);
		}
	}

	// add free research gifts..
	if(NetPlay.bHost)
	{
		addOilDrum( NetPlay.playercount*2 );		// add some free power.
	}

	playerResponding();			// say howdy!

	if(game.type == CAMPAIGN && game.base == CAMP_WALLS)
	{
		useTheForce(TRUE);
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// say hi to everyone else....
VOID playerResponding(VOID)
{
	NETMSG	msg;
	UDWORD	i;

	ingame.startTime = gameTime;
	ingame.localJoiningInProgress = FALSE;				// no longer joining.
	ingame.JoiningInProgress[selectedPlayer] = FALSE;
//	arenaPlayersReceived	= 0;						// clear rcvd list.

	cameraToHome(selectedPlayer,FALSE);						// home the camera to the player.

	NetAdd(msg,0,selectedPlayer);						// tell the world we're here.
	msg.size = sizeof(UDWORD);
	msg.type = NET_PLAYERRESPONDING;
	NETbcast(&msg,TRUE);

	// set the key from the lowest available dpid.
	for(i=0; !player2dpid[i] && i<MAX_PLAYERS;i++);
	NETsetKey(0,0,0,player2dpid[i]);

	NetPlay.bEncryptAllPackets = TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
//called when the game finally gets fired up.
BOOL multiGameInit(VOID)
{
	UDWORD player;

	for(player=0;player<MAX_PLAYERS;player++)
	{
		openchannels[player] =TRUE;								//open comms to this player.
	}

//	if(game.type == DMATCH)
//	{
//		dMatchInit();
//		if(NetPlay.bHost)
//		{
//			addDMatchDroid(1);							// each player adds a newdroid point.
//			playerResponding();							// say hi, only if host, clients wait until all info recvd.
//		}
//	}
//	else
//	{
		campInit();
//	}

	return TRUE;
}


////////////////////////////////
// at the end of every game.
BOOL multiGameShutdown(VOID)
{
	PLAYERSTATS	st;

	sendLeavingMsg();							// say goodbye
	updateMultiStatsGames();					// update games played.

	st = getMultiStats(selectedPlayer,TRUE);	// save stats

	saveMultiStats(getPlayerName(selectedPlayer),getPlayerName(selectedPlayer),&st);

	NETclose();									// close game.

	if(ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		FREE(ingame.pStructureLimits);
	}

	ingame.localJoiningInProgress   = FALSE;	// clean up
	ingame.localOptionsReceived		= FALSE;
	ingame.bHostSetup				= FALSE;	//dont attempt a host
	NetPlay.bLobbyLaunched			= FALSE;	//revert back to main menu, not multioptions.
	NetPlay.bHost					= FALSE;
	bMultiPlayer					= FALSE;	//back to single player mode
	selectedPlayer					= 0;		//back to use player 0 (single player friendly)
	bForceEditorLoaded				= FALSE;

	NetPlay.bEncryptAllPackets		= FALSE;	// pull security.
	return TRUE;
}
