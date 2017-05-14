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
#include "pieclip.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#ifndef GLEW_VERSION_4_3
#define GLEW_VERSION_4_3 false
#endif
#ifndef GLEW_KHR_debug
#define GLEW_KHR_debug false
#endif

/*
 *	Global Variables
 */

std::vector<pie_internal::SHADER_PROGRAM> pie_internal::shaderProgram;
static GLfloat shaderStretch = 0;
SHADER_MODE pie_internal::currentShaderMode = SHADER_NONE;
GLuint pie_internal::rectBuffer = 0;
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

void pie_SetDefaultStates()//Sets all states
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

bool pie_GetFogEnabled()
{
	return rendStates.fogEnabled;
}

bool pie_GetFogStatus()
{
	return rendStates.fog;
}

void pie_SetFogColour(PIELIGHT colour)
{
	rendStates.fogColour = colour;
}

PIELIGHT pie_GetFogColour()
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
	ASSERT_OR_RETURN(nullptr, fp != nullptr, "Could not open %s", name);
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

static void getLocs(pie_internal::SHADER_PROGRAM *program)
{
	glUseProgram(program->program);

	// Attributes
	program->locVertex = glGetAttribLocation(program->program, "vertex");
	program->locNormal = glGetAttribLocation(program->program, "vertexNormal");
	program->locTexCoord = glGetAttribLocation(program->program, "vertexTexCoord");
	program->locColor = glGetAttribLocation(program->program, "vertexColor");

	// Uniforms, these never change.
	GLint locTex0 = glGetUniformLocation(program->program, "Texture");
	GLint locTex1 = glGetUniformLocation(program->program, "TextureTcmask");
	GLint locTex2 = glGetUniformLocation(program->program, "TextureNormal");
	GLint locTex3 = glGetUniformLocation(program->program, "TextureSpecular");
	glUniform1i(locTex0, 0);
	glUniform1i(locTex1, 1);
	glUniform1i(locTex2, 2);
	glUniform1i(locTex3, 3);
}

void pie_FreeShaders()
{
	while (pie_internal::shaderProgram.size() > SHADER_MAX)
	{
		glDeleteShader(pie_internal::shaderProgram.back().program);
		pie_internal::shaderProgram.pop_back();
	}
}

// Read/compile/link shaders
SHADER_MODE pie_LoadShader(const char *programName, const char *vertexPath, const char *fragmentPath,
	const std::vector<std::string> &uniformNames)
{
	pie_internal::SHADER_PROGRAM program;
	GLint status;
	bool success = true; // Assume overall success
	char *buffer[2];

	program.program = glCreateProgram();
	glBindAttribLocation(program.program, 0, "vertex");
	glBindAttribLocation(program.program, 1, "vertexTexCoord");
	glBindAttribLocation(program.program, 2, "vertexColor");
	ASSERT_OR_RETURN(SHADER_NONE, program.program, "Could not create shader program!");

	*buffer = (char *)"";

	if (vertexPath)
	{
		success = false; // Assume failure before reading shader file

		if ((*(buffer + 1) = readShaderBuf(vertexPath)))
		{
			GLuint shader = glCreateShader(GL_VERTEX_SHADER);

			glShaderSource(shader, 2, (const char **)buffer, nullptr);
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

			glShaderSource(shader, 2, (const char **)buffer, nullptr);
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
	GLuint p = program.program;
	std::transform(uniformNames.begin(), uniformNames.end(),
		std::back_inserter(program.locations),
		[p](const std::string name) { return glGetUniformLocation(p, name.data()); });

	getLocs(&program);
	glUseProgram(0);

	pie_internal::shaderProgram.push_back(program);

	return SHADER_MODE(pie_internal::shaderProgram.size() - 1);
}

static float fogBegin;
static float fogEnd;

// Run from screen.c on init. Do not change the order of loading here! First ones are enumerated.
bool pie_LoadShaders()
{
	pie_internal::SHADER_PROGRAM program;
	int result;

	// Load some basic shaders
	memset(&program, 0, sizeof(program));
	pie_internal::shaderProgram.push_back(program);
	int shaderEnum = 0;

	// TCMask shader for map-placed models with advanced lighting
	debug(LOG_3D, "Loading shader: SHADER_COMPONENT");
	result = pie_LoadShader("Component program", "shaders/tcmask.vert", "shaders/tcmask.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_COMPONENT, "Failed to load component shader");

	// TCMask shader for buttons with flat lighting
	debug(LOG_3D, "Loading shader: SHADER_BUTTON");
	result = pie_LoadShader("Button program", "shaders/button.vert", "shaders/button.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_BUTTON, "Failed to load button shader");

	// Plain shader for no lighting
	debug(LOG_3D, "Loading shader: SHADER_NOLIGHT");
	result = pie_LoadShader("Plain program", "shaders/nolight.vert", "shaders/nolight.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_NOLIGHT, "Failed to load no-lighting shader");

	debug(LOG_3D, "Loading shader: SHADER_TERRAIN");
	result = pie_LoadShader("terrain program", "shaders/terrain_water.vert", "shaders/terrain.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex", "lightmap_tex", "textureMatrix1", "textureMatrix2",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TERRAIN, "Failed to load terrain shader");

	debug(LOG_3D, "Loading shader: SHADER_TERRAIN_DEPTH");
	result = pie_LoadShader("terrain_depth program", "shaders/terrain_water.vert", "shaders/terraindepth.frag",
	{ "ModelViewProjectionMatrix", "paramx2", "paramy2", "lightmap_tex", "textureMatrix1", "textureMatrix2" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TERRAIN_DEPTH, "Failed to load terrain_depth shader");

	debug(LOG_3D, "Loading shader: SHADER_DECALS");
	result = pie_LoadShader("decals program", "shaders/decals.vert", "shaders/decals.frag",
		{ "ModelViewProjectionMatrix", "paramxlight", "paramylight", "tex", "lightmap_tex", "lightTextureMatrix",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_DECALS, "Failed to load decals shader");

	debug(LOG_3D, "Loading shader: SHADER_WATER");
	result = pie_LoadShader("water program", "shaders/terrain_water.vert", "shaders/water.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex1", "tex2", "textureMatrix1", "textureMatrix2",
		"fogEnabled", "fogEnd", "fogStart" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_WATER, "Failed to load water shader");

	// Rectangular shader
	debug(LOG_3D, "Loading shader: SHADER_RECT");
	result = pie_LoadShader("Rect program", "shaders/rect.vert", "shaders/rect.frag",
		{ "transformationMatrix", "color" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_RECT, "Failed to load rect shader");

	// Textured rectangular shader
	debug(LOG_3D, "Loading shader: SHADER_TEXRECT");
	result = pie_LoadShader("Textured rect program", "shaders/rect.vert", "shaders/texturedrect.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "texture" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TEXRECT, "Failed to load textured rect shader");

	debug(LOG_3D, "Loading shader: SHADER_GFX_COLOUR");
	result = pie_LoadShader("gfx_color program", "shaders/gfx.vert", "shaders/gfx.frag",
		{ "posMatrix" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_GFX_COLOUR, "Failed to load textured gfx shader");

	debug(LOG_3D, "Loading shader: SHADER_GFX_TEXT");
	result = pie_LoadShader("gfx_text program", "shaders/gfx.vert", "shaders/texturedrect.frag",
		{ "posMatrix", "color", "texture" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_GFX_TEXT, "Failed to load textured gfx shader");

	debug(LOG_3D, "Loading shader: SHADER_GENERIC_COLOR");
	result = pie_LoadShader("generic color program", "shaders/generic.vert", "shaders/rect.frag", { "ModelViewProjectionMatrix", "color" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_GENERIC_COLOR, "Failed to load generic color shader");

	debug(LOG_3D, "Loading shader: SHADER_LINE");
	result = pie_LoadShader("line program", "shaders/line.vert", "shaders/rect.frag", { "from", "to", "color", "ModelViewProjectionMatrix" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_LINE, "Failed to load line shader");

	// Text shader
	debug(LOG_3D, "Loading shader: SHADER_TEXT");
	result = pie_LoadShader("Text program", "shaders/rect.vert", "shaders/text.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "texture" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TEXT, "Failed to load text shader");

	pie_internal::currentShaderMode = SHADER_NONE;

	GLbyte rect[] {
		0, 1, 0, 1,
		0, 0, 0, 1,
		1, 1, 0, 1,
		1, 0, 0, 1
	};
	glGenBuffers(1, &pie_internal::rectBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pie_internal::rectBuffer);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLbyte), rect, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

void pie_DeactivateShader()
{
	pie_internal::currentShaderMode = SHADER_NONE;
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

float pie_GetShaderStretchDepth()
{
	return shaderStretch;
}

pie_internal::SHADER_PROGRAM &pie_ActivateShaderDeprecated(SHADER_MODE shaderMode, const iIMDShape *shape, PIELIGHT teamcolour, PIELIGHT colour, const glm::mat4 &ModelView, const glm::mat4 &Proj,
	const glm::vec4 &sunPos, const glm::vec4 &sceneColor, const glm::vec4 &ambient, const glm::vec4 &diffuse, const glm::vec4 &specular)
{
	int maskpage = shape->tcmaskpage;
	int normalpage = shape->normalpage;
	int specularpage = shape->specularpage;
	pie_internal::SHADER_PROGRAM &program = pie_internal::shaderProgram[shaderMode];

	if (shaderMode != pie_internal::currentShaderMode)
	{
		glUseProgram(program.program);

		// These do not change during our drawing pass
		glUniform1i(program.locations[4], rendStates.fog);
		glUniform1f(program.locations[9], timeState);

		pie_internal::currentShaderMode = shaderMode;
	}
	glUniform4fv(program.locations[0], 1, &pal_PIELIGHTtoVec4(colour)[0]);
	pie_SetTexturePage(shape->texpage);

	glUniform4fv(program.locations[1], 1, &pal_PIELIGHTtoVec4(teamcolour)[0]);
	glUniform1i(program.locations[3], maskpage != iV_TEX_INVALID);

	glUniform1i(program.locations[8], 0);

	glUniformMatrix4fv(program.locations[10], 1, GL_FALSE, glm::value_ptr(ModelView));
	glUniformMatrix4fv(program.locations[11], 1, GL_FALSE, glm::value_ptr(Proj * ModelView));
	glUniformMatrix4fv(program.locations[12], 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(ModelView))));
	glUniform4fv(program.locations[13], 1, &sunPos[0]);
	glUniform4fv(program.locations[14], 1, &sceneColor[0]);
	glUniform4fv(program.locations[15], 1, &ambient[0]);
	glUniform4fv(program.locations[16], 1, &diffuse[0]);
	glUniform4fv(program.locations[17], 1, &specular[0]);

	glUniform1f(program.locations[18], fogBegin);
	glUniform1f(program.locations[19], fogEnd);

	float color[4] = {
		rendStates.fogColour.vector[0] / 255.f,
		rendStates.fogColour.vector[1] / 255.f,
		rendStates.fogColour.vector[2] / 255.f,
		rendStates.fogColour.vector[3] / 255.f
	};
	glUniform4fv(program.locations[20], 1, color);

	if (program.locations[2] >= 0)
	{
		glUniform1f(program.locations[2], shaderStretch);
	}
	if (program.locations[5] >= 0)
	{
		glUniform1i(program.locations[5], normalpage != iV_TEX_INVALID);
	}
	if (program.locations[6] >= 0)
	{
		glUniform1i(program.locations[6], specularpage != iV_TEX_INVALID);
	}
	if (program.locations[7] >= 0)
	{
		glUniform1i(program.locations[7], ecmState);
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

	return program;
}

void pie_SetDepthBufferStatus(DEPTH_MODE depthMode)
{
	switch (depthMode)
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
	if (offset == 0.0f)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
	else
	{
		glPolygonOffset(offset, offset);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}
}

/// Set the OpenGL fog start and end
void pie_UpdateFogDistance(float begin, float end)
{
	rendStates.fogBegin = begin;
	rendStates.fogEnd = end;
}

void pie_SetFogStatus(bool val)
{
	if (rendStates.fogEnabled)
	{
		//fog enabled so toggle if required
		rendStates.fog = val;
	}
	else
	{
		rendStates.fog = false;
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
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case TEXPAGE_EXTERN:
			break;
		default:
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

		case REND_TEXT:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA /* Should be GL_ONE_MINUS_SRC1_COLOR, if supported. Also, gl_FragData[1] then needs to be set in text.frag. */);
			break;

		default:
			ASSERT(false, "Bad render state");
			break;
		}
	}
	return;
}

RENDER_STATE getCurrentRenderState()
{
	return rendStates;
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

int pie_GetMaxAntialiasing()
{
	GLint maxSamples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	return maxSamples;
}
