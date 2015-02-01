/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * @file transporter.c
 *
 * Code to deal with loading/unloading, interface and flight of transporters.
 */
#include <string.h>
#include <sys/stat.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/math_ext.h"
#include "lib/widget/label.h"
#include "lib/widget/widget.h"

#include "stats.h"
#include "hci.h"
#include "intdisplay.h"
#include "objmem.h"
#include "transporter.h"
#include "group.h"
#include "display3d.h"
#include "mission.h"
#include "objects.h"
#include "display.h"
#include "qtscript.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "order.h"
#include "action.h"
#include "lib/gamelib/gtime.h"
#include "console.h"
#include "lib/ivis_opengl/bitimage.h"
#include "warcam.h"
#include "selection.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "mapgrid.h"
#include "visibility.h"
#include "multiplay.h"
#include "qtscript.h"

//#define IDTRANS_FORM			9000	//The Transporter base form
#define IDTRANS_CLOSE			9002	//The close button icon
#define IDTRANS_CONTCLOSE		9005	//The close icon on the Contents form
#define IDTRANS_DROIDTAB		9007	//The Droid tab form
#define IDTRANS_DROIDCLOSE		9008	//The close icon for the Droid form

#define IDTRANS_START			9100	//The first button on the Transporter tab form
#define	IDTRANS_END			9199	//The last button on the Transporter tab form
#define IDTRANS_STATSTART		9200	//The status button for the first Transporter
#define IDTRANS_STATEND			9299	//The status button for the last Transporter
#define IDTRANS_CONTSTART		9300	//The first button on the Transporter contents tab form
#define	IDTRANS_CONTEND			9399	//The last button on the Transporter contents tab form
#define	IDTRANS_DROIDSTART		9400	//The first button on the Droid tab form
#define	IDTRANS_DROIDEND		9499	//The last button on the Droid tab form
#define IDTRANS_REPAIRBARSTART		9600    //The first repair status bar on Droid button
#define IDTRANS_REPAIRBAREND		9699    //The last repair status bar on Droid button

/* Transporter screen positions */
#define TRANS_X					OBJ_BACKX
#define TRANS_Y					OBJ_BACKY
#define TRANS_WIDTH				OBJ_BACKWIDTH
#define TRANS_HEIGHT			OBJ_BACKHEIGHT

/*tabbed form screen positions */
#define TRANS_TABY				OBJ_TABY

/*Transported contents screen positions */
#define TRANSCONT_X				STAT_X
#define TRANSCONT_Y				STAT_Y
#define TRANSCONT_WIDTH			STAT_WIDTH
#define TRANSCONT_HEIGHT		STAT_HEIGHT

/*contents tabbed form screen positions */
#define TRANSCONT_TABY			STAT_TABFORMY

/*droid form screen positions */
#define TRANSDROID_X			RADTLX
#define TRANSDROID_Y			STAT_Y
#define TRANSDROID_WIDTH		STAT_WIDTH
#define TRANSDROID_HEIGHT		STAT_HEIGHT

//start y position of the available droids buttons
#define AVAIL_STARTY			0

//They all take up the same amount of space now - AB 30/10/98
//defines how much space each sized droid takes up on the Transporter
#define	LIGHT_DROID					1
#define MEDIUM_DROID				2
#define HEAVY_DROID					3


/* the widget screen */
extern W_SCREEN		*psWScreen;

/* Static variables */
static	DROID			*psCurrTransporter;
static	DROID			*g_psCurScriptTransporter = NULL;
static	bool			onMission;
static	UDWORD			g_iLaunchTime = 0;
//used for audio message for reinforcements
static  bool            bFirstTransporter;
//the tab positions of the DroidsAvail window
static  UWORD           objMajor = 0;

/*functions */
static bool intAddTransporterContents(void);
static void setCurrentTransporter(UDWORD id);
static void intRemoveTransContentNoAnim(void);
static bool intAddTransButtonForm(void);
static bool intAddTransContentsForm(void);
static bool intAddDroidsAvailForm(void);
static void intRemoveTransContent(void);
static DROID *transInterfaceDroidList(void);
static void intTransporterAddDroid(UDWORD id);
static void intRemoveTransDroidsAvail(void);
static void intRemoveTransDroidsAvailNoAnim(void);
static bool _intRefreshTransporter(void);
static bool _intAddTransporter(DROID *psSelected, bool offWorld);
static void _intProcessTransporter(UDWORD id);

//initialises Transporter variables
void initTransporters(void)
{
	onMission = false;
	psCurrTransporter = NULL;
}

// Call to refresh the transporter screen, ie when a droids boards it.
//
bool intRefreshTransporter(void)
{
	return _intRefreshTransporter();
}

static bool _intRefreshTransporter(void)
{
	// Is the transporter screen up?
	if ((intMode == INT_TRANSPORTER) &&
	    (widgGetFromID(psWScreen, IDTRANS_FORM) != NULL))
	{
		bool Ret;
		// Refresh it by re-adding it.
		Ret = intAddTransporter(psCurrTransporter, onMission);
		intMode = INT_TRANSPORTER;
		return Ret;
	}

	return true;
}

bool intAddTransporter(DROID *psSelected, bool offWorld)
{
	return(_intAddTransporter(psSelected, offWorld));
}

/*Add the Transporter Interface*/
static bool _intAddTransporter(DROID *psSelected, bool offWorld)
{
	bool			Animate = true;

	onMission = offWorld;
	psCurrTransporter = psSelected;

	/*if transporter has died - close the interface - this can only happen in
	multiPlayer where the transporter can be killed*/
	if (bMultiPlayer)
	{
		if (psCurrTransporter && isDead((BASE_OBJECT *)psCurrTransporter))
		{
			intRemoveTransNoAnim();
			return true;
		}
	}

	// Add the main Transporter form
	// Is the form already up?
	if (widgGetFromID(psWScreen, IDTRANS_FORM) != NULL)
	{
		intRemoveTransNoAnim();
		Animate = false;
	}

	if (intIsRefreshing())
	{
		Animate = false;
	}

	WIDGET *parent = psWScreen->psForm;

	IntFormAnimated *transForm = new IntFormAnimated(parent, Animate);  // Do not animate the opening, if the window was already open.
	transForm->id = IDTRANS_FORM;
	transForm->setGeometry(TRANS_X, TRANS_Y, TRANS_WIDTH, TRANS_HEIGHT);

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDTRANS_FORM;
	sButInit.id = IDTRANS_CLOSE;
	sButInit.x = TRANS_WIDTH - CLOSE_WIDTH;
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

	if (!intAddTransButtonForm())
	{
		return false;
	}

	// Add the Transporter Contents form (and buttons)
	if (!intAddTransporterContents())
	{
		return false;
	}

	//if on a mission - add the Droids back at home base form
	if (onMission)
	{
		if (!intAddDroidsAvailForm())
		{
			return false;
		}
	}

	return true;
}

// Add the main Transporter Contents Interface
bool intAddTransporterContents(void)
{
	bool			Animate = true;

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDTRANS_CONTENTFORM) != NULL)
	{
		intRemoveTransContentNoAnim();
		Animate = false;
	}

	if (intIsRefreshing())
	{
		Animate = false;
	}

	WIDGET *parent = psWScreen->psForm;

	IntFormAnimated *transContentForm = new IntFormAnimated(parent, Animate);  // Do not animate the opening, if the window was already open.
	transContentForm->id = IDTRANS_CONTENTFORM;
	transContentForm->setGeometry(TRANSCONT_X, TRANSCONT_Y, TRANSCONT_WIDTH, TRANSCONT_HEIGHT);

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDTRANS_CONTENTFORM;
	sButInit.id = IDTRANS_CONTCLOSE;
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
	if (bMultiPlayer)
	{
		//add the capacity label
		W_LABINIT sLabInit;
		sLabInit.formID = IDTRANS_CONTENTFORM;
		sLabInit.id = IDTRANS_CAPACITY;
		sLabInit.x = (SWORD)sButInit.x - 40;
		sLabInit.y = 0;
		sLabInit.width = 16;
		sLabInit.height = 16;
		sLabInit.pText = "00/10";
		sLabInit.pCallback = intUpdateTransCapacity;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	//add the Launch button if on a mission
	if (onMission)
	{
		W_FORMINIT sButFInit;
		sButFInit.formID = IDTRANS_CONTENTFORM;
		sButFInit.id = IDTRANS_LAUNCH;
		sButFInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;

		sButFInit.x = OBJ_STARTX;
		sButFInit.y = (UWORD)(STAT_SLDY - 1);

		sButFInit.width = iV_GetImageWidth(IntImages, IMAGE_LAUNCHUP);
		sButFInit.height = iV_GetImageHeight(IntImages, IMAGE_LAUNCHUP);
		sButFInit.pTip = _("Launch Transport");
		sButFInit.pDisplay = intDisplayImageHilight;

		sButFInit.UserData = PACKDWORD_TRI(0, IMAGE_LAUNCHDOWN, IMAGE_LAUNCHUP);

		if (!widgAddForm(psWScreen, &sButFInit))
		{
			return false;
		}
	}

	if (!intAddTransContentsForm())
	{
		return false;
	}

	return true;
}

/*This is used to display the transporter button and capacity when at the home base ONLY*/
bool intAddTransporterLaunch(DROID *psDroid)
{
	UDWORD          capacity;
	DROID           *psCurr, *psNext;

	if (bMultiPlayer)
	{
		return true;
	}

	//do this first so that if the interface is already up it syncs with this transporter
	//set up the static transporter
	psCurrTransporter = psDroid;

	//check the button is not already up
	if (widgGetFromID(psWScreen, IDTRANS_LAUNCH) != NULL)
	{
		return true;
	}

	W_FORMINIT sButInit;              //needs to be a clickable form now
	sButInit.formID = 0;
	sButInit.id = IDTRANS_LAUNCH;
	sButInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;
	sButInit.x = RET_X;
	sButInit.y = (SWORD)TIMER_Y;
	sButInit.width = (UWORD)(10 + iV_GetImageWidth(IntImages, IMAGE_LAUNCHUP));
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_LAUNCHUP);
	sButInit.pTip = _("Launch Transport");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_LAUNCHDOWN, IMAGE_LAUNCHUP);
	if (!widgAddForm(psWScreen, &sButInit))
	{
		return false;
	}

	//add the capacity label
	W_LABINIT sLabInit;
	sLabInit.formID = IDTRANS_LAUNCH;
	sLabInit.id = IDTRANS_CAPACITY;
	sLabInit.x = (SWORD)(sButInit.x + 20);
	sLabInit.y = 0;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "00/10";
	sLabInit.pCallback = intUpdateTransCapacity;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	//when full flash the transporter button
	if (psCurrTransporter && psCurrTransporter->psGroup)
	{
		capacity = TRANSPORTER_CAPACITY;
		for (psCurr = psCurrTransporter->psGroup->psList; psCurr != NULL; psCurr = psNext)
		{
			psNext = psCurr->psGrpNext;
			if (psCurr != psCurrTransporter)
			{
				capacity -= transporterSpaceRequired(psCurr);
			}
		}
		if (capacity <= 0)
		{
			flashMissionButton(IDTRANS_LAUNCH);
		}
	}

	return true;
}

/* Remove the Transporter Launch widget from the screen*/
void intRemoveTransporterLaunch(void)
{
	if (widgGetFromID(psWScreen, IDTRANS_LAUNCH) != NULL)
	{
		widgDelete(psWScreen, IDTRANS_LAUNCH);
	}
}

/* Add the Transporter Button form */
bool intAddTransButtonForm(void)
{
	WIDGET *transForm = widgGetFromID(psWScreen, IDTRANS_FORM);

	/* Add the button form */
	IntListTabWidget *transList = new IntListTabWidget(transForm);
	transList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT*2);
	transList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int objListWidth = OBJ_BUTWIDTH*5 + OBJ_GAP*4;
	transList->setGeometry((OBJ_BACKWIDTH - objListWidth)/2, TRANS_TABY, objListWidth, transForm->height() - TRANS_TABY);

	/* Add the transporter and status buttons */
	int nextObjButtonId = IDTRANS_START;
	int nextStatButtonId = IDTRANS_STATSTART;

	//add each button
	for (DROID *psDroid = transInterfaceDroidList(); psDroid; psDroid = psDroid->psNext)
	{
		//only interested in Transporter droids
		if ((isTransporter(psDroid) && (psDroid->action == DACTION_TRANSPORTOUT  ||
			psDroid->action == DACTION_TRANSPORTIN)) || !isTransporter(psDroid))
		{
			continue;
		}

		WIDGET *buttonHolder = new WIDGET(transList);
		transList->addWidgetToLayout(buttonHolder);

		IntStatusButton *statButton = new IntStatusButton(buttonHolder);
		statButton->id = nextStatButtonId;
		statButton->setGeometry(0, 0, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);

		IntObjectButton *objButton = new IntObjectButton(buttonHolder);
		objButton->id = nextObjButtonId;
		objButton->setGeometry(0, OBJ_STARTY, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);

		/* Set the tip and add the button */
		objButton->setTip(droidGetName(psDroid));
		objButton->setObject(psDroid);

		//set the first Transporter to be the current one if not already set
		if (psCurrTransporter == nullptr)
		{
			psCurrTransporter = psDroid;
		}

		/* if the current droid matches psCurrTransporter lock the button */
		if (psDroid == psCurrTransporter)
		{
			objButton->setState(WBUT_LOCK);
			transList->setCurrentPage(transList->pages() - 1);
		}

		//now do status button
		statButton->setObject(nullptr);

		/* Update the init struct for the next buttons */
		++nextObjButtonId;
		ASSERT(nextObjButtonId < IDTRANS_END, "Too many Transporter buttons");

		++nextStatButtonId;
		ASSERT(nextStatButtonId < IDTRANS_STATEND, "Too many Transporter status buttons");
	}
	return true;
}

/* Add the Transporter Contents form */
bool intAddTransContentsForm(void)
{
	WIDGET *contForm = widgGetFromID(psWScreen, IDTRANS_CONTENTFORM);

	/* Add the contents form */
	IntListTabWidget *contList = new IntListTabWidget(contForm);
	contList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
	contList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int contListWidth = OBJ_BUTWIDTH*2 + OBJ_GAP;
	contList->setGeometry((contForm->width() - contListWidth)/2, TRANSCONT_TABY, contListWidth, contForm->height() - TRANSCONT_TABY);

	/* Add the transporter contents buttons */
	int nextButtonId = IDTRANS_CONTSTART;

	//add each button
	if (psCurrTransporter == nullptr)
	{
		return true;
	}

	for (DROID *psDroid = psCurrTransporter->psGroup->psList; psDroid != nullptr && psDroid != psCurrTransporter; psDroid = psDroid->psGrpNext)
	{
		if (psDroid->selected)
		{
			continue;  // Droid is queued to be ejected from the transport, so don't display it.
		}

		/* Set the tip and add the button */
		IntTransportButton *button = new IntTransportButton(contList);
		button->id = nextButtonId;
		button->setTip(droidGetName(psDroid));
		button->setObject(psDroid);
		contList->addWidgetToLayout(button);

		/* Update the init struct for the next button */
		++nextButtonId;
		ASSERT(nextButtonId < IDTRANS_CONTEND, "Too many Transporter Droid buttons");
	}
	return true;
}

/* Add the Droids back at home form */
bool intAddDroidsAvailForm(void)
{
	// Is the form already up?
	bool Animate = true;
	if (widgGetFromID(psWScreen, IDTRANS_DROIDS) != NULL)
	{
		intRemoveTransDroidsAvailNoAnim();
		Animate = false;
	}

	if (intIsRefreshing())
	{
		Animate = false;
	}

	WIDGET *parent = psWScreen->psForm;

	/* Add the droids available form */
	IntFormAnimated *transDroids = new IntFormAnimated(parent, Animate);  // Do not animate the opening, if the window was already open.
	transDroids->id = IDTRANS_DROIDS;
	transDroids->setGeometry(TRANSDROID_X, TRANSDROID_Y, TRANSDROID_WIDTH, TRANSDROID_HEIGHT);

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDTRANS_DROIDS;
	sButInit.id = IDTRANS_DROIDCLOSE;
	sButInit.x = TRANSDROID_WIDTH - CLOSE_WIDTH;
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

	//now add the tabbed droids available form
	IntListTabWidget *droidList = new IntListTabWidget(transDroids);
	droidList->id = IDTRANS_DROIDTAB;
	droidList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
	droidList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int droidListWidth = OBJ_BUTWIDTH*2 + OBJ_GAP;
	droidList->setGeometry((TRANSDROID_WIDTH - droidListWidth)/2, AVAIL_STARTY + 15, droidListWidth, TRANSDROID_HEIGHT - (AVAIL_STARTY + 15));

	/* Add the droids available buttons */
	int nextButtonId = IDTRANS_DROIDSTART;

	/* Add the state of repair bar for each droid*/
	W_BARINIT sBarInit;
	sBarInit.id = IDTRANS_REPAIRBARSTART;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.y = STAT_TIMEBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;

	//add droids built before the mission
	for (DROID *psDroid = mission.apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		//stop adding the buttons once IDTRANS_DROIDEND has been reached
		if (nextButtonId == IDTRANS_DROIDEND)
		{
			break;
		}
		//don't add Transporter Droids!
		if (!isTransporter(psDroid))
		{
			/* Set the tip and add the button */
			IntTransportButton *button = new IntTransportButton(droidList);
			button->id = nextButtonId;
			button->setTip(droidGetName(psDroid));
			button->setObject(psDroid);
			droidList->addWidgetToLayout(button);

			//add bar to indicate stare of repair
			sBarInit.size = (UWORD) PERCENT(psDroid->body, psDroid->originalBody);
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			sBarInit.formID = nextButtonId;
			//sBarInit.iRange = TBAR_MAX_REPAIR;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return false;
			}

			/* Update the init struct for the next button */
			++nextButtonId;
			ASSERT(nextButtonId < IDTRANS_DROIDEND, "Too many Droids Built buttons");

			//and bar
			sBarInit.id += 1;
		}
	}

	//reset which tab we were on
	droidList->setCurrentPage(objMajor);

	return true;
}

/*calculates how much space is remaining on the transporter - allows droids to take
up different amount depending on their body size - currently all are set to one!*/
UDWORD calcRemainingCapacity(DROID *psTransporter)
{
	SDWORD	capacity = TRANSPORTER_CAPACITY;
	DROID *psDroid, *psNext;

	// If it's dead then just return 0.
	if (isDead((BASE_OBJECT *)psTransporter))
	{
		return 0;
	}

	for (psDroid = psTransporter->psGroup->psList; psDroid != NULL && psDroid !=
	     psTransporter; psDroid = psNext)
	{
		psNext = psDroid->psGrpNext;
		capacity -= transporterSpaceRequired(psDroid);
	}

	if (capacity < 0)
	{
		capacity = 0;
	}

	return (UDWORD)capacity;
}

bool transporterIsEmpty(const DROID *psTransporter)
{
	ASSERT(isTransporter(psTransporter), "Non-transporter droid given");

	// Assume dead droids and non-transporter droids to be empty
	return (isDead((const BASE_OBJECT *)psTransporter)
			|| !isTransporter(psTransporter)
	        || psTransporter->psGroup->psList == NULL
	        || psTransporter->psGroup->psList == psTransporter);
}

static void intSetTransCapacityLabel(QString &text)
{
	UDWORD capacity = TRANSPORTER_CAPACITY;

	if (psCurrTransporter)
	{
		capacity = calcRemainingCapacity(psCurrTransporter);

		//change round the way the remaining capacity is displayed - show 0/10 when empty now
		capacity = TRANSPORTER_CAPACITY - capacity;

		char tmp[40];
		ssprintf(tmp, "%02u/10", capacity);
		text = QString::fromUtf8(tmp);
	}
}

/*updates the capacity of the current Transporter*/
void intUpdateTransCapacity(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL		*Label = (W_LABEL *)psWidget;

	intSetTransCapacityLabel(Label->aText);
}

/* Process return codes from the Transporter Screen*/
void intProcessTransporter(UDWORD id)
{
	_intProcessTransporter(id);
}

static void _intProcessTransporter(UDWORD id)
{
	if (id >= IDTRANS_START && id <= IDTRANS_END)
	{
		/* A Transporter button has been pressed */
		setCurrentTransporter(id);
		/*refresh the Contents list */
		intAddTransporterContents();
	}
	else if (id >= IDTRANS_CONTSTART && id <= IDTRANS_CONTEND)
	{
		//got to have a current transporter for this to work - and can't be flying
		if (psCurrTransporter != NULL && !transporterFlying(psCurrTransporter))
		{
			unsigned currID = IDTRANS_CONTSTART;
			DROID *psDroid;
			for (psDroid = psCurrTransporter->psGroup->psList; psDroid != NULL && psDroid != psCurrTransporter; psDroid = psDroid->psGrpNext)
			{
				if (psDroid->selected)
				{
					continue;  // Already scheduled this droid for removal.
				}
				if (currID == id)
				{
					break;
				}
				currID++;
			}
			if (psDroid != NULL)
			{
				transporterRemoveDroid(psCurrTransporter, psDroid, ModeQueue);
			}
			/*refresh the Contents list */
			intAddTransporterContents();
			if (onMission)
			{
				/*refresh the Avail list */
				intAddDroidsAvailForm();
			}
		}
	}
	else if (id == IDTRANS_CLOSE)
	{
		intRemoveTransContent();
		intRemoveTrans();
		psCurrTransporter = NULL;
	}
	else if (id == IDTRANS_CONTCLOSE)
	{
		intRemoveTransContent();
	}
	else if (id == IDTRANS_DROIDCLOSE)
	{
		intRemoveTransDroidsAvail();
	}
	else if (id >= IDTRANS_DROIDSTART && id <= IDTRANS_DROIDEND)
	{
		//got to have a current transporter for this to work - and can't be flying
		if (psCurrTransporter != NULL && !transporterFlying(psCurrTransporter))
		{
			intTransporterAddDroid(id);
			/*don't need to explicitly refresh here since intRefreshScreen()
			is called by intTransporterAddDroid()*/
		}
	}
}

/* Remove the Transporter widgets from the screen */
void intRemoveTrans(void)
{
	// Start the window close animation.
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDTRANS_FORM);
	if (form)
	{
		form->closeAnimateDelete();
	}

	intRemoveTransContent();
	intRemoveTransDroidsAvail();
	intMode = INT_NORMAL;
}

/* Remove the Transporter Content widgets from the screen w/o animation!*/
void intRemoveTransNoAnim(void)
{
	//remove main screen
	widgDelete(psWScreen, IDTRANS_FORM);
	intRemoveTransContentNoAnim();
	intRemoveTransDroidsAvailNoAnim();
	intMode = INT_NORMAL;
}

/* Remove the Transporter Content widgets from the screen */
void intRemoveTransContent(void)
{
	// Start the window close animation.
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDTRANS_CONTENTFORM);
	if (form)
	{
		form->closeAnimateDelete();
	}
}

/* Remove the Transporter Content widgets from the screen w/o animation!*/
void intRemoveTransContentNoAnim(void)
{
	//remove main screen
	widgDelete(psWScreen, IDTRANS_CONTENTFORM);
}

/* Remove the Transporter Droids Avail widgets from the screen */
void intRemoveTransDroidsAvail(void)
{
	// Start the window close animation.
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDTRANS_DROIDS);
	if (form)
	{
		//remember which tab we were on
		ListTabWidget *droidList = (ListTabWidget *)widgGetFromID(psWScreen, IDTRANS_DROIDTAB);
		objMajor = droidList->currentPage();
		form->closeAnimateDelete();
	}
}

/* Remove the Transporter Droids Avail widgets from the screen w/o animation!*/
void intRemoveTransDroidsAvailNoAnim(void)
{
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDTRANS_DROIDS);
	if (form != nullptr)
	{
		//remember which tab we were on
		ListTabWidget *droidList = (ListTabWidget *)widgGetFromID(psWScreen, IDTRANS_DROIDTAB);
		objMajor = droidList->currentPage();

		//remove main screen
		delete form;
	}
}

/*sets psCurrTransporter */
void setCurrentTransporter(UDWORD id)
{
	DROID	*psDroid;
	UDWORD	currID;

	psCurrTransporter = NULL;
	currID = IDTRANS_START;

	//loop thru all the droids to find the selected one
	for (psDroid = transInterfaceDroidList(); psDroid != NULL; psDroid =
	        psDroid->psNext)
	{
		if (isTransporter(psDroid) &&
		    (psDroid->action != DACTION_TRANSPORTOUT &&
		     psDroid->action != DACTION_TRANSPORTIN))
		{
			if (currID == id)
			{
				break;
			}
			currID++;
		}
	}
	if (psDroid)
	{
		psCurrTransporter = psDroid;
		//set the data for the transporter timer
		widgSetUserData(psWScreen, IDTRANTIMER_DISPLAY, (void *)psCurrTransporter);
	}
}

/*removes a droid from the group associated with the transporter*/
void transporterRemoveDroid(DROID *psTransport, DROID *psDroid, QUEUE_MODE mode)
{
	ASSERT_OR_RETURN(, psTransport != NULL && psDroid != NULL && psTransport != psDroid, "Something NULL or unloading transporter from itself");

	if (bMultiMessages && mode == ModeQueue)
	{
		sendDroidDisembark(psTransport, psDroid);
		psDroid->selected = true;  // Remove from interface.
		return;
	}

	/*if we're offWorld we can't pick a tile without swapping the map
	pointers - can't be bothered so just do this...*/
	if (onMission)
	{
		psDroid->pos.x = INVALID_XY;
		psDroid->pos.y = INVALID_XY;
	}
	else
	{
		Vector2i droidPos;
		if (bMultiPlayer)
		{
			//set the units next to the transporter's current location
			droidPos = map_coord(psTransport->pos);
		}
		else
		{
			//pick a tile because save games won't remember where the droid was when it was loaded
			droidPos = map_coord(Vector2i(getLandingX(0), getLandingY(0)));
		}
		if (!pickATileGen(&droidPos, LOOK_FOR_EMPTY_TILE, zonedPAT))
		{
			ASSERT(false, "Unable to find a valid location");
		}
		droidSetPosition(psDroid, world_coord(droidPos.x), world_coord(droidPos.y));
		updateDroidOrientation(psDroid);
	}

	// remove it from the transporter group
	psDroid->psGroup->remove(psDroid);

	//add it back into apsDroidLists
	if (onMission)
	{
		addDroid(psDroid, mission.apsDroidLists);
	}
	else
	{
		// add the droid back onto the droid list
		addDroid(psDroid, apsDroidLists);
	}

	if (psDroid->pos.x != INVALID_XY)
	{
		// We can update the orders now, since everyone has been
		// notified of the droid exiting the transporter
		updateDroidOrientation(psDroid);
	}
	//initialise the movement data
	initDroidMovement(psDroid);
	//reset droid orders
	orderDroid(psDroid, DORDER_STOP, ModeImmediate);
	psDroid->cluster = 0;
	// check if it is a commander
	if (psDroid->droidType == DROID_COMMAND)
	{
		DROID_GROUP *psGroup = grpCreate();
		psGroup->add(psDroid);
	}
	psDroid->selected = true;

	if (calcRemainingCapacity(psTransport))
	{
		//make sure the button isn't flashing
		stopMissionButtonFlash(IDTRANS_LAUNCH);
	}
}

/*adds a droid to the current transporter via the interface*/
void intTransporterAddDroid(UDWORD id)
{
	DROID		*psDroid, *psNext;
	UDWORD		currID;

	ASSERT(psCurrTransporter != NULL, "intTransporterAddUnit:can't remove units");

	currID = IDTRANS_DROIDSTART;
	for (psDroid = transInterfaceDroidList(); psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		if (!isTransporter(psDroid))
		{
			if (currID == id)
			{
				break;
			}
			currID++;
		}
	}
	if (psDroid)
	{
		transporterAddDroid(psCurrTransporter, psDroid);
	}
}

/*Adds a droid to the transporter, removing it from the world */
void transporterAddDroid(DROID *psTransporter, DROID *psDroidToAdd)
{
	bool    bDroidRemoved;

	ASSERT(psTransporter != NULL, "Was passed a NULL transporter");
	ASSERT(psDroidToAdd != NULL, "Was passed a NULL droid, can't add to transporter");

	if (!psTransporter || !psDroidToAdd)
	{
		debug(LOG_ERROR, "We can't add the unit to the transporter!");
		return;
	}
	/* check for space */
	if (!checkTransporterSpace(psTransporter, psDroidToAdd))
	{
		if (psTransporter->player == selectedPlayer)
		{
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
			addConsoleMessage(_("There is not enough room in the Transport!"), DEFAULT_JUSTIFY, selectedPlayer);
		}
		return;
	}
	if (onMission)
	{
		// removing from droid mission list
		bDroidRemoved = droidRemove(psDroidToAdd, mission.apsDroidLists);
	}
	else
	{
		// removing from droid list
		bDroidRemoved = droidRemove(psDroidToAdd, apsDroidLists);
	}

	if (bDroidRemoved)
	{
		// adding to transporter unit's group list
		psTransporter->psGroup->add(psDroidToAdd);
		psDroidToAdd->selected = false;  // Display in transporter interface.
	}
	else
	{
		debug(LOG_ERROR, "droid %d not found, so nothing added to transporter!", psDroidToAdd->id);
	}
	if (onMission)
	{
		visRemoveVisibilityOffWorld((BASE_OBJECT *)psDroidToAdd);
	}
	else
	{
		visRemoveVisibility((BASE_OBJECT *)psDroidToAdd);
	}
	fpathRemoveDroidData(psDroidToAdd->id);

	// This is called by droidRemove. But we still need to refresh after adding to the transporter group.
	intRefreshScreen();
}

/*check to see if the droid can fit on the Transporter - return true if fits*/
bool checkTransporterSpace(DROID const *psTransporter, DROID const *psAssigned, bool mayFlash)
{
	DROID		*psDroid, *psNext;
	UDWORD		capacity;

	ASSERT(psTransporter != NULL, "Invalid droid pointer");
	ASSERT(psAssigned != NULL, "Invalid droid pointer");
	ASSERT(isTransporter(psTransporter), "Droid is not a Transporter");
	ASSERT(psTransporter->psGroup != NULL, "tranporter doesn't have a group");

	//work out how much space is currently left
	capacity = TRANSPORTER_CAPACITY;
	for (psDroid = psTransporter->psGroup->psList; psDroid != NULL && psDroid != psTransporter; psDroid = psNext)
	{
		psNext = psDroid->psGrpNext;
		capacity -= transporterSpaceRequired(psDroid);
	}
	if (capacity >= transporterSpaceRequired(psAssigned))
	{
		//when full flash the transporter button
		if (mayFlash && capacity - transporterSpaceRequired(psAssigned) == 0)
		{
			flashMissionButton(IDTRANS_LAUNCH);
		}
		return true;
	}
	else
	{
		return false;
	}
}

/*returns the space the droid occupies on a transporter based on the body size*/
UDWORD transporterSpaceRequired(DROID const *psDroid)
{
	// all droids are the same weight for campaign games.
	// TODO - move this into a droid flag
	return bMultiPlayer ? (asBodyStats + psDroid->asBits[COMP_BODY])->size : 1;
}

/*sets which list of droids to use for the transporter interface*/
DROID *transInterfaceDroidList(void)
{
	if (onMission)
	{
		return mission.apsDroidLists[selectedPlayer];
	}
	else
	{
		return apsDroidLists[selectedPlayer];
	}
}

UDWORD transporterGetLaunchTime(void)
{
	return g_iLaunchTime;
}

void transporterSetLaunchTime(UDWORD time)
{
	g_iLaunchTime = time;
}

/*launches the defined transporter to the offworld map*/
bool launchTransporter(DROID *psTransporter)
{
	UDWORD	iX, iY;

	//close the interface
	intResetScreen(true);

	//this launches the mission if on homebase when the button is pressed
	if (!onMission)
	{
		//tell the transporter to move to the new offworld location
		missionGetTransporterExit(psTransporter->player, &iX, &iY);
		orderDroidLoc(psTransporter, DORDER_TRANSPORTOUT, iX, iY, ModeQueue);
		//g_iLaunchTime = gameTime;
		transporterSetLaunchTime(gameTime);
	}
	//otherwise just launches the Transporter
	else
	{
		if (!isTransporter(psTransporter))
		{
			ASSERT(false, "Invalid Transporter Droid");
			return false;
		}

		orderDroid(psTransporter, DORDER_TRANSPORTIN, ModeImmediate);
		/* set action transporter waits for timer */
		actionDroid(psTransporter, DACTION_TRANSPORTWAITTOFLYIN);

		missionSetReinforcementTime(gameTime);
	}

	return true;
}

#define	TRANSPORTOUT_TIME	4*GAME_TICKS_PER_SEC

/*checks how long the transporter has been travelling to see if it should
have arrived - returns true when there*/
bool updateTransporter(DROID *psTransporter)
{
	ASSERT_OR_RETURN(true, psTransporter != NULL, "Invalid droid pointer");
	ASSERT_OR_RETURN(true, isTransporter(psTransporter), "Invalid droid type");

	//if not moving to mission site, exit
	if (psTransporter->action != DACTION_TRANSPORTOUT &&
	    psTransporter->action != DACTION_TRANSPORTIN)
	{
		return true;
	}

	/*if the transporter (selectedPlayer only) is moving droids to safety and
	all remaining droids are destoyed then we need to flag the end of mission
	as long as we're not flying out*/
	if (psTransporter->player == selectedPlayer && getDroidsToSafetyFlag()
	    && psTransporter->action != DACTION_TRANSPORTOUT)
	{
		//if there aren't any droids left...
		if (!missionDroidsRemaining(selectedPlayer))
		{
			// Set the Transporter to have arrived at its destination
			psTransporter->action = DACTION_NONE;

			//the script can call startMission for this callback for offworld missions
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_START_NEXT_LEVEL);
			triggerEvent(TRIGGER_TRANSPORTER_EXIT, psTransporter);

			// clear order
			psTransporter->order = DroidOrder(DORDER_NONE);

			return true;
		}
	}

	// moving to a location
	// if we're coming back for more droids then we want the transporter to
	// fly to edge of map before turning round again
	if (psTransporter->sMove.Status == MOVEINACTIVE ||
	    psTransporter->sMove.Status == MOVEHOVER ||
	    (psTransporter->action == DACTION_TRANSPORTOUT && !missionIsOffworld() &&
	     (gameTime > transporterGetLaunchTime() + TRANSPORTOUT_TIME) &&
	     !getDroidsToSafetyFlag()))
	{
		audio_StopObjTrack(psTransporter, ID_SOUND_BLIMP_FLIGHT);
		if (psTransporter->action == DACTION_TRANSPORTIN)
		{
			/* !!!! GJ Hack - should be landing audio !!!! */
			audio_PlayObjDynamicTrack(psTransporter, ID_SOUND_BLIMP_TAKE_OFF, NULL);
		}

		if (!bFirstTransporter && missionForReInforcements() &&
		    psTransporter->action == DACTION_TRANSPORTIN &&
		    psTransporter->player == selectedPlayer)
		{
			//play reinforcements have arrived message
			audio_QueueTrackPos(ID_SOUND_TRANSPORT_LANDING,
			        psTransporter->pos.x, psTransporter->pos.y, psTransporter->pos.z);
			addConsoleMessage(_("Reinforcements landing"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
			//reset the data for the transporter timer
			widgSetUserData(psWScreen, IDTRANTIMER_DISPLAY, (void *)NULL);
			return true;
		}

		// Got to destination
		psTransporter->action = DACTION_NONE;

		//reset the flag to trigger the audio message
		bFirstTransporter = false;

		return true;
	}

	//not arrived yet...
	return false;
}

//process the launch transporter button click
void processLaunchTransporter(void)
{
	UDWORD		capacity = TRANSPORTER_CAPACITY;
	W_CLICKFORM *psForm;

	//launch the Transporter
	if (psCurrTransporter)
	{
		//check there is something on the transporter
		capacity = calcRemainingCapacity(psCurrTransporter);
		if (capacity != TRANSPORTER_CAPACITY)
		{
			//make sure the button doesn't flash once launched
			stopMissionButtonFlash(IDTRANS_LAUNCH);
			//disable the form so can't add any more droids into the transporter
			psForm = (W_CLICKFORM *)widgGetFromID(psWScreen, IDTRANS_LAUNCH);
			if (psForm)
			{
				psForm->setState(WBUT_LOCK);
			}

			//disable the form so can't add any more droids into the transporter
			psForm = (W_CLICKFORM *)widgGetFromID(psWScreen, IDTRANTIMER_BUTTON);
			if (psForm)
			{
				psForm->setState(WBUT_LOCK);
			}

			launchTransporter(psCurrTransporter);
			//set the data for the transporter timer
			widgSetUserData(psWScreen, IDTRANTIMER_DISPLAY, (void *)psCurrTransporter);

			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_LAUNCH_TRANSPORTER);
			triggerEvent(TRIGGER_TRANSPORTER_LAUNCH, psCurrTransporter);
		}
	}
}

SDWORD	bobTransporterHeight(void)
{
	// Because 4320/12 = 360 degrees
	// this gives us a bob frequency of 4.32 seconds.
	// we scale amplitude to 10 (world coordinate metric).
	// we need to use 360 degrees and not 180, as otherwise
	// it will not 'bounce' off the top _and_ bottom of
	// it's movemment arc.

	return iSinSR(gameTime, 4320, 10);
}

/*causes one of the mission buttons (Launch Button or Mission Timer) to start flashing*/
void flashMissionButton(UDWORD buttonID)
{
	//get the button from the id
	WIDGET *psForm = widgGetFromID(psWScreen, buttonID);
	if (psForm)
	{
		switch (buttonID)
		{
		case IDTRANS_LAUNCH:
			psForm->UserData = PACKDWORD_TRI(1, IMAGE_LAUNCHDOWN, IMAGE_LAUNCHUP);
			break;
		case IDTIMER_FORM:
			psForm->UserData = PACKDWORD_TRI(1, IMAGE_MISSION_CLOCK, IMAGE_MISSION_CLOCK_UP);
			break;
		default:
			//do nothing other than in debug
			ASSERT(false, "flashMissionButton: Unknown button ID");
			break;
		}
	}
}

/*stops one of the mission buttons (Launch Button or Mission Timer) flashing*/
void stopMissionButtonFlash(UDWORD buttonID)
{
	//get the button from the id
	WIDGET *psForm = widgGetFromID(psWScreen, buttonID);
	if (psForm)
	{
		switch (buttonID)
		{
		case IDTRANS_LAUNCH:
			psForm->UserData = PACKDWORD_TRI(0, IMAGE_LAUNCHDOWN, IMAGE_LAUNCHUP);
			break;
		case IDTIMER_FORM:
			psForm->UserData = PACKDWORD_TRI(0, IMAGE_MISSION_CLOCK, IMAGE_MISSION_CLOCK_UP);
			break;
		default:
			//do nothing other than in debug
			ASSERT(false, "stopMissionButtonFlash: Unknown button ID");
			break;
		}
	}
}

/* set current transporter (for script callbacks) */
void transporterSetScriptCurrent(DROID *psTransporter)
{
	g_psCurScriptTransporter = psTransporter;
}

/* get current transporter (for script callbacks) */
DROID *transporterGetScriptCurrent(void)
{
	return g_psCurScriptTransporter;
}

/*called when a Transporter has arrived back at the LZ when sending droids to safety*/
void resetTransporter()
{
	W_CLICKFORM *psForm;

	//enable the form so can add more droids into the transporter
	psForm = (W_CLICKFORM *)widgGetFromID(psWScreen, IDTRANS_LAUNCH);
	if (psForm)
	{
		psForm->setState(0);
	}
}

/*checks the order of the droid to see if its currently flying*/
bool transporterFlying(DROID *psTransporter)
{
	ASSERT(psTransporter != NULL, "Invalid droid pointer");
	ASSERT(isTransporter(psTransporter), "Droid is not a Transporter");

	return psTransporter->order.type == DORDER_TRANSPORTOUT ||
	       psTransporter->order.type == DORDER_TRANSPORTIN ||
	       psTransporter->order.type == DORDER_TRANSPORTRETURN ||
	       //in multiPlayer mode the Transporter can be moved around
	       (bMultiPlayer && psTransporter->order.type == DORDER_MOVE) ||
	       //in multiPlayer mode the Transporter can be moved and emptied!
	       (bMultiPlayer && psTransporter->order.type == DORDER_DISEMBARK) ||
	       //in multiPlayer, descending still counts as flying
	       (bMultiPlayer && psTransporter->order.type == DORDER_NONE &&
	        psTransporter->sMove.iVertSpeed != 0);
}

//initialise the flag to indicate the first transporter has arrived - set in startMission()
void initFirstTransporterFlag(void)
{
	bFirstTransporter = true;
}
