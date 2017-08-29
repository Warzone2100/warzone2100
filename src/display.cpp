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
#include "keymap.h"
#include "intimage.h"
#include "mechanics.h"
#include "lighting.h"
#include "ingameop.h"
#include "oprint.h"
#include "warcam.h"
#include "keybind.h"
#include "keymap.h"
#include "projectile.h"
#include "message.h"
#include "effects.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptextern.h"
#include "scriptcb.h"
#include "cmddroid.h"
#include "selection.h"
#include "transporter.h"
#include "intorder.h"
#include "multiplay.h"
#include "qtscript.h"
#include "warzoneconfig.h"
#include "lib/ivis_opengl/piematrix.h"

struct	_dragBox dragBox3D, wallDrag;

#define POSSIBLE_SELECTIONS		14
#define POSSIBLE_TARGETS		23

extern char DROIDDOING[512];		// holds the string on what the droid is doing

// NOTE: the external file "cursorselection" is used, so you can import that into a spreadsheet, and edit it there, much easier.
static const CURSOR arnMPointers[POSSIBLE_TARGETS][POSSIBLE_SELECTIONS] =
{
#include "cursorselection"
};

/// acceleration on scrolling. Game Option.
UDWORD	scroll_speed_accel;

/// Control zoom. Add an amount to zoom this much each second.
static float zoom_speed = 0.0f;
static float zoom_target = 0.0f;

static bool	buildingDamaged(STRUCTURE *psStructure);
static bool	repairDroidSelected(UDWORD player);
static bool vtolDroidSelected(UDWORD player);
static bool	anyDroidSelected(UDWORD player);
static bool cyborgDroidSelected(UDWORD player);
static bool bInvertMouse = true;
static bool bRightClickOrders = false;
static bool bMiddleClickRotate = false;
static bool bDrawShadows = true;
static SELECTION_TYPE	establishSelection(UDWORD selectedPlayer);
static void	dealWithLMB();
static void	dealWithLMBDClick();
static void	dealWithRMB();
static bool	mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
static OBJECT_POSITION *checkMouseLoc();

void finishDeliveryPosition();

static bool	bInstantRadarJump = false;
static SDWORD	desiredPitch = 340;
static UDWORD	currentFrame;
static UDWORD StartOfLastFrame;
static SDWORD	rotX;
static SDWORD	rotY;
static UDWORD	rotInitial;
static UDWORD	rotInitialUp;
static UDWORD	xMoved, yMoved;
static uint32_t scrollRefTime;
static float	scrollSpeedLeftRight; //use two directions and add them because its simple
static float	scrollStepLeftRight;
static float	scrollSpeedUpDown;
static float	scrollStepUpDown;
static bool	mouseOverRadar = false;
static bool	mouseOverConsole = false;
static bool	ignoreOrder = false;
static bool	ignoreRMBC	= true;
static DROID	*psSelectedVtol;
static DROID	*psDominantSelected;
static bool bRadarDragging = false;
static bool mouseScroll = true;
static UDWORD CurrentItemUnderMouse = 0;

bool	rotActive = false;
bool	gameStats = false;

//used to determine is a weapon droid is assigned to a sensor tower or sensor droid
static bool bSensorAssigned;
//used to determine if the player has selected a Las Sat structure
static bool bLasSatStruct;
// Local prototypes
static MOUSE_TARGET	itemUnderMouse(BASE_OBJECT **ppObjUnderCursor);

float getZoom()
{
	return zoom_target;
}

float getZoomSpeed()
{
	return fabsf(zoom_speed);
}

void setZoom(float zoomSpeed, float zoomTarget)
{
	float zoom_origin = getViewDistance();
	zoom_speed = zoomSpeed;
	zoom_target = zoomTarget;
	zoom_speed *= zoom_target > zoom_origin ? 1 : -1; // get direction
}

void zoom()
{
	if (zoom_speed != 0.0f)
	{
		float distance = getViewDistance();
		distance += graphicsTimeAdjustedIncrement(zoom_speed);
		if ((zoom_speed > 0.0f && distance > zoom_target) || (zoom_speed <= 0.0f && distance < zoom_target))
		{
			distance = zoom_target; // reached target
			zoom_speed = 0.0f;
		}
		setViewDistance(distance);
		UpdateFogDistance(distance);
	}
}

bool isMouseOverRadar()
{
	return mouseOverRadar;
}

void setMouseScroll(bool scroll)
{
	mouseScroll = scroll;
}

void	setRadarJump(bool	val)
{
	bInstantRadarJump = val;
}

bool	getRadarJumpStatus()
{
	return (bInstantRadarJump);
}

bool	getInvertMouseStatus()
{
	return (bInvertMouse);
}

void	setInvertMouseStatus(bool val)
{
	bInvertMouse = val;
}


#define MOUSE_ORDER (bRightClickOrders?MOUSE_RMB:MOUSE_LMB)
#define MOUSE_SELECT (bRightClickOrders?MOUSE_LMB:MOUSE_RMB)
#define MOUSE_ROTATE (bMiddleClickRotate?MOUSE_MMB:MOUSE_RMB)

bool	getRightClickOrders()
{
	return bRightClickOrders;
}

void	setRightClickOrders(bool val)
{
	bRightClickOrders = val;
}

bool	getMiddleClickRotate()
{
	return bMiddleClickRotate;
}

void	setMiddleClickRotate(bool val)
{
	bMiddleClickRotate = val;
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

void ProcessRadarInput()
{
	int PosX, PosY;
	int x = mouseX();
	int y = mouseY();
	UDWORD	temp1, temp2;

	/* Only allow jump-to-area-of-map if radar is on-screen */
	mouseOverRadar = false;
	if (radarOnScreen && radarPermitted)
	{
		if (CoordInRadar(x, y))
		{
			mouseOverRadar = true;

			if (mousePressed(MOUSE_ORDER) || (mousePressed(MOUSE_MMB) && keyDown(KEY_LALT)))
			{
				if (mousePressed(MOUSE_ORDER))
				{
					x = mousePressPos_DEPRECATED(MOUSE_ORDER).x;
					y = mousePressPos_DEPRECATED(MOUSE_ORDER).y;
				}
				else
				{
					x = mousePressPos_DEPRECATED(MOUSE_MMB).x;
					y = mousePressPos_DEPRECATED(MOUSE_MMB).y;
				}

				/* If we're tracking a droid, then cancel that */
				CalcRadarPosition(x, y, &PosX, &PosY);
				if (mouseOverRadar)
				{
					// MARKER
					// Send all droids to that location
					orderSelectedLoc(selectedPlayer, (PosX * TILE_UNITS) + TILE_UNITS / 2,
					                 (PosY * TILE_UNITS) + TILE_UNITS / 2, ctrlShiftDown()); // ctrlShiftDown() = ctrl clicked a destination, add an order

				}
				CheckScrollLimits();
				audio_PlayTrack(ID_SOUND_MESSAGEEND);
			}


			if (mouseDrag(MOUSE_SELECT, &temp1, &temp2) && !rotActive)
			{
				CalcRadarPosition(x, y, &PosX, &PosY);
				setViewPos(PosX, PosY, true);
				bRadarDragging = true;
				if (ctrlShiftDown())
				{
					player.r.y = 0;
				}
			}
			else if (mousePressed(MOUSE_SELECT))
			{
				x = mousePressPos_DEPRECATED(MOUSE_SELECT).x;
				y = mousePressPos_DEPRECATED(MOUSE_SELECT).y;

				CalcRadarPosition(x, y, &PosX, &PosY);

				if (bInstantRadarJump)
				{
					/* Go instantly */
					setViewPos(PosX, PosY, true);
				}
				else
				{
					/* Pan to it */
					requestRadarTrack(PosX * TILE_UNITS, PosY * TILE_UNITS);
				}
			}
			// ctrl-alt-scroll changes game speed
			if (!keyDown(KEY_LCTRL) || !keyDown(KEY_LALT))
			{
				if (mousePressed(MOUSE_WUP))
				{
					kf_RadarZoomIn();
				}
				else if (mousePressed(MOUSE_WDN))
				{
					kf_RadarZoomOut();
				}
			}
		}
	}
}

// reset the input state
void resetInput()
{
	rotActive = false;
	dragBox3D.status = DRAG_INACTIVE;
	wallDrag.status = DRAG_INACTIVE;
}

/* Process the user input. This just processes the key input and jumping around the radar*/
void processInput()
{
	bool mOverConstruction = false;

	if (InGameOpUp || isInGamePopupUp)
	{
		dragBox3D.status = DRAG_RELEASED;	// disengage the dragging since it stops menu input
	}

	if (CoordInBuild(mouseX(), mouseY()))
	{
		mOverConstruction = true;
	}

	StartOfLastFrame = currentFrame;
	currentFrame = frameGetFrameNumber();

	ignoreRMBC = false;

	/* Process all of our key mappings */
	mouseOverConsole = mouseOverHistoryConsoleBox();
	if (mousePressed(MOUSE_WUP) && !isMouseOverRadar())
	{
		/* Ctrl+Alt+WheelUp makes game speed up */
		if (keyDown(KEY_LCTRL) && keyDown(KEY_LALT))
		{
			kf_SpeedUp();
		}
		else if (mOverConstruction)
		{
			kf_BuildPrevPage();
		}
		else if (!mouseOverConsole)
		{
			kf_ZoomInStep();
		}
	}

	if (mousePressed(MOUSE_WDN) && !isMouseOverRadar())
	{
		/* Ctrl+Alt+WheelDown makes game slow down */
		if (keyDown(KEY_LCTRL) && keyDown(KEY_LALT))
		{
			kf_SlowDown();
		}
		else if (mOverConstruction)
		{
			kf_BuildNextPage();
		}
		else if (!mouseOverConsole)
		{
			kf_ZoomOutStep();
		}
	}

	if (intMode == INT_DESIGN)
	{
		/* Only process the function keys */
		keyProcessMappings(true);
	}
	else if (bAllowOtherKeyPresses)
	{
		/* Run all standard mappings */
		keyProcessMappings(false);
	}
	/* Allow the user to clear the (Active) console if need be */
	if (mouseOverConsoleBox() && mousePressed(MOUSE_LMB))
	{
		clearActiveConsole();
	}
}

static bool OverRadarAndNotDragging()
{
	return mouseOverRadar && dragBox3D.status != DRAG_DRAGGING && radarPermitted && wallDrag.status != DRAG_DRAGGING;
}

static void CheckFinishedDrag()
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
				    && sBuildDetails.psStats->ref >= REF_STRUCTURE_START
				    && sBuildDetails.psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
				{
					if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
					     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE
					     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
					     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD)
					    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
					{
						wallDrag.x2 = mouseTileX;
						wallDrag.y2 = mouseTileY;
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
		}
		else
		{
			dragBox3D.status = DRAG_INACTIVE;
			wallDrag.status = DRAG_INACTIVE;
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
		    && sBuildDetails.psStats->ref >= REF_STRUCTURE_START
		    && sBuildDetails.psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
		{
			if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD)
			    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
			{
				wallDrag.x1 = wallDrag.x2 = mouseTileX;
				wallDrag.y1 = wallDrag.y2 = mouseTileY;
				wallDrag.status = DRAG_PLACING;
				debug(LOG_NEVER, "Start Wall Drag\n");
			}
		}
		else if (intBuildSelectMode())//if we were in build select mode
		{
			//uhoh no place to build here
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
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
			if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE)
			    && sBuildDetails.psStats->ref >= REF_STRUCTURE_START
			    && sBuildDetails.psStats->ref < (REF_STRUCTURE_START + REF_RANGE)
			    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
			{
				int dx, dy;

				wallDrag.x2 = mouseTileX;
				wallDrag.y2 = mouseTileY;

				dx = abs(mouseTileX - wallDrag.x1);
				dy = abs(mouseTileY - wallDrag.y1);

				if (dx >= dy)
				{
					wallDrag.y2 = wallDrag.y1;
				}
				else if (dx < dy)
				{
					wallDrag.x2 = wallDrag.x1;
				}

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

	if (mouseDrag(MOUSE_LMB, &dragX, &dragY) && !mouseOverRadar && !mouseDown(MOUSE_RMB))
	{
		dragBox3D.x1 = dragX;
		dragBox3D.x2 = mouseX();
		dragBox3D.y1 = dragY;
		dragBox3D.y2 = mouseY();
		dragBox3D.status = DRAG_DRAGGING;

		if (buildState == BUILD3D_VALID)
		{
			if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD)
			    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
			{
				int dx, dy;

				wallDrag.x2 = mouseTileX;
				wallDrag.y2 = mouseTileY;

				dx = abs(mouseTileX - wallDrag.x1);
				dy = abs(mouseTileY - wallDrag.y1);

				if (dx >= dy)
				{
					wallDrag.y2 = wallDrag.y1;
				}
				else if (dx < dy)
				{
					wallDrag.x2 = wallDrag.x1;
				}

				wallDrag.status = DRAG_DRAGGING;
			}
		}
	}
}

UDWORD getTargetType()
{
	return CurrentItemUnderMouse;
}

//don't want to do any of these whilst in the Intelligence Screen
CURSOR processMouseClickInput()
{
	UDWORD	i;
	SELECTION_TYPE	selection;
	MOUSE_TARGET	item = MT_NOTARGET;
	bool OverRadar = OverRadarAndNotDragging();

	CURSOR cursor = CURSOR_DEFAULT;

	ignoreOrder = CheckFinishedFindPosition();

	CheckStartWallDrag();

	HandleDrag();

	CheckFinishedDrag();

	if ((mouseReleased(MOUSE_LMB) || (mouseReleased(MOUSE_MMB) && (keyDown(KEY_LALT) || keyDown(KEY_RALT)))) && !OverRadar &&
	    dragBox3D.status != DRAG_RELEASED && !ignoreOrder && !mouseOverConsole && !bDisplayMultiJoiningStatus)
	{
		if (bRightClickOrders)
		{
			dealWithRMB();
		}
		else
		{
			if (!bMultiPlayer  && (establishSelection(selectedPlayer) == SC_DROID_TRANSPORTER || establishSelection(selectedPlayer) == SC_DROID_SUPERTRANSPORTER))
			{
				// Never, *ever* let user control the transport in SP games--it breaks the scripts!
				ASSERT(game.type == CAMPAIGN, "Game type was set incorrectly!");
				return cursor;
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

	if (mouseReleased(MOUSE_RMB) && !rotActive && !ignoreRMBC)
	{
		dragBox3D.status = DRAG_INACTIVE;
		// Pretty sure we wan't set walldrag status here aswell.
		wallDrag.status = DRAG_INACTIVE;
		bRadarDragging = false;
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

	if (!mouseDrag(MOUSE_SELECT, (UDWORD *)&rotX, (UDWORD *)&rotY) && bRadarDragging)
	{
		bRadarDragging = false;
	}

	/* Right mouse click kills a building placement */
	if (mouseReleased(MOUSE_RMB) &&
	    (buildState == BUILD3D_POS || buildState == BUILD3D_VALID))
	{
		/* Stop the placement */
		kill3DBuilding();
		bRadarDragging = false;
	}
	if (mouseReleased(MOUSE_RMB))
	{
		cancelDeliveryRepos();
	}
	if (mouseDrag(MOUSE_ROTATE, (UDWORD *)&rotX, (UDWORD *)&rotY) && !rotActive && !bRadarDragging)
	{
		rotInitial = player.r.y;
		rotInitialUp = player.r.x;
		xMoved = 0;
		yMoved = 0;
		rotActive = true;
	}

	selection = establishSelection(selectedPlayer);
	ASSERT(selection <= POSSIBLE_SELECTIONS, "Weirdy selection!");

	if (gamePaused())
	{
		cursor = CURSOR_DEFAULT;
	}
	if (buildState == BUILD3D_VALID)
	{
		// special casing for building
		cursor = CURSOR_BUILD;
	}
	else if (buildState == BUILD3D_POS)
	{
		// special casing for building - can't build here
		cursor = CURSOR_NOTPOSSIBLE;
	}
	else if (selection != SC_INVALID)
	{
		BASE_OBJECT *ObjUnderMouse;
		bool ObjAllied;

		item = itemUnderMouse(&ObjUnderMouse);
		ASSERT(item < POSSIBLE_TARGETS, "Weirdy target!");

		ObjAllied = (ObjUnderMouse && selectedPlayer != ObjUnderMouse->player && aiCheckAlliances(selectedPlayer, ObjUnderMouse->player));

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
				for (DROID *psCurr = apsDroidLists[selectedPlayer]; psCurr != nullptr; psCurr = psCurr->psNext)
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
				if (ObjAllied || (ObjUnderMouse && (ObjUnderMouse->type != OBJ_STRUCTURE || (((STRUCTURE *)ObjUnderMouse)->status == SS_BLUEPRINT_PLANNED))))
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
			    !droidIsDamaged((DROID *)ObjUnderMouse))
			{
				item = MT_OWNDROID;
			}
			if ((arnMPointers[item][selection] == CURSOR_SELECT ||
			     arnMPointers[item][selection] == CURSOR_EMBARK ||
			     arnMPointers[item][selection] == CURSOR_ATTACH ||
			     arnMPointers[item][selection] == CURSOR_LOCKON ||
			     arnMPointers[item][selection] == CURSOR_DEST) && ObjAllied)
			{
				// If you want to do these things, just gift your unit to your ally.
				item = MT_BLOCKING;
			}

			if (specialOrderKeyDown() && (selection == SC_DROID_TRANSPORTER || selection == SC_DROID_SUPERTRANSPORTER) &&
			    arnMPointers[item][selection] == CURSOR_MOVE && bMultiPlayer)
			{
				// Alt+move = disembark transporter
				cursor = CURSOR_DISEMBARK;
			}
			else if (specialOrderKeyDown() && selection == SC_DROID_DIRECT &&
			         arnMPointers[item][selection] == CURSOR_MOVE)
			{
				// Alt+move = scout
				cursor = CURSOR_SCOUT;
			}
			else if (arnMPointers[item][selection] == CURSOR_NOTPOSSIBLE &&
			         ObjUnderMouse && (ObjUnderMouse->player == selectedPlayer) &&
			         ObjUnderMouse->type == OBJ_STRUCTURE && ((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat &&
			         (asWeaponStats[((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat].weaponSubClass == WSC_LAS_SAT))
			{
				cursor = CURSOR_SELECT; // Special casing for LasSat
			}
			else
			{
				cursor = arnMPointers[item][selection];
			}
		}
		else
		{
			cursor = CURSOR_DEFAULT;
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
				cursor = CURSOR_ATTACK;
			}
			else if (ObjUnderMouse && ObjUnderMouse->player == selectedPlayer && (ObjUnderMouse->type == OBJ_DROID ||
			         (ObjUnderMouse->type == OBJ_STRUCTURE && lasSatStructSelected((STRUCTURE *)ObjUnderMouse))))
			{
				// Special casing for selectables
				cursor = CURSOR_SELECT;
			}
			else if (ObjUnderMouse && ObjUnderMouse->player == selectedPlayer && ObjUnderMouse->type == OBJ_STRUCTURE)
			{
				cursor = CURSOR_DEFAULT;
			}
			else
			{
				//display block cursor
				cursor = CURSOR_NOTPOSSIBLE;
			}
		}
		else if (ObjUnderMouse && (ObjUnderMouse->player == selectedPlayer) &&
		         ((ObjUnderMouse->type == OBJ_STRUCTURE && ((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat
		           && (asWeaponStats[((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat].weaponSubClass == WSC_LAS_SAT))
		          || ObjUnderMouse->type == OBJ_DROID))
		{
			cursor = CURSOR_SELECT; // Special casing for LasSat or own unit
		}
		else
		{
			// when one of the arrow key gets pressed, set cursor appropriately
			if (keyDown(KEY_UPARROW))
			{
				cursor = CURSOR_UARROW;
			}
			else if (keyDown(KEY_DOWNARROW))
			{
				cursor = CURSOR_DARROW;
			}
			else if (keyDown(KEY_LEFTARROW))
			{
				cursor = CURSOR_LARROW;
			}
			else if (keyDown(KEY_RIGHTARROW))
			{
				cursor = CURSOR_RARROW;
			}
		}
	}

	CurrentItemUnderMouse = item;
	return cursor;
}

static void calcScroll(float *y, float *dydt, float accel, float decel, float targetVelocity, float dt)
{
	double tMid;

	// Stop instantly, if trying to change direction.
	if (targetVelocity * *dydt < -1e-8f)
	{
		*dydt = 0;
	}

	if (targetVelocity < *dydt)
	{
		accel = -accel;
		decel = -decel;
	}

	// Decelerate if needed.
	tMid = (0 - *dydt) / decel;
	CLIP(tMid, 0, dt);
	*y += *dydt * tMid + decel / 2 * tMid * tMid;
	*dydt += decel * tMid;
	dt -= tMid;

	// Accelerate if needed.
	tMid = (targetVelocity - *dydt) / accel;
	CLIP(tMid, 0, dt);
	*y += *dydt * tMid + accel / 2 * tMid * tMid;
	*dydt += accel * tMid;
	dt -= tMid;

	// Continue at target velocity.
	*y += *dydt * dt;
}

CURSOR scroll()
{
	SDWORD	xDif, yDif;
	uint32_t timeDiff;
	int scrollDirLeftRight = 0, scrollDirUpDown = 0;
	float scroll_zoom_factor = 1 + 2 * ((getViewDistance() - MINDISTANCE) / ((float)(MAXDISTANCE - MINDISTANCE)));
	float scaled_max_scroll_speed = scroll_zoom_factor * MAX_SCROLL_SPEED;
	float scaled_accel;

	static float xDiffFrac = 0, yDiffFrac = 0;

	CURSOR cursor = CURSOR_DEFAULT;

	if (InGameOpUp || bDisplayMultiJoiningStatus || isInGamePopupUp)		// cant scroll when menu up. or when over radar
	{
		return cursor;
	}

	if (mouseScroll && wzMouseInWindow())
	{
		// Scroll left or right
		scrollDirLeftRight += (mouseX() > (pie_GetVideoBufferWidth() - BOUNDARY_X)) - (mouseX() < BOUNDARY_X);

		// Scroll down or up
		scrollDirUpDown += (mouseY() < BOUNDARY_Y) - (mouseY() > (pie_GetVideoBufferHeight() - BOUNDARY_Y));
		// when mouse cursor goes to an edge, set cursor appropriately
		if (scrollDirUpDown > 0)
		{
			cursor = CURSOR_UARROW;
		}
		else if (scrollDirUpDown < 0)
		{
			cursor = CURSOR_DARROW;
		}
		else if (scrollDirLeftRight < 0)
		{
			cursor = CURSOR_LARROW;
		}
		else if (scrollDirLeftRight > 0)
		{
			cursor = CURSOR_RARROW;
		}
	}
	if (!keyDown(KEY_LCTRL) && !keyDown(KEY_RCTRL))
	{
		// Scroll left or right
		scrollDirLeftRight += keyDown(KEY_RIGHTARROW) - keyDown(KEY_LEFTARROW);

		// Scroll down or up
		scrollDirUpDown += keyDown(KEY_UPARROW) - keyDown(KEY_DOWNARROW);

	}
	CLIP(scrollDirLeftRight, -1, 1);
	CLIP(scrollDirUpDown,    -1, 1);

	if (scrollDirLeftRight != 0 || scrollDirUpDown != 0)
	{
		setWarCamActive(false);  // Don't let this thing override the user trying to scroll.
	}

	scaled_accel = scroll_zoom_factor * scroll_speed_accel;

	// Apparently there's stutter if using deltaRealTime, so we have our very own delta time here, just for us.
	timeDiff = wzGetTicks() - scrollRefTime;
	scrollRefTime += timeDiff;
	timeDiff = std::min<unsigned>(timeDiff, 500);  // Since we're using our own time variable, which isn't updated when dragging a box, clamp the time here so releasing the box doesn't scroll to the edge of the map suddenly.

	scrollStepLeftRight = 0;
	scrollStepUpDown = 0;
	calcScroll(&scrollStepLeftRight, &scrollSpeedLeftRight, scaled_accel, 2 * scaled_accel, scrollDirLeftRight * scaled_max_scroll_speed, (float)timeDiff / GAME_TICKS_PER_SEC);
	calcScroll(&scrollStepUpDown,    &scrollSpeedUpDown,    scaled_accel, 2 * scaled_accel, scrollDirUpDown    * scaled_max_scroll_speed, (float)timeDiff / GAME_TICKS_PER_SEC);

	/* Get x component of movement */
	xDiffFrac += cos(-player.r.y * (M_PI / 32768)) * scrollStepLeftRight + sin(-player.r.y * (M_PI / 32768)) * scrollStepUpDown;
	/* Get y component of movement */
	yDiffFrac += sin(-player.r.y * (M_PI / 32768)) * scrollStepLeftRight - cos(-player.r.y * (M_PI / 32768)) * scrollStepUpDown;

	xDif = (int)xDiffFrac;
	yDif = (int)yDiffFrac;

	xDiffFrac -= xDif;
	yDiffFrac -= yDif;

	/* Adjust player's position by these components */
	player.p.x += xDif;
	player.p.z += yDif;

	CheckScrollLimits();

	return cursor;
}

/*
 * Reset scrolling, so we don't jump around after unpausing.
 */
void resetScroll()
{
	scrollRefTime = wzGetTicks();
	scrollSpeedUpDown = 0.0f;
	scrollSpeedLeftRight = 0.0f;
}

// Check a coordinate is within the scroll limits, SDWORD version.
// Returns true if edge hit.
//
bool CheckInScrollLimits(SDWORD *xPos, SDWORD *zPos)
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
	SDWORD xp = player.p.x;
	SDWORD zp = player.p.z;
	bool ret = CheckInScrollLimits(&xp, &zp);

	player.p.x = xp;
	player.p.z = zp;

	return ret;
}

/* Do the 3D display */
void displayWorld()
{
	Vector3i pos;

	if (mouseDown(MOUSE_ROTATE) && rotActive)
	{
		if (abs(mouseX() - rotX) > 2 || xMoved > 2 || abs(mouseY() - rotY) > 2 || yMoved > 2)
		{
			xMoved += abs(mouseX() - rotX);
			if (mouseX() < rotX)
			{
				player.r.y = rotInitial + (rotX - mouseX()) * DEG(1) / 2;
			}
			else
			{
				player.r.y = rotInitial - (mouseX() - rotX) * DEG(1) / 2;
			}
			yMoved += abs(mouseY() - rotY);
			if (bInvertMouse)
			{
				if (mouseY() < rotY)
				{
					player.r.x = rotInitialUp + (rotY - mouseY()) * DEG(1) / 3;
				}
				else
				{
					player.r.x = rotInitialUp - (mouseY() - rotY) * DEG(1) / 3;
				}
			}
			else
			{
				if (mouseY() < rotY)
				{
					player.r.x = rotInitialUp - (rotY - mouseY()) * DEG(1) / 3;
				}
				else
				{
					player.r.x = rotInitialUp + (mouseY() - rotY) * DEG(1) / 3;
				}
			}
			if (player.r.x > DEG(360 + MAX_PLAYER_X_ANGLE))
			{
				player.r.x = DEG(360 + MAX_PLAYER_X_ANGLE);
			}
			if (player.r.x < DEG(360 + MIN_PLAYER_X_ANGLE))
			{
				player.r.x = DEG(360 + MIN_PLAYER_X_ANGLE);
			}

			setDesiredPitch(player.r.x / DEG_1);
		}
	}

	if (!mouseDown(MOUSE_ROTATE) && rotActive)
	{
		rotActive = false;
		xMoved = yMoved = 0;
		ignoreRMBC = true;
		pos.x = player.r.x;
		pos.y = player.r.y;
		pos.z = player.r.z;
		camInformOfRotation(&pos);
		bRadarDragging = false;
	}

	draw3DScene();
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
		/* Note the !psObject check isn't really necessary as the goto will jump out */
		for (DROID *psDroid = apsDroidLists[i]; psDroid && !psReturn; psDroid = psDroid->psNext)
		{
			dispX = psDroid->sDisplay.screenX;
			dispY = psDroid->sDisplay.screenY;
			dispR = psDroid->sDisplay.screenR;

			// Has the droid been drawn since the start of the last frame
			if (psDroid->visible[selectedPlayer] && DrawnInLastFrame(psDroid->sDisplay.frameNumber))
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
	FLAG_POSITION	*psFlagPos;

	if (tryingToGetLocation()) // if we're placing a building don't place
	{
		return;
	}

	//clear the selected delivery point
	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos; psFlagPos = psFlagPos->psNext)
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

	if (bInTutorial)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DELIVPOINTMOVED);
	}
}

// Finished repositioning a delivery point.
//
void finishDeliveryPosition()
{
	FLAG_POSITION *psFlagPos;
	if (flagStructId)
	{
		flagReposVarsValid = false;
		STRUCTURE *psStruct = IdToStruct(flagStructId, selectedPlayer);
		if (StructIsFactory(psStruct) && psStruct->pFunctionality
		    && psStruct->pFunctionality->factory.psAssemblyPoint)
		{
			setAssemblyPoint(psStruct->pFunctionality->factory.psAssemblyPoint,
			                 flagPos.coords.x, flagPos.coords.y, selectedPlayer, true);
		}
		else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY && psStruct->pFunctionality != nullptr)
		{
			setAssemblyPoint(psStruct->pFunctionality->repairFacility.psDeliveryPoint,
			                 flagPos.coords.x, flagPos.coords.y, selectedPlayer, true);
		}
		//deselect once moved
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos; psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = false;
		}
	}
	flagReposFinished = true;
}

// Is there a valid delivery point repositioning going on.
bool deliveryReposValid()
{
	if (!flagReposVarsValid)
	{
		return false;
	}

	Vector2i map = map_coord(flagPos.coords.xy);

	//make sure we are not too near map edge
	if (map.x < scrollMinX + TOO_NEAR_EDGE || map.x + 1 > scrollMaxX - TOO_NEAR_EDGE ||
	    map.y < scrollMinY + TOO_NEAR_EDGE || map.y + 1 > scrollMaxY - TOO_NEAR_EDGE)
	{
		return false;
	}

	// cant place on top of a delivery point...
	for (FLAG_POSITION const *psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
	{
		Vector2i flagTile = map_coord(psCurrFlag->coords.xy);
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

	int bX = clip(mouseTileX, 2, mapWidth - 3);
	int bY = clip(mouseTileY, 2, mapHeight - 3);

	flagPos.coords = Vector3i(world_coord(Vector2i(bX, bY)) + Vector2i(TILE_UNITS / 2, TILE_UNITS / 2), map_TileHeight(bX, bY) + 2 * ASSEMBLY_POINT_Z_PADDING);
}

// Cancel repositioning of the delivery point without moving it.
//
void cancelDeliveryRepos()
{
	flagReposVarsValid = false;
}

void renderDeliveryRepos(const glm::mat4 &viewMatrix)
{
	if (flagReposVarsValid)
	{
		renderDeliveryPoint(&flagPos, true, viewMatrix);
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
	if (!bDragBox && psDroid->selected == true)
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
		psCBSelectedDroid = psDroid;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_SELECTED);
		psCBSelectedDroid = nullptr;
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
	addConsoleMessage(_("Cannot Build. Oil Resource Burning."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	audio_PlayTrack(ID_SOUND_BUILD_FAIL);
}

static void printDroidClickInfo(DROID *psDroid)
{
	if (getDebugMappingStatus()) // cheating on, so output debug info
	{
		console("%s - Hitpoints %d/%d - ID %d - experience %f, %s - order %s - action %s - sensor range %hu - ECM %u - pitch %.0f - frust %u",
		        droidGetName(psDroid), psDroid->body, psDroid->originalBody, psDroid->id,
		        psDroid->experience / 65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order.type), getDroidActionName(psDroid->action),
		        droidSensorRange(psDroid), droidJammerPower(psDroid), UNDEG(psDroid->rot.pitch), psDroid->lastFrustratedTime);
		FeedbackOrderGiven();
	}
	else if (!psDroid->selected)
	{
		console(_("%s - Hitpoints %d/%d - Experience %.1f, %s"), droidGetName(psDroid), psDroid->body, psDroid->originalBody,
		        psDroid->experience / 65536.f, _(getDroidLevelName(psDroid)));
		FeedbackOrderGiven();
	}
	clearSelection();
	dealWithDroidSelect(psDroid, false);
}

static void dealWithLMBDroid(DROID *psDroid, SELECTION_TYPE selection)
{
	bool ownDroid; // Not an allied droid

	if (!aiCheckAlliances(selectedPlayer, psDroid->player))
	{
		memset(DROIDDOING, 0x0 , sizeof(DROIDDOING)); // take over the other players droid by debug menu.
		/* We've clicked on enemy droid */
		if (getDebugMappingStatus())
		{
			console("(Enemy!) %s - Hitpoints %d/%d - ID %d - experience %f, %s - order %s - action %s - sensor range %d - ECM %d - pitch %.0f",
			        droidGetName(psDroid),  psDroid->body, psDroid->originalBody, psDroid->id,
			        psDroid->experience / 65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order.type),
			        getDroidActionName(psDroid->action), droidSensorRange(psDroid), droidJammerPower(psDroid), UNDEG(psDroid->rot.pitch));
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
	else if (isTransporter(psDroid))
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
		DROID *psCurr;

		bSensorAssigned = false;
		for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			//must be indirect weapon droid or VTOL weapon droid
			if ((psCurr->droidType == DROID_WEAPON) &&
			    (psCurr->selected) &&
			    (psCurr->asWeaps[0].nStat > 0) &&
			    ((!proj_Direct(asWeaponStats + psCurr->asWeaps[0].nStat)) ||
			     isVtolDroid(psCurr)) &&
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
	else if (droidIsDamaged(psDroid) && repairDroidSelected(selectedPlayer))
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

	if (!aiCheckAlliances(psStructure->player, selectedPlayer))
	{
		/* We've clicked on an enemy building */
		if (getDebugMappingStatus())
		{
			console("(Enemy!) %s, ref: %d, ID: %d Hitpoints: %d/%d", getID(psStructure->pStructureType), psStructure->pStructureType->ref,
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

	/* Got to be built. Also, you can't 'select' derricks */
	if (!specialOrderKeyDown() && (psStructure->status == SS_BUILT) &&
	    (psStructure->pStructureType->type != REF_RESOURCE_EXTRACTOR) && ownStruct)
	{
		if (bRightClickOrders)
		{
			if (StructIsFactory(psStructure) && selection != SC_DROID_CONSTRUCT)
			{
				intAddFactoryOrder(psStructure);
			}
		}
		else
		{
			// now only display interface if nothing selected
			if (!anyDroidSelected(selectedPlayer))
			{
				intObjectSelected((BASE_OBJECT *)psStructure);
				FeedbackOrderGiven();
			}
			if (selection == SC_INVALID)
			{
				STRUCTURE *psCurr;

				/* Clear old building selection(s) - should only be one */
				for (psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
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
		}

	}
	else if ((psStructure->status == SS_BUILT) &&
	         (psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR) &&
	         selection == SC_INVALID && ownStruct)
	{
		STRUCTURE *psCurr;

		/* Clear old building selection(s) - should only be one */
		for (psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
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
			DROID *psCurr;

			// for each droid
			for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
			{
				if ((droidType(psCurr) == DROID_CONSTRUCT ||
				     droidType(psCurr) == DROID_CYBORG_CONSTRUCT) && (psCurr->selected))
				{
					if (fireOnLocation(psFeature->pos.x, psFeature->pos.y))
					{
						// Can't build because it's burning
						AddDerrickBurningMessage();
					}

					sendDroidInfo(psCurr, DroidOrder(DORDER_BUILD, &asStructureStats[i], psFeature->pos.xy, player.r.y), ctrlShiftDown());
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
	if (getDebugMappingStatus())
	{
		console("(Feature) %s ID: %d ref: %d Hipoints: %d/%d", getID(psFeature->psStats), psFeature->id, psFeature->psStats->ref, psFeature->psStats->body, psFeature->body);
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
	OBJECT_POSITION     *psLocation;
	STRUCTURE			*psStructure;

	/* Don't process if in game options are on screen */
	if (mouseOverRadar ||
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

	/*Check for a Delivery Point or a Proximity Message*/
	psLocation = checkMouseLoc();
	if (psLocation == nullptr || selNumSelected(selectedPlayer))
	{
		// now changed to use the multiple order stuff
		// clicked on a destination.
		orderSelectedLoc(selectedPlayer, mousePos.x, mousePos.y, ctrlShiftDown());  // ctrlShiftDown() = ctrl clicked a destination, add an order
		/* Otherwise send them all */
		if (getNumDroidsSelected())
		{
			assignDestTarget();
			audio_PlayTrack(ID_SOUND_SELECT);
		}

		if (getDebugMappingStatus() && tileOnMap(mouseTileX, mouseTileY))
		{
			MAPTILE *psTile = mapTile(mouseTileX, mouseTileY);
			uint8_t aux = auxTile(mouseTileX, mouseTileY, selectedPlayer);

			console("%s tile %d, %d [%d, %d] continent(l%d, h%d) level %g illum %d %s %s w=%d s=%d j=%d",
			        tileIsExplored(psTile) ? "Explored" : "Unexplored",
			        mouseTileX, mouseTileY, world_coord(mouseTileX), world_coord(mouseTileY),
			        (int)psTile->limitedContinent, (int)psTile->hoverContinent, psTile->level, (int)psTile->illumination,
			        aux & AUXBITS_DANGER ? "danger" : "", aux & AUXBITS_THREAT ? "threat" : "",
			        (int)psTile->watchers[selectedPlayer], (int)psTile->sensors[selectedPlayer], (int)psTile->jammers[selectedPlayer]);
		}

		return;
	}

	switch (psLocation->type)
	{
	case POS_DELIVERY:
		if (psLocation->player == selectedPlayer)
		{
			if (bRightClickOrders)
			{
				//centre the view on the owning Factory
				psStructure = findDeliveryFactory((FLAG_POSITION *)psLocation);
				if (psStructure)
				{
					setViewPos(map_coord(psStructure->pos.x),
					           map_coord(psStructure->pos.y),
					           true);
				}
			}
			else
			{
				startDeliveryPosition((FLAG_POSITION *)psLocation);
			}
		}
		break;
	default:
		ASSERT(!"unknown object position type", "Unknown type from checkMouseLoc");
	}
}

bool	getRotActive()
{
	return (rotActive);
}

SDWORD	getDesiredPitch()
{
	return (desiredPitch);
}

void	setDesiredPitch(SDWORD pitch)
{
	desiredPitch = pitch;
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
				/* If we've double clicked on a constructor droid, activate build menu */
				if (psDroid->droidType == DROID_COMMAND)
				{
					intResetScreen(true);
					intCommanderSelected(psDroid);
				}
				else
				{
					// Now selects all of same type on screen
					selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_ALL_SAME, true);
				}
			}
		}
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
			/* We clicked on structure */
			psStructure = (STRUCTURE *) psClickedOn;
			if (psStructure->player == selectedPlayer && !structureIsBlueprint(psStructure))
			{
				if (StructIsFactory(psStructure))
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

/*This checks to see if the mouse was over a delivery point or a proximity message
when the mouse button was pressed */
static OBJECT_POSITION 	*checkMouseLoc()
{
	FLAG_POSITION		*psPoint;
	UDWORD				i;
	UDWORD				dispX, dispY, dispR;

	// First have a look through the DeliveryPoint lists
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		//new way, handles multiple points.
		for (psPoint = apsFlagPosLists[i]; psPoint; psPoint = psPoint->psNext)
		{
			dispX = psPoint->screenX;
			dispY = psPoint->screenY;
			dispR = psPoint->screenR;
			if (DrawnInLastFrame(psPoint->frameNumber) == true)	// Only check DP's that are on screen
			{
				if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
				{
					// We HAVE clicked on DP!
					return psPoint;
				}
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

	if (mouseOverRadar || InGameOpUp == true || widgGetFromID(psWScreen, INTINGAMEOP))
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
				else if (!isTransporter(psDroid))
				{
					if (bRightClickOrders)
					{
						/* We've clicked on one of our own droids */
						printDroidClickInfo(psDroid);
					}
					else
					{
						if (psDroid->selected != true)
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
						if (bRightClickOrders && psDroid->selected != true)
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
				else if (psStructure->selected == true)
				{
					psStructure->selected = false;
					intObjectSelected(nullptr);
					triggerEventSelected();
					jsDebugSelected(psStructure);
				}
				else if (!structureIsBlueprint(psStructure))
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
					else if (StructIsFactory(psStructure))
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
		/*Check for a Delivery Point*/
		OBJECT_POSITION *psLocation = checkMouseLoc();

		if (psLocation)
		{
			switch (psLocation->type)
			{
			case POS_DELIVERY:
				if (psLocation->player == selectedPlayer)
				{
					if (bRightClickOrders)
					{
						startDeliveryPosition((FLAG_POSITION *)psLocation);
					}
					else
					{
						//centre the view on the owning Factory
						psStructure = findDeliveryFactory((FLAG_POSITION *)psLocation);
						if (psStructure)
						{
							setViewPos(map_coord(psStructure->pos.x),
							           map_coord(psStructure->pos.y),
							           true);
						}
					}
				}
				break;

			default:
				ASSERT(!"unknown object position type", "Unknown type from checkMouseLoc");
			}
		}
		else
		{
			clearSelection();
			intObjectSelected(nullptr);
			memset(DROIDDOING, 0x0 , sizeof(DROIDDOING));	// clear string when deselected
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
	DROID		*psDroid;
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
		/* Note the !psObject check isn't really necessary as the goto will jump out */
		for (psDroid = apsDroidLists[i]; psDroid && retVal == MT_NOTARGET;
		     psDroid = psDroid->psNext)
		{
			dispX = psDroid->sDisplay.screenX;
			dispY = psDroid->sDisplay.screenY;
			dispR = psDroid->sDisplay.screenR;
			/* Only check droids that're on screen */
			if (psDroid->sDisplay.frameNumber + 1 == currentFrame && psDroid->visible[selectedPlayer])
			{
				if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
				{
					/* We HAVE clicked on droid! */
					if (aiCheckAlliances(psDroid->player, selectedPlayer))
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
						else if (isTransporter(psDroid) &&
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
							if (droidIsDamaged(psDroid))
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

			if (aiCheckAlliances(psNotDroid->player, selectedPlayer))
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
	if (retVal == MT_NOTARGET || (psNotDroid && !(psNotDroid->visible[selectedPlayer])))
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
static SELECTION_TYPE	establishSelection(UDWORD selectedPlayer)
{
	DROID *psDominant = nullptr;
	UBYTE CurrWeight = UBYTE_MAX;
	SELECTION_TYPE selectionClass = SC_INVALID;

	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
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
			if (proj_Direct(asWeaponStats + psDominant->asWeaps[0].nStat))
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
	return psStructure->body < structureBody(psStructure);
}

/*Looks through the list of selected players droids to see if one is a repair droid*/
bool	repairDroidSelected(UDWORD player)
{
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
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
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
	{
		if (psCurr->selected && isVtolDroid(psCurr))
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
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
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
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
	{
		if (psCurr->selected && cyborgDroid(psCurr))
		{
			return true;
		}
	}

	//didn't find one...
	return false;
}

/* Clear the selection flag for a player */
void clearSel()
{
	DROID			*psCurrDroid;
	STRUCTURE		*psStruct;
	FLAG_POSITION	*psFlagPos;

	for (psCurrDroid = apsDroidLists[selectedPlayer]; psCurrDroid; psCurrDroid = psCurrDroid->psNext)
	{
		psCurrDroid->selected = false;
	}
	for (psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		psStruct->selected = false;
	}
	bLasSatStruct = false;
	//clear the Deliv Point if one
	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos; psFlagPos = psFlagPos->psNext)
	{
		psFlagPos->selected = false;
	}

	intRefreshScreen();

	triggerEventSelected();
}

void clearSelection()
{
	clearSel();
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
	return true;
}
