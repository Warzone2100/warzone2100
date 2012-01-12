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

#ifndef WIDGET_H_
#define WIDGET_H_

// TODO: Make this cross platform (MSVC)
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "internal-cairo.h"

#include "../../ivis_opengl/GLee.h"

#include "vector.h"
#include "geom.h"

#include "keycode.h"
#include "clipboard.h"
#include "font.h"

/*
 * Forward declarations
 */
typedef struct _classInfo classInfo;

typedef struct _widget widget;
typedef struct _widgetVtbl widgetVtbl;

typedef struct _event           event;
typedef struct _eventMouse      eventMouse;
typedef struct _eventMouseBtn   eventMouseBtn;
typedef struct _eventKey        eventKey;
typedef struct _eventText		eventText;
typedef struct _eventTimer      eventTimer;
typedef struct _eventToolTip    eventToolTip;
typedef struct _eventReposition eventReposition;
typedef struct _eventResize     eventResize;
typedef struct _eventMisc       eventMisc;

/**
 * Function signature for event handler callbacks. All callback functions must
 * be of this form.
 *
 * @param self  The widget that received/handled the event.
 * @param evt   A pointer to the event structure. Depending on the value of
 *              evt->type it may be necessary to cast this to derived event
 *              structure (e.g., evtMouse or evtMisc).
 * @param handlerId The (unique) id of this event handler. This can be used to:
 *                   - Remove the event handler from the widgets event table;
 *                     which can be done by calling widgetRemoveEventHandler.
 *                     This will result in the event handlers destruct method
 *                     being called, so long as such a method exists.
 *                   - Set the *userData pointer by using
 *                     widgetSetEventHandlerUserData.
 * @param userData  The user-data associated with the callback; this is stored
 *                  and passed verbatim.
 * @return True if the callback executed without error, otherwise false.
 */
typedef bool (*callback)        (widget *self, const event *evt, int handlerId,
                                 void *userData);

typedef struct _eventTableEntry eventTableEntry;

/**
 * Information about the `type' (class) of a widget
 */
struct _classInfo
{
	const struct _classInfo *parentType;
	const char *ourType;
};

/**
 * The valid event types
 */
typedef enum
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

	// Text input events
	EVT_TEXT,

	// Drag events
	EVT_DRAG_BEGIN,
	EVT_DRAG_TRACK,
	EVT_DRAG_END,

	// Timer events
	EVT_TIMER,
	EVT_TIMER_SINGLE_SHOT,
	EVT_TIMER_PERSISTENT,

	// Tool-tip events
	EVT_TOOL_TIP_SHOW,
	EVT_TOOL_TIP_HIDE,

	// Resize and reposition
	EVT_RESIZE,
	EVT_REPOSITION,

	// Misc
	EVT_FOCUS,
	EVT_BLUR,
	EVT_ENABLE,
	EVT_DISABLE,

	// Destroy
	EVT_DESTRUCT
} eventType;

/**
 * The possible mouse states as understood by the events system
 */
typedef enum
{
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_WHEEL_UP,
	BUTTON_WHEEL_DOWN,
	BUTTON_OTHER
} mouseButton;

/**
 * Possible drag states
 */
typedef enum
{
	/// No active drag
	DRAG_NONE,

	/// Drag offer pending acceptance/declination
	DRAG_PENDING,

	/// Offer was accepted
	DRAG_ACCEPTED,

	/// Offer was declined
	DRAG_DECLINED,

	/// Drag is currently active
	DRAG_ACTIVE
} dragStates;

/*
 * Event structures
 */


/**
 * The 'base' event structure. All events can be cast to this
 */
struct _event
{
	// The time at which the event took place
	int time;

	// The type of the event
	eventType type;
};

/**
 * The event structure used for mouse motion events
 */
struct _eventMouse
{
	struct _event event;

	/// Location of the mouse
	point loc;

	/// Previous location of the mouse
	point previousLoc;
};

/**
 * The event structure used for mouse button events
 */
struct _eventMouseBtn
{
	struct _event event;

	/// Location
	point loc;

	/// Button pressed
	mouseButton button;
};

/**
 * The event structure used for keyboard events
 */
struct _eventKey
{
	struct _event event;

	/// The keycode of the key which was pressed
	eventKeycode keycode;

	/// Active modifier keys
	bool ctrl;
	bool shift;
	bool alt;
};

/**
 * The event structure for text input events
 */
struct _eventText
{
	struct _event event;

	/// The text that was typed, UTF-8 encoded
	const char *utf8;
};

/**
 * The event structure for timer events
 */
struct _eventTimer
{
	struct _event event;
};

/**
 * The event structure for tool-tip events
 */
struct _eventToolTip
{
	struct _event event;

	widget *target;
};

/**
 * The event structure for resize events
 */
struct _eventResize
{
	struct _event event;

	/// The previous size of the widget
	size oldSize;
};

/**
 * The event structure for reposition events
 */
struct _eventReposition
{
	struct _event event;

	/// The previous position of the widget relative to its parent
	point oldPosition;

	/// The previous position of the widget relative to the screen
	point oldAbsolutePosition;
};

/**
 * The event structure for miscellaneous events
 */
struct _eventMisc
{
	struct _event event;
};

/**
 * Event table structure
 */
struct _eventTableEntry
{
	/// The unique id of the event handler
	int id;

	/// The event for which the handler is registered for
	eventType type;

	/// The method to call
	bool (*callback)(widget*, const event*, int, void*);

	/// The method to call when removing the event handler
	bool (*destructor)(widget*, const event*, int, void*);

	/// Pointer to user supplied data to pass to callback
	void *userData;

	/// The time when the event was last called (for debugging and timer events)
	int lastCalled;

	/// For timer events only; how often the event should fire; in ms
	int interval;
};

/**
 * Possible types of widget animation
 */
typedef enum
{
	ANI_TYPE_TRANSLATE,
	ANI_TYPE_ROTATE,
	ANI_TYPE_SCALE,
	ANI_TYPE_ALPHA,

	/// Must be the last member
	ANI_TYPE_COUNT
} animationType;

typedef struct
{
	/// The animation type represented by this keyframe
	animationType type;

	/// The time this keyframe represents in ms
	int time;

	/// Animation specific information
	union
	{
		/// Where to translate ourself to, relative to our parent
		point translate;

		/// Number of degrees to rotate the widget by
		float rotate;

		/// Scale factor to scale the widget by
		size scale;

		/// Alpha value to use
		float alpha;
	} data;
} animationFrame;

/**
 * The widget classes virtual method table
 */
struct _widgetVtbl
{
	bool    (*handleEvent)                  (widget *self, const event *evt);

	bool    (*addChild)                     (widget *self, widget *child);
	void    (*removeChild)                  (widget *self, widget *child);

	bool    (*fireCallbacks)                (widget *self, const event *evt);
	bool    (*fireTimerCallbacks)           (widget *self, const event *evt);

	int     (*addEventHandler)              (widget *self, eventType type,
	                                         callback handler,
	                                         callback destructor,
	                                         void *userData);
	int     (*addTimerEventHandler)         (widget *self, eventType type,
	                                         int interval, callback handler,
	                                         callback destructor,
	                                         void *userData);
	void    (*removeEventHandler)           (widget *self, int id);

	point   (*animationInterpolateTranslate)    (widget *self,
	                                             animationFrame k1,
	                                             animationFrame k2,
	                                             int time);

	float   (*animationInterpolateRotate)       (widget *self,
	                                             animationFrame k1,
	                                             animationFrame k2,
	                                             int time);

	point   (*animationInterpolateScale)        (widget *self,
	                                             animationFrame k1,
	                                             animationFrame k2,
	                                             int time);

	float   (*animationInterpolateAlpha)        (widget *self,
	                                             animationFrame k1,
	                                             animationFrame k2,
	                                             int time);


	void    (*focus)                        (widget *self);
	void    (*blur)                         (widget *self);

	void    (*acceptDrag)                   (widget *self);
	void    (*declineDrag)                  (widget *self);

	void    (*enable)                       (widget *self);
	void    (*disable)                      (widget *self);

	void    (*show)                         (widget *self);
	void    (*hide)                         (widget *self);

	size    (*getMinSize)                   (widget *self);
	size    (*getMaxSize)                   (widget *self);

	void    (*resize)                       (widget *self, int w, int h);
	void    (*reposition)                   (widget *self, int x, int y);

	void    (*enableGL)                     (widget *self);
	void    (*disableGL)                    (widget *self);

	void    (*beginGL)                      (widget *self);
	void    (*endGL)                        (widget *self,
	                                         bool finishedDrawing);

	void    (*composite)                    (widget *self);

	void    (*doDraw)                       (widget *self);
	void	(*doDrawMask)                   (widget *self);
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
	 * The tool-tip of the widget
	 */
	const char *toolTip;

	/*
	 * If the widgets tool-tip is currently visible
	 */
	bool toolTipVisible;

	/*
	 * Current drag state
	 */
	dragStates dragState;

	/*
	 * The widgets cairo drawing context
	 */
	cairo_t *cr;

	/*
	 * The id of the OpenGL texture to which self->cr is mapped
	 */
	GLuint textureId;

	/*
	 * The id of the OpenGL framebuffer used for rendering GL content
	 */
	GLuint fboId;

	/*
	 * The id of the OpenGL depthbuffer attached to the FBO
	 */
	GLuint depthbufferId;

	/*
	 * The widgets mouse-event mask
	 */
	cairo_t *maskCr;

	//--------------------------------------
	// Public members
	//--------------------------------------

	/*
	 * The id of the widget
	 */
	const char *id;

	/*
	 * The class (or subclass) that widget is (used for type checking)
	 */
	const struct _classInfo *classInfo;

	/*
	 * Arbitrary user-defined data
	 */
	void *pUserData;
	int32_t userData;

	/*
	 * The offset of the widget relative to its parent
	 */
	point offset;

	/*
	 * How many degrees to rotate the widget about the z-axis
	 */
	float rotate;

	/*
	 * How much to scale the widget by in the x- and y-axis; the deformation is
	 * non vector
	 */
	point scale;

	/*
	 * Alpha value to multiply the cairo context by when compositing
	 */
	float alpha;

	/*
	 * The size of the widget
	 */
	struct _point size;

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
	 * If the widget is visible or not
	 */
	bool isVisible;

	/*
	 * If the widget is dirty (i.e., needs to be re-drawn)
	 */
	bool needsRedraw;

	/*
	 * If the widget uses an mouse event mask
	 */
	bool maskEnabled;

	/*
	 * If the widget supports OpenGL content
	 */
	bool openGLEnabled;

	/*
	 * If we are in a widgetBeginGL ... widgetEndGL block
	 */
	bool openGLInProgress;

	/*
	 * If the texture needs syncing with the Cairo context
	 */
	bool textureNeedsUploading;
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
bool widgetFireCallbacksImpl(widget *self, const event *evt);
bool widgetFireTimerCallbacksImpl(widget *self, const event *evt);
int widgetAddEventHandlerImpl(widget *self, eventType type,
                              callback handler, callback destructor,
                              void *userData);
int widgetAddTimerEventHandlerImpl(widget *self, eventType type, int interval,
                                   callback handler, callback destructor,
                                   void *userData);
void widgetRemoveEventHandlerImpl(widget *self, int id);
point widgetAnimationInterpolateTranslateImpl(widget *self, animationFrame k1,
                                              animationFrame k2, int time);
float widgetAnimationInterpolateRotateImpl(widget *self, animationFrame k1,
                                           animationFrame k2, int time);
point widgetAnimationInterpolateScaleImpl(widget *self, animationFrame k1,
                                          animationFrame k2, int time);
float widgetAnimationInterpolateAlphaImpl(widget *self, animationFrame k1,
										  animationFrame k2, int time);
void widgetAcceptDragImpl(widget *self);
void widgetDeclineDragImpl(widget *self);
void widgetEnableImpl(widget *self);
void widgetDisableImpl(widget *self);
void widgetShowImpl(widget *self);
void widgetHideImpl(widget *self);
void widgetFocusImpl(widget *self);
void widgetBlurImpl(widget *self);
void widgetResizeImpl(widget *self, int w, int h);
void widgetRepositionImpl(widget *self, int x, int y);
bool widgetHandleEventImpl(widget *self, const event *evt);
void widgetCompositeImpl(widget *self);
void widgetEnableGLImpl(widget *self);
void widgetDisableGLImpl(widget *self);
void widgetBeginGLImpl(widget *self);
void widgetEndGLImpl(widget *self, bool finishedDrawing);

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
bool widgetIsA(const widget *self, const classInfo *instanceOf);

/**
 * Creates and fills out a new event structure using the information provided.
 * This is strictly a convenience method to make it easier to generate new
 * events.
 *
 * The event structure returned is the building block of all other, more complex
 * events (such as mouse and keyboard events).
 *
 * @param type  The type of the newly created event.
 * @return A complete event structure.
 */
event widgetCreateEvent(eventType type);

/*
 * Public static, implementation defined methods
 */

/**
 * This method should return the number of milliseconds since an undefined
 * epoch.
 *
 * @return The number of milliseconds since the epoch.
 */
int widgetGetTime(void);

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
 * Composites the widget self onto the frame-buffer. In addition this method is
 * also responsible for transforming the widget for the purposes of animation.
 * Finally this method will loop over the child widgets of self and call
 * widgetComposite on each one.
 *
 * @param self  The widget (along with its children to composite.
 */
void widgetComposite(widget *self);

/**
 * Enables the widgets mask.
 *
 * @param self  The widget whose mask to enable.
 */
void widgetEnableMask(widget *self);

/**
 * Disables the widgets mouse-event mask.
 *
 * @param self	The widget whose mask to disable.
 */
void widgetDisableMask(widget *self);

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
point widgetAbsolutePosition(const widget *self);

/**
 * Returns the absolute bounding rectangle of the widget.
 *
 * @param self  The widget to get the bounds of.
 * @return The absolute bounds of self.
 */
rect widgetAbsoluteBounds(const widget *self);

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
 * A convenient way of using this method is as follows:
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
 * @param destructor    The function to call when the event handler is removed.
 * @param userData      User specified data pointer to pass to handler.
 * @return The id of the newly added event.
 */
int widgetAddEventHandler(widget *self, eventType type, callback handler,
                          callback destructor, void *userData);

/**
 * Similar to widgetAddEventHandler in many respects, except that it is designed
 * to add timer event handlers (EVT_TIMER_SINGLE_SHOT and EVT_TIMER_PERSISTENT).
 *
 * @param self          The widget to add the timer event handler to.
 * @param type          The type of the timer to register the handler for.
 * @param interval      The duration in ms to wait.
 * @param handler       The function to call when the event fires.
 * @param destructor    The function to call when the event handler is removed.
 * @param userData      User specified data pointer to pass to handler.
 * @return The id of the newly added event.
 */
int widgetAddTimerEventHandler(widget *self, eventType type, int interval,
                               callback handler, callback destructor,
                               void *userData);

/**
 * Removes the event from the events table at offset id.
 *
 * @param self  The widget to remove the event handler from.
 * @param id    The id of the event to be removed.
 */
void widgetRemoveEventHandler(widget *self, int id);

/**
 * Returns if id is that of a valid event handler.
 *
 * @param self  The widget to check if id is a valid event handler for.
 * @param id    The id in question.
 */
bool widgetIsEventHandler(const widget *self, int id);

/**
 * Returns the user-data for the event whose id is id.
 *
 * @param self  The widget to whom the event handler is registered to.
 * @param id    The id of the event handler to fetch the user-data for.
 * @return The user-data for the event handler, or NULL if the eventId is
 *         invalid.
 */
void *widgetGetEventHandlerUserData(const widget *self, int id);

/**
 * Sets the user-data for the event handler with an id of id registered to self
 * to userData.
 *
 * @param self  The widget to whom the event handler is registered to.
 * @param id    The id of the widget to set the user-data for.
 * @param userData  The new user-data for the event handler
 */
void widgetSetEventHandlerUserData(widget *self, int id, void *userData);

/**
 * Accepts the current drag offer, if any. Should no drag offer be on the table
 * then the results are undefined.
 *
 * @param self  The widget to accept the drag offer for.
 */
void widgetAcceptDrag(widget *self);

/**
 * Declines the current drag offer. Like with widgetAcceptDrag the results are
 * undefined if there is no drag offer .
 *
 * @param self  The widget to decline the drag offer for.
 */
void widgetDeclineDrag(widget *self);

/**
 * TODO
 */
int widgetAddAnimation(widget *self, int nframes,
                       const animationFrame *frames);

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
 * Shows the current widget (makes it visible). This is a no-op if the widget is
 * already shown.
 *
 * @param self  The widget to show.
 */
void widgetShow(widget *self);

/**
 * Hides the current widget. Again, this is a no-op if the widget is already
 * hidden.
 *
 * @param self  The widget to hide.
 */
void widgetHide(widget *self);

/**
 * Destroys the widget and frees *all* memory associated with it.
 *
 * @param self  The widget to destroy.
 */
void widgetDestroy(widget *self);

/**
 * Returns a pointer to the widget farthest down the hierarchy which currently
 * has focus. If self does not currently have focus then NULL is returned.
 * Should none of self's child widgets have focus (but it does) then self is
 * returned.
 *
 * @param self  The widget to get the farthest down focused child of.
 * @return A pointer to the widget, or NULL if self is not focused.
 */
widget *widgetGetCurrentlyFocused(widget *self);

/**
 * Very much the same as widgetGetCurrentlyFocused except that it returns a
 * pointer to the widget farthest down the hierarchy which currently has the
 * mouse over it. Like with widgetGetCurrentlyFocused should self not have the
 * mouse over it, NULL is returned.
 *
 * @param self  The widget to get the farthest down moused-over child of.
 * @param A pointer to the widget, or NULL if self does not have the mouse.
 */
widget *widgetGetCurrentlyMousedOver(widget *self);

/**
 * If the widget is capable of holding keyboard focus and does not currently
 * hold it then this method will bring it into focus. Should ->parent not be
 * focused then widgetFocus(self->parent) will be called to focus it.
 *
 * This method will also blur any widgets which will no longer be in focus as a
 * result of self being focused. In addition it takes responsibility for firing
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
 * @param self  The widget to return the minimum size of.
 * @return The minimum (x,y) size of the widget.
 */
size widgetGetMinSize(widget *self);

/**
 * Returns the maximum size that the widget can be.
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
 * This method should not be invoked directly on non-root widget; setting the
 * dimensions of a widget remains the exclusive responsibility of the parent.
 *
 * @param self  The widget to resize.
 * @param w     The new size of the widget in the x-axis.
 * @param h     The new size of the widget in the y-axis.
 */
void widgetResize(widget *self, int w, int h);

/**
 * Sets the offset of the widget, relative to its parent widget to (x,y).
 *
 * @param self  The widget to reposition.
 * @param w     The new x-offset of the widget.
 * @param h     The new y-offset of the widget.
 */
void widgetReposition(widget *self, int x, int y);

/**
 * This is the main event dispatching function. Its purpose is to take an event,
 * evt and decide what course of action needs to be taken (if any). It is
 * responsible for setting widget-states such as hasMouse and hasFocus and for
 * deciding if evt is relevant to any child widgets of self.
 *
 * @param self  The widget to handle the event.
 * @param evt   The event itself.
 * @param True if the event was `handled', false otherwise.
 */
bool widgetHandleEvent(widget *self, const event *evt);

/**
 * Sets the tool-tip for the widget to tip. Since this method makes a copy of
 * tip there is no need for the caller to retain it. Should one wish to disable
 * tool-tips for this widget NULL should be passed in-place of tip.
 *
 * @param self  The widget to set the tool-tip for.
 * @param tip   The tool-tip to set for the widget.
 */
void widgetSetToolTip(widget *self, const char *tip);

/**
 * Returns a pointer to the widgets tool-tip text. Should the widget not have a
 * tool-tip, NULL is returned.
 *
 * @param self  The widget to get the tool-tip for.
 * @return A pointer to the widgets tool-tip.
 */
const char *widgetGetToolTip(widget *self);

/**
 * Allows the widget to host OpenGL content. This is done by creating a
 * framebuffer object for the widget which is then used for rendering.
 *
 * If GL support is already enabled then this function is a no-op.
 *
 * @param self  The widget to enable OpenGL rendering on.
 */
void widgetEnableGL(widget *self);

/**
 * Disables OpenGL content support for the widget.
 *
 * If GL support is not enabled then this function is a no-op.
 *
 * @param self  The widget to disable OpenGL rendering on.
 */
void widgetDisableGL(widget *self);

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
 * Configures the mask context (self->maskCr) for drawing and then delegates the
 * drawing to widgetDoDrawMask. This method is required as the mask context
 * requires some additional initialisation to a regular Cairo context.
 *
 * @param self  The widget whose mask to draw.
 */
void widgetDrawMask(widget *self);

/**
 * A protected `pure virtual` method which is called to draw the widgets mouse-
 * event mask.
 *
 * @param self  The widget that should draw its mask.
 */
void widgetDoDrawMask(widget *self);

/**
 * A protected `pure virtual' method which is called to lay out any child
 * widgets of self.
 *
 * This method may fail (return false) if self is not large enough to hold all
 * of its children.
 *
 * @param self  The widget whose children should be layed out.
 * @return True if the widgets children were successfully layed out, false
 *         otherwise.
 */
bool widgetDoLayout(widget *self);

/**
 * Fires all of the event handlers registered for evt->type on the widget self.
 *
 * @param self  The widget to fire the callbacks on.
 * @param evt   The event to fire the callbacks for.
 */
bool widgetFireCallbacks(widget *self, const event *evt);

/**
 * Fires all of the timer event handles for the widget self.
 *
 * @param self  The widget to fire the timer callbacks on.
 * @param evt   The event to fire the callbacks for.
 */
bool widgetFireTimerCallbacks(widget *self, const event *evt);

/**
 * Checks to see if the point loc is masked or not by the widgets mouse-event
 * mask.
 *
 * @param self  The widget to check the mask of.
 * @param loc   The point (x,y) to check the mask status of.
 * @return true if loc is masked; false otherwise;
 */
bool widgetPointMasked(const widget *self, point loc);

/**
 * Asks the root widget to show the tool-tip of self.
 *
 * @param self  The widget to show the tool-tip for.
 */
void widgetShowToolTip(widget *self);

/**
 * Asks the root widget to hide the tool-tip of self.
 *
 * @param self  The widget to hide the tool-tip for.
 */
void widgetHideToolTip(widget *self);

/**
 * Sets up the widget for OpenGL rendering. Any OpenGL calls made after a call
 * to this function (but before a call to widgetEndGL) will affect the widgets
 * local framebuffer object (FBO).
 *
 * This function takes care of storing the current OpenGL states and updating
 * the viewport and projection matrix so that the framebuffer is ready for use.
 *
 * It is illegal to call this function on a widget that does not have OpenGL
 * rendering enabled (self->openGLEnabled). Furthermore, any call to
 * widgetBeginGL must be accompanied by a call to widgetEndGL.
 *
 * @param self  The widget to begin rendering GL content to.
 */
void widgetBeginGL(widget *self);

/**
 * Finishes any active OpenGL rendering to the widget. It is analogous to glEnd.
 * If finishedDrawing is false then the texture (which the FBO has rendered to)
 * will be copied back to the Cairo context to allow for further drawing. This
 * has performance implications.
 *
 * Otherwise, if it is true then the is no texture copy, however the result of
 * attempting to use the Cairo context is undefined.
 *
 * @param self  The widget to end GL rendering on.
 * @param finishedDrawing   If any further drawing is to be performed on the
 *                          widget.
 */
void widgetEndGL(widget *self, bool finishedDrawing);

/**
 * TODO
 */
point widgetAnimationInterpolateTranslate(widget *self,
                                          animationFrame k1, animationFrame k2,
                                          int time);

/**
 * TODO
 */
float widgetAnimationInterpolateRotate(widget *self,
                                       animationFrame k1, animationFrame k2,
                                       int time);

/**
 * TODO
 */
point widgetAnimationInterpolateScale(widget *self,
                                      animationFrame k1, animationFrame k2,
                                      int time);

/**
 * TODO
 */
float widgetAnimationInterpolateAlpha(widget *self,
                                      animationFrame k1, animationFrame k2,
                                      int time);

#endif /*WIDGET_H_*/
