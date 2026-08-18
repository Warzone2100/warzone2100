// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file topology.h
 * Frame topology snapshot, feature flags, and `IRenderTopologyQuery` facade.
 */

#pragma once

#include "attachment.h"
#include "../piedef.h"

#include <cstdint>
#include <tuple>
#include <utility>

namespace gfx_api
{

/// <summary>
/// Which frame-graph template to build (`PassGraphTopologyBlueprint::fromSnapshot`).
/// </summary>
enum class RenderScreenKind : uint8_t
{
	InGame,
	Title,
	Loading,
	Video,
};

/// <summary>
/// Optional pass/feature flags OR'd into `RenderTopologySnapshot::features`.
/// </summary>
struct RenderFeatures
{
	enum : uint32_t
	{
		/// Include a backdrop swapchain pass before main content.
		Backdrop           = 1u << 0,
		/// Reserve the in-game start fade pass slot (in-game topology only).
		GameStartFadeSlot  = 1u << 1,
		/// Include the debug overlay pass slot (in-game topology only).
		DebugOverlays      = 1u << 2,
		/// In-game world frozen: UI/backdrop-only pass graph (no 3D scene passes).
		FrozenWorldOverlay = 1u << 3,
		/// Replace the scene blit with the FSR1 upscale + sharpen pass pair.
		SceneUpscale       = 1u << 4,
		/// Insert the SMAA edge detection, blending weight and neighborhood blend passes.
		Smaa               = 1u << 5,
		/// The SMAA blend writes a scene-sized intermediate consumed by the
		/// blit or upscale chain (otherwise it writes the swapchain directly).
		SmaaIntermediate   = 1u << 6,
		/// Include SSAO generate / blur / compose passes (in-game, not frozen).
		/// ScenePrepass is inserted when `prepassNeeds()` is not None.
		SSAO               = 1u << 7,
		/// Include the deferred FogApply pass (in-game, not frozen).
		FogApply           = 1u << 8,
		/// Downsample generate AO into the blur target when blur is coarser.
		SSAODownsample     = 1u << 9,
	};
};

/// <summary>
/// Per-frame inputs that drive blueprint selection, materialization, and cache keys.
///
/// Collected from `IRenderTopologyQuery` via `render_topology::snapshot`. Split into
/// `topologyHash` (graph shape / MSAA / features) and `materializeHash` (adds allocated
/// sizes, SSAO divisors, and `backendEpoch`) so `CachedRenderGraph` can rebuild blueprint vs rematerialize passes separately. Used extents / dyn-res fraction are not hashed.
/// </summary>
struct RenderTopologySnapshot
{
	RenderScreenKind screenKind = RenderScreenKind::InGame;
	/// `RenderFeatures` bit mask.
	uint32_t features = 0;
	/// Active shadow-map passes when shadow mapping is enabled.
	uint32_t numShadowCascades = 0;
	bool sceneMsaa = false;
	bool swapchainMsaa = false;
	/// Color load op for the scene-to-swapchain blit pass (`Load` when backdrop is present).
	AttachmentLoadOp sceneBlitColorLoad = AttachmentLoadOp::Clear;
	/// Bumps when swapchain/surfaces are recreated; part of `materializeHash` only.
	uint64_t backendEpoch = 0;
	uint32_t drawableW = 0;
	uint32_t drawableH = 0;
	uint32_t sceneW = 0;
	uint32_t sceneH = 0;
	uint32_t shadowMapSize = 0;
	/// Allocated SSAO generate extent divisor (1 = full scene). Part of `materializeHash` only.
	uint32_t ssaoGenerateDivisor = 1;
	/// Allocated SSAO blur extent divisor (1 = full scene). Part of `materializeHash` only.
	uint32_t ssaoBlurDivisor = 1;

	bool operator==(const RenderTopologySnapshot& other) const
	{
		return std::tie(screenKind, features, numShadowCascades, sceneMsaa, swapchainMsaa,
			sceneBlitColorLoad, backendEpoch, drawableW, drawableH, sceneW, sceneH, shadowMapSize,
			ssaoGenerateDivisor, ssaoBlurDivisor)
			== std::tie(other.screenKind, other.features, other.numShadowCascades, other.sceneMsaa,
				other.swapchainMsaa, other.sceneBlitColorLoad, other.backendEpoch, other.drawableW,
				other.drawableH, other.sceneW, other.sceneH, other.shadowMapSize,
				other.ssaoGenerateDivisor, other.ssaoBlurDivisor);
	}

	/// Hash of screen kind, features, cascade count, MSAA flags, and scene blit load op.
	uint64_t topologyHash() const;
	/// `topologyHash` plus allocated dimensions, `backendEpoch`, `shadowMapSize`, and SSAO divisors.
	uint64_t materializeHash() const;
};

/// <summary>
/// Game-state facade used to build a `RenderTopologySnapshot` each frame.
///
/// Implemented by the active screen / game loop; keeps render_graph free of direct
/// calls into pie, loop, or loading code.
/// </summary>
class IRenderTopologyQuery
{
public:
	virtual ~IRenderTopologyQuery() = default;

	virtual bool isTitleScreenActive() const = 0;
	virtual ShadowMode shadowMode() const = 0;
	virtual bool hasBackdrop() const = 0;
	virtual bool headlessOrSkipDrawing() const = 0;
	virtual bool isLoadingScreenActive() const = 0;
	virtual bool isVideoPlaybackActive() const = 0;
	virtual uint32_t numDepthPasses() const = 0;
	virtual bool sceneMsaa() const = 0;
	virtual bool swapchainMsaa() const = 0;
	virtual uint64_t backendEpoch() const = 0;
	virtual std::pair<uint32_t, uint32_t> drawableDimensions() const = 0;
	virtual std::pair<uint32_t, uint32_t> sceneColorDimensions() const = 0;
	/// True when the backend has an upscale intermediate surface for the FSR1 pass pair.
	virtual bool sceneUpscaleActive() const = 0;
	/// True when the backend has the SMAA edge/weight surfaces for the SMAA pass chain.
	virtual bool smaaActive() const = 0;
	/// True when the backend has a scene-sized SMAA blend output surface for a
	/// following scaling pass to consume.
	virtual bool smaaIntermediateActive() const = 0;
	/// True when SSAO is enabled in config (surfaces may still be syncing).
	virtual bool ssaoEnabled() const = 0;
	/// True when blur is coarser than generate, so the downsample pass is in the graph.
	virtual bool ssaoDownsampleActive() const = 0;
	/// SSAO generate target divisor used for `MatchSceneDivided` allocation (1 = full scene).
	virtual uint32_t ssaoGenerateDivisor() const = 0;
	/// SSAO blur target divisor used for `MatchSceneDivided` allocation (1 = full scene).
	virtual uint32_t ssaoBlurDivisor() const = 0;
	/// True when deferred fog surfaces are enabled (surfaces may still be syncing).
	virtual bool fogEnabled() const = 0;
	virtual uint32_t shadowMapSize() const = 0;
	/// True when the in-game debug overlay pass slot should exist (persistent toggles only).
	virtual bool debugOverlaysEnabled() const = 0;
	/// True when in-game simulation/rendering is frozen but UI overlays still need to draw.
	virtual bool inGameWorldFrozen() const = 0;
};

namespace render_topology
{

/// Fill a `RenderTopologySnapshot` from current game/screen state.
RenderTopologySnapshot snapshot(const IRenderTopologyQuery& query);

} // namespace render_topology

/// Expected pass count for validation after blueprint materialize (per screen kind / features).
size_t expectedPassCount(const RenderTopologySnapshot& snapshot);

/// Global query instance used during normal gameplay frame setup.
IRenderTopologyQuery& getGameRenderTopologyQuery();

} // namespace gfx_api
