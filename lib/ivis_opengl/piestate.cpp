/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
#include "lib/framework/physfs_ext.h"

#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "screen.h"
#include "pieclip.h"
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
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
gfx_api::buffer* pie_internal::rectBuffer = nullptr;
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

static inline glm::vec4 pal_PIELIGHTtoVec4(PIELIGHT rgba)
{
	return (1 / 255.0f) * glm::vec4{
		rgba.byte.r,
		rgba.byte.g,
		rgba.byte.b,
		rgba.byte.a
	};
}

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
		WZ_PHYSFS_readBytes(fp, buffer, filesize);
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

GLint wz_GetGLIntegerv(GLenum pname, GLint defaultValue = 0)
{
	GLint retVal = defaultValue;
	while(glGetError() != GL_NO_ERROR) { } // clear the OpenGL error queue
	glGetIntegerv(pname, &retVal);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		retVal = defaultValue;
	}
	return retVal;
}

SHADER_VERSION getMinimumShaderVersionForCurrentGLContext()
{
	// Determine the shader version directive we should use by examining the current OpenGL context
	GLint gl_majorversion = wz_GetGLIntegerv(GL_MAJOR_VERSION, 0);
	GLint gl_minorversion = wz_GetGLIntegerv(GL_MINOR_VERSION, 0);

	SHADER_VERSION version = VERSION_120; // for OpenGL < 3.2, we default to VERSION_120 shaders
	if ((gl_majorversion > 3) || ((gl_majorversion == 3) && (gl_minorversion >= 2)))
	{
		// OpenGL 3.2+
		// Since WZ only supports Core contexts with OpenGL 3.2+, we cannot use the version_120 directive
		// (which only works in compatbility contexts). Instead, use GLSL version "150 core".
		version = VERSION_150_CORE;
	}
	return version;
}

SHADER_VERSION getMaximumShaderVersionForCurrentGLContext()
{
	// Instead of querying the GL_SHADING_LANGUAGE_VERSION string and trying to parse it,
	// which is rife with difficulties because drivers can report very different strings (and formats),
	// use the known (and explicit) mapping table between OpenGL version and supported GLSL version.

	GLint gl_majorversion = wz_GetGLIntegerv(GL_MAJOR_VERSION, 0);
	GLint gl_minorversion = wz_GetGLIntegerv(GL_MINOR_VERSION, 0);

	// For OpenGL < 3.2, default to VERSION_120 shaders
	SHADER_VERSION version = VERSION_120;
	if(gl_majorversion == 3)
	{
		switch(gl_minorversion)
		{
			case 0: // 3.0 => 1.30
				version = VERSION_130;
				break;
			case 1: // 3.1 => 1.40
				version = VERSION_140;
				break;
			case 2: // 3.2 => 1.50
				version = VERSION_150_CORE;
				break;
			case 3: // 3.3 => 3.30
				version = VERSION_330_CORE;
				break;
			default:
				// Return the 3.3 value
				version = VERSION_330_CORE;
				break;
		}
	}
	else if (gl_majorversion == 4)
	{
		switch(gl_minorversion)
		{
			case 0: // 4.0 => 4.00
				version = VERSION_400_CORE;
				break;
			case 1: // 4.1 => 4.10
				version = VERSION_410_CORE;
				break;
			default:
				// Return the 4.1 value
				// NOTE: Nothing above OpenGL 4.1 is supported on macOS
				version = VERSION_410_CORE;
				break;
		}
	}
	else if (gl_majorversion > 4)
	{
		// Return the OpenGL 4.1 value (for now)
		version = VERSION_410_CORE;
	}
	return version;
}

const char * shaderVersionString(SHADER_VERSION version)
{
	switch(version)
	{
		case VERSION_120:
			return "#version 120\n";
		case VERSION_130:
			return "#version 130\n";
		case VERSION_140:
			return "#version 140\n";
		case VERSION_150_CORE:
			return "#version 150 core\n";
		case VERSION_330_CORE:
			return "#version 330 core\n";
		case VERSION_400_CORE:
			return "#version 400 core\n";
		case VERSION_410_CORE:
			return "#version 410 core\n";
		case VERSION_AUTODETECT_FROM_LEVEL_LOAD:
			return "";
		case VERSION_FIXED_IN_FILE:
			return "";
		// Deliberately omit "default:" case to trigger a compiler warning if the SHADER_VERSION enum is expanded but the new cases aren't handled here
	}
	return ""; // Should not not reach here - silence a GCC warning
}

std::string getShaderVersionDirective(const char* shaderData)
{
	// Relying on the GLSL documentation, which says:
	// "The #version directive must occur in a shader before anything else, except for comments and white space."
	// Look for the first line that contains a non-whitespace character (that isn't preceeded by a comment indicator)
	//
	// White space: the space character, horizontal tab, vertical tab, form feed, carriage-return, and line-feed.
	const char glsl_whitespace_chars[] = " \t\v\f\r\n";

	enum CommentMode {
		NONE,
		LINE_DELIMITED_COMMENT,
		ASTERISK_SLASH_DELIMITED_COMMENT
	};

	const size_t shaderLen = strlen(shaderData);
	CommentMode currentCommentMode = CommentMode::NONE;
	const char* pChar = shaderData;
	// Find first non-whitespace, non-comment character
	for (; pChar < shaderData + shaderLen; ++pChar)
	{
		bool foundFirstNonIgnoredChar = false;
		switch (*pChar)
		{
			// the non-newline whitespace chars
			case ' ':
			case '\t':
			case '\v':
			case '\f':
				// ignore whitespace
				break;
			case '\r':
			case '\n':
				// newlines reset line-comment state
				if (currentCommentMode == CommentMode::LINE_DELIMITED_COMMENT)
				{
					currentCommentMode = CommentMode::NONE;
				}
				// otherwise, ignore
				break;
			case '/':
				if (currentCommentMode == CommentMode::NONE)
				{
					// peek-ahead at next char to see if this starts a comment, or should be treated as a usable character
					switch(*(pChar + 1))
					{
						case '/':
							// "//" starts a comment on a line
							currentCommentMode = CommentMode::LINE_DELIMITED_COMMENT;
							++pChar;
							break;
						case '*':
							// "/*" starts a comment until "*/" is detected
							currentCommentMode = CommentMode::ASTERISK_SLASH_DELIMITED_COMMENT;
							++pChar;
							break;
						default:
							// this doesn't start a comment - it's the first usable character
							foundFirstNonIgnoredChar = true;
							break;
					}
				}
				else
				{
					// ignore, part of a comment
				}
				break;
			case '*':
				if (currentCommentMode == CommentMode::ASTERISK_SLASH_DELIMITED_COMMENT)
				{
					// peek-ahead at next char to see if this ends the current comment
					if (*(pChar + 1) == '/')
					{
						// this is the beginning of a "*/" that terminates the current comment
						currentCommentMode = CommentMode::NONE;
						++pChar;
						break;
					}
				}
				else if (currentCommentMode == CommentMode::LINE_DELIMITED_COMMENT)
				{
					// ignore - part of a comment
					break;
				}
				else if (currentCommentMode == CommentMode::NONE)
				{
					// this is the first usable character
					foundFirstNonIgnoredChar = true;
				}
				break;
			default:
				if (currentCommentMode == CommentMode::NONE)
				{
					foundFirstNonIgnoredChar = true;
				}
		}

		if (foundFirstNonIgnoredChar) break;
	}

	if (pChar >= shaderData + shaderLen)
	{
		// Did not find a non-ignored (whitespace/comment) character before the end of the shader?
		return "";
	}

	// Check if the first non-ignored characters start a #version directive
	std::string shaderStringTrimmedBeginning(pChar);
	std::string versionPrefix("#version");
	if (shaderStringTrimmedBeginning.length() < versionPrefix.length())
	{
		// not enough remaining characters to form the "#version" prefix
		return "";
	}
	if (shaderStringTrimmedBeginning.compare(0, versionPrefix.length(), versionPrefix) != 0)
	{
		// Does not start with a version directive
		return "";
	}

	// Starts with a #version directive - extract all the characters after versionPrefix until a newline
	size_t posNextNewline = shaderStringTrimmedBeginning.find_first_of("\r\n", versionPrefix.length());
	size_t lenVersionLine = (posNextNewline != std::string::npos) ? (posNextNewline - versionPrefix.length()) : std::string::npos;
	std::string versionLine = shaderStringTrimmedBeginning.substr(versionPrefix.length(), lenVersionLine);
	// Remove any trailing comment (starting with "//" or "/*")
	size_t posCommentStart = versionLine.find("//");
	if (posCommentStart != std::string::npos)
	{
		versionLine = versionLine.substr(0, posCommentStart);
	}
	posCommentStart = versionLine.find("/*");
	if (posCommentStart != std::string::npos)
	{
		versionLine = versionLine.substr(0, posCommentStart);
	}

	// trim whitespace chars from beginning/end
	versionLine = versionLine.substr(versionLine.find_first_not_of(glsl_whitespace_chars));
	versionLine.erase(versionLine.find_last_not_of(glsl_whitespace_chars)+1);

	return versionLine;
}

SHADER_VERSION pie_detectShaderVersion(const char* shaderData)
{
	std::string shaderVersionDirective = getShaderVersionDirective(shaderData);

	// Special case for external loaded shaders that want to opt-in to an automatic #version header
	// that is the minimum supported on the current OpenGL context.
	// To properly support this, the shaders should be written to support GLSL "version 120" through "version 150 core".
	//
	// For examples on how to do this, see the built-in shaders in the data/shaders directory, and pay
	// particular attention to their use of "#if __VERSION__" preprocessor directives.
	if (shaderVersionDirective == "WZ_GLSL_VERSION_MINIMUM_FOR_CONTEXT")
	{
		return getMinimumShaderVersionForCurrentGLContext();
	}
	// Special case for external loaded shaders that want to opt-in to an automatic #version header
	// that is the maximum supported on the current OpenGL context.
	//
	// Important: For compatibility with the same systems that Warzone supports, you should strive to ensure the
	// shaders are compatible with a minimum of GLSL version 1.20 / GLSL version 1.50 core (and / or fallback gracefully).
	else if (shaderVersionDirective == "WZ_GLSL_VERSION_MAXIMUM_FOR_CONTEXT")
	{
		return getMaximumShaderVersionForCurrentGLContext();
	}
	else
	{
		return SHADER_VERSION::VERSION_FIXED_IN_FILE;
	}
}

SHADER_VERSION autodetectShaderVersion_FromLevelLoad(const char* filePath, const char* shaderContents)
{
	SHADER_VERSION version = pie_detectShaderVersion(shaderContents);
	if (version == SHADER_VERSION::VERSION_FIXED_IN_FILE)
	{
		debug(LOG_WARNING, "SHADER '%s' specifies a fixed #version directive. This may not work with Warzone's ability to use either OpenGL < 3.2 Compatibility Profiles, or OpenGL 3.2+ Core Profiles.", filePath);
	}
	return version;
}

// Read/compile/link shaders
SHADER_MODE pie_LoadShader(SHADER_VERSION vertex_version, SHADER_VERSION fragment_version, const char *programName, const std::string &vertexPath, const std::string &fragmentPath,
	const std::vector<std::string> &uniformNames)
{
	pie_internal::SHADER_PROGRAM program;
	GLint status;
	bool success = true; // Assume overall success

	program.program = glCreateProgram();
	glBindAttribLocation(program.program, 0, "vertex");
	glBindAttribLocation(program.program, 1, "vertexTexCoord");
	glBindAttribLocation(program.program, 2, "vertexColor");
	ASSERT_OR_RETURN(SHADER_NONE, program.program, "Could not create shader program!");

	char* shaderContents = nullptr;

	if (!vertexPath.empty())
	{
		success = false; // Assume failure before reading shader file

		if ((shaderContents = readShaderBuf(vertexPath.c_str())))
		{
			GLuint shader = glCreateShader(GL_VERTEX_SHADER);

			if (vertex_version == SHADER_VERSION::VERSION_AUTODETECT_FROM_LEVEL_LOAD)
			{
				vertex_version = autodetectShaderVersion_FromLevelLoad(vertexPath.c_str(), shaderContents);
			}

			const char* ShaderStrings[2] = { shaderVersionString(vertex_version), shaderContents };

			glShaderSource(shader, 2, ShaderStrings, nullptr);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Vertex shader compilation has failed [%s]", vertexPath.c_str());
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
				glObjectLabel(GL_SHADER, shader, -1, vertexPath.c_str());
			}
			free(shaderContents);
			shaderContents = nullptr;
		}
	}

	if (success && !fragmentPath.empty())
	{
		success = false; // Assume failure before reading shader file

		if ((shaderContents = readShaderBuf(fragmentPath.c_str())))
		{
			GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

			if (fragment_version == SHADER_VERSION::VERSION_AUTODETECT_FROM_LEVEL_LOAD)
			{
				fragment_version = autodetectShaderVersion_FromLevelLoad(fragmentPath.c_str(), shaderContents);
			}

			const char* ShaderStrings[2] = { shaderVersionString(fragment_version), shaderContents };

			glShaderSource(shader, 2, ShaderStrings, nullptr);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Fragment shader compilation has failed [%s]", fragmentPath.c_str());
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
				glObjectLabel(GL_SHADER, shader, -1, fragmentPath.c_str());
			}
			free(shaderContents);
			shaderContents = nullptr;
		}
	}

	if (success)
	{
		glLinkProgram(program.program);

		// Check for linkage errors
		glGetProgramiv(program.program, GL_LINK_STATUS, &status);
		if (!status)
		{
			debug(LOG_ERROR, "Shader program linkage has failed [%s, %s]", vertexPath.c_str(), fragmentPath.c_str());
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

	// Determine the shader version directive we should use by examining the current OpenGL context
	// (The built-in shaders support (and have been tested with) VERSION_120 and VERSION_150_CORE)
	SHADER_VERSION version = getMinimumShaderVersionForCurrentGLContext();

	// Load some basic shaders
	pie_internal::shaderProgram.push_back(program);
	int shaderEnum = 0;

	// TCMask shader for map-placed models with advanced lighting
	debug(LOG_3D, "Loading shader: SHADER_COMPONENT");
	result = pie_LoadShader(version, "Component program", "shaders/tcmask.vert", "shaders/tcmask.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_COMPONENT, "Failed to load component shader");

	// TCMask shader for buttons with flat lighting
	debug(LOG_3D, "Loading shader: SHADER_BUTTON");
	result = pie_LoadShader(version, "Button program", "shaders/button.vert", "shaders/button.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_BUTTON, "Failed to load button shader");

	// Plain shader for no lighting
	debug(LOG_3D, "Loading shader: SHADER_NOLIGHT");
	result = pie_LoadShader(version, "Plain program", "shaders/nolight.vert", "shaders/nolight.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_NOLIGHT, "Failed to load no-lighting shader");

	debug(LOG_3D, "Loading shader: SHADER_TERRAIN");
	result = pie_LoadShader(version, "terrain program", "shaders/terrain_water.vert", "shaders/terrain.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex", "lightmap_tex", "textureMatrix1", "textureMatrix2",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TERRAIN, "Failed to load terrain shader");

	debug(LOG_3D, "Loading shader: SHADER_TERRAIN_DEPTH");
	result = pie_LoadShader(version, "terrain_depth program", "shaders/terrain_water.vert", "shaders/terraindepth.frag",
	{ "ModelViewProjectionMatrix", "paramx2", "paramy2", "lightmap_tex", "textureMatrix1", "textureMatrix2" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TERRAIN_DEPTH, "Failed to load terrain_depth shader");

	debug(LOG_3D, "Loading shader: SHADER_DECALS");
	result = pie_LoadShader(version, "decals program", "shaders/decals.vert", "shaders/decals.frag",
		{ "ModelViewProjectionMatrix", "paramxlight", "paramylight", "tex", "lightmap_tex", "lightTextureMatrix",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_DECALS, "Failed to load decals shader");

	debug(LOG_3D, "Loading shader: SHADER_WATER");
	result = pie_LoadShader(version, "water program", "shaders/terrain_water.vert", "shaders/water.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex1", "tex2", "textureMatrix1", "textureMatrix2",
		"fogEnabled", "fogEnd", "fogStart" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_WATER, "Failed to load water shader");

	// Rectangular shader
	debug(LOG_3D, "Loading shader: SHADER_RECT");
	result = pie_LoadShader(version, "Rect program", "shaders/rect.vert", "shaders/rect.frag",
		{ "transformationMatrix", "color" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_RECT, "Failed to load rect shader");

	// Textured rectangular shader
	debug(LOG_3D, "Loading shader: SHADER_TEXRECT");
	result = pie_LoadShader(version, "Textured rect program", "shaders/rect.vert", "shaders/texturedrect.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "theTexture" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TEXRECT, "Failed to load textured rect shader");

	debug(LOG_3D, "Loading shader: SHADER_GFX_COLOUR");
	result = pie_LoadShader(version, "gfx_color program", "shaders/gfx.vert", "shaders/gfx.frag",
		{ "posMatrix" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_GFX_COLOUR, "Failed to load textured gfx shader");

	debug(LOG_3D, "Loading shader: SHADER_GFX_TEXT");
	result = pie_LoadShader(version, "gfx_text program", "shaders/gfx.vert", "shaders/texturedrect.frag",
		{ "posMatrix", "color", "theTexture" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_GFX_TEXT, "Failed to load textured gfx shader");

	debug(LOG_3D, "Loading shader: SHADER_GENERIC_COLOR");
	result = pie_LoadShader(version, "generic color program", "shaders/generic.vert", "shaders/rect.frag", { "ModelViewProjectionMatrix", "color" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_GENERIC_COLOR, "Failed to load generic color shader");

	debug(LOG_3D, "Loading shader: SHADER_LINE");
	result = pie_LoadShader(version, "line program", "shaders/line.vert", "shaders/rect.frag", { "from", "to", "color", "ModelViewProjectionMatrix" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_LINE, "Failed to load line shader");

	// Text shader
	debug(LOG_3D, "Loading shader: SHADER_TEXT");
	result = pie_LoadShader(version, "Text program", "shaders/rect.vert", "shaders/text.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "theTexture" });
	ASSERT_OR_RETURN(false, result && ++shaderEnum == SHADER_TEXT, "Failed to load text shader");

	pie_internal::currentShaderMode = SHADER_NONE;

	GLbyte rect[] {
		0, 1, 0, 1,
		0, 0, 0, 1,
		1, 1, 0, 1,
		1, 0, 0, 1
	};
	if (!pie_internal::rectBuffer)
		pie_internal::rectBuffer = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer);
	pie_internal::rectBuffer->upload(16 * sizeof(GLbyte), rect);
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
		pie_Texture(maskpage).bind();
	}
	if (normalpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE2);
		pie_Texture(normalpage).bind();
	}
	if (specularpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE3);
		pie_Texture(specularpage).bind();
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
			pie_Texture(num).bind();
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

// A replacement for gluErrorString()
// NOTE: May return `nullptr` if error is unknown
const char * wzGLErrorString(GLenum error)
{
	switch (error) {
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
		default:
			return nullptr;
	}
}

bool _glerrors(const char *function, const char *file, int line)
{
	bool ret = false;
	GLenum err = glGetError();
	while (err != GL_NO_ERROR)
	{
		ret = true;
		const char * pErrorString = wzGLErrorString(err);
		if (pErrorString)
		{
			debug(LOG_ERROR, "OpenGL error in function %s at %s:%u: %s\n", function, file, line, pErrorString);
		}
		else
		{
			debug(LOG_ERROR, "OpenGL error in function %s at %s:%u: <unknown GL error: %u>\n", function, file, line, err);
		}
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
