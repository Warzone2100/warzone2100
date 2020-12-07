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

#include <utility>
#if defined(WZ_CC_MSVC)
#include "widgbase.h"		// this is generated on the pre-build event.
#endif
#include "widgint.h"

#include "form.h"
#include "label.h"
#include "button.h"
#include "editbox.h"
#include "bar.h"
#include "slider.h"
#include "tip.h"

#include <algorithm>

static	bool	bWidgetsActive = true;

/* The widget the mouse is over this update */
static std::shared_ptr<WIDGET> psMouseOverWidget = nullptr;

static WIDGET_AUDIOCALLBACK AudioCallback = nullptr;
static SWORD HilightAudioID = -1;
static SWORD ClickedAudioID = -1;
static SWORD ErrorAudioID = -1;

static WIDGET_KEY lastReleasedKey_DEPRECATED = WKEY_NONE;

static std::vector<std::shared_ptr<WIDGET>> widgetDeletionQueue;

static bool debugBoundingBoxesOnly = false;

struct OverlayScreen
{
	std::shared_ptr<W_SCREEN> psScreen;
	uint16_t zOrder;
};
static std::vector<OverlayScreen> overlays;


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
	tipShutdown();
}

void widgRegisterOverlayScreen(std::shared_ptr<W_SCREEN> const &psScreen, uint16_t zOrder)
{
	OverlayScreen newOverlay {psScreen, zOrder};
	overlays.insert(std::upper_bound(overlays.begin(), overlays.end(), newOverlay, [](const OverlayScreen& a, const OverlayScreen& b){ return a.zOrder > b.zOrder; }), newOverlay);
}

void widgRemoveOverlayScreen(std::shared_ptr<W_SCREEN> const &psScreen)
{
	overlays.erase(
		std::remove_if(
			overlays.begin(), overlays.end(),
			[psScreen](const OverlayScreen& a) { return a.psScreen == psScreen; }
		)
	);
}

bool isMouseOverScreenOverlayChild(int mx, int my)
{
	for (const auto& overlay : overlays)
	{
		auto psRoot = overlay.psScreen->psForm;

		// Hit-test all children of root form
		for (auto const &psCurr: psRoot->children())
		{
			if (!psCurr->visible() || !psCurr->hitTest(mx, my))
			{
				continue;  // Skip any hidden widgets, or widgets the click missed.
			}

			// hit test succeeded for child widget
			return true;
		}
	}

	return false;
}

static void deleteOldWidgets()
{
	while (!widgetDeletionQueue.empty())
	{
		auto guiltyWidget = widgetDeletionQueue.back();
		widgetDeletionQueue.pop_back();  // Do this before deleting widget, in case it calls deleteLater() on other widgets.

		ASSERT_OR_RETURN(, std::find(widgetDeletionQueue.begin(), widgetDeletionQueue.end(), guiltyWidget) == widgetDeletionQueue.end(), "Called deleteLater() twice on the same widget.");

		guiltyWidget->parent()->detach(guiltyWidget);
	}
}

W_INIT::W_INIT()
	: formID(0)
	, majorID(0)
	, id(0)
	, style(0)
	, x(0), y(0)
	, width(0), height(0)
	, pDisplay(nullptr)
	, pCallback(nullptr)
	, pUserData(nullptr)
	, UserData(0)
	, calcLayout(nullptr)
	, onDelete(nullptr)
	, customHitTest(nullptr)
	, initPUserDataFunc(nullptr)
{}

WIDGET::WIDGET(W_INIT const *init, WIDGET_TYPE type)
	: id(init->id)
	, type(type)
	, style(init->style)
	, displayFunction(init->pDisplay)
	, callback(init->pCallback)
	, pUserData(init->pUserData)
	, UserData(init->UserData)
	, calcLayout(init->calcLayout)
	, onDelete(init->onDelete)
	, customHitTest(init->customHitTest)
	, dim(init->x, init->y, init->width, init->height)
	, dirty(true)
{
	/* Initialize and set the pUserData if necessary */
	if (init->initPUserDataFunc != nullptr)
	{
		assert(pUserData == nullptr); // if the initPUserDataFunc is set, pUserData should not be already set
		pUserData = init->initPUserDataFunc();
	}

	// if calclayout is not null, call it
	callCalcLayout();
}

WIDGET::WIDGET(WIDGET_TYPE type)
	: id(0xFFFFEEEEu)
	, type(type)
	, style(0)
	, displayFunction(nullptr)
	, callback(nullptr)
	, pUserData(nullptr)
	, UserData(0)
	, calcLayout(nullptr)
	, onDelete(nullptr)
	, customHitTest(nullptr)
	, dim(0, 0, 1, 1)
	, dirty(true)
{
}

WIDGET::~WIDGET()
{
	if (onDelete != nullptr)
	{
		onDelete(*this);	// Call the onDelete function to handle any extra logic
	}

	setScreenPointer(nullptr);  // Clear any pointers to us directly from screenPointer.
	tipStop(this);  // Stop showing tooltip, if we are.
}

void WIDGET::deleteLater()
{
	widgetDeletionQueue.push_back(shared_from_this());
}

void WIDGET::setGeometry(WzRect const &r)
{
	if (dim == r)
	{
		return;  // Nothing to do.
	}
	dim = r;
	geometryChanged();
	dirty = true;
}

void WIDGET::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// Default implementation of screenSizeDidChange calls its own calcLayout callback function (if present)
	callCalcLayout();

	// Then propagates the event to all children
	for (auto const &child: childWidgets)
	{
		child->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	}
}

void WIDGET::setCalcLayout(const WIDGET_CALCLAYOUT_FUNC& calcLayoutFunc)
{
	calcLayout = calcLayoutFunc;
	callCalcLayout();
}

void WIDGET::callCalcLayout()
{
	if (calcLayout)
	{
		calcLayout(*this, screenWidth, screenHeight, screenWidth, screenHeight);
	}
#ifdef DEBUG
//	// FOR DEBUGGING:
//	// To help track down WIDGETs missing a calc layout function
//	// (because something isn't properly re-adjusting when live window resizing occurs)
//	// uncomment the else branch below.
//	else
//	{
//		debug(LOG_ERROR, "Object is missing calc layout function");
//	}
#endif
}

void WIDGET::setOnDelete(const WIDGET_ONDELETE_FUNC& onDeleteFunc)
{
	onDelete = onDeleteFunc;
}

void WIDGET::setCustomHitTest(const WIDGET_HITTEST_FUNC& newCustomHitTestFunc)
{
	customHitTest = newCustomHitTestFunc;
}

void WIDGET::attach(std::shared_ptr<WIDGET> widget)
{
	ASSERT_OR_RETURN(, widget != nullptr && widget->parentWidget.expired(), "Bad attach.");
	widget->parentWidget = shared_from_this();
	if (auto screen = screenPointer.lock())
	{
		widget->setScreenPointer(screen);
	}
	childWidgets.push_back(widget);
}

void WIDGET::detach(std::shared_ptr<WIDGET> widget)
{
	ASSERT_OR_RETURN(, widget != nullptr && !widget->parentWidget.expired(), "Bad detach.");

	widget->parentWidget.reset();
	widget->setScreenPointer(nullptr);
	childWidgets.erase(std::find(childWidgets.begin(), childWidgets.end(), widget));

	widgetLost(widget);
}

void WIDGET::setScreenPointer(std::shared_ptr<W_SCREEN> const &newScreen)
{
	if (screenPointer.lock() == newScreen)
	{
		return;
	}

	if (psMouseOverWidget.get() == this)
	{
		psMouseOverWidget = nullptr;
	}

	if (auto screen = screenPointer.lock())
	{
		if (screen->psFocus.get() == this)
		{
			screen->psFocus = nullptr;
		}
		if (screen->lastHighlight.get() == this)
		{
			screen->lastHighlight = nullptr;
		}
	}

	screenPointer = newScreen;
	for (auto const &child: childWidgets)
	{
		child->setScreenPointer(newScreen);
	}
}

void WIDGET::widgetLost(std::shared_ptr<WIDGET> const &widget)
{
	if (auto parent = parentWidget.lock())
	{
		parent->widgetLost(widget);  // We don't care about the lost widget, maybe the parent does. (Special code for W_TABFORM.)
	}
}

void W_SCREEN::initialize()
{
	W_FORMINIT sInit;
	sInit.id = 0;
	sInit.style = WFORM_PLAIN | WFORM_INVISIBLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;

	psForm = std::make_shared<W_FORM>(&sInit);
	psForm->screenPointer = shared_from_this();
}

void W_SCREEN::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// resize the top-level form
	psForm->setGeometry(0, 0, screenWidth - 1, screenHeight - 1);

	// inform the top-level form of the event
	psForm->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

/* Check whether an ID has been used on a form */
static bool widgCheckIDForm(std::shared_ptr<WIDGET> const &psForm, UDWORD id)
{
	if (psForm->id == id)
	{
		return true;
	}
	for (auto const &child: psForm->children())
	{
		if (widgCheckIDForm(child, id))
		{
			return true;
		}
	}
	return false;
}

static bool widgAddWidget(std::shared_ptr<W_SCREEN> const &psScreen, W_INIT const *psInit, std::shared_ptr<WIDGET> widget)
{
	ASSERT_OR_RETURN(false, widget != nullptr, "Invalid widget");
	ASSERT_OR_RETURN(false, psScreen != nullptr, "Invalid screen pointer");
	ASSERT_OR_RETURN(false, !widgCheckIDForm(psScreen->psForm, psInit->id), "ID number has already been used (%d)", psInit->id);
	// Find the form to add the widget to.
	WIDGET *psParent = nullptr;
	if (psInit->formID == 0)
	{
		/* Add to the base form */
		psParent = psScreen->psForm.get();
	}
	else
	{
		psParent = widgGetFromID(psScreen, psInit->formID);
	}
	ASSERT_OR_RETURN(false, psParent != nullptr && psParent->type == WIDG_FORM, "Could not find parent form from formID");

	psParent->attach(widget);
	return true;
}

/* Add a form to the widget screen */
std::shared_ptr<W_FORM> widgAddForm(std::shared_ptr<W_SCREEN> const &psScreen, const W_FORMINIT *psInit)
{
	std::shared_ptr<W_FORM> psForm;
	if (psInit->style & WFORM_CLICKABLE)
	{
		psForm = std::make_shared<W_CLICKFORM>(psInit);
	}
	else
	{
		psForm = std::make_shared<W_FORM>(psInit);
	}

	return widgAddWidget(psScreen, psInit, psForm) ? psForm : nullptr;
}

/* Add a label to the widget screen */
W_LABEL *widgAddLabel(std::shared_ptr<W_SCREEN> const &psScreen, const W_LABINIT *psInit)
{
	auto label = std::make_shared<W_LABEL>(psInit);
	return widgAddWidget(psScreen, psInit, label) ? label.get() : nullptr;
}

/* Add a button to the widget screen */
W_BUTTON *widgAddButton(std::shared_ptr<W_SCREEN> const &psScreen, const W_BUTINIT *psInit)
{
	auto button = std::make_shared<W_BUTTON>(psInit);
	return widgAddWidget(psScreen, psInit, button) ? button.get() : nullptr;
}

/* Add an edit box to the widget screen */
W_EDITBOX *widgAddEditBox(std::shared_ptr<W_SCREEN> const &psScreen, const W_EDBINIT *psInit)
{
	auto editBox = std::make_shared<W_EDITBOX>(psInit);
	return widgAddWidget(psScreen, psInit, editBox) ? editBox.get() : nullptr;
}

/* Add a bar graph to the widget screen */
W_BARGRAPH *widgAddBarGraph(std::shared_ptr<W_SCREEN> const &psScreen, const W_BARINIT *psInit)
{
	auto barGraph = std::make_shared<W_BARGRAPH>(psInit);
	return widgAddWidget(psScreen, psInit, barGraph) ? barGraph.get() : nullptr;
}

/* Add a slider to a form */
W_SLIDER *widgAddSlider(std::shared_ptr<W_SCREEN> const &psScreen, const W_SLDINIT *psInit)
{
	auto slider = std::make_shared<W_SLIDER>(psInit);
	return widgAddWidget(psScreen, psInit, slider) ? slider.get() : nullptr;
}

/* Find a widget on a form from its id number */
static std::shared_ptr<WIDGET> widgFormGetFromID(std::shared_ptr<WIDGET> widget, UDWORD id)
{
	if (widget->id == id)
	{
		return widget;
	}

	for (auto &child: widget->children())
	{
		auto w = widgFormGetFromID(child, id);
		if (w != nullptr)
		{
			return w;
		}
	}
	return nullptr;
}

/* Delete a widget from the screen */
void widgDelete(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	if (psScreen->psForm->id == id) {
		psScreen->psForm = nullptr;
		return;
	}

	auto widget = widgFormGetFromID(psScreen->psForm, id);
	if (widget && widget->parent())
	{
		widget->parent()->detach(widget);
	}
}

/* Find a widget in a screen from its ID number */
WIDGET *widgGetFromID(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	return widgFormGetFromID(psScreen->psForm, id).get();
}

void widgHide(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	auto psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget from id.");

	psWidget->hide();
}

void widgReveal(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	auto psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget from id.");

	psWidget->show();
}


/* Get the current position of a widget */
void widgGetPos(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id, SWORD *pX, SWORD *pY)
{
	auto psWidget = widgGetFromID(psScreen, id);
	if (psWidget != nullptr)
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
UDWORD widgGetMouseOver(std::shared_ptr<W_SCREEN> const &psScreen)
{
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if psMouseOverWidget needs to stop being a static and moves into
	   the screen structure */
	(void)psScreen;

	if (psMouseOverWidget == nullptr)
	{
		return 0;
	}

	return psMouseOverWidget->id;
}


/* Return the user data for a widget */
void *widgGetUserData(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	auto psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->pUserData;
	}

	return nullptr;
}


/* Return the user data for a widget */
UDWORD widgGetUserData2(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	auto psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->UserData;
	}

	return 0;
}


/* Set user data for a widget */
void widgSetUserData(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id, void *UserData)
{
	auto psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->pUserData = UserData;
	}
}

/* Set user data for a widget */
void widgSetUserData2(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id, UDWORD UserData)
{
	auto psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->UserData = UserData;
	}
}

void WIDGET::setTip(std::string)
{
	ASSERT(false, "Can't set widget type %u's tip.", type);
}

WIDGET *WIDGET::parent()
{
	if (auto locked = parentWidget.lock())
	{
		return locked.get();
	}

	return nullptr;
}

/* Set tip string for a widget */
void widgSetTip(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id, std::string pTip)
{
	auto psWidget = widgGetFromID(psScreen, id);

	if (!psWidget)
	{
		return;
	}

	psWidget->setTip(std::move(pTip));
}

/* Return which key was used to press the last returned widget */
UDWORD widgGetButtonKey_DEPRECATED(std::shared_ptr<W_SCREEN> const &psScreen)
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
UDWORD widgGetButtonState(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	/* Get the button */
	auto psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(0, psWidget != nullptr, "Couldn't find widget by ID %u", id);

	return psWidget->getState();
}

void WIDGET::setFlash(bool)
{
	ASSERT(false, "Can't set widget type %u's flash.", type);
}

void widgSetButtonFlash(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	/* Get the button */
	auto psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget by ID %u", id);

	psWidget->setFlash(true);
}

void widgClearButtonFlash(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	/* Get the button */
	auto psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget by ID %u", id);

	psWidget->setFlash(false);
}


void WIDGET::setState(unsigned)
{
	ASSERT(false, "Can't set widget type %u's state.", type);
}

/* Set a button or clickable form's state */
void widgSetButtonState(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id, UDWORD state)
{
	/* Get the button */
	auto psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget by ID %u", id);

	psWidget->setState(state);
}

WzString WIDGET::getString() const
{
	ASSERT(false, "Can't get widget type %u's string.", type);
	return WzString();
}

/* Return a pointer to a buffer containing the current string of a widget.
 * NOTE: The string must be copied out of the buffer
 */
const char *widgGetString(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	auto const psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN("", psWidget != nullptr, "Couldn't find widget by ID %u", id);

	static WzString ret;  // Must be static so it isn't immediately freed when this function returns.
	ret = psWidget->getString();
	return ret.toUtf8().c_str();
}

const WzString& widgGetWzString(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id)
{
	static WzString emptyString = WzString();
	auto const psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(emptyString, psWidget != nullptr, "Couldn't find widget by ID %u", id);

	static WzString ret;  // Must be static so it isn't immediately freed when this function returns.
	ret = psWidget->getString();
	return ret;
}

void WIDGET::setString(WzString)
{
	ASSERT(false, "Can't set widget type %u's string.", type);
}

/* Set the text in a widget */
void widgSetString(std::shared_ptr<W_SCREEN> const &psScreen, UDWORD id, const char *pText)
{
	/* Get the widget */
	auto psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget by ID %u", id);

	if (psWidget->type == WIDG_EDITBOX && psScreen->psFocus.get() == psWidget)
	{
		psScreen->resetFocus();
	}

	psWidget->setString(WzString::fromUtf8(pText));
}

void WIDGET::processCallbacksRecursive(W_CONTEXT *psContext)
{
	/* Go through all the widgets on the form */
	for (auto child: childWidgets)
	{
		/* Call the callback */
		if (child->callback)
		{
			child->callback(*child, psContext);
		}

		/* and then recurse */
		W_CONTEXT sFormContext;
		sFormContext.mx = psContext->mx - child->x();
		sFormContext.my = psContext->my - child->y();
		sFormContext.xOffset = psContext->xOffset + child->x();
		sFormContext.yOffset = psContext->yOffset + child->y();
		child->processCallbacksRecursive(&sFormContext);
	}
}

/* Process all the widgets on a form.
 * mx and my are the coords of the mouse relative to the form origin.
 */
void WIDGET::runRecursive(W_CONTEXT *psContext)
{
	/* Process the form's widgets */
	for (auto const &child: childWidgets)
	{
		/* Skip any hidden widgets */
		if (!child->visible())
		{
			continue;
		}

		/* Found a sub form, so set up the context */
		W_CONTEXT sFormContext;
		sFormContext.mx = psContext->mx - child->x();
		sFormContext.my = psContext->my - child->y();
		sFormContext.xOffset = psContext->xOffset + child->x();
		sFormContext.yOffset = psContext->yOffset + child->y();

		/* Process it */
		child->runRecursive(&sFormContext);

		/* Run the widget */
		child->run(psContext);
	}
}

bool WIDGET::hitTest(int x, int y)
{
	// default hit-testing bounding rect (based on the widget's x, y, width, height)
	bool hitTestResult = dim.contains(x, y);

	if(customHitTest)
	{
		// if the default bounding-rect hit-test succeeded, use the custom hit-testing func
		hitTestResult = hitTestResult && customHitTest(*this, x, y);
	}

	return hitTestResult;
}

bool WIDGET::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	bool didProcessClick = false;

	W_CONTEXT shiftedContext;
	shiftedContext.mx = psContext->mx - x();
	shiftedContext.my = psContext->my - y();
	shiftedContext.xOffset = psContext->xOffset + x();
	shiftedContext.yOffset = psContext->yOffset + y();

	// Process subwidgets.
	for (auto const &child: childWidgets)
	{
		if (!child->visible() || !child->hitTest(shiftedContext.mx, shiftedContext.my))
		{
			continue;  // Skip any hidden widgets, or widgets the click missed.
		}

		// Process it (recursively).
		didProcessClick = child->processClickRecursive(&shiftedContext, key, wasPressed) || didProcessClick;
	}

	if (!visible())
	{
		// special case for root form
		// since the processClickRecursive of children is only called if they are visible
		// this should only trigger for the root form of a screen that's been set to invisible
		return false;
	}

	if (psMouseOverWidget == nullptr)
	{
		psMouseOverWidget = shared_from_this();  // Mark that the mouse is over a widget (if we haven't already).
	}

	if (auto screen = screenPointer.lock())
	{
		if (screen->lastHighlight.get() != this && psMouseOverWidget.get() == this)
		{
			if (screen->lastHighlight != nullptr)
			{
				screen->lastHighlight->highlightLost();
			}
			screen->lastHighlight = shared_from_this();  // Mark that the mouse is over a widget (if we haven't already).
			highlight(psContext);
		}
	}

	if (key == WKEY_NONE)
	{
		return false;  // Just checking mouse position, not a click.
	}

	if (psMouseOverWidget.get() == this)
	{
		if (wasPressed)
		{
			clicked(psContext, key);
		}
		else
		{
			released(psContext, key);
		}
		didProcessClick = true;
	}

	return didProcessClick;
}


/* Execute a set of widgets for one cycle.
 * Returns a list of activated widgets.
 */
WidgetTriggers const &widgRunScreen(std::shared_ptr<W_SCREEN> const &psScreen)
{
	psScreen->retWidgets.clear();

	/* Initialise the context */
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	psMouseOverWidget = nullptr;

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
			for (const auto& overlay : overlays)
			{
				overlay.psScreen->psForm->processClickRecursive(&sContext, wkey, pressed);
			}
			psScreen->psForm->processClickRecursive(&sContext, wkey, pressed);

			lastReleasedKey_DEPRECATED = wkey;
		}
	}

	sContext.mx = mouseX();
	sContext.my = mouseY();
	for (const auto& overlay : overlays)
	{
		overlay.psScreen->psForm->processClickRecursive(&sContext, WKEY_NONE, true);  // Update highlights and psMouseOverWidget.
	}
	psScreen->psForm->processClickRecursive(&sContext, WKEY_NONE, true);  // Update highlights and psMouseOverWidget.

	/* Process the screen's widgets */
	for (const auto& overlay : overlays)
	{
		overlay.psScreen->psForm->runRecursive(&sContext);
	}
	psScreen->psForm->runRecursive(&sContext);

	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being run.

	/* Return the ID of a pressed button or finished edit box if any */
	return psScreen->retWidgets;
}

/* Set the id number for widgRunScreen to return */
void W_SCREEN::setReturn(std::shared_ptr<WIDGET> psWidget)
{
	WidgetTrigger trigger;
	trigger.widget = psWidget;
	retWidgets.push_back(trigger);
}

void WIDGET::displayRecursive(WidgetGraphicsContext const &context)
{
	if (context.clipContains(geometry())) {
		if (debugBoundingBoxesOnly)
		{
			// Display bounding boxes.
			PIELIGHT col;
			col.byte.r = 128 + iSinSR(realTime, 2000, 127); col.byte.g = 128 + iSinSR(realTime + 667, 2000, 127); col.byte.b = 128 + iSinSR(realTime + 1333, 2000, 127); col.byte.a = 128;
			iV_Box(context.getXOffset() + x(), context.getYOffset() + y(), context.getXOffset() + x() + width() - 1, context.getYOffset() + y() + height() - 1, col);
		}
		else if (displayFunction)
		{
			displayFunction(*this, context.getXOffset(), context.getYOffset());
		}
		else
		{
			// Display widget.
			display(context.getXOffset(), context.getYOffset());
		}
	}

	if (type == WIDG_FORM && ((W_FORM *)this)->disableChildren)
	{
		return;
	}

	auto childrenContext = context.translatedBy(x(), y());

	// If this is a clickable form, the widgets on it have to move when it's down.
	if (type == WIDG_FORM && (((W_FORM *)this)->style & WFORM_NOCLICKMOVE) == 0)
	{
		if ((((W_FORM *)this)->style & WFORM_CLICKABLE) != 0 &&
		    (((W_CLICKFORM *)this)->state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0)
		{
			childrenContext = childrenContext.translatedBy(1, 1);
		}
	}

	// Display the widgets on this widget.
	for (auto const &child: childWidgets)
	{
		if (child->visible())
		{
			child->displayRecursive(childrenContext);
		}
	}
}

/* Display the screen's widgets in their current state
 * (Call after calling widgRunScreen, this allows the input
 *  processing to be separated from the display of the widgets).
 */
void widgDisplayScreen(std::shared_ptr<W_SCREEN> const &psScreen)
{
	// To toggle debug bounding boxes: Press: Left Shift   --  --  --------------
	//                                        Left Ctrl  ------------  --  --  ----
	static const int debugSequence[] = { -1, 0, 1, 3, 1, 3, 1, 3, 2, 3, 2, 3, 2, 3, 1, 0, -1};
	static int const *debugLoc = debugSequence;
	static bool debugBoundingBoxes = false;
	int debugCode = keyDown(KEY_LCTRL) + 2 * keyDown(KEY_LSHIFT);
	debugLoc = debugLoc[1] == -1 ? debugSequence : debugLoc[0] == debugCode ? debugLoc : debugLoc[1] == debugCode ? debugLoc + 1 : debugSequence;
	debugBoundingBoxes = debugBoundingBoxes ^ (debugLoc[1] == -1);

	/* Process any user callback functions */
	W_CONTEXT sContext;
	sContext.xOffset = 0;
	sContext.yOffset = 0;
	sContext.mx = mouseX();
	sContext.my = mouseY();
	psScreen->psForm->processCallbacksRecursive(&sContext);

	// Display the widgets.
	psScreen->psForm->displayRecursive();

	// Always overlays on-top (i.e. draw them last)
	for (const auto& overlay : overlays)
	{
		overlay.psScreen->psForm->processCallbacksRecursive(&sContext);
		overlay.psScreen->psForm->displayRecursive();
	}

	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being displayed.

	/* Display the tool tip if there is one */
	tipDisplay();

	if (debugBoundingBoxes)
	{
		debugBoundingBoxesOnly = true;
		psScreen->psForm->displayRecursive();
		debugBoundingBoxesOnly = false;
	}
}

void W_SCREEN::setFocus(std::shared_ptr<WIDGET> widget)
{
	resetFocus();
	psFocus = widget;
}

void W_SCREEN::resetFocus()
{
	if (psFocus != nullptr)
	{
		psFocus->focusLost();
	}
	psFocus = nullptr;
}

void WidgSetAudio(WIDGET_AUDIOCALLBACK Callback, SWORD HilightID, SWORD ClickedID, SWORD ErrorID)
{
	AudioCallback = Callback;
	HilightAudioID = HilightID;
	ClickedAudioID = ClickedID;
	ErrorAudioID = ErrorID;
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

SWORD WidgGetErrorAudioID(void)
{
	return ErrorAudioID;
}

void setWidgetsStatus(bool var)
{
	bWidgetsActive = var;
}

bool getWidgetsStatus()
{
	return bWidgetsActive;
}

bool WidgetGraphicsContext::clipContains(WzRect const& rect) const
{
	return !clipped || clipRect.contains({offset.x + rect.x(), offset.y + rect.y(), rect.width(), rect.height()});
}

WidgetGraphicsContext WidgetGraphicsContext::translatedBy(int32_t x, int32_t y) const
{
	WidgetGraphicsContext newContext(*this);
	newContext.offset.x += x;
	newContext.offset.y += y;
	return newContext;
}

WidgetGraphicsContext WidgetGraphicsContext::clippedBy(WzRect const &newRect) const
{
	WidgetGraphicsContext newContext(*this);
	newContext.clipRect = {
		offset.x + newRect.x(),
		offset.y + newRect.y(),
		newRect.width(),
		newRect.height()
	};

	if (clipped) {
		newContext.clipRect = newContext.clipRect.intersectionWith(clipRect);
	}

	newContext.clipped = true;

	return newContext;
}
