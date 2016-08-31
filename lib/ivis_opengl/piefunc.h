/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/***************************************************************************/
/*
 * piefunc.h
 *
 * type defines for extended image library functions.
 *
 */
/***************************************************************************/

#ifndef _piefunc_h
#define _piefunc_h

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/pieclip.h"

void pie_TransColouredTriangle(Vector3f *vrt, PIELIGHT c);

void pie_SetViewingWindow(Vector3i *v, PIELIGHT colour);
void pie_DrawViewingWindow(const glm::mat4 &modelViewProjectionMatrix);

void pie_DrawSkybox(float scale);
void pie_Skybox_Init();
void pie_Skybox_Shutdown();
void pie_Skybox_Texture(const char *filename);

#endif // _piedef_h
