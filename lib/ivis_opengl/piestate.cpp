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
/** \file
 *  Renderer setup and state control routines for 3D rendering.
 */
#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"

#include <physfs.h>

#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "screen.h"

/*
 *	Global Variables
 */

static bool shadersAvailable = false;
static GLuint shaderProgram[SHADER_MAX];
static GLfloat shaderStretch = 0;
static GLint locTeam, locStretch, locTCMask, locFog, locNormalMap, locEcm, locTime;
static SHADER_MODE currentShaderMode = SHADER_NONE;
unsigned int pieStateCount = 0; // Used in pie_GetResetCounts
static RENDER_STATE rendStates;
static GLint ecmState = 0;
static GLfloat timeState = 0.0f;

void rendStatesRendModeHack()
{
	rendStates.rendMode = REND_ALPHA;
}

/*
 *	Source
 */

void pie_SetDefaultStates(void)//Sets all states
{
	PIELIGHT black;

	//fog off
	rendStates.fogEnabled = false;// enable fog before renderer
	rendStates.fog = false;//to force reset to false
	pie_SetFogStatus(false);
	black.rgba = 0;
	black.byte.a = 255;
	pie_SetFogColour(black);

	//depth Buffer on
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);

	rendStates.rendMode = REND_ALPHA;	// to force reset to REND_OPAQUE
	pie_SetRendMode(REND_OPAQUE);

	//chroma keying on black
	rendStates.keyingOn = false;//to force reset to true
	pie_SetAlphaTest(true);
}

//***************************************************************************
//
// pie_EnableFog(bool val)
//
// Global enable/disable fog to allow fog to be turned of ingame
//
//***************************************************************************
void pie_EnableFog(bool val)
{
	if (rendStates.fogEnabled != val)
	{
		debug(LOG_FOG, "pie_EnableFog: Setting fog to %s", val ? "ON" : "OFF");
		rendStates.fogEnabled = val;
		if (val)
		{
			pie_SetFogColour(WZCOL_FOG);
		}
		else
		{
			pie_SetFogColour(WZCOL_BLACK); // clear background to black
		}
	}
}

bool pie_GetFogEnabled(void)
{
	return rendStates.fogEnabled;
}

//***************************************************************************
//
// pie_SetFogStatus(bool val)
//
// Toggle fog on and off for rendering objects inside or outside the 3D world
//
//***************************************************************************
bool pie_GetFogStatus(void)
{
	return rendStates.fog;
}

void pie_SetFogColour(PIELIGHT colour)
{
	rendStates.fogColour = colour;
}

PIELIGHT pie_GetFogColour(void)
{
	return rendStates.fogColour;
}

bool pie_GetShaderAvailability(void)
{
	return shadersAvailable;
}

void pie_SetShaderAvailability(bool availability)
{
	shadersAvailable = availability;
}

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
	buffer = (char *)malloc(filesize + 1);
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
	GLint infologLen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
		debug(part, "Shader info log: %s", infoLog);
		free(infoLog);
	}
}

// Retrieve shader linkage errors
static void printProgramInfoLog(code_part part, GLuint program)
{
	GLint infologLen = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
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
	bool result;

	// Try to load some shaders
	shaderProgram[SHADER_NONE] = 0;

	// TCMask shader for map-placed models with advanced lighting
	debug(LOG_3D, "Loading shader: SHADER_COMPONENT");
	result = loadShaders(&program, "", "shaders/tcmask.vert", "shaders/tcmask.frag");
	ASSERT_OR_RETURN(false, result, "Failed to load component shader");
	shaderProgram[SHADER_COMPONENT] = program;

	// TCMask shader for buttons with flat lighting
	debug(LOG_3D, "Loading shader: SHADER_BUTTON");
	result = loadShaders(&program, "", "shaders/button.vert", "shaders/button.frag");
	ASSERT_OR_RETURN(false, result, "Failed to load button shader");
	shaderProgram[SHADER_BUTTON] = program;

	currentShaderMode = SHADER_NONE;

	return true;
}

void pie_DeactivateShader(void)
{
	currentShaderMode = SHADER_NONE;
	glUseProgram(0);
}

void pie_SetShaderTime(uint32_t shaderTime)
{
	uint32_t base = shaderTime % 1000;
	if (base > 500)
	{
		base = 1000 - base;	// cycle
	}
	timeState = (GLfloat)base / 1000.0f;
}

void pie_SetShaderEcmEffect(bool value)
{
	ecmState = (int)value;
}

void pie_SetShaderStretchDepth(float stretch)
{
	shaderStretch = stretch;
}

void pie_ActivateFallback(SHADER_MODE, iIMDShape* shape, PIELIGHT teamcolour, PIELIGHT colour)
{
	if (shape->tcmaskpage == iV_TEX_INVALID)
	{
		return;
	}

	//Set the environment colour with tcmask
	GLfloat tc_env_colour[4];
	pal_PIELIGHTtoRGBA4f(&tc_env_colour[0], teamcolour);

	// TU0
	glActiveTexture(GL_TEXTURE0);
	pie_SetTexturePage(shape->texpage);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,	GL_COMBINE);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, tc_env_colour);

	// TU0 RGB
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,		GL_ADD_SIGNED);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB,		GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB,		GL_SRC_COLOR);

	// TU0 Alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,	GL_SRC_ALPHA);

	// TU1
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[shape->tcmaskpage].id);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,	GL_COMBINE);

	// TU1 RGB
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,		GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB,		GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB,		GL_SRC_ALPHA);

	// TU1 Alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,	GL_SRC_ALPHA);

	glEnable(GL_BLEND);
	glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
	glBlendColor(colour.byte.r / 255.0, colour.byte.g / 255.0,
				colour.byte.b / 255.0, colour.byte.a / 255.0);

	glActiveTexture(GL_TEXTURE0);
}

void pie_DeactivateFallback()
{
	glDisable(GL_BLEND);
	rendStates.rendMode = REND_OPAQUE;

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void pie_ActivateShader(SHADER_MODE shaderMode, iIMDShape* shape, PIELIGHT teamcolour, PIELIGHT colour)
{
	int maskpage = shape->tcmaskpage;
	int normalpage = shape->normalpage;
	GLfloat colour4f[4];

	if (shaderMode != currentShaderMode)
	{
		GLint locTex0, locTex1, locTex2;

		glUseProgram(shaderProgram[shaderMode]);
		locTex0 = glGetUniformLocation(shaderProgram[shaderMode], "Texture0");
		locTex1 = glGetUniformLocation(shaderProgram[shaderMode], "Texture1");
		locTex2 = glGetUniformLocation(shaderProgram[shaderMode], "Texture2");
		locTeam = glGetUniformLocation(shaderProgram[shaderMode], "teamcolour");
		locStretch = glGetUniformLocation(shaderProgram[shaderMode], "stretch");
		locTCMask = glGetUniformLocation(shaderProgram[shaderMode], "tcmask");
		locNormalMap = glGetUniformLocation(shaderProgram[shaderMode], "normalmap");
		locFog = glGetUniformLocation(shaderProgram[shaderMode], "fogEnabled");
		locEcm = glGetUniformLocation(shaderProgram[shaderMode], "ecmEffect");
		locTime = glGetUniformLocation(shaderProgram[shaderMode], "graphicsCycle");

		// These never change
		glUniform1i(locTex0, 0);
		glUniform1i(locTex1, 1);
		glUniform1i(locTex2, 2);

		currentShaderMode  = shaderMode;
	}

	glColor4ubv(colour.vector);
	pie_SetTexturePage(shape->texpage);

	pal_PIELIGHTtoRGBA4f(&colour4f[0], teamcolour);
	glUniform4fv(locTeam, 1, &colour4f[0]);
	glUniform1f(locStretch, shaderStretch);
	glUniform1i(locTCMask, maskpage != iV_TEX_INVALID);
	glUniform1i(locNormalMap, normalpage != iV_TEX_INVALID);
	glUniform1i(locFog, rendStates.fog);
	glUniform1f(locTime, timeState);
	glUniform1i(locEcm, ecmState);

	if (maskpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[maskpage].id);
	}
	if (normalpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[normalpage].id);
	}
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
// pie_SetFogStatus(bool val)
//
// Toggle fog on and off for rendering objects inside or outside the 3D world
//
void pie_SetFogStatus(bool val)
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

void pie_SetAlphaTest(bool keyingOn)
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

void pie_SetRendMode(REND_MODE rendMode)
{
	if (rendMode != rendStates.rendMode)
	{
		rendStates.rendMode = rendMode;
		switch (rendMode)
		{
			case REND_OPAQUE:
				glDisable(GL_BLEND);
				break;

			case REND_ALPHA:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;

			case REND_ADDITIVE:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;

			case REND_MULTIPLICATIVE:
				glEnable(GL_BLEND);
				glBlendFunc(GL_ZERO, GL_SRC_COLOR);
				break;

			case REND_PREMULTIPLIED:
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				break;

			default:
				ASSERT(false, "Bad render state");
				break;
		}
	}
	return;
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
