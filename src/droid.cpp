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
/**
 * @file droid.c
 *
 * Droid method functions.
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/geometry.h"
#include "lib/framework/strres.h"

#include "lib/gamelib/gtime.h"
#include "lib/gamelib/animobj.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/framework/fixedpoint.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/netplay/netplay.h"

#include "objects.h"
#include "loop.h"
#include "visibility.h"
#include "map.h"
#include "drive.h"
#include "droid.h"
#include "hci.h"
#include "game.h"
#include "power.h"
#include "miscimd.h"
#include "effects.h"
#include "feature.h"
#include "action.h"
#include "order.h"
#include "move.h"
#include "anim_id.h"
#include "geometry.h"
#include "display.h"
#include "console.h"
#include "component.h"
#include "function.h"
#include "lighting.h"
#include "multiplay.h"
#include "warcam.h"
#include "display3d.h"
#include "group.h"
#include "text.h"
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
#include "combat.h"
#include "scriptfuncs.h"			//for ThreatInRange()
#include "template.h"
#include "qtscript.h"

#define DEFAULT_RECOIL_TIME	(GAME_TICKS_PER_SEC/4)
#define	DROID_DAMAGE_SPREAD	(16 - rand()%32)
#define	DROID_REPAIR_SPREAD	(20 - rand()%40)

// store the experience of recently recycled droids
UWORD	aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];
UDWORD	selectedGroup = UBYTE_MAX;
UDWORD	selectedCommander = UBYTE_MAX;

/* default droid design template */
extern DROID_TEMPLATE	sDefaultDesignTemplate;

/** Height the transporter hovers at above the terrain. */
#define TRANSPORTER_HOVER_HEIGHT	10

// the structure that was last hit
DROID	*psLastDroidHit;

//determines the best IMD to draw for the droid - A TEMP MEASURE!
void	groupConsoleInformOfSelection( UDWORD groupNumber );
void	groupConsoleInformOfCreation( UDWORD groupNumber );
void	groupConsoleInformOfCentering( UDWORD groupNumber );

void cancelBuild(DROID *psDroid)
{
	if (orderDroidList(psDroid))
	{
		objTrace(psDroid->id, "Droid build order cancelled - changing to next order");
	}
	else
	{
		objTrace(psDroid->id, "Droid build order cancelled");
		psDroid->action = DACTION_NONE;
		psDroid->order = DroidOrder(DORDER_NONE);
		setDroidActionTarget(psDroid, NULL, 0);

		/* Notify scripts we just became idle */
		psScrCBOrderDroid = psDroid;
		psScrCBOrder = psDroid->order.type;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_REACH_LOCATION);
		psScrCBOrderDroid = NULL;
		psScrCBOrder = DORDER_NONE;

		triggerEventDroidIdle(psDroid);
	}
}

// initialise droid module
bool droidInit(void)
{
	memset(aDroidExperience, 0, sizeof(UWORD) * MAX_PLAYERS * MAX_RECYCLED_DROIDS);
	psLastDroidHit = NULL;

	return true;
}


#define UNIT_LOST_DELAY	(5*GAME_TICKS_PER_SEC)
/* Deals damage to a droid
 * \param psDroid droid to deal damage to
 * \param damage amount of damage to deal
 * \param weaponClass the class of the weapon that deals the damage
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \param angle angle of impact (from the damage dealing projectile in relation to this droid)
 * \return > 0 when the dealt damage destroys the droid, < 0 when the droid survives
 *
 * NOTE: This function will damage but _never_ destroy transports when in single player (campaign) mode
 */
int32_t droidDamage(DROID *psDroid, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime)
{
	int32_t relativeDamage;

	CHECK_DROID(psDroid);

	// VTOLs on the ground take triple damage
	if (isVtolDroid(psDroid) && psDroid->sMove.Status == MOVEINACTIVE)
	{
		damage *= 3;
	}

	relativeDamage = objDamage((BASE_OBJECT *)psDroid, damage, psDroid->originalBody, weaponClass, weaponSubClass);

	if (relativeDamage > 0)
	{
		// reset the attack level
		if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ATTACKED)
		{
			secondarySetState(psDroid, DSO_ATTACK_LEVEL, DSS_ALEV_ALWAYS);
		}
		// Now check for auto return on droid's secondary orders (i.e. return on medium/heavy damage)
		secondaryCheckDamageLevel(psDroid);

		if (!bMultiPlayer)
		{
			// Now check for scripted run-away based on health left
			orderHealthCheck(psDroid);
		}

		CHECK_DROID(psDroid);
	}
	else if (relativeDamage < 0)
	{
		// HACK: Prevent transporters from being destroyed in single player
		if ( (game.type == CAMPAIGN) && !bMultiPlayer && (psDroid->droidType == DROID_TRANSPORTER) )
		{
			debug(LOG_ATTACK, "Transport(%d) saved from death--since it should never die (SP only)", psDroid->id);
			psDroid->body = 1;
			return 0;
		}

		// Droid destroyed
		debug(LOG_ATTACK, "droid (%d): DESTROYED", psDroid->id);

		// Deal with score increase/decrease and messages to the player
		if( psDroid->player == selectedPlayer)
		{
			CONPRINTF(ConsoleString,(ConsoleString, _("Unit Lost!")));
			scoreUpdateVar(WD_UNITS_LOST);
			audio_QueueTrackMinDelayPos(ID_SOUND_UNIT_DESTROYED,UNIT_LOST_DELAY,
										psDroid->pos.x, psDroid->pos.y, psDroid->pos.z );
		}
		else
		{
			scoreUpdateVar(WD_UNITS_KILLED);
		}

		// If this is droid is a person and was destroyed by flames,
		// show it nicely by burning him/her to death.
		if (psDroid->droidType == DROID_PERSON && weaponClass == WC_HEAT)
		{
			droidBurn(psDroid);
		}
		// Otherwise use the default destruction animation
		else
		{
			debug(LOG_DEATH, "Droid %d (%p) is toast", (int)psDroid->id, psDroid);
			// This should be sent even if multi messages are turned off, as the group message that was
			// sent won't contain the destroyed droid
			if (bMultiPlayer && !bMultiMessages)
			{
				bMultiMessages = true;
				destroyDroid(psDroid, impactTime);
				bMultiMessages = false;
			}
			else
			{
				destroyDroid(psDroid, impactTime);
			}
		}
	}

	return relativeDamage;
}

// Check that psVictimDroid is not referred to by any other object in the game. We can dump out some
// extra data in debug builds that help track down sources of dangling pointer errors.
#ifdef DEBUG
#define DROIDREF(func, line) "Illegal reference to droid from %s line %d", func, line
#else
#define DROIDREF(func, line) "Illegal reference to droid"
#endif
bool droidCheckReferences(DROID *psVictimDroid)
{
	int plr;

	for (plr = 0; plr < MAX_PLAYERS; plr++)
	{
		STRUCTURE *psStruct;
		DROID *psDroid;

		for (psStruct = apsStructLists[plr]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			unsigned int i;

			for (i = 0; i < psStruct->numWeaps; i++)
			{
				ASSERT_OR_RETURN(false, (DROID *)psStruct->psTarget[i] != psVictimDroid, DROIDREF(psStruct->targetFunc[i], psStruct->targetLine[i]));
			}
		}
		for (psDroid = apsDroidLists[plr]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			unsigned int i;

			ASSERT_OR_RETURN(false, psDroid->order.psObj != psVictimDroid || psVictimDroid == psDroid, DROIDREF(psDroid->targetFunc, psDroid->targetLine));
			for (i = 0; i < psDroid->numWeaps; i++)
			{
				ASSERT_OR_RETURN(false, (DROID *)psDroid->psActionTarget[i] != psVictimDroid || psVictimDroid == psDroid, 
				                 DROIDREF(psDroid->actionTargetFunc[i], psDroid->actionTargetLine[i]));
			}
		}
	}
	return true;
}
#undef DROIDREF

DROID::DROID(uint32_t id, unsigned player)
	: BASE_OBJECT(OBJ_DROID, id, player)
	, droidType(DROID_ANY)
	, psGroup(NULL)
	, psGrpNext(NULL)
	, secondaryOrder(DSS_ARANGE_DEFAULT | DSS_REPLEV_NEVER | DSS_ALEV_ALWAYS | DSS_HALT_GUARD)
	, action(DACTION_NONE)
	, actionPos(0, 0)
	, psCurAnim(NULL)
	, gameCheckDroid(NULL)
{
	order.type = DORDER_NONE;
	order.pos = Vector2i(0, 0);
	order.pos2 = Vector2i(0, 0);
	order.direction = 0;
	order.psObj = NULL;
	order.psStats = NULL;

	sMove.asPath = NULL;
	sMove.Status = MOVEINACTIVE;
}

/* DROID::~DROID: release all resources associated with a droid -
 * should only be called by objmem - use vanishDroid preferably
 */
DROID::~DROID()
{
	DROID *psDroid = this;
	DROID	*psCurr, *psNext;

	/* remove animation if present */
	if (psDroid->psCurAnim != NULL)
	{
		animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID);
		psDroid->psCurAnim = NULL;
	}

	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		if (psDroid->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDroid->psGroup->psList; psCurr != NULL && psCurr !=
				psDroid; psCurr = psNext)
			{
				psNext = psCurr->psGrpNext;
				delete psCurr;
			}
		}
	}

	fpathRemoveDroidData(psDroid->id);

	// leave the current group if any
	if (psDroid->psGroup)
	{
		psDroid->psGroup->remove(psDroid);
	}

	// remove the droid from the cluster system
	clustRemoveObject((BASE_OBJECT *)psDroid);

	free(sMove.asPath);

	delete gameCheckDroid;
}


// recycle a droid (retain it's experience and some of it's cost)
void recycleDroid(DROID *psDroid)
{
	UDWORD		numKills, minKills;
	SDWORD		i, cost, storeIndex;
	Vector3i position;

	CHECK_DROID(psDroid);

	// store the droids kills
	numKills = psDroid->experience/65536;
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
		psDroid->psGroup->remove(psDroid);
	}

	position.x = psDroid->pos.x;				// Add an effect
	position.z = psDroid->pos.y;
	position.y = psDroid->pos.z;

	vanishDroid(psDroid);

	addEffect(&position, EFFECT_EXPLOSION, EXPLOSION_TYPE_DISCOVERY, false, NULL, false, gameTime - deltaGameTime);

	CHECK_DROID(psDroid);
}


void	removeDroidBase(DROID *psDel)
{
	DROID	*psCurr, *psNext;
	STRUCTURE	*psStruct;

	CHECK_DROID(psDel);

	if (isDead((BASE_OBJECT *)psDel))
	{
		// droid has already been killed, quit
		return;
	}

	syncDebugDroid(psDel, '#');

	//ajl, inform others of destruction.
	// Everyone else should be doing this at the same time, assuming it's in synch (so everyone sends a GAME_DROIDDEST message at once)...
	if (!isInSync() && bMultiMessages
	 && !(psDel->player != selectedPlayer && psDel->order.type == DORDER_RECYCLE))
	{
		ASSERT_OR_RETURN( , droidOnMap(psDel), "Asking other players to destroy droid driving off the map");
		SendDestroyDroid(psDel);
	}

	/* remove animation if present */
	if (psDel->psCurAnim != NULL)
	{
		const bool bRet = animObj_Remove(psDel->psCurAnim, psDel->psCurAnim->psAnim->uwID);
		ASSERT(bRet, "animObj_Remove failed");
		psDel->psCurAnim = NULL;
	}

	//kill all the droids inside the transporter
	if (psDel->droidType == DROID_TRANSPORTER)
	{
		if (psDel->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDel->psGroup->psList; psCurr != NULL && psCurr !=
				psDel; psCurr = psNext)
			{
				psNext = psCurr->psGrpNext;

				/* add droid to droid list then vanish it - hope this works! - GJ */
				addDroid(psCurr, apsDroidLists);
				vanishDroid(psCurr);
			}
		}
	}

	if (!bMultiPlayer)  // The moral(e?) checks don't seem like something that belongs in real games.
	{
		// check moral
		if (psDel->psGroup && psDel->psGroup->refCount > 1 && !bMultiPlayer)
		{
			DROID_GROUP *group = psDel->psGroup;
			psDel->psGroup->remove(psDel);
			psDel->psGroup = NULL;
			orderGroupMoralCheck(group);
		}
		else
		{
			orderMoralCheck(psDel->player);
		}
	}

	// leave the current group if any
	if (psDel->psGroup)
	{
		psDel->psGroup->remove(psDel);
		psDel->psGroup = NULL;
	}

	/* Put Deliv. Pts back into world when a command droid dies */
	if (psDel->droidType == DROID_COMMAND)
	{
		for (psStruct = apsStructLists[psDel->player]; psStruct; psStruct=psStruct->psNext)
		{
			// alexl's stab at a right answer.
			if (StructIsFactory(psStruct)
			 && psStruct->pFunctionality->factory.psCommander == psDel)
			{
				assignFactoryCommandDroid(psStruct, NULL);
			}
		}
	}

	// Check to see if constructor droid currently trying to find a location to build
	if (psDel->player == selectedPlayer && psDel->selected && isConstructionDroid(psDel))
	{
		// If currently trying to build, kill off the placement
		if (tryingToGetLocation())
		{
			int numSelectedConstructors = 0;
			for (DROID *psDroid = apsDroidLists[psDel->player]; psDroid != NULL; psDroid = psDroid->psNext)
			{
				numSelectedConstructors += psDroid->selected && isConstructionDroid(psDroid);
			}
			if (numSelectedConstructors <= 1)  // If we were the last selected construction droid.
			{
				kill3DBuilding();
			}
		}
	}

	// remove the droid from the cluster systerm
	clustRemoveObject((BASE_OBJECT *)psDel);

	if (psDel->player == selectedPlayer)
	{
		intRefreshScreen();
	}

	killDroid(psDel);
}

static void removeDroidFX(DROID *psDel, unsigned impactTime)
{
	Vector3i pos;

	CHECK_DROID(psDel);

	// only display anything if the droid is visible
	if (!psDel->visible[selectedPlayer])
	{
		return;
	}

	if (psDel->droidType == DROID_PERSON && psDel->order.type != DORDER_RUNBURN)
	{
		/* blow person up into blood and guts */
		compPersonToBits(psDel);
	}

	/* if baba and not running (on fire) then squish */
	if (psDel->droidType == DROID_PERSON
	 && psDel->order.type != DORDER_RUNBURN
	 && psDel->visible[selectedPlayer])
	{
		// The babarian has been run over ...
		audio_PlayStaticTrack(psDel->pos.x, psDel->pos.y, ID_SOUND_BARB_SQUISH);
	}
	else if (psDel->visible[selectedPlayer])
	{
		destroyFXDroid(psDel, impactTime);
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;
		pos.y = psDel->pos.z;
		addEffect(&pos, EFFECT_DESTRUCTION, DESTRUCTION_TYPE_DROID, false, NULL, 0, impactTime);
		audio_PlayStaticTrack( psDel->pos.x, psDel->pos.y, ID_SOUND_EXPLOSION );
	}
}

void destroyDroid(DROID *psDel, unsigned impactTime)
{
	if (psDel->lastHitWeapon == WSC_LAS_SAT)		// darken tile if lassat.
	{
		UDWORD width, breadth, mapX, mapY;
		MAPTILE	*psTile;

		mapX = map_coord(psDel->pos.x);
		mapY = map_coord(psDel->pos.y);
		for (width = mapX-1; width <= mapX+1; width++)
		{
			for (breadth = mapY-1; breadth <= mapY+1; breadth++)
			{
				psTile = mapTile(width,breadth);
				if(TEST_TILE_VISIBLE(selectedPlayer, psTile))
				{
					psTile->illumination /= 2;
				}
			}
		}
	}

	removeDroidFX(psDel, impactTime);
	removeDroidBase(psDel);
	psDel->died = impactTime;
	return;
}

void	vanishDroid(DROID *psDel)
{
	removeDroidBase(psDel);
}

/* Remove a droid from the List so doesn't update or get drawn etc
TAKE CARE with removeDroid() - usually want droidRemove since it deal with cluster and grid code*/
//returns false if the droid wasn't removed - because it died!
bool droidRemove(DROID *psDroid, DROID *pList[MAX_PLAYERS])
{
	CHECK_DROID(psDroid);

	driveDroidKilled(psDroid);	// Tell the driver system it's gone.

	if (isDead((BASE_OBJECT *) psDroid))
	{
		// droid has already been killed, quit
		return false;
	}

	// leave the current group if any - not if its a Transporter droid
	if (psDroid->droidType != DROID_TRANSPORTER && psDroid->psGroup)
	{
		psDroid->psGroup->remove(psDroid);
		psDroid->psGroup = NULL;
	}

	// reset the baseStruct
	setDroidBase(psDroid, NULL);

	// remove the droid from the cluster systerm
	clustRemoveObject((BASE_OBJECT *)psDroid);

	removeDroid(psDroid, pList);

	if (psDroid->player == selectedPlayer)
	{
		intRefreshScreen();
	}

	return true;
}

static void droidFlameFallCallback( ANIM_OBJECT * psObj )
{
	DROID	*psDroid;

	ASSERT_OR_RETURN( , psObj != NULL, "invalid anim object pointer");
	psDroid = (DROID *) psObj->psParent;

	ASSERT_OR_RETURN( , psDroid != NULL, "invalid Unit pointer");
	psDroid->psCurAnim = NULL;

	// This breaks synch, obviously. Animations are not synched, so changing game state as part of an animation is not completely ideal.
	//debug(LOG_DEATH, "droidFlameFallCallback: Droid %d destroyed", (int)psDroid->id);
	//destroyDroid( psDroid );
}

static void droidBurntCallback( ANIM_OBJECT * psObj )
{
	DROID	*psDroid;

	ASSERT_OR_RETURN( , psObj != NULL, "invalid anim object pointer");
	psDroid = (DROID *) psObj->psParent;

	ASSERT_OR_RETURN( , psDroid != NULL, "invalid Unit pointer");

	/* add falling anim */
	psDroid->psCurAnim = animObj_Add((BASE_OBJECT *)psDroid, ID_ANIM_DROIDFLAMEFALL, 0, 1);
	if (!psDroid->psCurAnim)
	{
		debug( LOG_ERROR, "couldn't add fall over anim");
		return;
	}

	animObj_SetDoneFunc( psDroid->psCurAnim, droidFlameFallCallback );
}

void droidBurn(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	if ( psDroid->droidType != DROID_PERSON )
	{
		ASSERT(LOG_ERROR, "can't burn anything except babarians currently!");
		return;
	}

	if (psDroid->order.type != DORDER_RUNBURN)
	{
		/* set droid running */
		orderDroid(psDroid, DORDER_RUNBURN, ModeImmediate);
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
			const bool bRet = animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID);
			ASSERT(bRet, "animObj_Remove failed");
			psDroid->psCurAnim = NULL;
		}
	}

	/* add burning anim */
	psDroid->psCurAnim = animObj_Add( (BASE_OBJECT *) psDroid,
											ID_ANIM_DROIDBURN, 0, 3 );
	if ( psDroid->psCurAnim == NULL )
	{
		debug( LOG_ERROR, "couldn't add burn anim" );
		return;
	}

	/* set callback */
	animObj_SetDoneFunc( psDroid->psCurAnim, droidBurntCallback );

	/* add scream */
	debug( LOG_NEVER, "baba burn" );
	// NOTE: 3 types of screams are available ID_SOUND_BARB_SCREAM - ID_SOUND_BARB_SCREAM3
	audio_PlayObjDynamicTrack( psDroid, ID_SOUND_BARB_SCREAM+(rand()%3), NULL );
}

void _syncDebugDroid(const char *function, DROID const *psDroid, char ch)
{
	int list[] =
	{
		ch,

		psDroid->id,

		psDroid->player,
		psDroid->pos.x, psDroid->pos.y, psDroid->pos.z,
		psDroid->rot.direction, psDroid->rot.pitch, psDroid->rot.roll,
		psDroid->order.type, psDroid->order.pos.x, psDroid->order.pos.y, psDroid->listSize,
		psDroid->action,
		psDroid->secondaryOrder,
		psDroid->body,
		psDroid->sMove.Status,
		psDroid->sMove.speed, psDroid->sMove.moveDir,
		psDroid->sMove.pathIndex, psDroid->sMove.numPoints,
		psDroid->sMove.src.x, psDroid->sMove.src.y, psDroid->sMove.target.x, psDroid->sMove.target.y, psDroid->sMove.destination.x, psDroid->sMove.destination.y,
		psDroid->sMove.bumpDir, psDroid->sMove.bumpTime, psDroid->sMove.lastBump, psDroid->sMove.pauseTime, psDroid->sMove.bumpX, psDroid->sMove.bumpY, psDroid->sMove.shuffleStart,
		psDroid->experience,
	};
	_syncDebugIntList(function, "%c droid%d = p%d;pos(%d,%d,%d),rot(%d,%d,%d),order%d(%d,%d)^%d,action%d,secondaryOrder%X,body%d,sMove(status%d,speed%d,moveDir%d,path%d/%d,src(%d,%d),target(%d,%d),destination(%d,%d),bump(%d,%d,%d,%d,(%d,%d),%d)),exp%u", list, ARRAY_SIZE(list));
}

/* The main update routine for all droids */
void droidUpdate(DROID *psDroid)
{
	Vector3i        dv;
	UDWORD          percentDamage, emissionInterval;
	BASE_OBJECT     *psBeingTargetted = NULL;
	SDWORD          damageToDo;
	unsigned        i;

	CHECK_DROID(psDroid);

#ifdef DEBUG
	// Check that we are (still) in the sensor list
	if (psDroid->droidType == DROID_SENSOR)
	{
		BASE_OBJECT	*psSensor;

		for (psSensor = apsSensorList[0]; psSensor; psSensor = psSensor->psNextFunc)
		{
			if (psSensor == (BASE_OBJECT *)psDroid) break;
		}
		ASSERT(psSensor == (BASE_OBJECT *)psDroid, "%s(%p) not in sensor list!", 
		       droidGetName(psDroid), psDroid);
	}
#endif

	syncDebugDroid(psDroid, '<');

	// Save old droid position, update time.
	psDroid->prevSpacetime = getSpacetime(psDroid);
	psDroid->time = gameTime;
	for (i = 0; i < MAX(1, psDroid->numWeaps); ++i)
	{
		psDroid->asWeaps[i].prevRot = psDroid->asWeaps[i].rot;
	}

	// update the cluster of the droid
	if ((psDroid->id + gameTime)/2000 != (psDroid->id + gameTime - deltaGameTime)/2000)
	{
		clustUpdateObject((BASE_OBJECT *)psDroid);
	}

	// ai update droid
	aiUpdateDroid(psDroid);

	// Update the droids order. The droid may be killed here due to burn out.
	orderUpdateDroid(psDroid);
	if (isDead((BASE_OBJECT *)psDroid))
	{
		return;	// FIXME: Workaround for babarians that were burned to death
	}

	// update the action of the droid
	actionUpdateDroid(psDroid);

	syncDebugDroid(psDroid, 'M');

	// update the move system
	moveUpdateDroid(psDroid);

	/* Only add smoke if they're visible */
	if((psDroid->visible[selectedPlayer]) && psDroid->droidType != DROID_PERSON)
	{
		// need to clip this value to prevent overflow condition
		percentDamage = 100 - clip(PERCENT(psDroid->body, psDroid->originalBody), 0, 100);

		// Is there any damage?
		if(percentDamage>=25)
		{
			if(percentDamage>=100)
			{
				percentDamage = 99;
			}

			emissionInterval = CALC_DROID_SMOKE_INTERVAL(percentDamage);

			int effectTime = std::max(gameTime - deltaGameTime, psDroid->lastEmission + emissionInterval);
			if (gameTime > effectTime)
			{
   				dv.x = psDroid->pos.x + DROID_DAMAGE_SPREAD;
   				dv.z = psDroid->pos.y + DROID_DAMAGE_SPREAD;
				dv.y = psDroid->pos.z;

				dv.y += (psDroid->sDisplay.imd->max.y * 2);
				addEffect(&dv, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING_SMALL, false, NULL, 0, effectTime);
				psDroid->lastEmission = effectTime;
			}
		}
	}

	processVisibilityLevel((BASE_OBJECT*)psDroid);

	// -----------------
	/* Are we a sensor droid or a command droid? Show where we target for selectedPlayer. */
	if (psDroid->player == selectedPlayer && (psDroid->droidType == DROID_SENSOR || psDroid->droidType == DROID_COMMAND))
	{
		/* If we're attacking or sensing (observing), then... */
		if ((psBeingTargetted = orderStateObj(psDroid, DORDER_ATTACK))
		    || (psBeingTargetted = orderStateObj(psDroid, DORDER_OBSERVE)))
		{
			psBeingTargetted->bTargetted = true;
		}
	}
	// -----------------

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

					droidDamage(psDroid, damageToDo, WC_HEAT, WSC_FLAME, gameTime - deltaGameTime/2);
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

	// At this point, the droid may be dead due to burn damage.
	if (isDead((BASE_OBJECT *)psDroid))
	{
		return;
	}

	calcDroidIllumination(psDroid);

	// Check the resistance level of the droid
	if ((psDroid->id + gameTime)/833 != (psDroid->id + gameTime - deltaGameTime)/833)
	{
		// Zero resistance means not currently been attacked - ignore these
		if (psDroid->resistance && psDroid->resistance < droidResistance(psDroid))
		{
			// Increase over time if low
			psDroid->resistance++;
		}
	}

	syncDebugDroid(psDroid, '>');

	CHECK_DROID(psDroid);
}

/* See if a droid is next to a structure */
static bool droidNextToStruct(DROID *psDroid, BASE_OBJECT *psStruct)
{
	SDWORD	minX, maxX, maxY, x,y;

	CHECK_DROID(psDroid);

	minX = map_coord(psDroid->pos.x) - 1;
	y = map_coord(psDroid->pos.y) - 1;
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
			if (TileHasStructure(mapTile((UWORD)x,(UWORD)y)) &&
				getTileStructure(x,y) == (STRUCTURE *)psStruct)

			{
				return true;
			}
		}
	}

	return false;
}

static bool
droidCheckBuildStillInProgress( void *psObj )
{
	DROID	*psDroid;

	if ( psObj == NULL )
	{
		return false;
	}

	psDroid = (DROID*)psObj;
	CHECK_DROID(psDroid);

	if ( !psDroid->died && psDroid->action == DACTION_BUILD )
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool
droidBuildStartAudioCallback( void *psObj )
{
	DROID	*psDroid;

	psDroid = (DROID*)psObj;

	if (psDroid == NULL)
	{
		return true;
	}

	if ( psDroid->visible[selectedPlayer] )
	{
		audio_PlayObjDynamicTrack( psDroid, ID_SOUND_CONSTRUCTION_LOOP, droidCheckBuildStillInProgress );
	}

	return true;
}


/* Set up a droid to build a structure - returns true if successful */
bool droidStartBuild(DROID *psDroid)
{
	STRUCTURE *psStruct;

	CHECK_DROID(psDroid);

	/* See if we are starting a new structure */
	if ((psDroid->order.psObj == NULL) &&
		(psDroid->order.type == DORDER_BUILD ||
		 psDroid->order.type == DORDER_LINEBUILD))
	{
		STRUCTURE_STATS *psStructStat = psDroid->order.psStats;
		STRUCTURE_LIMITS *structLimit = &asStructLimits[psDroid->player][psStructStat - asStructureStats];

		//need to check structLimits have not been exceeded
		if (structLimit->currentQuantity >= structLimit->limit)
		{
			intBuildFinished(psDroid);
			cancelBuild(psDroid);
			return false;
		}
		// Can't build on burning oil derricks.
		if (psStructStat->type == REF_RESOURCE_EXTRACTOR && fireOnLocation(psDroid->order.pos.x,psDroid->order.pos.y))
		{
			intBuildFinished(psDroid);
			cancelBuild(psDroid);
			return false;
		}
		//ok to build
		psStruct = buildStructureDir(psStructStat, psDroid->order.pos.x, psDroid->order.pos.y, psDroid->order.direction, psDroid->player,false);
		if (!psStruct)
		{
			intBuildFinished(psDroid);
			cancelBuild(psDroid);
			return false;
		}
		psStruct->body = (psStruct->body + 9) / 10;  // Structures start at 10% health. Round up.
	}
	else
	{
		/* Check the structure is still there to build (joining a partially built struct) */
		psStruct = (STRUCTURE *)psDroid->order.psObj;
		if (!droidNextToStruct(psDroid, (BASE_OBJECT *)psStruct))
		{
			/* Nope - stop building */
			debug( LOG_NEVER, "not next to structure" );
		}
	}

	//check structure not already built, and we still 'own' it
	if (psStruct->status != SS_BUILT && aiCheckAlliances(psStruct->player, psDroid->player))
	{
		psDroid->actionStarted = gameTime;
		psDroid->actionPoints = 0;
		setDroidTarget(psDroid, (BASE_OBJECT *)psStruct);
		setDroidActionTarget(psDroid, (BASE_OBJECT *)psStruct, 0);
	}

	if ( psStruct->visible[selectedPlayer] )
	{
		audio_PlayObjStaticTrackCallback( psDroid, ID_SOUND_CONSTRUCTION_START,
										droidBuildStartAudioCallback );
	}

	CHECK_DROID(psDroid);

	return true;
}

static void droidAddWeldSound( Vector3i iVecEffect )
{
	SDWORD		iAudioID;

	iAudioID = ID_SOUND_CONSTRUCTION_1 + (rand()%4);

	audio_PlayStaticTrack( iVecEffect.x, iVecEffect.z, iAudioID );
}

static void addConstructorEffect(STRUCTURE *psStruct)
{
	UDWORD		widthRange,breadthRange;
	Vector3i temp;

	//FIXME
	if((ONEINTEN) && (psStruct->visible[selectedPlayer]))
	{
		/* This needs fixing - it's an arse effect! */
		widthRange   = getStructureWidth  (psStruct)*TILE_UNITS/4;
		breadthRange = getStructureBreadth(psStruct)*TILE_UNITS/4;
		temp.x = psStruct->pos.x+((rand()%(2*widthRange)) - widthRange);
		temp.y = map_TileHeight(map_coord(psStruct->pos.x), map_coord(psStruct->pos.y))+
						(psStruct->sDisplay.imd->max.y / 6);
		temp.z = psStruct->pos.y+((rand()%(2*breadthRange)) - breadthRange);
		if(rand()%2)
		{
			droidAddWeldSound( temp );
		}
	}
}

/* Update a construction droid while it is building
   returns true while building continues */
bool droidUpdateBuild(DROID *psDroid)
{
	UDWORD		pointsToAdd, constructPoints;
	STRUCTURE	*psStruct;

	CHECK_DROID(psDroid);
	ASSERT_OR_RETURN(false, psDroid->action == DACTION_BUILD, "%s (order %s) has wrong action for construction: %s",
	                 droidGetName(psDroid), getDroidOrderName(psDroid->order.type), getDroidActionName(psDroid->action));
	ASSERT_OR_RETURN(false, psDroid->order.psObj != NULL, "Trying to update a construction, but no target!");

	psStruct = (STRUCTURE *)psDroid->order.psObj;
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure" );
	ASSERT_OR_RETURN(false, psDroid->asBits[COMP_CONSTRUCT].nStat < numConstructStats, "Invalid construct pointer for unit" );

	// First check the structure hasn't been completed by another droid
	if (psStruct->status == SS_BUILT)
	{
		// Update the interface
		intBuildFinished(psDroid);
		// Check if line order build is completed, or we are not carrying out a line order build
		if ((map_coord(psDroid->order.pos) == map_coord(psDroid->order.pos2))
		    || psDroid->order.type != DORDER_LINEBUILD)
		{
			cancelBuild(psDroid);
		}
		else
		{
			psDroid->action = DACTION_NONE;	// make us continue line build
			setDroidTarget(psDroid, NULL);
			setDroidActionTarget(psDroid, NULL, 0);
		}
		return false;
	}

	// make sure we still 'own' the building in question
	if (!aiCheckAlliances(psStruct->player, psDroid->player))
	{
		cancelBuild(psDroid);		// stop what you are doing fool it isn't ours anymore!
		return false;
	}

	constructPoints = constructorPoints(asConstructStats + psDroid->
		asBits[COMP_CONSTRUCT].nStat, psDroid->player);

	pointsToAdd = constructPoints * (gameTime - psDroid->actionStarted) /
		GAME_TICKS_PER_SEC;

	structureBuild(psStruct, psDroid, pointsToAdd - psDroid->actionPoints, constructPoints);

	//store the amount just added
	psDroid->actionPoints = pointsToAdd;

	addConstructorEffect(psStruct);

	return true;
}

bool droidStartDemolishing( DROID *psDroid )
{
	psDroid->actionStarted = gameTime;
	psDroid->actionPoints = 0;
	return true;
}

bool droidUpdateDemolishing( DROID *psDroid )
{
	STRUCTURE	*psStruct;
	UDWORD		pointsToAdd, constructPoints;

	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->action == DACTION_DEMOLISH, "unit is not demolishing");
	psStruct = (STRUCTURE *)psDroid->order.psObj;
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");

	//constructPoints = (asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat)->
	//	constructPoints;
	constructPoints = 5 * constructorPoints(asConstructStats + psDroid->
		asBits[COMP_CONSTRUCT].nStat, psDroid->player);

	pointsToAdd = constructPoints * (gameTime - psDroid->actionStarted) /
		GAME_TICKS_PER_SEC;

	structureDemolish(psStruct, psDroid, pointsToAdd - psDroid->actionPoints);

	//store the amount just subtracted
	psDroid->actionPoints = pointsToAdd;

	addConstructorEffect(psStruct);

	CHECK_DROID(psDroid);

	return true;
}

bool droidStartRepair( DROID *psDroid )
{
	STRUCTURE	*psStruct;

	CHECK_DROID(psDroid);

	psStruct = (STRUCTURE *)psDroid->psActionTarget[0];
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	CHECK_DROID(psDroid);

	return true;
}


/*Start a Repair Droid working on a damaged droid*/
bool droidStartDroidRepair( DROID *psDroid )
{
	DROID	*psDroidToRepair;

	CHECK_DROID(psDroid);

	psDroidToRepair = (DROID *)psDroid->psActionTarget[0];
	ASSERT_OR_RETURN(false, psDroidToRepair->type == OBJ_DROID, "target is not a unit");

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	CHECK_DROID(psDroid);

	return true;
}

/*checks a droids current body points to see if need to self repair*/
void droidSelfRepair(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	if (!isVtolDroid(psDroid))
	{
		if (psDroid->body < psDroid->originalBody)
		{
			if (psDroid->asBits[COMP_REPAIRUNIT].nStat != 0)
			{
				psDroid->action = DACTION_DROIDREPAIR;
				setDroidActionTarget(psDroid, (BASE_OBJECT *)psDroid, 0);
				psDroid->actionStarted = gameTime;
				psDroid->actionPoints  = 0;
			}
		}
	}

	CHECK_DROID(psDroid);
}


/*Start a EW weapon droid working on a low resistance structure*/
bool droidStartRestore( DROID *psDroid )
{
	STRUCTURE	*psStruct;

	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->order.type == DORDER_RESTORE, "unit is not restoring");
	psStruct = (STRUCTURE *)psDroid->order.psObj;
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");

	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;

	CHECK_DROID(psDroid);

	return true;
}

/*continue restoring a structure*/
bool droidUpdateRestore( DROID *psDroid )
{
	STRUCTURE		*psStruct;
	UDWORD			pointsToAdd, restorePoints;
	WEAPON_STATS	*psStats;
	int compIndex;

	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->action == DACTION_RESTORE, "unit is not restoring");
	psStruct = (STRUCTURE *)psDroid->order.psObj;
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");
	ASSERT_OR_RETURN(false, psStruct->pStructureType->resistance != 0, "invalid structure for EW");

	ASSERT_OR_RETURN(false, psDroid->asWeaps[0].nStat > 0, "droid doesn't have any weapons");

	compIndex = psDroid->asWeaps[0].nStat;
	ASSERT_OR_RETURN(false, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
	psStats = asWeaponStats + compIndex;

	ASSERT_OR_RETURN(false, psStats->weaponSubClass == WSC_ELECTRONIC, "unit's weapon is not EW");

	restorePoints = calcDamage(weaponDamage(psStats, psDroid->player),
		psStats->weaponEffect,(BASE_OBJECT *)psStruct);

	pointsToAdd = restorePoints * (gameTime - psDroid->actionStarted) /
		GAME_TICKS_PER_SEC;

	psStruct->resistance = (SWORD)(psStruct->resistance + (pointsToAdd - psDroid->actionPoints));

	//store the amount just added
	psDroid->actionPoints = pointsToAdd;

	CHECK_DROID(psDroid);

	/* check if structure is restored */
	if (psStruct->resistance < (SDWORD)structureResistance(psStruct->
		pStructureType, psStruct->player))
	{
		return true;
	}
	else
	{
		addConsoleMessage(_("Structure Restored") ,DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
		psStruct->resistance = (UWORD)structureResistance(psStruct->pStructureType,
			psStruct->player);
		return false;
	}
}

// Declared in weapondef.h.
int getRecoil(WEAPON const &weapon)
{
	if (weapon.nStat != 0)
	{
		// We have a weapon.
		if (graphicsTime >= weapon.lastFired && graphicsTime < weapon.lastFired + DEFAULT_RECOIL_TIME)
		{
			int recoilTime = graphicsTime - weapon.lastFired;
			int recoilAmount = DEFAULT_RECOIL_TIME/2 - abs(recoilTime - DEFAULT_RECOIL_TIME/2);
			int maxRecoil = asWeaponStats[weapon.nStat].recoilValue;  // Max recoil is 1/10 of this value.
			return maxRecoil * recoilAmount/(DEFAULT_RECOIL_TIME/2 * 10);
		}
		// Recoil effect is over.
	}
	return 0;
}


bool droidUpdateRepair( DROID *psDroid )
{
	STRUCTURE	*psStruct;
	UDWORD		iPointsToAdd, iRepairPoints;

	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->action == DACTION_REPAIR, "unit does not have repair order");
	psStruct = (STRUCTURE *)psDroid->psActionTarget[0];

	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");
	iRepairPoints = constructorPoints(asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat, psDroid->player);
	iPointsToAdd = iRepairPoints * (gameTime - psDroid->actionStarted) / GAME_TICKS_PER_SEC;

	/* add points to structure */
	if (iPointsToAdd - psDroid->actionPoints > 0)
	{
		if (structureRepair(psStruct, psDroid, iPointsToAdd - psDroid->actionPoints))
		{
			/* store the amount just added */
			psDroid->actionPoints = iPointsToAdd;
		}
	}

	/* if not finished repair return true else complete repair and return false */
	if (psStruct->body < structureBody(psStruct))
	{
		return true;
	}
	else
	{
		objTrace(psDroid->id, "Repaired of %s all done with %u - %u points", objInfo((BASE_OBJECT *)psStruct), iPointsToAdd, psDroid->actionPoints);
		psStruct->body = (UWORD)structureBody(psStruct);
		return false;
	}
}

/*Updates a Repair Droid working on a damaged droid*/
bool droidUpdateDroidRepair(DROID *psRepairDroid)
{
	DROID		*psDroidToRepair;
	UDWORD		iPointsToAdd, iRepairPoints;
	Vector3i iVecEffect;

	CHECK_DROID(psRepairDroid);

	ASSERT_OR_RETURN(false, psRepairDroid->action == DACTION_DROIDREPAIR, "Unit does not have unit repair order");
	ASSERT_OR_RETURN(false, psRepairDroid->asBits[COMP_REPAIRUNIT].nStat != 0, "Unit does not have a repair turret");

	psDroidToRepair = (DROID *)psRepairDroid->psActionTarget[0];
	ASSERT_OR_RETURN(false, psDroidToRepair->type == OBJ_DROID, "Target is not a unit");

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

        //just add the points if the power cost is negligable
        //if these points would make the droid healthy again then just add
        if (psDroidToRepair->body + iPointsToAdd >= psDroidToRepair->originalBody)
        {
            //anothe HACK but sorts out all the rounding errors when values get small
		iPointsToAdd = psDroidToRepair->originalBody - psDroidToRepair->body;
        }
	psDroidToRepair->body += iPointsToAdd;
	psRepairDroid->actionPoints += iPointsToAdd;

	/* add plasma repair effect whilst being repaired */
	if ((ONEINFIVE) && (psDroidToRepair->visible[selectedPlayer]))
	{
		iVecEffect.x = psDroidToRepair->pos.x + DROID_REPAIR_SPREAD;
		iVecEffect.y = psDroidToRepair->pos.z + rand()%8;;
		iVecEffect.z = psDroidToRepair->pos.y + DROID_REPAIR_SPREAD;
		effectGiveAuxVar(90+rand()%20);
		addEffect(&iVecEffect, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, NULL, 0, gameTime - deltaGameTime + rand()%deltaGameTime);
		droidAddWeldSound( iVecEffect );
	}

	CHECK_DROID(psRepairDroid);

	/* if not finished repair return true else complete repair and return false */
	return psDroidToRepair->body < psDroidToRepair->originalBody;
}

// return whether a droid is IDF
bool idfDroid(DROID *psDroid)
{
	//add Cyborgs
	//if (psDroid->droidType != DROID_WEAPON)
	if (!(psDroid->droidType == DROID_WEAPON || psDroid->droidType == DROID_CYBORG ||
		psDroid->droidType == DROID_CYBORG_SUPER))
	{
		return false;
	}

	if (proj_Direct(psDroid->asWeaps[0].nStat + asWeaponStats))
	{
		return false;
	}

	return true;
}

/* Return the type of a droid */
DROID_TYPE droidType(DROID *psDroid)
{
	return psDroid->droidType;
}

/* Return the type of a droid from it's template */
DROID_TYPE droidTemplateType(DROID_TEMPLATE *psTemplate)
{
	DROID_TYPE type = DROID_DEFAULT;

	if (psTemplate->droidType == DROID_PERSON ||
	    psTemplate->droidType == DROID_CYBORG ||
	    psTemplate->droidType == DROID_CYBORG_SUPER ||
	    psTemplate->droidType == DROID_CYBORG_CONSTRUCT ||
	    psTemplate->droidType == DROID_CYBORG_REPAIR ||
	    psTemplate->droidType == DROID_TRANSPORTER)
	{
		type = psTemplate->droidType;
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
	else if ((asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->location == LOC_TURRET)
	{
		type = DROID_REPAIR;
	}
	else if ( psTemplate->asWeaps[0] != 0 )
	{
		type = DROID_WEAPON;
	}
	/* with more than weapon is still a DROID_WEAPON */
	else if ( psTemplate->numWeaps > 1)
	{
		type = DROID_WEAPON;
	}

	return type;
}

//Load the weapons assigned to Droids in the Access database
bool loadDroidWeapons(const char *pWeaponData, UDWORD bufferSize)
{
	TableView table(pWeaponData, bufferSize);

	for (unsigned i = 0; i < table.size(); ++i)
	{
		LineView line(table, i);

		DROID_TEMPLATE *pTemplate;
		std::string templateName = line.s(0);

		for (int player = 0; player < MAX_PLAYERS + 2; ++player)
		{
			if (player < MAX_PLAYERS)  // a player
			{
				if (!isHumanPlayer(player))
				{
					continue;	// no need to add to AIs, they use the static list
				}
				pTemplate = getTemplateFromUniqueName(templateName.c_str(), player);
			}
			else if (player == MAX_PLAYERS)  // special exception - the static list
			{
				// Add weapons to static list
				pTemplate = getTemplateFromTranslatedNameNoPlayer(templateName.c_str());
			}
			else  // Special exception - the local UI list.
			{
				pTemplate = NULL;
				for (std::list<DROID_TEMPLATE>::iterator j = localTemplates.begin(); j != localTemplates.end(); ++j)
				{
					if (j->pName == templateName)
					{
						pTemplate = &*j;
						break;
					}
				}
			}

			/* if Template not found - try default design */
			if (!pTemplate)
			{
				if (templateName == sDefaultDesignTemplate.pName)
				{
					pTemplate = &sDefaultDesignTemplate;
				}
				else
				{
					continue;	// ok, this player did not have this template. that's fine.
				}
			}

			ASSERT_OR_RETURN(false, pTemplate->numWeaps <= DROID_MAXWEAPS, "stack corruption unavoidable");

			for (unsigned j = 0; j < pTemplate->numWeaps; j++)
			{
				int incWpn = getCompFromName(COMP_WEAPON, line.s(1 + j).c_str());

				ASSERT_OR_RETURN(false, incWpn != -1, "Unable to find Weapon %s for template %s", line.s(1 + j).c_str(), templateName.c_str());

				//Weapon found, alloc this to the current Template
				pTemplate->asWeaps[pTemplate->storeCount] = incWpn;

				//check valid weapon/propulsion
				ASSERT_OR_RETURN(false, pTemplate->storeCount <= pTemplate->numWeaps, "Allocating more weapons than allowed for Template %s", templateName.c_str());
				ASSERT_OR_RETURN(false, checkValidWeaponForProp(pTemplate), "Weapon is invalid for air propulsion for template %s", templateName.c_str());
				pTemplate->storeCount++;
			}
		}
	}

	return true;
}

/* Calculate the weight of a droid from it's template */
UDWORD calcDroidWeight(DROID_TEMPLATE *psTemplate)
{
	UDWORD weight, i;

	/* Get the basic component weight */
	weight =
		(asBodyStats + psTemplate->asParts[COMP_BODY])->weight +
		(asBrainStats + psTemplate->asParts[COMP_BRAIN])->weight +
		//(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->weight +
		(asSensorStats + psTemplate->asParts[COMP_SENSOR])->weight +
		(asECMStats + psTemplate->asParts[COMP_ECM])->weight +
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
		body += asWeaponStats[psTemplate->asWeaps[i]].body;
	}

	//add on any upgrade value that may need to be applied
	body += (body * asBodyUpgrade[player]->body / 100);
	return body;
}

/* Calculate the base body points of a droid without upgrades*/
UDWORD calcDroidBaseBody(DROID *psDroid)
{
	//re-enabled i;
	UDWORD      body, i;

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
	for(i=0; i<psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			//body += (asWeaponStats + psDroid->asWeaps[i].nStat)->body;
			body += (asWeaponStats + psDroid->asWeaps[i].nStat)->body;
		}
	}

	return body;
}


/* Calculate the base speed of a droid from it's template */
UDWORD calcDroidBaseSpeed(DROID_TEMPLATE *psTemplate, UDWORD weight, UBYTE player)
{
	UDWORD	speed;

	if (psTemplate->droidType == DROID_CYBORG ||
		psTemplate->droidType == DROID_CYBORG_SUPER ||
		psTemplate->droidType == DROID_CYBORG_CONSTRUCT ||
		psTemplate->droidType == DROID_CYBORG_REPAIR)
	{
		speed = (asPropulsionTypes[(asPropulsionStats + psTemplate->
			asParts[COMP_PROPULSION])->propulsionType].powerRatioMult *
			bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY],
			player, CYBORG_BODY_UPGRADE)) / MAX(1, weight);
	}
	else
	{
		speed = (asPropulsionTypes[(asPropulsionStats + psTemplate->
			asParts[COMP_PROPULSION])->propulsionType].powerRatioMult *
			bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY],
			player, DROID_BODY_UPGRADE)) / MAX(1, weight);
	}

	// reduce the speed of medium/heavy VTOLs
	if (asPropulsionStats[psTemplate->asParts[COMP_PROPULSION]].propulsionType == PROPULSION_TYPE_LIFT)
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

	// Wateremelon:applies the engine output bonus if output > weight
	if ( (asBodyStats + psTemplate->asParts[COMP_BODY])->powerOutput > weight )
	{
		speed = speed * 3 / 2;
	}

	return speed;
}


/* Calculate the speed of a droid over a terrain */
UDWORD calcDroidSpeed(UDWORD baseSpeed, UDWORD terrainType, UDWORD propIndex, UDWORD level)
{
	PROPULSION_STATS	*propulsion = asPropulsionStats + propIndex;
	UDWORD				speed;

	speed  = baseSpeed;
	// Factor in terrain
	speed *= getSpeedFactor(terrainType, propulsion->propulsionType);
	speed /= 100;

	// Need to ensure doesn't go over the max speed possible for this propulsion
	if (speed > propulsion->maxSpeed)
	{
		speed = propulsion->maxSpeed;
	}

	// Factor in experience
	speed *= (100 + EXP_SPEED_BONUS * level);
	speed /= 100;

	return speed;
}

/* Calculate the points required to build the template - used to calculate time*/
UDWORD calcTemplateBuild(DROID_TEMPLATE *psTemplate)
{
	UDWORD	build, i;

	build = (asBodyStats + psTemplate->asParts[COMP_BODY])->buildPoints +
	(asBrainStats + psTemplate->asParts[COMP_BRAIN])->buildPoints +
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
		// FIX ME:
		ASSERT( psTemplate->asWeaps[i]<numWeaponStats,
			//"Invalid Template weapon for %s", psTemplate->pName );
			"Invalid Template weapon for %s", getTemplateName(psTemplate) );
		build += (asWeaponStats + psTemplate->asWeaps[i])->buildPoints;
	}

	return build;
}


/* Calculate the power points required to build/maintain a template */
UDWORD	calcTemplatePower(DROID_TEMPLATE *psTemplate)
{
	UDWORD power, i;

	//get the component power
	power = (asBodyStats + psTemplate->asParts[COMP_BODY])->buildPower;
	power += (asBrainStats + psTemplate->asParts[COMP_BRAIN])->buildPower;
	power += (asSensorStats + psTemplate->asParts[COMP_SENSOR])->buildPower;
	power += (asECMStats + psTemplate->asParts[COMP_ECM])->buildPower;
	power += (asRepairStats + psTemplate->asParts[COMP_REPAIRUNIT])->buildPower;
	power += (asConstructStats + psTemplate->asParts[COMP_CONSTRUCT])->buildPower;

	/* propulsion power points are a percentage of the bodys' power points */
	power += (((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->buildPower *
		(asBodyStats + psTemplate->asParts[COMP_BODY])->buildPower) / 100);

	//add weapon power
	for(i=0; i<psTemplate->numWeaps; i++)
	{
		power += (asWeaponStats + psTemplate->asWeaps[i])->buildPower;
	}

	return power;
}


/* Calculate the power points required to build/maintain a droid */
UDWORD	calcDroidPower(DROID *psDroid)
{
	//re-enabled i
	UDWORD      power, i;

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
	for(i=0; i<psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			//power += (asWeaponStats + psDroid->asWeaps[i].nStat)->buildPower;
			power += (asWeaponStats + psDroid->asWeaps[i].nStat)->buildPower;
		}
	}

	return power;
}

UDWORD calcDroidPoints(DROID *psDroid)
{
	unsigned int i;
	int points;

	points  = getBodyStats(psDroid)->buildPoints;
	points += getBrainStats(psDroid)->buildPoints;
	points += getPropulsionStats(psDroid)->buildPoints;
	points += getSensorStats(psDroid)->buildPoints;
	points += getECMStats(psDroid)->buildPoints;
	points += getRepairStats(psDroid)->buildPoints;
	points += getConstructStats(psDroid)->buildPoints;

	for (i = 0; i < psDroid->numWeaps; i++)
	{
		points += (asWeaponStats + psDroid->asWeaps[i].nStat)->buildPoints;
	}

	return points;
}

//Builds an instance of a Droid - the x/y passed in are in world coords.
DROID *reallyBuildDroid(DROID_TEMPLATE *pTemplate, Position pos, UDWORD player, bool onMission, Rotation rot)
{
	DROID			*psDroid;
	DROID_GROUP		*psGrp;
	UDWORD			inc;
	SDWORD			i, experienceLoc;

	// Don't use this assertion in single player, since droids can finish building while on an away mission
	ASSERT(!bMultiPlayer || worldOnMap(pos.x, pos.y), "the build locations are not on the map");

	//allocate memory
	psDroid = new DROID(generateSynchronisedObjectId(), player);
	if (psDroid == NULL)
	{
		debug(LOG_NEVER, "unit build: unable to create");
		ASSERT(!"out of memory", "Cannot get the memory for the unit");
		return NULL;
	}

	//fill in other details

	droidSetName(psDroid, pTemplate->aName);

	// Set the droids type
	psDroid->droidType = droidTemplateType(pTemplate);  // Is set again later to the same thing, in droidSetBits.
	psDroid->pos = pos;
	psDroid->rot = rot;

	//don't worry if not on homebase cos not being drawn yet
	if (!onMission)
	{
		//set droid height
		psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
	}

	if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_COMMAND)
	{
		psGrp = grpCreate();
		psGrp->add(psDroid);
	}

	psDroid->lastFrustratedTime = -UINT16_MAX;	// make sure we do not start the game frustrated

	psDroid->listSize = 0;
	psDroid->listPendingBegin = 0;

	psDroid->iAudioID = NO_SOUND;
	psDroid->psCurAnim = NULL;
	psDroid->group = UBYTE_MAX;
	psDroid->psBaseStruct = NULL;

	// find the highest stored experience
	// Unless game time is stopped, then we're hopefully loading a game and
	// don't want to use up recycled experience for the droids we just loaded.
	if (!gameTimeIsStopped() &&
		(psDroid->droidType != DROID_CONSTRUCT) &&
		(psDroid->droidType != DROID_CYBORG_CONSTRUCT) &&
		(psDroid->droidType != DROID_REPAIR) &&
		(psDroid->droidType != DROID_CYBORG_REPAIR) &&
		(psDroid->droidType != DROID_TRANSPORTER))
	{
		uint32_t numKills = 0;
		experienceLoc = 0;
		for(i=0; i<MAX_RECYCLED_DROIDS; i++)
		{
			if (aDroidExperience[player][i]*65536 > numKills)
			{
				numKills = aDroidExperience[player][i]*65536;
				experienceLoc = i;
			}
		}
		aDroidExperience[player][experienceLoc] = 0;
		psDroid->experience = numKills;
	}
	else
	{
		psDroid->experience = 0;
	}

	droidSetBits(pTemplate,psDroid);

	//calculate the droids total weight
	psDroid->weight = calcDroidWeight(pTemplate);

	// Initialise the movement stuff
	psDroid->baseSpeed = calcDroidBaseSpeed(pTemplate, psDroid->weight, (UBYTE)player);

	initDroidMovement(psDroid);

	// it was never drawn before
	psDroid->sDisplay.frameNumber = 0;

	//allocate 'easy-access' data!
	objSensorCache((BASE_OBJECT *)psDroid, asSensorStats + pTemplate->asParts[COMP_SENSOR]);
	objEcmCache((BASE_OBJECT *)psDroid, asECMStats + pTemplate->asParts[COMP_ECM]);
	psDroid->body = calcTemplateBody(pTemplate, (UBYTE)player);  // Redundant? (Is set in droidSetBits, too.)
	psDroid->originalBody = psDroid->body;  // Redundant? (Is set in droidSetBits, too.)

	for (inc = 0; inc < WC_NUM_WEAPON_CLASSES; inc++)
	{
		psDroid->armour[inc] = bodyArmour(asBodyStats + pTemplate->asParts[COMP_BODY], player, 
		                                  cyborgDroid(psDroid) ? CYBORG_BODY_UPGRADE : DROID_BODY_UPGRADE, (WEAPON_CLASS)inc);
	}

	//init the resistance to indicate no EW performed on this droid
	psDroid->resistance = ACTION_START_TIME;

	for (unsigned vPlayer = 0; vPlayer < MAX_PLAYERS; ++vPlayer)
	{
		psDroid->visible[vPlayer] = hasSharedVision(vPlayer, player)? UINT8_MAX : 0;
	}
	memset(psDroid->seenThisTick, 0, sizeof(psDroid->seenThisTick));
	psDroid->died = 0;
	psDroid->inFire = 0;
	psDroid->burnStart = 0;
	psDroid->burnDamage = 0;
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
		visTilesUpdate((BASE_OBJECT *)psDroid);
 		clustNewDroid(psDroid);
	}

	/* transporter-specific stuff */
	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		//add transporter launch button if selected player and not a reinforcable situation
		if ( player == selectedPlayer && !missionCanReEnforce())
		{
			(void)intAddTransporterLaunch(psDroid);
		}

		//set droid height to be above the terrain
		psDroid->pos.z += TRANSPORTER_HOVER_HEIGHT;

		/* reset halt secondary order from guard to hold */
		secondarySetState( psDroid, DSO_HALTTYPE, DSS_HALT_HOLD );
	}

	if(player == selectedPlayer)
	{
		scoreUpdateVar(WD_UNITS_BUILT);
	}

	return psDroid;
}

DROID *buildDroid(DROID_TEMPLATE *pTemplate, UDWORD x, UDWORD y, UDWORD player, bool onMission, const INITIAL_DROID_ORDERS *initialOrders)
{
	// ajl. droid will be created, so inform others
	if (bMultiMessages)
	{
		// Only sends if it's ours, otherwise the owner should send the message.
		SendDroid(pTemplate, x, y, player, generateNewObjectId(), initialOrders);
		return NULL;
	}
	else
	{
		return reallyBuildDroid(pTemplate, Position(x, y, 0), player, onMission);
	}
}

//initialises the droid movement model
void initDroidMovement(DROID *psDroid)
{
	memset(&psDroid->sMove, 0, sizeof(MOVE_CONTROL));
}

// Set the asBits in a DROID structure given it's template.
void droidSetBits(DROID_TEMPLATE *pTemplate,DROID *psDroid)
{
	psDroid->droidType = droidTemplateType(pTemplate);
	psDroid->numWeaps = pTemplate->numWeaps;
	psDroid->body = calcTemplateBody(pTemplate, psDroid->player);
	psDroid->originalBody = psDroid->body;
	psDroid->expectedDamage = 0;  // Begin life optimistically.
	psDroid->time = gameTime - deltaGameTime;         // Start at beginning of tick.
	psDroid->prevSpacetime.time = psDroid->time - 1;  // -1 for interpolation.

	//create the droids weapons
	for (int inc = 0; inc < DROID_MAXWEAPS; inc++)
	{
		psDroid->psActionTarget[inc] = NULL;
		psDroid->asWeaps[inc].lastFired=0;
		psDroid->asWeaps[inc].shotsFired=0;
		// no weapon (could be a construction droid for example)
		// this is also used to check if a droid has a weapon, so zero it
		psDroid->asWeaps[inc].nStat = 0;
		psDroid->asWeaps[inc].ammo = 0;
		psDroid->asWeaps[inc].rot.direction = 0;
		psDroid->asWeaps[inc].rot.pitch = 0;
		psDroid->asWeaps[inc].rot.roll = 0;
		psDroid->asWeaps[inc].prevRot = psDroid->asWeaps[inc].rot;
		if (inc < pTemplate->numWeaps)
		{
			psDroid->asWeaps[inc].nStat = pTemplate->asWeaps[inc];
			psDroid->asWeaps[inc].ammo = (asWeaponStats + psDroid->asWeaps[inc].nStat)->numRounds;
		}
	}
	//allocate the components hit points
	psDroid->asBits[COMP_BODY].nStat = (UBYTE)pTemplate->asParts[COMP_BODY];

	// ajl - changed this to init brains for all droids (crashed)
	psDroid->asBits[COMP_BRAIN].nStat = 0;

	// This is done by the Command droid stuff - John.
	// Not any more - John.
	psDroid->asBits[COMP_BRAIN].nStat = pTemplate->asParts[COMP_BRAIN];
	psDroid->asBits[COMP_PROPULSION].nStat = pTemplate->asParts[COMP_PROPULSION];
	psDroid->asBits[COMP_SENSOR].nStat = pTemplate->asParts[COMP_SENSOR];
	psDroid->asBits[COMP_ECM].nStat = pTemplate->asParts[COMP_ECM];
	psDroid->asBits[COMP_REPAIRUNIT].nStat = pTemplate->asParts[COMP_REPAIRUNIT];
	psDroid->asBits[COMP_CONSTRUCT].nStat = pTemplate->asParts[COMP_CONSTRUCT];

	switch (getPropulsionStats(psDroid)->propulsionType)  // getPropulsionStats(psDroid) only defined after psDroid->asBits[COMP_PROPULSION] is set.
	{
	case PROPULSION_TYPE_LIFT:
		psDroid->blockedBits = AIR_BLOCKED;
		break;
	case PROPULSION_TYPE_HOVER:
		psDroid->blockedBits = FEATURE_BLOCKED;
		break;
	case PROPULSION_TYPE_PROPELLOR:
		psDroid->blockedBits = FEATURE_BLOCKED | LAND_BLOCKED;
		break;
	default:
		psDroid->blockedBits = FEATURE_BLOCKED | WATER_BLOCKED;
		break;
	}
}


// Sets the parts array in a template given a droid.
void templateSetParts(const DROID *psDroid, DROID_TEMPLATE *psTemplate)
{
	psTemplate->numWeaps = 0;
	psTemplate->droidType = psDroid->droidType;
	for (int inc = 0; inc < DROID_MAXWEAPS; inc++)
	{
		//this should fix the NULL weapon stats for empty weaponslots
		psTemplate->asWeaps[inc] = 0;
		if (psDroid->asWeaps[inc].nStat > 0)
		{
			psTemplate->numWeaps += 1;
			psTemplate->asWeaps[inc] = psDroid->asWeaps[inc].nStat;
		}
	}
	psTemplate->asParts[COMP_BODY] = psDroid->asBits[COMP_BODY].nStat;
	psTemplate->asParts[COMP_BRAIN] = psDroid->asBits[COMP_BRAIN].nStat;
	psTemplate->asParts[COMP_PROPULSION] = psDroid->asBits[COMP_PROPULSION].nStat;
	psTemplate->asParts[COMP_SENSOR] = psDroid->asBits[COMP_SENSOR].nStat;
	psTemplate->asParts[COMP_ECM] = psDroid->asBits[COMP_ECM].nStat;
	psTemplate->asParts[COMP_REPAIRUNIT] = psDroid->asBits[COMP_REPAIRUNIT].nStat;
	psTemplate->asParts[COMP_CONSTRUCT] = psDroid->asBits[COMP_CONSTRUCT].nStat;
}

/* Make all the droids for a certain player a member of a specific group */
void assignDroidsToGroup(UDWORD	playerNumber, UDWORD groupNumber)
{
DROID	*psDroid;
bool	bAtLeastOne = false;
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
				bAtLeastOne = true;
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
			psFlagPos->selected = false;
		}
		groupConsoleInformOfCreation(groupNumber);
		secondarySetAverageGroupState(selectedPlayer, groupNumber);
	}
}


bool activateGroupAndMove(UDWORD playerNumber, UDWORD groupNumber)
{
	DROID	*psDroid,*psCentreDroid = NULL;
	bool selected = false;
	FLAG_POSITION	*psFlagPos;

	if (groupNumber < UBYTE_MAX)
	{
		for(psDroid = apsDroidLists[playerNumber]; psDroid!=NULL; psDroid = psDroid->psNext)
		{
			/* Wipe out the ones in the wrong group */
			if (psDroid->selected && psDroid->group != groupNumber)
			{
				DeSelectDroid(psDroid);
			}
			/* Get the right ones */
			if(psDroid->group == groupNumber)
			{
				SelectDroid(psDroid);
				psCentreDroid = psDroid;
			}
		}

		/* There was at least one in the group */
		if (psCentreDroid)
		{
			//clear the Deliv Point if one
			for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
			psFlagPos = psFlagPos->psNext)
			{
				psFlagPos->selected = false;
			}

			selected = true;
			if (!driveModeActive())
			{
				if (getWarCamStatus())
				{
					camToggleStatus();			 // messy - fix this
					processWarCam(); //odd, but necessary
					camToggleStatus();				// messy - FIXME
				}
				else
				{
					/* Centre display on him if warcam isn't active */
					setViewPos(map_coord(psCentreDroid->pos.x), map_coord(psCentreDroid->pos.y), true);
				}
			}
		}
	}

	if(selected)
	{
		setSelectedGroup(groupNumber);
		groupConsoleInformOfCentering(groupNumber);
	}
	else
	{
		setSelectedGroup(UBYTE_MAX);
	}

	return selected;
}

bool activateGroup(UDWORD playerNumber, UDWORD groupNumber)
{
	DROID	*psDroid;
	bool selected = false;
	FLAG_POSITION	*psFlagPos;

	if (groupNumber < UBYTE_MAX)
	{
		for (psDroid = apsDroidLists[playerNumber]; psDroid; psDroid = psDroid->psNext)
		{
			/* Wipe out the ones in the wrong group */
			if (psDroid->selected && psDroid->group != groupNumber)
			{
				DeSelectDroid(psDroid);
			}
			/* Get the right ones */
			if (psDroid->group == groupNumber)
			{
				SelectDroid(psDroid);
				selected = true;
			}
		}
	}

	if (selected)
	{
		setSelectedGroup(groupNumber);
		//clear the Deliv Point if one
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = false;
		}
		groupConsoleInformOfSelection(groupNumber);
	}
	else
	{
		setSelectedGroup(UBYTE_MAX);
	}
	return selected;
}

void	groupConsoleInformOfSelection( UDWORD groupNumber )
{
	// ffs am
	char groupInfo[255];
	unsigned int num_selected = selNumSelected(selectedPlayer);

	ssprintf(groupInfo, ngettext("Group %u selected - %u Unit", "Group %u selected - %u Units", num_selected), groupNumber, num_selected);
	addConsoleMessage(groupInfo,RIGHT_JUSTIFY,SYSTEM_MESSAGE);

}

void	groupConsoleInformOfCreation( UDWORD groupNumber )
{
	char groupInfo[255];

	if (!getWarCamStatus())
	{
		unsigned int num_selected = selNumSelected(selectedPlayer);

		ssprintf(groupInfo, ngettext("%u unit assigned to Group %u", "%u units assigned to Group %u", num_selected), num_selected, groupNumber);
		addConsoleMessage(groupInfo,RIGHT_JUSTIFY,SYSTEM_MESSAGE);
	}

}

void	groupConsoleInformOfCentering( UDWORD groupNumber )
{
	char	groupInfo[255];
	unsigned int num_selected = selNumSelected(selectedPlayer);

	if(!getWarCamStatus())
	{
		ssprintf(groupInfo, ngettext("Centered on Group %u - %u Unit", "Centered on Group %u - %u Units", num_selected), groupNumber, num_selected);
	}
	else
	{
		ssprintf(groupInfo, ngettext("Aligning with Group %u - %u Unit", "Aligning with Group %u - %u Units", num_selected), groupNumber, num_selected);
	}

	addConsoleMessage(groupInfo,RIGHT_JUSTIFY,SYSTEM_MESSAGE);
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

/**
 * calculate muzzle base location in 3d world
 */
bool calcDroidMuzzleBaseLocation(DROID *psDroid, Vector3i *muzzle, int weapon_slot)
{
	iIMDShape *psBodyImd = BODY_IMD(psDroid, psDroid->player);

	CHECK_DROID(psDroid);

	if (psBodyImd && psBodyImd->nconnectors)
	{
		Vector3i barrel(0, 0, 0);

		Affine3F af;

		af.Trans(psDroid->pos.x, -psDroid->pos.z, psDroid->pos.y);

		//matrix = the center of droid
		af.RotY(psDroid->rot.direction);
		af.RotX(psDroid->rot.pitch);
		af.RotZ(-psDroid->rot.roll);
		af.Trans(psBodyImd->connectors[weapon_slot].x, -psBodyImd->connectors[weapon_slot].z,
					 -psBodyImd->connectors[weapon_slot].y);//note y and z flipped

		*muzzle = swapYZ(af*barrel);
		muzzle->z = -muzzle->z;
	}
	else
	{
		*muzzle = psDroid->pos + Vector3i(0, 0, psDroid->sDisplay.imd->max.y);
	}

	CHECK_DROID(psDroid);

	return true;
}

/**
 * calculate muzzle tip location in 3d world
 */
bool calcDroidMuzzleLocation(DROID *psDroid, Vector3i *muzzle, int weapon_slot)
{
	iIMDShape *psBodyImd = BODY_IMD(psDroid, psDroid->player);

	CHECK_DROID(psDroid);

	if (psBodyImd && psBodyImd->nconnectors)
	{
		char debugStr[250], debugLen = 0;  // Each "(%d,%d,%d)" uses up to 34 bytes, for very large values. So 250 isn't exaggerating.

		Vector3i barrel(0, 0, 0);
		iIMDShape *psWeaponImd = 0, *psMountImd = 0;

		if (psDroid->asWeaps[weapon_slot].nStat)
		{
			psMountImd = WEAPON_MOUNT_IMD(psDroid, weapon_slot);
			psWeaponImd = WEAPON_IMD(psDroid, weapon_slot);
		}

		Affine3F af;

		af.Trans(psDroid->pos.x, -psDroid->pos.z, psDroid->pos.y);

		//matrix = the center of droid
		af.RotY(psDroid->rot.direction);
		af.RotX(psDroid->rot.pitch);
		af.RotZ(-psDroid->rot.roll);
		af.Trans(psBodyImd->connectors[weapon_slot].x, -psBodyImd->connectors[weapon_slot].z,
					 -psBodyImd->connectors[weapon_slot].y);//note y and z flipped
		debugLen += sprintf(debugStr + debugLen, "connect:body[%d]=(%d,%d,%d)", weapon_slot, psBodyImd->connectors[weapon_slot].x, -psBodyImd->connectors[weapon_slot].z, -psBodyImd->connectors[weapon_slot].y);

		//matrix = the weapon[slot] mount on the body
		af.RotY(psDroid->asWeaps[weapon_slot].rot.direction);  // +ve anticlockwise

		// process turret mount
		if (psMountImd && psMountImd->nconnectors)
		{
			af.Trans(psMountImd->connectors->x, -psMountImd->connectors->z, -psMountImd->connectors->y);
			debugLen += sprintf(debugStr + debugLen, ",turret=(%d,%d,%d)", psMountImd->connectors->x, -psMountImd->connectors->z, -psMountImd->connectors->y);
		}

		//matrix = the turret connector for the gun
		af.RotX(psDroid->asWeaps[weapon_slot].rot.pitch);      // +ve up

		//process the gun
		if (psWeaponImd && psWeaponImd->nconnectors)
		{
			unsigned int connector_num = 0;

			// which barrel is firing if model have multiple muzzle connectors?
			if (psDroid->asWeaps[weapon_slot].shotsFired && (psWeaponImd->nconnectors > 1))
			{
				// shoot first, draw later - substract one shot to get correct results
				connector_num = (psDroid->asWeaps[weapon_slot].shotsFired - 1) % (psWeaponImd->nconnectors);
			}
			
			barrel = Vector3i(psWeaponImd->connectors[connector_num].x,
							  -psWeaponImd->connectors[connector_num].z,
							-psWeaponImd->connectors[connector_num].y);
			debugLen += sprintf(debugStr + debugLen, ",barrel[%u]=(%d,%d,%d)", connector_num, psWeaponImd->connectors[connector_num].x, -psWeaponImd->connectors[connector_num].y, -psWeaponImd->connectors[connector_num].z);
		}

		*muzzle = swapYZ(af*barrel);
		muzzle->z = -muzzle->z;
		sprintf(debugStr + debugLen, ",muzzle=(%d,%d,%d)", muzzle->x, muzzle->y, muzzle->z);

		syncDebug("%s", debugStr);
	}
	else
	{
		*muzzle = psDroid->pos + Vector3i(0, 0, psDroid->sDisplay.imd->max.y);
	}

	CHECK_DROID(psDroid);

	return true;
}


// finds a droid for the player and sets it to be the current selected droid
bool selectDroidByID(UDWORD id, UDWORD player)
{
	DROID	*psCurr;

	//look through the list of droids for the player and find the matching id
	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->id == id)
		{
			break;
		}
	}

	if (psCurr)
	{
		clearSelection();
		SelectDroid(psCurr);
		return true;
	}

	return false;
}

struct rankMap
{
	unsigned int kills;          // required minimum amount of kills to reach this rank
	unsigned int commanderKills; // required minimum amount of kills for a commander (or sensor) to reach this rank
	const char*  name;           // name of this rank
};

static const struct rankMap arrRank[] =
{
	{0,   0,    N_("Rookie")},
	{4,   8,    NP_("rank", "Green")},
	{8,   16,   N_("Trained")},
	{16,  32,   N_("Regular")},
	{32,  64,   N_("Professional")},
	{64,  128,  N_("Veteran")},
	{128, 256,  N_("Elite")},
	{256, 512,  N_("Special")},
	{512, 1024, N_("Hero")}
};

unsigned int getDroidLevel(const DROID* psDroid)
{
	bool isCommander = (psDroid->droidType == DROID_COMMAND ||
						psDroid->droidType == DROID_SENSOR) ? true : false;
	unsigned int numKills = psDroid->experience/65536;
	unsigned int i;

	// Search through the array of ranks until one is found
	// which requires more kills than the droid has.
	// Then fall back to the previous rank.
	for (i = 1; i < ARRAY_SIZE(arrRank); ++i)
	{
		const unsigned int requiredKills = isCommander ? arrRank[i].commanderKills : arrRank[i].kills;

		if (numKills < requiredKills)
		{
			return i - 1;
		}
	}

	// If the criteria of the last rank are met, then select the last one
	return ARRAY_SIZE(arrRank) - 1;
}

UDWORD getDroidEffectiveLevel(DROID *psDroid)
{
	UDWORD level = getDroidLevel(psDroid);
	UDWORD cmdLevel = 0;

	// get commander level
	if (hasCommander(psDroid))
	{
		cmdLevel = cmdGetCommanderLevel(psDroid);

		// Commanders boost units' effectiveness just by being assigned to it
		level++;
	}

	return MAX(level, cmdLevel);
}


const char *getDroidNameForRank(UDWORD rank)
{
	ASSERT_OR_RETURN(PE_("rank", "invalid"), rank < (sizeof(arrRank) / sizeof(struct rankMap)),
			"given rank number (%d) out of bounds, we only have %lu ranks", rank, (unsigned long) (sizeof(arrRank) / sizeof(struct rankMap)) );

	return PE_("rank", arrRank[rank].name);
}

const char *getDroidLevelName(DROID *psDroid)
{
	return(getDroidNameForRank(getDroidLevel(psDroid)));
}

UDWORD	getNumDroidsForLevel(UDWORD	level)
{
	DROID	*psDroid;
	UDWORD	count;

	for(psDroid = apsDroidLists[selectedPlayer],count = 0;
		psDroid; psDroid = psDroid->psNext)
	{
		if (getDroidLevel(psDroid) == level)
		{
			count++;
		}
	}

	return count;
}

// Get the name of a droid from it's DROID structure.
//
const char *droidGetName(const DROID *psDroid)
{
	return psDroid->aName;
}

//
// Set the name of a droid in it's DROID structure.
//
// - only possible on the PC where you can adjust the names,
//
void droidSetName(DROID *psDroid,const char *pName)
{
	sstrcpy(psDroid->aName, pName);
}

// ////////////////////////////////////////////////////////////////////////////
// returns true when no droid on x,y square.
bool noDroid(UDWORD x, UDWORD y)
{
	unsigned int i;

	// check each droid list
	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		const DROID* psDroid;
		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if (map_coord(psDroid->pos.x) == x
			 && map_coord(psDroid->pos.y) == y)
			{
					return false;
			}
		}
	}
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// returns true when at most one droid on x,y square.
static bool oneDroidMax(UDWORD x, UDWORD y)
{
	UDWORD i;
	bool bFound = false;
	DROID *pD;
	// check each droid list
	for(i=0;i<MAX_PLAYERS;i++)
	{
		for(pD = apsDroidLists[i]; pD ; pD= pD->psNext)
		{
			if (map_coord(pD->pos.x) == x
			 && map_coord(pD->pos.y) == y)
			{
				if (bFound)
				{
					return false;
				}

				bFound = true;//first droid on this square so continue
			}
		}
	}
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// returns true if it's a sensible place to put that droid.
static bool sensiblePlace(SDWORD x, SDWORD y, PROPULSION_TYPE propulsion)
{
	// not too near the edges.
	if((x < TOO_NEAR_EDGE) || (x > (SDWORD)(mapWidth - TOO_NEAR_EDGE)))
		return false;
	if((y < TOO_NEAR_EDGE) || (y > (SDWORD)(mapHeight - TOO_NEAR_EDGE)))
		return false;

	// not on a blocking tile.
	if (fpathBlockingTile(x, y, propulsion))
	{
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------------
// Should stop things being placed in inaccessible areas? Assume wheeled propulsion.
bool	zonedPAT(UDWORD x, UDWORD y)
{
	return sensiblePlace(x, y, PROPULSION_TYPE_WHEELED) && noDroid(x,y);
}

static bool canFitDroid(UDWORD x, UDWORD y)
{
	return sensiblePlace(x, y, PROPULSION_TYPE_WHEELED) && oneDroidMax(x, y);
}

/// find a tile for which the function will return true
bool	pickATileGen(UDWORD *x, UDWORD *y, UBYTE numIterations,
					 bool (*function)(UDWORD x, UDWORD y))
{
	return pickATileGenThreat(x, y, numIterations, -1, -1, function);
}

bool pickATileGen(Vector2i *pos, unsigned numIterations, bool (*function)(UDWORD x, UDWORD y))
{
	UDWORD x = pos->x, y = pos->y;
	bool ret = pickATileGenThreat(&x, &y, numIterations, -1, -1, function);
	*pos = Vector2i(x, y);
	return ret;
}

/// find a tile for which the passed function will return true without any threat in the specified range 
bool	pickATileGenThreat(UDWORD *x, UDWORD *y, UBYTE numIterations, SDWORD threatRange,
					 SDWORD player, bool (*function)(UDWORD x, UDWORD y))
{
	SDWORD		i, j;
	SDWORD		startX, endX, startY, endY;
	UDWORD		passes;
	Vector3i	origin(world_coord(*x), world_coord(*y), 0);

	ASSERT_OR_RETURN(false, *x<mapWidth,"x coordinate is off-map for pickATileGen" );
	ASSERT_OR_RETURN(false, *y<mapHeight,"y coordinate is off-map for pickATileGen" );

	if (function(*x,*y) && ((threatRange <=0) || (!ThreatInRange(player, threatRange, *x, *y, false))))	//TODO: vtol check really not needed?
	{
		return(true);
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
				if(i==startX || i==endX || j==startY || j==endY)
				{
					Vector3i newPos(world_coord(i), world_coord(j), 0);

					/* Good enough? */
					if (function(i, j)
					    && fpathCheck(origin, newPos, PROPULSION_TYPE_WHEELED)
					    && ((threatRange <= 0) || (!ThreatInRange(player, threatRange, world_coord(i), world_coord(j), false))))
					{
						/* Set exit conditions and get out NOW */
						*x = i;	*y = j;
						return true;
					}
				}
			}
		}
		/* Expand the box out in all directions - off map handled by tileAcceptable */
		startX--; startY--;	endX++;	endY++;	passes++;
	}
	/* If we got this far, then we failed - passed in values will be unchanged */
	return false;

}

/// find a tile for a wheeled droid with only one other droid present
PICKTILE pickHalfATile(UDWORD *x, UDWORD *y, UBYTE numIterations)
{
	return pickATileGen(x, y, numIterations, canFitDroid)? FREE_TILE : NO_FREE_TILE;
}

/* Looks through the players list of droids to see if any of them are
building the specified structure - returns true if finds one*/
bool checkDroidsBuilding(STRUCTURE *psStructure)
{
	DROID				*psDroid;

	for (psDroid = apsDroidLists[psStructure->player]; psDroid != NULL; psDroid =
		psDroid->psNext)
	{
		//check DORDER_BUILD, HELP_BUILD is handled the same
		BASE_OBJECT * const psStruct = orderStateObj(psDroid, DORDER_BUILD);
		if ((STRUCTURE*)psStruct == psStructure)
		{
			return true;
		}
	}
	return false;
}

/* Looks through the players list of droids to see if any of them are
demolishing the specified structure - returns true if finds one*/
bool checkDroidsDemolishing(STRUCTURE *psStructure)
{
	DROID				*psDroid;

	for (psDroid = apsDroidLists[psStructure->player]; psDroid != NULL; psDroid =
		psDroid->psNext)
	{
		//check DORDER_DEMOLISH
		BASE_OBJECT * const psStruct = orderStateObj(psDroid, DORDER_DEMOLISH);
		if ((STRUCTURE*)psStruct == psStructure)
		{
			return true;
		}
	}
	return false;
}


int nextModuleToBuild(STRUCTURE const *psStruct, int lastOrderedModule)
{
	int order = 0;
	UDWORD	i = 0;

	ASSERT_OR_RETURN(0, psStruct != NULL && psStruct->pStructureType != NULL, "Invalid structure pointer");

	int next = psStruct->status == SS_BUILT? 1 : 0;  // If complete, next is one after the current number of modules, otherwise next is the one we're working on.
	int max;
	switch (psStruct->pStructureType->type)
	{
		case REF_POWER_GEN:
			//check room for one more!
			ASSERT_OR_RETURN(false, psStruct->pFunctionality != NULL, "Functionality missing for power!");
			max = std::max<int>(psStruct->pFunctionality->powerGenerator.capacity/NUM_POWER_MODULES + next, lastOrderedModule + 1);
			if (max <= 1)
			{
				i = powerModuleStat;
				order = max*NUM_POWER_MODULES;  // Power modules are weird. Build one, get three free.
			}
			break;
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
			//check room for one more!
			ASSERT_OR_RETURN(false, psStruct->pFunctionality != NULL, "Functionality missing for factory!");
			max = std::max<int>(psStruct->pFunctionality->factory.capacity + next, lastOrderedModule + 1);
			if (max <= NUM_FACTORY_MODULES)
			{
				i = factoryModuleStat;
				order = max;
			}
			break;
		case REF_RESEARCH:
			//check room for one more!
			ASSERT_OR_RETURN(false, psStruct->pFunctionality != NULL, "Functionality missing for research!");
			max = std::max<int>(psStruct->pFunctionality->researchFacility.capacity/NUM_RESEARCH_MODULES + next, lastOrderedModule + 1);
			if (max <= 1)
			{
				i = researchModuleStat;
				order = max*NUM_RESEARCH_MODULES;  // Research modules are weird. Build one, get three free.
			}
			break;
		default:
			//no other structures can have modules attached
			break;
	}

	if (order)
	{
		// Check availability of Module
		if (!((i < numStructureStats) &&
			(apStructTypeLists[psStruct->player][i] == AVAILABLE)))
		{
			order = 0;
		}
	}

	return order;
}

/*Deals with building a module - checking if any droid is currently doing this
 - if so, helping to build the current one*/
void setUpBuildModule(DROID *psDroid)
{
	Vector2i tile = map_coord(psDroid->order.pos);

	//check not another Truck started
	STRUCTURE *psStruct = getTileStructure(tile.x, tile.y);
	if (psStruct)
	{	// if a droid is currently building, or building is in progress of being built/upgraded the droid's order should be DORDER_HELPBUILD
		if (checkDroidsBuilding(psStruct) || !psStruct->status )
		{
			//set up the help build scenario
			psDroid->order.type = DORDER_HELPBUILD;
			setDroidTarget(psDroid, (BASE_OBJECT *)psStruct);
			if (droidStartBuild(psDroid))
			{
				psDroid->action = DACTION_BUILD;
				intBuildStarted(psDroid);
				return;
			}
		}
		else
		{
			if (nextModuleToBuild(psStruct, -1) > 0)
			{
				//no other droids building so just start it off
				if (droidStartBuild(psDroid))
				{
					psDroid->action = DACTION_BUILD;
					intBuildStarted(psDroid);
					return;
				}
			}
		}
	}
	cancelBuild(psDroid);
}

/* Just returns true if the droid's present body points aren't as high as the original*/
bool	droidIsDamaged(DROID *psDroid)
{
	if(psDroid->body < psDroid->originalBody)
	{
		return(true);
	}
	else
	{
		return(false);
	}
}


char const *getDroidResourceName(char const *pName)
{
	/* See if the name has a string resource associated with it by trying
	 * to get the string resource.
	 */
	return strresGetString(psStringRes, pName);
}


/*checks to see if an electronic warfare weapon is attached to the droid*/
bool electronicDroid(DROID *psDroid)
{
	DROID	*psCurr;

	CHECK_DROID(psDroid);

	//use slot 0 for now
	//if (psDroid->numWeaps && asWeaponStats[psDroid->asWeaps[0].nStat].
	if (psDroid->numWeaps > 0 && asWeaponStats[psDroid->asWeaps[0].nStat].
		weaponSubClass == WSC_ELECTRONIC)
	{
		return true;
	}

	if (psDroid->droidType == DROID_COMMAND && psDroid->psGroup && psDroid->psGroup->psCommander == psDroid)
	{
		// if a commander has EW units attached it is electronic
		for (psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			if (psDroid != psCurr && electronicDroid(psCurr))
			{
				return true;
			}
		}
	}

	return false;
}

/*checks to see if the droid is currently being repaired by another*/
bool droidUnderRepair(DROID *psDroid)
{
	DROID		*psCurr;

	CHECK_DROID(psDroid);

	//droid must be damaged
	if (droidIsDamaged(psDroid))
	{
		//look thru the list of players droids to see if any are repairing this droid
		for (psCurr = apsDroidLists[psDroid->player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if ((psCurr->droidType == DROID_REPAIR || psCurr->droidType ==
				DROID_CYBORG_REPAIR) && psCurr->action ==
				DACTION_DROIDREPAIR && psCurr->order.psObj == psDroid)
			{
				return true;
			}
		}
	}
	return false;
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
bool isVtolDroid(const DROID* psDroid)
{
	return asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT
	    && psDroid->droidType != DROID_TRANSPORTER;
}

/* returns true if the droid has lift propulsion and is moving */ 
bool isFlying(const DROID* psDroid)
{
	return (asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat)->propulsionType == PROPULSION_TYPE_LIFT  
			&& ( psDroid->sMove.Status != MOVEINACTIVE || psDroid->droidType == DROID_TRANSPORTER ); 
}

/* returns true if it's a VTOL weapon droid which has completed all runs */
bool vtolEmpty(DROID *psDroid)
{
	UBYTE	i;

	CHECK_DROID(psDroid);

	if (!isVtolDroid(psDroid))
	{
		return false;
	}
	if (psDroid->droidType != DROID_WEAPON)
	{
		return false;
	}

	for (i = 0; i < psDroid->numWeaps; i++)
	{
		if (asWeaponStats[psDroid->asWeaps[i].nStat].vtolAttackRuns > 0 &&
		    psDroid->sMove.iAttackRuns[i] < getNumAttackRuns(psDroid, i))
		{
			return false;
		}
	}

	return true;
}

/* returns true if it's a VTOL weapon droid which still has full ammo */
bool vtolFull(DROID *psDroid)
{
	UBYTE	i;

	CHECK_DROID(psDroid);

	if (!isVtolDroid(psDroid))
	{
		return false;
	}
	if (psDroid->droidType != DROID_WEAPON)
	{
		return false;
	}

	for (i = 0; i < psDroid->numWeaps; i++)
	{
		if (asWeaponStats[psDroid->asWeaps[i].nStat].vtolAttackRuns > 0 &&
		    psDroid->sMove.iAttackRuns[i] > 0)
		{
			return false;
		}
	}

	return true;
}

// true if a vtol is waiting to be rearmed by a particular rearm pad
bool vtolReadyToRearm(DROID *psDroid, STRUCTURE *psStruct)
{
	BASE_OBJECT *psRearmPad;

	CHECK_DROID(psDroid);

	if (!isVtolDroid(psDroid)
	 || psDroid->action != DACTION_WAITFORREARM)
	{
		return false;
	}

	// If a unit has been ordered to rearm make sure it goes to the correct base
	psRearmPad = orderStateObj(psDroid, DORDER_REARM);
	if (psRearmPad
	 && (STRUCTURE*)psRearmPad != psStruct
	 && !vtolOnRearmPad((STRUCTURE*)psRearmPad, psDroid))
	{
		// target rearm pad is clear - let it go there
		return false;
	}

	if (vtolHappy(psDroid) &&
		vtolOnRearmPad(psStruct, psDroid))
	{
		// there is a vtol on the pad and this vtol is already rearmed
		// don't bother shifting the other vtol off
		return false;
	}

	if ((psDroid->psActionTarget[0] != NULL) &&
		(psDroid->psActionTarget[0]->cluster != psStruct->cluster))
	{
		// vtol is rearming at a different base
		return false;
	}

	return true;
}

// true if a vtol droid currently returning to be rearmed
bool vtolRearming(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	if (!isVtolDroid(psDroid))
	{
		return false;
	}
	if (psDroid->droidType != DROID_WEAPON)
	{
		return false;
	}

	if (psDroid->action == DACTION_MOVETOREARM ||
		psDroid->action == DACTION_WAITFORREARM ||
		psDroid->action == DACTION_MOVETOREARMPOINT ||
		psDroid->action == DACTION_WAITDURINGREARM)
	{
		return true;
	}

	return false;
}

// true if a droid is currently attacking
bool droidAttacking(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	//what about cyborgs?
	if (!(psDroid->droidType == DROID_WEAPON || psDroid->droidType == DROID_CYBORG ||
		psDroid->droidType == DROID_CYBORG_SUPER))
	{
		return false;
	}

	if (psDroid->action == DACTION_ATTACK ||
		psDroid->action == DACTION_MOVETOATTACK ||
		psDroid->action == DACTION_ROTATETOATTACK ||
		psDroid->action == DACTION_VTOLATTACK ||
		psDroid->action == DACTION_MOVEFIRE)
	{
		return true;
	}

	return false;
}

// see if there are any other vtols attacking the same target
// but still rearming
bool allVtolsRearmed(DROID *psDroid)
{
	DROID	*psCurr;
	bool	stillRearming;

	CHECK_DROID(psDroid);

	// ignore all non vtols
	if (!isVtolDroid(psDroid))
	{
		return true;
	}

	stillRearming = false;
	for (psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (vtolRearming(psCurr) &&
			psCurr->order.type == psDroid->order.type &&
			psCurr->order.psObj == psDroid->order.psObj)
		{
			stillRearming = true;
			break;
		}
	}

	return !stillRearming;
}


/*returns a count of the base number of attack runs for the weapon attached to the droid*/
//adds int weapon_slot
UWORD   getNumAttackRuns(DROID *psDroid, int weapon_slot)
{
	UWORD   numAttackRuns;

	ASSERT_OR_RETURN(0, isVtolDroid(psDroid), "not a VTOL Droid");

	/*if weapon attached to the droid is a salvo weapon, then number of shots that
	can be fired = vtolAttackRuns*numRounds */
	if (asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].reloadTime)
	{
		numAttackRuns = (UWORD)(asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].numRounds *
			asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].vtolAttackRuns);
	}
	else
	{
		numAttackRuns = asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].vtolAttackRuns;
	}

	return numAttackRuns;
}

/*Checks a vtol for being fully armed and fully repaired to see if ready to
leave reArm pad */
bool vtolHappy(const DROID* psDroid)
{
	unsigned int i;

	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, isVtolDroid(psDroid), "not a VTOL droid");

	if (psDroid->body < psDroid->originalBody)
	{
		// VTOLs with less health than their original aren't happy
		return false;
	}

	if (psDroid->droidType != DROID_WEAPON)
	{
		// Not an armed droid, so don't check the (non-existent) weapons
		return true;
	}

	/* NOTE: Previous code (r5410) returned false if a droid had no weapon,
	 *       which IMO isn't correct, but might be expected behaviour. I'm
	 *       also not sure if weapon droids (see the above droidType check)
	 *       can even have zero weapons. -- Giel
	 */
	ASSERT_OR_RETURN(false, psDroid->numWeaps > 0, "VTOL weapon droid without weapons found!");

	//check full complement of ammo
	for (i = 0; i < psDroid->numWeaps; ++i)
	{
		if (asWeaponStats[psDroid->asWeaps[i].nStat].vtolAttackRuns > 0
		 && psDroid->sMove.iAttackRuns[i] != 0)
		{
			return false;
		}
	}

	return true;
}

/*checks if the droid is a VTOL droid and updates the attack runs as required*/
void updateVtolAttackRun(DROID *psDroid , int weapon_slot)
{
	if (isVtolDroid(psDroid))
	{
		if (psDroid->numWeaps > 0)
		{
			if (asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].vtolAttackRuns > 0)
			{
				psDroid->sMove.iAttackRuns[weapon_slot]++;
				if (psDroid->sMove.iAttackRuns[weapon_slot] == getNumAttackRuns(psDroid, weapon_slot))
				{
					psDroid->asWeaps[weapon_slot].ammo = 0;
				}
				//quick check doesn't go over limit
				ASSERT( psDroid->sMove.iAttackRuns[weapon_slot] < UWORD_MAX, "too many attack runs");
			}
		}
	}
}

/*this mends the VTOL when it has been returned to home base whilst on an
offworld mission*/
void mendVtol(DROID *psDroid)
{
	UBYTE	i;
	ASSERT_OR_RETURN( , vtolEmpty(psDroid), "droid is not an empty weapon VTOL!");

	CHECK_DROID(psDroid);

	/* set rearm value to no runs made */
	for (i = 0;i < psDroid->numWeaps;i++)
	{
		psDroid->sMove.iAttackRuns[i] = 0;
		//reset ammo and lastTimeFired
		psDroid->asWeaps[i].ammo = asWeaponStats[psDroid->
			asWeaps[i].nStat].numRounds;
		psDroid->asWeaps[i].lastFired = 0;
	}
	/* set droid points to max */
	psDroid->body = psDroid->originalBody;

	CHECK_DROID(psDroid);
}

//assign rearmPad to the VTOL
void assignVTOLPad(DROID *psNewDroid, STRUCTURE *psReArmPad)
{
	ASSERT_OR_RETURN( , isVtolDroid(psNewDroid), "not a vtol droid");
	ASSERT_OR_RETURN( ,  psReArmPad->type == OBJ_STRUCTURE
		&& psReArmPad->pStructureType->type == REF_REARM_PAD, "not a ReArm Pad" );

	setDroidBase(psNewDroid, psReArmPad);
}

/*compares the droid sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
bool droidSensorDroidWeapon(BASE_OBJECT *psObj, DROID *psDroid)
{
	SENSOR_STATS	*psStats = NULL;
	int compIndex;

	CHECK_DROID(psDroid);

	if(!psObj || !psDroid)
	{
		return false;
	}

	//first check if the object is a droid or a structure
	if ( (psObj->type != OBJ_DROID) &&
		 (psObj->type != OBJ_STRUCTURE) )
	{
		return false;
	}
	//check same player
	if (psObj->player != psDroid->player)
	{
		return false;
	}
	//check obj is a sensor droid/structure
	switch (psObj->type)
	{
	case OBJ_DROID:
		if (((DROID *)psObj)->droidType != DROID_SENSOR &&
			((DROID *)psObj)->droidType != DROID_COMMAND)
		{
			return false;
		}
		compIndex = ((DROID *)psObj)->asBits[COMP_SENSOR].nStat;
		ASSERT_OR_RETURN( false, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = asSensorStats + compIndex;
		break;
	case OBJ_STRUCTURE:
		psStats = ((STRUCTURE *)psObj)->pStructureType->pSensor;
		if ((psStats == NULL) ||
			(psStats->location != LOC_TURRET))
		{
			return false;
		}
		break;
	default:
		break;
	}

	//check droid is a weapon droid - or Cyborg!!
	if (!(psDroid->droidType == DROID_WEAPON || psDroid->droidType ==
		DROID_CYBORG || psDroid->droidType == DROID_CYBORG_SUPER))
	{
		return false;
	}

	//finally check the right droid/sensor combination
	// check vtol droid with commander
	if ((isVtolDroid(psDroid) || !proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
		psObj->type == OBJ_DROID && ((DROID *)psObj)->droidType == DROID_COMMAND)
	{
		return true;
	}

	//check vtol droid with vtol sensor
	if (isVtolDroid(psDroid) && psDroid->asWeaps[0].nStat > 0)
	{
		if (psStats->type == VTOL_INTERCEPT_SENSOR || psStats->type == VTOL_CB_SENSOR || psStats->type == SUPER_SENSOR || psStats->type == RADAR_DETECTOR_SENSOR)
		{
			return true;
		}
		return false;
	}

	// Check indirect weapon droid with standard/CB/radar detector sensor
	if (!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat))
	{
		if (psStats->type == STANDARD_SENSOR ||	psStats->type == INDIRECT_CB_SENSOR || psStats->type == SUPER_SENSOR || psStats->type == RADAR_DETECTOR_SENSOR)
		{
			return true;
		}
		return false;
	}
	return false;
}

// return whether a droid has a CB sensor on it
bool cbSensorDroid(DROID *psDroid)
{
	if (psDroid->droidType != DROID_SENSOR)
	{
		return false;
	}

	/*Super Sensor works as any type*/
	if (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		VTOL_CB_SENSOR ||
		asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		INDIRECT_CB_SENSOR ||
		asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		SUPER_SENSOR)
	{
		return true;
	}

	return false;
}

// return whether a droid has a standard sensor on it (standard, VTOL strike, or wide spectrum)
bool standardSensorDroid(DROID *psDroid)
{
	if (psDroid->droidType != DROID_SENSOR)
	{
		return false;
	}
	
	/*Super Sensor works as any type*/
	if (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		VTOL_INTERCEPT_SENSOR ||
		asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		STANDARD_SENSOR ||
		asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
		SUPER_SENSOR)
	{
		return true;
	}
	
	return false;
}

// ////////////////////////////////////////////////////////////////////////////
// give a droid from one player to another - used in Electronic Warfare and multiplayer
//returns the droid created - for single player
DROID * giftSingleDroid(DROID *psD, UDWORD to)
{
	DROID_TEMPLATE	sTemplate;
	DROID		*psNewDroid, *psCurr;
	STRUCTURE	*psStruct;
	UDWORD		body, armourK, armourH;
	int them = 0;

	CHECK_DROID(psD);

	if(psD->player == to)
	{
		return psD;
	}

	// FIXME: why completely separate code paths for multiplayer and single player?? - Per

	if (bMultiPlayer)
	{
		bool tooMany = false;

		incNumDroids(to);
		tooMany = tooMany || getNumDroids(to) > getMaxDroids(to);
		if (psD->droidType == DROID_CYBORG_CONSTRUCT || psD->droidType == DROID_CONSTRUCT)
		{
			incNumConstructorDroids(to);
			tooMany = tooMany || getNumConstructorDroids(to) > MAX_CONSTRUCTOR_DROIDS;
		}
		else if (psD->droidType == DROID_COMMAND)
		{
			incNumCommandDroids(to);
			tooMany = tooMany || getNumCommandDroids(to) > MAX_COMMAND_DROIDS;
		}

		if (tooMany)
		{
			if (to == selectedPlayer)
			{
				CONPRINTF(ConsoleString, (ConsoleString, _("%s wanted to give you a %s but you have too many!"), getPlayerName(psD->player), psD->aName));
			}
			else if(psD->player == selectedPlayer)
			{
				CONPRINTF(ConsoleString, (ConsoleString, _("You wanted to give %s a %s but they have too many!"), getPlayerName(to), psD->aName));
			}
			return NULL;
		}

		// reset order
		orderDroid(psD, DORDER_STOP, ModeQueue);

		visRemoveVisibility((BASE_OBJECT *)psD);

		if (droidRemove(psD, apsDroidLists)) 		// remove droid from one list
		{
			if (!isHumanPlayer(psD->player))
			{
				droidSetName(psD, "Enemy Unit");
			}

			// if successfully removed the droid from the players list add it to new player's list
			psD->selected	= false;
			psD->player	= to;		// move droid

			addDroid(psD, apsDroidLists);	// add to other list.

			// the new player may have different default sensor/ecm/repair components
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
		else
		{
			// if we couldn't remove it, then get rid of it.
			return NULL;
		}
		// add back into cluster system
		clustNewDroid(psD);

		// Update visibility
		visTilesUpdate((BASE_OBJECT*)psD);

		// check through the players, and our allies, list of droids to see if any are targetting it
		for (them = 0; them < MAX_PLAYERS; them++)
		{
			if (!aiCheckAlliances(them, to))	// scan all the droid list for ALLIANCE_FORMED (yes, we have a alliance with ourselves)
			{
				continue;
			}

			for (psCurr = apsDroidLists[them]; psCurr != NULL; psCurr = psCurr->psNext)
			{
				if (psCurr->order.psObj == psD || psCurr->psActionTarget[0] == psD)
				{
					orderDroid(psCurr, DORDER_STOP, ModeQueue);
				}
				// check through order list
				orderClearTargetFromDroidList(psCurr, (BASE_OBJECT *)psD);
			}
		}

		for (them = 0; them < MAX_PLAYERS; them++)
		{
			if (!aiCheckAlliances(them, to))	// scan all the droid list for ALLIANCE_FORMED (yes, we have a alliance with ourselves)
			{
				continue;
			}

			// check through the players list, and our allies, of structures to see if any are targetting it
			for (psStruct = apsStructLists[them]; psStruct != NULL; psStruct = psStruct->psNext)
			{
				if (psStruct->psTarget[0] == (BASE_OBJECT *)psD)
				{
					psStruct->psTarget[0] = NULL;
				}
			}
		}
		// skirmish callback!
		psScrCBDroidTaken = psD;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_UNITTAKEOVER);
		psScrCBDroidTaken = NULL;

		return NULL;
	}
	else
	{
		uint32_t numKills;
		int32_t x, y;
		uint16_t direction;

		// got to destroy the droid and build another since there are too many complications re order/action!

		// create a template based on the droid
		templateSetParts(psD, &sTemplate);

		// copy the name across
		sstrcpy(sTemplate.aName, psD->aName);

		x = psD->pos.x;
		y = psD->pos.y;
		body = psD->body;
		armourK = psD->armour[WC_KINETIC];
		armourH = psD->armour[WC_HEAT];
		numKills = psD->experience;
		direction = psD->rot.direction;
		// only play the sound if unit being taken over is selectedPlayer's but not going to the selectedPlayer
		if (psD->player == selectedPlayer && to != selectedPlayer)
		{
			scoreUpdateVar(WD_UNITS_LOST);
			audio_QueueTrackPos( ID_SOUND_NEXUS_UNIT_ABSORBED, x, y, psD->pos.z );
		}
		// make the old droid vanish
		vanishDroid(psD);
		// create a new droid
		psNewDroid = buildDroid(&sTemplate, x, y, to, false, NULL);
		ASSERT(psNewDroid != NULL, "unable to build a unit");
		if (psNewDroid)
		{
			addDroid(psNewDroid, apsDroidLists);
			psNewDroid->body = body;
			psNewDroid->armour[WC_KINETIC] = armourK;
			psNewDroid->armour[WC_HEAT] = armourH;
			psNewDroid->experience = numKills;
			psNewDroid->rot.direction = direction;
			if (!(psNewDroid->droidType == DROID_PERSON || cyborgDroid(psNewDroid) || psNewDroid->droidType == DROID_TRANSPORTER))
			{
				updateDroidOrientation(psNewDroid);
			}
		}
		return psNewDroid;
	}
}

/*calculates the electronic resistance of a droid based on its experience level*/
SWORD   droidResistance(DROID *psDroid)
{
	unsigned resistance;

	CHECK_DROID(psDroid);

	resistance = psDroid->experience / (65536/DROID_RESISTANCE_FACTOR);

	//ensure base minimum in MP before the upgrade effect
	if (bMultiPlayer)
	{
		//ensure resistance is a base minimum
		resistance = MAX(resistance, DROID_RESISTANCE_FACTOR);
	}

	//structure resistance upgrades are passed on to droids
	resistance = resistance + resistance*asStructureUpgrade[psDroid->player].resistance / 100;

	//ensure resistance is a base minimum
	resistance = MAX(resistance, DROID_RESISTANCE_FACTOR);

	return MIN(resistance, INT16_MAX);
}

/*this is called to check the weapon is 'allowed'. Check if VTOL, the weapon is
direct fire. Also check numVTOLattackRuns for the weapon is not zero - return
true if valid weapon*/
/* this will be buggy if the droid being checked has both AA weapon and non-AA weapon
Cannot think of a solution without adding additional return value atm.
*/
bool checkValidWeaponForProp(DROID_TEMPLATE *psTemplate)
{
	PROPULSION_STATS	*psPropStats;

	//check propulsion stat for vtol
	psPropStats = asPropulsionStats + psTemplate->asParts[COMP_PROPULSION];

	ASSERT_OR_RETURN(false, psPropStats != NULL, "invalid propulsion stats pointer");

	// if there are no weapons, then don't even bother continuing
	if (psTemplate->numWeaps == 0)
	{
		return false;
	}

	if (asPropulsionTypes[psPropStats->propulsionType].travel == AIR)
	{
		//check weapon stat for indirect
		if (!proj_Direct(asWeaponStats + psTemplate->asWeaps[0])
			|| !asWeaponStats[psTemplate->asWeaps[0]].vtolAttackRuns)
		{
			return false;
		}
	}
	else
	{
		// VTOL weapons do not go on non-AIR units.
		if ( asWeaponStats[psTemplate->asWeaps[0]].vtolAttackRuns )
		{
			return false;
		}
	}

	//also checks that there is no other system component
	if (psTemplate->asParts[COMP_BRAIN] != 0
		&& asWeaponStats[psTemplate->asWeaps[0]].weaponSubClass != WSC_COMMAND)
	{
		assert(false);
		return false;
	}

	return true;
}




// Select a droid and do any necessary housekeeping.
//
void SelectDroid(DROID *psDroid)
{
	// we shouldn't ever control the transporter in SP games
	if (psDroid->droidType != DROID_TRANSPORTER || bMultiPlayer)
	{
		psDroid->selected = true;
		intRefreshScreen();
	}
}


// De-select a droid and do any necessary housekeeping.
//
void DeSelectDroid(DROID *psDroid)
{
	psDroid->selected = false;
	intRefreshScreen();
}

/** Callback function for stopped audio tracks
 *  Sets the droid's current track id to NO_SOUND
 *  \return true on success, false on failure
 */
bool droidAudioTrackStopped( void *psObj )
{
	DROID	*psDroid;

	psDroid = (DROID*)psObj;
	if (psDroid == NULL)
	{
		debug( LOG_ERROR, "droid pointer invalid" );
		return false;
	}

	if ( psDroid->type != OBJ_DROID || psDroid->died )
	{
		return false;
	}

	psDroid->iAudioID = NO_SOUND;
	return true;
}

/*returns true if droid type is one of the Cyborg types*/
bool cyborgDroid(const DROID* psDroid)
{
	return (psDroid->droidType == DROID_CYBORG
		 || psDroid->droidType == DROID_CYBORG_CONSTRUCT
	 || psDroid->droidType == DROID_CYBORG_REPAIR
	 || psDroid->droidType == DROID_CYBORG_SUPER);
}

bool isConstructionDroid(DROID const *psDroid)
{
	return psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT;
}

bool isConstructionDroid(BASE_OBJECT const *psObject)
{
	return isDroid(psObject) && isConstructionDroid(castDroid(psObject));
}

bool droidOnMap(const DROID *psDroid)
{
	if (psDroid->died == NOT_CURRENT_LIST || psDroid->droidType == DROID_TRANSPORTER
		|| psDroid->pos.x == INVALID_XY || psDroid->pos.y == INVALID_XY || missionIsOffworld()
		|| mapHeight == 0)
	{
		// Off world or on a transport or is a transport or in mission list, or on a mission, or no map - ignore
		return true;
	}
	return worldOnMap(psDroid->pos.x, psDroid->pos.y);
}

/** Teleport a droid to a new position on the map */
void droidSetPosition(DROID *psDroid, int x, int y)
{
	psDroid->pos.x = x;
	psDroid->pos.y = y;
	psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
	initDroidMovement(psDroid);
	visTilesUpdate((BASE_OBJECT *)psDroid);
}

/** Check validity of a droid. Crash hard if it fails. */
void checkDroid(const DROID *droid, const char *const location, const char *function, const int recurse)
{
	if (recurse < 0)
	{
		return;
	}

	ASSERT_HELPER(droid != NULL, location, function, "CHECK_DROID: NULL pointer");
	ASSERT_HELPER(droid->type == OBJ_DROID, location, function, "CHECK_DROID: Not droid (type %d)", (int)droid->type);
	ASSERT_HELPER(droid->numWeaps <= DROID_MAXWEAPS, location, function, "CHECK_DROID: Bad number of droid weapons %d", (int)droid->numWeaps);
	ASSERT_HELPER((unsigned)droid->listSize <= droid->asOrderList.size() && (unsigned)droid->listPendingBegin <= droid->asOrderList.size(), location, function, "CHECK_DROID: Bad number of droid orders %d %d %d", (int)droid->listSize, (int)droid->listPendingBegin, (int)droid->asOrderList.size());
	ASSERT_HELPER(droid->player < MAX_PLAYERS, location, function, "CHECK_DROID: Bad droid owner %d", (int)droid->player);
	ASSERT_HELPER(droidOnMap(droid), location, function, "CHECK_DROID: Droid off map");
	ASSERT_HELPER(droid->body <= droid->originalBody, location, function, "CHECK_DROID: More body points (%u) than original body points (%u).", (unsigned)droid->body, (unsigned)droid->originalBody);

	for (int i = 0; i < DROID_MAXWEAPS; ++i)
	{
		ASSERT_HELPER(droid->asWeaps[i].lastFired <= gameTime, location, function, "CHECK_DROID: Bad last fired time for turret %u", i);
	}
}

int droidSqDist(DROID *psDroid, BASE_OBJECT *psObj)
{
	PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;

	if (!fpathCheck(psDroid->pos, psObj->pos, psPropStats->propulsionType))
	{
		return -1;
	}
	return objPosDiffSq(psDroid->pos, psObj->pos);
}
