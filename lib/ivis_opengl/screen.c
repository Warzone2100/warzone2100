/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
#include <physfs.h>
#include <png.h>
#include "lib/ivis_common/png_util.h"
#include "lib/ivis_common/tex.h"

#include "lib/framework/frameint.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/pieblitfunc.h"

#if defined(WZ_OS_MAC)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include "screen.h"
#include "src/console.h"
#include "src/levels.h"

/* global used to indicate preferred internal OpenGL format */
int wz_texture_compression;

static BOOL		bBackDrop = false;
static char		screendump_filename[PATH_MAX];
static BOOL		screendump_required = false;
static GLuint		backDropTexture = ~0;

// Variables needed for our FBO
GLuint fbo;					// Our handle to the FBO
GLuint FBOtexture;			// The texture we are going to use
GLuint FBOdepthbuffer;		// Our handle to the depth render buffer
static BOOL FBOinit = false;
BOOL bFboProblem = false;	// hack to work around people with bad drivers. (*cough*intel*cough*)

/* Initialise the double buffered display */
bool screenInitialise()
{
	char buf[256];
	GLint glMaxTUs;

	// Copy this info to be used by the crash handler for the dump file
	ssprintf(buf, "OpenGL Vendor : %s", glGetString(GL_VENDOR));
	addDumpInfo(buf);
	ssprintf(buf, "OpenGL Renderer : %s", glGetString(GL_RENDERER));
	addDumpInfo(buf);
	ssprintf(buf, "OpenGL Version : %s", glGetString(GL_VERSION));
	addDumpInfo(buf);
	if (GLEE_VERSION_2_0)
	{
		{
		ssprintf(buf, "OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		addDumpInfo(buf);
		}
	}
	/* Dump general information about OpenGL implementation to the console and the dump file */
	ssprintf(buf, "OpenGL Vendor : %s", glGetString(GL_VENDOR));
	addDumpInfo(buf);
	debug(LOG_3D, buf);
	ssprintf(buf, "OpenGL Renderer : %s", glGetString(GL_RENDERER));
	addDumpInfo(buf);
	debug(LOG_3D, buf);
	ssprintf(buf, "OpenGL Version : %s", glGetString(GL_VERSION));
	addDumpInfo(buf);
	debug(LOG_3D, buf);

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
		debug(LOG_INFO, "OpenGL 2.0 is not supported by your system, using fixed pipeline.");
	}

/*  // This code block left over from qt-trunk merge, should probably be deleted.
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, (double)width, (double)height, 0.0f, 1.0f, -1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);
*/

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glErrors();
	return true;
}


/* Release the DD objects */
void screenShutDown(void)
{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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
	pie_SetTexturePage(TEXPAGE_FONT);

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
	pie_SetTexturePage(TEXPAGE_FONT);

	glBindTexture(GL_TEXTURE_2D, backDropTexture);
	glColor3f(1, 1, 1);

	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0);
		glVertex2f(0, 0);
		glTexCoord2f(1, 0);
		glVertex2f(screenWidth, 0);
		glTexCoord2f(0, 1);
		glVertex2f(0, screenHeight);
		glTexCoord2f(1, 1);
		glVertex2f(screenWidth, screenHeight);
	glEnd();
}

/* Swap between windowed and full screen mode */
void screenToggleMode(void)
{
	// TODO
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

	if (image.width != screenWidth || image.height != screenHeight)
	{
		if (image.bmp != NULL)
		{
			free(image.bmp);
		}

		image.width = screenWidth;
		image.height = screenHeight;
		image.bmp = malloc(channelsPerPixel * image.width * image.height);
		if (image.bmp == NULL)
		{
			image.width = 0; image.height = 0;
			debug(LOG_ERROR, "Couldn't allocate memory");
			return;
		}
	}
	glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.bmp);

	// Write the screen to a PNG
	iV_saveImage_PNG(fileName, &image);

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
	static unsigned int screendump_num = 0;

	while (++screendump_num != 0) {
		// We can safely use '/' as path separator here since PHYSFS uses that as its default separator
		ssprintf(screendump_filename, "%s/wz2100_%s_shot_%03i.png", path, getLevelName(), screendump_num);
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


BOOL Init_FBO(unsigned int width, unsigned int height)
{
	GLenum status;

	glErrors();
	// Bail out if FBOs aren't supported
	if (!GLEE_EXT_framebuffer_object)
		return false;

	// No need to create two FBOs
	if (FBOinit)
		return true;

	// Create the FBO
	glGenFramebuffersEXT(1, &fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

	// create depthbuffer
	glGenRenderbuffersEXT(1, &FBOdepthbuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, FBOdepthbuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, FBOdepthbuffer);

	// Now setup a texture to render to
	glGenTextures(1, &FBOtexture);
	glBindTexture(GL_TEXTURE_2D, FBOtexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width, height,0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// attach that texture to the color
	glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
				   GL_TEXTURE_2D, FBOtexture, 0);
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0); // unbind FBO

	// make sure everything went OK
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		switch (status)
		{
			case GL_FRAMEBUFFER_COMPLETE_EXT:
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
				debug(LOG_ERROR, "Error: FBO missing a required image/buffer attachment!");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
				debug(LOG_ERROR, "Error: FBO has no images/buffers attached!");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				debug(LOG_ERROR, "Error: FBO has mismatched image/buffer dimensions!");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
				debug(LOG_ERROR, "Error: FBO colorbuffer attachments have different types!");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
				debug(LOG_ERROR, "Error: FBO trying to draw to non-attached color buffer!");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
				debug(LOG_ERROR, "Error: FBO trying to read from a non-attached color buffer!");
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				debug(LOG_ERROR, "Error: FBO format is not supported by current graphics card/driver!");
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION_EXT :
				debug(LOG_ERROR, "Error: FBO Non-framebuffer passed to glCheckFramebufferStatusEXT()!");
				break;
			default:
				debug(LOG_ERROR, "*UNKNOWN FBO ERROR* reported from glCheckFramebufferStatusEXT() for %x!", (unsigned int)status);
				break;
		}
		FBOinit = false;	//we have a error with the FBO setup
		return false;
	}
	else
	{
		FBOinit = true;		//everything is OK with FBO setup.
	}

	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0); // unbind it for now.

	bFboProblem |= glErrors(); // don't use FBOs if something here caused an error
	return true;
}

void Delete_FBO(void)
{
	if(FBOinit)
	{
		glErrors();
		glDeleteFramebuffersEXT(1, &fbo);
		glDeleteRenderbuffersEXT(1, &FBOdepthbuffer);
		glDeleteTextures(1,&FBOtexture);
		bFboProblem |= glErrors();
		fbo = FBOdepthbuffer = FBOtexture = FBOinit = 0;	//reset everything.
	}
}
