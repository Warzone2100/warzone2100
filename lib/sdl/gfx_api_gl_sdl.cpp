/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2019  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "gfx_api_gl_sdl.h"
#include <SDL_opengl.h>
#include <SDL_timer.h>
#include <SDL_hints.h>
#include <thread>
#include <chrono>

bool vsyncIsEnabled = true;

sdl_OpenGL_Impl::sdl_OpenGL_Impl(SDL_Window* _window, bool _useOpenGLES, bool _useOpenGLESLibrary)
{
	ASSERT(_window != nullptr, "Invalid SDL_Window*");
	window = _window;
	useOpenglES = _useOpenGLES;
	contextRequest = getInitialContextRequest(useOpenglES);
	useOpenGLESLibrary = _useOpenGLESLibrary;
}

GLADloadproc sdl_OpenGL_Impl::getGLGetProcAddress()
{
	return SDL_GL_GetProcAddress;
}

void sdl_OpenGL_Impl::setOpenGLESDriver(bool useOpenGLESLibrary)
{
	if (useOpenGLESLibrary)
	{
#if defined(SDL_HINT_OPENGL_ES_DRIVER)
		SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#else
		debug(LOG_WARNING, "SDL_HINT_OPENGL_ES_DRIVER is not available - may not use the OpenGL ES library");
#endif
	}
	else
	{
#if defined(SDL_HINT_OPENGL_ES_DRIVER)
		SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "0");
#endif
	}
}

bool sdl_OpenGL_Impl::configureNextOpenGLContextRequest()
{
	contextRequest = GLContextRequests(contextRequest + 1);
	return configureOpenGLContextRequest(contextRequest, useOpenGLESLibrary);
}

sdl_OpenGL_Impl::GLContextRequests sdl_OpenGL_Impl::getInitialContextRequest(bool useOpenglES /*= false*/)
{
	if (useOpenglES)
	{
		return OpenGLES30;
	}
	else
	{
		return OpenGLCore_HighestAvailable;
	}
}

bool sdl_OpenGL_Impl::configureOpenGLContextRequest(GLContextRequests request, bool useOpenGLESLibrary)
{
	switch (request)
	{
		// NOTES:
		// Requesting an OpenGL 3.0+ Core Profile context
		// - On macOS, **MUST** request at least OpenGL 3.2+ Core Profile (with FORWARD_COMPATIBLE_FLAG)
		//   to get the highest version OpenGL Core Profile context that's supported.
		// - Mesa allegedly requires a request for OpenGL 3.1+ Core Profile to get the highest version
		//   OpenGL Core Profile context that's supported.
		// - Some systems return the highest OpenGL Core Profile version that they support with the below,
		//   but others only return the exact version requested (even if a higher version is supported).
		// - Thus, we start by requesting the maximum version of OpenGL that we'd like for WZ functionality,
		//	 and then try every earlier version (as / if each fails).
		// (Note: There is fallback handling inside sdl_OpenGL_Impl::createGLContext())

		// SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG is *required* to obtain an OpenGL >= 3 Core Context on macOS
		case OpenGLCore_3_3:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			return true;
		case OpenGLCore_3_2:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
			return true;
		case OpenGLCore_3_1:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			return true;
		case OpenGLCore_3_0:
			// Specify FORWARD_COMPATIBLE, so 3.0 simulates the 3.1 experience (basically: core profile)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			return true;
		case OpenGL21Compat:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			return true;
#  if !defined(WZ_OS_MAC)
		case OpenGLES30:
			setOpenGLESDriver(useOpenGLESLibrary);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			return true;
		case OpenGLES20:
			setOpenGLESDriver(useOpenGLESLibrary);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			return true;
#  else
		case OpenGLES30:
			return false;
		case OpenGLES20:
			return false;
#  endif
		case MAX_CONTEXT_REQUESTS:
			return false;
	}
	return false;
}

std::string sdl_OpenGL_Impl::to_string(const GLContextRequests& request) const\
{
	switch (contextRequest)
	{
		case OpenGLCore_3_3:
			return "OpenGL 3.3";
		case OpenGLCore_3_2:
			return "OpenGL 3.2";
		case OpenGLCore_3_1:
			return "OpenGL 3.1";
		case OpenGLCore_3_0:
			return "OpenGL 3.0";
		case OpenGL21Compat:
			return "OpenGL 2.1 Compatibility";
		case OpenGLES30:
			return "OpenGL ES 3.0";
		case OpenGLES20:
			return "OpenGL ES 2.0";
		case MAX_CONTEXT_REQUESTS:
			return "";
	}
	return "";
}

bool sdl_OpenGL_Impl::isOpenGLES() const
{
	return contextRequest >= OpenGLES30;
}

bool sdl_OpenGL_Impl::createGLContext()
{
	SDL_GLContext WZglcontext = SDL_GL_CreateContext(window);
	std::string glContextErrors;
	while (!WZglcontext)
	{
		glContextErrors += "Failed to create an " + to_string(contextRequest) + " context! [" + std::string(SDL_GetError()) + "]\n";
		if (!configureNextOpenGLContextRequest())
		{
			// No more context requests to try
			debug_multiline(LOG_ERROR, glContextErrors);
			return false;
		}
		WZglcontext = SDL_GL_CreateContext(window);
	}
	if (!glContextErrors.empty())
	{
		// Although context creation eventually succeeded, log the attempts that failed
		debug_multiline(LOG_3D, glContextErrors);
	}
	debug(LOG_3D, "Requested %s context", to_string(contextRequest).c_str());

	int value = 0;
	if (SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == 0)
	{
		if (value == 0)
		{
			debug(LOG_FATAL, "OpenGL initialization did not give double buffering! (%d)", value);
			debug(LOG_FATAL, "Double buffering is required for this game!");
			SDL_GL_DeleteContext(WZglcontext);
			return false;
		}
	}
	else
	{
		// SDL_GL_GetAttribute failed for SDL_GL_DOUBLEBUFFER
		// For desktop OpenGL, treat this as a fatal error
		code_part log_type = LOG_FATAL;
		if (isOpenGLES())
		{
			// For OpenGL ES (EGL?), log this + let execution continue
			//
			// If SDL is compiled with Desktop OpenGL support, it may not properly
			// query double buffering status for OpenGL ES contexts (as of: SDL 2.0.10)
			log_type = LOG_3D;
		}
		debug(log_type, "SDL_GL_GetAttribute failed to get value for SDL_GL_DOUBLEBUFFER (%s)", SDL_GetError());
		debug(log_type, "Double buffering is required for this game - if it isn't actually enabled, things will fail!");
		if (log_type == LOG_FATAL)
		{
			SDL_GL_DeleteContext(WZglcontext);
			return false;
		}
	}

	int r, g, b, a;
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
	debug(LOG_3D, "Current values for: SDL_GL_RED_SIZE (%d), SDL_GL_GREEN_SIZE (%d), SDL_GL_BLUE_SIZE (%d), SDL_GL_ALPHA_SIZE (%d)", r, g, b, a);

	if (SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value) != 0)
	{
		debug(LOG_3D, "Failed to get value for SDL_GL_DEPTH_SIZE (%s)", SDL_GetError());
	}
	debug(LOG_3D, "Current value for SDL_GL_DEPTH_SIZE: (%d)", value);
	
	if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &value) != 0)
	{
		debug(LOG_3D, "Failed to get value for SDL_GL_STENCIL_SIZE (%s)", SDL_GetError());
	}
	debug(LOG_3D, "Current value for SDL_GL_STENCIL_SIZE: (%d)", value);

	int windowWidth, windowHeight = 0;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	debug(LOG_WZ, "Logical Window Size: %d x %d", windowWidth, windowHeight);

	return true;
}

bool sdl_OpenGL_Impl::destroyGLContext()
{
	SDL_GLContext WZglcontext = SDL_GL_GetCurrentContext();
	if (!WZglcontext)
	{
		return false;
	}
	SDL_GL_DeleteContext(WZglcontext);
	return true;
}

void sdl_OpenGL_Impl::swapWindow()
{
#if defined(WZ_OS_MAC)
	// Workaround for OpenGL on macOS (see below)
	const uint32_t swapStartTime = SDL_GetTicks();
#endif

	SDL_GL_SwapWindow(window);

#if defined(WZ_OS_MAC)
	// Workaround for OpenGL on macOS
	// - If the OpenGL window is minimized (or occluded), SwapWindow may not wait for the vertical blanking interval
	// - To workaround this, detect when we seem to be spinning without any wait, and sleep for a bit
	static uint32_t numFramesNoVsync = 0;
	static uint32_t lastSwapEndTime = 0;
	const bool isMinimized = static_cast<bool>(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED);
	const uint32_t minFrameInterval = 1000 / ((isMinimized) ? 60 : 120);
	const uint32_t minSwapEndTick = swapStartTime + 2;
	uint32_t swapEndTime = SDL_GetTicks();
	const uint32_t frameTime = swapEndTime - lastSwapEndTime;
	if ((vsyncIsEnabled || isMinimized) && !SDL_TICKS_PASSED(swapEndTime, minSwapEndTick) && (frameTime < minFrameInterval))
	{
		const uint32_t leewayFramesBeforeThrottling = (isMinimized) ? 2 : 4;
		if (leewayFramesBeforeThrottling < numFramesNoVsync)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(minFrameInterval - frameTime));
			swapEndTime = SDL_GetTicks();
		}
		else
		{
			++numFramesNoVsync;
		}
	}
	else if (0 < numFramesNoVsync)
	{
		--numFramesNoVsync;
	}
	lastSwapEndTime = swapEndTime;
#endif
}

void sdl_OpenGL_Impl::getDrawableSize(int* w, int* h)
{
	SDL_GL_GetDrawableSize(window, w, h);
}

bool to_sdl_swap_interval(gfx_api::context::swap_interval_mode mode, int &output_value)
{
	switch (mode)
	{
		case gfx_api::context::swap_interval_mode::immediate:
			output_value = 0;
			return true;
		case gfx_api::context::swap_interval_mode::vsync:
			output_value = 1;
			return true;
		case gfx_api::context::swap_interval_mode::adaptive_vsync:
			output_value = -1;
			return true;
		default:
			// unsupported swap_interval_mode
			return false;
	}
	return false;
}

gfx_api::context::swap_interval_mode from_sdl_swap_interval(int swapInterval)
{
	switch (swapInterval)
	{
		case 0:
			return gfx_api::context::swap_interval_mode::immediate;
		case 1:
			return gfx_api::context::swap_interval_mode::vsync;
		case -1:
			return gfx_api::context::swap_interval_mode::adaptive_vsync;
	}
	return gfx_api::context::swap_interval_mode::vsync;
}

bool sdl_OpenGL_Impl::setSwapInterval(gfx_api::context::swap_interval_mode mode)
{
	int interval = 1;
	if (!to_sdl_swap_interval(mode, interval))
	{
		debug(LOG_3D, "Unsupported swap_interval_mode for SDL OpenGL backend: %d", to_int(mode));
		return false;
	}
	if (SDL_GL_SetSwapInterval(interval) != 0)
	{
		debug(LOG_WARNING, "SDL_GL_SetSwapInterval(%d) failed with error (%s)", interval, SDL_GetError());
		return false;
	}
	vsyncIsEnabled = interval != 0;
	return true;
}

gfx_api::context::swap_interval_mode sdl_OpenGL_Impl::getSwapInterval() const
{
	int interval = SDL_GL_GetSwapInterval();
	return from_sdl_swap_interval(interval);
}

