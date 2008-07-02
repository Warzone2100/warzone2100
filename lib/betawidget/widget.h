#ifndef WIDGET_H_
#define WIDGET_H_

// TODO: Make this cross platform (MSVC)
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <cairo.h>

#include "vector.h"
#include "geom.h"

/*
 * Forward declarations
 */
typedef struct _classInfo classInfo;

typedef struct _widget widget;
typedef struct _widgetVtbl widgetVtbl;

typedef enum _eventType eventType;
typedef enum _mouseButton mouseButton;

typedef struct _event           event;
typedef struct _eventMouse      eventMouse;
typedef struct _eventMouseBtn   eventMouseBtn;
typedef struct _eventKey        eventKey;
typedef struct _eventMisc       eventMisc;

typedef bool (*callback)        (widget *widget, event *evt, void *userData);

typedef struct _eventTableEntry eventTableEntry;

/*
 * Information about the `type' (class) of a widget
 */
struct _classInfo
{
	const struct _classInfo *parentType;
	const char *ourType;
};

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
	BUTTON_WHEEL_UP,
	BUTTON_WHEEL_DOWN,
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

	// The key which was pressed, this should be used for text-input &c
	// NB: This is only valid for EVT_KEY_DOWN events!
	int unicode;

	// The keycode of the key which was pressed
	int keycode;

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
	void *userData;
};

/*
 * The widget classes virtual method table
 */
struct _widgetVtbl
{
	bool    (*handleEvent)                  (widget *self, event *evt);

	bool    (*addChild)                     (widget *self, widget *child);
	void    (*removeChild)                  (widget *self, widget *child);

	bool    (*fireCallbacks)                (widget *self, event *evt);
	int     (*addEventHandler)              (widget *self, eventType type,
	                                         callback handler, void *userData);
	void    (*removeEventHandler)           (widget *self, int id);

	void    (*focus)                        (widget *self);
	void    (*blur)                         (widget *self);

	void    (*enable)                       (widget *self);
	void    (*disable)                      (widget *self);

	size    (*getMinSize)                   (widget *self);
	size    (*getMaxSize)                   (widget *self);
	
	void    (*resize)                       (widget *self, int x, int y);
	
	void    (*composite)                    (widget *self, cairo_t *comp);

	void    (*doDraw)                       (widget *self);
	bool    (*doLayout)                     (widget *self);

	void    (*destroy)                      (widget *self);
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
	 * The list of registered event handlers
	 */
	vector *eventVtbl;

	/*
	 * The child widgets of ourself
	 */
	vector *children;

	/*
	 * The widgets parent widget
	 */
	widget *parent;

	/*
	 * If a mouse button is currently depressed on the widget
	 */
	bool hasMouseDown;
	
	/*
	 * The widgets cairo drawing context
	 */
	cairo_t *cr;

	//--------------------------------------
	// Public members
	//--------------------------------------

	/*
	 * The id of the widget
	 */
	char *id;

	/*
	 * The class (or subclass) that widget is (used for type checking)
	 */
	const classInfo *classInfo;
	
	/*
	 * Arbitary user-defined data
	 */
	void *pUserData;
	int32_t userData;

	/*
	 * The offset of the widget relative to its parent
	 */
	point offset;

	/*
	 * The size of the widget
	 */
	size size;

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
	
	/*
	 * If the widget is dirty (i.e., needs to be re-drawn)
	 */
	bool needsRedraw;
};

/*
 * Type information
 */
extern const classInfo widgetClassInfo;

/*
 * Helper macros
 */
#define WIDGET(self) ((widget *) (self))
#define WIDGET_GET_VTBL(self) ((WIDGET(self))->vtbl)
#define WIDGET_CHECK_METHOD(self, method) (assert(WIDGET_GET_VTBL(self)->method))

/*
 * Protected methods
 */
void widgetInit(widget *instance, const char *id);
void widgetDestroyImpl(widget *instance);
bool widgetAddChildImpl(widget *self, widget *child);
void widgetRemoveChildImpl(widget *self, widget *child);
bool widgetFireCallbacksImpl(widget *self, event *evt);
int widgetAddEventHandlerImpl(widget *self, eventType type,
                              callback handler, void *userData);
void widgetRemoveEventHandlerImpl(widget *self, int id);
void widgetEnableImpl(widget *self);
void widgetDisableImpl(widget *self);
void widgetFocusImpl(widget *self);
void widgetBlurImpl(widget *self);
void widgetResizeImpl(widget *self, int w, int h);
bool widgetHandleEventImpl(widget *self, event *evt);
void widgetCompositeImpl(widget *self, cairo_t *comp);

/*
 * Public static methods
 */

/**
 * Checks to see if it is legal to cast self to instanceOf. Or, put in OO terms
 * checks if self `is a' instanceOf instance.
 * 
 * @param self	The widget to check the class of.
 * @param instanceOf	The class we are interested in.
 * @return True if it is legal to cast, false otherwise.
 */
bool widgetIsA(widget *self, const classInfo *instanceOf);

/*
 * Public methods
 */

/**
 * Draws the widget along with its child widgets.
 *
 * @param self  The widget to be drawn.
 */
void widgetDraw(widget *self);

/**
 * TODO
 */
void widgetComposite(widget *self, cairo_t *comp);

/**
 * Recursively searches the child widgets of self for a widget whose ->id is
 * id. If no widget with such an id exists NULL is returned.
 *
 * @param self  The widget to start the search from.
 * @param id    The id of the desired widget.
 * @return A pointer to the widget if found, NULL otherwise.
 */
widget *widgetFindById(widget *self, const char *id);

/**
 * Returns the absolute position of the widget (ie, a position that is not
 * relative to any other widget.
 *
 * @param self  The widget to get the position of.
 * @return The absolute position of self.
 */
point widgetAbsolutePosition(widget *self);

/**
 * Returns the absolute bounding rectangle of the widget.
 *
 * @param self  The widget to get the bounds of.
 * @return The absolute bounds of self.
 */
rect widgetAbsoluteBounds(widget *self);

/**
 * Transverses up the hierarchy until it finds parent-less widget (known as
 * the root widget). A pointer to this widget is returned.
 *
 * @param self  The widget to find the root widget of.
 * @return A pointer to the root widget.
 */
widget *widgetGetRoot(widget *self);

/**
 * Attempts to add child as a child widget of self. The exact location of the
 * widget (as well as its dimensions) are decided based off of the min & max
 * sizes of the child.
 *
 * @param self  The widget to add the child widget to.
 * @param child The widget to be added.
 * @return true if child was successfully added, false otherwise.
 */
bool widgetAddChild(widget *self, widget *child);

/**
 * Attempts to remove child from the list of child widgets. If the child widget
 * is found anywhere in the hierarchy it is removed and its destructor called.
 *
 * A convenient way of using this methid is as follows:
 * widgetRemoveChild(self, widgetFindById(self, "id_to_remove"));
 *
 * @param self  The widget to remove child from.
 * @param child The child widget to remove.
 */
void widgetRemoveChild(widget *self, widget *child);

/**
 * Adds handler to self's event handler table, registering it to respond to
 * events of type. An unique id is assigned to the event when it is added. This
 * id can be used at a later date to widgetRemoveEventHandler to remove the
 * event.
 *
 * The userData pointer is passed verbatim to handler via the userData
 * parameter. If no user data is required then NULL can be passed.
 *
 * It is perfectly legal for there to be multiple event handlers installed for
 * a single event type. When this is the case the event handlers are fired in
 * the order in which they were added.
 *
 * @param self          The widget to add the event handler to.
 * @param type          The type of event that handler should respond to.
 * @param handler       The function to call when the event type fires.
 * @return The id of the newly added event.
 */
int widgetAddEventHandler(widget *self, eventType type,
                          callback handler, void *userData);

/**
 * Removes the event from the events table at offset id.
 *
 * @param self  The widget to remove the event handler from.
 * @param id    The id of the event to be removed.
 */
void widgetRemoveEventHandler(widget *self, int id);

/**
 * Enables the current widget along with all of its child widgets. If the
 * widget is currently enabled but one or more of its child widgets are not
 * then they will also be enabled.
 *
 * If, however, the parent widget is disabled then this method is effectively
 * a no-op.
 *
 * @param self  The widget to enable.
 */
void widgetEnable(widget *self);

/**
 * Disables the current widget along with all of its child widgets. If the
 * widget is currently disabled then this method is a no-op.
 *
 * @param self  The widget to disable.
 */
void widgetDisable(widget *self);

/**
 * Destroys the widget and frees *all* memory associated with it.
 *
 * @param self  The widget to destroy.
 */
void widgetDestroy(widget *self);

/**
 * Returns a pointer to the widget furthest down the hierarchy which currently
 * has focus. If self does not currently have focus then NULL is returned.
 * Should none of self's child widgets have focus (but it does) then self is
 * returned.
 *
 * @param self  The widget to get the further down focused child of.
 * @return A pointer to the widget, or NULL if self is not focused.
 */
widget *widgetGetCurrentlyFocused(widget *self);

/**
 * If the widget is capable of holding keyboard focus and does not currently
 * hold it then this method will bring it into focus. Should ->parent not be
 * focused then widgetFocus(self->parent) will be called to focus it.
 *
 * This method will also blur any widgets which will no longer be in focus as a
 * result of self being focued. In addition it takes responsibility for firing
 * the EVT_FOCUS callbacks for self.
 *
 * @param self  The widget to focus.
 */
void widgetFocus(widget *self);

/**
 * Blurs the current widget (removes keyboard focus from it). Before self is
 * blurred any child widget with focus is blurred first. Finally the EVT_BLUR
 * event handlers for self are fired.
 *
 * @param self  The widget to blur.
 */
void widgetBlur(widget *self);

/**
 * Returns the minimum size that the widget can be.
 *
 * @param self  The widget to return the miniumum size of.
 * @return The miniumum (x,y) size of the widget.
 */
size widgetGetMinSize(widget *self);

/**
 * Returns the maxiumum size that the widget can be.
 *
 * @param self  The widget to return the maximum size of.
 * @return The maximum (x,y) size of the widget.
 */
size widgetGetMaxSize(widget *self);

/**
 * Sets the size of the widget to (x,y). x and y are subject to the following
 * conditions:
 *  widgetGetMinSize().x <= x <= widgetGetMaxSize().x and
 *  widgetGetMinSize().y <= y <= widgetGetMaxSize().y.
 * 
 * @param self  The widget to resize.
 * @param w     The new size of the widget in the x-axis.
 * @param h     The new size of the widget in the y-axis.
 */
void widgetResize(widget *self, int w, int h);

/**
 * @TODO
 */
bool widgetHandleEvent(widget *self, event *evt);

/*
 * Protected methods
 */

/**
 * A protected `pure virtual' method which is called to draw the widget.
 *
 * @param self  The widget that should draw itself.
 */
void widgetDoDraw(widget *self);

/**
 *
 */
bool widgetDoLayout(widget *self);

/**
 * Fires all of the event handlers registered for evt->type on the widger self.
 *
 * @param self  The widget to fire the callbacks on.
 * @param evt   The event to fire the callbacks for.
 */
bool widgetFireCallbacks(widget *self, event *evt);

#endif /*WIDGET_H_*/
