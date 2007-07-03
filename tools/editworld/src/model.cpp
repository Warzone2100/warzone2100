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
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "math.h"
#include "typedefs.h"
#include "debugprint.h"

#include "directx.h"
#include "geometry.h"

#include "ddimage.h"
#include "tiletypes.h"

#include "assert.h"
#include "undoredo.h"

#include "model.h"

CModel::CModel()
{
}

CModel::~CModel()
{
}


BOOL CModel::ReadIMD(char *FileName,char *Description,char *TextDir,int TypeID,BOOL Flanged,BOOL Snap,int ColourKeyIndex,NORMALTYPE NType,
					 int StructureIndex,int PlayerIndex,C3DObject *Object)
{
	return TRUE;
}

void CModel::DrawIMD(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
					 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap)
{
}

void CModel::DrawIMDBox(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
					 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,DWORD MatID,BOOL GroundSnap,D3DCOLOR Colour)
{
}

void CModel::DrawIMDBox2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
							int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip)
{
}

void CModel::DrawIMDFootprint2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
							int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip)
{
}

void CModel::DrawIMDSphere(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
					 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
					 DWORD MatID,BOOL GroundSnap,UBYTE Red,UBYTE Green,UBYTE Blue)
{
}

void CModel::DrawIMDStats(C3DObjectInstance *Data,
					 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
}

void CModel::RenderFlatIMD(C3DObject *Object)
{
}

void CModel::RenderIMD(C3DObject *Object)
{
}

void CModel::RenderTerrainMorphIMD(C3DObject *Object,D3DVECTOR *Position,D3DVECTOR *Rotation)
{
}



