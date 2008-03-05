/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2008  Warzone Resurrection Project

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

#ifndef _wzmviewer_h
#define _wzmviewer_h

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#ifndef WIN32
#include <stdbool.h>
#include <limits.h>
#else
#include <windows.h>
typedef int bool;
#define PATH_MAX 255
#define true 1
#define false 0
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#define MAX_MESHES 4
#define MAX_TEXARRAYS 16
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct { float x, y, z; } Vector3f;

typedef struct
{
	float timeSlice;
	int textureArray;
	Vector3f translation;
	Vector3f rotation;
} FRAME;

typedef struct
{
	int faces, vertices, frames, textureArrays, connectors;
	bool teamColours;
	GLfloat *vertexArray;
	GLuint *indexArray;	// TODO: use short instead
	GLfloat *textureArray[MAX_TEXARRAYS];
	FRAME *frameArray;
	int currentFrame;
	uint32_t lastChange;	// animation
} MESH;

typedef struct
{
	int w, h;
	char *pixels;
} PIXMAP;

typedef struct
{
	int meshes;
	MESH *mesh;
	PIXMAP *pixmap;
	char texPath[PATH_MAX];
	GLuint texture;
} MODEL;

MODEL *createModel(int meshes);
PIXMAP *readPixmap(const char *filename);
MODEL *readModel(const char *filename, const char *path);
void freeModel(MODEL *psModel);

#endif
