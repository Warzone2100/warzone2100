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
	Component.c
	Draws component objects - oh yes indeed.
	Alex M, Pumpkin Studios, EIDOS Interactive, 1997.

*/
#include <string.h>

#include "lib/framework/frame.h"
#include "basedef.h"
#include "droid.h"
#include "action.h"
#include "order.h"
#include "component.h"
#include "lib/ivis_common/ivisdef.h" //ivis matrix code
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piedef.h" //ivis matrix code
#include "lib/ivis_common/piestate.h" //ivis render code
#include "lib/ivis_common/piepalette.h"
#include "lighting.h"
#include "loop.h"

#define GetRadius(x) ((x)->sradius)

#include "stats.h"
#include "lib/ivis_common/piemode.h"
#include "objects.h"
#include "display.h"
#include "geometry.h"
#include "display3d.h"
#include "map.h"
#include "lib/gamelib/gtime.h"
#include "intdisplay.h"
#include "miscimd.h"
#include "effects.h"
#include "e3demo.h"
#include "transporter.h"
#include "projectile.h"

static void unsetMatrix(void);
static void setMatrix(Vector3i *Position, Vector3i *Rotation, Vector3i *CameraPos, BOOL RotXYZ);

#define BLIP_ANIM_DURATION			200
#define	DEFAULT_COMPONENT_TRANSLUCENCY	128
#define	DROID_EMP_SPREAD	(20 - rand()%40)

//Watermelon:VTOL weapon connector start
#define VTOL_CONNECTOR_START 5

static void displayCompObj(BASE_OBJECT *psObj, BOOL bButton);
static iIMDShape *getLeftPropulsionIMD(DROID *psDroid);
static iIMDShape *getRightPropulsionIMD(DROID *psDroid);
static UDWORD getStructureHeight(STRUCTURE *psStructure);

static BOOL		leftFirst;
static SDWORD		droidLightLevel = 224;
static UDWORD		lightInterval = 15;
static UDWORD		lightLastChanged;
SDWORD		lightSpeed=2;
extern UDWORD selectedPlayer;

UBYTE		PlayerColour[MAX_PLAYERS];// = {0,1,2,3,4,5,6,7}

// Colour Lookups
// use col = MAX_PLAYERS for anycolour (see multiint.c)
BOOL setPlayerColour(UDWORD player, UDWORD col)
{
	if(player >MAX_PLAYERS || col >MAX_PLAYERS)
	{
		debug( LOG_ERROR, "setplayercolour: wrong values" );
		abort();
		return FALSE;
	}
	PlayerColour[(UBYTE)player] = (UBYTE)col;
	return TRUE;
}

UBYTE getPlayerColour(UDWORD pl)
{
	return PlayerColour[pl];
}

void initPlayerColours(void)
{
	UBYTE i;
	for(i=0;i<MAX_PLAYERS;i++)
	{
		PlayerColour[i] = i;
	}
}


void updateLightLevels(void)
{
	if(gameTime>(lightLastChanged+lightInterval))
	{
	   	droidLightLevel+=lightSpeed;
		lightLastChanged = gameTime;
	 	if(droidLightLevel>255 || droidLightLevel<128)
		{
			if(lightSpeed>0)
			{
					lightSpeed = -lightSpeed;
			}
			else
			{
					lightSpeed = -lightSpeed;
			}
		}
	}
}


static void setMatrix(Vector3i *Position, Vector3i *Rotation, Vector3i *CameraPos, BOOL RotXYZ)
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


UDWORD getComponentDroidRadius(DROID *psDroid)
{
	return 100;
}


UDWORD getComponentDroidTemplateRadius(DROID_TEMPLATE *psDroid)
{
	return 100;
}


UDWORD getComponentRadius(BASE_STATS *psComponent)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	SDWORD compID;

	compID = StatIsComponent(psComponent);
	if (compID > 0)	{
		StatGetComponentIMD(psComponent, compID,&ComponentIMD, &MountIMD);
		if(ComponentIMD)
		{
			return GetRadius(ComponentIMD);
		}
	}

	/* VTOL bombs are only stats allowed to have NULL ComponentIMD */
	if ( (StatIsComponent(psComponent) != COMP_WEAPON) ||
		  ((WEAPON_STATS *)psComponent)->weaponSubClass != WSC_BOMB )
	{
		debug(LOG_ERROR, "getComponentRadius: ComponentPIE == NULL");
	}

	return COMPONENT_RADIUS;
}


UDWORD getResearchRadius(BASE_STATS *Stat)
{
	iIMDShape *ResearchIMD = ((RESEARCH *)Stat)->pIMD;

	if(ResearchIMD) {
		return GetRadius(ResearchIMD);
	}

	debug(LOG_ERROR, "getResearchRadius: ResearchPIE == NULL");

	return 100;
}


UDWORD getStructureSize(STRUCTURE *psStructure)
{
	UDWORD size;
	//radius based on base plate size

	size = psStructure->pStructureType->baseWidth;
	if (psStructure->pStructureType->baseBreadth > size)
	{
		size = psStructure->pStructureType->baseBreadth;
	}
	return (size);
}

UDWORD getStructureStatSize(STRUCTURE_STATS *Stats)
{
	UDWORD size;
	//radius based on base plate size

	size = Stats->baseWidth;
	if (Stats->baseBreadth > size)
	{
		size = Stats->baseBreadth;
	}
	return (size);
}

UDWORD getStructureHeight(STRUCTURE *psStructure)
{
	return (getStructureStatHeight(psStructure->pStructureType));
}

UDWORD getStructureStatHeight(STRUCTURE_STATS *psStat)
{
	if (psStat->pIMD)
	{
		return (psStat->pIMD->ymax - psStat->pIMD->ymin);
	}

	return 0;
}


void displayIMDButton(iIMDShape *IMDShape, Vector3i *Rotation, Vector3i *Position, BOOL RotXYZ, SDWORD scale)
{
	Vector3i TmpCamPos = {0,0,0};

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	pie_MatScale(scale);

	pie_SetFogStatus(FALSE);
	pie_Draw3DShape(IMDShape, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
	unsetMatrix();
}


//Watermelon:changed it to loop thru and draw all weapons
void displayStructureButton(STRUCTURE *psStructure, Vector3i *Rotation, Vector3i *Position, BOOL RotXYZ, SDWORD scale)
{
	iIMDShape *baseImd,*strImd;//*mountImd,*weaponImd;
	iIMDShape *mountImd[STRUCT_MAXWEAPS];
	iIMDShape *weaponImd[STRUCT_MAXWEAPS];
	UDWORD			nWeaponStat;
	Vector3i TmpCamPos = {0,0,0};
	int		i;

	/*HACK HACK HACK!
	if its a 'tall thin (ie tower)' structure with something on the top - offset the
	position to show the object on top*/
	if (psStructure->pStructureType->pIMD->nconnectors && scale == SMALL_STRUCT_SCALE &&
	    getStructureHeight(psStructure) > TOWER_HEIGHT)
	{
		Position->y -= 20;
	}

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	pie_MatScale(scale);


	/* Draw the building's base first */
	baseImd = psStructure->pStructureType->pBaseIMD;
	if(baseImd!=NULL) {
		pie_Draw3DShape(baseImd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
	}
	pie_Draw3DShape(psStructure->sDisplay.imd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
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
		//Watermelon:re-enabled if (psStructure->numWeaps > 0)
		if (psStructure->numWeaps > 0)
		{
			for (i = 0;i < psStructure->numWeaps;i++)
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
		}
		else
		{
			if (psStructure->asWeaps[0].nStat > 0)
			{
				nWeaponStat = psStructure->asWeaps[0].nStat;
				weaponImd[0] =  asWeaponStats[nWeaponStat].pIMD;
				mountImd[0] =  asWeaponStats[nWeaponStat].pMountGraphic;
			}

			if (weaponImd[0] == NULL)
			{
				//check for ECM
				if (psStructure->pStructureType->pECM != NULL)
				{
					weaponImd[0] =  psStructure->pStructureType->pECM->pIMD;
					mountImd[0] =  psStructure->pStructureType->pECM->pMountGraphic;
				}
			}

			if (weaponImd[0] == NULL)
			{
				//check for sensor
				if (psStructure->pStructureType->pSensor != NULL)
				{
					weaponImd[0] =  psStructure->pStructureType->pSensor->pIMD;
					mountImd[0]  =  psStructure->pStructureType->pSensor->pMountGraphic;
				}
			}

		}

		//draw Weapon/ECM/Sensor for structure
		//Watermelon:uses 0
		if(weaponImd[0] != NULL)
		{
			if (psStructure->numWeaps > 0)
			{
				for (i = 0;i < psStructure->numWeaps;i++)
				{
					iV_MatrixBegin();
					iV_TRANSLATE(strImd->connectors[i].x,strImd->connectors[i].z,strImd->connectors[i].y);
					pie_MatRotY(DEG(-((SDWORD)psStructure->turretRotation[i])));
					if (mountImd[i] != NULL)
					{
						pie_Draw3DShape(mountImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
						if(mountImd[i]->nconnectors)
						{
							iV_TRANSLATE(mountImd[i]->connectors->x,mountImd[i]->connectors->z,mountImd[i]->connectors->y);
						}
					}
					iV_MatrixRotateX(DEG(psStructure->turretPitch[i]));
					pie_Draw3DShape(weaponImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
					//we have a droid weapon so do we draw a muzzle flash
					iV_MatrixEnd();
				}
			}
			else
			{
				iV_MatrixBegin();
				iV_TRANSLATE(strImd->connectors->x,strImd->connectors->z,strImd->connectors->y);
				pie_MatRotY(DEG(-((SDWORD)psStructure->turretRotation[0])));
				if (mountImd[0] != NULL)
				{
					pie_Draw3DShape(mountImd[0], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
					if(mountImd[0]->nconnectors)
					{
						iV_TRANSLATE(mountImd[0]->connectors->x,mountImd[0]->connectors->z,mountImd[0]->connectors->y);
					}
				}
				iV_MatrixRotateX(DEG(psStructure->turretPitch[0]));
				pie_Draw3DShape(weaponImd[0], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
				//we have a droid weapon so do we draw a muzzle flash
				iV_MatrixEnd();
			}
		}
	}
	unsetMatrix();
}

void displayStructureStatButton(STRUCTURE_STATS *Stats,UDWORD Player, Vector3i *Rotation, Vector3i *Position, BOOL RotXYZ, SDWORD scale)
{
	iIMDShape		*baseImd,*strImd;//*mountImd,*weaponImd;
	iIMDShape *mountImd[STRUCT_MAXWEAPS];
	iIMDShape *weaponImd[STRUCT_MAXWEAPS];
	Vector3i TmpCamPos = {0,0,0};
	//UDWORD			nWeaponStat;
	UBYTE	i;

	/*HACK HACK HACK!
	if its a 'tall thin (ie tower)' structure stat with something on the top - offset the
	position to show the object on top*/
	if (Stats->pIMD->nconnectors && scale == SMALL_STRUCT_SCALE &&
	    getStructureStatHeight(Stats) > TOWER_HEIGHT)
	{
		Position->y -= 20;
	}

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	pie_MatScale(scale);

	/* Draw the building's base first */
	baseImd = Stats->pBaseIMD;

	if (baseImd != NULL)
	{
		pie_Draw3DShape(baseImd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
	}
	pie_Draw3DShape(Stats->pIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);

	//and draw the turret
	if(Stats->pIMD->nconnectors)
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
		strImd = Stats->pIMD;
		//get an imd to draw on the connector priority is weapon, ECM, sensor
		//check for weapon
		//Watermelon:can only have the STRUCT_MAXWEAPS
		if (Stats->numWeaps > 0)
		{
			for (i = 0;i < Stats->numWeaps;i++)
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
		}
		else
		{
			if (Stats->psWeapStat[0] != NULL)
			{
				weaponImd[0] = Stats->psWeapStat[0]->pIMD;
				mountImd[0] = Stats->psWeapStat[0]->pMountGraphic;
			}

			if (weaponImd[0] == NULL)
			{
				//check for ECM
				if (Stats->pECM != NULL)
				{
					weaponImd[0] =  Stats->pECM->pIMD;
					mountImd[0] =  Stats->pECM->pMountGraphic;
				}
			}

			if (weaponImd[0] == NULL)
			{
				//check for sensor
				if (Stats->pSensor != NULL)
				{
					weaponImd[0] =  Stats->pSensor->pIMD;
					mountImd[0]  =  Stats->pSensor->pMountGraphic;
				}
			}
		}

		//draw Weapon/ECM/Sensor for structure
		if(weaponImd[0] != NULL)
		{
			if (Stats->numWeaps > 0)
			{
				for (i = 0;i < Stats->numWeaps;i++)
				{
					iV_MatrixBegin();
					iV_TRANSLATE(strImd->connectors[i].x,strImd->connectors[i].z,strImd->connectors[i].y);
					pie_MatRotY(DEG(0));
					if (mountImd[i] != NULL)
					{
						pie_Draw3DShape(mountImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
						if(mountImd[i]->nconnectors)
						{
							iV_TRANSLATE(mountImd[i]->connectors->x,mountImd[i]->connectors->z,mountImd[i]->connectors->y);
						}
					}
					iV_MatrixRotateX(DEG(0));
					pie_Draw3DShape(weaponImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
					//we have a droid weapon so do we draw a muzzle flash
					iV_MatrixEnd();
				}
			}
			else
			{
				iV_MatrixBegin();
				iV_TRANSLATE(strImd->connectors[0].x,strImd->connectors[0].z,strImd->connectors[0].y);
				pie_MatRotY(DEG(0));
				if (mountImd[0] != NULL)
				{
					pie_Draw3DShape(mountImd[0], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
					if(mountImd[0]->nconnectors)
					{
						iV_TRANSLATE(mountImd[0]->connectors->x,mountImd[0]->connectors->z,mountImd[0]->connectors->y);
					}
				}
				iV_MatrixRotateX(DEG(0));
				pie_Draw3DShape(weaponImd[0], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
				//we have a droid weapon so do we draw a muzzle flash
				iV_MatrixEnd();
			}
		}
	}

	unsetMatrix();
}



// Render a component given a BASE_STATS structure.
//
void displayComponentButton(BASE_STATS *Stat, Vector3i *Rotation, Vector3i *Position,
                            BOOL RotXYZ, SDWORD scale)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	SDWORD compID;
	Vector3i TmpCamPos = {0,0,0};

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	pie_MatScale(scale);

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
	if ( (ComponentIMD == NULL) &&
		 ((StatIsComponent(Stat) != COMP_WEAPON) ||
		  ((WEAPON_STATS *)Stat)->weaponSubClass != WSC_BOMB) )
	{
		debug(LOG_ERROR, "ComponentPIE == NULL");
	}

	if(MountIMD)
	{
		pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
	}
	if(ComponentIMD)
	{
		pie_Draw3DShape(ComponentIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
	}

	unsetMatrix();
}


// Render a research item given a BASE_STATS structure.
//
void displayResearchButton(BASE_STATS *Stat, Vector3i *Rotation, Vector3i *Position, BOOL RotXYZ, SDWORD scale)
{
	iIMDShape *ResearchIMD = ((RESEARCH *)Stat)->pIMD;
	iIMDShape *MountIMD = ((RESEARCH *)Stat)->pIMD2;
	Vector3i TmpCamPos = {0,0,0};

	if(ResearchIMD)
	{
		setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
		pie_MatScale(scale);

		if(MountIMD) {
			pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);
		}
		pie_Draw3DShape(ResearchIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, WZCOL_BLACK, pie_BUTTON, 0);

		unsetMatrix();
	}
	else
	{
		debug(LOG_ERROR, "displayResearchButton: ResearchIMD == NULL");
	}
}



// Render a composite droid given a DROID_TEMPLATE structure.
//
void displayComponentButtonTemplate(DROID_TEMPLATE *psTemplate, Vector3i *Rotation, Vector3i *Position, BOOL RotXYZ, SDWORD scale)
{
	static DROID Droid;	// Made static to reduce stack usage.
	SDWORD difference;
 	Vector3i TmpCamPos = {0,0,0};

	/* init to NULL */
	memset( &Droid, 0, sizeof(DROID) );

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	pie_MatScale(scale);

// Decide how to sort it.

	difference = Rotation->y%360;

	if((difference>0 && difference <180) || difference<-180)
	{
		leftFirst = FALSE;
	}
	else
	{
		leftFirst = TRUE;
	}

	droidSetBits(psTemplate,&Droid);
	Droid.player = (UBYTE)selectedPlayer;

	Droid.pos.x = Droid.pos.y = Droid.pos.z = 0;

	//draw multi component object as a button object
	displayCompObj((BASE_OBJECT*)&Droid, TRUE);


	unsetMatrix();
}


// Render a composite droid given a DROID structure.
//
void displayComponentButtonObject(DROID *psDroid, Vector3i *Rotation, Vector3i *Position, BOOL RotXYZ, SDWORD scale)
{
	SDWORD		difference;
 	Vector3i TmpCamPos = {0,0,0};

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	pie_MatScale(scale);

// Decide how to sort it.
	difference = Rotation->y%360;

	if((difference>0 && difference <180) || difference<-180)
	{
		leftFirst = FALSE;
	}
	else
	{
		leftFirst = TRUE;
	}

// And render the composite object.
	//draw multi component object as a button object
	displayCompObj((BASE_OBJECT*)psDroid, TRUE);

	unsetMatrix();
}



/* Assumes matrix context is already set */
// Watermelon:multiple turrets display removed the pointless mountRotation
void displayComponentObject(BASE_OBJECT *psObj)
{
	DROID		*psDroid = (DROID *)psObj;
	Vector3i	position, rotation;
	Sint32		xShift,zShift;
	UDWORD		worldAngle;
	SDWORD		difference;
	SDWORD		frame;
	PROPULSION_STATS	*psPropStats;
	UDWORD	tileX,tileY;
	MAPTILE	*psTile;

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	worldAngle = (UDWORD)(player.r.y / DEG_1) % 360;
	difference = worldAngle - psObj->direction;

	if((difference>0 && difference <180) || difference<-180)
	{
		leftFirst = FALSE;
	}
	else
	{
		leftFirst = TRUE;
	}

	/* Push the matrix */
	pie_MatBegin();

	/* Get internal tile units coordinates */
	xShift = map_round(player.p.x);
	zShift = map_round(player.p.z);

	/* Mask out to tile_units resolution */
	pie_TRANSLATE(xShift,0,-zShift);

	/* Get the real position */
	position.x = (psDroid->pos.x - player.p.x) - terrainMidX*TILE_UNITS;
	position.z = terrainMidY*TILE_UNITS - (psDroid->pos.y - player.p.z);
	position.y = psDroid->pos.z;

	if(psDroid->droidType == DROID_TRANSPORTER)
	{
		position.y += bobTransporterHeight();
	}

	/* Get all the pitch,roll,yaw info */
	rotation.y = -(SDWORD)psDroid->direction;
	rotation.x = psDroid->pitch;
	rotation.z = psDroid->roll;

	/* Translate origin */
	pie_TRANSLATE(position.x,position.y,position.z);

	/* Rotate for droid */
	pie_MatRotY(DEG(rotation.y));
	pie_MatRotX(DEG(rotation.x));
	pie_MatRotZ(DEG(rotation.z));

	if( (gameTime-psDroid->timeLastHit < GAME_TICKS_PER_SEC) && psDroid->lastHitWeapon == WSC_ELECTRONIC)
	{
		objectShimmy( (BASE_OBJECT*) psDroid );
	}

	if (psDroid->lastHitWeapon == WSC_EMP &&
	    (gameTime - psDroid->timeLastHit < EMP_DISABLE_TIME))
	{
		Vector3i position;

		//add an effect on the droid
		position.x = psDroid->pos.x + DROID_EMP_SPREAD;
		position.y = psDroid->pos.z + rand()%8;;
		position.z = psDroid->pos.y + DROID_EMP_SPREAD;
		effectGiveAuxVar(90+rand()%20);
		addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_PLASMA,FALSE,NULL,0);
	}

	if (godMode || (psDroid->visible[selectedPlayer] == UBYTE_MAX) || demoGetStatus())
	{
		//ingame not button object
		//Watermelon:should render 3 mounted weapons now
		displayCompObj(psObj,FALSE);
	}
	else
	{

		// make sure it's not over water.
		tileX = psDroid->pos.x/TILE_UNITS;
		tileY = psDroid->pos.y/TILE_UNITS;
		// double check it's on map
		if ( tileX < mapWidth && tileY < mapHeight )
		{
			psTile = mapTile(tileX,tileY);
			if (terrainType(psTile) != TER_WATER)
			{
			   	frame = gameTime/BLIP_ANIM_DURATION + psDroid->id; //visible[selectedPlayer];
			   	pie_Draw3DShape(getImdFromIndex(MI_BLIP), frame, 0, WZCOL_WHITE, WZCOL_BLACK, pie_ADDITIVE, psDroid->visible[selectedPlayer] / 2);
				/* set up all the screen coords stuff - need to REMOVE FROM THIS LOOP */
			}
		}
	}
	pie_MatEnd();
}


/* Assumes matrix context is already set */
// Watermelon:this is able to handle multiple weapon graphics now
// removed mountRotation,they get such stuff from psObj directly now
void displayCompObj(BASE_OBJECT *psObj, BOOL bButton)
{
	DROID				*psDroid;
	iIMDShape			*psShape, *psJet, *psShapeTemp = NULL;
	Vector3i				zero = {0, 0, 0};
	Vector2i				screenCoords;
	SDWORD				dummyZ, iConnector;
	PROPULSION_STATS	*psPropStats;
	SDWORD				frame;
	SDWORD				pieFlag, iPieData;
	PIELIGHT			brightness;
	const PIELIGHT			specular = WZCOL_BLACK;
	UDWORD				colour;
	UDWORD				bDarkSide = FALSE;
	UBYTE	i;

   	/* Cast the droid pointer */
	psDroid = (DROID *)psObj;
	if( (gameTime-psDroid->timeLastHit < GAME_TICKS_PER_SEC/4 ) && psDroid->lastHitWeapon == WSC_ELECTRONIC && !gamePaused())
	{
		colour = getPlayerColour(rand()%MAX_PLAYERS);
		bDarkSide = TRUE;
	}
	else
	{
		colour = getPlayerColour(psDroid->player);
	}

	//set pieflag for button object or ingame object
	if ( bButton )
	{
		pieFlag = pie_BUTTON;
	}
	else
	{
		pieFlag = 0;
	}

	if(!bButton)
	{
		brightness = pal_SetBrightness(psDroid->illumination);
		pieFlag = pie_SHADOW;
	}
	else
	{
		brightness = WZCOL_WHITE;
	}

	/* We've got a z value here _and_ screen coords of origin */
	dummyZ = pie_RotateProject(&zero, &screenCoords);

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

	//Watermelon:uses psShapeTemp too separate it from turret's psShape
	psShapeTemp = (leftFirst ? getLeftPropulsionIMD(psDroid) : getRightPropulsionIMD(psDroid));
	if(psShapeTemp!=NULL)
	{
		pie_Draw3DShape(psShapeTemp, 0, colour, brightness, specular, pieFlag, iPieData);
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
	//Watermelon:uses psShapeTemp too separate it from turret's psShape
	psShapeTemp = BODY_IMD(psDroid,psDroid->player);
	if(psShapeTemp!=NULL)
	{
		// FIXME
		if ( psDroid->droidType == DROID_PERSON)
		{
			/* draw body if not animating */
			if ( psDroid->psCurAnim == NULL  || psDroid->psCurAnim->bVisible == FALSE )
			{
				// FIXME - hideous....!!!!
				pie_MatScale(75);
				pie_Draw3DShape(psShapeTemp, 0, psDroid->player-6, brightness, specular, pieFlag, iPieData);
			}
		}
		//else if( psDroid->droidType == DROID_CYBORG)
	else if (cyborgDroid(psDroid))
		{
			/* draw body if cyborg not animating */
			if ( psDroid->psCurAnim == NULL || psDroid->psCurAnim->bVisible == FALSE )
			{
				pie_Draw3DShape(psShapeTemp, 0, colour, brightness, specular, pieFlag, iPieData);
			}
		}
		else
		{
			pie_Draw3DShape(psShapeTemp, 0, colour, brightness, specular, pieFlag, iPieData);
		}
	}

	/* get propulsion stats */
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( psPropStats != NULL,
			"moveUpdateUnit: invalid propulsion stats pointer" );

	/* render vtol jet if flying - horrible hack - GJ */
	if (((psPropStats->propulsionType == LIFT) &&
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
				pie_Draw3DShape(psJet, getStaticTimeValueRange(100,psJet->numFrames), colour, brightness, specular, pie_ADDITIVE, 200);
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
			/* Watermelon:VTOL's now skip the first 5 connectors(0 to 4),
			VTOL's use 5,6,7,8 etc now */
			if ( (psPropStats->propulsionType == LIFT) &&
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
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_WEAPON:
			case DROID_COMMAND:		// command droids have a weapon to store all the graphics
				/*	Get the mounting graphic - we've already moved to the right position
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				/* Double check that the weapon droid actually has any */
				//Watermelon:uses numWeaps
				//if(psDroid->numWeaps)
				for (i = 0;i < psDroid->numWeaps;i++)
				{
					if (psDroid->asWeaps[i].nStat > 0 || psDroid->droidType == DROID_DEFAULT)
					{
						if ( psShapeTemp->connectors )
						{
							pie_MatBegin();
							//Watermelon:reset Z?
							dummyZ = pie_RotateProject(&zero, &screenCoords);

							//Watermelon:to skip number of VTOL_CONNECTOR_START ground unit connectors
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

							if ( psDroid->turretRotation[i] )
							{
								pie_MatRotY(DEG( (-(SDWORD)psDroid->turretRotation[i])) );
							}


							/* vtol weapons inverted */
							if ( iConnector >= VTOL_CONNECTOR_START )
							{
								pie_MatRotZ( DEG_360/2 );//this might affect gun rotation
							}


							//psShape = WEAPON_MOUNT_IMD(psDroid,psDroid->player);
							psShape = (asWeaponStats[psDroid->asWeaps[i].nStat]).pMountGraphic;
							/* Draw it */
							//if(psDroid->numWeaps) already done this check above?!
							{
								pie_TRANSLATE(0,0,psDroid->asWeaps[i].recoilValue/3);
							}
							if(psShape)
							{
								pie_Draw3DShape(psShape, 0, colour, brightness, specular, pieFlag, iPieData);
							}

							//if(psDroid->numWeaps) already done this check above?!
							{
								pie_TRANSLATE(0,0,psDroid->asWeaps[i].recoilValue);
							}

							/* translate for weapon mount point if cyborg */
							//if( psDroid->droidType == DROID_CYBORG &&
							if (cyborgDroid(psDroid) && psShape && psShape->nconnectors)
							{
								pie_TRANSLATE( psShape->connectors[0].x,
											   psShape->connectors[0].z,
											   psShape->connectors[0].y  );
							}

							/* vtol weapons inverted */
							if ( iConnector >= VTOL_CONNECTOR_START )
							{
								//pitch the barrel down
								pie_MatRotX(DEG( (-(psDroid->turretPitch[i])) ));
							}
							else
							{
								//pitch the barrel up
								pie_MatRotX(DEG(psDroid->turretPitch[i]));
							}

							/* Get the weapon (gun?) graphic */
							//psShape = WEAPON_IMD(psDroid,psDroid->player);
							psShape = (asWeaponStats[psDroid->asWeaps[i].nStat]).pIMD;
							/* Draw it */
							if(psShape)
							{
								pie_Draw3DShape(psShape, 0, colour, brightness, specular, pieFlag, iPieData);
							}
							//we have a droid weapon so do we draw a muzzle flash
							if( psShape && psShape->nconnectors )
							{
								/* Now we need to move to the end fo the barrel */
								//Watermelon:fixed the bug,I mingled body connector and weapon connector :/
								pie_TRANSLATE( psShape->connectors[0].x,
											   psShape->connectors[0].z,
											   psShape->connectors[0].y  );
								//and draw the muzzle flash
								//animate for the duration of the flash only
								//Watermelon:change macro to actual accessor for each turret effect
								//psShape = MUZZLE_FLASH_PIE(psDroid,psDroid->player);
								psShape = (asWeaponStats[psDroid->asWeaps[i].nStat]).pMuzzleGraphic;
								if(psShape)
								{
									//assume no clan colours formuzzle effects
									if ((psShape->numFrames == 0) || (psShape->animInterval <= 0))//no anim so display one frame for a fixed time
									{
										if (gameTime < (psDroid->asWeaps[i].lastFired + BASE_MUZZLE_FLASH_DURATION))
										{
											pie_Draw3DShape(psShape, 0, 0, brightness, WZCOL_BLACK, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);//muzzle flash
										}
									}
									else
									{
										frame = (gameTime - psDroid->asWeaps[i].lastFired)/psShape->animInterval;
										if (frame < psShape->numFrames)
										{
											pie_Draw3DShape(psShape, frame, 0, brightness, WZCOL_BLACK, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);//muzzle flash
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
				/*	Get the mounting graphic - we've already moved to the right position
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				//Watermelon:sensor uses connectors[0]
				pie_MatBegin();
				//Watermelon:reset Z?
				dummyZ = pie_RotateProject(&zero, &screenCoords);
				/* vtol weapons inverted */
				if ( iConnector >= VTOL_CONNECTOR_START )
				{
					pie_MatRotZ( DEG_360/2 );//this might affect gun rotation
				}

				pie_TRANSLATE( psShapeTemp->connectors[0].x,
							   psShapeTemp->connectors[0].z,
							   psShapeTemp->connectors[0].y  );

				if(psDroid->turretRotation[0])
				{
					pie_MatRotY(DEG( (-(SDWORD)(psDroid->turretRotation[0])) ));
				}
				psShape = SENSOR_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour, brightness, specular, pieFlag, iPieData);
				}

				/* Get the sensor graphic, assuming it's there */
				psShape = SENSOR_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour, brightness, specular, pieFlag, iPieData);
				}
				/* Pop Matrix */
				pie_MatEnd();
				break;

			case DROID_CONSTRUCT:
			case DROID_CYBORG_CONSTRUCT:
				/*	Get the mounting graphic - we've already moved to the right position
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				//Watermelon:cyborg uses connectors[0]
				pie_MatBegin();
				//Watermelon:reset Z?
				dummyZ = pie_RotateProject(&zero, &screenCoords);
				/* vtol weapons inverted */
				if ( iConnector >= VTOL_CONNECTOR_START )
				{
					pie_MatRotZ( DEG_360/2 );//this might affect gun rotation
				}
				pie_TRANSLATE( psShapeTemp->connectors[0].x,
							   psShapeTemp->connectors[0].z,
							   psShapeTemp->connectors[0].y  );

				if(psDroid->turretRotation[0])
				{
					pie_MatRotY(DEG( (-(SDWORD)(psDroid->turretRotation[0])) ));
				}
				psShape = CONSTRUCT_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour, brightness, specular, pieFlag, iPieData);
				}

				/* translate for construct mount point if cyborg */
				if (cyborgDroid(psDroid) && psShape && psShape->nconnectors)
				{
					pie_TRANSLATE( psShape->connectors[0].x,
								   psShape->connectors[0].z,
								   psShape->connectors[0].y  );
				}

				/* Get the construct graphic assuming it's there */
				psShape = CONSTRUCT_IMD(psDroid,psDroid->player);

				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour, brightness, specular, pieFlag, iPieData);
				}
				/* Pop Matrix */
				pie_MatEnd();
				break;
			case DROID_ECM:
				/*	Get the mounting graphic - we've already moved to the right position
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				//Watermelon:ecm uses connectors[0]
				pie_MatBegin();
				//Watermelon:reset Z?
				dummyZ = pie_RotateProject(&zero, &screenCoords);
				/* vtol weapons inverted */
				if ( iConnector >= VTOL_CONNECTOR_START )
				{
					pie_MatRotZ( DEG_360/2 );//this might affect gun rotation
				}
				pie_TRANSLATE( psShapeTemp->connectors[0].x,
							   psShapeTemp->connectors[0].z,
							   psShapeTemp->connectors[0].y  );

				if(psDroid->turretRotation[0])
				{
					pie_MatRotY(DEG( (-(SDWORD)(psDroid->turretRotation[0])) ));
				}
				psShape = ECM_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0, colour, brightness, specular, pieFlag, iPieData);
				}

				/* Get the ECM graphic assuming it's there.... */
				psShape = ECM_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0, colour, brightness, specular, pieFlag, iPieData);
				}
				/* Pop Matrix */
				pie_MatEnd();
				break;
			case DROID_REPAIR:
			case DROID_CYBORG_REPAIR:
				/*	Get the mounting graphic - we've already moved to the right position
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				//Watermelon:cyborg uses connectors[0]
				pie_MatBegin();
				//Watermelon:reset Z?
				dummyZ = pie_RotateProject(&zero, &screenCoords);
				/* vtol weapons inverted */
				if ( iConnector >= VTOL_CONNECTOR_START )
				{
					pie_MatRotZ( DEG_360/2 );//this might affect gun rotation
				}
				pie_TRANSLATE( psShapeTemp->connectors[0].x,
							   psShapeTemp->connectors[0].z,
							   psShapeTemp->connectors[0].y  );

				if(psDroid->turretRotation[0])
				{
					pie_MatRotY(DEG( (-(SDWORD)(psDroid->turretRotation[0])) ));
				}
				psShape = REPAIR_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0, colour, brightness, specular, pieFlag, iPieData);
				}

				/* translate for construct mount point if cyborg */
				if (cyborgDroid(psDroid) && psShape && psShape->nconnectors)
				{
					pie_TRANSLATE( psShape->connectors[0].x,
								   psShape->connectors[0].z,
								   psShape->connectors[0].y  );
				}

				/* Get the Repair graphic assuming it's there.... */
				psShape = REPAIR_IMD(psDroid,psDroid->player);

				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0, colour, brightness, specular, pieFlag, iPieData);
					if(psShape->nconnectors && psDroid->action == DACTION_DROIDREPAIR)
					{
						pie_TRANSLATE( psShape->connectors[0].x,
									   psShape->connectors[0].z,
									   psShape->connectors[0].y  );
						pie_TRANSLATE(0,-20,0);

						psShape = getImdFromIndex(MI_FLAME);

						/* Rotate for droid */
						pie_MatRotY( DEG( (SDWORD)psDroid->direction ) );
						pie_MatRotX( DEG( -psDroid->pitch ) );
						pie_MatRotZ( DEG( -psDroid->roll ) );
						//Watermelon:rotate Y
					   	pie_MatRotY(DEG( -( (-(SDWORD)(psDroid->turretRotation[0])) ) ));

						iV_MatrixRotateY(-player.r.y);
						iV_MatrixRotateX(-player.r.x);
							/* Dither on software */

					   	pie_Draw3DShape(psShape, getStaticTimeValueRange(100,psShape->numFrames), 0, brightness, WZCOL_BLACK, pie_ADDITIVE, 140);
					  		/* Dither off software */

						iV_MatrixRotateX(player.r.x);
						iV_MatrixRotateY(player.r.y);
					}
				}
				/* Pop Matrix */
				pie_MatEnd();
					break;
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
		pie_Draw3DShape(psShape, 0, colour, brightness, specular, pieFlag, iPieData);
	}
}


void destroyFXDroid(DROID	*psDroid)
{
	UDWORD	i;
	iIMDShape	*psImd = NULL;
	SDWORD	widthScatter, breadthScatter, heightScatter;
	Vector3i pos;

 	widthScatter = TILE_UNITS/4;
	breadthScatter = TILE_UNITS/4;
	heightScatter = TILE_UNITS/5;
	for(i=0; i<5; i++)
	{
		pos.x = psDroid->pos.x + widthScatter - rand()%(2*widthScatter);
		pos.z = psDroid->pos.y + breadthScatter - rand()%(2*breadthScatter);
		pos.y = psDroid->pos.z + 16 +heightScatter;
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
						psImd = WEAPON_MOUNT_IMD(psDroid,psDroid->player);
					}
				}
				else
				{
					psImd = getRandomDebrisImd();
				}
				break;
			default:
				psImd = getRandomDebrisImd();
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
					psImd = WEAPON_IMD(psDroid,psDroid->player);
				}
				else
				{
					psImd = getRandomDebrisImd();
				}
				break;
			default:
				psImd = getRandomDebrisImd();
				break;
			}
			break;
		case 2:
		case 3:
		case 4:
			psImd = getRandomDebrisImd();
			break;
		}
		if(psImd)
		{
			addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,psImd,getPlayerColour(psDroid->player));
		}
		else
		{
			addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,getRandomDebrisImd(),0);
		}
	}
}


void	compPersonToBits(DROID *psDroid)
{
	Vector3i position;	//,rotation,velocity;
	iIMDShape	*headImd, *legsImd, *armImd, *bodyImd;
	UDWORD		groundHeight;
	UDWORD		col;

	if(!psDroid->visible[selectedPlayer])
	{
		/* We can't see the person or cyborg - so get out */
		return;
	}
	/* get bits pointers according to whether baba or cyborg*/
	if (cyborgDroid(psDroid))
	{
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
	groundHeight = psDroid->pos.z;
	position.z = psDroid->pos.y;


	/* Tell about player colour */
	col = getPlayerColour(psDroid->player);

	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,headImd,col);
	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,legsImd,col);
	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,armImd,col);
	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,bodyImd,col);
}


iIMDShape *getLeftPropulsionIMD(DROID *psDroid)
{
	UDWORD			bodyStat, propStat;
	iIMDShape		**imd;

	bodyStat = psDroid->asBits[COMP_BODY].nStat;
	propStat = psDroid->asBits[COMP_PROPULSION].nStat;

	imd = asBodyStats[bodyStat].ppIMDList;
	imd += (propStat * NUM_PROP_SIDES + LEFT_PROP);

	return *imd;
}


iIMDShape *getRightPropulsionIMD(DROID *psDroid)
{
	UDWORD			bodyStat, propStat;
	iIMDShape		**imd;

	bodyStat = psDroid->asBits[COMP_BODY].nStat;
	propStat = psDroid->asBits[COMP_PROPULSION].nStat;

	imd = asBodyStats[bodyStat].ppIMDList;
	imd += (propStat * NUM_PROP_SIDES + RIGHT_PROP);

	return *imd;
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
