//#include "stdafx.h"
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "math.h"
#include "typedefs.h"
#include "DebugPrint.h"

#include "DirectX.h"
#include "Geometry.h"

#include "DDImage.h"
#include "TileTypes.h"

#include "assert.h"
#include "UndoRedo.h"

#include "Model.h"

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



