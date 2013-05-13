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
 * @file design.c
 *
 * Functions for design screen.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"

#include "objects.h"
#include "loop.h"
#include "map.h"

/* Includes direct access to render library */
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"//matrix code
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/piemode.h"

#include "display3d.h"
#include "edit3d.h"
#include "structure.h"
#include "research.h"
#include "lib/gamelib/gtime.h"
#include "hci.h"
#include "stats.h"
#include "game.h"
#include "power.h"
#include "lib/sound/audio.h"
#include "lib/widget/widgint.h"
#include "lib/widget/bar.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/slider.h"
#include "order.h"
#include "projectile.h"

#include "intimage.h"
#include "intdisplay.h"
#include "design.h"
#include "component.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "main.h"
#include "objects.h"
#include "display.h"
#include "console.h"
#include "cmddroid.h"
#include "scriptextern.h"
#include "mission.h"
#include "template.h"
#include "multiplay.h"
#include "multistat.h"
#include "qtscript.h"


#define MAX_DESIGN_COMPONENTS 40		// Max number of stats the design screen can cope with.
#define MAX_SYSTEM_COMPONENTS 128


/***************************************************************************************/
/*                  Max values for the design bar graphs                               */

#define DBAR_TEMPLATEMAXPOINTS      8400            //maximum body points for a template
#define DBAR_TEMPLATEMAXPOWER       1000            //maximum power points for a template

/* The maximum number of characters on the component buttons */
#define DES_COMPBUTMAXCHAR			5


/* Which type of system is displayed on the design screen */
enum DES_SYSMODE
{
	IDES_SENSOR,		// The sensor clickable is displayed
	IDES_ECM,			// The ECM clickable is displayed
	IDES_CONSTRUCT,		// The Constructor clickable is displayed
	IDES_REPAIR,		// The Repair clickable is displayed
	IDES_WEAPON,		// The Weapon clickable is displayed
	IDES_COMMAND,		// The command droid clickable is displayed
	IDES_NOSYSTEM,		// No system clickable has been displayed
};
static DES_SYSMODE desSysMode;

/* The major component tabs on the design screen */
#define IDES_MAINTAB	0
#define IDES_EXTRATAB	1

/* Which component type is being selected on the design screen */
//added IDES_TURRET_A,IDES_TURRET_B,changing the name of IDES_TURRET might break exist codes
enum DES_COMPMODE
{
	IDES_SYSTEM,		// The main system for the droid (sensor, ECM, constructor)
	IDES_TURRET,		// The weapon for the droid
	IDES_BODY,			// The droid body
	IDES_PROPULSION,	// The propulsion system
	IDES_NOCOMPONENT,	// No system has been selected
	IDES_TURRET_A,		// The 2nd turret
	IDES_TURRET_B,		// The 3rd turret
};
static DES_COMPMODE desCompMode;

/* Which type of propulsion is being selected */
enum DES_PROPMODE
{
	IDES_GROUND,		// Ground propulsion (wheeled, tracked, etc).
	IDES_AIR,			// Air propulsion
	IDES_NOPROPULSION,	// No propulsion has been selected
};
static DES_PROPMODE desPropMode;


#define STRING_BUFFER_SIZE (32 * MAX_STR_LENGTH)
char StringBuffer[STRING_BUFFER_SIZE];

/* Design screen positions */
#define DESIGN_Y				(59 + D_H)	//the top left y value for all forms on the design screen

#define DES_TABBUTGAP		2
#define DES_TABBUTWIDTH		60
#define DES_TABBUTHEIGHT	46

#define DES_LEFTFORMX		RET_X
#define DES_LEFTFORMY		DESIGN_Y
#define DES_LEFTFORMWIDTH	RET_FORMWIDTH
#define DES_LEFTFORMHEIGHT	258
#define	DES_LEFTFORMBUTX	2
#define	DES_LEFTFORMBUTY	2

#define DES_CENTERFORMWIDTH		315
#define DES_CENTERFORMHEIGHT	262
#define DES_CENTERFORMX			POW_X
#define DES_CENTERFORMY			DESIGN_Y

#define DES_3DVIEWX				8
#define DES_3DVIEWY				25
#define DES_3DVIEWWIDTH			236
#define DES_3DVIEWHEIGHT		192

#define	DES_STATSFORMX			POW_X
#define	DES_STATSFORMY			(DES_CENTERFORMY + DES_CENTERFORMHEIGHT + 3)
#define	DES_STATSFORMWIDTH		DES_CENTERFORMWIDTH
#define	DES_STATSFORMHEIGHT		100

#define	DES_PARTFORMX			DES_3DVIEWX + DES_3DVIEWWIDTH + 2
#define	DES_PARTFORMY			DES_3DVIEWY
#define	DES_PARTFORMHEIGHT		DES_3DVIEWHEIGHT

#define DES_POWERFORMX			DES_3DVIEWX
#define DES_POWERFORMY			(DES_3DVIEWY + DES_3DVIEWHEIGHT + 2)
#define DES_POWERFORMWIDTH		(DES_CENTERFORMWIDTH - 2*DES_POWERFORMX)
#define DES_POWERFORMHEIGHT		40

#define DES_RIGHTFORMWIDTH	(RET_FORMWIDTH + 20)
#define DES_RIGHTFORMHEIGHT DES_LEFTFORMHEIGHT
#define	DES_RIGHTFORMBUTX	2
#define	DES_RIGHTFORMBUTY	2

#define DES_BARFORMX		6
#define DES_BARFORMY		6
#define	DES_BARFORMWIDTH	300
#define	DES_BARFORMHEIGHT	85

#define DES_NAMEBOXX		DES_3DVIEWX
#define DES_NAMEBOXY		6
#define	DES_NAMEBOXWIDTH	DES_CENTERFORMWIDTH - 2*DES_NAMEBOXX
#define	DES_NAMEBOXHEIGHT	14

/* The central boxes on the design screen */
#define DES_COMPBUTWIDTH	150
#define DES_COMPBUTHEIGHT	85

#define DES_POWERX				1
#define DES_POWERY				6
#define DES_POWERSEPARATIONX	4
#define DES_POWERSEPARATIONY	2

#define	DES_PARTSEPARATIONX		6
#define	DES_PARTSEPARATIONY		6

/* Positions of stuff on the clickable boxes (Design screen) */
#define DES_CLICKBARX			154
#define DES_CLICKBARY			7
#define DES_CLICKBARWIDTH		140
#define DES_CLICKBARHEIGHT		11
#define DES_CLICKGAP			9
#define DES_CLICKBARNAMEX		126
#define DES_CLICKBARNAMEWIDTH	20
#define DES_CLICKBARNAMEHEIGHT	19

#define DES_CLICKBARMAJORRED	255		//0xcc
#define DES_CLICKBARMAJORGREEN	235		//0
#define DES_CLICKBARMAJORBLUE	19		//0
#define DES_CLICKBARMINORRED	0x55
#define DES_CLICKBARMINORGREEN	0
#define DES_CLICKBARMINORBLUE	0

#define DES_WEAPONBUTTON_X		26
#define DES_SYSTEMBUTTON_X		68
#define DES_SYSTEMBUTTON_Y		10

// Stat bar y positions.
#define	DES_STATBAR_Y1	(DES_CLICKBARY)
#define	DES_STATBAR_Y2	(DES_CLICKBARY+DES_CLICKBARHEIGHT + DES_CLICKGAP)
#define	DES_STATBAR_Y3	(DES_CLICKBARY+(DES_CLICKBARHEIGHT + DES_CLICKGAP)*2)
#define	DES_STATBAR_Y4	(DES_CLICKBARY+(DES_CLICKBARHEIGHT + DES_CLICKGAP)*3)

/* the widget screen */
extern W_SCREEN		*psWScreen;

/* default droid design template */
static DROID_TEMPLATE sDefaultDesignTemplate;

static void desSetupDesignTemplates();
static void setDesignPauseState();
static void resetDesignPauseState();
static bool intAddTemplateButtons(ListTabWidget *templList, DROID_TEMPLATE *psSelected);
static void intDisplayDesignForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

/* Set the current mode of the design screen, and display the appropriate component lists */
static void intSetDesignMode(DES_COMPMODE newCompMode);
/* Set all the design bar graphs from a design template */
static void intSetDesignStats(DROID_TEMPLATE *psTemplate);
/* Set up the system clickable form of the design screen given a set of stats */
static bool intSetSystemForm(COMPONENT_STATS *psStats);
/* Set up the propulsion clickable form of the design screen given a set of stats */
static bool intSetPropulsionForm(PROPULSION_STATS *psStats);
/* Add the component tab form to the design screen */
static ListTabWidget *intAddComponentForm();
/* Add the template tab form to the design screen */
static bool intAddTemplateForm(DROID_TEMPLATE *psSelected);
/* Add the system buttons (weapons, command droid, etc) to the design screen */
static bool intAddSystemButtons(DES_COMPMODE mode);
/* Add the component buttons to the main tab of the system or component form */
static bool intAddComponentButtons(ListTabWidget *compList, COMPONENT_STATS *psStats, unsigned size, UBYTE *aAvailable, unsigned numEntries, unsigned compID);
/* Add the component buttons to the main tab of the component form */
static bool intAddExtraSystemButtons(ListTabWidget *compList, unsigned sensorIndex, unsigned ecmIndex, unsigned constIndex, unsigned repairIndex, unsigned brainIndex);
/* Set the bar graphs for the system clickable */
static void intSetSystemStats(COMPONENT_STATS *psStats);
/* Set the shadow bar graphs for the system clickable */
static void intSetSystemShadowStats(COMPONENT_STATS *psStats);
/* Set the bar graphs for the sensor stats */
static void intSetSensorStats(SENSOR_STATS *psStats);
/* Set the shadow bar graphs for the sensor stats */
static void intSetSensorShadowStats(SENSOR_STATS *psStats);
/* Set the bar graphs for the ECM stats */
static void intSetECMStats(ECM_STATS *psStats);
/* Set the shadow bar graphs for the ECM stats */
static void intSetECMShadowStats(ECM_STATS *psStats);
/* Set the bar graphs for the Repair stats */
static void intSetRepairStats(REPAIR_STATS *psStats);
/* Set the shadow bar graphs for the Repair stats */
static void intSetRepairShadowStats(REPAIR_STATS *psStats);
/* Set the bar graphs for the Constructor stats */
static void intSetConstructStats(CONSTRUCT_STATS *psStats);
/* Set the shadow bar graphs for the Constructor stats */
static void intSetConstructShadowStats(CONSTRUCT_STATS *psStats);
/* Set the bar graphs for the Weapon stats */
static void intSetWeaponStats(WEAPON_STATS *psStats);
/* Set the shadow bar graphs for the weapon stats */
static void intSetWeaponShadowStats(WEAPON_STATS *psStats);
/* Set the bar graphs for the Body stats */
static void intSetBodyStats(BODY_STATS *psStats);
/* Set the shadow bar graphs for the Body stats */
static void intSetBodyShadowStats(BODY_STATS *psStats);
/* Set the bar graphs for the Propulsion stats */
static void intSetPropulsionStats(PROPULSION_STATS *psStats);
/* Set the shadow bar graphs for the Propulsion stats */
static void intSetPropulsionShadowStats(PROPULSION_STATS *psStats);
/* Sets the Design Power Bar for a given Template */
static void intSetDesignPower(DROID_TEMPLATE *psTemplate);
/* Sets the Power shadow Bar for the current Template with new stat*/
static void intSetTemplatePowerShadowStats(COMPONENT_STATS *psStats);
/* Sets the Body Points Bar for a given Template */
static void intSetBodyPoints(DROID_TEMPLATE *psTemplate);
/* Sets the Body Points shadow Bar for the current Template with new stat*/
static void intSetTemplateBodyShadowStats(COMPONENT_STATS *psStats);
/* set flashing flag for button */
static void intSetButtonFlash(UDWORD id, bool bFlash);
/*Function to set the shadow bars for all the stats when the mouse is over
the Template buttons*/
static void runTemplateShadowStats(UDWORD id);

static bool intCheckValidWeaponForProp(void);

static bool checkTemplateIsVtol(DROID_TEMPLATE *psTemplate);

/* save the current Template if valid. Return true if stored */
static bool saveTemplate();

static void desCreateDefaultTemplate(void);

/**
 * Updates the status of the stored template toggle button.
 *
 * @param isStored If the template is stored or not.
 */
static void updateStoreButton(bool isStored);

/* The current name of the design */
static char			aCurrName[MAX_STR_LENGTH];

/* Store a list of component stats pointers for the design screen */
extern UDWORD			maxComponent;
extern UDWORD			numComponent;
extern COMPONENT_STATS	**apsComponentList;
extern UDWORD			maxExtraSys;
extern UDWORD			numExtraSys;
extern COMPONENT_STATS	**apsExtraSysList;

/* The button id of the component that is in the design */
static UDWORD			desCompID;

static UDWORD			droidTemplID;

/* The current design being edited on the design screen */
static DROID_TEMPLATE sCurrDesign;
static bool haveCurrentDesign = false;

static void intDisplayStatForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void intDisplayViewForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

extern bool bRender3DOnly;


/* Add the design widgets to the widget screen */
static bool _intAddDesign(bool bShowCentreScreen)
{
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;
	W_EDBINIT		sEdInit;
	W_BUTINIT		sButInit;
	W_BARINIT		sBarInit;

	desSetupDesignTemplates();

	//set which states are to be paused while design screen is up
	setDesignPauseState();

	if (GetGameMode() == GS_NORMAL && !bMultiPlayer)
	{
		bool radOnScreen = radarOnScreen;
		bRender3DOnly = true;
		radarOnScreen = false;
		// Just display the 3d, no interface
		displayWorld();
		radarOnScreen = radOnScreen;
		bRender3DOnly = false;
	}

	WIDGET *parent = psWScreen->psForm;

	/* Add the main design form */
	IntFormAnimated *desForm = new IntFormAnimated(parent, false);
	desForm->id = IDDES_FORM;
	desForm->setGeometry(DES_CENTERFORMX, DES_CENTERFORMY, DES_CENTERFORMWIDTH, DES_CENTERFORMHEIGHT);

	/* add the edit name box */
	sEdInit.formID = IDDES_FORM;
	sEdInit.id = IDDES_NAMEBOX;
	sEdInit.x = DES_NAMEBOXX;
	sEdInit.y = DES_NAMEBOXY;
	sEdInit.width = DES_NAMEBOXWIDTH;
	sEdInit.height = DES_NAMEBOXHEIGHT;
	sEdInit.pText = _("New Vehicle");
	sEdInit.pBoxDisplay = intDisplayEditBox;
	if (!widgAddEditBox(psWScreen, &sEdInit))
	{
		return false;
	}

	haveCurrentDesign = false;

	/* Initialise the current design */
	sDefaultDesignTemplate.droidType = DROID_ANY;
	sCurrDesign = sDefaultDesignTemplate;
	sCurrDesign.stored = false;
	sstrcpy(aCurrName, _("New Vehicle"));
	sCurrDesign.name = aCurrName;

	/* Add the design templates form */
	if (!intAddTemplateForm(NULL))  // Was psCurrTemplate instead of NULL, but psCurrTemplate was always NULL. Deleted psCurrTemplate, but leaving this here, in case intAddTemplateForm(NULL) does something useful.
	{
		return false;
	}

	/* Add the 3D View form */
	sFormInit.formID = IDDES_FORM;
	sFormInit.id = IDDES_3DVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = DES_3DVIEWX;
	sFormInit.y = DES_3DVIEWY;
	sFormInit.width = DES_3DVIEWWIDTH;
	sFormInit.height = DES_3DVIEWHEIGHT;
	sFormInit.pDisplay = intDisplayViewForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the part button form */
	sFormInit.formID = IDDES_FORM;
	sFormInit.id = IDDES_PARTFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = DES_PARTFORMX;
	sFormInit.y = DES_PARTFORMY;
	sFormInit.width = (UWORD)(iV_GetImageWidth(IntImages, IMAGE_DES_TURRET) +
	                          2 * DES_PARTSEPARATIONX);
	sFormInit.height = DES_PARTFORMHEIGHT;
	sFormInit.pDisplay = intDisplayDesignForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// add the body part button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_BODYBUTTON;
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = DES_PARTSEPARATIONY;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_BODY);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_BODY);
	sButInit.pTip = _("Vehicle Body");
	sButInit.pDisplay = intDisplayButtonFlash;
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_BODYH, IMAGE_DES_BODY);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the propulsion part button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_PROPBUTTON;
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
	                     2 * DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_PROPULSION);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION);
	sButInit.pTip = _("Vehicle Propulsion");
	sButInit.pDisplay = intDisplayButtonFlash;
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_PROPULSIONH, IMAGE_DES_PROPULSION);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the turret part button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_SYSTEMBUTTON;
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
	                     3 * DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_TURRET);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_TURRET);
	sButInit.pTip = _("Vehicle Turret");
	sButInit.pDisplay = intDisplayButtonFlash;
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_TURRETH, IMAGE_DES_TURRET);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the turret_a button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_WPABUTTON;
	sButInit.x = DES_PARTSEPARATIONX;
	// use BODY height for now
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
	                     4 * DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_TURRET);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_TURRET);
	sButInit.pTip = _("Vehicle Turret");
	sButInit.pDisplay = intDisplayButtonFlash;
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_TURRETH, IMAGE_DES_TURRET);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the turret_b button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_WPBBUTTON;
	sButInit.x = DES_PARTSEPARATIONX;
	//use body height for now
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
	                     5 * DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_TURRET);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_TURRET);
	sButInit.pTip = _("Vehicle Turret");
	sButInit.pDisplay = intDisplayButtonFlash;
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_TURRETH, IMAGE_DES_TURRET);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* add the delete button */
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_BIN;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_BIN);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_BIN);
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = (UWORD)(DES_PARTFORMHEIGHT - sButInit.height - DES_PARTSEPARATIONY);
	sButInit.pTip = _("Delete Design");
	sButInit.pDisplay = intDisplayButtonHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_DES_BINH, IMAGE_DES_BIN);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// Add the store template button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_STOREBUTTON;
	sButInit.style = WBUT_PLAIN;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_SAVE);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_SAVE);
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = DES_PARTFORMHEIGHT - 2 * sButInit.height - 2 * DES_PARTSEPARATIONY;
	sButInit.pTip = _("Store Template");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayButtonHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_DES_SAVEH, IMAGE_DES_SAVE);

	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* add central stats form */
	IntFormAnimated *statsForm = new IntFormAnimated(parent, false);
	statsForm->id = IDDES_STATSFORM;
	statsForm->setGeometry(DES_STATSFORMX, DES_STATSFORMY, DES_STATSFORMWIDTH, DES_STATSFORMHEIGHT);

	/* Add the body form */
	sFormInit.formID = IDDES_STATSFORM;
	sFormInit.id = IDDES_BODYFORM;
	sFormInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;
	sFormInit.width = DES_BARFORMWIDTH;
	sFormInit.height = DES_BARFORMHEIGHT;
	sFormInit.x = DES_BARFORMX;
	sFormInit.y = DES_BARFORMY;
	sFormInit.pDisplay = intDisplayStatForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the graphs for the Body */
	sBarInit.formID = IDDES_BODYFORM;
	sBarInit.id = IDDES_BODYARMOUR_K;
	sBarInit.x = DES_CLICKBARX;
	sBarInit.y = DES_STATBAR_Y1;	//DES_CLICKBARY;
	sBarInit.width = DES_CLICKBARWIDTH;
	sBarInit.height = DES_CLICKBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol.byte.r = DES_CLICKBARMAJORRED;
	sBarInit.sCol.byte.g = DES_CLICKBARMAJORGREEN;
	sBarInit.sCol.byte.b = DES_CLICKBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = DES_CLICKBARMINORRED;
	sBarInit.sMinorCol.byte.g = DES_CLICKBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = DES_CLICKBARMINORBLUE;
	sBarInit.pDisplay = intDisplayStatsBar;
	sBarInit.pTip = _("Kinetic Armour");
	sBarInit.iRange = getMaxBodyArmour();
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}

	sBarInit.id = IDDES_BODYARMOUR_H;
	sBarInit.y  = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sBarInit.pTip = _("Thermal Armour");
	sBarInit.iRange = getMaxBodyArmour();
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}

	sBarInit.id = IDDES_BODYPOWER;
	sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sBarInit.pTip = _("Engine Output");
	sBarInit.iRange = (UWORD)getMaxBodyPower();//DBAR_BODYMAXPOWER;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}
	sBarInit.id = IDDES_BODYWEIGHT;
	sBarInit.y = DES_STATBAR_Y4;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sBarInit.pTip = _("Weight");
	sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}

	/* Add the labels for the Body */
	sLabInit.formID = IDDES_BODYFORM;
	sLabInit.id = IDDES_BODYARMOURKLAB;
	sLabInit.x = DES_CLICKBARNAMEX;
	sLabInit.y = DES_CLICKBARY - DES_CLICKBARHEIGHT / 3;
	sLabInit.width = DES_CLICKBARNAMEWIDTH;
	sLabInit.height = DES_CLICKBARHEIGHT;
	sLabInit.pTip = _("Kinetic Armour");
	sLabInit.pDisplay = intDisplayImage;
	//just to confuse things even more - the graphics were named incorrectly!
	sLabInit.UserData = IMAGE_DES_ARMOUR_EXPLOSIVE;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}
	sLabInit.id = IDDES_BODYARMOURHLAB;
	sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sLabInit.pTip = _("Thermal Armour");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_ARMOUR_KINETIC;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}
	sLabInit.id = IDDES_BODYPOWERLAB;
	sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sLabInit.pTip = _("Engine Output");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_POWER;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}
	sLabInit.id = IDDES_BODYWEIGHTLAB;
	sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sLabInit.pTip = _("Weight");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_WEIGHT;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}

	/* add power/points bar subform */
	sFormInit = W_FORMINIT();
	sFormInit.formID = IDDES_FORM;
	sFormInit.id = IDDES_POWERFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = DES_POWERFORMX;
	sFormInit.y = DES_POWERFORMY;
	sFormInit.width = DES_POWERFORMWIDTH;
	sFormInit.height = DES_POWERFORMHEIGHT;
	sFormInit.pDisplay = intDisplayDesignForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the design template power bar and label*/
	sLabInit.formID	= IDDES_POWERFORM;
	sLabInit.id = IDDES_TEMPPOWERLAB;
	sLabInit.x = DES_POWERX;
	sLabInit.y = DES_POWERY;
	sLabInit.pTip = _("Total Power Required");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_POWER;
	widgAddLabel(psWScreen, &sLabInit);

	sBarInit = W_BARINIT();
	sBarInit.formID = IDDES_POWERFORM;
	sBarInit.id = IDDES_POWERBAR;
	sBarInit.x = (SWORD)(DES_POWERX + DES_POWERSEPARATIONX +
	                     iV_GetImageWidth(IntImages, IMAGE_DES_BODYPOINTS));
	sBarInit.y = DES_POWERY;
	sBarInit.width = (SWORD)(DES_POWERFORMWIDTH - 15 -
	                         iV_GetImageWidth(IntImages, IMAGE_DES_BODYPOINTS));
	sBarInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_POWERBACK);
	sBarInit.pDisplay = intDisplayDesignPowerBar;//intDisplayStatsBar;
	sBarInit.pTip = _("Total Power Required");
	sBarInit.iRange = DBAR_TEMPLATEMAXPOWER;//WBAR_SCALE;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return false;
	}

	/* Add the design template body points bar and label*/
	sLabInit.formID	= IDDES_POWERFORM;
	sLabInit.id = IDDES_TEMPBODYLAB;
	sLabInit.x = DES_POWERX;
	sLabInit.y = (SWORD)(DES_POWERY + DES_POWERSEPARATIONY +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODYPOINTS));
	sLabInit.pTip = _("Total Body Points");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_BODYPOINTS;
	widgAddLabel(psWScreen, &sLabInit);

	sBarInit = W_BARINIT();
	sBarInit.formID = IDDES_POWERFORM;
	sBarInit.id = IDDES_BODYPOINTS;
	sBarInit.x = (SWORD)(DES_POWERX + DES_POWERSEPARATIONX +
	                     iV_GetImageWidth(IntImages, IMAGE_DES_BODYPOINTS));
	sBarInit.y = (SWORD)(DES_POWERY + DES_POWERSEPARATIONY + 4 +
	                     iV_GetImageHeight(IntImages, IMAGE_DES_BODYPOINTS));
	sBarInit.width = (SWORD)(DES_POWERFORMWIDTH - 15 -
	                         iV_GetImageWidth(IntImages, IMAGE_DES_BODYPOINTS));
	sBarInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_POWERBACK);
	sBarInit.pDisplay = intDisplayDesignPowerBar;//intDisplayStatsBar;
	sBarInit.pTip = _("Total Body Points");
	sBarInit.iRange = DBAR_TEMPLATEMAXPOINTS;//(UWORD)getMaxBodyPoints();//DBAR_BODYMAXPOINTS;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return false;
	}

	/* Add the variable bits of the design screen and set the bar graphs */
	desCompMode = IDES_NOCOMPONENT;
	desSysMode = IDES_NOSYSTEM;
	desPropMode = IDES_NOPROPULSION;
	intSetDesignStats(&sCurrDesign);
	intSetBodyPoints(&sCurrDesign);
	intSetDesignPower(&sCurrDesign);
	intSetDesignMode(IDES_BODY);

	/* hide design and component forms until required */
	desForm->show(bShowCentreScreen);
	statsForm->hide();
	widgHide(psWScreen, IDDES_RIGHTBASE);

	return true;
}

/* set up droid templates before going into design screen */
void desSetupDesignTemplates(void)
{
	/* init template list */
	apsTemplateList.clear();
	apsTemplateList.push_back(&sDefaultDesignTemplate);
	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		DROID_TEMPLATE *psTempl = &*i;  // &* changes iterators into pointers.
		/* add template to list if not a transporter,
		 * cyborg, person or command droid,
		 */
		if (psTempl->droidType != DROID_TRANSPORTER        &&
		    psTempl->droidType != DROID_SUPERTRANSPORTER   &&
		    psTempl->droidType != DROID_CYBORG             &&
		    psTempl->droidType != DROID_CYBORG_SUPER       &&
		    psTempl->droidType != DROID_CYBORG_CONSTRUCT   &&
		    psTempl->droidType != DROID_CYBORG_REPAIR      &&
		    psTempl->droidType != DROID_PERSON             &&
		    researchedTemplate(psTempl, selectedPlayer))
		{
			apsTemplateList.push_back(psTempl);
		}
	}
}

/* Add the design template form */
static bool _intAddTemplateForm(DROID_TEMPLATE *psSelected)
{
	WIDGET *parent = psWScreen->psForm;

	/* add a form to place the tabbed form on */
	IntFormAnimated *templbaseForm = new IntFormAnimated(parent, false);
	templbaseForm->id = IDDES_TEMPLBASE;
	templbaseForm->setGeometry(RET_X, DESIGN_Y, RET_FORMWIDTH, DES_LEFTFORMHEIGHT + 4);

	/* Add the design templates form */
	IntListTabWidget *templList = new IntListTabWidget(templbaseForm);
	templList->setChildSize(DES_TABBUTWIDTH, DES_TABBUTHEIGHT);
	templList->setChildSpacing(DES_TABBUTGAP, DES_TABBUTGAP);
	int templListWidth = OBJ_BUTWIDTH*2 + DES_TABBUTGAP;
	templList->setGeometry((RET_FORMWIDTH - templListWidth)/2, 2, templListWidth, templbaseForm->height() - 2);

	/* Put the buttons on it */
	return intAddTemplateButtons(templList, psSelected);
}

/* Add the droid template buttons to a form */
static bool intAddTemplateButtons(ListTabWidget *templList, DROID_TEMPLATE *psSelected)
{
	DROID_TEMPLATE	*psTempl = NULL;
	char TempString[256];

	/* Set up the button struct */
	int nextButtonId = IDDES_TEMPLSTART;

	/* Add each button */
	W_BARINIT sBarInit;
	sBarInit.id = IDDES_BARSTART;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.y = STAT_TIMEBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;
	sBarInit.pTip = _("Power Usage");

	droidTemplID = 0;
	for (unsigned i = 0; i < apsTemplateList.size(); ++i)
	{
		psTempl = apsTemplateList[i];

		/* Set the tip and add the button */
		IntStatsButton *button = new IntStatsButton(templList);
		button->id = nextButtonId;
		button->setStatsAndTip(psTempl);
		templList->addWidgetToLayout(button);

		sBarInit.iRange = POWERPOINTS_DROIDDIV;
		sBarInit.size = calcTemplatePower(psTempl) / POWERPOINTS_DROIDDIV;
		if (sBarInit.size > WBAR_SCALE)
		{
			sBarInit.size = WBAR_SCALE;
		}

		ssprintf(TempString, "%s - %d", _("Power Usage"), calcTemplatePower(psTempl));
		sBarInit.pTip = TempString;
		sBarInit.formID = nextButtonId;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* if the current template matches psSelected lock the button */
		if (psTempl == psSelected)
		{
			droidTemplID = nextButtonId;
			button->setState(WBUT_LOCK);
			templList->setCurrentPage(templList->pages() - 1);
		}

		/* Update the init struct for the next button */
		sBarInit.id += 1;
		++nextButtonId;
		//check don't go over max templates that can fit on the form
		if (nextButtonId >= IDDES_TEMPLEND)
		{
			break;
		}
	}

	return true;
}


/* Set the current mode of the design screen, and display the appropriate
 * component lists
 * added case IDES_TURRET_A,IDES_TURRET_B
 */
static void intSetDesignMode(DES_COMPMODE newCompMode)
{
	UDWORD	weaponIndex;

	if (newCompMode == desCompMode)
	{
		return;
	}
	/* Have to change the component display - remove the old one */
	if (desCompMode != IDES_NOCOMPONENT)
	{
		widgDelete(psWScreen, IDDES_RIGHTBASE);

		widgSetButtonState(psWScreen, IDDES_BODYFORM, 0);
		widgSetButtonState(psWScreen, IDDES_PROPFORM, 0);
		widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, 0);
		widgHide(psWScreen, IDDES_BODYFORM);
		widgHide(psWScreen, IDDES_PROPFORM);
		widgHide(psWScreen, IDDES_SYSTEMFORM);

		widgSetButtonState(psWScreen, IDDES_BODYBUTTON, 0);
		widgSetButtonState(psWScreen, IDDES_PROPBUTTON, 0);
		widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, 0);
		widgSetButtonState(psWScreen, IDDES_WPABUTTON, 0);
		widgSetButtonState(psWScreen, IDDES_WPBBUTTON, 0);
	}

	ListTabWidget *compList;

	/* Set up the display for the new mode */
	desCompMode = newCompMode;
	switch (desCompMode)
	{
	case IDES_NOCOMPONENT:
		/* Nothing to display */
		break;
	case IDES_SYSTEM:
		compList = intAddComponentForm();
		intAddExtraSystemButtons(compList,
		                         sCurrDesign.asParts[COMP_SENSOR],
		                         sCurrDesign.asParts[COMP_ECM],
		                         sCurrDesign.asParts[COMP_CONSTRUCT],
		                         sCurrDesign.asParts[COMP_REPAIRUNIT],
		                         sCurrDesign.asParts[COMP_BRAIN]);
		intAddSystemButtons(IDES_SYSTEM);
		widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
		widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, WBUT_CLICKLOCK);
		widgReveal(psWScreen, IDDES_SYSTEMFORM);
		break;
	case IDES_TURRET:
		compList = intAddComponentForm();
		weaponIndex = (sCurrDesign.numWeaps > 0) ? sCurrDesign.asWeaps[0] : 0;
		intAddComponentButtons(compList, asWeaponStats, sizeof(*asWeaponStats), apCompLists[selectedPlayer][COMP_WEAPON], numWeaponStats, weaponIndex);
		intAddSystemButtons(IDES_TURRET);
		widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
		widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, WBUT_CLICKLOCK);
		widgReveal(psWScreen, IDDES_SYSTEMFORM);
		intSetSystemForm((COMPONENT_STATS *)(asWeaponStats + sCurrDesign.asWeaps[0])); // in case previous was a different slot
		break;
	case IDES_BODY:
		compList = intAddComponentForm();
		intAddComponentButtons(compList, asBodyStats, sizeof(*asBodyStats), apCompLists[selectedPlayer][COMP_BODY], numBodyStats, sCurrDesign.asParts[COMP_BODY]);
		widgSetButtonState(psWScreen, IDDES_BODYFORM, WBUT_LOCK);
		widgSetButtonState(psWScreen, IDDES_BODYBUTTON, WBUT_CLICKLOCK);
		widgReveal(psWScreen, IDDES_BODYFORM);
		break;
	case IDES_PROPULSION:
		compList = intAddComponentForm();
		intAddComponentButtons(compList, asPropulsionStats, sizeof(*asPropulsionStats), apCompLists[selectedPlayer][COMP_PROPULSION], numPropulsionStats, sCurrDesign.asParts[COMP_PROPULSION]);
		widgSetButtonState(psWScreen, IDDES_PROPFORM, WBUT_LOCK);
		widgSetButtonState(psWScreen, IDDES_PROPBUTTON, WBUT_CLICKLOCK);
		widgReveal(psWScreen, IDDES_PROPFORM);
		break;
	case IDES_TURRET_A:
		compList = intAddComponentForm();
		weaponIndex = (sCurrDesign.numWeaps > 1) ? sCurrDesign.asWeaps[1] : 0;
		intAddComponentButtons(compList, asWeaponStats, sizeof(*asWeaponStats), apCompLists[selectedPlayer][COMP_WEAPON], numWeaponStats, weaponIndex);
		intAddSystemButtons(IDES_TURRET_A);
		widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
		widgSetButtonState(psWScreen, IDDES_WPABUTTON, WBUT_CLICKLOCK);
		widgReveal(psWScreen, IDDES_SYSTEMFORM);
		intSetSystemForm((COMPONENT_STATS *)(asWeaponStats + sCurrDesign.asWeaps[1])); // in case previous was a different slot
		// Stop the button flashing
		intSetButtonFlash(IDDES_WPABUTTON,   false);
		break;
	case IDES_TURRET_B:
		compList = intAddComponentForm();
		weaponIndex = (sCurrDesign.numWeaps > 2) ? sCurrDesign.asWeaps[2] : 0;
		intAddComponentButtons(compList, asWeaponStats, sizeof(*asWeaponStats), apCompLists[selectedPlayer][COMP_WEAPON], numWeaponStats, weaponIndex);
		intAddSystemButtons(IDES_TURRET_B);
		widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
		widgSetButtonState(psWScreen, IDDES_WPBBUTTON, WBUT_CLICKLOCK);
		widgReveal(psWScreen, IDDES_SYSTEMFORM);
		intSetSystemForm((COMPONENT_STATS *)(asWeaponStats + sCurrDesign.asWeaps[2])); // in case previous was a different slot
		// Stop the button flashing
		intSetButtonFlash(IDDES_WPBBUTTON,   false);
		break;
	}
}

static COMPONENT_STATS *
intChooseSystemStats(DROID_TEMPLATE *psTemplate)
{
	COMPONENT_STATS		*psStats = NULL;
	int compIndex;

	// Choose correct system stats
	switch (droidTemplateType(psTemplate))
	{
	case DROID_COMMAND:
		compIndex = psTemplate->asParts[COMP_BRAIN];
		ASSERT_OR_RETURN(NULL, compIndex < numBrainStats, "Invalid range referenced for numBrainStats, %d > %d", compIndex, numBrainStats);
		psStats = (COMPONENT_STATS *)(asBrainStats + compIndex);
		break;
	case DROID_SENSOR:
		compIndex = psTemplate->asParts[COMP_SENSOR];
		ASSERT_OR_RETURN(NULL, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = (COMPONENT_STATS *)(asSensorStats + compIndex);
		break;
	case DROID_ECM:
		compIndex = psTemplate->asParts[COMP_ECM];
		ASSERT_OR_RETURN(NULL, compIndex < numECMStats, "Invalid range referenced for numECMStats, %d > %d", compIndex, numECMStats);
		psStats = (COMPONENT_STATS *)(asECMStats + compIndex);
		break;
	case DROID_CONSTRUCT:
	case DROID_CYBORG_CONSTRUCT:
		compIndex = psTemplate->asParts[COMP_CONSTRUCT];
		ASSERT_OR_RETURN(NULL, compIndex < numConstructStats, "Invalid range referenced for numConstructStats, %d > %d", compIndex, numConstructStats);
		psStats = (COMPONENT_STATS *)(asConstructStats + compIndex);
		break;
	case DROID_REPAIR:
	case DROID_CYBORG_REPAIR:
		compIndex = psTemplate->asParts[COMP_REPAIRUNIT];
		ASSERT_OR_RETURN(NULL, compIndex < numRepairStats, "Invalid range referenced for numRepairStats, %d > %d", compIndex, numRepairStats);
		psStats = (COMPONENT_STATS *)(asRepairStats + compIndex);
		break;
	case DROID_WEAPON:
	case DROID_PERSON:
	case DROID_CYBORG:
	case DROID_CYBORG_SUPER:
	case DROID_DEFAULT:
		compIndex = psTemplate->asWeaps[0];
		ASSERT_OR_RETURN(NULL, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
		psStats = (COMPONENT_STATS *)(asWeaponStats + compIndex);
		break;
	default:
		debug(LOG_ERROR, "unrecognised droid type");
		return NULL;
	}

	return psStats;
}

const char *GetDefaultTemplateName(DROID_TEMPLATE *psTemplate)
{
	// NOTE:	At this time, savegames can support a max of 60. We are using MAX_STR_LENGTH (currently 256) for display
	// We are also returning a truncated string, instead of NULL if the string is too long.
	COMPONENT_STATS *psStats = NULL;
	int compIndex;

	/*
		First we check for the special cases of the Transporter & Cyborgs
	*/
	if (psTemplate->droidType == DROID_TRANSPORTER)
	{
		sstrcpy(aCurrName, _("Transport"));
		return aCurrName;
	}
	if (psTemplate->droidType == DROID_SUPERTRANSPORTER)
	{
		sstrcpy(aCurrName, _("Super Transport"));
		return aCurrName;
	}

	/*
		Now get the normal default droid name based on its components
	*/
	aCurrName[0] = '\0'; // Reset string to null
	psStats = intChooseSystemStats(psTemplate);
	if (psTemplate->asWeaps[0]					!= 0 ||
	    psTemplate->asParts[COMP_CONSTRUCT]	!= 0 ||
	    psTemplate->asParts[COMP_SENSOR]		!= 0 ||
	    psTemplate->asParts[COMP_ECM]			!= 0 ||
	    psTemplate->asParts[COMP_REPAIRUNIT]   != 0 ||
	    psTemplate->asParts[COMP_BRAIN]		!= 0)
	{
		sstrcpy(aCurrName, getName(psStats));
		sstrcat(aCurrName, " ");
	}

	if (psTemplate->numWeaps > 1)
	{
		sstrcat(aCurrName, _("Hydra "));
	}

	compIndex = psTemplate->asParts[COMP_BODY];
	ASSERT_OR_RETURN("", compIndex < numBodyStats, "Invalid range referenced for numBodyStats, %d > %d", compIndex, numBodyStats);
	psStats = (COMPONENT_STATS *)(asBodyStats + compIndex);
	if (psTemplate->asParts[COMP_BODY] != 0)
	{
		if (strlen(aCurrName) + psStats->name.size() > MAX_STR_LENGTH)
		{
			debug(LOG_ERROR, "Name string too long %s+%s > %u", aCurrName, getName(psStats), MAX_STR_LENGTH);
			debug(LOG_ERROR, "Please report what language you are using in the bug report!");
		}

		sstrcat(aCurrName, getName(psStats));
		sstrcat(aCurrName, " ");
	}

	compIndex = psTemplate->asParts[COMP_PROPULSION];
	ASSERT_OR_RETURN("", compIndex < numPropulsionStats, "Invalid range referenced for numPropulsionStats, %d > %d", compIndex, numPropulsionStats);
	psStats = (COMPONENT_STATS *)(asPropulsionStats + compIndex);
	if (psTemplate->asParts[COMP_PROPULSION] != 0)
	{
		if (strlen(aCurrName) + psStats->name.size() > MAX_STR_LENGTH)
		{
			debug(LOG_ERROR, "Name string too long %s+%s", aCurrName, getName(psStats));
			debug(LOG_ERROR, "Please report what language you are using in the bug report!");
		}

		sstrcat(aCurrName, getName(psStats));
	}

	return aCurrName;
}

static void intSetEditBoxTextFromTemplate(DROID_TEMPLATE *psTemplate)
{
	sstrcpy(aCurrName, "");

	/* show component names if default template else show stat name */
	if (psTemplate->droidType != DROID_DEFAULT)
	{
		sstrcpy(aCurrName, getName(psTemplate));
	}
	else
	{
		GetDefaultTemplateName(psTemplate);	// sets aCurrName
	}

	widgSetString(psWScreen, IDDES_NAMEBOX, aCurrName);
}

/* Set all the design bar graphs from a design template */
static void intSetDesignStats(DROID_TEMPLATE *psTemplate)
{
	COMPONENT_STATS		*psStats = intChooseSystemStats(psTemplate);

	/* Set system stats */
	intSetSystemForm(psStats);

	/* Set the body stats */
	intSetBodyStats(asBodyStats + psTemplate->asParts[COMP_BODY]);

	/* Set the propulsion stats */
	intSetPropulsionForm(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION]);

	/* Set the name in the edit box */
	intSetEditBoxTextFromTemplate(psTemplate);
}

/* Set up the system clickable form of the design screen given a set of stats */
static bool _intSetSystemForm(COMPONENT_STATS *psStats)
{
	DES_SYSMODE		newSysMode = (DES_SYSMODE)0;

	/* Figure out what the new mode should be */
	switch (psStats->compType)
	{
	case COMP_WEAPON:
		newSysMode = IDES_WEAPON;
		break;
	case COMP_SENSOR:
		newSysMode = IDES_SENSOR;
		break;
	case COMP_ECM:
		newSysMode = IDES_ECM;
		break;
	case COMP_CONSTRUCT:
		newSysMode = IDES_CONSTRUCT;
		break;
	case COMP_BRAIN:
		newSysMode = IDES_COMMAND;
		break;
	case COMP_REPAIRUNIT:
		newSysMode = IDES_REPAIR;
		break;
	default:
		ASSERT(false, "Bad choice");
	}

	/* If the correct form is already displayed just set the stats */
	if (newSysMode == desSysMode)
	{
		intSetSystemStats(psStats);

		return true;
	}

	/* Remove the old form if necessary */
	if (desSysMode != IDES_NOSYSTEM)
	{
		widgDelete(psWScreen, IDDES_SYSTEMFORM);
	}

	/* Set the new mode */
	desSysMode = newSysMode;

	/* Add the system form */
	W_FORMINIT sFormInit;
	sFormInit.formID = IDDES_STATSFORM;
	sFormInit.id = IDDES_SYSTEMFORM;
	sFormInit.style = (WFORM_CLICKABLE | WFORM_NOCLICKMOVE);
	sFormInit.x = DES_BARFORMX;
	sFormInit.y = DES_BARFORMY;
	sFormInit.width = DES_BARFORMWIDTH;	//COMPBUTWIDTH;
	sFormInit.height = DES_BARFORMHEIGHT;	//COMPBUTHEIGHT;
	sFormInit.pTip = getStatName(psStats);	/* set form tip to stats string */
	sFormInit.pUserData = psStats;			/* store component stats */
	sFormInit.pDisplay = intDisplayStatForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Initialise the bargraph struct */
	W_BARINIT sBarInit;
	sBarInit.formID = IDDES_SYSTEMFORM;
	//sBarInit.style = WBAR_DOUBLE;
	sBarInit.x = DES_CLICKBARX;
	sBarInit.y = DES_STATBAR_Y1;	//DES_CLICKBARY;
	sBarInit.width = DES_CLICKBARWIDTH;
	sBarInit.height = DES_CLICKBARHEIGHT;
	sBarInit.sCol.byte.r = DES_CLICKBARMAJORRED;
	sBarInit.sCol.byte.g = DES_CLICKBARMAJORGREEN;
	sBarInit.sCol.byte.b = DES_CLICKBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = DES_CLICKBARMINORRED;
	sBarInit.sMinorCol.byte.g = DES_CLICKBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = DES_CLICKBARMINORBLUE;
	sBarInit.pDisplay = intDisplayStatsBar;

	/* Initialise the label struct */
	W_LABINIT sLabInit;
	sLabInit.formID = IDDES_SYSTEMFORM;
	sLabInit.x = DES_CLICKBARNAMEX;
	sLabInit.y = DES_CLICKBARY - DES_CLICKBARHEIGHT / 3;
	sLabInit.width = DES_CLICKBARNAMEWIDTH;
	sLabInit.height = DES_CLICKBARHEIGHT;
	sLabInit.pDisplay = intDisplayImage;

	/* See what type of system stats we've got */
	if (psStats->ref >= REF_SENSOR_START && psStats->ref < REF_SENSOR_START + REF_RANGE)
	{
		/* Add the bar graphs*/
		sBarInit.id = IDDES_SENSORRANGE;
		sBarInit.iRange = (UWORD)getMaxSensorRange();//DBAR_SENSORMAXRANGE;
		sBarInit.pTip = _("Sensor Range");
		sBarInit.denominator = TILE_UNITS;
		sBarInit.precision = 1;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.denominator = 0;
		sBarInit.precision = 0;
		sBarInit.id = IDDES_SENSORPOWER;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UDWORD)getMaxSensorRange();	// FIXME: Remove
		sBarInit.pTip = _("Sensor Power");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_SENSORWEIGHT;
		sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		sBarInit.pTip = _("Weight");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_SENSORRANGELAB;
		sLabInit.pTip = _("Sensor Range");
		sLabInit.UserData = IMAGE_DES_RANGE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_SENSORPOWERLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Sensor Power");
		sLabInit.UserData = IMAGE_DES_POWER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_SENSORWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_ECM_START && psStats->ref < REF_ECM_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_ECMPOWER;
		sBarInit.iRange = (UWORD)getMaxECMRange();
		sBarInit.pTip = _("ECM Power");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_ECMWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		sBarInit.pTip = _("Weight");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_ECMPOWERLAB;
		sLabInit.pTip = _("ECM Power");
		sLabInit.UserData = IMAGE_DES_POWER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_ECMWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_CONSTRUCT_START && psStats->ref < REF_CONSTRUCT_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_CONSTPOINTS;
		sBarInit.pTip = _("Build Points");
		sBarInit.iRange = (UWORD)getMaxConstPoints();//DBAR_CONSTMAXPOINTS;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_CONSTWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Weight");
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_CONSTPOINTSLAB;
		sLabInit.pTip = _("Build Points");
		sLabInit.UserData = IMAGE_DES_BUILDRATE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_CONSTWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_REPAIR_START && psStats->ref < REF_REPAIR_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_REPAIRPOINTS;
		sBarInit.pTip = _("Build Points");
		sBarInit.iRange = (UWORD)getMaxRepairPoints();//DBAR_REPAIRMAXPOINTS;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_REPAIRWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Weight");
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_REPAIRPTLAB;
		sLabInit.pTip = _("Build Points");
		sLabInit.UserData = IMAGE_DES_BUILDRATE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_REPAIRWGTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_WEAPON_START && psStats->ref < REF_WEAPON_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_WEAPRANGE;
		sBarInit.iRange = (UWORD)getMaxWeaponRange();//DBAR_WEAPMAXRANGE;
		sBarInit.pTip = _("Range");
		sBarInit.denominator = TILE_UNITS;
		sBarInit.precision = 1;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.denominator = 1;
		sBarInit.precision = 0;
		sBarInit.id = IDDES_WEAPDAMAGE;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxWeaponDamage();//DBAR_WEAPMAXDAMAGE;
		sBarInit.pTip = _("Damage");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_WEAPROF;
		sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = getMaxWeaponROF();
		sBarInit.pTip = _("Rate-of-Fire");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_WEAPWEIGHT;
		sBarInit.y = DES_STATBAR_Y4;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		sBarInit.pTip = _("Weight");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_WEAPRANGELAB;
		sLabInit.pTip = _("Range");
		sLabInit.UserData = IMAGE_DES_RANGE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_WEAPDAMAGELAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Damage");
		sLabInit.UserData = IMAGE_DES_DAMAGE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_WEAPROFLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Rate-of-Fire");
		sLabInit.UserData = IMAGE_DES_FIRERATE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_WEAPWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}

	// Add the correct component form
	switch (desSysMode)
	{
	case IDES_SENSOR:
	case IDES_CONSTRUCT:
	case IDES_ECM:
	case IDES_REPAIR:
	case IDES_COMMAND:
		intSetDesignMode(IDES_SYSTEM);
		break;
	case IDES_WEAPON:
		intSetDesignMode(IDES_TURRET);
		break;
	default:
		break;
	}

	/* Set the stats */
	intSetSystemStats(psStats);

	/* Lock the form down if necessary */
	if (desCompMode == IDES_SYSTEM)
	{
		widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
	}

	return true;
}


/* Set up the propulsion clickable form of the design screen given a set of stats */
static bool intSetPropulsionForm(PROPULSION_STATS *psStats)
{
	DES_PROPMODE	newPropMode = (DES_PROPMODE)0;

	ASSERT_OR_RETURN(false, psStats != NULL, "Invalid propulsion stats pointer");

	/* figure out what the new mode should be */
	switch (asPropulsionTypes[psStats->propulsionType].travel)
	{
	case GROUND:
		newPropMode = IDES_GROUND;
		break;
	case AIR:
		newPropMode = IDES_AIR;
		break;
	}

	/* If the mode hasn't changed, just set the stats */
	if (desPropMode == newPropMode)
	{
		intSetPropulsionStats(psStats);
		return true;
	}

	/* Remove the old form if necessary */
	if (desPropMode != IDES_NOPROPULSION)
	{
		widgDelete(psWScreen, IDDES_PROPFORM);
	}

	/* Set the new mode */
	desPropMode = newPropMode;

	/* Add the propulsion form */
	W_FORMINIT sFormInit;
	sFormInit.formID = IDDES_STATSFORM;
	sFormInit.id = IDDES_PROPFORM;
	sFormInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;
	sFormInit.x = DES_BARFORMX;
	sFormInit.y = DES_BARFORMY;
	sFormInit.width = DES_BARFORMWIDTH;	//DES_COMPBUTWIDTH;
	sFormInit.height = DES_BARFORMHEIGHT;	//DES_COMPBUTHEIGHT;
	sFormInit.pTip = getStatName(psStats);	/* set form tip to stats string */
	sFormInit.pDisplay = intDisplayStatForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Initialise the bargraph struct */
	W_BARINIT sBarInit;
	sBarInit.formID = IDDES_PROPFORM;
	//sBarInit.style = WBAR_DOUBLE;
	sBarInit.x = DES_CLICKBARX;
	sBarInit.y = DES_STATBAR_Y1;	//DES_CLICKBARY;
	sBarInit.width = DES_CLICKBARWIDTH;
	sBarInit.height = DES_CLICKBARHEIGHT;
	sBarInit.sCol.byte.r = DES_CLICKBARMAJORRED;
	sBarInit.sCol.byte.g = DES_CLICKBARMAJORGREEN;
	sBarInit.sCol.byte.b = DES_CLICKBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = DES_CLICKBARMINORRED;
	sBarInit.sMinorCol.byte.g = DES_CLICKBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = DES_CLICKBARMINORBLUE;
	sBarInit.pDisplay = intDisplayStatsBar;

	/* Initialise the label struct */
	W_LABINIT sLabInit;
	sLabInit.formID = IDDES_PROPFORM;
	sLabInit.x = DES_CLICKBARNAMEX;
	sLabInit.y = DES_CLICKBARY - DES_CLICKBARHEIGHT / 3;
	sLabInit.width = DES_CLICKBARNAMEWIDTH;
	sLabInit.height = DES_CLICKBARNAMEHEIGHT;	//DES_CLICKBARHEIGHT;
	sLabInit.pDisplay = intDisplayImage;

	/* See what type of propulsion we've got */
	switch (desPropMode)
	{
	case IDES_AIR:
		/* Add the bar graphs */
		sBarInit.id = IDDES_PROPAIR;
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		sBarInit.pTip = _("Air Speed");
		sBarInit.denominator = TILE_UNITS;
		sBarInit.precision = 2;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.denominator = 1;
		sBarInit.precision = 0;
		sBarInit.id = IDDES_PROPWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		sBarInit.pTip = _("Weight");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_PROPAIRLAB;
		sLabInit.pTip = _("Air Speed");
		sLabInit.UserData = IMAGE_DES_HOVER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		break;
	case IDES_GROUND:
		/* Add the bar graphs */
		sBarInit.id = IDDES_PROPROAD;
		sBarInit.pTip = _("Road Speed");
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		sBarInit.denominator = TILE_UNITS;
		sBarInit.precision = 2;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_PROPCOUNTRY;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Off-Road Speed");
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_PROPWATER;
		sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Water Speed");
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.denominator = 1;
		sBarInit.precision = 0;
		sBarInit.id = IDDES_PROPWEIGHT;
		sBarInit.y = DES_STATBAR_Y4;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Weight");
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_PROPROADLAB;
		sLabInit.pTip = _("Road Speed");
		sLabInit.UserData = IMAGE_DES_ROAD;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPCOUNTRYLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Off-Road Speed");
		sLabInit.UserData = IMAGE_DES_CROSSCOUNTRY;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPWATERLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Water Speed");
		sLabInit.UserData = IMAGE_DES_HOVER;	//WATER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		break;
	default:
		break;
	}

	/* Set the stats */
	intSetPropulsionStats(psStats);

	/* Lock the form down if necessary */
	if (desCompMode == IDES_PROPULSION)
	{
		widgSetButtonState(psWScreen, IDDES_PROPFORM, WBUT_LOCK);
	}

	return true;
}

/* Add the component tab form to the design screen */
static ListTabWidget *intAddComponentForm()
{
	WIDGET *parent = psWScreen->psForm;

	/* add a form to place the tabbed form on */
	IntFormAnimated *rightBase = new IntFormAnimated(parent, false);
	rightBase->id = IDDES_RIGHTBASE;
	rightBase->setGeometry(RADTLX - 2, DESIGN_Y, RET_FORMWIDTH, DES_RIGHTFORMHEIGHT + 4);

	//now a single form
	IntListTabWidget *compList = new IntListTabWidget(rightBase);
	compList->setChildSize(DES_TABBUTWIDTH, DES_TABBUTHEIGHT);
	compList->setChildSpacing(DES_TABBUTGAP, DES_TABBUTGAP);
	int objListWidth = DES_TABBUTWIDTH*2 + DES_TABBUTGAP;
	compList->setGeometry((rightBase->width() - objListWidth)/2, 40, objListWidth, rightBase->height() - 40);
	return compList;
}

/* Add the system buttons (weapons, command droid, etc) to the design screen */
static bool intAddSystemButtons(DES_COMPMODE mode)
{
	// add the weapon button
	W_BUTINIT sButInit;
	sButInit.formID = IDDES_RIGHTBASE;
	sButInit.id = IDDES_WEAPONS;
	sButInit.x = DES_WEAPONBUTTON_X;
	sButInit.y = DES_SYSTEMBUTTON_Y;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_WEAPONS);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_WEAPONS);
	sButInit.pTip = _("Weapons");
	sButInit.pDisplay = intDisplayButtonHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_DES_EXTRAHI , IMAGE_DES_WEAPONS);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// if currently got a VTOL proplusion attached then don't add the system buttons
	// dont add the system button if mode is IDES_TURRET_A or IDES_TURRET_B
	if (!checkTemplateIsVtol(&sCurrDesign) && mode != IDES_TURRET_A && mode != IDES_TURRET_B)
	{
		// add the system button
		sButInit.formID = IDDES_RIGHTBASE;
		sButInit.id = IDDES_SYSTEMS;
		sButInit.x = DES_SYSTEMBUTTON_X;
		sButInit.y = DES_SYSTEMBUTTON_Y;
		sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_SYSTEMS);
		sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_SYSTEMS);
		sButInit.pTip = _("Systems");
		sButInit.pDisplay = intDisplayButtonHilight;
		sButInit.UserData = PACKDWORD_TRI(0, IMAGE_DES_EXTRAHI , IMAGE_DES_SYSTEMS);
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}
		if (mode == IDES_SYSTEM)
		{
			widgSetButtonState(psWScreen, IDDES_SYSTEMS, WBUT_LOCK);
		}
	}

	// lock down the correct button
	switch (mode)
	{
	case IDES_TURRET:
	case IDES_TURRET_A:
	case IDES_TURRET_B:
		widgSetButtonState(psWScreen, IDDES_WEAPONS, WBUT_LOCK);
		break;
	case IDES_SYSTEM:
		break;
	default:
		ASSERT(!"invalid/unexpected mode", "unexpected mode");
		break;
	}

	return true;
}


/* Add the component buttons to the main tab of the component form */
static bool intAddComponentButtons(ListTabWidget *compList, COMPONENT_STATS *psStats, unsigned size, UBYTE *aAvailable, unsigned numEntries, unsigned compID)
{
	UDWORD				i, maxComponents;
	COMPONENT_STATS		*psCurrStats;
	PROPULSION_STATS	*psPropStats;
	bool				bVTol, bWeapon;
	int bodysize = SIZE_NUM;

	/* Set up the button struct */
	int nextButtonId = IDDES_COMPSTART;

	//need to set max number of buttons possible
	if (psStats->ref >= REF_WEAPON_START && psStats->ref < REF_WEAPON_START + REF_RANGE)
	{
		maxComponents = MAX_SYSTEM_COMPONENTS;
	}
	else
	{
		maxComponents = MAX_DESIGN_COMPONENTS;
	}

	/*if adding weapons - need to check if the propulsion is a VTOL*/
	bVTol = false;

	if (psStats->ref >= REF_WEAPON_START && psStats->ref < REF_WEAPON_START + REF_RANGE)
	{
		bWeapon = true;
	}
	else
	{
		bWeapon = false;
	}

	if (bWeapon)
	{
		//check if the current Template propulsion has been set
		if (sCurrDesign.asParts[COMP_PROPULSION])
		{
			psPropStats = asPropulsionStats + sCurrDesign.
			              asParts[COMP_PROPULSION];
			ASSERT_OR_RETURN(false, psPropStats != NULL, "invalid propulsion stats pointer");

			if (asPropulsionTypes[psPropStats->propulsionType].travel == AIR)
			{
				bVTol = true;
			}
		}
		if (sCurrDesign.asParts[COMP_BODY])
		{
			bodysize = (asBodyStats + sCurrDesign.asParts[COMP_BODY])->size;
		}
	}

	/* Add each button */
	desCompID = 0;
	numComponent = 0;
	psCurrStats = psStats;
	for (i = 0; i < numEntries; i++)
	{
		/* If we are out of space in the list - stop */
		if (numComponent >= maxComponents)
		{
			break;
		}

		/* Skip unavailable entries and non-design ones*/
		if (!(aAvailable[i] & AVAILABLE) || !psCurrStats->designable)
		{
			/* Update the stats pointer for the next button */
			psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);
			continue;
		}

		/*skip indirect weapons if VTOL propulsion or numVTOLattackRuns for the weapon is zero*/
		if (bWeapon)
		{
			WEAPON_STATS *psWeapon = (WEAPON_STATS *)psCurrStats;
			if ((psWeapon->vtolAttackRuns > 0) != bVTol
			    || (psWeapon->weaponSize == WEAPON_SIZE_LIGHT && bodysize != SIZE_LIGHT)
			    || (psWeapon->weaponSize == WEAPON_SIZE_HEAVY && bodysize == SIZE_LIGHT))
			{
				/* Update the stats pointer for the next button */
				psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);
				continue;
			}
		}

		/* Set the tip and add the button */
		IntStatsButton *button = new IntStatsButton(compList);
		button->id = nextButtonId;
		button->setStatsAndTip(psCurrStats);
		compList->addWidgetToLayout(button);

		/* Store the stat pointer in the list */
		apsComponentList[numComponent++] = psCurrStats;

		/* If this matches the component ID lock the button */
		if (i == compID)
		{
			desCompID = nextButtonId;
			button->setState(WBUT_LOCK);
			compList->setCurrentPage(compList->pages() - 1);
		}

		// if this is a command droid that is in use or dead - make it unavailable
		if (psCurrStats->compType == COMP_BRAIN)
		{
			if ((((COMMAND_DROID *)psCurrStats)->psDroid != NULL) ||
			    ((COMMAND_DROID *)psCurrStats)->died)
			{
				button->setState(WBUT_DISABLE);
			}
		}

		/* Update the init struct for the next button */
		++nextButtonId;

		/* Update the stats pointer for the next button */
		psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);
	}

	return true;
}

/* Add the component buttons to the main tab of the component form */
static bool intAddExtraSystemButtons(ListTabWidget *compList, unsigned sensorIndex, unsigned ecmIndex, unsigned constIndex, unsigned repairIndex, unsigned brainIndex)
{
	UDWORD			i, buttonType, size = 0;
	UDWORD			compIndex = 0, numStats = 0;
	COMPONENT_STATS	*psCurrStats = 0;
	UBYTE			*aAvailable = 0;

	// Set up the button struct
	int nextButtonId = IDDES_EXTRASYSSTART;

	// Add the buttons :
	// buttonType == 0  -  Sensor Buttons
	// buttonType == 1  -  ECM Buttons
	// buttonType == 2  -  Constructor Buttons
	// buttonType == 3  -  Repair Buttons
	// buttonType == 4  -  Brain Buttons
	numExtraSys = 0;
	for (buttonType = 0; buttonType < 5; buttonType++)
	{
		switch (buttonType)
		{
		case 0:
			// Sensor Buttons
			psCurrStats = (COMPONENT_STATS *)asSensorStats;
			size = sizeof(SENSOR_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_SENSOR];
			numStats = numSensorStats;
			compIndex = sensorIndex;
			break;
		case 1:
			// ECM Buttons
			psCurrStats = (COMPONENT_STATS *)asECMStats;
			size = sizeof(ECM_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_ECM];
			numStats = numECMStats;
			compIndex = ecmIndex;
			break;
		case 2:
			// Constructor Buttons
			psCurrStats = (COMPONENT_STATS *)asConstructStats;
			size = sizeof(CONSTRUCT_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_CONSTRUCT];
			numStats = numConstructStats;
			compIndex = constIndex;
			break;
		case 3:
			// Repair Buttons
			psCurrStats = (COMPONENT_STATS *)asRepairStats;
			size = sizeof(REPAIR_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_REPAIRUNIT];
			numStats = numRepairStats;
			compIndex = repairIndex;
			break;
		case 4:
			// Brain Buttons
			psCurrStats = (COMPONENT_STATS *)asBrainStats;
			size = sizeof(BRAIN_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_BRAIN];
			numStats = numBrainStats;
			compIndex = brainIndex;
			break;
		}
		for (i = 0; i < numStats; i++)
		{
			// If we are out of space in the list - stop
			if (numExtraSys >= MAXEXTRASYS)
			{
				ASSERT(false, "Too many components for the list");
				return false;
			}

			// Skip unavailable entries or non-design ones
			if (!(aAvailable[i] & AVAILABLE)
			    || !psCurrStats->designable)
			{
				// Update the stats pointer for the next button
				psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);

				continue;
			}

			// Set the tip and add the button
			IntStatsButton *button = new IntStatsButton(compList);
			button->id = nextButtonId;
			button->setStatsAndTip(psCurrStats);
			compList->addWidgetToLayout(button);

			//just use one set of buffers for mixed system form
			if (psCurrStats->compType == COMP_BRAIN)
			{
				button->setStats(((BRAIN_STATS *)psCurrStats)->psWeaponStat);
			}

			// Store the stat pointer in the list
			apsExtraSysList[numExtraSys++] = psCurrStats;

			// If this matches the sensorIndex note the form and button
			if (i == compIndex)
			{
				desCompID = nextButtonId;
				button->setState(WBUT_LOCK);
				compList->setCurrentPage(compList->pages() - 1);
			}

			// Update the init struct for the next button
			++nextButtonId;

			// Update the stats pointer for the next button
			psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);
		}
	}

	return true;
}


/* Set the bar graphs for the system clickable */
static void intSetSystemStats(COMPONENT_STATS *psStats)
{
	W_FORM *psForm;

	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");

	/* set form tip to stats string */
	widgSetTip(psWScreen, IDDES_SYSTEMFORM, getStatName(psStats));

	/* set form stats for later display in intDisplayStatForm */
	psForm = (W_FORM *) widgGetFromID(psWScreen, IDDES_SYSTEMFORM);
	if (psForm != NULL)
	{
		psForm->pUserData = psStats;
	}

	/* Set the correct system stats */
	switch (psStats->compType)
	{
	case COMP_SENSOR:
		intSetSensorStats((SENSOR_STATS *)psStats);
		break;
	case COMP_ECM:
		intSetECMStats((ECM_STATS *)psStats);
		break;
	case COMP_WEAPON:
		intSetWeaponStats((WEAPON_STATS *)psStats);
		break;
	case COMP_CONSTRUCT:
		intSetConstructStats((CONSTRUCT_STATS *)psStats);
		break;
	case COMP_REPAIRUNIT:
		intSetRepairStats((REPAIR_STATS *)psStats);
		break;
	default:
		ASSERT(false, "Bad choice");
	}
}

/* Set the shadow bar graphs for the system clickable */
static void intSetSystemShadowStats(COMPONENT_STATS *psStats)
{
	/* Set the correct system stats - psStats can be set to NULL if
	 * desSysMode does not match the type of the stats.
	 */
	if (psStats)
	{
		switch (psStats->compType)
		{
		case COMP_SENSOR:
			if (desSysMode == IDES_SENSOR)
			{
				intSetSensorShadowStats((SENSOR_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_ECM:
			if (desSysMode == IDES_ECM)
			{
				intSetECMShadowStats((ECM_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_WEAPON:
			if (desSysMode == IDES_WEAPON)
			{
				intSetWeaponShadowStats((WEAPON_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_CONSTRUCT:
			if (desSysMode == IDES_CONSTRUCT)
			{
				intSetConstructShadowStats((CONSTRUCT_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_BRAIN:
			psStats = NULL;
			break;
		case COMP_REPAIRUNIT:
			if (desSysMode == IDES_REPAIR)
			{
				intSetRepairShadowStats((REPAIR_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		default:
			ASSERT(false, "Bad choice");
		}
	}
	else // if !psStats
	{
		switch (desSysMode)
		{
		case IDES_SENSOR:
			intSetSensorShadowStats(NULL);
			break;
		case IDES_ECM:
			intSetECMShadowStats(NULL);
			break;
		case IDES_WEAPON:
			intSetWeaponShadowStats(NULL);
			break;
		case IDES_CONSTRUCT:
			intSetConstructShadowStats(NULL);
			break;
		case IDES_REPAIR:
			intSetRepairShadowStats(NULL);
			break;
		default:
			break;
		}
	}
}

/* Set the bar graphs for the sensor stats */
static void intSetSensorStats(SENSOR_STATS *psStats)
{
	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN(, (psStats->ref >= REF_SENSOR_START) &&
	                 (psStats->ref < REF_SENSOR_START + REF_RANGE), "stats ref is out of range");

	/* range */
	widgSetBarSize(psWScreen, IDDES_SENSORRANGE, sensorRange(psStats, selectedPlayer));
	/* power */
	widgSetBarSize(psWScreen, IDDES_SENSORPOWER, 0);	// FIXME: Remove
	/* weight */
	widgSetBarSize(psWScreen, IDDES_SENSORWEIGHT, psStats->weight);
}

/* Set the shadow bar graphs for the sensor stats */
static void intSetSensorShadowStats(SENSOR_STATS *psStats)
{
	ASSERT(psStats == NULL ||
	       ((psStats->ref >= REF_SENSOR_START) &&
	        (psStats->ref < REF_SENSOR_START + REF_RANGE)),
	       "stats ref is out of range");

	if (psStats)
	{
		/* range */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORRANGE,
		                    sensorRange(psStats, (UBYTE)selectedPlayer));
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORPOWER, 0);	// FIXME: Remove
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORWEIGHT, psStats->weight);
	}
	else
	{
		/* Remove the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORRANGE, 0);
		widgSetMinorBarSize(psWScreen, IDDES_SENSORPOWER, 0);
		widgSetMinorBarSize(psWScreen, IDDES_SENSORWEIGHT, 0);
	}
}


/* Set the bar graphs for the ECM stats */
static void intSetECMStats(ECM_STATS *psStats)
{
	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN(, (psStats->ref >= REF_ECM_START) &&
	                 (psStats->ref < REF_ECM_START + REF_RANGE), "stats ref is out of range");

	/* range */
	widgSetBarSize(psWScreen, IDDES_ECMPOWER, ecmRange(psStats, selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_ECMWEIGHT, psStats->weight);
}

/* Set the shadow bar graphs for the ECM stats */
static void intSetECMShadowStats(ECM_STATS *psStats)
{
	ASSERT(psStats == NULL ||
	       ((psStats->ref >= REF_ECM_START) &&
	        (psStats->ref < REF_ECM_START + REF_RANGE)),
	       "stats ref is out of range");

	if (psStats)
	{
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_ECMPOWER, ecmRange(psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_ECMWEIGHT, psStats->weight);
	}
	else
	{
		/* Remove the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_ECMPOWER, 0);
		widgSetMinorBarSize(psWScreen, IDDES_ECMWEIGHT, 0);
	}
}


/* Set the bar graphs for the Constructor stats */
static void intSetConstructStats(CONSTRUCT_STATS *psStats)
{
	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN(, (psStats->ref >= REF_CONSTRUCT_START) &&
	                 (psStats->ref < REF_CONSTRUCT_START + REF_RANGE), "stats ref is out of range");

	/* power */
	widgSetBarSize(psWScreen, IDDES_CONSTPOINTS,
	               constructorPoints(psStats, (UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_CONSTWEIGHT, psStats->weight);
}


/* Set the shadow bar graphs for the Constructor stats */
static void intSetConstructShadowStats(CONSTRUCT_STATS *psStats)
{
	ASSERT(psStats == NULL ||
	       ((psStats->ref >= REF_CONSTRUCT_START) &&
	        (psStats->ref < REF_CONSTRUCT_START + REF_RANGE)),
	       "stats ref is out of range");

	if (psStats)
	{
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_CONSTPOINTS,
		                    constructorPoints(psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_CONSTWEIGHT, psStats->weight);
	}
	else
	{
		/* reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_CONSTPOINTS, 0);
		widgSetMinorBarSize(psWScreen, IDDES_CONSTWEIGHT, 0);
	}
}

/* Set the bar graphs for the Repair stats */
static void intSetRepairStats(REPAIR_STATS *psStats)
{
	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN(, (psStats->ref >= REF_REPAIR_START) &&
	                 (psStats->ref < REF_REPAIR_START + REF_RANGE), "stats ref is out of range");

	/* power */
	widgSetBarSize(psWScreen, IDDES_REPAIRPOINTS,
	               repairPoints(psStats, (UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_REPAIRWEIGHT, psStats->weight);
}


/* Set the shadow bar graphs for the Repair stats */
static void intSetRepairShadowStats(REPAIR_STATS *psStats)
{
	ASSERT(psStats == NULL ||
	       ((psStats->ref >= REF_REPAIR_START) &&
	        (psStats->ref < REF_REPAIR_START + REF_RANGE)),
	       "stats ref is out of range");

	if (psStats)
	{
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRPOINTS,
		                    repairPoints(psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRWEIGHT, psStats->weight);
	}
	else
	{
		/* reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRPOINTS, 0);
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRWEIGHT, 0);
	}
}


/* Set the bar graphs for the Weapon stats */
static void intSetWeaponStats(WEAPON_STATS *psStats)
{
	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN(, (psStats->ref >= REF_WEAPON_START) &&
	                 (psStats->ref < REF_WEAPON_START + REF_RANGE), "stats ref is out of range");

	/* range */
	widgSetBarSize(psWScreen, IDDES_WEAPRANGE, proj_GetLongRange(psStats, selectedPlayer));
	/* rate of fire */
	widgSetBarSize(psWScreen, IDDES_WEAPROF, weaponROF(psStats, (SBYTE)selectedPlayer));
	/* damage */
	widgSetBarSize(psWScreen, IDDES_WEAPDAMAGE, (UWORD)weaponDamage(psStats,
	               (UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_WEAPWEIGHT, psStats->weight);
}

/* Set the shadow bar graphs for the Weapon stats */
static void intSetWeaponShadowStats(WEAPON_STATS *psStats)
{
	ASSERT(psStats == NULL ||
	       ((psStats->ref >= REF_WEAPON_START) &&
	        (psStats->ref < REF_WEAPON_START + REF_RANGE)),
	       "stats ref is out of range");

	if (psStats)
	{
		/* range */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPRANGE, proj_GetLongRange(psStats, selectedPlayer));
		/* rate of fire */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPROF, weaponROF(psStats, (SBYTE)selectedPlayer));
		/* damage */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPDAMAGE, (UWORD)weaponDamage(
		                        psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPWEIGHT, psStats->weight);
	}
	else
	{
		/* Reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPRANGE, 0);
		widgSetMinorBarSize(psWScreen, IDDES_WEAPROF, 0);
		widgSetMinorBarSize(psWScreen, IDDES_WEAPDAMAGE, 0);
		widgSetMinorBarSize(psWScreen, IDDES_WEAPWEIGHT, 0);
	}
}

/* Set the bar graphs for the Body stats */
static void intSetBodyStats(BODY_STATS *psStats)
{
	W_FORM	*psForm;

	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN(, (psStats->ref >= REF_BODY_START) &&
	                 (psStats->ref < REF_BODY_START + REF_RANGE),
	                 "stats ref is out of range");

	/* set form tip to stats string */
	widgSetTip(psWScreen, IDDES_BODYFORM, getStatName(psStats));

	/* armour */
	//do kinetic armour
	widgSetBarSize(psWScreen, IDDES_BODYARMOUR_K, bodyArmour(psStats, selectedPlayer, WC_KINETIC));
	//do heat armour
	widgSetBarSize(psWScreen, IDDES_BODYARMOUR_H, bodyArmour(psStats, selectedPlayer, WC_HEAT));
	/* power */
	widgSetBarSize(psWScreen, IDDES_BODYPOWER, bodyPower(psStats, selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_BODYWEIGHT, psStats->weight);

	/* set form stats for later display in intDisplayStatForm */
	psForm = (W_FORM *) widgGetFromID(psWScreen, IDDES_BODYFORM);
	if (psForm != NULL)
	{
		psForm->pUserData = psStats;
	}
}

/* Set the shadow bar graphs for the Body stats */
static void intSetBodyShadowStats(BODY_STATS *psStats)
{
	ASSERT(psStats == NULL ||
	       ((psStats->ref >= REF_BODY_START) &&
	        (psStats->ref < REF_BODY_START + REF_RANGE)),
	       "stats ref is out of range");

	if (psStats)
	{
		/* armour - kinetic*/
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_K, bodyArmour(psStats, selectedPlayer, WC_KINETIC));
		//armour - heat
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_H, bodyArmour(psStats, selectedPlayer, WC_HEAT));
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_BODYPOWER, bodyPower(psStats, selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_BODYWEIGHT, psStats->weight);
	}
	else
	{
		/* Reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_K, 0);
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_H, 0);
		widgSetMinorBarSize(psWScreen, IDDES_BODYPOWER, 0);
		widgSetMinorBarSize(psWScreen, IDDES_BODYWEIGHT, 0);
	}
}

/* Sets the Design Power Bar for a given Template */
static void intSetDesignPower(DROID_TEMPLATE *psTemplate)
{
	/* use the same scale as PowerBar in main window so values are relative */
	widgSetBarSize(psWScreen, IDDES_POWERBAR, calcTemplatePower(psTemplate));
}

// work out current system component
static COMPONENT_TYPE getSystemType(DROID_TEMPLATE *droidTemplate)
{
	if (droidTemplate->asParts[COMP_ECM])
	{
		return COMP_ECM;
	}
	else if (droidTemplate->asParts[COMP_SENSOR])
	{
		return COMP_SENSOR;
	}
	else if (droidTemplate->asParts[COMP_CONSTRUCT])
	{
		return COMP_CONSTRUCT;
	}
	else if (droidTemplate->asParts[COMP_REPAIRUNIT])
	{
		return COMP_REPAIRUNIT;
	}
	else if (droidTemplate->asWeaps[0])
	{
		return COMP_WEAPON;
	}
	else
	{
		// compare it with the current weapon
		return COMP_WEAPON;
	}
}

/* Set the shadow bar graphs for the template power points - psStats is new hilited stats*/
static void intSetTemplatePowerShadowStats(COMPONENT_STATS *psStats)
{
	if (!psStats)
	{
		/* Reset the shadow bar */
		widgSetMinorBarSize(psWScreen, IDDES_POWERBAR, 0);
		return;
	}

	COMPONENT_TYPE type = psStats->compType;
	UDWORD power;
	UDWORD bodyPower        = asBodyStats[sCurrDesign.asParts[COMP_BODY]].buildPower;
	UDWORD brainPower       = asBrainStats[sCurrDesign.asParts[COMP_BRAIN]].buildPower;
	UDWORD sensorPower      = asSensorStats[sCurrDesign.asParts[COMP_SENSOR]].buildPower;
	UDWORD ECMPower         = asECMStats[sCurrDesign.asParts[COMP_ECM]].buildPower;
	UDWORD repairPower      = asRepairStats[sCurrDesign.asParts[COMP_REPAIRUNIT]].buildPower;
	UDWORD constructPower   = asConstructStats[sCurrDesign.asParts[COMP_CONSTRUCT]].buildPower;
	UDWORD propulsionPower  = asPropulsionStats[sCurrDesign.asParts[COMP_PROPULSION]].buildPower;
	UDWORD weaponPower1     = asWeaponStats[sCurrDesign.numWeaps ? sCurrDesign.asWeaps[0] : 0].buildPower;
	UDWORD weaponPower2     = asWeaponStats[sCurrDesign.numWeaps >= 2 ? sCurrDesign.asWeaps[1] : 0].buildPower;
	UDWORD weaponPower3     = asWeaponStats[sCurrDesign.numWeaps >= 3 ? sCurrDesign.asWeaps[2] : 0].buildPower;
	UDWORD newComponentPower = psStats->buildPower;

	// Commanders receive the stats of their associated weapon.
	if (type == COMP_BRAIN)
	{
		newComponentPower += ((BRAIN_STATS *)psStats)->psWeaponStat->buildPower;
	}
	/*if type = BODY or PROPULSION can do a straight comparison but if the new stat is
	a 'system' stat then need to find out which 'system' is currently in place so the
	comparison is meaningful*/
	if (desCompMode == IDES_SYSTEM)
	{
		type = getSystemType(&sCurrDesign);
	}

	switch (type)
	{
	case COMP_BODY:
		bodyPower = newComponentPower;
		break;
	case COMP_PROPULSION:
		propulsionPower = newComponentPower;
		break;
	case COMP_ECM:
		ECMPower = newComponentPower;
		break;
	case COMP_SENSOR:
		sensorPower = newComponentPower;
		break;
	case COMP_CONSTRUCT:
		constructPower = newComponentPower;
		break;
	case COMP_REPAIRUNIT:
		repairPower = newComponentPower;
		break;
	case COMP_WEAPON:
		brainPower = 0;
		if (desCompMode == IDES_TURRET_A)
		{
			weaponPower2 = newComponentPower;
		}
		else if (desCompMode == IDES_TURRET_B)
		{
			weaponPower3 = newComponentPower;
		}
		else
		{
			weaponPower1 = newComponentPower;
		}
		break;
	default:
		ASSERT(false, "Bad choice");
	}

	// this code is from calcTemplatePower

	//get the component power
	power = bodyPower + brainPower + sensorPower + ECMPower + repairPower + constructPower;

	/* propulsion power points are a percentage of the bodys' power points */
	power += (propulsionPower * bodyPower) / 100;

	//add weapon power
	power += weaponPower1 + weaponPower2 + weaponPower3;
	widgSetMinorBarSize(psWScreen, IDDES_POWERBAR, power);
}

/* Sets the Body Points Bar for a given Template */
static void intSetBodyPoints(DROID_TEMPLATE *psTemplate)
{
	// If total greater than Body Bar size then scale values.
	widgSetBarSize(psWScreen, IDDES_BODYPOINTS, calcTemplateBody(psTemplate, selectedPlayer));
}

/* Set the shadow bar graphs for the template Body points - psStats is new hilited stats*/
static void intSetTemplateBodyShadowStats(COMPONENT_STATS *psStats)
{
	if (!psStats)
	{
		/* Reset the shadow bar */
		widgSetMinorBarSize(psWScreen, IDDES_BODYPOINTS, 0);
		return;
	}

	COMPONENT_TYPE type = psStats->compType;
	UDWORD body;
	UDWORD bodyBody        = asBodyStats[sCurrDesign.asParts[COMP_BODY]].body;
	UDWORD brainBody       = asBrainStats[sCurrDesign.asParts[COMP_BRAIN]].body;
	UDWORD sensorBody      = asSensorStats[sCurrDesign.asParts[COMP_SENSOR]].body;
	UDWORD ECMBody         = asECMStats[sCurrDesign.asParts[COMP_ECM]].body;
	UDWORD repairBody      = asRepairStats[sCurrDesign.asParts[COMP_REPAIRUNIT]].body;
	UDWORD constructBody   = asConstructStats[sCurrDesign.asParts[COMP_CONSTRUCT]].body;
	UDWORD propulsionBody  = asPropulsionStats[sCurrDesign.asParts[COMP_PROPULSION]].body;
	UDWORD weaponBody1     = asWeaponStats[sCurrDesign.numWeaps ? sCurrDesign.asWeaps[0] : 0].body;
	UDWORD weaponBody2     = asWeaponStats[sCurrDesign.numWeaps >= 2 ? sCurrDesign.asWeaps[1] : 0].body;
	UDWORD weaponBody3     = asWeaponStats[sCurrDesign.numWeaps >= 3 ? sCurrDesign.asWeaps[2] : 0].body;
	UDWORD newComponentBody = psStats->body;

	// Commanders receive the stats of their associated weapon.
	if (type == COMP_BRAIN)
	{
		newComponentBody += ((BRAIN_STATS *)psStats)->psWeaponStat->body;
	}
	/*if type = BODY or PROPULSION can do a straight comparison but if the new stat is
	a 'system' stat then need to find out which 'system' is currently in place so the
	comparison is meaningful*/
	if (desCompMode == IDES_SYSTEM)
	{
		type = getSystemType(&sCurrDesign);
	}

	switch (type)
	{
	case COMP_BODY:
		bodyBody = newComponentBody;
		break;
	case COMP_PROPULSION:
		propulsionBody = newComponentBody;
		break;
	case COMP_ECM:
		ECMBody = newComponentBody;
		break;
	case COMP_SENSOR:
		sensorBody = newComponentBody;
		break;
	case COMP_CONSTRUCT:
		constructBody = newComponentBody;
		break;
	case COMP_REPAIRUNIT:
		repairBody = newComponentBody;
		break;
	case COMP_WEAPON:
		brainBody = 0;
		if (desCompMode == IDES_TURRET_A)
		{
			weaponBody2 = newComponentBody;
		}
		else if (desCompMode == IDES_TURRET_B)
		{
			weaponBody3 = newComponentBody;
		}
		else
		{
			weaponBody1 = newComponentBody;
		}
		break;
	default:
		ASSERT(false, "Bad choice");
	}
	// this code is from calcTemplateBody

	//get the component HP
	body = bodyBody + brainBody + sensorBody + ECMBody + repairBody + constructBody;

	/* propulsion HP are a percentage of the body's HP */
	body += (propulsionBody * bodyBody) / 100;

	//add weapon HP
	body += weaponBody1 + weaponBody2 + weaponBody3;
	body += (body * asBodyStats[sCurrDesign.asParts[COMP_BODY]].upgrade[selectedPlayer].body / 100);
	widgSetMinorBarSize(psWScreen, IDDES_BODYPOINTS, body);
}


/* Calculate the speed of a droid over a type of terrain */
static UDWORD intCalcSpeed(TYPE_OF_TERRAIN type, PROPULSION_STATS *psProp)
{
	UDWORD weight = calcDroidWeight(&sCurrDesign);
	if (weight == 0)
	{
		return 0;
	}
	//we want the design screen to show zero speed over water for all prop types except Hover and Vtol
	if (type == TER_WATER)
	{
		if (!(psProp->propulsionType == PROPULSION_TYPE_HOVER || psProp->propulsionType == PROPULSION_TYPE_LIFT))
		{
			return 0;
		}
	}
	return calcDroidSpeed(calcDroidBaseSpeed(&sCurrDesign, weight, selectedPlayer), type, psProp - asPropulsionStats, 0);
}


/* Set the bar graphs for the Propulsion stats */
static void intSetPropulsionStats(PROPULSION_STATS *psStats)
{
	W_FORM	    *psForm;
	UDWORD      weight;

	ASSERT_OR_RETURN(, psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN(, (psStats->ref >= REF_PROPULSION_START) &&
	                 (psStats->ref < REF_PROPULSION_START + REF_RANGE), "stats ref is out of range");

	/* set form tip to stats string */
	widgSetTip(psWScreen, IDDES_PROPFORM, getStatName(psStats));

	/* set form stats for later display in intDisplayStatForm */
	psForm = (W_FORM *) widgGetFromID(psWScreen, IDDES_PROPFORM);
	if (psForm != NULL)
	{
		psForm->pUserData = psStats;
	}

	switch (desPropMode)
	{
	case IDES_GROUND:
		/* Road speed */
		widgSetBarSize(psWScreen, IDDES_PROPROAD, intCalcSpeed(TER_ROAD, psStats));
		/* Cross country speed - grass */
		widgSetBarSize(psWScreen, IDDES_PROPCOUNTRY, intCalcSpeed(TER_SANDYBRUSH, psStats));
		/* Water speed */
		widgSetBarSize(psWScreen, IDDES_PROPWATER, intCalcSpeed(TER_WATER, psStats));
		break;
	case IDES_AIR:
		/* Air speed - terrain type doesn't matter, use road */
		widgSetBarSize(psWScreen, IDDES_PROPAIR, intCalcSpeed(TER_ROAD, psStats));
		break;
	default:
		break;
	}

	/* weight */
	//widgSetBarSize(psWScreen, IDDES_PROPWEIGHT, psStats->weight);

	/* propulsion weight is a percentage of the body weight */
	if (sCurrDesign.asParts[COMP_BODY] != 0)
	{
		weight = psStats->weight * asBodyStats[sCurrDesign.asParts[COMP_BODY]].weight / 100;
	}
	else
	{
		//if haven't got a body - can't calculate a value
		weight = 0;
	}
	widgSetBarSize(psWScreen, IDDES_PROPWEIGHT, weight);
}


/* Set the shadow bar graphs for the Propulsion stats */
static void intSetPropulsionShadowStats(PROPULSION_STATS *psStats)
{
	UDWORD      weight;

	ASSERT(psStats == NULL ||
	       ((psStats->ref >= REF_PROPULSION_START) &&
	        (psStats->ref < REF_PROPULSION_START + REF_RANGE)),
	       "stats ref is out of range");

	/* Only set the shadow stats if they are the right type */
	if (psStats &&
	    ((asPropulsionTypes[psStats->propulsionType].travel == GROUND &&
	      desPropMode != IDES_GROUND) ||
	     (asPropulsionTypes[psStats->propulsionType].travel == AIR &&
	      desPropMode != IDES_AIR)))
	{
		return;
	}

	switch (desPropMode)
	{
	case IDES_GROUND:
		if (psStats)
		{
			/* Road speed */
			widgSetMinorBarSize(psWScreen, IDDES_PROPROAD,
			                    intCalcSpeed(TER_ROAD, psStats));
			/* Cross country speed - grass */
			widgSetMinorBarSize(psWScreen, IDDES_PROPCOUNTRY,
			                    intCalcSpeed(TER_SANDYBRUSH, psStats));
			/* Water speed */
			widgSetMinorBarSize(psWScreen, IDDES_PROPWATER,
			                    intCalcSpeed(TER_WATER, psStats));
		}
		else
		{
			/* Reset the shadow bars */
			widgSetMinorBarSize(psWScreen, IDDES_PROPROAD, 0);
			widgSetMinorBarSize(psWScreen, IDDES_PROPCOUNTRY, 0);
			widgSetMinorBarSize(psWScreen, IDDES_PROPWATER, 0);
		}
		break;
	case IDES_AIR:
		if (psStats)
		{
			/* Air speed - terrain type doesn't matter, use ROAD */
			widgSetMinorBarSize(psWScreen, IDDES_PROPAIR,
			                    intCalcSpeed(TER_ROAD, psStats));
		}
		else
		{
			/* Reset the shadow bar */
			widgSetMinorBarSize(psWScreen, IDDES_PROPAIR, 0);
		}
		break;
	default:
		break;
	}

	if (psStats)
	{
		/* weight */
		//widgSetMinorBarSize(psWScreen, IDDES_PROPWEIGHT, psStats->weight);

		/* propulsion weight is a percentage of the body weight */
		if (sCurrDesign.asParts[COMP_BODY] != 0)
		{
			weight = psStats->weight * asBodyStats[sCurrDesign.asParts[COMP_BODY]].weight / 100;
		}
		else
		{
			//if haven't got a body - can't calculate a value
			weight = 0;
		}
		widgSetMinorBarSize(psWScreen, IDDES_PROPWEIGHT, weight);
	}
	else
	{
		/* Reset the shadow bar */
		widgSetMinorBarSize(psWScreen, IDDES_PROPWEIGHT, 0);
	}
}


/* Check whether a droid template is valid */
bool intValidTemplate(DROID_TEMPLATE *psTempl, const char *newName, bool complain, int player)
{
	code_part level = complain ? LOG_ERROR : LOG_NEVER;
	int bodysize = (asBodyStats + psTempl->asParts[COMP_BODY])->size;

	// set the weapon for a command droid
	if (psTempl->asParts[COMP_BRAIN] != 0)
	{
		psTempl->numWeaps = 1;
		psTempl->asWeaps[0] = asBrainStats[psTempl->asParts[COMP_BRAIN]].psWeaponStat - asWeaponStats;
	}

	/* Check all the components have been set */
	if (psTempl->asParts[COMP_BODY] == 0)
	{
		debug(level, "No body given for template");
		return false;
	}
	else if (psTempl->asParts[COMP_PROPULSION] == 0)
	{
		debug(level, "No propulsion given for template");
		return false;
	}

	// Check a turret has been installed
	if (psTempl->numWeaps == 0 &&
	    psTempl->asParts[COMP_SENSOR] == 0 &&
	    psTempl->asParts[COMP_ECM] == 0 &&
	    psTempl->asParts[COMP_BRAIN] == 0 &&
	    psTempl->asParts[COMP_REPAIRUNIT] == 0 &&
	    psTempl->asParts[COMP_CONSTRUCT] == 0)
	{
		debug(level, "No turret for template");
		return false;
	}

	/* Check the weapons */
	for (int i = 0; i < psTempl->numWeaps; i++)
	{
		int weaponSize = (asWeaponStats + psTempl->asWeaps[i])->weaponSize;

		if ((weaponSize == WEAPON_SIZE_LIGHT && bodysize != SIZE_LIGHT)
		    || (weaponSize == WEAPON_SIZE_HEAVY && bodysize == SIZE_LIGHT)
		    || psTempl->asWeaps[i] == 0)
		{
			debug(level, "No weapon given for weapon droid, or wrong weapon size");
			return false;
		}
		if (checkTemplateIsVtol(psTempl)
		    && (asWeaponStats + psTempl->asWeaps[i])->vtolAttackRuns <= 0)
		{
			debug(level, "VTOL with non-VTOL turret, not possible");
			return false;
		}
	}

	// Check number of weapon slots
	if (psTempl->numWeaps > (asBodyStats + psTempl->asParts[COMP_BODY])->weaponSlots)
	{
		debug(level, "Too many weapon turrets");
		return false;
	}

	// Check no mixing of systems and weapons
	if (psTempl->numWeaps != 0 &&
	    (psTempl->asParts[COMP_SENSOR] ||
	     psTempl->asParts[COMP_ECM] ||
	     (psTempl->asParts[COMP_REPAIRUNIT] && psTempl->asParts[COMP_REPAIRUNIT] != aDefaultRepair[player]) ||
	     psTempl->asParts[COMP_CONSTRUCT]))
	{
		debug(level, "Cannot mix system and weapon turrets in a template!");
		return false;
	}
	if (psTempl->numWeaps != 1 && psTempl->asParts[COMP_BRAIN])
	{
		debug(level, "Commander template needs 1 weapon turret");
		return false;
	}

	//can only have a VTOL weapon on a VTOL propulsion
	if (checkTemplateIsVtol(psTempl) && psTempl->numWeaps == 0)
	{
		debug(level, "VTOL with system turret, not possible");
		return false;
	}

	if (psTempl->asParts[COMP_SENSOR] == 0)
	{
		/* Set the default Sensor */
		psTempl->asParts[COMP_SENSOR] = aDefaultSensor[player];
	}

	if (psTempl->asParts[COMP_ECM] == 0)
	{
		/* Set the default ECM */
		psTempl->asParts[COMP_ECM] = aDefaultECM[player];
	}

	if (psTempl->asParts[COMP_REPAIRUNIT] == 0)
	{
		/* Set the default Repair */
		psTempl->asParts[COMP_REPAIRUNIT] = aDefaultRepair[player];
	}

	psTempl->ref = REF_TEMPLATE_START;

	//set the droidtype
	psTempl->droidType = droidTemplateType(psTempl);

	psTempl->enabled = true;

	/* copy name into template */
	psTempl->name = newName;

	return true;
}

static void desCreateDefaultTemplate(void)
{
	/* set current design to default */
	sCurrDesign = sDefaultDesignTemplate;
	sCurrDesign.stored = false;

	/* reset stats */
	intSetDesignStats(&sCurrDesign);
	widgDelete(psWScreen, IDDES_SYSTEMFORM);
	desSysMode = IDES_NOSYSTEM;
	haveCurrentDesign = true;
}

/* Remove the design widgets from the widget screen */
void intRemoveDesign(void)
{
	//save the current design on exit if it is valid
	saveTemplate();

	widgDelete(psWScreen, IDDES_POWERFORM);
	widgDelete(psWScreen, IDDES_NAMEBOX);
	widgDelete(psWScreen, IDDES_TEMPLBASE);
	widgDelete(psWScreen, IDDES_RIGHTBASE);

	widgDelete(psWScreen, IDDES_BODYFORM);
	widgDelete(psWScreen, IDDES_PROPFORM);
	widgDelete(psWScreen, IDDES_SYSTEMFORM);

	widgDelete(psWScreen, IDDES_FORM);
	widgDelete(psWScreen, IDDES_STATSFORM);

	resetDesignPauseState();
}

/* set flashing flag for button */
static void intSetButtonFlash(UDWORD id, bool bFlash)
{
	WIDGET	*psWidget = widgGetFromID(psWScreen, id);

	ASSERT_OR_RETURN(, psWidget->type == WIDG_BUTTON, "Not a button");

	psWidget->displayFunction = bFlash? intDisplayButtonFlash : intDisplayButtonHilight;
}

/*
 * desTemplateNameCustomised
 *
 * Checks whether user has customised template name : template not
 * customised if not complete or if generated name same as current.
 */
static bool desTemplateNameCustomised(DROID_TEMPLATE *psTemplate)
{
	if ((psTemplate->droidType == DROID_DEFAULT) ||
	    (strcmp(getName(psTemplate),
	            GetDefaultTemplateName(psTemplate)) == 0))
	{
		return false;
	}
	else
	{
		return true;
	}
}

static DROID_TEMPLATE *templateFromButtonId(unsigned buttonId, bool allowBlankTemplate = false)
{
	unsigned minIndex = allowBlankTemplate ? 0 : 1;
	unsigned index = buttonId - IDDES_TEMPLSTART;

	if (index >= minIndex && index < apsTemplateList.size())
	{
		return apsTemplateList[index];
	}
	return NULL;
}

/* Process return codes from the design screen */
void intProcessDesign(UDWORD id)
{
	bool bTemplateNameCustomised;

	/* check template button pressed */
	if (id >= IDDES_TEMPLSTART && id <= IDDES_TEMPLEND)
	{
		/* if first template create blank design */
		if (id == IDDES_TEMPLSTART)
		{
			desCreateDefaultTemplate();

			aCurrName[0] = '\0';
			sCurrDesign.name = aCurrName;

			/* reveal body button */
			widgReveal(psWScreen, IDDES_BODYBUTTON);
			/* hide other component buttons */
			widgHide(psWScreen, IDDES_SYSTEMBUTTON);
			widgHide(psWScreen, IDDES_PROPBUTTON);
			widgHide(psWScreen, IDDES_WPABUTTON);
			widgHide(psWScreen, IDDES_WPBBUTTON);

			/* set button render routines to flash */
			intSetButtonFlash(IDDES_BODYBUTTON,   true);
			intSetButtonFlash(IDDES_SYSTEMBUTTON, true);
			intSetButtonFlash(IDDES_PROPBUTTON,   true);
			intSetButtonFlash(IDDES_WPABUTTON,   true);
			intSetButtonFlash(IDDES_WPBBUTTON,   true);

			widgHide(psWScreen, IDDES_STOREBUTTON);
		}
		else
		{
			/* Find the template for the new button */
			DROID_TEMPLATE *psTempl = templateFromButtonId(id, true);

			ASSERT_OR_RETURN(, psTempl != NULL, "template not found!");

			if (psTempl != NULL)
			{
				/* Set the new template */
				sCurrDesign = *psTempl;
				sstrcpy(aCurrName, getName(psTempl));

				/* reveal body/propulsion/turret component buttons */
				widgReveal(psWScreen, IDDES_BODYBUTTON);
				widgReveal(psWScreen, IDDES_PROPBUTTON);
				widgReveal(psWScreen, IDDES_SYSTEMBUTTON);
				/* hide extra turrets */
				widgHide(psWScreen, IDDES_WPABUTTON);
				widgHide(psWScreen, IDDES_WPBBUTTON);

				/* turn off button flashes */
				intSetButtonFlash(IDDES_BODYBUTTON,   false);
				intSetButtonFlash(IDDES_SYSTEMBUTTON, false);
				intSetButtonFlash(IDDES_PROPBUTTON,   false);
				intSetButtonFlash(IDDES_WPABUTTON,   false);
				intSetButtonFlash(IDDES_WPBBUTTON,   false);

				// reveal additional buttons
				if (psTempl->numWeaps >= 2)
				{
					widgReveal(psWScreen, IDDES_WPABUTTON);
				}
				else
				{
					intSetButtonFlash(IDDES_WPABUTTON,   true);
				}
				if (psTempl->numWeaps == 3)
				{
					widgReveal(psWScreen, IDDES_WPBBUTTON);
				}
				else
				{
					intSetButtonFlash(IDDES_WPBBUTTON,   true);
				}

				widgReveal(psWScreen, IDDES_STOREBUTTON);
				updateStoreButton(sCurrDesign.stored);
			}
		}

		/* reveal design form if not already on-screen */
		widgReveal(psWScreen, IDDES_FORM);

		/* Droid template button has been pressed - clear the old button */
		if (droidTemplID != 0)
		{
			widgSetButtonState(psWScreen, droidTemplID, 0);
		}

		intSetDesignStats(&sCurrDesign);

		/* show body stats only */
		widgReveal(psWScreen, IDDES_STATSFORM);
		widgReveal(psWScreen, IDDES_BODYFORM);
		widgHide(psWScreen, IDDES_PROPFORM);
		widgHide(psWScreen, IDDES_SYSTEMFORM);

		/*Update the Power bar stats as the power to build will have changed */
		intSetDesignPower(&sCurrDesign);
		/*Update the body points */
		intSetBodyPoints(&sCurrDesign);

		/* Lock the button */
		widgSetButtonState(psWScreen, id, WBUT_LOCK);
		droidTemplID = id;

		/* Update the component form */
		widgDelete(psWScreen, IDDES_RIGHTBASE);
		/* reset button states */
		widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, 0);
		widgSetButtonState(psWScreen, IDDES_BODYBUTTON,   0);
		widgSetButtonState(psWScreen, IDDES_PROPBUTTON,   0);
		widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
		widgSetButtonState(psWScreen, IDDES_WPBBUTTON,   0);
		desCompMode = IDES_NOCOMPONENT;
		intSetDesignMode(IDES_BODY);
	}
	else if (id >= IDDES_COMPSTART && id <= IDDES_COMPEND)
	{
		/* check whether can change template name */
		bTemplateNameCustomised = desTemplateNameCustomised(&sCurrDesign);

		/* Component stats button has been pressed - clear the old button */
		if (desCompID != 0)
		{
			widgSetButtonState(psWScreen, desCompID, 0);
		}

		/* Set the stats in the template */
		switch (desCompMode)
		{
		case IDES_SYSTEM:
			//0 weapon for utility droid
			sCurrDesign.numWeaps = 0;
			break;
		case IDES_TURRET:
			/* Calculate the index of the component */
			sCurrDesign.asWeaps[0] =
			    ((WEAPON_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
			    asWeaponStats;
			if (sCurrDesign.numWeaps < 1)
			{
				sCurrDesign.numWeaps = 1;
			}
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			//Watemelon:weaponslots >= 2
			if ((asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots >= 2)
			{
				/* reveal turret_a button if hidden */
				widgReveal(psWScreen, IDDES_WPABUTTON);
			}
			/* Set the new stats on the display */
			intSetSystemForm(apsComponentList[id - IDDES_COMPSTART]);
			// Stop the button flashing
			intSetButtonFlash(IDDES_SYSTEMBUTTON, false);
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_WEAPON);
			}
			break;
			//Added cases for 2nd/3rd turret
		case IDES_TURRET_A:
			/* Calculate the index of the component */
			sCurrDesign.asWeaps[1] =
			    ((WEAPON_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
			    asWeaponStats;
			if (sCurrDesign.numWeaps < 2)
			{
				sCurrDesign.numWeaps = 2;
			}
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			//Watemelon:weaponSlots > 2
			if ((asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots > 2)
			{
				/* reveal turret_b button if hidden */
				widgReveal(psWScreen, IDDES_WPBBUTTON);
			}
			/* Set the new stats on the display */
			intSetSystemForm(apsComponentList[id - IDDES_COMPSTART]);
			// Stop the button flashing
			intSetButtonFlash(IDDES_WPABUTTON,   false);
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_WEAPON);
			}
			break;
		case IDES_TURRET_B:
			/* Calculate the index of the component */
			sCurrDesign.asWeaps[2] =
			    ((WEAPON_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
			    asWeaponStats;
			sCurrDesign.numWeaps = 3;
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			/* Set the new stats on the display */
			intSetSystemForm(apsComponentList[id - IDDES_COMPSTART]);
			// Stop the button flashing
			intSetButtonFlash(IDDES_WPBBUTTON,   false);
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_WEAPON);
			}
			break;
		case IDES_BODY:
			/* reveal propulsion button if hidden */
			widgReveal(psWScreen, IDDES_PROPBUTTON);

			/* Calculate the index of the component */
			sCurrDesign.asParts[COMP_BODY] =
			    ((BODY_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
			    asBodyStats;
			/* Set the new stats on the display */
			intSetBodyStats((BODY_STATS *)apsComponentList[id - IDDES_COMPSTART]);

			if ((sCurrDesign.asParts[COMP_BODY] + asBodyStats)->weaponSlots == 1)
			{
				if (sCurrDesign.numWeaps > 1)
				{
					sCurrDesign.numWeaps = 1;
					sCurrDesign.asWeaps[1] = 0;
					sCurrDesign.asWeaps[2] = 0;
				}
				widgHide(psWScreen, IDDES_WPABUTTON);
				widgHide(psWScreen, IDDES_WPBBUTTON);
			}
			else if ((sCurrDesign.asParts[COMP_BODY] + asBodyStats)->weaponSlots >= 2)
			{
				if (sCurrDesign.numWeaps > 2)
				{
					sCurrDesign.numWeaps = 2;
					sCurrDesign.asWeaps[2] = 0;
				}
				else if (sCurrDesign.numWeaps == 1 && sCurrDesign.asWeaps[0] && sCurrDesign.asParts[COMP_BRAIN] == 0)
				{
					widgReveal(psWScreen, IDDES_WPABUTTON);
					widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
					intSetButtonFlash(IDDES_WPABUTTON,   false);
				}
				else
				{
					widgHide(psWScreen, IDDES_WPBBUTTON);
				}
			}
			if ((sCurrDesign.asParts[COMP_BODY] + asBodyStats)->weaponSlots == 3)
			{
				if (sCurrDesign.numWeaps == 2)
				{
					widgReveal(psWScreen, IDDES_WPBBUTTON);
					widgSetButtonState(psWScreen, IDDES_WPBBUTTON,   0);
					intSetButtonFlash(IDDES_WPABUTTON,   false);
				}
				else if (sCurrDesign.numWeaps == 1 && sCurrDesign.asParts[COMP_BRAIN] == 0)
				{
					widgReveal(psWScreen, IDDES_WPABUTTON);
					widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
				}
			}
			// Stop the button flashing
			intSetButtonFlash(IDDES_BODYBUTTON,   false);
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_BODY);
			}
			break;
		case IDES_PROPULSION:
			/* Calculate the index of the component */
			sCurrDesign.asParts[COMP_PROPULSION] = ((PROPULSION_STATS *)apsComponentList[id - IDDES_COMPSTART]) - asPropulsionStats;

			/* Set the new stats on the display */
			intSetPropulsionForm((PROPULSION_STATS *)apsComponentList[id - IDDES_COMPSTART]);

			// Check that the weapon (if any) is valid for this propulsion
			if (!intCheckValidWeaponForProp())
			{
				// Not valid weapon so initialise the weapon stat
				sCurrDesign.asWeaps[0] = 0;
				sCurrDesign.asWeaps[1] = 0;
				sCurrDesign.asWeaps[2] = 0;
				sCurrDesign.numWeaps = 0;
				widgHide(psWScreen, IDDES_WPABUTTON);
				widgHide(psWScreen, IDDES_WPBBUTTON);

				// Init all other stats as well!
				sCurrDesign.asParts[COMP_SENSOR] = 0;
				sCurrDesign.asParts[COMP_BRAIN] = 0;
				sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
				sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
				sCurrDesign.asParts[COMP_ECM] = 0;

				// We need a turret again
				intSetButtonFlash(IDDES_SYSTEMBUTTON, true);
			}

			// Stop the button flashing
			intSetButtonFlash(IDDES_PROPBUTTON,   false);
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_PROPULSION);
			}
			break;
		default:
			break;
		}

		/* Lock the new button */
		widgSetButtonState(psWScreen, id, WBUT_LOCK);
		desCompID = id;

		/* Update the propulsion stats as the droid weight will have changed */
		intSetPropulsionStats(asPropulsionStats + sCurrDesign.asParts[COMP_PROPULSION]);

		/*Update the Power bar stats as the power to build will have changed */
		intSetDesignPower(&sCurrDesign);
		/*Update the body points */
		intSetBodyPoints(&sCurrDesign);

		/* update name if not customised */
		if (bTemplateNameCustomised == false)
		{
			sCurrDesign.name = GetDefaultTemplateName(&sCurrDesign);
		}

		/* Update the name in the edit box */
		intSetEditBoxTextFromTemplate(&sCurrDesign);
	}
	else if (id >= IDDES_EXTRASYSSTART && id <= IDDES_EXTRASYSEND)
	{
		/* check whether can change template name */
		bTemplateNameCustomised = desTemplateNameCustomised(&sCurrDesign);

		// Extra component stats button has been pressed - clear the old button
		if (desCompID != 0)
		{
			widgSetButtonState(psWScreen, desCompID, 0);
		}

		// Now store the new stats
		switch (apsExtraSysList[id - IDDES_EXTRASYSSTART]->compType)
		{
		case COMP_SENSOR:
			// Calculate the index of the component
			sCurrDesign.asParts[COMP_SENSOR] =
			    ((SENSOR_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
			    asSensorStats;
			// Reset the ECM, constructor and weapon and repair
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide(psWScreen, IDDES_WPABUTTON);
			widgHide(psWScreen, IDDES_WPBBUTTON);
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_ECM:
			// Calculate the index of the component
			sCurrDesign.asParts[COMP_ECM] =
			    ((ECM_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
			    asECMStats;
			// Reset the Sensor, constructor and weapon and repair
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide(psWScreen, IDDES_WPABUTTON);
			widgHide(psWScreen, IDDES_WPBBUTTON);
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_CONSTRUCT:
			// Calculate the index of the component and repair
			sCurrDesign.asParts[COMP_CONSTRUCT] =
			    ((CONSTRUCT_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
			    asConstructStats;
			// Reset the Sensor, ECM and weapon
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide(psWScreen, IDDES_WPABUTTON);
			widgHide(psWScreen, IDDES_WPBBUTTON);
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_REPAIRUNIT:
			// Calculate the index of the component
			sCurrDesign.asParts[COMP_REPAIRUNIT] =
			    ((REPAIR_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
			    asRepairStats;
			// Reset the Sensor, ECM and weapon and construct
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide(psWScreen, IDDES_WPABUTTON);
			widgHide(psWScreen, IDDES_WPBBUTTON);
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_BRAIN:
			/* Calculate the index of the brain */
			sCurrDesign.asParts[COMP_BRAIN] =
			    ((BRAIN_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
			    asBrainStats;
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.numWeaps = 1;
			sCurrDesign.asWeaps[0] =
			    (((BRAIN_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART])->psWeaponStat) -
			    asWeaponStats;
			widgHide(psWScreen, IDDES_WPABUTTON);
			widgHide(psWScreen, IDDES_WPBBUTTON);
			/* Set the new stats on the display */
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		default:
			ASSERT(false, "Bad choice");
		}
		// Stop the button flashing
		intSetButtonFlash(IDDES_SYSTEMBUTTON, false);
		// Lock the new button
		widgSetButtonState(psWScreen, id, WBUT_LOCK);
		desCompID = id;

		// Update the propulsion stats as the droid weight will have changed
		intSetPropulsionStats(asPropulsionStats + sCurrDesign.asParts[COMP_PROPULSION]);

		// Update the Power bar stats as the power to build will have changed
		intSetDesignPower(&sCurrDesign);
		// Update the body points
		intSetBodyPoints(&sCurrDesign);

		/* update name if not customised */
		if (bTemplateNameCustomised == false)
		{
			sCurrDesign.name = GetDefaultTemplateName(&sCurrDesign);
		}

		/* Update the name in the edit box */
		intSetEditBoxTextFromTemplate(&sCurrDesign);

		// do the callback if in the tutorial
		if (bInTutorial)
		{
			if (apsExtraSysList[id - IDDES_EXTRASYSSTART]->compType == COMP_BRAIN)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_COMMAND);
			}
			else
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_SYSTEM);
			}
		}
	}
	else
	{
		switch (id)
		{
			/* The four component clickable forms */
			/* the six component clickable forms... */
		case IDDES_WEAPONS:
			desCompID = 0;
			intSetDesignMode(IDES_TURRET);
			break;
		case IDDES_WEAPONS_A:
			desCompID = 0;
			intSetDesignMode(IDES_TURRET_A);
			break;
		case IDDES_WEAPONS_B:
			desCompID = 0;
			intSetDesignMode(IDES_TURRET_B);
			break;
		case IDDES_COMMAND:
			desCompID = 0;
			break;
		case IDDES_SYSTEMS:
			desCompID = 0;
			intSetDesignMode(IDES_SYSTEM);
			break;
			/* The name edit box */
		case IDDES_NAMEBOX:
			sCurrDesign.name = widgGetString(psWScreen, IDDES_NAMEBOX);
			sstrcpy(aCurrName, getName(&sCurrDesign));
			break;
		case IDDES_BIN:
			{
				/* Find the template for the current button */
				DROID_TEMPLATE *psTempl = templateFromButtonId(droidTemplID);  // Does not return the first template, which is the empty template.

				/* remove template if found */
				if (psTempl != NULL)
				{
					SendDestroyTemplate(psTempl, selectedPlayer);

					//update player template list.
					for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
					{
						if (&*i == psTempl)
						{
							//before deleting the template, need to make sure not being used in production
							deleteTemplateFromProduction(psTempl, selectedPlayer, ModeQueue);
							// Delete the template.
							localTemplates.erase(i);
							break;
						}
					}

					/* get previous template and set as current */
					psTempl = templateFromButtonId(droidTemplID - 1, true);  // droidTemplID - 1 always valid (might be the first template), since droidTemplID is not the first template.

					/* update local list */
					desSetupDesignTemplates();

					/* Now update the droid template form */
					widgDelete(psWScreen, IDDES_TEMPLBASE);
					intAddTemplateForm(psTempl);

					/* Set the new template */
					sCurrDesign = *psTempl;
					sstrcpy(aCurrName, getName(psTempl));

					intSetEditBoxTextFromTemplate(psTempl);

					intSetDesignStats(&sCurrDesign);

					/* show body stats only */
					widgReveal(psWScreen, IDDES_STATSFORM);
					widgReveal(psWScreen, IDDES_BODYFORM);
					widgHide(psWScreen, IDDES_PROPFORM);
					widgHide(psWScreen, IDDES_SYSTEMFORM);

					/*Update the Power bar stats as the power to build will have changed */
					intSetDesignPower(&sCurrDesign);
					/*Update the body points */
					intSetBodyPoints(&sCurrDesign);

					/* show correct body component highlight */
					widgDelete(psWScreen, IDDES_RIGHTBASE);
					/* reset button states */
					widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, 0);
					widgSetButtonState(psWScreen, IDDES_BODYBUTTON,   0);
					widgSetButtonState(psWScreen, IDDES_PROPBUTTON,   0);
					widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
					widgSetButtonState(psWScreen, IDDES_WPBBUTTON,   0);
					desCompMode = IDES_NOCOMPONENT;
					intSetDesignMode(IDES_BODY);
				}
				break;
			}
		case IDDES_STOREBUTTON:
			sCurrDesign.stored = !sCurrDesign.stored;	// Invert the current status
			saveTemplate();
			break;
		case IDDES_SYSTEMBUTTON:
			// Add the correct component form
			switch (droidTemplateType(&sCurrDesign))
			{
			case DROID_COMMAND:
			case DROID_SENSOR:
			case DROID_CONSTRUCT:
			case DROID_ECM:
			case DROID_REPAIR:
				intSetDesignMode(IDES_SYSTEM);
				break;
			default:
				intSetDesignMode(IDES_TURRET);
				break;
			}
			/* reveal components if not already onscreen */
			widgReveal(psWScreen, IDDES_STATSFORM);
			widgReveal(psWScreen, IDDES_RIGHTBASE);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			widgHide(psWScreen, IDDES_BODYFORM);
			widgHide(psWScreen, IDDES_PROPFORM);

			break;
			// WPABUTTON
		case IDDES_WPABUTTON:
			// Add the correct component form
			switch (droidTemplateType(&sCurrDesign))
			{
			case DROID_COMMAND:
			case DROID_SENSOR:
			case DROID_CONSTRUCT:
			case DROID_ECM:
			case DROID_REPAIR:
				break;
			default:
				intSetDesignMode(IDES_TURRET_A);
				break;
			}
			/* reveal components if not already onscreen */
			widgReveal(psWScreen, IDDES_STATSFORM);
			widgReveal(psWScreen, IDDES_RIGHTBASE);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			widgHide(psWScreen, IDDES_BODYFORM);
			widgHide(psWScreen, IDDES_PROPFORM);

			break;
			// WPBBUTTON
		case IDDES_WPBBUTTON:
			// Add the correct component form
			switch (droidTemplateType(&sCurrDesign))
			{
			case DROID_COMMAND:
			case DROID_SENSOR:
			case DROID_CONSTRUCT:
			case DROID_ECM:
			case DROID_REPAIR:
				break;
			default:
				intSetDesignMode(IDES_TURRET_B);
				break;
			}
			/* reveal components if not already onscreen */
			widgReveal(psWScreen, IDDES_STATSFORM);
			widgReveal(psWScreen, IDDES_RIGHTBASE);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			widgHide(psWScreen, IDDES_BODYFORM);
			widgHide(psWScreen, IDDES_PROPFORM);

			break;
		case IDDES_BODYBUTTON:
			/* reveal components if not already onscreen */
			widgReveal(psWScreen, IDDES_RIGHTBASE);
			intSetDesignMode(IDES_BODY);

			widgReveal(psWScreen, IDDES_STATSFORM);
			widgHide(psWScreen, IDDES_SYSTEMFORM);
			widgReveal(psWScreen, IDDES_BODYFORM);
			widgHide(psWScreen, IDDES_PROPFORM);

			break;
		case IDDES_PROPBUTTON:
			/* reveal components if not already onscreen */
			widgReveal(psWScreen, IDDES_RIGHTBASE);
			intSetDesignMode(IDES_PROPULSION);
			widgReveal(psWScreen, IDDES_STATSFORM);
			widgHide(psWScreen, IDDES_SYSTEMFORM);
			widgHide(psWScreen, IDDES_BODYFORM);
			widgReveal(psWScreen, IDDES_PROPFORM);

			break;
		}
	}

	/* show body button if component button pressed and
	 * save template if valid
	 */
	if ((id >= IDDES_COMPSTART && id <= IDDES_COMPEND) || (id >= IDDES_EXTRASYSSTART && id <= IDDES_EXTRASYSEND))
	{
		/* reveal body button if hidden */
		widgReveal(psWScreen, IDDES_BODYBUTTON);

		/* save template if valid */
		if (saveTemplate())
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROIDDESIGNED);
			triggerEventDesignCreated(&sCurrDesign);
		}

		switch (desCompMode)
		{
		case IDES_BODY:
			widgReveal(psWScreen, IDDES_BODYFORM);
			widgHide(psWScreen, IDDES_PROPFORM);
			widgHide(psWScreen, IDDES_SYSTEMFORM);
			break;

		case IDES_PROPULSION:
			widgHide(psWScreen, IDDES_BODYFORM);
			widgReveal(psWScreen, IDDES_PROPFORM);
			widgHide(psWScreen, IDDES_SYSTEMFORM);
			break;

		case IDES_SYSTEM:
		case IDES_TURRET:
			// reveals SYSTEMFORM
		case IDES_TURRET_A:
		case IDES_TURRET_B:
			widgHide(psWScreen, IDDES_BODYFORM);
			widgHide(psWScreen, IDDES_PROPFORM);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			break;
		default:
			break;
		}

		widgReveal(psWScreen, IDDES_STATSFORM);

		/* switch automatically to next component type if initial design */
		if (!intValidTemplate(&sCurrDesign, aCurrName, false, selectedPlayer))
		{
			/* show next component design screen */
			switch (desCompMode)
			{
			case IDES_BODY:
				intSetDesignMode(IDES_PROPULSION);
				widgReveal(psWScreen, IDDES_PROPBUTTON);
				break;

			case IDES_PROPULSION:
				intSetDesignMode(IDES_TURRET);
				widgReveal(psWScreen, IDDES_SYSTEMBUTTON);
				break;

			case IDES_SYSTEM:
			case IDES_TURRET:
				if ((asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots > 1 &&
				    sCurrDesign.numWeaps == 1 && sCurrDesign.asParts[COMP_BRAIN] == 0)
				{
					debug(LOG_GUI, "intProcessDesign: First weapon selected, doing next.");
					intSetDesignMode(IDES_TURRET_A);
					widgReveal(psWScreen, IDDES_WPABUTTON);
				}
				else
				{
					debug(LOG_GUI, "intProcessDesign: First weapon selected, is final.");
				}
				break;
			case IDES_TURRET_A:
				if ((asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots > 2)
				{
					debug(LOG_GUI, "intProcessDesign: Second weapon selected, doing next.");
					intSetDesignMode(IDES_TURRET_B);
					widgReveal(psWScreen, IDDES_WPBBUTTON);
				}
				else
				{
					debug(LOG_GUI, "intProcessDesign: Second weapon selected, is final.");
				}
				break;
			case IDES_TURRET_B:
				debug(LOG_GUI, "intProcessDesign: Third weapon selected, is final.");
				break;
			default:
				break;
			}
		}
	}
	//save the template if the name gets edited
	if (id == IDDES_NAMEBOX)
	{
		saveTemplate();
	}
}


/* Set the shadow bar graphs for the design screen */
void intRunDesign(void)
{
	UDWORD				statID;
	COMPONENT_STATS		*psStats;
	bool				templateButton;
	int compIndex;

	/* Find out which button was hilited */
	templateButton = false;
	statID = widgGetMouseOver(psWScreen);

	// Somut around here is casuing a nasty crash.....
	/* If a component button is hilited get the stats for it */
	if (statID == desCompID)
	{
		/* The mouse is over the selected component - no shadow stats */
		psStats = NULL;
	}
	else if (statID >= IDDES_COMPSTART && statID <= IDDES_COMPEND)
	{
		compIndex = statID - IDDES_COMPSTART;
		ASSERT_OR_RETURN(, compIndex < numComponent, "Invalid range referenced for numComponent, %d > %d", compIndex, numComponent);
		psStats = apsComponentList[compIndex];
	}
	else if (statID >= IDDES_EXTRASYSSTART && statID <= IDDES_EXTRASYSEND)
	{
		compIndex = statID - IDDES_EXTRASYSSTART;
		ASSERT_OR_RETURN(, compIndex < numExtraSys, "Invalid range referenced for numExtraSys, %d > %d", compIndex, numExtraSys);
		psStats = apsExtraSysList[compIndex];
	}
	else if (statID >= IDDES_TEMPLSTART && statID <= IDDES_TEMPLEND)
	{
		runTemplateShadowStats(statID);
		templateButton = true;
		psStats = NULL;
	}
	else
	{
		/* No component button so reset the stats to nothing */
		psStats = NULL;
	}

	/* Now set the bar graphs for the stats - don't bother if over template
	since setting them all!*/
	if (!templateButton)
	{
		switch (desCompMode)
		{
		case IDES_SYSTEM:
		case IDES_TURRET:
			intSetSystemShadowStats(psStats);
			intSetBodyShadowStats(NULL);
			intSetPropulsionShadowStats(NULL);
			break;
		case IDES_BODY:
			intSetSystemShadowStats(NULL);
			intSetBodyShadowStats((BODY_STATS *)psStats);
			intSetPropulsionShadowStats(NULL);
			break;
		case IDES_PROPULSION:
			intSetSystemShadowStats(NULL);
			intSetBodyShadowStats(NULL);
			intSetPropulsionShadowStats((PROPULSION_STATS *)psStats);
			break;
		default:
			break;
		}

		//set the template shadow stats
		intSetTemplateBodyShadowStats(psStats);
		intSetTemplatePowerShadowStats(psStats);
	}
}

static void intDisplayStatForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	static UDWORD	iRY = 45;
	W_CLICKFORM		*Form = (W_CLICKFORM *)psWidget;
	UWORD			x0 = xOffset + Form->x(), y0 = yOffset + Form->y();

	/* get stats from userdata pointer in widget stored in
	 * intSetSystemStats, intSetBodyStats, intSetPropulsionStats
	 */
	BASE_STATS *psStats = (BASE_STATS *) Form->pUserData;

	SWORD templateRadius = getComponentRadius(psStats);

	Vector3i Rotation(-30, iRY, 0), Position(0, -templateRadius / 4, BUTTON_DEPTH /* templateRadius * 12 */);

	//scale the object around the BUTTON_RADIUS so that half size objects are draw are draw 75% the size of normal objects
	SDWORD falseScale = (DESIGN_COMPONENT_SCALE * COMPONENT_RADIUS) / templateRadius / 2 + (DESIGN_COMPONENT_SCALE / 2);

	iV_DrawImage(IntImages, (UWORD)(IMAGE_DES_STATBACKLEFT), x0, y0);
	iV_DrawImageRepeatX(IntImages, IMAGE_DES_STATBACKMID, x0 + iV_GetImageWidth(IntImages, IMAGE_DES_STATBACKLEFT), y0,
	                    Form->width() - iV_GetImageWidth(IntImages, IMAGE_DES_STATBACKLEFT) - iV_GetImageWidth(IntImages, IMAGE_DES_STATBACKRIGHT));
	iV_DrawImage(IntImages, IMAGE_DES_STATBACKRIGHT, x0 + Form->width() - iV_GetImageWidth(IntImages, IMAGE_DES_STATBACKRIGHT), y0);

	/* display current component */
	pie_SetGeometricOffset(xOffset + psWidget->width() / 4, yOffset + psWidget->height() / 2);

	/* inc rotation if highlighted */
	if ((Form->getState() & WBUT_HIGHLIGHT) != 0)
	{
		iRY += realTimeAdjustedAverage(BUTTONOBJ_ROTSPEED);
		iRY %= 360;
	}

	//display component in bottom design screen window
	displayComponentButton(psStats, &Rotation, &Position, true, falseScale);
}

/* Displays the 3D view of the droid in a window on the design form */
static void intDisplayViewForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_FORM			*Form = (W_FORM *)psWidget;
	UDWORD			x0, y0, x1, y1;
	static UDWORD	iRY = 45;
	Vector3i			Rotation, Position;
	SWORD			templateRadius;
	SDWORD			falseScale;

	x0 = xOffset + Form->x();
	y0 = yOffset + Form->y();
	x1 = x0 + Form->width();
	y1 = y0 + Form->height();

	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);

	if (haveCurrentDesign)
	{
		pie_SetGeometricOffset((DES_CENTERFORMX + DES_3DVIEWX) + (DES_3DVIEWWIDTH / 2),
		                       (DES_CENTERFORMY + DES_3DVIEWY) + (DES_3DVIEWHEIGHT / 4) + 32);

		Rotation.x = -30;
		Rotation.y = iRY;
		Rotation.z = 0;

		/* inc rotation */
		iRY += realTimeAdjustedAverage(BUTTONOBJ_ROTSPEED);
		iRY %= 360;

		//fixed depth scale the pie
		Position.x = 0;
		Position.y = -100;
		Position.z = BUTTON_DEPTH;

		templateRadius = getComponentDroidTemplateRadius(&sCurrDesign);
		//scale the object around the OBJECT_RADIUS so that half size objects are draw are draw 75% the size of normal objects
		falseScale = (DESIGN_DROID_SCALE * OBJECT_RADIUS) / templateRadius;

		//display large droid view in the design screen
		displayComponentButtonTemplate(&sCurrDesign, &Rotation, &Position, true, falseScale);
	}
}

/* General display window for the design form  SOLID BACKGROUND - NOT TRANSPARENT*/
static void intDisplayDesignForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x0 = xOffset + psWidget->x();
	int y0 = yOffset + psWidget->y();
	int x1 = x0 + psWidget->width();
	int y1 = y0 + psWidget->height();

	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
}


/* save the current Template if valid. Return true if stored */
static bool saveTemplate(void)
{
	if (!intValidTemplate(&sCurrDesign, aCurrName, false, selectedPlayer))
	{
		widgHide(psWScreen, IDDES_STOREBUTTON);
		return false;
	}
	widgReveal(psWScreen, IDDES_STOREBUTTON);
	updateStoreButton(sCurrDesign.stored);	// Change the buttons icon

	/* if first (New Design) button selected find empty template
	 * else find current button template
	 */
	DROID_TEMPLATE *psTempl;
	if (droidTemplID == IDDES_TEMPLSTART)
	{
		/* create empty template and point to that */
		localTemplates.push_back(DROID_TEMPLATE());
		psTempl = &localTemplates.back();
		apsTemplateList.push_back(psTempl);

		psTempl->ref = REF_TEMPLATE_START;

		/* set button render routines to highlight, not flash */
		intSetButtonFlash(IDDES_SYSTEMBUTTON, false);
		intSetButtonFlash(IDDES_BODYBUTTON,   false);
		intSetButtonFlash(IDDES_PROPBUTTON,   false);
	}
	else
	{
		/* Find the template for the current button */
		psTempl = templateFromButtonId(droidTemplID);
		if (psTempl == NULL)
		{
			debug(LOG_ERROR, "Template not found for button");
			return false;
		}

		// ANY change to the template affect the production - even if the template is changed and then changed back again!
		deleteTemplateFromProduction(psTempl, selectedPlayer, ModeQueue);
		SendDestroyTemplate(psTempl, selectedPlayer);
	}

	/* Copy the template */
	*psTempl = sCurrDesign;

	/* Now update the droid template form */
	widgDelete(psWScreen, IDDES_TEMPLBASE);
	intAddTemplateForm(psTempl);

	// Send template to in-game template list, since localTemplates/apsDroidTemplates is for UI use only.
	psTempl->multiPlayerID = generateNewObjectId();
	sendTemplate(selectedPlayer, psTempl);

	return true;
}


/* Function to set the shadow bars for all the stats when the mouse is over the Template buttons */
void runTemplateShadowStats(UDWORD id)
{
	DROID_TEMPLATE	*psTempl = NULL;
	COMPONENT_STATS	*psStats;
	DROID_TYPE		templType;
	int				compIndex;

	/* Find the template for the new button */
	//we're ignoring the Blank Design so start at the second button
	psTempl = templateFromButtonId(id);

	//if we're over a different template
	if (psTempl && psTempl != &sCurrDesign)
	{
		/* Now set the bar graphs for the stats */
		intSetBodyShadowStats(asBodyStats + psTempl->asParts[COMP_BODY]);
		intSetPropulsionShadowStats(asPropulsionStats + psTempl->asParts[COMP_PROPULSION]);
		//only set the system shadow bar if the same type of droid
		psStats = NULL;
		templType = droidTemplateType(psTempl);
		if (templType == droidTemplateType(&sCurrDesign))
		{
			switch (templType)
			{
			case DROID_WEAPON:
				compIndex = psTempl->asWeaps[0];
				ASSERT_OR_RETURN(, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
				psStats = (COMPONENT_STATS *)(asWeaponStats + compIndex);
				break;
			case DROID_SENSOR:
				compIndex = psTempl->asParts[COMP_SENSOR];
				ASSERT_OR_RETURN(, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
				psStats = (COMPONENT_STATS *)(asSensorStats + compIndex);
				break;
			case DROID_ECM:
				compIndex = psTempl->asParts[COMP_ECM];
				ASSERT_OR_RETURN(, compIndex < numECMStats, "Invalid range referenced for numECMStats, %d > %d", compIndex, numECMStats);
				psStats = (COMPONENT_STATS *)(asECMStats + compIndex);
				break;
			case DROID_CONSTRUCT:
				compIndex = psTempl->asParts[COMP_CONSTRUCT];
				ASSERT_OR_RETURN(, compIndex < numConstructStats, "Invalid range referenced for numConstructStats, %d > %d", compIndex, numConstructStats);
				psStats = (COMPONENT_STATS *)(asConstructStats + compIndex);
				break;
			case DROID_REPAIR:
				compIndex = psTempl->asParts[COMP_REPAIRUNIT];
				ASSERT_OR_RETURN(, compIndex < numRepairStats, "Invalid range referenced for numRepairStats, %d > %d", compIndex, numRepairStats);
				psStats = (COMPONENT_STATS *)(asRepairStats + compIndex);
				break;
			default:
				break;
			}
		}

		if (psStats)
		{
			intSetSystemShadowStats(psStats);
		}
		//haven't got a stat so just do the code required here...
		widgSetMinorBarSize(psWScreen, IDDES_BODYPOINTS, calcTemplateBody(psTempl, selectedPlayer));
		widgSetMinorBarSize(psWScreen, IDDES_POWERBAR, calcTemplatePower(psTempl));
	}
}

/*sets which states need to be paused when the design screen is up*/
static void setDesignPauseState(void)
{
	if (!bMultiPlayer)
	{
		//need to clear mission widgets from being shown on design screen
		clearMissionWidgets();
		gameTimeStop();
		setGameUpdatePause(true);
		setScrollPause(true);
		screen_RestartBackDrop();
	}
}

/*resets the pause states */
static void resetDesignPauseState(void)
{
	if (!bMultiPlayer)
	{
		//put any widgets back on for the missions
		resetMissionWidgets();
		setGameUpdatePause(false);
		setScrollPause(false);
		gameTimeStart();
		screen_StopBackDrop();
		pie_ScreenFlip(CLEAR_BLACK);
	}
}

/*this is called when a new propulsion type is added to the current design
to check the weapon is 'allowed'. Check if VTOL, the weapon is direct fire.
Also check numVTOLattackRuns for the weapon is not zero - return true if valid weapon*/
static bool intCheckValidWeaponForProp(void)
{
	if (asPropulsionTypes[((PROPULSION_STATS *)(asPropulsionStats + sCurrDesign.asParts[COMP_PROPULSION]))->propulsionType].travel != AIR)
	{
		if (sCurrDesign.numWeaps == 0 &&
		    (sCurrDesign.asParts[COMP_SENSOR] ||
		     sCurrDesign.asParts[COMP_REPAIRUNIT] ||
		     sCurrDesign.asParts[COMP_CONSTRUCT] ||
		     sCurrDesign.asParts[COMP_ECM]))
		{
			// non-AIR propulsions can have systems, too.
			return true;
		}
	}
	return checkValidWeaponForProp(&sCurrDesign);
}

bool intAddDesign(bool bShowCentreScreen)
{
	return _intAddDesign(bShowCentreScreen);
}


/* Set up the system clickable form of the design screen given a set of stats */
static bool intSetSystemForm(COMPONENT_STATS *psStats)
{
	return _intSetSystemForm(psStats);
}


static bool intAddTemplateForm(DROID_TEMPLATE *psSelected)
{
	return _intAddTemplateForm(psSelected);
}

//checks if the template has PROPULSION_TYPE_LIFT propulsion attached - returns true if it does
bool checkTemplateIsVtol(DROID_TEMPLATE *psTemplate)
{
	if (asPropulsionStats[psTemplate->asParts[COMP_PROPULSION]].
	    propulsionType == PROPULSION_TYPE_LIFT)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void updateStoreButton(bool isStored)
{
	UDWORD imageset;

	if (isStored)
	{
		imageset = PACKDWORD_TRI(0, IMAGE_DES_DELETEH, IMAGE_DES_DELETE);
		widgGetFromID(psWScreen, IDDES_STOREBUTTON)->setTip(_("Delete Template"));
	}
	else
	{
		imageset = PACKDWORD_TRI(0, IMAGE_DES_SAVEH, IMAGE_DES_SAVE);
		widgGetFromID(psWScreen, IDDES_STOREBUTTON)->setTip(_("Store Template"));
	}

	widgSetUserData2(psWScreen, IDDES_STOREBUTTON, imageset);
}
