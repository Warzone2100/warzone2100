/*
	This file is part of Warzone 2100.
	Copyright (C) 2013  Warzone 2100 Project

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
/**
 * @file main_sdl.cpp
 *
 * SDL backend code
 */

#include <QtGui/QApplication>
#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"
#include "lib/framework/utf.h"
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/gamelib/gtime.h"
#include "src/warzoneconfig.h"
#include <SDL.h>
#include <QtCore/QSize>
#include <QtCore/QString>
#include "scrap.h"
#include "wz2100icon.h"
#include "cursors_sdl.h"
#include <algorithm>

extern void mainLoop();
// used in crash reports & version info
const char *BACKEND="SDL";

int realmain(int argc, char *argv[]);
int main(int argc, char *argv[])
{
	return realmain(argc, argv);
}

unsigned                screenWidth = 0;   // Declared in frameint.h.
unsigned                screenHeight = 0;  // Declared in frameint.h.
static unsigned         screenDepth = 0;
static SDL_Surface *    screen = NULL;

QCoreApplication *appPtr;


/* The possible states for keys */
enum KEY_STATE
{
	KEY_UP,
	KEY_PRESSED,
	KEY_DOWN,
	KEY_RELEASED,
	KEY_PRESSRELEASE,	// When a key goes up and down in a frame
	KEY_DOUBLECLICK,	// Only used by mouse keys
	KEY_DRAG			// Only used by mouse keys
};

struct INPUT_STATE
{
	KEY_STATE state; /// Last key/mouse state
	UDWORD lastdown; /// last key/mouse button down timestamp
	Vector2i pressPos;    ///< Location of last mouse press event.
	Vector2i releasePos;  ///< Location of last mouse release event.
};

/// constant for the interval between 2 singleclicks for doubleclick event in ms
#define DOUBLE_CLICK_INTERVAL 250

/* The current state of the keyboard */
static INPUT_STATE aKeyState[KEY_MAXSCAN];

/* The current location of the mouse */
static Uint16 mouseXPos = 0;
static Uint16 mouseYPos = 0;
static bool mouseInWindow = true;

/* How far the mouse has to move to start a drag */
#define DRAG_THRESHOLD	5

/* Which button is being used for a drag */
static MOUSE_KEY_CODE dragKey;

/* The start of a possible drag by the mouse */
static int dragX = 0;
static int dragY = 0;

/* The current mouse button state */
static INPUT_STATE aMouseState[MOUSE_BAD];
static MousePresses mousePresses;

/* The size of the input buffer */
#define INPUT_MAXSTR	512

/* The input string buffer */
struct InputKey
{
	UDWORD key;
	utf_32_char unicode;
};
static InputKey	pInputBuffer[INPUT_MAXSTR];
static InputKey	*pStartBuffer, *pEndBuffer;

/**************************/
/***     Misc support   ***/
/**************************/

/* Put a character into a text buffer overwriting any text under the cursor */
QString wzGetSelection()
{
	QString retval;
	static char* scrap = NULL;
	int scraplen;

	get_scrap(T('T','E','X','T'), &scraplen, &scrap);
	if (scraplen > 0)
	{
		retval = QString::fromUtf8(scrap);
	}
	return retval;
}


#if defined(WZ_WS_X11)

#include <GL/glx.h> // GLXDrawable
// X11 polution
#ifdef Status
#undef Status
#endif // Status
#ifdef CursorShape
#undef CursorShape
#endif // CursorShape
#ifdef Bool
#undef Bool
#endif // Bool

#ifndef GLX_SWAP_INTERVAL_EXT
#define GLX_SWAP_INTERVAL_EXT 0x20F1
#endif // GLX_SWAP_INTERVAL_EXT

// Need this global for use case of only having glXSwapIntervalSGI
static int swapInterval = -1;

void wzSetSwapInterval(int interval)
{
	typedef void (* PFNGLXQUERYDRAWABLEPROC) (Display *, GLXDrawable, int, unsigned int *);
	typedef void ( * PFNGLXSWAPINTERVALEXTPROC) (Display*, GLXDrawable, int);
	typedef int (* PFNGLXGETSWAPINTERVALMESAPROC)(void);
	typedef int (* PFNGLXSWAPINTERVALMESAPROC)(unsigned);
	typedef int (* PFNGLXSWAPINTERVALSGIPROC) (int);
	PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT;
	PFNGLXQUERYDRAWABLEPROC glXQueryDrawable;
	PFNGLXGETSWAPINTERVALMESAPROC glXGetSwapIntervalMESA;
	PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA;
	PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;

	if (interval < 0)
			interval = 0;

#if GLX_VERSION_1_2
	// Hack-ish, but better than not supporting GLX_SWAP_INTERVAL_EXT?
	GLXDrawable drawable = glXGetCurrentDrawable();
	Display * display =  glXGetCurrentDisplay();
	glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC) SDL_GL_GetProcAddress("glXSwapIntervalEXT");
	glXQueryDrawable = (PFNGLXQUERYDRAWABLEPROC) SDL_GL_GetProcAddress("glXQueryDrawable");

	if (glXSwapIntervalEXT && glXQueryDrawable && drawable)
	{
		unsigned clampedInterval;
		glXSwapIntervalEXT(display, drawable, interval);
		glXQueryDrawable(display, drawable, GLX_SWAP_INTERVAL_EXT, &clampedInterval);
		swapInterval = clampedInterval;
		return;
	}
#endif

	glXSwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC) SDL_GL_GetProcAddress("glXSwapIntervalMESA");
	glXGetSwapIntervalMESA = (PFNGLXGETSWAPINTERVALMESAPROC) SDL_GL_GetProcAddress("glXGetSwapIntervalMESA");
	if (glXSwapIntervalMESA && glXGetSwapIntervalMESA)
	{
		glXSwapIntervalMESA(interval);
		swapInterval = glXGetSwapIntervalMESA();
		return;
	}

	glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC) SDL_GL_GetProcAddress("glXSwapIntervalSGI");
	if (glXSwapIntervalSGI)
	{
		if (interval < 1)
			interval = 1;
		if (glXSwapIntervalSGI(interval))
		{
			// Error, revert to default
			swapInterval = 1;
			glXSwapIntervalSGI(1);
		}
		else
		{
			swapInterval = interval;
		}
		return;
	}
	swapInterval = 0;
}

int wzGetSwapInterval()
{
	if (swapInterval >= 0)
		return swapInterval;

	typedef void (* PFNGLXQUERYDRAWABLEPROC) (Display *, GLXDrawable, int, unsigned int *);
	typedef int (* PFNGLXGETSWAPINTERVALMESAPROC)(void);
	typedef int (* PFNGLXSWAPINTERVALSGIPROC) (int);
	PFNGLXQUERYDRAWABLEPROC glXQueryDrawable;
	PFNGLXGETSWAPINTERVALMESAPROC glXGetSwapIntervalMESA;
	PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;

#if GLX_VERSION_1_2
	// Hack-ish, but better than not supporting GLX_SWAP_INTERVAL_EXT?
	GLXDrawable drawable = glXGetCurrentDrawable();
	Display * display =  glXGetCurrentDisplay();
	glXQueryDrawable = (PFNGLXQUERYDRAWABLEPROC) SDL_GL_GetProcAddress("glXQueryDrawable");

	if (glXQueryDrawable && drawable)
	{
		unsigned interval;
		glXQueryDrawable(display, drawable, GLX_SWAP_INTERVAL_EXT, &interval);
		swapInterval = interval;
		return swapInterval;
	}
#endif

	glXGetSwapIntervalMESA = (PFNGLXGETSWAPINTERVALMESAPROC) SDL_GL_GetProcAddress("glXGetSwapIntervalMESA");
	if (glXGetSwapIntervalMESA)
	{
		swapInterval = glXGetSwapIntervalMESA();
		return swapInterval;
	}

	glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC) SDL_GL_GetProcAddress("glXSwapIntervalSGI");
	if (glXSwapIntervalSGI)
	{
		swapInterval = 1;
	}
	else
	{
		swapInterval = 0;
	}
	return swapInterval;
}

#elif defined(WZ_WS_WIN)

void wzSetSwapInterval(int interval)
{
	typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

	if (interval < 0)
		interval = 0;

	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) SDL_GL_GetProcAddress("wglSwapIntervalEXT");

	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(interval);
}

int wzGetSwapInterval()
{
	typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC) (void);
	PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

	wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC) SDL_GL_GetProcAddress("wglGetSwapIntervalEXT");
	if (wglGetSwapIntervalEXT)
		return wglGetSwapIntervalEXT();
	return 0;
}

#elif !defined(WZ_OS_MAC)

void wzSetSwapInterval(int)
{
	return;
}

int wzGetSwapInterval()
{
	return 0;
}

#endif

QList<QSize> wzAvailableResolutions()
{
	QList<QSize> list;
	int count;
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);
	for (count = 0; modes[count]; count++)
	{
		QSize s(modes[count]->w, modes[count]->h);
		list.push_back(s);
	}
	return list;
}

void wzShowMouse(bool visible)
{
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

int wzGetTicks()
{
	return SDL_GetTicks();
}

void wzFatalDialog(char const*)
{
	// no-op
}

void wzScreenFlip()
{
	SDL_GL_SwapBuffers();
}

void wzToggleFullscreen()
{
	war_setFullscreen(!war_getFullscreen());
	SDL_WM_ToggleFullScreen(screen);
}

void wzQuit()
{
	// Create a quit event to halt game loop.
	SDL_Event quitEvent;
	quitEvent.type = SDL_QUIT;
	SDL_PushEvent(&quitEvent);
}

void wzGrabMouse()
{
	SDL_WM_GrabInput(SDL_GRAB_ON);
}

void wzReleaseMouse()
{
	SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void wzDelay(unsigned int delay)
{
	SDL_Delay(delay);
}

/**************************/
/***    Thread support  ***/
/**************************/

WZ_THREAD *wzThreadCreate(int (*threadFunc)(void *), void *data)
{
	return (WZ_THREAD *)SDL_CreateThread(threadFunc, data);
}

int wzThreadJoin(WZ_THREAD *thread)
{
	int result;
	SDL_WaitThread((SDL_Thread *)thread, &result);
	return result;
}

void wzThreadStart(WZ_THREAD *thread)
{
	(void)thread; // no-op
}

void wzYieldCurrentThread()
{
	SDL_Delay(40);
}

WZ_MUTEX *wzMutexCreate()
{
	return (WZ_MUTEX *)SDL_CreateMutex();
}

void wzMutexDestroy(WZ_MUTEX *mutex)
{
	SDL_DestroyMutex((SDL_mutex *)mutex);
}

void wzMutexLock(WZ_MUTEX *mutex)
{
	SDL_LockMutex((SDL_mutex *)mutex);
}

void wzMutexUnlock(WZ_MUTEX *mutex)
{
	SDL_UnlockMutex((SDL_mutex *)mutex);
}

WZ_SEMAPHORE *wzSemaphoreCreate(int startValue)
{
	return (WZ_SEMAPHORE *)SDL_CreateSemaphore(startValue);
}

void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore)
{
	SDL_DestroySemaphore((SDL_sem *)semaphore);
}

void wzSemaphoreWait(WZ_SEMAPHORE *semaphore)
{
	SDL_SemWait((SDL_sem *)semaphore);
}

void wzSemaphorePost(WZ_SEMAPHORE *semaphore)
{
	SDL_SemPost((SDL_sem *)semaphore);
}

/**************************/
/***     Main support   ***/
/**************************/

std::pair<KEY_CODE, SDLKey> KEY_CODE_to_SDLKey[] =
{
	std::pair<KEY_CODE, SDLKey>(KEY_ESC, SDLK_ESCAPE),
	std::pair<KEY_CODE, SDLKey>(KEY_1, SDLK_1),
	std::pair<KEY_CODE, SDLKey>(KEY_2, SDLK_2),
	std::pair<KEY_CODE, SDLKey>(KEY_3, SDLK_3),
	std::pair<KEY_CODE, SDLKey>(KEY_4, SDLK_4),
	std::pair<KEY_CODE, SDLKey>(KEY_5, SDLK_5),
	std::pair<KEY_CODE, SDLKey>(KEY_6, SDLK_6),
	std::pair<KEY_CODE, SDLKey>(KEY_7, SDLK_7),
	std::pair<KEY_CODE, SDLKey>(KEY_8, SDLK_8),
	std::pair<KEY_CODE, SDLKey>(KEY_9, SDLK_9),
	std::pair<KEY_CODE, SDLKey>(KEY_0, SDLK_0),
	std::pair<KEY_CODE, SDLKey>(KEY_MINUS, SDLK_MINUS),
	std::pair<KEY_CODE, SDLKey>(KEY_EQUALS, SDLK_EQUALS),
	std::pair<KEY_CODE, SDLKey>(KEY_BACKSPACE, SDLK_BACKSPACE),
	std::pair<KEY_CODE, SDLKey>(KEY_TAB, SDLK_TAB),
	std::pair<KEY_CODE, SDLKey>(KEY_Q, SDLK_q),
	std::pair<KEY_CODE, SDLKey>(KEY_W, SDLK_w),
	std::pair<KEY_CODE, SDLKey>(KEY_E, SDLK_e),
	std::pair<KEY_CODE, SDLKey>(KEY_R, SDLK_r),
	std::pair<KEY_CODE, SDLKey>(KEY_T, SDLK_t),
	std::pair<KEY_CODE, SDLKey>(KEY_Y, SDLK_y),
	std::pair<KEY_CODE, SDLKey>(KEY_U, SDLK_u),
	std::pair<KEY_CODE, SDLKey>(KEY_I, SDLK_i),
	std::pair<KEY_CODE, SDLKey>(KEY_O, SDLK_o),
	std::pair<KEY_CODE, SDLKey>(KEY_P, SDLK_p),
	std::pair<KEY_CODE, SDLKey>(KEY_LBRACE, SDLK_LEFTBRACKET),
	std::pair<KEY_CODE, SDLKey>(KEY_RBRACE, SDLK_RIGHTBRACKET),
	std::pair<KEY_CODE, SDLKey>(KEY_RETURN, SDLK_RETURN),
	std::pair<KEY_CODE, SDLKey>(KEY_LCTRL, SDLK_LCTRL),
	std::pair<KEY_CODE, SDLKey>(KEY_A, SDLK_a),
	std::pair<KEY_CODE, SDLKey>(KEY_S, SDLK_s),
	std::pair<KEY_CODE, SDLKey>(KEY_D, SDLK_d),
	std::pair<KEY_CODE, SDLKey>(KEY_F, SDLK_f),
	std::pair<KEY_CODE, SDLKey>(KEY_G, SDLK_g),
	std::pair<KEY_CODE, SDLKey>(KEY_H, SDLK_h),
	std::pair<KEY_CODE, SDLKey>(KEY_J, SDLK_j),
	std::pair<KEY_CODE, SDLKey>(KEY_K, SDLK_k),
	std::pair<KEY_CODE, SDLKey>(KEY_L, SDLK_l),
	std::pair<KEY_CODE, SDLKey>(KEY_SEMICOLON, SDLK_SEMICOLON),
	std::pair<KEY_CODE, SDLKey>(KEY_QUOTE, SDLK_QUOTE),
	std::pair<KEY_CODE, SDLKey>(KEY_BACKQUOTE, SDLK_BACKQUOTE),
	std::pair<KEY_CODE, SDLKey>(KEY_LSHIFT, SDLK_LSHIFT),
	std::pair<KEY_CODE, SDLKey>(KEY_LMETA, SDLK_LMETA),
	std::pair<KEY_CODE, SDLKey>(KEY_LSUPER, SDLK_LSUPER),
	std::pair<KEY_CODE, SDLKey>(KEY_BACKSLASH, SDLK_BACKSLASH),
	std::pair<KEY_CODE, SDLKey>(KEY_Z, SDLK_z),
	std::pair<KEY_CODE, SDLKey>(KEY_X, SDLK_x),
	std::pair<KEY_CODE, SDLKey>(KEY_C, SDLK_c),
	std::pair<KEY_CODE, SDLKey>(KEY_V, SDLK_v),
	std::pair<KEY_CODE, SDLKey>(KEY_B, SDLK_b),
	std::pair<KEY_CODE, SDLKey>(KEY_N, SDLK_n),
	std::pair<KEY_CODE, SDLKey>(KEY_M, SDLK_m),
	std::pair<KEY_CODE, SDLKey>(KEY_COMMA, SDLK_COMMA),
	std::pair<KEY_CODE, SDLKey>(KEY_FULLSTOP, SDLK_PERIOD),
	std::pair<KEY_CODE, SDLKey>(KEY_FORWARDSLASH, SDLK_SLASH),
	std::pair<KEY_CODE, SDLKey>(KEY_RSHIFT, SDLK_RSHIFT),
	std::pair<KEY_CODE, SDLKey>(KEY_RMETA, SDLK_RMETA),
	std::pair<KEY_CODE, SDLKey>(KEY_RSUPER, SDLK_RSUPER),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_STAR, SDLK_KP_MULTIPLY),
	std::pair<KEY_CODE, SDLKey>(KEY_LALT, SDLK_LALT),
	std::pair<KEY_CODE, SDLKey>(KEY_SPACE, SDLK_SPACE),
	std::pair<KEY_CODE, SDLKey>(KEY_CAPSLOCK, SDLK_CAPSLOCK),
	std::pair<KEY_CODE, SDLKey>(KEY_F1, SDLK_F1),
	std::pair<KEY_CODE, SDLKey>(KEY_F2, SDLK_F2),
	std::pair<KEY_CODE, SDLKey>(KEY_F3, SDLK_F3),
	std::pair<KEY_CODE, SDLKey>(KEY_F4, SDLK_F4),
	std::pair<KEY_CODE, SDLKey>(KEY_F5, SDLK_F5),
	std::pair<KEY_CODE, SDLKey>(KEY_F6, SDLK_F6),
	std::pair<KEY_CODE, SDLKey>(KEY_F7, SDLK_F7),
	std::pair<KEY_CODE, SDLKey>(KEY_F8, SDLK_F8),
	std::pair<KEY_CODE, SDLKey>(KEY_F9, SDLK_F9),
	std::pair<KEY_CODE, SDLKey>(KEY_F10, SDLK_F10),
	std::pair<KEY_CODE, SDLKey>(KEY_NUMLOCK, SDLK_NUMLOCK),
	std::pair<KEY_CODE, SDLKey>(KEY_SCROLLLOCK, SDLK_SCROLLOCK),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_7, SDLK_KP7),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_8, SDLK_KP8),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_9, SDLK_KP9),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_MINUS, SDLK_KP_MINUS),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_4, SDLK_KP4),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_5, SDLK_KP5),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_6, SDLK_KP6),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_PLUS, SDLK_KP_PLUS),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_1, SDLK_KP1),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_2, SDLK_KP2),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_3, SDLK_KP3),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_0, SDLK_KP0),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_FULLSTOP, SDLK_KP_PERIOD),
	std::pair<KEY_CODE, SDLKey>(KEY_F11, SDLK_F11),
	std::pair<KEY_CODE, SDLKey>(KEY_F12, SDLK_F12),
	std::pair<KEY_CODE, SDLKey>(KEY_RCTRL, SDLK_RCTRL),
	std::pair<KEY_CODE, SDLKey>(KEY_KP_BACKSLASH, SDLK_KP_DIVIDE),
	std::pair<KEY_CODE, SDLKey>(KEY_RALT, SDLK_RALT),
	std::pair<KEY_CODE, SDLKey>(KEY_HOME, SDLK_HOME),
	std::pair<KEY_CODE, SDLKey>(KEY_UPARROW, SDLK_UP),
	std::pair<KEY_CODE, SDLKey>(KEY_PAGEUP, SDLK_PAGEUP),
	std::pair<KEY_CODE, SDLKey>(KEY_LEFTARROW, SDLK_LEFT),
	std::pair<KEY_CODE, SDLKey>(KEY_RIGHTARROW, SDLK_RIGHT),
	std::pair<KEY_CODE, SDLKey>(KEY_END, SDLK_END),
	std::pair<KEY_CODE, SDLKey>(KEY_DOWNARROW, SDLK_DOWN),
	std::pair<KEY_CODE, SDLKey>(KEY_PAGEDOWN, SDLK_PAGEDOWN),
	std::pair<KEY_CODE, SDLKey>(KEY_INSERT, SDLK_INSERT),
	std::pair<KEY_CODE, SDLKey>(KEY_DELETE, SDLK_DELETE),
	std::pair<KEY_CODE, SDLKey>(KEY_KPENTER, SDLK_KP_ENTER),
	std::pair<KEY_CODE, SDLKey>(KEY_IGNORE, SDLKey(5190)),
	std::pair<KEY_CODE, SDLKey>(KEY_MAXSCAN, SDLK_LAST),
};

std::pair<SDLKey, KEY_CODE> SDLKey_to_KEY_CODE[ARRAY_SIZE(KEY_CODE_to_SDLKey)];

static inline std::pair<SDLKey, KEY_CODE> swapit(std::pair<KEY_CODE, SDLKey> x)
{
	return  std::pair<SDLKey, KEY_CODE>(x.second, x.first);
}

static inline void initKeycodes()
{
	std::transform(KEY_CODE_to_SDLKey, KEY_CODE_to_SDLKey + ARRAY_SIZE(KEY_CODE_to_SDLKey), SDLKey_to_KEY_CODE, swapit);
	std::sort(KEY_CODE_to_SDLKey, KEY_CODE_to_SDLKey + ARRAY_SIZE(KEY_CODE_to_SDLKey));
	std::sort(SDLKey_to_KEY_CODE, SDLKey_to_KEY_CODE + ARRAY_SIZE(SDLKey_to_KEY_CODE));
}

static inline KEY_CODE sdlKeyToKeyCode(SDLKey key)
{
	std::pair<SDLKey, KEY_CODE> k(key, KEY_CODE(0));
	std::pair<SDLKey, KEY_CODE> const *found = std::lower_bound(SDLKey_to_KEY_CODE, SDLKey_to_KEY_CODE + ARRAY_SIZE(SDLKey_to_KEY_CODE), k);
	if (found != SDLKey_to_KEY_CODE + ARRAY_SIZE(SDLKey_to_KEY_CODE) && found->first == key)
	{
		return found->second;
	}

	return (KEY_CODE)key;
}

static inline SDLKey keyCodeToSDLKey(KEY_CODE code)
{
	std::pair<KEY_CODE, SDLKey> k(code, SDLKey(0));
	std::pair<KEY_CODE, SDLKey> const *found = std::lower_bound(KEY_CODE_to_SDLKey, KEY_CODE_to_SDLKey + ARRAY_SIZE(KEY_CODE_to_SDLKey), k);
	if (found != KEY_CODE_to_SDLKey + ARRAY_SIZE(KEY_CODE_to_SDLKey) && found->first == code)
	{
		return found->second;
	}

	return (SDLKey)code;
}

// Cyclic increment.
static InputKey *inputPointerNext(InputKey *p)
{
    return p + 1 == pInputBuffer + INPUT_MAXSTR ? pInputBuffer : p + 1;
}

/* add count copies of the characater code to the input buffer */
static void inputAddBuffer(UDWORD key, utf_32_char unicode)
{
	/* Calculate what pEndBuffer will be set to next */
	InputKey	*pNext = inputPointerNext(pEndBuffer);

	if (pNext == pStartBuffer)
	{
		return;  // Buffer full.
	}

	// Add key to buffer.
	pEndBuffer->key = key;
	pEndBuffer->unicode = unicode;
	pEndBuffer = pNext;
}


void keyScanToString(KEY_CODE code, char *ascii, UDWORD maxStringSize)
{
	if(keyCodeToSDLKey(code) == SDLK_LAST)
	{
		strcpy(ascii,"???");
		return;
	}
	else if (code == KEY_LCTRL)
	{
		// shortcuts with modifier keys work with either key.
		strcpy(ascii, "Ctrl");
		return;
	}
	else if (code == KEY_LSHIFT)
	{
		// shortcuts with modifier keys work with either key.
		strcpy(ascii, "Shift");
		return;
	}
	else if (code == KEY_LALT)
	{
		// shortcuts with modifier keys work with either key.
		strcpy(ascii, "Alt");
		return;
	}
	else if (code == KEY_LMETA)
	{
		// shortcuts with modifier keys work with either key.
#ifdef WZ_OS_MAC
		strcpy(ascii, "Cmd");
#else
		strcpy(ascii, "Meta");
#endif
		return;
	}
	ASSERT(keyCodeToSDLKey(code) < SDLK_LAST, "Invalid key code: %d", code);
	snprintf(ascii, maxStringSize, "%s", SDL_GetKeyName(keyCodeToSDLKey(code)));
	if (ascii[0] >= 'a' && ascii[0] <= 'z' && ascii[1] != 0)
	{
		// capitalize
		ascii[0] += 'A'-'a';
	}
}


/* Initialise the input module */
void inputInitialise(void)
{
	unsigned int i;

	for (i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}

	for (i = 0; i < MOUSE_BAD; i++)
	{
		aMouseState[i].state = KEY_UP;
	}

	pStartBuffer = pInputBuffer;
	pEndBuffer = pInputBuffer;

	dragX = mouseXPos = screenWidth/2;
	dragY = mouseYPos = screenHeight/2;
	dragKey = MOUSE_LMB;

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

/* Clear the input buffer */
void inputClearBuffer(void)
{
	pStartBuffer = pInputBuffer;
	pEndBuffer = pInputBuffer;
}

/* Return the next key press or 0 if no key in the buffer.
 * The key returned will have been remaped to the correct ascii code for the
 * windows key map.
 * All key presses are buffered up (including windows auto repeat).
 */
UDWORD inputGetKey(utf_32_char *unicode)
{
	UDWORD	retVal;

	if (pStartBuffer == pEndBuffer)
	{
	    return 0;  // Buffer empty.
	}

	retVal = pStartBuffer->key;
	if (unicode)
	{
	    *unicode = pStartBuffer->unicode;
	}
	if (!retVal)
	{
	    retVal = ' ';  // Don't return 0 if we got a virtual key, since that's interpreted as no input.
	}
	pStartBuffer = inputPointerNext(pStartBuffer);

	return retVal;
}

MousePresses const &inputGetClicks()
{
	return mousePresses;
}


/*!
 * This is called once a frame so that the system can tell
 * whether a key was pressed this turn or held down from the last frame.
 */
void inputNewFrame(void)
{
	unsigned int i;

	/* Do the keyboard */
	for (i = 0; i < KEY_MAXSCAN; i++)
	{
		if (aKeyState[i].state == KEY_PRESSED)
		{
			aKeyState[i].state = KEY_DOWN;
		}
		else if ( aKeyState[i].state == KEY_RELEASED  ||
			  aKeyState[i].state == KEY_PRESSRELEASE )
		{
			aKeyState[i].state = KEY_UP;
		}
	}

	/* Do the mouse */
	for (i = 0; i < 6; i++)
	{
		if (aMouseState[i].state == KEY_PRESSED)
		{
			aMouseState[i].state = KEY_DOWN;
		}
		else if ( aMouseState[i].state == KEY_RELEASED
		       || aMouseState[i].state == KEY_DOUBLECLICK
		       || aMouseState[i].state == KEY_PRESSRELEASE )
		{
			aMouseState[i].state = KEY_UP;
		}
	}

	mousePresses.clear();
}

/*!
 * Release all keys (and buttons) when we loose focus
 */
// FIXME This seems to be totally ignored! (Try switching focus while the dragbox is open)
void inputLoseFocus(void)
{
	unsigned int i;

	/* Lost the window focus, have to take this as a global key up */
	for(i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}
	for (i = 0; i < 6; i++)
	{
		aMouseState[i].state = KEY_UP;
	}
}

/* This returns true if the key is currently depressed */
bool keyDown(KEY_CODE code)
{
	ASSERT(keyCodeToSDLKey(code) < SDLK_LAST, "Invalid key code: %d", code);
	return (aKeyState[code].state != KEY_UP);
}

/* This returns true if the key went from being up to being down this frame */
bool keyPressed(KEY_CODE code)
{
	ASSERT(keyCodeToSDLKey(code) < SDLK_LAST, "Invalid key code: %d", code);
	return ((aKeyState[code].state == KEY_PRESSED) || (aKeyState[code].state == KEY_PRESSRELEASE));
}

/* This returns true if the key went from being down to being up this frame */
bool keyReleased(KEY_CODE code)
{
	ASSERT(keyCodeToSDLKey(code) < SDLK_LAST, "Invalid key code: %d", code);
	return ((aKeyState[code].state == KEY_RELEASED) || (aKeyState[code].state == KEY_PRESSRELEASE));
}

/* Return the X coordinate of the mouse */
Uint16 mouseX(void)
{
	return mouseXPos;
}

/* Return the Y coordinate of the mouse */
Uint16 mouseY(void)
{
	return mouseYPos;
}

bool wzMouseInWindow()
{
	return mouseInWindow;
}

Vector2i mousePressPos_DEPRECATED(MOUSE_KEY_CODE code)
{
	return aMouseState[code].pressPos;
}

Vector2i mouseReleasePos_DEPRECATED(MOUSE_KEY_CODE code)
{
	return aMouseState[code].releasePos;
}

/* This returns true if the mouse key is currently depressed */
bool mouseDown(MOUSE_KEY_CODE code)
{
	return (aMouseState[code].state != KEY_UP) ||

	       // holding down LMB and RMB counts as holding down MMB
	       (code == MOUSE_MMB && aMouseState[MOUSE_LMB].state != KEY_UP && aMouseState[MOUSE_RMB].state != KEY_UP);
}

/* This returns true if the mouse key was double clicked */
bool mouseDClicked(MOUSE_KEY_CODE code)
{
	return (aMouseState[code].state == KEY_DOUBLECLICK);
}

/* This returns true if the mouse key went from being up to being down this frame */
bool mousePressed(MOUSE_KEY_CODE code)
{
	return ((aMouseState[code].state == KEY_PRESSED) ||
			(aMouseState[code].state == KEY_DOUBLECLICK) ||
			(aMouseState[code].state == KEY_PRESSRELEASE));
}

/* This returns true if the mouse key went from being down to being up this frame */
bool mouseReleased(MOUSE_KEY_CODE code)
{
	return ((aMouseState[code].state == KEY_RELEASED) ||
			(aMouseState[code].state == KEY_DOUBLECLICK) ||
			(aMouseState[code].state == KEY_PRESSRELEASE));
}

/* Check for a mouse drag, return the drag start coords if dragging */
bool mouseDrag(MOUSE_KEY_CODE code, UDWORD *px, UDWORD *py)
{
	if ((aMouseState[code].state == KEY_DRAG) ||

	    // dragging LMB and RMB counts as dragging MMB
	    (code == MOUSE_MMB && ((aMouseState[MOUSE_LMB].state == KEY_DRAG && aMouseState[MOUSE_RMB].state != KEY_UP) ||
	     (aMouseState[MOUSE_LMB].state != KEY_UP && aMouseState[MOUSE_RMB].state == KEY_DRAG))))
	{
		*px = dragX;
		*py = dragY;
		return true;
	}

	return false;
}

// TODO: Rewrite this silly thing
void setMousePos(uint16_t x, uint16_t y)
{
	static int mousewarp = -1;

	if (mousewarp == -1)
	{
		int val;

		mousewarp = 1;
		if ((val = getMouseWarp()))
		{
			mousewarp = !val;
		}
	}
	if (mousewarp)
		SDL_WarpMouse(x, y);
}

/*!
 * Handle keyboard events
 */
static void inputHandleKeyEvent(SDL_KeyboardEvent * keyEvent)
{
	UDWORD code, vk;
	utf_32_char unicode = keyEvent->keysym.unicode;

	switch (keyEvent->type)
	{
		case SDL_KEYDOWN:
			switch (keyEvent->keysym.sym)
			{
				case SDLK_LEFT:
					vk = INPBUF_LEFT;
					break;
				case SDLK_RIGHT:
					vk = INPBUF_RIGHT;
					break;
				case SDLK_UP:
					vk = INPBUF_UP;
					break;
				case SDLK_DOWN:
					vk = INPBUF_DOWN;
					break;
				case SDLK_HOME:
					vk = INPBUF_HOME;
					break;
				case SDLK_END:
					vk = INPBUF_END;
					break;
				case SDLK_INSERT:
					vk = INPBUF_INS;
					break;
				case SDLK_DELETE:
					vk = INPBUF_DEL;
					break;
				case SDLK_PAGEUP:
					vk = INPBUF_PGUP;
					break;
				case SDLK_PAGEDOWN:
					vk = INPBUF_PGDN;
					break;
				default:
					vk = keyEvent->keysym.sym;
					break;
			}

			debug( LOG_INPUT, "Key Code (pressed): 0x%x, %d, [%c] SDLkey=[%s]", vk, vk, (vk < 128) && (vk > 31) ? (char) vk : '?' , SDL_GetKeyName(keyCodeToSDLKey((KEY_CODE)keyEvent->keysym.sym)));
			if (unicode < 32)
			{
				unicode = 0;
			}
			inputAddBuffer(vk, unicode);

			code = sdlKeyToKeyCode(keyEvent->keysym.sym);
			if ( aKeyState[code].state == KEY_UP ||
				 aKeyState[code].state == KEY_RELEASED ||
				 aKeyState[code].state == KEY_PRESSRELEASE )
			{
				//whether double key press or not
					aKeyState[code].state = KEY_PRESSED;
					aKeyState[code].lastdown = 0;
			}
			break;
		case SDL_KEYUP:
			code = sdlKeyToKeyCode(keyEvent->keysym.sym);
			if (aKeyState[code].state == KEY_PRESSED)
			{
				aKeyState[code].state = KEY_PRESSRELEASE;
			}
			else if (aKeyState[code].state == KEY_DOWN )
			{
				aKeyState[code].state = KEY_RELEASED;
			}
			break;
		default:
			break;
	}
}

/*!
 * Handle mousebutton events
 */
static void inputHandleMouseButtonEvent(SDL_MouseButtonEvent * buttonEvent)
{
	mouseXPos = buttonEvent->x;
	mouseYPos = buttonEvent->y;

	MOUSE_KEY_CODE mouseKeyCode;
	switch (buttonEvent->button)
	{
		default: return;  // Unknown button.
		case SDL_BUTTON_LEFT: mouseKeyCode = MOUSE_LMB; break;
		case SDL_BUTTON_MIDDLE: mouseKeyCode = MOUSE_MMB; break;
		case SDL_BUTTON_RIGHT: mouseKeyCode = MOUSE_RMB; break;
		case SDL_BUTTON_WHEELUP: mouseKeyCode = MOUSE_WUP; break;
		case SDL_BUTTON_WHEELDOWN: mouseKeyCode = MOUSE_WDN; break;
	}

	MousePress mousePress;
	mousePress.key = mouseKeyCode;
	mousePress.pos = Vector2i(mouseXPos, mouseYPos);

	switch (buttonEvent->type)
	{
		case SDL_MOUSEBUTTONDOWN:
			mousePress.action = MousePress::Press;
			mousePresses.push_back(mousePress);

			aMouseState[mouseKeyCode].pressPos.x = mouseXPos;
			aMouseState[mouseKeyCode].pressPos.y = mouseYPos;
			if ( aMouseState[mouseKeyCode].state == KEY_UP
				|| aMouseState[mouseKeyCode].state == KEY_RELEASED
				|| aMouseState[mouseKeyCode].state == KEY_PRESSRELEASE )
			{
				if ( (buttonEvent->button != SDL_BUTTON_WHEELUP		//skip doubleclick check for wheel
					&& buttonEvent->button != SDL_BUTTON_WHEELDOWN))
				{
					// whether double click or not
					if ( realTime - aMouseState[mouseKeyCode].lastdown < DOUBLE_CLICK_INTERVAL )
					{
						aMouseState[mouseKeyCode].state = KEY_DOUBLECLICK;
						aMouseState[mouseKeyCode].lastdown = 0;
					}
					else
					{
						aMouseState[mouseKeyCode].state = KEY_PRESSED;
						aMouseState[mouseKeyCode].lastdown = realTime;
					}

				}
				else	//mouse wheel up/down was used, so notify.
				{
					aMouseState[mouseKeyCode].state = KEY_PRESSED;
					aMouseState[mouseKeyCode].lastdown = 0;
				}

				if (mouseKeyCode < 4) // Not the mousewheel
				{
					dragKey = mouseKeyCode;
					dragX = mouseXPos;
					dragY = mouseYPos;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			mousePress.action = MousePress::Release;
			mousePresses.push_back(mousePress);

			aMouseState[mouseKeyCode].releasePos.x = mouseXPos;
			aMouseState[mouseKeyCode].releasePos.y = mouseYPos;
			if (aMouseState[mouseKeyCode].state == KEY_PRESSED)
			{
				aMouseState[mouseKeyCode].state = KEY_PRESSRELEASE;
			}
			else if ( aMouseState[mouseKeyCode].state == KEY_DOWN
					|| aMouseState[mouseKeyCode].state == KEY_DRAG
					|| aMouseState[mouseKeyCode].state == KEY_DOUBLECLICK)
			{
				aMouseState[mouseKeyCode].state = KEY_RELEASED;
			}
			break;
		default:
			break;
	}
}


/*!
 * Handle mousemotion events
 */
static void inputHandleMouseMotionEvent(SDL_MouseMotionEvent * motionEvent)
{
	switch (motionEvent->type)
	{
		case SDL_MOUSEMOTION:
			/* store the current mouse position */
			mouseXPos = motionEvent->x;
			mouseYPos = motionEvent->y;

			/* now see if a drag has started */
			if ((aMouseState[dragKey].state == KEY_PRESSED ||
			     aMouseState[dragKey].state == KEY_DOWN) &&
			    (ABSDIF(dragX, mouseXPos) > DRAG_THRESHOLD ||
			     ABSDIF(dragY, mouseYPos) > DRAG_THRESHOLD))
			{
				aMouseState[dragKey].state = KEY_DRAG;
			}
			break;
		default:
			break;
	}
}

void wzMain(int &argc, char **argv)
{
	initKeycodes();

	appPtr = new QApplication(argc, argv);  // For Qt-script.
}

bool wzMain2()
{
	//BEGIN **** Was in old frameInitialise. ****

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		debug( LOG_ERROR, "Error: Could not initialise SDL (%s).\n", SDL_GetError() );
		return false;
	}

#if !defined(WZ_OS_MAC) // Don't replace our 512 dock icon with a 32 one
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	uint32_t rmask = 0xff000000;
	uint32_t gmask = 0x00ff0000;
	uint32_t bmask = 0x0000ff00;
	uint32_t amask = 0x000000ff;
#else
	uint32_t rmask = 0x000000ff;
	uint32_t gmask = 0x0000ff00;
	uint32_t bmask = 0x00ff0000;
	uint32_t amask = 0xff000000;
#endif

	SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom((void*)wz2100icon.pixel_data, wz2100icon.width, wz2100icon.height, wz2100icon.bytes_per_pixel * 8,
	                                        wz2100icon.width * wz2100icon.bytes_per_pixel, rmask, gmask, bmask, amask), NULL);
#endif
	SDL_WM_SetCaption(PACKAGE_NAME, NULL);

	/* initialise all cursors */
	sdlInitCursors();
	//END **** Was in old frameInitialise. ****

	// TODO Fall back to windowed mode, if fullscreen mode fails.
	//BEGIN **** Was in old screenInitialise. ****
	unsigned        width = pie_GetVideoBufferWidth();
	unsigned        height = pie_GetVideoBufferHeight();
	unsigned        bitDepth = pie_GetVideoBufferDepth();
	unsigned        fsaa = war_getFSAA();
	bool            fullScreen = war_getFullscreen();
	bool            vsync = war_GetVsync();

	// Fetch the video info.
	const SDL_VideoInfo* video_info = SDL_GetVideoInfo();

	if (video_info == NULL)
	{
		debug(LOG_ERROR, "SDL_GetVideoInfo failed.");
		return false;
	}

	if (width == 0 || height == 0)
	{
		pie_SetVideoBufferWidth(width = screenWidth = video_info->current_w);
		pie_SetVideoBufferHeight(height = screenHeight = video_info->current_h);
		pie_SetVideoBufferDepth(bitDepth = screenDepth = video_info->vfmt->BitsPerPixel);
	}
	else
	{
		screenWidth = width;
		screenHeight = height;
		screenDepth = bitDepth;
	}
	screenWidth = MAX(screenWidth, 640);
	screenHeight = MAX(screenHeight, 480);

	// The flags to pass to SDL_SetVideoMode.
	int video_flags  = SDL_OPENGL;    // Enable OpenGL in SDL.
	video_flags |= SDL_ANYFORMAT; // Don't emulate requested BPP if not available.

	if (fullScreen)
	{
		video_flags |= SDL_FULLSCREEN;
	}

	// Set the double buffer OpenGL attribute.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable vsync if requested by the user
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);

	// Enable FSAA anti-aliasing if and at the level requested by the user
	if (fsaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);
	}

	int bpp = SDL_VideoModeOK(width, height, bitDepth, video_flags);
	if (!bpp)
	{
		debug(LOG_ERROR, "Video mode %dx%d@%dbpp is not supported!", width, height, bitDepth);
		return false;
	}
	switch (bpp)
	{
		case 32:
		case 24:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			break;
		case 16:
			info("Using colour depth of %i instead of %i.", bpp, screenDepth);
			info("You will experience graphics glitches!");
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			break;
		case 8:
			debug(LOG_FATAL, "You don't want to play Warzone with a bit depth of %i, do you?", bpp);
			exit(1);
			break;
		default:
			debug(LOG_FATAL, "Unsupported bit depth: %i", bpp);
			exit(1);
			break;
	}

	screen = SDL_SetVideoMode(width, height, bpp, video_flags);
	if (!screen)
	{
		debug(LOG_ERROR, "SDL_SetVideoMode failed (%s).", SDL_GetError());
		return false;
	}
	int value = 0;
	if (SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == -1 || value == 0)
	{
		debug( LOG_FATAL, "OpenGL initialization did not give double buffering!" );
		debug( LOG_FATAL, "Double buffering is required for this game!");
		exit(1);
	}

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, height, 0, 1, -1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//END **** Was in old screenInitialise. ****

	init_scrap();
	return true;
}

/*!
 * Activation (focus change) eventhandler
 */
static void handleActiveEvent(SDL_ActiveEvent * activeEvent)
{
	switch (activeEvent->state)
	{
		case SDL_APPMOUSEFOCUS:
			mouseInWindow = activeEvent->gain;
			break;
		default:
			break;
	}
}

// Actual mainloop
void wzMain3()
{
	SDL_Event event;

	while (true)
	{
		/* Deal with any windows messages */
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				inputHandleKeyEvent(&event.key);
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				inputHandleMouseButtonEvent(&event.button);
				break;
			case SDL_MOUSEMOTION:
				inputHandleMouseMotionEvent(&event.motion);
				break;
			case SDL_ACTIVEEVENT:
				handleActiveEvent(&event.active);
				break;
			case SDL_QUIT:
				//saveConfig();
				return;
			default:
				break;
			}
		}

		appPtr->processEvents();

		mainLoop();
		inputNewFrame();
	}
}

void wzShutdown()
{
	sdlFreeCursors();
	SDL_Quit();
	delete appPtr;
	appPtr = NULL;
}
