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
size_t terrainPage; // texture ID of the terrain page
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
	while (res > 0 && !PHYSFS_exists(WzString::fromUtf8(dir + "-" + std::to_string(res) + "/tile-00.png")))
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
	int texPage;

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

	sprintf(fullPath, "%s.radar", fileName);
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

	auto uploadTextureAtlasPage = [fileName](iV_Image &&bitmap, size_t texPage) {
		std::string textureDebugName = std::string("pageTexture::") + ((fileName) ? fileName : "");
		auto pTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(bitmap), gfx_api::texture_type::game_texture, textureDebugName.c_str(), -1, -1);
		pie_AssignTexture(texPage, pTexture);
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

		// Generate the empty texture buffer in VRAM
		texPage = newPage(fileName, pageNum, xSize, ySize, 0);
		iV_Image textureAtlas;
		textureAtlas.allocate(xSize, ySize, 4, true);

		sprintf(partialPath, "%s-%d", fileName, maxTileTexSize);

		// Load until we cannot find anymore of them
		for (k = 0; k < MAX_TILES; k++)
		{
			std::unique_ptr<iV_Image> tile;

			snprintf(fullPath, sizeof(fullPath), "%s/tile-%02d.png", partialPath, k);
			if (PHYSFS_exists(fullPath)) // avoid dire warning
			{
				tile = gfx_api::loadUncompressedImageFromFile(fullPath, gfx_api::texture_type::game_texture, -1, -1, true);
				ASSERT_OR_RETURN(false, tile != nullptr, "Could not load %s!", fullPath);
			}
			else
			{
				// no more textures in this set
				ASSERT_OR_RETURN(false, k > 0, "Could not find %s", fullPath);
				break;
			}

			tile->scale_image_max_size(mipmap_max, mipmap_max);
			tile->convert_to_rgba();

			ASSERT(tile->width() == mipmap_max && tile->height() == mipmap_max, "Unexpected texture dimensions");

			// Blit this tile onto the parent texture atlas bitmap
			if (!textureAtlas.blit_image(*tile, xOffset, yOffset))
			{
				debug(LOG_ERROR, "Failed to copy source image to texture atlas");
			}

			tile->clear();
			tile.reset();

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
				uploadTextureAtlasPage(std::move(textureAtlas), texPage);
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
		uploadTextureAtlasPage(std::move(textureAtlas), texPage);
		debug(LOG_TEXTURE, "texLoad: Found %d textures for %s mipmap level %d, added to page %d, opengl id %u",
		      k, partialPath, mipmap_max, texPage, (unsigned)pie_Texture(texPage).id());
	}
	return true;
}
