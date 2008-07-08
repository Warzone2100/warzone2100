#ifndef SPACER_H_
#define SPACER_H_

#include "widget.h"

/*
 * Forward declarations
 */
typedef struct _spacer spacer;
typedef struct _spacerVtbl spacerVtbl;

typedef enum _spacerDirection spacerDirection;

/*
 * The directions a spacer can act in
 */
enum _spacerDirection
{
	SPACER_DIRECTION_HORIZONTAL,
	SPACER_DIRECTION_VERTICAL
};

struct _spacerVtbl
{
	widgetVtbl widgetVtbl;
	
	// No additional virtual methods
};

struct _spacer
{
	/*
	 * Parent
	 */
	widget widget;
	
	/*
	 * Our vtable
	 */
	spacerVtbl *vtbl;
	
	/*
	 * Which direction we should act in
	 */
	spacerDirection direction;
};

/*
 * Type information
 */
extern const classInfo spacerClassInfo;

/*
 * Helper macros
 */
#define SPACER(self) (assert(widgetIsA(WIDGET(self), &spacerClassInfo)), \
                      (spacer *) (self))

/*
 * Protected methods
 */
void spacerInit(spacer *self, const char *id, spacerDirection direction);
void spacerDestroyImpl(widget *self);
void spacerDoDrawImpl(widget *self);
size spacerGetMinSizeImpl(widget *self);
size spacerGetMaxSizeImpl(widget *self);

/*
 * Public methods
 */

/**
 * Constructs a new spacer object and returns it.
 * 
 * @param id    The id of the widget.
 * @param direction The direction of the spacer.
 * @return A pointer to an spacer on success otherwise NULL.
 */
spacer *spacerCreate(const char *id, spacerDirection direction);

#endif /*SPACER_H_*/
