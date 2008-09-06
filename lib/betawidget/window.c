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

#include "window.h"

static windowVtbl vtbl;
static vector *windowVector = NULL;
static int screenWidth = -1, screenHeight = -1;

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
		vtbl.widgetVtbl.resize      = windowResizeImpl;
		
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
	widgetInit(WIDGET(self), id);
	
	// Prepare our vtable
	windowInitVtbl(self);
	
	// Set our type
	WIDGET(self)->classInfo = &windowClassInfo;
	
	// Make sure we have a window list to add the window to
	assert(windowVector);
	
	// Add ourselves to the window list
	vectorAdd(windowVector, self);
	
	// Set our size to (w,h)
	widgetResize(WIDGET(self), w, h);
	
	// Default anchor state is static (as opposed to dynamic)
	self->anchorState = ANCHOR_STATIC;
	self->anchorWindow = NULL;
	self->anchorRepositionId = self->anchorResizeId = -1;
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

void windowDoDrawImpl(widget *self)
{
	// We really need something better than this
	cairo_rectangle(self->cr, 0.0, 0.0, self->size.x, self->size.y);
	cairo_set_source_rgba(self->cr, 0.0, 0.004, 0.380, 0.25);
	cairo_fill(self->cr);
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
			x = screenWidth / 2 - ourSize.x / 2;
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
			y = screenHeight / 2 - ourSize.y / 2;
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

void windowSetAnchorState(window *self, anchorState state)
{
	// If the anchor is being disabled, remove any active event handlers
	if (state == ANCHOR_STATIC && self->anchorWindow)
	{
		widget *anchorWidget = WIDGET(self->anchorWindow);

		// Remove the reposition event handler
		if (widgetIsEventHandler(anchorWidget, self->anchorRepositionId))
		{
			widgetRemoveEventHandler(anchorWidget, self->anchorRepositionId);
		}
		
		// Remove the resize event handler
		if (widgetIsEventHandler(anchorWidget, self->anchorResizeId))
		{
			widgetRemoveEventHandler(anchorWidget, self->anchorResizeId);
		}
		
		// We no longer have an anchor window
		self->anchorWindow = NULL;
	}
	
	// Update the state
	self->anchorState = state;
}

static void windowDoRepositionFromAnchor(window *self, const window *anchor,
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
			x = anchorPos.x + (anchorSize.x / 2 - ourSize.x / 2);
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
			y = anchorPos.y + (anchorSize.x / 2 - ourSize.y / 2);
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

void windowResizeImpl(widget *self, int w, int h)
{
	window *windowSelf = WINDOW(self);
	
	// Call our parents resize method
	widgetResizeImpl(self, w, h);
	
	// If dynamic anchoring is enabled, reposition ourself
	if (windowSelf->anchorState == ANCHOR_DYNAMIC && windowSelf->anchorWindow)
	{
		windowDoRepositionFromAnchor(windowSelf, windowSelf->anchorWindow, 
		                             windowSelf->anchorHAlign, 
		                             windowSelf->anchorXOffset, 
		                             windowSelf->anchorVAlign, 
		                             windowSelf->anchorYOffset);
	}
}

static bool windowAnchorCallback(widget *self, const event *evt, int handlerId,
                                 void *userData)
{
	// The window to be repositioned is stored in userData
	window *anchoredWindow = WINDOW(userData);
	
	// We should only be called to respond to resize and reposition eventd
	assert(evt->type == EVT_REPOSITION || evt->type == EVT_RESIZE);
	
	// Call windowDoRepositionFromAnchor to update the position
	windowDoRepositionFromAnchor(anchoredWindow, WINDOW(self),
	                             anchoredWindow->anchorHAlign,
	                             anchoredWindow->anchorXOffset,
	                             anchoredWindow->anchorVAlign,
	                             anchoredWindow->anchorYOffset);
	
	return true;
}

static bool windowAnchorDestroyCallback(widget *self, const event *evt,
                                        int handlerId, void *userData)
{
	// The window that installed us is in userData
	window *anchoredWindow = WINDOW(userData);
	
	// Make sure the event is a destruct event
	assert(evt->type == EVT_DESTRUCT);
	
	// We are being removed and therefore are no longer anchored
	anchoredWindow->anchorWindow = NULL;
	anchoredWindow->anchorRepositionId = anchoredWindow->anchorResizeId = -1;
	
	return true;
}

void windowRepositionFromAnchor(window *self, const window *anchor,
                                hAlign hAlign, int xOffset,
                                vAlign vAlign, int yOffset)
{
	// If dynamic anchors are enabled, set up dynamic anchoring
	if (self->anchorState == ANCHOR_DYNAMIC)
	{
		// Get the current anchor (so that we may remove the event handlers)
		widget *anchorWidget = WIDGET(self->anchorWindow);
		
		if (anchorWidget)
		{
			// Remove any current event handlers we've installed
			if (widgetIsEventHandler(anchorWidget, self->anchorRepositionId))
			{
				widgetRemoveEventHandler(anchorWidget, self->anchorRepositionId);
			}
			if (widgetIsEventHandler(anchorWidget, self->anchorResizeId))
			{
				widgetRemoveEventHandler(anchorWidget, self->anchorResizeId);
			}
		}
		
		// Save the window/alignment/offsets
		self->anchorWindow = self;
		self->anchorHAlign = hAlign;
		self->anchorXOffset = xOffset;
		
		self->anchorVAlign = vAlign;
		self->anchorYOffset = yOffset;
		
		/*
		 * Install the event handlers, which are required to track changes to
		 * the size and position of the anchor window.
		 *
		 * Here we make the assumption that the event handlers when removed,
		 * will be done so in pairs (and therefore only need to install a single
		 * destructor). The same callback is used for both resize and reposition
		 * events.
		 */
		self->anchorRepositionId = widgetAddEventHandler(WIDGET(anchor), EVT_REPOSITION, 
		                                                 windowAnchorCallback, 
		                                                 NULL, self);
		self->anchorResizeId = widgetAddEventHandler(WIDGET(anchor), EVT_RESIZE,
		                                             windowAnchorCallback,
		                                             windowAnchorDestroyCallback,
		                                             self);
	}
	
	// Reposition the window
	windowDoRepositionFromAnchor(self, anchor, hAlign, xOffset, vAlign, yOffset);
}
