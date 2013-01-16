/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

	$Revision$
	$Id$
	$HeadURL$
*/

//#include "stdafx.h"
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "typedefs.h"
#include "debugprint.hpp"

#include "directx.h"

#include "macros.h"
#include "ddimage.h"



DDImage::DDImage(DWORD SourceX,DWORD SourceY,DWORD Width,DWORD Height,
			CDirectDraw *DirectDrawView,IDDSURFACE *Surface)
{
	m_DirectDrawView=DirectDrawView;
	m_Surface=Surface;
	m_SourceX=SourceX;
	m_SourceY=SourceY;
	m_Width=Width;
	m_Height=Height;
}

// Blit image from DirectDraw surface to DirectDraw display surface.
//
BOOL DDImage::DDBlitImage(IDDSURFACE *DestSurface,SLONG x,SLONG y,DWORD Flags)
{
	HRESULT	ddrval;
    DDBLTFX     ddbltfx;
    RECT    src;
    RECT    dst;
	SLONG XRes,YRes;

    memset( &ddbltfx, 0, sizeof( ddbltfx ) );
    ddbltfx.dwSize = sizeof( ddbltfx );
	m_Surface->GetColorKey(DDCKEY_SRCBLT,&ddbltfx.ddckSrcColorkey);

	setRectWH(&src,m_SourceX,m_SourceY,m_Width,m_Height);
	setRectWH(&dst,x,y,m_Width,m_Height);

	m_DirectDrawView->GetRes((DWORD*)&XRes,(DWORD*)&YRes);

	if(x < 0) {
		src.left-=x;
		dst.left=0;
	}

	if(x+m_Width > XRes) {
		src.right-=dst.right-XRes;
		dst.right=XRes;
	}
	
	if(src.right<=src.left) {
		return TRUE;
	}

	if(y < 0) {
		src.top-=y;
		dst.top=0;
	}

	if(y+m_Height > YRes) {
		src.bottom-=dst.bottom-YRes;
		dst.bottom=YRes;
	}
	
	if(src.bottom<=src.top) {
		return TRUE;
	}

	do {
		ddrval = DestSurface->Blt(&dst, m_Surface, &src, Flags | DDBLT_ASYNC, &ddbltfx);
		if(ddrval==DDERR_WASSTILLDRAWING) {
			ddrval = DestSurface->Blt(&dst, m_Surface, &src, Flags | DDBLT_WAIT, &ddbltfx);
		}
		if( ddrval == DDERR_SURFACELOST ) {
			m_DirectDrawView->RestoreSurfaces();
			DXCALL(ddrval=m_Surface->Restore());
		} else {
			DisplayError(ddrval,__FILE__,__LINE__,0);
			return FALSE;
		}
	} while(ddrval==DDERR_SURFACELOST);

	return TRUE;
}


// Blit image from DirectDraw surface to Windows DIB section.
//
void DDImage::DDBlitImageDIB(CDIBDraw *DIBDraw,SLONG XPos,SLONG YPos,DWORD Flags)
{
//	Flags = 0;
//	Flags |= SF_TEXTUREFLIPX;
//	Flags |= SF_TEXTUREFLIPY;
//	Flags |= SF_ROT90;

#if(1)
	DDBlitFRImageDIB(DIBDraw,XPos,YPos,Flags);
#else
	if((Flags&SF_TEXTUREFLIPX) && (Flags&SF_TEXTUREFLIPY)) {
		DDBlitImageDIBFlipXY(DIBDraw,XPos,YPos);
		return;
	}
	if(Flags&SF_TEXTUREFLIPX) {
		DDBlitImageDIBFlipX(DIBDraw,XPos,YPos);
		return;
	}
	if(Flags&SF_TEXTUREFLIPY) {
		DDBlitImageDIBFlipY(DIBDraw,XPos,YPos);
		return;
	}
	if(Flags&SF_TEXTUREROTMASK) {
		switch(Flags&SF_TEXTUREROTMASK) {
			case	SF_ROT90:
				DDBlitImageDIBRot90(DIBDraw,XPos,YPos);
				break;

			case	SF_ROT180:
				break;

			case	SF_ROT270:
				break;
		}
		return;
	}

    RECT    src;
    RECT    dst;

	setRectWH(&src,m_SourceX,m_SourceY,m_Width,m_Height);
	setRectWH(&dst,x,y,m_Width,m_Height);

	if(ClipRect(DIBDraw,XPos,YPos,src,dst) & CLIP_GONE) {
		return;
	}

	HDC hdc=m_DirectDrawView->DDGetSurfaceDC(m_Surface);

	BitBlt(DIBDraw->GetDIBDC(), dst.left, 
								dst.top, 
								dst.right-dst.left, 
								dst.bottom-dst.top,
							    hdc , src.left, src.top, SRCCOPY);


	m_DirectDrawView->DDReleaseSurfaceDC(m_Surface,hdc);
#endif
}


UWORD DDImage::GetRepColour(void)
{
	UWORD Data;

// Lock the source surface.
    SURFACEDESC ddsd;
    HRESULT ddrval;
    ddsd.dwSize = sizeof(ddsd);
    while ((ddrval = m_Surface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING);
	if(ddrval != DD_OK) {
		DisplayError(ddrval,__FILE__,__LINE__,0);
	}

	Data = *( ((UWORD*)((UBYTE*)ddsd.lpSurface+ddsd.lPitch * m_SourceY)) + m_SourceX);

	m_Surface->Unlock(NULL);

	return Data;
}


// Blit an image from a direct draw surface to a windows DIB section.
// Allows cumulative xy flipping and 90 degree rotations.
// Base on John's code for drawing 2d tiles.
// Dos'nt do clipping, if an image overlaps the DIB's edge then it is thrown out.
//
void DDImage::DDBlitFRImageDIB(CDIBDraw *DIBDraw,int XPos,int YPos,DWORD Flags)
{
	int XRes = DIBDraw->GetDIBWidth();
	int YRes = DIBDraw->GetDIBHeight();

	if(XPos < 0) {
		return;
	}

	if(XPos + m_Width > XRes) {
		return;
	}
	
	if(YPos < 0) {
		return;
	}

	if(YPos + m_Height > YRes) {
		return;
	}

	RECT psDestRect;
	RECT psSrcRect;
	
	psDestRect.left = XPos;
	psDestRect.top = YPos;
	psDestRect.right = XPos + m_Width;
	psDestRect.bottom = YPos + m_Height;

	psSrcRect.left = m_SourceX;
	psSrcRect.top = m_SourceY;
	psSrcRect.right = m_SourceX + m_Width;
	psSrcRect.bottom = m_SourceY + m_Height;

	POINT sP1,sP2,sP3,sP4;
	POINT *psP1,*psP2,*psP3,*psP4,*psPTemp;
	UWORD *p16Src, *p16Dest;
	int x,y, xDir,yDir, srcInc,destInc;
	int SourcePitch,DestPitch;

	/* Store the source rect as four points */
	sP1.x = psSrcRect.left;
	sP1.y = psSrcRect.top;
	sP2.x = psSrcRect.right;
	sP2.y = psSrcRect.top;
	sP3.x = psSrcRect.right;
	sP3.y = psSrcRect.bottom;
	sP4.x = psSrcRect.left;
	sP4.y = psSrcRect.bottom;

	/* Store pointers to the points */
	psP1 = &sP1;
	psP2 = &sP2;
	psP3 = &sP3;
	psP4 = &sP4;

	/* Now flip and rotate the source rectangle using the point pointers */
	if (Flags & SF_TEXTUREFLIPX)
	{
		psPTemp = psP1;
		psP1 = psP2;
		psP2 = psPTemp;
		psPTemp = psP3;
		psP3 = psP4;
		psP4 = psPTemp;
	}
	if (Flags & SF_TEXTUREFLIPY)
	{
		psPTemp = psP1;
		psP1 = psP4;
		psP4 = psPTemp;
		psPTemp = psP2;
		psP2 = psP3;
		psP3 = psPTemp;
	}

	switch (Flags & SF_TEXTUREROTMASK)	{
	case SF_TEXTUREROT90:
		psPTemp = psP1;
		psP1 = psP4;
		psP4 = psP3;
		psP3 = psP2;
		psP2 = psPTemp;
		break;
	case SF_TEXTUREROT180:
		psPTemp = psP1;
		psP1 = psP3;
		psP3 = psPTemp;
		psPTemp = psP4;
		psP4 = psP2;
		psP2 = psPTemp;
		break;
	case SF_TEXTUREROT270:
		psPTemp = psP1;
		psP1 = psP2;
		psP2 = psP3;
		psP3 = psP4;
		psP4 = psPTemp;
		break;
	}

// Lock the source surface.
    SURFACEDESC ddsd;
    HRESULT ddrval;
    ddsd.dwSize = sizeof(ddsd);
    while ((ddrval = m_Surface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING);
	if(ddrval != DD_OK) {
		DisplayError(ddrval,__FILE__,__LINE__,0);
	}

	SourcePitch = ddsd.lPitch/2;
	DestPitch = DIBDraw->GetDIBWidth();

	/* See if P1 -> P2 is horizontal or vertical */
	if (psP1->y == psP2->y)
	{
		/* P1 -> P2 is horizontal */
		xDir = (psP1->x > psP2->x) ? -1 : 1;
		yDir = (psP1->y > psP4->y) ? -1 : 1;

		p16Dest = (UWORD*)DIBDraw->GetDIBBits();
		p16Dest += psDestRect.left + (psDestRect.top*DestPitch);
		destInc = DestPitch - (psDestRect.right - psDestRect.left);

		p16Src = (UWORD*)ddsd.lpSurface;
		p16Src += SourcePitch*psP1->y + psP1->x;
		
		if (psP1->y < psP4->y)
		{
			srcInc = SourcePitch - (psP2->x - psP1->x);
		}
		else
		{
			srcInc = -SourcePitch - (psP2->x - psP1->x);
		}
		/* Have to adjust start point if xDir or yDir are negative */
		if (xDir < 0)
		{
			p16Src--;
		}
		if (yDir < 0)
		{
			p16Src = p16Src - SourcePitch;
		}
		for(y = psP1->y; y != psP4->y; y += yDir)
		{
			for(x = psP1->x; x != psP2->x; x += xDir)
			{
				*p16Dest++ = *p16Src;
				p16Src += xDir;
			}
			p16Dest = p16Dest + destInc;
			p16Src = p16Src + srcInc;
		}
	}
	else
	{
		/* P1 -> P2 is vertical */
		xDir = (psP1->x > psP4->x) ? -1 : 1;
		yDir = (psP1->y > psP2->y) ? -1 : 1;

		p16Dest = (UWORD*)DIBDraw->GetDIBBits();
		p16Dest += psDestRect.left + (psDestRect.top*DestPitch);
		destInc = DestPitch - (psDestRect.right - psDestRect.left);

		for(x = psP1->x; x != psP4->x; x += xDir)
		{
			p16Src = (UWORD*)ddsd.lpSurface;
			p16Src += SourcePitch*psP1->y + x;

			if (xDir < 0)
			{
				p16Src --;
			}
			if (yDir < 0)
			{
				p16Src = p16Src - SourcePitch;
			}
			for(y = psP1->y; y != psP2->y; y += yDir)
			{
				*p16Dest++ = *p16Src;
				p16Src = p16Src + SourcePitch*yDir;
			}
			p16Dest = p16Dest + destInc;
		}
	}

	m_Surface->Unlock(NULL);
}


/*
void DDImage::DDBlitFRImageDIB(CDIBDraw *DIBDraw,int x,int y,DWORD Flags)
{
	POINT	p0,p1,p2,p3;	// Source rectangle.
	POINT	Tmp;

	p0.x = m_SourceX;			p0.y = m_SourceY;
	p1.x = m_SourceX+m_Width;	p1.y = m_SourceY;
	p2.x = m_SourceX+m_Width;	p2.y = m_SourceY+m_Height;
	p3.x = m_SourceX;			p3.y = m_SourceY+m_Height;

// If x flipped then swap p0,p1 and p2,p3 
	float Tmp;
	if(Tile->Flags & SF_TEXTUREFLIPX) {
		Tmp = p0;
		p0 = p1;
		p1 = Tmp;
		Tmp = p2;
		p2 = p3;
		p3 = Tmp;
	}

// If x flipped then swap p0,p3 and p1,p2 
	if(Tile->Flags & SF_TEXTUREFLIPY) {
		Tmp = p0;
		p0 = p3;
		p3 = Tmp;
		Tmp = p1;
		p1 = p2;
		p2 = Tmp;
	}

// Now handle any rotation.
	switch(Tile->Flags & SF_TEXTUREROTMASK) {
   		case SF_TEXTUREROT90:
   			Tmp = p0;
   			p0 = p3;
   			p3 = p2;
   			p2 = p1;
   			p1 = Tmp;
   			break;

   		case SF_TEXTUREROT180:
   			Tmp = p0;
   			p0 = p2;
   			p2 = Tmp;
   			Tmp = p1;
   			p1 = p3;
   			p3 = Tmp;
   			break;

   		case SF_TEXTUREROT270:
   			Tmp = p0;
   			p0 = p1;
   			p1 = p2;
   			p2 = p3;
   			p3 = Tmp;
   			break;
   }
}
*/


void DDImage::DDBlitImageDIBFlipX(CDIBDraw *DIBDraw,SLONG x,SLONG y)
{
    RECT    src;
    RECT    dst;
	DWORD ClipFlags;

	setRectWH(&src,m_SourceX,m_SourceY,m_Width,m_Height);
	setRectWH(&dst,x,y,m_Width,m_Height);

	RECT pcSrc = src;		// Pre-clipped source rectangle.

	if( (ClipFlags = ClipRect(DIBDraw,x,y,src,dst)) & CLIP_GONE) {
		return;
	}

// Since were X flipped we need to adjust the clipped source rect.
	if(ClipFlags & CLIP_LEFT) {
		src.left = pcSrc.left;
	}
	if(ClipFlags & CLIP_RIGHT) {
		src.left += m_Width-(src.right-pcSrc.left);
	}

	int Width = dst.right-dst.left;
	int Height = dst.bottom-dst.top;

    SURFACEDESC ddsd;
    HRESULT ddrval;

    ddsd.dwSize = sizeof(ddsd);
    while ((ddrval = m_Surface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING);
	if(ddrval != DD_OK) {
		DisplayError(ddrval,__FILE__,__LINE__,0);
	}

	m_Surface->Unlock(NULL);

	ULONG SourcePitch = ddsd.lPitch/2;
	ULONG DestPitch = DIBDraw->GetDIBWidth();

	UWORD *Dest = (UWORD*)DIBDraw->GetDIBBits();
	Dest += dst.left + (dst.top*DestPitch) + (Width-1);
	UWORD *Source = (UWORD*)ddsd.lpSurface;
	Source += src.left + (src.top*SourcePitch);

	for(int iy=0; iy<Height; iy++) {
		for(int ix=0; ix<Width; ix++){
			*(Dest-ix) = *(Source+ix);
		}

		Source += SourcePitch;
		Dest += DestPitch;
	}
}


void DDImage::DDBlitImageDIBFlipY(CDIBDraw *DIBDraw,SLONG x,SLONG y)
{
    RECT    src;
    RECT    dst;
	DWORD ClipFlags;

	setRectWH(&src,m_SourceX,m_SourceY,m_Width,m_Height);
	setRectWH(&dst,x,y,m_Width,m_Height);

	RECT pcSrc = src;		// Pre-clipped source rectangle.

	if( (ClipFlags = ClipRect(DIBDraw,x,y,src,dst)) & CLIP_GONE) {
		return;
	}

// Since were Y flipped we need to adjust the clipped source rect.
	if(ClipFlags & CLIP_TOP) {
		src.top = pcSrc.top;
	}
	if(ClipFlags & CLIP_BOTTOM) {
		src.top += m_Height-(src.bottom-pcSrc.top);
	}

	int Width = dst.right-dst.left;
	int Height = dst.bottom-dst.top;

    SURFACEDESC ddsd;
    HRESULT ddrval;

    ddsd.dwSize = sizeof(ddsd);
    while ((ddrval = m_Surface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING);
	if(ddrval != DD_OK) {
		DisplayError(ddrval,__FILE__,__LINE__,0);
	}

	m_Surface->Unlock(NULL);

	ULONG SourcePitch = ddsd.lPitch/2;
	ULONG DestPitch = DIBDraw->GetDIBWidth();

	UWORD *Dest = (UWORD*)DIBDraw->GetDIBBits();
	Dest += dst.left + ((dst.top + (Height-1))*DestPitch);
	UWORD *Source = (UWORD*)ddsd.lpSurface;
	Source += src.left + (src.top*SourcePitch);

	for(int iy=0; iy<Height; iy++) {
		for(int ix=0; ix<Width; ix++){
			*(Dest+ix) = *(Source+ix);
		}

		Source += SourcePitch;
		Dest -= DestPitch;
	}
}

void DDImage::DDBlitImageDIBFlipXY(CDIBDraw *DIBDraw,SLONG x,SLONG y)
{
    RECT    src;
    RECT    dst;
	DWORD ClipFlags;

	setRectWH(&src,m_SourceX,m_SourceY,m_Width,m_Height);
	setRectWH(&dst,x,y,m_Width,m_Height);

	RECT pcSrc = src;		// Pre-clipped source rectangle.

	if( (ClipFlags = ClipRect(DIBDraw,x,y,src,dst)) & CLIP_GONE) {
		return;
	}

// Since were X flipped we need to adjust the clipped source rect.
	if(ClipFlags & CLIP_LEFT) {
		src.left = pcSrc.left;
	}
	if(ClipFlags & CLIP_RIGHT) {
		src.left += m_Width-(src.right-pcSrc.left);
	}
// Since were Y flipped we need to adjust the clipped source rect.
	if(ClipFlags & CLIP_TOP) {
		src.top = pcSrc.top;
	}
	if(ClipFlags & CLIP_BOTTOM) {
		src.top += m_Height-(src.bottom-pcSrc.top);
	}

	int Width = dst.right-dst.left;
	int Height = dst.bottom-dst.top;

    SURFACEDESC ddsd;
    HRESULT ddrval;

    ddsd.dwSize = sizeof(ddsd);
    while ((ddrval = m_Surface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING);
	if(ddrval != DD_OK) {
		DisplayError(ddrval,__FILE__,__LINE__,0);
	}

	m_Surface->Unlock(NULL);

	ULONG SourcePitch = ddsd.lPitch/2;
	ULONG DestPitch = DIBDraw->GetDIBWidth();

	UWORD *Dest = (UWORD*)DIBDraw->GetDIBBits();
	Dest += dst.left + ((dst.top + (Height-1))*DestPitch) + (Width-1);
	UWORD *Source = (UWORD*)ddsd.lpSurface;
	Source += src.left + (src.top*SourcePitch);

	for(int iy=0; iy<Height; iy++) {
		for(int ix=0; ix<Width; ix++){
			*(Dest-ix) = *(Source+ix);
		}

		Source += SourcePitch;
		Dest -= DestPitch;
	}
}


void DDImage::DDBlitImageDIBRot90(CDIBDraw *DIBDraw,SLONG x,SLONG y)
{
    RECT    src;
    RECT    dst;
	DWORD ClipFlags;

// Can only rotate square images.
	ASSERT(m_Width == m_Height);

	setRectWH(&src,m_SourceX,m_SourceY,m_Width,m_Height);
	setRectWH(&dst,x,y,m_Width,m_Height);

	if( (ClipFlags = ClipRect(DIBDraw,x,y,src,dst)) & CLIP_GONE) {
		return;
	}

	int Width = dst.right-dst.left;
	int Height = dst.bottom-dst.top;

//	if(Width!=Height) return;
	if(Width > Height) return;

    SURFACEDESC ddsd;
    HRESULT ddrval;

    ddsd.dwSize = sizeof(ddsd);
    while ((ddrval = m_Surface->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING);
	if(ddrval != DD_OK) {
		DisplayError(ddrval,__FILE__,__LINE__,0);
	}

	m_Surface->Unlock(NULL);

	ULONG SourcePitch = ddsd.lPitch/2;
	ULONG DestPitch = DIBDraw->GetDIBWidth();

	UWORD *Dest = (UWORD*)DIBDraw->GetDIBBits();
	Dest += dst.left + (dst.top*DestPitch) + (Width-1);
	UWORD *Source = (UWORD*)ddsd.lpSurface;
	Source += src.left + (src.top*SourcePitch);

	for(int iy=0; iy<Height; iy++) {
		for(int ix=0; ix<Width; ix++) {
			*(Dest+ix*DestPitch) = *(Source+ix);
		}

		Source += SourcePitch;
		Dest--;
	}
}


DWORD DDImage::ClipRect(CDIBDraw *DIBDraw,SLONG x,SLONG y,RECT &src,RECT &dst)
{
	SLONG XRes = DIBDraw->GetDIBWidth();
	SLONG YRes = DIBDraw->GetDIBHeight();
	DWORD ClipFlags = 0;

	if(x < 0) {
		src.left-=x;
		dst.left=0;
		ClipFlags |= CLIP_LEFT;
	}

	if(x+m_Width > XRes) {
		src.right-=dst.right-XRes;
		dst.right=XRes;
		ClipFlags |= CLIP_RIGHT;
	}
	
	if(src.right<=src.left) {
		ClipFlags |= CLIP_GONE;
		return ClipFlags;
	}

	if(y < 0) {
		src.top-=y;
		dst.top=0;
		ClipFlags |= CLIP_TOP;
	}

	if(y+m_Height > YRes) {
		src.bottom-=dst.bottom-YRes;
		dst.bottom=YRes;
		ClipFlags |= CLIP_BOTTOM;
	}
	
	if(src.bottom<=src.top) {
		ClipFlags |= CLIP_GONE;
		return ClipFlags;
	}

	return ClipFlags;
}


DDSprite::DDSprite(SWORD CenterX,SWORD CenterY,
			DWORD SourceX,DWORD SourceY,
			DWORD Width,DWORD Height,
			CDirectDraw *DirectDrawView,IDDSURFACE *Surface):DDImage(SourceX,SourceY,
			Width,Height,
			DirectDrawView,Surface)

{
	m_CenterX=CenterX;
	m_CenterY=CenterY;
}

void DDSprite::DDBlitSprite(IDDSURFACE *DestSurface,SLONG x,SLONG y,DWORD Flags)
{
	DDBlitImage(DestSurface,x-m_CenterX,y-m_CenterY,Flags);
}
