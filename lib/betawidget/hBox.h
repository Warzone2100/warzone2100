#ifndef HBOX_H_
#define HBOX_H_

#include "widget.h"

/*
 * Forward declarations
 */
typedef struct _hBox hBox;
typedef struct _hBoxVtbl hBoxVtbl;

typedef enum _vAlign vAlign;

/*
 * The possible vertical sides a child can be aligned to
 */
enum _vAlign
{
	TOP,
	MIDDLE,
	BOTTOM
};

struct _hBoxVtbl
{
	widgetVtbl widgetVtbl;
	
	// No additional virtual methods
};

/*
 * 
 */
struct _hBox
{
	/*
	 * Parent
	 */
	widget widget;
	
	/*
	 * Our vtable
	 */
	hBoxVtbl *vtbl;
	
	/*
	 * Child alignment
	 */
	vAlign vAlignment;
};

/*
 * Type information
 */
extern const classInfo hBoxClassInfo;

/*
 * Helper macros
 */
#define HBOX(self) (assert(widgetIsA(WIDGET(self), &hBoxClassInfo)), \
                    (hBox *) (self))

/*
 * Protected methods
 */
void hBoxInit(hBox *instance, const char *id);
void hBoxDestroyImpl(widget *instance);
bool hBoxDoLayoutImpl(widget *self);
void hBoxDoDrawImpl(widget *self);
size hBoxGetMinSizeImpl(widget *self);
size hBoxGetMaxSizeImpl(widget *self);

/*
 * Public methods
 */

/**
 * Constructs a new hBox object and returns it.
 * 
 * @param id    The id of the widget.
 * @return A pointer to an hBox on success otherwise NULL.
 */
hBox *hBoxCreate(const char *id);

/**
 * Sets the alignment of child widgets of self. This comes into effect when the
 * maximum vertical size of the child widgets is less than that or self.
 * 
 * @param self  The hBox to set the alignment properties of.
 * @param v     The vertical alignment scheme to use.
 */
void hBoxSetVAlign(hBox *self, vAlign v);

#endif /*HBOX_H_*/
