/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

struct SHADER_PROGRAM
{
	GLuint program;
	GLint locTeam, locStretch, locTCMask, locFog, locNormalMap, locSpecularMap, locEcm, locTime;
};

static QList<SHADER_PROGRAM> shaderProgram;
static GLfloat shaderStretch = 0;
static int currentShaderMode = SHADER_NONE;
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

static void getLocs(SHADER_PROGRAM *program)
{
	GLint locTex0, locTex1, locTex2, locTex3;

	glUseProgram(program->program);
	locTex0 = glGetUniformLocation(program->program, "Texture0");
	locTex1 = glGetUniformLocation(program->program, "Texture1");
	locTex2 = glGetUniformLocation(program->program, "Texture2");
	locTex3 = glGetUniformLocation(program->program, "Texture3");
	program->locTeam = glGetUniformLocation(program->program, "teamcolour");
	program->locStretch = glGetUniformLocation(program->program, "stretch");
	program->locTCMask = glGetUniformLocation(program->program, "tcmask");
	program->locNormalMap = glGetUniformLocation(program->program, "normalmap");
	program->locSpecularMap = glGetUniformLocation(program->program, "specularmap");
	program->locFog = glGetUniformLocation(program->program, "fogEnabled");
	program->locEcm = glGetUniformLocation(program->program, "ecmEffect");
	program->locTime = glGetUniformLocation(program->program, "graphicsCycle");

	// These never change
	glUniform1i(locTex0, 0);
	glUniform1i(locTex1, 1);
	glUniform1i(locTex2, 2);
	glUniform1i(locTex3, 3);
}

void pie_FreeShaders()
{
	while (shaderProgram.size() > SHADER_MAX)
	{
		SHADER_PROGRAM program = shaderProgram.takeLast();
		glDeleteShader(program.program);
	}
}

// Read/compile/link shaders
GLuint pie_LoadShader(const char *programName, const char *vertexPath, const char *fragmentPath)
{
	SHADER_PROGRAM program;
	GLint status;
	bool success = true; // Assume overall success
	char *buffer[2];

	memset(&program, 0, sizeof(program));

	program.program = glCreateProgram();
	ASSERT_OR_RETURN(false, program.program, "Could not create shader program!");

	*buffer = (char *)"";

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
				glAttachShader(program.program, shader);
				success = true;
			}
			if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
			{
				glObjectLabel(GL_SHADER, shader, -1, vertexPath);
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
				glAttachShader(program.program, shader);
				success = true;
			}
			if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
			{
				glObjectLabel(GL_SHADER, shader, -1, fragmentPath);
			}
			free(*(buffer + 1));
		}
	}

	if (success)
	{
		glLinkProgram(program.program);

		// Check for linkage errors
		glGetProgramiv(program.program, GL_LINK_STATUS, &status);
		if (!status)
		{
			debug(LOG_ERROR, "Shader program linkage has failed [%s, %s]", vertexPath, fragmentPath);
			printProgramInfoLog(LOG_ERROR, program.program);
			success = false;
		}
		else
		{
			printProgramInfoLog(LOG_3D, program.program);
		}
		if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
		{
			glObjectLabel(GL_PROGRAM, program.program, -1, programName);
		}
	}

	getLocs(&program);
	glUseProgram(0);

	shaderProgram.append(program);

	return shaderProgram.size() - 1;
}

// Run from screen.c on init. Do not change the order of loading here! First ones are enumerated.
bool pie_LoadShaders()
{
	SHADER_PROGRAM program;
	int result;

	// Load some basic shaders
	memset(&program, 0, sizeof(program));
	shaderProgram.append(program);

	// TCMask shader for map-placed models with advanced lighting
	debug(LOG_3D, "Loading shader: SHADER_COMPONENT");
	result = pie_LoadShader("Component program", "shaders/tcmask.vert", "shaders/tcmask.frag");
	ASSERT_OR_RETURN(false, result, "Failed to load component shader");

	// TCMask shader for buttons with flat lighting
	debug(LOG_3D, "Loading shader: SHADER_BUTTON");
	result = pie_LoadShader("Button program", "shaders/button.vert", "shaders/button.frag");
	ASSERT_OR_RETURN(false, result, "Failed to load button shader");

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

void pie_ActivateShader(int shaderMode, const iIMDShape* shape, PIELIGHT teamcolour, PIELIGHT colour)
{
	int maskpage = shape->tcmaskpage;
	int normalpage = shape->normalpage;
	int specularpage = shape->specularpage;
	GLfloat colour4f[4];
	SHADER_PROGRAM program = shaderProgram[shaderMode];

	if (shaderMode != currentShaderMode)
	{
		glUseProgram(program.program);

		// These do not change during our drawing pass
		glUniform1i(program.locFog, rendStates.fog);
		glUniform1f(program.locTime, timeState);

		currentShaderMode = shaderMode;
	}

	glColor4ubv(colour.vector);
	pie_SetTexturePage(shape->texpage);

	pal_PIELIGHTtoRGBA4f(&colour4f[0], teamcolour);
	glUniform4fv(program.locTeam, 1, &colour4f[0]);
	glUniform1i(program.locTCMask, maskpage != iV_TEX_INVALID);
	if (program.locStretch >= 0)
	{
		glUniform1f(program.locStretch, shaderStretch);
	}
	if (program.locNormalMap >= 0)
	{
		glUniform1i(program.locNormalMap, normalpage != iV_TEX_INVALID);
	}
	if (program.locSpecularMap >= 0)
	{
		glUniform1i(program.locSpecularMap, specularpage != iV_TEX_INVALID);
	}
	if (program.locEcm >= 0)
	{
		glUniform1i(program.locEcm, ecmState);
	}

	if (maskpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pie_Texture(maskpage));
	}
	if (normalpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, pie_Texture(normalpage));
	}
	if (specularpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, pie_Texture(specularpage));
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
			} else {
				glDisable(GL_FOG);
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
				glBindTexture(GL_TEXTURE_2D, pie_Texture(num));
		}
		rendStates.texPage = num;
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
