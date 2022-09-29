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
 *  Function definitions for the in game interface code.
 */

#ifndef __INCLUDED_SRC_HCI_H__
#define __INCLUDED_SRC_HCI_H__

#include <list>

#include <functional>
typedef std::function<void (const int)> playerCallbackFunc; // callback function (that receives the player)

#include "lib/framework/types.h"
#include "lib/ivis_opengl/pieclip.h"

#include "message.h"

class MultipleChoiceButton;
class WIDGET;
struct DROID;
struct PROXIMITY_DISPLAY;
struct STRUCTURE;
struct W_SCREEN;
struct iIMDShape;

enum  				  // Reticule button indecies.
{
	RETBUT_CANCEL,
	RETBUT_FACTORY,
	RETBUT_RESEARCH,
	RETBUT_BUILD,
	RETBUT_DESIGN,
	RETBUT_INTELMAP,
	RETBUT_COMMAND,
};

#define BASE_COORDS_X	(640)
#define BASE_COORDS_Y	(460)
#define E_W (pie_GetVideoBufferWidth() - BASE_COORDS_X)
#define E_H (pie_GetVideoBufferHeight() - BASE_COORDS_Y)
#define D_W	((pie_GetVideoBufferWidth() - BASE_COORDS_X)/2)
#define D_H ((pie_GetVideoBufferHeight() - BASE_COORDS_Y)/2)

#define IDRET_FORM				1		// The reticule form
#define IDRET_OPTIONS			2		// option button
#define IDRET_BUILD				3		// build button
#define IDRET_MANUFACTURE		4		// manufacture button
#define IDRET_RESEARCH			5		// research button
#define IDRET_INTEL_MAP			6		// intelligence map button
#define IDRET_DESIGN			7		// design droids button
#define IDRET_CANCEL			8		// central cancel button
#define IDRET_COMMAND			9		// command droid button
#define IDRET_TRANSPORTER		10		// transporter button
#define IDPOW_POWERBAR_T		102		// power bar - trough
#define	IDTRANTIMER_BUTTON		11012	//transporter button on timer display


/* Object screen IDs */

#define IDOBJ_FORM			3000		// The object back form for build/manufacture/research
#define IDOBJ_CLOSE 		3001        // The form for the close button
#define IDOBJ_TABFORM		3500		// The object tab form for build/manufacture/research
#define IDOBJ_GROUP		3502		// The tab for groups

#define IDSTAT_FORM				14000		// The stats form for structure/droid/research type
#define IDSTAT_CLOSE			14003		// The stats close box
#define IDSTAT_TABFORM			14004		// The tab form with the stats buttons
#define IDSTAT_START			14100		// The first stats ID
#define IDSTAT_END				14999		// The last stats ID enough for 899 things
#define IDSTAT_TIMEBARSTART             16205000
#define IDSTAT_TIMEBAREND               16205999
#define IDSTAT_OBSOLETE_BUTTON          4406
#define IDSTAT_PRODSTART		4600
#define IDSTAT_PRODEND			4699

// Reticule position.
#define RET_X				6
#define RET_Y				(324+E_H)
#define RET_FORMWIDTH		132
#define RET_FORMHEIGHT		132

// chat stuff
#define CHAT_CONSOLEBOX	0x4A001
#define CHAT_EDITBOX	0x4A004
#define CHAT_TEAM		0x1
#define CHAT_GLOB		0x2
#define CHAT_CONSOLEBOXX	RET_X + RET_FORMWIDTH + D_W
#define CHAT_CONSOLEBOXY	(1)
#define CHAT_CONSOLEBOXW	240
#define CHAT_CONSOLEBOXH	16

/* Option positions */
#define OPT_GAP			5

// Object screen position. (aka where the factories, research builds show up) [right of command retile]
#define BASE_GAP		6
#define OBJ_BACKX		(RET_X + RET_FORMWIDTH + BASE_GAP + D_W)	// X coord of object screen back form.
#define OBJ_BACKY		RET_Y	// Y coord of object screen back form.
#define OBJ_BACKWIDTH	320	//316		// Width of object screen back form.
#define OBJ_BACKHEIGHT	115		// Height of object screen back form.

#define GROUP_BACKX OBJ_BACKX
#define GROUP_BACKY OBJ_BACKY + 40
#define GROUP_BACKWIDTH OBJ_BACKWIDTH
#define GROUP_BACKHEIGHT OBJ_BACKHEIGHT - 40

/* Build screen positions */
#define OBJ_TABY		6	// Y coord of object screen tab form.
#define OBJ_WIDTH		316	//312//310	// Width of object screen tab form.
#define OBJ_HEIGHT		112	// Height of object screen tab form.
#define OBJ_GAP			2	// Gap between buttons.
#define OBJ_STARTX		2	// Offset of first obj button from left of tab form.
#define OBJ_STARTY		42	//44	// Offset of first obj button from top of tab form.
#define OBJ_STATSTARTY	0

//slider bar positions

#define STAT_SLDX			8	// Slider x.
#define STAT_SLDY			4	// Slider y.

#define STAT_SLDWIDTH		70	// Slider width.

// Power bar position.
#define POW_X			OBJ_BACKX + 12
#define POW_Y			(OBJ_BACKY + OBJ_BACKHEIGHT + 6)
#define POW_BARWIDTH	308

/* close button data */
#define CLOSE_WIDTH		15
#define CLOSE_HEIGHT	15
#define CLOSE_SIZE		15

// Stat screen position. [aka, the 'dialog' of items you research/create]
#define STAT_X			6
#define STAT_Y			(E_H)
#define STAT_WIDTH			RET_FORMWIDTH	// Width of the tab form.
#define STAT_HEIGHT		322			// Height of the tab form.
#define STAT_TABWIDTH		15
#define STAT_TABFORMX		0	// Offset of the tab form within the main form.
#define STAT_TABFORMY		18	// Offset of the tab form within the main form.


// 2 16 bit values packed into a DWORD.
#define PACKDWORD(a,b)	( ( (a)<<16 ) | (b) )
#define UNPACKDWORD_HI(a) ( (a)>>16 )
#define UNPACKDWORD_LOW(a) ( (a) & 0xffff)

// 3 10 bit values packed into a DWORD.
#define PACKDWORD_TRI(a,b,c) ( (((a) & 0x3ff) << 20) | (((b) & 0x3ff) << 10) | ((c) & 0x3ff) )
#define UNPACKDWORD_TRI_A(a) ( ((a)>>20) & 0x3ff )
#define UNPACKDWORD_TRI_B(a) ( ((a)>>10) & 0x3ff )
#define UNPACKDWORD_TRI_C(a) ( (a) & 0x3ff)

#define POWERPOINTS_DROIDDIV	5

#define OBJ_BUTWIDTH		60		// Button width.
#define OBJ_BUTHEIGHT		46		// Button height.

#define OBJ_TEXTX			2

#define OBJ_T1TEXTY			2
#define OBJ_T2TEXTY			14
#define OBJ_T3TEXTY			26
#define OBJ_B1TEXTY			8

#define STAT_SLD_OX	(0)			// Stat window slider offset.
#define STAT_SLD_OY	(0)
#define STAT_SLDSTOPS		10	// Slider number of stops.

#define STAT_PROGBARX			3
#define STAT_PROGBARY			36
#define STAT_PROGBARWIDTH		(OBJ_BUTWIDTH-8)
#define STAT_PROGBARHEIGHT		4
#define STAT_TIMEBARX			3
#define STAT_TIMEBARY			(OBJ_BUTHEIGHT-STAT_PROGBARHEIGHT-3)
#define STAT_POWERBARX			3
#define STAT_POWERBARY			(OBJ_BUTHEIGHT-STAT_PROGBARHEIGHT-6)

/* maximum array sizes */
#define	MAXSTRUCTURES	65535	//bumped up from 80.  NOTE: was used for max # in build menus.
#define	MAXRESEARCH		65535	//was 80 topic displayed   "           "
#define	MAXFEATURES		80
#define	MAXCOMPONENT	200
#define	MAXEXTRASYS		80

enum INTMODE
{
	INT_NORMAL,		// Standard mode (just the reticule)

	INT_OPTION,		// Option screen
	INT_EDIT,		// Edit mode

	INT_EDITSTAT,	// Stat screen up for placing objects
	INT_OBJECT,		// Object screen
	INT_STAT,		// Object screen with stat screen
	INT_CMDORDER,	// Object screen with command droids and orders screen
	INT_DESIGN,		// Design screen
	INT_INTELMAP,	// Intelligence Map
	INT_ORDER,
	INT_INGAMEOP,	// in game options.
	INT_TRANSPORTER, //Loading/unloading a Transporter
	INT_MISSIONRES,	// Results of a mission display.
	INT_MULTIMENU,	// multiplayer only, player stats etc...
	INT_CDCHANGE,		// CD Change message box
	INT_POPUPMSG,	// Adds a popup message to user

	INT_MAXMODE,   //leave as last so we can start the objMode at this value
};

extern INTMODE intMode;

/* Which type of object screen is being displayed. Starting value is where the intMode left off*/
enum OBJECT_MODE
{
	IOBJ_NONE = INT_MAXMODE,	// Nothing doing.
	IOBJ_BUILD,			        // The build screen
	IOBJ_BUILDSEL,		        // Selecting a position for a new structure
	IOBJ_DEMOLISHSEL,	        // Selecting a structure to demolish
	IOBJ_MANUFACTURE,	        // The manufacture screen
	IOBJ_RESEARCH,		        // The research screen
	IOBJ_COMMAND,		        // the command droid screen

	IOBJ_DEBUG_STRUCTURE,
	IOBJ_DEBUG_DROID,

	IOBJ_MAX,			        // maximum object mode,
};

extern OBJECT_MODE objMode;

/* The widget screen */
extern std::shared_ptr<W_SCREEN> psWScreen;

// The last widget ID from widgRunScreen
extern UDWORD			intLastWidget;

/* The current template for the design screen to start with*/
extern std::vector<DROID_TEMPLATE *> apsTemplateList;  ///< Either a list of templates a factory can build or a list of designable templates, for UI use only.
extern std::list<DROID_TEMPLATE> localTemplates;       ///< Unsynchronised list, for UI use only.

/* pointer to hold the imd to use for a new template in the design screen */
extern iIMDShape	*pNewDesignIMD;

/* Initialise the in game interface */
bool intInitialise();

// Check of coordinate is in the build menu
bool CoordInBuild(int x, int y);

/* Shut down the in game interface */
void interfaceShutDown();

/* Return codes for the widget interface */
enum INT_RETVAL
{
	INT_NONE,		// no key clicks have been intercepted
	INT_INTERCEPT,	// key clicks have been intercepted
	INT_QUIT,		// The game should quit
};

void intDoScreenRefresh();

/* Run the widgets for the in game interface */
INT_RETVAL intRunWidgets();

/* Display the widgets for the in game interface */
void intDisplayWidgets();

/* Add the reticule widgets to the widget screen */
bool intAddReticule();
bool intShowGroupSelectionMenu();
bool intAddPower();
void intRemoveReticule();
void setReticuleStats(int ButId, std::string tip = std::string(), std::string filename = std::string(), std::string filenameDown = std::string(), const playerCallbackFunc& callbackFunc = nullptr);
void setReticulesEnabled(bool enabled);
void setReticuleFlash(int ButId, bool flash);

/* Set the map view point to the world coordinates x,y */
void intSetMapPos(UDWORD x, UDWORD y);

/* Set the map view point to the world coordinates x,y */
void intSetMapPos(UDWORD x, UDWORD y);

/* Tell the interface a research facility has completed a topic */
void intResearchFinished(STRUCTURE *psBuilding);
void intAlliedResearchChanged();

/* Sync the interface to an object */
void intObjectSelected(BASE_OBJECT *psObj);

bool intBuildSelectMode();
bool intDemolishSelectMode();

//sets up the Intelligence Screen as far as the interface is concerned
void addIntelScreen();

/* Reset the widget screen to just the reticule */
void intResetScreen(bool NoAnim, bool skipMissionResultScreen = false);

void intScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight);

/* Refresh icons on the interface, without disturbing the layout. i.e. smartreset*/
void intRefreshScreen();

/* Remove the stats widgets from the widget screen */
void intRemoveStats();

/* Remove the stats widgets from the widget screen */
void intRemoveStatsNoAnim();

void intRemoveObjectNoAnim();

/*sets which list of structures to use for the interface*/
STRUCTURE *interfaceStructList();

//sets up the Transporter Screen as far as the interface is concerned
void addTransporterInterface(DROID *psSelected, bool onMission);

/*causes a reticule button to start flashing*/
void flashReticuleButton(UDWORD buttonID);

// stop a reticule button flashing
void stopReticuleButtonFlash(UDWORD buttonID);

//toggles the Power Bar display on and off
void togglePowerBar();
void intShowPowerBar();
void intHidePowerBar();

void intShowWidget(int buttonID);

//hides the power bar from the display - regardless of what player requested!
void forceHidePowerBar(bool forceSetPowerBarUpState = false);

/* Add the Proximity message buttons */
bool intAddProximityButton(PROXIMITY_DISPLAY *psProxDisp, UDWORD inc);

/*Remove a Proximity Button - when the message is deleted*/
void intRemoveProximityButton(PROXIMITY_DISPLAY *psProxDisp);

/* Allows us to fool the widgets with a keypress */
void	setKeyButtonMapping(UDWORD	val);

STRUCTURE *intFindAStructure();
DROID *intGotoNextDroidType(DROID *CurrDroid, DROID_TYPE droidType, bool AllowGroup);

/// Returns the number of researches that selectedPlayer is not already researching, or 0 if there are no free laboratories.
int intGetResearchState();

/// Flashes the button if the research button should flash, and more researches are available to research than before.
/// Stops the button from flashing, if the research button shouldn't flash, and prevState is non-zero.
void intNotifyResearchButton(int prevState);

// see if a reticule button is enabled
bool intCheckReticuleButEnabled(UDWORD id);

//access function for selected object in the interface
BASE_OBJECT *getCurrentSelected();

bool intIsRefreshing();

void intDemolishCancel();

void makeObsoleteButton(const std::shared_ptr<WIDGET> &parent);  ///< Makes a button to toggle showing obsolete items.

void chatDialog(int mode);
bool isChatUp();
bool isSecondaryWindowUp();
void setSecondaryWindowUp(bool value);

void intOpenDebugMenu(OBJECT_TYPE id);

void intStartConstructionPosition(DROID *builder, STRUCTURE_STATS *structure);
void intSetShouldShowRedundantDesign(bool value);
bool intGetShouldShowRedundantDesign();

/* Start looking for a structure location */
void intStartStructPosition(BASE_STATS *psStats);

/* Remove the object widgets from the widget screen */
void intRemoveObject();

#endif // __INCLUDED_SRC_HCI_H__
