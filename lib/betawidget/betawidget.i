%module betawidget
%{
extern "C" {
#include "widget.h"
#include "window.h"
#include "hBox.h"
#include "clipboard.h"
#include "spacer.h"
#include "textEntry.h"
#include "lua-wrap.h"
#include <assert.h>
}

typedef struct
{
	lua_State*      L;
	int             table;

#ifndef NDEBUG
	widget*         self;
	eventType       type;
	int             handlerId;
#endif
} lua_widget_callback;

static bool callbackHandler(widget* const self, const event* const evt, int const handlerId, lua_widget_callback const * const callbackRef)
{
	const int stack_top = lua_gettop(callbackRef->L);
	bool result;

	assert(self == callbackRef->self);
	assert(evt->type == callbackRef->type);
	assert(handlerId == callbackRef->handlerId);

	// callback.function(callback.weak.widget, evt, handlerId)
	lua_getref(callbackRef->L, callbackRef->table);
	lua_getfield(callbackRef->L, -1, "function");
	lua_getfield(callbackRef->L, -2, "weak");
	lua_getfield(callbackRef->L, -1, "widget");
	lua_replace(callbackRef->L, -2);
	lua_remove(callbackRef->L, -3);

	assert(self == (widget const *)((swig_lua_userdata*)lua_touserdata(callbackRef->L, -1))->ptr);

	swig_lua_userdata* const usr = (swig_lua_userdata*)lua_newuserdata(callbackRef->L, sizeof(*usr));
	usr->ptr = (event* const)evt;
	usr->type = SWIGTYPE_p__event;
	usr->own = 0;
	_SWIG_Lua_AddMetatable(callbackRef->L, SWIGTYPE_p__event);

	lua_pushnumber(callbackRef->L, handlerId);

	// return callback(self, evt, handlerId)
	if (lua_pcall(callbackRef->L, 3, 1, 0) != 0)
                return false;
	if (lua_gettop(callbackRef->L) != (stack_top + 1))
		// If no value got returned assume the event handler was succesfull
		return true;

	// Pop the result from the stack and return it
	result = lua_toboolean(callbackRef->L, -1);
	lua_pop(callbackRef->L, 1);
	return result;
}

static bool callbackDestructor(widget* const self, const event* const evt, int const handlerId, lua_widget_callback* const callbackRef)
{
	assert(self == callbackRef->self);
	assert(handlerId == callbackRef->handlerId);

	lua_unref(callbackRef->L, callbackRef->table);
	free(callbackRef);
	return true;
}
%}

%rename (getClipboardText) widgetGetClipboardText;
%rename (setClipboardText) widgetSetClipboardText;
%typemap (newfree) char * "free($1);";
%newobject widgetGetClipboardText;
%include "clipboard.h"

typedef enum
{
	LEFT,
	CENTRE,
	RIGHT
} hAlign;

typedef enum
{
	TOP,
	MIDDLE,
	BOTTOM
} vAlign;

typedef enum
{
	EVT_MOUSE_DOWN,
	EVT_MOUSE_UP,
	EVT_MOUSE_CLICK,
	EVT_MOUSE_ENTER,
	EVT_MOUSE_MOVE,
	EVT_MOUSE_LEAVE,
	EVT_KEY_DOWN,
	EVT_KEY_UP,
	EVT_TEXT,
	EVT_DRAG_BEGIN,
	EVT_DRAG_TRACK,
	EVT_DRAG_END,
	EVT_TIMER,
	EVT_TIMER_SINGLE_SHOT,
	EVT_TIMER_PERSISTENT,
	EVT_TOOL_TIP_SHOW,
	EVT_TOOL_TIP_HIDE,
	EVT_RESIZE,
	EVT_REPOSITION,
	EVT_FOCUS,
	EVT_BLUR,
	EVT_DESTRUCT
} eventType;

%rename (event) _event;
typedef struct _event
{
	int time;
	eventType type;
};

typedef struct _point
{
	float x;
	float y;
} point;

typedef point size;

%rename (widget) _widget;
struct _widget
{
        const char * const id;
	_point offset;

        %extend
        {
                _widget(const char* id)
                {
                        struct _widget * const self = (struct _widget*)malloc(sizeof(*self));
                        if (self == NULL)
                                return NULL;

                        widgetInit(self, id);
                        return self;
                }

                virtual ~_widget()
                {
                        widgetDestroy(WIDGET($self));
                }

                virtual bool    addChild(struct _widget *child)
                {
                        return widgetAddChild(WIDGET($self), child);
                }

                virtual void    removeChild(struct _widget *child)
                {
                        return widgetRemoveChild(WIDGET($self), child);
                }

                virtual void    removeEventHandler(int id)
                {
                        return widgetRemoveEventHandler(WIDGET($self), id);
                }

                virtual void    focus()
                {
                        return widgetFocus(WIDGET($self));
                }

                virtual void    blur()
                {
                        return widgetBlur(WIDGET($self));
                }

                virtual void    acceptDrag()
                {
                        return widgetAcceptDrag(WIDGET($self));
                }

                virtual void    declineDrag()
                {
                        return widgetDeclineDrag(WIDGET($self));
                }

                virtual void    enable()
                {
                        return widgetEnable(WIDGET($self));
                }

                virtual void    disable()
                {
                        return widgetDisable(WIDGET($self));
                }

                virtual void    show()
                {
                        return widgetShow(WIDGET($self));
                }

                virtual void    hide()
                {
                        return widgetHide(WIDGET($self));
                }

                virtual size    getMinSize()
                {
                        return widgetGetMinSize(WIDGET($self));
                }

                virtual size    getMaxSize()
                {
                        return widgetGetMaxSize(WIDGET($self));
                }

                virtual void    resize(int w, int h)
                {
                        return widgetResize(WIDGET($self), w, h);
                }

                virtual void    reposition(int x, int y)
                {
                        return widgetReposition(WIDGET($self), x, y);
                }
        }
};

%rename (window) _window;
struct _window : public _widget
{
        %extend
        {
                _window(const char* id, int width, int heigth)
                {
                        struct _window * const self = (struct _window*)malloc(sizeof(*self));
                        if (self == NULL)
                                return NULL;

                        windowInit(self, id, width, heigth);
                        return self;
                }

                virtual ~_window()
                {
                        widgetDestroy(WIDGET($self));
                }

                virtual void repositionFromScreen(hAlign hAlign, int xOffset, vAlign vAlign, int yOffset)
                {
                        return windowRepositionFromScreen($self, hAlign, xOffset, vAlign, yOffset);
                }

                virtual void repositionFromAnchor(const struct _window *anchor, hAlign hAlign, int xOffset, vAlign vAlign, int yOffset)
                {
                        return windowRepositionFromAnchor($self, anchor, hAlign, xOffset, vAlign, yOffset);
                }
        }
};

%rename (hBox) _hBox;
struct _hBox : public _widget
{
        %extend
        {
                _hBox(const char* id)
                {
                        return hBoxCreate(id);
                }

                virtual ~_hBox()
                {
                        widgetDestroy(WIDGET($self));
                }

                virtual void setVAlign(vAlign v)
                {
                        return hBoxSetVAlign($self, v);
                }
        }
};

%rename (spacer) _spacer;
struct _spacer : public _widget
{
        %extend
        {
                _spacer(const char* id, spacerDirection direction)
                {
                        return spacerCreate(id, direction);
                }

                virtual ~_spacer()
                {
                        widgetDestroy(WIDGET($self));
                }
        }
};

%rename (textEntry) _textEntry;
struct _textEntry : public _widget
{
        %extend
        {
                _textEntry(const char* id)
                {
                        return textEntryCreate(id);
                }

                virtual ~_textEntry()
                {
                        widgetDestroy(WIDGET($self));
                }

                virtual const char* getContents()
                {
                        return textEntryGetContents($self);
                }

                virtual bool setContents(const char* contents)
                {
                        return textEntrySetContents($self, contents);
                }
        }
};

%wrapper %{
static void createLuaCallbackTable(lua_State* const L)
{
	// Create a table to hold the function and a weak table with additional data
	lua_newtable(L); // callback = {}
	lua_newtable(L); // weak = {}
	// setmetatable(weak, { __mode = 'v' })
	lua_newtable(L);
	lua_pushstring(L, "v");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);
	// callback.weak = weak
	lua_setfield(L, -2, "weak");
}

static int lua_widget_addEventHandler(lua_State* const L)
{
	int args = 0;
        widget* self;
        eventType type;
        int handlerId;
	lua_widget_callback* callbackRef = 0;

        SWIG_check_num_args("widgetAddEventHandler", 3, 3)
        if (!SWIG_isptrtype(L, 1))
                SWIG_fail_arg("widgetAddEventHandler", 1, "_widget *");
        if (!lua_isnumber(L, 2))
                SWIG_fail_arg("widgetAddEventHandler", 2, "eventType");
	if (!lua_isfunction(L, 3))
                SWIG_fail_arg("widgetAddEventHandler", 3, "function");

	// value = function = -1

	// Allocate a chunk of memory to place a reference to the function in
	callbackRef = (lua_widget_callback*)malloc(sizeof(*callbackRef));
	if (callbackRef == NULL)
	{
		lua_pushstring(L, "Error in widgetAddEventHandler: Out of memory");
		goto fail;
	}

        if (!SWIG_IsOK(SWIG_ConvertPtr(L, 1, (void**)&self, SWIGTYPE_p__widget,0)))
        {
                SWIG_fail_ptr("widget_addEventHandler",1,SWIGTYPE_p__widget);
        }
	type = (eventType)lua_tonumber(L, 2);

	// Create a table to store the Lua callback info in
	createLuaCallbackTable(L);
	lua_insert(L, 1);

	// callback.function = function
	lua_setfield(L, 1, "function");

	// callback.weak.widget = self
	lua_pop(L, 1);
	lua_getfield(L, 1, "weak");
	lua_insert(L, -2);
	lua_setfield(L, -2, "widget");

	// Retrieve a reference to the callback table
	lua_settop(L, 1);
	callbackRef->L     = L;
	callbackRef->table = lua_ref(L, true);

	// Register the event handler with the widget
	handlerId = widgetAddEventHandler(WIDGET(self), type, (callback)callbackHandler, (callback)callbackDestructor, callbackRef);
	lua_pushnumber(L, handlerId); ++args;

#ifndef NDEBUG
        callbackRef->self      = self;
        callbackRef->type      = type;
        callbackRef->handlerId = handlerId;
#endif

	return args;

fail:
        if (callbackRef)
                free(callbackRef);

	lua_error(L);
	return args;
}

static int lua_widget_addTimerEventHandler(lua_State* const L)
{
	int args = 0;
        widget* self;
        eventType type;
        int handlerId;
        int interval;
	lua_widget_callback* callbackRef = 0;

        SWIG_check_num_args("addTimerEventHandler", 4, 4)
        if (!SWIG_isptrtype(L, 1))
                SWIG_fail_arg("addTimerEventHandler", 1, "_widget *");
        if (!lua_isnumber(L, 2))
                SWIG_fail_arg("addTimerEventHandler", 2, "eventType");
        if (!lua_isnumber(L, 3))
                SWIG_fail_arg("addTimerEventHandler", 3, "int");
	if (!lua_isfunction(L, 4))
                SWIG_fail_arg("addTimerEventHandler", 4, "function");

	// value = function = -1

	// Allocate a chunk of memory to place a reference to the function in
	callbackRef = (lua_widget_callback*)malloc(sizeof(*callbackRef));
	if (callbackRef == NULL)
	{
		lua_pushstring(L, "Error in addTimerEventHandler: Out of memory");
		goto fail;
	}

        if (!SWIG_IsOK(SWIG_ConvertPtr(L, 1, (void**)&self, SWIGTYPE_p__widget,0)))
        {
                SWIG_fail_ptr("widget_addTimerEventHandler",1,SWIGTYPE_p__widget);
        }

	type = (eventType)lua_tonumber(L, 2);
        interval = lua_tonumber(L, 3);

	// Create a table to store the Lua callback info in
	createLuaCallbackTable(L);
	lua_insert(L, 1);

	// callback.function = function
	lua_setfield(L, 1, "function");

	// callback.weak.widget = self
	lua_pop(L, 2);
	lua_getfield(L, 1, "weak");
	lua_insert(L, -2);
	lua_setfield(L, -2, "widget");

	// Retrieve a reference to the callback table
	lua_settop(L, 1);
	callbackRef->L     = L;
	callbackRef->table = lua_ref(L, true);

	// Register the event handler with the widget
	handlerId = widgetAddTimerEventHandler(WIDGET(self), type, interval, (callback)callbackHandler, (callback)callbackDestructor, callbackRef);
	lua_pushnumber(L, handlerId); ++args;

#ifndef NDEBUG
        callbackRef->self      = self;
        callbackRef->type      = type;
        callbackRef->handlerId = handlerId;
#endif

	return args;

fail:
        if (callbackRef)
                free(callbackRef);

	lua_error(L);
	return args;
}

static bool lua_inherits_From(swig_lua_class const * const clss, swig_lua_class const * const from)
{
        int i;
        if (clss == from)
                return true;

        for (i = 0; clss->bases[i]; ++i)
        {
                if (lua_inherits_From(clss->bases[i], from))
                        return true;
        }

        return false;
}

static void addLuaFuncToSwigBaseClass(lua_State* const L,
                                      swig_lua_class const * const base,
                                      swig_type_info const * const * const classes,
                                      int const class_count,
                                      const char * const name,
                                      lua_CFunction const func)
{
        int i;
        for (i = 0; i < class_count; ++i)
        {
                swig_lua_class const * const clss = (swig_lua_class const *)classes[i]->clientdata;

                if (clss
                 && lua_inherits_From(clss, base))
                {
                        SWIG_Lua_get_class_metatable(L, clss->name);
                        SWIG_Lua_get_table(L, ".fn"); /* find the .fn table */
                        SWIG_Lua_add_function(L, name, func);
                        lua_pop(L,1);       /* tidy stack (remove table) */
                        lua_pop(L,1);      /* tidy stack (remove class metatable) */
                }
        }
}
%}

%init %{
        static swig_type_info const * const * const classes = swig_type_initial;
        static int const class_count = sizeof(swig_type_initial) / sizeof(swig_type_initial[0]);

        static const struct
        {
                swig_lua_class const * const    base;
                const char * const              name;
                lua_CFunction                   func;
        } methods[] =
        // Table of custom crafted virtual functions
        {
                { &_wrap_class__widget, "addEventHandler",      lua_widget_addEventHandler },
                { &_wrap_class__widget, "addTimerEventHandler", lua_widget_addTimerEventHandler },
        };

        int i;
        for (i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        {
                addLuaFuncToSwigBaseClass(L, methods[i].base, classes, class_count, methods[i].name, methods[i].func);
        }
%}
