/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/input.h"

#include "lib/ivis_common/ivisdef.h" //ivis matrix code
#include "lib/ivis_common/piedef.h" //ivis matrix code
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/gamelib/gtime.h"
#include "display3d.h"
#include "map.h"
#include "bucket3d.h"

#include "lib/ivis_common/piemode.h"
#include "mission.h"

/*Remove this one!!! :-( */
#include "miscimd.h"
#include "effects.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "hci.h"
#include "lighting.h"
#include "console.h"
#include "loop.h"

#include "multiplay.h"
#include "game.h"

extern UWORD OffScreenEffects;

/* Our list of all game world effects */
static EFFECT        asEffectsList[MAX_EFFECTS];
static EFFECT_STATUS effectStatus[MAX_EFFECTS];

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

/* Tick counts for updates on a particular interval */
static	UDWORD	lastUpdateDroids[EFFECT_DROID_DIVISION];
static	UDWORD	lastUpdateStructures[EFFECT_STRUCTURE_DIVISION];

static	UDWORD	freeEffect; /* Current next slot to use - cyclic */
static	UDWORD	numEffects;
static	UDWORD	activeEffects;
static	UDWORD	missCount;
static	UDWORD	skipped,skippedEffects,letThrough;
static	UDWORD	auxVar; // dirty filthy hack - don't look for what this does.... //FIXME
static	UDWORD	auxVarSec; // dirty filthy hack - don't look for what this does.... //FIXME
static	UDWORD	aeCalls;
static	UDWORD	specifiedSize;
static  UDWORD	ellSpec;

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
static void	renderExplosionEffect	( EFFECT *psEffect );
static void	renderSmokeEffect		( EFFECT *psEffect );
static void	renderGravitonEffect	( EFFECT *psEffect );
static void	renderConstructionEffect( EFFECT *psEffect );
static void	renderWaypointEffect	( EFFECT *psEffect );
static void	renderBloodEffect		( EFFECT *psEffect );
static void	renderDestructionEffect	( EFFECT *psEffect );
static void renderFirework			( EFFECT *psEffect );
static void positionEffect(EFFECT *psEffect);
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
static void	effectSetUpSatLaser		( EFFECT *psEffect );
static void effectSetUpFirework		( EFFECT *psEffect );
#ifdef DEBUG
static BOOL	validatePie( EFFECT_GROUP group, iIMDShape *pie );
#endif
static void effectStructureUpdates(void);
static void effectDroidUpdates(void);

static void positionEffect(EFFECT *psEffect)
{
	Vector3i dv;
	SDWORD rx, rz;

	/* Establish world position */
	dv.x = (psEffect->position.x - player.p.x) - terrainMidX * TILE_UNITS;
	dv.y = psEffect->position.y;
	dv.z = terrainMidY * TILE_UNITS - (psEffect->position.z - player.p.z);

	/* Push the indentity matrix */
	iV_MatrixBegin();

	/* Move to position */
	iV_TRANSLATE(dv.x, dv.y, dv.z);

	/* Get the x,z translation components */
	rx = map_round(player.p.x);
	rz = map_round(player.p.z);

	/* Move to camera reference */
	iV_TRANSLATE(rx, 0, -rz);
}

static void killEffect(EFFECT *e)
{
	effectStatus[e-asEffectsList] = ES_INACTIVE;
	e->control = (UBYTE) 0;
}

// ----------------------------------------------------------------------------------------
static BOOL	essentialEffect(EFFECT_GROUP group, EFFECT_TYPE type)
{
	switch(group)
	{
	case	EFFECT_FIRE:
	case	EFFECT_WAYPOINT:
	case	EFFECT_DESTRUCTION:
	case	EFFECT_SAT_LASER:
	case	EFFECT_STRUCTURE:
		return(TRUE);
		break;
	case	EFFECT_EXPLOSION:
		if(type == EXPLOSION_TYPE_LAND_LIGHT)
		{
			return(TRUE);
		}
		else
		{
			return(FALSE);
		}
	default:
		return(FALSE);
		break;
	}
}

static BOOL utterlyReject( EFFECT_GROUP group )
{
	switch(group)
	{
	case EFFECT_BLOOD:
	case EFFECT_DUST_BALL:
	case EFFECT_CONSTRUCTION:
		return(TRUE);
	default:
		return(FALSE);
		break;
	}
}

// ----------------------------------------------------------------------------------------
/*	Simply sets the free pointer to the start - actually this isn't necessary
	as it will work just fine anyway. This WOULD be necessary were we to change
	the system so that it seeks FREE slots rather than the oldest one. This is because
	different effects last for different times and the oldest effect may have
	just got going (if it was a long effect).
*/
void	initEffectsSystem( void )
{
UDWORD	i;
EFFECT	*psEffect;


	/* Set position to first */
	freeEffect = 0;

	/* None are active */
	numEffects = 0;

	activeEffects = 0;

	missCount=0;

	skipped = letThrough = 0;

	for(i=0; i<MAX_EFFECTS; i++)
	{
		/* Get a pointer - just cos our macro requires it, speeds not an issue here */
		psEffect = &asEffectsList[i];
		/* Clear all the control bits */
		psEffect->control = (UBYTE)0;
		/* All effects are initially inactive */
		effectStatus[i] = ES_INACTIVE;
	}
}

// ----------------------------------------------------------------------------------------
void	effectSetLandLightSpec(LAND_LIGHT_SPEC spec)
{
	ellSpec = spec;
}
// ----------------------------------------------------------------------------------------
void	effectSetSize(UDWORD size)
{
	specifiedSize = size;
}
// ----------------------------------------------------------------------------------------
void	addMultiEffect(Vector3i *basePos, Vector3i *scatter, EFFECT_GROUP group,
					   EFFECT_TYPE type,BOOL specified, iIMDShape *imd, UDWORD number, BOOL lit, UDWORD size)
{
UDWORD	i;
Vector3i scatPos;


	if(number==0)
	{
		return;
	}
	/* Set up the scaling for specified ones */
	specifiedSize = size;

	/* If there's only one, make sure it's in the centre */
	if(number == 1)
	{
		scatPos.x = basePos->x;
		scatPos.y = basePos->y;
		scatPos.z = basePos->z;
		addEffect(&scatPos,group,type,specified,imd,lit);
	}
	else
	{
		/* Fix for jim */
		scatter->x/=10;
		scatter->y/=10;
		scatter->z/=10;

		/* There are multiple effects - so scatter them around according to parameter */
		for(i=0; i<number; i++)
		{
			scatPos.x = basePos->x + (scatter->x ? ( scatter->x	- (rand()%(2*scatter->x)) ) : 0 );
			scatPos.y = basePos->y + (scatter->y ? ( scatter->y	- (rand()%(2*scatter->y)) ) : 0 );
			scatPos.z = basePos->z + (scatter->z ? ( scatter->z	- (rand()%(2*scatter->z)) ) : 0 );
			addEffect(&scatPos,group,type,specified,imd,lit);
		}
	}
}

// ----------------------------------------------------------------------------------------
UDWORD	getNumActiveEffects( void )
{
	return(activeEffects);
}
// ----------------------------------------------------------------------------------------
UDWORD	getMissCount( void )
{
	return(missCount);
}

UDWORD	getNumSkippedEffects(void)
{
	return(skippedEffects);
}

UDWORD	getNumEvenEffects(void)
{
	return(letThrough);
}
// ----------------------------------------------------------------------------------------

UDWORD Reject1;

void	addEffect(Vector3i *pos, EFFECT_GROUP group, EFFECT_TYPE type,BOOL specified, iIMDShape *imd, BOOL lit)
{
	UDWORD	essentialCount;
	UDWORD	i;
	BOOL	bSmoke;

	aeCalls++;

	if(gamePaused())
	{
		return;
	}

	/* Quick optimsation to reject every second non-essential effect if it's off grid */
//	if(clipXY(pos->pos.x, pos->pos.z) == FALSE)
	if(clipXY(pos->x, pos->z) == FALSE)
	{
		/* 	If effect is essentail - then let it through */
	  	if(!essentialEffect(group,type) )
		{
			/* Some we can get rid of right away */
			if ( utterlyReject( group ) )
			{
				skipped++;
				return;
			}
			/* Smoke gets culled more than most off grid effects */
			if(group == EFFECT_SMOKE)
			{
				bSmoke = TRUE;
			}
			else
			{
				bSmoke = FALSE;
			}
			/* Others intermittently (50/50 for most and 25/100 for smoke */
			if(bSmoke ? (aeCalls & 0x03) : (aeCalls & 0x01) )
			{
				/* Do one */
				skipped++;
				return;
			}
			letThrough++;
		}
	}


	for(i=freeEffect,essentialCount=0; (asEffectsList[i].control & EFFECT_ESSENTIAL)
		&& essentialCount<MAX_EFFECTS; i++)
	{
		/* Check for wrap around */
		if(i>= (MAX_EFFECTS-1))
		{
			/* Go back to the first one */
			i = 0;
		}
		essentialCount++;
		missCount++;
	}

	/* Check the list isn't just full of essential effects */
	if(essentialCount>=MAX_EFFECTS)
	{
		/* All of the effects are essential!?!? */
		return;
	}
	else
	{
		freeEffect = i;
	}

	/* Store away it's position - into FRACTS */
	asEffectsList[freeEffect].position.x = (float)pos->x;
	asEffectsList[freeEffect].position.y = (float)pos->y;
	asEffectsList[freeEffect].position.z = (float)pos->z;



	/* Now, note group and type */
	asEffectsList[freeEffect].group =(UBYTE) group;
	asEffectsList[freeEffect].type = (UBYTE) type;

	/* Set when it entered the world */
	asEffectsList[freeEffect].birthTime = asEffectsList[freeEffect].lastFrame = gameTime;

	if(group == EFFECT_GRAVITON && (type == GRAVITON_TYPE_GIBLET || type == GRAVITON_TYPE_EMITTING_DR))
	{
		asEffectsList[freeEffect].frameNumber = lit;
	}

	else
	{
		/* Starts off on frame zero */
		asEffectsList[freeEffect].frameNumber = 0;
	}

	/*
		See what kind of effect it is - the add fucnction is different for each,
		although some things are shared
	*/
	asEffectsList[freeEffect].imd = NULL;
	if(lit)
	{
		SET_LITABS(asEffectsList[freeEffect]);
	}

	if(specified)
	{
		/* We're specifying what the imd is - override */
		asEffectsList[freeEffect].imd = imd;
//		if(type == EXPLOSION_TYPE_SPECIFIED_FIXME)
//		{
//			asEffectsList[freeEffect].size = EXPLOSION_SIZE;
//		}
//		else
//		{
		asEffectsList[freeEffect].size =(UWORD) specifiedSize;
//		}
	}

	/* Do all the effect type specific stuff */
	switch(group)
	{
		case EFFECT_SMOKE:
			effectSetupSmoke(&asEffectsList[freeEffect]);
			break;
		case EFFECT_GRAVITON:
			effectSetupGraviton(&asEffectsList[freeEffect]);
			break;
		case EFFECT_EXPLOSION:
			effectSetupExplosion(&asEffectsList[freeEffect]);
			break;
		case EFFECT_CONSTRUCTION:
			effectSetupConstruction(&asEffectsList[freeEffect]);
			break;
		case EFFECT_WAYPOINT:
			effectSetupWayPoint(&asEffectsList[freeEffect]);
			break;
		case EFFECT_BLOOD:
			effectSetupBlood(&asEffectsList[freeEffect]);
			break;
		case EFFECT_DESTRUCTION:
			effectSetupDestruction(&asEffectsList[freeEffect]);
			break;
		case EFFECT_FIRE:
			effectSetupFire(&asEffectsList[freeEffect]);
			break;
		case EFFECT_SAT_LASER:
			effectSetUpSatLaser(&asEffectsList[freeEffect]);
			break;
		case EFFECT_FIREWORK:
			effectSetUpFirework(&asEffectsList[freeEffect]);
			break;
		default:
			ASSERT( FALSE,"Weirdy group type for an effect" );
			break;
	}

	/* Make the effect active */
	effectStatus[freeEffect] = ES_ACTIVE;

	/* As of yet, it hasn't bounced (or whatever)... */
	if(type!=EXPLOSION_TYPE_LAND_LIGHT)
	{
		asEffectsList[freeEffect].specific = 0;
	}

	/* Looks like we didn't establish an imd for the effect */
	/*
	ASSERT( asEffectsList[freeEffect].imd != NULL || group == EFFECT_DESTRUCTION || group == EFFECT_FIRE || group == EFFECT_SAT_LASER,
		"null effect imd" );
	*/

#ifdef DEBUG
	if ( validatePie( group, asEffectsList[freeEffect].imd ) == FALSE )
	{
		ASSERT( FALSE,"No PIE found or specified for an effect" );
	}
#endif

	/* No more slots available? */
	if(freeEffect++ >= (MAX_EFFECTS-1))
	{
		/* Go back to the first one */
		freeEffect = 0;
	}
}

#ifdef DEBUG
// ----------------------------------------------------------------------------------------
static BOOL validatePie( EFFECT_GROUP group, iIMDShape *pie )
{

	/* If we haven't got a pie */
	if(pie == NULL)
	{
		if(group == EFFECT_DESTRUCTION || group == EFFECT_FIRE || group == EFFECT_SAT_LASER)
		{
			/* Ok in these cases */
			return(TRUE);
		}

		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}
// ----------------------------------------------------------------------------------------
#endif

/* Calls all the update functions for each different currently active effect */
void	processEffects(void)
{
	unsigned int i;

	/* Establish how long the last game frame took */
	missCount = 0;

	/* Reset counter */
	numEffects = 0;

	/* Traverse the list */
	for (i = 0; i < MAX_EFFECTS; i++)
	{
		/* Don't bother unless it's active */
		if(effectStatus[i] == ES_ACTIVE)
		{
			updateEffect(&asEffectsList[i]);
			/* One more is active */
			numEffects++;
			/* Is it on the grid */
			if (clipXY(asEffectsList[i].position.x, asEffectsList[i].position.z))
			{
				/* Add it to the bucket */
				bucketAddTypeToList(RENDER_EFFECT,&asEffectsList[i]);
			}

		}
	}

	/* Add any droid effects */
	effectDroidUpdates();

	/* Add any structure effects */
	effectStructureUpdates();

	activeEffects = numEffects;
	skippedEffects = skipped;
}

// ----------------------------------------------------------------------------------------
/* The general update function for all effects - calls a specific one for each */
static void updateEffect(EFFECT *psEffect)
{
	/* What type of effect are we dealing with? */
	switch(psEffect->group)
	{
	case EFFECT_EXPLOSION:
		updateExplosion(psEffect);
		break;

	case EFFECT_WAYPOINT:
		if(!gamePaused()) updateWaypoint(psEffect);
		break;

	case EFFECT_CONSTRUCTION:
		if(!gamePaused()) updateConstruction(psEffect);
		break;

	case EFFECT_SMOKE:
		if(!gamePaused()) updatePolySmoke(psEffect);
		break;

	case EFFECT_STRUCTURE:
		break;

	case EFFECT_GRAVITON:
		if(!gamePaused()) updateGraviton(psEffect);
		break;

	case EFFECT_BLOOD:
		if(!gamePaused()) updateBlood(psEffect);
		break;

	case EFFECT_DESTRUCTION:
		if(!gamePaused()) updateDestruction(psEffect);
		break;

	case EFFECT_FIRE:
		if(!gamePaused()) updateFire(psEffect);
		break;

	case EFFECT_SAT_LASER:
		if(!gamePaused()) updateSatLaser(psEffect);
		break;
	case EFFECT_FIREWORK:
		if(!gamePaused()) updateFirework(psEffect);
		break;
	default:
		debug( LOG_ERROR, "Weirdy class of effect passed to updateEffect" );
		abort();
		break;
	}
}

// ----------------------------------------------------------------------------------------
// ALL THE UPDATE FUNCTIONS
// ----------------------------------------------------------------------------------------
/* Update the waypoint effects.*/
static void updateWaypoint(EFFECT *psEffect)
{
	if(!(keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) ||
	     keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)))
	{
		killEffect(psEffect);
	}
}


// ----------------------------------------------------------------------------------------
static void updateFirework(EFFECT *psEffect)
{
	UDWORD	height;
	UDWORD	xDif,yDif,radius,val;
	Vector3i dv;
	SDWORD	dif;
	UDWORD	drop;

	/* Move it */
	psEffect->position.x += timeAdjustedIncrement(psEffect->velocity.x, TRUE);
	psEffect->position.y += timeAdjustedIncrement(psEffect->velocity.y, TRUE);
	psEffect->position.z += timeAdjustedIncrement(psEffect->velocity.z, TRUE);

	if(psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
		height = psEffect->position.y;
		if(height > psEffect->size)
		{
			dv.x = psEffect->position.x;
   			dv.z = psEffect->position.z;
   			dv.y = psEffect->position.y + (psEffect->radius / 2);
			addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
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
   					xDif = radius * (SIN(DEG(val)));
   					yDif = radius * (COS(DEG(val)));
   					xDif = xDif/4096;	 // cos it's fixed point
   					yDif = yDif/4096;
   					dv.x = psEffect->position.x + xDif;
   					dv.z = psEffect->position.z + yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,FALSE,NULL,0);
					dv.x = psEffect->position.x - xDif;
   					dv.z = psEffect->position.z - yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,FALSE,NULL,0);
					dv.x = psEffect->position.x + xDif;
   					dv.z = psEffect->position.z - yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,FALSE,NULL,0);
					dv.x = psEffect->position.x - xDif;
   					dv.z = psEffect->position.z + yDif;
   					dv.y = psEffect->position.y + dif;
					effectGiveAuxVar(100);
   					addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,FALSE,NULL,0);

					//   			dv.x = dv.x - (2*xDif);
	//   			dv.z = dv.z - (2*yDif);	// buildings are level!
	//			effectGiveAuxVar(100);
	//   			addEffect(&dv,EFFECT_FIREWORK, FIREWORK_TYPE_STARBURST,FALSE,NULL,0);
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
			addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,FALSE,NULL,0);
		}
	}
	else	// must be a startburst
	{
			/* Time to update the frame number on the smoke sprite */
		if(gameTime - psEffect->lastFrame > psEffect->frameDelay)
		{
			/* Store away last frame change time */
			psEffect->lastFrame = gameTime;

			/* Are we on the last frame? */
			if(++psEffect->frameNumber >= EffectGetNumFrames(psEffect))
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
			if(gameTime - psEffect->birthTime > psEffect->lifeSpan)
			{
				/* Kill it */
				killEffect(psEffect);
			}
		}


	}

}

// ----------------------------------------------------------------------------------------
static void updateSatLaser(EFFECT *psEffect)
{
	Vector3i dv;
	UDWORD	val;
	UDWORD	radius;
	UDWORD	xDif,yDif;
	UDWORD	i;
	UDWORD	startHeight,endHeight;
	iIMDShape	*pie;
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
		pie = getImdFromIndex(MI_FLAME);

		/* Add some big explosions....! */

		for(i=0; i<16; i++)
		{
			dv.x = xPos+(200-rand()%400);
			dv.z = yPos+(200-rand()%400);
			dv.y = startHeight + rand()%100;
			addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
		}
		/* Add a sound effect */
  		audio_PlayStaticTrack(psEffect->position.x, psEffect->position.z, ID_SOUND_EXPLOSION);

		/* Add a shockwave */
		dv.x = xPos;
		dv.z = yPos;
		dv.y = startHeight+SHOCK_WAVE_HEIGHT;
		addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_SHOCKWAVE,FALSE,NULL,0);


		/* Now, add the column of light */
		for(i=startHeight; i<endHeight; i+=56)
		{
			radius = 80;
			/* Add 36 around in a circle..! */
			for(val = 0; val<=180; val+=30)
			{
   				xDif = radius * (SIN(DEG(val)));
   				yDif = radius * (COS(DEG(val)));
   				xDif = xDif/4096;	 // cos it's fixed point
   				yDif = yDif/4096;
   				dv.x = xPos+xDif+i/64;
   				dv.z = yPos+yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
				dv.x = xPos-xDif+i/64;
   				dv.z = yPos-yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
				dv.x = xPos+xDif+i/64;
   				dv.z = yPos-yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
				dv.x = xPos-xDif+i/64;
   				dv.z = yPos+yDif;
   				dv.y = startHeight+i;
				effectGiveAuxVar(100);
   				addEffect(&dv,EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
			}
		}
	}

	if(gameTime-psEffect->birthTime < 1000)
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

// ----------------------------------------------------------------------------------------
/* The update function for the explosions */
static void updateExplosion(EFFECT *psEffect)
{
	LIGHT light;
	UDWORD percent;
	UDWORD range;
	float scaling;

	if(TEST_LIT(psEffect))
	{
		if(psEffect->lifeSpan)
		{
			percent = PERCENT(gameTime-psEffect->birthTime,psEffect->lifeSpan);
			if(percent > 100)
			{
				percent = 100;
			}
			else
			{
				if(percent>50)
				{
					percent = 100-percent;
				}
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

/*
	if(psEffect->type == EXPLOSION_TYPE_LAND_LIGHT)
	{
		light.position.x = psEffect->position.x;
		light.position.y = psEffect->position.y;
		light.position.z = psEffect->position.z;
		light.range = getTimeValueRange(1024,512);
		if(light.range>256) light.range = 512-light.range;
		light.colour = LIGHT_RED;
		processLight(&light);
	}
*/

	if(psEffect->type == EXPLOSION_TYPE_SHOCKWAVE)
	{
		psEffect->size += timeAdjustedIncrement(SHOCKWAVE_SPEED, TRUE);
		scaling = (float)psEffect->size / (float)MAX_SHOCKWAVE_SIZE;
		psEffect->frameNumber = scaling * EffectGetNumFrames(psEffect);

		light.position.x = psEffect->position.x;
		light.position.y = psEffect->position.y;
		light.position.z = psEffect->position.z;
		light.range = psEffect->size+200;
		light.colour = LIGHT_YELLOW;
		processLight(&light);

		if(psEffect->size>MAX_SHOCKWAVE_SIZE || light.range>600)
		{
 			/* Kill it off */
			killEffect(psEffect);
			return;

		}
	}

	/* Time to update the frame number on the explosion */
	else
		if(gameTime - psEffect->lastFrame > psEffect->frameDelay)
		{
			psEffect->lastFrame = gameTime;
			/* Are we on the last frame? */

			if(++psEffect->frameNumber >= EffectGetNumFrames(psEffect))
			{
		 		if(psEffect->type!=EXPLOSION_TYPE_LAND_LIGHT)
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
			psEffect->position.y += timeAdjustedIncrement(psEffect->velocity.y, TRUE);
		}

	}
}

// ----------------------------------------------------------------------------------------
/* The update function for blood */
static void updateBlood(EFFECT *psEffect)
{
	/* Time to update the frame number on the blood */
	if(gameTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = gameTime;
		/* Are we on the last frame? */
		if(++psEffect->frameNumber >= EffectGetNumFrames(psEffect))
		{
			/* Kill it off */
			killEffect(psEffect);
			return;
		}
	}
	/* Move it about in the world */
	psEffect->position.x += timeAdjustedIncrement(psEffect->velocity.x, TRUE);
	psEffect->position.y += timeAdjustedIncrement(psEffect->velocity.y, TRUE);
	psEffect->position.z += timeAdjustedIncrement(psEffect->velocity.z, TRUE);
}

// ----------------------------------------------------------------------------------------
/* Processes all the drifting smoke
	Handles the smoke puffing out the factory as well */
static void updatePolySmoke(EFFECT *psEffect)
{

	/* Time to update the frame number on the smoke sprite */
	if(gameTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		/* Store away last frame change time */
		psEffect->lastFrame = gameTime;

		/* Are we on the last frame? */
		if(++psEffect->frameNumber >= EffectGetNumFrames(psEffect))
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
	psEffect->position.x += timeAdjustedIncrement(psEffect->velocity.x, TRUE);
	psEffect->position.y += timeAdjustedIncrement(psEffect->velocity.y, TRUE);
	psEffect->position.z += timeAdjustedIncrement(psEffect->velocity.z, TRUE);

	/* If it doesn't get killed by frame number, then by age */
	if(TEST_CYCLIC(psEffect))
	{
		/* Has it overstayed it's welcome? */
		if(gameTime - psEffect->birthTime > psEffect->lifeSpan)
		{
			/* Kill it */
			killEffect(psEffect);
		}
	}
}

// ----------------------------------------------------------------------------------------
/*
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

		psEffect->position.x += timeAdjustedIncrement(psEffect->velocity.x, TRUE);
		psEffect->position.y += timeAdjustedIncrement(psEffect->velocity.y, TRUE);
		psEffect->position.z += timeAdjustedIncrement(psEffect->velocity.z, TRUE);

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
		if(gameTime>psEffect->lastFrame+psEffect->frameDelay)
		{
			/* Store away last update */
			psEffect->lastFrame = gameTime;

			/* Add an effect at the gravitons's position */
			dv.x = psEffect->position.x;
			dv.y = psEffect->position.y;
			dv.z = psEffect->position.z;

			/* Add a trail graphic */
			addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,FALSE,NULL,0);
		}
	}

	else if(psEffect->type == GRAVITON_TYPE_GIBLET && (psEffect->position.y>(groundHeight+5)))
	{
		/* Time to add another trail 'thing'? */
		if(gameTime>psEffect->lastFrame+psEffect->frameDelay)
		{
			/* Store away last update */
			psEffect->lastFrame = gameTime;

			/* Add an effect at the gravitons's position */
			dv.x = psEffect->position.x;
			dv.y = psEffect->position.y;
			dv.z = psEffect->position.z;
			addEffect(&dv,EFFECT_BLOOD,BLOOD_TYPE_NORMAL,FALSE,NULL,0);
		}
	}

	/* Spin it round a bit */
	psEffect->rotation.x += timeAdjustedIncrement(psEffect->spin.x, TRUE);
	psEffect->rotation.y += timeAdjustedIncrement(psEffect->spin.y, TRUE);
	psEffect->rotation.z += timeAdjustedIncrement(psEffect->spin.z, TRUE);

	/* Update velocity (and retarding of descent) according to present frame rate */
	accel = timeAdjustedIncrement(GRAVITON_GRAVITY, TRUE);
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
					addEffect(&dv,EFFECT_EXPLOSION,EXPLOSION_TYPE_VERY_SMALL,FALSE,NULL,0);
				}
				killEffect(psEffect);
				return;
			}
		}
	}
}


// ----------------------------------------------------------------------------------------
/* updateDestruction
This isn't really an on-screen effect itself - it just spawns other ones....
  */
static void updateDestruction(EFFECT *psEffect)
{
	Vector3i pos;
	UDWORD	effectType;
	UDWORD	widthScatter = 0, breadthScatter = 0, heightScatter = 0;
	SDWORD	iX, iY;
	LIGHT	light;
	UDWORD	percent;
	UDWORD	range;
	float	div;
	UDWORD	height;

	percent = PERCENT(gameTime-psEffect->birthTime,psEffect->lifeSpan);
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

	if(gameTime > (psEffect->birthTime + psEffect->lifeSpan))
	{
		/* Kill it - it's too old */
		killEffect(psEffect);
		return;
	}

	if(psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
	{

		if((gameTime - psEffect->birthTime) > (unsigned int)((9*psEffect->lifeSpan)/10))
		{
			pos.x = psEffect->position.x;
			pos.z = psEffect->position.z;
			pos.y = psEffect->position.y;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LARGE,FALSE,NULL,0);
			killEffect(psEffect);
			return;
		}

		div = 1.f - (float)(gameTime - psEffect->birthTime) / psEffect->lifeSpan;
		if(div < 0.f)
			div = 0.f;
		height = div * psEffect->imd->max.y;
	}
	else
	{
		height = 16;
	}


	/* Time to add another effect? */
	if((gameTime - psEffect->lastFrame) > psEffect->frameDelay)
	{
		psEffect->lastFrame = gameTime;
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
			ASSERT( FALSE,"Weirdy destruction type effect" );
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
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,FALSE,NULL,0);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			if(psEffect->type == DESTRUCTION_TYPE_SKYSCRAPER)
			{
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LARGE,FALSE,NULL,0);
			}
			/* Only structures get the big explosions */
			else if(psEffect->type==DESTRUCTION_TYPE_STRUCTURE)
			{
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
			}
			else
			{
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
			}
			break;
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			if(psEffect->type == DESTRUCTION_TYPE_STRUCTURE)
			{
				addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_ST,TRUE,getRandomDebrisImd(),0);
			}
			else
			{
				addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,getRandomDebrisImd(),0);
			}
			break;
		case 11:
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,FALSE,NULL,0);
			break;
		case 12:
		case 13:
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
			break;
		case 14:
			/* Add sound effect, but only if we're less than 3/4 of the way thru' destruction */
			if( gameTime < ((3*(psEffect->birthTime + psEffect->lifeSpan)/4)) )
			{
				iX = psEffect->position.x;
				iY = psEffect->position.z;
				audio_PlayStaticTrack( iX, iY, ID_SOUND_EXPLOSION );
			}
			break;

		}
	}
}

// ----------------------------------------------------------------------------------------
/*
updateConstruction:-
Moves the construction graphic about - dust cloud or whatever....
*/
static void updateConstruction(EFFECT *psEffect)
{

	/* Time to update the frame number on the construction sprite */
	if(gameTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = gameTime;
		/* Are we on the last frame? */
		if(++psEffect->frameNumber >= EffectGetNumFrames(psEffect))
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

	psEffect->position.x += timeAdjustedIncrement(psEffect->velocity.x, TRUE);
	psEffect->position.y += timeAdjustedIncrement(psEffect->velocity.y, TRUE);
	psEffect->position.z += timeAdjustedIncrement(psEffect->velocity.z, TRUE);


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

		if(gameTime - psEffect->birthTime > psEffect->lifeSpan)
		{
			killEffect(psEffect);
			return;
		}
	}
}

// ----------------------------------------------------------------------------------------
/* Update fire sequences */
static void updateFire(EFFECT *psEffect)
{
	Vector3i pos;
	LIGHT	light;
	UDWORD	percent;

	percent = PERCENT(gameTime-psEffect->birthTime,psEffect->lifeSpan);
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
	if(gameTime - psEffect->lastFrame > psEffect->frameDelay)
	{
		psEffect->lastFrame = gameTime;
		pos.x = (psEffect->position.x + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
		pos.z = (psEffect->position.z + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
		pos.y = map_Height(pos.x,pos.z);

		if(psEffect->type == FIRE_TYPE_SMOKY_BLUE)
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,FALSE,NULL,0);
		}
		else
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
		}

		if(psEffect->type == FIRE_TYPE_SMOKY || psEffect->type == FIRE_TYPE_SMOKY_BLUE)
		{
			pos.x = (psEffect->position.x + ((rand() % psEffect->radius / 2) - (rand() % (2 * psEffect->radius / 2))));
			pos.z = (psEffect->position.z + ((rand() % psEffect->radius / 2) - (rand() % (2 * psEffect->radius / 2))));
			pos.y = map_Height(pos.x,pos.z);
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING_HIGH,FALSE,NULL,0);
		}
		else
		{
			pos.x = (psEffect->position.x + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
			pos.z = (psEffect->position.z + ((rand() % psEffect->radius) - (rand() % (2 * psEffect->radius))));
			pos.y = map_Height(pos.x,pos.z);
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
		}

		/*
		pos.x = psEffect->position.x;
		pos.y = psEffect->position.y;
		pos.z = psEffect->position.z;

		scatter.x = psEffect->radius; scatter.y = 0; scatter.z = psEffect->radius;
		addMultiEffect(&pos,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,2,0,0);
		*/

	}

	if(gameTime - psEffect->birthTime > psEffect->lifeSpan)
	{
		killEffect(psEffect);
		return;
	}
}

// ----------------------------------------------------------------------------------------
// ALL THE RENDER FUNCTIONS
// ----------------------------------------------------------------------------------------
/*
renderEffect:-
Calls the appropriate render routine for each type of effect
*/
void	renderEffect(EFFECT *psEffect)
{
		/* What type of effect are we dealing with? */
	switch(psEffect->group)
	{
	case EFFECT_WAYPOINT:
		renderWaypointEffect(psEffect);
		break;

	case EFFECT_EXPLOSION:
		renderExplosionEffect(psEffect);
		break;

	case EFFECT_CONSTRUCTION:
		renderConstructionEffect(psEffect);
		break;

	case EFFECT_SMOKE:
		renderSmokeEffect(psEffect);
		break;

	case EFFECT_GRAVITON:
		renderGravitonEffect(psEffect);
		break;

	case EFFECT_BLOOD:
		renderBloodEffect(psEffect);
		break;

	case EFFECT_STRUCTURE:
		break;

	case EFFECT_DESTRUCTION:
		/*	There is no display func for a destruction effect -
			it merely spawn other effects over time */
		renderDestructionEffect(psEffect);
		break;
	case EFFECT_FIRE:
		/* Likewise */
		break;
	case EFFECT_SAT_LASER:
		/* Likewise */
		break;
	case EFFECT_FIREWORK:
		renderFirework(psEffect);
		break;
	default:
		debug( LOG_ERROR, "Weirdy class of effect passed to renderEffect" );
		abort();
		break;
	}
}

// ----------------------------------------------------------------------------------------
/* drawing func for wapypoints . AJL. */
void	renderWaypointEffect(EFFECT *psEffect)
{
	positionEffect(psEffect);

	pie_Draw3DShape(psEffect->imd, 0, 0, WZCOL_WHITE, WZCOL_BLACK, 0, 0);
	iV_MatrixEnd();
}

// ----------------------------------------------------------------------------------------
void	renderFirework(EFFECT *psEffect)
{
	/* these don't get rendered */
	if(psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
		return;
	}

	positionEffect(psEffect);

	iV_MatrixRotateY(-player.r.y);
	iV_MatrixRotateX(-player.r.x);

	pie_MatScale(psEffect->size);
 	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, WZCOL_WHITE, WZCOL_BLACK, pie_ADDITIVE, EFFECT_EXPLOSION_ADDITIVE);
 	iV_MatrixEnd();
}

// ----------------------------------------------------------------------------------------
/* drawing func for blood. */
void	renderBloodEffect(EFFECT *psEffect)
{
	positionEffect(psEffect);

	iV_MatrixRotateY(-player.r.y);
	iV_MatrixRotateX(-player.r.x);
	pie_MatScale(psEffect->size);

	pie_Draw3DShape(getImdFromIndex(MI_BLOOD), psEffect->frameNumber, 0, WZCOL_WHITE, WZCOL_BLACK, pie_TRANSLUCENT, EFFECT_BLOOD_TRANSPARENCY);
	iV_MatrixEnd();
}

// ----------------------------------------------------------------------------------------
void	renderDestructionEffect(EFFECT *psEffect)
{
	float	div;
	SDWORD	percent;

	if(psEffect->type!=DESTRUCTION_TYPE_SKYSCRAPER)
	{
		return;
	}

	positionEffect(psEffect);

	div = (float)(gameTime - psEffect->birthTime) / psEffect->lifeSpan;
	if(div>1.0)	div = 1.0;	//temporary!
	{
		div = 1.0 - div;
		percent = (SDWORD)(div*pie_RAISE_SCALE);
	}

	if(!gamePaused())
	{
 		iV_MatrixRotateX(SKY_SHIMMY);
 		iV_MatrixRotateY(SKY_SHIMMY);
 		iV_MatrixRotateZ(SKY_SHIMMY);
	}
 	pie_Draw3DShape(psEffect->imd, 0, 0, WZCOL_WHITE, WZCOL_BLACK, pie_RAISE, percent);

	iV_MatrixEnd();
}

// ----------------------------------------------------------------------------------------
static BOOL	rejectLandLight(LAND_LIGHT_SPEC type)
{
UDWORD	timeSlice;

	timeSlice = gameTime%2000;
	if(timeSlice<400)
	{
		if(type == LL_MIDDLE) return(FALSE); else return(TRUE);	// reject all expect middle
	}
	else if(timeSlice<800)
	{
		if(type == LL_OUTER) return(TRUE); else return(FALSE);	// reject only outer
	}
	else if(timeSlice<1200)
	{
		return(FALSE);	//reject none
	}
	else if(timeSlice<1600)
	{
		if(type == LL_OUTER) return(TRUE); else return(FALSE);	// reject only outer
	}
	else
	{
		if(type == LL_MIDDLE) return(FALSE); else return(TRUE);	// reject all expect middle
	}
}

// ----------------------------------------------------------------------------------------
/* Renders the standard explosion effect */
void	renderExplosionEffect(EFFECT *psEffect)
{
	SDWORD	percent;
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
/*		TEST_FLIPPED_Y(psEffect) ? iV_MatrixRotateY(-player.r.y+iV_DEG(180)) :*/ iV_MatrixRotateY(-player.r.y);
/*		TEST_FLIPPED_X(psEffect) ? iV_MatrixRotateX(-player.r.x+iV_DEG(180)) :*/ iV_MatrixRotateX(-player.r.x);
	}

	/* Tesla explosions diminish in size */
	if(psEffect->type == EXPLOSION_TYPE_TESLA)
	{
		percent = PERCENT((gameTime - psEffect->birthTime),psEffect->lifeSpan);
		if(percent<0) percent = 0;
		if(percent>45) percent = 45;
		pie_MatScale(psEffect->size - percent);
	}
	else if(psEffect->type == EXPLOSION_TYPE_PLASMA)
	{
		percent = PERCENT(gameTime - psEffect->birthTime, psEffect->lifeSpan) / 3;
		pie_MatScale(BASE_PLASMA_SIZE + percent);
	}
	else
	{
		pie_MatScale(psEffect->size);
	}

	if(psEffect->type == EXPLOSION_TYPE_PLASMA)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, WZCOL_BLACK, pie_ADDITIVE, EFFECT_PLASMA_ADDITIVE);
	}
	else if(psEffect->type == EXPLOSION_TYPE_KICKUP)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, WZCOL_BLACK, pie_TRANSLUCENT, 128);
	}
	else
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, WZCOL_BLACK, pie_ADDITIVE, EFFECT_EXPLOSION_ADDITIVE);
	}

	iV_MatrixEnd();
}

// ----------------------------------------------------------------------------------------
void	renderGravitonEffect(EFFECT *psEffect)
{

	positionEffect(psEffect);

	iV_MatrixRotateX(psEffect->rotation.x);
	iV_MatrixRotateY(psEffect->rotation.y);
	iV_MatrixRotateZ(psEffect->rotation.z);

	/* Buildings emitted by gravitons are chunkier */
	if(psEffect->type == GRAVITON_TYPE_EMITTING_ST)
	{
		/* Twice as big - 150 percent */
		pie_MatScale(psEffect->size);
	}
	else
	{
		pie_MatScale(100);
	}

	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, WZCOL_WHITE, WZCOL_BLACK, 0, 0);

	/* Pop the matrix */
	iV_MatrixEnd();
}

// ----------------------------------------------------------------------------------------
/*
renderConstructionEffect:-
Renders the standard construction effect */
void	renderConstructionEffect(EFFECT *psEffect)
{
	Vector3i null;
	SDWORD	percent;
	UDWORD	translucency;
	UDWORD	size;

	/* No rotation about arbitrary axis */
	null.x = null.y = null.z = 0;

	positionEffect(psEffect);

	/* Bit in comments doesn't quite work yet? */
	if(TEST_FACING(psEffect))
	{
/*		TEST_FLIPPED_Y(psEffect) ? iV_MatrixRotateY(-player.r.y+iV_DEG(180)) :*/ iV_MatrixRotateY(-player.r.y);
/*		TEST_FLIPPED_X(psEffect) ? iV_MatrixRotateX(-player.r.x+iV_DEG(180)) :*/ iV_MatrixRotateX(-player.r.x);
	}

	/* Scale size according to age */
	percent = PERCENT(gameTime - psEffect->birthTime, psEffect->lifeSpan);
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
	size = 2*translucency;
	if(size>90) size = 90;
	pie_MatScale(size);

	pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, WZCOL_WHITE, WZCOL_BLACK, pie_TRANSLUCENT, (UBYTE)(translucency));

	/* Pop the matrix */
	iV_MatrixEnd();
}

// ----------------------------------------------------------------------------------------
/*
renderSmokeEffect:-
Renders the standard smoke effect - it is now scaled in real-time as well
*/
void	renderSmokeEffect(EFFECT *psEffect)
{
	UDWORD	transparency = 0;
	const PIELIGHT brightness = WZCOL_WHITE;
	const PIELIGHT specular = WZCOL_BLACK;

	positionEffect(psEffect);

	/* Bit in comments doesn't quite work yet? */
	if(TEST_FACING(psEffect))
	{
 		/* Always face the viewer! */
/*		TEST_FLIPPED_Y(psEffect) ? iV_MatrixRotateY(-player.r.y+iV_DEG(180)) : */iV_MatrixRotateY(-player.r.y);
/*		TEST_FLIPPED_X(psEffect) ? iV_MatrixRotateX(-player.r.x+iV_DEG(180)) : */iV_MatrixRotateX(-player.r.x);
	}

	/* Small smoke - used for the droids */
//		if(psEffect->type == SMOKE_TYPE_DRIFTING_SMALL || psEffect->type == SMOKE_TYPE_TRAIL)

	if(TEST_SCALED(psEffect))
	{
		const unsigned int lifetime = gameTime - psEffect->birthTime;
		unsigned int percent;

		// Since psEffect->birthTime will be set to gameTime on
		// creation, and gameTime only increments, birthTime should be
		// smaller than or equal to gameTime. As a great man once said
		// though (hehe, I just love exaggarating ;-) -- Giel):
		// "Always assert your assumptions!". So here it goes:
		assert(psEffect->birthTime <= gameTime);

		ASSERT(psEffect->lifeSpan != 0, "Effect lifespan is zero (seconds); it really should be non-zero!");

		// HACK: Work around a bug that bit me causing a "integer divide by zero" at this location -- Giel
		if (psEffect->lifeSpan != 0)
			percent = (lifetime * 100) / psEffect->lifeSpan;
		else
			percent = 100;


		pie_MatScale(percent + psEffect->baseScale);
		transparency = (EFFECT_SMOKE_TRANSPARENCY * (100 - percent))/100;
	}

	transparency = (transparency * 3) / 2;  //JPS smoke strength increased for d3d 12 may 99

	/* Make imds be transparent on 3dfx */
	if(psEffect->type==SMOKE_TYPE_STEAM)
	{
		pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, specular, pie_TRANSLUCENT, (UBYTE)(EFFECT_STEAM_TRANSPARENCY)/2);
	}
	else
	{
		if(psEffect->type == SMOKE_TYPE_TRAIL)
		{
			pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, specular, pie_TRANSLUCENT, (UBYTE)((2*transparency)/3));
		}
		else
		{
			pie_Draw3DShape(psEffect->imd, psEffect->frameNumber, 0, brightness, specular, pie_TRANSLUCENT, (UBYTE)(transparency)/2);
		}
	}

	/* Pop the matrix */
	iV_MatrixEnd();
}


// ----------------------------------------------------------------------------------------
// ALL THE SETUP FUNCTIONS
// ----------------------------------------------------------------------------------------
void	effectSetUpFirework(EFFECT *psEffect)
{
	UDWORD	camExtra;

	if(psEffect->type == FIREWORK_TYPE_LAUNCHER)
	{
	 	psEffect->velocity.x = 200 - rand()%400;
		psEffect->velocity.z = 200 - rand()%400;
		psEffect->velocity.y = 400 + rand()%200;	//height
		psEffect->lifeSpan = GAME_TICKS_PER_SEC * 3;
		psEffect->radius = 80 + rand()%150;
		camExtra = 0;
		if(getCampaignNumber()!=1)
		{
			camExtra+=rand()%200;
		}
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

// ----------------------------------------------------------------------------------------
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
		ASSERT( FALSE,"Weird smoke type" );
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

// ----------------------------------------------------------------------------------------
void effectSetUpSatLaser(EFFECT *psEffect)
{
	/* Does nothing at all..... Runs only for one frame! */
	psEffect->baseScale = 1;
	return;
}

// ----------------------------------------------------------------------------------------
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
		ASSERT( FALSE,"Weirdy type of graviton" );
		break;

	}

	psEffect->rotation.x = DEG((rand()%360));
	psEffect->rotation.z = DEG((rand()%360));
	psEffect->rotation.y = DEG((rand()%360));

	psEffect->spin.x = DEG((rand()%100)+20);
	psEffect->spin.z = DEG((rand()%100)+20);
	psEffect->spin.y = DEG((rand()%100)+20);

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

// ----------------------------------------------------------------------------------------
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
		psEffect->frameDelay = (UWORD)(EXPLOSION_FRAME_DELAY/2);
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

// ----------------------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------------------
void	effectSetupFire(EFFECT *psEffect)
{
	psEffect->frameDelay = 300;	   // needs to be investigated...
	psEffect->radius = auxVar;	// needs to be investigated
	psEffect->lifeSpan = (UWORD)auxVarSec;
	psEffect->birthTime = gameTime;
	SET_ESSENTIAL(psEffect);

}

// ----------------------------------------------------------------------------------------
void	effectSetupWayPoint(EFFECT *psEffect)
{
	psEffect->imd = pProximityMsgIMD;

	/* These effects musnt make way for others */
	SET_ESSENTIAL(psEffect);
}

// ----------------------------------------------------------------------------------------
void	effectSetupBlood(EFFECT *psEffect)
{
	psEffect->frameDelay = BLOOD_FRAME_DELAY;
	psEffect->velocity.y = (float)BLOOD_FALL_SPEED;
	psEffect->imd = getImdFromIndex(MI_BLOOD);
	psEffect->size = (UBYTE)BLOOD_SIZE;
}

// ----------------------------------------------------------------------------------------
void    effectSetupDestruction(EFFECT *psEffect)
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

#define FX_PER_EDGE 6
#define	SMOKE_SHIFT	(16 - (rand()%32))
// ----------------------------------------------------------------------------------------
void	initPerimeterSmoke(iIMDShape *pImd, UDWORD x, UDWORD y, UDWORD z)
{
	SDWORD	i;
	SDWORD	inStart, inEnd;
	SDWORD	varStart, varEnd, varStride;
	SDWORD	shift = 0;
	Vector3i base;
	Vector3i pos;

	base.x = x;
	base.y = y;
	base.z = z;

	varStart = pImd->min.x -16;
	varEnd = pImd->max.x + 16;
	varStride = 24;//(varEnd-varStart)/FX_PER_EDGE;

	inStart = pImd->min.z - 16;
	inEnd = pImd->max.z + 16;

	for(i=varStart; i<varEnd; i+=varStride)
	{
		shift = SMOKE_SHIFT;
		pos.x = base.x + i + shift;
		pos.y = base.y;
		pos.z = base.z + inStart + shift;
		if(rand()%6==1)
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
		}
		else
		{
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_BILLOW,FALSE,NULL,0);
		}

		pos.x = base.x + i + shift;
		pos.y = base.y;
		pos.z = base.z + inEnd + shift;
		if(rand()%6==1)
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
		}
		else
		{
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_BILLOW,FALSE,NULL,0);
		}

	}


	varStart = pImd->min.z - 16;
	varEnd = pImd->max.z + 16;
	varStride = 24;//(varEnd-varStart)/FX_PER_EDGE;

	inStart = pImd->min.x - 16;
	inEnd = pImd->max.x + 16;


	for(i=varStart; i<varEnd; i+=varStride)
	{
		pos.x = base.x + inStart + shift;
		pos.y = base.y;
		pos.z = base.z + i + shift;
		if(rand()%6==1)
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
		}
		else
		{
	   		addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_BILLOW,FALSE,NULL,0);
		}


		pos.x = base.x + inEnd + shift;
		pos.y = base.y;
		pos.z = base.z + i + shift;
		if(rand()%6==1)
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
		}
		else
		{
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_BILLOW,FALSE,NULL,0);
		}
	}
}

// ----------------------------------------------------------------------------------------
UDWORD	getNumEffects( void )
{
	return(numEffects);
}


// ----------------------------------------------------------------------------------------
UDWORD EffectGetNumFrames(EFFECT *psEffect)
{

	return psEffect->imd->numFrames;

}

UDWORD IMDGetNumFrames(iIMDShape *Shape)
{

	return Shape->numFrames;

}

UDWORD IMDGetAnimInterval(iIMDShape *Shape)
{

	return Shape->animInterval;

}

void	effectGiveAuxVar( UDWORD var)
{
	auxVar = var;
}

void	effectGiveAuxVarSec( UDWORD var)
{
	auxVarSec = var;
}

// ----------------------------------------------------------------------------------------
/* Runs all the spot effect stuff for the droids - adding of dust and the like... */
static void effectDroidUpdates(void)
{
	UDWORD	i;
	DROID	*psDroid;
	UDWORD	partition;
	Vector3i pos;
	SDWORD	xBehind,yBehind;

	/* Go through all players */
	for(i=0; i<MAX_PLAYERS; i++)
	{
		/* Now go through all their droids */
		for(psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			/* Gets it's group number */
			partition = psDroid->id % EFFECT_DROID_DIVISION;
			/* Right frame to process? */
			if(partition == frameGetFrameNumber() % EFFECT_DROID_DIVISION && ONEINFOUR)
			{
				/* Sufficent time since last update? - The EQUALS comparison is needed */
				if(gameTime >= (lastUpdateDroids[partition] + DROID_UPDATE_INTERVAL))
				{
					/* Store away when we last processed this group */
					lastUpdateDroids[partition] = gameTime;

					/*	Now add some dust at it's arse end if it's moving or skidding.
						The check that it's not 0 is probably not sufficient.
					*/
					if( (SDWORD)psDroid->sMove.speed != 0 )
					{
				   		/* Present direction is important */
						xBehind = ( ( 50 * iV_SIN( DEG( (int)psDroid->direction) ) ) >> FP12_SHIFT );
						yBehind = ( ( 50 * iV_COS( DEG( (int)psDroid->direction) ) ) >> FP12_SHIFT );
						pos.x = psDroid->pos.x - xBehind;
						pos.z = psDroid->pos.y - yBehind;
						pos.y = map_Height(pos.x, pos.z);
//						addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,FALSE,NULL);
					}
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------------------
/* Runs all the structure effect stuff - steam puffing out etc */
static void effectStructureUpdates(void)
{
	UDWORD		i;
	UDWORD		partition;
	STRUCTURE	*psStructure;
	Vector3i eventPos;
	UDWORD		capacity;
	POWER_GEN	*psPowerGen;
	BOOL		active;

	/* Go thru' all players */
	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(psStructure = apsStructLists[i]; psStructure; psStructure = psStructure->psNext)
		{
			/* Find it's group */
			partition = psStructure->id % EFFECT_STRUCTURE_DIVISION;
			/* Is it the right frame? */
			if(partition == frameGetFrameNumber() % EFFECT_STRUCTURE_DIVISION)
			{
				/* Is it the right time? */
				if(gameTime > (lastUpdateStructures[partition] + STRUCTURE_UPDATE_INTERVAL))
				{
					/* Store away the last update time */
					lastUpdateStructures[partition] = gameTime;
					// -------------------------------------------------------------------------------
					/* Factories puff out smoke, power stations puff out tesla stuff */

				 	if( (psStructure->pStructureType->type == REF_FACTORY) ||
						(psStructure->pStructureType->type == REF_POWER_GEN) )
					if( (bMultiPlayer && isHumanPlayer(psStructure->player))
						|| (psStructure->player == 0) )

					if(psStructure->status==SS_BUILT)
					if(psStructure->visible[selectedPlayer])
					{
						/*	We're a factory, so better puff out a bit of steam
							Complete hack with the magic numbers - just for IAN demo
						*/
					if(psStructure->pStructureType->type == REF_FACTORY)
						{
							if (psStructure->sDisplay.imd->nconnectors == 1)
							{
								eventPos.x = psStructure->pos.x+psStructure->sDisplay.imd->connectors->x;
								eventPos.z = psStructure->pos.y-psStructure->sDisplay.imd->connectors->y;
								eventPos.y = psStructure->pos.z+psStructure->sDisplay.imd->connectors->z;
								addEffect(&eventPos,EFFECT_SMOKE,SMOKE_TYPE_STEAM,FALSE,NULL,0);


								if(selectedPlayer == psStructure->player)
								{
									audio_PlayObjStaticTrack( (void *) psStructure, ID_SOUND_STEAM );
								}

							}
						}
						else if(psStructure->pStructureType->type == REF_POWER_GEN)
						{
							psPowerGen = &psStructure->pFunctionality->powerGenerator;
							eventPos.x = psStructure->pos.x;
							eventPos.z = psStructure->pos.y;
							if (psStructure->sDisplay.imd->nconnectors > 0)
							{
								eventPos.y = psStructure->pos.z+psStructure->sDisplay.imd->connectors->z;
							}
							else
							{
								eventPos.y = psStructure->pos.z;
							}
							capacity = psPowerGen->capacity;
							/*if(capacity)
							{
								eventPos.y = psStructure->pos.z + 48;
							}*/
							/* Add an effect over the central spire - if
							connected to Res Extractor and it is active*/
							//look through the list to see if any connected Res Extr
							active = FALSE;
							for (i=0; i < NUM_POWER_MODULES; i++)
							{
								if (psPowerGen->apResExtractors[i]
								 && psPowerGen->apResExtractors[i]->pFunctionality->resourceExtractor.active)
								{
									active = TRUE;
									break;
								}
							}
							/*
							if (((POWER_GEN*)psStructure->pFunctionality)->
								apResExtractors[0] && ((RES_EXTRACTOR *)((POWER_GEN*)
								psStructure->pFunctionality)->apResExtractors[0]->
								pFunctionality)->active)
							*/
							//if (active)
							{
								eventPos.y = psStructure->pos.z + 48;
								addEffect(&eventPos,EFFECT_EXPLOSION,
										EXPLOSION_TYPE_TESLA,FALSE,NULL,0);

								if(selectedPlayer == psStructure->player)
								{
									audio_PlayObjStaticTrack( (void *) psStructure, ID_SOUND_POWER_SPARK );
								}

							}
							/*	Work out how many spires it has. This is a particularly unpleasant
								hack and I'm not proud of it, but it needs to done. Honest. AM
							*/
							//if(capacity)

						}



					}
				}
			}
		}
	}
}

UDWORD	getFreeEffect( void )
{
	return(freeEffect);
}


// ----------------------------------------------------------------------------------------
void	effectResetUpdates( void )
{
UDWORD	i;

	for(i=0; i<EFFECT_DROID_DIVISION; i++)
	{
		lastUpdateDroids[i] = 0;
	}
	for(i=0; i<EFFECT_STRUCTURE_DIVISION; i++)
	{
		lastUpdateStructures[i] = 0;
	}
}


// -----------------------------------------------------------------------------------
bool fireOnLocation(unsigned int x, unsigned int y)
{
	unsigned int i;

	for(i = 0; i < MAX_EFFECTS; ++i)
	{
	 	if (effectStatus[i] == ES_ACTIVE
	 	 && asEffectsList[i].group == EFFECT_FIRE)
		{
			const unsigned int posX = asEffectsList[i].position.x;
			const unsigned int posY = asEffectsList[i].position.z;

			if (posX == x
			 && posY == y)
			{
				return true;
			}
		}
	}

	return false;
}

/** Write the given EFFECT to the given file with endianness swapped correctly
 *  \param fileHandle a PhysicsFS file handle of the file to write to
 *  \param serializeEffect a pointer to the EFFECT to write serialized into the file
 *  \return true on success or false on the first encounterd error
 */
static bool serializeFXData(PHYSFS_file* fileHandle, const EFFECT* serializeEffect)
{
	uint32_t imdHashedNumber;

	// Retrieve the IMD hash number
	if (serializeEffect->imd)
	{
		resGetHashfromData("IMD", serializeEffect->imd, &imdHashedNumber);
	}
	else
	{
		imdHashedNumber = 0;
	}

	return (PHYSFS_writeUBE8 (fileHandle, serializeEffect->  control)
	     && PHYSFS_writeUBE8 (fileHandle, serializeEffect->  group)
	     && PHYSFS_writeUBE8 (fileHandle, serializeEffect->  type)
	     && PHYSFS_writeUBE8 (fileHandle, serializeEffect->  frameNumber)
	     && PHYSFS_writeUBE16(fileHandle, serializeEffect->  size)
	     && PHYSFS_writeUBE8 (fileHandle, serializeEffect->  baseScale)
	     && PHYSFS_writeUBE8 (fileHandle, serializeEffect->  specific)

	     // Write Vector3f types
	     && PHYSFS_writeBEFloat(fileHandle, serializeEffect->position.x)
	     && PHYSFS_writeBEFloat(fileHandle, serializeEffect->position.y)
	     && PHYSFS_writeBEFloat(fileHandle, serializeEffect->position.z)
	     && PHYSFS_writeBEFloat(fileHandle, serializeEffect->velocity.x)
	     && PHYSFS_writeBEFloat(fileHandle, serializeEffect->velocity.y)
	     && PHYSFS_writeBEFloat(fileHandle, serializeEffect->velocity.z)

	     // Write Vector3i types
	     && PHYSFS_writeSBE32(fileHandle, serializeEffect->  rotation.x)
	     && PHYSFS_writeSBE32(fileHandle, serializeEffect->  rotation.y)
	     && PHYSFS_writeSBE32(fileHandle, serializeEffect->  rotation.z)
	     && PHYSFS_writeSBE32(fileHandle, serializeEffect->  spin.x)
	     && PHYSFS_writeSBE32(fileHandle, serializeEffect->  spin.y)
	     && PHYSFS_writeSBE32(fileHandle, serializeEffect->  spin.z)

	     && PHYSFS_writeUBE32(fileHandle, serializeEffect->  birthTime)
	     && PHYSFS_writeUBE32(fileHandle, serializeEffect->  lastFrame)
	     && PHYSFS_writeUBE16(fileHandle, serializeEffect->  frameDelay)
	     && PHYSFS_writeUBE16(fileHandle, serializeEffect->  lifeSpan)
	     && PHYSFS_writeUBE16(fileHandle, serializeEffect->  radius)
	     && PHYSFS_writeUBE32(fileHandle, imdHashedNumber));
}

/** Read an EFFECT from the given file with endianness swapped correctly
 *  \param fileHandle a PhysicsFS file handle of the file to read from
 *  \param serializeEffect a pointer to an EFFECT to write the deserialized data from the file into
 *  \return true on success or false on the first encounterd error
 */
static bool deserializeFXData(PHYSFS_file* fileHandle, EFFECT* serializeEffect)
{
	uint32_t imdHashedNumber;

	if (!PHYSFS_readUBE8 (fileHandle, &serializeEffect->  control)
	 || !PHYSFS_readUBE8 (fileHandle, &serializeEffect->  group)
	 || !PHYSFS_readUBE8 (fileHandle, &serializeEffect->  type)
	 || !PHYSFS_readUBE8 (fileHandle, &serializeEffect->  frameNumber)
	 || !PHYSFS_readUBE16(fileHandle, &serializeEffect->  size)
	 || !PHYSFS_readUBE8 (fileHandle, &serializeEffect->  baseScale)
	 || !PHYSFS_readUBE8 (fileHandle, &serializeEffect->  specific)

	 // Write Vector3f types
	 || !PHYSFS_readBEFloat(fileHandle, &serializeEffect->position.x)
	 || !PHYSFS_readBEFloat(fileHandle, &serializeEffect->position.y)
	 || !PHYSFS_readBEFloat(fileHandle, &serializeEffect->position.z)
	 || !PHYSFS_readBEFloat(fileHandle, &serializeEffect->velocity.x)
	 || !PHYSFS_readBEFloat(fileHandle, &serializeEffect->velocity.y)
	 || !PHYSFS_readBEFloat(fileHandle, &serializeEffect->velocity.z)

	 // Write Vector3i types
	 || !PHYSFS_readSBE32(fileHandle, &serializeEffect->  rotation.x)
	 || !PHYSFS_readSBE32(fileHandle, &serializeEffect->  rotation.y)
	 || !PHYSFS_readSBE32(fileHandle, &serializeEffect->  rotation.z)
	 || !PHYSFS_readSBE32(fileHandle, &serializeEffect->  spin.x)
	 || !PHYSFS_readSBE32(fileHandle, &serializeEffect->  spin.y)
	 || !PHYSFS_readSBE32(fileHandle, &serializeEffect->  spin.z)
	 || !PHYSFS_readUBE32(fileHandle, &serializeEffect->  birthTime)
	 || !PHYSFS_readUBE32(fileHandle, &serializeEffect->  lastFrame)
	 || !PHYSFS_readUBE16(fileHandle, &serializeEffect->  frameDelay)
	 || !PHYSFS_readUBE16(fileHandle, &serializeEffect->  lifeSpan)
	 || !PHYSFS_readUBE16(fileHandle, &serializeEffect->  radius)
	 || !PHYSFS_readUBE32(fileHandle, &imdHashedNumber))
	{
		return false;
	}

	if (imdHashedNumber)
	{
		// Restore the pointer from the hashed ID
		serializeEffect->imd = (iIMDShape*)resGetDataFromHash("IMD", imdHashedNumber);
	}
	else
	{
		serializeEffect->imd = NULL;
	}

	return true;
}

// -----------------------------------------------------------------------------------
/* This will save out the effects data */
bool writeFXData(const char* fileName)
{
	unsigned int i;

	FX_SAVEHEADER fileHeader;

	PHYSFS_file* fileHandle = openSaveFile(fileName);
	if (!fileHandle)
	{
		return false;
	}

	fileHeader.aFileType[0] = 'f';
	fileHeader.aFileType[1] = 'x';
	fileHeader.aFileType[2] = 'd';
	fileHeader.aFileType[3] = 'a';

	fileHeader.version = CURRENT_VERSION_NUM;

	// Count all the active EFFECTs
	for (i=0, fileHeader.entries = 0; i < MAX_EFFECTS; ++i)
	{
		if(effectStatus[i] == ES_ACTIVE)
		{
			++fileHeader.entries;
		}
	}

	if (PHYSFS_write(fileHandle, fileHeader.aFileType, sizeof(fileHeader.aFileType), 1) != 1
	 || !PHYSFS_writeUBE32(fileHandle, fileHeader.version)
	 || !PHYSFS_writeUBE32(fileHandle, fileHeader.entries))
	{
		debug(LOG_ERROR, "writeFXData: could not write to %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return false;
	}

	for (i = 0; i < MAX_EFFECTS; ++i)
	{
		const EFFECT* curEffect = &asEffectsList[i];

		// Don't save inactive effects
		if (effectStatus[i] != ES_ACTIVE)
			continue;

		// Write the current EFFECT to the file with endianness swapped correctly
		// And abort on the first encountered error
		if (!serializeFXData(fileHandle, curEffect))
		{
			debug(LOG_ERROR, "writeFXData: could not write to %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
			PHYSFS_close(fileHandle);
			return false;
		}
	}

	// Close the file
	PHYSFS_close(fileHandle);

	// Everything is just fine!
	return true;
}

// -----------------------------------------------------------------------------------
/* This will read in the effects data */
bool readFXData(const char* fileName)
{
	FX_SAVEHEADER fileHeader;
	unsigned int expectedFileSize, fileSize;
	unsigned int i;

	PHYSFS_file* fileHandle = openLoadFile(fileName, false);
	if (!fileHandle)
	{
		// Failure to open the file is no failure to read it
		return true;
	}

	// Read the header from the file
	if (PHYSFS_read(fileHandle, fileHeader.aFileType, sizeof(fileHeader.aFileType), 1) != 1
	 || !PHYSFS_readUBE32(fileHandle, &fileHeader.version)
	 || !PHYSFS_readUBE32(fileHandle, &fileHeader.entries))
	{
		debug(LOG_ERROR, "readFXData: error while reading file: %s", PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return false;
	}

	// Check the header to see if we've been given a file of the right type
	if (fileHeader.aFileType[0] != 'f'
	 || fileHeader.aFileType[1] != 'x'
	 || fileHeader.aFileType[2] != 'd'
	 || fileHeader.aFileType[3] != 'a')
	{
		debug(LOG_ERROR, "readFXData: Weird file type found? Has header letters - '%c' '%c' '%c' '%c' (should be 'f' 'x' 'd' 'a')",
		      fileHeader.aFileType[0],
		      fileHeader.aFileType[1],
		      fileHeader.aFileType[2],
		      fileHeader.aFileType[3]);

		PHYSFS_close(fileHandle);
		return false;
	}

	// Validate the filesize
	expectedFileSize = sizeof(fileHeader.aFileType) + sizeof(fileHeader.version) + sizeof(fileHeader.entries) + fileHeader.entries * 74; // 74 = sizeof(EFFECT) without the padding
	fileSize = PHYSFS_fileLength(fileHandle);
	if (fileSize != expectedFileSize)
	{
		PHYSFS_close(fileHandle);
		ASSERT(!"readFXData: unexpected filesize", "readFXData: unexpected filesize; should be %u, but is %u", expectedFileSize, fileSize);
		abort();
		return false;
	}

	// Clear out anything that's there already!
	initEffectsSystem();

	// Load all EFFECTs
	for(i = 0; i < fileHeader.entries; ++i)
	{
		EFFECT* curEffect = &asEffectsList[i];

		ASSERT(i < MAX_EFFECTS, "readFXData: more effects in this file than our array can contain");

		// Read the current EFFECT from the file with endianness swapped correctly
		// And abort on the first encountered error
		if (!deserializeFXData(fileHandle, curEffect))
		{
			debug(LOG_ERROR, "readFXData: error while reading file: %s", PHYSFS_getLastError());
			PHYSFS_close(fileHandle);
			return false;
		}

		effectStatus[i] = ES_ACTIVE;
	}

	/* Ensure free effects kept up to date */
	freeEffect = i;

	// Close the file
	PHYSFS_close(fileHandle);

	/* Hopefully everything's just fine by now */
	return true;
}
