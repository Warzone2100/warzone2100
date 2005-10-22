//#include "stdafx.h"
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"

#include "typedefs.h"

#include "DirectX.h"
#include "Geometry.h"

#include "DebugPrint.h"
#include "DDImage.h"
#include "HeightMap.h"

#include "TileTypes.h"

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

