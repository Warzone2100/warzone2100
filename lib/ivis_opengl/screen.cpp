/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#include <GLee.h>
#include "lib/framework/frame.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include <SDL.h>
#include <physfs.h>
#include <png.h>
#include "lib/ivis_common/png_util.h"
#include "lib/ivis_common/tex.h"

#include "lib/framework/frameint.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/pieclip.h"

#include "screen.h"
#include "src/console.h"
#include "src/levels.h"

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

static int preview_width = 0, preview_height = 0;
static Vector2i player_pos[MAX_PLAYERS];
static BOOL mappreview = false;
static char mapname[256];

/* Initialise the double buffered display */
bool screenInitialise(
			UDWORD		width,		// Display width
			UDWORD		height,		// Display height
			UDWORD		bitDepth,	// Display bit depth
			unsigned int fsaa,      // FSAA anti aliasing level
			bool		fullScreen,	// Whether to start windowed
							// or full screen
			bool		vsync)		// If to sync to vblank or not
{
	int video_flags = 0;
	int bpp = 0, value;
	char buf[512];
	GLint glMaxTUs;
	// Fetch the video info.
	const SDL_VideoInfo* video_info = SDL_GetVideoInfo();

	if (width == 0 || height == 0)
	{
		pie_SetVideoBufferWidth(width = screenWidth = video_info->current_w);
		pie_SetVideoBufferHeight(height = screenHeight = video_info->current_h);
		pie_SetVideoBufferDepth(bitDepth = screenDepth = video_info->vfmt->BitsPerPixel);
	}
	else
	{
		screenWidth = width;
		screenHeight = height;
		screenDepth = bitDepth;
	}
	screenWidth = MAX(screenWidth, 640);
	screenHeight = MAX(screenHeight, 480);

	if (!video_info)
	{
		return false;
	}

	// The flags to pass to SDL_SetVideoMode.
	video_flags  = SDL_OPENGL;    // Enable OpenGL in SDL.
	video_flags |= SDL_ANYFORMAT; // Don't emulate requested BPP if not available.

	if (fullScreen)
	{
		video_flags |= SDL_FULLSCREEN;
	}

	// Set the double buffer OpenGL attribute.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable vsync if requested by the user
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);

	// Enable FSAA anti-aliasing if and at the level requested by the user
	if (fsaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);
	}

	bpp = SDL_VideoModeOK(width, height, bitDepth, video_flags);
	if (!bpp)
	{
		debug(LOG_ERROR, "Video mode %dx%d@%dbpp is not supported!", width, height, bitDepth);
		return false;
	}
	switch (bpp)
	{
		case 32:
		case 24:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			break;
		case 16:
			info("Using colour depth of %i instead of %i.", bpp, screenDepth);
			info("You will experience graphics glitches!");
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			break;
		case 8:
			debug(LOG_FATAL, "You don't want to play Warzone with a bit depth of %i, do you?", bpp);
			exit(1);
			break;
		default:
			debug(LOG_FATAL, "Unsupported bit depth: %i", bpp);
			exit(1);
			break;
	}

	screen = SDL_SetVideoMode(width, height, bpp, video_flags);
	if (!screen)
	{
		debug(LOG_ERROR, "SDL_SetVideoMode failed (%s).", SDL_GetError());
		return false;
	}
	if ( SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == -1)
	{
		debug( LOG_FATAL, "OpenGL initialization did not give double buffering!" );
		debug( LOG_FATAL, "Double buffering is required for this game!");
		exit(1);
	}
	
	/* Dump general information about OpenGL implementation to the console and the dump file */
	ssprintf(buf, "OpenGL Vendor : %s", glGetString(GL_VENDOR));
	addDumpInfo(buf);
	debug(LOG_3D, "%s", buf);
	ssprintf(buf, "OpenGL Renderer : %s", glGetString(GL_RENDERER));
	addDumpInfo(buf);
	debug(LOG_3D, "%s", buf);
	ssprintf(buf, "OpenGL Version : %s", glGetString(GL_VERSION));
	addDumpInfo(buf);
	debug(LOG_3D, "%s", buf);
	ssprintf(buf, "Video Mode %d x %d (%d bpp) (%s)", width, height, bpp, fullScreen ? "fullscreen" : "window");
	addDumpInfo(buf);
	debug(LOG_3D, "%s", buf);

	/* Dump extended information about OpenGL implementation to the console */
	debug(LOG_3D, "OpenGL Extensions : %s", glGetString(GL_EXTENSIONS)); // FIXME This is too much for MAX_LEN_LOG_LINE
	debug(LOG_3D, "Supported OpenGL extensions:");
	debug(LOG_3D, "  * OpenGL 1.2 %s supported!", GLEE_VERSION_1_2 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.3 %s supported!", GLEE_VERSION_1_3 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.4 %s supported!", GLEE_VERSION_1_4 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.5 %s supported!", GLEE_VERSION_1_5 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.0 %s supported!", GLEE_VERSION_2_0 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.1 %s supported!", GLEE_VERSION_2_1 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 3.0 %s supported!", GLEE_VERSION_3_0 ? "is" : "is NOT");
	debug(LOG_3D, "  * Texture compression %s supported.", GLEE_ARB_texture_compression ? "is" : "is NOT");
	debug(LOG_3D, "  * Two side stencil %s supported.", GLEE_EXT_stencil_two_side ? "is" : "is NOT");
	debug(LOG_3D, "  * ATI separate stencil is%s supported.", GLEE_ATI_separate_stencil ? "" : " NOT");
	debug(LOG_3D, "  * Stencil wrap %s supported.", GLEE_EXT_stencil_wrap ? "is" : "is NOT");
	debug(LOG_3D, "  * Anisotropic filtering %s supported.", GLEE_EXT_texture_filter_anisotropic ? "is" : "is NOT");
	debug(LOG_3D, "  * Rectangular texture %s supported.", GLEE_ARB_texture_rectangle ? "is" : "is NOT");
	debug(LOG_3D, "  * FrameBuffer Object (FBO) %s supported.", GLEE_EXT_framebuffer_object ? "is" : "is NOT");
	debug(LOG_3D, "  * Vertex Buffer Object (VBO) %s supported.", GLEE_ARB_vertex_buffer_object ? "is" : "is NOT");
	debug(LOG_3D, "  * NPOT %s supported.", GLEE_ARB_texture_non_power_of_two ? "is" : "is NOT");
	debug(LOG_3D, "  * texture cube_map %s supported.", GLEE_ARB_texture_cube_map ? "is" : "is NOT");
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &glMaxTUs);
	debug(LOG_3D, "  * Total number of Texture Units (TUs) supported is %d.", (int) glMaxTUs);

	if (!GLEE_VERSION_1_4)
	{
		debug(LOG_FATAL, "OpenGL 1.4 + VBO extension is required for this game!");
		exit(1);
	}

#ifndef WZ_OS_MAC
	// Make OpenGL's VBO functions available under the core names for
	// implementations that have them only as extensions, namely Mesa.
	if (!GLEE_VERSION_1_5)
	{
		if (GLEE_ARB_vertex_buffer_object)
		{
			info("Using VBO extension functions under the core names.");

			GLeeFuncPtr_glBindBuffer = GLeeFuncPtr_glBindBufferARB;
			GLeeFuncPtr_glDeleteBuffers = GLeeFuncPtr_glDeleteBuffersARB;
			GLeeFuncPtr_glGenBuffers = GLeeFuncPtr_glGenBuffersARB;
			GLeeFuncPtr_glIsBuffer = GLeeFuncPtr_glIsBufferARB;
			GLeeFuncPtr_glBufferData = GLeeFuncPtr_glBufferDataARB;
			GLeeFuncPtr_glBufferSubData = GLeeFuncPtr_glBufferSubDataARB;
			GLeeFuncPtr_glGetBufferSubData = GLeeFuncPtr_glGetBufferSubDataARB;
			GLeeFuncPtr_glMapBuffer = GLeeFuncPtr_glMapBufferARB;
			GLeeFuncPtr_glUnmapBuffer = GLeeFuncPtr_glUnmapBufferARB;
			GLeeFuncPtr_glGetBufferParameteriv = GLeeFuncPtr_glGetBufferParameterivARB;
			GLeeFuncPtr_glGetBufferPointerv = GLeeFuncPtr_glGetBufferPointervARB;
		}
		else
		{
			debug(LOG_FATAL, "OpenGL 1.4 + VBO extension is required for this game!");
			exit(1);
		}

		debug(LOG_WARNING, "OpenGL 1.5 is not supported by your system! Expect some glitches...");
	}
#endif

	/* Dump information about OpenGL 2.0+ implementation to the console and the dump file */
	if (GLEE_VERSION_2_0)
	{
		GLint glMaxTIUs;

		debug(LOG_3D, "  * OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		ssprintf(buf, "OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		addDumpInfo(buf);

		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &glMaxTIUs);
		debug(LOG_3D, "  * Total number of Texture Image Units (TIUs) supported is %d.", (int) glMaxTIUs);

		if (!pie_LoadShaders())
			debug(LOG_INFO, "Can't use shaders, switching back to fixed pipeline.");;
	}
	else
	{
		debug(LOG_POPUP, "OpenGL 2.0 is not supported by your system, current shaders require this.");
		debug(LOG_POPUP, "Team colors will not function correctly on your system.");
	}

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, (double)width, (double)height, 0.0f, 1.0f, -1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glErrors();
	return true;
}


/* Release the DD objects */
void screenShutDown(void)
{
	if (screen != NULL)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glFlush();
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
void screen_Upload(const char *newBackDropBmp, BOOL preview)
{
	static bool processed = false;
	int x1 = 0, x2 = screenWidth, y1 = 0, y2 = screenHeight, i, scale = 0, w = 0, h = 0;
	float tx = 1, ty = 1;
	const float aspect = screenWidth / (float)screenHeight, backdropAspect = 4 / (float)3;

	if (aspect < backdropAspect)
	{
		int offset = (screenWidth - screenHeight * backdropAspect) / 2;
		x1 += offset;
		x2 -= offset;
	}
	else
	{
		int offset = (screenHeight - screenWidth / backdropAspect) / 2;
		y1 += offset;
		y2 -= offset;
	}

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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		processed = true;
	}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	// Make sure the current texture page is reloaded after we are finished
	// Otherwise WZ will think it is still loaded and not load it again
	pie_SetTexturePage(TEXPAGE_EXTERN);

	glBindTexture(GL_TEXTURE_2D, backDropTexture);
	glColor3f(1, 1, 1);

	if (preview)
	{
		int s1, s2;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		s1 = screenWidth / preview_width;
		s2 = screenHeight / preview_height;
		scale = MIN(s1, s2);

		w = preview_width * scale;
		h = preview_height * scale;
		x1 = screenWidth / 2 - w / 2;
		x2 = screenWidth / 2 + w / 2;
		y1 = screenHeight / 2 - h / 2;
		y2 = screenHeight / 2 + h / 2;

		tx = preview_width / (float)BACKDROP_HACK_WIDTH;
		ty = preview_height / (float)BACKDROP_HACK_HEIGHT;
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0);
		glVertex2f(x1, y1);
		glTexCoord2f(tx, 0);
		glVertex2f(x2, y1);
		glTexCoord2f(0, ty);
		glVertex2f(x1, y2);
		glTexCoord2f(tx, ty);
		glVertex2f(x2, y2);
	glEnd();

	if (preview)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			int x = player_pos[i].x;
			int y = player_pos[i].y;
			char text[5];

			if (x == 0x77777777)
				continue;

			x = screenWidth / 2 - w / 2 + x * scale;
			y = screenHeight / 2 - h / 2 + y * scale;
			ssprintf(text, "%d", i);
			iV_SetFont(font_large);
			iV_SetTextColour(WZCOL_BLACK);
			iV_DrawText(text, x - 1, y - 1);
			iV_DrawText(text, x + 1, y - 1);
			iV_DrawText(text, x - 1, y + 1);
			iV_DrawText(text, x + 1, y + 1);
			iV_SetTextColour(WZCOL_WHITE);
			iV_DrawText(text, x, y);
		}
	}
}

void screen_enableMapPreview(char *name, int width, int height, Vector2i *playerpositions)
{
	int i;
	mappreview = true;
	preview_width = width;
	preview_height = height;
	sstrcpy(mapname, name);
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		player_pos[i].x = playerpositions[i].x;
		player_pos[i].y = playerpositions[i].y;
	}
}

void screen_disableMapPreview(void)
{
	mappreview = false;
	sstrcpy(mapname, "none");
}

BOOL screen_getMapPreview(void)
{
	return mappreview;
}

/* Swap between windowed and full screen mode */
void screenToggleMode(void)
{
	(void) SDL_WM_ToggleFullScreen(screen);
}

// Screenshot code goes below this
static const unsigned int channelsPerPixel = 3;

/** Writes a screenshot of the current frame to file.
 *
 *  Performs the actual work of writing the frame currently displayed on screen
 *  to the filename specified by screenDumpToDisk().
 *
 *  @NOTE This function will only dump a screenshot to file if it was requested
 *        by screenDumpToDisk().
 *
 *  \sa screenDumpToDisk()
 */
void screenDoDumpToDiskIfRequired(void)
{
	const char* fileName = screendump_filename;
	iV_Image image = { 0, 0, 8, NULL };

	if (!screendump_required) return;
	debug( LOG_3D, "Saving screenshot %s\n", fileName );

	// Dump the currently displayed screen in a buffer
	// Casting to unsigned int here to prevent GCC from warning about a
	// comparison between unsigned and signed integers. Why does SDL use
	// a signed integer anyway? When will your screen ever have a negative
	// width or height for your screen? Assert it to be sure though. -- Giel
	ASSERT(screen->w >= 0 && screen->h >= 0, "Somehow our screen has negative dimensions! Width = %d; Height = %d", screen->w, screen->h);
	if (image.width != (unsigned int)screen->w || image.height != (unsigned int)screen->h)
	{
		if (image.bmp != NULL)
		{
			free(image.bmp);
		}

		image.width = screen->w;
		image.height = screen->h;
		image.bmp = (unsigned char *)malloc(channelsPerPixel * image.width * image.height);
		if (image.bmp == NULL)
		{
			image.width = 0; image.height = 0;
			debug(LOG_ERROR, "Couldn't allocate memory");
			return;
		}
	}
	glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.bmp);

	iV_saveImage_PNG(fileName, &image);
	iV_saveImage_JPEG(fileName, &image);

	// display message to user about screenshot
	snprintf(ConsoleString,sizeof(ConsoleString),"Screenshot %s saved!",fileName);
	addConsoleMessage(ConsoleString, LEFT_JUSTIFY,SYSTEM_MESSAGE);
	if (image.bmp)
	{
		free(image.bmp);
	}
	screendump_required = false;
}

/** Registers the currently displayed frame for making a screen shot.
 *
 *  The filename will be suffixed with a number, such that no files are
 *  overwritten.
 *
 *  \param path The directory path to save the screenshot in.
 */
void screenDumpToDisk(const char* path)
{
	unsigned int screendump_num = 0;
	time_t aclock;
	struct tm *t;

	time(&aclock);           /* Get time in seconds */
	t = localtime(&aclock);  /* Convert time to struct */

	ssprintf(screendump_filename, "%s/wz2100-%04d%02d%02d_%02d%02d%02d-%s.png", path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, getLevelName());

        while (PHYSFS_exists(screendump_filename))
	{
		ssprintf(screendump_filename, "%s/wz2100-%04d%02d%02d_%02d%02d%02d-%s-%d.png", path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, getLevelName(), ++screendump_num);
	}
	screendump_required = true;
}

