/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

/* This is where we do texture atlas generation */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/ivis_common/pietypes.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_opengl/pietexture.h"
#include "lib/ivis_opengl/screen.h"
#include "display3ddef.h"
#include "texture.h"
#include "radar.h"

#include <physfs.h>
#include <SDL/SDL_opengl.h>
#ifdef __APPLE__
#include <opengl/glu.h>
#else
#include <GL/glu.h>
#endif

#define MIPMAP_LEVELS		4
#define MIPMAP_MIN		16
#define MIPMAP_MAX		128

/* Texture page and coordinates for each tile */
TILE_TEX_INFO tileTexInfo[MAX_TILES];

static int firstPage; // the last used page before we start adding terrain textures

// Generate a new texture page both in the texture page table, and on the graphics card
static int newPage(const char *name, int level, int width, int height, int count)
{
	int texPage = firstPage + ((count + 1) / TILES_IN_PAGE);

	// debug(LOG_TEXTURE, "newPage: texPage=%d firstPage=%d %s %d (%d,%d) (count %d + 1) / %d _TEX_INDEX=%u",
	//      texPage, firstPage, name, level, width, height, count, TILES_IN_PAGE, _TEX_INDEX);
	if (texPage == _TEX_INDEX)
	{
		// We need to create a new texture page; create it and increase texture table to store it
		glGenTextures(1, (GLuint *) &_TEX_PAGE[texPage].id);
		_TEX_INDEX++;
	}

	ASSERT(_TEX_INDEX > texPage, "newPage: Index too low (%d > %d)", _TEX_INDEX, texPage);
	ASSERT(_TEX_INDEX < iV_TEX_MAX, "Too many texture pages used");

	strncpy(_TEX_PAGE[texPage].name, name, iV_TEXNAME_MAX);

	pie_SetTexturePage(texPage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_LEVELS - 1);
	// debug(LOG_TEXTURE, "newPage: glTexImage2D(page=%d, level=%d) opengl id=%u", texPage, level, _TEX_PAGE[texPage].id);
	glTexImage2D(GL_TEXTURE_2D, level, wz_texture_compression, width, height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return texPage;
}

static inline WZ_DECL_CONST unsigned int getTileUIndex(unsigned int tileNumber)
{
	return (tileNumber % TILES_IN_PAGE) % TILES_IN_PAGE_COLUMN;
}

static inline WZ_DECL_CONST unsigned int getTileVIndex(unsigned int tileNumber)
{
	return (tileNumber % TILES_IN_PAGE) / TILES_IN_PAGE_ROW;
}

void texLoad(const char *fileName)
{
	char fullPath[PATH_MAX], partialPath[PATH_MAX], *buffer;
	unsigned int i, j, k, size;
	int texPage;

	firstPage = _TEX_INDEX;

	ASSERT(_TEX_INDEX < iV_TEX_MAX, "Too many texture pages used");
	ASSERT(MIPMAP_MAX == TILE_WIDTH && MIPMAP_MAX == TILE_HEIGHT, "Bad tile sizes");

	/* Get and set radar colours */

	sprintf(fullPath, "%s.radar", fileName);
	if (!loadFile(fullPath, &buffer, &size))
	{
		debug(LOG_ERROR, "texLoad: Could not find radar colours at %s", fullPath);
		abort(); // cannot recover; we could possibly generate a random palette?
	}
	i = 0; // tile
	k = 0; // number of values read
	j = 0; // place in buffer
	do {
		unsigned int r, g, b;
		int cnt = 0;

		k = sscanf(buffer + j, "%2x%2x%2x%n", &r, &g, &b, &cnt);
		j += cnt;
		if (k >= 3)
		{
			radarColour(i, r, g, b);
		}
		i++; // next tile
	} while (k >= 3 && j + 6 < size);
	free(buffer);

	/* Now load the actual tiles */

	i = MIPMAP_MAX; // i is used to keep track of the tile dimensions
	for (j = 0; j < MIPMAP_LEVELS; j++)
	{
		int xOffset = 0, yOffset = 0; // offsets into the texture atlas
		int xSize = TILES_IN_PAGE_COLUMN * i;
		int ySize = TILES_IN_PAGE_ROW * i;

		// Generate the empty texture buffer in VRAM
		texPage = newPage(fileName, j, xSize, ySize, 0);

		sprintf(partialPath, "%s-%d", fileName, i);

		// Load until we cannot find anymore of them
		for (k = 0; k < MAX_TILES; k++)
		{
			iTexture tile;

			sprintf(fullPath, "%s/tile-%02d.png", partialPath, k);
			if (PHYSFS_exists(fullPath)) // avoid dire warning
			{
				BOOL retval = iV_loadImage_PNG(fullPath, &tile);
				ASSERT(retval, "texLoad: Could not load %s", fullPath);
			}
			else
			{
				// no more textures in this set
				ASSERT(k > 0, "texLoad: Could not find %s", fullPath);
				break;
			}
			// Insert into texture page
			glTexSubImage2D(GL_TEXTURE_2D, j, xOffset, yOffset, tile.width, tile.height,
			                GL_RGBA, GL_UNSIGNED_BYTE, tile.bmp);
			free(tile.bmp);
			xOffset += i; // i is width of tile
			if (xOffset >= xSize)
			{
				yOffset += i; // i is also height of tile
				xOffset = 0;
			}
			if (i == TILE_WIDTH) // dealing with main texture page; so register coordinates
			{
				// 256 is an integer hack for GLfloat texture coordinates
				tileTexInfo[k].uOffset = getTileUIndex(k) * (256 / TILES_IN_PAGE_COLUMN);
				tileTexInfo[k].vOffset = getTileVIndex(k) * (256 / TILES_IN_PAGE_ROW);
				tileTexInfo[k].texPage = texPage;
				//debug(LOG_TEXTURE, "  texLoad: Registering k=%d i=%d u=%f v=%f xoff=%u yoff=%u tex=%d (%s)",
				//     k, i, tileTexInfo[k].uOffset, tileTexInfo[k].vOffset, xOffset, yOffset, texPage, fullPath);
			}
			if (yOffset >= ySize)
			{
				/* Change to next texture page */
				xOffset = 0;
				yOffset = 0;
				debug(LOG_TEXTURE, "texLoad: Extra page added at %d for %s, was page %d, opengl id %u",
				      k, partialPath, texPage, _TEX_PAGE[texPage].id);
				texPage = newPage(fileName, j, xSize, ySize, k);
			}
		}
		debug(LOG_TEXTURE, "texLoad: Found %d textures for %s mipmap level %d, added to page %d, opengl id %u",
		      k, partialPath, i, texPage, _TEX_PAGE[texPage].id);
		i /= 2;	// halve the dimensions for the next series; OpenGL mipmaps start with largest at level zero
	}
}
