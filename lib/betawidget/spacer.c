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
		vtbl.widgetVtbl.doDraw      = spacerDoDrawImpl;
		vtbl.widgetVtbl.getMinSize  = spacerGetMinSizeImpl;
		vtbl.widgetVtbl.getMaxSize  = spacerGetMaxSizeImpl;
		
		initialised = true;
	}
	
	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;
	
	// Set our vtablr
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

void spacerDoDrawImpl(widget *self)
{
	// NO-OP
	(void) self;
}
