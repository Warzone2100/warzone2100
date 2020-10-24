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
/**
 * @file drive.cpp
 * Handles driving.
 */
#include "lib/framework/frameresource.h"
#include "lib/framework/input.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/piemode.h"

#include <glm/gtx/transform.hpp>

#include "effects.h"
#include "map.h"
#include "loop.h"
#include "move.h"
#include "order.h"
#include "action.h"
#include "intdisplay.h"
#include "display3d.h"
#include "selection.h"
#include "projectile.h"
#include "combat.h"
#include "animation.h"
#include "display3d.h"

bool driving;
BASE_OBJECT *drivingTarget = nullptr;
STRUCTURE *drivingStructure = nullptr;
DROID *drivingDroid = nullptr;
float driveDir;
float driveSpeed;
#define DRIVE_TURNSPEED	(40)
#define DRIVE_ACCELERATE (40)
#define DRIVE_BRAKE (40)
#define DRIVE_DECELERATE (20)

const int CAMERA_DISTANCE = 500;
const float CAMERA_ROTATION_SMOOTHNESS = 10.0f;
const float CAMERA_ROTATION_SPEED = 100.0f;

bool isMouseRotating = false;
SDWORD	oldMouseX;
SDWORD	oldMouseY;
int	rotInitialHorizontal;
int	rotInitialVertical;
float	rotationHorizontal;
float	rotationVertical;
int previousCameraDistance;
int cameraHorizontal;
int cameraVertical;

bool isTransitioningIn = false;
bool isTransitioningOut = false;
UDWORD cameraTransitionStopTime;
int cameraTransitionDuration = 2000;
std::unique_ptr<AngleAnimation> cameraTransitionRotation = std::unique_ptr<AngleAnimation>(new AngleAnimation());
Animation<Vector3f> cameraTransitionPosition(graphicsTime, EASE_IN_OUT);
Animation<float> cameraTransitionDistance(graphicsTime, EASE_IN_OUT);

bool tryGetDrivingTarget(){
	for (auto psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			drivingTarget = psDroid;
			drivingDroid = psDroid;
			return true;
		}
	}

	for (auto psStructure = apsStructLists[selectedPlayer]; psStructure; psStructure = psStructure->psNext)
	{
		if (psStructure->selected)
		{
			drivingTarget = psStructure;
			drivingStructure = psStructure;
			return true;
		}
	}

	return false;
}

void startDrivingModeMouseRotation();
void stopDrivingModeMouseRotation();

void startDriving()
{
	if(bMultiPlayer){
		return;
	}
	if(!tryGetDrivingTarget()){
		return;
	}

    driving = true;
	auto st = interpolateObjectSpacetime(drivingTarget, graphicsTime);
	driveDir = (int)(UNDEG(st.rot.direction) / RADIANS(1));
	driveSpeed = 0;

	if(drivingDroid){
		drivingDroid->sMove.Status = MOVEDRIVE;
	}

	cameraHorizontal = DEG(180);
	cameraVertical = 0;

	cameraTransitionStopTime = graphicsTime + cameraTransitionDuration;

	previousCameraDistance = getViewDistance();
	cameraTransitionDistance
		.setInitialData(getViewDistance())
		.setFinalData(CAMERA_DISTANCE)
		.setDuration(cameraTransitionDuration)
		.start();
	cameraTransitionPosition
		.setInitialData(player.p)
		.setDuration(cameraTransitionDuration)
		.start();
	cameraTransitionRotation
		->setInitial(player.r)
		->setDuration(cameraTransitionDuration)
		->startNow();

	isTransitioningIn = true;

	setWidgetsStatus(false); // hide UI
}

void stopDriving()
{
	if(isTransitioningOut){
		return;
	}

	isTransitioningOut = true;

	cameraTransitionStopTime = graphicsTime + cameraTransitionDuration;

	cameraTransitionDistance
		.setInitialData(getViewDistance())
		.setFinalData(previousCameraDistance)
		.setDuration(cameraTransitionDuration)
		.start();
	cameraTransitionPosition
		.setInitialData(player.p)
		.setFinalData({ player.p.x, calculateCameraHeight(averageCentreTerrainHeight), player.p.z })
		.setDuration(cameraTransitionDuration)
		.start();
	cameraTransitionRotation
		->setInitial(player.r)
		->setTarget(INITIAL_STARTING_PITCH, player.r.y, player.r.z)
		->setDuration(cameraTransitionDuration)
		->startNow();

	auto rotX = player.r.x;

	if(rotX < DEG(180)){
		rotX += DEG(360);
	}

	driveDir = 0;
	driveSpeed = 0;
	drivingTarget = nullptr;
	drivingStructure = nullptr;
	if(drivingDroid){
		drivingDroid->sMove.Status = MOVEINACTIVE;
	}
	drivingDroid = nullptr;

	if(isMouseRotating){
		stopDrivingModeMouseRotation();
	}

	setWidgetsStatus(true); // show UI
}

bool shouldUpdateDriveModeClick;

void driveModeClick(){
	if(isTransitioningOut){
		return;
	}
	shouldUpdateDriveModeClick = true;
}

void driveModeClickUpdate(){
	if(isTransitioningOut){
		return;
	}
	if(!shouldUpdateDriveModeClick){
		return;
	}

	shouldUpdateDriveModeClick = false;

	auto psClickedOn = mouseTarget();

	if(psClickedOn == nullptr){
		return;
	}

	if(psClickedOn == drivingTarget){
		return;
	}

	if(psClickedOn->type != OBJ_DROID && psClickedOn->type != OBJ_STRUCTURE){
		return;
	}

	if(drivingDroid && !actionVisibleTarget(drivingDroid, psClickedOn, 0)){
		printf("Can't see droid\n");
		return;
	}

	if(drivingDroid && !actionInRange(drivingDroid, psClickedOn, 0, false)){
		printf("Out of range\n");
		return;
	}

	WEAPON_STATS *const psWeapStats = &asWeaponStats[drivingTarget->asWeaps[0].nStat];
	auto blockingWall = visGetBlockingWall(drivingTarget, psClickedOn);
	if(proj_Direct(psWeapStats) && blockingWall != nullptr){
		printf("Blocked by wall -- firing on the wall!\n");
		combFire(&drivingTarget->asWeaps[0], drivingTarget, blockingWall, 0);
		return;
	}

	/* In range - fire !!! */
	combFire(&drivingTarget->asWeaps[0], drivingTarget, psClickedOn, 0);
}

void startDrivingModeMouseRotation(){
	isMouseRotating = true;
	oldMouseX = mouseX();
	oldMouseY = mouseY();
	rotInitialHorizontal = cameraHorizontal;
	rotInitialVertical = cameraVertical; // (drivingTarget->asWeaps[0].rot.direction % 65536) / DEG(1.0f);
	rotationHorizontal = rotInitialHorizontal; // instead of player.r.x, will track decimals for us
	rotationVertical = rotInitialVertical; // instead of player.r.y, will track decimals for us
}

void stopDrivingModeMouseRotation(){
	isMouseRotating = false;
	oldMouseX = 0;
	oldMouseY = 0;
	rotInitialHorizontal = 0;
	rotInitialVertical = 0;
	rotationHorizontal = 0;
	rotationVertical = 0;
}

void processDrivingCamera();
void processDrivingControls();
void processDrivingMouseRotation();

void processDriving(){
	if(isTransitioningIn && graphicsTime > cameraTransitionStopTime){
		isTransitioningIn = false;
	}

	processDrivingCamera();

	if(isTransitioningOut) {
		if(graphicsTime > cameraTransitionStopTime){
			isTransitioningOut = false;
			driving = false;
		}
		return;
	}

	if(isTransitioningIn){
		return;
	}

	processDrivingControls();
	processDrivingMouseRotation();
}

void processDrivingMouseRotation()
{
	if(mousePressed(MOUSE_RMB)){
		startDrivingModeMouseRotation();
	}
	if(mouseReleased(MOUSE_RMB)){
		stopDrivingModeMouseRotation();
		return;
	}

	if(!isMouseRotating && drivingTarget->numWeaps > 0)
	{
		Vector2i pos;
		locate2DCoordsIn3D(Vector2i(mouseX(), mouseY()), pos);

		Spacetime st = interpolateObjectSpacetime(drivingTarget, graphicsTime);
		Vector2i deltaPos = pos - st.pos.xy();
		uint16_t targetDir = iAtan2(deltaPos.xy());
		drivingTarget->asWeaps[0].rot.direction = targetDir - st.rot.direction;
	}

	if(!isMouseRotating){
		return;
	}

	float mouseDeltaX = mouseX() - oldMouseX;
	float mouseDeltaY = mouseY() - oldMouseY;

	// the subtraction and then addition of rotationHorizontal is for the bug where wrapping between eg 350-10 degrees didn't work
	rotationHorizontal = (rotInitialHorizontal - mouseDeltaX * CAMERA_ROTATION_SPEED - rotationHorizontal) * realTimeAdjustedIncrement(CAMERA_ROTATION_SMOOTHNESS) + rotationHorizontal;
	cameraHorizontal = rotationHorizontal;
	
	// if(drivingTarget->numWeaps > 0)
	// {
	// 	drivingTarget->asWeaps[0].rot.direction = DEG(rotationHorizontal);
	// }

	// the subtraction and then addition of rotationVertical is for the bug where wrapping between eg 350-10 degrees didn't work
	rotationVertical = (rotInitialVertical - mouseDeltaY * CAMERA_ROTATION_SPEED - rotationVertical) * realTimeAdjustedIncrement(CAMERA_ROTATION_SMOOTHNESS) + rotationVertical;
	cameraVertical = rotationVertical; // saved in a separate (float) variable in order to keep decimals for smoothness

	// TODO: Clamp this is a meaningful way!
	// if(player.r.x <= -1){
	// 	player.r.x = glm::clamp(player.r.x, DEG(-90), DEG(-1));
	// } else {
	// 	player.r.x = glm::clamp(player.r.x, DEG(0), DEG(15));
	// }
}

BASE_OBJECT *drawBoxOn = nullptr;

void processDrivingCursor(){
	wzSetCursor(CURSOR_DEFAULT);
	drawBoxOn = nullptr;

	if(isTransitioningIn || isTransitioningOut){
		return;
	}

	if(isMouseRotating){
		return;
	}
	
	if(drivingTarget->numWeaps == 0){
		return;
	}

	auto psClickedOn = mouseTarget();

	if(psClickedOn == nullptr){
		return;
	}

	if(psClickedOn == drivingTarget){
		return;
	}

	if(psClickedOn->type != OBJ_DROID && psClickedOn->type != OBJ_STRUCTURE){
		return;
	}

	// TODO: Do this for both droids and structures
	if(drivingDroid && !actionInRange(drivingDroid, psClickedOn, 0, false)){
		wzSetCursor(CURSOR_NOTPOSSIBLE);
		return;
	}

	if(psClickedOn->type == OBJ_DROID){
		drawBoxOn = psClickedOn;
	}

	wzSetCursor(CURSOR_ATTACK);
}

void drawDriveModeBoxes(){
	if(drawBoxOn == nullptr){
		return;
	}

	drawDroidSelection(castDroid(drawBoxOn), true);
}

extern int scrollDirUpDown;
extern int scrollDirLeftRight;

void processDrivingControls()
{
	if(!drivingDroid){
		return;
	}

	SDWORD MaxSpeed = moveCalcDroidSpeed(drivingDroid);

	if (scrollDirLeftRight < 0)
	{
		driveDir += realTimeAdjustedIncrement(DRIVE_TURNSPEED);
		if (driveDir > 360)
		{
			driveDir -= 360;
		}
	}
	else if (scrollDirLeftRight > 0)
	{
		driveDir -= realTimeAdjustedIncrement(DRIVE_TURNSPEED);
		if (driveDir < 0)
		{
			driveDir += 360;
		}
	}

	if (scrollDirUpDown > 0)
	{
		if (driveSpeed >= 0)
		{
			driveSpeed += realTimeAdjustedIncrement(DRIVE_ACCELERATE);
			if (driveSpeed > MaxSpeed)
			{
				driveSpeed = MaxSpeed;
			}
		}
		else
		{
			driveSpeed += realTimeAdjustedIncrement(DRIVE_BRAKE);
			if (driveSpeed > 0)
			{
				driveSpeed = 0;
			}
		}
	}
	else if (scrollDirUpDown < 0)
	{
		if (driveSpeed <= 0)
		{
			driveSpeed -= realTimeAdjustedIncrement(DRIVE_ACCELERATE);
			if (driveSpeed < -MaxSpeed)
			{
				driveSpeed = -MaxSpeed;
			}
		}
		else
		{
			driveSpeed -= realTimeAdjustedIncrement(DRIVE_BRAKE);
			if (driveSpeed < 0)
			{
				driveSpeed = 0;
			}
		}
	}
	else
	{
		if (driveSpeed > 0)
		{
			driveSpeed -= realTimeAdjustedIncrement(DRIVE_DECELERATE);
			if (driveSpeed < 0)
			{
				driveSpeed = 0;
			}
		}
		else
		{
			driveSpeed += realTimeAdjustedIncrement(DRIVE_DECELERATE);
			if (driveSpeed > 0)
			{
				driveSpeed = 0;
			}
		}
	}

	scrollDirLeftRight = 0;
	scrollDirUpDown = 0;
}

void processDrivingCamera()
{
	if(isTransitioningOut){
		player.p = cameraTransitionPosition.update().getCurrent();
		player.r = cameraTransitionRotation->update()->getCurrent();
		setViewDistance(cameraTransitionDistance.update().getCurrent());

		return;
	}

	auto st = interpolateObjectSpacetime(drivingTarget, graphicsTime);

	if(isTransitioningIn){
		player.p = cameraTransitionPosition
			.setFinalData({ st.pos.x, st.pos.z + 200, st.pos.y })
			.update()
			.getCurrent();

		player.r = cameraTransitionRotation
			->setTarget(st.rot.pitch + cameraVertical, st.rot.direction + cameraHorizontal, player.r.z)
			->update()
			->getCurrent();

		setViewDistance(cameraTransitionDistance.update().getCurrent());

		return;
	}

	
	player.p.x = st.pos.x;
	player.p.y = st.pos.z + 200;
	player.p.z = st.pos.y;
	player.r.x = st.rot.pitch + cameraVertical;
	player.r.y = st.rot.direction + cameraHorizontal;
	setViewDistance(CAMERA_DISTANCE);
}

SDWORD driveGetMoveSpeed(void)
{
	return (int)driveSpeed;
}

UWORD driveGetMoveDir(void)
{
	return (UWORD)(int)driveDir;
}

bool isDriving()
{
	if(!driving){
		return false;
	}

	if(isTransitioningOut){
		return true;
	}

    if (drivingTarget->died)
    {
		stopDriving();
    }

    return driving;
}

bool isDrivingDroid(DROID* droid)
{
	return driving && drivingTarget == droid;
}
