/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
 * Screen.c
 *
 * Basic double buffered display using direct draw.
 *
 */

#include "lib/ivis_opengl/GLee.h"
#include "lib/framework/frame.h"

#include <SDL.h>
#include <physfs.h>
#include <png.h>
#include "lib/ivis_common/png_util.h"
#include "lib/ivis_common/tex.h"

#include "lib/framework/frameint.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "screen.h"

/* The Current screen size and bit depth */
UDWORD		screenWidth = 0;
UDWORD		screenHeight = 0;
UDWORD		screenDepth = 0;

/* global used to indicate preferred internal OpenGL format */
int wz_texture_compression;

static SDL_Surface	*screen = NULL;
static BOOL		bBackDrop = false;
static char		screendump_filename[PATH_MAX];
static BOOL		screendump_required = false;
static GLuint		backDropTexture = ~0;

/* Initialise the double buffered display */
BOOL screenInitialise(
			UDWORD		width,		// Display width
			UDWORD		height,		// Display height
			UDWORD		bitDepth,	// Display bit depth
			BOOL		fullScreen	// Whether to start windowed
							// or full screen
			)
{
	static int video_flags = 0;
	int bpp = 0, value;

	/* Store the screen information */
	screenWidth = width;
	screenHeight = height;
	screenDepth = bitDepth;

	// Calculate the common flags for windowed and fullscreen modes.
	if (video_flags == 0) {
		// Fetch the video info.
		const SDL_VideoInfo* video_info = SDL_GetVideoInfo();

		if (!video_info) {
			return false;
		}

		// The flags to pass to SDL_SetVideoMode.
		video_flags  = SDL_OPENGL;    // Enable OpenGL in SDL.
		video_flags |= SDL_ANYFORMAT; // Don't emulate requested BPP if not available.
		video_flags |= SDL_HWPALETTE; // Store the palette in hardware.

		// This checks to see if surfaces can be stored in memory.
		if (video_info->hw_available) {
			video_flags |= SDL_HWSURFACE;
		} else {
			video_flags |= SDL_SWSURFACE;
		}

		// This checks if hardware blits can be done.
		if (video_info->blit_hw) {
			video_flags |= SDL_HWACCEL;
		}

		if (fullScreen) {
			video_flags |= SDL_FULLSCREEN;
		}

		// Set the double buffer OpenGL attribute.
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		bpp = SDL_VideoModeOK(width, height, bitDepth, video_flags);
		if (!bpp) {
			debug( LOG_ERROR, "Error: Video mode %dx%d@%dbpp is not supported!\n", width, height, bitDepth );
			return false;
		}
		switch ( bpp )
		{
			case 32:
			case 24:
				SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
				SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
				break;
			case 16:
				debug( LOG_ERROR, "Warning: Using colour depth of %i instead of %i.", bpp, screenDepth );
				debug( LOG_ERROR, "         You will experience graphics glitches!" );
				SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
				SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
				SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
				SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
				SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
				break;
			case 8:
				debug( LOG_ERROR, "Error: You don't want to play Warzone with a bit depth of %i, do you?", bpp );
				exit( 1 );
				break;
			default:
				debug( LOG_ERROR, "Error: Unsupported bit depth: %i", bpp );
				exit( 1 );
				break;
		}
	}

	screen = SDL_SetVideoMode(width, height, bpp, video_flags);
	if ( !screen ) {
		debug( LOG_ERROR, "Error: SDL_SetVideoMode failed (%s).", SDL_GetError() );
		return false;
	}
	if ( SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == -1)
	{
		debug( LOG_ERROR, "OpenGL initialization did not give double buffering!" );
	}
	// Note that no initialisation of GLee is required, since this is handled automatically.

	/* Dump information about OpenGL implementation to the console */
	debug(LOG_3D, "OpenGL Vendor : %s", glGetString(GL_VENDOR));
	debug(LOG_3D, "OpenGL Renderer : %s", glGetString(GL_RENDERER));
	debug(LOG_3D, "OpenGL Version : %s", glGetString(GL_VERSION));
	debug(LOG_3D, "OpenGL Extensions : %s", glGetString(GL_EXTENSIONS)); // FIXME This is too much for MAX_LEN_LOG_LINE
	debug(LOG_3D, "Supported OpenGL extensions:");
	debug(LOG_3D, "  * OpenGL 1.2 %s supported!", GLEE_VERSION_1_2 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.3 %s supported!", GLEE_VERSION_1_3 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.4 %s supported!", GLEE_VERSION_1_4 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.5 %s supported!", GLEE_VERSION_1_5 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.0 %s supported!", GLEE_VERSION_2_0 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.1 %s supported!", GLEE_VERSION_2_1 ? "is" : "is NOT");
	debug(LOG_3D, "  * Texture compression %s supported.", GLEE_ARB_texture_compression ? "is" : "is NOT");
	debug(LOG_3D, "  * Two side stencil %s supported.", GLEE_EXT_stencil_two_side ? "is" : "is NOT");
	debug(LOG_3D, "  * Stencil wrap %s supported.", GLEE_EXT_stencil_wrap ? "is" : "is NOT");
	debug(LOG_3D, "  * Anisotropic filtering %s supported.", GLEE_EXT_texture_filter_anisotropic ? "is" : "is NOT");

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, height, 0, 1, -1);

	glMatrixMode(GL_TEXTURE);
	glScalef(1.0f/OLD_TEXTURE_SIZE_FIX, 1.0f/OLD_TEXTURE_SIZE_FIX, 1.0f); // FIXME Scaling texture coords to 256x256!

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	return true;
}


/* Release the DD objects */
void screenShutDown(void)
{
	if (screen != NULL)
	{
		SDL_FreeSurface(screen);
		screen = NULL;
	}
}


void screen_SetBackDropFromFile(const char* filename)
{
	// HACK : We should use a resource handler here!
	const char *extension = strrchr(filename, '.');// determine the filetype
	iV_Image image;

	if(!extension)
	{
		debug(LOG_ERROR, "Image without extension: \"%s\"!", filename);
		return; // filename without extension... don't bother
	}

	// Make sure the current texture page is reloaded after we are finished
	// Otherwise WZ will think it is still loaded and not load it again
	pie_SetTexturePage(TEXPAGE_NONE);

	if( strcmp(extension,".png") == 0 )
	{
		if (iV_loadImage_PNG( filename, &image ) )
		{
			if (~backDropTexture == 0)
				glGenTextures(1, &backDropTexture);

			glBindTexture(GL_TEXTURE_2D, backDropTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					image.width, image.height,
					0, iV_getPixelFormat(&image), GL_UNSIGNED_BYTE, image.bmp);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

			iV_unloadImage(&image);
		}
		return;
	}
	else
		debug(LOG_ERROR, "Unknown extension \"%s\" for image \"%s\"!", extension, filename);
}
//===================================================================

void screen_StopBackDrop(void)
{
	bBackDrop = false;	//checking [movie]
}

void screen_RestartBackDrop(void)
{
	bBackDrop = true;
}

BOOL screen_GetBackDrop(void)
{
	return bBackDrop;
}

//******************************************************************
//slight hack to display maps (or whatever) in background.
//bitmap MUST be (BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT) for now.
void screen_Upload(const char *newBackDropBmp)
{
	static bool processed = false;

	if(newBackDropBmp != NULL)
	{
		if (processed)	// lets free a texture when we use a new one.
		{
			glDeleteTextures( 1, &backDropTexture );
		}

		glGenTextures(1, &backDropTexture);
		pie_SetTexturePage(TEXPAGE_NONE);
		glBindTexture(GL_TEXTURE_2D, backDropTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
			BACKDROP_HACK_WIDTH, BACKDROP_HACK_HEIGHT,
			0, GL_RGB, GL_UNSIGNED_BYTE, newBackDropBmp);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		processed = true;
	}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	// Make sure the current texture page is reloaded after we are finished
	// Otherwise WZ will think it is still loaded and not load it again
	pie_SetTexturePage(TEXPAGE_NONE);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, backDropTexture);
	glColor3f(1, 1, 1);

	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0);
		glVertex2f(0, 0);
		glTexCoord2f(255, 0);
		glVertex2f(screenWidth, 0);
		glTexCoord2f(0, 255);
		glVertex2f(0, screenHeight);
		glTexCoord2f(255, 255);
		glVertex2f(screenWidth, screenHeight);
	glEnd();
}

/* Swap between windowed and full screen mode */
void screenToggleMode(void)
{
	(void) SDL_WM_ToggleFullScreen(screen);
}

// Screenshot code goes below this
static const unsigned int channelsPerPixel = 3;

void screenDoDumpToDiskIfRequired(void)
{
	const char* fileName = screendump_filename;
	static iV_Image image = { 0, 0, 0, NULL };

	if (!screendump_required) return;
	debug( LOG_3D, "Saving screenshot %s\n", fileName );

	// Dump the currently displayed screen in a buffer
	// Casting to unsigned int here to prevent GCC from warning about a
	// comparison between unsigned and signed integers. Why does SDL use
	// a signed integer anyway? When will your screen ever have a negative
	// width or height for your screen? Assert it to be sure though. -- Giel
	ASSERT(screen->w >= 0 && screen->h >= 0, "screenDoDumpToDiskIfRequired: Somehow our screen has negative dimensions! Width = %d; Height = %d", screen->w, screen->h);
	if (image.width != (unsigned int)screen->w || image.height != (unsigned int)screen->h)
	{
		if (image.bmp != NULL)
		{
			free(image.bmp);
		}

		image.width = screen->w;
		image.height = screen->h;
		image.bmp = malloc(channelsPerPixel * image.width * image.height);
		if (image.bmp == NULL)
		{
			image.width = 0; image.height = 0;
			debug(LOG_ERROR, "screenDoDumpToDiskIfRequired: Couldn't allocate memory\n");
			return;
		}
	}
	glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.bmp);

	// Write the screen to a PNG
	iV_saveImage_PNG(fileName, &image);

	screendump_required = false;
}

void screenDumpToDisk(const char* path) {
	static unsigned int screendump_num = 0;

	while (++screendump_num != 0) {
		// We can safely use '/' as path separator here since PHYSFS uses that as its default separator
		snprintf(screendump_filename, PATH_MAX, "%s/wz2100_shot_%03i.png", path, screendump_num);
		if (!PHYSFS_exists(screendump_filename)) {
			// Found a usable filename, so we'll stop searching.
			break;
		}
	}

	ASSERT( screendump_num != 0, "screenDumpToDisk: integer overflow; no more filenumbers available.\n" );

	// If we have an integer overflow, we don't want to go about and overwrite files
	if (screendump_num != 0)
		screendump_required = true;
}
