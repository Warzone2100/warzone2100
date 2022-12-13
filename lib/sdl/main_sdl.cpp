/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2020  Warzone 2100 Project

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

// Get platform defines before checking for them!
#include "lib/framework/wzapp.h"

#include "lib/framework/input.h"
#include "lib/framework/utf.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/gamelib/gtime.h"
#include "src/configuration.h"
#include "src/warzoneconfig.h"
#include "src/game.h"
#include "gfx_api_sdl.h"
#include "gfx_api_gl_sdl.h"

#if defined( _MSC_VER )
	// Silence warning when using MSVC ARM64 compiler
	//	warning C4121: 'SDL_hid_device_info': alignment of a member was sensitive to packing
	#pragma warning( push )
	#pragma warning( disable : 4121 )
#endif

#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_clipboard.h>
#if defined(HAVE_SDL_VULKAN_H)
#include <SDL_vulkan.h>
#endif
#include <SDL_version.h>

#if defined( _MSC_VER )
	#pragma warning( pop )
#endif

#include "wz2100icon.h"
#include "cursors_sdl.h"
#include <algorithm>
#include <map>
#include <locale.h>
#include <atomic>
#include <chrono>

#if defined(WZ_OS_MAC)
#include "cocoa_sdl_helpers.h"
#include "cocoa_wz_menus.h"
#endif

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

void mainLoop();
// used in crash reports & version info
const char *BACKEND = "SDL";

std::map<KEY_CODE, SDL_Keycode> KEY_CODE_to_SDLKey;
std::map<SDL_Keycode, KEY_CODE > SDLKey_to_KEY_CODE;

int realmain(int argc, char *argv[]);

// the main stub which calls realmain() aka, WZ's main startup routines
int main(int argc, char *argv[])
{
	return realmain(argc, argv);
}

// At this time, we only have 1 window.
static SDL_Window *WZwindow = nullptr;
static optional<video_backend> WZbackend = video_backend::opengl;

#if defined(WZ_OS_MAC) || defined(WZ_OS_WIN)
// on macOS, SDL_WINDOW_FULLSCREEN_DESKTOP *must* be used (or high-DPI fullscreen toggling breaks)
const WINDOW_MODE WZ_SDL_DEFAULT_FULLSCREEN_MODE = WINDOW_MODE::desktop_fullscreen;
#else
const WINDOW_MODE WZ_SDL_DEFAULT_FULLSCREEN_MODE = WINDOW_MODE::fullscreen;
#endif
static WINDOW_MODE altEnterToggleFullscreenMode = WZ_SDL_DEFAULT_FULLSCREEN_MODE;

// The screen that the game window is on.
int screenIndex = 0;
// The logical resolution of the game in the game's coordinate system (points).
unsigned int screenWidth = 0;
unsigned int screenHeight = 0;
// The logical resolution of the SDL window in the window's coordinate system (points) - i.e. not accounting for the Game Display Scale setting.
unsigned int windowWidth = 0;
unsigned int windowHeight = 0;
// The current display scale factor.
unsigned int current_displayScale = 100;
float current_displayScaleFactor = 1.f;

struct QueuedWindowDimensions
{
	int width;
	int height;
	bool recenter;
};
optional<QueuedWindowDimensions> deferredDimensionReset = nullopt;

static std::vector<screeninfo> displaylist;	// holds all our possible display lists

std::atomic<Uint32> wzSDLAppEvent((Uint32)-1);
enum wzSDLAppEventCodes
{
	MAINTHREADEXEC
};

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

// Clipboard routines
bool has_scrap(void);
bool get_scrap(char **dst);

/// constant for the interval between 2 singleclicks for doubleclick event in ms
#define DOUBLE_CLICK_INTERVAL 250

/* The current state of the keyboard */
static INPUT_STATE aKeyState[KEY_MAXSCAN];		// NOTE: SDL_NUM_SCANCODES is the max, but KEY_MAXSCAN is our limit

/* The current location of the mouse */
static Uint16 mouseXPos = 0;
static Uint16 mouseYPos = 0;
static Vector2i mouseWheelSpeed;
static bool mouseInWindow = true;

/* How far the mouse has to move to start a drag */
#define DRAG_THRESHOLD	5

/* Which button is being used for a drag */
static MOUSE_KEY_CODE dragKey;

/* The start of a possible drag by the mouse */
static int dragX = 0;
static int dragY = 0;

/* The current mouse button state */
static INPUT_STATE aMouseState[MOUSE_END];
static MousePresses mousePresses;

/* The current screen resizing state for this iteration through the game loop, in the game coordinate system */
struct ScreenSizeChange
{
	ScreenSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
	:	oldWidth(oldWidth)
	,	oldHeight(oldHeight)
	,	newWidth(newWidth)
	,	newHeight(newHeight)
	{ }
	unsigned int oldWidth;
	unsigned int oldHeight;
	unsigned int newWidth;
	unsigned int newHeight;
};
static ScreenSizeChange* currentScreenResizingStatus = nullptr;

/* The size of the input buffer */
#define INPUT_MAXSTR 256

/* The input string buffer */
struct InputKey
{
	UDWORD key;
	utf_32_char unicode;
};

static InputKey	pInputBuffer[INPUT_MAXSTR];
static InputKey	*pStartBuffer, *pEndBuffer;
static utf_32_char *utf8Buf;				// is like the old 'unicode' from SDL 1.x
void* GetTextEventsOwner = nullptr;

static optional<int> wzQuitExitCode;

bool wzReduceDisplayScalingIfNeeded(int currWidth, int currHeight);

/**************************/
/***     Misc support   ***/
/**************************/

WzString wzGetPlatform()
{
	return WzString::fromUtf8(SDL_GetPlatform());
}

// See if we have TEXT in the clipboard
bool has_scrap(void)
{
	return SDL_HasClipboardText();
}

// Set the clipboard text
bool wzSetClipboardText(const char *src)
{
	if (SDL_SetClipboardText(src))
	{
		debug(LOG_ERROR, "Could not put clipboard text because : %s", SDL_GetError());
		return false;
	}
	return true;
}

// Get text from the clipboard
bool get_scrap(char **dst)
{
	if (has_scrap())
	{
		char *cliptext = SDL_GetClipboardText();
		if (!cliptext)
		{
			debug(LOG_ERROR, "Could not get clipboard text because : %s", SDL_GetError());
			return false;
		}
		*dst = cliptext;
		return true;
	}
	else
	{
		// wasn't text or no text in the clipboard
		return false;
	}
}

void StartTextInput(void* pTextInputRequester)
{
	if (!GetTextEventsOwner)
	{
		SDL_StartTextInput();	// enable text events
		debug(LOG_INPUT, "SDL text events started");
	}
	else if (pTextInputRequester != GetTextEventsOwner)
	{
		debug(LOG_INPUT, "StartTextInput called by new input requester before old requester called StopTextInput");
	}
	GetTextEventsOwner = pTextInputRequester;
}

void StopTextInput(void* pTextInputResigner)
{
	if (!GetTextEventsOwner)
	{
		debug(LOG_INPUT, "Ignoring StopTextInput call when text input is already disabled");
		return;
	}
	if (pTextInputResigner != GetTextEventsOwner)
	{
		// Rejecting StopTextInput from regsigner who is not the last requester
		debug(LOG_INPUT, "Ignoring StopTextInput call from resigner that is not the last requester (i.e. caller of StartTextInput)");
		return;
	}
	SDL_StopTextInput();	// disable text events
	GetTextEventsOwner = nullptr;
	debug(LOG_INPUT, "SDL text events stopped");
}

bool isInTextInputMode()
{
	bool result = (GetTextEventsOwner != nullptr);
	ASSERT((SDL_IsTextInputActive() != SDL_FALSE) == result, "How did GetTextEvents state and SDL_IsTextInputActive get out of sync?");
	return result;
}

/* Put a character into a text buffer overwriting any text under the cursor */
WzString wzGetSelection()
{
	WzString retval;
	static char *scrap = nullptr;

	if (get_scrap(&scrap))
	{
		retval = WzString::fromUtf8(scrap);
	}
	return retval;
}

std::vector<screeninfo> wzAvailableResolutions()
{
	return displaylist;
}

screeninfo wzGetCurrentFullscreenDisplayMode()
{
	screeninfo result = {};
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzGetCurrentFullscreenDisplayMode called when window is not available");
		return {};
	}
	SDL_DisplayMode current = { 0, 0, 0, 0, 0 };
	int currScreen = SDL_GetWindowDisplayIndex(WZwindow);
	if (currScreen < 0)
	{
		debug(LOG_WZ, "Failed to get current screen index: %s", SDL_GetError());
		currScreen = 0;
	}
	if (SDL_GetWindowDisplayMode(WZwindow, &current) != 0)
	{
		// Failed to get current fullscreen display mode for window?
		debug(LOG_WZ, "Failed to get current WindowDisplayMode: %s", SDL_GetError());
		return {};
	}
	result.screen = currScreen;
	result.width = current.w;
	result.height = current.h;
	result.refresh_rate = current.refresh_rate;
	return result;
}

std::vector<unsigned int> wzAvailableDisplayScales()
{
	static const unsigned int wzDisplayScales[] = { 100, 125, 150, 200, 250, 300, 400, 500 };
	return std::vector<unsigned int>(wzDisplayScales, wzDisplayScales + (sizeof(wzDisplayScales) / sizeof(wzDisplayScales[0])));
}

static std::vector<video_backend>& sortGfxBackendsForCurrentSystem(std::vector<video_backend>& backends)
{
#if defined(_WIN32) && defined(WZ_BACKEND_DIRECTX) && (defined(_M_ARM64) || defined(_M_ARM))
	// On ARM-based Windows, DirectX should be first (for compatibility)
	std::stable_sort(backends.begin(), backends.end(), [](video_backend a, video_backend b) -> bool {
		if (a == b) { return false; }
		if (a == video_backend::directx) { return true; }
		return false;
	});
#else
	// currently, no-op
#endif
	return backends;
}

std::vector<video_backend> wzAvailableGfxBackends()
{
	std::vector<video_backend> availableBackends;
	availableBackends.push_back(video_backend::opengl);
#if !defined(WZ_OS_MAC) // OpenGL ES is not supported on macOS, and WZ doesn't currently ship with an OpenGL ES library on macOS
	availableBackends.push_back(video_backend::opengles);
#endif
#if defined(WZ_VULKAN_ENABLED) && defined(HAVE_SDL_VULKAN_H)
	availableBackends.push_back(video_backend::vulkan);
#endif
#if defined(WZ_BACKEND_DIRECTX)
	availableBackends.push_back(video_backend::directx);
#endif
	sortGfxBackendsForCurrentSystem(availableBackends);
	return availableBackends;
}

video_backend wzGetDefaultGfxBackendForCurrentSystem()
{
	// SDL backend supports: OpenGL, OpenGLES, Vulkan (if compiled with support), DirectX (on Windows, via LibANGLE)

#if defined(_WIN32) && defined(WZ_BACKEND_DIRECTX) && (defined(_M_ARM64) || defined(_M_ARM))
	// On ARM-based Windows, DirectX should be the default (for compatibility)
	return video_backend::directx;
#else
	// Future TODO examples:
	//	- Default to Vulkan backend on macOS versions > 10.??, to use Metal via MoltenVK (needs testing - and may require exclusions depending on hardware?)
	//	- Default to DirectX (via LibANGLE) backend on Windows, depending on Windows version (and possibly hardware? / DirectX-level support?)
	//	- Check if Vulkan appears to be properly supported on a Windows / Linux system, and default to Vulkan backend?

	// For now, default to OpenGL (which automatically falls back to OpenGL ES if needed)
	return video_backend::opengl;
#endif
}

static video_backend wzGetNextFallbackGfxBackendForCurrentSystem(const video_backend& current_failed_backend)
{
	video_backend next_backend;
#if defined(_WIN32) && defined(WZ_BACKEND_DIRECTX)
	switch (current_failed_backend)
	{
		case video_backend::opengl:
			// offer DirectX as a fallback option if OpenGL failed
			next_backend = video_backend::directx;
			break;
#if (defined(_M_ARM64) || defined(_M_ARM))
		case video_backend::directx:
			// since DirectX is the default on ARM-based Windows, offer OpenGL as an alternative
			next_backend = video_backend::opengl;
			break;
#endif
		default:
			// offer usual default
			next_backend = wzGetDefaultGfxBackendForCurrentSystem();
			break;
	}
#elif defined(WZ_OS_MAC)
	switch (current_failed_backend)
	{
		case video_backend::opengl:
			// offer Vulkan (which uses Vulkan -> Metal) as a fallback option if OpenGL failed
			next_backend = video_backend::vulkan;
			break;
		default:
			// offer usual default
			next_backend = wzGetDefaultGfxBackendForCurrentSystem();
			break;
	}
#else
	next_backend = wzGetDefaultGfxBackendForCurrentSystem();
#endif

	// sanity-check: verify that next_backend is in available backends
	const auto available = wzAvailableGfxBackends();
	if (std::find(available.begin(), available.end(), next_backend) == available.end())
	{
		// next_backend does not exist in the list of available backends, so default to wzGetDefaultGfxBackendForCurrentSystem()
		next_backend = wzGetDefaultGfxBackendForCurrentSystem();
	}

	return next_backend;
}

void SDL_WZBackend_GetDrawableSize(SDL_Window* window,
								   int*        w,
								   int*        h)
{
	if (!WZbackend.has_value())
	{
		return;
	}
	switch (WZbackend.value())
	{
#if defined(WZ_BACKEND_DIRECTX)
		case video_backend::directx: // because DirectX is supported via OpenGLES (LibANGLE)
#endif
		case video_backend::opengl:
		case video_backend::opengles:
			return SDL_GL_GetDrawableSize(window, w, h);
		case video_backend::vulkan:
#if defined(HAVE_SDL_VULKAN_H)
			return SDL_Vulkan_GetDrawableSize(window, w, h);
#else
			SDL_version compiled_version;
			SDL_VERSION(&compiled_version);
			debug(LOG_FATAL, "The version of SDL used for compilation (%u.%u.%u) did not have the SDL_vulkan.h header", (unsigned int)compiled_version.major, (unsigned int)compiled_version.minor, (unsigned int)compiled_version.patch);
			if (w) { w = 0; }
			if (h) { h = 0; }
			return;
#endif
		case video_backend::num_backends:
			debug(LOG_FATAL, "Should never happen");
			return;
	}
}

void setDisplayScale(unsigned int displayScale)
{
	ASSERT(displayScale >= 100, "Invalid display scale: %u", displayScale);
	current_displayScale = displayScale;
	current_displayScaleFactor = (float)displayScale / 100.f;
}

unsigned int wzGetCurrentDisplayScale()
{
	return current_displayScale;
}

void wzShowMouse(bool visible)
{
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

int wzGetTicks()
{
	return SDL_GetTicks();
}

void wzDisplayDialog(DialogType type, const char *title, const char *message)
{
	if (!WZbackend.has_value())
	{
		// while this could be thread_local, thread_local may not yet be supported on all platforms properly
		// and wzDisplayDialog should probably be called **only** on the main thread anyway
		static bool processingDialog = false;
		if (!processingDialog)
		{
			// in headless mode, do not display a messagebox (which might block the main thread)
			// but just log the details
			processingDialog = true;
			debug(LOG_INFO, "Suppressed dialog (headless):\n\tTitle: %s\n\tMessage: %s", title, message);
			processingDialog = false;
		}
		return;
	}

	Uint32 sdl_messagebox_flags = 0;
	switch (type)
	{
		case Dialog_Error:
			sdl_messagebox_flags = SDL_MESSAGEBOX_ERROR;
			break;
		case Dialog_Warning:
			sdl_messagebox_flags = SDL_MESSAGEBOX_WARNING;
			break;
		case Dialog_Information:
			sdl_messagebox_flags = SDL_MESSAGEBOX_INFORMATION;
			break;
	}
	SDL_ShowSimpleMessageBox(sdl_messagebox_flags, title, message, WZwindow);
}

WINDOW_MODE wzGetCurrentWindowMode()
{
	if (!WZbackend.has_value())
	{
		// return a dummy value
		return WINDOW_MODE::windowed;
	}
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzGetCurrentWindowMode called when window is not available");
		return WINDOW_MODE::windowed;
	}

	Uint32 flags = SDL_GetWindowFlags(WZwindow);
	if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP)
	{
		return WINDOW_MODE::desktop_fullscreen;
	}
	else if (flags & SDL_WINDOW_FULLSCREEN)
	{
		return WINDOW_MODE::fullscreen;
	}
	else
	{
		return WINDOW_MODE::windowed;
	}
}

std::vector<WINDOW_MODE> wzSupportedWindowModes()
{
#if defined(WZ_OS_MAC)
	// on macOS, SDL_WINDOW_FULLSCREEN_DESKTOP *must* be used (or high-DPI fullscreen toggling breaks)
	// thus "classic" fullscreen is not supported
	return {WINDOW_MODE::desktop_fullscreen, WINDOW_MODE::windowed};
#else
	return {WINDOW_MODE::desktop_fullscreen, WINDOW_MODE::windowed, WINDOW_MODE::fullscreen};
#endif
}

bool wzIsSupportedWindowMode(WINDOW_MODE mode)
{
	auto supportedModes = wzSupportedWindowModes();
	return std::any_of(supportedModes.begin(), supportedModes.end(), [mode](WINDOW_MODE i) -> bool {
		return i == mode;
	});
}

WINDOW_MODE wzGetNextWindowMode(WINDOW_MODE currentMode)
{
	auto supportedModes = wzSupportedWindowModes();
	ASSERT_OR_RETURN(WINDOW_MODE::windowed, !supportedModes.empty(), "No supported fullscreen / windowed modes available?");
	auto it = std::find(supportedModes.begin(), supportedModes.end(), currentMode);
	if (it == supportedModes.end())
	{
		// we appear to be in an unsupported mode - so default to the first
		return *supportedModes.begin();
	}
	++it;
	if (it == supportedModes.end()) { it = supportedModes.begin(); }
	return *it;
}

WINDOW_MODE wzAltEnterToggleFullscreen()
{
	// toggles out of and into the "last" fullscreen mode
	auto mode = wzGetCurrentWindowMode();
	switch (mode)
	{
		case WINDOW_MODE::desktop_fullscreen:
			// fall-through
		case WINDOW_MODE::fullscreen:
			altEnterToggleFullscreenMode = mode;
			if (wzChangeWindowMode(WINDOW_MODE::windowed))
			{
				mode = WINDOW_MODE::windowed;
			}
			break;
		case WINDOW_MODE::windowed:
			// use altEnterToggleFullscreenMode
			if (wzChangeWindowMode(altEnterToggleFullscreenMode))
			{
				mode = altEnterToggleFullscreenMode;
			}
			break;
	}
	return mode;
}

bool wzSetToggleFullscreenMode(WINDOW_MODE fullscreenMode)
{
	switch (fullscreenMode)
	{
		case WINDOW_MODE::fullscreen:
		case WINDOW_MODE::desktop_fullscreen:
		{
			if (!wzIsSupportedWindowMode(fullscreenMode))
			{
				// not a supported mode on this system!
				return false;
			}
			altEnterToggleFullscreenMode = fullscreenMode;
			return true;
		}
		default:
			return false;
	}
}

WINDOW_MODE wzGetToggleFullscreenMode()
{
	return altEnterToggleFullscreenMode;
}

bool wzChangeWindowMode(WINDOW_MODE mode)
{
	auto currMode = wzGetCurrentWindowMode();
	if (currMode == mode)
	{
		// already in this mode
		return true;
	}

	if (!wzIsSupportedWindowMode(mode))
	{
		// not a supported mode on this system
		return false;
	}

	debug(LOG_INFO, "Changing window mode: %s -> %s", to_display_string(currMode).c_str(), to_display_string(mode).c_str());

	int sdl_result = -1;
	switch (mode)
	{
		case WINDOW_MODE::desktop_fullscreen:
			sdl_result = SDL_SetWindowFullscreen(WZwindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
			if (sdl_result != 0) { return false; }
			wzSetWindowIsResizable(false);
			break;
		case WINDOW_MODE::windowed:
		{
			int currDisplayIndex = SDL_GetWindowDisplayIndex(WZwindow);
			if (currDisplayIndex < 0)
			{
				currDisplayIndex = screenIndex;
			}
			// disable fullscreen mode
			sdl_result = SDL_SetWindowFullscreen(WZwindow, 0);
			if (sdl_result != 0) { return false; }
			wzSetWindowIsResizable(true);
			// Determine the maximum usable windowed size for this display/screen, and cap the desired window size at that
			int desiredWidth = war_GetWidth(), desiredHeight = war_GetHeight();
			SDL_Rect displayUsableBounds = { 0, 0, 0, 0 };
#if defined(WZ_OS_MAC)
			// SDL currently triggers an internal assert when calling SDL_GetDisplayUsableBounds here on macOS - so use SDL_GetDisplayBounds for now
			if (SDL_GetDisplayBounds(currDisplayIndex, &displayUsableBounds) == 0)
#else
			if (SDL_GetDisplayUsableBounds(currDisplayIndex, &displayUsableBounds) == 0)
#endif
			{
				if (displayUsableBounds.w > 0 && displayUsableBounds.h > 0)
				{
					desiredWidth = std::min(displayUsableBounds.w, desiredWidth);
					desiredHeight = std::min(displayUsableBounds.h, desiredHeight);
				}
			}
			// restore the old windowed size
			SDL_SetWindowSize(WZwindow, desiredWidth, desiredHeight);
			// Position the window (centered) on the screen (for its new size)
			SDL_SetWindowPosition(WZwindow, SDL_WINDOWPOS_CENTERED_DISPLAY(currDisplayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(currDisplayIndex));
			// Workaround for issues properly restoring window dimensions when changing from fullscreen -> windowed (on some platforms)
			deferredDimensionReset = QueuedWindowDimensions {desiredWidth, desiredHeight, true};
			break;
		}
		case WINDOW_MODE::fullscreen:
		{
			sdl_result = SDL_SetWindowFullscreen(WZwindow, SDL_WINDOW_FULLSCREEN);
			if (sdl_result != 0) { return false; }
			wzSetWindowIsResizable(false);
			int currWidth = 0, currHeight = 0;
			SDL_GetWindowSize(WZwindow, &currWidth, &currHeight);
			wzReduceDisplayScalingIfNeeded(currWidth, currHeight);
			break;
		}
	}

	return sdl_result == 0;
}

bool wzIsFullscreen()
{
	if (WZwindow == nullptr)
	{
		// NOTE: Can't use debug(...) to log here or it will risk overwriting fatal error messages,
		// as `_debug(...)` calls this function
		return false;
	}
	Uint32 flags = SDL_GetWindowFlags(WZwindow);
	if ((flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
	{
		return true;
	}
	return false;
}

bool wzIsMaximized()
{
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzIsMaximized called when window is not available");
		return false;
	}
	Uint32 flags = SDL_GetWindowFlags(WZwindow);
	if (flags & SDL_WINDOW_MAXIMIZED)
	{
		return true;
	}
	return false;
}

void wzQuit(int exitCode)
{
	if (!wzQuitExitCode.has_value())
	{
		wzQuitExitCode = exitCode;
	}
	// Create a quit event to halt game loop.
	SDL_Event quitEvent;
	quitEvent.type = SDL_QUIT;
	SDL_PushEvent(&quitEvent);
}

int wzGetQuitExitCode()
{
	return wzQuitExitCode.value_or(0);
}

void wzGrabMouse()
{
	if (!WZbackend.has_value())
	{
		return;
	}
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzGrabMouse called when window is not available - ignoring");
		return;
	}
	SDL_SetWindowGrab(WZwindow, SDL_TRUE);
}

void wzReleaseMouse()
{
	if (!WZbackend.has_value())
	{
		return;
	}
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzReleaseMouse called when window is not available - ignoring");
		return;
	}
	SDL_SetWindowGrab(WZwindow, SDL_FALSE);
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
	return (WZ_THREAD *)SDL_CreateThread(threadFunc, "wzThread", data);
}

unsigned long wzThreadID(WZ_THREAD *thread)
{
	SDL_threadID threadID = SDL_GetThreadID((SDL_Thread *)thread);
	return threadID;
}

int wzThreadJoin(WZ_THREAD *thread)
{
	int result;
	SDL_WaitThread((SDL_Thread *)thread, &result);
	return result;
}

void wzThreadDetach(WZ_THREAD *thread)
{
	SDL_DetachThread((SDL_Thread *)thread);
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

// Asynchronously executes exec->doExecOnMainThread() on the main thread
// `exec` should be a subclass of `WZ_MAINTHREADEXEC`
//
// `exec` must be allocated on the heap since the main event loop takes ownership of it
// and will handle deleting it once it has been processed.
// It is not safe to access `exec` after calling wzAsyncExecOnMainThread.
//
// No guarantees are made about when execFunc() will be called relative to the
// calling of this function - this function may return before, during, or after
// execFunc()'s execution on the main thread.
void wzAsyncExecOnMainThread(WZ_MAINTHREADEXEC *exec)
{
	Uint32 _wzSDLAppEvent = wzSDLAppEvent.load();
	assert(_wzSDLAppEvent != ((Uint32)-1));
	if (_wzSDLAppEvent == ((Uint32)-1)) {
		// The app-defined event has not yet been registered with SDL
		return;
	}
	SDL_Event execEvent;
	SDL_memset(&execEvent, 0, sizeof(execEvent));
	execEvent.type = _wzSDLAppEvent;
	execEvent.user.code = wzSDLAppEventCodes::MAINTHREADEXEC;
	execEvent.user.data1 = exec;
	assert(execEvent.user.data1 != nullptr);
	execEvent.user.data2 = 0;
	SDL_PushEvent(&execEvent);
	// receiver handles deleting `exec` on the main thread after doExecOnMainThread() has been called
}

/*!
** The keycodes we care about
**/
static inline void initKeycodes()
{
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_ESC, SDLK_ESCAPE));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_1, SDLK_1));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_2, SDLK_2));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_3, SDLK_3));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_4, SDLK_4));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_5, SDLK_5));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_6, SDLK_6));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_7, SDLK_7));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_8, SDLK_8));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_9, SDLK_9));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_0, SDLK_0));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_MINUS, SDLK_MINUS));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_EQUALS, SDLK_EQUALS));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_BACKSPACE, SDLK_BACKSPACE));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_TAB, SDLK_TAB));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_Q, SDLK_q));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_W, SDLK_w));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_E, SDLK_e));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_R, SDLK_r));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_T, SDLK_t));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_Y, SDLK_y));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_U, SDLK_u));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_I, SDLK_i));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_O, SDLK_o));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_P, SDLK_p));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_LBRACE, SDLK_LEFTBRACKET));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RBRACE, SDLK_RIGHTBRACKET));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RETURN, SDLK_RETURN));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_LCTRL, SDLK_LCTRL));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_A, SDLK_a));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_S, SDLK_s));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_D, SDLK_d));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F, SDLK_f));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_G, SDLK_g));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_H, SDLK_h));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_J, SDLK_j));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_K, SDLK_k));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_L, SDLK_l));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_SEMICOLON, SDLK_SEMICOLON));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_QUOTE, SDLK_QUOTE));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_BACKQUOTE, SDLK_BACKQUOTE));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_LSHIFT, SDLK_LSHIFT));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_LMETA, SDLK_LGUI));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_LSUPER, SDLK_LGUI));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_BACKSLASH, SDLK_BACKSLASH));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_Z, SDLK_z));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_X, SDLK_x));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_C, SDLK_c));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_V, SDLK_v));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_B, SDLK_b));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_N, SDLK_n));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_M, SDLK_m));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_COMMA, SDLK_COMMA));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_FULLSTOP, SDLK_PERIOD));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_FORWARDSLASH, SDLK_SLASH));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RSHIFT, SDLK_RSHIFT));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RMETA, SDLK_RGUI));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RSUPER, SDLK_RGUI));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_STAR, SDLK_KP_MULTIPLY));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_LALT, SDLK_LALT));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_SPACE, SDLK_SPACE));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_CAPSLOCK, SDLK_CAPSLOCK));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F1, SDLK_F1));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F2, SDLK_F2));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F3, SDLK_F3));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F4, SDLK_F4));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F5, SDLK_F5));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F6, SDLK_F6));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F7, SDLK_F7));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F8, SDLK_F8));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F9, SDLK_F9));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F10, SDLK_F10));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_NUMLOCK, SDLK_NUMLOCKCLEAR));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_SCROLLLOCK, SDLK_SCROLLLOCK));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_7, SDLK_KP_7));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_8, SDLK_KP_8));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_9, SDLK_KP_9));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_MINUS, SDLK_KP_MINUS));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_4, SDLK_KP_4));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_5, SDLK_KP_5));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_6, SDLK_KP_6));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_PLUS, SDLK_KP_PLUS));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_1, SDLK_KP_1));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_2, SDLK_KP_2));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_3, SDLK_KP_3));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_0, SDLK_KP_0));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_FULLSTOP, SDLK_KP_PERIOD));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F11, SDLK_F11));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_F12, SDLK_F12));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RCTRL, SDLK_RCTRL));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KP_BACKSLASH, SDLK_KP_DIVIDE));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RALT, SDLK_RALT));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_HOME, SDLK_HOME));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_UPARROW, SDLK_UP));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_PAGEUP, SDLK_PAGEUP));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_LEFTARROW, SDLK_LEFT));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_RIGHTARROW, SDLK_RIGHT));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_END, SDLK_END));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_DOWNARROW, SDLK_DOWN));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_PAGEDOWN, SDLK_PAGEDOWN));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_INSERT, SDLK_INSERT));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_DELETE, SDLK_DELETE));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_KPENTER, SDLK_KP_ENTER));
	KEY_CODE_to_SDLKey.insert(std::pair<KEY_CODE, SDL_Keycode>(KEY_IGNORE, SDL_Keycode(5190)));

	std::map<KEY_CODE, SDL_Keycode>::iterator it;
	for (it = KEY_CODE_to_SDLKey.begin(); it != KEY_CODE_to_SDLKey.end(); it++)
	{
		SDLKey_to_KEY_CODE.insert(std::pair<SDL_Keycode, KEY_CODE>(it->second, it->first));
	}
}

static inline KEY_CODE sdlKeyToKeyCode(SDL_Keycode key)
{
	std::map<SDL_Keycode, KEY_CODE >::iterator it;

	it = SDLKey_to_KEY_CODE.find(key);
	if (it != SDLKey_to_KEY_CODE.end())
	{
		return it->second;
	}
	return (KEY_CODE)key;
}

static inline SDL_Keycode keyCodeToSDLKey(KEY_CODE code)
{
	std::map<KEY_CODE, SDL_Keycode>::iterator it;

	it = KEY_CODE_to_SDLKey.find(code);
	if (it != KEY_CODE_to_SDLKey.end())
	{
		return it->second;
	}
	return (SDL_Keycode)code;
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
		return;	// Buffer full.
	}

	// Add key to buffer.
	pEndBuffer->key = key;
	pEndBuffer->unicode = unicode;
	pEndBuffer = pNext;
}

void keyScanToString(KEY_CODE code, char *ascii, UDWORD maxStringSize)
{
	if (code == KEY_LCTRL)
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

	if (code < KEY_MAXSCAN)
	{
		snprintf(ascii, maxStringSize, "%s", SDL_GetKeyName(keyCodeToSDLKey(code)));
		if (ascii[0] >= 'a' && ascii[0] <= 'z' && ascii[1] != 0)
		{
			// capitalize
			ascii[0] += 'A' - 'a';
			return;
		}
	}
	else
	{
		strcpy(ascii, "???");
	}
}

void mouseKeyCodeToString(const MOUSE_KEY_CODE code, char* ascii, const int maxStringLength)
{
	switch (code)
	{
	case MOUSE_KEY_CODE::MOUSE_LMB:
		strcpy(ascii, "Mouse Left");
		break;
	case MOUSE_KEY_CODE::MOUSE_MMB:
		strcpy(ascii, "Mouse Middle");
		break;
	case MOUSE_KEY_CODE::MOUSE_RMB:
		strcpy(ascii, "Mouse Right");
		break;
	case MOUSE_KEY_CODE::MOUSE_X1:
		strcpy(ascii, "Mouse 4");
		break;
	case MOUSE_KEY_CODE::MOUSE_X2:
		strcpy(ascii, "Mouse 5");
		break;
	case MOUSE_KEY_CODE::MOUSE_WUP:
		strcpy(ascii, "Mouse Wheel Up");
		break;
	case MOUSE_KEY_CODE::MOUSE_WDN:
		strcpy(ascii, "Mouse Wheel Down");
		break;
	default:
		strcpy(ascii, "Mouse ???");
		break;
	}
}

/* Initialise the input module */
void inputInitialise(void)
{
	for (unsigned int i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}

	for (unsigned int i = 0; i < MOUSE_END; i++)
	{
		aMouseState[i].state = KEY_UP;
	}

	pStartBuffer = pInputBuffer;
	pEndBuffer = pInputBuffer;

	dragX = mouseXPos = screenWidth / 2;
	dragY = mouseYPos = screenHeight / 2;
	dragKey = MOUSE_LMB;

}

/* Clear the input buffer */
void inputClearBuffer(void)
{
	pStartBuffer = pInputBuffer;
	pEndBuffer = pInputBuffer;
}

/* Return the next key press or 0 if no key in the buffer.
 * The key returned will have been remapped to the correct ascii code for the
 * windows key map.
 * All key presses are buffered up (including windows auto repeat).
 */
UDWORD inputGetKey(utf_32_char *unicode)
{
	UDWORD	 retVal;

	if (pStartBuffer == pEndBuffer)
	{
		return 0;	// Buffer empty.
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
	// handle the keyboard
	for (unsigned int i = 0; i < KEY_MAXSCAN; i++)
	{
		if (aKeyState[i].state == KEY_PRESSED)
		{
			aKeyState[i].state = KEY_DOWN;
			debug(LOG_NEVER, "This key is DOWN! %x, %d [%s]", i, i, SDL_GetKeyName(keyCodeToSDLKey((KEY_CODE)i)));
		}
		else if (aKeyState[i].state == KEY_RELEASED  ||
		         aKeyState[i].state == KEY_PRESSRELEASE)
		{
			aKeyState[i].state = KEY_UP;
			debug(LOG_NEVER, "This key is UP! %x, %d [%s]", i, i, SDL_GetKeyName(keyCodeToSDLKey((KEY_CODE)i)));
		}
	}

	// handle the mouse
	for (unsigned int i = 0; i < MOUSE_END; i++)
	{
		if (aMouseState[i].state == KEY_PRESSED)
		{
			aMouseState[i].state = KEY_DOWN;
		}
		else if (aMouseState[i].state == KEY_RELEASED
		         || aMouseState[i].state == KEY_DOUBLECLICK
		         || aMouseState[i].state == KEY_PRESSRELEASE)
		{
			aMouseState[i].state = KEY_UP;
		}
	}
	mousePresses.clear();
	mouseWheelSpeed = Vector2i(0, 0);
}

/*!
 * Release all keys (and buttons) when we lose focus
 */
void inputLoseFocus(void)
{
	/* Lost the window focus, have to take this as a global key up */
	for (unsigned int i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}
	for (unsigned int i = 0; i < MOUSE_END; i++)
	{
		aMouseState[i].state = KEY_UP;
	}
}

/* This returns true if the key is currently depressed */
bool keyDown(KEY_CODE code)
{
	ASSERT_OR_RETURN(false, code < KEY_MAXSCAN, "Invalid keycode of %d!", (int)code);
	return (aKeyState[code].state != KEY_UP);
}

/* This returns true if the key went from being up to being down this frame */
bool keyPressed(KEY_CODE code)
{
	ASSERT_OR_RETURN(false, code < KEY_MAXSCAN, "Invalid keycode of %d!", (int)code);
	return ((aKeyState[code].state == KEY_PRESSED) || (aKeyState[code].state == KEY_PRESSRELEASE));
}

/* This returns true if the key went from being down to being up this frame */
bool keyReleased(KEY_CODE code)
{
	ASSERT_OR_RETURN(false, code < KEY_MAXSCAN, "Invalid keycode of %d!", (int)code);
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

Vector2i const& getMouseWheelSpeed()
{
	return mouseWheelSpeed;
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

/*!
 * Handle keyboard events
 */
static void inputHandleKeyEvent(SDL_KeyboardEvent *keyEvent)
{
	switch (keyEvent->type)
	{
	case SDL_KEYDOWN:
	{
		unsigned vk = 0;
		switch (keyEvent->keysym.sym)
		{
		// our "editing" keys for text
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
		case KEY_BACKSPACE:
			vk = INPBUF_BKSPACE;
			break;
		case KEY_TAB:
			vk = INPBUF_TAB;
			break;
		case KEY_RETURN:
			vk = INPBUF_CR;
			break;
		case KEY_ESC:
			vk = INPBUF_ESC;
			break;
		default:
			break;
		}
		// Keycodes without character representations are determined by their scancode bitwise OR-ed with 1<<30 (0x40000000).
		unsigned currentKey = keyEvent->keysym.sym;
		if (vk)
		{
			// Take care of 'editing' keys that were pressed
			inputAddBuffer(vk, 0);
			debug(LOG_INPUT, "Editing key: 0x%x, %d SDLkey=[%s] pressed", vk, vk, SDL_GetKeyName(currentKey));
		}
		else
		{
			// add everything else
			inputAddBuffer(currentKey, 0);
		}

		debug(LOG_INPUT, "Key Code (pressed): 0x%x, %d, [%c] SDLkey=[%s]", currentKey, currentKey, currentKey < 128 && currentKey > 31 ? (char)currentKey : '?', SDL_GetKeyName(currentKey));

		KEY_CODE code = sdlKeyToKeyCode(currentKey);
		if (code >= KEY_MAXSCAN)
		{
			break;
		}
		if (aKeyState[code].state == KEY_UP ||
		    aKeyState[code].state == KEY_RELEASED ||
		    aKeyState[code].state == KEY_PRESSRELEASE)
		{
			// whether double key press or not
			aKeyState[code].state = KEY_PRESSED;
			aKeyState[code].lastdown = 0;
		}
		break;
	}

	case SDL_KEYUP:
	{
		unsigned currentKey = keyEvent->keysym.sym;
		debug(LOG_INPUT, "Key Code (*Depressed*): 0x%x, %d, [%c] SDLkey=[%s]", currentKey, currentKey, currentKey < 128 && currentKey > 31 ? (char)currentKey : '?', SDL_GetKeyName(currentKey));
		KEY_CODE code = sdlKeyToKeyCode(keyEvent->keysym.sym);
		if (code >= KEY_MAXSCAN)
		{
			break;
		}
		if (aKeyState[code].state == KEY_PRESSED)
		{
			aKeyState[code].state = KEY_PRESSRELEASE;
		}
		else if (aKeyState[code].state == KEY_DOWN)
		{
			aKeyState[code].state = KEY_RELEASED;
		}
		break;
	}
	default:
		break;
	}
}

/*!
 * Handle text events (if we were to use SDL2)
*/
void inputhandleText(SDL_TextInputEvent *Tevent)
{
	size_t size = SDL_strlen(Tevent->text);
	if (size)
	{
		if (utf8Buf)
		{
			// clean up memory from last use.
			free(utf8Buf);
			utf8Buf = nullptr;
		}
		size_t newtextsize = 0;
		utf8Buf = UTF8toUTF32(Tevent->text, &newtextsize);
		debug(LOG_INPUT, "Keyboard: text input \"%s\"", Tevent->text);
		for (unsigned i = 0; i < newtextsize / sizeof(utf_32_char); ++i)
		{
			inputAddBuffer(0, utf8Buf[i]);
		}
	}
}

/*!
 * Handle mouse wheel events
 */
static void inputHandleMouseWheelEvent(SDL_MouseWheelEvent *wheel)
{
	mouseWheelSpeed += Vector2i(wheel->x, wheel->y);

	if (wheel->x > 0 || wheel->y > 0)
	{
		aMouseState[MOUSE_WUP].state = KEY_PRESSED;
		aMouseState[MOUSE_WUP].lastdown = 0;
	}
	else if (wheel->x < 0 || wheel->y < 0)
	{
		aMouseState[MOUSE_WDN].state = KEY_PRESSED;
		aMouseState[MOUSE_WDN].lastdown = 0;
	}
}

/*!
 * Handle mouse button events (We can handle up to 5)
 */
static void inputHandleMouseButtonEvent(SDL_MouseButtonEvent *buttonEvent)
{
	mouseXPos = (int)((float)buttonEvent->x / current_displayScaleFactor);
	mouseYPos = (int)((float)buttonEvent->y / current_displayScaleFactor);

	MOUSE_KEY_CODE mouseKeyCode;
	switch (buttonEvent->button)
	{
	case SDL_BUTTON_LEFT: mouseKeyCode = MOUSE_LMB; break;
	case SDL_BUTTON_MIDDLE: mouseKeyCode = MOUSE_MMB; break;
	case SDL_BUTTON_RIGHT: mouseKeyCode = MOUSE_RMB; break;
	case SDL_BUTTON_X1: mouseKeyCode = MOUSE_X1; break;
	case SDL_BUTTON_X2: mouseKeyCode = MOUSE_X2; break;
	default: return;  // Unknown button.
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
		if (aMouseState[mouseKeyCode].state == KEY_UP
		    || aMouseState[mouseKeyCode].state == KEY_RELEASED
		    || aMouseState[mouseKeyCode].state == KEY_PRESSRELEASE)
		{
			// whether double click or not
			if (realTime - aMouseState[mouseKeyCode].lastdown < DOUBLE_CLICK_INTERVAL)
			{
				aMouseState[mouseKeyCode].state = KEY_DOUBLECLICK;
				aMouseState[mouseKeyCode].lastdown = 0;
			}
			else
			{
				aMouseState[mouseKeyCode].state = KEY_PRESSED;
				aMouseState[mouseKeyCode].lastdown = realTime;
			}

			if (mouseKeyCode < MOUSE_X1) // Assume they are draggin' with either LMB|RMB|MMB
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
		else if (aMouseState[mouseKeyCode].state == KEY_DOWN
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
static void inputHandleMouseMotionEvent(SDL_MouseMotionEvent *motionEvent)
{
	switch (motionEvent->type)
	{
	case SDL_MOUSEMOTION:
		/* store the current mouse position */
		mouseXPos = (int)((float)motionEvent->x / current_displayScaleFactor);
		mouseYPos = (int)((float)motionEvent->y / current_displayScaleFactor);

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

static int copied_argc = 0;
static char** copied_argv = nullptr;

// This stage, we only setup keycodes, and copy argc & argv for later use initializing stuff.
void wzMain(int &argc, char **argv)
{
	initKeycodes();

	// Create copies of argc and arv (for later use initializing QApplication for the script engine)
	copied_argv = new char*[argc+1];
	for(int i=0; i < argc; i++) {
		size_t len = strlen(argv[i]) + 1;
		copied_argv[i] = new char[len];
		memcpy(copied_argv[i], argv[i], len);
	}
	copied_argv[argc] = NULL;
	copied_argc = argc;
}

#define MIN_WZ_GAMESCREEN_WIDTH 640
#define MIN_WZ_GAMESCREEN_HEIGHT 480

void handleGameScreenSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	screenWidth = newWidth;
	screenHeight = newHeight;

	pie_SetVideoBufferWidth(screenWidth);
	pie_SetVideoBufferHeight(screenHeight);
	pie_UpdateSurfaceGeometry();

	if (currentScreenResizingStatus == nullptr)
	{
		// The screen size change details are stored in scaled, logical units (points)
		// i.e. the values expect by the game engine.
		currentScreenResizingStatus = new ScreenSizeChange(oldWidth, oldHeight, screenWidth, screenHeight);
	}
	else
	{
		// update the new screen width / height, in case more than one resize message is processed this event loop
		currentScreenResizingStatus->newWidth = screenWidth;
		currentScreenResizingStatus->newHeight = screenHeight;
	}
}

void handleWindowSizeChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	windowWidth = newWidth;
	windowHeight = newHeight;

	// NOTE: This function receives the window size in the window's logical units, but not accounting for the interface scale factor.
	// Therefore, the provided old/newWidth/Height must be divided by the interface scale factor to calculate the new
	// *game* screen logical width / height.
	unsigned int oldScreenWidth = static_cast<unsigned int>(oldWidth / current_displayScaleFactor);
	unsigned int oldScreenHeight = static_cast<unsigned int>(oldHeight / current_displayScaleFactor);
	unsigned int newScreenWidth = static_cast<unsigned int>(newWidth / current_displayScaleFactor);
	unsigned int newScreenHeight = static_cast<unsigned int>(newHeight / current_displayScaleFactor);

	handleGameScreenSizeChange(oldScreenWidth, oldScreenHeight, newScreenWidth, newScreenHeight);

	gfx_api::context::get().handleWindowSizeChange(oldWidth, oldHeight, newWidth, newHeight);
}


void wzGetMinimumWindowSizeForDisplayScaleFactor(unsigned int *minWindowWidth, unsigned int *minWindowHeight, float displayScaleFactor = current_displayScaleFactor)
{
	if (minWindowWidth != nullptr)
	{
		*minWindowWidth = (int)ceil(MIN_WZ_GAMESCREEN_WIDTH * displayScaleFactor);
	}
	if (minWindowHeight != nullptr)
	{
		*minWindowHeight = (int)ceil(MIN_WZ_GAMESCREEN_HEIGHT * displayScaleFactor);
	}
}

void wzGetMaximumDisplayScaleFactorsForWindowSize(unsigned int width, unsigned int height, float *horizScaleFactor, float *vertScaleFactor)
{
	if (horizScaleFactor != nullptr)
	{
		*horizScaleFactor = (float)width / (float)MIN_WZ_GAMESCREEN_WIDTH;
	}
	if (vertScaleFactor != nullptr)
	{
		*vertScaleFactor = (float)height / (float)MIN_WZ_GAMESCREEN_HEIGHT;
	}
}

float wzGetMaximumDisplayScaleFactorForWindowSize(unsigned int width, unsigned int height)
{
	float maxHorizScaleFactor = 0.f, maxVertScaleFactor = 0.f;
	wzGetMaximumDisplayScaleFactorsForWindowSize(width, height, &maxHorizScaleFactor, &maxVertScaleFactor);
	return std::min(maxHorizScaleFactor, maxVertScaleFactor);
}

// returns: the maximum display scale percentage (sourced from wzAvailableDisplayScales), or 0 if window is below the minimum required size for the minimum supported display scale
unsigned int wzGetMaximumDisplayScaleForWindowSize(unsigned int width, unsigned int height)
{
	float maxDisplayScaleFactor = wzGetMaximumDisplayScaleFactorForWindowSize(width, height);
	unsigned int maxDisplayScalePercentage = static_cast<unsigned int>(floor(maxDisplayScaleFactor * 100.f));

	auto availableDisplayScales = wzAvailableDisplayScales();
	std::sort(availableDisplayScales.begin(), availableDisplayScales.end());

	auto maxDisplayScale = std::lower_bound(availableDisplayScales.begin(), availableDisplayScales.end(), maxDisplayScalePercentage);
	if (maxDisplayScale == availableDisplayScales.end())
	{
		// return the largest available display scale
		return availableDisplayScales.back();
	}
	if (*maxDisplayScale != maxDisplayScalePercentage)
	{
		if (maxDisplayScale == availableDisplayScales.begin())
		{
			// no lower display scale to return
			return 0;
		}
		--maxDisplayScale;
	}
	return *maxDisplayScale;
}

unsigned int wzGetMaximumDisplayScaleForCurrentWindowSize()
{
	return wzGetMaximumDisplayScaleForWindowSize(windowWidth, windowHeight);
}

unsigned int wzGetSuggestedDisplayScaleForCurrentWindowSize(unsigned int desiredMaxScreenDimension)
{
	unsigned int maxDisplayScale = wzGetMaximumDisplayScaleForCurrentWindowSize();

	auto availableDisplayScales = wzAvailableDisplayScales();
	std::sort(availableDisplayScales.begin(), availableDisplayScales.end());

	for (auto it = availableDisplayScales.begin(); it != availableDisplayScales.end() && (*it <= maxDisplayScale); it++)
	{
		auto resultingLogicalScreenWidth = (windowWidth * 100) / *it;
		auto resultingLogicalScreenHeight = (windowHeight * 100) / *it;
		if (resultingLogicalScreenWidth <= desiredMaxScreenDimension && resultingLogicalScreenHeight <= desiredMaxScreenDimension)
		{
			return *it;
		}
	}

	return maxDisplayScale;
}

bool wzWindowSizeIsSmallerThanMinimumRequired(unsigned int width, unsigned int height, float displayScaleFactor = current_displayScaleFactor)
{
	unsigned int minWindowWidth = 0, minWindowHeight = 0;
	wzGetMinimumWindowSizeForDisplayScaleFactor(&minWindowWidth, &minWindowHeight, displayScaleFactor);
	return ((width < minWindowWidth) || (height < minWindowHeight));
}

void processScreenSizeChangeNotificationIfNeeded()
{
	if (currentScreenResizingStatus != nullptr)
	{
		// WZ must process the screen size change
		screen_updateGeometry(); // must come after gfx_api::context::handleWindowSizeChange
		gameScreenSizeDidChange(currentScreenResizingStatus->oldWidth, currentScreenResizingStatus->oldHeight, currentScreenResizingStatus->newWidth, currentScreenResizingStatus->newHeight);
		delete currentScreenResizingStatus;
		currentScreenResizingStatus = nullptr;
	}
}

#if defined(WZ_OS_WIN)

# if defined(__has_include)
#  if __has_include(<shellscalingapi.h>)
#   include <shellscalingapi.h>
#  endif
# endif

# if !defined(DPI_ENUMS_DECLARED)
typedef enum PROCESS_DPI_AWARENESS
{
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
# endif
typedef HRESULT (WINAPI *GetProcessDpiAwarenessFunction)(
	HANDLE                hprocess,
	PROCESS_DPI_AWARENESS *value
);
typedef BOOL (WINAPI *IsProcessDPIAwareFunction)();

static bool win_IsProcessDPIAware()
{
	HMODULE hShcore = LoadLibraryW(L"Shcore.dll");
	if (hShcore != NULL)
	{
		GetProcessDpiAwarenessFunction func_getProcessDpiAwareness = reinterpret_cast<GetProcessDpiAwarenessFunction>(reinterpret_cast<void*>(GetProcAddress(hShcore, "GetProcessDpiAwareness")));
		if (func_getProcessDpiAwareness)
		{
			PROCESS_DPI_AWARENESS result = PROCESS_DPI_UNAWARE;
			if (func_getProcessDpiAwareness(nullptr, &result) == S_OK)
			{
				FreeLibrary(hShcore);
				return result != PROCESS_DPI_UNAWARE;
			}
		}
		FreeLibrary(hShcore);
	}
	HMODULE hUser32 = LoadLibraryW(L"User32.dll");
	ASSERT_OR_RETURN(false, hUser32 != NULL, "Unable to get handle to User32?");
	IsProcessDPIAwareFunction func_isProcessDPIAware = reinterpret_cast<IsProcessDPIAwareFunction>(reinterpret_cast<void*>(GetProcAddress(hUser32, "IsProcessDPIAware")));
	bool bIsProcessDPIAware = false;
	if (func_isProcessDPIAware)
	{
		bIsProcessDPIAware = (func_isProcessDPIAware() == TRUE);
	}
	FreeLibrary(hUser32);
	return bIsProcessDPIAware;
}
#endif

#if defined(WZ_OS_WIN) // currently only used on Windows
static bool wzGetDisplayDPI(int displayIndex, float* dpi, float* baseDpi)
{
	const float systemBaseDpi =
#if defined(WZ_OS_WIN) || defined(_WIN32)
	96.f;
#elif defined(WZ_OS_MAC) || defined(__APPLE__)
	72.f;
#elif defined(__ANDROID__)
	160.f;
#else
	// default to 96, but possibly extend if needed
	96.f;
#endif
	if (baseDpi)
	{
		*baseDpi = systemBaseDpi;
	}

	float hdpi, vdpi;
	if (SDL_GetDisplayDPI(displayIndex, nullptr, &hdpi, &vdpi) != 0)
	{
		debug(LOG_WARNING, "Failed to get the display (%d) DPI because : %s", displayIndex, SDL_GetError());
		return false;
	}
	if (dpi)
	{
		*dpi = std::min(hdpi, vdpi);
	}
	return true;
}
#endif

unsigned int wzGetDefaultBaseDisplayScale(int displayIndex)
{
#if defined(WZ_OS_WIN)
	// SDL 2.24.0+ has DPI scaling support on Windows
	SDL_version linked_sdl_version;
	SDL_GetVersion(&linked_sdl_version);
	if (linked_sdl_version.major > 2 || (linked_sdl_version.major == 2 && linked_sdl_version.minor >= 24))
	{
		// Check if the hints have been set to enable it
		if (SDL_GetHintBoolean(SDL_HINT_WINDOWS_DPI_SCALING, SDL_FALSE) == SDL_TRUE)
		{
			// SDL DPI awareness + scaling is enabled
			// Just return 100%
			return 100;
		}
		else
		{
			debug(LOG_INFO, "Can't rely on SDL high-DPI support (not available, or explicitly disabled)");
			// fall-through
		}
	}

	// SDL Windows DPI awareness support is unavailable
	// Thus, all adjustments must be made using the Display Scale feature in WZ itself
	// Calculate a good base game Display Scale based on the actual screen display scale
	if (!win_IsProcessDPIAware())
	{
		return 100;
	}
	float dpi, baseDpi;
	if (!wzGetDisplayDPI(displayIndex, &dpi, &baseDpi))
	{
		// Failed to get the display DPI
		return 100;
	}
	unsigned int approxActualDisplayScale = static_cast<unsigned int>(ceil((dpi / baseDpi) * 100.f));
	auto availableDisplayScales = wzAvailableDisplayScales();
	std::sort(availableDisplayScales.begin(), availableDisplayScales.end());
	auto displayScale = std::lower_bound(availableDisplayScales.begin(), availableDisplayScales.end(), approxActualDisplayScale);
	if (displayScale == availableDisplayScales.end())
	{
		// return the largest available display scale
		return availableDisplayScales.back();
	}
	if (*displayScale != approxActualDisplayScale)
	{
		if (displayScale == availableDisplayScales.begin())
		{
			// no lower display scale to return
			return 100;
		}
		--displayScale;
	}
	return *displayScale;
#elif defined(WZ_OS_MAC)
	// SDL has built-in "high-DPI display" support on Apple platforms
	// Just return 100%
	return 100;
#else
	// SDL has built-in "high-DPI display" support on many other platforms (Wayland [SDL 2.0.10+])
	// For now, rely on that, and just return 100%
	return 100;
#endif
}

bool wzChangeDisplayScale(unsigned int displayScale)
{
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzChangeDisplayScale called when window is not available");
		return false;
	}

	float newDisplayScaleFactor = (float)displayScale / 100.f;

	if (wzWindowSizeIsSmallerThanMinimumRequired(windowWidth, windowHeight, newDisplayScaleFactor))
	{
		// The current window width and/or height are below the required minimum window size
		// for this display scale factor.
		return false;
	}

	// Store the new display scale factor
	setDisplayScale(displayScale);

	// Set the new minimum window size
	unsigned int minWindowWidth = 0, minWindowHeight = 0;
	wzGetMinimumWindowSizeForDisplayScaleFactor(&minWindowWidth, &minWindowHeight, newDisplayScaleFactor);
	SDL_SetWindowMinimumSize(WZwindow, minWindowWidth, minWindowHeight);

	// Update the game's logical screen size
	unsigned int oldScreenWidth = screenWidth, oldScreenHeight = screenHeight;
	unsigned int newScreenWidth = windowWidth, newScreenHeight = windowHeight;
	if (newDisplayScaleFactor > 1.0f)
	{
		newScreenWidth = static_cast<unsigned int>(windowWidth / newDisplayScaleFactor);
		newScreenHeight = static_cast<unsigned int>(windowHeight / newDisplayScaleFactor);
	}
	handleGameScreenSizeChange(oldScreenWidth, oldScreenHeight, newScreenWidth, newScreenHeight);
	gameDisplayScaleFactorDidChange(newDisplayScaleFactor);

	// Update the current mouse coordinates
	// (The prior stored mouseXPos / mouseYPos apply to the old coordinate system, and must be translated to the
	// new game coordinate system. Since the mouse hasn't moved - or it would generate events that override this -
	// the current position with respect to the window (which hasn't changed size) can be queried and used to
	// calculate the new game coordinate system mouse position.)
	//
	int windowMouseXPos = 0, windowMouseYPos = 0;
	SDL_GetMouseState(&windowMouseXPos, &windowMouseYPos);
	debug(LOG_WZ, "Old mouse position: %d, %d", mouseXPos, mouseYPos);
	mouseXPos = (int)((float)windowMouseXPos / current_displayScaleFactor);
	mouseYPos = (int)((float)windowMouseYPos / current_displayScaleFactor);
	debug(LOG_WZ, "New mouse position: %d, %d", mouseXPos, mouseYPos);


	processScreenSizeChangeNotificationIfNeeded();

	return true;
}

bool wzChangeCursorScale(unsigned int cursorScale)
{
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzChangeCursorScale called when window is not available");
		return false;
	}

	if (cursorScale == war_getCursorScale())
	{
		return true;
	}

	war_setCursorScale(cursorScale); // must be set before cursors are reinit
	wzSDLReinitCursors();
	return true;
}

bool wzReduceDisplayScalingIfNeeded(int currWidth, int currHeight)
{
	// Check whether the desired window size is smaller than the minimum required for the current Display Scale
	if (wzWindowSizeIsSmallerThanMinimumRequired(currWidth, currHeight))
	{
		// The new window size is smaller than the minimum required size for the current display scale level.

		unsigned int maxDisplayScale = wzGetMaximumDisplayScaleForWindowSize(currWidth, currHeight);
		if (maxDisplayScale < 100)
		{
			// Cannot adjust display scale factor below 1. Desired window size is below the minimum supported.
			debug(LOG_WZ, "Size (%d x %d) is smaller than the minimum supported at a 100%% display scale", currWidth, currHeight);
			return false;
		}

		// Adjust the current display scale level to the nearest supported level.
		debug(LOG_WZ, "The current Display Scale (%d%%) is too high for the desired window size. Reducing the current Display Scale to the maximum possible for the desired window size: %d%%.", current_displayScale, maxDisplayScale);
		wzChangeDisplayScale(maxDisplayScale);

		// Store the new display scale
		war_SetDisplayScale(maxDisplayScale);
	}

	return true;
}

bool wzChangeFullscreenDisplayMode(int screen, unsigned int width, unsigned int height) // TODO: Take refresh rate as well...
{
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzChangeFullscreenDisplayMode called when window is not available");
		return false;
	}
	debug(LOG_INFO, "Changing fullscreen mode to [%d] %dx%d", screen, width, height);

	bool hasPrior = true;
	int priorScreen = SDL_GetWindowDisplayIndex(WZwindow);
	if (priorScreen < 0)
	{
		debug(LOG_WZ, "Failed to get current screen index: %s", SDL_GetError());
		priorScreen = screenIndex;
	}
	SDL_DisplayMode prior = { 0, 0, 0, 0, 0 };
	if (SDL_GetWindowDisplayMode(WZwindow, &prior) != 0)
	{
		// Failed to get current fullscreen display mode for window?
		debug(LOG_WZ, "Failed to get current WindowDisplayMode: %s", SDL_GetError());
		// Proceed with defaults...
		hasPrior = false;
	}

	if (screen != priorScreen)
	{
		debug(LOG_ERROR, "Currently, switching to a fullscreen display mode on a different screen is unsupported. Move the window to the desired display first and try again.");
		return false;
	}

	SDL_DisplayMode	closest;
	closest.format = closest.w = closest.h = closest.refresh_rate = 0;
	closest.driverdata = 0;

	// Find closest available display mode
	SDL_DisplayMode desired;
	desired.format = 0;
	desired.w = width;
	desired.h = height;
	desired.refresh_rate = 0;
	desired.driverdata = 0;


	if (width == 0 || height == 0)
	{
		debug(LOG_INFO, "Getting desktop display mode");
		if (SDL_GetDesktopDisplayMode(screen, &closest) != 0)
		{
			debug(LOG_INFO, "Unable to get desktop DisplayMode?: %s", SDL_GetError());
			return false;
		}
	}
	else
	{
		if (SDL_GetClosestDisplayMode(screen, &desired, &closest) == NULL)
		{
			// no match was found
			debug(LOG_INFO, "No closest DisplayMode found ([%d] [%u x %u]); error: %s", screen, width, height, SDL_GetError());
			return false;
		}
	}

	int result = SDL_SetWindowDisplayMode(WZwindow, &closest);
	if (result != 0)
	{
		// SDL_SetWindowDisplayMode failed
		debug(LOG_INFO, "SDL_SetWindowDisplayMode ([%d] [%u x %u]) failed: %s", screen, width, height, SDL_GetError());
		return false;
	}

	if (wzGetCurrentWindowMode() == WINDOW_MODE::fullscreen)
	{
		// If we are already in fullscreen mode, *also* trigger a SetWindowSize
		// to ensure everything (including drawable size) gets properly updated
		int currWidth = 0, currHeight = 0;
		SDL_GetWindowSize(WZwindow, &currWidth, &currHeight);

		// Check that logical window size isn't < minimum required
		if ((currWidth < MIN_WZ_GAMESCREEN_WIDTH) || (currHeight < MIN_WZ_GAMESCREEN_HEIGHT))
		{
			// Revert to prior display mode
			result = SDL_SetWindowDisplayMode(WZwindow, (hasPrior) ? &prior : nullptr);
			if (result != 0)
			{
				// SDL_SetWindowDisplayMode failed - unable to revert??
				debug(LOG_ERROR, "Failed to revert to prior WindowDisplayMode: %s", SDL_GetError());
			}
			return false;
		}

		SDL_SetWindowSize(WZwindow, currWidth, currHeight);

		// Reduce display scaling if needed
		if (!wzReduceDisplayScalingIfNeeded(currWidth, currHeight))
		{
			// Desired window size is below the minimum supported
			// Revert to prior display mode
			result = SDL_SetWindowDisplayMode(WZwindow, (hasPrior) ? &prior : nullptr);
			if (result != 0)
			{
				// SDL_SetWindowDisplayMode failed - unable to revert??
				debug(LOG_ERROR, "Failed to revert to prior WindowDisplayMode: %s", SDL_GetError());
			}
			return false;
		}

		// Store the updated screenIndex
		screenIndex = screen;
	}

	war_SetFullscreenModeScreen(screen);
	war_SetFullscreenModeWidth(closest.w);
	war_SetFullscreenModeHeight(closest.h);

	return true;
}

bool wzChangeWindowResolution(int screen, unsigned int width, unsigned int height)
{
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzChangeWindowResolution called when window is not available");
		return false;
	}
	debug(LOG_WZ, "Attempt to change resolution to [%d] %dx%d", screen, width, height);

#if defined(WZ_OS_MAC)
	// Workaround for SDL (2.0.5) quirk on macOS:
	//	When the green titlebar button is used to fullscreen the app in a new space:
	//		- SDL does not return SDL_WINDOW_MAXIMIZED nor SDL_WINDOW_FULLSCREEN.
	//		- Attempting to change the window resolution "succeeds" (in that the new window size is "set" and returned
	//		  by the SDL GetWindowSize functions).
	//		- But other things break (ex. mouse coordinate translation) if the resolution is changed while the window
	//        is maximized in this way.
	//		- And the GL drawable size remains unchanged.
	//		- So if it's been fullscreened by the user like this, but doesn't show as SDL_WINDOW_FULLSCREEN,
	//		  prevent window resolution changes.
	if (cocoaIsSDLWindowFullscreened(WZwindow) && !wzIsFullscreen())
	{
		debug(LOG_WZ, "The main window is fullscreened, but SDL doesn't think it is. Changing window resolution is not possible in this state. (SDL Bug).");
		return false;
	}
#endif

	// Get current window size + position + bounds
	int prev_x = 0, prev_y = 0, prev_width = 0, prev_height = 0;
	SDL_GetWindowPosition(WZwindow, &prev_x, &prev_y);
	SDL_GetWindowSize(WZwindow, &prev_width, &prev_height);

	// Get the usable bounds for the current screen
	SDL_Rect bounds;
	if (wzIsFullscreen())
	{
		// When in fullscreen mode, obtain the screen's overall bounds
		if (SDL_GetDisplayBounds(screen, &bounds) != 0) {
			debug(LOG_ERROR, "Failed to get display bounds for screen: %d", screen);
			return false;
		}
		debug(LOG_WZ, "SDL_GetDisplayBounds for screen [%d]: pos %d x %d : res %d x %d", screen, (int)bounds.x, (int)bounds.y, (int)bounds.w, (int)bounds.h);
	}
	else
	{
		// When in windowed mode, obtain the screen's *usable* display bounds
		if (SDL_GetDisplayUsableBounds(screen, &bounds) != 0) {
			debug(LOG_ERROR, "Failed to get usable display bounds for screen: %d", screen);
			return false;
		}
		debug(LOG_WZ, "SDL_GetDisplayUsableBounds for screen [%d]: pos %d x %d : WxH %d x %d", screen, (int)bounds.x, (int)bounds.y, (int)bounds.w, (int)bounds.h);

		// Verify that the desired window size does not exceed the usable bounds of the specified display.
		if ((width > bounds.w) || (height > bounds.h))
		{
			debug(LOG_WZ, "Unable to change window size to (%d x %d) because it is larger than the screen's usable bounds", width, height);
			return false;
		}
	}

	// Check whether the desired window size is smaller than the minimum required for the current Display Scale
	unsigned int priorDisplayScale = current_displayScale;
	if (wzWindowSizeIsSmallerThanMinimumRequired(width, height))
	{
		// The new window size is smaller than the minimum required size for the current display scale level.

		unsigned int maxDisplayScale = wzGetMaximumDisplayScaleForWindowSize(width, height);
		if (maxDisplayScale < 100)
		{
			// Cannot adjust display scale factor below 1. Desired window size is below the minimum supported.
			debug(LOG_WZ, "Unable to change window size to (%d x %d) because it is smaller than the minimum supported at a 100%% display scale", width, height);
			return false;
		}

		// Adjust the current display scale level to the nearest supported level.
		debug(LOG_WZ, "The current Display Scale (%d%%) is too high for the desired window size. Reducing the current Display Scale to the maximum possible for the desired window size: %d%%.", current_displayScale, maxDisplayScale);
		wzChangeDisplayScale(maxDisplayScale);

		// Store the new display scale
		war_SetDisplayScale(maxDisplayScale);
	}

	// Position the window (centered) on the screen (for its upcoming new size)
	SDL_SetWindowPosition(WZwindow, SDL_WINDOWPOS_CENTERED_DISPLAY(screen), SDL_WINDOWPOS_CENTERED_DISPLAY(screen));

	// Change the window size
	// NOTE: Changing the window size will trigger an SDL window size changed event which will handle recalculating layout.
	SDL_SetWindowSize(WZwindow, width, height);

	// Check that the new size is the desired size
	int resultingWidth, resultingHeight = 0;
	SDL_GetWindowSize(WZwindow, &resultingWidth, &resultingHeight);
	if (resultingWidth != width || resultingHeight != height) {
		// Attempting to set the resolution failed
		debug(LOG_WZ, "Attempting to change the resolution to %dx%d seems to have failed (result: %dx%d).", width, height, resultingWidth, resultingHeight);

		// Revert to the prior position + resolution + display scale, and return false
		SDL_SetWindowSize(WZwindow, prev_width, prev_height);
		SDL_SetWindowPosition(WZwindow, prev_x, prev_y);
		if (current_displayScale != priorDisplayScale)
		{
			// Reverse the correction applied to the Display Scale to support the desired resolution.
			wzChangeDisplayScale(priorDisplayScale);
			war_SetDisplayScale(priorDisplayScale);
		}
		return false;
	}

	// Store the updated screenIndex
	screenIndex = screen;

	return true;
}

MinimizeOnFocusLossBehavior wzGetCurrentMinimizeOnFocusLossBehavior()
{
	const char* hint = SDL_GetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS);
	if (!hint || !*hint || SDL_strcasecmp(hint, "auto") == 0)
	{
		return MinimizeOnFocusLossBehavior::Auto;
	}
	bool bValue = SDL_GetHintBoolean(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, SDL_FALSE);
	return (bValue) ? MinimizeOnFocusLossBehavior::On_Fullscreen : MinimizeOnFocusLossBehavior::Off;
}

void wzSetMinimizeOnFocusLoss(MinimizeOnFocusLossBehavior behavior)
{
#if !defined(__EMSCRIPTEN__)
	const char* value = "auto";
	switch (behavior)
	{
		case MinimizeOnFocusLossBehavior::Off:
			value = "0";
			break;
		case MinimizeOnFocusLossBehavior::On_Fullscreen:
			value = "1";
			break;
		case MinimizeOnFocusLossBehavior::Auto:
			value = "auto";
			break;
		default:
			return;
	}
	SDL_SetHintWithPriority(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, value, SDL_HINT_OVERRIDE);
#else
	// no-op on Emscripten
#endif
}

// Returns the current window screen, width, and height
void wzGetWindowResolution(int *screen, unsigned int *width, unsigned int *height)
{
	ASSERT_OR_RETURN(, WZwindow != nullptr, "wzGetWindowResolution called when window is not available");

	if (screen != nullptr)
	{
		*screen = screenIndex;
	}

	int currentWidth = 0, currentHeight = 0;
	SDL_GetWindowSize(WZwindow, &currentWidth, &currentHeight);
	assert(currentWidth >= 0);
	assert(currentHeight >= 0);
	if (width != nullptr)
	{
		*width = currentWidth;
	}
	if (height != nullptr)
	{
		*height = currentHeight;
	}
}

static SDL_WindowFlags SDL_backend(const video_backend& backend)
{
	switch (backend)
	{
#if defined(WZ_BACKEND_DIRECTX)
		case video_backend::directx: // because DirectX is supported via OpenGLES (LibANGLE)
#endif
		case video_backend::opengl:
		case video_backend::opengles:
			return SDL_WINDOW_OPENGL;
		case video_backend::vulkan:
#if SDL_VERSION_ATLEAST(2, 0, 6)
			return SDL_WINDOW_VULKAN;
#else
			debug(LOG_FATAL, "The version of SDL used for compilation does not support SDL_WINDOW_VULKAN");
			break;
#endif
		case video_backend::num_backends:
			debug(LOG_FATAL, "Should never happen");
			break;
	}
	return SDL_WindowFlags{};
}

bool shouldResetGfxBackendPrompt_internal(video_backend currentBackend, video_backend newBackend, std::string failureVerb = "initialize", std::string failedToInitializeObject = "graphics", std::string additionalErrorDetails = "")
{
	// Offer to reset to the specified gfx backend
	std::string resetString = std::string("Reset to ") + to_display_string(newBackend) + "";
	const SDL_MessageBoxButtonData buttons[] = {
	   { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, resetString.c_str() },
	   { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Not Now" },
	};
	std::string titleString = std::string("Warzone: Failed to ") + failureVerb + " " + failedToInitializeObject;
	std::string messageString = std::string("Failed to ") + failureVerb + " " + failedToInitializeObject + " for backend: " + to_display_string(currentBackend) + ".\n\n";
	if (!additionalErrorDetails.empty())
	{
		messageString += "Error Details: \n\"" + additionalErrorDetails + "\"\n\n";
	}
	messageString += "Do you want to reset the graphics backend to: " + to_display_string(newBackend) + "?";
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR, /* .flags */
		WZwindow, /* .window */
		titleString.c_str(), /* .title */
		messageString.c_str(), /* .message */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		nullptr /* .colorScheme */
	};
	int buttonid;
	if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
		// error displaying message box
		debug(LOG_FATAL, "Failed to display message box");
		return false;
	}
	if (buttonid == 1)
	{
		return true;
	}
	return false;
}

bool shouldResetGfxBackendPrompt(video_backend currentBackend, video_backend newBackend, std::string failedToInitializeObject = "graphics", std::string additionalErrorDetails = "")
{
	return shouldResetGfxBackendPrompt_internal(currentBackend, newBackend, "initialize", failedToInitializeObject, additionalErrorDetails);
}

void resetGfxBackend(video_backend newBackend, bool displayRestartMessage = true)
{
	war_setGfxBackend(newBackend);
	if (displayRestartMessage)
	{
		std::string title = std::string("Backend reset to: ") + to_display_string(newBackend);
		wzDisplayDialog(Dialog_Information, title.c_str(), "(Note: Do not specify a --gfxbackend option, or it will override this new setting.)\n\nPlease restart Warzone 2100 to use the new graphics setting.");
	}
}

bool wzPromptToChangeGfxBackendOnFailure(std::string additionalErrorDetails /*= ""*/)
{
	if (!WZbackend.has_value())
	{
		ASSERT(false, "Can't reset gfx backend when there is no gfx backend.");
		return false;
	}
	video_backend defaultBackend = wzGetNextFallbackGfxBackendForCurrentSystem(WZbackend.value());
	if (WZbackend.value() != defaultBackend)
	{
		if (shouldResetGfxBackendPrompt_internal(WZbackend.value(), defaultBackend, "draw", "graphics", additionalErrorDetails))
		{
			resetGfxBackend(defaultBackend);
			saveGfxConfig(); // must force-persist the new value before returning!
			return true;
		}
	}
	else
	{
		// Display message that there was a failure with the gfx backend (but there's no other backend to offer changing it to?)
		std::string title = std::string("Graphics Error: ") + to_display_string(WZbackend.value());
		std::string messageString = std::string("An error occured with graphics backend: ") + to_display_string(WZbackend.value()) + ".\n\n";
		if (!additionalErrorDetails.empty())
		{
			messageString += "Error Details: \n\"" + additionalErrorDetails + "\"\n\n";
		}
		messageString += "Warzone 2100 will now close.";
		wzDisplayDialog(Dialog_Error, title.c_str(), messageString.c_str());
	}
	return false;
}

bool wzSDLOneTimeInit()
{
	const Uint32 sdl_init_flags = SDL_INIT_EVENTS | SDL_INIT_TIMER;
	if (!(SDL_WasInit(sdl_init_flags) == sdl_init_flags))
	{
		if (SDL_Init(sdl_init_flags) != 0)
		{
			debug(LOG_ERROR, "Error: Could not initialise SDL (%s).", SDL_GetError());
			return false;
		}

		if ((SDL_IsTextInputActive() != SDL_FALSE) && (GetTextEventsOwner == nullptr))
		{
			// start text input disabled
			SDL_StopTextInput();
		}
	}

	if (wzSDLAppEvent == ((Uint32)-1))
	{
		wzSDLAppEvent = SDL_RegisterEvents(1);
		if (wzSDLAppEvent == ((Uint32)-1))
		{
			// Failed to register app-defined event with SDL
			debug(LOG_ERROR, "Error: Failed to register app-defined SDL event (%s).", SDL_GetError());
			return false;
		}
	}

	return true;
}

static bool wzSDLOneTimeInitSubsystem(uint32_t subsystem_flag)
{
	if (SDL_WasInit(subsystem_flag) == subsystem_flag)
	{
		// already initialized
		return true;
	}
	if (SDL_InitSubSystem(subsystem_flag) != 0)
	{
		debug(LOG_WZ, "SDL_InitSubSystem(%" PRIu32 ") failed", subsystem_flag);
		return false;
	}
	if ((SDL_IsTextInputActive() != SDL_FALSE) && (GetTextEventsOwner == nullptr))
	{
		// start text input disabled
		SDL_StopTextInput();
	}
	return true;
}

void wzSDLPreWindowCreate_InitOpenGLAttributes(int antialiasing, bool useOpenGLES, bool useOpenGLESLibrary)
{
	// Set OpenGL attributes before creating the SDL Window

	// Set minimum number of bits for the RGB channels of the color buffer
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
#if defined(WZ_OS_WIN)
	if (useOpenGLES)
	{
		// Always force minimum 8-bit color channels when using OpenGL ES on Windows
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	}
#endif

	// Set the double buffer OpenGL attribute.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Request a 24-bit depth buffer.
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Enable stencil buffer, needed for shadows to work.
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	if (antialiasing)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, antialiasing);
	}

	if (!sdl_OpenGL_Impl::configureOpenGLContextRequest(sdl_OpenGL_Impl::getInitialContextRequest(useOpenGLES), useOpenGLESLibrary))
	{
		// Failed to configure OpenGL context request
		debug(LOG_FATAL, "Failed to configure OpenGL context request");
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
}

void wzSDLPreWindowCreate_InitVulkanLibrary()
{
#if defined(WZ_OS_MAC)
	// Attempt to explicitly load libvulkan.dylib with a full path
	// to support situations where run-time search paths using @executable_path are prohibited
	std::string fullPathToVulkanLibrary = cocoaGetFrameworksPath("libvulkan.dylib");
	if (SDL_Vulkan_LoadLibrary(fullPathToVulkanLibrary.c_str()) != 0)
	{
		debug(LOG_ERROR, "Failed to explicitly load Vulkan library");
	}
#else
	// rely on SDL's built-in loading paths / behavior
#endif
}

// This stage, we handle display mode setting
optional<SDL_gfx_api_Impl_Factory::Configuration> wzMainScreenSetup_CreateVideoWindow(const video_backend& backend, int antialiasing, WINDOW_MODE fullscreen, int vsync, bool highDPI)
{
	const bool useOpenGLES = (backend == video_backend::opengles)
#if defined(WZ_BACKEND_DIRECTX)
		|| (backend == video_backend::directx)
#endif
	;
	const bool useOpenGLESLibrary = false
#if defined(WZ_BACKEND_DIRECTX)
		|| (backend == video_backend::directx)
#endif
	;
	const bool usesSDLBackend_OpenGL = useOpenGLES || (backend == video_backend::opengl);

	// Initialize video subsystem (if not yet initialized)
	if (!wzSDLOneTimeInitSubsystem(SDL_INIT_VIDEO))
	{
		debug(LOG_FATAL, "Error: Could not initialise SDL video subsystem (%s).", SDL_GetError());
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	// ensure "fullscreen" is in the supported fullscreen modes for the current system
	auto supportedFullscreenModes = wzSupportedWindowModes();
	if (std::find(supportedFullscreenModes.begin(), supportedFullscreenModes.end(), fullscreen) == supportedFullscreenModes.end())
	{
		debug(LOG_ERROR, "Unsupported fullscreen mode specified: %d; using default", static_cast<int>(fullscreen));
		fullscreen = WINDOW_MODE::windowed;
		war_setWindowMode(fullscreen); // persist the change
	}

	if (usesSDLBackend_OpenGL)
	{
		wzSDLPreWindowCreate_InitOpenGLAttributes(antialiasing, useOpenGLES, useOpenGLESLibrary);
	}
	else if (backend == video_backend::vulkan)
	{
		wzSDLPreWindowCreate_InitVulkanLibrary();
	}

	// Populated our resolution list (does all displays now)
	SDL_DisplayMode	displaymode;
	struct screeninfo screenlist;
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)		// How many monitors we got
	{
		optional<float> dpiAdjustmentFactor = nullopt;
#if defined(WZ_OS_WIN) // currently only used on Windows
		// get DPI for this display (DPI-aware Windows returns pixel values, so we must use this to convert to logical values for minimum checks!)
		float dpi, baseDpi;
		if (wzGetDisplayDPI(i, &dpi, &baseDpi))
		{
			if (dpi != baseDpi)
			{
				dpiAdjustmentFactor = ceil(dpi / baseDpi);
			}
		}
#endif

		int numdisplaymodes = SDL_GetNumDisplayModes(i);	// Get the number of display modes on this monitor
		for (int j = 0; j < numdisplaymodes; j++)
		{
			displaymode.format = displaymode.w = displaymode.h = displaymode.refresh_rate = 0;
			displaymode.driverdata = 0;
			if (SDL_GetDisplayMode(i, j, &displaymode) < 0)
			{
				debug(LOG_FATAL, "SDL_LOG_CATEGORY_APPLICATION error:%s", SDL_GetError());
				SDL_Quit();
				exit(EXIT_FAILURE);
			}

			debug(LOG_WZ, "Monitor [%d] %dx%d %d %s", i, displaymode.w, displaymode.h, displaymode.refresh_rate, SDL_GetPixelFormatName(displaymode.format));
			int logicalWidth = displaymode.w;
			int logicalHeight = displaymode.h;
			if (dpiAdjustmentFactor.has_value())
			{
				logicalWidth = static_cast<int>(ceil(static_cast<float>(logicalWidth) / dpiAdjustmentFactor.value()));
				logicalHeight = static_cast<int>(ceil(static_cast<float>(logicalHeight) / dpiAdjustmentFactor.value()));
			}
			if ((logicalWidth < MIN_WZ_GAMESCREEN_WIDTH) || (logicalHeight < MIN_WZ_GAMESCREEN_HEIGHT))
			{
				debug(LOG_WZ, "Monitor mode logical resolution < %d x %d -- discarding entry", MIN_WZ_GAMESCREEN_WIDTH, MIN_WZ_GAMESCREEN_HEIGHT);
			}
			else if (displaymode.refresh_rate < 59)
			{
				debug(LOG_WZ, "Monitor mode refresh rate < 59 -- discarding entry");
				// only store 60Hz & higher modes, some display report 59 on Linux
			}
			else
			{
				screenlist.width = displaymode.w;
				screenlist.height = displaymode.h;
				screenlist.refresh_rate = displaymode.refresh_rate;
				screenlist.screen = i;		// which monitor this belongs to
				displaylist.push_back(screenlist);
			}
		}
	}

	SDL_DisplayMode current = { 0, 0, 0, 0, 0 };
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
	{
		int display = SDL_GetCurrentDisplayMode(i, &current);
		if (display != 0)
		{
			debug(LOG_FATAL, "Can't get the current display mode, because: %s", SDL_GetError());
			SDL_Quit();
			exit(EXIT_FAILURE);
		}
		debug(LOG_WZ, "Monitor [%d] %dx%d %d", i, current.w, current.h, current.refresh_rate);
	}

	// populate with the saved configuration values (if we had any)
	int desiredWindowWidth = (fullscreen == WINDOW_MODE::fullscreen) ? war_GetFullscreenModeWidth() : war_GetWidth();
	int desiredWindowHeight = (fullscreen == WINDOW_MODE::fullscreen) ? war_GetFullscreenModeHeight() : war_GetHeight();

	// NOTE: Prior to wzMainScreenSetup being run, the display system is populated with the window width + height
	// (i.e. not taking into account the game display scale). This function later sets the display system
	// to the *game screen* width and height (taking into account the display scale).

	if (desiredWindowWidth == 0 || desiredWindowHeight == 0)
	{
		windowWidth = current.w;
		windowHeight = current.h;
	}
	else
	{
		windowWidth = desiredWindowWidth;
		windowHeight = desiredWindowHeight;
	}

	setDisplayScale(war_GetDisplayScale());

	SDL_Rect bounds;
	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++)
	{
		SDL_GetDisplayBounds(i, &bounds);
		debug(LOG_WZ, "Monitor %d: pos %d x %d : res %d x %d", i, (int)bounds.x, (int)bounds.y, (int)bounds.w, (int)bounds.h);
	}
	const int currentNumDisplays = SDL_GetNumVideoDisplays();
	if (currentNumDisplays < 1)
	{
		debug(LOG_FATAL, "SDL_GetNumVideoDisplays returned: %d, with error: %s", currentNumDisplays, SDL_GetError());
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	// Check desired screen index values versus current system
	if (war_GetScreen() > currentNumDisplays)
	{
		debug(LOG_WARNING, "Invalid screen [%d] defined in configuration; there are only %d displays; falling back to display 0", war_GetScreen(), currentNumDisplays);
		war_SetScreen(0);
	}
	if (war_GetFullscreenModeScreen() > currentNumDisplays || war_GetFullscreenModeScreen() < 0)
	{
		debug(LOG_WARNING, "Invalid fullscreen screen [%d] defined in configuration; there are only %d displays; falling back to display 0", war_GetFullscreenModeScreen(), currentNumDisplays);
		war_SetFullscreenModeScreen(0);
	}

	screenIndex = (fullscreen == WINDOW_MODE::fullscreen) ? war_GetFullscreenModeScreen() : war_GetScreen();

	// Calculate the minimum window size (in *logical* points) given the current display scale
	unsigned int minWindowWidth = 0, minWindowHeight = 0;
	wzGetMinimumWindowSizeForDisplayScaleFactor(&minWindowWidth, &minWindowHeight);

	if (fullscreen != WINDOW_MODE::fullscreen)
	{
		if (war_getAutoAdjustDisplayScale())
		{
			// Since SDL (at least <= 2.0.14) does not provide built-in support for high-DPI displays on *some* platforms
			// (example: Windows), do our best to bump up the game's Display Scale setting to be a better match if the screen
			// on which the game starts has a higher effective Display Scale than the game's starting Display Scale setting.
			unsigned int screenBaseDisplayScale = wzGetDefaultBaseDisplayScale(screenIndex);
			if (screenBaseDisplayScale > wzGetCurrentDisplayScale())
			{
				// When bumping up the display scale, also increase the target window size proportionally
				debug(LOG_WZ, "Increasing game Display Scale to better match calculated default base display scale: %u%% -> %u%%", wzGetCurrentDisplayScale(), screenBaseDisplayScale);
				unsigned int priorDisplayScale = wzGetCurrentDisplayScale();
				setDisplayScale(screenBaseDisplayScale);
				float displayScaleDiff = (float)screenBaseDisplayScale / (float)priorDisplayScale;
				windowWidth = static_cast<unsigned int>(ceil((float)windowWidth * displayScaleDiff));
				windowHeight = static_cast<unsigned int>(ceil((float)windowHeight * displayScaleDiff));
				war_SetDisplayScale(screenBaseDisplayScale); // save the new display scale configuration
			}
		}

		if ((windowWidth < minWindowWidth) || (windowHeight < minWindowHeight))
		{
			// The desired window width and/or height is lower than the required minimum for the current display scale.
			// Reduce the display scale to the maximum supported (for the desired window size), and recalculate the required minimum window size.
			unsigned int maxDisplayScale = wzGetMaximumDisplayScaleForWindowSize(windowWidth, windowHeight);
			maxDisplayScale = std::max(100u, maxDisplayScale); // if wzGetMaximumDisplayScaleForWindowSize fails, it returns < 100
			setDisplayScale(maxDisplayScale);
			war_SetDisplayScale(maxDisplayScale); // save the new display scale configuration
			wzGetMinimumWindowSizeForDisplayScaleFactor(&minWindowWidth, &minWindowHeight);
		}

		windowWidth = std::max(windowWidth, minWindowWidth);
		windowHeight = std::max(windowHeight, minWindowHeight);

		if (fullscreen == WINDOW_MODE::windowed)
		{
			// Determine the maximum usable windowed size for this display/screen
			SDL_Rect displayUsableBounds = { 0, 0, 0, 0 };
			if (SDL_GetDisplayUsableBounds(screenIndex, &displayUsableBounds) == 0)
			{
				if (displayUsableBounds.w > 0 && displayUsableBounds.h > 0)
				{
					windowWidth = std::min((unsigned int)displayUsableBounds.w, windowWidth);
					windowHeight = std::min((unsigned int)displayUsableBounds.h, windowHeight);
				}
			}
		}
	}

	//// The flags to pass to SDL_CreateWindow
	int video_flags  = SDL_backend(backend) | SDL_WINDOW_SHOWN;

	switch (fullscreen)
	{
		case WINDOW_MODE::desktop_fullscreen:
			video_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			break;
		case WINDOW_MODE::fullscreen:
			video_flags |= SDL_WINDOW_FULLSCREEN;
			break;
		case WINDOW_MODE::windowed:
			// Allow the window to be manually resized, if not fullscreen
			video_flags |= SDL_WINDOW_RESIZABLE;
			break;
	}

	if (highDPI)
	{
		// Allow SDL to enable its built-in High-DPI display support.
		// This flag is ignored on some platforms (ex. Windows is not supported as of SDL 2.0.10),
		// but does support: macOS, Wayland [SDL 2.0.10+].
		video_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
	}

	WZwindow = SDL_CreateWindow(PACKAGE_NAME, SDL_WINDOWPOS_CENTERED_DISPLAY(screenIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(screenIndex), windowWidth, windowHeight, video_flags);

	if (!WZwindow)
	{
		std::string createWindowErrorStr = SDL_GetError();
		video_backend defaultBackend = wzGetNextFallbackGfxBackendForCurrentSystem(backend);
		if ((backend != defaultBackend) && shouldResetGfxBackendPrompt(backend, defaultBackend, "window", createWindowErrorStr))
		{
			resetGfxBackend(defaultBackend);
			return nullopt; // must return so new configuration will be saved
		}
		else
		{
			debug(LOG_FATAL, "Can't create a window, because: %s", createWindowErrorStr.c_str());
		}
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	// Always set the fullscreen mode (so switching works to the desired mode, even if we don't start in fullscreen mode)
	if (!wzChangeFullscreenDisplayMode(screenIndex, war_GetFullscreenModeWidth(), war_GetFullscreenModeHeight()))
	{
		if (fullscreen == WINDOW_MODE::fullscreen)
		{
			debug(LOG_ERROR, "Failed to initialize at fullscreen mode ([%d] [%u x %u]); reverting to windowed mode", screenIndex, war_GetFullscreenModeWidth(), war_GetFullscreenModeHeight());
			wzChangeWindowMode(WINDOW_MODE::windowed);
			fullscreen = WINDOW_MODE::windowed;
		}
	}

	// Get resulting logical window size
	int resultingWidth, resultingHeight = 0;
	SDL_GetWindowSize(WZwindow, &resultingWidth, &resultingHeight);

	// Check that the actual window size matches the desired window size (but not for classic fullscreen mode)
	if ((wzGetCurrentWindowMode() != WINDOW_MODE::fullscreen) && (resultingWidth < minWindowWidth || resultingHeight < minWindowHeight))
	{
		// The created window size (that the system returned) is less than the minimum required window size (for this display scale)
		debug(LOG_WARNING, "Failed to create window at desired resolution: [%d] %d x %d; instead, received window of resolution: [%d] %d x %d; which is below the required minimum size of %d x %d for the current display scale level", war_GetScreen(), windowWidth, windowHeight, war_GetScreen(), resultingWidth, resultingHeight, minWindowWidth, minWindowHeight);

		// Adjust the display scale (if possible)
		unsigned int maxDisplayScale = wzGetMaximumDisplayScaleForWindowSize(resultingWidth, resultingHeight);
		if (maxDisplayScale >= 100)
		{
			// Reduce the display scale
			debug(LOG_WARNING, "Reducing the display scale level to the maximum supported for this window size: %u", maxDisplayScale);
			setDisplayScale(maxDisplayScale);
			war_SetDisplayScale(maxDisplayScale); // save the new display scale configuration
			wzGetMinimumWindowSizeForDisplayScaleFactor(&minWindowWidth, &minWindowHeight);
		}
		else
		{
			// Problem: The resulting window size that the system provided is below the minimum required
			//          for the lowest possible display scale - i.e. the window is just too small
			debug(LOG_ERROR, "The window size created by the system is too small!");

			// TODO: Should probably exit with a fatal error here?
			// For now...

			// Attempt to default to base resolution at 100%
			setDisplayScale(100);
			war_SetDisplayScale(100); // save the new display scale configuration
			wzGetMinimumWindowSizeForDisplayScaleFactor(&minWindowWidth, &minWindowHeight);

			SDL_SetWindowSize(WZwindow, minWindowWidth, minWindowHeight);
			windowWidth = minWindowWidth;
			windowHeight = minWindowHeight;

			// Center window on screen
			SDL_SetWindowPosition(WZwindow, SDL_WINDOWPOS_CENTERED_DISPLAY(screenIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(screenIndex));

			// Re-request resulting window size
			SDL_GetWindowSize(WZwindow, &resultingWidth, &resultingHeight);
		}
	}
	ASSERT(resultingWidth > 0 && resultingHeight > 0, "Invalid - SDL returned window width: %d, window height: %d", resultingWidth, resultingHeight);
	windowWidth = (unsigned int)resultingWidth;
	windowHeight = (unsigned int)resultingHeight;

	// Calculate the game screen's logical dimensions
	screenWidth = windowWidth;
	screenHeight = windowHeight;
	if (current_displayScaleFactor > 1.0f)
	{
		screenWidth = static_cast<unsigned int>(windowWidth / current_displayScaleFactor);
		screenHeight = static_cast<unsigned int>(windowHeight / current_displayScaleFactor);
	}
	pie_SetVideoBufferWidth(screenWidth);
	pie_SetVideoBufferHeight(screenHeight);

	// Set the minimum window size
	SDL_SetWindowMinimumSize(WZwindow, minWindowWidth, minWindowHeight);

#if !defined(WZ_OS_MAC) // Do not use this method to set the window icon on macOS.

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

#if defined(__GNUC__)
	#pragma GCC diagnostic push
	// ignore warning: cast from type 'const unsigned char*' to type 'void*' casts away qualifiers [-Wcast-qual]
	// FIXME?
	#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
	SDL_Surface *surface_icon = SDL_CreateRGBSurfaceFrom((void *)wz2100icon.pixel_data, wz2100icon.width, wz2100icon.height, wz2100icon.bytes_per_pixel * 8,
	                            wz2100icon.width * wz2100icon.bytes_per_pixel, rmask, gmask, bmask, amask);
#if defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif

	if (surface_icon)
	{
		SDL_SetWindowIcon(WZwindow, surface_icon);
		SDL_FreeSurface(surface_icon);
	}
	else
	{
		debug(LOG_ERROR, "Could not set window icon because %s", SDL_GetError());
	}
#endif

	SDL_SetWindowTitle(WZwindow, PACKAGE_NAME);

	SDL_gfx_api_Impl_Factory::Configuration sdl_impl_config;
	sdl_impl_config.useOpenGLES = useOpenGLES;
	sdl_impl_config.useOpenGLESLibrary = useOpenGLESLibrary;
	return sdl_impl_config;
}

bool wzMainScreenSetup_VerifyWindow()
{
	int bitDepth = war_GetVideoBufferDepth();

	int bpp = SDL_BITSPERPIXEL(SDL_GetWindowPixelFormat(WZwindow));
	debug(LOG_WZ, "Bpp = %d format %s" , bpp, SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(WZwindow)));
	if (!bpp)
	{
		debug(LOG_ERROR, "Video mode %ux%u@%dbpp is not supported!", windowWidth, windowHeight, bitDepth);
		return false;
	}
	switch (bpp)
	{
	case 32:
	case 24:		// all is good...
		break;
	case 16:
		wz_info("Using colour depth of %i instead of a 32/24 bit depth (True color).", bpp);
		wz_info("You will experience graphics glitches!");
		break;
	case 8:
		debug(LOG_FATAL, "You don't want to play Warzone with a bit depth of %i, do you?", bpp);
		SDL_Quit();
		exit(1);
		break;
	default:
		debug(LOG_FATAL, "Unsupported bit depth: %i", bpp);
		exit(1);
		break;
	}

	return true;
}

bool wzMainScreenSetup(optional<video_backend> backend, int antialiasing, WINDOW_MODE fullscreen, int vsync, int lodDistanceBiasPercentage, bool highDPI)
{
	// Output linked SDL version
	char buf[512];
	SDL_version linked_sdl_version;
	SDL_GetVersion(&linked_sdl_version);
	ssprintf(buf, "Linked SDL version: %u.%u.%u\n", (unsigned int)linked_sdl_version.major, (unsigned int)linked_sdl_version.minor, (unsigned int)linked_sdl_version.patch);
	addDumpInfo(buf);
	debug(LOG_WZ, "%s", buf);

	if (!wzSDLOneTimeInit())
	{
		// wzSDLOneTimeInit already logged an error on failure
		return false;
	}

#if defined(WZ_OS_MAC)
	// on macOS, support maximizing to a fullscreen space (modern behavior)
	if (SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "1") == SDL_FALSE)
	{
		debug(LOG_WARNING, "Failed to set hint: SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES");
	}
#endif
#if defined(WZ_OS_WIN)
	// on Windows, opt-in to SDL 2.24.0+'s DPI scaling support
	if (linked_sdl_version.major > 2 || (linked_sdl_version.major == 2 && linked_sdl_version.minor >= 24))
	{
		// SDL_HINT_WINDOWS_DPI_AWARENESS does not appear to be needed if SDL_HINT_WINDOWS_DPI_SCALING is set
		// But set it anyway for completeness
		if (SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2") == SDL_FALSE)
		{
			debug(LOG_ERROR, "Failed to set hint: SDL_HINT_WINDOWS_DPI_AWARENESS");
		}
		if (SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1") == SDL_FALSE)
		{
			debug(LOG_ERROR, "Failed to set hint: SDL_HINT_WINDOWS_DPI_SCALING");
		}
	}
#endif
	int minOnFocusLossSettingVal = war_getMinimizeOnFocusLoss();
	if (minOnFocusLossSettingVal < -1 || minOnFocusLossSettingVal > 1)
	{
		minOnFocusLossSettingVal = -1;
	}
	const char* hint = SDL_GetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS);
	if (!hint || !*hint || SDL_strcasecmp(hint, "auto") == 0)
	{
		wzSetMinimizeOnFocusLoss(static_cast<MinimizeOnFocusLossBehavior>(minOnFocusLossSettingVal));
	}
	int toggleFullscreenModeVal = war_getToggleFullscreenMode();
	switch (toggleFullscreenModeVal)
	{
		case -1:
			wzSetToggleFullscreenMode(WINDOW_MODE::desktop_fullscreen);
			break;
		case 1:
			wzSetToggleFullscreenMode(WINDOW_MODE::fullscreen);
			break;
		case 0:
		default:
			altEnterToggleFullscreenMode = WZ_SDL_DEFAULT_FULLSCREEN_MODE;
			break;
	}

	WZbackend = backend;

#if defined(WZ_OS_WIN)
	// Windows: Workaround for Nvidia "threaded optimization"
	// Set the process affinity mask to 1 before creating the window and initializing OpenGL
	// This disables Nvidia's "threaded optimization" feature, which can cause issues with WZ in OpenGL mode
	// NOTE: Must restore the affinity mask afterwards! (See below)
	DWORD_PTR originalProcessAffinityMask = 0;
	DWORD_PTR systemAffinityMask = 0;
	bool restoreAffinityMask = false;

	if (backend.has_value() && (backend.value() == video_backend::opengl)) // only do this for OpenGL mode, for now
	{
		if (::GetProcessAffinityMask(::GetCurrentProcess(), &originalProcessAffinityMask, &systemAffinityMask) != 0)
		{
			if (::SetProcessAffinityMask(::GetCurrentProcess(), 1) != 0)
			{
				restoreAffinityMask = true;
			}
			else
			{
				debug(LOG_INFO, "Failed to set process affinity mask");
			}
		}
		else
		{
			// Failed to get the current process affinity mask
			debug(LOG_INFO, "Failed to get current process affinity mask");
		}
	}
#endif

	SDL_gfx_api_Impl_Factory::Configuration sdl_impl_config;

	if (backend.has_value())
	{
		auto result = wzMainScreenSetup_CreateVideoWindow(backend.value(), antialiasing, fullscreen, vsync, highDPI);
		if (!result.has_value())
		{
			return false; // must return so new configuration will be saved
		}
		sdl_impl_config = result.value();

		/* initialise all cursors */
		if (war_GetColouredCursor())
		{
			sdlInitColoredCursors();
		}
		else
		{
			sdlInitCursors();
		}
	}

	setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")

#if defined(WZ_OS_MAC)
	if (backend.has_value())
	{
		cocoaSetupWZMenus();
	}
#endif

	const auto vsyncMode = to_swap_mode(vsync);

	gfx_api::backend_type gfxapi_backend = gfx_api::backend_type::null_backend;
	if (backend.has_value())
	{
		if (backend.value() == video_backend::vulkan)
		{
			gfxapi_backend = gfx_api::backend_type::vulkan_backend;
		}
		else
		{
			gfxapi_backend = gfx_api::backend_type::opengl_backend;
		}
	}

	optional<float> lodDistanceBias = nullopt;
	if (lodDistanceBiasPercentage != 0)
	{
		lodDistanceBias = static_cast<float>(lodDistanceBiasPercentage) / 100.f;
	}

	if (!gfx_api::context::initialize(SDL_gfx_api_Impl_Factory(WZwindow, sdl_impl_config), antialiasing, vsyncMode, lodDistanceBias, gfxapi_backend))
	{
		// Failed to initialize desired backend / renderer settings
		if (backend.has_value())
		{
			video_backend defaultBackend = wzGetNextFallbackGfxBackendForCurrentSystem(backend.value());
			if ((backend.value() != defaultBackend) && shouldResetGfxBackendPrompt(backend.value(), defaultBackend))
			{
				resetGfxBackend(defaultBackend);
				return false; // must return so new configuration will be saved
			}
			else
			{
				debug(LOG_FATAL, "gfx_api::context::get().initialize failed for backend: %s", to_string(backend.value()).c_str());
			}
		}
		else
		{
			// headless mode failed in gfx_api::context::initialize??
			debug(LOG_FATAL, "gfx_api::context::get().initialize failed for headless-mode backend?");
		}
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	if (backend.has_value())
	{
		wzMainScreenSetup_VerifyWindow();
	}

#if defined(WZ_OS_WIN)
	if (restoreAffinityMask)
	{
		// restore process affinity mask
		if (::SetProcessAffinityMask(::GetCurrentProcess(), originalProcessAffinityMask) != 0)
		{
			debug(LOG_WZ, "Restored process affinity mask");
		}
		else
		{
			debug(LOG_ERROR, "Failed to restore process affinity mask");
		}
	}
#endif

	return true;
}


// Calculates and returns the scale factor from the SDL window's coordinate system (in points) to the raw
// underlying pixels of the viewport / renderer.
//
// IMPORTANT: This value is *non-inclusive* of any user-configured Game Display Scale.
//
// This exposes what is effectively the SDL window's "High-DPI Scale Factor", if SDL's high-DPI support is enabled and functioning.
//
// In the normal, non-high-DPI-supported case, (in which the context's drawable size in pixels and the window's logical
// size in points are equal) this will return 1.0 for both values.
//
void wzGetWindowToRendererScaleFactor(float *horizScaleFactor, float *vertScaleFactor)
{
	if (!WZbackend.has_value())
	{
		if (horizScaleFactor != nullptr)
		{
			*horizScaleFactor = 1.0f;
		}
		if (vertScaleFactor != nullptr)
		{
			*vertScaleFactor = 1.0f;
		}
		return;
	}
	ASSERT_OR_RETURN(, WZwindow != nullptr, "wzGetWindowToRendererScaleFactor called when window is not available");

	// Obtain the window context's drawable size in pixels
	int drawableWidth, drawableHeight = 0;
	SDL_WZBackend_GetDrawableSize(WZwindow, &drawableWidth, &drawableHeight);

	// Obtain the logical window size (in points)
	int logicalWindowWidth = 0, logicalWindowHeight = 0;
	SDL_GetWindowSize(WZwindow, &logicalWindowWidth, &logicalWindowHeight);

	debug(LOG_WZ, "Window Logical Size (%d, %d) vs Drawable Size in Pixels (%d, %d)", logicalWindowWidth, logicalWindowHeight, drawableWidth, drawableHeight);

	if (horizScaleFactor != nullptr)
	{
		*horizScaleFactor = ((float)drawableWidth / (float)logicalWindowWidth); // Do **NOT** multiply by current_displayScaleFactor
	}
	if (vertScaleFactor != nullptr)
	{
		*vertScaleFactor = ((float)drawableHeight / (float)logicalWindowHeight); // Do **NOT** multiply by current_displayScaleFactor
	}

	int displayIndex = SDL_GetWindowDisplayIndex(WZwindow);
	if (displayIndex >= 0)
	{
		float hdpi, vdpi;
		if (SDL_GetDisplayDPI(displayIndex, nullptr, &hdpi, &vdpi) < 0)
		{
			debug(LOG_WARNING, "Failed to get the display (%d) DPI because : %s", displayIndex, SDL_GetError());
		}
		else
		{
			debug(LOG_WZ, "Display (%d) DPI: %f, %f", displayIndex, hdpi, vdpi);
		}
	}
	else
	{
		debug(LOG_WARNING, "Failed to get the display index for the window because : %s", SDL_GetError());
	}
}

// Calculates and returns the total scale factor from the game's coordinate system (in points)
// to the raw underlying pixels of the viewport / renderer.
//
// IMPORTANT: This value is *inclusive* of both the user-configured "Display Scale" *AND* any underlying
// high-DPI / "Retina" display support provided by SDL.
//
// It is equivalent to: (SDL Window's High-DPI Scale Factor) x (WZ Game Display Scale Factor)
//
// Therefore, if SDL is providing a supported high-DPI window / context, this value will be greater
// than the WZ (user-configured) Display Scale Factor.
//
// It should be used only for internal (non-user-displayed) cases in which the full scaling factor from
// the game system's coordinate system (in points) to the underlying display pixels is required.
// (For example, when rasterizing text for best display.)
//
void wzGetGameToRendererScaleFactor(float *horizScaleFactor, float *vertScaleFactor)
{
	float horizWindowScaleFactor = 0.f, vertWindowScaleFactor = 0.f;
	wzGetWindowToRendererScaleFactor(&horizWindowScaleFactor, &vertWindowScaleFactor);
	assert(horizWindowScaleFactor != 0.f);
	assert(vertWindowScaleFactor != 0.f);

	if (horizScaleFactor != nullptr)
	{
		*horizScaleFactor = horizWindowScaleFactor * current_displayScaleFactor;
	}
	if (vertScaleFactor != nullptr)
	{
		*vertScaleFactor = vertWindowScaleFactor * current_displayScaleFactor;
	}
}
void wzGetGameToRendererScaleFactorInt(unsigned int *horizScalePercentage, unsigned int *vertScalePercentage)
{
	float horizWindowScaleFactor = 0.f, vertWindowScaleFactor = 0.f;
	wzGetWindowToRendererScaleFactor(&horizWindowScaleFactor, &vertWindowScaleFactor);
	assert(horizWindowScaleFactor != 0.f);
	assert(vertWindowScaleFactor != 0.f);

	if (horizScalePercentage != nullptr)
	{
		*horizScalePercentage = static_cast<unsigned int>(ceil(horizWindowScaleFactor * current_displayScale));
	}
	if (vertScalePercentage != nullptr)
	{
		*vertScalePercentage = static_cast<unsigned int>(ceil(vertWindowScaleFactor * current_displayScale));
	}
}

void wzSetWindowIsResizable(bool resizable)
{
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzSetWindowIsResizable called when window is not available");
		return;
	}
	SDL_bool sdl_resizable = (resizable) ? SDL_TRUE : SDL_FALSE;
	SDL_SetWindowResizable(WZwindow, sdl_resizable);

	if (resizable)
	{
		// Set the minimum window size
		unsigned int minWindowWidth = 0, minWindowHeight = 0;
		wzGetMinimumWindowSizeForDisplayScaleFactor(&minWindowWidth, &minWindowHeight, current_displayScaleFactor);
		SDL_SetWindowMinimumSize(WZwindow, minWindowWidth, minWindowHeight);
	}
}

bool wzIsWindowResizable()
{
	if (WZwindow == nullptr)
	{
		debug(LOG_WARNING, "wzIsWindowResizable called when window is not available");
		return false;
	}
	Uint32 flags = SDL_GetWindowFlags(WZwindow);
	if (flags & SDL_WINDOW_RESIZABLE)
	{
		return true;
	}
	return false;
}

/*!
 * Activation (focus change ... and) eventhandler.  Mainly for debugging.
 */
static void handleActiveEvent(SDL_Event *event)
{
	if (event->type == SDL_WINDOWEVENT)
	{
		switch (event->window.event)
		{
		case SDL_WINDOWEVENT_SHOWN:
			debug(LOG_WZ, "Window %d shown", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_HIDDEN:
			debug(LOG_WZ, "Window %d hidden", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			debug(LOG_WZ, "Window %d exposed", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_MOVED:
			debug(LOG_WZ, "Window %d moved to %d,%d", event->window.windowID, event->window.data1, event->window.data2);
				// FIXME: Handle detecting which screen the window was moved to, and update saved war_SetScreen?
			break;
		case SDL_WINDOWEVENT_RESIZED:
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			debug(LOG_WZ, "Window %d resized to %dx%d", event->window.windowID, event->window.data1, event->window.data2);
			{
				unsigned int oldWindowWidth = windowWidth;
				unsigned int oldWindowHeight = windowHeight;

				Uint32 windowFlags = SDL_GetWindowFlags(WZwindow);
				debug(LOG_WZ, "Window resized to window flags: %u", windowFlags);

				int newWindowWidth = 0, newWindowHeight = 0;
				SDL_GetWindowSize(WZwindow, &newWindowWidth, &newWindowHeight);

				if ((event->window.data1 != newWindowWidth) || (event->window.data2 != newWindowHeight))
				{
					// This can happen - so we use the values retrieved from SDL_GetWindowSize in any case - but
					// log it for tracking down the SDL-related causes later.
					debug(LOG_WARNING, "Received width and height (%d x %d) do not match those from GetWindowSize (%d x %d)", event->window.data1, event->window.data2, newWindowWidth, newWindowHeight);
				}

				handleWindowSizeChange(oldWindowWidth, oldWindowHeight, newWindowWidth, newWindowHeight);

				// Store the new values (in case the user manually resized the window bounds)
				if (wzGetCurrentWindowMode() == WINDOW_MODE::windowed)
				{
					war_SetWidth(newWindowWidth);
					war_SetHeight(newWindowHeight);
				}

				// Handle deferred size reset
				if (deferredDimensionReset.has_value())
				{
					if (WZwindow != nullptr)
					{
						SDL_SetWindowSize(WZwindow, deferredDimensionReset.value().width, deferredDimensionReset.value().height);
						if (deferredDimensionReset.value().recenter)
						{
							SDL_SetWindowPosition(WZwindow, SDL_WINDOWPOS_CENTERED_DISPLAY(screenIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(screenIndex));
						}
						wzReduceDisplayScalingIfNeeded(deferredDimensionReset.value().width, deferredDimensionReset.value().height);
					}
					deferredDimensionReset.reset();
				}
			}
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			debug(LOG_INFO, "Window %d minimized", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			debug(LOG_WZ, "Window %d maximized", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_RESTORED:
			debug(LOG_INFO, "Window %d restored", event->window.windowID);
			{
				unsigned int oldWindowWidth = windowWidth;
				unsigned int oldWindowHeight = windowHeight;

				int newWindowWidth = 0, newWindowHeight = 0;
				SDL_GetWindowSize(WZwindow, &newWindowWidth, &newWindowHeight);

				int newDrawableWidth = 0, newDrawableHeight = 0;
				SDL_WZBackend_GetDrawableSize(WZwindow, &newDrawableWidth, &newDrawableHeight);
				auto oldDrawableDimensions = gfx_api::context::get().getDrawableDimensions();

				if (oldWindowWidth != newWindowWidth || oldWindowHeight != newWindowHeight
					|| oldDrawableDimensions.first != newDrawableWidth || oldDrawableDimensions.second != newDrawableHeight)
				{
					debug(LOG_WZ, "Triggering handleWindowSizeChange from window restore event");
					handleWindowSizeChange(oldWindowWidth, oldWindowHeight, newWindowWidth, newWindowHeight);
				}
			}
			break;
		case SDL_WINDOWEVENT_ENTER:
			debug(LOG_WZ, "Mouse entered window %d", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_LEAVE:
			debug(LOG_WZ, "Mouse left window %d", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			mouseInWindow = SDL_TRUE;
			debug(LOG_WZ, "Window %d gained keyboard focus", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			mouseInWindow = SDL_FALSE;
			debug(LOG_WZ, "Window %d lost keyboard focus", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_CLOSE:
			debug(LOG_WZ, "Window %d closed", event->window.windowID);
			WZwindow = nullptr;
			break;
		case SDL_WINDOWEVENT_TAKE_FOCUS:
			debug(LOG_WZ, "Window %d is being offered focus", event->window.windowID);
			break;
		default:
			debug(LOG_WZ, "Window %d got unknown event %d", event->window.windowID, event->window.event);
			break;
		}
	}
}

static SDL_Event event;

void wzEventLoopOneFrame(void* arg)
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
		case SDL_MOUSEWHEEL:
			inputHandleMouseWheelEvent(&event.wheel);
			break;
		case SDL_WINDOWEVENT:
			handleActiveEvent(&event);
			break;
		case SDL_TEXTINPUT:	// SDL now handles text input differently
			inputhandleText(&event.text);
			break;
		case SDL_QUIT:
			ASSERT(arg != nullptr, "No valid bContinue");
			if (arg)
			{
				bool *bContinue = static_cast<bool*>(arg);
				*bContinue = false;
			}
			return;
		default:
			break;
		}

		if (wzSDLAppEvent == event.type)
		{
			// Custom WZ App Event
			switch (event.user.code)
			{
				case wzSDLAppEventCodes::MAINTHREADEXEC:
					if (event.user.data1 != nullptr)
					{
						WZ_MAINTHREADEXEC * pExec = static_cast<WZ_MAINTHREADEXEC *>(event.user.data1);
						pExec->doExecOnMainThread();
						delete pExec;
					}
					break;
				default:
					break;
			}
		}
	}

	processScreenSizeChangeNotificationIfNeeded();
	mainLoop();				// WZ does its thing
	inputNewFrame();			// reset input states
}

// Actual mainloop
void wzMainEventLoop(std::function<void()> onShutdown)
{
	event.type = 0;

	bool bContinue = true;
	while (bContinue)
	{
		wzEventLoopOneFrame(&bContinue);
	}

	if (onShutdown)
	{
		onShutdown();
	}
}

void wzPumpEventsWhileLoading()
{
	SDL_PumpEvents();
}

void wzShutdown()
{
	// order is important!
	sdlFreeCursors();
	if (WZwindow != nullptr)
	{
		SDL_DestroyWindow(WZwindow);
		WZwindow = nullptr;
	}
	SDL_Quit();

	// delete copies of argc, argv
	if (copied_argv != nullptr)
	{
		for(int i=0; i < copied_argc; i++) {
			delete [] copied_argv[i];
		}
		delete [] copied_argv;
		copied_argv = nullptr;
		copied_argc = 0;
	}
}

// NOTE: wzBackendAttemptOpenURL should *not* be called directly - instead, call openURLInBrowser() from urlhelpers.h
bool wzBackendAttemptOpenURL(const char *url)
{
#if SDL_VERSION_ATLEAST(2, 0, 14) // SDL >= 2.0.14
	// Can use SDL_OpenURL to support many (not all) platforms if run-time SDL library is also >= 2.0.14
	SDL_version linked_sdl_version;
	SDL_GetVersion(&linked_sdl_version);
	if ((linked_sdl_version.major > 2) || (linked_sdl_version.major == 2 && (linked_sdl_version.minor > 0 || (linked_sdl_version.minor == 0 && linked_sdl_version.patch >= 14))))
	{
		return (SDL_OpenURL(url) == 0);
	}
#endif
	// SDL_OpenURL requires SDL >= 2.0.14
	return false;
}

// Gets the system RAM in MiB
uint64_t wzGetCurrentSystemRAM()
{
	int value = SDL_GetSystemRAM();
	if (value <= 0) { return 0; }
	return static_cast<uint64_t>(value);
}
