/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include "projectile.h"
#include "transporter.h"
#include "mission.h"
#include <glm/gtx/transform.hpp>

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
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS && col < MAX_PLAYERS, "Bad colour setting");
	NetPlay.players[player].colour = col;
	return true;
}

UBYTE getPlayerColour(UDWORD pl)
{
	if (pl == MAX_PLAYERS)
	{
		return 0; // baba
	}
	ASSERT_OR_RETURN(0, pl < MAX_PLAYERS, "Invalid player number %u", pl);
	return NetPlay.players[pl].colour;
}

static glm::mat4 setMatrix(const Vector3i *Position, const Vector3i *Rotation, int scale)
{
	return glm::translate(*Position) *
		glm::rotate(UNDEG(DEG(Rotation->x)), glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(UNDEG(DEG(Rotation->y)), glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(UNDEG(DEG(Rotation->z)), glm::vec3(0.f, 0.f, 1.f)) *
		glm::scale(scale / 100.f, scale / 100.f, scale / 100.f);
}

UDWORD getComponentDroidRadius(DROID *)
{
	return 100;
}


UDWORD getComponentDroidTemplateRadius(DROID_TEMPLATE *)
{
	return 100;
}


UDWORD getComponentRadius(BASE_STATS *psComponent)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	SDWORD compID;

	compID = StatIsComponent(psComponent);
	if (compID >= 0)
	{
		StatGetComponentIMD(psComponent, compID, &ComponentIMD, &MountIMD);
		if (ComponentIMD)
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

	if (ResearchIMD)
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

UDWORD getStructureStatHeight(STRUCTURE_STATS *psStat)
{
	if (psStat->pIMD[0])
	{
		return (psStat->pIMD[0]->max.y - psStat->pIMD[0]->min.y);
	}

	return 0;
}

void displayIMDButton(iIMDShape *IMDShape, const Vector3i *Rotation, const Vector3i *Position, int scale)
{
	pie_Draw3DShape(IMDShape, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, setMatrix(Position, Rotation, scale));
}

static void sharedStructureButton(STRUCTURE_STATS *Stats, iIMDShape *strImd, const Vector3i *Rotation, const Vector3i *Position, int scale)
{
	iIMDShape *baseImd, *mountImd[MAX_WEAPONS], *weaponImd[MAX_WEAPONS];
	Vector3i pos = *Position;

	/* HACK HACK HACK!
	if its a 'tall thin (ie tower)' structure stat with something on the top - offset the position to show the object on top */
	if (strImd->nconnectors && scale == SMALL_STRUCT_SCALE && getStructureStatHeight(Stats) > TOWER_HEIGHT)
	{
		pos.y -= 20;
	}

	const glm::mat4 matrix = setMatrix(&pos, Rotation, scale);

	/* Draw the building's base first */
	baseImd = Stats->pBaseIMD;

	if (baseImd != NULL)
	{
		pie_Draw3DShape(baseImd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix);
	}
	pie_Draw3DShape(strImd, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix);

	//and draw the turret
	if (strImd->nconnectors)
	{
		weaponImd[0] = NULL;
		mountImd[0] = NULL;
		for (int i = 0; i < Stats->numWeaps; i++)
		{
			weaponImd[i] = NULL;//weapon is gun ecm or sensor
			mountImd[i] = NULL;
		}
		//get an imd to draw on the connector priority is weapon, ECM, sensor
		//check for weapon
		//can only have the MAX_WEAPONS
		for (int i = 0; i < MAX(1, Stats->numWeaps); i++)
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
			for (int i = 0; i < MAX(1, Stats->numWeaps); i++)
			{
				glm::mat4 localMatrix = glm::translate(strImd->connectors[i].xzy);
				if (mountImd[i] != NULL)
				{
					pie_Draw3DShape(mountImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix * localMatrix);
					if (mountImd[i]->nconnectors)
					{
						localMatrix *= glm::translate(mountImd[i]->connectors->xzy);
					}
				}
				pie_Draw3DShape(weaponImd[i], 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix * localMatrix);
				//we have a droid weapon so do we draw a muzzle flash
			}
		}
	}
}

void displayStructureButton(STRUCTURE *psStructure, const Vector3i *rotation, const Vector3i *Position, int scale)
{
	sharedStructureButton(psStructure->pStructureType, psStructure->sDisplay.imd, rotation, Position, scale);
}

void displayStructureStatButton(STRUCTURE_STATS *Stats, const Vector3i *rotation, const Vector3i *Position, int scale)
{
	sharedStructureButton(Stats, Stats->pIMD[0], rotation, Position, scale);
}

// Render a component given a BASE_STATS structure.
//
void displayComponentButton(BASE_STATS *Stat, const Vector3i *Rotation, const Vector3i *Position, int scale)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	int compID = StatIsComponent(Stat);

	if (compID >= 0)
	{
		StatGetComponentIMD(Stat, compID, &ComponentIMD, &MountIMD);
	}
	else
	{
		return;
	}
	glm::mat4 matrix = setMatrix(Position, Rotation, scale);

	/* VTOL bombs are only stats allowed to have NULL ComponentIMD */
	if (StatIsComponent(Stat) != COMP_WEAPON
	    || (((WEAPON_STATS *)Stat)->weaponSubClass != WSC_BOMB
	        && ((WEAPON_STATS *)Stat)->weaponSubClass != WSC_EMP))
	{
		ASSERT(ComponentIMD, "No ComponentIMD");
	}

	if (MountIMD)
	{
		pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix);

		/* translate for weapon mount point */
		if (MountIMD->nconnectors)
		{
			matrix *= glm::translate(MountIMD->connectors->xzy);
		}
	}
	if (ComponentIMD)
	{
		pie_Draw3DShape(ComponentIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix);
	}
}


// Render a research item given a BASE_STATS structure.
//
void displayResearchButton(BASE_STATS *Stat, const Vector3i *Rotation, const Vector3i *Position, int scale)
{
	iIMDShape *ResearchIMD = ((RESEARCH *)Stat)->pIMD;
	iIMDShape *MountIMD = ((RESEARCH *)Stat)->pIMD2;

	ASSERT(ResearchIMD, "ResearchIMD is NULL");
	if (ResearchIMD)
	{
		const glm::mat4 &matrix = setMatrix(Position, Rotation, scale);

		if (MountIMD)
		{
			pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix);
		}
		pie_Draw3DShape(ResearchIMD, 0, getPlayerColour(selectedPlayer), WZCOL_WHITE, pie_BUTTON, 0, matrix);
	}
}


static inline iIMDShape *getLeftPropulsionIMD(DROID *psDroid)
{
	int bodyStat = psDroid->asBits[COMP_BODY];
	int propStat = psDroid->asBits[COMP_PROPULSION];
	return asBodyStats[bodyStat].ppIMDList[propStat * NUM_PROP_SIDES + LEFT_PROP];
}

static inline iIMDShape *getRightPropulsionIMD(DROID *psDroid)
{
	int bodyStat = psDroid->asBits[COMP_BODY];
	int propStat = psDroid->asBits[COMP_PROPULSION];
	return asBodyStats[bodyStat].ppIMDList[propStat * NUM_PROP_SIDES + RIGHT_PROP];
}

void drawMuzzleFlash(WEAPON sWeap, iIMDShape *weaponImd, iIMDShape *flashImd, PIELIGHT buildingBrightness, int pieFlag, int iPieData, const glm::mat4 &viewMatrix, UBYTE colour)
{
	if (!weaponImd || !flashImd || !weaponImd->nconnectors || graphicsTime < sWeap.lastFired)
	{
		return;
	}

	int connector_num = 0;

	// which barrel is firing if model have multiple muzzle connectors?
	if (sWeap.shotsFired && (weaponImd->nconnectors > 1))
	{
		// shoot first, draw later - substract one shot to get correct results
		connector_num = (sWeap.shotsFired - 1) % (weaponImd->nconnectors);
	}

	/* Now we need to move to the end of the firing barrel */
	const glm::mat4 modelMatrix = glm::translate(weaponImd->connectors[connector_num].xzy);

	// assume no clan colours for muzzle effects
	if (flashImd->numFrames == 0 || flashImd->animInterval <= 0)
	{
		// no anim so display one frame for a fixed time
		if (graphicsTime >= sWeap.lastFired && graphicsTime < sWeap.lastFired + BASE_MUZZLE_FLASH_DURATION)
		{
			pie_Draw3DShape(flashImd, 0, colour, buildingBrightness, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE, viewMatrix * modelMatrix);
		}
	}
	else if (graphicsTime >= sWeap.lastFired)
	{
		// animated muzzle
		int frame = (graphicsTime - sWeap.lastFired) / flashImd->animInterval;
		if (frame < flashImd->numFrames)
		{
			pie_Draw3DShape(flashImd, frame, colour, buildingBrightness, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE, viewMatrix * modelMatrix);
		}
	}
}

/* Assumes matrix context is already set */
// this is able to handle multiple weapon graphics now
// removed mountRotation,they get such stuff from psObj directly now
static void displayCompObj(DROID *psDroid, bool bButton, const glm::mat4 &viewMatrix)
{
	iIMDShape *psMoveAnim, *psStillAnim;
	SDWORD				iConnector;
	PROPULSION_STATS	*psPropStats;
	SDWORD				pieFlag, iPieData;
	PIELIGHT			brightness;
	UDWORD				colour;
	UBYTE	i;

	glm::mat4 modelMatrix = glm::mat4(1.f);

	if (graphicsTime - psDroid->timeLastHit < GAME_TICKS_PER_SEC / 4 && psDroid->lastHitWeapon == WSC_ELECTRONIC && !gamePaused())
	{
		colour = getPlayerColour(rand() % MAX_PLAYERS);
	}
	else
	{
		colour = getPlayerColour(psDroid->player);
	}

	/* get propulsion stats */
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION];
	ASSERT_OR_RETURN(, psPropStats != NULL, "invalid propulsion stats pointer");

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
		if (!isTransporter(psDroid) && !missionIsOffworld())
		{
			MAPTILE *psTile = worldTile(psDroid->pos.x, psDroid->pos.y);
			if (psTile->jammerBits & alliancebits[psDroid->player])
			{
				pieFlag |= pie_ECM;
			}
		}
	}

	/* set default components transparent */
	if (psDroid->asBits[COMP_PROPULSION] == 0)
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
		modelMatrix *= glm::translate(0.f, -world_coord(1) / 2.3f, 0.f);
	}

	iIMDShape *psShapeProp = (leftFirst ? getLeftPropulsionIMD(psDroid) : getRightPropulsionIMD(psDroid));
	if (psShapeProp)
	{
		pie_Draw3DShape(psShapeProp, 0, colour, brightness, pieFlag, iPieData, viewMatrix * modelMatrix);
	}

	/* set default components transparent */
	if (psDroid->asBits[COMP_BODY] == 0)
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
	iIMDShape *psShapeBody = BODY_IMD(psDroid, psDroid->player);
	if (psShapeBody)
	{
		iIMDShape *strImd = psShapeBody;
		if (psDroid->droidType == DROID_PERSON)
		{
			modelMatrix *= glm::scale(.75f, .75f, .75f); // FIXME - hideous....!!!!
		}
		if (strImd->objanimpie[psDroid->animationEvent])
		{
			strImd = psShapeBody->objanimpie[psDroid->animationEvent];
		}
		while (strImd)
		{
			drawShape(psDroid, strImd, colour, brightness, pieFlag, iPieData, viewMatrix * modelMatrix);
			strImd = strImd->next;
		}
	}

	/* Render animation effects based on movement or lack thereof, if any */
	psMoveAnim = asBodyStats[psDroid->asBits[COMP_BODY]].ppMoveIMDList[psDroid->asBits[COMP_PROPULSION]];
	psStillAnim = asBodyStats[psDroid->asBits[COMP_BODY]].ppStillIMDList[psDroid->asBits[COMP_PROPULSION]];
	if (!bButton && psMoveAnim && psDroid->sMove.Status != MOVEINACTIVE)
	{
		pie_Draw3DShape(psMoveAnim, getModularScaledGraphicsTime(psMoveAnim->animInterval, psMoveAnim->numFrames), colour, brightness, pie_ADDITIVE, 200, viewMatrix * modelMatrix);
	}
	else if (!bButton && psStillAnim) // standing still
	{
		pie_Draw3DShape(psStillAnim, getModularScaledGraphicsTime(psStillAnim->animInterval, psStillAnim->numFrames), colour, brightness, 0, 0, viewMatrix * modelMatrix);
	}

	//don't change the screen coords of an object if drawing it in a button
	if (!bButton)
	{
		/* set up all the screen coords stuff - need to REMOVE FROM THIS LOOP */
		calcScreenCoords(psDroid, viewMatrix * modelMatrix);
	}

	/* set default components transparent */
	if (psDroid->asWeaps[0].nStat        == 0 &&
	    psDroid->asBits[COMP_SENSOR]     == 0 &&
	    psDroid->asBits[COMP_ECM]        == 0 &&
	    psDroid->asBits[COMP_BRAIN]      == 0 &&
	    psDroid->asBits[COMP_REPAIRUNIT] == 0 &&
	    psDroid->asBits[COMP_CONSTRUCT]  == 0)
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	if (psShapeBody && psShapeBody->nconnectors)
	{
		/* vtol weapons attach to connector 2 (underneath);
		 * all others to connector 1 */
		/* VTOL's now skip the first 5 connectors(0 to 4),
		VTOL's use 5,6,7,8 etc now */
		if (psPropStats->propulsionType == PROPULSION_TYPE_LIFT && psDroid->droidType == DROID_WEAPON)
		{
			iConnector = VTOL_CONNECTOR_START;
		}
		else
		{
			iConnector = 0;
		}

		switch (psDroid->droidType)
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
			for (i = 0; i < psDroid->numWeaps; i++)
			{
				if ((psDroid->asWeaps[i].nStat > 0 || psDroid->droidType == DROID_DEFAULT)
				    && psShapeBody->connectors)
				{
					Rotation rot = getInterpolatedWeaponRotation(psDroid, i, graphicsTime);

					glm::mat4 localModelMatrix = modelMatrix;

					//to skip number of VTOL_CONNECTOR_START ground unit connectors
					if (iConnector < VTOL_CONNECTOR_START)
					{
						localModelMatrix *= glm::translate(psShapeBody->connectors[i].xzy);
					}
					else
					{
						localModelMatrix *= glm::translate(psShapeBody->connectors[iConnector + i].xzy);
					}
					localModelMatrix *= glm::rotate(UNDEG(-rot.direction), glm::vec3(0.f, 1.f, 0.f));

					/* vtol weapons inverted */
					if (iConnector >= VTOL_CONNECTOR_START)
					{
						//this might affect gun rotation
						localModelMatrix *= glm::rotate(UNDEG(65536 / 2), glm::vec3(0.f, 0.f, 1.f));
					}

					/* Get the mount graphic */
					iIMDShape *psShape = WEAPON_MOUNT_IMD(psDroid, i);

					int recoilValue = getRecoil(psDroid->asWeaps[i]);
					localModelMatrix *= glm::translate(0.f, 0.f, recoilValue / 3.f);

					/* Draw it */
					if (psShape)
					{
						pie_Draw3DShape(psShape, 0, colour, brightness, pieFlag, iPieData, viewMatrix * localModelMatrix);
					}
					localModelMatrix *= glm::translate(0, 0, recoilValue);

					/* translate for weapon mount point */
					if (psShape && psShape->nconnectors)
					{
						localModelMatrix *= glm::translate(psShape->connectors->xzy);
					}

					/* vtol weapons inverted */
					if (iConnector >= VTOL_CONNECTOR_START)
					{
						//pitch the barrel down
						localModelMatrix *= glm::rotate(UNDEG(-rot.pitch), glm::vec3(1.f, 0.f, 0.f));
					}
					else
					{
						//pitch the barrel up
						localModelMatrix *= glm::rotate(UNDEG(rot.pitch), glm::vec3(1.f, 0.f, 0.f));
					}

					/* Get the weapon (gun?) graphic */
					psShape = WEAPON_IMD(psDroid, i);

					// We have a weapon so we draw it and a muzzle flash from weapon connector
					if (psShape)
					{
						pie_Draw3DShape(psShape, 0, colour, brightness, pieFlag, iPieData, viewMatrix * localModelMatrix);
						drawMuzzleFlash(psDroid->asWeaps[i], psShape, MUZZLE_FLASH_PIE(psDroid, i), brightness, pieFlag, iPieData, viewMatrix * localModelMatrix);
					}
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
				iIMDShape *psShape = nullptr;
				iIMDShape *psMountShape = nullptr;

				switch (psDroid->droidType)
				{
				default:
					ASSERT(false, "Bad component type");
					break;
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

				glm::mat4 localModelMatrix = modelMatrix;
				/* vtol weapons inverted */
				if (iConnector >= VTOL_CONNECTOR_START)
				{
					//this might affect gun rotation
					localModelMatrix *= glm::rotate(UNDEG(65536 / 2), glm::vec3(0.f, 0.f, 1.f));
				}

				localModelMatrix *= glm::translate(psShapeBody->connectors[0].xzy);

				localModelMatrix *= glm::rotate(UNDEG(-rot.direction), glm::vec3(0.f, 1.f, 0.f));
				/* Draw it */
				if (psMountShape)
				{
					pie_Draw3DShape(psMountShape, 0, colour, brightness, pieFlag, iPieData, viewMatrix * localModelMatrix);
				}

				/* translate for construct mount point if cyborg */
				if (cyborgDroid(psDroid) && psMountShape && psMountShape->nconnectors)
				{
					localModelMatrix *= glm::translate(psMountShape->connectors[0].xzy);
				}

				/* Draw it */
				if (psShape)
				{
					pie_Draw3DShape(psShape, 0, colour, brightness, pieFlag, iPieData, viewMatrix * localModelMatrix);

					// In repair droid case only:
					if ((psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR) &&
					    psShape->nconnectors && psDroid->action == DACTION_DROIDREPAIR)
					{
						Spacetime st = interpolateObjectSpacetime(psDroid, graphicsTime);
						localModelMatrix *= glm::translate(psShape->connectors[0].xzy);
						localModelMatrix *= glm::translate(0.f, -20.f, 0.f);

						psShape = getImdFromIndex(MI_FLAME);

						/* Rotate for droid */
						localModelMatrix *= glm::rotate(UNDEG(st.rot.direction), glm::vec3(0.f, 1.f, 0.f));
						localModelMatrix *= glm::rotate(UNDEG(-st.rot.pitch), glm::vec3(1.f, 0.f, 0.f));
						localModelMatrix *= glm::rotate(UNDEG(-st.rot.roll), glm::vec3(0.f, 0.f, 1.f));
						//rotate Y
						localModelMatrix *= glm::rotate(UNDEG(rot.direction), glm::vec3(0.f, 1.f, 0.f));

						localModelMatrix *= glm::rotate(UNDEG(-player.r.y), glm::vec3(0.f, 1.f, 0.f));
						localModelMatrix *= glm::rotate(UNDEG(-player.r.x), glm::vec3(1.f, 0.f, 0.f));

						pie_Draw3DShape(psShape, getModularScaledGraphicsTime(psShape->animInterval, psShape->numFrames), 0, brightness, pie_ADDITIVE, 140, viewMatrix * localModelMatrix);

						localModelMatrix *= glm::rotate(UNDEG(player.r.x), glm::vec3(1.f, 0.f, 0.f));
						localModelMatrix *= glm::rotate(UNDEG(player.r.y), glm::vec3(0.f, 1.f, 0.f));
					}
				}
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

	/* set default components transparent */
	if (psDroid->asBits[COMP_PROPULSION] == 0)
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	// now render the other propulsion side
	psShapeProp = (leftFirst ? getRightPropulsionIMD(psDroid) : getLeftPropulsionIMD(psDroid));
	if (psShapeProp)
	{
		pie_Draw3DShape(psShapeProp, 0, colour, brightness, pieFlag, iPieData, viewMatrix * modelMatrix);
	}
}


// Render a composite droid given a DROID_TEMPLATE structure.
//
void displayComponentButtonTemplate(DROID_TEMPLATE *psTemplate, const Vector3i *Rotation, const Vector3i *Position, int scale)
{
	const glm::mat4 matrix = setMatrix(Position, Rotation, scale);

	// Decide how to sort it.
	leftFirst = angleDelta(DEG(Rotation->y)) < 0;

	DROID Droid(0, selectedPlayer);
	memset(Droid.asBits, 0, sizeof(Droid.asBits));
	droidSetBits(psTemplate, &Droid);

	Droid.pos = Vector3i(0, 0, 0);
	Droid.rot = Vector3i(0, 0, 0);

	//draw multi component object as a button object
	displayCompObj(&Droid, true, matrix);
}


// Render a composite droid given a DROID structure.
//
void displayComponentButtonObject(DROID *psDroid, const Vector3i *Rotation, const Vector3i *Position, int scale)
{
	SDWORD		difference;

	const glm::mat4 matrix = setMatrix(Position, Rotation, scale);

	// Decide how to sort it.
	difference = Rotation->y % 360;

	leftFirst = !((difference > 0 && difference < 180) || difference < -180);

	// And render the composite object.
	//draw multi component object as a button object
	displayCompObj(psDroid, true, matrix);
}


/* Assumes matrix context is already set */
// multiple turrets display removed the pointless mountRotation
void displayComponentObject(DROID *psDroid, const glm::mat4 &viewMatrix)
{
	Vector3i position, rotation;
	Spacetime st = interpolateObjectSpacetime(psDroid, graphicsTime);

	leftFirst = angleDelta(player.r.y - st.rot.direction) <= 0;

	/* Get the real position */
	position.x = st.pos.x - player.p.x;
	position.z = -(st.pos.y - player.p.z);
	position.y = st.pos.z;

	if (isTransporter(psDroid))
	{
		position.y += bobTransporterHeight();
	}

	/* Get all the pitch,roll,yaw info */
	rotation.y = -st.rot.direction;
	rotation.x = st.rot.pitch;
	rotation.z = st.rot.roll;

	/* Translate origin */
	/* Rotate for droid */
	glm::mat4 modelMatrix = glm::translate(position) *
		glm::rotate(UNDEG(rotation.y), glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(UNDEG(rotation.x), glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(UNDEG(rotation.z), glm::vec3(0.f, 0.f, 1.f));

	if (graphicsTime - psDroid->timeLastHit < GAME_TICKS_PER_SEC && psDroid->lastHitWeapon == WSC_ELECTRONIC)
	{
		modelMatrix *= objectShimmy((BASE_OBJECT *) psDroid);
	}

	if (psDroid->lastHitWeapon == WSC_EMP && graphicsTime - psDroid->timeLastHit < EMP_DISABLE_TIME)
	{
		Vector3i position;

		//add an effect on the droid
		position.x = st.pos.x + DROID_EMP_SPREAD;
		position.y = st.pos.z + rand() % 8;
		position.z = st.pos.y + DROID_EMP_SPREAD;
		effectGiveAuxVar(90 + rand() % 20);
		addEffect(&position, EFFECT_EXPLOSION, EXPLOSION_TYPE_PLASMA, false, NULL, 0);
	}

	if (psDroid->visible[selectedPlayer] == UBYTE_MAX)
	{
		//ingame not button object
		//should render 3 mounted weapons now
		displayCompObj(psDroid, false, viewMatrix * modelMatrix);
	}
	else
	{
		int frame = graphicsTime / BLIP_ANIM_DURATION + psDroid->id % 8192; // de-sync the blip effect, but don't overflow the int
		pie_Draw3DShape(getImdFromIndex(MI_BLIP), frame, 0, WZCOL_WHITE, pie_ADDITIVE, psDroid->visible[selectedPlayer] / 2, viewMatrix * modelMatrix);
	}
}


void destroyFXDroid(DROID *psDroid, unsigned impactTime)
{
	for (int i = 0; i < 5; ++i)
	{
		iIMDShape *psImd = NULL;

		int maxHorizontalScatter = TILE_UNITS / 4;
		int heightScatter = TILE_UNITS / 5;
		Vector2i horizontalScatter = iSinCosR(rand(), rand() % maxHorizontalScatter);

		Vector3i pos = (psDroid->pos + Vector3i(horizontalScatter, 16 + heightScatter)).xzy;
		switch (i)
		{
		case 0:
			switch (psDroid->droidType)
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
					if (psDroid->asWeaps[0].nStat > 0)
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
			switch (psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_CYBORG_CONSTRUCT:
			case DROID_CYBORG_REPAIR:
			case DROID_WEAPON:
			case DROID_COMMAND:
				if (psDroid->numWeaps)
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

	if (!psDroid->visible[selectedPlayer])
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
	position.y = psDroid->pos.z + 1;
	position.z = psDroid->pos.y;

	/* Tell about player colour */
	col = getPlayerColour(psDroid->player);

	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, headImd, col, gameTime - deltaGameTime + 1);
	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, legsImd, col, gameTime - deltaGameTime + 1);
	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, armImd, col, gameTime - deltaGameTime + 1);
	addEffect(&position, EFFECT_GRAVITON, GRAVITON_TYPE_GIBLET, true, bodyImd, col, gameTime - deltaGameTime + 1);
}


SDWORD	rescaleButtonObject(SDWORD radius, SDWORD baseScale, SDWORD baseRadius)
{
	SDWORD newScale;
	newScale = 100 * baseRadius;
	newScale /= radius;
	if (baseScale > 0)
	{
		newScale += baseScale;
		newScale /= 2;
	}
	return newScale;
}
