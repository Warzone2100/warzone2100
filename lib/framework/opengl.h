/*
	This file is part of Warzone 2100.
	Copyright (C) 2010  Giel van Schijndel
	Copyright (C) 2013-2017  Warzone 2100 Project

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
/** @file
 *  @brief Platform independent OpenGL inclusion.
 */

#ifndef __INCLUDED_LIB_FRAMEWORK_OPENGL_H__
#define __INCLUDED_LIB_FRAMEWORK_OPENGL_H__

/*
 * glew.h includes inttypes.h under mingw and cygwin
 * inttypes.h includes stdint.h
 * We need to include wzglobal.h first because we must
 * define __STDC_LIMIT_MACROS before including stdint.h
 */
#include "lib/framework/wzglobal.h"
#include <GL/glew.h>

#endif
