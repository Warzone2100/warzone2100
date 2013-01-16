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
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"

#include "typedefs.h"

#include "directx.h"
#include "geometry.h"

#include "debugprint.hpp"
#include "ddimage.h"
#include "heightmap.h"

#include "tiletypes.h"

BOOL g_OverlayTypes=FALSE;

void DisplayTerrainType(CDIBDraw *DIBDraw,int XPos,int YPos,int Width,int Height,DWORD Flags)
{
	if(g_OverlayTypes) {
		COLORREF Colour;

//		switch(Flags&TF_TYPEALL) {
		switch(Flags) {
			case TF_TYPESAND:
				Colour = RGB(255,255,0);
				break;
			case TF_TYPESANDYBRUSH:
				Colour = RGB(136,136,70);
				break;
			case TF_TYPEBAKEDEARTH:
				Colour = RGB(67,67,35);
				break;
			case TF_TYPEGREENMUD:
				Colour = RGB(0,128,0);
				break;
			case TF_TYPEREDBRUSH:
				Colour = RGB(255,0,0);
				break;
			case TF_TYPEPINKROCK:
				Colour = RGB(255,128,128);
				break;
			case TF_TYPEROAD:
				Colour = RGB(0,0,0);
				break;
			case TF_TYPEWATER:
				Colour = RGB(0,0,255);
				break;
			case TF_TYPECLIFFFACE:
				Colour = RGB(128,128,128);
				break;
			case TF_TYPERUBBLE:
				Colour = RGB(64,64,64);
				break;
			case TF_TYPESHEETICE:
				Colour = RGB(255,255,255);
				break;
			case TF_TYPESLUSH:
				Colour = RGB(196,196,196);
				break;

//			case TF_TYPESAND:
//				Colour = RGB(255,255,0);
//				break;
//			case TF_TYPEGRASS:
//				Colour = RGB(0x00,0x77,0x00);
//				break;
//			case TF_TYPESTONE:
//				Colour = RGB(0x77,0x77,0x77);
//				break;
//			case TF_TYPEWATER:
//				Colour = RGB(0x00,0x00,0xFF);
//				break;
//			case TF_TYPEWATER:
//				Colour = RGB(0x00,0x00,0xFF);
//				break;
			default:
				Colour = RGB(255,0,255);
		}

		HDC hdc = (HDC)DIBDraw->GetDIBDC();
		HBRUSH Brush = CreateSolidBrush(Colour);
		RECT Rect;

		int Rad = (8*Width)/64;

		SetRect(&Rect,XPos+(Width/2)-Rad,YPos+(Height/2)-Rad,XPos+(Width/2)+Rad,YPos+(Height/2)+Rad);
		FillRect(hdc,&Rect,Brush);
		DeleteObject(Brush);
	}
}

