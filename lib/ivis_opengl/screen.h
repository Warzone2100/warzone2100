/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
/*
 * Screen.h
 *
 * Interface to the OpenGL double buffered display.
 *
 */
#ifndef _screen_h
#define _screen_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include "lib/framework/types.h"
#include "lib/framework/vector.h"

/* ------------------------------------------------------------------------------------------- */


extern unsigned screenWidth;
extern unsigned screenHeight;

/* backDrop */
void screen_SetRandomBackdrop(const char *dirname,
                              const char *basename);
extern void screen_SetBackDropFromFile(const char *filename);
extern void screen_StopBackDrop(void);
extern void screen_RestartBackDrop(void);
extern bool screen_GetBackDrop(void);
extern void screen_Upload(const char *newBackDropBmp);
void screen_Display();

/* screendump */
void screenDumpToDisk(const char *path, const char *level);

extern int wz_texture_compression;

extern void screenDoDumpToDiskIfRequired(void);

void screen_enableMapPreview(int width, int height, Vector2i *playerpositions);
void screen_disableMapPreview(void);

/// gaphics performance measurement points
enum PERF_POINT
{
	PERF_START_FRAME,
	PERF_EFFECTS,
	PERF_TERRAIN,
	PERF_SKYBOX,
	PERF_MODEL_INIT,
	PERF_PARTICLES,
	PERF_WATER,
	PERF_MODELS,
	PERF_MISC,
	PERF_GUI,
	PERF_COUNT
};

bool screenInitialise();
void screenShutDown();

void wzPerfBegin(PERF_POINT pp, const char *descr);
void wzPerfEnd(PERF_POINT pp);
void wzPerfStart();
void wzPerfShutdown();
void wzPerfFrame();
/// Are performance measurements available?
bool wzPerfAvailable();

void wzSceneBegin(const char *descr);
void wzSceneEnd(const char *descr);

struct OPENGL_DATA
{
	char vendor[256];
	char renderer[256];
	char version[256];
	char GLEWversion[256];
	char GLSLversion[256];
};
extern OPENGL_DATA opengl;
#endif
