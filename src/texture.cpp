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

#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/screen.h"

#include "display3ddef.h"
#include "texture.h"
#include "radar.h"
#include "map.h"


#define MIPMAP_LEVELS		4
#define MIPMAP_MAX		128

/* Texture page and coordinates for each tile */
TILE_TEX_INFO tileTexInfo[MAX_TILES];

static int firstPage; // the last used page before we start adding terrain textures
int terrainPage; // texture ID of the terrain page
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

// Generate a new texture page both in the texture page table, and on the graphics card
static int newPage(const char *name, int level, int width, int height, int count)
{
	int texPage = firstPage + ((count + 1) / TILES_IN_PAGE);

	if (texPage == pie_NumberOfPages())
	{
		// We need to create a new texture page; create it and increase texture table to store it
		pie_ReserveTexture(name, width, height);
		pie_AssignTexture(texPage,
			gfx_api::context::get().create_texture(mipmap_levels, width, height,
				gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8));
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

	ASSERT_OR_RETURN(false, MIPMAP_MAX == TILE_WIDTH && MIPMAP_MAX == TILE_HEIGHT, "Bad tile sizes");

	// store the filename so we can later determine which tileset we are using
	if (tilesetDir)
	{
		free(tilesetDir);
	}
	tilesetDir = strdup(fileName);

	// reset defaults
	mipmap_max = MIPMAP_MAX;
	mipmap_levels = MIPMAP_LEVELS;

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

	/* Now load the actual tiles */

	i = mipmap_max; // i is used to keep track of the tile dimensions
	for (j = 0; j < mipmap_levels; j++)
	{
		int xOffset = 0, yOffset = 0; // offsets into the texture atlas
		int xSize = 1;
		int ySize = 1;
		const int xLimit = TILES_IN_PAGE_COLUMN * i;
		const int yLimit = TILES_IN_PAGE_ROW * i;

		// pad width and height into ^2 values
		while (xLimit > (xSize *= 2)) {}
		while (yLimit > (ySize *= 2)) {}

		// Generate the empty texture buffer in VRAM
		texPage = newPage(fileName, j, xSize, ySize, 0);

		sprintf(partialPath, "%s-%d", fileName, i);

		// Load until we cannot find anymore of them
		for (k = 0; k < MAX_TILES; k++)
		{
			iV_Image tile;

			snprintf(fullPath, sizeof(fullPath), "%s/tile-%02d.png", partialPath, k);
			if (PHYSFS_exists(fullPath)) // avoid dire warning
			{
				bool retval = iV_loadImage_PNG(fullPath, &tile);
				ASSERT_OR_RETURN(false, retval, "Could not load %s!", fullPath);
			}
			else
			{
				// no more textures in this set
				ASSERT_OR_RETURN(false, k > 0, "Could not find %s", fullPath);
				break;
			}
			// Insert into texture page
			pie_Texture(texPage).upload(j, xOffset, yOffset, tile.width, tile.height, gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8, tile.bmp);
			free(tile.bmp);
			if (i == mipmap_max) // dealing with main texture page; so register coordinates
			{
				tileTexInfo[k].uOffset = (float)xOffset / (float)xSize;
				tileTexInfo[k].vOffset = (float)yOffset / (float)ySize;
				tileTexInfo[k].texPage = texPage;
				debug(LOG_TEXTURE, "  texLoad: Registering k=%d i=%d u=%f v=%f xoff=%d yoff=%d xsize=%d ysize=%d tex=%d (%s)",
				      k, i, tileTexInfo[k].uOffset, tileTexInfo[k].vOffset, xOffset, yOffset, xSize, ySize, texPage, fullPath);
			}
			xOffset += i; // i is width of tile
			if (xOffset + i > xLimit)
			{
				yOffset += i; // i is also height of tile
				xOffset = 0;
			}
			if (yOffset + i > yLimit)
			{
				/* Change to next texture page */
				xOffset = 0;
				yOffset = 0;
				debug(LOG_TEXTURE, "texLoad: Extra page added at %d for %s, was page %d, opengl id %u",
				      k, partialPath, texPage, (unsigned)pie_Texture(texPage).id());
				texPage = newPage(fileName, j, xSize, ySize, k);
			}
		}
		debug(LOG_TEXTURE, "texLoad: Found %d textures for %s mipmap level %d, added to page %d, opengl id %u",
		      k, partialPath, i, texPage, (unsigned)pie_Texture(texPage).id());
		i /= 2;	// halve the dimensions for the next series; OpenGL mipmaps start with largest at level zero
	}
	return true;
}
