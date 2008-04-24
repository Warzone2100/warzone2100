#ifndef WIDGET_H_
#define WIDGET_H_

#include <cairo.h>

#include "vector.h"
#include "geom.h"

/*
 * Forward declarations 
 */
typedef struct _widget widget;
typedef struct _widgetVtbl widgetVtbl;

typedef enum _eventType eventType;
typedef enum _mouseButton mouseButton;

typedef struct _event			event;
typedef struct _eventMouse		eventMouse;
typedef struct _eventMouseBtn	eventMouseBtn;
typedef struct _eventKey		eventKey;
typedef struct _eventMisc		eventMisc;

typedef bool (*callback)		(widget *widget, event *evt);

typedef struct _eventTableEntry eventTableEntry;

/*
 * The valid event types
 */
enum _eventType
{
	// Mouse events
	EVT_MOUSE_DOWN,
	EVT_MOUSE_UP,
	EVT_MOUSE_CLICK,
	
	EVT_MOUSE_ENTER,
	EVT_MOUSE_MOVE,
	EVT_MOUSE_LEAVE,
	
	// Keyboard events
	EVT_KEY_DOWN,
	EVT_KEY_UP,
	
	// Misc
	EVT_FOCUS,
	EVT_BLUR
};

/*
 * The possible mouse states as understood by the events system
 */
enum _mouseButton
{
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_OTHER
};

/*
 * Event structures
 */


/*
 * The 'base' event structure. All events can be cast to this
 */
struct _event
{
	// The time at which the event took place
	int time;
	
	// The type of the event
	eventType type;
};

/*
 * The event structure used for mouse motion events
 */
struct _eventMouse
{
	event event;
	
	// Location of the event
	point loc;
};

/*
 * The event structure used for mouse button events
 */
struct _eventMouseBtn
{
	event event;
	
	// Location
	point loc;
	
	// Button pressed
	mouseButton button;
};

/*
 * The event structure used for keyboard events
 */
struct _eventKey
{
	event event;
	
	// The key which was pressed
	int key;
	
	// Active modifier keys
	bool ctrl;
	bool shift;
	bool alt;
};

/*
 * 
 */
struct _eventMisc
{
	event event;
};

/*
 * Event table structure
 */
struct _eventTableEntry
{
	eventType type;
	callback callback;
};

/*
 * The widget classes virtual method table.
 */
struct _widgetVtbl
{
	bool	(*handleEvent)			(widget *self, event *evt);
	
	bool	(*addChild)				(widget *self, widget *child);
	void	(*removeChild)			(widget *self, widget *child);
	
	bool	(*fireCallbacks)		(widget *self, event *evt);
	int		(*addEventHandler)		(widget *self, eventType type, callback handler);
	void	(*removeEventHandler)	(widget *self, int id);
	
	void	(*focus)				(widget *self);
	void	(*blur)					(widget *self);
	
	void	(*enable)				(widget *self);
	void	(*disable)				(widget *self);
	
	void	(*doDraw)				(widget *self, cairo_t *cr);
	
	void	(*destroy)				(widget *self);
};

/*
 * 
 */
struct _widget
{
	//--------------------------------------
	// Private/protected members
	//--------------------------------------
	widgetVtbl *vtbl;
	
	/*
	 * The list of registered event handlers.
	 */
	vector *eventVtbl;
	
	/*
	 * The child widgets of ourself.
	 */
	vector *children;
	
	/*
	 * The widgets parent widget.
	 */
	widget *parent;
	
	//--------------------------------------
	// Public members
	//--------------------------------------
	
	/*
	 * The id of the widget
	 */
	char *id;
	
	/*
	 * Arbitary user-defined data
	 */
	void *pUserData;
	int userData;
	
	/*
	 * The origin and size of the widget
	 */
	rect bounds;
	
	/*
	 * If the widget currently has keyboard focus
	 */
	bool hasFocus;
	
	/*
	 * If the mouse is currently over the widget
	 */
	bool hasMouse;
	
	/*
	 * If the widget is currently enabled or not
	 */
	bool isEnabled;
};

/*
 * Helper macros
 */
#define WIDGET(self) ((widget *) (self))
#define WIDGET_GET_VTBL(self) ((WIDGET(self))->vtbl)

/*
 * Protected methods
 */
void widgetInit(widget *instance, const char *id);
void widgetDestroyImpl(widget *instance);
bool widgetAddChildImpl(widget *self, widget *child);
void widgetRemoveChildImpl(widget *self, widget *child);
bool widgetFireCallbacksImpl(widget *self, event *evt);
int widgetAddEventHandlerImpl(widget *self, eventType type, callback handler);
void widgetRemoveEventHandlerImpl(widget *self, int id);
void widgetEnableImpl(widget *self);
void widgetDisableImpl(widget *self);
void widgetFocusImpl(widget *self);
void widgetBlurImpl(widget *self);

bool widgetHandleEventImpl(widget *instance, event *evt);

/*
 * Public methods
 */

/**
 * Draws the widget along with its child widgets.
 */
void widgetDraw(widget *self, cairo_t *cr);

/**
 * Recursively searches the child widgets of self for a widget whose ->id is
 * id. If no widget with such an id exists NULL is returned.
 */
widget *widgetFindById(widget *self, const char *id);

/**
 * Transverses up the hierarchy until it finds parent-less widget (known as
 * the root widget). A pointer to this widget is returned.
 */
widget *windgetGetRoot(widget *self);

/**
 *
 */
bool widgetAddChild(widget *self, widget *child);

/**
 *
 */
void widgetRemoveChild(widget *self, widget *child);

/**
 * 
 */
int widgetAddEventHandler(widget *self, eventType type, callback handler);

/**
 * 
 */
void widgetRemoveEventHandler(widget *self, int id);

/**
 * Enables the current widget along with all of its child widgets. If the
 * widget is currently enabled but one or more of its child widgets are not
 * then they will also be enabled.
 *
 * If, however, the parent widget is disabled then this method is effectively
 * a no-op.
 */
void widgetEnable(widget *self);

/**
 * Disables the current widget along with all of its child widgets. If the
 * widget is currently disabled then this method is a no-op.
 */
void widgetDisable(widget *self);

/**
 * Destroys the widget and frees *all* memory associated with it.
 */
void widgetDestroy(widget *self);

/**
 * Returns a pointer to the widget furthest down the hierarchy which currently
 * has focus. If self does not currently have focus then NULL is returned.
 * Should none of self's child widgets have focus (but it does) then self is
 * returned.
 */
widget *widgetGetCurrentlyFocused(widget *self);

/**
 * If the widget is capable of holding keyboard focus and does not currently
 * hold it then this method should bring the widget into focus.
 *
 * This method will call the blur() method of the root widget first.
 */
void widgetFocus(widget *self);

/**
 * Blurs the current widget (removes keyboard focus from it) and fires the
 * EVT_BLUR event handlers for affected widgets.
 */
void widgetBlur(widget *self);

/**
 * 
 */
bool widgetHandleEvent(widget *self, event *evt);

/**
 *
 */
bool widgetFireCallbacks(widget *self, event *evt);

/*
 * Protected methods
 */

/**
 * 
 */
void widgetDoDraw(widget *self, cairo_t *cr);

#endif /*WIDGET_H_*/
