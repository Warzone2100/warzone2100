/*
	This file is part of Warzone 2100.
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

#include "spacer.h"

static spacerVtbl vtbl;

const classInfo spacerClassInfo =
{
	&widgetClassInfo,
	"spacer"
};

spacer *spacerCreate(const char *id, spacerDirection direction)
{
	spacer *instance = malloc(sizeof(spacer));
	
	if (instance == NULL)
	{
		return NULL;
	}
	
	// Call the constructor
	spacerInit(instance, id, direction);
	
	// Return the new object
	return instance;
}

static void spacerInitVtbl(spacer *self)
{
	static bool initialised = false;
	
	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.widgetVtbl = *(WIDGET(self)->vtbl);
		
		// Overload widget's destroy, draw and size methods
		vtbl.widgetVtbl.destroy     = spacerDestroyImpl;
		vtbl.widgetVtbl.addChild    = spacerAddChildDummyImpl;
		vtbl.widgetVtbl.doDraw      = spacerDoDrawImpl;
		vtbl.widgetVtbl.doLayout    = spacerDoLayoutImpl;
		vtbl.widgetVtbl.getMinSize  = spacerGetMinSizeImpl;
		vtbl.widgetVtbl.getMaxSize  = spacerGetMaxSizeImpl;
		
		initialised = true;
	}
	
	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;
	
	// Set our vtable
	self->vtbl = &vtbl;
}

void spacerInit(spacer *self, const char *id, spacerDirection direction)
{
	// Init our parent
	widgetInit(WIDGET(self), id);
	
	// Prepare our vtable
	spacerInitVtbl(self);
	
	// Set our type
	WIDGET(self)->classInfo = &spacerClassInfo;
	
	// Set our direction
	self->direction = direction;
}

void spacerDestroyImpl(widget *self)
{
	// Call our parents destructor
	widgetDestroyImpl(self);
}

bool spacerAddChildDummyImpl(widget *self, widget *child)
{
	assert(!"widgetAddChild is not applicable for spacers");
}

size spacerGetMinSizeImpl(widget *self)
{
	(void) self;
	
	// We have no minimum size
	size minSize = { 0, 0 };
	
	return minSize;
}

size spacerGetMaxSizeImpl(widget *self)
{
	size maxSize = { 0, 0 };
	
	switch (SPACER(self)->direction)
	{
		case SPACER_DIRECTION_HORIZONTAL:
			// We have no max horizontal (x-axis) size
			maxSize.x = INT16_MAX;
			break;
		case SPACER_DIRECTION_VERTICAL:
			// We have no max vertical (y-axis) size
			maxSize.y = INT16_MAX;
			break;
	}
	
	return maxSize;
}

bool spacerDoLayoutImpl(widget *self)
{
	// NO-OP
	(void) self;
	
	return true;
}

void spacerDoDrawImpl(widget *self)
{
	// NO-OP
	(void) self;
}
