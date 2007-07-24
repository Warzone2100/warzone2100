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

#include <vector>

class BMPHandler
{
	public:
		BMPHandler();
		~BMPHandler();

		bool ReadBMP(char* FilePath, bool Flip = false);
		bool Create(unsigned int Width, unsigned int Height, void* Bits, unsigned int BPP = 8, bool Is555 = false);
		void Clear();

		void DeleteDC(void* hdc);
		void* CreateDC(void* hWnd);

		bool WriteBMP(char* FilePath, bool Flip = false);
		
		unsigned int GetBitmapWidth()
		{
			return _BitmapInfo->bmiHeader.biWidth;
		}
		
		unsigned int GetBitmapHeight()
		{
			return abs(_BitmapInfo->bmiHeader.biHeight);
		}

	private:
		BITMAPINFO*   _BitmapInfo;
		HBITMAP       _DIBBitmap;
		void*         _DIBBits;
		std::vector<PALETTEENTRY> _palette;
};

#endif // __BMPHANDLER_INCLUDED__
