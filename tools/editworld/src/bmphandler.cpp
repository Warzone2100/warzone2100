/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
*/
//#include "stdafx.h"
#include "winstuff.h"
#include "stdio.h"
#include "assert.h"
#include "debugprint.h"
#include "bmphandler.h"


BMPHandler::BMPHandler(void)
{
	m_Palette = NULL;
	m_DIBBitmap = NULL;
	m_BitmapInfo = NULL;
}


BMPHandler::~BMPHandler(void)
{
	DebugPrint("Deleted BMPHandler\n");

	if(m_Palette!=NULL) {
		delete m_Palette;
	}
	if(m_DIBBitmap!=NULL) {
		DeleteObject(m_DIBBitmap);
	}
	if(m_BitmapInfo!=NULL) {
		delete m_BitmapInfo;
	}
}


BOOL BMPHandler::ReadBMP(char *FilePath,BOOL Flip)
{
	FILE	*fid;

	fid=fopen(FilePath,"rb");
    if(fid==NULL) {
		return (FALSE);
	}

	DWORD	i;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	if(m_Palette!=NULL) {
		delete m_Palette;
		m_Palette = NULL;
	}
	if(m_DIBBitmap!=NULL) {
		DeleteObject(m_DIBBitmap);
		m_DIBBitmap = NULL;
	}
	if(m_BitmapInfo!=NULL) {
		delete m_BitmapInfo;
		m_BitmapInfo = NULL;
	}

// Get the BITMAPFILEHEADER structure.
	fread(&bmfh,sizeof(BITMAPFILEHEADER),1,fid);
	if( (((char*)&bmfh.bfType)[0] != 'B') ||
		(((char*)&bmfh.bfType)[1] != 'M') ) {

		fclose(fid);
		MessageBox( NULL, FilePath, "File is not a valid BMP.", MB_OK );
		return FALSE;
	}

// Get the BITMAPINFOHEADER structure.
	fread(&bmih,sizeof(BITMAPINFOHEADER),1,fid);

	WORD PaletteSize=0;

	switch(bmih.biBitCount) {
		case	1:
			PaletteSize=2;
			break;
		case	4:
			PaletteSize=16;
			break;
		case	8:
			PaletteSize=256;
			break;
		default:
			PaletteSize=0;
	}

	m_BitmapInfo=(BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)+sizeof(RGBQUAD)*PaletteSize];
	m_BitmapInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	m_BitmapInfo->bmiHeader.biWidth=bmih.biWidth;
	m_BitmapInfo->bmiHeader.biHeight=bmih.biHeight;
	m_BitmapInfo->bmiHeader.biPlanes=bmih.biPlanes;
	m_BitmapInfo->bmiHeader.biBitCount=bmih.biBitCount;
	m_BitmapInfo->bmiHeader.biCompression=bmih.biCompression;
	m_BitmapInfo->bmiHeader.biSizeImage=bmih.biSizeImage;
	m_BitmapInfo->bmiHeader.biXPelsPerMeter=bmih.biXPelsPerMeter;
	m_BitmapInfo->bmiHeader.biYPelsPerMeter=bmih.biYPelsPerMeter;
	m_BitmapInfo->bmiHeader.biClrUsed=bmih.biClrUsed;
	m_BitmapInfo->bmiHeader.biClrImportant=bmih.biClrImportant;

// If there's a palette then get it.
	if(PaletteSize) {
		m_Palette=new PALETTEENTRY[PaletteSize];

		for (i=0; i<PaletteSize; i++)
		{
			m_Palette[i].peBlue  = (BYTE)getc(fid);
			m_Palette[i].peGreen = (BYTE)getc(fid);
			m_Palette[i].peRed   = (BYTE)getc(fid);
			getc(fid);
			m_Palette[i].peFlags = 0;
		}

		memcpy(m_BitmapInfo->bmiColors,m_Palette,PaletteSize*sizeof(PALETTEENTRY));
	}

	m_DIBBitmap=CreateDIBSection(NULL,m_BitmapInfo,DIB_RGB_COLORS,&m_DIBBits,NULL,0);

	if(m_DIBBitmap==NULL) {
		MessageBox( NULL, FilePath, "Failed to create DIB.", MB_OK );
		return(FALSE);
	}

// Get the bitmap data.
 	fread(m_DIBBits, (bmfh.bfSize - bmfh.bfOffBits),1,fid);

	fclose(fid);

	if( Flip && (bmih.biHeight > 0) ) {
		char *Top = (char*)m_DIBBits;
		char *Bottom = Top + bmih.biWidth*(bmih.biHeight-1);
		char Tmp;
		int x;
		while( ((DWORD)Top) < ((DWORD)Bottom) ) {
			for(x=0; x<bmih.biWidth; x++) {
				Tmp = Top[x];
				Top[x] = Bottom[x];
				Bottom[x] = Tmp;
			}
			Top+=bmih.biWidth;
			Bottom-=bmih.biWidth;
		}
	}

	return(TRUE);
}


BOOL BMPHandler::Create(int Width,int Height,void *Bits,PALETTEENTRY *Palette,int BPP,BOOL Is555)
{
	int Planes = 1;
//	int BPP = 8;
	int PaletteSize;
	int i;

	if(Palette) {
		PaletteSize=1<<BPP;

		m_Palette = new PALETTEENTRY[PaletteSize];

		for (i=0; i<PaletteSize; i++) {
			m_Palette[i] = Palette[i];
		}
	} else {
		PaletteSize = 3;
	}

	m_BitmapInfo=(BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)+sizeof(RGBQUAD)*PaletteSize];
	m_BitmapInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	m_BitmapInfo->bmiHeader.biWidth=Width;
	m_BitmapInfo->bmiHeader.biHeight=Height;
	m_BitmapInfo->bmiHeader.biPlanes=Planes;
	m_BitmapInfo->bmiHeader.biBitCount=BPP;
	switch(BPP) {
		case	4:
			m_BitmapInfo->bmiHeader.biCompression=BI_RGB;
			m_BitmapInfo->bmiHeader.biSizeImage=(Width/2)*Height;
			break;
		case	8:
			m_BitmapInfo->bmiHeader.biCompression=BI_RGB;
			m_BitmapInfo->bmiHeader.biSizeImage=Width*Height;
			break;
		case	16:
			m_BitmapInfo->bmiHeader.biCompression= BI_BITFIELDS;
			m_BitmapInfo->bmiHeader.biSizeImage=Width*2*Height;
			break;
	}
	m_BitmapInfo->bmiHeader.biXPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biYPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biClrUsed=0;
	m_BitmapInfo->bmiHeader.biClrImportant=0;

	if(Palette) {
		for(i=0; i<PaletteSize; i++) {
			m_BitmapInfo->bmiColors[i].rgbRed   = m_Palette[i].peRed;
			m_BitmapInfo->bmiColors[i].rgbGreen = m_Palette[i].peGreen;
			m_BitmapInfo->bmiColors[i].rgbBlue  = m_Palette[i].peBlue;
			m_BitmapInfo->bmiColors[i].rgbReserved = 0;
		}
	} else {

		DWORD *PFormat = (DWORD*)m_BitmapInfo->bmiColors;

		if(Is555) {
			PFormat[0] = 0x7c00;
			PFormat[1] = 0x03e0;
			PFormat[2] = 0x001f;
		} else {
			PFormat[0] = 0xf800;
			PFormat[1] = 0x07e0;
			PFormat[2] = 0x001f;
		}
	}

	m_DIBBitmap=CreateDIBSection(NULL,m_BitmapInfo,DIB_RGB_COLORS,&m_DIBBits,NULL,0);

	if(m_DIBBitmap==NULL) {
		MessageBox( NULL, "Error", "Failed to create DIB.", MB_OK );
		return FALSE;
	}

	BYTE	*Dst=(BYTE*)m_DIBBits;
	BYTE	*Src;
	WORD	j,p;

	switch(BPP) {
		case	4:
			Width /= 2;
			break;
		case	8:
			break;
		case	16:
			Width *= 2;
			break;
	}

	if(Bits) {
		for(j=0; j < Height; j++) {
			Src=((BYTE*)Bits)+j*Width*Planes;
			for(i=0; i < Width; i++) {
				for(p=0; p < Planes; p++) {
					*Dst=*(Src+Width*p);
					Dst++;
				}	
				Src++;
			}
		}
	} else {
//		for(j=0; j < Height; j++) {
//			for(i=0; i < Width; i++) {
//				for(p=0; p < Planes; p++) {
//					*Dst=0;
//					Dst++;
//				}	
//			}
//		}
	}

	return TRUE;
}


void BMPHandler::Clear(void)
{
	int i,j,p;
	int Width,Height,Planes;
	BYTE *Dst=(BYTE*)m_DIBBits;

	Height = m_BitmapInfo->bmiHeader.biHeight;
	Planes = m_BitmapInfo->bmiHeader.biPlanes;

	switch(m_BitmapInfo->bmiHeader.biBitCount) {
		case	4:
			Width = m_BitmapInfo->bmiHeader.biWidth / 2;
			break;
		case	8:
			Width = m_BitmapInfo->bmiHeader.biWidth;
			break;
		case	16:
			Width = m_BitmapInfo->bmiHeader.biWidth * 2;
			break;
	}

	for(j=0; j < Height; j++) {
		for(i=0; i < Width; i++) {
			for(p=0; p < Planes; p++) {
				*Dst=0;
				Dst++;
			}	
		}
	}
}


void *BMPHandler::CreateDC(void *hWnd)
{
	HDC BmpHdc;
	HDC hdc=GetDC((HWND)hWnd);
	assert(hdc!=NULL);

	BmpHdc = CreateCompatibleDC(hdc);
	HGDIOBJ Res = SelectObject(BmpHdc, m_DIBBitmap);

	assert(Res!=NULL);
	assert((DWORD)Res!=GDI_ERROR);
	ReleaseDC((HWND)hWnd,hdc);

	return BmpHdc;
}


void BMPHandler::DeleteDC(void *hdc)
{
	::DeleteDC((HDC)hdc);	
}



BOOL BMPHandler::WriteBMP(char *FilePath,BOOL Flip)
{
	FILE	*fid;
	BOOL Flipped=FALSE;
	int j;

	fid=fopen(FilePath,"wb");
    if(fid==NULL) {
		MessageBox( NULL, FilePath, "Unable to create file\nFile may be write protected.", MB_OK );
		return FALSE;
	}

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	bmih = m_BitmapInfo->bmiHeader;

	WORD PaletteSize;

	switch(bmih.biBitCount) {
		case	1:
			PaletteSize=2;
			break;
		case	4:
			PaletteSize=16;
			break;
		case	8:
			PaletteSize=256;
			break;
		default:
			PaletteSize=0;
	}

	((char*)&bmfh.bfType)[0] = 'B';
	((char*)&bmfh.bfType)[1] = 'M';

	bmfh.bfOffBits = PaletteSize * sizeof(RGBQUAD) + 
					sizeof(BITMAPFILEHEADER) + 
					sizeof(BITMAPINFOHEADER);

	switch(m_BitmapInfo->bmiHeader.biBitCount) {
		case 4:
			bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth/2 * abs(bmih.biHeight);
			break;
		case 8:
			bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth * abs(bmih.biHeight);
			break;
		case 16:
			bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth*2 * abs(bmih.biHeight);
			break;
	}

//	if(m_BitmapInfo->bmiHeader.biBitCount == 4) {
//		bmfh.bfSize = bmfh.bfOffBits + (bmih.biWidth/2) * abs(bmih.biHeight);
//	} else {
//		bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth * abs(bmih.biHeight);
//	}

	fwrite(&bmfh,sizeof(BITMAPFILEHEADER),1,fid);

	fwrite(&bmih,sizeof(BITMAPINFOHEADER),1,fid);

// If there's a palette then put it.
	if(PaletteSize) {
		for (int i=0; i<PaletteSize; i++)
		{
			putc(m_Palette[i].peBlue,fid);
			putc(m_Palette[i].peGreen,fid);
			putc(m_Palette[i].peRed,fid);
			putc(0,fid);
		}
	}

	if(Flip) {
		switch(bmih.biBitCount) {
			case	4:
				for(j=abs(bmih.biHeight)-1; j >= 0; j--) {
					BYTE *Src=((BYTE*)m_DIBBits)+j*bmih.biWidth/2*bmih.biPlanes;
		 			fwrite(Src, bmih.biWidth/2*bmih.biPlanes,1,fid);
				}
				break;
			case	8:
				for(j=abs(bmih.biHeight)-1; j >= 0; j--) {
					BYTE *Src=((BYTE*)m_DIBBits)+j*bmih.biWidth*bmih.biPlanes;
		 			fwrite(Src, bmih.biWidth*bmih.biPlanes,1,fid);
				}
				break;
			case	16:
				for(j=abs(bmih.biHeight)-1; j >= 0; j--) {
					BYTE *Src=((BYTE*)m_DIBBits)+j*bmih.biWidth*2*bmih.biPlanes;
		 			fwrite(Src, bmih.biWidth*2*bmih.biPlanes,1,fid);
				}
			default:
				PaletteSize=0;
		}

	} else {
	 	fwrite(m_DIBBits, (bmfh.bfSize - bmfh.bfOffBits),1,fid);
	}

	fclose(fid);

	return TRUE;
}


