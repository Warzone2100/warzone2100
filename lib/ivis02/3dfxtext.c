#ifdef INC_GLIDE
/* 3dfxText.c */

/* Texture Handling Functions for a 3dfx card */
/* Alex McLean, Pumpkin Studios, 1997 */

#include "Frame.h"
#include "stdlib.h"
#include "dGlide.h"
#include "stdio.h"
#include "3dfxText.h"
#include "pieState.h"
#include "bug.h"
#include "tex.h"

static	unsigned long	g3dfxMinVideo;
static	unsigned long	g3dfxMaxVideo;
static	unsigned long	g3dfxPageSize;
static	unsigned long	g3dfxRadarSize;
static	unsigned long	g3dfxMaxPages;
static	unsigned long	g3dfxPresVideo;
static	unsigned long	g3dfxMemUsed;
unsigned long	presentPage;
unsigned long	lastPageDownloaded;

#define PAGE_SIZE	65536
#define MAX_TEXTURE_PAGES 32
BOOL	downLoad8bitTexturePageAbs(void* bitmap,UWORD Width,UWORD Height,FxU32 destination);

/* -------------------------------------------------------------------------------------- */
/*	
	Set up the texture memory handling pointers 
	We're doing our own texture memory management remember!	
*/
/* -------------------------------------------------------------------------------------- */
void	init3dfxTexMemory( void )
{
	/* Get the base address of texture memory */
	g3dfxMinVideo = grTexMinAddress(GR_TMU0);

	/* Get the maximum address */
	g3dfxMaxVideo = grTexMaxAddress(GR_TMU0);

	/* set our counter to equal base address*/
	g3dfxPresVideo = g3dfxMinVideo;

	/* get page size*/
	g3dfxPageSize = grTexCalcMemRequired(GR_LOD_256,GR_LOD_256,GR_ASPECT_1x1,GR_TEXFMT_P_8);
	g3dfxRadarSize = grTexCalcMemRequired(GR_LOD_128,GR_LOD_128,GR_ASPECT_1x1,GR_TEXFMT_P_8);

	/* and calculate max pages*/
	g3dfxMaxPages = ((g3dfxMaxVideo - g3dfxMinVideo) - g3dfxRadarSize)/g3dfxPageSize;
	ASSERT((g3dfxMaxPages >= (MAX_TEXTURE_PAGES - 1), "init3dfxTexMemory, Unable to allocate sufficient texture space"));

	/* None used so far */
	g3dfxMemUsed = 0;

	/* No pages used */
	presentPage = 0;

	/* No page downloaded */
	lastPageDownloaded = 0;
}

/* -------------------------------------------------------------------------------------- */
/*	
	Set memory pointers back to beginnning of texture memory - can only be called
	after init3dfxTexMemory has been called 
*/
/* -------------------------------------------------------------------------------------- */
void	free3dfxTexMemory( void )
{
	g3dfxPresVideo = g3dfxMinVideo;
	g3dfxMemUsed = 0;
}

/* -------------------------------------------------------------------------------------- */
/* 
	Returns the LOD (level of detail) value in the correct enumeration for 3dfx 
*/
/* -------------------------------------------------------------------------------------- */
GrLOD_t	getLODVal(short dim)
{
GrLOD_t retVal = 0;

	switch(dim)
	{
	case 1:
		retVal = GR_LOD_1;
		break;

	case 2:
		retVal = GR_LOD_2;
		break;

	case 4:
		retVal = GR_LOD_4;
		break;

	case 8:
		retVal = GR_LOD_8;
		break;

	case 16:
		retVal = GR_LOD_16;
		break;

	case 32:
		retVal = GR_LOD_32;
		break;

	case 64:
		retVal = GR_LOD_64;
		break;

	case 128:
		retVal = GR_LOD_128;
		break;

	case 256:
		retVal = GR_LOD_256;
		break;
	}
	return (retVal);
}
/* -------------------------------------------------------------------------------------- */
/* 
	Gets the aspect ratio of the texture - should always be 1x1 for us with 256*256
	pages. Aspect ration is essentially MAX(width,height)/min(width,height)
	
*/
/* -------------------------------------------------------------------------------------- */
GrAspectRatio_t getAspectRatio(short width, short height)
{
GrAspectRatio_t	ratio;
int	divisor;

	if(width>= height)
	{
		divisor = width/height;
		switch(divisor)
		{
		case 1:
			ratio = GR_ASPECT_1x1;
			break;

		case 2:
			ratio = GR_ASPECT_2x1;
			break;

		case 4:
			ratio = GR_ASPECT_4x1;
			break;

		case 8:
			ratio = GR_ASPECT_8x1;
			break;
		default:
			ratio = GR_ASPECT_1x1;
		}
	}
	else
		if(height > width)
		{
			divisor = height/width;
			switch(divisor)
			{
			case 1:
				ratio = GR_ASPECT_1x1;
				break;

			case 2:
				ratio = GR_ASPECT_1x2;
				break;

			case 4:
				ratio = GR_ASPECT_1x4;
				break;

			case 8:
				ratio = GR_ASPECT_1x8;
				break;
			default:
				ratio = GR_ASPECT_1x1;
		}
	}
	return(ratio);
}

/* -------------------------------------------------------------------------------------- */
/* 
	Attempts to get an address for a new page to be downloaded. Will return the address to 
	give to textureDownload if possible otherwise will print an error! 
	
*/
/* -------------------------------------------------------------------------------------- */
FxU32	alloc3dfxTexPage( void )
{
GrMipMapId_t	handle;
FxU32			retVal;

	handle = g3dfxPresVideo;
	if(handle+g3dfxPageSize>g3dfxMaxVideo)
	{
		handle = GR_NULL_MIPMAP_HANDLE;
	}
	else
	{
		g3dfxPresVideo+=g3dfxPageSize;
	}

	if(handle == GR_NULL_MIPMAP_HANDLE)
	{
		printf("FAILED TO ALLOC 3DFX MEMORY\n");
	}
	else
	{
		retVal = (handle|0x80000000);
	}

	return(retVal);
}

/* -------------------------------------------------------------------------------------- */
/* 
	Just sets up the texture and colour units so that bilinear textured polygons 
	can be drawn.
*/
/* -------------------------------------------------------------------------------------- */
void gl_SetupTexturing(void)
{
#ifdef STATES
	grTexCombine(GR_TMU0,
		GR_COMBINE_FUNCTION_LOCAL,
		GR_COMBINE_FACTOR_LOCAL,
		GR_COMBINE_FUNCTION_LOCAL,
		GR_COMBINE_FACTOR_LOCAL,
		FXFALSE,FXFALSE);

    /* Enable Bilinear Filtering + Mipmapping */
	pie_SetBilinear(TRUE);
    grTexMipMapMode( GR_TMU0,
                     GR_MIPMAP_NEAREST,
                     FXFALSE );
	pie_SetColourCombine(TRUE);
#endif	
}

/* -------------------------------------------------------------------------------------- */
/* 
	This will actually send a 256 square texture page to the video card.
	Only square textures and only 256*256 sized ones at that! 
*/
/* -------------------------------------------------------------------------------------- */
int	gl_downLoad8bitTexturePage(UBYTE* bitmap,UWORD Width,UWORD Height)
{
FxU32		destination;

	/* Find out where to send it */
	destination = alloc3dfxTexPage();

	if(downLoad8bitTexturePageAbs(bitmap,Width,Height,destination) == FALSE) {
		return 0;
	}

	return presentPage-1;
}

int	gl_GetLastPageDownLoaded(void)
{
	return lastPageDownloaded;
}

BOOL downLoad8bitTexturePageAbs(void* bitmap,UWORD Width,UWORD Height,FxU32 destination)
{
	GrTexInfo	info;

	iV_DEBUG2("downloadTexture %p %x\n",bitmap,destination);

	/* Was there enough space? */
	if(destination == GR_NULL_MIPMAP_HANDLE)
	{
		/* If not, bomb out */
		iV_DEBUG0("downloadTexture FAILED, no memory\n");
		DBERROR(("Can't download the page - full up?"));
		return FALSE;
	}
	/* Otherwise we're ok and can download */
	else
	{
		/* Tell TMU the size etc... */
		info.smallLod = getLODVal(Width);	//GR_LOD_256;

		if(info.smallLod < 0) {
			iV_DEBUG0("downloadTexture FAILED, illegal Width\n");
			DBERROR(("Can't download the page - illegal Width?"));
			return FALSE;
		}

		info.largeLod = getLODVal(Height);	//GR_LOD_256;

		if(info.largeLod < 0) {
			iV_DEBUG0("downloadTexture FAILED, illegal Height\n");
			DBERROR(("Can't download the page - illegal Height?"));
			return FALSE;
		}

		info.aspectRatio = getAspectRatio(Width,Height);	//GR_ASPECT_1x1;
		info.format = GR_TEXFMT_P_8;
		info.data = bitmap;

		/* Download texture data to TMU */
		grTexDownloadMipMap( GR_TMU0,
                         destination,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &info );

		/* Update page */
		lastPageDownloaded = presentPage;

		/* One more page used */
		presentPage++;

		iV_DEBUG1("downloadTexture %d OK\n",presentPage-1);

		/* Everything's fine! */
		return TRUE;
	}
}

int	gl_Reload8bitTexturePage(UBYTE* bitmap,UWORD Width,UWORD Height,SDWORD index)
{
	FxU32 destination;
	GrTexInfo	info;

	/* Tell TMU the size etc... */
	ASSERT((index < (SDWORD)g3dfxMaxPages,"gl_Reload8bitTexturePage: index out of range"));
	ASSERT((Width == 256,"gl_Reload8bitTexturePage: texture not 256 square"));
	ASSERT((Height == 256,"gl_Reload8bitTexturePage: texture not 256 square"));

	/* Usual 256 square stuff */
	info.smallLod = GR_LOD_256;
	info.largeLod = GR_LOD_256;
	info.aspectRatio = GR_ASPECT_1x1;
	info.format = GR_TEXFMT_P_8;
	info.data = bitmap;

	destination = g3dfxMinVideo + index * g3dfxPageSize;

	/* Download texture data to TMU */
	grTexDownloadMipMap( GR_TMU0,
		                 destination,
                         GR_MIPMAPLEVELMASK_BOTH,
                         &info );

	/* Update page */
	lastPageDownloaded = index;

	/* Everything's fine! */
	return TRUE;
}

/* -------------------------------------------------------------------------------------- */
/* 
	Quickie function that just dumps a texture page to the screen so we can view it.
	Could easily call the showSpecificTextPage with lastPageDownLoaded as a parameter!
*/
/* -------------------------------------------------------------------------------------- */
void showCurrentTextPage( void )
{
GrTexInfo	info;
GrVertex vtxs[4];
	
		/* Usual 256 square stuff */
		info.smallLod = GR_LOD_256;
		info.largeLod = GR_LOD_256;
		info.aspectRatio = GR_ASPECT_1x1;
		info.format = GR_TEXFMT_P_8;
		info.data = 0;

		/* Clear the screen */
		grBufferClear(0,0,GR_ZDEPTHVALUE_FARTHEST);

 	    vtxs[0].oow = 1.0f;
        vtxs[1] = vtxs[2] = vtxs[3] = vtxs[0];
		vtxs[0].x = (float) 100;
		vtxs[0].y = (float) 100;

		vtxs[1].x = (float) 400;
		vtxs[1].y = (float) 100;
		
		vtxs[2].x = (float) 400;
		vtxs[2].y = (float) 400;
		
		vtxs[3].x = (float) 100;
		vtxs[3].y = (float) 400;

		vtxs[0].tmuvtx[0].sow = 0.0f;
		vtxs[0].tmuvtx[0].tow = 0.0f;

		vtxs[1].tmuvtx[0].sow = 255.0f;
		vtxs[1].tmuvtx[0].tow = 0.0f;
		
		vtxs[2].tmuvtx[0].sow = 255.0f;
		vtxs[2].tmuvtx[0].tow = 255.0f;
		
		vtxs[3].tmuvtx[0].sow = 0.0f;
		vtxs[3].tmuvtx[0].tow = 255.0f;

		/* Bright as possible */
        vtxs[0].r = 255.0f, vtxs[0].g =   255.0f, vtxs[0].b =   255.0f, vtxs[0].a = 128.0f;
        vtxs[1].r = 255.0f, vtxs[1].g =   255.0f, vtxs[1].b =   255.0f, vtxs[1].a = 128.0f;
        vtxs[2].r = 255.0f, vtxs[2].g =   255.0f, vtxs[2].b =   255.0f, vtxs[2].a = 128.0f;
        vtxs[3].r = 255.0f, vtxs[3].g =   255.0f, vtxs[3].b =   255.0f, vtxs[3].a = 128.0f;

	/* Dump it to the screen */
  	grDrawPlanarPolygonVertexList(4,vtxs);
	/* NO 3dfx logo */
	grGlideShamelessPlug(FXFALSE);
	/* Flip buffers */
	grBufferSwap(1);
	/* Wait for blitter nasty bit ?! */
	grSstIdle();
}


/* -------------------------------------------------------------------------------------- */
/*
	Displays any specified texture page on screen, clearing what was there before. 
*/
/* -------------------------------------------------------------------------------------- */
void showSpecificTextPage( UDWORD num )
{
GrTexInfo	info;
GrVertex vtxs[4];
	
		info.smallLod = GR_LOD_256;
		info.largeLod = GR_LOD_256;
		info.aspectRatio = GR_ASPECT_1x1;
		info.format = GR_TEXFMT_P_8;
		info.data = 0;

	/* Select Texture As Source of all texturing operations */
		grTexSource( GR_TMU0,
                 num*PAGE_SIZE,
                 GR_MIPMAPLEVELMASK_BOTH,
                 &info );
		grBufferClear(0,0,GR_ZDEPTHVALUE_FARTHEST);

		vtxs[0].oow = 1.0f;
        vtxs[1] = vtxs[2] = vtxs[3] = vtxs[0];
		vtxs[0].x = (float) 100;
		vtxs[0].y = (float) 100;

		vtxs[1].x = (float) 400;
		vtxs[1].y = (float) 100;
		
		vtxs[2].x = (float) 400;
		vtxs[2].y = (float) 400;
		
		vtxs[3].x = (float) 100;
		vtxs[3].y = (float) 400;

		vtxs[0].tmuvtx[0].sow = 0.0f;
		vtxs[0].tmuvtx[0].tow = 0.0f;

		vtxs[1].tmuvtx[0].sow = 255.0f;
		vtxs[1].tmuvtx[0].tow = 0.0f;
		
		vtxs[2].tmuvtx[0].sow = 255.0f;
		vtxs[2].tmuvtx[0].tow = 255.0f;
		
		vtxs[3].tmuvtx[0].sow = 0.0f;
		vtxs[3].tmuvtx[0].tow = 255.0f;

        vtxs[0].r = 255.0f, vtxs[0].g =   255.0f, vtxs[0].b =   255.0f, vtxs[0].a = 255.0f;
        vtxs[1].r = 255.0f, vtxs[1].g =   255.0f, vtxs[1].b =   255.0f, vtxs[1].a = 255.0f;
        vtxs[2].r = 255.0f, vtxs[2].g =   255.0f, vtxs[2].b =   255.0f, vtxs[2].a = 255.0f;
        vtxs[3].r = 255.0f, vtxs[3].g =   255.0f, vtxs[3].b =   255.0f, vtxs[3].a = 255.0f;

  	grDrawPlanarPolygonVertexList(4,vtxs);
#ifdef STATES
	pie_SetColour(0x00ff0000);
#endif
	grBufferSwap(1);
	grSstIdle();
}

/* -------------------------------------------------------------------------------------- */
/* 
	Selects a specific texture page to be the source of all future texture operations
	until we select another. Optimised in the sense that it won't do the switch if
	what you're asking for is the same as the presently selected one.
*/
/* -------------------------------------------------------------------------------------- */
BOOL	gl_SelectTexturePage( UDWORD num )
{
GrTexInfo	info;

	if(num >= MAX_TEXTURE_PAGES)
	{
		return(FALSE);
	}
	if (num == 31)
	{
	 	info.smallLod = GR_LOD_128;
		info.largeLod = GR_LOD_128;
	}
	else
	{
		info.smallLod = GR_LOD_256;
		info.largeLod = GR_LOD_256;
	}
	info.aspectRatio = GR_ASPECT_1x1;
	info.format = GR_TEXFMT_P_8;
	info.data = 0;
//	presentPage = num;

	/* Select Texture As Source of all texturing operations */
	grTexSource( GR_TMU0,
             num*(PAGE_SIZE),
             GR_MIPMAPLEVELMASK_BOTH,
             &info );
	return(TRUE);
}

BOOL gl_RestoreTextures(void)
{
	SDWORD		i;
	//free them and try for MED textures
	for (i= 0; (i < iV_TEX_MAX); i++)
	{
		/* set pie texture pointer */
		if ((_TEX_PAGE[i].tex.bmp != NULL) && (i != 31))
		{
			gl_Reload8bitTexturePage( _TEX_PAGE[i].tex.bmp , _TEX_PAGE[i].tex.width , _TEX_PAGE[i].tex.height , i);
		}
	}
	return TRUE;
}


#endif