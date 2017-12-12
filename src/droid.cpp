/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/netplay/netplay.h"

#include "objects.h"
#include "loop.h"
#include "visibility.h"
#include "map.h"
#include "droid.h"
#include "hci.h"
#include "power.h"
#include "effects.h"
#include "feature.h"
#include "action.h"
#include "order.h"
#include "move.h"
#include "geometry.h"
#include "display.h"
#include "console.h"
#include "component.h"
#include "lighting.h"
#include "multiplay.h"
#include "warcam.h"
#include "display3d.h"
#include "group.h"
#include "text.h"
#include "scriptcb.h"
#include "cmddroid.h"
#include "fpath.h"
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
static std::priority_queue<int> recycled_experience[MAX_PLAYERS];

/** Height the transporter hovers at above the terrain. */
#define TRANSPORTER_HOVER_HEIGHT	10

// the structure that was last hit
DROID	*psLastDroidHit;

//determines the best IMD to draw for the droid - A TEMP MEASURE!
static void groupConsoleInformOfSelection(UDWORD groupNumber);
static void groupConsoleInformOfCreation(UDWORD groupNumber);
static void groupConsoleInformOfCentering(UDWORD groupNumber);

static void droidUpdateDroidSelfRepair(DROID *psRepairDroid);
static UDWORD calcDroidBaseBody(DROID *psDroid);

void cancelBuild(DROID *psDroid)
{
	if (psDroid->order.type == DORDER_NONE || psDroid->order.type == DORDER_PATROL || psDroid->order.type == DORDER_HOLD || psDroid->order.type == DORDER_SCOUT || psDroid->order.type == DORDER_GUARD)
	{
		objTrace(psDroid->id, "Droid build action cancelled");
		psDroid->order.psObj = nullptr;
		psDroid->action = DACTION_NONE;
		setDroidActionTarget(psDroid, nullptr, 0);
		return;  // Don't cancel orders.
	}

	if (orderDroidList(psDroid))
	{
		objTrace(psDroid->id, "Droid build order cancelled - changing to next order");
	}
	else
	{
		objTrace(psDroid->id, "Droid build order cancelled");
		psDroid->action = DACTION_NONE;
		psDroid->order = DroidOrder(DORDER_NONE);
		setDroidActionTarget(psDroid, nullptr, 0);

		/* Notify scripts we just became idle */
		psScrCBOrderDroid = psDroid;
		psScrCBOrder = psDroid->order.type;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_REACH_LOCATION);
		psScrCBOrderDroid = nullptr;
		psScrCBOrder = DORDER_NONE;

		triggerEventDroidIdle(psDroid);
	}
}

static void droidBodyUpgrade(DROID *psDroid)
{
	const int factor = 10000; // use big numbers to scare away rounding errors
	int prev = psDroid->originalBody;
	psDroid->originalBody = calcDroidBaseBody(psDroid);
	int increase = psDroid->originalBody * factor / prev;
	psDroid->body = MIN(psDroid->originalBody, (psDroid->body * increase) / factor + 1);
	if (isTransporter(psDroid))
	{
		for (DROID *psCurr = psDroid->psGroup->psList; psCurr != nullptr; psCurr = psCurr->psGrpNext)
		{
			if (psCurr != psDroid)
			{
				droidBodyUpgrade(psCurr);
			}
		}
	}
}

// initialise droid module
bool droidInit()
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		recycled_experience[i] = std::priority_queue <int>(); // clear it
	}
	psLastDroidHit = nullptr;

	return true;
}

int droidReloadBar(BASE_OBJECT *psObj, WEAPON *psWeap, int weapon_slot)
{
	WEAPON_STATS *psStats;
	bool bSalvo;
	int firingStage, interval;

	if (psWeap->nStat == 0)	// no weapon
	{
		return -1;
	}
	psStats = asWeaponStats + psWeap->nStat;

	/* Justifiable only when greater than a one second reload or intra salvo time  */
	bSalvo = (psStats->upgrade[psObj->player].numRounds > 1);
	if ((bSalvo && psStats->upgrade[psObj->player].reloadTime > GAME_TICKS_PER_SEC)
	    || psStats->upgrade[psObj->player].firePause > GAME_TICKS_PER_SEC
	    || (psObj->type == OBJ_DROID && isVtolDroid((DROID *)psObj)))
	{
		if (psObj->type == OBJ_DROID && isVtolDroid((DROID *)psObj))
		{
			//deal with VTOLs
			firingStage = getNumAttackRuns((DROID *)psObj, weapon_slot) - ((DROID *)psObj)->asWeaps[weapon_slot].usedAmmo;

			//compare with max value
			interval = getNumAttackRuns((DROID *)psObj, weapon_slot);
		}
		else
		{
			firingStage = gameTime - psWeap->lastFired;
			interval = bSalvo ? weaponReloadTime(psStats, psObj->player) : weaponFirePause(psStats, psObj->player);
		}
		if (firingStage < interval && interval > 0)
		{
			return PERCENT(firingStage, interval);
		}
		return 100;
	}
	return -1;
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
int32_t droidDamage(DROID *psDroid, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime, bool isDamagePerSecond, int minDamage)
{
	int32_t relativeDamage;

	CHECK_DROID(psDroid);

	// VTOLs (and transporters in MP) on the ground take triple damage
	if ((isVtolDroid(psDroid) || (isTransporter(psDroid) && bMultiPlayer)) && (psDroid->sMove.Status == MOVEINACTIVE))
	{
		damage *= 3;
	}

	relativeDamage = objDamage(psDroid, damage, psDroid->originalBody, weaponClass, weaponSubClass, isDamagePerSecond, minDamage);

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
		// FIXME: When we fix campaign scripts to use DROID_SUPERTRANSPORTER
		if ((game.type == CAMPAIGN) && !bMultiPlayer && (psDroid->droidType == DROID_TRANSPORTER))
		{
			debug(LOG_ATTACK, "Transport(%d) saved from death--since it should never die (SP only)", psDroid->id);
			psDroid->body = 1;
			return 0;
		}

		// Droid destroyed
		debug(LOG_ATTACK, "droid (%d): DESTROYED", psDroid->id);

		// Deal with score increase/decrease and messages to the player
		if (psDroid->player == selectedPlayer)
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("Unit Lost!")));
			scoreUpdateVar(WD_UNITS_LOST);
			audio_QueueTrackMinDelayPos(ID_SOUND_UNIT_DESTROYED, UNIT_LOST_DELAY,
			                            psDroid->pos.x, psDroid->pos.y, psDroid->pos.z);
		}
		else
		{
			scoreUpdateVar(WD_UNITS_KILLED);
		}

		// Do we have a dying animation?
		if (psDroid->sDisplay.imd->objanimpie[ANIM_EVENT_DYING] && psDroid->animationEvent != ANIM_EVENT_DYING)
		{
			debug(LOG_DEATH, "%s droid %d (%p) is starting death animation", objInfo(psDroid), (int)psDroid->id, psDroid);
			psDroid->timeAnimationStarted = gameTime;
			psDroid->animationEvent = ANIM_EVENT_DYING;
			if (psDroid->droidType == DROID_PERSON)
			{
				// NOTE: 3 types of screams are available ID_SOUND_BARB_SCREAM - ID_SOUND_BARB_SCREAM3
				audio_PlayObjDynamicTrack(psDroid, ID_SOUND_BARB_SCREAM + (rand() % 3), nullptr);
			}
		}
		// Otherwise use the default destruction animation
		else if (psDroid->animationEvent != ANIM_EVENT_DYING)
		{
			debug(LOG_DEATH, "%s droid %d (%p) is toast", objInfo(psDroid), (int)psDroid->id, psDroid);
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

DROID::DROID(uint32_t id, unsigned player)
	: BASE_OBJECT(OBJ_DROID, id, player)
	, droidType(DROID_ANY)
	, psGroup(nullptr)
	, psGrpNext(nullptr)
	, secondaryOrder(DSS_REPLEV_NEVER | DSS_ALEV_ALWAYS)
	, secondaryOrderPending(DSS_REPLEV_NEVER | DSS_ALEV_ALWAYS)
	, secondaryOrderPendingCount(0)
	, action(DACTION_NONE)
	, actionPos(0, 0)
{
	memset(aName, 0, sizeof(aName));
	memset(asBits, 0, sizeof(asBits));
	pos = Vector3i(0, 0, 0);
	rot = Vector3i(0, 0, 0);
	order.type = DORDER_NONE;
	order.pos = Vector2i(0, 0);
	order.pos2 = Vector2i(0, 0);
	order.direction = 0;
	order.psObj = nullptr;
	order.psStats = nullptr;
	sMove.asPath = nullptr;
	sMove.Status = MOVEINACTIVE;
	listSize = 0;
	listPendingBegin = 0;
	iAudioID = NO_SOUND;
	group = UBYTE_MAX;
	psBaseStruct = nullptr;
	sDisplay.frameNumber = 0;	// it was never drawn before
	for (unsigned vPlayer = 0; vPlayer < MAX_PLAYERS; ++vPlayer)
	{
		visible[vPlayer] = hasSharedVision(vPlayer, player) ? UINT8_MAX : 0;
	}
	memset(seenThisTick, 0, sizeof(seenThisTick));
	periodicalDamageStart = 0;
	periodicalDamage = 0;
	sDisplay.screenX = OFF_SCREEN;
	sDisplay.screenY = OFF_SCREEN;
	sDisplay.screenR = 0;
	sDisplay.imd = nullptr;
	illumination = UBYTE_MAX;
	resistance = ACTION_START_TIME;	// init the resistance to indicate no EW performed on this droid
	lastFrustratedTime = 0;		// make sure we do not start the game frustrated
}

/* DROID::~DROID: release all resources associated with a droid -
 * should only be called by objmem - use vanishDroid preferably
 */
DROID::~DROID()
{
	// Make sure to get rid of some final references in the sound code to this object first
	// In BASE_OBJECT::~BASE_OBJECT() is too late for this, since some callbacks require us to still be a DROID.
	audio_RemoveObj(this);

	DROID *psDroid = this;
	DROID	*psCurr, *psNext;

	if (isTransporter(psDroid))
	{
		if (psDroid->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDroid->psGroup->psList; psCurr != nullptr && psCurr != psDroid; psCurr = psNext)
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
}

std::priority_queue<int> copy_experience_queue(int player)
{
	return recycled_experience[player];
}

void add_to_experience_queue(int player, int value)
{
	recycled_experience[player].push(value);
}

// recycle a droid (retain it's experience and some of it's cost)
void recycleDroid(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	// store the droids kills
	if (psDroid->experience > 0)
	{
		recycled_experience[psDroid->player].push(psDroid->experience);
	}

	// return part of the cost of the droid
	int cost = calcDroidPower(psDroid);
	cost = (cost / 2) * psDroid->body / psDroid->originalBody;
	addPower(psDroid->player, (UDWORD)cost);

	// hide the droid
	memset(psDroid->visible, 0, sizeof(psDroid->visible));
	// stop any group moral checks
	if (psDroid->psGroup)
	{
		psDroid->psGroup->remove(psDroid);
	}

	triggerEvent(TRIGGER_OBJECT_RECYCLED, psDroid);
	vanishDroid(psDroid);

	Vector3i position = psDroid->pos.xzy;
	addEffect(&position, EFFECT_EXPLOSION, EXPLOSION_TYPE_DISCOVERY, false, nullptr, false, gameTime - deltaGameTime + 1);

	CHECK_DROID(psDroid);
}


bool removeDroidBase(DROID *psDel)
{
	DROID	*psCurr, *psNext;
	STRUCTURE	*psStruct;

	CHECK_DROID(psDel);

	if (isDead((BASE_OBJECT *)psDel))
	{
		// droid has already been killed, quit
		return true;
	}

	syncDebugDroid(psDel, '#');

	//kill all the droids inside the transporter
	if (isTransporter(psDel))
	{
		if (psDel->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDel->psGroup->psList; psCurr != nullptr && psCurr != psDel; psCurr = psNext)
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
			psDel->psGroup = nullptr;
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
		psDel->psGroup = nullptr;
	}

	/* Put Deliv. Pts back into world when a command droid dies */
	if (psDel->droidType == DROID_COMMAND)
	{
		for (psStruct = apsStructLists[psDel->player]; psStruct; psStruct = psStruct->psNext)
		{
			// alexl's stab at a right answer.
			if (StructIsFactory(psStruct)
			    && psStruct->pFunctionality->factory.psCommander == psDel)
			{
				assignFactoryCommandDroid(psStruct, nullptr);
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
			for (DROID *psDroid = apsDroidLists[psDel->player]; psDroid != nullptr; psDroid = psDroid->psNext)
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
	return true;
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

	if (psDel->animationEvent != ANIM_EVENT_DYING)
	{
		compPersonToBits(psDel);
	}

	/* if baba then squish */
	if (psDel->droidType == DROID_PERSON && psDel->visible[selectedPlayer])
	{
		// The barbarian has been run over ...
		audio_PlayStaticTrack(psDel->pos.x, psDel->pos.y, ID_SOUND_BARB_SQUISH);
	}
	else if (psDel->visible[selectedPlayer])
	{
		destroyFXDroid(psDel, impactTime);
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;
		pos.y = psDel->pos.z;
		if (psDel->droidType == DROID_SUPERTRANSPORTER)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LARGE, false, nullptr, 0, impactTime);
		}
		else
		{
			addEffect(&pos, EFFECT_DESTRUCTION, DESTRUCTION_TYPE_DROID, false, nullptr, 0, impactTime);
		}
		audio_PlayStaticTrack(psDel->pos.x, psDel->pos.y, ID_SOUND_EXPLOSION);
	}
}

bool destroyDroid(DROID *psDel, unsigned impactTime)
{
	ASSERT(gameTime - deltaGameTime < impactTime, "Expected %u < %u, gameTime = %u, bad impactTime", gameTime - deltaGameTime, impactTime, gameTime);

	if (psDel->lastHitWeapon == WSC_LAS_SAT)		// darken tile if lassat.
	{
		UDWORD width, breadth, mapX, mapY;
		MAPTILE	*psTile;

		mapX = map_coord(psDel->pos.x);
		mapY = map_coord(psDel->pos.y);
		for (width = mapX - 1; width <= mapX + 1; width++)
		{
			for (breadth = mapY - 1; breadth <= mapY + 1; breadth++)
			{
				psTile = mapTile(width, breadth);
				if (TEST_TILE_VISIBLE(selectedPlayer, psTile))
				{
					psTile->illumination /= 2;
				}
			}
		}
	}

	removeDroidFX(psDel, impactTime);
	removeDroidBase(psDel);
	psDel->died = impactTime;
	return true;
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

	if (isDead((BASE_OBJECT *) psDroid))
	{
		// droid has already been killed, quit
		return false;
	}

	// leave the current group if any - not if its a Transporter droid
	if (!isTransporter(psDroid) && psDroid->psGroup)
	{
		psDroid->psGroup->remove(psDroid);
		psDroid->psGroup = nullptr;
	}

	// reset the baseStruct
	setDroidBase(psDroid, nullptr);

	// remove the droid from the cluster systerm
	clustRemoveObject((BASE_OBJECT *)psDroid);

	removeDroid(psDroid, pList);

	if (psDroid->player == selectedPlayer)
	{
		intRefreshScreen();
	}

	return true;
}

void _syncDebugDroid(const char *function, DROID const *psDroid, char ch)
{
	int list[] =
	{
		ch,

		(int)psDroid->id,

		psDroid->player,
		psDroid->pos.x, psDroid->pos.y, psDroid->pos.z,
		psDroid->rot.direction, psDroid->rot.pitch, psDroid->rot.roll,
		(int)psDroid->order.type, psDroid->order.pos.x, psDroid->order.pos.y, psDroid->listSize,
		(int)psDroid->action,
		(int)psDroid->secondaryOrder,
		(int)psDroid->body,
		(int)psDroid->sMove.Status,
		psDroid->sMove.speed, psDroid->sMove.moveDir,
		psDroid->sMove.pathIndex, psDroid->sMove.numPoints,
		psDroid->sMove.src.x, psDroid->sMove.src.y, psDroid->sMove.target.x, psDroid->sMove.target.y, psDroid->sMove.destination.x, psDroid->sMove.destination.y,
		psDroid->sMove.bumpDir, (int)psDroid->sMove.bumpTime, psDroid->sMove.lastBump, psDroid->sMove.pauseTime, psDroid->sMove.bumpPos.x, psDroid->sMove.bumpPos.y, (int)psDroid->sMove.shuffleStart,
		(int)psDroid->experience,
	};
	_syncDebugIntList(function, "%c droid%d = p%d;pos(%d,%d,%d),rot(%d,%d,%d),order%d(%d,%d)^%d,action%d,secondaryOrder%X,body%d,sMove(status%d,speed%d,moveDir%d,path%d/%d,src(%d,%d),target(%d,%d),destination(%d,%d),bump(%d,%d,%d,%d,(%d,%d),%d)),exp%u", list, ARRAY_SIZE(list));
}

/* The main update routine for all droids */
void droidUpdate(DROID *psDroid)
{
	Vector3i        dv;
	UDWORD          percentDamage, emissionInterval;
	BASE_OBJECT     *psBeingTargetted = nullptr;
	unsigned        i;

	CHECK_DROID(psDroid);

#ifdef DEBUG
	// Check that we are (still) in the sensor list
	if (psDroid->droidType == DROID_SENSOR)
	{
		BASE_OBJECT	*psSensor;

		for (psSensor = apsSensorList[0]; psSensor; psSensor = psSensor->psNextFunc)
		{
			if (psSensor == (BASE_OBJECT *)psDroid)
			{
				break;
			}
		}
		ASSERT(psSensor == (BASE_OBJECT *)psDroid, "%s(%p) not in sensor list!",
		       droidGetName(psDroid), psDroid);
	}
#endif

	syncDebugDroid(psDroid, '<');

	if (psDroid->flags.test(OBJECT_FLAG_DIRTY))
	{
		visTilesUpdate(psDroid);
		droidBodyUpgrade(psDroid);
		psDroid->flags.set(OBJECT_FLAG_DIRTY, false);
	}

	// Save old droid position, update time.
	psDroid->prevSpacetime = getSpacetime(psDroid);
	psDroid->time = gameTime;
	for (i = 0; i < MAX(1, psDroid->numWeaps); ++i)
	{
		psDroid->asWeaps[i].prevRot = psDroid->asWeaps[i].rot;
	}

	if (psDroid->animationEvent != ANIM_EVENT_NONE)
	{
		iIMDShape *imd = psDroid->sDisplay.imd->objanimpie[psDroid->animationEvent];
		if (imd && imd->objanimcycles > 0 && gameTime > psDroid->timeAnimationStarted + imd->objanimtime * imd->objanimcycles)
		{
			// Done animating (animation is defined by body - other components should follow suit)
			if (psDroid->animationEvent == ANIM_EVENT_DYING)
			{
				debug(LOG_DEATH, "%s (%d) died to burn anim (died=%d)", objInfo(psDroid), (int)psDroid->id, (int)psDroid->died);
				destroyDroid(psDroid, gameTime);
				return;
			}
			psDroid->animationEvent = ANIM_EVENT_NONE;
		}
	}
	else if (psDroid->animationEvent == ANIM_EVENT_DYING)
	{
		return; // rest below is irrelevant if dead
	}

	// update the cluster of the droid
	if ((psDroid->id + gameTime) / 2000 != (psDroid->id + gameTime - deltaGameTime) / 2000)
	{
		clustUpdateObject((BASE_OBJECT *)psDroid);
	}

	// ai update droid
	aiUpdateDroid(psDroid);

	// Update the droids order.
	orderUpdateDroid(psDroid);

	// update the action of the droid
	actionUpdateDroid(psDroid);

	syncDebugDroid(psDroid, 'M');

	// update the move system
	moveUpdateDroid(psDroid);

	/* Only add smoke if they're visible */
	if ((psDroid->visible[selectedPlayer]) && psDroid->droidType != DROID_PERSON)
	{
		// need to clip this value to prevent overflow condition
		percentDamage = 100 - clip(PERCENT(psDroid->body, psDroid->originalBody), 0, 100);

		// Is there any damage?
		if (percentDamage >= 25)
		{
			if (percentDamage >= 100)
			{
				percentDamage = 99;
			}

			emissionInterval = CALC_DROID_SMOKE_INTERVAL(percentDamage);

			int effectTime = std::max(gameTime - deltaGameTime + 1, psDroid->lastEmission + emissionInterval);
			if (gameTime >= effectTime)
			{
				dv.x = psDroid->pos.x + DROID_DAMAGE_SPREAD;
				dv.z = psDroid->pos.y + DROID_DAMAGE_SPREAD;
				dv.y = psDroid->pos.z;

				dv.y += (psDroid->sDisplay.imd->max.y * 2);
				addEffect(&dv, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING_SMALL, false, nullptr, 0, effectTime);
				psDroid->lastEmission = effectTime;
			}
		}
	}

	// -----------------
	/* Are we a sensor droid or a command droid? Show where we target for selectedPlayer. */
	if (psDroid->player == selectedPlayer && (psDroid->droidType == DROID_SENSOR || psDroid->droidType == DROID_COMMAND))
	{
		/* If we're attacking or sensing (observing), then... */
		if ((psBeingTargetted = orderStateObj(psDroid, DORDER_ATTACK))
		    || (psBeingTargetted = orderStateObj(psDroid, DORDER_OBSERVE)))
		{
			psBeingTargetted->flags.set(OBJECT_FLAG_TARGETED, true);
		}
	}
	// -----------------

	// See if we can and need to self repair.
	if (!isVtolDroid(psDroid) && psDroid->body < psDroid->originalBody && psDroid->asBits[COMP_REPAIRUNIT] != 0 && selfRepairEnabled(psDroid->player))
	{
		droidUpdateDroidSelfRepair(psDroid);
	}

	/* Update the fire damage data */
	if (psDroid->periodicalDamageStart != 0 && psDroid->periodicalDamageStart != gameTime - deltaGameTime)  // -deltaGameTime, since projectiles are updated after droids.
	{
		// The periodicalDamageStart has been set, but is not from the previous tick, so we must be out of the fire.
		psDroid->periodicalDamage = 0;  // Reset periodical damage done this tick.
		if (psDroid->periodicalDamageStart + BURN_TIME < gameTime)
		{
			// Finished periodical damaging.
			psDroid->periodicalDamageStart = 0;
		}
		else
		{
			// do hardcoded burn damage (this damage automatically applied after periodical damage finished)
			droidDamage(psDroid, BURN_DAMAGE, WC_HEAT, WSC_FLAME, gameTime - deltaGameTime / 2 + 1, true, BURN_MIN_DAMAGE);
		}
	}

	// At this point, the droid may be dead due to periodical damage or hardcoded burn damage.
	if (isDead((BASE_OBJECT *)psDroid))
	{
		return;
	}

	calcDroidIllumination(psDroid);

	// Check the resistance level of the droid
	if ((psDroid->id + gameTime) / 833 != (psDroid->id + gameTime - deltaGameTime) / 833)
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
	SDWORD	minX, maxX, maxY, x, y;

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
	for (; y <= maxY; y++)
	{
		for (x = minX; x <= maxX; x++)
		{
			if (TileHasStructure(mapTile((UWORD)x, (UWORD)y)) &&
			    getTileStructure(x, y) == (STRUCTURE *)psStruct)

			{
				return true;
			}
		}
	}

	return false;
}

static bool
droidCheckBuildStillInProgress(void *psObj)
{
	DROID	*psDroid;

	if (psObj == nullptr)
	{
		return false;
	}

	psDroid = (DROID *)psObj;
	CHECK_DROID(psDroid);

	if (!psDroid->died && psDroid->action == DACTION_BUILD)
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool
droidBuildStartAudioCallback(void *psObj)
{
	DROID	*psDroid;

	psDroid = (DROID *)psObj;

	if (psDroid == nullptr)
	{
		return true;
	}

	if (psDroid->visible[selectedPlayer])
	{
		audio_PlayObjDynamicTrack(psDroid, ID_SOUND_CONSTRUCTION_LOOP, droidCheckBuildStillInProgress);
	}

	return true;
}


/* Set up a droid to build a structure - returns true if successful */
DroidStartBuild droidStartBuild(DROID *psDroid)
{
	STRUCTURE *psStruct = nullptr;
	ASSERT_OR_RETURN(DroidStartBuildFailed, psDroid != nullptr, "Bad Droid");
	CHECK_DROID(psDroid);

	/* See if we are starting a new structure */
	if (psDroid->order.psObj == nullptr &&
	    (psDroid->order.type == DORDER_BUILD ||
	     psDroid->order.type == DORDER_LINEBUILD))
	{
		STRUCTURE_STATS *psStructStat = psDroid->order.psStats;

		ItemAvailability ia = (ItemAvailability)apStructTypeLists[psDroid->player][psStructStat - asStructureStats];
		if (ia != AVAILABLE && ia != REDUNDANT)
		{
			ASSERT(false, "Cannot build \"%s\" for player %d.", psStructStat->name.toUtf8().constData(), psDroid->player);
			intBuildFinished(psDroid);
			cancelBuild(psDroid);
			objTrace(psDroid->id, "DroidStartBuildFailed: not researched");
			return DroidStartBuildFailed;
		}

		//need to check structLimits have not been exceeded
		if (psStructStat->curCount[psDroid->player] >= psStructStat->upgrade[psDroid->player].limit)
		{
			intBuildFinished(psDroid);
			cancelBuild(psDroid);
			objTrace(psDroid->id, "DroidStartBuildFailed: structure limits");
			return DroidStartBuildFailed;
		}
		// Can't build on burning oil derricks.
		if (psStructStat->type == REF_RESOURCE_EXTRACTOR && fireOnLocation(psDroid->order.pos.x, psDroid->order.pos.y))
		{
			// Don't cancel build, since we can wait for it to stop burning.
			objTrace(psDroid->id, "DroidStartBuildPending: burning");
			return DroidStartBuildPending;
		}
		//ok to build
		psStruct = buildStructureDir(psStructStat, psDroid->order.pos.x, psDroid->order.pos.y, psDroid->order.direction, psDroid->player, false);
		if (!psStruct)
		{
			intBuildFinished(psDroid);
			cancelBuild(psDroid);
			objTrace(psDroid->id, "DroidStartBuildFailed: buildStructureDir failed");
			return DroidStartBuildFailed;
		}
		psStruct->body = (psStruct->body + 9) / 10;  // Structures start at 10% health. Round up.
	}
	else
	{
		/* Check the structure is still there to build (joining a partially built struct) */
		psStruct = castStructure(psDroid->order.psObj);
		if (psStruct == nullptr)
		{
			psStruct = castStructure(worldTile(psDroid->actionPos)->psObject);
		}
		if (psStruct && !droidNextToStruct(psDroid, psStruct))
		{
			/* Nope - stop building */
			debug(LOG_NEVER, "not next to structure");
			objTrace(psDroid->id, "DroidStartBuildSuccess: not next to structure");
		}
	}

	//check structure not already built, and we still 'own' it
	if (psStruct)
	{
		if (psStruct->status != SS_BUILT && aiCheckAlliances(psStruct->player, psDroid->player))
		{
			psDroid->actionStarted = gameTime;
			psDroid->actionPoints = 0;
			setDroidTarget(psDroid, psStruct);
			setDroidActionTarget(psDroid, psStruct, 0);
			objTrace(psDroid->id, "DroidStartBuild: set target");
		}

		if (psStruct->visible[selectedPlayer])
		{
			audio_PlayObjStaticTrackCallback(psDroid, ID_SOUND_CONSTRUCTION_START, droidBuildStartAudioCallback);
		}
	}
	CHECK_DROID(psDroid);

	objTrace(psDroid->id, "DroidStartBuildSuccess");
	return DroidStartBuildSuccess;
}

static void droidAddWeldSound(Vector3i iVecEffect)
{
	SDWORD		iAudioID;

	iAudioID = ID_SOUND_CONSTRUCTION_1 + (rand() % 4);

	audio_PlayStaticTrack(iVecEffect.x, iVecEffect.z, iAudioID);
}

static void addConstructorEffect(STRUCTURE *psStruct)
{
	UDWORD		widthRange, breadthRange;
	Vector3i temp;

	//FIXME
	if ((ONEINTEN) && (psStruct->visible[selectedPlayer]))
	{
		/* This needs fixing - it's an arse effect! */
		widthRange   = getStructureWidth(psStruct) * TILE_UNITS / 4;
		breadthRange = getStructureBreadth(psStruct) * TILE_UNITS / 4;
		temp.x = psStruct->pos.x + ((rand() % (2 * widthRange)) - widthRange);
		temp.y = map_TileHeight(map_coord(psStruct->pos.x), map_coord(psStruct->pos.y)) +
		         (psStruct->sDisplay.imd->max.y / 6);
		temp.z = psStruct->pos.y + ((rand() % (2 * breadthRange)) - breadthRange);
		if (rand() % 2)
		{
			droidAddWeldSound(temp);
		}
	}
}

/* Update a construction droid while it is building
   returns true while building continues */
bool droidUpdateBuild(DROID *psDroid)
{
	UDWORD		pointsToAdd, constructPoints;

	CHECK_DROID(psDroid);
	ASSERT_OR_RETURN(false, psDroid->action == DACTION_BUILD, "%s (order %s) has wrong action for construction: %s",
	                 droidGetName(psDroid), getDroidOrderName(psDroid->order.type), getDroidActionName(psDroid->action));

	STRUCTURE *psStruct = castStructure(psDroid->order.psObj);
	if (psStruct == nullptr)
	{
		// Target missing, stop trying to build it.
		psDroid->action = DACTION_NONE;
		return false;
	}

	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");
	ASSERT_OR_RETURN(false, psDroid->asBits[COMP_CONSTRUCT] < numConstructStats, "Invalid construct pointer for unit");

	// First check the structure hasn't been completed by another droid
	if (psStruct->status == SS_BUILT)
	{
		// Update the interface
		intBuildFinished(psDroid);
		// Check if line order build is completed, or we are not carrying out a line order build
		if (psDroid->order.type != DORDER_LINEBUILD ||
		    map_coord(psDroid->order.pos) == map_coord(psDroid->order.pos2))
		{
			cancelBuild(psDroid);
		}
		else
		{
			psDroid->action = DACTION_NONE;	// make us continue line build
			setDroidTarget(psDroid, nullptr);
			setDroidActionTarget(psDroid, nullptr, 0);
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
	                                    asBits[COMP_CONSTRUCT], psDroid->player);

	pointsToAdd = constructPoints * (gameTime - psDroid->actionStarted) /
	              GAME_TICKS_PER_SEC;

	structureBuild(psStruct, psDroid, pointsToAdd - psDroid->actionPoints, constructPoints);

	//store the amount just added
	psDroid->actionPoints = pointsToAdd;

	addConstructorEffect(psStruct);

	return true;
}

bool droidUpdateDemolishing(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->action == DACTION_DEMOLISH, "unit is not demolishing");
	STRUCTURE *psStruct = (STRUCTURE *)psDroid->order.psObj;
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");

	int constructRate = 5 * constructorPoints(asConstructStats + psDroid->asBits[COMP_CONSTRUCT], psDroid->player);
	int pointsToAdd = gameTimeAdjustedAverage(constructRate);

	structureDemolish(psStruct, psDroid, pointsToAdd);

	addConstructorEffect(psStruct);

	CHECK_DROID(psDroid);

	return true;
}

void droidStartAction(DROID *psDroid)
{
	psDroid->actionStarted = gameTime;
	psDroid->actionPoints  = 0;
}

/*continue restoring a structure*/
bool droidUpdateRestore(DROID *psDroid)
{
	STRUCTURE		*psStruct;
	UDWORD			pointsToAdd, restorePoints;
	WEAPON_STATS	*psStats;
	int compIndex;

	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->action == DACTION_RESTORE, "Unit is not restoring");
	psStruct = (STRUCTURE *)psDroid->order.psObj;
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "Target is not a structure");
	ASSERT_OR_RETURN(false, psDroid->asWeaps[0].nStat > 0, "Droid doesn't have any weapons");

	compIndex = psDroid->asWeaps[0].nStat;
	ASSERT_OR_RETURN(false, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
	psStats = asWeaponStats + compIndex;

	ASSERT_OR_RETURN(false, psStats->weaponSubClass == WSC_ELECTRONIC, "unit's weapon is not EW");

	restorePoints = calcDamage(weaponDamage(psStats, psDroid->player),
	                           psStats->weaponEffect, (BASE_OBJECT *)psStruct);

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
		addConsoleMessage(_("Structure Restored") , DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
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
			int recoilAmount = DEFAULT_RECOIL_TIME / 2 - abs(recoilTime - DEFAULT_RECOIL_TIME / 2);
			int maxRecoil = asWeaponStats[weapon.nStat].recoilValue;  // Max recoil is 1/10 of this value.
			return maxRecoil * recoilAmount / (DEFAULT_RECOIL_TIME / 2 * 10);
		}
		// Recoil effect is over.
	}
	return 0;
}


bool droidUpdateRepair(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->action == DACTION_REPAIR, "unit does not have repair order");
	STRUCTURE *psStruct = (STRUCTURE *)psDroid->psActionTarget[0];

	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "target is not a structure");
	int iRepairRate = constructorPoints(asConstructStats + psDroid->asBits[COMP_CONSTRUCT], psDroid->player);

	/* add points to structure */
	structureRepair(psStruct, psDroid, iRepairRate);

	/* if not finished repair return true else complete repair and return false */
	if (psStruct->body < structureBody(psStruct))
	{
		return true;
	}
	else
	{
		objTrace(psDroid->id, "Repaired of %s all done with %u", objInfo(psStruct), iRepairRate);
		return false;
	}
}

/*Updates a Repair Droid working on a damaged droid*/
static bool droidUpdateDroidRepairBase(DROID *psRepairDroid, DROID *psDroidToRepair)
{
	CHECK_DROID(psRepairDroid);

	int iRepairRateNumerator = repairPoints(asRepairStats + psRepairDroid->asBits[COMP_REPAIRUNIT], psRepairDroid->player);
	int iRepairRateDenominator = 1;

	//if self repair then add repair points depending on the time delay for the stat
	if (psRepairDroid == psDroidToRepair)
	{
		iRepairRateNumerator *= GAME_TICKS_PER_SEC;
		iRepairRateDenominator *= (asRepairStats + psRepairDroid->asBits[COMP_REPAIRUNIT])->time;
	}

	int iPointsToAdd = gameTimeAdjustedAverage(iRepairRateNumerator, iRepairRateDenominator);

	psDroidToRepair->body = clip(psDroidToRepair->body + iPointsToAdd, 0, psDroidToRepair->originalBody);

	/* add plasma repair effect whilst being repaired */
	if ((ONEINFIVE) && (psDroidToRepair->visible[selectedPlayer]))
	{
		Vector3i iVecEffect = (psDroidToRepair->pos + Vector3i(DROID_REPAIR_SPREAD, DROID_REPAIR_SPREAD, rand() % 8)).xzy;
		effectGiveAuxVar(90 + rand() % 20);
		addEffect(&iVecEffect, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, nullptr, 0, gameTime - deltaGameTime + 1 + rand() % deltaGameTime);
		droidAddWeldSound(iVecEffect);
	}

	CHECK_DROID(psRepairDroid);

	/* if not finished repair return true else complete repair and return false */
	return psDroidToRepair->body < psDroidToRepair->originalBody;
}

bool droidUpdateDroidRepair(DROID *psRepairDroid)
{
	ASSERT_OR_RETURN(false, psRepairDroid->action == DACTION_DROIDREPAIR, "Unit does not have unit repair order");
	ASSERT_OR_RETURN(false, psRepairDroid->asBits[COMP_REPAIRUNIT] != 0, "Unit does not have a repair turret");

	DROID *psDroidToRepair = (DROID *)psRepairDroid->psActionTarget[0];
	ASSERT_OR_RETURN(false, psDroidToRepair->type == OBJ_DROID, "Target is not a unit");

	return droidUpdateDroidRepairBase(psRepairDroid, psDroidToRepair);
}

static void droidUpdateDroidSelfRepair(DROID *psRepairDroid)
{
	droidUpdateDroidRepairBase(psRepairDroid, psRepairDroid);
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
	    psTemplate->droidType == DROID_TRANSPORTER ||
	    psTemplate->droidType == DROID_SUPERTRANSPORTER)
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
	else if (psTemplate->asWeaps[0] != 0)
	{
		type = DROID_WEAPON;
	}
	/* with more than weapon is still a DROID_WEAPON */
	else if (psTemplate->numWeaps > 1)
	{
		type = DROID_WEAPON;
	}

	return type;
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
	for (i = 0; i < psTemplate->numWeaps; i++)
	{
		weight += (asWeaponStats + psTemplate->asWeaps[i])->weight;
	}

	return weight;
}

static uint32_t calcDroidOrTemplateBody(uint8_t (&asParts)[DROID_MAXCOMP], unsigned numWeaps, uint8_t (&asWeaps)[MAX_WEAPONS], unsigned player)
{
	const auto &bodyStats = asBodyStats[asParts[COMP_BODY]];

	// Get the basic component body points
	int hitpoints =
	    bodyStats.upgrade[player].hitpoints +
	    asBrainStats[asParts[COMP_BRAIN]].upgrade[player].hitpoints +
	    asSensorStats[asParts[COMP_SENSOR]].upgrade[player].hitpoints +
	    asECMStats[asParts[COMP_ECM]].upgrade[player].hitpoints +
	    asRepairStats[asParts[COMP_REPAIRUNIT]].upgrade[player].hitpoints +
	    asPropulsionStats[asParts[COMP_PROPULSION]].upgrade[player].hitpoints +
	    asConstructStats[asParts[COMP_CONSTRUCT]].upgrade[player].hitpoints;

	int hitpointpct =
	    bodyStats.upgrade[player].hitpointPct - 100 +
	    asBrainStats[asParts[COMP_BRAIN]].upgrade[player].hitpointPct - 100 +
	    asSensorStats[asParts[COMP_SENSOR]].upgrade[player].hitpointPct - 100 +
	    asECMStats[asParts[COMP_ECM]].upgrade[player].hitpointPct - 100 +
	    asRepairStats[asParts[COMP_REPAIRUNIT]].upgrade[player].hitpointPct - 100 +
	    asPropulsionStats[asParts[COMP_PROPULSION]].upgrade[player].hitpointPct - 100 +
	    asConstructStats[asParts[COMP_CONSTRUCT]].upgrade[player].hitpointPct - 100;

	// propulsion hitpoints can be a percentage of the body's hitpoints
	hitpoints += bodyStats.upgrade[player].hitpoints * asPropulsionStats[asParts[COMP_PROPULSION]].upgrade[player].hitpointPctOfBody / 100;

	// Add the weapon body points
	for (unsigned i = 0; i < numWeaps; ++i)
	{
		if (asWeaps[i] > 0)
		{
			hitpoints += asWeaponStats[asWeaps[i]].upgrade[player].hitpoints;
			hitpointpct += asWeaponStats[asWeaps[i]].upgrade[player].hitpointPct - 100;
		}
	}

	// Final adjustment based on the hitpoint modifier
	return (hitpoints * (100 + hitpointpct)) / 100;
}

// Calculate the body points of a droid from its template
UDWORD calcTemplateBody(DROID_TEMPLATE *psTemplate, UBYTE player)
{
	if (psTemplate == nullptr)
	{
		ASSERT(false, "null template");
		return 0;
	}

	return calcDroidOrTemplateBody(psTemplate->asParts, psTemplate->numWeaps, psTemplate->asWeaps, player);
}

// Calculate the base body points of a droid with upgrades
static UDWORD calcDroidBaseBody(DROID *psDroid)
{
	uint8_t asWeaps[MAX_WEAPONS];
	std::transform(std::begin(psDroid->asWeaps), std::end(psDroid->asWeaps), asWeaps, [](WEAPON &weap) {
		return weap.nStat;
	});
	return calcDroidOrTemplateBody(psDroid->asBits, psDroid->numWeaps, asWeaps, psDroid->player);
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
		         bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY], player)) / MAX(1, weight);
	}
	else
	{
		speed = (asPropulsionTypes[(asPropulsionStats + psTemplate->
		                            asParts[COMP_PROPULSION])->propulsionType].powerRatioMult *
		         bodyPower(asBodyStats + psTemplate->asParts[COMP_BODY], player)) / MAX(1, weight);
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

	// applies the engine output bonus if output > weight
	if ((asBodyStats + psTemplate->asParts[COMP_BODY])->base.power > weight)
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
	for (i = 0; i < psTemplate->numWeaps; i++)
	{
		// FIX ME:
		ASSERT(psTemplate->asWeaps[i] < numWeaponStats,
		       //"Invalid Template weapon for %s", psTemplate->pName );
		       "Invalid Template weapon for %s", getName(psTemplate));
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
	for (i = 0; i < psTemplate->numWeaps; i++)
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
	power = (asBodyStats + psDroid->asBits[COMP_BODY])->buildPower +
	        (asBrainStats + psDroid->asBits[COMP_BRAIN])->buildPower +
	        (asSensorStats + psDroid->asBits[COMP_SENSOR])->buildPower +
	        (asECMStats + psDroid->asBits[COMP_ECM])->buildPower +
	        (asRepairStats + psDroid->asBits[COMP_REPAIRUNIT])->buildPower +
	        (asConstructStats + psDroid->asBits[COMP_CONSTRUCT])->buildPower;

	/* propulsion power points are a percentage of the bodys' power points */
	power += (((asPropulsionStats + psDroid->asBits[COMP_PROPULSION])->buildPower *
	           (asBodyStats + psDroid->asBits[COMP_BODY])->buildPower) / 100);

	//add weapon power
	for (i = 0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
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

	// Don't use this assertion in single player, since droids can finish building while on an away mission
	ASSERT(!bMultiPlayer || worldOnMap(pos.x, pos.y), "the build locations are not on the map");

	psDroid = new DROID(generateSynchronisedObjectId(), player);
	droidSetName(psDroid, getName(pTemplate));

	// Set the droids type
	psDroid->droidType = droidTemplateType(pTemplate);  // Is set again later to the same thing, in droidSetBits.
	psDroid->pos = pos;
	psDroid->rot = rot;
	psDroid->prevSpacetime.pos = pos;
	psDroid->prevSpacetime.rot = rot;

	//don't worry if not on homebase cos not being drawn yet
	if (!onMission)
	{
		//set droid height
		psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);
	}

	if (isTransporter(psDroid) || psDroid->droidType == DROID_COMMAND)
	{
		psGrp = grpCreate();
		psGrp->add(psDroid);
	}

	// find the highest stored experience
	// Unless game time is stopped, then we're hopefully loading a game and
	// don't want to use up recycled experience for the droids we just loaded.
	if (!gameTimeIsStopped() &&
	    (psDroid->droidType != DROID_CONSTRUCT) &&
	    (psDroid->droidType != DROID_CYBORG_CONSTRUCT) &&
	    (psDroid->droidType != DROID_REPAIR) &&
	    (psDroid->droidType != DROID_CYBORG_REPAIR) &&
	    !isTransporter(psDroid) &&
	    !recycled_experience[psDroid->player].empty())
	{
		psDroid->experience = recycled_experience[psDroid->player].top();
		recycled_experience[psDroid->player].pop();
	}
	else
	{
		psDroid->experience = 0;
	}

	droidSetBits(pTemplate, psDroid);

	//calculate the droids total weight
	psDroid->weight = calcDroidWeight(pTemplate);

	// Initialise the movement stuff
	psDroid->baseSpeed = calcDroidBaseSpeed(pTemplate, psDroid->weight, (UBYTE)player);

	initDroidMovement(psDroid);

	//allocate 'easy-access' data!
	psDroid->body = calcDroidBaseBody(psDroid); // includes upgrades
	ASSERT(psDroid->body > 0, "Invalid number of hitpoints");
	psDroid->originalBody = psDroid->body;

	/* Set droid's initial illumination */
	psDroid->sDisplay.imd = BODY_IMD(psDroid, psDroid->player);

	//don't worry if not on homebase cos not being drawn yet
	if (!onMission)
	{
		/* People always stand upright */
		if (psDroid->droidType != DROID_PERSON)
		{
			updateDroidOrientation(psDroid);
		}
		visTilesUpdate((BASE_OBJECT *)psDroid);
		clustNewDroid(psDroid);
	}

	/* transporter-specific stuff */
	if (isTransporter(psDroid))
	{
		//add transporter launch button if selected player and not a reinforcable situation
		if (player == selectedPlayer && !missionCanReEnforce())
		{
			(void)intAddTransporterLaunch(psDroid);
		}

		//set droid height to be above the terrain
		psDroid->pos.z += TRANSPORTER_HOVER_HEIGHT;
	}

	if (player == selectedPlayer)
	{
		scoreUpdateVar(WD_UNITS_BUILT);
	}

	debug(LOG_LIFE, "created droid for player %d, droid = %p, id=%d (%s): position: x(%d)y(%d)z(%d)", player, psDroid, (int)psDroid->id, psDroid->aName, psDroid->pos.x, psDroid->pos.y, psDroid->pos.z);

	return psDroid;
}

DROID *buildDroid(DROID_TEMPLATE *pTemplate, UDWORD x, UDWORD y, UDWORD player, bool onMission, const INITIAL_DROID_ORDERS *initialOrders)
{
	// ajl. droid will be created, so inform others
	if (bMultiMessages)
	{
		// Only sends if it's ours, otherwise the owner should send the message.
		SendDroid(pTemplate, x, y, player, generateNewObjectId(), initialOrders);
		return nullptr;
	}
	else
	{
		return reallyBuildDroid(pTemplate, Position(x, y, 0), player, onMission);
	}
}

//initialises the droid movement model
void initDroidMovement(DROID *psDroid)
{
	free(psDroid->sMove.asPath);
	memset(&psDroid->sMove, 0, sizeof(MOVE_CONTROL));
}

// Set the asBits in a DROID structure given it's template.
void droidSetBits(DROID_TEMPLATE *pTemplate, DROID *psDroid)
{
	psDroid->droidType = droidTemplateType(pTemplate);
	psDroid->numWeaps = pTemplate->numWeaps;
	psDroid->body = calcTemplateBody(pTemplate, psDroid->player);
	psDroid->originalBody = psDroid->body;
	psDroid->expectedDamageDirect = 0;  // Begin life optimistically.
	psDroid->expectedDamageIndirect = 0;  // Begin life optimistically.
	psDroid->time = gameTime - deltaGameTime + 1;         // Start at beginning of tick.
	psDroid->prevSpacetime.time = psDroid->time - 1;  // -1 for interpolation.

	//create the droids weapons
	for (int inc = 0; inc < MAX_WEAPONS; inc++)
	{
		psDroid->psActionTarget[inc] = nullptr;
		psDroid->asWeaps[inc].lastFired = 0;
		psDroid->asWeaps[inc].shotsFired = 0;
		// no weapon (could be a construction droid for example)
		// this is also used to check if a droid has a weapon, so zero it
		psDroid->asWeaps[inc].nStat = 0;
		psDroid->asWeaps[inc].ammo = 0;
		psDroid->asWeaps[inc].rot.direction = 0;
		psDroid->asWeaps[inc].rot.pitch = 0;
		psDroid->asWeaps[inc].rot.roll = 0;
		psDroid->asWeaps[inc].prevRot = psDroid->asWeaps[inc].rot;
		psDroid->asWeaps[inc].origin = ORIGIN_UNKNOWN;
		if (inc < pTemplate->numWeaps)
		{
			psDroid->asWeaps[inc].nStat = pTemplate->asWeaps[inc];
			psDroid->asWeaps[inc].ammo = (asWeaponStats + psDroid->asWeaps[inc].nStat)->upgrade[psDroid->player].numRounds;
		}
		psDroid->asWeaps[inc].usedAmmo = 0;
	}
	memcpy(psDroid->asBits, pTemplate->asParts, sizeof(psDroid->asBits));

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
	for (int inc = 0; inc < MAX_WEAPONS; inc++)
	{
		//this should fix the NULL weapon stats for empty weaponslots
		psTemplate->asWeaps[inc] = 0;
		if (psDroid->asWeaps[inc].nStat > 0)
		{
			psTemplate->numWeaps += 1;
			psTemplate->asWeaps[inc] = psDroid->asWeaps[inc].nStat;
		}
	}
	memcpy(psTemplate->asParts, psDroid->asBits, sizeof(psDroid->asBits));
}

/* Make all the droids for a certain player a member of a specific group */
void assignDroidsToGroup(UDWORD	playerNumber, UDWORD groupNumber)
{
	DROID	*psDroid;
	bool	bAtLeastOne = false;
	FLAG_POSITION	*psFlagPos;

	if (groupNumber < UBYTE_MAX)
	{
		/* Run through all the droids */
		for (psDroid = apsDroidLists[playerNumber]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			/* Clear out the old ones */
			if (psDroid->group == groupNumber)
			{
				psDroid->group = UBYTE_MAX;
			}

			/* Only assign the currently selected ones */
			if (psDroid->selected)
			{
				/* Set them to the right group - they can only be a member of one group */
				psDroid->group = (UBYTE)groupNumber;
				bAtLeastOne = true;
			}
		}
	}
	if (bAtLeastOne)
	{
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
	DROID	*psDroid, *psCentreDroid = nullptr;
	bool selected = false;
	FLAG_POSITION	*psFlagPos;

	if (groupNumber < UBYTE_MAX)
	{
		for (psDroid = apsDroidLists[playerNumber]; psDroid != nullptr; psDroid = psDroid->psNext)
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

	if (selected)
	{
		groupConsoleInformOfCentering(groupNumber);
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
		//clear the Deliv Point if one
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		     psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = false;
		}
		groupConsoleInformOfSelection(groupNumber);
	}
	return selected;
}

void	groupConsoleInformOfSelection(UDWORD groupNumber)
{
	unsigned int num_selected = selNumSelected(selectedPlayer);

	CONPRINTF(ConsoleString, (ConsoleString, ngettext("Group %u selected - %u Unit", "Group %u selected - %u Units", num_selected), groupNumber, num_selected));
}

void	groupConsoleInformOfCreation(UDWORD groupNumber)
{
	if (!getWarCamStatus())
	{
		unsigned int num_selected = selNumSelected(selectedPlayer);

		CONPRINTF(ConsoleString, (ConsoleString, ngettext("%u unit assigned to Group %u", "%u units assigned to Group %u", num_selected), num_selected, groupNumber));
	}

}

void	groupConsoleInformOfCentering(UDWORD groupNumber)
{
	unsigned int num_selected = selNumSelected(selectedPlayer);

	if (!getWarCamStatus())
	{
		CONPRINTF(ConsoleString, (ConsoleString, ngettext("Centered on Group %u - %u Unit", "Centered on Group %u - %u Units", num_selected), groupNumber, num_selected));
	}
	else
	{
		CONPRINTF(ConsoleString, (ConsoleString, ngettext("Aligning with Group %u - %u Unit", "Aligning with Group %u - %u Units", num_selected), groupNumber, num_selected));
	}
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

		*muzzle = (af * barrel).xzy;
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
		iIMDShape *psWeaponImd = nullptr, *psMountImd = nullptr;

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

		*muzzle = (af * barrel).xzy;
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
	const char  *name;           // name of this rank
};

unsigned int getDroidLevel(const DROID *psDroid)
{
	unsigned int numKills = psDroid->experience / 65536;
	unsigned int i;

	// Search through the array of ranks until one is found
	// which requires more kills than the droid has.
	// Then fall back to the previous rank.
	const BRAIN_STATS *psStats = getBrainStats(psDroid);
	auto &vec = psStats->upgrade[psDroid->player].rankThresholds;
	for (i = 1; i < vec.size(); ++i)
	{
		if (numKills < vec.at(i))
		{
			return i - 1;
		}
	}

	// If the criteria of the last rank are met, then select the last one
	return vec.size() - 1;
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

const char *getDroidLevelName(DROID *psDroid)
{
	const BRAIN_STATS *psStats = getBrainStats(psDroid);
	return PE_("rank", psStats->rankNames[getDroidLevel(psDroid)].c_str());
}

UDWORD	getNumDroidsForLevel(UDWORD	level)
{
	DROID	*psDroid;
	UDWORD	count;

	for (psDroid = apsDroidLists[selectedPlayer], count = 0;
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
void droidSetName(DROID *psDroid, const char *pName)
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
		const DROID *psDroid;
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
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (pD = apsDroidLists[i]; pD ; pD = pD->psNext)
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
	if ((x < TOO_NEAR_EDGE) || (x > (SDWORD)(mapWidth - TOO_NEAR_EDGE)))
	{
		return false;
	}
	if ((y < TOO_NEAR_EDGE) || (y > (SDWORD)(mapHeight - TOO_NEAR_EDGE)))
	{
		return false;
	}

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
	return sensiblePlace(x, y, PROPULSION_TYPE_WHEELED) && noDroid(x, y);
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

	ASSERT_OR_RETURN(false, *x < mapWidth, "x coordinate is off-map for pickATileGen");
	ASSERT_OR_RETURN(false, *y < mapHeight, "y coordinate is off-map for pickATileGen");

	if (function(*x, *y) && ((threatRange <= 0) || (!ThreatInRange(player, threatRange, *x, *y, false))))	//TODO: vtol check really not needed?
	{
		return (true);
	}

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = *x;	startY = endY = *y;	passes = 0;

	/* Keep going until we get a tile or we exceed distance */
	while (passes < numIterations)
	{
		/* Process whole box */
		for (i = startX; i <= endX; i++)
		{
			for (j = startY; j <= endY; j++)
			{
				/* Test only perimeter as internal tested previous iteration */
				if (i == startX || i == endX || j == startY || j == endY)
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
	return pickATileGen(x, y, numIterations, canFitDroid) ? FREE_TILE : NO_FREE_TILE;
}

/* Looks through the players list of droids to see if any of them are
building the specified structure - returns true if finds one*/
bool checkDroidsBuilding(STRUCTURE *psStructure)
{
	DROID				*psDroid;

	for (psDroid = apsDroidLists[psStructure->player]; psDroid != nullptr; psDroid =
	         psDroid->psNext)
	{
		//check DORDER_BUILD, HELP_BUILD is handled the same
		BASE_OBJECT *const psStruct = orderStateObj(psDroid, DORDER_BUILD);
		if ((STRUCTURE *)psStruct == psStructure)
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

	for (psDroid = apsDroidLists[psStructure->player]; psDroid != nullptr; psDroid =
	         psDroid->psNext)
	{
		//check DORDER_DEMOLISH
		BASE_OBJECT *const psStruct = orderStateObj(psDroid, DORDER_DEMOLISH);
		if ((STRUCTURE *)psStruct == psStructure)
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

	ASSERT_OR_RETURN(0, psStruct != nullptr && psStruct->pStructureType != nullptr, "Invalid structure pointer");

	int next = psStruct->status == SS_BUILT ? 1 : 0; // If complete, next is one after the current number of modules, otherwise next is the one we're working on.
	int max;
	switch (psStruct->pStructureType->type)
	{
	case REF_POWER_GEN:
		//check room for one more!
		max = std::max<int>(psStruct->capacity + next, lastOrderedModule + 1);
		if (max <= 1)
		{
			i = powerModuleStat;
			order = max;
		}
		break;
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
		//check room for one more!
		max = std::max<int>(psStruct->capacity + next, lastOrderedModule + 1);
		if (max <= NUM_FACTORY_MODULES)
		{
			i = factoryModuleStat;
			order = max;
		}
		break;
	case REF_RESEARCH:
		//check room for one more!
		max = std::max<int>(psStruct->capacity + next, lastOrderedModule + 1);
		if (max <= 1)
		{
			i = researchModuleStat;
			order = max;  // Research modules are weird. Build one, get three free.
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
	{
		// if a droid is currently building, or building is in progress of being built/upgraded the droid's order should be DORDER_HELPBUILD
		if (checkDroidsBuilding(psStruct) || !psStruct->status)
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
	if (psDroid->body < psDroid->originalBody)
	{
		return (true);
	}
	else
	{
		return (false);
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
	CHECK_DROID(psDroid);

	//use slot 0 for now
	if (psDroid->numWeaps > 0 && asWeaponStats[psDroid->asWeaps[0].nStat].weaponSubClass == WSC_ELECTRONIC)
	{
		return true;
	}

	if (psDroid->droidType == DROID_COMMAND && psDroid->psGroup && psDroid->psGroup->psCommander == psDroid)
	{
		// if a commander has EW units attached it is electronic
		for (DROID *psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
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
	CHECK_DROID(psDroid);

	//droid must be damaged
	if (droidIsDamaged(psDroid))
	{
		//look thru the list of players droids to see if any are repairing this droid
		for (DROID *psCurr = apsDroidLists[psDroid->player]; psCurr != nullptr; psCurr = psCurr->psNext)
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
	UBYTE	quantity = 0;

	for (DROID *psDroid = apsDroidLists[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		if (psDroid->droidType == DROID_COMMAND)
		{
			quantity++;
		}
	}
	return quantity;
}

static inline bool isTransporter(DROID_TYPE type)
{
	return type == DROID_TRANSPORTER || type == DROID_SUPERTRANSPORTER;
}

bool isTransporter(DROID const *psDroid)
{
	return isTransporter(psDroid->droidType);
}

bool isTransporter(DROID_TEMPLATE const *psTemplate)
{
	return isTransporter(psTemplate->droidType);
}

//access functions for vtols
bool isVtolDroid(const DROID *psDroid)
{
	return asPropulsionStats[psDroid->asBits[COMP_PROPULSION]].propulsionType == PROPULSION_TYPE_LIFT
	       && !isTransporter(psDroid);
}

/* returns true if the droid has lift propulsion and is moving */
bool isFlying(const DROID *psDroid)
{
	return (asPropulsionStats + psDroid->asBits[COMP_PROPULSION])->propulsionType == PROPULSION_TYPE_LIFT
	       && (psDroid->sMove.Status != MOVEINACTIVE || isTransporter(psDroid));
}

/* returns true if it's a VTOL weapon droid which has completed all runs */
bool vtolEmpty(DROID *psDroid)
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

	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (asWeaponStats[psDroid->asWeaps[i].nStat].vtolAttackRuns > 0 &&
		    psDroid->asWeaps[i].usedAmmo < getNumAttackRuns(psDroid, i))
		{
			return false;
		}
	}

	return true;
}

/* returns true if it's a VTOL weapon droid which still has full ammo */
bool vtolFull(DROID *psDroid)
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

	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (asWeaponStats[psDroid->asWeaps[i].nStat].vtolAttackRuns > 0 &&
		    psDroid->asWeaps[i].usedAmmo > 0)
		{
			return false;
		}
	}

	return true;
}

// true if a vtol is waiting to be rearmed by a particular rearm pad
bool vtolReadyToRearm(DROID *psDroid, STRUCTURE *psStruct)
{
	CHECK_DROID(psDroid);

	if (!isVtolDroid(psDroid)
	    || psDroid->action != DACTION_WAITFORREARM)
	{
		return false;
	}

	// If a unit has been ordered to rearm make sure it goes to the correct base
	BASE_OBJECT *psRearmPad = orderStateObj(psDroid, DORDER_REARM);
	if (psRearmPad
	    && (STRUCTURE *)psRearmPad != psStruct
	    && !vtolOnRearmPad((STRUCTURE *)psRearmPad, psDroid))
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

	if ((psDroid->psActionTarget[0] != nullptr) &&
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
	CHECK_DROID(psDroid);

	// ignore all non vtols
	if (!isVtolDroid(psDroid))
	{
		return true;
	}

	bool stillRearming = false;
	for (DROID *psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
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
UWORD   getNumAttackRuns(DROID *psDroid, int weapon_slot)
{
	ASSERT_OR_RETURN(0, isVtolDroid(psDroid), "not a VTOL Droid");
	// if weapon is a salvo weapon, then number of shots that can be fired = vtolAttackRuns * numRounds
	if (asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].upgrade[psDroid->player].reloadTime)
	{
		return asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].upgrade[psDroid->player].numRounds
		       * asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].vtolAttackRuns;
	}
	return asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].vtolAttackRuns;
}

/*Checks a vtol for being fully armed and fully repaired to see if ready to
leave reArm pad */
bool vtolHappy(const DROID *psDroid)
{
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
	for (int i = 0; i < psDroid->numWeaps; ++i)
	{
		if (asWeaponStats[psDroid->asWeaps[i].nStat].vtolAttackRuns > 0
		    && psDroid->asWeaps[i].usedAmmo != 0)
		{
			return false;
		}
	}

	return true;
}

/*checks if the droid is a VTOL droid and updates the attack runs as required*/
void updateVtolAttackRun(DROID *psDroid, int weapon_slot)
{
	if (isVtolDroid(psDroid))
	{
		if (psDroid->numWeaps > 0)
		{
			if (asWeaponStats[psDroid->asWeaps[weapon_slot].nStat].vtolAttackRuns > 0)
			{
				++psDroid->asWeaps[weapon_slot].usedAmmo;
				if (psDroid->asWeaps[weapon_slot].usedAmmo == getNumAttackRuns(psDroid, weapon_slot))
				{
					psDroid->asWeaps[weapon_slot].ammo = 0;
				}
				//quick check doesn't go over limit
				ASSERT(psDroid->asWeaps[weapon_slot].usedAmmo < UWORD_MAX, "too many attack runs");
			}
		}
	}
}

//assign rearmPad to the VTOL
void assignVTOLPad(DROID *psNewDroid, STRUCTURE *psReArmPad)
{
	ASSERT_OR_RETURN(, isVtolDroid(psNewDroid), "%s is not a VTOL droid", objInfo(psNewDroid));
	ASSERT_OR_RETURN(,  psReArmPad->type == OBJ_STRUCTURE && psReArmPad->pStructureType->type == REF_REARM_PAD,
	                 "%s cannot rearm", objInfo(psReArmPad));

	setDroidBase(psNewDroid, psReArmPad);
}

/*compares the droid sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
bool droidSensorDroidWeapon(BASE_OBJECT *psObj, DROID *psDroid)
{
	SENSOR_STATS	*psStats = nullptr;
	int compIndex;

	CHECK_DROID(psDroid);

	if (!psObj || !psDroid)
	{
		return false;
	}

	//first check if the object is a droid or a structure
	if ((psObj->type != OBJ_DROID) &&
	    (psObj->type != OBJ_STRUCTURE))
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
		compIndex = ((DROID *)psObj)->asBits[COMP_SENSOR];
		ASSERT_OR_RETURN(false, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = asSensorStats + compIndex;
		break;
	case OBJ_STRUCTURE:
		psStats = ((STRUCTURE *)psObj)->pStructureType->pSensor;
		if ((psStats == nullptr) ||
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
	if (asSensorStats[psDroid->asBits[COMP_SENSOR]].type == VTOL_CB_SENSOR
	    || asSensorStats[psDroid->asBits[COMP_SENSOR]].type == INDIRECT_CB_SENSOR)
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
	if (asSensorStats[psDroid->asBits[COMP_SENSOR]].type == VTOL_INTERCEPT_SENSOR
	    || asSensorStats[psDroid->asBits[COMP_SENSOR]].type == STANDARD_SENSOR
	    || asSensorStats[psDroid->asBits[COMP_SENSOR]].type == SUPER_SENSOR)
	{
		return true;
	}

	return false;
}

// ////////////////////////////////////////////////////////////////////////////
// Give a droid from one player to another - used in Electronic Warfare and multiplayer.
// Got to destroy the droid and build another since there are too many complications otherwise.
// Returns the droid created.
DROID *giftSingleDroid(DROID *psD, UDWORD to)
{
	DROID_TEMPLATE sTemplate;
	DROID *psNewDroid;

	CHECK_DROID(psD);
	ASSERT_OR_RETURN(nullptr, !isDead(psD), "Cannot gift dead unit");
	ASSERT_OR_RETURN(psD, psD->player != to, "Cannot gift to self");

	// Check unit limits (multiplayer only)
	if (bMultiPlayer
	    && (getNumDroids(to) + 1 > getMaxDroids(to)
	        || ((psD->droidType == DROID_CYBORG_CONSTRUCT || psD->droidType == DROID_CONSTRUCT)
	            && getNumConstructorDroids(to) + 1 > getMaxConstructors(to))
	        || (psD->droidType == DROID_COMMAND && getNumCommandDroids(to) + 1 > getMaxCommanders(to))))
	{
		if (to == selectedPlayer || psD->player == selectedPlayer)
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("Unit transfer failed -- unit limits exceeded")));
		}
		return nullptr;
	}
	templateSetParts(psD, &sTemplate);	// create a template based on the droid
	sTemplate.name = psD->aName;	// copy the name across
	// only play the nexus sound if unit being taken over is selectedPlayer's but not going to the selectedPlayer
	if (psD->player == selectedPlayer && to != selectedPlayer && !bMultiPlayer)
	{
		scoreUpdateVar(WD_UNITS_LOST);
		audio_QueueTrackPos(ID_SOUND_NEXUS_UNIT_ABSORBED, psD->pos.x, psD->pos.y, psD->pos.z);
	}
	// make the old droid vanish (but is not deleted until next tick)
	vanishDroid(psD);
	// create a new droid
	psNewDroid = reallyBuildDroid(&sTemplate, Position(psD->pos.x, psD->pos.y, 0), to, false, psD->rot);
	ASSERT_OR_RETURN(nullptr, psNewDroid, "Unable to build unit");
	addDroid(psNewDroid, apsDroidLists);
	psNewDroid->body = clip((psD->body*psNewDroid->originalBody + psD->originalBody/2)/std::max(psD->originalBody, 1u), 1u, psNewDroid->originalBody);
	psNewDroid->experience = psD->experience;
	if (!(psNewDroid->droidType == DROID_PERSON || cyborgDroid(psNewDroid) || isTransporter(psNewDroid)))
	{
		updateDroidOrientation(psNewDroid);
	}
	if (bMultiPlayer)	// skirmish callback!
	{
		psScrCBDroidTaken = psD;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_UNITTAKEOVER);
		psScrCBDroidTaken = nullptr;
	}
	triggerEventObjectTransfer(psNewDroid, psD->player);
	return psNewDroid;
}

/*calculates the electronic resistance of a droid based on its experience level*/
SWORD   droidResistance(DROID *psDroid)
{
	CHECK_DROID(psDroid);
	BODY_STATS *psStats = asBodyStats + psDroid->asBits[COMP_BODY];
	int resistance = psDroid->experience / (65536 / MAX(1, psStats->upgrade[psDroid->player].resistance));
	// ensure resistance is a base minimum
	resistance = MAX(resistance, psStats->upgrade[psDroid->player].resistance);
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

	ASSERT_OR_RETURN(false, psPropStats != nullptr, "invalid propulsion stats pointer");

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
		if (asWeaponStats[psTemplate->asWeaps[0]].vtolAttackRuns)
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
	if (psDroid->flags.test(OBJECT_FLAG_UNSELECTABLE))
	{
		return;
	}
	// we shouldn't ever control the transporter in SP games
	if (!isTransporter(psDroid) || bMultiPlayer)
	{
		psDroid->selected = true;
		intRefreshScreen();
	}
	triggerEventSelected();
	jsDebugSelected(psDroid);
}

// De-select a droid and do any necessary housekeeping.
//
void DeSelectDroid(DROID *psDroid)
{
	psDroid->selected = false;
	intRefreshScreen();
	triggerEventSelected();
}

/** Callback function for stopped audio tracks
 *  Sets the droid's current track id to NO_SOUND
 *  \return true on success, false on failure
 */
bool droidAudioTrackStopped(void *psObj)
{
	DROID	*psDroid;

	psDroid = (DROID *)psObj;
	if (psDroid == nullptr)
	{
		debug(LOG_ERROR, "droid pointer invalid");
		return false;
	}

	if (psDroid->type != OBJ_DROID || psDroid->died)
	{
		return false;
	}

	psDroid->iAudioID = NO_SOUND;
	return true;
}

/*returns true if droid type is one of the Cyborg types*/
bool cyborgDroid(const DROID *psDroid)
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
	if (psDroid->died == NOT_CURRENT_LIST || isTransporter(psDroid)
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

	ASSERT_HELPER(droid != nullptr, location, function, "CHECK_DROID: NULL pointer");
	ASSERT_HELPER(droid->type == OBJ_DROID, location, function, "CHECK_DROID: Not droid (type %d)", (int)droid->type);
	ASSERT_HELPER(droid->numWeaps <= MAX_WEAPONS, location, function, "CHECK_DROID: Bad number of droid weapons %d", (int)droid->numWeaps);
	ASSERT_HELPER((unsigned)droid->listSize <= droid->asOrderList.size() && (unsigned)droid->listPendingBegin <= droid->asOrderList.size(), location, function, "CHECK_DROID: Bad number of droid orders %d %d %d", (int)droid->listSize, (int)droid->listPendingBegin, (int)droid->asOrderList.size());
	ASSERT_HELPER(droid->player < MAX_PLAYERS, location, function, "CHECK_DROID: Bad droid owner %d", (int)droid->player);
	ASSERT_HELPER(droidOnMap(droid), location, function, "CHECK_DROID: Droid off map");
	ASSERT_HELPER(droid->body <= droid->originalBody, location, function, "CHECK_DROID: More body points (%u) than original body points (%u).", (unsigned)droid->body, (unsigned)droid->originalBody);

	for (int i = 0; i < MAX_WEAPONS; ++i)
	{
		ASSERT_HELPER(droid->asWeaps[i].lastFired <= gameTime, location, function, "CHECK_DROID: Bad last fired time for turret %u", i);
	}
}

int droidSqDist(DROID *psDroid, BASE_OBJECT *psObj)
{
	PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION];

	if (!fpathCheck(psDroid->pos, psObj->pos, psPropStats->propulsionType))
	{
		return -1;
	}
	return objPosDiffSq(psDroid->pos, psObj->pos);
}
