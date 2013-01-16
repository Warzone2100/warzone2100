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

#ifndef __INCLUDED_GEOMETRY_H__
#define __INCLUDED_GEOMETRY_H__

#include <ddraw.h>
#include <d3d.h>
#include "typedefs.h"
#include "macros.h"
#include "directx.h"

#define VecZero(v)		( (v)->x = (v)->y = (v)->z = 0.0 )
#define VecLength(v)	( sqrt((v)->x * (v)->x + (v)->y * (v)->y + (v)->z * (v)->z) )
#define VecDot(u,v)		( (u)->x * (v)->x + (u)->y * (v)->y + (u)->z * (v)->z )
#define VecInc(u,v)		( (u)->x += (v)->x, (u)->y += (v)->y, (u)->z += (v)->z )
#define VecSub(a,b,c)	( (c)->x = (a)->x - (b)->x, (c)->y = (a)->y - (b)->y, (c)->z = (a)->z - (b)->z )
#define VecAdd(a,b,c)	( (c)->x = (a)->x + (b)->x, (c)->y = (a)->y + (b)->y, (c)->z = (a)->z + (b)->z )
#define VecCopy(a,b)	( (a)->x = (b)->x, (a)->y = (b)->y, (a)->z = (b)->z )
#define VecDiv(u,d)		( (u)->x /= (d), (u)->y /= (d), (u)->z /= (d) )

#define MAXLIGHTS 64

struct LightSource {
	int InUse;
	BOOL Transform;
	D3DVECTOR Position;
	D3DVECTOR Direction;
    D3DLIGHT LightDesc;
	LPDIRECT3DLIGHT Light;
};

struct VECTOR2D {
	float x;
	float y;
};

#define	MAXMATRIXSTACKSIZE 8
#define M_PI (3.14159265358979323846)
#define RADIANS(Degs) (float)(((Degs)/360) * (M_PI * 2.0))
#define DEGREES(Rads) (float)(((Rads)*360) / (M_PI * 2.0))

class CGeometry {
	public:
		CGeometry(ID3D *Direct3D,ID3DDEVICE *Device,ID3DVIEPORT *Viewport);
		~CGeometry(void);
		void SetTransState(void);
		void GetIdentity(D3DMATRIX *Matrix) { *Matrix = m_identity; }
		void ConcatenateXRotation(LPD3DMATRIX lpM, float Degrees );
		void ConcatenateYRotation(LPD3DMATRIX lpM, float Degrees );
		void ConcatenateZRotation(LPD3DMATRIX lpM, float Degrees );
		LPD3DVECTOR D3DVECTORNormalise(LPD3DVECTOR v);
		LPD3DVECTOR D3DVECTORCrossProduct(LPD3DVECTOR lpd, LPD3DVECTOR lpa, LPD3DVECTOR lpb);
		LPD3DMATRIX MultiplyD3DMATRIX(LPD3DMATRIX lpDst, LPD3DMATRIX lpSrc1, LPD3DMATRIX lpSrc2);
		LPD3DMATRIX D3DMATRIXInvert(LPD3DMATRIX d, LPD3DMATRIX a);
		LPD3DMATRIX D3DMATRIXSetRotation(LPD3DMATRIX lpM, LPD3DVECTOR lpD, LPD3DVECTOR lpU);
		void DirectionVector(D3DVECTOR &Rotation,D3DVECTOR &Direction);
		void RotateVector2D(VECTOR2D *Vector,VECTOR2D *TVector,VECTOR2D *Pos,float Angle,int Count);
		BOOL RotateTranslateProject(D3DVECTOR *Vector,D3DVECTOR *Result);
		void RotateVector(D3DVECTOR *Vector,D3DVECTOR *Result);
		void spline(LPD3DVECTOR p, float t, LPD3DVECTOR p1, LPD3DVECTOR p2, LPD3DVECTOR p3, LPD3DVECTOR p4);
		void CalcNormal(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Normal);
		void CalcPlaneEquation(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Normal,float *Offset);
		void SetWorldMatrix(D3DVECTOR *CameraRotation);
		void PushWorldMatrix(void);
		void PopWorldMatrix(void);
		void PushMatrix(D3DMATRIX &Matrix);
		void PopMatrix(D3DMATRIX &Matrix);
		void SetObjectMatrix(D3DVECTOR *ObjectRotation,D3DVECTOR *ObjectPosition,D3DVECTOR *CameraPosition);
		void SetObjectScale(D3DVECTOR *Vector);
		void MulObjectMatrix(D3DMATRIX *Matrix);
		BOOL SetTransformation(void);
		void SetTranslation(LPD3DMATRIX Matrix,D3DVECTOR *Vector);
		void SetScale(LPD3DMATRIX Matrix,D3DVECTOR *Vector);
		BOOL TransformVertex(D3DVERTEX *Dest,D3DVERTEX *Source,DWORD NumVertices,D3DHVERTEX *HVertex=NULL,BOOL Clipped=TRUE);
		void InverseTransformVertex(D3DVECTOR *Dest,D3DVECTOR *Source);
		void Motion(D3DVECTOR &Position,D3DVECTOR &Angle,float Speed);
		void LookAt(D3DVECTOR &Target,D3DVECTOR &Position,D3DVECTOR &Rotation);
		float Orbit(D3DVECTOR &Position,D3DVECTOR &Rotation,D3DVECTOR &Center,float Angle,float Radius,float Step);
		void FindAverageVec(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Average);
		BOOL PointInFace(float Xpos,float YPos,D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2);
		DWORD CheckEdge(D3DVECTOR *v0,D3DVECTOR *v1,float x,float y,DWORD l);

		int AddLight(DWORD Type);
		void SetTransform(int LightID,BOOL Transform);
		void SetLightPosition(int LightID,D3DVECTOR *Position);
		void SetLightDirection(int LightID,D3DVECTOR *Rotation);
		void SetLightColour(int LightID,D3DCOLORVALUE *Colour);
		void SetLightRange(int LightID,float Range);
		void SetLightFalloff(int LightID,float Falloff);
		void SetLightCone(int LightID,float Umbra,float Penumbra);
		void SetLightAttenuation(int LightID,float Att0,float Att1,float Att2);
		BOOL RemoveLight(int LightID);
		BOOL TransformLights(D3DVECTOR *CameraRotation,D3DVECTOR *CameraPosition);

	private:
		LightSource m_Lights[MAXLIGHTS];

		D3DMATRIXHANDLE m_hProj, m_hView, m_hWorld;
		D3DMATRIX m_proj;
		D3DMATRIX m_view;
		D3DMATRIX m_identity;
		D3DMATRIX m_world;
		D3DMATRIX m_spin;

		ID3D				*m_Direct3D;
		ID3DDEVICE			*m_3DDevice;
		ID3DVIEPORT			*m_Viewport;

		int m_MatrixStackPos;
		D3DMATRIX m_MatrixStack[MAXMATRIXSTACKSIZE];
};

#endif // __INCLUDED_GEOMETRY_H__
