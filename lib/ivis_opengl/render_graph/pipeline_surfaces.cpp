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
/** @file pipeline_surfaces.cpp
 * Catalog table, resolvePipelineSurfaces, PipelineSurfaceStore ensure engine, and sync helpers.
 */

#include "pipeline_surfaces.h"

#include "gfx_api.h"

#include "lib/framework/wzapp.h"

namespace gfx_api
{
namespace
{

constexpr PipelineSurfaceCatalogEntry makeCatalogEntry(
	PipelineSurfaceUsage usage,
	SurfaceExtentPolicy extentPolicy,
	SurfaceSamplePolicy samplePolicy,
	SurfaceFormatClass formatClass,
	SurfaceGpuUsage gpuUsage,
	SurfaceArrayLayerPolicy arrayLayerPolicy,
	SurfaceEnablePolicy enablePolicy,
	SurfaceProvisionMode provisionMode,
	SurfaceStorageKind storageKind,
	SurfaceLifetimePolicy lifetimePolicy,
	PipelineSurfaceId formatCompanion = PipelineSurfaceId::Count)
{
	PipelineSurfaceCatalogEntry entry;
	entry.usage = usage;
	entry.extentPolicy = extentPolicy;
	entry.samplePolicy = samplePolicy;
	entry.formatClass = formatClass;
	entry.gpuUsage = gpuUsage;
	entry.arrayLayerPolicy = arrayLayerPolicy;
	entry.enablePolicy = enablePolicy;
	entry.provisionMode = provisionMode;
	entry.storageKind = storageKind;
	entry.lifetimePolicy = lifetimePolicy;
	entry.formatCompanion = formatCompanion;
	return entry;
}

const PipelineSurfaceCatalogTable PIPELINE_SURFACE_CATALOG = {{
	// SceneColor
	makeCatalogEntry(
		PipelineSurfaceUsage::ColorResolve,
		SurfaceExtentPolicy::MatchScene,
		SurfaceSamplePolicy::One,
		SurfaceFormatClass::SceneColor,
		SurfaceGpuUsage::ColorAttachment | SurfaceGpuUsage::Sampled,
		SurfaceArrayLayerPolicy::One,
		SurfaceEnablePolicy::Always,
		SurfaceProvisionMode::Allocate,
		SurfaceStorageKind::SampledColor2D,
		SurfaceLifetimePolicy::SwapchainBound),
	// SceneMSAAColor
	makeCatalogEntry(
		PipelineSurfaceUsage::ColorMSAA,
		SurfaceExtentPolicy::MatchScene,
		SurfaceSamplePolicy::SceneMsaa,
		SurfaceFormatClass::MatchCompanion,
		SurfaceGpuUsage::ColorAttachment,
		SurfaceArrayLayerPolicy::One,
		SurfaceEnablePolicy::SceneMsaaActive,
		SurfaceProvisionMode::Allocate,
		SurfaceStorageKind::MsaaColorAttachment,
		SurfaceLifetimePolicy::SwapchainBound,
		PipelineSurfaceId::SceneColor),
	// SceneDepth
	makeCatalogEntry(
		PipelineSurfaceUsage::DepthStencil,
		SurfaceExtentPolicy::MatchScene,
		SurfaceSamplePolicy::SceneMsaa,
		SurfaceFormatClass::DepthStencil,
		SurfaceGpuUsage::DepthStencilAttachment,
		SurfaceArrayLayerPolicy::One,
		SurfaceEnablePolicy::Always,
		SurfaceProvisionMode::Allocate,
		SurfaceStorageKind::DepthStencilAttachment,
		SurfaceLifetimePolicy::SwapchainBound),
	// ShadowMap
	makeCatalogEntry(
		PipelineSurfaceUsage::DepthOnly,
		SurfaceExtentPolicy::ShadowMapSquare,
		SurfaceSamplePolicy::One,
		SurfaceFormatClass::DepthSampled,
		SurfaceGpuUsage::DepthStencilAttachment | SurfaceGpuUsage::Sampled,
		SurfaceArrayLayerPolicy::ShadowCascadeCount,
		SurfaceEnablePolicy::ShadowCascadesNonZero,
		SurfaceProvisionMode::Allocate,
		SurfaceStorageKind::SampledDepthArray,
		SurfaceLifetimePolicy::Persistent),
	// SwapchainColor
	makeCatalogEntry(
		PipelineSurfaceUsage::SwapchainPresent,
		SurfaceExtentPolicy::MatchDrawable,
		SurfaceSamplePolicy::One,
		SurfaceFormatClass::PresentColor,
		SurfaceGpuUsage::ColorAttachment,
		SurfaceArrayLayerPolicy::One,
		SurfaceEnablePolicy::Always,
		SurfaceProvisionMode::WsiPresentColor,
		SurfaceStorageKind::None,
		SurfaceLifetimePolicy::SwapchainBound),
	// SwapchainMSAAColor — see header note: GL keeps this disabled via sync inputs / EnablePolicy.
	makeCatalogEntry(
		PipelineSurfaceUsage::ColorMSAA,
		SurfaceExtentPolicy::MatchDrawable,
		SurfaceSamplePolicy::SwapchainMsaa,
		SurfaceFormatClass::MatchCompanion,
		SurfaceGpuUsage::ColorAttachment,
		SurfaceArrayLayerPolicy::One,
		SurfaceEnablePolicy::SwapchainMsaaActive,
		SurfaceProvisionMode::Allocate,
		SurfaceStorageKind::MsaaColorAttachment,
		SurfaceLifetimePolicy::SwapchainBound,
		PipelineSurfaceId::SwapchainColor),
	// SwapchainDepth
	makeCatalogEntry(
		PipelineSurfaceUsage::DepthStencil,
		SurfaceExtentPolicy::MatchDrawable,
		SurfaceSamplePolicy::SwapchainMsaa,
		SurfaceFormatClass::DepthStencil,
		SurfaceGpuUsage::DepthStencilAttachment,
		SurfaceArrayLayerPolicy::One,
		SurfaceEnablePolicy::Always,
		SurfaceProvisionMode::WsiPresentDepth,
		SurfaceStorageKind::None,
		SurfaceLifetimePolicy::SwapchainBound),
}};

void validatePipelineSurfaceCatalog()
{
	size_t persistentCount = 0;
	for (size_t i = 0; i < PIPELINE_SURFACE_COUNT; ++i)
	{
		const PipelineSurfaceCatalogEntry& cat = PIPELINE_SURFACE_CATALOG[i];
		if (cat.provisionMode == SurfaceProvisionMode::Allocate)
		{
			ASSERT(cat.storageKind != SurfaceStorageKind::None,
				"Allocate catalog row %zu must set storageKind", i);
		}
		else
		{
			ASSERT(cat.storageKind == SurfaceStorageKind::None,
				"WSI catalog row %zu must use storageKind None", i);
		}

		SurfaceGpuUsage expected = SurfaceGpuUsage::None;
		switch (cat.storageKind)
		{
		case SurfaceStorageKind::SampledColor2D:
			expected = SurfaceGpuUsage::ColorAttachment | SurfaceGpuUsage::Sampled;
			break;
		case SurfaceStorageKind::MsaaColorAttachment:
			expected = SurfaceGpuUsage::ColorAttachment;
			break;
		case SurfaceStorageKind::DepthStencilAttachment:
			expected = SurfaceGpuUsage::DepthStencilAttachment;
			break;
		case SurfaceStorageKind::SampledDepthArray:
			expected = SurfaceGpuUsage::DepthStencilAttachment | SurfaceGpuUsage::Sampled;
			break;
		case SurfaceStorageKind::None:
			expected = (cat.provisionMode == SurfaceProvisionMode::WsiPresentDepth)
				? SurfaceGpuUsage::DepthStencilAttachment
				: SurfaceGpuUsage::ColorAttachment;
			break;
		}
		ASSERT(cat.gpuUsage == expected,
			"Catalog row %zu gpuUsage does not match storageKind", i);

		if (cat.lifetimePolicy == SurfaceLifetimePolicy::Persistent)
		{
			++persistentCount;
			ASSERT(static_cast<PipelineSurfaceId>(i) == PipelineSurfaceId::ShadowMap,
				"Only ShadowMap should be Persistent (row %zu)", i);
		}

		if (cat.formatClass == SurfaceFormatClass::MatchCompanion)
		{
			ASSERT(cat.formatCompanion != PipelineSurfaceId::Count,
				"MatchCompanion catalog row %zu requires formatCompanion", i);
			ASSERT(static_cast<size_t>(cat.formatCompanion) < i,
				"MatchCompanion companion index must precede dependent (row %zu)", i);
		}
	}
	ASSERT(persistentCount == 1, "Expected exactly one Persistent catalog surface, got %zu", persistentCount);
}

struct CatalogValidator
{
	CatalogValidator()
	{
		validatePipelineSurfaceCatalog();
	}
};
const CatalogValidator kCatalogValidator {};

bool evalEnablePolicy(SurfaceEnablePolicy policy, const PipelineSurfaceSyncInputs& inputs)
{
	switch (policy)
	{
	case SurfaceEnablePolicy::Always:
		return true;
	case SurfaceEnablePolicy::SceneMsaaActive:
		return inputs.sceneMsaaSamples > 1u;
	case SurfaceEnablePolicy::SwapchainMsaaActive:
		return inputs.swapchainMsaaSamples > 1u;
	case SurfaceEnablePolicy::ShadowCascadesNonZero:
		return inputs.numShadowCascades > 0u;
	}
	return false;
}

void resolveExtent(SurfaceExtentPolicy policy, const PipelineSurfaceSyncInputs& inputs,
	uint32_t& width, uint32_t& height)
{
	switch (policy)
	{
	case SurfaceExtentPolicy::MatchDrawable:
		width = inputs.drawableW;
		height = inputs.drawableH;
		break;
	case SurfaceExtentPolicy::MatchScene:
		width = inputs.sceneW;
		height = inputs.sceneH;
		break;
	case SurfaceExtentPolicy::ShadowMapSquare:
		width = inputs.shadowMapSize;
		height = inputs.shadowMapSize;
		break;
	}
}

uint32_t resolveSamples(SurfaceSamplePolicy policy, const PipelineSurfaceSyncInputs& inputs)
{
	switch (policy)
	{
	case SurfaceSamplePolicy::One:
		return 1u;
	case SurfaceSamplePolicy::SceneMsaa:
		return inputs.sceneMsaaSamples > 0u ? inputs.sceneMsaaSamples : 1u;
	case SurfaceSamplePolicy::SwapchainMsaa:
		return inputs.swapchainMsaaSamples > 0u ? inputs.swapchainMsaaSamples : 1u;
	}
	return 1u;
}

uint32_t resolveArrayLayers(SurfaceArrayLayerPolicy policy, const PipelineSurfaceSyncInputs& inputs)
{
	switch (policy)
	{
	case SurfaceArrayLayerPolicy::One:
		return 1u;
	case SurfaceArrayLayerPolicy::ShadowCascadeCount:
		return inputs.numShadowCascades;
	}
	return 1u;
}

pixel_format resolveFormat(SurfaceFormatClass formatClass, PipelineSurfaceId formatCompanion,
	const PipelineSurfaceSyncInputs& inputs, const SurfaceCapabilityHints& caps,
	const ResolvedSurfaceTable& out)
{
	switch (formatClass)
	{
	case SurfaceFormatClass::SceneColor:
		return caps.sceneColorFormat;
	case SurfaceFormatClass::DepthStencil:
		return caps.depthStencilFormat;
	case SurfaceFormatClass::DepthSampled:
		return caps.depthSampledFormat;
	case SurfaceFormatClass::PresentColor:
		return inputs.presentColorFormat;
	case SurfaceFormatClass::MatchCompanion:
		ASSERT(formatCompanion != PipelineSurfaceId::Count, "MatchCompanion requires formatCompanion");
		ASSERT(static_cast<size_t>(formatCompanion) < PIPELINE_SURFACE_COUNT,
			"Invalid formatCompanion id");
		return out[static_cast<size_t>(formatCompanion)].format;
	}
	return pixel_format::invalid;
}

} // namespace

const PipelineSurfaceCatalogTable& pipelineSurfaceCatalog()
{
	return PIPELINE_SURFACE_CATALOG;
}

const PipelineSurfaceCatalogEntry& pipelineSurfaceCatalogEntry(PipelineSurfaceId id)
{
	ASSERT(id != PipelineSurfaceId::Count, "Invalid pipeline surface ID");
	return PIPELINE_SURFACE_CATALOG[static_cast<size_t>(id)];
}

void collectPipelineSurfaceIdsByLifetime(SurfaceLifetimePolicy policy,
	std::vector<PipelineSurfaceId>& out)
{
	out.clear();
	for (size_t i = 0; i < PIPELINE_SURFACE_COUNT; ++i)
	{
		if (PIPELINE_SURFACE_CATALOG[i].lifetimePolicy == policy)
		{
			out.push_back(static_cast<PipelineSurfaceId>(i));
		}
	}
}

bool context::syncPipelineSurfaces()
{
	return ensurePipelineSurfaces(resolvePipelineSurfaces(pipelineSurfaceSyncInputs(), surfaceCapabilities()));
}

PipelineSurfaceUsage context::pipelineSurfaceUsage(PipelineSurfaceId id) const
{
	return pipelineSurfaceCatalogEntry(id).usage;
}

const ResolvedSurfaceSpec& context::resolvedPipelineSurface(PipelineSurfaceId id) const
{
	static const auto EMPTY_SPECS = [] {
		std::array<ResolvedSurfaceSpec, PIPELINE_SURFACE_COUNT> table {};
		for (size_t i = 0; i < PIPELINE_SURFACE_COUNT; ++i)
		{
			table[i].id = static_cast<PipelineSurfaceId>(i);
			table[i].enabled = false;
		}
		return table;
	}();
	ASSERT(id != PipelineSurfaceId::Count, "Invalid pipeline surface ID");
	return EMPTY_SPECS[static_cast<size_t>(id)];
}

optional<std::pair<uint32_t, uint32_t>> context::getPipelineSurfaceDimensions(PipelineSurfaceId id) const
{
	const ResolvedSurfaceSpec& spec = resolvedPipelineSurface(id);
	if (!spec.enabled || spec.width == 0 || spec.height == 0)
	{
		return nullopt;
	}
	return {{spec.width, spec.height}};
}

ResolvedSurfaceTable resolvePipelineSurfaces(const PipelineSurfaceSyncInputs& inputs,
	const SurfaceCapabilityHints& caps)
{
	ResolvedSurfaceTable out {};
	for (size_t i = 0; i < PIPELINE_SURFACE_COUNT; ++i)
	{
		const PipelineSurfaceId id = static_cast<PipelineSurfaceId>(i);
		const PipelineSurfaceCatalogEntry& cat = PIPELINE_SURFACE_CATALOG[i];
		ResolvedSurfaceSpec& spec = out[i];

		spec.id = id;
		spec.usage = cat.usage;
		spec.provisionMode = cat.provisionMode;
		spec.storageKind = cat.storageKind;
		spec.gpuUsage = cat.gpuUsage;
		spec.enabled = evalEnablePolicy(cat.enablePolicy, inputs);
		spec.preserveIfDisabled = false;

		uint32_t width = 0;
		uint32_t height = 0;
		resolveExtent(cat.extentPolicy, inputs, width, height);

		if (cat.extentPolicy == SurfaceExtentPolicy::ShadowMapSquare && inputs.shadowMapSize == 0u)
		{
			spec.enabled = false;
			spec.preserveIfDisabled = false;
			width = 0;
			height = 0;
		}
		else if (spec.enabled
			&& (cat.extentPolicy == SurfaceExtentPolicy::MatchDrawable
				|| cat.extentPolicy == SurfaceExtentPolicy::MatchScene)
			&& (width == 0u || height == 0u))
		{
			spec.enabled = false;
			spec.preserveIfDisabled = true;
			width = 0;
			height = 0;
		}

		spec.width = width;
		spec.height = height;
		spec.samples = resolveSamples(cat.samplePolicy, inputs);
		spec.arrayLayers = resolveArrayLayers(cat.arrayLayerPolicy, inputs);
		spec.format = resolveFormat(cat.formatClass, cat.formatCompanion, inputs, caps, out);
	}
	return out;
}

abstract_texture* PipelineSurfaceStore::get(PipelineSurfaceId id) const
{
	ASSERT(id != PipelineSurfaceId::Count, "Invalid pipeline surface ID");
	return _slots[static_cast<size_t>(id)].texture;
}

const ResolvedSurfaceSpec& PipelineSurfaceStore::spec(PipelineSurfaceId id) const
{
	ASSERT(id != PipelineSurfaceId::Count, "Invalid pipeline surface ID");
	return _slots[static_cast<size_t>(id)].spec;
}

bool PipelineSurfaceStore::has(PipelineSurfaceId id) const
{
	return get(id) != nullptr;
}

nonstd::optional<PipelineSurfaceId> PipelineSurfaceStore::find(abstract_texture* texture) const
{
	if (texture == nullptr)
	{
		return nonstd::nullopt;
	}
	for (size_t i = 0; i < PIPELINE_SURFACE_COUNT; ++i)
	{
		if (_slots[i].texture == texture)
		{
			return static_cast<PipelineSurfaceId>(i);
		}
	}
	return nonstd::nullopt;
}

PipelineSurfaceUsage PipelineSurfaceStore::usage(PipelineSurfaceId id) const
{
	ASSERT(id != PipelineSurfaceId::Count, "Invalid pipeline surface ID");
	const PipelineSurfaceSlotView& slot = _slots[static_cast<size_t>(id)];
	if (slot.spec.id == id)
	{
		return slot.spec.usage;
	}
	return pipelineSurfaceCatalogEntry(id).usage;
}

bool PipelineSurfaceStore::matches(const PipelineSurfaceSlotView& slot, const ResolvedSurfaceSpec& spec)
{
	if (slot.spec.enabled != spec.enabled
		|| slot.spec.format != spec.format
		|| slot.spec.samples != spec.samples
		|| slot.spec.width != spec.width
		|| slot.spec.height != spec.height
		|| slot.spec.arrayLayers != spec.arrayLayers
		|| slot.spec.gpuUsage != spec.gpuUsage
		|| slot.spec.provisionMode != spec.provisionMode
		|| slot.spec.storageKind != spec.storageKind)
	{
		return false;
	}
	if (spec.enabled)
	{
		return slot.texture != nullptr;
	}
	if (spec.preserveIfDisabled)
	{
		return true;
	}
	return slot.texture == nullptr;
}

bool PipelineSurfaceStore::ensure(const ResolvedSurfaceTable& resolved, SurfaceAllocator& alloc)
{
	bool changed = false;
	bool prepareCalled = false;
	auto ensurePrepared = [&]() {
		if (!prepareCalled)
		{
			alloc.prepareForSurfaceDestroy();
			prepareCalled = true;
		}
	};
	for (size_t i = 0; i < PIPELINE_SURFACE_COUNT; ++i)
	{
		const auto id = static_cast<PipelineSurfaceId>(i);
		const ResolvedSurfaceSpec& spec = resolved[i];
		PipelineSurfaceSlotView& slot = _slots[i];
		if (matches(slot, spec))
		{
			slot.spec = spec;
			continue;
		}
		if (!spec.enabled)
		{
			if (spec.preserveIfDisabled)
			{
				slot.spec = spec;
				continue;
			}
			if (slot.texture != nullptr)
			{
				ensurePrepared();
				alloc.destroy(id, slot.texture);
				slot.texture = nullptr;
				changed = true;
			}
			slot.spec = spec;
			continue;
		}
		if (slot.texture != nullptr)
		{
			ensurePrepared();
			alloc.destroy(id, slot.texture);
			slot.texture = nullptr;
			changed = true;
		}
		abstract_texture* created = nullptr;
		if (!alloc.create(id, spec, created) || created == nullptr)
		{
			slot.spec = ResolvedSurfaceSpec {};
			slot.texture = nullptr;
			// Partial store state; caller should treat as hard failure and re-sync / abort init.
			if (changed)
			{
				alloc.onChanged();
			}
			return false;
		}
		slot.texture = created;
		slot.spec = spec;
		changed = true;
	}
	if (changed)
	{
		alloc.onChanged();
	}
	return true;
}

void PipelineSurfaceStore::resetAll(SurfaceAllocator& alloc)
{
	alloc.prepareForSurfaceDestroy();
	for (size_t i = 0; i < PIPELINE_SURFACE_COUNT; ++i)
	{
		const auto id = static_cast<PipelineSurfaceId>(i);
		PipelineSurfaceSlotView& slot = _slots[i];
		alloc.destroy(id, slot.texture);
		slot.texture = nullptr;
		slot.spec = ResolvedSurfaceSpec {};
	}
	alloc.onChanged();
}

void PipelineSurfaceStore::resetIds(SurfaceAllocator& alloc, const PipelineSurfaceId* ids, size_t count)
{
	alloc.prepareForSurfaceDestroy();
	for (size_t i = 0; i < count; ++i)
	{
		const PipelineSurfaceId id = ids[i];
		ASSERT(id != PipelineSurfaceId::Count, "Invalid pipeline surface ID");
		PipelineSurfaceSlotView& slot = _slots[static_cast<size_t>(id)];
		alloc.destroy(id, slot.texture);
		slot.texture = nullptr;
		slot.spec = ResolvedSurfaceSpec {};
	}
	alloc.onChanged();
}

} // namespace gfx_api
