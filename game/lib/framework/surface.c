/*
 * Surface.c
 *
 * Utility functions for loading image data into surfaces.
 *
 */

// surfRecreate pix format stuff 
//#define DEBUG_GROUP1
#include "frame.h"
#include "frameint.h"

#define NUM_8BIT_PAL_ENTRIES	256

/* The number of bits in one colour gun of the windows PALETTEENTRY struct */
#define PALETTEENTRY_BITS 8

/* The list of all surfaces created by surfCreate() */
SURFACE_LIST	*psSurfaces = NULL;

/* Create a DD surface */
BOOL surfCreate(LPDIRECTDRAWSURFACE4	*ppsSurface,	// The created surface
			   UDWORD width, UDWORD height,			// The size of the surface
			   UDWORD				caps,			// The caps bits for the surfac
			   DDPIXELFORMAT		*psPixFormat,	// The pixel format for the surface
			   BOOL					bColourKey,		// Colour key flag
			   BOOL					bAddToList   )	// Add pointer to surface list
{
	HRESULT			ddrval;
	LPDIRECTDRAW4	psDD;
	DDSURFACEDESC2	ddsd;
	SURFACE_LIST	*psNew;

	bColourKey = bColourKey;

	psDD = screenGetDDObject();

	ASSERT((psDD != NULL,
		"surfCreate: NULL DD object - framework not initialised?"));

	if ((screenMode == SCREEN_WINDOWED) && (displayMode == MODE_8BITFUDGE))
	{
		/* Can't store the surfaces in video memory when we're fudging 8bit mode */
		caps = (caps & ~((UDWORD)DDSCAPS_VIDEOMEMORY));
	}

	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);

	if (caps == 0)
	{
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	}
	else
	{
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT |
					   DDSD_CAPS ;
	}
	
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	ddsd.ddsCaps.dwCaps = caps;
	if (psPixFormat == NULL)
	{
		memcpy(&ddsd.ddpfPixelFormat, (void *)screenGetBackBufferPixelFormat(),
			sizeof(DDPIXELFORMAT));
	}
	else
	{
		memcpy(&ddsd.ddpfPixelFormat, (void *)psPixFormat, sizeof(DDPIXELFORMAT));
	}
	ddrval = psDD->lpVtbl->CreateSurface(
				psDD,
				&ddsd,
				ppsSurface,
				NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Create surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
	
	if (bAddToList)
	{
		/* Create a new surface list entry */
		psNew = (SURFACE_LIST *)MALLOC(sizeof(SURFACE_LIST));
		if (psNew == NULL)
		{
			DBERROR(("Out of memory"));
			return FALSE;
		}

		/* Add the entry to the list */
		psNew->psSurface = *ppsSurface;
		psNew->psNext = psSurfaces;
		psSurfaces = psNew;
	}

	return TRUE;
}

/* Release a surface */
void surfRelease(LPDIRECTDRAWSURFACE4	psSurface)
{
	SURFACE_LIST	*psPrev, *psCurr;


	if (psSurfaces->psSurface == psSurface)
	{
		psCurr = psSurfaces;
		psSurfaces = psSurfaces->psNext;
		RELEASE(psCurr->psSurface);
		FREE(psCurr);
	}
	else
	{
		for(psCurr = psSurfaces; (psCurr != NULL) && (psCurr->psSurface != psSurface);
			psCurr = psCurr->psNext)
		{
			psPrev = psCurr;
		}
		ASSERT((psCurr != NULL,
			"surfRelease: Couldn't find surface"));
		if (psCurr != NULL)
		{
			psPrev->psNext = psCurr->psNext;
			RELEASE(psCurr->psSurface);
			FREE(psCurr);
		}
	}
}

/* Re-create a surface - useful for video surfaces after a mode change */
BOOL surfRecreate(LPDIRECTDRAWSURFACE4 *ppsSurface)
{
	DDSURFACEDESC2	ddsd;
	HRESULT			ddrval;
	SURFACE_LIST	*psCurr;
	LPDIRECTDRAW4	psDD;
	UDWORD			mask;

	psDD = screenGetDDObject();

	/* Find the surface entry */
	for(psCurr = psSurfaces; (psCurr != NULL) && (psCurr->psSurface != *ppsSurface);
		psCurr = psCurr->psNext)
		;

	if (psCurr == NULL)
	{
		DBERROR(("Couldn't find surface"));
		return FALSE;
	}

	/* Get the description of the old surface */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddrval = (*ppsSurface)->lpVtbl->GetSurfaceDesc(*ppsSurface, &ddsd);
	if (ddrval != DD_OK)
	{
		ASSERT((FALSE, "Couldn't get surface description:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

	/* only want the caps, width,height and pixel format from this */
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

	if ((displayMode == MODE_8BITFUDGE) && (screenMode == SCREEN_WINDOWED))
	{
		/* Direct draw will have set the pixel format to the windows pixel format
		 * even though you can't use the surface.
		 * Have to make the surface non video memory and 8bit again.
		 */
		mask = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM;
		ddsd.ddsCaps.dwCaps = ddsd.ddsCaps.dwCaps & ~mask;
		memset(&ddsd.ddpfPixelFormat, 0, sizeof(DDPIXELFORMAT));
		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
	}

	DBP1(("Recreating surface with %d bpp\nSurf %p",
		ddsd.ddpfPixelFormat.dwRGBBitCount, *ppsSurface));

	/* Release the old surface */
	RELEASE(*ppsSurface);

	/* Create it again */
	ddrval = psDD->lpVtbl->CreateSurface(
				psDD,
				&ddsd,
				ppsSurface,
				NULL);
	if (ddrval != DD_OK)
	{
		ASSERT((FALSE, "Create surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

	DBP1(("to %p\n", *ppsSurface));

#ifdef DEBUG
	ddrval = (*ppsSurface)->lpVtbl->GetSurfaceDesc(*ppsSurface, &ddsd);
	if (ddrval != DD_OK)
	{
		ASSERT((FALSE, "Couldn't get surface description:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
	DBP1(("New bpp for surface : %d\n", ddsd.ddpfPixelFormat.dwRGBBitCount));
#endif

	psCurr->psSurface = *ppsSurface;
}

/* Release all the allocated surfaces */
void surfShutDown(void)
{
	SURFACE_LIST	*psCurr, *psNext;

	for(psCurr = psSurfaces; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		/* The DD surface might have been released elsewhere */
		if (psCurr->psSurface->lpVtbl != NULL)
		{
			RELEASE(psCurr->psSurface);
		}
		FREE(psCurr);
	}
}


/* Copy the data from one surface to another - useful for loading
 * a video memory surface from a system memory surface.
 */
BOOL surfLoadFromSurface(
			LPDIRECTDRAWSURFACE4		psDest,		// The surface to load to
			LPDIRECTDRAWSURFACE4		psSrc)		// The surface to load from
{
	DDSURFACEDESC2	ddsdSrc, ddsdDest;
	RECT			sDestRect, sSrcRect;
	BOOL			destVideo, srcSystem;
	HRESULT			ddrval;

	/* Get the size of the surfaces */
	memset(&ddsdDest, 0, sizeof(DDSURFACEDESC2));
	ddsdDest.dwSize = sizeof(DDSURFACEDESC2);
	ddrval = psDest->lpVtbl->GetSurfaceDesc(psDest, &ddsdDest);
	if (ddrval != DD_OK)
	{
		ASSERT((FALSE, "Couldn't get surface description:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
	sDestRect.left = 0;
	sDestRect.top = 0;
	sDestRect.right = ddsdDest.dwWidth;
	sDestRect.bottom = ddsdDest.dwHeight;
	destVideo = ddsdDest.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY;

	memset(&ddsdSrc, 0, sizeof(DDSURFACEDESC2));
	ddsdSrc.dwSize = sizeof(DDSURFACEDESC2);
	ddrval = psSrc->lpVtbl->GetSurfaceDesc(psSrc, &ddsdSrc);
	if (ddrval != DD_OK)
	{
		ASSERT((FALSE, "Couldn't get surface description:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
	sSrcRect.left = 0;
	sSrcRect.top = 0;
	sSrcRect.right = ddsdSrc.dwWidth;
	sSrcRect.bottom = ddsdSrc.dwHeight;
	srcSystem = ddsdSrc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;

	/* Now cut down the source rectangle if it's bigger than the destination surface */
	sSrcRect.right = sSrcRect.right > sDestRect.right ? sDestRect.right : sSrcRect.right;
	sSrcRect.bottom = sSrcRect.bottom > sDestRect.bottom ? sDestRect.bottom : sSrcRect.bottom;

	/* Now make sure the rects are the same,
	 * effectively this just reduces the DestRect if it is bigger than the source.
	 */
	sDestRect.right = sSrcRect.right;
	sDestRect.bottom = sSrcRect.bottom;

	/* Copy the data into the surface */
	ddrval = psDest->lpVtbl->Blt(psDest, &sDestRect,
								 psSrc, &sSrcRect,
								 DDBLT_WAIT, NULL);
	if (ddrval != DD_OK)
	{
		ASSERT((FALSE, "Couldn't do the blit:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

	return TRUE;
}

/***************************************************************************/
/*
 * DDColorMatch
 *
 * convert a RGB color to a pysical color.
 *
 * we do this by leting GDI SetPixel() do the color matching
 * then we lock the memory and see what it got mapped to.
 */
/***************************************************************************/
/*********NOT************/

/***************************************************************************/
/*
 * DDSetColorKey
 *
 * set a color key for a surface, given a RGB.
 * if you pass CLR_INVALID as the color key, the pixel
 * in the upper-left corner will be used.
 */
/***************************************************************************/

HRESULT
DDSetColorKey(IDirectDrawSurface4 *pdds, COLORREF rgb)
{
    DDCOLORKEY          ddck;

    ddck.dwColorSpaceLowValue  = rgb;
    ddck.dwColorSpaceHighValue = ddck.dwColorSpaceLowValue;
    return pdds->lpVtbl->SetColorKey(pdds, DDCKEY_SRCBLT, &ddck);
}

/***************************************************************************/

/* Load image data into a Direct Draw surface */
BOOL surfLoadFrom8Bit(
			LPDIRECTDRAWSURFACE4		psSurf,			// The surface to load to
			UDWORD width, UDWORD height,			// The size of the image data
			UBYTE				*pImageData,			// The image data
			PALETTEENTRY		*psPalette)			// The image palette data
{
	UDWORD		i,j;					// Loop counters
	UDWORD		surfWidth, surfHeight;	// actual size of surface
	DWORD		palFlags;				// Flags for creating the palette
	UBYTE		*pImageSrc;				// Pointer into the image data
	UBYTE		*pImageEnd;				// End of the image data
	UBYTE		*pSurf8Dest;
	UWORD		*pSurf16Dest;			// Pointers into the image surface
	UDWORD		*pSurf32Dest;
	HRESULT		ddrval;					// DD or D3D return value
	DDSURFACEDESC2			ddsd;		// Surface description for creating surfaces
	LPDIRECTDRAWPALETTE		psPal=0;		// The textures DD palette
//	PALETTEENTRY			psNewPalette;	// The surfaces palette if necessary
	DDPIXELFORMAT			sPixelFormat;	// The surfaces pixel format
	LPDIRECTDRAW4			psDD;			// The direct draw object

	BOOL		aColoursUsed[NUM_8BIT_PAL_ENTRIES];
	UDWORD		coloursUsed;

	/* Variables for storing the bit patterns for RGB data */
	SDWORD		rShift,rPalShift;
	SDWORD		gShift,gPalShift;
	SDWORD		bShift,bPalShift;
	SDWORD		aShift,aPalShift;
	DWORD		currMask;
	UWORD		r,g,b;


	/* Validate the arguments */
	ASSERT((psSurf != NULL, "NULL surface pointer"));
	ASSERT((PTRVALID(pImageData, width*height),
			"Invalid image data pointer."));
	ASSERT((PTRVALID(psPalette, NUM_8BIT_PAL_ENTRIES*sizeof(PALETTEENTRY)),
			"Invalid palette pointer."));

	/* Get the DD object */
	psDD = screenGetDDObject();

	ASSERT((psDD != NULL,
		"surfLoadFrom8Bit: NULL DD object - framework not initialised?"));

	/* Get the pixel format for the surface */
	memset(&sPixelFormat, 0, sizeof(DDPIXELFORMAT));
	sPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddrval=psSurf->lpVtbl->GetPixelFormat(psSurf, &sPixelFormat);
	if (ddrval != DD_OK)
	{
		DBERROR(("Couldn't get pixel format for surface load:\n%s",
			DDErrorToString(ddrval)));
		return FALSE;
	}

	ASSERT((sPixelFormat.dwRGBBitCount >= 8,
		"surfLoadFrom8Bit: less than 8 bit palettised not yet implemented"));

	/* Create a palette for the texture if necessary */
	if (sPixelFormat.dwRGBBitCount <= 8)
	{
		if (sPixelFormat.dwRGBBitCount < 8)
		{
			/* The surface is paletised with less than 256 colours.
			 * Need to see how many colours are in the image data.
			 */
			for(i=0; i<NUM_8BIT_PAL_ENTRIES; i++)
			{
				aColoursUsed[i] = FALSE;
			}
			pImageEnd = pImageData + width * height;
			for(pImageSrc = pImageData; pImageSrc < pImageEnd; pImageSrc++)
			{
				aColoursUsed[*pImageSrc] = TRUE;
			}
			coloursUsed = 0;
			for(i=0; i<NUM_8BIT_PAL_ENTRIES; i++)
			{
				if (aColoursUsed[i])
				{
					coloursUsed++;
				}
			}
		}
		/* Might have to deal with paletted surfaces that aren't 8 bit */
		switch (sPixelFormat.dwRGBBitCount)
		{
		case 1:
			if (coloursUsed > 2)
			{
				DBERROR(("Too many colours to load image into surface"));
				return FALSE;
			}
			palFlags = DDPCAPS_1BIT;
			break;
		case 2:
			if (coloursUsed > 4)
			{
				DBERROR(("Too many colours to load image into surface"));
				return FALSE;
			}
			palFlags = DDPCAPS_2BIT;
			break;
		case 4:
			if (coloursUsed > 16)
			{
				DBERROR(("Too many colours to load image into surface"));
				return FALSE;
			}
			palFlags = DDPCAPS_4BIT;
			break;
		case 8:
			palFlags = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
			break;
		}
		ddrval = psDD->lpVtbl->CreatePalette(
					psDD,
					palFlags,
					psPalette,
					&psPal, NULL);
		if (ddrval != DD_OK)
		{
			DBERROR(("CreatePalette failed for image surface:\n%s",
					 DDErrorToString(ddrval)));
			goto exit_with_error;
		}
		/* Set the palette on the texture surface */
		ddrval = psSurf->lpVtbl->SetPalette(psSurf, psPal);
		if (ddrval != DD_OK)
		{
			DBERROR(("SetPalette failed for image surface:\n%s",
					DDErrorToString(ddrval)));
			goto exit_with_error;
		}
	}

	/* Lock the texture surface to store the image data */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddrval = psSurf->lpVtbl->Lock(psSurf, NULL, &ddsd, 0, NULL);
//	ddsd.lpSurface = MALLOC(surfWidth*surfHeight*4);
//	ddsd.lPitch = surfWidth;
	if (ddrval != DD_OK)
	{
		DBERROR(("Lock failed for surface image load:\n%s",
				 DDErrorToString(ddrval)));
		goto exit_with_error;
	}

	surfWidth = ddsd.dwWidth;
	surfHeight = ddsd.dwHeight;

	/* store the texture data */
	if (sPixelFormat.dwRGBBitCount >= 16)
	{
		/* Storing to a true colour surface.
		   Find the shifts needed to store each colour */
		if ((sPixelFormat.dwRBitMask > 0) &&
			(sPixelFormat.dwGBitMask > 0) &&
			(sPixelFormat.dwBBitMask > 0))
		{
			/*lint -save -e722 */
			currMask = sPixelFormat.dwRBitMask;
			for(rShift=0; !(currMask & 1); currMask >>= 1, rShift++);
			for(rPalShift=0; (currMask & 1); currMask >>=1, rPalShift++);
			rPalShift = PALETTEENTRY_BITS - rPalShift;

			currMask = sPixelFormat.dwGBitMask;
			for(gShift=0; !(currMask & 1); currMask >>= 1, gShift++);
			for(gPalShift=0; (currMask & 1); currMask >>= 1, gPalShift++);
			gPalShift = PALETTEENTRY_BITS - gPalShift;

			currMask = sPixelFormat.dwBBitMask;
			for(bShift=0; !(currMask & 1); currMask >>= 1, bShift++);
			for(bPalShift=0; (currMask & 1); currMask >>= 1, bPalShift++);
			bPalShift = PALETTEENTRY_BITS - bPalShift;
		}
		else
		{
			DBERROR(("Unknown surface format : Palettised with BitCount >= 16 ?!?"));
			goto exit_with_error;
		}
		if (sPixelFormat.dwRGBAlphaBitMask > 0)
		{
			currMask = sPixelFormat.dwRGBAlphaBitMask;
			for(aShift=0; !(currMask & 1); currMask >>= 1, aShift++);
			for(aPalShift=0; (currMask & 1); currMask >>= 1, aPalShift++);
			aPalShift = PALETTEENTRY_BITS - aPalShift;
			/*lint -restore */
		}
		else
		{
			/* No alpha information, so set up the palette shift to loose it */
			aShift = 0;
			aPalShift = PALETTEENTRY_BITS;
		}
	}
	switch (sPixelFormat.dwRGBBitCount)
	{
	case 8:
		pImageSrc = pImageData;
		for(j=0; (j<surfHeight) && (j<height); j++)
		{
			pSurf8Dest = (UBYTE *)(ddsd.lpSurface) + ddsd.lPitch*j;
			for(i=0; (i<surfWidth) && (i<width); i++)
			{
				*(pSurf8Dest++) = *(pImageSrc++);
			}
		}
		break;
	case 16:
		pImageSrc = pImageData;
		if (((surfHeight * 2)  == height) && ((surfHeight * 2)  == height))//half scale
		{
			for(j=0; (j<surfHeight) && (j<height); j++)
			{
				pSurf16Dest = (UWORD *)((UBYTE *)ddsd.lpSurface + ddsd.lPitch*j);
				for(i=0; (i<surfWidth) && (i<width); i++)
				{
					/*lint -save -e644 */
					r = (UWORD)(psPalette[*pImageSrc].peRed >> rPalShift);
					r += (UWORD)(psPalette[pImageSrc[1]].peRed >> rPalShift);
					r += (UWORD)(psPalette[pImageSrc[width]].peRed >> rPalShift);
					r += (UWORD)(psPalette[pImageSrc[width + 1]].peRed >> rPalShift);
					r >>= 2;//average of 4 pixels
					g = (UWORD)(psPalette[*pImageSrc].peGreen >> gPalShift);
					g += (UWORD)(psPalette[pImageSrc[1]].peGreen >> gPalShift);
					g += (UWORD)(psPalette[pImageSrc[width]].peGreen >> gPalShift);
					g += (UWORD)(psPalette[pImageSrc[width + 1]].peGreen >> gPalShift);
					g >>= 2;//average of 4 pixels
					b = (UWORD)(psPalette[*pImageSrc].peBlue >> bPalShift);
					b += (UWORD)(psPalette[pImageSrc[1]].peBlue >> bPalShift);
					b += (UWORD)(psPalette[pImageSrc[width]].peBlue >> bPalShift);
					b += (UWORD)(psPalette[pImageSrc[width + 1]].peBlue >> bPalShift);
					b >>= 2;//average of 4 pixels

					*pSurf16Dest = r << rShift;
					*pSurf16Dest |= g << gShift;
					*pSurf16Dest |= b << bShift;
					/*lint -restore */
					pImageSrc+= 2;//skip a pixel every pixel 
					pSurf16Dest++;
				}
				pImageSrc += width;//skip a line every line 
			}
		}
		else
		{
			for(j=0; (j<surfHeight) && (j<height); j++)
			{
				pSurf16Dest = (UWORD *)((UBYTE *)ddsd.lpSurface + ddsd.lPitch*j);
				for(i=0; (i<surfWidth) && (i<width); i++)
				{
					/*lint -save -e644 */
					*pSurf16Dest = (UWORD)((psPalette[*pImageSrc].peRed >> rPalShift) << rShift);
					*pSurf16Dest |= (UWORD)((psPalette[*pImageSrc].peGreen >> gPalShift) << gShift);
					*pSurf16Dest |= (UWORD)((psPalette[*pImageSrc].peBlue >> bPalShift) << bShift);
					/*lint -restore */
					pImageSrc++;
					pSurf16Dest++;
				}
			}
		}
		break;
	case 24:
		pImageSrc = pImageData;
		for(j=0; (j<surfHeight) && (j<height); j++)
		{
			pSurf32Dest = (UDWORD *)((UBYTE *)ddsd.lpSurface + ddsd.lPitch*j);
			for(i=0; (i<surfWidth) && (i<width); i++)
			{
				/*lint -save -e644 */
				*pSurf32Dest = (UDWORD)((psPalette[*pImageSrc].peRed >> rPalShift) << rShift);
				*pSurf32Dest |= (UDWORD)((psPalette[*pImageSrc].peGreen >> gPalShift) << gShift);
				*pSurf32Dest |= (UDWORD)((psPalette[*pImageSrc].peBlue >> bPalShift) << bShift);
				/*lint -restore */
				pImageSrc++;
				pSurf32Dest = (UDWORD *)(((UBYTE *)pSurf32Dest) + 3);
			}
		}
		break;
	case 32:
		pImageSrc = pImageData;
		for(j=0; (j<surfHeight) && (j<height); j++)
		{
			pSurf32Dest = (UDWORD *)((UBYTE *)ddsd.lpSurface + ddsd.lPitch*j);
			for(i=0; (i<surfWidth) && (i<width); i++)
			{
				/*lint -save -e644 */
				*pSurf32Dest = (UDWORD)((psPalette[*pImageSrc].peRed >> rPalShift) << rShift);
				*pSurf32Dest |= (UDWORD)((psPalette[*pImageSrc].peGreen >> gPalShift) << gShift);
				*pSurf32Dest |= (UDWORD)((psPalette[*pImageSrc].peBlue >> bPalShift) << bShift);
				/*lint -restore */
				pImageSrc++;
				pSurf32Dest++;
			}
		}
		break;
	default:
		DBERROR(("Unsupported surface pixel format"));
		goto exit_with_error;
	}

	ddrval = psSurf->lpVtbl->Unlock(psSurf, NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Unlock failed for texture page load:\n%s",
				DDErrorToString(ddrval)));
		goto exit_with_error;
	}

	return TRUE;

exit_with_error:
	RELEASE(psPal);
	RELEASE(psSurf);

	return FALSE;
}


/* Load a BMP file and create a system memory surface to store it in.
 * If pWidth or pHeight is NULL the size of the image will not be returned.
 */
BOOL surfCreateFromBMP(
				STRING				*pFileName,	// The BMP file
				LPDIRECTDRAWSURFACE4	*ppsSurface,// The created surface
				UDWORD				*pWidth,	// The width of the image
				UDWORD				*pHeight)	// The height of the image
{
	UBYTE	*pImageFile, *pImageData;
	UDWORD	imageFileSize, width, height;
	PALETTEENTRY	*psImagePalette;


	if (!loadFile(pFileName, &pImageFile, &imageFileSize))
	{
		return FALSE;
	}

	if (!imageParseBMP(pImageFile, imageFileSize,
					   &width, &height, &pImageData, &psImagePalette))
	{
		FREE(pImageFile);
		return FALSE;
	}

	FREE(pImageFile);

	if (!surfCreate(ppsSurface, width,height,
					DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
					screenGetBackBufferPixelFormat(), FALSE, TRUE))
	{
		FREE(pImageData);
		if (psImagePalette)
		{
			FREE(psImagePalette);
		}
		return FALSE;
	}

	if (psImagePalette)
	{
		if (!surfLoadFrom8Bit(*ppsSurface, width,height, pImageData, psImagePalette))
		{
			FREE(pImageData);
			FREE(psImagePalette);
			return FALSE;
		}
		FREE(pImageData);
		FREE(psImagePalette);
	}
	else
	{
		DBERROR(("Surface loading from true colour images not implemented"));
		FREE(pImageData);
		return FALSE;
	}

	/* Return the size of the image if it is needed */
	if (pWidth && pHeight)
	{
		*pWidth = width;
		*pHeight = height;
	}

	return TRUE;
}

