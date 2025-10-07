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

/**
 * @file texture.c
 * This is where we do texture atlas generation.
 */

#include "lib/framework/frame.h"

#include <string.h>
#include <physfs.h>

#include "lib/framework/file.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/physfs_ext.h"

#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/screen.h"

#include "display3ddef.h"
#include "texture.h"
#include "radar.h"
#include "map.h"

#define MIPMAP_MAX			512

/* Texture page and coordinates for each tile */
TILE_TEX_INFO tileTexInfo[MAX_TILES];

static size_t firstPage; // the last used page before we start adding terrain textures
optional<size_t> terrainPage; // texture ID of the terrain page
gfx_api::texture_array *decalTexArr = nullptr;
gfx_api::texture_array *decalNormalArr = nullptr;
gfx_api::texture_array *decalSpecularArr = nullptr;
gfx_api::texture_array *decalHeightArr = nullptr;
static int mipmap_max, mipmap_levels;
static int maxTextureSize = 2048; ///< the maximum size texture we will create

void setTextureSize(int texSize)
{
	if (texSize < 16 || texSize % 16 != 0)
	{
		debug(LOG_ERROR, "Attempted to set bad texture size %d! Ignored.", texSize);
		return;
	}
	maxTextureSize = texSize;
	debug(LOG_TEXTURE, "texture size set to %d", texSize);
}

int getTextureSize()
{
	return maxTextureSize;
}

int getCurrentTileTextureSize()
{
	return mipmap_max;
}

int getMaxTileTextureSize(std::string dir)
{
	int res = MIPMAP_MAX;
	while (res > 0 && !PHYSFS_exists(WzString::fromUtf8(dir + "-" + std::to_string(res) + "/tile-00.ktx2")) && !PHYSFS_exists(WzString::fromUtf8(dir + "-" + std::to_string(res) + "/tile-00.png")))
		res /= 2;
	return res;
}

// Generate a new texture page both in the texture page table, and on the graphics card
static size_t newPage(const char *name, int pageNum, int width, int height, int count)
{
	size_t texPage = firstPage + ((count + 1) / TILES_IN_PAGE);

	if (texPage == pie_NumberOfPages())
	{
		// Reserve a new texture page
		std::string textureDebugName = std::string("pageTexture::") + ((name) ? name : "") + std::to_string(pageNum);
		pie_ReserveTexture(name, width, height);
	}
	terrainPage = texPage;
	return texPage;
}

bool texLoad(const char *fileName)
{
	char fullPath[PATH_MAX], partialPath[PATH_MAX], *buffer;
	unsigned int i, j, k, size;
	int texPage = 0;
	iV_Image textureAtlas;

	firstPage = pie_NumberOfPages();

	// store the filename so we can later determine which tileset we are using
	if (tilesetDir)
	{
		free(tilesetDir);
	}
	tilesetDir = strdup(fileName);

	// reset defaults
	const int maxTileTexSize = getMaxTileTextureSize(fileName);
	mipmap_max = maxTileTexSize;
	mipmap_levels = (int)log2(mipmap_max / 8); // 4 for 128

	int32_t max_texture_size = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TEXTURE_SIZE);

	while (max_texture_size < mipmap_max * TILES_IN_PAGE_COLUMN)
	{
		mipmap_max /= 2;
		mipmap_levels--;
		debug(LOG_ERROR, "Max supported texture size %dx%d is too low - reducing texture detail to %dx%d.",
		      (int)max_texture_size, (int)max_texture_size, mipmap_max, mipmap_max);
		ASSERT(mipmap_levels > 0, "Supported texture size %d is too low to load any mipmap levels!",
		       (int)max_texture_size);
		if (mipmap_levels == 0)
		{
			debug(LOG_FATAL, "Supported texture size %d is too low to load any mipmap levels!: %s", (int)max_texture_size, fileName);
			exit(1);
		}
	}
	while (maxTextureSize < mipmap_max)
	{
		mipmap_max /= 2;
		mipmap_levels--;
		debug(LOG_TEXTURE, "Downgrading texture quality to %d due to user setting %d", mipmap_max, maxTextureSize);
	}

	/* Get and set radar colours */

	ssprintf(fullPath, "%s.radar", fileName);
	if (!loadFile(fullPath, &buffer, &size))
	{
		debug(LOG_FATAL, "texLoad: Could not find radar colours at %s", fullPath);
		abort(); // cannot recover; we could possibly generate a random palette?
	}
	i = 0; // tile
	j = 0; // place in buffer
	do
	{
		unsigned int r, g, b;
		int cnt = 0;

		k = sscanf(buffer + j, "%2x%2x%2x%n", &r, &g, &b, &cnt);
		j += cnt;
		if (k >= 3)
		{
			radarColour(i, r, g, b);
		}
		i++; // next tile
	}
	while (k >= 3 && j + 6 < size);
	free(buffer);

	auto uploadTextureAtlasPage = [fileName](iV_Image &&bitmap, size_t texPage) -> bool {
		std::string textureDebugName = std::string("pageTexture::") + ((fileName) ? fileName : "");
		auto pTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(bitmap), gfx_api::texture_type::game_texture, textureDebugName.c_str(), -1, -1);
		ASSERT_OR_RETURN(false, pTexture != nullptr, "Failed to load texture");
		pie_AssignTexture(texPage, pTexture);
		return true;
	};

	/* Now load the actual tiles */
	int pageNum = 0;
	{
		int xOffset = 0, yOffset = 0; // offsets into the texture atlas
		int xSize = 1;
		int ySize = 1;
		const int xLimit = TILES_IN_PAGE_COLUMN * mipmap_max;
		const int yLimit = TILES_IN_PAGE_ROW * mipmap_max;

		// pad width and height into ^2 values
		while (xLimit > (xSize *= 2)) {}
		while (yLimit > (ySize *= 2)) {}

		ssprintf(partialPath, "%s-%d", fileName, maxTileTexSize);

		bool has_nm = false, has_sm = false, has_hm = false;
		bool has_auxillary_texture_info = false;
		int maxTileNo = 0;

		auto currentTerrainShaderType = getTerrainShaderType();

		switch (currentTerrainShaderType)
		{
		case TerrainShaderType::FALLBACK:
			// Generate the empty texture buffer in VRAM
			texPage = newPage(fileName, pageNum, xSize, ySize, 0);
			textureAtlas.allocate(xSize, ySize, 4, true);
			break;
		case TerrainShaderType::SINGLE_PASS:
			WZ_PHYSFS_enumerateFiles(partialPath, [&](const char *fileName) -> bool {
				size_t len = strlen(fileName);
				auto hasSuffix = [&](const char *suf) -> bool {
					size_t l = strlen(suf);
					return strncmp(fileName + len - l, suf, l) == 0;
				};
				has_nm |= hasSuffix("_nm.png") || hasSuffix("_nm.ktx2");
				has_sm |= hasSuffix("_sm.png") || hasSuffix("_sm.ktx2");
				has_hm |= hasSuffix("_hm.png") || hasSuffix("_hm.ktx2");
				int tile = 0;
				if (sscanf(fileName, "tile-%02d.png", &tile)) {
					maxTileNo = std::max(maxTileNo, tile);
				}
				return true;
			});
			has_auxillary_texture_info = has_nm | has_sm | has_hm;
			debug(LOG_TEXTURE, "texLoad: Found %d tiles", maxTileNo);
			delete decalTexArr; decalTexArr = nullptr;
			delete decalNormalArr; decalNormalArr = nullptr;
			delete decalSpecularArr; decalSpecularArr = nullptr;
			delete decalHeightArr; decalHeightArr = nullptr;
			break;
		}

		WzString fullPath_nm;
		WzString fullPath_sm;
		WzString fullPath_hm;

		switch (currentTerrainShaderType)
		{
		case TerrainShaderType::FALLBACK:
			// Load until we cannot find anymore of them
			for (k = 0; k < MAX_TILES; k++)
			{
				std::unique_ptr<iV_Image> tile;

				snprintf(fullPath, sizeof(fullPath), "%s/tile-%02d.png", partialPath, k);
				if (PHYSFS_exists(fullPath)) // avoid dire warning
				{
					tile = gfx_api::loadUncompressedImageFromFile(fullPath, (currentTerrainShaderType != TerrainShaderType::FALLBACK) ? gfx_api::pixel_format_target::texture_2d_array : gfx_api::pixel_format_target::texture_2d, gfx_api::texture_type::game_texture, mipmap_max, mipmap_max, true);
					ASSERT_OR_RETURN(false, tile != nullptr, "Could not load %s!", fullPath);
				}
				else
				{
					// no more textures in this set
					ASSERT_OR_RETURN(false, k > 0, "Could not find %s", fullPath);
					break;
				}

				tile->resize(mipmap_max, mipmap_max);
				tile->convert_to_rgba();

				ASSERT(tile->width() == mipmap_max && tile->height() == mipmap_max, "Unexpected texture dimensions");

				// Blit this tile onto the parent texture atlas bitmap
				if (!textureAtlas.blit_image(*tile, xOffset, yOffset))
				{
					debug(LOG_ERROR, "Failed to copy source image to texture atlas");
				}

				tileTexInfo[k].uOffset = (float)xOffset / (float)xSize;
				tileTexInfo[k].vOffset = (float)yOffset / (float)ySize;
				tileTexInfo[k].texPage = texPage;
				debug(LOG_TEXTURE, "  texLoad: Registering k=%d i=%d u=%f v=%f xoff=%d yoff=%d xsize=%d ysize=%d tex=%d (%s)",
					  k, mipmap_max, tileTexInfo[k].uOffset, tileTexInfo[k].vOffset, xOffset, yOffset, xSize, ySize, texPage, fullPath);

				xOffset += mipmap_max; // mipmap_max is width of tile
				if (xOffset + mipmap_max > xLimit)
				{
					yOffset += mipmap_max; // mipmap_max is also height of tile
					xOffset = 0;
				}

				if (yOffset + mipmap_max > yLimit)
				{
					/* Upload current working texture page */
					if (!uploadTextureAtlasPage(std::move(textureAtlas), texPage))
					{
						ASSERT_OR_RETURN(false, false, "Could not upload texture atlas page [%d]: %s", texPage, fullPath);
					}
					/* Change to next texture page */
					xOffset = 0;
					yOffset = 0;
					debug(LOG_TEXTURE, "texLoad: Extra page added at %d for %s, was page %d, opengl id %u",
						  k, partialPath, texPage, (unsigned)pie_Texture(texPage).id());
					++pageNum;
					texPage = newPage(fileName, pageNum, xSize, ySize, k);
					textureAtlas = iV_Image();
					textureAtlas.allocate(xSize, ySize, 4, true);
				}
			}
			/* Upload current working texture page */
			if (!uploadTextureAtlasPage(std::move(textureAtlas), texPage))
			{
				ASSERT_OR_RETURN(false, false, "Could not upload texture atlas page [%d]: %s", texPage, fullPath);
			}
			debug(LOG_TEXTURE, "texLoad: Found %d textures for %s mipmap level %d, added to page %d, opengl id %u",
				  k, partialPath, mipmap_max, texPage, (unsigned)pie_Texture(texPage).id());
			break;
		case TerrainShaderType::SINGLE_PASS:
			{

				std::vector<WzString> tile_base_filepaths; tile_base_filepaths.reserve(maxTileNo+1);
				std::vector<WzString> tile_nm_filepaths; tile_nm_filepaths.reserve(maxTileNo+1);
				std::vector<WzString> tile_sm_filepaths; tile_sm_filepaths.reserve(maxTileNo+1);
				std::vector<WzString> tile_hm_filepaths; tile_hm_filepaths.reserve(maxTileNo+1);

				std::vector<WzString> usedFilenames_tmp;
				for (k = 0; k <= maxTileNo; ++k)
				{
					auto fullPath_base = gfx_api::imageLoadFilenameFromInputFilename(WzString::fromUtf8(astringf("%s/tile-%02d.png", partialPath, k)));
					tile_base_filepaths.push_back(fullPath_base);
					usedFilenames_tmp.push_back(fullPath_base);

					if (has_auxillary_texture_info)
					{
						fullPath_nm = gfx_api::imageLoadFilenameFromInputFilename(WzString::fromUtf8(astringf("%s/tile-%02d_nm.png", partialPath, k)));
						fullPath_sm = gfx_api::imageLoadFilenameFromInputFilename(WzString::fromUtf8(astringf("%s/tile-%02d_sm.png", partialPath, k)));
						fullPath_hm = gfx_api::imageLoadFilenameFromInputFilename(WzString::fromUtf8(astringf("%s/tile-%02d_hm.png", partialPath, k)));

						if (has_nm)
						{
							usedFilenames_tmp.push_back(fullPath_nm);
						}
						if (has_sm)
						{
							usedFilenames_tmp.push_back(fullPath_sm);
						}
						if (has_hm)
						{
							usedFilenames_tmp.push_back(fullPath_hm);
						}

						if (!gfx_api::checkImageFilesWouldLoadFromSameParentMountPath(usedFilenames_tmp, true))
						{
							debug(LOG_INFO, "One of these images would load from a different path / package: %s, %s, %s, %s - defaulting to using base texture only", fullPath, fullPath_nm.toUtf8().c_str(), fullPath_sm.toUtf8().c_str(), fullPath_hm.toUtf8().c_str());
							fullPath_nm.clear();
							fullPath_sm.clear();
							fullPath_hm.clear();
						}
						if (has_nm)
						{
							if (!PHYSFS_exists(fullPath_nm))
							{
								fullPath_nm.clear();
							}
							tile_nm_filepaths.push_back(fullPath_nm);
						}
						if (has_sm)
						{
							if (!PHYSFS_exists(fullPath_sm))
							{
								fullPath_sm.clear();
							}
							tile_sm_filepaths.push_back(fullPath_sm);
						}
						if (has_hm)
						{
							if (!PHYSFS_exists(fullPath_hm))
							{
								fullPath_hm.clear();
							}
							tile_hm_filepaths.push_back(fullPath_hm);
						}
					}
				}

				decalTexArr = gfx_api::context::get().loadTextureArrayFromFiles(tile_base_filepaths, gfx_api::texture_type::game_texture, mipmap_max, mipmap_max, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
						std::unique_ptr<iV_Image> pDefaultTexture = std::unique_ptr<iV_Image>(new iV_Image);
						pDefaultTexture->allocate(width, height, channels, true);
						return pDefaultTexture;
					}, nullptr, "decalTexArr");
				if (has_auxillary_texture_info)
				{
					if (has_nm)
					{
						decalNormalArr = gfx_api::context::get().loadTextureArrayFromFiles(tile_nm_filepaths, gfx_api::texture_type::normal_map, mipmap_max, mipmap_max, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
							std::unique_ptr<iV_Image> pDefaultNormalMap = std::make_unique<iV_Image>();
							pDefaultNormalMap->allocate(width, height, channels, true);
							// default normal map: (0,0,1)
							unsigned char* pBmpWrite = pDefaultNormalMap->bmp_w();
							memset(pBmpWrite, 0x7f, pDefaultNormalMap->data_size());
							if (channels >= 3)
							{
								size_t pixelIncrement = static_cast<size_t>(channels);
								for (size_t b = 0; b < pDefaultNormalMap->data_size(); b += pixelIncrement)
								{
									pBmpWrite[b+2] = 0xff; // blue=z
								}
							}
							return pDefaultNormalMap;
						}, nullptr, "decalNormalArr");
						ASSERT(decalNormalArr != nullptr, "Failed to load tile normals");
					}
					if (has_sm)
					{
						decalSpecularArr = gfx_api::context::get().loadTextureArrayFromFiles(tile_sm_filepaths, gfx_api::texture_type::specular_map, mipmap_max, mipmap_max, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
							std::unique_ptr<iV_Image> pDefaultSpecularMap = std::make_unique<iV_Image>();
							// default specular map: 0
							pDefaultSpecularMap->allocate(width, height, channels, true);
							return pDefaultSpecularMap;
						}, nullptr, "decalSpecularArr");
						ASSERT(decalSpecularArr != nullptr, "Failed to load tile specular maps");
					}
					if (has_hm)
					{
						decalHeightArr = gfx_api::context::get().loadTextureArrayFromFiles(tile_hm_filepaths, gfx_api::texture_type::height_map, mipmap_max, mipmap_max, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
							std::unique_ptr<iV_Image> pDefaultHeightMap = std::make_unique<iV_Image>();
							// default height map: 0
							pDefaultHeightMap->allocate(width, height, channels, true);
							return pDefaultHeightMap;
						}, nullptr, "decalHeightArr");
						ASSERT(decalHeightArr != nullptr, "Failed to load terrain height maps");
					}
				}
				// flush all of the array textures
				if (decalTexArr)
				{
					decalTexArr->flush();
				}
				else
				{
					debug(LOG_FATAL, "Failed to load one or more terrain decals");
				}
				if (decalNormalArr) { decalNormalArr->flush(); }
				if (decalSpecularArr) { decalSpecularArr->flush(); }
				if (decalHeightArr) { decalHeightArr->flush(); }
			}
			break;
		}
	}
	return true;
}

gfx_api::texture* getFallbackTerrainDecalsPage()
{
	return &pie_Texture(terrainPage.value_or(0));
}

bool reloadTileTextures()
{
	if (!tilesetDir)
	{
		return false;
	}
	switch (getTerrainShaderType())
	{
		case TerrainShaderType::FALLBACK:
			if (terrainPage.has_value())
			{
				pie_AssignTexture(terrainPage.value(), nullptr);
			}
			break;
		case TerrainShaderType::SINGLE_PASS:
			break;
	}
	char *dir = strdup(tilesetDir);
	bool result = texLoad(dir);
	free(dir);
	return result;
}
