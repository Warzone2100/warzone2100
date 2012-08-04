/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2012  Warzone 2100 Project

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

#include "wzmviewer.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <png.h>

#if (defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define inline __inline
#  define alloca _alloca
#  define fileno _fileno
#  define isfinite _finite
#endif

// The WZM format is a proposed successor to the PIE format used by Warzone.
// For an explanation of the WZM format, see http://wiki.wz2100.net/WZM_format

// To compile: gcc -o wzmviewer wzmviewer.c wzmutils.c -Wall -g -O0 -Wshadow -lpng `sdl-config --libs --cflags` -lGL -lGLU

static char *input = "";
static char *texPath = "";
static SDL_Surface *screen;
static uint32_t now = 0;

static void parse_args(int argc, char **argv)
{
	unsigned int i = 1;

	if (argc < 2 + i)
	{
		fprintf(stderr, "Syntax: wzmviewer filename.wzm path_to_textures\n");
		exit(1);
	}
	input = argv[i++];
	texPath = argv[i++];
}

static void resizeWindow(int width, int height)
{
	glViewport(0, 0, width, height);
}

int main(int argc, char **argv)
{
	MODEL *psModel;
	const int width = 640, height = 480;
	SDL_Event event;
	GLfloat angle = 0.0f;
	const float aspect = (float)width / (float)height;
	bool quit = false;
	float dimension = 0.0f;
	int i;
	char path[PATH_MAX];

	parse_args(argc, argv);

	/* Initialize SDL */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	atexit(SDL_Quit);

	psModel = readModel(input, SDL_GetTicks());
	strcpy(path, texPath);
	strcat(path, psModel->texPath);
	psModel->pixmap = readPixmap(path);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	/* Initialize the display */
	screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL|SDL_ANYFORMAT);
	if (screen == NULL)
	{
		fprintf(stderr, "Couldn't initialize display: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	printf("OpenGL vendor: %s\n", glGetString(GL_VENDOR));

	resizeWindow(width, height);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, aspect, 0.1f, 500.0f);
	glMatrixMode(GL_MODELVIEW);

	prepareModel(psModel);
	for (i = 0; i < psModel->meshes; i++)
	{
		int j;
		MESH *psMesh = &psModel->mesh[i];

		for (j = 0; j < psMesh->vertices * 3; j++)
		{
			dimension = MAX(fabs(psMesh->vertexArray[j]), dimension);
		}
	}

	/* Find model size */

	while (!quit)
	{
		now = SDL_GetTicks();
		while (SDL_PollEvent(&event))
		{
			SDL_keysym *keysym = &event.key.keysym;

			switch (event.type)
			{
				case SDL_VIDEORESIZE:
					resizeWindow(event.resize.w, event.resize.h);
					break;
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_KEYDOWN:
					switch (keysym->sym)
					{
						case SDLK_F1:
							glEnable(GL_CULL_FACE);
							printf("Culling enabled.\n");
							break;
						case SDLK_F2:
							glDisable(GL_CULL_FACE);
							printf("Culling disabled.\n");
							break;
						case SDLK_F3:
							glDisable(GL_TEXTURE_2D);
							glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
							printf("Wireframe mode.\n");
							break;
						case SDLK_F4:
							glEnable(GL_TEXTURE_2D);
							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
							printf("Texturing mode.\n");
							break;
						case SDLK_ESCAPE:
							quit = true;
							break;
						case SDLK_KP_PLUS:
						case SDLK_PLUS:
							for (i = 0; i < psModel->meshes; i++)
							{
								MESH *psMesh = &psModel->mesh[i];

								if (!psMesh->teamColours)
								{
									continue;
								}

								if (psMesh->currentTextureArray < 7)
								{
									psMesh->currentTextureArray++;
								}
								else
								{
									psMesh->currentTextureArray = 0;
								}
							}
							break;
						default:
							break;
					}
					break;
			}
		}

		glLoadIdentity();
		glTranslatef(0.0f, -30.0f, -50.0f + -(dimension * 2.0f));;
		glRotatef(angle, 0, 1, 0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		drawModel(psModel, now);
		SDL_GL_SwapBuffers();
		SDL_Delay(10);
		angle += 0.1;
		if (angle > 360.0f)
		{
			angle = 0.0f;
		}
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	freeModel(psModel);
	
	return 0;
}
