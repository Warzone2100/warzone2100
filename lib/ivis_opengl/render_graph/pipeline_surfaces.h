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
/** @file pipeline_surfaces.h
 * Pipeline surface architecture:
 * catalog policies -> resolvePipelineSurfaces -> PipelineSurfaceStore::ensure(SurfaceAllocator)
 * -> backend GPU ownership (VkSurfaceGpu / GlSurfaceGpu). Context exposes get / usage / resolved.
 *
 * Adding a PipelineSurfaceId:
 * 1. Append enum value (before Count) and catalog row (policies + lifetime).
 *    Post-effect rows use EnablePolicy::ScenePostEffect and `enableEffect`.
 * 2. Implement create/destroy for storageKind in VK and GL PipelineSurfaceAllocator.
 * 3. Wire blueprint attachments / topology if the surface is graph-visible.
 * 4. beginPass attachment binding if new cascade/view rules are needed.
 * 5. LifetimePolicy=Persistent only if it must survive swapchain recreate.
 *
 * Swapchain MSAA: enabled only when sync inputs report swapchainMsaaSamples > 1.
 * GL currently forces samples=1 / isSwapchainMSAAEnabled()==false - catalog row stays
 * for VK parity; surface remains disabled on GL via EnablePolicy::SwapchainMsaaActive.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

#include "../gfx_api_formats_def.h"
#include "scene_post_effect_id.h"

#include <nonstd/optional.hpp>

namespace gfx_api
{

struct abstract_texture;

/// Independent SSAO/fog/range-ring catalog requests. The scene prepass always provides
/// depth for forward transparents and adds effect-specific capabilities via `prepassNeeds(cfg)`.
struct SceneEffectSurfaces
{
	bool ssao = false;
	uint32_t ssaoGenerateDivisor = 1;
	uint32_t ssaoBlurDivisor = 1;
	bool fog = false;
	bool rangeRings = false;

	bool operator==(const SceneEffectSurfaces& other) const
	{
		return ssao == other.ssao
			&& ssaoGenerateDivisor == other.ssaoGenerateDivisor
			&& ssaoBlurDivisor == other.ssaoBlurDivisor
			&& fog == other.fog
			&& rangeRings == other.rangeRings;
	}

	bool enabled(ScenePostEffectId id) const
	{
		switch (id)
		{
		case ScenePostEffectId::Ssao:
			return ssao;
		case ScenePostEffectId::Fog:
			return fog;
		case ScenePostEffectId::RangeRings:
			return rangeRings;
		case ScenePostEffectId::Count:
			break;
		}
		return false;
	}
};

/// <summary>
/// Named persistent render targets shared across passes (scene, swapchain, shadow map).
///
/// Backends store live GPU objects behind PipelineSurfaceStore; context exposes them via
/// `getPipelineSurface` / usage / resolved-spec accessors. Blueprints refer to surfaces by id.
/// </summary>
enum class PipelineSurfaceId : uint8_t
{
	SceneColor,
	SceneMSAAColor,
	SceneDepth,
	/// Drawable-sized intermediate between the scene upscale and sharpen passes.
	UpscaledColor,
	/// Scene-sized SMAA edge detection output (RG edge flags).
	SmaaEdges,
	/// Scene-sized SMAA blending weights.
	SmaaWeights,
	/// Scene-sized SMAA neighborhood blend output, present only when a scaling
	/// pass consumes it (otherwise the blend writes the swapchain directly).
	SmaaColor,
	/// Scene-sized sampleable depth for SSAO and deferred fog.
	ScenePrepassDepth,
	/// Scene-sized view-space normals (RGB) + SSAO application weight (A).
	ScenePrepassNormals,
	/// SSAO generate output / final blurred AO when generate and blur share a size.
	SSAORaw,
	/// Horizontal SSAO blur intermediate.
	SSAOBlurH,
	/// Blur-resolution dest when blur is coarser than generate (downsample + blurV).
	SSAOBlurred,
	/// Scene-sized lit scene with AO applied; feeds fog / SMAA / blit / FSR.
	SSAOComposedColor,
	/// Scene-sized lit(+AO) scene with distance fog applied; feeds SMAA/blit/FSR.
	FogColor,
	/// Packed per-type range-ring union field (RGB = sensor / weapon / min-range).
	RangeRingSdf,
	/// Scratch depth for the range-ring cone Z-test (not sampled).
	RangeRingSdfDepth,
	/// Scene-sized lit scene with range-ring overlay applied; feeds SMAA/blit/FSR.
	RangeRingColor,
	ShadowMap,
	SwapchainColor,
	SwapchainMSAAColor,
	SwapchainDepth,
	/// Array size and invalid id sentinel.
	Count
};

/// <summary>
/// Role of a pipeline surface (drives format defaults and layout/barrier hints).
/// </summary>
enum class PipelineSurfaceUsage : uint8_t
{
	ColorResolve,
	ColorMSAA,
	DepthStencil,
	DepthOnly,
	/// Swapchain / default framebuffer presentable color (not shader-sampled).
	SwapchainPresent,
};

inline bool isDepthUsage(PipelineSurfaceUsage usage)
{
	return usage == PipelineSurfaceUsage::DepthStencil
		|| usage == PipelineSurfaceUsage::DepthOnly;
}

/// How resolve derives width/height from sync inputs.
enum class SurfaceExtentPolicy : uint8_t
{
	MatchDrawable,
	MatchScene,
	/// sceneW/H divided by `PipelineSurfaceCatalogEntry::extentDivisorSource`.
	MatchSceneDivided,
	ShadowMapSquare,
};

/// Which sync-input divisor `MatchSceneDivided` uses.
enum class SurfaceExtentDivisorSource : uint8_t
{
	None,
	SsaoGenerate,
	SsaoBlur,
};

inline uint32_t divideSurfaceExtent(uint32_t size, uint32_t divisor)
{
	if (size == 0u)
	{
		return 0u;
	}
	if (divisor <= 1u)
	{
		return size; // identical to MatchScene - required for full-res SSAO
	}
	return std::max(size / divisor, 2u);
}

/// True when the resolved blur RT is strictly smaller than the generate RT.
/// Compare extents, not raw divisors: tiny scenes can clamp both to 2.
inline bool ssaoBlurIsCoarser(uint32_t sceneW, uint32_t sceneH, uint32_t generateDivisor, uint32_t blurDivisor)
{
	return divideSurfaceExtent(sceneW, generateDivisor) > divideSurfaceExtent(sceneW, blurDivisor)
		|| divideSurfaceExtent(sceneH, generateDivisor) > divideSurfaceExtent(sceneH, blurDivisor);
}

/// How resolve derives MSAA sample count from sync inputs.
enum class SurfaceSamplePolicy : uint8_t
{
	One,
	SceneMsaa,
	SwapchainMsaa,
};

/// Which format source resolve uses (HW capability slot, present format, or companion).
enum class SurfaceFormatClass : uint8_t
{
	SceneColor,
	DepthStencil,
	DepthSampled,
	PresentColor,
	MatchCompanion,
	/// Fixed two channel color (backends may widen where unsupported).
	FixedRG8,
	/// Fixed four channel color, independent of the negotiated scene format.
	FixedRGBA8,
	/// Fixed FORMAT_R8_UNORM (e.g. SSAO occlusion buffers).
	SingleChannelR8,
};

/// Abstract GPU usage flags the backend must honor when allocating.
enum class SurfaceGpuUsage : uint32_t
{
	None = 0,
	ColorAttachment = 1u << 0,
	DepthStencilAttachment = 1u << 1,
	Sampled = 1u << 2,
};

constexpr SurfaceGpuUsage operator|(SurfaceGpuUsage a, SurfaceGpuUsage b)
{
	return static_cast<SurfaceGpuUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr SurfaceGpuUsage operator&(SurfaceGpuUsage a, SurfaceGpuUsage b)
{
	return static_cast<SurfaceGpuUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr bool hasFlag(SurfaceGpuUsage value, SurfaceGpuUsage flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0u;
}

/// How resolve derives array layer count from sync inputs.
enum class SurfaceArrayLayerPolicy : uint8_t
{
	One,
	ShadowCascadeCount,
};

/// When resolve marks a catalog surface enabled for the current inputs.
enum class SurfaceEnablePolicy : uint8_t
{
	Always,
	SceneMsaaActive,
	SwapchainMsaaActive,
	ShadowCascadesNonZero,
	/// FSR 1.0 upscaling selected and the scene renders below the drawable size.
	FsrUpscaleActive,
	/// SMAA post processing enabled.
	SmaaActive,
	/// SMAA enabled and its blend output feeds a scaling pass instead of the swapchain.
	SmaaIntermediateActive,
	/// Catalog row owned by a `ScenePostEffectId` (`enableEffect`).
	ScenePostEffect,
	/// Extra blur-resolution buffer when blur is coarser than generate.
	SsaoSeparateBlurBuffers,
	/// Scene prepass depth/normals when `prepassNeeds` is not None.
	ScenePrepassActive,
};

/// How the backend materializes the surface (allocate vs WSI import).
enum class SurfaceProvisionMode : uint8_t
{
	Allocate,
	WsiPresentColor,
	WsiPresentDepth,
};

/// Which GPU constructor the backend allocator uses when provisionMode == Allocate.
enum class SurfaceStorageKind : uint8_t
{
	None,                    // WSI rows; must be None if provisionMode != Allocate
	SampledColor2D,          // VkRenderedImage / GL color texture
	MsaaColorAttachment,     // Vk MSAA color image / GL MSAA color renderbuffer
	DepthStencilAttachment,  // Vk depth image / GL depth-stencil renderbuffer
	SampledDepthArray,       // VkDepthMapImage + cascade views / GL depth array + compare
	SampledDepth2D,          // 1x sampleable depth texture (attachment + sampled)
};

/// Whether a surface survives swapchain teardown/recreate.
enum class SurfaceLifetimePolicy : uint8_t
{
	SwapchainBound, // reset with swapchain (scene + present surfaces)
	Persistent,    // survives swapchain recreate (ShadowMap)
};

/// Static policy row for one PipelineSurfaceId (never holds runtime sizes/formats).
struct PipelineSurfaceCatalogEntry
{
	PipelineSurfaceUsage usage = PipelineSurfaceUsage::ColorResolve;
	SurfaceExtentPolicy extentPolicy = SurfaceExtentPolicy::MatchDrawable;
	SurfaceSamplePolicy samplePolicy = SurfaceSamplePolicy::One;
	SurfaceFormatClass formatClass = SurfaceFormatClass::SceneColor;
	SurfaceGpuUsage gpuUsage = SurfaceGpuUsage::None;
	SurfaceArrayLayerPolicy arrayLayerPolicy = SurfaceArrayLayerPolicy::One;
	SurfaceEnablePolicy enablePolicy = SurfaceEnablePolicy::Always;
	SurfaceProvisionMode provisionMode = SurfaceProvisionMode::Allocate;
	SurfaceStorageKind storageKind = SurfaceStorageKind::None;
	SurfaceLifetimePolicy lifetimePolicy = SurfaceLifetimePolicy::SwapchainBound;
	PipelineSurfaceId formatCompanion = PipelineSurfaceId::Count; // MatchCompanion only
	SurfaceExtentDivisorSource extentDivisorSource = SurfaceExtentDivisorSource::None;
	/// Used when `enablePolicy == ScenePostEffect`.
	ScenePostEffectId enableEffect = ScenePostEffectId::Count;
};

/// Runtime identity produced by resolve; consumed by ensure / matches.
struct ResolvedSurfaceSpec
{
	PipelineSurfaceId id = PipelineSurfaceId::Count;
	PipelineSurfaceUsage usage = PipelineSurfaceUsage::ColorResolve;
	SurfaceProvisionMode provisionMode = SurfaceProvisionMode::Allocate;
	SurfaceStorageKind storageKind = SurfaceStorageKind::None;
	SurfaceGpuUsage gpuUsage = SurfaceGpuUsage::None;
	pixel_format format = pixel_format::invalid;
	uint32_t samples = 1;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t arrayLayers = 1;
	bool enabled = false;
	bool preserveIfDisabled = false; // keep GPU objects when disabled due to zero extent
};

/// Per-sync sizes, sample counts, cascade count, and present format from the backend.
struct PipelineSurfaceSyncInputs
{
	uint32_t drawableW = 0;
	uint32_t drawableH = 0;
	uint32_t sceneW = 0;
	uint32_t sceneH = 0;
	uint32_t shadowMapSize = 0;
	uint32_t numShadowCascades = 0;
	uint32_t sceneMsaaSamples = 1;
	uint32_t swapchainMsaaSamples = 1;
	pixel_format presentColorFormat = pixel_format::FORMAT_RGBA8_UNORM_PACK8;
	bool fsr1SceneUpscale = false;
	/// Dynamic resolution keeps scene-sized intermediates alive even at a 1:1 scene size.
	bool sceneDynamicResolution = false;
	bool smaa = false;
	/// Allocated scene-effect catalog request (enable flags + SSAO divisors).
	SceneEffectSurfaces effects;
	/// Baseline transparent-depth need plus enabled post-effect capabilities (`prepassNeeds(effects)`).
	PrepassNeed prepassNeeds = PrepassNeed::None;
};

/// Backend HW-negotiated formats for each SurfaceFormatClass capability slot.
struct SurfaceCapabilityHints
{
	pixel_format sceneColorFormat = pixel_format::FORMAT_RGBA8_UNORM_PACK8;
	pixel_format depthStencilFormat = pixel_format::FORMAT_D24_UNORM_S8;
	pixel_format depthSampledFormat = pixel_format::FORMAT_D24_UNORM_S8;
};

constexpr size_t PIPELINE_SURFACE_COUNT = static_cast<size_t>(PipelineSurfaceId::Count);

using PipelineSurfaceCatalogTable = std::array<PipelineSurfaceCatalogEntry, PIPELINE_SURFACE_COUNT>;
using ResolvedSurfaceTable = std::array<ResolvedSurfaceSpec, PIPELINE_SURFACE_COUNT>;

/// Catalog policy row for `id`.
const PipelineSurfaceCatalogEntry& pipelineSurfaceCatalogEntry(PipelineSurfaceId id);
/// Full static catalog table.
const PipelineSurfaceCatalogTable& pipelineSurfaceCatalog();

/// Derive width/height from catalog extent policy and sync inputs.
/// Pass allocated sceneW/H for GPU image size, or used sceneW/H (dyn-res) for the execute viewport.
void resolveExtent(const PipelineSurfaceCatalogEntry& cat, const PipelineSurfaceSyncInputs& inputs,
	uint32_t& width, uint32_t& height);

inline std::pair<uint32_t, uint32_t> resolveCatalogExtent(PipelineSurfaceId id,
	const PipelineSurfaceSyncInputs& inputs)
{
	uint32_t w = 0;
	uint32_t h = 0;
	resolveExtent(pipelineSurfaceCatalogEntry(id), inputs, w, h);
	return {w, h};
}

/// Resolve catalog policies against sync inputs and HW capability hints.
ResolvedSurfaceTable resolvePipelineSurfaces(const PipelineSurfaceSyncInputs& inputs,
	const SurfaceCapabilityHints& caps);

/// Append all catalog IDs with the given lifetime policy (stable enum order).
void collectPipelineSurfaceIdsByLifetime(SurfaceLifetimePolicy policy,
	std::vector<PipelineSurfaceId>& out);

/// Per-id view held by `PipelineSurfaceStore` (non-owning texture pointer).
struct PipelineSurfaceSlotView
{
	ResolvedSurfaceSpec spec {};
	abstract_texture* texture = nullptr;
};

/// Backend port: GPU create/destroy only. Must not mutate the store directly.
struct SurfaceAllocator
{
	virtual ~SurfaceAllocator() = default;
	/// Allocate or import; on success set outTexture non-null. On failure leave outTexture null.
	virtual bool create(PipelineSurfaceId id, const ResolvedSurfaceSpec& spec,
		abstract_texture*& outTexture) = 0;
	/// Destroy GPU objects for id. texture may be null (no-op / clear orphans). Store clears its pointer after.
	virtual void destroy(PipelineSurfaceId id, abstract_texture* texture) = 0;
	/// Called once before any destroy in reset*/ensure mutate paths.
	/// Must clear pass-resource caches and *defer* framebuffer deletes (VK).
	/// Do not flush here - runtime ensure() is not GPU-idle. Hard-reset paths
	/// flush deferred FBs after idle (reset wrappers / swapchain teardown).
	virtual void prepareForSurfaceDestroy() = 0;
	/// Called once if any slot changed (create/destroy). Clear FBO/FB caches + bump epoch.
	virtual void onChanged() = 0;
};

/// Shared resolved-state table + ensure engine (texture ownership stays in the backend allocator).
///
/// Teardown contract:
/// - Store order: prepareForSurfaceDestroy() -> destroy GPU objects -> onChanged()
/// - prepare clears pass FB/FBO caches and defers FB deletes (VK); it does not flush
/// - Runtime ensure (not GPU-idle): defer FBs + defer attachment view/image/memory
/// - Hard reset (after waitForAllIdle): same store order; reset wrappers / swapchain
///   teardown flush deferred FBs; attachment images defer while buffering is live and
///   are freed on buffering destroy, or destroyed immediately once buffering is gone
/// Framebuffers/FBOs must not outlive the image views/attachments they reference.
class PipelineSurfaceStore
{
public:
	abstract_texture* get(PipelineSurfaceId id) const;
	const ResolvedSurfaceSpec& spec(PipelineSurfaceId id) const;
	bool has(PipelineSurfaceId id) const;
	PipelineSurfaceUsage usage(PipelineSurfaceId id) const;

	bool ensure(const ResolvedSurfaceTable& resolved, SurfaceAllocator& alloc);
	void resetAll(SurfaceAllocator& alloc);
	void resetIds(SurfaceAllocator& alloc, const PipelineSurfaceId* ids, size_t count);
	void resetIds(SurfaceAllocator& alloc, std::initializer_list<PipelineSurfaceId> ids)
	{
		resetIds(alloc, ids.begin(), ids.size());
	}
	void resetIds(SurfaceAllocator& alloc, const std::vector<PipelineSurfaceId>& ids)
	{
		resetIds(alloc, ids.data(), ids.size());
	}

private:
	std::array<PipelineSurfaceSlotView, PIPELINE_SURFACE_COUNT> _slots {};

	static bool matches(const PipelineSurfaceSlotView& slot, const ResolvedSurfaceSpec& spec);
};

} // namespace gfx_api
