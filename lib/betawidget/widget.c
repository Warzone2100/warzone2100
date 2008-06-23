#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "widget.h"

static widgetVtbl vtbl;

bool widgetIsA(widget *self, classInfo *instanceOf)
{
	classInfo *widgetClass;
	
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

/**
 * Prepares the widget classes vtable.
 */
static void widgetInitVtbl(widget *self)
{
	static bool initialised = false;

	if (!initialised)
	{
		vtbl.addChild           = widgetAddChildImpl;
		vtbl.removeChild        = widgetRemoveChildImpl;

		vtbl.fireCallbacks      = widgetFireCallbacksImpl;
		vtbl.addEventHandler    = widgetAddEventHandlerImpl;
		vtbl.removeEventHandler = widgetRemoveEventHandlerImpl;
		vtbl.handleEvent        = widgetHandleEventImpl;

		vtbl.focus              = widgetFocusImpl;
		vtbl.blur               = widgetBlurImpl;

		vtbl.enable             = widgetEnableImpl;
		vtbl.disable            = widgetDisableImpl;

		vtbl.getMinSize         = widgetGetMinSizeImpl;
		vtbl.getMaxSize         = widgetGetMaxSizeImpl;

		vtbl.setAlign           = widgetSetAlignImpl;

		vtbl.drawChildren       = widgetDrawChildrenImpl;

		vtbl.doLayout           = NULL;
		vtbl.doDraw             = NULL;

		vtbl.destroy            = widgetDestroyImpl;

		initialised = true;
	}

	// Set the classes vtable
	self->vtbl = &vtbl;

	// Do any overloading of inherited methods here
}

/*
 * Widget class constructor
 */
void widgetInit(widget *self, const char *id)
{
	// Prepare our vtable
	widgetInitVtbl(self);

	// Set our type
	self->classInfo = &widgetClassInfo;
	
	// Prepare our container
	self->children = vectorCreate((destroyCallback) widgetDestroy);

	// Prepare our events table
	self->eventVtbl = vectorCreate(free);

	// Copy the ID of the widget
	self->id = strdup(id);

	// Default parent is none
	self->parent = NULL;

	// Focus and mouse are false by default
	self->hasFocus = false;
	self->hasMouse = false;
	self->hasMouseDown = false;
}

/*
 * Widget class destructor (virtual).
 */
void widgetDestroyImpl(widget *self)
{
	// Release the container
	vectorDestroy(self->children);

	// Release the event handler table
	vectorDestroy(self->eventVtbl);

	// Free the ID
	free(self->id);

	// Free ourself
	free(self);
}

/*
 * Draws and widget and its child widgets
 */
void widgetDraw(widget *self, cairo_t *cr)
{
	// Draw ourself
	widgetDoDraw(self, cr);

	// Draw our children
	widgetDrawChildren(self, cr);
}

point widgetAbsolutePosition(widget *self)
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

rect widgetAbsoluteBounds(widget *self)
{
	// Get our position
	point p = widgetAbsolutePosition(self);

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

/*
 *
 */
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

/*
 *
 */
bool widgetAddChildImpl(widget *self, widget *child)
{
	// Make sure the id of the child is unqie
	if (widgetFindById(widgetGetRoot(self), child->id) != NULL)
	{
		// TODO: An error/debug message is probably required
		return false;
	}

	// Add the widget
	vectorAdd(self->children, child);

	// Re-layout ourself
	if (widgetDoLayout(self))
	{
		// Set ourself as its parent
		child->parent = self;

		return true;
	}
	// Not enough space to fit the widget
	else
	{
		// Remove child
		widgetRemoveChild(self, child);

		// Restore the layout
		widgetDoLayout(self);

		return false;
	}
}

/*
 *
 */
void widgetRemoveChildImpl(widget *self, widget *child)
{
	int i;

	for (i = 0; i < vectorSize(self->children); i++)
	{
		// If the child is the to-be-removed widget, remove it
		if (vectorAt(self->children, i) == child)
		{
			// vectorRemoveAt will take care of calling the widgets destructor
			vectorRemoveAt(self->children, i);
		}
		// See if it is one of its children
		else
		{
			widgetRemoveChild(vectorAt(self->children, i), child);
		}
	}
}

/*
 *
 */
int widgetAddEventHandlerImpl(widget *self, eventType type, callback handler, void *userData)
{
	eventTableEntry *entry = malloc(sizeof(eventTableEntry));

	entry->type     = type;
	entry->callback = handler;
	entry->userData = userData;

	// Add the handler to the table
	vectorAdd(self->eventVtbl, entry);

	// Offset = size - 1
	return vectorSize(self->eventVtbl) - 1;
}

/*
 *
 */
void widgetRemoveEventHandlerImpl(widget *self, int id)
{
	vectorRemoveAt(self->eventVtbl, id);
}

bool widgetFireCallbacksImpl(widget *self, event *evt)
{
	int i;

	for (i = 0; i < vectorSize(self->eventVtbl); i++)
	{
		eventTableEntry *handler = vectorAt(self->eventVtbl, i);

		if (handler->type == evt->type)
		{
			handler->callback(self, evt, handler->userData);
		}
	}

	// FIXME
	return true;
}

/*
 *
 */
void widgetEnableImpl(widget *self)
{
	int i;

	// First make sure our parent is enabled
	if (self->parent && !self->parent->isEnabled)
	{
		return;
	}

	// Enable ourself
	self->isEnabled = true;

	// Enable all of our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widgetEnable(vectorAt(self->children, i));
	}
}

void widgetDisableImpl(widget *self)
{
	int i;

	// If we are currently disabled, return
	if (!self->isEnabled)
	{
		return;
	}

	// Disable ourself
	self->isEnabled = false;

	// Disable our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widgetDisable(WIDGET(vectorAt(self->children, i)));
	}
}

void widgetFocusImpl(widget *self)
{
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
	event evt;
	evt.type = EVT_FOCUS;
	widgetFireCallbacks(self, &evt);
}

void widgetBlurImpl(widget *self)
{
	widget *current;

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
	event evt;
	evt.type = EVT_BLUR;
	widgetFireCallbacks(self, &evt);
}

point widgetGetMinSizeImpl(widget *self)
{
	return self->minSize;
}

point widgetGetMaxSizeImpl(widget *self)
{
	return self->maxSize;
}

void widgetSetAlignImpl(widget *self, vAlign v, hAlign h)
{
	self->vAlignment = v;
	self->hAlignment = h;

	// Re-align our children
	// TODO: We should check the return value here
	widgetDoLayout(self);
}

void widgetDrawChildrenImpl(widget *self, cairo_t *cr)
{
	int i;

	// Draw our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widget *child = vectorAt(self->children, i);
		cairo_matrix_t current;

		// Translate such that (0,0) is the location of the widget
		cairo_get_matrix(cr, &current);
		cairo_translate(cr, child->offset.x, child->offset.y);

		widgetDraw(child, cr);

		// Restore the matrix
		cairo_set_matrix(cr, &current);
	}
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

bool widgetHandleEventImpl(widget *self, event *evt)
{
	// If the event should be passed onto our children
	bool relevant = true;

	switch (evt->type)
	{
		case EVT_MOUSE_MOVE:
		{
			eventMouse evtMouse = *((eventMouse *) evt);
			bool newHasMouse = pointInRect(evtMouse.loc, widgetAbsoluteBounds(self));

			/*
			 * Mouse motion events should not be dispatched if a mouse button
			 * is currently `down' on our parent but not ourself.
			 */
			if (self->parent && self->parent->hasMouseDown && !self->hasMouseDown)
			{
				relevant = false;
				break;
			}

			// If we have just `got' the mouse
			if (newHasMouse && !self->hasMouse)
			{
				// Generate a EVT_MOUSE_ENTER event
				evtMouse.event.type = EVT_MOUSE_ENTER;

				// Fire the event handler
				widgetFireCallbacks(self, (event *) &evtMouse);
			}
			// If we have just lost the mouse
			else if (!newHasMouse && self->hasMouse)
			{
				// Generate a EVT_MOUSE_LEAVE event
				evtMouse.event.type = EVT_MOUSE_LEAVE;

				// Fire the handler
				widgetFireCallbacks(self, (event *) &evtMouse);
			}
			// We had and still have the mouse
			else if (newHasMouse && self->hasMouse)
			{
				// Pass the event as-is
				widgetFireCallbacks(self, (event *) &evtMouse);
			}
			// Of no interest to us (and therefore not to our children either)
			else
			{
				relevant = false;
			}

			// Update the status of the mouse
			self->hasMouse = newHasMouse;
			break;
		}
		case EVT_MOUSE_DOWN:
		case EVT_MOUSE_UP:
		{
			eventMouseBtn evtMouseBtn = *((eventMouseBtn *) evt);

			if (pointInRect(evtMouseBtn.loc, widgetAbsoluteBounds(self)))
			{
				// If it is a mouse-down event set hasMouseDown to true
				if (evt->type == EVT_MOUSE_DOWN)
				{
					self->hasMouseDown = true;
				}

				widgetFireCallbacks(self, (event *) &evtMouseBtn);

				if (evt->type == EVT_MOUSE_UP && self->hasMouseDown)
				{
					evtMouseBtn.event.type = EVT_MOUSE_CLICK;

					widgetFireCallbacks(self, (event *) &evtMouseBtn);
				}
			}
			else if (evt->type == EVT_MOUSE_UP && self->hasMouseDown)
			{
				self->hasMouseDown = false;
			}
			else
			{
				relevant = false;
			}
			break;
		}
		case EVT_KEY_DOWN:
		case EVT_KEY_UP:
		{
			eventKey evtKey = *((eventKey *) evt);

			// Only relevant if we have focus
			if (self->hasFocus)
			{
				widgetFireCallbacks(self, (event *) &evtKey);
			}
			else
			{
				relevant = false;
			}
			break;
		}
		default:
			break;
	}

	// If necessary pass the event onto our children
	if (relevant)
	{
		int i;

		for (i = 0; i < vectorSize(self->children); i++)
		{
			widgetHandleEvent(vectorAt(self->children, i), evt);
		}
	}

	return true;
}

bool widgetAddChild(widget *self, widget *child)
{
	return WIDGET_GET_VTBL(self)->addChild(self, child);
}

void widgetRemoveChild(widget *self, widget *child)
{
	WIDGET_GET_VTBL(self)->removeChild(self, child);
}

int widgetAddEventHandler(widget *self, eventType type,
                          callback handler, void *userData)
{
	return WIDGET_GET_VTBL(self)->addEventHandler(self, type, handler, userData);
}

void widgetRemoveEventHandler(widget *self, int id)
{
	WIDGET_GET_VTBL(self)->removeEventHandler(self, id);
}

bool widgetFireCallbacks(widget *self, event *evt)
{
	return WIDGET_GET_VTBL(self)->fireCallbacks(self, evt);
}

void widgetEnable(widget *self)
{
	WIDGET_GET_VTBL(self)->enable(self);
}

void widgetDisable(widget *self)
{
	WIDGET_GET_VTBL(self)->disable(self);
}

void widgetFocus(widget *self)
{
	WIDGET_GET_VTBL(self)->focus(self);
}

void widgetBlur(widget *self)
{
	WIDGET_GET_VTBL(self)->blur(self);
}

point widgetGetMinSize(widget *self)
{
	return WIDGET_GET_VTBL(self)->getMinSize(self);
}

point widgetGetMaxSize(widget *self)
{
	return WIDGET_GET_VTBL(self)->getMaxSize(self);
}

void widgetSetAlign(widget *self, vAlign v, hAlign h)
{
	WIDGET_GET_VTBL(self)->setAlign(self, v, h);
}

void widgetDrawChildren(widget *self, cairo_t *cr)
{
	WIDGET_GET_VTBL(self)->drawChildren(self, cr);
}

void widgetDoDraw(widget *self, cairo_t *cr)
{
	WIDGET_CHECK_METHOD(self, doDraw);

	WIDGET_GET_VTBL(self)->doDraw(self, cr);
}

bool widgetDoLayout(widget *self)
{
	WIDGET_CHECK_METHOD(self, doLayout);

	return WIDGET_GET_VTBL(self)->doLayout(self);
}

bool widgetHandleEvent(widget *self, event *evt)
{
	return WIDGET_GET_VTBL(self)->handleEvent(self, evt);
}

void widgetDestroy(widget *self)
{
	return WIDGET_GET_VTBL(self)->destroy(self);
}
