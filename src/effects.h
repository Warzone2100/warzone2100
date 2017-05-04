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

#ifndef __INCLUDED_SRC_EFFECTS_H__
#define __INCLUDED_SRC_EFFECTS_H__

/*
	All singing, all dancing new effects code. Does all the stuff
	that explosion.c and most of particle.c used to do as well as
	all the construction dust clouds, body parts and all other
	temporary world 'effects
	Alex McLean, Pumpkin Studios, EIDOS Interactive, 1998.
*/
#include "lib/ivis_opengl/piedef.h"
#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/pietypes.h"

#define SHOCK_WAVE_HEIGHT	(64)


/* All the effect groups */
enum EFFECT_GROUP
{
	EFFECT_EXPLOSION,
	EFFECT_CONSTRUCTION,
	EFFECT_SMOKE,
	EFFECT_GRAVITON,
	EFFECT_WAYPOINT,
	EFFECT_BLOOD,
	EFFECT_DESTRUCTION,
	EFFECT_SAT_LASER,
	EFFECT_FIRE,
	EFFECT_FIREWORK,

	EFFECT_FREED,
};


/* Might not even need this */
enum EFFECT_TYPE
{
	EXPLOSION_TYPE_SMALL,
	EXPLOSION_TYPE_VERY_SMALL,
	EXPLOSION_TYPE_MEDIUM,
	EXPLOSION_TYPE_LARGE,
	EXPLOSION_TYPE_SPECIFIED,	// superfluous?
	EXPLOSION_TYPE_NOT_FACING,
	EXPLOSION_TYPE_SPECIFIED_SOLID,
	EXPLOSION_TYPE_SPECIFIED_FIXME,
	EXPLOSION_TYPE_FLAMETHROWER,
	EXPLOSION_TYPE_LASER,
	EXPLOSION_TYPE_TESLA,
	EXPLOSION_TYPE_DISCOVERY,
	EXPLOSION_TYPE_FLARE,
	EXPLOSION_TYPE_PLASMA,
	EXPLOSION_TYPE_KICKUP,
	EXPLOSION_TYPE_LAND_LIGHT,
	EXPLOSION_TYPE_SHOCKWAVE,

	//EXPLOSION_TYPE_SKYSCRAPER,

	GRAVITON_TYPE_STANDARD,
	GRAVITON_TYPE_EMITTING_DR,
	GRAVITON_TYPE_EMITTING_ST,
	GRAVITON_TYPE_GIBLET,

	SMOKE_TYPE_DRIFTING,
	SMOKE_TYPE_DRIFTING_HIGH,
	SMOKE_TYPE_DRIFTING_SMALL,
	SMOKE_TYPE_BILLOW,
	SMOKE_TYPE_STEAM,
	SMOKE_TYPE_TRAIL,
	FIRE_TYPE_LOCALISED,
	FIRE_TYPE_SMOKY,
	FIRE_TYPE_SMOKY_BLUE,
	CONSTRUCTION_TYPE_DRIFTING,

	BLOOD_TYPE_NORMAL,

	DUST_TYPE_NORMAL,
	DESTRUCTION_TYPE_DROID,
	DESTRUCTION_TYPE_STRUCTURE,
	DESTRUCTION_TYPE_POWER_STATION,
	DESTRUCTION_TYPE_WALL_SECTION,
	DESTRUCTION_TYPE_FEATURE,
	DESTRUCTION_TYPE_SKYSCRAPER,

	SAT_LASER_STANDARD,

	WAYPOINT_TYPE,

	FIREWORK_TYPE_STARBURST,
	FIREWORK_TYPE_LAUNCHER,
};


enum LAND_LIGHT_SPEC
{
	LL_MIDDLE,
	LL_INNER,
	LL_OUTER
};


struct EFFECT
{
	uint8_t           player;      //	when the effect in question needs a player's color
	uint8_t           control;     // Controls the bits above - essential,flips etc
	EFFECT_GROUP      group;       // what group is it - explosion, building effect etc....
	EFFECT_TYPE       type;        // what type is it within the group?
	uint8_t           frameNumber; // what frame number is the imd on?
	uint16_t          size;        // Size in terms of percent of original imd.
	uint8_t           baseScale;   // if scaled, what's bottom line?
	uint8_t           specific;    // how many times has it bounced?
	Vector3f          position;    // world coordinates of the effect - floats on the PC.
	Vector3f          velocity;    // movement values per update
	Vector3i          rotation;    // current rotation - only for gravitons
	Vector3i          spin;        // rotation info for spinning things.
	uint32_t          birthTime;   // what time was it introduced into the world?
	uint32_t          lastFrame;   // when did we last update the frame?
	uint16_t          frameDelay;  // how many game ticks between each frame?
	uint16_t          lifeSpan;    // what is it's life expectancy?
	uint16_t          radius;      // Used for area effects
	iIMDShape         *imd;        // pointer to the imd the effect uses.
	EFFECT *prev, *next; // Previous and next element in linked list

	EFFECT() : player(MAX_PLAYERS), control(0), group(EFFECT_FREED), type(EXPLOSION_TYPE_SMALL), frameNumber(0), size(0),
	           baseScale(0), specific(0), birthTime(0), lastFrame(0), frameDelay(0), lifeSpan(0), radius(0),
	           imd(nullptr), prev(nullptr), next(nullptr) {}
};

/* Maximum number of effects in the world - need to investigate what this should be */
/* EXTERNAL REFERENCES */
void	effectGiveAuxVar(UDWORD var);		// naughty
void	effectGiveAuxVarSec(UDWORD var);	// and so's this

void	initEffectsSystem(void);
void	shutdownEffectsSystem(void);
void	processEffects(const glm::mat4 &viewMatrix);
void 	addEffect(const Vector3i *pos, EFFECT_GROUP group, EFFECT_TYPE type, bool specified, iIMDShape *imd, int lit);
void    addEffect(const Vector3i *pos, EFFECT_GROUP group, EFFECT_TYPE type, bool specified, iIMDShape *imd, int lit, unsigned effectTime);
void    addMultiEffect(const Vector3i *basePos, Vector3i *scatter, EFFECT_GROUP group, EFFECT_TYPE type, bool specified, iIMDShape *imd, unsigned int number, bool lit, unsigned int size, unsigned effectTime);

void	renderEffect(const EFFECT *psEffect, const glm::mat4 &viewMatrix);
void	effectResetUpdates(void);

void	initPerimeterSmoke(iIMDShape *pImd, Vector3i base);

bool	readFXData(const char *fileName);
bool	writeFXData(const char *fileName);
void	effectSetSize(UDWORD size);
void	effectSetLandLightSpec(LAND_LIGHT_SPEC spec);
void	SetEffectForPlayer(uint8_t player);

#endif // __INCLUDED_SRC_EFFECTS_H__
