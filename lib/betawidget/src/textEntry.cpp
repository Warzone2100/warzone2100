/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
#include <string.h>

#include "textEntry.h"

static textEntryVtbl vtbl;

// Default size of the text buffer
static const int defautSize = 128;

const classInfo textEntryClassInfo =
{
	&widgetClassInfo,
	"textEntry"
};

textEntry *textEntryCreate(const char *id)
{
	textEntry *instance = malloc(sizeof(textEntry));
	
	if (instance == NULL)
	{
		return NULL;
	}
	
	// Call the constructor
	textEntryInit(instance, id);
	
	// Return the new object
	return instance;
}

static void textEntryInitVtbl(textEntry *self)
{
	static bool initialised = false;
	
	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.widgetVtbl = *(WIDGET(self)->vtbl);
		
		// Overload the destroy method
		vtbl.widgetVtbl.destroy     = textEntryDestroyImpl;
		
		// Draw method
		vtbl.widgetVtbl.doDraw      = textEntryDoDrawImpl;
		
		initialised = true;
	}
	
	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;
	
	// Set our vtable
	self->vtbl = &vtbl;
}

void textEntryInit(textEntry *self, const char *id)
{
	// Init our parent
	widgetInit(WIDGET(self), id);
	
	// Prepare our vtable
	textEntryInitVtbl(self);
	
	// Set our type
	WIDGET(self)->classInfo = &textEntryClassInfo;
	
	// Allocate a modestly sized text buffer
	self->text = calloc(defautSize, 1);
	self->textSize = defautSize;
	
	// On account of there being no text, all of these are 0
	self->textHead = 0;
	self->insertPosition = 0;
	self->renderLeftOffset = 0;
}

void textEntryDestroyImpl(widget *self)
{
	// Free the text buffer
	free(TEXT_ENTRY(self)->text);
	
	// Call our parents destructor
	widgetDestroyImpl(self);
}

void textEntryDoDrawImpl(widget *self)
{
	// Draw our frame
	textEntryDoDrawFrame(TEXT_ENTRY(self));
	textEntryDoDrawText(TEXT_ENTRY(self));
}

void textEntryDoDrawFrame(textEntry *self)
{
	cairo_t *cr = WIDGET(self)->cr;
	
	// Lets keep it simple
	cairo_rectangle(cr, 0, 0, WIDGET(self)->size.x, WIDGET(self)->size.y);
	cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 0.4);
	cairo_fill_preserve(cr);
	
	// Black stroke
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 2);
	cairo_stroke(cr);
}

void textEntryDoDrawText(textEntry *self)
{
	cairo_t *cr = WIDGET(self)->cr;
	
	// Set the render left offset
	cairo_translate(cr, -self->renderLeftOffset, 0);
	
	// Draw the text
	cairo_move_to(cr, 0, WIDGET(self)->size.y);
	
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
	                       CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 25);
	
	cairo_show_text(cr, self->text);
	
	// Restore the offset
	cairo_translate(cr, self->renderLeftOffset, 0);
}

const char *textEntryGetContents(textEntry *self)
{
	return self->text;
}

bool textEntrySetContents(textEntry *self, const char *contents)
{
	// Free the current contents
	free(self->text);
	
	// Copy the new contents
	self->text = strdup(contents);
	
	// TODO: Check for NULL and return false
	
	// Update the size information
	self->textHead = self->textSize = strlen(contents);
	
	// Place the caret at the start of the string
	self->insertPosition = 0;
	
	// Since the insert position is the far left, the render offset is 0
	self->renderLeftOffset = 0;
	
	return true;
}
