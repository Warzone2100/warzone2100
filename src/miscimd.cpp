/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#include <assert.h>
#include <string.h>
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "effects.h"
#include "structure.h"
#include "messagedef.h"
#include "miscimd.h"

/* Our great big array of imds */
static MISC_IMD	miscImds[] =
{						// Previous imd pointer names
	{NULL,"fxsexp"},	// explosionSmallImd	MI_EXPLOSION_SMALL
	{NULL,"fxlexp"},	// explosionMediumImd	MI_EXPLOSION_MEDIUM
	{NULL,"fxdust"},	// constructionImd		MI_CONSTRUCTION
	{NULL,"fxsmoke"},	// smallSmokeImd		MI_SMALL_SMOKE
	{NULL,"parthead"},	// babaHeadImd			MI_BABA_HEAD
	{NULL,"partlegs"},	// babaLegsImd			MI_BABA_LEGS
	{NULL,"partarm"},	// babaArmImd			MI_BABA_ARM
	{NULL,"partbody"},	// babaBodyImd			MI_BABA_BODY
	{NULL,"cybitrkt"},	// cyborgHeadImd		MI_CYBORG_HEAD
	{NULL,"cybitlg1"},	// cyborgLegsImd		MI_CYBORG_LEGS
	{NULL,"cybitgun"},	// cyborgArmImd			MI_CYBORG_ARM
	{NULL,"cybitbod"},	// cyborgBodyImd		MI_CYBORG_BODY
	{NULL,"fxatexp"},	// waterImd				MI_WATER
	{NULL,"fxssteam"},	// droidDamageImd		MI_DROID_DAMAGE
	{NULL,"fxssteam"},	// smallSteamImd		MI_SMALL_STEAM
	{NULL,"fxplasma"},	// plasmaImd			MI_PLASMA
	{NULL,"fxblip"},	// blipImd				MI_BLIP
	{NULL,"cyshadow"},	// shadowImd			MI_SHADOW
	{NULL,"Mitrnshd"},	// transporterShadowImd	MI_TRANSPORTER_SHADOW
	{NULL,"fxblood"},	// bloodImd				MI_BLOOD
	{NULL,"fxssmoke"},	// trailImd				MI_TRAIL
	{NULL,"fxft"},		// flameImd				MI_FLAME
	{NULL,"fxpower"},	// teslaImd				MI_TESLA
	{NULL,"fxmflare"},	// mFlareImd			MI_MFLARE
	{NULL,"mirain"},	// rainImd				MI_RAIN
	{NULL,"misnow"},	// snowImd				MI_SNOW
	{NULL,"fxssplsh"},	// splashImd			MI_SPLASH

	{NULL,"fxexpdrt"},	// kickImd				MI_KICK
	{NULL,"fxlightr"},	// landingImd			MI_LANDING
	{NULL,"fxl3dshk"},	// shockImd				MI_SHOCK

	{NULL,"blipenm"},	// proximityImds[0]	   	MI_BLIP_ENEMY
	{NULL,"blipres"},	// proximityImds[1]	   	MI_BLIP_RESOURCE
	{NULL,"blipart"},	// proximityImds[2]	   	MI_BLIP_ARTEFACT

	{NULL,"miwrek1"},	// wreckageImds[0]		MI_WRECK0
	{NULL,"miwrek2"},	// wreckageImds[1]		MI_WRECK1
	{NULL,"miwrek3"},	// wreckageImds[2]		MI_WRECK2
	{NULL,"miwrek4"},	// wreckageImds[3]		MI_WRECK3
	{NULL,"miwrek5"},	// wreckageImds[4]		MI_WRECK4

	{NULL,"midebr1"},	// debrisImds[0]		MI_DEBRIS0
	{NULL,"midebr2"},	// debrisImds[1]		MI_DEBRIS1
	{NULL,"midebr3"},	// debrisImds[2]		MI_DEBRIS2
	{NULL,"midebr4"},	// debrisImds[3]		MI_DEBRIS3
	{NULL,"midebr5"},	// debrisImds[4]		MI_DEBRIS4

	{NULL,"fxflecht"},	// met hit - for repair centre MI_FIREWORK



	{NULL,"END_OF_IMD_LIST"}
};

// -------------------------------------------------------------------------------
// Load up all the imds into an array
static bool multiLoadMiscImds( void )
{
UDWORD	i=0;
bool	bMoreToProcess=true;
char	name[15];	// hopefully!

	/* Go thru' the list */
	while(bMoreToProcess)
	{
		snprintf(name, sizeof(name), "%s.pie", miscImds[i].pName);

		/* see if the resource loader can find it */
		miscImds[i].pImd = (iIMDShape*)resGetData("IMD",name);
		/* If it didn't get it then... */
		if(!miscImds[i].pImd)
		{
			/* Say which one and return false */
			debug( LOG_ERROR, "Can't find misselaneous PIE file : %s", miscImds[i].pName );
			ASSERT( false,"NULL PIE" );
			return(false);
		}
		/*	If the next one's the end one, then get out now.
			This is cos strcmp will return 0 only at end of list
		*/
		bMoreToProcess = strcmp(miscImds[++i].pName,"END_OF_IMD_LIST");
	}
	return true;
}
// -------------------------------------------------------------------------------
// Returns a pointer to the imd from a #define number passed in - see above
iIMDShape	*getImdFromIndex(UDWORD	index)
{
	ASSERT( index<MI_TOO_MANY,"Out of range index in getImdFromIndex" );

	return(miscImds[index].pImd);
}
// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
iIMDShape	*getRandomDebrisImd( void )
{
	iIMDShape *DebrisIMD;

	DebrisIMD = getImdFromIndex(MI_DEBRIS0 + rand()%((MI_DEBRIS4-MI_DEBRIS0)+1));

	ASSERT( DebrisIMD != NULL,"getRandomDebrisImd : NULL PIE" );

	return DebrisIMD;
}
// -------------------------------------------------------------------------------

iIMDShape	*pAssemblyPointIMDs[NUM_FLAG_TYPES][MAX_FACTORY_FLAG_IMDS];

static bool initMiscImd(unsigned i, unsigned n, const char *nameFormat, unsigned flagType)
{
	char pieName[100];
	snprintf(pieName, sizeof(pieName), nameFormat, n);
	pAssemblyPointIMDs[flagType][i] = (iIMDShape *)resGetData("IMD", pieName);
	if (!pAssemblyPointIMDs[flagType][i])
	{
		debug(LOG_ERROR, "Can't find assembly point graphic %s for factory", pieName);

		return false;
	}
	return true;
}

bool	initMiscImds( void )
{
	/* Do the new loading system */
	multiLoadMiscImds();

	/* Now load the multi array stuff */
	for (unsigned i = 0; i < MAX_FACTORY_FLAG_IMDS; ++i)
	{
		unsigned n = i + 1;

		STATIC_ASSERT(MAX_FACTORY <= MAX_FACTORY_FLAG_IMDS);
		STATIC_ASSERT(MAX_FACTORY_FLAG_IMDS <= 32);  // Need to add more assembly point graphics, if increasing MAX_FACTORY_FLAG_IMDS.
		if (!initMiscImd(i, n, "minum%u.pie",  FACTORY_FLAG) ||
		    !initMiscImd(i, n, "micnum%u.pie", CYBORG_FLAG) ||
		    !initMiscImd(i, n, "mivnum%u.pie", VTOL_FLAG) ||
		    !initMiscImd(i, 1, "mirnum%u.pie", REPAIR_FLAG))
		{
			return false;
		}
	}
	return(true);
}
