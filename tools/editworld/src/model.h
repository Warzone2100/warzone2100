#ifndef __MODEL_INCLUDED__
#define	__MODEL_INCLUDED__

#include "pcxhandler.h"
#include "bmphandler.h"
#include "grdland.h"
#include "listtemp.h"
#include "chnkio.h"
#include "devmap.h"
#include "fileparse.h"
#include "dibdraw.h"

class CModel {

public:
	CModel();
	~CModel();

	BOOL ReadIMD(char *FileName,char *Description,char *TextDir,int TypeID,BOOL Flanged,BOOL Snap,int ColourKeyIndex,NORMALTYPE NType,
						 int StructureIndex,int PlayerIndex,C3DObject *Object);
	void DrawIMD(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap);
	void DrawIMDBox(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,DWORD MatID,BOOL GroundSnap,D3DCOLOR Colour);
	void DrawIMDBox2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
								int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip);
	void DrawIMDFootprint2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
								int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip);
	void DrawIMDSphere(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
						 DWORD MatID,BOOL GroundSnap,UBYTE Red,UBYTE Green,UBYTE Blue);
	void DrawIMDStats(C3DObjectInstance *Data,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);

	void RenderFlatIMD(C3DObject *Object);
	void RenderIMD(C3DObject *Object);
	void RenderTerrainMorphIMD(C3DObject *Object,D3DVECTOR *Position,D3DVECTOR *Rotation);

protected:

};

#endif
