/*
 *  PieToaster is an OpenGL application to edit 3D models in
 *  Warzone 2100's (an RTS game) PIE 3D model format, which is heavily
 *  inspired by PieSlicer created by stratadrake.
 *  Copyright (C) 2007  Carl Hee
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _screen_h
#define _screen_h

#include <SDL_opengl.h>

#include "wzglobal.h"
#include "pie_types.h"


class CScreen {
public:
	PFNGLGENBUFFERSARBPROC glGenBuffersARB;
	PFNGLBINDBUFFERARBPROC glBindBufferARB;
	PFNGLBUFFERDATAARBPROC glBufferDataARB;
	PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB;
	PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;
	PFNGLGETBUFFERPARAMETERIVARBPROC glGetBufferParameterivARB;
	PFNGLMAPBUFFERARBPROC glMapBufferARB;
	PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB;

	Uint16	m_width;	///<width of window
	Uint16	m_height;	///<height of window
	Sint32	m_bpp;	///<bit per pixel
	Uint32	m_flags;	///<SDL surface flags
	bool	m_useVBO;	///uses VBO or not

	int		initialize(void);
	void	initializeVBOExtension(void);
	bool	setVideoMode(void);
	SDL_Surface	*m_Surface;

};

extern CScreen g_Screen;

#endif
