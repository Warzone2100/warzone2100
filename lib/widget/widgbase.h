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
 *  Definitions for the basic widget types.
 */

#ifndef __INCLUDED_LIB_WIDGET_WIDGBASE_H__
#define __INCLUDED_LIB_WIDGET_WIDGBASE_H__

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/textdraw.h"
#include <QtCore/QRect>


/* Forward definitions */
struct WIDGET;
struct W_CONTEXT;
struct W_FORM;
struct W_INIT;
struct W_SCREEN;
struct W_EDITBOX;
struct W_BARGRAPH;
struct W_BUTTON;
struct W_LABEL;
struct W_SLIDER;

/* The display function prototype */
typedef void (*WIDGET_DISPLAY)(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

/* The optional user callback function */
typedef void (*WIDGET_CALLBACK)(WIDGET *psWidget, W_CONTEXT *psContext);
typedef void (*WIDGET_AUDIOCALLBACK)(int AudioID);


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

/* The base widget data type */
struct WIDGET
{
	typedef std::vector<WIDGET *> Children;


	WIDGET(W_INIT const *init, WIDGET_TYPE type);
	WIDGET(WIDGET *parent, WIDGET_TYPE type = WIDG_UNSPECIFIED_TYPE);
	virtual ~WIDGET();

	virtual void widgetLost(WIDGET *);

	virtual void clicked(W_CONTEXT *, WIDGET_KEY = WKEY_PRIMARY) {}
	virtual void released(W_CONTEXT *, WIDGET_KEY = WKEY_PRIMARY) {}
	virtual void highlight(W_CONTEXT *) {}
	virtual void highlightLost() {}
	virtual void focusLost(W_SCREEN *) {}
	virtual void run(W_CONTEXT *) {}
	virtual void display(int, int) {}

	virtual unsigned getState();
	virtual void setState(unsigned state);
	virtual void setFlash(bool enable);
	virtual QString getString() const;
	virtual void setString(QString string);
	virtual void setTip(QString string);

	void show(bool doShow = true) { style = (style & ~WIDG_HIDDEN) | (!doShow*WIDG_HIDDEN); }
	void hide() { show(false); }
	bool visible() { return (style & WIDG_HIDDEN) == 0; }

	void setString(char const *stringUtf8) { setString(QString::fromUtf8(stringUtf8)); }
	void setTip(char const *stringUtf8) { setTip(QString::fromUtf8(stringUtf8)); }

	WIDGET *parent() { return parentWidget; }
	Children const &children() { return childWidgets; }
	QRect const &geometry() const { return dim; }
	int x() const { return dim.x(); }
	int y() const { return dim.y(); }
	int width() const { return dim.width(); }
	int height() const { return dim.height(); }
	void move(int x, int y) { dim.moveTo(x, y); }
	void setGeometry(int x, int y, int w, int h) { dim.setRect(x, y, w, h); }

	void attach(WIDGET *widget);
	void detach(WIDGET *widget);

	UDWORD                  id;                     ///< The user set ID number for the widget. This is returned when e.g. a button is pressed.
	WIDGET_TYPE             type;                   ///< The widget type
	UDWORD                  style;                  ///< The style of the widget
	WIDGET_DISPLAY          displayFunction;        ///< Override function to display the widget.
	WIDGET_CALLBACK         callback;               ///< User callback (if any)
	void                   *pUserData;              ///< Pointer to a user data block (if any)
	UDWORD                  UserData;               ///< User data (if any)
	W_SCREEN *              screenPointer;          ///< Pointer to screen the widget is on (if attached).

private:
	void setScreenPointer(W_SCREEN *screen);        ///< Set screen pointer for us and all children.
public:
	void displayRecursive(int xOffset, int yOffset);  ///< Display this widget, and all visible children.
private:

	WIDGET *                parentWidget;           ///< Parent widget.
	std::vector<WIDGET *>   childWidgets;           ///< Child widgets. Will be deleted if we are deleted.

	QRect                   dim;

private:
#ifdef WZ_CXX11
	WIDGET(WIDGET const &) = delete;
	WIDGET &operator =(WIDGET const &) = delete;
#else
	WIDGET(WIDGET const &);  // Non-copyable.
	WIDGET &operator =(WIDGET const &);  // Non-copyable.
#endif
};


struct WidgetTrigger
{
	WIDGET *widget;
};
typedef std::vector<WidgetTrigger> WidgetTriggers;

/* The screen structure which stores all info for a widget screen */
struct W_SCREEN
{
	W_SCREEN();
	~W_SCREEN();

	W_FORM          *psForm;        ///< The root form of the screen
	WIDGET          *psFocus;       ///< The widget that has keyboard focus
	iV_fonts         TipFontID;     ///< ID of the IVIS font to use for tool tips.
	WidgetTriggers   retWidgets;    ///< The widgets to be returned by widgRunScreen.

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
	W_SCREEN	*psScreen;			// Parent screen of the widget
	W_FORM         *psForm;                 ///< Parent form of the widget
	SDWORD		xOffset, yOffset;	// Screen offset of the parent form
	SDWORD		mx, my;				// mouse position on the form
};

#endif // __INCLUDED_LIB_WIDGET_WIDGBASE_H__
