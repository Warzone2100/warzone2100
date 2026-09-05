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
/** @file
 *  Light thrown by effects, configured from the data files.
 */

#include "effectlights.h"

#include "lib/framework/file.h"
#include "lib/framework/wzconfig.h"
#include "lib/ivis_opengl/imd.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/pielighting.h"
#include "lib/gamelib/gtime.h"

#include "stats.h"
#include "statsdef.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

// NOTE: A range past the max could crowd other lights out of the point light budget improperly
static constexpr UDWORD minLightRange = 1;
static constexpr UDWORD maxLightRange = 1024;
static constexpr float minLightIntensity = 0.01f;
static constexpr float maxLightIntensity = 4.f;
static constexpr float minLightScale = 0.05f;
static constexpr float maxLightScale = 4.f;

// Values past these are far outside anything the game ships (but still applied)
static constexpr UDWORD advisoryLightRange = 512;
static constexpr float advisoryLightIntensity = 2.f;

static constexpr UDWORD maxFadeDuration = 1000; // ms - keeps a data file from creating long-lived phantom lights
static constexpr UDWORD defaultFadeDuration = 200; // ms

/// A field with no value is taken from the next place that has one
struct EffectLightSettings
{
	nonstd::optional<bool> enabled;
	nonstd::optional<PIELIGHT> color;
	nonstd::optional<UDWORD> range;
	nonstd::optional<float> intensity;
	nonstd::optional<float> rangeScale;
	nonstd::optional<float> intensityScale;
	nonstd::optional<UDWORD> fadeDuration;
};

static std::array<EffectLightSettings, WSC_NUM_WEAPON_SUBCLASSES> projectileSubClassSettings;
static std::vector<EffectLightSettings> projectileWeaponSettings;
static std::string effectLightsFileName;

/// The in-flight light each weapon throws, resolved once when the settings load and indexed by weapon stat index.
struct ResolvedProjectileLight
{
	nonstd::optional<ProjectileLight> light;
	UDWORD fadeDuration = defaultFadeDuration;
};
static std::vector<ResolvedProjectileLight> resolvedProjectileLights;

/// A subclass with no entry uses the weapon class default.
static const std::array<nonstd::optional<PIELIGHT>, WSC_NUM_WEAPON_SUBCLASSES> projectileLightColorBySubClass = []() {
	std::array<nonstd::optional<PIELIGHT>, WSC_NUM_WEAPON_SUBCLASSES> table;
	// WSC_FLAME is WC_HEAT, which would otherwise take the color meant for energy weapons.
	table[WSC_FLAME] = pal_Colour(255, 180, 100);
	return table;
}();

static PIELIGHT defaultProjectileLightColor(const WEAPON_STATS *psStats)
{
	if (psStats->weaponSubClass < WSC_NUM_WEAPON_SUBCLASSES)
	{
		const auto &subClassColor = projectileLightColorBySubClass[psStats->weaponSubClass];
		if (subClassColor.has_value())
		{
			return subClassColor.value();
		}
	}
	return (psStats->weaponClass == WC_HEAT) ? pal_Colour(150, 205, 255) : pal_Colour(255, 170, 90);
}

// Not getWeaponSubClass(const char *), which asserts on a name that is not a subclass.
static nonstd::optional<WEAPON_SUBCLASS> weaponSubClassFromName(const std::string &name)
{
	for (unsigned i = 0; i < WSC_NUM_WEAPON_SUBCLASSES; ++i)
	{
		const WEAPON_SUBCLASS subClass = static_cast<WEAPON_SUBCLASS>(i);
		if (name == getWeaponSubClass(subClass))
		{
			return subClass;
		}
	}
	return nonstd::nullopt;
}

static std::string weaponSubClassNameList()
{
	std::string result;
	for (unsigned i = 0; i < WSC_NUM_WEAPON_SUBCLASSES; ++i)
	{
		if (!result.empty())
		{
			result += ", ";
		}
		result += getWeaponSubClass(static_cast<WEAPON_SUBCLASS>(i));
	}
	return result;
}

static nonstd::optional<size_t> weaponIndexFromId(const std::string &id)
{
	const COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(id));
	if (psComp == nullptr || psComp->compType != COMP_WEAPON)
	{
		return nonstd::nullopt;
	}
	return psComp->index;
}

static bool isCommentKey(const std::string &key)
{
	return !key.empty() && key.front() == '_';
}

static bool parseColorValue(const nlohmann::json &value, const std::string &what, PIELIGHT &output)
{
	if (!value.is_array() || value.size() != 3)
	{
		debug(LOG_ERROR, "%s: \"color\" must be an array of three values", what.c_str());
		return false;
	}
	UBYTE channels[3];
	for (size_t i = 0; i < 3; ++i)
	{
		if (!value[i].is_number_integer())
		{
			debug(LOG_ERROR, "%s: \"color\" must hold whole numbers", what.c_str());
			return false;
		}
		const int64_t channel = value[i].get<int64_t>();
		if (channel < 0 || channel > 255)
		{
			debug(LOG_ERROR, "%s: \"color\" value %" PRId64 " is outside 0-255", what.c_str(), channel);
			return false;
		}
		channels[i] = static_cast<UBYTE>(channel);
	}
	output = pal_Colour(channels[0], channels[1], channels[2]);
	return true;
}

static bool parseRangeValue(const nlohmann::json &value, const std::string &what, UDWORD &output)
{
	if (!value.is_number_integer())
	{
		debug(LOG_ERROR, "%s: \"range\" must be a whole number", what.c_str());
		return false;
	}
	const int64_t range = value.get<int64_t>();
	if (range < static_cast<int64_t>(minLightRange))
	{
		debug(LOG_ERROR, "%s: \"range\" must be at least %" PRIu32 " (to throw no light, set \"enabled\" to false)", what.c_str(), minLightRange);
		return false;
	}
	if (range > static_cast<int64_t>(maxLightRange))
	{
		debug(LOG_ERROR, "%s: \"range\" %" PRId64 " is above the limit of %" PRIu32 ", using the limit", what.c_str(), range, maxLightRange);
		output = maxLightRange;
		return true;
	}
	output = static_cast<UDWORD>(range);
	if (output > advisoryLightRange)
	{
		debug(LOG_WARNING, "%s: \"range\" %" PRIu32 " reaches much further than anything the game ships, and may crowd other lights out of the point light budget", what.c_str(), output);
	}
	return true;
}

static bool parseFadeDurationValue(const nlohmann::json &value, const std::string &what, UDWORD &output)
{
	if (!value.is_number_integer())
	{
		debug(LOG_ERROR, "%s: \"fadeDuration\" must be a whole number of milliseconds", what.c_str());
		return false;
	}
	const int64_t duration = value.get<int64_t>();
	if (duration < 0)
	{
		debug(LOG_ERROR, "%s: \"fadeDuration\" cannot be negative", what.c_str());
		return false;
	}
	if (duration > static_cast<int64_t>(maxFadeDuration))
	{
		debug(LOG_ERROR, "%s: \"fadeDuration\" %" PRId64 " is above the limit of %" PRIu32 ", using the limit", what.c_str(), duration, maxFadeDuration);
		output = maxFadeDuration;
		return true;
	}
	output = static_cast<UDWORD>(duration);
	return true;
}

static bool parseFloatValue(const nlohmann::json &value, const std::string &what, const std::string &key, float lowest, float highest, float &output)
{
	if (!value.is_number())
	{
		debug(LOG_ERROR, "%s: \"%s\" must be a number", what.c_str(), key.c_str());
		return false;
	}
	const double number = value.get<double>();
	if (!std::isfinite(number))
	{
		debug(LOG_ERROR, "%s: \"%s\" must be a finite number", what.c_str(), key.c_str());
		return false;
	}
	if (number < static_cast<double>(lowest) || number > static_cast<double>(highest))
	{
		output = static_cast<float>(std::min(std::max(number, static_cast<double>(lowest)), static_cast<double>(highest)));
		debug(LOG_ERROR, "%s: \"%s\" %g is outside %g-%g, using %g", what.c_str(), key.c_str(), number, lowest, highest, output);
		return true;
	}
	output = static_cast<float>(number);
	return true;
}

static void parseEntry(const nlohmann::json &entry, const std::string &what, EffectLightSettings &output)
{
	if (!entry.is_object())
	{
		debug(LOG_ERROR, "%s: expecting a group of settings", what.c_str());
		return;
	}
	for (auto it = entry.begin(); it != entry.end(); ++it)
	{
		const std::string key = it.key();
		if (isCommentKey(key))
		{
			continue;
		}
		if (key == "enabled")
		{
			if (!it.value().is_boolean())
			{
				debug(LOG_ERROR, "%s: \"enabled\" must be true or false", what.c_str());
				continue;
			}
			output.enabled = it.value().get<bool>();
		}
		else if (key == "color")
		{
			PIELIGHT color;
			if (parseColorValue(it.value(), what, color))
			{
				output.color = color;
			}
		}
		else if (key == "range")
		{
			UDWORD range = 0;
			if (parseRangeValue(it.value(), what, range))
			{
				output.range = range;
			}
		}
		else if (key == "intensity")
		{
			float intensity = 0.f;
			if (parseFloatValue(it.value(), what, key, minLightIntensity, maxLightIntensity, intensity))
			{
				output.intensity = intensity;
				if (intensity > advisoryLightIntensity)
				{
					debug(LOG_WARNING, "%s: \"intensity\" %g is bright enough to flatten the light into a disk of flat color", what.c_str(), intensity);
				}
			}
		}
		else if (key == "fadeDuration")
		{
			UDWORD duration = 0;
			if (parseFadeDurationValue(it.value(), what, duration))
			{
				output.fadeDuration = duration;
			}
		}
		else if (key == "rangeScale")
		{
			float scale = 1.f;
			if (parseFloatValue(it.value(), what, key, minLightScale, maxLightScale, scale))
			{
				output.rangeScale = scale;
			}
		}
		else if (key == "intensityScale")
		{
			float scale = 1.f;
			if (parseFloatValue(it.value(), what, key, minLightScale, maxLightScale, scale))
			{
				output.intensityScale = scale;
			}
		}
		else
		{
			debug(LOG_ERROR, "%s: unknown key \"%s\"", what.c_str(), key.c_str());
		}
	}
}

static void parseProjectileSubClasses(const nlohmann::json &group)
{
	if (!group.is_object())
	{
		debug(LOG_ERROR, "projectiles: \"subclass\" must be a group of weapon subclasses");
		return;
	}
	for (auto it = group.begin(); it != group.end(); ++it)
	{
		const std::string name = it.key();
		if (isCommentKey(name))
		{
			continue;
		}
		const auto subClass = weaponSubClassFromName(name);
		if (!subClass.has_value())
		{
			debug(LOG_ERROR, "projectiles: unknown weapon subclass \"%s\", expecting one of: %s", name.c_str(), weaponSubClassNameList().c_str());
			continue;
		}
		parseEntry(it.value(), "projectiles.subclass." + name, projectileSubClassSettings[subClass.value()]);
	}
}

static void parseProjectileWeapons(const nlohmann::json &group)
{
	if (!group.is_object())
	{
		debug(LOG_ERROR, "projectiles: \"weapon\" must be a group of weapons");
		return;
	}
	for (auto it = group.begin(); it != group.end(); ++it)
	{
		const std::string id = it.key();
		if (isCommentKey(id))
		{
			continue;
		}
		const auto index = weaponIndexFromId(id);
		if (!index.has_value())
		{
			debug(LOG_WARNING, "projectiles: no weapon called \"%s\" in this ruleset", id.c_str());
			continue;
		}
		ASSERT_OR_RETURN(, index.value() < projectileWeaponSettings.size(), "Weapon %s has an index past the end of the weapon stats", id.c_str());
		parseEntry(it.value(), "projectiles.weapon." + id, projectileWeaponSettings[index.value()]);
	}
}

static void parseProjectiles(const nlohmann::json &section)
{
	if (!section.is_object())
	{
		debug(LOG_ERROR, "\"projectiles\" must be a group of settings");
		return;
	}
	for (auto it = section.begin(); it != section.end(); ++it)
	{
		const std::string key = it.key();
		if (isCommentKey(key))
		{
			continue;
		}
		if (key == "subclass")
		{
			parseProjectileSubClasses(it.value());
		}
		else if (key == "weapon")
		{
			parseProjectileWeapons(it.value());
		}
		else
		{
			debug(LOG_ERROR, "projectiles: unknown key \"%s\"", key.c_str());
		}
	}
}

template<typename T>
static nonstd::optional<T> resolveField(const EffectLightSettings *weapon, const EffectLightSettings *subClass, nonstd::optional<T> EffectLightSettings::*field)
{
	if (weapon != nullptr && (weapon->*field).has_value())
	{
		return weapon->*field;
	}
	if (subClass != nullptr && (subClass->*field).has_value())
	{
		return subClass->*field;
	}
	return nonstd::nullopt;
}

template<typename T>
static const char *fieldSource(const EffectLightSettings *weapon, const EffectLightSettings *subClass, nonstd::optional<T> EffectLightSettings::*field)
{
	if (weapon != nullptr && (weapon->*field).has_value())
	{
		return "weapon";
	}
	if (subClass != nullptr && (subClass->*field).has_value())
	{
		return "subclass";
	}
	return "default";
}

static const EffectLightSettings *projectileWeaponSettingsFor(const WEAPON_STATS *psStats)
{
	return (psStats->index < projectileWeaponSettings.size()) ? &projectileWeaponSettings[psStats->index] : nullptr;
}

static const EffectLightSettings *projectileSubClassSettingsFor(const WEAPON_STATS *psStats)
{
	return (psStats->weaponSubClass < WSC_NUM_WEAPON_SUBCLASSES) ? &projectileSubClassSettings[psStats->weaponSubClass] : nullptr;
}

// The graphic already says how big the glow is.
// Its cross section rather than its radius, because some models are drawn as streaks.
static float projectilePlumeScale(const iIMDShape *pIMD)
{
	constexpr float referenceGlowSize = 32.f; // the medium rocket plume, where the values here were set
	return std::min(std::max(static_cast<float>(pIMD->crossSection) / referenceGlowSize, 0.5f), 1.5f);
}

nonstd::optional<ProjectileLight> resolveProjectileLight(const WEAPON_STATS *psStats, const iIMDShape *pIMD, bool modelIsGlowing)
{
	ASSERT_OR_RETURN(nonstd::nullopt, psStats != nullptr && pIMD != nullptr, "Expecting a weapon and its in-flight graphic");

	const EffectLightSettings *weapon = projectileWeaponSettingsFor(psStats);
	const EffectLightSettings *subClass = projectileSubClassSettingsFor(psStats);

	if (!resolveField(weapon, subClass, &EffectLightSettings::enabled).value_or(modelIsGlowing))
	{
		return nonstd::nullopt;
	}

	const float plumeScale = projectilePlumeScale(pIMD);

	ProjectileLight light;
	light.color = resolveField(weapon, subClass, &EffectLightSettings::color).value_or(defaultProjectileLightColor(psStats));

	// A scale multiplies whatever the value would otherwise have been (whether set or from the graphic)
	const float range = static_cast<float>(resolveField(weapon, subClass, &EffectLightSettings::range).value_or(static_cast<UDWORD>(260.f * plumeScale)))
		* resolveField(weapon, subClass, &EffectLightSettings::rangeScale).value_or(1.f);
	const float intensity = resolveField(weapon, subClass, &EffectLightSettings::intensity).value_or(1.1f * plumeScale)
		* resolveField(weapon, subClass, &EffectLightSettings::intensityScale).value_or(1.f);

	light.range = std::min(std::max(static_cast<UDWORD>(range), minLightRange), maxLightRange);
	light.intensity = std::min(std::max(intensity, minLightIntensity), maxLightIntensity);
	return light;
}

// A part is drawn as a glow when its subclass or model flags make it additive or premultiplied.
// The flag order here matches renderProjectile - later flags override earlier ones.
static bool projectilePartGlows(const WEAPON_STATS *psStats, const iIMDShape *pIMD)
{
	bool additive = psStats->weaponSubClass == WSC_ROCKET || psStats->weaponSubClass == WSC_MISSILE
		|| psStats->weaponSubClass == WSC_SLOWROCKET || psStats->weaponSubClass == WSC_SLOWMISSILE;
	bool premultiplied = false;
	if (pIMD->flags & iV_IMD_NO_ADDITIVE)
	{
		additive = false;
	}
	if (pIMD->flags & iV_IMD_ADDITIVE)
	{
		additive = true;
	}
	if (pIMD->flags & iV_IMD_PREMULTIPLIED)
	{
		additive = false;
		premultiplied = true;
	}
	return additive || premultiplied;
}

bool projectileGraphicGlows(const WEAPON_STATS *psStats, const iIMDShape *pFirstIMD)
{
	for (const iIMDShape *pIMD = pFirstIMD; pIMD != nullptr; pIMD = pIMD->next.get())
	{
		if (projectilePartGlows(psStats, pIMD))
		{
			return true;
		}
	}
	return false;
}

nonstd::optional<ProjectileLight> resolveInFlightProjectileLight(const WEAPON_STATS *psStats, const iIMDShape *pFirstIMD)
{
	if (psStats == nullptr || pFirstIMD == nullptr)
	{
		return nonstd::nullopt;
	}
	// The first glowing part lights the world (its plume sizes the light). If none glow, only the data files can.
	for (const iIMDShape *pIMD = pFirstIMD; pIMD != nullptr; pIMD = pIMD->next.get())
	{
		if (projectilePartGlows(psStats, pIMD))
		{
			return resolveProjectileLight(psStats, pIMD, true);
		}
	}
	return resolveProjectileLight(psStats, pFirstIMD, false);
}

static void buildResolvedProjectileLights()
{
	resolvedProjectileLights.assign(asWeaponStats.size(), ResolvedProjectileLight{});
	for (const auto &psStats : asWeaponStats)
	{
		if (psStats.index >= resolvedProjectileLights.size())
		{
			continue;
		}
		const iIMDShape *pIMD = (psStats.pInFlightGraphic != nullptr) ? psStats.pInFlightGraphic->displayModel() : nullptr;
		ResolvedProjectileLight &resolved = resolvedProjectileLights[psStats.index];
		resolved.light = resolveInFlightProjectileLight(&psStats, pIMD);
		resolved.fadeDuration = resolveField(projectileWeaponSettingsFor(&psStats), projectileSubClassSettingsFor(&psStats),
			&EffectLightSettings::fadeDuration).value_or(defaultFadeDuration);
	}
}

const ProjectileLight *cachedInFlightProjectileLight(size_t weaponIndex)
{
	if (weaponIndex >= resolvedProjectileLights.size())
	{
		return nullptr;
	}
	const auto &resolved = resolvedProjectileLights[weaponIndex];
	return resolved.light.has_value() ? &resolved.light.value() : nullptr;
}

// Max fades that may exist at once - past the cap the cheapest is evicted
static constexpr size_t maxProjectileFades = 24;
// Below this a light is invisible but would still cost a light slot
static constexpr float fadeCutoffIntensity = 0.05f;

/// The light a projectile threw last frame, kept so the frame it stops can start a fade from it.
struct EmittedProjectileLight
{
	uint32_t id;
	Vector3i position;
	ProjectileLight light;
	uint32_t duration;
};

struct FadingProjectileLight
{
	uint32_t id;
	Vector3i position;
	ProjectileLight light;
	uint32_t startTime;
	uint32_t duration;
};

// Swapped each frame - what threw a light this frame, and what did last frame.
static std::vector<EmittedProjectileLight> emittedThisFrame;
static std::vector<EmittedProjectileLight> emittedLastFrame;
static std::vector<FadingProjectileLight> fadingLights;

void beginProjectileLightFrame()
{
	emittedThisFrame.clear();
}

void recordProjectileLight(uint32_t id, const Vector3i &position, size_t weaponIndex)
{
	if (weaponIndex >= resolvedProjectileLights.size())
	{
		return;
	}
	const ResolvedProjectileLight &resolved = resolvedProjectileLights[weaponIndex];
	if (!resolved.light.has_value())
	{
		return;
	}
	EmittedProjectileLight emitted;
	emitted.id = id;
	emitted.position = position;
	emitted.light = resolved.light.value();
	emitted.duration = resolved.fadeDuration;
	emittedThisFrame.push_back(emitted);
}

// A fade cools like an ember: its intensity falls quadratically and its range shrinks toward half.
// Returns false once the light has cooled past the point it is worth a light slot.
static bool fadeToLight(const FadingProjectileLight &fade, uint32_t now, LIGHT &out)
{
	const uint32_t age = now - fade.startTime;
	if (age >= fade.duration)
	{
		return false;
	}
	const float remaining = 1.f - static_cast<float>(age) / static_cast<float>(fade.duration);
	const float intensity = fade.light.intensity * remaining * remaining;
	if (intensity < fadeCutoffIntensity)
	{
		return false;
	}
	out.position = fade.position;
	out.range = static_cast<UDWORD>(static_cast<float>(fade.light.range) * (0.5f + 0.5f * remaining));
	out.colour = fade.light.color;
	out.intensity = intensity;
	return true;
}

// The importance the light manager would give this fade
static float fadeImportance(const FadingProjectileLight &fade, uint32_t now)
{
	LIGHT light;
	if (!fadeToLight(fade, now, light))
	{
		return 0.f;
	}
	return light.intensity * static_cast<float>(light.range) * static_cast<float>(light.range);
}

static void spawnFade(const EmittedProjectileLight &seed, uint32_t now)
{
	if (seed.duration == 0)
	{
		return;
	}
	FadingProjectileLight fade;
	fade.id = seed.id;
	fade.position = seed.position;
	fade.light = seed.light;
	fade.startTime = now;
	fade.duration = seed.duration;

	if (fadingLights.size() < maxProjectileFades)
	{
		fadingLights.push_back(fade);
		return;
	}
	size_t cheapest = 0;
	float cheapestImportance = fadeImportance(fadingLights[0], now);
	for (size_t i = 1; i < fadingLights.size(); ++i)
	{
		const float importance = fadeImportance(fadingLights[i], now);
		if (importance < cheapestImportance)
		{
			cheapest = i;
			cheapestImportance = importance;
		}
	}
	fadingLights[cheapest] = fade;
}

static bool idEmittedThisFrame(uint32_t id)
{
	for (const auto &emitted : emittedThisFrame)
	{
		if (emitted.id == id)
		{
			return true;
		}
	}
	return false;
}

void emitProjectileFades(LightingData &lightData)
{
	const uint32_t now = graphicsTime;

	// A projectile that re-enters emission cancels its own fade, so a one-frame flap never doubles the light.
	fadingLights.erase(std::remove_if(fadingLights.begin(), fadingLights.end(),
		[](const FadingProjectileLight &fade) { return idEmittedThisFrame(fade.id); }), fadingLights.end());

	for (const auto &emitted : emittedLastFrame)
	{
		if (!idEmittedThisFrame(emitted.id))
		{
			spawnFade(emitted, now);
		}
	}

	for (size_t i = 0; i < fadingLights.size(); )
	{
		LIGHT light;
		if (fadeToLight(fadingLights[i], now, light))
		{
			lightData.lights.push_back(light);
			++i;
		}
		else
		{
			fadingLights[i] = fadingLights.back();
			fadingLights.pop_back();
		}
	}

	std::swap(emittedLastFrame, emittedThisFrame);
}

void clearFadingProjectileLights()
{
	fadingLights.clear();
	emittedThisFrame.clear();
	emittedLastFrame.clear();
}

static void reportResolvedProjectileLights()
{
	if (!enabled_debug[LOG_WZ])
	{
		return;
	}

	for (const auto &psStats : asWeaponStats)
	{
		const EffectLightSettings *weapon = projectileWeaponSettingsFor(&psStats);
		const EffectLightSettings *subClass = projectileSubClassSettingsFor(&psStats);
		const iIMDShape *pIMD = (psStats.pInFlightGraphic != nullptr) ? psStats.pInFlightGraphic->displayModel() : nullptr;
		if (pIMD == nullptr)
		{
			debug(LOG_WZ, "%s (%s): no in-flight graphic", getID(&psStats), getWeaponSubClass(psStats.weaponSubClass));
			continue;
		}
		// Reported as though the graphic glows,
		// so the values show even for a weapon whose graphic decides against a light.
		const auto light = resolveProjectileLight(&psStats, pIMD, true);
		if (!light.has_value())
		{
			debug(LOG_WZ, "%s (%s): turned off (%s)", getID(&psStats), getWeaponSubClass(psStats.weaponSubClass),
			      fieldSource(weapon, subClass, &EffectLightSettings::enabled));
			continue;
		}
		const UDWORD fadeDuration = resolveField(weapon, subClass, &EffectLightSettings::fadeDuration).value_or(defaultFadeDuration);
		debug(LOG_WZ, "%s (%s): color [%u, %u, %u] (%s), range %" PRIu32 " (%s), intensity %g (%s), fade %" PRIu32 " ms (%s)",
		      getID(&psStats), getWeaponSubClass(psStats.weaponSubClass),
		      light->color.byte.r, light->color.byte.g, light->color.byte.b, fieldSource(weapon, subClass, &EffectLightSettings::color),
		      light->range, fieldSource(weapon, subClass, &EffectLightSettings::range),
		      light->intensity, fieldSource(weapon, subClass, &EffectLightSettings::intensity),
		      fadeDuration, fieldSource(weapon, subClass, &EffectLightSettings::fadeDuration));
	}
}

static void parseEffectLightsSections(const char *pFileName, const std::vector<char> &fileContents)
{
	nlohmann::json root;
	try
	{
		root = nlohmann::json::parse(fileContents.begin(), fileContents.end());
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "%s is not valid JSON, ignoring it: %s", pFileName, e.what());
		return;
	}

	if (!root.is_object())
	{
		debug(LOG_ERROR, "%s: expecting a group of sections", pFileName);
		return;
	}

	for (auto it = root.begin(); it != root.end(); ++it)
	{
		const std::string key = it.key();
		if (isCommentKey(key))
		{
			continue;
		}
		if (key == "projectiles")
		{
			parseProjectiles(it.value());
		}
		else
		{
			debug(LOG_ERROR, "%s: unknown section \"%s\"", pFileName, key.c_str());
		}
	}
}

bool loadEffectLights(const char *pFileName)
{
	ASSERT_OR_RETURN(false, pFileName != nullptr, "Expecting a file name");
	effectLightsFileName = pFileName;

	projectileSubClassSettings = {};
	projectileWeaponSettings.clear();
	projectileWeaponSettings.resize(asWeaponStats.size());

	// No file is fine - every light then comes from its graphic and the built-in values.
	std::vector<char> fileContents;
	if (loadFileToBufferVector(pFileName, fileContents, false))
	{
		parseEffectLightsSections(pFileName, fileContents);
	}

	buildResolvedProjectileLights();
	reportResolvedProjectileLights();
	return true;
}

void reloadEffectLights()
{
	if (effectLightsFileName.empty())
	{
		debug(LOG_ERROR, "No effect light settings have been loaded to reload");
		return;
	}
	loadEffectLights(effectLightsFileName.c_str());
}

void effectLightsShutDown()
{
	projectileSubClassSettings = {};
	projectileWeaponSettings.clear();
	resolvedProjectileLights.clear();
	clearFadingProjectileLights();
	effectLightsFileName.clear();
}
