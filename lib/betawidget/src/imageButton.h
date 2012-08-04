/*
	This file is part of Warzone 2100.
	Copyright (C) 2009  Elio Gubser
	Copyright (C) 2012  Warzone 2100 Project

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

#ifndef IMAGEBUTTON_H_
#define IMAGEBUTTON_H_

#include "button.h"
#include "svgManager.h"

/*
 * Forward declarations
 */
typedef struct _imageButton imageButton;
typedef struct _imageButtonVtbl imageButtonVtbl;

struct _imageButtonVtbl
{
	struct _buttonVtbl buttonVtbl;
	
};

struct _imageButton
{
	/**
	 * Parent
	 */
	struct _button button;
	
	/**
	 * Vtable
	 */
	struct _imageButtonVtbl *vtbl;
	
	/**
	 * An image for each button state
	 */
	svgRenderedImage *images[BUTTON_STATE_COUNT];
	
	/**
	 * The filename for each image. A Filename acts as an Id.
	 */
	const char *imagesFilename[BUTTON_STATE_COUNT];
};

/*
 * Type information
 */
extern const classInfo imageButtonClassInfo;

/*
 * Helper macros
 */
#define IMAGEBUTTON(self) (assert(widgetIsA(WIDGET(self), &imageButtonClassInfo)), \
		              (imageButton *) (self))
#define IMAGEBUTTON_GET_VTBL(self) (IMAGEBUTTON(self)->vtbl)
#define IMAGEBUTTON_CHECK_METHOD(self, method) (assert(IMAGEBUTTON_GET_VTBL(self)->method))

/*
 * Protected methods
 */
imageButton *imageButtonCreate(const char *id, int w, int h);
void imageButtonInit(imageButton *self, const char *id, int w, int h);
void imageButtonDestroyImpl(widget *self);
void imageButtonDoDrawImpl(widget *self);
void imageButtonResizeImpl(widget *self, int w, int h);

/*
 * Public functions
 */
/**
 * Sets an image used for displaying a button state.
 * TODO: Actually only an image for the normal state is necessary.
 * The other button states will be generated and displayed with
 * different brightness. (requires implementation of rsvg)
 * At the moment it just uses the normal state.
 *
 * @param self The image button which is affected
 * @param state The button state for which the image will be displayed
 * @param imageFilename A path to an svg image
 */
void imageButtonSetImage(imageButton *self, buttonState state, const char *imageFilename);

#endif /* IMAGEBUTTON_H */