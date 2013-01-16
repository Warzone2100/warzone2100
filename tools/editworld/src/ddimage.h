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

#ifndef __INCLUDED_DDIMAGE_H__
#define __INCLUDED_DDIMAGE_H__

#include "dibdraw.h"

#define	BLT_NORMAL	0
#define	BLT_USEKEY	DDBLT_KEYSRCOVERRIDE

/*
 Sprite transformation flags.

 bbbbbbbb
 76543210
 ..rryxfh

  rr	Texture rotation 0=0, 1=90, 2=180, 3=270.
  y		Y texture flip.
  x     X texture flip.
  f     Diagonal flip.
  h     Hide.
*/

#define	SF_HIDE			1
#define SF_VERTEXFLIP	2
#define SF_TEXTUREFLIPX	4
#define SF_TEXTUREFLIPY	8
#define SF_TEXTUREROTMASK	0x30
#define SF_TEXTUREROTSHIFT	4
#define SF_TEXTUREROT90		(1<<SF_TEXTUREROTSHIFT)
#define SF_TEXTUREROT180	(2<<SF_TEXTUREROTSHIFT)
#define	SF_TEXTUREROT270	(3<<SF_TEXTUREROTSHIFT)
#define SF_ROT90	(1<<SF_TEXTUREROTSHIFT)
#define SF_ROT180	(2<<SF_TEXTUREROTSHIFT)
#define	SF_ROT270	(3<<SF_TEXTUREROTSHIFT)

#define SF_RANDTEXTUREFLIPX	128
#define SF_RANDTEXTUREFLIPY 256
#define SF_RANDTEXTUREROTATE 512
#define SF_INCTEXTUREROTATE 1024
#define SF_TOGTEXTUREFLIPX 2048
#define SF_TOGTEXTUREFLIPY 4096


// Clipping result flags.

#define CLIP_LEFT	1
#define CLIP_RIGHT	2
#define CLIP_TOP	4
#define CLIP_BOTTOM 8
#define CLIP_GONE	16


class DDImage {
public:
	DDImage(DWORD SourceX,DWORD SourceY,DWORD Width,DWORD Height,
			CDirectDraw *DirectDrawView,IDDSURFACE *Surface);
	BOOL DDBlitImage(IDDSURFACE *DestSurface,SLONG x,SLONG y,DWORD Flags=BLT_NORMAL);
	void DDBlitFRImageDIB(CDIBDraw *DIBDraw,int x,int y,DWORD Flags);
	void DDBlitImageDIB(CDIBDraw *DIBDraw,SLONG x,SLONG y,DWORD Flags=0);
	void DDBlitImageDIBFlipX(CDIBDraw *DIBDraw,SLONG x,SLONG y);
	void DDBlitImageDIBFlipY(CDIBDraw *DIBDraw,SLONG x,SLONG y);
	void DDBlitImageDIBFlipXY(CDIBDraw *DIBDraw,SLONG x,SLONG y);
	void DDBlitImageDIBRot90(CDIBDraw *DIBDraw,SLONG x,SLONG y);
	DWORD ClipRect(CDIBDraw *DIBDraw,SLONG x,SLONG y,RECT &src,RECT &dst);
	IDDSURFACE *DDGetImageSurface(void) { return(m_Surface); }
	UWORD GetRepColour(void);
protected:
	CDirectDraw* m_DirectDrawView;
	IDDSURFACE *m_Surface;
	DWORD m_SourceX;
	DWORD m_SourceY;
	int m_Width;
	int m_Height;
};


class DDSprite : public DDImage {
public:
	DDSprite(SWORD CenterX,SWORD CenterY,
			DWORD SourceX,DWORD SourceY,
			DWORD Width,DWORD Height,
			CDirectDraw *DirectDrawView,IDDSURFACE *Surface);
	void DDBlitSprite(IDDSURFACE *DestSurface,SLONG x,SLONG y,DWORD Flags=BLT_NORMAL);
protected:
	SWORD m_CenterX;
	SWORD m_CenterY;
};

#endif // __INCLUDED_DDIMAGE_H__
