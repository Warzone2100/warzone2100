#ifndef __INCLUDED_BITIMAGE__
#define __INCLUDED_BITIMAGE__


typedef struct {
	UWORD NumCluts;
	UWORD *ClutIDs;
} CLUTLIST;

typedef void (*CLUTCALLBACK)(UWORD *clut);

UWORD iV_GetImageWidth(IMAGEFILE *ImageFile,UWORD ID);
UWORD iV_GetImageHeight(IMAGEFILE *ImageFile,UWORD ID);
UWORD iV_GetImageWidthNoCC(IMAGEFILE *ImageFile,UWORD ID);
UWORD iV_GetImageHeightNoCC(IMAGEFILE *ImageFile,UWORD ID);
SWORD iV_GetImageXOffset(IMAGEFILE *ImageFile,UWORD ID);
SWORD iV_GetImageYOffset(IMAGEFILE *ImageFile,UWORD ID);
UWORD iV_GetImageCenterX(IMAGEFILE *ImageFile,UWORD ID);
UWORD iV_GetImageCenterY(IMAGEFILE *ImageFile,UWORD ID);

IMAGEFILE *iV_LoadImageFile(char *FileData, UDWORD FileSize);
void iV_FreeImageFile(IMAGEFILE *ImageFile);

#endif
