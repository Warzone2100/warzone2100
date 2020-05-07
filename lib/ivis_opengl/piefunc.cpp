/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 *  Extended render routines for 3D rendering.
 */

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/framework/math_ext.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "piematrix.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieclip.h"
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include <array>

static GFX *skyboxGfx = nullptr;
static GFX *radarViewGfx[2] = { nullptr, nullptr };

#define VW_VERTICES 5

void pie_SetViewingWindow(Vector3i *v, PIELIGHT colour)
{
	Vector3i pieVrts_insidefill[VW_VERTICES - 1];
	Vector3i pieVrts_outline[VW_VERTICES];

	if (!radarViewGfx[0])
	{
		radarViewGfx[0] = new GFX(GFX_COLOUR, GL_TRIANGLE_STRIP, 2);
		radarViewGfx[1] = new GFX(GFX_COLOUR, GL_LINE_STRIP, 2);
	}

	static_assert(VW_VERTICES == 5, "Assumption that VW_VERTICES == 5 invalid. Update the following code.");
	pieVrts_outline[0] = v[1];
	pieVrts_outline[1] = v[0];
	pieVrts_outline[2] = v[2];
	pieVrts_outline[3] = v[3];
	pieVrts_outline[4] = v[1];
	pieVrts_insidefill[0] = v[2];
	pieVrts_insidefill[1] = v[3];
	pieVrts_insidefill[2] = v[0];
	pieVrts_insidefill[3] = v[1];

	gfx_api::gfxFloat vert[VW_VERTICES * 2];
	GLbyte cols[VW_VERTICES * 4];

	// Initialize the RGB values for both, and the alpha for the inside fill
	for (int i = 0; i < VW_VERTICES * 4; i += 4)
	{
		cols[i + 0] = colour.byte.r;
		cols[i + 1] = colour.byte.g;
		cols[i + 2] = colour.byte.b;
		cols[i + 3] = colour.byte.a >> 1;
	}

	// Inside fill
	for (int i = 0; i < (VW_VERTICES - 1); i++)
	{
		vert[i * 2 + 0] = pieVrts_insidefill[i].x;
		vert[i * 2 + 1] = pieVrts_insidefill[i].y;
	}
	radarViewGfx[0]->buffers(VW_VERTICES - 1, vert, cols);

	// Outline
	for (int i = 0; i < VW_VERTICES; i++)
	{
		vert[i * 2 + 0] = pieVrts_outline[i].x;
		vert[i * 2 + 1] = pieVrts_outline[i].y;
	}
	for (int i = 0; i < VW_VERTICES * 4; i += 4) // set the proper alpha for the outline
	{
		cols[i + 3] = colour.byte.a;
	}
	radarViewGfx[1]->buffers(VW_VERTICES, vert, cols);
}

void pie_DrawViewingWindow(const glm::mat4 &modelViewProjectionMatrix)
{
	pie_SetRendMode(REND_ALPHA);
	radarViewGfx[0]->draw(modelViewProjectionMatrix);
	radarViewGfx[1]->draw(modelViewProjectionMatrix);
}

void pie_TransColouredTriangle(const std::array<Vector3f, 3> &vrt, PIELIGHT c, const glm::mat4 &modelViewMatrix)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ADDITIVE);
	glm::vec4 color(c.byte.r / 255.f, c.byte.g / 255.f, c.byte.b / 255.f, 128.f / 255.f);
	const auto &program = pie_ActivateShader(SHADER_GENERIC_COLOR, pie_PerspectiveGet() * modelViewMatrix, color);

	static gfx_api::buffer* buffer = nullptr;
	if (!buffer)
		buffer = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::stream_draw);
	buffer->upload(3 * sizeof(Vector3f), vrt.data());
	buffer->bind();
	glVertexAttribPointer(program.locVertex, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(program.locVertex);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisableVertexAttribArray(program.locVertex);
}

GLuint depthTexture;

void saveDepthBuffer(unsigned int screenWidth, unsigned int screenHeight){
	if(depthTexture == 0)
	{
		glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	}

	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenWidth,screenHeight);
}

void drawRange(Vector3i p1, int radius, Vector3i position, Vector3i rotation, float distance, int screenWidth, int screenHeight, Vector3f color)
{
	const glm::mat4 &viewMatrix =
		glm::translate(glm::vec3(0.f, 0.f, distance)) *
		glm::scale(glm::vec3(pie_GetResScalingFactor() / 100.f)) *
		glm::rotate(rotation.z * (360.f / 65536.0f) * (3.141592f / 180.0f), glm::vec3(0.f, 0.f, 1.f)) *
		glm::rotate(rotation.x * (360.f / 65536.0f) * (3.141592f / 180.0f), glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(rotation.y * (360.f / 65536.0f) * (3.141592f / 180.0f), glm::vec3(0.f, 1.f, 0.f)) *
		glm::translate(glm::vec3(-position.x, -position.y, position.z)) *
		glm::translate(glm::vec3(p1.x, 0, -p1.y)) *
		glm::scale(glm::vec3(radius));
		
	static GLuint shaderProgram = 0;

	if(shaderProgram == 0)
	{
		shaderProgram = glCreateProgram();
		glBindAttribLocation(shaderProgram, 0, "vertex");

		GLint status;
		GLint infologLen = 0;
		GLint charsWritten = 0;
		GLchar *infoLog = nullptr;

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		const char* vertexShaderSource[1] = {
			"attribute vec4 c;uniform mat4 ModelViewProjectionMatrix;void main(void) {gl_Position = ModelViewProjectionMatrix * c;}"
		};
		glShaderSource(vertexShader, 1, vertexShaderSource, nullptr);
		glCompileShader(vertexShader);

		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);

		if (!status)
		{
			printf("Error compiling vertex shader\n");
			glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infologLen);
			infoLog = (GLchar *)malloc(infologLen);
			glGetShaderInfoLog(vertexShader, infologLen, &charsWritten, infoLog);
			printf("%s\n", infoLog);
			free(infoLog);
			exit(1);
		}

		glAttachShader(shaderProgram, vertexShader);

		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		const char* fragmentShaderSource[1] = {
			"uniform ivec2 screen;uniform sampler2D tex;uniform vec3 color;void main(void) {vec4 t = texture(tex, vec2(gl_FragCoord.x / screen.x, gl_FragCoord.y / screen.y));float z = abs(gl_FragCoord.z - t.z) * 1000;float distance = clamp(z, 0, 1);float opacity = 1 - clamp(z / 4, 0, 1);gl_FragColor=vec4(distance > 0.5 ? color : mix(vec3(1), color, distance / 0.5), opacity);}"
		};
		glShaderSource(fragmentShader, 1, fragmentShaderSource, nullptr);
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);

		if (!status)
		{
			printf("Error compiling fragment shader\n");
			glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infologLen);
			infoLog = (GLchar *)malloc(infologLen);
			glGetShaderInfoLog(fragmentShader, infologLen, &charsWritten, infoLog);
			printf("%s\n", infoLog);
			free(infoLog);
			exit(1);
		}

		glAttachShader(shaderProgram, fragmentShader);

		glLinkProgram(shaderProgram);

		// Check for linkage errors
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
		if (!status)
		{
			printf("Error linking program\n");
			glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infologLen);
			infoLog = (GLchar *)malloc(infologLen);
			glGetProgramInfoLog(shaderProgram, infologLen, &charsWritten, infoLog);
			printf("%s\n", infoLog);
			free(infoLog);
			exit(1);
		}

	}

	glUseProgram(shaderProgram);
	pie_SetRendMode(REND_ADDITIVE);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	// TODO: Interpolate position!

	static std::vector<float> vrt;

	if(vrt.size() == 0)
	{
		float v1, v2;

		for(int i = 0; i < 32; i++)
		{
			v1 = 2.f * M_PI / 32 * i;
			v2 = 2.f * M_PI / 32 * (i + 1);

			vrt.push_back(cos(v1));
			vrt.push_back(1000);
			vrt.push_back(sin(v1));

			vrt.push_back(cos(v1));
			vrt.push_back(0);
			vrt.push_back(sin(v1));

			vrt.push_back(cos(v2));
			vrt.push_back(0);
			vrt.push_back(sin(v2));

			vrt.push_back(cos(v1));
			vrt.push_back(1000);
			vrt.push_back(sin(v1));

			vrt.push_back(cos(v2));
			vrt.push_back(0);
			vrt.push_back(sin(v2));

			vrt.push_back(cos(v2));
			vrt.push_back(1000);
			vrt.push_back(sin(v2));
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform3f(glGetUniformLocation(shaderProgram, "color"), color.x, color.y, color.b);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
	glUniform2i(glGetUniformLocation(shaderProgram, "screen"), screenWidth, screenHeight);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "ModelViewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(pie_PerspectiveGet() * viewMatrix));

	static gfx_api::buffer* vrtBuffer = nullptr;
	if (!vrtBuffer)
		vrtBuffer = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::stream_draw);
	vrtBuffer->upload(vrt.size() * sizeof(float), vrt.data());
	vrtBuffer->bind();
	glVertexAttribPointer(glGetAttribLocation(shaderProgram, "c"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0));
	glEnableVertexAttribArray(glGetAttribLocation(shaderProgram, "c"));

	glDrawArrays(GL_TRIANGLES, 0, vrt.size() / 3);
	glDisableVertexAttribArray(glGetAttribLocation(shaderProgram, "c"));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
}

void pie_Skybox_Init()
{
	const int u = 0;
	const int v = 0;
	const int w = 1;
	const int h = 1;
	const float r = 1.0f; // just because it is shorter than 1.0f

	const Vector3f
		vertexFront0 = Vector3f(-r, 0, r), // front
		vertexFront1 = Vector3f(-r, r, r),
		vertexFront2 = Vector3f(r, 0, r),
		vertexFront3 = Vector3f(r, r, r),
		vertexRight0 = Vector3f(r, 0, -r), // right
		vertexRight1 = Vector3f(r, r, -r),
		vertexBack0 = Vector3f(-r, 0, -r), // back
		vertexBack1 = Vector3f(-r, r, -r),
		vertexLeft0 = Vector3f(-r, 0, r),
		vertexLeft1 = Vector3f(-r, r, r);

	const std::array<Vector3f, 24> vertex{
		// Front quad
		vertexFront0, vertexFront1, vertexFront2,
		vertexFront3, vertexFront2, vertexFront1,
		// Right quad
		vertexFront2, vertexFront3, vertexRight0,
		vertexRight1, vertexRight0, vertexFront3,
		// Back quad
		vertexRight0, vertexRight1, vertexBack0,
		vertexBack1, vertexBack0, vertexRight1,
		// Left quad
		vertexBack0, vertexBack1, vertexLeft0,
		vertexLeft1, vertexLeft0, vertexBack1,

	};
	const Vector2f
		uvFront0 = Vector2f(u + w * 0, (v + h)),
		uvFront1 = Vector2f(u + w * 0, v),
		uvFront2 = Vector2f(u + w * 2, v + h),
		uvFront3 = Vector2f(u + w * 2, v),
		uvRight0 = Vector2f(u + w * 4, v + h),
		uvRight1 = Vector2f(u + w * 4, v),
		uvBack0 = Vector2f(u + w * 6, v + h),
		uvBack1 = Vector2f(u + w * 6, v),
		uvLeft0 = Vector2f(u + w * 8, v + h),
		uvLeft1 = Vector2f(u + w * 8, v);

	const std::array<Vector2f, 24> texc =
	{
		// Front quad
		uvFront0, uvFront1, uvFront2,
		uvFront3, uvFront2, uvFront1,
		// Right quad
		uvFront2, uvFront3, uvRight0,
		uvRight1, uvRight0, uvFront3,
		// Back quad
		uvRight0, uvRight1, uvBack0,
		uvBack1, uvBack0, uvRight1,
		// Left quad
		uvBack0, uvBack1, uvLeft0,
		uvLeft1, uvLeft0, uvBack1,
	};

	GL_DEBUG("Initializing skybox");
	skyboxGfx = new GFX(GFX_TEXTURE, GL_TRIANGLES, 3);
	skyboxGfx->buffers(24, vertex.data(), texc.data());
}

void pie_Skybox_Texture(const char *filename)
{
	skyboxGfx->loadTexture(filename);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
}

void pie_Skybox_Shutdown()
{
	delete skyboxGfx;
}

void pie_DrawSkybox(float scale, const glm::mat4 &viewMatrix)
{
	// no use in updating the depth buffer
	glDepthMask(GL_FALSE);
	// enable alpha
	pie_SetRendMode(REND_ALPHA);

	// Apply scale matrix
	skyboxGfx->draw(pie_PerspectiveGet() * viewMatrix * glm::scale(glm::vec3(scale, scale / 2.f, scale)));
}
