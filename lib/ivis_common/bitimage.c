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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "piepalette.h"
#include "tex.h"
#include "ivispatch.h"
#include "bitimage.h"
#include "lib/framework/frameresource.h"
#include <physfs.h>


static unsigned short LoadTextureFile(const char *FileName, iTexture *pSprite)
{
	unsigned int i;

	ASSERT(resPresent("IMGPAGE", FileName), "Texture file \"%s\" not preloaded.", FileName);

	*pSprite = *(iTexture*)resGetData("IMGPAGE", FileName);
	debug(LOG_TEXTURE, "Load texture from resource cache: %s (%d, %d)",
	      FileName, pSprite->width, pSprite->height);

	/* Have we already loaded this one? */
	for (i = 0; i < _TEX_INDEX; ++i)
	{
		if (strcasecmp(FileName, _TEX_PAGE[i].name) == 0)
		{
			debug(LOG_TEXTURE, "LoadTextureFile: already loaded");
			return _TEX_PAGE[i].id;
		}
	}

	return pie_AddTexPage(pSprite, FileName, 0);
}

static inline IMAGEFILE* iV_AllocImageFile(size_t NumTPages, size_t NumImages)
{
	const size_t totalSize = sizeof(IMAGEFILE) + sizeof(iTexture) * NumTPages + sizeof(IMAGEDEF) * NumImages;

	IMAGEFILE* ImageFile = malloc(totalSize);
	if (ImageFile == NULL)
	{
		debug(LOG_ERROR, "iV_AllocImageFile: Out of memory");
		return NULL;
	}

	// Set member pointers to their respective areas in the allocated memory area
	ImageFile->TexturePages = (iTexture*)(ImageFile + 1);
	ImageFile->ImageDefs = (IMAGEDEF*)(ImageFile->TexturePages + NumTPages);

	return ImageFile;
}

IMAGEFILE *iV_LoadImageFile(const char *fileName)
{
	IMAGEFILE *ImageFile;
	IMAGEDEF* ImageDef;
	unsigned int i;

	IMAGEHEADER Header;
	PHYSFS_file* fileHandle;

	fileHandle = PHYSFS_openRead(fileName);
	if (!fileHandle)
	{
		debug(LOG_ERROR, "iV_LoadImageFile: PHYSFS_openRead failed (opening %s) with error: %s", fileName, PHYSFS_getLastError());
		return NULL;
	}

	// Read header from file
	PHYSFS_readULE16 (fileHandle, &Header.Version);
	PHYSFS_readULE16 (fileHandle, &Header.NumImages);
	PHYSFS_readULE16 (fileHandle, &Header.BitDepth);
	PHYSFS_readULE16 (fileHandle, &Header.NumTPages);
	PHYSFS_read      (fileHandle, &Header.TPageFiles, sizeof(Header.TPageFiles), 1);

	ImageFile = iV_AllocImageFile(Header.NumTPages, Header.NumImages);
	if(ImageFile == NULL)
	{
		PHYSFS_close(fileHandle);
		return NULL;
	}

	ImageFile->Header = Header;

	// Load the texture pages.
	for (i = 0; i < Header.NumTPages; i++)
	{
		ImageFile->TPageIDs[i] = LoadTextureFile((char *)Header.TPageFiles[i], &ImageFile->TexturePages[i]);
	}

	for(ImageDef = &ImageFile->ImageDefs[0]; ImageDef != &ImageFile->ImageDefs[Header.NumImages]; ++ImageDef)
	{
		// Read image definition from file
		PHYSFS_readULE16(fileHandle, &ImageDef->TPageID);
		PHYSFS_readULE16(fileHandle, &ImageDef->Tu);
		PHYSFS_readULE16(fileHandle, &ImageDef->Tv);
		PHYSFS_readULE16(fileHandle, &ImageDef->Width);
		PHYSFS_readULE16(fileHandle, &ImageDef->Height);
		PHYSFS_readSLE16(fileHandle, &ImageDef->XOffset);
		PHYSFS_readSLE16(fileHandle, &ImageDef->YOffset);

		if( (ImageDef->Width <= 0) || (ImageDef->Height <= 0) )
		{
			debug( LOG_ERROR, "iV_LoadImageFromFile: Illegal image size" );
			free(ImageFile);
			PHYSFS_close(fileHandle);
			return NULL;
		}
	}

	PHYSFS_close(fileHandle);

	return ImageFile;
}

void iV_FreeImageFile(IMAGEFILE *ImageFile)
{
	free(ImageFile);
}
