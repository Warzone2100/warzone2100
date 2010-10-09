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
/** \file
 *  Renderer setup and state control routines for 3D rendering.
 */

#include <GLee.h>
#include "lib/framework/frame.h"

#include <SDL.h>
#include <SDL_mouse.h>
#include <SDL_opengl.h>
#include <physfs.h>

#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/rendmode.h"
#include "screen.h"

/*
 *	Global Variables
 */

// Variables for the coloured mouse cursor
static CURSOR MouseCursor = 0;
static bool ColouredMouse = false;
static IMAGEFILE* MouseCursors = NULL;
static uint16_t MouseCursorIDs[CURSOR_MAX];
static bool MouseVisible = true;
static GLuint shaderProgram[SHADER_MAX];
static bool shadersAvailable = false;		// Can we use shaders?
static bool shadersActivate = true;			// If we can, should we use them?

/*
 *	Source
 */

// Read shader into text buffer
static char *readShaderBuf(const char *name)
{
	PHYSFS_file	*fp;
	int	filesize;
	char *buffer;

	fp = PHYSFS_openRead(name);
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name), name);
	ASSERT_OR_RETURN(0, fp != NULL, "Could not open %s", name);
	filesize = PHYSFS_fileLength(fp);
	buffer = malloc(filesize + 1);
	if (buffer)
	{
		PHYSFS_read(fp, buffer, 1, filesize);
		buffer[filesize] = '\0';
	}
	PHYSFS_close(fp);

	return buffer;
}

// Retrieve shader compilation errors
static void printShaderInfoLog(code_part part, GLuint shader)
{
	int infologLen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		int charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
		debug(part, "Shader info log: %s", infoLog);
		free(infoLog);
	}
}

// Retrieve shader linkage errors
static void printProgramInfoLog(code_part part, GLuint program)
{
	int infologLen = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		int charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
		debug(part, "Program info log: %s", infoLog);
		free(infoLog);
	}
}

// Read/compile/link shaders
static bool loadShaders(GLuint *program, const char *definitions,
						const char *vertexPath, const char *fragmentPath)
{
	GLint status;
	bool success = true; // Assume overall success
	char *buffer[2];

	*program = glCreateProgram();
	ASSERT_OR_RETURN(false, definitions != NULL, "Null in preprocessor definitions!");
	ASSERT_OR_RETURN(false, *program, "Could not create shader program!");

	*buffer = (char *)definitions;

	if (vertexPath)
	{
		success = false; // Assume failure before reading shader file

		if ((*(buffer + 1) = readShaderBuf(vertexPath)))
		{
			GLuint shader = glCreateShader(GL_VERTEX_SHADER);

			glShaderSource(shader, 2, (const char **)buffer, NULL);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Vertex shader compilation has failed [%s]", vertexPath);
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(*program, shader);
				success = true;
			}

			free(*(buffer + 1));
		}
	}

	if (success && fragmentPath)
	{
		success = false; // Assume failure before reading shader file

		if ((*(buffer + 1) = readShaderBuf(fragmentPath)))
		{
			GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

			glShaderSource(shader, 2, (const char **)buffer, NULL);
			glCompileShader(shader);
			
			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Fragment shader compilation has failed [%s]", fragmentPath);
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(*program, shader);
				success = true;
			}

			free(*(buffer + 1));
		}
	}

	if (success)
	{
		glLinkProgram(*program);

		// Check for linkage errors
		glGetProgramiv(*program, GL_LINK_STATUS, &status);
		if (!status)
		{
			debug(LOG_ERROR, "Shader program linkage has failed [%s, %s]", vertexPath, fragmentPath);
			printProgramInfoLog(LOG_ERROR, *program);
			success = false;
		}
		else
		{
			printProgramInfoLog(LOG_3D, *program);
		}
	}

	return success;	
}

// Run from screen.c on init. FIXME: do some kind of FreeShaders on failure.
bool pie_LoadShaders()
{
	GLuint program;

	// Reset shaders status
	shadersAvailable = false;

	// Try and load some shaders
	shaderProgram[SHADER_NONE]		= 0;

	// TCMask shader
	debug(LOG_3D, "Loading shader: SHADER_TCMASK");
	if (!loadShaders(&program, "",
					"shaders/tcmask.vert", "shaders/tcmask.frag"))
		return false;
	shaderProgram[SHADER_TCMASK] = program;

	debug(LOG_3D, "Loading shader: SHADER_TCMASK_FOGGED");
	if (!loadShaders(&program, "#define FOG_ENABLED\n",
					"shaders/tcmask.vert", "shaders/tcmask.frag"))
		return false;
	shaderProgram[SHADER_TCMASK_FOGGED] = program;

	// Good to go
	shadersAvailable = true;
	return true;
}

static inline GLuint pie_SetShader(SHADER_MODE shaderMode)
{
	if (!shadersAvailable || shaderMode >= SHADER_MAX)
		return shaderProgram[SHADER_NONE];

	glUseProgram(shaderProgram[shaderMode]);
	return shaderProgram[shaderMode];
}

void pie_DeactivateShader(void)
{
	pie_SetShader(SHADER_NONE);
}

bool pie_GetShadersStatus(void)
{
	return shadersAvailable && shadersActivate;
}

void pie_SetShadersStatus(bool status)
{
	shadersActivate = status;
}

void pie_ActivateShader_TCMask(PIELIGHT teamcolour, SDWORD maskpage)
{
	GLint loc;
	GLuint shaderProgram;
	GLfloat colour4f[4];

	if (!shadersAvailable)
		return;

	// Check if fog is enabled
	if (rendStates.fog)
	{
		shaderProgram = pie_SetShader(SHADER_TCMASK_FOGGED);
	}
	else
	{
		shaderProgram = pie_SetShader(SHADER_TCMASK);
	}

	loc = glGetUniformLocation(shaderProgram, "Texture0"); 
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(shaderProgram, "Texture1"); 
	glUniform1i(loc, 1);

	loc = glGetUniformLocation(shaderProgram, "teamcolour"); 
	pal_PIELIGHTtoRGBA4f(&colour4f[0], teamcolour);
	glUniform4fv(loc, 1, &colour4f[0]);

	glActiveTexture(GL_TEXTURE1);
	pie_SetTexturePage(maskpage);
	glActiveTexture(GL_TEXTURE0);

#ifdef _DEBUG
	glErrors();
#endif
}

void pie_SetDepthBufferStatus(DEPTH_MODE depthMode)
{
	switch(depthMode)
	{
		case DEPTH_CMP_LEQ_WRT_ON:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_TRUE);
			break;

		case DEPTH_CMP_ALWAYS_WRT_ON:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			break;

		case DEPTH_CMP_LEQ_WRT_OFF:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);
			break;

		case DEPTH_CMP_ALWAYS_WRT_OFF:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			break;
	}
}

/// Set the depth (z) offset
/// Negative values are closer to the screen
void pie_SetDepthOffset(float offset)
{
	if(offset == 0.0f)
	{
		glDisable (GL_POLYGON_OFFSET_FILL);
	}
	else
	{
		glPolygonOffset(offset, offset);
		glEnable (GL_POLYGON_OFFSET_FILL);
	}
}

/// Set the OpenGL fog start and end
void pie_UpdateFogDistance(float begin, float end)
{
	glFogf(GL_FOG_START, begin);
	glFogf(GL_FOG_END, end);
}

//
// pie_SetFogStatus(BOOL val)
//
// Toggle fog on and off for rendering objects inside or outside the 3D world
//

void pie_SetFogStatus(BOOL val)
{
	float fog_colour[4];

	if (rendStates.fogEnabled)
	{
		//fog enabled so toggle if required
		if (rendStates.fog != val)
		{
			rendStates.fog = val;
			if (rendStates.fog) {
				PIELIGHT fog = pie_GetFogColour();

				fog_colour[0] = fog.byte.r/255.0f;
				fog_colour[1] = fog.byte.g/255.0f;
				fog_colour[2] = fog.byte.b/255.0f;
				fog_colour[3] = fog.byte.a/255.0f;

				glFogi(GL_FOG_MODE, GL_LINEAR);
				glFogfv(GL_FOG_COLOR, fog_colour);
				glFogf(GL_FOG_DENSITY, 0.35f);
				glHint(GL_FOG_HINT, GL_DONT_CARE);
				glEnable(GL_FOG);
				glClearColor(fog_colour[0], fog_colour[1], fog_colour[2], fog_colour[3]);
			} else {
				glDisable(GL_FOG);
				glClearColor(0, 0, 0, 0);
			}
		}
	}
	else
	{
		//fog disabled so turn it off if not off already
		if (rendStates.fog != false)
		{
			rendStates.fog = false;
		}
	}
}

/** Selects a texture page and binds it for the current texture unit
 *  \param num a number indicating the texture page to bind. If this is a
 *         negative value (doesn't matter what value) it will disable texturing.
 */
void pie_SetTexturePage(SDWORD num)
{
	// Only bind textures when they're not bound already
	if (num != rendStates.texPage)
	{
		switch (num)
		{
			case TEXPAGE_NONE:
				glDisable(GL_TEXTURE_2D);
				break;
			case TEXPAGE_EXTERN:
				// GLC will set the texture, we just need to enable texturing
				glEnable(GL_TEXTURE_2D);
				break;
			default:
				if (rendStates.texPage == TEXPAGE_NONE)
				{
					glEnable(GL_TEXTURE_2D);
				}
				ASSERT_OR_RETURN(, num < iV_TEX_MAX, "Index out of bounds: %d", num);
				glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[num].id);
		}
		rendStates.texPage = num;
	}
}

void pie_SetAlphaTest(BOOL keyingOn)
{
	if (keyingOn != rendStates.keyingOn)
	{
		rendStates.keyingOn = keyingOn;
		pieStateCount++;

		if (keyingOn == true) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.1f);
		} else {
			glDisable(GL_ALPHA_TEST);
		}
	}
}

void pie_SetTranslucencyMode(TRANSLUCENCY_MODE transMode)
{
	if (transMode != rendStates.transMode) {
		rendStates.transMode = transMode;
		switch (transMode) {
			case TRANS_ALPHA:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case TRANS_ADDITIVE:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
			case TRANS_MULTIPLICATIVE:
				glEnable(GL_BLEND);
				glBlendFunc(GL_ZERO, GL_SRC_COLOR);
				break;
			case TRANS_DECAL:
				glDisable(GL_BLEND);
				break;
			case TRANS_FILTER:
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				break;
			default:
				break;
		}
	}
}

void pie_InitColourMouse(IMAGEFILE* img, const uint16_t cursorIDs[CURSOR_MAX])
{
	MouseCursors = img;
	memcpy(MouseCursorIDs, cursorIDs, sizeof(MouseCursorIDs));
}

/** Selects the given mouse cursor.
 *  \param cursor   mouse cursor to render
 *  \param coloured wether a coloured or black&white cursor should be used
 */
void pie_SetMouse(CURSOR cursor, bool coloured)
{
	ASSERT(cursor < CURSOR_MAX, "Attempting to load non-existent cursor: %u", (unsigned int)cursor);

	MouseCursor = cursor;

	frameSetCursor(MouseCursor);
	ColouredMouse = coloured;
}

/** Draws the current mouse cursor at the given coordinates
 *  \param X,Y mouse coordinates
 */
void pie_DrawMouse(unsigned int X, unsigned int Y)
{
	if (ColouredMouse && MouseVisible)
	{
		ASSERT(MouseCursors != NULL, "Drawing coloured mouse cursor while no coloured mouse cursors have been loaded yet!");

		iV_DrawImage(MouseCursors, MouseCursorIDs[MouseCursor], X, Y);
	}
}

/** Set the visibility of the mouse cursor */
void pie_ShowMouse(bool visible)
{
	MouseVisible = visible;
	if (MouseVisible && !ColouredMouse)
	{
		SDL_ShowCursor(SDL_ENABLE);
	}
	else
	{
		SDL_ShowCursor(SDL_DISABLE);
	}
}

bool _glerrors(const char *function, const char *file, int line)
{
	bool ret = false;
	GLenum err = glGetError();
	while (err != GL_NO_ERROR)
	{
		ret = true;
		debug(LOG_ERROR, "OpenGL error in function %s at %s:%u: %s\n", function, file, line, gluErrorString(err));
		err = glGetError();
	}
	return ret;
}
