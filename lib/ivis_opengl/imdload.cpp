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

/*!
 * \file imdload.c
 *
 * Load IMD (.pie) files
 */

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <array>

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/pienormalize.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"

#include "ivisdef.h" // for imd structures
#include "imd.h" // for imd structures
#include "tex.h" // texture page loading

#include <glm/vec4.hpp>
using Vector4f = glm::vec4;

// The maximum polygons for a model is influenced by several factors:
// 1. The vertexCount *must* be less than UINT16_MAX (65535), because this special vertex index value is treated as restarting the assembly of primitives in some cases (example: Vulkan on MoltenVK) - NOTE: we currently use index_type::u16 for our model VBO_INDEX buffers
// 2. Given how the game world and models are displayed, using a very large number of polygons is effectively a waste
//#define MAX_PIE_POLYGONS 21844 // floor((UINT16_MAX-1) / 3)
#define MAX_PIE_POLYGONS 8192
static_assert(MAX_PIE_POLYGONS <= ((UINT16_MAX-1) / 3), "MAX_PIE_POLYGONS must not currently exceed floor((UINT16_MAX-1) / 3)");

// Scale animation numbers from int to float
#define INT_SCALE       1000

typedef std::unordered_map<std::string, std::unique_ptr<iIMDBaseShape>> ModelMap;
static ModelMap models;
static size_t currentTilesetIdx = 0;

static size_t modelLoadingErrors = 0;
static size_t modelTextureLoadingFailures = 0;

static std::unique_ptr<iIMDShape> iV_ProcessIMD(const WzString &filename, const char **ppFileData, const char *FileDataEnd, bool skipGPUData, bool skipDuplicateLoadChecks = false);
static bool _imd_load_level_textures(const iIMDShape& s, size_t tilesetIdx, iIMDShapeTextures& output);
static std::unique_ptr<iIMDShape> tryLoadDisplayModelInternal(const WzString &path, const WzString &filename, bool skipGPUupload, bool skipDuplicateLoadChecks = false);

iIMDShape::~iIMDShape()
{
	freeInternalResources();
}

void iIMDShape::freeInternalResources()
{
	free(shadowEdgeList);
	shadowEdgeList = nullptr;
	for (auto& buffer : buffers)
	{
		delete buffer;
		buffer = nullptr;
	}
}

iIMDShape& iIMDShape::operator=(iIMDShape&& other) noexcept
{
	if (this != &other)
	{
		// Release existing resources
		freeInternalResources();

		// Transfer resources from other
		std::swap(min, other.min);
		std::swap(max, other.max);
		std::swap(sradius, other.sradius);
		std::swap(radius, other.radius);
		std::swap(ocen, other.ocen);
		std::swap(connectors, other.connectors);
		std::swap(flags, other.flags);
		std::swap(numFrames, other.numFrames);
		std::swap(animInterval, other.animInterval);
		std::swap(shadowEdgeList, other.shadowEdgeList);
		std::swap(nShadowEdges, other.nShadowEdges);
		std::swap(points, other.points);
		std::swap(polys, other.polys);
		std::swap(altShadowPoints, other.altShadowPoints);
		std::swap(altShadowPolys, other.altShadowPolys);
		if (!altShadowPoints.empty() && !altShadowPolys.empty())
		{
			pShadowPoints = &altShadowPoints;
			pShadowPolys = &altShadowPolys;
		}
		else
		{
			pShadowPoints = &points;
			pShadowPolys = &polys;
		}
		std::swap(buffers, other.buffers);
		std::swap(vertexCount, other.vertexCount);
		std::swap(objanimdata, other.objanimdata);
		std::swap(objanimframes, objanimframes);
		std::swap(objanimtime, other.objanimtime);
		std::swap(objanimcycles, other.objanimcycles);
		std::swap(objanimpie, other.objanimpie);
		std::swap(interpolate, other.interpolate);
		std::swap(modelName, other.modelName);
		std::swap(modelLevel, other.modelLevel);
		std::swap(tilesetTextureFiles, other.tilesetTextureFiles);
		std::swap(next, other.next);
		std::swap(m_textures, other.m_textures);

		// Reset other's resources
		other.shadowEdgeList = nullptr;
		for (auto& buffer : other.buffers)
		{
			buffer = nullptr;
		}
	}
	return *this;
}

const iIMDShapeTextures& iIMDShape::getTextures() const
{
	if (!m_textures->initialized)
	{
		// Load the textures on-demand
		if (!_imd_load_level_textures(*this, currentTilesetIdx, *m_textures))
		{
			++modelTextureLoadingFailures;
		}
		m_textures->initialized = true;
	}

	return *m_textures.get();
}

PIELIGHT iIMDShape::getTeamColourForModel(int team) const
{
	return pal_GetTeamColour(team);
}

void iIMDShape::reloadTexturesIfLoaded()
{
	if (!m_textures->initialized)
	{
		return;
	}
	m_textures->texpage = iV_TEX_INVALID;
	m_textures->tcmaskpage = iV_TEX_INVALID;
	m_textures->normalpage = iV_TEX_INVALID;
	m_textures->specularpage = iV_TEX_INVALID;
	_imd_load_level_textures(*this, currentTilesetIdx, *m_textures);
	m_textures->initialized = true;
}

iIMDBaseShape::iIMDBaseShape(std::unique_ptr<iIMDShape> a, const WzString &path, const WzString &filename)
: min(a->min)
, max(a->max)
, sradius(a->sradius)
, radius(a->radius)
, ocen(a->ocen)
, connectors(a->connectors)
, path(path)
, filename(filename)
, m_displayModel(std::move(a))
{ }

iIMDBaseShape::~iIMDBaseShape()
{
	// currently nothing else to do
}

static void fixupDisplayModelForBaseModel(const iIMDBaseShape& baseModel, const std::unique_ptr<iIMDShape>& newDisplayModel)
{
	// first, some sanity checks
	const auto& baseModelConnectors = baseModel.connectors;
	if (newDisplayModel->connectors.size() < baseModelConnectors.size())
	{
		// override models should not have fewer connectors than the base model
		size_t prior_override_model_connectors_count = newDisplayModel->connectors.size();
		// to avoid crashes later, copy over the extra connectors from the base model
		for (size_t i = newDisplayModel->connectors.size(); i < baseModelConnectors.size(); ++i)
		{
			newDisplayModel->connectors.push_back(baseModelConnectors[i]);
		}
		// also output a log entry, so someone can know to fix the graphics override model
		const WzString& filename = baseModel.filename;
		debug(LOG_INFO, "Graphics override model %s is missing connectors (override connectors: %zu, base model connectors: %zu)", filename.toUtf8().c_str(), prior_override_model_connectors_count, baseModelConnectors.size());
	}
}

void iIMDBaseShape::replaceDisplayModel(std::unique_ptr<iIMDShape> newDisplayModel)
{
	fixupDisplayModelForBaseModel(*this, newDisplayModel);

	m_displayModel = std::move(newDisplayModel);
}

bool iIMDBaseShape::debugReloadDisplayModel()
{
	return debugReloadDisplayModelInternal(true);
}

bool iIMDBaseShape::debugReloadDisplayModelInternal(bool recurseAnimPie)
{
	ASSERT_OR_RETURN(false , m_displayModel != nullptr, "No display model??");

	// first, try to load a graphics override - at the same path but with a prefix
	auto newDisplayModel = tryLoadDisplayModelInternal(WZ_CURRENT_GRAPHICS_OVERRIDES_PREFIX "/" + path, filename, false, true);

	if (!newDisplayModel)
	{
		// load model from base path
		newDisplayModel = tryLoadDisplayModelInternal(path, filename, false, true);
	}

	if (!newDisplayModel)
	{
		debug(LOG_ERROR, "Failed to reload display model?: %s%s", path.toUtf8().c_str(), filename.toUtf8().c_str());
		return false;
	}

	fixupDisplayModelForBaseModel(*this, newDisplayModel);

	// swap out the display model - use move assignment to avoid invalidating any cached pointers to the displaymodel iIMDShape* (as this function is likely called at runtime)
	*m_displayModel = std::move(*newDisplayModel);

	// also reload the display models for any objanimpie in the newDisplayModel (as tryLoadDisplayModelInternal will not force-reload them if their names are equal to a previously-loaded model)
	if (recurseAnimPie)
	{
		for (int i = 0; i < ANIM_EVENT_COUNT; i++)
		{
			if (m_displayModel->objanimpie[i] != nullptr)
			{
				(m_displayModel->objanimpie[i])->debugReloadDisplayModelInternal(false);
			}
		}
	}

	return true;
}

size_t getModelLoadingErrorCount()
{
	return modelLoadingErrors;
}

size_t getModelTextureLoadingFailuresCount()
{
	return modelTextureLoadingFailures;
}

void modelShutdown()
{
	models.clear();
	modelLoadingErrors = 0;
	modelTextureLoadingFailures = 0;
}

void modelReloadAllModelTextures()
{
	std::unordered_set<size_t> texPagesToReloadFromDisk;
	enumerateLoadedModels([&texPagesToReloadFromDisk](const std::string &modelName, iIMDBaseShape &s){
		for (iIMDShape *pDisplayShape = s.mutableDisplayModel(); pDisplayShape != nullptr; pDisplayShape = pDisplayShape->next.get())
		{
			const iIMDShapeTextures& textures = pDisplayShape->getTextures();
			if (!textures.initialized)
			{
				continue;
			}
			std::array<size_t, 4> pages = {textures.texpage, textures.tcmaskpage, textures.normalpage, textures.specularpage};
			for (auto page : pages)
			{
				if (page != iV_TEX_INVALID)
				{
					texPagesToReloadFromDisk.insert(page);
				}
			}
		}
	});
	debugReloadTexturesFromDisk(texPagesToReloadFromDisk);
}

bool debugReloadDisplayModelsForBaseModel(iIMDBaseShape& baseModel)
{
	return baseModel.debugReloadDisplayModel();
}

void debugReloadAllDisplayModels()
{
	enumerateLoadedModels([](const std::string &modelName, iIMDBaseShape &s){
		s.debugReloadDisplayModelInternal(false);
	});
}

void modelUpdateTilesetIdx(size_t tilesetIdx)
{
	if (tilesetIdx == currentTilesetIdx)
	{
		return;
	}
	currentTilesetIdx = tilesetIdx;
	// reload all initialized model textures
	enumerateLoadedModels([](const std::string &modelName, iIMDBaseShape &s){
		for (iIMDShape *pDisplayShape = s.mutableDisplayModel(); pDisplayShape != nullptr; pDisplayShape = pDisplayShape->next.get())
		{
			pDisplayShape->reloadTexturesIfLoaded();
		}
	});
}

void enumerateLoadedModels(const std::function<void (const std::string& modelName, iIMDBaseShape& model)>& func)
{
	for (auto& keyvaluepair : models)
	{
		func(keyvaluepair.first, *keyvaluepair.second.get());
	}
}

static std::unique_ptr<iIMDShape> tryLoadDisplayModelInternal(const WzString &path, const WzString &filename, bool skipGPUupload, bool skipDuplicateLoadChecks)
{
	if (PHYSFS_exists(path + filename))
	{
		char *pFileData = nullptr, *fileEnd;
		UDWORD size = 0;
		if (!loadFile(WzString(path + filename).toUtf8().c_str(), &pFileData, &size))
		{
			debug(LOG_ERROR, "Failed to load model file: %s", WzString(path + filename).toUtf8().c_str());
			return nullptr;
		}
		fileEnd = pFileData + size;
		const char *pFileDataPt = pFileData;
		auto result = iV_ProcessIMD(filename, (const char **)&pFileDataPt, fileEnd, skipGPUupload, skipDuplicateLoadChecks);
		free(pFileData);
		return result;
	}
	return nullptr;
}

bool tryLoad(const WzString &path, const WzString &filename)
{
	if (!PHYSFS_exists(path + filename))
	{
		// base model does not exist
		return false;
	}

	// first, try to load a graphics override - at the same path but with a prefix
	auto graphics_override_model = tryLoadDisplayModelInternal(WZ_CURRENT_GRAPHICS_OVERRIDES_PREFIX "/" + path, filename, false);

	// try to load base model
	auto baseModel = tryLoadDisplayModelInternal(path, filename, (graphics_override_model != nullptr));
	if (!baseModel)
	{
		return false;
	}

	// create BaseShape from full (base) model (first level)
	// the iIMDBaseShape then "owns" the display model iIMDShape
	auto modelName = baseModel->modelName;
	auto baseInsertResult = models.insert(ModelMap::value_type(modelName.toUtf8(), std::make_unique<iIMDBaseShape>(std::move(baseModel), path, filename)));
	ASSERT_OR_RETURN(false, baseInsertResult.second, "%s: Loaded duplicate model? (%s)", filename.toUtf8().c_str(), modelName.toUtf8().c_str());
	// do NOT use baseModel after this point!

	if (!graphics_override_model)
	{
		// no graphics override - just use the base model as the display model (the default)
		return true;
	}

	// there *is* a graphics override version of this model - swap out the base model's displayModel for the graphics override model
	baseInsertResult.first->second->replaceDisplayModel(std::move(graphics_override_model));
	return true;
}

const WzString &modelName(const iIMDShape *model)
{
	if (model != nullptr)
	{
		ASSERT(!model->modelName.isEmpty(), "An IMD pointer could not be backtraced to a filename!");
		if (model->modelName.isEmpty())
		{
			debug(LOG_INFO, "Found one");
		}
		return model->modelName;
	}
	ASSERT(false, "Null IMD pointer??");
	static WzString error;
	return error;
}

iIMDBaseShape *modelGet(const WzString &filename)
{
	WzString name(filename.toLower());
	auto it = models.find(name.toStdString());
	if (it != models.end())
	{
		return it->second.get(); // cached
	}
	else if (tryLoad("structs/", name) || tryLoad("misc/", name) || tryLoad("effects/", name)
	         || tryLoad("components/prop/", name) || tryLoad("components/weapons/", name)
	         || tryLoad("components/bodies/", name) || tryLoad("features/", name)
	         || tryLoad("misc/micnum/", name) || tryLoad("misc/minum/", name) || tryLoad("misc/mivnum/", name) || tryLoad("misc/researchimds/", name))
	{
		return models.at(name.toStdString()).get();
	}
	debug(LOG_ERROR, "Could not find: %s", name.toUtf8().c_str());
	return nullptr;
}

struct IMD_Line
{
	std::string lineContents;
	const char *pNextLineBegin = nullptr; // a pointer to the data after this line (including past any newline characters)

	void reset()
	{
		lineContents.clear();
		pNextLineBegin = nullptr;
	}
};

static bool _imd_get_next_line(const char *pFileData, const char *FileDataEnd, IMD_Line& output)
{
	if (pFileData == nullptr) { output.reset(); return false; }
	const char *pCurrLineBegin = pFileData;
	const char *pLineEnd = nullptr;
	const char *pNextLineBegin = nullptr;
	const char *pLineCommentStarted = nullptr;

	bool getNextLine = false;
	bool foundNonWhitespace = false; // found non whitespace, non-comment characters
	do
	{
		for (const char* pCurr = pCurrLineBegin; pCurr != FileDataEnd; ++pCurr)
		{
			if (*pCurr == '\n' || *pCurr == '\r')
			{
				if (!pLineEnd) { pLineEnd = pCurr; }
			}
			else if (pLineEnd != nullptr)
			{
				// found a prior newline, and now found the beginning of the next line
				pNextLineBegin = pCurr;
				break;
			}
			else if (*pCurr == '#')
			{
				if (!pLineCommentStarted)
				{
					if (foundNonWhitespace)
					{
						pLineCommentStarted = pCurr;
					}
					else
					{
						// ignore whitespace before a '#' that effectively starts a line
						pLineCommentStarted = pCurrLineBegin;
					}
				}
			}
			else if (*pCurr != ' ' && *pCurr != '\t')
			{
				// found non-whitespace character
				if (!pLineCommentStarted)
				{
					foundNonWhitespace = true;
				}
			}
		}

		if (pNextLineBegin == nullptr)
		{
			pNextLineBegin = FileDataEnd;
		}

		if (pLineCommentStarted && pLineCommentStarted == pCurrLineBegin)
		{
			// skip lines that begin with a '#'
			if (!pLineEnd)
			{
				// last line was a comment - ignore
				output.reset();
				return false;
			}
			pCurrLineBegin = pLineEnd;
			pLineEnd = nullptr;
			pNextLineBegin = nullptr;
			pLineCommentStarted = nullptr;
			getNextLine = true;
			foundNonWhitespace = false;
		}
	} while (getNextLine);

	if (pLineCommentStarted)
	{
		pLineEnd = pLineCommentStarted;
	}

	if (!pLineEnd)
	{
		if (foundNonWhitespace)
		{
			// last line had actual data on it
			pLineEnd = FileDataEnd;
			pNextLineBegin = FileDataEnd;
		}
		else
		{
			output.reset();
			return false; // no more lines
		}
	}
	output.lineContents.assign(pFileData, (pLineEnd - pFileData));
	output.pNextLineBegin = pNextLineBegin;
	return true;
}

typedef std::unordered_map<std::string, std::string> LevelTextureFilesMapping;
typedef std::array<LevelTextureFilesMapping, 3> LevelTextureFiles;

struct LevelSettings
{
	// TYPE
	optional<uint32_t> imd_flags;

	// INTERPOLATE
	optional<bool> interpolate;

	// TEXTURES
	LevelTextureFiles tilesetTextureFiles;
};

static bool _imd_load_texture_command(const WzString &filename, const IMD_Line &lineToProcess, const char* command, int value, int cnt, const char* expectedCommand, LevelSettings &levelSettings)
{
	if (strcmp(command, expectedCommand) != 0)
	{
		return false;
	}

	if (value < 0 || value >= levelSettings.tilesetTextureFiles.size())
	{
		debug(LOG_ERROR, "%s: %s first parameter (%d) is invalid", filename.toUtf8().c_str(), expectedCommand, value);
		return false;
	}

	const char* pRestOfLine = lineToProcess.lineContents.c_str() + cnt;
	char buffer[PATH_MAX] = {};

	/* the first parameter for textures is always ignored; which is why we ignore
	 * value read in above */

	if (sscanf(pRestOfLine, "%255s%n", buffer, &cnt) != 1)
	{
		debug(LOG_ERROR, "%s: Texture info corrupt: %s", filename.toUtf8().c_str(), buffer);
		return false;
	}

	// verify the extension is .png
	std::string outputFilename = buffer;
	if (!strEndsWith(outputFilename, ".png"))
	{
		debug(LOG_ERROR, "%s: Only png textures supported", filename.toUtf8().c_str());
		outputFilename.clear();
		return false;
	}

	levelSettings.tilesetTextureFiles[value][expectedCommand] = outputFilename;

	pRestOfLine += cnt;

	// ignoring old texture width / height directives (which were only for TEXTURE lines)

	return true;
}

static bool _imd_load_level_settings(const WzString &filename, const char **ppFileData, const char *FileDataEnd, int pieVersion, bool globalScope, LevelSettings &levelSettings)
{
	const char *pFileData = *ppFileData;
	char buffer[PATH_MAX] = {'\0'};
	int cnt = 0, value = 0, scanResult = 0;
	unsigned uvalue = 0;

	// Every line always starts with <string> <number>
	// The commands that this function handles (per-level options) are:
	// - TYPE
	// - INTERPOLATE
	// - TEXTURE
	// - TCMASK
	// - NORMALMAP
	// - SPECULARMAP
	// They are expected in the order above (although all lines are optional, except TYPE - which is currently required at the global scope)
	IMD_Line lineToProcess;

	// TYPE <hex number>
	if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
	{
		if (globalScope)
		{
			// TYPE is required in the global scope
			debug(LOG_ERROR, "%s: Expecting TYPE: %s", filename.toUtf8().c_str(), buffer);
			return false;
		}
		else
		{
			return true;
		}
	}
	if ((scanResult = sscanf(lineToProcess.lineContents.c_str(), "%255s %x", buffer, &uvalue)) != 2 || strncmp(buffer, "TYPE", 4) != 0)
	{
		if (globalScope)
		{
			// TYPE is required in the global scope
			debug(LOG_ERROR, "%s: Expecting TYPE: %s", filename.toUtf8().c_str(), buffer);
			return false;
		}
		else
		{
			// non-global scope does not require it - just continue looking for other commands
			lineToProcess.pNextLineBegin = pFileData; // reset to re-examine this line with the standard directive format
		}
	}
	else
	{
		// found TYPE
		levelSettings.imd_flags = uvalue;
	}

	auto getNextPossibleLevelSettingLine = [&]() -> bool {
		pFileData = lineToProcess.pNextLineBegin;
		*ppFileData = pFileData;
		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			return false;
		}
		scanResult = sscanf(lineToProcess.lineContents.c_str(), "%255s %d%n", buffer, &value, &cnt);
		if (scanResult != 2)
		{
			// does not fit format of level settings commands
			return false;
		}
		return true;
	};

	if (!getNextPossibleLevelSettingLine())
	{
		return true;
	}

	// INTERPOLATE
	if (strncmp(buffer, "INTERPOLATE", 11) == 0)
	{
		levelSettings.interpolate = (value > 0);
		if (!getNextPossibleLevelSettingLine())
		{
			return true;
		}
	}

	do {
		// TEXTURE
		if (_imd_load_texture_command(filename, lineToProcess, buffer, value, cnt, "TEXTURE", levelSettings))
		{
			if (!getNextPossibleLevelSettingLine())
			{
				return true;
			}
			if (pieVersion >= PIE_VER_4)
			{
				continue; // loop to allow multiple TEXTURE, TCMASK, NORMALMAP, and SPECULARMAP commands
			}
		}

		if (pieVersion >= PIE_VER_4)
		{
			// TCMASK (WZ 4.4+ only - earlier versions automatically generated a filename based on the TEXTURE filename)
			if (_imd_load_texture_command(filename, lineToProcess, buffer, value, cnt, "TCMASK", levelSettings))
			{
				if (!getNextPossibleLevelSettingLine())
				{
					return true;
				}
				if (pieVersion >= PIE_VER_4)
				{
					continue; // loop to allow multiple TEXTURE, TCMASK, NORMALMAP, and SPECULARMAP commands
				}
			}
		}

		// NORMALMAP
		if (_imd_load_texture_command(filename, lineToProcess, buffer, value, cnt, "NORMALMAP", levelSettings))
		{
			if (!getNextPossibleLevelSettingLine())
			{
				return true;
			}
			if (pieVersion >= PIE_VER_4)
			{
				continue; // loop to allow multiple TEXTURE, TCMASK, NORMALMAP, and SPECULARMAP commands
			}
		}

		// SPECULARMAP
		if (_imd_load_texture_command(filename, lineToProcess, buffer, value, cnt, "SPECULARMAP", levelSettings))
		{
			if (!getNextPossibleLevelSettingLine())
			{
				return true;
			}
			if (pieVersion >= PIE_VER_4)
			{
				continue; // loop to allow multiple TEXTURE, TCMASK, NORMALMAP, and SPECULARMAP commands
			}
		}

		// not one of the expected commands - exit loop
		break;

	} while (pieVersion >= PIE_VER_4); // there may be multiple TEXTURE, TCMASK, NORMALMAP, and SPECULARMAP commands when pieVersion >= PIE_VER_4

	return true;
}

/*!
 * Load shape level polygons
 * \param ppFileData Pointer to the data (usually read from a file)
 * \param s Pointer to shape level
 * \return false on error (memory allocation failure/bad file format), true otherwise
 */
static bool _imd_load_polys(const WzString &filename, const char **ppFileData, const char *FileDataEnd, iIMDShape *s, int pieVersion, const uint32_t npoints, bool shadowPolys = false)
{
	const char *pFileData = *ppFileData;
	IMD_Line lineToProcess;

	s->numFrames = 0;
	s->animInterval = 0;

	for (unsigned i = 0; i < s->polys.size(); i++)
	{
		iIMDPoly *poly = &s->polys[i];
		unsigned int flags, npnts;
		int cnt;

		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			debug(LOG_ERROR, "File corrupt - could not read next polygon line (EOF)");
			return false;
		}

		const char* pRestOfLine = lineToProcess.lineContents.c_str();
		if (sscanf(pRestOfLine, "%x %u%n", &flags, &npnts, &cnt) != 2)
		{
			debug(LOG_ERROR, "(_load_polys) [poly %u] error loading flags and npoints", i);
		}
		pRestOfLine += cnt;

		if (shadowPolys)
		{
			ASSERT_OR_RETURN(false, flags == 0, "Invalid polygon flags (%d) for SHADOWPOLYGONS", flags);
		}

		poly->flags = flags;
		ASSERT_OR_RETURN(false, npnts == 3, "Invalid polygon size (%d)", npnts);
		if (sscanf(pRestOfLine, "%" PRIu32 "%" PRIu32 "%" PRIu32 "%n", &poly->pindex[0], &poly->pindex[1], &poly->pindex[2], &cnt) != 3)
		{
			debug(LOG_ERROR, "failed reading triangle, point %d", i);
			return false;
		}
		pRestOfLine += cnt;

		// sanity check
		for (size_t pIdx = 0; pIdx < 3; pIdx++)
		{
			ASSERT_OR_RETURN(false, (poly->pindex[pIdx] < npoints), "Point index (%" PRIu32 ") exceeds max index (%" PRIu32 ")", poly->pindex[pIdx], (npoints - 1));
		}

		// calc poly normal
		{
			Vector3f p0, p1, p2;

			//assumes points already set
			p0.x = s->points[poly->pindex[0]].x;
			p0.y = s->points[poly->pindex[0]].y;
			p0.z = s->points[poly->pindex[0]].z;

			p1.x = s->points[poly->pindex[1]].x;
			p1.y = s->points[poly->pindex[1]].y;
			p1.z = s->points[poly->pindex[1]].z;

			p2.x = s->points[poly->pindex[2]].x;
			p2.y = s->points[poly->pindex[2]].y;
			p2.z = s->points[poly->pindex[2]].z;

			poly->normal = pie_SurfaceNormal3fv(p0, p1, p2);
		}

		// texture coord routine
		if (poly->flags & iV_IMD_TEX)
		{
			int nFrames, pbRate;
			float tWidth, tHeight;

			if (poly->flags & iV_IMD_TEXANIM)
			{
				if (sscanf(pRestOfLine, "%d %d %f %f%n", &nFrames, &pbRate, &tWidth, &tHeight, &cnt) != 4)
				{
					debug(LOG_ERROR, "(_load_polys) [poly %u] error reading texanim data", i);
					return false;
				}
				pRestOfLine += cnt;

				ASSERT(tWidth > 0.0001f, "%s: texture width = %f", filename.toUtf8().c_str(), tWidth);
				ASSERT(tHeight > 0.f, "%s: texture height = %f (width=%f)", filename.toUtf8().c_str(), tHeight, tWidth);
				ASSERT(nFrames > 1, "%s: animation frames = %d", filename.toUtf8().c_str(), nFrames);
				ASSERT(pbRate > 0, "%s: animation interval = %d ms", filename.toUtf8().c_str(), pbRate);

				/* Must have same number of frames and same playback rate for all polygons */
				if (s->numFrames == 0)
				{
					s->numFrames = nFrames;
					s->animInterval = pbRate;
				}
				else
				{
					ASSERT(s->numFrames == nFrames,
					       "%s: varying number of frames within one PIE level: %d != %d",
					       filename.toUtf8().c_str(), nFrames, s->numFrames);
					ASSERT(s->animInterval == pbRate,
					       "%s: varying animation intervals within one PIE level: %d != %d",
					       filename.toUtf8().c_str(), pbRate, s->animInterval);
				}

				poly->texAnim.x = tWidth;
				poly->texAnim.y = tHeight;

				if (pieVersion < PIE_FLOAT_VER)
				{
					poly->texAnim.x /= OLD_TEXTURE_SIZE_FIX;
					poly->texAnim.y /= OLD_TEXTURE_SIZE_FIX;
				}
			}
			else
			{
				nFrames = 1;
				pbRate = 1;
				tWidth = 0.f;
				tHeight = 0.f;
				poly->texAnim.x = 0;
				poly->texAnim.y = 0;
			}

			for (unsigned j = 0; j < 3; j++)
			{
				float VertexU, VertexV;

				if (sscanf(pRestOfLine, "%f %f%n", &VertexU, &VertexV, &cnt) != 2)
				{
					debug(LOG_ERROR, "(_load_polys) [poly %u] error reading tex outline", i);
					return false;
				}
				pRestOfLine += cnt;

				if (pieVersion < PIE_FLOAT_VER)
				{
					VertexU /= OLD_TEXTURE_SIZE_FIX;
					VertexV /= OLD_TEXTURE_SIZE_FIX;
				}

				Vector2f *c = &poly->texCoord[j];
				c->x = VertexU;
				c->y = VertexV;
			}
		}
		else
		{
			ASSERT_OR_RETURN(false, !(poly->flags & iV_IMD_TEXANIM), "Polygons with texture animation must have textures!");
		}

		pFileData = lineToProcess.pNextLineBegin;
	}

	*ppFileData = pFileData;

	return true;
}


static bool ReadPoints(const char **ppFileData, const char *FileDataEnd, iIMDShape &s)
{
	const char *pFileData = *ppFileData;
	int cnt;
	IMD_Line lineToProcess;
	lineToProcess.pNextLineBegin = pFileData;

	for (Vector3f &points : s.points)
	{
		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			debug(LOG_ERROR, "File corrupt - could not read points (EOF)");
			return false;
		}
		if (sscanf(lineToProcess.lineContents.c_str(), "%f %f %f%n", &points.x, &points.y, &points.z, &cnt) != 3)
		{
			debug(LOG_ERROR, "File corrupt - could not read points");
			return false;
		}
		pFileData = lineToProcess.pNextLineBegin;
	}

	*ppFileData = pFileData;

	return true;
}

static void _imd_calc_bounds(iIMDShape &s, bool allLevels = false)
{
	int32_t xmax, ymax, zmax;
	double dx, dy, dz, rad_sq, rad, old_to_p_sq, old_to_p, old_to_new;
	double xspan, yspan, zspan, maxspan;
	Vector3f dia1, dia2, cen;
	Vector3f vxmin(0, 0, 0), vymin(0, 0, 0), vzmin(0, 0, 0),
	         vxmax(0, 0, 0), vymax(0, 0, 0), vzmax(0, 0, 0);

	s.max.x = s.max.y = s.max.z = -FP12_MULTIPLIER;
	s.min.x = s.min.y = s.min.z = FP12_MULTIPLIER;

	vxmax.x = vymax.y = vzmax.z = -FP12_MULTIPLIER;
	vxmin.x = vymin.y = vzmin.z = FP12_MULTIPLIER;

	// set up bounding data for minimum number of vertices
	const iIMDShape *pShapeLevel = &s;
	do {
		for (const Vector3f &p : pShapeLevel->points)
		{
			if (p.x > s.max.x)
			{
				s.max.x = static_cast<int>(p.x);
			}
			if (p.x < s.min.x)
			{
				s.min.x = static_cast<int>(p.x);
			}

			if (p.y > s.max.y)
			{
				s.max.y = static_cast<int>(p.y);
			}
			if (p.y < s.min.y)
			{
				s.min.y = static_cast<int>(p.y);
			}

			if (p.z > s.max.z)
			{
				s.max.z = static_cast<int>(p.z);
			}
			if (p.z < s.min.z)
			{
				s.min.z = static_cast<int>(p.z);
			}

			// for tight sphere calculations
			if (p.x < vxmin.x)
			{
				vxmin.x = p.x;
				vxmin.y = p.y;
				vxmin.z = p.z;
			}

			if (p.x > vxmax.x)
			{
				vxmax.x = p.x;
				vxmax.y = p.y;
				vxmax.z = p.z;
			}

			if (p.y < vymin.y)
			{
				vymin.x = p.x;
				vymin.y = p.y;
				vymin.z = p.z;
			}

			if (p.y > vymax.y)
			{
				vymax.x = p.x;
				vymax.y = p.y;
				vymax.z = p.z;
			}

			if (p.z < vzmin.z)
			{
				vzmin.x = p.x;
				vzmin.y = p.y;
				vzmin.z = p.z;
			}

			if (p.z > vzmax.z)
			{
				vzmax.x = p.x;
				vzmax.y = p.y;
				vzmax.z = p.z;
			}
		}

		pShapeLevel = pShapeLevel->next.get();
	} while (allLevels && pShapeLevel != nullptr);

	// no need to scale an IMD shape (only FSD)
	xmax = MAX(s.max.x, -s.min.x);
	ymax = MAX(s.max.y, -s.min.y);
	zmax = MAX(s.max.z, -s.min.z);

	s.radius = MAX(xmax, MAX(ymax, zmax));
	s.sradius = static_cast<int>(sqrtf(xmax * xmax + ymax * ymax + zmax * zmax));

// START: tight bounding sphere

	// set xspan = distance between 2 points xmin & xmax (squared)
	dx = vxmax.x - vxmin.x;
	dy = vxmax.y - vxmin.y;
	dz = vxmax.z - vxmin.z;
	xspan = dx * dx + dy * dy + dz * dz;

	// same for yspan
	dx = vymax.x - vymin.x;
	dy = vymax.y - vymin.y;
	dz = vymax.z - vymin.z;
	yspan = dx * dx + dy * dy + dz * dz;

	// and ofcourse zspan
	dx = vzmax.x - vzmin.x;
	dy = vzmax.y - vzmin.y;
	dz = vzmax.z - vzmin.z;
	zspan = dx * dx + dy * dy + dz * dz;

	// set points dia1 & dia2 to maximally separated pair
	// assume xspan biggest
	dia1 = vxmin;
	dia2 = vxmax;
	maxspan = xspan;

	if (yspan > maxspan)
	{
		maxspan = yspan;
		dia1 = vymin;
		dia2 = vymax;
	}

	if (zspan > maxspan)
	{
		dia1 = vzmin;
		dia2 = vzmax;
	}

	// dia1, dia2 diameter of initial sphere
	cen.x = (dia1.x + dia2.x) / 2.f;
	cen.y = (dia1.y + dia2.y) / 2.f;
	cen.z = (dia1.z + dia2.z) / 2.f;

	// calc initial radius
	dx = dia2.x - cen.x;
	dy = dia2.y - cen.y;
	dz = dia2.z - cen.z;

	rad_sq = dx * dx + dy * dy + dz * dz;
	rad = sqrt((double)rad_sq);

	// second pass (find tight sphere)
	for (const Vector3f &p : s.points)
	{
		dx = p.x - cen.x;
		dy = p.y - cen.y;
		dz = p.z - cen.z;
		old_to_p_sq = dx * dx + dy * dy + dz * dz;

		// do r**2 first
		if (old_to_p_sq > rad_sq)
		{
			// this point outside current sphere
			old_to_p = sqrt((double)old_to_p_sq);
			// radius of new sphere
			rad = (rad + old_to_p) / 2.;
			// rad**2 for next compare
			rad_sq = rad * rad;
			old_to_new = old_to_p - rad;
			// centre of new sphere
			cen.x = static_cast<float>((rad * cen.x + old_to_new * p.x) / old_to_p);
			cen.y = static_cast<float>((rad * cen.y + old_to_new * p.y) / old_to_p);
			cen.z = static_cast<float>((rad * cen.z + old_to_new * p.z) / old_to_p);
		}
	}

	s.ocen = cen;

// END: tight bounding sphere
}

static bool _imd_load_points(const char **ppFileData, const char *FileDataEnd, iIMDShape &s, uint32_t npoints)
{
	//load the points then pass through a second time to setup bounding datavalues
	s.points.resize(npoints);

	// Read in points and remove duplicates (!)
	if (ReadPoints(ppFileData, FileDataEnd, s) == false)
	{
		s.points.clear();
		return false;
	}

	_imd_calc_bounds(s);

	return true;
}


/*!
 * Load shape level connectors
 * \param ppFileData Pointer to the data (usually read from a file)
 * \param s Pointer to shape level
 * \return false on error (memory allocation failure/bad file format), true otherwise
 * \pre ppFileData loaded
 * \pre s allocated
 * \post s->connectors populated
 */
bool _imd_load_connectors(const char **ppFileData, const char *FileDataEnd, unsigned int numConnectors, iIMDShape &s)
{
	const char *pFileData = *ppFileData;
	int cnt;
	IMD_Line lineToProcess;
	lineToProcess.pNextLineBegin = pFileData;
	Vector3i newVector(0, 0, 0);

	s.connectors.reserve(numConnectors);
	for (unsigned int i = 0; i < numConnectors; ++i)
	{
		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			debug(LOG_ERROR, "(_load_connectors) file corrupt - could not read any more lines (EOF)");
			return false;
		}
		if (sscanf(lineToProcess.lineContents.c_str(), "%d %d %d%n",                         &newVector.x, &newVector.y, &newVector.z, &cnt) != 3 &&
		    sscanf(lineToProcess.lineContents.c_str(), "%d%*[.0-9] %d%*[.0-9] %d%*[.0-9]%n", &newVector.x, &newVector.y, &newVector.z, &cnt) != 3)
		{
			debug(LOG_ERROR, "(_load_connectors) file corrupt -M");
			return false;
		}
		pFileData = lineToProcess.pNextLineBegin;
		s.connectors.push_back(newVector);
	}

	*ppFileData = pFileData;

	return true;
}

static std::string _imd_get_level_tileset_texture_file(const char* type, const LevelTextureFilesMapping& levelTilesetSettings, const LevelTextureFilesMapping& globalTilesetSettings)
{
	// 1.) try the level tileset settings
	auto it_level_tileset_texture = levelTilesetSettings.find(type);
	if (it_level_tileset_texture != levelTilesetSettings.end() && !it_level_tileset_texture->second.empty())
	{
		return it_level_tileset_texture->second;
	}
	// 2.) fall back to the global tileset settings
	auto it_global_tileset_texture = globalTilesetSettings.find(type);
	if (it_global_tileset_texture != globalTilesetSettings.end() && !it_global_tileset_texture->second.empty())
	{
		return it_global_tileset_texture->second;
	}
	return {};
}

static void _imd_determine_tileset_texture_files(iIMDShape& s, const LevelSettings &globalLevelSettings, const LevelSettings &levelSettings)
{
	for (size_t tilesetIdx = 0; tilesetIdx < s.tilesetTextureFiles.size(); ++tilesetIdx)
	{
		const auto& globalTilesetSettings = globalLevelSettings.tilesetTextureFiles[tilesetIdx];
		const auto& levelTilesetSettings = levelSettings.tilesetTextureFiles[tilesetIdx];

		s.tilesetTextureFiles[tilesetIdx].texfile = _imd_get_level_tileset_texture_file("TEXTURE", levelTilesetSettings, globalTilesetSettings);
		s.tilesetTextureFiles[tilesetIdx].tcmaskfile = _imd_get_level_tileset_texture_file("TCMASK", levelTilesetSettings, globalTilesetSettings);
		s.tilesetTextureFiles[tilesetIdx].normalfile = _imd_get_level_tileset_texture_file("NORMALMAP", levelTilesetSettings, globalTilesetSettings);
		s.tilesetTextureFiles[tilesetIdx].specfile = _imd_get_level_tileset_texture_file("SPECULARMAP", levelTilesetSettings, globalTilesetSettings);
	}
}

static bool _imd_load_level_textures(const iIMDShape& s, size_t tilesetIdx, iIMDShapeTextures& output)
{
	const auto& defaultSettings = s.tilesetTextureFiles[0];
	const auto& tilesetSettings = s.tilesetTextureFiles[tilesetIdx];

	const WzString &filename = s.modelName;
	const TilesetTextureFiles* pLevelSettingsToUseForTextures = (!tilesetSettings.texfile.empty()) ? &tilesetSettings : &defaultSettings;
	if (!pLevelSettingsToUseForTextures->texfile.empty())
	{
		optional<size_t> texpage = iV_GetTexture(pLevelSettingsToUseForTextures->texfile.c_str(), gfx_api::texture_type::game_texture);
		optional<size_t> tcmaskpage;
		optional<size_t> normalpage;
		optional<size_t> specpage;

		ASSERT_OR_RETURN(false, texpage.has_value(), "%s could not load tex page %s", filename.toUtf8().c_str(), pLevelSettingsToUseForTextures->texfile.c_str());

		const TilesetTextureFiles* pLevelSettingsToUseForTCMask = (!tilesetSettings.tcmaskfile.empty()) ? &tilesetSettings : &defaultSettings;
		if (!pLevelSettingsToUseForTCMask->tcmaskfile.empty())
		{
			// load explicitly specified tcmask file
			debug(LOG_TEXTURE, "Loading tcmask %s for %s", pLevelSettingsToUseForTCMask->tcmaskfile.c_str(), filename.toUtf8().c_str());
			tcmaskpage = iV_GetTexture(pLevelSettingsToUseForTCMask->tcmaskfile.c_str(), gfx_api::texture_type::alpha_mask);
			ASSERT_OR_RETURN(false, tcmaskpage.has_value(), "%s could not load tcmask %s", filename.toUtf8().c_str(), pLevelSettingsToUseForTCMask->tcmaskfile.c_str());
		}
		else
		{
			// BACKWARDS-COMPATIBILITY (PIE 2/3 compatibility)
			// check if model should use team colour mask
			if (s.flags & iV_IMD_TCMASK)
			{
				std::string tcmask_name = pie_MakeTexPageTCMaskName(pLevelSettingsToUseForTextures->texfile.c_str());
				tcmask_name += ".png";
				tcmaskpage = iV_GetTexture(tcmask_name.c_str(), gfx_api::texture_type::alpha_mask);
				ASSERT_OR_RETURN(false, tcmaskpage.has_value(), "%s could not load tcmask %s", filename.toUtf8().c_str(), tcmask_name.c_str());
			}
		}

		const TilesetTextureFiles* pLevelSettingsToUseForNormals = (!tilesetSettings.normalfile.empty()) ? &tilesetSettings : &defaultSettings;
		if (!pLevelSettingsToUseForNormals->normalfile.empty())
		{
			debug(LOG_TEXTURE, "Loading normal map %s for %s", pLevelSettingsToUseForNormals->normalfile.c_str(), filename.toUtf8().c_str());
			normalpage = iV_GetTexture(pLevelSettingsToUseForNormals->normalfile.c_str(), gfx_api::texture_type::normal_map);
			ASSERT_OR_RETURN(false, normalpage.has_value(), "%s could not load tex page %s", filename.toUtf8().c_str(), pLevelSettingsToUseForNormals->normalfile.c_str());
		}

		const TilesetTextureFiles* pLevelSettingsToUseForSpecular = (!tilesetSettings.specfile.empty()) ? &tilesetSettings : &defaultSettings;
		if (!pLevelSettingsToUseForSpecular->specfile.empty())
		{
			debug(LOG_TEXTURE, "Loading specular map %s for %s", pLevelSettingsToUseForSpecular->specfile.c_str(), filename.toUtf8().c_str());
			specpage = iV_GetTexture(pLevelSettingsToUseForSpecular->specfile.c_str(), gfx_api::texture_type::specular_map);
			ASSERT_OR_RETURN(false, specpage.has_value(), "%s could not load tex page %s", filename.toUtf8().c_str(), pLevelSettingsToUseForSpecular->specfile.c_str());
		}

		// assign tex pages and flags for this level
		output.texpage = texpage.value();
		output.tcmaskpage = (tcmaskpage.has_value()) ? tcmaskpage.value() : iV_TEX_INVALID;
		output.normalpage = (normalpage.has_value()) ? normalpage.value() : iV_TEX_INVALID;
		output.specularpage = (specpage.has_value()) ? specpage.value() : iV_TEX_INVALID;
	}

	return true;
}

// performance hack
static std::vector<gfx_api::gfxFloat> vertices;
static std::vector<gfx_api::gfxFloat> normals;
static std::vector<gfx_api::gfxFloat> texcoords; // texcoords + texAnim
static std::vector<gfx_api::gfxFloat> tangents;
static std::vector<gfx_api::gfxFloat> bitangents;
static std::vector<uint16_t> indices; // size is npolys * 3 * numFrames
static uint16_t vertexCount = 0;

static bool ReadNormals(const char **ppFileData, const char *FileDataEnd, std::vector<Vector3f> &pie_level_normals, uint32_t num_normal_lines)
{
	const char *pFileData = *ppFileData;
	int dataCnt;
	IMD_Line lineToProcess;
	lineToProcess.pNextLineBegin = pFileData;
	Vector3f *pNormals = nullptr;
	const char *pRestOfLine = nullptr;

	for (size_t normalLine = 0; normalLine < num_normal_lines; ++normalLine)
	{
		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			debug(LOG_ERROR, "File corrupt - could not read normals (EOF)");
			return false;
		}
		pNormals = &pie_level_normals[normalLine * 3];
		pRestOfLine = lineToProcess.lineContents.c_str();
		for (size_t i = 0; i < 3; ++i)
		{
			if (sscanf(pRestOfLine, "%f %f %f%n", &(pNormals[i].x), &(pNormals[i].y), &(pNormals[i].z), &dataCnt) != 3)
			{
				debug(LOG_ERROR, "File corrupt - could not read normal(%zu) from normals line: %zu", i, normalLine);
				return false;
			}
			pRestOfLine += dataCnt;
		}
		pFileData = lineToProcess.pNextLineBegin;
	}

	*ppFileData = pFileData;

	return true;
}

static bool _imd_load_normals(const char **ppFileData, const char *FileDataEnd, std::vector<Vector3f> &pie_level_normals, uint32_t num_normal_lines)
{
   // We only support triangles!
   pie_level_normals.resize(static_cast<size_t>(num_normal_lines * 3));

   if (ReadNormals(ppFileData, FileDataEnd, pie_level_normals, num_normal_lines) == false)
   {
	   pie_level_normals.clear();
	   return false;
   }

   return true;
}

static inline uint16_t addVertex(iIMDShape &s, size_t i, const iIMDPoly *p, size_t polyIdx, std::vector<Vector3f> &pie_level_normals)
{
	const Vector3f* normal;

 	// Do not weld for for models with normals, those are presumed to be correct. Otherwise, it will break tangents
 	if (pie_level_normals.empty())
 	{
		normal = &p->normal;
		// See if we already have this defined, if so, return reference to it.
		for (uint16_t j = 0; j < vertexCount; j++)
		{
			if (texcoords[j * 4 + 0] == p->texCoord[i].x
 			    && texcoords[j * 4 + 1] == p->texCoord[i].y
				&& texcoords[j * 4 + 2] == p->texAnim.x
				&& texcoords[j * 4 + 3] == p->texAnim.y
 			    && vertices[j * 3 + 0] == s.points[p->pindex[i]].x
 			    && vertices[j * 3 + 1] == s.points[p->pindex[i]].y
 			    && vertices[j * 3 + 2] == s.points[p->pindex[i]].z
 			    && normals[j * 3 + 0] == normal->x
 			    && normals[j * 3 + 1] == normal->y
 			    && normals[j * 3 + 2] == normal->z)
 			{
 				return j;
 			}
		}
	}
	else
	{
		normal = &pie_level_normals[polyIdx * 3 + i];
	}

	// We don't have it, add it.
	normals.emplace_back(normal->x);
 	normals.emplace_back(normal->y);
 	normals.emplace_back(normal->z);
	texcoords.emplace_back(p->texCoord[i].x);
	texcoords.emplace_back(p->texCoord[i].y);
	texcoords.emplace_back(p->texAnim.x);
	texcoords.emplace_back(p->texAnim.y);
	vertices.emplace_back(s.points[p->pindex[i]].x);
	vertices.emplace_back(s.points[p->pindex[i]].y);
	vertices.emplace_back(s.points[p->pindex[i]].z);
	vertexCount++;

	return vertexCount - 1;
}

void calculateTangentsForTriangle(const uint16_t ia, const uint16_t ib, const uint16_t ic)
{
   // This will work as long as vecs are packed (which is default in glm)
   const Vector3f* verticesAsVec3 = reinterpret_cast<const Vector3f*>(vertices.data());
   const Vector4f* texcoordsAsVec4 = reinterpret_cast<const Vector4f*>(texcoords.data());
   Vector4f* tangentsAsVec4 = reinterpret_cast<Vector4f*>(tangents.data());
   Vector3f* bitangentsAsVec3 = reinterpret_cast<Vector3f*>(bitangents.data());

   // Shortcuts for vertices
   const Vector3f& va(verticesAsVec3[ia]);
   const Vector3f& vb(verticesAsVec3[ib]);
   const Vector3f& vc(verticesAsVec3[ic]);

   // Shortcuts for UVs
   const Vector2f uva(texcoordsAsVec4[ia].xy());
   const Vector2f uvb(texcoordsAsVec4[ib].xy());
   const Vector2f uvc(texcoordsAsVec4[ic].xy());

   // Edges of the triangle : postion delta
   const Vector3f deltaPos1 = vb - va;
   const Vector3f deltaPos2 = vc - va;

   // UV delta
   const Vector2f deltaUV1 = uvb - uva;
   const Vector2f deltaUV2 = uvc - uva;

   // check for nan
   float r = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
   if (r != 0.f)
	   r = 1.f / r;

   const Vector4f tangent(Vector3f(deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r, 0.f);
   const Vector3f bitangent(Vector3f(deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r);

   tangentsAsVec4[ia] += tangent;
   tangentsAsVec4[ib] += tangent;
   tangentsAsVec4[ic] += tangent;

   bitangentsAsVec3[ia] += bitangent;
   bitangentsAsVec3[ib] += bitangent;
   bitangentsAsVec3[ic] += bitangent;
}

void finishTangentsGeneration()
{
   // This will work as long as vecs are packed (which is default in glm)
   const Vector3f* normalsAsVec3 = reinterpret_cast<const Vector3f*>(normals.data());
   Vector4f* tangentsAsVec4 = reinterpret_cast<Vector4f*>(tangents.data());
   const Vector3f* bitangentsAsVec3 = reinterpret_cast<const Vector3f*>(bitangents.data());

   Vector3f t;

   for (auto i = 0; i < vertexCount; ++i)
   {
	   const Vector3f& n = normalsAsVec3[i];
	   const Vector3f& b = bitangentsAsVec3[i];
	   t = tangentsAsVec4[i].xyz();

	   // Gram-Schmidt orthogonalize
	   t = glm::normalize(t - n * glm::dot(n, t));

	   // Calculate handedness
	   if (glm::dot(glm::cross(n, t), b) < 0.f)
	   {
		   tangentsAsVec4[i] = Vector4f(-t, 1.f);
	   }
	   else
	   {
		   tangentsAsVec4[i] = Vector4f(-t, -1.f);
	   }
   }
}

/*!
 * Load shape levels recursively
 * \param ppFileData Pointer to the data (usually read from a file)
 * \param FileDataEnd ???
 * \param nlevels Number of levels to load
 * \return pointer to iFSDShape structure (or NULL on error)
 * \pre ppFileData loaded
 * \post s allocated
 */
static_assert(PATH_MAX >= 255, "PATH_MAX is insufficient!");
static std::unique_ptr<iIMDShape> _imd_load_level(const WzString &filename, const char **ppFileData, const char *FileDataEnd, int pieVersion, uint32_t level, const LevelSettings &globalLevelSettings, bool skipGPUData, bool skipDuplicateLoadChecks)
{
	const char *pFileData = *ppFileData;
	char buffer[PATH_MAX] = {'\0'}; uint32_t value = 0;
	int cnt = 0, scanResult = 0;

	// insert model
	std::string key = filename.toStdString();
	if (level > 0)
	{
		key += "_" + std::to_string(level);
	}
	if (!skipDuplicateLoadChecks)
	{
		ASSERT(models.count(key) == 0, "Duplicate model load for %s!", key.c_str());
	}
	auto pAllocatedShape = std::make_unique<iIMDShape>();
	iIMDShape &s = *pAllocatedShape;
	s.modelName = WzString::fromUtf8(key);
	s.modelLevel = level;
	s.pShadowPoints = &s.points;
	s.pShadowPolys = &s.polys;

	LevelSettings levelSettings;
	if (pieVersion >= PIE_VER_4)
	{
		// While level settings are supported at the global level, each level can also specify its own settings overrides (as of WZ 4.4+) which take precedence
		if (!_imd_load_level_settings(filename, &pFileData, FileDataEnd, pieVersion, false, levelSettings))
		{
			debug(LOG_ERROR, "%s: Failed to load level settings (level: %" PRIu32 ")", filename.toUtf8().c_str(), level);
			return nullptr;
		}
	}

	IMD_Line lineToProcess;
	lineToProcess.pNextLineBegin = pFileData;

	auto getNextPossibleIgnoredCommand = [&]() -> bool {
		pFileData = lineToProcess.pNextLineBegin;
		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			return false;
		}
		scanResult = sscanf(lineToProcess.lineContents.c_str(), "%255s %n", buffer, &cnt);
		if (scanResult != 1)
		{
			return false;
		}
		return true;
	};

	auto getNextPossibleCommandLine = [&]() -> bool {
		pFileData = lineToProcess.pNextLineBegin;
		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			return false;
		}
		scanResult = sscanf(lineToProcess.lineContents.c_str(), "%255s %" PRIu32 "%n", buffer, &value, &cnt);
		if (scanResult != 2)
		{
			return false;
		}
		return true;
	};

	getNextPossibleIgnoredCommand();
	ASSERT_OR_RETURN(nullptr, scanResult == 1, "Bad directive following LEVEL");

	// Optionally load and ignore deprecated MATERIALS directive
	if (strcmp(buffer, "MATERIALS") == 0)
	{
		debug(LOG_WARNING, "MATERIALS directive no longer supported!");
		if (!getNextPossibleIgnoredCommand())
		{
			debug(LOG_ERROR, "_imd_load_level(1a): file corrupt: %s", filename.toUtf8().c_str());
			return nullptr;
		}
	}
	if (strcmp(buffer, "SHADERS") == 0)
	{
		debug(LOG_WARNING, "SHADERS directive no longer supported!");
		if (!getNextPossibleIgnoredCommand())
		{
			debug(LOG_ERROR, "_imd_load_level(1b): file corrupt: %s", filename.toUtf8().c_str());
			return nullptr;
		}
	}

	lineToProcess.pNextLineBegin = pFileData; // re-process the directive
	if (!getNextPossibleCommandLine())
	{
		debug(LOG_ERROR, "_imd_load_level(2): file corrupt: %s", filename.toUtf8().c_str());
		return nullptr;
	}

	// load points
	ASSERT_OR_RETURN(nullptr, strcmp(buffer, "POINTS") == 0, "Expecting 'POINTS' directive, got: %s", buffer);
	uint32_t npoints = value;
	if (!_imd_load_points(&lineToProcess.pNextLineBegin, FileDataEnd, s, npoints))
	{
		debug(LOG_ERROR, "_imd_load_level(2a): file corrupt: %s", filename.toUtf8().c_str());
		return nullptr;
	}

	if (!getNextPossibleCommandLine())
	{
		debug(LOG_ERROR, "_imd_load_level(3): file corrupt: %s", filename.toUtf8().c_str());
		return nullptr;
	}

	uint32_t npolys = value;
	std::vector<Vector3f> pie_level_normals;

	// It could be optional normals directive
 	if (strcmp(buffer, "NORMALS") == 0)
 	{
 		_imd_load_normals(&lineToProcess.pNextLineBegin, FileDataEnd, pie_level_normals, npolys);

 		// Attemps to read polys again
		if (!getNextPossibleCommandLine())
		{
			debug(LOG_ERROR, "_imd_load_level(3a): file corrupt: %s", filename.toUtf8().c_str());
			return nullptr;
		}
		npolys = value;
		debug(LOG_3D, "imd[_load_level] = normals %d", static_cast<int>(pie_level_normals.size()));
 	}

	ASSERT_OR_RETURN(nullptr, strcmp(buffer, "POLYGONS") == 0, "Expecting 'POLYGONS' directive, got: %s", buffer);
	ASSERT_OR_RETURN(nullptr, npolys <= MAX_PIE_POLYGONS, "'POLYGONS' directive count (%" PRIu32") exceeds maximum supported (%" PRIu32")", npolys, MAX_PIE_POLYGONS);
	s.polys.resize(npolys);

	if (!_imd_load_polys(filename, &lineToProcess.pNextLineBegin, FileDataEnd, &s, pieVersion, npoints))
	{
		debug(LOG_ERROR, "_imd_load_level(3b): file corrupt - invalid polys: %s", filename.toUtf8().c_str());
		return nullptr;
	}

	// optional stuff : levels, object animations, connectors, shadow points + polys
	s.objanimframes = 0;
	while (getNextPossibleCommandLine())
	{
		if (strcmp(buffer, "LEVEL") == 0)	// check for next level
		{
			debug(LOG_3D, "imd[_load_level] = npoints %" PRIu32 ", npolys %" PRIu32 "", npoints, npolys);
			// Quit this loop - parent will call _imd_load_level again to process another LEVEL section
			break;
		}
		else if (strcmp(buffer, "CONNECTORS") == 0)
		{
			//load connector stuff
			if (!_imd_load_connectors(&lineToProcess.pNextLineBegin, FileDataEnd, value, s))
			{
				debug(LOG_ERROR, "_imd_load_level(5): file corrupt - invalid shadowpoints: %s", filename.toUtf8().c_str());
				return nullptr;
			}
		}
		else if (strcmp(buffer, "ANIMOBJECT") == 0)
		{
			s.objanimtime = value;
			if (s.objanimtime == 0)
			{
				debug(LOG_ERROR, "%s bad ANIMOBJ time: %" PRIu32, filename.toUtf8().c_str(), value);
				return nullptr;
			}
			const char* pRestOfLine = lineToProcess.lineContents.c_str() + cnt;
			if (sscanf(pRestOfLine, "%d %d%n", &s.objanimcycles, &s.objanimframes, &cnt) != 2)
			{
				debug(LOG_ERROR, "%s bad ANIMOBJ: %s", filename.toUtf8().c_str(), pFileData);
				return nullptr;
			}
			pFileData = lineToProcess.pNextLineBegin;
			s.objanimdata.resize(s.objanimframes);
			for (int i = 0; i < s.objanimframes; i++)
			{
				if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
				{
					// no more lines
					debug(LOG_ERROR, "%s: Missing ANIMOBJECT frame line (EOF)", filename.toUtf8().c_str());
					return nullptr;
				}

				int frame;
				Vector3i pos(0, 0, 0), rot(0, 0, 0);

				if (sscanf(lineToProcess.lineContents.c_str(), "%d %d %d %d %d %d %d %f %f %f%n",
				           &frame, &pos.x, &pos.y, &pos.z, &rot.x, &rot.y, &rot.z,
				           &s.objanimdata[i].scale.x, &s.objanimdata[i].scale.y, &s.objanimdata[i].scale.z, &cnt) != 10)
				{
					debug(LOG_ERROR, "%s: Invalid object animation level %" PRIu32 ", line %d, frame %d", filename.toUtf8().c_str(), level, i, frame);
				}
				ASSERT(frame == i, "%s: Invalid frame enumeration object animation (level %" PRIu32 ") %d: %d", filename.toUtf8().c_str(), level, i, frame);
				s.objanimdata[i].pos.x = static_cast<float>(pos.x) / static_cast<float>(INT_SCALE);
				s.objanimdata[i].pos.y = static_cast<float>(pos.z) / static_cast<float>(INT_SCALE);
				s.objanimdata[i].pos.z = static_cast<float>(pos.y) / static_cast<float>(INT_SCALE);
				s.objanimdata[i].rot.pitch = static_cast<uint16_t>(static_cast<int32_t>(-(rot.x * DEG_1 / INT_SCALE)));
				s.objanimdata[i].rot.direction = static_cast<uint16_t>(static_cast<int32_t>(-(rot.z * DEG_1 / INT_SCALE)));
				s.objanimdata[i].rot.roll = static_cast<uint16_t>(static_cast<int32_t>(-(rot.y * DEG_1 / INT_SCALE)));
				pFileData = lineToProcess.pNextLineBegin;
			}
		}
		else if (strcmp(buffer, "SHADOWPOINTS") == 0)
		{
			ASSERT_OR_RETURN(nullptr, value > 0, "Invalid 'SHADOW_POINTS' count, got: %" PRIu32, value);
			uint32_t nShadowPoints = value;

			iIMDShape tmpShadowShape;
			if (!_imd_load_points(&lineToProcess.pNextLineBegin, FileDataEnd, tmpShadowShape, nShadowPoints))
			{
				debug(LOG_ERROR, "_imd_load_level(7): file corrupt - invalid shadowpoints: %s", filename.toUtf8().c_str());
				return nullptr;
			}

			s.altShadowPoints = std::move(tmpShadowShape.points);
		}
		else if (strcmp(buffer, "SHADOWPOLYGONS") == 0)
		{
			ASSERT_OR_RETURN(nullptr, s.altShadowPoints.size() > 0, "'SHADOW_POLYGONS' must follow a non-empty SHADOW_POINTS section");
			ASSERT_OR_RETURN(nullptr, value > 0, "Invalid 'SHADOW_POLYGONS' count, got: %" PRIu32, value);
			uint32_t nShadowPolys = static_cast<uint32_t>(value);

			iIMDShape tmpShadowShape;
			tmpShadowShape.points = std::move(s.altShadowPoints);
			tmpShadowShape.polys.resize(nShadowPolys);
			if (!_imd_load_polys(filename, &lineToProcess.pNextLineBegin, FileDataEnd, &tmpShadowShape, PIE_FLOAT_VER, static_cast<uint32_t>(tmpShadowShape.points.size()), true))
			{
				debug(LOG_ERROR, "_imd_load_level(8): file corrupt - invalid shadowpolygons: %s", filename.toUtf8().c_str());
				return nullptr;
			}

			s.altShadowPoints = std::move(tmpShadowShape.points);
			s.altShadowPolys = std::move(tmpShadowShape.polys);
		}
		else
		{
			debug(LOG_ERROR, "(_load_level) unexpected directive %s %" PRIu32, buffer, value);
			break;
		}
	}

	// Sanity checks
 	if (!pie_level_normals.empty())
 	{
 		if (s.polys.size() * 3 != pie_level_normals.size())
 		{
 			debug(LOG_ERROR, "imd[_load_level] = got %" PRIu32 " npolys, but there are only %zu normals! Discarding normals...",
 			      npolys, pie_level_normals.size());
 			// This will force usage of calculated normals in addVertex
 			pie_level_normals.clear();
 		}
 	}
	if (!s.altShadowPoints.empty())
	{
		if (!s.altShadowPolys.empty())
		{
			s.pShadowPoints = &s.altShadowPoints;
			s.pShadowPolys = &s.altShadowPolys;
		}
		else
		{
			debug(LOG_ERROR, "imd[_load_level] = %zu SHADOWPOINTS specified, but there are no SHADOWPOLYGONS! Discarding shadow points...",
				  s.altShadowPoints.size());
			s.altShadowPoints.clear();
			s.altShadowPolys.clear();
		}
	}
	else
	{
		s.altShadowPoints.clear();
		s.altShadowPolys.clear();
	}

	// FINALLY, massage the data into what can stream directly to GPU buffers
	if (!skipGPUData)
	{
		vertexCount = 0;
		// Go through all polygons for each frame
		for (size_t npol = 0; npol < s.polys.size(); ++npol)
		{
			const iIMDPoly& p = s.polys[npol];
			// Do we already have the vertex data for this polygon?
			indices.emplace_back(addVertex(s, 0, &p, npol, pie_level_normals));
			indices.emplace_back(addVertex(s, 1, &p, npol, pie_level_normals));
			indices.emplace_back(addVertex(s, 2, &p, npol, pie_level_normals));
		}

		s.vertexCount = vertexCount;

		// Tangents are optional, only if normals were loaded and passed sanity check above
		if (!pie_level_normals.empty())
		{
			tangents.resize(vertexCount * 4);
			bitangents.resize(vertexCount * 3);

			for (size_t i = 0; i < indices.size(); i += 3)
				calculateTangentsForTriangle(indices[i], indices[i+1], indices[i+2]);
			finishTangentsGeneration();

			if (!tangents.empty())
			{
				if (!s.buffers[VBO_TANGENT])
					s.buffers[VBO_TANGENT] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::static_draw, "tangent buffer");
				s.buffers[VBO_TANGENT]->upload(tangents.size() * sizeof(gfx_api::gfxFloat), tangents.data());
			}
		}

		if (!s.buffers[VBO_VERTEX])
			s.buffers[VBO_VERTEX] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::static_draw, "vertex buffer");
		if (vertices.empty())
		{
			debug(LOG_ERROR, "_imd_load_level: file corrupt? - no vertices?: %s (key: %s)", filename.toUtf8().c_str(), key.c_str());
		}
		s.buffers[VBO_VERTEX]->upload(vertices.size() * sizeof(gfx_api::gfxFloat), vertices.data());

		if (!s.buffers[VBO_NORMAL])
			s.buffers[VBO_NORMAL] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::static_draw, "normals buffer");
		if (normals.empty())
		{
			debug(LOG_ERROR, "_imd_load_level: file corrupt? - no normals?: %s (key: %s)", filename.toUtf8().c_str(), key.c_str());
		}
		s.buffers[VBO_NORMAL]->upload(normals.size() * sizeof(gfx_api::gfxFloat), normals.data());

		if (!s.buffers[VBO_INDEX])
			s.buffers[VBO_INDEX] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "index buffer");
		if (indices.empty())
		{
			debug(LOG_ERROR, "_imd_load_level: file corrupt? - no indices?: %s (key: %s)", filename.toUtf8().c_str(), key.c_str());
		}
		s.buffers[VBO_INDEX]->upload(indices.size() * sizeof(uint16_t), indices.data());

		if (!s.buffers[VBO_TEXCOORD])
			s.buffers[VBO_TEXCOORD] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::static_draw, "tex coords buffer");
		if (texcoords.empty())
		{
			debug(LOG_ERROR, "_imd_load_level: file corrupt? - no texcoords?: %s (key: %s)", filename.toUtf8().c_str(), key.c_str());
		}
		s.buffers[VBO_TEXCOORD]->upload(texcoords.size() * sizeof(gfx_api::gfxFloat), texcoords.data());
	}

	indices.resize(0);
	vertices.resize(0);
	texcoords.resize(0);
	normals.resize(0);
	tangents.resize(0);
	bitangents.resize(0);

	*ppFileData = pFileData;

	// decide which flags to use (local level override or global)
	unsigned int flags = (levelSettings.imd_flags.has_value() ? levelSettings.imd_flags.value() : globalLevelSettings.imd_flags.value_or(0));
	s.flags = flags;

	// default for interpolation is on
	s.interpolate = (levelSettings.interpolate.has_value() ? levelSettings.interpolate.value() : globalLevelSettings.interpolate.value_or(true));

	_imd_determine_tileset_texture_files(s, globalLevelSettings, levelSettings);

	return pAllocatedShape;
}

/*!
 * Load ppFileData into a shape
 * \param ppFileData Data from the IMD file
 * \param FileDataEnd Endpointer
 * \return The shape, constructed from the data read
 */
// ppFileData is incremented to the end of the file on exit!
static std::unique_ptr<iIMDShape> iV_ProcessIMD(const WzString &filename, const char **ppFileData, const char *FileDataEnd, bool skipGPUData, bool skipDuplicateLoadChecks)
{
	const char *pFileData = *ppFileData;
	char buffer[PATH_MAX] = {};
	int cnt = 0;
	unsigned value = 0;
	unsigned nlevels = 0;
	int32_t imd_version;
	iIMDBaseShape *objanimpie[ANIM_EVENT_COUNT];

	IMD_Line lineToProcess;
	if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess) || sscanf(lineToProcess.lineContents.c_str(), "%255s %d", buffer, &imd_version) != 2)
	{
		debug(LOG_ERROR, "%s: bad PIE version: (%s)", filename.toUtf8().c_str(), buffer);
		++modelLoadingErrors;
		assert(false);
		return nullptr;
	}
	pFileData = lineToProcess.pNextLineBegin;

	if (strcmp(PIE_NAME, buffer) != 0)
	{
		debug(LOG_ERROR, "%s: Not an IMD file (%s %d)", filename.toUtf8().c_str(), buffer, imd_version);
		++modelLoadingErrors;
		return nullptr;
	}

	//Now supporting version PIE_VER and PIE_FLOAT_VER files
	if (imd_version < PIE_MIN_VER || imd_version > PIE_MAX_VER)
	{
		debug(LOG_ERROR, "%s: Version %d not supported", filename.toUtf8().c_str(), imd_version);
		++modelLoadingErrors;
		return nullptr;
	}

	LevelSettings globalLevelSettings;
	if (!_imd_load_level_settings(filename, &pFileData, FileDataEnd, imd_version, true, globalLevelSettings))
	{
		debug(LOG_ERROR, "%s: Failed to load level settings", filename.toUtf8().c_str());
		++modelLoadingErrors;
		return nullptr;
	}
	// TYPE is required in the global scope
	ASSERT_OR_RETURN(nullptr, globalLevelSettings.imd_flags.has_value(), "%s: Missing TYPE line", filename.toUtf8().c_str());

	auto getNextPossibleCommandLine = [&]() -> bool {
		pFileData = lineToProcess.pNextLineBegin;
		if (!_imd_get_next_line(pFileData, FileDataEnd, lineToProcess))
		{
			// no more lines
			return false;
		}
		auto scanResult = sscanf(lineToProcess.lineContents.c_str(), "%255s %u%n", buffer, &value, &cnt);
		if (scanResult != 2)
		{
			return false;
		}
		return true;
	};

	lineToProcess.pNextLineBegin = pFileData;

	/* Next: either EVENT or LEVELS */
	if (!getNextPossibleCommandLine())
	{
		debug(LOG_ERROR, "%s: Expecting EVENT or LEVELS: %s", filename.toUtf8().c_str(), buffer);
		++modelLoadingErrors;
		return nullptr;
	}

	for (int i = 0; i < ANIM_EVENT_COUNT; i++)
	{
		objanimpie[i] = nullptr;
	}
	while (strncmp(buffer, "EVENT", 5) == 0)
	{
		char animpie[PATH_MAX];

		ASSERT(value < ANIM_EVENT_COUNT, "Invalid event type %u", value);
		const char* pRestOfLine = lineToProcess.lineContents.c_str() + cnt;
		if (sscanf(pRestOfLine, "%255s%n", animpie, &cnt) != 1)
		{
			debug(LOG_ERROR, "%s animation model corrupt: %s", filename.toUtf8().c_str(), buffer);
			++modelLoadingErrors;
			return nullptr;
		}

		objanimpie[value] = modelGet(animpie);

		/* Try -yet again- to read in LEVELS directive */
		if (!getNextPossibleCommandLine())
		{
			debug(LOG_ERROR, "%s: Bad levels info: %s", filename.toUtf8().c_str(), buffer);
			++modelLoadingErrors;
			return nullptr;
		}
	}

	if (strncmp(buffer, "LEVELS", 6) != 0)
	{
		debug(LOG_ERROR, "%s: Expecting 'LEVELS' directive (%s)", filename.toUtf8().c_str(), buffer);
		++modelLoadingErrors;
		return nullptr;
	}
	nlevels = value;

	std::unique_ptr<iIMDShape> firstLevel = nullptr;
	iIMDShape *lastLevel = nullptr;
	for (uint32_t level = 0; level < nlevels; ++level)
	{
		/* Read LEVEL directive */
		if (!getNextPossibleCommandLine())
		{
			debug(LOG_ERROR, "(_load_level) file corrupt -J");
			++modelLoadingErrors;
			return nullptr;
		}
		if (strncmp(buffer, "LEVEL", 5) != 0)
		{
			debug(LOG_ERROR, "%s: Expecting 'LEVEL' directive (%s)", filename.toUtf8().c_str(), buffer);
			++modelLoadingErrors;
			return nullptr;
		}
		if (value != (level + 1))
		{
			debug(LOG_ERROR, "%s: LEVEL %" PRIu32 " is invalid - expecting LEVEL %" PRIu32 " (LEVELS must be sequential, starting at 1)", filename.toUtf8().c_str(), value, level);
			++modelLoadingErrors;
			return nullptr;
		}

		std::unique_ptr<iIMDShape> shape = _imd_load_level(filename, &lineToProcess.pNextLineBegin, FileDataEnd, imd_version, level, globalLevelSettings, skipGPUData, skipDuplicateLoadChecks);
		if (shape == nullptr)
		{
			debug(LOG_ERROR, "%s: Unsuccessful loading level %" PRIu32, filename.toUtf8().c_str(), (level + 1));
			++modelLoadingErrors;
			return nullptr;
		}

		if (lastLevel)
		{
			lastLevel->next = std::move(shape);
			lastLevel = lastLevel->next.get();
		}
		else
		{
			ASSERT(level == 0, "Invalid level");
			firstLevel = std::move(shape);
			lastLevel = firstLevel.get();
		}

		pFileData = lineToProcess.pNextLineBegin;
	}

	ASSERT_OR_RETURN(nullptr, firstLevel != nullptr, "%s: Has no levels?", filename.toUtf8().c_str());

	// copy over model-wide animation information, stored only in the first level
	for (int i = 0; i < ANIM_EVENT_COUNT; i++)
	{
		firstLevel->objanimpie[i] = objanimpie[i];
	}

	// TODO: once all levels have been loaded, re-calculate the bounds of the first level using all the levels' points?

	*ppFileData = pFileData;
	return firstLevel;
}
