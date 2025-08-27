/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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
#include "lib/framework/file.h"
#include "lib/framework/paged_entity_container.h"

#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/geometry.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/imd.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "display3d.h"
#include "map.h"
#include "bucket3d.h"

#include "effects.h"

#include "miscimd.h"
#include "lighting.h"
#include "console.h"
#include "loop.h"

#include "multiplay.h"
#include "component.h"
#include "profiling.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

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

#define	SET_FLIPPED_X(x)		((x.control) = (UBYTE)(x.control | EFFECT_X_FLIP))
#define	SET_FLIPPED_Y(x)		((x.control) = (UBYTE)(x.control | EFFECT_Y_FLIP))
#define SET_ESSENTIAL(x)		((x.control) = (UBYTE)(x.control | EFFECT_ESSENTIAL))
#define SET_FACING(x)			((x.control) = (UBYTE)(x.control | EFFECT_FACING))
#define SET_CYCLIC(x)			((x.control) = (UBYTE)(x.control | EFFECT_CYCLIC))
#define SET_SCALED(x)			((x.control) = (UBYTE)(x.control | EFFECT_SCALED))
#define SET_LIT(x)				((x.control) = (UBYTE)(x.control | EFFECT_LIT))

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


#define	EFFECT_EXPLOSION_ADDITIVE		164
#define EFFECT_PLASMA_ADDITIVE			224
#define	EFFECT_SMOKE_TRANSPARENCY		130
#define	EFFECT_STEAM_TRANSPARENCY		128
#define	EFFECT_BLOOD_TRANSPARENCY		128

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

static PagedEntityContainer<EFFECT> gActiveEffects;

/* Tick counts for updates on a particular interval */
static	UDWORD	lastUpdateStructures[EFFECT_STRUCTURE_DIVISION];

static	UDWORD	auxVar; // dirty filthy hack - don't look for what this does.... //FIXME
static	UDWORD	auxVarSec; // dirty filthy hack - don't look for what this does.... //FIXME
static	UDWORD	specifiedSize;
static	UDWORD	ellSpec;
static	uint8_t	EffectForPlayer = 0;
// ----------------------------------------------------------------------------------------
/* PROTOTYPES */


static bool rejectLandLight(LAND_LIGHT_SPEC type);

// ----------------------------------------------------------------------------------------
// ---- Update functions - every group type of effect has one of these */
static bool updateWaypoint(EFFECT *psEffect);
static bool updateExplosion(EFFECT *psEffect, LightingData& lightData);
static bool updatePolySmoke(EFFECT *psEffect);
static bool updateGraviton(EFFECT *psEffect, LightingData& lightData);
static bool updateConstruction(EFFECT *psEffect);
static bool updateBlood(EFFECT *psEffect);
static bool updateDestruction(EFFECT *psEffect, LightingData& lightData);
static bool updateFire(EFFECT *psEffect, LightingData& lightData);
static bool updateSatLaser(EFFECT *psEffect, LightingData& lightData);
static bool updateFirework(EFFECT *psEffect);
static bool updateEffect(EFFECT *psEffect, LightingData& lightData);	// MASTER function

// ----------------------------------------------------------------------------------------
// ---- The render functions - every group type of effect has a distinct one
static void renderExplosionEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
static void renderSmokeEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
static void renderGravitonEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
static void renderConstructionEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
static void renderWaypointEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
static void renderBloodEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
static void renderDestructionEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
static void renderFirework(const EFFECT *psEffect, const glm::mat4 &viewMatrix);

static glm::mat4 positionEffect(const EFFECT *psEffect);
/* There is no render destruction effect! */

// ----------------------------------------------------------------------------------------
// ---- The set up functions - every type has one
static void effectSetupSmoke(EFFECT& effect);
static void effectSetupGraviton(EFFECT& effect);
static void effectSetupExplosion(EFFECT& effect);
static void effectSetupConstruction(EFFECT& effect);
static void effectSetupWayPoint(EFFECT& effect);
static void effectSetupBlood(EFFECT& effect);
static void effectSetupDestruction(EFFECT& effect);
static void effectSetupFire(EFFECT& effect);
static void effectSetupSatLaser(EFFECT& effect);
static void effectSetupFirework(EFFECT& effect);

static void effectStructureUpdates();

static UDWORD effectGetNumFrames(EFFECT *psEffect);

void shutdownEffectsSystem()
{
	gActiveEffects.clear();
}

/*!
 * Initialise effects system
 */
void initEffectsSystem()
{
	shutdownEffectsSystem();
}

static glm::mat4 positionEffect(const EFFECT *psEffect)
{
	/* Establish world position */
	glm::vec3 dv(
	    psEffect->position.x,
	    psEffect->position.y,
	    -(psEffect->position.z)
	);

	return glm::translate(dv);
}

void effectSetLandLightSpec(LAND_LIGHT_SPEC spec)
{
	ellSpec = spec;
}

void effectSetSize(UDWORD size)
{
	specifiedSize = size;
}

void addMultiEffect(const Vector3i *basePos, Vector3i *scatter, EFFECT_GROUP group,
                    EFFECT_TYPE type, bool specified, const iIMDShape *imd, unsigned int number, bool lit, unsigned int size, unsigned effectTime)
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
		for (i = 0; i < number; i++)
		{
			// This scatters in a cube - is there a good reason for that, or just legacy?
			Vector3i scatPos = *basePos + *scatter - Vector3i(rand() % (scatter->x * 2 + 1),
			                   rand() % (scatter->y * 2 + 1),
			                   rand() % (scatter->z * 2 + 1)
			                                                 );
			addEffect(&scatPos, group, type, specified, imd, lit, effectTime);
		}
	}
}

// When we need to set the effect for the player's color
void SetEffectForPlayer(uint8_t player)
{
	ASSERT(player < MAX_PLAYERS, "player is set to a invalid number of %d", (int) player);

	EffectForPlayer = getPlayerColour(player);
}

void addEffect(const Vector3i *pos, EFFECT_GROUP group, EFFECT_TYPE type, bool specified, const iIMDShape *imd, int lit)
{
	return addEffect(pos, group, type, specified, imd, lit, graphicsTime);
}

static bool updateDroidDeathAnimationEffect(EFFECT *psEffect)
{
	const iIMDShape *imd = psEffect->imd;
	if (!imd)
	{
		return false;
	}

	/* Move it */
	auto deltaX = graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	auto deltaY = graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.x += deltaX;
	psEffect->position.z += deltaY;

	// NOTE: We actually store the timeAnimationStarted (in gameTime) in psEffect->lastFrame
	UDWORD timeAnimationStarted = psEffect->lastFrame;
	int objanimcycles = (imd->objanimcycles > 0) ? imd->objanimcycles : 1; // do not allow infinite looping for death animations
	if (gameTime > timeAnimationStarted + imd->objanimtime * objanimcycles)
	{
		// Done animating (animation is defined by body - other components should follow suit)
		/* Kill off effect */
		return false;
	}
	return true;
}

static void renderDroidDeathAnimationEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	glm::mat4 modelMatrix = positionEffect(psEffect);
	modelMatrix *=
		glm::rotate(UNDEG(psEffect->rotation.x), glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(UNDEG(psEffect->rotation.y), glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(UNDEG(psEffect->rotation.z), glm::vec3(0.f, 0.f, 1.f));

	// If original (psDroid->droidType == DROID_PERSON)
	if (psEffect->type == DROID_ANIMEVENT_DYING_NORMAL_ST)
	{
		// previously, used to scale the model by 0.75 - no longer needed with the fixed models though...
	}

	PIELIGHT brightness = pal_SetBrightness(255); // NOTE: brightness is not preserved from the original droid that died - FIXME: ?
	SDWORD pieFlag = pie_SHADOW;
	SDWORD iPieData = 0;

	const iIMDShape *strImd = psEffect->imd;
	UDWORD timeAnimationStarted = psEffect->lastFrame;
	while (strImd)
	{
		drawShape(strImd, timeAnimationStarted, psEffect->player, brightness, pieFlag, iPieData, modelMatrix, viewMatrix);
		strImd = strImd->next.get();
	}
}

void addEffect(const Vector3i *pos, EFFECT_GROUP group, EFFECT_TYPE type, bool specified, const iIMDShape *imd, int lit, unsigned effectTime, Vector3i *rot /*= nullptr*/, Vector3f *velocity /*= nullptr*/)
{
	if (gamePaused())
	{
		return;
	}
	EFFECT effect;
	/* Reset control bits */
	effect.control = 0;

	/* Store away it's position - into FRACTS */
	effect.position.x = pos->x;
	effect.position.y = pos->y;
	effect.position.z = pos->z;

	if (rot)
	{
		effect.rotation.x = rot->x;
		effect.rotation.y = rot->y;
		effect.rotation.z = rot->z;
	}

	if (velocity)
	{
		effect.velocity = *velocity;
	}

	/* Now, note group and type */
	effect.group = group;
	effect.type = type;

	// and if the effect needs the player's color for certain things
	effect.player = EffectForPlayer;
	SetEffectForPlayer(0);	// reset it

	/* Set when it entered the world */
	effect.birthTime = effect.lastFrame = effectTime;

	if (group == EFFECT_GRAVITON && (type == GRAVITON_TYPE_GIBLET || type == GRAVITON_TYPE_EMITTING_DR))
	{
		effect.frameNumber = lit;
	}
	else
	{
		/* Starts off on frame zero */
		effect.frameNumber = 0;
	}

	/*
		See what kind of effect it is - the add function is different for each,
		although some things are shared
	*/
	effect.imd = nullptr;
	if (lit)
	{
		SET_LIT(effect);
	}

	if (specified)
	{
		/* We're specifying what the imd is - override */
		effect.imd = imd;
		effect.size = specifiedSize;
	}

	/* Do all the effect type specific stuff */
	switch (group)
	{
	case EFFECT_SMOKE:
		effectSetupSmoke(effect);
		break;
	case EFFECT_GRAVITON:
		effectSetupGraviton(effect);
		break;
	case EFFECT_EXPLOSION:
		effectSetupExplosion(effect);
		break;
	case EFFECT_CONSTRUCTION:
		effectSetupConstruction(effect);
		break;
	case EFFECT_WAYPOINT:
		effectSetupWayPoint(effect);
		break;
	case EFFECT_BLOOD:
		effectSetupBlood(effect);
		break;
	case EFFECT_DESTRUCTION:
		effectSetupDestruction(effect);
		break;
	case EFFECT_FIRE:
		effectSetupFire(effect);
		break;
	case EFFECT_SAT_LASER:
		effectSetupSatLaser(effect);
		break;
	case EFFECT_FIREWORK:
		effectSetupFirework(effect);
		break;
	case EFFECT_DROID_ANIMEVENT_DYING:
		effect.lastFrame = gameTime; // store the timeAnimationStarted in lastFrame
		break;
	case EFFECT_FREED:
		ASSERT(false, "Weirdy group type for an effect");
		break;
	}

	/* As of yet, it hasn't bounced (or whatever)... */
	if (type != EXPLOSION_TYPE_LAND_LIGHT)
	{
		effect.specific = 0;
	}

	ASSERT(effect.imd != nullptr || group == EFFECT_DESTRUCTION || group == EFFECT_FIRE || group == EFFECT_SAT_LASER, "null effect imd");

	gActiveEffects.emplace(std::move(effect));
}


/* Calls all the update functions for each different currently active effect */
void processEffects(const glm::mat4 &perspectiveViewMatrix, LightingData& lightData)
{
	WZ_PROFILE_SCOPE(processEffects);
	for (auto it = gActiveEffects.begin(); it != gActiveEffects.end(); ++it)
	{
		EFFECT& e = *it;

		if (e.birthTime <= graphicsTime)  // Don't process, if it doesn't exist yet
		{
			if (!updateEffect(&e, lightData))
			{
				gActiveEffects.erase(it);
				continue;
			}
			if (e.group != EFFECT_FREED && clipXY(static_cast<SDWORD>(e.position.x), static_cast<SDWORD>(e.position.z)))
			{
				bucketAddTypeToList(RENDER_EFFECT, &e, perspectiveViewMatrix);
			}
		}
	}

	/* Add any structure effects */
	effectStructureUpdates();
}

/* The general update function for all effects - calls a specific one for each. Returns false if effect should be deleted. */
static bool updateEffect(EFFECT *psEffect, LightingData& lightData)
{
	/* What type of effect are we dealing with? */
	switch (psEffect->group)
	{
	case EFFECT_EXPLOSION:
		return updateExplosion(psEffect, lightData);
	case EFFECT_WAYPOINT:
		if (!gamePaused())
		{
			return updateWaypoint(psEffect);
		}
		return true;
	case EFFECT_CONSTRUCTION:
		if (!gamePaused())
		{
			return updateConstruction(psEffect);
		}
		return true;
	case EFFECT_SMOKE:
		if (!gamePaused())
		{
			return updatePolySmoke(psEffect);
		}
		return true;
	case EFFECT_GRAVITON:
		if (!gamePaused())
		{
			return updateGraviton(psEffect, lightData);
		}
		return true;
	case EFFECT_BLOOD:
		if (!gamePaused())
		{
			return updateBlood(psEffect);
		}
		return true;
	case EFFECT_DESTRUCTION:
		if (!gamePaused())
		{
			return updateDestruction(psEffect, lightData);
		}
		return true;
	case EFFECT_FIRE:
		if (!gamePaused())
		{
			return updateFire(psEffect, lightData);
		}
		return true;
	case EFFECT_SAT_LASER:
		if (!gamePaused())
		{
			return updateSatLaser(psEffect, lightData);
		}
		return true;
	case EFFECT_FIREWORK:
		if (!gamePaused())
		{
			return updateFirework(psEffect);
		}
		return true;
	case EFFECT_DROID_ANIMEVENT_DYING:
		return updateDroidDeathAnimationEffect(psEffect);
	case EFFECT_FREED:
		break;
	}
	debug(LOG_ERROR, "Weirdy class of effect passed to updateEffect");
	abort();
}

// ----------------------------------------------------------------------------------------
// ALL THE UPDATE FUNCTIONS
// ----------------------------------------------------------------------------------------
/** Update the waypoint effects.*/
static bool updateWaypoint(EFFECT *psEffect)
{
	if (!(keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)))
	{
		return false;
	}
	return true;
}

static bool updateFirework(EFFECT *psEffect)
{
	UDWORD	height;
	UDWORD	xDif, yDif, radius, val;
	Vector3i dv;
	SDWORD	dif;
	UDWORD	drop;

	/* Move it */
	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);

	if (psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
		height = static_cast<UDWORD>(psEffect->position.y);
		if (height > psEffect->size)
		{
			dv.x = static_cast<int>(psEffect->position.x);
			dv.z = static_cast<int>(psEffect->position.z);
			dv.y = static_cast<int>(psEffect->position.y + (psEffect->radius / 2));
			addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0);
			audio_PlayStaticTrack(static_cast<SDWORD>(psEffect->position.x), static_cast<SDWORD>(psEffect->position.z), ID_SOUND_EXPLOSION);

			for (dif = 0; dif < (psEffect->radius * 2); dif += 20)
			{
				if (dif < psEffect->radius)
				{
					drop = psEffect->radius - dif;
				}
				else
				{
					drop = dif - psEffect->radius;
				}
				radius = (UDWORD)sqrtf(psEffect->radius * psEffect->radius - drop * drop);
				//val = getStaticTimeValueRange(720,360);	// grab an angle - 4 seconds cyclic
				for (val = 0; val <= 180; val += 20)
				{
					xDif = iSinR(DEG(val), radius);
					yDif = iCosR(DEG(val), radius);
					dv.x = static_cast<int>(psEffect->position.x + xDif);
					dv.z = static_cast<int>(psEffect->position.z + yDif);
					dv.y = static_cast<int>(psEffect->position.y + dif);
					effectGiveAuxVar(100);
					addEffect(&dv, EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST, false, nullptr, 0);
					dv.x = static_cast<int>(psEffect->position.x - xDif);
					dv.z = static_cast<int>(psEffect->position.z - yDif);
					dv.y = static_cast<int>(psEffect->position.y + dif);
					effectGiveAuxVar(100);
					addEffect(&dv, EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST, false, nullptr, 0);
					dv.x = static_cast<int>(psEffect->position.x + xDif);
					dv.z = static_cast<int>(psEffect->position.z - yDif);
					dv.y = static_cast<int>(psEffect->position.y + dif);
					effectGiveAuxVar(100);
					addEffect(&dv, EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST, false, nullptr, 0);
					dv.x = static_cast<int>(psEffect->position.x - xDif);
					dv.z = static_cast<int>(psEffect->position.z + yDif);
					dv.y = static_cast<int>(psEffect->position.y + dif);
					effectGiveAuxVar(100);
					addEffect(&dv, EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST, false, nullptr, 0);
				}
			}
			return false;
		}
		else
		{
			/* Add an effect at the firework's position */
			dv.x = static_cast<int>(psEffect->position.x);
			dv.y = static_cast<int>(psEffect->position.y);
			dv.z = static_cast<int>(psEffect->position.z);

			/* Add a trail graphic */
			addEffect(&dv, EFFECT_SMOKE, SMOKE_TYPE_TRAIL, false, nullptr, 0);
		}
	}
	else	// must be a startburst
	{
		/* Time to update the frame number on the smoke sprite */
		if (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
		{
			/* Store away last frame change time */
			psEffect->lastFrame = graphicsTime;

			/* Are we on the last frame? */
			if (++psEffect->frameNumber >= effectGetNumFrames(psEffect))
			{
				/* Does the anim wrap around? */
				if (TEST_CYCLIC(psEffect))
				{
					psEffect->frameNumber = 0;
				}
				else
				{
					return false; /* Kill it off */
				}
			}
		}

		/* If it doesn't get killed by frame number, then by age */
		if (TEST_CYCLIC(psEffect))
		{
			/* Has it overstayed it's welcome? */
			if (graphicsTime - psEffect->birthTime > psEffect->lifeSpan)
			{
				return false; /* Kill it */
			}
		}
	}
	return true;
}

static bool updateSatLaser(EFFECT *psEffect, LightingData& lightData)
{
	Vector3i dv;
	UDWORD	val;
	UDWORD	radius;
	UDWORD	xDif, yDif;
	UDWORD	i;
	UDWORD	startHeight, endHeight;
	UDWORD	xPos, yPos;

	// Do these here because they are used by the lighting code below this if
	xPos = static_cast<UDWORD>(psEffect->position.x);
	startHeight = static_cast<UDWORD>(psEffect->position.y);
	endHeight = startHeight + 1064;
	yPos = static_cast<UDWORD>(psEffect->position.z);

	if (psEffect->baseScale)
	{
		psEffect->baseScale = 0;

		/* Add some big explosions....! */

		for (i = 0; i < 16; i++)
		{
			dv.x = xPos + (200 - rand() % 400);
			dv.z = yPos + (200 - rand() % 400);
			dv.y = startHeight + rand() % 100;
			addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0);
		}
		/* Add a sound effect */
		audio_PlayStaticTrack(static_cast<SDWORD>(psEffect->position.x), static_cast<SDWORD>(psEffect->position.z), ID_SOUND_EXPLOSION);

		/* Add a shockwave */
		dv.x = xPos;
		dv.z = yPos;
		dv.y = startHeight + SHOCK_WAVE_HEIGHT;
		addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_SHOCKWAVE, false, nullptr, 0);

		/* Now, add the column of light */
		for (i = startHeight; i < endHeight; i += 56)
		{
			radius = 80;
			/* Add 36 around in a circle..! */
			for (val = 0; val <= 180; val += 30)
			{
				xDif = iSinR(DEG(val), radius);
				yDif = iCosR(DEG(val), radius);
				dv.x = xPos + xDif + i / 64;
				dv.z = yPos + yDif;
				dv.y = startHeight + i;
				effectGiveAuxVar(100);
				addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0);
				dv.x = xPos - xDif + i / 64;
				dv.z = yPos - yDif;
				dv.y = startHeight + i;
				effectGiveAuxVar(100);
				addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0);
				dv.x = xPos + xDif + i / 64;
				dv.z = yPos - yDif;
				dv.y = startHeight + i;
				effectGiveAuxVar(100);
				addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0);
				dv.x = xPos - xDif + i / 64;
				dv.z = yPos + yDif;
				dv.y = startHeight + i;
				effectGiveAuxVar(100);
				addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0);
			}
		}
	}

	if (graphicsTime - psEffect->birthTime < 1000)
	{
		LIGHT light;
		light.position = Vector3f(xPos, startHeight, yPos);
		light.range = 800;
		light.colour = pal_Colour(0, 0, 255);
		lightData.lights.push_back(light);
		return true;
	}
	else
	{
		return false;
	}
}

/** The update function for the explosions */
static bool updateExplosion(EFFECT *psEffect, LightingData& lightData)
{
	if (TEST_LIT(psEffect))
	{
		UDWORD percent;
		LIGHT light;

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

		if (psEffect->type != EXPLOSION_TYPE_LAND_LIGHT || !rejectLandLight(static_cast<LAND_LIGHT_SPEC>(psEffect->specific)))
		{
			UDWORD range = percent;
			light.position = psEffect->position;
			light.range = (3 * range) / 2;
			light.colour = pal_Colour(255, 0, 0);
			lightData.lights.push_back(light);
		}
	}

	if (psEffect->type == EXPLOSION_TYPE_SHOCKWAVE)
	{
		LIGHT light;
		const float scaling = (float)psEffect->size / (float)MAX_SHOCKWAVE_SIZE;

		psEffect->size += static_cast<uint16_t>(graphicsTimeAdjustedIncrement(SHOCKWAVE_SPEED));
		psEffect->frameNumber = static_cast<uint8_t>(scaling * effectGetNumFrames(psEffect));

		light.position = psEffect->position;
		light.range = psEffect->size + 200;
		light.colour = pal_Colour(255, 255, 0);
		lightData.lights.push_back(light);

		if (psEffect->size > MAX_SHOCKWAVE_SIZE || light.range > 600)
		{
			return false; /* Kill it off */
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
					return false; /* Kill it off */
				}
				else
				{
					psEffect->frameNumber = 0;
				}
			}
		}

	if (!gamePaused())
	{
		/* Tesla explosions are the only ones that rise, or indeed move */
		if (psEffect->type == EXPLOSION_TYPE_TESLA)
		{
			psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
		}
	}
	return true;
}

/** The update function for blood */
static bool updateBlood(EFFECT *psEffect)
{
	/* Time to update the frame number on the blood */
	if (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		/* Are we on the last frame? */
		if (++psEffect->frameNumber >= effectGetNumFrames(psEffect))
		{
			return false; /* Kill it off */
		}
	}
	/* Move it about in the world */
	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);
	return true;
}

/** Processes all the drifting smoke. Handles the smoke puffing out the factory as well. */
static bool updatePolySmoke(EFFECT *psEffect)
{
	/* Time to update the frame number on the smoke sprite */
	while (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		/* Store away last frame change time */
		psEffect->lastFrame += psEffect->frameDelay;

		/* Are we on the last frame? */
		if (++psEffect->frameNumber >= effectGetNumFrames(psEffect))
		{
			/* Does the anim wrap around? */
			if (TEST_CYCLIC(psEffect))
			{
				/* Does it change drift direction? */
				if (psEffect->type == SMOKE_TYPE_DRIFTING)
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
				return false;
			}
		}
	}

	/* Update position */
	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);

	/* If it doesn't get killed by frame number, then by age */
	if (TEST_CYCLIC(psEffect))
	{
		/* Has it overstayed it's welcome? */
		if (graphicsTime - psEffect->birthTime > psEffect->lifeSpan)
		{
			return false; /* Kill it */
		}
	}
	return true;
}

/**
	Gravitons just fly up for a bit and then drop down and are
	killed off when they hit the ground
*/
static bool updateGraviton(EFFECT *psEffect, LightingData& lightData)
{
	float	accel;
	Vector3i dv;
	MAPTILE	*psTile;
	LIGHT	light;

	if (psEffect->type != GRAVITON_TYPE_GIBLET)
	{
		light.position = psEffect->position;
		light.range = 128;
		light.colour = pal_Colour(255, 255, 0);
		lightData.lights.push_back(light);
	}

	if (gamePaused())
	{
		/* Only update the lights if it's paused */
		return true;
	}

	/* Move it about in the world */
	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);

	/* If it's bounced/drifted off the map then kill it */
	if (map_coord(static_cast<int32_t>(psEffect->position.x)) >= mapWidth || map_coord(static_cast<int32_t>(psEffect->position.z)) >= mapHeight)
	{
		return false;
	}

	int groundHeight = map_Height(static_cast<int>(psEffect->position.x), static_cast<int>(psEffect->position.z));

	/* If it's going up and it's still under the landscape, then remove it... */
	if (psEffect->position.y < groundHeight && psEffect->velocity.y > 0)
	{
		return false;
	}

	/* Does it emit a trail? And is it high enough? */
	if ((psEffect->type == GRAVITON_TYPE_EMITTING_DR)
	    || ((psEffect->type == GRAVITON_TYPE_EMITTING_ST)
	        && (psEffect->position.y > (groundHeight + 10))))
	{
		/* Time to add another trail 'thing'? */
		if (graphicsTime > psEffect->lastFrame + psEffect->frameDelay)
		{
			/* Store away last update */
			psEffect->lastFrame = graphicsTime;

			/* Add an effect at the gravitons's position */
			dv.x = static_cast<int>(psEffect->position.x);
			dv.y = static_cast<int>(psEffect->position.y);
			dv.z = static_cast<int>(psEffect->position.z);

			/* Add a trail graphic */
			addEffect(&dv, EFFECT_SMOKE, SMOKE_TYPE_TRAIL, false, nullptr, 0);
		}
	}
	else if (psEffect->type == GRAVITON_TYPE_GIBLET && (psEffect->position.y > (groundHeight + 5)))
	{
		/* Time to add another trail 'thing'? */
		if (graphicsTime > psEffect->lastFrame + psEffect->frameDelay)
		{
			/* Store away last update */
			psEffect->lastFrame = graphicsTime;

			/* Add an effect at the gravitons's position */
			dv.x = static_cast<int>(psEffect->position.x);
			dv.y = static_cast<int>(psEffect->position.y);
			dv.z = static_cast<int>(psEffect->position.z);
			addEffect(&dv, EFFECT_BLOOD, BLOOD_TYPE_NORMAL, false, nullptr, 0);
		}
	}

	/* Spin it round a bit */
	psEffect->rotation.x += static_cast<int>(graphicsTimeAdjustedIncrement(psEffect->spin.x));
	psEffect->rotation.y += static_cast<int>(graphicsTimeAdjustedIncrement(psEffect->spin.y));
	psEffect->rotation.z += static_cast<int>(graphicsTimeAdjustedIncrement(psEffect->spin.z));

	/* Update velocity (and retarding of descent) according to present frame rate */
	accel = graphicsTimeAdjustedIncrement(GRAVITON_GRAVITY);
	psEffect->velocity.y += accel;

	/* If it's bounced/drifted off the map then kill it */
	if ((int)psEffect->position.x <= TILE_UNITS || (int)psEffect->position.z <= TILE_UNITS)
	{
		return false;
	}

	/* Are we below it? - Hit the ground? */
	if ((int)psEffect->position.y < (int)groundHeight)
	{
		psTile = mapTile(map_coord(static_cast<int32_t>(psEffect->position.x)), map_coord(static_cast<int32_t>(psEffect->position.z)));
		if (terrainType(psTile) == TER_WATER)
		{
			return false;
		}
		else if ((int)psEffect->velocity.y < 0) // Are we falling - rather than rising?
		{
			/* Has it sufficient energy to keep bouncing? */
			if (abs(psEffect->velocity.y) > 16 && psEffect->specific <= 2)
			{
				psEffect->specific++;

				/* Half it's velocity */
				psEffect->velocity.y /= (float)(-2); // only y gets flipped

				/* Also decrease other velocities */
				psEffect->velocity.x *= 0.5f;
				psEffect->velocity.z *= 0.5f;

				/* Set it at ground level - may have gone through */
				psEffect->position.y = (float)groundHeight;
			}
			else
			{
				/* Giblets don't blow up when they hit the ground! */
				if (psEffect->type != GRAVITON_TYPE_GIBLET)
				{
					/* Remove the graviton and add an explosion */
					dv.x = static_cast<int>(psEffect->position.x);
					dv.y = static_cast<int>(psEffect->position.y + 10);
					dv.z = static_cast<int>(psEffect->position.z);
					addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_VERY_SMALL, false, nullptr, 0);
				}
				return false;
			}
		}
	}
	return true;
}


/** This isn't really an on-screen effect itself - it just spawns other ones.... */
static bool updateDestruction(EFFECT *psEffect, LightingData& lightData)
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
	if (percent > 100)
	{
		percent = 100;
	}
	range = 50 - abs(50 - percent);

	light.position = psEffect->position;
	if (psEffect->type == DESTRUCTION_TYPE_STRUCTURE)
	{
		light.range = range * 10;
	}
	else
	{
		light.range = range * 4;
	}
	if (psEffect->type == DESTRUCTION_TYPE_POWER_STATION)
	{
		light.range *= 3;
		light.colour = pal_Colour(255, 255, 255);
	}
	else
	{
		light.colour = pal_Colour(255, 0, 0);
	}
	lightData.lights.push_back(light);

	if (graphicsTime > psEffect->birthTime + psEffect->lifeSpan)
	{
		/* Kill it - it's too old */
		return false;
	}

	if (psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
	{

		if (graphicsTime - psEffect->birthTime > psEffect->lifeSpan * 9 / 10)
		{
			pos.x = static_cast<int>(psEffect->position.x);
			pos.z = static_cast<int>(psEffect->position.z);
			pos.y = static_cast<int>(psEffect->position.y);
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LARGE, false, nullptr, 0);
			return false;
		}

		div = 1.f - (float)(graphicsTime - psEffect->birthTime) / psEffect->lifeSpan;
		if (div < 0.f)
		{
			div = 0.f;
		}
		height = static_cast<UDWORD>(div * psEffect->imd->max.y);
	}
	else
	{
		height = 16;
	}

	/* Time to add another effect? */
	if ((graphicsTime - psEffect->lastFrame) > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		switch (psEffect->type)
		{
		case DESTRUCTION_TYPE_SKYSCRAPER:
			widthScatter = TILE_UNITS;
			breadthScatter = TILE_UNITS;
			heightScatter = TILE_UNITS;
			break;

		case DESTRUCTION_TYPE_POWER_STATION:
		case DESTRUCTION_TYPE_STRUCTURE:
			widthScatter = TILE_UNITS / 2;
			breadthScatter = TILE_UNITS / 2;
			heightScatter = TILE_UNITS / 4;
			break;

		case DESTRUCTION_TYPE_DROID:
		case DESTRUCTION_TYPE_WALL_SECTION:
		case DESTRUCTION_TYPE_FEATURE:
			widthScatter = TILE_UNITS / 6;
			breadthScatter = TILE_UNITS / 6;
			heightScatter = TILE_UNITS / 6;
			break;
		default:
			ASSERT(false, "Weirdy destruction type effect");
			break;
		}


		/* Find a position to dump it at */
		pos.x = static_cast<int>(psEffect->position.x + widthScatter - rand() % (2 * widthScatter));
		pos.z = static_cast<int>(psEffect->position.z + breadthScatter - rand() % (2 * breadthScatter));
		pos.y = static_cast<int>(psEffect->position.y + height + rand() % heightScatter);

		if (psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
		{
			pos.y = static_cast<int>(psEffect->position.y + height);
		}


		/* Choose an effect */
		effectType = rand() % 15;
		switch (effectType)
		{
		case 0:
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, nullptr, 0);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			if (psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
			{
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LARGE, false, nullptr, 0);
			}
			/* Only structures get the big explosions */
			else if (psEffect->type == DESTRUCTION_TYPE_STRUCTURE)
			{
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0);
			}
			else
			{
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
			}
			break;
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			if (psEffect->type == DESTRUCTION_TYPE_STRUCTURE)
			{
				addEffect(&pos, EFFECT_GRAVITON, GRAVITON_TYPE_EMITTING_ST, true, getRandomDebrisImd()->displayModel(), 0);
			}
			else
			{
				addEffect(&pos, EFFECT_GRAVITON, GRAVITON_TYPE_EMITTING_DR, true, getRandomDebrisImd()->displayModel(), 0);
			}
			break;
		case 11:
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, nullptr, 0);
			break;
		case 12:
		case 13:
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
			break;
		case 14:
			/* Add sound effect, but only if we're less than 3/4 of the way thru' destruction */
			if (graphicsTime < (psEffect->birthTime + psEffect->lifeSpan) * 3 / 4)
			{
				iX = static_cast<SDWORD>(psEffect->position.x);
				iY = static_cast<SDWORD>(psEffect->position.z);
				audio_PlayStaticTrack(iX, iY, ID_SOUND_EXPLOSION);
			}
			break;

		}
	}
	return true;
}

/** Moves the construction graphic about - dust cloud or whatever.... */
static bool updateConstruction(EFFECT *psEffect)
{
	/* Time to update the frame number on the construction sprite */
	if (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		/* Are we on the last frame? */
		if (++psEffect->frameNumber >= effectGetNumFrames(psEffect))
		{
			/* Is it a cyclic sprite? */
			if (TEST_CYCLIC(psEffect))
			{
				psEffect->frameNumber = 0;
			}
			else
			{
				return false;
			}
		}
	}

	/* Move it about in the world */

	psEffect->position.x += graphicsTimeAdjustedIncrement(psEffect->velocity.x);
	psEffect->position.y += graphicsTimeAdjustedIncrement(psEffect->velocity.y);
	psEffect->position.z += graphicsTimeAdjustedIncrement(psEffect->velocity.z);


	/* If it doesn't get killed by frame number, then by height */
	if (TEST_CYCLIC(psEffect))
	{
		/* Has it hit the ground */
		if (static_cast<int>(psEffect->position.y) <= map_Height(static_cast<int>(psEffect->position.x), static_cast<int>(psEffect->position.z)))
		{
			return false;
		}

		if (graphicsTime - psEffect->birthTime > psEffect->lifeSpan)
		{
			return false;
		}
	}
	return true;
}

/** Update fire sequences */
static bool updateFire(EFFECT *psEffect, LightingData& lightData)
{
	Vector3i pos;
	LIGHT	light;
	UDWORD	percent;

	percent = PERCENT(graphicsTime - psEffect->birthTime, std::max<unsigned>(psEffect->lifeSpan, 1));
	if (percent > 100)
	{
		percent = 100;
	}

	constexpr int fireLightHeightAboveGround = 28;

	light.position = psEffect->position;
	light.position.y += fireLightHeightAboveGround;
	light.range = (percent * psEffect->radius * 6) / 100;
	light.colour = pal_Colour(255, 0, 0);
	lightData.lights.push_back(light);

	/* Time to update the frame number on the construction sprite */
	if (graphicsTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = graphicsTime;
		pos.x = static_cast<int>(psEffect->position.x + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
		pos.z = static_cast<int>(psEffect->position.z + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));

		// Effect is off map, no need to update it anymore
		if (!worldOnMap(pos.x, pos.z))
		{
			return false;
		}

		pos.y = map_Height(pos.x, pos.z);

		if (psEffect->type == FIRE_TYPE_SMOKY_BLUE)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_FLAMETHROWER, false, nullptr, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
		}

		if (psEffect->type == FIRE_TYPE_SMOKY || psEffect->type == FIRE_TYPE_SMOKY_BLUE)
		{
			pos.x = static_cast<int>(psEffect->position.x + ((rand() % psEffect->radius / 2) - (rand() % (2 * psEffect->radius / 2))));
			pos.z = static_cast<int>(psEffect->position.z + ((rand() % psEffect->radius / 2) - (rand() % (2 * psEffect->radius / 2))));
			pos.y = map_Height(pos.x, pos.z);
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING_HIGH, false, nullptr, 0);
		}
		else
		{
			pos.x = static_cast<int>(psEffect->position.x + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
			pos.z = static_cast<int>(psEffect->position.z + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));

			// Effect is off map, no need to update it anymore
			if (!worldOnMap(pos.x, pos.z))
			{
				return false;
			}

			pos.y = map_Height(pos.x, pos.z);
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
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
		return false;
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// ALL THE RENDER FUNCTIONS
// ----------------------------------------------------------------------------------------
/** Calls the appropriate render routine for each type of effect */
void renderEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	/* What type of effect are we dealing with? */
	switch (psEffect->group)
	{
	case EFFECT_WAYPOINT:
		renderWaypointEffect(psEffect, viewMatrix);
		return;

	case EFFECT_EXPLOSION:
		renderExplosionEffect(psEffect, viewMatrix);
		return;

	case EFFECT_CONSTRUCTION:
		renderConstructionEffect(psEffect, viewMatrix);
		return;

	case EFFECT_SMOKE:
		renderSmokeEffect(psEffect, viewMatrix);
		return;

	case EFFECT_GRAVITON:
		renderGravitonEffect(psEffect, viewMatrix);
		return;

	case EFFECT_BLOOD:
		renderBloodEffect(psEffect, viewMatrix);
		return;

	case EFFECT_DESTRUCTION:
		/*	There is no display func for a destruction effect -
			it merely spawn other effects over time */
		renderDestructionEffect(psEffect, viewMatrix);
		return;

	case EFFECT_FIRE:
		/* Likewise */
		return;

	case EFFECT_SAT_LASER:
		/* Likewise */
		return;

	case EFFECT_FIREWORK:
		renderFirework(psEffect, viewMatrix);
		return;

	case EFFECT_DROID_ANIMEVENT_DYING:
		renderDroidDeathAnimationEffect(psEffect, viewMatrix);
		return;

	case EFFECT_FREED:
		break;
	}

	debug(LOG_ERROR, "Weirdy class of effect passed to renderEffect");
	abort();
}

/** drawing func for wapypoints */
static void renderWaypointEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	pie_Draw3DShape(psEffect->imd, 0, 0, WZCOL_WHITE, 0, 0, positionEffect(psEffect), viewMatrix);
}

static void renderFirework(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	/* these don't get rendered */
	if (psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
		return;
	}

	glm::mat4 modelMatrix = positionEffect(psEffect);
	modelMatrix *= glm::rotate(UNDEG(-playerPos.r.y), glm::vec3(0.f, 1.f, 0.f)) * glm::rotate(UNDEG(-playerPos.r.x), glm::vec3(1.f, 0.f, 0.f))
	               * glm::scale(glm::vec3(psEffect->size / 100.f));

	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, WZCOL_WHITE, pie_ADDITIVE | pie_NODEPTHWRITE, EFFECT_EXPLOSION_ADDITIVE, modelMatrix, viewMatrix);
}

/** drawing func for blood. */
static void renderBloodEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	glm::mat4 modelMatrix = positionEffect(psEffect);
	modelMatrix *= glm::rotate(UNDEG(-playerPos.r.y), glm::vec3(0.f, 1.f, 0.f)) * glm::rotate(UNDEG(-playerPos.r.x), glm::vec3(1.f, 0.f, 0.f))
	               * glm::scale(glm::vec3(psEffect->size / 100.f));

	pie_Draw3DShape(getDisplayImdFromIndex(MI_BLOOD), psEffect->frameNumber, 0, WZCOL_WHITE, pie_TRANSLUCENT | pie_NODEPTHWRITE, EFFECT_BLOOD_TRANSPARENCY, modelMatrix, viewMatrix);
}

static void renderDestructionEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	float	div;
	SDWORD	percent;

	if (psEffect->type != DESTRUCTION_TYPE_SKYSCRAPER)
	{
		return;
	}

	glm::mat4 modelMatrix = positionEffect(psEffect);

	div = (float)(graphicsTime - psEffect->birthTime) / psEffect->lifeSpan;
	if (div > 1.0f)
	{
		div = 1.0f;    //temporary!
	}
	{
		div = 1.0f - div;
		percent = (SDWORD)(div * pie_RAISE_SCALE);
	}

	if (!gamePaused())
	{
		modelMatrix *=
			glm::rotate(UNDEG(SKY_SHIMMY), glm::vec3(1.f, 0.f, 0.f)) *
			glm::rotate(UNDEG(SKY_SHIMMY), glm::vec3(0.f, 1.f, 0.f)) *
			glm::rotate(UNDEG(SKY_SHIMMY), glm::vec3(0.f, 0.f, 1.f));
	}
	pie_Draw3DShape(psEffect->imd, 0, 0, WZCOL_WHITE, pie_RAISE, percent, modelMatrix, viewMatrix);
}

static bool rejectLandLight(LAND_LIGHT_SPEC type)
{
	unsigned int timeSlice = graphicsTime % 2000;

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
		return (false); //reject none
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
static void renderExplosionEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	const PIELIGHT brightness = WZCOL_WHITE;

	if (psEffect == nullptr || psEffect->imd == nullptr)
	{
		return;
	}

	if (psEffect->type == EXPLOSION_TYPE_LAND_LIGHT)
	{
		if (rejectLandLight((LAND_LIGHT_SPEC)psEffect->specific))
		{
			return;
		}
	}

	glm::mat4 modelMatrix = positionEffect(psEffect);

	/* Bit in comments - doesn't quite work yet? */
	if (TEST_FACING(psEffect))
	{
		/* Always face the viewer! */
		// TODO This only faces towards the viewer, if the effect is in the middle of the screen... It draws the effect parallel with the screens near/far planes.
		modelMatrix *=
			glm::rotate(UNDEG(-playerPos.r.y), glm::vec3(0.f, 1.f, 0.f)) *
			glm::rotate(UNDEG(-playerPos.r.x), glm::vec3(1.f, 0.f, 0.f));
	}

	/* Tesla explosions diminish in size */
	if (psEffect->type == EXPLOSION_TYPE_TESLA)
	{
		float scale = (graphicsTime - psEffect->birthTime) / (float)psEffect->lifeSpan;
		if (scale < 0.f)
		{
			scale = 0.f;
		}
		else if (scale > .45f)
		{
			scale = .45f;
		}
		modelMatrix *= glm::scale(glm::vec3(psEffect->size / 100.f - scale));
	}
	else if (psEffect->type == EXPLOSION_TYPE_PLASMA)
	{
		float scale = (graphicsTime - psEffect->birthTime) / (float)psEffect->lifeSpan / 3.f;
		modelMatrix *= glm::scale(glm::vec3(BASE_PLASMA_SIZE / 100.f + scale));
	}
	else
	{
		modelMatrix *= glm::scale(glm::vec3(psEffect->size / 100.f));
	}

	bool premultiplied = false;
	if (psEffect->imd->flags & iV_IMD_PREMULTIPLIED)
	{
		premultiplied = true;
	}

	if (premultiplied)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_PREMULTIPLIED | pie_NODEPTHWRITE, 0, modelMatrix, viewMatrix);
	}
	else if (psEffect->type == EXPLOSION_TYPE_PLASMA)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_ADDITIVE | pie_NODEPTHWRITE, EFFECT_PLASMA_ADDITIVE, modelMatrix, viewMatrix);
	}
	else if (psEffect->type == EXPLOSION_TYPE_KICKUP)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT | pie_NODEPTHWRITE, 128, modelMatrix, viewMatrix);
	}
	else
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_ADDITIVE | pie_NODEPTHWRITE, EFFECT_EXPLOSION_ADDITIVE, modelMatrix, viewMatrix);
	}
}

static void renderGravitonEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	glm::mat4 modelMatrix = positionEffect(psEffect);
	modelMatrix *=
		glm::rotate(UNDEG(psEffect->rotation.x), glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(UNDEG(psEffect->rotation.y), glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(UNDEG(psEffect->rotation.z), glm::vec3(0.f, 0.f, 1.f));

	/* Buildings emitted by gravitons are chunkier */
	if (psEffect->type == GRAVITON_TYPE_EMITTING_ST)
	{
		/* Twice as big - 150 percent */
		modelMatrix *= glm::scale(glm::vec3(psEffect->size / 100.f));
	}

	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, psEffect->player, WZCOL_WHITE, pie_SHADOW, 0, modelMatrix, viewMatrix);
}

/** Renders the standard construction effect */
static void renderConstructionEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	Vector3i null;
	int percent, translucency;
	float size;

	/* No rotation about arbitrary axis */
	null.x = null.y = null.z = 0;

	glm::mat4 modelMatrix = positionEffect(psEffect);

	/* Bit in comments doesn't quite work yet? */
	if (TEST_FACING(psEffect))
	{
		modelMatrix *=
			glm::rotate(UNDEG(-playerPos.r.y), glm::vec3(0.f, 1.f, 0.f)) *
			glm::rotate(UNDEG(-playerPos.r.x), glm::vec3(1.f, 0.f, 0.f));
	}

	/* Scale size according to age */
	percent = PERCENT(graphicsTime - psEffect->birthTime, psEffect->lifeSpan);
	if (percent < 0)
	{
		percent = 0;
	}
	if (percent > 100)
	{
		percent = 100;
	}

	/* Make imds be transparent on 3dfx */
	if (percent < 50)
	{
		translucency = percent * 2;
	}
	else
	{
		translucency = (100 - percent) * 2;
	}
	translucency += 10;
	size = MIN(2.f * translucency / 100.f, .90f);
	modelMatrix *= glm::scale(glm::vec3(size));

	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, WZCOL_WHITE, pie_TRANSLUCENT | pie_NODEPTHWRITE, translucency, modelMatrix, viewMatrix);
}

/** Renders the standard smoke effect - it is now scaled in real-time as well */
static void renderSmokeEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix)
{
	int transparency = 0;
	const PIELIGHT brightness = WZCOL_WHITE;

	glm::mat4 modelMatrix = positionEffect(psEffect);

	/* Bit in comments doesn't quite work yet? */
	if (TEST_FACING(psEffect))
	{
		/* Always face the viewer! */
		modelMatrix *= glm::rotate(UNDEG(-playerPos.r.y), glm::vec3(0.f, 1.f, 0.f)) *
			glm::rotate(UNDEG(-playerPos.r.x), glm::vec3(1.f, 0.f, 0.f));
	}

	if (TEST_SCALED(psEffect))
	{
		const unsigned int lifetime = graphicsTime - psEffect->birthTime;
		unsigned int percent;

		// Since psEffect->birthTime will be set to graphicsTime on
		// creation, and graphicsTime only increments, birthTime should be
		// smaller than or equal to graphicsTime. As a great man once said
		// though (hehe, I just love exaggerating ;-) -- Giel):
		// "Always assert your assumptions!". So here it goes:
		assert(psEffect->birthTime <= graphicsTime);

		ASSERT(psEffect->lifeSpan != 0, "Effect lifespan is zero (seconds); it really should be non-zero!");

		// HACK: Work around a bug that bit me causing a "integer divide by zero" at this location -- Giel
		if (psEffect->lifeSpan != 0)
		{
			percent = (lifetime * 100) / psEffect->lifeSpan;
		}
		else
		{
			percent = 100;
		}

		float scale = (percent + psEffect->baseScale) / 100.f;
		modelMatrix *= glm::scale(glm::vec3(scale));
		transparency = (EFFECT_SMOKE_TRANSPARENCY * (100 - percent)) / 100;
	}

	transparency = (transparency * 3) / 2;  //JPS smoke strength increased for d3d 12 may 99

	/* Make imds be transparent on 3dfx */
	if (psEffect->type == SMOKE_TYPE_STEAM)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT | pie_NODEPTHWRITE, EFFECT_STEAM_TRANSPARENCY / 2, modelMatrix, viewMatrix);
	}
	else
	{
		if (psEffect->type == SMOKE_TYPE_TRAIL)
		{
			pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT | pie_NODEPTHWRITE, (2 * transparency) / 3, modelMatrix, viewMatrix);
		}
		else
		{
			pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, pie_TRANSLUCENT | pie_NODEPTHWRITE, transparency / 2, modelMatrix, viewMatrix);
		}
	}
}


// ----------------------------------------------------------------------------------------
// ALL THE SETUP FUNCTIONS
// ----------------------------------------------------------------------------------------
void	effectSetupFirework(EFFECT& effect)
{
	if (effect.type == FIREWORK_TYPE_LAUNCHER)
	{
		effect.velocity.x = 200 - rand() % 400;
		effect.velocity.z = 200 - rand() % 400;
		effect.velocity.y = 400 + rand() % 200;	//height
		effect.lifeSpan = GAME_TICKS_PER_SEC * 3;
		effect.radius = 80 + rand() % 150;
		effect.size = 300 + rand() % 300;	//height it goes off
		effect.imd = getDisplayImdFromIndex(MI_FIREWORK); // not actually drawn
	}
	else
	{
		effect.velocity.x = 20 - rand() % 40;
		effect.velocity.z = 20 - rand() % 40;
		effect.velocity.y = 0 - (20 + rand() % 40);	//height
		effect.lifeSpan = GAME_TICKS_PER_SEC * 4;

		/* setup the imds */
		switch (rand() % 3)
		{
		case 0:
			effect.imd = getDisplayImdFromIndex(MI_FIREWORK);
			effect.size = 45;	//size of graphic
			break;
		case 1:
			effect.imd = getDisplayImdFromIndex(MI_SNOW);
			SET_CYCLIC(effect);
			effect.size = 60;	//size of graphic

			break;
		default:
			effect.imd = getDisplayImdFromIndex(MI_FLAME);
			effect.size = 40;	//size of graphic


			break;
		}
	}
	effect.frameDelay = (EXPLOSION_FRAME_DELAY * 2);
}

void	effectSetupSmoke(EFFECT& effect)
{
	/* everything except steam drifts about */
	if (effect.type == SMOKE_TYPE_STEAM)
	{
		/* Only upwards */
		effect.velocity.x = 0.f;
		effect.velocity.z = 0.f;
	}
	else if (effect.type == SMOKE_TYPE_BILLOW)
	{

		effect.velocity.x = (float)(10 - rand() % 20);
		effect.velocity.z = (float)(10 - rand() % 20);
	}
	else
	{
		effect.velocity.x = (float)(rand() % 20);
		effect.velocity.z = (float)(10 - rand() % 20);
	}

	/* Steam isn't cyclic  - it doesn't grow with time either */
	if (effect.type != SMOKE_TYPE_STEAM)
	{
		SET_CYCLIC(effect);
		SET_SCALED(effect);
	}

	switch (effect.type)
	{
	case SMOKE_TYPE_DRIFTING:
		effect.imd = getDisplayImdFromIndex(MI_SMALL_SMOKE);
		effect.lifeSpan = (UWORD)NORMAL_SMOKE_LIFESPAN;
		effect.velocity.y = (float)(35 + rand() % 30);
		effect.baseScale = 40;
		break;
	case SMOKE_TYPE_DRIFTING_HIGH:
		effect.imd = getDisplayImdFromIndex(MI_SMALL_SMOKE);
		effect.lifeSpan = (UWORD)NORMAL_SMOKE_LIFESPAN;
		effect.velocity.y = (float)(40 + rand() % 45);
		effect.baseScale = 25;
		break;
	case SMOKE_TYPE_DRIFTING_SMALL:
		effect.imd = getDisplayImdFromIndex(MI_SMALL_SMOKE);
		effect.lifeSpan = (UWORD)SMALL_SMOKE_LIFESPAN;
		effect.velocity.y = (float)(25 + rand() % 35);
		effect.baseScale = 17;
		break;
	case SMOKE_TYPE_BILLOW:
		effect.imd = getDisplayImdFromIndex(MI_SMALL_SMOKE);
		effect.lifeSpan = (UWORD)SMALL_SMOKE_LIFESPAN;
		effect.velocity.y = (float)(10 + rand() % 20);
		effect.baseScale = 80;
		break;
	case SMOKE_TYPE_STEAM:
		effect.imd = getDisplayImdFromIndex(MI_SMALL_STEAM);
		effect.velocity.y = (float)(rand() % 5);
		break;
	case SMOKE_TYPE_TRAIL:
		effect.imd = getDisplayImdFromIndex(MI_TRAIL);
		effect.lifeSpan = TRAIL_SMOKE_LIFESPAN;
		effect.velocity.y = (float)(5 + rand() % 10);
		effect.baseScale = 25;
		break;
	default:
		ASSERT(false, "Weird smoke type");
		break;
	}

	/* It always faces you */
	SET_FACING(effect);

	effect.frameDelay = (UWORD)SMOKE_FRAME_DELAY;
	/* Randomly flip gfx for variation */
	if (ONEINTWO)
	{
		SET_FLIPPED_X(effect);
	}
	if (ONEINTWO)
	{
		SET_FLIPPED_Y(effect);
	}
}

void effectSetupSatLaser(EFFECT& effect)
{
	/* Does nothing at all..... Runs only for one frame! */
	effect.baseScale = 1;
	return;
}

void	effectSetupGraviton(EFFECT& effect)
{
	switch (effect.type)
	{
	case GRAVITON_TYPE_GIBLET:
		effect.velocity.x += GIBLET_INIT_VEL_X;
		effect.velocity.z += GIBLET_INIT_VEL_Z;
		effect.velocity.y = GIBLET_INIT_VEL_Y;
		break;
	case GRAVITON_TYPE_EMITTING_ST:
		effect.velocity.x = GRAVITON_INIT_VEL_X;
		effect.velocity.z = GRAVITON_INIT_VEL_Z;
		effect.velocity.y = (5 * GRAVITON_INIT_VEL_Y) / 4;
		effect.size = (UWORD)(120 + rand() % 30);
		break;
	case GRAVITON_TYPE_EMITTING_DR:
		effect.velocity.x += GRAVITON_INIT_VEL_X / 2;
		effect.velocity.z += GRAVITON_INIT_VEL_Z / 2;
		effect.velocity.y = GRAVITON_INIT_VEL_Y;
		break;
	default:
		ASSERT(false, "Weirdy type of graviton");
		break;

	}

	effect.rotation.x = rand() % DEG(360);
	effect.rotation.z = rand() % DEG(360);
	effect.rotation.y = rand() % DEG(360);

	effect.spin.x = rand() % DEG(100) + DEG(20);
	effect.spin.z = rand() % DEG(100) + DEG(20);
	effect.spin.y = rand() % DEG(100) + DEG(20);

	/* Gravitons are essential */
	SET_ESSENTIAL(effect);

	if (effect.type == GRAVITON_TYPE_GIBLET)
	{
		effect.frameDelay = (UWORD)GRAVITON_BLOOD_DELAY;
	}
	else
	{
		effect.frameDelay = (UWORD)GRAVITON_FRAME_DELAY;
	}
}

void effectSetupExplosion(EFFECT& effect)
{
	/* Get an imd if it's not established */
	if (effect.imd == nullptr)
	{
		switch (effect.type)
		{
		case EXPLOSION_TYPE_SMALL:
			effect.imd = getDisplayImdFromIndex(MI_EXPLOSION_SMALL);
			effect.size = (UBYTE)((6 * EXPLOSION_SIZE) / 5);
			break;
		case EXPLOSION_TYPE_VERY_SMALL:
			effect.imd = getDisplayImdFromIndex(MI_EXPLOSION_SMALL);
			effect.size = (UBYTE)(BASE_FLAME_SIZE + auxVar);
			break;
		case EXPLOSION_TYPE_MEDIUM:
			effect.imd = getDisplayImdFromIndex(MI_EXPLOSION_MEDIUM);
			effect.size = (UBYTE)EXPLOSION_SIZE;
			break;
		case EXPLOSION_TYPE_LARGE:
			effect.imd = getDisplayImdFromIndex(MI_EXPLOSION_MEDIUM);
			effect.size = (UBYTE)EXPLOSION_SIZE * 2;
			break;
		case EXPLOSION_TYPE_FLAMETHROWER:
			effect.imd = getDisplayImdFromIndex(MI_FLAME);
			effect.size = (UBYTE)(BASE_FLAME_SIZE + auxVar);
			break;
		case EXPLOSION_TYPE_LASER:
			effect.imd = getDisplayImdFromIndex(MI_FLAME);	// change this
			effect.size = (UBYTE)(BASE_LASER_SIZE + auxVar);
			break;
		case EXPLOSION_TYPE_DISCOVERY:
			effect.imd = getDisplayImdFromIndex(MI_TESLA);	// change this
			effect.size = DISCOVERY_SIZE;
			break;
		case EXPLOSION_TYPE_FLARE:
			effect.imd = getDisplayImdFromIndex(MI_MFLARE);
			effect.size = FLARE_SIZE;
			break;
		case EXPLOSION_TYPE_TESLA:
			effect.imd = getDisplayImdFromIndex(MI_TESLA);
			effect.size = TESLA_SIZE;
			effect.velocity.y = (float)TESLA_SPEED;
			break;

		case EXPLOSION_TYPE_KICKUP:
			effect.imd = getDisplayImdFromIndex(MI_KICK);
			effect.size = 100;
			break;
		case EXPLOSION_TYPE_PLASMA:
			effect.imd = getDisplayImdFromIndex(MI_PLASMA);
			effect.size = BASE_PLASMA_SIZE;
			effect.velocity.y = 0.0f;
			break;
		case EXPLOSION_TYPE_LAND_LIGHT:
			effect.imd = getDisplayImdFromIndex(MI_LANDING);
			effect.size = 120;
			effect.specific = ellSpec;
			effect.velocity.y = 0.0f;
			SET_ESSENTIAL(effect);		// Landing lights are permanent and cyclic
			break;
		case EXPLOSION_TYPE_SHOCKWAVE:
			effect.imd = getDisplayImdFromIndex(MI_SHOCK);
			effect.size = 50;
			effect.velocity.y = 0.0f;
			break;
		default:
			ASSERT_OR_RETURN(, effect.imd != nullptr, "Explosion type unknown!");
			break;
		}
	}

	if (effect.type == EXPLOSION_TYPE_FLAMETHROWER)
	{
		effect.frameDelay = 45;
	}
	/* Set how long it lasts */
	else if (effect.type == EXPLOSION_TYPE_LASER)
	{
		effect.frameDelay = EXPLOSION_FRAME_DELAY / 2;
	}
	else if (effect.type == EXPLOSION_TYPE_TESLA)
	{
		effect.frameDelay = EXPLOSION_TESLA_FRAME_DELAY;
	}
	else

		if (effect.type == EXPLOSION_TYPE_PLASMA)
		{
			effect.frameDelay = EXPLOSION_PLASMA_FRAME_DELAY;
		}
		else if (effect.type == EXPLOSION_TYPE_LAND_LIGHT)
		{
			effect.frameDelay = 120;
		}
		else
		{
			effect.frameDelay = (UWORD)EXPLOSION_FRAME_DELAY;
		}


	if (effect.type == EXPLOSION_TYPE_SHOCKWAVE)
	{
		effect.lifeSpan = GAME_TICKS_PER_SEC;
	}
	else
	{
		effect.lifeSpan = (effect.frameDelay * effect.imd->numFrames);
	}


	if ((effect.type != EXPLOSION_TYPE_NOT_FACING) && (effect.type != EXPLOSION_TYPE_SHOCKWAVE))
	{
		SET_FACING(effect);
	}
	/* Randomly flip x and y for variation */
	if (ONEINTWO)
	{
		SET_FLIPPED_X(effect);
	}
	if (ONEINTWO)
	{
		SET_FLIPPED_Y(effect);
	}
}

void	effectSetupConstruction(EFFECT& effect)
{
	effect.velocity.x = 0.f;//(1-rand()%3);
	effect.velocity.z = 0.f;//(1-rand()%3);
	effect.velocity.y = (float)(0 - rand() % 3);
	effect.frameDelay = (UWORD)CONSTRUCTION_FRAME_DELAY;
	effect.imd = getDisplayImdFromIndex(MI_CONSTRUCTION);
	effect.lifeSpan = CONSTRUCTION_LIFESPAN;

	/* These effects always face you */
	SET_FACING(effect);

	/* It's a cyclic anim - dies on age */
	SET_CYCLIC(effect);

	/* Randomly flip the construction graphics in x and y for variation */
	if (ONEINTWO)
	{
		SET_FLIPPED_X(effect);
	}
	if (ONEINTWO)
	{
		SET_FLIPPED_Y(effect);
	}
}

void	effectSetupFire(EFFECT& effect)
{
	effect.frameDelay = 300;	   // needs to be investigated...
	effect.radius = auxVar;	// needs to be investigated
	effect.lifeSpan = (UWORD)auxVarSec;
	effect.birthTime = graphicsTime;
	SET_ESSENTIAL(effect);

}

void	effectSetupWayPoint(EFFECT& effect)
{
	effect.imd = (pProximityMsgIMD) ? pProximityMsgIMD->displayModel() : nullptr;

	/* These effects musnt make way for others */
	SET_ESSENTIAL(effect);
}


static void effectSetupBlood(EFFECT& effect)
{
	effect.frameDelay = BLOOD_FRAME_DELAY;
	effect.velocity.y = (float)BLOOD_FALL_SPEED;
	effect.imd = getDisplayImdFromIndex(MI_BLOOD);
	effect.size = (UBYTE)BLOOD_SIZE;
}

static void effectSetupDestruction(EFFECT& effect)
{
	if (effect.type == DESTRUCTION_TYPE_SKYSCRAPER)
	{
		effect.lifeSpan = (3 * GAME_TICKS_PER_SEC) / 2 + (rand() % GAME_TICKS_PER_SEC);
		effect.frameDelay = DESTRUCTION_FRAME_DELAY / 2;
	}
	else if (effect.type == DESTRUCTION_TYPE_DROID)
	{
		/* It's all over quickly for droids */
		effect.lifeSpan = DROID_DESTRUCTION_DURATION;
		effect.frameDelay = DESTRUCTION_FRAME_DELAY;
	}
	else if (effect.type == DESTRUCTION_TYPE_WALL_SECTION ||
		effect.type == DESTRUCTION_TYPE_FEATURE)
	{
		effect.lifeSpan = STRUCTURE_DESTRUCTION_DURATION / 4;
		effect.frameDelay = DESTRUCTION_FRAME_DELAY / 2;
	}
	else if (effect.type == DESTRUCTION_TYPE_POWER_STATION)
	{
		effect.lifeSpan = STRUCTURE_DESTRUCTION_DURATION / 2;
		effect.frameDelay = DESTRUCTION_FRAME_DELAY / 4;
	}
	else
	{
		/* building's destruction is longer */
		effect.lifeSpan = STRUCTURE_DESTRUCTION_DURATION;
		effect.frameDelay = DESTRUCTION_FRAME_DELAY / 2;
	}
}


#define SMOKE_SHIFT (16 - (rand()%32))
void initPerimeterSmoke(const iIMDShape *pImd, Vector3i base)
{
	int i;
	int shift = SMOKE_SHIFT;
	int inStart = pImd->min.z - 16, inEnd = pImd->max.z + 16;
	int varStart = pImd->min.x - 16, varEnd = pImd->max.x + 16, varStride = 24;

	for (i = varStart; i < varEnd; i += varStride)
	{
		Vector3i pos = base + Vector3i(i + shift, 0, inStart + shift);

		if (rand() % 6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, nullptr, 0);
		}

		pos = base + Vector3i(i + shift, 0, inEnd + shift);

		if (rand() % 6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, nullptr, 0);
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

		if (rand() % 6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, nullptr, 0);
		}

		pos = base + Vector3i(inEnd + shift, 0, i + shift);

		if (rand() % 6 == 1)
		{
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, nullptr, 0);
		}
		else
		{
			addEffect(&pos, EFFECT_SMOKE, SMOKE_TYPE_BILLOW, false, nullptr, 0);
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


void	effectGiveAuxVar(UDWORD var)
{
	auxVar = var;
}


void	effectGiveAuxVarSec(UDWORD var)
{
	auxVarSec = var;
}

/** Runs all the structure effect stuff - steam puffing out etc */
static void effectStructureUpdates()
{
	auto frameNumber = frameGetFrameNumber();
	unsigned curPartition = frameNumber % EFFECT_STRUCTURE_DIVISION;
	// Is it the right time?
	if (graphicsTime <= lastUpdateStructures[curPartition] + STRUCTURE_UPDATE_INTERVAL)
	{
		return;
	}
	// Store away the last update time.
	lastUpdateStructures[curPartition] = graphicsTime;

	/* Go thru' all players */
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (STRUCTURE *psStructure : apsStructLists[player])
		{
			// Find its group.
			unsigned int partition = psStructure->id % EFFECT_STRUCTURE_DIVISION;

			/* Is it the right frame? */
			if (partition != curPartition)
			{
				continue;
			}

			if (psStructure->status != SS_BUILT || !psStructure->visibleForLocalDisplay())
			{
				continue;
			}

			const iIMDShape *pDisplayModel = psStructure->sDisplay.imd->displayModel();

			/* Factories puff out smoke, power stations puff out tesla stuff */
			switch (psStructure->pStructureType->type)
			{
			case REF_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
				/*
					We're a factory, so better puff out a bit of steam
					Complete hack with the magic numbers - just for IAN demo
				*/
				// Display only - use the display model
				if (pDisplayModel->connectors.size() > 0)
				{
					unsigned int numConnectors = static_cast<unsigned int>(pDisplayModel->connectors.size());;
					unsigned int orderAdjustment = frameNumber % numConnectors;
					for (unsigned int idx = 0; idx < numConnectors; ++idx)
					{
						Vector3i eventPos = psStructure->pos.xzy() + Affine3F().RotY(psStructure->rot.direction)*Vector3i(
												pDisplayModel->connectors[idx].x,
												pDisplayModel->connectors[idx].z,
												-pDisplayModel->connectors[idx].y
											);

						unsigned int orderIdx = idx + orderAdjustment;
						if (orderIdx >= numConnectors)
						{
							orderIdx = orderIdx - numConnectors;
						}
						unsigned effectStartTime = graphicsTime + static_cast<UDWORD>(150 * orderIdx);
						addEffect(&eventPos, EFFECT_SMOKE, SMOKE_TYPE_STEAM, false, nullptr, 0, effectStartTime);
					}
					audio_PlayObjStaticTrack(psStructure, ID_SOUND_STEAM);
				}
				break;
			case REF_POWER_GEN:
				{
					Vector3i eventPos = psStructure->pos.xzy();

					// Add an effect over the central spire.

					eventPos.y = psStructure->pos.z + 48;

					addEffect(&eventPos, EFFECT_EXPLOSION, EXPLOSION_TYPE_TESLA, false, nullptr, 0);

					audio_PlayObjStaticTrack(psStructure, ID_SOUND_POWER_SPARK);
					audio_PlayObjStaticTrack(psStructure, ID_SOUND_POWER_HUM);
					break;
				}
			case REF_RESOURCE_EXTRACTOR:
				{
					if (psStructure->pFunctionality &&
						psStructure->pFunctionality->resourceExtractor.psPowerGen &&
						psStructure->animationEvent == ANIM_EVENT_ACTIVE)
					{
						audio_PlayObjStaticTrack(psStructure, ID_SOUND_OIL_PUMP_2);
					}
					break;
				}
			default:
				break;
			}
		}
	}
}

void effectResetUpdates()
{
	memset(lastUpdateStructures, 0, sizeof(lastUpdateStructures));
}

/** This will save out the effects data */
bool writeFXData(const char *fileName)
{
	int i = 0;
	nlohmann::json mRoot = nlohmann::json::object();
	for (auto iter = gActiveEffects.begin(); iter != gActiveEffects.end(); ++iter, i++)
	{
		const EFFECT& e = *iter;

		nlohmann::json effectObj = nlohmann::json::object();
		effectObj["control"] = e.control;
		effectObj["group"] = e.group;
		effectObj["type"] = e.type;
		effectObj["frameNumber"] = e.frameNumber;
		effectObj["size"] = e.size;
		effectObj["baseScale"] = e.baseScale;
		effectObj["specific"] = e.specific;
		effectObj["position"] = e.position;
		effectObj["velocity"] = e.velocity;
		effectObj["rotation"] = e.rotation;
		effectObj["spin"] = e.spin;
		effectObj["birthTime"] = e.birthTime;
		effectObj["lastFrame"] = e.lastFrame;
		effectObj["frameDelay"] = e.frameDelay;
		effectObj["lifeSpan"] = e.lifeSpan;
		effectObj["radius"] = e.radius;

		if (e.imd)
		{
			effectObj["imd_name"] = modelName(e.imd).toUtf8();
		}

		auto effectKey = "effect_" + WzString::number(i);
		mRoot[effectKey.toUtf8()] = std::move(effectObj);

		// Move on to reading the next effect
	}

	std::string jsonString;
	try {
		jsonString = mRoot.dump(4);
	}
	catch (const std::exception &e) {
		ASSERT(false, "Failed to save JSON to %s with error: %s", fileName, e.what());
		return false;
	}
	debug(LOG_SAVE, "%s %s", "Saving", fileName);
	saveFile(fileName, jsonString.c_str(), jsonString.size());

	// Everything is just fine!
	return true;
}

/** This will read in the effects data */
bool readFXData(const char *fileName)
{
	// Clear out anything that's there already!
	initEffectsSystem();

	WzConfig ini(WzString::fromUtf8(fileName), WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();

	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		EFFECT curEffect;

		curEffect.control      = ini.value("control").toInt();
		curEffect.group        = (EFFECT_GROUP)ini.value("group").toInt();
		curEffect.type         = (EFFECT_TYPE)ini.value("type").toInt();
		curEffect.frameNumber  = ini.value("frameNumber").toInt();
		curEffect.size         = ini.value("size").toInt();
		curEffect.baseScale    = ini.value("baseScale").toInt();
		curEffect.specific     = ini.value("specific").toInt();
		curEffect.position     = ini.vector3f("position");
		curEffect.velocity     = ini.vector3f("velocity");
		curEffect.rotation     = ini.vector3i("rotation");
		curEffect.spin         = ini.vector3i("spin");
		curEffect.birthTime    = ini.value("birthTime").toInt();
		curEffect.lastFrame    = ini.value("lastFrame").toInt();
		curEffect.frameDelay   = ini.value("frameDelay").toInt();
		curEffect.lifeSpan     = ini.value("lifeSpan").toInt(); // this is the original duration of the efect, not the time remaining
		curEffect.radius       = ini.value("radius").toInt();
		if (ini.contains("imd_name"))
		{
			WzString imd_name = ini.value("imd_name").toWzString();
			if (!imd_name.isEmpty())
			{
				auto baseImd = modelGet(imd_name);
				curEffect.imd = (baseImd) ? baseImd->displayModel() : nullptr;
			}
		}
		else
		{
			curEffect.imd = nullptr;
		}

		// For fire effects, set the tile as being on fire so that (e.g.) burning oil resources can't
		// immediately be built on
		if (EFFECT_FIRE == curEffect.group)
		{
			const int timeThatEffectHasBeenRunning = curEffect.lastFrame - curEffect.birthTime;
			const int timeLeftToRun = curEffect.lifeSpan - timeThatEffectHasBeenRunning;

			// Sanity check - don't allow a negative time to wrap to a huge positive unsigned value.
			if (timeLeftToRun > 0)
			{
				tileSetFire(static_cast<int32_t>(curEffect.position.x), static_cast<int32_t>(curEffect.position.z), static_cast<uint32_t>(timeLeftToRun));
			}
		}

		// Move on to reading the next effect
		ini.endGroup();

		gActiveEffects.emplace(std::move(curEffect));
	}

	/* Hopefully everything's just fine by now */
	return true;
}
