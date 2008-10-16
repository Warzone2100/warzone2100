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

#include "hBox.h"

static hBoxVtbl vtbl;

const classInfo hBoxClassInfo =
{
	&widgetClassInfo,
	"hBox"
};

hBox *hBoxCreate(const char *id)
{
	hBox *instance = malloc(sizeof(hBox));
	
	if (instance == NULL)
	{
		return NULL;
	}
	
	// Call the constructor
	hBoxInit(instance, id);
	
	// Return the new object
	return instance;
}

static void hBoxInitVtbl(hBox *self)
{
	static bool initialised = false;
	
	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.widgetVtbl = *(WIDGET(self)->vtbl);
		
		// Overload widget's destroy and doLayout methods
		vtbl.widgetVtbl.destroy     = hBoxDestroyImpl;
		vtbl.widgetVtbl.doLayout    = hBoxDoLayoutImpl;
		
		// Along with the min- and max-size methods
		vtbl.widgetVtbl.getMinSize  = hBoxGetMinSizeImpl;
		vtbl.widgetVtbl.getMaxSize  = hBoxGetMaxSizeImpl;
		
		// A dummy draw method also
		vtbl.widgetVtbl.doDraw		= hBoxDoDrawImpl;
		
		initialised = true;
	}
	
	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;
	
	// Set our vtable
	self->vtbl = &vtbl;
}

void hBoxInit(hBox *self, const char *id)
{
	// Init our parent
	widgetInit(WIDGET(self), id);
	
	// Prepare our vtable
	hBoxInitVtbl(self);
	
	// Set our type
	WIDGET(self)->classInfo = &hBoxClassInfo;
	
	// Default vertical alignment is top
	self->vAlignment = TOP;
	
	// Default padding is 0
	self->padding = 0;
}

void hBoxDestroyImpl(widget *self)
{
	// Call our parents destructor
	widgetDestroyImpl(self);
}

bool hBoxDoLayoutImpl(widget *self)
{
	typedef struct
	{
		size minSize;
		size maxSize;
		size currentSize;
		point offset;
	} sizeInfo;
	
	int i, temp;
	const int numChildren = vectorSize(self->children);
	const size minSize = widgetGetMinSize(self);
	sizeInfo *childSizeInfo = alloca(sizeof(sizeInfo) * numChildren);
	
	// First make sure we are large enough to hold all of our children
	if (self->size.x < minSize.x
	 || self->size.y < minSize.y)
	{		
		return false;
	}
	
	// Get the min/max size of our children
	for (i = 0; i < numChildren; i++)
	{
		widget *child = vectorAt(self->children, i);
		
		childSizeInfo[i].currentSize.x = childSizeInfo[i].currentSize.y = 0;
		childSizeInfo[i].minSize = widgetGetMinSize(child);
		childSizeInfo[i].maxSize = widgetGetMaxSize(child);
	}
	
	// Work out how much horizontal space is available for child widgets
	temp = self->size.x - (numChildren - 1) * HBOX(self)->padding;
	
	// Next do y-axis positioning and initial x-axis sizing
	for (i = 0; i < numChildren; i++)
	{
		sizeInfo *child = &childSizeInfo[i];
		
		// Make the child as high as possible
		child->currentSize.y = MIN(self->size.y, child->maxSize.y);
		
		// Make the child as thin as possible, noting how much x-space is left
		child->currentSize.x = child->minSize.x;
		temp -= child->currentSize.x;
		
		// Pad out the remaining y-space
		switch (HBOX(self)->vAlignment)
		{
			case TOP:
				child->offset.y = 0;
				break;
			case MIDDLE:
				child->offset.y = (self->size.y - child->currentSize.y) / 2;
				break;
			case BOTTOM:
				child->offset.y = (self->size.y - child->currentSize.y);
				break;
		}
		
	}
	
	// Gradually increase the x-size of child widgets until all space is used
	for (i = 0; temp; i = (i + 1) % numChildren)
	{
		sizeInfo *child = &childSizeInfo[i];
		
		// If the child is not as wide as possible; widen it
		if (child->currentSize.x < child->maxSize.x)
		{
			child->currentSize.x++;
			temp--;
		}
	}
	
	// All widgets are now as large as possible
	for (i = temp = 0; i < numChildren; i++)
	{
		widget *child = vectorAt(self->children, i);
		sizeInfo *childSize = &childSizeInfo[i];
		
		// Set the size and offset
		widgetResize(child, childSize->currentSize.x,
		             childSize->currentSize.y);
		
		widgetReposition(child, temp, childSize->offset.y);
		
		temp += child->size.x + HBOX(self)->padding;
	}
	
	return true;
}

void hBoxSetVAlign(hBox *self, vAlign v)
{
	// Set the new alignment scheme
	self->vAlignment = v;
	
	// Re-layout outself (should *not* fail)
	if (!widgetDoLayout(WIDGET(self)))
	{
		assert(!"widgetDoLayout failed after changing vertical alignment");
	}
}

bool hBoxSetPadding(hBox *self, int padding)
{
	// Save the current padding
	int oldPadding = self->padding;
	
	// Set the padding
	self->padding = padding;
	
	// Redo the window's layout
	if (widgetDoLayout(widgetGetRoot(WIDGET(self))))
	{
		return true;
	}
	// New padding is untenable
	else
	{
		// Restore the old padding
		self->padding = oldPadding;
		
		// Restore the layout
		widgetDoLayout(widgetGetRoot(WIDGET(self)));
		
		return false;
	}
}

size hBoxGetMinSizeImpl(widget *self)
{
	size minSize = { 0, 0 };
	int i;
	const int numChildren = vectorSize(self->children);
	
	// Sum up the minimum size of our children
	for (i = 0; i < numChildren; i++)
	{
		const size minChildSize = widgetGetMinSize(vectorAt(self->children, i));
		
		minSize.x += minChildSize.x;
		minSize.y = MAX(minSize.y, minChildSize.y);
	}
	
	// Factor in padding between children
	minSize.x += (numChildren - 1) * HBOX(self)->padding;
	
	return minSize;
}

size hBoxGetMaxSizeImpl(widget *self)
{
	size maxSize = { 0, 0 };
	int i;
	const int numChildren = vectorSize(self->children);
	
	// Sum up the maximum size of our children
	for (i = 0; i < numChildren; i++)
	{
		const size maxChildSize = widgetGetMaxSize(vectorAt(self->children, i));
		
		maxSize.x += maxChildSize.x;
		maxSize.y = MAX(maxSize.y, maxChildSize.y);
	}
	
	// Factor in padding between children
	maxSize.x += (numChildren - 1) * HBOX(self)->padding;
	
	return maxSize;
}

void hBoxDoDrawImpl(widget *self)
{
	// NO-OP
	(void) self;
}
