/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008  Warzone Resurrection Project

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

const classInfo buttonClassInfo =
{
	&widgetClassInfo,
	"button"
};

static bool buttonSetNeedsRedrawHandler(widget *self, const event *evt,
                                        int handlerId, void *userData)
{
	// Request a redraw
	self->needsRedraw = true;

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
		vtbl.widgetVtbl.destroy = buttonDestroyImpl;
		vtbl.widgetVtbl.doDraw  = buttonDoDrawImpl;

		initialised = true;
	}

	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;

	// Set our vtable
	self->vtbl = &vtbl;
}

void buttonInit(button *self, const char *id)
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
	widgetInit(WIDGET(self), id);

	// Prepare our vtable
	buttonInitVtbl(self);

	// Set our type
	WIDGET(self)->classInfo = &buttonClassInfo;

	// Install necessary event handlers
	for (i = 0; i < sizeof(handlers) / sizeof(int); i++)
	{
		widgetAddEventHandler(WIDGET(self), handlers[i],
		                      buttonSetNeedsRedrawHandler, NULL, NULL);
	}
}


void buttonDestroyImpl(widget *self)
{
	// Call our parents destructor
	widgetDestroy(self);
}

void buttonDoDrawImpl(widget *selfWidget)
{
	// Cast ourself to a button
	button *self = BUTTON(selfWidget);

	// If we are enabled
	if (selfWidget->isEnabled)
	{
		// See if the mouse is over us or not
		if (selfWidget->hasMouse)
		{
			// See if the mouse is down or not
			if (selfWidget->hasMouseDown)
			{
				// Call the mouse down draw method
				buttonDoMouseDownDraw(self);
			}
			// Mouse is over but not down
			else
			{
				// Call the mouse over draw method
				buttonDoMouseOverDraw(self);
			}
		}
		// Mouse is neither over or down
		else
		{
			// Call the normal mouse draw method
			buttonDoNormalDraw(self);
		}
	}
	// Otherwise if we are disabled
	else
	{
		// Call the disabled draw method
		buttonDoDisabledDraw(self);
	}
}

void buttonDoDisabledDrawImpl(button *self)
{
	int x, y;
	cairo_surface_t *surface = cairo_get_target(WIDGET(self)->cr);
	const int width = cairo_image_surface_get_width(surface);
	const int height = cairo_image_surface_get_height(surface);
	const int stride = cairo_image_surface_get_stride(surface);
	uint8_t *data = cairo_image_surface_get_data(surface);

	// Call the normal draw method first
	buttonDoNormalDraw(self);

	// Greyscale it
	// TODO
}

void buttonDoNormalDraw(button *self)
{
	BUTTON_CHECK_METHOD(self, doNormalDraw);

	BUTTON_GET_VTBL(self)->doNormalDraw(self);
}

void buttonDoDisabledDraw(button *self)
{
	BUTTON_CHECK_METHOD(self, doDisabledDraw);

	BUTTON_GET_VTBL(self)->doDisabledDraw(self);
}

void buttonDoMouseOverDraw(button *self)
{
	BUTTON_CHECK_METHOD(self, doMouseOverDraw);

	BUTTON_GET_VTBL(self)->doMouseOverDraw(self);
}

void buttonDoMouseDownDraw(button *self)
{
	BUTTON_CHECK_METHOD(self, doMouseDownDraw);

	BUTTON_GET_VTBL(self)->doMouseDownDraw(self);
}
