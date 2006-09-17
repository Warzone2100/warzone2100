#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bug.h"
#include "piepalette.h"
#include "tex.h"
#include "ivispatch.h"
#include "bitimage.h"

static BOOL LoadTextureFile(STRING *FileName, iSprite *TPage, int *TPageID);

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


// Get image width with no coordinate conversion.
//
UWORD iV_GetImageWidthNoCC(IMAGEFILE *ImageFile, UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].Width;
}

// Get image height with no coordinate conversion.
//
UWORD iV_GetImageHeightNoCC(IMAGEFILE *ImageFile, UWORD ID)
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
		LoadTextureFile((STRING*)Header->TPageFiles[i],
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


static BOOL LoadTextureFile(STRING *FileName, iSprite *pSprite, int *texPageID)
{
	SDWORD i;
	char real_filename[200];

	// this is a hideous kludge to avoid having to change .img files, which
	// still contain pcx references
	strcpy(real_filename, FileName);
	real_filename[strlen(real_filename) - 4] = '\0'; // strip extension
	strcat(real_filename, ".png");

	debug(LOG_TEXTURE, "LoadTextureFile: %s", real_filename);

	if (!resPresent("IMGPAGE",real_filename)) {
		debug(LOG_ERROR, "Texture file \"%s\" not preloaded.", real_filename);
		assert(FALSE);
		return FALSE;
	} else {
		*pSprite = *(iSprite*)resGetData("IMGPAGE", real_filename);
		debug(LOG_TEXTURE, "Load texture from resource cache: %s (%d, %d)", 
		      real_filename, pSprite->width, pSprite->height);
	}

	/* Back to beginning */
	i = 0;

	/* We have already loaded this one? */
	while (i < _TEX_INDEX) {
		if (stricmp(real_filename, _TEX_PAGE[i].name) == 0) {
			*texPageID = (_TEX_PAGE[i].textPage3dfx);
			debug(LOG_TEXTURE, "LoadTextureFile: already loaded");
			return TRUE;
		}
		i++;
	}

#ifdef PIETOOL
	*texPageID=NULL;
#else
	*texPageID = pie_AddBMPtoTexPages(pSprite, real_filename, 1, TRUE, TRUE);
#endif
	return TRUE;
}
