#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dos.h>
#include "rendmode.h"
#include "bug.h"
#include "piePalette.h"
#include "pcx.h"
#include "tex.h"
#include "ivispatch.h"

#ifdef WIN32

#ifdef INC_GLIDE
	#include "tex.h"
	#include "3dfxText.h"
	#include "3dfxfunc.h"
#endif

#else

#include "vpsx.h"
#include "psxvram.h"

#endif

#include "BitImage.h"





#ifdef WIN32
static BOOL LoadTextureFile(char *FileName,iSprite *TPage,int *TPageID);

#else

static BOOL LoadTextureFile(char *FileName,UWORD *TPageID);
static UWORD LoadClutFile(UBYTE *FileName,UWORD *ClutIDs);

#endif

UWORD iV_GetImageWidth(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
#ifdef WIN32
	return ImageFile->ImageDefs[ID].Width;
#else
	return PSXToWidth(ImageFile->ImageDefs[ID].Width);	// /2;
#endif
}

UWORD iV_GetImageHeight(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
#ifdef WIN32
	return ImageFile->ImageDefs[ID].Height;
#else
	return PSXToHeight(ImageFile->ImageDefs[ID].Height);	// /2;
#endif

}


// Get image width with no coordinate conversion.
//
UWORD iV_GetImageWidthNoCC(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].Width;
}

// Get image height with no coordinate conversion.
//
UWORD iV_GetImageHeightNoCC(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
	return ImageFile->ImageDefs[ID].Height;
}


SWORD iV_GetImageXOffset(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
#ifdef WIN32
	return ImageFile->ImageDefs[ID].XOffset;
#else
	return PSXToX(ImageFile->ImageDefs[ID].XOffset);	// /2;
#endif
}

SWORD iV_GetImageYOffset(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
#ifdef WIN32
	return ImageFile->ImageDefs[ID].YOffset;
#else
	return PSXToY(ImageFile->ImageDefs[ID].YOffset);	// /2;
#endif
}

UWORD iV_GetImageCenterX(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
#ifdef WIN32
	return ImageFile->ImageDefs[ID].XOffset + ImageFile->ImageDefs[ID].Width/2;
#else
	return PSXToX(ImageFile->ImageDefs[ID].XOffset + ImageFile->ImageDefs[ID].Width/2);	// /2;
#endif
}

UWORD iV_GetImageCenterY(IMAGEFILE *ImageFile,UWORD ID)
{
	assert(ID < ImageFile->Header.NumImages);
#ifdef WIN32
	return ImageFile->ImageDefs[ID].YOffset + ImageFile->ImageDefs[ID].Height/2;
#else
	return PSXToY(ImageFile->ImageDefs[ID].YOffset + ImageFile->ImageDefs[ID].Height/2);	// /2;
#endif
}

#ifdef WIN32

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
	for(i=0; i<Header->NumTPages; i++) {
		LoadTextureFile((char*)Header->TPageFiles[i],&ImageFile->TexturePages[i],(int*)&ImageFile->TPageIDs[i]);
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


static BOOL LoadTextureFile(char *FileName,iSprite *pSprite,int *texPageID)
{
	SDWORD i;
//	iPalette pal;

	DBPRINTF(("ltf) %s\n",FileName));

	if(!resPresent("IMGPAGE",FileName))
	{
		if(!iV_PCXLoad(FileName,pSprite,NULL))
		{
			DBERROR(("Unable to load texture file : %s",FileName));
			return FALSE;
		}
	}
	else
	{
		*pSprite = *(iSprite*)resGetData("IMGPAGE",FileName);
	}

	/* Back to beginning */
	i = 0;
	/* Have we already loaded this one then? */
	while (i<_TEX_INDEX) 
	{
		if (stricmp(FileName,_TEX_PAGE[i].name) == 0)
		{
			/* Send back 3dfx texpage number if we're on 3dfx - they're NOT the same */
		 	if(rendSurface.usr == REND_GLIDE_3DFX)
			{

				*texPageID = (_TEX_PAGE[i].textPage3dfx);
				return TRUE;
			}
			else
			{
				/* Otherwise send back the software one */
				*texPageID = i;
				return TRUE;
			}
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

#else	// End of PC version, start of PSX version.

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

// Allocate memory for the image file.
	ImageFile = MALLOC(sizeof(IMAGEFILE));
	if(ImageFile == NULL) {
		DBERROR(("Out of memory"));
		return NULL;
	}

// Allocate memory for the image definitions in the image file.
	ImageFile->ImageDefs = MALLOC(sizeof(IMAGEDEF)*Header->NumImages);
	if(ImageFile->ImageDefs == NULL) {
		DBERROR(("Out of memory"));
		return NULL;
	}

DBPRINTF(("IMAGEFILE ALLOC - %p %p\n",ImageFile,ImageFile->ImageDefs));

	ImageFile->Header = *Header;

// Load the texture pages.
	for(i=0; i<Header->NumTPages; i++) {
		LoadTextureFile((char*)Header->TPageFiles[i],&ImageFile->TPageIDs[i]);
	}

// Load the Cluts.
	ImageFile->NumCluts = LoadClutFile(Header->PalFile,ImageFile->ClutIDs);

// Create the image definitions.
	ImageDef = (IMAGEDEF*)Ptr;
	for(i=0; i<Header->NumImages; i++) {
		ImageFile->ImageDefs[i] = *ImageDef;
		ImageFile->ImageDefs[i].TPageID = ImageFile->TPageIDs[ImageDef->TPageID];
		ImageFile->ImageDefs[i].PalIndex = ImageFile->ImageDefs[i].PalID;
		ImageFile->ImageDefs[i].PalID = ImageFile->ClutIDs[ImageDef->PalID];
		ImageDef++;
	}

	return ImageFile;
}


void iV_FreeImageFile(IMAGEFILE *ImageFile)
{
DBPRINTF(("IMAGEFILE FREE - %p %p\n",ImageFile,ImageFile->ImageDefs));
	FREE(ImageFile->ImageDefs);
	FREE(ImageFile);
}


// Load a tim format texture page to VRAM.
//
BOOL iV_ReLoadTexturePage_PSX(void *Data,RECT *StoredArea,CLUTCALLBACK ClutCallback)
{
	TIM_IMAGE TimInfo;

	OpenTIM(Data);
	ReadTIM(&TimInfo);

	DrawSync(0);						

	StoredArea->w = TimInfo.prect->w;
	StoredArea->h = TimInfo.prect->h;
DBPRINTF(("width=%d height=%d\n",StoredArea->w,StoredArea->h));

	LoadImage(StoredArea,TimInfo.paddr);
	if (ClutCallback)
	{
		ClutCallback(TimInfo.caddr);
	}

	return TRUE;
}


BOOL iV_LoadTexturePage_PSX(void *Data,RECT *StoredArea,int *TextureMode,CLUTCALLBACK ClutCallback)
{
	TIM_IMAGE TimInfo;
	RECT CurrentImage;
	AREA *VRAMArea;
	UWORD Width,Height;

	OpenTIM(Data);
	ReadTIM(&TimInfo);

// Get it's pixel mode.
	TimInfo.mode &= 3;

	Width = TimInfo.prect->w;
	Width = Width << (UWORD)(2-TimInfo.mode);
	Height = TimInfo.prect->h;

// Allocate vram...
	VRAMArea = AllocTexture(Width,Height, TimInfo.mode,0);
	if (VRAMArea==NULL)	{
		DBPRINTF(("Unable to allocate VRAM!\n"));
	 	return FALSE;
	}

// and download.
	CurrentImage.x=VRAMArea->area_x0;
	CurrentImage.y=VRAMArea->area_y0;
	CurrentImage.w=TimInfo.prect->w;
	CurrentImage.h=TimInfo.prect->h;
	DrawSync(0);
	LoadImage(&CurrentImage,TimInfo.paddr);

	memcpy(StoredArea,VRAMArea,sizeof(RECT));
	*TextureMode=TimInfo.mode;


	if (ClutCallback)
	{
		ClutCallback(TimInfo.caddr);
	}

// Get it's TPage ID.

//	DBPRINTF(("Texture page downloaded x %d y %d TPageID %04x\n",VRAMArea->area_x0,VRAMArea->area_y0,*TPageID));

	return TRUE;
}


// Load a clut file into VRAM.
//
// Allocates a CLUTLIST structure in ClutList which contains the number of
// cluts loaded into VRAM and an array of there ID's.
//
BOOL iV_LoadClut_PSX(UBYTE *Data,CLUTLIST **ClutList,BOOL HalfBright)
{
	UBYTE *Ptr;
	CLUTHEADER *Header;
	UWORD *Def;
	RECT CurrentClut;
	AREA *VRAMArea;
	int i,j;
	int NumCluts = 0;

	Ptr = Data;
	Header = (CLUTHEADER*)Ptr;
	Ptr += sizeof(CLUTHEADER);

	Def = (UWORD*)Ptr;

//	HalfBright = FALSE;

// Allocate the CLUTLIST structure.
	*ClutList = MALLOC(sizeof(CLUTLIST));
	if(*ClutList == NULL) {
		return FALSE;
	}

	// If half bright specified then need to allocate twice as many clut id's
	if(HalfBright) {
		(*ClutList)->ClutIDs = MALLOC(sizeof(UWORD)*Header->NumCluts*3);
		if((*ClutList)->ClutIDs == NULL) {
			return FALSE;
		}
		(*ClutList)->NumCluts = Header->NumCluts*3;
	} else {
		(*ClutList)->ClutIDs = MALLOC(sizeof(UWORD)*Header->NumCluts);
		if((*ClutList)->ClutIDs == NULL) {
			return FALSE;
		}
		(*ClutList)->NumCluts = Header->NumCluts;
	}

	// Download the CLUTS to VRAM.
	for(i=0; i<Header->NumCluts; i++) {

		for(j=1; j<Header->ClutSize; j++) {
			Def[j] |= 0x8000;		// Enable transparency.
		}

		VRAMArea = AllocCLUT(Header->ClutSize);	//*2);

		CurrentClut.x = VRAMArea->area_x0;
		CurrentClut.y = VRAMArea->area_y0;
		CurrentClut.w = Header->ClutSize;
		CurrentClut.h = 1;

		DrawSync(0);
		LoadImage(&CurrentClut,(void*)Def);

		(*ClutList)->ClutIDs[i] = GetClut(VRAMArea->area_x0,VRAMArea->area_y0);

		Def += Header->ClutSize;
		NumCluts++;
	}


	if(HalfBright) {
		Def = (UWORD*)Ptr;

		// Download the CLUTS to VRAM again but at two thirds brightness.
		for(i=0; i<Header->NumCluts; i++) {

			for(j=1; j<Header->ClutSize; j++) {
				int r,g,b,t;
				r = ((Def[j] >> 10) & 0x1f);
				g = ((Def[j] >> 5) & 0x1f);
				b = (Def[j] & 0x1f);

				t = r + g + b;
				r = (r*192) >> 8;		// Two thirds brightness.
				g = (g*192) >> 8;
				b = (b*192) >> 8;
				if( (t != 0) && (r+g+b == 0) ) {
					r = g = b = 1;
				}
				Def[j] = (r << 10) | (g << 5) | (b);
			}

			VRAMArea = AllocCLUT(Header->ClutSize);	//*2);

			CurrentClut.x = VRAMArea->area_x0;
			CurrentClut.y = VRAMArea->area_y0;
			CurrentClut.w = Header->ClutSize;
			CurrentClut.h = 1;

			DrawSync(0);
			LoadImage(&CurrentClut,(void*)Def);

			(*ClutList)->ClutIDs[i+NumCluts] = GetClut(VRAMArea->area_x0,VRAMArea->area_y0);

			Def += Header->ClutSize;
		}

		Def = (UWORD*)Ptr;

		// Download the CLUTS to VRAM again but make them grey scale.
		for(i=0; i<Header->NumCluts; i++) {

			for(j=1; j<Header->ClutSize; j++) {
				int r,g,b,t;
				r = ((Def[j] >> 10) & 0x1f);
				g = ((Def[j] >> 5) & 0x1f);
				b = (Def[j] & 0x1f);

				t = r + g + b;
				r = t/3;		// Grey scale.
				r /= 2;					// and half the brightness.
				if( (t != 0) && (r == 0) ) {
					r = 1;
				}
				Def[j] = (r << 10) | (r << 5) | (r);
			}

			VRAMArea = AllocCLUT(Header->ClutSize);	//*2);

			CurrentClut.x = VRAMArea->area_x0;
			CurrentClut.y = VRAMArea->area_y0;
			CurrentClut.w = Header->ClutSize;
			CurrentClut.h = 1;

			DrawSync(0);
			LoadImage(&CurrentClut,(void*)Def);

			(*ClutList)->ClutIDs[i+NumCluts*2] = GetClut(VRAMArea->area_x0,VRAMArea->area_y0);

			Def += Header->ClutSize;
		}
	}

	return TRUE;
}


// Free up a clut list allocated by iV_LoadClut_PSX.
//
void iV_FreeClut_PSX(CLUTLIST *ClutList)
{
	FREE(ClutList->ClutIDs);
	FREE(ClutList);
}


static BOOL LoadTextureFile(char *FileName,UWORD *TPageID)
{
	UDWORD FileData;

	DBPRINTF(("ltf1) %s : ",FileName));

// Load in the TIM file.
	if(!resPresent("IMGPAGE",FileName))
	{
//		if(!loadFile(FileName,&FileData,&FileSize))
//		{
//			DBERROR(("Unable to load texture file : %s",FileName));
//			return FALSE;
//		}
		DBPRINTF(("Texture page not loaded\n"));
		return FALSE;
	} else {
		FileData = (UDWORD)resGetData("IMGPAGE",FileName);
	}

// Get it's TPage ID.
	*TPageID = (UWORD)FileData;

	DBPRINTF(("Texture page referenced TPageID %04x\n",*TPageID));

	return TRUE;
}


static UWORD LoadClutFile(UBYTE *FileName,UWORD *ClutIDs)
{
	CLUTLIST *ClutList;
	int i;

	DBPRINTF(("LCF) %s\n",FileName));

// Load the CLUT file...
	if(resPresent("IMGCLUT",FileName)) {
		ClutList = (CLUTLIST*)resGetData("IMGCLUT",FileName);
	} else if(resPresent("IMGCLUTHB",FileName)) {
		ClutList = (CLUTLIST*)resGetData("IMGCLUTHB",FileName);
	} else {
		DBPRINTF(("Clut file not loaded\n"));
		return 0;
	}


//	if(!resPresent("IMGCLUT",FileName))
//	{
////		if(!loadFile(FileName,&FileData,&FileSize))
////		{
////			DBERROR(("Unable to load clut file : %s",FileName));
////			return FALSE;
////		}
//		DBPRINTF(("Clut file not loaded\n"));
//		return FALSE;
//	} else {
//		ClutList = (CLUTLIST*)resGetData("IMGCLUT",FileName);
//	}

	for(i=0; i<ClutList->NumCluts; i++) {

		ClutIDs[i] = ClutList->ClutIDs[i];
	}

	return ClutList->NumCluts;
}


//static BOOL LoadTextureFile(char *FileName,UWORD *TPageID)
//{
//	UBYTE *FileData;
//	UDWORD FileSize; 
//	TIM_IMAGE TimInfo;
//	RECT CurrentImage;
//	AREA *VRAMArea;
//	AREAINFO *Avail;
//	UWORD Width,Height;
//
//	DBPRINTF(("ltf1) %s : ",FileName));
//
//// Load in the TIM file.
//	if(!resPresent("IMGPAGE",FileName))
//	{
//		if(!loadFile(FileName,&FileData,&FileSize))
//		{
//			DBERROR(("Unable to load texture file : %s",FileName));
//			return FALSE;
//		}
//	} else {
//		FileData = (UBYTE*)resGetData("IMGPAGE",FileName);
//	}
//
//	OpenTIM((void*)FileData);
//	ReadTIM(&TimInfo);
//
//// Get it's pixel mode.
//	TimInfo.mode &= 3;
//
//	Width = TimInfo.prect->w;
//	Width = Width << (UWORD)(2-TimInfo.mode);
//	Height = TimInfo.prect->h;
//
//// Allocate vram...
//	VRAMArea = AllocTexture(Width,Height, TimInfo.mode,0);
//	if (VRAMArea==NULL)	{
//		DBPRINTF(("Unable to allocate VRAM!\n"));
//	 	return FALSE;
//	}
//
//// and download.
//	CurrentImage.x=VRAMArea->area_x0;
//	CurrentImage.y=VRAMArea->area_y0;
//	CurrentImage.w=TimInfo.prect->w;
//	CurrentImage.h=TimInfo.prect->h;
//	DrawSync(0);
//	LoadImage(&CurrentImage,TimInfo.paddr);
//
//// Get it's TPage ID.
//	*TPageID = GetTPage(TimInfo.mode,0,VRAMArea->area_x0,VRAMArea->area_y0);
//
//	DBPRINTF(("Texture page downloaded x %d y %d TPageID %04x\n",VRAMArea->area_x0,VRAMArea->area_y0,*TPageID));
//
////	FREE(FileData);
//
//	return TRUE;
//}
//
//
//static BOOL LoadClutFile(UBYTE *FileName,UWORD *ClutIDs)
//{
//	UBYTE *FileData,*Ptr;
//	UDWORD FileSize;
//	CLUTHEADER *Header;
//	UWORD *Def;
//	RECT CurrentClut;
//	AREA *VRAMArea;
//	int i,j;
//
//	DBPRINTF(("LCF) %s\n",FileName));
//
//// Load the CLUT file...
//	if(!resPresent("IMGCLUT",FileName))
//	{
//		if(!loadFile(FileName,&FileData,&FileSize))
//		{
//			DBERROR(("Unable to load clut file : %s",FileName));
//			return FALSE;
//		}
//	} else {
//		FileData = (UBYTE*)resGetData("IMGCLUT",FileName);
//	}
//
//	Ptr = FileData;
//	Header = (CLUTHEADER*)Ptr;
//	Ptr += sizeof(CLUTHEADER);
//
//	Def = (UWORD*)Ptr;
//
//// and download the CLUTS to VRAM.
//	for(i=0; i<Header->NumCluts; i++) {
//
//		for(j=1; j<Header->ClutSize; j++) {
//			Def[j] |= 0x8000;		// Enable transparency.
//		}
//
//		VRAMArea = AllocCLUT(Header->ClutSize);	//*2);
//
//		CurrentClut.x = VRAMArea->area_x0;
//		CurrentClut.y = VRAMArea->area_y0;
//		CurrentClut.w = Header->ClutSize*2;
//		CurrentClut.h = 1;
//
//		DrawSync(0);
//		LoadImage(&CurrentClut,(void*)Def);
//
//		ClutIDs[i] = GetClut(VRAMArea->area_x0,VRAMArea->area_y0);
//
//		Def += Header->ClutSize;
//	}
//
////	FREE(FileData);
//
//	return TRUE;
//}

#endif

