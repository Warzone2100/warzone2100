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

#include <algorithm>

// This definition is a variation of PALETTEENTRY which provides a copy operator (operator=)
struct CopyAblePALETTEENTRY
{
	BYTE        peRed;
	BYTE        peGreen;
	BYTE        peBlue;
	BYTE        peFlags;

	CopyAblePALETTEENTRY& operator=(const PALETTEENTRY& rhs)
	{
		peRed   = rhs.peRed;
		peGreen = rhs.peGreen;
		peBlue  = rhs.peBlue;
		peFlags = rhs.peFlags;

		return *this;
	}
};

extern char sa48_static_assert[!!(sizeof(CopyAblePALETTEENTRY) == sizeof(PALETTEENTRY)) * 2 - 1];

BMPHandler::BMPHandler() :
	_BitmapInfo(NULL),
	_DIBBitmap(NULL),
	_DIBBits(NULL)
{
}

BMPHandler::~BMPHandler()
{
	DebugPrint("Deleted BMPHandler\n");

	if(_DIBBitmap != NULL)
		DeleteObject(_DIBBitmap);

	delete [] _BitmapInfo;
}

bool BMPHandler::ReadBMP(char* FilePath, bool Flip)
{
	FILE* fid = fopen(FilePath, "rb");
    if(fid == NULL)
		return false;

	_palette.clear();

	if(_DIBBitmap != NULL)
	{
		DeleteObject(_DIBBitmap);
		_DIBBitmap = NULL;
	}

	delete [] _BitmapInfo;
	_BitmapInfo = NULL;

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

// Get the BITMAPFILEHEADER structure.
	fread(&bmfh,sizeof(BITMAPFILEHEADER),1,fid);
	if (reinterpret_cast<char*>(&bmfh.bfType)[0] != 'B'
	 || reinterpret_cast<char*>(&bmfh.bfType)[1] != 'M')
	{
		fclose(fid);
		MessageBox(NULL, FilePath, "File is not a valid BMP.", MB_OK);
		return false;
	}

// Get the BITMAPINFOHEADER structure.
	fread(&bmih,sizeof(BITMAPINFOHEADER),1,fid);

	unsigned int PaletteSize;
	switch(bmih.biBitCount)
	{
		case	1:
		case	4:
		case	8:
			PaletteSize = 1 << bmih.biBitCount;
			break;
		default:
			PaletteSize = 0;
	}

	_BitmapInfo = reinterpret_cast<BITMAPINFO*>(new char[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * PaletteSize]);
	_BitmapInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	_BitmapInfo->bmiHeader.biWidth=bmih.biWidth;
	_BitmapInfo->bmiHeader.biHeight=bmih.biHeight;
	_BitmapInfo->bmiHeader.biPlanes=bmih.biPlanes;
	_BitmapInfo->bmiHeader.biBitCount=bmih.biBitCount;
	_BitmapInfo->bmiHeader.biCompression=bmih.biCompression;
	_BitmapInfo->bmiHeader.biSizeImage=bmih.biSizeImage;
	_BitmapInfo->bmiHeader.biXPelsPerMeter=bmih.biXPelsPerMeter;
	_BitmapInfo->bmiHeader.biYPelsPerMeter=bmih.biYPelsPerMeter;
	_BitmapInfo->bmiHeader.biClrUsed=bmih.biClrUsed;
	_BitmapInfo->bmiHeader.biClrImportant=bmih.biClrImportant;

// If there's a palette then get it.
	if(PaletteSize)
	{
		_palette.reserve(PaletteSize);

		for (unsigned int i = 0; i < PaletteSize; ++i)
		{
			PALETTEENTRY newEntry;
			newEntry.peBlue = static_cast<unsigned char>(getc(fid));
			newEntry.peGreen = static_cast<unsigned char>(getc(fid));
			newEntry.peRed   = static_cast<unsigned char>(getc(fid));
			getc(fid);
			newEntry.peFlags = 0;

			_palette.push_back(newEntry);
		}

		std::copy(_palette.begin(), _palette.end(), reinterpret_cast<CopyAblePALETTEENTRY*>(&_BitmapInfo->bmiColors[0]));
	}

	_DIBBitmap = CreateDIBSection(NULL, _BitmapInfo, DIB_RGB_COLORS, &_DIBBits, NULL, 0);

	if(_DIBBitmap == NULL)
	{
		MessageBox(NULL, FilePath, "Failed to create DIB.", MB_OK);
		return false;
	}

// Get the bitmap data.
 	fread(_DIBBits, (bmfh.bfSize - bmfh.bfOffBits), 1, fid);

	fclose(fid);

	if (Flip
	 && bmih.biHeight > 0)
	{
		char *Top = reinterpret_cast<char*>(_DIBBits);
		char *Bottom = Top + bmih.biWidth * (bmih.biHeight - 1);

		while(reinterpret_cast<size_t>(Top) < reinterpret_cast<size_t>(Bottom))
		{
			for(int x = 0; x < bmih.biWidth; ++x)
				std::swap(Top[x], Bottom[x]);

			Top += bmih.biWidth;
			Bottom -= bmih.biWidth;
		}
	}

	return true;
}

bool BMPHandler::Create(unsigned int Width, unsigned int Height, void *Bits, unsigned int BPP, bool Is555)
{
	const unsigned int Planes = 1;
	const unsigned int PaletteSize = 3;
	_palette.clear();

	delete [] _BitmapInfo;
	_BitmapInfo = NULL;

	_BitmapInfo= reinterpret_cast<BITMAPINFO*>(new char[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * PaletteSize]);
	_BitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	_BitmapInfo->bmiHeader.biWidth = Width;
	_BitmapInfo->bmiHeader.biHeight = Height;
	_BitmapInfo->bmiHeader.biPlanes = Planes;
	_BitmapInfo->bmiHeader.biBitCount = BPP;

	switch(BPP)
	{
		case	4:
			_BitmapInfo->bmiHeader.biCompression=BI_RGB;
			_BitmapInfo->bmiHeader.biSizeImage=(Width/2)*Height;
			break;
		case	8:
			_BitmapInfo->bmiHeader.biCompression=BI_RGB;
			_BitmapInfo->bmiHeader.biSizeImage=Width*Height;
			break;
		case	16:
			_BitmapInfo->bmiHeader.biCompression= BI_BITFIELDS;
			_BitmapInfo->bmiHeader.biSizeImage=Width*2*Height;
			break;
	}
	_BitmapInfo->bmiHeader.biXPelsPerMeter=0;
	_BitmapInfo->bmiHeader.biYPelsPerMeter=0;
	_BitmapInfo->bmiHeader.biClrUsed=0;
	_BitmapInfo->bmiHeader.biClrImportant=0;

	DWORD *PFormat = (DWORD*)_BitmapInfo->bmiColors;

	if(Is555)
	{
		PFormat[0] = 0x7c00;
		PFormat[1] = 0x03e0;
		PFormat[2] = 0x001f;
	}
	else
	{
		PFormat[0] = 0xf800;
		PFormat[1] = 0x07e0;
		PFormat[2] = 0x001f;
	}

	_DIBBitmap = CreateDIBSection(NULL, _BitmapInfo, DIB_RGB_COLORS, &_DIBBits, NULL, 0);

	if(_DIBBitmap == NULL)
	{
		MessageBox(NULL, "Error", "Failed to create DIB.", MB_OK);
		return false;
	}

	BYTE	*Dst=(BYTE*)_DIBBits;
	BYTE	*Src;

	switch(BPP)
	{
		case	4:
			Width /= 2;
			break;
		case	8:
			break;
		case	16:
			Width *= 2;
			break;
	}

	if(Bits)
	{
		for(unsigned int j = 0; j < Height; ++j)
		{
			Src = reinterpret_cast<BYTE*>(Bits) + j * Width * Planes;
			for(unsigned int i = 0; i < Width; ++i)
			{
				for(unsigned int p = 0; p < Planes; ++p)
				{
					*Dst= *(Src + Width * p);
					++Dst;
				}	
				++Src;
			}
		}
	}
//	else
//	{
//		for(unsigned int j=0; j < Height; ++j)
//		{
//			for(unsigned int i = 0; i < Width; ++i)
//			{
//				for(unsigned int p= 0; p < Planes; ++p)
//				{
//					*Dst=0;
//					++Dst;
//				}
//			}
//		}
//	}

	return true;
}

void BMPHandler::Clear()
{
	int i,j,p;
	int Width,Height,Planes;
	BYTE *Dst= (BYTE*)_DIBBits;

	Height = _BitmapInfo->bmiHeader.biHeight;
	Planes = _BitmapInfo->bmiHeader.biPlanes;

	switch(_BitmapInfo->bmiHeader.biBitCount) {
		case	4:
			Width = _BitmapInfo->bmiHeader.biWidth / 2;
			break;
		case	8:
			Width = _BitmapInfo->bmiHeader.biWidth;
			break;
		case	16:
			Width = _BitmapInfo->bmiHeader.biWidth * 2;
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

void *BMPHandler::CreateDC(void* hWnd)
{
	HDC BmpHdc;
	HDC hdc=GetDC((HWND)hWnd);
	assert(hdc != NULL);

	BmpHdc = CreateCompatibleDC(hdc);
	HGDIOBJ Res = SelectObject(BmpHdc, _DIBBitmap);

	assert(Res != NULL);
	assert((DWORD)Res != GDI_ERROR);
	ReleaseDC((HWND)hWnd, hdc);

	return BmpHdc;
}


void BMPHandler::DeleteDC(void *hdc)
{
	::DeleteDC((HDC)hdc);	
}

bool BMPHandler::WriteBMP(char *FilePath, bool Flip)
{
	FILE* fid = fopen(FilePath,"wb");
    if(fid == NULL)
	{
		MessageBox( NULL, FilePath, "Unable to create file\nFile may be write protected.", MB_OK );
		return false;
	}

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	bmih = _BitmapInfo->bmiHeader;

	unsigned int PaletteSize;
	switch(bmih.biBitCount)
	{
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

	reinterpret_cast<char*>(&bmfh.bfType)[0] = 'B';
	reinterpret_cast<char*>(&bmfh.bfType)[1] = 'M';

	bmfh.bfOffBits = PaletteSize * sizeof(RGBQUAD) + 
	                 sizeof(BITMAPFILEHEADER) + 
	                 sizeof(BITMAPINFOHEADER);

	switch(_BitmapInfo->bmiHeader.biBitCount)
	{
		case 4:
			bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth / 2 * abs(bmih.biHeight);
			break;
		case 8:
			bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth * abs(bmih.biHeight);
			break;
		case 16:
			bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth * 2 * abs(bmih.biHeight);
			break;
	}

//	if(_BitmapInfo->bmiHeader.biBitCount == 4) {
//		bmfh.bfSize = bmfh.bfOffBits + (bmih.biWidth/2) * abs(bmih.biHeight);
//	} else {
//		bmfh.bfSize = bmfh.bfOffBits + bmih.biWidth * abs(bmih.biHeight);
//	}

	fwrite(&bmfh,sizeof(BITMAPFILEHEADER),1,fid);

	fwrite(&bmih,sizeof(BITMAPINFOHEADER),1,fid);

	// If there's a palette then put it.
	if(PaletteSize)
	{
		for (std::vector<PALETTEENTRY>::const_iterator curPaletteEntry = _palette.begin(); curPaletteEntry != _palette.end(); ++curPaletteEntry)
		{
			putc(curPaletteEntry->peBlue, fid);
			putc(curPaletteEntry->peGreen, fid);
			putc(curPaletteEntry->peRed, fid);
			putc(0, fid);
		}
	}

	if (Flip)
	{
		int j; // Declared here instead of in the for declarations since MSVC's scoping sucks!

		switch(bmih.biBitCount) {
			case 4:
				for (j = abs(bmih.biHeight) - 1; j >= 0; --j)
				{
					char* Src = reinterpret_cast<char*>(_DIBBits) + j * bmih.biWidth / 2 * bmih.biPlanes;
		 			fwrite(Src, bmih.biWidth / 2 * bmih.biPlanes, 1, fid);
				}
				break;

			case 8:
				for (j = abs(bmih.biHeight) - 1; j >= 0; --j)
				{
					char* Src = reinterpret_cast<char*>(_DIBBits) + j * bmih.biWidth *bmih.biPlanes;
		 			fwrite(Src, bmih.biWidth * bmih.biPlanes, 1, fid);
				}
				break;

			case 16:
				for (j = abs(bmih.biHeight) - 1; j >= 0; --j)
				{
					char* Src = reinterpret_cast<char*>(_DIBBits) + j * bmih.biWidth * 2 * bmih.biPlanes;
		 			fwrite(Src, bmih.biWidth * 2 * bmih.biPlanes, 1, fid);
				}
		}
	}
	else
	{
	 	fwrite(_DIBBits, (bmfh.bfSize - bmfh.bfOffBits), 1, fid);
	}

	fclose(fid);

	return true;
}
