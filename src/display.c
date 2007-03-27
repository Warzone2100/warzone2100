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
 * Display.c
 *
 * Display routines.
 *
 */



#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"

#include "display.h"
#include "map.h"
#include "disp2d.h"
#include "loop.h"
#include "atmos.h"	// temporary only for here
#include "csnap.h"
/* Includes direct access to render library */
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"

// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"
//#include "ivisheap.h"//make a call for this

#include "lib/ivis_common/tex.h"//make a call for this
#include "component.h"
#include "display3d.h"
#include "resource.h"
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
#include "lib/framework/fractions.h"
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
#include "target.h"
#include "drive.h"
#include "cmddroid.h"
#include "gateway.h"
#include "selection.h"
#include "transporter.h"
#include "intorder.h"


#include "lib/ivis_common/pieclip.h"		// ffs am
#include "multiplay.h"

#define	SHAKE_TIME	(1500)

#define TD_SCROLL_DIV 2

//#define TEST_EFFECT	// Kick off an effect when left mouse button pressed.

struct	_dragBox dragBox3D,wallDrag;

// control whether the scroll is limited to visible tiles
//#define VISIBLE_SCROLL

#define POSSIBLE_SELECTIONS		13
#define POSSIBLE_TARGETS		23

UDWORD	arnMPointers[POSSIBLE_TARGETS][POSSIBLE_SELECTIONS] =
{
// empty terrain tile
{IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,
	IDC_BOMB,IDC_NOTPOSSIBLE,IDC_MOVE,IDC_MOVE},
// resource tile
{IDC_BUILD,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,IDC_MOVE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE},
// impassible tile
{IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE},
// river tile
{IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_BRIDGE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE},
// trench tile
{IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_BRIDGE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE},
// damaged friendly structure
{IDC_FIX,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_MOVE,IDC_MOVE,
	IDC_GUARD,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_DEST,IDC_MOVE},
// undamaged friendly structure
{IDC_NOTPOSSIBLE,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_MOVE,IDC_MOVE,
	IDC_GUARD,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_DEST,IDC_MOVE},
// partially built friendly structure
{IDC_BUILD,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_MOVE,IDC_MOVE,
	IDC_GUARD,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_DEST,IDC_MOVE},
// friendly repair facility
{IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,
	IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,
	IDC_NOTPOSSIBLE,IDC_DEST,IDC_SEEKREPAIR},
// friendly damagedrepair facility
{IDC_FIX,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,
	IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,IDC_SEEKREPAIR,
	IDC_NOTPOSSIBLE,IDC_DEST,IDC_SEEKREPAIR},
// enemy structure
{IDC_MOVE,IDC_ATTACK,IDC_ATTACK,IDC_ATTACK,IDC_LOCKON,IDC_JAM,IDC_ATTACK,
	IDC_ATTACK,IDC_ATTACK,IDC_BOMB,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_MOVE},
// transport
{IDC_EMBARK,IDC_EMBARK,IDC_EMBARK,IDC_EMBARK,IDC_EMBARK,IDC_EMBARK,IDC_EMBARK,
	IDC_EMBARK,IDC_EMBARK,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_EMBARK},
// friendly droid
{IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,
	IDC_SELECT,IDC_SELECT,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_SELECT},
// damaged friendly droid
{IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,
	IDC_PICKUP,IDC_SELECT,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_FIX},
// enemy droid
{IDC_MOVE,IDC_ATTACK,IDC_ATTACK,IDC_ATTACK,IDC_LOCKON,IDC_JAM,IDC_MOVE,
	IDC_MOVE,IDC_ATTACK,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_MOVE},
// command droid
{IDC_SELECT,IDC_ATTACH,IDC_ATTACH,IDC_ATTACH,IDC_ATTACH,IDC_ATTACH,IDC_ATTACH,
	IDC_ATTACH,IDC_SELECT,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_ATTACH},
// artefact
{IDC_PICKUP,IDC_PICKUP,IDC_PICKUP,IDC_PICKUP,IDC_PICKUP,IDC_PICKUP,IDC_PICKUP,
	IDC_PICKUP,IDC_PICKUP,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_PICKUP,IDC_PICKUP},
// feature that can be damaged ie attacked
{IDC_NOTPOSSIBLE,IDC_ATTACK,IDC_ATTACK,IDC_ATTACK,IDC_LOCKON,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_ATTACK,IDC_ATTACK,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE},
// sensor droid
{IDC_SELECT,IDC_GUARD,IDC_LOCKON,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,
	IDC_SELECT,IDC_SELECT,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_FIX},
// wrecked building feature
{IDC_DEST,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,
	IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE},
// construction droid
{IDC_SELECT,IDC_GUARD,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,IDC_SELECT,
	IDC_SELECT,IDC_SELECT,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_FIX},



// sensor structure
{IDC_BUILD,IDC_GUARD,IDC_LOCKON,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_MOVE,IDC_MOVE,
	IDC_GUARD,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_DEST,IDC_MOVE},

// damaged sensor structure.
{IDC_FIX,IDC_GUARD,IDC_LOCKON,IDC_GUARD,IDC_GUARD,IDC_GUARD,IDC_MOVE,IDC_MOVE,
	IDC_GUARD,IDC_NOTPOSSIBLE,IDC_NOTPOSSIBLE,IDC_DEST,IDC_MOVE},

};


UWORD MPointerImageIDs[]={
	IMAGE_CURSOR_DEST,		// IDC_DEST
	IMAGE_CURSOR_DEFAULT,	// IDC_SIGHT
	IMAGE_CURSOR_DEFAULT,	// IDC_TARGET
	IMAGE_CURSOR_DEFAULT,	// IDC_LARROW
	IMAGE_CURSOR_DEFAULT,	// IDC_RARROW
	IMAGE_CURSOR_DEFAULT,	// IDC_DARROW
	IMAGE_CURSOR_DEFAULT,	// IDC_UARROW
	IMAGE_CURSOR_DEFAULT,	// IDC_DEFAULT
	IMAGE_CURSOR_DEFAULT,	// IDC_EDGEOFMAP
	IMAGE_CURSOR_ATTACH,	// IDC_ATTACH
	IMAGE_CURSOR_ATTACK,	// IDC_ATTACK
	IMAGE_CURSOR_DEFAULT,	// IDC_BOMB
	IMAGE_CURSOR_BRIDGE,	// IDC_BRIDGE
	IMAGE_CURSOR_BUILD,		// IDC_BUILD
	IMAGE_CURSOR_EMBARK,	// IDC_EMBARK
	IMAGE_CURSOR_FIX,		// IDC_FIX
	IMAGE_CURSOR_GUARD,		// IDC_GUARD
	IMAGE_CURSOR_ECM,		// IDC_JAM
	IMAGE_CURSOR_LOCKON,	// IDC_LOCKON
	IMAGE_CURSOR_DEFAULT,	// IDC_MENU
	IMAGE_CURSOR_MOVE,		// IDC_MOVE
	IMAGE_CURSOR_NOTPOS,	// IDC_NOTPOSSIBLE
	IMAGE_CURSOR_PICKUP,	// IDC_PICKUP
	IMAGE_CURSOR_REPAIR,	// IDC_SEEKREPAIR
	IMAGE_CURSOR_SELECT,	// IDC_SELECT
};


UDWORD	scroll_speed_accel=800;		// acceleration on scrolling. Game Option.

BOOL	buildingDamaged(STRUCTURE *psStructure);
static BOOL	repairDroidSelected(UDWORD player);
//static DROID *constructorDroidSelected(UDWORD player);
static BOOL vtolDroidSelected(UDWORD player);
static BOOL	anyDroidSelected(UDWORD player);
static BOOL cyborgDroidSelected(UDWORD player);
//static BOOL ctrlShiftDown(void);
static BOOL bInvertMouse = TRUE;
static BOOL bDrawShadows = TRUE;
//BOOL	widgetsOn=TRUE;	//FALSE;
//BOOL	forceWidgetsOn = FALSE;
SELECTION_TYPE	establishSelection(UDWORD selectedPlayer);
void	dealWithLMB( void );
void	dealWithLMBDClick( void );
void	dealWithRMB( void );
BOOL	mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
BASE_OBJECT	*mouseTarget( void );
OBJECT_POSITION *	checkMouseLoc(void);

/* Mouse x and y - no point checking them more than once per frame */
SDWORD	mX = OFF_SCREEN, mY = OFF_SCREEN;

BOOL	bInstantRadarJump = FALSE;

BOOL	rotActive = FALSE;
SDWORD	desiredPitch = 340;
UDWORD	currentFrame;
static UDWORD StartOfLastFrame;
SDWORD	rotX;
SDWORD	rotY;
UDWORD	worldAngle;
UDWORD	rotInitial;
UDWORD	rotInitialUp;
UDWORD	xMoved,yMoved;
//DROID	*psSelected3D = NULL;
BOOL	gameStats = FALSE;
STRUCTURE	*psBuilding;
SDWORD	direction = 0;
BOOL	edgeOfMap	= FALSE;
UDWORD	scrollRefTime;
float	scrollAccel;
float	scrollSpeedLeftRight; //use two directions and add them because its simple
float	scrollStepLeftRight;
float	scrollSpeedUpDown;
float	scrollStepUpDown;
BOOL	noDrag3D;
BOOL	mouseOverRadar = FALSE;
BOOL	mouseOverConsole = FALSE;
BOOL	ignoreOrder = FALSE;
BOOL	ignoreRMBC	= TRUE;
/* Hackety hack hack hack */
/* Has the big blue droid been added to the world? */
BOOL	bigBlueInWorld = FALSE;
BOOL	missionComplete = FALSE;
UWORD	RadarZoomLevel = 0;
int gammaValue = 20;
DROID	*psSelectedVtol;
DROID	*psDominantSelected;

BOOL CheckScrollLimits(void);


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

PALETTEENTRY	gamePalette[255];		// another game palette (yawn)

#define TESTLEVEL_ID (0)

#define	METAKEY_DOWN	(keyDown(KEY_LALT) || keyDown(KEY_RALT) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT) )

static BOOL	bScreenShakeActive = FALSE;
static UDWORD screenShakeStarted;
static UDWORD screenShakeLength;
//used to determine is a weapon droid is assigned to a sensor tower or sensor droid
static BOOL bSensorAssigned;
//used to determine if the player has selected a Las Sat structure
static BOOL bLasSatStruct;

// Local prototypes
static MOUSE_TARGET	itemUnderMouse(BASE_OBJECT **ppObjUnderCursor);
BOOL	bShakingPermitted = TRUE;

void	setRadarJump(BOOL	val)
{
	bInstantRadarJump = val;
}

BOOL	getRadarJumpStatus( void )
{
	return(bInstantRadarJump);
}

BOOL	getShakeStatus( void )
{
	return(bShakingPermitted);
}

BOOL	getInvertMouseStatus( void )
{
	return(bInvertMouse);
}

void	setInvertMouseStatus( BOOL val )
{
	bInvertMouse = val;
}


BOOL	getDrawShadows( void )
{
	return(bDrawShadows);
}

void	setDrawShadows( BOOL val )
{
	bDrawShadows = val;
}


void	setShakeStatus( BOOL val )
{
	bShakingPermitted = val;
}


void shakeStart(void)
{
	if(bShakingPermitted)
	{
		if(!bScreenShakeActive)
		{
			bScreenShakeActive = TRUE;
			screenShakeStarted = gameTime;
			screenShakeLength = SHAKE_TIME;//1500;
		}
	}
}


void shakeStop(void)
{
	bScreenShakeActive = FALSE;
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
			bScreenShakeActive = FALSE;
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


BOOL LoadLevelGraphics(UBYTE LevelNumber)
{

	(void)LevelNumber;

	return TRUE;
}


/* Initialise the display system */
BOOL dispInitialise(void)
{
/*	UBYTE	*pFileData, *pImageData;
	UDWORD	fileSize, width,height;
	UDWORD i;*/


	/*	Build the transparency table that's
		used for the rgb filter rectangle plotter in ivis */
	iV_SetTransFilter(TRANS_BLUE,0);		// set the table.
	iV_SetTransFilter(TRANS_BRITE,1);		// set the other table.
	iV_SetTransFilter(TINT_BLUE,2);		// set the other other table.
	iV_SetTransFilter(TINT_DEEPBLUE,3);		// set the other other table.

//	screenSetTextColour(0xff,0xff,0xff);

	noDrag3D = FALSE;
	RadarZoomLevel = 0;

	return TRUE;
}

/* Tidy up after a mode change */
BOOL dispModeChange(void)
{
	return TRUE;
}

BOOL	bRadarDragging = FALSE;

void ProcessRadarInput(void)
{
	SDWORD PosX,PosY;
	int x = mouseX();
	int y = mouseY();
	UDWORD	temp1,temp2;

	/* Only allow jump-to-area-of-map if radar is on-screen */
	mouseOverRadar = FALSE;
	if(radarOnScreen &&  getHQExists(selectedPlayer))
	{
		if(CoordInRadar(x,y))
		{
			mouseOverRadar = TRUE;

   			if (mousePressed(MOUSE_LMB))
   			{
				if(driveModeActive()) {
					driveProcessRadarInput(x,y);
				} else {

					/* If we're tracking a droid, then cancel that */
//					if(getWarCamStatus() == TRUE)
//					{
//						camToggleStatus();
//					}
					CalcRadarPosition(x,y,(UDWORD *)&PosX,(UDWORD *)&PosY);
					if(mouseOverRadar)
					{
					//	requestRadarTrack(PosX*TILE_UNITS,PosY*TILE_UNITS);
						// MARKER
						// Send all droids to that location
						orderSelectedLoc(selectedPlayer, (PosX*TILE_UNITS)+TILE_UNITS/2,
							(PosY*TILE_UNITS)+TILE_UNITS/2);


					}
	//				setViewPos(PosX,PosY);
	   				CheckScrollLimits();
	   				audio_PlayTrack( ID_SOUND_MESSAGEEND );
				}
   			}


			if(mouseDrag(MOUSE_RMB,&temp1,&temp2) && !rotActive)
			{
   				CalcRadarPosition(x,y,(UDWORD*)&PosX,(UDWORD*)&PosY);
				setViewPos(PosX,PosY,TRUE);
				bRadarDragging = TRUE;
				if(keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL))
				{
					player.r.y = 0;
				}
			}
   			else if (mousePressed(MOUSE_RMB))
   			{
#ifdef RADAR_POSITION_AT_ZOOM
   				CalcRadarPosition(x,y,&PosX,&PosY);
   				setViewPos(PosX,PosY,TRUE);
   				CheckScrollLimits();
#endif
				/*
   				RadarZoomLevel++;
   				if(RadarZoomLevel > MAX_RADARZOOM) {
   					RadarZoomLevel = 0;
   				}
   				SetRadarZoom(RadarZoomLevel);
   				audio_PlayTrack( ID_SOUND_BUTTON_CLICK_5 );
				*/
				CalcRadarPosition(x,y,(UDWORD*)&PosX,(UDWORD*)&PosY);

				if(bInstantRadarJump)
				{
					/* Go instantly */
					setViewPos(PosX,PosY,TRUE);
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
	rotActive = FALSE;
	dragBox3D.status = DRAG_INACTIVE;
	wallDrag.status = DRAG_INACTIVE;
}


/* Process the user input. This just processes the key input and jumping around the radar*/
//BOOL processInput(void)
void processInput(void)
{
	UDWORD	x,y;
	BOOL	mOverR = FALSE;
	int	WheelZoomIterator;

	x = mouseX();
	y = mouseY();
	if(radarOnScreen && getHQExists(selectedPlayer))
	{
		if(CoordInRadar(x,y))
		{
			mOverR = TRUE;
		}
	}

	StartOfLastFrame=currentFrame;
	currentFrame=frameGetFrameNumber();

	psBuilding = NULL;

	edgeOfMap = FALSE;

	ignoreRMBC = FALSE;

	mX = mouseX();
	mY = mouseY();

	/* Process all of our key mappings */
//	keyProcessMappings();	// done later - see below.

//	if(mousePressed(MOUSE_LMB) && getRadarTrackingStatus() && !mOverR)
	if(mousePressed(MOUSE_LMB) && !mOverR && getRadarTrackingStatus())
	{
		camToggleStatus();
	}

//	if(mousePressed(MOUSE_RMB) && getRadarTrackingStatus() && !mOverR)
	if(mousePressed(MOUSE_LMB) && !mOverR && getRadarTrackingStatus())
	{
		camToggleStatus();
	}

	if(mousePressed(MOUSE_WUP))
	{
		/* CTRL+WheelUp makes game speed up */
		if(keyDown(KEY_LCTRL))
		{
			kf_SpeedUp();
		}
		else
		{
			/* Decide if radar or world zoom in */
			if(mOverR)
			{
				kf_RadarZoomIn();
			}
			else
			{
				for (WheelZoomIterator=0;WheelZoomIterator<10;WheelZoomIterator++)
					kf_ZoomIn();
			}
		}
	}

	if(mousePressed(MOUSE_WDN))
	{
		/* CTRL+WheelDown makes game slow down */
		if(keyDown(KEY_LCTRL))
		{
			kf_SlowDown();
		}
		else
		{
			/* Decide if radar or world zoom out */
			if(mOverR)
			{
				kf_RadarZoomOut();
			}
			else
			{
				for (WheelZoomIterator=0;WheelZoomIterator<10;WheelZoomIterator++)
					kf_ZoomOut();
			}
		}
	}

	if(intMode != INT_DESIGN)
	{
		if(bAllowOtherKeyPresses)
		{
			/* Run all standard mappings */
			keyProcessMappings(FALSE);
		}
		else
		{
			kf_SendTextMessage();		// process multiplayer chat message.
		}
	}
	else
	{
		/* Only process the function keys */
		keyProcessMappings(TRUE);
	}

	/* Allow the user to clear the console if need be */
	mouseOverConsole = mouseOverConsoleBox();
	if(mouseOverConsole && mousePressed(MOUSE_LMB))
	{
		setConsolePermanence(FALSE,TRUE);
	}

/*   	if(keyDown(KEY_F1))
	{
		theSun.x+=64;
		initLighting();
	}

	if(keyDown(KEY_F2))
	{
		theSun.x-=64;
		initLighting();
	}

	if(keyDown(KEY_F3))
	{
		theSun.y+=64;
		initLighting();
	}

	if(keyDown(KEY_F4))
	{
		theSun.y-=64;
		initLighting();
	}

	if(keyDown(KEY_F5))
	{
		theSun.z+=64;
		initLighting();
	}

	if(keyDown(KEY_F6))
	{
		theSun.z-=64;
		initLighting();
	}
*/
}

////don't want to do any of these whilst in the Intelligence Screen
//void processMouseClickInput(void)
//{
//	UDWORD	dragX,dragY,i;
//	SELECTION_TYPE	selection;
//	MOUSE_TARGET	item;
//	BOOL OverRadar = mouseOverRadar;
//
//	/* Have we tried to click on something - only used to signal to display3d.c */
//	if (mouseDown(MOUSE_LMB))
//	{
//		selectAttempt = TRUE;
//	}
//
//	if(mouseDown(MOUSE_RMB) &&	rotActive)
//	{
//  		if(abs(mX-rotX)>8)
//		{
//			if(mX<rotX)
//			{
//				player.r.y = rotInitial + ( ((rotX-mX)/4) * DEG(1) );
//			}
//			else
//			{
//				player.r.y = rotInitial - ( ((mX-rotX)/4) * DEG(1) );
//		   	}
//		}
//	}
//
//	if(mouseReleased(MOUSE_RMB) && rotActive)
//	{
//		rotActive = FALSE;
//		ignoreRMBC = TRUE;
//	}
//}


static BOOL OverRadarAndNotDragging(void)
{
	BOOL OverRadar = mouseOverRadar;

	if(getHQExists(selectedPlayer)==FALSE)
	{
		return(FALSE);
	}

	if( (dragBox3D.status == DRAG_DRAGGING) || (wallDrag.status == DRAG_DRAGGING) ) {
		OverRadar = FALSE;
	}

	return OverRadar;
}


static void CheckFinishedDrag(void)
{
	if(driveModeActive()) {
		return;
	}

	if (mouseReleased(MOUSE_LMB))
	{
		selectAttempt = FALSE;
		if(dragBox3D.status == DRAG_DRAGGING)
		{
    	    if(wallDrag.status == DRAG_DRAGGING)
		    {
                //if invalid location keep looking for a valid one
                if (buildState == BUILD3D_VALID || buildState == BUILD3D_FINISHED)
                {
			    	if( ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL ||
			    		(((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE))
			    	{
   			    		wallDrag.x2 = mouseTileX;
    		    		wallDrag.y2 = mouseTileY;
	    	    		wallDrag.status = DRAG_RELEASED;
			    	}
			    }
            }

			/* Only clear if shift isn't down - this is for the drag selection box for units*/
			if(!keyDown(KEY_LCTRL) && !keyDown(KEY_RCTRL)
				&& !keyDown(KEY_LSHIFT) && !keyDown(KEY_RSHIFT) && wallDrag.status==DRAG_INACTIVE)
			{
				clearSelection();
			}
    		dragBox3D.status = DRAG_RELEASED;
	    	dragBox3D.x2 = mX;
		    dragBox3D.y2 = mY;
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
		if( (buildState == BUILD3D_POS) || (buildState == BUILD3D_VALID) )
		{
			//if(((STRUCTURE_STATS *)sBuildDetails.psStats)->type >= REF_WALLH &&
			//	((STRUCTURE_STATS *)sBuildDetails.psStats)->type <= REF_WALLV)
			if( (((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL) ||
				(((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE) )
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

//BOOL CheckFinishedWallDrag(void) - misleading name - sorry Paul!
//this function is called when a location has been chosen to place a structure or a DP
static BOOL CheckFinishedFindPosition(void)
{
	BOOL OverRadar = OverRadarAndNotDragging();

//	if(driveModeActive()) {
//		return FALSE;
//	}

//	if (buildState == BUILD3D_VALID)
//	{
		/* Do not let the player position buildings 'under' the radar */
		if(mouseReleased(MOUSE_LMB) && !OverRadar)
		{
			if (buildState == BUILD3D_VALID)
			{
				if ( ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL ||
					 (((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE))
				{
					int dx,dy;

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
				return TRUE;
			} else {
				//uhoh no place to build here
//				audio_PlayTrack(ID_SOUND_BUILD_FAIL);
			}
		}
//	}

	return FALSE;
}


static void HandleDrag(void)
{
	BOOL OverRadar = mouseOverRadar;
	UDWORD	dragX,dragY;

	if(driveModeActive()) {
		if(mouseDown(MOUSE_LMB)) {
			if(buildState == BUILD3D_VALID)
			{
				if( ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL ||
					(((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE))
				{
					int dx,dy;

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

		return;
	}

	if(mouseDrag(MOUSE_LMB,&dragX,&dragY) && !OverRadar && !mouseDown(MOUSE_RMB))
	{
		dragBox3D.x1 = dragX;
		dragBox3D.x2 = mX;
		dragBox3D.y1 = dragY;
		dragBox3D.y2 = mY;

		if(buildState == BUILD3D_VALID)
		{
			if( ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL ||
				(((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE))
			{
				int dx,dy;

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
  		dragBox3D.status = DRAG_DRAGGING;
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
	BOOL OverRadar = OverRadarAndNotDragging();

	// These four functions were embedded in this function but I moved them out for readability. In the
// absense of any comments I had a guess as to there use and named them accordingly PD 28/05/98.
	//ignoreOrder = CheckFinishedWallDrag(); - this name is misleading since called for all Structures AB
    ignoreOrder = CheckFinishedFindPosition();

	CheckStartWallDrag();

	HandleDrag();

	CheckFinishedDrag();
//
	if ((mouseReleased(MOUSE_LMB) /*|| keyPressed(KEY_RCTRL)*/) && !OverRadar &&
		dragBox3D.status!=DRAG_RELEASED && !ignoreOrder && !mouseOverConsole && !bDisplayMultiJoiningStatus)
	{
		dealWithLMB();
	}

	if (mouseDClicked(MOUSE_LMB))
	{
		dealWithLMBDClick();
	}

	if(driveModeActive() && !driveTacticalActive()) {
		driveProcessAquireButton();
	} else {
		if(!driveModeActive()) {
			if(mouseReleased(MOUSE_RMB) && !rotActive && !ignoreRMBC)
			{
		//		clearSelection();
		//		psSelected3D = NULL;
				dragBox3D.status = DRAG_INACTIVE;
				// Pretty sure we wan't set walldrag status here aswell.
				wallDrag.status = DRAG_INACTIVE;
	//			printf("Cancel Wall Drag\n");
				bRadarDragging = FALSE;
				dealWithRMB();
				// Why?
				if(getWarCamStatus())
				{
					camToggleStatus();
				}
			}

			if(!mouseDrag(MOUSE_RMB,(UDWORD*)&rotX,(UDWORD*)&rotY) && bRadarDragging)
			{
				bRadarDragging = FALSE;
			}

			/* Right mouse click kills a building placement */
			if(	mouseReleased(MOUSE_RMB) &&
				(buildState == BUILD3D_POS || buildState == BUILD3D_VALID))
			{
				/* Stop the placement */
				kill3DBuilding();
				bRadarDragging = FALSE;
			}
			if(mouseDrag(MOUSE_RMB,(UDWORD *)&rotX,(UDWORD *)&rotY) && !rotActive && !bRadarDragging)
			{
				rotInitial = player.r.y;
				rotInitialUp = player.r.x;
				xMoved = 0;
				yMoved = 0;
				rotActive = TRUE;
			}
		}
	}

	selection = establishSelection(selectedPlayer);
	ASSERT( selection<=POSSIBLE_SELECTIONS,"Weirdy selection!" );

	if((selection != SC_INVALID) && !gamePaused())
	{
		BASE_OBJECT *ObjUnderMouse;

		item = itemUnderMouse(&ObjUnderMouse);
		ASSERT( item<POSSIBLE_TARGETS,"Weirdy target!" );

		// alliance override. If in alli then just use the move icon. - but not if its the same player
        //in single player, the genexp script defaults to setting an alliance between player 0 and selectedPlayer
		if(ObjUnderMouse && (selectedPlayer != ObjUnderMouse->player) &&
            aiCheckAlliances(selectedPlayer,ObjUnderMouse->player))
		{
			item = MT_NOTARGET;
		}


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
			else if ((keyDown(KEY_LALT) || keyDown(KEY_RALT)) &&
				(ObjUnderMouse != NULL) &&
				(ObjUnderMouse->player == selectedPlayer))
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
            //in multiPlayer can only put cyborgs onto a Transporter
            else if (bMultiPlayer && item == MT_TRANDROID)
            {
                if (!cyborgDroidSelected(selectedPlayer))
                {
                    item = MT_BLOCKING;
                }
            }
			else if (selection==SC_DROID_CONSTRUCT)
			{
				// We don't allow the build cursor under certain circumstances ....
				// can't build if res extractors arent available.
				if (item == MT_RESOURCE)
				{
					for(i=0;(i<numStructureStats)&&(asStructureStats[i].type != REF_RESOURCE_EXTRACTOR);i++);	// find resource stat
						if( (i < numStructureStats) && (apStructTypeLists[selectedPlayer][i] == UNAVAILABLE))		// check if you can build it!
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

			//VTOL's can't be moved to empty terrain
/*			else if (vtolDroidSelected((UBYTE)selectedPlayer)
					&& item == MT_TERRAIN
					&& selection == SC_DROID_DIRECT)
			{
				item = MT_BLOCKING;
			}*/
            //in multiPlayer Transporters can be moved around the terrain - and repaired
            if (bMultiPlayer && selection == SC_DROID_TRANSPORTER && (
                item == MT_TERRAIN || item == MT_REPAIR))
            {
                //change to standard droid selection
                selection = SC_DROID_INDIRECT;
            }
			else if (item == MT_TERRAIN
					&& TERRAIN_TYPE(mapTile(mouseTileX,mouseTileY)) == TER_CLIFFFACE)
			{
				item = MT_BLOCKING;
			}

			frameSetCursorFromRes((SWORD)arnMPointers[item][selection]);
		}
	}
	else
	{
        //exceptions, exceptions...AB 10/06/99
        if (bMultiPlayer && bLasSatStruct)
        {
    		BASE_OBJECT *ObjUnderMouse;

            item = itemUnderMouse(&ObjUnderMouse);
	    	ASSERT( item<POSSIBLE_TARGETS,"Weirdy target!" );
            if (item == MT_ENEMYDROID || item == MT_ENEMYSTR || item == MT_DAMFEATURE)
            {
                //display attack cursor
                frameSetCursorFromRes(IDC_ATTACK);
            }
            else
            {
                //display block cursor
                frameSetCursorFromRes(IDC_NOTPOSSIBLE);
            }
        }
        else
        {
	    	frameSetCursorFromRes(IDC_DEFAULT);
        }
	}

	CurrentItemUnderMouse= item;
}


void scroll(void)
{
	float	radians;
	float	cosine, sine;
	SDWORD	xDif,yDif;
#ifdef VISIBLE_SCROLL
	SDWORD	newx,newy;
#endif
	UDWORD	timeDiff;
	BOOL	bRetardScroll = FALSE;
	BOOL mouseAtLeft = FALSE, mouseAtRight = FALSE,
	     mouseAtTop = FALSE, mouseAtBottom = FALSE;

	if(InGameOpUp || bDisplayMultiJoiningStatus )		// cant scroll when menu up. or when over radar
	{
		return;
	}

	if(!keyDown(KEY_LCTRL) && !keyDown(KEY_RCTRL))
	{
		/* Scroll left */
		if (keyDown(KEY_LEFTARROW) || (mX<BOUNDARY_X))
  		{
			mouseAtLeft = TRUE;
			if(!keyDown(KEY_LEFTARROW) && mX > BOUNDARY_X/2)
			{
				bRetardScroll = TRUE;
			}
		}

		/* Scroll right */
		if (keyDown(KEY_RIGHTARROW) || (mX > (SDWORD)(pie_GetVideoBufferWidth()-BOUNDARY_X)))
		{
			mouseAtRight = TRUE;
			if(!keyDown(KEY_RIGHTARROW) && mX < (SDWORD)(pie_GetVideoBufferWidth()-(BOUNDARY_X/2)))
			{
				bRetardScroll = TRUE;
			}
		}

		/* Scroll up */
		if (keyDown(KEY_UPARROW) || (mY < BOUNDARY_Y))
		{
			mouseAtBottom = TRUE;
			if(!keyDown(KEY_UPARROW) && mY > BOUNDARY_Y/2)
			{
				bRetardScroll = TRUE;
			}
   		}

		/* Scroll down */
 		if (keyDown(KEY_DOWNARROW) || (mY > (SDWORD)(pie_GetVideoBufferHeight()-BOUNDARY_Y)))
		{
			mouseAtTop = TRUE;
			if(!keyDown(KEY_DOWNARROW) && mY < (SDWORD)(pie_GetVideoBufferHeight()-(BOUNDARY_Y/2)))
			{
				bRetardScroll = TRUE;
			}

		}
	}
	else
	{
		/* Scroll left */
		if ((mX<BOUNDARY_X))
  		{
			mouseAtLeft = TRUE;
			if(!keyDown(KEY_LEFTARROW) && mX > BOUNDARY_X/2)
			{
				bRetardScroll = TRUE;
			}
		}

		/* Scroll right */
		if ((mX > (SDWORD)(pie_GetVideoBufferWidth()-BOUNDARY_X)))
		{
			mouseAtRight = TRUE;
			if(!keyDown(KEY_RIGHTARROW) && mX < (SDWORD)(pie_GetVideoBufferWidth()-(BOUNDARY_X/2)))
			{
				bRetardScroll = TRUE;
			}
		}

		/* Scroll up */
		if ((mY < BOUNDARY_Y))
		{
			mouseAtBottom = TRUE;
			if(!keyDown(KEY_UPARROW) && mY > BOUNDARY_Y/2)
			{
				bRetardScroll = TRUE;
			}
   		}

		/* Scroll down */
 		if ((mY > (SDWORD)(pie_GetVideoBufferHeight()-BOUNDARY_Y)))
		{
			mouseAtTop = TRUE;
			if(!keyDown(KEY_DOWNARROW) && mY < (SDWORD)(pie_GetVideoBufferHeight()-(BOUNDARY_Y/2)))
			{
				bRetardScroll = TRUE;
			}

		}
	}
	/* Time to update scroll - change to should be time */
	//scrollAccel = (float)SCROLL_SPEED_ACCEL * (float)(gameTime - scrollRefTime)/(float)GAME_TICKS_PER_SEC;
	timeDiff = SDL_GetTicks() - scrollRefTime;
    //WHEN its fixed - you can uncomment it!
	//if(bRetardScroll && FALSE)	//temp until fixed
	//{
	//	timeDiff/=2;
	//}
	/* Store reference time */
	scrollRefTime = SDL_GetTicks();

	if (timeDiff > GTIME_MAXFRAME)
	{
		timeDiff = GTIME_MAXFRAME;
	}
	scrollAccel = (float)scroll_speed_accel * (float)(timeDiff) /
		(float)GAME_TICKS_PER_SEC;
	if(mouseAtLeft)
	{
		if(scrollSpeedLeftRight>0) scrollSpeedLeftRight = (float)0.0;
		scrollSpeedLeftRight -= scrollAccel;
		if(scrollSpeedLeftRight < -(float)MAX_SCROLL_SPEED)
		{
			scrollSpeedLeftRight = -(float)MAX_SCROLL_SPEED;
		}
	}
	else if(mouseAtRight)
	{
		if(scrollSpeedLeftRight<0) scrollSpeedLeftRight = (float)0.0;
		scrollSpeedLeftRight += scrollAccel;
		if(scrollSpeedLeftRight > (float)MAX_SCROLL_SPEED)
		{
			scrollSpeedLeftRight = (float)MAX_SCROLL_SPEED;
		}
	}
	else // not at left or right so retard the scroll
	{
		if(scrollSpeedLeftRight > 2*scrollAccel)
		{
			scrollSpeedLeftRight -= 2*scrollAccel;
		}
		else if(scrollSpeedLeftRight < -2*scrollAccel)
		{
			scrollSpeedLeftRight += 2*scrollAccel;
		}
		else
		{
			scrollSpeedLeftRight = (float)0.0;
		}
	}
	if(mouseAtBottom)//its at the top??
	{
		if(scrollSpeedUpDown<0) scrollSpeedUpDown = (float)0.0;
		scrollSpeedUpDown += scrollAccel;
		if(scrollSpeedUpDown > (float)MAX_SCROLL_SPEED)
		{
			scrollSpeedUpDown = (float)MAX_SCROLL_SPEED;
		}
	}
	else if(mouseAtTop)//its at the bottom??
	{
		if(scrollSpeedUpDown>0) scrollSpeedUpDown = (float)0.0;
		scrollSpeedUpDown -= scrollAccel;
		if(scrollSpeedUpDown < -(float)MAX_SCROLL_SPEED)
		{
			scrollSpeedUpDown = -(float)MAX_SCROLL_SPEED;
		}
	}
	else // not at top or bottom so retard the scroll
	{
		if(scrollSpeedUpDown > scrollAccel)
		{
			scrollSpeedUpDown -= 2*scrollAccel;
		}
		else if(scrollSpeedUpDown < -scrollAccel)
		{
			scrollSpeedUpDown += 2*scrollAccel;
		}
		else
		{
			scrollSpeedUpDown = (float)0.0;
		}
	}

	// scrool speeds updated in proportion to frame time calculate how far to step in each direction
	//scrollStepLeftRight = scrollSpeedLeftRight * (float)(gameTime - scrollRefTime)/(float)GAME_TICKS_PER_SEC;
	scrollStepLeftRight = scrollSpeedLeftRight * (float)(timeDiff) /
		(float)GAME_TICKS_PER_SEC;
	//scrollStepUpDown = scrollSpeedUpDown * (float)(gameTime - scrollRefTime)/(float)GAME_TICKS_PER_SEC;
	scrollStepUpDown = scrollSpeedUpDown * (float)(timeDiff) /
		(float)GAME_TICKS_PER_SEC;

	/* Get angle vector to scroll along */
	worldAngle = (UDWORD) ((UDWORD)player.r.y/DEG_1)%360;
	direction = (360) - worldAngle;

	/* Convert to radians */
	radians = (((float)M_PI / 180) * (direction));
	cosine = (float)cos(radians);
	sine = (float)sin(radians);

  	/* Get x component of movement */
	xDif = ROUND(cosine * scrollStepLeftRight + sine * scrollStepUpDown);
	/* Get y component of movement */
	yDif = ROUND(sine * scrollStepLeftRight - cosine * scrollStepUpDown);

	/* Adjust player's position by these components */
#ifdef VISIBLE_SCROLL
	newx = player.p.x + xDif;
	newy = player.p.z + yDif;
	if (mapTile((((VISIBLE_XTILES/2)<<TILE_SHIFT) + newx) >> TILE_SHIFT,
				(((VISIBLE_YTILES/2)<<TILE_SHIFT) + newy) >> TILE_SHIFT)->tileVisBits & (1 << selectedPlayer))
	{
		player.p.x = newx;
		player.p.z = newy;
	}
	else
	{
		if (player.p.x >> TILE_SHIFT != newx >> TILE_SHIFT)
		{
			scrollSpeedLeftRight = 0;
		}
		else
		{
			player.p.x = newx;
		}
		if (player.p.z >> TILE_SHIFT != newy >> TILE_SHIFT)
		{
			scrollSpeedUpDown = 0;
		}
		else
		{
			player.p.z = newy;
		}
	}
#else
	player.p.x += xDif;
	player.p.z += yDif;
#endif

	edgeOfMap = CheckScrollLimits();
}


// Check a coordinate is within the scroll limits, uses object type coords ie UWORDs.
// Returns TRUE if edge hit.
//
BOOL CheckObjInScrollLimits(UWORD *xPos,UWORD *zPos)
{
	SDWORD xp = ((SWORD)(*xPos)) - VISIBLE_XTILES*TILE_UNITS/2;
	SDWORD zp = ((SWORD)(*zPos)) - VISIBLE_YTILES*TILE_UNITS/2;
	BOOL ret = CheckInScrollLimits(&xp,&zp);

	*xPos = (UWORD)(xp + VISIBLE_XTILES*TILE_UNITS/2);
	*zPos = (UWORD)(zp + VISIBLE_YTILES*TILE_UNITS/2);

	return ret;
}


// Check a coordinate is within the scroll limits, SDWORD version.
// Returns TRUE if edge hit.
//
BOOL CheckInScrollLimits(SDWORD *xPos,SDWORD *zPos)
{
	BOOL EdgeHit = FALSE;
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
    minX = (scrollMinX - (visibleXTiles/2)) * TILE_UNITS;
    maxX = ((scrollMaxX - 1) - (visibleXTiles/2)) * TILE_UNITS;
    minY = (scrollMinY - (visibleYTiles/2)) * TILE_UNITS;
    maxY = ((scrollMaxY - 1) - (visibleYTiles/2)) * TILE_UNITS;

	//scroll is limited to what can be seen for current campaign
	if (*xPos < minX)
	{
		*xPos = minX;
		EdgeHit = TRUE;
	}
	else
	if (*xPos >= maxX)
	{
		*xPos = maxX;
		EdgeHit = TRUE;
	}

	if (*zPos < minY)
	{
		*zPos = minY;
		EdgeHit = TRUE;
	}
	else
	if (*zPos >= maxY)
	{
		*zPos = maxY;
		EdgeHit = TRUE;
	}


	return EdgeHit;
}


// Check the view is within the scroll limits,
// Returns TRUE if edge hit.
//
BOOL CheckScrollLimits(void)
{
	SDWORD xp = player.p.x;
	SDWORD zp = player.p.z;
	BOOL ret = CheckInScrollLimits(&xp,&zp);

	player.p.x = xp;
	player.p.z = zp;

	return ret;
}



/* Do the 3D display */
void displayWorld(void)
{
	Vector3i pos;
	shakeUpdate();

	if(mouseDown(MOUSE_RMB) &&	rotActive)
	{
  		if( (abs(mX-rotX)>8) || xMoved>8)
		{
			xMoved += (abs(mX-rotX));
			if(mX<rotX)
			{
				player.r.y = rotInitial + ( ((rotX-mX)/2) * DEG(1) );
			}
			else
			{
				player.r.y = rotInitial - ( ((mX-rotX)/2) * DEG(1) );
		   	}
		}
		if( (abs(mY-rotY)>8) || yMoved>8)
		{
				yMoved += (abs(mY-rotY));
				if(bInvertMouse)
				{
				 	if(mY<rotY)
					{
				   		player.r.x = rotInitialUp + (((rotY-mY)/3) * DEG(1));
					}
					else
					{
				 		player.r.x = rotInitialUp - (((mY-rotY)/3) * DEG(1));
					}
				}
				else
				{
					if(mY<rotY)
					{
				   		player.r.x = rotInitialUp - (((rotY-mY)/3) * DEG(1));
					}
					else
					{
				 		player.r.x = rotInitialUp + (((mY-rotY)/3) * DEG(1));
					}
				}
				if(player.r.x>DEG(360 + MAX_PLAYER_X_ANGLE))
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

	if(mouseReleased(MOUSE_RMB) && rotActive)
	{
		rotActive = FALSE;
		xMoved = yMoved = 0;
		ignoreRMBC = TRUE;
		pos.x = player.r.x;
		pos.y = player.r.y;
		pos.z = player.r.z;
		camInformOfRotation(&pos);
		bRadarDragging = FALSE;
	}

	draw3DScene();

}

BOOL	mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	if(mX>x0 && mX<x1)
	{
		if(mY>y0 && mY<y1)
		{
			return TRUE;
		}

	}

	return FALSE;
}

BOOL DrawnInLastFrame(SDWORD Frame)
{
	if (Frame>=(SDWORD)StartOfLastFrame)
	{
		return TRUE;
	}
	return FALSE;
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
			if (psDroid->visible[selectedPlayer] && DrawnInLastFrame(psDroid->sDisplay.frameNumber)==TRUE)
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

 	/* Send the result back - if it's null then we clicked on an area of terrain */
	return(psReturn);
}

UWORD lastangle;	// debugging only

// Dummy structure stats used for positioning delivery points.
static STRUCTURE_STATS ReposStats;
static BOOL ReposValid = FALSE;
static BOOL BVReposValid = FALSE;
static FLAG_POSITION *ReposFlag;

void StartTacticalScroll(BOOL driveActive)
{
}

void StartTacticalScrollObj(BOOL driveActive,BASE_OBJECT *psObj)
{
}

void CancelTacticalScroll(void)
{
}


void displayInitVars(void)
{
	ReposValid = FALSE;
	BVReposValid = FALSE;
}



// Start repositioning a delivery point.
//
void StartDeliveryPosition( OBJECT_POSITION *psLocation )
{
	FLAG_POSITION	*psFlagPos;
	/* clear the selection */
//	clearSelection();
	//clear the Deliv Point if one
	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		psFlagPos = psFlagPos->psNext)
	{
		psFlagPos->selected = FALSE;
	}

	//set this object position to be highlighted
	psLocation->selected = TRUE;

	if(bInTutorial)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DELIVPOINTMOVED);
	}

	// Setup dummy structure stats for positioning a delivery point.
	ReposValid = FALSE;
	ReposFlag = NULL;
	ReposStats.baseWidth = 1;
	ReposStats.baseBreadth = 1;
	ReposStats.ref = 0;//REF_STRUCTURE_START;

    //set up the buildSite variable for drawing
	buildSite.xTL = (UWORD)psLocation->screenX;
 	buildSite.yTL = (UWORD)psLocation->screenY;
	buildSite.xBR = (UWORD)(buildSite.xTL-1);
  	buildSite.yBR = (UWORD)(buildSite.yTL-1);


	init3DBuilding((BASE_STATS *)&ReposStats, FinishDeliveryPosition, psLocation);
}


// Finished repositioning a delivery point.
//
void FinishDeliveryPosition(UDWORD xPos,UDWORD yPos,void *UserData)
{
	//This deals with adding waypoints and moving the primary
	processDeliveryPoint(((FLAG_POSITION*)UserData)->player,
		xPos<<TILE_SHIFT, yPos<<TILE_SHIFT);

	//deselect it
	((FLAG_POSITION*)UserData)->selected = FALSE;

	CancelDeliveryRepos();
}



// Cancel repositioning of the delivery point without moveing it.
//
/*void CancelDeliveryRepos(void)
{
	if((ReposValid) && (ReposFlag!=NULL))
	{
		if(driveModeActive())
		{
			DROID *Driven = driveGetDriven();
			if(Driven != NULL) {
				Driven->selected = TRUE;
				camAllignWithTarget(Driven);
			}
			driveEnableControl();
		}
		ReposValid = FALSE;
		ReposFlag = NULL;
		buildState = BUILD3D_NONE;
	}
}*/


// Get the current screen position of the delivery point.
//
/*BOOL GetDeliveryRepos(UDWORD *xPos,UDWORD *yPos)
{
	if((ReposValid) && (ReposFlag!=NULL))
	{
		*xPos = scoord_PC2PSXx(ReposFlag->screenX);
		*yPos = scoord_PC2PSXy(ReposFlag->screenY);
		return TRUE;
	}

	return FALSE;
}*/


// Is there a valid delivery point repositioning going on.
//
BOOL DeliveryReposValid(void)
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
//				Driven->selected = TRUE;
				SelectDroid(Driven);
				camAllignWithTarget((BASE_OBJECT *)Driven);
			}
			driveEnableControl();
		}
		ReposValid = FALSE;
		ReposFlag = NULL;
	}

	BVReposValid = FALSE;
}


// check whether a clicked on droid is in a command group or assigned to a sensor
static BOOL droidHasLeader(DROID *psDroid)
{
	BASE_OBJECT		*psLeader;

	if (psDroid->droidType == DROID_COMMAND ||
		psDroid->droidType == DROID_SENSOR)
	{
		return FALSE;
	}

	psLeader = NULL;
	if ((psDroid->psGroup != NULL) &&
		(psDroid->psGroup->type == GT_COMMAND))
	{
		psLeader = (BASE_OBJECT *)psDroid->psGroup->psCommander;
	}
	else
	{
        //psLeader can be either a droid or a structure
		orderStateObj(psDroid, DORDER_FIRESUPPORT, &psLeader);
	}

	if (psLeader != NULL)
	{
//		psLeader->selected = TRUE;
        if (psLeader->type == OBJ_DROID)
        {
    		SelectDroid((DROID *)psLeader);
        }
		assignSensorTarget(psLeader);
		return TRUE;
	}

	return FALSE;
}


// deal with selecting a droid
void dealWithDroidSelect(DROID *psDroid, BOOL bDragBox)
{
	DROID	*psD;
	BOOL	bGotGroup;
	SDWORD	groupNumber = 0;

	/*	Toggle selection on and off - allows you drag around a big
		area of droids and then exclude certain individuals */
	if(!bDragBox &&
		psDroid->selected==TRUE)
	{
//		psDroid->selected = FALSE;
		DeSelectDroid(psDroid);
		// was "=="
		psDroid->group = UBYTE_MAX;
//		if(OrderUp)
		{
			/* Fix this ALEX M!!! */
		}
	}
	else if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)
			|| !droidHasLeader(psDroid))
	{

	for(psD = apsDroidLists[selectedPlayer],bGotGroup = FALSE;
		psD && !bGotGroup; psD = psD->psNext)
		{
			if(psD->selected && (psD->group!=UBYTE_MAX))
			{
				bGotGroup = TRUE;
				groupNumber = psD->group;
			}
		}
		if(bGotGroup && ( keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)) )
		{
			psDroid->group = (UBYTE)groupNumber;
			secondarySetAverageGroupState(selectedPlayer, (UDWORD)groupNumber);
		}
//		psDroid->selected = TRUE;
		if(keyDown(KEY_LALT) || keyDown(KEY_RALT) )
		{
			/* We only want to select weapon units if ALT is down on a drag */
		   	if(psDroid->asWeaps[0].nStat > 0)
			{
				SelectDroid(psDroid);
			}
		}
		else
		{
			SelectDroid(psDroid);
		}
/*						if(psDroid->droidType == DROID_COMMAND)
		{
			cmdSelectSubDroids(psDroid);
		}*/
//					intObjectSelected((BASE_OBJECT *)psDroid);
		if (bInTutorial)
		{
			psCBSelectedDroid = psDroid;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_SELECTED);
			psCBSelectedDroid = NULL;
		}
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


static void FeedbackClickedOn(void)
{
	FeedbackOrderGiven();
}


// check whether the queue order keys are pressed
BOOL ctrlShiftDown(void)
{
	return keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
}


void BeepMessage(UDWORD StringID)
{
	static UDWORD LastFrame = 0;
	UDWORD ThisFrame = frameGetFrameNumber();

	// Ensure only done once per game cycle.
	if(ThisFrame != LastFrame) {
		addConsoleMessage(strresGetString(psStringRes,StringID),DEFAULT_JUSTIFY);
		audio_PlayTrack( ID_SOUND_BUILD_FAIL );
		LastFrame = ThisFrame;
	}
}


void AddDerrickBurningMessage(void)
{
	addConsoleMessage(strresGetString(psStringRes,STR_GAM_DERRICK_BURNING),DEFAULT_JUSTIFY);
	audio_PlayTrack( ID_SOUND_BUILD_FAIL );
}


void	dealWithLMB( void )
{
BASE_OBJECT		*psClickedOn;
DROID			*psDroid,*psCurr,*psNearestUnit;
//BOOL			bWeapDroidSelected;
STRUCTURE		*psStructure,*psSLoop;
FEATURE			*psFeature;
OBJECT_POSITION	*psLocation;
UDWORD			i;
SELECTION_TYPE	selection;
DROID_OACTION_INFO oaInfo = {{NULL}};


	/* Don't process if in game options are on screen */
	if(InGameOpUp == TRUE || widgGetFromID(psWScreen,INTINGAMEOP))
	{
		return;
	}

	/* What have we clicked on? */
	if(driveModeActive() && !driveTacticalActive())
	{
		psClickedOn = targetGetCurrent();
	}
	else
	{
		psClickedOn = mouseTarget();
	}

	/* If not NULL, then it's a droid or a structure */
	if(psClickedOn != NULL)
	{
		selection = establishSelection(selectedPlayer);

		/* We've got a droid or a structure */
		if(psClickedOn->type == OBJ_DROID)
		{
			/* We clicked on droid */
			psDroid = (DROID *) psClickedOn;
			if(psDroid->player == selectedPlayer)
			{
				if (ctrlShiftDown())
				{
					// select/deselect etc. the droid
					dealWithDroidSelect(psDroid, FALSE);
				}
				else
				if (psDroid->droidType == DROID_TRANSPORTER)
				{
					if (selection == SC_INVALID)
					{
						//in multiPlayer mode we RMB to get the interface up
						if (bMultiPlayer)
						{
							psDroid->selected = TRUE;
						}
						else
						{
							intResetScreen(FALSE);
							if(!getWidgetsStatus())
							{
								setWidgetsStatus(TRUE);
							}
							addTransporterInterface(psDroid, FALSE);
                        }
					}
					else
					{
						orderSelectedObj(selectedPlayer, psClickedOn);
						FeedbackOrderGiven();
					}
				}
				else if (keyDown(KEY_LALT) || keyDown(KEY_RALT))
				{
					// try to attack your own unit
					for(psCurr=apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
					{
						if ((psCurr != psDroid) && // can't attack yourself
							(psCurr->selected))
						{
							if ((psCurr->droidType == DROID_WEAPON) ||
								(psCurr->droidType == DROID_CYBORG) ||
								(psCurr->droidType == DROID_CYBORG_SUPER) ||
							    (psCurr->droidType == DROID_COMMAND))
							{
								oaInfo.objects[0] = psClickedOn;
								orderDroidObj(psCurr, DORDER_ATTACK, &oaInfo);
								FeedbackOrderGiven();
							}
							else if (psCurr->droidType == DROID_SENSOR)
							{
								oaInfo.objects[0] = psClickedOn;
								orderDroidObj(psCurr, DORDER_OBSERVE, &oaInfo);
								FeedbackOrderGiven();
							}
							else if (psCurr->droidType == DROID_REPAIR ||
                                psCurr->droidType == DROID_CYBORG_REPAIR)
							{
								oaInfo.objects[0] = psClickedOn;
								orderDroidObj(psCurr, DORDER_DROIDREPAIR, &oaInfo);
								FeedbackOrderGiven();
							}
						}
					}
				}
				// Clicked on a commander? Will link to it.
				else if (psDroid->droidType == DROID_COMMAND &&	selection != SC_INVALID &&
					selection != SC_DROID_COMMAND &&
					selection != SC_DROID_CONSTRUCT &&
					!( keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) ) &&
					!( keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))	)
				{
					turnOffMultiMsg(TRUE);
					orderSelectedObj(selectedPlayer, psClickedOn);
					FeedbackOrderGiven();
					clearSelection();
					assignSensorTarget((BASE_OBJECT *)psDroid);
					dealWithDroidSelect(psDroid, FALSE);
					turnOffMultiMsg(FALSE);
				}
				// Clicked on a construction unit? Will guard it.
				else if ( ((psDroid->droidType == DROID_CONSTRUCT) || (psDroid->droidType == DROID_SENSOR) )
					&& selection == SC_DROID_DIRECT)
				{
					orderSelectedObj(selectedPlayer, psClickedOn);
					FeedbackOrderGiven();
				}
				// Clicked on a damaged unit? Will repair it.
				else if (droidIsDamaged(psDroid) && repairDroidSelected(selectedPlayer))
				{
					assignDestTarget();
					orderSelectedObjAdd(selectedPlayer, psClickedOn, ctrlShiftDown());
					FeedbackOrderGiven();
				}
				// Just plain clicked on?
				else
				{
					// Display unit info.
					/* We've clicked on one of our own droids */
					if(godMode)
					{
						CONPRINTF(ConsoleString, (ConsoleString,
						"%s - Damage %d%% - Serial ID %d - Kills %d order %d action %d, %s",
						droidGetName(psDroid), 100 - PERCENT(psDroid->body,
						psDroid->originalBody),psDroid->id,psDroid->numKills,
						psDroid->order, psDroid->action, getDroidLevelName(psDroid)));
						FeedbackClickedOn();
					}
					else
					{
						if(!psDroid->selected)
						{
							CONPRINTF(ConsoleString, (ConsoleString,
								strresGetString(psStringRes,STR_GAM_DROIDSTATE),
							/*	"%s - Damage %d%% - Kills %d, %s",*/
							droidGetName(psDroid), 100 - PERCENT(psDroid->body,
							psDroid->originalBody),psDroid->numKills,
							getDroidLevelName(psDroid)));
							FeedbackClickedOn();
						}
					}

					if(psDroid->droidType == DROID_SENSOR)
					{
						//bWeapDroidSelected = FALSE;
						bSensorAssigned = FALSE;
					   	for(psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
						{
							//must be indirect weapon droid or VTOL weapon droid
							if( (psCurr->droidType == DROID_WEAPON) &&
								(psCurr->selected)&&
								(psCurr->asWeaps[0].nStat > 0) &&
								((!proj_Direct(asWeaponStats + psCurr->asWeaps[0].nStat)) ||
								vtolDroid(psCurr)) &&
								droidSensorDroidWeapon((BASE_OBJECT *)psDroid, psCurr))
							{
								//bWeapDroidSelected = TRUE;
								bSensorAssigned = TRUE;
								oaInfo.objects[0] = (BASE_OBJECT *)psDroid;
								orderDroidObj(psCurr, DORDER_FIRESUPPORT, &oaInfo);
								FeedbackOrderGiven();
							}
						}
						//if(bWeapDroidSelected)
						if (bSensorAssigned)
						{
							//assignSensorTarget(psDroid);
							assignSensorTarget((BASE_OBJECT *)psDroid);
						}
					}

					//cannot have LasSat struct and Droid selected
					bLasSatStruct = FALSE;

					// select/deselect etc. the droid
 					if(!ctrlShiftDown())
					{
						clearSelection();
						dealWithDroidSelect(psDroid, FALSE);
					}
				}
			}
			else
			{
				/* We've clicked on somebody else's droid */
//				addConsoleMessage("Clicked on another player's droid",DEFAULT_JUSTIFY);
				orderSelectedObjAdd(selectedPlayer,psClickedOn, ctrlShiftDown());

				//lasSat structure can select a target - in multiPlayer only
				if (bMultiPlayer && bLasSatStruct &&
					aiCheckAlliances(selectedPlayer,psClickedOn->player) == FALSE)
				{
					orderStructureObj(selectedPlayer,psClickedOn);
				}

				FeedbackOrderGiven();
		 		driveDisableTactical();
			}

		}	// end if its a droid
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
//			clearSelection();	// Clear droid selection.

			psStructure = (STRUCTURE *) psClickedOn;
			if(psStructure->player == selectedPlayer)
			{
				/* We've clicked on our own building */

				//print some info at the top of the screen for the specific structure
				printStructureInfo(psStructure);

				/* Got to be built. Also, you can't 'select' derricks */
				if( (psStructure->status==SS_BUILT) &&
					(psStructure->pStructureType->type != REF_RESOURCE_EXTRACTOR) )
				{
					//if selected object is an upgradeable structure then don't
					//inform the interface (if not fully upgraded) and a any droid
					//is selected
/*					if (!(((psStructure->pStructureType->type == REF_FACTORY &&
						((FACTORY *)psStructure->pFunctionality)->capacity <
						NUM_FACTORY_MODULES) ||
						(psStructure->pStructureType->type == REF_RESEARCH &&
						((RESEARCH_FACILITY *)psStructure->pFunctionality)->capacity <
						NUM_RESEARCH_MODULES) ||
						(psStructure->pStructureType->type == REF_VTOL_FACTORY &&
						((FACTORY *)psStructure->pFunctionality)->capacity <
						NUM_FACTORY_MODULES)) &&
						//constructorDroidSelected(selectedPlayer)))
						anyDroidSelected(selectedPlayer)))*/
					// now only display interface if nothing selected
					if (!anyDroidSelected(selectedPlayer))
					{
						intObjectSelected((BASE_OBJECT *)psStructure);
						FeedbackClickedOn();
					}
// We don't actually wan't to select structures, just inform the interface we've clicked on it,
// might wan't to do this on PC as well as it fixes the problem with the interface locking multiple
// buttons in the object window.
					if (selection == SC_INVALID)
					{
						/* Clear old building selection(s) - should only be one */
						for(psSLoop = apsStructLists[selectedPlayer]; psSLoop; psSLoop = psSLoop->psNext)
						{
							psSLoop->selected = FALSE;
						}
						/* Establish new one */
						psStructure->selected = TRUE;
					}
                //determine if LasSat structure has been selected
                bLasSatStruct = FALSE;
                if (lasSatStructSelected(psStructure))
                {
                    bLasSatStruct = TRUE;
                }

				}
				else if ( (psStructure->status==SS_BUILT) &&
						(psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR) &&
						selection == SC_INVALID)
				{
					/* Clear old building selection(s) - should only be one */
					for(psSLoop = apsStructLists[selectedPlayer]; psSLoop; psSLoop = psSLoop->psNext)
					{
						psSLoop->selected = FALSE;
					}
					/* Establish new one */
					psStructure->selected = TRUE;
				}
				if (keyDown(KEY_LALT) || keyDown(KEY_RALT))
				{
					// try to attack your own structure
					for(psCurr=apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
					{
						if (psCurr->selected)
						{
							//if ((psCurr->droidType == DROID_WEAPON) || (psCurr->droidType == DROID_CYBORG) ||
                            if ((psCurr->droidType == DROID_WEAPON) || cyborgDroid(psCurr) ||
								(psCurr->droidType == DROID_COMMAND))
							{
								oaInfo.objects[0] = psClickedOn;
								orderDroidObj(psCurr, DORDER_ATTACK, &oaInfo);
								FeedbackOrderGiven();
							}
							else if (psCurr->droidType == DROID_SENSOR)
							{
								oaInfo.objects[0] = psClickedOn;
								orderDroidObj(psCurr, DORDER_OBSERVE, &oaInfo);
								FeedbackOrderGiven();
							}
						}
					}
				}
				else
				{
					bSensorAssigned = FALSE;
					orderSelectedObjAdd(selectedPlayer,psClickedOn, ctrlShiftDown());
					FeedbackOrderGiven();
					if (bSensorAssigned)
					{
						clearSelection();
						assignSensorTarget((BASE_OBJECT *)psStructure);
					}
				}
			}
			else
			{
				/* We've clicked on somebody else's building */
//				addConsoleMessage("Clicked on another player's building",DEFAULT_JUSTIFY);
				orderSelectedObjAdd(selectedPlayer,psClickedOn, ctrlShiftDown());
                //lasSat structure can select a target - in multiPlayer only
                if (bMultiPlayer && bLasSatStruct)
                {
                    orderStructureObj(selectedPlayer,psClickedOn);
                }
				FeedbackOrderGiven();
			}
	 		driveDisableTactical();
		}	// end if its a structure

		else if (psClickedOn->type == OBJ_FEATURE)
		{
			//some features are targetable
			psFeature = (FEATURE *) psClickedOn;
			//check for constructor droid trying to remove wrecked building first
			if (psFeature->psStats->subType == FEAT_BUILD_WRECK &&
				(constructorDroidSelected(selectedPlayer) != NULL) )
			{
				orderSelectedObjAdd(selectedPlayer,psClickedOn, ctrlShiftDown());
				FeedbackOrderGiven();
			}

			//go on to check for
			if (psFeature->psStats->damageable)
			{
				orderSelectedObjAdd(selectedPlayer,psClickedOn, ctrlShiftDown());
                //lasSat structure can select a target - in multiPlayer only
                if (bMultiPlayer && bLasSatStruct)
                {
                    orderStructureObj(selectedPlayer,psClickedOn);
                }
				FeedbackOrderGiven();
			}


			//clicking an oil field should start a build..
			//if(psFeature->subType == FEAT_OIL_RESOURCE)
			if(psFeature->psStats->subType == FEAT_OIL_RESOURCE)
			{
				// find any construction droids. and order them to build an oil resource.

				// first find the derrick.
				for(i=0;(i<numStructureStats)&&(asStructureStats[i].type != REF_RESOURCE_EXTRACTOR);i++);

				if( (i < numStructureStats) &&
					(apStructTypeLists[selectedPlayer][i] == AVAILABLE) )	// dont go any further if no derrick stat found.
				{
					// for each droid
					for(psDroid = apsDroidLists[selectedPlayer]; psDroid;psDroid = psDroid->psNext)
					{
						//if((droidType(psDroid) == DROID_CONSTRUCT) && (psDroid->selected))
                        if ((droidType(psDroid) == DROID_CONSTRUCT ||
                            droidType(psDroid) == DROID_CYBORG_CONSTRUCT) && (psDroid->selected))
						{
							if(!fireOnLocation(psFeature->x,psFeature->y))
							{
                                if (ctrlShiftDown())
                                {
                                    orderDroidStatsLocAdd(psDroid, DORDER_BUILD,
                                        (BASE_STATS*) &asStructureStats[i],
                                        psFeature->x, psFeature->y);
                                }
                                else
                                {
								    orderDroidStatsLoc(psDroid, DORDER_BUILD,
                                        (BASE_STATS*) &asStructureStats[i],
                                        psFeature->x, psFeature->y);
                                }
								addConsoleMessage(strresGetString(psStringRes,STR_GAM_DERRICK),DEFAULT_JUSTIFY);
					//				"Construction vehicle ordered to build a Derrick.",DEFAULT_JUSTIFY);
								FeedbackOrderGiven();
							}
							else	// can't build because it's burning
							{
								AddDerrickBurningMessage();
								break;
							}
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
  					psNearestUnit = getNearestDroid(mouseTileX*TILE_UNITS+TILE_UNITS/2,
												   mouseTileY*TILE_UNITS+TILE_UNITS/2,TRUE);
					/* If so then find the nearest unit! */
					if(psNearestUnit)	// bloody well should be!!!
					{
						oaInfo.objects[0] = (BASE_OBJECT *)psClickedOn;
//						orderDroidLoc(psNearestUnit,DORDER_MOVE, mouseTileX*TILE_UNITS+TILE_UNITS/2,mouseTileY*TILE_UNITS+TILE_UNITS/2);
						orderDroidObj(psNearestUnit, DORDER_RECOVER, &oaInfo);
						FeedbackOrderGiven();
					}
					else
					{
//						orderSelectedLoc(selectedPlayer, psFeature->x,psFeature->y);	// recover it.
						orderSelectedObj(selectedPlayer, psClickedOn);
						FeedbackOrderGiven();
					}
					break;
/*				case FEAT_OIL_DRUM:
   					psNearestUnit = getNearestDroid(mouseTileX*TILE_UNITS+TILE_UNITS/2,
												   mouseTileY*TILE_UNITS+TILE_UNITS/2,TRUE);
					// If so then find the nearest unit!
					if(psNearestUnit)	// bloody well should be!!!
					{
						orderDroidLoc(psNearestUnit,DORDER_MOVE, mouseTileX*TILE_UNITS+TILE_UNITS/2,mouseTileY*TILE_UNITS+TILE_UNITS/2);
					}
					else
					{
						orderSelectedLoc(selectedPlayer, psFeature->x,psFeature->y);	// recover it.
					}
					break;*/
				case FEAT_BOULDER:
					//addConsoleMessage("Clicked on a Boulder",DEFAULT_JUSTIFY);
					break;
				case FEAT_BUILD_WRECK:
//					addConsoleMessage("Clicked on a Wrecked Building",DEFAULT_JUSTIFY);
					break;
				case FEAT_HOVER:
//					addConsoleMessage("Clicked on a Hover",DEFAULT_JUSTIFY);
					break;
				case FEAT_OIL_RESOURCE:
				case FEAT_VEHICLE:
				default:
//					addConsoleMessage("Clicked on an Obstacle",DEFAULT_JUSTIFY);
					break;

				}
			}

	 		driveDisableTactical();
		}
		else
		{
			ASSERT( FALSE,"Weirdy selection from LMB?!" );
		}
	}
	else
	if( !driveModeActive() || driveTacticalActive()) {
		/*Check for a Delivery Point or a Proximity Message*/
		psLocation = NULL;
		psLocation = checkMouseLoc();
		if (psLocation && !driveModeActive() && !selNumSelected(selectedPlayer))
		{
			switch (psLocation->type)
			{
			case POS_DELIVERY:
				if(psLocation->player == selectedPlayer)
				{
					StartDeliveryPosition( psLocation );
					/* We've clicked on one of our own DP */
 //					addConsoleMessage("Clicked on your delivery point",DEFAULT_JUSTIFY);


//					/* clear the selection */
//					clearSelection();
//
//					//set this object position to be highlighted
//					psLocation->selected = TRUE;
				}
				else
				{
					/* We've clicked on somebody else's DP - remove this sometime?*/
 //					addConsoleMessage("Clicked on another player's delivery point",DEFAULT_JUSTIFY);
				}
				break;
			/*case POS_PROX:
				if(psLocation->player == selectedPlayer)
				{
					displayProximityMessage((PROXIMITY_DISPLAY *)psLocation);
				}
				break;*/
			default:
				ASSERT( FALSE, "Unknown type from checkMouseLoc" );
			}
		}
		else
		{
#ifdef TEST_EFFECT
// Code to test an effect when left mouse button pressed
			Vector3i Pos;
			Pos.x = mouseTileX*TILE_UNITS+TILE_UNITS/2;
			Pos.z = mouseTileY*TILE_UNITS+TILE_UNITS/2;
			Pos.y = 100;
//			addEffect(&Pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,resGetData("IMD","fxlswave.pie"));
//			DBPRINTF(("Added test effect %p : %d,%d,%d\n",resGetData("IMD","fxlswave.pie"),Pos.x,Pos.y,Pos.z);
//			addEffect(&Pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,debrisImds[rand()%MAX_DEBRIS]);
			addEffect(&Pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
#endif

			/* We've just clicked on an area of terrain. A 'send to' operation */
			/* We need to establish the world coordinates */
/*			if(keyDown(KEY_LALT) || keyDown(KEY_RALT))		// shift clicked a destination, add a waypoint.
			{
				orderSelectedWaypoint(selectedPlayer,mouseTileX*TILE_UNITS+TILE_UNITS/2,mouseTileY*TILE_UNITS+TILE_UNITS/2);
			}*/

			// now changed to use the multiple order stuff
			if(ctrlShiftDown())		// shift clicked a destination, add an order
			{
				orderSelectedLocAdd(selectedPlayer,
									mouseTileX*TILE_UNITS+TILE_UNITS/2,
									mouseTileY*TILE_UNITS+TILE_UNITS/2, TRUE);
			}
			else		// clicked on a destination.
			{
				/* Otherwise send them all */
				orderSelectedLoc(selectedPlayer, mouseTileX*TILE_UNITS+TILE_UNITS/2,mouseTileY*TILE_UNITS+TILE_UNITS/2);
//DBPRINTF(("orderSelectedLoc(%d,%d,%d)\n",selectedPlayer, mouseTileX*TILE_UNITS+TILE_UNITS/2,mouseTileY*TILE_UNITS+TILE_UNITS/2));
				if(getNumDroidsSelected())
				{
					assignDestTarget();
	   				audio_PlayTrack( ID_SOUND_SELECT );
				}

				if(godMode && (mouseTileX >= 0) && (mouseTileX < (SDWORD)mapWidth) &&
                    (mouseTileY >= 0) && (mouseTileY < (SDWORD)mapHeight))
				{
					DBCONPRINTF(ConsoleString,(ConsoleString,"Tile Coords : %d,%d (%d,%d) Zone :%d", mouseTileX,mouseTileY,
						mouseTileX*TILE_UNITS + TILE_UNITS/2, mouseTileY*TILE_UNITS + TILE_UNITS/2,
						gwGetZone(mouseTileX, mouseTileY)));
				}

				//addConsoleMessage("Droid ordered to new location",DEFAULT_JUSTIFY);
			}
	 		driveDisableTactical();
		}
	}
}


BOOL	getRotActive( void )
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
void dealWithLMBDClick(void)
{
	BASE_OBJECT		*psClickedOn;
	DROID			*psDroid;

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
				//if (psDroid->droidType == DROID_CONSTRUCT)
                if (psDroid->droidType == DROID_CONSTRUCT ||
                    psDroid->droidType == DROID_CYBORG_CONSTRUCT)
				{
					intResetScreen(TRUE);
					intConstructorSelected(psDroid);
				}
				else if (psDroid->droidType == DROID_COMMAND)
				{
					intResetScreen(TRUE);
					intCommanderSelected(psDroid);
				}
				else
				{
					/* Otherwise, activate the droid's group (if any) */
//					activateGroup(selectedPlayer,psDroid->group);
					// Now selects all of smae type on screen
					selDroidSelection(selectedPlayer,DS_BY_TYPE,DST_ALL_SAME,TRUE);

				}
			}
		}
	}
}


/*This checks to see if the mouse was over a delivery point or a proximity message
  when the mouse button was pressed */
OBJECT_POSITION *	checkMouseLoc(void)
{
	OBJECT_POSITION		*psReturn;
	FLAG_POSITION		*psPoint;
	//PROXIMITY_DISPLAY	*psProxDisp;
	UDWORD				i;
	UDWORD				dispX,dispY,dispR;

	// We haven't found anything yet
	psReturn = NULL;

	// First have a look through the DeliveryPoint lists
	for (i=0; i<MAX_PLAYERS; i++)
	{
		//new way, handles multiple points.
		for(psPoint = apsFlagPosLists[i];psPoint;psPoint=psPoint->psNext)
		{
			dispX = psPoint->screenX;
			dispY = psPoint->screenY;
			dispR = psPoint->screenR;
			if (DrawnInLastFrame(psPoint->frameNumber)==TRUE)	// Only check DP's that are on screen
			{
				if (mouseInBox(dispX-dispR, dispY-dispR, dispX+dispR, dispY+dispR))
				{
					// We HAVE clicked on DP!
					psReturn = (OBJECT_POSITION *)psPoint;
					//There's no point in checking other object types
					return(psReturn);
				}
			}
		}
// old way, only one point allowed.
//
//		//look throught the list of structures to see if there is a factory
//		//and therefore a DP
//		for (psStructure = apsStructLists[i]; psStructure; psStructure = psStructure->psNext)
//		{
//			if (psStructure->pStructureType->type == REF_FACTORY)
//			{
//				psPoint = ((FACTORY *)psStructure->pFunctionality)->psAssemblyPoint;
//				dispX = psPoint->screenX;
//				dispY = psPoint->screenY;
//				dispR = psPoint->screenR;
//				// Only check DP's that are on screen
//				if (DrawnInLastFrame(psPoint->frameNumber)==TRUE)
//				{
//					if (mouseInBox(dispX-dispR, dispY-dispR, dispX+dispR, dispY+dispR))
//					{
//						// We HAVE clicked on DP!
//						psReturn = psPoint;
//						//There's no point in checking other object types
//						return(psReturn);
//					}
//				}
//			}
//		} // end of checking for droids

	}
	//now check for Proximity Message
	/*for(psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp; psProxDisp =
		psProxDisp->psNext)
	{
		dispX = psProxDisp->screenX;
		dispY = psProxDisp->screenY;
		dispR = psProxDisp->screenR;
		// Only check DP's that are on screen
		if (DrawnInLastFrame(psProxDisp->frameNumber)==TRUE)
		{
			if (mouseInBox(dispX-dispR, dispY-dispR, dispX+dispR, dispY+dispR))
			{
				// We HAVE clicked on Proximity Message!
				psReturn = (OBJECT_POSITION *)psProxDisp;
				//There's no point in checking other object types
				return(psReturn);
			}
		}
	}*/
	return NULL;
}


void dealWithRMB( void )
{
	BASE_OBJECT			*psClickedOn;
	DROID				*psDroid;
	STRUCTURE			*psStructure;
	STRUCTURE			*psSLoop;
	OBJECT_POSITION		*psLocation;

//printf("dealWithRMB %d\n",mouseOverRadar);

	if(driveModeActive()) {
		return;
	}

	if(mouseOverRadar) {
		return;
	}

    //RMB will always cancel the selection of the Las Sat struct
    bLasSatStruct = FALSE;

	/* What have we clicked on? */
	psClickedOn = mouseTarget();
	/* If not NULL, then it's a droid or a structure */
	if(psClickedOn != NULL)
	{
		/* We've got a droid or a structure */
		if(psClickedOn->type == OBJ_DROID )
		{
//			if(radarCheckForHQ(selectedPlayer))	// removed by Jim, well kind of, he asked 19 oct 98
			{
				/* We clicked on droid */
				psDroid = (DROID *) psClickedOn;
				if(psDroid->player == selectedPlayer)
				{
					//ignore RMB on a Transporter - for now?
					if ( (psDroid->droidType != DROID_TRANSPORTER) )
					{
						/* We've clicked on one of our own droids */
		//				addGameMessage("Right clicked on own droid",1000,TRUE);
	//					addConsoleMessage("Right click detected on own droid",DEFAULT_JUSTIFY);

						if(psDroid->selected==TRUE)
						{
		//					psDroid->selected = FALSE;
		//					intObjectSelected(NULL);
							intObjectSelected((BASE_OBJECT *)psDroid);
						}
						else
						{
							clearSelection();
//							psDroid->selected = TRUE;
							SelectDroid(psDroid);
							intObjectSelected((BASE_OBJECT *)psDroid);
						}
					}
                    //it was only just 'for now'!!!
                    else
                    {
                        if (bMultiPlayer)
                        {
                            intResetScreen(FALSE);
						    if(!getWidgetsStatus())
						    {
							    setWidgetsStatus(TRUE);
						    }
						    addTransporterInterface(psDroid, FALSE);
                        }
                    }
				}
				else
				{
					if(bMultiPlayer)
					{
						if(isHumanPlayer(psDroid->player) )
						{
							CONPRINTF(ConsoleString, (ConsoleString,"%s",droidGetName(psDroid)));
							FeedbackClickedOn();
						}
					}
				}

			}

		}	// end if its a droid
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
			psStructure = (STRUCTURE *) psClickedOn;
			if(psStructure->player == selectedPlayer)
			{
				/* We've clicked on our own building */
//				addGameMessage("Right clicked on own building",1000,TRUE);
//				addConsoleMessage("Right clicked on own building",DEFAULT_JUSTIFY);


				if(psStructure->selected==TRUE) {
					psStructure->selected = FALSE;
					intObjectSelected(NULL);
				} else {
// We don't actually wan't to select structures, just inform the interface weve clicked on it,
// might wan't to do this on PC as well as it fixes the problem with the interface locking multiple
// buttons in the object window.
//					clearSelection();
					/* Clear old building selection(s) - should only be one */
					for(psSLoop = apsStructLists[selectedPlayer]; psSLoop; psSLoop = psSLoop->psNext)
					{
						psSLoop->selected = FALSE;
					}

//					psStructure->selected = TRUE;
					//if a factory go to deliv point rather than setting up the interface
					if (StructIsFactory(psStructure))
					{
						setViewPos(((FACTORY *)psStructure->pFunctionality)->
							psAssemblyPoint->coords.x >> TILE_SHIFT,
							((FACTORY *)psStructure->pFunctionality)->psAssemblyPoint->
							coords.y >> TILE_SHIFT,TRUE);
                        //pop up the order interface for the factory - AB 21/04/99 Patch v1.2->
                        intAddFactoryOrder(psStructure);
					}
					else if (psStructure->pStructureType->type == REF_REPAIR_FACILITY)
					{
						setViewPos(((REPAIR_FACILITY *)psStructure->pFunctionality)->
							psDeliveryPoint->coords.x >> TILE_SHIFT,
							((REPAIR_FACILITY *)psStructure->pFunctionality)->psDeliveryPoint->
							coords.y >> TILE_SHIFT,TRUE);
					}
					else
					{
						intObjectSelected((BASE_OBJECT *)psStructure);
					}
				}
			}
		}	// end if its a structure
		/* And if it's not a feature, then we're in trouble! */
		else if (psClickedOn->type != OBJ_FEATURE)
		{
			ASSERT( FALSE,"Weirdy selection from RMB?!" );
		}
	}
	else
	{
		/*Check for a Delivery Point*/
		psLocation = NULL;
		psLocation = checkMouseLoc();
		if (psLocation)
		{
			switch (psLocation->type)
			{
			case POS_DELIVERY:
				if(psLocation->player == selectedPlayer)
				{
					//centre the view on the owning Factory
					psStructure = findDeliveryFactory((FLAG_POSITION *)psLocation);
					if (psStructure)
					{
						setViewPos(psStructure->x >> TILE_SHIFT,
							psStructure->y >> TILE_SHIFT,TRUE);
					}
				}
				break;
			default:
				ASSERT( FALSE, "Unknown type from checkMouseLoc" );
			}
		}
		else
		{
            /* We've just clicked on an area of terrain. A 'send to' operation
            for Transporter in multiPlay mode*/
            /*if (bMultiPlayer)
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
                			psDroid->selected = FALSE;
                        }
                    }
                 }
            }
            else*/
            {
    			clearSelection();
	    		intObjectSelected(NULL);
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

//DBPRINTF(("%d %d\n",mouseTileX,mouseTileY);

	/* We haven't found anything yet */
	retVal = MT_NOTARGET;

	if(driveModeActive() && !driveTacticalActive()) {
		BASE_OBJECT *psObj = targetGetCurrent();
		if(psObj != NULL) {

			if(psObj->type == OBJ_DROID) {
				psDroid = (DROID*)psObj;
				if(psDroid->player == selectedPlayer)
				{
					*ppObjectUnderMouse=psObj;
					// need to check for command droids here as well
					if (psDroid->droidType == DROID_SENSOR)
					{
//	DBPRINTF(("MT_SENSOR\n");
						return MT_SENSOR;
					}
					else if (psDroid->droidType == DROID_TRANSPORTER)
					{
//	DBPRINTF(("MT_TRANDROID\n");
                        //check the transporter is not full
                        if (calcRemainingCapacity(psDroid))
                        {
							return MT_TRANDROID;
                        }
                        else
                        {
                            return MT_BLOCKING;
                        }
					}
					else if (psDroid->droidType == DROID_CONSTRUCT ||
                        psDroid->droidType == DROID_CYBORG_CONSTRUCT)
					{
						return MT_CONSTRUCT;
					}
					else if (psDroid->droidType == DROID_COMMAND)
					{
//	DBPRINTF(("MT_COMMAND\n");
						return MT_COMMAND;
					}
					else
					{
						if (droidIsDamaged(psDroid))
						{
//	DBPRINTF(("MT_OWNDROIDDAM\n");
							return MT_OWNDROIDDAM;
						}
						else
						{
//							return MT_OWNDROID;
						}
					}
				}
				else
				{
//	DBPRINTF(("MT_ENEMYDROID\n");
					return MT_ENEMYDROID;
				}
			}

//			if( (psObj->type == OBJ_DROID) && (psObj->player != selectedPlayer) ) {
//				return MT_ENEMYDROID;
//			}
		}
	} else {
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
						if(psDroid->player == selectedPlayer)
						{
							*ppObjectUnderMouse=(BASE_OBJECT *)psDroid;
							// need to check for command droids here as well
							if (psDroid->droidType == DROID_SENSOR)
							{
								retVal = MT_SENSOR;
							}
							else if (psDroid->droidType == DROID_TRANSPORTER)
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
								retVal = MT_COMMAND;
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
	//printf("Enemy Droid %d %d %d\n",dispX,dispY,dispR);
							retVal = MT_ENEMYDROID;
						}
						/* There's no point in checking other object types */
						return(retVal);
					}
				}
			}
		} // end of checking for droids
	}

	/*	Not a droid, so maybe a structure or feature?
		If still NULL after this then nothing */
	if(driveModeActive() && !driveTacticalActive()) {
		psNotDroid = targetGetCurrent();
	} else {
		psNotDroid = getTileOccupier(mouseTileX, mouseTileY);
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
//printf("Damagable Feature %d %d\n",mouseTileX,mouseTileY);
				retVal = MT_DAMFEATURE;
			}
			else if(((FEATURE *)psNotDroid)->psStats->subType == FEAT_OIL_RESOURCE)
			{
				retVal = MT_RESOURCE;
			}
			else if(((FEATURE *)psNotDroid)->psStats->subType == FEAT_BUILD_WRECK)
			{
				retVal = MT_WRECKFEATURE;
			}
			else
			{
				retVal = MT_BLOCKING;
			}
		}
		else if(psNotDroid->type == OBJ_STRUCTURE)
		{
			psStructure = (STRUCTURE *)psNotDroid;

			if(psNotDroid->player == selectedPlayer)
			{
				if(psStructure->status == SS_BEING_BUILT)
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
				// if factory/powgen/research and not upgraded. make build icon available..
				else if( ( psStructure->pStructureType->type  == REF_FACTORY
						 && ((FACTORY *)psStructure->pFunctionality)->capacity < NUM_FACTORY_MODULES )

					||	 ( psStructure->pStructureType->type  == REF_VTOL_FACTORY
					     && ((FACTORY *)psStructure->pFunctionality)->capacity < NUM_FACTORY_MODULES)

					||	 ( psStructure->pStructureType->type  == REF_POWER_GEN
					     && ((POWER_GEN *)psStructure->pFunctionality)->capacity < NUM_POWER_MODULES)

					||	 ( psStructure->pStructureType->type  == REF_RESEARCH
					     && ((RESEARCH_FACILITY *)psStructure->pFunctionality)->capacity < NUM_RESEARCH_MODULES)
					 )
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
SELECTION_TYPE	establishSelection(UDWORD selectedPlayer)
{
DROID			*psDroid,*psDominant=NULL;
	UBYTE	CurrWeight;
//BOOL			gotWeapon = FALSE;
//DROID			*psWeapDroid = NULL;
BOOL			atLeastOne;
SELECTION_TYPE	selectionClass;

	atLeastOne = FALSE;
	selectionClass = SC_INVALID;
	CurrWeight = UBYTE_MAX;

	for(psDroid = apsDroidLists[selectedPlayer];
			psDroid /*&& !atLeastOne*/; psDroid = psDroid->psNext)
	{
		// This code dos'nt work, what about the case of a selection of DROID_WEAPON types with a
		// DROID_CONSTRUCT type grouped with them,claims to handle this but dos'nt.
//PD		if(psDroid->selected)
//PD		{
//PD			atLeastOne = TRUE;
//PD			if(psDroid->type == DROID_WEAPON)
//PD			{
//PD				gotWeapon = TRUE;
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

			atLeastOne = TRUE;
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
            /* Re-written to allow demolish order to be added to the queuing system
			if ((psDominant->psTarget == NULL && psDominant->psTarStats ==
                (BASE_STATS *) structGetDemolishStat())*/
            if (intDemolishSelectMode())
			{
				selectionClass = SC_DROID_DEMOLISH;			// demolish mode.
			}
			else
			{
				selectionClass = SC_DROID_CONSTRUCT;		// ordinary mode.
			}
			break;

		case DROID_COMMAND:
			selectionClass = SC_DROID_COMMAND;
			break;

		case DROID_REPAIR:
        case DROID_CYBORG_REPAIR:
			selectionClass = SC_DROID_REPAIR;
			break;

		default:
			ASSERT( FALSE,"Weirdy droid type on what you've clicked on!!!" );
			break;

		}
	}
	return(selectionClass);
}

/* Just returns true if the building's present body points aren't 100 percent */
BOOL	buildingDamaged(STRUCTURE *psStructure)
{
	//if( PERCENT(psStructure->body , psStructure->baseBodyPoints ) < 100)
	if( PERCENT(psStructure->body , structureBody(psStructure)) < 100)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

/*Looks through the list of selected players droids to see if one is a repair droid*/
BOOL	repairDroidSelected(UDWORD player)
{
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		//if (psCurr->selected && psCurr->droidType == DROID_REPAIR)
        if (psCurr->selected && (
            psCurr->droidType == DROID_REPAIR ||
            psCurr->droidType == DROID_CYBORG_REPAIR))
		{
			return TRUE;
		}
	}

	//didn't find one...
	return FALSE;

}

/*Looks through the list of selected players droids to see if one is a constructor droid*/
DROID *constructorDroidSelected(UDWORD player)
{
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		//if (psCurr->selected && psCurr->droidType == DROID_CONSTRUCT)
        if (psCurr->selected && (
            psCurr->droidType == DROID_CONSTRUCT ||
            psCurr->droidType == DROID_CYBORG_CONSTRUCT))
		{
			return psCurr;
		}
	}

	//didn't find one...
	return NULL;

}

/*Looks through the list of selected players droids to see if one is a VTOL droid*/
BOOL	vtolDroidSelected(UDWORD player)
{
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->selected && vtolDroid(psCurr))
		{
			// horrible hack to note one of the selected vtols
			psSelectedVtol = psCurr;
			return TRUE;
		}
	}

	//didn't find one...
	return FALSE;

}

/*Looks through the list of selected players droids to see if any is selected*/
BOOL	anyDroidSelected(UDWORD player)
{
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->selected)
		{
			return TRUE;
		}
	}

	//didn't find one...
	return FALSE;

}

/*Looks through the list of selected players droids to see if one is a cyborg droid*/
BOOL cyborgDroidSelected(UDWORD player)
{
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->selected && cyborgDroid(psCurr))
		{
			return TRUE;
		}
	}

	//didn't find one...
	return FALSE;

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
		psCurrDroid->selected = FALSE;
	}
	for(psStruct = apsStructLists[selectedPlayer]; psStruct;
		psStruct = psStruct->psNext)
	{
		psStruct->selected = FALSE;
	}
	//can a feature ever be selected?
	/*for(psFeat = apsFeatureLists[0]; psFeat;
		psFeat = psFeat->psNext)
	{
		psFeat->selected = FALSE;
	}*/
	//clear the Deliv Point if one
	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
		psFlagPos = psFlagPos->psNext)
	{
		psFlagPos->selected = FALSE;
	}

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
	bSensorAssigned = TRUE;
}
