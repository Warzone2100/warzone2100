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

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"

#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/png_util.h"

#include "screen.h"

#include <algorithm>
#include <unordered_map>

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-align"
#endif
#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#include "3rdparty/stb_image_resize.h"

#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

//*************************************************************************

struct iTexPage
{
	std::string name;
	gfx_api::texture* id = nullptr;
	gfx_api::texture_type textureType = gfx_api::texture_type::user_interface;

	iTexPage() = default;

	iTexPage(iTexPage&& input)
	{
		std::swap(name, input.name);
		id = input.id;
		input.id = nullptr;
	}

	~iTexPage()
	{
		if (id)
			delete id;
	}

	iTexPage (const iTexPage &) = delete;
	iTexPage & operator = (const iTexPage &) = delete;
};

std::vector<iTexPage> _TEX_PAGE;
std::unordered_map<std::string, size_t> _NAME_TO_TEX_PAGE_MAP;

//*************************************************************************

gfx_api::texture& pie_Texture(size_t page)
{
	return *(_TEX_PAGE[page].id);
}

size_t pie_NumberOfPages()
{
	return _TEX_PAGE.size();
}

// Add a new texture page to the list
size_t pie_ReserveTexture(const char *name, const size_t& width, const size_t& height)
{
	iTexPage tex;
	tex.name = name;
	_TEX_PAGE.push_back(std::move(tex));
	size_t page = _TEX_PAGE.size() - 1;
	_NAME_TO_TEX_PAGE_MAP[name] = page;
	return page;
}

void pie_AssignTexture(size_t page, gfx_api::texture* texture)
{
	if (_TEX_PAGE[page].id)
		delete _TEX_PAGE[page].id;
	_TEX_PAGE[page].id = texture;
}

static size_t pie_AddTexPage_Impl(gfx_api::texture *pTexture, const char *filename, gfx_api::texture_type textureType, size_t page)
{
	ASSERT(pTexture && filename, "Bad input parameter");

	_NAME_TO_TEX_PAGE_MAP[filename] = page;
	debug(LOG_TEXTURE, "%s page=%zu", filename, page);

	if (_TEX_PAGE[page].id)
		delete _TEX_PAGE[page].id;
	_TEX_PAGE[page].id = pTexture;

	/* Send back the texpage number so we can store it in the IMD */
	return page;
}

size_t pie_AddTexPage(gfx_api::texture *pTexture, const char *filename, gfx_api::texture_type textureType)
{
	ASSERT_OR_RETURN(0, pTexture && filename, "Bad input parameter");
	ASSERT(_NAME_TO_TEX_PAGE_MAP.count(filename) == 0, "tex page %s already exists", filename);
	iTexPage tex;
	size_t page = _TEX_PAGE.size();
	tex.name = filename;
	_TEX_PAGE.push_back(std::move(tex));

	return pie_AddTexPage_Impl(pTexture, filename, textureType, page);
}

size_t pie_AddTexPage(gfx_api::texture *pTexture, const char *filename, gfx_api::texture_type textureType, size_t page)
{
	ASSERT_OR_RETURN(0, pTexture && filename, "Bad input parameter");

	// replace
	_NAME_TO_TEX_PAGE_MAP.erase(_TEX_PAGE[page].name);
	_TEX_PAGE[page].name = filename;

	return pie_AddTexPage_Impl(pTexture, filename, textureType, page);
}

/*!
 * Turns filename into a pagename if possible
 * \param[in,out] filename Filename to pagify
 */
std::string pie_MakeTexPageName(const std::string& filename)
{
	size_t c = filename.find(iV_TEXNAME_TCSUFFIX);
	if (c != std::string::npos)
	{
		return filename.substr(0, c + 7);
	}
	c = filename.find('-', 5);
	if (c != std::string::npos)
	{
		return filename.substr(0, c);
	}
	return filename;
}

/*!
 * Turns page filename into a pagename + tc mask if possible
 * \param[in,out] filename Filename to pagify
 */
std::string pie_MakeTexPageTCMaskName(const std::string& filename)
{
	std::string result = filename;
	if (filename.rfind("page-", 0) == 0)
	{
		// filename starts with "page-"
		size_t i;
		for (i = 5; i < filename.size() - 1 && isdigit(filename[i]); i++) {}
		result = filename.substr(0, i);
		result += iV_TEXNAME_TCSUFFIX;
	}
	return result;
}

// TODO: Remove this wrapper
bool scaleImageMaxSize(iV_Image *s, int maxWidth, int maxHeight)
{
	return s->scale_image_max_size(maxWidth, maxHeight);
}

/** Retrieve the texture number for a given texture resource.
 *
 *  @note We keep textures in a separate data structure _TEX_PAGE apart from the
 *        normal resource system.
 *
 *  @param filename The filename of the texture page to search for.
 *  @param compression If we need to load it, should we use texture compression?
 *  @param maxWidth If we need to load it, should we limit the texture width? (Resizes and preserves the texture image's aspect ratio)
 *  @param maxHeight If we need to load it, should we limit the texture height? (Resizes and preserves the texture image's aspect ratio)
 *
 *  @return a non-negative index number for the texture, negative if no texture
 *          with the given filename could be found
 */
optional<size_t> iV_GetTexture(const char *filename, gfx_api::texture_type textureType, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	ASSERT(filename != nullptr, "filename must not be null");

	/* Have we already loaded this one then? */
	std::string path = pie_MakeTexPageName(filename);
	const auto it = _NAME_TO_TEX_PAGE_MAP.find(path);
	if (it != _NAME_TO_TEX_PAGE_MAP.end())
	{
		return it->second;
	}

	// Try to load it
	std::string loadPath = "texpages/";
	loadPath += filename;
	gfx_api::texture *pTexture = gfx_api::context::get().loadTextureFromFile(loadPath.c_str(), textureType, maxWidth, maxHeight);
	if (!pTexture)
	{
		debug(LOG_ERROR, "Failed to load %s", loadPath.c_str());
		return nullopt;
	}

	size_t page = pie_AddTexPage(pTexture, path.c_str(), textureType);
	resDoResLoadCallback(); // ensure loading screen doesn't freeze when loading large images
	return optional<size_t>(page);
}

bool replaceTexture(const WzString &oldfile, const WzString &newfile)
{
	// Load new one to replace it
	std::string tmpname = pie_MakeTexPageName(oldfile.toUtf8());
	// Have we already loaded this one?
	const auto it = _NAME_TO_TEX_PAGE_MAP.find(tmpname);
	if (it != _NAME_TO_TEX_PAGE_MAP.end())
	{
		gfx_api::context::get().debugStringMarker("Replacing texture");
		size_t page = it->second;
		gfx_api::texture_type existingTextureType = _TEX_PAGE[page].textureType;
		debug(LOG_TEXTURE, "Replacing texture %s with %s from index %zu (tex id %u)", it->first.c_str(), newfile.toUtf8().c_str(), page, _TEX_PAGE[page].id->id());
		gfx_api::texture *pTexture = gfx_api::context::get().loadTextureFromFile(WzString("texpages/" + newfile).toUtf8().c_str(), existingTextureType);
		tmpname = pie_MakeTexPageName(newfile.toUtf8());
		pie_AddTexPage(pTexture, tmpname.c_str(), existingTextureType, page);
		return true;
	}
	debug(LOG_ERROR, "Nothing to replace!");
	return false;
}

void pie_TexShutDown()
{
	// TODO, lazy deletions for faster loading of next level
	debug(LOG_TEXTURE, "Cleaning out %u textures", static_cast<unsigned>(_TEX_PAGE.size()));
	_TEX_PAGE.clear();
	_NAME_TO_TEX_PAGE_MAP.clear();
}

void pie_TexInit()
{
	debug(LOG_TEXTURE, "pie_TexInit successful");
}

void iV_unloadImage(iV_Image *image)
{
	if (image)
	{
		image->clear();
	}
	else
	{
		debug(LOG_ERROR, "Tried to free invalid image!");
	}
}

gfx_api::pixel_format iV_getPixelFormat(const iV_Image *image)
{
	switch (image->channels())
	{
	case 1:
		return gfx_api::pixel_format::FORMAT_R8_UNORM;
	case 2:
		return gfx_api::pixel_format::FORMAT_RG8_UNORM;
	case 3:
		return gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8;
	case 4:
		return gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8;
	default:
		debug(LOG_FATAL, "iV_getPixelFormat: Unsupported image channels: %u", image->channels());
		return gfx_api::pixel_format::invalid;
	}
}
