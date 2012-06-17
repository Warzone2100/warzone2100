/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * screen.cpp
 *
 * Basic double buffered display using OpenGL.
 *
 */


#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include <physfs.h>
#include <png.h>
#include "lib/ivis_opengl/png_util.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/framework/frameint.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieclip.h"

#include "screen.h"
#include "src/console.h"
#include "src/levels.h"
#include <vector>
#include <algorithm>

/* global used to indicate preferred internal OpenGL format */
int wz_texture_compression = 0;

bool opengl_fallback_mode = false;

static bool		bBackDrop = false;
static char		screendump_filename[PATH_MAX];
static bool		screendump_required = false;
static GLuint		backDropTexture = 0;

static int preview_width = 0, preview_height = 0;
static Vector2i player_pos[MAX_PLAYERS];
static bool mappreview = false;
static char mapname[256];
OPENGL_DATA opengl;
extern bool writeGameInfo(const char *pFileName); // Used to help debug issues when we have fatal errors.

/* Initialise the double buffered display */
bool screenInitialise()
{
	GLint glMaxTUs;
	GLenum err;

	glErrors();

	err = glewInit();
	if (GLEW_OK != err)
	{
		debug(LOG_FATAL, "Error: %s", glewGetErrorString(err));
		exit(1);
	}

	/* Dump general information about OpenGL implementation to the console and the dump file */
	ssprintf(opengl.vendor, "OpenGL Vendor: %s", glGetString(GL_VENDOR));
	addDumpInfo(opengl.vendor);
	debug(LOG_3D, "%s", opengl.vendor);
	ssprintf(opengl.renderer, "OpenGL Renderer: %s", glGetString(GL_RENDERER));
	addDumpInfo(opengl.renderer);
	debug(LOG_3D, "%s", opengl.renderer);
	ssprintf(opengl.version, "OpenGL Version: %s", glGetString(GL_VERSION));
	addDumpInfo(opengl.version);
	debug(LOG_3D, "%s", opengl.version);
	ssprintf(opengl.GLEWversion, "GLEW Version: %s", glewGetString(GLEW_VERSION));
	addDumpInfo(opengl.GLEWversion);
	debug(LOG_3D, "%s", opengl.GLEWversion);

	GLubyte const *extensionsBegin = glGetString(GL_EXTENSIONS);
	GLubyte const *extensionsEnd = extensionsBegin + strlen((char const *)extensionsBegin);
	std::vector<std::string> glExtensions;
	for (GLubyte const *i = extensionsBegin; i < extensionsEnd; )
	{
		GLubyte const *j = std::find(i, extensionsEnd, ' ');
		glExtensions.push_back(std::string(i, j));
		i = j + 1;
	}

	/* Dump extended information about OpenGL implementation to the console */

	std::string line;
	for (unsigned n = 0; n < glExtensions.size(); ++n)
	{
		std::string word = " ";
		word += glExtensions[n];
		if (n + 1 != glExtensions.size())
		{
			word += ',';
		}
		if (line.size() + word.size() > 160)
		{
			debug(LOG_3D, "OpenGL Extensions:%s", line.c_str());
			line.clear();
		}
		line += word;
	}
	debug(LOG_3D, "OpenGL Extensions:%s", line.c_str());
	debug(LOG_3D, "Notable OpenGL features:");
	debug(LOG_3D, "  * OpenGL 1.2 %s supported!", GLEW_VERSION_1_2 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.3 %s supported!", GLEW_VERSION_1_3 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.4 %s supported!", GLEW_VERSION_1_4 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 1.5 %s supported!", GLEW_VERSION_1_5 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.0 %s supported!", GLEW_VERSION_2_0 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 2.1 %s supported!", GLEW_VERSION_2_1 ? "is" : "is NOT");
	debug(LOG_3D, "  * OpenGL 3.0 %s supported!", GLEW_VERSION_3_0 ? "is" : "is NOT");
	debug(LOG_3D, "  * Texture compression %s supported.", GLEW_ARB_texture_compression ? "is" : "is NOT");
	debug(LOG_3D, "  * Two side stencil %s supported.", GLEW_EXT_stencil_two_side ? "is" : "is NOT");
	debug(LOG_3D, "  * ATI separate stencil is%s supported.", GLEW_ATI_separate_stencil ? "" : " NOT");
	debug(LOG_3D, "  * Stencil wrap %s supported.", GLEW_EXT_stencil_wrap ? "is" : "is NOT");
	debug(LOG_3D, "  * Anisotropic filtering %s supported.", GLEW_EXT_texture_filter_anisotropic ? "is" : "is NOT");
	debug(LOG_3D, "  * Rectangular texture %s supported.", GLEW_ARB_texture_rectangle ? "is" : "is NOT");
	debug(LOG_3D, "  * FrameBuffer Object (FBO) %s supported.", GLEW_EXT_framebuffer_object ? "is" : "is NOT");
	debug(LOG_3D, "  * ARB Vertex Buffer Object (VBO) %s supported.", GLEW_ARB_vertex_buffer_object ? "is" : "is NOT");
	debug(LOG_3D, "  * NPOT %s supported.", GLEW_ARB_texture_non_power_of_two ? "is" : "is NOT");
	debug(LOG_3D, "  * texture cube_map %s supported.", GLEW_ARB_texture_cube_map ? "is" : "is NOT");
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &glMaxTUs);
	debug(LOG_3D, "  * Total number of Texture Units (TUs) supported is %d.", (int) glMaxTUs);

	screenWidth = MAX(screenWidth, 640);
	screenHeight = MAX(screenHeight, 480);

	std::pair<int, int> glslVersion(0, 0);
	if (GLEW_ARB_shading_language_100 && GLEW_ARB_shader_objects)
	{
		sscanf((char const *)glGetString(GL_SHADING_LANGUAGE_VERSION), "%d.%d", &glslVersion.first, &glslVersion.second);

		/* Dump information about OpenGL 2.0+ implementation to the console and the dump file */
		GLint glMaxTIUs, glMaxTCs, glMaxTIUAs, glmaxSamples, glmaxSamplesbuf;

		debug(LOG_3D, "  * OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		ssprintf(opengl.GLSLversion, "OpenGL GLSL Version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		addDumpInfo(opengl.GLSLversion);

		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &glMaxTIUs);
		debug(LOG_3D, "  * Total number of Texture Image Units (TIUs) supported is %d.", (int) glMaxTIUs);
		glGetIntegerv(GL_MAX_TEXTURE_COORDS, &glMaxTCs);
		debug(LOG_3D, "  * Total number of Texture Coords (TCs) supported is %d.", (int) glMaxTCs);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB,&glMaxTIUAs);
		debug(LOG_3D, "  * Total number of Texture Image Units ARB(TIUAs) supported is %d.", (int) glMaxTIUAs);
		glGetIntegerv(GL_SAMPLE_BUFFERS, &glmaxSamplesbuf);
		debug(LOG_3D, "  * (current) Max Sample buffer is %d.", (int) glmaxSamplesbuf);
		glGetIntegerv(GL_SAMPLES, &glmaxSamples);
		debug(LOG_3D, "  * (current) Max Sample level is %d.", (int) glmaxSamples);
	}

	bool canRunAtAll = GLEW_VERSION_1_2 && GLEW_ARB_vertex_buffer_object && GLEW_ARB_texture_env_crossbar;
	bool canRunShaders = canRunAtAll && glslVersion >= std::make_pair(1, 20);  // glGetString(GL_SHADING_LANGUAGE_VERSION) >= "1.20"

	if (canRunShaders && !opengl_fallback_mode)
	{
		screen_EnableMissingFunctions();  // We need to do this before pie_LoadShaders(), but the effect of this call will be undone later by iV_TextInit(), so we will need to call it again.
		if (pie_LoadShaders())
		{
			pie_SetShaderAvailability(true);
		}
	}
	else if (canRunAtAll)
	{
		// corner cases: vbo(core 1.5 or ARB ext), texture crossbar (core 1.4 or ARB ext)
		debug(LOG_POPUP, _("OpenGL GLSL shader version 1.20 is not supported by your system. Some things may look wrong. Please upgrade your graphics driver/hardware, if possible."));
	}
	else
	{
		// We write this file in hopes that people will upload the information in it to us.
		writeGameInfo("WZdebuginfo.txt");
		debug(LOG_FATAL, _("OpenGL 1.2 + VBO + TEC is not supported by your system. The game requires this. Please upgrade your graphics drivers/hardware, if possible."));
		exit(1);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glErrors();
	return true;
}

void screenShutDown(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

// Make OpenGL's VBO functions available under the core names for drivers that support OpenGL 1.4 only but have the VBO extension
void screen_EnableMissingFunctions()
{
	if (!GLEW_VERSION_1_3 && GLEW_ARB_multitexture)
	{
		debug(LOG_WARNING, "Pre-OpenGL 1.3: Fixing ARB_multitexture extension function names.");

		__glewActiveTexture = __glewActiveTextureARB;
		__glewMultiTexCoord2fv = __glewMultiTexCoord2fvARB;
	}

	if (!GLEW_VERSION_1_5 && GLEW_ARB_vertex_buffer_object)
	{
		debug(LOG_WARNING, "Pre-OpenGL 1.5: Fixing ARB_vertex_buffer_object extension function names.");

		__glewBindBuffer = __glewBindBufferARB;
		__glewBufferData = __glewBufferDataARB;
		__glewBufferSubData = __glewBufferSubDataARB;
		__glewDeleteBuffers = __glewDeleteBuffersARB;
		__glewGenBuffers = __glewGenBuffersARB;
		__glewGetBufferParameteriv = __glewGetBufferParameterivARB;
		__glewGetBufferPointerv = __glewGetBufferPointervARB;
		__glewGetBufferSubData = __glewGetBufferSubDataARB;
		__glewIsBuffer = __glewIsBufferARB;
		__glewMapBuffer = __glewMapBufferARB;
		__glewUnmapBuffer = __glewUnmapBufferARB;
	}

	if (!GLEW_VERSION_2_0 && GLEW_ARB_shader_objects)
	{
		debug(LOG_WARNING, "Pre-OpenGL 2.0: Fixing ARB_shader_objects extension function names.");

		__glewGetUniformLocation = __glewGetUniformLocationARB;
		__glewAttachShader = __glewAttachObjectARB;
		__glewCompileShader = __glewCompileShaderARB;
		__glewCreateProgram = __glewCreateProgramObjectARB;
		__glewCreateShader = __glewCreateShaderObjectARB;
		__glewGetProgramInfoLog = __glewGetInfoLogARB;
		__glewGetShaderInfoLog = __glewGetInfoLogARB;  // Same as previous.
		__glewGetProgramiv = __glewGetObjectParameterivARB;
		__glewUseProgram = __glewUseProgramObjectARB;
		__glewGetShaderiv = __glewGetObjectParameterivARB;
		__glewLinkProgram = __glewLinkProgramARB;
		__glewShaderSource = __glewShaderSourceARB;
		__glewUniform1f = __glewUniform1fARB;
		__glewUniform1i = __glewUniform1iARB;
		__glewUniform4fv = __glewUniform4fvARB;
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
	pie_SetTexturePage(TEXPAGE_EXTERN);

	if( strcmp(extension,".png") == 0 )
	{
		if (iV_loadImage_PNG( filename, &image ) )
		{
			if (backDropTexture == 0)
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

bool screen_GetBackDrop(void)
{
	return bBackDrop;
}

//******************************************************************
//slight hack to display maps (or whatever) in background.
//bitmap MUST be (BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT) for now.
void screen_Upload(const char *newBackDropBmp, bool preview)
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

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_OFF);

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
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
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

bool screen_getMapPreview(void)
{
	return mappreview;
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
		image.bmp = (unsigned char *)malloc(channelsPerPixel * image.width * image.height);
		if (image.bmp == NULL)
		{
			image.width = 0; image.height = 0;
			debug(LOG_ERROR, "Couldn't allocate memory");
			return;
		}
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
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
