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

#include "stdafx.h"
#include "btedit.h"
#include "bteditdoc.h"
#include "brush.h"
#include "tiletypes.h"
#include "debugprint.hpp"
#include "assert.h"
#include "undoredo.h"

extern DWORD g_Flags[MAXTILES];
extern CUndoRedo *g_UndoRedo;

#define MAX 10000		/* max depth of stack */

#define PUSH(Y, XL, XR, DY)	/* push new segment on stack */ \
    if (sp<stack+MAX && Y+(DY)>=win->y0 && Y+(DY)<=win->y1) \
    {sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}

#define POP(Y, XL, XR, DY)	/* pop segment off stack */ \
    {sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}


DWORD CEdgeBrush::m_NumImages = 0;
DWORD CEdgeBrush::m_NumInstances = 0;
BOOL CEdgeBrush::m_BrushSize = TRUE;

//int CEdgeBrush::m_SmallBrush[5]={0,2,6,8,-1};
//int CEdgeBrush::m_LargeBrush[10]={0,1,2,3,4,5,6,7,8,-1};

CEdgeBrush::CEdgeBrush(CHeightMap* HeightMap,DWORD TextureWidth,DWORD TextureHeight,DWORD NumImages)
{
	Brush DefaultEdgeBrush[13] = {
		{-1,-1,52}, { 0,-1,53}, { 1,-1,54},
		{-1, 0,55}, { 0, 0,64}, { 1, 0,56},
		{-1, 1,57}, { 0, 1,58}, { 1, 1,59},
		{0,0,52},	{1,0,54},
		{0,1,57},	{1,1,59}
	};

	BrushMapEntry DefaultBrushMap[]={
		{ 0,0,	0,0,	-1, -1},	// 0 0000
		{ 0,0,	1,1,	-1, 52},	// 1 0001 
		{ 0,0,	3,1,	-1, 54},	// 2 0010 
		{ 0,0,	2,1,	-1, 53},	// 3 0011 
		{ 0,0,	1,3,	-1, 57},	// 4 0100 
		{ 0,0,	1,2,	-1, 55},	// 5 0101 
		{ 0,0,	3,6,	-1, -1},	// 6 0110 
		{ 0,0,	8,4,	-1, 63},	// 7 0111 
		{ 0,0,	3,3,	-1, 59},	// 8 1000 
		{ 0,0,	1,6,	-1, -1},	// 9 1001 
		{ 0,0,	3,2,	-1, 56},	// 10 1010
		{ 0,0,	5,4,	-1, 62},	// 11 1011
		{ 0,0,	2,3,	-1, 58},	// 12 1100
		{ 0,0,	8,1,	-1, 61},	// 13 1101
		{ 0,0,	5,1,	-1, 60},	// 14 1110
		{ 0,0,	2,2,	-1,	64},	// 15 1111
		{ 0,0,	6,1,	12, -1},	// 16
		{ 0,0,	7,1,	12, -1},	// 17
		{ 0,0,	5,2,	10, -1},	// 18
		{ 0,0,	5,3,	10, -1},	// 19
		{ 0,0,	8,2,	5, -1},		// 20
		{ 0,0,	8,3,	5, -1},		// 21
		{ 0,0,	6,4,	3, -1},		// 22
		{ 0,0,	7,4,	3, -1},		// 23
		{ 0,0,	1,5,	10, -1},	// 24
		{ 0,0,	1,7,	5, -1},		// 25
		{ 0,0,	3,5,	5, -1},		// 26
		{ 0,0,	3,7,	10, -1},	// 27
		{-1,0,	0,0,	0,	0}		// 28
	};

	m_HeightMap = HeightMap;
	m_TextureWidth = TextureWidth;
	m_TextureHeight = TextureHeight;
	m_NumImages = NumImages;

// Setup the default brush map.
	for(int i=0; i<29; i++) {
		m_BrushMap[i] = DefaultBrushMap[i];
		if(m_NumInstances != 0) {
			m_BrushMap[i].Tid = -1;
		}
	}

	for(i=0; i<13; i++) {
		m_EdgeBrush[i] = DefaultEdgeBrush[i];
		m_EdgeBrush[i].Tid = 0;
	}

	for(i=0; i<15; i++) {
		m_EdgeTable[i].Tid = 0;
		m_EdgeTable[i].TFlags = 0;
	}

	SetBrushTextureFromMap();


//	m_SetHeight = TRUE;
	_HeightMode = EB_HM_NOSET;
	_Height = 128;
	_RandomRange = 0;

	m_ID = m_NumInstances;

	m_NumInstances++;
}

CEdgeBrush::~CEdgeBrush()
{
	m_NumInstances--;
}

DWORD CEdgeBrush::ConvertTileIDToCode(DWORD Tid,DWORD TFlags)
{
	for(int i=0; i<16; i++) {
		if(( ((DWORD)m_EdgeTable[i].Tid) == Tid ) && (m_EdgeTable[i].TFlags == TFlags) ) {
			return (DWORD)i;
		}
	}

	return 0;
}

void CEdgeBrush::ConvertTileCodeToID(DWORD Code,DWORD &Tid,DWORD &TFlags)
{
	if(m_EdgeTable[Code].Tid < 0) {
		Tid = 65536;
		return;
	}

	Tid = (DWORD)m_EdgeTable[Code].Tid;
	TFlags = m_EdgeTable[Code].TFlags;
}

//void CEdgeBrush::SetBrushTexture(DWORD TableIndex,int Tid)
//{
//	ASSERT(TableIndex < 16);
//
//	m_EdgeTable[TableIndex] = Tid;
//
//	m_EdgeBrush[0].Tid = m_EdgeTable[1];
//	m_EdgeBrush[1].Tid = m_EdgeTable[3];
//	m_EdgeBrush[2].Tid = m_EdgeTable[2];
//	m_EdgeBrush[3].Tid = m_EdgeTable[5];
//	m_EdgeBrush[4].Tid = m_EdgeTable[15];
//	m_EdgeBrush[5].Tid = m_EdgeTable[10];
//	m_EdgeBrush[6].Tid = m_EdgeTable[4];
//	m_EdgeBrush[7].Tid = m_EdgeTable[12];
//	m_EdgeBrush[8].Tid = m_EdgeTable[8];
//
//	m_EdgeBrush[9].Tid = m_EdgeTable[1];
//	m_EdgeBrush[10].Tid = m_EdgeTable[2];
//	m_EdgeBrush[11].Tid = m_EdgeTable[4];
//	m_EdgeBrush[12].Tid = m_EdgeTable[8];
//}


void CEdgeBrush::SetBrushTextureFromMap(void)
{
	m_IsWater = FALSE;

	for(int i=0; i<16; i++) {
		m_EdgeTable[i].Tid = m_BrushMap[i].Tid;
		m_EdgeTable[i].TFlags = m_BrushMap[i].TFlags;
		if(g_Flags[m_EdgeTable[i].Tid] == TF_TYPEWATER) {
			m_IsWater = TRUE; 
		}
	}

	m_EdgeBrush[0].Tid = m_EdgeTable[1].Tid;
	m_EdgeBrush[0].TFlags = m_EdgeTable[1].TFlags;
	m_EdgeBrush[1].Tid = m_EdgeTable[3].Tid;
	m_EdgeBrush[1].TFlags = m_EdgeTable[3].TFlags;
	m_EdgeBrush[2].Tid = m_EdgeTable[2].Tid;
	m_EdgeBrush[2].TFlags = m_EdgeTable[2].TFlags;
	m_EdgeBrush[3].Tid = m_EdgeTable[5].Tid;
	m_EdgeBrush[3].TFlags = m_EdgeTable[5].TFlags;
	m_EdgeBrush[4].Tid = m_EdgeTable[15].Tid;
	m_EdgeBrush[4].TFlags = m_EdgeTable[15].TFlags;
	m_EdgeBrush[5].Tid = m_EdgeTable[10].Tid;
	m_EdgeBrush[5].TFlags = m_EdgeTable[10].TFlags;
	m_EdgeBrush[6].Tid = m_EdgeTable[4].Tid;
	m_EdgeBrush[6].TFlags = m_EdgeTable[4].TFlags;
	m_EdgeBrush[7].Tid = m_EdgeTable[12].Tid;
	m_EdgeBrush[7].TFlags = m_EdgeTable[12].TFlags;
	m_EdgeBrush[8].Tid = m_EdgeTable[8].Tid;
	m_EdgeBrush[8].TFlags = m_EdgeTable[8].TFlags;

	m_EdgeBrush[9].Tid = m_EdgeTable[1].Tid;
	m_EdgeBrush[9].TFlags = m_EdgeTable[1].TFlags;
	m_EdgeBrush[10].Tid = m_EdgeTable[2].Tid;
	m_EdgeBrush[10].TFlags = m_EdgeTable[2].TFlags;
	m_EdgeBrush[11].Tid = m_EdgeTable[4].Tid;
	m_EdgeBrush[11].TFlags = m_EdgeTable[4].TFlags;
	m_EdgeBrush[12].Tid = m_EdgeTable[8].Tid;
	m_EdgeBrush[12].TFlags = m_EdgeTable[8].TFlags;
}


//int CEdgeBrush::GetBrushTexture(DWORD TableIndex)
//{
//	return m_EdgeTable[TableIndex];
//}

// Paint with edge brush, returns TRUE if heights have been altered.
//
BOOL CEdgeBrush::Paint(DWORD XCoord,DWORD YCoord,BOOL SetHeight)
{
	DWORD DstCode;
	int Edge;
	unsigned int mw;
	unsigned int mh;
	BOOL ChangedHeights = FALSE;

	m_HeightMap->GetMapSize(mw, mh);

	int Count,Offset;

	if(m_BrushSize) {
		Count = 9;
		Offset = 0;
	} else {
		Count = 4;
		Offset = 9;
	}

	g_UndoRedo->BeginGroup();

	int BaseHeight;	// = (int)m_HeightMap->GetVertexHeight(XCoord + (YCoord * (int)mw),0);

	if(_RandomRange != 0) {
		BaseHeight = rand()%_RandomRange;
	}


	for(int i=Offset; i<Offset+Count; i++) {
		int ox = m_EdgeBrush[i].ox;
		int oy = m_EdgeBrush[i].oy;
		if((XCoord + ox >= 0) && (XCoord + ox < (int)mw) &&
		  (YCoord + oy >= 0) && (YCoord + oy < (int)mh) ) {
			Edge = XCoord + (YCoord * (int)mw) + ox + (oy * (int)mw);

			DWORD DstTid = m_HeightMap->GetTextureID(Edge);
			DWORD DstFlags = m_HeightMap->GetTileFlags(Edge);
			DWORD DstType;	// = DstFlags & TF_TYPEMASK;
			DstFlags &= 0xfe;	// Only want bits 7-1 which control rotation and flipping.

			DWORD SrcTid = m_EdgeBrush[i].Tid;
			DWORD SrcFlags = m_EdgeBrush[i].TFlags;

			DstCode = ConvertTileIDToCode(SrcTid,SrcFlags) |
						ConvertTileIDToCode(DstTid,DstFlags);

			ConvertTileCodeToID(DstCode,DstTid,DstFlags);

			if(DstTid == 65536) {
				DstTid = SrcTid;
			}

			g_UndoRedo->AddUndo(&(m_HeightMap->GetMapTiles()[Edge]));

//			DebugPrint("%d : %d %d\n",i,ox,oy);

			m_HeightMap->SetTextureID(Edge,DstTid);
			DstType = g_Flags[DstTid];

			int Height;

			switch(_HeightMode) {
				case EB_HM_SEALEVEL:
					// Use sea level
					Height = m_HeightMap->GetSeaLevel();
					break;

				case EB_HM_SETABB:
					// Set absolute height
					Height = _Height;
					break;
//
//				case EB_HM_SETADD:
//					// Set addative height
//					Height0 = BaseHeight0 + _Height;
//					Height1 = BaseHeight1 + _Height;
//					Height2 = BaseHeight2 + _Height;
//					Height3 = BaseHeight3 + _Height;
//					if(Height0 > 255) Height0 = 255;
//					if(Height1 > 255) Height1 = 255;
//					if(Height2 > 255) Height2 = 255;
//					if(Height3 > 255) Height3 = 255;
//
//					break;
			}

			if(_RandomRange != 0) {
				Height = Height + BaseHeight;	//rand()%_RandomRange;
			}

			if(Height > 255) {
				Height = 255;
			}

			if(m_BrushSize) {
				if( (ox == 0) && (oy == 0) && (_HeightMode != EB_HM_NOSET) ) {
//					if(_HeightMode == EB_HM_SETADD) {
//						m_HeightMap->SetVertexHeight(Edge,0,(float)Height0);
//						m_HeightMap->SetVertexHeight(Edge,1,(float)Height1);
//						m_HeightMap->SetVertexHeight(Edge,2,(float)Height2);
//						m_HeightMap->SetVertexHeight(Edge,3,(float)Height3);
//					} else {
						m_HeightMap->SetTileHeightUndo(Edge,(float)Height);
//					}
					ChangedHeights = TRUE;
				}
			} else {
				if(_HeightMode != EB_HM_NOSET) {
					switch(i) {
						case 9:
							m_HeightMap->SetVertexHeight(Edge,2,(float)Height);
							break;
						case 10:
							m_HeightMap->SetVertexHeight(Edge,3,(float)Height);
							break;
						case 11:
							m_HeightMap->SetVertexHeight(Edge,1,(float)Height);
							break;
						case 12:
							m_HeightMap->SetVertexHeight(Edge,0,(float)Height);
							break;
					}
					ChangedHeights = TRUE;
				}
			}

//			if(m_IsWater && SetHeight) {
//				m_HeightMap->SetTileHeightUndo(Edge,(float)m_HeightMap->GetSeaLevel());
//				ChangedHeights = TRUE;
//			}

			m_HeightMap->SetTileFlags(Edge,DstFlags | DstType);
		}
	}

	g_UndoRedo->EndGroup();

	return ChangedHeights;
}

extern HICON g_IconIncrement;
extern HICON g_IconDecrement;
extern HICON g_IconSmallBrush;
extern HICON g_IconSmallBrush2;
extern HICON g_IconLargeBrush;
extern HICON g_IconLargeBrush2;

void CEdgeBrush::DrawEdgeBrushIcon(CDIBDraw *DIBDraw,DDImage **Images,int x, int y)
{
	BrushMapEntry *Current = &m_BrushMap[0];

	Images[m_BrushMap[5].Tid-1]->DDBlitImageDIB(DIBDraw,x,y,m_BrushMap[5].TFlags);
	Current++;
	x+= m_TextureWidth;
	Images[m_BrushMap[15].Tid-1]->DDBlitImageDIB(DIBDraw,x,y,m_BrushMap[15].TFlags);
	Current++;
	x+= m_TextureWidth;
	Images[m_BrushMap[10].Tid-1]->DDBlitImageDIB(DIBDraw,x,y,m_BrushMap[10].TFlags);
}


void CEdgeBrush::DrawEdgeBrush(CDIBDraw *DIBDraw,DDImage **Images,int ScrollX, int ScrollY)
{
	BrushMapEntry *Current = &m_BrushMap[1];
	SLONG	StartX = ScrollX;	///m_TextureWidth;
	SLONG	StartY = ScrollY;	///m_TextureHeight;

//	DebugPrint("%d,%d\n",StartX,StartY);

	while(Current->Flags != -1) {
		SLONG XCoord = (Current->XCoord * m_TextureWidth) - StartX;
		SLONG YCoord = (Current->YCoord * m_TextureHeight) - StartY;

		if(Current->Tid-1 >= 0) {
			if(Current->Tid <= (int)m_NumImages) {
				Images[Current->Tid-1]->DDBlitImageDIB(DIBDraw,XCoord,YCoord,Current->TFlags);
			}
		} else {
			if(Current->Link >= 0) {
				if(m_BrushMap[Current->Link].Tid-1 >= 0) {
					if(m_BrushMap[Current->Link].Tid <= (int)m_NumImages) {
						Images[m_BrushMap[Current->Link].Tid-1]->DDBlitImageDIB(DIBDraw,
								XCoord,YCoord,m_BrushMap[Current->Link].TFlags);
					}
				}
			}
		}

		Current++;
	}

	SLONG	x0,y0,x1,y1;

	CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());
	CPen Pen;
	Pen.CreatePen( PS_SOLID, 2, RGB(255,255,255) );
	CPen* pOldPen = dc->SelectObject( &Pen );

	Current = &m_BrushMap[1];

	while(Current->Flags != -1) {
		SLONG XCoord = (Current->XCoord * m_TextureWidth) - StartX;
		SLONG YCoord = (Current->YCoord * m_TextureHeight) - StartY;

		x0 = XCoord;
		y0 = YCoord;
		x1 = XCoord + m_TextureWidth;
		y1 = YCoord + m_TextureHeight;

 		dc->MoveTo(x0,y0);
   		dc->LineTo(x1,y0);
   		dc->LineTo(x1,y1);
   		dc->LineTo(x0,y1);
   		dc->LineTo(x0,y0);

		if(Current->TFlags & TF_VERTEXFLIP) {
 			dc->MoveTo(x1,y0);
 			dc->LineTo(x0,y1);
		} else {
 			dc->MoveTo(x0,y0);
 			dc->LineTo(x1,y1);
		}

		Current++;
	}

	dc->SelectObject( pOldPen );

	char String[16];
	sprintf(String,"Edge Brush %d",m_ID);
	dc->SetTextColor(RGB(255,255,255));
	dc->SetBkColor(RGB(0,0,0));
	dc->TextOut(16,48,String,strlen(String));

	DrawButtons(dc);
}

void CEdgeBrush::RotateTile(int XCoord,int YCoord,int ScrollX,int ScrollY)
{
	BrushMapEntry *Current = &m_BrushMap[1];
	SLONG	StartX = ScrollX;	///m_TextureWidth;
	SLONG	StartY = ScrollY;	///m_TextureHeight;
	SLONG	x0,y0,x1,y1;

	while(Current->Flags != -1) {
		SLONG x = (Current->XCoord * m_TextureWidth) - StartX;
		SLONG y = (Current->YCoord * m_TextureHeight) - StartY;

		x0 = x;
		y0 = y;
		x1 = x + m_TextureWidth;
		y1 = y + m_TextureHeight;

		if( (XCoord > x0) && (XCoord < x1) && (YCoord > y0) && (YCoord < y1) ) {
			if(Current->Link < 0) {
				DWORD Rotate = (Current->TFlags & TF_TEXTUREROTMASK) >> TF_TEXTUREROTSHIFT;
				Rotate += 1;
				Rotate &= 3;
				Current->TFlags &= !TF_TEXTUREROTMASK;
				Current->TFlags |= Rotate << TF_TEXTUREROTSHIFT;
				SetBrushTextureFromMap();
				return;
			}
		}

		Current++;
	}
}

void CEdgeBrush::XFlipTile(int XCoord,int YCoord,int ScrollX,int ScrollY)
{
	BrushMapEntry *Current = &m_BrushMap[1];
	SLONG	StartX = ScrollX;	///m_TextureWidth;
	SLONG	StartY = ScrollY;	///m_TextureHeight;
	SLONG	x0,y0,x1,y1;

	while(Current->Flags != -1) {
		SLONG x = (Current->XCoord * m_TextureWidth) - StartX;
		SLONG y = (Current->YCoord * m_TextureHeight) - StartY;

		x0 = x;
		y0 = y;
		x1 = x + m_TextureWidth;
		y1 = y + m_TextureHeight;

		if( (XCoord > x0) && (XCoord < x1) && (YCoord > y0) && (YCoord < y1) ) {
			if(Current->Link < 0) {
				DWORD Rotate = (Current->TFlags & TF_TEXTUREROTMASK) >> TF_TEXTUREROTSHIFT;
				if(Rotate & 1) {
					if(Current->TFlags & TF_TEXTUREFLIPY) {
						Current->TFlags &= ~TF_TEXTUREFLIPY;
					} else {
						Current->TFlags |= TF_TEXTUREFLIPY;
					}
				} else {
					if(Current->TFlags & TF_TEXTUREFLIPX) {
						Current->TFlags &= ~TF_TEXTUREFLIPX;
					} else {
						Current->TFlags |= TF_TEXTUREFLIPX;
					}
				}
				SetBrushTextureFromMap();
				return;
			}
		}

		Current++;
	}
}

void CEdgeBrush::YFlipTile(int XCoord,int YCoord,int ScrollX,int ScrollY)
{
	BrushMapEntry *Current = &m_BrushMap[1];
	SLONG	StartX = ScrollX;	///m_TextureWidth;
	SLONG	StartY = ScrollY;	///m_TextureHeight;
	SLONG	x0,y0,x1,y1;

	while(Current->Flags != -1) {
		SLONG x = (Current->XCoord * m_TextureWidth) - StartX;
		SLONG y = (Current->YCoord * m_TextureHeight) - StartY;

		x0 = x;
		y0 = y;
		x1 = x + m_TextureWidth;
		y1 = y + m_TextureHeight;

		if( (XCoord > x0) && (XCoord < x1) && (YCoord > y0) && (YCoord < y1) ) {
			if(Current->Link < 0) {
				DWORD Rotate = (Current->TFlags & TF_TEXTUREROTMASK) >> TF_TEXTUREROTSHIFT;
				if(Rotate & 1) {
					if(Current->TFlags & TF_TEXTUREFLIPX) {
						Current->TFlags &= ~TF_TEXTUREFLIPX;
					} else {
						Current->TFlags |= TF_TEXTUREFLIPX;
					}
				} else {
					if(Current->TFlags & TF_TEXTUREFLIPY) {
						Current->TFlags &= ~TF_TEXTUREFLIPY;
					} else {
						Current->TFlags |= TF_TEXTUREFLIPY;
					}
				}
				SetBrushTextureFromMap();
				return;
			}
		}

		Current++;
	}
}


void CEdgeBrush::TriFlipTile(int XCoord,int YCoord,int ScrollX,int ScrollY)
{
	BrushMapEntry *Current = &m_BrushMap[1];
	SLONG	StartX = ScrollX;	///m_TextureWidth;
	SLONG	StartY = ScrollY;	///m_TextureHeight;
	SLONG	x0,y0,x1,y1;

	while(Current->Flags != -1) {
		SLONG x = (Current->XCoord * m_TextureWidth) - StartX;
		SLONG y = (Current->YCoord * m_TextureHeight) - StartY;

		x0 = x;
		y0 = y;
		x1 = x + m_TextureWidth;
		y1 = y + m_TextureHeight;

		if( (XCoord > x0) && (XCoord < x1) && (YCoord > y0) && (YCoord < y1) ) {
			if(Current->Link < 0) {
				if(Current->TFlags & TF_VERTEXFLIP) {
					Current->TFlags &= ~TF_VERTEXFLIP;
				} else {
					Current->TFlags |= TF_VERTEXFLIP;
				}
				SetBrushTextureFromMap();
				return;
			}
		}

		Current++;
	}
}


void CEdgeBrush::SetBrushMap(int Tid,DWORD TFlags,int XCoord,int YCoord,int ScrollX,int ScrollY)
{
	BrushMapEntry *Current = &m_BrushMap[1];
	SLONG	StartX = ScrollX;	///m_TextureWidth;
	SLONG	StartY = ScrollY;	///m_TextureHeight;
	SLONG	x0,y0,x1,y1;

	while(Current->Flags != -1) {
		SLONG x = (Current->XCoord * m_TextureWidth) - StartX;
		SLONG y = (Current->YCoord * m_TextureHeight) - StartY;

		x0 = x;
		y0 = y;
		x1 = x + m_TextureWidth;
		y1 = y + m_TextureHeight;

		if( (XCoord > x0) && (XCoord < x1) && (YCoord > y0) && (YCoord < y1) ) {
			if(Current->Link < 0) {
				Current->Tid = Tid;
				Current->TFlags = TFlags;
				SetBrushTextureFromMap();
				return;
			}
		}

		Current++;
	}
}

struct ButtonEntry {
	int Command;
	int x,y;
	int Width,Height;
	int State;
	HICON *Icon;
	HICON *Icon2;
};

ButtonEntry EBButtons[]={
	{EB_DECREMENT,	16,16,	32,32,	0,&g_IconDecrement,&g_IconDecrement},
	{EB_INCREMENT,	16+32,16,	32,32,	0,&g_IconIncrement,&g_IconIncrement},
	{EB_SMALLBRUSH,	16+32*2,16,	32,32,	0,&g_IconSmallBrush,&g_IconSmallBrush2},
	{EB_LARGEBRUSH,	16+32*3,16,	32,32,	1,&g_IconLargeBrush,&g_IconLargeBrush2},
	{-1,	0,0,	0,0,	0,NULL,NULL}
};

void CEdgeBrush::DrawButtons(CDC *dc)
{
	ButtonEntry *Button = &EBButtons[0];

	while(Button->Command != -1) {

		if(Button->State) {
			dc->DrawIcon(Button->x,Button->y,*Button->Icon2);
		} else {
			dc->DrawIcon(Button->x,Button->y,*Button->Icon);
		}

		Button++;
	}
}

int CEdgeBrush::DoButtons(int XCoord,int YCoord)
{
	ButtonEntry *Button = &EBButtons[0];

	while(Button->Command != -1) {

		if( (XCoord > Button->x) && (XCoord < Button->x + Button->Width) &&
			(YCoord > Button->y) && (YCoord < Button->y + Button->Height) ) {

//			Button->State = (Button->State+1)&1;

			return Button->Command;
		}

		Button++;
	}

	return -1;
}

int CEdgeBrush::GetButtonState(int Index)
{
	return EBButtons[Index].State;
}

void CEdgeBrush::SetButtonState(int Index,int State)
{
	EBButtons[Index].State = State;
}


void CEdgeBrush::SetBrushDirect(int Index,int HeightMode,int Height,int RandomRange,int Tid,int Flags)
{
	_HeightMode = HeightMode;
	_Height = Height;
	_RandomRange = RandomRange;
	m_BrushMap[Index].Tid = Tid;
	m_BrushMap[Index].TFlags = Flags&255;
}


BOOL CEdgeBrush::Write(FILE* Stream)
{
	fprintf(Stream,"        ");

	fprintf(Stream,"%d ",_HeightMode);
	fprintf(Stream,"%d ",_Height);
	fprintf(Stream,"%d ",_RandomRange);

	for(int i=0; i<16; i++) {
		fprintf(Stream,"%d ",m_EdgeTable[i].Tid);
		fprintf(Stream,"%d ",m_EdgeTable[i].TFlags);
	}
	fprintf(Stream,"\n");

	return TRUE;
}


BOOL CEdgeBrush::ReadV1(FILE* Stream)
{
	for(int i=0; i<16; i++) {
		LONG Tid;
		LONG TFlags;

		CHECKTRUE(ReadLong(Stream,NULL,&Tid));
		m_BrushMap[i].Tid = (int)Tid;
		CHECKTRUE(ReadLong(Stream,NULL,&TFlags));
		m_BrushMap[i].TFlags = (DWORD)TFlags&255;
	}

	SetBrushTextureFromMap();

	return TRUE;
}

BOOL CEdgeBrush::ReadV2(FILE* Stream)
{
	LONG Tmp;

	CHECKTRUE(ReadLong(Stream,NULL,(long*)&Tmp));
	_HeightMode = (int)Tmp;
	CHECKTRUE(ReadLong(Stream,NULL,(long*)&Tmp));
	_Height = (int)Tmp;
	CHECKTRUE(ReadLong(Stream,NULL,(long*)&Tmp));
	_RandomRange = (int)Tmp;

	for(int i=0; i<16; i++) {
		LONG Tid;
		LONG TFlags;

		CHECKTRUE(ReadLong(Stream,NULL,&Tid));
		m_BrushMap[i].Tid = (int)Tid;
		CHECKTRUE(ReadLong(Stream,NULL,&TFlags));
		m_BrushMap[i].TFlags = (DWORD)TFlags&255;
	}

	SetBrushTextureFromMap();

	return TRUE;
}


void CEdgeBrush::FillMap(DWORD Selected,DWORD TextureID,DWORD Type,DWORD Flags)
{
	SWORD y = (SWORD)(Selected / m_HeightMap->m_MapWidth);
	SWORD x = (SWORD)(Selected - (y*m_HeightMap->m_MapWidth));

	FRect Rect;
	Rect.x0 = 0;
	Rect.y0 = 0;
	Rect.x1 = m_HeightMap->m_MapWidth-1;
	Rect.y1 = m_HeightMap->m_MapHeight-1;

//	SeedFill(x,y,&Rect,0,Type,Flags);
	SeedFill(x,y,&Rect,TextureID,Type,Flags);
}


Pixel CEdgeBrush::PixelRead(int x,int y)
{
	return m_HeightMap->m_MapTiles[(y*m_HeightMap->m_MapWidth)+x].TMapID;
}


BOOL CEdgeBrush::IsBrushEdge(Pixel ct)
{
	for(int i=0; i<13; i++) {
		if(ct == m_EdgeBrush[13].Tid) {
			return TRUE;
		}
	}

	return FALSE;
}


BOOL CEdgeBrush::PixelCompare(int x,int y,Pixel ov)
{
	Pixel ct = m_HeightMap->m_MapTiles[(y*m_HeightMap->m_MapWidth)+x].TMapID;

	if( (ct == ov) || IsBrushEdge(ct) ) {
		return TRUE;
	}

	return FALSE;
}


void CEdgeBrush::PixelWrite(int x,int y,Pixel nv,DWORD Type,DWORD Flags)
{
	assert(x >=0);
	assert(y >=0);
	assert(x < m_HeightMap->m_MapWidth);
	assert(y < m_HeightMap->m_MapHeight);

	Paint(x,y,FALSE);
}


struct FillPoint {
	int x;
	int y;
};

#define MAX_FILL_LIST_SIZE	16384
FillPoint FillList[MAX_FILL_LIST_SIZE];
int FillListSize = 0;


/*
 * fill: set the pixel at (x,y) and all of its 4-connected neighbors
 * with the same pixel value to the new pixel value nv.
 * A 4-connected neighbor is a pixel above, below, left, or right of a pixel.
 */
void CEdgeBrush::SeedFill(int x, int y, FRect *win, Pixel nv,DWORD Type,DWORD Flags)
{
    int l, x1, x2, dy;
    Pixel ov;							/* old pixel value */
    Segment stack[MAX], *sp = stack;	/* stack of filled segments */

    ov = PixelRead(x, y);		/* read pv at seed point */
	nv = 255;

	FillListSize = 0;

    if (ov==nv || x<win->x0 || x>win->x1 || y<win->y0 || y>win->y1) {
		return;
	}

    PUSH(y, x, x, 1);			/* needed in some cases */
    PUSH(y+1, x, x, -1);		/* seed segment (popped 1st) */

    while (sp>stack) {
		/* pop segment off stack and fill a neighboring scan line */
		POP(y, x1, x2, dy);
		/*
		 * segment of scan line y-dy for x1<=x<=x2 was previously filled,
		 * now explore adjacent pixels in scan line y
		 */
		for (x=x1; x>=win->x0 && PixelRead(x, y)==ov; x--) {
   			if(FillListSize+1 < MAX_FILL_LIST_SIZE) {
   				m_HeightMap->m_MapTiles[(y*m_HeightMap->m_MapWidth)+x].TMapID = 255;
   				FillList[FillListSize].x = x;
   				FillList[FillListSize].y = y;
   				FillListSize++;
   			} else {
   				break;
   			}
		}

		if (x>=x1) {
			goto skip;
		}

		l = x+1;
		if (l<x1) {
			PUSH(y, l, x1-1, -dy);		/* leak on left? */
		}
		x = x1+1;

		do {
			for (; x<=win->x1 && PixelRead(x, y)==ov; x++) {
				if(FillListSize+1 < MAX_FILL_LIST_SIZE) {
					m_HeightMap->m_MapTiles[(y*m_HeightMap->m_MapWidth)+x].TMapID = 255;
					FillList[FillListSize].x = x;
					FillList[FillListSize].y = y;
					FillListSize++;
				} else {
					break;
				}
			}

			PUSH(y, l, x-1, dy);

			if (x>x2+1) {
				PUSH(y, x2+1, x-1, -dy);	/* leak on right? */
			}

skip:
			for (x++; x<=x2 && PixelRead(x, y)!=ov; x++);
			l = x;
		} while (x<=x2);
    }

	DebugPrint("FillListSize %d\n",FillListSize);

	for(int i=0; i<FillListSize; i++) {
		m_HeightMap->m_MapTiles[(FillList[i].y*m_HeightMap->m_MapWidth)+FillList[i].x].TMapID = ov;
	}

	for(i=0; i<FillListSize; i++) {
		Paint(FillList[i].x,FillList[i].y,FALSE);
	}
}

