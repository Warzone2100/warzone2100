/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * @file component.c
 * Draws component objects - oh yes indeed.
*/

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/netplay/netplay.h"

#include "action.h"
#include "component.h"
#include "display3d.h"
#include "effects.h"
#include "intdisplay.h"
#include "loop.h"
#include "map.h"
#include "miscimd.h"
#include "order.h"
#include "projectile.h"
#include "transporter.h"
#include "mission.h"

#define GetRadius(x) ((x)->sradius)

#define	DEFAULT_COMPONENT_TRANSLUCENCY	128
#define	DROID_EMP_SPREAD	(20 - rand()%40)

//VTOL weapon connector start
#define VTOL_CONNECTOR_START 5

static bool		leftFirst;

// Colour Lookups
// use col = MAX_PLAYERS for anycolour (see multiint.c)
bool setPlayerColour(UDWORD player, UDWORD col)
{
	ASSERT(player < MAX_PLAYERS && col < MAX_PLAYERS, "Bad colour setting");
	NetPlay.players[player].colour = col;
	return true;
}

UBYTE getPlayerColour(UDWORD pl)
{
	return NetPlay.players[pl].colour;
}


static void setMatrix(Vector3i *Position, Vector3i *Rotation, bool RotXYZ)
{
	pie_PerspectiveBegin();
	pie_MatBegin();

	pie_TRANSLATE(Position->x,Position->y,Position->z);

	if(RotXYZ) {
		pie_MatRotX(DEG(Rotation->x));
		pie_MatRotY(DEG(Rotation->y));
		pie_MatRotZ(DEG(Rotation->z));
	} else {
		pie_MatRotY(DEG(Rotation->y));
		pie_MatRotX(DEG(Rotation->x));
		pie_MatRotZ(DEG(Rotation->z));
	}
}


static void unsetMatrix(void)
{
	pie_MatEnd();
	pie_PerspectiveEnd();
}


UDWORD getComponentDroidRadius(WZ_DECL_UNUSED DROID *psDroid)
{
	return 100;
}


UDWORD getComponentDroidTemplateRadius(WZ_DECL_UNUSED DROID_TEMPLATE *psDroid)
{
	return 100;
}


UDWORD getComponentRadius(BASE_STATS *psComponent)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	SDWORD compID;

	compID = StatIsComponent(psComponent);
	if (compID > 0)
	{
		StatGetComponentIMD(psComponent, compID,&ComponentIMD, &MountIMD);
		if(ComponentIMD)
		{
			return GetRadius(ComponentIMD);
		}
	}

	/* VTOL bombs are only stats allowed to have NULL ComponentIMD */
	if (StatIsComponent(psComponent) != COMP_WEAPON
		|| (((WEAPON_STATS *)psComponent)->weaponSubClass != WSC_BOMB
			&& ((WEAPON_STATS *)psComponent)->weaponSubClass != WSC_EMP))
	{
		ASSERT(ComponentIMD, "No ComponentIMD!");
	}

	return COMPONENT_RADIUS;
}


UDWORD getResearchRadius(BASE_STATS *Stat)
{
	iIMDShape *ResearchIMD = ((RESEARCH *)Stat)->pIMD;

	if(ResearchIMD)
	{
		return GetRadius(ResearchIMD);
	}

	debug(LOG_ERROR, "ResearchPIE == NULL");

	return 100;
}


UDWORD getStructureSizeMax(STRUCTURE *psStructure)
{
	//radius based on base plate size
	return MAX(psStructure->pStructureType->baseWidth, psStructure->pStructureType->baseBreadth);
}

UDWORD getStructureStatSizeMax(STRUCTURE_STATS *Stats)
{
	//radius based on base plate size
	return MAX(Stats->baseWidth, Stats->baseBreadth);
}

static UDWORD getStructureHeight(STRUCTURE *psStructure)
{
	return (getStructureStatHeight(psStructure->pStructureType));
}

UDWORD getStructureStatHeight(STRUCTURE_STATS *psStat)
{
	if (psStat->pIMD[0])
	{
		return (psStat->pIMD[0]->max.y - psStat->pIMD[0]->min.y);
	}

	return 0;
}


void displayIMDButton(iIMDShape *IMDShape, Vector3i *Rotation, Vector3i *Position, bool RotXYZ, SDWORD scale)
{
	setMatrix(Position, Rotation, RotXYZ);
	pie_MatScale(scale / 100.f);

	pie_SetFogStatus(false);
	pie_Draw3DShape(IMDShape, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
	unsetMatrix();
}


//changed it to loop thru and draw all weapons
void displayStructureButton(STRUCTURE *psStructure, Vector3i *rotation, Vector3i *Position, bool RotXYZ, SDWORD scale)
{
	iIMDShape *baseImd,*strImd;//*mountImd,*weaponImd;
	iIMDShape *mountImd[STRUCT_MAXWEAPS];
	iIMDShape *weaponImd[STRUCT_MAXWEAPS];
	UDWORD			nWeaponStat;
	int		i;

	/*HACK HACK HACK!
	if its a 'tall thin (ie tower)' structure with something on the top - offset the
	position to show the object on top*/
	if (psStructure->pStructureType->pIMD[0]->nconnectors && scale == SMALL_STRUCT_SCALE &&
		getStructureHeight(psStructure) > TOWER_HEIGHT)
	{
		Position->y -= 20;
	}

	setMatrix(Position, rotation, RotXYZ);
	pie_MatScale(scale / 100.f);

	/* Draw the building's base first */
	baseImd = psStructure->pStructureType->pBaseIMD;
	if(baseImd!=NULL) {
		pie_Draw3DShape(baseImd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
	}
	pie_Draw3DShape(psStructure->sDisplay.imd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
	//and draw the turret
	if(psStructure->sDisplay.imd->nconnectors)
	{
		for (i = 0;i < STRUCT_MAXWEAPS;i++)
		{
			weaponImd[i] = NULL;//weapon is gun ecm or sensor
			mountImd[i] = NULL;
		}

		strImd = psStructure->sDisplay.imd;
		//get an imd to draw on the connector priority is weapon, ECM, sensor
		//check for weapon
		for (i = 0; i < MAX(1, psStructure->numWeaps); i++)
		{
			if (psStructure->asWeaps[i].nStat > 0)
			{
				nWeaponStat = psStructure->asWeaps[i].nStat;
				weaponImd[i] =  asWeaponStats[nWeaponStat].pIMD;
				mountImd[i] =  asWeaponStats[nWeaponStat].pMountGraphic;
			}

			if (weaponImd[i] == NULL)
			{
				//check for ECM
				if (psStructure->pStructureType->pECM != NULL)
				{
					weaponImd[i] =  psStructure->pStructureType->pECM->pIMD;
					mountImd[i] =  psStructure->pStructureType->pECM->pMountGraphic;
				}
			}

			if (weaponImd[i] == NULL)
			{
				//check for sensor
				if (psStructure->pStructureType->pSensor != NULL)
				{
					weaponImd[i] =  psStructure->pStructureType->pSensor->pIMD;
					mountImd[i]  =  psStructure->pStructureType->pSensor->pMountGraphic;
				}
			}
		}

		//draw Weapon/ECM/Sensor for structure
		//uses 0
		if (weaponImd[0] != NULL)
		{
			for (i = 0; i < MAX(1, psStructure->numWeaps); i++)
			{
				Rotation rot = structureGetInterpolatedWeaponRotation(psStructure, i, graphicsTime);

				pie_MatBegin();
				pie_TRANSLATE(strImd->connectors[i].x,strImd->connectors[i].z,strImd->connectors[i].y);
				pie_MatRotY(-rot.direction);
				if (mountImd[i] != NULL)
				{
					pie_Draw3DShape(mountImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
					if(mountImd[i]->nconnectors)
					{
						pie_TRANSLATE(mountImd[i]->connectors->x,mountImd[i]->connectors->z,mountImd[i]->connectors->y);
					}
				}
				pie_MatRotX(rot.pitch);
				pie_Draw3DShape(weaponImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
				//we have a droid weapon so do we draw a muzzle flash
				pie_MatEnd();
			}
		}
	}
	unsetMatrix();
}

void displayStructureStatButton(STRUCTURE_STATS *Stats, Vector3i *Rotation, Vector3i *Position, bool RotXYZ, SDWORD scale)
{
	iIMDShape		*baseImd,*strImd;//*mountImd,*weaponImd;
	iIMDShape *mountImd[STRUCT_MAXWEAPS];
	iIMDShape *weaponImd[STRUCT_MAXWEAPS];
	UBYTE	i;

	/*HACK HACK HACK!
	if its a 'tall thin (ie tower)' structure stat with something on the top - offset the
	position to show the object on top*/
	if (Stats->pIMD[0]->nconnectors && scale == SMALL_STRUCT_SCALE &&
	    getStructureStatHeight(Stats) > TOWER_HEIGHT)
	{
		Position->y -= 20;
	}

	setMatrix(Position, Rotation, RotXYZ);
	pie_MatScale(scale / 100.f);

	/* Draw the building's base first */
	baseImd = Stats->pBaseIMD;

	if (baseImd != NULL)
	{
		pie_Draw3DShape(baseImd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
	}
	pie_Draw3DShape(Stats->pIMD[0], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);

	//and draw the turret
	if(Stats->pIMD[0]->nconnectors)
	{
		if (Stats->numWeaps > 0)
		{
			for (i = 0;i < Stats->numWeaps;i++)
			{
				weaponImd[i] = NULL;//weapon is gun ecm or sensor
				mountImd[i] = NULL;
			}
		}
		else
		{
			weaponImd[0] = NULL;
			mountImd[0] = NULL;
		}
		strImd = Stats->pIMD[0];
		//get an imd to draw on the connector priority is weapon, ECM, sensor
		//check for weapon
		//can only have the STRUCT_MAXWEAPS
		for (i = 0; i < MAX(1, Stats->numWeaps); i++)
		{
			//can only have the one
			if (Stats->psWeapStat[i] != NULL)
			{
				weaponImd[i] = Stats->psWeapStat[i]->pIMD;
				mountImd[i] = Stats->psWeapStat[i]->pMountGraphic;
			}

			if (weaponImd[i] == NULL)
			{
				//check for ECM
				if (Stats->pECM != NULL)
				{
					weaponImd[i] =  Stats->pECM->pIMD;
					mountImd[i] =  Stats->pECM->pMountGraphic;
				}
			}

			if (weaponImd[i] == NULL)
			{
				//check for sensor
				if (Stats->pSensor != NULL)
				{
					weaponImd[i] =  Stats->pSensor->pIMD;
					mountImd[i]  =  Stats->pSensor->pMountGraphic;
				}
			}
		}

		//draw Weapon/ECM/Sensor for structure
		if (weaponImd[0] != NULL)
		{
			for (i = 0; i < MAX(1, Stats->numWeaps); i++)
			{
				pie_MatBegin();
				pie_TRANSLATE(strImd->connectors[i].x,strImd->connectors[i].z,strImd->connectors[i].y);
				if (mountImd[i] != NULL)
				{
					pie_Draw3DShape(mountImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
					if(mountImd[i]->nconnectors)
					{
						pie_TRANSLATE(mountImd[i]->connectors->x,mountImd[i]->connectors->z,mountImd[i]->connectors->y);
					}
				}
				pie_Draw3DShape(weaponImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
				//we have a droid weapon so do we draw a muzzle flash
				pie_MatEnd();
			}
		}
	}

	unsetMatrix();
}



// Render a component given a BASE_STATS structure.
//
void displayComponentButton(BASE_STATS *Stat, Vector3i *Rotation, Vector3i *Position,
                            bool RotXYZ, SDWORD scale)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	SDWORD compID;

	setMatrix(Position, Rotation, RotXYZ);
	pie_MatScale(scale / 100.f);

	compID = StatIsComponent(Stat);
	if (compID > 0)	{
		StatGetComponentIMD(Stat, compID,&ComponentIMD, &MountIMD);
	}
	else
	{
		unsetMatrix();
		return;
	}

	/* VTOL bombs are only stats allowed to have NULL ComponentIMD */
	if (StatIsComponent(Stat) != COMP_WEAPON
		|| (((WEAPON_STATS *)Stat)->weaponSubClass != WSC_BOMB
			&& ((WEAPON_STATS *)Stat)->weaponSubClass != WSC_EMP))
	{
		ASSERT(ComponentIMD, "No ComponentIMD");
	}

	if (MountIMD)
	{
		pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);

		/* translate for weapon mount point */
		if (MountIMD->nconnectors)
		{
			pie_TRANSLATE(MountIMD->connectors->x, MountIMD->connectors->z, MountIMD->connectors->y);
		}
	}
	if (ComponentIMD)
	{
		pie_Draw3DShape(ComponentIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
	}

	unsetMatrix();
}


// Render a research item given a BASE_STATS structure.
//
void displayResearchButton(BASE_STATS *Stat, Vector3i *Rotation, Vector3i *Position, bool RotXYZ, SDWORD scale)
{
	iIMDShape *ResearchIMD = ((RESEARCH *)Stat)->pIMD;
	iIMDShape *MountIMD = ((RESEARCH *)Stat)->pIMD2;

	if(ResearchIMD)
	{
		setMatrix(Position, Rotation, RotXYZ);
		pie_MatScale(scale / 100.f);

		if(MountIMD) {
			pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);
		}
		pie_Draw3DShape(ResearchIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0);

		unsetMatrix();
	}
	else
	{
		debug(LOG_ERROR, "ResearchIMD == NULL");
	}
}



static iIMDShape *getLeftPropulsionIMD(DROID *psDroid)
{
	UDWORD			bodyStat, propStat;
	iIMDShape		**imd;

	bodyStat = psDroid->asBits[COMP_BODY].nStat;
	propStat = psDroid->asBits[COMP_PROPULSION].nStat;

	imd = asBodyStats[bodyStat].ppIMDList;
	imd += (propStat * NUM_PROP_SIDES + LEFT_PROP);

	return *imd;
}


static iIMDShape *getRightPropulsionIMD(DROID *psDroid)
{
	UDWORD			bodyStat, propStat;
	iIMDShape		**imd;

	bodyStat = psDroid->asBits[COMP_BODY].nStat;
	propStat = psDroid->asBits[COMP_PROPULSION].nStat;

	imd = asBodyStats[bodyStat].ppIMDList;
	imd += (propStat * NUM_PROP_SIDES + RIGHT_PROP);

	return *imd;
}


/* Assumes matrix context is already set */
// this is able to handle multiple weapon graphics now
// removed mountRotation,they get such stuff from psObj directly now
static void displayCompObj(DROID *psDroid, bool bButton)
{
	iIMDShape               *psShape, *psJet, *psShapeTemp = NULL, *psMountShape;
	Vector3i                zero(0, 0, 0);
	Vector2i				screenCoords;
	SDWORD				iConnector;
	PROPULSION_STATS	*psPropStats;
	SDWORD				frame;
	SDWORD				pieFlag, iPieData;
	PIELIGHT			brightness;
	UDWORD				colour;
	UBYTE	i;

	if (graphicsTime - psDroid->timeLastHit < GAME_TICKS_PER_SEC/4 && psDroid->lastHitWeapon == WSC_ELECTRONIC && !gamePaused())
	{
		colour = getPlayerColour(rand()%MAX_PLAYERS);
	}
	else
	{
		colour = getPlayerColour(psDroid->player);
	}
	
	/* get propulsion stats */
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT_OR_RETURN( , psPropStats != NULL, "invalid propulsion stats pointer");

	//set pieflag for button object or ingame object
	if (bButton)
	{
		pieFlag = pie_BUTTON;
		brightness = WZCOL_WHITE;
	}
	else
	{
		pieFlag = pie_SHADOW;
		brightness = pal_SetBrightness(psDroid->illumination);
		// NOTE: Beware of transporters that are offscreen, on a mission!  We should *not* be checking tiles at this point in time!
		if (psDroid->droidType != DROID_TRANSPORTER && !missionIsOffworld())
		{
			MAPTILE *psTile = worldTile(psDroid->pos.x, psDroid->pos.y);
			if (psTile->jammerBits & alliancebits[psDroid->player])
			{
				pieFlag |= pie_ECM;
			}
		}
	}

	/* set default components transparent */
	if ( psDroid->asBits[COMP_PROPULSION].nStat == 0 )
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		iPieData = 0;
	}
	
	if (!bButton && psPropStats->propulsionType == PROPULSION_TYPE_PROPELLOR)
	{
		// FIXME: change when adding submarines to the game
		pie_TRANSLATE(0, -world_coord(1)/2.3f, 0);
	}

	//uses psShapeTemp too separate it from turret's psShape
	psShapeTemp = (leftFirst ? getLeftPropulsionIMD(psDroid) : getRightPropulsionIMD(psDroid));
	if(psShapeTemp!=NULL)
	{
		pie_Draw3DShape(psShapeTemp, 0, colour, brightness, pieFlag, iPieData);
	}

	/* set default components transparent */
	if ( psDroid->asBits[COMP_BODY].nStat == 0 )
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	/* Get the body graphic now*/
	//uses psShapeTemp too separate it from turret's psShape
	psShapeTemp = BODY_IMD(psDroid,psDroid->player);
	if(psShapeTemp!=NULL)
	{
		// FIXME
		if ( psDroid->droidType == DROID_PERSON)
		{
			/* draw body if not animating */
			if ( psDroid->psCurAnim == NULL  || psDroid->psCurAnim->bVisible == false )
			{
				// FIXME - hideous....!!!!
				pie_MatScale(.75f);
				pie_Draw3DShape(psShapeTemp, 0, psDroid->player-6, brightness, pieFlag, iPieData);
			}
		}
		else if (cyborgDroid(psDroid))
		{
			/* draw body if cyborg not animating */
			if ( psDroid->psCurAnim == NULL || psDroid->psCurAnim->bVisible == false )
			{
				pie_Draw3DShape(psShapeTemp, 0, colour, brightness, pieFlag, iPieData);
			}
		}
		else
		{
			pie_Draw3DShape(psShapeTemp, 0, colour, brightness, pieFlag, iPieData);
		}
	}

	/* render vtol jet if flying - horrible hack - GJ */
	if (((psPropStats->propulsionType == PROPULSION_TYPE_LIFT) &&
		//(psDroid->droidType != DROID_CYBORG)) && (!bButton))
		(!cyborgDroid(psDroid))) && (!bButton))
	{
		/* show flame if above ground */
		if ( psDroid->sMove.Status != MOVEINACTIVE )
		{
			/* draw flame if found  */

			/* GJ TODO: add flame-finding code here */
			psJet = asBodyStats[psDroid->asBits[COMP_BODY].nStat].pFlameIMD;

			if ( psJet != NULL )
			{
				pie_Draw3DShape(psJet, getModularScaledGraphicsTime(psJet->animInterval, psJet->numFrames), colour, brightness, pie_ADDITIVE, 200);
			}
		}
	}

	//don't change the screen coords of an object if drawing it in a button
	if (!bButton)
	{
		/* set up all the screen coords stuff - need to REMOVE FROM THIS LOOP */
		calcScreenCoords(psDroid);
	}

	/* set default components transparent */
	if (psDroid->asWeaps[0].nStat              == 0 &&
	    psDroid->asBits[COMP_SENSOR].nStat     == 0 &&
	    psDroid->asBits[COMP_ECM].nStat        == 0 &&
	    psDroid->asBits[COMP_BRAIN].nStat      == 0 &&
	    psDroid->asBits[COMP_REPAIRUNIT].nStat == 0 &&
	    psDroid->asBits[COMP_CONSTRUCT].nStat  == 0)
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	/* Indenting here is only to show new matrix context */
	{
		psShapeTemp = BODY_IMD(psDroid,psDroid->player);
		if( psShapeTemp->nconnectors )
		{
			/* vtol weapons attach to connector 2 (underneath);
			 * all others to connector 1 */
			/* VTOL's now skip the first 5 connectors(0 to 4),
			VTOL's use 5,6,7,8 etc now */
			if ( (psPropStats->propulsionType == PROPULSION_TYPE_LIFT) &&
				  psDroid->droidType == DROID_WEAPON )
			{
				iConnector = VTOL_CONNECTOR_START;
			}
			else
			{
				iConnector = 0;
			}

			switch(psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_TRANSPORTER:
			case DROID_SUPERTRANSPORTER:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_WEAPON:
			case DROID_COMMAND:		// command droids have a weapon to store all the graphics
				/*	Get the mounting graphic - we've already moved to the right position
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				/* Double check that the weapon droid actually has any */
				for (i = 0;i < psDroid->numWeaps;i++)
				{
					if (psDroid->asWeaps[i].nStat > 0 || psDroid->droidType == DROID_DEFAULT)
					{
						if ( psShapeTemp->connectors )
						{
							Rotation rot = getInterpolatedWeaponRotation(psDroid, i, graphicsTime);

							pie_MatBegin();

							//to skip number of VTOL_CONNECTOR_START ground unit connectors
							if ( iConnector < VTOL_CONNECTOR_START )
							{
								pie_TRANSLATE(psShapeTemp->connectors[i].x,
								              psShapeTemp->connectors[i].z,
								              psShapeTemp->connectors[i].y);
							}
							else
							{
								pie_TRANSLATE( psShapeTemp->connectors[iConnector + i].x,
											   psShapeTemp->connectors[iConnector + i].z,
											   psShapeTemp->connectors[iConnector + i].y  );
							}

							pie_MatRotY(-rot.direction);

							/* vtol weapons inverted */
							if ( iConnector >= VTOL_CONNECTOR_START )
							{
								pie_MatRotZ(65536/2);  //this might affect gun rotation
							}

							/* Get the mount graphic */
							psShape = WEAPON_MOUNT_IMD(psDroid, i);

							int recoilValue = getRecoil(psDroid->asWeaps[i]);
							pie_TRANSLATE(0, 0, recoilValue / 3);

							/* Draw it */
							if(psShape)
							{
								pie_Draw3DShape(psShape, 0, colour, brightness, pieFlag, iPieData);
							}
							
							pie_TRANSLATE(0, 0, recoilValue);

							/* translate for weapon mount point */
							if (psShape && psShape->nconnectors)
							{
								pie_TRANSLATE(psShape->connectors->x, psShape->connectors->z, psShape->connectors->y);
							}

							/* vtol weapons inverted */
							if ( iConnector >= VTOL_CONNECTOR_START )
							{
								//pitch the barrel down
								pie_MatRotX(-rot.pitch);
							}
							else
							{
								//pitch the barrel up
								pie_MatRotX(rot.pitch);
							}

							/* Get the weapon (gun?) graphic */
							psShape = WEAPON_IMD(psDroid, i);
							
							// We have a weapon so we draw it and a muzzle flash from weapon connector
							if (psShape)
							{
								pie_Draw3DShape(psShape, 0, colour, brightness, pieFlag, iPieData);
								
								if (psShape->nconnectors)
								{
									unsigned int connector_num = 0;

									// which barrel is firing if model have multiple muzzle connectors?
									if (psDroid->asWeaps[i].shotsFired && (psShape->nconnectors > 1))
									{
										// shoot first, draw later - substract one shot to get correct results
										connector_num = (psDroid->asWeaps[i].shotsFired - 1) % (psShape->nconnectors);
									}
									
									/* Now we need to move to the end of the firing barrel (there maybe multiple barrels) */
									pie_TRANSLATE( psShape->connectors[connector_num].x,
												   psShape->connectors[connector_num].z,
												   psShape->connectors[connector_num].y);
									
									//and draw the muzzle flash
									psShape = MUZZLE_FLASH_PIE(psDroid, i);
									
									if (psShape && graphicsTime >= psDroid->asWeaps[i].lastFired)
									{
										//assume no clan colours for muzzle effects
										if ((psShape->numFrames == 0) || (psShape->animInterval <= 0))										
										{
											//no anim so display one frame for a fixed time
											if (graphicsTime < psDroid->asWeaps[i].lastFired + BASE_MUZZLE_FLASH_DURATION)
											{
												pie_Draw3DShape(psShape, 0, 0, brightness, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);
											}
										}
										else
										{
											// animated muzzle
											frame = (graphicsTime - psDroid->asWeaps[i].lastFired) / psShape->animInterval;
											if (frame < psShape->numFrames)
											{
												pie_Draw3DShape(psShape, frame, 0, brightness, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);
											}
										}
									}
								}
							}
						}
						/* Pop Matrix */
						pie_MatEnd();
					}
				}
				break;

			case DROID_SENSOR:
			case DROID_CONSTRUCT:
			case DROID_CYBORG_CONSTRUCT:
			case DROID_ECM:
			case DROID_REPAIR:
			case DROID_CYBORG_REPAIR:
			{
				Rotation rot = getInterpolatedWeaponRotation(psDroid, 0, graphicsTime);

				switch (psDroid->droidType)
				{
				default: ASSERT(false, "...");
				case DROID_SENSOR:
					psMountShape = SENSOR_MOUNT_IMD(psDroid, psDroid->player);
					/* Get the sensor graphic, assuming it's there */
					psShape = SENSOR_IMD(psDroid, psDroid->player);
					break;
				case DROID_CONSTRUCT:
				case DROID_CYBORG_CONSTRUCT:
					psMountShape = CONSTRUCT_MOUNT_IMD(psDroid, psDroid->player);
					/* Get the construct graphic assuming it's there */
					psShape = CONSTRUCT_IMD(psDroid, psDroid->player);
					break;
				case DROID_ECM:
					psMountShape = ECM_MOUNT_IMD(psDroid, psDroid->player);
					/* Get the ECM graphic assuming it's there.... */
					psShape = ECM_IMD(psDroid, psDroid->player);
					break;
				case DROID_REPAIR:
				case DROID_CYBORG_REPAIR:
					psMountShape = REPAIR_MOUNT_IMD(psDroid, psDroid->player);
					/* Get the Repair graphic assuming it's there.... */
					psShape = REPAIR_IMD(psDroid, psDroid->player);
					break;
				}
				/*	Get the mounting graphic - we've already moved to the right position
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				//sensor and cyborg and ecm uses connectors[0]
				pie_MatBegin();
				/* vtol weapons inverted */
				if ( iConnector >= VTOL_CONNECTOR_START )
				{
					pie_MatRotZ(65536/2);  //this might affect gun rotation
				}

				pie_TRANSLATE( psShapeTemp->connectors[0].x,
							   psShapeTemp->connectors[0].z,
							   psShapeTemp->connectors[0].y  );

				pie_MatRotY(-rot.direction);
				/* Draw it */
				if (psMountShape)
				{
					pie_Draw3DShape(psMountShape, 0, colour, brightness, pieFlag, iPieData);
				}

				/* translate for construct mount point if cyborg */
				if (cyborgDroid(psDroid) && psMountShape && psMountShape->nconnectors)
				{
					pie_TRANSLATE(psMountShape->connectors[0].x,
					              psMountShape->connectors[0].z,
					              psMountShape->connectors[0].y);
				}

				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0, colour, brightness, pieFlag, iPieData);

					// In repair droid case only:
					if ((psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR) &&
					    psShape->nconnectors && psDroid->action == DACTION_DROIDREPAIR)
					{
						Spacetime st = interpolateObjectSpacetime(psDroid, graphicsTime);

						pie_TRANSLATE( psShape->connectors[0].x,
									   psShape->connectors[0].z,
									   psShape->connectors[0].y  );
						pie_TRANSLATE(0,-20,0);

						psShape = getImdFromIndex(MI_FLAME);

						/* Rotate for droid */
						pie_MatRotY(st.rot.direction);
						pie_MatRotX(-st.rot.pitch);
						pie_MatRotZ(-st.rot.roll);
						//rotate Y
						pie_MatRotY(rot.direction);

						pie_MatRotY(-player.r.y);
						pie_MatRotX(-player.r.x);

						pie_Draw3DShape(psShape, getModularScaledGraphicsTime(psShape->animInterval, psShape->numFrames), 0, brightness, pie_ADDITIVE, 140);

						pie_MatRotX(player.r.x);
						pie_MatRotY(player.r.y);
					}
				}
				/* Pop Matrix */
				pie_MatEnd();
				break;
			}
			case DROID_PERSON:
				// no extra mounts for people
				break;
			default:
				ASSERT(!"invalid droid type", "Whoa! Weirdy type of droid found in drawComponentObject!!!");
				break;
			}
		}
		/*	We've also got a handle on the psShape here for the weapon which has a connector to point to
			muzzle flash attachment points - just grab it from psShape->connectors->[x|y|z] */
	} // end of illustrative indentation - see above

	/* set default components transparent */
	if ( psDroid->asBits[COMP_PROPULSION].nStat == 0 )
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	psShape = (leftFirst ? getRightPropulsionIMD(psDroid) : getLeftPropulsionIMD(psDroid));
	if(psShape!=NULL)
	{
		pie_Draw3DShape(psShape, 0, colour, brightness, pieFlag, iPieData);
	}
}


// Render a composite droid given a DROID_TEMPLATE structure.
//
void displayComponentButtonTemplate(DROID_TEMPLATE *psTemplate, Vector3i *Rotation, Vector3i *Position, bool RotXYZ, SDWORD scale)
{
	setMatrix(Position, Rotation, RotXYZ);
	pie_MatScale(scale / 100.f);

	// Decide how to sort it.
	leftFirst = angleDelta(DEG(Rotation->y)) < 0;

	DROID Droid(0, selectedPlayer);
	memset(Droid.asBits, 0, sizeof(Droid.asBits));
	droidSetBits(psTemplate,&Droid);

	Droid.pos = Vector3i(0, 0, 0);
	Droid.rot = Vector3i(0, 0, 0);

	//draw multi component object as a button object
	displayCompObj(&Droid, true);

	unsetMatrix();
}


// Render a composite droid given a DROID structure.
//
void displayComponentButtonObject(DROID *psDroid, Vector3i *Rotation, Vector3i *Position, bool RotXYZ, SDWORD scale)
{
	SDWORD		difference;

	setMatrix(Position, Rotation, RotXYZ);
	pie_MatScale(scale / 100.f);

// Decide how to sort it.
	difference = Rotation->y%360;

	leftFirst = !((difference > 0 && difference < 180) || difference < -180);

// And render the composite object.
	//draw multi component object as a button object
	displayCompObj(psDroid, true);

	unsetMatrix();
}



/* Assumes matrix context is already set */
// multiple turrets display removed the pointless mountRotation
void displayComponentObject(DROID *psDroid)
{
	Vector3i position, rotation;
	Spacetime st = interpolateObjectSpacetime(psDroid, graphicsTime);

	leftFirst = angleDelta(player.r.y - st.rot.direction) <= 0;

	/* Push the matrix */
	pie_MatBegin();

	/* Get the real position */
	position.x = st.pos.x - player.p.x;
	position.z = -(st.pos.y - player.p.z);
	position.y = st.pos.z;

	if(psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
	{
		position.y += bobTransporterHeight();
	}

	/* Get all the pitch,roll,yaw info */
	rotation.y = -st.rot.direction;
	rotation.x = st.rot.pitch;
	rotation.z = st.rot.roll;

	/* Translate origin */
	pie_TRANSLATE(position.x,position.y,position.z);

	/* Rotate for droid */
	pie_MatRotY(rotation.y);
	pie_MatRotX(rotation.x);
	pie_MatRotZ(rotation.z);

	if (graphicsTime - psDroid->timeLastHit < GAME_TICKS_PER_SEC && psDroid->lastHitWeapon == WSC_ELECTRONIC)
	{
		objectShimmy( (BASE_OBJECT*) psDroid );
	}

	if (psDroid->lastHitWeapon == WSC_EMP && graphicsTime - psDroid->timeLastHit < EMP_DISABLE_TIME)
	{
		Vector3i position;

		//add an effect on the droid
		position.x = st.pos.x + DROID_EMP_SPREAD;
		position.y = st.pos.z + rand()%8;
		position.z = st.pos.y + DROID_EMP_SPREAD;
		effectGiveAuxVar(90+rand()%20);
		addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_PLASMA,false,NULL,0);
	}

	if (psDroid->visible[selectedPlayer] == UBYTE_MAX)
	{
		//ingame not button object
		//should render 3 mounted weapons now
		displayCompObj(psDroid, false);
	}
	else
	{
		int frame = graphicsTime/BLIP_ANIM_DURATION + psDroid->id % 8192; // de-sync the blip effect, but don't overflow the int
		pie_Draw3DShape(getImdFromIndex(MI_BLIP), frame, 0, WZCOL_WHITE, pie_ADDITIVE, psDroid->visible[selectedPlayer] / 2);
	}
	pie_MatEnd();
}


void destroyFXDroid(DROID *psDroid, unsigned impactTime)
{
	for (int i = 0; i < 5; ++i)
	{
		iIMDShape *psImd = NULL;

		int maxHorizontalScatter = TILE_UNITS/4;
		int heightScatter = TILE_UNITS/5;
		Vector2i horizontalScatter = iSinCosR(rand(), rand()%maxHorizontalScatter);

		Vector3i pos = swapYZ(psDroid->pos + Vector3i(horizontalScatter, 16 + heightScatter));
		switch(i)
		{
		case 0:
			switch(psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_CYBORG_CONSTRUCT:
			case DROID_CYBORG_REPAIR:
			case DROID_WEAPON:
			case DROID_COMMAND:
				if (psDroid->numWeaps > 0)
				{
					if(psDroid->asWeaps[0].nStat > 0)
					{
						psImd = WEAPON_MOUNT_IMD(psDroid, 0);
					}
				}
				break;
			default:
				break;
			}
			break;
		case 1:
			switch(psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_CYBORG_CONSTRUCT:
			case DROID_CYBORG_REPAIR:
			case DROID_WEAPON:
			case DROID_COMMAND:
				if(psDroid->numWeaps)
				{
					// get main weapon
					psImd = WEAPON_IMD(psDroid, 0);
				}
				break;
			default:
				break;
			}
			break;
		}
		if (psImd == NULL)
		{
			psImd = getRandomDebrisImd();
		}
		// Tell the effect system that it needs to use this player's color for the next effect
		SetEffectForPlayer(psDroid->player);
		addEffect(&pos, EFFECT_GRAVITON, GRAVITON_TYPE_EMITTING_DR, true, psImd, getPlayerColour(psDroid->player), impactTime);
	}
}


void	compPersonToBits(DROID *psDroid)
{
	Vector3i position;	//,rotation,velocity;
	iIMDShape	*headImd, *legsImd, *armImd, *bodyImd;
	UDWORD		col;

	if(!psDroid->visible[selectedPlayer])
	{
		/* We can't see the person or cyborg - so get out */
		return;
	}
	/* get bits pointers according to whether baba or cyborg*/
	if (cyborgDroid(psDroid))
	{
		// This is probably unused now, since there's a more appropriate effect for cyborgs.
		headImd = getImdFromIndex(MI_CYBORG_HEAD);
		legsImd = getImdFromIndex(MI_CYBORG_LEGS);
		armImd  = getImdFromIndex(MI_CYBORG_ARM);
		bodyImd = getImdFromIndex(MI_CYBORG_BODY);
	}
	else
	{
		headImd = getImdFromIndex(MI_BABA_HEAD);
		legsImd = getImdFromIndex(MI_BABA_LEGS);
		armImd  = getImdFromIndex(MI_BABA_ARM);
		bodyImd = getImdFromIndex(MI_BABA_BODY);
	}

	/* Get where he's at */
	position.x = psDroid->pos.x;
	position.y = psDroid->pos.z+1;
	position.z = psDroid->pos.y;

	/* Tell about player colour */
	col = getPlayerColour(psDroid->player);

	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, headImd, col, gameTime - deltaGameTime + 1);
	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, legsImd, col, gameTime - deltaGameTime + 1);
	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, armImd, col, gameTime - deltaGameTime + 1);
	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, bodyImd, col, gameTime - deltaGameTime + 1);
}


SDWORD	rescaleButtonObject(SDWORD radius, SDWORD baseScale,SDWORD baseRadius)
{
	SDWORD newScale;
	newScale = 100 * baseRadius;
	newScale /= radius;
	if(baseScale > 0)
	{
		newScale += baseScale;
		newScale /= 2;
	}
	return newScale;
}
