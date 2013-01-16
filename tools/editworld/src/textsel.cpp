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
// TextureView.cpp : implementation file
//

#include "stdafx.h"
#include "debugprint.hpp"
#include "textsel.h"
#include "heightmap.h"
#include "tiletypes.h"

#include <fstream>
#include <string>

using std::string;

extern string g_HomeDirectory;

/////////////////////////////////////////////////////////////////////////////
// CTextureSelector

DWORD g_Flags[MAXTILES];
DWORD g_Types[MAXTILES];

CTextureSelector::CTextureSelector(CDirectDraw *DirectDraw)
{
	m_DirectDraw = DirectDraw;
	m_NumSprites = 0;
	m_NumTextures = 0;
	m_Width = 64;
	m_Height = 64;

	for(int i=0; i<8; i++) {
		m_ButtonImage[i] = NULL;
	}

	m_Button.Surface = NULL;
	m_NumObjects = 0;
//	m_ObjectNames = NULL;

	m_Font.CreateFont(10,0,0,0,FW_NORMAL,0,0,0,
				DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif");
//	SetFont( &m_Font,FALSE );
}

CTextureSelector::~CTextureSelector(void)
{
	for(int i=0; i<(int)m_NumSprites; i++) {
		if(m_SpriteImage[i]) delete m_SpriteImage[i];
	}
	for(i=0; i<(int)m_NumTextures; i++) {
		if(m_Texture[i].Surface) m_Texture[i].Surface->Release();
	}

	DeleteData();
}

BOOL CTextureSelector::Read(DWORD NumTextures,char **TextureList,DWORD TextureWidth,DWORD TextureHeight,
							DWORD NumObjects,CHeightMap *HeightMap,BOOL InitTypes)
{
	m_TextureWidth = TextureWidth;
	m_TextureHeight = TextureHeight;
	m_HeightMap = HeightMap;

	DeleteData();

	for(int i=0; i<(int)m_NumSprites; i++) {
		if(m_SpriteImage[i]) delete m_SpriteImage[i];
	}

	if(TextureList) {
		char	FileName[256];
		char	Drive[256];
   		char	Dir[256];
   		char	FName[256];
   		char	Ext[256];

		DWORD	TexNum;
		DWORD	SpriteNum=0;
		DWORD	x,y;

		for(TexNum = 0; TexNum<NumTextures; TexNum++) {
			ASSERT(TexNum < 16);

   			strcpy(m_Texture[TexNum].Name,TextureList[TexNum]);
   			strcpy(FileName,TextureList[TexNum]);
			_splitpath(FileName,Drive,Dir,FName,Ext);

			DWORD	i;
			for(i=0; i<strlen(Ext); i++) {
				if( islower( Ext[i] ) ) {
				  Ext[i] = toupper( Ext[i] );
				}
			}

			if(strcmp(Ext,".PCX")==0) {	

			// Load the PCX bitmap for the textures.
				std::ifstream PCXFile(FileName, std::ios_base::binary);
				if (!PCXFile.is_open())
				{
					MessageBox(NULL, FileName, "Unable to open file.", MB_OK);
					return FALSE;
				}

				PCXHandler* PCXTexBitmap = new PCXHandler;

				if(!PCXTexBitmap->ReadPCX(PCXFile))
				{
					char tmpCharArr[1024];
					GetCurrentDirectory(sizeof(tmpCharArr), tmpCharArr);
					string CurDir(tmpCharArr);
					CurDir += "\\";
					CurDir += FileName;
					MessageBox(NULL, "Error reading PCX", CurDir.c_str(), MB_OK);
					delete PCXTexBitmap;
					return FALSE;
				}

				m_Texture[TexNum].Width = PCXTexBitmap->GetBitmapWidth();
				m_Texture[TexNum].Height = PCXTexBitmap->GetBitmapHeight();

				m_Texture[TexNum].Surface = m_DirectDraw->CreateImageSurfaceFromBitmap(
												PCXTexBitmap->GetBitmapBits(),
												m_Texture[TexNum].Width,m_Texture[TexNum].Height,
												PCXTexBitmap->GetBitmapPaletteEntries(),SURFACE_SYSTEMMEMORY);

				for(y=0; y<m_Texture[TexNum].Height; y+=m_TextureHeight) {
					for(x=0; x<m_Texture[TexNum].Width; x+=m_TextureWidth) {
						m_SpriteImage[SpriteNum]=new DDImage(x,y,m_TextureWidth,m_TextureHeight,m_DirectDraw,m_Texture[TexNum].Surface);
						if(InitTypes) {
							g_Flags[SpriteNum] = 0;
							g_Types[SpriteNum] = TF_DEFAULTTYPE;
						}
						m_SpriteFlags[SpriteNum] = 0;
//						m_SpriteFlags[SpriteNum] = SF_TEXTUREFLIPX;
						SpriteNum++;
					}
				}

				delete PCXTexBitmap;
			}
		}

		SetRect(&m_SelectedRect, 0,0, m_TextureWidth,m_TextureHeight);
		m_SelectedID = 0;
		m_NumTextures = NumTextures;
		m_NumSprites = SpriteNum;
		m_Width = m_TextureWidth* 4;
		m_Height = (SpriteNum / (m_Width / m_TextureWidth)) * m_TextureHeight;
	}

	string Name = g_HomeDirectory + "\\Data\\Buttons.PCX";

	std::ifstream PCXFile(Name.c_str(), std::ios_base::binary);
	if (!PCXFile.is_open())
	{
		MessageBox(NULL, Name.c_str(), "Unable to open file.", MB_OK);
		return FALSE;
	}

	PCXHandler* PCXTexBitmap = new PCXHandler;

	if(!PCXTexBitmap->ReadPCX(PCXFile))
	{
		MessageBox(NULL, "Error reading PCX", Name.c_str(), MB_OK);
		delete PCXTexBitmap;
		return FALSE;
	}


	m_StructureBase = -1;
	m_NumStructures = 0;

	m_NumObjects = NumObjects;

	if(m_NumObjects) {
		for(int i=0; i<(int)NumObjects; i++) {
			if(HeightMap->GetObjectType(i) == IMD_STRUCTURE) {
				if(m_StructureBase < 0) m_StructureBase = i;
				m_NumStructures++;
//				i+=7;
//				m_NumObjects -= 7;
			}
		}
	}

	DebugPrint("NumStructures %d, StructureBase %d\n",m_NumStructures,m_StructureBase);


	m_Button.Width = PCXTexBitmap->GetBitmapWidth();
	m_Button.Height = PCXTexBitmap->GetBitmapHeight();
	m_Button.Surface = m_DirectDraw->CreateImageSurfaceFromBitmap(
											PCXTexBitmap->GetBitmapBits(),
											m_Button.Width,m_Button.Height,
											PCXTexBitmap->GetBitmapPaletteEntries(),SURFACE_SYSTEMMEMORY);

	m_ButtonImage[0] = new DDImage(0,0,64,64,m_DirectDraw,m_Button.Surface);
	m_ButtonImage[1] = new DDImage(64,0,64,64,m_DirectDraw,m_Button.Surface);
	m_ButtonImage[2] = new DDImage(0,64,64,64,m_DirectDraw,m_Button.Surface);
	m_ButtonImage[3] = new DDImage(64,64,64,64,m_DirectDraw,m_Button.Surface);
	m_ButtonImage[4] = new DDImage(0,0+128,64,64,m_DirectDraw,m_Button.Surface);
	m_ButtonImage[5] = new DDImage(64,0+128,64,64,m_DirectDraw,m_Button.Surface);
	m_ButtonImage[6] = new DDImage(0,64+128,64,64,m_DirectDraw,m_Button.Surface);
	m_ButtonImage[7] = new DDImage(64,64+128,64,64,m_DirectDraw,m_Button.Surface);


	delete PCXTexBitmap;

//	m_NumObjects = NumObjects;
//	if(NumObjects) {
//		m_ObjectNames = new char*[NumObjects];
//		for(int i=0; i<NumObjects; i++) {
//			m_ObjectNames[i] = new char[strlen(HeightMap->GetObjectName(i))+1];
//			strcpy(m_ObjectNames[i],HeightMap->GetObjectName(i));
//		}
//	}

	return TRUE;
}

void CTextureSelector::DeleteData(void)
{
	for(int i=0; i<8; i++) {
		if(m_ButtonImage[i]) {
			delete m_ButtonImage[i];
			m_ButtonImage[i] = NULL;
		}
	}

	if(m_Button.Surface) {
		m_Button.Surface->Release();
		m_Button.Surface = NULL;
	}

	if(m_NumObjects) {
//		for(int i=0; i<m_NumObjects; i++) {
//			delete m_ObjectNames[i];
//		}
//
//		delete m_ObjectNames;
//
//		m_ObjectNames = NULL;
		m_NumObjects = 0;
	}
}


void CTextureSelector::CalcSize(HWND hWnd)
{
	DWORD NumItems = m_NumSprites;	// + m_NumObjects;
 	DWORD TPerRow;
	RECT Rect;
	::GetClientRect(hWnd,&Rect);

	if(NumItems) {
		TPerRow = Rect.right/m_TextureWidth;
		if(TPerRow < 1) {
			TPerRow = 1;
		}
		m_Width = TPerRow * m_TextureWidth;
		m_Height = ((NumItems+TPerRow-1) / TPerRow) * m_TextureHeight;
	} else {
		m_Width = 64;
		m_Height = 64;
	}

	NumItems = m_NumObjects;
	if(NumItems) {
		TPerRow = Rect.right/64;
		if(TPerRow < 1) {
			TPerRow = 1;
		}
		if(m_NumSprites == 0) {
			m_Width = TPerRow*64;
			m_Height = 0;
		}
		m_ObjHeight = ((NumItems+TPerRow-1) / TPerRow) * 64;
	} else {
		m_ObjHeight = 0;
	}
}

DWORD CTextureSelector::GetWidth(HWND hWnd)
{
	CalcSize(hWnd);
	return m_Width;
}

DWORD CTextureSelector::GetHeight(HWND hWnd)
{
	CalcSize(hWnd);
	return m_Height + m_ObjHeight;
}

void CTextureSelector::Draw(CDIBDraw *DIBDraw,int XOffset,int YOffset)
{
	DWORD	x,y;
	DWORD	SpriteNum = 0;
	DWORD	ObjectNum = 0;
	char String[256];
	char Flags[16];

	YOffset+=m_TextureHeight;

	if(m_NumSprites + m_NumObjects) {
		CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

		CPen Pen;
		Pen.CreatePen( PS_SOLID, 2, RGB(255,255,255) );
        CPen* pOldPen = dc->SelectObject( &Pen );
		dc->SelectObject(&m_Font);
		dc->SetBkMode(TRANSPARENT);
		dc->SetTextColor(RGB(0,0,0));
//		dc->SetBkColor(RGB(191,191,191));

		CalcSize((HWND)DIBDraw->GetDIBWindow());

		DWORD Step = m_TextureWidth;

//		m_SpriteFlags[1] = SF_RANDTEXTUREROTATE;
//		m_SpriteFlags[2] = SF_INCTEXTUREROTATE;
//		m_SpriteFlags[4] = SF_RANDTEXTUREROTATE | SF_RANDTEXTUREFLIPX;
//		m_SpriteFlags[5] = SF_INCTEXTUREROTATE | SF_TOGTEXTUREFLIPX;
//		m_SpriteFlags[8] = SF_RANDTEXTUREROTATE | SF_RANDTEXTUREFLIPX | SF_RANDTEXTUREFLIPY;
//		m_SpriteFlags[9] = SF_INCTEXTUREROTATE | SF_TOGTEXTUREFLIPX | SF_TOGTEXTUREFLIPY;
//		m_SpriteFlags[12] = SF_RANDTEXTUREFLIPX | SF_RANDTEXTUREFLIPY;
//		m_SpriteFlags[13] = SF_TOGTEXTUREFLIPX | SF_TOGTEXTUREFLIPY;

		for(y=0; (y<m_Height) && (SpriteNum<m_NumSprites); y+= Step) {
			for(x=0; (x<m_Width) && (SpriteNum<m_NumSprites); x+= Step) {
				CSize Size;
				int ty = y+YOffset+4;	//+16;
				int Rotate;

				m_SpriteImage[SpriteNum]->DDBlitImageDIB(DIBDraw,x+XOffset,y+YOffset,0);
//															m_SpriteFlags[SpriteNum]);

				DisplayTerrainType(DIBDraw,x+XOffset,y+YOffset,
									m_TextureWidth,m_TextureHeight,g_Types[SpriteNum+1]);

				sprintf(String,"%d",SpriteNum);
				Size = dc->GetTextExtent( String,strlen(String));

				dc->SetTextColor(RGB(0,0,0));
				dc->TextOut(x+XOffset+4+1,ty+1,String);
				dc->SetTextColor(RGB(255,255,255));
				dc->TextOut(x+XOffset+4,ty,String);
				ty += Size.cy;

				Flags[0] = 0;
				if(m_SpriteFlags[SpriteNum] & SF_RANDTEXTUREFLIPX) {
					strcat(Flags,"x");
				}
				if(m_SpriteFlags[SpriteNum] & SF_RANDTEXTUREFLIPY) {
					strcat(Flags,"y");
				}
				if(m_SpriteFlags[SpriteNum] & SF_RANDTEXTUREROTATE) {
					strcat(Flags,"r");
				}

				if(Flags[0]) {
					sprintf(String,"Rnd %s",Flags);
					Size = dc->GetTextExtent( String,strlen(String));

					dc->SetTextColor(RGB(0,0,0));
					dc->TextOut(x+XOffset+4+1,ty+1,String);
					dc->SetTextColor(RGB(255,255,255));
					dc->TextOut(x+XOffset+4,ty,String);
					ty += Size.cy;
				}

				Flags[0] = 0;
				if(m_SpriteFlags[SpriteNum] & SF_TEXTUREFLIPX) {
					strcat(Flags,"x");
				}
				if(m_SpriteFlags[SpriteNum] & SF_TEXTUREFLIPY) {
					strcat(Flags,"y");
				}

				if(Flags[0]) {
					sprintf(String,"Flip %s",Flags);
					Size = dc->GetTextExtent( String,strlen(String));

					dc->SetTextColor(RGB(0,0,0));
					dc->TextOut(x+XOffset+4+1,ty+1,String);
					dc->SetTextColor(RGB(255,255,255));
					dc->TextOut(x+XOffset+4,ty,String);
					ty += Size.cy;
				}

				Rotate = (m_SpriteFlags[SpriteNum] & SF_TEXTUREROTMASK) >> SF_TEXTUREROTSHIFT;
				if(Rotate) {
					sprintf(String,"Rot %d",Rotate * 90);
					Size = dc->GetTextExtent( String,strlen(String));

					dc->SetTextColor(RGB(0,0,0));
					dc->TextOut(x+XOffset+4+1,ty+1,String);
					dc->SetTextColor(RGB(255,255,255));
					dc->TextOut(x+XOffset+4,ty,String);
					ty += Size.cy;
				}



				SpriteNum++;
			}
		}

		DWORD xs = 0;
		DWORD ys = m_Height;
		Step = 64;

		int ExtraObjects = 0;	//m_NumStructures;	//*7;

		for(y=ys; (y<ys+m_ObjHeight) && (ObjectNum<m_NumObjects+ExtraObjects); y+= Step) {
			for(x=xs; (x<xs+m_Width) && (ObjectNum<m_NumObjects+ExtraObjects); x+= Step) {

				int Type = m_HeightMap->GetObjectType(ObjectNum);
				int ty = y+YOffset+4;	//+16;
				char TString[256];
				CSize Size;

				if(m_HeightMap->StructureIsWall(ObjectNum)) {
					m_ButtonImage[Type+3]->DDBlitImageDIB(DIBDraw,x+XOffset,y+YOffset);
				} else if(m_HeightMap->StructureIsDefense(ObjectNum)) {
					m_ButtonImage[Type+4]->DDBlitImageDIB(DIBDraw,x+XOffset,y+YOffset);
				} else {
					m_ButtonImage[Type]->DDBlitImageDIB(DIBDraw,x+XOffset,y+YOffset);
				}

				strcpy(String,m_HeightMap->GetObjectName(ObjectNum));
				Size = dc->GetTextExtent( String,strlen(String));

				if(Size.cx > 63-6) {
					char *Str = String;

					while(*Str) {
						int Len = strlen(Str);

						for(int j=0; j<Len; j++) {
							TString[j] = *Str;
							TString[j+1] = 0;
							Size = dc->GetTextExtent( TString,strlen(TString));
							if(Size.cx > 63-6) {
								TString[j] = 0;
								break;
							} else {
								Str++;
							}
						}

						dc->SetTextColor(RGB(0,0,0));
						dc->TextOut(x+XOffset+4+1,ty+1,TString);
						dc->SetTextColor(RGB(255,255,255));
						dc->TextOut(x+XOffset+4,ty,TString);
						ty += Size.cy;
					}
				} else {
					dc->SetTextColor(RGB(0,0,0));
					dc->TextOut(x+XOffset+4+1,ty+1,String);
					dc->SetTextColor(RGB(255,255,255));
					dc->TextOut(x+XOffset+4,ty,String);
				}

				ObjectNum++;
			}
		}

		dc->MoveTo(m_SelectedRect.left+2+XOffset,m_SelectedRect.top+2+YOffset);
		dc->LineTo(m_SelectedRect.right-2+XOffset,m_SelectedRect.top+2+YOffset);
		dc->LineTo(m_SelectedRect.right-2+XOffset,m_SelectedRect.bottom-2+YOffset);
		dc->LineTo(m_SelectedRect.left+2+XOffset,m_SelectedRect.bottom-2+YOffset);
		dc->LineTo(m_SelectedRect.left+2+XOffset,m_SelectedRect.top+2+YOffset);

        dc->SelectObject( pOldPen );
	}
}


void CTextureSelector::SetSelectedTexture(DWORD Selected)
{
	DWORD	YCoord = (Selected-1)/(m_Width/m_TextureWidth);
	DWORD	XCoord = (Selected-1)-(YCoord*(m_Width/m_TextureWidth));

	XCoord *=m_TextureWidth;
	YCoord *=m_TextureHeight;

	SetRect(&m_SelectedRect, XCoord,YCoord, XCoord+m_TextureWidth,YCoord+m_TextureHeight);
	m_SelectedID = YCoord*(m_Width/m_TextureWidth)+XCoord+1;
	if(m_SelectedID > m_NumSprites) {
		m_SelectedID = m_NumSprites;
	}
}


//void CTextureSelector::SetSelectedTexture(DWORD Selected)
//{
//	DWORD	YCoord = (Selected-1)/(m_Width/m_TextureWidth);
//	DWORD	XCoord = (Selected-1)-(YCoord*(m_Width/m_TextureWidth));
//
//	XCoord *=m_TextureWidth;
//	YCoord *=m_TextureHeight;
//
//	SetRect(&m_SelectedRect, XCoord,YCoord, XCoord+m_TextureWidth,YCoord+m_TextureHeight);
//	m_SelectedID = YCoord*(m_Width/m_TextureWidth)+XCoord+1;
//	if(m_SelectedID > m_NumSprites) {
//		m_SelectedID = m_NumSprites;
//	}
//}


BOOL CTextureSelector::SelectTexture(DWORD XCoord,DWORD YCoord)
{
	if(m_NumSprites + m_NumObjects) {
		if((XCoord < m_Width) && ( YCoord < m_Height)) {
			XCoord = ( XCoord / m_TextureWidth ) * m_TextureWidth;
			YCoord = ( YCoord / m_TextureHeight ) * m_TextureHeight;

			SetRect(&m_SelectedRect, XCoord,YCoord, XCoord+m_TextureWidth,YCoord+m_TextureHeight);

			XCoord = ( XCoord / m_TextureWidth );
			YCoord = ( YCoord / m_TextureHeight );

			m_SelectedID = YCoord*(m_Width/m_TextureWidth)+XCoord+1;
			if(m_SelectedID > m_NumSprites) {
				m_SelectedID = m_NumSprites;
			}

			DebugPrint("Selected Texture %d\n",m_SelectedID);
			return TRUE;
		}

		if((XCoord < m_Width) && (YCoord > m_Height) && (YCoord < m_Height+m_ObjHeight)) {
			YCoord -= m_Height;
			XCoord = ( XCoord / 64 ) * 64;
			YCoord = ( YCoord / 64 ) * 64;

			SetRect(&m_SelectedRect, XCoord,YCoord+m_Height, XCoord+64,YCoord+m_Height+64);

			XCoord = ( XCoord / 64 );
			YCoord = ( YCoord / 64 );

			m_SelectedID = YCoord*(m_Width/64)+XCoord+1;
			m_SelectedID += m_NumSprites;
			if(m_SelectedID > m_NumSprites + m_NumObjects) {
				m_SelectedID = m_NumSprites + m_NumObjects;
			}


			int ObjID = m_SelectedID - m_NumSprites - 1;
//			if((ObjID >= m_StructureBase) && (ObjID < m_StructureBase+m_NumStructures)) {
//				m_SelectedID = m_NumSprites + 1 + m_StructureBase + (ObjID-m_StructureBase)*8;
//				DebugPrint("Is Structure %d\n",(ObjID-m_StructureBase)*8);
//			}
//
//			if((ObjID >= m_StructureBase+m_NumStructures)  && m_NumStructures) {
//				m_SelectedID += m_NumStructures * 7;
//			}
//
			DebugPrint("Selected Object %d\n",m_SelectedID);

			return TRUE;
		}
	}

	return FALSE;
}


void CTextureSelector::SetTextureType(DWORD Index,DWORD Type)
{
	if(Index < m_NumSprites) {
//	ASSERT(Index <= m_NumSprites);
//	g_Flags[Index] = Type;
		g_Types[Index] = Type;
	}
}


BOOL CTextureSelector::GetTextureFlipX(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_TEXTUREFLIPX ? TRUE : FALSE;
}

BOOL CTextureSelector::GetTextureFlipY(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_TEXTUREFLIPY ? TRUE : FALSE;
}

DWORD CTextureSelector::GetTextureRotate(DWORD Index)
{
	return (m_SpriteFlags[Index] & SF_TEXTUREROTMASK) >> SF_TEXTUREROTSHIFT;
}

void CTextureSelector::SetTextureFlip(DWORD Index,BOOL FlipX,BOOL FlipY)
{
	if(FlipX) {
		m_SpriteFlags[Index] |= SF_TEXTUREFLIPX;
	} else {
		m_SpriteFlags[Index] &= ~SF_TEXTUREFLIPX;
	}
	if(FlipY) {
		m_SpriteFlags[Index] |= SF_TEXTUREFLIPY;
	} else {
		m_SpriteFlags[Index] &= ~SF_TEXTUREFLIPY;
	}
}

void CTextureSelector::SetTextureRotate(DWORD Index,DWORD Rotate)
{
	Rotate <<= SF_TEXTUREROTSHIFT;
	m_SpriteFlags[Index] &= ~SF_TEXTUREROTMASK;
	m_SpriteFlags[Index] |= Rotate;
}


BOOL CTextureSelector::GetTextureRandomFlipX(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_RANDTEXTUREFLIPX ? TRUE : FALSE;
}


BOOL CTextureSelector::GetTextureRandomFlipY(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_RANDTEXTUREFLIPY ? TRUE : FALSE;
}


BOOL CTextureSelector::GetTextureRandomRotate(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_RANDTEXTUREROTATE ? TRUE : FALSE;
}


BOOL CTextureSelector::GetTextureToggleFlipX(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_TOGTEXTUREFLIPX ? TRUE : FALSE;
}


BOOL CTextureSelector::GetTextureToggleFlipY(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_TOGTEXTUREFLIPY ? TRUE : FALSE;
}


BOOL CTextureSelector::GetTextureIncrementRotate(DWORD Index)
{
	return m_SpriteFlags[Index] & SF_INCTEXTUREROTATE ? TRUE : FALSE;
}



void CTextureSelector::SetTextureRandomFlip(DWORD Index,BOOL FlipX,BOOL FlipY)
{
	if(FlipX) {
		m_SpriteFlags[Index] |= SF_RANDTEXTUREFLIPX;
	} else {
		m_SpriteFlags[Index] &= ~SF_RANDTEXTUREFLIPX;
	}
	if(FlipY) {
		m_SpriteFlags[Index] |= SF_RANDTEXTUREFLIPY;
	} else {
		m_SpriteFlags[Index] &= ~SF_RANDTEXTUREFLIPY;
	}
}


void CTextureSelector::SetTextureRandomRotate(DWORD Index,BOOL Rotate)
{
	if(Rotate) {
		m_SpriteFlags[Index] |= SF_RANDTEXTUREROTATE;
	} else {
		m_SpriteFlags[Index] &= ~SF_RANDTEXTUREROTATE;
	}
}


void CTextureSelector::SetTextureToggleFlip(DWORD Index,BOOL FlipX,BOOL FlipY)
{
	if(FlipX) {
		m_SpriteFlags[Index] |= SF_TOGTEXTUREFLIPX;
	} else {
		m_SpriteFlags[Index] &= ~SF_TOGTEXTUREFLIPX;
	}
	if(FlipY) {
		m_SpriteFlags[Index] |= SF_TOGTEXTUREFLIPY;
	} else {
		m_SpriteFlags[Index] &= ~SF_TOGTEXTUREFLIPY;
	}
}


void CTextureSelector::SetTextureIncrementRotate(DWORD Index,BOOL Rotate)
{
	if(Rotate) {
		m_SpriteFlags[Index] |= SF_INCTEXTUREROTATE;
	} else {
		m_SpriteFlags[Index] &= ~SF_INCTEXTUREROTATE;
	}
}


DWORD CTextureSelector::GetTextureFlags(DWORD Index)
{
	if(Index >= m_NumSprites) return 0;
	return m_SpriteFlags[Index];
}


void CTextureSelector::SetTextureFlags(DWORD Index,DWORD Flags)
{
	if(Index >= m_NumSprites) return;
	m_SpriteFlags[Index] = Flags;
}



DWORD CTextureSelector::GetTextureType(DWORD Index)
{
	if(Index > m_NumSprites) return 0;
//	return g_Flags[Index];
	return g_Types[Index];
}

DWORD CTextureSelector::GetNumTiles(void)
{
	return m_NumSprites+1;
}


void CTextureSelector::ResetTextureFlags(void)
{
	for(int i=0; i < (int)m_NumSprites; i++) {
		m_SpriteFlags[i] = 0;
	}
}


void CTextureSelector::SetTextureType(DWORD XCoord,DWORD YCoord,DWORD Type)
{
	if(m_NumSprites) {
		if((XCoord < m_Width) && ( YCoord < m_Height)) {
			XCoord = ( XCoord / m_TextureWidth ) * m_TextureWidth;
			YCoord = ( YCoord / m_TextureHeight ) * m_TextureHeight;

			XCoord = ( XCoord / m_TextureWidth );
			YCoord = ( YCoord / m_TextureHeight );

			DWORD Selected = YCoord*(m_Width/m_TextureWidth)+XCoord+1;

			if(Selected > m_NumSprites) {
				return;
			}

//			g_Flags[Selected] = Type;
			g_Types[Selected] = Type;
		}
	}
}


