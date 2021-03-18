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
/** @file
 *  Definitions for the display system structures and routines.
 */

#ifndef __INCLUDED_SRC_DISPLAY_H__
#define __INCLUDED_SRC_DISPLAY_H__

#include "basedef.h"
#include "structure.h"
#include <glm/fwd.hpp>

/* Initialise the display system */
bool dispInitialise();

/* Initialize fade-in transition */
bool transitionInit();

void ProcessRadarInput();

void processInput();
/*don't want to do any of these whilst in the Intelligence Screen*/
void processMouseClickInput();

void resetScroll();

bool DrawnInLastFrame(SDWORD Frame);

// Clear all selections
void clearSelection();
// deal with selecting a droid
void dealWithDroidSelect(DROID *psDroid, bool bDragBox);

bool isMouseOverRadar();

void	setInvertMouseStatus(bool val);
bool	getInvertMouseStatus();

void	setRightClickOrders(bool val);
bool	getRightClickOrders();

void	setMiddleClickRotate(bool val);
bool	getMiddleClickRotate();

void	setDrawShadows(bool val);
bool	getDrawShadows();

bool	getCameraAccel();
void	setCameraAccel(bool val);

/* Do the 3D display */
void displayWorld();

// Illumination value for standard light level "as the artist drew it" ... not darker, not lighter

enum DragState
{
	DRAG_INACTIVE,
	DRAG_DRAGGING,
	DRAG_RELEASED,
	DRAG_PLACING
};

struct DragBox3D
{
	int x1;
	int y1;
	int x2;
	int y2;
	DragState status;
	float pulse = 0;
};

struct WallDrag
{
	Vector2i pos;
	Vector2i pos2;
	DragState status;
};

extern DragBox3D dragBox3D;
extern WallDrag wallDrag;

enum MOUSE_POINTER
{
	MP_ATTACH = 99,
	MP_ATTACK,
	MP_BRIDGE,
	MP_BUILD,
	MP_EMBARK,
	MP_FIX,
	MP_GUARD,
	MP_JAM,
	MP_MOVE,
	MP_PICKUP,
	MP_REPAIR,
	MP_SELECT,
	MP_LOCKON,
	MP_MENSELECT,
	MP_BOMB
};

enum SELECTION_TYPE
{
	SC_DROID_CONSTRUCT,
	SC_DROID_DIRECT,
	SC_DROID_INDIRECT,
	SC_DROID_CLOSE,
	SC_DROID_SENSOR,
	SC_DROID_ECM,
	SC_DROID_BRIDGE,
	SC_DROID_RECOVERY,
	SC_DROID_COMMAND,
	SC_DROID_BOMBER,
	SC_DROID_TRANSPORTER,
	SC_DROID_SUPERTRANSPORTER,
	SC_DROID_DEMOLISH,
	SC_DROID_REPAIR,
	SC_INVALID,

};

enum MOUSE_TARGET
{
	MT_TERRAIN,
	MT_RESOURCE,
	MT_BLOCKING,
	MT_RIVER,
	MT_TRENCH,
	MT_OWNSTRDAM,
	MT_OWNSTROK,
	MT_OWNSTRINCOMP,
	MT_REPAIR,
	MT_REPAIRDAM,
	MT_ENEMYSTR,
	MT_TRANDROID,
	MT_OWNDROID,
	MT_OWNDROIDDAM,
	MT_ENEMYDROID,
	MT_COMMAND,
	MT_ARTIFACT,
	MT_DAMFEATURE,
	MT_SENSOR,
	MT_UNUSED,
	MT_CONSTRUCT,
	MT_SENSORSTRUCT,
	MT_SENSORSTRUCTDAM,

	MT_NOTARGET		//leave as last one
};

extern bool		gameStats;
extern bool		godMode;
extern bool		lockCameraScrollWhileRotating;

extern bool getShakeStatus();
extern void setShakeStatus(bool val);
extern void shakeStart(unsigned int length);
extern void shakeStop();

// reset the input state
void resetInput();

bool CheckInScrollLimits(SDWORD *xPos, SDWORD *zPos);
bool CheckScrollLimits();

BASE_OBJECT	*mouseTarget();

void cancelDeliveryRepos();
void startDeliveryPosition(FLAG_POSITION *psFlag);
bool deliveryReposValid();
void processDeliveryRepos();
void renderDeliveryRepos(const glm::mat4 &viewMatrix);
bool deliveryReposFinished(FLAG_POSITION *psFlag = nullptr);

bool	getRotActive();

#define MAX_PLAYER_X_ANGLE	(-1)
#define MIN_PLAYER_X_ANGLE	(-90)

#define MAXDISTANCE	(5000)
#define MINDISTANCE	(0)
#define MINDISTANCE_CONFIG (1600)
#define MAP_ZOOM_CONFIG_STEP (200)
#define STARTDISTANCE	(2500)
#define OLD_START_HEIGHT (1500) // only used in savegames <= 10

#define CAMERA_PIVOT_HEIGHT (500)

#define INITIAL_STARTING_PITCH (-40)
#define OLD_INITIAL_ROTATION (-45) // only used in savegames <= 10

#define	HIDDEN_FRONTEND_WIDTH	(640)
#define	HIDDEN_FRONTEND_HEIGHT	(480)

//access function for bSensorAssigned variable
void setSensorAssigned();

void AddDerrickBurningMessage();

// check whether the queue order keys are pressed
bool ctrlShiftDown();

#define	DEFAULT_VIEW_DISTANCE_ANIMATION_SPEED (5000)

void animateToViewDistance(float target, float speed = DEFAULT_VIEW_DISTANCE_ANIMATION_SPEED);
void incrementViewDistance(float amount);
void displayRenderLoop();
bool clipXYZ(int x, int y, int z, const glm::mat4 &viewMatrix);
bool clipXYZNormalized(const Vector3i &normalizedPosition, const glm::mat4 &viewMatrix);

#endif // __INCLUDED_SRC_DISPLAY_H__
