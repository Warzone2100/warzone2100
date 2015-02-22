/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2015  Warzone 2100 Project

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

#ifndef _wzmutils_h
#define _wzmutils_h

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <GL/gl.h>

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

#define MAX_TEXARRAYS 16
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct { float x, y, z; } Vector3f;

typedef struct
{
	Vector3f pos;
	int type;
} CONNECTOR;

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
	int currentTextureArray;
	uint32_t lastChange;	// animation
	CONNECTOR *connectorArray;
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

MODEL *createModel(int meshes, int now);
PIXMAP *readPixmap(const char *filename);
MODEL *readModel(const char *filename, int now);
int saveModel(const char *filename, MODEL *psModel);
void freeModel(MODEL *psModel);
void drawModel(MODEL *psModel, int now);
void prepareModel(MODEL *psModel);

#endif
