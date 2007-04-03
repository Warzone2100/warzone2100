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
 * MultiGift.c
 * gifts one player can give to another..
 * Also home to Deathmatch hardcoded RULES.
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"
#include "objmem.h"
#include "console.h"
#include "map.h"
#include "research.h"
#include "power.h"
#include "group.h"
#include "anim_id.h"
#include "hci.h"
#include "text.h"
#include "scriptfuncs.h"		// for objectinrange.
#include "lib/gamelib/gtime.h"
#include "effects.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"			// for samples.
#include "wrappers.h"			// for gameover..
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "loop.h"

#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multigifts.h"
#include "multiint.h"			// for force name.
#include "multimenu.h"			// for multimenu
#include "multistat.h"

//////////////////////////////////////////////////////////////////////////////

//#define DMATCH_DROID_LIMIT	25			// max number of droids in a dmatch game (per player).

#define	ENDFREQUENCY		5000		 // how often to check end game conditions
#define MAXFRAGS			10000		 // max score in a frag match.
#define MAXTIME				(5*60*1000) // max time in a time limit dmatch.

///////////////////////////////////////////////////////////////////////////////
// prototypes
BOOL			recvGift						(NETMSG *pMsg);
BOOL			sendGift						(UDWORD type,UDWORD to);
void			giftRadar						(UDWORD from, UDWORD to,BOOL send);
//static void		giftSingleDroid					(DROID *psD,UDWORD from,UDWORD to);
static void		recvGiftDroids					(UDWORD from,UDWORD to,NETMSG *pMsg);
static void		sendGiftDroids					(UDWORD from,UDWORD to);
static void		giftResearch					(UDWORD from,UDWORD to,BOOL send);
void			giftPower						(UDWORD from,UDWORD to,BOOL send);

void			requestAlliance					(UBYTE from ,UBYTE to,BOOL prop,BOOL allowAudio);
void			breakAlliance					(UBYTE p1, UBYTE p2,BOOL prop,BOOL allowAudio);
void			formAlliance					(UBYTE p1, UBYTE p2,BOOL prop,BOOL allowAudio,BOOL allowNotification);
void			sendAlliance					(UBYTE from, UBYTE to, UBYTE state,SDWORD value);
void			createTeamAlliances				(void);
BOOL			recvAlliance					(NETMSG *pMsg,BOOL allowAudio);
void			technologyGiveAway				(STRUCTURE *pS);
void			addMultiPlayerRandomArtifacts	(UDWORD quantity,SDWORD type);
void			recvMultiPlayerRandomArtifacts	(NETMSG *pMsg);
void			giftArtifact					(UDWORD owner,UDWORD x,UDWORD y);
void			processMultiPlayerArtifacts		(void);
//DROID_TEMPLATE *pickDistribTempl				(UDWORD player);
BOOL			addOilDrum						(UDWORD count);

//BOOL			addDMatchDroid					(UDWORD count);
//BOOL			foundDMatchDroid				(UDWORD player,UDWORD x,UDWORD y);
//BOOL			deathmatchCheck					(void);
//static BOOL		dMatchWinner					(UDWORD winplayer,BOOL bcast);
//BOOL			recvdMatchWinner				(NETMSG *pMsg);
void			addLoserGifts					(void);


///////////////////////////////////////////////////////////////////////////////
// gifts..

// recieve gift
BOOL recvGift(NETMSG *pMsg)
{
	UDWORD t,from,to;

	t    = pMsg->body[0];	//decode msg
	from = pMsg->body[1];
	to   = pMsg->body[2];


	switch(t)
	{
	case RADAR_GIFT:
		giftRadar(from,to,FALSE);
		break;
	case DROID_GIFT:
		recvGiftDroids(from,to,pMsg);
		break;
	case RESEARCH_GIFT:
		giftResearch(from,to,FALSE);
		break;
	case POWER_GIFT:
		giftPower(from,to,FALSE);
		break;
	default:
		debug( LOG_ERROR, "Unknown Gift recvd" );
		abort();
		return FALSE;
		break;
	}

	// play some audio.
	if(to == selectedPlayer)
	{
		audio_QueueTrack(ID_GIFT);
		switch(t)
		{
		case RADAR_GIFT:
			audio_QueueTrack(ID_SENSOR_DOWNLOAD);
			break;
		case DROID_GIFT:
			audio_QueueTrack(ID_UNITS_TRANSFER );
			break;
		case RESEARCH_GIFT:
			audio_QueueTrack(ID_TECHNOLOGY_TRANSFER);
			break;
		case POWER_GIFT:
			audio_QueueTrack(ID_POWER_TRANSMIT);
			break;
		default:
			break;
		}
	}

	return TRUE;
}


BOOL sendGift(UDWORD type,UDWORD to)
{
	switch(type)
	{
	case RADAR_GIFT:
		giftRadar(selectedPlayer,to,TRUE);
		audio_QueueTrack(ID_SENSOR_DOWNLOAD);
		break;
	case DROID_GIFT:
		sendGiftDroids(selectedPlayer,to);
		audio_QueueTrack(ID_UNITS_TRANSFER );
		break;
	case RESEARCH_GIFT:
		giftResearch(selectedPlayer,to,TRUE);
		audio_QueueTrack(ID_TECHNOLOGY_TRANSFER);
		break;
	case POWER_GIFT:
		giftPower(selectedPlayer,to,TRUE);
		audio_QueueTrack(ID_POWER_TRANSMIT);
		break;
	default:
		debug( LOG_ERROR, "Unknown Gift sent" );
		abort();
		return FALSE;
		break;
	}

	return TRUE;
}
// ////////////////////////////////////////////////////////////////////////////
// give radar information
void giftRadar(UDWORD from, UDWORD to,BOOL send)
{
	NETMSG m;

	hqReward((UBYTE)from, (UBYTE)to);

	if(send)
	{
		m.body[0] = RADAR_GIFT;
		m.body[1] = (UBYTE)from;
		m.body[2] = (UBYTE)to;
		m.type = NET_GIFT;
		m.size = 3;
		NETbcast(&m,TRUE);	//send it
	}
	else
	{
		if(to == selectedPlayer)
		{
			CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You A Visibility Report"),
				getPlayerName(from)));
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// give a droid - MOVED INTO DROID.C - AB 5/11/98

/*static void giftSingleDroid(DROID *psD,UDWORD from,UDWORD to)
{
}*/

static void recvGiftDroids(UDWORD from,UDWORD to,NETMSG *pMsg)
{

	UDWORD id,pos=3;
	DROID *pD;

	while(pos < pMsg->size)
	{
		NetGet(pMsg,pos,id);
		pos += sizeof(UDWORD);

		if(IdToDroid(id,from,&pD))	// find the droid.
		{
			//giftSingleDroid(pD,from,to);	// give it away.
            (void)giftSingleDroid(pD,to);	// give it away.
		}
	}

	if(to == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You Units"),getPlayerName(from) ));
	}
}




// give selected droid
static void sendGiftDroids(UDWORD from,UDWORD to)
{
	DROID	*next, *psD= apsDroidLists[from];
	NETMSG	m;
	UDWORD	count;

	if(!apsDroidLists[from])
	{
		return;
	}

	m.body[0] = DROID_GIFT;
	m.body[1] = (UBYTE)from;
	m.body[2] = (UBYTE)to;
	m.type = NET_GIFT;
	m.size = 3;

	count = 0;
	do{
		next = psD->psNext;		// store copy, since droid list may change.

		if(psD->selected)
		{
			// check if recv has too many droids..
	//		if(! IsPlayerDroidLimitReached(to) )
			if(getNumDroids(to)+count < getMaxDroids(to) )
			{
			   (void)giftSingleDroid(psD,to);

				NetAdd(m,m.size,psD->id);
				m.size += sizeof(psD->id);
				count++;
			}
		}

		psD = next;
	}while (psD);

	if(m.size >3)
	{
		NETbcast(&m,TRUE);	//send it
	}
}


// ////////////////////////////////////////////////////////////////////////////
// give technologies.
static void giftResearch(UDWORD from,UDWORD to,BOOL send)
{
	PLAYER_RESEARCH *pR,*pRto;
	UDWORD i;
	NETMSG m;

	pR	 = asPlayerResList[from];
	pRto = asPlayerResList[to];

	for(i=0; i<numResearch; i++)								// do for each topic.
	{
		if(IsResearchCompleted(&pR[i]) )
		{
			if(IsResearchCompleted(&pRto[i])==FALSE)
			{
				MakeResearchCompleted(&pRto[i]);
				researchResult(i,(UBYTE)to,FALSE);
			}
		}
	}


/*	pPlayerRes = asPlayerResList[player];
	pPlayerRes += index;
	if(IsResearchCompleted(pPlayerRes)==FALSE)
	{
		MakeResearchCompleted(pPlayerRes);
		rese
*/
	if(send)
	{
		m.body[0] = RESEARCH_GIFT;
		m.body[1] = (UBYTE)from;
		m.body[2] = (UBYTE)to;
		m.type = NET_GIFT;
		m.size = 3;
		NETbcast(&m,TRUE);	//send it
	}
	else
	{
		if(to == selectedPlayer)
		{
			CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You Technology Documents"),getPlayerName(from) ));
		}
	}

}


// ////////////////////////////////////////////////////////////////////////////
// give Power
void giftPower(UDWORD from,UDWORD to,BOOL send)
{
	UDWORD gifval;
	NETMSG m;

	if(from == ANYPLAYER)
	{
		gifval = OILDRUM_POWER;
	}
	else
	{
		gifval = asPower[from]->currentPower /3;
//		asPower[from]->currentPower -= gifval;
		usePower(from, gifval);
	}

	addPower(to,gifval);

	if(send)
	{
		m.body[0] = POWER_GIFT;
		m.body[1] = (UBYTE)from;
		m.body[2] = (UBYTE)to;
		m.type = NET_GIFT;
		m.size = 3;
		NETbcast(&m,TRUE);	//send it
	}
	else
	{
		if(to == selectedPlayer)
		{
			CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You Power"),getPlayerName(from) ));
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// alliance code......

void requestAlliance(UBYTE from ,UBYTE to,BOOL prop,BOOL allowAudio)
{
	alliances[from][to] = ALLIANCE_REQUESTED;			// we've asked.
	alliances[to][from] = ALLIANCE_INVITATION;		// they've been invited.


	CBallFrom = from;
	CBallTo = to;
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_ALLIANCEOFFER);

	if(to == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Requests An Alliance With You"),getPlayerName(from) ));
		if(allowAudio)
		{
			audio_QueueTrack(ID_ALLIANCE_OFF);
		}
	}
	if(from == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("You Invite %s To Form An Alliance"),getPlayerName(to) ));
		if(allowAudio)
		{
			audio_QueueTrack(ID_ALLIANCE_OFF);
		}
	}

	if(prop)
	{
		sendAlliance(from,to,ALLIANCE_REQUESTED,0);
	}
}

void breakAlliance(UBYTE p1, UBYTE p2,BOOL prop,BOOL allowAudio)
{
	char	tm1[128];
	if(alliances[p1][p2] == ALLIANCE_FORMED)
	{
		strcpy(tm1,getPlayerName(p1));
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Breaks The Alliance With %s"),tm1,getPlayerName(p2) ));
		if(allowAudio && (p1 == selectedPlayer || p2 == selectedPlayer))
		{
			audio_QueueTrack(ID_ALLIANCE_BRO);
		}
	}

	alliances[p1][p2] = ALLIANCE_BROKEN;
	alliances[p2][p1] = ALLIANCE_BROKEN;

	if(prop)
	{
		sendAlliance(p1,p2,ALLIANCE_BROKEN,0);
	}
}

void formAlliance(UBYTE p1, UBYTE p2,BOOL prop,BOOL allowAudio,BOOL allowNotification)
{
	DROID	*psDroid;
	char	tm1[128];
	UBYTE	i;

	// dont add message if already allied,
	if(bMultiPlayer && !(alliances[p1][p2] == ALLIANCE_FORMED) && allowNotification )
	{
		strcpy(tm1,getPlayerName(p1));
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Forms An Alliance With %s"),tm1,getPlayerName(p2) ));
	}

	alliances[p1][p2] = ALLIANCE_FORMED;
	alliances[p2][p1] = ALLIANCE_FORMED;

	//make sure they can see our base location


	if(allowAudio && (p1 == selectedPlayer || p2== selectedPlayer))
	{
		audio_QueueTrack(ID_ALLIANCE_ACC);
	}

	if(bMultiPlayer)//jps 15apr99
	{
		if(prop)
		{
			sendAlliance(p1,p2,ALLIANCE_FORMED,0);
		}
	}

	// teamplay init others vis.
	if(bMultiPlayer && game.type == TEAMPLAY && (p1 == selectedPlayer || p2== selectedPlayer))
	{
		// THIS BIT ADDED BY AJL 28th april
		for(i=0;i<MAX_PLAYERS;i++)
		{
			//for each alliance with the selectedPlayer make a new one with the newly allied player
			if(p2 == selectedPlayer)
			{
				if ( p1 != i && p2 != i && aiCheckAlliances(p1,i) )
				{
					if (!aiCheckAlliances(p2,i))
					{
						formAlliance(p2,i,TRUE,FALSE,TRUE);
					}
				}
			}
			else
			{
				if( p1 != i && p2 != i && aiCheckAlliances(p2,i) )
				{
					if (!aiCheckAlliances(p1,i))
					{
						formAlliance(p1,i,TRUE,FALSE,TRUE);
					}
				}
			}
		}
	}
	else if((bMultiPlayer || game.type == SKIRMISH) && game.alliance == ALLIANCES_TEAMS)	//not campaign and alliances are transitive
	{
		giftRadar(p1,p2,FALSE);
		giftRadar(p2,p1,FALSE);
	}

	// clear out any attacking orders.
	turnOffMultiMsg(TRUE);
	for(psDroid= apsDroidLists[p1];psDroid;psDroid=psDroid->psNext)	// from -> to
	{
		if((psDroid->order == DORDER_ATTACK ||
			psDroid->order == DORDER_ATTACK_M)
			&& psDroid->psTarget[0]
			&& psDroid->psTarget[0]->player == p2)
		{
			orderDroid(psDroid,DORDER_STOP);
		}
	}
	for(psDroid= apsDroidLists[p2];psDroid;psDroid=psDroid->psNext)	// to -> from
	{
		if((psDroid->order == DORDER_ATTACK ||
			psDroid->order == DORDER_ATTACK_M)
			&& psDroid->psTarget[0]
 			&& psDroid->psTarget[0]->player == p1)
		{
			orderDroid(psDroid,DORDER_STOP);
		}
	}
	turnOffMultiMsg(FALSE);

}



void sendAlliance(UBYTE from, UBYTE to, UBYTE state,SDWORD value)
{
	NETMSG m;

	m.size =0;
	NetAdd(m,m.size,from);
	m.size+=sizeof(from);

	NetAdd(m,m.size,to);
	m.size+=sizeof(to);

	NetAdd(m,m.size,state);
	m.size+=sizeof(state);

	NetAdd(m,m.size,value);
	m.size+=sizeof(value);

	m.type = NET_ALLIANCE;
	NETbcast(&m,TRUE);

/*
	// teamplay init others vis.
	if(bMultiPlayer && state == ALLIANCE_FORMED && game.type == TEAMPLAY && (from == selectedPlayer || to== selectedPlayer))
	{
		giftRadar(from,to,FALSE);
		giftRadar(to,from,FALSE);
	}
*/
}

BOOL recvAlliance(NETMSG *pMsg,BOOL allowAudio)
{
	UBYTE to,from,state,pos=0;
	SDWORD value;

	NetGet(pMsg,pos,from);
	pos += sizeof(from);

	NetGet(pMsg,pos,to);
	pos += sizeof(to);

	NetGet(pMsg,pos,state);
	pos += sizeof(state);

	NetGet(pMsg,pos,value);

	switch(state)
	{
	case ALLIANCE_NULL:
		break;
	case ALLIANCE_REQUESTED:
		requestAlliance(from,to,FALSE,allowAudio);
		break;
	case ALLIANCE_FORMED:
		formAlliance(from,to,FALSE,allowAudio,TRUE);
		break;
	case ALLIANCE_BROKEN:
		breakAlliance(from,to,FALSE,allowAudio);
		break;
	default:
		debug( LOG_ERROR, "Unknown alliance state recvd." );
		abort();
		break;
	}

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// add an artifact on destruction if required.
void  technologyGiveAway(STRUCTURE *pS)
{
	UDWORD	i;
	UDWORD	x,y;
	UWORD	nx,ny;
	FEATURE	*pF=NULL;
	SDWORD	type = FEAT_GEN_ARTE;
	NETMSG	m;

	if(pS->pStructureType->type == REF_FACTORY  &&  myResponsibility(pS->player))
	{

		x = (pS->x >> TILE_SHIFT);
		y = (pS->y >> TILE_SHIFT);
		if (!pickATileGen(&x,&y,LOOK_FOR_EMPTY_TILE,zonedPAT))
		{
			ASSERT( FALSE, "technologyGiveAway: Unable to find a free location" );
		}

		for(i=0; (i<numFeatureStats) && (asFeatureStats[i].subType != FEAT_GEN_ARTE); i++);
		pF = buildFeature((asFeatureStats+i),x<<TILE_SHIFT, y<<TILE_SHIFT,FALSE);
		if(pF)
		{
			pF->player = pS->player;
		}

		m.body[0] = (UBYTE) 1;		// note how many
		m.size	  = 1;

		// type.
		NetAdd(m,m.size,type);
		m.size += sizeof(type);

		nx = (UWORD)x;
		NetAdd(m,m.size,nx);
		m.size += sizeof(UWORD);

		ny = (UWORD)y;
		NetAdd(m,m.size,ny);
		m.size += sizeof(UWORD);

		NetAdd(m,m.size,pF->id);
		m.size += sizeof(pF->id);

		m.body[m.size] = (UBYTE) pS->player;
		m.size	+=1;

		m.type  = NET_ARTIFACTS;	// send it.
		NETbcast(&m,FALSE);		// tell everyone.

	}
	return;
}



///////////////////////////////////////////////////////////////////////////////
// loooosseeeerrr  gifts
// if the player is losing then give a simple gift.

#define GIFTFREQ (1000*(60*5))			// every 5 mins tops..

void addLoserGifts(void)
{
//	DROID			*psD;
//	DROID_TEMPLATE	*psTempl;
//	Vector3i			position;
	static UDWORD	lastgift=0;
	UDWORD			i,x,y,quantity,count;
	UWORD			nx,ny;
	NETMSG			m;
	FEATURE			*pF;
	SDWORD			type = FEAT_OIL_DRUM;
	STRUCTURE		*psStruct;

	if(lastgift>gameTime)lastgift=0;	// might be a restart

	// player has no power
	if(apsStructLists[selectedPlayer] && asPower[selectedPlayer]->currentPower < 10)	// give some oil drums.
	{
		// only proceed if it's been a while
		if(gameTime - lastgift< GIFTFREQ)
		{
			return;
		}

		// only proceed if no powergen.
		for(psStruct = apsStructLists[selectedPlayer];
			psStruct && psStruct->pStructureType->type != REF_POWER_GEN;
			psStruct = psStruct->psNext);
		if(psStruct)
		{
			return;
		}

		lastgift = gameTime;

		for(i=0; (i<numFeatureStats) && (asFeatureStats[i].subType != FEAT_OIL_DRUM); i++);
		quantity = rand()%5+1;

		m.size  = 1;
		m.body[0] = (UBYTE) quantity;		// note how many

		NetAdd(m,m.size,type);
		m.size += sizeof(type);

		for(count = 0;count<quantity;count++)
		{
			x = apsStructLists[selectedPlayer]->x >> TILE_SHIFT;
			y = apsStructLists[selectedPlayer]->y >> TILE_SHIFT;
			if (!pickATileGen(&x,&y,LOOK_FOR_EMPTY_TILE,zonedPAT))
			{
				ASSERT( FALSE, "addlosergifts: Unable to find a free location" );
			}

			NETlogEntry("gift",0,0);

			pF = buildFeature((asFeatureStats+i),x<<TILE_SHIFT, y<<TILE_SHIFT,FALSE);

			nx = (UWORD)x;
			ny = (UWORD)y;
			NetAdd(m,m.size,x);			m.size += sizeof(UWORD);
			NetAdd(m,m.size,x);			m.size += sizeof(UWORD);
			NetAdd(m,m.size,pF->id);			m.size += sizeof(pF->id);

			m.body[m.size]  = ONEPLAYER;
			m.size +=1;
			if(pF)
			{
				pF->player = ONEPLAYER;		// flag for multiplayer artifacts
			}
		}
		audio_QueueTrack(ID_GIFT);
		m.type  = NET_ARTIFACTS;
		NETbcast(&m,FALSE);		// tell everyone.

	}

/* removed. too confusing.. con droids all over!
	// player has no construction droids
	for(psD=apsDroidLists[selectedPlayer];(psD != NULL)&&(psD->droidType !=DROID_CONSTRUCT);psD = psD->psNext);
	if(!psD)
	{
		for(psTempl=apsDroidTemplates[selectedPlayer];
		psTempl && (psTempl->asParts[COMP_CONSTRUCT] == 0);
		psTempl = psTempl->psNext);

		if(psTempl)
		{
			// give player a construction Droid.right now!
			if(apsStructLists[selectedPlayer])
			{
				x = apsStructLists[selectedPlayer]->x >>TILE_SHIFT;
				y = apsStructLists[selectedPlayer]->y >>TILE_SHIFT;
				z = apsStructLists[selectedPlayer]->z >>TILE_SHIFT;

				pickATileGen(&x,&y,LOOK_FOR_EMPTY_TILE,normalPAT);

				position.x = x<<TILE_SHIFT;				// Add an effect
				position.z = y<<TILE_SHIFT;
				position.y = z<<TILE_SHIFT;

				if(gameTime - lastgift< GIFTFREQ)
				{
					return;
				}
				lastgift = gameTime;
				powerCalc(FALSE);
				psD=buildDroid(	psTempl, x<<TILE_SHIFT,  y<<TILE_SHIFT, selectedPlayer, FALSE);
				if(psD)
				{
					audio_QueueTrack(ID_GIFT);
					addDroid(psD,apsDroidLists);							// add droid. telling everyone
					addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_DISCOVERY,FALSE,NULL,FALSE);
				}
				powerCalc(TRUE);											// power back on.
			}
		}
	}
	*/
}


///////////////////////////////////////////////////////////////////////////////
// splatter artifact gifts randomly about.
void  addMultiPlayerRandomArtifacts(UDWORD quantity,SDWORD type)
{
	NETMSG m;
	FEATURE	*pF;
	UDWORD i,count;
	UDWORD	x,y;
	UWORD	nx,ny;

	m.type  = NET_ARTIFACTS;
	m.size  = 1;
	m.body[0] = (UBYTE) quantity;		// note how many

	NetAdd(m,m.size,type);
	m.size += sizeof(type);

	for(i=0; (i<numFeatureStats) && (asFeatureStats[i].subType != type); i++);

	ASSERT( mapWidth>20,"map not big enough" );
	ASSERT( mapHeight>20,"map not big enough" );

	for(count = 0;count<quantity;count++)
	{
		x = (rand()% (mapWidth-20))+10 ;		//( between 10 and mapwidth-10)
		y = (rand()% (mapHeight-20))+10 ;
		if (!pickATileGen(&x,&y,LOOK_FOR_EMPTY_TILE,zonedPAT))
		{
			ASSERT( FALSE, "addMultiPlayerRandomArtifacts: Unable to find a free location" );
		}

		pF = buildFeature((asFeatureStats+i),x<<TILE_SHIFT, y<<TILE_SHIFT,FALSE);

		nx = (UWORD)x;
		ny = (UWORD)y;
		NetAdd(m,m.size,nx);
		m.size += sizeof(UWORD);
		NetAdd(m,m.size,ny);
		m.size += sizeof(UWORD);

		NetAdd(m,m.size,pF->id);
		m.size += sizeof(pF->id);

		m.body[m.size]  = ANYPLAYER;
		m.size +=1;
		if(pF)
		{
			pF->player = ANYPLAYER;		// flag for multiplayer artifacts
		}
	}

	NETbcast(&m,FALSE);		// tell everyone.
}

// ///////////////////////////////////////////////////////////////
BOOL addDMatchDroid(UDWORD count)
{
	addMultiPlayerRandomArtifacts(count,FEAT_GEN_ARTE);
	return TRUE;
}

// ///////////////////////////////////////////////////////////////
BOOL addOilDrum(UDWORD count)
{
	addMultiPlayerRandomArtifacts(count,FEAT_OIL_DRUM);
	return TRUE;
}

// ///////////////////////////////////////////////////////////////
// receive splattered artifacts
void recvMultiPlayerRandomArtifacts(NETMSG *pMsg)
{
	UDWORD index,count,i,quantity,ref;
	SDWORD type;
	FEATURE *pF;
	UWORD	tx,ty;

	quantity = (UDWORD) pMsg->body[0];
	index = 1;

	NetGet(pMsg,index,type);
	index += sizeof(type);

	for(i=0; (i<numFeatureStats) && (asFeatureStats[i].subType != type); i++);

	for(count = 0;count<quantity;count++)
	{
		NetGet(pMsg,index,tx);
		index += sizeof(tx);

		NetGet(pMsg,index,ty);
		index += sizeof(ty);

		NetGet(pMsg,index,ref);
		index += sizeof(ref);
		pF = buildFeature((asFeatureStats+i),tx<<TILE_SHIFT, ty<<TILE_SHIFT,FALSE);
		if(pF)
		{
			pF->id		= ref;
			pF->player	= pMsg->body[index];		// flag for multiplayer artifacts
			index+=1;
		}
	}
}

// ///////////////////////////////////////////////////////////////
void giftArtifact(UDWORD owner,UDWORD x,UDWORD y)
{
	PLAYER_RESEARCH *pO,*pR;
	UDWORD	topic=0;
	pR   = asPlayerResList[selectedPlayer];

	if(owner == ANYPLAYER)
	{
//		foundDMatchDroid(selectedPlayer,x,y);
	}
	else if(owner >= MAX_PLAYERS)	//1.04 bodge to stop savegame crash
	{
		return;
	}
	else	// steal a research topic from owner player.
	{
		pO	 = asPlayerResList[owner];

		for(topic =numResearch; topic>0; topic--)
		{
			if( (IsResearchCompleted(&pO[topic]) ) && (IsResearchPossible(&pR[topic])==FALSE )  )
			{
				MakeResearchPossible(&pR[topic]);
				CONPRINTF(ConsoleString,(ConsoleString,_("You Discover Blueprints For %s"),
					getName(asResearch[topic].pName)));
				return;
			}
		}
	}
}

// ///////////////////////////////////////////////////////////////
void processMultiPlayerArtifacts(void)
{
	static UDWORD lastCall;
	FEATURE	*pF,*pFN;
	UDWORD	x,y,pl;
	Vector3i position;
	BOOL	found=FALSE;

	// only do this every now and again.
	if(lastCall > gameTime)lastCall= 0;
	if ( (gameTime - lastCall) <2000)
	{
		return;
	}
	lastCall = gameTime;

	addLoserGifts();

	for(pF = apsFeatureLists[0]; pF ; pF = pFN)
	{
		pFN = pF->psNext;
		// artifacts
		if(pF->psStats->subType == FEAT_GEN_ARTE)
		{
			found = objectInRange((BASE_OBJECT *)apsDroidLists[selectedPlayer], pF->x,pF->y,(TILE_UNITS+(TILE_UNITS/3))  );
			if(found)
			{
				position.x = pF->x;				// Add an effect
				position.z = pF->y;
				position.y = pF->z;
				addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_DISCOVERY,FALSE,NULL,FALSE);

//				if(game.type == DMATCH)
//				{
//					x = pF->x;
//					y = pF->y;
//					removeFeature(pF);			// remove artifact+ send info.
//					foundDMatchDroid(selectedPlayer, x,y);
//					addDMatchDroid(1);
//				}
//				else
//				{
					x = pF->x;
					y = pF->y;
					pl= pF->player;
					removeFeature(pF);			// remove artifact+ send info.
					giftArtifact(pl,x,y);		// reward player.
					pF->player = 0;
					audio_QueueTrack( ID_SOUND_ARTIFACT_RECOVERED );
//				}
			}
		}


		// oil drums
//		if(pF->psStats->subType == FEAT_OIL_DRUM)
//		{
//			found = objectInRange((BASE_OBJECT *)apsDroidLists[selectedPlayer], pF->x,pF->y,(TILE_UNITS+(TILE_UNITS/3))  );
//			if(found)
//			{
//				giftPower(ANYPLAYER,selectedPlayer,TRUE);		// give power and tell everyone.
//				removeFeature(pF);								// remove artifact+ send info.
//				addOilDrum(1);
//			}
//		}

	}

}


/* Ally team members with each other */
void createTeamAlliances(void)
{
	UDWORD i,j;

	debug(LOG_WZ, "Creating teams");

	for(i=0; i<MAX_PLAYERS; i++ )
	{
		for(j=0; j<MAX_PLAYERS; j++ )
		{
			if( i!=j && (playerTeam[i] == playerTeam[j])			//wto different players belonging to the same team
				&& !aiCheckAlliances(i,j) && (playerTeam[i] >= 0))	//not allied and not ignoring teams
			{
				if(game.skDiff[i] && game.skDiff[j])		//make sure both players are enabled
					formAlliance(i,j,FALSE,FALSE,FALSE);		//create silently
			}
		}
	}

}


/*
// ///////////////////////////////////////////////////////////////
// deathmatch Stuff find/add droids using artifacts.

// pick a template approximating the normal distribution around powerpoints.
DROID_TEMPLATE * pickDistribTempl(UDWORD player)
{
	DROID_TEMPLATE *psTempl;
	UDWORD	i,rn,rt,dist,min=UDWORD_MAX,max=0,av=0,num=0;

	// gather data
	for(psTempl=apsDroidTemplates[player];psTempl;psTempl=psTempl->psNext)
	{
		num++;
		av += psTempl->powerPoints;
		if(psTempl->powerPoints < min) min = psTempl->powerPoints;
		if(psTempl->powerPoints > max) max = psTempl->powerPoints;
	}

	av = av / num; 		// get average build pts.
	if(av-min > max-av)	// get furthest from average.
	{
		max = min;
	}

	rn = (rand()% abs(max-av) )+ av;	// pick a test level. av - max.
	rn = abs(rn - av);					// rn is now a random distance from av.

	dist = 0;					//init distn

	while(dist <= rn)
	{
		//pick a template
		rt = (rand()%num);	// template to pick
		i =0;
		for(psTempl=apsDroidTemplates[player];i<rt;psTempl=psTempl->psNext)
		{
			i++;
		}

		// distance of this template
		dist = abs(psTempl->powerPoints - av);
	}
	return psTempl;

}
// ///////////////////////////////////////////////////////////////
// player runs into a dmatch artifact
BOOL foundDMatchDroid(UDWORD player,UDWORD x,UDWORD y)
{
	DROID_TEMPLATE *psTempl;
	DROID	*psD;
	UDWORD	count=0;

	// check to see if player has too many droids.
	for(psD = apsDroidLists[player];psD; psD=psD->psNext)
	{
		count++;
	}
	if(count >= DMATCH_DROID_LIMIT)
	{
		return TRUE;
	}

	psTempl = pickDistribTempl(player);	//pick a droid

	// no longer need to turn off power, but just in case.
	powerCalc(FALSE);					//turn off power temporarily.

	psD=buildDroid(	psTempl,  x,  y,  player, FALSE);
	if(psD)
	{
		addDroid(psD,apsDroidLists);	// add droid. telling everyone
	}
	powerCalc(TRUE);					// power back on.

	return TRUE;
}
*/

/*
// ///////////////////////////////////////////////////////////////
// check for respawn or win situations.
BOOL deathmatchCheck(void)
{
	UDWORD			pl,maxScP;
	SDWORD			maxSc;
	static BOOL		gameComplete=FALSE;
	static UDWORD	lastCheck=0;
	char			sTemp[256];

	// respawn check
	if(apsDroidLists[selectedPlayer] == NULL)
	{
		setPower(selectedPlayer,LEV_HI);		// reset power.
		sendPowerCheck(TRUE);					// tell everyone.

		strcpy(sTemp, "multiplay/forces/");
		strcat(sTemp, sForceName);
		strcat(sTemp,".for");
		loadForce( sTemp);

		useTheForce(FALSE);

		cameraToHome(selectedPlayer,FALSE);			// move camera.
	}


	// go no further if recently called.
	if(lastCheck > gameTime)lastCheck = 0;
	if(gameTime-lastCheck < ENDFREQUENCY)
	{
		return TRUE;
	}
	lastCheck = gameTime;

	// end game conditions.
	// fraglimit / timelimit.
	if(NetPlay.bHost && !gameComplete)
	{
		switch(game.limit)
		{
		case FRAGLIMIT:		// check for fraglimit => FRAGLIMIT
			for(pl=0; pl<MAX_PLAYERS;pl++)
			{
				if(isHumanPlayer(pl) && getMultiStats(pl,TRUE).recentScore >= MAXFRAGS)
				{
					gameComplete = TRUE;
					dMatchWinner(pl,TRUE);
					break;
				}
			}
			break;

		case TIMELIMIT:		// check for timelimit exceeded.
			if(gameTime-ingame.startTime > MAXTIME)
			{
				// pick a winner.
				maxScP = 0;
				maxSc  = 0;
				for(pl=0; pl<MAX_PLAYERS;pl++)
				{
					if(isHumanPlayer(pl) && getMultiStats(pl,FALSE).recentScore  > maxSc)
					{
						maxSc  = getMultiStats(pl,FALSE).recentScore;
						maxScP = pl;
					}
				}
				gameComplete == TRUE;
				dMatchWinner(pl,TRUE);
			}
			break;

		default:
			break;
		}


	}

	return TRUE;
}

// ///////////////////////////////////////////////////////////////
// Winner recieved.

BOOL recvdMatchWinner(NETMSG *pMsg)
{
	return dMatchWinner( (UDWORD)pMsg->body[0] ,FALSE);
}

// ///////////////////////////////////////////////////////////////
// Winner.
static BOOL dMatchWinner(UDWORD winplayer,BOOL bcast)
{
	NETMSG m;

	if(bcast)
	{
		m.type = NET_DMATCHWIN;		// send a netmsg.
		m.size = 1;
		m.body[0] = (UBYTE)winplayer;
		NETbcast(m,TRUE);
	}

	// declare player winner/looser.
	if(winplayer == selectedPlayer)
	{
		displayGameOver(TRUE);		// winner.
	}
	else
	{
		displayGameOver(FALSE);		// looooseeer.
	}

	// throw up the multimenu
	if(widgGetFromID(psWScreen,MULTIMENU_FORM) == NULL)
	{
		intAddMultiMenu();
	}

	// add some effects??

	// fire up a new level????????

	return TRUE;
}

*/
