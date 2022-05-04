/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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

#include "map_script.h"
#include "../include/wzmaplib/map.h"
#include "../include/wzmaplib/map_debug.h"
#include "map_internal.h"

#include <algorithm>
#include <cinttypes>
#include <limits>

#define MAX_PLAYERS         11                 ///< Maximum number of players in the game.

// MARK: - Script Maps handling

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#if defined(_MSC_VER)
__pragma(warning( push ))
__pragma(warning( disable : 4191 )) // disable "warning C4191: 'type cast': unsafe conversion from 'JSCFunctionMagic (__cdecl *)' to 'JSCFunction (__cdecl *)'"
#endif
#include "quickjs.h"
#include "quickjs-limitedcontext.h"
#if defined(_MSC_VER)
__pragma(warning( pop ))
#endif
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif
#include "3rdparty/gsl_finally.h"
#include <random>
#include <chrono>

// QUICKJS HELPERS

namespace WzMap {

/// Assert for scripts that give useful backtraces and other info.
#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(pCustomLogger, LOG_ERROR, __VA_ARGS__); \
			JS_ThrowReferenceError(context, "%s failed in %s at line %d", #expr, __FUNCTION__, __LINE__); \
			return JS_NULL; } } while (0)

#define SCRIPT_ASSERT_AND_RETURNERROR(context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(pCustomLogger, LOG_ERROR, __VA_ARGS__); \
			return JS_ThrowReferenceError(context, "%s failed in %s at line %d", #expr, __FUNCTION__, __LINE__); \
			} } while (0)

bool QuickJS_GetArrayLength(JSContext* ctx, JSValueConst arr, uint64_t& len)
{
	len = 0;

	if (!JS_IsArray(ctx, arr))
		return false;

	JSValue len_val = JS_GetPropertyStr(ctx, arr, "length");
	if (JS_IsException(len_val))
		return false;

	if (JS_ToIndex(ctx, &len, len_val)) {
		JS_FreeValue(ctx, len_val);
		return false;
	}

	JS_FreeValue(ctx, len_val);
	return true;
}

static inline int32_t JSValueToInt32(JSContext *ctx, JSValue &value)
{
	int intVal = 0;
	if (JS_ToInt32(ctx, &intVal, value))
	{
		// failed
//		ASSERT(false, "Failed"); // TODO:
	}
	return intVal;
}

static inline uint32_t JSValueToUint32(JSContext *ctx, JSValue &value)
{
	uint32_t uintVal = 0;
	if (JS_ToUint32(ctx, &uintVal, value))
	{
		// failed
//		ASSERT(false, "Failed"); // TODO:
	}
	return uintVal;
}

static inline std::string JSValueToStdString(JSContext *ctx, JSValue &value)
{
	const char* pStr = JS_ToCString(ctx, value);
	std::string result;
	if (pStr) result = pStr;
	JS_FreeCString(ctx, pStr);
	return result;
}
static std::string QuickJS_GetStdString(JSContext *ctx, JSValueConst this_obj, const char *prop)
{
	JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
	std::string result = JSValueToStdString(ctx, val);
	JS_FreeValue(ctx, val);
	return result;
}

static int32_t QuickJS_GetInt32(JSContext *ctx, JSValueConst this_obj, const char *prop)
{
	JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
	int32_t result = JSValueToInt32(ctx, val);
	JS_FreeValue(ctx, val);
	return result;
}

static uint32_t QuickJS_GetUint32(JSContext *ctx, JSValueConst this_obj, const char *prop)
{
	JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
	uint32_t result = JSValueToUint32(ctx, val);
	JS_FreeValue(ctx, val);
	return result;
}

static std::string QuickJS_DumpObject(JSContext *ctx, JSValue obj)
{
	std::string result;
	const char *str = JS_ToCString(ctx, obj);
	if (str)
	{
		result = str;
		JS_FreeCString(ctx, str);
	}
	else
	{
		result = "[failed to convert object to string]";
	}
	return result;
}

static std::string QuickJS_DumpError(JSContext *ctx)
{
	std::string result;
	JSValue exception_val = JS_GetException(ctx);
	bool isError = JS_IsError(ctx, exception_val);
	result = QuickJS_DumpObject(ctx, exception_val);
	if (isError)
	{
		JSValue stack = JS_GetPropertyStr(ctx, exception_val, "stack");
		if (!JS_IsUndefined(stack))
		{
			result += "\n" + QuickJS_DumpObject(ctx, stack);
		}
		JS_FreeValue(ctx, stack);
	}
	JS_FreeValue(ctx, exception_val);
	return result;
}

// Alternatives for C++ - can't use the JS_CFUNC_DEF / JS_CGETSET_DEF / etc defines
// #define JS_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }
static inline JSCFunctionListEntry QJS_CFUNC_DEF(const char *name, uint8_t length, JSCFunction *func1)
{
	JSCFunctionListEntry entry;
	entry.name = name;
	entry.prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE;
	entry.def_type = JS_DEF_CFUNC;
	entry.magic = 0;
	entry.u.func.length = length;
	entry.u.func.cproto = JS_CFUNC_generic;
	entry.u.func.cfunc.generic = func1;
	return entry;
}

struct ScriptMapData
{
	std::shared_ptr<WzMap::Map> map = std::make_shared<WzMap::Map>();
	std::mt19937 mt;
	WzMap::LoggingProtocol* pCustomLogger = nullptr;
};

#define MAPSCRIPT_GET_CONTEXT_DATA() \
	void* pOpaque = JS_GetContextOpaque(ctx); \
	if (!pOpaque) \
	{ \
		return JS_ThrowInternalError(ctx, "Unable to acquire context information"); \
	} \
	auto &data = *static_cast<ScriptMapData*>(pOpaque);

//MAP-- ## gameRand([mod])
//MAP--
//MAP-- Generates a random number in a "safe" way, that will ensure that randomly-generated
//MAP-- maps are the same for all connected players.
//MAP--
static JSValue runMap_gameRand(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	MAPSCRIPT_GET_CONTEXT_DATA()//;
	uint32_t num = data.mt();
	uint32_t mod = (argc >= 1) ? JSValueToUint32(ctx, argv[0]) : 0;
	uint32_t result = mod? num%mod : num;
	return JS_NewUint32(ctx, result);
}

//MAP-- ## log(string)
//MAP--
//MAP-- Outputs to the game's debug log a string (or object that can be converted to string)
//MAP--
static JSValue runMap_log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	MAPSCRIPT_GET_CONTEXT_DATA()//;
	auto pCustomLogger = data.pCustomLogger;
	SCRIPT_ASSERT(ctx, argc == 1, "Must have one parameter");
	std::string str = JSValueToStdString(ctx, argv[0]);
	debug(pCustomLogger, LOG_INFO, "game.js: \"%s\"", str.c_str());
	return JS_UNDEFINED;
}

//MAP-- ## setMapData(mapWidth, mapHeight, texture, height, structures, droids, features)
//MAP--
//MAP-- Sets map data from that generated by the script.
//MAP--
//MAP-- (NOTE: Documentation incomplete. Look at the example mapScript maps: 6c-Entropy/game.js, 10c-WaterLoop/game.js)
//MAP--
//MAP-- `structures` is an array of structure data objects. Example:
//MAP--    {name: "A0CommandCentre", position: [x, y], direction: 0x4000*gameRand(4), modules: 0, player: 1}
//MAP-- `droids` is an array of droid data objects. Example:
//MAP--    {name: "ConstructionDroid", position: [x, y], direction: gameRand(0x10000), player: 1}
//MAP-- `features` is an array of feature data objects. Example:
//MAP--    {name: "Tree1", position: [x, y], direction: gameRand(0x10000)}
//MAP--
static JSValue runMap_setMapData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	MAPSCRIPT_GET_CONTEXT_DATA()//;
	auto pCustomLogger = data.pCustomLogger;
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, argc == 7, "Must have 7 parameters");
	auto jsVal_mapWidth = argv[0];
	auto jsVal_mapHeight = argv[1];
	auto texture = argv[2];
	auto height = argv[3];
	auto structures = argv[4];
	auto droids = argv[5];
	auto features = argv[6];
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_mapWidth), "mapWidth must be number");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_mapHeight), "mapHeight must be number");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsArray(ctx, texture), "texture must be array");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsArray(ctx, height), "height must be array");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsArray(ctx, structures), "structures must be array");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsArray(ctx, droids), "droids must be array");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsArray(ctx, features), "features must be array");
	auto mapData = data.map->mapData();
	mapData->width = JSValueToInt32(ctx, jsVal_mapWidth);
	mapData->height = JSValueToInt32(ctx, jsVal_mapHeight);
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, mapData->width > 1 && mapData->height > 1 && (uint64_t)mapData->width*mapData->height <= MAP_MAXAREA, "Map size out of bounds");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, mapData->width <= MAP_MAXWIDTH, "Map width too large");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, mapData->height <= MAP_MAXHEIGHT, "Map height too large");
	size_t N = (size_t)mapData->width*mapData->height;
	mapData->mMapTiles.resize(N);
	uint64_t arrayLen = 0;
	bool bGotArrayLength = QuickJS_GetArrayLength(ctx, texture, arrayLen);
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen == ((uint64_t)mapData->width * (uint64_t)mapData->height)), "texture array length must equal (mapWidth * mapHeight); actual length is: %" PRIu64"", arrayLen);
	bGotArrayLength = QuickJS_GetArrayLength(ctx, height, arrayLen);
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen == ((uint64_t)mapData->width * (uint64_t)mapData->height)), "height array length must equal (mapWidth * mapHeight); actual length is: %" PRIu64"", arrayLen);
	for (uint32_t n = 0; n < N; ++n)
	{
		JSValue textureVal = JS_GetPropertyUint32(ctx, texture, n);
		auto free_texture_ref = gsl::finally([ctx, textureVal] { JS_FreeValue(ctx, textureVal); });
		JSValue heightVal = JS_GetPropertyUint32(ctx, height, n);
		auto free_height_ref = gsl::finally([ctx, heightVal] { JS_FreeValue(ctx, heightVal); });
		uint32_t textureUint32 = JSValueToUint32(ctx, textureVal);
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, textureUint32 <= (uint32_t)std::numeric_limits<uint16_t>::max(), "texture value exceeds uint16::max: %" PRIu32 "", textureUint32);
		mapData->mMapTiles[n].texture = static_cast<uint16_t>(textureUint32);
		mapData->mMapTiles[n].height = JSValueToUint32(ctx, heightVal);
	}
	bGotArrayLength = QuickJS_GetArrayLength(ctx, structures, arrayLen);
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen <= (uint64_t)std::numeric_limits<uint16_t>::max()), "structures array length must be <= uint16::max; actual length is: %" PRIu64"", arrayLen);
	uint16_t structureCount = static_cast<uint16_t>(arrayLen);
	auto mapStructures = data.map->mapStructures();
	for (uint16_t i = 0; i < structureCount; ++i)
	{
		JSValue structure = JS_GetPropertyUint32(ctx, structures, i);
		auto free_structure_ref = gsl::finally([ctx, structure] { JS_FreeValue(ctx, structure); });
		WzMap::Structure sd;
		sd.name = QuickJS_GetStdString(ctx, structure, "name");
		JSValue position = JS_GetPropertyStr(ctx, structure, "position");
		auto free_position_ref = gsl::finally([ctx, position] { JS_FreeValue(ctx, position); });
		if (JS_IsArray(ctx, position))
		{
			// [x, y]
			bGotArrayLength = QuickJS_GetArrayLength(ctx, position, arrayLen);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen == 2), "position array length must equal 2; actual length is: %" PRIu64"", arrayLen);
			JSValue xVal = JS_GetPropertyUint32(ctx, position, 0);
			JSValue yVal = JS_GetPropertyUint32(ctx, position, 1);
			sd.position.x = JSValueToInt32(ctx, xVal);
			sd.position.y = JSValueToInt32(ctx, yVal);
			JS_FreeValue(ctx, xVal);
			JS_FreeValue(ctx, yVal);
		}
		auto direction = QuickJS_GetInt32(ctx, structure, "direction");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, direction >= 0 && direction < static_cast<int32_t>(std::numeric_limits<uint16_t>::max()), "Invalid direction (%d) for structure", direction);
		sd.direction = static_cast<uint16_t>(direction);
		auto modules = QuickJS_GetUint32(ctx, structure, "modules");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, modules < static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()), "Invalid modules (%" PRIu32 ") for structure", modules);
		sd.modules = static_cast<uint8_t>(modules);
		auto player = QuickJS_GetInt32(ctx, structure, "player");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, sd.player >= -1 && sd.player < MAX_PLAYERS, "Invalid player (%d) for structure", (int)sd.player);
		sd.player = static_cast<int8_t>(player);
		mapStructures->push_back(std::move(sd));
	}
	bGotArrayLength = QuickJS_GetArrayLength(ctx, droids, arrayLen);
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen <= (uint64_t)std::numeric_limits<uint16_t>::max()), "droids array length be <= uint16::max; actual length is: %" PRIu64"", arrayLen);
	uint16_t droidCount = static_cast<uint16_t>(arrayLen);
	auto mapDroids = data.map->mapDroids();
	for (uint16_t i = 0; i < droidCount; ++i)
	{
		JSValue droid = JS_GetPropertyUint32(ctx, droids, i);
		auto free_droid_ref = gsl::finally([ctx, droid] { JS_FreeValue(ctx, droid); });
		WzMap::Droid sd;
		sd.name = QuickJS_GetStdString(ctx, droid, "name");
		JSValue position = JS_GetPropertyStr(ctx, droid, "position");
		auto free_position_ref = gsl::finally([ctx, position] { JS_FreeValue(ctx, position); });
		if (JS_IsArray(ctx, position))
		{
			// [x, y]
			bGotArrayLength = QuickJS_GetArrayLength(ctx, position, arrayLen);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen == 2), "position array length must equal 2; actual length is: %" PRIu64"", arrayLen);
			JSValue xVal = JS_GetPropertyUint32(ctx, position, 0);
			JSValue yVal = JS_GetPropertyUint32(ctx, position, 1);
			sd.position.x = JSValueToInt32(ctx, xVal);
			sd.position.y = JSValueToInt32(ctx, yVal);
			JS_FreeValue(ctx, xVal);
			JS_FreeValue(ctx, yVal);
		}
		auto direction = QuickJS_GetInt32(ctx, droid, "direction");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, direction >= 0 && direction < static_cast<int32_t>(std::numeric_limits<uint16_t>::max()), "Invalid direction (%d) for droid", direction);
		sd.direction = static_cast<uint16_t>(direction);
		auto player = QuickJS_GetInt32(ctx, droid, "player");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, sd.player >= -1 && sd.player < MAX_PLAYERS, "Invalid player (%d) for droid", (int)sd.player);
		sd.player = static_cast<int8_t>(player);
		mapDroids->push_back(std::move(sd));
	}
	bGotArrayLength = QuickJS_GetArrayLength(ctx, features, arrayLen);
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen <= (uint64_t)std::numeric_limits<uint16_t>::max()), "features array length be <= uint16::max; actual length is: %" PRIu64"", arrayLen);
	uint16_t featureCount = static_cast<uint16_t>(arrayLen);
	auto mapFeatures = data.map->mapFeatures();
	for (unsigned i = 0; i < featureCount; ++i)
	{
		JSValue feature = JS_GetPropertyUint32(ctx, features, i);
		auto free_feature_ref = gsl::finally([ctx, feature] { JS_FreeValue(ctx, feature); });
		WzMap::Feature sd;
		sd.name = QuickJS_GetStdString(ctx, feature, "name");
		JSValue position = JS_GetPropertyStr(ctx, feature, "position");
		auto free_position_ref = gsl::finally([ctx, position] { JS_FreeValue(ctx, position); });
		if (JS_IsArray(ctx, position))
		{
			// [x, y]
			bGotArrayLength = QuickJS_GetArrayLength(ctx, position, arrayLen);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotArrayLength && (arrayLen == 2), "position array length must equal 2; actual length is: %" PRIu64"", arrayLen);
			JSValue xVal = JS_GetPropertyUint32(ctx, position, 0);
			JSValue yVal = JS_GetPropertyUint32(ctx, position, 1);
			sd.position.x = JSValueToInt32(ctx, xVal);
			sd.position.y = JSValueToInt32(ctx, yVal);
			JS_FreeValue(ctx, xVal);
			JS_FreeValue(ctx, yVal);
		}
		auto direction = QuickJS_GetInt32(ctx, feature, "direction");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, direction >= 0 && direction < static_cast<int32_t>(std::numeric_limits<uint16_t>::max()), "Invalid direction (%d) for feature", direction);
		sd.direction = static_cast<uint16_t>(direction);
		mapFeatures->push_back(std::move(sd));
	}
	return JS_TRUE;
}

//MAP-- ## generateFractalValueNoise(width, height, range, crispness, scale, normalizeToRange = 0,
//MAP--                              [[x1, y1, x2, y2, (x, y, layerIdx, layerRange) => value], ...] = [])
//MAP--
//MAP-- A fast (native) fractal value noise generator, a cheaper version of Perlin noise.
//MAP-- Such noise can aid random map generation in a variety of ways, most prominently
//MAP-- for generating beautiful realistic landscapes.
//MAP--
//MAP-- This function returns a contiguous 2D array of size `(width * height)`
//MAP-- filled with random values that constitute fractal value noise. Such noise
//MAP-- is a sum of white noise layers with exponentially increasing resolution
//MAP-- and exponentially decreasing range, interpolated linearly when stretched
//MAP-- to the original (`width` x `height`) rectangle.
//MAP--
//MAP-- `range` is the range of the first layer. In other words, the first layer of noise
//MAP-- will have values populated via `gameRand(range)`.
//MAP--
//MAP-- `crispness` controls the range decay. Each next layer has range `(crispness/10)` times
//MAP-- the previous layer's range. Integer values from 1 to 10 typically make sense.
//MAP--
//MAP-- `scale` is the resolution of the first layer. For example, a `scale` of 16
//MAP-- means that the first layer would stretch every white noise pixel to
//MAP-- a 16x16 square in the final noise. Each next layer doubles the
//MAP-- resolution, so the noise with `scale` equal to 16 shall have 5 layers
//MAP-- in total, with pixel sizes 16, 8, 4, 2, 1 respectively.
//MAP--
//MAP-- `normalizedRange` is an optional argument that causes scales the final output
//MAP-- to be scaled to `[0, normalizeToRange)`. The maximum observed value of the resulting noise
//MAP-- becomes `normalizeToRange` and the minimum observed value becomes `0`. If
//MAP-- `normalizedRange` is unset or set to 0, normalization is not performed.
//MAP-- Specifying this parameter yields much better performance than manual normalization.
//MAP--
//MAP-- The last optional argument allows finer control over certain areas of the map.
//MAP-- Each element specifies such "rigged" rectangle [x1, x2] x [y1, y2] and the callback
//MAP-- that determines the value of each noise layer within that rectangle.
//MAP-- That value has to be within range [0, layerRange).
//MAP-- For example, if you want to make sure there's room for a player base
//MAP-- at the top left corner of the map, you can rig the respective rectangle
//MAP-- to always have a value of 0 by including an element, say,
//MAP-- `[0, 0, 32, 32, () => 0]`. This gives much nicer results than post-processing
//MAP-- the noise manually to reset these values to 0 because it causes the surrounding noise
//MAP-- to blend smoothly with the rigged area. The rigged area does not need to
//MAP-- be flat; an arbitrary function of `x` and `y` can be provided.
//MAP-- The callback shall not be stored for later use or called after
//MAP-- `generateFractalValueNoise` returns.
static JSValue runMap_generateFractalValueNoise(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	MAPSCRIPT_GET_CONTEXT_DATA()//;
	auto pCustomLogger = data.pCustomLogger;

	SCRIPT_ASSERT_AND_RETURNERROR(ctx, argc >= 5 && argc <= 7, "Must have 5 to 7 parameters");
	JSValue jsVal_width = argv[0];
	JSValue jsVal_height = argv[1];
	JSValue jsVal_range = argv[2];
	JSValue jsVal_crispness = argv[3];
	JSValue jsVal_scale = argv[4];

	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_width), "width must be number");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_height), "height must be number");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_range), "range must be number");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_crispness), "crispness must be number");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_scale), "scale must be number");

	uint32_t width = JSValueToUint32(ctx, jsVal_width);
	uint32_t height = JSValueToUint32(ctx, jsVal_height);
	uint32_t range = JSValueToUint32(ctx, jsVal_range);
	uint32_t scale = JSValueToUint32(ctx, jsVal_scale);
	uint32_t crispness = JSValueToUint32(ctx, jsVal_crispness);

	SCRIPT_ASSERT_AND_RETURNERROR(ctx, width > 0, "width must be > 0");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, height > 0, "height must be > 0");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, range > 0, "range must be > 0");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, scale > 0, "scale must be > 0");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, crispness > 0, "crispness must be > 0");

	SCRIPT_ASSERT_AND_RETURNERROR(ctx, width <= 256, "width must be <= 256");
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, height <= 256, "height must be <= 256");

	uint32_t normalizeToRange = 0;
	if (argc >= 6) {
		JSValue jsVal_normalizeToRange = argv[5];
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_normalizeToRange),
		                              "normalizeToRange must be number");
		int64_t normalizeToRange_int64 = 0;
		int result = JS_ToInt64(ctx, &normalizeToRange_int64, jsVal_normalizeToRange);
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, result == 0,
									  "JS_ToInt64 failed - normalizeToRange must be an integer >= 0");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, normalizeToRange_int64 >= 0 && normalizeToRange_int64 <= UINT32_MAX,
		                              "normalizeToRange must be positive or zero");
		normalizeToRange = static_cast<uint32_t>(normalizeToRange_int64);
	}

	struct RiggedRegion
	{
		uint32_t x1, y1, x2, y2;
		JSValue callback;
	};

	std::vector<RiggedRegion> riggedRegions;
	if (argc == 7)
	{
		// Unwrap JS regions for faster lookup.
		JSValue jsVal_riggedRegions = argv[6];
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsArray(ctx, jsVal_riggedRegions), "width must be array");

		uint64_t regionsArrayLength = 0;
		bool bGotRegionsArrayLength = QuickJS_GetArrayLength(ctx, jsVal_riggedRegions, regionsArrayLength);
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotRegionsArrayLength,
		                              "Unable to retrieve riggedRegions array length");
		SCRIPT_ASSERT_AND_RETURNERROR(ctx, regionsArrayLength <= UINT16_MAX,
		                              "Too many riggedRegions (%" PRIu64 ")", regionsArrayLength);

		size_t numRiggedRegions = static_cast<size_t>(regionsArrayLength);
		riggedRegions.reserve(numRiggedRegions);
		auto free_riggedRegions_ref = gsl::finally([ctx, &riggedRegions]
		{
			for (const auto &r : riggedRegions)
			{
				JS_FreeValue(ctx, r.callback);
			}
		});

		for (uint32_t i = 0; i < numRiggedRegions; ++i)
		{
			JSValue jsVal_riggedRegion = JS_GetPropertyUint32(ctx, jsVal_riggedRegions, i);
			auto free_riggedRegion_ref = gsl::finally([ctx, jsVal_riggedRegion] { JS_FreeValue(ctx, jsVal_riggedRegion); });
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsArray(ctx, jsVal_riggedRegion),
			                              "riggedRegion %" PRIu32 " must be array", i);
			uint64_t regionArrayLength = 0;
			bool bGotRegionArrayLength = QuickJS_GetArrayLength(ctx, jsVal_riggedRegion, regionArrayLength);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, bGotRegionArrayLength && regionArrayLength == 5,
			                              "riggedRegion[%" PRIu32 "] length must be 5; actual length %" PRIu64,
			                              i, regionArrayLength);
			JSValue jsVal_x1 = JS_GetPropertyUint32(ctx, jsVal_riggedRegion, 0);
			JSValue jsVal_y1 = JS_GetPropertyUint32(ctx, jsVal_riggedRegion, 1);
			JSValue jsVal_x2 = JS_GetPropertyUint32(ctx, jsVal_riggedRegion, 2);
			JSValue jsVal_y2 = JS_GetPropertyUint32(ctx, jsVal_riggedRegion, 3);
			JSValue jsVal_callback = JS_GetPropertyUint32(ctx, jsVal_riggedRegion, 4);

			auto free_riggedRegionValues_ref = gsl::finally([ctx, jsVal_x1, jsVal_y1, jsVal_x2, jsVal_y2, jsVal_callback]
			{
				JS_FreeValue(ctx, jsVal_x1);
				JS_FreeValue(ctx, jsVal_y1);
				JS_FreeValue(ctx, jsVal_x2);
				JS_FreeValue(ctx, jsVal_y2);
				JS_FreeValue(ctx, jsVal_callback);
			});

			SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_x1), "riggedRegion[%" PRIu32 "][0] must be number", i);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_y1), "riggedRegion[%" PRIu32 "][1] must be number", i);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_x2), "riggedRegion[%" PRIu32 "][2] must be number", i);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_y2), "riggedRegion[%" PRIu32 "][3] must be number", i);
			SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsFunction(ctx, jsVal_callback),
			                              "riggedRegion[%" PRIu32 "][4] must be function", i);

			riggedRegions.push_back(RiggedRegion
			{
				JSValueToUint32(ctx, jsVal_x1),
				JSValueToUint32(ctx, jsVal_y1),
				JSValueToUint32(ctx, jsVal_x2),
				JSValueToUint32(ctx, jsVal_y2),
				JS_DupValue(ctx, jsVal_callback)
			});
		};
	}

	uint32_t size = width * height;
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, size <= UINT16_MAX,
	                              "Requested data too large to fit into Array");

	uint32_t maxLayerSize = (width + 1) * (height + 1);

	// This check covers a lot of other multiplications.
	SCRIPT_ASSERT_AND_RETURNERROR(ctx, maxLayerSize / (width + 1) == height + 1, "integer overflow");

	std::vector<uint32_t> noiseData(size, 0);

	// Allocate layer data once and reuse it for performance.
	std::vector<uint32_t> layerData(maxLayerSize);

	uint32_t layerScale = scale;
	uint32_t layerRange = range;
	uint32_t layerIdx = -1;

	do
	{
		layerScale /= 2;
		layerRange = layerRange * crispness / 10;
		++layerIdx;

		uint32_t layerWidth = width / layerScale + 1;
		uint32_t layerHeight = height / layerScale + 1;

		uint32_t layerScaleArea = layerScale * layerScale;

		for (uint32_t x = 0; x < layerWidth; ++x)
		{
			for (uint32_t y = 0; y < layerHeight; ++y)
			{
				uint32_t mapX = x * layerScale, mapY = y * layerScale;

				bool rigged = false;
				for (const auto &r : riggedRegions)
				{
					if (mapX >= r.x1 && mapY >= r.y1 && mapX <= r.x2 && mapY <= r.y2)
					{
						rigged = true;
						JSValue callArgv[4] =
						{
							JS_NewUint32(ctx, mapX),
							JS_NewUint32(ctx, mapY),
							JS_NewUint32(ctx, layerIdx),
							JS_NewUint32(ctx, layerRange)
						};
						JSValue jsVal_ret = JS_Call(ctx, r.callback, this_val, 4, callArgv);
						auto free_ret_ref = gsl::finally([ctx, jsVal_ret] { JS_FreeValue(ctx, jsVal_ret); });
						SCRIPT_ASSERT_AND_RETURNERROR(ctx, JS_IsNumber(jsVal_ret),
							"riggedRegion callback must return number");
						uint32_t ret = JSValueToUint32(ctx, jsVal_ret);
						SCRIPT_ASSERT_AND_RETURNERROR(ctx, ret < layerRange,
							"riggedRegion callback must return non-negative number within range");
						layerData[x * layerHeight + y] = ret;
					}
				}

				if (!rigged)
				{
					// This is equivalent to gameRand(layerRange).
					uint32_t num = data.mt() % layerRange;
					layerData[x * layerHeight + y] = num;
				}
			}
		}


		for (uint32_t x = 0; x < layerWidth - 1; ++x)
		{
			for (uint32_t y = 0; y < layerHeight - 1; ++y)
			{
				uint32_t mapX = x * layerScale, mapY = y * layerScale;

				uint32_t tl = layerData[x * layerHeight + y];
				uint32_t tr = layerData[(x + 1) * layerHeight + y];
				uint32_t bl = layerData[x * layerHeight + (y + 1)];
				uint32_t br = layerData[(x + 1) * layerHeight + (y + 1)];

				for (uint32_t innerX = 0; innerX < layerScale; ++innerX)
				{
					for (uint32_t innerY = 0; innerY < layerScale; ++innerY)
					{
						// Interpolation.
						noiseData[(mapX + innerX) * height + (mapY + innerY)] += (
							br * innerX * innerY +
							bl * (layerScale - 1 - innerX) * innerY +
							tr * innerX * (layerScale - 1 - innerY) +
							tl * (layerScale - 1 - innerX) * (layerScale - 1 - innerY)
						) / layerScaleArea;
					}
				}
			}
		}
	}
	while (layerScale > 1 && layerRange > 1);

	// Normalization.
	if (normalizeToRange > 0)
	{
		auto minmax = std::minmax_element(noiseData.begin(), noiseData.end());
		uint32_t min = *minmax.first;
		uint32_t max = *minmax.second;
		std::transform(
			noiseData.begin(), noiseData.end(), noiseData.begin(),
			[min, max, normalizeToRange] (uint32_t val)
			{
				return (val - min) * normalizeToRange / (max - min);
			});
	}

	JSValue retVal = JS_NewArray(ctx);
	for (uint32_t i = 0; i < size; ++i)
	{
		JS_SetPropertyUint32(ctx, retVal, i, JS_NewUint32(ctx, noiseData[i]));
	}
	return retVal;
}

struct MapScriptRuntimeInfo {
	std::chrono::system_clock::time_point startTime;
};

#define MAX_MAPSCRIPT_RUNTIME_SECONDS 30

/* return != 0 if the JS code needs to be interrupted */
static int runMapScript_InterruptHandler(JSRuntime *rt, void *opaque)
{
	assert(opaque != nullptr); // "Null opaque pointer?"
	const MapScriptRuntimeInfo* pInfo = static_cast<const MapScriptRuntimeInfo*>(opaque);
	auto secondsElapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - pInfo->startTime).count();
	return (secondsElapsed >= MAX_MAPSCRIPT_RUNTIME_SECONDS);
}

static LoggingProtocol* pRuntimeFree_CustomLogger = nullptr;
static void QJSRuntimeFree_LeakHandler_Warning(const char* msg)
{
	debug(pRuntimeFree_CustomLogger, LOG_WARNING, "QuickJS FreeRuntime leak: %s", msg);
}

std::shared_ptr<Map> runMapScript(const std::vector<char>& fileBuffer, const std::string &path, uint32_t seed, bool preview, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	ScriptMapData data;
	data.mt = std::mt19937(seed);
	data.pCustomLogger = pCustomLogger;

	JSRuntime *rt = JS_NewRuntime();
	if (rt == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "JS_NewRuntime failed");
		return nullptr;
	}
	auto free_runtime_ref = gsl::finally([rt, pCustomLogger] {
		pRuntimeFree_CustomLogger = pCustomLogger;
		JS_FreeRuntime2(rt, QJSRuntimeFree_LeakHandler_Warning);
		pRuntimeFree_CustomLogger = nullptr;
	});
	JSLimitedContextOptions ctxOptions;
	ctxOptions.baseObjects = true;
	ctxOptions.dateObject = false;
	ctxOptions.eval = true; // required for JS_Eval to work
	ctxOptions.stringNormalize = false;
	ctxOptions.regExp = false;
	ctxOptions.json = false;
	ctxOptions.proxy = false;
	ctxOptions.mapSet = true;
	ctxOptions.typedArrays = false;
	ctxOptions.promise = false;
	JSContext *ctx = JS_NewLimitedContext(rt, &ctxOptions);
	if (ctx == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "JS_NewLimitedContext failed");
		return nullptr;
	}
	auto free_context_ref = gsl::finally([ctx] { JS_FreeContext(ctx); });
	JSValue global_obj = JS_GetGlobalObject(ctx);
	auto free_globalobj_ref = gsl::finally([ctx, global_obj] { JS_FreeValue(ctx, global_obj); });

	if (fileBuffer.empty())
	{
		debug(pCustomLogger, LOG_ERROR, "fileBuffer is empty");
		return nullptr;
	}
	// ensure fileBuffer is null-terminated, but ignore the null terminator when calculating the size
	if (fileBuffer.back() != 0)
	{
		debug(pCustomLogger, LOG_ERROR, "fileBuffer must be null-terminated");
		return nullptr;
	}
	size_t fileBufLen = fileBuffer.size() - 1;
	JSValue compiledScriptObj = JS_Eval(ctx, fileBuffer.data(), fileBufLen, path.c_str(), JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
	if (JS_IsException(compiledScriptObj))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(pCustomLogger, LOG_ERROR, "Syntax / compilation error in %s: %s", path.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, compiledScriptObj);
		compiledScriptObj = JS_UNINITIALIZED;
		return nullptr;
	}

	JS_DefinePropertyValueStr(ctx, global_obj, "preview", JS_NewBool(ctx, preview), 0);
	JS_DefinePropertyValueStr(ctx, global_obj, "XFLIP", JS_NewInt32(ctx, TILE_XFLIP), 0);
	JS_DefinePropertyValueStr(ctx, global_obj, "YFLIP", JS_NewInt32(ctx, TILE_YFLIP), 0);
	JS_DefinePropertyValueStr(ctx, global_obj, "ROTMASK", JS_NewInt32(ctx, TILE_ROTMASK), 0);
	JS_DefinePropertyValueStr(ctx, global_obj, "ROTSHIFT", JS_NewInt32(ctx, TILE_ROTSHIFT), 0);
	JS_DefinePropertyValueStr(ctx, global_obj, "TRIFLIP", JS_NewInt32(ctx, TILE_TRIFLIP), 0);
	JS_SetContextOpaque(ctx, &data);

	// Special mapScript functions
	static const JSCFunctionListEntry js_builtin_mapFuncs[] = {
		QJS_CFUNC_DEF("gameRand", 0, runMap_gameRand ),
		QJS_CFUNC_DEF("log", 1, runMap_log ),
		QJS_CFUNC_DEF("setMapData", 7, runMap_setMapData ),
		QJS_CFUNC_DEF("generateFractalValueNoise", 6, runMap_generateFractalValueNoise )
	};
	JS_SetPropertyFunctionList(ctx, global_obj, js_builtin_mapFuncs, sizeof(js_builtin_mapFuncs) / sizeof(js_builtin_mapFuncs[0]));

	// configure limitations on runtime
	JS_SetMaxStackSize(rt, 512*1024);
	JS_SetMemoryLimit(rt, 100*1024*1024); // 100MiB better be enough...

	// install interrupt handler to prevent endless script execution (due to bugs or otherwise)
	MapScriptRuntimeInfo mapScriptRuntimeInfo;
	mapScriptRuntimeInfo.startTime = std::chrono::system_clock::now();
	JS_SetInterruptHandler(rt, runMapScript_InterruptHandler, &mapScriptRuntimeInfo);

	// run the map script
	JSValue result = JS_EvalFunction(ctx, compiledScriptObj);
	compiledScriptObj = JS_UNINITIALIZED;
	JS_SetInterruptHandler(rt, nullptr, nullptr);

	if (JS_IsException(result))
	{
		// compilation error / syntax error / runtime error
		std::string errorAsString = QuickJS_DumpError(ctx);
		debug(pCustomLogger, LOG_ERROR, "Uncaught exception in %s: %s", path.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, result);
		return nullptr;
	}

	JS_FreeValue(ctx, result);
	return std::move(data.map);
}

} // namespace WzMap
