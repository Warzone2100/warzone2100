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

#ifndef __INCLUDED_SRC_MISCIMD_H__
#define __INCLUDED_SRC_MISCIMD_H__

#include "structure.h"
#include "structuredef.h"
#include "messagedef.h"

bool initMiscImds();
iIMDShape *getImdFromIndex(UDWORD index);
iIMDShape *getRandomDebrisImd();

#define	MAX_DEBRIS		5
#define	MAX_WRECKAGE	5

extern iIMDShape	*explosionSmallImd;	// Set this up to point to the explosion imd
extern iIMDShape	*explosionMediumImd;	// Set this up to point to the explosion imd
extern iIMDShape	*constructionImd;
extern iIMDShape	*smallSmokeImd;
extern iIMDShape	*babaHeadImd;
extern iIMDShape	*babaBodyImd;
extern iIMDShape	*babaLegsImd;
extern iIMDShape	*babaArmImd;
extern iIMDShape	*cyborgHeadImd;
extern iIMDShape	*cyborgBodyImd;
extern iIMDShape	*cyborgLegsImd;
extern iIMDShape	*cyborgArmImd;
extern iIMDShape	*waterImd;
extern iIMDShape	*droidDamageImd;
extern iIMDShape	*smallSteamImd;
extern iIMDShape	*plasmaImd;
#define MAX_FACTORY_FLAG_IMDS 32
extern iIMDShape	*pAssemblyPointIMDs[NUM_FLAG_TYPES][MAX_FACTORY_FLAG_IMDS];
extern iIMDShape	*blipImd;
extern iIMDShape	*shadowImd;
extern iIMDShape	*transporterShadowImd;
extern iIMDShape	*bloodImd;
extern iIMDShape	*trailImd;
extern iIMDShape	*cameraImd;
extern iIMDShape	*debrisImds[MAX_DEBRIS];
extern iIMDShape	*flameImd;
extern iIMDShape	*wreckageImds[MAX_WRECKAGE];
extern iIMDShape	*proximityImds[PROX_TYPES];
extern iIMDShape	*teslaImd;
extern iIMDShape	*mFlareImd;
extern iIMDShape	*snowImd;
extern iIMDShape	*rainImd;
extern iIMDShape	*splashImd;
extern iIMDShape	*kickImd;
extern iIMDShape	*landingImd;
extern iIMDShape	*shockImd;

/* An imd entry */
struct MISC_IMD
{
	iIMDShape	*pImd;
	const char	*pName;
};

enum
{
	MI_EXPLOSION_SMALL,
	MI_EXPLOSION_MEDIUM,
	MI_CONSTRUCTION,
	MI_SMALL_SMOKE,
	MI_BABA_HEAD,
	MI_BABA_LEGS,
	MI_BABA_ARM,
	MI_BABA_BODY,
	MI_CYBORG_HEAD,
	MI_CYBORG_LEGS,
	MI_CYBORG_ARM,
	MI_CYBORG_BODY,
	MI_WATER,
	MI_DROID_DAMAGE,
	MI_SMALL_STEAM,
	MI_PLASMA,
	MI_BLIP,
	MI_SHADOW,
	MI_TRANSPORTER_SHADOW,
	MI_BLOOD,
	MI_TRAIL,
	MI_FLAME,
	MI_TESLA,
	MI_MFLARE,
	MI_RAIN,
	MI_SNOW,
	MI_SPLASH,
	MI_KICK,
	MI_LANDING,
	MI_SHOCK,
	MI_BLIP_ENEMY,
	MI_BLIP_RESOURCE,
	MI_BLIP_ARTEFACT,

	MI_WRECK0,
	MI_WRECK1,
	MI_WRECK2,
	MI_WRECK3,
	MI_WRECK4,

	MI_DEBRIS0,
	MI_DEBRIS1,
	MI_DEBRIS2,
	MI_DEBRIS3,
	MI_DEBRIS4,

	MI_FIREWORK,


	MI_TOO_MANY
};

#endif // __INCLUDED_SRC_MISCIMD_H__
