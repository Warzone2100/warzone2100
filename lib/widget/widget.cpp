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
/** @file
 *  The main interface functions to the widget library
 */

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/frameint.h"
#include "lib/framework/utf.h"
#include "lib/ivis_opengl/textdraw.h"

#include "widget.h"
#include "widgint.h"

#include "form.h"
#include "label.h"
#include "button.h"
#include "editbox.h"
#include "bar.h"
#include "slider.h"
#include "tip.h"

static	bool	bWidgetsActive = true;

/* The widget the mouse is over this update */
static WIDGET	*psMouseOverWidget = NULL;

static WIDGET_KEY pressed, released;
static WIDGET_AUDIOCALLBACK AudioCallback = NULL;
static SWORD HilightAudioID = -1;
static SWORD ClickedAudioID = -1;

/* Function prototypes */
void widgHiLite(WIDGET *psWidget, W_CONTEXT *psContext);
void widgHiLiteLost(WIDGET *psWidget, W_CONTEXT *psContext);
static void widgReleased(WIDGET *psWidget, UDWORD key, W_CONTEXT *psContext);
static void widgRun(WIDGET *psWidget, W_CONTEXT *psContext);
static void widgDisplayForm(W_FORM *psForm, UDWORD xOffset, UDWORD yOffset);
static void widgRelease(WIDGET *psWidget);

/* Buffer to return strings in */
static char aStringRetBuffer[WIDG_MAXSTR];

/* Initialise the widget module */
bool widgInitialise()
{
	tipInitialise();

	return true;
}


// Reset the widgets.
//
void widgReset(void)
{
	tipInitialise();
}


/* Shut down the widget module */
void widgShutDown(void)
{
}


W_INIT::W_INIT()
	: formID(0)
	, majorID(0), minorID(0)
	, id(0)
	, style(0)
	, x(0), y(0)
	, width(0), height(0)
	, pDisplay(NULL)
	, pCallback(NULL)
	, pUserData(NULL)
	, UserData(0)
{}

WIDGET::WIDGET(W_INIT const *init, WIDGET_TYPE type)
	: formID(init->formID)
	, id(init->id)
	, type(type)
	, style(init->style)
	, x(init->x), y(init->y)
	, width(init->width), height(init->height)
	, display(init->pDisplay)
	, callback(init->pCallback)
	, pUserData(init->pUserData)
	, UserData(init->UserData)

	, psNext(NULL)
{}


// reset psMouseOverWidget (a global) when needed
void CheckpsMouseOverWidget(void *psWidget)
{
	// in formFreePlain() (and maybe the others?) it is possible to free() the form
	// thus invalidating this pointer, causing a crash in widgDelete()
	if ((WIDGET *)psWidget == psMouseOverWidget)
	{
		debug(LOG_WARNING, "psMouseOverWidget (%p) has become dangling. Reseting.", psMouseOverWidget);
		psMouseOverWidget = NULL;
	}
}

/* Create an empty widget screen */
W_SCREEN *widgCreateScreen()
{
	W_SCREEN *psScreen = new W_SCREEN;
	if (psScreen == NULL)
	{
		debug(LOG_FATAL, "widgCreateScreen: Out of memory");
		abort();
		return NULL;
	}

	W_FORMINIT sInit;
	sInit.id = 0;
	sInit.style = WFORM_PLAIN | WFORM_INVISIBLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = (UWORD)(screenWidth - 1);
	sInit.height = (UWORD)(screenHeight - 1);

	W_FORM *psForm = formCreate(&sInit);
	if (psForm == NULL)
	{
		delete psScreen;
		return NULL;
	}

	psScreen->psForm = psForm;
	psScreen->psFocus = NULL;
	psScreen->TipFontID = font_regular;

	return psScreen;
}


/* Release a list of widgets */
void widgReleaseWidgetList(WIDGET *psWidgets)
{
	WIDGET	*psCurr, *psNext;

	for (psCurr = psWidgets; psCurr; psCurr = psNext)
	{
		psNext = psCurr->psNext;

		// the mouse can't be over it anymore
		if (psMouseOverWidget && psMouseOverWidget->id == psCurr->id)
		{
			psMouseOverWidget = NULL;
		}
		widgRelease(psCurr);
	}
}

/* Release a screen and all its associated data */
void widgReleaseScreen(W_SCREEN *psScreen)
{
	ASSERT(psScreen != NULL,
	       "widgReleaseScreen: Invalid screen pointer");

	formFree((W_FORM *)psScreen->psForm);

	delete psScreen;
}


/* Release a widget */
static void widgRelease(WIDGET *psWidget)
{
	switch (psWidget->type)
	{
	case WIDG_FORM:
		formFree((W_FORM *)psWidget);
		break;
	case WIDG_LABEL:
		labelFree((W_LABEL *)psWidget);
		break;
	case WIDG_BUTTON:
		buttonFree((W_BUTTON *)psWidget);
		break;
	case WIDG_EDITBOX:
		delete psWidget;
		break;
	case WIDG_BARGRAPH:
		barGraphFree((W_BARGRAPH *)psWidget);
		break;
	case WIDG_SLIDER:
		sliderFree((W_SLIDER *)psWidget);
		break;
	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}

/* Check whether an ID has been used on a form */
static bool widgCheckIDForm(W_FORM *psForm, UDWORD id)
{
	WIDGET			*psCurr;
	W_FORMGETALL	sGetAll;

	/* Check the widgets on the form */
	formInitGetAllWidgets(psForm, &sGetAll);
	psCurr = formGetAllWidgets(&sGetAll);
	while (psCurr != NULL)
	{
		if (psCurr->id == id)
		{
			return true;
		}

		if (psCurr->type == WIDG_FORM)
		{
			/* Another form so recurse */
			if (widgCheckIDForm((W_FORM *)psCurr, id))
			{
				return true;
			}
		}

		psCurr = psCurr->psNext;
		if (!psCurr)
		{
			/* Got to the end of this list see if there is another */
			psCurr = formGetAllWidgets(&sGetAll);
		}
	}

	return false;
}

/* Set the tool tip font for a screen */
void widgSetTipFont(W_SCREEN *psScreen, enum iV_fonts FontID)
{
	ASSERT(psScreen != NULL,
	       "widgSetTipFont: Invalid screen pointer");

	psScreen->TipFontID = FontID;
}

/* Add a form to the widget screen */
bool widgAddForm(W_SCREEN *psScreen, const W_FORMINIT *psInit)
{
	W_FORM	*psParent, *psForm;

	ASSERT(psScreen != NULL,
	       "widgAddForm: Invalid screen pointer");

	if (widgCheckIDForm((W_FORM *)psScreen->psForm, psInit->id))
	{
		ASSERT(false, "widgAddForm: ID number has already been used (%d)", psInit->id);
		return false;
	}

	/* Find the form to add the widget to */
	if (psInit->formID == 0)
	{
		/* Add to the base form */
		psParent = (W_FORM *)psScreen->psForm;
	}
	else
	{
		psParent = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
		if (!psParent || psParent->type != WIDG_FORM)
		{
			ASSERT(false,
			       "widgAddForm: Could not find parent form from formID");
			return false;
		}
	}

	/* Create the form structure */
	psForm = formCreate(psInit);
	if (psForm == NULL
	    /* Add it to the screen */
	    || !formAddWidget(psParent, (WIDGET *)psForm, (W_INIT *)psInit))
	{
		return false;
	}

	return true;
}


/* Add a label to the widget screen */
bool widgAddLabel(W_SCREEN *psScreen, const W_LABINIT *psInit)
{
	W_LABEL		*psLabel;
	W_FORM		*psForm;

	ASSERT(psScreen != NULL,
	       "widgAddLabel: Invalid screen pointer");

	if (widgCheckIDForm((W_FORM *)psScreen->psForm, psInit->id))
	{
		ASSERT(false, "widgAddLabel: ID number has already been used (%d)", psInit->id);
		return false;
	}

	/* Find the form to put the button on */
	if (psInit->formID == 0)
	{
		psForm = (W_FORM *)psScreen->psForm;
	}
	else
	{
		psForm = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
		if (psForm == NULL || psForm->type != WIDG_FORM)
		{
			ASSERT(false,
			       "widgAddLabel: Could not find parent form from formID");
			return false;
		}
	}

	/* Create the button structure */
	psLabel = labelCreate(psInit);
	if (psInit == NULL
	    /* Add it to the form */
	    || !formAddWidget(psForm, (WIDGET *)psLabel, (W_INIT *)psInit))
	{
		return false;
	}

	return true;
}


/* Add a button to the widget screen */
bool widgAddButton(W_SCREEN *psScreen, const W_BUTINIT *psInit)
{
	W_BUTTON	*psButton;
	W_FORM		*psForm;

	ASSERT(psScreen != NULL,
	       "widgAddButton: Invalid screen pointer");

	if (widgCheckIDForm((W_FORM *)psScreen->psForm, psInit->id))
	{
		ASSERT(false, "widgAddButton: ID number has already been used(%d)", psInit->id);
		return false;
	}

	/* Find the form to put the button on */
	if (psInit->formID == 0)
	{
		psForm = (W_FORM *)psScreen->psForm;
	}
	else
	{
		psForm = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
		if (psForm == NULL || psForm->type != WIDG_FORM)
		{
			ASSERT(false,
			       "widgAddButton: Could not find parent form from formID");
			return false;
		}
	}

	/* Create the button structure */
	psButton = buttonCreate(psInit);
	if (psButton == NULL
	    /* Add it to the form */
	    || !formAddWidget(psForm, (WIDGET *)psButton, (W_INIT *)psInit))
	{
		return false;
	}

	return true;
}


/* Add an edit box to the widget screen */
bool widgAddEditBox(W_SCREEN *psScreen, const W_EDBINIT *psInit)
{
	W_EDITBOX	*psEdBox;
	W_FORM		*psForm;

	ASSERT(psScreen != NULL,
	       "widgAddEditBox: Invalid screen pointer");

	if (widgCheckIDForm((W_FORM *)psScreen->psForm, psInit->id))
	{
		ASSERT(false, "widgAddEditBox: ID number has already been used (%d)", psInit->id);
		return false;
	}

	/* Find the form to put the edit box on */
	if (psInit->formID == 0)
	{
		psForm = (W_FORM *)psScreen->psForm;
	}
	else
	{
		psForm = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
		if (!psForm || psForm->type != WIDG_FORM)
		{
			ASSERT(false,
			       "widgAddEditBox: Could not find parent form from formID");
			return false;
		}
	}

	/* Create the edit box structure */
	psEdBox = editBoxCreate(psInit);
	if (psEdBox == NULL
	    /* Add it to the form */
	    || !formAddWidget(psForm, (WIDGET *)psEdBox, (W_INIT *)psInit))
	{
		return false;
	}

	return true;
}


/* Add a bar graph to the widget screen */
bool widgAddBarGraph(W_SCREEN *psScreen, const W_BARINIT *psInit)
{
	W_BARGRAPH	*psBarGraph;
	W_FORM		*psForm;

	ASSERT(psScreen != NULL,
	       "widgAddEditBox: Invalid screen pointer");

	if (widgCheckIDForm((W_FORM *)psScreen->psForm, psInit->id))
	{
		ASSERT(false, "widgAddBarGraph: ID number has already been used (%d)", psInit->id);
		return false;
	}

	/* Find the form to put the bar graph on */
	if (psInit->formID == 0)
	{
		psForm = (W_FORM *)psScreen->psForm;
	}
	else
	{
		psForm = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
		if (!psForm || psForm->type != WIDG_FORM)
		{
			ASSERT(false,
			       "widgAddBarGraph: Could not find parent form from formID");
			return false;
		}
	}

	/* Create the bar graph structure */
	psBarGraph = barGraphCreate(psInit);
	if (psBarGraph == NULL
	    /* Add it to the form */
	    || !formAddWidget(psForm, (WIDGET *)psBarGraph, (W_INIT *)psInit))
	{
		return false;
	}

	return true;
}


/* Add a slider to a form */
bool widgAddSlider(W_SCREEN *psScreen, const W_SLDINIT *psInit)
{
	W_SLIDER	*psSlider;
	W_FORM		*psForm;

	ASSERT(psScreen != NULL,
	       "widgAddEditBox: Invalid screen pointer");

	if (widgCheckIDForm((W_FORM *)psScreen->psForm, psInit->id))
	{
		ASSERT(false, "widgSlider: ID number has already been used (%d)", psInit->id);
		return false;
	}

	/* Find the form to put the slider on */
	if (psInit->formID == 0)
	{
		psForm = (W_FORM *)psScreen->psForm;
	}
	else
	{
		psForm = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
		if (!psForm
		    || psForm->type != WIDG_FORM)
		{
			ASSERT(false, "widgAddSlider: Could not find parent form from formID");
			return false;
		}
	}

	/* Create the slider structure */
	psSlider = sliderCreate(psInit);
	if (psSlider == NULL
	    /* Add it to the form */
	    || !formAddWidget(psForm, (WIDGET *)psSlider, (W_INIT *)psInit))
	{
		return false;
	}

	return true;
}


/* Delete a widget from a form */
static bool widgDeleteFromForm(W_FORM *psForm, UDWORD id, W_CONTEXT *psContext)
{
	WIDGET		*psPrev = NULL, *psCurr, *psNext;
	W_TABFORM	*psTabForm;
	UDWORD		minor, major;
	W_MAJORTAB	*psMajor;
	W_MINORTAB	*psMinor;
	W_CONTEXT	sNewContext;

	/* Clear the last hilite if necessary */
	if ((psForm->psLastHiLite != NULL) && (psForm->psLastHiLite->id == id))
	{
		widgHiLiteLost(psForm->psLastHiLite, psContext);
		psForm->psLastHiLite = NULL;
	}

	if (psForm->style & WFORM_TABBED)
	{
		psTabForm = (W_TABFORM *)psForm;
		ASSERT(psTabForm != NULL,
		       "widgDeleteFromForm: Invalid form pointer");

		/* loop through all the tabs */
		psMajor = psTabForm->asMajor;
		for (major = 0; major < psTabForm->numMajor; major++)
		{
			psMinor = psMajor->asMinor;
			for (minor = 0; minor < psMajor->numMinor; minor++)
			{
				if (psMinor->psWidgets && psMinor->psWidgets->id == id)
				{
					/* The widget is the first on this tab */
					psNext = psMinor->psWidgets->psNext;
					widgRelease(psMinor->psWidgets);
					psMinor->psWidgets = psNext;

					return true;
				}
				else
				{
					for (psCurr = psMinor->psWidgets; psCurr; psCurr = psCurr->psNext)
					{
						if (psCurr->id == id)
						{
							psPrev->psNext = psCurr->psNext;
							widgRelease(psCurr);

							return true;
						}
						if (psCurr->type == WIDG_FORM)
						{
							/* Recurse down to other form */
							sNewContext.psScreen = psContext->psScreen;
							sNewContext.psForm = (W_FORM *)psCurr;
							sNewContext.xOffset = psContext->xOffset - psCurr->x;
							sNewContext.yOffset = psContext->yOffset - psCurr->y;
							sNewContext.mx = psContext->mx - psCurr->x;
							sNewContext.my = psContext->my - psCurr->y;
							if (widgDeleteFromForm((W_FORM *)psCurr, id, &sNewContext))
							{
								return true;
							}
						}
						psPrev = psCurr;
					}
				}
				psMinor++;
			}
			psMajor++;
		}
	}
	else
	{
		ASSERT(psForm != NULL,
		       "widgDeleteFromForm: Invalid form pointer");

		/* Delete from a normal form */
		if (psForm->psWidgets && psForm->psWidgets->id == id)
		{
			/* The widget is the first in the list */
			psNext = psForm->psWidgets->psNext;
			widgRelease(psForm->psWidgets);
			psForm->psWidgets = psNext;

			return true;
		}
		else
		{
			/* Search the rest of the list */
			for (psCurr = psForm->psWidgets; psCurr; psCurr = psCurr->psNext)
			{
				if (psCurr->id == id)
				{
					psPrev->psNext = psCurr->psNext;
					widgRelease(psCurr);

					return true;
				}
				if (psCurr->type == WIDG_FORM)
				{
					/* Recurse down to other form */
					sNewContext.psScreen = psContext->psScreen;
					sNewContext.psForm = (W_FORM *)psCurr;
					sNewContext.xOffset = psContext->xOffset - psCurr->x;
					sNewContext.yOffset = psContext->yOffset - psCurr->y;
					sNewContext.mx = psContext->mx - psCurr->x;
					sNewContext.my = psContext->my - psCurr->y;
					if (widgDeleteFromForm((W_FORM *)psCurr, id, &sNewContext))
					{
						return true;
					}
				}
				psPrev = psCurr;
			}
		}
	}

	return false;
}


/* Delete a widget from the screen */
void widgDelete(W_SCREEN *psScreen, UDWORD id)
{
	W_CONTEXT	sContext;

	ASSERT(psScreen != NULL,
	       "widgDelete: Invalid screen pointer");

	/* Clear the keyboard focus if necessary */
	if ((psScreen->psFocus != NULL) && (psScreen->psFocus->id == id))
	{
		screenClearFocus(psScreen);
	}

	// NOTE: This is where it would crash because of a dangling pointer. See CheckpsMouseOverWidget() for info.
	// the mouse can't be over it anymore
	if (psMouseOverWidget && psMouseOverWidget->id == id)
	{
		psMouseOverWidget = NULL;
	}

	/* Set up the initial context */
	sContext.psScreen = psScreen;
	sContext.psForm = (W_FORM *)psScreen->psForm;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	sContext.mx = mouseX();
	sContext.my = mouseY();
	(void)widgDeleteFromForm((W_FORM *)psScreen->psForm, id, &sContext);
}


/* Initialise a form and all it's widgets */
static void widgStartForm(W_FORM *psForm)
{
	WIDGET			*psCurr;
	W_FORMGETALL	sGetAll;

	/* Initialise this form */
	// This whole function should be redundant, since all widgets are initialised when created...
	//formInitialise(psForm);

	/*Initialise the widgets on the form */
	formInitGetAllWidgets(psForm, &sGetAll);
	psCurr = formGetAllWidgets(&sGetAll);
	while (psCurr != NULL)
	{
		switch (psCurr->type)
		{
		case WIDG_FORM:
			widgStartForm((W_FORM *)psCurr);
			break;
		case WIDG_LABEL:
			break;
		case WIDG_BUTTON:
			buttonInitialise((W_BUTTON *)psCurr);
			break;
		case WIDG_EDITBOX:
			((W_EDITBOX *)psCurr)->initialise();
			break;
		case WIDG_BARGRAPH:
			break;
		case WIDG_SLIDER:
			sliderInitialise((W_SLIDER *)psCurr);
			break;
		default:
			ASSERT(!"Unknown widget type", "Unknown widget type");
			break;
		}

		psCurr = psCurr->psNext;
		if (!psCurr)
		{
			/* Got to the end of this list see if there is another */
			psCurr = formGetAllWidgets(&sGetAll);
		}
	}
}

/* Initialise the set of widgets that make up a screen */
void widgStartScreen(W_SCREEN *psScreen)
{
	psScreen->psFocus = NULL;
	widgStartForm((W_FORM *)psScreen->psForm);
}

/* Clean up after a screen has been run */
void widgEndScreen(W_SCREEN *psScreen)
{
	(void)psScreen;
}

/* Find a widget on a form from it's id number */
static WIDGET *widgFormGetFromID(W_FORM *psForm, UDWORD id)
{
	WIDGET			*psCurr, *psFound;
	W_FORMGETALL	sGetAll;

	/* See if the form matches the ID */
	if (psForm->id == id)
	{
		return (WIDGET *)psForm;
	}

	/* Now search the widgets on the form */
	psFound = NULL;
	formInitGetAllWidgets(psForm, &sGetAll);
	psCurr = formGetAllWidgets(&sGetAll);
	while (psCurr && !psFound)
	{
		if (psCurr->id == id)
		{
			psFound = psCurr;
		}
		else if (psCurr->type == WIDG_FORM)
		{
			psFound = widgFormGetFromID((W_FORM *)psCurr, id);
		}

		psCurr = psCurr->psNext;
		if (!psCurr)
		{
			/* Got to the end of this list see if there is another */
			psCurr = formGetAllWidgets(&sGetAll);
		}
	}

	return psFound;
}

/* Find a widget in a screen from its ID number */
WIDGET *widgGetFromID(W_SCREEN *psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(NULL, psScreen != NULL, "Invalid screen pointer");
	return widgFormGetFromID((W_FORM *)psScreen->psForm, id);
}


/* Hide a widget */
void widgHide(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	ASSERT(psWidget != NULL,
	       "widgHide: couldn't find widget from id");
	if (psWidget)
	{
		psWidget->style |= WIDG_HIDDEN;
	}
}


/* Reveal a widget */
void widgReveal(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	ASSERT(psWidget != NULL,
	       "widgReveal: couldn't find widget from id");
	if (psWidget)
	{
		psWidget->style &= ~WIDG_HIDDEN;
	}
}


/* Get the current position of a widget */
void widgGetPos(W_SCREEN *psScreen, UDWORD id, SWORD *pX, SWORD *pY)
{
	WIDGET	*psWidget;

	/* Find the widget */
	psWidget = widgGetFromID(psScreen, id);
	if (psWidget != NULL)
	{
		*pX = psWidget->x;
		*pY = psWidget->y;
	}
	else
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find widget by ID");
		*pX = 0;
		*pY = 0;
	}
}

/* Return the ID of the widget the mouse was over this frame */
UDWORD widgGetMouseOver(W_SCREEN *psScreen)
{
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if psMouseOverWidget needs to stop being a static and moves into
	   the screen structure */
	(void)psScreen;

	if (psMouseOverWidget == NULL)
	{
		return 0;
	}

	return psMouseOverWidget->id;
}


/* Return the user data for a widget */
void *widgGetUserData(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->pUserData;
	}

	return NULL;
}


/* Return the user data for a widget */
UDWORD widgGetUserData2(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->UserData;
	}

	return 0;
}


/* Set user data for a widget */
void widgSetUserData(W_SCREEN *psScreen, UDWORD id, void *UserData)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->pUserData = UserData;
	}
}

/* Set user data for a widget */
void widgSetUserData2(W_SCREEN *psScreen, UDWORD id, UDWORD UserData)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->UserData = UserData;
	}
}



/* Return the user data for the returned widget */
void *widgGetLastUserData(W_SCREEN *psScreen)
{
	assert(psScreen != NULL);

	if (psScreen->psRetWidget)
	{
		return psScreen->psRetWidget->pUserData;
	}

	return NULL;
}

/* Set tip string for a widget */
void widgSetTip(W_SCREEN *psScreen, UDWORD id, const char *pTip)
{
	WIDGET *psWidget = widgGetFromID(psScreen, id);

	if (!psWidget)
	{
		return;
	}

	widgSetTipText(psWidget, pTip);
}

void widgSetTipText(WIDGET *psWidget, const char *pTip)
{
	ASSERT(psWidget != NULL, "invalid widget pointer");

	switch (psWidget->type)
	{
	case WIDG_FORM:
		if (psWidget->style & WFORM_CLICKABLE)
		{
			((W_CLICKFORM *) psWidget)->pTip = pTip;
		}
		else if (psWidget->style & WFORM_TABBED)
		{
			ASSERT(!"tabbed forms don't have a tip", "widgSetTip: tabbed forms do not have a tip");
		}
		else
		{
			ASSERT(!"plain forms don't have a tip", "widgSetTip: plain forms do not have a tip");
		}
		break;

	case WIDG_LABEL:
		((W_LABEL *) psWidget)->pTip = pTip;
		break;

	case WIDG_BUTTON:
		((W_BUTTON *) psWidget)->pTip = pTip;
		break;

	case WIDG_BARGRAPH:
		((W_BARGRAPH *) psWidget)->pTip = pTip;
		break;

	case WIDG_SLIDER:
		((W_SLIDER *) psWidget)->pTip = pTip;
		break;

	case WIDG_EDITBOX:
		ASSERT(!"wrong widget type", "widgSetTip: edit boxes do not have a tip");
		break;

	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}

/* Return which key was used to press the last returned widget */
UDWORD widgGetButtonKey(W_SCREEN *psScreen)
{
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if released needs to stop being a static and moves into
	   the screen structure */
	(void)psScreen;

	return released;
}


/* Get a button or clickable form's state */
UDWORD widgGetButtonState(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	/* Get the button */
	psWidget = widgGetFromID(psScreen, id);
	if (psWidget == NULL)
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find button or clickable widget by ID");
	}
	else if (psWidget->type == WIDG_BUTTON)
	{
		return buttonGetState((W_BUTTON *)psWidget);
	}
	else if ((psWidget->type == WIDG_FORM) && (psWidget->style & WFORM_CLICKABLE))
	{
		return formGetClickState((W_CLICKFORM *)psWidget);
	}
	else
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find button or clickable widget by ID");
	}
	return 0;
}


void widgSetButtonFlash(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	/* Get the button */
	psWidget = widgGetFromID(psScreen, id);
	if (psWidget == NULL)
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find button or clickable widget by ID");
	}
	else if (psWidget->type == WIDG_BUTTON)
	{
		buttonSetFlash((W_BUTTON *)psWidget);
	}
	else if ((psWidget->type == WIDG_FORM) && (psWidget->style & WFORM_CLICKABLE))
	{
		formSetFlash((W_FORM *)psWidget);
	}
	else if (psWidget->type == WIDG_EDITBOX)
	{
//		editBoxSetState((W_EDITBOX *)psWidget, state);
	}
	else
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find button or clickable widget by ID");
	}
}


void widgClearButtonFlash(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	/* Get the button */
	psWidget = widgGetFromID(psScreen, id);
	if (psWidget == NULL)
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find button or clickable widget by ID");
	}
	else if (psWidget->type == WIDG_BUTTON)
	{
		buttonClearFlash((W_BUTTON *)psWidget);
	}
	else if ((psWidget->type == WIDG_FORM) && (psWidget->style & WFORM_CLICKABLE))
	{
		formClearFlash((W_FORM *)psWidget);
	}
	else if (psWidget->type == WIDG_EDITBOX)
	{
	}
	else
	{
		ASSERT(!"Couldn't find widget by ID", "Couldn't find button or clickable widget by ID");
	}
}


/* Set a button or clickable form's state */
void widgSetButtonState(W_SCREEN *psScreen, UDWORD id, UDWORD state)
{
	WIDGET	*psWidget;

	/* Get the button */
	psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find button or clickable widget by ID %u", state);

	if (psWidget->type == WIDG_BUTTON)
	{
		buttonSetState((W_BUTTON *)psWidget, state);
	}
	else if ((psWidget->type == WIDG_FORM) && (psWidget->style & WFORM_CLICKABLE))
	{
		formSetClickState((W_CLICKFORM *)psWidget, state);
	}
	else if (psWidget->type == WIDG_EDITBOX)
	{
		editBoxSetState((W_EDITBOX *)psWidget, state);
	}
	else
	{
		ASSERT(false, "Couldn't find button or clickable widget by type %d", (int)psWidget->type);
	}
}


/* Return a pointer to a buffer containing the current string of a widget.
 * NOTE: The string must be copied out of the buffer
 */
const char *widgGetString(W_SCREEN *psScreen, UDWORD id)
{
	const WIDGET *psWidget = widgGetFromID(psScreen, id);

	ASSERT(psScreen != NULL, "widgGetString: Invalid screen pointer");

	/* Get the widget */
	if (psWidget != NULL)
	{
		switch (psWidget->type)
		{
		case WIDG_FORM:
			ASSERT(false, "widgGetString: Forms do not have a string");
			aStringRetBuffer[0] = '\0';
			break;
		case WIDG_LABEL:
			sstrcpy(aStringRetBuffer, ((W_LABEL *)psWidget)->aText);
			break;
		case WIDG_BUTTON:
			if (((W_BUTTON *)psWidget)->pText)
			{
				sstrcpy(aStringRetBuffer, ((W_BUTTON *)psWidget)->pText);
			}
			else
			{
				aStringRetBuffer[0] = '\0';
			}
			break;
		case WIDG_EDITBOX:
			{
				sstrcpy(aStringRetBuffer, ((W_EDITBOX *)psWidget)->aText.toUtf8().constData());
				break;
			}
		case WIDG_BARGRAPH:
			ASSERT(false, "widgGetString: Bar Graphs do not have a string");
			aStringRetBuffer[0] = '\0';
			break;
		case WIDG_SLIDER:
			ASSERT(false, "widgGetString: Sliders do not have a string");
			aStringRetBuffer[0] = '\0';
			break;
		default:
			ASSERT(!"Unknown widget type", "Unknown widget type");
			aStringRetBuffer[0] = '\0';
			break;
		}
	}
	else
	{
		ASSERT(!"Couldn't find widget by ID", "widgGetString: couldn't find widget by ID");
		aStringRetBuffer[0] = '\0';
	}

	return aStringRetBuffer;
}


/* Set the text in a widget */
void widgSetString(W_SCREEN *psScreen, UDWORD id, const char *pText)
{
	WIDGET	*psWidget;

	ASSERT(psScreen != NULL,
	       "widgSetString: Invalid screen pointer");

	/* Get the widget */
	psWidget = widgGetFromID(psScreen, id);
	if (psWidget == NULL)
	{
		debug(LOG_ERROR, "widgSetString: couldn't get widget from id");
		return;
	}

	switch (psWidget->type)
	{
	case WIDG_FORM:
		ASSERT(false, "widgSetString: forms do not have a string");
		break;

	case WIDG_LABEL:
		sstrcpy(((W_LABEL *)psWidget)->aText, pText);
		break;

	case WIDG_BUTTON:
		((W_BUTTON *)psWidget)->pText = pText;
		break;

	case WIDG_EDITBOX:
		if (psScreen->psFocus == psWidget)
		{
			screenClearFocus(psScreen);
		}
		((W_EDITBOX *)psWidget)->setString(pText);
		break;

	case WIDG_BARGRAPH:
		ASSERT(!"wrong widget type", "widgGetString: Bar graphs do not have a string");
		break;

	case WIDG_SLIDER:
		ASSERT(!"wrong widget type", "widgGetString: Sliders do not have a string");
		break;

	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}


/* Call any callbacks for the widgets on a form */
static void widgProcessCallbacks(W_CONTEXT *psContext)
{
	WIDGET			*psCurr;
	W_CONTEXT		sFormContext, sWidgContext;
	SDWORD			xOrigin, yOrigin;
	W_FORMGETALL	sFormCtl;

	/* Initialise the form context */
	sFormContext.psScreen = psContext->psScreen;

	/* Initialise widget context */
	formGetOrigin(psContext->psForm, &xOrigin, &yOrigin);
	sWidgContext.psScreen = psContext->psScreen;
	sWidgContext.psForm = psContext->psForm;
	sWidgContext.mx = psContext->mx - xOrigin;
	sWidgContext.my = psContext->my - yOrigin;
	sWidgContext.xOffset = psContext->xOffset + xOrigin;
	sWidgContext.yOffset = psContext->yOffset + yOrigin;

	/* Go through all the widgets on the form */
	formInitGetAllWidgets(psContext->psForm, &sFormCtl);
	psCurr = formGetAllWidgets(&sFormCtl);
	while (psCurr)
	{
		for (; psCurr; psCurr = psCurr->psNext)
		{
			/* Call the callback */
			if (psCurr->callback)
			{
				psCurr->callback(psCurr, &sWidgContext);
			}

			/* and then recurse */
			if (psCurr->type == WIDG_FORM)
			{
				sFormContext.psForm = (W_FORM *)psCurr;
				sFormContext.mx = sWidgContext.mx - psCurr->x;
				sFormContext.my = sWidgContext.my - psCurr->y;
				sFormContext.xOffset = sWidgContext.xOffset + psCurr->x;
				sFormContext.yOffset = sWidgContext.yOffset + psCurr->y;
				widgProcessCallbacks(&sFormContext);
			}
		}

		/* See if the form has any more widgets on it */
		psCurr = formGetAllWidgets(&sFormCtl);
	}
}


/* Process all the widgets on a form.
 * mx and my are the coords of the mouse relative to the form origin.
 */
static void widgProcessForm(W_CONTEXT *psContext)
{
	WIDGET		*psCurr, *psOver;
	SDWORD		mx, my, omx, omy, xOffset, yOffset, xOrigin, yOrigin;
	W_FORM		*psForm;
	W_CONTEXT	sFormContext, sWidgContext;

	/* Note current form */
	psForm = psContext->psForm;

//	if(psForm->disableChildren == true) {
//		return;
//	}

	/* Note the current mouse position */
	mx = psContext->mx;
	my = psContext->my;

	/* Note the current offset */
	xOffset = psContext->xOffset;
	yOffset = psContext->yOffset;

	/* Initialise the form context */
	sFormContext.psScreen = psContext->psScreen;

	/* Initialise widget context */
	formGetOrigin(psForm, &xOrigin, &yOrigin);
	sWidgContext.psScreen = psContext->psScreen;
	sWidgContext.psForm = psForm;
	sWidgContext.mx = mx - xOrigin;
	sWidgContext.my = my - yOrigin;
	sWidgContext.xOffset = xOffset + xOrigin;
	sWidgContext.yOffset = yOffset + yOrigin;

	/* Process the form's widgets */
	psOver = NULL;
	for (psCurr = formGetWidgets(psForm); psCurr; psCurr = psCurr->psNext)
	{
		/* Skip any hidden widgets */
		if (psCurr->style & WIDG_HIDDEN)
		{
			continue;
		}

		if (psCurr->type == WIDG_FORM)
		{
			/* Found a sub form, so set up the context */
			sFormContext.psForm = (W_FORM *)psCurr;
			sFormContext.mx = mx - psCurr->x - xOrigin;
			sFormContext.my = my - psCurr->y - yOrigin;
			sFormContext.xOffset = xOffset + psCurr->x + xOrigin;
			sFormContext.yOffset = yOffset + psCurr->y + yOrigin;

			/* Process it */
			widgProcessForm(&sFormContext);
		}
		else
		{
			/* Run the widget */
			widgRun(psCurr, &sWidgContext);
		}
	}

	/* Now check for mouse clicks */
	omx = mx - xOrigin;
	omy = my - yOrigin;
	if (mx >= 0 && mx <= psForm->width &&
	    my >= 0 && my <= psForm->height)
	{
		/* Update for the origin */

		/* Mouse is over the form - is it over any of the widgets */
		for (psCurr = formGetWidgets(psForm); psCurr; psCurr = psCurr->psNext)
		{
			/* Skip any hidden widgets */
			if (psCurr->style & WIDG_HIDDEN)
			{
				continue;
			}

			if (omx >= psCurr->x &&
			    omy >= psCurr->y &&
			    omx <= psCurr->x + psCurr->width &&
			    omy <= psCurr->y + psCurr->height)
			{
				/* Note the widget the mouse is over */
				if (!psMouseOverWidget)
				{
					psMouseOverWidget = (WIDGET *)psCurr;
				}
				psOver = psCurr;

				/* Don't check the widgets if it is a clickable form */
				if (!(psForm->style & WFORM_CLICKABLE))
				{
					if (pressed != WKEY_NONE && psCurr->type != WIDG_FORM)
					{
						/* Tell the widget it has been clicked */
						psCurr->clicked(&sWidgContext, pressed);
					}
					if (released != WKEY_NONE && psCurr->type != WIDG_FORM)
					{
						/* Tell the widget the mouse button has gone up */
						widgReleased(psCurr, released, &sWidgContext);
					}
				}
			}
		}
		/* Note that the mouse is over this form */
		if (!psMouseOverWidget)
		{
			psMouseOverWidget = (WIDGET *)psForm;
		}

		/* Only send the Clicked or Released messages if a widget didn't get the message */
		if (pressed != WKEY_NONE &&
		    (psOver == NULL || (psForm->style & WFORM_CLICKABLE)))
		{
			/* Tell the form it has been clicked */
			psForm->clicked(psContext, pressed);
		}
		if (released != WKEY_NONE &&
		    (psOver == NULL || (psForm->style & WFORM_CLICKABLE)))
		{
			/* Tell the form the mouse button has gone up */
			widgReleased((WIDGET *)psForm, released, psContext);
		}
	}

	/* See if the mouse has moved onto or off a widget */
	if (psForm->psLastHiLite != psOver)
	{
		if (psOver != NULL)
		{
			widgHiLite(psOver, &sWidgContext);
		}
		if (psForm->psLastHiLite != NULL)
		{
			widgHiLiteLost(psForm->psLastHiLite, &sWidgContext);
		}
		psForm->psLastHiLite = psOver;
	}

	/* Run this form */
	widgRun((WIDGET *)psForm, psContext);
}



/* Execute a set of widgets for one cycle.
 * Return the id of the widget that was activated, or 0 for none.
 */
UDWORD widgRunScreen(W_SCREEN *psScreen)
{
	W_CONTEXT	sContext;

	psScreen->psRetWidget = NULL;

	// Note which keys have been pressed
	pressed = WKEY_NONE;
	sContext.mx = mouseX();
	sContext.my = mouseY();
	if (getWidgetsStatus())
	{
		if (mousePressed(MOUSE_LMB))
		{
			pressed = WKEY_PRIMARY;
			sContext.mx = mousePressPos(MOUSE_LMB).x;
			sContext.my = mousePressPos(MOUSE_LMB).y;
		}
		else if (mousePressed(MOUSE_RMB))
		{
			pressed = WKEY_SECONDARY;
			sContext.mx = mousePressPos(MOUSE_RMB).x;
			sContext.my = mousePressPos(MOUSE_RMB).y;
		}
		released = WKEY_NONE;
		if (mouseReleased(MOUSE_LMB))
		{
			released = WKEY_PRIMARY;
			sContext.mx = mouseReleasePos(MOUSE_LMB).x;
			sContext.my = mouseReleasePos(MOUSE_LMB).y;
		}
		else if (mouseReleased(MOUSE_RMB))
		{
			released = WKEY_SECONDARY;
			sContext.mx = mouseReleasePos(MOUSE_RMB).x;
			sContext.my = mouseReleasePos(MOUSE_RMB).y;
		}
	}
	/* Initialise the context */
	sContext.psScreen = psScreen;
	sContext.psForm = (W_FORM *)psScreen->psForm;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	psMouseOverWidget = NULL;

	/* Process the screen's widgets */
	widgProcessForm(&sContext);

	/* Process any user callback functions */
	widgProcessCallbacks(&sContext);

	/* Return the ID of a pressed button or finished edit box if any */
	return psScreen->psRetWidget ? psScreen->psRetWidget->id : 0;
}


/* Set the id number for widgRunScreen to return */
void widgSetReturn(W_SCREEN *psScreen, WIDGET *psWidget)
{
	psScreen->psRetWidget = psWidget;
}


/* Display the widgets on a form */
static void widgDisplayForm(W_FORM *psForm, UDWORD xOffset, UDWORD yOffset)
{
	WIDGET	*psCurr = NULL;
	SDWORD	xOrigin = 0, yOrigin = 0;


	/* Display the form */
	psForm->display((WIDGET *)psForm, xOffset, yOffset, psForm->aColours);
	if (psForm->disableChildren == true)
	{
		return;
	}

	/* Update the offset from the current form's position */
	formGetOrigin(psForm, &xOrigin, &yOrigin);
	xOffset += psForm->x + xOrigin;
	yOffset += psForm->y + yOrigin;

	/* If this is a clickable form, the widgets on it have to move when it's down */
	if (!(psForm->style & WFORM_NOCLICKMOVE))
	{
		if ((psForm->style & WFORM_CLICKABLE) &&
		    (((W_CLICKFORM *)psForm)->state &
		     (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)))
		{
			xOffset += 1;
			yOffset += 1;
		}
	}

	/* Display the widgets on the form */
	for (psCurr = formGetWidgets(psForm); psCurr; psCurr = psCurr->psNext)
	{
		/* Skip any hidden widgets */
		if (psCurr->style & WIDG_HIDDEN)
		{
			continue;
		}

		if (psCurr->type == WIDG_FORM)
		{
			widgDisplayForm((W_FORM *)psCurr, xOffset, yOffset);
		}
		else
		{
			psCurr->display(psCurr, xOffset, yOffset, psForm->aColours);
		}
	}
}



/* Display the screen's widgets in their current state
 * (Call after calling widgRunScreen, this allows the input
 *  processing to be seperated from the display of the widgets).
 */
void widgDisplayScreen(W_SCREEN *psScreen)
{
	/* Display the widgets */
	widgDisplayForm((W_FORM *)psScreen->psForm, 0, 0);

	/* Display the tool tip if there is one */
	tipDisplay();
}

/* Call the correct function for loss of focus */
static void widgFocusLost(W_SCREEN *psScreen, WIDGET *psWidget)
{
	switch (psWidget->type)
	{
	case WIDG_FORM:
		break;
	case WIDG_LABEL:
		break;
	case WIDG_BUTTON:
		break;
	case WIDG_EDITBOX:
		((W_EDITBOX *)psWidget)->focusLost(psScreen);
		break;
	case WIDG_BARGRAPH:
		break;
	case WIDG_SLIDER:
		break;
	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}

/* Set the keyboard focus for the screen */
void screenSetFocus(W_SCREEN *psScreen, WIDGET *psWidget)
{
	if (psScreen->psFocus != NULL)
	{
		widgFocusLost(psScreen, psScreen->psFocus);
	}
	psScreen->psFocus = psWidget;
}


/* Clear the keyboard focus */
void screenClearFocus(W_SCREEN *psScreen)
{
	if (psScreen->psFocus != NULL)
	{
		widgFocusLost(psScreen, psScreen->psFocus);
		psScreen->psFocus = NULL;
	}
}

/* Call the correct function for mouse over */
void widgHiLite(WIDGET *psWidget, W_CONTEXT *psContext)
{
	(void)psContext;
	switch (psWidget->type)
	{
	case WIDG_FORM:
		formHiLite((W_FORM *)psWidget, psContext);
		break;
	case WIDG_LABEL:
		labelHiLite((W_LABEL *)psWidget, psContext);
		break;
	case WIDG_BUTTON:
		buttonHiLite((W_BUTTON *)psWidget, psContext);
		break;
	case WIDG_EDITBOX:
		editBoxHiLite((W_EDITBOX *)psWidget);
		break;
	case WIDG_BARGRAPH:
		barGraphHiLite((W_BARGRAPH *)psWidget, psContext);
		break;
	case WIDG_SLIDER:
		sliderHiLite((W_SLIDER *)psWidget);
		break;
	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}


/* Call the correct function for mouse moving off */
void widgHiLiteLost(WIDGET *psWidget, W_CONTEXT *psContext)
{
	(void)psContext;
	switch (psWidget->type)
	{
	case WIDG_FORM:
		formHiLiteLost((W_FORM *)psWidget, psContext);
		break;
	case WIDG_LABEL:
		labelHiLiteLost((W_LABEL *)psWidget);
		break;
	case WIDG_BUTTON:
		buttonHiLiteLost((W_BUTTON *)psWidget);
		break;
	case WIDG_EDITBOX:
		editBoxHiLiteLost((W_EDITBOX *)psWidget);
		break;
	case WIDG_BARGRAPH:
		barGraphHiLiteLost((W_BARGRAPH *)psWidget);
		break;
	case WIDG_SLIDER:
		sliderHiLiteLost((W_SLIDER *)psWidget);
		break;
	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}

/* Call the correct function for mouse released */
static void widgReleased(WIDGET *psWidget, UDWORD key, W_CONTEXT *psContext)
{
	switch (psWidget->type)
	{
	case WIDG_FORM:
		formReleased((W_FORM *)psWidget, key, psContext);
		break;
	case WIDG_LABEL:
		break;
	case WIDG_BUTTON:
		buttonReleased(psContext->psScreen, (W_BUTTON *)psWidget, key);
		break;
	case WIDG_EDITBOX:
		break;
	case WIDG_BARGRAPH:
		break;
	case WIDG_SLIDER:
		sliderReleased((W_SLIDER *)psWidget);
		break;
	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}


/* Call the correct function to run a widget */
static void widgRun(WIDGET *psWidget, W_CONTEXT *psContext)
{
	switch (psWidget->type)
	{
	case WIDG_FORM:
		formRun((W_FORM *)psWidget, psContext);
		break;
	case WIDG_LABEL:
		break;
	case WIDG_BUTTON:
		buttonRun((W_BUTTON *)psWidget);
		break;
	case WIDG_EDITBOX:
		((W_EDITBOX *)psWidget)->run(psContext);
		break;
	case WIDG_BARGRAPH:
		break;
	case WIDG_SLIDER:
		sliderRun((W_SLIDER *)psWidget, psContext);
		break;
	default:
		ASSERT(!"Unknown widget type", "Unknown widget type");
		break;
	}
}



void WidgSetAudio(WIDGET_AUDIOCALLBACK Callback, SWORD HilightID, SWORD ClickedID)
{
	AudioCallback = Callback;
	HilightAudioID = HilightID;
	ClickedAudioID = ClickedID;
}


WIDGET_AUDIOCALLBACK WidgGetAudioCallback(void)
{
	return AudioCallback;
}


SWORD WidgGetHilightAudioID(void)
{
	return HilightAudioID;
}


SWORD WidgGetClickedAudioID(void)
{
	return ClickedAudioID;
}


void	setWidgetsStatus(bool var)
{
	bWidgetsActive = var;
}

bool	getWidgetsStatus(void)
{
	return(bWidgetsActive);
}
