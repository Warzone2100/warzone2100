#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "lib/ivis_opengl/render_graph/blueprint.h"
#include "lib/ivis_opengl/shadows.h"

#include <array>
#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct InGame3DFrameContext
{
	glm::mat4 perspectiveViewMatrix{};
	glm::mat4 viewMatrix{};
	glm::mat4 baseViewMatrix{};
	glm::mat4 perspectiveMatrix{};
	glm::vec3 cameraPos{};
	ShadowCascadesInfo shadowCascadesInfo{};
	std::array<glm::mat4, WZ_MAX_SHADOW_CASCADES> cascadeProj{};
	std::array<glm::mat4, WZ_MAX_SHADOW_CASCADES> cascadeView{};
	uint32_t currentGameFrame = 0;
};

InGame3DFrameContext& pie_GetInGame3DFrameContext();
void pie_BindInGame3DFrameContext(InGame3DFrameContext* ctx);
void pie_ResetInGame3DFrameContextForFrame();
bool pie_IsInGame3DFrameContextReady();

void display3d_recordSceneOverlays(const gfx_api::RenderPassContext& passCtx);
void display3d_recordSceneDebugOverlays(const gfx_api::RenderPassContext& passCtx);

void registerInGame3DRecordFuncs(gfx_api::RecordFuncTable& table);
