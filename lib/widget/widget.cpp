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
#include "lib/framework/utf.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/gamelib/gtime.h"

#include "widget.h"
#if defined(WZ_CC_MSVC)
#include "widgbase_moc.h"		// this is generated on the pre-build event.
#endif
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

static WIDGET_AUDIOCALLBACK AudioCallback = NULL;
static SWORD HilightAudioID = -1;
static SWORD ClickedAudioID = -1;

static WIDGET_KEY lastReleasedKey_DEPRECATED = WKEY_NONE;

static std::vector<WIDGET *> widgetDeletionQueue;

static bool debugBoundingBoxesOnly = false;


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

static void deleteOldWidgets()
{
	while (!widgetDeletionQueue.empty())
	{
		WIDGET *guiltyWidget = widgetDeletionQueue.back();
		widgetDeletionQueue.pop_back();  // Do this before deleting widget, in case it calls deleteLater() on other widgets.

		ASSERT_OR_RETURN(, std::find(widgetDeletionQueue.begin(), widgetDeletionQueue.end(), guiltyWidget) == widgetDeletionQueue.end(), "Called deleteLater() twice on the same widget.");

		delete guiltyWidget;
	}
}

W_INIT::W_INIT()
	: formID(0)
	, majorID(0)
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
	: id(init->id)
	, type(type)
	, style(init->style)
	, displayFunction(init->pDisplay)
	, callback(init->pCallback)
	, pUserData(init->pUserData)
	, UserData(init->UserData)
	, screenPointer(nullptr)
	, parentWidget(NULL)
	, dim(init->x, init->y, init->width, init->height)
	, dirty(true)
{}

WIDGET::WIDGET(WIDGET *parent, WIDGET_TYPE type)
	: id(0xFFFFEEEEu)
	, type(type)
	, style(0)
	, displayFunction(NULL)
	, callback(NULL)
	, pUserData(NULL)
	, UserData(0)
	, screenPointer(nullptr)
	, parentWidget(NULL)
	, dim(0, 0, 1, 1)
	, dirty(true)
{
	parent->attach(this);
}

WIDGET::~WIDGET()
{
	setScreenPointer(nullptr);  // Clear any pointers to us directly from screenPointer.

	if (parentWidget != NULL)
	{
		parentWidget->detach(this);
	}
	for (unsigned n = 0; n < childWidgets.size(); ++n)
	{
		childWidgets[n]->parentWidget = NULL;  // Detach in advance, slightly faster than detach(), and doesn't change our list.
		delete childWidgets[n];
	}
}

void WIDGET::deleteLater()
{
	   widgetDeletionQueue.push_back(this);
}

void WIDGET::setGeometry(QRect const &r)
{
	if (dim == r)
	{
		return;  // Nothing to do.
	}
	dim = r;
	geometryChanged();
	dirty = true;
}

void WIDGET::attach(WIDGET *widget)
{
	ASSERT_OR_RETURN(, widget != NULL && widget->parentWidget == NULL, "Bad attach.");
	widget->parentWidget = this;
	widget->setScreenPointer(screenPointer);
	childWidgets.push_back(widget);
}

void WIDGET::detach(WIDGET *widget)
{
	ASSERT_OR_RETURN(, widget != NULL && widget->parentWidget != NULL, "Bad detach.");

	widget->parentWidget = NULL;
	widget->setScreenPointer(NULL);
	childWidgets.erase(std::find(childWidgets.begin(), childWidgets.end(), widget));

	widgetLost(widget);
}

void WIDGET::setScreenPointer(W_SCREEN *screen)
{
	if (screenPointer == screen)
	{
		return;
	}

	if (psMouseOverWidget == this)
	{
		psMouseOverWidget = nullptr;
	}
	if (screenPointer != nullptr && screenPointer->psFocus == this)
	{
		screenPointer->psFocus = nullptr;
	}
	if (screenPointer != nullptr && screenPointer->lastHighlight == this)
	{
		screenPointer->lastHighlight = nullptr;
	}

	screenPointer = screen;
	for (Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		(*i)->setScreenPointer(screen);
	}
}

void WIDGET::widgetLost(WIDGET *widget)
{
	if (parentWidget != NULL)
	{
		parentWidget->widgetLost(widget);  // We don't care about the lost widget, maybe the parent does. (Special code for W_TABFORM.)
	}
}

W_SCREEN::W_SCREEN()
	: psFocus(NULL)
	, lastHighlight(nullptr)
	, TipFontID(font_regular)
{
	W_FORMINIT sInit;
	sInit.id = 0;
	sInit.style = WFORM_PLAIN | WFORM_INVISIBLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;

	psForm = new W_FORM(&sInit);
	psForm->screenPointer = this;
}

W_SCREEN::~W_SCREEN()
{
	delete psForm;
}

/* Check whether an ID has been used on a form */
static bool widgCheckIDForm(WIDGET *psForm, UDWORD id)
{
	if (psForm->id == id)
	{
		return true;
	}
	for (WIDGET::Children::const_iterator i = psForm->children().begin(); i != psForm->children().end(); ++i)
	{
		if (widgCheckIDForm(*i, id))
		{
			return true;
		}
	}
	return false;
}

static bool widgAddWidget(W_SCREEN *psScreen, W_INIT const *psInit, WIDGET *widget)
{
	ASSERT_OR_RETURN(false, widget != NULL, "Invalid widget");
	ASSERT_OR_RETURN(false, psScreen != NULL, "Invalid screen pointer");
	ASSERT_OR_RETURN(false, !widgCheckIDForm(psScreen->psForm, psInit->id), "ID number has already been used (%d)", psInit->id);
	// Find the form to add the widget to.
	W_FORM *psParent;
	if (psInit->formID == 0)
	{
		/* Add to the base form */
		psParent = psScreen->psForm;
	}
	else
	{
		psParent = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
	}
	ASSERT_OR_RETURN(false, psParent != NULL && psParent->type == WIDG_FORM, "Could not find parent form from formID");

	psParent->attach(widget);
	return true;
}

/* Add a form to the widget screen */
W_FORM *widgAddForm(W_SCREEN *psScreen, const W_FORMINIT *psInit)
{
	W_FORM *psForm;
	if (psInit->style & WFORM_CLICKABLE)
	{
		psForm = new W_CLICKFORM(psInit);
	}
	else
	{
		psForm = new W_FORM(psInit);
	}

	return widgAddWidget(psScreen, psInit, psForm)? psForm : nullptr;
}

/* Add a label to the widget screen */
W_LABEL *widgAddLabel(W_SCREEN *psScreen, const W_LABINIT *psInit)
{
	W_LABEL *psLabel = new W_LABEL(psInit);
	return widgAddWidget(psScreen, psInit, psLabel)? psLabel : nullptr;
}

/* Add a button to the widget screen */
W_BUTTON *widgAddButton(W_SCREEN *psScreen, const W_BUTINIT *psInit)
{
	W_BUTTON *psButton = new W_BUTTON(psInit);
	return widgAddWidget(psScreen, psInit, psButton)? psButton : nullptr;
}

/* Add an edit box to the widget screen */
W_EDITBOX *widgAddEditBox(W_SCREEN *psScreen, const W_EDBINIT *psInit)
{
	W_EDITBOX *psEdBox = new W_EDITBOX(psInit);
	return widgAddWidget(psScreen, psInit, psEdBox)? psEdBox : nullptr;
}

/* Add a bar graph to the widget screen */
W_BARGRAPH *widgAddBarGraph(W_SCREEN *psScreen, const W_BARINIT *psInit)
{
	W_BARGRAPH *psBarGraph = new W_BARGRAPH(psInit);
	return widgAddWidget(psScreen, psInit, psBarGraph)? psBarGraph : nullptr;
}

/* Add a slider to a form */
W_SLIDER *widgAddSlider(W_SCREEN *psScreen, const W_SLDINIT *psInit)
{
	W_SLIDER *psSlider = new W_SLIDER(psInit);
	return widgAddWidget(psScreen, psInit, psSlider)? psSlider : nullptr;
}

/* Delete a widget from the screen */
void widgDelete(W_SCREEN *psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(, psScreen != NULL, "Invalid screen pointer");

	delete widgGetFromID(psScreen, id);
}

/* Find a widget on a form from its id number */
static WIDGET *widgFormGetFromID(WIDGET *widget, UDWORD id)
{
	if (widget->id == id)
	{
		return widget;
	}
	WIDGET::Children const &c = widget->children();
	for (WIDGET::Children::const_iterator i = c.begin(); i != c.end(); ++i)
	{
		WIDGET *w = widgFormGetFromID(*i, id);
		if (w != NULL)
		{
			return w;
		}
	}
	return NULL;
}

/* Find a widget in a screen from its ID number */
WIDGET *widgGetFromID(W_SCREEN *psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(NULL, psScreen != NULL, "Invalid screen pointer");
	return widgFormGetFromID(psScreen->psForm, id);
}

void widgHide(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != NULL, "Couldn't find widget from id.");

	psWidget->hide();
}

void widgReveal(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != NULL, "Couldn't find widget from id.");

	psWidget->show();
}


/* Get the current position of a widget */
void widgGetPos(W_SCREEN *psScreen, UDWORD id, SWORD *pX, SWORD *pY)
{
	WIDGET	*psWidget;

	/* Find the widget */
	psWidget = widgGetFromID(psScreen, id);
	if (psWidget != NULL)
	{
		*pX = psWidget->x();
		*pY = psWidget->y();
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

void WIDGET::setTip(QString)
{
	ASSERT(false, "Can't set widget type %u's tip.", type);
}

/* Set tip string for a widget */
void widgSetTip(W_SCREEN *psScreen, UDWORD id, QString pTip)
{
	WIDGET *psWidget = widgGetFromID(psScreen, id);

	if (!psWidget)
	{
		return;
	}

	psWidget->setTip(pTip);
}

/* Return which key was used to press the last returned widget */
UDWORD widgGetButtonKey_DEPRECATED(W_SCREEN *psScreen)
{
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if released needs to stop being a static and moves into
	   the screen structure */
	(void)psScreen;

	return lastReleasedKey_DEPRECATED;
}

unsigned WIDGET::getState()
{
	ASSERT(false, "Can't get widget type %u's state.", type);
	return 0;
}

/* Get a button or clickable form's state */
UDWORD widgGetButtonState(W_SCREEN *psScreen, UDWORD id)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(0, psWidget, "Couldn't find widget by ID %u", id);

	return psWidget->getState();
}

void WIDGET::setFlash(bool)
{
	ASSERT(false, "Can't set widget type %u's flash.", type);
}

void widgSetButtonFlash(W_SCREEN *psScreen, UDWORD id)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	psWidget->setFlash(true);
}

void widgClearButtonFlash(W_SCREEN *psScreen, UDWORD id)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	psWidget->setFlash(false);
}


void WIDGET::setState(unsigned)
{
	ASSERT(false, "Can't set widget type %u's state.", type);
}

/* Set a button or clickable form's state */
void widgSetButtonState(W_SCREEN *psScreen, UDWORD id, UDWORD state)
{
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	psWidget->setState(state);
}

QString WIDGET::getString() const
{
	ASSERT(false, "Can't get widget type %u's string.", type);
	return QString();
}

/* Return a pointer to a buffer containing the current string of a widget.
 * NOTE: The string must be copied out of the buffer
 */
const char *widgGetString(W_SCREEN *psScreen, UDWORD id)
{
	const WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN("", psWidget, "Couldn't find widget by ID %u", id);

	static QByteArray ret;  // Must be static so it isn't immediately freed when this function returns.
	ret = psWidget->getString().toUtf8();
	return ret.constData();
}

void WIDGET::setString(QString)
{
	ASSERT(false, "Can't set widget type %u's string.", type);
}

/* Set the text in a widget */
void widgSetString(W_SCREEN *psScreen, UDWORD id, const char *pText)
{
	/* Get the widget */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	if (psWidget->type == WIDG_EDITBOX && psScreen->psFocus == psWidget)
	{
		psScreen->setFocus(nullptr);
	}

	psWidget->setString(QString::fromUtf8(pText));
}

void WIDGET::processCallbacksRecursive(W_CONTEXT *psContext)
{
	/* Go through all the widgets on the form */
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		/* Call the callback */
		if (psCurr->callback)
		{
			psCurr->callback(psCurr, psContext);
		}

		/* and then recurse */
		W_CONTEXT sFormContext;
		sFormContext.mx = psContext->mx - psCurr->x();
		sFormContext.my = psContext->my - psCurr->y();
		sFormContext.xOffset = psContext->xOffset + psCurr->x();
		sFormContext.yOffset = psContext->yOffset + psCurr->y();
		psCurr->processCallbacksRecursive(&sFormContext);
	}
}

/* Process all the widgets on a form.
 * mx and my are the coords of the mouse relative to the form origin.
 */
void WIDGET::runRecursive(W_CONTEXT *psContext)
{
	/* Process the form's widgets */
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		/* Skip any hidden widgets */
		if (!psCurr->visible())
		{
			continue;
		}

		/* Found a sub form, so set up the context */
		W_CONTEXT sFormContext;
		sFormContext.mx = psContext->mx - psCurr->x();
		sFormContext.my = psContext->my - psCurr->y();
		sFormContext.xOffset = psContext->xOffset + psCurr->x();
		sFormContext.yOffset = psContext->yOffset + psCurr->y();

		/* Process it */
		psCurr->runRecursive(&sFormContext);

		/* Run the widget */
		psCurr->run(psContext);
	}
}

void WIDGET::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	W_CONTEXT shiftedContext;
	shiftedContext.mx = psContext->mx - x();
	shiftedContext.my = psContext->my - y();
	shiftedContext.xOffset = psContext->xOffset + x();
	shiftedContext.yOffset = psContext->yOffset + y();

	// Process subwidgets.
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		if (!psCurr->visible() || !psCurr->dim.contains(shiftedContext.mx, shiftedContext.my))
		{
			continue;  // Skip any hidden widgets, or widgets the click missed.
		}

		// Process it (recursively).
		psCurr->processClickRecursive(&shiftedContext, key, wasPressed);
	}

	if (psMouseOverWidget == nullptr)
	{
		psMouseOverWidget = this;  // Mark that the mouse is over a widget (if we haven't already).
	}
	if (screenPointer->lastHighlight != this && psMouseOverWidget == this)
	{
		if (screenPointer->lastHighlight != nullptr)
		{
			screenPointer->lastHighlight->highlightLost();
		}
		screenPointer->lastHighlight = this;  // Mark that the mouse is over a widget (if we haven't already).
		highlight(psContext);
	}

	if (key == WKEY_NONE)
	{
		return;  // Just checking mouse position, not a click.
	}

	if (wasPressed)
	{
		clicked(psContext, key);
	}
	else
	{
		released(psContext, key);
	}
}


/* Execute a set of widgets for one cycle.
 * Returns a list of activated widgets.
 */
WidgetTriggers const &widgRunScreen(W_SCREEN *psScreen)
{
	psScreen->retWidgets.clear();

	/* Initialise the context */
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	psMouseOverWidget = NULL;

	// Note which keys have been pressed
	lastReleasedKey_DEPRECATED = WKEY_NONE;
	if (getWidgetsStatus())
	{
		MousePresses const &clicks = inputGetClicks();
		for (MousePresses::const_iterator c = clicks.begin(); c != clicks.end(); ++c)
		{
			WIDGET_KEY wkey;
			switch (c->key)
			{
				case MOUSE_LMB: wkey = WKEY_PRIMARY; break;
				case MOUSE_RMB: wkey = WKEY_SECONDARY; break;
				default: continue;  // Who cares about other mouse buttons?
			}
			bool pressed;
			switch (c->action)
			{
				case MousePress::Press: pressed = true; break;
				case MousePress::Release: pressed = false; break;
				default: continue;
			}
			sContext.mx = c->pos.x;
			sContext.my = c->pos.y;
			psScreen->psForm->processClickRecursive(&sContext, wkey, pressed);

			lastReleasedKey_DEPRECATED = wkey;
		}
	}

	sContext.mx = mouseX();
	sContext.my = mouseY();
	psScreen->psForm->processClickRecursive(&sContext, WKEY_NONE, true);  // Update highlights and psMouseOverWidget.

	/* Process the screen's widgets */
	psScreen->psForm->runRecursive(&sContext);

	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being run.

	/* Return the ID of a pressed button or finished edit box if any */
	return psScreen->retWidgets;
}

/* Set the id number for widgRunScreen to return */
void W_SCREEN::setReturn(WIDGET *psWidget)
{
	WidgetTrigger trigger;
	trigger.widget = psWidget;
	retWidgets.push_back(trigger);
}

void WIDGET::displayRecursive(int xOffset, int yOffset)
{
	if (debugBoundingBoxesOnly)
	{
		// Display bounding boxes.
		PIELIGHT col;
		col.byte.r = 128 + iSinSR(realTime, 2000, 127); col.byte.g = 128 + iSinSR(realTime + 667, 2000, 127); col.byte.b = 128 + iSinSR(realTime + 1333, 2000, 127); col.byte.a = 128;
		iV_Box(xOffset + x(), yOffset + y(), xOffset + x() + width() - 1, yOffset + y() + height() - 1, col);
	}
	else if (displayFunction)
	{
		displayFunction(this, xOffset, yOffset);
	}
	else
	{
		// Display widget.
		display(xOffset, yOffset);
	}

	if (type == WIDG_FORM && ((W_FORM *)this)->disableChildren)
	{
		return;
	}

	// Update the offset from the current widget's position.
	xOffset += x();
	yOffset += y();

	// If this is a clickable form, the widgets on it have to move when it's down.
	if (type == WIDG_FORM && (((W_FORM *)this)->style & WFORM_NOCLICKMOVE) == 0)
	{
		if ((((W_FORM *)this)->style & WFORM_CLICKABLE) != 0 &&
		    (((W_CLICKFORM *)this)->state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0)
		{
			++xOffset;
			++yOffset;
		}
	}

	// Display the widgets on this widget.
	for (WIDGET::Children::const_iterator i = childWidgets.begin(); i != childWidgets.end(); ++i)
	{
		WIDGET *psCurr = *i;

		// Skip any hidden widgets.
		if (!psCurr->visible())
		{
			continue;
		}

		psCurr->displayRecursive(xOffset, yOffset);
	}
}

/* Display the screen's widgets in their current state
 * (Call after calling widgRunScreen, this allows the input
 *  processing to be seperated from the display of the widgets).
 */
void widgDisplayScreen(W_SCREEN *psScreen)
{
	// To toggle debug bounding boxes: Press: Left Shift   --  --  --------------
	//                                        Left Ctrl  ------------  --  --  ----
	static const int debugSequence[] = {-1, 0, 1, 3, 1, 3, 1, 3, 2, 3, 2, 3, 2, 3, 1, 0, -1};
	static int const *debugLoc = debugSequence;
	static bool debugBoundingBoxes = false;
	int debugCode = keyDown(KEY_LCTRL) + 2*keyDown(KEY_LSHIFT);
	debugLoc = debugLoc[1] == -1? debugSequence : debugLoc[0] == debugCode? debugLoc : debugLoc[1] == debugCode? debugLoc + 1 : debugSequence;
	debugBoundingBoxes = debugBoundingBoxes ^ (debugLoc[1] == -1);

	/* Process any user callback functions */
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	sContext.mx = mouseX();
	sContext.my = mouseY();
	psScreen->psForm->processCallbacksRecursive(&sContext);

	// Display the widgets.
	psScreen->psForm->displayRecursive(0, 0);

	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being displayed.

	/* Display the tool tip if there is one */
	tipDisplay();

	if (debugBoundingBoxes)
	{
		debugBoundingBoxesOnly = true;
		pie_SetRendMode(REND_ALPHA);
		psScreen->psForm->displayRecursive(0, 0);
		debugBoundingBoxesOnly = false;
	}
}

void W_SCREEN::setFocus(WIDGET *widget)
{
	if (psFocus != nullptr)
	{
		psFocus->focusLost();
	}
	psFocus = widget;
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

void setWidgetsStatus(bool var)
{
	bWidgetsActive = var;
}

bool getWidgetsStatus()
{
	return bWidgetsActive;
}
