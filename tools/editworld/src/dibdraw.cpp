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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <assert.h>

#include "dibdraw.h"
#include "debugprint.hpp"


// Create a DIBDraw class with user specified dimensions and colour depth.
//
CDIBDraw::CDIBDraw(void *hWnd,DWORD Width,DWORD Height,DWORD Planes,DWORD BitsPerPixel,BOOL Is555)
{
// Create the DIB.
	CreateDIB(hWnd,Width,Height,Planes,BitsPerPixel,Is555);
}

// Create a DIBDraw class with user specified dimensions and colour depth of the
// specified window.
//
CDIBDraw::CDIBDraw(void *hWnd,DWORD Width,DWORD Height,BOOL Is555)
{
	DWORD	Planes;
	DWORD	BitsPerPixel;
	HDC		hdc;

	assert(hWnd!=NULL);

// Find out about the client windows pixel mode.
	hdc=GetDC((HWND)hWnd);
	assert(hdc!=NULL);

	Planes = GetDeviceCaps(hdc,PLANES) ;
	BitsPerPixel = GetDeviceCaps(hdc,BITSPIXEL);
	ReleaseDC((HWND)hWnd,hdc);

// Create the DIB.
	CreateDIB(hWnd,Width,Height+4,Planes,BitsPerPixel,Is555);
}

// Create a CDIBDraw instances with the dimensions and colour depth of the
// specified window.
//
CDIBDraw::CDIBDraw(void *hWnd,BOOL Is555)
{
	DWORD	Planes;
	DWORD	BitsPerPixel;
	RECT	ClientRect;
	HDC		hdc;

// Find out about the client windows pixel mode.
	hdc=GetDC((HWND)hWnd);
	Planes = GetDeviceCaps(hdc,PLANES);
	BitsPerPixel = GetDeviceCaps(hdc,BITSPIXEL);
	ReleaseDC((HWND)hWnd,hdc);

// Get the size of the client window.
	GetClientRect((HWND)hWnd,&ClientRect);

// Create the DIB.
	CreateDIB(hWnd,(ClientRect.right+1)&0xfffe,
					ClientRect.bottom+4,		// Add 4 for some strange reason.
					Planes,BitsPerPixel);
}

void CDIBDraw::CreateDIB(void *hWnd,DWORD Width,DWORD Height,DWORD Planes,DWORD BitsPerPixel,BOOL Is555)
{
	DWORD	PaletteSize;
	HDC		hdc;

	assert(hWnd!=NULL);

	m_hWnd = (HWND)hWnd;
	m_Width = Width;
	m_Height = Height;

// Set the palette size.
	switch(Planes * BitsPerPixel) {
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
			PaletteSize=3;
	}

	assert(PaletteSize == 3);

// Create the bitmap info structure.
	m_BitmapInfo=(BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)+sizeof(RGBQUAD)*PaletteSize];
	m_BitmapInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	m_BitmapInfo->bmiHeader.biWidth=m_Width;
	m_BitmapInfo->bmiHeader.biHeight=-(int)m_Height;	// Top down please.
	m_BitmapInfo->bmiHeader.biPlanes=(WORD)Planes;
	m_BitmapInfo->bmiHeader.biBitCount=(WORD)BitsPerPixel;
	m_BitmapInfo->bmiHeader.biCompression= BI_BITFIELDS;	//BI_RGB;
	m_BitmapInfo->bmiHeader.biSizeImage=0;
	m_BitmapInfo->bmiHeader.biXPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biYPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biClrUsed=0;
	m_BitmapInfo->bmiHeader.biClrImportant=0;
// Specify rgb bit masks.
	DWORD *PFormat = (DWORD*)m_BitmapInfo->bmiColors;

	m_Is555 = Is555;
	if(Is555) {
		PFormat[0] = 0x7c00;
		PFormat[1] = 0x03e0;
		PFormat[2] = 0x001f;
	} else {
		PFormat[0] = 0xf800;
		PFormat[1] = 0x07e0;
		PFormat[2] = 0x001f;
	}

	hdc=GetDC((HWND)hWnd);
	assert(hdc!=NULL);

	m_hDIB = CreateDIBSection(hdc, m_BitmapInfo, DIB_RGB_COLORS, &m_DIBBits, NULL, 0);
	assert(m_hDIB!=NULL);
	assert(m_DIBBits!=NULL);

	m_hdcDIB = CreateCompatibleDC(hdc);
	HGDIOBJ Res = SelectObject(m_hdcDIB, m_hDIB);

	assert(Res!=NULL);
	assert((DWORD)Res!=GDI_ERROR);
	ReleaseDC((HWND)hWnd,hdc);

	m_Valid = MAGIC;
}

CDIBDraw::~CDIBDraw(void)
{
	assert(m_Valid == MAGIC);
 	m_Valid = 0;
	DeleteDC(m_hdcDIB);	
	DeleteObject(m_hDIB);
	delete m_BitmapInfo;
}

// Clear the DIB Section to black.
//
void CDIBDraw::ClearDIB(void)
{
	assert(m_Valid == MAGIC);
	BitBlt(m_hdcDIB, 0, 0, m_Width, m_Height,
			   NULL, 0, 0, BLACKNESS);
}

// Copy the DIB section to it's client window.
//
void CDIBDraw::SwitchBuffers(void)
{
//	RECT DestRect;
//	GetClientRect(m_hWnd,&DestRect);

	assert(m_Valid == MAGIC);
	HDC hdc;
	hdc=GetDC(m_hWnd);

// 	BitBlt(hdc, 0, 0, DestRect.right, DestRect.bottom,
//			   m_hdcDIB, 0, 0, SRCCOPY);

	BitBlt(hdc, 0, 0, m_Width, m_Height,
			   m_hdcDIB, 0, 0, SRCCOPY);

	ReleaseDC(m_hWnd,hdc);

	ValidateRect(m_hWnd, NULL);
}

void CDIBDraw::SwitchBuffers(int SrcOffX,int SrcOffY)
{
	assert(m_Valid == MAGIC);
	HDC hdc;
	hdc=GetDC(m_hWnd);

	BitBlt(hdc, 0, 0, m_Width, m_Height,
			   m_hdcDIB, SrcOffX, SrcOffY, SRCCOPY);

	ReleaseDC(m_hWnd,hdc);

	ValidateRect(m_hWnd, NULL);
}

// Copy the DIB Section to the specified DC
//
void CDIBDraw::SwitchBuffers(void *hdc)
{
	assert(m_Valid == MAGIC);
	BitBlt((HDC)hdc, 0, 0, m_Width, m_Height,
			   m_hdcDIB, 0, 0, SRCCOPY);

	ValidateRect(m_hWnd, NULL);
}

void *CDIBDraw::GetDIBBits(void)
{
	assert(m_Valid == MAGIC);
	return m_DIBBits;
}

DWORD CDIBDraw::GetDIBWidth(void)
{
	assert(m_Valid == MAGIC);
	return m_Width;
}

DWORD CDIBDraw::GetDIBHeight(void)
{
	assert(m_Valid == MAGIC);
	return m_Height;
}

void *CDIBDraw::GetDIBDC(void)
{
	assert(m_Valid == MAGIC);
	return m_hdcDIB;
}

void *CDIBDraw::GetDIBWindow(void)
{
	assert(m_Valid == MAGIC);
	return m_hWnd;
}


// Given and RGB colour, return a pixel value which can be
// written directly into the DIB section.
//
DWORD CDIBDraw::GetDIBValue(DWORD r,DWORD g,DWORD b)
{
	if(m_Is555) {
		return (((r>>3)&0x1f) << 10) | (((g>>3)&0x1f) << 5) | ((b>>3)&0x1f);
	} else {
		return (((r>>3)&0x1f) << 11) | (((g>>2)&0x3f) << 5) | ((b>>3)&0x1f);
	}
}


void CDIBDraw::PutDIBPixel(DWORD Value,DWORD x,DWORD y)
{
	if((x >= 0) && (y >= 0) && (x < m_Width) && (y < m_Height)) {
		short *Dst = ((short*)m_DIBBits) + y*m_Width+x;
		*Dst = Value;
	}
}


void CDIBDraw::PutDIBFatPixel(DWORD Value,DWORD x,DWORD y)
{
	if((x >= 0) && (y >= 0) && (x < m_Width-1) && (y < m_Height-1)) {
		short *Dst = ((short*)m_DIBBits) + y*m_Width+x;
		*Dst = Value;
		*(Dst+1) = Value;
		*(Dst+m_Width) = Value;
		*(Dst+1+m_Width) = Value;
	}
}


