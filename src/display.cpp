/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
#include "lib/framework/fixedpoint.h"
#include "lib/framework/wzapp.h"

#include "action.h"
#include "display.h"
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
#include "lighting.h"
#include "lib/framework/math_ext.h"
#include "console.h"
#include "order.h"
#include "wrappers.h"
#include "power.h"
#include "map.h"
#include "keymap.h"
#include "intimage.h"
#include "mechanics.h"
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
#include "drive.h"
#include "cmddroid.h"
#include "selection.h"
#include "transporter.h"
#include "intorder.h"
#include "multiplay.h"
#include "warzoneconfig.h"

#define	SHAKE_TIME	(1500)

struct	_dragBox dragBox3D,wallDrag;

#define POSSIBLE_SELECTIONS		13
#define POSSIBLE_TARGETS		23

extern char DROIDDOING[512];		// holds the string on what the droid is doing

static const CURSOR arnMPointers[POSSIBLE_TARGETS][POSSIBLE_SELECTIONS] =
{
// empty terrain tile
{CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,
	CURSOR_BOMB,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE},
// resource tile
{CURSOR_BUILD,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,CURSOR_MOVE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE},
// impassible tile
{CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE},
// river tile
{CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_BRIDGE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE},
// trench tile
{CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_BRIDGE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE},
// damaged friendly structure
{CURSOR_FIX,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_MOVE,CURSOR_MOVE,
	CURSOR_GUARD,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_DEST,CURSOR_MOVE},
// undamaged friendly structure
{CURSOR_NOTPOSSIBLE,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_MOVE,CURSOR_MOVE,
	CURSOR_GUARD,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_DEST,CURSOR_MOVE},
// partially built friendly structure
{CURSOR_BUILD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_MOVE,CURSOR_MOVE,
	CURSOR_GUARD,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_DEST,CURSOR_MOVE},
// friendly repair facility
{CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,
	CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,
	CURSOR_SEEKREPAIR,CURSOR_DEST,CURSOR_SEEKREPAIR},
// friendly damagedrepair facility
{CURSOR_FIX,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,
	CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,CURSOR_SEEKREPAIR,
	CURSOR_SEEKREPAIR,CURSOR_DEST,CURSOR_SEEKREPAIR},
// enemy structure
{CURSOR_MOVE,CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_LOCKON,CURSOR_JAM,CURSOR_ATTACK,
	CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_BOMB,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_MOVE},
// transport
{CURSOR_EMBARK,CURSOR_EMBARK,CURSOR_EMBARK,CURSOR_EMBARK,CURSOR_EMBARK,CURSOR_EMBARK,CURSOR_EMBARK,
	CURSOR_EMBARK,CURSOR_EMBARK,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_EMBARK},
// friendly droid
{CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,
	CURSOR_SELECT,CURSOR_SELECT,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_SELECT},
// damaged friendly droid
{CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,
	CURSOR_PICKUP,CURSOR_SELECT,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_FIX},
// enemy droid
{CURSOR_MOVE,CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_LOCKON,CURSOR_JAM,CURSOR_MOVE,
	CURSOR_MOVE,CURSOR_ATTACK,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_MOVE},
// command droid
{CURSOR_SELECT,CURSOR_ATTACH,CURSOR_ATTACH,CURSOR_ATTACH,CURSOR_ATTACH,CURSOR_ATTACH,CURSOR_ATTACH,
	CURSOR_ATTACH,CURSOR_SELECT,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_ATTACH},
// artefact
{CURSOR_PICKUP,CURSOR_PICKUP,CURSOR_PICKUP,CURSOR_PICKUP,CURSOR_PICKUP,CURSOR_PICKUP,CURSOR_PICKUP,
	CURSOR_PICKUP,CURSOR_PICKUP,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_PICKUP,CURSOR_PICKUP},
// feature that can be damaged ie attacked
{CURSOR_NOTPOSSIBLE,CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_LOCKON,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_ATTACK,CURSOR_ATTACK,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE},
// sensor droid
{CURSOR_SELECT,CURSOR_GUARD,CURSOR_LOCKON,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,
	CURSOR_SELECT,CURSOR_SELECT,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_FIX},
// wrecked building feature
{CURSOR_DEST,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,
	CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE},
// construction droid
{CURSOR_SELECT,CURSOR_GUARD,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,CURSOR_SELECT,
	CURSOR_SELECT,CURSOR_SELECT,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_FIX},
// sensor structure
{CURSOR_NOTPOSSIBLE,CURSOR_GUARD,CURSOR_LOCKON,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_MOVE,CURSOR_MOVE,
	CURSOR_GUARD,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_DEST,CURSOR_MOVE},
// damaged sensor structure.
{CURSOR_FIX,CURSOR_GUARD,CURSOR_LOCKON,CURSOR_GUARD,CURSOR_GUARD,CURSOR_GUARD,CURSOR_MOVE,CURSOR_MOVE,
	CURSOR_GUARD,CURSOR_NOTPOSSIBLE,CURSOR_NOTPOSSIBLE,CURSOR_DEST,CURSOR_MOVE},
};

/// acceleration on scrolling. Game Option.
UDWORD	scroll_speed_accel;

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
static void	dealWithLMB( void );
static void	dealWithLMBDClick( void );
static void	dealWithRMB( void );
static bool	mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
static OBJECT_POSITION *checkMouseLoc(void);

static bool	bInstantRadarJump = false;
static SDWORD	desiredPitch = 340;
static UDWORD	currentFrame;
static UDWORD StartOfLastFrame;
static SDWORD	rotX;
static SDWORD	rotY;
static UDWORD	rotInitial;
static UDWORD	rotInitialUp;
static UDWORD	xMoved, yMoved;
static STRUCTURE	*psBuilding;
static bool	edgeOfMap = false;
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

bool	rotActive = false;
bool	gameStats = false;

/* Hackety hack hack hack */
SDWORD	screenShakeTable[100] =
{
-2,-2,-3,-4,-3,-3,-5,-4,-4,-4,
-4,-5,-5,-5,-5,-7,-5,-6,-8,-6,
-7,-8,-6,-4,-8,-7,-7,-7,-6,-5,
-6,-5,-2,-5,-6,-3,-5,-3,-2,-4,
-5,-3,-2,-0,1,2,2,1,0,0,
0,1,1,3,2,1,0,2,3,4,
4,2,6,4,5,3,7,7,3,6,
4,7,9,10,9,8,6,4,7,5,
5,4,6,2,4,5,3,3,2,1,
1,0,-1,-1,-2,-1,1,0,1,0
};

static bool	bScreenShakeActive = false;
static UDWORD screenShakeStarted;
static UDWORD screenShakeLength;
//used to determine is a weapon droid is assigned to a sensor tower or sensor droid
static bool bSensorAssigned;
//used to determine if the player has selected a Las Sat structure
static bool bLasSatStruct;
// Local prototypes
static MOUSE_TARGET	itemUnderMouse(BASE_OBJECT **ppObjUnderCursor);
static bool	bShakingPermitted = true;

void setMouseScroll(bool scroll)
{
	mouseScroll = scroll;
}

void	setRadarJump(bool	val)
{
	bInstantRadarJump = val;
}

bool	getRadarJumpStatus( void )
{
	return(bInstantRadarJump);
}

bool	getShakeStatus( void )
{
	return(bShakingPermitted);
}

bool	getInvertMouseStatus( void )
{
	return(bInvertMouse);
}

void	setInvertMouseStatus( bool val )
{
	bInvertMouse = val;
}


#define MOUSE_ORDER (bRightClickOrders?MOUSE_RMB:MOUSE_LMB)
#define MOUSE_SELECT (bRightClickOrders?MOUSE_LMB:MOUSE_RMB)
#define MOUSE_ROTATE (bMiddleClickRotate?MOUSE_MMB:MOUSE_RMB)

bool	getRightClickOrders( void )
{
	return bRightClickOrders;
}

void	setRightClickOrders( bool val )
{
	bRightClickOrders = val;
}


bool	getMiddleClickRotate( void )
{
	return bMiddleClickRotate;
}

void	setMiddleClickRotate( bool val )
{
	bMiddleClickRotate = val;
}


bool	getDrawShadows( void )
{
	return(bDrawShadows);
}

void	setDrawShadows( bool val )
{
	bDrawShadows = val;
}


void	setShakeStatus( bool val )
{
	bShakingPermitted = val;
}


void shakeStart(void)
{
	if(bShakingPermitted)
	{
		if(!bScreenShakeActive)
		{
			bScreenShakeActive = true;
			screenShakeStarted = gameTime;
			screenShakeLength = SHAKE_TIME;//1500;
		}
	}
}


void shakeStop(void)
{
	bScreenShakeActive = false;
	player.r.z = 0;
}


static void shakeUpdate(void)
{
	UDWORD	screenShakePercentage;

	/* Check if we're shaking the screen or not */
	if(bScreenShakeActive)
	{
//		screenShakePercentage = (((gameTime-screenShakeStarted)<<8) / screenShakeLength) * 100;
		screenShakePercentage = PERCENT(gameTime2-screenShakeStarted,screenShakeLength);
//		screenShakePercentage = screenShakePercentage >> 8;

		if(screenShakePercentage<100)
		{
			player.r.z = 0 + DEG(screenShakeTable[screenShakePercentage]);
		}
		if(gameTime>(screenShakeStarted+screenShakeLength))
		{
			bScreenShakeActive = false;
			player.r.z = 0;
		}
	}
	else
	{
		if(!getWarCamStatus())
		{
			player.r.z = 0;
		}
	}
}


/* Initialise the display system */
bool dispInitialise(void)
{
	return true;
}


void ProcessRadarInput(void)
{
	int PosX, PosY;
	int x = mouseX();
	int y = mouseY();
	UDWORD	temp1,temp2;

	/* Only allow jump-to-area-of-map if radar is on-screen */
	mouseOverRadar = false;
	if(radarOnScreen &&  getHQExists(selectedPlayer))
	{
		if(CoordInRadar(x,y))
		{
			mouseOverRadar = true;

			if (mousePressed(MOUSE_ORDER) || (mousePressed(MOUSE_MMB) && keyDown(KEY_LALT)))
			{
				if (mousePressed(MOUSE_ORDER))
				{
					x = mousePressPos(MOUSE_ORDER).x;
					y = mousePressPos(MOUSE_ORDER).y;
				}
				else
				{
					x = mousePressPos(MOUSE_MMB).x;
					y = mousePressPos(MOUSE_MMB).y;
				}
				if(driveModeActive()) {
					driveProcessRadarInput(x,y);
				} else {

					/* If we're tracking a droid, then cancel that */
//					if(getWarCamStatus() == true)
//					{
//						camToggleStatus();
//					}
					CalcRadarPosition(x, y, &PosX, &PosY);
					if(mouseOverRadar)
					{
					//	requestRadarTrack(PosX*TILE_UNITS,PosY*TILE_UNITS);
						// MARKER
						// Send all droids to that location
						orderSelectedLoc(selectedPlayer, (PosX*TILE_UNITS)+TILE_UNITS/2,
						                                 (PosY*TILE_UNITS)+TILE_UNITS/2, ctrlShiftDown());  // ctrlShiftDown() = ctrl clicked a destination, add an order


					}
	//				setViewPos(PosX,PosY);
					CheckScrollLimits();
					audio_PlayTrack( ID_SOUND_MESSAGEEND );
				}
			}


			if(mouseDrag(MOUSE_SELECT,&temp1,&temp2) && !rotActive)
			{
				CalcRadarPosition(x, y, &PosX, &PosY);
				setViewPos(PosX,PosY,true);
				bRadarDragging = true;
				if (ctrlShiftDown())
				{
					player.r.y = 0;
				}
			}
			else if (mousePressed(MOUSE_SELECT))
			{
				x = mousePressPos(MOUSE_SELECT).x;
				y = mousePressPos(MOUSE_SELECT).y;

				CalcRadarPosition(x, y, &PosX, &PosY);

				if(bInstantRadarJump)
				{
					/* Go instantly */
					setViewPos(PosX,PosY,true);
				}
				else
				{
					/* Pan to it */
					requestRadarTrack(PosX*TILE_UNITS,PosY*TILE_UNITS);
				}
			}
		}
	}
}


// reset the input state
void resetInput(void)
{
	rotActive = false;
	dragBox3D.status = DRAG_INACTIVE;
	wallDrag.status = DRAG_INACTIVE;
}


/* Process the user input. This just processes the key input and jumping around the radar*/
void processInput(void)
{
	bool mOverRadar = false;
	bool mOverConstruction = false;

	if (InGameOpUp || isInGamePopupUp)
	{
		dragBox3D.status = DRAG_RELEASED;	// disengage the dragging since it stops menu input
	}

	if(radarOnScreen && getHQExists(selectedPlayer) && CoordInRadar(mouseX(), mouseY()))
	{
		mOverRadar = true;
	}

	if(CoordInBuild(mouseX(), mouseY()))
	{
		mOverConstruction = true;
	}

	StartOfLastFrame = currentFrame;
	currentFrame = frameGetFrameNumber();

	psBuilding = NULL;

	edgeOfMap = false;

	ignoreRMBC = false;

	/* Process all of our key mappings */
	if (mousePressed(MOUSE_WUP))
	{
		/* Ctrl+Alt+WheelUp makes game speed up */
		if (keyDown(KEY_LCTRL) && keyDown(KEY_LALT))
		{
			kf_SpeedUp();
		}
		/* Decide if radar or world zoom in */
		else if (mOverRadar)
		{
			kf_RadarZoomIn();
		}
		else if (mOverConstruction)
		{
			kf_BuildPrevPage();
		}
		else
		{
			kf_ZoomInStep();
		}
	}

	if (mousePressed(MOUSE_WDN))
	{
		/* Ctrl+Alt+WheelDown makes game slow down */
		if (keyDown(KEY_LCTRL) && keyDown(KEY_LALT))
		{
			kf_SlowDown();
		}
		/* Decide if radar or world zoom out */
		else if (mOverRadar)
		{
			kf_RadarZoomOut();
		}
		else if (mOverConstruction)
		{
			kf_BuildNextPage();
		}
		else
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
	else
	{
		kf_SendTextMessage();		// process multiplayer chat message.
	}

	/* Allow the user to clear the console if need be */
	mouseOverConsole = mouseOverConsoleBox();
	if(mouseOverConsole && mousePressed(MOUSE_LMB))
	{
		setConsolePermanence(false, true);
	}
}


static bool OverRadarAndNotDragging(void)
{
	bool OverRadar = mouseOverRadar;

	if(getHQExists(selectedPlayer)==false)
	{
		return(false);
	}

	if( (dragBox3D.status == DRAG_DRAGGING) || (wallDrag.status == DRAG_DRAGGING) ) {
		OverRadar = false;
	}

	return OverRadar;
}


static void CheckFinishedDrag(void)
{
	if(driveModeActive()) {
		return;
	}

	if (mouseReleased(MOUSE_LMB) || mouseDown(MOUSE_RMB))
	{
		selectAttempt = false;
		if(dragBox3D.status == DRAG_DRAGGING)
		{
			if(wallDrag.status == DRAG_DRAGGING)
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
			if(!ctrlShiftDown() && wallDrag.status==DRAG_INACTIVE)
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

static void CheckStartWallDrag(void)
{
	if(driveModeActive()) {
		return;
	}

	if(mousePressed(MOUSE_LMB))
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
				debug( LOG_NEVER, "Start Wall Drag\n" );
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
static bool CheckFinishedFindPosition(void)
{
	bool OverRadar = OverRadarAndNotDragging();

	/* Do not let the player position buildings 'under' the radar */
	if(mouseReleased(MOUSE_LMB) && !OverRadar)
	{
		if (buildState == BUILD3D_VALID)
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

				if(dx >= dy) {
					wallDrag.y2 = wallDrag.y1;
				} else if(dx < dy) {
					wallDrag.x2 = wallDrag.x1;
				}

				wallDrag.status = DRAG_RELEASED;
			}
			debug( LOG_NEVER, "BUILD3D_FINISHED\n" );
			buildState = BUILD3D_FINISHED;
			return true;
		}
	}

	return false;
}


static void HandleDrag(void)
{
	UDWORD dragX, dragY;

	if ( (driveModeActive() && mouseDown(MOUSE_LMB))
		|| (mouseDrag(MOUSE_LMB, &dragX, &dragY) && !mouseOverRadar && !mouseDown(MOUSE_RMB)) )
	{
		if(!driveModeActive()) {
			dragBox3D.x1 = dragX;
			dragBox3D.x2 = mouseX();
			dragBox3D.y1 = dragY;
			dragBox3D.y2 = mouseY();

			dragBox3D.status = DRAG_DRAGGING;
		}

		if(buildState == BUILD3D_VALID)
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

				if(dx >= dy) {
					wallDrag.y2 = wallDrag.y1;
				} else if(dx < dy) {
					wallDrag.x2 = wallDrag.x1;
				}

				wallDrag.status = DRAG_DRAGGING;
			}
		}
	}
}

static UDWORD CurrentItemUnderMouse;

UDWORD getTargetType(void)
{
	return CurrentItemUnderMouse;
}

//don't want to do any of these whilst in the Intelligence Screen
void processMouseClickInput(void)
{
	UDWORD	i;
	SELECTION_TYPE	selection;
	MOUSE_TARGET	item=MT_NOTARGET;
	bool OverRadar = OverRadarAndNotDragging();

	// These four functions were embedded in this function but I moved them out for readability. In the
	// absense of any comments I had a guess as to there use and named them accordingly PD 28/05/98.
	//ignoreOrder = CheckFinishedWallDrag(); - this name is misleading since called for all Structures AB
	ignoreOrder = CheckFinishedFindPosition();

	CheckStartWallDrag();

	HandleDrag();

	CheckFinishedDrag();

	if ((mouseReleased(MOUSE_LMB) || (mouseReleased(MOUSE_MMB) && (keyDown(KEY_LALT) || keyDown(KEY_RALT)))) && !OverRadar &&
		dragBox3D.status!=DRAG_RELEASED && !ignoreOrder && !mouseOverConsole && !bDisplayMultiJoiningStatus)
	{
		if (bRightClickOrders)
		{
			dealWithRMB();
		}
		else
		{
			if (!bMultiPlayer  && establishSelection(selectedPlayer) == SC_DROID_TRANSPORTER)
			{
				// Never, *ever* let user control the transport in SP games--it breaks the scripts!
				ASSERT(game.type == CAMPAIGN, "Game type was set incorrectly!");
				return;
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

	if (driveModeActive() && !driveTacticalActive())
	{
		driveProcessAquireButton();
	}
	else
	{
		if (!driveModeActive())
		{
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
				if(getWarCamStatus())
				{
					camToggleStatus();
				}
			}

			if (!mouseDrag(MOUSE_SELECT,(UDWORD*)&rotX,(UDWORD*)&rotY) && bRadarDragging)
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
			if (mouseDrag(MOUSE_ROTATE,(UDWORD *)&rotX,(UDWORD *)&rotY) && !rotActive && !bRadarDragging)
			{
				rotInitial = player.r.y;
				rotInitialUp = player.r.x;
				xMoved = 0;
				yMoved = 0;
				rotActive = true;
			}
		}
	}

	selection = establishSelection(selectedPlayer);
	ASSERT( selection<=POSSIBLE_SELECTIONS,"Weirdy selection!" );

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
		ASSERT( item<POSSIBLE_TARGETS,"Weirdy target!" );

		// alliance override. If in alli then just use the move icon. - but not if its the same player
		//in single player, the genexp script defaults to setting an alliance between player 0 and selectedPlayer
		/* if(ObjUnderMouse && (selectedPlayer != ObjUnderMouse->player) &&
			aiCheckAlliances(selectedPlayer,ObjUnderMouse->player))
		{
			item = MT_NOTARGET;
		} */
		ObjAllied = (ObjUnderMouse && selectedPlayer != ObjUnderMouse->player && aiCheckAlliances(selectedPlayer,ObjUnderMouse->player));

		if(item != MT_NOTARGET)
		{
			// exceptions to the lookup table.
			if (ctrlShiftDown() &&
				(ObjUnderMouse != NULL) &&
				(ObjUnderMouse->player == selectedPlayer) &&
				(ObjUnderMouse->type == OBJ_DROID))
			{
				item = MT_OWNDROID;
			}
			else if (specialOrderKeyDown() &&
				(ObjUnderMouse != NULL) &&
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
			else if (selection == SC_DROID_DEMOLISH && ObjAllied)
			{
				// Can't demolish allied objects
				item = MT_BLOCKING;
			}
			// in multiPlayer check for what kind of unit can use it (TODO)
			else if (bMultiPlayer && item == MT_TRANDROID)
			{
				if ( ObjUnderMouse->player != selectedPlayer)
				{
					item = MT_OWNDROID;
				}
			}
			else if (selection==SC_DROID_CONSTRUCT)
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
					if(ObjUnderMouse							// something valid
						&& (ObjUnderMouse->type == OBJ_STRUCTURE))// check if struct
					{
						if(buildingDamaged((STRUCTURE *)ObjUnderMouse))
						{
							item = MT_OWNSTRDAM;				// replace guard/sense with usual icons.
						}else{
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
					&& selection == SC_DROID_INDIRECT )
			{
				if (!droidSensorDroidWeapon(ObjUnderMouse, psDominantSelected))
				{
					item = MT_BLOCKING;
				}
			}

			//check for VTOL droids being assigned to a sensor droid/structure
			else if ( (item == MT_SENSOR || item == MT_SENSORSTRUCT || item == MT_SENSORSTRUCTDAM)
					&& selection == SC_DROID_DIRECT
					&& vtolDroidSelected((UBYTE)selectedPlayer))
			{
				// NB. psSelectedVtol was set by vtolDroidSelected - yes I know its horrible, but it
				// only smells as much as the rest of display.c so I don't feel so bad
				if (droidSensorDroidWeapon(ObjUnderMouse, psSelectedVtol))
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
					&& terrainType(mapTile(mouseTileX,mouseTileY)) == TER_CLIFFFACE)
			{
				item = MT_BLOCKING;
			}
			// special droid at full health
			if (arnMPointers[item][selection] == CURSOR_FIX && ObjUnderMouse->type == OBJ_DROID &&
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

			if (specialOrderKeyDown() && selection == SC_DROID_TRANSPORTER &&
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
			         (asWeaponStats[((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat].weaponSubClass == WSC_LAS_SAT))
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
			ASSERT( item<POSSIBLE_TARGETS,"Weirdy target!" );
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
			&& (asWeaponStats[((STRUCTURE *)ObjUnderMouse)->asWeaps[0].nStat].weaponSubClass == WSC_LAS_SAT))
		    || ObjUnderMouse->type == OBJ_DROID))
		{
			wzSetCursor(CURSOR_SELECT); // Special casing for LasSat or own unit
		}
		else
		{
			wzSetCursor(CURSOR_DEFAULT);
		}
	}

	CurrentItemUnderMouse = item;
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
	*y += *dydt * tMid + decel/2 * tMid*tMid;
	*dydt += decel * tMid;
	dt -= tMid;

	// Accelerate if needed.
	tMid = (targetVelocity - *dydt) / accel;
	CLIP(tMid, 0, dt);
	*y += *dydt * tMid + accel/2 * tMid*tMid;
	*dydt += accel * tMid;
	dt -= tMid;

	// Continue at target velocity.
	*y += *dydt * dt;
}

void scroll(void)
{
	SDWORD	xDif,yDif;
	uint32_t timeDiff;
	int scrollDirLeftRight = 0, scrollDirUpDown = 0;
	float scroll_zoom_factor = 1+2*((getViewDistance()-MINDISTANCE)/((float)(MAXDISTANCE-MINDISTANCE)));
	float scaled_max_scroll_speed = scroll_zoom_factor * MAX_SCROLL_SPEED;
	float scaled_accel;

	if(InGameOpUp || bDisplayMultiJoiningStatus || isInGamePopupUp)		// cant scroll when menu up. or when over radar
	{
		return;
	}

	if (mouseScroll && wzMouseInWindow())
	{
		// Scroll left or right
		scrollDirLeftRight += (mouseX() > (pie_GetVideoBufferWidth() - BOUNDARY_X)) -
		                       (mouseX() < BOUNDARY_X);

		// Scroll down or up
		scrollDirUpDown += (mouseY() < BOUNDARY_Y) -
		                    (mouseY() > (pie_GetVideoBufferHeight() - BOUNDARY_Y));
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

	scaled_accel = scroll_zoom_factor * scroll_speed_accel;

	// Apparently there's stutter if using deltaRealTime, so we have our very own delta time here, just for us.
	timeDiff = wzGetTicks() - scrollRefTime;
	scrollRefTime += timeDiff;
	timeDiff = std::min<unsigned>(timeDiff, 500);  // Since we're using our own time variable, which isn't updated when dragging a box, clamp the time here so releasing the box doesn't scroll to the edge of the map suddenly.

	scrollStepLeftRight = 0;
	scrollStepUpDown = 0;
	calcScroll(&scrollStepLeftRight, &scrollSpeedLeftRight, scaled_accel, 2*scaled_accel, scrollDirLeftRight * scaled_max_scroll_speed, (float)timeDiff / GAME_TICKS_PER_SEC);
	calcScroll(&scrollStepUpDown,    &scrollSpeedUpDown,    scaled_accel, 2*scaled_accel, scrollDirUpDown    * scaled_max_scroll_speed, (float)timeDiff / GAME_TICKS_PER_SEC);

	/* Get x component of movement */
	xDif = iCosR(-player.r.y, scrollStepLeftRight) + iSinR(-player.r.y, scrollStepUpDown);
	/* Get y component of movement */
	yDif = iSinR(-player.r.y, scrollStepLeftRight) - iCosR(-player.r.y, scrollStepUpDown);

	/* Adjust player's position by these components */
	player.p.x += xDif;
	player.p.z += yDif;

	edgeOfMap = CheckScrollLimits();
}

/*
 * Reset scrolling, so we don't jump around after unpausing.
 */
void resetScroll(void)
{
	scrollRefTime = wzGetTicks();
	scrollSpeedUpDown = 0.0f;
	scrollSpeedLeftRight = 0.0f;
}

// Check a coordinate is within the scroll limits, SDWORD version.
// Returns true if edge hit.
//
bool CheckInScrollLimits(SDWORD *xPos,SDWORD *zPos)
{
	bool EdgeHit = false;
	SDWORD	minX,minY,maxX,maxY;

	//always view that little bit more than the scroll limits...
	/*minX = scrollMinX * TILE_UNITS;
	minY = scrollMinY * TILE_UNITS;
	maxX = (((scrollMaxX-1) - visibleXTiles) * TILE_UNITS);
	maxY = (((scrollMaxY-1) - visibleYTiles) * TILE_UNITS);

	if(scrollMinX==0)
	{
		minX = ((0 - visibleXTiles/2) * TILE_UNITS);
	}

	if((UDWORD)scrollMaxX == mapWidth)
	{
		maxX = ((mapWidth-1-(visibleXTiles/2)) * TILE_UNITS);
	}

	if(scrollMinY==0)
	{
		minY = ((0 - visibleYTiles/2) * TILE_UNITS);
	}

	if((UDWORD)scrollMaxY == mapHeight)
	{
		maxY = ((mapHeight-1-(visibleYTiles/2)) * TILE_UNITS);
	}*/
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
	else
	if (*xPos >= maxX)
	{
		*xPos = maxX;
		EdgeHit = true;
	}

	if (*zPos < minY)
	{
		*zPos = minY;
		EdgeHit = true;
	}
	else
	if (*zPos >= maxY)
	{
		*zPos = maxY;
		EdgeHit = true;
	}


	return EdgeHit;
}


// Check the view is within the scroll limits,
// Returns true if edge hit.
//
bool CheckScrollLimits(void)
{
	SDWORD xp = player.p.x;
	SDWORD zp = player.p.z;
	bool ret = CheckInScrollLimits(&xp,&zp);

	player.p.x = xp;
	player.p.z = zp;

	return ret;
}



/* Do the 3D display */
void displayWorld(void)
{
	Vector3i pos;

	shakeUpdate();

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
					if(mouseY() < rotY)
					{
						player.r.x = rotInitialUp - (rotY - mouseY()) * DEG(1) / 3;
					}
					else
					{
						player.r.x = rotInitialUp + (mouseY() - rotY) * DEG(1) / 3;
					}
				}
				if(player.r.x > DEG(360 + MAX_PLAYER_X_ANGLE))
				{
					player.r.x = DEG(360 + MAX_PLAYER_X_ANGLE);
				}
				if(player.r.x < DEG(360 + MIN_PLAYER_X_ANGLE))
				{
					player.r.x = DEG(360 + MIN_PLAYER_X_ANGLE);
				}

			setDesiredPitch(player.r.x/DEG_1);
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

BASE_OBJECT	*mouseTarget( void )
{
UDWORD		i;
BASE_OBJECT	*psReturn;
DROID		*psDroid;
UDWORD		dispX,dispY,dispR;

	if( (mouseTileX < 0) ||
		(mouseTileY < 0) ||
		(mouseTileX > (SDWORD)(mapWidth-1)) ||
		(mouseTileY > (SDWORD)(mapHeight-1)) )
	{
		return(NULL);
	}


	/* We haven't found anything yet */
	psReturn = NULL;

	/* First have a look through the droid lists */
	for (i=0; i<MAX_PLAYERS; i++)
	{
		/* Note the !psObject check isn't really necessary as the goto will jump out */
		for (psDroid = apsDroidLists[i]; psDroid && !psReturn;
			psDroid = psDroid->psNext)
		{

			dispX = psDroid->sDisplay.screenX;
			dispY = psDroid->sDisplay.screenY;
			dispR = psDroid->sDisplay.screenR;
			/* Only check droids that're on screen */


// Has the droid been drawn since the start of the last frame
			if (psDroid->visible[selectedPlayer] && DrawnInLastFrame(psDroid->sDisplay.frameNumber)==true)
// 			if(psDroid->sDisplay.frameNumber+1 == currentFrame)
			{
				if (mouseInBox(dispX-dispR, dispY-dispR, dispX+dispR, dispY+dispR))
				{
					/* We HAVE clicked on droid! */
					psReturn = (BASE_OBJECT *) psDroid;
					/* There's no point in checking other object types */
					return(psReturn);
				}
			}
		}
	} // end of checking for droids

	/*	Not a droid, so maybe a structure or feature?
		If still NULL after this then nothing */
	psReturn = getTileOccupier(mouseTileX, mouseTileY);

	if (psReturn == NULL)
	{
		psReturn = getTileBlueprintStructure(mouseTileX, mouseTileY);
	}

	/* Send the result back - if it's null then we clicked on an area of terrain */
	return(psReturn);
}

// Dummy structure stats used for positioning delivery points.
static STRUCTURE_STATS ReposStats;
static bool ReposValid = false;
static bool BVReposValid = false;
static FLAG_POSITION *ReposFlag;
FLAG_POSITION *deliveryPointToMove = NULL;

void StartTacticalScrollObj(WZ_DECL_UNUSED bool driveActive, WZ_DECL_UNUSED BASE_OBJECT* psObj)
{
}

void CancelTacticalScroll(void)
{
}


void displayInitVars(void)
{
	ReposValid = false;
	BVReposValid = false;
}



// Start repositioning a delivery point.
//
void StartDeliveryPosition( OBJECT_POSITION *psLocation )
{
	FLAG_POSITION	*psFlagPos;
	STRUCTURE_STATS	*tmpStructStats;
	BASE_STATS	*tmpBaseStats;

	//clear the Deliv Point if one
	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		psFlagPos = psFlagPos->psNext)
	{
		psFlagPos->selected = false;
	}

	//set this object position to be highlighted
	psLocation->selected = true;
	deliveryPointToMove = (FLAG_POSITION*)psLocation;

	if(bInTutorial)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DELIVPOINTMOVED);
	}

	// Setup dummy structure stats for positioning a delivery point.
	ReposValid = false;
	ReposFlag = NULL;
	ReposStats.baseWidth = 1;
	ReposStats.baseBreadth = 1;
	ReposStats.ref = 0;//REF_STRUCTURE_START;

	//set up the buildSite variable for drawing
	buildSite.xTL = (UWORD)psLocation->screenX;
	buildSite.yTL = (UWORD)psLocation->screenY;
	buildSite.xBR = (UWORD)(buildSite.xTL-1);
	buildSite.yBR = (UWORD)(buildSite.yTL-1);

	tmpStructStats = &ReposStats;
	tmpBaseStats = (BASE_STATS *)tmpStructStats;
	init3DBuilding(tmpBaseStats, FinishDeliveryPosition, psLocation);
}


// Finished repositioning a delivery point.
//
void FinishDeliveryPosition(UDWORD xPos,UDWORD yPos,void *UserData)
{
	//This deals with adding waypoints and moving the primary
	processDeliveryPoint(((FLAG_POSITION*)UserData)->player,
		world_coord(xPos), world_coord(yPos));

	//deselect it
	((FLAG_POSITION*)UserData)->selected = false;
	deliveryPointToMove = NULL;

	CancelDeliveryRepos();
}


// Is there a valid delivery point repositioning going on.
//
bool DeliveryReposValid(void)
{
	if(driveModeActive()) {
		return ReposValid && (ReposFlag !=NULL);
	} else {
		return BVReposValid;
	}
}


// Cancel repositioning of the delivery point without moving it.
//
void CancelDeliveryRepos(void)
{
	if((ReposValid) && (ReposFlag!=NULL))
	{
		if(driveModeActive())
		{
			DROID *Driven = driveGetDriven();
			if(Driven != NULL) {
//				Driven->selected = true;
				SelectDroid(Driven);
				camAllignWithTarget((BASE_OBJECT *)Driven);
			}
			driveEnableControl();
		}
		ReposValid = false;
		ReposFlag = NULL;
	}

	BVReposValid = false;
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

	if (psLeader != NULL)
	{
//		psLeader->selected = true;
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
	DROID	*psD;
	bool	bGotGroup;

	/*	Toggle selection on and off - allows you drag around a big
		area of droids and then exclude certain individuals */
	if (!bDragBox && psDroid->selected == true)
	{
		DeSelectDroid(psDroid);
	}
	else if (ctrlShiftDown() || !droidHasLeader(psDroid))
	{
		for(psD = apsDroidLists[selectedPlayer],bGotGroup = false; psD && !bGotGroup; psD = psD->psNext)
		{
			if(psD->selected && (psD->group!=UBYTE_MAX))
			{
				bGotGroup = true;
			}
		}
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
		psCBSelectedDroid = NULL;
	}
}


static void FeedbackOrderGiven(void)
{
	static UDWORD LastFrame = 0;
	UDWORD ThisFrame = frameGetFrameNumber();

	// Ensure only played once per game cycle.
	if(ThisFrame != LastFrame) {
		audio_PlayTrack(ID_SOUND_SELECT);
		LastFrame = ThisFrame;
	}
}

// check whether the queue order keys are pressed
bool ctrlShiftDown(void)
{
	return keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
}


void AddDerrickBurningMessage(void)
{
	addConsoleMessage(_("Cannot Build. Oil Resource Burning."),DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
	audio_PlayTrack( ID_SOUND_BUILD_FAIL );
}

static void dealWithLMBDroid(DROID* psDroid, SELECTION_TYPE selection)
{
	bool ownDroid; // Not an allied droid

	if (!aiCheckAlliances(selectedPlayer, psDroid->player))
	{
		memset(DROIDDOING, 0x0 , sizeof(DROIDDOING)); // take over the other players droid by debug menu.
		/* We've clicked on somebody else's droid */
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "(Enemy!) %s - Damage %d%% - ID %d - experience %f, %s - order %s - action %s - sensor range %hu - ECM %u - pitch %.0f",
						droidGetName(psDroid), 	100 - clip(PERCENT(psDroid->body, psDroid->originalBody), 0, 100), psDroid->id,
						psDroid->experience/65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order), getDroidActionName(psDroid->action),
						droidSensorRange(psDroid), droidConcealment(psDroid), UNDEG(psDroid->rot.pitch)));
			FeedbackOrderGiven();
		}
#endif
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT*)psDroid, ctrlShiftDown());

		//lasSat structure can select a target - in multiPlayer only
		if (bMultiPlayer && bLasSatStruct)
		{
			orderStructureObj(selectedPlayer, (BASE_OBJECT*)psDroid);
		}

		FeedbackOrderGiven();
		driveDisableTactical();

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
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT*)psDroid, ctrlShiftDown());
		FeedbackOrderGiven();
		driveDisableTactical();
	}
	else if (psDroid->droidType == DROID_TRANSPORTER)
	{
		if (selection == SC_INVALID)
		{
			//in multiPlayer mode we RMB to get the interface up
			if (bMultiPlayer && !bRightClickOrders)
			{
				psDroid->selected = true;
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
		{	// We can order all units to use the transport now
			if (cyborgDroidSelected(selectedPlayer))
			{
				// TODO add special processing for cyborgDroids
			}
			orderSelectedObj(selectedPlayer, (BASE_OBJECT*)psDroid);
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
		orderSelectedObj(selectedPlayer, (BASE_OBJECT*)psDroid);
		FeedbackOrderGiven();
		clearSelection();
		assignSensorTarget((BASE_OBJECT *)psDroid);
		dealWithDroidSelect(psDroid, false);
		turnOffMultiMsg(false);
	}
	// Clicked on a sensor? Will assign to it.
	else if (psDroid->droidType == DROID_SENSOR)
	{
		DROID* psCurr;

		bSensorAssigned = false;
		for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			//must be indirect weapon droid or VTOL weapon droid
			if ((psCurr->droidType == DROID_WEAPON) &&
			    (psCurr->selected)&&
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
		orderSelectedObj(selectedPlayer, (BASE_OBJECT*)psDroid);
		FeedbackOrderGiven();
	}
	// Clicked on a damaged unit? Will repair it.
	else if (droidIsDamaged(psDroid) && repairDroidSelected(selectedPlayer))
	{
		assignDestTarget();
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT*)psDroid, ctrlShiftDown());
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
		// Display unit info.
		/* We've clicked on one of our own droids */
#ifdef DEBUG
		if (getDebugMappingStatus()) // cheating on, so output debug info
		{
			CONPRINTF(ConsoleString, (ConsoleString,
						"%s - Damage %d%% - ID %d - experience %f, %s - order %s - action %s - sensor range %hu - ECM %u - pitch %.0f",
						droidGetName(psDroid),
						100 - clip(PERCENT(psDroid->body, psDroid->originalBody), 0, 100), psDroid->id,
						psDroid->experience/65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order), getDroidActionName(psDroid->action),
						droidSensorRange(psDroid), droidConcealment(psDroid), UNDEG(psDroid->rot.pitch)));
			FeedbackOrderGiven();
		}
		else
#endif
		if (godMode)
		{
			CONPRINTF(ConsoleString, (ConsoleString,
					"%s - Damage %d%% - Serial ID %d - Experience %f order %d action %d, %s",
					droidGetName(psDroid),
					100 - clip(PERCENT(psDroid->body, psDroid->originalBody), 0, 100),
					psDroid->id, psDroid->experience/65536.f, psDroid->order,
					psDroid->action, getDroidLevelName(psDroid)));

			FeedbackOrderGiven();
		}
		else
		{
			if(!psDroid->selected)
			{
				CONPRINTF(ConsoleString, (ConsoleString,
					_("%s - Damage %d%% - Experience %.1f, %s"),
					droidGetName(psDroid),
					100 - clip(PERCENT(psDroid->body,psDroid->originalBody), 0, 100),
					psDroid->experience/65536.f, _(getDroidLevelName(psDroid))));

				FeedbackOrderGiven();
			}
		}

		// select/deselect etc. the droid
		clearSelection();
		dealWithDroidSelect(psDroid, false);
	}
	else // Clicked on allied unit with no other possible actions
	{
		CONPRINTF(ConsoleString, (ConsoleString,
								  _("%s - Allied - Damage %d%% - Experience %d, %s"),
								  droidGetName(psDroid),
								  100 - clip(PERCENT(psDroid->body,psDroid->originalBody), 0, 100),
								  psDroid->experience/65536, getDroidLevelName(psDroid)));

		FeedbackOrderGiven();
	}


}

static void dealWithLMBStructure(STRUCTURE* psStructure, SELECTION_TYPE selection)
{
//	clearSelection();	// Clear droid selection.
	bool ownStruct = (psStructure->player == selectedPlayer);

	if (!aiCheckAlliances(psStructure->player, selectedPlayer))
	{
		/* We've clicked on somebody else's building */
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "(Enemy!) %s, ref: %d, ID: %d Damage %d%%", psStructure->pStructureType->pName, psStructure->pStructureType->ref,
				psStructure->id, 100 - clip(PERCENT(psStructure->body, psStructure->pStructureType->bodyPoints), 0, 100)));
		}
#endif
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT*)psStructure, ctrlShiftDown());
		//lasSat structure can select a target - in multiPlayer only
		if (bMultiPlayer && bLasSatStruct)
		{
			orderStructureObj(selectedPlayer, (BASE_OBJECT*)psStructure);
		}
		FeedbackOrderGiven();
		driveDisableTactical();

		return;
	}

	/* We've clicked on our own building */

	//print some info at the top of the screen for the specific structure
	if (!bRightClickOrders) printStructureInfo(psStructure);

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
// We don't actually wan't to select structures, just inform the interface we've clicked on it,
// might wan't to do this on PC as well as it fixes the problem with the interface locking multiple
// buttons in the object window.
			if (selection == SC_INVALID)
			{
				STRUCTURE* psCurr;

				/* Clear old building selection(s) - should only be one */
				for(psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
				{
					psCurr->selected = false;
				}
				/* Establish new one */
				psStructure->selected = true;
			}
			//determine if LasSat structure has been selected
			bLasSatStruct = lasSatStructSelected(psStructure);
		}

	}
	else if ((psStructure->status==SS_BUILT) &&
			(psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR) &&
			selection == SC_INVALID && ownStruct)
	{
		STRUCTURE* psCurr;

		/* Clear old building selection(s) - should only be one */
		for (psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			psCurr->selected = false;
		}
		/* Establish new one */
		psStructure->selected = true;
	}
	bSensorAssigned = false;
	orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT*)psStructure, ctrlShiftDown());
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

	driveDisableTactical();
}

static void dealWithLMBFeature(FEATURE* psFeature)
{
	//go on to check for
	if (psFeature->psStats->damageable)
	{
		orderSelectedObjAdd(selectedPlayer, (BASE_OBJECT*)psFeature, ctrlShiftDown());
		//lasSat structure can select a target - in multiPlayer only
		if (bMultiPlayer && bLasSatStruct)
		{
			orderStructureObj(selectedPlayer, (BASE_OBJECT*)psFeature);
		}
		FeedbackOrderGiven();
	}

	//clicking an oil field should start a build..
	if(psFeature->psStats->subType == FEAT_OIL_RESOURCE)
	{
		unsigned int i;
		// find any construction droids. and order them to build an oil resource.

		// first find the derrick.
		for (i = 0; (i < numStructureStats) && (asStructureStats[i].type != REF_RESOURCE_EXTRACTOR); ++i) {}

		if( (i < numStructureStats) &&
			(apStructTypeLists[selectedPlayer][i] == AVAILABLE) )	// dont go any further if no derrick stat found.
		{
			DROID* psCurr;

			// for each droid
			for(psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
			{
				if ((droidType(psCurr) == DROID_CONSTRUCT ||
					droidType(psCurr) == DROID_CYBORG_CONSTRUCT) && (psCurr->selected))
				{
					if(fireOnLocation(psFeature->pos.x,psFeature->pos.y))
					{
						// Can't build because it's burning
						AddDerrickBurningMessage();
						break;
					}

					if (ctrlShiftDown())
					{
						orderDroidStatsLocDirAdd(psCurr, DORDER_BUILD, (BASE_STATS*) &asStructureStats[i], psFeature->pos.x, psFeature->pos.y, player.r.y);
					}
					else
					{
						orderDroidStatsLocDir(psCurr, DORDER_BUILD, (BASE_STATS*) &asStructureStats[i], psFeature->pos.x, psFeature->pos.y, player.r.y, ModeQueue);
					}
					FeedbackOrderGiven();
				}
			}
		}

	}
	else
	{
		switch(psFeature->psStats->subType)
		{
			case FEAT_GEN_ARTE:
			case FEAT_OIL_DRUM:
			{
				DROID* psNearestUnit = getNearestDroid(mouseTileX*TILE_UNITS+TILE_UNITS/2,
											mouseTileY*TILE_UNITS+TILE_UNITS/2,true);
				/* If so then find the nearest unit! */
				if (psNearestUnit)	// bloody well should be!!!
				{
					orderDroidObj(psNearestUnit, DORDER_RECOVER, (BASE_OBJECT *)psFeature, ModeQueue);
					FeedbackOrderGiven();
				}
				else
				{
					orderSelectedObj(selectedPlayer, (BASE_OBJECT*)psFeature);
					FeedbackOrderGiven();
				}
				break;
			}
			case FEAT_BOULDER:
			case FEAT_HOVER:
			case FEAT_OIL_RESOURCE:
			case FEAT_VEHICLE:
			default:
				break;
		}
	}
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "(Feature) %s, ID: %d, ref: %d, body: (%d):%d", psFeature->psStats->pName, psFeature->id, psFeature->psStats->ref, psFeature->psStats->body, psFeature->body ));
		}
#endif
	driveDisableTactical();
}

static void dealWithLMBObject(BASE_OBJECT* psClickedOn)
{
	SELECTION_TYPE selection = establishSelection(selectedPlayer);
	OBJECT_TYPE type = psClickedOn->type;

	switch (type)
	{
		case OBJ_DROID:
			dealWithLMBDroid((DROID*)psClickedOn, selection);
			break;

		case OBJ_STRUCTURE:
			dealWithLMBStructure((STRUCTURE*)psClickedOn, selection);
			break;

		case OBJ_FEATURE:
			dealWithLMBFeature((FEATURE*)psClickedOn);
			break;

		default:
			// assert only when the value is outside of the valid range
			ASSERT(type < OBJ_NUM_TYPES, "Weird selection from LMB - type of clicked object is %d", (int)type);
			break;
	}
}

void	dealWithLMB( void )
{
	BASE_OBJECT         *psClickedOn;
	OBJECT_POSITION     *psLocation;
	STRUCTURE			*psStructure;


	/* Don't process if in game options are on screen */
	if (mouseOverRadar ||
	    InGameOpUp == true || widgGetFromID(psWScreen,INTINGAMEOP))
	{
		return;
	}

	/* What have we clicked on? */
	if(driveModeActive() && !driveTacticalActive())
	{
		psClickedOn = NULL;
		if (psClickedOn)
		{
			dealWithLMBObject(psClickedOn);
		}

		return;
	}
	else
	{
		psClickedOn = mouseTarget();
		if (psClickedOn)
		{
			dealWithLMBObject(psClickedOn);

			return;
		}
	}

	/*Check for a Delivery Point or a Proximity Message*/
	psLocation = checkMouseLoc();
	if (psLocation == NULL || driveModeActive() || selNumSelected(selectedPlayer))
	{
		// now changed to use the multiple order stuff
		// clicked on a destination.
		orderSelectedLoc(selectedPlayer, mousePos.x, mousePos.y, ctrlShiftDown());  // ctrlShiftDown() = ctrl clicked a destination, add an order
		/* Otherwise send them all */
		if(getNumDroidsSelected())
		{
			assignDestTarget();
			audio_PlayTrack(ID_SOUND_SELECT);
		}

		if (getDebugMappingStatus() && tileOnMap(mouseTileX, mouseTileY))
		{
			MAPTILE *psTile = mapTile(mouseTileX, mouseTileY);
			uint8_t aux = auxTile(mouseTileX, mouseTileY, selectedPlayer);

			CONPRINTF(ConsoleString, (ConsoleString, "%s tile %d, %d [%d, %d] continent(l%d, h%d) level %g illum %d %s %s w=%d s=%d j=%d",
			          tileIsExplored(psTile) ? "Explored" : "Unexplored",
			          mouseTileX, mouseTileY, world_coord(mouseTileX), world_coord(mouseTileY),
			          (int)psTile->limitedContinent, (int)psTile->hoverContinent, psTile->level, (int)psTile->illumination,
				  aux & AUXBITS_DANGER ? "danger" : "", aux & AUXBITS_THREAT ? "threat" : "",
				  (int)psTile->watchers[selectedPlayer], (int)psTile->sensors[selectedPlayer], (int)psTile->jammers[selectedPlayer]));
		}

		driveDisableTactical();

		return;
	}

	switch (psLocation->type)
	{
		case POS_DELIVERY:
			if(psLocation->player == selectedPlayer)
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
					StartDeliveryPosition(psLocation);
				}
#if 0
				/* We've clicked on one of our own DP */
				addConsoleMessage("Clicked on your delivery point",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);

				/* clear the selection */
				clearSelection();

				//set this object position to be highlighted
				psLocation->selected = true;
			}
			else
			{
				/* We've clicked on somebody else's DP - remove this sometime?*/
				addConsoleMessage("Clicked on another player's delivery point",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
#endif
			}
			break;

#if 0
		case POS_PROX:
			if(psLocation->player == selectedPlayer)
			{
				displayProximityMessage((PROXIMITY_DISPLAY *)psLocation);
			}
			break;
#endif
		default:
			ASSERT(!"unknown object position type", "Unknown type from checkMouseLoc" );
	}
}


bool	getRotActive( void )
{
	return(rotActive);
}

SDWORD	getDesiredPitch( void )
{
	return(desiredPitch);
}

void	setDesiredPitch(SDWORD pitch)
{
	desiredPitch = pitch;
}

// process LMB double clicks
static void dealWithLMBDClick(void)
{
	BASE_OBJECT		*psClickedOn;
	DROID			*psDroid;
	STRUCTURE		*psStructure;

	/* What have we clicked on? */
	psClickedOn = mouseTarget();
	/* If not NULL, then it's a droid or a structure */
	if(psClickedOn != NULL)
	{
		/* We've got a droid or a structure */
		if(psClickedOn->type == OBJ_DROID)
		{
			/* We clicked on droid */
			psDroid = (DROID *) psClickedOn;
			if(psDroid->player == selectedPlayer)
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
					selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_SAME,true);
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
static OBJECT_POSITION *	checkMouseLoc(void)
{
	FLAG_POSITION		*psPoint;
	//PROXIMITY_DISPLAY	*psProxDisp;
	UDWORD				i;
	UDWORD				dispX,dispY,dispR;

	// First have a look through the DeliveryPoint lists
	for (i=0; i<MAX_PLAYERS; i++)
	{
		//new way, handles multiple points.
		for(psPoint = apsFlagPosLists[i];psPoint;psPoint=psPoint->psNext)
		{
			dispX = psPoint->screenX;
			dispY = psPoint->screenY;
			dispR = psPoint->screenR;
			if (DrawnInLastFrame(psPoint->frameNumber)==true)	// Only check DP's that are on screen
			{
				if (mouseInBox(dispX-dispR, dispY-dispR, dispX+dispR, dispY+dispR))
				{
					// We HAVE clicked on DP!
					return psPoint;
				}
			}
		}
	}
	//now check for Proximity Message
	/*for(psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp; psProxDisp =
		psProxDisp->psNext)
	{
		dispX = psProxDisp->screenX;
		dispY = psProxDisp->screenY;
		dispR = psProxDisp->screenR;
		// Only check DP's that are on screen
		if (DrawnInLastFrame(psProxDisp->frameNumber)==true)
		{
			if (mouseInBox(dispX-dispR, dispY-dispR, dispX+dispR, dispY+dispR))
			{
				// We HAVE clicked on Proximity Message!
				return psProxDisp;
			}
		}
	}*/
	return NULL;
}


static void dealWithRMB( void )
{
	BASE_OBJECT			*psClickedOn;
	DROID				*psDroid;
	STRUCTURE			*psStructure;

	if (driveModeActive() || mouseOverRadar ||
	    InGameOpUp == true || widgGetFromID(psWScreen,INTINGAMEOP))
	{
		return;
	}

	/* What have we clicked on? */
	psClickedOn = mouseTarget();
	/* If not NULL, then it's a droid or a structure */
	if (psClickedOn != NULL)
	{
		/* We've got a droid or a structure */
		if (psClickedOn->type == OBJ_DROID )
		{
			/* We clicked on droid */
			psDroid = (DROID *) psClickedOn;
			if (psDroid->player == selectedPlayer)
			{
//				addGameMessage("Right clicked on own droid",1000,true);
//				addConsoleMessage("Right click detected on own droid",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);

				if (bRightClickOrders && ctrlShiftDown())
				{
					dealWithDroidSelect(psDroid, false);
				}
				// Not a transporter
				else if (psDroid->droidType != DROID_TRANSPORTER)
				{
					if (bRightClickOrders)
					{
						/* We've clicked on one of our own droids */
						// Display unit info.
#ifdef DEBUG
						if (getDebugMappingStatus()) // cheating on, so output debug info
						{
							CONPRINTF(ConsoleString, (ConsoleString,
										"%s - Damage %d%% - ID %d - experience %f, %s - order %s - action %s - sensor range %hu - ECM %u",
										droidGetName(psDroid),
										100 - clip(PERCENT(psDroid->body, psDroid->originalBody), 0, 100), psDroid->id,
										psDroid->experience/65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order), getDroidActionName(psDroid->action),
										droidSensorRange(psDroid), droidConcealment(psDroid)));
							FeedbackOrderGiven();
						}
						else
#endif
						if (godMode)
						{
							CONPRINTF(ConsoleString, (ConsoleString,
									"%s - Damage %d%% - Serial ID %d - Experience %f order %d action %d, %s",
									droidGetName(psDroid),
									100 - clip(PERCENT(psDroid->body, psDroid->originalBody), 0, 100),
									psDroid->id, psDroid->experience/65536.f, psDroid->order,
									psDroid->action, getDroidLevelName(psDroid)));

							FeedbackOrderGiven();
						}
						else
						{
							if(!psDroid->selected)
							{
								CONPRINTF(ConsoleString, (ConsoleString,
									_("%s - Damage %d%% - Experience %.1f, %s"),
									droidGetName(psDroid),
									100 - clip(PERCENT(psDroid->body,psDroid->originalBody), 0, 100),
									psDroid->experience/65536.f, _(getDroidLevelName(psDroid))));

								FeedbackOrderGiven();
							}
						}
						clearSelection();
						dealWithDroidSelect(psDroid, false);
					}
					else
					{
						if (psDroid->selected!=true)
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
						if (bRightClickOrders && psDroid->selected!=true)
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
			else
			{
				if (bMultiPlayer)
				{
					if (isHumanPlayer(psDroid->player))
					{
						CONPRINTF(ConsoleString, (ConsoleString,"%s",droidGetName(psDroid)));
						FeedbackOrderGiven();
					}
				}
			}

		}	// end if its a droid
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
			/* We clicked on structure */
			psStructure = (STRUCTURE *) psClickedOn;
			if (psStructure->player == selectedPlayer)
			{
				/* We've clicked on our own building */
//				addGameMessage("Right clicked on own building",1000,true);
//				addConsoleMessage("Right clicked on own building",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);

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
				else if (psStructure->selected==true)
				{
					psStructure->selected = false;
					intObjectSelected(NULL);
				}
				else if (!structureIsBlueprint(psStructure))
				{
// We don't actually wan't to select structures, just inform the interface weve clicked on it,
// might wan't to do this on PC as well as it fixes the problem with the interface locking multiple
// buttons in the object window.
					clearSelection();

					if (bRightClickOrders)
					{
						if ((psStructure->status == SS_BUILT) &&
							(psStructure->pStructureType->type != REF_RESOURCE_EXTRACTOR))
						{
							printStructureInfo(psStructure);

							psStructure->selected = true;

							// Open structure menu
							intObjectSelected((BASE_OBJECT *)psStructure);
							FeedbackOrderGiven();

							bLasSatStruct = lasSatStructSelected(psStructure);
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
		OBJECT_POSITION* psLocation = checkMouseLoc();

		if (psLocation)
		{
			switch (psLocation->type)
			{
				case POS_DELIVERY:
					if(psLocation->player == selectedPlayer)
					{
						if (bRightClickOrders)
						{
							StartDeliveryPosition(psLocation);
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
			/* Transporter orders disabled */
#if 0
			/* We've just clicked on an area of terrain. A 'send to' operation
			for Transporter in multiPlay mode*/
			if (bMultiPlayer && !bLeftClickOrders)
			{
				//there may be more than one Transporter selected
				for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL;
					psDroid = psDroid->psNext)
				{
					if (psDroid->selected)
					{
						if (psDroid->droidType == DROID_TRANSPORTER)
						{
							orderDroidLoc(psDroid, DORDER_DISEMBARK, mouseTileX *
								TILE_UNITS + TILE_UNITS/2, mouseTileY * TILE_UNITS +
								TILE_UNITS/2);
						}
						else
						{
							//de-select any other units
							psDroid->selected = false;
						}
					}
				}
			}
			else
#endif
			{
				clearSelection();
				intObjectSelected(NULL);
				memset(DROIDDOING, 0x0 , sizeof(DROIDDOING));	// clear string when deselected
			}
		}
	}
}


/* if there is a valid object under the mouse this routine returns not only the type of the object in the
return code, but also a pointer to the BASE_OBJECT) ... well if your going to be "object orientated" you might as well do it right
- it sets it to null if we don't find anything
*/
static MOUSE_TARGET	itemUnderMouse( BASE_OBJECT **ppObjectUnderMouse )
{
UDWORD		i;
MOUSE_TARGET retVal;
BASE_OBJECT	 *psNotDroid;
DROID		*psDroid;
UDWORD		dispX,dispY,dispR;
STRUCTURE	*psStructure;

	*ppObjectUnderMouse=NULL;

	if(!driveModeActive() || driveTacticalActive()) {
		if( (mouseTileX < 0) ||
			(mouseTileY < 0) ||
			(mouseTileX > (SDWORD)(mapWidth-1)) ||
			(mouseTileY > (SDWORD)(mapHeight-1)) )
		{
			retVal = MT_BLOCKING;
			return(retVal);
		}
	}

	/* We haven't found anything yet */
	retVal = MT_NOTARGET;

	/* First have a look through the droid lists */
	for (i=0; i<MAX_PLAYERS; i++)
	{
		/* Note the !psObject check isn't really necessary as the goto will jump out */
		for (psDroid = apsDroidLists[i]; psDroid && retVal==MT_NOTARGET;
			psDroid = psDroid->psNext)
		{
			dispX = psDroid->sDisplay.screenX;
			dispY = psDroid->sDisplay.screenY;
			dispR = psDroid->sDisplay.screenR;
			/* Only check droids that're on screen */
			if(psDroid->sDisplay.frameNumber+1 == currentFrame && psDroid->visible[selectedPlayer] )
			{
				if (mouseInBox(dispX-dispR, dispY-dispR, dispX+dispR, dispY+dispR))
				{
					/* We HAVE clicked on droid! */
					if(aiCheckAlliances(psDroid->player, selectedPlayer))
					{
						*ppObjectUnderMouse=(BASE_OBJECT *)psDroid;
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
						else if (psDroid->droidType == DROID_TRANSPORTER &&
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
						*ppObjectUnderMouse=(BASE_OBJECT *)psDroid;
						retVal = MT_ENEMYDROID;
					}
					/* There's no point in checking other object types */
					return(retVal);
				}
			}
		}
	} // end of checking for droids

	/*	Not a droid, so maybe a structure or feature?
		If still NULL after this then nothing */
	if(driveModeActive() && !driveTacticalActive()) {
		psNotDroid = NULL;
	}
	else
	{
		psNotDroid = getTileOccupier(mouseTileX, mouseTileY);
		if (psNotDroid == NULL)
		{
			psNotDroid = getTileBlueprintStructure(mouseTileX, mouseTileY);
		}
	}

	if(psNotDroid!=NULL)
	{
		*ppObjectUnderMouse=(BASE_OBJECT *)psNotDroid;

		if(psNotDroid->type == OBJ_FEATURE)
		{
			if(   (((FEATURE *)psNotDroid)->psStats->subType == FEAT_GEN_ARTE)
				|| (((FEATURE *)psNotDroid)->psStats->subType == FEAT_OIL_DRUM) )
			{
				retVal = MT_ARTIFACT;
			}
			else if(((FEATURE *)psNotDroid)->psStats->damageable)	//make damageable features return 'target' mouse pointer
			{
				retVal = MT_DAMFEATURE;
			}
			else if(((FEATURE *)psNotDroid)->psStats->subType == FEAT_OIL_RESOURCE)
			{
				retVal = MT_RESOURCE;
			}
			else
			{
				retVal = MT_BLOCKING;
			}
		}
		else if(psNotDroid->type == OBJ_STRUCTURE)
		{
			psStructure = (STRUCTURE *)psNotDroid;

			if (aiCheckAlliances(psNotDroid->player, selectedPlayer))
			{
				if (psStructure->status == SS_BEING_BUILT || isBlueprint(psStructure))
				{
					retVal = MT_OWNSTRINCOMP;
				}
				// repair center.
				else if(psStructure->pStructureType->type == REF_REPAIR_FACILITY)
				{
					if(buildingDamaged(psStructure))
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
					if(buildingDamaged(psStructure))
					{
						retVal = MT_SENSORSTRUCTDAM;
					}
					else
					{
						retVal = MT_SENSORSTRUCT;
					}
				}

				// standard buildings. - check for buildingDamaged BEFORE upgrades
				else if(buildingDamaged(psStructure))
				{
					retVal = MT_OWNSTRDAM;
				}
				// If this building is a factory/power generator/research facility
				// which isn't upgraded. Make the build icon available.
				else if(((psStructure->pStructureType->type == REF_FACTORY
				       || psStructure->pStructureType->type == REF_VTOL_FACTORY)
				      && psStructure->pFunctionality->factory.capacity < NUM_FACTORY_MODULES)

				     || (psStructure->pStructureType->type  == REF_POWER_GEN
				      && psStructure->pFunctionality->powerGenerator.capacity < NUM_POWER_MODULES)

				     || (psStructure->pStructureType->type  == REF_RESEARCH
				      && psStructure->pFunctionality->researchFacility.capacity < NUM_RESEARCH_MODULES))
				{
						retVal = MT_OWNSTRINCOMP;
				}

				// standard buildings.
				//else if(buildingDamaged(psStructure))
				//{
				//	retVal = MT_OWNSTRDAM;
				//}
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
	if(retVal == MT_NOTARGET || !(psNotDroid->visible[selectedPlayer])  )
	{
		retVal = MT_TERRAIN;
	}
	return(retVal);
}


// Indicates the priority given to any given droid
// type in a multiple droid selection, the larger the
// number, the lower the priority. The order of entries
// corresponds to the order of droid types in the DROID_TYPE
// enum in DroidDef.h
//
//#define NUM_DROID_WEIGHTS (10)
#define NUM_DROID_WEIGHTS (13)
UBYTE DroidSelectionWeights[NUM_DROID_WEIGHTS] = {
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
};


/* Only deals with one type of droid being selected!!!! */
/*	We'll have to make it assesss which selection is to be dominant in the case
	of multiple selections */
static SELECTION_TYPE	establishSelection(UDWORD selectedPlayer)
{
DROID			*psDroid,*psDominant=NULL;
	UBYTE	CurrWeight;
//bool			gotWeapon = false;
//DROID			*psWeapDroid = NULL;
bool			atLeastOne;
SELECTION_TYPE	selectionClass;

	atLeastOne = false;
	selectionClass = SC_INVALID;
	CurrWeight = UBYTE_MAX;

	if (intDemolishSelectMode())
	{
		return SC_DROID_DEMOLISH;
	}

	for(psDroid = apsDroidLists[selectedPlayer];
			psDroid /*&& !atLeastOne*/; psDroid = psDroid->psNext)
	{
		// This code dos'nt work, what about the case of a selection of DROID_WEAPON types with a
		// DROID_CONSTRUCT type grouped with them,claims to handle this but dos'nt.
//PD		if(psDroid->selected)
//PD		{
//PD			atLeastOne = true;
//PD			if(psDroid->type == DROID_WEAPON)
//PD			{
//PD				gotWeapon = true;
//PD				psWeapDroid = psDroid;
//PD			}
//PD			if (psDroid->droidType == DROID_COMMAND ||
//PD				psDominant == NULL)
//PD			{
//PD				psDominant = psDroid;
//PD			}
//PD		}

		// This works, uses the DroidSelectionWeights[] table to priorities the different
		// droid types and find the dominant selection.
		if(psDroid->selected) {
			ASSERT( psDroid->droidType < NUM_DROID_WEIGHTS,
				"establishSelection : droidType exceeds NUM_DROID_WEIGHTS" );

			atLeastOne = true;
			if(DroidSelectionWeights[psDroid->droidType] < CurrWeight) {
				CurrWeight = DroidSelectionWeights[psDroid->droidType];
				psDominant = psDroid;
			}
		}
	}

//	/* Weapon droids in a selection will override all others */
//	if(psWeapDroid)
//	{
//		psDominant = psWeapDroid;
//	}

//	if(psDominant) {
//		DBPRINTF(("Dominant selection type == %d\n",psDominant->droidType));
//	}

	if(atLeastOne)
	{
		psDominantSelected = psDominant;
		switch(psDominant->droidType)
		{
		case DROID_WEAPON:
			if (proj_Direct( asWeaponStats + psDominant->asWeaps[0].nStat ))
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
	return(selectionClass);
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

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		//if (psCurr->selected && psCurr->droidType == DROID_REPAIR)
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

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
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

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
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

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
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
void clearSel(void)
{
	DROID			*psCurrDroid;
	STRUCTURE		*psStruct;
	//FEATURE			*psFeat;
	FLAG_POSITION	*psFlagPos;

	for(psCurrDroid = apsDroidLists[selectedPlayer]; psCurrDroid;
		psCurrDroid = psCurrDroid->psNext)
	{
		psCurrDroid->selected = false;
	}
	for(psStruct = apsStructLists[selectedPlayer]; psStruct;
		psStruct = psStruct->psNext)
	{
		psStruct->selected = false;
	}
	bLasSatStruct = false;
	//can a feature ever be selected?
	/*for(psFeat = apsFeatureLists[0]; psFeat;
		psFeat = psFeat->psNext)
	{
		psFeat->selected = false;
	}*/
	//clear the Deliv Point if one
	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		psFlagPos = psFlagPos->psNext)
	{
		psFlagPos->selected = false;
	}
	deliveryPointToMove = NULL;

	setSelectedGroup(UBYTE_MAX);
	setSelectedCommander(UBYTE_MAX);
	intRefreshScreen();
}


// Clear the selection and stop driver mode.
//
void clearSelection(void)
{
	StopDriverMode();	// Cancel driver mode ( if active ).
	clearSel();
}

//access function for bSensorAssigned variable
void setSensorAssigned(void)
{
	bSensorAssigned = true;
}
