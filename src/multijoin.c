/*
 * Multijoin.c
 *
 * Alex Lee, pumpkin studios, bath,
 *
 * Stuff to handle the comings and goings of players.
 */

#include <stdio.h>					// for sprintf

#include "lib/framework/frame.h"

#include "objmem.h"
#include "statsdef.h"
#include "droiddef.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/gamelib/gtime.h"
#include "game.h"
#include "projectile.h"
#include "droid.h"
#include "map.h"
#include "power.h"
#include "game.h"					// for loading maps
#include "player.h"
#include "message.h"				// for clearing game messages
#include "text.h"					// for string resources.
#include "order.h"
#include "console.h"
#include "orderdef.h"				// for droid_order_data
#include "hci.h"
#include "component.h"
#include "research.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "wrappers.h"
#include "intimage.h"
#include "data.h"
#include "lib/script/script.h"
#include "scripttabs.h"

#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multijoin.h"
#include "multirecv.h"
#include "multiint.h"
#include "multistat.h"
#include "multigifts.h"

// ////////////////////////////////////////////////////////////////////////////
// External Variables
extern BOOL		bHosted;
extern BOOL		multiRequestUp;
// ////////////////////////////////////////////////////////////////////////////
//external functions

// ////////////////////////////////////////////////////////////////////////////
// Local Functions
BOOL sendVersionCheck	();
BOOL recvVersionCheck	(NETMSG *pMsg);
BOOL intDisplayMultiJoiningStatus (UBYTE joinCount);
void clearPlayer		(UDWORD player,BOOL quietly,BOOL removeOil);// what to do when a arena player leaves.
BOOL MultiPlayerLeave	(DPID dp);								// remote player has left.
BOOL MultiPlayerJoin	(DPID dp);								// remote player has just joined.
void setupNewPlayer		(DPID dpid,UDWORD player);				// stuff to do when player joins.
//BOOL multiPlayerRequest	(NETMSG *pMsg);							// remote player has requested info
//BOOL UpdateClient		(DPID dest, UDWORD playerToSend);		// send information to a remote player
//BOOL ProcessDroidOrders	(VOID);									// ince setup, this player issues each droid order.
//BOOL SendFeatures		(FEATURE *pFeature, DPID player);
//BOOL recvFeatures		(NETMSG *pMsg);
void resetMultiVisibility(UDWORD player);

// ////////////////////////////////////////////////////////////////////////////
// Version Check

BOOL sendVersionCheck()
{
	NETMSG msg;

	msg.size = 0;

	NetAdd(msg,0,selectedPlayer);
	NetAdd(msg,1,cheatHash);
	msg.size = sizeof(cheatHash)+1;
	msg.type = NET_VERSION;

	return NETbcast(&msg,TRUE);
}

BOOL recvVersionCheck(NETMSG *pMsg)
{
	UDWORD	extCheat[CHEAT_MAXCHEAT];
	UBYTE	pl;
	CHAR	sTmp[128];
	NetGet(pMsg,1,extCheat);

	if( memcmp(extCheat,cheatHash, CHEAT_MAXCHEAT*4) != 0)
	{
		goto FAILURE;
	}
	return TRUE;

FAILURE:

	// what should we do now ?
	NetGet(pMsg,0,pl);

//	DBERROR(("%s has different data to you CHEATING?"));

	sprintf(sTmp,"%s has different data. CHEATING or wrong version",getPlayerName(pl));
	addConsoleMessage(sTmp,DEFAULT_JUSTIFY);
	sendTextMessage(sTmp,TRUE);
/*
	// We need to disable this for now. It is in any case trivial to circumvent, and
	// may give a false sense of security. - Per
	if(NetPlay.bHost)
	{
		kickPlayer( player2dpid[pl] );
	}
*/
//	setPlayerHasLost(TRUE);
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Wait For Players

BOOL intDisplayMultiJoiningStatus(UBYTE joinCount)
{
	UDWORD			x,y,w,h;
	CHAR			sTmp[6];

	w = RET_FORMWIDTH;
	h = RET_FORMHEIGHT;
	x = RET_X;
	y = RET_Y;

//	cameraToHome(selectedPlayer);				// home the camera to the player.
	RenderWindowFrame(&FrameNormal, x, y ,w, h);		// draw a wee blu box.

	// display how far done..
	pie_DrawText(strresGetString(psStringRes, STR_GAM_JOINING),
					x+(w/2)-(iV_GetTextWidth(strresGetString(psStringRes,STR_GAM_JOINING))/2),
					y+(h/2)-8 );
	sprintf(sTmp,"%d%%", PERCENT((NetPlay.playercount-joinCount),NetPlay.playercount) );
	pie_DrawText(sTmp ,x + (w / 2) - 10, y + (h / 2) + 10);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// when a remote player leaves an arena game do this!
void clearPlayer(UDWORD player,BOOL quietly,BOOL removeOil)
{
	UDWORD			i;
	BOOL			bTemp;
	STRUCTURE		*psStruct,*psNext;

	player2dpid[player] = 0;					// remove player, make computer controlled
	ingame.JoiningInProgress[player] = FALSE;	// if they never joined, reset the flag

	for(i = 0;i<MAX_PLAYERS;i++)				// remove alliances
	{
		alliances[player][i]	= ALLIANCE_BROKEN;
		alliances[i][player]	= ALLIANCE_BROKEN;
	}

	while(apsDroidLists[player])				// delete all droids
	{
		if(quietly)
		{
			killDroid(apsDroidLists[player]);
		}else{
			destroyDroid(apsDroidLists[player]);
		}
	}

	psStruct = apsStructLists[player];
	while(psStruct)				// delete all structs
	{
		psNext = psStruct->psNext;
		bTemp = FALSE;

		if(removeOil)
		{
			if (psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)
			{
				bTemp =  TRUE;
			}
		}

		if(quietly)
		{
			removeStruct(psStruct, TRUE);
		}
		else
		{
			if(	(psStruct->pStructureType->type != REF_WALL &&
				 psStruct->pStructureType->type != REF_WALLCORNER ) )
			{
				destroyStruct(psStruct);
			}
		}

		if(bTemp)
		{
			if(apsFeatureLists[0]->psStats->subType == FEAT_OIL_RESOURCE)
			{
				removeFeature(apsFeatureLists[0]);
			}
		}
		psStruct = psNext;
	}

	return;
}

// Reset visibilty, so a new player can't see the old stuff!!
void resetMultiVisibility(UDWORD player)
{
	UDWORD		owned;
	DROID		*pDroid;
	STRUCTURE	*pStruct;

	for(owned = 0 ; owned <MAX_PLAYERS ;owned++)		// for each player
	{
		if(owned != player)								// done reset own stuff..
		{
			//droids
			for(pDroid = apsDroidLists[owned];pDroid;pDroid=pDroid->psNext)
			{
				pDroid->visible[player] = FALSE;
			}

			//structures
			for(pStruct= apsStructLists[owned];pStruct;pStruct=pStruct->psNext)
			{
				pStruct->visible[player] = FALSE;
			}

		}
	}
	return;
}

// ////////////////////////////////////////////////////////////////////////////
// A remote player has left the game
BOOL MultiPlayerLeave( DPID dp)
{
	UDWORD	i = 0;
	char	buf[255];

	while((player2dpid[i] != dp) && (i<MAX_PLAYERS) )i++;	// find out which!

	if(i != MAX_PLAYERS)									// player not already removed
	{
		NETlogEntry("Player Unexpectedly leaving, came from directplay...",0,dp);

		sprintf( buf,strresGetString(psStringRes, STR_MUL_LEAVE),getPlayerName(i) );

		turnOffMultiMsg(TRUE);
		clearPlayer(i,FALSE,FALSE);
		game.skDiff[i] = 10;

		turnOffMultiMsg(FALSE);

		addConsoleMessage(buf,DEFAULT_JUSTIFY);

		if(widgGetFromID(psWScreen,IDRET_FORM))
		{
			audio_QueueTrack( ID_CLAN_EXIT );
		}
	}

	NETplayerInfo();									// update the player info stuff


	// fire script callback to reassign skirmish players.
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_PLAYERLEFT);


	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// A Remote Player has joined the game.
BOOL MultiPlayerJoin(DPID dpid)
{
	UDWORD i;

	if(widgGetFromID(psWScreen,IDRET_FORM))	// if ingame.
	{
		audio_QueueTrack( ID_CLAN_ENTER );
	}

	if(widgGetFromID(psWScreen,MULTIOP_PLAYERS))	// if in multimenu.
	{
		if( !multiRequestUp && (bHosted
								|| (ingame.localJoiningInProgress && !NetPlay.bLobbyLaunched)
								|| (NetPlay.bLobbyLaunched && ingame.localOptionsReceived)
			))
			{
				addPlayerBox(TRUE);				// update the player box.
			}
	}

	if(NetPlay.bHost)		// host responsible for welcoming this player.
	{
		// if we've already received a request from this player don't reallocate.
		for(i=0; (i<MAX_PLAYERS) ;i++)
		{
			if((player2dpid[i] == dpid) && ingame.JoiningInProgress[i] )
			{
				return TRUE;
			}
		}
		ASSERT( NetPlay.playercount<=MAX_PLAYERS,"Too many players!" );

		// setup data for this player, then broadcast it to the other players.
#if 0
		for(i=0; player2dpid[i]!= 0 ;i++);			// find a zero entry, for a new player. MAKE RANDOM!!!
#else
		do{											// randomly allocate a player to this new machine.
			i = rand()%game.maxPlayers;
		}while(player2dpid[i] != 0);
#endif

		setPlayerColour(i,MAX_PLAYERS);				// force a colourchoice
		chooseColour(i);							// pick an unused colour.

		setupNewPlayer(dpid,i);						// setup all the guff for that player.
		if(!NetPlay.bLobbyLaunched
		   || (NetPlay.bLobbyLaunched && bHosted))
		{
			sendOptions(dpid,i);
		}

		// if skirmish and game full, then kick...
		if(game.type == SKIRMISH && NetPlay.playercount > game.maxPlayers )
		{
			kickPlayer(dpid);
		}

	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Setup Stuff for a new player.
void setupNewPlayer(DPID dpid,UDWORD player)
{
	UDWORD i;//,col;
	char buf[255];

	player2dpid[player] = dpid;							// assign them a player too.
	ingame.PingTimes[player] =0;						// reset ping time
	ingame.JoiningInProgress[player] = TRUE;			// note that player is now joining.

//	if (game.type == DMATCH)
//	{
//		// set the power level for that player..
//		setPower(player, game.power);
//	}

	for(i=0;i<MAX_PLAYERS;i++)							// set all alliances to broken.
	{
		alliances[selectedPlayer][i] = ALLIANCE_BROKEN;
		alliances[i][selectedPlayer] = ALLIANCE_BROKEN;
	}

	resetMultiVisibility(player);						// set visibility flags.
	NETplayerInfo();								// update the net info stuff

	setMultiStats(player2dpid[player],getMultiStats(player,FALSE),TRUE);  // get the players score from the ether.

	sprintf( buf,strresGetString(psStringRes, STR_MUL_JOINING),getPlayerName(player) );
	addConsoleMessage(buf,DEFAULT_JUSTIFY);
}

// ////////////////////////////////////////////////////////////////////////////
// CAMPAIGN STUFF


// ////////////////////////////////////////////////////////////////////////////
// ARENA STUFF

// ////////////////////////////////////////////////////////////////////////////
// a remote player has requested information about another player.

/*
BOOL multiPlayerRequest(NETMSG *pMsg)
{
	DPID dp;
	UDWORD pl,newpl;//,rpl;
	BOOL responsible = FALSE;

	NetGet(pMsg,0,dp);			// player dpid requesting info
	NetGet(pMsg,4,pl);			// player they require.

	for(newpl=0;(player2dpid[newpl] != dp) &&(newpl<MAX_PLAYERS);newpl++);		// find player


	if( (pl == selectedPlayer) )
	{
		responsible = TRUE;
	}

	if( NetPlay.bHost && ingame.JoiningInProgress[pl]  )
	{
		responsible = TRUE;
	}

	if( NetPlay.bHost && !isHumanPlayer(pl) )
	{
		responsible = TRUE;
	}

	if(responsible)
	{
		UpdateClient(dp,pl);
	}
	return TRUE;
}


// /////////////////////////////////////////////////////////////////////////////////////
// send information to allow player 'dest' to update their game state to the current one.
BOOL UpdateClient(DPID dest, UDWORD playerToSend)
{
	NETMSG			 m;
	DROID			*pD;
	UDWORD			 i=0;
//	STRUCTURE		*pS;
//	DROID_TEMPLATE	*pT;
//	PLAYER_RESEARCH	*pR;

	for(pD = apsDroidLists[playerToSend]; pD; pD=pD->psNext)	// **DROIDS**
	{
		sendWholeDroid(pD,dest);
	}

	if(playerToSend == 0)										// **FEATURES** SEND ONLY ONCE (BIG MSG!)
	{
		SendFeatures(apsFeatureLists[0],dest);
	}

//	pR = asPlayerResList[playerToSend];							// **RESEARCH**
//	for(i=0; i<numResearch; i++)								// do for each topic.
//	{
//		if(pR[i].researched == RESEARCHED)						// send if researched
//		{
//			NetAdd(m,0,(char)playerToSend);
//			NetAdd(m,1,i);										// reference into topic.
//			m.size =1+sizeof(UDWORD);
//			m.type = NET_RESEARCH;
//			NETsend(m,dest,TRUE);
//		}
//	}

//	pT = apsDroidTemplates[playerToSend];						// **TEMPLATES**
//	while(pT)
//	{
//		m.body[0]= (char) playerToSend;							//player to attach template to
//		memcpy(&(m.body[1]),pT,	sizeof(DROID_TEMPLATE));		//the template itself
//		strcpy(&(m.body[1+sizeof(DROID_TEMPLATE)]),	pT->pName );//the name to give the template
//		m.size =(UWORD)(sizeof(DROID_TEMPLATE)+ strlen(pT->pName) +2);
//		m.type=NET_TEMPLATE;
//		NETsend(m,dest,TRUE);
//		pT=pT->psNext;											// onto next template
//	}

//	for(pS=apsStructLists[playerToSend]; pS; pS=pS->psNext)		//  **STRUCTURES**
//	{
	// NOTE SEND WHOLE STRUCTURE DOESNT WORK! need to do as sendwholedroids.
//		SendWholeStructure(pS,dest);
//	}



	m.type = NET_PLAYERCOMPLETE;								// completed message
	NetAdd(m,0,player2dpid[selectedPlayer]);
	m.size = sizeof(player2dpid[selectedPlayer]);
	NETsend(m,dest,TRUE);

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Send/Recv Features when a player joins the game.

// send a list of features on this machine.
BOOL SendFeatures(FEATURE *pFeature, DPID player)
{
	NETMSG msg;

	msg.size = 0;
	msg.type = NET_FEATURES;

	for(;pFeature;pFeature=pFeature->psNext)
	{
		NetAdd(msg,msg.size,pFeature->id);				//id
		msg.size += sizeof(pFeature->id);

		NetAdd(msg,msg.size,pFeature->x);				// x
		msg.size += sizeof(pFeature->x);

		NetAdd(msg,msg.size,pFeature->y);				// y
		msg.size += sizeof(pFeature->y);

		NetAdd(msg,msg.size,pFeature->psStats->ref);	// type
		msg.size += sizeof(pFeature->psStats->ref);

		NetAdd(msg,msg.size,pFeature->body);			// body points
		msg.size += sizeof(pFeature->body);

		NetAdd(msg,msg.size,((UBYTE)pFeature->player) );			// player.
		msg.size += 1;

	}

	return(	NETsend(msg,player,TRUE));
}

// ////////////////////////////////////////////////////////////////////////////
//   features not found on other machines.
BOOL recvFeatures(NETMSG *pMsg)
{
	UDWORD		id,type,msgdepth,i,body,player;
	UWORD		x,y;
	for(msgdepth=0; msgdepth<pMsg->size ;msgdepth+=17) 							// go down the list
	{
		NetGet(pMsg,msgdepth,   id);											// Recv Msg.
		NetGet(pMsg,msgdepth+4, x);
		NetGet(pMsg,msgdepth+6, y);
		NetGet(pMsg,msgdepth+8,type);
		NetGet(pMsg,msgdepth+12,body);
		player = pMsg->body[msgdepth+16];

		for(i=0; (i<numFeatureStats) && (asFeatureStats[i].ref != type); i++);  // find structure target
		if(asFeatureStats[i].ref == type)
		{
			buildFeature(&(asFeatureStats[i]),x,y,TRUE);						// build a feature.
			apsFeatureLists[0]->id = id;										//set id, naughty i know.
			apsFeatureLists[0]->body = body;
			apsFeatureLists[0]->player = (UBYTE)player;
		}
	}
	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////////
// Process the droids we previously recevied, issuing the orders..
BOOL ProcessDroidOrders(VOID)
{
	UDWORD		i;
	DROID		*pD;
	UDWORD		TarRef;
	DROID_ORDER_DATA	sOrder;
	DROIDSTORE *pStore;

	if(tempDroidList == NULL)
	{
		DBPRINTF(("MULTIJOIN: NO DROIDS AT ALL! GULP!\n"));
		return TRUE;
	}

	pD = tempDroidList->psDroid;	//apsDroidLists[i];							// look at player i's droids.
	while( pD )
	{
		if(pD->psTarget)														// check for a target.
		{
			TarRef = (UDWORD) pD->psTarget;
			pD->psTarget = IdToPointer(TarRef,ANYPLAYER);

			if(pD->psTarget == NULL)											// target wasn't found. check list of things to add.
			{
				for(pStore = tempDroidList;pStore;pStore = pStore->psNext)
				{
					if(pStore->psDroid->id == TarRef)
					{
						pD->psTarget = (BASE_OBJECT*)pStore->psDroid;
						break;
					}
				}
			}
		}
		if( pD->psTarStats )													// if target has stats then find them
		{
			TarRef = (UDWORD) pD->psTarStats;
			pD->psTarStats = NULL;
			for(i=0; ( i<numStructureStats ) && (asStructureStats[i].ref != TarRef); i++);  // find structure target

			if(asStructureStats[i].ref == TarRef)
			{
				pD->psTarStats = (BASE_STATS *) &(asStructureStats[i]);
			}
			else
			{// wasn't a structure target. Must be a feature
				for(i=0; (i<numFeatureStats) && (asFeatureStats[i].ref != TarRef); i++);  // find structure target
				if(asFeatureStats[i].ref == TarRef)
				{
					pD->psTarStats = (BASE_STATS *) &(asFeatureStats[i]);
				}
			}
		}

		sOrder.order	= pD->order;											// set the order
		sOrder.x		= pD->orderX;
		sOrder.y		= pD->orderY;
		sOrder.psObj	= pD->psTarget;
		sOrder.psStats	= pD->psTarStats;

		if(pD->psTarget)											// double check to see if we caught it!
		{
			orderDroidBase(pD,&sOrder);								// issue the order.
		}
		else if(pD->order == DORDER_BUILD)							// building has no
		{															//	target in the early stages.
			orderDroidBase(pD,&sOrder);
		}

		addDroid(pD, apsDroidLists);								// add the droid to the world.

		pStore = tempDroidList->psNext;								// goto next droid.
		FREE(tempDroidList);
		tempDroidList = pStore;

		if(tempDroidList)
		{
			pD=tempDroidList->psDroid;
		}
		else
		{
			pD=NULL;
		}

	}

	return TRUE;
}
*/
// ////////////////////////////////////////////////////////////////////////////
// reduce the amount of oil that can be extracted.
void modifyResources(POWER_GEN_FUNCTION* psFunction)
{
	switch(game.power)
	{
	case LEV_LOW:
		psFunction->powerMultiplier = psFunction->powerMultiplier * 3/4 ;	// careful with the brackets! (do mul before div)
		break;
	case LEV_MED:
		psFunction->powerMultiplier = psFunction->powerMultiplier * 1;
		break;
	case LEV_HI:
		psFunction->powerMultiplier = psFunction->powerMultiplier * 5/4  ;
		break;
	default:
		break;
	}
	return;
}
