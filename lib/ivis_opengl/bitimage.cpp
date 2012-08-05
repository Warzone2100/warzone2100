/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
#include "lib/framework/file.h"

#include "bitimage.h"
#include "tex.h"


static unsigned short LoadTextureFile(const char *FileName, int *imageSize)
{
	iV_Image *pSprite;
	unsigned int i;

	ASSERT_OR_RETURN( 0, resPresent("IMGPAGE", FileName), "Texture file \"%s\" not preloaded.", FileName);

	pSprite = (iV_Image*)resGetData("IMGPAGE", FileName);
	debug(LOG_TEXTURE, "Load texture from resource cache: %s (%d, %d)",
	      FileName, pSprite->width, pSprite->height);

	*imageSize = pSprite->width;

	/* Have we already uploaded this one? */
	for (i = 0; i < _TEX_INDEX; ++i)
	{
		if (strcasecmp(FileName, _TEX_PAGE[i].name) == 0)
		{
			debug(LOG_TEXTURE, "LoadTextureFile: already uploaded");
			return _TEX_PAGE[i].id;
		}
	}

	debug(LOG_TEXTURE, "LoadTextureFile: had to upload texture!");
	return pie_AddTexPage(pSprite, FileName, 0, 0, true);
}

IMAGEFILE *iV_LoadImageFile(const char *fileName)
{
	char *pFileData, *ptr, *dot;
	unsigned int pFileSize, numImages = 0, i, tPages = 0;
	IMAGEFILE *ImageFile;
	char texFileName[PATH_MAX];

	if (!loadFile(fileName, &pFileData, &pFileSize))
	{
		debug(LOG_ERROR, "iV_LoadImageFile: failed to open %s", fileName);
		return NULL;
	}
	ptr = pFileData;
	// count lines, which is identical to number of images
	while (ptr < pFileData + pFileSize && *ptr != '\0')
	{
		numImages += (*ptr == '\n') ? 1 : 0;
		ptr++;
	}
	ImageFile = new IMAGEFILE;
	ImageFile->imageDefs.resize(numImages);
	ptr = pFileData;
	numImages = 0;
	while (ptr < pFileData + pFileSize)
	{
		int temp, retval;
		ImageDef *ImageDef = &ImageFile->imageDefs[numImages];

		retval = sscanf(ptr, "%u,%u,%u,%u,%u,%d,%d%n", &ImageDef->TPageID, &ImageDef->Tu, &ImageDef->Tv, &ImageDef->Width,
		       &ImageDef->Height, &ImageDef->XOffset, &ImageDef->YOffset, &temp);
		if (retval != 7)
		{
			break;
		}
		ptr += temp;
		numImages++;
		// Find number of texture pages to load (no gaps allowed in number series, eg use intfac0, intfac1, etc.!)
		if (ImageDef->TPageID > tPages)
		{
			tPages = ImageDef->TPageID;
		}
		while (ptr < pFileData + pFileSize && *ptr++ != '\n') {} // skip rest of line
	}
	ImageFile->pages.resize(tPages + 1);

	dot = (char *)strrchr(fileName, '/');  // go to last path character
	dot++;				// skip it
	strcpy(texFileName, dot);	// make a copy
	dot = strchr(texFileName, '.');	// find extension
	*dot = '\0';			// then discard it
	// Load the texture pages.
	for (i = 0; i <= tPages; i++)
	{
		char path[PATH_MAX];

		snprintf(path, PATH_MAX, "%s%u.png", texFileName, i);
		ImageFile->pages[i].id = LoadTextureFile(path, &ImageFile->pages[i].size);
	}
	free(pFileData);

	return ImageFile;
}

void iV_FreeImageFile(IMAGEFILE *ImageFile)
{
	delete ImageFile;
}
