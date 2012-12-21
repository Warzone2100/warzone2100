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
/**
 * @file hci.c
 *
 * Functions for the in game interface.
 * (Human Computer Interface - thanks to Alex for the file name).
 *
 * Obviously HCI should mean "Hellish Code Incoming" -- Toksyuryel.
 *
 */

#include <string.h>
#include <algorithm>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/stdio_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"
#include "lib/script/script.h"
#include "lib/netplay/netplay.h"

#include "action.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "console.h"
#include "design.h"
#include "display.h"
#include "display3d.h"
#include "drive.h"
#include "edit3d.h"
#include "effects.h"
#include "game.h"
#include "hci.h"
#include "ingameop.h"
#include "intdisplay.h"
#include "intelmap.h"
#include "intorder.h"
#include "keymap.h"
#include "loadsave.h"
#include "loop.h"
#include "mapdisplay.h"
#include "mission.h"
#include "multimenu.h"
#include "multiplay.h"
#include "multigifts.h"
#include "radar.h"
#include "research.h"
#include "scriptcb.h"
#include "scriptextern.h"
#include "scripttabs.h"
#include "transporter.h"
#include "warcam.h"
#include "main.h"
#include "template.h"
#include "wrappers.h"
#include "keybind.h"
#include "qtscript.h"

//#define DEBUG_SCROLLTABS 	//enable to see tab scroll button info for buttons

// Empty edit window
static bool SecondaryWindowUp = false;

#define RETXOFFSET (0)// Reticule button offset
#define RETYOFFSET (0)
#define NUMRETBUTS	7 // Number of reticule buttons.

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

struct BUTSTATE
{
	UDWORD id;
	bool Enabled;
	bool Hidden;
};

struct BUTOFFSET
{
	SWORD x;
	SWORD y;
};

BUTOFFSET ReticuleOffsets[NUMRETBUTS] =  	// Reticule button form relative positions.
{
	{48, 47},	// RETBUT_CANCEL,
	{53, 15},	// RETBUT_FACTORY,
	{87, 33},	// RETBUT_RESEARCH,
	{87, 68},	// RETBUT_BUILD,
	{53, 86},	// RETBUT_DESIGN,
	{19, 68},	// RETBUT_INTELMAP,
	{19, 33},	// RETBUT_COMMAND,
};

BUTSTATE ReticuleEnabled[NUMRETBUTS] =  	// Reticule button enable states.
{
	{IDRET_CANCEL, false, false},
	{IDRET_MANUFACTURE, false, false},
	{IDRET_RESEARCH, false, false},
	{IDRET_BUILD, false, false},
	{IDRET_DESIGN, false, false},
	{IDRET_INTEL_MAP, false, false},
	{IDRET_COMMAND, false, false},
};

static bool ClosingObject = false;
static bool ClosingStats = false;
static UDWORD	keyButtonMapping = 0;
bool ClosingMessageView = false;
bool ClosingIntelMap = false;
bool ClosingOrder = false;
bool ClosingTrans = false;
bool ClosingTransCont = false;
bool ClosingTransDroids = false;
static bool ReticuleUp = false;
bool Refreshing = false;

/***************************************************************************************/
/*                  Widget ID numbers                                                  */

#define IDPOW_FORM			100		// power bar form

/* Option screen IDs */
#define IDOPT_FORM			1000		// The option form
#define IDOPT_CLOSE			1006		// The close button
#define IDOPT_LABEL			1007		// The Option screen label
#define IDOPT_PLAYERFORM	1008		// The player button form
#define IDOPT_PLAYERLABEL	1009		// The player form label
#define IDOPT_PLAYERSTART	1010		// The first player button
#define IDOPT_PLAYEREND		1030		// The last possible player button
#define IDOPT_QUIT			1031		// Quit the game
#define IDOPT_DROID			1037		// The place droid button
#define IDOPT_STRUCT		1038		// The place struct button
#define IDOPT_FEATURE		1039		// The place feature button
#define IDOPT_IVISFORM		1043		// iViS engine form
#define IDOPT_IVISLABEL		1044		// iViS form label

/* Edit screen IDs */
#define IDED_FORM			2000		// The edit form
#define IDED_LABEL			2001		// The edit screen label
#define IDED_CLOSE			2002		// The edit screen close box
#define	IDED_STATFORM		2003		// The edit screen stats form (for droids/structs/features)

#define	IDPROX_START		120000		// The first proximity button
#define	IDPROX_END		129999		// The last proximity button

#define PROX_BUTWIDTH		9
#define PROX_BUTHEIGHT		9
/***************************************************************************************/
/*                  Widget Positions                                                   */

/* Reticule positions */
#define RET_BUTWIDTH		25
#define RET_BUTHEIGHT		28
#define RET_BUTGAPX			6
#define RET_BUTGAPY			6

/* Option positions */
#define OPT_X			(640-300)
#define OPT_Y			20
#define OPT_WIDTH		275
#define OPT_HEIGHT		350
#define OPT_BUTWIDTH	60
#define OPT_BUTHEIGHT	20
#define OPT_MAPY		25
#define OPT_EDITY		50
#define OPT_PLAYERY		150

#define STAT_BUTX				4
#define STAT_BUTY				2

/* Structure type screen positions */
#define STAT_BASEWIDTH 		134	// Size of the main form.
#define STAT_BASEHEIGHT		254
#define STAT_GAP			2
#define STAT_BUTWIDTH		60
#define STAT_BUTHEIGHT		46

/* Close strings */
static char pCloseText[] = "X";

/* Player button strings */
static const char	*apPlayerText[] =
{
	"0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", "10", "11", "12", "13", "14", "15",
};
static const char	*apPlayerTip[] =
{
	"Select Player 0",
	"Select Player 1",
	"Select Player 2",
	"Select Player 3",
	"Select Player 4",
	"Select Player 5",
	"Select Player 6",
	"Select Player 7",
	"Select Player 8",
	"Select Player 9",
	"Select Player 10",
	"Select Player 11",
	"Select Player 12",
	"Select Player 13",
	"Select Player 14",
	"Select Player 15",
};

/* The widget screen */
W_SCREEN		*psWScreen;

// The last widget ID from widgRunScreen
UDWORD				intLastWidget;

INTMODE intMode;

/* Status of the positioning for the object placement */
static enum _edit_pos_mode
{
	IED_NOPOS,
	IED_POS,
} editPosMode;

/* Which type of object screen is being displayed. Starting value is where the intMode left off*/
static enum _obj_mode
{
	IOBJ_NONE = INT_MAXMODE,	// Nothing doing.
	IOBJ_BUILD,			        // The build screen
	IOBJ_BUILDSEL,		        // Selecting a position for a new structure
	IOBJ_DEMOLISHSEL,	        // Selecting a structure to demolish
	IOBJ_MANUFACTURE,	        // The manufacture screen
	IOBJ_RESEARCH,		        // The research screen
	IOBJ_COMMAND,		        // the command droid screen

	IOBJ_MAX,			        // maximum object mode
} objMode;

/* Function type for selecting a base object while building the object screen */
typedef bool (* OBJ_SELECT)(BASE_OBJECT *psObj);

/* Function type for getting the appropriate stats for an object */
typedef BASE_STATS *(* OBJ_GETSTATS)(BASE_OBJECT *psObj);

/* Function type for setting the appropriate stats for an object */
typedef bool (* OBJ_SETSTATS)(BASE_OBJECT *psObj, BASE_STATS *psStats);

/* functions to select and get stats from the current object list */
static OBJ_SELECT		objSelectFunc;
static OBJ_GETSTATS		objGetStatsFunc;
static OBJ_SETSTATS		objSetStatsFunc;

/* Whether the objects that are on the object screen have changed this frame */
static bool				objectsChanged;

/* The current stats list being used by the stats screen */
static BASE_STATS		**ppsStatsList;
static UDWORD			numStatsListEntries;

/* The selected object on the object screen when the stats screen is displayed */
static BASE_OBJECT		*psObjSelected;

/* The button ID of the objects stat when the stat screen is displayed */
UDWORD			objStatID;

/* The button ID of an objects stat on the stat screen if it is locked down */
static UDWORD			statID;

/* The stats for the current getStructPos */
static BASE_STATS		*psPositionStats;

/* The number of tabs on the object form (used by intObjDestroyed to tell whether */
/* the number of tabs has changed).                                             */
static UWORD			objNumTabs;

/* The tab positions of the object form when the structure form is displayed */
static UWORD			objMajor, objMinor;

/* Store a list of stats pointers from the main structure stats */
static STRUCTURE_STATS	**apsStructStatsList;

/* Store a list of research pointers for topics that can be performed*/
static RESEARCH			**ppResearchList;

/* Store a list of Template pointers for Droids that can be built */
std::vector<DROID_TEMPLATE *>   apsTemplateList;
std::list<DROID_TEMPLATE>       localTemplates;

/* Store a list of Feature pointers for features to be placed on the map */
static FEATURE_STATS	**apsFeatureList;

/*Store a list of research indices which can be performed*/
static UWORD			*pList;
static UWORD			*pSList;

/* Store a list of component stats pointers for the design screen */
UDWORD			numComponent;
COMPONENT_STATS	**apsComponentList;
UDWORD			numExtraSys;
COMPONENT_STATS	**apsExtraSysList;

// store the objects that are being used for the object bar
std::vector<BASE_OBJECT *> apsObjectList;


/* The current design being edited on the design screen */
extern DROID_TEMPLATE	sCurrDesign;

/* Flags to check whether the power bars are currently on the screen */
static bool				powerBarUp = false;
static bool				StatsUp = false;
static BASE_OBJECT		*psStatsScreenOwner = NULL;

/* The previous object for each object bar */
static BASE_OBJECT		*apsPreviousObj[IOBJ_MAX];

/* The jump position for each object on the base bar */
static std::vector<Vector2i> asJumpPos;

/***************************************************************************************/
/*              Function Prototypes                                                    */
static bool intUpdateObject(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected, bool bForceStats);
/* Remove the object widgets from the widget screen */
void intRemoveObject(void);
static void intRemoveObjectNoAnim(void);
/* Process the object widgets */
static void intProcessObject(UDWORD id);
/* Get the object refered to by a button ID on the object screen.
 * This works for droid or structure buttons
 */
static BASE_OBJECT *intGetObject(UDWORD id);
/* Reset the stats button for an object */
static void intSetStats(UDWORD id, BASE_STATS *psStats);

/* Add the stats widgets to the widget screen */
/* If psSelected != NULL it specifies which stat should be hilited */
static bool intAddStats(BASE_STATS **ppsStatsList, UDWORD numStats,
        BASE_STATS *psSelected, BASE_OBJECT *psOwner);
/* Process return codes from the stats screen */
static void intProcessStats(UDWORD id);
// clean up when an object dies
static void intObjectDied(UDWORD objID);

/* Add the build widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static bool intAddBuild(DROID *psSelected);
/* Add the manufacture widgets to the widget screen */
/* If psSelected != NULL it specifies which factory should be hilited */
static bool intAddManufacture(STRUCTURE *psSelected);
/* Add the research widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static bool intAddResearch(STRUCTURE *psSelected);
/* Add the command droid widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static bool intAddCommand(DROID *psSelected);

/* Start looking for a structure location */
static void intStartStructPosition(BASE_STATS *psStats);
/* Stop looking for a structure location */
static void intStopStructPosition(void);

static STRUCTURE *CurrentStruct = NULL;
static SWORD CurrentStructType = 0;
static DROID *CurrentDroid = NULL;
static DROID_TYPE CurrentDroidType = DROID_ANY;

/******************Power Bar Stuff!**************/
/* Add the power bars */
static bool intAddPower(void);

/* Set the shadow for the PowerBar */
static void intRunPower(void);

static void intRunStats(void);

/*Deals with the RMB click for the stats screen */
static void intStatsRMBPressed(UDWORD id);

/*Deals with the RMB click for the object screen */
static void intObjectRMBPressed(UDWORD id);

/*Deals with the RMB click for the Object Stats buttons */
static void intObjStatRMBPressed(UDWORD id);

//proximity display stuff
static void processProximityButtons(UDWORD id);

static DROID *intCheckForDroid(UDWORD droidType);
static STRUCTURE *intCheckForStructure(UDWORD structType);

static void intCheckReticuleButtons(void);

// count the number of selected droids of a type
static SDWORD intNumSelectedDroids(UDWORD droidType);


/***************************GAME CODE ****************************/

// Set the x,y members of a button widget initialiser given a reticule button index.
//
static void SetReticuleButPos(UWORD ButId, W_BUTINIT *sButInit)
{
	ASSERT(ButId < NUMRETBUTS, "SetReticuleButPos : Bad button index");

	sButInit->x = (SWORD)(ReticuleOffsets[ButId].x + RETXOFFSET);
	sButInit->y = (SWORD)(ReticuleOffsets[ButId].y + RETYOFFSET);
}

/* Initialise the in game interface */
bool intInitialise(void)
{
	for (int i = 0; i < NUMRETBUTS; i++)
	{
		ReticuleEnabled[i].Hidden = false;
	}

	widgSetTipColour(WZCOL_TOOLTIP_TEXT);

	if (GetGameMode() == GS_NORMAL)
	{
		WidgSetAudio(WidgetAudioCallback, -1, ID_SOUND_SELECT);
	}
	else
	{
		WidgSetAudio(WidgetAudioCallback, -1, ID_SOUND_SELECT);
	}

	/* Create storage for Structures that can be built */
	apsStructStatsList = (STRUCTURE_STATS **)malloc(sizeof(STRUCTURE_STATS *) * MAXSTRUCTURES);

	//create the storage for Research topics - max possible size
	ppResearchList = (RESEARCH **) malloc(sizeof(RESEARCH *) * MAXRESEARCH);

	//create the list for the selected player
	//needs to be UWORD sized for Patches
	pList = (UWORD *) malloc(sizeof(UWORD) * MAXRESEARCH);
	pSList = (UWORD *) malloc(sizeof(UWORD) * MAXRESEARCH);

	/* Create storage for Templates that can be built */
	apsTemplateList.clear();

	/* Create storage for the feature list */
	apsFeatureList = (FEATURE_STATS **)malloc(sizeof(FEATURE_STATS *) * MAXFEATURES);

	/* Create storage for the component list */
	apsComponentList = (COMPONENT_STATS **)malloc(sizeof(COMPONENT_STATS *) * MAXCOMPONENT);

	/* Create storage for the extra systems list */
	apsExtraSysList = (COMPONENT_STATS **)malloc(sizeof(COMPONENT_STATS *) * MAXEXTRASYS);

	// allocate the object list
	apsObjectList.clear();

	intInitialiseGraphics();

	psWScreen = widgCreateScreen();
	if (psWScreen == NULL)
	{
		debug(LOG_ERROR, "intInitialise: Couldn't create widget screen");
		return false;
	}

	widgSetTipFont(psWScreen, font_regular);

	if (GetGameMode() == GS_NORMAL)
	{

		if (!intAddReticule())
		{
			debug(LOG_ERROR, "Couldn't create reticule widgets (Out of memory ?)");
			return false;
		}
		if (!intAddPower())
		{
			debug(LOG_ERROR, "Couldn't create power Bar widget(Out of memory ?)");
			return false;
		}
	}

	/* Initialise the screen to be run */
	widgStartScreen(psWScreen);

	/* Note the current screen state */
	intMode = INT_NORMAL;

	objectsChanged = false;

	// reset the previous objects
	intResetPreviousObj();

	// reset the jump positions
	asJumpPos.clear();

	/* make demolish stat always available */
	if (!bInTutorial)
	{
		for (int comp = 0; comp < numStructureStats; comp++)
		{
			if (asStructureStats[comp].type == REF_DEMOLISH)
			{
				for (int inc = 0; inc < MAX_PLAYERS; inc++)
				{
					apStructTypeLists[inc][comp] = AVAILABLE;
				}
			}
		}
	}

	return true;
}


//initialise all the previous obj - particularly useful for when go Off world!
void intResetPreviousObj(void)
{
	//make sure stats screen doesn't think it should be up
	StatsUp = false;
	// reset the previous objects
	memset(apsPreviousObj, 0, sizeof(apsPreviousObj));
}


/* Shut down the in game interface */
void interfaceShutDown(void)
{
	widgReleaseScreen(psWScreen);

	free(apsStructStatsList);
	free(ppResearchList);
	free(pList);
	free(pSList);
	apsTemplateList.clear();
	free(apsFeatureList);
	free(apsComponentList);
	free(apsExtraSysList);
	apsObjectList.clear();

	psWScreen = NULL;
	apsStructStatsList = NULL;
	ppResearchList = NULL;
	pList = NULL;
	pSList = NULL;
	apsFeatureList = NULL;
	apsComponentList = NULL;
	apsExtraSysList = NULL;

	interfaceDeleteGraphics();

	//obviously!
	ReticuleUp = false;
}

static bool IntRefreshPending = false;

// Set widget refresh pending flag.
//
void intRefreshScreen(void)
{
	IntRefreshPending = true;
}

bool intIsRefreshing(void)
{
	return Refreshing;
}


// see if a delivery point is selected
static FLAG_POSITION *intFindSelectedDelivPoint(void)
{
	FLAG_POSITION *psFlagPos;

	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
	     psFlagPos = psFlagPos->psNext)
	{
		if (psFlagPos->selected && (psFlagPos->type == POS_DELIVERY))
		{
			return psFlagPos;
		}
	}

	return NULL;
}

// Refresh widgets once per game cycle if pending flag is set.
//
static void intDoScreenRefresh(void)
{
	UWORD			objMajor = 0, objMinor = 0, statMajor = 0, statMinor = 0;
	FLAG_POSITION	*psFlag;

	if (IntRefreshPending)
	{
		Refreshing = true;

		if (((intMode == INT_OBJECT) ||
		     (intMode == INT_STAT) ||
		     (intMode == INT_CMDORDER) ||
		     (intMode == INT_ORDER) ||
		     (intMode == INT_TRANSPORTER)) &&
		    (widgGetFromID(psWScreen, IDOBJ_FORM) != NULL) &&
		    !(widgGetFromID(psWScreen, IDOBJ_FORM)->style & WIDG_HIDDEN))
		{
			bool StatsWasUp = false;
			bool OrderWasUp = false;

			// If the stats form is up then remove it, but remember that it was up.
			if ((intMode == INT_STAT) && widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
			{
				StatsWasUp = true;
			}

			// store the current tab position
			if (widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL)
			{
				widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor, &objMinor);
			}
			if (StatsWasUp)
			{
				widgGetTabs(psWScreen, IDSTAT_TABFORM, &statMajor, &statMinor);
			}
			// now make sure the stats screen isn't up
			if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
			{
				intRemoveStatsNoAnim();
			}

			if (psObjSelected &&
			    psObjSelected->died)
			{
				// refresh when unit dies
				psObjSelected = NULL;
				objMajor = objMinor = 0;
				statMajor = statMinor = 0;
			}

			// see if there was a delivery point being positioned
			psFlag = intFindSelectedDelivPoint();

			// see if the commander order screen is up
			if ((intMode == INT_CMDORDER) &&
			    (widgGetFromID(psWScreen, IDORDER_FORM) != NULL))
			{
				OrderWasUp = true;
			}

			switch (objMode)
			{
			case IOBJ_MANUFACTURE:	// The manufacture screen (factorys on bottom bar)
			case IOBJ_RESEARCH:		// The research screen
				//pass in the currently selected object
				intUpdateObject((BASE_OBJECT *)interfaceStructList(), psObjSelected, StatsWasUp);
				break;

			case IOBJ_BUILD:
			case IOBJ_COMMAND:		// the command droid screen
			case IOBJ_BUILDSEL:		// Selecting a position for a new structure
			case IOBJ_DEMOLISHSEL:	// Selecting a structure to demolish
				//pass in the currently selected object
				intUpdateObject((BASE_OBJECT *)apsDroidLists[selectedPlayer], psObjSelected, StatsWasUp);
				break;

			default:
				// generic refresh (trouble at the moment, cant just always pass in a null to addobject
				// if object screen is up, refresh it if stats screen is up, refresh that.
				break;
			}

			// set the tabs again
			if (widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL)
			{
				if (objMajor >= widgGetNumTabMajor(psWScreen, IDOBJ_TABFORM))
				{
					objMajor = 0;	// reset
					objMinor = 0;
					debug(LOG_ERROR, "Reset tabs since objMajor is too big");
				}
				else if (objMinor >= widgGetNumTabMinor(psWScreen, IDOBJ_TABFORM, objMajor))
				{
					objMinor = 0;	// reset minor only
					debug(LOG_ERROR, "Reset minor tabs since objMinor is too big");
				}
				widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor, objMinor);
			}

			if (widgGetFromID(psWScreen, IDSTAT_TABFORM) != NULL)
			{
				if (statMajor >= widgGetNumTabMajor(psWScreen, IDSTAT_TABFORM))
				{
					statMajor = 0;	// reset
					statMinor = 0;
					debug(LOG_ERROR, "Reset tabs since statMajor is too big");
				}
				else if (statMinor >= widgGetNumTabMinor(psWScreen, IDSTAT_TABFORM, statMajor))
				{
					statMinor = 0;	// reset minor only
					debug(LOG_ERROR, "Reset minor tabs since statMinor is too big");
				}
				widgSetTabs(psWScreen, IDSTAT_TABFORM, statMajor, statMinor);
			}

			if (psFlag != NULL)
			{
				// need to restart the delivery point position
				startDeliveryPosition(psFlag);
			}

			// make sure the commander order screen is in the right state
			if ((intMode == INT_CMDORDER) &&
			    !OrderWasUp &&
			    (widgGetFromID(psWScreen, IDORDER_FORM) != NULL))
			{
				intRemoveOrderNoAnim();
				if (statID)
				{
					widgSetButtonState(psWScreen, statID, 0);
				}
			}
		}

		// Refresh the transporter interface.
		intRefreshTransporter();

		// Refresh the order interface.
		intRefreshOrder();

		Refreshing = false;
	}

	IntRefreshPending = false;
}


//hides the power bar from the display
static void intHidePowerBar(void)
{
	//only hides the power bar if the player has requested no power bar
	if (!powerBarUp)
	{
		if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
		{
			widgHide(psWScreen, IDPOW_POWERBAR_T);
		}
	}
}

/* Remove the options widgets from the widget screen */
static void intRemoveOptions(void)
{
	widgDelete(psWScreen, IDOPT_FORM);
}


/* Reset the widget screen to just the reticule */
void intResetScreen(bool NoAnim)
{
	// Ensure driver mode is turned off.
	StopDriverMode();

	if (getWidgetsStatus() == false)
	{
		NoAnim = true;
	}

	if (ReticuleUp)
	{
		/* Reset the reticule buttons */
		widgSetButtonState(psWScreen, IDRET_COMMAND, 0);
		widgSetButtonState(psWScreen, IDRET_BUILD, 0);
		widgSetButtonState(psWScreen, IDRET_MANUFACTURE, 0);
		widgSetButtonState(psWScreen, IDRET_INTEL_MAP, 0);
		widgSetButtonState(psWScreen, IDRET_RESEARCH, 0);
		widgSetButtonState(psWScreen, IDRET_DESIGN, 0);
	}

	/* Remove whatever extra screen was displayed */
	switch (intMode)
	{

	case INT_OPTION:
		intRemoveOptions();
		break;


	case INT_EDITSTAT:
		intStopStructPosition();
		if (NoAnim)
		{
			intRemoveStatsNoAnim();
		}
		else
		{
			intRemoveStats();
		}
		break;

	case INT_OBJECT:
		intStopStructPosition();
		if (NoAnim)
		{
			intRemoveObjectNoAnim();
		}
		else
		{
			intRemoveObject();
		}
		break;

	case INT_STAT:
		if (NoAnim)
		{
			intRemoveStatsNoAnim();
			intRemoveObjectNoAnim();
		}
		else
		{
			intRemoveStats();
			intRemoveObject();
		}
		break;

	case INT_CMDORDER:
		if (NoAnim)
		{
			intRemoveOrderNoAnim();
			intRemoveObjectNoAnim();
		}
		else
		{
			intRemoveOrder();
			intRemoveObject();
		}
		break;

	case INT_ORDER:
		if (NoAnim)
		{
			intRemoveOrderNoAnim();
		}
		else
		{
			intRemoveOrder();
		}
		break;

	case INT_INGAMEOP:
		if (NoAnim)
		{
			intCloseInGameOptionsNoAnim(true);
		}
		else
		{
			intCloseInGameOptions(false, true);
		}
		break;

	case INT_MISSIONRES:
		intRemoveMissionResultNoAnim();
		break;


	case INT_MULTIMENU:
		if (NoAnim)
		{
			intCloseMultiMenuNoAnim();
		}
		else
		{
			intCloseMultiMenu();
		}
		break;


	case INT_DESIGN:
		intRemoveDesign();
		intHidePowerBar();

		if (bInTutorial)
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_QUIT);
		}

		break;

	case INT_INTELMAP:
		if (NoAnim)
		{
			intRemoveIntelMapNoAnim();
		}
		else
		{
			intRemoveIntelMap();
		}
		intHidePowerBar();

		if (!bMultiPlayer)
		{
			gameTimeStart();
		}

		break;

	case INT_TRANSPORTER:
		if (NoAnim)
		{
			intRemoveTransNoAnim();
		}
		else
		{
			intRemoveTrans();
		}
		break;
	default:
		break;
	}
	SecondaryWindowUp = false;
	intMode = INT_NORMAL;
	//clearSel() sets IntRefreshPending = true by calling intRefreshScreen() but if we're doing this then we won't need to refresh - hopefully!
	IntRefreshPending = false;
}


// calulate the center world coords for a structure stat given
// top left tile coords
static void intCalcStructCenter(STRUCTURE_STATS *psStats, UDWORD tilex, UDWORD tiley, uint16_t direction, UDWORD *pcx, UDWORD *pcy)
{
	unsigned width  = getStructureStatsWidth(psStats, direction) * TILE_UNITS;
	unsigned height = getStructureStatsBreadth(psStats, direction) * TILE_UNITS;

	*pcx = tilex * TILE_UNITS + width / 2;
	*pcy = tiley * TILE_UNITS + height / 2;
}


/* Process return codes from the Options screen */
static void intProcessOptions(UDWORD id)
{
	if (id >= IDOPT_PLAYERSTART && id <= IDOPT_PLAYEREND)
	{
		int oldSelectedPlayer = selectedPlayer;

		widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, 0);
		oldSelectedPlayer = selectedPlayer;
		selectedPlayer = id - IDOPT_PLAYERSTART;
		NetPlay.players[selectedPlayer].allocated = !NetPlay.players[selectedPlayer].allocated;
		NetPlay.players[oldSelectedPlayer].allocated = !NetPlay.players[oldSelectedPlayer].allocated;
		// Do not change realSelectedPlayer here, so game doesn't pause.
		widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, WBUT_LOCK);
	}
	else
	{
		switch (id)
		{
			/* The add object buttons */
		case IDOPT_DROID:
			intRemoveOptions();
			apsTemplateList.clear();
			for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
			{
				apsTemplateList.push_back(&*i);
			}
			ppsStatsList = (BASE_STATS **)&apsTemplateList[0]; // FIXME Ugly cast, and is undefined behaviour (strict-aliasing violation) in C/C++.
			objMode = IOBJ_MANUFACTURE;
			intAddStats(ppsStatsList, apsTemplateList.size(), NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
			break;
		case IDOPT_STRUCT:
			intRemoveOptions();
			for (unsigned i = 0; i < std::min<unsigned>(numStructureStats, MAXSTRUCTURES); ++i)
			{
				apsStructStatsList[i] = asStructureStats + i;
			}
			ppsStatsList = (BASE_STATS **)apsStructStatsList;
			objMode = IOBJ_BUILD;
			intAddStats(ppsStatsList, std::min<unsigned>(numStructureStats, MAXSTRUCTURES), NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
			break;
		case IDOPT_FEATURE:
			intRemoveOptions();
			for (unsigned i = 0; i < std::min<unsigned>(numFeatureStats, MAXFEATURES); ++i)
			{
				apsFeatureList[i] = asFeatureStats + i;
			}
			ppsStatsList = (BASE_STATS **)apsFeatureList;
			intAddStats(ppsStatsList, std::min<unsigned>(numFeatureStats, MAXFEATURES), NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
			break;
			/* Close window buttons */
		case IDOPT_CLOSE:
			intRemoveOptions();
			intMode = INT_NORMAL;
			break;
			/* Ignore these */
		case IDOPT_FORM:
		case IDOPT_LABEL:
		case IDOPT_PLAYERFORM:
		case IDOPT_PLAYERLABEL:
		case IDOPT_IVISFORM:
		case IDOPT_IVISLABEL:
			break;
		default:
			ASSERT(false, "Unknown return code");
			break;
		}
	}
}


/* Process return codes from the object placement stats screen */
static void intProcessEditStats(UDWORD id)
{
	if (id >= IDSTAT_START && id <= IDSTAT_END)
	{
		/* Clicked on a stat button - need to look for a location for it */
		psPositionStats = ppsStatsList[id - IDSTAT_START];
		if (psPositionStats->ref >= REF_TEMPLATE_START &&
		    psPositionStats->ref < REF_TEMPLATE_START + REF_RANGE)
		{
			FLAG_POSITION debugMenuDroidDeliveryPoint;
			// Placing a droid from the debug menu, set up the flag. (This would probably be safe to do, even if we're placing something else.)
			debugMenuDroidDeliveryPoint.factoryType = REPAIR_FLAG;
			debugMenuDroidDeliveryPoint.factoryInc = 0;
			debugMenuDroidDeliveryPoint.player = selectedPlayer;
			startDeliveryPosition(&debugMenuDroidDeliveryPoint);
		}
		else
		{
			intStartStructPosition(psPositionStats);
		}
		editPosMode = IED_POS;
	}
	else if (id == IDSTAT_CLOSE)
	{
		intRemoveStats();
		intStopStructPosition();
		intMode = INT_NORMAL;
		objMode = IOBJ_NONE;
	}
	else if (id == IDSTAT_TABSCRL_LEFT)	//user hit left scroll tab from DEBUG menu.
	{
		W_TABFORM	*psTForm;
		int temp;
#ifdef  DEBUG_SCROLLTABS
		char buf[200];		//only used for debugging
#endif
		psTForm = (W_TABFORM *)widgGetFromID(psWScreen, IDSTAT_TABFORM);	//get our form
		psTForm->TabMultiplier -= 1;				// -1 since user hit left button
		if (psTForm->TabMultiplier < 1)
		{
			psTForm->TabMultiplier = 1;			//Must be at least 1.
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		}
		// add routine to update tab widgets now...
		temp = psTForm->majorT;					//set tab # to previous "page"
		temp -= TAB_SEVEN;						//7 = 1 "page" of tabs
		if (temp < 0)
		{
			psTForm->majorT = 0;
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		}
		else
		{
			psTForm->majorT = temp;
		}
#ifdef  DEBUG_SCROLLTABS
		sprintf(buf, "[debug menu]Clicked LT %d tab #=%d", psTForm->TabMultiplier, psTForm->majorT);
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
#endif
	}
	else if (id == IDSTAT_TABSCRL_RIGHT) // user hit right scroll tab from DEBUG menu
	{
		W_TABFORM	*psTForm;
		UWORD numTabs;
#ifdef  DEBUG_SCROLLTABS
		char buf[200];					// only used for debugging.
#endif
		psTForm = (W_TABFORM *)widgGetFromID(psWScreen, IDSTAT_TABFORM);
		numTabs = numForms(psTForm->numStats, psTForm->numButtons);
		numTabs = ((numTabs / TAB_SEVEN) + 1);	// (Total tabs needed / 7(max tabs that fit))+1
		psTForm->TabMultiplier += 1;
		if (psTForm->TabMultiplier > numTabs)			// add 'Bzzt' sound effect?
		{
			psTForm->TabMultiplier -= 1;					// to signify past max?
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		}
		psTForm->majorT += TAB_SEVEN;					// set tab # to next "page"
		if (psTForm->majorT >= psTForm->numMajor)
		{
			psTForm->majorT = psTForm->numMajor - 1;		//set it back to max -1
		}
#ifdef  DEBUG_SCROLLTABS		//for debuging
		sprintf(buf, "[debug menu]Clicked RT %d numtabs %d tab # %d", psTForm->TabMultiplier, numTabs, psTForm->majorT);
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
#endif
	}
}

/* Run the widgets for the in game interface */
INT_RETVAL intRunWidgets(void)
{
	UDWORD			retID;

	INT_RETVAL		retCode;
	bool			quitting = false;
	UDWORD			structX, structY, structX2, structY2;
	UWORD			objMajor, objMinor;
	STRUCTURE		*psStructure;
	DROID			*psDroid;
	SDWORD			i;
	UDWORD			widgOverID;

	intDoScreenRefresh();

	/* Update the object list if necessary */
	if (intMode == INT_OBJECT || intMode == INT_STAT || intMode == INT_CMDORDER)
	{
		// see if there is a dead object in the list
		for (unsigned i = 0; i < apsObjectList.size(); ++i)
		{
			if (apsObjectList[i] && apsObjectList[i]->died)
			{
				intObjectDied((UDWORD)(i + IDOBJ_OBJSTART));
				apsObjectList[i] = NULL;
			}
		}
	}

	/* Update the previous object array */
	for (i = 0; i < IOBJ_MAX; i++)
	{
		if (apsPreviousObj[i] && apsPreviousObj[i]->died)
		{
			apsPreviousObj[i] = NULL;
		}
	}

	/* if objects in the world have changed, may have to update the interface */
	if (objectsChanged)
	{
		/* The objects on the object screen have changed */
		if (intMode == INT_OBJECT)
		{
			ASSERT(widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL, "No object form");

			/* Remove the old screen */
			widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor, &objMinor);
			intRemoveObject();

			/* Add the new screen */
			switch (objMode)
			{
			case IOBJ_BUILD:
			case IOBJ_BUILDSEL:
				intAddBuild(NULL);
				break;
			case IOBJ_MANUFACTURE:
				intAddManufacture(NULL);
				break;
			case IOBJ_RESEARCH:
				intAddResearch(NULL);
				break;
			default:
				break;
			}

			/* Reset the tabs on the object screen */
			if (objMajor > objNumTabs)
			{
				widgSetTabs(psWScreen, IDOBJ_TABFORM, objNumTabs, objMinor);
			}
			else
			{
				widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor, objMinor);
			}
		}
		else if (intMode == INT_STAT)
		{
			/* Need to get the stats screen to update as well */
		}
	}
	objectsChanged = false;

	if (bLoadSaveUp && runLoadSave(true) && strlen(sRequestResult) > 0)
	{
		if (bRequestLoad)
		{
			NET_InitPlayers();	// reinitialize starting positions
			loopMissionState = LMS_LOADGAME;
			sstrcpy(saveGameName, sRequestResult);
		}
		else
		{
			if (saveGame(sRequestResult, GTYPE_SAVE_START))
			{
				char msg[256] = {'\0'};

				sstrcpy(msg, _("GAME SAVED: "));
				sstrcat(msg, saveGameName);
				addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);

				if (widgGetFromID(psWScreen, IDMISSIONRES_SAVE))
				{
					widgDelete(psWScreen, IDMISSIONRES_SAVE);
				}
			}
			else
			{
				ASSERT(false, "intRunWidgets: saveGame Failed");
				deleteSaveGame(sRequestResult);
			}
		}
	}

	if (MissionResUp)
	{
		intRunMissionResult();
	}

	/* Run the current set of widgets */
	if (!bLoadSaveUp)
	{
		retID = widgRunScreen(psWScreen);
	}
	else
	{
		retID = 0;
	}

	/* We may need to trigger widgets with a key press */
	if (keyButtonMapping)
	{
		/* Set the appropriate id */
		retID = keyButtonMapping;

		/* Clear it so it doesn't trigger next time around */
		keyButtonMapping = 0;
	}

	intLastWidget = retID;
	if (bInTutorial && retID != 0)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BUTTON_PRESSED);
	}

	/* Extra code for the power bars to deal with the shadow */
	if (powerBarUp)
	{
		intRunPower();
	}

	if (StatsUp)
	{
		intRunStats();
	}

	if (OrderUp)
	{
		intRunOrder();
	}

	if (MultiMenuUp)
	{
		intRunMultiMenu();
	}

	if (retID >= IDPROX_START && retID <= IDPROX_END)
	{
		processProximityButtons(retID);
		return INT_NONE;
	}

	/* Extra code for the design screen to deal with the shadow bar graphs */
	if (intMode == INT_DESIGN)
	{
		SecondaryWindowUp = true;
		intRunDesign();
	}

	/* Deal with any clicks */
	switch (retID)
	{
	case 0:
		/* default return value */
		break;
		/*****************  Reticule buttons  *****************/

	case IDRET_OPTIONS:
		intResetScreen(false);
		(void)intAddOptions();
		intMode = INT_OPTION;
		break;

	case IDRET_COMMAND:
		intResetScreen(false);
		widgSetButtonState(psWScreen, IDRET_COMMAND, WBUT_CLICKLOCK);
		intAddCommand(NULL);
		break;

	case IDRET_BUILD:
		intResetScreen(true);
		widgSetButtonState(psWScreen, IDRET_BUILD, WBUT_CLICKLOCK);
		if (editMode)
		{
			intProcessOptions(IDOPT_STRUCT);
		}
		else
		{
			(void)intAddBuild(NULL);
		}
		break;

	case IDRET_MANUFACTURE:
		intResetScreen(true);
		widgSetButtonState(psWScreen, IDRET_MANUFACTURE, WBUT_CLICKLOCK);
		if (editMode)
		{
			intProcessOptions(IDOPT_DROID);
		}
		else
		{
			(void)intAddManufacture(NULL);
		}
		break;

	case IDRET_RESEARCH:
		intResetScreen(true);
		widgSetButtonState(psWScreen, IDRET_RESEARCH, WBUT_CLICKLOCK);
		(void)intAddResearch(NULL);
		break;

	case IDRET_INTEL_MAP:
		// check if RMB was clicked
		if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
		{
			//set the current message to be the last non-proximity message added
			setCurrentMsg();
			setMessageImmediate(true);
		}
		else
		{
			psCurrentMsg = NULL;
		}
		addIntelScreen();
		break;

	case IDRET_DESIGN:
		intResetScreen(true);
		widgSetButtonState(psWScreen, IDRET_DESIGN, WBUT_CLICKLOCK);
		/*add the power bar - for looks! */
		intShowPowerBar();
		(void)intAddDesign(false);
		intMode = INT_DESIGN;
		break;

	case IDRET_CANCEL:
		intResetScreen(false);
		psCurrentMsg = NULL;
		break;

		/*Transporter button pressed - OFFWORLD Mission Maps ONLY *********/
	case IDTRANTIMER_BUTTON:
		addTransporterInterface(NULL, true);
		break;

	case IDTRANS_LAUNCH:
		processLaunchTransporter();
		break;

		/* Catch the quit button here */
	case INTINGAMEOP_POPUP_QUIT:
	case IDMISSIONRES_QUIT:			// mission quit
	case INTINGAMEOP_QUIT_CONFIRM:			// esc quit confrim
	case IDOPT_QUIT:						// options screen quit
		intResetScreen(false);
		quitting = true;
		break;

		// Process form tab clicks.
	case IDOBJ_TABFORM:		// If tab clicked on in object screen then refresh all rendered buttons.
		RefreshObjectButtons();
		RefreshTopicButtons();
		break;

	case IDSTAT_TABFORM:	// If tab clicked on in stats screen then refresh all rendered buttons.
		RefreshStatsButtons();
		break;

	case IDDES_TEMPLFORM:	// If tab clicked on in design template screen then refresh all rendered buttons.
		RefreshStatsButtons();
		break;

	case IDDES_COMPFORM:	// If tab clicked on in design component screen then refresh all rendered buttons.
		RefreshObjectButtons();
		RefreshSystem0Buttons();
		break;

		/* Default case passes remaining IDs to appropriate function */
	default:
		switch (intMode)
		{
		case INT_OPTION:
			intProcessOptions(retID);
			break;
		case INT_EDITSTAT:
			intProcessEditStats(retID);
			break;
		case INT_STAT:
		case INT_CMDORDER:
			/* In stat mode ids get passed to processObject
			 * and then through to processStats
			 */
			// NO BREAK HERE! THIS IS CORRECT;
		case INT_OBJECT:
			intProcessObject(retID);
			break;
		case INT_ORDER:
			intProcessOrder(retID);
			break;
		case INT_MISSIONRES:
			intProcessMissionResult(retID);
			break;
		case INT_INGAMEOP:
			intProcessInGameOptions(retID);
			break;
		case INT_MULTIMENU:
			intProcessMultiMenu(retID);
			break;
		case INT_DESIGN:
			intProcessDesign(retID);
			break;
		case INT_INTELMAP:
			intProcessIntelMap(retID);
			break;
		case INT_TRANSPORTER:
			intProcessTransporter(retID);
			break;
		case INT_NORMAL:
			break;
		default:
			ASSERT(false, "intRunWidgets: unknown interface mode");
			break;
		}
		break;
	}

	if (!quitting && !retID)
	{
		if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL)
		{
			// See if a position for the structure has been found
			if (found3DBuildLocTwo(&structX, &structY, &structX2, &structY2))
			{
				// check if it's a straight line.
				if ((structX == structX2) || (structY == structY2))
				{
					// Send the droid off to build the structure assuming the droid
					// can get to the location chosen
					structX = world_coord(structX) + TILE_UNITS / 2;
					structY = world_coord(structY) + TILE_UNITS / 2;
					structX2 = world_coord(structX2) + TILE_UNITS / 2;
					structY2 = world_coord(structY2) + TILE_UNITS / 2;

					// Set the droid order
					if (intNumSelectedDroids(DROID_CONSTRUCT) == 0
					    && intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0
					    && psObjSelected != NULL && isConstructionDroid(psObjSelected))
					{
						orderDroidStatsTwoLocDir((DROID *)psObjSelected, DORDER_LINEBUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, structX2, structY2, player.r.y, ModeQueue);
					}
					else
					{
						orderSelectedStatsTwoLocDir(selectedPlayer, DORDER_LINEBUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, structX2, structY2, player.r.y, ctrlShiftDown());
					}
				}
				if (!quickQueueMode)
				{
					// Clear the object screen, only if we aren't immediately building something else
					intResetScreen(false);
				}

			}
			else if (found3DBuilding(&structX, &structY))	//found building
			{
				//check droid hasn't died
				if ((psObjSelected == NULL) ||
				    !psObjSelected->died)
				{
					bool CanBuild = true;

					// Send the droid off to build the structure assuming the droid
					// can get to the location chosen
					intCalcStructCenter((STRUCTURE_STATS *)psPositionStats, structX, structY, player.r.y, &structX, &structY);

					// Don't allow derrick to be built on burning ground.
					if (((STRUCTURE_STATS *)psPositionStats)->type == REF_RESOURCE_EXTRACTOR)
					{
						if (fireOnLocation(structX, structY))
						{
							AddDerrickBurningMessage();
						}
					}
					if (CanBuild)
					{
						// Set the droid order
						if (intNumSelectedDroids(DROID_CONSTRUCT) == 0
						    && intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0
						    && psObjSelected != NULL)
						{
							orderDroidStatsLocDir((DROID *)psObjSelected, DORDER_BUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, player.r.y, ModeQueue);
						}
						else
						{
							orderSelectedStatsLocDir(selectedPlayer, DORDER_BUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, player.r.y, ctrlShiftDown());
						}
					}
				}

				if (!quickQueueMode)
				{
					// Clear the object screen, only if we aren't immediately building something else
					intResetScreen(false);
				}
			}
			if (buildState == BUILD3D_NONE)
			{
				intResetScreen(false);
			}
		}
		else if (intMode == INT_EDITSTAT && editPosMode == IED_POS)
		{
			/* Directly positioning some type of object */
			unsigned structX1 = INT32_MAX;
			unsigned structY1 = INT32_MAX;
			FLAG_POSITION flag;
			structX2 = INT32_MAX - 1;
			structY2 = INT32_MAX - 1;
			if (sBuildDetails.psStats && (found3DBuilding(&structX1, &structY1) || found3DBuildLocTwo(&structX1, &structY1, &structX2, &structY2)))
			{
				if (structX2 == INT32_MAX - 1)
				{
					structX2 = structX1;
					structY2 = structY1;
				}
				if (structX1 > structX2)
				{
					std::swap(structX1, structX2);
				}
				if (structY1 > structY2)
				{
					std::swap(structY1, structY2);
				}
			}
			else if (deliveryReposFinished(&flag))
			{
				structX2 = structX1 = map_coord(flag.coords.x);
				structY2 = structY1 = map_coord(flag.coords.y);
			}

			for (unsigned j = structY1; j <= structY2; ++j)
				for (unsigned i = structX1; i <= structX2; ++i)
				{
					structX = i;
					structY = j;
					/* See what type of thing is being put down */
					if (psPositionStats->ref >= REF_STRUCTURE_START
					    && psPositionStats->ref < REF_STRUCTURE_START + REF_RANGE)
					{
						STRUCTURE_STATS *psBuilding = (STRUCTURE_STATS *)psPositionStats;
						STRUCTURE tmp(0, selectedPlayer);

						intCalcStructCenter(psBuilding, structX, structY, player.r.y, &structX, &structY);
						if (psBuilding->type == REF_DEMOLISH)
						{
							MAPTILE *psTile = mapTile(map_coord(structX), map_coord(structY));
							FEATURE *psFeature = (FEATURE *)psTile->psObject;
							psStructure = (STRUCTURE *)psTile->psObject; /* reuse var */

							if (psStructure && psTile->psObject->type == OBJ_STRUCTURE)
							{
								//removeStruct(psStructure, true);
								SendDestroyStructure(psStructure);
							}
							else if (psFeature && psTile->psObject->type == OBJ_FEATURE)
							{
								removeFeature(psFeature);
							}
							psStructure = NULL;
						}
						else
						{
							psStructure = &tmp;
							tmp.id = generateNewObjectId();
							tmp.pStructureType = (STRUCTURE_STATS *)psPositionStats;
							tmp.pos.x = structX;
							tmp.pos.y = structY;
							tmp.pos.z = map_Height(structX, structY) + world_coord(1) / 10;
							const char *msg;

							// In multiplayer games be sure to send a message to the
							// other players, telling them a new structure has been
							// placed.
							SendBuildFinished(psStructure);
							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new
							// structure.
							sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new structure: %s."),
							        selectedPlayer, psStructure->pStructureType->pName);
							sendTextMessage(msg, true);
							Cheated = true;
						}
					}
					else if (psPositionStats->ref >= REF_FEATURE_START && psPositionStats->ref < REF_FEATURE_START + REF_RANGE)
					{
						const char *msg;

						// Send a text message to all players, notifying them of the fact that we're cheating ourselves a new feature.
						sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new feature: %s."),
						        selectedPlayer, psPositionStats->pName);
						sendTextMessage(msg, true);
						Cheated = true;
						// Notify the other hosts that we've just built ourselves a feature
						//sendMultiPlayerFeature(result->psStats->subType, result->pos.x, result->pos.y, result->id);
						sendMultiPlayerFeature(((FEATURE_STATS *)psPositionStats)->subType, world_coord(structX), world_coord(structY), generateNewObjectId());
					}
					else if (psPositionStats->ref >= REF_TEMPLATE_START &&
					        psPositionStats->ref < REF_TEMPLATE_START + REF_RANGE)
					{
						const char *msg;
						psDroid = buildDroid((DROID_TEMPLATE *)psPositionStats,
						        world_coord(structX) + TILE_UNITS / 2, world_coord(structY) + TILE_UNITS / 2,
						        selectedPlayer, false, NULL);
						cancelDeliveryRepos();
						if (psDroid)
						{
							addDroid(psDroid, apsDroidLists);

							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new droid.
							sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new droid: %s."), selectedPlayer, psDroid->aName);

							psScrCBNewDroid = psDroid;
							psScrCBNewDroidFact = NULL;
							eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NEWDROID);	// notify scripts so it will get assigned jobs
							psScrCBNewDroid = NULL;

							triggerEventDroidBuilt(psDroid, NULL);
						}
						else
						{
							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new droid.
							sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new droid."), selectedPlayer);
						}
						sendTextMessage(msg, true);
						Cheated = true;
					}
					if (!quickQueueMode)
					{
						editPosMode = IED_NOPOS;
					}
				}
		}
	}

	widgOverID = widgGetMouseOver(psWScreen);

	retCode = INT_NONE;
	if (quitting)
	{
		retCode = INT_QUIT;
	}
	else if (retID || intMode == INT_EDIT || intMode == INT_MISSIONRES || widgOverID != 0)
	{
		retCode = INT_INTERCEPT;
	}

	if ((testPlayerHasLost() || testPlayerHasWon()) && !bMultiPlayer && intMode != INT_MISSIONRES && !getDebugMappingStatus())
	{
		debug(LOG_ERROR, "PlayerHasLost Or Won");
		intResetScreen(true);
		retCode = INT_QUIT;
	}
	return retCode;
}

/* Set the shadow for the PowerBar */
static void intRunPower(void)
{
	UDWORD				statID;
	BASE_STATS			*psStat;
	UDWORD				quantity = 0;
	RESEARCH			*psResearch;

	/* Find out which button was hilited */
	statID = widgGetMouseOver(psWScreen);
	if (statID >= IDSTAT_START && statID <= IDSTAT_END)
	{
		psStat = ppsStatsList[statID - IDSTAT_START];
		if (psStat->ref >= REF_STRUCTURE_START && psStat->ref <
		    REF_STRUCTURE_START + REF_RANGE)
		{
			//get the structure build points
			quantity = ((STRUCTURE_STATS *)apsStructStatsList[statID -
			        IDSTAT_START])->powerToBuild;
		}
		else if (psStat->ref >= REF_TEMPLATE_START &&
		        psStat->ref < REF_TEMPLATE_START + REF_RANGE)
		{
			//get the template build points
			quantity = calcTemplatePower(apsTemplateList[statID - IDSTAT_START]);
		}
		else if (psStat->ref >= REF_RESEARCH_START &&
		        psStat->ref < REF_RESEARCH_START + REF_RANGE)
		{
			//get the research points
			psResearch = (RESEARCH *)ppResearchList[statID - IDSTAT_START];

			// has research been not been canceled
			int rindex = psResearch->index;
			if (IsResearchCancelled(&asPlayerResList[selectedPlayer][rindex]) == 0)
			{
				quantity = ((RESEARCH *)ppResearchList[statID - IDSTAT_START])->researchPower;
			}
		}

		//update the power bars
		intSetShadowPower(quantity);
	}
	else
	{
		intSetShadowPower(0);
	}
}


// Process stats screen.
static void intRunStats(void)
{
	BASE_OBJECT			*psOwner;
	STRUCTURE			*psStruct;
	FACTORY				*psFactory;

	if (intMode != INT_EDITSTAT && objMode == IOBJ_MANUFACTURE)
	{
		psOwner = (BASE_OBJECT *)widgGetUserData(psWScreen, IDSTAT_LOOP_LABEL);
		ASSERT(psOwner->type == OBJ_STRUCTURE, "intRunStats: Invalid object type");

		psStruct = (STRUCTURE *)psOwner;
		ASSERT(StructIsFactory(psStruct), "intRunStats: Invalid Structure type");

		psFactory = (FACTORY *)psStruct->pFunctionality;
		//adjust the loop button if necessary
		if (psFactory->psSubject != NULL && psFactory->productionLoops != 0)
		{
			widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
		}
	}
}


/* Add the stats screen for a given object */
static void intAddObjectStats(BASE_OBJECT *psObj, UDWORD id)
{
	BASE_STATS		*psStats;
	UWORD			statMajor = 0, statMinor = 0, newStatMajor, newStatMinor;
	UDWORD			i, j, index;
	UDWORD			count;
	SDWORD			iconNumber, entryIN;
	W_TABFORM	    *psForm;

	/* Clear a previous structure pos if there is one */
	intStopStructPosition();

	/* Get the current tab pos */
	widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor, &objMinor);

	/* if there is already a stats form up, remove it */
	statMajor = statMinor = 0;

	// Store the tab positions.
	if (intMode == INT_STAT)
	{
		if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
		{
			widgGetTabs(psWScreen, IDSTAT_TABFORM, &statMajor, &statMinor);
		}
		intRemoveStatsNoAnim();
	}

	/* Display the stats window
	 *  - restore the tab position if there is no stats selected
	 */
	psStats = objGetStatsFunc(psObj);

	// note the object for the screen
	apsPreviousObj[objMode] = psObj;

	// NOTE! The below functions populate our list (building/units...)
	// up to MAX____.  We have unlimited potential, but it is capped at 200 now.
	//determine the Structures that can be built
	if (objMode == IOBJ_BUILD)
	{
		numStatsListEntries = fillStructureList(apsStructStatsList,
		        selectedPlayer, MAXSTRUCTURES - 1);

		ppsStatsList = (BASE_STATS **)apsStructStatsList;
	}

	//have to determine the Template list once the factory has been chosen
	if (objMode == IOBJ_MANUFACTURE)
	{
		fillTemplateList(apsTemplateList, (STRUCTURE *)psObj);
		numStatsListEntries = apsTemplateList.size();
		ppsStatsList = (BASE_STATS **)&apsTemplateList[0];  // FIXME Ugly cast, and is undefined behaviour (strict-aliasing violation) in C/C++.
	}

	/*have to calculate the list each time the Topic button is pressed
	  so that only one topic can be researched at a time*/
	if (objMode == IOBJ_RESEARCH)
	{
		//set to value that won't be reached in fillResearchList
		index = asResearch.size() + 1;
		if (psStats)
		{
			index = ((RESEARCH *)psStats)->index;
		}
		//recalculate the list
		numStatsListEntries = fillResearchList(pList, selectedPlayer, (UWORD)index, MAXRESEARCH);

		//	-- Alex's reordering of the list
		// NOTE!  Do we even want this anymore, since we can have basically
		// unlimted tabs? Future enhancement assign T1/2/3 button on form
		// so we can pick what level of tech we want to build instead of
		// Alex picking for us?
		count = 0;
		for (i = 0; i < RID_MAXRID; i++)
		{
			iconNumber = mapRIDToIcon(i);
			for (j = 0; j < numStatsListEntries; j++)
			{
				entryIN = asResearch[pList[j]].iconID;
				if (entryIN == iconNumber)
				{
					pSList[count++] = pList[j];
				}

			}
		}
		// Tag on the ones at the end that have no BASTARD icon IDs - why is this?!!?!?!?
		// NOTE! more pruning [future ref]
		for (j = 0; j < numStatsListEntries; j++)
		{
			iconNumber = mapIconToRID(asResearch[pList[j]].iconID);
			if (iconNumber < 0)
			{
				pSList[count++] = pList[j];
			}
		}

		//fill up the list with topics
		for (i = 0; i < numStatsListEntries; i++)
		{
			ppResearchList[i] = &asResearch[pSList[i]];	  // note change from pList
		}
	}

	intAddStats(ppsStatsList, numStatsListEntries, psStats, psObj);

	// get the tab positions for the new stat form
	psForm = (W_TABFORM *)widgGetFromID(psWScreen, IDSTAT_TABFORM);
	if (psForm != NULL)
	{
		newStatMajor = psForm->numMajor;
		newStatMinor = psForm->asMajor[statMajor].numMinor;

		// Restore the tab positions.
		if ((!psStats) && (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL))
		{
			// only restore if we've still got at least that many tabs
			if (newStatMajor > statMajor && newStatMinor > statMinor)
			{
				widgSetTabs(psWScreen, IDSTAT_TABFORM, statMajor, statMinor);
			}
		}
	}

	intMode = INT_STAT;
	/* Note the object */
	psObjSelected = psObj;
	objStatID = id;

	/* Reset the tabs and lock the button */
	widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor, objMinor);
	if (id != 0)
	{
		widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
	}
}


static void intSelectDroid(BASE_OBJECT *psObj)
{
	if (driveModeActive())
	{
		clearSel();
		((DROID *)psObj)->selected = true;
		driveSelectionChanged();
		driveDisableControl();
	}
	else
	{
		clearSelection();
		((DROID *)psObj)->selected = true;
	}
}


static void intResetWindows(BASE_OBJECT *psObj)
{
	if (psObj)
	{
		// reset the object screen with the new object
		switch (objMode)
		{
		case IOBJ_BUILD:
		case IOBJ_BUILDSEL:
		case IOBJ_DEMOLISHSEL:
			intAddBuild((DROID *)psObj);
			break;
		case IOBJ_RESEARCH:
			intAddResearch((STRUCTURE *)psObj);
			break;
		case IOBJ_MANUFACTURE:
			intAddManufacture((STRUCTURE *)psObj);
			break;
		case IOBJ_COMMAND:

			intAddCommand((DROID *)psObj);
			break;
		default:
			break;
		}
	}
}


/* Process return codes from the object screen */
static void intProcessObject(UDWORD id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStruct;
	SDWORD			butIndex;
	UDWORD			statButID;

	ASSERT(widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL, "Missing form");

	// deal with CRTL clicks
	if (objMode == IOBJ_BUILD &&	// What..................?
	    (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)) &&
	    ((id >= IDOBJ_OBJSTART && id <= IDOBJ_OBJEND) ||
	     (id >= IDOBJ_STATSTART && id <= IDOBJ_STATEND)))
	{
		/* Find the object that the ID refers to */
		psObj = intGetObject(id);
		if (id >= IDOBJ_OBJSTART && id <= IDOBJ_OBJEND)
		{
			statButID = IDOBJ_STATSTART + id - IDOBJ_OBJSTART;
		}
		else
		{
			statButID = id;
		}
		if (psObj->selected)
		{
			psObj->selected = false;
			widgSetButtonState(psWScreen, statButID, 0);
			if (intNumSelectedDroids(DROID_CONSTRUCT) == 0 && intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0)
			{
				intRemoveStats();
			}
			if (psObjSelected == psObj)
			{
				psObjSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CONSTRUCT);
				if (!psObjSelected)
				{
					psObjSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CYBORG_CONSTRUCT);
				}
			}
		}
		else
		{
			if (psObjSelected)
			{
				psObjSelected->selected = true;
			}
			psObj->selected = true;
			widgSetButtonState(psWScreen, statButID, WBUT_CLICKLOCK);
			intAddObjectStats(psObj, statButID);
		}
	}
	else if (id >= IDOBJ_OBJSTART && id <= IDOBJ_OBJEND)
	{
		/* deal with RMB clicks */
		if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
		{
			intObjectRMBPressed(id);
		}
		/* deal with LMB clicks */
		else
		{
			/* An object button has been pressed */
			/* Find the object that the ID refers to */
			psObj = intGetObject(id);
			if (!psObj)
			{
				return;
			}
			if (psObj->type == OBJ_STRUCTURE && !offWorldKeepLists)
			{
				/* Deselect old buildings */
				for (psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
				{
					psStruct->selected = false;
				}

				/* Select new one */
				((STRUCTURE *)psObj)->selected = true;
			}

			if (!driveModeActive())
			{
				// don't do this if offWorld and a structure object has been selected
				if (!(psObj->type == OBJ_STRUCTURE && offWorldKeepLists))
				{
					// set the map position - either the object position, or the position jumped from
					butIndex = id - IDOBJ_OBJSTART;
					if (butIndex >= 0 && butIndex <= IDOBJ_OBJEND - IDOBJ_OBJSTART)
					{
						asJumpPos.resize(IDOBJ_OBJEND - IDOBJ_OBJSTART, Vector2i(0, 0));
						if (((asJumpPos[butIndex].x == 0) && (asJumpPos[butIndex].y == 0)) ||
						    !DrawnInLastFrame((SDWORD)psObj->sDisplay.frameNumber) ||
						    ((psObj->sDisplay.screenX > pie_GetVideoBufferWidth()) ||
						     (psObj->sDisplay.screenY > pie_GetVideoBufferHeight())))
						{
							asJumpPos[butIndex] = getPlayerPos();
							setPlayerPos(psObj->pos.x, psObj->pos.y);
							if (getWarCamStatus())
							{
								camToggleStatus();
							}
						}
						else
						{
							setPlayerPos(asJumpPos[butIndex].x, asJumpPos[butIndex].y);
							if (getWarCamStatus())
							{
								camToggleStatus();
							}
							asJumpPos[butIndex].x = 0;
							asJumpPos[butIndex].y = 0;
						}
					}
				}
			}

			intResetWindows(psObj);

			// If a construction droid button was clicked then
			// clear all other selections and select it.
			if (psObj->type == OBJ_DROID)
			{
				intSelectDroid(psObj);
				psObjSelected = psObj;

			}
		}
	}
	/* A object stat button has been pressed */
	else if (id >= IDOBJ_STATSTART &&
	        id <= IDOBJ_STATEND)
	{
		/* deal with RMB clicks */
		if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
		{
			intObjStatRMBPressed(id);
		}
		else
		{
			/* Find the object that the stats ID refers to */
			psObj = intGetObject(id);

			intResetWindows(psObj);

			// If a droid button was clicked then clear all other selections and select it.
			if (psObj->type == OBJ_DROID)
			{
				// Select the droid when the stat button (in the object window) is pressed.
				intSelectDroid(psObj);
				psObjSelected = psObj;
			}
			else if (psObj->type == OBJ_STRUCTURE)
			{
				if (StructIsFactory((STRUCTURE *)psObj))
				{
					//might need to cancel the hold on production
					releaseProduction((STRUCTURE *)psObj, ModeQueue);
				}
				else if (((STRUCTURE *)psObj)->pStructureType->type == REF_RESEARCH)
				{
					//might need to cancel the hold on research facilty
					releaseResearch((STRUCTURE *)psObj, ModeQueue);
				}

				for (STRUCTURE *psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
				{
					psCurr->selected = false;
				}
				psObj->selected = true;
			}
		}
	}
	else if (id == IDOBJ_CLOSE)
	{
		intResetScreen(false);
		intMode = INT_NORMAL;
	}
	else
	{
		if (objMode != IOBJ_COMMAND && id != IDOBJ_TABFORM)
		{
			/* Not a button on the build form, must be on the stats form */
			intProcessStats(id);
		}
		else  if (id != IDOBJ_TABFORM)
		{
			intProcessOrder(id);
		}
	}
}


/* Process return codes from the stats screen */
static void intProcessStats(UDWORD id)
{
	BASE_STATS		*psStats;
	STRUCTURE		*psStruct;
	FLAG_POSITION	*psFlag;
	int compIndex;

	ASSERT(widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL, "Missing form");

	if (id >= IDSTAT_START &&
	    id <= IDSTAT_END)
	{
		ASSERT(id - IDSTAT_START < numStatsListEntries,
		       "intProcessStructure: Invalid structure stats id");

		/* deal with RMB clicks */
		if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
		{
			intStatsRMBPressed(id);
		}
		/* deal with LMB clicks */
		else
		{
			//manufacture works differently!
			if (objMode == IOBJ_MANUFACTURE)
			{
				compIndex = id - IDSTAT_START;
				//get the stats
				ASSERT_OR_RETURN(, compIndex < numStatsListEntries, "Invalid range referenced for numStatsListEntries, %d > %d", compIndex, numStatsListEntries);
				psStats = ppsStatsList[compIndex];
				ASSERT_OR_RETURN(, psObjSelected != NULL, "Invalid structure pointer");
				ASSERT_OR_RETURN(, psStats != NULL, "Invalid template pointer");
				if (productionPlayer == (SBYTE)selectedPlayer)
				{
					STRUCTURE *psStructure = (STRUCTURE *)psObjSelected;
					FACTORY  *psFactory = &psStructure->pFunctionality->factory;
					DROID_TEMPLATE *psNext = (DROID_TEMPLATE *)psStats;

					//increase the production
					factoryProdAdjust(psStructure, psNext, true);

					//need to check if this was the template that was mid-production
					if (getProduction(psStructure, FactoryGetTemplate(psFactory)).numRemaining() == 0)
					{
						doNextProduction(psStructure, FactoryGetTemplate(psFactory), ModeQueue);
						psNext = FactoryGetTemplate(psFactory);
					}
					else if (!StructureIsManufacturingPending(psStructure))
					{
						structSetManufacture(psStructure, psNext, ModeQueue);
					}

					if (StructureIsOnHoldPending(psStructure))
					{
						releaseProduction(psStructure, ModeQueue);
					}

					// Reset the button on the object form
					intSetStats(objStatID, (BASE_STATS *)psNext);
				}
			}
			else
			{
				/* See if this was a click on an already selected stat */
				psStats = objGetStatsFunc(psObjSelected);
				// only do the cancel operation if not trying to add to the build list
				if (psStats == ppsStatsList[id - IDSTAT_START] && objMode != IOBJ_BUILD)
				{
					// this needs to be done before the topic is cancelled from the structure
					/* If Research then need to set topic to be cancelled */
					if (objMode == IOBJ_RESEARCH)
					{
						if (psObjSelected->type == OBJ_STRUCTURE)
						{
							// TODO This call seems to be redundant, since cancelResearch is called from objSetStatsFunc==setResearchStats.
							cancelResearch((STRUCTURE *)psObjSelected, ModeQueue);
						}
					}

					/* Clear the object stats */
					objSetStatsFunc(psObjSelected, NULL);

					/* Reset the button on the object form */
					intSetStats(objStatID, NULL);

					/* Unlock the button on the stats form */
					widgSetButtonState(psWScreen, id, 0);
				}
				else
				{
					//If Research then need to set the topic - if one, to be cancelled
					if (objMode == IOBJ_RESEARCH)
					{
						if (psObjSelected->type == OBJ_STRUCTURE && ((STRUCTURE *)
						        psObjSelected)->pStructureType->type == REF_RESEARCH)
						{
							//if there was a topic currently being researched - cancel it
							if (((RESEARCH_FACILITY *)((STRUCTURE *)psObjSelected)->
							     pFunctionality)->psSubject)
							{
								cancelResearch((STRUCTURE *)psObjSelected, ModeQueue);
							}
						}
					}

					// call the tutorial callback if necessary
					if (bInTutorial && objMode == IOBJ_BUILD)
					{

						eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BUILDGRID);
					}

					// Set the object stats
					compIndex = id - IDSTAT_START;
					ASSERT_OR_RETURN(, compIndex < numStatsListEntries, "Invalid range referenced for numStatsListEntries, %d > %d", compIndex, numStatsListEntries);
					psStats = ppsStatsList[compIndex];

					// Reset the button on the object form
					//if this returns false, there's a problem so set the button to NULL
					if (!objSetStatsFunc(psObjSelected, psStats))
					{
						intSetStats(objStatID, NULL);
					}
					else
					{
						// Reset the button on the object form
						intSetStats(objStatID, psStats);
					}
				}

				// Get the tabs on the object form
				widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor, &objMinor);

				// Close the stats box
				intRemoveStats();
				intMode = INT_OBJECT;

				// Reset the tabs on the object form
				widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor, objMinor);

				// Close the object box as well if selecting a location to build- no longer hide/reveal
				// or if selecting a structure to demolish
				if (objMode == IOBJ_BUILDSEL || objMode == IOBJ_DEMOLISHSEL)
				{
					if (driveModeActive())
					{
						// Make sure weve got a construction droid selected.
						if (driveGetDriven()->droidType != DROID_CONSTRUCT
						    && driveGetDriven()->droidType != DROID_CYBORG_CONSTRUCT)
						{
							driveDisableControl();
						}
						driveDisableTactical();
						driveStartBuild();
						intRemoveObject();
					}

					intRemoveObject();
					// hack to stop the stats window re-opening in demolish mode
					if (objMode == IOBJ_DEMOLISHSEL)
					{
						IntRefreshPending = false;
					}
				}
			}
		}
	}
	else if (id == IDSTAT_CLOSE)
	{
		/* Get the tabs on the object form */
		widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor, &objMinor);

		/* Close the structure box without doing anything */
		intRemoveStats();
		intMode = INT_OBJECT;

		/* Reset the tabs on the build form */
		widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor, objMinor);

		/* Unlock the stats button */
		widgSetButtonState(psWScreen, objStatID, 0);
	}
	else if (id == IDSTAT_SLIDER)
	{
		// Process the quantity slider.
	}
	else if (id >= IDPROX_START && id <= IDPROX_END)
	{
		// process the proximity blip buttons.
	}
	else if (id == IDSTAT_LOOP_BUTTON)
	{
		// Process the loop button.
		psStruct = (STRUCTURE *)widgGetUserData(psWScreen, IDSTAT_LOOP_LABEL);
		if (psStruct)
		{
			//LMB pressed
			if (widgGetButtonKey(psWScreen) & WKEY_PRIMARY)
			{
				factoryLoopAdjust(psStruct, true);
			}
			//RMB pressed
			else if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
			{
				factoryLoopAdjust(psStruct, false);
			}
			if (((FACTORY *)psStruct->pFunctionality)->psSubject != NULL && ((FACTORY *)psStruct->pFunctionality)->productionLoops != 0)
			{
				//lock the button
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
			}
			else
			{
				//unlock
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, 0);
			}
		}
	}
	else if (id == IDSTAT_DP_BUTTON)
	{
		// Process the DP button
		psStruct = (STRUCTURE *)widgGetUserData(psWScreen, IDSTAT_DP_BUTTON);
		if (psStruct)
		{
			// make sure that the factory isn't assigned to a commander
			assignFactoryCommandDroid(psStruct, NULL);
			psFlag = FindFactoryDelivery(psStruct);
			if (psFlag)
			{
				startDeliveryPosition(psFlag);
			}
		}
	}
	else if (id == IDSTAT_TABSCRL_LEFT)	//user hit left scroll tab from BUILD menu
	{
		W_TABFORM	*psTForm;
		int temp;
#ifdef  DEBUG_SCROLLTABS
		char buf[200];		//only used for debugging
#endif
		psTForm = (W_TABFORM *)widgGetFromID(psWScreen, IDSTAT_TABFORM);	//get our form
		psTForm->TabMultiplier -= 1;				// -1 since user hit left button
		if (psTForm->TabMultiplier < 1)
		{
			psTForm->TabMultiplier = 1;			// Must be at least 1.
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		}
		//add routine to update tab widgets now...
		temp = psTForm->majorT;					// set tab # to previous "page"
		temp -= TAB_SEVEN;						// 7 = 1 "page" of tabs
		if (temp < 0)
		{
			psTForm->majorT = 0;
		}
		else
		{
			psTForm->majorT = temp;
		}
#ifdef  DEBUG_SCROLLTABS
		sprintf(buf, "[build menu]Clicked LT %d tab #=%d", psTForm->TabMultiplier, psTForm->majorT);
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
#endif
	}
	else if (id == IDSTAT_TABSCRL_RIGHT)	// user hit right scroll tab from BUILD menu
	{
		W_TABFORM	*psTForm;
		UWORD numTabs;
#ifdef  DEBUG_SCROLLTABS
		char buf[200];					// only used for debugging.
#endif
		psTForm = (W_TABFORM *)widgGetFromID(psWScreen, IDSTAT_TABFORM);
		numTabs = numForms(psTForm->numStats, psTForm->numButtons);
		numTabs = ((numTabs / TAB_SEVEN) + 1);	// (Total tabs needed / 7(max tabs that fit))+1
		psTForm->TabMultiplier += 1;
		if (psTForm->TabMultiplier > numTabs)		//add 'Bzzt' sound effect?
		{
			psTForm->TabMultiplier -= 1;				//to signify past max?
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		}
		//add routine to update tab widgets now...
		psTForm->majorT += TAB_SEVEN;				// set tab # to next "page"
		if (psTForm->majorT >= psTForm->numMajor)	// check if too many
		{
			psTForm->majorT = psTForm->numMajor - 1;	// set it back to max -1
		}
#ifdef  DEBUG_SCROLLTABS		//for debuging
		sprintf(buf, "[build menu]Clicked RT %d numtabs %d tab # %d", psTForm->TabMultiplier, numTabs, psTForm->majorT);
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
#endif
	}
}


/* Set the map view point to the world coordinates x,y */
void intSetMapPos(UDWORD x, UDWORD y)
{
	if (!driveModeActive())
	{
		setViewPos(map_coord(x), map_coord(y), true);
	}
}


/* Sync the interface to an object */
// If psObj is NULL then reset interface displays.
//
// There should be two version of this function, one for left clicking and one got right.
//
void intObjectSelected(BASE_OBJECT *psObj)
{
	if (psObj)
	{
		setWidgetsStatus(true);
		switch (psObj->type)
		{
		case OBJ_DROID:
			if (!OrderUp)
			{
				intResetScreen(false);
				// changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
				intAddOrder(psObj);
				intMode = INT_ORDER;
			}
			else
			{
				// changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
				intAddOrder(psObj);
			}
			break;

		case OBJ_STRUCTURE:
			//don't do anything if structure is only partially built
			intResetScreen(false);

			if (objMode == IOBJ_DEMOLISHSEL)
			{
				/* do nothing here */
				break;
			}

			if (((STRUCTURE *)psObj)->status == SS_BUILT)
			{
				if (((STRUCTURE *)psObj)->pStructureType->type == REF_FACTORY ||
				    ((STRUCTURE *)psObj)->pStructureType->type == REF_CYBORG_FACTORY ||
				    ((STRUCTURE *)psObj)->pStructureType->type == REF_VTOL_FACTORY)
				{
					intAddManufacture((STRUCTURE *)psObj);
				}
				else if (((STRUCTURE *)psObj)->pStructureType->type == REF_RESEARCH)
				{
					intAddResearch((STRUCTURE *)psObj);
				}
			}
			break;
		default:
			break;
		}
	}
	else
	{
		intResetScreen(false);
	}
}


// add the construction interface if a constructor droid is selected
void intConstructorSelected(DROID *psDroid)
{
	setWidgetsStatus(true);
	intAddBuild(psDroid);
	widgHide(psWScreen, IDOBJ_FORM);
}

// add the construction interface if a constructor droid is selected
void intCommanderSelected(DROID *psDroid)
{
	setWidgetsStatus(true);
	intAddCommand(psDroid);
	widgHide(psWScreen, IDOBJ_FORM);
}

/* Start looking for a structure location */
static void intStartStructPosition(BASE_STATS *psStats)
{
	init3DBuilding(psStats, NULL, NULL);
}


/* Stop looking for a structure location */
static void intStopStructPosition(void)
{
	/* Check there is still a struct position running */
	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL)
	{
		// Reset the stats button
		objMode = IOBJ_BUILD;
	}
	kill3DBuilding();
}


/* Display the widgets for the in game interface */
void intDisplayWidgets(void)
{
	if (ReticuleUp && !bInTutorial)
	{
		intCheckReticuleButtons();
	}

	/*draw the background for the design screen and the Intelligence screen*/
	if (intMode == INT_DESIGN || intMode == INT_INTELMAP)
	{
		// When will they ever learn!!!!
		if (!bMultiPlayer)
		{
			screen_RestartBackDrop();

			// We need to add the console messages to the intelmap for the tutorial so that it can display messages
			if ((intMode == INT_DESIGN) || (bInTutorial && intMode == INT_INTELMAP))
			{
				displayConsoleMessages();
			}
		}
	}

	widgDisplayScreen(psWScreen);

	if (bLoadSaveUp)
	{
		displayLoadSave();
	}
}


/* Tell the interface when an object is created - it may have to be added to a screen */
void intNewObj(BASE_OBJECT *psObj)
{
	if (intMode == INT_OBJECT || intMode == INT_STAT)
	{
		if ((objMode == IOBJ_BUILD || objMode == IOBJ_BUILDSEL) &&
		    psObj->type == OBJ_DROID && objSelectFunc(psObj))
		{
			objectsChanged = true;
		}
		else if ((objMode == IOBJ_RESEARCH || objMode == IOBJ_MANUFACTURE) &&
		        psObj->type == OBJ_STRUCTURE && objSelectFunc(psObj))
		{
			objectsChanged = true;
		}
	}
}


// clean up when an object dies
static void intObjectDied(UDWORD objID)
{
	UDWORD				statsID, gubbinsID;

	// clear the object button
	RENDERED_BUTTON *psBut = (RENDERED_BUTTON *)widgGetUserData(psWScreen, objID);
	if (psBut)
	{
		psBut->Data = NULL;
		// and its gubbins
		gubbinsID = IDOBJ_FACTORYSTART + objID - IDOBJ_OBJSTART;
		widgSetUserData(psWScreen, gubbinsID, NULL);
		gubbinsID = IDOBJ_COUNTSTART + objID - IDOBJ_OBJSTART;
		widgSetUserData(psWScreen, gubbinsID, NULL);
		gubbinsID = IDOBJ_PROGBARSTART + objID - IDOBJ_OBJSTART;
		widgSetUserData(psWScreen, gubbinsID, NULL);

		// clear the stats button
		statsID = IDOBJ_STATSTART + objID - IDOBJ_OBJSTART;
		intSetStats(statsID, NULL);
		// and disable it
		widgSetButtonState(psWScreen, statsID, WBUT_DISABLE);

		// remove the stat screen if necessary
		if ((intMode == INT_STAT) && statsID == objStatID)
		{
			intRemoveStatsNoAnim();
			intMode = INT_OBJECT;
		}
	}
}


/* Tell the interface a construction droid has finished building */
void intBuildFinished(DROID *psDroid)
{
	UDWORD	droidID;
	DROID	*psCurr;

	ASSERT(psDroid != NULL, "Invalid droid pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) &&
	    objMode == IOBJ_BUILD)
	{
		/* Find which button the droid is on and clear it's stats */
		droidID = 0;
		for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			if (objSelectFunc((BASE_OBJECT *)psCurr))
			{
				if (psCurr == psDroid)
				{
					intSetStats(droidID + IDOBJ_STATSTART, NULL);
					break;
				}
				droidID++;
			}
		}
	}
}

/* Tell the interface a construction droid has started building*/
void intBuildStarted(DROID *psDroid)
{
	UDWORD	droidID;
	DROID	*psCurr;

	ASSERT(psDroid != NULL,
	       "intBuildStarted: Invalid droid pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) &&
	    objMode == IOBJ_BUILD)
	{
		/* Find which button the droid is on and clear it's stats */
		droidID = 0;
		for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			if (objSelectFunc((BASE_OBJECT *)psCurr))
			{
				if (psCurr == psDroid)
				{
					intSetStats(droidID + IDOBJ_STATSTART, ((STRUCTURE *)psCurr->order.psObj)->pStructureType);
					break;
				}
				droidID++;
			}
		}
	}
}

/* Are we in build select mode*/
bool intBuildSelectMode(void)
{
	return (objMode == IOBJ_BUILDSEL);
}

/* Are we in demolish select mode*/
bool intDemolishSelectMode(void)
{
	return (objMode == IOBJ_DEMOLISHSEL);
}

//is the build interface up?
bool intBuildMode(void)
{
	return (objMode == IOBJ_BUILD);
}

//Written to allow demolish order to be added to the queuing system
void intDemolishCancel(void)
{
	if (objMode == IOBJ_DEMOLISHSEL)
	{
		objMode = IOBJ_NONE;
	}
}


//reorder the research facilities so that first built is first in the list
static void orderResearch(void)
{
	std::reverse(apsObjectList.begin(), apsObjectList.end());  // Why reverse this list, instead of sorting it?
}


static inline bool sortObjectByIdFunction(BASE_OBJECT *a, BASE_OBJECT *b)
{
	return (a == NULL ? 0 : a->id) < (b == NULL ? 0 : b->id);
}

// reorder the commanders
static void orderDroids(void)
{
	// bubble sort on the ID - first built will always be first in the list
	std::sort(apsObjectList.begin(), apsObjectList.end(), sortObjectByIdFunction);  // Why sort this list, instead of reversing it?
}

static inline bool sortFactoryByTypeFunction(BASE_OBJECT *a, BASE_OBJECT *b)
{
	if (a == NULL || b == NULL)
	{
		return (a == NULL) < (b == NULL);
	}
	STRUCTURE *s = castStructure(a), *t = castStructure(b);
	ASSERT(s != NULL && StructIsFactory(s) && t != NULL && StructIsFactory(t), "object is not a factory");
	FACTORY *x = (FACTORY *)s->pFunctionality, *y = (FACTORY *)t->pFunctionality;
	if (x->psAssemblyPoint->factoryType != y->psAssemblyPoint->factoryType)
	{
		return x->psAssemblyPoint->factoryType < y->psAssemblyPoint->factoryType;
	}
	return x->psAssemblyPoint->factoryInc < y->psAssemblyPoint->factoryInc;
}

/*puts the selected players factories in order - Standard factories 1-5, then
cyborg factories 1-5 and then Vtol factories 1-5*/
static void orderFactories(void)
{
	std::sort(apsObjectList.begin(), apsObjectList.end(), sortFactoryByTypeFunction);
}


/** Order the objects in the bottom bar according to their type. */
static void orderObjectInterface(void)
{
	if (apsObjectList.empty())
	{
		//no objects so nothing to order!
		return;
	}

	switch (apsObjectList[0]->type)
	{
	case OBJ_STRUCTURE:
		if (StructIsFactory((STRUCTURE *)apsObjectList[0]))
		{
			orderFactories();
		}
		else if (((STRUCTURE *)apsObjectList[0])->pStructureType->type == REF_RESEARCH)
		{
			orderResearch();
		}
		break;
	case OBJ_DROID:
		orderDroids();
	default:
		//nothing to do as yet!
		break;
	}
}

// Rebuilds apsObjectList, and returns the index of psBuilding in apsObjectList, or returns apsObjectList.size() if not present (not sure whether that's supposed to be possible).
static unsigned rebuildFactoryListAndFindIndex(STRUCTURE *psBuilding)
{
	apsObjectList.clear();
	for (STRUCTURE *psCurr = interfaceStructList(); psCurr; psCurr = psCurr->psNext)
	{
		if (objSelectFunc(psCurr))
		{
			// The list is ordered now so we have to get all possible entries and sort it before checking if this is the one!
			apsObjectList.push_back(psCurr);
		}
	}
	// order the list
	orderFactories();
	// now look thru the list to see which one corresponds to the factory that has just finished
	return std::find(apsObjectList.begin(), apsObjectList.end(), psBuilding) - apsObjectList.begin();
}

/* Tell the interface a factory has completed building ALL droids */
void intManufactureFinished(STRUCTURE *psBuilding)
{
	ASSERT(psBuilding != NULL, "Invalid structure pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_MANUFACTURE)
	{
		/* Find which button the structure is on and clear it's stats */
		unsigned structureIndex = rebuildFactoryListAndFindIndex(psBuilding);
		if (structureIndex != apsObjectList.size())
		{
			intSetStats(structureIndex + IDOBJ_STATSTART, NULL);
			// clear the loop button if interface is up
			if (widgGetFromID(psWScreen, IDSTAT_LOOP_BUTTON))
			{
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, 0);
			}
		}
	}
}

void intUpdateManufacture(STRUCTURE *psBuilding)
{
	ASSERT(psBuilding != NULL, "Invalid structure pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_MANUFACTURE)
	{
		/* Find which button the structure is on and update its stats */
		unsigned structureIndex = rebuildFactoryListAndFindIndex(psBuilding);
		if (structureIndex != apsObjectList.size())
		{
			intSetStats(structureIndex + IDOBJ_STATSTART, psBuilding->pFunctionality->factory.psSubject);
		}
	}
}

/* Tell the interface a research facility has completed a topic */
void intResearchFinished(STRUCTURE *psBuilding)
{
	ASSERT(psBuilding != NULL, "Invalid structure pointer");

	// just do a screen refresh
	intRefreshScreen();
}

void intAlliedResearchChanged()
{
	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_RESEARCH)
	{
		intRefreshScreen();
	}
}

/* Do the annoying calculation for how many forms are needed
 * given the total number of buttons and the number of
 * buttons per page.
 * A simple div just doesn't quite do it....
 */
UWORD numForms(UDWORD total, UDWORD perForm)
{
	return std::max((total + perForm - 1) / perForm, 1u);
}


/* Add the reticule widgets to the widget screen */
bool intAddReticule(void)
{
	if (!ReticuleUp)
	{
		/* Create the basic form */

		W_FORMINIT sFormInit;
		sFormInit.formID = 0;
		sFormInit.id = IDRET_FORM;
		sFormInit.style = WFORM_PLAIN;
		sFormInit.x = RET_X;
		sFormInit.y = (SWORD)RET_Y;
		sFormInit.width = RET_FORMWIDTH;
		sFormInit.height = 	RET_FORMHEIGHT;
		sFormInit.pDisplay = intDisplayPlainForm;
		if (!widgAddForm(psWScreen, &sFormInit))
		{
			return false;
		}

		/* Now add the buttons */

		//set up default button data
		W_BUTINIT sButInit;
		sButInit.formID = IDRET_FORM;
		sButInit.id = IDRET_COMMAND;
		sButInit.width = RET_BUTWIDTH;
		sButInit.height = RET_BUTHEIGHT;

		//add buttons as required...

		//options button
		SetReticuleButPos(RETBUT_COMMAND, &sButInit);
		sButInit.style = WBUT_PLAIN;
		sButInit.pTip = _("Commanders (F6)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_COMMANDDROID_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		/* Intelligence Map button - this needs to respond to RMB as well*/
		sButInit.style = WBUT_SECONDARY;
		sButInit.id = IDRET_INTEL_MAP;
		SetReticuleButPos(RETBUT_INTELMAP, &sButInit);
		sButInit.pTip = _("Intelligence Display (F5)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_INTELMAP_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		/* Manufacture button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_MANUFACTURE;
		SetReticuleButPos(RETBUT_FACTORY, &sButInit);
		sButInit.pTip = _("Manufacture (F1)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_MANUFACTURE_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		/* Design button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_DESIGN;
		SetReticuleButPos(RETBUT_DESIGN, &sButInit);
		sButInit.pTip = _("Design (F4)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_DESIGN_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		/* Research button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_RESEARCH;
		SetReticuleButPos(RETBUT_RESEARCH, &sButInit);
		sButInit.pTip = _("Research (F2)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_RESEARCH_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		/* Build button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_BUILD;
		SetReticuleButPos(RETBUT_BUILD, &sButInit);
		sButInit.pTip = _("Build (F3)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_BUILD_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		/* Cancel button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_CANCEL;
		SetReticuleButPos(RETBUT_CANCEL, &sButInit);
		sButInit.width = RET_BUTWIDTH + 10;
		sButInit.height = RET_BUTHEIGHT + 8;
		sButInit.pTip = _("Close");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_CANCEL_UP;
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}
		ReticuleUp = true;
	}

	return true;
}

void intRemoveReticule(void)
{
	if (ReticuleUp == true)
	{
		widgDelete(psWScreen, IDRET_FORM);		// remove reticule
		ReticuleUp = false;
	}
}

//toggles the Power Bar display on and off
void togglePowerBar(void)
{
	//toggle the flag
	powerBarUp = !powerBarUp;

	if (powerBarUp)
	{
		intShowPowerBar();
	}
	else
	{
		intHidePowerBar();
	}
}

/* Add the power bars to the screen */
bool intAddPower(void)
{
	W_BARINIT sBarInit;

	/* Add the trough bar */
	sBarInit.formID = 0;	//IDPOW_FORM;
	sBarInit.id = IDPOW_POWERBAR_T;
	//start the power bar off in view (default)
	sBarInit.style = WBAR_TROUGH;
	sBarInit.x = (SWORD)POW_X;
	sBarInit.y = (SWORD)POW_Y;
	sBarInit.width = POW_BARWIDTH;
	sBarInit.height = iV_GetImageHeight(IntImages, IMAGE_PBAR_EMPTY);
	sBarInit.sCol = WZCOL_POWER_BAR;
	sBarInit.pDisplay = intDisplayPowerBar;
	sBarInit.iRange = POWERBAR_SCALE;

	sBarInit.pTip = _("Power");

	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return false;
	}

	powerBarUp = true;
	return true;
}

/* Add the options widgets to the widget screen */
bool intAddOptions(void)
{
	W_FORMINIT	sFormInit;
	W_EDBINIT	sEdInit;
	W_BUTINIT	sButInit;
	W_LABINIT	sLabInit;
	UDWORD		player;

	/* Add the option form */

	sFormInit.formID = 0;
	sFormInit.id = IDOPT_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_X;
	sFormInit.y = OPT_Y;
	sFormInit.width = OPT_WIDTH;
	sFormInit.height = OPT_HEIGHT;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// set the interface mode
	intMode = INT_OPTION;

	/* Add the Option screen label */
	sLabInit.formID = IDOPT_FORM;
	sLabInit.id = IDOPT_LABEL;
	sLabInit.x = OPT_GAP;
	sLabInit.y = OPT_GAP;
	sLabInit.width = OPT_BUTWIDTH;
	sLabInit.height = OPT_BUTHEIGHT;
	sLabInit.pText = _("Options");
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	/* Add the close box */
	sButInit.formID = IDOPT_FORM;
	sButInit.id = IDOPT_CLOSE;
	sButInit.x = OPT_WIDTH - OPT_GAP - CLOSE_SIZE;
	sButInit.y = OPT_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.pText = pCloseText;
	sButInit.pTip = _("Close");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* Add the add object buttons */
	sButInit.id = IDOPT_DROID;
	sButInit.width = OPT_BUTWIDTH;
	sButInit.height = OPT_BUTHEIGHT;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_EDITY;
	sButInit.pText = _("Unit");
	sButInit.pTip = _("Place Unit on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	sButInit.id = IDOPT_STRUCT;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = _("Struct");
	sButInit.pTip = _("Place Structures on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	sButInit.id = IDOPT_FEATURE;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = _("Feat");
	sButInit.pTip = _("Place Features on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}
	if (NetPlay.bComms)
	{
		widgSetButtonState(psWScreen, sButInit.id, WBUT_DISABLE);
	}

	/* Add the quit button */
	sButInit.formID = IDOPT_FORM;
	sButInit.id = IDOPT_QUIT;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_HEIGHT - OPT_GAP - OPT_BUTHEIGHT;
	sButInit.width = OPT_WIDTH - OPT_GAP * 2;
	sButInit.height = OPT_BUTHEIGHT;
	sButInit.pText = _("Quit");
	sButInit.pTip = _("Exit Game");
	int quitButtonY = sButInit.y - OPT_GAP;
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* Add the player form */
	sFormInit.formID = IDOPT_FORM;
	sFormInit.id = IDOPT_PLAYERFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_GAP;
	sFormInit.y = OPT_PLAYERY;
	sFormInit.width = OPT_WIDTH - OPT_GAP * 2;
	sFormInit.height = (OPT_BUTHEIGHT + OPT_GAP) * (1 + (MAX_PLAYERS + 3) / 4) + OPT_GAP;
	int nextFormY = sFormInit.y + sFormInit.height + OPT_GAP;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the player label */
	sLabInit.formID = IDOPT_PLAYERFORM;
	sLabInit.id = IDOPT_PLAYERLABEL;
	sLabInit.x = OPT_GAP;
	sLabInit.y = OPT_GAP;
	sLabInit.pText = _("Current Player:");
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	/* Add the player buttons */
	sButInit.formID = IDOPT_PLAYERFORM;
	sButInit.id = IDOPT_PLAYERSTART;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_BUTHEIGHT + OPT_GAP * 2;
	sButInit.width = OPT_BUTWIDTH;
	sButInit.height = OPT_BUTHEIGHT;
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		STATIC_ASSERT(MAX_PLAYERS <= ARRAY_SIZE(apPlayerText) && MAX_PLAYERS <= ARRAY_SIZE(apPlayerTip));
		sButInit.pText = apPlayerText[player];
		sButInit.pTip = apPlayerTip[player];
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}
		if (NetPlay.bComms)
		{
			widgSetButtonState(psWScreen, sButInit.id, WBUT_DISABLE);
		}
		/* Update the initialisation structure for the next button */
		sButInit.id += 1;
		sButInit.x += OPT_BUTWIDTH + OPT_GAP;
		if (sButInit.x + OPT_BUTWIDTH + OPT_GAP > OPT_WIDTH - OPT_GAP * 2)
		{
			sButInit.x = OPT_GAP;
			sButInit.y += OPT_BUTHEIGHT + OPT_GAP;
		}
	}

	/* Add iViS form */
	sFormInit.formID = IDOPT_FORM;
	sFormInit.id = IDOPT_IVISFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_GAP;
	sFormInit.y = nextFormY;  //OPT_PLAYERY + OPT_BUTHEIGHT * 3 + OPT_GAP * 5;
	sFormInit.width = OPT_WIDTH - OPT_GAP * 2;
	sFormInit.height = quitButtonY - nextFormY;  //OPT_BUTHEIGHT * 3 + OPT_GAP * 4;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, WBUT_LOCK);

	return true;
}


/* Add the object screen widgets to the widget screen.
 * select is a pointer to a function that returns true when the object is
 * to be added to the screen.
 * getStats is a pointer to a function that returns the appropriate stats
 * for the object.
 * If psSelected != NULL it specifies which object should be hilited.
 */
static bool intAddObjectWindow(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected, bool bForceStats)
{
	UDWORD			displayForm;
	UDWORD                  statID = 0;
	BASE_OBJECT            *psFirst;
	BASE_STATS		*psStats;
	SDWORD			BufferID;
	DROID			*Droid;
	STRUCTURE		*Structure;
	bool			IsFactory;
	int				compIndex;

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDOBJ_FORM) != NULL)
	{
		intRemoveObjectNoAnim();
	}
	else
	{
		// reset the object position array
		asJumpPos.clear();
	}

	ClearObjectBuffers();
	ClearTopicBuffers();

	/* See how many objects the player has */
	apsObjectList.clear();
	for (BASE_OBJECT *psObj = psObjects; psObj; psObj = psObj->psNext)
	{
		if (objSelectFunc(psObj))
		{
			apsObjectList.push_back(psObj);
		}
	}

	if (apsObjectList.empty())
	{
		// No objects so close the stats window if it's up...
		if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
		{
			intRemoveStatsNoAnim();
		}
		// and return.
		return false;
	}
	psFirst = apsObjectList.front();

	/*if psSelected != NULL then check its in the list of suitable objects for
	this instance of the interface - this could happen when a structure is upgraded*/
	//if have reached the end of the loop and not quit out, then can't have found the selected object in the list
	if (std::find(apsObjectList.begin(), apsObjectList.end(), psSelected) == apsObjectList.end())
	{
		//initialise psSelected so gets set up with an iten in the list
		psSelected = NULL;
	}

	//order the objects according to what they are
	orderObjectInterface();

	// set the selected object if necessary
	if (psSelected == NULL)
	{
		//first check if there is an object selected of the required type
		switch (objMode)
		{
		case IOBJ_RESEARCH:
			psSelected = (BASE_OBJECT *)intCheckForStructure(REF_RESEARCH);
			break;
		case IOBJ_MANUFACTURE:
			psSelected = (BASE_OBJECT *)intCheckForStructure(REF_FACTORY);
			//if haven't got a Factory, check for specific types of factory
			if (!psSelected)
			{
				psSelected = (BASE_OBJECT *)intCheckForStructure(REF_CYBORG_FACTORY);
			}
			if (!psSelected)
			{
				psSelected = (BASE_OBJECT *)intCheckForStructure(REF_VTOL_FACTORY);
			}
			break;
		case IOBJ_BUILD:
			psSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CONSTRUCT);
			if (!psSelected)
			{
				psSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CYBORG_CONSTRUCT);
			}
			break;
		case IOBJ_COMMAND:
			psSelected = (BASE_OBJECT *)intCheckForDroid(DROID_COMMAND);
			break;
		default:
			break;
		}
		if (!psSelected)
		{
			if (apsPreviousObj[objMode]
			    && apsPreviousObj[objMode]->player == selectedPlayer)
			{
				psSelected = apsPreviousObj[objMode];
				//it is possible for a structure to change status - building of modules
				if (psSelected->type == OBJ_STRUCTURE
				    && ((STRUCTURE *)psSelected)->status != SS_BUILT)
				{
					//structure not complete so just set selected to the first valid object
					psSelected = psFirst;
				}
			}
			else
			{
				psSelected = psFirst;
			}
		}
	}

	/* Reset the current object and store the current list */
	psObjSelected = NULL;

	/* Create the basic form */

	W_FORMINIT sFormInit;
	sFormInit.formID = 0;
	sFormInit.id = IDOBJ_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)OBJ_BACKX;
	sFormInit.y = (SWORD)OBJ_BACKY;
	sFormInit.width = OBJ_BACKWIDTH;
	sFormInit.height = 	OBJ_BACKHEIGHT;
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDOBJ_FORM;
	sButInit.id = IDOBJ_CLOSE;
	sButInit.x = OBJ_BACKWIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/*add the tabbed form */
	sFormInit = W_FORMINIT();
	sFormInit.formID = IDOBJ_FORM;
	sFormInit.id = IDOBJ_TABFORM;
	sFormInit.style = WFORM_TABBED;
	sFormInit.x = OBJ_TABX;
	sFormInit.y = OBJ_TABY;
	sFormInit.width = OBJ_WIDTH;
	sFormInit.height = OBJ_HEIGHT;
	sFormInit.numMajor = numForms(apsObjectList.size(), (OBJ_WIDTH - OBJ_GAP) / (OBJ_BUTWIDTH + OBJ_GAP));
	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = OBJ_TABWIDTH;
	sFormInit.majorOffset = OBJ_TABOFFSET;
	sFormInit.tabVertOffset = (OBJ_TABHEIGHT / 2);
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;
	sFormInit.tabMajorGap = OBJ_TABOFFSET;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;

	if (sFormInit.numMajor * (sFormInit.majorSize + sFormInit.tabMajorGap) > sFormInit.width)
	{
		sFormInit.pUserData = &SmallTab;
		sFormInit.majorSize /= 2;
	}
	sFormInit.maxTabsShown = WFORM_MAXMAJOR;
	sFormInit.numMajor = std::min<unsigned>(sFormInit.numMajor, WFORM_MAXMAJOR);
	for (unsigned i = 0; i < sFormInit.numMajor; ++i)
	{
		sFormInit.aNumMinors[i] = 1;
	}
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Store the number of tabs */
	objNumTabs = sFormInit.numMajor;

	/* Add the object and stats buttons */
	W_FORMINIT sBFormInit;
	sBFormInit.formID = IDOBJ_TABFORM;
	sBFormInit.id = IDOBJ_OBJSTART;
	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	sBFormInit.x = OBJ_STARTX;
	sBFormInit.y = OBJ_STARTY;
	sBFormInit.width = OBJ_BUTWIDTH;
	sBFormInit.height = OBJ_BUTHEIGHT;
	W_FORMINIT sBFormInit2 = sBFormInit;
	sBFormInit2.id = IDOBJ_STATSTART;
	sBFormInit2.y = OBJ_STATSTARTY;
	//right click on a Template will put the production on hold
	sBFormInit2.style = WFORM_CLICKABLE | WFORM_SECONDARY;

	// Action progress bar.
	W_BARINIT sBarInit;
	sBarInit.formID = IDOBJ_OBJSTART;
	sBarInit.id = IDOBJ_PROGBARSTART;
	sBarInit.style = WBAR_TROUGH | WIDG_HIDDEN;
	sBarInit.x = STAT_PROGBARX;
	sBarInit.y = STAT_PROGBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 0;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;
	sBarInit.pTip = _("Progress Bar");

	// object output bar ie manuf power o/p, research power o/p
	W_BARINIT sBarInit2 = sBarInit;
	sBarInit2.id = IDOBJ_POWERBARSTART;
	sBarInit2.style = WBAR_PLAIN;
	sBarInit2.x = STAT_POWERBARX;
	sBarInit2.y = STAT_POWERBARY;
	sBarInit2.size = 50;
	// don't set the tip cos we haven't got a suitable text string at this point - 2/2/99
	sBarInit2.pTip = NULL;

	W_LABINIT sLabInit;
	sLabInit.id = IDOBJ_COUNTSTART;
	sLabInit.style = WIDG_HIDDEN;
	sLabInit.x = OBJ_TEXTX;
	sLabInit.y = OBJ_T1TEXTY;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "BUG! (a)";

	W_LABINIT sLabInitCmdFac;
	sLabInitCmdFac.id = IDOBJ_CMDFACSTART;
	sLabInitCmdFac.style = WIDG_HIDDEN;
	sLabInitCmdFac.x = OBJ_TEXTX;
	sLabInitCmdFac.y = OBJ_T2TEXTY;
	sLabInitCmdFac.width = 16;
	sLabInitCmdFac.height = 16;
	sLabInitCmdFac.pText = "BUG! (b)";

	W_LABINIT sLabInitCmdFac2;
	sLabInitCmdFac2.id = IDOBJ_CMDVTOLFACSTART;
	sLabInitCmdFac2.style = WIDG_HIDDEN;
	sLabInitCmdFac2.x = OBJ_TEXTX;
	sLabInitCmdFac2.y = OBJ_T3TEXTY;
	sLabInitCmdFac2.width = 16;
	sLabInitCmdFac2.height = 16;
	sLabInitCmdFac2.pText = "BUG! (c)";

	W_LABINIT sLabIntObjText;
	sLabIntObjText.id = IDOBJ_FACTORYSTART;
	sLabIntObjText.style = WIDG_HIDDEN;
	sLabIntObjText.x = OBJ_TEXTX;
	sLabIntObjText.y = OBJ_B1TEXTY;
	sLabIntObjText.width = 16;
	sLabIntObjText.height = 16;
	sLabIntObjText.pText = "xxx/xxx - overrun";

	W_LABINIT sLabInitCmdExp;
	sLabInitCmdExp.id = IDOBJ_CMDEXPSTART;
	sLabInitCmdExp.style = WIDG_HIDDEN;
	sLabInitCmdExp.x = STAT_POWERBARX;
	sLabInitCmdExp.y = STAT_POWERBARY;
	sLabInitCmdExp.width = 16;
	sLabInitCmdExp.height = 16;
	sLabInitCmdExp.pText = "@@@@@ - overrun";

	displayForm = 0;
	for (unsigned i = 0; i < apsObjectList.size(); ++i)
	{
		BASE_OBJECT *psObj = apsObjectList[i];
		if (psObj->died != 0)
		{
			continue; // Don't add the button if the objects dead.
		}
		IsFactory = false;

		/* Got an object - set the text and tip for the button */
		switch (psObj->type)
		{
		case OBJ_DROID:
			// Get the construction power of a construction droid.. Not convinced this is right.
			Droid = (DROID *)psObj;
			if (Droid->droidType == DROID_CONSTRUCT || Droid->droidType == DROID_CYBORG_CONSTRUCT)
			{
				compIndex = Droid->asBits[COMP_CONSTRUCT].nStat;
				ASSERT_OR_RETURN(false, Droid->asBits[COMP_CONSTRUCT].nStat, "Invalid droid type");
				ASSERT_OR_RETURN(false, compIndex < numConstructStats, "Invalid range referenced for numConstructStats, %d > %d", compIndex, numConstructStats);
				psStats = (BASE_STATS *)(asConstructStats + compIndex);
				sBarInit2.size = (UWORD)constructorPoints((CONSTRUCT_STATS *)psStats, Droid->player);
				if (sBarInit2.size > WBAR_SCALE)
				{
					sBarInit2.size = WBAR_SCALE;
				}
			}
			sBFormInit.pTip = droidGetName((DROID *)psObj);
			break;

		case OBJ_STRUCTURE:
			// Get the construction power of a structure
			Structure = (STRUCTURE *)psObj;
			switch (Structure->pStructureType->type)
			{
			case REF_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
				sBarInit2.size = (UWORD)((FACTORY *)Structure->
				        pFunctionality)->productionOutput;	// Need to scale?
				if (sBarInit2.size > WBAR_SCALE)
				{
					sBarInit2.size = WBAR_SCALE;
				}
				IsFactory = true;
				//right click on factory centres on DP
				sBFormInit.style = WFORM_CLICKABLE | WFORM_SECONDARY;
				break;

			case REF_RESEARCH:
				sBarInit2.size = (UWORD)((RESEARCH_FACILITY *)Structure->
				        pFunctionality)->researchPoints;	// Need to scale?
				if (sBarInit2.size > WBAR_SCALE)
				{
					sBarInit2.size = WBAR_SCALE;
				}
				break;

			default:
				ASSERT(false, "intAddObject: invalid structure type");
			}
			sBFormInit.pTip = getName(((STRUCTURE *)psObj)->pStructureType->pName);
			break;

		case OBJ_FEATURE:
			sBFormInit.pTip = getName(((FEATURE *)psObj)->psStats->pName);
			break;

		default:
			sBFormInit.pTip = NULL;
		}

		//BufferID = (sBFormInit.id-IDOBJ_OBJSTART)*2;
		BufferID = sBFormInit.id - IDOBJ_OBJSTART;
		ASSERT(BufferID < NUM_TOPICBUFFERS, "BufferID > NUM_TOPICBUFFERS");
		ClearTopicButtonBuffer(BufferID);
		RENDERBUTTON_INUSE(&TopicBuffers[BufferID]);
		TopicBuffers[BufferID].Data = (void *)psObj;
		sBFormInit.pUserData = &TopicBuffers[BufferID];
		sBFormInit.pDisplay = intDisplayObjectButton;

		if (!widgAddForm(psWScreen, &sBFormInit))
		{
			return false;
		}

		if (IsFactory)
		{
			// Add a text label for the factory Inc.
			sLabIntObjText.formID = sBFormInit.id;
			sLabIntObjText.pCallback = intAddFactoryInc;
			sLabIntObjText.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabIntObjText))
			{
				return false;
			}
			sLabIntObjText.id++;
		}
		// Add the power bar.
		if (psObj->type != OBJ_DROID || (((DROID *)psObj)->droidType == DROID_CONSTRUCT || ((DROID *)psObj)->droidType == DROID_CYBORG_CONSTRUCT))
		{
			sBarInit2.formID = sBFormInit.id;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			if (!widgAddBarGraph(psWScreen, &sBarInit2))
			{
				return false;
			}
		}

		// Add command droid bits
		if ((psObj->type == OBJ_DROID) &&
		    (((DROID *)psObj)->droidType == DROID_COMMAND))
		{
			// the group size label
			sLabIntObjText.formID = sBFormInit.id;
			sLabIntObjText.pCallback = intUpdateCommandSize;
			sLabIntObjText.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabIntObjText))
			{
				return false;
			}
			sLabIntObjText.id++;

			// the experience stars
			sLabInitCmdExp.formID = sBFormInit.id;
			sLabInitCmdExp.pCallback = intUpdateCommandExp;
			sLabInitCmdExp.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabInitCmdExp))
			{
				return false;
			}
			sLabInitCmdExp.id++;
		}

		/* Now do the stats button */
		psStats = objGetStatsFunc(psObj);

		if (psStats != NULL)
		{
			//sBFormInit2.pTip = psStats->pName;
			// If it's a droid the name might not be a stringID
			if (psStats->ref >= REF_TEMPLATE_START &&
			    psStats->ref < REF_TEMPLATE_START + REF_RANGE)
			{
				sBFormInit2.pTip = getTemplateName((DROID_TEMPLATE *)psStats);
			}
			else
			{
				sBFormInit2.pTip = getName(psStats->pName);
			}


			BufferID = (sBFormInit2.id - IDOBJ_STATSTART) * 2 + 1;
			ASSERT(BufferID < NUM_OBJECTBUFFERS, "BufferID > NUM_OBJECTBUFFERS");
			ClearObjectButtonBuffer(BufferID);
			RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
			ObjectBuffers[BufferID].Data = (void *)psObj;
			ObjectBuffers[BufferID].Data2 = (void *)psStats;
			sBFormInit2.pUserData = &ObjectBuffers[BufferID];
		}
		else if ((psObj->type == OBJ_DROID) && (((DROID *)psObj)->droidType == DROID_COMMAND))
		{
			sBFormInit2.pTip = NULL;

			BufferID = (sBFormInit2.id - IDOBJ_STATSTART) * 2 + 1;
			ASSERT(BufferID < NUM_OBJECTBUFFERS, "BufferID > NUM_OBJECTBUFFERS");
			ClearObjectButtonBuffer(BufferID);
			RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
			ObjectBuffers[BufferID].Data = (void *)psObj;
			ObjectBuffers[BufferID].Data2 = NULL;
			sBFormInit2.pUserData = &ObjectBuffers[BufferID];
		}
		else
		{
			sBFormInit2.pTip = NULL;

			BufferID = (sBFormInit2.id - IDOBJ_STATSTART) * 2 + 1;
			ASSERT(BufferID < NUM_OBJECTBUFFERS, "BufferID > NUM_OBJECTBUFFERS");
			ClearObjectButtonBuffer(BufferID);
			RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
			sBFormInit2.pUserData = &ObjectBuffers[BufferID];
		}

		sBFormInit2.pDisplay = intDisplayStatusButton;

		if (!widgAddForm(psWScreen, &sBFormInit2))
		{
			return false;
		}

		if (psObj->type != OBJ_DROID || (((DROID *)psObj)->droidType == DROID_CONSTRUCT || ((DROID *)psObj)->droidType == DROID_CYBORG_CONSTRUCT))
		{
			// Set the colour for the production run size text.
			widgSetColour(psWScreen, sBFormInit2.id, WCOL_TEXT, WZCOL_ACTION_PRODUCTION_RUN_TEXT);
			widgSetColour(psWScreen, sBFormInit2.id, WCOL_BKGRND, WZCOL_ACTION_PRODUCTION_RUN_BACKGROUND);
		}

		// Add command droid bits
		if ((psObj->type == OBJ_DROID) &&
		    (((DROID *)psObj)->droidType == DROID_COMMAND))
		{
			// the assigned factories label
			sLabInit.formID = sBFormInit2.id;
			sLabInit.pCallback = intUpdateCommandFact;
			sLabInit.pUserData = psObj;

			// the assigned cyborg factories label
			sLabInitCmdFac.formID = sBFormInit2.id;
			sLabInitCmdFac.pCallback = intUpdateCommandFact;
			sLabInitCmdFac.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabInitCmdFac))
			{
				return false;
			}
			// the assigned VTOL factories label
			sLabInitCmdFac2.formID = sBFormInit2.id;
			sLabInitCmdFac2.pCallback = intUpdateCommandFact;
			sLabInitCmdFac2.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabInitCmdFac2))
			{
				return false;
			}
		}
		else
		{
			// Add a text label for the size of the production run.
			sLabInit.formID = sBFormInit2.id;
			sLabInit.pCallback = intUpdateQuantity;
			sLabInit.pUserData = psObj;
		}
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}

		// Add the progress bar.
		sBarInit.formID = sBFormInit2.id;
		// Setup widget update callback and object pointer so we can update the progress bar.
		sBarInit.pCallback = intUpdateProgressBar;
		sBarInit.pUserData = psObj;
		sBarInit.iRange = GAME_TICKS_PER_SEC;

		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* If this matches psSelected note which form to display */
		if (psSelected == psObj)
		{
			displayForm = sBFormInit.majorID;
			statID = sBFormInit2.id;
		}

		/* Set up the next button (Objects) */
		sBFormInit.id += 1;
		ASSERT(sBFormInit.id < IDOBJ_OBJEND, "Too many object buttons");

		sBFormInit.x += OBJ_BUTWIDTH + OBJ_GAP;
		if (sBFormInit.x + OBJ_BUTWIDTH + OBJ_GAP > OBJ_WIDTH)
		{
			sBFormInit.x = OBJ_STARTX;
			sBFormInit.majorID += 1;
		}

		/* Set up the next button (Stats) */
		sLabInit.id += 1;
		sLabInitCmdFac.id += 1;
		sLabInitCmdFac2.id += 1;

		sBarInit.id += 1;
		ASSERT(sBarInit.id < IDOBJ_PROGBAREND, "Too many progress bars");

		sBarInit2.id += 1;
		ASSERT(sBarInit2.id < IDOBJ_POWERBAREND, "Too many power bars");

		sBFormInit2.id += 1;
		ASSERT(sBFormInit2.id < IDOBJ_STATEND, "Too many stat buttons");

		sBFormInit2.x += OBJ_BUTWIDTH + OBJ_GAP;
		if (sBFormInit2.x + OBJ_BUTWIDTH + OBJ_GAP > OBJ_WIDTH)
		{
			sBFormInit2.x = OBJ_STARTX;
			sBFormInit2.majorID += 1;
		}

		if (sBFormInit.id > IDOBJ_OBJEND)
		{
			//can't fit any more on the screen!
			debug(LOG_WARNING, "This is just a Warning! Max buttons have been allocated");
			break;
		}
	}

	widgSetTabs(psWScreen, IDOBJ_TABFORM, (UWORD)displayForm, 0);

	// if the selected object isn't on one of the main buttons (too many objects)
	// reset the selected pointer
	if (statID == 0)
	{
		psSelected = NULL;
	}

	if (psSelected && (objMode != IOBJ_COMMAND))
	{
		if (bForceStats || widgGetFromID(psWScreen, IDSTAT_FORM))
		{
			objStatID = statID;
			intAddObjectStats(psSelected, statID);
			intMode = INT_STAT;
		}
		else
		{
			widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);
			intMode = INT_OBJECT;
		}
	}
	else if (psSelected)
	{
		/* Note the object */
		psObjSelected = psSelected;
		objStatID = statID;
		intAddOrder(psSelected);
		widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);

		intMode = INT_CMDORDER;
	}
	else
	{
		intMode = INT_OBJECT;
	}

	if (objMode == IOBJ_BUILD || objMode == IOBJ_MANUFACTURE || objMode == IOBJ_RESEARCH)
	{
		intShowPowerBar();
	}

	if (bInTutorial)
	{
		debug(LOG_NEVER, "Go with object open callback!");
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_OBJECTOPEN);
	}

	return true;
}


static bool intUpdateObject(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected, bool bForceStats)
{
	intAddObjectWindow(psObjects, psSelected, bForceStats);

	// if the stats screen is up and..
	if (StatsUp)
	{
		if (psStatsScreenOwner != NULL)
		{
			// it's owner is dead then..
			if (psStatsScreenOwner->died != 0)
			{
				// remove it.
				intRemoveStatsNoAnim();
			}
		}
	}

	return true;
}

/* Remove the build widgets from the widget screen */
void intRemoveObject(void)
{
	W_TABFORM *Form;

	widgDelete(psWScreen, IDOBJ_TABFORM);
	widgDelete(psWScreen, IDOBJ_CLOSE);

	// Start the window close animation.
	Form = (W_TABFORM *)widgGetFromID(psWScreen, IDOBJ_FORM);
	if (Form)
	{
		Form->display = intClosePlainForm;
		Form->disableChildren = true;
		Form->pUserData = NULL; // Used to signal when the close anim has finished.
		ClosingObject = true;
	}

	ClearObjectBuffers();
	ClearTopicBuffers();

	intHidePowerBar();

	if (bInTutorial)
	{
		debug(LOG_NEVER, "Go with object close callback!");
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_OBJECTCLOSE);
	}
}


/* Remove the build widgets from the widget screen */
static void intRemoveObjectNoAnim(void)
{
	widgDelete(psWScreen, IDOBJ_TABFORM);
	widgDelete(psWScreen, IDOBJ_CLOSE);
	widgDelete(psWScreen, IDOBJ_FORM);

	ClearObjectBuffers();
	ClearTopicBuffers();

	intHidePowerBar();
}


/* Remove the stats widgets from the widget screen */
void intRemoveStats(void)
{
	W_TABFORM *Form;

	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);

	// Start the window close animation.
	Form = (W_TABFORM *)widgGetFromID(psWScreen, IDSTAT_FORM);
	if (Form)
	{
		Form->display = intClosePlainForm;
		Form->pUserData = NULL; // Used to signal when the close anim has finished.
		Form->disableChildren = true;
		ClosingStats = true;
	}

	ClearStatBuffers();

	StatsUp = false;
	psStatsScreenOwner = NULL;
}


/* Remove the stats widgets from the widget screen */
void intRemoveStatsNoAnim(void)
{
	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);
	widgDelete(psWScreen, IDSTAT_FORM);

	ClearStatBuffers();

	StatsUp = false;
	psStatsScreenOwner = NULL;
}

// Poll for closing windows and handle them, ensure called even if game is paused.
//
void HandleClosingWindows(void)
{
	WIDGET *Widg;

	if (ClosingObject)
	{
		Widg = widgGetFromID(psWScreen, IDOBJ_FORM);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDOBJ_FORM);
				ClosingObject = false;
			}
		}
		else
		{
			ClosingObject = false;
		}
	}

	if (ClosingStats)
	{
		Widg = widgGetFromID(psWScreen, IDSTAT_FORM);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDSTAT_FORM);
				ClosingStats = false;
			}
		}
		else
		{
			ClosingStats = false;
		}
	}
	if (ClosingMessageView)
	{
		Widg = widgGetFromID(psWScreen, IDINTMAP_MSGVIEW);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDINTMAP_MSGVIEW);
				ClosingMessageView = false;
			}
		}
		else
		{
			ClosingMessageView = false;
		}
	}
	if (ClosingIntelMap)
	{
		Widg = widgGetFromID(psWScreen, IDINTMAP_FORM);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDINTMAP_FORM);
				ClosingIntelMap = false;
			}
		}
		else
		{
			ClosingIntelMap = false;
		}
	}

	if (ClosingOrder)
	{
		Widg = widgGetFromID(psWScreen, IDORDER_FORM);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDORDER_FORM);
				ClosingOrder = false;
			}
		}
		else
		{
			ClosingOrder = false;
		}
	}
	if (ClosingTrans)
	{
		Widg = widgGetFromID(psWScreen, IDTRANS_FORM);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDTRANS_FORM);
				ClosingTrans = false;
			}
		}
		else
		{
			ClosingTrans = false;
		}
	}
	if (ClosingTransCont)
	{
		Widg = widgGetFromID(psWScreen, IDTRANS_CONTENTFORM);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDTRANS_CONTENTFORM);
				ClosingTransCont = false;
			}
		}
		else
		{
			ClosingTransCont = false;
		}
	}
	if (ClosingTransDroids)
	{
		Widg = widgGetFromID(psWScreen, IDTRANS_DROIDS);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, IDTRANS_DROIDS);
				ClosingTransDroids = false;
			}
		}
		else
		{
			ClosingTransDroids = false;
		}
	}

	if (ClosingInGameOp)
	{
		Widg = widgGetFromID(psWScreen, INTINGAMEOP);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, INTINGAMEOP);
				ClosingInGameOp = false;
			}
		}
		else
		{
			ClosingInGameOp = false;
		}
	}

	if (ClosingMultiMenu)
	{
		Widg = widgGetFromID(psWScreen, MULTIMENU_FORM);
		if (Widg)
		{
			// Has the window finished closing?
			if (Widg->pUserData)
			{
				widgDelete(psWScreen, MULTIMENU_FORM);
				ClosingMultiMenu = false;
			}
		}
		else
		{
			ClosingMultiMenu = false;
		}
	}
}


/**
 * Get the object refered to by a button ID on the object screen. This works for object or stats buttons.
 */
static BASE_OBJECT *intGetObject(UDWORD id)
{
	BASE_OBJECT		*psObj;

	/* If this is a stats button, find the object button linked to it */
	if (id >= IDOBJ_STATSTART && id <= IDOBJ_STATEND)
	{
		id = IDOBJ_OBJSTART + id - IDOBJ_STATSTART;
	}

	/* Find the object that the ID refers to */
	ASSERT_OR_RETURN(NULL, id - IDOBJ_OBJSTART < apsObjectList.size(), "Invalid button ID %u", id);
	psObj = apsObjectList[id - IDOBJ_OBJSTART];

	return psObj;
}


/* Reset the stats button for an object */
static void intSetStats(UDWORD id, BASE_STATS *psStats)
{
	UDWORD		butPerForm, butPos;
	SDWORD BufferID;
	BASE_OBJECT	*psObj;

	/* Update the button on the object screen */
	widgDelete(psWScreen, id);

	W_FORMINIT sFormInit;
	sFormInit.formID = IDOBJ_TABFORM;
	butPerForm = (OBJ_WIDTH - OBJ_GAP) / (OBJ_BUTWIDTH + OBJ_GAP);
	sFormInit.majorID = (UWORD)((id - IDOBJ_STATSTART) / butPerForm);
	sFormInit.minorID = 0;
	sFormInit.id = id;
	sFormInit.style = WFORM_CLICKABLE | WFORM_SECONDARY;
	butPos = (id - IDOBJ_STATSTART) % butPerForm;
	sFormInit.x = (UWORD)(butPos * (OBJ_BUTWIDTH + OBJ_GAP) + OBJ_STARTX);
	sFormInit.y = OBJ_STATSTARTY;
	sFormInit.width = OBJ_BUTWIDTH;
	sFormInit.height = OBJ_BUTHEIGHT;

	// Action progress bar.
	W_BARINIT sBarInit;
	sBarInit.formID = id;
	sBarInit.id = (id - IDOBJ_STATSTART) + IDOBJ_PROGBARSTART;
	sBarInit.style = WBAR_TROUGH;
	sBarInit.x = STAT_PROGBARX;
	sBarInit.y = STAT_PROGBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 0;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;
	sBarInit.iRange = GAME_TICKS_PER_SEC;
	// Setup widget update callback and object pointer so we can update the progress bar.
	sBarInit.pCallback = intUpdateProgressBar;
	sBarInit.pUserData = intGetObject(id);

	W_LABINIT sLabInit;
	sLabInit.formID = id;
	sLabInit.id = (id - IDOBJ_STATSTART) + IDOBJ_COUNTSTART;
	sLabInit.style = WIDG_HIDDEN;
	sLabInit.x = OBJ_TEXTX;
	sLabInit.y = OBJ_T1TEXTY;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "BUG! (d)";

	if (psStats)
	{
		// If it's a droid the name might not be a stringID
		if (psStats->ref >= REF_TEMPLATE_START &&
		    psStats->ref < REF_TEMPLATE_START + REF_RANGE)
		{
			sFormInit.pTip = getTemplateName((DROID_TEMPLATE *)psStats);
		}
		else
		{
			sFormInit.pTip = getName(psStats->pName);
		}

		BufferID = (sFormInit.id - IDOBJ_STATSTART) * 2 + 1;
		ASSERT(BufferID < NUM_OBJECTBUFFERS, "BufferID > NUM_OBJECTBUFFERS");
		ClearObjectButtonBuffer(BufferID);
		RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
		ObjectBuffers[BufferID].Data = (void *)intGetObject(id);
		ObjectBuffers[BufferID].Data2 = (void *)psStats;
		sFormInit.pUserData = &ObjectBuffers[BufferID];

		// Add a text label for the size of the production run.
		sLabInit.pCallback = intUpdateQuantity;
		sLabInit.pUserData = sBarInit.pUserData;
	}
	else
	{
		sFormInit.pTip = NULL;

		BufferID = (sFormInit.id - IDOBJ_STATSTART) * 2 + 1;
		ASSERT(BufferID < NUM_OBJECTBUFFERS, "BufferID > NUM_OBJECTBUFFERS");
		ClearObjectButtonBuffer(BufferID);
		RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
		sFormInit.pUserData = &ObjectBuffers[BufferID];

		/* Reset the stats screen button if necessary */
		if ((INTMODE)objMode == INT_STAT && statID != 0)
		{
			widgSetButtonState(psWScreen, statID, 0);
		}
	}

	sFormInit.pDisplay = intDisplayStatusButton;

	widgAddForm(psWScreen, &sFormInit);
	// Set the colour for the production run size text.
	widgSetColour(psWScreen, sFormInit.id, WCOL_TEXT, WZCOL_ACTION_PRODUCTION_RUN_TEXT);
	widgSetColour(psWScreen, sFormInit.id, WCOL_BKGRND, WZCOL_ACTION_PRODUCTION_RUN_BACKGROUND);

	widgAddLabel(psWScreen, &sLabInit);
	widgAddBarGraph(psWScreen, &sBarInit);

	psObj = intGetObject(id);
	if (psObj && psObj->selected)
	{
		widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
	}
}

/* Add the stats widgets to the widget screen */
/* If psSelected != NULL it specifies which stat should be hilited
   psOwner specifies which object is hilighted on the object bar for this stat*/
static bool intAddStats(BASE_STATS **ppsStatsList, UDWORD numStats,
        BASE_STATS *psSelected, BASE_OBJECT *psOwner)
{
	UDWORD				i, butPerForm, statForm;
	SDWORD				BufferID;
	BASE_STATS			*Stat;
	FACTORY				*psFactory;

	int                             allyResearchIconCount = 0;

	// should this ever be called with psOwner == NULL?

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
	{
		intRemoveStatsNoAnim();
	}

	// is the order form already up ?
	if (widgGetFromID(psWScreen, IDORDER_FORM) != NULL)
	{
		intRemoveOrderNoAnim();
	}

	if (psOwner != NULL)
	{
		// Return if the owner is dead.
		if (psOwner->died)
		{
			debug(LOG_GUI, "intAddStats: Owner is dead");
			return false;
		}
	}
	SecondaryWindowUp = true;
	psStatsScreenOwner = psOwner;

	ClearStatBuffers();

	widgEndScreen(psWScreen);

	/* Create the basic form */

	W_FORMINIT sFormInit;
	sFormInit.formID = 0;
	sFormInit.id = IDSTAT_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = STAT_X;
	sFormInit.y = (SWORD)STAT_Y;
	sFormInit.width = STAT_WIDTH;
	sFormInit.height = 	STAT_HEIGHT;
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		debug(LOG_ERROR, "intAddStats: Failed to add form");
		return false;
	}

	W_LABINIT sLabInit;

	// Add the quantity slider ( if it's a factory ).
	if (objMode == IOBJ_MANUFACTURE)
	{
		//add the Factory DP button
		W_BUTINIT sButInit;
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_DP_BUTTON;
		sButInit.style = WBUT_SECONDARY;
		sButInit.x = 4;
		sButInit.y = STAT_SLDY;
		sButInit.width = iV_GetImageWidth(IntImages, IMAGE_FDP_DOWN);
		sButInit.height = iV_GetImageHeight(IntImages, IMAGE_FDP_DOWN);
		sButInit.pTip = _("Factory Delivery Point");
		sButInit.pDisplay = intDisplayDPButton;
		sButInit.pUserData = psOwner;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		//add the Factory Loop button!
		sButInit = W_BUTINIT();
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_LOOP_BUTTON;
		sButInit.style = WBUT_SECONDARY;
		sButInit.x = STAT_SLDX + STAT_SLDWIDTH + 2;
		sButInit.y = STAT_SLDY;
		sButInit.width = iV_GetImageWidth(IntImages, IMAGE_LOOP_DOWN);
		sButInit.height = iV_GetImageHeight(IntImages, IMAGE_LOOP_DOWN);
		sButInit.pTip = _("Loop Production");
		sButInit.pDisplay = intDisplayButtonPressed;
		sButInit.UserData = PACKDWORD_TRI(IMAGE_LOOP_DOWN, IMAGE_LOOP_HI, IMAGE_LOOP_UP);

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}

		if (psOwner != NULL)
		{
			psFactory = (FACTORY *)((STRUCTURE *)psOwner)->pFunctionality;
			if (psFactory->psSubject != NULL && psFactory->productionLoops != 0)
			{
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
			}
		}

		// create a text label for the loop quantity.
		sLabInit.formID = IDSTAT_FORM;
		sLabInit.id = IDSTAT_LOOP_LABEL;
		sLabInit.style = WIDG_HIDDEN;
		sLabInit.x = (UWORD)(sButInit.x - 15);
		sLabInit.y = sButInit.y;
		sLabInit.width = 12;
		sLabInit.height = 15;
		sLabInit.pUserData = psOwner;
		sLabInit.pCallback = intAddLoopQuantity;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}

		/* store the common values for the text labels for the quantity
		to produce (on each button).*/
		sLabInit = W_LABINIT();
		sLabInit.id = IDSTAT_PRODSTART;
		sLabInit.style = WIDG_HIDDEN | WLAB_ALIGNRIGHT;

		sLabInit.x = STAT_BUTWIDTH - 12 - 6;
		sLabInit.y = 2;

		sLabInit.width = 12;
		sLabInit.height = 15;
		sLabInit.pCallback = intAddProdQuantity;
	}

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDSTAT_FORM;
	sButInit.id = IDSTAT_CLOSE;
	sButInit.x = STAT_WIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}
	/* Calculate how many buttons will go on a form */
	butPerForm = ((STAT_WIDTH - STAT_GAP) /
	        (STAT_BUTWIDTH + STAT_GAP)) *
	        ((STAT_HEIGHT - STAT_GAP) /
	                (STAT_BUTHEIGHT + STAT_GAP));
	//================== adds L/R Scroll buttons ===================================
	if (numForms(numStats, butPerForm) > MAX_TAB_SMALL_SHOWN)	//only want these buttons when tab count >8
	{
		// Add the left tab scroll button
		sButInit = W_BUTINIT();
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_TABSCRL_LEFT;
		sButInit.x = STAT_TABFORMX + 4;
		sButInit.y = STAT_TABFORMY;
		sButInit.width = TABSCRL_WIDTH;
		sButInit.height = TABSCRL_HEIGHT;
		sButInit.pTip = _("Tab Scroll left");
		sButInit.pDisplay = intDisplayImageHilight;
		sButInit.UserData = PACKDWORD_TRI(0, IMAGE_LFTTABD, IMAGE_LFTTAB);
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}
		// Add the right tab scroll button
		sButInit = W_BUTINIT();
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_TABSCRL_RIGHT;
		sButInit.x = STAT_WIDTH - 14;
		sButInit.y = STAT_TABFORMY;
		sButInit.width = TABSCRL_WIDTH;
		sButInit.height = TABSCRL_HEIGHT;
		sButInit.pTip = _("Tab Scroll right");
		sButInit.pDisplay = intDisplayImageHilight;
		sButInit.UserData = PACKDWORD_TRI(0, IMAGE_RGTTABD, IMAGE_RGTTAB);
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}
	}
	//==============buttons before tabbed form!==========================
	// Add the tabbed form
	sFormInit = W_FORMINIT();
	sFormInit.formID = IDSTAT_FORM;
	sFormInit.id = IDSTAT_TABFORM;
	sFormInit.style = WFORM_TABBED;
	sFormInit.x = STAT_TABFORMX;
	sFormInit.y = STAT_TABFORMY;
	sFormInit.width = STAT_WIDTH;
	sFormInit.height = STAT_HEIGHT;
	sFormInit.numButtons = butPerForm;		// store # of buttons per form
	sFormInit.numStats = numStats;			// store # of 'stats' (items) in form
	sFormInit.numMajor = numForms(numStats, butPerForm);	// STUPID name for # of tabs!
	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = OBJ_TABWIDTH;
	sFormInit.majorOffset = OBJ_TABOFFSET;
	sFormInit.tabVertOffset = (OBJ_TABHEIGHT / 2);
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;
	sFormInit.tabMajorGap = OBJ_TABOFFSET;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;
	// Build menu can have up to 80 stats - so can research now 13/09/99 AB
	// NOTE, there is really no limit now to the # of menu items we can have,
	// It is #defined in hci.h to be 200 now. [#define	MAXSTRUCTURES	200]
	//Same goes for research. [#define	MAXRESEARCH		200]
	if (sFormInit.numMajor > MAX_TAB_STD_SHOWN)
	{
		//Just switching from normal sized tabs to smaller ones to fit more in form.
		sFormInit.pUserData = &SmallTab;
		sFormInit.majorSize /= 2;
		if (sFormInit.numMajor > MAX_TAB_SMALL_SHOWN)		// 7 tabs is all we can fit with current form size.
		{
			// make room for new tab item (tab scroll buttons)
			sFormInit.majorOffset = OBJ_TABOFFSET + 10;
			sFormInit.TabMultiplier = 1;		// Enable our tabMultiplier buttons.
		}
	}
	for (i = 0; i < sFormInit.numMajor; i++)	// Sets # of tab's minors
	{
		sFormInit.aNumMinors[i] = 1;
	}
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the stat buttons */
	W_FORMINIT sBFormInit;
	sBFormInit.formID = IDSTAT_TABFORM;
	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.id = IDSTAT_START;
	sBFormInit.style = WFORM_CLICKABLE | WFORM_SECONDARY;
	sBFormInit.x = STAT_BUTX;
	sBFormInit.y = STAT_BUTY;
	sBFormInit.width = STAT_BUTWIDTH;
	sBFormInit.height = STAT_BUTHEIGHT;

	W_BARINIT sBarInit;
	sBarInit.id = IDSTAT_TIMEBARSTART;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;

	statID = 0;
	statForm = 0;
	for (i = 0; i < numStats; i++)
	{
		sBarInit.y = STAT_TIMEBARY;

		if (sBFormInit.id > IDSTAT_END)
		{
			//can't fit any more on the screen!
			debug(LOG_WARNING, "This is just a Warning! Max buttons have been allocated");
			break;
		}

		Stat = ppsStatsList[i];
		// If it's a droid the name might not be a stringID
		if (Stat->ref >= REF_TEMPLATE_START &&
		    Stat->ref < REF_TEMPLATE_START + REF_RANGE)
		{

			sBFormInit.pTip = getTemplateName((DROID_TEMPLATE *)ppsStatsList[i]);
		}
		else
		{
			sBFormInit.pTip = getName(ppsStatsList[i]->pName);
		}
		BufferID = i;
		ASSERT(BufferID < NUM_STATBUFFERS, "BufferID > NUM_STATBUFFERS");

		RENDERBUTTON_INUSE(&StatBuffers[BufferID]);
		StatBuffers[BufferID].Data = (void *)ppsStatsList[i];
		sBFormInit.pUserData = &StatBuffers[BufferID];
		sBFormInit.pDisplay = intDisplayStatsButton;


		if (!widgAddForm(psWScreen, &sBFormInit))
		{
			return false;
		}
		widgSetColour(psWScreen, sBFormInit.id, WCOL_BKGRND, WZCOL_BLACK);

		//Stat = ppsStatsList[i];
		if (Stat->ref >= REF_STRUCTURE_START &&
		    Stat->ref < REF_STRUCTURE_START + REF_RANGE)  		// It's a structure.
		{

			//sBarInit.pTip = _("Build Speed");
			//sBarInit.size = (UWORD)(((STRUCTURE_STATS*)Stat)->buildPoints / BUILDPOINTS_STRUCTDIV);
			sBarInit.size = (UWORD)(((STRUCTURE_STATS *)Stat)->powerToBuild /
			        POWERPOINTS_DROIDDIV);
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			sBarInit.formID = sBFormInit.id;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return false;
			}

		}
		else if (Stat->ref >= REF_TEMPLATE_START &&
		        Stat->ref < REF_TEMPLATE_START + REF_RANGE)  	// It's a droid.
		{

			//sBarInit.size = (UWORD)(((DROID_TEMPLATE*)Stat)->buildPoints  / BUILDPOINTS_DROIDDIV);
			sBarInit.size = (UWORD)(((DROID_TEMPLATE *)Stat)->powerPoints /
			        POWERPOINTS_DROIDDIV);
			//sBarInit.pTip = _("Power Usage");
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			sBarInit.formID = sBFormInit.id;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return false;
			}

			// Add a text label for the quantity to produce.
			sLabInit.formID = sBFormInit.id;
			sLabInit.pUserData = Stat;
			if (!widgAddLabel(psWScreen, &sLabInit))
			{
				return false;
			}
			sLabInit.id++;
		}

		else if (Stat->ref >= REF_RESEARCH_START &&
		        Stat->ref < REF_RESEARCH_START + REF_RANGE)				// It's a Research topic.
		{
			sLabInit = W_LABINIT();
			sLabInit.formID = sBFormInit.id ;
			sLabInit.id = IDSTAT_RESICONSTART + (sBFormInit.id - IDSTAT_START);

			sLabInit.x = STAT_BUTWIDTH - 16;
			sLabInit.y = 3;

			sLabInit.width = 12;
			sLabInit.height = 15;
			sLabInit.pUserData = Stat;
			sLabInit.pDisplay = intDisplayResSubGroup;
			widgAddLabel(psWScreen, &sLabInit);

			//add power bar as well
			sBarInit.size = (UWORD)(((RESEARCH *)Stat)->researchPower /
			        POWERPOINTS_DROIDDIV);
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			// if multiplayer, if research topic is being done by another ally then mark as such..
			if (bMultiPlayer)
			{
				int labsDone = 0;
				for (unsigned ii = 0; ii < MAX_PLAYERS && labsDone < 4; ++ii)
				{
					if (ii != selectedPlayer && aiCheckAlliances(selectedPlayer, ii))
					{
						//check each research facility to see if they are doing this topic.
						for (STRUCTURE *psOtherStruct = apsStructLists[ii]; psOtherStruct; psOtherStruct = psOtherStruct->psNext)
						{
							if (psOtherStruct->pStructureType->type == REF_RESEARCH
							    && psOtherStruct->status == SS_BUILT
							    && ((RESEARCH_FACILITY *)psOtherStruct->pFunctionality)->psSubject
							    && ((RESEARCH_FACILITY *)psOtherStruct->pFunctionality)->psSubject->ref == Stat->ref
							   )
							{
								// add a label.
								sLabInit = W_LABINIT();
								sLabInit.formID = sBFormInit.id ;
								sLabInit.id = IDSTAT_ALLYSTART + allyResearchIconCount;
								sLabInit.width = iV_GetImageWidth(IntImages, IMAGE_ALLY_RESEARCH);
								sLabInit.height = iV_GetImageHeight(IntImages, IMAGE_ALLY_RESEARCH);
								sLabInit.x = STAT_BUTWIDTH  - (sLabInit.width + 2) * labsDone - sLabInit.width - 2;
								sLabInit.y = STAT_BUTHEIGHT - sLabInit.height - 3 - STAT_PROGBARHEIGHT;
								sLabInit.UserData = ii;
								sLabInit.pTip = getPlayerName(ii);
								sLabInit.pDisplay = intDisplayAllyIcon;
								widgAddLabel(psWScreen, &sLabInit);

								++labsDone;
								++allyResearchIconCount;
								ASSERT(allyResearchIconCount < IDSTAT_ALLYEND - IDSTAT_ALLYSTART, " ");
							}
						}
					}
				}
				if (labsDone > 0)
				{
					W_BARINIT progress;
					progress.formID = sBFormInit.id;
					progress.id = IDSTAT_ALLYSTART + allyResearchIconCount;
					progress.width = STAT_PROGBARWIDTH;
					progress.height = STAT_PROGBARHEIGHT;
					progress.x = STAT_TIMEBARX;
					progress.y = STAT_TIMEBARY;
					progress.UserData = Stat->ref - REF_RESEARCH_START;
					progress.pTip = _("Ally progress");
					progress.pDisplay = intDisplayAllyBar;
					widgAddBarGraph(psWScreen, &progress);

					++allyResearchIconCount;

					sBarInit.y -= STAT_PROGBARHEIGHT + 2;  // Move cost bar up, to avoid overlap.
				}
			}

			sBarInit.formID = sBFormInit.id;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return false;
			}
		}

		/* If this matches psSelected note the form and button */
		if (ppsStatsList[i] == psSelected)
		{
			statID = sBFormInit.id;
			statForm = sBFormInit.majorID;
		}

		/* Update the init struct for the next button */
		sBFormInit.id += 1;
		sBFormInit.x += STAT_BUTWIDTH + STAT_GAP;
		if (sBFormInit.x + STAT_BUTWIDTH + STAT_GAP > STAT_WIDTH)	// - STAT_TABWIDTH)
		{
			sBFormInit.x = STAT_BUTX;	//STAT_GAP;
			sBFormInit.y += STAT_BUTHEIGHT + STAT_GAP;
		}
		if (sBFormInit.y + STAT_BUTHEIGHT + STAT_GAP > STAT_HEIGHT) // - STAT_TITLEHEIGHT)
		{
			sBFormInit.y = STAT_BUTY;	//STAT_GAP;
			sBFormInit.majorID += 1;
		}

		sBarInit.id += 1;
	}

	/* Set the correct page and button if necessary */
	if (statID)
	{
		widgSetTabs(psWScreen, IDSTAT_TABFORM, (UWORD)statForm, 0);
		widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);
	}

	StatsUp = true;

	// call the tutorial callbacks if necessary
	if (bInTutorial)
	{
		switch (objMode)
		{
		case IOBJ_BUILD:
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BUILDLIST);
			break;
		case IOBJ_RESEARCH:
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEARCHLIST);
			break;
		case IOBJ_MANUFACTURE:
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MANULIST);
			break;
		default:
			break;
		}
	}

	return true;
}


/* Select a command droid */
static bool selectCommand(BASE_OBJECT *psObj)
{
	ASSERT(psObj && psObj->type == OBJ_DROID, "Invalid droid pointer");
	DROID *psDroid = (DROID *)psObj;
	if (psDroid->droidType == DROID_COMMAND && psDroid->died == 0)
	{
		return true;
	}
	return false;
}

/* Return the stats for a command droid */
static BASE_STATS *getCommandStats(WZ_DECL_UNUSED BASE_OBJECT *psObj)
{
	return NULL;
}

/* Set the stats for a command droid */
static bool setCommandStats(WZ_DECL_UNUSED BASE_OBJECT *psObj, WZ_DECL_UNUSED BASE_STATS *psStats)
{
	return true;
}

/* Select a construction droid */
static bool selectConstruction(BASE_OBJECT *psObj)
{
	DROID	*psDroid;

	ASSERT(psObj != NULL && psObj->type == OBJ_DROID,
	       "selectConstruction: invalid droid pointer");
	psDroid = (DROID *)psObj;

	//check the droid type
	if ((psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT) && (psDroid->died == 0))
	{
		return true;
	}

	return false;
}

/* Return the stats for a construction droid */
static BASE_STATS *getConstructionStats(BASE_OBJECT *psObj)
{
	DROID *psDroid = (DROID *)psObj;
	BASE_STATS *Stats;
	BASE_OBJECT *Structure;
	UDWORD x, y;

	ASSERT(psObj != NULL && psObj->type == OBJ_DROID,
	       "getConstructionStats: invalid droid pointer");

	if (!(droidType(psDroid) == DROID_CONSTRUCT ||
	      droidType(psDroid) == DROID_CYBORG_CONSTRUCT))
	{
		return NULL;
	}

	if (orderStateStatsLoc(psDroid, DORDER_BUILD, &Stats, &x, &y)) // Moving to build location?
	{
		return Stats;
	}
	else if ((Structure = orderStateObj(psDroid, DORDER_BUILD))
	        && psDroid->order.type == DORDER_BUILD) // Is building
	{
		return psDroid->order.psStats;
	}
	else if ((Structure = orderStateObj(psDroid, DORDER_HELPBUILD))
	        && (psDroid->order.type == DORDER_HELPBUILD
	                || psDroid->order.type == DORDER_LINEBUILD)) // Is helping
	{
		return (BASE_STATS *)((STRUCTURE *)Structure)->pStructureType;
	}
	else if (orderState(psDroid, DORDER_DEMOLISH))
	{
		return (BASE_STATS *)structGetDemolishStat();
	}

	return NULL;
}

/* Set the stats for a construction droid */
static bool setConstructionStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	DROID *psDroid = castDroid(psObj);
	ASSERT(psDroid != NULL, "invalid droid pointer");

	/* psStats might be NULL if the operation is canceled in the middle */
	if (psStats != NULL)
	{
		//check for demolish first
		if (psStats == structGetDemolishStat())
		{
			objMode = IOBJ_DEMOLISHSEL;

			return true;
		}

		/* Store the stats for future use */
		psPositionStats = psStats;

		/* Now start looking for a location for the structure */
		objMode = IOBJ_BUILDSEL;

		intStartStructPosition(psStats);

		return true;
	}
	orderDroid(psDroid, DORDER_STOP, ModeQueue);
	return true;
}

/* Select a research facility */
static bool selectResearch(BASE_OBJECT *psObj)
{
	STRUCTURE	*psResFacility;

	ASSERT(psObj != NULL && psObj->type == OBJ_STRUCTURE,
	       "selectResearch: invalid Structure pointer");

	psResFacility = (STRUCTURE *)psObj;

	/* A Structure is a research facility if its type = REF_RESEARCH and is
	   completely built*/
	if (psResFacility->pStructureType->type == REF_RESEARCH && (psResFacility->
	        status == SS_BUILT) && (psResFacility->died == 0))
	{
		return true;
	}
	return false;
}

/* Return the stats for a research facility */
static BASE_STATS *getResearchStats(BASE_OBJECT *psObj)
{
	STRUCTURE *psBuilding;
	RESEARCH_FACILITY *psResearchFacility;

	ASSERT(psObj != NULL && psObj->type == OBJ_STRUCTURE,
	       "getResearchTip: invalid Structure pointer");
	psBuilding = (STRUCTURE *)psObj;
	psResearchFacility = &psBuilding->pFunctionality->researchFacility;

	if (psResearchFacility->psSubjectPending != NULL && !IsResearchCompleted(&asPlayerResList[psObj->player][psResearchFacility->psSubjectPending->index]))
	{
		return psResearchFacility->psSubjectPending;
	}

	return psResearchFacility->psSubject;
}

/* Set the stats for a research facility */
static bool setResearchStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	STRUCTURE			*psBuilding;
	RESEARCH               *pResearch = (RESEARCH *)psStats;
	PLAYER_RESEARCH		*pPlayerRes;
	UDWORD				count;
	RESEARCH_FACILITY	*psResFacilty;

	ASSERT(psObj != NULL && psObj->type == OBJ_STRUCTURE,
	       "setResearchStats: invalid Structure pointer");
	/* psStats might be NULL if the operation is canceled in the middle */
	psBuilding = (STRUCTURE *)psObj;
	psResFacilty = &psBuilding->pFunctionality->researchFacility;

	if (bMultiMessages)
	{
		if (pResearch != NULL)
		{
			// Say that we want to do reseach [sic].
			sendResearchStatus(psBuilding, pResearch->ref - REF_RESEARCH_START, selectedPlayer, true);
			setStatusPendingStart(*psResFacilty, pResearch);  // Tell UI that we are going to research.
		}
		else
		{
			cancelResearch(psBuilding, ModeQueue);
		}
		//stop the button from flashing once a topic has been chosen
		stopReticuleButtonFlash(IDRET_RESEARCH);
		return true;
	}

	//initialise the subject
	psResFacilty->psSubject = NULL;

	//set up the player_research
	if (pResearch != NULL)
	{
		count = pResearch->ref - REF_RESEARCH_START;
		//meant to still be in the list but greyed out
		pPlayerRes = &asPlayerResList[selectedPlayer][count];

		//set the subject up
		psResFacilty->psSubject = pResearch;

		sendResearchStatus(psBuilding, count, selectedPlayer, true);	// inform others, I'm researching this.

		MakeResearchStarted(pPlayerRes);
		psResFacilty->timeStartHold = 0;
		//stop the button from flashing once a topic has been chosen
		stopReticuleButtonFlash(IDRET_RESEARCH);
	}
	return true;
}

/* Select a Factory */
static bool selectManufacture(BASE_OBJECT *psObj)
{
	STRUCTURE		*psBuilding;

	ASSERT(psObj != NULL && psObj->type == OBJ_STRUCTURE,
	       "selectManufacture: invalid Structure pointer");
	psBuilding = (STRUCTURE *)psObj;

	/* A Structure is a Factory if its type = REF_FACTORY or REF_CYBORG_FACTORY or
	REF_VTOL_FACTORY and it is completely built*/
	if ((psBuilding->pStructureType->type == REF_FACTORY ||
	     psBuilding->pStructureType->type == REF_CYBORG_FACTORY ||
	     psBuilding->pStructureType->type == REF_VTOL_FACTORY) &&
	    (psBuilding->status == SS_BUILT) && (psBuilding->died == 0))
	{
		return true;
	}

	return false;
}

/* Return the stats for a Factory */
static BASE_STATS *getManufactureStats(BASE_OBJECT *psObj)
{
	STRUCTURE	*psBuilding;

	ASSERT(psObj != NULL && psObj->type == OBJ_STRUCTURE,
	       "getManufactureTip: invalid Structure pointer");
	psBuilding = (STRUCTURE *)psObj;

	return ((BASE_STATS *)((FACTORY *)psBuilding->pFunctionality)->psSubject);
}


/* Set the stats for a Factory */
static bool setManufactureStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	STRUCTURE		*Structure;

	ASSERT(psObj != NULL && psObj->type == OBJ_STRUCTURE,
	       "setManufactureStats: invalid Structure pointer");
	/* psStats might be NULL if the operation is canceled in the middle */

	Structure = (STRUCTURE *)psObj;
	//check to see if the factory was already building something
	if (!((FACTORY *)Structure->pFunctionality)->psSubject)
	{
		//factory not currently building so set up the factory stats
		if (psStats != NULL)
		{
			/* Set the factory to build droid(s) */
			if (!structSetManufacture(Structure, (DROID_TEMPLATE *)psStats, ModeQueue))
			{
				return false;
			}
		}
	}

	return true;
}


/* Add the build widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static bool intAddBuild(DROID *psSelected)
{
	/* Store the correct stats list for future reference */
	ppsStatsList = (BASE_STATS **)apsStructStatsList;

	objSelectFunc = selectConstruction;
	objGetStatsFunc = getConstructionStats;
	objSetStatsFunc = setConstructionStats;

	/* Set the sub mode */
	objMode = IOBJ_BUILD;

	/* Create the object screen with the required data */

	return intAddObjectWindow((BASE_OBJECT *)apsDroidLists[selectedPlayer],
	        (BASE_OBJECT *)psSelected, true);
}


/* Add the manufacture widgets to the widget screen */
/* If psSelected != NULL it specifies which factory should be hilited */
static bool intAddManufacture(STRUCTURE *psSelected)
{
	/* Store the correct stats list for future reference */
	ppsStatsList = (BASE_STATS **)&apsTemplateList[0]; // FIXME Ugly cast, and is undefined behaviour (strict-aliasing violation) in C/C++.

	objSelectFunc = selectManufacture;
	objGetStatsFunc = getManufactureStats;
	objSetStatsFunc = setManufactureStats;

	/* Set the sub mode */
	objMode = IOBJ_MANUFACTURE;

	/* Create the object screen with the required data */
	return intAddObjectWindow((BASE_OBJECT *)interfaceStructList(),
	        (BASE_OBJECT *)psSelected, true);
}


/* Add the research widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static bool intAddResearch(STRUCTURE *psSelected)
{
	ppsStatsList = (BASE_STATS **)ppResearchList;

	objSelectFunc = selectResearch;
	objGetStatsFunc = getResearchStats;
	objSetStatsFunc = setResearchStats;

	/* Set the sub mode */
	objMode = IOBJ_RESEARCH;

	/* Create the object screen with the required data */
	return intAddObjectWindow((BASE_OBJECT *)interfaceStructList(),
	        (BASE_OBJECT *)psSelected, true);
}


/* Add the command droid widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static bool intAddCommand(DROID *psSelected)
{
	ppsStatsList = NULL;//(BASE_STATS **)ppResearchList;

	objSelectFunc = selectCommand;
	objGetStatsFunc = getCommandStats;
	objSetStatsFunc = setCommandStats;

	/* Set the sub mode */
	objMode = IOBJ_COMMAND;

	/* Create the object screen with the required data */
	return intAddObjectWindow((BASE_OBJECT *)apsDroidLists[selectedPlayer],
	        (BASE_OBJECT *)psSelected, true);
}


/*Deals with the RMB click for the stats screen */
static void intStatsRMBPressed(UDWORD id)
{
	ASSERT_OR_RETURN(, id - IDSTAT_START < numStatsListEntries, "Invalid range referenced for numStatsListEntries, %d > %d", id - IDSTAT_START, numStatsListEntries);

	if (objMode == IOBJ_MANUFACTURE)
	{
		BASE_STATS *psStats = ppsStatsList[id - IDSTAT_START];

		//this now causes the production run to be decreased by one

		ASSERT_OR_RETURN(, psObjSelected != NULL, "Invalid structure pointer");
		ASSERT_OR_RETURN(, psStats != NULL, "Invalid template pointer");
		if (productionPlayer == (SBYTE)selectedPlayer)
		{
			STRUCTURE *psStructure = (STRUCTURE *)psObjSelected;
			FACTORY  *psFactory = &psStructure->pFunctionality->factory;
			DROID_TEMPLATE *psNext = (DROID_TEMPLATE *)psStats;

			//decrease the production
			factoryProdAdjust(psStructure, psNext, false);

			//need to check if this was the template that was mid-production
			if (getProduction(psStructure, FactoryGetTemplate(psFactory)).numRemaining() == 0)
			{
				doNextProduction(psStructure, FactoryGetTemplate(psFactory), ModeQueue);
				psNext = FactoryGetTemplate(psFactory);
			}
			else if (!StructureIsManufacturingPending(psStructure))
			{
				structSetManufacture(psStructure, psNext, ModeQueue);
			}

			if (StructureIsOnHoldPending(psStructure))
			{
				releaseProduction(psStructure, ModeQueue);
			}

			// Reset the button on the object form
			intSetStats(objStatID, (BASE_STATS *)psNext);
		}
	}
}

/*Deals with the RMB click for the Object screen */
static void intObjectRMBPressed(UDWORD id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStructure;

	ASSERT(id - IDOBJ_OBJSTART < apsObjectList.size(), "intObjectRMBPressed: Invalid object id");

	/* Find the object that the ID refers to */
	psObj = intGetObject(id);
	if (psObj)
	{
		//don't jump around when offworld
		if (psObj->type == OBJ_STRUCTURE && !offWorldKeepLists)
		{
			psStructure = (STRUCTURE *)psObj;
			if (psStructure->pStructureType->type == REF_FACTORY ||
			    psStructure->pStructureType->type == REF_CYBORG_FACTORY ||
			    psStructure->pStructureType->type == REF_VTOL_FACTORY)
			{
				//centre the view on the delivery point
				setViewPos(map_coord(((FACTORY *)psStructure->pFunctionality)->psAssemblyPoint->coords.x),
				        map_coord(((FACTORY *)psStructure->pFunctionality)->psAssemblyPoint->coords.y),
				        true);
			}
		}
	}
}

/*Deals with the RMB click for the Object Stats buttons */
static void intObjStatRMBPressed(UDWORD id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStructure;

	ASSERT(id - IDOBJ_STATSTART < apsObjectList.size(), "intObjStatRMBPressed: Invalid stat id");

	/* Find the object that the ID refers to */
	psObj = intGetObject(id);
	if (!psObj)
	{
		return;
	}
	intResetWindows(psObj);
	if (psObj->type == OBJ_STRUCTURE)
	{
		psStructure = (STRUCTURE *)psObj;
		if (StructIsFactory(psStructure))
		{
			//check if active
			if (StructureIsManufacturingPending(psStructure))
			{
				//if not curently on hold, set it
				if (!StructureIsOnHoldPending(psStructure))
				{
					holdProduction(psStructure, ModeQueue);
				}
				else
				{
					//cancel if have RMB-clicked twice
					cancelProduction(psStructure, ModeQueue);
					//play audio to indicate cancelled
					audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
				}
			}
		}
		else if (psStructure->pStructureType->type == REF_RESEARCH)
		{
			//check if active
			if (structureIsResearchingPending(psStructure))
			{
				//if not curently on hold, set it
				if (!StructureIsOnHoldPending(psStructure))
				{
					holdResearch(psStructure, ModeQueue);
				}
				else
				{
					//cancel if have RMB-clicked twice
					cancelResearch(psStructure, ModeQueue);
					//play audio to indicate cancelled
					audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
				}
			}
		}
	}
}


//sets up the Intelligence Screen as far as the interface is concerned
void addIntelScreen(void)
{
	bool	radOnScreen;

	if (driveModeActive() && !driveInterfaceEnabled())
	{
		driveDisableControl();
		driveEnableInterface(true);
	}

	intResetScreen(false);

	//lock the reticule button
	widgSetButtonState(psWScreen, IDRET_INTEL_MAP, WBUT_CLICKLOCK);

	//add the power bar - for looks!
	intShowPowerBar();

	// Only do this in main game.
	if ((GetGameMode() == GS_NORMAL) && !bMultiPlayer)
	{
		radOnScreen = radarOnScreen;

		bRender3DOnly = true;
		radarOnScreen = false;

		// Just display the 3d, no interface
		displayWorld();

		// Upload the current display back buffer into system memory.
		pie_UploadDisplayBuffer();

		radarOnScreen = radOnScreen;
		bRender3DOnly = false;
	}

	//add all the intelligence screen interface
	(void)intAddIntelMap();
	intMode = INT_INTELMAP;
}

//sets up the Transporter Screen as far as the interface is concerned
void addTransporterInterface(DROID *psSelected, bool onMission)
{
	// if psSelected = NULL add interface but if psSelected != NULL make sure its not flying
	if (!psSelected || (psSelected && !transporterFlying(psSelected)))
	{
		intResetScreen(false);
		intAddTransporter(psSelected, onMission);
		intMode = INT_TRANSPORTER;
	}
}

/*sets which list of structures to use for the interface*/
STRUCTURE *interfaceStructList(void)
{
	if (offWorldKeepLists)
	{
		return mission.apsStructLists[selectedPlayer];
	}
	else
	{
		return apsStructLists[selectedPlayer];
	}
}


/*causes a reticule button to start flashing*/
void flashReticuleButton(UDWORD buttonID)
{
	W_TABFORM		*psButton;

	//get the button for the id
	psButton = (W_TABFORM *)widgGetFromID(psWScreen, buttonID);
	if (psButton)
	{
		//set flashing byte to true
		psButton->UserData = (1 << 24) | psButton->UserData;
	}
}

// stop a reticule button flashing
void stopReticuleButtonFlash(UDWORD buttonID)
{
	WIDGET	*psButton = widgGetFromID(psWScreen, buttonID);
	if (psButton)
	{
		UBYTE DownTime = UNPACKDWORD_QUAD_C(psButton->UserData);
		UBYTE Index = UNPACKDWORD_QUAD_D(psButton->UserData);
		// clear flashing byte
		UBYTE flashing = false;
		UBYTE flashTime = 0;
		psButton->UserData = PACKDWORD_QUAD(flashTime, flashing, DownTime, Index);
	}
}

//displays the Power Bar
void intShowPowerBar(void)
{
	//if its not already on display
	if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
	{
		widgReveal(psWScreen, IDPOW_POWERBAR_T);
	}
}

//hides the power bar from the display - regardless of what player requested!
void forceHidePowerBar(void)
{
	if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
	{
		widgHide(psWScreen, IDPOW_POWERBAR_T);
	}
}


/* Add the Proximity message buttons */
bool intAddProximityButton(PROXIMITY_DISPLAY *psProxDisp, UDWORD inc)
{
	PROXIMITY_DISPLAY	*psProxDisp2;
	UDWORD				cnt;

	W_FORMINIT sBFormInit;
	sBFormInit.formID = 0;
	sBFormInit.id = IDPROX_START + inc;
	//store the ID so we can detect which one has been clicked on
	psProxDisp->buttonID = sBFormInit.id;

	// loop back and find a free one!
	if (sBFormInit.id >= IDPROX_END)
	{
		for (cnt = IDPROX_START; cnt < IDPROX_END; cnt++)
		{
			// go down the prox msgs and see if it's free.
			for (psProxDisp2 = apsProxDisp[selectedPlayer]; psProxDisp2 && psProxDisp2->buttonID != cnt; psProxDisp2 = psProxDisp2->psNext) {}

			if (psProxDisp2 == NULL)	// value was unused.
			{
				sBFormInit.id = cnt;
				break;
			}
		}
		ASSERT_OR_RETURN(false, cnt != IDPROX_END, "Ran out of proximity displays");
	}
	ASSERT(sBFormInit.id < IDPROX_END, "Invalid proximity message button ID %d", (int)sBFormInit.id);

	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	sBFormInit.width = PROX_BUTWIDTH;
	sBFormInit.height = PROX_BUTHEIGHT;
	//the x and y need to be set up each time the button is drawn - see intDisplayProximityBlips

	sBFormInit.pDisplay = intDisplayProximityBlips;
	//set the data for this button
	sBFormInit.pUserData = psProxDisp;

	if (!widgAddForm(psWScreen, &sBFormInit))
	{
		return false;
	}
	return true;
}


/*Remove a Proximity Button - when the message is deleted*/
void intRemoveProximityButton(PROXIMITY_DISPLAY *psProxDisp)
{
	ASSERT(psProxDisp->buttonID >= IDPROX_START && psProxDisp->buttonID <= IDPROX_END, "Invalid proximity ID");
	widgDelete(psWScreen, psProxDisp->buttonID);
}

/*deals with the proximity message when clicked on*/
void processProximityButtons(UDWORD id)
{
	PROXIMITY_DISPLAY	*psProxDisp;

	if (!doWeDrawProximitys())
	{
		return;
	}

	//find which proximity display this relates to
	psProxDisp = NULL;
	for (psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp; psProxDisp = psProxDisp->psNext)
	{
		if (psProxDisp->buttonID == id)
		{
			break;
		}
	}
	if (psProxDisp)
	{
		//if not been read - display info
		if (!psProxDisp->psMessage->read)
		{
			displayProximityMessage(psProxDisp);
		}
	}
}

/*	Fools the widgets by setting a key value */
void	setKeyButtonMapping(UDWORD	val)
{
	keyButtonMapping = val;
}


/*Looks through the players list of structures to see if there is one selected
of the required type. If there is more than one, they are all deselected and
the first one reselected*/
STRUCTURE *intCheckForStructure(UDWORD structType)
{
	STRUCTURE	*psStruct, *psSel = NULL;

	for (psStruct = interfaceStructList(); psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (psStruct->selected && psStruct->pStructureType->type == structType && psStruct->status == SS_BUILT)
		{
			if (psSel != NULL)
			{
				clearSelection();
				psSel->selected = true;
				break;
			}
			psSel = psStruct;
		}
	}

	return psSel;
}

/*Looks through the players list of droids to see if there is one selected
of the required type. If there is more than one, they are all deselected and
the first one reselected*/
// no longer do this for constructor droids - (gleeful its-near-the-end-of-the-project hack - JOHN)
DROID *intCheckForDroid(UDWORD droidType)
{
	DROID	*psDroid, *psSel = NULL;

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if (psDroid->selected && psDroid->droidType == droidType)
		{
			if (psSel != NULL)
			{
				if (droidType != DROID_CONSTRUCT
				    && droidType != DROID_CYBORG_CONSTRUCT)
				{
					clearSelection();
				}
				SelectDroid(psSel);
				break;
			}
			psSel = psDroid;
		}
	}

	return psSel;
}


// count the number of selected droids of a type
static SDWORD intNumSelectedDroids(UDWORD droidType)
{
	DROID	*psDroid;
	SDWORD	num;

	num = 0;
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected && psDroid->droidType == droidType)
		{
			num += 1;
		}
	}

	return num;
}

// Check that each reticule button has the structure or droid required for it and
// enable/disable accordingly.
//
void intCheckReticuleButtons(void)
{
	STRUCTURE	*psStruct;
	DROID	*psDroid;
	int i;

	ReticuleEnabled[RETBUT_CANCEL].Enabled = true;
	ReticuleEnabled[RETBUT_FACTORY].Enabled = editMode;
	ReticuleEnabled[RETBUT_RESEARCH].Enabled = false;
	ReticuleEnabled[RETBUT_BUILD].Enabled = editMode;
	ReticuleEnabled[RETBUT_DESIGN].Enabled = allowDesign;
	ReticuleEnabled[RETBUT_INTELMAP].Enabled = true;
	ReticuleEnabled[RETBUT_COMMAND].Enabled = false;

	for (psStruct = interfaceStructList(); psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (psStruct->status == SS_BUILT)
		{
			switch (psStruct->pStructureType->type)
			{
			case REF_RESEARCH:
				if (!missionLimboExpand())
				{
					ReticuleEnabled[RETBUT_RESEARCH].Enabled = true;
				}
				break;
			case REF_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
				if (!missionLimboExpand())
				{
					ReticuleEnabled[RETBUT_FACTORY].Enabled = true;
				}
				break;
			default:
				break;
			}
		}
	}

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid =
	        psDroid->psNext)
	{
		switch (psDroid->droidType)
		{
		case DROID_CONSTRUCT:
		case DROID_CYBORG_CONSTRUCT:
			ReticuleEnabled[RETBUT_BUILD].Enabled = true;
			break;
		case DROID_COMMAND:
			ReticuleEnabled[RETBUT_COMMAND].Enabled = true;
			break;
		default:
			break;
		}
	}

	for (i = 0; i < NUMRETBUTS; i++)
	{
		WIDGET *psWidget = widgGetFromID(psWScreen, ReticuleEnabled[i].id);

		if (psWidget != NULL)
		{
			if (psWidget->type != WIDG_LABEL)
			{
				if (ReticuleEnabled[i].Enabled)
				{
					widgSetButtonState(psWScreen, ReticuleEnabled[i].id, 0);
				}
				else
				{
					widgSetButtonState(psWScreen, ReticuleEnabled[i].id, WBUT_DISABLE);
				}

				if (ReticuleEnabled[i].Hidden)
				{
					widgHide(psWScreen, ReticuleEnabled[i].id);
				}
				else
				{
					widgReveal(psWScreen, ReticuleEnabled[i].id);
				}
			}
		}
	}
}

/*Checks to see if there are any research topics to do and flashes the button -
only if research facility is free*/
int intGetResearchState()
{
	bool resFree = false;
	for (STRUCTURE *psStruct = interfaceStructList(); psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_RESEARCH &&
		    psStruct->status == SS_BUILT &&
		    getResearchStats(psStruct) == NULL)
		{
			resFree = true;
			break;
		}

	}

	int count = 0;
	if (resFree)
	{
		//set to value that won't be reached in fillResearchList
		int index = asResearch.size() + 1;
		//calculate the list
		int preCount = fillResearchList(pList, selectedPlayer, index, MAXRESEARCH);
		count = preCount;
		for (int n = 0; n < preCount; ++n)
		{
			for (int player = 0; player < MAX_PLAYERS; ++player)
			{
				if (aiCheckAlliances(player, selectedPlayer) && IsResearchStarted(&asPlayerResList[player][pList[n]]))
				{
					--count;  // An ally is already researching this topic, so don't flash the button because of it.
					break;
				}
			}
		}
	}

	return count;
}

void intNotifyResearchButton(int prevState)
{
	int newState = intGetResearchState();
	if (newState > prevState)
	{
		// Set the research reticule button to flash.
		flashReticuleButton(IDRET_RESEARCH);
	}
	else if (newState == 0 && prevState > 0)
	{
		stopReticuleButtonFlash(IDRET_RESEARCH);
	}
}

// see if a reticule button is enabled
bool intCheckReticuleButEnabled(UDWORD id)
{
	for (int i = 0; i < NUMRETBUTS; i++)
	{
		if (ReticuleEnabled[i].id == id)
		{
			return ReticuleEnabled[i].Enabled;
		}
	}
	return false;
}

// Find any structure. Returns NULL if none found.
//
STRUCTURE *intFindAStructure(void)
{
	STRUCTURE *Struct;

	// First try and find a factory.
	Struct = intGotoNextStructureType(REF_FACTORY, false, false);
	if (Struct == NULL)
	{
		// If that fails then look for a command center.
		Struct = intGotoNextStructureType(REF_HQ, false, false);
		if (Struct == NULL)
		{
			// If that fails then look for a any structure.
			Struct = intGotoNextStructureType(REF_ANY, false, false);
		}
	}

	return Struct;
}


// Look through the players structures and find the next one of type structType.
//
STRUCTURE *intGotoNextStructureType(UDWORD structType, bool JumpTo, bool CancelDrive)
{
	STRUCTURE	*psStruct;
	bool Found = false;

	if ((SWORD)structType != CurrentStructType)
	{
		CurrentStruct = NULL;
		CurrentStructType = (SWORD)structType;
	}

	if (CurrentStruct != NULL)
	{
		psStruct = CurrentStruct;
	}
	else
	{
		psStruct = interfaceStructList();
	}

	for (; psStruct != NULL; psStruct = psStruct->psNext)
	{
		if ((psStruct->pStructureType->type == structType || structType == REF_ANY) && psStruct->status == SS_BUILT)
		{
			if (psStruct != CurrentStruct)
			{
				if (CancelDrive)
				{
					clearSelection();
				}
				else
				{
					clearSel();
				}
				psStruct->selected = true;
				CurrentStruct = psStruct;
				Found = true;
				break;
			}
		}
	}

	// Start back at the begining?
	if ((!Found) && (CurrentStruct != NULL))
	{
		for (psStruct = interfaceStructList(); psStruct != CurrentStruct && psStruct != NULL; psStruct = psStruct->psNext)
		{
			if ((psStruct->pStructureType->type == structType || structType == REF_ANY) && psStruct->status == SS_BUILT)
			{
				if (psStruct != CurrentStruct)
				{
					if (CancelDrive)
					{
						clearSelection();
					}
					else
					{
						clearSel();
					}
					psStruct->selected = true;
					CurrentStruct = psStruct;
					break;
				}
			}
		}
	}

	// Center it on screen.
	if ((CurrentStruct) && (JumpTo))
	{
		intSetMapPos(CurrentStruct->pos.x, CurrentStruct->pos.y);
	}

	return CurrentStruct;
}

// Look through the players droids and find the next one of type droidType.
// If Current=NULL then start at the beginning overwise start at Current.
//
DROID *intGotoNextDroidType(DROID *CurrDroid, DROID_TYPE droidType, bool AllowGroup)
{
	DROID *psDroid;
	bool Found = false;

	if (CurrDroid != NULL)
	{
		CurrentDroid = CurrDroid;
	}

	if (droidType != CurrentDroidType && droidType != DROID_ANY)
	{
		CurrentDroid = NULL;
		CurrentDroidType = droidType;
	}

	if (CurrentDroid != NULL)
	{
		psDroid = CurrentDroid;
	}
	else
	{
		psDroid = apsDroidLists[selectedPlayer];
	}

	for (; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if ((psDroid->droidType == droidType
		     || (droidType == DROID_ANY && (psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)))
		    && (psDroid->group == UBYTE_MAX || AllowGroup))
		{
			if (psDroid != CurrentDroid)
			{
				clearSel();
				SelectDroid(psDroid);
				CurrentDroid = psDroid;
				Found = true;
				break;
			}
		}
	}

	// Start back at the begining?
	if ((!Found) && (CurrentDroid != NULL))
	{
		for (psDroid = apsDroidLists[selectedPlayer]; (psDroid != CurrentDroid) && (psDroid != NULL); psDroid = psDroid->psNext)
		{
			if ((psDroid->droidType == droidType ||
			     ((droidType == DROID_ANY) && (psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER))) &&
			    ((psDroid->group == UBYTE_MAX) || AllowGroup))
			{
				if (psDroid != CurrentDroid)
				{
					clearSel();
					SelectDroid(psDroid);
					CurrentDroid = psDroid;
					Found = true;
					break;
				}
			}
		}
	}

	if (Found == true)
	{
		psCBSelectedDroid = CurrentDroid;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_SELECTED);
		psCBSelectedDroid = NULL;

		// Center it on screen.
		if (CurrentDroid)
		{
			intSetMapPos(CurrentDroid->pos.x, CurrentDroid->pos.y);
		}
		return CurrentDroid;
	}
	return NULL;
}

//access function for selected object in the interface
BASE_OBJECT *getCurrentSelected(void)
{
	return psObjSelected;
}

// Checks if a coordinate is over the build menu
bool CoordInBuild(int x, int y)
{
	// This measurement is valid for the menu, so the buildmenu_height
	// value is used to "nudge" it all upwards from the command menu.
	// FIXME: hardcoded value (?)
	const int buildmenu_height = 300;
	Vector2f pos;

	pos.x = x - RET_X;
	pos.y = y - RET_Y + buildmenu_height; // guesstimation

	if ((pos.x < 0 || pos.y < 0 || pos.x >= RET_FORMWIDTH || pos.y >= buildmenu_height) || !SecondaryWindowUp)
	{
		return false;
	}

	return true;
}
