#include <string.h>

#include "widget.h"

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
	
	// Creatr a context to draw on the surface
	*cr = cairo_create(surface);
	
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

bool widgetIsA(widget *self, const classInfo *instanceOf)
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

		vtbl.getMinSize         = NULL;
		vtbl.getMaxSize         = NULL;
		
		vtbl.resize	            = widgetResizeImpl;
		
		vtbl.composite          = widgetCompositeImpl;

		vtbl.doLayout           = NULL;
		vtbl.doDraw             = NULL;
		vtbl.doDrawMask         = NULL;

		vtbl.destroy            = widgetDestroyImpl;

		initialised = true;
	}

	// Set the classes vtable
	self->vtbl = &vtbl;
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
	self->children = vectorCreate();

	// Prepare our events table
	self->eventVtbl = vectorCreate();

	// Copy the ID of the widget
	self->id = strdup(id);

	// Default parent is none
	self->parent = NULL;
	
	// Create a dummy cairo context (getMin/MaxSize may depend on it)
	self->cr = NULL;
	widgetCairoCreate(&self->cr, CAIRO_FORMAT_ARGB32, 0, 0);

	// Focus and mouse are false by default
	self->hasFocus = false;
	self->hasMouse = false;
	self->hasMouseDown = false;
	
	// By default we need drawing
	self->needsRedraw = true;
	
	// By default the mouse-event mask is disabled
	self->maskCr = NULL;
	self->maskEnabled = false;
}

/*
 * Widget class destructor (virtual).
 */
void widgetDestroyImpl(widget *self)
{
	// Release the container
	vectorMapAndDestroy(self->children, (mapCallback) widgetDestroy);

	// Release the event handler table
	vectorMapAndDestroy(self->eventVtbl, free);

	// Destroy the cairo context
	cairo_destroy(self->cr);

	// If we use a mask, destroy it
	if (self->maskEnabled)
	{
		cairo_destroy(self->maskCr);
	}
	
	// Free the ID
	free(self->id);

	// Free ourself
	free(self);
}

/*
 * Draws and widget and its child widgets
 */
void widgetDraw(widget *self)
{
	int i;
	
	// See if we need to be redrawn
	if (self->needsRedraw)
	{
		self->needsRedraw = false;
		
		// Clear the current context
		cairo_set_operator(self->cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(self->cr, 0.0, 0.0, 0.0, 0.0);
		cairo_paint(self->cr);
		
		// Restore the compositing operator back to the default
		cairo_set_operator(self->cr, CAIRO_OPERATOR_OVER);
		
		// Redaw ourself
		widgetDoDraw(self);
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
	// Make sure the id of the child is unquie
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
		// Remove child *without* calling its destructor
		vectorRemoveAt(self->children, vectorSize(self->children) - 1);

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
			// Call the destructor for the widget
			widgetDestroy(vectorAt(self->children, i));
			
			// Remove it from the list of children
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
	bool ret;

	for (i = 0; i < vectorSize(self->eventVtbl); i++)
	{
		eventTableEntry *handler = vectorAt(self->eventVtbl, i);

		// If handler is registered to handle evt
		if (handler->type == evt->type)
		{
			// Fire the callback
			ret = handler->callback(self, evt, handler->userData);
			
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
	// FIXME: We need to set the timestamp of the event
	eventMisc evt;
	evt.event.type = EVT_FOCUS;
	widgetFireCallbacks(self, (event *) &evt);
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
	// FIXME: We need to set the timestamp of the event
	eventMisc evt;
	evt.event.type = EVT_BLUR;
	widgetFireCallbacks(self, (event *) &evt);
}

void widgetResizeImpl(widget *self, int w, int h)
{
	const size minSize = widgetGetMinSize(self);
	const size maxSize = widgetGetMaxSize(self);
	
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
	
	// Set the needs redraw flag
	self->needsRedraw = true;
	
	// If we have any children, we need to redo their layout
	if (vectorSize(self->children))
	{
		widgetDoLayout(self);
	}
}

void widgetCompositeImpl(widget *self, cairo_t *comp)
{
	int i;
	
	// Composite ourself
	cairo_set_source_surface(comp, cairo_get_target(self->cr), 0, 0);
	cairo_paint(comp);

	// Now our children
	for (i = 0; i < vectorSize(self->children); i++)
	{
		widget *child = vectorAt(self->children, i);
		cairo_matrix_t current;
		
		// Translate such that (0,0) is the location of the widget
		cairo_get_matrix(comp, &current);
		cairo_translate(comp, child->offset.x, child->offset.y);
		
		// Composite
		widgetComposite(child, comp);
		
		// Restore the matrix
		cairo_set_matrix(comp, &current);
	}
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

bool widgetHandleEventImpl(widget *self, event *evt)
{
	// If the event should be passed onto our children
	bool relevant = true;

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

			// If the mouse is inside of the widget
			if (widgetHasMouse(self, evtMouseBtn.loc))
			{
				// If it is a mouse-down event set hasMouseDown to true
				if (evt->type == EVT_MOUSE_DOWN)
				{
					self->hasMouseDown = true;
				}

				// Fire mouse down and mouse up callbacks
				widgetFireCallbacks(self, (event *) &evtMouseBtn);

				// Check for a click event
				if (evt->type == EVT_MOUSE_UP && self->hasMouseDown)
				{
					evtMouseBtn.event.type = EVT_MOUSE_CLICK;

					widgetFireCallbacks(self, (event *) &evtMouseBtn);
				}
			}
			// If the mouse is no longer down
			else if (evt->type == EVT_MOUSE_UP && self->hasMouseDown)
			{
				self->hasMouseDown = false;
			}
			// Of no interest to us or our children
			else
			{
				relevant = false;
			}
			break;
		}
		case EVT_KEY_DOWN:
		case EVT_KEY_UP:
		{
			// Only relevant if we have focus
			if (self->hasFocus)
			{
				widgetFireCallbacks(self, evt);
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

bool widgetPointMasked(widget *self, point loc)
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
	const uint32_t bits = data[loc.y * (stride / 4) + (loc.x / 32)];
	
	// Where in the 32-bit segment the pixel is located
	const uint32_t pixelMask = 1 << (loc.x % 32);
	
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

void widgetResize(widget *self, int x, int y)
{
	WIDGET_GET_VTBL(self)->resize(self, x, y);
}

void widgetComposite(widget *self, cairo_t *comp)
{
	WIDGET_GET_VTBL(self)->composite(self, comp);
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

bool widgetHandleEvent(widget *self, event *evt)
{
	return WIDGET_GET_VTBL(self)->handleEvent(self, evt);
}

void widgetDestroy(widget *self)
{
	return WIDGET_GET_VTBL(self)->destroy(self);
}
