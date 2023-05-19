/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "cmddroid.h"
#include "fpath.h"
#include "projectile.h"
#include "mission.h"
#include "levels.h"
#include "transporter.h"
#include "selection.h"
#include "difficulty.h"
#include "edit3d.h"
#include "scores.h"
#include "research.h"
#include "combat.h"
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
static void groupConsoleInformOfRemoval();
static void droidUpdateDroidSelfRepair(DROID *psRepairDroid);
static UDWORD calcDroidBaseBody(DROID *psDroid);

int getTopExperience(int player)
{
	if (recycled_experience[player].size() == 0)
	{
		return 0;
	}
	return recycled_experience[player].top();
}
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

		// The droid has no more build orders, so halt in place rather than clumping around the build objective
		moveStopDroid(psDroid);

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
	DROID_TEMPLATE sTemplate;
	templateSetParts(psDroid, &sTemplate);
	// update engine too
	psDroid->baseSpeed = calcDroidBaseSpeed(&sTemplate, psDroid->weight, psDroid->player);
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

int droidReloadBar(const BASE_OBJECT *psObj, const WEAPON *psWeap, int weapon_slot)
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
		|| (psObj->type == OBJ_DROID && isVtolDroid((const DROID *)psObj)))
	{
		if (psObj->type == OBJ_DROID && isVtolDroid((const DROID *)psObj))
		{
			//deal with VTOLs
			firingStage = getNumAttackRuns((const DROID *)psObj, weapon_slot) - ((const DROID *)psObj)->asWeaps[weapon_slot].usedAmmo;

			//compare with max value
			interval = getNumAttackRuns((const DROID *)psObj, weapon_slot);
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

		CHECK_DROID(psDroid);
	}
	else if (relativeDamage < 0)
	{
		// Droid destroyed
		debug(LOG_ATTACK, "droid (%d): DESTROYED", psDroid->id);

		// Deal with score increase/decrease and messages to the player
		if (psDroid->player == selectedPlayer)
		{
			// TRANSLATORS:	Refers to the loss of a single unit, known by its name
			CONPRINTF(_("%s Lost!"), objInfo(psDroid));
			scoreUpdateVar(WD_UNITS_LOST);
			audio_QueueTrackMinDelayPos(ID_SOUND_UNIT_DESTROYED, UNIT_LOST_DELAY,
										psDroid->pos.x, psDroid->pos.y, psDroid->pos.z);
		}
		// only counts as a kill if it's not our ally
		else if (selectedPlayer < MAX_PLAYERS && !aiCheckAlliances(psDroid->player, selectedPlayer))
		{
			scoreUpdateVar(WD_UNITS_KILLED);
		}

		// Do we have a dying animation?
		if (psDroid->sDisplay.imd->objanimpie[ANIM_EVENT_DYING] && psDroid->animationEvent != ANIM_EVENT_DYING)
		{
			bool useDeathAnimation = true;
			//Babas should not burst into flames from non-heat weapons
			if (psDroid->droidType == DROID_PERSON)
			{
				if (weaponClass == WC_HEAT)
				{
					// NOTE: 3 types of screams are available ID_SOUND_BARB_SCREAM - ID_SOUND_BARB_SCREAM3
					audio_PlayObjDynamicTrack(psDroid, ID_SOUND_BARB_SCREAM + (rand() % 3), nullptr);
				}
				else
				{
					useDeathAnimation = false;
				}
			}
			if (useDeathAnimation)
			{
				debug(LOG_DEATH, "%s droid %d (%p) is starting death animation", objInfo(psDroid), (int)psDroid->id, static_cast<void *>(psDroid));
				psDroid->timeAnimationStarted = gameTime;
				psDroid->animationEvent = ANIM_EVENT_DYING;
			}
		}
		// Otherwise use the default destruction animation
		if (psDroid->animationEvent != ANIM_EVENT_DYING)
		{
			debug(LOG_DEATH, "%s droid %d (%p) is toast", objInfo(psDroid), (int)psDroid->id, static_cast<void *>(psDroid));
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
	, secondaryOrder(DSS_ARANGE_LONG | DSS_REPLEV_NEVER | DSS_ALEV_ALWAYS | DSS_HALT_GUARD)
	, secondaryOrderPending(DSS_ARANGE_LONG | DSS_REPLEV_NEVER | DSS_ALEV_ALWAYS | DSS_HALT_GUARD)
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
	DROID	*psCurr, *pNextGroupDroid = nullptr;

	if (isTransporter(psDroid))
	{
		if (psDroid->psGroup)
		{
			//free all droids associated with this Transporter
			for (psCurr = psDroid->psGroup->psList; psCurr != nullptr && psCurr != psDroid; psCurr = pNextGroupDroid)
			{
				pNextGroupDroid = psCurr->psGrpNext;
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

	if (psDroid->psGroup)
	{
		psDroid->psGroup->remove(psDroid);
	}

	triggerEvent(TRIGGER_OBJECT_RECYCLED, psDroid);
	vanishDroid(psDroid);

	Vector3i position = psDroid->pos.xzy();
	const auto mapCoord = map_coord({psDroid->pos.x, psDroid->pos.y});
	const auto psTile = mapTile(mapCoord);
	if (tileIsClearlyVisible(psTile))
	{
		addEffect(&position, EFFECT_EXPLOSION, EXPLOSION_TYPE_DISCOVERY, false, nullptr, false, gameTime - deltaGameTime + 1);
	}
	
	CHECK_DROID(psDroid);
}


bool removeDroidBase(DROID *psDel)
{
	CHECK_DROID(psDel);

	if (isDead(psDel))
	{
		// droid has already been killed, quit
		syncDebug("droid already dead");
		return true;
	}

	syncDebugDroid(psDel, '#');

	//kill all the droids inside the transporter
	if (isTransporter(psDel))
	{
		if (psDel->psGroup)
		{
			//free all droids associated with this Transporter
			DROID *psNext;
			for (auto psCurr = psDel->psGroup->psList; psCurr != nullptr && psCurr != psDel; psCurr = psNext)
			{
				psNext = psCurr->psGrpNext;

				/* add droid to droid list then vanish it - hope this works! - GJ */
				addDroid(psCurr, apsDroidLists);
				vanishDroid(psCurr);
			}
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
		for (auto psStruct = apsStructLists[psDel->player]; psStruct; psStruct = psStruct->psNext)
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
	if (!psDel->visibleForLocalDisplay())
	{
		return;
	}

	if (psDel->animationEvent != ANIM_EVENT_DYING)
	{
		compPersonToBits(psDel);
	}

	/* if baba then squish */
	if (psDel->droidType == DROID_PERSON)
	{
		// The barbarian has been run over ...
		audio_PlayStaticTrack(psDel->pos.x, psDel->pos.y, ID_SOUND_BARB_SQUISH);
	}
	else
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
	ASSERT(gameTime - deltaGameTime <= impactTime, "Expected %u <= %u, gameTime = %u, bad impactTime", gameTime - deltaGameTime, impactTime, gameTime);

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
				if (TEST_TILE_VISIBLE_TO_SELECTEDPLAYER(psTile))
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

void vanishDroid(DROID *psDel)
{
	removeDroidBase(psDel);
}

/* Remove a droid from the List so doesn't update or get drawn etc
TAKE CARE with removeDroid() - usually want droidRemove since it deal with grid code*/
//returns false if the droid wasn't removed - because it died!
bool droidRemove(DROID *psDroid, DROID *pList[MAX_PLAYERS])
{
	CHECK_DROID(psDroid);

	if (isDead(psDroid))
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

	removeDroid(psDroid, pList);

	if (psDroid->player == selectedPlayer)
	{
		intRefreshScreen();
	}

	return true;
}

void _syncDebugDroid(const char *function, DROID const *psDroid, char ch)
{
	if (psDroid->type != OBJ_DROID) {
		ASSERT(false, "%c Broken psDroid->type %u!", ch, psDroid->type);
		syncDebug("Broken psDroid->type %u!", psDroid->type);
	}
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
		psDroid->sMove.pathIndex, (int)psDroid->sMove.asPath.size(),
		psDroid->sMove.src.x, psDroid->sMove.src.y, psDroid->sMove.target.x, psDroid->sMove.target.y, psDroid->sMove.destination.x, psDroid->sMove.destination.y,
		psDroid->sMove.bumpDir, (int)psDroid->sMove.bumpTime, (int)psDroid->sMove.lastBump, (int)psDroid->sMove.pauseTime, psDroid->sMove.bumpPos.x, psDroid->sMove.bumpPos.y, (int)psDroid->sMove.shuffleStart,
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
			   droidGetName(psDroid), static_cast<void *>(psDroid));
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
	if (psDroid->visibleForLocalDisplay() && psDroid->droidType != DROID_PERSON)
	{
		// need to clip this value to prevent overflow condition
		percentDamage = 100 - clip<UDWORD>(PERCENT(psDroid->body, psDroid->originalBody), 0, 100);

		// Is there any damage?
		if (percentDamage >= 25)
		{
			if (percentDamage >= 100)
			{
				percentDamage = 99;
			}

			emissionInterval = CALC_DROID_SMOKE_INTERVAL(percentDamage);

			uint32_t effectTime = std::max(gameTime - deltaGameTime + 1, psDroid->lastEmission + emissionInterval);
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
		else if (secondaryGetState(psDroid, DSO_HALTTYPE) != DSS_HALT_PURSUE &&
			psDroid->psActionTarget[0] != nullptr &&
			validTarget(psDroid, psDroid->psActionTarget[0], 0) &&
			(psDroid->action == DACTION_ATTACK ||
			psDroid->action == DACTION_OBSERVE ||
			 orderState(psDroid, DORDER_HOLD)))
		{
			psBeingTargetted = psDroid->psActionTarget[0];
			psBeingTargetted->flags.set(OBJECT_FLAG_TARGETED, true);
		}
	}
	// ------------------------
	// See if we can and need to self repair.
	if (!isVtolDroid(psDroid) && droidIsDamaged(psDroid) && psDroid->asBits[COMP_REPAIRUNIT] != 0 && selfRepairEnabled(psDroid->player))
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
	if (isDead(psDroid))
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
static bool droidNextToStruct(DROID *psDroid, STRUCTURE *psStruct)
{
	CHECK_DROID(psDroid);

	auto pos = map_coord(psDroid->pos);
	int minX = std::max(pos.x - 1, 0);
	int minY = std::max(pos.y - 1, 0);
	int maxX = std::min(pos.x + 1, mapWidth);
	int maxY = std::min(pos.y + 1, mapHeight);
	for (int y = minY; y <= maxY; ++y)
	{
		for (int x = minX; x <= maxX; ++x)
		{
			if (TileHasStructure(mapTile(x, y)) &&
				getTileStructure(x, y) == psStruct)
			{
				return true;
			}
		}
	}

	return false;
}

static bool droidCheckBuildStillInProgress(void *psObj)
{
	if (psObj == nullptr)
	{
		return false;
	}

	auto psDroid = (DROID *)psObj;
	CHECK_DROID(psDroid);

	return !psDroid->died && psDroid->action == DACTION_BUILD;
}

static bool droidBuildStartAudioCallback(void *psObj)
{
	auto psDroid = (DROID *)psObj;

	if (psDroid != nullptr && psDroid->visibleForLocalDisplay())
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
			ASSERT(false, "Cannot build \"%s\" for player %d.", psStructStat->name.toUtf8().c_str(), psDroid->player);
			cancelBuild(psDroid);
			objTrace(psDroid->id, "DroidStartBuildFailed: not researched");
			return DroidStartBuildFailed;
		}

		//need to check structLimits have not been exceeded
		if (psStructStat->curCount[psDroid->player] >= psStructStat->upgrade[psDroid->player].limit)
		{
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
		const auto id = generateSynchronisedObjectId();
		psStruct = buildStructureDir(psStructStat, psDroid->order.pos.x, psDroid->order.pos.y, psDroid->order.direction, psDroid->player, false, id);
		if (!psStruct)
		{
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

		if (psStruct->visibleForLocalDisplay())
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
	int iAudioID = ID_SOUND_CONSTRUCTION_1 + (rand() % 4);

	audio_PlayStaticTrack(iVecEffect.x, iVecEffect.z, iAudioID);
}

static void addConstructorEffect(STRUCTURE *psStruct)
{
	if ((ONEINTEN) && (psStruct->visibleForLocalDisplay()))
	{
		/* This needs fixing - it's an arse effect! */
		const Vector2i size = psStruct->size() * TILE_UNITS / 4;
		Vector3i temp;
		temp.x = psStruct->pos.x + ((rand() % (2 * size.x)) - size.x);
		temp.y = map_TileHeight(map_coord(psStruct->pos.x), map_coord(psStruct->pos.y)) + (psStruct->sDisplay.imd->max.y / 6);
		temp.z = psStruct->pos.y + ((rand() % (2 * size.y)) - size.y);
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

	unsigned constructPoints = constructorPoints(asConstructStats + psDroid->
										asBits[COMP_CONSTRUCT], psDroid->player);

	unsigned pointsToAdd = constructPoints * (gameTime - psDroid->actionStarted) /
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
	CHECK_DROID(psDroid);

	ASSERT_OR_RETURN(false, psDroid->action == DACTION_RESTORE, "Unit is not restoring");
	STRUCTURE *psStruct = (STRUCTURE *)psDroid->order.psObj;
	ASSERT_OR_RETURN(false, psStruct->type == OBJ_STRUCTURE, "Target is not a structure");
	ASSERT_OR_RETURN(false, psDroid->asWeaps[0].nStat > 0, "Droid doesn't have any weapons");

	unsigned compIndex = psDroid->asWeaps[0].nStat;
	ASSERT_OR_RETURN(false, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %u > %u", compIndex, numWeaponStats);
	WEAPON_STATS *psStats = &asWeaponStats[compIndex];

	ASSERT_OR_RETURN(false, psStats->weaponSubClass == WSC_ELECTRONIC, "unit's weapon is not EW");

	unsigned restorePoints = calcDamage(weaponDamage(psStats, psDroid->player),
							   psStats->weaponEffect, (BASE_OBJECT *)psStruct);

	unsigned pointsToAdd = restorePoints * (gameTime - psDroid->actionStarted) /
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

void droidWasFullyRepaired(DROID *psDroid, const REPAIR_FACILITY *psRepairFac)
{
	const bool prevWasRTR = psDroid->order.type == DORDER_RTR || psDroid->order.type == DORDER_RTR_SPECIFIED;
	if (prevWasRTR && hasCommander(psDroid))
	{
		objTrace(psDroid->id, "Repair complete - move to commander");
		orderDroidObj(psDroid, DORDER_GUARD, psDroid->psGroup->psCommander, ModeImmediate);
	}
	else if (prevWasRTR && psRepairFac && psRepairFac->psDeliveryPoint != nullptr)
	{
		const FLAG_POSITION *dp = psRepairFac->psDeliveryPoint;
		objTrace(psDroid->id, "Repair complete - move to delivery point");
		// ModeQueue because delivery points are not yet synchronised!
		orderDroidLoc(psDroid, DORDER_MOVE, dp->coords.x, dp->coords.y, ModeQueue);
	}
	else if (prevWasRTR)
	{ // nothing to do, no commander, no repair point to go to. Stop, and guard this place.
		psDroid->order.psObj = nullptr;
		objTrace(psDroid->id, "Repair complete - guarding the place at x=%i y=%i", psDroid->pos.x, psDroid->pos.y);
		orderDroidLoc(psDroid, DORDER_GUARD, psDroid->pos.x, psDroid->pos.y, ModeImmediate);
	}
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

	psDroidToRepair->body = clip<UDWORD>(psDroidToRepair->body + iPointsToAdd, 0, psDroidToRepair->originalBody);

	/* add plasma repair effect whilst being repaired */
	if ((ONEINFIVE) && (psDroidToRepair->visibleForLocalDisplay()))
	{
		Vector3i iVecEffect = (psDroidToRepair->pos + Vector3i(DROID_REPAIR_SPREAD, DROID_REPAIR_SPREAD, rand() % 8)).xzy();
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
	bool needMoreRepair = droidUpdateDroidRepairBase(psRepairDroid, psDroidToRepair);
	if (needMoreRepair && psDroidToRepair->order.type == DORDER_RTR && psDroidToRepair->order.rtrType == RTR_TYPE_DROID && psDroidToRepair->action == DACTION_NONE)
	{
		psDroidToRepair->action = DACTION_WAITDURINGREPAIR;
	}
	if (!needMoreRepair && psDroidToRepair->order.type == DORDER_RTR && psDroidToRepair->order.rtrType == RTR_TYPE_DROID)
	{
		// if psDroidToRepair has a commander, commander will call him back anyway
		// if no commanders, just DORDER_GUARD the repair turret
		orderDroidObj(psDroidToRepair, DORDER_GUARD, psRepairDroid, ModeImmediate);
		secondarySetState(psDroidToRepair, DSO_RETURN_TO_LOC, DSS_NONE);
		psDroidToRepair->order.psObj = nullptr;
	}
	return needMoreRepair;
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

	return !proj_Direct(psDroid->asWeaps[0].nStat + asWeaponStats);
}

/* Return the type of a droid */
DROID_TYPE droidType(DROID *psDroid)
{
	return psDroid->droidType;
}

/* Return the type of a droid from it's template */
DROID_TYPE droidTemplateType(const DROID_TEMPLATE *psTemplate)
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

template <typename F, typename G>
static unsigned calcSum(const uint8_t (&asParts)[DROID_MAXCOMP], int numWeaps, const uint32_t (&asWeaps)[MAX_WEAPONS], F func, G propulsionFunc)
{
	unsigned sum =
		func(asBrainStats    [asParts[COMP_BRAIN]]) +
		func(asSensorStats   [asParts[COMP_SENSOR]]) +
		func(asECMStats      [asParts[COMP_ECM]]) +
		func(asRepairStats   [asParts[COMP_REPAIRUNIT]]) +
		func(asConstructStats[asParts[COMP_CONSTRUCT]]) +
		propulsionFunc(asBodyStats[asParts[COMP_BODY]], asPropulsionStats[asParts[COMP_PROPULSION]]);
	for (int i = 0; i < numWeaps; ++i)
	{
		sum += func(asWeaponStats[asWeaps[i]]);
	}
	return sum;
}

#define ASSERT_PLAYER_OR_RETURN(retVal, player) \
	ASSERT_OR_RETURN(retVal, player >= 0 && player < MAX_PLAYERS, "Invalid player: %" PRIu32 "", player);

template <typename F, typename G>
static unsigned calcUpgradeSum(const uint8_t (&asParts)[DROID_MAXCOMP], int numWeaps, const uint32_t (&asWeaps)[MAX_WEAPONS], int player, F func, G propulsionFunc)
{
	ASSERT_PLAYER_OR_RETURN(0, player);
	unsigned sum =
		func(asBrainStats    [asParts[COMP_BRAIN]].upgrade[player]) +
		func(asSensorStats   [asParts[COMP_SENSOR]].upgrade[player]) +
		func(asECMStats      [asParts[COMP_ECM]].upgrade[player]) +
		func(asRepairStats   [asParts[COMP_REPAIRUNIT]].upgrade[player]) +
		func(asConstructStats[asParts[COMP_CONSTRUCT]].upgrade[player]) +
		propulsionFunc(asBodyStats[asParts[COMP_BODY]].upgrade[player], asPropulsionStats[asParts[COMP_PROPULSION]].upgrade[player]);
	for (int i = 0; i < numWeaps; ++i)
	{
		// asWeaps[i] > 0 check only needed for droids, not templates.
		if (asWeaps[i] > 0)
		{
			ASSERT(asWeaps[i] < numWeaponStats, "Invalid weapon stat index (%" PRIu32 ", numWeaponStats: %" PRIu32 ") (player: %d)", asWeaps[i], numWeaponStats, player);
			sum += func(asWeaponStats[asWeaps[i]].upgrade[player]);
		}
	}
	return sum;
}

struct FilterDroidWeaps
{
	FilterDroidWeaps(unsigned numWeaps, const WEAPON (&asWeaps)[MAX_WEAPONS])
	{
		std::transform(asWeaps, asWeaps + numWeaps, this->asWeaps, [](const WEAPON &weap) {
			return weap.nStat;
		});
		this->numWeaps = std::remove_if(this->asWeaps, this->asWeaps + numWeaps, [](uint32_t stat) {
			return stat == 0;
		}) - this->asWeaps;
	}

	unsigned numWeaps;
	uint32_t asWeaps[MAX_WEAPONS];
};

template <typename F, typename G>
static unsigned calcSum(const DROID_TEMPLATE *psTemplate, F func, G propulsionFunc)
{
	return calcSum(psTemplate->asParts, psTemplate->numWeaps, psTemplate->asWeaps, func, propulsionFunc);
}

template <typename F, typename G>
static unsigned calcSum(const DROID *psDroid, F func, G propulsionFunc)
{
	FilterDroidWeaps f = {psDroid->numWeaps, psDroid->asWeaps};
	return calcSum(psDroid->asBits, f.numWeaps, f.asWeaps, func, propulsionFunc);
}

template <typename F, typename G>
static unsigned calcUpgradeSum(const DROID_TEMPLATE *psTemplate, int player, F func, G propulsionFunc)
{
	return calcUpgradeSum(psTemplate->asParts, psTemplate->numWeaps, psTemplate->asWeaps, player, func, propulsionFunc);
}

template <typename F, typename G>
static unsigned calcUpgradeSum(const DROID *psDroid, int player, F func, G propulsionFunc)
{
	FilterDroidWeaps f = {psDroid->numWeaps, psDroid->asWeaps};
	return calcUpgradeSum(psDroid->asBits, f.numWeaps, f.asWeaps, player, func, propulsionFunc);
}

/* Calculate the weight of a droid from it's template */
UDWORD calcDroidWeight(const DROID_TEMPLATE *psTemplate)
{
	return calcSum(psTemplate, [](COMPONENT_STATS const &stat) {
		return stat.weight;
	}, [](BODY_STATS const &bodyStat, PROPULSION_STATS const &propStat) {
		// Propulsion weight is a percentage of the body weight.
		return bodyStat.weight * (100 + propStat.weight) / 100;
	});
}

template <typename T>
static uint32_t calcBody(T *obj, int player)
{
	int hitpoints = calcUpgradeSum(obj, player, [](COMPONENT_STATS::UPGRADE const &upgrade) {
		return upgrade.hitpoints;
	}, [](BODY_STATS::UPGRADE const &bodyUpgrade, PROPULSION_STATS::UPGRADE const &propUpgrade) {
		// propulsion hitpoints can be a percentage of the body's hitpoints
		return bodyUpgrade.hitpoints * (100 + propUpgrade.hitpointPctOfBody) / 100 + propUpgrade.hitpoints;
	});

	int hitpointPct = calcUpgradeSum(obj, player, [](COMPONENT_STATS::UPGRADE const &upgrade) {
		return upgrade.hitpointPct - 100;
	}, [](BODY_STATS::UPGRADE const &bodyUpgrade, PROPULSION_STATS::UPGRADE const &propUpgrade) {
		return bodyUpgrade.hitpointPct - 100 + propUpgrade.hitpointPct - 100;
	});

	// Final adjustment based on the hitpoint modifier
	return hitpoints * (100 + hitpointPct) / 100;
}

// Calculate the body points of a droid from its template
UDWORD calcTemplateBody(const DROID_TEMPLATE *psTemplate, UBYTE player)
{
	if (psTemplate == nullptr)
	{
		ASSERT(false, "null template");
		return 0;
	}

	return calcBody(psTemplate, player);
}

// Calculate the base body points of a droid with upgrades
static UDWORD calcDroidBaseBody(DROID *psDroid)
{
	return calcBody(psDroid, psDroid->player);
}


/* Calculate the base speed of a droid from it's template */
UDWORD calcDroidBaseSpeed(const DROID_TEMPLATE *psTemplate, UDWORD weight, UBYTE player)
{
	unsigned speed = asPropulsionTypes[asPropulsionStats[psTemplate->asParts[COMP_PROPULSION]].propulsionType].powerRatioMult *
				 bodyPower(&asBodyStats[psTemplate->asParts[COMP_BODY]], player) / MAX(1, weight);

	// reduce the speed of medium/heavy VTOLs
	if (asPropulsionStats[psTemplate->asParts[COMP_PROPULSION]].propulsionType == PROPULSION_TYPE_LIFT)
	{
		if (asBodyStats[psTemplate->asParts[COMP_BODY]].size == SIZE_HEAVY)
		{
			speed /= 4;
		}
		else if (asBodyStats[psTemplate->asParts[COMP_BODY]].size == SIZE_MEDIUM)
		{
			speed = speed * 3 / 4;
		}
	}

	// applies the engine output bonus if output > weight
	if (asBodyStats[psTemplate->asParts[COMP_BODY]].base.power > weight)
	{
		speed = speed * 3 / 2;
	}

	return speed;
}


/* Calculate the speed of a droid over a terrain */
UDWORD calcDroidSpeed(UDWORD baseSpeed, UDWORD terrainType, UDWORD propIndex, UDWORD level)
{
	PROPULSION_STATS const &propulsion = asPropulsionStats[propIndex];

	// Factor in terrain
	unsigned speed = baseSpeed * getSpeedFactor(terrainType, propulsion.propulsionType) / 100;

	// Need to ensure doesn't go over the max speed possible for this propulsion
	speed = std::min(speed, propulsion.maxSpeed);

	// Factor in experience
	speed *= 100 + EXP_SPEED_BONUS * level;
	speed /= 100;

	return speed;
}

template <typename T>
static uint32_t calcBuild(T *obj)
{
	return calcSum(obj, [](COMPONENT_STATS const &stat) {
		return stat.buildPoints;
	}, [](BODY_STATS const &bodyStat, PROPULSION_STATS const &propStat) {
		// Propulsion power points are a percentage of the body's build points.
		return bodyStat.buildPoints * (100 + propStat.buildPoints) / 100;
	});
}

/* Calculate the points required to build the template - used to calculate time*/
UDWORD calcTemplateBuild(const DROID_TEMPLATE *psTemplate)
{
	return calcBuild(psTemplate);
}

UDWORD calcDroidPoints(DROID *psDroid)
{
	return calcBuild(psDroid);
}

template <typename T>
static uint32_t calcPower(const T *obj)
{
	ASSERT_NOT_NULLPTR_OR_RETURN(0, obj);
	return calcSum(obj, [](COMPONENT_STATS const &stat) {
		return stat.buildPower;
	}, [](BODY_STATS const &bodyStat, PROPULSION_STATS const &propStat) {
		// Propulsion power points are a percentage of the body's power points.
		return bodyStat.buildPower * (100 + propStat.buildPower) / 100;
	});
}

/* Calculate the power points required to build/maintain a template */
UDWORD calcTemplatePower(const DROID_TEMPLATE *psTemplate)
{
	return calcPower(psTemplate);
}


/* Calculate the power points required to build/maintain a droid */
UDWORD calcDroidPower(const DROID *psDroid)
{
	return calcPower(psDroid);
}

//Builds an instance of a Droid - the x/y passed in are in world coords.
DROID *reallyBuildDroid(const DROID_TEMPLATE *pTemplate, Position pos, UDWORD player, bool onMission, Rotation rot, uint32_t id)
{
	// Don't use this assertion in single player, since droids can finish building while on an away mission
	ASSERT(!bMultiPlayer || worldOnMap(pos.x, pos.y), "the build locations are not on the map");

	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "Invalid player: %" PRIu32 "", player);

	DROID *psDroid = new DROID(id, player);
	droidSetName(psDroid, getStatsName(pTemplate));

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

	if (isTransporter(psDroid) || psDroid->droidType == DROID_COMMAND)
	{
		DROID_GROUP *psGrp = grpCreate();
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
	psDroid->kills = 0;

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
		visTilesUpdate(psDroid);
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

		/* reset halt secondary order from guard to hold */
		secondarySetState(psDroid, DSO_HALTTYPE, DSS_HALT_HOLD);
	}

	if (player == selectedPlayer)
	{
		scoreUpdateVar(WD_UNITS_BUILT);
	}

	// Avoid droid appearing to jump or turn on spawn.
	psDroid->prevSpacetime.pos = psDroid->pos;
	psDroid->prevSpacetime.rot = psDroid->rot;

	debug(LOG_LIFE, "created droid for player %d, droid = %p, id=%d (%s): position: x(%d)y(%d)z(%d)", player, static_cast<void *>(psDroid), (int)psDroid->id, psDroid->aName, psDroid->pos.x, psDroid->pos.y, psDroid->pos.z);

	return psDroid;
}

DROID *reallyBuildDroid(const DROID_TEMPLATE *pTemplate, Position pos, UDWORD player, bool onMission, Rotation rot)
{
	const auto id = generateSynchronisedObjectId();
	return reallyBuildDroid(pTemplate, pos, player, onMission, rot, id);
}

DROID *buildDroid(DROID_TEMPLATE *pTemplate, UDWORD x, UDWORD y, UDWORD player, bool onMission, const INITIAL_DROID_ORDERS *initialOrders, Rotation rot)
{
	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "invalid player?: %" PRIu32 "", player);
	// ajl. droid will be created, so inform others
	if (bMultiMessages)
	{
		// Only sends if it's ours, otherwise the owner should send the message.
		SendDroid(pTemplate, x, y, player, generateNewObjectId(), initialOrders);
		return nullptr;
	}
	else
	{
		return reallyBuildDroid(pTemplate, Position(x, y, 0), player, onMission, rot);
	}
}

//initialises the droid movement model
void initDroidMovement(DROID *psDroid)
{
	psDroid->sMove.asPath.clear();
	psDroid->sMove.pathIndex = 0;
}

// Set the asBits in a DROID structure given it's template.
void droidSetBits(const DROID_TEMPLATE *pTemplate, DROID *psDroid)
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
void assignDroidsToGroup(UDWORD	playerNumber, UDWORD groupNumber, bool clearGroup)
{
	DROID	*psDroid;
	bool	bAtLeastOne = false;
	FLAG_POSITION	*psFlagPos;

	ASSERT_OR_RETURN(, playerNumber < MAX_PLAYERS, "Invalid player: %" PRIu32 "", playerNumber);

	if (groupNumber < UBYTE_MAX)
	{
		/* Run through all the droids */
		for (psDroid = apsDroidLists[playerNumber]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			/* Clear out the old ones */
			if (clearGroup && psDroid->group == groupNumber)
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
		ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "Unsupported selectedPlayer: %" PRIu32 "", selectedPlayer);
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
			 psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = false;
		}
		groupConsoleInformOfCreation(groupNumber);
		secondarySetAverageGroupState(selectedPlayer, groupNumber);
	}
}


void removeDroidsFromGroup(UDWORD playerNumber)
{
	DROID	*psDroid;
	unsigned removedCount = 0;

	ASSERT_OR_RETURN(, playerNumber < MAX_PLAYERS, "Invalid player: %" PRIu32 "", playerNumber);

	for (psDroid = apsDroidLists[playerNumber]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			psDroid->group = UBYTE_MAX;
			removedCount++;
		}
	}
	if (removedCount)
	{
		groupConsoleInformOfRemoval();
	}
}

bool activateGroupAndMove(UDWORD playerNumber, UDWORD groupNumber)
{
	DROID	*psDroid, *psCentreDroid = nullptr;
	bool selected = false;
	FLAG_POSITION	*psFlagPos;

	ASSERT_OR_RETURN(false, playerNumber < MAX_PLAYERS, "Invalid player: %" PRIu32 "", playerNumber);

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
			ASSERT(selectedPlayer < MAX_PLAYERS, "Unsupported selectedPlayer: %" PRIu32 "", selectedPlayer);
			if (selectedPlayer < MAX_PLAYERS)
			{
				for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
					 psFlagPos = psFlagPos->psNext)
				{
					psFlagPos->selected = false;
				}
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

bool activateNoGroup(UDWORD playerNumber, const SELECTIONTYPE selectionType, const SELECTION_CLASS selectionClass, const bool bOnScreen) {
	DROID	*psDroid;
	bool selected = false;
	FLAG_POSITION	*psFlagPos;
	SELECTIONTYPE dselectionType = selectionType;
	SELECTION_CLASS dselectionClass = selectionClass;
	unsigned selectionCount = 0;
	bool dbOnScreen = bOnScreen;

	ASSERT_OR_RETURN(false, playerNumber < MAX_PLAYERS, "Invalid player: %" PRIu32 "", playerNumber);

	selectionCount = selDroidSelection(selectedPlayer, dselectionClass, dselectionType, dbOnScreen);
	for (psDroid = apsDroidLists[playerNumber]; psDroid; psDroid = psDroid->psNext)
	{
		/* Wipe out the ones in the wrong group */
		if (psDroid->selected && psDroid->group != UBYTE_MAX)
		{
			DeSelectDroid(psDroid);
			selectionCount--;
		}
	}
	if (selected)
	{
		//clear the Deliv Point if one
		ASSERT_OR_RETURN(false, selectedPlayer < MAX_PLAYERS, "Unsupported selectedPlayer: %" PRIu32 "", selectedPlayer);
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
			 psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = false;
		}
	}
	CONPRINTF(ngettext("%u unit selected", "%u units selected", selectionCount), selectionCount);
	return selected;
}

bool activateGroup(UDWORD playerNumber, UDWORD groupNumber)
{
	DROID	*psDroid;
	bool selected = false;
	FLAG_POSITION	*psFlagPos;

	ASSERT_OR_RETURN(false, playerNumber < MAX_PLAYERS, "Invalid player: %" PRIu32 "", playerNumber);

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
		ASSERT_OR_RETURN(false, selectedPlayer < MAX_PLAYERS, "Unsupported selectedPlayer: %" PRIu32 "", selectedPlayer);
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

	CONPRINTF(ngettext("Group %u selected - %u Unit", "Group %u selected - %u Units", num_selected), groupNumber, num_selected);
}

void	groupConsoleInformOfCreation(UDWORD groupNumber)
{
	if (!getWarCamStatus())
	{
		unsigned int num_selected = selNumSelected(selectedPlayer);

		CONPRINTF(ngettext("%u unit assigned to Group %u", "%u units assigned to Group %u", num_selected), num_selected, groupNumber);
	}

}

void 	groupConsoleInformOfRemoval()
{
	if (!getWarCamStatus())
	{
		unsigned int num_selected = selNumSelected(selectedPlayer);

		CONPRINTF(ngettext("%u units removed from their Group", "%u units removed from their Group", num_selected), num_selected);
	}
}

void	groupConsoleInformOfCentering(UDWORD groupNumber)
{
	unsigned int num_selected = selNumSelected(selectedPlayer);

	if (!getWarCamStatus())
	{
		CONPRINTF(ngettext("Centered on Group %u - %u Unit", "Centered on Group %u - %u Units", num_selected), groupNumber, num_selected);
	}
	else
	{
		CONPRINTF(ngettext("Aligning with Group %u - %u Unit", "Aligning with Group %u - %u Units", num_selected), groupNumber, num_selected);
	}
}

/**
 * calculate muzzle base location in 3d world
 */
bool calcDroidMuzzleBaseLocation(const DROID *psDroid, Vector3i *muzzle, int weapon_slot)
{
	const iIMDShape *psBodyImd = BODY_IMD(psDroid, psDroid->player);

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

		*muzzle = (af * barrel).xzy();
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
bool calcDroidMuzzleLocation(const DROID *psDroid, Vector3i *muzzle, int weapon_slot)
{
	const iIMDShape *psBodyImd = BODY_IMD(psDroid, psDroid->player);

	CHECK_DROID(psDroid);

	if (psBodyImd && psBodyImd->nconnectors)
	{
		char debugStr[250], debugLen = 0;  // Each "(%d,%d,%d)" uses up to 34 bytes, for very large values. So 250 isn't exaggerating.

		Vector3i barrel(0, 0, 0);
		const iIMDShape *psWeaponImd = nullptr, *psMountImd = nullptr;

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

		*muzzle = (af * barrel).xzy();
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

struct rankMap
{
	unsigned int kills;          // required minimum amount of kills to reach this rank
	unsigned int commanderKills; // required minimum amount of kills for a commander (or sensor) to reach this rank
	const char  *name;           // name of this rank
};

unsigned int getDroidLevel(unsigned int experience, uint8_t player, uint8_t brainComponent)
{
	unsigned int numKills = experience / 65536;
	ASSERT_OR_RETURN(0, brainComponent < numBrainStats, "Invalid brainComponent: %u", (unsigned)brainComponent);
	const BRAIN_STATS *psStats = asBrainStats + brainComponent;
	ASSERT_OR_RETURN(0, player < MAX_PLAYERS, "Invalid player: %u", (unsigned)player);
	auto &vec = psStats->upgrade[player].rankThresholds;
	ASSERT_OR_RETURN(0, vec.size() > 0, "rankThreshold was empty?");
	for (int i = 1; i < vec.size(); ++i)
	{
		if (numKills < vec.at(i))
		{
			return i - 1;
		}
	}

	// If the criteria of the last rank are met, then select the last one
	return vec.size() - 1;
}

unsigned int getDroidLevel(const DROID *psDroid)
{
	// Sensors will use the first non-null brain component for ranks
	return getDroidLevel(psDroid->experience, psDroid->player, (psDroid->droidType != DROID_SENSOR) ? psDroid->asBits[COMP_BRAIN] : 1);
}

UDWORD getDroidEffectiveLevel(const DROID *psDroid)
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

const char *getDroidLevelName(const DROID *psDroid)
{
	const BRAIN_STATS *psStats = getBrainStats(psDroid);
	return PE_("rank", psStats->rankNames[getDroidLevel(psDroid)].c_str());
}

UDWORD	getNumDroidsForLevel(uint32_t player, UDWORD level)
{
	DROID	*psDroid;
	UDWORD	count;

	if (player >= MAX_PLAYERS) { return 0; }

	for (psDroid = apsDroidLists[player], count = 0;
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
	ASSERT_NOT_NULLPTR_OR_RETURN("", psDroid);
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

static bool ThreatInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs)
{
	UDWORD				i, structType;
	STRUCTURE			*psStruct;
	DROID				*psDroid;

	const int tx = map_coord(rangeX);
	const int ty = map_coord(rangeY);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if ((alliances[player][i] == ALLIANCE_FORMED) || (i == player))
		{
			continue;
		}

		//check structures
		for (psStruct = apsStructLists[i]; psStruct; psStruct = psStruct->psNext)
		{
			if (psStruct->visible[player] || psStruct->born == 2)	// if can see it or started there
			{
				if (psStruct->status == SS_BUILT)
				{
					structType = psStruct->pStructureType->type;

					switch (structType)		//dangerous to get near these structures
					{
					case REF_DEFENSE:
					case REF_CYBORG_FACTORY:
					case REF_FACTORY:
					case REF_VTOL_FACTORY:
					case REF_REARM_PAD:

						if (range < 0
							|| world_coord(static_cast<int32_t>(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y)))) < range)	//enemy in range
						{
							return true;
						}

						break;
					}
				}
			}
		}

		//check droids
		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->visible[player])		//can see this droid?
			{
				if (!objHasWeapon((BASE_OBJECT *)psDroid))
				{
					continue;
				}

				//if VTOLs are excluded, skip them
				if (!bVTOLs && ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION]].propulsionType == PROPULSION_TYPE_LIFT) || isTransporter(psDroid)))
				{
					continue;
				}

				if (range < 0
					|| world_coord(static_cast<int32_t>(hypotf(tx - map_coord(psDroid->pos.x), ty - map_coord(psDroid->pos.y)))) < range)	//enemy in range
				{
					return true;
				}
			}
		}
	}

	return false;
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
					return;
				}
			}
		}
	}
	cancelBuild(psDroid);
}

/* Just returns true if the droid's present body points aren't as high as the original*/
bool	droidIsDamaged(const DROID *psDroid)
{
	return psDroid->body < psDroid->originalBody;
}


char const *getDroidResourceName(char const *pName)
{
	/* See if the name has a string resource associated with it by trying
	 * to get the string resource.
	 */
	return strresGetString(psStringRes, pName);
}


/*checks to see if an electronic warfare weapon is attached to the droid*/
bool electronicDroid(const DROID *psDroid)
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
		for (const DROID *psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
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
bool droidUnderRepair(const DROID *psDroid)
{
	CHECK_DROID(psDroid);

	//droid must be damaged
	if (droidIsDamaged(psDroid))
	{
		//look thru the list of players droids to see if any are repairing this droid
		for (const DROID *psCurr = apsDroidLists[psDroid->player]; psCurr != nullptr; psCurr = psCurr->psNext)
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
bool vtolEmpty(const DROID *psDroid)
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
bool vtolFull(const DROID *psDroid)
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

	if (!isVtolDroid(psDroid) || psDroid->action != DACTION_WAITFORREARM)
	{
		return false;
	}

	// If a unit has been ordered to rearm make sure it goes to the correct base
	STRUCTURE *psRearmPad = castStructure(orderStateObj(psDroid, DORDER_REARM));
	if (psRearmPad && psRearmPad != psStruct && !vtolOnRearmPad(psRearmPad, psDroid))
	{
		// target rearm pad is clear - let it go there
		objTrace(psDroid->id, "rearm pad at %d,%d won't snatch us - we already have another available at %d,%d", psStruct->pos.x / TILE_UNITS, psStruct->pos.y / TILE_UNITS, psRearmPad->pos.x / TILE_UNITS, psRearmPad->pos.y / TILE_UNITS);
		return false;
	}

	if (vtolHappy(psDroid) && vtolOnRearmPad(psStruct, psDroid))
	{
		// there is a vtol on the pad and this vtol is already rearmed
		// don't bother shifting the other vtol off
		objTrace(psDroid->id, "rearm pad at %d,%d won't snatch us - we're rearmed and pad is busy", psStruct->pos.x / TILE_UNITS, psStruct->pos.y / TILE_UNITS);
		return false;
	}

	STRUCTURE *psTarget = castStructure(psDroid->psActionTarget[0]);
	if (psTarget && psTarget->pFunctionality && psTarget->pFunctionality->rearmPad.psObj == psDroid)
	{
		// vtol is rearming at a different base, leave it alone
		objTrace(psDroid->id, "rearm pad at %d,%d won't snatch us - we already are snatched by %d,%d", psStruct->pos.x / TILE_UNITS, psStruct->pos.y / TILE_UNITS, psTarget->pos.x / TILE_UNITS, psTarget->pos.y / TILE_UNITS);
		return false;
	}

	return true;
}

// true if a vtol droid currently returning to be rearmed
bool vtolRearming(const DROID *psDroid)
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
bool droidAttacking(const DROID *psDroid)
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
bool allVtolsRearmed(const DROID *psDroid)
{
	CHECK_DROID(psDroid);

	// ignore all non vtols
	if (!isVtolDroid(psDroid))
	{
		return true;
	}

	bool stillRearming = false;
	for (const DROID *psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
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
UWORD   getNumAttackRuns(const DROID *psDroid, int weapon_slot)
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

	if (droidIsDamaged(psDroid))
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
bool droidSensorDroidWeapon(const BASE_OBJECT *psObj, const DROID *psDroid)
{
	const SENSOR_STATS	*psStats = nullptr;
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
		if (((const DROID *)psObj)->droidType != DROID_SENSOR &&
			((const DROID *)psObj)->droidType != DROID_COMMAND)
		{
			return false;
		}
		compIndex = ((const DROID *)psObj)->asBits[COMP_SENSOR];
		ASSERT_OR_RETURN(false, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = asSensorStats + compIndex;
		break;
	case OBJ_STRUCTURE:
		psStats = ((const STRUCTURE *)psObj)->pStructureType->pSensor;
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
	if (isVtolDroid(psDroid) && psObj->type == OBJ_DROID
		&& ((const DROID *)psObj)->droidType == DROID_COMMAND)
	{
		return true;
	}

	//check vtol droid with vtol sensor
	if (isVtolDroid(psDroid) && psDroid->asWeaps[0].nStat > 0)
	{
		if (psStats->type == VTOL_INTERCEPT_SENSOR || psStats->type == VTOL_CB_SENSOR || psStats->type == SUPER_SENSOR /*|| psStats->type == RADAR_DETECTOR_SENSOR*/)
		{
			return true;
		}
		return false;
	}

	// Check indirect weapon droid with standard/CB/radar detector sensor
	if (!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat))
	{
		if ((psStats->type == STANDARD_SENSOR || psStats->type == INDIRECT_CB_SENSOR || psStats->type == SUPER_SENSOR /*|| psStats->type == RADAR_DETECTOR_SENSOR*/)
			&& !(psObj->type == OBJ_DROID && ((const DROID*)psObj)->droidType == DROID_COMMAND))
		{
			return true;
		}
		return false;
	}
	return false;
}

// return whether a droid has a CB sensor on it
bool cbSensorDroid(const DROID *psDroid)
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
bool standardSensorDroid(const DROID *psDroid)
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
DROID *giftSingleDroid(DROID *psD, UDWORD to, bool electronic)
{
	CHECK_DROID(psD);
	ASSERT_OR_RETURN(nullptr, !isDead(psD), "Cannot gift dead unit");
	ASSERT_OR_RETURN(psD, psD->player != to, "Cannot gift to self");
	ASSERT_OR_RETURN(nullptr, to < MAX_PLAYERS, "Cannot gift to = %" PRIu32 "", to);

	// Check unit limits (multiplayer only)
	syncDebug("Limits: %u/%d %u/%d %u/%d", getNumDroids(to), getMaxDroids(to), getNumConstructorDroids(to), getMaxConstructors(to), getNumCommandDroids(to), getMaxCommanders(to));
	if (bMultiPlayer
		&& ((int)getNumDroids(to) >= getMaxDroids(to)
			|| ((psD->droidType == DROID_CYBORG_CONSTRUCT || psD->droidType == DROID_CONSTRUCT)
				&& (int)getNumConstructorDroids(to) >= getMaxConstructors(to))
			|| (psD->droidType == DROID_COMMAND && (int)getNumCommandDroids(to) >= getMaxCommanders(to))))
	{
		if (to == selectedPlayer || psD->player == selectedPlayer)
		{
			CONPRINTF("%s", _("Unit transfer failed -- unit limits exceeded"));
		}
		return nullptr;
	}

	// electronic or campaign will destroy and recreate the droid.
	if (electronic || !bMultiPlayer)
	{
		DROID_TEMPLATE sTemplate;
		DROID *psNewDroid;

		templateSetParts(psD, &sTemplate);	// create a template based on the droid
		sTemplate.name = WzString::fromUtf8(psD->aName);	// copy the name across
		// update score
		if (psD->player == selectedPlayer && to != selectedPlayer && !bMultiPlayer)
		{
			scoreUpdateVar(WD_UNITS_LOST);
		}
		// make the old droid vanish (but is not deleted until next tick)
		adjustDroidCount(psD, -1);
		vanishDroid(psD);
		// create a new droid
		psNewDroid = reallyBuildDroid(&sTemplate, Position(psD->pos.x, psD->pos.y, 0), to, false, psD->rot);
		ASSERT_OR_RETURN(nullptr, psNewDroid, "Unable to build unit");

		addDroid(psNewDroid, apsDroidLists);
		adjustDroidCount(psNewDroid, 1);

		psNewDroid->body = clip((psD->body*psNewDroid->originalBody + psD->originalBody/2)/std::max(psD->originalBody, 1u), 1u, psNewDroid->originalBody);
		psNewDroid->experience = psD->experience;
		psNewDroid->kills = psD->kills;

		if (!(psNewDroid->droidType == DROID_PERSON || cyborgDroid(psNewDroid) || isTransporter(psNewDroid)))
		{
			updateDroidOrientation(psNewDroid);
		}

		triggerEventObjectTransfer(psNewDroid, psD->player);
		return psNewDroid;
	}

	int oldPlayer = psD->player;

	// reset the assigned state of units attached to a leader
	for (DROID *psCurr = apsDroidLists[oldPlayer]; psCurr != nullptr; psCurr = psCurr->psNext)
	{
		BASE_OBJECT	*psLeader;

		if (hasCommander(psCurr))
		{
			psLeader = (BASE_OBJECT *)psCurr->psGroup->psCommander;
		}
		else
		{
			//psLeader can be either a droid or a structure
			psLeader = orderStateObj(psCurr, DORDER_FIRESUPPORT);
		}

		if (psLeader && psLeader->id == psD->id)
		{
			psCurr->selected = false;
			orderDroid(psCurr, DORDER_STOP, ModeQueue);
		}
	}

	visRemoveVisibility((BASE_OBJECT *)psD);
	psD->selected = false;

	adjustDroidCount(psD, -1);
	scriptRemoveObject(psD); //Remove droid from any script groups

	if (droidRemove(psD, apsDroidLists))
	{
		psD->player	= to;

		addDroid(psD, apsDroidLists);
		adjustDroidCount(psD, 1);

		// the new player may have different default sensor/ecm/repair components
		if ((asSensorStats + psD->asBits[COMP_SENSOR])->location == LOC_DEFAULT)
		{
			if (psD->asBits[COMP_SENSOR] != aDefaultSensor[psD->player])
			{
				psD->asBits[COMP_SENSOR] = aDefaultSensor[psD->player];
			}
		}
		if ((asECMStats + psD->asBits[COMP_ECM])->location == LOC_DEFAULT)
		{
			if (psD->asBits[COMP_ECM] != aDefaultECM[psD->player])
			{
				psD->asBits[COMP_ECM] = aDefaultECM[psD->player];
			}
		}
		if ((asRepairStats + psD->asBits[COMP_REPAIRUNIT])->location == LOC_DEFAULT)
		{
			if (psD->asBits[COMP_REPAIRUNIT] != aDefaultRepair[psD->player])
			{
				psD->asBits[COMP_REPAIRUNIT] = aDefaultRepair[psD->player];
			}
		}
	}
	else
	{
		// if we couldn't remove it, then get rid of it.
		return nullptr;
	}

	// Update visibility
	visTilesUpdate((BASE_OBJECT*)psD);

	// check through the players, and our allies, list of droids to see if any are targetting it
	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!aiCheckAlliances(i, to))
		{
			continue;
		}

		for (DROID *psCurr = apsDroidLists[i]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			if (psCurr->order.psObj == psD || psCurr->psActionTarget[0] == psD)
			{
				orderDroid(psCurr, DORDER_STOP, ModeQueue);
				break;
			}
			for (unsigned iWeap = 0; iWeap < psCurr->numWeaps; ++iWeap)
			{
				if (psCurr->psActionTarget[iWeap] == psD)
				{
					orderDroid(psCurr, DORDER_STOP, ModeImmediate);
					break;
				}
			}
			// check through order list
			orderClearTargetFromDroidList(psCurr, (BASE_OBJECT *)psD);
		}
	}

	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!aiCheckAlliances(i, to))
		{
			continue;
		}

		// check through the players list, and our allies, of structures to see if any are targetting it
		for (STRUCTURE *psStruct = apsStructLists[i]; psStruct != nullptr; psStruct = psStruct->psNext)
		{
			if (psStruct->psTarget[0] == psD)
			{
				setStructureTarget(psStruct, nullptr, 0, ORIGIN_UNKNOWN);
			}
		}
	}

	triggerEventObjectTransfer(psD, oldPlayer);
	return psD;
}

/*calculates the electronic resistance of a droid based on its experience level*/
SWORD   droidResistance(const DROID *psDroid)
{
	CHECK_DROID(psDroid);
	const BODY_STATS *psStats = asBodyStats + psDroid->asBits[COMP_BODY];
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

// Check if a droid can be selected.
bool isSelectable(DROID const *psDroid)
{
	if (psDroid->flags.test(OBJECT_FLAG_UNSELECTABLE))
	{
		return false;
	}

	// we shouldn't ever control the transporter in SP games
	if (isTransporter(psDroid) && !bMultiPlayer)
	{
		return false;
	}

	return true;
}

// Select a droid and do any necessary housekeeping.
//
void SelectDroid(DROID *psDroid)
{
	if (!isSelectable(psDroid))
	{
		return;
	}

	psDroid->selected = true;
	intRefreshScreen();
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
