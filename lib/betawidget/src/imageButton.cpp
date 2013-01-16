/*
	This file is part of Warzone 2100.
	Copyright (C) 2009  Elio Gubser
	Copyright (C) 2013  Warzone 2100 Project

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

#include "imageButton.h"
#include <string.h>

static imageButtonVtbl vtbl;

const classInfo imageButtonClassInfo =
{
	&buttonClassInfo,
	"imageButton"
};

static void imageButtonInitVtbl(imageButton *self)
{
	static bool initialised = false;

	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.buttonVtbl = *(BUTTON(self)->vtbl);

		// Overload some widget's methods
		vtbl.buttonVtbl.widgetVtbl.destroy			= imageButtonDestroyImpl;
		vtbl.buttonVtbl.widgetVtbl.doDraw			= imageButtonDoDrawImpl;
		vtbl.buttonVtbl.widgetVtbl.resize			= imageButtonResizeImpl;

		initialised = true;
	}
	
	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.buttonVtbl.widgetVtbl;

	// Set our vtable
	self->vtbl = &vtbl;
}

imageButton *imageButtonCreate(const char *id, int w, int h)
{
	imageButton *instance = malloc(sizeof(imageButton));
	
	if (instance == NULL)
	{
		return NULL;
	}
	
	// Call the constructor
	imageButtonInit(instance, id, w, h);
	
	// Return the new object
	return instance;
}

void imageButtonInit(imageButton *self, const char *id, int w, int h)
{
	int i;
	
	// Set that we don't have any images at the beginning
	for(i = 0;i < BUTTON_STATE_COUNT;i++)
	{
		self->imagesFilename[i] = NULL;
		self->images[i] = NULL;
	}
	
	// Init our parent.
	buttonInit((button *)self, id, w, h);
	
	// Set our type
	((widget *)self)->classInfo = &imageButtonClassInfo;
	
	// Prepare our vtable
	imageButtonInitVtbl(self);
	
}

void imageButtonDestroyImpl(widget *selfWidget)
{
	int i;
	imageButton *self;
	
	self = IMAGEBUTTON(selfWidget);
	
	for(i = 0;i < BUTTON_STATE_COUNT;i++)
	{
		free(self->imagesFilename[i]);
		self->imagesFilename[i] = NULL;
		self->images[i] = NULL;
	}
	
	buttonDestroyImpl(selfWidget);
}

void imageButtonDoDrawImpl(widget *selfWidget)
{
	imageButton *self;
	buttonState stateToDraw;
	cairo_t *cr;
	size imageSize;
	point origin;
	
	// First call our parents drawing function
	buttonDoDrawImpl(selfWidget);
	
	self = IMAGEBUTTON(selfWidget);
	
	stateToDraw = BUTTON(self)->state;
	
	// Check if we have an image for this sate
	if(self->images[stateToDraw] == NULL)
	{
		// Try the normal state image
		if(self->images[BUTTON_STATE_NORMAL] == NULL)
		{
			// Okay, so nothing to do here
			return;
		}
		else
		{
			stateToDraw = BUTTON_STATE_NORMAL;
		}
	}
	
	// Otherwise prepare to draw, save current cairo state
	cr = selfWidget->cr;
	cairo_save(cr);
	
	// Calculate the origin of the image
	imageSize = self->images[stateToDraw]->patternSize;
	origin.x = selfWidget->size.x/2-imageSize.x/2;
	origin.y = selfWidget->size.y/2-imageSize.y/2;
	
	// Translate to the origin
	cairo_translate(cr, origin.x, origin.y);
	
	// Finally blit the image
	svgManagerBlit(cr, self->images[stateToDraw]);
	
	// Restore cairo state
	cairo_restore(cr);
}

/**
 * Renders the desired image. It keeps proportion and tries to
 * fit within widget size.
 *
 * @param self The image button which is affected.
 * @param state The state of the button which image is being rendered
 */
void imageButtonRenderImage(imageButton *self, buttonState state)
{
	assert(state < BUTTON_STATE_COUNT);
	
	if(self->imagesFilename[BUTTON(self)->state] == NULL)
	{
		self->images[BUTTON(self)->state] = NULL;
		return;
	}
	
	// May not be freed as this is managed completely by svgManager.
	self->images[BUTTON(self)->state] = svgManagerGet(self->imagesFilename[BUTTON(self)->state], SVG_SIZE_WIDTH_AND_HEIGHT_FIT, (unsigned int) WIDGET(self)->size.x, (unsigned int) WIDGET(self)->size.y);
}

void imageButtonResizeImpl(widget *self, int w, int h)
{
	int i;
	
	// Rerender each image
	for(i = 0;i < BUTTON_STATE_COUNT;i++)
	{
		imageButtonRenderImage(IMAGEBUTTON(self), i);
	}
	
	widgetResizeImpl(self, w, h);
}

void imageButtonSetImage(imageButton *self, buttonState state, const char *imageFilename)
{
	assert(state < BUTTON_STATE_COUNT);
	
	if(imageFilename != NULL && imageFilename[0] != '\0')
	{
		if(self->imagesFilename[state] != NULL)
		{
			free(self->imagesFilename[state]);
		}
		self->imagesFilename[state] = strdup(imageFilename);
		imageButtonRenderImage(self, state);
	}
	else
	{
		self->imagesFilename[state] = NULL;
		self->images[state] = NULL;
	}
}
