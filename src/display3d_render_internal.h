#pragma once

#include <glm/fwd.hpp>

namespace gfx_api
{
struct abstract_texture;
}

void display3d_renderSurroundings(const glm::mat4& projectionMatrix, const glm::mat4& skyboxViewMatrix);
void display3d_doConstructionLines(const glm::mat4& viewMatrix);
void display3d_locateMouse();
void display3d_drawWorldToScreenBlit(gfx_api::abstract_texture* sourceTexture);
void display3d_processSensorTarget();
void display3d_processDestinationTarget();
