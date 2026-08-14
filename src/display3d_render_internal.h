#pragma once

#include "lib/ivis_opengl/gfx_api.h"

#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <cstdint>

void display3d_renderSurroundings(const glm::mat4& projectionMatrix, const glm::mat4& skyboxViewMatrix);
void display3d_doConstructionLines(const glm::mat4& viewMatrix);
void display3d_locateMouse();
void display3d_drawWorldToScreenBlit(gfx_api::abstract_texture* sourceTexture);
void display3d_drawFsr1Easu(gfx_api::abstract_texture* sourceTexture);
void display3d_drawFsr1Rcas(gfx_api::abstract_texture* sourceTexture);
void display3d_drawSmaaEdges(gfx_api::abstract_texture* sourceTexture);
void display3d_drawSmaaWeights(gfx_api::abstract_texture* edgesTexture);
void display3d_drawSmaaBlend(gfx_api::abstract_texture* colorTexture, gfx_api::abstract_texture* weightsTexture);
void display3d_processSensorTarget();
void display3d_processDestinationTarget();
/// Fullscreen triangle VBO used by post-processing passes (null before init3DView).
gfx_api::buffer* display3d_getScreenTriangleVBO();
/// Scale/clamp UVs from `sourceTex` onto a used sub-rect of size usedW x usedH.
void display3d_fillUvScaleClamp(uint32_t usedW, uint32_t usedH, gfx_api::abstract_texture* sourceTex, glm::vec4& uvScaleClamp);
/// Scale/clamp UVs from pipeline surface `id` onto that surface's used (dyn-res) extent.
void display3d_fillSurfaceUvScaleClamp(gfx_api::PipelineSurfaceId id, glm::vec4& uvScaleClamp);
/// Scale/clamp UVs from a scene-sized source texture onto the rendered scene sub-rect.
void display3d_fillSceneUvScaleClamp(gfx_api::abstract_texture* sourceTex, glm::vec4& uvScaleClamp);
