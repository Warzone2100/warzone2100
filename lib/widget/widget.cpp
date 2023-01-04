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
#include "lib/netplay/netplay.h"
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
#include <unordered_set>
#include <deque>

static	bool	bWidgetsInitialized = false;
static	bool	bWidgetsActive = true;

/* The widget the mouse is over this update */
static auto psMouseOverWidget = std::weak_ptr<WIDGET>();
static auto psClickDownWidgetScreen = std::shared_ptr<W_SCREEN>();
static auto psMouseOverWidgetScreen = std::shared_ptr<W_SCREEN>();

static WIDGET_AUDIOCALLBACK AudioCallback = nullptr;
static SWORD HilightAudioID = -1;
static SWORD ClickedAudioID = -1;
static SWORD ErrorAudioID = -1;

static WIDGET_KEY lastReleasedKey_DEPRECATED = WKEY_NONE;

static std::deque<std::shared_ptr<WIDGET>> widgetDeletionQueue;
static std::vector<std::function<void()>> widgetScheduledTasks;

static bool debugBoundingBoxesOnly = false;

#ifdef DEBUG
#include "lib/framework/demangle.hpp"
static std::unordered_set<const WIDGET*> debugLiveWidgets;
static std::shared_ptr<W_SCREEN> psCurrentlyRunningScreen;
#endif

// Verify overlay screen reserved bits
#define Bits(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) \
if (!m##d##a##l##j##y.n##e##s[h##ec##b##a##e].g##S##n##o##f##or) { return; }
#define OverlayScreenAssert(s, z) \
if (s == nullptr) { return; } \
if ((z & 0xFFF0) == 0xFFF0 && (z & 0xC) == 0xC && !(z & 0x3)) \
{ \
	Bits(P,ted,f,et,layer,ctat,is,sel,i,a,k,l,N,p,e); \
}

struct OverlayScreen
{
	std::shared_ptr<W_SCREEN> psScreen;
	uint16_t zOrder;
};
static std::vector<OverlayScreen> overlays;
static std::unordered_set<std::shared_ptr<W_SCREEN>> overlaySet; // for quick lookup
static std::unordered_set<std::shared_ptr<W_SCREEN>> overlaysToDelete;

static void runScheduledTasks()
{
	auto tasksCopy = std::move(widgetScheduledTasks);
	for (auto task: tasksCopy)
	{
		task();
	}
}

static void deleteOldWidgets()
{
	while (!widgetDeletionQueue.empty())
	{
		std::shared_ptr<WIDGET> guiltyWidget = widgetDeletionQueue.front();
		widgDelete(guiltyWidget.get());
		widgetDeletionQueue.pop_front();
	}
}

static inline void _widgDebugAssertIfRunningScreen(const char *function)
{
#ifdef DEBUG
	if (psCurrentlyRunningScreen != nullptr)
	{
		debug(LOG_INFO, "%s is being called from within a screen's click / run handlers", function);
		if (assertEnabled)
		{
			(void)wz_assert(psCurrentlyRunningScreen == nullptr);
		}
	}
#else
	// do nothing
#endif
}
#define widgDebugAssertIfRunningScreen() _widgDebugAssertIfRunningScreen(__FUNCTION__)

/* Initialise the widget module */
bool widgInitialise()
{
	bWidgetsInitialized = true;
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
	psClickDownWidgetScreen = nullptr;
	psMouseOverWidgetScreen = nullptr;
	tipShutdown();
	overlays.clear();
	overlaySet.clear();
	overlaysToDelete.clear();
	deleteOldWidgets();
	widgetScheduledTasks.clear();
#ifdef DEBUG
	if (!debugLiveWidgets.empty())
	{
		// Some widgets still exist - there may be reference cycles, non-cleared references, or other bugs
		for (auto widget : debugLiveWidgets)
		{
			debug(LOG_ERROR, "Widget not cleaned up: %p (type: %s)", (const void*)widget, cxxDemangle(typeid(*widget).name()).c_str());
		}
		ASSERT(debugLiveWidgets.empty(), "There are %zu widgets that were not cleaned up.", debugLiveWidgets.size());
	}
#endif
	bWidgetsInitialized = false;
}

void widgScheduleTask(std::function<void ()> task)
{
	widgetScheduledTasks.push_back(task);
}

void widgRegisterOverlayScreen(const std::shared_ptr<W_SCREEN> &psScreen, uint16_t zOrder)
{
	OverlayScreenAssert(psScreen, zOrder);
	OverlayScreen newOverlay {psScreen, zOrder};
	auto it = std::find_if(overlays.begin(), overlays.end(), [psScreen](const OverlayScreen& overlay) -> bool {
		return overlay.psScreen == psScreen;
	});
	if (it != overlays.end())
	{
		// screen already exists in overlay stack

		// check if it's queued for removal, and if so clear that status
		if (overlaysToDelete.count(psScreen))
		{
			debug(LOG_WZ, "Overlay was queued for deletion, but is being re-added - clear the queued deletion");
			overlaysToDelete.erase(psScreen);
		}

		if (zOrder == it->zOrder)
		{
			// no need to update - duplicate call (same zOrder)
			return;
		}
		else
		{
			// remove the existing entry
			overlays.erase(it);
			it = overlays.end();
			// fall-through to inserting it again in the new zOrder
		}
	}
	// the screens are stored in decreasing z-order
	overlays.insert(std::upper_bound(overlays.begin(), overlays.end(), newOverlay, [](const OverlayScreen& a, const OverlayScreen& b){ return a.zOrder > b.zOrder; }), newOverlay);
	overlaySet.insert(psScreen);
}

// Note: If priorScreen is not the "regular" screen (i.e. if priorScreen is an overlay screen) it should already be registered
void widgRegisterOverlayScreenOnTopOfScreen(const std::shared_ptr<W_SCREEN> &psScreen, const std::shared_ptr<W_SCREEN> &priorScreen)
{
	auto it = std::find_if(overlays.begin(), overlays.end(), [priorScreen](const OverlayScreen& overlay) -> bool {
		return overlay.psScreen == priorScreen;
	});
	if (it != overlays.end())
	{
		OverlayScreen newOverlay {psScreen, it->zOrder}; // use the same z-order as priorScreen, but insert *before* it in the list (i.e. "above" it, since overlays are stored in decreasing z-order)
		overlays.insert(it, newOverlay);
		overlaySet.insert(psScreen);
	}
	else
	{
		// priorScreen does not exist in the overlays list, so it is probably the "regular" screen
		// just insert this overlay at the bottom of the overlay list
		OverlayScreen newOverlay {psScreen, 0};
		overlays.insert(overlays.end(), newOverlay);
		overlaySet.insert(psScreen);
	}
}

void widgRemoveOverlayScreen(const std::shared_ptr<W_SCREEN> &psScreen)
{
	auto it = std::find_if(overlays.begin(), overlays.end(), [psScreen](const OverlayScreen& overlay) -> bool {
		return overlay.psScreen == psScreen;
	});
	if (it != overlays.end())
	{
		overlaysToDelete.insert(psScreen);
	}
}

static void cleanupDeletedOverlays()
{
	if (overlaysToDelete.empty()) { return; }
	overlays.erase(
		std::remove_if(
			overlays.begin(), overlays.end(),
			[](const OverlayScreen& a) { return overlaysToDelete.count(a.psScreen) > 0; }
		),
		overlays.end()
	);
	for (const auto& overlayToDelete : overlaysToDelete)
	{
		overlaySet.erase(overlayToDelete);
	}
	overlaysToDelete.clear();
}

bool WIDGET::isMouseOverWidget() const
{
	return psMouseOverWidget.lock().get() == this;
}

void WIDGET::setTransparentToClicks(bool hasClickTransparency)
{
	isTransparentToClicks = hasClickTransparency;
}

void WIDGET::setTransparentToMouse(bool hasMouseTransparency)
{
	isTransparentToMouse = hasMouseTransparency;
}

bool WIDGET::transparentToClicks() const
{
	return isTransparentToClicks;
}

int32_t WIDGET::idealWidth()
{
	if (!defaultIdealWidth.has_value())
	{
		defaultIdealWidth = width();
	}

	return defaultIdealWidth.value();
}

int32_t WIDGET::idealHeight()
{
	if (!defaultIdealHeight.has_value())
	{
		defaultIdealHeight = height();
	}

	return defaultIdealHeight.value();
}

template<typename Iterator>
static inline void iterateOverlayScreens(Iterator iter, Iterator end, const std::function<bool (const OverlayScreen& overlay)>& func)
{
	ASSERT_OR_RETURN(, func.operator bool(), "Requires a valid func");
	for ( ; iter != end; ++iter )
	{
		if (!func(*iter))
		{
			break; // stop enumerating
		}
	}
	// now that we aren't in the middle of enumerating overlays, handling removing any that were queued for deletion
	cleanupDeletedOverlays();
}

// enumerate the overlay screens in decreasing z-order (i.e. "top-down")
static inline void forEachOverlayScreen(const std::function<bool (const OverlayScreen& overlay)>& func)
{
	// the screens are stored in decreasing z-order
	iterateOverlayScreens(overlays.cbegin(), overlays.cend(), func);
}

void widgForEachOverlayScreen(const std::function<bool (const std::shared_ptr<W_SCREEN>& psScreen, uint16_t zOrder)>& func)
{
	if (!func) { return; }
	forEachOverlayScreen([func](const OverlayScreen& overlay) -> bool {
		return func(overlay.psScreen, overlay.zOrder);
	});
}

// enumerate the overlay screens in increasing z-order (i.e. "bottom-up")
static inline void forEachOverlayScreenBottomUp(const std::function<bool (const OverlayScreen& overlay)>& func)
{
	iterateOverlayScreens(overlays.crbegin(), overlays.crend(), func);
}

void widgOverlaysScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	forEachOverlayScreen([oldWidth, oldHeight, newWidth, newHeight](const OverlayScreen& overlay) -> bool
	{
		overlay.psScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
		return true; // keep enumerating
	});
}

static bool isScreenARegisteredOverlay(const std::shared_ptr<W_SCREEN> &psScreen)
{
	if (!psScreen) { return false; }
	return overlaySet.count(psScreen) > 0;
}

bool isMouseOverScreenOverlayChild(int mx, int my)
{
	if (psMouseOverWidgetScreen != nullptr)
	{
		return isScreenARegisteredOverlay(psMouseOverWidgetScreen);
	}
	bool bMouseIsOverOverlayChild = false;
	if (auto mouseOverWidget = psMouseOverWidget.lock())
	{
		// Fallback - the mouse is over a widget, but the widget does not have a screen pointer
		// Hit-test each overlay screen to identify if the mouse is over one
		// (Note: This can currently occur with DropdownWidget members.)
		forEachOverlayScreen([mx, my, &bMouseIsOverOverlayChild](const OverlayScreen& overlay)
		{
			auto psRoot = overlay.psScreen->psForm;

			// Hit-test all children of root form
			for (const auto &psCurr: psRoot->children())
			{
				if (!psCurr->visible() || !psCurr->hitTest(mx, my))
				{
					continue;  // Skip any hidden widgets, or widgets the click missed.
				}

				// hit test succeeded for child widget
				bMouseIsOverOverlayChild = true;
				return false; // stop enumerating
			}
			return true; // keep enumerating
		});
	}
	return bMouseIsOverOverlayChild;
}

bool isMouseClickDownOnScreenOverlayChild()
{
	if (!psClickDownWidgetScreen) { return false; }
	return isScreenARegisteredOverlay(psClickDownWidgetScreen);
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

#ifdef DEBUG
	debugLiveWidgets.insert(this);
#endif
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
#ifdef DEBUG
	debugLiveWidgets.insert(this);
#endif
}

WIDGET::~WIDGET()
{
	if (onDelete != nullptr)
	{
		onDelete(this);	// Call the onDelete function to handle any extra logic
	}

#ifdef DEBUG
	debugLiveWidgets.erase(this);
#endif
}

void WIDGET::deleteLater()
{
	auto shared_widget_ptr = shared_from_this();
	ASSERT_OR_RETURN(, std::find(widgetDeletionQueue.begin(), widgetDeletionQueue.end(), shared_widget_ptr) == widgetDeletionQueue.end(), "Called deleteLater() twice on the same widget.");
	widgetDeletionQueue.push_back(shared_widget_ptr);
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
	for (auto const &psCurr: childWidgets)
	{
		psCurr->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
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
		calcLayout(this);
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

void WIDGET::attach(const std::shared_ptr<WIDGET> &widget)
{
	ASSERT_OR_RETURN(, widget != nullptr && widget->parentWidget.expired(), "Bad attach.");
	widget->parentWidget = shared_from_this();
	widget->setScreenPointer(screenPointer.lock());
	childWidgets.push_back(widget);
}

void WIDGET::detach(const std::shared_ptr<WIDGET> &widget)
{
	ASSERT_OR_RETURN(, widget != nullptr && !widget->parentWidget.expired(), "Bad detach.");

	widget->parentWidget.reset();
	widget->setScreenPointer(nullptr);

	auto it = std::find(childWidgets.begin(), childWidgets.end(), widget);
	if (it != childWidgets.end())
	{
		childWidgets.erase(it);
	}

	widgetLost(widget.get());
}

void WIDGET::setScreenPointer(const std::shared_ptr<W_SCREEN> &screen)
{
	if (screenPointer.lock() == screen)
	{
		return;
	}

	if (isMouseOverWidget())
	{
		psMouseOverWidget.reset();
	}

	if (auto lockedScreen = screenPointer.lock())
	{
		if (lockedScreen->hasFocus(*this))
		{
			lockedScreen->psFocus.reset();
		}
		if (lockedScreen->isLastHighlight(*this))
		{
			lockedScreen->lastHighlight.reset();
		}
	}

	screenPointer = screen;
	for (auto const &child: childWidgets)
	{
		child->setScreenPointer(screen);
	}
}

void WIDGET::widgetLost(WIDGET *widget)
{
	if (auto lockedParent = parentWidget.lock())
	{
		lockedParent->widgetLost(widget);  // We don't care about the lost widget, maybe the parent does. (Special code for W_TABFORM.)
	}
}

W_SCREEN::~W_SCREEN()
{
	if (auto focusedWidget = psFocus.lock())
	{
		// must trigger a resignation of focus
		if (bWidgetsInitialized) // do not call focusLost if widgets have already shutdown / are not initialized
		{
			focusedWidget->focusLost();
		}
	}
	psFocus.reset();
}

void W_SCREEN::initialize(const std::shared_ptr<W_FORM>& customRootForm)
{
	if (customRootForm)
	{
		auto customRootFormParent = customRootForm->parent();
		if (customRootFormParent)
		{
			customRootFormParent->detach(customRootForm);
		}
		psForm = customRootForm;
		psForm->screenPointer = shared_from_this();
		return;
	}

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
	psForm->setGeometry(0, 0, screenWidth, screenHeight);

	// inform the top-level form of the event
	psForm->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

/* Check whether an ID has been used on a form */
static bool widgCheckIDForm(WIDGET *psForm, UDWORD id)
{
	if (psForm->id == id)
	{
		return true;
	}
	for (auto const &child: psForm->children())
	{
		if (widgCheckIDForm(child.get(), id))
		{
			return true;
		}
	}
	return false;
}

static bool widgAddWidget(const std::shared_ptr<W_SCREEN> &psScreen, W_INIT const *psInit, const std::shared_ptr<WIDGET> &widget)
{
	ASSERT_OR_RETURN(false, widget != nullptr, "Invalid widget");
	ASSERT_OR_RETURN(false, psScreen != nullptr, "Invalid screen pointer");
	ASSERT_OR_RETURN(false, !widgCheckIDForm(psScreen->psForm.get(), psInit->id), "ID number has already been used (%d)", psInit->id);
	// Find the form to add the widget to.
	W_FORM *psParent;
	if (psInit->formID == 0)
	{
		/* Add to the base form */
		psParent = psScreen->psForm.get();
	}
	else
	{
		psParent = (W_FORM *)widgGetFromID(psScreen, psInit->formID);
	}
	ASSERT_OR_RETURN(false, psParent != nullptr && psParent->type == WIDG_FORM, "Could not find parent form from formID");

	psParent->attach(widget);
	return true;
}

/* Add a form to the widget screen */
W_FORM *widgAddForm(const std::shared_ptr<W_SCREEN> &psScreen, const W_FORMINIT *psInit)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	std::shared_ptr<W_FORM> psForm = nullptr;
	if (psInit->style & WFORM_CLICKABLE)
	{
		psForm = std::make_shared<W_CLICKFORM>(psInit);
	}
	else
	{
		psForm = std::make_shared<W_FORM>(psInit);
	}

	return widgAddWidget(psScreen, psInit, psForm) ? psForm.get() : nullptr;
}

/* Add a label to the widget screen */
W_LABEL *widgAddLabel(const std::shared_ptr<W_SCREEN> &psScreen, const W_LABINIT *psInit)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	auto psLabel = std::make_shared<W_LABEL>(psInit);
	return widgAddWidget(psScreen, psInit, psLabel) ? psLabel.get() : nullptr;
}

/* Add a button to the widget screen */
W_BUTTON *widgAddButton(const std::shared_ptr<W_SCREEN> &psScreen, const W_BUTINIT *psInit)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	auto psButton = std::make_shared<W_BUTTON>(psInit);
	return widgAddWidget(psScreen, psInit, psButton) ? psButton.get() : nullptr;
}

/* Add an edit box to the widget screen */
W_EDITBOX *widgAddEditBox(const std::shared_ptr<W_SCREEN> &psScreen, const W_EDBINIT *psInit)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	auto psEdBox = std::make_shared<W_EDITBOX>(psInit);
	return widgAddWidget(psScreen, psInit, psEdBox) ? psEdBox.get() : nullptr;
}

/* Add a bar graph to the widget screen */
W_BARGRAPH *widgAddBarGraph(const std::shared_ptr<W_SCREEN> &psScreen, const W_BARINIT *psInit)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	auto psBarGraph = std::make_shared<W_BARGRAPH>(psInit);
	return widgAddWidget(psScreen, psInit, psBarGraph) ? psBarGraph.get() : nullptr;
}

/* Add a slider to a form */
W_SLIDER *widgAddSlider(const std::shared_ptr<W_SCREEN> &psScreen, const W_SLDINIT *psInit)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	auto psSlider = std::make_shared<W_SLIDER>(psInit);
	return widgAddWidget(psScreen, psInit, psSlider) ? psSlider.get() : nullptr;
}

/* Delete a widget from the screen */
void widgDelete(WIDGET *widget)
{
	if (!widget)
	{
		return;
	}
	widgDebugAssertIfRunningScreen();

	if (auto lockedScreen = widget->screenPointer.lock())
	{
		if (auto widgetWithFocus = lockedScreen->getWidgetWithFocus())
		{
			if (widgetWithFocus->hasAncestor(widget))
			{
				// must trigger a resignation of focus
				lockedScreen->setFocus(nullptr);
			}
		}
		if (lockedScreen->psForm.get() == widget)
		{
			lockedScreen->psForm = nullptr;
		}
	}

	if (auto parent = widget->parent())
	{
		parent->detach(widget);
	}
}

/* Delete a widget from the screen */
void widgDelete(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	widgDelete(widgGetFromID(psScreen, id));
}
void widgDeleteLater(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	auto psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->deleteLater();
	}
}

/* Find a widget on a form from its id number */
std::shared_ptr<WIDGET> widgFormGetFromID(const std::shared_ptr<WIDGET>& widget, UDWORD id)
{
	ASSERT_OR_RETURN(nullptr, widget != nullptr, "Invalid widget pointer");
	if (widget->id == id)
	{
		return widget->shared_from_this();
	}
	for (auto const &child: widget->children())
	{
		std::shared_ptr<WIDGET> w = widgFormGetFromID(child, id);
		if (w != nullptr)
		{
			return w;
		}
	}
	return nullptr;
}

/* Find a widget in a screen from its ID number */
WIDGET *widgGetFromID(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	return widgFormGetFromID(psScreen->psForm, id).get();
}

void widgHide(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget from id.");

	psWidget->hide();
}

void widgReveal(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget != nullptr, "Couldn't find widget from id.");

	psWidget->show();
}

/* Return the ID of the widget the mouse was over this frame */
UDWORD widgGetMouseOver(const std::shared_ptr<W_SCREEN> &psScreen)
{
	ASSERT_OR_RETURN(0, psScreen != nullptr, "Invalid screen pointer");
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if psMouseOverWidget needs to stop being a static and moves into
	   the screen structure */

	if (auto lockedMouserOverWidget = psMouseOverWidget.lock())
	{
		return lockedMouserOverWidget->id;
	}

	return 0;
}

bool isMouseOverSomeWidget(const std::shared_ptr<W_SCREEN> &psScreen)
{
	if (auto lockedMouserOverWidget = psMouseOverWidget.lock())
	{
		return lockedMouserOverWidget != psScreen->psForm;
	}

	return false;
}

/* Return the user data for a widget */
void *widgGetUserData(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->pUserData;
	}

	return nullptr;
}


/* Return the user data for a widget */
UDWORD widgGetUserData2(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(0, psScreen != nullptr, "Invalid screen pointer");
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		return psWidget->UserData;
	}

	return 0;
}


/* Set user data for a widget */
void widgSetUserData(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, void *UserData)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->pUserData = UserData;
	}
}

/* Set user data for a widget */
void widgSetUserData2(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, UDWORD UserData)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	if (psWidget)
	{
		psWidget->UserData = UserData;
	}
}

void WIDGET::setTip(std::string)
{
	ASSERT(false, "Can't set widget type %u's tip.", type);
}

/* Set tip string for a widget */
void widgSetTip(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, std::string pTip)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	WIDGET *psWidget = widgGetFromID(psScreen, id);

	if (!psWidget)
	{
		return;
	}

	psWidget->setTip(std::move(pTip));
}

/* Return which key was used to press the last returned widget */
UDWORD widgGetButtonKey_DEPRECATED(const std::shared_ptr<W_SCREEN> &psScreen)
{
	ASSERT_OR_RETURN(lastReleasedKey_DEPRECATED, psScreen != nullptr, "Invalid screen pointer");
	/* Don't actually need the screen parameter at the moment - but it might be
	   handy if released needs to stop being a static and moves into
	   the screen structure */

	return lastReleasedKey_DEPRECATED;
}

unsigned WIDGET::getState()
{
	ASSERT(false, "Can't get widget type %u's state.", type);
	return 0;
}

/* Get a button or clickable form's state */
UDWORD widgGetButtonState(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(0, psScreen != nullptr, "Invalid screen pointer");
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(0, psWidget, "Couldn't find widget by ID %u", id);

	return psWidget->getState();
}

void WIDGET::setFlash(bool)
{
	ASSERT(false, "Can't set widget type %u's flash.", type);
}

void widgSetButtonFlash(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	psWidget->setFlash(true);
}

void widgClearButtonFlash(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
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
void widgSetButtonState(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, UDWORD state)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	/* Get the button */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

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
const char *widgGetString(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN("", psScreen != nullptr, "Invalid screen pointer");
	const WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN("", psWidget, "Couldn't find widget by ID %u", id);

	static WzString ret;  // Must be static so it isn't immediately freed when this function returns.
	ret = psWidget->getString();
	return ret.toUtf8().c_str();
}

const WzString& widgGetWzString(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	static WzString emptyString = WzString();
	ASSERT_OR_RETURN(emptyString, psScreen != nullptr, "Invalid screen pointer");
	const WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(emptyString, psWidget, "Couldn't find widget by ID %u", id);

	static WzString ret;  // Must be static so it isn't immediately freed when this function returns.
	ret = psWidget->getString();
	return ret;
}

void WIDGET::setString(WzString)
{
	ASSERT(false, "Can't set widget type %u's string.", type);
}

/* Set the text in a widget */
void widgSetString(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, const char *pText)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	/* Get the widget */
	WIDGET *psWidget = widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(, psWidget, "Couldn't find widget by ID %u", id);

	if (psWidget->type == WIDG_EDITBOX && psScreen->hasFocus(*psWidget))
	{
		psScreen->setFocus(nullptr);
	}

	psWidget->setString(WzString::fromUtf8(pText));
}

void WIDGET::processCallbacksRecursive(W_CONTEXT *psContext)
{
	/* Go through all the widgets on the form */
	for (auto const &psCurr: childWidgets)
	{
		/* Call the callback */
		if (psCurr->callback)
		{
			psCurr->callback(psCurr.get(), psContext);
		}

		/* and then recurse */
		W_CONTEXT sFormContext(psContext);
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
	for (auto const &psCurr: childWidgets)
	{
		/* Skip any hidden widgets */
		if (!psCurr->visible())
		{
			continue;
		}

		/* Found a sub form, so set up the context */
		W_CONTEXT sFormContext(psContext);
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

bool WIDGET::hitTest(int x, int y)
{
	// default hit-testing bounding rect (based on the widget's x, y, width, height)
	bool hitTestResult = dim.contains(x, y);

	if(customHitTest)
	{
		// if the default bounding-rect hit-test succeeded, use the custom hit-testing func
		hitTestResult = hitTestResult && customHitTest(this, x, y);
	}

	return hitTestResult;
}

bool WIDGET::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	bool didProcessClick = false;

	W_CONTEXT shiftedContext(psContext);
	shiftedContext.mx = psContext->mx - x();
	shiftedContext.my = psContext->my - y();
	shiftedContext.xOffset = psContext->xOffset + x();
	shiftedContext.yOffset = psContext->yOffset + y();

	bool skipProcessingChildren = false;
	if (type == WIDG_FORM && ((W_FORM *)this)->disableChildren)
	{
		skipProcessingChildren = true;
	}

	if (!skipProcessingChildren)
	{
		// Process subwidgets.
		for (auto const &psCurr: childWidgets)
		{
			if (!psCurr->visible() || !psCurr->hitTest(shiftedContext.mx, shiftedContext.my))
			{
				continue;  // Skip any hidden widgets, or widgets the click missed.
			}

			// Process it (recursively).
			didProcessClick = psCurr->processClickRecursive(&shiftedContext, key, wasPressed) || didProcessClick;
		}
	}

	if (!visible())
	{
		// special case for root form
		// since the processClickRecursive of children is only called if they are visible
		// this should only trigger for the root form of a screen that's been set to invisible
		return didProcessClick;
	}

	if (!isTransparentToMouse && psMouseOverWidget.expired())
	{
		psMouseOverWidget = shared_from_this();  // Mark that the mouse is over a widget (if we haven't already).
	}

	if (isMouseOverWidget())
	{
		if (auto lockedScreen = screenPointer.lock())
		{
			if (!lockedScreen->isLastHighlight(*this))
			{
				if (auto lockedLastHighligh = lockedScreen->lastHighlight.lock())
				{
					lockedLastHighligh->highlightLost();
				}
				lockedScreen->lastHighlight = shared_from_this();  // Mark that the mouse is over a widget (if we haven't already).
				highlight(psContext);
			}
			if (psMouseOverWidgetScreen != lockedScreen)
			{
				if (psMouseOverWidgetScreen)
				{
					// ensure the last mouseOverWidgetScreen receives highlightLost event
					if (auto lockedLastHighlight = psMouseOverWidgetScreen->lastHighlight.lock())
					{
						lockedLastHighlight->highlightLost();
					}
					psMouseOverWidgetScreen->lastHighlight.reset();
				}
				// update psMouseOverWidgetScreen
				psMouseOverWidgetScreen = lockedScreen;
			}
		}
		else
		{
			// Note: There are rare exceptions where this can happen, if a WIDGET is doing something funky like drawing widgets it hasn't attached.
			psMouseOverWidgetScreen = nullptr;
		}

		if (key == WKEY_NONE)
		{
			// We're just checking mouse position, and this isn't a click, but return that we handled it
			return true;
		}
	}

	if (key == WKEY_NONE)
	{
		return didProcessClick;  // Just checking mouse position, not a click.
	}

	if (isMouseOverWidget())
	{
		auto psClickedWidget = shared_from_this();
		if (isTransparentToClicks)
		{
			do {
				psClickedWidget = psClickedWidget->parent();
			} while (psClickedWidget != nullptr && psClickedWidget->isTransparentToClicks);
			if (psClickedWidget == nullptr)
			{
				return false;
			}
		}
		if (wasPressed)
		{
			psClickedWidget->clicked(psContext, key);
			psClickDownWidgetScreen = psClickedWidget->screenPointer.lock();
		}
		else
		{
			psClickedWidget->released(psContext, key);
			psClickDownWidgetScreen.reset();
		}
		didProcessClick = true;
	}

	return didProcessClick;
}


/* Execute a set of widgets for one cycle.
 * Returns a list of activated widgets.
 */
WidgetTriggers const &widgRunScreen(const std::shared_ptr<W_SCREEN> &psScreen)
{
	static WidgetTriggers assertReturn;
	ASSERT_OR_RETURN(assertReturn, psScreen != nullptr, "Invalid screen pointer");
	psScreen->retWidgets.clear();
	cleanupDeletedOverlays();

	/* Initialise the context */
	W_CONTEXT sContext = W_CONTEXT::ZeroContext();
	psMouseOverWidget.reset();

#ifdef DEBUG
	psCurrentlyRunningScreen = psScreen;
#endif

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
			bool didProcessClick = false;
			forEachOverlayScreen([&sContext, &didProcessClick, wkey, pressed](const OverlayScreen& overlay) -> bool
			{
				didProcessClick = overlay.psScreen->psForm->processClickRecursive(&sContext, wkey, pressed);
				return !didProcessClick;
			});
			if (!didProcessClick)
			{
				psScreen->psForm->processClickRecursive(&sContext, wkey, pressed);
			}

			lastReleasedKey_DEPRECATED = wkey;
		}
	}

	sContext.mx = mouseX();
	sContext.my = mouseY();
	bool didProcessClick = false;
	forEachOverlayScreen([&sContext, &didProcessClick](const OverlayScreen& overlay) -> bool
	{
		didProcessClick = overlay.psScreen->psForm->processClickRecursive(&sContext, WKEY_NONE, true);  // Update highlights and psMouseOverWidget.
		return !didProcessClick;
	});
	if (!didProcessClick)
	{
		psScreen->psForm->processClickRecursive(&sContext, WKEY_NONE, true);  // Update highlights and psMouseOverWidget.
	}
	if (psMouseOverWidget.lock() == nullptr)
	{
		psMouseOverWidgetScreen.reset();
	}

	/* Process the screen's widgets */
	forEachOverlayScreen([&sContext](const OverlayScreen& overlay) -> bool
	{
		overlay.psScreen->psForm->runRecursive(&sContext);
		overlay.psScreen->psForm->run(&sContext); // ensure run() is called on root form
		return true;
	});
	psScreen->psForm->runRecursive(&sContext);

#ifdef DEBUG
	psCurrentlyRunningScreen = nullptr;
#endif

	runScheduledTasks();
	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being run.
	cleanupDeletedOverlays();

	/* Return the ID of a pressed button or finished edit box if any */
	return psScreen->retWidgets;
}

/* Set the id number for widgRunScreen to return */
void W_SCREEN::setReturn(const std::shared_ptr<WIDGET> &psWidget)
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
			displayFunction(this, context.getXOffset(), context.getYOffset());
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
void widgDisplayScreen(const std::shared_ptr<W_SCREEN> &psScreen)
{
	ASSERT_OR_RETURN(, psScreen != nullptr, "Invalid screen pointer");
	// To toggle debug bounding boxes: Press: Left Shift   --  --  --------------
	//                                        Left Ctrl  ------------  --  --  ----
	static const int debugSequence[] = { -1, 0, 1, 3, 1, 3, 1, 3, 2, 3, 2, 3, 2, 3, 1, 0, -1};
	static int const *debugLoc = debugSequence;
	static bool debugBoundingBoxes = false;
	int debugCode = keyDown(KEY_LCTRL) + 2 * keyDown(KEY_LSHIFT);
	debugLoc = debugLoc[1] == -1 ? debugSequence : debugLoc[0] == debugCode ? debugLoc : debugLoc[1] == debugCode ? debugLoc + 1 : debugSequence;
	debugBoundingBoxes = debugBoundingBoxes ^ (debugLoc[1] == -1);

	bool skipDrawing = !gfx_api::context::get().shouldDraw();

	cleanupDeletedOverlays();

	/* Process any user callback functions */
	W_CONTEXT sContext = W_CONTEXT::ZeroContext();
	sContext.mx = mouseX();
	sContext.my = mouseY();
	psScreen->psForm->processCallbacksRecursive(&sContext);

	if (!skipDrawing)
	{
		// Display the widgets.
		psScreen->psForm->displayRecursive();
	}

	// Always overlays on-top (i.e. draw them last)
	forEachOverlayScreenBottomUp([&sContext, skipDrawing](const OverlayScreen& overlay) -> bool
	{
		overlay.psScreen->psForm->processCallbacksRecursive(&sContext);
		if (!skipDrawing)
		{
			overlay.psScreen->psForm->displayRecursive();
		}
		return true;
	}); // <- enumerate in *increasing* z-order for drawing

	deleteOldWidgets();  // Delete any widgets that called deleteLater() while being displayed.

	if (!skipDrawing)
	{
		/* Display the tool tip if there is one */
		tipDisplay();
	}

	if (debugBoundingBoxes && !skipDrawing)
	{
		debugBoundingBoxesOnly = true;
		psScreen->psForm->displayRecursive();
		debugBoundingBoxesOnly = false;
	}
}

void W_SCREEN::setFocus(const std::shared_ptr<WIDGET> &widget)
{
	if (auto locked = psFocus.lock())
	{
		if (bWidgetsInitialized) // do not call focusLost if widgets have already shutdown / are not initialized
		{
			locked->focusLost();
		}
	}
	psFocus = widget;
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

std::weak_ptr<WIDGET> getMouseOverWidget()
{
	return psMouseOverWidget;
}
