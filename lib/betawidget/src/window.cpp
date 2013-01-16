/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008  Elio Gubser
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

#include "window.h"

static windowVtbl vtbl;
static vector *windowVector = NULL;
static int screenWidth = -1, screenHeight = -1;

static const float borderRadius = 20;

const classInfo windowClassInfo =
{
	&widgetClassInfo,
	"window"
};

static void windowInitVtbl(window *self)
{
	static bool initialised = false;
	
	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.widgetVtbl = *(WIDGET(self)->vtbl);
		
		// Overload some of our parents methods
		vtbl.widgetVtbl.destroy     = windowDestroyImpl;
		vtbl.widgetVtbl.addChild    = windowAddChildImpl;
		vtbl.widgetVtbl.doLayout    = windowDoLayoutImpl;
		vtbl.widgetVtbl.doDraw      = windowDoDrawImpl;
		vtbl.widgetVtbl.doDrawMask  = windowDoDrawMaskImpl;
		
		vtbl.widgetVtbl.getMinSize  = windowGetMinSizeImpl;
		vtbl.widgetVtbl.getMaxSize  = windowGetMaxSizeImpl;
		
		initialised = true;
	}
	
	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;
	
	// Set our vtable
	self->vtbl = &vtbl;
}

void windowInit(window *self, const char *id, int w, int h)
{	
	// Init our parent
	widgetInit(((widget *)self), id);
	
	// Set our type
	((widget *)self)->classInfo = &windowClassInfo;
	
	// Prepare our vtable
	windowInitVtbl(self);
	
	// Make sure we have a window list to add the window to
	assert(windowVector);
	
	// Add ourselves to the window list
	vectorAdd(windowVector, self);
	
	// Set our size to (w,h)
	widgetResize(WIDGET(self), w, h);

	// Mask for exact mouse events
	widgetEnableMask(WIDGET(self));

	// Patterns initialisation
	windowSetBackgroundPattern(self, "window");
}

void windowDestroyImpl(widget *self)
{
	int i;
	
	// Remove ourself from the window list
	for (i = 0; i < vectorSize(windowVector); i++)
	{		
		// If the window matches, remove it from the vector
		if (vectorAt(windowVector, i) == self)
		{
			vectorRemoveAt(windowVector, i);
		}
	}
	
	// Call our parents destructor
	widgetDestroyImpl(self);
}

bool windowAddChildImpl(widget *self, widget *child)
{
	// We can only hold a single child, no more (use a container!)
	assert(vectorSize(self->children) < 1);
	
	// Call the normal widgetAddChild method directly
	return widgetAddChildImpl(self, child);
}

bool windowDoLayoutImpl(widget *self)
{
	/*
	 * Position our (only) child at (0,0) making it as big as possible.
	 */
	if (vectorSize(self->children))
	{
		widget *child = vectorHead(self->children);
		const size minSize = widgetGetMinSize(child);
		const size maxSize = widgetGetMaxSize(child);
		
		// Make sure we are large enough to hold the child
		if (minSize.x > self->size.x
		 || minSize.y > self->size.y)
		{
			return false;
		}
		
		// Position the widget at (0,0)
		widgetReposition(child, 0, 0);
		
		// Resize it so that it is as large as possible
		widgetResize(child, MIN(self->size.x, maxSize.x),
		                    MIN(self->size.y, maxSize.y));
	}
	
	return true;
}

void windowDoWindowPath(widget *self, cairo_t *cr)
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

void windowDoDrawImpl(widget *self)
{
	// Get drawing context
	cairo_t *cr = self->cr;

	// Select window gradient
	patternManagerSetAsSource(cr, WINDOW(self)->windowPattern, self->size.x, self->size.y);

	// Do the rounded rectangle path
	windowDoWindowPath(self, cr);
	
	// Finally fill the path
	cairo_fill(cr);
}

void windowDoDrawMaskImpl(widget *self)
{
	// Get the mask context
	cairo_t *cr = WIDGET(self)->maskCr;

	// Do the rounded rectangle path
	windowDoWindowPath(self, cr);

	// We don't have to specify the color, it's already set in our parent
	cairo_fill(cr);
}

size windowGetMinSizeImpl(widget *self)
{
	const size minSize = { 1.0f, 1.0f };
	
	return minSize;
}

size windowGetMaxSizeImpl(widget *self)
{
	// Note the int => float conversion
	const size maxSize = { INT16_MAX, INT16_MAX };
	
	return maxSize;
}

void windowSetWindowVector(vector *v)
{
	// Make sure the vector is valid
	assert(v != NULL);
	
	windowVector = v;
}

vector *windowGetWindowVector()
{
	return windowVector;
}

void windowHandleEventForWindowVector(const event *evt)
{
	int i;
	
	// Make sure the vector is valid
	assert(windowVector != NULL);
	
	// Pass the event to each of the windows
	for (i = 0; i < vectorSize(windowVector); i++)
	{
		widgetHandleEvent(vectorAt(windowVector, i), evt);
	}
}

void windowSetScreenSize(int w, int h)
{
	screenWidth = w;
	screenHeight = h;
}

void windowRepositionFromScreen(window *self, hAlign hAlign, int xOffset,
                                              vAlign vAlign, int yOffset)
{
	int x = 0, y = 0;
	size ourSize = WIDGET(self)->size;
	
	// Ensure the screen width and height have been set
	assert(screenWidth != -1 && screenHeight != -1);
	
	switch (hAlign)
	{
		case LEFT:
			x = 0;
			break;
		case CENTRE:
			x = (screenWidth - ourSize.x) / 2;
			break;
		case RIGHT:
			x = screenWidth;
			
			// Transform xOffset so that +ve moves us away from the right
			xOffset = -xOffset;
			break;
	}
	
	switch (vAlign)
	{
		case TOP:
			y = 0;
			break;
		case MIDDLE:
			y = (screenHeight - ourSize.y) / 2;
			break;
		case BOTTOM:
			x = screenHeight;
			
			// Transform yOffset so that +ve moves us away from the bottom
			yOffset = -yOffset;
			break;
	}
	
	// Take offsets into account
	x += xOffset;
	y += yOffset;
	
	// Reposition
	widgetReposition(WIDGET(self), x, y);
}

void windowRepositionFromAnchor(window *self, const window *anchor,
                                hAlign hAlign, int xOffset,
                                vAlign vAlign, int yOffset)
{
	int x = 0, y = 0;
	size anchorSize = WIDGET(anchor)->size;
	point anchorPos = WIDGET(anchor)->offset;
	size ourSize = WIDGET(self)->size;
	
	switch (hAlign)
	{
		case LEFT:
			x = anchorPos.x - ourSize.x;
			
			// Transform xOffset so that +ve moves us away from anchorPos.x
			xOffset = -xOffset;
			break;
		case CENTRE:
			x = anchorPos.x + (anchorSize.x - ourSize.x) / 2;
			break;
		case RIGHT:
			x = anchorPos.x + anchorSize.x;
			break;
	}
	
	switch (vAlign)
	{
		case TOP:
			y = anchorPos.y;
			break;
		case MIDDLE:
			y = anchorPos.y + (anchorSize.x - ourSize.y) / 2;
			break;
		case BOTTOM:
			y = anchorPos.y + anchorSize.y;
			break;
	}
	
	// Take the offsets into account
	x += xOffset;
	y += yOffset;
	
	// Reposition
	widgetReposition(WIDGET(self), x, y);
}

void windowSetBackgroundPattern(window *self, const char *patternId)
{
	assert(self != NULL);
	assert(patternId != NULL);
	
	self->windowPattern = patternManagerGetPattern(patternId);
	
	WIDGET(self)->needsRedraw = true;
	widgetDraw(WIDGET(self));
}
