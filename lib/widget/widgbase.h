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
 *  Definitions for the basic widget types.
 */

#ifndef __INCLUDED_LIB_WIDGET_WIDGBASE_H__
#define __INCLUDED_LIB_WIDGET_WIDGBASE_H__

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/textdraw.h"
#include <vector>
#include <functional>
#include <string>
#include "lib/framework/geometry.h"
#include "lib/framework/wzstring.h"


/* Forward definitions */
class WIDGET;
struct W_CONTEXT;
class W_FORM;
struct W_INIT;
struct W_SCREEN;
class W_EDITBOX;
class W_BARGRAPH;
class W_BUTTON;
class W_LABEL;
class W_SLIDER;
class StateButton;
class ListWidget;
class ScrollBarWidget;

/* The display function prototype */
typedef void (*WIDGET_DISPLAY)(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

/* The optional user callback function */
typedef void (*WIDGET_CALLBACK)(WIDGET *psWidget, W_CONTEXT *psContext);
typedef void (*WIDGET_AUDIOCALLBACK)(int AudioID);

/* The optional "calc layout" callback function, to support runtime layout recalculation */
typedef std::function<void (WIDGET *psWidget, unsigned int oldScreenWidth, unsigned int oldScreenHeight, unsigned int newScreenWidth, unsigned int newScreenHeight)> WIDGET_CALCLAYOUT_FUNC;

// To avoid typing, use the following define to construct a lambda for WIDGET_CALCLAYOUT_FUNC
// psWidget is the widget
// The { } are still required (for clarity).
#define LAMBDA_CALCLAYOUT_SIMPLE(x) [](WIDGET *psWidget, unsigned int, unsigned int, unsigned int, unsigned int) x

/* The optional "onDelete" callback function */
typedef std::function<void (WIDGET *psWidget)> WIDGET_ONDELETE_FUNC;

/* The optional hit-testing function, used for custom hit-testing within the outer bounding rectangle */
typedef std::function<bool (WIDGET *psWidget, int x, int y)> WIDGET_HITTEST_FUNC;


/* The different base types of widget */
enum WIDGET_TYPE
{
	WIDG_FORM,
	WIDG_LABEL,
	WIDG_BUTTON,
	WIDG_EDITBOX,
	WIDG_BARGRAPH,
	WIDG_SLIDER,
	WIDG_UNSPECIFIED_TYPE,
};

/* The keys that can be used to press a button */
enum WIDGET_KEY
{
	WKEY_NONE,
	WKEY_PRIMARY,
	WKEY_SECONDARY,
};

enum
{
	WIDG_HIDDEN = 0x8000,  ///< The widget is initially hidden
};

// Possible states for a button or clickform.
enum ButtonState
{
	WBUT_DISABLE   = 0x01,  ///< Disable (grey out) a button.
	WBUT_LOCK      = 0x02,  ///< Fix a button down.
	WBUT_CLICKLOCK = 0x04,  ///< Fix a button down but it is still clickable.
	WBUT_FLASH     = 0x08,  ///< Make a button flash.
	WBUT_DOWN      = 0x10,  ///< Button is down.
	WBUT_HIGHLIGHT = 0x20,  ///< Button is highlighted.
};

class WidgetGraphicsContext
{
private:
	Vector2i offset = {0, 0};
	WzRect clipRect = {0, 0, 0, 0};
	bool clipped = false;

public:
	int32_t getXOffset() const
	{
		return offset.x;
	}

	int32_t getYOffset() const
	{
		return offset.y;
	}

	bool clipContains(WzRect const& rect) const;

	WidgetGraphicsContext translatedBy(int32_t x, int32_t y) const;

	WidgetGraphicsContext clippedBy(WzRect const &newRect) const;
};

/* The base widget data type */
class WIDGET: public std::enable_shared_from_this<WIDGET>
{

public:
	typedef std::vector<std::shared_ptr<WIDGET>> Children;

	WIDGET(WIDGET_TYPE type = WIDG_UNSPECIFIED_TYPE);
	WIDGET(W_INIT const *init, WIDGET_TYPE type = WIDG_UNSPECIFIED_TYPE);
	virtual ~WIDGET();

	void deleteLater();  ///< Like "delete this;", but safe to call from display/run callbacks.

	virtual void widgetLost(WIDGET *);

	virtual void focusLost() {}
	virtual void clicked(W_CONTEXT *, WIDGET_KEY = WKEY_PRIMARY) {}

protected:
	virtual void released(W_CONTEXT *, WIDGET_KEY = WKEY_PRIMARY) {}
	virtual void highlight(W_CONTEXT *) {}
	virtual void highlightLost() {}
	virtual void run(W_CONTEXT *) {}
	virtual void display(int, int) {}
	virtual void geometryChanged() {}

	virtual bool hitTest(int x, int y);

public:
	virtual unsigned getState();
	virtual void setState(unsigned state);
	virtual void setFlash(bool enable);
	virtual WzString getString() const;
	virtual void setString(WzString string);
	virtual void setTip(std::string string);

	virtual void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight); // used to handle screen resizing

	void show(bool doShow = true)
	{
		style = (style & ~WIDG_HIDDEN) | (!doShow * WIDG_HIDDEN);
	}
	void hide()
	{
		show(false);
	}
	bool visible()
	{
		return (style & WIDG_HIDDEN) == 0;
	}

	void setTip(char const *stringUtf8)
	{
		setTip((stringUtf8 != nullptr) ? std::string(stringUtf8) : std::string());
	}

	std::shared_ptr<WIDGET> parent() const
	{
		return parentWidget.lock();
	}
	Children const &children()
	{
		return childWidgets;
	}
	void removeAllChildren()
	{
		for (auto& child : childWidgets)
		{
			if (child->parent().get() == this)
			{
				child->parentWidget.reset();
				child->setScreenPointer(nullptr);
			}
		}
		childWidgets = {};
	}
	WzRect const &geometry() const
	{
		return dim;
	}
	int x() const
	{
		return dim.x();
	}
	int y() const
	{
		return dim.y();
	}
	int screenPosX() const
	{
		int screenX = x();
		for (auto psParent = parent(); psParent != nullptr; psParent = psParent->parent())
		{
			screenX += psParent->x();
		}
		return screenX;
	}
	int screenPosY() const
	{
		int screenY = y();
		for (auto psParent = parent(); psParent != nullptr; psParent = psParent->parent())
		{
			screenY += psParent->y();
		}
		return screenY;
	}
	int width() const
	{
		return dim.width();
	}
	int height() const
	{
		return dim.height();
	}
	void move(int x, int y)
	{
		setGeometry(WzRect(x, y, width(), height()));
	}
	void setGeometry(int x, int y, int w, int h)
	{
		setGeometry(WzRect(x, y, w, h));
	}
	void setGeometry(WzRect const &r);

	void attach(const std::shared_ptr<WIDGET> &widget);
	/**
	 * @deprecated use `void WIDGET::attach(const std::shared_ptr<WIDGET> &widget)` instead
	 **/
	void attach(WIDGET *widget) { attach(widget->shared_from_this()); }

	void detach(const std::shared_ptr<WIDGET> &widget);
	/**
	 * @deprecated use `void WIDGET::detach(const std::shared_ptr<WIDGET> &widget)` instead
	 **/
	void detach(WIDGET *widget) { detach(widget->shared_from_this()); }

	void setCalcLayout(const WIDGET_CALCLAYOUT_FUNC& calcLayoutFunc);
	void callCalcLayout();

	void setOnDelete(const WIDGET_ONDELETE_FUNC& onDeleteFunc);

	void setCustomHitTest(const WIDGET_HITTEST_FUNC& newCustomHitTestFunc);

	bool isMouseOverWidget() const;

	UDWORD                  id;                     ///< The user set ID number for the widget. This is returned when e.g. a button is pressed.
	WIDGET_TYPE             type;                   ///< The widget type
	UDWORD                  style;                  ///< The style of the widget
	WIDGET_DISPLAY          displayFunction;        ///< Override function to display the widget.
	WIDGET_CALLBACK         callback;               ///< User callback (if any)
	void                   *pUserData;              ///< Pointer to a user data block (if any)
	UDWORD                  UserData;               ///< User data (if any)
	std::weak_ptr<W_SCREEN> screenPointer;          ///< Pointer to screen the widget is on (if attached).

private:
	WIDGET_CALCLAYOUT_FUNC  calcLayout;				///< Optional calc layout callback
	WIDGET_ONDELETE_FUNC	onDelete;				///< Optional callback called when the Widget is about to be deleted
	WIDGET_HITTEST_FUNC		customHitTest;			///< Optional hit-testing custom function
	void setScreenPointer(const std::shared_ptr<W_SCREEN> &screen); ///< Set screen pointer for us and all children.
public:
	virtual bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed);
	void runRecursive(W_CONTEXT *psContext);
	void processCallbacksRecursive(W_CONTEXT *psContext);
	virtual void displayRecursive(WidgetGraphicsContext const &context);  ///< Display this widget, and all visible children.
	void displayRecursive()
	{
		WidgetGraphicsContext context;
		displayRecursive(context);
	}

private:
	std::weak_ptr<WIDGET> parentWidget;
	std::vector<std::shared_ptr<WIDGET>> childWidgets;

	WzRect                  dim;

	WIDGET(WIDGET const &) = delete;
	WIDGET &operator =(WIDGET const &) = delete;

public:
	bool dirty; ///< Whether widget is changed and needs to be redrawn
public:
	friend bool isMouseOverScreenOverlayChild(int mx, int my);
};


struct WidgetTrigger
{
	std::shared_ptr<WIDGET> widget;
};
typedef std::vector<WidgetTrigger> WidgetTriggers;

/* The screen structure which stores all info for a widget screen */
struct W_SCREEN: public std::enable_shared_from_this<W_SCREEN>
{
protected:
	W_SCREEN(): TipFontID(font_regular) {}
	void initialize();

public:
	static std::shared_ptr<W_SCREEN> make()
	{
		class make_shared_enabler: public W_SCREEN {};
		auto screen = std::make_shared<make_shared_enabler>();
		screen->initialize();
		return screen;
	}

	void setFocus(const std::shared_ptr<WIDGET> &widget);  ///< Sets psFocus, notifying the old widget, if any.
	void setReturn(const std::shared_ptr<WIDGET> &psWidget);  ///< Adds psWidget to retWidgets.
	void screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight); // used to handle screen resizing

	std::shared_ptr<W_FORM> psForm; ///< The root form of the screen
	std::weak_ptr<WIDGET> psFocus;  ///< The widget that has keyboard focus
	std::weak_ptr<WIDGET> lastHighlight; ///< The last widget to be highlighted. This is used to track when the mouse moves off something.
	iV_fonts         TipFontID;     ///< ID of the IVIS font to use for tool tips.
	WidgetTriggers   retWidgets;    ///< The widgets to be returned by widgRunScreen.

	bool hasFocus(WIDGET const &widget) const
	{
		return psFocus.lock().get() == &widget;
	}

	bool isLastHighlight(WIDGET const &widget) const
	{
		return lastHighlight.lock().get() == &widget;
	}

private:
#ifdef WZ_CXX11
	W_SCREEN(W_SCREEN const &) = delete;
	W_SCREEN &operator =(W_SCREEN const &) = delete;
#else
	W_SCREEN(W_SCREEN const &);  // Non-copyable.
	W_SCREEN &operator =(W_SCREEN const &);  // Non-copyable.
#endif
};

/* Context information to pass into the widget functions */
struct W_CONTEXT
{
public:
	SDWORD		xOffset, yOffset;	// Screen offset of the parent form
	SDWORD		mx, my;				// mouse position on the form

private:
	W_CONTEXT()
	: xOffset(0)
	, yOffset(0)
	, mx(0)
	, my(0)
	{ }
public:
	static W_CONTEXT ZeroContext()
	{
		return W_CONTEXT();
	}
	W_CONTEXT(SDWORD xOffset, SDWORD yOffset, SDWORD mx, SDWORD my)
	: xOffset(xOffset)
	, yOffset(yOffset)
	, mx(mx)
	, my(my)
	{ }
	W_CONTEXT(const W_CONTEXT& other) = default;
	W_CONTEXT(const W_CONTEXT* other)
	: xOffset(0)
	, yOffset(0)
	, mx(0)
	, my(0)
	{
		ASSERT_OR_RETURN(, other, "Initializing with null W_CONTEXT");
		*this = *other;
	}
	W_CONTEXT& operator=(const W_CONTEXT& other) = default;
};

#endif // __INCLUDED_LIB_WIDGET_WIDGBASE_H__
