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

#ifndef __INCLUDED_PCXHANDLER_H__
#define __INCLUDED_PCXHANDLER_H__

#include <windows.h>
#include <istream>
#include <ostream>

// Returns true if the given value is a power of two
static inline bool IsPower2(int value)
{
	return value > 0
	    && (value & (value - 1)) == 0;
}

// Round the given value up to the nearest power of 2.
int Power2(int value);

struct PCXHeader
{
	BYTE	Manufacturer;   // 1   Constant Flag  10 = ZSoft .PCX
	BYTE	Version;        // 1   Version information:
	                        // 0 = Version 2.5
	                        // 2 = Version 2.8 w/palette information
	                        // 3 = Version 2.8 w/o palette information
	                        // 5 = Version 3.0
	BYTE	Encoding;       // 1   1 = .PCX run length encoding
	BYTE	BitsPerPixel;	// 1   Number of bits/pixel per plane
	WORD	Window[4];      // 8   Picture Dimensions 
	                        // (Xmin, Ymin) - (Xmax - Ymax)
	                        // in pixels, inclusive
	WORD	HRes;           // 2   Horizontal Resolution of creating device
	WORD	VRes;           // 2   Vertical Resolution of creating device
	BYTE	Colormap[48];   // 48  Color palette setting, see text
	BYTE	Reserved;       // 1
	BYTE	NPlanes;        // 1   Number of color planes
	WORD	BytesPerLine;	// 2   Number of bytes per scan line per 
	                        // color plane (always even for .PCX files)
	WORD	PaletteInfo;	// 2   How to interpret palette - 1 = color/BW,
	                        // 2 = grayscale
	BYTE	Filler[58];     // 58  blank to fill out 128 byte header
};


#define BMR_ROUNDUP	1

class PCXHandler {
	public:
		PCXHandler();
		~PCXHandler();

		bool Create(int Width,int Height,void *Bits,PALETTEENTRY *Palette);
		bool ReadPCX(std::istream& input, DWORD Flags = 0);
		bool WritePCX(std::ostream& output);

		inline unsigned int GetBitmapWidth()
		{
			return(_BitmapInfo->bmiHeader.biWidth);
		}

		inline unsigned int GetBitmapHeight()
		{
			return(abs(_BitmapInfo->bmiHeader.biHeight));
		}

		inline unsigned int GetBitmapBitCount()
		{
			return(_BitmapInfo->bmiHeader.biBitCount);
		}

		inline void* GetBitmapBits()
		{
			return(_DIBBits);
		}

		inline PALETTEENTRY* GetBitmapPaletteEntries()
		{
			return(_Palette);
		}

	private:
		PCXHeader     _Header;
		BITMAPINFO*   _BitmapInfo;
		HBITMAP       _DIBBitmap;
		void*         _DIBBits;
 		PALETTEENTRY* _Palette;
};

#endif // __INCLUDED_PCXHANDLER_H__
