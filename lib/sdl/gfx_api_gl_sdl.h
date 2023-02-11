/*
	This file is part of Warzone 2100.
	Copyright (C) 2019  Warzone 2100 Project

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

#pragma once

#ifndef __INCLUDED_GFX_API_GL_SDL_H__
#define __INCLUDED_GFX_API_GL_SDL_H__

#include "lib/ivis_opengl/gfx_api_gl.h"
#include <SDL_video.h>

class sdl_OpenGL_Impl final : public gfx_api::backend_OpenGL_Impl
{
public:
	sdl_OpenGL_Impl(SDL_Window* window, bool useOpenGLES, bool useOpenGLESLibrary);

	virtual GLADloadproc getGLGetProcAddress() override;
	virtual bool createGLContext() override;
	virtual bool destroyGLContext() override;
	virtual void swapWindow() override;
	virtual void getDrawableSize(int* w, int* h) override;

	virtual bool isOpenGLES() const override;

	virtual bool setSwapInterval(gfx_api::context::swap_interval_mode mode) override;
	virtual gfx_api::context::swap_interval_mode getSwapInterval() const override;

public:

	enum GLContextRequests {
		// Desktop OpenGL Context Requests
		OpenGLCore_3_3,
		OpenGLCore_3_2,
		OpenGLCore_3_1,
		OpenGLCore_3_0,
		OpenGL21Compat,
		// OpenGL ES Context Requests
		OpenGLES30,
		OpenGLES20,
		//
		MAX_CONTEXT_REQUESTS
	};
	static constexpr GLContextRequests OpenGLCore_HighestAvailable = OpenGLCore_3_3;

	static bool configureOpenGLContextRequest(GLContextRequests request, bool useOpenGLESLibrary);
	static GLContextRequests getInitialContextRequest(bool useOpenglES = false);

private:
	SDL_Window* window;
	bool useOpenglES = false;
	bool useOpenGLESLibrary = false;

	GLContextRequests contextRequest = OpenGLCore_HighestAvailable;

private:
	bool configureNextOpenGLContextRequest();
	std::string to_string(const GLContextRequests& request) const;
	static void setOpenGLESDriver(bool useOpenGLESLibrary);
};

#endif // __INCLUDED_GFX_API_GL_SDL_H__
