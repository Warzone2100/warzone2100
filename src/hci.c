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
/**
 * @file hci.c
 *
 * Functions for the in game interface.
 * (Human Computer Interface - thanks to Alex for the file name).
 *
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piepalette.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"
#include "lib/script/script.h"

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
#include "seqdisp.h"
#include "transporter.h"
#include "warcam.h"
#include "main.h"
#include "wrappers.h"

//#define DEBUG_SCROLLTABS 	//enable to see tab scroll button info for buttons

//#define EDIT_OPTIONS
#ifdef EDIT_OPTIONS
static UDWORD		newMapWidth, newMapHeight;
#endif

#define RETXOFFSET (0)// Reticule button offset
#define RETYOFFSET (0)
#define NUMRETBUTS	7 // Number of reticule buttons.

enum {				  // Reticule button indecies.
	RETBUT_CANCEL,
	RETBUT_FACTORY,
	RETBUT_RESEARCH,
	RETBUT_BUILD,
	RETBUT_DESIGN,
	RETBUT_INTELMAP,
	RETBUT_COMMAND,
};

typedef struct {
	UDWORD id;
	BOOL Enabled;
	BOOL Hidden;
} BUTSTATE;

typedef struct {
	SWORD x;
	SWORD y;
} BUTOFFSET;

BUTOFFSET ReticuleOffsets[NUMRETBUTS] = {	// Reticule button form relative positions.
	{48,49},	// RETBUT_CANCEL,
	{53,17},	// RETBUT_FACTORY,
	{87,35},	// RETBUT_RESEARCH,
	{87,70},	// RETBUT_BUILD,
	{53,88},	// RETBUT_DESIGN,
	{19,70},	// RETBUT_INTELMAP,
	{19,35},	// RETBUT_COMMAND,
};

BUTSTATE ReticuleEnabled[NUMRETBUTS] = {	// Reticule button enable states.
	{IDRET_CANCEL,FALSE,FALSE},
	{IDRET_MANUFACTURE,FALSE,FALSE},
	{IDRET_RESEARCH,FALSE,FALSE},
	{IDRET_BUILD,FALSE,FALSE},
	{IDRET_DESIGN,FALSE,FALSE},
	{IDRET_INTEL_MAP,FALSE,FALSE},
	{IDRET_COMMAND,FALSE,FALSE},
};


// Set the x,y members of a button widget initialiser given a reticule button index.
//
static void SetReticuleButPos(UWORD ButId, W_BUTINIT *sButInit)
{
	ASSERT( ButId < NUMRETBUTS,"SetReticuleButPos : Bad button index" );

	sButInit->x = (SWORD)(ReticuleOffsets[ButId].x + RETXOFFSET);
	sButInit->y = (SWORD)(ReticuleOffsets[ButId].y + RETYOFFSET);
}


static BOOL ClosingObject = FALSE;
static BOOL ClosingStats = FALSE;
static UDWORD	keyButtonMapping = 0;
BOOL ClosingMessageView = FALSE;
BOOL ClosingIntelMap = FALSE;
BOOL ClosingOrder = FALSE;
BOOL ClosingTrans = FALSE;
BOOL ClosingTransCont = FALSE;
BOOL ClosingTransDroids = FALSE;
BOOL ReticuleUp = FALSE;
BOOL Refreshing = FALSE;


/***************************************************************************************/
/*                  Widget ID numbers                                                  */

/* Reticule ID's */
//#define IDRET_FORM			1		// The reticule form
/* defined in HCI.h now
#define IDRET_OPTIONS		2		// option button
#define IDRET_BUILD			3		// build button
#define IDRET_MANUFACTURE	4		// manufacture button
#define IDRET_RESEARCH		5		// research button
#define IDRET_INTEL_MAP		6		// intelligence map button
#define IDRET_DESIGN		7		// design droids button
#define IDRET_CANCEL		8		// central cancel button
*/

#define IDPOW_FORM			100		// power bar form

/* Option screen IDs */
#define IDOPT_FORM			1000		// The option form
#define IDOPT_MAPFORM		1001
#define IDOPT_MAPLOAD		1002		// The load map button
#define IDOPT_MAPSAVE		1003		// The save map button
#define IDOPT_MAPLABEL		1004		// The map label
#define IDOPT_EDIT			1005		// The edit mode toggle
#define IDOPT_CLOSE			1006		// The close button
#define IDOPT_LABEL			1007		// The Option screen label
#define IDOPT_PLAYERFORM	1008		// The player button form
#define IDOPT_PLAYERLABEL	1009		// The player form label
#define IDOPT_PLAYERSTART	1010		// The first player button
#define IDOPT_PLAYEREND		1030		// The last possible player button
#define IDOPT_QUIT			1031		// Quit the game
#define IDOPT_MAPNEW		1032		// The new map button
#define IDOPT_MAPWIDTH		1033		// The edit box for the map width
#define IDOPT_MAPHEIGHT		1034		// The edit box for the map height
#define IDOPT_DROID			1037		// The place droid button
#define IDOPT_STRUCT		1038		// The place struct button
#define IDOPT_FEATURE		1039		// The place feature button

/* Edit screen IDs */
#define IDED_FORM			2000		// The edit form
#define IDED_LABEL			2001		// The edit screen label
#define IDED_CLOSE			2002		// The edit screen close box
#define	IDED_STATFORM		2003		// The edit screen stats form (for droids/structs/features)



//Design Screen uses		5000
//Intelligence Map uses		6000
//Droid order screen uses	8000
//Transporter screen uses	9000
//CD span screen uses		9800

//MultiPlayer Frontend uses 10100/10200/10300/10400
//Ingame Options use		10500
//Ingame MultiMenu uses		10600

//Mission Timer uses		11000
//Frontend uses				20000
//LOADSAVE uses				21000
//MULTILIMITS uses			22000

#define	IDPROX_START		12000		//The first proximity button
#define	IDPROX_END			12019		//The last proximity button - max of 20

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
#define OPT_EDITY		100
#define OPT_PLAYERY		150
#define OPT_LOADY		260

/* Edit positions */
#define ED_X			32
#define ED_Y			200
#define ED_WIDTH		80
#define ED_HEIGHT		105
#define ED_GAP			5
#define ED_BUTWIDTH		60
#define ED_BUTHEIGHT	20


#define	STAT_TABOFFSET			2
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

};

/* The widget screen */
W_SCREEN		*psWScreen;

//two colours used for drawing the footprint outline for objects in 2D
PIELIGHT			outlineOK;
PIELIGHT			outlineNotOK;
BOOL				outlineTile = FALSE;

// The last widget ID from widgRunScreen
UDWORD				intLastWidget;

INTMODE intMode;

/* Which type of object is being displayed on the edit stats screen */
enum _edit_obj_mode
{
	IED_DROID,		// Droids
	IED_STRUCT,		// Structures
	IED_FEATURE,	// Features
} editObjMode;

/* Status of the positioning for the object placement */
enum _edit_pos_mode
{
	IED_NOPOS,
	IED_POS,
} editPosMode;

/* Which type of object screen is being displayed. Starting value is where the intMode left off*/
enum _obj_mode
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
typedef BOOL (* OBJ_SELECT)(BASE_OBJECT *psObj);
/* Function type for getting the appropriate stats for an object */
typedef BASE_STATS *(* OBJ_GETSTATS)(BASE_OBJECT *psObj);
/* Function type for setting the appropriate stats for an object */
typedef BOOL (* OBJ_SETSTATS)(BASE_OBJECT *psObj, BASE_STATS *psStats);

/* The current object list being used by the object screen */
static BASE_OBJECT		*psObjList;

/* functions to select and get stats from the current object list */
static OBJ_SELECT		objSelectFunc;
OBJ_GETSTATS		objGetStatsFunc;
static OBJ_SETSTATS		objSetStatsFunc;

/* Whether the objects that are on the object screen have changed this frame */
static BOOL				objectsChanged;

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
DROID_TEMPLATE			**apsTemplateList;
DROID_TEMPLATE			*psCurrTemplate = NULL;

/* Store a list of Feature pointers for features to be placed on the map */
static FEATURE_STATS	**apsFeatureList;

/*Store a list of research indices which can be performed*/
static UWORD			*pList;
static UWORD			*pSList;

/* Store a list of component stats pointers for the design screen */
UDWORD			numComponent;
COMP_BASE_STATS	**apsComponentList;
UDWORD			numExtraSys;
COMP_BASE_STATS	**apsExtraSysList;

//defined in HCI.h now
// store the objects that are being used for the object bar
//#define			MAX_OBJECTS		15//10 we need at least 15 for the 3 different types of factory
BASE_OBJECT		**apsObjectList;
SDWORD			numObjects;
//this list is used for sorting the objects - at the mo' this is just factories
BASE_OBJECT		**apsListToOrder;
/*max size required to store unordered factories */
#define			ORDERED_LIST_SIZE		(NUM_FACTORY_TYPES * MAX_FACTORY)


/* The current design being edited on the design screen */
extern DROID_TEMPLATE	sCurrDesign;

/* The button id of the component that is in the design */
//UDWORD			desCompID;

/* The button id of the droid template that has been locked down */
//UDWORD			droidTemplID;

/* Flags to check whether the power bars are currently on the screen */
static BOOL				powerBarUp = FALSE;
static BOOL				StatsUp = FALSE;
static BASE_OBJECT		*psStatsScreenOwner = NULL;

#ifdef INCLUDE_PRODSLIDER
// Size of a production run for manufacturing.
static UBYTE			ProductionRun = 1;
#endif

/* pointer to hold the imd to use for a new template in the design screen */
//iIMDShape	*pNewDesignIMD = NULL;

/* The previous object for each object bar */
static BASE_OBJECT		*apsPreviousObj[IOBJ_MAX];

/* The jump position for each object on the base bar */
static Vector2i asJumpPos[IOBJ_MAX];

// whether to reopen the build menu
// chnaged back to pre Mark Donald setting at Jim's request - AlexM
static BOOL				bReopenBuildMenu = FALSE;

/***************************************************************************************/
/*              Function Prototypes                                                    */
static BOOL intUpdateObject(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected,BOOL bForceStats);
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
static BOOL intAddStats(BASE_STATS **ppsStatsList, UDWORD numStats,
						BASE_STATS *psSelected, BASE_OBJECT *psOwner);
/* Process return codes from the stats screen */
static void intProcessStats(UDWORD id);
// clean up when an object dies
static void intObjectDied(UDWORD objID);

/* Add the build widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static BOOL intAddBuild(DROID *psSelected);
/* Add the manufacture widgets to the widget screen */
/* If psSelected != NULL it specifies which factory should be hilited */
static BOOL intAddManufacture(STRUCTURE *psSelected);
/* Add the research widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static BOOL intAddResearch(STRUCTURE *psSelected);
/* Add the command droid widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static BOOL intAddCommand(DROID *psSelected);


/* Start looking for a structure location */
static void intStartStructPosition(BASE_STATS *psStats,DROID *psDroid);
/* Stop looking for a structure location */
static void intStopStructPosition(void);
/* See if a structure location has been found */
static BOOL intGetStructPosition(UDWORD *pX, UDWORD *pY);

static STRUCTURE *CurrentStruct = NULL;
static SWORD CurrentStructType = 0;
static DROID *CurrentDroid = NULL;
static SWORD CurrentDroidType = 0;

/******************Power Bar Stuff!**************/
/* Add the power bars */
static BOOL intAddPower(void);

/* Remove the power bars */
//static void intRemovePower(void);

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

static void intInitialiseReticule(void);
static DROID* intCheckForDroid(UDWORD droidType);
static STRUCTURE* intCheckForStructure(UDWORD structType);

static void intCheckReticuleButtons(void);

// count the number of selected droids of a type
static SDWORD intNumSelectedDroids(UDWORD droidType);


/***************************GAME CODE ****************************/
/* Initialise the in game interface */
BOOL intInitialise(void)
{
	UDWORD			comp, inc;

	intInitialiseReticule();

	widgSetTipColour(psWScreen, WZCOL_TOOLTIP_TEXT);

	if(GetGameMode() == GS_NORMAL) {
//		WidgSetAudio(WidgetAudioCallback,ID_SOUND_HILIGHTBUTTON,ID_SOUND_SELECT);
		WidgSetAudio(WidgetAudioCallback,-1,ID_SOUND_SELECT);
	} else {
//		WidgSetAudio(WidgetAudioCallback,FE_AUDIO_HILIGHTBUTTON,FE_AUDIO_SELECTBUT);
		WidgSetAudio(WidgetAudioCallback,-1,ID_SOUND_SELECT);
	}

	/* Create storage for Structures that can be built */
	apsStructStatsList = (STRUCTURE_STATS **)malloc(sizeof(STRUCTURE_STATS *) *
		MAXSTRUCTURES);
	if (!apsStructStatsList)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	//create the storage for Research topics - max possible size
	ppResearchList = (RESEARCH **) malloc(sizeof(RESEARCH *) * MAXRESEARCH);
	if (ppResearchList == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for research list" );
		abort();
		return FALSE;
	}

	//create the list for the selected player
	//needs to be UWORD sized for Patches
    pList = (UWORD *) malloc(sizeof (UWORD) * MAXRESEARCH);
	pSList = (UWORD *) malloc(sizeof (UWORD) * MAXRESEARCH);
    //pList = (UBYTE *) malloc(sizeof (UBYTE) * MAXRESEARCH);
	//pSList = (UBYTE *) malloc(sizeof (UBYTE) * MAXRESEARCH);

	if (pList == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for research list" );
		abort();
		return FALSE;
	}
	if (pSList == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for sorted research list" );
		abort();
		return FALSE;
	}

	/* Create storage for Templates that can be built */
	apsTemplateList = (DROID_TEMPLATE **)malloc(sizeof(DROID_TEMPLATE*) *
		MAXTEMPLATES);
	if (apsTemplateList == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for template list" );
		abort();
		return FALSE;
	}

	if(GetGameMode() == GS_NORMAL) {
		//load up the 'blank' template imd


/*
		if (pNewDesignIMD == NULL)
		{
			DBERROR(("Unable to load Blank Template IMD"));
			return FALSE;
		}
		*/
	}

	/* Create storage for the feature list */
	apsFeatureList = (FEATURE_STATS **)malloc(sizeof(FEATURE_STATS *) *
		MAXFEATURES);
	if (apsFeatureList == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for feature list" );
		abort();
		return FALSE;
	}

	/* Create storage for the component list */
	apsComponentList = (COMP_BASE_STATS **)malloc(sizeof(COMP_BASE_STATS *) *
		MAXCOMPONENT);
	if (apsComponentList == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for component list" );
		abort();
		return FALSE;
	}

	/* Create storage for the extra systems list */
	apsExtraSysList = (COMP_BASE_STATS **)malloc(sizeof(COMP_BASE_STATS *) *
		MAXEXTRASYS);
	if (apsExtraSysList == NULL)
	{
		debug( LOG_ERROR, "Unable to allocate memory for extra systems list" );
		abort();
		return FALSE;
	}

	// allocate the object list
	apsObjectList = (BASE_OBJECT **)malloc(sizeof(BASE_OBJECT *) * MAX_OBJECTS);
	if (!apsObjectList)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	//allocate the order list - ONLY SIZED FOR FACTORIES AT PRESENT!!
	apsListToOrder = (BASE_OBJECT **)malloc(sizeof(BASE_OBJECT *) * ORDERED_LIST_SIZE);
	if (!apsListToOrder)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}


	LOADBARCALLBACK();	//	loadingScreenCallback();

	intInitialiseGraphics();

	LOADBARCALLBACK();	//	loadingScreenCallback();

	psWScreen = widgCreateScreen();
	if (psWScreen == NULL)
	{
		debug(LOG_ERROR, "intInitialise: Couldn't create widget screen");
		abort();
		return FALSE;
	}

	widgSetTipFont(psWScreen, font_regular);

	if(GetGameMode() == GS_NORMAL) {

		if (!intAddReticule())
		{
			debug( LOG_ERROR, "intInitialise: Couldn't create reticule widgets (Out of memory ?)" );
			abort();
			return FALSE;
		}

		if (!intAddPower())
		{
			debug( LOG_ERROR, "intInitialise: Couldn't create power Bar widget(Out of memory ?)" );
			abort();
			return FALSE;
		}
	}

	/* Initialise the screen to be run */
	widgStartScreen(psWScreen);

	/* Note the current screen state */
	intMode = INT_NORMAL;

	objectsChanged = FALSE;

	//set the default colours to be used for drawing outlines in 2D
	outlineOK = WZCOL_MAP_OUTLINE_OK;
	outlineNotOK = WZCOL_MAP_OUTLINE_BAD;

	LOADBARCALLBACK();	//	loadingScreenCallback();
	LOADBARCALLBACK();	//	loadingScreenCallback();

	// reset the previous objects
	//memset(apsPreviousObj, 0, sizeof(apsPreviousObj));
	intResetPreviousObj();

	// reset the jump positions
	memset(asJumpPos, 0, sizeof(asJumpPos));

	/* make demolish stat always available */
	if(!bInTutorial)
	{
		for (comp=0; comp < numStructureStats; comp++)
		{
			//if (!strcmp(asStructureStats[comp].pName, "Demolish Structure"))
			if (asStructureStats[comp].type == REF_DEMOLISH)
			{
				for (inc = 0; inc < MAX_PLAYERS; inc++)
				{
					apStructTypeLists[inc][comp] = AVAILABLE;
				}
			}
		}
	}



	return TRUE;
}

void intReopenBuild(BOOL reopen)
{
	bReopenBuildMenu = reopen;
}

BOOL intGetReopenBuild(void)
{
	return bReopenBuildMenu;
}

//initialise all the previous obj - particularly useful for when go Off world!
void intResetPreviousObj(void)
{
    //make sure stats screen doesn't think it should be up
    StatsUp = FALSE;
	// reset the previous objects
	memset(apsPreviousObj, 0, sizeof(apsPreviousObj));
}


/* Shut down the in game interface */
void intShutDown(void)
{
//	widgEndScreen(psWScreen);
	widgReleaseScreen(psWScreen);

	free(apsStructStatsList);
	free(ppResearchList);
	free(pList);
	free(pSList);
	free(apsTemplateList);
	free(apsFeatureList);
	free(apsComponentList);
	free(apsExtraSysList);
	free(apsObjectList);
	free(apsListToOrder);

	apsStructStatsList = NULL;
	ppResearchList = NULL;
	pList = NULL;
	pSList = NULL;
	apsTemplateList = NULL;
	apsFeatureList = NULL;
	apsComponentList = NULL;
	apsExtraSysList = NULL;
	apsObjectList = NULL;
	apsListToOrder = NULL;

	intDeleteGraphics();

	//obviously!
	ReticuleUp = FALSE;
}

static BOOL IntRefreshPending = FALSE;

// Set widget refresh pending flag.
//
void intRefreshScreen(void)
{
	IntRefreshPending = TRUE;
}


BOOL intIsRefreshing(void)
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
	UWORD			objMajor=0, objMinor=0, statMajor=0, statMinor=0;
	FLAG_POSITION	*psFlag;

	if(IntRefreshPending) {
		Refreshing = TRUE;

		if (( (intMode == INT_OBJECT) ||
			  (intMode == INT_STAT) ||
			  (intMode == INT_CMDORDER) ||
			  (intMode == INT_ORDER) ||
			  (intMode == INT_TRANSPORTER) ) &&
			(widgGetFromID(psWScreen,IDOBJ_FORM) != NULL) &&
			!(widgGetFromID(psWScreen,IDOBJ_FORM)->style & WIDG_HIDDEN) )
		{
			BOOL StatsWasUp = FALSE;
			BOOL OrderWasUp = FALSE;

			// If the stats form is up then remove it, but remember that it was up.
/*			if(widgGetFromID(psWScreen,IDSTAT_FORM) != NULL) {
				StatsWasUp = TRUE;
//				intRemoveStatsNoAnim();
			}*/
			if ( (intMode == INT_STAT) &&
				 widgGetFromID(psWScreen,IDSTAT_FORM) != NULL )
			{
				StatsWasUp = TRUE;
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
			if (widgGetFromID(psWScreen,IDSTAT_FORM) != NULL )
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
			if ( (intMode == INT_CMDORDER) &&
				 (widgGetFromID(psWScreen,IDORDER_FORM) != NULL) )
			{
				OrderWasUp = TRUE;
			}

	//		if(widgGetFromID(psWScreen,IDOBJ_FORM) != NULL) {
	//			intRemoveObjectNoAnim();
	//		}

			switch(objMode)
			{
			case IOBJ_MANUFACTURE:	// The manufacture screen (factorys on bottom bar)
			case IOBJ_RESEARCH:		// The research screen
				//pass in the currently selected object
				intUpdateObject((BASE_OBJECT *)interfaceStructList(),psObjSelected,
					StatsWasUp);
				break;

			case IOBJ_BUILD:
			case IOBJ_COMMAND:		// the command droid screen
			case IOBJ_BUILDSEL:		// Selecting a position for a new structure
			case IOBJ_DEMOLISHSEL:	// Selecting a structure to demolish
				//pass in the currently selected object
				intUpdateObject((BASE_OBJECT *)apsDroidLists[selectedPlayer],psObjSelected,
					StatsWasUp);
				break;

			default:
				// generic refresh (trouble at the moment, cant just always pass in a null to addobject
				// if object screen is up, refresh it if stats screen is up, refresh that.
				break;
			}

			// set the tabs again
			if (widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL)
			{
				widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor, objMinor);
			}
			if (widgGetFromID(psWScreen,IDSTAT_TABFORM) != NULL )
			{
				widgSetTabs(psWScreen, IDSTAT_TABFORM, statMajor, statMinor);
			}

			if (psFlag != NULL)
			{
				// need to restart the delivery point position
				StartDeliveryPosition( (OBJECT_POSITION *)psFlag );
			}

			// make sure the commander order screen is in the right state
			if ( (intMode == INT_CMDORDER) &&
				 !OrderWasUp &&
				 (widgGetFromID(psWScreen,IDORDER_FORM) != NULL) )
			{
				intRemoveOrderNoAnim();
				widgSetButtonState(psWScreen, statID, 0);
			}
		}

		// Refresh the transporter interface.
		intRefreshTransporter();

		// Refresh the order interface.
		intRefreshOrder();

		Refreshing = FALSE;
	}

	IntRefreshPending = FALSE;
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
//	widgEndScreen(psWScreen);
	widgDelete(psWScreen, IDOPT_FORM);
//	widgStartScreen(psWScreen);
}


#ifdef EDIT_OPTIONS
/* Add the edit widgets to the widget screen */
static BOOL intAddEdit(void)
{
	W_FORMINIT sFormInit;
	W_LABINIT sLabInit;
	W_BUTINIT sButInit;

//	widgEndScreen(psWScreen);

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	memset(&sButInit, 0, sizeof(W_BUTINIT));

	/* Add the edit form */
	sFormInit.formID = 0;
	sFormInit.id = IDED_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = ED_X;
	sFormInit.y = ED_Y;
	sFormInit.width = ED_WIDTH;
	sFormInit.height = ED_HEIGHT;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	/* Add the Option screen label */
	sLabInit.formID = IDED_FORM;
	sLabInit.id = IDED_LABEL;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = ED_GAP;
	sLabInit.y = ED_GAP;
	sLabInit.width = ED_WIDTH;
	sLabInit.height = ED_BUTHEIGHT;
	sLabInit.pText = "Edit";
	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}

	/* Add the close box */
	sButInit.formID = IDED_FORM;
	sButInit.id = IDED_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = ED_WIDTH - ED_GAP - CLOSE_SIZE;
	sButInit.y = ED_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.FontID = font_regular;
	sButInit.pText = pCloseText;
	sButInit.pTip = _("Close");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}
	return TRUE;
}


/* Remove the edit widgets from the widget screen */
static void intRemoveEdit(void)
{
//	widgEndScreen(psWScreen);
	widgDelete(psWScreen, IDED_FORM);
//	widgStartScreen(psWScreen);
}


/* Get  and validate the new map size from the options screen */
static void intGetMapSize(void)
{
	SDWORD editWidth, editHeight;
	char aText[WIDG_MAXSTR];
	UDWORD i, tmp, bitCount;
	BOOL widthChanged = FALSE, heightChanged = FALSE;
	const char *pStr = widgGetString(psWScreen, IDOPT_MAPWIDTH);

	if (isdigit(*pStr))
	{
		// There is a number in the string
		sscanf(pStr, "%d", &editWidth);
	}
	else
	{
		// No number in the string, restore the old value
		editWidth = newMapWidth;
		widthChanged = TRUE;
	}

	// Get the new height
	pStr = widgGetString(psWScreen, IDOPT_MAPHEIGHT);
	if (isdigit(*pStr))
	{
		// There is a number in the string
		sscanf(pStr, "%d", &editHeight);
	}
	else
	{
		// No number in the string, restore the old value
		editHeight = newMapHeight;
		heightChanged = TRUE;
	}

	// now validate the sizes
	if (editWidth <= 0 || editWidth > MAP_MAXWIDTH)
	{
		editWidth = newMapWidth;
		widthChanged = TRUE;
	}
	else
	{
		// Check it is a power of 2
		bitCount = 0;
		tmp = editWidth;
		for (i = 0; i < 32; i++)
		{
			if (tmp & 1)
			{
				bitCount ++;
			}
			tmp = tmp >> 1;
		}
		if (bitCount != 1)
		{
			editWidth = newMapWidth;
			widthChanged = TRUE;
		}
	}
	if (editHeight <= 0 || editHeight > MAP_MAXHEIGHT)
	{
		editHeight = newMapHeight;
		heightChanged = TRUE;
	}
	else
	{
		// Check it is a power of 2
		bitCount = 0;
		tmp = editHeight;
		for (i = 0; i < 32; i++)
		{
			if (tmp & 1)
			{
				bitCount ++;
			}
			tmp = tmp >> 1;
		}
		if (bitCount != 1)
		{
			editHeight = newMapHeight;
			heightChanged = TRUE;
		}
	}

	// Store the new size
	newMapWidth = editWidth;
	newMapHeight = editHeight;

	// Syncronise the edit boxes if necessary
	if (widthChanged)
	{
		sprintf(aText, "%d", newMapWidth);
		widgSetString(psWScreen, IDOPT_MAPWIDTH, aText);
	}
	if (heightChanged)
	{
		sprintf(aText, "%d", newMapHeight);
		widgSetString(psWScreen, IDOPT_MAPHEIGHT, aText);
	}
}
#endif


/* Reset the widget screen to just the reticule */
void intResetScreen(BOOL NoAnim)
{
//	// Ensure driver mode is turned off.
	StopDriverMode();

	if(getWidgetsStatus() == FALSE)
	{
		NoAnim = TRUE;
	}

	if(ReticuleUp) {
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
		if(NoAnim) {
			intRemoveStatsNoAnim();
		} else {
			intRemoveStats();
		}
		break;

#ifdef EDIT_OPTIONS
	case INT_EDIT:
		intRemoveEdit();
		break;
#endif

	case INT_OBJECT:
		intStopStructPosition();
		if(NoAnim) {
			intRemoveObjectNoAnim();
		} else {
			intRemoveObject();
		}
		break;

	case INT_STAT:
		if(NoAnim) {
			intRemoveStatsNoAnim();
			intRemoveObjectNoAnim();
		} else {
			intRemoveStats();
			intRemoveObject();
		}
		break;

	case INT_CMDORDER:
		if(NoAnim) {
			intRemoveOrderNoAnim();
			intRemoveObjectNoAnim();
		} else {
			intRemoveOrder();
			intRemoveObject();
		}
		break;

	case INT_ORDER:
		if(NoAnim) {
			intRemoveOrderNoAnim();
		} else {
			intRemoveOrder();
		}
		break;

	case INT_INGAMEOP:
		if(NoAnim) {
			intCloseInGameOptionsNoAnim(TRUE);
		} else {
			intCloseInGameOptions(FALSE, TRUE);
		}
		break;

	case INT_MISSIONRES:
//		if(NoAnim)	{
			intRemoveMissionResultNoAnim();
//		}else{
//			intRemoveMissionResult();
//		}
		break;


	case INT_MULTIMENU:
		if(NoAnim) {
			intCloseMultiMenuNoAnim();
		} else {
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

						// pc
		if(!bMultiPlayer)
		{
			gameTimeStart();
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

		if(!bMultiPlayer)
		{

			gameTimeStart();

		}

		break;

/*	case INT_TUTORIAL:
		//remove 3dView
		intRemoveMessageView();

		if(!bMultiPlayer)
		{
			gameTimeStart();
		}
		break;*/

	case INT_TRANSPORTER:
		if(NoAnim)
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

	intMode = INT_NORMAL;
    //clearSel() sets IntRefreshPending = TRUE by calling intRefreshScreen() but if we're doing this then we won't need to refresh - hopefully!
	IntRefreshPending = FALSE;
}





// calulate the center world coords for a structure stat given
// top left tile coords
static void intCalcStructCenter(STRUCTURE_STATS *psStats, UDWORD tilex, UDWORD tiley, UDWORD *pcx, UDWORD *pcy)
{
	SDWORD	width, height;

	width = psStats->baseWidth * TILE_UNITS;
	height = psStats->baseBreadth * TILE_UNITS;

	*pcx = tilex * TILE_UNITS + width/2;
	*pcy = tiley * TILE_UNITS + height/2;
}


/* Process return codes from the Options screen */
static void intProcessOptions(UDWORD id)
{
	UDWORD i;
	DROID_TEMPLATE *psTempl;

	if (id >= IDOPT_PLAYERSTART && id <= IDOPT_PLAYEREND)
	{
		widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, 0);
		selectedPlayer = id - IDOPT_PLAYERSTART;
		widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, WBUT_LOCK);
	}
	else
	{
		switch (id)
		{
#ifdef EDIT_OPTIONS
		case IDOPT_MAPLOAD:
			debug(LOG_ERROR, "We should call loadFile and mapLoad here");
			{
				/* Managed to load so quit the option screen */
				intRemoveOptions();
				intMode = INT_NORMAL;
			}
			break;
		case IDOPT_MAPSAVE:
			debug(LOG_ERROR, "We should call writeMapFile here");
			{
				/* Managed to save so quit the option screen */
				intRemoveOptions();
				intMode = INT_NORMAL;
			}
			break;

		case IDOPT_MAPNEW:
			intGetMapSize();
			if (mapNew(newMapWidth, newMapHeight))
			{
				/* Managed to create a new map so quit the option screen */
				intRemoveOptions();
				intMode = INT_NORMAL;
			}
			break;
		case IDOPT_MAPWIDTH:
			intGetMapSize();
			break;
		case IDOPT_MAPHEIGHT:
			intGetMapSize();
			break;
		case IDOPT_EDIT:
			intRemoveOptions();
			intAddEdit();
			intMode = INT_EDIT;
			break;
#endif
			/* The add object buttons */
		case IDOPT_DROID:
			intRemoveOptions();
			i = 0;
			psTempl = apsDroidTemplates[selectedPlayer];
			while ((psTempl != NULL) && (i < MAXTEMPLATES))
			{
				apsTemplateList[i] = psTempl;
				psTempl = psTempl->psNext;
				i++;
			}
			ppsStatsList = (BASE_STATS**)apsTemplateList;
			objMode = IOBJ_MANUFACTURE;
			intAddStats(ppsStatsList, i, NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
//			widgSetButtonState(psWScreen, IDRET_OPTIONS, 0);
			break;
		case IDOPT_STRUCT:
			intRemoveOptions();
			for (i = 0; i < numStructureStats && i < MAXSTRUCTURES; i++)
			{
				apsStructStatsList[i] = asStructureStats + i;
			}
			ppsStatsList = (BASE_STATS**)apsStructStatsList;
			objMode = IOBJ_BUILD;
			intAddStats(ppsStatsList, i, NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
//			widgSetButtonState(psWScreen, IDRET_OPTIONS, 0);
			break;
		case IDOPT_FEATURE:
			intRemoveOptions();
			for (i = 0; i < numFeatureStats && i < MAXFEATURES; i++)
			{
				apsFeatureList[i] = asFeatureStats + i;
			}
			ppsStatsList = (BASE_STATS**)apsFeatureList;
			intAddStats(ppsStatsList, i, NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
//			widgSetButtonState(psWScreen, IDRET_OPTIONS, 0);
			break;
			/* Close window buttons */
		case IDOPT_CLOSE:
			intRemoveOptions();
			intMode = INT_NORMAL;
//			widgSetButtonState(psWScreen, IDRET_OPTIONS, 0);
			break;
			/* Ignore these */
		case IDOPT_FORM:
		case IDOPT_LABEL:
		case IDOPT_MAPFORM:
		case IDOPT_MAPLABEL:
		case IDOPT_PLAYERFORM:
		case IDOPT_PLAYERLABEL:
			break;
		default:
			ASSERT( FALSE, "intProcessOptions: Unknown return code" );
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
		/*if it is a structure - need to check there is enough power available
		to build */
		if (psPositionStats->ref >= REF_STRUCTURE_START &&
		    psPositionStats->ref < REF_STRUCTURE_START + REF_RANGE)
		{
			if (!checkPower(selectedPlayer, ((STRUCTURE_STATS*)psPositionStats)->powerToBuild, TRUE))
			{
				return;
			}
		}
		/*if it is a template - need to check there is enough power available
		to build */
		if (psPositionStats->ref >= REF_TEMPLATE_START &&
		    psPositionStats->ref < REF_TEMPLATE_START + REF_RANGE)
		{
			if (!checkPower(selectedPlayer, ((DROID_TEMPLATE*)psPositionStats)->powerPoints, TRUE))
			{
				return;
			}
		}
		intStartStructPosition(psPositionStats, NULL);
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
		psTForm->TabMultiplier -=1;				// -1 since user hit left button
		if (psTForm->TabMultiplier < 1 )
		{
			psTForm->TabMultiplier = 1;			//Must be at least 1.
		}
		// add routine to update tab widgets now...
		temp = psTForm->majorT;					//set tab # to previous "page"
		temp -=TAB_SEVEN;						//7 = 1 "page" of tabs
		if ( temp < 0)
			psTForm->majorT = 0;
		else
			psTForm->majorT = temp;
#ifdef  DEBUG_SCROLLTABS
		sprintf(buf,"[debug menu]Clicked LT %d tab #=%d",psTForm->TabMultiplier,psTForm->majorT);
		addConsoleMessage(buf,DEFAULT_JUSTIFY);
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
		numTabs = numForms(psTForm->numStats,psTForm->numButtons);
		numTabs = ((numTabs /TAB_SEVEN) + 1);	// (Total tabs needed / 7(max tabs that fit))+1
		psTForm->TabMultiplier += 1;
		if (psTForm->TabMultiplier > numTabs)			// add 'Bzzt' sound effect?
		{
			psTForm->TabMultiplier -= 1;					// to signify past max?
		}
	//add routine to update tab widgets now...
		psTForm->majorT += TAB_SEVEN;					// set tab # to next "page"
		if (psTForm->majorT >= psTForm->numMajor)
		{
			psTForm->majorT = psTForm->numMajor - 1;		//set it back to max -1
		}
#ifdef  DEBUG_SCROLLTABS		//for debuging
		sprintf(buf, "[debug menu]Clicked RT %d numtabs %d tab # %d", psTForm->TabMultiplier, numTabs, psTForm->majorT);
		addConsoleMessage(buf, DEFAULT_JUSTIFY);
#endif
	}
//	else		//Do we add this or does it not matter?
//		ASSERT(FALSE,"unexpected id [%d] found!",id);
}


#ifdef EDIT_OPTIONS
/* Process return codes from the edit screen */
static void intProcessEdit(UDWORD id)
{
	switch (id)
	{
	case IDED_CLOSE:
		intRemoveEdit();
		intMode = INT_NORMAL;
		break;
	case IDED_FORM:
	case IDED_LABEL:
		break;
	default:
		ASSERT( FALSE, "intProcessEdit: Unknown return code" );
		break;
	}
}
#endif


/* Run the widgets for the in game interface */
INT_RETVAL intRunWidgets(void)
{
	UDWORD			retID;

	INT_RETVAL		retCode;
	BOOL			quitting = FALSE;
	UDWORD			structX,structY, structX2,structY2;
	UWORD			objMajor, objMinor;
	STRUCTURE		*psStructure;
	DROID			*psDroid;
	SDWORD			i;
	UDWORD			widgOverID;

	intDoScreenRefresh();

	/* Update the object list if necessary */
	if (intMode == INT_OBJECT || intMode == INT_STAT || intMode == INT_CMDORDER)
	{
/*		switch (objMode)
		{
		case IOBJ_BUILD:
			psObjList = (BASE_OBJECT *)apsDroidLists[selectedPlayer];
			break;
		case IOBJ_MANUFACTURE:
			psObjList = (BASE_OBJECT *)apsStructLists[selectedPlayer];
			break;
		case IOBJ_RESEARCH:
			psObjList = (BASE_OBJECT *)apsStructLists[selectedPlayer];
			break;
		}*/
		// see if there is a dead object in the list
		for(i=0; i<numObjects; i++)
		{
			if (apsObjectList[i] && apsObjectList[i]->died)
			{
				intObjectDied((UDWORD)(i + IDOBJ_OBJSTART));
				apsObjectList[i] = NULL;
			}
		}
	}

	/* Update the previous object array */
	for (i=0; i<IOBJ_MAX; i++)
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
			ASSERT( widgGetFromID(psWScreen,IDOBJ_TABFORM) != NULL,"No object form\n" );

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
	objectsChanged = FALSE;


	if(bLoadSaveUp)
	{
		if(runLoadSave(TRUE))// check for file name.
		{
			if(strlen(sRequestResult))
			{
				debug( LOG_ERROR, "Returned %s", sRequestResult );
				if(bRequestLoad)
				{
//					loadGame(sRequestResult,TRUE,FALSE,TRUE);
					loopMissionState = LMS_LOADGAME;
					strlcpy(saveGameName, sRequestResult, sizeof(saveGameName));
				}
				else
				{
					if (saveGame(sRequestResult, GTYPE_SAVE_START))
					{
						addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY);

						if(widgGetFromID(psWScreen,IDMISSIONRES_SAVE))
						{
							widgDelete(psWScreen,IDMISSIONRES_SAVE);
						}
					}
					else
					{
						ASSERT( FALSE,"intRunWidgets: saveGame Failed" );
						deleteSaveGame(sRequestResult);
					}
				}
			}
		}
	}

	if (MissionResUp) {
		intRunMissionResult();
	}

	/* Run the current set of widgets */
	if(!bLoadSaveUp)
	{
		retID = widgRunScreen(psWScreen);
	}
	else
	{
		retID =0;
	}

	/* We may need to trigger widgets with a key press */
	if(keyButtonMapping)
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

	if(StatsUp) {
		intRunStats();
	}

	if(OrderUp) {
		intRunOrder();
	}

	if(MultiMenuUp)
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
		intResetScreen(FALSE);
//		widgSetButtonState(psWScreen, IDRET_OPTIONS, WBUT_CLICKLOCK);	// commented out by ajl, now command droids menu
		(void)intAddOptions();
		intMode = INT_OPTION;
		break;

	case IDRET_COMMAND:
		intResetScreen(FALSE);
		widgSetButtonState(psWScreen, IDRET_COMMAND, WBUT_CLICKLOCK);
		intAddCommand(NULL);
		break;

	case IDRET_BUILD:
		//intResetScreen(FALSE);
        intResetScreen(TRUE);
		widgSetButtonState(psWScreen, IDRET_BUILD, WBUT_CLICKLOCK);
		(void)intAddBuild(NULL);
//		intMode = INT_OBJECT;
		break;

	case IDRET_MANUFACTURE:
//		OrderDroidsToEmbark();
//		missionDestroyObjects();
		//intResetScreen(FALSE);
        intResetScreen(TRUE);
		widgSetButtonState(psWScreen, IDRET_MANUFACTURE, WBUT_CLICKLOCK);
		(void)intAddManufacture(NULL);
//		intMode = INT_OBJECT;
		break;

	case IDRET_RESEARCH:
		//intResetScreen(FALSE);
        intResetScreen(TRUE);
		widgSetButtonState(psWScreen, IDRET_RESEARCH, WBUT_CLICKLOCK);
		(void)intAddResearch(NULL);
//		intMode = INT_OBJECT;
		break;

	case IDRET_INTEL_MAP:
//		intResetScreen(FALSE);
//		//check if RMB was clicked
		if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
		{
			//set the current message to be the last non-proximity message added
			setCurrentMsg();
			setMessageImmediate(TRUE);
		}
		else
		{
			psCurrentMsg = NULL;
		}
		addIntelScreen();
		break;

	case IDRET_DESIGN:
		intResetScreen(TRUE);
		widgSetButtonState(psWScreen, IDRET_DESIGN, WBUT_CLICKLOCK);
		/*add the power bar - for looks! */
		intShowPowerBar();
		(void)intAddDesign( FALSE );
		intMode = INT_DESIGN;
		break;

	case IDRET_CANCEL:
		intResetScreen(FALSE);
		psCurrentMsg = NULL;
		break;

	/*Transporter button pressed - OFFWORLD Mission Maps ONLY *********/
	case IDTRANTIMER_BUTTON:
		addTransporterInterface(NULL, TRUE);
		break;

	case IDTRANS_LAUNCH:
		processLaunchTransporter();
		break;

	/* Catch the quit button here */
	case IDMISSIONRES_QUIT:			// mission quit
	case INTINGAMEOP_QUIT_CONFIRM:			// esc quit confrim
	case IDOPT_QUIT:						// options screen quit
		intResetScreen(FALSE);
        //clearMissionWidgets();
		quitting = TRUE;
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
#ifdef EDIT_OPTIONS
		case INT_EDIT:
			intProcessEdit(retID);
			break;
#endif
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
		/*case INT_TUTORIAL:
			intProcessMessageView(retID);
			break;*/
		case INT_TRANSPORTER:
			intProcessTransporter(retID);
			break;
		case INT_NORMAL:
			break;
		default:
			ASSERT( FALSE, "intRunWidgets: unknown interface mode" );
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
				if((structX == structX2) || (structY == structY2))
				{
					// Send the droid off to build the structure assuming the droid
					// can get to the location chosen
					structX = world_coord(structX) + TILE_UNITS / 2;
					structY = world_coord(structY) + TILE_UNITS / 2;
					structX2 = world_coord(structX2) + TILE_UNITS / 2;
					structY2 = world_coord(structY2) + TILE_UNITS / 2;

					if(IsPlayerStructureLimitReached(selectedPlayer)) {
						/* Nothing */
					} else {
						// Set the droid order
						if ( (intNumSelectedDroids(DROID_CONSTRUCT) == 0) &&
                            (intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0) &&
							 (psObjSelected != NULL) )
						{
							orderDroidStatsTwoLoc((DROID *)psObjSelected, DORDER_LINEBUILD,
											   psPositionStats, structX, structY, structX2,structY2);
						}
						else
						{
							orderSelectedStatsTwoLoc(selectedPlayer, DORDER_LINEBUILD,
											   psPositionStats, structX, structY, structX2,
                                               structY2, ctrlShiftDown());
						}
					}
				}

				// put the build menu up again after the structure position has been chosen
				//or ctrl/shift is down and we're queing the build orders
#ifdef DISABLE_BUILD_QUEUE
                if (bReopenBuildMenu)
#else
                if (bReopenBuildMenu || ctrlShiftDown())
#endif
				{
				    intAddBuild(NULL);
				}
				else
				{
					// Clear the object screen
					intResetScreen(FALSE);
				}

			}
			else if (intGetStructPosition(&structX, &structY))//found building
			{
				//check droid hasn't died
				if ((psObjSelected == NULL) ||
					!psObjSelected->died)
				{
					BOOL CanBuild = TRUE;

					// Send the droid off to build the structure assuming the droid
					// can get to the location chosen
//					structX = world_coord(structX);
//					structY = world_coord(structY);
					intCalcStructCenter((STRUCTURE_STATS *)psPositionStats, structX,structY,
												&structX,&structY);

					// Don't allow derrick to be built on burning ground.
					if( ((STRUCTURE_STATS*)psPositionStats)->type == REF_RESOURCE_EXTRACTOR) {
						if(fireOnLocation(structX,structY)) {
							AddDerrickBurningMessage();
							CanBuild = FALSE;
						}
					}

					if(CanBuild) {
						if(IsPlayerStructureLimitReached(selectedPlayer)) {

						} else {
							// Set the droid order
							if ( (intNumSelectedDroids(DROID_CONSTRUCT) == 0) &&
                                (intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0) &&
								 (psObjSelected != NULL) )
							{
								orderDroidStatsLoc((DROID *)psObjSelected, DORDER_BUILD,
									   psPositionStats, structX, structY);
							}
							else
							{
								orderSelectedStatsLoc(selectedPlayer, DORDER_BUILD,
									   psPositionStats, structX, structY, ctrlShiftDown());
							}
						}
					}
				}
//				if(!driveModeActive()) {
//					((DROID *)psObjSelected)->selected = FALSE;//deselect the droid if build command successful
//					DeSelectDroid((DROID*)psObjSelected);
//				}

				// put the build menu up again after the structure position has been chosen
                //or ctrl/shift is down and we're queuing the build orders
#ifdef DISABLE_BUILD_QUEUE
                if (bReopenBuildMenu)

				{
					intAddBuild(NULL);
				}
				else
				{
					// Clear the object screen
					intResetScreen(FALSE);
				}
#else
				// Clear the object screen
				intResetScreen(FALSE);
#endif
			}
		}
		else if (intMode == INT_EDITSTAT && editPosMode == IED_POS)
		{
			/* Directly positioning some type of object */
			if (intGetStructPosition(&structX, &structY))
			{
				/* See what type of thing is being put down */
				if (psPositionStats->ref >= REF_STRUCTURE_START &&
					psPositionStats->ref < REF_STRUCTURE_START + REF_RANGE)
				{
					STRUCTURE_STATS *psBuilding = (STRUCTURE_STATS *)psPositionStats;

					intCalcStructCenter(psBuilding, structX, structY, &structX, &structY);
					if (psBuilding->type == REF_DEMOLISH)
					{
						MAPTILE *psTile = mapTile(map_coord(structX), map_coord(structY));
						FEATURE *psFeature = (FEATURE *)psTile->psObject;
						psStructure = (STRUCTURE *)psTile->psObject; /* reuse var */

						if (psStructure && psTile->psObject->type == OBJ_STRUCTURE)
						{
							removeStruct(psStructure, TRUE);
						}
						else if (psFeature && psTile->psObject->type == OBJ_FEATURE)
						{
							removeFeature(psFeature);
						}
						psStructure = NULL;
					}
					else
					{
						const char* msg;
						psStructure = buildStructure(psBuilding, structX, structY,
						                             selectedPlayer, FALSE);
						/* NOTE: if this was a regular buildprocess we would
						 * have to call sendBuildStarted(psStructure, <droid>);
						 * In this case there is no droid working on the
						 * building though. So we cannot fill out the <droid>
						 * part.
						 */

						// Send a text message to all players, notifying them of
						// the fact that we're cheating ourselves a new
						// structure.
						sasprintf((char**)&msg, _("Player %u is cheating (debug menu) him/herself a new structure: %s."), selectedPlayer, psStructure->pStructureType->pName);
						sendTextMessage(msg, TRUE);
					}
					if (psStructure)
					{
						psStructure->status = SS_BUILT;
						buildingComplete(psStructure);

						// In multiplayer games be sure to send a message to the
						// other players, telling them a new structure has been
						// placed.
						SendBuildFinished(psStructure);
					}
				}
				else if (psPositionStats->ref >= REF_FEATURE_START &&
					psPositionStats->ref < REF_FEATURE_START + REF_RANGE)
				{
					const char* msg;
					buildFeature((FEATURE_STATS *)psPositionStats,
								 world_coord(structX), world_coord(structY), FALSE);

					// Send a text message to all players, notifying them of
					// the fact that we're cheating ourselves a new feature.
					sasprintf((char**)&msg, _("Player %u is cheating (debug menu) him/herself a new feature: %s."), selectedPlayer, psPositionStats->pName);
					sendTextMessage(msg, TRUE);

					// Notify the other hosts that we've just built ourselves a feature
					sendMultiPlayerFeature(((FEATURE_STATS *)psPositionStats)->subType, world_coord(structX), world_coord(structY));
				}
				else if (psPositionStats->ref >= REF_TEMPLATE_START &&
						 psPositionStats->ref < REF_TEMPLATE_START + REF_RANGE)
				{
					psDroid = buildDroid((DROID_TEMPLATE *)psPositionStats,
								 world_coord(structX) + TILE_UNITS / 2, world_coord(structY) + TILE_UNITS / 2,
								 selectedPlayer, FALSE);
					if (psDroid)
					{
						const char* msg;
						addDroid(psDroid, apsDroidLists);

						// Send a text message to all players, notifying them of
						// the fact that we're cheating ourselves a new droid.
						sasprintf((char**)&msg, _("Player %u is cheating (debug menu) him/herself a new droid: %s."), selectedPlayer, psDroid->aName);
						sendTextMessage(msg, TRUE);
					}
				}
				editPosMode = IED_NOPOS;
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

	if(	(testPlayerHasLost() || (testPlayerHasWon() && !bMultiPlayer)) && // yeah yeah yeah - I know....
        (intMode != INT_MISSIONRES) && !getDebugMappingStatus())

	{
		debug( LOG_ERROR, "PlayerHasLost Or Won\n" );
		intResetScreen(TRUE);
		retCode = INT_QUIT;
		quitting = TRUE;
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
			quantity = calcTemplatePower((DROID_TEMPLATE *)apsTemplateList[
				statID - IDSTAT_START]);
		}
		else if (psStat->ref >= REF_RESEARCH_START &&
				 psStat->ref < REF_RESEARCH_START + REF_RANGE)
		{
			//get the research points
			psResearch = (RESEARCH *)ppResearchList[statID - IDSTAT_START];


//			if (asPlayerResList[selectedPlayer][psResearch - asResearch].researched != CANCELLED_RESEARCH)
			// has research been not been canceled
			if (IsResearchCancelled(&asPlayerResList[selectedPlayer][psResearch - asResearch])==0)
			{
				quantity = ((RESEARCH *)ppResearchList[statID -
					IDSTAT_START])->researchPower;
			}
		}

#ifdef INCLUDE_PRODSLIDER
		// Multiply the power quantity by the size of the production run.
		if(objMode == IOBJ_MANUFACTURE) {
			quantity *= widgGetSliderPos(psWScreen,IDSTAT_SLIDER) + 1;
		}
#endif


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
#ifdef INCLUDE_PRODSLIDER
	UDWORD				statID;
	UDWORD				Power = 0;
	UBYTE				Quantity;
	BASE_OBJECT			*psOwner;
	STRUCTURE			*psStruct;
	FACTORY				*psFactory;

	if(intMode != INT_EDITSTAT && objMode == IOBJ_MANUFACTURE)
	{
		psOwner = (BASE_OBJECT *)widgGetUserData(psWScreen, IDSTAT_SLIDERCOUNT);
		psStruct = (STRUCTURE *)psOwner;
		psFactory = (FACTORY *)psStruct->pFunctionality;
		if (psFactory->psSubject)
		{
			Quantity = psFactory->quantity;
			//adjust the infinity button if necessary
			if (Quantity == NON_STOP_PRODUCTION)
			{
				widgSetButtonState(psWScreen, IDSTAT_INFINITE_BUTTON,
					WBUT_CLICKLOCK);
			}
		}
		else
		{
			//check if the infinite production button has been pressed
			if (widgGetButtonState(psWScreen, IDSTAT_INFINITE_BUTTON) &
				WBUT_CLICKLOCK)
			{
				Quantity = STAT_SLDSTOPS + 1;
			}
			else
			{
				Quantity = (UBYTE)(widgGetSliderPos(psWScreen,IDSTAT_SLIDER) + 1);
				//Quantity = widgGetSliderPos(psWScreen,IDSTAT_SLIDER);
			}
		}
		//check for available power if not non stop production
		if (Quantity < STAT_SLDSTOPS)
		{
			/* Find out which button was hilited */
			statID = widgGetMouseOver(psWScreen);
			if (statID >= IDSTAT_START && statID <= IDSTAT_END)
			{
				//get the template build points
				Power = calcTemplatePower((DROID_TEMPLATE *)apsTemplateList[statID - IDSTAT_START]);
				if (Power * Quantity > getPower(selectedPlayer))
				{
					Quantity = (UBYTE)(getPower(selectedPlayer) / Power);
				}
			}
		}
		psFactory->quantity = Quantity;

		// fire the tutorial trigger if neccessary
		if (bInTutorial && Quantity != ProductionRun && Quantity > 1)
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MANURUN);
		}

		ProductionRun = Quantity;
	}
#endif
#ifdef INCLUDE_FACTORYLISTS

	BASE_OBJECT			*psOwner;
	STRUCTURE			*psStruct;
	FACTORY				*psFactory;

	if(intMode != INT_EDITSTAT && objMode == IOBJ_MANUFACTURE)
	{
		psOwner = (BASE_OBJECT *)widgGetUserData(psWScreen, IDSTAT_LOOP_LABEL);
		ASSERT( psOwner->type == OBJ_STRUCTURE, "intRunStats: Invalid object type" );

		psStruct = (STRUCTURE *)psOwner;
		ASSERT( StructIsFactory(psStruct), "intRunStats: Invalid Structure type" );

		psFactory = (FACTORY *)psStruct->pFunctionality;
		//adjust the loop button if necessary
		if (psFactory->psSubject && psFactory->quantity)
		{
			widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
		}
	}

#endif
}


/* Add the stats screen for a given object */
static void intAddObjectStats(BASE_OBJECT *psObj, UDWORD id)
{
	BASE_STATS		*psStats;
	UWORD			statMajor = 0,statMinor = 0, newStatMajor, newStatMinor;
	UDWORD			i,j, index;
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
		if(widgGetFromID(psWScreen,IDSTAT_FORM) != NULL) {
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
			selectedPlayer, MAXSTRUCTURES-1);

		ppsStatsList = (BASE_STATS **)apsStructStatsList;
	}

	//have to determine the Template list once the factory has been chosen
	if (objMode == IOBJ_MANUFACTURE)
	{
		numStatsListEntries = fillTemplateList(apsTemplateList,
			(STRUCTURE *)psObj, MAXTEMPLATES);
		ppsStatsList = (BASE_STATS **)apsTemplateList;
	}

	/*have to calculate the list each time the Topic button is pressed
	  so that only one topic can be researched at a time*/
	if (objMode == IOBJ_RESEARCH)
	{
		//set to value that won't be reached in fillResearchList
		index = numResearch + 1;
		if (psStats)
		{
			index = (RESEARCH *)psStats - asResearch;
		}
		//recalculate the list
		numStatsListEntries = fillResearchList(pList,selectedPlayer, (UWORD)index, MAXRESEARCH);

		//	-- Alex's reordering of the list
		// NOTE!  Do we even want this anymore, since we can have basically
		// unlimted tabs? Future enhancement assign T1/2/3 button on form
		// so we can pick what level of tech we want to build instead of
		// Alex picking for us?
		count = 0;
		for(i=0; i<RID_MAXRID; i++)
		{
			iconNumber = mapRIDToIcon(i);
			for(j=0; j<numStatsListEntries; j++)
			{
				entryIN = asResearch[pList[j]].iconID;
				if(entryIN == iconNumber)
				{
					pSList[count++] = pList[j];
				}

			}
		}
		// Tag on the ones at the end that have no BASTARD icon IDs - why is this?!!?!?!?
		// NOTE! more pruning [future ref]
		for(j=0; j<numStatsListEntries; j++)
		{
			//this can't be assumed cos we've added some more icons and they have higher #define values than QUESTIONMARK!
            //entryIN = asResearch[pList[j]].iconID;
			//if(entryIN<mapRIDToIcon(RID_ROCKET) || entryIN>mapRIDToIcon(RID_QUESTIONMARK))
            iconNumber = mapIconToRID(asResearch[pList[j]].iconID);
            if (iconNumber < 0)
			{
				pSList[count++] = pList[j];
			}
		}


		//fill up the list with topics
		for (i=0; i < numStatsListEntries; i++)
		{
			ppResearchList[i] = asResearch + pSList[i];	  // note change from pList
		}
	}

//DBPRINTF(("intAddStats(%p,%d,%p,%p)\n",ppsStatsList, numStatsListEntries, psStats, psObj);
	intAddStats(ppsStatsList, numStatsListEntries, psStats, psObj);

    //get the tab positions for the new stat form
    psForm = (W_TABFORM*)widgGetFromID(psWScreen,IDSTAT_TABFORM);
    if (psForm != NULL)
    {
        newStatMajor = psForm->numMajor;
        newStatMinor = psForm->asMajor[statMajor].numMinor;

        // Restore the tab positions.
	    if ( (!psStats) && (widgGetFromID(psWScreen,IDSTAT_FORM) != NULL) )
	    {
            //only restore if we've still got at least that many tabs
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
	if(id != 0)
	{
		widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
	}
}


static void intSelectDroid(BASE_OBJECT *psObj)
{
	if(driveModeActive()) {
		clearSel();
		((DROID*)psObj)->selected = TRUE;
		driveSelectionChanged();
//		clearSelection();
//		((DROID*)psObj)->selected = TRUE;
//		StopDriverMode();
//		StartDriverMode();
		driveDisableControl();
	} else {
		clearSelection();
		((DROID*)psObj)->selected = TRUE;
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
		//intAddObjectStats(psObj, id);
	}
}


/* Process return codes from the object screen */
static void intProcessObject(UDWORD id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStruct;
	BOOL IsDeliveryRepos = FALSE;
	SDWORD			butIndex;
	UDWORD			statButID;

	ASSERT( widgGetFromID(psWScreen,IDOBJ_TABFORM) != NULL,"intProcessObject, missing form\n" );

	// deal with CRTL clicks
	if (objMode == IOBJ_BUILD &&	// What..................?
		(keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)) &&
		((id >= IDOBJ_OBJSTART && id <= IDOBJ_OBJEND) ||
		 (id >= IDOBJ_STATSTART && id <= IDOBJ_STATEND)) )
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
			psObj->selected = FALSE;
			widgSetButtonState(psWScreen, statButID, 0);
			if ((intNumSelectedDroids(DROID_CONSTRUCT) == 0) &&
                (intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0))
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
				psObjSelected->selected = TRUE;
			}
			psObj->selected = TRUE;
			widgSetButtonState(psWScreen, statButID, WBUT_CLICKLOCK);
			intAddObjectStats(psObj, statButID);
		}
	}
	else if (id >= IDOBJ_OBJSTART &&
		id <= IDOBJ_OBJEND)
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
			if (psObj)
			{
				//Only do this if not offworld - only check if a structure
				//if (!offWorldKeepLists)
				{

					if(psObj->type == OBJ_STRUCTURE && !offWorldKeepLists)
					{
						/* Deselect old buildings */
						for(psStruct = apsStructLists[selectedPlayer];
							psStruct; psStruct=psStruct->psNext)
						{
							psStruct->selected = FALSE;
						}

						/* Select new one */
						((STRUCTURE*)psObj)->selected = TRUE;
					}

					if(!driveModeActive())
                    {
                        //don't do this if offWorld and a structure object has been selected
                        if (!(psObj->type == OBJ_STRUCTURE && offWorldKeepLists))
                        {
    						// set the map position - either the object position, or the position jumped from
	    					butIndex = id - IDOBJ_OBJSTART;
		    				if (butIndex >= 0 && butIndex < IOBJ_MAX)
			    			{
				    			if (((asJumpPos[butIndex].x == 0) && (asJumpPos[butIndex].y == 0)) ||
					    			!DrawnInLastFrame((SDWORD)psObj->sDisplay.frameNumber) ||
						    		((psObj->sDisplay.screenX > pie_GetVideoBufferWidth()) || (psObj->sDisplay.screenY > pie_GetVideoBufferHeight())))
							    {
								    getPlayerPos((SDWORD*)&asJumpPos[butIndex].x, (SDWORD*)&asJumpPos[butIndex].y);


    								setPlayerPos(psObj->pos.x, psObj->pos.y);
	    							if(getWarCamStatus())
		    						{
			    						camToggleStatus();
				    				}
	//							intSetMapPos(psObj->pos.x, psObj->pos.y);

			    				}
				    			else
					    		{

						    		setPlayerPos(asJumpPos[butIndex].x, asJumpPos[butIndex].y);
							    	if(getWarCamStatus())
								    {
									    camToggleStatus();
    								}
	//							intSetMapPos(asJumpPos[butIndex].x, asJumpPos[butIndex].y);

					    			asJumpPos[butIndex].x = 0;
						    		asJumpPos[butIndex].y = 0;
							    }
						    }
					    }
				    }
                }

				psObj = intGetObject(id);
				if(!IsDeliveryRepos) {
					intResetWindows(psObj);
				}

				// If a construction droid button was clicked then
				// clear all other selections and select it.
				if(psObj->type == OBJ_DROID) {			// If it's a droid...
					intSelectDroid(psObj);
					psObjSelected = psObj;

				}
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
			if(psObj->type == OBJ_DROID)
			{
				// Select the droid when the stat button (in the object window) is pressed.
				intSelectDroid(psObj);
				psObjSelected = psObj;
			}
			else if (psObj->type == OBJ_STRUCTURE)
			{

//				clearSelection();
//				psObj->selected = TRUE;

				if (StructIsFactory((STRUCTURE *)psObj))
				{
					//might need to cancel the hold on production
					releaseProduction((STRUCTURE *)psObj);
				}
                else if (((STRUCTURE *)psObj)->pStructureType->type == REF_RESEARCH)
                {
					//might need to cancel the hold on research facilty
					releaseResearch((STRUCTURE *)psObj);
                }
			}
		}
	}
	else if (id == IDOBJ_CLOSE)
	{
		intResetScreen(FALSE);
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

#ifdef INCLUDE_FACTORYLISTS
	DROID_TEMPLATE	*psNext;
#endif

	ASSERT( widgGetFromID(psWScreen,IDOBJ_TABFORM) != NULL,"intProcessStats, missing form\n" );

	if (id >= IDSTAT_START &&
		id <= IDSTAT_END)
	{
		ASSERT( id - IDSTAT_START < numStatsListEntries,
			"intProcessStructure: Invalid structure stats id" );

		/* deal with RMB clicks */
		if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
		{
			intStatsRMBPressed(id);
		}
		/* deal with LMB clicks */
		else
		{
#ifdef INCLUDE_FACTORYLISTS
			//manufacture works differently!
			if(objMode == IOBJ_MANUFACTURE)
			{
				//get the stats
				psStats = ppsStatsList[id - IDSTAT_START];
				ASSERT( psObjSelected != NULL,
					"intProcessStats: Invalid structure pointer" );
				ASSERT( psStats != NULL,
					"intProcessStats: Invalid template pointer" );
                if (productionPlayer == (SBYTE)selectedPlayer)
                {
                    FACTORY  *psFactory = (FACTORY *)((STRUCTURE *)psObjSelected)->
                        pFunctionality;

                    //increase the production
				    factoryProdAdjust((STRUCTURE *)psObjSelected, (DROID_TEMPLATE *)psStats, TRUE);
                    //need to check if this was the template that was mid-production
                    if (psStats == psFactory->psSubject)
                    {
				        //if have wrapped round to zero then cancel the production
				        if (getProductionQuantity((STRUCTURE *)psObjSelected,
					        (DROID_TEMPLATE *)psStats) == 0)
				        {
					        //init the factory production
					        psFactory->psSubject = NULL;
					        //check to see if anything left to produce
					        psNext = factoryProdUpdate((STRUCTURE *)psObjSelected, NULL);
					        if (psNext == NULL)
					        {
						        intManufactureFinished((STRUCTURE *)psObjSelected);
					        }
					        else
					        {
						        if (!objSetStatsFunc(psObjSelected, (BASE_STATS *)psNext))
						        {
							        intSetStats(objStatID, NULL);
						        }
						        else
						        {
							        // Reset the button on the object form
							        intSetStats(objStatID, psStats);
						        }
					        }
				        }
                    }
				    else
				    {
					    //if factory wasn't currently on line then set the object button
					    if (!psFactory->psSubject)
					    {
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
				    }
                }
			}
			else
#endif
			{
				/* See if this was a click on an already selected stat */
				psStats = objGetStatsFunc(psObjSelected);
                //only do the cancel operation if not trying to add to the build list
				if (psStats == ppsStatsList[id - IDSTAT_START] &&
                    !(objMode == IOBJ_BUILD && ctrlShiftDown()))
				{
                    //this needs to be done before the topic is cancelled from the structure
                    //research works differently now! - AB 5/2/99
					/* If Research then need to set topic to be cancelled */
					if (objMode == IOBJ_RESEARCH)
					{
						if (psObjSelected->type == OBJ_STRUCTURE )
						{
							cancelResearch((STRUCTURE *)psObjSelected);
						}
					}

					/* Clear the object stats */
					objSetStatsFunc(psObjSelected, NULL);

					/* Reset the button on the object form */
					intSetStats(objStatID, NULL);

					/* Unlock the button on the stats form */
					widgSetButtonState(psWScreen, id, 0);

                    //research works differently now! - AB 5/2/99
					/* If Research then need to set topic to be cancelled */
					/*if (objMode == IOBJ_RESEARCH)
					{
						if (psObjSelected->type == OBJ_STRUCTURE )
						{
							cancelResearch((STRUCTURE *)psObjSelected);
						}
					}*/
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
								cancelResearch((STRUCTURE *)psObjSelected);
							}
						}
					}

					// call the tutorial callback if necessary
					if (bInTutorial && objMode == IOBJ_BUILD)
					{

						eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BUILDGRID);
					}

					// Set the object stats
					psStats = ppsStatsList[id - IDSTAT_START];

					// Reset the button on the object form
					//if this returns FALSE, there's a problem so set the button to NULL
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
				widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor,&objMinor);

				// Close the stats box
				intRemoveStats();
				intMode = INT_OBJECT;

				// Reset the tabs on the object form
				widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor,objMinor);

				// Close the object box as well if selecting a location to build- no longer hide/reveal
                //or if selecting a structure to demolish
				if (objMode == IOBJ_BUILDSEL || objMode == IOBJ_DEMOLISHSEL)
				{
					if(driveModeActive()) {
						// Make sure weve got a construction droid selected.
						//if(driveGetDriven()->droidType != DROID_CONSTRUCT) {
                        if(driveGetDriven()->droidType != DROID_CONSTRUCT &&
                            driveGetDriven()->droidType != DROID_CYBORG_CONSTRUCT) {
//PD30 							driveSelectionChanged();
							driveDisableControl();
						}
				 		driveDisableTactical();
						driveStartBuild();
						intRemoveObject();
					}

					intRemoveObject();
                    //hack to stop the stats window re-opening in demolish mode
                    if (objMode == IOBJ_DEMOLISHSEL)
                    {
                        IntRefreshPending = FALSE;
                    }

				}


			}
		}
	}
	else if (id == IDSTAT_CLOSE)
	{
		/* Get the tabs on the object form */
		widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor,&objMinor);

		/* Close the structure box without doing anything */
		intRemoveStats();
		intMode = INT_OBJECT;

		/* Reset the tabs on the build form */
		widgSetTabs(psWScreen, IDOBJ_TABFORM, objMajor,objMinor);

		/* Unlock the stats button */
		widgSetButtonState(psWScreen, objStatID, 0);
	}
	else if(id == IDSTAT_SLIDER) {
		// Process the quantity slider.
	}
	else if( id >= IDPROX_START && id <= IDPROX_END)
	{
		// process the proximity blip buttons.
	}
#ifdef INCLUDE_PRODSLIDER
	else if(id == IDSTAT_INFINITE_BUTTON)
	{
		// Process the infinte button.
		//if the button is locked - unlock and vice versa
		if (widgGetButtonState(psWScreen, IDSTAT_INFINITE_BUTTON) &
			WBUT_CLICKLOCK)
		{
			//unlock
			widgSetButtonState(psWScreen, IDSTAT_INFINITE_BUTTON, 0);
		}
		else
		{
			//lock
			widgSetButtonState(psWScreen, IDSTAT_INFINITE_BUTTON,
				WBUT_CLICKLOCK);
		}
	}
#endif
	else if(id == IDSTAT_LOOP_BUTTON)
	{
		// Process the loop button.
		psStruct = (STRUCTURE*)widgGetUserData(psWScreen, IDSTAT_LOOP_LABEL);
		if (psStruct)
		{
			//LMB pressed
			if (widgGetButtonKey(psWScreen) & WKEY_PRIMARY)
			{
				factoryLoopAdjust(psStruct, TRUE);
			}
			//RMB pressed
			else if (widgGetButtonKey(psWScreen) & WKEY_SECONDARY)
			{
				factoryLoopAdjust(psStruct, FALSE);
			}
			if (((FACTORY *)psStruct->pFunctionality)->psSubject &&
				((FACTORY *)psStruct->pFunctionality)->quantity)
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

	else if(id == IDSTAT_DP_BUTTON)
	{
		// Process the DP button
		psStruct = (STRUCTURE*)widgGetUserData(psWScreen, IDSTAT_DP_BUTTON);
		if (psStruct)
		{
			// make sure that the factory isn't assigned to a commander
			assignFactoryCommandDroid(psStruct, NULL);
			psFlag = FindFactoryDelivery(psStruct);
			if (psFlag)
			{
				StartDeliveryPosition( (OBJECT_POSITION *)psFlag );
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
		}
		//add routine to update tab widgets now...
		temp = psTForm->majorT;					// set tab # to previous "page"
		temp -= TAB_SEVEN;						// 7 = 1 "page" of tabs
		if ( temp < 0)
			psTForm->majorT = 0;
		else
			psTForm->majorT = temp;
#ifdef  DEBUG_SCROLLTABS
		sprintf(buf, "[build menu]Clicked LT %d tab #=%d", psTForm->TabMultiplier, psTForm->majorT);
		addConsoleMessage(buf, DEFAULT_JUSTIFY);
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
		}
	//add routine to update tab widgets now...
		psTForm->majorT += TAB_SEVEN;				// set tab # to next "page"
		if (psTForm->majorT >= psTForm->numMajor)	// check if too many
		{
			psTForm->majorT = psTForm->numMajor - 1;	// set it back to max -1
		}
#ifdef  DEBUG_SCROLLTABS		//for debuging
		sprintf(buf, "[build menu]Clicked RT %d numtabs %d tab # %d", psTForm->TabMultiplier, numTabs, psTForm->majorT);
		addConsoleMessage(buf, DEFAULT_JUSTIFY);
#endif

	}
	else
	{
		ASSERT( id == IDSTAT_FORM || id == IDSTAT_TITLEFORM ||
				id == IDSTAT_LABEL || id == IDSTAT_TABFORM  ||
				id == IDSTAT_TABSCRL_RIGHT || id ==IDSTAT_TABSCRL_LEFT,
			"intProcessStructure: Unknown widget ID" );
	}
}


/* Set the map view point to the world coordinates x,y */
void intSetMapPos(UDWORD x, UDWORD y)
{
	if (!driveModeActive())
	{
		setViewPos(map_coord(x), map_coord(y), TRUE);
	}
}


/* Sync the interface to an object */
// If psObj is NULL then reset interface displays.
//
// There should be two version of this function, one for left clicking and one got right.
//
void intObjectSelected(BASE_OBJECT *psObj)
{
	/* Remove whatever is up */
//	intResetScreen(FALSE);

	if(psObj) {
//		intResetScreen(TRUE);
		setWidgetsStatus(TRUE);
		switch(psObj->type)
		{
		case OBJ_DROID:
/*			stop build interface appearing for constuction droids
			if (droidType((DROID *)psObj) == DROID_CONSTRUCT)
			{
				intResetScreen(FALSE);
				intAddBuild((DROID *)psObj);
			}
			else*/

//			if(!OrderUp)
//			{
//				intResetScreen(FALSE);
//			}
//			intAddOrder((DROID *)psObj);
//			intMode = INT_ORDER;


			if(!OrderUp)
			{
				intResetScreen(FALSE);
                //changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
                //intAddOrder((DROID *)psObj);
                intAddOrder(psObj);
				intMode = INT_ORDER;
			}
			else
			{
                //changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
				//intAddOrder((DROID *)psObj);
                intAddOrder(psObj);
			}


			break;

		case OBJ_STRUCTURE:
			//don't do anything if structure is only partially built
			intResetScreen(FALSE);

			if ( objMode == IOBJ_DEMOLISHSEL )
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

					//widgHide(psWScreen, IDOBJ_FORM);
				}
				else if (((STRUCTURE *)psObj)->pStructureType->type == REF_RESEARCH)
				{

					intAddResearch((STRUCTURE *)psObj);

					//widgHide(psWScreen, IDOBJ_FORM);
				}
//		  		for(psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct=psStruct->psNext)
//				{
//					psStruct->selected = FALSE;
//				}
//				((STRUCTURE*)psObj)->selected = TRUE;		// wrong place?
			}
			break;
		default:
			break;
		}
	} else {
		intResetScreen(FALSE);
//		if(OrderUp) {
//			intRemoveOrder();
//		}
	}
}


// add the construction interface if a constructor droid is selected
void intConstructorSelected(DROID *psDroid)
{
//	intResetScreen(FALSE);
	setWidgetsStatus(TRUE);
	intAddBuild(psDroid);
	widgHide(psWScreen, IDOBJ_FORM);
}

// add the construction interface if a constructor droid is selected
void intCommanderSelected(DROID *psDroid)
{
	setWidgetsStatus(TRUE);
	intAddCommand(psDroid);
	widgHide(psWScreen, IDOBJ_FORM);
}

extern void FinishStructurePosition(UDWORD xPos,UDWORD yPos,void *UserData);

/* Start looking for a structure location */
//static void intStartStructPosition(UDWORD width, UDWORD height)
static void intStartStructPosition(BASE_STATS *psStats,DROID *psDroid)
{
	init3DBuilding(psStats,NULL,NULL);

	/*if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL) {
		widgGetTabs(psWScreen, IDOBJ_TABFORM, &objMajor, &objMinor);
		// Hide the object form while we select a position.
		//widgHide(psWScreen,IDOBJ_TABFORM);	only need to hide the top form -all else follows
		widgHide(psWScreen,IDOBJ_FORM);
	}*/
}


/* Stop looking for a structure location */
static void intStopStructPosition(void)
{
	/* Check there is still a struct position running */
//	if (intMode == INT_OBJECT && objMode == IOBJ_BUILDSEL) {
	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL) {
		// Reset the stats button
//		widgGetTabs(psWScreen, IDOBJ_FORM, &objMajor, &objMinor);
//		widgEndScreen(psWScreen);

		/*if(DroidIsBuilding((DROID *)psObjSelected)) {
			STRUCTURE *Structure = DroidGetBuildStructure((DROID *)psObjSelected);
			ASSERT( Structure!=NULL,"Bad structure pointer" );
			intSetStats(objStatID,(BASE_STATS*)Structure->pStructureType);
		} else if(DroidGoingToBuild((DROID *)psObjSelected)) {
			intSetStats(objStatID,DroidGetBuildStats((DROID *)psObjSelected));
		} else {
			intSetStats(objStatID,NULL);
		}*/

//		widgStartScreen(psWScreen);
		objMode = IOBJ_BUILD;
	}

	kill3DBuilding();
}


/* See if a structure location has been found */
static BOOL intGetStructPosition(UDWORD *pX, UDWORD *pY)
{
	BOOL	retVal = FALSE;

	retVal = found3DBuilding(pX,pY);
	if (retVal)
	{
#if 0
//		if (intMode == INT_OBJECT && objMode == IOBJ_BUILDSEL) {
		if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL)
		{
			widgReveal(psWScreen,IDOBJ_TABFORM);	// Reveal the object form.
			widgReveal(psWScreen,IDOBJ_FORM);
		}
#endif
	}

	return retVal;
}


/* Display the widgets for the in game interface */
void intDisplayWidgets(void)
{
	BOOL bPlayerHasHQ;

	// God only knows...
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

			/*Add the radar to the design screen - only if player has HQ*/
			bPlayerHasHQ = getHQExists(selectedPlayer);

			if (bPlayerHasHQ)	//NOTE: This flickers badly, so turn it off for now.
			{
				// drawRadar();
			}

			// We need to add the console messages to the intelmap for the tutorial so that it can display messages
			if ((intMode == INT_DESIGN)||(bInTutorial && intMode==INT_INTELMAP))
			{
				displayConsoleMessages();
			}
		}
	}

	widgDisplayScreen(psWScreen);

	if(bLoadSaveUp)
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
			objectsChanged = TRUE;
		}
		else if ((objMode == IOBJ_RESEARCH || objMode == IOBJ_MANUFACTURE) &&
				 psObj->type == OBJ_STRUCTURE && objSelectFunc(psObj))
		{
			objectsChanged = TRUE;
		}
	}
}


// clean up when an object dies
static void intObjectDied(UDWORD objID)
{
	RENDERED_BUTTON		*psBut;
	UDWORD				statsID, gubbinsID;

	// clear the object button
	psBut = (RENDERED_BUTTON *)widgGetUserData(psWScreen, objID);
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
		psBut = (RENDERED_BUTTON *)widgGetUserData(psWScreen, statsID);
		// and disable it
		widgSetButtonState(psWScreen, statsID, WBUT_DISABLE);

		// remove the stat screen if necessary
		if ( (intMode == INT_STAT) && statsID == objStatID )
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

	ASSERT( psDroid != NULL,
		"intBuildFinished: Invalid droid pointer" );

	if ((intMode == INT_OBJECT || intMode == INT_STAT) &&
		//(objMode == IOBJ_BUILDSEL || objMode == IOBJ_BUILD))
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

	ASSERT( psDroid != NULL,
		"intBuildStarted: Invalid droid pointer" );

	if ((intMode == INT_OBJECT || intMode == INT_STAT) &&
		//(objMode == IOBJ_BUILDSEL || objMode == IOBJ_BUILD))
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
					intSetStats(droidID + IDOBJ_STATSTART, ((BASE_STATS *)(
						(STRUCTURE *)psCurr->psTarget)->pStructureType));
					break;
				}
				droidID++;
			}
		}
	}
}

/* Are we in build select mode*/
BOOL intBuildSelectMode(void)
{
	return (objMode == IOBJ_BUILDSEL);
}

/* Are we in demolish select mode*/
BOOL intDemolishSelectMode(void)
{
	return (objMode == IOBJ_DEMOLISHSEL);
}

//is the build interface up?
BOOL intBuildMode(void)
{
	return (objMode == IOBJ_BUILD);
}

//Written to allow demolish order to be added to the queuing system
void intDemolishCancel(void)
{
    if (objMode==IOBJ_DEMOLISHSEL)
    {
        objMode = IOBJ_NONE;
    }
}


//reorder the research facilities so that first built is first in the list
static void orderResearch(void)
{
	BASE_OBJECT *psTemp;
	UDWORD i, maxLoop  = (UDWORD)(numObjects / 2);

	for (i = 0; i < maxLoop; i++)
	{
		psTemp = apsObjectList[i];
		apsObjectList[i] = apsObjectList[(numObjects - 1) - i];
		apsObjectList[(numObjects - 1) - i] = psTemp;
	}
}


// reorder the commanders
static void orderDroids(void)
{
	SDWORD i, j;
	BASE_OBJECT *psTemp;

	debug( LOG_NEVER, "orderUnit\n" );

	// bubble sort on the ID - first built will always be first in the list
	for (i = 0; i < MAX_OBJECTS; i++)
	{
		for(j = i + 1; j < MAX_OBJECTS; j++)
		{
			if (apsObjectList[i] != NULL && apsObjectList[j] != NULL &&
				apsObjectList[i]->id > apsObjectList[j]->id)
			{
				psTemp = apsObjectList[i];
				apsObjectList[i] = apsObjectList[j];
				apsObjectList[j] = psTemp;
			}
		}
	}
}


/*puts the selected players factories in order - Standard factories 1-5, then
cyborg factories 1-5 and then Vtol factories 1-5*/
static void orderFactories(void)
{
	STRUCTURE *psStruct, *psNext;
	SDWORD entry = 0;
	UDWORD inc = 0, type = FACTORY_FLAG, objectInc = 0;

	ASSERT( numObjects <= NUM_FACTORY_TYPES * MAX_FACTORY, "orderFactories : too many factories!" );

	//copy the object list into the list to order
	memcpy(apsListToOrder, apsObjectList, sizeof(BASE_OBJECT*) * ORDERED_LIST_SIZE);

	//go through the list of structures and extract them in order
	while (entry < numObjects)
	{
		for (psStruct = (STRUCTURE*)apsListToOrder[objectInc]; psStruct != NULL; psStruct = psNext)
		{
			psNext = (STRUCTURE*)apsListToOrder[++objectInc];
			if ((SDWORD)objectInc >= numObjects)
			{
				psNext = NULL;
			}

			ASSERT( StructIsFactory(psStruct), "orderFactories: structure is not a factory" );

			if (((FACTORY*)psStruct->pFunctionality)->psAssemblyPoint->factoryInc == inc
			    && ((FACTORY*)psStruct->pFunctionality)->psAssemblyPoint->factoryType == type)
			{
				apsObjectList[entry++] = (BASE_OBJECT*)psStruct;
				//quick check that don't end up with more!
				if (entry > numObjects)
				{
					ASSERT( FALSE, "orderFactories: too many objects!" );
					return;
				}
				break;
			}
		}
		inc++;
		if (inc > MAX_FACTORY)
		{
			inc = 0;
			type++;
		}
		objectInc = 0;
	}
}


/*order the objects in the bottom bar according to their type*/
static void orderObjectInterface(void)
{

	if (apsObjectList == NULL)
	{
		//no objects so nothing to order!
		return;
	}

	switch(apsObjectList[0]->type)
	{
	case OBJ_STRUCTURE:
		//if (((STRUCTURE *)apsObjectList[0])->pStructureType->type == REF_FACTORY ||
		//	((STRUCTURE *)apsObjectList[0])->pStructureType->type == REF_CYBORG_FACTORY ||
		//	((STRUCTURE *)apsObjectList[0])->pStructureType->type == REF_VTOL_FACTORY)
        if (StructIsFactory((STRUCTURE *)apsObjectList[0]))
		{
			orderFactories();
		}
        else if (((STRUCTURE *)apsObjectList[0])->pStructureType->type ==
            REF_RESEARCH)
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


/* Tell the interface a factory has completed building ALL droids */
void intManufactureFinished(STRUCTURE *psBuilding)
{
	SDWORD		    structureID;
	STRUCTURE       *psCurr;
    BASE_OBJECT     *psObj;

	ASSERT( psBuilding != NULL,
		"intManufactureFinished: Invalid structure pointer" );

	if ((intMode == INT_OBJECT || intMode == INT_STAT) &&
		(objMode == IOBJ_MANUFACTURE))
	{
		/* Find which button the structure is on and clear it's stats */
		structureID = 0;
    	numObjects = 0;
	    memset(apsObjectList, 0, sizeof(BASE_OBJECT *) * MAX_OBJECTS);
		//for (psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		for (psCurr = interfaceStructList(); psCurr; psCurr = psCurr->psNext)
		{
			if (objSelectFunc((BASE_OBJECT *)psCurr))
			{
                //the list is ordered now so we have to get all possible entries and sort it before checking if this is the one!
                apsObjectList[numObjects] = (BASE_OBJECT *)psCurr;
			    numObjects++;
            }
			// make sure the list doesn't overflow
			if (numObjects >= MAX_OBJECTS)
			{
				break;
			}
        }
        //order the list
        orderFactories();

        //now look thru the list to see which one corresponds to the factory that has just finished
        structureID = 0;
        for (psObj = apsObjectList[structureID]; structureID < numObjects; structureID++)
        {
			if ((STRUCTURE *)psObj == psBuilding)
			{
				intSetStats(structureID + IDOBJ_STATSTART, NULL);
        		//clear the loop button if interface is up
				if (widgGetFromID(psWScreen,IDSTAT_LOOP_BUTTON))
				{
					widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, 0);
				}

                break;
			}
		}
	}
}

/* Tell the interface a research facility has completed a topic */
void intResearchFinished(STRUCTURE *psBuilding)
{
	//SDWORD		    structureID;
	//STRUCTURE       *psCurr;
    //BASE_OBJECT     *psObj;

	ASSERT( psBuilding != NULL,
		"intResearchFinished: Invalid structure pointer" );

	// just do a screen refresh
	intRefreshScreen();
	return;

/*	if ((intMode == INT_OBJECT || intMode == INT_STAT) &&
		(objMode == IOBJ_RESEARCH))
	{
		// Find which button the structure is on and clear it's stats
		structureID = 0;
    	numObjects = 0;
	    memset(apsObjectList, 0, sizeof(BASE_OBJECT *) * MAX_OBJECTS);
		//for (psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		for (psCurr = interfaceStructList(); psCurr; psCurr = psCurr->psNext)
		{
			if (objSelectFunc((BASE_OBJECT *)psCurr))
			{
                //the list is ordered now so we have to get all possible entries and sort it before checking if this is the one!
                apsObjectList[numObjects] = (BASE_OBJECT *)psCurr;
			    numObjects++;
            }
			// make sure the list doesn't overflow
			if (numObjects >= MAX_OBJECTS)
			{
				break;
			}
        }
        //order the list
        orderResearch();

        //now look thru the list to see which one corresponds to the factory that has just finished
        structureID = 0;
        for (psObj = apsObjectList[structureID]; structureID < numObjects; structureID++)
        {
            if ((STRUCTURE *)psObj == psBuilding)
			{
				intSetStats(structureID + IDOBJ_STATSTART, NULL);
				break;
			}
		}
	}*/

	// refresh the research interface to update with new topics.
	//intRefreshScreen();
}

/* Do the annoying calculation for how many forms are needed
 * given the total number of buttons and the number of
 * buttons per page.
 * A simple div just doesn't quite do it....
 */
UWORD numForms(UDWORD total, UDWORD perForm)
{
	/* If the buttons fit exactly, don't have to add one */
	if (total != 0 && (total % perForm) == 0)
	{
		return (UWORD)(total/perForm);
	}

	/* Otherwise add one to the div */
	return (UWORD)(total/perForm + 1);
}


/* Add the reticule widgets to the widget screen */
BOOL intAddReticule(void)
{
	if(ReticuleUp == FALSE) {
		W_FORMINIT		sFormInit;
		W_BUTINIT		sButInit;

		/* Create the basic form */

		memset(&sFormInit, 0, sizeof(W_FORMINIT));
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
			return FALSE;
		}

		/* Now add the buttons */

		//set up default button data
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = IDRET_FORM;
		sButInit.id = IDRET_COMMAND;
		sButInit.width = RET_BUTWIDTH;
		sButInit.height = RET_BUTHEIGHT;
		sButInit.FontID = font_regular;

		//add buttons as required...

		//options button
		sButInit.style = WBUT_PLAIN;
		SetReticuleButPos(RETBUT_COMMAND,&sButInit);
//		sButInit.x = 19+RETXOFFSET;
//		sButInit.y = 35+RETYOFFSET;
	//	sButInit.pText = "O";
		sButInit.pTip = _("Commanders (F6)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_COMMANDDROID_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		/* Intelligence Map button - this needs to respond to RMB as well*/
		sButInit.style = WBUT_PLAIN | WFORM_SECONDARY;
		sButInit.id = IDRET_INTEL_MAP;
		SetReticuleButPos(RETBUT_INTELMAP,&sButInit);
//		sButInit.x = 19+RETXOFFSET;
//		sButInit.y = 70+RETYOFFSET;
	//	sButInit.pText = "S";
		sButInit.pTip = _("Intelligence Display (F5)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_INTELMAP_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		/* Manufacture button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_MANUFACTURE;
		SetReticuleButPos(RETBUT_FACTORY,&sButInit);
//		sButInit.x = 53+RETXOFFSET;
//		sButInit.y = 17+RETYOFFSET;
	//	sButInit.pText = "M";
		sButInit.pTip = _("Manufacture (F1)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_MANUFACTURE_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		/* Design button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_DESIGN;
		SetReticuleButPos(RETBUT_DESIGN,&sButInit);
//		sButInit.x = 53+RETXOFFSET;
//		sButInit.y = 88+RETYOFFSET;
	//	sButInit.pText = "D";
		sButInit.pTip = _("Design (F4)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_DESIGN_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		/* Research button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_RESEARCH;
		SetReticuleButPos(RETBUT_RESEARCH,&sButInit);
//		sButInit.x = 87+RETXOFFSET;
//		sButInit.y = 35+RETYOFFSET;
	//	sButInit.pText = "R";
		sButInit.pTip = _("Research (F2)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_RESEARCH_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		/* Build button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_BUILD;
		SetReticuleButPos(RETBUT_BUILD,&sButInit);
//		sButInit.x = 87+RETXOFFSET;
//		sButInit.y = 70+RETYOFFSET;
	//	sButInit.pText = "B";
		sButInit.pTip = _("Build (F3)");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_BUILD_UP;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		/* Cancel button */
		sButInit.style = WBUT_PLAIN;
		sButInit.id = IDRET_CANCEL;
		SetReticuleButPos(RETBUT_CANCEL,&sButInit);
//		sButInit.x = 48+RETXOFFSET;
//		sButInit.y = 49+RETYOFFSET;
		sButInit.width = RET_BUTWIDTH + 10;
		sButInit.height = RET_BUTHEIGHT + 8;
	//	sButInit.pText = "C";
		sButInit.pTip = _("Close");
		sButInit.pDisplay = intDisplayReticuleButton;
		sButInit.UserData = IMAGE_CANCEL_UP;
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}
	//	intCheckReticuleButtons();
		ReticuleUp = TRUE;
	}

	return TRUE;
}

void intRemoveReticule(void)
{
	if(ReticuleUp == TRUE) {
		widgDelete(psWScreen,IDRET_FORM);		// remove reticule
		ReticuleUp = FALSE;
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
BOOL intAddPower(void)
{
	W_BARINIT	sBarInit;

	memset(&sBarInit, 0, sizeof(W_BARINIT));

	/* Add the trough bar */
	sBarInit.formID = 0;	//IDPOW_FORM;
	sBarInit.id = IDPOW_POWERBAR_T;
	//start the power bar off in view (default)
	sBarInit.style = WBAR_TROUGH;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = (SWORD)POW_X;
	sBarInit.y = (SWORD)POW_Y;
	sBarInit.width = POW_BARWIDTH;
	sBarInit.height = iV_GetImageHeight(IntImages,IMAGE_PBAR_EMPTY);
	sBarInit.sCol.byte.r = POW_CLICKBARMAJORRED;
	sBarInit.sCol.byte.g = POW_CLICKBARMAJORGREEN;
	sBarInit.sCol.byte.b = POW_CLICKBARMAJORBLUE;
	sBarInit.pDisplay = intDisplayPowerBar;
	sBarInit.iRange = POWERBAR_SCALE;

	sBarInit.pTip = _("Power");

	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return FALSE;
	}

	powerBarUp = TRUE;
	return TRUE;
}


/* Remove the power bar widgets */
/*void intRemovePower(void)
{
	if (powerBarUp)
	{
		widgDelete(psWScreen, IDPOW_POWERBAR_T);
		powerBarUp = FALSE;
	}
}*/

/* Set the shadow power for the selected player */
// Now just sets the global variable ManuPower which is used in the power bar display callback. PD
void intSetShadowPower(UDWORD quantity)
{
	ManuPower = quantity;
}


/* Add the options widgets to the widget screen */
BOOL intAddOptions(void)
{
	W_FORMINIT	sFormInit;
	W_EDBINIT	sEdInit;
	W_BUTINIT	sButInit;
	W_LABINIT	sLabInit;
	UDWORD		player;
#ifdef EDIT_OPTIONS
	char		aText[WIDG_MAXSTR];
#endif

//	widgEndScreen(psWScreen);

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	memset(&sEdInit, 0, sizeof(W_EDBINIT));

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
		return FALSE;
	}

	// set the interface mode
	intMode = INT_OPTION;

	/* Add the Option screen label */
	sLabInit.formID = IDOPT_FORM;
	sLabInit.id = IDOPT_LABEL;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = OPT_GAP;
	sLabInit.y = OPT_GAP;
	sLabInit.width = OPT_BUTWIDTH;
	sLabInit.height = OPT_BUTHEIGHT;
	sLabInit.pText = "Options";
	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}


	/* Add the close box */
	sButInit.formID = IDOPT_FORM;
	sButInit.id = IDOPT_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = OPT_WIDTH - OPT_GAP - CLOSE_SIZE;
	sButInit.y = OPT_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.FontID = font_regular;
	sButInit.pText = pCloseText;
	sButInit.pTip = _("Close");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}

#ifdef EDIT_OPTIONS
	/* Add the map form */
	sFormInit.formID = IDOPT_FORM;
	sFormInit.id = IDOPT_MAPFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_GAP;
	sFormInit.y = OPT_MAPY;
	sFormInit.width = OPT_WIDTH - OPT_GAP*2;
	sFormInit.height = OPT_BUTHEIGHT*2 + OPT_GAP*3;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	/* Add the map label */
	sLabInit.formID = IDOPT_MAPFORM;
	sLabInit.id = IDOPT_MAPLABEL;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = OPT_GAP;
	sLabInit.y = OPT_GAP;
	sLabInit.pText = "Map:";
	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}

	/* Add the load save and new buttons */
	sButInit.formID = IDOPT_MAPFORM;
	sButInit.id = IDOPT_MAPLOAD;
	sButInit.x = OPT_GAP*2 + OPT_BUTWIDTH;
	sButInit.y = OPT_GAP;
	sButInit.width = OPT_BUTWIDTH;
	sButInit.height = OPT_BUTHEIGHT;
	sButInit.pText = "Load";
	sButInit.pTip = "Load Map File";
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}
	sButInit.id = IDOPT_MAPSAVE;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = "Save";
	sButInit.pTip = "Save Map File";
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}
	sButInit.id = IDOPT_MAPNEW;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_GAP*2 + OPT_BUTHEIGHT;
	sButInit.pText = "New";
	sButInit.pTip = "New Blank Map";
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}

	/* Add the map size edit boxes */
	newMapWidth = mapWidth;
	newMapHeight = mapHeight;
	sEdInit.formID = IDOPT_MAPFORM;
	sEdInit.id = IDOPT_MAPWIDTH;
	sEdInit.style = WEDB_PLAIN;
	sEdInit.x = OPT_GAP*2 + OPT_BUTWIDTH;
	sEdInit.y = OPT_GAP*2 + OPT_BUTHEIGHT;
	sEdInit.width = OPT_BUTWIDTH;
	sEdInit.height = OPT_BUTHEIGHT;
	sEdInit.pText = aText;
	sprintf(aText, "%d", mapWidth);
	sEdInit.FontID = font_regular;
	if (!widgAddEditBox(psWScreen, &sEdInit))
	{
		return FALSE;
	}
	sEdInit.id = IDOPT_MAPHEIGHT;
	sEdInit.x += OPT_GAP + OPT_BUTWIDTH;
	sprintf(aText, "%d", mapHeight);
	if (!widgAddEditBox(psWScreen, &sEdInit))
	{
		return FALSE;
	}
#endif

	/* Add the edit button */
	sButInit.formID = IDOPT_FORM;
	sButInit.id = IDOPT_EDIT;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_EDITY;
	sButInit.width = OPT_BUTWIDTH;
	sButInit.height = OPT_BUTHEIGHT;
#ifdef EDIT_OPTIONS
	sButInit.pText = "Edit";
	sButInit.pTip = "Start Edit Mode";
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}
#endif

	/* Add the add object buttons */
	sButInit.id = IDOPT_DROID;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = _("Unit");
	sButInit.pTip = _("Place Unit on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}

	sButInit.id = IDOPT_STRUCT;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = _("Struct");
	sButInit.pTip = _("Place Structures on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}

	sButInit.id = IDOPT_FEATURE;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = _("Feat");
	sButInit.pTip = _("Place Features on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}

	/* Add the quit button */
	sButInit.formID = IDOPT_FORM;
	sButInit.id = IDOPT_QUIT;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_HEIGHT - OPT_GAP - OPT_BUTHEIGHT;
	sButInit.width = OPT_WIDTH - OPT_GAP*2;
	sButInit.height = OPT_BUTHEIGHT;
	sButInit.pText = _("Quit");
	sButInit.pTip = _("Exit Game");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}

	/* Add the player form */
	sFormInit.formID = IDOPT_FORM;
	sFormInit.id = IDOPT_PLAYERFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_GAP;
	sFormInit.y = OPT_PLAYERY;
	sFormInit.width = OPT_WIDTH - OPT_GAP*2;
	sFormInit.height = OPT_BUTHEIGHT*3 + OPT_GAP*4;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	/* Add the player label */
	sLabInit.formID = IDOPT_PLAYERFORM;
	sLabInit.id = IDOPT_PLAYERLABEL;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = OPT_GAP;
	sLabInit.y = OPT_GAP;
	sLabInit.pText = "Current Player:";
	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}

	/* Add the player buttons */
	sButInit.formID = IDOPT_PLAYERFORM;
	sButInit.id = IDOPT_PLAYERSTART;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_BUTHEIGHT + OPT_GAP*2;
	sButInit.width = OPT_BUTWIDTH;
	sButInit.height = OPT_BUTHEIGHT;
	sButInit.FontID = font_regular;
	for(player = 0; player < MAX_PLAYERS; player++)
	{
		sButInit.pText = apPlayerText[player];
		sButInit.pTip = apPlayerTip[player];
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		/* Update the initialisation structure for the next button */
		sButInit.id += 1;
		sButInit.x += OPT_BUTWIDTH+OPT_GAP;
		if (sButInit.x + OPT_BUTWIDTH + OPT_GAP > OPT_WIDTH - OPT_GAP*2)
		{
			sButInit.x = OPT_GAP;
			sButInit.y += OPT_BUTHEIGHT + OPT_GAP;
		}
	}

//	widgStartScreen(psWScreen);
	widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, WBUT_LOCK);

	return TRUE;
}


/* Add the object screen widgets to the widget screen.
 * select is a pointer to a function that returns true when the object is
 * to be added to the screen.
 * getStats is a pointer to a function that returns the appropriate stats
 * for the object.
 * If psSelected != NULL it specifies which object should be hilited.
 */
static BOOL intAddObjectWindow(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected,BOOL bForceStats)
{
	W_FORMINIT		sFormInit;
	W_FORMINIT		sBFormInit,sBFormInit2;
	W_BARINIT		sBarInit;
	W_BARINIT		sBarInit2;
	W_BUTINIT		sButInit;
	UDWORD			displayForm;
	UDWORD			i, statID=0;
    SDWORD          objLoop;
	BASE_OBJECT		*psObj, *psFirst;
	BASE_STATS		*psStats;
	SDWORD			BufferID;
	DROID			*Droid;
	STRUCTURE		*Structure;
	W_LABINIT		sLabInit;
	W_LABINIT		sLabIntObjText;
	W_LABINIT		sLabInitCmdExp;
	W_LABINIT		sLabInitCmdFac;
	W_LABINIT		sLabInitCmdFac2;
//	W_LABINIT		sLabInitCmdFacts;
	BOOL			IsFactory;
	BOOL			Animate = TRUE;
	UWORD           FormX,FormY;

// Is the form already up?
	if(widgGetFromID(psWScreen,IDOBJ_FORM) != NULL) {
		intRemoveObjectNoAnim();
		Animate = FALSE;
	}
	else
	{
		// reset the object position array
		memset(asJumpPos, 0, sizeof(asJumpPos));
	}

	Animate = FALSE;

	ClearObjectBuffers();
	ClearTopicBuffers();

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sBFormInit, 0, sizeof(W_FORMINIT));
	memset(&sBFormInit2, 0, sizeof(W_FORMINIT));
	memset(&sBarInit, 0, sizeof(W_BARINIT));

	/* See how many objects the player has */
	numObjects = 0;
	psFirst = NULL;
	memset(apsObjectList, 0, sizeof(BASE_OBJECT *) * MAX_OBJECTS);
	for(psObj=psObjects; psObj; psObj = psObj->psNext)
	{
		if (objSelectFunc(psObj))
		{
			apsObjectList[numObjects] = psObj;
			numObjects++;
			if (numObjects == 1)
			{
				psFirst = psObj;
			}

			// make sure the list doesn't overflow
			if (numObjects >= MAX_OBJECTS)
			{
				break;
			}
		}
	}

	if(numObjects == 0) {
		// No objects so close the stats window if it's up...
		if(widgGetFromID(psWScreen,IDSTAT_FORM) != NULL) {
			intRemoveStatsNoAnim();
		}
		// and return.
		return FALSE;
	}

    /*if psSelected != NULL then check its in the list of suitable objects for
    this instance of the interface - this could happen when a structure is upgraded*/
	objLoop = 0;
    if (psSelected != NULL)
    {
        for(objLoop = 0; objLoop < numObjects; objLoop++)
        {
            if (psSelected == apsObjectList[objLoop])
            {
                //found it so quit loop
                break;
            }
        }
    }
    //if have reached the end of the loop and not quit out, then can't have found the selected object in the list
    if (objLoop == numObjects)
    {
        //initialise psSelected so gets set up with an iten in the list
        psSelected = NULL;
    }

	//order the objects according to what they are
	orderObjectInterface();

// wont ever get here cause if theres no research facility then the research reticule button
// is disabled so commented out.
//	if (numObjects == 0 && objMode == IOBJ_RESEARCH)
//	{
//		audio_QueueTrack(ID_SOUND_RESEARCH_FAC_REQ);
//		return FALSE;
//	}

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
			if (apsPreviousObj[objMode] && apsPreviousObj[objMode]->player == selectedPlayer)
			{
				psSelected = apsPreviousObj[objMode];
				//it is possible for a structure to change status - building of modules
				if (psSelected->type == OBJ_STRUCTURE)
				{
					if (((STRUCTURE *)psSelected)->status != SS_BUILT)
					{
						//structure not complete so just set selected to the first valid object
						psSelected = psFirst;
					}
				}
			}
			else
			{
				psSelected = psFirst;
			}
		}
		//make sure this matches in game once decided - DON'T!
		//clearSelection();
		//psSelected->selected = TRUE;
	}

	/* Reset the current object and store the current list */
	psObjSelected = NULL;
	psObjList = psObjects;

	/* Create the basic form */

	sFormInit.formID = 0;
	sFormInit.id = IDOBJ_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)OBJ_BACKX;
	sFormInit.y = (SWORD)OBJ_BACKY;
	FormX = sFormInit.x;
	FormY = sFormInit.y;
	sFormInit.width = OBJ_BACKWIDTH;
	sFormInit.height = 	OBJ_BACKHEIGHT;
// If the window was closed then do open animation.
	if(Animate) {
		sFormInit.pDisplay = intOpenPlainForm;
		sFormInit.disableChildren = TRUE;
	} else {
// otherwise just recreate it.
		sFormInit.pDisplay = intDisplayPlainForm;
	}
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	/* Add the close button */
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = IDOBJ_FORM;
	sButInit.id = IDOBJ_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = OBJ_BACKWIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}


	/*add the tabbed form */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDOBJ_FORM;
	sFormInit.id = IDOBJ_TABFORM;
	sFormInit.style = WFORM_TABBED;
	sFormInit.x = OBJ_TABX;
	sFormInit.y = OBJ_TABY;
	sFormInit.width = OBJ_WIDTH;
	sFormInit.height = OBJ_HEIGHT;
	sFormInit.numMajor = numForms((OBJ_BUTWIDTH + OBJ_GAP) * numObjects,
								  OBJ_WIDTH - OBJ_GAP);
	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = OBJ_TABWIDTH;
	sFormInit.majorOffset = OBJ_TABOFFSET;
	sFormInit.tabVertOffset = (OBJ_TABHEIGHT/2);
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;
	sFormInit.tabMajorGap = OBJ_TABOFFSET;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;

	if (sFormInit.numMajor > MAX_TAB_STD_SHOWN)
	{
		// We do NOT want more than this amount of tabs, 40 items should be more than enough(?)
		sFormInit.numMajor = MAX_TAB_STD_SHOWN;
		// If we were to change this in future then :
		//Just switching from normal sized tabs to smaller ones to fit more in form.
		//		sFormInit.pUserData = &SmallTab;
		//		sFormInit.majorSize /= 2;
		// Change MAX_TAB_STD_SHOWN to ..SMALL_SHOWN, this will give us 80 items max.
	}
	for (i=0; i< sFormInit.numMajor; i++)
	{
		sFormInit.aNumMinors[i] = 1;
	}
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	/* Store the number of tabs */
	objNumTabs = sFormInit.numMajor;

	/* Add the object and stats buttons */
	sBFormInit.formID = IDOBJ_TABFORM;
	sBFormInit.id = IDOBJ_OBJSTART;
	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	sBFormInit.x = OBJ_STARTX;
	sBFormInit.y = OBJ_STARTY;
	sBFormInit.width = OBJ_BUTWIDTH;
	sBFormInit.height = OBJ_BUTHEIGHT;
	memcpy(&sBFormInit2,&sBFormInit,sizeof(W_FORMINIT));
	sBFormInit2.id = IDOBJ_STATSTART;
	sBFormInit2.y = OBJ_STATSTARTY;
	//right click on a Template will put the production on hold
	sBFormInit2.style = WFORM_CLICKABLE | WFORM_SECONDARY;

// Action progress bar.
	sBarInit.formID = IDOBJ_OBJSTART;
	sBarInit.id = IDOBJ_PROGBARSTART;
	sBarInit.style = WBAR_TROUGH | WIDG_HIDDEN;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = STAT_PROGBARX;
	sBarInit.y = STAT_PROGBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 0;
	sBarInit.sCol.byte.r = STAT_PROGBARMAJORRED;
	sBarInit.sCol.byte.g = STAT_PROGBARMAJORGREEN;
	sBarInit.sCol.byte.b = STAT_PROGBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = STAT_PROGBARMINORRED;
	sBarInit.sMinorCol.byte.g = STAT_PROGBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = STAT_PROGBARMINORBLUE;
	sBarInit.pTip = _("Progress Bar");

    //object output bar ie manuf power o/p, research power o/p
	memcpy(&sBarInit2,&sBarInit,sizeof(W_BARINIT));
	sBarInit2.id = IDOBJ_POWERBARSTART;
	sBarInit2.style = WBAR_PLAIN;
	sBarInit2.x = STAT_POWERBARX;
	sBarInit2.y = STAT_POWERBARY;
	sBarInit2.size = 50;
    //don't set the tip cos we haven't got a suitable text string at this point - 2/2/99
	//sBarInit2.pTip = _("Build Speed");
    sBarInit2.pTip = NULL;

	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.id = IDOBJ_COUNTSTART;
	sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInit.x = OBJ_TEXTX;
	sLabInit.y = OBJ_T1TEXTY;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "10";
	sLabInit.FontID = font_regular;

	memset(&sLabInitCmdFac,0,sizeof(W_LABINIT));
	sLabInitCmdFac.id = IDOBJ_CMDFACSTART;
	sLabInitCmdFac.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInitCmdFac.x = OBJ_TEXTX;
	sLabInitCmdFac.y = OBJ_T2TEXTY;
	sLabInitCmdFac.width = 16;
	sLabInitCmdFac.height = 16;
	sLabInitCmdFac.pText = "10";
	sLabInitCmdFac.FontID = font_regular;

	memset(&sLabInitCmdFac2,0,sizeof(W_LABINIT));
	sLabInitCmdFac2.id = IDOBJ_CMDVTOLFACSTART;
	sLabInitCmdFac2.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInitCmdFac2.x = OBJ_TEXTX;
	sLabInitCmdFac2.y = OBJ_T3TEXTY;
	sLabInitCmdFac2.width = 16;
	sLabInitCmdFac2.height = 16;
	sLabInitCmdFac2.pText = "10";
	sLabInitCmdFac2.FontID = font_regular;

	memset(&sLabIntObjText,0,sizeof(W_LABINIT));
	sLabIntObjText.id = IDOBJ_FACTORYSTART;
	sLabIntObjText.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabIntObjText.x = OBJ_TEXTX;
	sLabIntObjText.y = OBJ_B1TEXTY;
	sLabIntObjText.width = 16;
	sLabIntObjText.height = 16;
	sLabIntObjText.pText = "xxx/xxx - overrun";
	sLabIntObjText.FontID = font_regular;

	memset(&sLabInitCmdExp,0,sizeof(W_LABINIT));
	sLabInitCmdExp.id = IDOBJ_CMDEXPSTART;
	sLabInitCmdExp.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInitCmdExp.x = STAT_POWERBARX;
	sLabInitCmdExp.y = STAT_POWERBARY;
	sLabInitCmdExp.width = 16;
	sLabInitCmdExp.height = 16;
	sLabInitCmdExp.pText = "@@@@@ - overrun";
	sLabInitCmdExp.FontID = font_regular;


	displayForm = 0;
	for(i=0; i<(UDWORD)numObjects; i++)
	{
		psObj = apsObjectList[i];
		if(psObj->died == 0) {	// Don't add the button if the objects dead.
			IsFactory = FALSE;

			/* Got an object - set the text and tip for the button */
			switch (psObj->type)
			{
			case OBJ_DROID:
	// Get the construction power of a construction droid.. Not convinced this is right.
				Droid = (DROID*)psObj;
				if (Droid->droidType == DROID_CONSTRUCT ||
                    Droid->droidType == DROID_CYBORG_CONSTRUCT)
				{
			   		ASSERT( Droid->asBits[COMP_CONSTRUCT].nStat,"intUpdateProgressBar: invalid droid type" );
					psStats = (BASE_STATS*)(asConstructStats + Droid->asBits[COMP_CONSTRUCT].nStat);
					//sBarInit2.size = (UWORD)((CONSTRUCT_STATS*)psStats)->constructPoints;	// Need to scale? YEP!
					sBarInit2.size = (UWORD)constructorPoints((CONSTRUCT_STATS*)psStats,
						Droid->player);
					if (sBarInit2.size > WBAR_SCALE)
					{
						sBarInit2.size = WBAR_SCALE;
					}
				}
	//				sBFormInit.pTip = ((DROID *)psObj)->pName;

				sBFormInit.pTip = droidGetName((DROID *)psObj);

				break;

			case OBJ_STRUCTURE:
	// Get the construction power of a structure..
				Structure = (STRUCTURE *)psObj;
				switch(Structure->pStructureType->type) {
					case REF_FACTORY:
					case REF_CYBORG_FACTORY:
					case REF_VTOL_FACTORY:
						sBarInit2.size = (UWORD)((FACTORY*)Structure->
							pFunctionality)->productionOutput;	// Need to scale?
						if (sBarInit2.size > WBAR_SCALE)
						{
							sBarInit2.size = WBAR_SCALE;
						}
						IsFactory = TRUE;
						//right click on factory centres on DP
						sBFormInit.style = WFORM_CLICKABLE | WFORM_SECONDARY;
						break;

					case REF_RESEARCH:
						sBarInit2.size = (UWORD)((RESEARCH_FACILITY*)Structure->
							pFunctionality)->researchPoints;	// Need to scale?
						if (sBarInit2.size > WBAR_SCALE)
						{
							sBarInit2.size = WBAR_SCALE;
						}
						break;

					default:
						ASSERT( FALSE, "intAddObject: invalid structure type" );
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
			BufferID = sBFormInit.id-IDOBJ_OBJSTART;
			ASSERT( BufferID < NUM_TOPICBUFFERS,"BufferID > NUM_TOPICBUFFERS" );
			ClearTopicButtonBuffer(BufferID);
			RENDERBUTTON_INUSE(&TopicBuffers[BufferID]);
			TopicBuffers[BufferID].Data = (void*)psObj;
			sBFormInit.pUserData = &TopicBuffers[BufferID];
			sBFormInit.pDisplay = intDisplayObjectButton;


			if (!widgAddForm(psWScreen, &sBFormInit))
			{
				return FALSE;
			}


			if (IsFactory)
			{
				// Add a text label for the factory Inc.
				sLabIntObjText.formID = sBFormInit.id;
				sLabIntObjText.pCallback = intAddFactoryInc;
				sLabIntObjText.pUserData = psObj;
				if (!widgAddLabel(psWScreen, &sLabIntObjText))
				{
					return FALSE;
				}
				sLabIntObjText.id++;
			}
			// Add the power bar.
			if (psObj->type != OBJ_DROID ||
				(((DROID *)psObj)->droidType == DROID_CONSTRUCT ||
                ((DROID *)psObj)->droidType == DROID_CYBORG_CONSTRUCT))
			{
				sBarInit2.formID = sBFormInit.id;
				sBarInit.iRange = GAME_TICKS_PER_SEC;
				if (!widgAddBarGraph(psWScreen, &sBarInit2))
				{
					return FALSE;
				}
			}

			// Add command droid bits
			if ( (psObj->type == OBJ_DROID) &&
				 (((DROID *)psObj)->droidType == DROID_COMMAND) )
			{
				// the group size label
				sLabIntObjText.formID = sBFormInit.id;
				sLabIntObjText.pCallback = intUpdateCommandSize;
				sLabIntObjText.pUserData = psObj;
				if (!widgAddLabel(psWScreen, &sLabIntObjText))
				{
					return FALSE;
				}
				sLabIntObjText.id++;

				// the experience stars
				sLabInitCmdExp.formID = sBFormInit.id;
				sLabInitCmdExp.pCallback = intUpdateCommandExp;
	//			sLabInitCmdExp.pDisplay = intDisplayCommandExp;
				sLabInitCmdExp.pUserData = psObj;
				if (!widgAddLabel(psWScreen, &sLabInitCmdExp))
				{
					return FALSE;
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


				BufferID = (sBFormInit2.id-IDOBJ_STATSTART)*2+1;
				ASSERT( BufferID < NUM_OBJECTBUFFERS,"BufferID > NUM_OBJECTBUFFERS" );
				ClearObjectButtonBuffer(BufferID);
				RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
				ObjectBuffers[BufferID].Data = (void*)psObj;
				ObjectBuffers[BufferID].Data2 = (void*)psStats;
				sBFormInit2.pUserData = &ObjectBuffers[BufferID];
			}
			else if ( (psObj->type == OBJ_DROID) && ( ((DROID *)psObj)->droidType == DROID_COMMAND ) )
			{
				sBFormInit2.pTip = NULL;

				BufferID = (sBFormInit2.id-IDOBJ_STATSTART)*2+1;
				ASSERT( BufferID < NUM_OBJECTBUFFERS,"BufferID > NUM_OBJECTBUFFERS" );
				ClearObjectButtonBuffer(BufferID);
				RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
				ObjectBuffers[BufferID].Data = (void*)psObj;
				ObjectBuffers[BufferID].Data2 = NULL;
				sBFormInit2.pUserData = &ObjectBuffers[BufferID];
			}
			else
			{
				sBFormInit2.pTip = NULL;

				BufferID = (sBFormInit2.id-IDOBJ_STATSTART)*2+1;
				ASSERT( BufferID < NUM_OBJECTBUFFERS,"BufferID > NUM_OBJECTBUFFERS" );
				ClearObjectButtonBuffer(BufferID);
				RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
				sBFormInit2.pUserData = &ObjectBuffers[BufferID];
			}

			sBFormInit2.pDisplay = intDisplayStatusButton;


			if (!widgAddForm(psWScreen, &sBFormInit2))
			{
				return FALSE;
			}

			if (psObj->selected)
			{
				widgSetButtonState(psWScreen, sBFormInit2.id, WBUT_CLICKLOCK);
			}



			if ( psObj->type != OBJ_DROID ||
				 (((DROID *)psObj)->droidType == DROID_CONSTRUCT ||
                 ((DROID *)psObj)->droidType == DROID_CYBORG_CONSTRUCT))
			{
				// Set the colour for the production run size text.
				widgSetColour(psWScreen, sBFormInit2.id, WCOL_TEXT,
								STAT_TEXTRED,STAT_TEXTGREEN,STAT_TEXTBLUE);
				widgSetColour(psWScreen, sBFormInit2.id, WCOL_BKGRND,
								STAT_PROGBARTROUGHRED,STAT_PROGBARTROUGHGREEN,STAT_PROGBARTROUGHBLUE);
			}

			// Add command droid bits
			if ( (psObj->type == OBJ_DROID) &&
				 (((DROID *)psObj)->droidType == DROID_COMMAND) )
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
					return FALSE;
				}
				// the assigned VTOL factories label
				sLabInitCmdFac2.formID = sBFormInit2.id;
				sLabInitCmdFac2.pCallback = intUpdateCommandFact;
				sLabInitCmdFac2.pUserData = psObj;
				if (!widgAddLabel(psWScreen, &sLabInitCmdFac2))
				{
					return FALSE;
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
				return FALSE;
			}

			// Add the progress bar.
			sBarInit.formID = sBFormInit2.id;
			// Setup widget update callback and object pointer so we can update the progress bar.
			sBarInit.pCallback = intUpdateProgressBar;
			sBarInit.pUserData = psObj;
			sBarInit.iRange = GAME_TICKS_PER_SEC;

			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return FALSE;
			}


			/* If this matches psSelected note which form to display */
			if (psSelected == psObj)
			{
				displayForm = sBFormInit.majorID;
				statID = sBFormInit2.id;
			}

			/* Set up the next button (Objects) */
			sBFormInit.id += 1;
			ASSERT( sBFormInit.id < IDOBJ_OBJEND,"Too many object buttons" );

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
			ASSERT( sBarInit.id < IDOBJ_PROGBAREND,"Too many progress bars" );

			sBarInit2.id += 1;
			ASSERT( sBarInit2.id < IDOBJ_POWERBAREND,"Too many power bars" );

			sBFormInit2.id += 1;
			ASSERT( sBFormInit2.id < IDOBJ_STATEND,"Too many stat buttons" );

			sBFormInit2.x += OBJ_BUTWIDTH + OBJ_GAP;
			if (sBFormInit2.x + OBJ_BUTWIDTH + OBJ_GAP > OBJ_WIDTH)
			{
				sBFormInit2.x = OBJ_STARTX;
				sBFormInit2.majorID += 1;
			}

			if (sBFormInit.id > IDOBJ_OBJEND)
			{
				//can't fit any more on the screen!
				debug( LOG_NEVER, "This is just a Warning!\n Max buttons have been allocated" );
				break;
			}
		} else {
//DBPRINTF(("Skipped dead object\n");
		}
	}

//	widgStartScreen(psWScreen);
	widgSetTabs(psWScreen, IDOBJ_TABFORM, (UWORD)displayForm, 0);

	// if the selected object isn't on one of the main buttons (too many objects)
	// reset the selected pointer
	if (statID == 0)
	{
		psSelected = NULL;
	}

//DBPRINTF(("%p %d\n",psSelected,bForceStats);
	if (psSelected && (objMode != IOBJ_COMMAND))
	{
		if(bForceStats || widgGetFromID(psWScreen,IDSTAT_FORM ) )
		{
//DBPRINTF(("intAddObjectStats %p %d\n",psSelected,statID));
			objStatID = statID;
			intAddObjectStats(psSelected, statID);
			intMode = INT_STAT;
		} else {
			widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);
			intMode = INT_OBJECT;
		}
	}
	else if (psSelected)
	{
		/* Note the object */
		psObjSelected = psSelected;
		objStatID = statID;
// We don't want to be locking the button for command droids.
//		widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);

        //changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
		//intAddOrder((DROID *)psSelected);
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

//	if ((objMode==IOBJ_RESEARCH) && bInTutorial)
	if (bInTutorial)
	{
		debug( LOG_NEVER, "Go with object open callback!\n" );
	 	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_OBJECTOPEN);
	}

	return TRUE;
}


static BOOL intUpdateObject(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected,BOOL bForceStats)
{

	intAddObjectWindow(psObjects,psSelected,bForceStats);

	// if the stats screen is up and..
	if(StatsUp) {
		if(psStatsScreenOwner != NULL) {
			// it's owner is dead then..
			if(psStatsScreenOwner->died != 0) {
				// remove it.
//DBPRINTF(("psStatsScreenOwner died\n");
				intRemoveStatsNoAnim();
			}
		}
	}

	return TRUE;
}

/* Remove the build widgets from the widget screen */
void intRemoveObject(void)
{

	W_TABFORM *Form;

	widgDelete(psWScreen, IDOBJ_TABFORM);
	widgDelete(psWScreen, IDOBJ_CLOSE);

// Start the window close animation.
	Form = (W_TABFORM*)widgGetFromID(psWScreen,IDOBJ_FORM);
	if(Form) {
		Form->display = intClosePlainForm;
		Form->disableChildren = TRUE;
		Form->pUserData = NULL; // Used to signal when the close anim has finished.
		ClosingObject = TRUE;
	}



	ClearObjectBuffers();
	ClearTopicBuffers();

	intHidePowerBar();

	if (bInTutorial)
	{
		debug( LOG_NEVER, "Go with object close callback!\n" );
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

/*	if (bInTutorial)
	{
		DBPRINTF(("Go with object close callback!(noanim)\n"));
	 	eventFireCallbackTrigger(CALL_OBJECTCLOSE);
	}*/

}


/* Remove the stats widgets from the widget screen */
void intRemoveStats(void)
{
	W_TABFORM *Form;

#ifdef INCLUDE_PRODSLIDER
	widgDelete(psWScreen, IDSTAT_SLIDERCOUNT);
	widgDelete(psWScreen, IDSTAT_SLIDER);
#endif
	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);

// Start the window close animation.
	Form = (W_TABFORM*)widgGetFromID(psWScreen,IDSTAT_FORM);
	if(Form) {
		Form->display = intClosePlainForm;
		Form->pUserData = NULL; // Used to signal when the close anim has finished.
		Form->disableChildren = TRUE;
		ClosingStats = TRUE;
	}

	ClearStatBuffers();

	StatsUp = FALSE;
	psStatsScreenOwner = NULL;
}


/* Remove the stats widgets from the widget screen */
void intRemoveStatsNoAnim(void)
{
#ifdef INCLUDE_PRODSLIDER
	widgDelete(psWScreen, IDSTAT_SLIDERCOUNT);
	widgDelete(psWScreen, IDSTAT_SLIDER);
#endif
	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);
	widgDelete(psWScreen, IDSTAT_FORM);

	ClearStatBuffers();

	StatsUp = FALSE;
	psStatsScreenOwner = NULL;
}

// Poll for closing windows and handle them, ensure called even if game is paused.
//
void HandleClosingWindows(void)
{
	WIDGET *Widg;

	if(ClosingObject) {
		Widg = widgGetFromID(psWScreen,IDOBJ_FORM);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDOBJ_FORM);
				ClosingObject = FALSE;
			}
		} else {
			ClosingObject = FALSE;
		}
	}

	if(ClosingStats) {
		Widg = widgGetFromID(psWScreen,IDSTAT_FORM);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDSTAT_FORM);
				ClosingStats = FALSE;
			}
		} else {
			ClosingStats = FALSE;
		}
	}
	if(ClosingMessageView) {
		Widg = widgGetFromID(psWScreen,IDINTMAP_MSGVIEW);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDINTMAP_MSGVIEW);
				ClosingMessageView = FALSE;
			}
		} else {
			ClosingMessageView = FALSE;
		}
	}
	if(ClosingIntelMap) {
		Widg = widgGetFromID(psWScreen,IDINTMAP_FORM);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDINTMAP_FORM);
				ClosingIntelMap = FALSE;
			}
		} else {
			ClosingIntelMap = FALSE;
		}
	}

	if(ClosingOrder) {
		Widg = widgGetFromID(psWScreen,IDORDER_FORM);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDORDER_FORM);
				ClosingOrder = FALSE;
			}
		} else {
			ClosingOrder = FALSE;
		}
	}
	if(ClosingTrans) {
		Widg = widgGetFromID(psWScreen,IDTRANS_FORM);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDTRANS_FORM);
				ClosingTrans = FALSE;
			}
		} else {
			ClosingTrans = FALSE;
		}
	}
	if(ClosingTransCont) {
		Widg = widgGetFromID(psWScreen,IDTRANS_CONTENTFORM);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDTRANS_CONTENTFORM);
				ClosingTransCont = FALSE;
			}
		} else {
			ClosingTransCont = FALSE;
		}
	}
	if(ClosingTransDroids) {
		Widg = widgGetFromID(psWScreen,IDTRANS_DROIDS);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, IDTRANS_DROIDS);
				ClosingTransDroids = FALSE;
			}
		} else {
			ClosingTransDroids = FALSE;
		}
	}

	if(ClosingInGameOp) {
		Widg = widgGetFromID(psWScreen,INTINGAMEOP);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, INTINGAMEOP);
				ClosingInGameOp = FALSE;
			}
		} else {
			ClosingInGameOp = FALSE;
		}
	}

	//if(ClosingMissionRes) {
	//	Widg = widgGetFromID(psWScreen,IDMISSIONRES_FORM);
	//	if(Widg) {
// Has the window finished closing?
	//		if( ((UDWORD)Widg->pUserData) ) {
	//			intRemoveMissionResultNoAnim();
	//			resetMissionPauseState();	//reset the pauses
	//		}
	//	} else {
	//		ClosingMissionRes = FALSE;
	//		//reset the pauses
	//		resetMissionPauseState();
	//	}
	//}


	if(ClosingMultiMenu) {
		Widg = widgGetFromID(psWScreen,MULTIMENU_FORM);
		if(Widg) {
// Has the window finished closing?
			if( Widg->pUserData ) {
				widgDelete(psWScreen, MULTIMENU_FORM);
				ClosingMultiMenu = FALSE;
			}
		} else {
			ClosingMultiMenu = FALSE;
		}
	}
}


/* Get the object refered to by a button ID on the object screen.
 * This works for object or stats buttons
 */
static BASE_OBJECT *intGetObject(UDWORD id)
{
//	UDWORD			objID;
	BASE_OBJECT		*psObj;

	/* If this is a stats button, find the object button linked to it */
	if (id >= IDOBJ_STATSTART &&
		id <= IDOBJ_STATEND)
	{
		id = IDOBJ_OBJSTART + id - IDOBJ_STATSTART;
	}

	/* Find the object that the ID refers to */
	ASSERT( ( (SDWORD)id - IDOBJ_OBJSTART >= 0 ) &&
			 ( (SDWORD)id - IDOBJ_OBJSTART < numObjects ),
		"intGetObject: invalid button ID" );
	psObj = apsObjectList[id - IDOBJ_OBJSTART];
/*	objID = IDOBJ_OBJSTART;
	for(psObj = psObjList; psObj; psObj = psObj->psNext)
	{
		if (objSelectFunc(psObj))
		{
			if (objID == id)
			{
				// Found the object so jump out of the loops
				goto found;
			}
			objID++;
		}
	}
found:	// Jump to here if an object is found
	ASSERT( psObj != NULL, "intGetObject: couldn't match id to button" );
	*/

	return psObj;
}


/* Reset the stats button for an object */
static void intSetStats(UDWORD id, BASE_STATS *psStats)
{
	W_FORMINIT	sFormInit;
	W_BARINIT	sBarInit;
	W_LABINIT	sLabInit;
	UDWORD		butPerForm, butPos;
	SDWORD BufferID;
	BASE_OBJECT	*psObj;

	/* Update the button on the object screen */
	widgDelete(psWScreen, id);

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sBarInit, 0, sizeof(W_BARINIT));

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
	sBarInit.formID = id;
	sBarInit.id = (id - IDOBJ_STATSTART) + IDOBJ_PROGBARSTART;
	sBarInit.style = WBAR_TROUGH;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = STAT_PROGBARX;
	sBarInit.y = STAT_PROGBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 0;
	sBarInit.sCol.byte.r = STAT_PROGBARMAJORRED;
	sBarInit.sCol.byte.g = STAT_PROGBARMAJORGREEN;
	sBarInit.sCol.byte.b = STAT_PROGBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = STAT_PROGBARMINORRED;
	sBarInit.sMinorCol.byte.g = STAT_PROGBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = STAT_PROGBARMINORBLUE;
	sBarInit.iRange = GAME_TICKS_PER_SEC;
	// Setup widget update callback and object pointer so we can update the progress bar.
	sBarInit.pCallback = intUpdateProgressBar;
	sBarInit.pUserData = intGetObject(id);

	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.formID = id;
	sLabInit.id = (id - IDOBJ_STATSTART) + IDOBJ_COUNTSTART;
	sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;
	sLabInit.x = OBJ_TEXTX;
	sLabInit.y = OBJ_T1TEXTY;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "10";
	sLabInit.FontID = font_regular;


	if (psStats)
	{
//		sButInit.pText = "S";
		//sFormInit.pTip = psStats->pName;
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

		BufferID = (sFormInit.id-IDOBJ_STATSTART)*2+1;
//		DBPRINTF(("2 *sFormInit.id-IDOBJ_STATSTART : %d\n",BufferID));
//		BufferID = GetObjectBuffer();
		ASSERT( BufferID < NUM_OBJECTBUFFERS,"BufferID > NUM_OBJECTBUFFERS" );
		ClearObjectButtonBuffer(BufferID);
		RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
		ObjectBuffers[BufferID].Data = (void*)intGetObject(id);
		ObjectBuffers[BufferID].Data2 = (void*)psStats;
		sFormInit.pUserData = &ObjectBuffers[BufferID];

		// Add a text label for the size of the production run.
		sLabInit.pCallback = intUpdateQuantity;
		sLabInit.pUserData = sBarInit.pUserData;
//		sFormInit.pUserData = (void*)intGetObject(id);
	}
	else
	{
//		sButInit.pText = "NONE";
		sFormInit.pTip = NULL;

		BufferID = (sFormInit.id-IDOBJ_STATSTART)*2+1;
//		DBPRINTF(("2 sFormInit.id-IDOBJ_STATSTART : %d\n",BufferID));
//		BufferID = GetObjectBuffer();
		ASSERT( BufferID < NUM_OBJECTBUFFERS,"BufferID > NUM_OBJECTBUFFERS" );
		ClearObjectButtonBuffer(BufferID);
		RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
		sFormInit.pUserData = &ObjectBuffers[BufferID];

//		sFormInit.pUserData = NULL;

		/* Reset the stats screen button if necessary */
		if ((INTMODE)objMode == INT_STAT && statID != 0)
		{
			widgSetButtonState(psWScreen, statID, 0);
		}
	}

	sFormInit.pDisplay = intDisplayStatusButton;


	widgAddForm(psWScreen, &sFormInit);
	// Set the colour for the production run size text.
	widgSetColour(psWScreen, sFormInit.id, WCOL_TEXT,
							STAT_TEXTRED,STAT_TEXTGREEN,STAT_TEXTBLUE);
	widgSetColour(psWScreen, sFormInit.id, WCOL_BKGRND,
							STAT_PROGBARTROUGHRED,STAT_PROGBARTROUGHGREEN,STAT_PROGBARTROUGHBLUE);


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
static BOOL intAddStats(BASE_STATS **ppsStatsList, UDWORD numStats,
						BASE_STATS *psSelected, BASE_OBJECT *psOwner)
{
	W_FORMINIT			sFormInit;
	W_BUTINIT			sButInit;
	W_FORMINIT			sBFormInit;
	W_BARINIT			sBarInit;
	UDWORD				i, butPerForm, statForm;
	SDWORD				BufferID;
	BASE_STATS			*Stat;
	BOOL				Animate = TRUE;
	W_LABINIT			sLabInit;
	FACTORY				*psFactory;
#ifdef INCLUDE_PRODSLIDER
	W_SLDINIT			sSldInit;
#endif
	//char				sCaption[6];

	// should this ever be called with psOwner == NULL?

	// Is the form already up?
	if(widgGetFromID(psWScreen,IDSTAT_FORM) != NULL) {
		intRemoveStatsNoAnim();
		Animate = FALSE;
	}

	// is the order form already up ?
	if (widgGetFromID(psWScreen, IDORDER_FORM) != NULL)
	{
		intRemoveOrderNoAnim();
	}

	Animate = FALSE;

	if (psOwner != NULL)
	{
		// Return if the owner is dead.
		if (psOwner->died)
		{
			debug(LOG_GUI, "intAddStats: Owner is dead");
			return FALSE;
		}
	}

	psStatsScreenOwner = psOwner;

	ClearStatBuffers();

	widgEndScreen(psWScreen);

	/* Create the basic form */

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = IDSTAT_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = STAT_X;
	sFormInit.y = (SWORD)STAT_Y;
	sFormInit.width = STAT_WIDTH;
	sFormInit.height = 	STAT_HEIGHT;
// If the window was closed then do open animation.
	if(Animate) {
		sFormInit.pDisplay = intOpenPlainForm;
		sFormInit.disableChildren = TRUE;
	} else {
// otherwise just recreate it.
		sFormInit.pDisplay = intDisplayPlainForm;
	}
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		debug(LOG_ERROR, "intAddStats: Failed to add form");
		return FALSE;
	}

#ifdef INCLUDE_PRODSLIDER
	// Add the quantity slider ( if it's a factory ).
	if(objMode == IOBJ_MANUFACTURE) {

		//add the non stop production button
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_INFINITE_BUTTON;
		sButInit.style = WBUT_PLAIN;
		sButInit.x = STAT_SLDX + STAT_SLDWIDTH + 2;
		sButInit.y = STAT_SLDY;
		sButInit.width = iV_GetImageWidth(IntImages,IMAGE_INFINITE_DOWN);
		sButInit.height = iV_GetImageHeight(IntImages,IMAGE_INFINITE_DOWN);
	//	sButInit.pText = pCloseText;
		sButInit.pTip = "Infinite Production";
		sButInit.FontID = font_regular;
		sButInit.pDisplay = intDisplayButtonPressed;
		sButInit.UserData = PACKDWORD_TRI(IMAGE_INFINITE_DOWN, IMAGE_INFINITE_HI, IMAGE_INFINITE_UP);
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		//add the number display
		memset(&sLabInit,0,sizeof(W_LABINIT));
		sLabInit.formID = IDSTAT_FORM;	//0;
		sLabInit.id = IDSTAT_SLIDERCOUNT;
		sLabInit.style = WLAB_PLAIN;
		sLabInit.x = (SWORD)(STAT_SLDX + STAT_SLDWIDTH + sButInit.width + 2);
		sLabInit.y = STAT_SLDY + 3;
		sLabInit.width = 16;
		sLabInit.height = 16;
		sLabInit.FontID = font_regular;
		sLabInit.pUserData = psOwner;
		//sLabInit.pCallback = intUpdateSlider;
		sLabInit.pDisplay = intDisplayNumber;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return FALSE;
		}


		memset(&sSldInit, 0, sizeof(W_SLDINIT));
		sSldInit.formID = IDSTAT_FORM;
		sSldInit.id = IDSTAT_SLIDER;
		sSldInit.style = WSLD_PLAIN;
		sSldInit.x = STAT_SLDX;
		sSldInit.y = STAT_SLDY;
		sSldInit.width = STAT_SLDWIDTH;
		sSldInit.height = STAT_SLDHEIGHT;
		sSldInit.orientation = WSLD_LEFT;
		sSldInit.numStops = STAT_SLDSTOPS-1;
		sSldInit.barSize = iV_GetImageHeight(IntImages,IMAGE_SLIDER_BUT);
		sSldInit.pos = 0;
		if ( psOwner != NULL )
		{
			psFactory = (FACTORY *)((STRUCTURE *)psOwner)->pFunctionality;
			if (psFactory->psSubject)
			{
				if (psFactory->quantity > sSldInit.numStops)
				{
					sSldInit.pos = sSldInit.numStops;
				}
				else
				{
					sSldInit.pos = (UWORD)psFactory->quantity;
				}
			}
		}

		sSldInit.pDisplay = intDisplaySlider;
		if (!widgAddSlider(psWScreen, &sSldInit))
		{
			return FALSE;
		}
	}
#endif

#ifdef INCLUDE_FACTORYLISTS

	// Add the quantity slider ( if it's a factory ).
	if(objMode == IOBJ_MANUFACTURE)
	{
		//add the Factory DP button
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_DP_BUTTON;
		sButInit.style = WBUT_PLAIN | WFORM_SECONDARY;
		sButInit.x = 4;
		sButInit.y = STAT_SLDY;
		sButInit.width = iV_GetImageWidth(IntImages,IMAGE_FDP_DOWN);
		sButInit.height = iV_GetImageHeight(IntImages,IMAGE_FDP_DOWN);
		sButInit.pTip = _("Factory Delivery Point");
		sButInit.FontID = font_regular;
		sButInit.pDisplay = intDisplayDPButton;
		sButInit.pUserData = psOwner;

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		//add the Factory Loop button!
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_LOOP_BUTTON;
		sButInit.style = WBUT_PLAIN | WFORM_SECONDARY;
		sButInit.x = STAT_SLDX + STAT_SLDWIDTH + 2;
		sButInit.y = STAT_SLDY;
		sButInit.width = iV_GetImageWidth(IntImages,IMAGE_LOOP_DOWN);
		sButInit.height = iV_GetImageHeight(IntImages,IMAGE_LOOP_DOWN);
		sButInit.pTip = _("Loop Production");
		sButInit.FontID = font_regular;
		sButInit.pDisplay = intDisplayButtonPressed;
		sButInit.UserData = PACKDWORD_TRI(IMAGE_LOOP_DOWN, IMAGE_LOOP_HI, IMAGE_LOOP_UP);

		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}

		if ( psOwner != NULL )
		{
			psFactory = (FACTORY *)((STRUCTURE *)psOwner)->pFunctionality;
			if (psFactory->psSubject && psFactory->quantity)
			{
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
			}
		}

		// create a text label for the loop quantity.
		memset(&sLabInit,0,sizeof(W_LABINIT));
		sLabInit.formID = IDSTAT_FORM;
		sLabInit.id = IDSTAT_LOOP_LABEL;
		sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;
		sLabInit.x = (UWORD)(sButInit.x - 15);
		sLabInit.y = sButInit.y;
		sLabInit.width = 12;
		sLabInit.height = 15;
		sLabInit.FontID = font_regular;
		sLabInit.pUserData = psOwner;
		sLabInit.pCallback = intAddLoopQuantity;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return FALSE;
		}


		/* store the common values for the text labels for the quantity
		to produce (on each button).*/
		memset(&sLabInit,0,sizeof(W_LABINIT));
		sLabInit.id = IDSTAT_PRODSTART;
		sLabInit.style = WLAB_PLAIN | WIDG_HIDDEN;

		sLabInit.x = STAT_BUTWIDTH-12;
		sLabInit.y = 2;

		sLabInit.width = 12;
		sLabInit.height = 15;
		sLabInit.FontID = font_regular;
		sLabInit.pCallback = intAddProdQuantity;

	}
#endif




	/* Add the close button */
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = IDSTAT_FORM;
	sButInit.id = IDSTAT_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = STAT_WIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}
	/* Calculate how many buttons will go on a form */
	butPerForm = ((STAT_WIDTH - STAT_GAP) /
						(STAT_BUTWIDTH + STAT_GAP)) *
				 ((STAT_HEIGHT - STAT_GAP) /
						(STAT_BUTHEIGHT + STAT_GAP));
//================== adds L/R Scroll buttons ===================================
if (numForms(numStats, butPerForm)> MAX_TAB_SMALL_SHOWN)	//only want these buttons when tab count >8
	{
		// Add the left tab scroll button
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_TABSCRL_LEFT;
		sButInit.style = WBUT_PLAIN;
		sButInit.x = STAT_TABFORMX + 4;
		sButInit.y = STAT_TABFORMY;
		sButInit.width = TABSCRL_WIDTH;
		sButInit.height = TABSCRL_HEIGHT;
		sButInit.pTip = _("Tab Scroll left");
		sButInit.FontID = font_regular;
		sButInit.pDisplay = intDisplayImageHilight;
		sButInit.UserData = PACKDWORD_TRI(0, IMAGE_LFTTABD, IMAGE_LFTTAB);
		if (!widgAddButton(psWScreen, &sButInit))
		{
		return FALSE;
		}
		// Add the right tab scroll button
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = IDSTAT_FORM;
		sButInit.id = IDSTAT_TABSCRL_RIGHT;
		sButInit.style = WBUT_PLAIN;
		sButInit.x = STAT_WIDTH - 14;
		sButInit.y = STAT_TABFORMY;
		sButInit.width = TABSCRL_WIDTH;
		sButInit.height = TABSCRL_HEIGHT;
		sButInit.pTip = _("Tab Scroll right");
		sButInit.FontID = font_regular;
		sButInit.pDisplay = intDisplayImageHilight;
		sButInit.UserData = PACKDWORD_TRI(0, IMAGE_RGTTABD, IMAGE_RGTTAB);
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return FALSE;
		}
	}
//==============buttons before tabbed form!==========================
	// Add the tabbed form
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
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
	sFormInit.tabVertOffset = (OBJ_TABHEIGHT/2);
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;
	sFormInit.tabMajorGap = OBJ_TABOFFSET;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;
    //Build menu can have up to 80 stats - so can research now 13/09/99 AB
	// NOTE, there is really no limit now to the # of menu items we can have,
	// It is #defined in hci.h to be 200 now. [#define	MAXSTRUCTURES	200]
	//Same goes for research. [#define	MAXRESEARCH		200]
//	if (( (objMode == IOBJ_MANUFACTURE) || (objMode == IOBJ_BUILD) || (objMode == IOBJ_RESEARCH)) &&
	if (sFormInit.numMajor > MAX_TAB_STD_SHOWN)
	{	//Just switching from normal sized tabs to smaller ones to fit more in form.
		sFormInit.pUserData = &SmallTab;
		sFormInit.majorSize /= 2;
		if (sFormInit.numMajor > MAX_TAB_SMALL_SHOWN)		// 7 tabs is all we can fit with current form size.
		{	// make room for new tab item (tab scroll buttons)
			sFormInit.majorOffset = OBJ_TABOFFSET + 10;
			sFormInit.TabMultiplier = 1;		// Enable our tabMultiplier buttons.
		}
	}
	for (i = 0; i< sFormInit.numMajor; i++)	// Sets # of tab's minors
	{
		sFormInit.aNumMinors[i] = 1;
	}
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	/* Add the stat buttons */
	memset(&sBFormInit, 0, sizeof(W_FORMINIT));
	sBFormInit.formID = IDSTAT_TABFORM;
	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.id = IDSTAT_START;
	sBFormInit.style = WFORM_CLICKABLE | WFORM_SECONDARY;
	sBFormInit.x = STAT_BUTX;
	sBFormInit.y = STAT_BUTY;
	sBFormInit.width = STAT_BUTWIDTH;
	sBFormInit.height = STAT_BUTHEIGHT;

	memset(&sBarInit, 0, sizeof(W_BARINIT));
	sBarInit.id = IDSTAT_TIMEBARSTART;
	sBarInit.style = WBAR_PLAIN;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.y = STAT_TIMEBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol.byte.r = STAT_PROGBARMAJORRED;
	sBarInit.sCol.byte.g = STAT_PROGBARMAJORGREEN;
	sBarInit.sCol.byte.b = STAT_PROGBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = STAT_PROGBARMINORRED;
	sBarInit.sMinorCol.byte.g = STAT_PROGBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = STAT_PROGBARMINORBLUE;
	//sBarInit.pTip = _("Power Usage");

	statID = 0;
	statForm = 0;
	for (i=0; i<numStats; i++)
	{
		if (sBFormInit.id > IDSTAT_END)
		{
			//can't fit any more on the screen!
			debug( LOG_NEVER, "This is just a Warning!\n Max buttons have been allocated" );
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
		ASSERT( BufferID < NUM_STATBUFFERS,"BufferID > NUM_STATBUFFERS" );

		RENDERBUTTON_INUSE(&StatBuffers[BufferID]);
		StatBuffers[BufferID].Data = (void*)ppsStatsList[i];
		sBFormInit.pUserData = &StatBuffers[BufferID];
		sBFormInit.pDisplay = intDisplayStatsButton;


		if (!widgAddForm(psWScreen, &sBFormInit))
		{
			return FALSE;
		}
		widgSetColour(psWScreen, sBFormInit.id, WCOL_BKGRND, 0,0,0);

		//Stat = ppsStatsList[i];
		if (Stat->ref >= REF_STRUCTURE_START &&
			Stat->ref < REF_STRUCTURE_START + REF_RANGE) {		// It's a structure.

			//sBarInit.pTip = _("Build Speed");
			//sBarInit.size = (UWORD)(((STRUCTURE_STATS*)Stat)->buildPoints / BUILDPOINTS_STRUCTDIV);
			sBarInit.size = (UWORD)(((STRUCTURE_STATS*)Stat)->powerToBuild /
				POWERPOINTS_DROIDDIV);
			if(sBarInit.size > 100) sBarInit.size = 100;

			sBarInit.formID = sBFormInit.id;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return FALSE;
			}

		} else if (Stat->ref >= REF_TEMPLATE_START &&
			Stat->ref < REF_TEMPLATE_START + REF_RANGE) {	// It's a droid.

			//sBarInit.size = (UWORD)(((DROID_TEMPLATE*)Stat)->buildPoints  / BUILDPOINTS_DROIDDIV);
			sBarInit.size = (UWORD)(((DROID_TEMPLATE*)Stat)->powerPoints /
				POWERPOINTS_DROIDDIV);
			//sBarInit.pTip = _("Power Usage");
			if(sBarInit.size > 100) sBarInit.size = 100;

			sBarInit.formID = sBFormInit.id;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return FALSE;
			}

			// Add a text label for the quantity to produce.
			sLabInit.formID = sBFormInit.id;
			sLabInit.pUserData = Stat;
			if (!widgAddLabel(psWScreen, &sLabInit))
			{
				return FALSE;
			}
			sLabInit.id++;
		}

		else if(Stat->ref >= REF_RESEARCH_START &&
				Stat->ref < REF_RESEARCH_START + REF_RANGE)				// It's a Research topic.
		{
            //new icon in for groups - AB 12/01/99
			memset(&sLabInit,0,sizeof(W_LABINIT));
			sLabInit.formID = sBFormInit.id ;
			sLabInit.id = IDSTAT_RESICONSTART+(sBFormInit.id - IDSTAT_START);
			sLabInit.style = WLAB_PLAIN;

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
			//sBarInit.pTip = _("Power Usage");
			if(sBarInit.size > 100) sBarInit.size = 100;


			// if multiplayer, if research topic is being done by another ally then mark as such..
			if(bMultiPlayer)
			{
				STRUCTURE *psOtherStruct;
				UBYTE	ii;
				for(ii=0;ii<MAX_PLAYERS;ii++)
				{
					if(ii != selectedPlayer && aiCheckAlliances(selectedPlayer,ii))
					{
						//check each research facility to see if they are doing this topic.
						for(psOtherStruct=apsStructLists[ii];psOtherStruct;psOtherStruct=psOtherStruct->psNext)
						{
							if(   psOtherStruct->pStructureType->type == REF_RESEARCH
								 && psOtherStruct->status == SS_BUILT
								 && ((RESEARCH_FACILITY *)psOtherStruct->pFunctionality)->psSubject
								 && ((RESEARCH_FACILITY *)psOtherStruct->pFunctionality)->psSubject->ref == Stat->ref
							  )
							{
								// add a label.
								memset(&sLabInit,0,sizeof(W_LABINIT));
								sLabInit.formID = sBFormInit.id ;
								sLabInit.id = IDSTAT_ALLYSTART+(sBFormInit.id - IDSTAT_START);
								sLabInit.style = WLAB_PLAIN;
								sLabInit.x = STAT_BUTWIDTH  - 19;
								sLabInit.y = STAT_BUTHEIGHT - 19;
								sLabInit.width = 12;
								sLabInit.height = 15;
								sLabInit.UserData = ii;
								sLabInit.pTip = getPlayerName(ii);
								sLabInit.pDisplay = intDisplayAllyIcon;
								widgAddLabel(psWScreen, &sLabInit);

								goto donelab;
							}
						}

					}
				}
			}
donelab:
			sBarInit.formID = sBFormInit.id;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return FALSE;
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
		if (sBFormInit.x + STAT_BUTWIDTH+STAT_GAP > STAT_WIDTH)	// - STAT_TABWIDTH)
		{
			sBFormInit.x = STAT_BUTX;	//STAT_GAP;
			sBFormInit.y += STAT_BUTHEIGHT + STAT_GAP;
		}
		if (sBFormInit.y + STAT_BUTHEIGHT+STAT_GAP > STAT_HEIGHT) // - STAT_TITLEHEIGHT)
		{
			sBFormInit.y = STAT_BUTY;	//STAT_GAP;
			sBFormInit.majorID += 1;
		}

		sBarInit.id += 1;
	}

//	widgStartScreen(psWScreen);

	/* Set the correct page and button if necessary */
	if (statID)
	{
		widgSetTabs(psWScreen, IDSTAT_TABFORM, (UWORD)statForm, 0);
		widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);
	}



	StatsUp = TRUE;

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

	return TRUE;
}


/* Select a command droid */
static BOOL selectCommand(BASE_OBJECT *psObj)
{
//	UDWORD	i;
	DROID	*psDroid;

	ASSERT( psObj != NULL && psObj->type == OBJ_DROID,
		"selectConstruction: invalid droid pointer" );
	psDroid = (DROID *)psObj;

	//check the droid type
	if ( (psDroid->droidType == DROID_COMMAND) && (psDroid->died == 0) )
	{
		return TRUE;
	}

	/*for (i=0; i < psDroid->numProgs; i++)
	{
		if (psDroid->asProgs[i].psStats->order == ORDER_BUILD)
		{
			return TRUE;
		}
	}*/
	return FALSE;
}

/* Return the stats for a command droid */
static BASE_STATS *getCommandStats(BASE_OBJECT *psObj)
{
	return NULL;
}

/* Set the stats for a command droid */
static BOOL setCommandStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	return TRUE;
}

/* Select a construction droid */
static BOOL selectConstruction(BASE_OBJECT *psObj)
{
//	UDWORD	i;
	DROID	*psDroid;

	ASSERT( psObj != NULL && psObj->type == OBJ_DROID,
		"selectConstruction: invalid droid pointer" );
	psDroid = (DROID *)psObj;

	//check the droid type
	//if ( (psDroid->droidType == DROID_CONSTRUCT) && (psDroid->died == 0) )
    if ( (psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType ==
        DROID_CYBORG_CONSTRUCT) && (psDroid->died == 0) )
	{
		return TRUE;
	}

	/*for (i=0; i < psDroid->numProgs; i++)
	{
		if (psDroid->asProgs[i].psStats->order == ORDER_BUILD)
		{
			return TRUE;
		}
	}*/
	return FALSE;
}

/* Return the stats for a construction droid */
static BASE_STATS *getConstructionStats(BASE_OBJECT *psObj)
{
	DROID *psDroid = (DROID *)psObj;
	BASE_STATS *Stats;
	BASE_OBJECT *Structure;
	UDWORD x, y;

	ASSERT( psObj != NULL && psObj->type == OBJ_DROID,
		"getConstructionStats: invalid droid pointer" );

	if (!(droidType(psDroid) == DROID_CONSTRUCT ||
		droidType(psDroid) == DROID_CYBORG_CONSTRUCT))
	{
		return NULL;
	}

	if(orderStateStatsLoc(psDroid, DORDER_BUILD, &Stats, &x, &y)) // Moving to build location?
	{
		return Stats;
	}
	else if ((Structure = orderStateObj(psDroid, DORDER_BUILD))
	      && psDroid->order == DORDER_BUILD) // Is building
	{
		return psDroid->psTarStats;
	}
	else if ((Structure = orderStateObj(psDroid, DORDER_HELPBUILD))
	 && (psDroid->order == DORDER_HELPBUILD
	  || psDroid->order == DORDER_LINEBUILD)) // Is helping
	{
		return (BASE_STATS*)((STRUCTURE*)Structure)->pStructureType;
	}
	else if (orderState(psDroid, DORDER_DEMOLISH))
	{
		return (BASE_STATS *)structGetDemolishStat();
	}

	return NULL;
}

/* Set the stats for a construction droid */
static BOOL setConstructionStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	STRUCTURE_STATS		*psSStats;
	DROID				*psDroid;

	ASSERT( psObj != NULL && psObj->type == OBJ_DROID,
		"setConstructionStats: invalid droid pointer" );
	/* psStats might be NULL if the operation is canceled in the middle */

	if (psStats != NULL)
	{
		psSStats = (STRUCTURE_STATS *)psStats;
		psDroid = (DROID *)psObj;

		//check for demolish first
		if (psSStats == structGetDemolishStat())
		{
			objMode = IOBJ_DEMOLISHSEL;

			// When demolish requested, need to select a construction droid, not really any
			// choice in this as demolishing uses the droid targeting interface rather than
			// the build positioning interface and therefore requires a construction droid
			// to be selected.
			clearSel();
			SelectDroid(psDroid);
			if(driveModeActive()) {
				driveSelectionChanged();
			}
			return TRUE;
		}

		/* Store the stats for future use */
		psPositionStats = psStats;

		/* Now start looking for a location for the structure */
		if (psSStats)
		{
			{
				objMode = IOBJ_BUILDSEL;

				intStartStructPosition(psStats,psDroid);

				//set the droids current program
				/*for (i=0; i < psDroid->numProgs; i++)
				{
					if (psDroid->asProgs[i].psStats->order == ORDER_BUILD)
					{
						psDroid->activeProg = i;
					}
				}*/
			}
		}
		else
		{
			orderDroid(psDroid,DORDER_STOP);
		}
	}
	else
	{
		psDroid = (DROID *)psObj;
		orderDroid(psDroid,DORDER_STOP);
	}
	return TRUE;
}

/* Select a research facility */
static BOOL selectResearch(BASE_OBJECT *psObj)
{
	STRUCTURE	*psResFacility;

	ASSERT( psObj != NULL && psObj->type == OBJ_STRUCTURE,
		"selectResearch: invalid Structure pointer" );

	psResFacility = (STRUCTURE *)psObj;

	/* A Structure is a research facility if its type = REF_RESEARCH and is
	   completely built*/
	if (psResFacility->pStructureType->type == REF_RESEARCH && (psResFacility->
		status == SS_BUILT) && (psResFacility->died == 0))
	{
		return TRUE;
	}
	return FALSE;
}

/* Return the stats for a research facility */
static BASE_STATS *getResearchStats(BASE_OBJECT *psObj)
{
	STRUCTURE	*psBuilding;

	ASSERT( psObj != NULL && psObj->type == OBJ_STRUCTURE,
		"getResearchTip: invalid Structure pointer" );
	psBuilding = (STRUCTURE *)psObj;

	return (BASE_STATS*)(((RESEARCH_FACILITY*)psBuilding->pFunctionality)->
		psSubject);
}

/* Set the stats for a research facility */
static BOOL setResearchStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	STRUCTURE			*psBuilding;
	RESEARCH			*pResearch;
	PLAYER_RESEARCH		*pPlayerRes;
	UDWORD				count;
	RESEARCH_FACILITY	*psResFacilty;

	ASSERT( psObj != NULL && psObj->type == OBJ_STRUCTURE,
		"setResearchStats: invalid Structure pointer" );
	/* psStats might be NULL if the operation is canceled in the middle */
	psBuilding = (STRUCTURE *)psObj;

	psResFacilty = (RESEARCH_FACILITY*)psBuilding->pFunctionality;
	//initialise the subject
	psResFacilty->psSubject = NULL;

	//set up the player_research
	if (psStats != NULL)
	{
		pResearch = (RESEARCH*) psStats;

		count = pResearch->ref - REF_RESEARCH_START;
		//meant to still be in the list but greyed out
		pPlayerRes = asPlayerResList[selectedPlayer] + count;

		/*subtract the power required to research*/
		/*if (pPlayerRes->researched != CANCELLED_RESEARCH)
		{
			if (!usePower(selectedPlayer, pResearch->researchPower))
			{
				addConsoleMessage("Research: No Power",DEFAULT_JUSTIFY);
				return FALSE;
			}
		}*/

		//set the subject up
		psResFacilty->psSubject = psStats;

		if (IsResearchCancelled(pPlayerRes))
		{
			//set up as if all power available for cancelled topics
			psResFacilty->powerAccrued = pResearch->researchPower;
		}
		else
		{
			psResFacilty->powerAccrued = 0;
		}

		sendReseachStatus(psBuilding,count,selectedPlayer,TRUE);	// inform others, I'm researching this.

		MakeResearchStarted(pPlayerRes);
		//psResFacilty->timeStarted = gameTime;
		psResFacilty->timeStarted = ACTION_START_TIME;
        psResFacilty->timeStartHold = 0;
        //this is no longer used...AB 30/06/99
		psResFacilty->timeToResearch = pResearch->researchPoints /
			psResFacilty->researchPoints;
		//check for zero research time - usually caused by 'silly' data!
		if (psResFacilty->timeToResearch == 0)
		{
			//set to 1/1000th sec - ie very fast!
			psResFacilty->timeToResearch = 1;
		}
		//stop the button from flashing once a topic has been chosen
		stopReticuleButtonFlash(IDRET_RESEARCH);
	}
	return TRUE;
}

/* Select a Factory */
static BOOL selectManufacture(BASE_OBJECT *psObj)
{
	STRUCTURE		*psBuilding;

	ASSERT( psObj != NULL && psObj->type == OBJ_STRUCTURE,
		"selectManufacture: invalid Structure pointer" );
	psBuilding = (STRUCTURE *)psObj;

	/* A Structure is a Factory if its type = REF_FACTORY or REF_CYBORG_FACTORY or
	REF_VTOL_FACTORY and it is completely built*/
	if ((psBuilding->pStructureType->type == REF_FACTORY ||
		  psBuilding->pStructureType->type == REF_CYBORG_FACTORY ||
		  psBuilding->pStructureType->type == REF_VTOL_FACTORY) &&
		  (psBuilding->status == SS_BUILT) && (psBuilding->died == 0))
	{
		return TRUE;
	}

	return FALSE;
}

/* Return the stats for a Factory */
static BASE_STATS *getManufactureStats(BASE_OBJECT *psObj)
{
	STRUCTURE	*psBuilding;

	ASSERT( psObj != NULL && psObj->type == OBJ_STRUCTURE,
		"getManufactureTip: invalid Structure pointer" );
	psBuilding = (STRUCTURE *)psObj;

	return ((BASE_STATS*)((FACTORY*)psBuilding->pFunctionality)->psSubject);
}


/* Set the stats for a Factory */
static BOOL setManufactureStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	STRUCTURE		*Structure;

	ASSERT( psObj != NULL && psObj->type == OBJ_STRUCTURE,
		"setManufactureStats: invalid Structure pointer" );
	/* psStats might be NULL if the operation is canceled in the middle */

#ifdef INCLUDE_FACTORYLISTS
	Structure = (STRUCTURE*)psObj;
	//check to see if the factory was already building something
	if (!((FACTORY *)Structure->pFunctionality)->psSubject)
	{
		//factory not currently building so set up the factory stats
		if (psStats != NULL)
		{
			/* Set the factory to build droid(s) */
			if (!structSetManufacture(Structure, (DROID_TEMPLATE *)psStats, 1))
			{
				return FALSE;
			}
		}
	}
#endif

#ifdef INCLUDE_PRODSLIDER
	if (ProductionRun == 0)
	{
		//check if its because there isn't enough power - warning if it is
		if (psStats)
		{
			(void)checkPower(selectedPlayer, ((DROID_TEMPLATE *)psStats)->
				powerPoints, TRUE);
		}
		return FALSE;
	}

	Structure = (STRUCTURE*)psObj;
	if (psStats != NULL)
	{
		//temp code to set non stop production up
		if (ProductionRun == STAT_SLDSTOPS)
		{
			ProductionRun = NON_STOP_PRODUCTION;
		}
		/* check power if factory not on infinte production*/
		if (ProductionRun != NON_STOP_PRODUCTION)
		{
			if (!checkPower(selectedPlayer, ((DROID_TEMPLATE *)psStats)->powerPoints, TRUE))
			{
				return FALSE;
			}
		}
		/* Set the factory to build droid(s) */
		if (!structSetManufacture(Structure, (DROID_TEMPLATE *)psStats, ProductionRun))
		{
			return FALSE;
		}
		/*set the slider for this production */
		/*if (Quantity == 0)
		{
			widgSetSliderPos(psWScreen, IDSTAT_SLIDER, 0);
		}
		else
		{*/
//#ifdef INCLUDE_PRODSLIDER
			widgSetSliderPos(psWScreen, IDSTAT_SLIDER, (UWORD)(ProductionRun-1));
//#endif
		//}


	} else {
		// Stop manufacturing.
		//return half the power cost if cancelled mid production
		if (((FACTORY*)Structure->pFunctionality)->timeStarted != ACTION_START_TIME)
		{
			if (((FACTORY*)Structure->pFunctionality)->psSubject != NULL)
			{
				addPower(Structure->player, ((DROID_TEMPLATE *)((FACTORY*)Structure->
					pFunctionality)->psSubject)->powerPoints / 2);
			}
		}
		((FACTORY*)Structure->pFunctionality)->quantity = 0;
		((FACTORY*)Structure->pFunctionality)->psSubject = NULL;
		intManufactureFinished(Structure);
	}
#endif

	return TRUE;
}


/* Add the build widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static BOOL intAddBuild(DROID *psSelected)
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
							   (BASE_OBJECT *)psSelected,TRUE);
}


/* Add the manufacture widgets to the widget screen */
/* If psSelected != NULL it specifies which factory should be hilited */
static BOOL intAddManufacture(STRUCTURE *psSelected)
{
	/* Store the correct stats list for future reference */
	ppsStatsList = (BASE_STATS**)apsTemplateList;

	objSelectFunc = selectManufacture;
	objGetStatsFunc = getManufactureStats;
	objSetStatsFunc = setManufactureStats;

	/* Set the sub mode */
	objMode = IOBJ_MANUFACTURE;

	/* Create the object screen with the required data */
	//return intAddObject((BASE_OBJECT *)apsStructLists[selectedPlayer],

	return intAddObjectWindow((BASE_OBJECT *)interfaceStructList(),
							   (BASE_OBJECT *)psSelected,TRUE);
}


/* Add the research widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static BOOL intAddResearch(STRUCTURE *psSelected)
{
	ppsStatsList = (BASE_STATS **)ppResearchList;

	objSelectFunc = selectResearch;
	objGetStatsFunc = getResearchStats;
	objSetStatsFunc = setResearchStats;

	/* Set the sub mode */
	objMode = IOBJ_RESEARCH;

	/* Create the object screen with the required data */
	//return intAddObject((BASE_OBJECT *)apsStructLists[selectedPlayer],

	return intAddObjectWindow((BASE_OBJECT *)interfaceStructList(),
							   (BASE_OBJECT *)psSelected,TRUE);
}


/* Add the command droid widgets to the widget screen */
/* If psSelected != NULL it specifies which droid should be hilited */
static BOOL intAddCommand(DROID *psSelected)
{
	ppsStatsList = NULL;//(BASE_STATS **)ppResearchList;

	objSelectFunc = selectCommand;
	objGetStatsFunc = getCommandStats;
	objSetStatsFunc = setCommandStats;

	/* Set the sub mode */
	objMode = IOBJ_COMMAND;

	/* Create the object screen with the required data */
	//return intAddObject((BASE_OBJECT *)apsStructLists[selectedPlayer],

	return intAddObjectWindow((BASE_OBJECT *)apsDroidLists[selectedPlayer],
							   (BASE_OBJECT *)psSelected,TRUE);

}


/*Deals with the RMB click for the stats screen */
static void intStatsRMBPressed(UDWORD id)
{
	DROID_TEMPLATE		*psStats;
#ifdef INCLUDE_FACTORYLISTS
	DROID_TEMPLATE		*psNext;
#endif

	ASSERT( id - IDSTAT_START < numStatsListEntries,
			"intStatsRMBPressed: Invalid structure stats id" );

	if (objMode == IOBJ_MANUFACTURE)
	{
		psStats = (DROID_TEMPLATE *)ppsStatsList[id - IDSTAT_START];

		//this now causes the production run to be decreased by one
#ifdef INCLUDE_FACTORYLISTS

		ASSERT( psObjSelected != NULL,
			"intStatsRMBPressed: Invalid structure pointer" );
		ASSERT( psStats != NULL,
			"intStatsRMBPressed: Invalid template pointer" );
        if (productionPlayer == (SBYTE)selectedPlayer)
        {
            FACTORY  *psFactory = (FACTORY *)((STRUCTURE *)psObjSelected)->
                pFunctionality;

		    //decrease the production
		    factoryProdAdjust((STRUCTURE *)psObjSelected, psStats, FALSE);
            //need to check if this was the template that was mid-production
            if (psStats == (DROID_TEMPLATE *)psFactory->psSubject)
            {
    		    //if have decreased to zero then cancel the production
	    	    if (getProductionQuantity((STRUCTURE *)psObjSelected, psStats) == 0)
		        {
			        //init the factory production
			        psFactory->psSubject = NULL;
			        //check to see if anything left to produce
			        psNext = factoryProdUpdate((STRUCTURE *)psObjSelected, NULL);
			        if (psNext == NULL)
			        {
				        intManufactureFinished((STRUCTURE *)psObjSelected);
			        }
			        else
			        {
				        if (!objSetStatsFunc(psObjSelected, (BASE_STATS *)psNext))
				        {
					        intSetStats(objStatID, NULL);
				        }
				        else
				        {
					        // Reset the button on the object form
					        intSetStats(objStatID, (BASE_STATS *)psStats);
				        }
			        }
		        }
            }
		    else
		    {
			    //if factory wasn't currently on line then set the object button
			    if (!psFactory->psSubject)
			    {
				    if (!objSetStatsFunc(psObjSelected, (BASE_STATS *)psStats))
				    {
					    intSetStats(objStatID, NULL);
				    }
				    else
				    {
					    // Reset the button on the object form
					    intSetStats(objStatID, (BASE_STATS *)psStats);
				    }
			    }
		    }
        }
#else
		// set the current Template
		psCurrTemplate = apsDroidTemplates[selectedPlayer];
		while (psStats != psCurrTemplate)
		{
			//if get to the last template in the list, use this one
			if (psCurrTemplate->psNext == NULL)
			{
				break;
			}
			psCurrTemplate = psCurrTemplate->psNext;
		}

		//close the stats screen
		intResetScreen(TRUE);
		// open up the design screen
		widgSetButtonState(psWScreen, IDRET_DESIGN, WBUT_CLICKLOCK);

		/*add the power bar - for looks! */
		intShowPowerBar();
		(void)intAddDesign( TRUE );
		intMode = INT_DESIGN;
#endif
	}
}

/*Deals with the RMB click for the Object screen */
static void intObjectRMBPressed(UDWORD id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStructure;

	ASSERT( (SDWORD)id - IDOBJ_OBJSTART < numObjects,
			"intObjectRMBPressed: Invalid object id" );

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
				           TRUE);
			}
		}
	}
}

/*Deals with the RMB click for the Object Stats buttons */
static void intObjStatRMBPressed(UDWORD id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStructure;

	ASSERT( (SDWORD)id - IDOBJ_STATSTART < numObjects,
			"intObjStatRMBPressed: Invalid stat id" );

	/* Find the object that the ID refers to */
	psObj = intGetObject(id);
	if (psObj)
	{
		intResetWindows(psObj);
		if (psObj->type == OBJ_STRUCTURE)
		{
			psStructure = (STRUCTURE *)psObj;
			if (StructIsFactory(psStructure))
			{
				//check if active
				if (((FACTORY *)psStructure->pFunctionality)->psSubject)
				{
					//if not curently on hold, set it
					if (((FACTORY *)psStructure->pFunctionality)->timeStartHold == 0)
					{
						holdProduction(psStructure);
					}
					else
					{
						//cancel if have RMB-clicked twice
						cancelProduction(psStructure);
                		//play audio to indicate cancelled
		                audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
					}
				}
			}
            else if (psStructure->pStructureType->type == REF_RESEARCH)
            {
				//check if active
				if (((RESEARCH_FACILITY *)psStructure->pFunctionality)->psSubject)
				{
					//if not curently on hold, set it
					if (((RESEARCH_FACILITY *)psStructure->pFunctionality)->timeStartHold == 0)
					{
						holdResearch(psStructure);
					}
					else
					{
						//cancel if have RMB-clicked twice
                        cancelResearch(psStructure);
                		//play audio to indicate cancelled
		                audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
					}
				}
            }
		}
	}
}


//sets up the Intelligence Screen as far as the interface is concerned
void addIntelScreen(void)
{
	BOOL	radOnScreen;

	if(driveModeActive() && !driveInterfaceEnabled()) {
		driveDisableControl();
		driveEnableInterface(TRUE);
	}

	intResetScreen(FALSE);

	//lock the reticule button
	widgSetButtonState(psWScreen, IDRET_INTEL_MAP, WBUT_CLICKLOCK);

	//add the power bar - for looks!
	intShowPowerBar();

	// Only do this in main game.
	if((GetGameMode() == GS_NORMAL) && !bMultiPlayer)
	{
		radOnScreen = radarOnScreen;

		bRender3DOnly = TRUE;
		radarOnScreen = FALSE;

	// Just display the 3d, no interface
		displayWorld();
	// Upload the current display back buffer into system memory.
		pie_UploadDisplayBuffer();

		radarOnScreen = radOnScreen;
		bRender3DOnly = FALSE;
	}

	//add all the intelligence screen interface
	(void)intAddIntelMap();
	intMode = INT_INTELMAP;

	/*if (psCurrentMsg && psCurrentMsg->type == MSG_TUTORIAL)
	{
		//just display the message
		if (psCurrentMsg->pViewData)
		{
			intAddMessageView(psCurrentMsg->type);
			if (psCurrentMsg->pViewData->audioID != NO_AUDIO_MSG)
			{
				audio_PlayTrack(psCurrentMsg->pViewData->audioID);
			}
			intMode = INT_TUTORIAL;
		}
	}
	else
	{
		widgSetButtonState(psWScreen, IDRET_INTEL_MAP, WBUT_CLICKLOCK);
		//add the power bar - for looks!
		(void)intAddPower();
		(void)intAddIntelMap(playImmediate);
		intMode = INT_INTELMAP;
	}*/
}

//sets up the Transporter Screen as far as the interface is concerned
void addTransporterInterface(DROID *psSelected, BOOL onMission)
{
    //if psSelected = NULL add interface but if psSelected != NULL make sure its not flying
    if (!psSelected || (psSelected && !transporterFlying(psSelected)))
    {
    	intResetScreen(FALSE);
	    intAddTransporter(psSelected, onMission);
	    intMode = INT_TRANSPORTER;
    }
}

/*sets which list of structures to use for the interface*/
STRUCTURE* interfaceStructList(void)
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
	psButton = (W_TABFORM*)widgGetFromID(psWScreen,buttonID);
	if (psButton)
	{
		//set flashing byte to true
		psButton->UserData = (1 << 24) | psButton->UserData;
	}

}

// stop a reticule button flashing
void stopReticuleButtonFlash(UDWORD buttonID)
{

	WIDGET	*psButton = widgGetFromID(psWScreen,buttonID);
	if (psButton)
	{
		UBYTE DownTime = UNPACKDWORD_QUAD_C(psButton->UserData);
		UBYTE Index = UNPACKDWORD_QUAD_D(psButton->UserData);
		UBYTE flashing = UNPACKDWORD_QUAD_A(psButton->UserData);
		UBYTE flashTime = UNPACKDWORD_QUAD_B(psButton->UserData);

		// clear flashing byte
		flashing = FALSE;
		flashTime = 0;
		psButton->UserData = PACKDWORD_QUAD(flashTime,flashing,DownTime,Index);
	}

}

//displays the Power Bar
void intShowPowerBar(void)
{
	//if its not already on display
	if (widgGetFromID(psWScreen,IDPOW_POWERBAR_T))
	{
		widgReveal(psWScreen, IDPOW_POWERBAR_T);
	}
}

//hides the power bar from the display - regardless of what player requested!
void forceHidePowerBar(void)
{
	if (widgGetFromID(psWScreen,IDPOW_POWERBAR_T))
	{
		widgHide(psWScreen, IDPOW_POWERBAR_T);
	}
}


/* Add the Proximity message buttons */
BOOL intAddProximityButton(PROXIMITY_DISPLAY *psProxDisp, UDWORD inc)
{
	W_FORMINIT			sBFormInit;
	PROXIMITY_DISPLAY	*psProxDisp2;
	UDWORD				cnt;

	memset(&sBFormInit, 0, sizeof(W_FORMINIT));
	sBFormInit.formID = 0;
	sBFormInit.id = IDPROX_START + inc;
	//store the ID so we can detect which one has been clicked on
	psProxDisp->buttonID = sBFormInit.id;

//	loop back and find a free one!
//	ASSERT( sBFormInit.id < IDPROX_END,"Too many proximity message buttons" );
	if(sBFormInit.id >= IDPROX_END)
	{
		for(cnt = IDPROX_START;cnt<IDPROX_END;cnt++)
		{							// go down the prox msgs and see if it's free.
			for(psProxDisp2 = apsProxDisp[selectedPlayer];
				psProxDisp2 &&(psProxDisp2->buttonID!=cnt) ;
				psProxDisp2 = psProxDisp2->psNext);

			if(psProxDisp == NULL)	// value was unused.
			{
				sBFormInit.id = cnt;
				break;
			}
		}
		if(cnt == IDPROX_END)
		{
			return FALSE;			// no slot was found.
		}
	}

	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	//sBFormInit.width = iV_GetImageWidth(IntImages,IMAGE_GAM_ENMREAD);
	//sBFormInit.height = iV_GetImageHeight(IntImages,IMAGE_GAM_ENMREAD);
	sBFormInit.width = PROX_BUTWIDTH;
	sBFormInit.height = PROX_BUTHEIGHT;
	//the x and y need to be set up each time the button is drawn - see intDisplayProximityBlips

	sBFormInit.pDisplay = intDisplayProximityBlips;
	//set the data for this button
	sBFormInit.pUserData = psProxDisp;

	if (!widgAddForm(psWScreen, &sBFormInit))
	{
		return FALSE;
	}
	return TRUE;
}


/*Remove a Proximity Button - when the message is deleted*/
void intRemoveProximityButton(PROXIMITY_DISPLAY *psProxDisp)
{
	widgDelete(psWScreen, psProxDisp->buttonID);
}

/*deals with the proximity message when clicked on*/
void processProximityButtons(UDWORD id)
{
	PROXIMITY_DISPLAY	*psProxDisp;


	if(!doWeDrawProximitys())
	{
		return;
	}

	//find which proximity display this relates to
	psProxDisp = NULL;
	for(psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp; psProxDisp =
		psProxDisp->psNext)
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
void	setKeyButtonMapping( UDWORD	val )
{
	keyButtonMapping = val;
}


/*Looks through the players list of structures to see if there is one selected
of the required type. If there is more than one, they are all deselected and
the first one reselected*/
STRUCTURE* intCheckForStructure(UDWORD structType)
{
	STRUCTURE	*psStruct, *psSel = NULL;

	for (psStruct = interfaceStructList(); psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->selected && psStruct->pStructureType->type ==
			structType && psStruct->status == SS_BUILT)
		{
			if (psSel != NULL)
			{
				clearSelection();
				psSel->selected = TRUE;
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
DROID* intCheckForDroid(UDWORD droidType)
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
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected && psDroid->droidType == droidType)
		{
			num += 1;
		}
	}

	return num;
}

static void intInitialiseReticule(void)
{
	int i;

	for (i=0; i<NUMRETBUTS; i++) {
		ReticuleEnabled[i].Hidden = FALSE;
	}
}


// Check that each reticule button has the structure or droid required for it and
// enable/disable accordingly.
//
void intCheckReticuleButtons(void)
{
	STRUCTURE	*psStruct;
	DROID	*psDroid;
	int i;

	ReticuleEnabled[RETBUT_CANCEL].Enabled = TRUE;
	ReticuleEnabled[RETBUT_FACTORY].Enabled = FALSE;
	ReticuleEnabled[RETBUT_RESEARCH].Enabled = FALSE;
	ReticuleEnabled[RETBUT_BUILD].Enabled = FALSE;
	ReticuleEnabled[RETBUT_DESIGN].Enabled = FALSE;
	ReticuleEnabled[RETBUT_INTELMAP].Enabled = TRUE;
	ReticuleEnabled[RETBUT_COMMAND].Enabled = FALSE;


	for (psStruct = interfaceStructList(); psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if(psStruct->status == SS_BUILT) {
			switch(psStruct->pStructureType->type) {
				case REF_RESEARCH:
                    if (!missionLimboExpand())
                    {
					    ReticuleEnabled[RETBUT_RESEARCH].Enabled = TRUE;
                    }
					break;
				case REF_FACTORY:
				case REF_CYBORG_FACTORY:
				case REF_VTOL_FACTORY:
                    if (!missionLimboExpand())
                    {
					    ReticuleEnabled[RETBUT_FACTORY].Enabled = TRUE;
                    }
					break;
				case REF_HQ:
					ReticuleEnabled[RETBUT_DESIGN].Enabled = TRUE;
					break;
				default:
					break;
			}
		}
	}

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid =
		psDroid->psNext)
	{
		switch(psDroid->droidType) {
		case DROID_CONSTRUCT:
		case DROID_CYBORG_CONSTRUCT:
			ReticuleEnabled[RETBUT_BUILD].Enabled = TRUE;
			break;
		case DROID_COMMAND:
			ReticuleEnabled[RETBUT_COMMAND].Enabled = TRUE;
			break;
		default:
			break;
		}
	}

	for (i=0; i<NUMRETBUTS; i++) {
		WIDGET *psWidget = widgGetFromID(psWScreen,ReticuleEnabled[i].id);

		if(psWidget != NULL) {
			if(psWidget->type != WIDG_LABEL) {
				if(ReticuleEnabled[i].Enabled) {
					widgSetButtonState(psWScreen, ReticuleEnabled[i].id, 0);
				} else {
					widgSetButtonState(psWScreen, ReticuleEnabled[i].id, WBUT_DISABLE);
				}

				if(ReticuleEnabled[i].Hidden) {
					widgHide(psWScreen, ReticuleEnabled[i].id);
				} else {
					widgReveal(psWScreen, ReticuleEnabled[i].id);
				}
			}
		}
	}
}

/*Checks to see if there are any research topics to do and flashes the button -
only if research facility is free*/
void intCheckResearchButton(void)
{
    UWORD       index, count;
	STRUCTURE	*psStruct;
	BOOL		resFree = FALSE;

	for (psStruct = interfaceStructList(); psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_RESEARCH &&
            psStruct->status == SS_BUILT &&
			((RESEARCH_FACILITY *)psStruct->pFunctionality)->psSubject == NULL)
		{
			resFree = TRUE;
			break;
		}

	}

	if (resFree)
	{
		//set to value that won't be reached in fillResearchList
        //needs to be UWORD sized for the Patches
        index = (UWORD)(numResearch + 1);
		//index = (UBYTE)(numResearch + 1);
		//calculate the list
		count = fillResearchList(pList,selectedPlayer, index, MAXRESEARCH);
		if (count)
		{
			//set the research reticule button to flash
			flashReticuleButton(IDRET_RESEARCH);
		}
	}
}


// see if a reticule button is enabled
BOOL intCheckReticuleButEnabled(UDWORD id)
{
	SDWORD	i;

	for (i=0; i<NUMRETBUTS; i++)
	{
		if (ReticuleEnabled[i].id == id)
		{
			return ReticuleEnabled[i].Enabled;
		}
	}

	return FALSE;
}

// Find any structure. Returns NULL if none found.
//
STRUCTURE *intFindAStructure(void)
{
	STRUCTURE *Struct;

	// First try and find a factory.
	Struct = intGotoNextStructureType(REF_FACTORY,FALSE,FALSE);
	if(Struct == NULL) {
		// If that fails then look for a command center.
		Struct = intGotoNextStructureType(REF_HQ,FALSE,FALSE);
		if(Struct == NULL) {
			// If that fails then look for a any structure.
			Struct = intGotoNextStructureType(REF_ANY,FALSE,FALSE);
		}
	}

	return Struct;
}


// Look through the players structures and find the next one of type structType.
//
STRUCTURE* intGotoNextStructureType(UDWORD structType,BOOL JumpTo,BOOL CancelDrive)
{
	STRUCTURE	*psStruct;
	BOOL Found = FALSE;

	if((SWORD)structType != CurrentStructType) {
		CurrentStruct = NULL;
		CurrentStructType = (SWORD)structType;
	}

	if(CurrentStruct != NULL) {
		psStruct = CurrentStruct;
	} else {
		psStruct = interfaceStructList();
	}

	for(; psStruct != NULL; psStruct = psStruct->psNext)
	{
		if( ((psStruct->pStructureType->type == structType) || (structType == REF_ANY)) &&
			psStruct->status == SS_BUILT)
		{
			if(psStruct != CurrentStruct) {
				if(CancelDrive) {
					clearSelection();
				} else {
					clearSel();
				}
				psStruct->selected = TRUE;
				CurrentStruct = psStruct;
				Found = TRUE;
				break;
			}
		}
	}

	// Start back at the begining?
	if((!Found) && (CurrentStruct != NULL)) {
		for(psStruct = interfaceStructList(); (psStruct != CurrentStruct) && (psStruct != NULL); psStruct = psStruct->psNext)
		{
			if( ((psStruct->pStructureType->type == structType) || (structType == REF_ANY)) &&
				 psStruct->status == SS_BUILT)
			{
				if(psStruct != CurrentStruct) {
					if(CancelDrive) {
						clearSelection();
					} else {
						clearSel();
					}
					psStruct->selected = TRUE;
					CurrentStruct = psStruct;
					Found = TRUE;
					break;
				}
			}
		}
	}

	// Center it on screen.
	if((CurrentStruct) && (JumpTo)) {
		intSetMapPos(CurrentStruct->pos.x, CurrentStruct->pos.y);
	}

	return CurrentStruct;
}

// Look through the players droids and find the next one of type droidType.
// If Current=NULL then start at the beginning overwise start at Current.
//
DROID *intGotoNextDroidType(DROID *CurrDroid,UDWORD droidType,BOOL AllowGroup)
{
	DROID *psDroid;
	BOOL Found = FALSE;

	if(CurrDroid != NULL) {
		CurrentDroid = CurrDroid;
	}

	if( ((SWORD)droidType != CurrentDroidType) && (droidType != DROID_ANY)) {
		CurrentDroid = NULL;
		CurrentDroidType = (SWORD)droidType;
	}

	if(CurrentDroid != NULL) {
		psDroid = CurrentDroid;
	} else {
		psDroid = apsDroidLists[selectedPlayer];
	}

	for(; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if( ( ((UDWORD)psDroid->droidType == droidType) ||
				  ((droidType == DROID_ANY) && (psDroid->droidType != DROID_TRANSPORTER)) ) &&
				  ((psDroid->group == UBYTE_MAX) || AllowGroup) )
		{
			if(psDroid != CurrentDroid) {
				clearSel();
				SelectDroid(psDroid);
//				psDroid->selected = TRUE;
				CurrentDroid = psDroid;
				Found = TRUE;
				break;
			}
		}
	}

	// Start back at the begining?
	if((!Found) && (CurrentDroid != NULL)) {
		for(psDroid = apsDroidLists[selectedPlayer]; (psDroid != CurrentDroid) && (psDroid != NULL); psDroid = psDroid->psNext)
		{
			if( ( ((UDWORD)psDroid->droidType == droidType) ||
				  ((droidType == DROID_ANY) && (psDroid->droidType != DROID_TRANSPORTER)) ) &&
				  ((psDroid->group == UBYTE_MAX) || AllowGroup) )
			{
				if(psDroid != CurrentDroid) {
					clearSel();
					SelectDroid(psDroid);
//					psDroid->selected = TRUE;
					CurrentDroid = psDroid;
					Found = TRUE;
					break;
				}
			}
		}
	}

	if(Found == TRUE) {

		if (bInTutorial)
		{
			psCBSelectedDroid = CurrentDroid;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_SELECTED);
			psCBSelectedDroid = NULL;
		}

		// Center it on screen.
		if(CurrentDroid) {
			intSetMapPos(CurrentDroid->pos.x, CurrentDroid->pos.y);
		}

		return CurrentDroid;
	}

	return NULL;
}

//access function for selected object in the interface
BASE_OBJECT * getCurrentSelected(void)
{
	return psObjSelected;
}
