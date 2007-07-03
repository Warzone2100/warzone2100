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

#ifndef __BMPHANDLER_INCLUDED__
#define	__BMPHANDLER_INCLUDED__

class BMPHandler {
public:
	BMPHandler(void);
	~BMPHandler(void);
	BOOL ReadBMP(char *FilePath,BOOL Flip=FALSE);
	BOOL Create(int Width,int Height,void *Bits,PALETTEENTRY *Palette,int BPP=8,BOOL Is555 = FALSE);
	void Clear(void);
	void DeleteDC(void *hdc);
	void *CreateDC(void *hWnd);
	BOOL WriteBMP(char *FilePath,BOOL Flip=FALSE);
	LONG GetBitmapWidth(void) { return(m_BitmapInfo->bmiHeader.biWidth); }
	LONG GetBitmapHeight(void) { return(abs(m_BitmapInfo->bmiHeader.biHeight)); }
	WORD GetBitmapBitCount(void) { return(m_BitmapInfo->bmiHeader.biBitCount); }
	void *GetBitmapBits(void) { return(m_DIBBits); }
	HBITMAP GetBitmap(void) { return(m_DIBBitmap); }
	PALETTEENTRY *GetBitmapPaletteEntries(void) { return(m_Palette); }
protected:
	BITMAPINFO* m_BitmapInfo;
	HBITMAP	m_DIBBitmap;
	void *m_DIBBits;
 	PALETTEENTRY *m_Palette;
};

#endif
