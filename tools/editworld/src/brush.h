/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#ifndef __INCLUDED_BRUSH_H__
#define __INCLUDED_BRUSH_H__

#include "dibdraw.h"
#include "chnkio.h"

enum {
	EB_DECREMENT,
	EB_INCREMENT,
	EB_SMALLBRUSH,
	EB_LARGEBRUSH,
};

// Valid values for m_HeightMode.
enum {
	EB_HM_NOSET,	// Don't set heights
	EB_HM_SETABB,	// Set absolute height
	EB_HM_SEALEVEL,	// Use sea level
};


struct Brush {
	int ox,oy;
	DWORD Tid;			// Texture ID.
	DWORD TFlags;		// Texture transformation flags.
};

struct BrushMapEntry {
	int Flags;
	DWORD TFlags;		// Texture transformation flags.
	DWORD XCoord,YCoord;
	int Link;
	int Tid;			// Texture ID.
};

struct EdgeTableEntry {
	SLONG Tid;
	DWORD TFlags;
};

class CEdgeBrush : public CChnkIO
{
	public:
		CEdgeBrush(CHeightMap* HeightMap,DWORD TextureWidth,DWORD TextureHeight,DWORD NumImages);
		~CEdgeBrush();

		inline void SetTextureSize(DWORD TextureWidth,DWORD TextureHeight)
		{
			m_TextureWidth = TextureWidth;
			m_TextureHeight = TextureHeight;
		}

		void SetBrushTexture(DWORD TableIndex,int Tid);
		void SetBrushTextureFromMap(void);
		int GetBrushTexture(DWORD TableIndex);
		BOOL Paint(DWORD XCoord,DWORD YCoord,BOOL SetHeight);
		void DrawEdgeBrushIcon(CDIBDraw *DIBDraw,DDImage **Images,int x, int y);
		void DrawEdgeBrush(CDIBDraw *DIBDraw,DDImage **Images,int ScrollX, int ScrollY);
		void RotateTile(int XCoord,int YCoord,int ScrollX,int ScrollY);
		void XFlipTile(int XCoord,int YCoord,int ScrollX,int ScrollY);
		void YFlipTile(int XCoord,int YCoord,int ScrollX,int ScrollY);
		void TriFlipTile(int XCoord,int YCoord,int ScrollX,int ScrollY);
		void SetBrushMap(int Tid,DWORD TFlags,int XCoord,int YCoord,int ScrollX,int ScrollY);

		inline DWORD GetNumImages()
		{
			return m_NumImages;
		}

		inline void SetNumImages(DWORD NumImages)
		{
			m_NumImages = NumImages;
		}

		void DrawButtons(CDC *dc);
		int DoButtons(int XCoord,int YCoord);
		int GetButtonState(int Index);
		void SetButtonState(int Index,int State);
		void SetBrushDirect(int Index,int HeightMode,int Height,int RandomRange,int Tid,int Flags);

		inline void SetLargeBrush(BOOL LargeBrush)
		{
			m_BrushSize = LargeBrush;
		}

		inline BOOL GetLargeBrush()
		{
			return m_BrushSize;
		}

		BOOL Write(FILE* Stream);
		BOOL ReadV1(FILE* Stream);
		BOOL ReadV2(FILE* Stream);

		void FillMap(DWORD Selected,DWORD TextureID,DWORD Type,DWORD Flags);
		Pixel PixelRead(int x,int y);
		BOOL IsBrushEdge(Pixel ct);
		BOOL PixelCompare(int x,int y,Pixel ov);
		void PixelWrite(int x,int y,Pixel nv,DWORD Type,DWORD Flags);
		void SeedFill(int x, int y, FRect *win, Pixel nv,DWORD Type,DWORD Flags);

	public:
		int _HeightMode;
		int _Height;
		int _RandomRange;

	private:
		CHeightMap* m_HeightMap;
		EdgeTableEntry m_EdgeTable[16];
		int m_EdgeFlags[16];
		Brush m_EdgeBrush[13];
		BrushMapEntry m_BrushMap[29];
		DWORD m_TextureWidth;
		DWORD m_TextureHeight;
		BOOL m_IsWater;

		static DWORD m_NumImages;
		static DWORD m_NumInstances;
		static BOOL m_BrushSize;

		DWORD m_ID;

		DWORD ConvertTileIDToCode(DWORD Tid,DWORD TFlags);
		void ConvertTileCodeToID(DWORD Code,DWORD &Tid,DWORD &TFlags);
};

#endif // __INCLUDED_BRUSH_H__
