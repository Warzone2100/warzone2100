/*
 * MultiStat.c
 *
 * Alex Lee , pumpkin studios, EIDOS
 *
 * load / update / store multiplayer statistics for league tables etc...
 * Also handle the force save/loads and default teams for each tech level.
 */

#include <stdio.h>
#include "lib/framework/frame.h"
#include "objmem.h"
#include "power.h"
#include "map.h"
#include "lib/widget/widget.h"
#include "text.h"
#include "effects.h"	// for discovery flash
#include "lib/netplay/netplay.h"
#include "cmddroid.h"
#include "multiplay.h"
#include "multirecv.h"
#include "multistat.h"
#include "multiint.h"
#include "fpath.h"

extern char	MultiPlayersPath[255];

// ////////////////////////////////////////////////////////////////////////////
//  Force defs.

BOOL		removeFromForce			(UDWORD number);					// remove a droid from force
BOOL		addToForce				(DROID_TEMPLATE  *templ);			// add a droid (templ) to force
VOID		useTheForce				(BOOL bAddTempl);					// place the force in the game
BOOL		saveForce				(char *name,FORCE *pfForce);
BOOL		loadForce				(char *name);

FORCE		Force;														// the selected force.

// ////////////////////////////////////////////////////////////////////////////
//  Player Stat defs.
BOOL		saveMultiStats			(STRING *sFName, STRING *sPlayerName, PLAYERSTATS *plStats);
BOOL		loadMultiStats			(STRING *sPlayerName, PLAYERSTATS *plStats);
PLAYERSTATS getMultiStats			(UDWORD	player, BOOL bLocal);
BOOL		setMultiStats			(DWORD playerDPID, PLAYERSTATS plStats,BOOL bLocal);

VOID		updateMultiStatsDamage	(UDWORD attacker, UDWORD defender, UDWORD inflicted);
VOID		updateMultiStatsGames	(void);
VOID		updateMultiStatsWins	(void);
VOID		updateMultiStatsLoses	(void);
VOID		updateMultiStatsKills	(BASE_OBJECT *psKilled, UDWORD player);


// ////////////////////////////////////////////////////////////////////////////
// FORCE SELECT STUFF
// ////////////////////////////////////////////////////////////////////////////

// ////////////////////////////////////////////////////////////////////////////
// funcs to edit the force.

BOOL addToForce(DROID_TEMPLATE  *templ)
{
	FORCE_MEMBER	*pF;
	DROID_TEMPLATE	*pT,*psTempl;

	if (!usePower(selectedPlayer, templ->powerPoints))		//subtract the power required to build
	{
		return FALSE;										// go no further
	}

	// add template to list. if it doesn't exist.
	for( psTempl = Force.pForceTemplates;							// find relevant template
		 psTempl && (psTempl->ref != templ->ref);
		 psTempl = psTempl->psNext);
	if(!psTempl)
	{
		pT = (DROID_TEMPLATE*)MALLOC(sizeof(DROID_TEMPLATE));
		if ( !pT)
		{
			return FALSE;
		}
		memcpy(pT, templ, sizeof(DROID_TEMPLATE));

		// calculate power etc for this template
//		initTemplatePoints();
		pT->buildPoints = calcTemplateBuild(pT);
		pT->powerPoints = calcTemplatePower(pT);

		pT->psNext = Force.pForceTemplates;
		Force.pForceTemplates=pT;
	}
	else
	{
		pT = psTempl;		// set up to point to existing template.
	}

	// add droid.
	pF = MALLOC(sizeof(FORCE_MEMBER));						// create a slot in the force.
	if (!pF)
	{
		return FALSE;
	}
	pF->pTempl  = pT;										// add this droid.
	pF->psNext  = Force.pMembers;
	Force.pMembers = pF;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL removeFromForce(UDWORD number)
{
	UDWORD i,templateid;
	FORCE_MEMBER *pF;
	FORCE_MEMBER *pF2=NULL;
	DROID_TEMPLATE	*psCurr,*psPrev;

	BOOL	inuse;

	pF= Force.pMembers;
	for(i=0;i<number;i++)pF=pF->psNext;						// goto that force element;
	addPower(selectedPlayer, pF->pTempl->powerPoints);		// return that much power.

	if(number==0)											// if first remove it,
	{
		pF = Force.pMembers;
		Force.pMembers = pF->psNext;
	}
	else
	{
		pF = Force.pMembers;									// not first so linked list
		for(i=0;i<number;i++)									// needs modifying..
		{
			pF2 = pF;
			pF  = pF->psNext;
		}
		pF2->psNext = pF->psNext;
	}

	templateid = pF->pTempl->ref;
	FREE(pF);

	// now check if template is still in use.
	inuse = FALSE;
	for(pF = Force.pMembers;pF;pF=pF->psNext)
	{
		if(pF->pTempl->ref == templateid)
		{
			inuse = TRUE;
		}
	}

	if(!inuse)		// remove template, no longer needed.
	{
		psPrev = NULL;
		for(psCurr = Force.pForceTemplates; psCurr;psCurr = psCurr->psNext)
		{
			if( psCurr->ref == templateid )
			{
				break;
			}
			psPrev = psCurr;
		}

		if (psCurr)										// if we found itthen delete it.
		{
			if(psPrev)									// Update list pointers.
			{
				psPrev->psNext = psCurr->psNext;		// It's down the list somewhere
			}
			else
			{
				Force.pForceTemplates = psCurr->psNext;	// It's at the root
			}
			FREE(psCurr);			// Delete the template.
		}
	}

	return TRUE;
}

/*
// find a place for the force
VOID chooseForceLoc(UDWORD *pX,UDWORD *pY)
{
	FEATURE			*pFeat;
	UDWORD			x,y,chose,tcount=0;

	// pick a boulder on the arena map
	for(pFeat=apsFeatureLists[0];pFeat;pFeat=pFeat->psNext)//count boulders
	{
		if(pFeat->psStats->subType == FEAT_BOULDER)
		{
			tcount++;
		}
	}

	if(tcount)												// use boulders
	{
		chose = 0;
		while(chose == 0)									// dont pick zeroth boulder.
		{
			chose  = (rand()%(tcount+1));
		}

		tcount= 0;
		pFeat=apsFeatureLists[0];
		while(pFeat && (tcount!=chose))
		{
			if(pFeat->psStats->subType == FEAT_BOULDER)
			{
				tcount++;
			}
			if(tcount != chose)
			{
				pFeat = pFeat->psNext;
			}
		}
		x = pFeat->x >>TILE_SHIFT;
		y = pFeat->y >>TILE_SHIFT;
	}
	else													//dont use boulders
	{
		tcount = 0;
		x = rand()%mapWidth;								//choose an intitial x/y pos.
		y = rand()%mapHeight;
	}

	// set result
	*pX = x;
	*pY = y;
}
*/

// ////////////////////////////////////////////////////////////////////////////
// place the force on the map.
VOID useTheForce(BOOL bAddTempl)//Luke
{
	DROID			*pDr;
	DROID_TEMPLATE	*pTempl;
	iVector			position;
	UDWORD			x1,y1,x,y;
	DROID_TEMPLATE *psTempl;

//	if(game.type == DMATCH)
//	{
//		chooseForceLoc(&x,&y);
//	}
//	if(game.type == CAMPAIGN)
//	{
		if(apsDroidLists[selectedPlayer])
		{
			//set drop off point to pos of other droids.
			x = apsDroidLists[selectedPlayer]->x >>TILE_SHIFT;
			y = apsDroidLists[selectedPlayer]->y >>TILE_SHIFT;

			// send each of the extra templates used in the force
			for(psTempl=Force.pForceTemplates;psTempl;psTempl=psTempl->psNext)
			{
				sendTemplate(psTempl);			// OUCH! REALLY COM HEAVY
			}

		}
//	}

	x1 = x;													// now we have a coord, place droids
	y1 = y;

	while(Force.pMembers)									// for each force member
	{
		x = x1;
		y = y1;

		if (!pickATileGen(&x,&y,LOOK_FOR_EMPTY_TILE,zonedPAT))
		{
			ASSERT((FALSE, "UseTheForce: Unable to find a free location"));
		}
		DBPRINTF(("force droid dropping at :%d,%d\n",x,y));

		// copy template
		psTempl = NameToTemplate(Force.pMembers->pTempl->aName,selectedPlayer);
		if(!psTempl)
		{
			Force.pMembers->pTempl->ref = REF_TEMPLATE_START;
			addTemplate(selectedPlayer, Force.pMembers->pTempl);
		}
		// end of template copy
		pTempl = Force.pMembers->pTempl;

/*		if(!psTempl)												// already exists.
		{
			if (HEAP_ALLOC(psTemplateHeap, &psTempl))
			{
				memcpy(psTempl, Force.pMembers->pTempl, sizeof(DROID_TEMPLATE));
				psTempl->ref = REF_TEMPLATE_START;					// templates are the odd one out!
				if (apsDroidTemplates[selectedPlayer])						// Add it to the list
				{
					for(psCurr = apsDroidTemplates[selectedPlayer];
						psCurr->psNext != NULL;
						psCurr = psCurr->psNext
						);
					psCurr->psNext = psTempl;
					psTempl->psNext = NULL;
				}
			}
		}
		else
		{
			apsDroidTemplates[selectedPlayer]=psTempl;
			psTempl->psNext = NULL;
		}
*/



//		if(usePower(selectedPlayer, pTempl->powerPoints))
//		{
			pDr= buildDroid(pTempl, x<<TILE_SHIFT, y<<TILE_SHIFT, selectedPlayer, FALSE);

			removeFromForce(0);									// remove from force (to free power)
			if (pDr)
			{
				addDroid(pDr, apsDroidLists);					// add it to the world.
				position.x = pDr->x;								// Add an effect
				position.z = pDr->y;
				position.y = pDr->z;
				addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_DISCOVERY,FALSE,NULL,FALSE);
			}
//		}
	}

}


// ////////////////////////////////////////////////////////////////////////////
// save out force to a file.
// file format is as follows.
//  number of templates
//  number of droids in force
//  templates
//  droids

BOOL saveForce(char *name,FORCE *pfForce)
{
	STRING			fileName[255]="";
	FILE			*pFileHandle;
	DROID_TEMPLATE	*pT;
	FORCE_MEMBER	*pCount,*pMember;
	UDWORD			count=0;

	strcpy(fileName,name);

	pFileHandle = fopen(fileName, "wb");								// open the file
	if (!pFileHandle)
	{
		DBERROR(("Couldn't open %s", fileName));
		return FALSE;
	}

	// save header for force file.

	count =0;
	for(pT = pfForce->pForceTemplates;pT;pT=pT->psNext) count++;			// count templates
	if (fwrite(&count, sizeof(UDWORD), 1, pFileHandle) != 1)
	{
		DBERROR(("Write failed for %s", fileName));
		return FALSE;
	}

	count=0;
	for(pCount=pfForce->pMembers;pCount;pCount=pCount->psNext) count++;	// count droids
	if (fwrite(&count, sizeof(UDWORD), 1, pFileHandle) != 1)
	{
		DBERROR(("Write failed for %s", fileName));
		return FALSE;
	}

#if 0
	// old method. save whole template
	for(pT = pfForce->pForceTemplates;pT;pT=pT->psNext)					// save templates
	{
		if (fwrite(pT, sizeof(DROID_TEMPLATE), 1, pFileHandle) != 1)	// template
		{
			DBERROR(("Write failed for %s", fileName));
			return FALSE;
		}
		fputc(10,pFileHandle);											//seperator.
	}

	// save force
	for(pMember = pfForce->pMembers;pMember;pMember =pMember->psNext)
	{
		if (fwrite(&(pMember->pTempl->ref) ,sizeof(pMember->pTempl->ref), 1, pFileHandle) != 1)
		{
			DBERROR(("Write failed for %s", fileName));					// force type
			return FALSE;
		}
	}
#else
	// new method. refs to templates. USED FOR MULITLANG SUPP.
	for(pMember = pfForce->pMembers;pMember;pMember =pMember->psNext)
	{
		if (fwrite( &(pMember->pTempl->multiPlayerID) ,sizeof(pMember->pTempl->multiPlayerID), 1, pFileHandle) != 1)
		{
			DBERROR(("Write failed for %s", fileName));					// force type
			return FALSE;
		}
	}
#endif

	if (fclose(pFileHandle) != 0)
	{
		DBERROR(("Close failed for %s", fileName));
		return FALSE;
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// load a force from a file.
BOOL loadForce(char *name)
{
	STRING			fileName[255]="";
//	STRING			tname[255]="";
	FILE			*pFileHandle;
	UDWORD			tcount,fcount=0,ref;
	DROID_TEMPLATE	*psTempl;

	strcpy(fileName,name);
  debug(LOG_WZ, "loadForce: %s", fileName);
	pFileHandle = fopen(fileName, "rb");									// check file exists
	if (pFileHandle == NULL)
	{
		return FALSE;														// failed
	}

	while(Force.pMembers)													// clear current force
	{
		removeFromForce(0);
	}

//	DBERROR(("tem %d   :   mem %d",Force.pForceTemplates ,	Force.pMembers));

	Force.pForceTemplates = NULL;
	Force.pMembers = NULL;

	// load in new force.
	if (fread(&tcount, sizeof(UDWORD), 1, pFileHandle) != 1)				// get number of templates
	{
		DBERROR(("Read failed for %s", fileName));
		fclose(pFileHandle);
		return FALSE;
	}

	if (fread(&fcount, sizeof(UDWORD), 1, pFileHandle) != 1)				// get number of droids in force
	{
		DBERROR(("read failed for %s", fileName));
		fclose(pFileHandle);
		return FALSE;
	}

#if 0
	// old method
	for(tcount;tcount!=0;tcount--)											// get templates
	{
		psTempl = MALLOC(sizeof(DROID_TEMPLATE));
		if (psTempl == NULL)	//!HEAP_ALLOC(psTemplateHeap, &psTempl))
		{
			DBERROR(("Couldn't allocate template for %s", fileName));
			return FALSE;
		}
		if (fread(psTempl, sizeof(DROID_TEMPLATE), 1, pFileHandle) != 1)	// read in a template.
		{
			DBERROR(("read failed for %s", fileName));
			fclose(pFileHandle);
			return FALSE;
		}
		psTempl->pName = (CHAR*)&psTempl->aName;
		fgetc(pFileHandle);													// remove the template separator in the force file.
		psTempl->psNext = Force.pForceTemplates;
		Force.pForceTemplates=psTempl;
	}
	for(fcount;fcount!=0;fcount--)											// get forces.
	{
		if (fread(&ref, sizeof(ref), 1, pFileHandle) != 1)					// read in a template ref code.
		{
			DBERROR(("read failed for %s", fileName));
			fclose(pFileHandle);
			return FALSE;
		}

		for( psTempl = Force.pForceTemplates;								// find relevant template
			 psTempl && (psTempl->ref != ref);
			 psTempl = psTempl->psNext);

		if(!psTempl)
		{
			DBERROR(("failed to load. invalid file."));
			fclose(pFileHandle);
			return FALSE;
		}

		addToForce(psTempl);												// add it to the force.
	}
#else
	// new method.
	// get forces.
	while (fcount > 0)
	{
		if (fread(&ref, sizeof(ref), 1, pFileHandle) != 1)					// read in a template ref code.
		{
			DBERROR(("read failed for %s", fileName));
			fclose(pFileHandle);
			return FALSE;
		}

		psTempl = IdToTemplate(ref,DEATHMATCHTEMPLATES);
		if(psTempl)
		{
			addToForce(psTempl);												// add it to the force.
		}
		fcount--;
	}
#endif

	if (fclose(pFileHandle) != 0)
	{
		DBERROR(("Close failed for %s", fileName));
		return FALSE;
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// STATS STUFF
// ////////////////////////////////////////////////////////////////////////////

// ////////////////////////////////////////////////////////////////////////////
// Get Player's stats
PLAYERSTATS getMultiStats(UDWORD player,BOOL bLocal)
{
	static PLAYERSTATS stat;
	DWORD		statSize = sizeof(PLAYERSTATS);
	DPID		playerDPID;

	playerDPID = player2dpid[player];

	if(bLocal)
	{
		NETgetLocalPlayerData(playerDPID,&stat,&statSize);
	}
	else
	{
		NETgetGlobalPlayerData(playerDPID,&stat,&statSize);
	}

	return stat;
}

// ////////////////////////////////////////////////////////////////////////////
// Set Player's stats
BOOL setMultiStats(DWORD dp, PLAYERSTATS plStats, BOOL bLocal)
{
	DPID	playerDPID = (DPID) dp;

	if(bLocal)
	{
		NETsetLocalPlayerData(playerDPID,&plStats,sizeof(PLAYERSTATS));
	}
	else
	{
		NETsetGlobalPlayerData(playerDPID,&plStats,sizeof(PLAYERSTATS));
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Load Player Stats
BOOL loadMultiStats(STRING *sPlayerName,PLAYERSTATS *playerStats)
{
	STRING				fileName[255]="";
	UDWORD				size;
	UBYTE				*pFileData;
	FILE				*pFileHandle;
	PLAYERSTATS			blankstats = {0};
	SAVEDPLAYERSTATS	st,*codedst;
	UDWORD				tmp[4];

	strcpy(fileName,MultiPlayersPath);
	strcat(fileName,sPlayerName);
	strcat(fileName,".sta");

  debug(LOG_WZ, "loadMultiStats: %s",fileName);
	// check player already exists
	// FIXME: integrate with physfs stuff, and add basic sanity
	pFileHandle = fopen(fileName, "rb");
	if (pFileHandle == NULL)
	{
		saveMultiStats(sPlayerName,sPlayerName,&blankstats);		// didnt exist so create.
	}
	else
	{
		fclose(pFileHandle);
	}

	loadFile(fileName,&pFileData,&size);
	codedst = ((SAVEDPLAYERSTATS*)pFileData);

	//decode packet;
	memcpy(&tmp,&NetPlay.cryptKey,sizeof(tmp));
	NETsetKey(11974,224351,2023901,21080);
	NETunmangleData((long int*)codedst,(long int*)&st,sizeof(SAVEDPLAYERSTATS));
	NETsetKey(tmp[0],tmp[1],tmp[2],tmp[3]);

	//set stats.
	memcpy(playerStats,&(st.stats), sizeof(PLAYERSTATS));	// get

	//set the name. ASSUME STRING IS LONG ENOUGH!
	strcpy(sPlayerName,st.name);

	FREE(pFileData);

	// reset recent scores
	playerStats->recentKills = 0;
	playerStats->recentScore = 0;
	playerStats->killsToAdd  = 0;
	playerStats->scoreToAdd  = 0;


	// clear any skirmish stats.
	for(size = 0;size<MAX_PLAYERS;size++)
	{
		ingame.skScores[size][0] =0;
		ingame.skScores[size][1] =0;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Save Player Stats
BOOL saveMultiStats(STRING *sFileName, STRING *sPlayerName,PLAYERSTATS *playerStats)
{
	STRING				fileName[255]="";
	SAVEDPLAYERSTATS	codedst,st;
	UDWORD				tmp[4];

	// prepare file.
	memcpy(&st.stats,playerStats,sizeof(PLAYERSTATS));
	memset(st.name,0,255);
	memset(st.padding,1,4);
	strcpy(st.name, sPlayerName);

	//encode packet;
	memcpy(&tmp,&NetPlay.cryptKey,sizeof(tmp));
	NETsetKey(11974,224351,2023901,21080);
	NETmangleData((long int*)&st,(long int*)&codedst,sizeof(SAVEDPLAYERSTATS));
	NETsetKey(tmp[0],tmp[1],tmp[2],tmp[3]);

	strcpy(fileName,MultiPlayersPath);
	strcat(fileName,sFileName);
	strcat(fileName,".sta");
	saveFile(fileName,(UBYTE*)&codedst,sizeof(SAVEDPLAYERSTATS));

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// score update functions

// update players damage stats.
VOID updateMultiStatsDamage	(UDWORD attacker, UDWORD defender, UDWORD inflicted)
{
	PLAYERSTATS st;

	if(isHumanPlayer(attacker))
	{
		st = getMultiStats(attacker,TRUE);	// get stats
		if(NetPlay.bComms)
		{
			st.scoreToAdd += (2*inflicted);
		}
		else
		{
			st.recentScore += (2*inflicted);
		}
		setMultiStats(player2dpid[attacker], st, TRUE);
	}
	else
	{
		ingame.skScores[attacker][0] += (2*inflicted);	// increment skirmish players rough score.
	}


	if(isHumanPlayer(defender))
	{
		st = getMultiStats(defender,TRUE);	// get stats
		if(NetPlay.bComms)
		{
			st.scoreToAdd  -= inflicted;
		}
		else
		{
			st.recentScore  -= inflicted;
		}
		setMultiStats(player2dpid[defender], st, TRUE);
	}
	else
	{
		ingame.skScores[defender][0] -= inflicted;	// increment skirmish players rough score.
	}
}

// update games played.
VOID updateMultiStatsGames(void)
{
	PLAYERSTATS	st;

	st  = getMultiStats(selectedPlayer,TRUE);
	st.played ++;
	setMultiStats(player2dpid[selectedPlayer], st, TRUE);
}

// games won
VOID updateMultiStatsWins(void)
{
	PLAYERSTATS	st;
	st  = getMultiStats(selectedPlayer,TRUE);
	st.wins ++;
	setMultiStats(player2dpid[selectedPlayer], st, TRUE);
}

//games lost.
VOID updateMultiStatsLoses(void)
{
	PLAYERSTATS	st;
	st  = getMultiStats(selectedPlayer,TRUE);
	st.loses ++;
	setMultiStats(player2dpid[selectedPlayer], st, TRUE);
}

// update kills
VOID updateMultiStatsKills(BASE_OBJECT *psKilled,UDWORD player)
{
	PLAYERSTATS	st;

	if(isHumanPlayer(player))
	{
		st  = getMultiStats(player,TRUE);

		if(NetPlay.bComms)
		{
			st.killsToAdd++;		// increase kill count;
		}
		else
		{
			st.recentKills++;
		}
		setMultiStats(player2dpid[player], st, TRUE);
	}
	else
	{
		ingame.skScores[player][1]++;
	}
}

