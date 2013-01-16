/*
	This file is part of Warzone 2100.
	Copyright (C) 2009  Elio Gubser
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2013  Warzone 2100 Project

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

#include "button.h"

static buttonVtbl vtbl;

static const float borderRadius = 10;

const classInfo buttonClassInfo =
{
	&widgetClassInfo,
	"button"
};

static bool buttonSetButtonStateHandler(widget *selfWidget, const event *evt,
                                        int handlerId, void *userData)
{
	// Cast ourself to a button
	button *self = BUTTON(selfWidget);

	// Request a redraw
	WIDGET(self)->needsRedraw = true;

	switch(evt->type)
	{
		case EVT_MOUSE_DOWN:	self->state = BUTTON_STATE_MOUSEDOWN; break;
		case EVT_MOUSE_ENTER:	self->state = BUTTON_STATE_MOUSEOVER; break;
		case EVT_MOUSE_UP:		self->state = BUTTON_STATE_MOUSEOVER; break;
		case EVT_MOUSE_LEAVE:
		case EVT_ENABLE:		self->state = BUTTON_STATE_NORMAL; break;
		case EVT_DISABLE:		self->state = BUTTON_STATE_DISABLED; break;
		default: assert(0);
	}

	return true;
}

static void buttonInitVtbl(button *self)
{
	static bool initialised = false;

	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.widgetVtbl = *(WIDGET(self)->vtbl);

		// Overload the widget's destroy and doDraw methods
		vtbl.widgetVtbl.destroy		= buttonDestroyImpl;
		vtbl.widgetVtbl.doDraw		= buttonDoDrawImpl;
		vtbl.widgetVtbl.doDrawMask 	= buttonDoDrawMaskImpl;

		vtbl.doButtonPath 			= buttonDoButtonPathImpl;
		vtbl.doDrawNormal			= buttonDoDrawNormalImpl;
		vtbl.doDrawDisabled			= buttonDoDrawDisabledImpl;
		vtbl.doDrawMouseOver		= buttonDoDrawMouseOverImpl;
		vtbl.doDrawMouseDown		= buttonDoDrawMouseDownImpl;

		vtbl.widgetVtbl.getMinSize  = buttonGetMinSizeImpl;
		vtbl.widgetVtbl.getMaxSize  = buttonGetMaxSizeImpl;

		initialised = true;
	}

	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;

	// Set our vtable
	self->vtbl = &vtbl;
}

button *buttonCreate(const char *id, int w, int h)
{
	button *instance = malloc(sizeof(button));

	if (instance == NULL)
	{
		return NULL;
	}

	// Call the constructor
	buttonInit(instance, id, w, h);

	// Return the new object
	return instance;
}

void buttonInit(button *self, const char *id, int w, int h)
{
	// Buttons need to be redrawn when the following events fire
	int i, handlers[] = {
	    EVT_ENABLE,
	    EVT_DISABLE,
	    EVT_MOUSE_ENTER,
	    EVT_MOUSE_LEAVE,
	    EVT_MOUSE_DOWN,
	    EVT_MOUSE_UP
	};

	// Init our parent
	widgetInit((widget *)self, id);

	// Prepare our vtable
	buttonInitVtbl(self);

	// Set our type
	((widget *)self)->classInfo = &buttonClassInfo;

	// initialise button state
	self->state = BUTTON_STATE_NORMAL;

	// Mask for exact mouse events
	widgetEnableMask(WIDGET(self));

	// Install necessary event handlers
	for (i = 0; i < sizeof(handlers) / sizeof(int); i++)
	{
		widgetAddEventHandler(WIDGET(self), handlers[i],
		                      buttonSetButtonStateHandler, NULL, NULL);
	}

	buttonSetPatternsForState(self, BUTTON_STATE_NORMAL, "button/normal/fill", "button/normal/contour");
	buttonSetPatternsForState(self, BUTTON_STATE_DISABLED, "button/disabled/fill", "button/disabled/contour");
	buttonSetPatternsForState(self, BUTTON_STATE_MOUSEOVER, "button/mouseover/fill", "button/mouseover/contour");
	buttonSetPatternsForState(self, BUTTON_STATE_MOUSEDOWN, "button/mousedown/fill", "button/mousedown/contour");

	widgetResize(WIDGET(self), w, h);
}


void buttonDestroyImpl(widget *self)
{
	// Call our parents destructor
	widgetDestroyImpl(self);
}

void buttonDoDrawImpl(widget *widgetSelf)
{
	button *self;

	self = BUTTON(widgetSelf);
	switch(self->state)
	{
		case BUTTON_STATE_NORMAL: 		buttonDoDrawNormal(self); break;
		case BUTTON_STATE_DISABLED: 	buttonDoDrawDisabled(self); break;
		case BUTTON_STATE_MOUSEOVER:	buttonDoDrawMouseOver(self); break;
		case BUTTON_STATE_MOUSEDOWN:	buttonDoDrawMouseDown(self); break;
		default: assert(0); break;
	}
}

void buttonDoDrawMaskImpl(widget *self)
{
	// Get the mask context
	cairo_t *cr = self->maskCr;

	// Do the rounded rectangle path
	buttonDoButtonPath(BUTTON(self), cr);

	// We don't have to specify the color, it's already set in our parent
	cairo_fill(cr);
}

size buttonGetMinSizeImpl(widget *self)
{
	const size minSize = { 60.0f, 40.0f };

	return minSize;
}
size buttonGetMaxSizeImpl(widget *self)
{
	// Note the int => float conversion
	const size maxSize = { 60.f, 40.f };

	return maxSize;
}

/**
 * Draws the button's shape and background using the delivered patterns.
 *
 * @param self The button object which will be drawn.
 * @param fillPattern The pattern to fill its shape.
 * @param contourPattern The pattern to outline its shape.
 */
static void buttonDrawFinally(button *self, pattern *fillPattern, pattern *contourPattern)
{
	// Get drawing context
	cairo_t *cr = WIDGET(self)->cr;

	assert(self != NULL);
	assert(fillPattern != NULL);
	assert(contourPattern != NULL);

	// Do the rounded rectangle path
	buttonDoButtonPath(self, cr);

	// Select normal state fill gradient
	patternManagerSetAsSource(cr, fillPattern, WIDGET(self)->size.x, WIDGET(self)->size.y);
	// Fill the path, preserving the path
	cairo_fill_preserve(cr);

	// Select normal state contour gradient
	patternManagerSetAsSource(cr, contourPattern, WIDGET(self)->size.x, WIDGET(self)->size.y);
	// Finally stroke the path
	cairo_stroke(cr);
}

void buttonDoDrawNormalImpl(button *self)
{
	buttonDrawFinally(self, self->fillPatterns[BUTTON_STATE_NORMAL], self->contourPatterns[BUTTON_STATE_NORMAL]);
}

void buttonDoDrawDisabledImpl(button *self)
{
	buttonDrawFinally(self, self->fillPatterns[BUTTON_STATE_DISABLED], self->contourPatterns[BUTTON_STATE_DISABLED]);
}

void buttonDoDrawMouseOverImpl(button *self)
{
	buttonDrawFinally(self, self->fillPatterns[BUTTON_STATE_MOUSEOVER], self->contourPatterns[BUTTON_STATE_MOUSEOVER]);
}

void buttonDoDrawMouseDownImpl(button *self)
{
	buttonDrawFinally(self, self->fillPatterns[BUTTON_STATE_MOUSEDOWN], self->contourPatterns[BUTTON_STATE_MOUSEDOWN]);
}

void buttonDoButtonPathImpl(button *self, cairo_t *cr)
{
	size ourSize = WIDGET(self)->size;

	// Do the rounded rectangle
	cairo_arc_negative(cr, borderRadius, borderRadius, borderRadius, -M_PI_2, -M_PI);
	cairo_line_to(cr, 0, ourSize.y-borderRadius);
	cairo_arc_negative(cr, borderRadius, ourSize.y-borderRadius, borderRadius, M_PI, M_PI_2);
	cairo_line_to(cr, ourSize.x-borderRadius, ourSize.y);
	cairo_arc_negative(cr, ourSize.x-borderRadius, ourSize.y-borderRadius, borderRadius, M_PI_2, 0);
	cairo_line_to(cr, ourSize.x, borderRadius);
	cairo_arc_negative(cr, ourSize.x-borderRadius, borderRadius, borderRadius, 0, -M_PI_2);
	cairo_close_path(cr);
}

void buttonDoDrawNormal(button *self)
{
	BUTTON_CHECK_METHOD(self, doDrawNormal);

	BUTTON_GET_VTBL(self)->doDrawNormal(self);
}

void buttonDoDrawDisabled(button *self)
{
	BUTTON_CHECK_METHOD(self, doDrawDisabled);

	BUTTON_GET_VTBL(self)->doDrawDisabled(self);
}

void buttonDoDrawMouseOver(button *self)
{
	BUTTON_CHECK_METHOD(self, doDrawMouseOver);

	BUTTON_GET_VTBL(self)->doDrawMouseOver(self);
}

void buttonDoDrawMouseDown(button *self)
{
	BUTTON_CHECK_METHOD(self, doDrawMouseDown);

	BUTTON_GET_VTBL(self)->doDrawMouseDown(self);
}

void buttonDoButtonPath(button *self, cairo_t *cr)
{
	BUTTON_CHECK_METHOD(self, doButtonPath);

	BUTTON_GET_VTBL(self)->doButtonPath(self, cr);
}

void buttonSetPatternsForState(button *self, buttonState state, const char *fillPatternId, const char *contourPatternId)
{
	assert(self != NULL);
	assert(state < BUTTON_STATE_COUNT);

	if(fillPatternId)
	{
		if(fillPatternId[0] != '\0')
		{
			self->fillPatterns[state] = patternManagerGetPattern(fillPatternId);
		}
	}

	if(contourPatternId)
	{
		if(contourPatternId[0] != '\0')
		{
			self->contourPatterns[state] = patternManagerGetPattern(contourPatternId);
		}
	}
}
