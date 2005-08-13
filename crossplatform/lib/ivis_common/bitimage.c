#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include "rendmode.h"
#include "bug.h"
#include "piepalette.h"
#include "pcx.h"
#include "tex.h"
#include "ivispatch.h"
#include "bitimage.h"

extern BOOL   pie_Hardware(void);

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

IMAGEFILE *iV_LoadImageFile(UBYTE *FileData, UDWORD FileSize)
{
	UBYTE *Ptr;
	IMAGEHEADER *Header;
	IMAGEFILE *ImageFile;
	IMAGEDEF *ImageDef;

	int i;


	Ptr = FileData;

	Header = (IMAGEHEADER*)Ptr;
	Ptr += sizeof(IMAGEHEADER);

	ImageFile = MALLOC(sizeof(IMAGEFILE));
	if(ImageFile == NULL) {
		DBERROR(("Out of memory"));
		return NULL;
	}


	ImageFile->TexturePages = MALLOC(sizeof(iSprite)*Header->NumTPages);
	if(ImageFile->TexturePages == NULL) {
		DBERROR(("Out of memory"));
		return NULL;
	}

	ImageFile->ImageDefs = MALLOC(sizeof(IMAGEDEF)*Header->NumImages);
	if(ImageFile->ImageDefs == NULL) {
		DBERROR(("Out of memory"));
		return NULL;
	}

	ImageFile->Header = *Header;

	// Load the texture pages.
	for (i = 0; i < Header->NumTPages; i++) {
		LoadTextureFile((STRING*)Header->TPageFiles[i], &ImageFile->TexturePages[i],
                    (int*)&ImageFile->TPageIDs[i]);
	}

	ImageDef = (IMAGEDEF*)Ptr;
	for(i=0; i<Header->NumImages; i++) {
		ImageFile->ImageDefs[i] = *ImageDef;
		if( (ImageDef->Width <= 0) || (ImageDef->Height <= 0) ) {
			DBERROR(("Illegal image size"));
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

	DBPRINTF(("ltf) %s\n",FileName));

	if (!resPresent("IMGPAGE",FileName)) {
		if (!iV_PCXLoad(FileName,pSprite,NULL)) {
			DBERROR(("Unable to load texture file : %s",FileName));
			return FALSE;
		}
	} else {
		*pSprite = *(iSprite*)resGetData("IMGPAGE",FileName);
	}

	/* Back to beginning */
	i = 0;
	/* Have we already loaded this one then? */
	while (i<_TEX_INDEX) 
	{
		if (stricmp(FileName,_TEX_PAGE[i].name) == 0) {
			if (pie_Hardware()) {
				*texPageID = (_TEX_PAGE[i].textPage3dfx);
			} else {
				*texPageID = i;
			}
			return TRUE;
		}
		i++;
	}

#ifdef PIETOOL
	*texPageID=NULL;
#else
	*texPageID = pie_AddBMPtoTexPages(pSprite, FileName, 1, TRUE, TRUE);
#endif
	return TRUE;
}
