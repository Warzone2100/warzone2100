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

#ifndef __INCLUDED_DIBDRAW_H__
#define __INCLUDED_DIBDRAW_H__

#define MAGIC 29121967	// For debuging.

#include <windows.h>

class CDIBDraw {
public:
	CDIBDraw(void *hWnd,DWORD Width,DWORD Height,DWORD Planes,DWORD BitsPerPixel,BOOL Is555=FALSE);
	CDIBDraw(void *hWnd,DWORD Width,DWORD Height,BOOL Is555=FALSE);
	CDIBDraw(void *hWnd,BOOL Is555=FALSE);
	~CDIBDraw(void);
	void CreateDIB(void *hWnd,DWORD Width,DWORD Height,DWORD Planes,DWORD BitsPerPixel,BOOL Is555=FALSE);
	void ClearDIB(void);
	void SwitchBuffers(void);
	void SwitchBuffers(void *hdc);
	void SwitchBuffers(int SrcOffX,int SrcOffY);
	void *GetDIBBits(void);
	DWORD GetDIBWidth(void);
	DWORD GetDIBHeight(void);
	void *GetDIBDC(void);
	void *GetDIBWindow(void);
	DWORD GetDIBValue(DWORD r,DWORD g,DWORD b);
	void PutDIBPixel(DWORD Value,DWORD x,DWORD y);
	void PutDIBFatPixel(DWORD Value,DWORD x,DWORD y);
protected:
	BOOL m_Is555;
	DWORD m_Valid;
	HWND m_hWnd;
	BITMAPINFO *m_BitmapInfo;
	void *m_DIBBits;
	HBITMAP m_hDIB;
	HDC m_hdcDIB;
	DWORD m_Width;
	DWORD m_Height;
};

#endif // __INCLUDED_DIBDRAW_H__
