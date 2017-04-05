/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/***************************************************************************/
/*
 * pieState.h
 *
 * render State controlr all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piestate_h
#define _piestate_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "piedef.h"
#include "lib/framework/vector.h"
#include <glm/gtc/type_ptr.hpp>

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

struct RENDER_STATE
{
	bool				fogEnabled;
	bool				fog;
	PIELIGHT			fogColour;
	float				fogBegin;
	float				fogEnd;
	SDWORD				texPage;
	REND_MODE			rendMode;
};

void rendStatesRendModeHack();  // Sets rendStates.rendMode = REND_ALPHA; (Added during merge, since the renderStates is now static.)

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern unsigned int pieStateCount;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_SetDefaultStates(void);//Sets all states
extern void pie_SetDepthBufferStatus(DEPTH_MODE depthMode);
extern void pie_SetDepthOffset(float offset);
//fog available
extern void pie_EnableFog(bool val);
extern bool pie_GetFogEnabled(void);
//fog currently on
extern void pie_SetFogStatus(bool val);
extern bool pie_GetFogStatus(void);
extern void pie_SetFogColour(PIELIGHT colour);
extern PIELIGHT pie_GetFogColour(void) WZ_DECL_PURE;
extern void pie_UpdateFogDistance(float begin, float end);
//render states
extern void pie_SetTexturePage(SDWORD num);
extern void pie_SetRendMode(REND_MODE rendMode);
RENDER_STATE getCurrentRenderState();

int pie_GetMaxAntialiasing();

bool pie_LoadShaders();
void pie_FreeShaders();
SHADER_MODE pie_LoadShader(const char *programName, const char *vertexPath, const char *fragmentPath,
	const std::vector<std::string> &);

namespace pie_internal
{
	struct SHADER_PROGRAM
	{
		GLuint program;

		// Uniforms
		std::vector<GLint> locations;

		// Attributes
		GLint locVertex;
		GLint locNormal;
		GLint locTexCoord;
		GLint locColor;
	};

	extern std::vector<SHADER_PROGRAM> shaderProgram;
	extern SHADER_MODE currentShaderMode;
	extern GLuint rectBuffer;

	/**
	 * setUniforms is an overloaded wrapper around glUniform* functions
	 * accepting glm structures.
	 * Please do not use directly, use pie_ActivateShader below.
	 */

	inline void setUniforms(GLint location, const ::glm::vec4 &v)
	{
		glUniform4f(location, v.x, v.y, v.z, v.w);
	}

	inline void setUniforms(GLint location, const ::glm::mat4 &m)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
	}

	inline void setUniforms(GLint location, const Vector2i &v)
	{
		glUniform2i(location, v.x, v.y);
	}

	inline void setUniforms(GLint location, const Vector2f &v)
	{
		glUniform2f(location, v.x, v.y);
	}

	inline void setUniforms(GLint location, const int32_t &v)
	{
		glUniform1i(location, v);
	}

	inline void setUniforms(GLint location, const float &v)
	{
		glUniform1f(location, v);
	}

	/**
	 * uniformSetter is a variadic function object.
	 * It's recursively expanded so that uniformSetter(array, arg0, arg1...);
	 * will yield the following code:
	 * {
	 *     setUniforms(arr[0], arg0);
	 *     setUniforms(arr[1], arg1);
	 *     setUniforms(arr[2], arg2);
	 *     ...
	 *     setUniforms(arr[n], argn);
	 * }
	 */
	template<typename...T>
	struct uniformSetter
	{
		void operator()(const std::vector<GLint> &locations, T...) const;
	};

	template<>
	struct uniformSetter<>
	{
		void operator()(const std::vector<GLint> &) const {}
	};

	template<typename T, typename...Args>
	struct uniformSetter<T, Args...>
	{
		void operator()(const std::vector<GLint> &locations, const T& value, const Args&...args) const
		{
			constexpr int N = sizeof...(Args) + 1;
			setUniforms(locations[locations.size() - N], value);
			uniformSetter<Args...>()(locations, args...);
		}
	};
}

/**
 * Bind program and sets the uniforms Args.
 * The uniform binding mechanism works in conjonction with pie_LoadShader.
 * When shader is loaded uniform location is fetched in a certain order.
 * in pie_ActivateShader the uniforms passed as variadic Args template
 * must be given in the same order.
 *
 * For instance if a shader is loaded by :
 * pie_LoadShader(..., { vector_uniform_name, matrix_uniform_name });
 * then subsequent pie_ActivateShader must be called like :
 * pie_ActivateShader(..., some_vector, some_matrix);
 * It will be expanded as :
 * {
 *     glUseProgram(...);
 *     glUniform4f(vector_uniform_location, some_vector);
 *     glUniform4fv(matrix_uniform_location, some_matrix);
 * }
 * Note that uniform count is checked at run time.
 * Uniform type is not checked but the GL implementation will complain
 * if the uniform type doesn't match.
 */
template<typename...T>
pie_internal::SHADER_PROGRAM &pie_ActivateShader(SHADER_MODE shaderMode, const T&... Args)
{
	pie_internal::SHADER_PROGRAM &program = pie_internal::shaderProgram[shaderMode];
	if (shaderMode != pie_internal::currentShaderMode)
	{
		glUseProgram(program.program);
		pie_internal::currentShaderMode = shaderMode;
	}
	assert(program.locations.size() == sizeof...(T));
	pie_internal::uniformSetter<T...>()(program.locations, Args...);
	return program;
}


// Actual shaders (we do not want to export these calls)
pie_internal::SHADER_PROGRAM &pie_ActivateShaderDeprecated(SHADER_MODE shaderMode, const iIMDShape *shape, PIELIGHT teamcolour, PIELIGHT colour, const glm::mat4 &ModelView, const glm::mat4 &Proj,
	const glm::vec4 &sunPos, const glm::vec4 &sceneColor, const glm::vec4 &ambient, const glm::vec4 &diffuse, const glm::vec4 &specular);
void pie_DeactivateShader();
void pie_SetShaderStretchDepth(float stretch);
float pie_GetShaderStretchDepth();
void pie_SetShaderTime(uint32_t shaderTime);
void pie_SetShaderEcmEffect(bool value);

/* Errors control routine */
#ifdef DEBUG
#define glErrors() \
	_glerrors(__FUNCTION__, __FILE__, __LINE__)
#else
#define glErrors()
#endif

extern bool _glerrors(const char *, const char *, int);

#endif // _pieState_h
