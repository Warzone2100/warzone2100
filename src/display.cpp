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
 * @file display.c
 *
 * Display routines.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/wzapp.h"
#include "profiling.h"

#include "action.h"
#include "display.h"
#include "droid.h"
#include "fpath.h"
#include "group.h"
#include "map.h"
#include "loop.h"
#include "component.h"
#include "display3d.h"
#include "lib/framework/cursors.h"
#include "hci.h"
#include "text.h"
#include "edit3d.h"
#include "geometry.h"
#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "radar.h"
#include "miscimd.h"
#include "lib/framework/math_ext.h"
#include "console.h"
#include "order.h"
#include "wrappers.h"
#include "power.h"
#include "map.h"
#include "intimage.h"
#include "mechanics.h"
#include "lighting.h"
#include "ingameop.h"
#include "oprint.h"
#include "warcam.h"
#include "keybind.h"
#include "projectile.h"
#include "message.h"
#include "effects.h"
#include "cmddroid.h"
#include "selection.h"
#include "transporter.h"
#include "intorder.h"
#include "multiplay.h"
#include "qtscript.h"
#include "warzoneconfig.h"
#include "lib/ivis_opengl/piematrix.h"
#include "animation.h"
#include "input/manager.h"
#include "input/keyconfig.h"
#include "mapgrid.h"
#include "main.h"

InputManager gInputManager;
KeyFunctionConfiguration gKeyFuncConfig;
DragBox3D dragBox3D;
WallDrag wallDrag;

#define POSSIBLE_SELECTIONS		14
#define POSSIBLE_TARGETS		23

extern char DROIDDOING[512];		// holds the string on what the droid is doing

// NOTE: the external file "cursorselection" is used, so you can import that into a spreadsheet, and edit it there, much easier.
static const CURSOR arnMPointers[POSSIBLE_TARGETS][POSSIBLE_SELECTIONS] =
{
#include "cursorselection"
};

int scrollDirLeftRight = 0;
int scrollDirUpDown = 0;

#if defined(__EMSCRIPTEN__)
// For Emscripten, default "Edge Scroll Outside Window" to false, to better handle various combined touch + mouse/trackpad devices
# define DEFAULT_EDGE_SCROLL_OUTSIDE_WINDOW false
#else
# define DEFAULT_EDGE_SCROLL_OUTSIDE_WINDOW true
#endif

static bool	buildingDamaged(STRUCTURE *psStructure);
static bool	repairDroidSelected(UDWORD player);
static bool vtolDroidSelected(UDWORD player);
static bool	anyDroidSelected(UDWORD player);
static bool cyborgDroidSelected(UDWORD player);
static bool bInvertMouse = true;
static bool bRightClickOrders = false;
optional<MOUSE_KEY_CODE> rotateMouseKey = MOUSE_RMB;
optional<MOUSE_KEY_CODE> panMouseKey = nullopt;
static bool bDrawShadows = true;
static bool bEdgeScrollOutsideWindowBounds = DEFAULT_EDGE_SCROLL_OUTSIDE_WINDOW;
static SELECTION_TYPE	establishSelection(UDWORD selectedPlayer);
static void	dealWithLMB();
static void	dealWithLMBDClick();
static void	dealWithRMB();
static void	handleDeselectionClick();
static bool	mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
static FLAG_POSITION *findMouseDeliveryPoint();

void finishDeliveryPosition();

static UDWORD	currentFrame;
static UDWORD StartOfLastFrame;
static SDWORD	rotX;
static SDWORD	rotY;
std::unique_ptr<ValueTracker> rotationHorizontalTracker = std::make_unique<ValueTracker>();
std::unique_ptr<ValueTracker> rotationVerticalTracker = std::make_unique<ValueTracker>();
static std::chrono::steady_clock::time_point scrollRefTime;
static double	scrollSpeedLeftRight; //use two directions and add them because its simple
static double	scrollStepLeftRight;
static double	scrollSpeedUpDown;
static double	scrollStepUpDown;
static bool	mouseOverConsole = false;
static bool	ignoreOrder = false;
static bool	ignoreRMBC	= true;
static DROID	*psSelectedVtol;
static DROID	*psDominantSelected;
static bool cameraAccel = true;

bool	rotActive = false;
bool	gameStats = false;
bool	lockCameraScrollWhileRotating = false;

/* Hackety hack hack hack */
static int screenShakeTable[100] =
{
	-2, -2, -3, -4, -3, -3, -5, -4, -4, -4,
	-4, -5, -5, -5, -5, -7, -5, -6, -8, -6,
	-7, -8, -6, -4, -8, -7, -7, -7, -6, -5,
	-6, -5, -2, -5, -6, -3, -5, -3, -2, -4,
	-5, -3, -2, -0, 1, 2, 2, 1, 0, 0,
	0, 1, 1, 3, 2, 1, 0, 2, 3, 4,
	4, 2, 6, 4, 5, 3, 7, 7, 3, 6,
	4, 7, 9, 10, 9, 8, 6, 4, 7, 5,
	5, 4, 6, 2, 4, 5, 3, 3, 2, 1,
	1, 0, -1, -1, -2, -1, 1, 0, 1, 0
};

static bool bScreenShakeActive = false;
static UDWORD screenShakeStarted;
static UDWORD screenShakeLength;

static const UDWORD FADE_START_OF_GAME_TIME = 1000;
static UDWORD fadeEndTime = 0;
static void fadeStartOfGame();
static void handleAreaDemolition();

//used to determine is a weapon droid is assigned to a sensor tower or sensor droid
static bool bSensorAssigned;
//used to determine if the player has selected a Las Sat structure
static bool bLasSatStruct;
// Local prototypes
static MOUSE_TARGET	itemUnderMouse(BASE_OBJECT **ppObjUnderCursor);
// If shaking is allowed
static bool bShakingPermitted = true;

static auto viewDistanceAnimation = Animation<float>(realTime);
static uint32_t viewDistanceIncrementCooldownTime = 0;

void animateToViewDistance(float target, float speed)
{
	viewDistanceAnimation
		.setInitialData(getViewDistance())
		.setFinalData(target)
		.setEasing(viewDistanceAnimation.isActive() ? EASE_OUT : EASE_IN_OUT)
		.setDuration(speed <= 0 ? 0 : static_cast<uint32_t>(glm::log(std::abs(target - getViewDistance())) * 100 * DEFAULT_VIEW_DISTANCE_ANIMATION_SPEED / speed))
		.start();
}

void incrementViewDistance(float amount)
{
	if (InGameOpUp || bDisplayMultiJoiningStatus || isInGamePopupUp || realTime < viewDistanceIncrementCooldownTime)
	{
		return;
	}

	viewDistanceIncrementCooldownTime = realTime + GAME_TICKS_PER_SEC / 50;
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	auto target = (viewDistanceAnimation.isActive() ? viewDistanceAnimation.getFinalData() : getViewDistance()) + amount;
	if (!dbgInputManager.debugMappingsAllowed())
	{
		CLIP(target, MINDISTANCE, (!NETisReplay()) ? MAXDISTANCE : MAXDISTANCE_REPLAY);
	}

	animateToViewDistance(target);
}

static void updateViewDistanceAnimation()
{
	if (viewDistanceAnimation.isActive())
	{
		viewDistanceAnimation.update();
		setViewDistance(viewDistanceAnimation.getCurrent());
	}
}

bool getShakeStatus()
{
	return bShakingPermitted;
}

void setShakeStatus(bool val)
{
	bShakingPermitted = val;
}

void shakeStart(unsigned int length)
{
	if (bShakingPermitted)
	{
		if (!bScreenShakeActive)
		{
			bScreenShakeActive = true;
			screenShakeStarted = gameTime;
			screenShakeLength = length;
		}
	}
}

void shakeStop()
{
	bScreenShakeActive = false;
	playerPos.r.z = 0;
}

static void shakeUpdate()
{
	UDWORD	screenShakePercentage;

	/* Check if we're shaking the screen or not */
	if (bScreenShakeActive)
	{
		screenShakePercentage = PERCENT(gameTime - screenShakeStarted, screenShakeLength);
		if (screenShakePercentage < 100)
		{
			playerPos.r.z = 0 + DEG(screenShakeTable[screenShakePercentage]);
		}
		if (gameTime > (screenShakeStarted + screenShakeLength))
		{
			bScreenShakeActive = false;
			playerPos.r.z = 0;
		}
	}
	else
	{
		if (!getWarCamStatus())
		{
			playerPos.r.z = 0;
		}
	}
}

bool	getCameraAccel()
{
	return cameraAccel;
}

void	setCameraAccel(bool val)
{
	cameraAccel = val;
}

bool	getInvertMouseStatus()
{
	return (bInvertMouse);
}

void	setInvertMouseStatus(bool val)
{
	bInvertMouse = val;
}

bool	getRightClickOrders()
{
	return bRightClickOrders;
}

void	setRightClickOrders(bool val)
{
	if (val == bRightClickOrders)
	{
		return;
	}

	bRightClickOrders = val;
	if (bRightClickOrders)
	{
		// Check for conflicting rotate / pan mouse button settings, and reset to a saner default
		if (rotateMouseKey.value_or(MOUSE_END) == MOUSE_RMB)
		{
			setRotateMouseKey(MOUSE_MMB);
		}
		if (panMouseKey.value_or(MOUSE_END) == MOUSE_RMB)
		{
			panMouseKey = nullopt;
		}
	}
}

optional<MOUSE_KEY_CODE> getRotateMouseKey()
{
	return rotateMouseKey;
}

bool setRotateMouseKey(optional<MOUSE_KEY_CODE> key)
{
	if (!key.has_value())
	{
		rotateMouseKey = key;
		return true;
	}

	// otherwise, check for conflicts
	if (key.value() == MOUSE_LMB) { return false; }
	if (bRightClickOrders && key.value() == MOUSE_RMB) { return false; }
	if (key == panMouseKey)
	{
		panMouseKey = nullopt;
	}

	rotateMouseKey = key;
	return true;
}

optional<MOUSE_KEY_CODE> getPanMouseKey()
{
	return panMouseKey;
}

bool setPanMouseKey(optional<MOUSE_KEY_CODE> key)
{
	if (!key.has_value())
	{
		panMouseKey = key;
		return true;
	}

	// otherwise, check for conflicts
	if (key.value() == MOUSE_LMB) { return false; }
	if (bRightClickOrders && key.value() == MOUSE_RMB) { return false; }
	if (key == rotateMouseKey)
	{
		rotateMouseKey = nullopt;
	}

	panMouseKey = key;
	return true;
}

bool	getDrawShadows()
{
	return (bDrawShadows);
}

void	setDrawShadows(bool val)
{
	bDrawShadows = val;
	pie_setShadows(val);
}

void setEdgeScrollOutsideWindowBounds(bool val)
{
	bEdgeScrollOutsideWindowBounds = val;
}

bool getEdgeScrollOutsideWindowBounds()
{
	return bEdgeScrollOutsideWindowBounds;
}

// reset the input state
void resetInput()
{
	rotActive = false;
	dragBox3D.status = DRAG_INACTIVE;
	wallDrag.status = DRAG_INACTIVE;
	gInputManager.contexts().resetStates();
}

static bool localPlayerHasSelection()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}

	for (const DROID* psDroid : apsDroidLists[selectedPlayer])
	{
		if (psDroid->selected)
		{
			return true;
		}
	}

	for (const STRUCTURE* psStruct : apsStructLists[selectedPlayer])
	{
		if (psStruct->selected)
		{
			return true;
		}
	}

	return false;
}

/* Process the user input. This just processes the key input and jumping around the radar*/
void processInput()
{
	if (InGameOpUp || isInGamePopupUp)
	{
		dragBox3D.status = DRAG_INACTIVE;	// disengage the dragging since it stops menu input
	}


	StartOfLastFrame = currentFrame;
	currentFrame = frameGetFrameNumber();

	ignoreRMBC = false;

	const bool mOverConstruction = CoordInBuild(mouseX(), mouseY());
	const bool mouseIsOverScreenOverlayChild = isMouseOverScreenOverlayChild(mouseX(), mouseY());

	if (!mouseIsOverScreenOverlayChild)
	{
		mouseOverConsole = mouseOverHistoryConsoleBox();

		/* Process all of our key mappings */
		if (mOverConstruction)
		{
			if (mousePressed(MOUSE_WUP))
			{
				kf_BuildPrevPage();
			}

			if (mousePressed(MOUSE_WDN))
			{
				kf_BuildNextPage();
			}
		}
	}

	gInputManager.contexts().set(
		InputContext::DEBUG_HAS_SELECTION,
		localPlayerHasSelection() ? InputContext::State::ACTIVE : InputContext::State::INACTIVE
	);
	gInputManager.contexts().updatePriorityStatus();

	if (!isInTextInputMode())
	{
		const bool allowMouseWheelEvents = !mouseIsOverScreenOverlayChild && !mouseOverConsole && !mOverConstruction;
		gInputManager.processMappings(allowMouseWheelEvents);
	}
	/* Allow the user to clear the (Active) console if need be */
	if (!mouseIsOverScreenOverlayChild && mouseOverConsoleBox() && mousePressed(MOUSE_LMB))
	{
		clearActiveConsole();
	}
}

static bool OverRadarAndNotDragging()
{
	return isMouseOverRadar() && dragBox3D.status != DRAG_DRAGGING && wallDrag.status != DRAG_DRAGGING;
}

static void CheckFinishedDrag(SELECTION_TYPE selection)
{
	if (mouseReleased(MOUSE_LMB) || mouseDown(MOUSE_RMB))
	{
		selectAttempt = false;
		if (dragBox3D.status == DRAG_DRAGGING)
		{
			if (wallDrag.status == DRAG_DRAGGING)
			{
				//if invalid location keep looking for a valid one
				if ((buildState == BUILD3D_VALID || buildState == BUILD3D_FINISHED)
				    && sBuildDetails.psStats->hasType(STAT_STRUCTURE))
				{
					if (canLineBuild())
					{
						wallDrag.pos2 = mousePos;
						wallDrag.status = DRAG_RELEASED;
					}
				}
			}

			/* Only clear if shift isn't down - this is for the drag selection box for units*/
			if (!ctrlShiftDown() && wallDrag.status == DRAG_INACTIVE)
			{
				clearSelection();
			}
			dragBox3D.status = DRAG_RELEASED;
			dragBox3D.x2 = mouseX();
			dragBox3D.y2 = mouseY();
			if (selection == SC_DROID_DEMOLISH && ctrlShiftDown())
			{
				handleAreaDemolition();
			}
		}
		else
		{
			dragBox3D.status = DRAG_INACTIVE;
			wallDrag.status = DRAG_INACTIVE;
		}
	}
}
/**
 * Demolish all structures in given area
 * Note: Does not attempt to optimize movement paths,
 * so demolishing can be a little out of order
*/
static void handleAreaDemolition()
{
	const Vector2i pt1(dragBox3D.x1, dragBox3D.y1);
	const Vector2i pt2(dragBox3D.x2, dragBox3D.y2);
	Vector2i worldCoord1(0, 0), worldCoord2(0, 0), tmp;
	SDWORD notused1 = 0, notused2 = 0;
	screenCoordToWorld(pt1, worldCoord1, notused1, notused2);
	screenCoordToWorld(pt2, worldCoord2, notused1, notused2);
	// swap coordinates to be in increasing order.. otherwise gridIterate doesn't work
	tmp = worldCoord1;
	worldCoord1.x = worldCoord1.x < worldCoord2.x ? worldCoord1.x : worldCoord2.x;
	worldCoord1.y = worldCoord1.y < worldCoord2.y ? worldCoord1.y : worldCoord2.y;
	worldCoord2.x = worldCoord2.x > tmp.x ? worldCoord2.x : tmp.x;
	worldCoord2.y = worldCoord2.y > tmp.y ? worldCoord2.y : tmp.y;

	debug(LOG_INFO, "demolish everything in the area (%i %i) -> (%i %i)", worldCoord1.x, worldCoord1.y, worldCoord2.x, worldCoord2.y);
	std::vector<BASE_OBJECT *> gridList = gridStartIterateArea(worldCoord1.x, worldCoord1.y, worldCoord2.x, worldCoord2.y);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *psObj = *gi;
		if (psObj->type == OBJ_STRUCTURE && psObj->player == selectedPlayer)
		{
			// add demolish order to queue for every selected unit
			orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT *)psObj, true);
		}
	}
}

static void CheckStartWallDrag()
{
	if (mousePressed(MOUSE_LMB))
	{
		/* Store away the details if we're building */
		// You can start dragging walls from invalid locations so check for
		// BUILD3D_POS or BUILD3D_VALID, used tojust check for BUILD3D_VALID.
		if ((buildState == BUILD3D_POS || buildState == BUILD3D_VALID)
		    && sBuildDetails.psStats->hasType(STAT_STRUCTURE))
		{
			if (canLineBuild())
			{
				wallDrag.pos = wallDrag.pos2 = mousePos;
				wallDrag.status = DRAG_PLACING;
				debug(LOG_NEVER, "Start Wall Drag\n");
			}
		}
		else if (intBuildSelectMode())//if we were in build select mode
		{
			//uhoh no place to build here
			audio_PlayBuildFailedOnce();
		}
	}
}

//this function is called when a location has been chosen to place a structure or a DP
static bool CheckFinishedFindPosition()
{
	bool OverRadar = OverRadarAndNotDragging();

	/* Do not let the player position buildings 'under' the radar */
	if (mouseReleased(MOUSE_LMB) && !OverRadar)
	{

		if (deliveryReposValid())
		{
			finishDeliveryPosition();
			return true;
		}
		else if (buildState == BUILD3D_VALID)
		{
			if (sBuildDetails.psStats->hasType(STAT_STRUCTURE) && canLineBuild())
			{
				wallDrag.pos2 = mousePos;
				wallDrag.status = DRAG_RELEASED;
			}
			debug(LOG_NEVER, "BUILD3D_FINISHED\n");
			buildState = BUILD3D_FINISHED;
			return true;
		}
	}

	return false;
}

static void HandleDrag()
{
	UDWORD dragX = 0, dragY = 0;

	if (mouseDrag(MOUSE_LMB, &dragX, &dragY) && !isMouseOverRadar() && !mouseDown(MOUSE_RMB))
	{
		dragBox3D.x1 = dragX;
		dragBox3D.x2 = mouseX();
		dragBox3D.y1 = dragY;
		dragBox3D.y2 = mouseY();
		dragBox3D.status = DRAG_DRAGGING;

		if (buildState == BUILD3D_VALID && canLineBuild())
		{
			wallDrag.pos2 = mousePos;
			wallDrag.status = DRAG_DRAGGING;
		}
	}
}

// Mouse X coordinate at start of panning.
UDWORD panMouseX;
// Mouse Y coordinate at start of panning.
UDWORD panMouseY;
std::unique_ptr<ValueTracker> panXTracker = std::make_unique<ValueTracker>();
std::unique_ptr<ValueTracker> panZTracker = std::make_unique<ValueTracker>();
bool panActive;

//don't want to do any of these whilst in the Intelligence Screen
void processMouseClickInput()
{
	UDWORD	i;
	SELECTION_TYPE	selection;
	MOUSE_TARGET	item = MT_NOTARGET;
	bool OverRadar = OverRadarAndNotDragging();
	selection = establishSelection(selectedPlayer);
	ASSERT(selection <= POSSIBLE_SELECTIONS, "Weirdy selection!");

	ignoreOrder = CheckFinishedFindPosition();

	CheckStartWallDrag();

	HandleDrag();

	CheckFinishedDrag(selection);

	if (isMouseOverScreenOverlayChild(mouseX(), mouseY()))
	{
		// ignore clicks
		return;
	}

	if (mouseReleased(MOUSE_LMB) && !OverRadar && dragBox3D.status != DRAG_RELEASED && !ignoreOrder && !mouseOverConsole && !bDisplayMultiJoiningStatus)
	{
		if (bRightClickOrders)
		{
			dealWithRMB();
		}
		else
		{
			if (!bMultiPlayer && establishSelection(selectedPlayer) == SC_DROID_SUPERTRANSPORTER)
			{
				// Never, *ever* let user control the transport in SP games--it breaks the scripts!
				ASSERT(game.type == LEVEL_TYPE::CAMPAIGN, "Game type was set incorrectly!");
			}
			else
			{
				dealWithLMB();
			}
		}
	}

	if (mouseDClicked(MOUSE_LMB))
	{
		dealWithLMBDClick();
	}

	if (mouseReleased(MOUSE_RMB) && !rotActive && !panActive && !ignoreRMBC)
	{
		dragBox3D.status = DRAG_INACTIVE;
		// Pretty sure we wan't set walldrag status here aswell.
		wallDrag.status = DRAG_INACTIVE;
		if (bRightClickOrders)
		{
			dealWithLMB();
		}
		else
		{
			dealWithRMB();
		}
		// Why?
		if (getWarCamStatus())
		{
			camToggleStatus();
		}
	}

	/* Right mouse click kills a building placement */
	if (!rotActive && mouseReleased(MOUSE_RMB) &&
	    (buildState == BUILD3D_POS || buildState == BUILD3D_VALID))
	{
		/* Stop the placement */
		kill3DBuilding();
	}
	if (mouseReleased(MOUSE_RMB))
	{
		cancelDeliveryRepos();
	}
	if (rotateMouseKey.has_value() && mouseDrag(rotateMouseKey.value(), (UDWORD *)&rotX, (UDWORD *)&rotY) && !rotActive && !isRadarDragging() && !getRadarTrackingStatus())
	{
		rotationVerticalTracker->startTracking((UWORD)playerPos.r.x);
		rotationHorizontalTracker->startTracking((UWORD)playerPos.r.y); // negative values caused problems with float conversion
		rotActive = true;
	}
	if (panMouseKey.has_value() && mouseDrag(panMouseKey.value(), (UDWORD *)&panMouseX, (UDWORD *)&panMouseY) && !rotActive && !panActive && !isRadarDragging() && !getRadarTrackingStatus())
	{
		panXTracker->startTracking(playerPos.p.x);
		panZTracker->startTracking(playerPos.p.z);
		panActive = true;
	}


	if (gamePaused())
	{
		wzSetCursor(CURSOR_DEFAULT);
	}
	if (buildState == BUILD3D_VALID)
	{
		// special casing for building
		wzSetCursor(CURSOR_BUILD);
	}
	else if (buildState == BUILD3D_POS)
	{
		// special casing for building - can't build here
		wzSetCursor(CURSOR_NOTPOSSIBLE);
	}
	else if (selection != SC_INVALID)
	{
		BASE_OBJECT *ObjUnderMouse;
		bool ObjAllied;

		item = itemUnderMouse(&ObjUnderMouse);
		ASSERT(item < POSSIBLE_TARGETS, "Weirdy target!");

		ASSERT(selectedPlayer < MAX_PLAYERS, "selectedPlayer is too high: %" PRIu32 "", selectedPlayer);
		ObjAllied = (ObjUnderMouse && selectedPlayer != ObjUnderMouse->player && aiCheckAlliances(selectedPlayer, ObjUnderMouse->player));
		bool ObjDepartedAlly = (ObjUnderMouse && bMultiPlayer && NetPlay.players[ObjUnderMouse->player].difficulty == AIDifficulty::HUMAN && !NetPlay.players[ObjUnderMouse->player].allocated);

		if (item != MT_NOTARGET)
		{
			// exceptions to the lookup table.
			if (ctrlShiftDown() &&
			    (ObjUnderMouse != nullptr) &&
			    (ObjUnderMouse->player == selectedPlayer) &&
			    (ObjUnderMouse->type == OBJ_DROID))
			{
				item = MT_OWNDROID;
			}
			else if (specialOrderKeyDown() &&
			         (ObjUnderMouse != nullptr) &&
			         ObjUnderMouse->player == selectedPlayer)
			{
				if (selection == SC_DROID_REPAIR)
				{
					item = MT_OWNDROIDDAM;
				}
				else
				{
					// attacking own unit
					item = MT_ENEMYDROID;
				}
			}
			else if (selection == SC_DROID_REPAIR)
			{
				// We can't repair ourselves, so change it to a blocking cursor
				for (const DROID *psCurr : apsDroidLists[selectedPlayer])
				{
					if (psCurr->selected)
					{
						if ((ObjUnderMouse != nullptr) && ObjUnderMouse->player == selectedPlayer && psCurr->id == ObjUnderMouse->id)
						{
							item = MT_BLOCKING;
						}
						break;
					}
				}
			}
			else if (selection == SC_DROID_DEMOLISH)
			{
				// Can't demolish allied objects, or something that isn't built yet
				if ((ObjAllied && !ObjDepartedAlly) || (ObjUnderMouse && (ObjUnderMouse->type != OBJ_STRUCTURE || (((STRUCTURE *)ObjUnderMouse)->status == SS_BLUEPRINT_PLANNED))))
				{
					item = MT_BLOCKING;
				}
			}
			// in multiPlayer check for what kind of unit can use it (TODO)
			else if (bMultiPlayer && item == MT_TRANDROID)
			{
				if (ObjUnderMouse->player != selectedPlayer)
				{
					item = MT_OWNDROID;
				}
			}
			else if (selection == SC_DROID_CONSTRUCT)
			{
				// We don't allow the build cursor under certain circumstances ....
				// can't build if res extractors arent available.
				if (item == MT_RESOURCE)
				{
					for (i = 0; i < numStructureStats && asStructureStats[i].type != REF_RESOURCE_EXTRACTOR; i++) {} // find resource stat
					if (i < numStructureStats && apStructTypeLists[selectedPlayer][i] != AVAILABLE)		// check if you can build it!
					{
						item = MT_BLOCKING;				// don't allow build pointer.
					}
				}

				// repair instead of sensor/guard with cons. droids.
				else if (item == MT_SENSOR)
				{
					if (ObjUnderMouse							// something valid
					    && (ObjUnderMouse->type == OBJ_STRUCTURE))// check if struct
					{
						if (buildingDamaged((STRUCTURE *)ObjUnderMouse))
						{
							item = MT_OWNSTRDAM;				// replace guard/sense with usual icons.
						}
						else
						{
							item = MT_OWNSTROK;
						}
					}
				}
			}
			else if (item == MT_SENSOR
			         && selection == SC_DROID_INDIRECT
			         && (keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)))
			{
				selection = SC_DROID_SENSOR;
			}

			// check the type of sensor for indirect weapons
			else if ((item == MT_SENSOR || item == MT_SENSORSTRUCT || item == MT_SENSORSTRUCTDAM)
			         && selection == SC_DROID_INDIRECT)
			{
				if (ObjUnderMouse && !droidSensorDroidWeapon(ObjUnderMouse, psDominantSelected))
				{
					item = MT_BLOCKING;
				}
			}

			//check for VTOL droids being assigned to a sensor droid/structure
			else if ((item == MT_SENSOR || item == MT_SENSORSTRUCT || item == MT_SENSORSTRUCTDAM)
			         && selection == SC_DROID_DIRECT
			         && vtolDroidSelected((UBYTE)selectedPlayer))
			{
				// NB. psSelectedVtol was set by vtolDroidSelected - yes I know its horrible, but it
				// only smells as much as the rest of display.c so I don't feel so bad
				if (ObjUnderMouse && droidSensorDroidWeapon(ObjUnderMouse, psSelectedVtol))
				{
					selection = SC_DROID_INDIRECT;
				}
				else
				{
					item = MT_BLOCKING;
				}
			}

			//vtols cannot pick up artifacts
			else if (item == MT_ARTIFACT
			         && selection == SC_DROID_DIRECT
			         && vtolDroidSelected((UBYTE)selectedPlayer))
			{
				item = MT_BLOCKING;
			}

			if (item == MT_TERRAIN
			    && terrainType(mapTile(mouseTileX, mouseTileY)) == TER_CLIFFFACE)
			{
				item = MT_BLOCKING;
			}
			// special droid at full health
			if (arnMPointers[item][selection] == CURSOR_FIX && ObjUnderMouse && ObjUnderMouse->type == OBJ_DROID &&
			    !((DROID *)ObjUnderMouse)->isDamaged())
			{
				item = MT_OWNDROID;
			}
			if ((arnMPointers[item][selection] == CURSOR_SELECT ||
			     arnMPointers[item][selection] == CURSOR_EMBARK ||
			     arnMPointers[item][selection] == CURSOR_ATTACH ||
			     arnMPointers[item][selection] == CURSOR_LOCKON ||
			     arnMPointers[item][selection] == CURSOR_DEST) && ObjAllied
				&& (selection != SC_DROID_DEMOLISH || !ObjDepartedAlly))
			{
				// If you want to do these things, just gift your unit to your ally.
				item = MT_BLOCKING;
			}

			if (specialOrderKeyDown() && (selection == SC_DROID_TRANSPORTER || selection == SC_DROID_SUPERTRANSPORTER) &&
			    arnMPointers[item][selection] == CURSOR_MOVE && bMultiPlayer)
			{
				// Alt+move = disembark transporter
				wzSetCursor(CURSOR_DISEMBARK);
			}
			else if (specialOrderKeyDown() && selection == SC_DROID_DIRECT &&
			         arnMPointers[item][selection] == CURSOR_MOVE)
			{
				// Alt+move = scout
				wzSetCursor(CURSOR_SCOUT);
			}
			else if (arnMPointers[item][selection] == CURSOR_NOTPOSSIBLE &&
			         ObjUnderMouse && (ObjUnderMouse->player == selectedPlayer) &&
			         ObjUnderMouse->type == OBJ_STRUCTURE && ((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat &&
			         (((STRUCTURE*)ObjUnderMouse)->getWeaponStats(0)->weaponSubClass == WSC_LAS_SAT))
			{
				wzSetCursor(CURSOR_SELECT); // Special casing for LasSat
			}
			else
			{
				wzSetCursor(arnMPointers[item][selection]);
			}
		}
		else
		{
			wzSetCursor(CURSOR_DEFAULT);
		}
	}
	else
	{
		BASE_OBJECT *ObjUnderMouse;
		item = itemUnderMouse(&ObjUnderMouse);

		//exceptions, exceptions...AB 10/06/99
		if (bMultiPlayer && bLasSatStruct)
		{
			ASSERT(item < POSSIBLE_TARGETS, "Weirdy target!");
			if (item == MT_ENEMYDROID || item == MT_ENEMYSTR || item == MT_DAMFEATURE)
			{
				//display attack cursor
				wzSetCursor(CURSOR_ATTACK);
			}
			else if (ObjUnderMouse && ObjUnderMouse->player == selectedPlayer && (ObjUnderMouse->type == OBJ_DROID ||
			         (ObjUnderMouse->type == OBJ_STRUCTURE && lasSatStructSelected((STRUCTURE *)ObjUnderMouse))))
			{
				// Special casing for selectables
				wzSetCursor(CURSOR_SELECT);
			}
			else if (ObjUnderMouse && ObjUnderMouse->player == selectedPlayer && ObjUnderMouse->type == OBJ_STRUCTURE)
			{
				wzSetCursor(CURSOR_DEFAULT);
			}
			else
			{
				//display block cursor
				wzSetCursor(CURSOR_NOTPOSSIBLE);
			}
		}
		else if (ObjUnderMouse && (ObjUnderMouse->player == selectedPlayer) &&
		         ((ObjUnderMouse->type == OBJ_STRUCTURE && ((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat
		           && (((STRUCTURE*)ObjUnderMouse)->getWeaponStats(0)->weaponSubClass == WSC_LAS_SAT))
		          || ObjUnderMouse->type == OBJ_DROID))
		{
			wzSetCursor(CURSOR_SELECT); // Special casing for LasSat or own unit
		}
	}
}

static void calcScroll(double *y, double *dydt, double accel, double decel, double targetVelocity, double dt)
{
	double tMid;

	// Stop instantly, if trying to change direction.
	if (targetVelocity * *dydt < -1e-8f)
	{
		*dydt = 0.0;
	}

	if (targetVelocity < *dydt)
	{
		accel = -accel;
		decel = -decel;
	}

	// Decelerate if needed.
	tMid = (0.0 - *dydt) / decel;
	CLIP(tMid, 0.0, dt);
	*y += (*dydt * tMid + decel / 2.0 * tMid * tMid);
	if (cameraAccel)
	{
		*dydt += (decel * tMid);
	}
	dt -= (tMid);

	// Accelerate if needed.
	tMid = (targetVelocity - *dydt) / accel;
	CLIP(tMid, 0.0, dt);
	*y += (*dydt * tMid + accel / 2.0 * tMid * tMid);
	if (cameraAccel)
	{
		*dydt += accel * tMid;
	}
	else
	{
		*dydt = targetVelocity;
	}
	dt -= tMid;

	// Continue at target velocity.
	*y += *dydt * dt;
}

static inline bool shouldProcessEdgeScroll()
{
	return wzMouseInWindow() || (bEdgeScrollOutsideWindowBounds && wzWindowHasFocus());
}

static void handleCameraScrolling()
{
	int	xDif, yDif;
	double scroll_zoom_factor = 1.0 + 2.0 * ((getViewDistance() - MINDISTANCE) / ((float)(MAXDISTANCE - MINDISTANCE)));

	double scaled_max_scroll_speed = scroll_zoom_factor * (cameraAccel ? war_GetCameraSpeed() : war_GetCameraSpeed() / 2.0);
	double scaled_accel = scaled_max_scroll_speed / 2.0;

	if (InGameOpUp || bDisplayMultiJoiningStatus || isInGamePopupUp)		// cant scroll when menu up. or when over radar
	{
		return;
	}

	if (lockCameraScrollWhileRotating && rotActive && (scrollDirUpDown == 0 && scrollDirLeftRight == 0))
	{
		resetScroll();
		return;
	}

	if (shouldProcessEdgeScroll())
	{
		if (mouseY() < BOUNDARY_Y)
		{
			scrollDirUpDown++;
			wzSetCursor(CURSOR_UARROW);
		}
		if (mouseY() >= (pie_GetVideoBufferHeight() - BOUNDARY_Y))
		{
			scrollDirUpDown--;
			wzSetCursor(CURSOR_DARROW);
		}
		if (mouseX() < BOUNDARY_X)
		{
			wzSetCursor(CURSOR_LARROW);
			scrollDirLeftRight--;
		}
		if (mouseX() >= (pie_GetVideoBufferWidth() - BOUNDARY_X))
		{
			wzSetCursor(CURSOR_RARROW);
			scrollDirLeftRight++;
		}
	}
	CLIP(scrollDirLeftRight, -1, 1);
	CLIP(scrollDirUpDown,    -1, 1);

	if (scrollDirLeftRight != 0 || scrollDirUpDown != 0)
	{
		setWarCamActive(false);  // Don't let this thing override the user trying to scroll.
	}

	// Apparently there's stutter if using deltaRealTime, so we have our very own delta time here, just for us.
	auto now = std::chrono::steady_clock::now();
	auto timeDiff = now - scrollRefTime;
	if (std::chrono::duration_cast<std::chrono::nanoseconds>(timeDiff) >= std::chrono::nanoseconds(4000000)) // throttle max update rate to avoid really jerky scrolling
	{
		scrollRefTime = now;

		std::chrono::duration<double> timeDiff_as_seconds_double = std::chrono::duration_cast<std::chrono::duration<double>>(timeDiff);
		auto timeDiff_double = timeDiff_as_seconds_double.count();
		timeDiff_double = std::min(timeDiff_double, 0.5);  // Since we're using our own time variable, which isn't updated when dragging a box, clamp the time here so releasing the box doesn't scroll to the edge of the map suddenly.

		scrollStepLeftRight = 0.0;
		scrollStepUpDown = 0.0;
		calcScroll(&scrollStepLeftRight, &scrollSpeedLeftRight, scaled_accel, 2.0 * scaled_accel, scrollDirLeftRight * scaled_max_scroll_speed, timeDiff_double);
		calcScroll(&scrollStepUpDown,    &scrollSpeedUpDown,    scaled_accel, 2.0 * scaled_accel, scrollDirUpDown    * scaled_max_scroll_speed, timeDiff_double);

		/* Get x component of movement */
		xDif = (int) (cos(static_cast<double>(-playerPos.r.y) * (M_PI / 32768.0)) * scrollStepLeftRight + sin(static_cast<double>(-playerPos.r.y) * (M_PI / 32768.0)) * scrollStepUpDown);
		/* Get y component of movement */
		yDif = (int) (sin(static_cast<double>(-playerPos.r.y) * (M_PI / 32768.0)) * scrollStepLeftRight - cos(static_cast<double>(-playerPos.r.y) * (M_PI / 32768.0)) * scrollStepUpDown);

		/* Adjust player's position by these components */
		playerPos.p.x += xDif;
		playerPos.p.z += yDif;

		CheckScrollLimits();
	}

	// Reset scroll directions
	scrollDirLeftRight = 0;
	scrollDirUpDown = 0;
}

void displayRenderLoop()
{
	handleCameraScrolling();
	updateViewDistanceAnimation();
}

/*
 * Reset scrolling, so we don't jump around after unpausing.
 */
void resetScroll()
{
	scrollRefTime = std::chrono::steady_clock::now();
	scrollSpeedUpDown = 0.0;
	scrollSpeedLeftRight = 0.0;
	scrollDirLeftRight = 0;
	scrollDirUpDown = 0;
}

// Checks if coordinate is inside scroll limits, returns false if not.
bool CheckInScrollLimits(const int &xPos, const int &yPos)
{
	int minX = world_coord(scrollMinX);
	int maxX = world_coord(scrollMaxX - 1);
	int minY = world_coord(scrollMinY);
	int maxY = world_coord(scrollMaxY - 1);

	if ((xPos < minX) || (xPos >= maxX) || (yPos < minY) || (yPos >= maxY))
	{
		return false;
	}

	return true;
}

// Check a coordinate is within the scroll limits, SDWORD version.
// Returns true if edge hit.
//
bool CheckInScrollLimitsCamera(SDWORD *xPos, SDWORD *zPos)
{
	bool EdgeHit = false;
	SDWORD	minX, minY, maxX, maxY;

	minX = world_coord(scrollMinX);
	maxX = world_coord(scrollMaxX - 1);
	minY = world_coord(scrollMinY);
	maxY = world_coord(scrollMaxY - 1);

	//scroll is limited to what can be seen for current campaign
	if (*xPos < minX)
	{
		*xPos = minX;
		EdgeHit = true;
	}
	else if (*xPos >= maxX)
	{
		*xPos = maxX;
		EdgeHit = true;
	}

	if (*zPos < minY)
	{
		*zPos = minY;
		EdgeHit = true;
	}
	else if (*zPos >= maxY)
	{
		*zPos = maxY;
		EdgeHit = true;
	}

	return EdgeHit;
}

// Check the view is within the scroll limits,
// Returns true if edge hit.
//
bool CheckScrollLimits()
{
	SDWORD xp = playerPos.p.x;
	SDWORD zp = playerPos.p.z;
	bool ret = CheckInScrollLimitsCamera(&xp, &zp);

	playerPos.p.x = xp;
	playerPos.p.z = zp;

	return ret;
}

/* Do the 3D display */
void displayWorld()
{
	if (headlessGameMode())
	{
		return;
	}

	Vector3i pos;

	shakeUpdate();

	if (panMouseKey.has_value() && panActive)
	{
		if(!mouseDown(panMouseKey.value())){
			panActive = false;
		} else {
			int mouseDeltaX = mouseX() - panMouseX;
			int mouseDeltaY = mouseY() - panMouseY;

			int panningSpeed = std::max(1, war_GetCameraSpeed() / 200);

			float horizontalMovement = panXTracker->setTargetDelta(mouseDeltaX * panningSpeed)->update()->getCurrentDelta();
			float verticalMovement = -1 * panZTracker->setTargetDelta(mouseDeltaY * panningSpeed)->update()->getCurrentDelta();

			playerPos.p.x = static_cast<int>(panXTracker->getInitial()
				+ cos(-playerPos.r.y * (M_PI / 32768)) * horizontalMovement
				+ sin(-playerPos.r.y * (M_PI / 32768)) * verticalMovement);
			playerPos.p.z = static_cast<int>(panZTracker->getInitial()
				+ sin(-playerPos.r.y * (M_PI / 32768)) * horizontalMovement
				- cos(-playerPos.r.y * (M_PI / 32768)) * verticalMovement);
			CheckScrollLimits();
		}
	}

	if (rotateMouseKey.has_value() && rotActive)
	{
		if (mouseDown(rotateMouseKey.value()))
		{
			float mouseDeltaX = mouseX() - rotX;
			float mouseDeltaY = mouseY() - rotY;

			playerPos.r.y = rotationHorizontalTracker->setTargetDelta(static_cast<int>(DEG(-mouseDeltaX) / 4))->update()->getCurrent();

			if(bInvertMouse)
			{
				mouseDeltaY *= -1;
			}

			playerPos.r.x = rotationVerticalTracker->setTargetDelta(static_cast<int>(DEG(mouseDeltaY) / 4))->update()->getCurrent();
			playerPos.r.x = glm::clamp(playerPos.r.x, DEG(360 + MIN_PLAYER_X_ANGLE), DEG(360 + MAX_PLAYER_X_ANGLE));
		}
		else
		{
			rotActive = false;
			ignoreRMBC = true;
			pos.x = playerPos.r.x;
			pos.y = playerPos.r.y;
			pos.z = playerPos.r.z;
			camInformOfRotation(&pos);
		}
	}

	draw3DScene();

	if (fadeEndTime)
	{
		if (graphicsTime < fadeEndTime)
		{
			fadeStartOfGame();
		}
		else
		{
			// ensure the fade only happens once (per call to transitionInit() & graphicsTime init) - i.e. at game start - regardless of graphicsTime wrap-around
			fadeEndTime = 0;
		}
	}
}

bool transitionInit()
{
	fadeEndTime = FADE_START_OF_GAME_TIME;
	return true;
}

static void fadeStartOfGame()
{
	PIELIGHT color = WZCOL_BLACK;
	float delta = (static_cast<float>(graphicsTime) / static_cast<float>(fadeEndTime) - 1.f);
	color.byte.a = static_cast<uint8_t>(std::min<uint32_t>(255, static_cast<uint32_t>(std::ceil(255.f * (1.f - (delta * delta * delta + 1.f)))))); // cubic easing
	pie_UniTransBoxFill(0, 0, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight(), color);
}

static bool mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	return mouseX() > x0 && mouseX() < x1 && mouseY() > y0 && mouseY() < y1;
}

bool DrawnInLastFrame(int32_t frame)
{
	return frame >= (int32_t)StartOfLastFrame;
}


/*
	Returns what the mouse was clicked on. Only called if there was a mouse pressed message
	on MOUSE_LMB. We aren't concerned here with setting selection flags - just what it
	actually was
*/
BASE_OBJECT *mouseTarget()
{
	BASE_OBJECT *psReturn = nullptr;
	int dispX, dispY, dispR;

	if (mouseTileX < 0 || mouseTileY < 0 || mouseTileX > mapWidth - 1 || mouseTileY > mapHeight - 1)
	{
		return (nullptr);
	}

	/* First have a look through the droid lists */
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		for (DROID *psDroid : apsDroidLists[i])
		{
			dispX = psDroid->sDisplay.screenX;
			dispY = psDroid->sDisplay.screenY;
			dispR = psDroid->sDisplay.screenR;

			// Has the droid been drawn since the start of the last frame
			if (psDroid->visibleForLocalDisplay() && DrawnInLastFrame(psDroid->sDisplay.frameNumber))
			{
				if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
				{
					/* We HAVE clicked on droid! */
					psReturn = (BASE_OBJECT *) psDroid;
					/* There's no point in checking other object types */
					return psReturn;
				}
			}
		}
	} // end of checking for droids

	/*	Not a droid, so maybe a structure or feature?
		If still NULL after this then nothing */
	psReturn = getTileOccupier(mouseTileX, mouseTileY);

	if (psReturn == nullptr)
	{
		psReturn = getTileBlueprintStructure(mouseTileX, mouseTileY);
	}

	/* Send the result back - if it's null then we clicked on an area of terrain */
	return psReturn;
}

// Start repositioning a delivery point.
//
static FLAG_POSITION flagPos;
static int flagStructId;
static bool flagReposVarsValid;
static bool flagReposFinished;

void startDeliveryPosition(FLAG_POSITION *psFlag)
{
	if (tryingToGetLocation()) // if we're placing a building don't place
	{
		return;
	}

	ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "Invalid player (selectedPlayer: %" PRIu32 ")", selectedPlayer);

	//clear the selected delivery point
	for (auto& psFlagPos : apsFlagPosLists[selectedPlayer])
	{
		psFlagPos->selected = false;
	}

	//set this object position to be highlighted
	psFlag->selected = true;
	flagPos = *psFlag;

	STRUCTURE *psStruct = findDeliveryFactory(psFlag);
	if (!psStruct)
	{
		flagStructId = 0; // not a struct, just a flag.
	}
	else
	{
		flagStructId = psStruct->id;
	}
	flagReposVarsValid = true;
	flagReposFinished = false;

	triggerEvent(TRIGGER_DELIVERY_POINT_MOVING, psStruct);
}

// Finished repositioning a delivery point.
//
void finishDeliveryPosition()
{
	STRUCTURE *psStruct = nullptr;

	ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "Invalid player (selectedPlayer: %" PRIu32 ")", selectedPlayer);

	if (flagStructId)
	{
		flagReposVarsValid = false;
		psStruct = IdToStruct(flagStructId, selectedPlayer);
		if (psStruct)
		{
			if (psStruct->isFactory() && psStruct->pFunctionality
				&& psStruct->pFunctionality->factory.psAssemblyPoint)
			{
				setAssemblyPoint(psStruct->pFunctionality->factory.psAssemblyPoint,
								 flagPos.coords.x, flagPos.coords.y, selectedPlayer, true);
			}
			else if (psStruct->pStructureType && psStruct->pStructureType->type == REF_REPAIR_FACILITY && psStruct->pFunctionality != nullptr)
			{
				setAssemblyPoint(psStruct->pFunctionality->repairFacility.psDeliveryPoint,
								 flagPos.coords.x, flagPos.coords.y, selectedPlayer, true);
			}
		}
		//deselect once moved
		for (auto& psFlag : apsFlagPosLists[selectedPlayer])
		{
			psFlag->selected = false;
		}
	}
	triggerEvent(TRIGGER_DELIVERY_POINT_MOVED, psStruct);
	flagReposFinished = true;
}

// Is there a valid delivery point repositioning going on.
bool deliveryReposValid()
{
	if (!flagReposVarsValid)
	{
		return false;
	}

	ASSERT_OR_RETURN(false, selectedPlayer < MAX_PLAYERS, "Invalid player (selectedPlayer: %" PRIu32 ")", selectedPlayer);

	Vector2i map = map_coord(flagPos.coords.xy());

	//make sure we are not too near map edge
	if (map.x < scrollMinX + TOO_NEAR_EDGE || map.x + 1 > scrollMaxX - TOO_NEAR_EDGE ||
	    map.y < scrollMinY + TOO_NEAR_EDGE || map.y + 1 > scrollMaxY - TOO_NEAR_EDGE)
	{
		return false;
	}

	// cant place on top of a delivery point...
	for (const auto& psFlag : apsFlagPosLists[selectedPlayer])
	{
		Vector2i flagTile = map_coord(psFlag->coords.xy());
		if (flagTile == map)
		{
			return false;
		}
	}

	if (fpathBlockingTile(map.x, map.y, PROPULSION_TYPE_WHEELED))
	{
		return false;
	}

	return true;
}

bool deliveryReposFinished(FLAG_POSITION *psFlag)
{
	if (!flagReposVarsValid)
	{
		return false;
	}

	if (psFlag)
	{
		*psFlag = flagPos;
	}
	return flagReposFinished;
}

void processDeliveryRepos()
{
	if (!flagReposVarsValid)
	{
		return;
	}

	int bX = clip<int>(mouseTileX, 2, mapWidth - 3);
	int bY = clip<int>(mouseTileY, 2, mapHeight - 3);

	flagPos.coords = Vector3i(world_coord(Vector2i(bX, bY)) + Vector2i(TILE_UNITS / 2, TILE_UNITS / 2), map_TileHeight(bX, bY) + 2 * ASSEMBLY_POINT_Z_PADDING);
}

// Cancel repositioning of the delivery point without moving it.
//
void cancelDeliveryRepos()
{
	flagReposVarsValid = false;
}

void renderDeliveryRepos(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	if (flagReposVarsValid)
	{
		renderDeliveryPoint(&flagPos, true, viewMatrix, perspectiveViewMatrix);
	}
}

// check whether a clicked on droid is in a command group or assigned to a sensor
static bool droidHasLeader(DROID *psDroid)
{
	BASE_OBJECT		*psLeader;

	if (psDroid->droidType == DROID_COMMAND ||
	    psDroid->droidType == DROID_SENSOR)
	{
		return false;
	}

	if (hasCommander(psDroid))
	{
		psLeader = (BASE_OBJECT *)psDroid->psGroup->psCommander;
	}
	else
	{
		//psLeader can be either a droid or a structure
		psLeader = orderStateObj(psDroid, DORDER_FIRESUPPORT);
	}

	if (psLeader != nullptr)
	{
		if (psLeader->type == OBJ_DROID)
		{
			SelectDroid((DROID *)psLeader);
		}
		assignSensorTarget(psLeader);
		return true;
	}

	return false;
}


// deal with selecting a droid
void dealWithDroidSelect(DROID *psDroid, bool bDragBox)
{
	/*	Toggle selection on and off - allows you drag around a big
		area of droids and then exclude certain individuals */
	if (!bDragBox && (psDroid->selected))
	{
		DeSelectDroid(psDroid);
	}
	else if (ctrlShiftDown() || !droidHasLeader(psDroid))
	{
		if (specialOrderKeyDown())
		{
			/* We only want to select weapon units if ALT is down on a drag */
			if (psDroid->asWeaps[0].nStat > 0)
			{
				SelectDroid(psDroid);
			}
		}
		else
		{
			SelectDroid(psDroid);
		}
	}
}

static void FeedbackOrderGiven()
{
	static UDWORD LastFrame = 0;
	UDWORD ThisFrame = frameGetFrameNumber();

	// Ensure only played once per game cycle.
	if (ThisFrame != LastFrame)
	{
		audio_PlayTrack(ID_SOUND_SELECT);
		LastFrame = ThisFrame;
	}
}

// check whether the queue order keys are pressed
bool ctrlShiftDown()
{
	return keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
}

void AddDerrickBurningMessage()
{
	if (addConsoleMessageDebounced(_("Cannot Build. Oil Resource Burning."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, CANNOT_BUILD_BURNING))
	{
		audio_PlayBuildFailedOnce();
	}
}

static void printDroidClickInfo(DROID *psDroid)
{
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (dbgInputManager.debugMappingsAllowed()) // cheating on, so output debug info
	{
		console("%s - Hitpoints %d/%d - ID %d - experience %f, %s - order %s - action %s - sensor range %hu - ECM %u - pitch %.0f - frust %u - kills %d",
		        droidGetName(psDroid), psDroid->body, psDroid->originalBody, psDroid->id,
		        psDroid->experience / 65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order.type), getDroidActionName(psDroid->action),
		        droidSensorRange(psDroid), objJammerPower(psDroid), UNDEG(psDroid->rot.pitch), psDroid->lastFrustratedTime, psDroid->kills);
		FeedbackOrderGiven();
	}
	else if (!psDroid->selected)
	{
		console(_("%s - Hitpoints %d/%d - Experience %.1f, %s, Kills %d"), droidGetName(psDroid), psDroid->body, psDroid->originalBody,
		        psDroid->experience / 65536.f, _(getDroidLevelName(psDroid)), psDroid->kills);
		FeedbackOrderGiven();
	}
	clearSelection();
	dealWithDroidSelect(psDroid, false);
}

static void dealWithLMBDroid(DROID *psDroid, SELECTION_TYPE selection)
{
	bool ownDroid; // Not an allied droid
	const bool isSpectator = bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator;
	if (isSpectator)
	{
		console(_("%s - Hitpoints %d/%d - ID %d - %s"), droidGetName(psDroid), psDroid->body, psDroid->originalBody, psDroid->id, getDroidLevelName(psDroid));
		return;
	}

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op
	}

	if (!aiCheckAlliances(selectedPlayer, psDroid->player))
	{
		memset(DROIDDOING, 0x0 , sizeof(DROIDDOING)); // take over the other players droid by debug menu.
		/* We've clicked on enemy droid */
		const DebugInputManager& dbgInputManager = gInputManager.debugManager();
		if (dbgInputManager.debugMappingsAllowed())
		{
			console(_("(Enemy!) %s - Hitpoints %d/%d - ID %d - experience %f, %s - order %s - action %s - sensor range %d - ECM %d - pitch %.0f"),
			        droidGetName(psDroid),  psDroid->body, psDroid->originalBody, psDroid->id,
			        psDroid->experience / 65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order.type),
			        getDroidActionName(psDroid->action), droidSensorRange(psDroid), objJammerPower(psDroid), UNDEG(psDroid->rot.pitch));
			FeedbackOrderGiven();
		}
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT *)psDroid, ctrlShiftDown());

		//lasSat structure can select a target - in multiPlayer only
		if (bMultiPlayer && bLasSatStruct)
		{
			orderStructureObj(selectedPlayer, (BASE_OBJECT *)psDroid);
		}

		FeedbackOrderGiven();
		return;
	}

	ownDroid = (selectedPlayer == psDroid->player);
	// Hack to detect if sensor was assigned
	bSensorAssigned = true;
	if (!bRightClickOrders && ctrlShiftDown() && ownDroid)
	{
		// select/deselect etc. the droid
		dealWithDroidSelect(psDroid, false);
	}
	else if (specialOrderKeyDown() && ownDroid)
	{
		// try to attack your own unit
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT *)psDroid, ctrlShiftDown());
		FeedbackOrderGiven();
	}
	else if (psDroid->isTransporter())
	{
		if (selection == SC_INVALID)
		{
			//in multiPlayer mode we RMB to get the interface up
			if (bMultiPlayer && !bRightClickOrders)
			{
				psDroid->selected = true;
				triggerEventSelected();
			}
			else
			{
				intResetScreen(false);
				if (!getWidgetsStatus())
				{
					setWidgetsStatus(true);
				}
				addTransporterInterface(psDroid, false);
			}
		}
		else
		{
			// We can order all units to use the transport now
			if (cyborgDroidSelected(selectedPlayer))
			{
				// TODO add special processing for cyborgDroids
			}
			orderSelectedObj(selectedPlayer, (BASE_OBJECT *)psDroid);
			FeedbackOrderGiven();
		}
	}
	// Clicked on a commander? Will link to it.
	else if (psDroid->droidType == DROID_COMMAND && selection != SC_INVALID &&
	         selection != SC_DROID_COMMAND &&
	         selection != SC_DROID_CONSTRUCT &&
	         !ctrlShiftDown() && ownDroid)
	{
		turnOffMultiMsg(true);
		orderSelectedObj(selectedPlayer, (BASE_OBJECT *)psDroid);
		FeedbackOrderGiven();
		clearSelection();
		assignSensorTarget((BASE_OBJECT *)psDroid);
		dealWithDroidSelect(psDroid, false);
		turnOffMultiMsg(false);
	}
	// Clicked on a sensor? Will assign to it.
	else if (psDroid->droidType == DROID_SENSOR)
	{
		bSensorAssigned = false;
		for (DROID* psCurr : apsDroidLists[selectedPlayer])
		{
			//must be indirect weapon droid or VTOL weapon droid
			if ((psCurr->droidType == DROID_WEAPON) &&
			    (psCurr->selected) &&
			    (psCurr->asWeaps[0].nStat > 0) &&
			    ((!proj_Direct(psCurr->getWeaponStats(0))) ||
			     psCurr->isVtol()) &&
			    droidSensorDroidWeapon((BASE_OBJECT *)psDroid, psCurr))
			{
				bSensorAssigned = true;
				orderDroidObj(psCurr, DORDER_FIRESUPPORT, (BASE_OBJECT *)psDroid, ModeQueue);
				FeedbackOrderGiven();
			}
		}
		if (bSensorAssigned)
		{
			clearSelection();
			assignSensorTarget((BASE_OBJECT *)psDroid);
		}
	}
	// Hack to detect if anything was done with sensor
	else
	{
		bSensorAssigned = false;
	}
	if (bSensorAssigned)
	{
		return;
	}
	// Clicked on a construction unit? Will guard it.
	else if ((psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_SENSOR ||
	          psDroid->droidType == DROID_COMMAND)
	         && selection == SC_DROID_DIRECT)
	{
		orderSelectedObj(selectedPlayer, (BASE_OBJECT *)psDroid);
		FeedbackOrderGiven();
	}
	// Clicked on a damaged unit? Will repair it.
	else if (psDroid->isDamaged() && repairDroidSelected(selectedPlayer))
	{
		assignDestTarget();
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT *)psDroid, ctrlShiftDown());
		FeedbackOrderGiven();
	}
	else if (bRightClickOrders && ownDroid)
	{
		if (!(psDroid->selected))
		{
			clearSelection();
			SelectDroid(psDroid);
		}
		intObjectSelected((BASE_OBJECT *)psDroid);
	}
	// Just plain clicked on?
	else if (ownDroid)
	{
		printDroidClickInfo(psDroid);
	}
	else // Clicked on allied unit with no other possible actions
	{
		console(_("%s - Allied - Hitpoints %d/%d - Experience %d, %s"), droidGetName(psDroid), psDroid->body, psDroid->originalBody,
		        psDroid->experience / 65536, getDroidLevelName(psDroid));
		FeedbackOrderGiven();
	}
}

static void dealWithLMBStructure(STRUCTURE *psStructure, SELECTION_TYPE selection)
{
	bool ownStruct = (psStructure->player == selectedPlayer);
	const bool isSpectator = bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator;

	if (isSpectator)
	{
		printStructureInfo(psStructure);
		return;
	}

	if (selectedPlayer < MAX_PLAYERS && !aiCheckAlliances(psStructure->player, selectedPlayer))
	{
		/* We've clicked on an enemy building */
		const DebugInputManager& dbgInputManager = gInputManager.debugManager();
		if (dbgInputManager.debugMappingsAllowed())
		{
			// TRANSLATORS: "ref" is an internal unique id of the item (can leave untranslated as a technical term)
			console(_("(Enemy!) %s, ref: %d, ID: %d Hitpoints: %d/%d"), getID(psStructure->pStructureType), psStructure->pStructureType->ref,
			        psStructure->id, psStructure->body, psStructure->pStructureType->upgrade[psStructure->player].hitpoints);
		}
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT *)psStructure, ctrlShiftDown());
		//lasSat structure can select a target - in multiPlayer only
		if (bMultiPlayer && bLasSatStruct)
		{
			orderStructureObj(selectedPlayer, (BASE_OBJECT *)psStructure);
		}
		FeedbackOrderGiven();
		return;
	}

	/* We've clicked on allied or own building */

	//print some info at the top of the screen for the specific structure
	if (!bRightClickOrders)
	{
		printStructureInfo(psStructure);
	}

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // do not proceed
	}

	/* Got to be built. Also, you can't 'select' derricks */
	if (!specialOrderKeyDown() && (psStructure->status == SS_BUILT) && !psStructure->flags.test(OBJECT_FLAG_UNSELECTABLE) &&
	    (psStructure->pStructureType->type != REF_RESOURCE_EXTRACTOR) && ownStruct)
	{
		if (bRightClickOrders)
		{
			if (psStructure->isFactory() && selection != SC_DROID_CONSTRUCT)
			{
				intAddFactoryOrder(psStructure);
			}
		}
		else
		{
			auto shouldDisplayInterface = !anyDroidSelected(selectedPlayer);
			if (selection == SC_INVALID)
			{
				/* Clear old building selection(s) - should only be one */
				for (STRUCTURE* psCurr : apsStructLists[selectedPlayer])
				{
					psCurr->selected = false;
				}
				/* Establish new one */
				psStructure->selected = true;
				triggerEventSelected();
				jsDebugSelected(psStructure);
			}
			//determine if LasSat structure has been selected
			bLasSatStruct = lasSatStructSelected(psStructure);

			if (shouldDisplayInterface)
			{
				intObjectSelected((BASE_OBJECT *)psStructure);
				FeedbackOrderGiven();
			}
		}

	}
	else if ((psStructure->status == SS_BUILT) && !psStructure->flags.test(OBJECT_FLAG_UNSELECTABLE) &&
	         (psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR) &&
	         selection == SC_INVALID && ownStruct)
	{
		/* Clear old building selection(s) - should only be one */
		for (STRUCTURE* psCurr : apsStructLists[selectedPlayer])
		{
			psCurr->selected = false;
		}
		/* Establish new one */
		psStructure->selected = true;
		triggerEventSelected();
		jsDebugSelected(psStructure);
	}
	bSensorAssigned = false;
	orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT *)psStructure, ctrlShiftDown());
	FeedbackOrderGiven();
	if (bSensorAssigned)
	{
		clearSelection();
		assignSensorTarget((BASE_OBJECT *)psStructure);
	}
	if (intDemolishSelectMode())
	{
		// we were demolishing something - now we're done
		if (ctrlShiftDown())
		{
			quickQueueMode = true;
		}
		else
		{
			intDemolishCancel();
		}
	}
}

static void dealWithLMBFeature(FEATURE *psFeature)
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		goto debugOuput;
	}

	//go on to check for
	if (psFeature->psStats->damageable)
	{
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT *)psFeature, ctrlShiftDown());
		//lasSat structure can select a target - in multiPlayer only
		if (bMultiPlayer && bLasSatStruct)
		{
			orderStructureObj(selectedPlayer, (BASE_OBJECT *)psFeature);
		}
		FeedbackOrderGiven();
	}

	//clicking an oil field should start a build..
	if (psFeature->psStats->subType == FEAT_OIL_RESOURCE)
	{
		unsigned int i;
		// find any construction droids. and order them to build an oil resource.

		// first find the derrick.
		for (i = 0; (i < numStructureStats) && (asStructureStats[i].type != REF_RESOURCE_EXTRACTOR); ++i) {}

		if ((i < numStructureStats) &&
		    (apStructTypeLists[selectedPlayer][i] == AVAILABLE))	// don't go any further if no derrick stat found.
		{
			// for each droid
			for (DROID* psCurr : apsDroidLists[selectedPlayer])
			{
				if ((droidType(psCurr) == DROID_CONSTRUCT ||
				     droidType(psCurr) == DROID_CYBORG_CONSTRUCT) && (psCurr->selected))
				{
					if (fireOnLocation(psFeature->pos.x, psFeature->pos.y))
					{
						// Can't build because it's burning
						AddDerrickBurningMessage();
					}

					sendDroidInfo(psCurr, DroidOrder(DORDER_BUILD, &asStructureStats[i], psFeature->pos.xy(), playerPos.r.y), ctrlShiftDown());
					FeedbackOrderGiven();
				}
			}
		}
	}
	else
	{
		switch (psFeature->psStats->subType)
		{
		case FEAT_GEN_ARTE:
		case FEAT_OIL_DRUM:
			{
				DROID *psNearestUnit = getNearestDroid(mouseTileX * TILE_UNITS + TILE_UNITS / 2,
				                                       mouseTileY * TILE_UNITS + TILE_UNITS / 2, true);
				/* If so then find the nearest unit! */
				if (psNearestUnit)	// bloody well should be!!!
				{
					sendDroidInfo(psNearestUnit, DroidOrder(DORDER_RECOVER, psFeature), ctrlShiftDown());
					FeedbackOrderGiven();
				}
				break;
			}
		case FEAT_BOULDER:
		case FEAT_OIL_RESOURCE:
		case FEAT_VEHICLE:
		default:
			break;
		}
	}

debugOuput:
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (dbgInputManager.debugMappingsAllowed())
	{
		console("(Feature) %s ID: %d ref: %d Hitpoints: %d/%d", getID(psFeature->psStats), psFeature->id, psFeature->psStats->ref, psFeature->body, psFeature->psStats->body);
	}
}

static void dealWithLMBObject(BASE_OBJECT *psClickedOn)
{
	SELECTION_TYPE selection = establishSelection(selectedPlayer);
	OBJECT_TYPE type = psClickedOn->type;

	switch (type)
	{
	case OBJ_DROID:
		dealWithLMBDroid((DROID *)psClickedOn, selection);
		break;

	case OBJ_STRUCTURE:
		dealWithLMBStructure((STRUCTURE *)psClickedOn, selection);
		break;

	case OBJ_FEATURE:
		dealWithLMBFeature((FEATURE *)psClickedOn);
		break;

	default:
		// assert only when the value is outside of the valid range
		ASSERT(type < OBJ_NUM_TYPES, "Weird selection from LMB - type of clicked object is %d", (int)type);
		break;
	}
}

void	dealWithLMB()
{
	BASE_OBJECT         *psClickedOn;
	STRUCTURE			*psStructure;

	/* Don't process if in game options are on screen */
	if (isMouseOverRadar() ||
	    InGameOpUp == true || widgGetFromID(psWScreen, INTINGAMEOP))
	{
		return;
	}

	/* What have we clicked on? */
	psClickedOn = mouseTarget();
	if (psClickedOn)
	{
		dealWithLMBObject(psClickedOn);

		return;
	}

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // shortcut the rest (for now)
	}

	if (auto deliveryPoint = findMouseDeliveryPoint())
	{
		if (selNumSelected(selectedPlayer) == 0) {
			if (bRightClickOrders)
			{
				//centre the view on the owning Factory
				psStructure = findDeliveryFactory(deliveryPoint);
				if (psStructure)
				{
					setViewPos(map_coord(psStructure->pos.x), map_coord(psStructure->pos.y), true);
				}
			}
			else
			{
				startDeliveryPosition(deliveryPoint);
			}
			return;
		}
	}

	// now changed to use the multiple order stuff
	// clicked on a destination.
	orderSelectedLoc(selectedPlayer, mousePos.x, mousePos.y, ctrlShiftDown());  // ctrlShiftDown() = ctrl clicked a destination, add an order
	/* Otherwise send them all */
	if (getNumDroidsSelected())
	{
		assignDestTarget();
		audio_PlayTrack(ID_SOUND_SELECT);
	}

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (dbgInputManager.debugMappingsAllowed() && tileOnMap(mouseTileX, mouseTileY))
	{
		MAPTILE *psTile = mapTile(mouseTileX, mouseTileY);
		uint8_t aux = auxTile(mouseTileX, mouseTileY, selectedPlayer);

		int flipVal = 0;
		if (TileNumber_texture(psTile->texture) & TILE_XFLIP)
		{
			flipVal += 2;
		}
		if (TileNumber_texture(psTile->texture) & TILE_YFLIP)
		{
			flipVal += 1;
		}

		console("%s tile %d, %d [%d, %d] continent(l%d, h%d) level %g illum %d ao %d col %x %s %s w=%d s=%d j=%d tile#%d (decal=%s, ground [#%d, size=%.3f], f%d r%d)",
		        tileIsExplored(psTile) ? "Explored" : "Unexplored",
		        mouseTileX, mouseTileY, world_coord(mouseTileX), world_coord(mouseTileY),
		        (int)psTile->limitedContinent, (int)psTile->hoverContinent, psTile->level, (int)psTile->illumination,
				(int)psTile->ambientOcclusion, getCurrentLightmapData()(mouseTileX, mouseTileY).rgba(),
		        aux & AUXBITS_DANGER ? "danger" : "", aux & AUXBITS_THREAT ? "threat" : "",
		        (int)psTile->watchers[selectedPlayer], (int)psTile->sensors[selectedPlayer], (int)psTile->jammers[selectedPlayer],
				TileNumber_tile(psTile->texture), (TILE_HAS_DECAL(psTile)) ? "y" : "n",
				psTile->ground, getGroundType(psTile->ground).textureSize,
				flipVal, (TileNumber_texture(psTile->texture) & TILE_ROTMASK) >> TILE_ROTSHIFT);
	}
}

bool	getRotActive()
{
	return (rotActive);
}

// process LMB double clicks
static void dealWithLMBDClick()
{
	BASE_OBJECT		*psClickedOn;
	DROID			*psDroid;
	STRUCTURE		*psStructure;

	/* What have we clicked on? */
	psClickedOn = mouseTarget();
	/* If not NULL, then it's a droid or a structure */
	if (psClickedOn != nullptr)
	{
		/* We've got a droid or a structure */
		if (psClickedOn->type == OBJ_DROID)
		{
			/* We clicked on droid */
			psDroid = (DROID *) psClickedOn;
			if (psDroid->player == selectedPlayer)
			{
				// Now selects all of same type on screen
				selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_ALL_SAME, true);
			}
		}
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
			/* We clicked on structure */
			psStructure = (STRUCTURE *) psClickedOn;
			if (psStructure->player == selectedPlayer && !psStructure->isBlueprint())
			{
				if (psStructure->isFactory())
				{
					setViewPos(map_coord(psStructure->pFunctionality->factory.psAssemblyPoint->coords.x),
					           map_coord(psStructure->pFunctionality->factory.psAssemblyPoint->coords.y),
					           true);
				}
				else if (psStructure->pStructureType->type == REF_REPAIR_FACILITY)
				{
					setViewPos(map_coord(psStructure->pFunctionality->repairFacility.psDeliveryPoint->coords.x),
					           map_coord(psStructure->pFunctionality->repairFacility.psDeliveryPoint->coords.y),
					           true);
				}
			}
		}
	}
}

/**
 * Find a delivery point, owned by `selectedPlayer`, pointed by the mouse.
 */
static FLAG_POSITION *findMouseDeliveryPoint()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return nullptr;
	}

	for (const auto& psPoint : apsFlagPosLists[selectedPlayer])
	{
		if (psPoint->type != POS_DELIVERY) {
			continue;
		}

		auto dispX = psPoint->screenX;
		auto dispY = psPoint->screenY;
		auto dispR = psPoint->screenR;
		if (DrawnInLastFrame(psPoint->frameNumber) == true)	// Only check DP's that are on screen
		{
			if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
			{
				// We HAVE clicked on DP!
				return psPoint;
			}
		}
	}

	return nullptr;
}

static void dealWithRMB()
{
	BASE_OBJECT			*psClickedOn;
	DROID				*psDroid;
	STRUCTURE			*psStructure;

	if (isMouseOverRadar() || InGameOpUp == true || widgGetFromID(psWScreen, INTINGAMEOP))
	{
		return;
	}

	/* What have we clicked on? */
	psClickedOn = mouseTarget();
	/* If not NULL, then it's a droid or a structure */
	if (psClickedOn != nullptr)
	{
		/* We've got a droid or a structure */
		if (psClickedOn->type == OBJ_DROID)
		{
			/* We clicked on droid */
			psDroid = (DROID *) psClickedOn;
			if (psDroid->player == selectedPlayer)
			{
				if (bRightClickOrders && ctrlShiftDown())
				{
					dealWithDroidSelect(psDroid, false);
				}
				// Not a transporter
				else if (!psDroid->isTransporter())
				{
					if (bRightClickOrders)
					{
						/* We've clicked on one of our own droids */
						printDroidClickInfo(psDroid);
					}
					else
					{
						if (!(psDroid->selected))
						{
							clearSelection();
							SelectDroid(psDroid);
						}
						intObjectSelected((BASE_OBJECT *)psDroid);
					}
				}
				// Transporter
				else
				{
					if (bMultiPlayer)
					{
						if (bRightClickOrders && !(psDroid->selected))
						{
							clearSelection();
							SelectDroid(psDroid);
						}
						else
						{
							intResetScreen(false);
							if (!getWidgetsStatus())
							{
								setWidgetsStatus(true);
							}
							addTransporterInterface(psDroid, false);
						}
					}
				}
			}
			else if (bMultiPlayer && isHumanPlayer(psDroid->player))
			{
				console("%s", droidGetName(psDroid));
				FeedbackOrderGiven();
			}
		}	// end if its a droid
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
			/* We clicked on structure */
			psStructure = (STRUCTURE *) psClickedOn;
			if (psStructure->player == selectedPlayer)
			{
				/* We've clicked on our own building */
				if (bRightClickOrders && intDemolishSelectMode())
				{
					orderSelectedObjAdd(selectedPlayer, psClickedOn, ctrlShiftDown());
					FeedbackOrderGiven();
					// we were demolishing something - now we're done
					if (ctrlShiftDown())
					{
						quickQueueMode = true;
					}
					else
					{
						intDemolishCancel();
					}
				}
				else if (psStructure->selected)
				{
					psStructure->selected = false;
					intObjectSelected(nullptr);
					triggerEventSelected();
					jsDebugSelected(psStructure);
				}
				else if (!psStructure->isBlueprint())
				{
					clearSelection();

					if (bRightClickOrders)
					{
						if ((psStructure->status == SS_BUILT) &&
						    (psStructure->pStructureType->type != REF_RESOURCE_EXTRACTOR))
						{
							printStructureInfo(psStructure);

							psStructure->selected = true;
							jsDebugSelected(psStructure);

							// Open structure menu
							intObjectSelected((BASE_OBJECT *)psStructure);
							FeedbackOrderGiven();

							bLasSatStruct = lasSatStructSelected(psStructure);
							triggerEventSelected();
						}
					}
					else if (psStructure->isFactory())
					{
						//pop up the order interface for the factory
						intAddFactoryOrder(psStructure);
					}
					else
					{
						intObjectSelected((BASE_OBJECT *)psStructure);
					}
				}
			}
		}	// end if its a structure
		else
		{
			/* And if it's not a feature, then we're in trouble! */
			ASSERT(psClickedOn->type == OBJ_FEATURE, "Weird selection from RMB - type of clicked object is %d", (int)psClickedOn->type);
		}
	}
	else
	{
		if (auto deliveryPoint = findMouseDeliveryPoint())
		{
			if (bRightClickOrders)
			{
				startDeliveryPosition(deliveryPoint);
			}
			else
			{
				//centre the view on the owning Factory
				psStructure = findDeliveryFactory(deliveryPoint);
				if (psStructure)
				{
					setViewPos(map_coord(psStructure->pos.x), map_coord(psStructure->pos.y), true);
				}
			}
		}
		else
		{
			handleDeselectionClick();
		}
	}
}

/* if there is a valid object under the mouse this routine returns not only the type of the object in the
return code, but also a pointer to the BASE_OBJECT) ... well if your going to be "object orientated" you might as well do it right
- it sets it to null if we don't find anything
*/
static MOUSE_TARGET	itemUnderMouse(BASE_OBJECT **ppObjectUnderMouse)
{
	UDWORD		i;
	MOUSE_TARGET retVal;
	BASE_OBJECT	 *psNotDroid;
	UDWORD		dispX, dispY, dispR;
	STRUCTURE	*psStructure;

	*ppObjectUnderMouse = nullptr;

	if (mouseTileX < 0 || mouseTileY < 0 || mouseTileX > (int)(mapWidth - 1) || mouseTileY > (int)(mapHeight - 1))
	{
		retVal = MT_BLOCKING;
		return retVal;
	}

	/* We haven't found anything yet */
	retVal = MT_NOTARGET;

	/* First have a look through the droid lists */
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (DROID* psDroid : apsDroidLists[i])
		{
			dispX = psDroid->sDisplay.screenX;
			dispY = psDroid->sDisplay.screenY;
			dispR = psDroid->sDisplay.screenR;
			/* Only check droids that're on screen */
			if (psDroid->sDisplay.frameNumber + 1 == currentFrame && psDroid->visibleForLocalDisplay())
			{
				if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
				{
					/* We HAVE clicked on droid! */
					if (selectedPlayer < MAX_PLAYERS && aiCheckAlliances(psDroid->player, selectedPlayer))
					{
						*ppObjectUnderMouse = (BASE_OBJECT *)psDroid;
						// need to check for command droids here as well
						if (psDroid->droidType == DROID_SENSOR)
						{
							if (selectedPlayer != psDroid->player)
							{
								retVal = MT_CONSTRUCT; // Can't assign to allied units
							}
							else
							{
								retVal = MT_SENSOR;
							}
						}
						else if (psDroid->isTransporter() &&
						         selectedPlayer == psDroid->player)
						{
							//check the transporter is not full
							if (calcRemainingCapacity(psDroid))
							{
								retVal = MT_TRANDROID;
							}
							else
							{
								retVal = MT_BLOCKING;
							}
						}
						else if (psDroid->droidType == DROID_CONSTRUCT ||
						         psDroid->droidType == DROID_CYBORG_CONSTRUCT)
						{
							return MT_CONSTRUCT;
						}
						else if (psDroid->droidType == DROID_COMMAND)
						{
							if (selectedPlayer != psDroid->player)
							{
								retVal = MT_CONSTRUCT; // Can't assign to allied units
							}
							else
							{
								retVal = MT_COMMAND;
							}
						}
						else
						{
							if (psDroid->isDamaged())
							{
								retVal = MT_OWNDROIDDAM;
							}
							else
							{
								retVal = MT_OWNDROID;
							}
						}
					}
					else
					{
						*ppObjectUnderMouse = (BASE_OBJECT *)psDroid;
						retVal = MT_ENEMYDROID;
					}
					/* There's no point in checking other object types */
					return (retVal);
				}
			}
		}
	} // end of checking for droids

	/*	Not a droid, so maybe a structure or feature?
		If still NULL after this then nothing */
	psNotDroid = getTileOccupier(mouseTileX, mouseTileY);
	if (psNotDroid == nullptr)
	{
		psNotDroid = getTileBlueprintStructure(mouseTileX, mouseTileY);
	}

	if (psNotDroid != nullptr)
	{
		*ppObjectUnderMouse = (BASE_OBJECT *)psNotDroid;

		if (psNotDroid->type == OBJ_FEATURE)
		{
			if ((((FEATURE *)psNotDroid)->psStats->subType == FEAT_GEN_ARTE)
			    || (((FEATURE *)psNotDroid)->psStats->subType == FEAT_OIL_DRUM))
			{
				retVal = MT_ARTIFACT;
			}
			else if (((FEATURE *)psNotDroid)->psStats->damageable)	//make damageable features return 'target' mouse pointer
			{
				retVal = MT_DAMFEATURE;
			}
			else if (((FEATURE *)psNotDroid)->psStats->subType == FEAT_OIL_RESOURCE)
			{
				retVal = MT_RESOURCE;
			}
			else
			{
				retVal = MT_BLOCKING;
			}
		}
		else if (psNotDroid->type == OBJ_STRUCTURE)
		{
			psStructure = (STRUCTURE *)psNotDroid;

			if (selectedPlayer < MAX_PLAYERS && aiCheckAlliances(psNotDroid->player, selectedPlayer))
			{
				if (psStructure->status == SS_BEING_BUILT || isBlueprint(psStructure))
				{
					retVal = MT_OWNSTRINCOMP;
				}
				// repair center.
				else if (psStructure->pStructureType->type == REF_REPAIR_FACILITY)
				{
					if (buildingDamaged(psStructure))
					{
						retVal = MT_REPAIRDAM;
					}
					else
					{
						retVal = MT_REPAIR;
					}
				}
				//sensor tower
				else if ((psStructure->pStructureType->pSensor) &&
				         (psStructure->pStructureType->pSensor->location == LOC_TURRET))
				{
					if (buildingDamaged(psStructure))
					{
						retVal = MT_SENSORSTRUCTDAM;
					}
					else
					{
						retVal = MT_SENSORSTRUCT;
					}
				}

				// standard buildings. - check for buildingDamaged BEFORE upgrades
				else if (buildingDamaged(psStructure))
				{
					retVal = MT_OWNSTRDAM;
				}
				// If this building is a factory/power generator/research facility
				// which isn't upgraded. Make the build icon available.
				else if (nextModuleToBuild(psStructure, -1) > 0)
				{
					retVal = MT_OWNSTRINCOMP;
				}
				else
				{
					/* All the different stages of construction */
					retVal = MT_OWNSTROK;
				}
			}
			else
			{
				retVal = MT_ENEMYSTR;	// enemy structure
			}
		}
	}

	/* Send the result back - if it's null then we clicked on an area of terrain */
	/* make unseen objects just look like terrain. */
	if (retVal == MT_NOTARGET || (psNotDroid && !(psNotDroid->visibleForLocalDisplay())))
	{
		retVal = MT_TERRAIN;
	}
	return (retVal);
}

// Indicates the priority given to any given droid
// type in a multiple droid selection, the larger the
// number, the lower the priority. The order of entries
// corresponds to the order of droid types in the DROID_TYPE
// enum in DroidDef.h
//
#define NUM_DROID_WEIGHTS (14)
static UBYTE DroidSelectionWeights[NUM_DROID_WEIGHTS] =
{
	3,	//DROID_WEAPON,
	1,	//DROID_SENSOR,
	2,	//DROID_ECM,
	4,	//DROID_CONSTRUCT,
	3,	//DROID_PERSON,
	3,	//DROID_CYBORG,
	9,	//DROID_TRANSPORTER,
	0,	//DROID_COMMAND,
	4,	//DROID_REPAIR,
	5,	//DROID_DEFAULT,
	4,	//DROID_CYBORG_CONSTRUCT,
	4,	//DROID_CYBORG_REPAIR,
	3,	//DROID_CYBORG_SUPER,
	10, //DROID_SUPERTRANSPORTER
};

/* Only deals with one type of droid being selected!!!! */
/*	We'll have to make it assess which selection is to be dominant in the case
	of multiple selections */
static SELECTION_TYPE	establishSelection(UDWORD _selectedPlayer)
{
	DROID *psDominant = nullptr;
	UBYTE CurrWeight = UBYTE_MAX;
	SELECTION_TYPE selectionClass = SC_INVALID;

	if (_selectedPlayer >= MAX_PLAYERS)
	{
		return SC_INVALID;
	}

	for (DROID *psDroid : apsDroidLists[_selectedPlayer])
	{
		// This works, uses the DroidSelectionWeights[] table to priorities the different
		// droid types and find the dominant selection.
		if (psDroid->selected)
		{
			ASSERT_OR_RETURN(SC_INVALID, psDroid->droidType < NUM_DROID_WEIGHTS, "droidType exceeds NUM_DROID_WEIGHTS");
			if (DroidSelectionWeights[psDroid->droidType] < CurrWeight)
			{
				CurrWeight = DroidSelectionWeights[psDroid->droidType];
				psDominant = psDroid;
			}
		}
	}

	if (psDominant)
	{
		psDominantSelected = psDominant;
		switch (psDominant->droidType)
		{
		case DROID_WEAPON:
			if (proj_Direct(psDominant->getWeaponStats(0)))
			{
				selectionClass = SC_DROID_DIRECT;
			}
			else
			{
				selectionClass = SC_DROID_INDIRECT;
			}
			break;

		case DROID_PERSON:
			selectionClass = SC_DROID_DIRECT;
			break;
		case DROID_CYBORG:
		case DROID_CYBORG_SUPER:
			selectionClass = SC_DROID_DIRECT;
			break;
		case DROID_TRANSPORTER:
		case DROID_SUPERTRANSPORTER:
			//can remove this is NEVER going to select the Transporter to move
			//Never say Never!! cos here we go in multiPlayer!!
			selectionClass = SC_DROID_TRANSPORTER;
			break;
		case DROID_SENSOR:
			selectionClass = SC_DROID_SENSOR;
			break;

		case DROID_ECM:
			selectionClass = SC_DROID_ECM;
			break;

		case DROID_CONSTRUCT:
		case DROID_CYBORG_CONSTRUCT:
			if (intDemolishSelectMode())
			{
				return SC_DROID_DEMOLISH;
			}
			selectionClass = SC_DROID_CONSTRUCT;
			break;

		case DROID_COMMAND:
			selectionClass = SC_DROID_COMMAND;
			break;

		case DROID_REPAIR:
		case DROID_CYBORG_REPAIR:
			selectionClass = SC_DROID_REPAIR;
			break;

		default:
			ASSERT(!"unknown droid type", "Weirdy droid type on what you've clicked on!!!");
			break;

		}
	}
	return (selectionClass);
}

/* Just returns true if the building's present body points aren't 100 percent */
static bool	buildingDamaged(STRUCTURE *psStructure)
{
	return psStructure->body < psStructure->structureBody();
}

/*Looks through the list of selected players droids to see if one is a repair droid*/
bool	repairDroidSelected(UDWORD player)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player (%" PRIu32 ")", player);

	for (const DROID* psCurr : apsDroidLists[player])
	{
		if (psCurr->selected && (
		        psCurr->droidType == DROID_REPAIR ||
		        psCurr->droidType == DROID_CYBORG_REPAIR))
		{
			return true;
		}
	}

	//didn't find one...
	return false;
}

/*Looks through the list of selected players droids to see if one is a VTOL droid*/
bool	vtolDroidSelected(UDWORD player)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "player: %" PRIu32 "", player);

	for (DROID* psCurr : apsDroidLists[player])
	{
		if (psCurr->selected && psCurr->isVtol())
		{
			// horrible hack to note one of the selected vtols
			psSelectedVtol = psCurr;
			return true;
		}
	}

	//didn't find one...
	return false;
}

/*Looks through the list of selected players droids to see if any is selected*/
bool	anyDroidSelected(UDWORD player)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player (%" PRIu32 ")", player);

	for (const DROID* psCurr : apsDroidLists[player])
	{
		if (psCurr->selected)
		{
			return true;
		}
	}

	//didn't find one...
	return false;
}

/*Looks through the list of selected players droids to see if one is a cyborg droid*/
bool cyborgDroidSelected(UDWORD player)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player (%" PRIu32 ")", player);

	for (const DROID* psCurr : apsDroidLists[player])
	{
		if (psCurr->selected && psCurr->isCyborg())
		{
			return true;
		}
	}

	//didn't find one...
	return false;
}

/* Clear the selection flag for a player */
void clearSelection()
{
	memset(DROIDDOING, 0x0 , sizeof(DROIDDOING));	// clear string when deselected

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}

	for (DROID* psCurrDroid : apsDroidLists[selectedPlayer])
	{
		psCurrDroid->selected = false;
	}
	for (STRUCTURE* psStruct : apsStructLists[selectedPlayer])
	{
		psStruct->selected = false;
	}
	bLasSatStruct = false;
	//clear the Deliv Point if one
	for (auto& psFlag : apsFlagPosLists[selectedPlayer])
	{
		psFlag->selected = false;
	}

	intRefreshScreen();

	triggerEventSelected();
}

static void handleDeselectionClick()
{
	clearSelection();
	intObjectSelected(nullptr);
}

//access function for bSensorAssigned variable
void setSensorAssigned()
{
	bSensorAssigned = true;
}

/* Initialise the display system */
bool dispInitialise()
{
	flagReposVarsValid = false;
	gInputManager.contexts().resetStates();
	return true;
}
