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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

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
#include <png.h>

// The WZM format is a proposed successor to the PIE format used by Warzone.
// For an explanation of the WZM format, see http://wiki.wz2100.net/WZM_format

// To compile: gcc -o wzmviewer wzmviewer.c -Wall -g -O0 -Wshadow -lpng `sdl-config --libs --cflags` -lGL -lGLU

#define MAX_MESHES 4
#define MAX_TEXARRAYS 16

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
	GLuint texture;
} MODEL;

static char *input = "";
static char *texPath = "";
static SDL_Surface *screen;

static MODEL *createModel(int meshes)
{
	MODEL *psModel = malloc(sizeof(MODEL));
	int i;

	psModel->meshes = meshes;
	psModel->mesh = malloc(sizeof(MESH) * meshes);
	for (i = 0; i < meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];
		int j;

		psMesh->faces = 0;
		psMesh->frames = 0;
		psMesh->vertices = 0;
		psMesh->textureArrays = 0;
		psMesh->connectors = 0;
		psMesh->teamColours = false;
		psMesh->vertexArray = NULL;
		psMesh->indexArray = NULL;
		for (j = 0; j < MAX_TEXARRAYS; j++)
		{
			psMesh->textureArray[j] = NULL;
		}
		psMesh->frameArray = NULL;
		psMesh->currentFrame = 0;
	}

	return psModel;
}

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

PIXMAP *readPixmap(const char *filename)
{
	PIXMAP *gfx;
	png_structp pngp;
	png_infop infop;
	png_uint_32 width, height;
	png_int_32 y, stride;
	int bit_depth, color_type, interlace_type;
	FILE *fp;
	png_bytep *row_pointers;
	const unsigned int sig_length = 8;
	unsigned char header[8];
	unsigned char *image_data;

	if (PNG_LIBPNG_VER_MAJOR != 1 || PNG_LIBPNG_VER_MINOR < 2)
	{
		printf("libpng 1.2.6 or higher required!\n");
		exit(EXIT_FAILURE);
	}
	if (!(fp = fopen(filename, "rb")))
	{
		printf("%s won't open!\n", filename);
		exit(EXIT_FAILURE);
	}
	fread(header, 1, sig_length, fp);
	if (png_sig_cmp(header, 0, sig_length))
	{
		printf("%s is not a PNG file!\n", filename);
		exit(EXIT_FAILURE);
	}
	if (!(pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
	{
		printf("Failed creating PNG struct reading %s.\n", filename);
		exit(EXIT_FAILURE);
	}
	if (!(infop = png_create_info_struct(pngp)))
	{
		printf("Failed creating PNG info struct reading %s.\n", filename);
		exit(EXIT_FAILURE);
	}
	if (setjmp(pngp->jmpbuf))
	{
		printf("Failed while reading PNG file: %s\n", filename);
		exit(EXIT_FAILURE);
	}
	png_init_io(pngp, fp);
	png_set_sig_bytes(pngp, sig_length);
	png_read_info(pngp, infop);

	/* Transformations to ensure we end up with 32bpp, 4 channel RGBA */
	png_set_strip_16(pngp);
	png_set_gray_to_rgb(pngp);
	png_set_packing(pngp);
	png_set_palette_to_rgb(pngp);
	png_set_tRNS_to_alpha(pngp);
	png_set_filler(pngp, 0xFF, PNG_FILLER_AFTER);
	png_set_gray_1_2_4_to_8(pngp);

	png_read_update_info(pngp, infop);
	png_get_IHDR(pngp, infop, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	stride = png_get_rowbytes(pngp, infop);
	row_pointers = malloc(sizeof(png_bytep) * height);

	image_data = malloc(height * width * 4);
	for (y = 0; y < height; y++)
	{
		row_pointers[y] = image_data + (y * width * 4);
	}

	png_read_image(pngp, row_pointers);
	png_read_end(pngp, infop);
	fclose(fp);

	gfx = malloc(sizeof(*gfx));
	gfx->w = width;
	gfx->h = height;
	gfx->pixels = (char *)image_data;

	png_destroy_read_struct(&pngp, &infop, NULL);
	free(row_pointers);

	return gfx;
}

static MODEL *readModel(const char *filename, const char *path)
{
	FILE *fp = fopen(filename, "r");
	int num, x, meshes, mesh;
	char s[200], fullPath[PATH_MAX];
	MODEL *psModel;

	if (!fp)
	{
		fprintf(stderr, "Cannot open \"%s\" for reading: %s", input, strerror(errno));
		exit(1);
	}

	num = fscanf(fp, "WZM %d\n", &x);
	if (num != 1 || x != 1)
	{
		fprintf(stderr, "Bad WZM file %s\n", input);
		exit(1);
	}

	num = fscanf(fp, "TEXTURE %s\n", s);
	if (num != 1)
	{
		fprintf(stderr, "Bad TEXTURE directive in %s\n", input);
		exit(1);
	}
	strcpy(fullPath, path);
	strcat(fullPath, s);

	num = fscanf(fp, "MESHES %d", &meshes);
	if (num != 1)
	{
		fprintf(stderr, "Bad MESHES directive in %s\n", input);
		exit(1);
	}
	psModel = createModel(meshes);

	// WZM does not support multiple meshes, nor importing them from PIE
	for (mesh = 0; mesh < meshes; mesh++)
	{
		MESH *psMesh = &psModel->mesh[mesh];
		int j;

		num = fscanf(fp, "\nMESH %d\n", &x);
		if (num != 1 || mesh != x)
		{
			fprintf(stderr, "Bad MESH directive in %s, was %d should be %d.\n", input, x, mesh);
			exit(1);
		}

		num = fscanf(fp, "TEAMCOLOURS %d\n", &x);
		if (num != 1 || x > 1 || x < 0)
		{
			fprintf(stderr, "Bad TEAMCOLOURS directive in %s, mesh %d.\n", input, mesh);
			exit(1);
		}
		psMesh->teamColours = x;

		num = fscanf(fp, "VERTICES %d\n", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad VERTICES directive in %s, mesh %d.\n", input, mesh);
			exit(1);
		}
		psMesh->vertices = x;
		psMesh->vertexArray = malloc(sizeof(GLfloat) * x * 3);

		num = fscanf(fp, "FACES %d\n", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad VERTICES directive in %s, mesh %d.\n", input, mesh);
			exit(1);
		}
		psMesh->faces = x;
		psMesh->indexArray = malloc(sizeof(GLuint) * x * 3);

		(void) fscanf(fp, "VERTEXARRAY");

		for (j = 0; j < psMesh->vertices; j++)
		{
			GLfloat *v = &psMesh->vertexArray[j * 3];

			num = fscanf(fp, "%f %f %f\n", &v[0], &v[1], &v[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad VERTEXARRAY entry mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}

		num = fscanf(fp, "TEXTUREARRAYS %d", &x);
		if (num != 1 || x < 0)
		{
			fprintf(stderr, "Bad TEXTUREARRAYS directive in %s, mesh %d.\n", input, mesh);
			exit(1);
		}
		psMesh->textureArrays = x;
		for (j = 0; j < psMesh->textureArrays; j++)
		{
			int k;

			num = fscanf(fp, "\nTEXTUREARRAY %d", &x);
			if (num != 1 || x < 0 || x != j)
			{
				fprintf(stderr, "Bad TEXTUREARRAY directive in %s, mesh %d, array %d.\n", input, mesh, j);
				exit(1);
			}
			psMesh->textureArray[j] = malloc(sizeof(GLfloat) * psMesh->vertices * 2);
			for (k = 0; k < psMesh->vertices; k++)
			{
				GLfloat *v = &psMesh->textureArray[j][k * 2];

				num = fscanf(fp, "\n%f %f", &v[0], &v[1]);
				if (num != 2)
				{
					fprintf(stderr, "Bad TEXTUREARRAY entry mesh %d, array %d, number %d\n", mesh, j, k);
					exit(1);
				}
			}
		}

		(void) fscanf(fp, "\nINDEXARRAY");

		for (j = 0; j < psMesh->faces; j++)
		{
			GLuint *v = &psMesh->indexArray[j * 3];

			num = fscanf(fp, "\n%u %u %u", &v[0], &v[1], &v[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad INDEXARRAY directive in mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}

		num = fscanf(fp, "\nFRAMES %d", &psMesh->frames);
		if (num != 1)
		{
			fprintf(stderr, "Bad FRAMES directive in mesh %d\n", mesh);
			exit(1);
		}

		// throw away for now
		for (j = 0; j < psMesh->frames; j++)
		{
			int tex, translation[3], rotation[3];
			float spend;

			num = fscanf(fp, "\n%f %d %d %d %d %d %d %d", &spend, &tex, &translation[0], &translation[1], &translation[2],
			             &rotation[0], &rotation[1], &rotation[2]);
			if (num != 8)
			{
				fprintf(stderr, "Bad FRAMES directive in mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}

		num = fscanf(fp, "\nCONNECTORS %d", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad CONNECTORS directive in mesh %d\n", mesh);
			exit(1);
		}

		// throw away for now
		for (j = 0; j < x; j++)
		{
			int pos[3];

			num = fscanf(fp, "\n%d %d %d", &pos[0], &pos[1], &pos[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad CONNECTORS directive in mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}
	}

	// Do this last for faster debug time ;)
	psModel->pixmap = readPixmap(fullPath);

	return psModel;
}

static void freeModel(MODEL *psModel)
{
	int i;

	for (i = 0; i < psModel->meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];
		int j;

		free(psMesh->vertexArray);
		free(psMesh->indexArray);
		for (j = 0; j < MAX_TEXARRAYS; j++)
		{
			free(psMesh->textureArray[j]);
		}
		free(psMesh->frameArray);
	}
	free(psModel->mesh);
	free(psModel);
}

static void resizeWindow(int width, int height)
{
	glViewport(0, 0, width, height);
}

static void drawModel(MODEL *psModel, int x, int y)
{
	int i;

	glColor3f(1.0f, 1.0f, 1.0f);
	for (i = 0; i < psModel->meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];

		glTexCoordPointer(2, GL_FLOAT, 0, psMesh->textureArray[psMesh->currentFrame]);
		glVertexPointer(3, GL_FLOAT, 0, psMesh->vertexArray);

		glDrawElements(GL_TRIANGLES, psMesh->faces * 3, GL_UNSIGNED_INT, psMesh->indexArray);
	}
}

static void prepareModel(MODEL *psModel)
{
	glGenTextures(1, &psModel->texture);
	glBindTexture(GL_TEXTURE_2D, psModel->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, psModel->pixmap->w, psModel->pixmap->h, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, psModel->pixmap->pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

int main(int argc, char **argv)
{
	MODEL *psModel;
	const int width = 640, height = 480;
	SDL_Event event;
	GLfloat angle = 0.0f;
	const float aspect = (float)width / (float)height;
	bool quit = false;

	parse_args(argc, argv);
	psModel = readModel(input, texPath);

	/* Initialize SDL */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	atexit(SDL_Quit);

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
	gluPerspective(45.0f, aspect, 0.1f, 200.0f);
	glMatrixMode(GL_MODELVIEW);

	prepareModel(psModel);

	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			SDL_keysym *keysym = &event.key.keysym;
			int i;

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

								if (psMesh->currentFrame < psMesh->frames - 1)
								{
									psMesh->currentFrame++;
								}
								else
								{
									psMesh->currentFrame = 0;
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
		glTranslatef(0.0f, -30.0f, -125.0f);;
		glRotatef(angle, 0, 1, 0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		drawModel(psModel, 0, 0);
		SDL_GL_SwapBuffers();
		SDL_Delay(5);
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
