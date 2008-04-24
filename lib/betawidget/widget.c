#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "widget.h"

static widgetVtbl vtbl;

/*
 * Forward declarations 
 */
static void widgetDrawChildren(widget *self, cairo_t *cr);

/**
 * Prepares the widget classes vtable.
 */
static void widgetInitVtbl(widget *self)
{
	static bool initialised = false;
	
	if (!initialised)
	{
		vtbl.addChild			= widgetAddChildImpl;
		vtbl.removeChild		= widgetRemoveChildImpl;
		
		vtbl.fireCallbacks		= widgetFireCallbacksImpl;	
		vtbl.addEventHandler	= widgetAddEventHandlerImpl;
		vtbl.removeEventHandler	= widgetRemoveEventHandlerImpl;
		vtbl.handleEvent		= widgetHandleEventImpl;
		
		vtbl.focus				= widgetFocusImpl;
		vtbl.blur				= widgetBlurImpl;
		
		vtbl.enable				= widgetEnableImpl;
		vtbl.disable			= widgetDisableImpl;
		
		vtbl.doDraw		 		= NULL;
		
		vtbl.destroy			= widgetDestroyImpl;
		
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
	
	// Prepare our container
	self->children = vectorCreate((destroyCallback) widgetDestroy);
	
	// Prepare our events table
	self->eventVtbl = vectorCreate(free);
	
	// Copy the ID of the widget
	self->id = strdup(id);
	
	// Default parent is none
	self->parent = NULL;
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

/*
 * Draws the child widgets of self.
 */
static void widgetDrawChildren(widget *self, cairo_t *cr)
{
	int i;
	
	// Draw our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widget *child = WIDGET(vectorAt(self->children, i));
		cairo_matrix_t current;
		
		// Translate such that (0,0) is the location of the widget
		cairo_get_matrix(cr, &current);
		cairo_translate(cr, child->bounds.topLeft.x, child->bounds.topLeft.y);
		
		widgetDraw(child, cr);
		
		// Restore the matrix
		cairo_set_matrix(cr, &current);
	}
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
	// FIXME: We need to do some arbitration
	
	// Add the widget
	vectorAdd(self->children, child);
	
	// Set ourself as its parent
	child->parent = self;
	
	return true;
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
int widgetAddEventHandlerImpl(widget *self, eventType type, callback handler)
{
	eventTableEntry *entry = malloc(sizeof(eventTableEntry));
	
	entry->type		= type;
	entry->callback = handler;
	
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
			handler->callback(self, evt);
		}
	}
	
	// FIXIME
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
		widgetEnable(WIDGET(vectorAt(self->children, i)));
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
	switch (evt->type)
	{
		case EVT_MOUSE_MOVE:
		{	
			eventMouse evtMouse = *((eventMouse *) evt);
			bool newHasMouse = pointInRect(evtMouse.loc, self->bounds);
				
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
			
			// Update the status of the mouse
			self->hasMouse = newHasMouse;
			break;
		}
		default:
			break;
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

int widgetAddEventHandler(widget *self, eventType type, callback handler)
{
	return WIDGET_GET_VTBL(self)->addEventHandler(self, type, handler);
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

void widgetDoDraw(widget *self, cairo_t *cr)
{
	WIDGET_GET_VTBL(self)->doDraw(self, cr);
}

bool widgetHandleEvent(widget *self, event *evt)
{
	return WIDGET_GET_VTBL(self)->handleEvent(self, evt);
}

void widgetDestroy(widget *self)
{
	return WIDGET_GET_VTBL(self)->destroy(self);
}
