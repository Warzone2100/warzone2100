/*
 * Droid.c
 *
 * Droid method functions
 *
 */

#include <stdio.h>

/* Droid damage printf's */
//#define DEBUG_GROUP1
//#include <assert.h>
#include "lib/framework/frame.h"
#include "objects.h"
#include "loop.h"
#include "visibility.h"
#include "map.h"
#include "drive.h"
//#include "droid.h"
//#include "objmem.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"
#include "game.h"
#include "power.h"
#include "miscimd.h"
#include "effects.h"
#include "feature.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "action.h"
#include "order.h"
#include "lib/framework/fractions.h"
#include "move.h"
#include "lib/ivis_common/geo.h"
#include "anim_id.h"
#include "lib/gamelib/animobj.h"
#include "geometry.h"
#include "display.h"
#include "console.h"
#include "component.h"
#include "function.h"
#include "lighting.h"
#include "gateway.h"

#include "multiplay.h"		//ajl

#include "formationdef.h"
#include "formation.h"
#include "warcam.h"
#include "display3d.h"
#include "group.h"
#include "text.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "cmddroid.h"
#include "fpath.h"
#include "mapgrid.h"
#include "projectile.h"
#include "cluster.h"
#include "mission.h"
#include "levels.h"
#include "transporter.h"
#include "selection.h"
#include "difficulty.h"
#include "edit3d.h"
#include "scores.h"
#include "research.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"
#include "scriptfuncs.h"			//for ThreatInRange()


#define DEFAULT_RECOIL_TIME	(GAME_TICKS_PER_SEC/4)
#define	DROID_DAMAGE_SPREAD	(16 - rand()%32)
#define	DROID_REPAIR_SPREAD	(20 - rand()%40)


/* default droid design template */
extern DROID_TEMPLATE	sDefaultDesignTemplate;

//storage
DROID_TEMPLATE		*apsDroidTemplates[MAX_PLAYERS];

// store the experience of recently recycled droids
UWORD	aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];
UDWORD	selectedGroup = UBYTE_MAX;
UDWORD	selectedCommander = UBYTE_MAX;
/*Height the transporter hovers at above the terrain*/

#define TRANSPORTER_HOVER_HEIGHT	10


//how far round a repair droid looks for a damaged droid
#define REPAIR_DIST		(TILE_UNITS * 4)//8)

/* Store for the objects near the droid currently being updated
 * NAYBOR = neighbour - thanks to Keith for a great abreviation
 */
NAYBOR_INFO			asDroidNaybors[MAX_NAYBORS];
UDWORD				numNaybors=0;

// store the last time a structure was hit for a side
// this controls when the CALL_STRUCT_ATTACKED is made
//UDWORD	aLastDroidHit[MAX_PLAYERS];

// the structure that was last hit
DROID	*psLastDroidHit;

//determines the best IMD to draw for the droid - A TEMP MEASURE!
//static void setDroidIMD(DROID *psDroid);
void	groupConsoleInformOfSelection( UDWORD groupNumber );
void	groupConsoleInformOfCreation( UDWORD groupNumber );
void	groupConsoleInformOfCentering( UDWORD groupNumber );
void	setSelectedGroup(UDWORD groupNumber);
void assignDroidsToGroup(UDWORD	playerNumber, UDWORD groupNumber);
void	droidUpdateRecoil( DROID *psDroid );
BOOL activateGroup(UDWORD playerNumber, UDWORD groupNumber);
BOOL activateGroupAndMove(UDWORD playerNumber, UDWORD groupNumber);
void droidSetBits(DROID_TEMPLATE *pTemplate,DROID *psDroid);
BOOL	pickATile2(UDWORD *x, UDWORD *y, UDWORD numIterations);
UDWORD	getBound(UDWORD level);
STRING	*getDroidNameForRank(UDWORD rank);


/*time to move to a new location (when building foundation) */
//static void moveToNewTile(DROID *psDroid);

// initialise droid module
BOOL droidInit(void)
{
	memset(aDroidExperience, 0, sizeof(UWORD) * MAX_PLAYERS * MAX_RECYCLED_DROIDS);
//	memset(aLastDroidHit, 0, sizeof(UDWORD) * MAX_PLAYERS);
	psLastDroidHit = NULL;

	return TRUE;
}


/* Do damage to a droid.
 * Returns TRUE if the droid is destroyed
 */

#define UNIT_LOST_DELAY	(5*GAME_TICKS_PER_SEC)
BOOL droidDamage(DROID *psDroid, UDWORD damage, UDWORD weaponClass, UDWORD weaponSubClass)
{
	UDWORD		penDamage, armourDamage;
	BOOL		penetrated = FALSE;
	UDWORD		armour=0;
	SECONDARY_STATE		state;
	SDWORD		level, cmdLevel;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitDamage: Invalid Unit pointer" );

	DBP1(("unitDamage(%d): body %d armour %d damage: %d\n",
		psDroid->id, psDroid->body, psDroid->armour[WC_KINETIC], damage));

    //EMP cannons do not do body damage
    if (weaponSubClass == WSC_EMP)
    {
        //store the time
    	psDroid->timeLastHit = gameTime;
	    psDroid->lastHitWeapon = weaponSubClass;
        //quit early
        return FALSE;
    }



    //only overwrite if the last weapon to hit was not an EMP - need the time value for this
    if (psDroid->lastHitWeapon != WSC_EMP)
    {
    	psDroid->timeLastHit = gameTime;
	    psDroid->lastHitWeapon = weaponSubClass;
    }



//	if(selectedPlayer==0)
	if(psDroid->player != selectedPlayer)
	{
		// Player inflicting damage on enemy.
		damage = (UDWORD) modifyForDifficultyLevel( (SDWORD) damage,TRUE);
	} else {
		// Enemy inflicting damage on player.
		damage = (UDWORD) modifyForDifficultyLevel( (SDWORD) damage,FALSE);
	}

	// vtols on the ground take triple damage
	if (vtolDroid(psDroid) &&
		psDroid->sMove.Status == MOVEINACTIVE)
	{
		damage *= 3;
	}

	// reset the attack level
	if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state))
	{
		if (state == DSS_ALEV_ATTACKED)
		{
			secondarySetState(psDroid, DSO_ATTACK_LEVEL, DSS_ALEV_ALWAYS);
		}
	}

	switch (weaponClass)
	{
		case WC_KINETIC:
		//case WC_EXPLOSIVE:
			if (damage > psDroid->armour[WC_KINETIC])
			{
				penetrated = TRUE;
			}
			armour = psDroid->armour[WC_KINETIC];
			break;
		case WC_HEAT:
		//case WC_MISC:
			if (damage > psDroid->armour[WC_HEAT])
			{
				penetrated = TRUE;
			}
			armour = psDroid->armour[WC_HEAT];
			break;
	}

	clustObjectAttacked((BASE_OBJECT *)psDroid);

//	if (damage > psDroid->armour[WC_KINETIC])
	if (penetrated)
	{
		/* Damage has penetrated - reduce armour and body points */
		//penDamage = damage - psDroid->armour;
		penDamage = damage - armour;

		level = getDroidLevel(psDroid);
		cmdLevel = cmdGetCommanderLevel(psDroid);
		if (level > cmdLevel)
		{
			//penDamage = (penDamage * (100 - 5 * level)) / 100;
			penDamage = (penDamage * (100 - 6 * level)) / 100;
		}
		else
		{
			//penDamage = (penDamage * (100 - 5 * cmdLevel)) / 100;
			penDamage = (penDamage * (100 - 6 * cmdLevel)) / 100;
		}

		DBP1(("        penetrated: %d\n", penDamage));
		if (penDamage >= psDroid->body)
		{
            //we don't want this in multiPlayer
            if (!bMultiPlayer)
            {
                //hack to prevent Transporter's being blown up
                if (psDroid->droidType == DROID_TRANSPORTER)
                {
                    psDroid->body = 1;
                    return FALSE;
                }
            }
			/* Droid destroyed */
			DBP1(("        DESTROYED\n"));
			if(psDroid->player == selectedPlayer)
			{
				CONPRINTF(ConsoleString,(ConsoleString, strresGetString(psStringRes,STR_GAM_UNITLOST)));
				scoreUpdateVar(WD_UNITS_LOST);
				audio_QueueTrackMinDelayPos(ID_SOUND_UNIT_DESTROYED,UNIT_LOST_DELAY,
											psDroid->x, psDroid->y, psDroid->z );
	//				"INTEL REPORT : Unit Lost!"));
			}
			else
			{
				scoreUpdateVar(WD_UNITS_KILLED);
			}
			if(psDroid->droidType == DROID_PERSON AND weaponClass == WC_HEAT)
			{
				droidBurn(psDroid);
			}
			else
			{
	  			destroyDroid(psDroid);

			}
			return TRUE;
		}
		else
		{
			psDroid->body -= penDamage;
		}

		/* Do damage to armour */
//		armourDamage = (damage / PEN_ARMOUR_DAMAGE_FACTOR) + 1;

//		DBP1(("penetrated: %d, armour: %d\n", penDamage, armourDamage));
	}
	else
	{
		/* Damage didn't penetrate - only reduce armour */
		armourDamage = (damage / ARMOUR_DAMAGE_FACTOR) + 1;

		/* Do one point of damage to body */
		DBP1(("        not penetrated - 1 point damage\n"));
		if(psDroid->droidType == DROID_PERSON AND weaponClass == WC_HEAT)
		{
			droidBurn(psDroid);
		}
		if (psDroid->body == 1)
		{
            //we don't want this in multiPlayer
            if (!bMultiPlayer)
            {
                //hack to prevent Transporter's being blown up
                if (psDroid->droidType == DROID_TRANSPORTER)
                {
                    return FALSE;
                }
            }

            if(psDroid->player == selectedPlayer)
			{
				CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_UNITLOST)));
				scoreUpdateVar(WD_UNITS_LOST);
				audio_QueueTrackMinDelayPos( ID_SOUND_UNIT_DESTROYED,UNIT_LOST_DELAY,
											psDroid->x, psDroid->y, psDroid->z );
			}
			else
			{
				scoreUpdateVar(WD_UNITS_KILLED);
			}

  			destroyDroid(psDroid);

			DBP1(("        DESTROYED\n"));
			return TRUE;
		}
		else
		{
			psDroid->body -= 1;
		}

//		DBP1(("armour: %d\n", armourDamage));
	}

	/* Actually reduce the droids armour */
/*	if (armourDamage >= psDroid->armour)
	{
		psDroid->armour = 0;
	}
	else
	{
		psDroid->armour -= armourDamage;
	}*/

	DBP1(("        body left: %d armour left: %d\n",
		psDroid->body, psDroid->armour));

	/* now check for auto return on droid's secondary orders */
	secondaryCheckDamageLevel(psDroid);
    /* now check for scripted run-away based on health */
    orderHealthCheck(psDroid);


    //only overwrite if the last weapon to hit was not an EMP - need the time value for this
    if (psDroid->lastHitWeapon != WSC_EMP)
    {
    	psDroid->timeLastHit = gameTime;
	    psDroid->lastHitWeapon = weaponSubClass;
    }

	return FALSE;

}




void droidDeleteName(DROID *psDroid)
{
}

/* droidRelease: release all resources associated with a droid -
 * should only be called by objmem - use vanishDroid preferably
 */
void droidRelease(DROID *psDroid)
{
	DROID	*psCurr, *psNext;

	droidDeleteName(psDroid);

	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		if (psDroid->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDroid->psGroup->psList; psCurr != NULL AND psCurr !=
				psDroid; psCurr = psNext)
			{
				psNext = psCurr->psGrpNext;
				droidRelease(psCurr);
				HEAP_FREE(psDroidHeap, psCurr);
			}
		}
	}
	//FREE(psDroid->pName);

	// leave the current formation if any
	if (psDroid->sMove.psFormation)
	{
		formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
	}

	// leave the current group if any
	if (psDroid->psGroup)
	{
		grpLeave(psDroid->psGroup, psDroid);
	}

	// remove the object from the grid
	gridRemoveObject((BASE_OBJECT *)psDroid);

	// remove the droid from the cluster systerm
	clustRemoveObject((BASE_OBJECT *)psDroid);
}


// recycle a droid (retain it's experience and some of it's cost)
void recycleDroid(DROID *psDroid)
{
	UDWORD		numKills, minKills;
	SDWORD		i, cost, storeIndex;
	iVector		position;

	// store the droids kills
	numKills = psDroid->numKills;
	if (numKills > UWORD_MAX)
	{
		numKills = UWORD_MAX;
	}
	minKills = UWORD_MAX;
	storeIndex = 0;
	for(i=0; i<MAX_RECYCLED_DROIDS; i++)
	{
		if (aDroidExperience[psDroid->player][i] < (UWORD)minKills)
		{
			storeIndex = i;
			minKills = aDroidExperience[psDroid->player][i];
		}
	}
	aDroidExperience[psDroid->player][storeIndex] = (UWORD)numKills;

	// return part of the cost of the droid
	cost = calcDroidPower(psDroid);
	cost = (cost/2) * psDroid->body / psDroid->originalBody;
	addPower(psDroid->player, (UDWORD)cost);

	// hide the droid
	memset(psDroid->visible, 0, sizeof(psDroid->visible));
	// stop any group moral checks
	if (psDroid->psGroup)
	{
		grpLeave(psDroid->psGroup, psDroid);
	}

	position.x = psDroid->x;				// Add an effect
	position.z = psDroid->y;
	position.y = psDroid->z;

//	destroyDroid(psDroid);
	vanishDroid(psDroid);

	addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_DISCOVERY,FALSE,NULL,FALSE);

}


void	removeDroidBase(DROID *psDel)
{
	DROID	*psCurr, *psNext;
	BOOL	bRet;
	DROID_GROUP	*psGroup;
	STRUCTURE	*psStruct;


	if(!driveDroidKilled(psDel)) {	// Tell the driver system it's gone.

	}

	//tell the power system its gone
	powerDestroyObject((BASE_OBJECT *)psDel);

	if (psDel->died AND psDel->died != NOT_CURRENT_LIST)
	{
		// droid has already been killed, quit
		return;
	}


	//ajl, inform others of destruction.
	if(bMultiPlayer)
	{
		if( !((psDel->player != selectedPlayer) && (psDel->order == DORDER_RECYCLE)))
		{
			SendDestroyDroid(psDel);
		}
	}


	/* remove animation if present */
	if ( psDel->psCurAnim != NULL )
	{
		bRet = animObj_Remove( &psDel->psCurAnim, psDel->psCurAnim->psAnim->uwID );
		ASSERT( bRet == TRUE, "destroyUnit: animObj_Remove failed" );
		psDel->psCurAnim = NULL;
	}

	//put back the power required to maintain this droid (=power to build)
	//returnPower(psDel->player, psDel->power);

	// leave the current formation if any
	if (psDel->sMove.psFormation)
	{
		formationLeave(psDel->sMove.psFormation, (BASE_OBJECT *)psDel);
		psDel->sMove.psFormation = NULL;
	}

	//kill all the droids inside the transporter
	if (psDel->droidType == DROID_TRANSPORTER)
	{
		if (psDel->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDel->psGroup->psList; psCurr != NULL AND psCurr !=
				psDel; psCurr = psNext)
			{
				psNext = psCurr->psGrpNext;

				/* add droid to droid list then vanish it - hope this works! - GJ */
				addDroid(psCurr, apsDroidLists);
				vanishDroid(psCurr);
			}
		}
	}

	// check moral
	if (psDel->psGroup && psDel->psGroup->refCount > 1)
	{
		psGroup = psDel->psGroup;
		grpLeave(psDel->psGroup, psDel);
		orderGroupMoralCheck(psGroup);
	}
	//else if (psDel->player == BARB1 || psDel->player == BARB2)
    else
	{
		orderMoralCheck(psDel->player);
	}

	// leave the current group if any
	if (psDel->psGroup)
	{
		grpLeave(psDel->psGroup, psDel);
		psDel->psGroup = NULL;
	}

    //PUT THIS IN removeDroidFX()
	//once a droid is destroyed - it leaves a wrecked droid FEATURE in its place
//	buildFeature((asFeatureStats + droidFeature), psDel->x, psDel->y);
	//if( (psDel->droidType == DROID_PERSON || psDel->droidType == DROID_CYBORG) &&
	//	(psDel->order != DORDER_RUNBURN) )
	//{
	//	/* blow person up into blood and guts */
	//	compPersonToBits(psDel);
	//}

	/* Put Deliv. Pts back into world when a command droid dies */
	if(psDel->droidType == DROID_COMMAND)
	{

		for (psStruct = apsStructLists[psDel->player]; psStruct; psStruct=psStruct->psNext)
		{
/* Replace the delivery points for the factories assigned to this command droid */
//			if ( psStruct->pStructureType->type == REF_FACTORY )	/* Is it a factory? */
//			{
//				assignFactoryCommandDroid(psStruct, NULL);	/* Return d. pt. */
//			}

			// alexl's stab at a right answer.
			if ( StructIsFactory(psStruct) && ((FACTORY *)psStruct->pFunctionality)->psCommander == psDel)
			{
				assignFactoryCommandDroid(psStruct, NULL);
			}


		}
	}

    //check to see if constructor droid currently trying to find a location to build
    //if (psDel->droidType == DROID_CONSTRUCT AND psDel->player ==
    if ((psDel->droidType == DROID_CONSTRUCT OR psDel->droidType ==
        DROID_CYBORG_CONSTRUCT) AND psDel->player ==
        selectedPlayer AND psDel->selected)
    {
        //if currently trying to build, kill off the placement
        if (tryingToGetLocation())
        {
            kill3DBuilding();
        }
    }

	// kill the command droid if any
/*	- no seperate COMMAND_DROID structure now
	if (psDel->asBits[COMP_BRAIN].nStat != 0)
	{
		destroyCommandDroid((SDWORD)psDel->player,
							(SDWORD)psDel->asBits[COMP_BRAIN].nStat);
	}*/

	// remove the droid from the grid
	gridRemoveObject((BASE_OBJECT *)psDel);

	// remove the droid from the cluster systerm
	clustRemoveObject((BASE_OBJECT *)psDel);

	if(psDel->player == selectedPlayer)
	{
		intRefreshScreen();
	}

	killDroid(psDel);
}


/* Put this back in if everything starts blowing up at once!!!
// -------------------------------------------------------------------------------
UDWORD	lastDroidRemove=0;
UDWORD	droidRemoveKills=0;
// -------------------------------------------------------------------------------
*/
void	removeDroidFX(DROID *psDel)
{
 	iVector	pos;

	// only display anything if the droid is visible
	if (!psDel->visible[selectedPlayer])
	{
		return;
	}

	/*
	// -------------------------------------------------------------------------------
	if(psDel->player == selectedPlayer)
	{
		if(gameTime - lastDroidRemove < 100)	//assume 10 fames a sec min
		{
			droidRemoveKills++;
			if(droidRemoveKills>=2)
			{
				ASSERT( FALSE,"3 of your droids killed in less than a tenth of a second?" );
			}
		}
		else
		{
			droidRemoveKills = 0;
		}
		lastDroidRemove = gameTime;
	}
	// -------------------------------------------------------------------------------

  */

    //if( (psDel->droidType == DROID_PERSON || psDel->droidType == DROID_CYBORG) &&
    if( (psDel->droidType == DROID_PERSON || cyborgDroid(psDel)) &&
		(psDel->order != DORDER_RUNBURN) )
	{
		/* blow person up into blood and guts */
		compPersonToBits(psDel);
	}

	/* if baba and not running (on fire) then squish */
	if( psDel->droidType == DROID_PERSON )
	{
		if ( psDel->order != DORDER_RUNBURN )
		{
			if(psDel->visible[selectedPlayer])
			{
// The babarian has been run over ...

				audio_PlayStaticTrack( psDel->x, psDel->y, ID_SOUND_BARB_SQUISH );


			}
		}
	}
	else if(psDel->visible[selectedPlayer])
	{
		destroyFXDroid(psDel);
		// this sounds a bit crap on the psx
		pos.x = psDel->x;
		pos.z = psDel->y;
		pos.y = psDel->z;
		addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_DROID,FALSE,NULL,0);
		audio_PlayStaticTrack( psDel->x, psDel->y, ID_SOUND_EXPLOSION );

#if defined(PSX) && defined(LIBPAD)
		if(EnableVibration) {
	debug( LOG_NEVER, "SetVibro1\n" );
			SetVibro1(0,100,512);
		}
#endif
	}

}

void destroyDroid(DROID *psDel)
{

	if(psDel->lastHitWeapon==WSC_LAS_SAT)		// darken tile if lassat.
	{
		UDWORD width,breadth;
		MAPTILE	*psTile;

		mapX = (psDel->x ) >> TILE_SHIFT;
		mapY = (psDel->y ) >> TILE_SHIFT;
		for (width = mapX-1; width <= mapX+1; width++)
		{
			for (breadth = mapY-1; breadth <= mapY+1; breadth++)
			{
				psTile = mapTile(width,breadth);
				if(TEST_TILE_VISIBLE(selectedPlayer,psTile))
				{
					psTile->illumination /= 2;
					if(psTile->bMaxed AND psTile->level!=UBYTE_MAX) //only do one's already seen
					{
						psTile->level/=2;
					}

				}
			}
		}
	}

	removeDroidFX(psDel);
	removeDroidBase(psDel);
	return;
}

void	vanishDroid(DROID *psDel)
{
	removeDroidBase(psDel);
}

#if 0
/* Remove a droid and free it's memory */
void destroyDroid(DROID *psDel)
{
	iVector	pos;
	UDWORD	widthScatter,breadthScatter,heightScatter;
	UDWORD	i;
	DROID	*psCurr, *psNext;
	BOOL	bRet;
	DROID_GROUP	*psGroup;
	STRUCTURE	*psStruct;


	if(!driveDroidKilled(psDel)) {	// Tell the driver system it's gone.

	}

	if (psDel->died)
	{
		// droid has already been killed, quit
		return;
	}


	//ajl, inform others of destruction.
	if(bMultiPlayer)
	{
		SendDestroyDroid(psDel);
	}


	/* if baba and not running (on fire) then squish */
	if( psDel->droidType == DROID_PERSON )
	{
		if ( psDel->order != DORDER_RUNBURN )
		{
			if(psDel->visible[selectedPlayer])
			{
// The babarian has been run over ...

				audio_PlayStaticTrack( psDel->x, psDel->y, ID_SOUND_BARB_SQUISH );


			}
		}
	}
	else if(psDel->visible[selectedPlayer])
	{
		/* First, add some gravitons and explosions */
		widthScatter = TILE_UNITS/4;
		breadthScatter = TILE_UNITS/4;
		heightScatter = TILE_UNITS/5;
		for(i=0; i<4; i++)
		{
			pos.x = psDel->x + widthScatter - rand()%(2*widthScatter);
			pos.z = psDel->y + breadthScatter - rand()%(2*breadthScatter);
			pos.y = psDel->z + 16 +heightScatter;

			if(ONEINFOUR)
			{
				pos.y = psDel->z + 16 +heightScatter;
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
			}
			else

			{
				pos.y = psDel->z;
//				addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,debrisImds[rand()%MAX_DEBRIS],0);
				addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,getRandomDebrisImd(),0);
			}
		}


		// this sounds a bit crap on the psx
		/* And then a destruction sequence...*/
		pos.x = psDel->x;
		pos.z = psDel->y;
		pos.y = psDel->z;
		addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_DROID,FALSE,NULL,0);
		audio_PlayStaticTrack( psDel->x, psDel->y, ID_SOUND_EXPLOSION );

#if defined(PSX) && defined(LIBPAD)
		if(EnableVibration) {
			debug( LOG_NEVER, "SetVibro1\n" );
			SetVibro1(0,100,512);
		}
#endif
	}

	/* remove animation if present */
	if ( psDel->psCurAnim != NULL )
	{
		bRet = animObj_Remove( &psDel->psCurAnim, psDel->psCurAnim->psAnim->uwID );
		ASSERT( bRet == TRUE, "destroyUnit: animObj_Remove failed" );
		psDel->psCurAnim = NULL;
	}

	//put back the power required to maintain this droid (=power to build)
	//returnPower(psDel->player, psDel->power);

	// leave the current formation if any
	if (psDel->sMove.psFormation)
	{
		formationLeave(psDel->sMove.psFormation, (BASE_OBJECT *)psDel);
		psDel->sMove.psFormation = NULL;
	}

	//kill all the droids inside the transporter
	if (psDel->droidType == DROID_TRANSPORTER)
	{
		if (psDel->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDel->psGroup->psList; psCurr != NULL AND psCurr !=
				psDel; psCurr = psNext)
			{
				psNext = psCurr->psGrpNext;
				droidRelease(psCurr);
				HEAP_FREE(psDroidHeap, psCurr);
			}
		}
	}

	// check moral
	if (psDel->psGroup && psDel->psGroup->refCount > 1)
	{
		psGroup = psDel->psGroup;
		grpLeave(psDel->psGroup, psDel);
		orderGroupMoralCheck(psGroup);
	}
	//else if (psDel->player == BARB1 || psDel->player == BARB2)
    else
	{
		orderMoralCheck(psDel->player);
	}

	// leave the current group if any
	if (psDel->psGroup)
	{
		grpLeave(psDel->psGroup, psDel);
		psDel->psGroup = NULL;
	}

	//once a droid is destroyed - it leaves a wrecked droid FEATURE in its place
//	buildFeature((asFeatureStats + droidFeature), psDel->x, psDel->y);
	//if( (psDel->droidType == DROID_PERSON || psDel->droidType == DROID_CYBORG) &&
    if( (psDel->droidType == DROID_PERSON || cyborgDroid(psDel) &&
		(psDel->order != DORDER_RUNBURN) )
	{
		/* blow person up into blood and guts */
		compPersonToBits(psDel);
	}

	/* Put Deliv. Pts back into world when a command droid dies */
	if(psDel->droidType == DROID_COMMAND)
	{

		for (psStruct = apsStructLists[psDel->player]; psStruct; psStruct=psStruct->psNext)
		{
/* Replace the delivery points for the factories assigned to this command droid */
//			if ( psStruct->pStructureType->type == REF_FACTORY )	/* Is it a factory? */
//			{
//				assignFactoryCommandDroid(psStruct, NULL);	/* Return d. pt. */
//			}

			// alexl's stab at a right answer.
			if ( StructIsFactory(psStruct) && ((FACTORY *)psStruct->pFunctionality)->psCommander == psDel)
			{
				assignFactoryCommandDroid(psStruct, NULL);
			}


		}
	}

	// kill the command droid if any
/*	- no seperate COMMAND_DROID structure now
	if (psDel->asBits[COMP_BRAIN].nStat != 0)
	{
		destroyCommandDroid((SDWORD)psDel->player,
							(SDWORD)psDel->asBits[COMP_BRAIN].nStat);
	}*/

	// remove the droid from the grid
	gridRemoveObject((BASE_OBJECT *)psDel);

	// remove the droid from the cluster systerm
	clustRemoveObject((BASE_OBJECT *)psDel);


	if(psDel->player == selectedPlayer)
	{
		intRefreshScreen();
	}

	killDroid(psDel);

}

#endif
/* Remove a droid from the List so doesn't update or get drawn etc
TAKE CARE with removeDroid() - usually want droidRemove since it deal with cluster and grid code*/
//returns FALSE if the droid wasn't removed - because it died!
BOOL droidRemove(DROID *psDroid, DROID *pList[MAX_PLAYERS])
{
//	BOOL	bRet;

	if(!driveDroidKilled(psDroid)) {	// Tell the driver system it's gone.

	}

	//tell the power system its gone
	powerDestroyObject((BASE_OBJECT *)psDroid);

	if (psDroid->died AND psDroid->died != NOT_CURRENT_LIST)
	{
		// droid has already been killed, quit
		return FALSE;
	}

	/* remove animation if present  - do this when the droid is destroyed*/
	/*if ( psDroid->psCurAnim != NULL )
	{
		bRet = animObj_Remove( psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
		ASSERT( bRet == TRUE, "droidRemove: animObj_Remove failed" );
		psDroid->psCurAnim = NULL;
	}*/

	// leave the current formation if any
	if (psDroid->sMove.psFormation)
	{
		formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
		psDroid->sMove.psFormation = NULL;
	}

	// leave the current group if any - not if its a Transporter droid
	if (psDroid->droidType != DROID_TRANSPORTER AND psDroid->psGroup)
	{
		grpLeave(psDroid->psGroup, psDroid);
		psDroid->psGroup = NULL;
	}

    //reset the baseStruct
    psDroid->psBaseStruct = NULL;

	// remove the droid from the cluster systerm
	clustRemoveObject((BASE_OBJECT *)psDroid);

	// remove the droid from the grid
	gridRemoveObject((BASE_OBJECT *)psDroid);

	droidDeleteName(psDroid);

	removeDroid(psDroid, pList);

    //tell the power system its gone
	powerDestroyObject((BASE_OBJECT *)psDroid);

	if (psDroid->player == selectedPlayer)
	{
		intRefreshScreen();
	}

    return TRUE;
}

void droidFlameFallCallback( ANIM_OBJECT * psObj )
{
	DROID	*psDroid;

	ASSERT( PTRVALID(psObj, sizeof(ANIM_OBJECT)),
		"unitFlameFallCallback: invalid anim object pointer\n" );
	psDroid = (DROID *) psObj->psParent;
	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitFlameFallCallback: invalid Unit pointer\n" );

	psDroid->psCurAnim = NULL;

	destroyDroid( psDroid );
}

void droidBurntCallback( ANIM_OBJECT * psObj )
{
	DROID	*psDroid;

	ASSERT( PTRVALID(psObj, sizeof(ANIM_OBJECT)),
		"unitBurntCallback: invalid anim object pointer\n" );
	psDroid = (DROID *) psObj->psParent;
	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitBurntCallback: invalid Unit pointer\n" );

	/* add falling anim */
	psDroid->psCurAnim = animObj_Add( (BASE_OBJECT *) psDroid,
											ID_ANIM_DROIDFLAMEFALL, 0, 1 );
	if ( psDroid->psCurAnim == NULL )
	{
		debug( LOG_ERROR, "unitBurntCallback: couldn't add fall over anim\n" );
		abort();
		return;
	}

	animObj_SetDoneFunc( psDroid->psCurAnim, droidFlameFallCallback );
}

void droidBurn( DROID * psDroid )
{
	BOOL	bRet;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitBurn: invalid Unit pointer\n" );

	if ( psDroid->droidType != DROID_PERSON )
	{
		debug( LOG_ERROR, "unitBurn: can't burn anything except babarians currently!\n" );
		abort();
		return;
	}

	/* if already burning return else remove currently-attached anim if present */
	if ( psDroid->psCurAnim != NULL )
	{
		/* return if already burning */
		if ( psDroid->psCurAnim->psAnim->uwID == ID_ANIM_DROIDBURN )
		{
			return;
		}
		else
		{
			bRet = animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
			ASSERT( bRet == TRUE, "unitBurn: animObj_Remove failed" );
			psDroid->psCurAnim = NULL;
		}
	}

	/* add burning anim */
	psDroid->psCurAnim = animObj_Add( (BASE_OBJECT *) psDroid,
											ID_ANIM_DROIDBURN, 0, 3 );
	if ( psDroid->psCurAnim == NULL )
	{
		debug( LOG_ERROR, "unitBurn: couldn't add burn anim\n" );
		abort();
		return;
	}

	/* set callback */
	animObj_SetDoneFunc( psDroid->psCurAnim, droidBurntCallback );

	/* add scream */
	debug( LOG_NEVER, "baba burn\n" );

	audio_PlayObjDynamicTrack( psDroid, ID_SOUND_BARB_SCREAM+(rand()%3), NULL );


	/* set droid running */
	orderDroid( psDroid, DORDER_RUNBURN );
}

/* Add a new object to the naybor list */
static void addNaybor(BASE_OBJECT *psObj, UDWORD distSqr)
{
	UDWORD	pos;

	if (numNaybors >= MAX_NAYBORS)
	{
//		DBPRINTF(("Naybor list maxed out for id %d\n", psObj->id));
		return;
	}
	else if (numNaybors == 0)
	{
		// No objects in the list
		asDroidNaybors[0].psObj = psObj;
		asDroidNaybors[0].distSqr = distSqr;
		numNaybors++;
	}
	else if (distSqr >= asDroidNaybors[numNaybors-1].distSqr)
	{
		// Simple case - this is the most distant object
		asDroidNaybors[numNaybors].psObj = psObj;
		asDroidNaybors[numNaybors].distSqr = distSqr;
		numNaybors++;
	}
	else
	{
		// Move all the objects further away up the list
		pos = numNaybors;
		while (pos > 0 && asDroidNaybors[pos - 1].distSqr > distSqr)
		{
			memcpy(asDroidNaybors + pos, asDroidNaybors + (pos - 1), sizeof(NAYBOR_INFO));
			pos --;
		}

		// Insert the object at the correct position
		asDroidNaybors[pos].psObj = psObj;
		asDroidNaybors[pos].distSqr = distSqr;
		numNaybors++;
	}

	ASSERT( numNaybors <= MAX_NAYBORS,
		"addNaybor: numNaybors > MAX_NAYBORS" );
}


static DROID	*CurrentNaybors = NULL;
static UDWORD	nayborTime = 0;

void droidResetNaybors(void)
{
	CurrentNaybors = NULL;
}


//#ifndef PSX
//
//void droidGetNaybors(DROID *psDroid)
//{
//	droidGetNayb(psDroid);
//}
//
//#else
//
//void droidGetNaybors(DROID *psDroid)
//{
//	static DROID *psTmpDroid;
//	psTmpDroid = psDroid;
//
//	SetSpDCache();
//	droidGetNayb(psTmpDroid);
//	SetSpNormal();
//}
//
//#endif


// macro to see if an object is in NAYBOR_RANGE
// used by droidGetNayb
#define IN_NAYBOR_RANGE(psObj) \
	xdiff = dx - (SDWORD)psObj->x; \
	if (xdiff < 0) \
	{ \
		xdiff = -xdiff; \
	} \
	if (xdiff > NAYBOR_RANGE) \
	{ \
		continue; \
	} \
\
	ydiff = dy - (SDWORD)psObj->y; \
	if (ydiff < 0) \
	{ \
		ydiff = -ydiff; \
	} \
	if (ydiff > NAYBOR_RANGE) \
	{ \
		continue; \
	} \
\
	distSqr = xdiff*xdiff + ydiff*ydiff; \
	if (distSqr > NAYBOR_RANGE*NAYBOR_RANGE) \
	{ \
		continue; \
	} \


/* Find all the objects close to the droid */
void droidGetNaybors(DROID *psDroid)
{
//	DROID		*psCurrD;
//	STRUCTURE	*psCurrS;
//	FEATURE		*psCurrF;
	SDWORD		xdiff, ydiff;
//	UDWORD		player;
	UDWORD		dx,dy, distSqr;
	BASE_OBJECT	*psObj;

// Ensure only called max of once per droid per game cycle.
	if(CurrentNaybors == psDroid && nayborTime == gameTime) {
		return;
	}
	CurrentNaybors = psDroid;
	nayborTime = gameTime;



	// reset the naybor array
	numNaybors = 0;
#ifdef DEBUG
	memset(asDroidNaybors, 0xcd, sizeof(asDroidNaybors));
#endif

	// search for naybor objects
	dx = psDroid->x;
	dy = psDroid->y;
/*	for(player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurrD = apsDroidLists[player]; psCurrD; psCurrD = psCurrD->psNext)
		{
			if (psCurrD != psDroid)
			{
				IN_NAYBOR_RANGE(psCurrD);

				addNaybor((BASE_OBJECT *)psCurrD, distSqr);
			}
		}
		for (psCurrS = apsStructLists[player]; psCurrS; psCurrS = psCurrS->psNext)
		{
			IN_NAYBOR_RANGE(psCurrS);

			addNaybor((BASE_OBJECT *)psCurrS, distSqr);
		}
	}
	for (psCurrF = apsFeatureLists[0]; psCurrF; psCurrF = psCurrF->psNext)
	{
		IN_NAYBOR_RANGE(psCurrF);

		addNaybor((BASE_OBJECT *)psCurrF, distSqr);
	}*/

	gridStartIterate((SDWORD)dx, (SDWORD)dy);
	for (psObj = gridIterate(); psObj != NULL; psObj = gridIterate())
	{
		if (psObj != (BASE_OBJECT *)psDroid)
		{
			IN_NAYBOR_RANGE(psObj);

			addNaybor(psObj, distSqr);
		}
	}


}


/* Display the neigbours of a droid */
void displayNaybors(void)
{
	UDWORD	count, y;
	STRING	*pType = NULL;

	y = 100;
	for(count=0; count<numNaybors; count++)
	{
		switch(asDroidNaybors[count].psObj->type)
		{
		case OBJ_DROID:
			pType = "UNIT  ";
			break;
		case OBJ_STRUCTURE:
			pType = "STRUCT ";
			break;
		case OBJ_FEATURE:
			pType = "FEATURE";
			break;
		default:
			break;
		}

		screenTextOut(450, y, "%s %d", pType,
			asDroidNaybors[count].psObj->id);

		y+= 15;
	}
}

//static DROID *psTmpDroid;



/* The main update routine for all droids */
void droidUpdate(DROID *psDroid)
{
	iVector	dv;
	UDWORD	percentDamage, emissionInterval;
	BASE_OBJECT	*psBeingTargetted = NULL;
	SDWORD	damageToDo;


	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitUpdate: Invalid unit pointer" );

//	ASSERT( psDroid->x != 0 && psDroid->y != 0,
//		"unitUpdate: Unit at (0,0)" );

// Slap the stack in the DCache.

	// Find all the objects close to the droid
//	droidGetNaybors(psTmpDroid);	// Now done when needed.


	/* Clear down every droid as attacker could get killed */
//	psDroid->bTargetted = FALSE;

	// update the cluster of the droid
	if (psDroid->id % 20 == frameGetFrameNumber() % 20)
	{
		clustUpdateObject((BASE_OBJECT *)psDroid);
	}

    //may need power
    if (droidUsesPower(psDroid))
    {
//	    if ((asPower[psDroid->player]->currentPower > POWER_PER_CYCLE) OR
//		    (!powerCalculated))
		if (checkPower(psDroid->player, POWER_PER_CYCLE, FALSE))
	    {
		    //check if this droid is due some power
		    if (getLastPowered((BASE_OBJECT *)psDroid))
		    {
			    //get some power if necessary
			    if (accruePower((BASE_OBJECT *)psDroid))
			    {
				    updateLastPowered((BASE_OBJECT *)psDroid, psDroid->player);
			    }
		    }
	    }
    }

	// ai update droid
//#ifndef PSX
	aiUpdateDroid(psDroid);
//#else
//	PROFILE_START(4);
//	psTmpDroid = psDroid;
//	SetSpDCache();
//	aiUpdateDroid(psTmpDroid);
//	SetSpNormal();
//	PROFILE_END(4);
//#endif

	// update the droids order
	orderUpdateDroid(psDroid);

	// update the action of the droid
	actionUpdateDroid(psDroid);

//#ifndef PSX
	// update the move system
	moveUpdateDroid(psDroid);
//#else
//	PROFILE_START(5);
//	psTmpDroid = psDroid;
//	SetSpDCache();
//	moveUpdateDroid(psTmpDroid);
//	SetSpNormal();
//	PROFILE_END(5);
//#endif

	/* Only add smoke if they're visible */
	if((psDroid->visible[selectedPlayer]) AND psDroid->droidType != DROID_PERSON)
	{
		//percentDamage= PERCENT(psDroid->body,(asBodyStats + psDroid->asBits[COMP_BODY].nStat)->bodyPoints );
		percentDamage= (100-PERCENT(psDroid->body,psDroid->originalBody));
		// Is there any damage?
		if(percentDamage>=25)
		{
			if(percentDamage>=100)
			{
				percentDamage = 99;
			}
			//psDroid->emissionInterval = CALC_DROID_SMOKE_INTERVAL(percentDamage);
			emissionInterval = CALC_DROID_SMOKE_INTERVAL(percentDamage);
			//if(gameTime > (psDroid->lastEmission+psDroid->emissionInterval))
			if(gameTime > (psDroid->lastEmission + emissionInterval))
			{
   				dv.x = psDroid->x + DROID_DAMAGE_SPREAD;
   				dv.z = psDroid->y + DROID_DAMAGE_SPREAD;
				dv.y = psDroid->z;

				dv.y += (psDroid->sDisplay.imd->ymax*2);
				addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING_SMALL,FALSE,NULL,0);
		// FIXFX		addExplosion(&dv,TYPE_EXPLOSION_SMOKE_CLOUD,NULL);
				psDroid->lastEmission = gameTime;
			}
		}
	}

	processVisibility((BASE_OBJECT*)psDroid);

	// -----------------
	/* Are we a sensor droid or a command droid? */
	if( (psDroid->droidType == DROID_SENSOR) OR (psDroid->droidType == DROID_COMMAND) )
	{
		/* Nothing yet... */
		psBeingTargetted = NULL;
		/* If we're attacking or sensing (observing), then... */
		if( (orderStateObj(psDroid,DORDER_ATTACK,&psBeingTargetted)) OR
			(orderStateObj(psDroid,DORDER_OBSERVE,&psBeingTargetted)) )
		{
			/* If it's a structure */
			if(psBeingTargetted->type == OBJ_STRUCTURE)
			{
				/* And it's your structure or your droid... */
				if( (((STRUCTURE*)psBeingTargetted)->player == selectedPlayer)
					OR (psDroid->player == selectedPlayer) )
				{
					/* Highlight the structure in question */
					((STRUCTURE*)psBeingTargetted)->targetted = 1;
				}
			}
	// ffs AM
			else
			if(psBeingTargetted->type == OBJ_DROID)
			{
				/* And it's your  your droid... */
	 			if( (((DROID*)psBeingTargetted)->player == selectedPlayer)
 					OR (psDroid->player == selectedPlayer) )
 				{
 				   	((DROID*)psBeingTargetted)->bTargetted = TRUE;

 				}
			}
			else if(psBeingTargetted->type == OBJ_FEATURE)
			{
				if(psDroid->player == selectedPlayer)
				{
					((FEATURE*)psBeingTargetted)->bTargetted = TRUE;
				}
			}

		}
	}
	// -----------------
//	visTilesUpdate((BASE_OBJECT *)psDroid);

	/* Update the fire damage data */
	if (psDroid->inFire & IN_FIRE)
	{
		/* Still in a fire, reset the fire flag to see if we get out this turn */
		psDroid->inFire = 0;
	}
	else
	{
		/* The fire flag has not been set so we must be out of the fire */
		if (psDroid->inFire & BURNING)
		{
			if (psDroid->burnStart + BURN_TIME < gameTime)
			{
				// stop burning
				psDroid->inFire = 0;
				psDroid->burnStart = 0;
				psDroid->burnDamage = 0;
			}
			else
			{
				// do burn damage
				damageToDo = BURN_DAMAGE * ((SDWORD)gameTime - (SDWORD)psDroid->burnStart) /
								GAME_TICKS_PER_SEC;
				damageToDo -= (SDWORD)psDroid->burnDamage;
				if (damageToDo > 0)
				{
					psDroid->burnDamage += damageToDo;
					//psDroid->damage(psDroid, damageToDo, WC_HEAT);

					(void)droidDamage(psDroid, damageToDo, WC_HEAT,WSC_FLAME);

				}
			}
		}
		else if (psDroid->burnStart != 0)
		{
			// just left the fire
			psDroid->inFire |= BURNING;
			psDroid->burnStart = gameTime;
			psDroid->burnDamage = 0;
		}
	}

	droidUpdateRecoil(psDroid);

	// on the psx version, we do this in DisplayCompObj() in component.c
	calcDroidIllumination(psDroid);


    //check the resistance level of the droid
	if (psDroid->id % 50 == frameGetFrameNumber() % 50)
    {
        //zero resistance means not currently been attacked - ignore these
	    if (psDroid->resistance AND psDroid->resistance < droidResistance(psDroid))
	    {
		    //increase over time if low
		    psDroid->resistance++;
	    }
    }

//    ASSERT( psDroid->x != 0 && psDroid->y != 0,
//		"unitUpdate (end): Unit at (0,0)" );
}

//#ifndef PSX
//void droidUpdate(DROID *psDroid)
//{
//	droidUpd(psDroid);
//}
//#else
//void droidUpdate(DROID *psDroid)
//{
//	static DROID *psTmpDroid;
//	psTmpDroid = psDroid;
//
//	SetSpDCache();
//	droidUpd(psTmpDroid);
//	SetSpNormal();
//}
//#endif

// calculate the experience level of a droid
/*SDWORD droidCalcExp(DROID *psDroid)
{
	// figure out a to hit mod based on kills
	if (psDroid->numKills > 100)
	{
		return 6;
	}
	else if (psDroid->numKills > 50)
	{
		return 5;
	}
	else if (psDroid->numKills > 25)
	{
		return 4;
	}
	else if (psDroid->numKills > 10)
	{
		return 3;
	}
	else if (psDroid->numKills > 5)
	{
		return 2;
	}
	else if (psDroid->numKills > 1)
	{
		return 1;
	}

	return 0;
}*/


/* See if a droid is next to a structure */
static BOOL droidNextToStruct(DROID *psDroid, BASE_OBJECT *psStruct)
{
	SDWORD	minX, maxX, maxY, x,y;

	minX = (SDWORD)(psDroid->x >> TILE_SHIFT)-1;
	y = (SDWORD)(psDroid->y >> TILE_SHIFT)-1;
	maxX = minX + 2;
	maxY = y + 2;
	if (minX < 0)
	{
		minX = 0;
	}
	if (maxX >= (SDWORD)mapWidth)
	{
		maxX = (SDWORD)mapWidth;
	}
	if (y < 0)
	{
		y = 0;
	}
	if (maxY >= (SDWORD)mapHeight)
	{
		maxY = (SDWORD)mapHeight;
	}
	for(;y <= maxY; y++)
	{
		for(x = minX;x <= maxX; x++)
		{
//psor			if (mapTile(x,y)->psObject == psStruct)
			if (TILE_HAS_STRUCTURE(mapTile((UWORD)x,(UWORD)y)) &&
				getTileStructure(x,y) == (STRUCTURE *)psStruct)

			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


/* Set up a droid to build a foundation - returns true if successful */
BOOL droidStartFoundation(DROID *psDroid)
{
//	SDWORD	height;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitStartFoundation: invalid unit pointer" );

	/* See if we are starting a new structure */
	if (psDroid->order == DORDER_BUILD || psDroid->order == DORDER_LINEBUILD)
	{
        //buildStructure() calls this now so don't need it twice! AB 14/12/98
		//height = buildFoundation((STRUCTURE_STATS *)psDroid->psTarStats,
		//				psDroid->orderX,psDroid->orderY);
		//if (height >= 0)
		{
			//psDroid->actionHeight = (UWORD)height;
            //psDroid->actionHeight = 0;  //not used here anymore
			psDroid->actionStarted = gameTime;
			psDroid->actionPoints = 0;
			//psDroid->tileNumber = 0;
			return TRUE;
		}
	}
	return FALSE;
}



BOOL
droidCheckBuildStillInProgress( AUDIO_SAMPLE *psSample )
{
	DROID	*psDroid;

	ASSERT( PTRVALID(psSample, sizeof(AUDIO_SAMPLE)),
		"unitCheckBuildStillInProgress: audio sample pointer invalid\n" );

	if ( psSample->psObj == NULL )
	{
		return FALSE;
	}
	else
	{
		psDroid = psSample->psObj;
		ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"unitCheckBuildStillInProgress: unit pointer invalid\n" );
	}

	if ( !psDroid->died && psDroid->action == DACTION_BUILD )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static BOOL
droidBuildStartAudioCallback( AUDIO_SAMPLE *psSample )
{
	DROID	*psDroid;

	ASSERT( PTRVALID(psSample, sizeof(AUDIO_SAMPLE)),
		"unitBuildStartAudioCallback: audio sample pointer invalid\n" );

	psDroid = psSample->psObj;

	if ( psDroid != NULL )
	{
		ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"unitBuildStartAudioCallback: unit pointer invalid\n" );

		if ( psDroid->visible[selectedPlayer] )
		{
			audio_PlayObjDynamicTrack( psDroid, ID_SOUND_CONSTRUCTION_LOOP,
										droidCheckBuildStillInProgress );
		}
	}

	return TRUE;
}


/* Set up a droid to build a structure - returns true if successful */
BOOL droidStartBuild(DROID *psDroid)
{
	STRUCTURE			*psStruct;
	//FEATURE				*psCurr;
	STRUCTURE_STATS		*psStructStat;
	//MESSAGE				*psMessage;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitStartBuild: invalid unit pointer" );

	/* See if we are starting a new structure */
	if ((psDroid->psTarget == NULL) &&
		(psDroid->order == DORDER_BUILD ||
		 psDroid->order == DORDER_LINEBUILD))
	{
		//need to check structLimits have not been exceeded
		psStructStat = (STRUCTURE_STATS *)psDroid->psTarStats;
		if (asStructLimits[psDroid->player][psStructStat - asStructureStats].
			currentQuantity >= asStructLimits[psDroid->player][psStructStat -
			asStructureStats].limit)
		{
			intBuildFinished(psDroid);
			return FALSE;
		}
		//ok to build
		psStruct = buildStructure(psStructStat, psDroid->orderX,psDroid->orderY, psDroid->player,FALSE);
		if (!psStruct)
		{
			intBuildFinished(psDroid);
			return FALSE;
		}
		//add one to current quantity for this player
		asStructLimits[psDroid->player][psStructStat - asStructureStats].
			currentQuantity++;

		//The following is done in buildStructure now - AB 3/9/98
		//if resource extractor - need to remove oil feature and prox Msg
		/*if (psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			//find the resource at this point
			for (psCurr = apsFeatureLists[0]; psCurr != NULL; psCurr = psCurr->psNext)
			{
				if (psCurr->psStats->subType == FEAT_OIL_RESOURCE)
				{
					if ((psCurr->x == psStruct->x) AND (psCurr->y == psStruct->y))
					{
						//see if there is a proximity message at this location
						psMessage = findMessage(psCurr, MSG_PROXIMITY, psStruct->player);
						if (psMessage)
						{
							removeMessage(psMessage, psStruct->player);
						}
						//remove it from the map
						removeFeature(psCurr);
						//set the map to hold the resource extractor again
						SET_TILE_STRUCTURE(mapTile(psCurr->x >> TILE_SHIFT,
							psCurr->y >> TILE_SHIFT));
					}
				}
			}
		}*/

		//commented out for demo - 2/1/98
		//ASSERT( droidNextToStruct(psDroid, (BASE_OBJECT *)psStruct),
		//	"droidStartBuild: did not build structure next to droid" );


		if (bMultiPlayer)
		{
//			psStruct->id = psDroid->buildingId;// Change the id to the constructor droids required id.

			if(myResponsibility(psDroid->player) )
			{
				sendBuildStarted(psStruct, psDroid);
			}
		}

	}
	else
	{
		/* Check the structure is still there to build (joining a partially built struct) */
		psStruct = (STRUCTURE *)psDroid->psTarget;
		if (!droidNextToStruct(psDroid,  (BASE_OBJECT *)psStruct))
		{
			/* Nope - stop building */
			debug( LOG_NEVER, "unitStartBuild: not next to structure\n" );
		}
	}

	//check structure not already built
	if (psStruct->status != SS_BUILT)
	{
		psDroid->actionStarted = gameTime;
		psDroid->actionPoints = 0;
		psDroid->psTarget = (BASE_OBJECT *)psStruct;
		psDroid->psActionTarget = (BASE_OBJECT *)psStruct;
	}


	if ( psStruct->visible[selectedPlayer] )
	{
		audio_PlayObjStaticTrackCallback( psDroid, ID_SOUND_CONSTRUCTION_START,
										droidBuildStartAudioCallback );
	}


	return TRUE;
}

static void droidAddWeldSound( iVector iVecEffect )
{
	SDWORD		iAudioID;


	iAudioID = ID_SOUND_CONSTRUCTION_1 + (rand()%4);


	audio_PlayStaticTrack( iVecEffect.x, iVecEffect.z, iAudioID );
}

static void addConstructorEffect(STRUCTURE *psStruct)
{
	UDWORD		widthRange,breadthRange;
	iVector		temp;

// We definitly still want this called on the PSX.

	//FIXME
	if((ONEINTEN) AND (psStruct->visible[selectedPlayer]))
	{
		/* This needs fixing - it's an arse effect! */
		widthRange = (psStruct->pStructureType->baseWidth*TILE_UNITS)/4;
		breadthRange = (psStruct->pStructureType->baseBreadth*TILE_UNITS)/4;
		temp.x = psStruct->x+((rand()%(2*widthRange)) - widthRange);
		temp.y = map_TileHeight(psStruct->x>>TILE_SHIFT, psStruct->y>>TILE_SHIFT)+
						((psStruct->sDisplay.imd->ymax)/6);
		temp.z = psStruct->y+((rand()%(2*breadthRange)) - breadthRange);
//FIXFX				addExplosion(&temp,TYPE_EXPLOSION_SMOKE_CLOUD,NULL);
		if(rand()%2)
		{
			droidAddWeldSound( temp );
		}
	}
}

/* Update a construction droid while it is building
   returns TRUE while building continues */
BOOL droidUpdateBuild(DROID *psDroid)
{
//	UDWORD		widthRange,breadthRange;
//	iVector		temp;
	UDWORD		pointsToAdd, constructPoints;//, powerPercent, buildPercent;
	STRUCTURE	*psStruct;
	//UDWORD		mapX, mapY, i, j;
	//UBYTE		prevScale, currScale, current = 0, prev = 0;

	ASSERT( psDroid->action == DACTION_BUILD,
		"unitUpdateBuild: unit is not building" );
	psStruct = (STRUCTURE *)psDroid->psTarget;
	ASSERT( psStruct->type == OBJ_STRUCTURE,
		"unitUpdateBuild: target is not a structure" );
	ASSERT( psDroid->asBits[COMP_CONSTRUCT].nStat < numConstructStats,
		"unitUpdateBuild: Invalid construct pointer for unit" );

	/* Check the structure is still there to build */
	// FIXME - need to ensure construction droids move nearer to structures before building
/*	if (!droidNextToStruct(psDroid, (BASE_OBJECT *)psStruct))
	{
		// Nope - stop building
		return FALSE;
	}*/

	//first check the structure hasn't been completed by another droid
	if (psStruct->status == SS_BUILT)
	{
		//update the interface
		intBuildFinished(psDroid);
		return FALSE;
	}

    //for now wait until have enough power to build
    //if (psStruct->currentPowerAccrued < (SWORD)psStruct->pStructureType->powerToBuild)
    if (psStruct->currentPowerAccrued < (SWORD)structPowerToBuild(psStruct))
    {
        psDroid->actionStarted = gameTime;
        return TRUE;
    }
    //the amount of power accrued limits how much of the work can have been done
    /*if (psStruct->pStructureType->powerToBuild)
    {
        powerPercent = 100 * psStruct->currentPowerAccrued / psStruct->
            pStructureType->powerToBuild;
    }
    else
    {
        powerPercent = 100;
    }
    buildPercent = 100 * psStruct->currentBuildPts / psStruct->
        pStructureType->buildPoints;

    //check if enough power to do more
    if (buildPercent > powerPercent)
    {
        //reset the actionStarted time and actionPoints added so the correct
        //amount of points are added when there is more power
        psDroid->actionStarted = gameTime;
        psDroid->actionPoints = 0;
        return TRUE;
    }*/

	//constructPoints = (asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat)->
	//	constructPoints;
	constructPoints = constructorPoints(asConstructStats + psDroid->
		asBits[COMP_CONSTRUCT].nStat, psDroid->player);

	pointsToAdd = constructPoints * (gameTime - psDroid->actionStarted) /
		GAME_TICKS_PER_SEC;

	//these two lines needs to be removed if illumination is put pack in
	psStruct->currentBuildPts = (SWORD)(psStruct->currentBuildPts + pointsToAdd - psDroid->actionPoints);
	//psStruct->heightScale = (MAKEFRACT(psStruct->currentBuildPts))/(psStruct->pStructureType->buildPoints);
	//ILLUMINATION ISN'T BEING DONE ANYMORE
	/*
	//reserve the previous value
	prevScale= (UBYTE) MAKEINT(psStruct->heightScale*100);
	prev = (UBYTE)(prevScale / (UBYTE)10);

	psStruct->currentBuildPts += (pointsToAdd - psDroid->actionPoints);
	psStruct->heightScale = (MAKEFRACT(psStruct->currentBuildPts))/(psStruct->pStructureType->buildPoints);

	currScale = (UBYTE) MAKEINT(psStruct->heightScale * 100);
	current = (UBYTE)(currScale / (UBYTE)10);

	if (current != prev)
	{
		prev *= 10;
		current *= 10;
	}

	if (current != prev)
	{
#ifdef PSX
		FRACT divisor1,divisor2;
		divisor1= (MAKEFRACT(FOUNDATION_ILLUMIN + prev) - ((MAKEFRACT(FOUNDATION_ILLUMIN * prev))/100))/100;
		divisor2= (MAKEFRACT(FOUNDATION_ILLUMIN + current) - ((MAKEFRACT(FOUNDATION_ILLUMIN * current))/100))/100;
#endif
		//set the illumination of the tiles back to original value as building is completed

		mapX = (psStruct->x - psStruct->pStructureType->baseWidth *
			TILE_UNITS / 2) >> TILE_SHIFT;
		mapY = (psStruct->y - psStruct->pStructureType->baseBreadth *
			TILE_UNITS / 2) >> TILE_SHIFT;



		for (i = 0; i < psStruct->pStructureType->baseWidth+1; i++)
		{
			for (j = 0; j < psStruct->pStructureType->baseBreadth+1; j++)
			{
#ifndef PSX
				FRACT	divisor,illumin, currentIllumin;

				divisor = (FOUNDATION_ILLUMIN + prev -
					(FOUNDATION_ILLUMIN * prev)/(FRACT)100) / (FRACT)100;
				//work out what the initial value was before modifier was applied
				currentIllumin = mapTile(mapX+i, mapY+j)->illumination;
				illumin = currentIllumin / divisor;
				divisor = ( FOUNDATION_ILLUMIN+current-(FOUNDATION_ILLUMIN*current)/(FRACT)100 )
																			/ (FRACT)100;
				illumin = illumin * divisor;
				mapTile(mapX+i, mapY+j)->illumination = (UBYTE)illumin;

#else
				FRACT	illumin;
				MAPTILE *map;

				map=mapTile(mapX+i,mapY+j);
				illumin = FRACTdiv_1(MAKEFRACT(map->illumination),divisor1);	// Calculate the old illumination before we built
				map->illumination = (UBYTE)MAKEINT(FRACTmul_1(illumin,divisor2)); // new calculate the new adjust illumination
#endif
			}
		}
		prev = current;
	}
	*/

	//store the amount just added
	psDroid->actionPoints = pointsToAdd;

	//check if structure is built
	if (psStruct->currentBuildPts > (SDWORD)psStruct->pStructureType->buildPoints)
	{
		psStruct->currentBuildPts = (SWORD)psStruct->pStructureType->buildPoints;
		psStruct->status = SS_BUILT;
		buildingComplete(psStruct);
		// update the power if necessary
		/*if (psStruct->pStructureType->type == REF_POWER_GEN)
		{
			capacityUpdate(psStruct);
		}
		else if (psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR OR
			psStruct->pStructureType->type == REF_HQ)
		{
			extractedPowerUpdate(psStruct);
		}*/

		intBuildFinished(psDroid);

		/* This is done in buildingComplete now - AB 14/09/98
		GJ HACK! - add anim to deriks
		if ( psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR &&
				psStruct->psCurAnim == NULL )
		{
			psStruct->psCurAnim = animObj_Add( psStruct, ID_ANIM_DERIK, 0, 0 );
		}*/


		if((bMultiPlayer) && myResponsibility(psStruct->player))
		{
			SendBuildFinished(psStruct);
		}


		//only play the sound if selected player
		if ((psStruct->player == selectedPlayer) &&
			(psDroid->order != DORDER_LINEBUILD ||
			(psDroid->orderX >> TILE_SHIFT == psDroid->orderX2 >> TILE_SHIFT &&
			 psDroid->orderY >> TILE_SHIFT == psDroid->orderY2 >> TILE_SHIFT)))
		{
			audio_QueueTrackPos( ID_SOUND_STRUCTURE_COMPLETED,
					psStruct->x, psStruct->y, psStruct->z );
			intRefreshScreen();		// update any open interface bars.
		}
		structureCompletedCallback(psStruct->pStructureType);


		audio_StopObjTrack( psDroid, ID_SOUND_CONSTRUCTION_LOOP );

		return FALSE;
	}
	else
	{
		addConstructorEffect(psStruct);
//		//FIXME
//		if((ONEINTEN) AND (psStruct->visible[selectedPlayer]))
//			{
//				/* This needs fixing - it's an arse effect! */
//				widthRange = (psStruct->pStructureType->baseWidth*TILE_UNITS)/3;
//				breadthRange = (psStruct->pStructureType->baseBreadth*TILE_UNITS)/3;
//				temp.x = psStruct->x+((rand()%(2*widthRange)) - widthRange);
//				temp.y = map_TileHeight(psStruct->x>>TILE_SHIFT, psStruct->y>>TILE_SHIFT)+
//								((psStruct->sDisplay.imd->ymax)/2);
//				temp.z = psStruct->y+((rand()%(2*breadthRange)) - breadthRange);
////FIXFX				addExplosion(&temp,TYPE_EXPLOSION_SMOKE_CLOUD,NULL);
//				if(rand()%2)
//				{
//					addEffect(&temp,EFFECT_EXPLOSION,EXPLOSION_TYPE_PLASMA,FALSE,NULL,0);
//				}
//				else
//				{
//					addEffect(&temp,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,FALSE,NULL,0);
//				}
//			}
	}
	return TRUE;
}

//NEW VERSION WHEREBY THE GROUND IS LEVELLED IN ONE GO
/* Update a construction droid while it is building a
   foundation. Returns TRUE whilst foundation continues */
BOOL droidUpdateFoundation(DROID *psDroid)
{
	//UBYTE				width, breadth;
//	STRUCTURE_STATS		*psStructStats = (STRUCTURE_STATS *)psDroid->psTarStats;
	//UDWORD				x;
	//UDWORD				y;

	ASSERT( psDroid->action == DACTION_BUILD_FOUNDATION,
		"unitUpdateFoundation: unit is not building foundation" );
	/*x = (SDWORD)psDroid->orderX - (SDWORD)(psStructStats->baseWidth * TILE_UNITS)/2;
	y = (SDWORD)psDroid->orderY - (SDWORD)(psStructStats->baseBreadth * TILE_UNITS)/2;

	x = (x >> TILE_SHIFT);
	y = (y >> TILE_SHIFT);

	//find a section that is not at the actionHeight
	for (breadth = 0; breadth < (UBYTE)(psStructStats->baseBreadth + 1); breadth++)
	{
		for (width = 0; width < (UBYTE)(psStructStats->baseWidth + 1); width++)
		{
			//set the tile to the required foundation height
			setTileHeight(x + width, y + breadth, psDroid->actionHeight);
			// We need to raise features on raised tiles to the new height
			if(TILE_HAS_FEATURE(mapTile(x+width,y+breadth)))
			{
				getTileFeature(x+width, y+breadth)->z =
					psDroid->actionHeight;
			}
		}
	}
	//all must be at actionHeight
	return FALSE;*/

    //don't do this anymore since happens in one go and is called by buildStructure - see buildFlatten()!!
    return FALSE;
}

//OLD VERSION WHEREBY THE GROUND IS LOWERED/RAISED TILE BY TILE
/* Update a construction droid while it is building a
   foundation. Returns TRUE whilst foundation continues */
/*BOOL droidUpdateFoundation(DROID *psDroid)
{
	UBYTE				width, breadth, buildSpeed = 160, illumin;
	STRUCTURE_STATS		*psStructStats = (STRUCTURE_STATS *)psDroid->psTarStats;
	UDWORD				x = psDroid->orderX >> TILE_SHIFT;
	UDWORD				y = psDroid->orderY >> TILE_SHIFT;
	UDWORD				pointsToAdd, height;
	SDWORD				newHeight;

	ASSERT( psDroid->action == DACTION_BUILD_FOUNDATION,
		"droidUpdateFoundation: droid is not building foundation" );

	//we want this to happen almost immediately...so add lots each cycle
	//pointsToAdd = 40 * (gameTime - psDroid->actionStarted) / GAME_TICKS_PER_SEC;
	pointsToAdd = gameTime - psDroid->actionStarted;

	//if time to raise a new section
	if (pointsToAdd > psDroid->actionPoints)
	{
		//find a section that is not at the actionHeight
		breadth = (UBYTE)(psDroid->tileNumber / (psStructStats->baseBreadth + 1));
		for (; breadth < (UBYTE)(psStructStats->baseBreadth + 1); breadth++)
		{
			width = (UBYTE)(psDroid->tileNumber % (psStructStats->baseWidth + 1));
			for (; width < (UBYTE)(psStructStats->baseWidth + 1); width++)
			{

				//set the illumination of the tile to FOUNDATION_ILLUM% darker
				illumin = mapTile(x + width, y + breadth)->illumination;
				DBPRINTF(("Start illum=%d\n",illumin);
				illumin = (UBYTE)((FOUNDATION_ILLUMIN * illumin) / 100);
				mapTile(x + width, y + breadth)->illumination = illumin;

				height = map_TileHeight(x + width, y + breadth);
				if (height < psDroid->actionHeight)
				{
					newHeight = height + (buildSpeed * (pointsToAdd -
						psDroid->actionPoints));
					if (newHeight > (SDWORD)psDroid->actionHeight)
					{
						//got to the right height
						newHeight = psDroid->actionHeight;
						//psDroid->tileNumber++;
						moveToNewTile(psDroid);
					}
					setTileHeight(x + width, y + breadth, newHeight);
					// We need to raise features on raised tiles to the new height
					if(TILE_HAS_FEATURE(mapTile(x+width,y+breadth)))
					{
						getTileFeature(x+width,y+breadth)->z = newHeight;
					}
					psDroid->actionPoints = pointsToAdd;
					return TRUE;
				}
				else if (height > psDroid->actionHeight)
				{
					newHeight = (SDWORD)(height - (buildSpeed * (pointsToAdd -
						psDroid->actionPoints)));
					if (newHeight < (SDWORD)psDroid->actionHeight)
					{
						//got to the right height
						newHeight = psDroid->actionHeight;
						//psDroid->tileNumber++;
						moveToNewTile(psDroid);
					}
					setTileHeight(x + width, y + breadth, newHeight);
					if(TILE_HAS_FEATURE(mapTile(x+width,y+breadth)))
					{
						getTileFeature(x+width,y+breadth)->z = newHeight;
					}
					psDroid->actionPoints = pointsToAdd;
					return TRUE;
				}
				else
				{
					//already at the right height
					//psDroid->tileNumber++;
					moveToNewTile(psDroid);
				}
			}
		}
		//all must be at actionHeight
		return FALSE;
	}
	//still building
	return TRUE;
}*/


/*time to move to a new location (when building foundation) */
/*void moveToNewTile(DROID *psDroid)
{
	UBYTE				breadth, width;
	UDWORD				newX, newY;
	STRUCTURE_STATS		*psStructStats = (STRUCTURE_STATS *)psDroid->psTarStats;
	UDWORD				x = psDroid->orderX >> TILE_SHIFT;
	UDWORD				y = psDroid->orderY >> TILE_SHIFT;

	//increment the tile index
	psDroid->tileNumber++;

	//determine the tile now working on
	breadth = (UBYTE)(psDroid->tileNumber / (psStructStats->baseBreadth + 1));
	width = (UBYTE)(psDroid->tileNumber % (psStructStats->baseWidth + 1));
	newX = x + width;
	newY = y + breadth;

	//find a free location near the tile
	if (!pickATile(&newX, &newY,LOOK_FOR_EMPTY_TILE))
	{
		ASSERT( FALSE, "moveToNewTile: Unable to find a free location" );
	}

	//order the droid to move
	psDroid->action = DACTION_FOUNDATION_WANDER;
	newX = newX << TILE_SHIFT;
	newY = newY << TILE_SHIFT;
	psDroid->actionX = newX;
	psDroid->actionY = newY;
	moveDroidTo(psDroid, newX, newY);
}*/

BOOL droidStartDemolishing( DROID *psDroid )
{
	STRUCTURE	*psStruct;

	ASSERT( psDroid->order == DORDER_DEMOLISH,
		"unitStartDemolishing: unit is not demolishing" );
	psStruct = (STRUCTURE *)psDroid->psTarget;
	ASSERT( psStruct->type == OBJ_STRUCTURE,
		"unitStartDemolishing: target is not a structure" );

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	/* init build points Don't - could be partially demolished*/
	//psStruct->currentBuildPts = psStruct->pStructureType->buildPoints;
	psStruct->status = SS_BEING_DEMOLISHED;

	// Set height scale for demolishing
	//psStruct->heightScale = (MAKEFRACT(psStruct->currentBuildPts))/(
	//	psStruct->pStructureType->buildPoints);

	//if start to demolish a power gen need to inform the derricks
	if (psStruct->pStructureType->type == REF_POWER_GEN)
	{
		releasePowerGen(psStruct);
	}

	return TRUE;
}

BOOL droidUpdateDemolishing( DROID *psDroid )
{
	STRUCTURE	*psStruct;
	UDWORD		pointsToAdd, constructPoints;

	ASSERT( psDroid->action == DACTION_DEMOLISH,
		"unitUpdateDemolishing: unit is not demolishing" );
	psStruct = (STRUCTURE *)psDroid->psTarget;
	ASSERT( psStruct->type == OBJ_STRUCTURE,
		"unitUpdateDemolishing: target is not a structure" );

	//constructPoints = (asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat)->
	//	constructPoints;
	constructPoints = constructorPoints(asConstructStats + psDroid->
		asBits[COMP_CONSTRUCT].nStat, psDroid->player);

	pointsToAdd = constructPoints * (gameTime - psDroid->actionStarted) /
		GAME_TICKS_PER_SEC;

	psStruct->currentBuildPts = (SWORD)(psStruct->currentBuildPts - pointsToAdd - psDroid->actionPoints);

	//psStruct->heightScale = (MAKEFRACT(psStruct->currentBuildPts))/(psStruct->pStructureType->buildPoints);

	//store the amount just subtracted
	psDroid->actionPoints = pointsToAdd;

	/* check if structure is demolished */
	if ( psStruct->currentBuildPts <= 0 )
	{

		if(bMultiPlayer)
		{
			SendDemolishFinished(psStruct,psDroid);
		}


		if(psStruct->pStructureType->type == REF_POWER_GEN)
		{
            //can't assume it was completely built!!
			// put back all the power
			/*addPower(psStruct->player, structPowerToBuild(psStruct));
            //if it had a module attached, need to add the power for the base struct as well
            if (((POWER_GEN *)psStruct->pFunctionality)->capacity)
            {
                addPower(psStruct->player, psStruct->pStructureType->powerToBuild);
            }*/

            //if had module attached - the base must have been completely built
            if (((POWER_GEN *)psStruct->pFunctionality)->capacity)
            {
                //so add the power required to build the base struct
                addPower(psStruct->player, psStruct->pStructureType->powerToBuild);
            }
            //add the currentAccruedPower since this may or may not be all required
            addPower(psStruct->player, psStruct->currentPowerAccrued);
		}
		else
		{
            /* CAN'T assume completely built
			//put back half the power required to build this structure (=power to build)
			addPower(psStruct->player, structPowerToBuild(psStruct) / 2);
            //if it had a module attached, need to add the power for the base struct as well
            if (StructIsFactory(psStruct))
            {
                if (((FACTORY *)psStruct->pFunctionality)->capacity)
                {
                    addPower(psStruct->player, psStruct->pStructureType->
                        powerToBuild / 2);
                }
            }
            else if (psStruct->pStructureType->type == REF_RESEARCH)
            {
                if (((RESEARCH_FACILITY *)psStruct->pFunctionality)->capacity)
                {
                    addPower(psStruct->player, psStruct->pStructureType->
                        powerToBuild / 2);
                }
            }*/
            //if it had a module attached, need to add the power for the base struct as well
            if (StructIsFactory(psStruct))
            {
                if (((FACTORY *)psStruct->pFunctionality)->capacity)
                {
                    //add half power for base struct
                    addPower(psStruct->player, psStruct->pStructureType->
                        powerToBuild / 2);
                    //if large factory - add half power for one upgrade
                    if (((FACTORY *)psStruct->pFunctionality)->capacity > SIZE_MEDIUM)
                    {
                        addPower(psStruct->player, structPowerToBuild(psStruct) / 2);
                    }
                }
            }
            else if (psStruct->pStructureType->type == REF_RESEARCH)
            {
                if (((RESEARCH_FACILITY *)psStruct->pFunctionality)->capacity)
                {
                    //add half power for base struct
                    addPower(psStruct->player, psStruct->pStructureType->powerToBuild / 2);
                }
            }
            //add currentAccrued for the current layer of the structure
            addPower(psStruct->player, psStruct->currentPowerAccrued / 2);
        }
		/* remove structure and foundation */
		removeStruct( psStruct, TRUE );

		/* reset target stats*/
	    psDroid->psTarStats = NULL;

		return FALSE;
	}
    else
    {
		addConstructorEffect(psStruct);
	}

	return TRUE;
}

/* Set up a droid to clear a wrecked building feature - returns true if successful */
BOOL droidStartClearing( DROID *psDroid )
{
	FEATURE			*psFeature;

	ASSERT( psDroid->order == DORDER_CLEARWRECK,
		"unitStartClearing: unit is not clearing wreckage" );
	psFeature = (FEATURE *)psDroid->psTarget;
	ASSERT( psFeature->type == OBJ_FEATURE,
		"unitStartClearing: target is not a feature" );
	ASSERT( psFeature->psStats->subType == FEAT_BUILD_WRECK,
		"unitStartClearing: feature is not a wrecked building" );

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	return TRUE;
}

/* Update a construction droid while it is clearing
   returns TRUE while continues */
BOOL droidUpdateClearing( DROID *psDroid )
{
	FEATURE		*psFeature;
	UDWORD		pointsToAdd, constructPoints;

	ASSERT( psDroid->action == DACTION_CLEARWRECK,
		"unitUpdateClearing: unit is not clearing wreckage" );
	psFeature = (FEATURE *)psDroid->psTarget;
	ASSERT( psFeature->type == OBJ_FEATURE,
		"unitStartClearing: target is not a feature" );
	ASSERT( psFeature->psStats->subType == FEAT_BUILD_WRECK,
		"unitStartClearing: feature is not a wrecked building" );

	if (psFeature->body > 0)
	{
		constructPoints = constructorPoints(asConstructStats + psDroid->
			asBits[COMP_CONSTRUCT].nStat, psDroid->player);

		pointsToAdd = constructPoints * (gameTime - psDroid->actionStarted) /
			GAME_TICKS_PER_SEC;

		psFeature->body -= (pointsToAdd - psDroid->actionPoints);

		//store the amount just subtracted
		psDroid->actionPoints = pointsToAdd;
	}

	/* check if structure is demolished */
	if ( psFeature->body <= 0 )
	{
		/* remove feature */
		removeFeature(psFeature);

		/* reset target stats */
		psDroid->psTarStats = NULL;

		return FALSE;
	}

	return TRUE;
}

BOOL droidStartRepair( DROID *psDroid )
{
	STRUCTURE	*psStruct;

	//ASSERT( psDroid->order == DORDER_REPAIR,
	//	"droidStartRepair: droid does not have repair order" );
	//psStruct = (STRUCTURE *)psDroid->psTarget;
	psStruct = (STRUCTURE *)psDroid->psActionTarget;
	ASSERT( psStruct->type == OBJ_STRUCTURE,
		"unitStartRepair: target is not a structure" );

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	return TRUE;
}


/*Start a Repair Droid working on a damaged droid*/
BOOL droidStartDroidRepair( DROID *psDroid )
{
	DROID	*psDroidToRepair;

//	ASSERT( psDroid->order == DORDER_DROIDREPAIR,
//		"droidStartDroidRepair: droid does not have droid repair order" );
	psDroidToRepair = (DROID *)psDroid->psActionTarget;
	ASSERT( psDroidToRepair->type == OBJ_DROID,
		"unitStartUnitRepair: target is not a unit" );

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	return TRUE;
}

/*checks a droids current body points to see if need to self repair*/
void droidSelfRepair(DROID *psDroid)
{
    if (!vtolDroid(psDroid))
    {
	    if (psDroid->body < psDroid->originalBody)
	    {
		    if (psDroid->asBits[COMP_REPAIRUNIT].nStat != 0)
		    {
			    psDroid->action = DACTION_DROIDREPAIR;
//			    psDroid->psTarget = (BASE_OBJECT *)psDroid;
                psDroid->psActionTarget = (BASE_OBJECT *)psDroid;
			    psDroid->actionStarted = gameTime;
			    psDroid->actionPoints  = 0;
		    }
	    }
    }
}


/*Start a EW weapon droid working on a low resistance structure*/
BOOL droidStartRestore( DROID *psDroid )
{
	STRUCTURE	*psStruct;

	ASSERT( psDroid->order == DORDER_RESTORE,
		"unitStartRestore: unit is not restoring" );
	psStruct = (STRUCTURE *)psDroid->psTarget;
	ASSERT( psStruct->type == OBJ_STRUCTURE,
		"unitStartRestore: target is not a structure" );

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	return TRUE;
}

/*continue restoring a structure*/
BOOL droidUpdateRestore( DROID *psDroid )
{
	STRUCTURE		*psStruct;
	UDWORD			pointsToAdd, restorePoints;
	WEAPON_STATS	*psStats;

	ASSERT( psDroid->action == DACTION_RESTORE,
		"unitUpdateRestore: unit is not restoring" );
	psStruct = (STRUCTURE *)psDroid->psTarget;
	ASSERT( psStruct->type == OBJ_STRUCTURE,
		"unitUpdateRestore: target is not a structure" );
	ASSERT( psStruct->pStructureType->resistance != 0,
		"unitUpdateRestore: invalid structure for EW" );

	//ASSERT( psDroid->numWeaps != 0,
    ASSERT( psDroid->asWeaps[0].nStat > 0,
		"unitUpdateRestore: droid doean't have any weapons" );

	psStats = asWeaponStats + psDroid->asWeaps[0].nStat;

	ASSERT( psStats->weaponSubClass == WSC_ELECTRONIC,
		"unitUpdateRestore: unit's weapon is not EW" );

	//restorePoints = calcDamage(psStats->damage, psStats->weaponEffect,(BASE_OBJECT *)psStruct);
	restorePoints = calcDamage(weaponDamage(psStats, psDroid->player),
		psStats->weaponEffect,(BASE_OBJECT *)psStruct);

	pointsToAdd = restorePoints * (gameTime - psDroid->actionStarted) /
		GAME_TICKS_PER_SEC;

	psStruct->resistance = (SWORD)(psStruct->resistance + (pointsToAdd -
        psDroid->actionPoints));

	//store the amount just added
	psDroid->actionPoints = pointsToAdd;

	/* check if structure is restored */
	//if ( psStruct->resistance < (SDWORD)(psStruct->pStructureType->resistance))
	if (psStruct->resistance < (SDWORD)structureResistance(psStruct->
		pStructureType, psStruct->player))
	{
		return TRUE;
	}
	else
	{
		addConsoleMessage(strresGetString(psStringRes,STR_GAM_STRREST) ,DEFAULT_JUSTIFY);
		//psStruct->resistance = psStruct->pStructureType->resistance;
		psStruct->resistance = (UWORD)structureResistance(psStruct->pStructureType,
			psStruct->player);
		return FALSE;
	}
}

/* Code to have the droid's weapon assembly rock back upon firing */
void	droidUpdateRecoil( DROID *psDroid )
{
UDWORD	percent;
UDWORD	recoil;
FRACT	fraction;

	/* Check it's actually got a weapon */
	//if(psDroid->numWeaps == 0)
    if(psDroid->asWeaps[0].nStat == 0)
	{
		return;
	}

	/* We have a weapon */
	if(gameTime > (psDroid->asWeaps[0].lastFired + DEFAULT_RECOIL_TIME) )
	{
		/* Recoil effect is over */
		psDroid->asWeaps[0].recoilValue = 0;
		return;
	}

	/* Where should the assembly be? */
	percent = PERCENT((gameTime-psDroid->asWeaps[0].lastFired),DEFAULT_RECOIL_TIME);

	/* Outward journey */
	if(percent >= 50)
	{
		recoil = (100 - percent)/5;
	}
	/* Return journey */
	else
	{
		recoil = percent/5;
	}

	fraction =
		MAKEFRACT(asWeaponStats[psDroid->asWeaps[0].nStat].recoilValue)/
		(MAKEFRACT(100));

	recoil = MAKEINT( MAKEFRACT(recoil) * fraction);

	/* Put it into the weapon data */
	psDroid->asWeaps[0].recoilValue = recoil;
}


BOOL droidUpdateRepair( DROID *psDroid )
{
	STRUCTURE	*psStruct;
	UDWORD		iPointsToAdd, iRepairPoints;

	ASSERT( psDroid->action == DACTION_REPAIR,
		"unitUpdateRepair: unit does not have repair order" );
	//psStruct = (STRUCTURE *)psDroid->psTarget;
	psStruct = (STRUCTURE *)psDroid->psActionTarget;
	ASSERT( psStruct->type == OBJ_STRUCTURE,
		"unitUpdateRepair: target is not a structure" );

	//iRepairPoints = asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat)->
	//	constructPoints;
	iRepairPoints = constructorPoints(asConstructStats + psDroid->
		asBits[COMP_CONSTRUCT].nStat, psDroid->player);

	iPointsToAdd = iRepairPoints * (gameTime - psDroid->actionStarted) /
		GAME_TICKS_PER_SEC;

	/* add points to structure */
	psStruct->body = (UWORD)(psStruct->body  + (iPointsToAdd - psDroid->actionPoints));

	/* store the amount just added */
	psDroid->actionPoints = iPointsToAdd;

	/* if not finished repair return TRUE else complete repair and return FALSE */
	//if ( psStruct->body < psStruct->baseBodyPoints )
	if ( psStruct->body < structureBody(psStruct))
	{
		return TRUE;
	}
	else
	{
		//psStruct->body = psStruct->baseBodyPoints;
		psStruct->body = (UWORD)structureBody(psStruct);
		return FALSE;
	}
}

/*Updates a Repair Droid working on a damaged droid*/
BOOL droidUpdateDroidRepair(DROID *psRepairDroid)
{
	DROID		*psDroidToRepair;
	UDWORD		iPointsToAdd, iRepairPoints, powerCost;
	iVector		iVecEffect;

	ASSERT( psRepairDroid->action == DACTION_DROIDREPAIR,
		"unitUpdateUnitRepair: unit does not have unit repair order" );
	ASSERT( psRepairDroid->asBits[COMP_REPAIRUNIT].nStat != 0,
		"unitUpdateUnitRepair: unit does not have a repair turret" );

	psDroidToRepair = (DROID *)psRepairDroid->psActionTarget;
	ASSERT( psDroidToRepair->type == OBJ_DROID,
		"unitUpdateUnitRepair: target is not a unit" );

    //nah - once more unto the breach my friend...or something like that...

    //Changed again so waits until got enough power to do the repair work - AB 5/3/99
    //Do repair gradually as power comes in
    //for now wait until have enough power to repair
    /*if (powerReqForDroidRepair(psDroidToRepair) > psDroidToRepair->powerAccrued)
    {
        psRepairDroid->actionStarted = gameTime;
        psRepairDroid->actionPoints = 0;
        return TRUE;
    }*/

    //the amount of power accrued limits how much of the work can be done
    //self-repair doesn't cost power
    if (psRepairDroid == psDroidToRepair)
    {
        powerCost = 0;
    }
    else
    {
        //check if enough power to do any
        //powerCost = (powerReqForDroidRepair(psDroidToRepair) -
        //    psDroidToRepair->powerAccrued) / POWER_FACTOR;
        powerCost = powerReqForDroidRepair(psDroidToRepair);
        if (powerCost > psDroidToRepair->powerAccrued)
        {
            powerCost = (powerCost - psDroidToRepair->powerAccrued) / POWER_FACTOR;
        }
        else
        {
            powerCost = 0;
        }

    }

    //if the power cost is 0 (due to rounding) then do for free!
    if (powerCost)
    {
        if (!psDroidToRepair->powerAccrued)
        {
            //reset the actionStarted time and actionPoints added so the correct
            //amount of points are added when there is more power
			psRepairDroid->actionStarted = gameTime;
            //init so repair points to add won't be huge when start up again
            psRepairDroid->actionPoints = 0;
            return TRUE;
        }
    }

	//iRepairPoints = asRepairStats[psRepairDroid->asBits[COMP_REPAIRUNIT].
	//	nStat].repairPoints;
	iRepairPoints = repairPoints(asRepairStats + psRepairDroid->
		asBits[COMP_REPAIRUNIT].nStat, psRepairDroid->player);

	//if self repair then add repair points depending on the time delay for the stat
	if (psRepairDroid == psDroidToRepair)
	{
		iPointsToAdd = iRepairPoints * (gameTime - psRepairDroid->actionStarted) /
			(asRepairStats + psRepairDroid->asBits[COMP_REPAIRUNIT].nStat)->time;
	}
	else
	{
		iPointsToAdd = iRepairPoints * (gameTime - psRepairDroid->actionStarted) /
			GAME_TICKS_PER_SEC;
	}

    iPointsToAdd -= psRepairDroid->actionPoints;

    if (iPointsToAdd)
    {
        //just add the points if the power cost is negligable
        //if these points would make the droid healthy again then just add
        if (!powerCost OR (psDroidToRepair->body + iPointsToAdd >= psDroidToRepair->originalBody))
        {
            //anothe HACK but sorts out all the rounding errors when values get small
            psDroidToRepair->body += iPointsToAdd;
            psRepairDroid->actionPoints += iPointsToAdd;
        }
        else
        {
            //see if we have enough power to do this amount of repair
            powerCost = iPointsToAdd * repairPowerPoint(psDroidToRepair);
            if (powerCost <= psDroidToRepair->powerAccrued)
            {
                psDroidToRepair->body += iPointsToAdd;
                psRepairDroid->actionPoints += iPointsToAdd;
                //subtract the power cost for these points
                psDroidToRepair->powerAccrued -= powerCost;
            }
            else
            {
                /*reset the actionStarted time and actionPoints added so the correct
                amount of points are added when there is more power*/
                psRepairDroid->actionStarted = gameTime;
                psRepairDroid->actionPoints = 0;
            }
        }
    }


	/* add points to droid */
	//psDroidToRepair->body += (iPointsToAdd - psRepairDroid->actionPoints);

	/* store the amount just added */
	//psRepairDroid->actionPoints = iPointsToAdd;

	/* add plasma repair effect whilst being repaired */
	if ((ONEINFIVE) AND (psDroidToRepair->visible[selectedPlayer]))
	{
		iVecEffect.x = psDroidToRepair->x + DROID_REPAIR_SPREAD;
		iVecEffect.y = psDroidToRepair->z + rand()%8;;
		iVecEffect.z = psDroidToRepair->y + DROID_REPAIR_SPREAD;
		effectGiveAuxVar(90+rand()%20);
		addEffect(&iVecEffect,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,FALSE,NULL,0);
		droidAddWeldSound( iVecEffect );
	}

	/* if not finished repair return TRUE else complete repair and return FALSE */
	if (psDroidToRepair->body < psDroidToRepair->originalBody)
	{
		return TRUE;
	}
	else
	{
        //reset the power accrued
        psDroidToRepair->powerAccrued = 0;
		psDroidToRepair->body = psDroidToRepair->originalBody;
		return FALSE;
	}
}

/* load the Droid stats for the components from the Access database */
BOOL loadDroidTemplates(char *pDroidData, UDWORD bufferSize)
{
	char				*pStartDroidData;
        int cnt;
	UDWORD				NumDroids = 0, i, player;
	STRING				componentName[MAX_NAME_SIZE];
#ifdef HASH_NAMES
	STRING droidName[MAX_NAME_SIZE];
#endif
	BOOL				found = FALSE; //,EndOfFile;
	DROID_TEMPLATE		*pDroidDesign;
	COMP_BASE_STATS		*pStats;
	UDWORD				size, inc, templateID;
	BOOL				bDefaultTemplateFound = FALSE;
#ifdef STORE_RESOURCE_ID
//	STRING				*pDroidName = droidName;
#endif
#ifdef RESOURCE_NAMES
	UDWORD				id;
#endif
#ifdef HASH_NAMES
	UDWORD				HashedName;
#else
	UDWORD				id;
#endif


	/* init default template */
	memset( &sDefaultDesignTemplate, 0, sizeof(DROID_TEMPLATE) );

	pStartDroidData = pDroidData;

	NumDroids = numCR(pDroidData, bufferSize);

	for (i=0; i < NumDroids; i++)
	{
		if (!HEAP_ALLOC(psTemplateHeap, (void*) &pDroidDesign))
		{
			debug( LOG_ERROR, "Out of memory - Droid Templates" );
			abort();
			return FALSE;
		}
		memset(pDroidDesign, 0, sizeof(DROID_TEMPLATE));

#ifndef HASH_NAMES
		//pDroidDesign->pName = pDroidDesign->aName;
		//only fill in aName now
		pDroidDesign->pName = NULL;
#endif

		//read the data into the storage - the data is delimited using comma's
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%d,%n", componentName, &templateID, &cnt);
                pDroidData += cnt;

// Hideous mishmash of ifdef's ... sorry about that
#ifdef HASH_NAMES
		HashedName=HashString(componentName);
		pDroidDesign->NameHash=HashedName;
		strcpy(droidName,strresGetString(NULL,HashedName));
#else

	//We ain't EVER going back to the way it was so..just store the long (translated) name in aName
/*#ifdef RESOURCE_NAMES
		//get the name associated with the resource
		if (!strresGetIDNum(psStringRes, componentName, &id))
		{
			DBERROR(("Unable to find string resource for %s", componentName));
			return FALSE;
		}

		//get the string from the id and copy into the MALLOC'ed area
		strcpy(droidName,strresGetString(psStringRes, id));

#elif defined STORE_RESOURCE_ID
		//find the pointer where the string ID is stored
		if (!strresGetIDString(psStringRes, componentName, &pDroidName))
		{
			DBERROR(("Unable to find string resource for %s", componentName));
			return FALSE;
		}
		strcpy(droidName,pDroidName);
#else
		strcpy(droidName,componentName);
#endif
		//get the string from the id and copy into the MALLOC'ed area
		strcpy(droidName,strresGetString(psStringRes, id));

		//allocate storage for the name - constant array now (aName)
		strncpy(pDroidDesign->pName,droidName, DROID_MAXNAME);
		pDroidDesign->pName[DROID_MAXNAME-1]=0;*/


		/*ALWAYS get the name associated with the resource for PC regardless
		of STORE_RESOURCE_ID or RESOURCE_NAMES! - 25/06/98 AB*/
		if (!strresGetIDNum(psStringRes, componentName, &id))
		{
			debug( LOG_ERROR, "Unable to find string resource for %s", componentName );
			abort();
			return FALSE;
		}

		//get the string from the id and copy into the Name space
		strcpy(pDroidDesign->aName,strresGetString(psStringRes, id));
		pDroidDesign->aName[DROID_MAXNAME-1] = 0;
#endif

		//store the unique template id - NOT ON PSX

		pDroidDesign->multiPlayerID = templateID;


		//read in Body Name
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;

		found = FALSE;
		//get the Body stats pointer
		if (!strcmp(componentName,"0"))
		{
			pDroidDesign->asParts[COMP_BODY] = NULL_COMP;
		}
		else
		{
			pStats = (COMP_BASE_STATS *)asBodyStats;
			size = sizeof(BODY_STATS);
			if (!getResourceName(componentName))
			{
				return FALSE;
			}

#ifdef HASH_NAMES
			HashedName=HashString(componentName);
#endif
			for (inc=0; inc < numBodyStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pStats->NameHash==HashedName)
#else
				if (!strcmp(componentName, pStats->pName))
#endif
				{
					pDroidDesign->asParts[COMP_BODY] = inc;
					found = TRUE;
					break;
				}
				pStats = ((COMP_BASE_STATS*)((UBYTE*)pStats + size));
			}
			if (!found)
			{
				debug( LOG_ERROR, "Body component not found for droid %s", getTemplateName(pDroidDesign) );
				abort();
				return FALSE;
			}
		}

		//read in Brain Name
		found = FALSE;
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;

		//get the Brain stats pointer
		if (!strcmp(componentName,"0"))
		{
			pDroidDesign->asParts[COMP_BRAIN] = NULL_COMP;
		}
		else
		{
			pStats = (COMP_BASE_STATS *)asBrainStats;
			size = sizeof(BRAIN_STATS);
			if (!getResourceName(componentName))
			{
				return FALSE;
			}
#ifdef HASH_NAMES
			HashedName=HashString(componentName);
#endif
			for (inc=0; inc < numBrainStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pStats->NameHash==HashedName)
#else
				if (!strcmp(componentName, pStats->pName))
#endif
				{
					pDroidDesign->asParts[COMP_BRAIN] = inc;
					found = TRUE;
					break;
				}
				pStats = ((COMP_BASE_STATS*)((UBYTE*)pStats + size));
			}
			if (!found)
			{
				debug( LOG_ERROR, "Brain component not found for droid %s", getTemplateName(pDroidDesign) );
				abort();
//				DBERROR(("Brain component not found for droid %s", pDroidDesign->pName));
				return FALSE;
			}
		}

		//read in Construct Name
		found = FALSE;
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;

		//get the Construct stats pointer
		if (!strcmp(componentName,"0"))
		{
			pDroidDesign->asParts[COMP_CONSTRUCT] = NULL_COMP;
		}
		else
		{
			pStats = (COMP_BASE_STATS *)asConstructStats;
			size = sizeof(CONSTRUCT_STATS);
			if (!getResourceName(componentName))
			{
				return FALSE;
			}
#ifdef HASH_NAMES
			HashedName=HashString(componentName);
#endif
			for (inc=0; inc < numConstructStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pStats->NameHash==HashedName)
#else
				if (!strcmp(componentName, pStats->pName))
#endif
				{
					pDroidDesign->asParts[COMP_CONSTRUCT] = inc;
					found = TRUE;
					break;
				}
				pStats = ((COMP_BASE_STATS*)((UBYTE*)pStats + size));
			}
			if (!found)
			{
				debug( LOG_ERROR, "Construct component not found for droid %s", getTemplateName(pDroidDesign) );
				abort();
				return FALSE;
			}
		}

		//read in Ecm Name
		found = FALSE;
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;

		//get the Ecm stats pointer
		if (!strcmp(componentName,"0"))
		{
			pDroidDesign->asParts[COMP_ECM] = NULL_COMP;
		}
		else
		{
			pStats = (COMP_BASE_STATS *)asECMStats;
			size = sizeof(ECM_STATS);
			if (!getResourceName(componentName))
			{
				return FALSE;
			}
#ifdef HASH_NAMES
			HashedName=HashString(componentName);
#endif
			for (inc=0; inc < numECMStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pStats->NameHash==HashedName)
#else
				if (!strcmp(componentName, pStats->pName))
#endif
				{
					pDroidDesign->asParts[COMP_ECM] = inc;
					found = TRUE;
					break;
				}
				pStats = ((COMP_BASE_STATS*)((UBYTE*)pStats + size));
			}
			if (!found)
			{
				debug( LOG_ERROR, "ECM component not found for droid %s", getTemplateName(pDroidDesign) );
				abort();
				return FALSE;
			}
		}

		//read in player id - Access decides the order -crap hey?
		sscanf(pDroidData, "%d,%n", &player,&cnt);
                pDroidData += cnt;




		//read in Propulsion Name
		found = FALSE;
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;

		//get the Propulsion stats pointer
		if (!strcmp(componentName,"0"))
		{
			pDroidDesign->asParts[COMP_PROPULSION] = NULL_COMP;
		}
		else
		{
			pStats = (COMP_BASE_STATS *)asPropulsionStats;
			size = sizeof(PROPULSION_STATS);
			if (!getResourceName(componentName))
			{
				return FALSE;
			}
#ifdef HASH_NAMES
			HashedName=HashString(componentName);
#endif
			for (inc=0; inc < numPropulsionStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pStats->NameHash==HashedName)
#else
				if (!strcmp(componentName, pStats->pName))
#endif
				{
					pDroidDesign->asParts[COMP_PROPULSION] = inc;
					found = TRUE;
					break;
				}
				pStats = ((COMP_BASE_STATS*)((UBYTE*)pStats + size));
			}
			if (!found)
			{
				debug( LOG_ERROR, "Propulsion component not found for droid %s", getTemplateName(pDroidDesign) );
				abort();
				return FALSE;
			}
		}

		//read in Repair Name
		found = FALSE;
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;

		//get the Repair stats pointer
		if (!strcmp(componentName,"0"))
		{
			pDroidDesign->asParts[COMP_REPAIRUNIT] = NULL_COMP;
		}
		else
		{
			pStats = (COMP_BASE_STATS *)asRepairStats;
			size = sizeof(REPAIR_STATS);
			if (!getResourceName(componentName))
			{
				return FALSE;
			}
#ifdef HASH_NAMES
			HashedName=HashString(componentName);
#endif
			for (inc=0; inc < numRepairStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pStats->NameHash==HashedName)
#else
				if (!strcmp(componentName, pStats->pName))
#endif
				{
					pDroidDesign->asParts[COMP_REPAIRUNIT] = inc;
					found = TRUE;
					break;
				}
				pStats = ((COMP_BASE_STATS*)((UBYTE*)pStats + size));
			}
			if (!found)
			{
				debug( LOG_ERROR, "Repair component not found for droid %s", getTemplateName(pDroidDesign) );
				abort();
				return FALSE;
			}
		}

		//read in droid type - only interested if set to PERSON
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;
		if (!strcmp(componentName, "PERSON"))
		{
			pDroidDesign->droidType = DROID_PERSON;
		}
		if (!strcmp(componentName, "CYBORG"))
		{
			pDroidDesign->droidType = DROID_CYBORG;
		}
		if (!strcmp(componentName, "CYBORG_SUPER"))
		{
			pDroidDesign->droidType = DROID_CYBORG_SUPER;
		}
		if (!strcmp(componentName, "CYBORG_CONSTRUCT"))
		{
			pDroidDesign->droidType = DROID_CYBORG_CONSTRUCT;
		}
		if (!strcmp(componentName, "CYBORG_REPAIR"))
		{
			pDroidDesign->droidType = DROID_CYBORG_REPAIR;
		}
		if (!strcmp(componentName, "TRANSPORTER"))
		{
			pDroidDesign->droidType = DROID_TRANSPORTER;
		}
		if (!strcmp(componentName, "ZNULLDROID"))
		{
			pDroidDesign->droidType = DROID_DEFAULT;
			bDefaultTemplateFound = TRUE;
		}
		//pDroidData += (strlen(componentName)+1);

		//read in Sensor Name
		found = FALSE;
		componentName[0] = '\0';
		sscanf(pDroidData, "%[^','],%n", componentName, &cnt);
                pDroidData += cnt;

		//get the Sensor stats pointer
		if (!strcmp(componentName,"0"))
		{
			pDroidDesign->asParts[COMP_SENSOR] = NULL_COMP;
		}
		else
		{
			pStats = (COMP_BASE_STATS *)asSensorStats;
			size = sizeof(SENSOR_STATS);
			if (!getResourceName(componentName))
			{
				return FALSE;
			}

#ifdef HASH_NAMES
			HashedName=HashString(componentName);
#endif

			for (inc=0; inc < numSensorStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pStats->NameHash==HashedName)
#else
				if (!strcmp(componentName, pStats->pName))
#endif
				{
					pDroidDesign->asParts[COMP_SENSOR] = inc;
					found = TRUE;
					break;
				}
				pStats = ((COMP_BASE_STATS*)((UBYTE*)pStats + size));
			}
			if (!found)
			{
#ifdef HASH_NAMES
				debug( LOG_ERROR, "Sensor not found for droid Template: %s", strresGetString( NULL, pDroidDesign->NameHash ) );
#else
				debug( LOG_ERROR, "Sensor not found for droid Template: %s", pDroidDesign->aName );
#endif
				abort();
				return FALSE;
			}
		}

		//read in totals
		/*sscanf(pDroidData,"%d,%d",
			&pDroidDesign->numProgs, &pDroidDesign->numWeaps);*/
		sscanf(pDroidData,"%d", &pDroidDesign->numWeaps);
		//check that not allocating more weapons than allowed
		if (((asBodyStats + pDroidDesign->asParts[COMP_BODY])->weaponSlots <
			 pDroidDesign->numWeaps) ||
			pDroidDesign->numWeaps > DROID_MAXWEAPS)
		{
#ifdef HASH_NAMES
			debug( LOG_ERROR, "Too many weapons have been allocated for droid Template: %s (%x)", strresGetString( NULL, pDroidDesign->NameHash ), pDroidDesign->NameHash );
#else
			debug( LOG_ERROR, "Too many weapons have been allocated for droid Template: %s", pDroidDesign->aName );
#endif
			abort();

			return FALSE;
		}
		//check that not allocating more programs than allowed
		/*if (((asBrainStats + pDroidDesign->asParts[COMP_BRAIN])->progCap <
			 pDroidDesign->numProgs) ||
			pDroidDesign->numProgs > DROID_MAXPROGS)
		{
#ifdef HASH_NAMES
			DBERROR(("Too many programs have been allocated for droid Template: %s",strresGetString(NULL,pDroidDesign->NameHash)));
#else
			DBERROR(("Too many programs have been allocated for droid Template: %s",getName(pDroidDesign->pName)));
#endif
			return FALSE;
		}*/


		pDroidDesign->ref = REF_TEMPLATE_START + i;
/*	Loaded in from the database now AB 29/10/98
#ifndef PSX
			pDroidDesign->multiPlayerID = i;			// another unique number, just for multiplayer stuff.
#endif
*/
		/* store global default design if found else
		 * store in the appropriate array
		 */
		if ( pDroidDesign->droidType == DROID_DEFAULT )
		{
			memcpy( &sDefaultDesignTemplate, pDroidDesign, sizeof(DROID_TEMPLATE) );
			HEAP_FREE(psTemplateHeap, pDroidDesign);
		}
		else
		{
			pDroidDesign->psNext = apsDroidTemplates[player];
			apsDroidTemplates[player] = pDroidDesign;
		}

		//increment the pointer to the start of the next record
		pDroidData = strchr(pDroidData,'\n') + 1;
		pDroidDesign++;
	}
//	FREE(pStartDroidData);



	if ( bDefaultTemplateFound == FALSE )
	{
		debug( LOG_ERROR, "loadUnitTemplates: default template not found\n" );
		abort();
		return FALSE;
	}

	return TRUE;
}

/*initialise the template build and power points */
void initTemplatePoints(void)
{
	UDWORD			player;
	DROID_TEMPLATE	*pDroidDesign;

	for (player=0; player < MAX_PLAYERS; player++)
	{
		for(pDroidDesign = apsDroidTemplates[player]; pDroidDesign != NULL;
			pDroidDesign = pDroidDesign->psNext)
		{
			//calculate the total build points
			pDroidDesign->buildPoints = calcTemplateBuild(pDroidDesign);
			//calc the total power points
			pDroidDesign->powerPoints = calcTemplatePower(pDroidDesign);
		}
	}
}


// return whether a droid is IDF
BOOL idfDroid(DROID *psDroid)
{
    //add Cyborgs
	//if (psDroid->droidType != DROID_WEAPON)
    if (!(psDroid->droidType == DROID_WEAPON || psDroid->droidType == DROID_CYBORG ||
        psDroid->droidType == DROID_CYBORG_SUPER))
	{
		return FALSE;
	}

	if (proj_Direct(psDroid->asWeaps[0].nStat + asWeaponStats))
	{
		return FALSE;
	}

	return TRUE;
}

// return whether a template is for an IDF droid
BOOL templateIsIDF(DROID_TEMPLATE *psTemplate)
{
    //add Cyborgs
	//if (psTemplate->droidType != DROID_WEAPON)
    if (!(psTemplate->droidType == DROID_WEAPON || psTemplate->droidType == DROID_CYBORG ||
        psTemplate->droidType == DROID_CYBORG_SUPER))
	{
		return FALSE;
	}

	if (proj_Direct(psTemplate->asWeaps[0] + asWeaponStats))
	{
		return FALSE;
	}

	return TRUE;
}

/* Return the type of a droid */
DROID_TYPE droidType(DROID *psDroid)
{
/*	DROID_TYPE	type;

	if ((asSensorStats + psDroid->asBits[COMP_SENSOR].nStat)->location == LOC_TURRET)
	{
		type = DROID_SENSOR;
	}
	else if ((asECMStats + psDroid->asBits[COMP_ECM].nStat)->location == LOC_TURRET)
	{
		type = DROID_ECM;
	}
	else if (psDroid->asBits[COMP_CONSTRUCT].nStat != 0)
	{
		type = DROID_CONSTRUCT;
	}
	else
	{
		type = DROID_WEAPON;
	}*/

	return psDroid->droidType;
}


/* Return the type of a droid from it's template */
DROID_TYPE droidTemplateType(DROID_TEMPLATE *psTemplate)
{
	DROID_TYPE	type;

//	if (strcmp(psTemplate->pName, "BaBa People") == 0)
//	{
//		type = DROID_PERSON;
//	}
	if (psTemplate->droidType == DROID_PERSON)
	{
		type = DROID_PERSON;
	}
	else if (psTemplate->droidType == DROID_CYBORG)
	{
		type = DROID_CYBORG;
	}
	else if (psTemplate->droidType == DROID_CYBORG_SUPER)
	{
		type = DROID_CYBORG_SUPER;
	}
	else if (psTemplate->droidType == DROID_CYBORG_CONSTRUCT)
	{
		type = DROID_CYBORG_CONSTRUCT;
	}
	else if (psTemplate->droidType == DROID_CYBORG_REPAIR)
	{
		type = DROID_CYBORG_REPAIR;
	}
	else if (psTemplate->droidType == DROID_TRANSPORTER)
	{
		type = DROID_TRANSPORTER;
	}
	else if (psTemplate->asParts[COMP_BRAIN] != 0)
	{
		type = DROID_COMMAND;
	}
	else if ((asSensorStats + psTemplate->asParts[COMP_SENSOR])->location == LOC_TURRET)
	{
		type = DROID_SENSOR;
	}
	else if ((asECMStats + psTemplate->asParts[COMP_ECM])->location == LOC_TURRET)
	{
		type = DROID_ECM;
	}
	else if (psTemplate->asParts[COMP_CONSTRUCT] != 0)
	{
		type = DROID_CONSTRUCT;
	}
	//else if (psTemplate->asParts[COMP_REPAIRUNIT] != 0)
	else if ((asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->location == LOC_TURRET)
	{
		type = DROID_REPAIR;
	}
	else if ( psTemplate->asWeaps[0] != 0 )
	{
		type = DROID_WEAPON;
	}
	else
	{
		type = DROID_DEFAULT;
	}

	return type;
}

//Load the weapons assigned to Droids in the Access database
BOOL loadDroidWeapons(char *pWeaponData, UDWORD bufferSize)
{
	char				*pStartWeaponData;
	UDWORD				NumWeapons = 0, i, player;
	STRING				WeaponName[MAX_NAME_SIZE], TemplateName[MAX_NAME_SIZE];
	DROID_TEMPLATE		*pTemplate;
	BOOL				recFound;
	UWORD				SkippedWeaponCount=0;
	SDWORD				incW;
#ifdef HASH_NAMES
	UDWORD				HashedName;
#endif
	//initialise the store count variable
	for (player=0; player < MAX_PLAYERS; player++)
	{
		for(pTemplate = apsDroidTemplates[player]; pTemplate != NULL;
			pTemplate = pTemplate->psNext)
		{
			//clear the storage flags
			pTemplate->storeCount = 0;
		}
	}

	pStartWeaponData = pWeaponData;

	NumWeapons = numCR(pWeaponData, bufferSize);

	for (i=0; i < NumWeapons; i++)
	{
		recFound = FALSE;
		//read the data into the storage - the data is delimeted using comma's
		TemplateName[0] = '\0';
		WeaponName[0] = '\0';
		sscanf(pWeaponData,"%[^','],%[^','],%d", TemplateName, WeaponName, &player);
		//loop through each droid to compare the name

		player=RemapPlayerNumber(player);	// for psx ...

		/*if (!getResourceName(TemplateName))
		{
			return FALSE;
		}*/

		if (!getDroidResourceName(TemplateName))
		{
			return FALSE;
		}

		if (!getResourceName(WeaponName))
		{
			return FALSE;
		}

#ifdef HASH_NAMES
		HashedName=HashString(TemplateName);
#endif

		if (player < MAX_PLAYERS)
		{
			for(pTemplate = apsDroidTemplates[player]; pTemplate != NULL; pTemplate =
				pTemplate->psNext)
			{
#ifdef HASH_NAMES
				if (pTemplate->NameHash==HashedName)
#else
				//if (!(strcmp(TemplateName, pTemplate->pName)))
				if (!(strcmp(TemplateName, pTemplate->aName)))
#endif
				{
					//Template found
					recFound = TRUE;
					break;
				}
			}
			/* if Template not found - try default design */
			if (!recFound)
			{
				pTemplate = &sDefaultDesignTemplate;
#ifdef HASH_NAMES
				if (pTemplate->NameHash!=HashedName)
#else
				//if ( strcmp(TemplateName, pTemplate->pName) )
				if ( strcmp(TemplateName, pTemplate->aName) )
#endif
				{
					debug( LOG_ERROR, "Unable to find Template - %s", TemplateName );
					abort();
					return FALSE;
				}
			}

			incW = getCompFromName(COMP_WEAPON, WeaponName);
			//if weapon not found - error
			if (incW == -1)
			{
				debug( LOG_ERROR, "Unable to find Weapon %s for template %s", WeaponName, TemplateName );
				abort();
				return FALSE;
			}
			else
			{
				//Weapon found, alloc this to the current Template
				pTemplate->asWeaps[pTemplate->storeCount] = incW;
				//check not allocating more than allowed
				if (pTemplate->storeCount >
								(SDWORD)pTemplate->numWeaps)
				{
					debug( LOG_ERROR, "Trying to allocate more weapons than allowed for Template - %s", TemplateName );
					abort();
					return FALSE;
				}
                //check valid weapon/propulsion
                if (!checkValidWeaponForProp(pTemplate))
                {
			// ffs
					debug( LOG_ERROR, "Weapon is invalid for air propulsion for template %s", pTemplate->aName );
					abort();

					return FALSE;
                }

				pTemplate->storeCount++;
			}
		}
		else
		{
			SkippedWeaponCount++;
		}

		//increment the pointer to the start of the next record
		pWeaponData = strchr(pWeaponData,'\n') + 1;
	}

	if (SkippedWeaponCount > 0)
	{

		debug( LOG_ERROR, "Illegal player number in %d droid weapons", SkippedWeaponCount );
		abort();

	}

//	FREE(pStartWeaponData);
	return TRUE;
}

//Load the programs assigned to Droids in the Access database
/*BOOL loadDroidPrograms(char *pProgramData, UDWORD bufferSize)
{
	char				*pStartProgramData;
	UDWORD				NumPrograms = 0, i, incP, player;
	STRING				ProgramName[MAX_NAME_SIZE], TemplateName[MAX_NAME_SIZE];
	DROID_TEMPLATE		*pTemplate;
	PROGRAM_STATS		*pPrograms = asProgramStats;
	BOOL				recFound;
	UWORD				SkippedProgramCount=0;

#ifdef HASH_NAMES
	UDWORD				HashedTemplateName,HashedProgramName;
#endif

	//initialise the store count variable
	for (player=0; player < MAX_PLAYERS; player++)
	{
		for(pTemplate = apsDroidTemplates[player]; pTemplate != NULL;
			pTemplate = pTemplate->psNext)
		{
			//clear the storage flags
			pTemplate->storeCount = 0;
		}
	}

	pStartProgramData = pProgramData;

	NumPrograms = numCR(pProgramData, bufferSize);

	for (i=0; i < NumPrograms; i++)
	{
		recFound = FALSE;
		//read the data into the storage - the data is delimeted using comma's
		TemplateName[0] = '\0';
		ProgramName[0] = '\0';
		sscanf(pProgramData,"%[^','],%[^','],%d", &TemplateName, &ProgramName, &player);

		//if (!getResourceName(TemplateName))
		//{
		//	return FALSE;
		//}
#ifndef PSX
		if (!getDroidResourceName(TemplateName))
		{
			return FALSE;
		}
#endif
		if (!getResourceName(ProgramName))
		{
			return FALSE;
		}
#ifdef HASH_NAMES
		HashedTemplateName=HashString(TemplateName);
		HashedProgramName=HashString(ProgramName);
#endif

		if (player < MAX_PLAYERS)
		{
			//loop through each droid to compare the name
			for(pTemplate = apsDroidTemplates[player]; pTemplate != NULL; pTemplate =
				pTemplate->psNext)
			{

#ifdef HASH_NAMES
				if (pTemplate->NameHash==HashedTemplateName)
#else
				//if (!(strcmp(TemplateName, pTemplate->pName)))
				if (!(strcmp(TemplateName, pTemplate->aName)))
#endif
				{

					//Template found
					for (incP=0; incP < numProgramStats; incP++)
					{
#ifdef HASH_NAMES
						if (pPrograms[incP].NameHash==HashedProgramName)
#else
						if (!(strcmp(ProgramName, pPrograms[incP].pName)))
#endif
						{
							//Program found, alloc this to the current Template
							pTemplate->asProgs[pTemplate->storeCount] = incP;
							recFound = TRUE;
							//check not allocating more than allowed
							if (pTemplate->storeCount > (SDWORD)pTemplate->numProgs)
							{
								DBERROR(("Trying to allocate more programs than allowed for Template"));
								return FALSE;
							}
							pTemplate->storeCount++;
							break;
						}
					}
					//if program not found - error
					if (!recFound)
					{
						DBERROR(("Unable to find Program"));
						return FALSE;
					}
					else
					{
						break;
					}
				}
			}

			//if Template not found - error
			if (!recFound)
			{
				DBERROR(("Unable to allocate all Template programs"));
				return FALSE;
			}

		}
		else
		{
			SkippedProgramCount++;
		}

		//increment the pointer to the start of the next record
		pProgramData = strchr(pProgramData,'\n') + 1;
	}

	if (SkippedProgramCount>0)
	{
		DBERROR(("Illegal player number in %d droid programs",SkippedProgramCount));
	}

//	FREE(pStartProgramData);
	return TRUE;
}*/

//free the storage for the droid templates
BOOL droidTemplateShutDown()
{
	DROID_TEMPLATE	*pTemplate, *pNext;
	UDWORD			player;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(pTemplate = apsDroidTemplates[player]; pTemplate != NULL;
			pTemplate = pNext)
		{
			pNext = pTemplate->psNext;
			HEAP_FREE(psTemplateHeap, pTemplate);
		}
		apsDroidTemplates[player] = NULL;
	}
	return TRUE;
}

/* Calculate the weight of a droid from it's template */
UDWORD calcDroidWeight(DROID_TEMPLATE *psTemplate)
{
	UDWORD weight, i;

	/* Get the basic component weight */
	weight =
		(asBodyStats + psTemplate->asParts[COMP_BODY])->weight +
		(asBrainStats + psTemplate->asParts[COMP_BRAIN])->weight +
		//(asPowerStats + psTemplate->asParts[COMP_POWERPLANT])->weight +
		//(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->weight +
		(asSensorStats + psTemplate->asParts[COMP_SENSOR])->weight +
		(asECMStats + psTemplate->asParts[COMP_ECM])->weight +
		//(asArmourStats + psTemplate->asParts[COMP_ARMOUR])->weight +
		(asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->weight +
		(asConstructStats + psTemplate->asParts[COMP_CONSTRUCT])->weight;

	/* propulsion weight is a percentage of the body weight */
	weight += (((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->weight *
		(asBodyStats + psTemplate->asParts[COMP_BODY])->weight) / 100);

	/* Add the weapon weight */
	for(i=0; i<psTemplate->numWeaps; i++)
	{
		weight += (asWeaponStats + psTemplate->asWeaps[i])->weight;
	}

	return weight;
}

/* Calculate the body points of a droid from it's template */
UDWORD calcTemplateBody(DROID_TEMPLATE *psTemplate, UBYTE player)
{
	UDWORD body, i;

	if (psTemplate == NULL)
	{
		return 0;
	}
	/* Get the basic component body points */
	body =
		(asBodyStats + psTemplate->asParts[COMP_BODY])->body +
		(asBrainStats + psTemplate->asParts[COMP_BRAIN])->body +
		//(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->body +
		(asSensorStats + psTemplate->asParts[COMP_SENSOR])->body +
		(asECMStats + psTemplate->asParts[COMP_ECM])->body +
		(asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->body +
		(asConstructStats + psTemplate->asParts[COMP_CONSTRUCT])->body;

	/* propulsion body points are a percentage of the bodys' body points */
	body += (((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->body *
		(asBodyStats + psTemplate->asParts[COMP_BODY])->body) / 100);

	/* Add the weapon body points */
	for(i=0; i<psTemplate->numWeaps; i++)
	{
		body += (asWeaponStats + psTemplate->asWeaps[i])->body;
	}

	//add on any upgrade value that may need to be applied
	body += (body * asBodyUpgrade[player]->body / 100);
	return body;
}

/* Calculate the base body points of a droid without upgrades*/
UDWORD calcDroidBaseBody(DROID *psDroid)
{
	UDWORD      body; //, i;

	if (psDroid == NULL)
	{
		return 0;
	}
	/* Get the basic component body points */
	body =
		(asBodyStats + psDroid->asBits[COMP_BODY].nStat)->body +
		(asBrainStats + psDroid->asBits[COMP_BRAIN].nStat)->body +
		(asSensorStats + psDroid->asBits[COMP_SENSOR].nStat)->body +
		(asECMStats + psDroid->asBits[COMP_ECM].nStat)->body +
		(asRepairStats + psDroid->asBits[COMP_REPAIRUNIT].nStat)->body +
		(asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat)->body;

	/* propulsion body points are a percentage of the bodys' body points */
	body += (((asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat)->body *
		(asBodyStats + psDroid->asBits[COMP_BODY].nStat)->body) / 100);

	/* Add the weapon body points */
	//for(i=0; i<psDroid->numWeaps; i++)
    if (psDroid->asWeaps[0].nStat > 0)
	{
		//body += (asWeaponStats + psDroid->asWeaps[i].nStat)->body;
        body += (asWeaponStats + psDroid->asWeaps[0].nStat)->body;
	}

	return body;
}


/* Calculate the base speed of a droid from it's template */
UDWORD calcDroidBaseSpeed(DROID_TEMPLATE *psTemplate, UDWORD weight, UBYTE player)
{
	UDWORD	speed;

	//return ((asBodyStats + psTemplate->asParts[COMP_BODY])->
	//	powerOutput * 100) / weight;
	/*if (psTemplate->droidType == DROID_CYBORG)
	{
		return (bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY],
			player, CYBORG_BODY_UPGRADE) * 100) / weight;
	}
	else
	{
		return (bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY],
			player, DROID_BODY_UPGRADE) * 100) / weight;
	}*/
	if (psTemplate->droidType == DROID_CYBORG OR
		psTemplate->droidType == DROID_CYBORG_SUPER OR
        psTemplate->droidType == DROID_CYBORG_CONSTRUCT OR
        psTemplate->droidType == DROID_CYBORG_REPAIR)
	{
		speed = (asPropulsionTypes[(asPropulsionStats + psTemplate->
            asParts[COMP_PROPULSION])->propulsionType].powerRatioMult *
            bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY],
            player, CYBORG_BODY_UPGRADE)) / weight;
	}
	else
	{
		speed = (asPropulsionTypes[(asPropulsionStats + psTemplate->
            asParts[COMP_PROPULSION])->propulsionType].powerRatioMult *
            bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY],
			player, DROID_BODY_UPGRADE)) / weight;
	}

	// reduce the speed of medium/heavy VTOLs
	if (asPropulsionStats[psTemplate->asParts[COMP_PROPULSION]].propulsionType == LIFT)
	{
		if ((asBodyStats + psTemplate->asParts[COMP_BODY])->size == SIZE_HEAVY)
		{
			speed /= 4;
		}
		else if ((asBodyStats + psTemplate->asParts[COMP_BODY])->size == SIZE_MEDIUM)
		{
			speed = 3 * speed / 4;
		}
	}

	return speed;
}


/* Calculate the speed of a droid over a terrain */
UDWORD calcDroidSpeed(UDWORD baseSpeed, UDWORD terrainType, UDWORD propIndex)
{
    UDWORD  droidSpeed;
//	return baseSpeed * getSpeedFactor(terrainType,
//		(asPropulsionStats + propIndex)->propulsionType) / 100;

    //need to ensure doesn't go over the max speed possible for this propulsion
    droidSpeed = baseSpeed * getSpeedFactor(terrainType,
		(asPropulsionStats + propIndex)->propulsionType) / 100;
    if (droidSpeed > (asPropulsionStats + propIndex)->maxSpeed)
    {
        droidSpeed = (asPropulsionStats + propIndex)->maxSpeed;
    }

    return droidSpeed;
}

/* Calculate the points required to build the template - used to calculate time*/
UDWORD calcTemplateBuild(DROID_TEMPLATE *psTemplate)
{
	UDWORD	build, i;

	build = (asBodyStats + psTemplate->asParts[COMP_BODY])->buildPoints +
	(asBrainStats + psTemplate->asParts[COMP_BRAIN])->buildPoints +
	//(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->buildPoints +
	(asSensorStats + psTemplate->asParts[COMP_SENSOR])->buildPoints +
	(asECMStats + psTemplate->asParts[COMP_ECM])->buildPoints +
	(asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->buildPoints +
	(asConstructStats + psTemplate->asParts[COMP_CONSTRUCT])->buildPoints;

	/* propulsion build points are a percentage of the bodys' build points */
	build += (((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->buildPoints *
		(asBodyStats + psTemplate->asParts[COMP_BODY])->buildPoints) / 100);

	//add weapon power
	for(i=0; i<psTemplate->numWeaps; i++)
	{
		ASSERT( psTemplate->asWeaps[i]<numWeaponStats,
			//"Invalid Template weapon for %s", psTemplate->pName );
			"Invalid Template weapon for %s", getTemplateName(psTemplate) );
		build += (asWeaponStats + psTemplate->asWeaps[i])->buildPoints;
	}

	//add program power
	/*for(i=0; i<psTemplate->numProgs; i++)
	{
		ASSERT( psTemplate->asProgs[i]<numProgramStats,
			//"Invalid Template program for %s", psTemplate->pName );
			"Invalid Template program for %s", getTemplateName(psTemplate)));
		build += (asProgramStats + psTemplate->asProgs[i])->buildPoints;
	}*/

	return build;
}

/* Calculate the points required to build the droid */
/*UDWORD calcDroidBuild(DROID *psDroid)
{
	UDWORD	build, i;

	build = (asBodyStats + psDroid->asBits[COMP_BODY].nStat)->buildPoints +
	(asBrainStats + psDroid->asBits[COMP_BRAIN].nStat)->buildPoints +
	(asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat)->buildPoints +
	(asSensorStats + psDroid->asBits[COMP_SENSOR].nStat)->buildPoints +
	(asECMStats + psDroid->asBits[COMP_ECM].nStat)->buildPoints +
	(asRepairStats + psDroid->asBits[COMP_REPAIRUNIT].nStat)->buildPoints;

	//add weapon power
	for(i=0; i<psDroid->numWeaps; i++)
	{
		build += (asWeaponStats + psDroid->asWeaps[i].nStat)->buildPoints;
	}

	//add program power
	for(i=0; i<psDroid->numProgs; i++)
	{
		build += psDroid->asProgs[i].psStats->buildPoints;
	}

	return build;
}*/

/* Calculate the power points required to build/maintain a template */
UDWORD	calcTemplatePower(DROID_TEMPLATE *psTemplate)
{
	UDWORD power, i;

	//get the component power
	power = (asBodyStats + psTemplate->asParts[COMP_BODY])->buildPower +
	(asBrainStats + psTemplate->asParts[COMP_BRAIN])->buildPower +
	//(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->buildPower +
	(asSensorStats + psTemplate->asParts[COMP_SENSOR])->buildPower +
	(asECMStats + psTemplate->asParts[COMP_ECM])->buildPower +
	(asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->buildPower +
	(asConstructStats + psTemplate->asParts[COMP_CONSTRUCT])->buildPower;

	/* propulsion power points are a percentage of the bodys' power points */
	power += (((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->buildPower *
		(asBodyStats + psTemplate->asParts[COMP_BODY])->buildPower) / 100);

	//add weapon power
	for(i=0; i<psTemplate->numWeaps; i++)
	{
		power += (asWeaponStats + psTemplate->asWeaps[i])->buildPower;
	}

	//add program power
	/*for(i=0; i<psTemplate->numProgs; i++)
	{
		power += (asProgramStats + psTemplate->asProgs[i])->buildPower;
	}*/

	return power;
}


/* Calculate the power points required to build/maintain a droid */
UDWORD	calcDroidPower(DROID *psDroid)
{
	UDWORD      power;//, i;

	//get the component power
	power = (asBodyStats + psDroid->asBits[COMP_BODY].nStat)->buildPower +
	(asBrainStats + psDroid->asBits[COMP_BRAIN].nStat)->buildPower +
	//(asPropulsionStats + psDroid->asBits[COMP_PROPULSION])->buildPower +
	(asSensorStats + psDroid->asBits[COMP_SENSOR].nStat)->buildPower +
	(asECMStats + psDroid->asBits[COMP_ECM].nStat)->buildPower +
	(asRepairStats + psDroid->asBits[COMP_REPAIRUNIT].nStat)->buildPower +
	(asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat)->buildPower;

	/* propulsion power points are a percentage of the bodys' power points */
	power += (((asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat)->buildPower *
		(asBodyStats + psDroid->asBits[COMP_BODY].nStat)->buildPower) / 100);

	//add weapon power
	//for(i=0; i<psDroid->numWeaps; i++)
    if (psDroid->asWeaps[0].nStat > 0)
	{
		//power += (asWeaponStats + psDroid->asWeaps[i].nStat)->buildPower;
        power += (asWeaponStats + psDroid->asWeaps[0].nStat)->buildPower;
	}

	//add program power
/*	for(i=0; i<psDroid->numProgs; i++)
	{
		power += (asProgramStats + psDroid->asProgs[i])->buildPower;
	}*/

	return power;
}

//Builds an instance of a Droid - the x/y passed in are in world coords.
DROID* buildDroid(DROID_TEMPLATE *pTemplate, UDWORD x, UDWORD y, UDWORD player,
				  BOOL onMission)
{
	DROID			*psDroid;
	DROID_GROUP		*psGrp;
	//DROID_TEMPLATE	*psCurrTempl, *psPrevTempl;
	UDWORD			inc;
//	UDWORD			mapX, mapY;
	UDWORD			numKills;
	SDWORD			i, experienceLoc;
//	UDWORD			tileX,tileY;
//	BOOL			gotPos;
//	UDWORD			numIts;


	/*
	if(bMultiPlayer)
	{
		for(psDroid=apsDroidLists[selectedPlayer],i=0;
		psDroid; psDroid = psDroid->psNext,i++)
		{
		}
		if(i>=80)
		{
			if(player == selectedPlayer)
			{
				addConsoleMessage("Cannot build new unit - Command Control limit reached (DEMO)",LEFT_JUSTIFY);
				return(NULL);
			}
		}
	}
	*/


	//don't worry if not on homebase cos not being drawn yet
// this check no longer required coz John says so JPS 7Jan99
/*
	if (!onMission)
	{
		if ( TILE_OCCUPIED(mapTile(x >> TILE_SHIFT, y >> TILE_SHIFT)) )
		{
			DBPRINTF(("droid build: tile occupied\n"));
			ASSERT( FALSE,"Can't build a droid cos there's somthing here" );
			return NULL;
		}
	}
*/

	//allocate memory
	if (!createDroid(player, &psDroid))
	{
		debug( LOG_NEVER, "unit build: unable to create\n" );
		ASSERT( FALSE,"Cannot get the memory for the unit" );
		return NULL;
	}


	//THIS IS DONE BEFORE STARTING TO BUILD NOW IN STRUCTURE.C AB 14/05/98
	/*subtract the power required to build*/
	/*if (!usePower(player, pTemplate->powerPoints))
	{
//		addGameMessage("droid build: no power", 1000, TRUE);
		// Added next line so that we don't see this message for other players
		if(player == selectedPlayer)
		{
			addConsoleMessage("Droid build: No Power",DEFAULT_JUSTIFY);
		}
		HEAP_FREE(psDroidHeap, psDroid);
		return NULL;
	}*/

	//fill in other details
	//psDroid->pTemplate = pTemplate;

#ifdef HASH_NAMES
	psDroid->HashedDroidName=pTemplate->NameHash;
	droidSetName(psDroid,strresGetString(NULL,pTemplate->NameHash));
#else
	//droidSetName(psDroid,pTemplate->pName);
	droidSetName(psDroid,pTemplate->aName);
#endif
	// Set the droids type
	psDroid->droidType = droidTemplateType(pTemplate);

//legacy code
//	psDroid->x = (UWORD)((x & (~TILE_MASK)) + TILE_UNITS/2);
//	psDroid->y = (UWORD)((y & (~TILE_MASK)) + TILE_UNITS/2);
	psDroid->x = (UWORD)x;
	psDroid->y = (UWORD)y;

	//don't worry if not on homebase cos not being drawn yet
	if (!onMission)
	{
//		mapX = psDroid->x >> TILE_SHIFT;
//		mapY = psDroid->y >> TILE_SHIFT;
//		psDroid->lastTile = mapTile(mapX,mapY);
		//set droid height
		psDroid->z = map_Height(psDroid->x, psDroid->y);
	}
//	psDroid->x = (UWORD)((x & (~TILE_MASK)) + TILE_UNITS/2);
//	psDroid->y = (UWORD)((y & (~TILE_MASK)) + TILE_UNITS/2);
//	psDroid->x = (UWORD)x;
//	psDroid->y = (UWORD)y;
//	DBPRINTF(("new droid = %p height=%d\n",psDroid,psDroid->z);

	psDroid->cluster = 0;
	psDroid->psGroup = NULL;
	psDroid->psGrpNext = NULL;
	if ( (psDroid->droidType == DROID_TRANSPORTER) ||
		 (psDroid->droidType == DROID_COMMAND) )
	{
		if (!grpCreate(&psGrp))
		{
			debug( LOG_NEVER, "unit build: unable to create group\n" );
			ASSERT( FALSE,"Can't create unit because can't create group" );
			HEAP_FREE(psDroidHeap, psDroid);
			return NULL;
		}
		grpJoin(psGrp, psDroid);
	}

	psDroid->order = DORDER_NONE;
	psDroid->orderX = 0;
	psDroid->orderY = 0;
	psDroid->orderX2 = 0;
	psDroid->orderY2 = 0;
	psDroid->psTarget = NULL;
	psDroid->psTarStats = NULL;
	psDroid->secondaryOrder = DSS_ARANGE_DEFAULT | DSS_REPLEV_NEVER | DSS_ALEV_ALWAYS | DSS_HALT_GUARD;
	psDroid->action = DACTION_NONE;
	psDroid->actionX = 0;
	psDroid->actionY = 0;
	psDroid->psActionTarget = NULL;

		// ffs je
	psDroid->listSize = 0;
	memset(psDroid->asOrderList, 0, sizeof(ORDER_LIST)*ORDER_LIST_MAX);

	psDroid->iAudioID = NO_SOUND;
	psDroid->lastSync = 0;


//	psDroid->activeWeapon = -1;
	//psDroid->activeProg = -1;
	psDroid->psCurAnim = NULL;
	psDroid->group = UBYTE_MAX;
    psDroid->psBaseStruct = NULL;


	// find the highest stored experience
	if ((psDroid->droidType != DROID_CONSTRUCT) &&
        (psDroid->droidType != DROID_CYBORG_CONSTRUCT) &&
		(psDroid->droidType != DROID_REPAIR) &&
        (psDroid->droidType != DROID_CYBORG_REPAIR) &&
		(psDroid->droidType != DROID_TRANSPORTER))
	{
		numKills = 0;
		experienceLoc = 0;
		for(i=0; i<MAX_RECYCLED_DROIDS; i++)
		{
			if (aDroidExperience[player][i] > numKills)
			{
				numKills = aDroidExperience[player][i];
				experienceLoc = i;
			}
		}
		aDroidExperience[player][experienceLoc] = 0;
		psDroid->numKills = (UWORD)numKills;
	}
	else
	{
		psDroid->numKills = 0;
	}

//	//create the droids weapons	- done in droidSetBits()

	//create the droids programs
	/*psDroid->numProgs = pTemplate->numProgs;
	if (pTemplate->numProgs > 0)
	{
		for (inc=0; inc < pTemplate->numProgs; inc++)
		{
			psDroid->asProgs[inc].psStats = asProgramStats + pTemplate->asProgs[inc];
		}
		//set the first program to be the current one
		psDroid->activeProg = 0;
	}*/

	droidSetBits(pTemplate,psDroid);

	//calculate the droids total weight
	psDroid->weight = calcDroidWeight(pTemplate);

	// Initialise the movement stuff
	psDroid->baseSpeed = calcDroidBaseSpeed(pTemplate, psDroid->weight, (UBYTE)player);

	initDroidMovement(psDroid);
	/*psDroid->sMove.fx = MAKEFRACT(psDroid->x);
	psDroid->sMove.fy = MAKEFRACT(psDroid->y);
//	psDroid->sMove.Speed = 0;
	psDroid->sMove.speed = MAKEFRACT(0);
	psDroid->sMove.dir = 0;
	psDroid->sMove.bumpDir = 0;
	psDroid->sMove.speed = MAKEFRACT(0);
	psDroid->sMove.Status = MOVEINACTIVE;
//	psDroid->sMove.speedChange = FALSE;
//	psDroid->sMove.lastTime=0;
	psDroid->sMove.Direction=0;
	psDroid->sMove.Direction3D=0;
	psDroid->sMove.psFormation = NULL;*/

	psDroid->direction=0;
	psDroid->pitch =  0;
	psDroid->roll = 0;
	//psDroid->turretRotRate = 360;
	psDroid->turretRotation = 0;
	psDroid->turretPitch = 0;
	psDroid->selected = FALSE;
	psDroid->lastEmission = 0;
		// ffs AM
	psDroid->bTargetted = FALSE;
	psDroid->timeLastHit = UDWORD_MAX;
	psDroid->lastHitWeapon = UDWORD_MAX;	// no such weapon

	//allocate 'easy-access' data!
	//psDroid->sensorRange = (asSensorStats + pTemplate->asParts
	//	[COMP_SENSOR])->range;
	//psDroid->sensorPower = (asSensorStats + pTemplate->asParts
	//	[COMP_SENSOR])->power;
	psDroid->sensorRange = sensorRange((asSensorStats + pTemplate->asParts
		[COMP_SENSOR]), (UBYTE)player);
	psDroid->sensorPower = sensorPower((asSensorStats + pTemplate->asParts
		[COMP_SENSOR]), (UBYTE)player);

	/*psDroid->power = (asPowerStats + pTemplate->asParts
		[COMP_POWERPLANT])->output;
	psDroid->power = (asBodyStats + pTemplate->asParts[COMP_BODY])->
		powerOutput;*/

	//psDroid->ECMMod = (asECMStats + pTemplate->asParts[COMP_ECM])->power;
	psDroid->ECMMod = ecmPower((asECMStats + pTemplate->asParts[COMP_ECM]),
		(UBYTE) player);

	//psDroid->body = (asBodyStats + pTemplate->asParts[COMP_BODY])->bodyPoints;
	psDroid->body = calcTemplateBody(pTemplate, (UBYTE)player);
	psDroid->originalBody = psDroid->body;

	//if (psDroid->droidType == DROID_CYBORG)
    if (cyborgDroid(psDroid))
	{
		for (inc = 0; inc < NUM_WEAPON_CLASS; inc++)
		{
			//psDroid->armour[inc] = (asBodyStats + pTemplate->asParts[COMP_BODY])->armourValue[inc];
			psDroid->armour[inc] = bodyArmour(asBodyStats + pTemplate->
				asParts[COMP_BODY], (UBYTE)player, CYBORG_BODY_UPGRADE, inc);
		}
	}
	else
	{
		for (inc = 0; inc < NUM_WEAPON_CLASS; inc++)
		{
			psDroid->armour[inc] = bodyArmour(asBodyStats + pTemplate->
				asParts[COMP_BODY], (UBYTE)player, DROID_BODY_UPGRADE, inc);
		}
	}

		/*(asArmourStats + pTemplate->asParts[COMP_ARMOUR])->
		strength;*/

	//allocate the power points
	//psDroid->power = pTemplate->powerPoints;
    //init the resistance to indicate no EW performed on this droid
    psDroid->resistance = ACTION_START_TIME;

	memset(psDroid->visible, 0, sizeof(psDroid->visible));
	psDroid->visible[psDroid->player] = UBYTE_MAX;
	//psDroid->damage = droidDamage;
	psDroid->died = 0;
	psDroid->inFire = FALSE;
	psDroid->burnStart = 0;
	psDroid->burnDamage = 0;
//	psDroid->sAI.state = AI_PAUSE;
//	psDroid->sAI.psTarget = NULL;
//	psDroid->sAI.psSelectedWeapon = NULL;
//	psDroid->sAI.psStructToBuild = NULL;
	psDroid->sDisplay.screenX = OFF_SCREEN;
	psDroid->sDisplay.screenY = OFF_SCREEN;
	psDroid->sDisplay.screenR = 0;

	/* Set droid's initial illumination */
	psDroid->illumination = UBYTE_MAX;
	psDroid->sDisplay.imd = BODY_IMD(psDroid, psDroid->player);

	//don't worry if not on homebase cos not being drawn yet
	if (!onMission)
	{
		/* People always stand upright */
		if(psDroid->droidType != DROID_PERSON)
		{
			updateDroidOrientation(psDroid);
		}
		visTilesUpdate((BASE_OBJECT *)psDroid,FALSE);
		gridAddObject((BASE_OBJECT *)psDroid);
 		clustNewDroid(psDroid);
	}

	// create the command droid if necessary
	if (pTemplate->asParts[COMP_BRAIN] != 0)
	{
//		if (!buildCommandDroid((SDWORD)player, (SDWORD)pTemplate->asParts[COMP_BRAIN], psDroid))
//		{
//			droidRelease(psDroid);
//			HEAP_FREE(psDroidHeap, psDroid);
//	7		return NULL;
//		}

		//DON'T DO THIS ANYMORE - CAN HAVE 5 OF ONE TYPE ONLY!
		// now delete the template - can't build this twice
		/*psPrevTempl = NULL;
		for(psCurrTempl = apsDroidTemplates[player]; psCurrTempl;
			psCurrTempl = psCurrTempl->psNext)
		{
			if (psCurrTempl == pTemplate)
			{
				break;
			}
			psPrevTempl = psCurrTempl;
		}
		ASSERT( psCurrTempl != NULL,
			"buildDroid: command droid template not found for player %d", player );
		if (psPrevTempl)
		{
			psPrevTempl->psNext = psCurrTempl->psNext;
		}
		else
		{
			apsDroidTemplates[player] = psCurrTempl->psNext;
		}
		HEAP_FREE(psTemplateHeap, psCurrTempl);
		*/
	}


	// ajl. droid will be created, so inform others
	if(bMultiPlayer)
	{
		if (SendDroid(pTemplate,  x,  y,  (UBYTE)player, psDroid->id) == FALSE)
		{
			return NULL;
		}
	}


	/* transporter-specific stuff */
	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		//add transporter launch button if selected player and not a reinforcable situation
        if ( player == selectedPlayer AND !missionCanReEnforce())
		{
			(void)intAddTransporterLaunch(psDroid);
		}

		//set droid height to be above the terrain
		psDroid->z += TRANSPORTER_HOVER_HEIGHT;

		/* reset halt secondary order from guard to hold */
		secondarySetState( psDroid, DSO_HALTTYPE, DSS_HALT_HOLD );
	}

	if(player == selectedPlayer)
	{
		scoreUpdateVar(WD_UNITS_BUILT);
	}

	return psDroid;
}

//initialises the droid movement model
void initDroidMovement(DROID *psDroid)
{
	memset(&psDroid->sMove, 0, sizeof(MOVE_CONTROL));

	psDroid->sMove.fx = MAKEFRACT(psDroid->x);
	psDroid->sMove.fy = MAKEFRACT(psDroid->y);

	psDroid->sMove.fz = MAKEFRACT(psDroid->z);

}

// Set the asBits in a DROID structure given it's template.
//
void droidSetBits(DROID_TEMPLATE *pTemplate,DROID *psDroid)
{
	UDWORD						inc;

	psDroid->droidType = droidTemplateType(pTemplate);

	psDroid->direction=0;
	psDroid->pitch =  0;
	psDroid->roll = 0;
	//psDroid->turretRotRate = 360;
	psDroid->turretRotation = 0;
	psDroid->turretPitch = 0;

//	psDroid->ECMMod = (asECMStats + pTemplate->asParts[COMP_ECM])->power;

	psDroid->body = calcTemplateBody(pTemplate, psDroid->player);
	psDroid->originalBody = psDroid->body;

	//create the droids weapons
	//psDroid->numWeaps = pTemplate->numWeaps;
    psDroid->asWeaps[0].nStat = 0;
	if (pTemplate->numWeaps > 0)
	{
		//for (inc=0; inc < pTemplate->numWeaps; inc++)
        //can only have one weapon now
        inc = 0;
		{
			psDroid->asWeaps[inc].lastFired=0;
			psDroid->asWeaps[inc].nStat = pTemplate->asWeaps[inc];
			psDroid->asWeaps[inc].hitPoints = (asWeaponStats + psDroid->
				asWeaps[inc].nStat)->hitPoints;
			psDroid->asWeaps[inc].recoilValue = 0;
			psDroid->asWeaps[inc].ammo = (asWeaponStats + psDroid->
				asWeaps[inc].nStat)->numRounds;
		}
		//set the first weapon to be the current one
//		psDroid->activeWeapon = 0;
	}
	//allocate the components hit points
	psDroid->asBits[COMP_BODY].nStat = (UBYTE)pTemplate->asParts[COMP_BODY];
	//psDroid->asBits[COMP_BODY].hitPoints =
	//	(asBodyStats + pTemplate->asParts[COMP_BODY])->hitPoints;

	// ajl - changed this to init brains for all droids (crashed)
	psDroid->asBits[COMP_BRAIN].nStat = 0;

	// This is done by the Command droid stuff - John.
	// Not any more - John.
	psDroid->asBits[COMP_BRAIN].nStat = (UBYTE)pTemplate->asParts[COMP_BRAIN];
//	psDroid->asBits[COMP_BRAIN].hitPoints =
//		(asBrainStats + pTemplate->asParts[COMP_BRAIN])->hitPoints;

	/*psDroid->asBits[COMP_POWERPLANT].hitPoints =
		(asPowerStats + pTemplate->asParts[COMP_POWERPLANT])->hitPoints;*/

	psDroid->asBits[COMP_PROPULSION].nStat = (UBYTE)pTemplate->asParts[COMP_PROPULSION];
	//psDroid->asBits[COMP_PROPULSION].hitPoints =
	//	(asPropulsionStats + pTemplate->asParts[COMP_PROPULSION])->hitPoints;

	psDroid->asBits[COMP_SENSOR].nStat = (UBYTE)pTemplate->asParts[COMP_SENSOR];
	//psDroid->asBits[COMP_SENSOR].hitPoints =
	//	(asSensorStats + pTemplate->asParts[COMP_SENSOR])->hitPoints;

	psDroid->asBits[COMP_ECM].nStat = (UBYTE)pTemplate->asParts[COMP_ECM];
	//psDroid->asBits[COMP_ECM].hitPoints =
	//	(asECMStats + pTemplate->asParts[COMP_ECM])->hitPoints;

	/*psDroid->asBits[COMP_ARMOUR].hitPoints =
		(asArmourStats + pTemplate->asParts[COMP_ARMOUR])->hitPoints; */

	psDroid->asBits[COMP_REPAIRUNIT].nStat = (UBYTE)pTemplate->asParts[COMP_REPAIRUNIT];
	//psDroid->asBits[COMP_REPAIRUNIT].hitPoints =
	//	(asRepairStats + pTemplate->asParts[COMP_REPAIRUNIT])->hitPoints;

	psDroid->asBits[COMP_CONSTRUCT].nStat = (UBYTE)pTemplate->asParts[COMP_CONSTRUCT];
	//psDroid->asBits[COMP_CONSTRUCT].hitPoints =
	//	(asConstructStats + pTemplate->asParts[COMP_CONSTRUCT])->hitPoints;
}


// Sets the parts array in a template given a droid.
void templateSetParts(DROID *psDroid,DROID_TEMPLATE *psTemplate)
{
//	UDWORD inc;


	psTemplate->droidType = psDroid->droidType;

    //can only have one weapon now
    if (psDroid->asWeaps[0].nStat > 0)
    {
        psTemplate->numWeaps = 1;
		psTemplate->asWeaps[0] = psDroid->asWeaps[0].nStat;		// setup the weapon stat

    } else {
        psTemplate->numWeaps = 0;
	}

	psTemplate->asParts[COMP_BODY] = psDroid->asBits[COMP_BODY].nStat;

	psTemplate->asParts[COMP_BRAIN] = psDroid->asBits[COMP_BRAIN].nStat;

	psTemplate->asParts[COMP_PROPULSION] = psDroid->asBits[COMP_PROPULSION].nStat;

	psTemplate->asParts[COMP_SENSOR] = psDroid->asBits[COMP_SENSOR].nStat;

	psTemplate->asParts[COMP_ECM] = psDroid->asBits[COMP_ECM].nStat;

	psTemplate->asParts[COMP_REPAIRUNIT] = psDroid->asBits[COMP_REPAIRUNIT].nStat;

	psTemplate->asParts[COMP_CONSTRUCT] = psDroid->asBits[COMP_CONSTRUCT].nStat;
}




/*
fills the list with Templates that can be manufactured
in the Factory - based on size. There is a limit on how many can be manufactured
at any one time. Pass back the number available.
*/
UDWORD fillTemplateList(DROID_TEMPLATE **ppList, STRUCTURE *psFactory, UDWORD limit)
{
	DROID_TEMPLATE	*psCurr;
	UDWORD			count = 0;
	UDWORD			iCapacity = ((FACTORY*)psFactory->pFunctionality)->capacity;
	//BOOL			bCyborg = FALSE, bVtol = FALSE;

	/*if (psFactory->pStructureType->type == REF_CYBORG_FACTORY)
	{
		bCyborg = TRUE;
	}
	else if (psFactory->pStructureType->type == REF_VTOL_FACTORY)
	{
		bVtol = TRUE;
	}*/

	/* Add the templates to the list*/
	for (psCurr = apsDroidTemplates[psFactory->player]; psCurr != NULL;
		psCurr = psCurr->psNext)
	{
		//if on offworld mission, then can't build a Command Droid
		//if (mission.type == LDS_MKEEP OR mission.type == LDS_MCLEAR)
/*        if (missionIsOffworld())
		{
			if (psCurr->droidType == DROID_COMMAND)
			{
				continue;
			}
		}*/

		//must add Command Droid if currently in production
		if (!getProductionQuantity(psFactory, psCurr))
		{
			//can only have (MAX_CMDDROIDS) in the world at any one time
			if (psCurr->droidType == DROID_COMMAND)
			{
				if (checkProductionForCommand(psFactory->player) +
					checkCommandExist(psFactory->player) >= (MAX_CMDDROIDS))
				{
					continue;
				}
			}
		}

		if (!validTemplateForFactory(psCurr, psFactory))
		{
			continue;
		}

		//check the factory can cope with this sized body
		if (!((asBodyStats + psCurr->asParts[COMP_BODY])->size > iCapacity) )
		{
            //cyborg templates are available when the body has been research
            //-same for Transporter in multiPlayer
			if ( psCurr->droidType == DROID_CYBORG OR
				 psCurr->droidType == DROID_CYBORG_SUPER OR
                 psCurr->droidType == DROID_CYBORG_CONSTRUCT OR
                 psCurr->droidType == DROID_CYBORG_REPAIR OR
                 psCurr->droidType == DROID_TRANSPORTER)
			{
				if ( apCompLists[psFactory->player][COMP_BODY]
					 [psCurr->asParts[COMP_BODY]] != AVAILABLE )
				{
					//ignore if not research yet
					continue;
				}
			}
			*ppList++ = psCurr;
			count++;
			//once reached the limit, stop adding any more to the list
			if (count == limit)
			{
				return count;
			}
		}
	}
	return count;
}


#define DROIDPLAYER1 (6)
#define DROIDPLAYER2 (7)

//determines the best IMD to draw for the droid - A TEMP MEASURE!
/*void setDroidIMD(DROID *psDroid)
{
	UDWORD	imdNum;

	//Big droid for end of Demo
	if (psDroid->player == 1)
	{
		imdNum = BARBARIANS + 3;
	}
	else
	{
		//Barbarian droids!!
		if (psDroid->player == DROIDPLAYER1 OR psDroid->player == DROIDPLAYER2)
		{
			imdNum = BARBARIANS;
			if (!strcmp(asBodyStats[psDroid->asBits[COMP_BODY].nStat].pName,
				"BaBa Body"))
			{
				imdNum = BARBARIANS + 1;
			}
			else if (!strcmp(asBodyStats[psDroid->asBits[COMP_BODY].nStat].pName,
				"Buggy Body"))
			{
				imdNum = BARBARIANS + 2;
			}
		}
		else
		{
			if(psDroid->numWeaps)
			{
				if(asWeaponStats[psDroid->asWeaps[0].nStat].direct)
				{
					//either a light or heavy cannon or a machine gun
					if (!strcmp(asWeaponStats[psDroid->asWeaps[0].nStat].pName,
						"Light Cannon"))
					{
						imdNum = CANNON_DROIDS;
					}
					else if (!strcmp(asWeaponStats[psDroid->asWeaps[0].nStat].pName,
						"Heavy Cannon"))
					{
						imdNum = CANNON_HEAVY_DROIDS;
					}
					else
					{
						imdNum = MACHGUN_DROIDS;
					}
				}
				else
				{
					//either  rocket1 or rocket2
					if (!strcmp(asWeaponStats[psDroid->asWeaps[0].nStat].pName,
						"Single Rocket"))
					{
						imdNum = ROCKET_DROIDS;
					}
					else
					{
						imdNum = ROCKET2_DROIDS;
					}
				}
			}
			else
			// No weapon droid
			{
				if(psDroid->asBits[COMP_CONSTRUCT].nStat)
				{
					// Constructor droid
					imdNum = CONST_DROIDS;
				}
				else
				{
					imdNum = RADAR_DROIDS;
				}
			}

			//sub group droids by body type and propulsion
			if (asBodyStats[psDroid->asBits[COMP_BODY].nStat].size == SIZE_MEDIUM)
			{
				imdNum += 3;
			}
			//propulsion types 0=wheeled, 1=tracked, 3=hover
			if (asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].
				propulsionType == 1)
			{
				imdNum += 1;
			}
			else if (asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].
				propulsionType == 3)
			{
				imdNum +=2;
			}
		}
	}
	//fail safe in case haven't managed to work out a valid number!
#ifndef PSX		// currently psx only has 2 droid imds .. so this check will not work
	if (imdNum > NUM_DROID_TYPES - 1)
	{
		imdNum = 0;
	}
#endif
	psDroid->sDisplay.imd = BODY_IMD(psDroid, psDroid->player);
	psDroid->imdNum = imdNum; // use the imdnum to define which body and which turret
}*/


// Determine the best IMD to draw for the droid - A TEMP MEASURE!
//
// Same as setDroidIMD() but uses a template structure instead
// of a droid structure.
//
/*UDWORD GetIMDFromTemplate(DROID_TEMPLATE *Template,UDWORD Player)
{
	UDWORD	imdNum;

	//Big droid for end of Demo
	if (Player == 1)
	{
		imdNum = BARBARIANS + 3;
	}
	else
	{
		//Barbarian droids!!
		if (Player == 6 OR Player == 7)
		{
			imdNum = BARBARIANS;
			if (!strcmp(asBodyStats[Template->asParts[COMP_BODY]].pName,
				"BaBa Body"))
			{
				imdNum = BARBARIANS + 1;
			}
			else if (!strcmp(asBodyStats[Template->asParts[COMP_BODY]].pName,
				"Buggy Body"))
			{
				imdNum = BARBARIANS + 2;
			}
		}
		else
		{
			if(Template->numWeaps)
			{
				if(asWeaponStats[Template->asWeaps[0]].direct)
				{
					//either a light or heavy cannon or a machine gun
					if (!strcmp(asWeaponStats[Template->asWeaps[0]].pName,
						"Light Cannon"))
					{
						imdNum = CANNON_DROIDS;
					}
					else if (!strcmp(asWeaponStats[Template->asWeaps[0]].pName,
						"Heavy Cannon"))
					{
						imdNum = CANNON_HEAVY_DROIDS;
					}
					else
					{
						imdNum = MACHGUN_DROIDS;
					}
				}
				else
				{
					//either  rocket1 or rocket2
					if (!strcmp(asWeaponStats[Template->asWeaps[0]].pName,
						"Single Rocket"))
					{
						imdNum = ROCKET_DROIDS;
					}
					else
					{
						imdNum = ROCKET2_DROIDS;
					}
				}
			}
			else
			// No weapon droid
			{
				if(Template->asParts[COMP_CONSTRUCT])
				{
					// Constructor droid
					imdNum = CONST_DROIDS;
				}
				else
				{
					imdNum = RADAR_DROIDS;
				}
			}

			//sub group droids by body type and propulsion
			if (asBodyStats[Template->asParts[COMP_BODY]].size == SIZE_MEDIUM)
			{
				imdNum += 3;
			}
			//propulsion types 0=wheeled, 1=tracked, 3=hover
			if (asPropulsionStats[Template->asParts[COMP_PROPULSION]].
				propulsionType == 1)
			{
				imdNum += 1;
			}
			else if (asPropulsionStats[Template->asParts[COMP_PROPULSION]].
				propulsionType == 3)
			{
				imdNum +=2;
			}
		}
	}
	//fail safe in case haven't managed to work out a valid number!
	if (imdNum > NUM_DROID_TYPES - 1)
	{
		imdNum = 0;
	}

	return imdNum;
}*/

// set the keyboard group for a droid

/* Make all the droids for a certain player a member of a specific group */
void assignDroidsToGroup(UDWORD	playerNumber, UDWORD groupNumber)
{
DROID	*psDroid;
BOOL	bAtLeastOne = FALSE;
FLAG_POSITION	*psFlagPos;

	if(groupNumber<UBYTE_MAX)
	{
		/* Run through all the droids */
		for(psDroid = apsDroidLists[playerNumber]; psDroid!=NULL; psDroid = psDroid->psNext)
		{
			/* Clear out the old ones */
			if(psDroid->group == groupNumber)
			{
				psDroid->group = UBYTE_MAX;
			}

			/* Only assign the currently selected ones */
			if(psDroid->selected)
			{
				/* Set them to the right group - they can only be a member of one group */
				psDroid->group = (UBYTE)groupNumber;
				bAtLeastOne = TRUE;
			}
		}
	}
	if(bAtLeastOne)
	{
		setSelectedGroup(groupNumber);
		//clear the Deliv Point if one
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = FALSE;
		}
		groupConsoleInformOfCreation(groupNumber);
		secondarySetAverageGroupState(selectedPlayer, groupNumber);
	}
}


BOOL activateGroupAndMove(UDWORD playerNumber, UDWORD groupNumber)
{
DROID	*psDroid,*psCentreDroid;
BOOL Selected = FALSE;
FLAG_POSITION	*psFlagPos;

	if(groupNumber<UBYTE_MAX)
	{
		psCentreDroid = NULL;
		for(psDroid = apsDroidLists[playerNumber]; psDroid!=NULL; psDroid = psDroid->psNext)
		{
			/* Wipe out the ones in the wrong group */
			if(psDroid->selected AND psDroid->group!=groupNumber)
			{
//				psDroid->selected = FALSE;
				DeSelectDroid(psDroid);
			}
			/* Get the right ones */
			if(psDroid->group == groupNumber)
			{
//				psDroid->selected = TRUE;
				SelectDroid(psDroid);
				psCentreDroid = psDroid;
			}
		}

		/* There was at least one in the group */
		if(psCentreDroid)
		{
			//clear the Deliv Point if one
			for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
			psFlagPos = psFlagPos->psNext)
			{
				psFlagPos->selected = FALSE;
			}

			Selected = TRUE;
			if(!driveModeActive()) {

				if(getWarCamStatus())
				{
					camToggleStatus();			 // messy - fix this
			//		setViewPos(psCentreDroid->x>>TILE_SHIFT,psCentreDroid->y>>TILE_SHIFT);
					processWarCam(); //odd, but necessary
					camToggleStatus();				// messy - FIXME
				}
				else
				if(!getWarCamStatus())
				{
	//				camToggleStatus();
					/* Centre display on him if warcam isn't active */
//#ifndef PSX
					setViewPos(psCentreDroid->x>>TILE_SHIFT,psCentreDroid->y>>TILE_SHIFT,TRUE);
//#else
//					camPanToLocation(psCentreDroid->x, psCentreDroid->y);
//#endif
				}

			}
		}
	}

	if(Selected)
	{
		setSelectedGroup(groupNumber);
		groupConsoleInformOfCentering(groupNumber);
	}
	else
	{
		setSelectedGroup(UBYTE_MAX);
	}

	return Selected;
}

BOOL activateGroup(UDWORD playerNumber, UDWORD groupNumber)
{
DROID	*psDroid;
BOOL Selected = FALSE;
FLAG_POSITION	*psFlagPos;

	if(groupNumber<UBYTE_MAX)
	{
		for(psDroid = apsDroidLists[playerNumber]; psDroid!=NULL; psDroid = psDroid->psNext)
		{
			/* Wipe out the ones in the wrong group */
			if(psDroid->selected AND psDroid->group!=groupNumber)
			{
//				psDroid->selected = FALSE;
				DeSelectDroid(psDroid);
			}
			/* Get the right ones */
			if(psDroid->group == groupNumber)
			{
//				psDroid->selected = TRUE;
				SelectDroid(psDroid);
				Selected = TRUE;
			}
		}
	}

	if(Selected)
	{
//		if(getWarCamStatus())
//		{
//			camToggleStatus();
//		}
		setSelectedGroup(groupNumber);
		//clear the Deliv Point if one
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = FALSE;
		}
		groupConsoleInformOfSelection(groupNumber);
	}
	else
	{
		setSelectedGroup(UBYTE_MAX);
	}
	return Selected;
}

void	groupConsoleInformOfSelection( UDWORD groupNumber )
{
		// ffs am
char	groupInfo[255];
//	if(!getWarCamStatus())
//	{
		sprintf(groupInfo,strresGetString(psStringRes,STR_GP_SELECTED),groupNumber,selNumSelected(selectedPlayer));
		addConsoleMessage(groupInfo,RIGHT_JUSTIFY);
//	}

}

void	groupConsoleInformOfCreation( UDWORD groupNumber )
{

char	groupInfo[255];
	if(!getWarCamStatus())
	{
		sprintf(groupInfo,strresGetString(psStringRes,STR_GP_ASSIGNED),selNumSelected(selectedPlayer),groupNumber);
		addConsoleMessage(groupInfo,RIGHT_JUSTIFY);
	}

}

void	groupConsoleInformOfCentering( UDWORD groupNumber )
{

char	groupInfo[255];
	if(!getWarCamStatus())
	{
		sprintf(groupInfo,strresGetString(psStringRes,STR_GP_CENTERED),groupNumber,selNumSelected(selectedPlayer));
	}
	else
	{
		sprintf(groupInfo,strresGetString(psStringRes,STR_GP_ALLIGN),groupNumber,selNumSelected(selectedPlayer));
	}
		addConsoleMessage(groupInfo,RIGHT_JUSTIFY);

}



UDWORD	getSelectedGroup( void )
{
	return(selectedGroup);
}

void	setSelectedGroup(UDWORD groupNumber)
{
	selectedGroup = groupNumber;
	selectedCommander = UBYTE_MAX;
}

UDWORD	getSelectedCommander( void )
{
	return(selectedCommander);
}

void	setSelectedCommander(UDWORD commander)
{
	selectedGroup = UBYTE_MAX;
	selectedCommander = commander;
}

/* calculate muzzle tip location in 3d world */
BOOL calcDroidMuzzleLocation(DROID *psDroid, iVector *muzzle)
{
//	UDWORD turretType;
//	UDWORD bodyType;
	iVector			barrel;
 	iIMDShape		*psShape, *psWeapon, *psWeaponMount;

	psShape       = BODY_IMD(psDroid,psDroid->player);
	psWeapon      = WEAPON_IMD(psDroid,psDroid->player);
	psWeaponMount = WEAPON_MOUNT_IMD(psDroid,psDroid->player);


	if(psShape AND psShape->nconnectors)
	{



		// This code has not been translated to the PSX Yet !!!!                                     (sorry)
		pie_MatBegin();

		pie_TRANSLATE(psDroid->x,-(SDWORD)psDroid->z,psDroid->y);
		//matrix = the center of droid
		pie_MatRotY(DEG((SDWORD) psDroid->direction));
		pie_MatRotX(DEG(psDroid->pitch));
		pie_MatRotZ(DEG(-(SDWORD)psDroid->roll));
//		pie_TRANSLATE(100,0,0);			//	(left,-height,forward)
		pie_TRANSLATE( psShape->connectors->x, -psShape->connectors->z,
					  -psShape->connectors->y);//note y and z flipped

		//matrix = the gun and turret mount on the body
		pie_MatRotY(DEG((SDWORD)psDroid->turretRotation));//+ve anticlockwise
		pie_MatRotX(DEG(psDroid->turretPitch));//+ve up
   		pie_MatRotZ(DEG(0));
		//matrix = the muzzle mount on turret
		if( psWeapon AND psWeapon->nconnectors )
		{
			barrel.x = psWeapon->connectors->x;
			barrel.y = -psWeapon->connectors->y;
			barrel.z = -psWeapon->connectors->z;
		}
		else
		{
			barrel.x = 0;
			barrel.y = 0;
			barrel.z = 0;
		}

		pie_ROTATE_TRANSLATE(barrel.x, barrel.z, barrel.y, muzzle->x, muzzle->z, muzzle->y);
		muzzle->z = -muzzle->z;

		pie_MatEnd();




	}


  else
	{
		muzzle->x = psDroid->x;
		muzzle->y = psDroid->y;
		muzzle->z = psDroid->z+32;
	}
  return TRUE;
}



/* IF YOU USE THIS FUNCTION - NOTE THAT selectedPlayer's TEMPLATES ARE NOT USED!!!!
   gets a template from its name - relies on the name being unique (or it will
   return the first one it finds!! */
DROID_TEMPLATE * getTemplateFromName(STRING *pName)
{
	UDWORD			player;
	DROID_TEMPLATE	*psCurr;

#ifdef HASH_NAMES
	UDWORD			HashedName=HashString(pName);
#endif

#ifdef RESOURCE_NAMES

	if (!getResourceName(pName))
	{
		return NULL;
	}

#endif

	/*all droid and template names are now stored as the translated
	name regardless of RESOURCE_NAMES and STORE_RESOURCE_ID! - AB 26/06/98*/
	getDroidResourceName(pName);


	for (player = 0; player < MAX_PLAYERS; player++)
	{
        //OK so we want selectedPlayer's CYBORG templates since they cannot be edited
        //and we don't want to duplicate them for the sake of it! (ha!)
        //don't use selectedPlayer's templates if not multiplayer
        //this was written for use in the scripts and we don't want the scripts to use
        //selectedPlayer's templates because we cannot guarentee they will exist!
/*#ifndef PSX
        if (!bMultiPlayer)
#endif
        {
            if (player == selectedPlayer)
            {
                continue;
            }
        }
*/
		for (psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
#ifdef HASH_NAMES
			if (psCurr->NameHash==HashedName)
#else
			//if (!strcmp(psCurr->pName, pName))
			if (!strcmp(psCurr->aName, pName))
#endif
			{
                //if template is selectedPlayers' it must be a CYBORG or we ignore it

                if (!bMultiPlayer)

                {
                    //if (player == selectedPlayer AND psCurr->droidType != DROID_CYBORG)
                    if (player == selectedPlayer AND
                        !(psCurr->droidType == DROID_CYBORG OR
						  psCurr->droidType == DROID_CYBORG_SUPER OR
                          psCurr->droidType == DROID_CYBORG_CONSTRUCT OR
                          psCurr->droidType == DROID_CYBORG_REPAIR))
                    {
                        //ignore
                        continue;
                    }
                }
				return psCurr;
			}
		}
	}
	return NULL;
}

/*getTemplatefFromSinglePlayerID gets template for unique ID  searching one players list */
DROID_TEMPLATE* getTemplateFromSinglePlayerID(UDWORD multiPlayerID, UDWORD player)
{
	DROID_TEMPLATE	*pDroidDesign;

	for(pDroidDesign = apsDroidTemplates[player]; pDroidDesign != NULL; pDroidDesign = pDroidDesign->psNext)
	{
		if (pDroidDesign->multiPlayerID == multiPlayerID)
		{
			return pDroidDesign;
		}
	}
	return NULL;
}

/*getTemplatefFromMultiPlayerID gets template for unique ID  searching all lists */
DROID_TEMPLATE* getTemplateFromMultiPlayerID(UDWORD multiPlayerID)
{
	UDWORD			player;
	DROID_TEMPLATE	*pDroidDesign;

	for (player=0; player < MAX_PLAYERS; player++)
	{
		pDroidDesign = getTemplateFromSinglePlayerID(multiPlayerID, player);
		if (pDroidDesign != NULL)
		{
			return pDroidDesign;
		}
	}
	return NULL;
}

// finds a droid for the player and sets it to be the current selected droid
BOOL selectDroidByID(UDWORD id, UDWORD player)
{
	DROID	*psCurr;

	//look through the list of droids for the player and find the matching id
	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->id == id)
		{
			break;
		}
	}
	if (psCurr != NULL)
	{
		clearSelection();
//		psCurr->selected = TRUE;
		SelectDroid(psCurr);
		return TRUE;
	}
	return FALSE;
}

UDWORD	getDroidLevel(DROID *psDroid)
{
	if (psDroid->droidType == DROID_COMMAND ||
		psDroid->droidType == DROID_SENSOR)
	{
		return cmdDroidGetLevel(psDroid);
	}
	else if(psDroid->numKills <4)
	{
		return(0);
	}
	else if(psDroid->numKills<8)
	{
		return(1);
	}
	else if(psDroid->numKills<16)
	{
		return(2);
	}
	else if(psDroid->numKills<32)
	{
		return(3);
	}
	else if(psDroid->numKills<64)
	{
		return(4);
	}
	else if(psDroid->numKills<128)
	{
		return(5);
	}
	else if(psDroid->numKills<256)
	{
		return(6);
	}
	else if(psDroid->numKills<512)
	{
		return(7);
	}
	else
	{
		return(8);
	}
}



STRING	*getDroidNameForRank(UDWORD rank)
{

switch(rank)
{
	case 0:
		return strresGetString(psStringRes, STR_DL_LEVEL_ROOKIE);
	case 1:
		return strresGetString(psStringRes, STR_DL_LEVEL_GREEN);
	case 2:
		return strresGetString(psStringRes, STR_DL_LEVEL_TRAINED);
	case 3:
		return strresGetString(psStringRes, STR_DL_LEVEL_REGULAR);
	case 4:
		return strresGetString(psStringRes, STR_DL_LEVEL_VETERAN);
	case 5:
		return strresGetString(psStringRes, STR_DL_LEVEL_CRACK);
	case 6:
		return strresGetString(psStringRes, STR_DL_LEVEL_ELITE);
	case 7:
		return strresGetString(psStringRes, STR_DL_LEVEL_SPECIAL);
	case 8:
		return strresGetString(psStringRes, STR_DL_LEVEL_ACE);
	}

	return NULL;
}

STRING	*getDroidLevelName(DROID *psDroid)
{
//#ifndef PSX
	return(getDroidNameForRank(getDroidLevel(psDroid)));
	/*
	switch (getDroidLevel(psDroid))
	{
	case 0:
		return strresGetString(psStringRes, STR_DL_LEVEL_ROOKIE);
	case 1:
		return strresGetString(psStringRes, STR_DL_LEVEL_GREEN);
	case 2:
		return strresGetString(psStringRes, STR_DL_LEVEL_TRAINED);
	case 3:
		return strresGetString(psStringRes, STR_DL_LEVEL_REGULAR);
	case 4:
		return strresGetString(psStringRes, STR_DL_LEVEL_VETERAN);
	case 5:
		return strresGetString(psStringRes, STR_DL_LEVEL_CRACK);
	case 6:
		return strresGetString(psStringRes, STR_DL_LEVEL_ELITE);
	case 7:
		return strresGetString(psStringRes, STR_DL_LEVEL_SPECIAL);
	case 8:
		return strresGetString(psStringRes, STR_DL_LEVEL_ACE);
	}
	*/
//#else
//		return "Rank";
//#endif
}

UDWORD	getNumDroidsForLevel(UDWORD	level)
{
//UDWORD	lower,upper;
DROID	*psDroid;
UDWORD	count;
//UDWORD	numKills;

/*	if(level)
	{
		lower = getBound(level-1);
		upper = getBound(level);
	}
	else
	{
		lower = upper = 0;
	}*/

	for(psDroid = apsDroidLists[selectedPlayer],count = 0;
		psDroid; psDroid = psDroid->psNext)
	{
/*		numKills = psDroid->numKills;
		if(level ? (numKills > lower AND numKills <=upper) : (numKills ==0) )*/
		if (getDroidLevel(psDroid) == level)
		{
			count++;
		}
	}

	return(count);
}



UDWORD	getBound(UDWORD level)
{
	if(level ==0)
	{
		return(0);
	}
	else if(level<8)
	{
		return(1<<(level+2));
	}
	else return(UDWORD_MAX);

	switch(level)
	{
	case 0:
		return(0);
		break;
	case 1:
		return(4);
		break;
	case 2:
		return(8);
		break;
	case 3:
		return(16);
		break;
	case 4:
		return(32);
		break;
	case 5:
		return(64);
		break;
	case 6:
		return(128);
		break;
	case 7:
		return(512);
		break;
	default:
		return(UDWORD_MAX);
			break;
	}
}

/* Calculate the system points used by a template - NOT USED AT PRESENT*/
UDWORD	calcTemplateSystemPoints(DROID_TEMPLATE *psTemplate)
{
	UDWORD system, i;

	//get the component system points
	system = (asBodyStats + psTemplate->asParts[COMP_BODY])->systemPoints +
	(asBrainStats + psTemplate->asParts[COMP_BRAIN])->systemPoints +
	//(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->buildPower +
	(asSensorStats + psTemplate->asParts[COMP_SENSOR])->systemPoints +
	(asECMStats + psTemplate->asParts[COMP_ECM])->systemPoints +
	(asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->systemPoints +
	(asConstructStats + psTemplate->asParts[COMP_CONSTRUCT])->systemPoints;

	/* propulsion system points are a percentage of the bodys' system points */
	system += (((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->systemPoints *
		(asBodyStats + psTemplate->asParts[COMP_BODY])->systemPoints) / 100);

	//add weapon system
	for(i=0; i<psTemplate->numWeaps; i++)
	{
		system += (asWeaponStats + psTemplate->asWeaps[i])->systemPoints;
	}

	//add program system
	/*for(i=0; i<psTemplate->numProgs; i++)
	{
		system += (asProgramStats + psTemplate->asProgs[i])->systemPoints;
	}*/

	return system;
}


/* Calculate the system points used by a droid - NOT USED AT PRESENT*/
UDWORD	calcDroidSystemPoints(DROID *psDroid)
{
	UDWORD      system;//, i;

	//get the component system points
	system = (asBodyStats + psDroid->asBits[COMP_BODY].nStat)->systemPoints +
	(asBrainStats + psDroid->asBits[COMP_BRAIN].nStat)->systemPoints +
	(asSensorStats + psDroid->asBits[COMP_SENSOR].nStat)->systemPoints +
	(asECMStats + psDroid->asBits[COMP_ECM].nStat)->systemPoints +
	(asRepairStats + psDroid->asBits[COMP_REPAIRUNIT].nStat)->systemPoints +
	(asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat)->systemPoints;

	/* propulsion system points are a percentage of the bodys' system points */
	system += (((asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat)->
		systemPoints * (asBodyStats + psDroid->asBits[COMP_BODY].nStat)->
		systemPoints) / 100);

	//add weapon system
	//for(i=0; i<psDroid->numWeaps; i++)
    if (psDroid->asWeaps[0].nStat > 0)
	{
		//system += (asWeaponStats + psDroid->asWeaps[i].nStat)->systemPoints;
        system += (asWeaponStats + psDroid->asWeaps[0].nStat)->systemPoints;
	}

	//add program system
	/*for(i=0; i<psDroid->numProgs; i++)
	{
		system += psDroid->asProgs[i].psStats->systemPoints;
	}*/

	return system;
}




// Get the name of a droid from it's DROID structure.
//
STRING *droidGetName(DROID *psDroid)
{
#ifndef HASH_NAMES
	return (psDroid->aName);
#else
	const STRING ConstructedName[]="Body Mk XXIV";	// dummy name

	// if the hashed name is 0 - it must be a user created droid so we construct its name
	if (psDroid->HashedDroidName!=0x0)
	{
		STRING *DroidName;
		DroidName=strresGetString(NULL,psDroid->HashedDroidName);
		if (DroidName!=NULL) return(DroidName);	// if we found a name then return it...  other wise build the name
	}

	BuildNameFromDroid(psDroid,ConstructedName);
	return(ConstructedName);

#endif
}



//
// Set the name of a droid in it's DROID structure.
//
// - only possible on the PC where you can adjust the names,
//
void droidSetName(DROID *psDroid,STRING *pName)
{
#ifndef HASH_NAMES
	strncpy(psDroid->aName,pName, DROID_MAXNAME);
	psDroid->aName[DROID_MAXNAME-1] = 0;
#endif
}



// ////////////////////////////////////////////////////////////////////////////
// returns true when no droid on x,y square.
BOOL noDroid(UDWORD x, UDWORD y)
{
	UDWORD i;
	DROID *pD;
	// check each droid list
	for(i=0;i<MAX_PLAYERS;i++)
	{
		for(pD = apsDroidLists[i]; pD ; pD= pD->psNext)
		{
			if((UDWORD)(pD->x >> TILE_SHIFT) == x)
			{
				if((UDWORD)(pD->y >> TILE_SHIFT) == y)
				{
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// returns true when one droid on x,y square.
BOOL oneDroid(UDWORD x, UDWORD y)
{
	UDWORD i;
	BOOL bFound = FALSE;
	DROID *pD;
	// check each droid list
	for(i=0;i<MAX_PLAYERS;i++)
	{
		for(pD = apsDroidLists[i]; pD ; pD= pD->psNext)
		{
			if((UDWORD)(pD->x >> TILE_SHIFT) == x)
			{
				if((UDWORD)(pD->y >> TILE_SHIFT) == y)
				{
					if (bFound)
					{
						return FALSE;
					}
					else
					{
						bFound = TRUE;//first droid on this square so continue
					}
				}
			}
		}
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// returns true if it's a sensible place to put that droid.
BOOL sensiblePlace(SDWORD x, SDWORD y)
{
	UDWORD count=0;

	// not too near the edges.
	if((x < TOO_NEAR_EDGE) || (x > (SDWORD)(mapWidth - TOO_NEAR_EDGE)))
		return FALSE;
	if((y < TOO_NEAR_EDGE) || (y > (SDWORD)(mapHeight - TOO_NEAR_EDGE)))
		return FALSE;

    //check no features there
	if(TILE_HAS_FEATURE(mapTile(x,y)))
	{
		return FALSE;
	}

	// not on a blocking tile.
	if( fpathBlockingTile(x,y) )
		return FALSE;

	// shouldn't next to more than one blocking tile, to avoid windy paths.
	if( fpathBlockingTile(x-1 ,y-1) )
		count++;
	if( fpathBlockingTile(x,y-1) )
		count++;
	if( fpathBlockingTile(x+1,y-1) )
		count++;
	if( fpathBlockingTile(x-1,y) )
		count++;
	if( fpathBlockingTile(x+1,y) )
		count++;
	if( fpathBlockingTile(x-1,y+1) )
		count++;
	if( fpathBlockingTile(x,y+1) )
		count++;
	if( fpathBlockingTile(x+1,y+1) )
		count++;

	if(count > 1)
		return FALSE;

	return TRUE;
}


// ------------------------------------------------------------------------------------
BOOL	normalPAT(UDWORD x, UDWORD y)
{
	if(sensiblePlace(x,y) AND noDroid(x,y))
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}
// ------------------------------------------------------------------------------------
// Should stop things being placed in inaccessible areas?
BOOL	zonedPAT(UDWORD x, UDWORD y)
{
	if(sensiblePlace(x,y) AND noDroid(x,y) AND gwZoneReachable(gwGetZone(x,y)))
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}
// ------------------------------------------------------------------------------------
BOOL	halfPAT(UDWORD x, UDWORD y)
{
 	if(sensiblePlace(x,y) AND oneDroid(x,y))
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

// ------------------------------------------------------------------------------------
BOOL	pickATileGen(UDWORD *x, UDWORD *y, UBYTE numIterations,
					 BOOL (*function)(UDWORD x, UDWORD y))
{
SDWORD	i,j;
SDWORD	startX,endX,startY,endY;
UDWORD	passes;


	ASSERT( *x>=0 AND *x<mapWidth,"x coordinate is off-map for pickATileGen" );
	ASSERT( *y>=0 AND *y<mapHeight,"y coordinate is off-map for pickATileGen" );

	/* Exit if they're fine! */
	if(sensiblePlace(*x,*y) AND noDroid(*x,*y))
	{
		return(TRUE);
	}

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = *x;	startY = endY = *y;	passes = 0;



	/* Keep going until we get a tile or we exceed distance */
	while(passes<numIterations)
	{
		/* Process whole box */
		for(i = startX; i <= endX; i++)
		{
			for(j = startY; j<= endY; j++)
			{
				/* Test only perimeter as internal tested previous iteration */
				if(i==startX OR i==endX OR j==startY OR j==endY)
				{
					/* Good enough? */
					if(function(i,j))
					{
						/* Set exit conditions and get out NOW */
						*x = i;	*y = j;
						return(TRUE);
					}
				}
			}
		}
		/* Expand the box out in all directions - off map handled by tileAcceptable */
		startX--; startY--;	endX++;	endY++;	passes++;
	}
	/* If we got this far, then we failed - passed in values will be unchanged */
	return(FALSE);

}

//same as orig, but with threat check
BOOL	pickATileGenThreat(UDWORD *x, UDWORD *y, UBYTE numIterations, SDWORD threatRange,
					 SDWORD player, BOOL (*function)(UDWORD x, UDWORD y))
{
SDWORD	i,j;
SDWORD	startX,endX,startY,endY;
UDWORD	passes;


	ASSERT( *x>=0 AND *x<mapWidth,"x coordinate is off-map for pickATileGen" );
	ASSERT( *y>=0 AND *y<mapHeight,"y coordinate is off-map for pickATileGen" );

	if(function(*x,*y) && ((threatRange <=0) || (!ThreatInRange(player, threatRange, *x, *y, FALSE))))	//TODO: vtol check really not needed?
	{
		return(TRUE);
	}

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = *x;	startY = endY = *y;	passes = 0;

	/* Keep going until we get a tile or we exceed distance */
	while(passes<numIterations)
	{
		/* Process whole box */
		for(i = startX; i <= endX; i++)
		{
			for(j = startY; j<= endY; j++)
			{
				/* Test only perimeter as internal tested previous iteration */
				if(i==startX OR i==endX OR j==startY OR j==endY)
				{
					/* Good enough? */
					if(function(i,j) && ((threatRange <=0) || (!ThreatInRange(player, threatRange, i << TILE_SHIFT, j << TILE_SHIFT, FALSE))))		//TODO: vtols check really not needed?
					{
						/* Set exit conditions and get out NOW */
						*x = i;	*y = j;
						return(TRUE);
					}
				}
			}
		}
		/* Expand the box out in all directions - off map handled by tileAcceptable */
		startX--; startY--;	endX++;	endY++;	passes++;
	}
	/* If we got this far, then we failed - passed in values will be unchanged */
	return(FALSE);

}

// ------------------------------------------------------------------------------------
/* Improved pickATile - Replaces truly scary existing one. */
/* AM 22 - 10 - 98 */
BOOL	pickATile(UDWORD *x, UDWORD *y, UBYTE numIterations)
{
SDWORD	i,j;
SDWORD	startX,endX,startY,endY;
UDWORD	passes;


	ASSERT( *x>=0 AND *x<mapWidth,"x coordinate is off-map for pickATile" );
	ASSERT( *y>=0 AND *y<mapHeight,"y coordinate is off-map for pickATile" );

	/* Exit if they're fine! */
	if(sensiblePlace(*x,*y) AND noDroid(*x,*y))
	{
		return(TRUE);
	}

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = *x;	startY = endY = *y;	passes = 0;



	/* Keep going until we get a tile or we exceed distance */
	while(passes<numIterations)
	{
		/* Process whole box */
		for(i = startX; i <= endX; i++)
		{
			for(j = startY; j<= endY; j++)
			{
				/* Test only perimeter as internal tested previous iteration */
				if(i==startX OR i==endX OR j==startY OR j==endY)
				{
					/* Good enough? */
					if(sensiblePlace(i,j) AND noDroid(i,j))
					{
						/* Set exit conditions and get out NOW */
						*x = i;	*y = j;
						return(TRUE);
					}
				}
			}
		}
		/* Expand the box out in all directions - off map handled by tileAcceptable */
		startX--; startY--;	endX++;	endY++;	passes++;
	}
	/* If we got this far, then we failed - passed in values will be unchanged */
	return(FALSE);
}
// pickHalfATile just like improved pickATile but with Double Density Droid Placement
PICKTILE pickHalfATile(UDWORD *x, UDWORD *y, UBYTE numIterations)
{
SDWORD	i,j;
SDWORD	startX,endX,startY,endY;
UDWORD	passes;

	/*
		Why was this written - I wrote pickATileGen to take a function
		pointer for what qualified as a valid tile - could use that.
		I'm not going to change it in case I'm missing the point */
	if (pickATileGen(x, y, numIterations,zonedPAT))
	{
		return FREE_TILE;
	}

	/* Exit if they're fine! */
	if(sensiblePlace(*x,*y) AND oneDroid(*x,*y))
	{
		return(TRUE);
	}

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = *x;	startY = endY = *y;	passes = 0;



	/* Keep going until we get a tile or we exceed distance */
	while(passes<numIterations)
	{
		/* Process whole box */
		for(i = startX; i <= endX; i++)
		{
			for(j = startY; j<= endY; j++)
			{
				/* Test only perimeter as internal tested previous iteration */
				if(i==startX OR i==endX OR j==startY OR j==endY)
				{
					/* Good enough? */
					if(sensiblePlace(i,j) AND oneDroid(i,j))
					{
						/* Set exit conditions and get out NOW */
						*x = i;	*y = j;
						return HALF_FREE_TILE;
					}
				}
			}
		}
		/* Expand the box out in all directions - off map handled by tileAcceptable */
		startX--; startY--;	endX++;	endY++;	passes++;
	}
	/* If we got this far, then we failed - passed in values will be unchanged */
	return NO_FREE_TILE;
}


// ////////////////////////////////////////////////////////////////////////////
// returns an x/y tile coord to place a droid
/* works round the location in a clockwise direction  Returns FALSE if a valid
location isn't found*/
/*BOOL pickATile(UDWORD *pX0,UDWORD *pY0, UBYTE numIterations)
{
	BOOL		found = FALSE;
	UDWORD		startX, startY, incX, incY;
	SDWORD		x=0, y=0;

	startX = *pX0;
	startY = *pY0;


	for (incX = 1, incY = 1; incX < numIterations; incX++, incY++)
	{
		if (!found)
		{
			y = startY - incY;
			for(x = startX - incX; x < (SDWORD)(startX + incX); x++)
			{
				if (sensiblePlace(x,y) AND noDroid(x,y) )
				{
					found = TRUE;
					break;
				}
			}
		}
		if (!found)
		{
			x = startX + incX;
			for(y = startY - incY; y < (SDWORD)(startY + incY); y++)
			{
				if (sensiblePlace(x,y) AND noDroid(x,y) )
				{
					found = TRUE;
					break;
				}
			}
		}
		if (!found)
		{
			y = startY + incY;
			for(x = startX + incX; x > (SDWORD)(startX - incX); x--)
			{
				if (sensiblePlace(x,y) AND noDroid(x,y) )
				{
					found = TRUE;
					break;
				}
			}
		}
		if (!found)
		{
			x = startX - incX;
			for(y = startY + incY; y > (SDWORD)(startY - incY); y--)
			{
				if (sensiblePlace(x,y) AND noDroid(x,y) )
				{
					found = TRUE;
					break;
				}
			}
		}
		if (found)
		{
			break;
		}
	}
	if (!found)
	{
		*pX0 = startX;
		*pY0 = startY;
		return FALSE;
	}
	else
	{
		*pX0 = x;
		*pY0 = y;
		return TRUE;
	}
}
*/

/* Looks through the players list of droids to see if any of them are
building the specified structure - returns TRUE if finds one*/
BOOL checkDroidsBuilding(STRUCTURE *psStructure)
{
	DROID				*psDroid;
	STRUCTURE			*psStruct = NULL;

	for (psDroid = apsDroidLists[psStructure->player]; psDroid != NULL; psDroid =
		psDroid->psNext)
	{
		//check DORDER_BUILD, HELP_BUILD is handled the same
		orderStateObj(psDroid, DORDER_BUILD, (BASE_OBJECT **) &psStruct);
		if (psStruct == psStructure)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* Looks through the players list of droids to see if any of them are
demolishing the specified structure - returns TRUE if finds one*/
BOOL checkDroidsDemolishing(STRUCTURE *psStructure)
{
	DROID				*psDroid;
	STRUCTURE			*psStruct = NULL;

	for (psDroid = apsDroidLists[psStructure->player]; psDroid != NULL; psDroid =
		psDroid->psNext)
	{
		//check DORDER_DEMOLISH
		orderStateObj(psDroid, DORDER_DEMOLISH, (BASE_OBJECT **) &psStruct);
		if (psStruct == psStructure)
		{
			return TRUE;
		}
	}
	return FALSE;
}


/* checks the structure for type and capacity and **NOT orders the droid*** to build
a module if it can - returns TRUE if order is set */
BOOL buildModule(DROID *psDroid, STRUCTURE *psStruct,BOOL bCheckPower)
{
	BOOL	order;
	UDWORD	i=0;

//	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
//		"buildModule: Invalid droid pointer" );
	ASSERT( PTRVALID(psStruct, sizeof(STRUCTURE)),
		"buildModule: Invalid structure pointer" );

	order = FALSE;
	switch (psStruct->pStructureType->type)
	{
	case REF_POWER_GEN:
		//check room for one more!
		if (((POWER_GEN *)psStruct->pFunctionality)->capacity < NUM_POWER_MODULES)
		{
			/*for (i = 0; (i < numStructureStats) && (asStructureStats[i].type !=
				REF_POWER_MODULE);i++)
			{
				//keep looking for the Power Module stat...
			}*/
			i = powerModuleStat;

			order = TRUE;
		}
		break;
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
		//check room for one more!
		if (((FACTORY *)psStruct->pFunctionality)->capacity < NUM_FACTORY_MODULES)
		{
			/*for (i = 0; (i < numStructureStats) && (asStructureStats[i].type !=
				REF_FACTORY_MODULE);i++)
			{
				//keep looking for the Factory Module stat...
			}*/
			i = factoryModuleStat;

			order = TRUE;
		}
		break;
	case REF_RESEARCH:
		//check room for one more!
		if (((RESEARCH_FACILITY *)psStruct->pFunctionality)->capacity <
			NUM_RESEARCH_MODULES)
		{
			/*for (i = 0; (i < numStructureStats) && (asStructureStats[i].type !=
				REF_RESEARCH_MODULE);i++)
			{
				//keep looking for the Research Module stat...
			}*/
			i = researchModuleStat;

			order = TRUE;
		}
		break;
	default:
		//no other structures can have modules attached
		break;
	}

	if (order)
	{
		//check availability of Module
		if (!((i < numStructureStats) &&
			(apStructTypeLists[psDroid->player][i] == AVAILABLE)))
		{
			order = FALSE;
		}

        //Power is obtained gradually now, so allow order
		/*if(bCheckPower)
		{
			// check enough power to build
			if (!checkPower(selectedPlayer, asStructureStats[i].powerToBuild, TRUE))
			{
				order = FALSE;
			}
		}*/
	}

	return order;
}

/*Deals with building a module - checking if any droid is currently doing this
 - if so, helping to build the current one*/
void setUpBuildModule(DROID *psDroid)
{
	UDWORD		tileX, tileY;
	STRUCTURE	*psStruct;

	tileX = psDroid->orderX >> TILE_SHIFT;
	tileY = psDroid->orderY >> TILE_SHIFT;

	//check not another Truck started
	psStruct = getTileStructure(tileX,tileY);
	if (psStruct)
	{
		if (checkDroidsBuilding(psStruct))
		{
			//set up the help build scenario
			psDroid->order = DORDER_HELPBUILD;
			psDroid->psTarget = (BASE_OBJECT *)psStruct;
			if (droidStartBuild(psDroid))
			{
				psDroid->action = DACTION_BUILD;
				intBuildStarted(psDroid);
			}
			else
			{
				psDroid->action = DACTION_NONE;
			}
		}
		else
		{
			if(buildModule(psDroid,psStruct,FALSE))
			{
				//no other droids building so just start it off
				if (droidStartBuild(psDroid))
				{
					psDroid->action = DACTION_BUILD;
					intBuildStarted(psDroid);
				}
				else
				{
					psDroid->action = DACTION_NONE;
				}
			}
			else
			{
				psDroid->action = DACTION_NONE;
			}
		}
	}
	else
	{
		//we've got a problem if it didn't find a structure
		psDroid->action = DACTION_NONE;
	}
}


// not written yet - needs merging with code in Dr Jones' Design.c
void BuildNameFromDroid(DROID *psDroid, STRING *ConstructedName)
{
}


// We just need 1 buffer for the current displayed droid (or template) name
#define MAXCONNAME WIDG_MAXSTR	//(32)
#ifdef HASH_NAMES
static STRING ConstructedName[MAXCONNAME+1]="Body Mk XXIV";	// dummy name
#endif


STRING *getDroidName(DROID *psDroid)
{
	DROID_TEMPLATE sTemplate;

	templateSetParts(psDroid,&sTemplate);

	return getTemplateName(&sTemplate);
}



/*return the name to display for the interface - we don't know if this is
a string ID or something the user types in*/
STRING* getTemplateName(DROID_TEMPLATE *psTemplate)
{
#ifdef HASH_NAMES
	STRING *TempName;

	STRING *NewName;
/*  - Performed in 	GetDefaultTemplateName   - with other droids

	// On the PSX the NameHash entry points to the name for database generated templates
	// - user generated templates are NULL'ed
	// We only want database name if it's a cyborg.
	if(psTemplate->droidType == DROID_CYBORG) {
		if (psTemplate->NameHash!=NULL)
		{
			TempName=(strresGetString(psStringRes, psTemplate->NameHash));
			if (TempName!=NULL)
			{
//				DBPRINTF(("name1 [%s]\n",TempName));
				return(TempName);
			}
		}
	}
*/
	// If the above failed or it's not a cyborg then construct the name from the components.
	NewName=GetDefaultTemplateName(psTemplate);

	if (NewName==NULL || NewName[0]==0)
	{
		// No name ... we assume its the NEW VEHICLE BUTTON
		strcpy(ConstructedName, strresGetString(psStringRes, STR_DES_NEWVEH));
	}
	else
	{
		strncpy(ConstructedName,NewName,MAXCONNAME);
	}
	return(ConstructedName);

#else
	//STRING *pNameID=psTemplate->pName;
	STRING *pNameID = psTemplate->aName;
#ifdef STORE_RESOURCE_ID
	UDWORD			id;
	STRING			*pName = NULL;

	/*see if the name has a resource associated with it by trying to get
	the ID for the string*/
	if (strresGetIDNum(psStringRes, pNameID, &id))
	{
		//get the string from the id
		pName = strresGetString(psStringRes, id);
		if (pName)
		{
			return pName;
		}
	}
	//if haven't found a resource, return the name passed in
	if (!pName)
	{
		return pNameID;
	}

#else
	//just return the name passed in
	return pNameID;
#endif
#endif
	return NULL;
}

/* Just returns true if the droid's present body points aren't as high as the original*/
BOOL	droidIsDamaged(DROID *psDroid)
{
	if(psDroid->body < psDroid->originalBody)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}


BOOL getDroidResourceName(STRING *pName)
{
	UDWORD		id;

	//see if the name has a resource associated with it by trying to get the ID for the string
	if (!strresGetIDNum(psStringRes, pName, &id))
	{
		debug( LOG_ERROR, "Unable to find string resource for %s", pName );
		abort();
		return FALSE;
	}
	//get the string from the id
	strcpy(pName, strresGetString(psStringRes, id));

	return TRUE;
}


/*checks to see if an electronic warfare weapon is attached to the droid*/
BOOL electronicDroid(DROID *psDroid)
{
	DROID	*psCurr;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"electronicUnit: Invalid unit pointer" );

	//if (psDroid->numWeaps AND asWeaponStats[psDroid->asWeaps[0].nStat].
    if (psDroid->asWeaps[0].nStat > 0 AND asWeaponStats[psDroid->asWeaps[0].nStat].
		weaponSubClass == WSC_ELECTRONIC)
	{
		return TRUE;
	}

	if (psDroid->droidType == DROID_COMMAND AND psDroid->psGroup)
	{
		// if a commander has EW units attached it is electronic
		for (psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			if (electronicDroid(psCurr))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*checks to see if the droid is currently being repaired by another*/
BOOL droidUnderRepair(DROID *psDroid)
{
	DROID		*psCurr;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitUnderRepair: Invalid unit pointer" );

	//droid must be damaged
	if (droidIsDamaged(psDroid))
	{
		//look thru the list of players droids to see if any are repairing this droid
		for (psCurr = apsDroidLists[psDroid->player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			//if (psCurr->droidType == DROID_REPAIR AND psCurr->action ==
            if ((psCurr->droidType == DROID_REPAIR OR psCurr->droidType ==
                DROID_CYBORG_REPAIR) AND psCurr->action ==
				DACTION_DROIDREPAIR AND psCurr->psTarget == (BASE_OBJECT *)psDroid)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//count how many Command Droids exist in the world at any one moment
UBYTE checkCommandExist(UBYTE player)
{
	DROID	*psDroid;
	UBYTE	quantity = 0;

	for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if (psDroid->droidType == DROID_COMMAND)
		{
			quantity++;
		}
	}
	return quantity;
}

//access functions for vtols
BOOL vtolDroid(DROID *psDroid)
{
	return ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].
		propulsionType == LIFT) AND (psDroid->droidType != DROID_TRANSPORTER));
}

/*returns TRUE if a VTOL Weapon Droid which has completed all runs*/
BOOL vtolEmpty(DROID *psDroid)
{
	if (!vtolDroid(psDroid))
	{
		return FALSE;
	}
	if (psDroid->droidType != DROID_WEAPON)
	{
		return FALSE;
	}

	if (psDroid->sMove.iAttackRuns >= getNumAttackRuns(psDroid))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// true if a vtol is waiting to be rearmed by a particular rearm pad
BOOL vtolReadyToRearm(DROID *psDroid, STRUCTURE *psStruct)
{
	STRUCTURE	*psRearmPad;

	if (!vtolDroid(psDroid) ||
		psDroid->action != DACTION_WAITFORREARM)
	{
		return FALSE;
	}

	// If a unit has been ordered to rearm make sure it goes to the correct base
	if (orderStateObj(psDroid, DORDER_REARM, (BASE_OBJECT **)&psRearmPad))
	{
		if ((psRearmPad != psStruct) &&
			!vtolOnRearmPad(psRearmPad, psDroid))
		{
			// target rearm pad is clear - let it go there
			return FALSE;
		}
	}

	if (vtolHappy(psDroid) &&
		vtolOnRearmPad(psStruct, psDroid))
	{
		// there is a vtol on the pad and this vtol is already rearmed
		// don't bother shifting the other vtol off
		return FALSE;
	}

	if ((psDroid->psActionTarget != NULL) &&
		(psDroid->psActionTarget->cluster != psStruct->cluster))
	{
		// vtol is rearming at a different base
		return FALSE;
	}

	return TRUE;
}

// true if a vtol droid currently returning to be rearmed
BOOL vtolRearming(DROID *psDroid)
{
	if (!vtolDroid(psDroid))
	{
		return FALSE;
	}
	if (psDroid->droidType != DROID_WEAPON)
	{
		return FALSE;
	}

	if (psDroid->action == DACTION_MOVETOREARM OR
		psDroid->action == DACTION_WAITFORREARM OR
        psDroid->action == DACTION_MOVETOREARMPOINT OR
		psDroid->action == DACTION_WAITDURINGREARM)
	{
		return TRUE;
	}

	return FALSE;
}

// true if a droid is currently attacking
BOOL droidAttacking(DROID *psDroid)
{
    //what about cyborgs?
	//if (psDroid->droidType != DROID_WEAPON)
    if (!(psDroid->droidType == DROID_WEAPON OR psDroid->droidType == DROID_CYBORG OR
        psDroid->droidType == DROID_CYBORG_SUPER))
	{
		return FALSE;
	}

	if (psDroid->action == DACTION_ATTACK ||
		psDroid->action == DACTION_MOVETOATTACK ||
		psDroid->action == DACTION_ROTATETOATTACK ||
		psDroid->action == DACTION_VTOLATTACK ||
		psDroid->action == DACTION_MOVEFIRE)
	{
		return TRUE;
	}

	return FALSE;
}

// see if there are any other vtols attacking the same target
// but still rearming
BOOL allVtolsRearmed(DROID *psDroid)
{
	DROID	*psCurr;
	BOOL	stillRearming;

	// ignore all non vtols
	if (!vtolDroid(psDroid))
	{
		return TRUE;
	}

	stillRearming = FALSE;
	for (psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (vtolRearming(psCurr) &&
			psCurr->order == psDroid->order &&
			psCurr->psTarget == psDroid->psTarget)
		{
			stillRearming = TRUE;
			break;
		}
	}

	return !stillRearming;
}


/*returns a count of the base number of attack runs for the weapon attached to the droid*/
UWORD   getNumAttackRuns(DROID *psDroid)
{
    UWORD   numAttackRuns;

    ASSERT( vtolDroid(psDroid), "numAttackRuns:not a VTOL Droid" );

    /*if weapon attached to the droid is a salvo weapon, then number of shots that
    can be fired = vtolAttackRuns*numRounds */
    if (asWeaponStats[psDroid->asWeaps[0].nStat].reloadTime)
    {
        numAttackRuns = (UWORD)(asWeaponStats[psDroid->asWeaps[0].nStat].numRounds *
            asWeaponStats[psDroid->asWeaps[0].nStat].vtolAttackRuns);
    }
    else
    {
        numAttackRuns = asWeaponStats[psDroid->asWeaps[0].nStat].vtolAttackRuns;
    }

    return numAttackRuns;
}

/*Checks a vtol for being fully armed and fully repaired to see if ready to
leave reArm pad */
BOOL  vtolHappy(DROID *psDroid)
{
	ASSERT( vtolDroid(psDroid), "vtolHappy: not a VTOL droid" );
	ASSERT( psDroid->droidType == DROID_WEAPON, "vtolHappy: not a weapon droid" );

	//check full complement of ammo
	if (psDroid->sMove.iAttackRuns == 0)
	{
		//check fully repaired
		if (psDroid->body == psDroid->originalBody)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*checks if the droid is a VTOL droid and updates the attack runs as required*/
void updateVtolAttackRun(DROID *psDroid)
{
    if (vtolDroid(psDroid))
    {
        psDroid->sMove.iAttackRuns++;
        //quick check doesn't go over limit
        ASSERT( psDroid->sMove.iAttackRuns < UWORD_MAX, "updateVtolAttackRun: too many attack runs" );
    }
}

/*this mends the VTOL when it has been returned to home base whilst on an
offworld mission*/
void mendVtol(DROID *psDroid)
{
	ASSERT( vtolEmpty(psDroid), "mendVtol: droid is not an empty weapon VTOL!" );

	/* set rearm value to no runs made */
	psDroid->sMove.iAttackRuns = 0;
	//reset ammo and lastTimeFired
	psDroid->asWeaps[0].ammo = asWeaponStats[psDroid->
		asWeaps[0].nStat].numRounds;
	psDroid->asWeaps[0].lastFired = 0;
	/* set droid points to max */
	psDroid->body = psDroid->originalBody;
}

//assign rearmPad to the VTOL
void assignVTOLPad(DROID *psNewDroid, STRUCTURE *psReArmPad)
{
    ASSERT( vtolDroid(psNewDroid), "assignVTOLPad: not a vtol droid" );
    ASSERT( psReArmPad->type == OBJ_STRUCTURE &&
			psReArmPad->pStructureType->type == REF_REARM_PAD,
        "assignVTOLPad: not a ReArm Pad" );

    psNewDroid->psBaseStruct = psReArmPad;
}

//don't use this function any more - the droid checks each frame for this to have died
//look through all droids to see if any are associated with the ReArming Pad
/*void releaseVTOLPad(STRUCTURE *psReArmPad)
{
    DROID       *psDroid;

    ASSERT( psReArmPad->pStructureType->type == REF_REARM_PAD,
        "releaseVTOLPad: not a ReArm Pad" );

    //go thru the structure's player's list of droids looking for VTOLs
    for (psDroid = apsDroidLists[psReArmPad->player]; psDroid != NULL; psDroid = psDroid->psNext)
    {
        if (vtolDroid(psDroid))
        {
            //check this droid was assigned to the rearming pad
            if (psDroid->psBaseStruct == psReArmPad)
            {
                //need to find a new rearming pad - get the nearest to this one
                psDroid->psBaseStruct = NULL;
                psDroid->psBaseStruct = findNearestReArmPad(psDroid, psReArmPad, FALSE);
            }
        }
    }
}*/

/*compares the droid sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
BOOL droidSensorDroidWeapon(BASE_OBJECT *psObj, DROID *psDroid)
{
	SENSOR_STATS	*psStats = NULL;

	//first check if the object is a droid or a structure
	if ( (psObj->type != OBJ_DROID) &&
		 (psObj->type != OBJ_STRUCTURE) )
	{
		return FALSE;
	}
	//check same player
	if (psObj->player != psDroid->player)
	{
		return FALSE;
	}
	//check obj is a sensor droid/structure
	switch (psObj->type)
	{
	case OBJ_DROID:
		if (((DROID *)psObj)->droidType != DROID_SENSOR &&
			((DROID *)psObj)->droidType != DROID_COMMAND)
		{
			return FALSE;
		}
		psStats = asSensorStats + ((DROID *)psObj)->asBits[COMP_SENSOR].nStat;
		break;
	case OBJ_STRUCTURE:
		psStats = ((STRUCTURE *)psObj)->pStructureType->pSensor;
		if ((psStats == NULL) ||
			(psStats->location != LOC_TURRET))
		{
			return FALSE;
		}
		break;
	default:
		break;
	}

	//check droid is a weapon droid - or Cyborg!!
	//if (psDroid->droidType != DROID_WEAPON)
    if (!(psDroid->droidType == DROID_WEAPON OR psDroid->droidType ==
        DROID_CYBORG OR psDroid->droidType == DROID_CYBORG_SUPER))
	{
		return FALSE;
	}

	//finally check the right droid/sensor combination
	// check vtol droid with commander
	if ((vtolDroid(psDroid) || !proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
		((DROID *)psObj)->droidType == DROID_COMMAND)
	{
		return TRUE;
	}

	//check vtol droid with vtol sensor
	//if (vtolDroid(psDroid) AND psDroid->numWeaps > 0)
    if (vtolDroid(psDroid) AND psDroid->asWeaps[0].nStat > 0)
	{
		if (psStats->type == VTOL_INTERCEPT_SENSOR OR
			psStats->type == VTOL_CB_SENSOR OR
            psStats->type == SUPER_SENSOR)
		{
			return TRUE;
		}
		return FALSE;
	}

	//check indirect weapon droid with standard/cb sensor
    /*Super Sensor works as any type*/
	if (!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat))
	{
		if (psStats->type == STANDARD_SENSOR OR
			psStats->type == INDIRECT_CB_SENSOR OR
            psStats->type == SUPER_SENSOR)
		{
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// return whether a droid has a CB sensor on it
BOOL cbSensorDroid(DROID *psDroid)
{
	if (psDroid->droidType != DROID_SENSOR)
	{
		return FALSE;
	}

    /*Super Sensor works as any type*/
	if (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		VTOL_CB_SENSOR OR
		asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		INDIRECT_CB_SENSOR OR
		asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		SUPER_SENSOR)
	{
		return TRUE;
	}

	return FALSE;
}

//testing the new electronic warfare for multiPlayer - AB don't want to release with this in the game!!!!!!
//#define TEST_EW 1

// ////////////////////////////////////////////////////////////////////////////
// give a droid from one player to another - used in Electronic Warfare and multiplayer
//returns the droid created - for single player
DROID * giftSingleDroid(DROID *psD, UDWORD to)
{
    DROID_TEMPLATE      sTemplate;
    UWORD               x, y, numKills, direction, i;
    DROID               *psNewDroid, *psCurr;
    STRUCTURE           *psStruct;
    UDWORD              body, armourK, armourH;

    //leave any group it belongs to  - this gets called in droidRemove()
	/*if(psD->psGroup)
	{
		grpLeave( psD->psGroup, psD);
		psD->psGroup = NULL;
	}*/
	// remove the droid from the cluster systerm - this gets called in droidRemove()
	//clustRemoveObject((BASE_OBJECT *)psD);

	if(psD->player == to)
	{
		return psD;
	}

#ifdef TEST_EW
    bMultiPlayer = TRUE;
#endif


    if (bMultiPlayer)
    {
        //reset order
	    orderDroid(psD, DORDER_STOP);

	    if (droidRemove(psD,apsDroidLists)) 		// remove droid from one list
        {
			if(!isHumanPlayer(psD->player))
			{
				droidSetName(psD,"Enemy Unit");
			}


            //if successfully removed the droid from the players list add it to new player's list
            psD->selected = FALSE;
	        psD->player	  = (UBYTE)to;					// move droid

	        addDroid(psD, apsDroidLists);	// add to other list.

            //the new player may have different default sensor/ecm/repair components
            if ((asSensorStats + psD->asBits[COMP_SENSOR].nStat)->location == LOC_DEFAULT)
            {
                if (psD->asBits[COMP_SENSOR].nStat != aDefaultSensor[psD->player])
                {
                    psD->asBits[COMP_SENSOR].nStat = (UBYTE)aDefaultSensor[psD->player];
                }
            }
            if ((asECMStats + psD->asBits[COMP_ECM].nStat)->location == LOC_DEFAULT)
            {
                if (psD->asBits[COMP_ECM].nStat != aDefaultECM[psD->player])
                {
                    psD->asBits[COMP_ECM].nStat = (UBYTE)aDefaultECM[psD->player];
                }
            }
            if ((asRepairStats + psD->asBits[COMP_REPAIRUNIT].nStat)->location == LOC_DEFAULT)
            {
                if (psD->asBits[COMP_REPAIRUNIT].nStat != aDefaultRepair[psD->player])
                {
                    psD->asBits[COMP_REPAIRUNIT].nStat = (UBYTE)aDefaultRepair[psD->player];
                }
            }
        }
        //add back into cluster system
        clustNewDroid(psD);

        //add back into the grid system
        gridAddObject((BASE_OBJECT *)psD);

        //check through the 'to' players list of droids to see if any are targetting it
        for (psCurr = apsDroidLists[to]; psCurr != NULL; psCurr = psCurr->psNext)
        {
            if (psCurr->psTarget == (BASE_OBJECT *)psD OR
                psCurr->psActionTarget == (BASE_OBJECT *)psD)
            {
                orderDroid(psCurr, DORDER_STOP);
            }
            //check through order list
            for (i = 0; i < psCurr->listSize; i++)
            {
                if (psCurr->asOrderList[i].psOrderTarget == (BASE_OBJECT *)psD)
                {
            		// move the rest of the list down
		            memmove(&psCurr->asOrderList[i], &psCurr->asOrderList[i] + 1,
                        (psCurr->listSize - i) * sizeof(ORDER_LIST));
                    //adjust list size
                    psCurr->listSize -= 1;
                    //initialise the empty last slot
		            memset(psCurr->asOrderList + psCurr->listSize, 0,
                        sizeof(ORDER_LIST));
                }
            }
        }
        //check through the 'to' players list of structures to see if any are targetting it
        for (psStruct = apsStructLists[to]; psStruct != NULL; psStruct =
            psStruct->psNext)
        {
            if (psStruct->psTarget == (BASE_OBJECT *)psD)
            {
                psStruct->psTarget = NULL;
            }
        }

		// skirmish callback!
		psScrCBDroidTaken = psD;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_UNITTAKEOVER);
		psScrCBDroidTaken = NULL;

#ifdef TEST_EW
        bMultiPlayer = FALSE;
#endif
        return NULL;
    }

    {
        //got to destroy the droid and build another since there are too many complications re order/action!

        //create a template based on the droid
        templateSetParts(psD, &sTemplate);

        //copy the name across
        strcpy(sTemplate.aName, psD->aName);

        x = psD->x;
        y = psD->y;
        body = psD->body;
        armourK = psD->armour[WC_KINETIC];
        armourH = psD->armour[WC_HEAT];
        numKills = psD->numKills;
        direction = psD->direction;
        //only play the sound if unit being taken over is selectedPlayer's but not going to the selectedPlayer
        //if ((psD->player == selectedPlayer) &&
		//	(psD->player != to))
        if ((psD->player == selectedPlayer) AND (to != selectedPlayer))
        {
            scoreUpdateVar(WD_UNITS_LOST);

	        audio_QueueTrackPos( ID_SOUND_NEXUS_UNIT_ABSORBED, x, y, psD->z );

        }
        //make the old droid vanish
        vanishDroid(psD);
        //create a new droid
        psNewDroid = buildDroid(&sTemplate, x, y, to, FALSE);
        if (psNewDroid)
        {
            addDroid(psNewDroid, apsDroidLists);
            psNewDroid->body = body;
            psNewDroid->armour[WC_KINETIC] = armourK;
            psNewDroid->armour[WC_HEAT] = armourH;
            psNewDroid->numKills = numKills;
            psNewDroid->direction = direction;
    		if(!(psNewDroid->droidType == DROID_PERSON OR
                //psNewDroid->droidType == DROID_CYBORG OR
                cyborgDroid(psNewDroid) OR
                psNewDroid->droidType == DROID_TRANSPORTER))
	    	{
		    	updateDroidOrientation(psNewDroid);
		    }
        }
        else
        {
            ASSERT( FALSE, "giftSingleUnit: unable to build a unit" );
        }
        return psNewDroid;
    }
}

/*calculates the electronic resistance of a droid based on its experience level*/
SWORD   droidResistance(DROID *psDroid)
{
    SWORD   resistance;

    resistance = (SWORD)(psDroid->numKills * DROID_RESISTANCE_FACTOR);

    //ensure base minimum in MP before the upgrade effect
    if (bMultiPlayer)
    {
        //ensure resistance is a base minimum
        if (resistance < DROID_RESISTANCE_FACTOR)
        {
            resistance = DROID_RESISTANCE_FACTOR;
        }
    }

    //structure resistance upgrades are passed on to droids
    resistance = (SWORD)(resistance  + resistance * (
        asStructureUpgrade[psDroid->player].resistance/100));

    //ensure resistance is a base minimum
    if (resistance < DROID_RESISTANCE_FACTOR)
    {
        resistance = DROID_RESISTANCE_FACTOR;
    }

    return resistance;
}

/*this is called to check the weapon is 'allowed'. Check if VTOL, the weapon is
direct fire. Also check numVTOLattackRuns for the weapon is not zero - return
TRUE if valid weapon*/
BOOL checkValidWeaponForProp(DROID_TEMPLATE *psTemplate)
{
	PROPULSION_STATS	*psPropStats;
	BOOL				bValid;

	bValid = TRUE;
	//check propulsion stat for vtol
	psPropStats = asPropulsionStats + psTemplate->asParts[COMP_PROPULSION];
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
		"checkValidWeaponForProp: invalid propulsion stats pointer" );
	if (asPropulsionTypes[psPropStats->propulsionType].travel == AIR)
	{
		//check weapon stat for indirect
		//if (!(asWeaponStats + sCurrDesign.asWeaps[0])->direct)
		if (!proj_Direct(asWeaponStats + psTemplate->asWeaps[0]) OR
            !asWeaponStats[psTemplate->asWeaps[0]].vtolAttackRuns)
		{
			bValid = FALSE;
		}
	}
	else
	{
		if ( asWeaponStats[psTemplate->asWeaps[0]].vtolAttackRuns )
		{
			bValid = FALSE;
		}
	}

	//also checks that there is only a weapon attached and no other system component
	if (psTemplate->numWeaps == 0)
	{
		bValid = FALSE;
	}
	if ( (psTemplate->asParts[COMP_BRAIN] != 0) &&
		 (psTemplate->asParts[COMP_WEAPON] != 0)   )
	{
		bValid = FALSE;
	}

	return bValid;
}

/*called when a Template is deleted in the Design screen*/
void deleteTemplateFromProduction(DROID_TEMPLATE *psTemplate, UBYTE player)
{
    STRUCTURE   *psStruct;
    UDWORD      inc, i;
	STRUCTURE	*psList;

    //see if any factory is currently using the template
	for (i=0; i<2; i++)
	{
		psList = NULL;
		switch (i)
		{
		case 0:
			psList = apsStructLists[player];
			break;
		case 1:
			psList = mission.apsStructLists[player];
			break;
		}
		for (psStruct = psList; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (StructIsFactory(psStruct))
			{
				FACTORY             *psFactory = (FACTORY *)psStruct->pFunctionality;
				DROID_TEMPLATE      *psNextTemplate = NULL;

				//if template belongs to the production player - check thru the production list (if struct is busy)
				if (player == productionPlayer AND psFactory->psSubject)
				{
					for (inc = 0; inc < MAX_PROD_RUN; inc++)
					{
						if (asProductionRun[psFactory->psAssemblyPoint->factoryType][
	    					psFactory->psAssemblyPoint->factoryInc][inc].psTemplate == psTemplate)
						{
							//if this is the template currently being worked on
							if (psTemplate == (DROID_TEMPLATE *)psFactory->psSubject)
							{
								//set the quantity to 1 and then use factoryProdAdjust to subtract it
								asProductionRun[psFactory->psAssemblyPoint->factoryType][
	    							psFactory->psAssemblyPoint->factoryInc][inc].quantity = 1;
								factoryProdAdjust(psStruct, psTemplate, FALSE);
								//init the factory production
								psFactory->psSubject = NULL;
								//check to see if anything left to produce
								psNextTemplate = factoryProdUpdate(psStruct, NULL);
								//power is returned by factoryProdAdjust()
								if (psNextTemplate)
								{
									structSetManufacture(psStruct, psNextTemplate,psFactory->quantity);
								}
								else
								{
									//nothing more to manufacture - reset the Subject and Tab on HCI Form
									intManufactureFinished(psStruct);
									//power is returned by factoryProdAdjust()
								}
							}
							else
							{
								//just need to initialise this production run
								asProductionRun[psFactory->psAssemblyPoint->factoryType][
	    							psFactory->psAssemblyPoint->factoryInc][inc].psTemplate = NULL;
								asProductionRun[psFactory->psAssemblyPoint->factoryType][
	    							psFactory->psAssemblyPoint->factoryInc][inc].quantity = 0;
								asProductionRun[psFactory->psAssemblyPoint->factoryType][
	    							psFactory->psAssemblyPoint->factoryInc][inc].built = 0;
							}
						}
					}
				}
				else
				{
					//not the production player, so check not being built in the factory for the template player
					if (psFactory->psSubject == (BASE_STATS *)psTemplate)
					{
						//clear the factories subject and quantity
						psFactory->psSubject = NULL;
						psFactory->quantity = 0;
						//return any accrued power
						if (psFactory->powerAccrued)
						{
							addPower(psStruct->player, psFactory->powerAccrued);
						}
						//tell the interface
						intManufactureFinished(psStruct);
					}
				}
			}
		}
	}
}


// Select a droid and do any necessary housekeeping.
//
void SelectDroid(DROID *psDroid)
{
	psDroid->selected = TRUE;
	intRefreshScreen();
}


// De-select a droid and do any necessary housekeeping.
//
void DeSelectDroid(DROID *psDroid)
{
	psDroid->selected = FALSE;
	intRefreshScreen();
}

/*calculate the power cost to repair a droid*/
UWORD powerReqForDroidRepair(DROID *psDroid)
{
    UWORD   powerReq;//powerPercent;

    powerReq = (UWORD)(repairPowerPoint(psDroid) * (psDroid->originalBody - psDroid->body));

    //powerPercent = (UWORD)(100 - PERCENT(psDroid->body, psDroid->originalBody));
    //by including POWER_FACTOR don't have to worry about rounding errors
	//powerReq = (UWORD)((POWER_FACTOR * powerPercent * calcDroidPower(psDroid) *
    //    REPAIR_POWER_FACTOR) / 100);

    return powerReq;
}

/*power cost for One repair point*/
UWORD repairPowerPoint(DROID *psDroid)
{
    return (UWORD)(((POWER_FACTOR * calcDroidPower(psDroid)) / psDroid->originalBody) *
        REPAIR_POWER_FACTOR);
}


BOOL droidAudioTrackStopped( AUDIO_SAMPLE *psSample )
{
	DROID	*psDroid;

	ASSERT( PTRVALID(psSample, sizeof(AUDIO_SAMPLE)),
		"unitAudioTrackStopped: audio sample pointer invalid\n" );

	if ( psSample->psObj != NULL )
	{
		psDroid = psSample->psObj;

        if ( psDroid->type == OBJ_DROID && !psDroid->died )
		{
            ASSERT( PTRVALID(psDroid, sizeof(DROID)),
                    "unitAudioTrackStopped: unit pointer invalid\n" );
			psDroid->iAudioID = NO_SOUND;
		}
	}

	return TRUE;
}


/*returns TRUE if droid type is one of the Cyborg types*/
BOOL cyborgDroid(DROID *psDroid)
{
    if ( psDroid->droidType == DROID_CYBORG OR
         psDroid->droidType == DROID_CYBORG_CONSTRUCT OR
         psDroid->droidType == DROID_CYBORG_REPAIR OR
         psDroid->droidType == DROID_CYBORG_SUPER        )
    {
        return TRUE;
    }

    return FALSE;
}
