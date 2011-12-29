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
/*
	Spot FX code - will handle every miscellaneous imd render and update for temporary
	entities except projectiles.
	Handles stuff like
	- Smoke sprites on the card.
	- Explosions
	- Building body kit - flashing lights etc etc
	- Construction graphics
	- Gravitons
	- Dust
	- Blood

	It's now PSX friendly in that there's no floats

	************************************************************
	* STILL NEED TO REMOVE SOME MAGIC NUMBERS INTO #DEFINES!!! *
	************************************************************
*/
#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/input.h"
#include "lib/framework/math_ext.h"

#include "lib/ivis_opengl/ivisdef.h" //ivis matrix code
#include "lib/ivis_opengl/piedef.h" //ivis matrix code
#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piemode.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "display3d.h"
#include "map.h"
#include "bucket3d.h"

#include "effects.h"

#include "mission.h"

#include "miscimd.h"
#include "hci.h"
#include "lighting.h"
#include "console.h"
#include "loop.h"

#include "multiplay.h"
#include "game.h"
#include "component.h"

#define	GRAVITON_GRAVITY	((float)-800)
#define	EFFECT_X_FLIP		0x1
#define	EFFECT_Y_FLIP		0x2
#define	EFFECT_CYCLIC		0x4
#define EFFECT_ESSENTIAL	0x8
#define EFFECT_FACING		0x10
#define	EFFECT_SCALED		0x20
#define	EFFECT_LIT			0x40

#define	TEST_FLIPPED_X(x)		(x->control & EFFECT_X_FLIP)
#define	TEST_FLIPPED_Y(x)		(x->control & EFFECT_Y_FLIP)
#define TEST_ESSENTIAL(x)		(x->control & EFFECT_ESSENTIAL)
#define TEST_FACING(x)			(x->control & EFFECT_FACING)
#define TEST_CYCLIC(x)			(x->control & EFFECT_CYCLIC)
#define TEST_SCALED(x)			(x->control & EFFECT_SCALED)
#define TEST_LIT(x)				(x->control & EFFECT_LIT)

#define	SET_FLIPPED_X(x)		((x->control) = (UBYTE)(x->control | EFFECT_X_FLIP))
#define	SET_FLIPPED_Y(x)		((x->control) = (UBYTE)(x->control | EFFECT_Y_FLIP))
#define SET_ESSENTIAL(x)		((x->control) = (UBYTE)(x->control | EFFECT_ESSENTIAL))
#define SET_FACING(x)			((x->control) = (UBYTE)(x->control | EFFECT_FACING))
#define SET_CYCLIC(x)			((x->control) = (UBYTE)(x->control | EFFECT_CYCLIC))
#define SET_SCALED(x)			((x->control) = (UBYTE)(x->control | EFFECT_SCALED))
#define SET_LIT(x)				((x->control) = (UBYTE)(x->control | EFFECT_LIT))

#define MINIMUM_IMPACT_VELOCITY		(16)
#define	NORMAL_SMOKE_LIFESPAN		(6000 + rand()%3000)
#define SMALL_SMOKE_LIFESPAN		(3000 + rand()%3000)
#define	TRAIL_SMOKE_LIFESPAN		(1200)
#define	CONSTRUCTION_LIFESPAN		(5000)

#define	SMOKE_FRAME_DELAY			(40 + rand()%30)
#define	EXPLOSION_FRAME_DELAY		(25 + rand()%40)
#define	EXPLOSION_TESLA_FRAME_DELAY	(65)
#define	EXPLOSION_PLASMA_FRAME_DELAY	(45)
#define	BLOOD_FRAME_DELAY			(150)
#define DESTRUCTION_FRAME_DELAY		(200)

#define	TESLA_SPEED					(170)// + (30 - rand()%60))
#define	TESLA_SIZE					(100)// + (20 - rand()%40))

#define GRAVITON_FRAME_DELAY		(100 + rand()%50)
#define GRAVITON_BLOOD_DELAY		(200 + rand()%100)

#define CONSTRUCTION_FRAME_DELAY	(40 + rand()%30)

#define	EXPLOSION_SIZE				(110+(30-rand()%60))
#define	LIMITED_EXPLOSION_SIZE		(100+(5-rand()%10))
#define	BLOOD_SIZE					(100+(30-rand()%60))
#define	BLOOD_FALL_SPEED			(-(20+rand()%20))

#define GRAVITON_INIT_VEL_X			(float)(200 - rand() % 300)
#define GRAVITON_INIT_VEL_Z			(float)(200 - rand() % 300)
#define GRAVITON_INIT_VEL_Y			(float)(300 + rand() % 100)

#define GIBLET_INIT_VEL_X			(float)(50 - rand() % 100)
#define GIBLET_INIT_VEL_Z			(float)(50 - rand() % 100)
#define GIBLET_INIT_VEL_Y			12.f

#define	DROID_DESTRUCTION_DURATION		(3*GAME_TICKS_PER_SEC/2) // 1.5 seconds
#define	STRUCTURE_DESTRUCTION_DURATION	((7*GAME_TICKS_PER_SEC)/2)	 // 3.5 seconds


#define FIREWORK_EXPLODE_HEIGHT			400
#define STARBURST_RADIUS				150
#define STARBURST_DIAMETER				300

#define	EFFECT_SMOKE_ADDITIVE			1
#define	EFFECT_STEAM_ADDITIVE			8
#define	EFFECT_WAYPOINT_ADDITIVE		32
#define	EFFECT_EXPLOSION_ADDITIVE		164
#define EFFECT_PLASMA_ADDITIVE			224
#define	EFFECT_SMOKE_TRANSPARENCY		130
#define	EFFECT_STEAM_TRANSPARENCY		128
#define	EFFECT_WAYPOINT_TRANSPARENCY	128
#define	EFFECT_BLOOD_TRANSPARENCY		128
#define	EFFECT_EXPLOSION_TRANSPARENCY	164

#define	EFFECT_DROID_DIVISION			101
#define	EFFECT_STRUCTURE_DIVISION		103

#define	DROID_UPDATE_INTERVAL			500
#define	STRUCTURE_UPDATE_INTERVAL		1250
#define	BASE_FLAME_SIZE					80
#define	BASE_LASER_SIZE					10
#define BASE_PLASMA_SIZE				0
#define	DISCOVERY_SIZE					60
#define	FLARE_SIZE						100
#define SHOCKWAVE_SPEED	(GAME_TICKS_PER_SEC)
#define	MAX_SHOCKWAVE_SIZE				500


/*! Number of effects in one chunk */
#define EFFECT_CHUNK_SIZE 10000


/*! A memory chunk of effects */
struct EffectChunk
{
	EFFECT effects[EFFECT_CHUNK_SIZE]; //!< Chunk of effects
	EffectChunk *next; //!< Next element in list
};


/*! List containing all allocated effect chunks */
struct chunkList_t {
	EffectChunk *first; //!< First element of list, used for iteration
	EffectChunk *last; //!< Last element of list, used for appending
} chunkList = {
	NULL, NULL
};


/*! Our lists of all game world effects */
struct activeList_t {
	size_t num; //!< Number of effects in this list, used when saving
	EFFECT *first; //!< First element of list, used for iteration / finding free effects
	EFFECT *last; //!< Last element of list, used for appending
} activeList = { //!< List of all active effects
	0, NULL, NULL
}, inactiveList = { //!< List of unused effects
	0, NULL, NULL
};


/* Tick counts for updates on a particular interval */
static	UDWORD	lastUpdateDroids[EFFECT_DROID_DIVISION];
static	UDWORD	lastUpdateStructures[EFFECT_STRUCTURE_DIVISION];

static	UDWORD	auxVar; // dirty filthy hack - don't look for what this does.... //FIXME
static	UDWORD	auxVarSec; // dirty filthy hack - don't look for what this does.... //FIXME
static	UDWORD	specifiedSize;
static	UDWORD	ellSpec;
static	uint8_t	EffectForPlayer = 0;
// ----------------------------------------------------------------------------------------
/* PROTOTYPES */

// ----------------------------------------------------------------------------------------
// ---- Update functions - every group type of effect has one of these */
static void updateWaypoint(EFFECT *psEffect);
static void updateExplosion(EFFECT *psEffect);
static void updatePolySmoke(EFFECT *psEffect);
static void updateGraviton(EFFECT *psEffect);
static void updateConstruction(EFFECT *psEffect);
static void updateBlood	(EFFECT *psEffect);
static void updateDestruction(EFFECT *psEffect);
static void updateFire(EFFECT *psEffect);
static void updateSatLaser(EFFECT *psEffect);
static void updateFirework(EFFECT *psEffect);
static void updateEffect(EFFECT *psEffect);	// MASTER function

// ----------------------------------------------------------------------------------------
// ---- The render functions - every group type of effect has a distinct one
static void	renderExplosionEffect	( const EFFECT *psEffect );
static void	renderSmokeEffect		( const EFFECT *psEffect );
static void	renderGravitonEffect	( const EFFECT *psEffect );
static void	renderConstructionEffect( const EFFECT *psEffect );
static void	renderWaypointEffect	( const EFFECT *psEffect );
static void	renderBloodEffect		( const EFFECT *psEffect );
static void	renderDestructionEffect	( const EFFECT *psEffect );
static void renderFirework			( const EFFECT *psEffect );

static void positionEffect(const EFFECT *psEffect);
/* There is no render destruction effect! */

// ----------------------------------------------------------------------------------------
// ---- The set up functions - every type has one
static void	effectSetupSmoke		( EFFECT *psEffect );
static void	effectSetupGraviton		( EFFECT *psEffect );
static void	effectSetupExplosion	( EFFECT *psEffect );
static void	effectSetupConstruction ( EFFECT *psEffect );
static void	effectSetupWayPoint		( EFFECT *psEffect );
static void	effectSetupBlood		( EFFECT *psEffect );
static void effectSetupDestruction  ( EFFECT *psEffect );
static void	effectSetupFire			( EFFECT *psEffect );
static void	effectSetupSatLaser		( EFFECT *psEffect );
static void effectSetupFirework		( EFFECT *psEffect );

static void effectStructureUpdates(void);
static void effectDroidUpdates(void);

static UDWORD effectGetNumFrames(EFFECT *psEffect);
static void killEffect(EFFECT *e);

/*!
 * Initialise memory between first and last as singly linked list
 * \param first First element in memory chunk
 * \param last Last element in memory chunk
 */
static void initEffectPool(EFFECT *first, EFFECT *last)
{
	EFFECT *it;
	for (it = first; it < last; it++)
	{
		// We do not need a double-linked-list for inactiveeffects, since we always pick from the front:
		it->prev = NULL;
		it->next = it+1;
	}

	last->prev = NULL;
	last->next = NULL;
}


/*!
 * Allocate a new effect from memory pool
 * FIXME: Does not deal with out-of-memory conditions (yet)
 * \return New, uninitialised effect
 */
static EFFECT *Effect_malloc(void)
{
	/* Take the first item in inactiveList */
	EFFECT *instance = inactiveList.first;

	/* Remove from inactiveList */
	inactiveList.first = instance->next; // Singly linked, so do not update prev
	instance->next = NULL; // We are the last in activeList now

	/* Append to activeList */
	instance->prev = activeList.last;

	if (instance->prev == NULL) {
		activeList.first = instance; // activeList was empty, so fill it
	}
	else {
		activeList.last->next = instance;
	}

	activeList.last = instance;

	/* Adjust counts */
	activeList.num++;
	inactiveList.num--;

	/* Ensure the next search will have something to feed its hunger with */
	if (inactiveList.first == NULL)
	{
		/* Allocate new effect chunk */
		EffectChunk *chunk = (EffectChunk *)calloc(1, sizeof(EffectChunk));

		debug(LOG_MEMORY, "%lu effects in use, allocating %d extra", (unsigned long)activeList.num, EFFECT_CHUNK_SIZE);

		/* Deal with out-of-memory conditions */
		if (chunk == NULL) {
			debug(LOG_ERROR, "Out of memory");
			return NULL; // The next call relies on inactiveList being non-empty, and would thus segfault, so bail out early instead
		}

		/* Append to list */
		chunk->next = NULL; // Last element
		chunkList.last->next = chunk;  // chunkList is never empty, thus we can rely on last!=NULL

		/* Update inactiveList */
		inactiveList.num = EFFECT_CHUNK_SIZE;
		inactiveList.first = &chunk->effects[0];
		inactiveList.last = &chunk->effects[EFFECT_CHUNK_SIZE-1];

		/* Initialise list links between inactive effects */
		initEffectPool(inactiveList.first, inactiveList.last);
	}

	return instance;
}


/*!
 * Return an effect into memory pool
 * \param self Effect to be freed
 */
static void Effect_free(EFFECT *instance)
{
	instance->group = EFFECT_FREED;

	/* Remove from activeList and fixup endings necessary */
	if (instance->prev != NULL) {
		instance->prev->next = instance->next;
	}
	else {
		activeList.first = instance->next; // We were the first in activeList
	}
	if (instance->next != NULL) {
		instance->next->prev = instance->prev;
	}
	else {
		activeList.last = instance->prev; // We were the last in activeList
	}

	/* Append to inactiveList */
	instance->prev = NULL; // Singly linked
	instance->next = NULL; // Last element

	 // inactiveList is never empty (guaranteed in Effect_malloc), thus we can rely on last!=NULL
	inactiveList.last->next = instance;
	inactiveList.last = instance;

	/* Adjust counts */
	inactiveList.num++;
	ASSERT_OR_RETURN(, activeList.num > 0, "Underflow");
	activeList.num--;
}

void shutdownEffectsSystem(void)
{
	EFFECT *eff;

	/* Traverse the list */
	for (eff = activeList.first; eff;)
	{
		EFFECT *effNext = eff->next;

		killEffect(eff);
		eff = effNext;
	}
	chunkList.first = NULL;
	chunkList.last = NULL;
}

/*!
 * Initialise effects system
 * Cleans up old effects, allocates a first chunk of effects, initialises (in)active lists
 */
void initEffectsSystem(void)
{
	EffectChunk *chunk;

	/* Clean up old chunks */
	shutdownEffectsSystem();

	/* Allocate new chunk */
	chunk = (EffectChunk *)calloc(1, sizeof(EffectChunk));

	/* Deal with out-of-memory conditions */
	if (chunk == NULL)
		return; // FIXME We have no way to report an error here...

	/* Create chunkList */
	chunkList.first = chunk;
	chunkList.last = chunk;
	chunk->next = NULL; // Last element

	/* activeList is empty at start */
	activeList.num = 0;
	activeList.first = NULL;
	activeList.last = NULL;

	/* inactiveList contains all elements */
	inactiveList.num = EFFECT_CHUNK_SIZE;
	inactiveList.first = &chunk->effects[0];
	inactiveList.last = &chunk->effects[EFFECT_CHUNK_SIZE-1];

	/* Initialise free-effects pool */
	initEffectPool(inactiveList.first, inactiveList.last);
}


static void positionEffect(const EFFECT *psEffect)
{
	/* Establish world position */
	Vector3i dv(
		psEffect->position.x - player.p.x,
		psEffect->position.y,
		-(psEffect->position.z - player.p.z)
	);

	/* Push the indentity matrix */
	pie_MatBegin();

	/* Move to position */
	pie_TRANSLATE(dv.x, dv.y, dv.z);
}

static void killEffect(EFFECT *e)
{
	/* Put effect back into pool */
	Effect_free(e);
}

void	effectSetLandLightSpec(LAND_LIGHT_SPEC spec)
{
	ellSpec = spec;
}

void	effectSetSize(UDWORD size)
{
	specifiedSize = size;
}

void addMultiEffect(const Vector3i *basePos, Vector3i *scatter, EFFECT_GROUP group,
                    EFFECT_TYPE type, bool specified, iIMDShape *imd, unsigned int number, bool lit, unsigned int size, unsigned effectTime)
{
	if (number == 0)
	{
		return;
	}

	/* Set up the scaling for specified ones */
	specifiedSize = size;

	/* If there's only one, make sure it's in the centre */
	if (number == 1)
	{
		addEffect(basePos, group, type, specified, imd, lit, effectTime);
	}
	else
	{
		unsigned int i;

		/* Fix for jim */
		*scatter = *scatter / 10;

		/* There are multiple effects - so scatter them around according to parameter */
		for(i=0; i<number; i++)
		{
			// This scatters in a cube - is there a good reason for that, or just legacy?
			Vector3i scatPos = *basePos + *scatter - Vector3i(rand()%(scatter->x*2 + 1),
			                                                  rand()%(scatter->y*2 + 1),
			                                                  rand()%(scatter->z*2 + 1)
			                                                 );
			addEffect(&scatPos, group, type, specified, imd, lit, effectTime);
		}
	}
}

// When we need to set the effect for the player's color
void	SetEffectForPlayer(uint8_t player)
{
	ASSERT(player < MAX_PLAYERS, "player is set to a invalid number of %d", (int) player);

	EffectForPlayer = getPlayerColour(player);
}

void addEffect(const Vector3i *pos, EFFECT_GROUP group, EFFECT_TYPE type, bool specified, iIMDShape *imd, int lit)
{
	return addEffect(pos, group, type, specified, imd, lit, graphicsTime);
}

void addEffect(const Vector3i *pos, EFFECT_GROUP group, EFFECT_TYPE type, bool specified, iIMDShape *imd, int lit, unsigned effectTime)
{
	EFFECT *psEffect = NULL;

	if(gamePaused())
	{
		return;
	}

	/* Retrieve a new effect from pool */
	psEffect = Effect_malloc();

	/* Deal with out-of-memory conditions */
	if (psEffect == NULL) {
		debug(LOG_ERROR, "Out of memory");
		return; // FIXME We have no way to report an error here...
	}

	/* Reset control bits */
	psEffect->control = 0;

	/* Store away it's position - into FRACTS */
	psEffect->position.x = pos->x;
	psEffect->position.y = pos->y;
	psEffect->position.z = pos->z;

	/* Now, note group and type */
	psEffect->group = group;
	psEffect->type = type;

	// and if the effect needs the player's color for certain things
	psEffect->player = EffectForPlayer;
	SetEffectForPlayer(0);	// reset it

	/* Set when it entered the world */
	psEffect->birthTime = psEffect->lastFrame = effectTime;

	if(group == EFFECT_GRAVITON && (type == GRAVITON_TYPE_GIBLET || type == GRAVITON_TYPE_EMITTING_DR))
	{
		psEffect->frameNumber = lit;
	}
	else
	{
		/* Starts off on frame zero */
		psEffect->frameNumber = 0;
	}

	/*
		See what kind of effect it is - the add fucnction is different for each,
		although some things are shared
	*/
	psEffect->imd = NULL;
	if(lit)
	{
		SET_LIT(psEffect);
	}

	if(specified)
	{
		/* We're specifying what the imd is - override */
		psEffect->imd = imd;
		psEffect->size = specifiedSize;
	}

	/* Do all the effect type specific stuff */
	switch(group)
	{
		case EFFECT_SMOKE:
			effectSetupSmoke(psEffect);
			break;
		case EFFECT_GRAVITON:
			effectSetupGraviton(psEffect);
			break;
		case EFFECT_EXPLOSION:
			effectSetupExplosion(psEffect);
			break;
		case EFFECT_CONSTRUCTION:
			effectSetupConstruction(psEffect);
			break;
		case EFFECT_WAYPOINT:
			effectSetupWayPoint(psEffect);
			break;
		case EFFECT_BLOOD:
			effectSetupBlood(psEffect);
			break;
		case EFFECT_DESTRUCTION:
			effectSetupDestruction(psEffect);
			break;
		case EFFECT_FIRE:
			effectSetupFire(psEffect);
			break;
		case EFFECT_SAT_LASER:
			effectSetupSatLaser(psEffect);
			break;
		case EFFECT_FIREWORK:
			effectSetupFirework(psEffect);
			break;
		case EFFECT_FREED:
			ASSERT( false,"Weirdy group type for an effect" );
			break;
	}

	/* As of yet, it hasn't bounced (or whatever)... */
	if(type!=EXPLOSION_TYPE_LAND_LIGHT)
	{
		psEffect->specific = 0;
	}

	ASSERT(psEffect->imd != NULL || group == EFFECT_DESTRUCTION || group == EFFECT_FIRE || group == EFFECT_SAT_LASER, "null effect imd");
}


/* Calls all the update functions for each different currently active effect */
void processEffects(void)
{
	EFFECT *it;
	/* Traverse the list */
	for (it = activeList.first; it != NULL;)
	{
		EFFECT *itNext = it->next; // If updateEffect deletes something, we would be screwed...

		if (it->birthTime <= graphicsTime)  // Don't process, if it doesn't exist yet.
		{
			/* Run updates, effect may be deleted here */
			updateEffect(it);

			/* Is it on the grid */
			if (it->group != EFFECT_FREED && clipXY(it->position.x, it->position.z))
			{
				/* Add it to the bucket */
				bucketAddTypeToList(RENDER_EFFECT, it);
			}
		}

		it = itNext;
	}

	/* Add any droid effects */
	effectDroidUpdates();

	/* Add any structure effects */
	effectStructureUpdates();
}


/* The general update function for all effects - calls a specific one for each */
static void updateEffect(EFFECT *psEffect)
{
	/* What type of effect are we dealing with? */
	switch(psEffect->group)
	{
	case EFFECT_EXPLOSION:
		updateExplosion(psEffect);
		return;

	case EFFECT_WAYPOINT:
		if(!gamePaused()) updateWaypoint(psEffect);
		return;

	case EFFECT_CONSTRUCTION:
		if(!gamePaused()) updateConstruction(psEffect);
		return;

	case EFFECT_SMOKE:
		if(!gamePaused()) updatePolySmoke(psEffect);
		return;

	case EFFECT_GRAVITON:
		if(!gamePaused()) updateGraviton(psEffect);
		return;

	case EFFECT_BLOOD:
		if(!gamePaused()) updateBlood(psEffect);
		return;

	case EFFECT_DESTRUCTION:
		if(!gamePaused()) updateDestruction(psEffect);
		return;

	case EFFECT_FIRE:
		if(!gamePaused()) updateFire(psEffect);
		return;

	case EFFECT_SAT_LASER:
		if(!gamePaused()) updateSatLaser(psEffect);
		return;

	case EFFECT_FIREWORK:
		if(!gamePaused()) updateFirework(psEffect);
		return;

	case EFFECT_FREED:
		break;
	}

	debug( LOG_ERROR, "Weirdy class of effect passed to updateEffect" );
	abort();
}

// ----------------------------------------------------------------------------------------
// ALL THE UPDATE FUNCTIONS
// ----------------------------------------------------------------------------------------
/** Update the waypoint effects.*/
static void updateWaypoint(EFFECT *psEffect)
{
	if(!(keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) ||
	     keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)))
	{
		killEffect(psEffect);
	}
}

static void updateFirework(EFFECT *psEffect)
{
	UDWORD	height;
	UDWORD	xDif,yDif,radius,val;
	Vector3i dv;
	SDWORD	dif;
	UDWORD	drop;

	/* Move it */
	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);

	if(psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
		height = psEffect->position.y;
		if(height > psEffect->size)
		{
			dv.x = psEffect->position.x;
   			dv.z = psEffect->position.z;
   			dv.y = psEffect->position.y + (psEffect->radius / 2);
			addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,false,NULL,0);
			audio_PlayStaticTrack(psEffect->position.x, psEffect->position.z, ID_SOUND_EXPLOSION);

			for(dif =0; dif < (psEffect->radius*2); dif+=20)
			{
				if(dif<psEffect->radius)
				{
  					drop = psEffect->radius-dif;
				}
				else
				{
					drop = dif - psEffect->radius;
				}
				radius = (UDWORD)sqrtf(psEffect->radius * psEffect->radius - drop * drop);
				//val = getStaticTimeValueRange(720,360);	// grab an angle - 4 seconds cyclic
  				for(val = 0; val<=180; val+=20)
				{
					xDif = iSinR(DEG(val), radius);
					yDif = iCosR(DEG(val), radius);
   					dv.x = psEffect->position.x + xDif;
   					dv.z = psEffect->position.z + yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,false,NULL,0);
					dv.x = psEffect->position.x - xDif;
   					dv.z = psEffect->position.z - yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,false,NULL,0);
					dv.x = psEffect->position.x + xDif;
   					dv.z = psEffect->position.z - yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,false,NULL,0);
					dv.x = psEffect->position.x - xDif;
   					dv.z = psEffect->position.z + yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,false,NULL,0);
				}
			}
			killEffect(psEffect);
		}
		else
		{
			/* Add an effect at the firework's position */
			dv.x = psEffect->position.x;
			dv.y = psEffect->position.y;
			dv.z = psEffect->position.z;

			/* Add a trail graphic */
			addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,false,NULL,0);
		}
	}
	else	// must be a startburst
	{
		/* Time to update the frame number on the smoke sprite */
		if(graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
		{
			/* Store away last frame change time */
			psEffect->lastFrame = graphicsTime;

			/* Are we on the last frame? */
			if(++psEffect->frameNumber >= effectGetNumFrames(psEffect))
			{
				/* Does the anim wrap around? */
				if(TEST_CYCLIC(psEffect))
				{
					psEffect->frameNumber = 0;
				}
				else
				{
					/* Kill it off */
 					killEffect(psEffect);
					return;
				}
			}
		}

		/* If it doesn't get killed by frame number, then by age */
		if(TEST_CYCLIC(psEffect))
		{
			/* Has it overstayed it's welcome? */
			if(graphicsTime - psEffect->birthTime > psEffect->lifeSpan)
			{
				/* Kill it */
				killEffect(psEffect);
			}
		}
	}
}

static void updateSatLaser(EFFECT *psEffect)
{
	Vector3i dv;
	UDWORD	val;
	UDWORD	radius;
	UDWORD	xDif,yDif;
	UDWORD	i;
	UDWORD	startHeight,endHeight;
	UDWORD	xPos,yPos;
	LIGHT	light;

	// Do these here cause there used by the lighting code below this if.
	xPos = psEffect->position.x;
	startHeight = psEffect->position.y;
	endHeight = startHeight+1064;
	yPos = psEffect->position.z;

	if(psEffect->baseScale)
	{
		psEffect->baseScale = 0;

		/* Add some big explosions....! */

		for(i=0; i<16; i++)
		{
			dv.x = xPos+(200-rand()%400);
			dv.z = yPos+(200-rand()%400);
			dv.y = startHeight + rand()%100;
			addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,false,NULL,0);
		}
		/* Add a sound effect */
  		audio_PlayStaticTrack(psEffect->position.x, psEffect->position.z, ID_SOUND_EXPLOSION);

		/* Add a shockwave */
		dv.x = xPos;
		dv.z = yPos;
		dv.y = startHeight+SHOCK_WAVE_HEIGHT;
		addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_SHOCKWAVE,false,NULL,0);


		/* Now, add the column of light */
		for(i=startHeight; i<endHeight; i+=56)
		{
			radius = 80;
			/* Add 36 around in a circle..! */
			for(val = 0; val<=180; val+=30)
			{
				xDif = iSinR(DEG(val), radius);
				yDif = iCosR(DEG(val), radius);
   				dv.x = xPos+xDif+i/64;
   				dv.z = yPos+yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,false,NULL,0);
				dv.x = xPos-xDif+i/64;
   				dv.z = yPos-yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,false,NULL,0);
				dv.x = xPos+xDif+i/64;
   				dv.z = yPos-yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,false,NULL,0);
				dv.x = xPos-xDif+i/64;
   				dv.z = yPos+yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,false,NULL,0);
			}
		}
	}

	if(graphicsTime - psEffect->birthTime < 1000)
	{
  		light.position.x = xPos;
		light.position.y = startHeight;
		light.position.z = yPos;
		light.range = 800;
		light.colour = LIGHT_BLUE;
		processLight(&light);

	}
	else
	{
		killEffect(psEffect);
	}
}

/** The update function for the explosions */
static void updateExplosion(EFFECT *psEffect)
{
	LIGHT light;
	UDWORD percent;
	UDWORD range;
	float scaling;

	if (TEST_LIT(psEffect))
	{
		if (psEffect->lifeSpan)
		{
			percent = PERCENT(graphicsTime - psEffect->birthTime, psEffect->lifeSpan);
			if (percent > 100)
			{
				percent = 100;
			}
			else if (percent > 50)
			{
				percent = 100 - percent;
			}
		}
		else
		{
			percent = 100;
		}

		range = percent;
		light.position.x = psEffect->position.x;
		light.position.y = psEffect->position.y;
		light.position.z = psEffect->position.z;
		light.range = (3*range)/2;
		light.colour = LIGHT_RED;
		processLight(&light);
	}

	if (psEffect->type == EXPLOSION_TYPE_SHOCKWAVE)
	{
		psEffect->size += graphicsTimeAdjustedIncrement(SHOCKWAVE_SPEED);
		scaling = (float)psEffect->size / (float)MAX_SHOCKWAVE_SIZE;
		psEffect->frameNumber = scaling * effectGetNumFrames(psEffect);

		light.position.x = psEffect->position.x;
		light.position.y = psEffect->position.y;
		light.position.z = psEffect->position.z;
		light.range = psEffect->size+200;
		light.colour = LIGHT_YELLOW;
		processLight(&light);

		if (psEffect->size > MAX_SHOCKWAVE_SIZE || light.range > 600)
		{
 			/* Kill it off */
			killEffect(psEffect);
			return;
		}
	}
	/* Time to update the frame number on the explosion */
	else while (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame += psEffect->frameDelay;
		/* Are we on the last frame? */

		if (++psEffect->frameNumber >= effectGetNumFrames(psEffect))
		{
			if (psEffect->type != EXPLOSION_TYPE_LAND_LIGHT)
			{
				/* Kill it off */
				killEffect(psEffect);
				return;
			}
			else
			{
				psEffect->frameNumber = 0;
			}
		}
	}

	if(!gamePaused())
	{
		/* Tesla explosions are the only ones that rise, or indeed move */
		if(psEffect->type == EXPLOSION_TYPE_TESLA)
		{
			psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
		}
	}
}


/** The update function for blood */
static void updateBlood(EFFECT *psEffect)
{
	/* Time to update the frame number on the blood */
	if(graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		/* Are we on the last frame? */
		if(++psEffect->frameNumber >= effectGetNumFrames(psEffect))
		{
			/* Kill it off */
			killEffect(psEffect);
			return;
		}
	}
	/* Move it about in the world */
	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);
}

/** Processes all the drifting smoke
	Handles the smoke puffing out the factory as well */
static void updatePolySmoke(EFFECT *psEffect)
{

	/* Time to update the frame number on the smoke sprite */
	while (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		/* Store away last frame change time */
		psEffect->lastFrame += psEffect->frameDelay;

		/* Are we on the last frame? */
		if(++psEffect->frameNumber >= effectGetNumFrames(psEffect))
		{
			/* Does the anim wrap around? */
			if(TEST_CYCLIC(psEffect))
			{
				/* Does it change drift direction? */
				if(psEffect->type == SMOKE_TYPE_DRIFTING)
				{
					/* Make it change direction */
					psEffect->velocity.x = (float)(rand() % 20);
					psEffect->velocity.z = (float)(10 - rand() % 20);
					psEffect->velocity.y = (float)(10 + rand() % 20);
				}
				/* Reset the frame */
				psEffect->frameNumber = 0;
			}
			else
			{
				/* Kill it off */
 				killEffect(psEffect);
				return;
			}
		}
	}

	/* Update position */
	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);

	/* If it doesn't get killed by frame number, then by age */
	if(TEST_CYCLIC(psEffect))
	{
		/* Has it overstayed it's welcome? */
		if(graphicsTime - psEffect->birthTime > psEffect->lifeSpan)
		{
			/* Kill it */
			killEffect(psEffect);
		}
	}
}

/**
	Gravitons just fly up for a bit and then drop down and are
	killed off when they hit the ground
*/
static void updateGraviton(EFFECT *psEffect)
{
	float	accel;
	Vector3i dv;
	UDWORD	groundHeight;
	MAPTILE	*psTile;
	LIGHT	light;

	if(psEffect->type!=GRAVITON_TYPE_GIBLET)
	{
		light.position.x = psEffect->position.x;
		light.position.y = psEffect->position.y;
		light.position.z = psEffect->position.z;
		light.range = 128;
		light.colour = LIGHT_YELLOW;
		processLight(&light);
	}

	if(gamePaused())
	{
		/* Only update the lights if it's paused */
		return;
	}
	/* Move it about in the world */

	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);

	/* If it's bounced/drifted off the map then kill it */
	if (map_coord(psEffect->position.x) >= mapWidth
	 || map_coord(psEffect->position.z) >= mapHeight)
	{
		killEffect(psEffect);
		return;
	}

	groundHeight = map_Height(psEffect->position.x, psEffect->position.z);

	/* If it's going up and it's still under the landscape, then remove it... */
	if (psEffect->position.y<groundHeight
	 && psEffect->velocity.y > 0)
	{
		killEffect(psEffect);
		return;
	}

	/* Does it emit a trail? And is it high enough? */
	if ((psEffect->type == GRAVITON_TYPE_EMITTING_DR)
	    || ((psEffect->type == GRAVITON_TYPE_EMITTING_ST)
		&& (psEffect->position.y>(groundHeight+10))))
	{
		/* Time to add another trail 'thing'? */
		if(graphicsTime > psEffect->lastFrame + psEffect->frameDelay)
		{
			/* Store away last update */
			psEffect->lastFrame = graphicsTime;

			/* Add an effect at the gravitons's position */
			dv.x = psEffect->position.x;
			dv.y = psEffect->position.y;
			dv.z = psEffect->position.z;

			/* Add a trail graphic */
			addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,false,NULL,0);
		}
	}

	else if(psEffect->type == GRAVITON_TYPE_GIBLET && (psEffect->position.y>(groundHeight+5)))
	{
		/* Time to add another trail 'thing'? */
		if(graphicsTime > psEffect->lastFrame + psEffect->frameDelay)
		{
			/* Store away last update */
			psEffect->lastFrame = graphicsTime;

			/* Add an effect at the gravitons's position */
			dv.x = psEffect->position.x;
			dv.y = psEffect->position.y;
			dv.z = psEffect->position.z;
			addEffect(&dv,EFFECT_BLOOD,BLOOD_TYPE_NORMAL,false,NULL,0);
		}
	}

	/* Spin it round a bit */
	psEffect->rotation.x += graphicsTimeAdjustedIncrement(psEffect->spin.x);
	psEffect->rotation.y += graphicsTimeAdjustedIncrement(psEffect->spin.y);
	psEffect->rotation.z += graphicsTimeAdjustedIncrement(psEffect->spin.z);

	/* Update velocity (and retarding of descent) according to present frame rate */
	accel = graphicsTimeAdjustedIncrement(GRAVITON_GRAVITY);
	psEffect->velocity.y += accel;

	/* If it's bounced/drifted off the map then kill it */
	if ((int)psEffect->position.x <= TILE_UNITS
	 || (int)psEffect->position.z <= TILE_UNITS)
	{
		killEffect(psEffect);
		return;
	}

	/* Are we below it? - Hit the ground? */
	if ((int)psEffect->position.y < (int)groundHeight)
	{
		psTile = mapTile(map_coord(psEffect->position.x), map_coord(psEffect->position.z));
	   	if (terrainType(psTile) == TER_WATER)
		{
			killEffect(psEffect);
			return;
		}
		else
		/* Are we falling - rather than rising? */
   		if((int)psEffect->velocity.y < 0)
		{
			/* Has it sufficient energy to keep bouncing? */
			if (abs(psEffect->velocity.y) > 16
			 && psEffect->specific <= 2)
			{
				psEffect->specific++;
				/* Half it's velocity */

				psEffect->velocity.y/=(float)(-2); // only y gets flipped

				/* Set it at ground level - may have gone through */
				psEffect->position.y = (float)groundHeight;
			}
			else
			{
  				/* Giblets don't blow up when they hit the ground! */
				if(psEffect->type!=GRAVITON_TYPE_GIBLET)
				{
					/* Remove the graviton and add an explosion */
					dv.x = psEffect->position.x;
					dv.y = psEffect->position.y + 10;
					dv.z = psEffect->position.z;
					addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_VERY_SMALL,false,NULL,0);
				}
				killEffect(psEffect);
				return;
			}
		}
	}
}


/** This isn't really an on-screen effect itself - it just spawns other ones.... */
static void updateDestruction(EFFECT *psEffect)
{
	Vector3i pos;
	UDWORD	effectType;
	UDWORD	widthScatter = 0, breadthScatter = 0, heightScatter = 0;
	SDWORD	iX, iY;
	LIGHT	light;
	int     percent;
	UDWORD	range;
	float	div;
	UDWORD	height;

	percent = PERCENT(graphicsTime - psEffect->birthTime, psEffect->lifeSpan);
	if(percent > 100)
	{
		percent = 100;
	}
	range = 50 - abs(50-percent);

	light.position.x = psEffect->position.x;
	light.position.y = psEffect->position.y;
	light.position.z = psEffect->position.z;
	if(psEffect->type == DESTRUCTION_TYPE_STRUCTURE)
	{
		light.range = range*10;
	}
	else
	{
		light.range = range*4;
	}
	if(psEffect->type == DESTRUCTION_TYPE_POWER_STATION)
	{
		light.range *=3;
		light.colour = LIGHT_WHITE;
	}
	else
	{
		light.colour = LIGHT_RED;
	}
	processLight(&light);

	if(graphicsTime > psEffect->birthTime + psEffect->lifeSpan)
	{
		/* Kill it - it's too old */
		killEffect(psEffect);
		return;
	}

	if(psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
	{

		if(graphicsTime - psEffect->birthTime > psEffect->lifeSpan * 9/10)
		{
			pos.x = psEffect->position.x;
			pos.z = psEffect->position.z;
			pos.y = psEffect->position.y;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LARGE,false,NULL,0);
			killEffect(psEffect);
			return;
		}

		div = 1.f - (float)(graphicsTime - psEffect->birthTime) / psEffect->lifeSpan;
		if(div < 0.f)
			div = 0.f;
		height = div * psEffect->imd->max.y;
	}
	else
	{
		height = 16;
	}


	/* Time to add another effect? */
	if((graphicsTime - psEffect->lastFrame) > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		switch(psEffect->type)
		{
		case DESTRUCTION_TYPE_SKYSCRAPER:
			widthScatter = TILE_UNITS;
			breadthScatter = TILE_UNITS;
			heightScatter = TILE_UNITS;
			break;

		case DESTRUCTION_TYPE_POWER_STATION:
		case DESTRUCTION_TYPE_STRUCTURE:
			widthScatter = TILE_UNITS/2;
			breadthScatter = TILE_UNITS/2;
			heightScatter = TILE_UNITS/4;
			break;

		case DESTRUCTION_TYPE_DROID:
		case DESTRUCTION_TYPE_WALL_SECTION:
		case DESTRUCTION_TYPE_FEATURE:
			widthScatter = TILE_UNITS/6;
			breadthScatter = TILE_UNITS/6;
			heightScatter = TILE_UNITS/6;
			break;
		default:
			ASSERT( false,"Weirdy destruction type effect" );
			break;
		}


		/* Find a position to dump it at */
		pos.x = psEffect->position.x + widthScatter - rand() % (2 * widthScatter);
		pos.z = psEffect->position.z + breadthScatter - rand() % (2 * breadthScatter);
		pos.y = psEffect->position.y + height + rand() % heightScatter;

		if(psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
		{
	  		pos.y = psEffect->position.y + height;
  		}


		/* Choose an effect */
		effectType = rand()%15;
		switch(effectType)
		{
		case 0:
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,false,NULL,0);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			if(psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
			{
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LARGE,false,NULL,0);
			}
			/* Only structures get the big explosions */
			else if(psEffect->type==DESTRUCTION_TYPE_STRUCTURE)
			{
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,false,NULL,0);
			}
			else
			{
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,false,NULL,0);
			}
			break;
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			if(psEffect->type == DESTRUCTION_TYPE_STRUCTURE)
			{
				addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_ST,true,getRandomDebrisImd(),0);
			}
			else
			{
				addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,true,getRandomDebrisImd(),0);
			}
			break;
		case 11:
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,false,NULL,0);
			break;
		case 12:
		case 13:
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,false,NULL,0);
			break;
		case 14:
			/* Add sound effect, but only if we're less than 3/4 of the way thru' destruction */
			if (graphicsTime < (psEffect->birthTime + psEffect->lifeSpan) * 3/4)
			{
				iX = psEffect->position.x;
				iY = psEffect->position.z;
				audio_PlayStaticTrack( iX, iY, ID_SOUND_EXPLOSION );
			}
			break;

		}
	}
}

/** Moves the construction graphic about - dust cloud or whatever.... */
static void updateConstruction(EFFECT *psEffect)
{

	/* Time to update the frame number on the construction sprite */
	if(graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		/* Are we on the last frame? */
		if(++psEffect->frameNumber >= effectGetNumFrames(psEffect))
		{
			/* Is it a cyclic sprite? */
			if(TEST_CYCLIC(psEffect))
			{
				psEffect->frameNumber = 0;
			}
			else
			{
				killEffect(psEffect);
				return;
			}
		}
	}

	/* Move it about in the world */

	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);


	/* If it doesn't get killed by frame number, then by height */
	if(TEST_CYCLIC(psEffect))
	{
		/* Has it hit the ground */
		if ((int)psEffect->position.y <=
			map_Height(psEffect->position.x, psEffect->position.z))
		{
			killEffect(psEffect);
			return;
		}

		if (graphicsTime - psEffect->birthTime > psEffect->lifeSpan)
		{
			killEffect(psEffect);
			return;
		}
	}
}

/** Update fire sequences */
static void updateFire(EFFECT *psEffect)
{
	Vector3i pos;
	LIGHT	light;
	UDWORD	percent;

	percent = PERCENT(graphicsTime - psEffect->birthTime, psEffect->lifeSpan);
	if(percent > 100)
	{
		percent = 100;
	}

	light.position.x = psEffect->position.x;
	light.position.y = psEffect->position.y;
	light.position.z = psEffect->position.z;
	light.range = (percent*psEffect->radius*3)/100;
	light.colour = LIGHT_RED;
	processLight(&light);

	/* Time to update the frame number on the construction sprite */
	if (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		pos.x = (psEffect->position.x + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
		pos.z = (psEffect->position.z + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));

		// Effect is off map, no need to update it anymore
		if(!worldOnMap(pos.x, pos.z))
		{
			killEffect(psEffect);
			return;
		}

		pos.y = map_Height(pos.x,pos.z);

		if(psEffect->type == FIRE_TYPE_SMOKY_BLUE)
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,false,NULL,0);
		}
		else
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,false,NULL,0);
		}

		if(psEffect->type == FIRE_TYPE_SMOKY || psEffect->type == FIRE_TYPE_SMOKY_BLUE)
		{
			pos.x = (psEffect->position.x + ((rand() % psEffect->radius / 2) - (rand() % (2 * psEffect->radius / 2))));
			pos.z = (psEffect->position.z + ((rand() % psEffect->radius / 2) - (rand() % (2 * psEffect->radius / 2))));
			pos.y = map_Height(pos.x,pos.z);
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING_HIGH,false,NULL,0);
		}
		else
		{
			pos.x = (psEffect->position.x + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
			pos.z = (psEffect->position.z + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));

			// Effect is off map, no need to update it anymore
			if(!worldOnMap(pos.x, pos.z))
			{
				killEffect(psEffect);
				return;
			}

			pos.y = map_Height(pos.x,pos.z);
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,false,NULL,0);
		}

		/*
		pos.x = psEffect->position.x;
		pos.y = psEffect->position.y;
		pos.z = psEffect->position.z;

		scatter.x = psEffect->radius; scatter.y = 0; scatter.z = psEffect->radius;
		addMultiEffect(&pos,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,false,NULL,2,0,0);
		*/

	}

	if (graphicsTime - psEffect->birthTime > psEffect->lifeSpan)
	{
		killEffect(psEffect);
		return;
	}
}

// ----------------------------------------------------------------------------------------
// ALL THE RENDER FUNCTIONS
// ----------------------------------------------------------------------------------------
/** Calls the appropriate render routine for each type of effect */
void renderEffect(const EFFECT *psEffect)
{
		/* What type of effect are we dealing with? */
	switch(psEffect->group)
	{
	case EFFECT_WAYPOINT:
		renderWaypointEffect(psEffect);
		return;

	case EFFECT_EXPLOSION:
		renderExplosionEffect(psEffect);
		return;

	case EFFECT_CONSTRUCTION:
		renderConstructionEffect(psEffect);
		return;

	case EFFECT_SMOKE:
		renderSmokeEffect(psEffect);
		return;

	case EFFECT_GRAVITON:
		renderGravitonEffect(psEffect);
		return;

	case EFFECT_BLOOD:
		renderBloodEffect(psEffect);
		return;

	case EFFECT_DESTRUCTION:
		/*	There is no display func for a destruction effect -
			it merely spawn other effects over time */
		renderDestructionEffect(psEffect);
		return;

	case EFFECT_FIRE:
		/* Likewise */
		return;

	case EFFECT_SAT_LASER:
		/* Likewise */
		return;

	case EFFECT_FIREWORK:
		renderFirework(psEffect);
		return;

	case EFFECT_FREED:
		break;
	}

	debug( LOG_ERROR, "Weirdy class of effect passed to renderEffect" );
	abort();
}

/** drawing func for wapypoints */
static void renderWaypointEffect(const EFFECT *psEffect)
{
	positionEffect(psEffect);

	pie_Draw3DShape(psEffect->imd, 0, 0, WZCOL_WHITE, 0, 0);
	pie_MatEnd();
}

static void renderFirework(const EFFECT *psEffect)
{
	/* these don't get rendered */
	if(psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
		return;
	}

	positionEffect(psEffect);

	pie_MatRotY(-player.r.y);
	pie_MatRotX(-player.r.x);

	pie_MatScale(psEffect->size / 100.f);
 	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, WZCOL_WHITE, pie_ADDITIVE, EFFECT_EXPLOSION_ADDITIVE);
	pie_MatEnd();
}

/** drawing func for blood. */
static void renderBloodEffect(const EFFECT *psEffect)
{
	positionEffect(psEffect);

	pie_MatRotY(-player.r.y);
	pie_MatRotX(-player.r.x);
	pie_MatScale(psEffect->size / 100.f);

	pie_Draw3DShape(getImdFromIndex(MI_BLOOD), psEffect->frameNumber, 0, WZCOL_WHITE, pie_TRANSLUCENT, EFFECT_BLOOD_TRANSPARENCY);
	pie_MatEnd();
}

static void renderDestructionEffect(const EFFECT *psEffect)
{
	float	div;
	SDWORD	percent;

	if(psEffect->type!=DESTRUCTION_TYPE_SKYSCRAPER)
	{
		return;
	}

	positionEffect(psEffect);

	div = (float)(graphicsTime - psEffect->birthTime) / psEffect->lifeSpan;
	if(div>1.0)	div = 1.0;	//temporary!
	{
		div = 1.0 - div;
		percent = (SDWORD)(div*pie_RAISE_SCALE);
	}

	if(!gamePaused())
	{
		pie_MatRotX(SKY_SHIMMY);
		pie_MatRotY(SKY_SHIMMY);
		pie_MatRotZ(SKY_SHIMMY);
	}
 	pie_Draw3DShape(psEffect->imd, 0, 0, WZCOL_WHITE, pie_RAISE, percent);

	pie_MatEnd();
}

static bool rejectLandLight(LAND_LIGHT_SPEC type)
{
	unsigned int timeSlice = graphicsTime%2000;

	if (timeSlice < 400)
	{
		return (type != LL_MIDDLE); // reject all expect middle
	}
	else if (timeSlice < 800)
	{
		return (type == LL_OUTER); // reject only outer
	}
	else if (timeSlice < 1200)
	{
		return(false); //reject none
	}
	else if (timeSlice < 1600)
	{
		return (type == LL_OUTER); // reject only outer
	}
	else
	{
		return (type != LL_MIDDLE); // reject all expect middle
	}
}

/** Renders the standard explosion effect */
static void renderExplosionEffect(const EFFECT *psEffect)
{
	const PIELIGHT brightness = WZCOL_WHITE;

	if(psEffect->type == EXPLOSION_TYPE_LAND_LIGHT)
	{
		if(rejectLandLight((LAND_LIGHT_SPEC)psEffect->specific))
		{
			return;
		}
	}

	positionEffect(psEffect);

	/* Bit in comments - doesn't quite work yet? */
	if(TEST_FACING(psEffect))
	{
		/* Always face the viewer! */
		// TODO This only faces towards the viewer, if the effect is in the middle of the screen... It draws the effect parallel with the screens near/far planes.
		pie_MatRotY(-player.r.y);
		pie_MatRotX(-player.r.x);
	}

	/* Tesla explosions diminish in size */
	if(psEffect->type == EXPLOSION_TYPE_TESLA)
	{
		float scale = (graphicsTime - psEffect->birthTime) / (float)psEffect->lifeSpan;
		if      (scale < 0.f)
			scale = 0.f;
		else if (scale > .45f)
			scale = .45f;
		pie_MatScale(psEffect->size / 100.f - scale);
	}
	else if(psEffect->type == EXPLOSION_TYPE_PLASMA)
	{
		float scale = (graphicsTime - psEffect->birthTime) / (float)psEffect->lifeSpan / 3.f;
		pie_MatScale(BASE_PLASMA_SIZE / 100.f + scale);
	}
	else
	{
		pie_MatScale(psEffect->size / 100.f);
	}

	if(psEffect->type == EXPLOSION_TYPE_PLASMA)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_ADDITIVE, EFFECT_PLASMA_ADDITIVE);
	}
	else if(psEffect->type == EXPLOSION_TYPE_KICKUP)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT, 128);
	}
	else
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_ADDITIVE, EFFECT_EXPLOSION_ADDITIVE);
	}

	pie_MatEnd();
}

static void renderGravitonEffect(const EFFECT *psEffect)
{

	positionEffect(psEffect);

	pie_MatRotX(psEffect->rotation.x);
	pie_MatRotY(psEffect->rotation.y);
	pie_MatRotZ(psEffect->rotation.z);

	/* Buildings emitted by gravitons are chunkier */
	if(psEffect->type == GRAVITON_TYPE_EMITTING_ST)
	{
		/* Twice as big - 150 percent */
		pie_MatScale(psEffect->size / 100.f);
	}

	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, psEffect->player, WZCOL_WHITE, 0, 0);

	/* Pop the matrix */
	pie_MatEnd();
}

/** Renders the standard construction effect */
static void renderConstructionEffect(const EFFECT *psEffect)
{
	Vector3i null;
	int percent, translucency;
	float size;

	/* No rotation about arbitrary axis */
	null.x = null.y = null.z = 0;

	positionEffect(psEffect);

	/* Bit in comments doesn't quite work yet? */
	if(TEST_FACING(psEffect))
	{
		pie_MatRotY(-player.r.y);
		pie_MatRotX(-player.r.x);
	}

	/* Scale size according to age */
	percent = PERCENT(graphicsTime - psEffect->birthTime, psEffect->lifeSpan);
	if(percent<0) percent = 0;
	if(percent>100) percent = 100;

	/* Make imds be transparent on 3dfx */
	if(percent<50)
	{
		translucency = percent * 2;
	}
	else
	{
		translucency = (100 - percent) * 2;
	}
	translucency+=10;
	size = MIN(2.f * translucency / 100.f, .90f);
	pie_MatScale(size);

	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, WZCOL_WHITE, pie_TRANSLUCENT, translucency);

	/* Pop the matrix */
	pie_MatEnd();
}

/** Renders the standard smoke effect - it is now scaled in real-time as well */
static void renderSmokeEffect(const EFFECT *psEffect)
{
	int transparency = 0;
	const PIELIGHT brightness = WZCOL_WHITE;

	positionEffect(psEffect);

	/* Bit in comments doesn't quite work yet? */
	if(TEST_FACING(psEffect))
	{
 		/* Always face the viewer! */
		pie_MatRotY(-player.r.y);
		pie_MatRotX(-player.r.x);
	}

	if(TEST_SCALED(psEffect))
	{
		const unsigned int lifetime = graphicsTime - psEffect->birthTime;
		unsigned int percent;

		// Since psEffect->birthTime will be set to graphicsTime on
		// creation, and graphicsTime only increments, birthTime should be
		// smaller than or equal to graphicsTime. As a great man once said
		// though (hehe, I just love exaggarating ;-) -- Giel):
		// "Always assert your assumptions!". So here it goes:
		assert(psEffect->birthTime <= graphicsTime);

		ASSERT(psEffect->lifeSpan != 0, "Effect lifespan is zero (seconds); it really should be non-zero!");

		// HACK: Work around a bug that bit me causing a "integer divide by zero" at this location -- Giel
		if (psEffect->lifeSpan != 0)
			percent = (lifetime * 100) / psEffect->lifeSpan;
		else
			percent = 100;


		pie_MatScale((percent + psEffect->baseScale) / 100.f);
		transparency = (EFFECT_SMOKE_TRANSPARENCY * (100 - percent))/100;
	}

	transparency = (transparency * 3) / 2;  //JPS smoke strength increased for d3d 12 may 99

	/* Make imds be transparent on 3dfx */
	if(psEffect->type==SMOKE_TYPE_STEAM)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT, EFFECT_STEAM_TRANSPARENCY / 2);
	}
	else
	{
		if(psEffect->type == SMOKE_TYPE_TRAIL)
		{
			pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT, (2 * transparency) / 3);
		}
		else
		{
			pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT, transparency / 2);
		}
	}

	/* Pop the matrix */
	pie_MatEnd();
}


// ----------------------------------------------------------------------------------------
// ALL THE SETUP FUNCTIONS
// ----------------------------------------------------------------------------------------
void	effectSetupFirework(EFFECT *psEffect)
{
	if(psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
	 	psEffect->velocity.x = 200 - rand()%400;
		psEffect->velocity.z = 200 - rand()%400;
		psEffect->velocity.y = 400 + rand()%200;	//height
		psEffect->lifeSpan = GAME_TICKS_PER_SEC * 3;
		psEffect->radius = 80 + rand()%150;
		psEffect->size = 300+rand()%300;	//height it goes off
		psEffect->imd = getImdFromIndex(MI_FIREWORK); // not actually drawn
	}
	else
	{
		psEffect->velocity.x = 20 - rand()%40;
		psEffect->velocity.z = 20 - rand()%40;
		psEffect->velocity.y = 0-(20+rand()%40);	//height
		psEffect->lifeSpan = GAME_TICKS_PER_SEC * 4;

		/* setup the imds */
		switch(rand()%3)
		{
		case 0:
			psEffect->imd = getImdFromIndex(MI_FIREWORK);
			psEffect->size = 45;	//size of graphic
			break;
		case 1:
			psEffect->imd = getImdFromIndex(MI_SNOW);
			SET_CYCLIC(psEffect);
			psEffect->size = 60;	//size of graphic

			break;
		default:
			psEffect->imd = getImdFromIndex(MI_FLAME);
			psEffect->size = 40;	//size of graphic


			break;
		}
	}
	psEffect->frameDelay = (EXPLOSION_FRAME_DELAY*2);
}

void	effectSetupSmoke(EFFECT *psEffect)
{
	/* everything except steam drifts about */
	if(psEffect->type==SMOKE_TYPE_STEAM)
	{
   		/* Only upwards */
		psEffect->velocity.x = 0.f;
   		psEffect->velocity.z = 0.f;
	}
	else if (psEffect->type == SMOKE_TYPE_BILLOW)
	{

		psEffect->velocity.x = (float)(10 - rand() % 20);
   		psEffect->velocity.z = (float)(10 - rand() % 20);
	}
	else
	{
   	  	psEffect->velocity.x = (float)(rand() % 20);
   		psEffect->velocity.z = (float)(10 - rand() % 20);
	}

	/* Steam isn't cyclic  - it doesn't grow with time either */
	if(psEffect->type!=SMOKE_TYPE_STEAM)
	{
		SET_CYCLIC(psEffect);
		SET_SCALED(psEffect);
	}

	switch(psEffect->type)
  	{
  	case SMOKE_TYPE_DRIFTING:
  		psEffect->imd = getImdFromIndex(MI_SMALL_SMOKE);
		psEffect->lifeSpan = (UWORD)NORMAL_SMOKE_LIFESPAN;
   		psEffect->velocity.y = (float)(35 + rand() % 30);
		psEffect->baseScale = 40;
  		break;
  	case SMOKE_TYPE_DRIFTING_HIGH:
  		psEffect->imd = getImdFromIndex(MI_SMALL_SMOKE);
		psEffect->lifeSpan = (UWORD)NORMAL_SMOKE_LIFESPAN;
   		psEffect->velocity.y = (float)(40 + rand() % 45);
		psEffect->baseScale = 25;
  		break;
  	case SMOKE_TYPE_DRIFTING_SMALL:
  		psEffect->imd = getImdFromIndex(MI_SMALL_SMOKE);
		psEffect->lifeSpan = (UWORD)SMALL_SMOKE_LIFESPAN;
   		psEffect->velocity.y = (float)(25 + rand() % 35);
		psEffect->baseScale = 17;
  		break;
  	case SMOKE_TYPE_BILLOW:
  		psEffect->imd = getImdFromIndex(MI_SMALL_SMOKE);
		psEffect->lifeSpan = (UWORD)SMALL_SMOKE_LIFESPAN;
   		psEffect->velocity.y = (float)(10 + rand() % 20);
		psEffect->baseScale = 80;
  		break;
  	case SMOKE_TYPE_STEAM:
  		psEffect->imd = getImdFromIndex(MI_SMALL_STEAM);
   		psEffect->velocity.y = (float)(rand() % 5);
  		break;
	case SMOKE_TYPE_TRAIL:
		psEffect->imd = getImdFromIndex(MI_TRAIL);
		psEffect->lifeSpan = TRAIL_SMOKE_LIFESPAN;
   		psEffect->velocity.y = (float)(5 + rand() % 10);
		psEffect->baseScale = 25;
		break;
	default:
		ASSERT( false,"Weird smoke type" );
		break;
	}

	/* It always faces you */
	SET_FACING(psEffect);

	psEffect->frameDelay = (UWORD)SMOKE_FRAME_DELAY;
	/* Randomly flip gfx for variation */
	if(ONEINTWO)
	{
		SET_FLIPPED_X(psEffect);
	}
	if(ONEINTWO)
	{
		SET_FLIPPED_Y(psEffect);
	}
}

void effectSetupSatLaser(EFFECT *psEffect)
{
	/* Does nothing at all..... Runs only for one frame! */
	psEffect->baseScale = 1;
	return;
}

void	effectSetupGraviton(EFFECT *psEffect)
{
	switch(psEffect->type)
	{
	case GRAVITON_TYPE_GIBLET:
	   	psEffect->velocity.x = GIBLET_INIT_VEL_X;
		psEffect->velocity.z = GIBLET_INIT_VEL_Z;
		psEffect->velocity.y = GIBLET_INIT_VEL_Y;
		break;
	case GRAVITON_TYPE_EMITTING_ST:
		psEffect->velocity.x = GRAVITON_INIT_VEL_X;
		psEffect->velocity.z = GRAVITON_INIT_VEL_Z;
		psEffect->velocity.y = (5*GRAVITON_INIT_VEL_Y)/4;
		psEffect->size =(UWORD)( 120 + rand()%30);
		break;
	case GRAVITON_TYPE_EMITTING_DR:
		psEffect->velocity.x = GRAVITON_INIT_VEL_X/2;
		psEffect->velocity.z = GRAVITON_INIT_VEL_Z/2;
		psEffect->velocity.y = GRAVITON_INIT_VEL_Y;
		break;
	default:
		ASSERT( false,"Weirdy type of graviton" );
		break;

	}

	psEffect->rotation.x = rand()%DEG(360);
	psEffect->rotation.z = rand()%DEG(360);
	psEffect->rotation.y = rand()%DEG(360);

	psEffect->spin.x = rand()%DEG(100) + DEG(20);
	psEffect->spin.z = rand()%DEG(100) + DEG(20);
	psEffect->spin.y = rand()%DEG(100) + DEG(20);

	/* Gravitons are essential */
	SET_ESSENTIAL(psEffect);

	if(psEffect->type == GRAVITON_TYPE_GIBLET)
	{
		psEffect->frameDelay = (UWORD)GRAVITON_BLOOD_DELAY;
	}
	else
	{
		psEffect->frameDelay = (UWORD)GRAVITON_FRAME_DELAY;
	}
}

void effectSetupExplosion(EFFECT *psEffect)
{
	/* Get an imd if it's not established */
	if(psEffect->imd == NULL)
	{
		switch(psEffect->type)
		{
		case EXPLOSION_TYPE_SMALL:
  			psEffect->imd = getImdFromIndex(MI_EXPLOSION_SMALL);
  			psEffect->size = (UBYTE)((6*EXPLOSION_SIZE)/5);
  			break;
		case EXPLOSION_TYPE_VERY_SMALL:
  			psEffect->imd = getImdFromIndex(MI_EXPLOSION_SMALL);
  			psEffect->size = (UBYTE)(BASE_FLAME_SIZE + auxVar);
  			break;
		case EXPLOSION_TYPE_MEDIUM:
  			psEffect->imd = getImdFromIndex(MI_EXPLOSION_MEDIUM);
  			psEffect->size = (UBYTE)EXPLOSION_SIZE;
  			break;
		case EXPLOSION_TYPE_LARGE:
  			psEffect->imd = getImdFromIndex(MI_EXPLOSION_MEDIUM);
  			psEffect->size = (UBYTE)EXPLOSION_SIZE*2;
  			break;
		case EXPLOSION_TYPE_FLAMETHROWER:
			psEffect->imd = getImdFromIndex(MI_FLAME);
			psEffect->size = (UBYTE)(BASE_FLAME_SIZE + auxVar);
			break;
		case EXPLOSION_TYPE_LASER:
			psEffect->imd = getImdFromIndex(MI_FLAME);	// change this
			psEffect->size = (UBYTE)(BASE_LASER_SIZE + auxVar);
			break;
		case EXPLOSION_TYPE_DISCOVERY:
			psEffect->imd = getImdFromIndex(MI_TESLA);	// change this
			psEffect->size = DISCOVERY_SIZE;
			break;
		case EXPLOSION_TYPE_FLARE:
			psEffect->imd = getImdFromIndex(MI_MFLARE);
			psEffect->size = FLARE_SIZE;
			break;
		case EXPLOSION_TYPE_TESLA:
			psEffect->imd = getImdFromIndex(MI_TESLA);
			psEffect->size = TESLA_SIZE;
			psEffect->velocity.y = (float)TESLA_SPEED;
			break;

		case EXPLOSION_TYPE_KICKUP:
			psEffect->imd = getImdFromIndex(MI_KICK);
			psEffect->size = 100;
			break;
		case EXPLOSION_TYPE_PLASMA:
			psEffect->imd = getImdFromIndex(MI_PLASMA);
			psEffect->size = BASE_PLASMA_SIZE;
			psEffect->velocity.y = 0.0f;
			break;
		case EXPLOSION_TYPE_LAND_LIGHT:
			psEffect->imd = getImdFromIndex(MI_LANDING);
			psEffect->size = 120;
			psEffect->specific = ellSpec;
			psEffect->velocity.y = 0.0f;
			SET_ESSENTIAL(psEffect);		// Landing lights are permanent and cyclic
			break;
		case EXPLOSION_TYPE_SHOCKWAVE:
			psEffect->imd = getImdFromIndex(MI_SHOCK);//resGetData("IMD","blbhq.pie");
			psEffect->size = 50;
			psEffect->velocity.y = 0.0f;
			break;
		default:
  			break;
		}
	}

	if(psEffect->type == EXPLOSION_TYPE_FLAMETHROWER)
	{
		psEffect->frameDelay = 45;
	}
	/* Set how long it lasts */
	else if(psEffect->type == EXPLOSION_TYPE_LASER)
	{
		psEffect->frameDelay = EXPLOSION_FRAME_DELAY/2;
	}
	else
	if(psEffect->type==EXPLOSION_TYPE_TESLA)
	{
		psEffect->frameDelay = EXPLOSION_TESLA_FRAME_DELAY;
	}
	else

	if(psEffect->type==EXPLOSION_TYPE_PLASMA)
	{
		psEffect->frameDelay = EXPLOSION_PLASMA_FRAME_DELAY;
	}
	else
		if(psEffect->type == EXPLOSION_TYPE_LAND_LIGHT)
		{
			psEffect->frameDelay = 120;
		}
	else
	{
		psEffect->frameDelay = (UWORD)EXPLOSION_FRAME_DELAY;
	}


	if(psEffect->type == EXPLOSION_TYPE_SHOCKWAVE)
	{
		psEffect->lifeSpan = GAME_TICKS_PER_SEC;
	}
	else
	{
		psEffect->lifeSpan = (psEffect->frameDelay *  psEffect->imd->numFrames);
	}


	if ( (psEffect->type!=EXPLOSION_TYPE_NOT_FACING) && (psEffect->type!=EXPLOSION_TYPE_SHOCKWAVE))
	{
		SET_FACING(psEffect);
	}
	/* Randomly flip x and y for variation */
	if(ONEINTWO)
	{
		SET_FLIPPED_X(psEffect);
	}
	if(ONEINTWO)
	{
		SET_FLIPPED_Y(psEffect);
	}
}

void	effectSetupConstruction(EFFECT *psEffect)
{
	psEffect->velocity.x = 0.f;//(1-rand()%3);
	psEffect->velocity.z = 0.f;//(1-rand()%3);
	psEffect->velocity.y = (float)(0 - rand() % 3);
	psEffect->frameDelay = (UWORD)CONSTRUCTION_FRAME_DELAY;
	psEffect->imd = getImdFromIndex(MI_CONSTRUCTION);
	psEffect->lifeSpan = CONSTRUCTION_LIFESPAN;

	/* These effects always face you */
	SET_FACING(psEffect);

	/* It's a cyclic anim - dies on age */
	SET_CYCLIC(psEffect);

	/* Randomly flip the construction graphics in x and y for variation */
	if(ONEINTWO)
	{
		SET_FLIPPED_X(psEffect);
	}
	if(ONEINTWO)
	{
		SET_FLIPPED_Y(psEffect);
	}
}

void	effectSetupFire(EFFECT *psEffect)
{
	psEffect->frameDelay = 300;	   // needs to be investigated...
	psEffect->radius = auxVar;	// needs to be investigated
	psEffect->lifeSpan = (UWORD)auxVarSec;
	psEffect->birthTime = graphicsTime;
	SET_ESSENTIAL(psEffect);

}

void	effectSetupWayPoint(EFFECT *psEffect)
{
	psEffect->imd = pProximityMsgIMD;

	/* These effects musnt make way for others */
	SET_ESSENTIAL(psEffect);
}


static void effectSetupBlood(EFFECT *psEffect)
{
	psEffect->frameDelay = BLOOD_FRAME_DELAY;
	psEffect->velocity.y = (float)BLOOD_FALL_SPEED;
	psEffect->imd = getImdFromIndex(MI_BLOOD);
	psEffect->size = (UBYTE)BLOOD_SIZE;
}

static void effectSetupDestruction(EFFECT *psEffect)
{
	if(psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
	{
		psEffect->lifeSpan = (3*GAME_TICKS_PER_SEC)/2 + (rand()%GAME_TICKS_PER_SEC);
		psEffect->frameDelay = DESTRUCTION_FRAME_DELAY/2;
	}
	else if(psEffect->type == DESTRUCTION_TYPE_DROID)
	{
		/* It's all over quickly for droids */
		psEffect->lifeSpan = DROID_DESTRUCTION_DURATION;
		psEffect->frameDelay = DESTRUCTION_FRAME_DELAY;
	}
	else if(psEffect->type == DESTRUCTION_TYPE_WALL_SECTION ||
			psEffect->type == DESTRUCTION_TYPE_FEATURE)
	{
		psEffect->lifeSpan = STRUCTURE_DESTRUCTION_DURATION/4;
		psEffect->frameDelay = DESTRUCTION_FRAME_DELAY/2;
	}
	else if(psEffect->type == DESTRUCTION_TYPE_POWER_STATION)
	{
		psEffect->lifeSpan = STRUCTURE_DESTRUCTION_DURATION/2;
		psEffect->frameDelay = DESTRUCTION_FRAME_DELAY/4;
	}
	else
	{
		/* building's destruction is longer */
		psEffect->lifeSpan = STRUCTURE_DESTRUCTION_DURATION;
		psEffect->frameDelay = DESTRUCTION_FRAME_DELAY/2;
	}
}


#define SMOKE_SHIFT (16 - (rand()%32))
void initPerimeterSmoke(iIMDShape *pImd, Vector3i base)
{
	int i;
	int shift = SMOKE_SHIFT;
	int inStart = pImd->min.z - 16, inEnd = pImd->max.z + 16;
	int varStart = pImd->min.x - 16, varEnd = pImd->max.x + 16, varStride = 24;

	for (i = varStart; i < varEnd; i += varStride)
	{
		Vector3i pos = base + Vector3i(i + shift, 0, inStart + shift);

		if (rand()%6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, NULL, 0);
		}

		pos = base + Vector3i(i + shift, 0, inEnd + shift);

		if (rand()%6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, NULL, 0);
		}
	}

	varStart = pImd->min.z - 16;
	varEnd = pImd->max.z + 16;
	varStride = 24;

	inStart = pImd->min.x - 16;
	inEnd = pImd->max.x + 16;

	for (i = varStart; i < varEnd; i += varStride)
	{
		Vector3i pos = base + Vector3i(inStart + shift, 0, i + shift);

		if (rand()%6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, NULL, 0);
		}

		pos = base + Vector3i(inEnd + shift, 0, i + shift);

		if (rand()%6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, NULL, 0);
		}
	}
}


static UDWORD effectGetNumFrames(EFFECT *psEffect)
{
	if (psEffect->imd)
	{
		return psEffect->imd->numFrames;
	}
	else
	{
		return 0;
	}
}


void	effectGiveAuxVar( UDWORD var)
{
	auxVar = var;
}


void	effectGiveAuxVarSec( UDWORD var)
{
	auxVarSec = var;
}


/** Runs all the spot effect stuff for the droids - adding of dust and the like... */
static void effectDroidUpdates(void)
{
	unsigned int i;

	/* Go through all players */
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		DROID *psDroid;

		/* Now go through all their droids */
		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			/* Gets it's group number */
			unsigned int partition = psDroid->id % EFFECT_DROID_DIVISION;

			/* Right frame to process? */
			if (partition == frameGetFrameNumber() % EFFECT_DROID_DIVISION && ONEINFOUR)
			{
				/* Sufficent time since last update? - The EQUALS comparison is needed */
				if (graphicsTime >= lastUpdateDroids[partition] + DROID_UPDATE_INTERVAL)
				{
					/* Store away when we last processed this group */
					lastUpdateDroids[partition] = graphicsTime;

					/*	Now add some dust at it's arse end if it's moving or skidding.
						The check that it's not 0 is probably not sufficient.
					*/
					if ((int)psDroid->sMove.speed != 0)
					{
						/* Present direction is important */
						Vector2i behind = iSinCosR(psDroid->rot.direction, 50);
						Vector3i pos(
							clip(psDroid->pos.x - behind.x, 0, mapWidth),
							clip(psDroid->pos.y - behind.y, 0, mapHeight),
							0
						);

						pos.z = map_Height(pos.x, pos.y);

						// FIXME This does not do anything!!
					}
				}
			}
		}
	}
}


/** Runs all the structure effect stuff - steam puffing out etc */
static void effectStructureUpdates(void)
{
	unsigned int i;

	/* Go thru' all players */
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		STRUCTURE *psStructure;

		for (psStructure = apsStructLists[i]; psStructure; psStructure = psStructure->psNext)
		{
			/* Find it's group */
			unsigned int partition = psStructure->id % EFFECT_STRUCTURE_DIVISION;

			/* Is it the right frame? */
			if (partition == frameGetFrameNumber() % EFFECT_STRUCTURE_DIVISION)
			{
				/* Is it the right time? */
				if (graphicsTime > lastUpdateStructures[partition] + STRUCTURE_UPDATE_INTERVAL)
				{
					/* Store away the last update time */
					lastUpdateStructures[partition] = graphicsTime;

					/* Factories puff out smoke, power stations puff out tesla stuff */
					if ( (psStructure->pStructureType->type == REF_FACTORY
							|| psStructure->pStructureType->type == REF_POWER_GEN)
						&& psStructure->status == SS_BUILT
						&& psStructure->visible[selectedPlayer])
					{
						/*
							We're a factory, so better puff out a bit of steam
							Complete hack with the magic numbers - just for IAN demo
						*/
						if (psStructure->pStructureType->type == REF_FACTORY)
						{
							if (psStructure->sDisplay.imd->nconnectors == 1)
							{
								Vector3i eventPos = swapYZ(psStructure->pos) + Vector3i(
									psStructure->sDisplay.imd->connectors->x,
									psStructure->sDisplay.imd->connectors->z,
									-psStructure->sDisplay.imd->connectors->y
								);

								addEffect(&eventPos, EFFECT_SMOKE, SMOKE_TYPE_STEAM, false, NULL, 0);

								if (selectedPlayer == psStructure->player)
								{
									audio_PlayObjStaticTrack(psStructure, ID_SOUND_STEAM);
								}
							}
						}
						else if (psStructure->pStructureType->type == REF_POWER_GEN)
						{
							POWER_GEN *psPowerGen = &psStructure->pFunctionality->powerGenerator;
							Vector3i eventPos = swapYZ(psStructure->pos);

							if (psStructure->sDisplay.imd->nconnectors > 0)
							{
								eventPos.y += psStructure->sDisplay.imd->connectors->z;
							}

							/* Add an effect over the central spire - if
							connected to Res Extractor and it is active*/
							for (i = 0; i < NUM_POWER_MODULES; i++)
							{
								if (psPowerGen->apResExtractors[i]
								 && psPowerGen->apResExtractors[i]->pFunctionality->resourceExtractor.active)
								{
									break;
								}
							}

							{
								eventPos.y = psStructure->pos.z + 48;

								addEffect(&eventPos, EFFECT_EXPLOSION, EXPLOSION_TYPE_TESLA, false, NULL, 0);

								if (selectedPlayer == psStructure->player)
								{
									audio_PlayObjStaticTrack(psStructure, ID_SOUND_POWER_SPARK);
								}
							}
						}
					}
				}
			}
		}
	}
}


void effectResetUpdates(void)
{
	memset(lastUpdateDroids, 0, sizeof(lastUpdateDroids));
	memset(lastUpdateStructures, 0, sizeof(lastUpdateStructures));
}


/** This will save out the effects data */
bool writeFXData(const char *fileName)
{
	EFFECT *it;
	int i = 0;

	WzConfig ini(fileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", fileName);
		return false;
	}
	for (it = activeList.first; it != NULL; it = it->next, i++)
	{
		ini.beginGroup("effect_" + QString::number(i));
		ini.setValue("control", it->control);
		ini.setValue("group", it->group);
		ini.setValue("type", it->type);
		ini.setValue("frameNumber", it->frameNumber);
		ini.setValue("size", it->size);
		ini.setValue("baseScale", it->baseScale);
		ini.setValue("specific", it->specific);
		ini.setVector3f("position", it->position);
		ini.setVector3f("velocity", it->velocity);
		ini.setVector3i("rotation", it->rotation);
		ini.setVector3i("spin", it->spin);
		ini.setValue("birthTime", it->birthTime);
		ini.setValue("lastFrame", it->lastFrame);
		ini.setValue("frameDelay", it->frameDelay);
		ini.setValue("lifeSpan", it->lifeSpan);
		ini.setValue("radius", it->radius);

		const char *imd_name = resGetNamefromData("IMD", it->imd);
		if (imd_name && *imd_name)
		{
			ini.setValue("imd_name", imd_name);
		}

		// Move on to reading the next effect
		ini.endGroup();
	}

	// Everything is just fine!
	return true;
}

/** This will read in the effects data */
bool readFXData(const char *fileName)
{
	// Clear out anything that's there already!
	initEffectsSystem();

	WzConfig ini(fileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", fileName);
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		EFFECT *curEffect = Effect_malloc();

		curEffect->control      = ini.value("control").toInt();
		curEffect->group        = (EFFECT_GROUP)ini.value("group").toInt();
		curEffect->type         = (EFFECT_TYPE)ini.value("type").toInt();
		curEffect->frameNumber  = ini.value("frameNumber").toInt();
		curEffect->size         = ini.value("size").toInt();
		curEffect->baseScale    = ini.value("baseScale").toInt();
		curEffect->specific     = ini.value("specific").toInt();
		curEffect->position     = ini.vector3f("position");
		curEffect->velocity     = ini.vector3f("velocity");
		curEffect->rotation     = ini.vector3i("rotation");
		curEffect->spin         = ini.vector3i("spin");
		curEffect->birthTime    = ini.value("birthTime").toInt();
		curEffect->lastFrame    = ini.value("lastFrame").toInt();
		curEffect->frameDelay   = ini.value("frameDelay").toInt();
		curEffect->lifeSpan     = ini.value("lifeSpan").toInt();
		curEffect->radius       = ini.value("radius").toInt();
		if (ini.contains("imd_name"))
		{
			QString imd_name = ini.value("imd_name").toString();
			if (!imd_name.isEmpty())
			{
				curEffect->imd = (iIMDShape*)resGetData("IMD", imd_name.toUtf8().constData());
			}
		}
		else
		{
			curEffect->imd = NULL;
		}

		// Move on to reading the next effect
		ini.endGroup();
	}

	/* Hopefully everything's just fine by now */
	return true;
}
