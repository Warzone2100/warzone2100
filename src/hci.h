/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#include "lib/ivis_opengl/pieclip.h"
#include "lib/widget/widget.h"

#include "message.h"


#define BASE_COORDS_X	(640)
#define BASE_COORDS_Y	(480)
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
#define IDRET_ORDER				11		// droid order button
#define IDPOW_POWERBAR_T		102		// power bar - trough
#define	IDTRANTIMER_BUTTON		11012	//transporter button on timer display


/* Object screen IDs */

#define IDOBJ_FORM			3000		// The object back form for build/manufacture/research
#define IDOBJ_CLOSE			3001		// The form for the close button
#define IDOBJ_OBJSTART		3002		// The first ID for droids/factories/research
#define IDOBJ_OBJEND            3099            // The last ID for droids/factories/research
#define IDOBJ_STATSTART		3100		// The first ID for stats
#define IDOBJ_STATEND		3199		// The last ID for stats
#define IDOBJ_PROGBARSTART  3200		// The first ID for stats progress bars.
#define IDOBJ_PROGBAREND	3299		// The last ID for stats progress bars.
#define IDOBJ_POWERBARSTART 3300		// The first ID for power bars.
#define IDOBJ_POWERBAREND	3399		// The first ID for power bars.
#define IDOBJ_COUNTSTART	3400		// The first ID for progress number labels.
#define IDOBJ_COUNTEND		3499		// The last ID for progress number labels.
#define IDOBJ_TABFORM		3500		// The object tab form for build/manufacture/research
#define IDOBJ_FACTORYSTART	3600		// The first ID for factory number labels
#define IDOBJ_FACTORYEND	3699		// The last ID for factory number labels
#define IDOBJ_CMDEXPSTART	3700		// The first ID for factory number labels
#define IDOBJ_CMDEXPEND		3749		// The last ID for factory number labels
#define IDOBJ_CMDFACSTART	3750		// The first ID for factory number labels
#define IDOBJ_CMDFACEND		3799		// The last ID for factory number labels
#define IDOBJ_CMDVTOLFACSTART	3800	// The first ID for VTOL factory number labels
#define IDOBJ_CMDVTOLFACEND		3849	// The last ID for VTOL factory number labels


#define IDSTAT_FORM				14000		// The stats form for structure/droid/research type
#define IDSTAT_TITLEFORM		14001		// The form for the close box
#define IDSTAT_LABEL			14002		// Unused
#define IDSTAT_CLOSE			14003		// The stats close box
#define IDSTAT_TABFORM			14004		// The tab form with the stats buttons
#define IDSTAT_START			14100		// The first stats ID
#define IDSTAT_END				14999		// The last stats ID enough for 899 things
#define IDSTAT_TABSCRL_LEFT		15000		// Our left Tab scroll button
#define IDSTAT_TABSCRL_RIGHT	15002		// Our right Tab scroll button
//#define IDSTAT_BARSTART		4200
#define IDSTAT_BAREND			4299
#define IDSTAT_TIMEBARSTART		4300
#define IDSTAT_TIMEBAREND		4399
#define IDSTAT_SLIDER			4400
#define IDSTAT_SLIDERCOUNT		4401
#define IDSTAT_INFINITE_BUTTON	4402
#define IDSTAT_LOOP_BUTTON		4403
#define IDSTAT_LOOP_LABEL		4404
#define IDSTAT_DP_BUTTON		4405
#define IDSTAT_RESICONSTART		4500
#define IDSTAT_RESICONEND		4599
#define IDSTAT_PRODSTART		4600
#define IDSTAT_PRODEND			4699
#define IDSTAT_MANULIMITS		4700

#define IDSTAT_ALLYSTART		4800
#define IDSTAT_ALLYEND			5100

// Reticule position.
#define RET_X				24
#define RET_Y				(324+E_H)
#define RET_FORMWIDTH		132
#define RET_FORMHEIGHT		132

/* Option positions */
#define OPT_GAP			5

// Object screen position.
#define BASE_GAP		6
#define OBJ_BACKX		(RET_X + RET_FORMWIDTH + BASE_GAP + D_W)	// X coord of object screen back form.
#define OBJ_BACKY		RET_Y	// Y coord of object screen back form.
#define OBJ_BACKWIDTH	320	//316		// Width of object screen back form.
#define OBJ_BACKHEIGHT	115		// Height of object screen back form.

/* Build screen positions */
#define OBJ_TABX		2	// X coord of object screen tab form.
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
#define STAT_SLDHEIGHT		12	//4	// Slider height.

// Power bar position.
#define POW_X			OBJ_BACKX + 12
#define POW_Y			(OBJ_BACKY + OBJ_BACKHEIGHT + 6)
#define POW_BARWIDTH	308

#define POW_GAPX		5
#define POW_GAPY		2

//tab details
#define OBJ_TABWIDTH	26
#define OBJ_TABHEIGHT	11
#define	OBJ_TABOFFSET	2
//tab scroll buttons details
#define TABSCRL_HEIGHT	10
#define TABSCRL_WIDTH	7

/* close button data */
#define CLOSE_WIDTH		15
#define CLOSE_HEIGHT	15
#define CLOSE_SIZE		15

// Stat screen position.
#define STAT_X				23
#define STAT_Y				(45 + E_H)
#define STAT_WIDTH			RET_FORMWIDTH	// Width of the tab form.
#define STAT_HEIGHT			273				// Height of the tab form.
#define STAT_TABWIDTH		15
#define STAT_TABHEIGHT		40
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

// 4 8 bit values packed into a DWORD.
#define PACKDWORD_QUAD(a,b,c,d) ( (((a) & 0xff) << 24) | (((b) & 0xff) << 16) | (((c) & 0xff) << 8) | ((d) & 0xff)  )
#define UNPACKDWORD_QUAD_A(a) ( ((a)>>24) & 0xff )
#define UNPACKDWORD_QUAD_B(a) ( ((a)>>16) & 0xff )
#define UNPACKDWORD_QUAD_C(a) ( ((a)>>8) & 0xff )
#define UNPACKDWORD_QUAD_D(a) ( (a) & 0xff)

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
#define	MAXSTRUCTURES	200	//bumped up from 80.  NOTE: was used for max # in build menus.
#define	MAXRESEARCH		200 //was 80 topic displayed   "           "
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

/* The widget screen */
extern W_SCREEN		*psWScreen;

// The last widget ID from widgRunScreen
extern UDWORD			intLastWidget;

/* The button ID of the objects stat when the stat screen is displayed */
extern UDWORD			objStatID;

/* The current template for the design screen to start with*/
extern std::vector<DROID_TEMPLATE *> apsTemplateList;  ///< Either a list of templates a factory can build or a list of designable templates, for UI use only.
extern std::list<DROID_TEMPLATE> localTemplates;       ///< Unsychnronised list, for UI use only.

/*Message View Buffer width and height - MAXIMUM Sizes! - only need to be
as big as Pie View in Research Msg now*/
#define	MSG_BUFFER_WIDTH		INTMAP_PIEWIDTH//DISP_WIDTH//640
#define	MSG_BUFFER_HEIGHT		INTMAP_PIEHEIGHT//DISP_HEIGHT//480

/* pointer to hold the imd to use for a new template in the design screen */
extern iIMDShape	*pNewDesignIMD;

extern bool ClosingMessageView;
extern bool ClosingIntelMap;
extern bool	ClosingTrans;
extern bool	ClosingTransCont;
extern bool	ClosingTransDroids;
extern bool ClosingOrder;

/* Initialise the in game interface */
extern bool intInitialise(void);

// Check of coordinate is in the build menu
extern bool CoordInBuild(int x, int y);

/* Shut down the in game interface */
extern void interfaceShutDown(void);

/* Return codes for the widget interface */
enum INT_RETVAL
{
	INT_NONE,		// no key clicks have been intercepted
	INT_INTERCEPT,	// key clicks have been intercepted
	INT_QUIT,		// The game should quit
};

/* Run the widgets for the in game interface */
extern INT_RETVAL intRunWidgets(void);

/* Display the widgets for the in game interface */
extern void intDisplayWidgets(void);

/* Add the reticule widgets to the widget screen */
extern bool intAddReticule(void);
extern void intRemoveReticule(void);

/* Set the map view point to the world coordinates x,y */
extern void intSetMapPos(UDWORD x, UDWORD y);

/* Set the map view point to the world coordinates x,y */
extern void intSetMapPos(UDWORD x, UDWORD y);

/* Tell the interface when an object is created
 * - it may have to be added to a screen
 */
extern void intNewObj(BASE_OBJECT *psObj);

/* Tell the interface a construction droid has finished building */
extern void intBuildFinished(DROID *psDroid);
/* Tell the interface a construction droid has started building*/
extern void intBuildStarted(DROID *psDroid);
/* Tell the interface a research facility has completed a topic */
extern void intResearchFinished(STRUCTURE *psBuilding);
void intAlliedResearchChanged();
/* Tell the interface a factory has completed building ALL droids */
extern void intManufactureFinished(STRUCTURE *psBuilding);
extern void intUpdateManufacture(STRUCTURE *psBuilding);

/* Sync the interface to an object */
extern void intObjectSelected(BASE_OBJECT *psObj);

// add the construction interface if a constructor droid is selected
extern void intConstructorSelected(DROID *psDroid);
extern bool intBuildSelectMode(void);
extern bool intDemolishSelectMode(void);
extern bool intBuildMode(void);

// add the construction interface if a constructor droid is selected
void intCommanderSelected(DROID *psDroid);

extern UWORD numForms(UDWORD total, UDWORD perForm);

//sets up the Intelligence Screen as far as the interface is concerned
extern void addIntelScreen(void);

/* Reset the widget screen to just the reticule */
extern void intResetScreen(bool NoAnim);

/* Refresh icons on the interface, without disturbing the layout. i.e. smartreset*/
extern void intRefreshScreen(void);

/* Add the options widgets to the widget screen */
extern bool intAddOptions(void);

/* Remove the stats widgets from the widget screen */
extern void intRemoveStats(void);

/* Remove the stats widgets from the widget screen */
extern void intRemoveStatsNoAnim(void);

/*sets which list of structures to use for the interface*/
extern STRUCTURE *interfaceStructList(void);

//sets up the Transporter Screen as far as the interface is concerned
extern void addTransporterInterface(DROID *psSelected, bool onMission);

/*causes a reticule button to start flashing*/
extern void flashReticuleButton(UDWORD buttonID);

// stop a reticule button flashing
extern void stopReticuleButtonFlash(UDWORD buttonID);

//toggles the Power Bar display on and off
extern void togglePowerBar(void);

//displays the Power Bar
extern void intShowPowerBar(void);

//hides the power bar from the display - regardless of what player requested!
extern void forceHidePowerBar(void);

/* Add the Proximity message buttons */
extern bool intAddProximityButton(PROXIMITY_DISPLAY *psProxDisp, UDWORD inc);

/*Remove a Proximity Button - when the message is deleted*/
extern void intRemoveProximityButton(PROXIMITY_DISPLAY *psProxDisp);

/* Allows us to fool the widgets with a keypress */
void	setKeyButtonMapping(UDWORD	val);

STRUCTURE *intFindAStructure(void);
STRUCTURE *intGotoNextStructureType(UDWORD structType, bool JumpTo, bool CancelDrive);
DROID *intGotoNextDroidType(DROID *CurrDroid, DROID_TYPE droidType, bool AllowGroup);

/// Returns the number of researches that selectedPlayer is not already researching, or 0 if there are no free laboratories.
int intGetResearchState();

/// Flashes the button if the research button should flash, and more researches are available to research than before.
/// Stops the button from flashing, if the research button shouldn't flash, and prevState is non-zero.
void intNotifyResearchButton(int prevState);

// see if a reticule button is enabled
extern bool intCheckReticuleButEnabled(UDWORD id);

//access function for selected object in the interface
extern BASE_OBJECT *getCurrentSelected(void);

//initialise all the previous obj - particularly useful for when go Off world!
extern void intResetPreviousObj(void);

extern void HandleClosingWindows(void);

extern bool intIsRefreshing(void);

extern void intDemolishCancel(void);

#endif // __INCLUDED_SRC_HCI_H__
