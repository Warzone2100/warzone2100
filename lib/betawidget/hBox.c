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
		vtbl.widgetVtbl.getMaxSize  = hBoxGetMinSizeImpl;
		
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
	
	int i, temp = 0;
	const int numChildren = vectorSize(self->children);
	const size minSize = widgetGetMinSize(self);
	sizeInfo *childSizeInfo = malloc(sizeof(sizeInfo) * numChildren);
	bool maxedOut = false;
	
	// First make sure we are large enough to hold all of our children
	if (self->size.x < minSize.x
	 || self->size.y < minSize.y)
	{
		return false;
	}
	
	// Make sure malloc did not return NULL
	if (childSizeInfo == NULL)
	{
		// We're screwed
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
	
	// Next do y-axis positioning and initial x-axis sizing
	for (i = 0, temp = self->size.x; i < numChildren; i++)
	{
		sizeInfo *child = &childSizeInfo[i];
		
		// Make the child as heigh as possible
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
	
	// Gradually increase the x-size of child widgets
	while (!maxedOut)
	{
		maxedOut = true;
		
		for (i = 0; temp && i < numChildren; i++)
		{
			sizeInfo *child = &childSizeInfo[i];
			
			if (child->currentSize.x < child->maxSize.x)
			{
				child->currentSize.x++;
				temp--;
				maxedOut = false;
			}
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
		
		child->offset.x = temp;
		child->offset.y = childSize->offset.y;
		
		temp += child->size.x;
	}
	
	// We're done!
	free(childSizeInfo);
	
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

size hBoxGetMinSizeImpl(widget *self)
{
	size minSize = { 0, 0 };
	int i;
	
	for (i = 0; i < vectorSize(self->children); i++)
	{
		const size minChildSize = widgetGetMinSize(vectorAt(self->children, i));
		
		minSize.x += minChildSize.x;
		minSize.y = MAX(minSize.y, minChildSize.y);
	}
	
	return minSize;
}

size hBoxGetMaxSizeImpl(widget *self)
{
	size maxSize = { 0, 0 };
	int i;
	
	for (i = 0; i < vectorSize(self->children); i++)
	{
		const size maxChildSize = widgetGetMaxSize(vectorAt(self->children, i));
		
		maxSize.x += maxChildSize.x;
		maxSize.y = MAX(maxSize.y, maxChildSize.y);
	}
	
	return maxSize;
}

void hBoxDoDrawImpl(widget *self)
{
	// NO-OP
	(void) self;
}
