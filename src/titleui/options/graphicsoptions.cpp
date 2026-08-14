// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
 *  Graphics Options form
 */

#include "optionsforms.h"

#include "../../warzoneconfig.h"
#include "../../terrain.h"
#include "../../atmos.h"
#include "../../display.h"
#include "../../display3d.h"
#include "../../scene_effect_surfaces.h"
#include "lib/ivis_opengl/piestate.h"
#include "../../texture.h"
#include "lib/framework/wzapp.h"

// MARK: -

OptionInfo::AvailabilityResult TerrainShadingQualityAvailable(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = (getTerrainShaderQuality() == TerrainShaderQuality::NORMAL_MAPPING);
	result.localizedUnavailabilityReason = _("Terrain Shading Quality only applies when using terrain appearance: Remastered (HQ)");
	return result;
}

OptionInfo::AvailabilityResult TerrainDetailAvailable(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = (getTerrainShaderQuality() != TerrainShaderQuality::CLASSIC);
	result.localizedUnavailabilityReason = _("Terrain Detail is not available when using terrain appearance: Classic");
	return result;
}

OptionInfo::AvailabilityResult ShadowsEnabled(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = getDrawShadows();
	result.localizedUnavailabilityReason = _("Shadows are not enabled.");
	return result;
}

OptionInfo::AvailabilityResult SupportsShadowMapping(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	const bool bShadowMappingSupported = pie_supportsShadowMapping().value_or(false);
	result.available = bShadowMappingSupported;
	result.localizedUnavailabilityReason = _("Shadow mapping not available on this system.");
	return result;
}

OptionInfo::AvailabilityResult PerPixelLightingAvailable(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	if (pie_PointLightPerPixelShaderUnavailable())
	{
		result.available = false;
		result.localizedUnavailabilityReason = _("Advanced lighting is not supported by this graphics driver");
		return result;
	}
	result.available = (getTerrainShaderQuality() == TerrainShaderQuality::NORMAL_MAPPING);
	result.localizedUnavailabilityReason = _("Advanced lighting is only available when using terrain appearance: Remastered (HQ)");
	return result;
}

OptionInfo::AvailabilityResult UpscalingSharpnessAvailable(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = (war_getSceneUpscalingMode() == SCENE_UPSCALING_MODE::FSR1);
	result.localizedUnavailabilityReason = _("Sharpness applies when Upscaling is set to FSR 1.0.");
	return result;
}

// MARK: -

std::shared_ptr<OptionsForm> makeGraphicsOptionsForm()
{
	auto result = OptionsForm::make();

	// Terrain:
	result->addSection(OptionsSection(N_("Terrain"), ""), true);
	{
		auto optionInfo = OptionInfo("gfx.terrainAppearance", N_("Terrain Appearance"), "");
		auto valueChanger = OptionsDropdown<TerrainShaderQuality>::make(
			[]() {
				OptionChoices<TerrainShaderQuality> result;
				result.choices = {
					{ WzString::fromUtf8(to_display_string(TerrainShaderQuality::CLASSIC)), _("The classic terrain appearance from the initial release. Low-resolution, pixel-art textures."), TerrainShaderQuality::CLASSIC, !isSupportedTerrainShaderQualityOption(TerrainShaderQuality::CLASSIC) },
					{ WzString::fromUtf8(to_display_string(TerrainShaderQuality::MEDIUM)), _("The normal terrain appearance used as the default for more than a decade. Higher-resolution, recreated textures."), TerrainShaderQuality::MEDIUM, !isSupportedTerrainShaderQualityOption(TerrainShaderQuality::MEDIUM) },
					{ WzString::fromUtf8(to_display_string(TerrainShaderQuality::NORMAL_MAPPING)), _("The remastered, high-quality terrain appearance. High-resolution, with modern bump + specular mapping. Requires more memory and a better GPU."), TerrainShaderQuality::NORMAL_MAPPING, !isSupportedTerrainShaderQualityOption(TerrainShaderQuality::NORMAL_MAPPING) }
				};
				auto currentValue = getTerrainShaderQuality();
				result.currentIdx = (currentValue >= 0) ? static_cast<size_t>(currentValue) : 1;
				return result;
			},
			[](const auto& newValue) -> bool {
				if (!isSupportedTerrainShaderQualityOption(newValue))
				{
					return false;
				}
				if (!setTerrainShaderQuality(newValue))
				{
					debug(LOG_ERROR, "Failed to set terrain shader quality: %s", to_display_string(newValue).c_str());
					return false;
				}
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.terrainShadingQuality", N_("Terrain Shading Quality"), N_("The quality of terrain shading (normal maps, specular highlights, etc). Higher settings improve terrain details at the expense of performance and memory usage."));
		optionInfo.addAvailabilityCondition(IsNotInGame);
		optionInfo.addAvailabilityCondition(TerrainShadingQualityAvailable);
		auto valueChanger = OptionsDropdown<int32_t>::make(
			[]() {
				OptionChoices<int32_t> result;
				result.choices = {
					{ _("Medium Quality"), "", 512 },
					{ _("High Quality"), "", 1024 },
				};
				result.setCurrentIdxForValue(getTerrainMappingTexturesMaxSize());
				return result;
			},
			[](const auto& newValue) -> bool {
				if (!setTerrainMappingTexturesMaxSize(newValue))
				{
					debug(LOG_ERROR, "Failed to set terrain mapping texture quality: %d", newValue);
					return false;
				}
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.terrainDetail", N_("Terrain Detail"), N_("The geometric detail of the terrain. Higher settings smooth the terrain mesh, at the expense of memory usage and map load time."));
		optionInfo.addAvailabilityCondition(TerrainDetailAvailable);
		auto valueChanger = OptionsDropdown<int32_t>::make(
			[]() {
				OptionChoices<int32_t> result;
				result.choices = {
					{ _("Off"), _("The classic, low-poly terrain mesh."), 1 },
					{ _("Medium"), _("Smoothed terrain mesh."), 2 },
					{ _("High"), _("Smoothed terrain mesh, higher detail."), 3 },
					{ _("Ultra"), _("Smoothed terrain mesh, highest detail. Uses more memory."), 4 },
				};
				if (!result.setCurrentIdxForValue(getTerrainMeshSubdivision()))
				{
					result.currentIdx = 0;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				if (!setTerrainMeshSubdivision(newValue))
				{
					debug(LOG_ERROR, "Failed to set terrain detail: %d", newValue);
					return false;
				}
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Shadows:
	result->addSection(OptionsSection(N_("Shadows"), ""), true);
	{
		auto optionInfo = OptionInfo("gfx.drawShadows", N_("Draw Shadows"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.currentIdx = (getDrawShadows()) ? 1 : 0;
				return result;
			},
			[](const auto& newValue) -> bool {
				setDrawShadows(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.drawShadowsTerrain", N_("Terrain Shadows"), N_("Whether terrain casts shadows."));
		optionInfo.addAvailabilityCondition(ShadowsEnabled);
		optionInfo.addAvailabilityCondition(SupportsShadowMapping);
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(getDrawTerrainShadows());
				return result;
			},
			[](const auto& newValue) -> bool {
				setDrawTerrainShadows(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true, 1);
	}
	{
		auto optionInfo = OptionInfo("gfx.shadowResolution", N_("Shadow Resolution"), N_("The internal resolution used to render shadow maps. Higher values improve shadow appearance at the expense of performance and memory usage."));
		optionInfo.addAvailabilityCondition(ShadowsEnabled);
		optionInfo.addAvailabilityCondition(SupportsShadowMapping);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ WzString::fromUtf8(_("Normal")) + " (2048)", "", 2048 },
					{ WzString::fromUtf8(_("High")) + " (4096)", "", 4096 },
				};
				uint32_t currValue = pie_getShadowMapResolution();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({WzString::format("(Custom: %" PRIu32 ")", currValue), "", currValue});
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newResolution) -> bool {
				if (!pie_supportsShadowMapping().value_or(false))
				{
					return false;
				}
				if (!pie_setShadowMapResolution(newResolution))
				{
					debug(LOG_ERROR, "Failed to set map resolution: %" PRIu32, newResolution);
					return false;
				}
				war_setShadowMapResolution(newResolution);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.shadowFiltering", N_("Shadow Filtering"), N_("How much filtering / smoothing is applied to shadow appearance. Higher settings smooth shadows more at the expense of performance."));
		optionInfo.addAvailabilityCondition(ShadowsEnabled);
		optionInfo.addAvailabilityCondition(SupportsShadowMapping);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ _("Low"), "", 3 },
					{ _("High"), "", 5 },
					{ _("Ultra"), "", 7 },
				};
				uint32_t currValue = gfx_api::context::get().getShadowConstants().shadowFilterSize;
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({WzString::format("(Custom: %" PRIu32 ")", currValue), "", currValue});
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newFilterSize) -> bool {
				if (!pie_supportsShadowMapping().value_or(false))
				{
					return false;
				}
				auto shadowConstants = gfx_api::context::get().getShadowConstants();
				shadowConstants.shadowFilterSize = newFilterSize;
				if (!gfx_api::context::get().setShadowConstants(shadowConstants))
				{
					debug(LOG_ERROR, "Failed to set shadow filter size: %" PRIu32, newFilterSize);
					return false;
				}
				war_setShadowFilterSize(newFilterSize);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Lighting:
	result->addSection(OptionsSection(N_("Lighting"), ""), true);
	{
		auto optionInfo = OptionInfo("gfx.pointLights", N_("Point Lights"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		optionInfo.addAvailabilityCondition(PerPixelLightingAvailable);
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Lightmap"), _("Use a classic lightmap to calculate point lights impacting objects and tiles. (Classic & Normal default. Fastest performance.)"), false },
					{ _("Per-Pixel (HQ)"), _("Use advanced per-pixel point-lighting to improve visuals. (May impact performance. Requires a more powerful GPU.)"), true, (getTerrainShaderQuality() != TerrainShaderQuality::NORMAL_MAPPING) },
				};
				result.setCurrentIdxForValue(war_getPointLightPerPixelLighting());
				return result;
			},
			[](const auto& newValue) -> bool {
				if (getTerrainShaderQuality() != TerrainShaderQuality::NORMAL_MAPPING)
				{
					// point light per pixel lighting is only supported in normal_mapping mode
					if (newValue)
					{
						return false;
					}
				}
				auto shadowConstants = gfx_api::context::get().getShadowConstants();
				shadowConstants.isPointLightPerPixelEnabled = newValue;
				if (!gfx_api::context::get().setShadowConstants(shadowConstants))
				{
					debug(LOG_ERROR, "Failed to set per pixel point lighting value: %d", (int)newValue);
					return false;
				}
				war_setPointLightPerPixelLighting(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Effects:
	result->addSection(OptionsSection(N_("Effects"), ""), true);
	{
		auto optionInfo = OptionInfo("gfx.fog", N_("Fog"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(pie_GetFogEnabled());
				return result;
			},
			[](const auto& newValue) -> bool {
				if (pie_GetFogEnabled())
				{
					pie_SetFogStatus(false);
					pie_EnableFog(false);
				}
				else
				{
					pie_EnableFog(true);
				}
				applySceneEffectSurfaces();
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.ssao", N_("SSAO"), N_("Screen-space ambient occlusion. Darkens creases and contact areas for stronger depth cues. May impact performance."));
		auto valueChanger = OptionsDropdown<SSAO_MODE>::make(
			[]() {
				OptionChoices<SSAO_MODE> result;
				result.choices = {
					{ _("Off"), "", SSAO_MODE::OFF },
					{ _("Low"), "", SSAO_MODE::LOW },
					{ _("Normal"), "", SSAO_MODE::NORMAL },
					{ _("High"), "", SSAO_MODE::HIGH },
					{ _("Ultra"), "", SSAO_MODE::ULTRA },
				};
				result.setCurrentIdxForValue(war_getSsaoMode());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setSsaoMode(newValue);
				applySceneEffectSurfaces();
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.screenShake", N_("Screen Shake"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(getShakeStatus());
				return result;
			},
			[](const auto& newValue) -> bool {
				setShakeStatus(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.weather", N_("Weather"), N_("Show weather effects, like rain and snow, on maps that use them."));
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(atmosGetWeatherEnabled());
				return result;
			},
			[](const auto& newValue) -> bool {
				atmosSetWeatherEnabled(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Textures:
	result->addSection(OptionsSection(N_("Textures"), ""), true);
	{
		auto optionInfo = OptionInfo("gfx.textureSizeLimit", N_("Texture Size Limit"), N_("Textures larger than the configured limit will be downsampled. Unless running on a system with limited available memory, this should usually be set to the maximum value."));
		optionInfo.addAvailabilityCondition(IsNotInGame);
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ "512", "", 512 },
					{ "1024", "", 1024 },
					{ "2048", "", 2048 },
//					{ "4096", "", 4096 },
				};
				uint32_t currValue = static_cast<uint32_t>(getTextureSize());
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({WzString::format("(Custom: %" PRIu32 ")", currValue), "", currValue});
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newTexSize) -> bool {
				setTextureSize(newTexSize);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		// TRANSLATORS: "LOD" = "Level of Detail" - this setting is used to describe how level of detail (in textures) is preserved as distance increases (examples: "Default", "High", etc)
		std::string lodDistanceString = N_("LOD Distance");
		auto optionInfo = OptionInfo("gfx.lodDistance", lodDistanceString.c_str(), N_("How level of detail (in textures) is preserved as distance increases."));
		optionInfo.addAvailabilityCondition(IsNotInGame);
		optionInfo.setRequiresRestart(true);
		auto valueChanger = OptionsDropdown<int>::make(
			[]() {
				OptionChoices<int> result;
				result.choices = {
					{ _("Default"), _("The system default (no supplied bias, may result in less sharp visuals)."), 0 }, // the *system* default (no supplied bias)
					{ _("High"), _("Use LOD Distance bias to increase visual sharpness."), WZ_LODDISTANCEPERCENTAGE_HIGH },
				};
				int currValue = war_getLODDistanceBiasPercentage();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({WzString::format("(Custom: %" PRIu32 ")", currValue), "", currValue});
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setLODDistanceBiasPercentage(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Advanced:
	result->addSection(OptionsSection(N_("Advanced"), ""), true);
	{
		auto optionInfo = OptionInfo("gfx.backend", N_("Graphics Backend"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		optionInfo.setRequiresRestart(true);
		auto valueChanger = OptionsDropdown<video_backend>::make(
			[]() {
				const std::vector<video_backend> availableBackends = wzAvailableGfxBackends();
				OptionChoices<video_backend> result;
				for (const auto& backend : availableBackends)
				{
					result.choices.push_back({ WzString::fromUtf8(to_display_string(backend)), "", backend });
				}
				auto currValue = war_getGfxBackend();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// somehow the current graphics backend isn't in the available list... add it
					result.choices.push_back({ WzString::fromUtf8(to_display_string(currValue)), "", currValue });
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setGfxBackend(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.renderResolution", N_("Render Resolution"), N_("The percentage of the display resolution used to render the 3D world, which is then upscaled to your display. Lower values improve performance at the cost of sharpness. The interface is always rendered at full resolution."));
		auto valueChanger = OptionsDropdown<uint32_t>::make(
			[]() {
				OptionChoices<uint32_t> result;
				result.choices = {
					{ _("Native"), _("Render the 3D world at the full display resolution."), 100 },
					{ WzString::fromUtf8("75%"), "", 75 },
					{ WzString::fromUtf8("67%"), "", 67 },
					{ WzString::fromUtf8("59%"), "", 59 },
					{ WzString::fromUtf8("50%"), "", 50 },
					{ WzString::fromUtf8("33%"), "", 33 },
					{ _("Dynamic"), _("Automatically adjusts render resolution to keep GPU frame times within the display's budget. Requires GPU timing support."), 0 },
				};
				uint32_t currValue = war_getRenderResolutionPercent();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({WzString::format("(Custom: %" PRIu32 "%%)", currValue), "", currValue});
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				// dynamic resolution keeps the scene targets at native size,
				// the controller drives the per frame fraction from there
				const uint32_t scalePercent = (newValue == 0) ? 100 : newValue;
				if (!gfx_api::context::get().setSceneRenderScale(scalePercent))
				{
					debug(LOG_ERROR, "Failed to set render resolution: %" PRIu32 "%%", scalePercent);
					return false;
				}
				war_setRenderResolutionPercent(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.upscaling", N_("Upscaling"), N_("How the 3D world is upscaled to your display when Render Resolution is below Native. FSR 1.0 (AMD FidelityFX Super Resolution) preserves edges and detail better than bilinear filtering."));
		auto valueChanger = OptionsDropdown<SCENE_UPSCALING_MODE>::make(
			[]() {
				OptionChoices<SCENE_UPSCALING_MODE> result;
				result.choices = {
					{ _("Bilinear"), _("Simple bilinear filtering."), SCENE_UPSCALING_MODE::BILINEAR },
					{ WzString::fromUtf8("FSR 1.0"), _("AMD FidelityFX Super Resolution 1.0, an edge adaptive upscaler with sharpening. Works best combined with antialiasing."), SCENE_UPSCALING_MODE::FSR1 },
				};
				result.setCurrentIdxForValue(war_getSceneUpscalingMode());
				return result;
			},
			[](const auto& newValue) -> bool {
				auto gfxMode = (newValue == SCENE_UPSCALING_MODE::FSR1)
					? gfx_api::context::scene_upscaling_mode::fsr1
					: gfx_api::context::scene_upscaling_mode::bilinear;
				if (!gfx_api::context::get().setSceneUpscalingMode(gfxMode))
				{
					debug(LOG_ERROR, "Failed to set upscaling mode");
					return false;
				}
				war_setSceneUpscalingMode(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true, 1);
	}
	{
		auto optionInfo = OptionInfo("gfx.upscalingSharpness", N_("Upscaling Sharpness"), N_("How much sharpening FSR 1.0 applies to the upscaled image."));
		optionInfo.addAvailabilityCondition(UpscalingSharpnessAvailable);
		auto valueChanger = OptionsDropdown<int>::make(
			[]() {
				OptionChoices<int> result;
				result.choices = {
					{ _("Maximum"), "", 0 },
					{ _("High"), "", 25 },
					{ _("Medium"), "", 50 },
					{ _("Low"), "", 100 },
				};
				int currValue = war_getUpscalingSharpness();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({WzString::format("(Custom: %d)", currValue), "", currValue});
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setUpscalingSharpness(newValue);
				display3d_setUpscalingSharpness(war_getUpscalingSharpness() / 100.f);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true, 1);
	}
	{
		auto optionInfo = OptionInfo("gfx.antialiasing", N_("Antialiasing"), "");
		optionInfo.addAvailabilityCondition(IsNotInGame);
		optionInfo.setRequiresRestart(true);
		auto valueChanger = OptionsDropdown<int>::make(
			[]() {
				const auto maxAA = pie_GetMaxAntialiasing();
				OptionChoices<int> result;
				result.choices = {
					{ _("Off"), "", 0 }
				};
				for (int value = 2; value <= maxAA; value *= 2)
				{
					result.choices.push_back({ WzString::number(value) + "x", "", value });
				}
				auto currValue = war_getAntialiasing();
				if (!result.setCurrentIdxForValue(currValue))
				{
					result.choices.push_back({ WzString::number(currValue) + "x", "", currValue });
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				war_setAntialiasing(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.smaa", N_("SMAA"), N_("Post-process antialiasing over the 3D world. Works alongside Antialiasing and improves FSR 1.0 upscaling quality."));
		auto valueChanger = OptionsDropdown<SMAA_MODE>::make(
			[]() {
				OptionChoices<SMAA_MODE> result;
				result.choices = {
					{ _("Off"), "", SMAA_MODE::OFF },
					{ _("Low"), "", SMAA_MODE::LOW },
					{ _("Medium"), "", SMAA_MODE::MEDIUM },
					{ _("High"), "", SMAA_MODE::HIGH },
					{ _("Ultra"), "", SMAA_MODE::ULTRA },
				};
				result.setCurrentIdxForValue(war_getSmaaMode());
				return result;
			},
			[](const auto& newValue) -> bool {
				if (!display3d_setSmaaMode(newValue))
				{
					debug(LOG_ERROR, "Failed to set SMAA mode");
					return false;
				}
				war_setSmaaMode(newValue);
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("gfx.vsync", N_("Vertical Sync"), "");
		auto valueChanger = OptionsDropdown<gfx_api::context::swap_interval_mode>::make(
			[]() {
				OptionChoices<gfx_api::context::swap_interval_mode> result;
				result.choices = {
					{ _("Off"), "", gfx_api::context::swap_interval_mode::immediate },
					{ _("On"), "", gfx_api::context::swap_interval_mode::vsync },
					{ _("Adaptive"), "", gfx_api::context::swap_interval_mode::adaptive_vsync }
				};
				result.setCurrentIdxForValue(to_swap_mode(war_GetVsync()));
				return result;
			},
			[](const auto& newValue) -> bool {
				auto canChange = gfx_api::context::get().setSwapInterval(newValue, []() {
					wzPostChangedSwapInterval();
				});
				if (!canChange)
				{
					debug(LOG_INFO, "Vsync mode not supported by system: %d", to_int(newValue));
					return false;
				}
				war_SetVsync(to_int(newValue));
				return true;
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	return result;
}
