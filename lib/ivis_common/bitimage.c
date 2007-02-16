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


static BOOL LoadTextureFile(char *FileName, iSprite *TPage, int *TPageID);

UWORD iV_GetImageWidth(IMAGEFILE *ImageFile, UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].Width;
}

UWORD iV_GetImageHeight(IMAGEFILE *ImageFile, UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].Height;
}

SWORD iV_GetImageXOffset(IMAGEFILE *ImageFile, UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].XOffset;
}

SWORD iV_GetImageYOffset(IMAGEFILE *ImageFile, UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].YOffset;
}

UWORD iV_GetImageCenterX(IMAGEFILE *ImageFile, UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].XOffset + ImageFile->ImageDefs[ID].Width/2;
}

UWORD iV_GetImageCenterY(IMAGEFILE *ImageFile, UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].YOffset + ImageFile->ImageDefs[ID].Height/2;
}

IMAGEFILE *iV_LoadImageFile(char *FileData, UDWORD FileSize)
{
	char *Ptr;
	IMAGEHEADER *Header;
	IMAGEFILE *ImageFile;
	IMAGEDEF *ImageDef;
	int i;


	Ptr = FileData;

	Header = (IMAGEHEADER*)Ptr;
	Ptr += sizeof(IMAGEHEADER);

	endian_uword(&Header->Version);
	endian_uword(&Header->NumImages);
	endian_uword(&Header->BitDepth);
	endian_uword(&Header->NumTPages);

	ImageFile = (IMAGEFILE*)MALLOC(sizeof(IMAGEFILE));
	if(ImageFile == NULL) {
		debug( LOG_ERROR, "Out of memory" );
		return NULL;
	}


	ImageFile->TexturePages = (iSprite*)MALLOC(sizeof(iSprite)*Header->NumTPages);
	if(ImageFile->TexturePages == NULL) {
		debug( LOG_ERROR, "Out of memory" );
		return NULL;
	}

	ImageFile->ImageDefs = (IMAGEDEF*)MALLOC(sizeof(IMAGEDEF)*Header->NumImages);
	if(ImageFile->ImageDefs == NULL) {
		debug( LOG_ERROR, "Out of memory" );
		return NULL;
	}

	ImageFile->Header = *Header;

	// Load the texture pages.
	for (i = 0; i < Header->NumTPages; i++) {
		int tmp;	/* Workaround for MacOS gcc 4.0.0 bug. */
		LoadTextureFile((char*)Header->TPageFiles[i],
				&ImageFile->TexturePages[i],
				&tmp);
		ImageFile->TPageIDs[i] = tmp;
	}

	ImageDef = (IMAGEDEF*)Ptr;

	for(i=0; i<Header->NumImages; i++) {
		endian_uword(&ImageDef->TPageID);
		endian_uword(&ImageDef->PalID);
		endian_uword(&ImageDef->Tu);
		endian_uword(&ImageDef->Tv);
		endian_uword(&ImageDef->Width);
		endian_uword(&ImageDef->Height);
		endian_sword(&ImageDef->XOffset);
		endian_sword(&ImageDef->YOffset);

		ImageFile->ImageDefs[i] = *ImageDef;
		if( (ImageDef->Width <= 0) || (ImageDef->Height <= 0) ) {
			debug( LOG_ERROR, "Illegal image size" );
			return NULL;
		}
		ImageDef++;
	}

	return ImageFile;
}


void iV_FreeImageFile(IMAGEFILE *ImageFile)
{

//	for(i=0; i<ImageFile->Header.NumTPages; i++) {
//		FREE(ImageFile->TexturePages[i].bmp);
//	}

	FREE(ImageFile->TexturePages);
	FREE(ImageFile->ImageDefs);
	FREE(ImageFile);
}


static BOOL LoadTextureFile(char *FileName, iSprite *pSprite, int *texPageID)
{
	int i=0;

	debug(LOG_TEXTURE, "LoadTextureFile: %s", FileName);

	if (!resPresent("IMGPAGE",FileName)) {
		debug(LOG_ERROR, "Texture file \"%s\" not preloaded.", FileName);
		assert(FALSE);
		return FALSE;
	} else {
		*pSprite = *(iSprite*)resGetData("IMGPAGE", FileName);
		debug(LOG_TEXTURE, "Load texture from resource cache: %s (%d, %d)",
		      FileName, pSprite->width, pSprite->height);
	}

	/* We have already loaded this one? */
	while (i < _TEX_INDEX) {
		if (strcasecmp(FileName, _TEX_PAGE[i].name) == 0) {
			*texPageID = (_TEX_PAGE[i].textPage3dfx);
			debug(LOG_TEXTURE, "LoadTextureFile: already loaded");
			return TRUE;
		}
		i++;
	}

#ifdef PIETOOL
	*texPageID=NULL;
#else
	*texPageID = pie_AddBMPtoTexPages(pSprite, FileName, 1, TRUE);
#endif
	return TRUE;
}
