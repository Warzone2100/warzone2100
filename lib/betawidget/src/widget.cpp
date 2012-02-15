/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2012  Warzone 2100 Project

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

#include "widget.h"

/**
 * Performs linear interpolation between the points (x0,y0) and (x1,y1),
 * returning the y co-ordinate of the line when the x co-ordinate is x.
 */
#define LINEAR_INTERPOLATE(x0, y0, x1, y1, x) ((y0) + ((x) - (x0)) * \
                                               ((y1) - (y0)) / ((x1) - (x0)))


const classInfo widgetClassInfo =
{
	NULL,		// Root class and therefore no parent
	"widget"
};

static widgetVtbl vtbl;

/**
 * Creates a new cairo context and places it in *cr. The context has a format of
 * format and dimensions of w * h. Should *cr not be NULL it is assumed to be an
 * existing cairo context and so is destroyed.
 *
 * @param cr    The pointer to the region of memory to create the context in.
 * @param format    The format of the context; such as CAIRO_FORMAT_ARGB32.
 * @param w The width of the context in pixels.
 * @param h The height of the context in pixels.
 * @return true if the context was created successfully; false otherwise.
 */
static bool widgetCairoCreate(cairo_t **cr, cairo_format_t format, int w, int h)
{
	cairo_surface_t *surface;

	// See if a context already exists
	if (*cr)
	{
		// Destroy it
		cairo_destroy(*cr);

		// NULL the context
		*cr = NULL;
	}

	// Create the surface
	surface = cairo_image_surface_create(format, w, h);

	// Make sure it was created
	if (!surface)
	{
		return false;
	}

	// Create a context to draw on the surface
	*cr = cairo_create(surface);

	// Enable anti-aliasing to improvement the output quality
	cairo_set_antialias(*cr, CAIRO_ANTIALIAS_GRAY);

	// Destroy the surface (*cr still maintains a reference to it)
	cairo_surface_destroy(surface);

	// Make sure the context was created
	if (!*cr)
	{
		return false;
	}

	return true;
}

/**
 * Checks to see if the widget, self, has the mouse over it. This consists of
 * two checks; firstly seeing if the mouse is inside of the widgets bounding
 * rectangle; secondly, if the widget has a mask, seeing if the location is
 * masked or not.
 *
 * @param self  The widget we want to see if the mouse is over.
 * @param location  The location of the mouse, in absolute terms.
 * @return True if the mouse is over the widget, false otherwise.
 */
static bool widgetHasMouse(widget *self, point location)
{
	const rect bounds = widgetAbsoluteBounds(self);

	// Check to see if it is in the widgets bounding box
	bool hasMouse = pointInRect(location, bounds);

	// If the widget has a mask; ensure the location is not masked
	if (hasMouse && self->maskEnabled)
	{
		// Work out where the mouse is relative to the widget
		const point relativeLocation = pointSub(location, bounds.topLeft);

		// We have the mouse if the point is NOT masked
		hasMouse = !widgetPointMasked(self, relativeLocation);
	}

	return hasMouse;
}

/**
 * Returns a pointer to the event handler with an id of id. Should id be invalid
 * then NULL is returned.
 *
 * @param self  The widget whose event handler table to search.
 * @param id    The id of the desired event handler.
 * @return A pointer to the event handler, or NULL if id is invalid.
 */
static eventTableEntry *widgetGetEventHandlerById(const widget *self, int id)
{
	int i;
	eventTableEntry *entry = NULL;

	// Search the event handler table
	for (i = 0; i < vectorSize(self->eventVtbl); i++)
	{
		eventTableEntry *currEntry = vectorAt(self->eventVtbl, id);

		// See if the id matches
		if (currEntry->id == id)
		{
			entry = currEntry;
			break;
		}
	}

	return entry;
}

bool widgetIsA(const widget *self, const classInfo *instanceOf)
{
	const classInfo *widgetClass;

	// Transverse up the hierarchy
	for (widgetClass = self->classInfo;
	     widgetClass->parentType;
	     widgetClass = widgetClass->parentType)
	{
		// self `is a' instanceOf
		if (widgetClass == instanceOf)
		{
			return true;
		}
	}

	return false;
}

event widgetCreateEvent(eventType type)
{
	// Create the event
	event e;

	// Set the time of the event to now
	e.time = widgetGetTime();

	// Type of the event to the one provided
	e.type = type;

	return e;
}


bool widgetIsEventHandler(const widget *self, int id)
{
	return widgetGetEventHandlerById(self, id) ? true : false;
}

static bool widgetAnimationTimerCallback(widget *self, const event *evt,
                                         int handlerId, void *userData)
{
	int i;
	vector *frames = userData;
	animationFrame *currFrame;

	// Regular timer event
	if (evt->type == EVT_TIMER_PERSISTENT)
	{
		bool didInterpolate = false;

		// Currently active keyframes to be interpolated between
		struct
		{
			animationFrame *pre;
			animationFrame *post;
		} interpolateFrames[ANI_TYPE_COUNT] = { { NULL, NULL } };

		// Work out what frames we need to interpolate between
		for (i = 0; i < vectorSize(frames); i++)
		{
			currFrame = vectorAt(frames, i);

			/*
			 * We are only interested in the key frames which either directly
			 * precede now or come directly after. (As it is these frames which
			 * will be interpolated between.)
			 */
			if (currFrame->time <= evt->time)
			{
				interpolateFrames[currFrame->type].pre = currFrame;
			}
			else if (currFrame->time > evt->time
					 && !interpolateFrames[currFrame->type].post)
			{
				interpolateFrames[currFrame->type].post = currFrame;
			}
		}

		// Do the interpolation
		for (i = 0; i < ANI_TYPE_COUNT; i++)
		{
			// If there are frames to interpolate between then do so
			if (interpolateFrames[i].pre && interpolateFrames[i].post)
			{
				// Get the points to interpolate between
				animationFrame k1 = *interpolateFrames[i].pre;
				animationFrame k2 = *interpolateFrames[i].post;

				int time = evt->time;

				switch (i)
				{
					case ANI_TYPE_TRANSLATE:
					{
						// Get the new position
						point pos = widgetAnimationInterpolateTranslate(self, k1,
						                                                k2, time);

						// Set the new position
						widgetReposition(self, pos.x, pos.y);

						break;
					}
					case ANI_TYPE_ROTATE:
						// Update the widgets rotation
						self->rotate = widgetAnimationInterpolateRotate(self, k1,
						                                                k2, time);
						break;
					case ANI_TYPE_SCALE:
						// Update the widgets scale factor
						self->scale = widgetAnimationInterpolateScale(self, k1,
						                                              k2, time);
						break;
					case ANI_TYPE_ALPHA:
						// Update the widgets alpha
						self->alpha = widgetAnimationInterpolateAlpha(self, k1,
						                                              k2, time);
						break;
				}

				// Make a note that we did interpolate
				didInterpolate = true;
			}
		}

		// If there was no interpolation then the animation is over
		if (!didInterpolate)
		{
			// Remove ourself (the event handler)
			widgetRemoveEventHandler(self, handlerId);
		}
	}
	else if (evt->type == EVT_DESTRUCT)
	{
		animationFrame *lastFrame[ANI_TYPE_COUNT] = { NULL };

		// Find the last frame of each animation type
		for (i = 0; i < vectorSize(frames); i++)
		{
			currFrame = vectorAt(frames, i);

			lastFrame[currFrame->type] = currFrame;
		}

		// Set the position/rotation/scale/alpha to that of the final frame
		for (i = 0; i < ANI_TYPE_COUNT; i++)
		{
			if (lastFrame[i]) switch (i)
			{
				case ANI_TYPE_TRANSLATE:
					self->offset = lastFrame[ANI_TYPE_TRANSLATE]->data.translate;
					break;
				case ANI_TYPE_ROTATE:
					self->rotate = lastFrame[ANI_TYPE_ROTATE]->data.rotate;
					break;
				case ANI_TYPE_SCALE:
					self->scale = lastFrame[ANI_TYPE_SCALE]->data.scale;
					break;
				case ANI_TYPE_ALPHA:
					self->alpha = lastFrame[ANI_TYPE_ALPHA]->data.alpha;
					break;
				default:
					break;
			}
		}

		// Free the frames vector
		vectorMapAndDestroy(frames, free);
	}


	return true;
}

static bool widgetToolTipTimerCallback(widget *self, const event *evt,
                                       int handlerId, void *userData)
{
	// So long as we still have the mouse, show the tool-tip
	if (self->hasMouse)
	{
		widgetShowToolTip(self);
	}

	return true;
}

static bool widgetToolTipMouseEnterCallback(widget *self, const event *evt,
                                            int handlerId, void *userData)
{
	// Only relevant if we have a tool-tip
	if (self->toolTip)
	{
		// Install a single-shot event handler to show the tip
		widgetAddTimerEventHandler(self, EVT_TIMER_SINGLE_SHOT, 2000,
		                           widgetToolTipTimerCallback, NULL, NULL);
	}

	return true;
}

static bool widgetToolTipMouseLeaveCallback(widget *self, const event *evt,
                                            int handlerId, void *userData)
{
	// If we have a tool tip and it is visible; hide it
	if (self->toolTip && self->toolTipVisible)
	{
		widgetHideToolTip(self);
	}

	return true;
}

/**
 * Passes the event, evt, onto each child of self.
 *
 * @param self  The widget to dispatch the event for.
 * @param evt   The event to dispatch.
 * @return True if any child widgets of self handled the event, false if self
 *         either has no children or if none of them handled the event.
 */
static bool widgetDispatchEventToChildren(widget *self, const event *evt)
{
	int i;
	const int numChildren = vectorSize(self->children);
	bool handled = false;

	for (i = 0; i < numChildren; i++)
	{
		// 'We' handled the event if any of our children handled it
		if (widgetHandleEvent(vectorAt(self->children, i), evt))
		{
			handled = true;
		}
	}

	return handled;
}

static void widgetInitVtbl(widget *self)
{
	static bool initialised = false;

	if (!initialised)
	{
		vtbl.addChild               = widgetAddChildImpl;
		vtbl.removeChild            = widgetRemoveChildImpl;

		vtbl.fireCallbacks          = widgetFireCallbacksImpl;
		vtbl.fireTimerCallbacks     = widgetFireTimerCallbacksImpl;

		vtbl.addEventHandler        = widgetAddEventHandlerImpl;
		vtbl.addTimerEventHandler   = widgetAddTimerEventHandlerImpl;
		vtbl.removeEventHandler     = widgetRemoveEventHandlerImpl;
		vtbl.handleEvent            = widgetHandleEventImpl;

		vtbl.animationInterpolateTranslate  = widgetAnimationInterpolateTranslateImpl;
		vtbl.animationInterpolateRotate     = widgetAnimationInterpolateRotateImpl;
		vtbl.animationInterpolateScale      = widgetAnimationInterpolateScaleImpl;
		vtbl.animationInterpolateAlpha      = widgetAnimationInterpolateAlphaImpl;

		vtbl.focus                  = widgetFocusImpl;
		vtbl.blur                   = widgetBlurImpl;

		vtbl.acceptDrag             = widgetAcceptDragImpl;
		vtbl.declineDrag            = widgetDeclineDragImpl;

		vtbl.enable                 = widgetEnableImpl;
		vtbl.disable                = widgetDisableImpl;

		vtbl.show                   = widgetShowImpl;
		vtbl.hide                   = widgetHideImpl;

		vtbl.getMinSize             = NULL;
		vtbl.getMaxSize             = NULL;

		vtbl.resize                 = widgetResizeImpl;
		vtbl.reposition             = widgetRepositionImpl;

		vtbl.enableGL               = widgetEnableGLImpl;
		vtbl.disableGL              = widgetDisableGLImpl;

		vtbl.beginGL                = widgetBeginGLImpl;
		vtbl.endGL                  = widgetEndGLImpl;

		vtbl.composite              = widgetCompositeImpl;

		vtbl.doLayout               = NULL;
		vtbl.doDraw                 = NULL;
		vtbl.doDrawMask             = NULL;

		vtbl.destroy                = widgetDestroyImpl;

		initialised = true;
	}

	// Set the classes vtable
	self->vtbl = &vtbl;
}

void widgetInit(widget *self, const char *id)
{
	// Prepare our vtable
	widgetInitVtbl(self);

	// Set our type
	self->classInfo = &widgetClassInfo;

	// Prepare our container
	self->children = vectorCreate();

	// Prepare our events table
	self->eventVtbl = vectorCreate();

	// Copy the ID of the widget
	self->id = strdup(id);

	// Default parent is none
	self->parent = NULL;

	// Default size is (-1,-1) 'NULL' size
	self->size.x = -1.0f;
	self->size.y = -1.0f;

	// We are not rotated by default
	self->rotate = 0.0f;

	// Scale is (1,1)
	self->scale.x = 1.0f;
	self->scale.y = 1.0f;

	// Alpha is 1 (no change)
	self->alpha = 1.0f;

	// Zero the user data
	self->userData = 0;
	self->pUserData = NULL;

	// Create a dummy cairo context (getMin/MaxSize may depend on it)
	self->cr = NULL;
	widgetCairoCreate(&self->cr, CAIRO_FORMAT_ARGB32, 0, 0);

	// Ask OpenGL for a texture id
	glGenTextures(1, &self->textureId);

	// No tool-tip by default
	self->toolTip = NULL;
	self->toolTipVisible = false;

	// Install required event handlers for showing/hiding tool-tips
	widgetAddEventHandler(self, EVT_MOUSE_ENTER,
	                      widgetToolTipMouseEnterCallback, NULL, NULL);
	widgetAddEventHandler(self, EVT_MOUSE_LEAVE,
	                      widgetToolTipMouseLeaveCallback, NULL, NULL);

	// Focus and mouse are false by default
	self->hasFocus = false;
	self->hasMouse = false;
	self->hasMouseDown = false;

	// By default we need drawing
	self->needsRedraw = true;

	// Enabled by default
	self->isEnabled = true;

	// Also by default we need to be shown (hence are invisible)
	self->isVisible = false;

	// By default the mouse-event mask is disabled
	self->maskCr = NULL;
	self->maskEnabled = false;
}

void widgetDestroyImpl(widget *self)
{
	eventTableEntry *currEntry;

	// Release the container
	vectorMapAndDestroy(self->children, (mapCallback) widgetDestroy);

	// Release the event handler table
	while ((currEntry = vectorHead(self->eventVtbl)))
	{
		widgetRemoveEventHandler(self, currEntry->id);
	}

	vectorDestroy(self->eventVtbl);

	// Destroy the cairo context
	cairo_destroy(self->cr);

	// Destroy the texture
	glDeleteTextures(1, &self->textureId);

	// If we use a mask, destroy it
	if (self->maskEnabled)
	{
		cairo_destroy(self->maskCr);
	}

	// If GL content support is enabled, disable it
	if (self->openGLEnabled)
	{
		widgetDisableGL(self);
	}

	// Free the ID
	free((char *) self->id);

	// Free the tool-tip (if any)
	free((char *) self->toolTip);

	// Free ourself
	free(self);
}

void widgetDraw(widget *self)
{
	int i;

	// See if we need to be redrawn
	if (self->needsRedraw)
	{
		void *bits = cairo_image_surface_get_data(cairo_get_target(self->cr));

		self->needsRedraw = false;

		// Mark the texture as needing uploading
		self->textureNeedsUploading = true;

		// Clear the current context
		cairo_set_operator(self->cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(self->cr, 0.0, 0.0, 0.0, 0.0);
		cairo_paint(self->cr);

		// Restore the compositing operator back to the default
		cairo_set_operator(self->cr, CAIRO_OPERATOR_OVER);

		// Save (push) the current context
		cairo_save(self->cr);

		// Redaw ourself
		widgetDoDraw(self);

		// Restore the context
		cairo_restore(self->cr);

		// Update the texture if widgetEndGL has not done it for us
		if (self->textureNeedsUploading)
		{
			// Flush the cairo surface to ensure that all drawing is completed
			cairo_surface_flush(cairo_get_target(self->cr));

			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, self->textureId);

			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, self->size.x,
			             self->size.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, bits);

			self->textureNeedsUploading = false;
		}

	}

	// Draw our children (even if we did not need redrawing our children might)
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widget *child = vectorAt(self->children, i);

		// Ask the child to re-draw itself
		widgetDraw(child);
	}
}

void widgetDrawMask(widget *self)
{
	// Make sure masking is enabled and that the context is valid
	assert(self->maskEnabled);
	assert(self->maskCr);

	// Make the context opaque (so it is all masked)
	cairo_set_source_rgba(self->maskCr, 0.0, 0.0, 0.0, 1.0);
	cairo_paint(self->maskCr);

	// Change the blending mode so that the source overwrites the destination
	cairo_set_operator(self->maskCr, CAIRO_OPERATOR_SOURCE);

	// Change the source to transparency
	cairo_set_source_rgba(self->maskCr, 0.0, 0.0, 0.0, 0.0);

	// Draw the mask
	widgetDoDrawMask(self);
}

point widgetAbsolutePosition(const widget *self)
{
	// Get our own offset
	point pos = self->offset;

	// Add to this our parents offset
	if (self->parent != NULL)
	{
		pos = pointAdd(pos, widgetAbsolutePosition(self->parent));
	}

	return pos;
}

rect widgetAbsoluteBounds(const widget *self)
{
	// Get our position
	const point p = widgetAbsolutePosition(self);

	// Construct and return a rect from this
	return rectFromPointAndSize(p, self->size);
}

widget *widgetGetRoot(widget *self)
{
	// If we are the root widget, return early
	if (self->parent == NULL)
	{
		return self;
	}
	// Otherwise search the hierarchy
	else
	{
		widget *current;

		for (current = self->parent; current->parent; current = current->parent);

		return current;
	}
}

widget *widgetFindById(widget *self, const char *id)
{
	// See if we have that ID
	if (strcmp(self->id, id) == 0)
	{
		return self;
	}
	// Try our children
	else
	{
		int i;

		for (i = 0; i < vectorSize(self->children); i++)
		{
			// Get the child widget
			widget *child = vectorAt(self->children, i);

			// Call its findById method
			widget *match = widgetFindById(child, id);

			// If it matched, return
			if (match)
			{
				return match;
			}
		}
	}

	// If we found nothing return NULL
	return NULL;
}

void *widgetGetEventHandlerUserData(const widget *self, int id)
{
	eventTableEntry *entry = widgetGetEventHandlerById(self, id);

	// Make sure we found something
	assert(entry != NULL);

	// Return the user-data
	return entry->userData;
}

void widgetSetEventHandlerUserData(widget *self, int id, void *userData)
{
	eventTableEntry *entry = widgetGetEventHandlerById(self, id);

	// Make sure we found something
	assert(entry != NULL);

	// Set the user-data
	entry->userData = userData;
}

bool widgetAddChildImpl(widget *self, widget *child)
{
	// Make sure the id of the child is unquie
	assert(widgetFindById(widgetGetRoot(self), child->id) == NULL);

	// Make sure the child does not currently have a parent
	assert(child->parent == NULL);

	// Add the widget
	vectorAdd(self->children, child);

	// So long as our size is not (-1,-1) (NULL-size), re-layout the window
	if ((self->size.x == -1.0f && self->size.y == -1.0f)
	 || widgetDoLayout(widgetGetRoot(self)))
	{
		// Set ourself as its parent
		child->parent = self;

		return true;
	}
	// Not enough space to fit the widget (widgetDoLayout returned false)
	else
	{
		// Remove child *without* calling its destructor
		vectorRemoveAt(self->children, vectorSize(self->children) - 1);

		// Restore the layout
		widgetDoLayout(widgetGetRoot(self));

		return false;
	}
}

void widgetRemoveChildImpl(widget *self, widget *child)
{
	int i;

	for (i = 0; i < vectorSize(self->children); i++)
	{
		// If the child is the to-be-removed widget, remove it
		if (vectorAt(self->children, i) == child)
		{
			// Call the destructor for the widget
			widgetDestroy(vectorAt(self->children, i));

			// Remove it from the list of children
			vectorRemoveAt(self->children, i);

			// Re-layout the window (so long as we are part of one)
			if (self->size.x != -1.0f && self->size.y != -1.0f)
			{
				widgetDoLayout(widgetGetRoot(self));
			}
		}
		// See if it is one of its children
		else if (child->parent != self)
		{
			widgetRemoveChild(vectorAt(self->children, i), child);
		}
	}
}

int widgetAddEventHandlerImpl(widget *self, eventType type, callback handler,
                              callback destructor, void *userData)
{
	eventTableEntry *entry = malloc(sizeof(eventTableEntry));
	eventTableEntry *lastEntry = vectorHead(self->eventVtbl);

	// Timer events should use addTimerEventHandler
	assert(type != EVT_TIMER_SINGLE_SHOT && type != EVT_TIMER_PERSISTENT);

	// Assign the handler an id which is one higher than the current highest
	entry->id           = (lastEntry) ? lastEntry->id + 1 : 1;
	entry->type         = type;
	entry->callback     = handler;
	entry->destructor   = destructor;
	entry->userData     = userData;

	entry->lastCalled   = 0;    // We have never been called
	entry->interval     = -1;   // We are not a timer event

	// Add the handler to the table
	vectorAdd(self->eventVtbl, entry);

	// Return the id of the handler
	return entry->id;
}

int widgetAddTimerEventHandlerImpl(widget *self, eventType type, int interval,
                                   callback handler, callback destructor,
                                   void *userData)
{
	eventTableEntry *entry = malloc(sizeof(eventTableEntry));
	eventTableEntry *lastEntry = vectorHead(self->eventVtbl);

	// We should only be used to add timer events
	assert(type == EVT_TIMER_SINGLE_SHOT || type == EVT_TIMER_PERSISTENT);

	entry->id           = (lastEntry) ? lastEntry->id + 1 : 1;
	entry->type         = type;
	entry->callback     = handler;
	entry->destructor   = destructor;
	entry->userData     = userData;

	// the handler is called when lastCalled + interval >= current time
	entry->lastCalled   = widgetGetTime();
	entry->interval     = interval;

	// Add the handler to the table
	vectorAdd(self->eventVtbl, entry);

	// Return the id of the handler
	return entry->id;
}

void widgetRemoveEventHandlerImpl(widget *self, int id)
{
	int i;

	// Search for the handler with the id
	for (i = 0; i < vectorSize(self->eventVtbl); i++)
	{
		eventTableEntry *handler = vectorAt(self->eventVtbl, i);

		// If the handler matches, remove it
		if (handler->id == id)
		{
			// If there is a destructor; call it
			if (handler->destructor)
			{
				// Generate an EVT_DESTRUCT event
				eventMisc evtDestruct;
				evtDestruct.event = widgetCreateEvent(EVT_DESTRUCT);

				handler->destructor(self, (event *) &evtDestruct, handler->id,
				                    handler->userData);
			}

			// Release the handler
			free(handler);

			// Finally, remove the event handler from the table
			vectorRemoveAt(self->eventVtbl, i);
			break;
		}
	}
}

int widgetAddAnimation(widget *self, int nframes,
                       const animationFrame *frames)
{
	int i;
	int lastTime = 0;
	int translateCount = 0, rotateCount = 0, scaleCount = 0, alphaCount = 0;
	vector *ourFrames;
	animationFrame *currFrame;

	/*
	 * We need to make sure that the frames are in order and that there is
	 * enough information to do the required interpolation.
	 */
	for (i = 0; i < nframes; i++)
	{
		// Make sure the frame N+1 follows after N
		assert(frames[i].time >= lastTime);

		// Update the time
		lastTime = frames[i].time;

		// Update the animation frame count
		switch (frames[i].type)
		{
			case ANI_TYPE_TRANSLATE:
				translateCount++;
				break;
			case ANI_TYPE_ROTATE:
				rotateCount++;
				break;
			case ANI_TYPE_SCALE:
				scaleCount++;
				break;
			case ANI_TYPE_ALPHA:
				alphaCount++;
				break;
			default:
				assert(!"Invalid animation type");
				break;
		}
	}

	// Ensure the frame counts are valid
	assert(translateCount != 1);
	assert(rotateCount != 1);
	assert(scaleCount != 1);
	assert(alphaCount != 1);

	// Copy the frames over to our own vector
	ourFrames = vectorCreate();

	for (i = 0; i < nframes; i++)
	{
		// Allocate space for the frame
		currFrame = malloc(sizeof(animationFrame));

		// Copy the frame
		*currFrame = frames[i];

		// De-normalise the frames time (so that it is absolute)
		currFrame->time += widgetGetTime();

		// Add the frame to the vector
		vectorAdd(ourFrames, currFrame);
	}

	// Install the animation timer event handler
	return widgetAddTimerEventHandler(self, EVT_TIMER_PERSISTENT, 10,
	                                  widgetAnimationTimerCallback,
	                                  widgetAnimationTimerCallback, ourFrames);
}

point widgetAnimationInterpolateTranslateImpl(widget *self, animationFrame k1,
                                              animationFrame k2, int time)
{
	point newOffset;

	// Use linear interpolation to get the new (x,y) coords
	newOffset.x = LINEAR_INTERPOLATE(k1.time, k1.data.translate.x,
	                                 k2.time, k2.data.translate.x, time);
	newOffset.y = LINEAR_INTERPOLATE(k1.time, k1.data.translate.y,
                                     k2.time, k2.data.translate.y, time);

	return newOffset;
}

float widgetAnimationInterpolateRotateImpl(widget *self, animationFrame k1,
                                           animationFrame k2, int time)
{
	return LINEAR_INTERPOLATE(k1.time, k1.data.rotate,
	                          k2.time, k2.data.rotate, time);
}

point widgetAnimationInterpolateScaleImpl(widget *self, animationFrame k1,
                                          animationFrame k2, int time)
{
	point newScale;

	// Like with the translate method use linear interpolation
	newScale.x = LINEAR_INTERPOLATE(k1.time, k1.data.scale.x,
	                                k2.time, k2.data.scale.x, time);
	newScale.y = LINEAR_INTERPOLATE(k1.time, k1.data.scale.y,
	                                k2.time, k2.data.scale.y, time);

	return newScale;
}

float widgetAnimationInterpolateAlphaImpl(widget *self, animationFrame k1,
                                          animationFrame k2, int time)
{
	return LINEAR_INTERPOLATE(k1.time, k1.data.alpha,
	                          k2.time, k2.data.alpha, time);
}

bool widgetFireCallbacksImpl(widget *self, const event *evt)
{
	int i;
	bool ret;

	for (i = 0; i < vectorSize(self->eventVtbl); i++)
	{
		eventTableEntry *handler = vectorAt(self->eventVtbl, i);

		// If handler is registered to handle evt
		if (handler->type == evt->type)
		{
			// Fire the callback
			ret = handler->callback(self, evt, handler->id, handler->userData);

			// Update the last called time
			handler->lastCalled = evt->time;

			// Break if the handler returned false
			if (!ret)
			{
				break;
			}
		}
	}

	// FIXME
	return true;
}

bool widgetFireTimerCallbacksImpl(widget *self, const event *evt)
{
	int i;
	bool ret;
	eventTimer evtTimer = *((eventTimer *) evt);

	// We should only be passed EVT_TIMER events
	assert(evt->type == EVT_TIMER);

	for (i = 0; i < vectorSize(self->eventVtbl); i++)
	{
		eventTableEntry *handler = vectorAt(self->eventVtbl, i);

		// See if the handler is registered to handle timer events
		if (handler->type == EVT_TIMER_SINGLE_SHOT
		 || handler->type == EVT_TIMER_PERSISTENT)
		{
			// See if the event needs to be fired
			if (evt->time >= (handler->lastCalled + handler->interval))
			{
				// Ensure the type of our custom event matches
				evtTimer.event.type = handler->type;

				// Fire the associated callback
				ret = handler->callback(self, (event *) &evtTimer, handler->id,
				                        handler->userData);

				// Update the last called time
				handler->lastCalled = evt->time;

				// If the event is single shot then remove it
				if (handler->type == EVT_TIMER_SINGLE_SHOT)
				{
					widgetRemoveEventHandler(self, handler->id);
				}
			}
		}
	}

	// FIXME
	return true;
}

void widgetAcceptDragImpl(widget *self)
{
	// Make sure there is an active drag offer
	assert(self->dragState == DRAG_PENDING);

	// Accept the offer
	self->dragState = DRAG_ACCEPTED;
}

void widgetDeclineDragImpl(widget *self)
{
	// Make sure there is an active drag offer
	assert(self->dragState == DRAG_PENDING);

	// Decline the offer
	self->dragState = DRAG_DECLINED;
}

void widgetEnableImpl(widget *self)
{
	int i;
	eventMisc evt;

	// First make sure our parent is enabled
	if (self->parent && !self->parent->isEnabled)
	{
		return;
	}

	// Enable ourself
	self->isEnabled = true;

	// Fire any on-enable callbacks
	evt.event = widgetCreateEvent(EVT_ENABLE);
	widgetFireCallbacks(self, (event *) &evt);

	// Enable all of our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widgetEnable(vectorAt(self->children, i));
	}
}

void widgetDisableImpl(widget *self)
{
	int i;
	eventMisc evt;

	// If we are currently disabled, return
	if (!self->isEnabled)
	{
		return;
	}

	// Disable ourself
	self->isEnabled = false;

	// Fire any on-disable callbacks
	evt.event = widgetCreateEvent(EVT_DISABLE);
	widgetFireCallbacks(self, (event *) &evt);

	// Disable our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widgetDisable(vectorAt(self->children, i));
	}

}

void widgetShowImpl(widget *self)
{
	// Make ourself visible
	self->isVisible = true;

	// We need redrawing
	self->needsRedraw = true;
}

void widgetHideImpl(widget *self)
{
	// Make ourself invisible
	self->isVisible = false;

	// NB: No effect on redrawing
}

void widgetFocusImpl(widget *self)
{
	eventMisc evt;

	// Check that we are not currently focused
	if (self->hasFocus)
	{
		int i;

		// Blur any of our currently focused child widgets
		for (i = 0; i < vectorSize(self->children); i++)
		{
			widget *child = vectorAt(self->children, i);

			if (child->hasFocus)
			{
				widgetBlur(child);
			}
		}

		return;
	}

	// If we have a parent, focus it
	if (self->parent)
	{
		widgetFocus(self->parent);
	}

	// Focus ourself
	self->hasFocus = true;

	// Fire our on-focus callbacks
	evt.event = widgetCreateEvent(EVT_FOCUS);
	widgetFireCallbacks(self, (event *) &evt);
}

void widgetBlurImpl(widget *self)
{
	widget *current;
	eventMisc evt;

	// Make sure we have focus
	if (!self->hasFocus)
	{
		// TODO: We should log this eventuality
		return;
	}

	// First blur any focused child widgets
	while ((current = widgetGetCurrentlyFocused(self)) != self)
	{
		widgetBlur(current);
	}

	// Blur ourself
	self->hasFocus = false;

	// Fire off the on-blur callbacks
	evt.event = widgetCreateEvent(EVT_BLUR);
	widgetFireCallbacks(self, (event *) &evt);
}

void widgetShowToolTip(widget *self)
{
	// Create the event
	eventToolTip evt;
	evt.event = widgetCreateEvent(EVT_TOOL_TIP_SHOW);
	evt.target = self;

	// Dispatch the event to the root widget
	widgetHandleEvent(widgetGetRoot(self), (event *) &evt);

	// Note that the tool-tip is visible
	self->toolTipVisible = true;
}

void widgetHideToolTip(widget *self)
{
	// Create the event
	eventToolTip evt;
	evt.event = widgetCreateEvent(EVT_TOOL_TIP_HIDE);
	evt.target = self;

	// Dispatch the event to the root widget
	widgetHandleEvent(widgetGetRoot(self), (event *) &evt);

	// Note that the tool-tip is now hidden
	self->toolTipVisible = false;
}

void widgetResizeImpl(widget *self, int w, int h)
{
	const size minSize = widgetGetMinSize(self);
	const size maxSize = widgetGetMaxSize(self);

	// Create an event
	eventResize evtResize;
	evtResize.event = widgetCreateEvent(EVT_RESIZE);

	// Save the current size in the event
	evtResize.oldSize = self->size;

	assert(minSize.x <= w);
	assert(minSize.y <= h);
	assert(w <= maxSize.x);
	assert(h <= maxSize.y);

	self->size.x = w;
	self->size.y = h;

	// Re-create the cairo context at this new size
	widgetCairoCreate(&self->cr, CAIRO_FORMAT_ARGB32, w, h);

	// If a mask is enabled; re-create it also
	if (self->maskEnabled)
	{
		widgetCairoCreate(&self->maskCr, CAIRO_FORMAT_A1, w, h);

		// Re-draw the mask (only done on resize)
		widgetDrawMask(self);
	}

	// If OpenGL is enabled disable and re-enable it
	if (self->openGLEnabled)
	{
		widgetDisableGL(self);
		widgetEnableGL(self);
	}

	// Set the needs redraw flag
	self->needsRedraw = true;

	// If we have any children, we need to redo their layout
	if (vectorSize(self->children))
	{
		widgetDoLayout(self);
	}

	// Fire any EVT_RESIZE callbacks
	widgetFireCallbacks(self, (event *) &evtResize);
}

void widgetRepositionImpl(widget *self, int x, int y)
{
	// Generate a reposition event
	eventReposition evtReposition;
	evtReposition.event = widgetCreateEvent(EVT_REPOSITION);

	// Save the current position in the event
	evtReposition.oldPosition = self->offset;
	evtReposition.oldAbsolutePosition = widgetAbsolutePosition(self);

	// Update our position
	self->offset.x = x;
	self->offset.y = y;

	// Fire any callbacks for EVT_REPOSITION
	widgetFireCallbacks(self, (event *) &evtReposition);
}

void widgetEnableGLImpl(widget *self)
{
	GLenum status;

	// Check if GL is already enabled
	if (self->openGLEnabled)
	{
		return;
	}

	// Generate an FBO id
	glGenFramebuffersEXT(1, &self->fboId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, self->fboId);

	// Generate a renderbuffer to server as a depth buffer
	glGenRenderbuffersEXT(1, &self->depthbufferId);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, self->depthbufferId);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,
	                         self->size.x, self->size.y);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

	// Bind our drawing texture and initialise it
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, self->textureId);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, self->size.x,
	             self->size.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	// Attach the texture to the FBO
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0,
	                          GL_TEXTURE_RECTANGLE_ARB, self->textureId, 0);

	// Attach the depthbuffer to the FBO
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT,
	                             GL_RENDERBUFFER_EXT, self->depthbufferId);

	// Ensure that the FBO was created successfully
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

	// Un-bind the FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// Rendering GL content to the widget is now enabled
	self->openGLEnabled = true;
}

void widgetDisableGLImpl(widget *self)
{
	// Check if GL is already disabled
	if (!self->openGLEnabled)
	{
		return;
	}

	// Delete the FBO
	glDeleteFramebuffersEXT(1, &self->fboId);

	// Delete the FBOs depthbuffer (which is a renderbuffer)
	glDeleteRenderbuffersEXT(1, &self->depthbufferId);

	// Rendering GL content to the widget is now disabled
	self->openGLEnabled = false;
}

void widgetBeginGLImpl(widget *self)
{
	void *bits = cairo_image_surface_get_data(cairo_get_target(self->cr));

	// Ensure that drawing GL content is enabled
	assert(self->openGLEnabled);

	// Ensure that we are not in a begin block already
	assert(!self->openGLInProgress);

	// Flush the cairo surface to ensure that all drawing is completed
	cairo_surface_flush(cairo_get_target(self->cr));

	// GL rendering is now in progress for the widget
	self->openGLInProgress = true;

	// Copy the current drawing to the texture
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, self->textureId);

	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, self->size.x,
	             self->size.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, bits);

	// Un-bind the texture, this is crucial for the FBO rendering to work!
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	// Bind the FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, self->fboId);

	// Save the current GL states
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	// Reset the projection matrix (making sure to save it first)
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// Update the viewport so that it is the size of the texture
	glViewport(0, 0, self->size.x, self->size.y);
}

void widgetEndGLImpl(widget *self, bool finishedDrawing)
{
	void *bits = cairo_image_surface_get_data(cairo_get_target(self->cr));

	// Make sure we are actually in an OpenGL block
	assert(self->openGLInProgress);

	// Restore the GL states and un-bin the FBO
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// If drawing is not finished copy the texture back to the cairo context
	if (!finishedDrawing)
	{
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, self->textureId);
		glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_BGRA, GL_UNSIGNED_BYTE,
		              bits);

		// Tell cairo to re-read any cached areas
		cairo_surface_mark_dirty(cairo_get_target(self->cr));
	}
	// Else drawing is finished so no need for composite to upload the texture
	else
	{
		self->textureNeedsUploading = false;
	}

	// GL rendering has now finished
	self->openGLInProgress = false;
}

void widgetCompositeImpl(widget *self)
{
	float blendColour[4];
	int i;

	// Do not composite unless we are visible
	if (!self->isVisible)
	{
		return;
	}

	// Save the current model-view matrix
	glPushMatrix();

	// Translate such that (0,0) is the top-left of ourself
	glTranslatef(self->offset.x, self->offset.y, 0.0f);

	// Scale if necessary
	glScalef(self->scale.x,  self->scale.y,  1.0f);

	// Rotate ourself
	glRotatef(self->rotate, 0.0f, 0.0f, 1.0f);

	// Set our alpha (blend colour)
	glGetFloatv(GL_BLEND_COLOR, blendColour);
	glBlendColor(1.0f, 1.0f, 1.0f, blendColour[3] * self->alpha);

	// Composite ourself
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, self->textureId);

	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, self->size.y);
		glVertex2f(0.0f, self->size.y);

		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0f, 0.0f);

		glTexCoord2f(self->size.x, self->size.y);
		glVertex2f(self->size.x, self->size.y);

		glTexCoord2f(self->size.x, 0.0f);
		glVertex2f(self->size.x, 0.0f);
	glEnd();

	// Now our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widget *child = vectorAt(self->children, i);

		// Composite
		widgetComposite(child);
	}

	// Restore the model-view matrix
	glPopMatrix();

	// Restore the blend colour
	glBlendColor(1.0f, 1.0f, 1.0f, blendColour[3]);
}

void widgetEnableMask(widget *self)
{
	// Check if the mask is already enabled (worth asserting)
	if (self->maskEnabled)
	{
		return;
	}

	// Make sure we implement the doDrawMask method
	WIDGET_CHECK_METHOD(self, doDrawMask);

	// Prepare the cairo surface
	widgetCairoCreate(&self->maskCr, CAIRO_FORMAT_A1,
	                  self->size.x, self->size.y);

	// Note that the mask is now enabled
	self->maskEnabled = true;

	// Draw the mask
	widgetDrawMask(self);
}

void widgetDisableMask(widget *self)
{
	// NO-OP if the mask is not enabled
	if (!self->maskEnabled)
	{
		return;
	}

	// Release the cairo context
	cairo_destroy(self->maskCr);

	// Note that the mask is now disabled
	self->maskEnabled = false;
}

widget *widgetGetCurrentlyFocused(widget *self)
{
	int i;

	if (!self->hasFocus)
	{
		return NULL;
	}

	for (i = 0; i < vectorSize(self->children); i++)
	{
		widget *child = vectorAt(self->children, i);

		if (child->hasFocus)
		{
			return widgetGetCurrentlyFocused(child);
		}
	}

	// None of our children are focused, return ourself
	return self;
}

widget *widgetGetCurrentlyMousedOver(widget *self)
{
	int i;

	// Make sure we have the mouse
	if (!self->hasMouse)
	{
		return NULL;
	}

	// See if any of our children are moused over
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widget *child = vectorAt(self->children, i);

		if (child->hasMouse)
		{
			return widgetGetCurrentlyMousedOver(child);
		}
	}

	// None of our children have the mouse; return ourself
	return self;
}

bool widgetHandleEventImpl(widget *self, const event *evt)
{
	// If any callbacks were fired as a direct result of the event
	bool handled = false;

	// If we are disabled or invisible then only timer events are relevant
	if ((!self->isEnabled || !self->isVisible)
	 && evt->type != EVT_TIMER)
	{
		return true;
	}

	switch (evt->type)
	{
		case EVT_MOUSE_MOVE:
		{
			eventMouse evtMouse = *((eventMouse *) evt);
			bool newHasMouse = widgetHasMouse(self, evtMouse.loc);

			/*
			 * Mouse motion events should not be dispatched if a mouse button
			 * is currently `down' on our parent but not ourself.
			 */
			if (self->parent
			 && self->parent->hasMouseDown
			 && !self->hasMouseDown)
			{
				break;
			}

			// While dragging hasMouse irrelevant
			if (self->dragState == DRAG_ACTIVE)
			{
				// Morph the event into a DRAG_TRACK event
				evtMouse.event.type = EVT_DRAG_TRACK;

				widgetFireCallbacks(self, (event *) &evtMouse);

				// Event was handled
				handled = true;

				break;
			}

			// If we have or did have the mouse
			if (newHasMouse || self->hasMouse)
			{
				// Pass the event onto our children first
				widgetDispatchEventToChildren(self, evt);

				// If we have just `got' the mouse
				if (newHasMouse && !self->hasMouse)
				{
					// Generate an EVT_MOUSE_ENTER event instead
					evtMouse.event.type = EVT_MOUSE_ENTER;
				}
				// If we have just lost the mouse
				else if (!newHasMouse && self->hasMouse)
				{
					// Generate an EVT_MOUSE_LEAVE event
					evtMouse.event.type = EVT_MOUSE_LEAVE;
				}

				// Fire any registered callbacks for evtMouse.event.type
				widgetFireCallbacks(self, (event *) &evtMouse);

				// The event was handled
				handled = true;
			}

			// If the mouse is down offer a drag event
			if (newHasMouse && self->hasMouseDown)
			{
				// We are awaiting a response to our drag offer
				self->dragState = DRAG_PENDING;

				// Morph the event
				evtMouse.event.type = EVT_DRAG_BEGIN;

				// Fire any EVT_DRAG_BEGIN callbacks
				widgetFireCallbacks(self, (event *) &evtMouse);

				// Check to see if the drag was accepted or not
				if (self->dragState == DRAG_ACCEPTED)
				{
					self->dragState = DRAG_ACTIVE;
				}
				// Drag was declined or not handled (no BEGIN callback)
				else
				{
					self->dragState = DRAG_NONE;
				}
			}

			// Update the status of the mouse
			self->hasMouse = newHasMouse;
			break;
		}
		case EVT_MOUSE_DOWN:
		case EVT_MOUSE_UP:
		{
			eventMouseBtn evtMouseBtn = *((eventMouseBtn *) evt);

			// On mouse-up cancel any active drag events
			if (evt->type == EVT_MOUSE_UP && self->dragState == DRAG_ACTIVE)
			{
				// Morph the event
				evtMouseBtn.event.type = EVT_DRAG_END;

				// Fire the callbacks
				widgetFireCallbacks(self, (event *) &evtMouseBtn);

				// Event was handled
				handled = true;

				// No drag is active
				self->dragState = DRAG_NONE;

				// Mouse is no longer down
				self->hasMouseDown = false;

				break;
			}

			// If the mouse is inside of the widget
			if (widgetHasMouse(self, evtMouseBtn.loc))
			{
				// Pass the event onto our children first
				bool childHandled = widgetDispatchEventToChildren(self, evt);

				// If it is a mouse-down event set hasMouseDown to true
				if (evt->type == EVT_MOUSE_DOWN)
				{
					self->hasMouseDown = true;
				}

				// Only fire our callbacks if none of our children handled it
				if (!childHandled)
				{
					widgetFireCallbacks(self, (event *) &evtMouseBtn);

					// Check for a click event
					if (evt->type == EVT_MOUSE_UP && self->hasMouseDown)
					{
						// Morph the event into a mouse click
						evtMouseBtn.event.type = EVT_MOUSE_CLICK;

						widgetFireCallbacks(self, (event *) &evtMouseBtn);
					}
				}

				// Event was handled
				handled = true;
			}
			// If the mouse is no longer down
			else if (evt->type == EVT_MOUSE_UP && self->hasMouseDown)
			{
				// Pass the event onto our children
				widgetDispatchEventToChildren(self, evt);

				self->hasMouseDown = false;
			}
			break;
		}
		case EVT_KEY_DOWN:
		case EVT_KEY_UP:
		case EVT_TEXT:
		{
			// Only relevant if we have focus
			if (self->hasFocus)
			{
				// Allow our children to handle the event first
				bool childHandled = widgetDispatchEventToChildren(self, evt);

				// If none of our children handled it then handle it ourself
				if (!childHandled)
				{
					widgetFireCallbacks(self, evt);
				}

				// We handled the event
				handled = true;
			}
			break;
		}
		case EVT_TIMER:
		{
			// fireTimerCallbacks will handle this
			widgetFireTimerCallbacks(self, evt);
			break;
		}
		case EVT_TOOL_TIP_SHOW:
		case EVT_TOOL_TIP_HIDE:
		{
			// Fire any callback handlers
			widgetFireCallbacks(self, evt);
			break;
		}
		default:
			break;
	}

	return handled;
}

bool widgetPointMasked(const widget *self, point loc)
{
	/*
	 * The format of the mask is CAIRO_FORMAT_A1, where:
	 *  "each pixel is a 1-bit quantity holding an alpha value.
	 *   Pixels are packed together into 32-bit quantities"
	 *
	 * TODO: An explanation worth reading.
	 */

	cairo_surface_t *surface = cairo_get_target(self->maskCr);

	// The number of bytes to a row
	const int stride = cairo_image_surface_get_stride(surface);

	// The mask itself
	const uint32_t *data = (uint32_t *) cairo_image_surface_get_data(surface);

	// The 32-bit segment of data we are interested in
	const uint32_t bits = data[(int) loc.y * (stride / 4) + ((int) loc.x / 32)];

	// Where in the 32-bit segment the pixel is located
	const uint32_t pixelMask = 1 << ((int) loc.x % 32);

	// Check to see if the pixel is set or not
	if (bits & pixelMask)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void widgetSetToolTip(widget *self, const char *tip)
{
	// Free the current tip (if any)
	free((char *) self->toolTip);

	// If a new tip is being set, duplicate it
	if (tip)
	{
		self->toolTip = strdup(tip);
	}
	// No new tip being set; disable tool-tips
	else
	{
		self->toolTip = NULL;
	}
}

const char *widgetGetToolTip(widget *self)
{
	return self->toolTip;
}

bool widgetAddChild(widget *self, widget *child)
{
	return WIDGET_GET_VTBL(self)->addChild(self, child);
}

void widgetRemoveChild(widget *self, widget *child)
{
	WIDGET_GET_VTBL(self)->removeChild(self, child);
}

int widgetAddEventHandler(widget *self, eventType type, callback handler,
                          callback destructor, void *userData)
{
	return WIDGET_GET_VTBL(self)->addEventHandler(self, type, handler,
	                                              destructor, userData);
}

int widgetAddTimerEventHandler(widget *self, eventType type, int interval,
                               callback handler, callback destructor,
                               void *userData)
{
	return WIDGET_GET_VTBL(self)->addTimerEventHandler(self, type, interval,
	                                                   handler, destructor,
	                                                   userData);
}

void widgetRemoveEventHandler(widget *self, int id)
{
	WIDGET_GET_VTBL(self)->removeEventHandler(self, id);
}

bool widgetFireCallbacks(widget *self, const event *evt)
{
	return WIDGET_GET_VTBL(self)->fireCallbacks(self, evt);
}

bool widgetFireTimerCallbacks(widget *self, const event *evt)
{
	return WIDGET_GET_VTBL(self)->fireTimerCallbacks(self, evt);
}

void widgetEnable(widget *self)
{
	WIDGET_GET_VTBL(self)->enable(self);
}

void widgetDisable(widget *self)
{
	WIDGET_GET_VTBL(self)->disable(self);
}

void widgetShow(widget *self)
{
	WIDGET_GET_VTBL(self)->show(self);
}

void widgetHide(widget *self)
{
	WIDGET_GET_VTBL(self)->hide(self);
}

void widgetFocus(widget *self)
{
	WIDGET_GET_VTBL(self)->focus(self);
}

void widgetBlur(widget *self)
{
	WIDGET_GET_VTBL(self)->blur(self);
}

void widgetAcceptDrag(widget *self)
{
	WIDGET_GET_VTBL(self)->acceptDrag(self);
}

void widgetDeclineDrag(widget *self)
{
	WIDGET_GET_VTBL(self)->declineDrag(self);
}

size widgetGetMinSize(widget *self)
{
	WIDGET_CHECK_METHOD(self, getMinSize);

	return WIDGET_GET_VTBL(self)->getMinSize(self);
}

size widgetGetMaxSize(widget *self)
{
	WIDGET_CHECK_METHOD(self, getMaxSize);

	return WIDGET_GET_VTBL(self)->getMaxSize(self);
}

void widgetResize(widget *self, int w, int h)
{
	WIDGET_GET_VTBL(self)->resize(self, w, h);
}

void widgetReposition(widget *self, int x, int y)
{
	WIDGET_GET_VTBL(self)->reposition(self, x, y);
}

void widgetEnableGL(widget *self)
{
	WIDGET_GET_VTBL(self)->enableGL(self);
}

void widgetDisableGL(widget *self)
{
	WIDGET_GET_VTBL(self)->disableGL(self);
}

void widgetBeginGL(widget *self)
{
	WIDGET_GET_VTBL(self)->beginGL(self);
}

void widgetEndGL(widget *self, bool finishedDrawing)
{
	WIDGET_GET_VTBL(self)->endGL(self, finishedDrawing);
}

void widgetComposite(widget *self)
{
	WIDGET_GET_VTBL(self)->composite(self);
}

void widgetDoDraw(widget *self)
{
	WIDGET_CHECK_METHOD(self, doDraw);

	WIDGET_GET_VTBL(self)->doDraw(self);
}

void widgetDoDrawMask(widget *self)
{
	WIDGET_CHECK_METHOD(self, doDrawMask);

	WIDGET_GET_VTBL(self)->doDrawMask(self);
}

bool widgetDoLayout(widget *self)
{
	WIDGET_CHECK_METHOD(self, doLayout);

	return WIDGET_GET_VTBL(self)->doLayout(self);
}

bool widgetHandleEvent(widget *self, const event *evt)
{
	return WIDGET_GET_VTBL(self)->handleEvent(self, evt);
}

point widgetAnimationInterpolateTranslate(widget *self, animationFrame k1,
                                          animationFrame k2, int time)
{
	return WIDGET_GET_VTBL(self)->animationInterpolateTranslate(self, k1, k2,
                                                                time);
}

float widgetAnimationInterpolateRotate(widget *self, animationFrame k1,
                                       animationFrame k2, int time)
{
	return WIDGET_GET_VTBL(self)->animationInterpolateRotate(self, k1, k2,
	                                                         time);
}

point widgetAnimationInterpolateScale(widget *self, animationFrame k1,
                                      animationFrame k2, int time)
{
	return WIDGET_GET_VTBL(self)->animationInterpolateScale(self, k1, k2,
	                                                        time);
}

float widgetAnimationInterpolateAlpha(widget *self, animationFrame k1,
                                      animationFrame k2, int time)
{
	return WIDGET_GET_VTBL(self)->animationInterpolateAlpha(self, k1, k2,
	                                                        time);
}

void widgetDestroy(widget *self)
{
	WIDGET_GET_VTBL(self)->destroy(self);
}
