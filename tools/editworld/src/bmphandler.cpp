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
#include "winstuff.h"
#include "stdio.h"
#include "assert.h"
#include "debugprint.hpp"
#include "bmphandler.h"

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

bool BMPHandler::Create(unsigned int Width, unsigned int Height, unsigned int BPP)
{
	const unsigned int Planes = 1;
	const unsigned int PaletteSize = 3;

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

	// Predefined palette
	_BitmapInfo->bmiColors[0].rgbBlue     = 0;
	_BitmapInfo->bmiColors[0].rgbGreen    = 0xf8;
	_BitmapInfo->bmiColors[0].rgbRed      = 0;
	_BitmapInfo->bmiColors[0].rgbReserved = 0;

	_BitmapInfo->bmiColors[1].rgbBlue     = 0xe0;
	_BitmapInfo->bmiColors[1].rgbGreen    = 0x07;
	_BitmapInfo->bmiColors[1].rgbRed      = 0;
	_BitmapInfo->bmiColors[1].rgbReserved = 0;

	_BitmapInfo->bmiColors[2].rgbBlue     = 0x1f;
	_BitmapInfo->bmiColors[2].rgbGreen    = 0;
	_BitmapInfo->bmiColors[2].rgbRed      = 0;
	_BitmapInfo->bmiColors[2].rgbReserved = 0;

	_DIBBitmap = CreateDIBSection(NULL, _BitmapInfo, DIB_RGB_COLORS, &_DIBBits, NULL, 0);

	if(_DIBBitmap == NULL)
	{
		MessageBox(NULL, "Error", "Failed to create DIB.", MB_OK);
		return false;
	}

	BYTE	*Dst=(BYTE*)_DIBBits;

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

//	for(unsigned int j=0; j < Height; ++j)
//	{
//		for(unsigned int i = 0; i < Width; ++i)
//		{
//			for(unsigned int p= 0; p < Planes; ++p)
//			{
//				*Dst=0;
//				++Dst;
//			}
//		}
//	}

	return true;
}

HDC BMPHandler::CreateDC(HWND hWnd)
{
	HDC BmpHdc;
	HDC hdc = GetDC(hWnd);
	assert(hdc != NULL);

	BmpHdc = CreateCompatibleDC(hdc);
	HGDIOBJ Res = SelectObject(BmpHdc, _DIBBitmap);

	assert(Res != NULL);
	assert((DWORD)Res != GDI_ERROR);
	ReleaseDC(hWnd, hdc);

	return BmpHdc;
}


void BMPHandler::DeleteDC(HDC hdc)
{
	::DeleteDC(hdc);	
}

bool BMPHandler::WriteBMP(char *FilePath)
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

	reinterpret_cast<char*>(&bmfh.bfType)[0] = 'B';
	reinterpret_cast<char*>(&bmfh.bfType)[1] = 'M';

	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + 
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

	// Write file header
	fwrite(&bmfh,sizeof(BITMAPFILEHEADER),1,fid);

	// Write bitmap-info header
	fwrite(&bmih,sizeof(BITMAPINFOHEADER),1,fid);

	// Write bitmap data
 	fwrite(_DIBBits, (bmfh.bfSize - bmfh.bfOffBits), 1, fid);

	fclose(fid);

	return true;
}
