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
#include <math.h>

#include "debugprint.hpp"
#include "geometry.h"
#include "directx.h"


float Distance2D(float x,float y)
{
	float Dist = sqrt(x*x + y*y);

	return Dist;
}

float Distance3D(float x,float y,float z)
{
	return sqrt(x*x + y*y + z*z);
}


CGeometry::CGeometry(ID3D *Direct3D,ID3DDEVICE *Device,ID3DVIEPORT *Viewport)
{
	// Matrix for projecting 3d view onto a 2d screen surface
	D3DMATRIX InitProj = {
		D3DVAL(2.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(2.0), D3DVAL(0.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(1.0),
		D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-512.0), D3DVAL(0.0)
//		D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-8.0), D3DVAL(0.0)
	};
	D3DMATRIX InitView = {
		D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(0.0), D3DVAL(10.0), D3DVAL(1.0)
	};
	D3DMATRIX InitIdentity = {
		D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
		D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0)
	};


	m_Direct3D = Direct3D;
	m_3DDevice = Device;
	m_Viewport = Viewport;
	m_proj=InitProj;
	m_view=InitView;
	m_identity=InitIdentity;

// Set the view, world and projection matrices
// Create a buffer for matrix set commands etc.

    m_3DDevice->BeginScene();
	m_3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &m_proj);
	m_3DDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &m_view);      
	m_3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &m_world);    
    m_3DDevice->EndScene();

	for(int i=0; i<MAXLIGHTS; i++) {
		m_Lights[i].InUse = 0;
	}

	m_MatrixStackPos = 0;
}

CGeometry::~CGeometry()
{
	for(int i=0; i<MAXLIGHTS; i++) {
	   	RemoveLight(i);
	}
}

void CGeometry::ConcatenateXRotation(LPD3DMATRIX lpM, float Degrees )
{
	float Temp01, Temp11, Temp21, Temp31;
	float Temp02, Temp12, Temp22, Temp32;
	float aElements[4][4];

	float Radians = (float)((Degrees/360) * M_PI * 2.0);

	float Sin = (float)sin(Radians), Cos = (float)cos(Radians);

	memcpy(aElements, lpM, sizeof(D3DMATRIX));
	Temp01 = aElements[0][1] * Cos + aElements[0][2] * Sin;
	Temp11 = aElements[1][1] * Cos + aElements[1][2] * Sin;
	Temp21 = aElements[2][1] * Cos + aElements[2][2] * Sin;
	Temp31 = aElements[3][1] * Cos + aElements[3][2] * Sin;

	Temp02 = aElements[0][1] * -Sin + aElements[0][2] * Cos;
	Temp12 = aElements[1][1] * -Sin + aElements[1][2] * Cos;
	Temp22 = aElements[2][1] * -Sin + aElements[2][2] * Cos;
	Temp32 = aElements[3][1] * -Sin + aElements[3][2] * Cos;

	lpM->_12 = Temp01;
	lpM->_22 = Temp11;
	lpM->_32 = Temp21;
	lpM->_42 = Temp31;
	lpM->_13 = Temp02;
	lpM->_23 = Temp12;
	lpM->_33 = Temp22;
	lpM->_43 = Temp32;
}

void CGeometry::ConcatenateYRotation(LPD3DMATRIX lpM, float Degrees )
{
	float Temp00, Temp10, Temp20, Temp30;
	float Temp02, Temp12, Temp22, Temp32;
	float aElements[4][4];

	float Radians = (float)((Degrees/360) * M_PI * 2);

	float Sin = (float)sin(Radians), Cos = (float)cos(Radians);

	memcpy(aElements, lpM, sizeof(D3DMATRIX));
	Temp00 = aElements[0][0] * Cos + aElements[0][2] * -Sin;
	Temp10 = aElements[1][0] * Cos + aElements[1][2] * -Sin;
	Temp20 = aElements[2][0] * Cos + aElements[2][2] * -Sin;
	Temp30 = aElements[3][0] * Cos + aElements[3][2] * -Sin;

	Temp02 = aElements[0][0] * Sin + aElements[0][2] * Cos;
	Temp12 = aElements[1][0] * Sin + aElements[1][2] * Cos;
	Temp22 = aElements[2][0] * Sin + aElements[2][2] * Cos;
	Temp32 = aElements[3][0] * Sin + aElements[3][2] * Cos;

	lpM->_11 = Temp00;
	lpM->_21 = Temp10;
	lpM->_31 = Temp20;
	lpM->_41 = Temp30;
	lpM->_13 = Temp02;
	lpM->_23 = Temp12;
	lpM->_33 = Temp22;
	lpM->_43 = Temp32;
}

void CGeometry::ConcatenateZRotation(LPD3DMATRIX lpM, float Degrees )
{
  float Temp00, Temp10, Temp20, Temp30;
  float Temp01, Temp11, Temp21, Temp31;
  float aElements[4][4];

  float Radians = (float)((Degrees/360) * M_PI * 2);

  float Sin = (float)sin(Radians), Cos = (float)cos(Radians);

  memcpy(aElements, lpM, sizeof(D3DMATRIX));
  Temp00 = aElements[0][0] * Cos + aElements[0][1] * Sin;
  Temp10 = aElements[1][0] * Cos + aElements[1][1] * Sin;
  Temp20 = aElements[2][0] * Cos + aElements[2][1] * Sin;
  Temp30 = aElements[3][0] * Cos + aElements[3][1] * Sin;

  Temp01 = aElements[0][0] * -Sin + aElements[0][1] * Cos;
  Temp11 = aElements[1][0] * -Sin + aElements[1][1] * Cos;
  Temp21 = aElements[2][0] * -Sin + aElements[2][1] * Cos;
  Temp31 = aElements[3][0] * -Sin + aElements[3][1] * Cos;

  lpM->_11 = Temp00;
  lpM->_21 = Temp10;
  lpM->_31 = Temp20;
  lpM->_41 = Temp30;
  lpM->_12 = Temp01;
  lpM->_22 = Temp11;
  lpM->_32 = Temp21;
  lpM->_42 = Temp31;
}

/*
 * Normalises the vector v
 */
LPD3DVECTOR CGeometry::D3DVECTORNormalise(LPD3DVECTOR v)
{
    float vx, vy, vz, inv_mod;

    vx = v->x;
    vy = v->y;
    vz = v->z;

    if ((vx == 0) && (vy == 0) && (vz == 0)) {
        return v;
	}

    inv_mod = (float)(1.0 / sqrt(vx*vx + vy*vy + vz*vz));

    v->x = vx * inv_mod;
    v->y = vy * inv_mod;
    v->z = vz * inv_mod;

    return v;
}


/*
 * Calculates cross product of a and b.
 */
LPD3DVECTOR CGeometry::D3DVECTORCrossProduct(LPD3DVECTOR lpd, LPD3DVECTOR lpa, LPD3DVECTOR lpb)
{

    lpd->x = lpa->y * lpb->z - lpa->z * lpb->y;
    lpd->y = lpa->z * lpb->x - lpa->x * lpb->z;
    lpd->z = lpa->x * lpb->y - lpa->y * lpb->x;
    return lpd;
}


/*
 * lpDst = lpSrc1 * lpSrc2
 * lpDst can be equal to lpSrc1 or lpSrc2
 */
LPD3DMATRIX CGeometry::MultiplyD3DMATRIX(LPD3DMATRIX lpDst, LPD3DMATRIX lpSrc1, 
                              LPD3DMATRIX lpSrc2)
{
    D3DVALUE M1[4][4], M2[4][4], D[4][4];
    int i, r, c;

    memcpy(&M1[0][0], lpSrc1, sizeof(D3DMATRIX));
    memcpy(&M2[0][0], lpSrc2, sizeof(D3DMATRIX));
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            D[r][c] = (float)0.0;
            for (i = 0; i < 4; i++)
                D[r][c] += M1[r][i] * M2[i][c];
        }
    }
    memcpy(lpDst, &D[0][0], sizeof(D3DMATRIX));
    return lpDst;
}



/*
 * -1 d = a
 */
LPD3DMATRIX CGeometry::D3DMATRIXInvert(LPD3DMATRIX d, LPD3DMATRIX a)
{
    d->_11 = a->_11;
    d->_12 = a->_21;
    d->_13 = a->_31;
    d->_14 = a->_14;

    d->_21 = a->_12;
    d->_22 = a->_22;
    d->_23 = a->_32;
    d->_24 = a->_24;

    d->_31 = a->_13;
    d->_32 = a->_23;
    d->_33 = a->_33;
    d->_34 = a->_34;

    d->_41 = a->_14;
    d->_42 = a->_24;
    d->_43 = a->_34;
    d->_44 = a->_44;

    return d;
}


/*
 * Set the rotation part of a matrix such that the vector lpD is the new
 * z-axis and lpU is the new y-axis.
 */
LPD3DMATRIX CGeometry::D3DMATRIXSetRotation(LPD3DMATRIX lpM, LPD3DVECTOR lpD, LPD3DVECTOR lpU)
{
    float t;
    D3DVECTOR d, u, r;

    /*
     * Normalise the direction vector.
     */
    d.x = lpD->x;
    d.y = lpD->y;
    d.z = lpD->z;
    D3DVECTORNormalise(&d);

    u.x = lpU->x;
    u.y = lpU->y;
    u.z = lpU->z;
    /*
     * Project u into the plane defined by d and normalise.
     */
    t = u.x * d.x + u.y * d.y + u.z * d.z;
    u.x -= d.x * t;
    u.y -= d.y * t;
    u.z -= d.z * t;
    D3DVECTORNormalise(&u);

    /*
     * Calculate the vector pointing along the matrix x axis (in a right
     * handed coordinate system) using cross product.
     */
    D3DVECTORCrossProduct(&r, &u, &d);

    lpM->_11 = r.x;
    lpM->_12 = r.y, lpM->_13 = r.z;
    lpM->_21 = u.x;
    lpM->_22 = u.y, lpM->_23 = u.z;
    lpM->_31 = d.x;
    lpM->_32 = d.y;
    lpM->_33 = d.z;

    return lpM;
}

void CGeometry::CalcNormal(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Normal)
{
	D3DVECTOR	nvec;

	nvec.x = (v0->y - v1->y) * (v0->z + v1->z);
	nvec.y = (v0->z - v1->z) * (v0->x + v1->x);
	nvec.z = (v0->x - v1->x) * (v0->y + v1->y);

	nvec.x += (v1->y - v2->y) * (v1->z + v2->z);
	nvec.y += (v1->z - v2->z) * (v1->x + v2->x);
	nvec.z += (v1->x - v2->x) * (v1->y + v2->y);

	nvec.x += (v2->y - v0->y) * (v2->z + v0->z);
	nvec.y += (v2->z - v0->z) * (v2->x + v0->x);
	nvec.z += (v2->x - v0->x) * (v2->y + v0->y);

	D3DVECTORNormalise(&nvec);

	*Normal=nvec;
}

// Calculate plane equation given three vertecies on the plane.
//
void CGeometry::CalcPlaneEquation(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Normal,float *Offset)
{
	D3DVECTOR	refpt;
	D3DVECTOR	nvec;
	D3DVECTOR	tnorm;
	float	len;

// Compute the polygon's normal and a reference point on
// the plane. Note that the actual reference point is
// refpt / 3.

	nvec.x = (v0->y - v1->y) * (v0->z + v1->z);
	nvec.y = (v0->z - v1->z) * (v0->x + v1->x);
	nvec.z = (v0->x - v1->x) * (v0->y + v1->y);
	VecCopy(&refpt,v0);

	nvec.x += (v1->y - v2->y) * (v1->z + v2->z);
	nvec.y += (v1->z - v2->z) * (v1->x + v2->x);
	nvec.z += (v1->x - v2->x) * (v1->y + v2->y);
	VecInc(&refpt,v1);

	nvec.x += (v2->y - v0->y) * (v2->z + v0->z);
	nvec.y += (v2->z - v0->z) * (v2->x + v0->x);
	nvec.z += (v2->x - v0->x) * (v2->y + v0->y);
	VecInc(&refpt,v2);

// Normalize the polygon nvec to obtain the first
// three coefficients of the plane equation.

	len = (float)VecLength(&nvec);
	tnorm.x = nvec.x / len;
	tnorm.y = nvec.y / len;
	tnorm.z = nvec.z / len;

// Compute the last coefficient of the plane equation.

	len *= 3;
	*Offset = (float)(-VecDot(&refpt,&nvec) / len);
	Normal->x = tnorm.x;
	Normal->y = tnorm.y;
	Normal->z = tnorm.z;
}


void CGeometry::SetWorldMatrix(D3DVECTOR *CameraRotation)
{
	m_world = m_identity;
	ConcatenateYRotation(&m_world, CameraRotation->y);
	ConcatenateXRotation(&m_world, CameraRotation->x);
	ConcatenateZRotation(&m_world, CameraRotation->z);
}

void CGeometry::PushWorldMatrix(void)
{
	ASSERT(m_MatrixStackPos < MAXMATRIXSTACKSIZE);

	m_MatrixStack[m_MatrixStackPos] = m_world;
	m_MatrixStackPos++;
}

void CGeometry::PopWorldMatrix(void)
{
	ASSERT(m_MatrixStackPos > 0);

	m_MatrixStackPos--;
	m_world = m_MatrixStack[m_MatrixStackPos];
}

void CGeometry::PushMatrix(D3DMATRIX &Matrix)
{
	ASSERT(m_MatrixStackPos < MAXMATRIXSTACKSIZE);

	m_MatrixStack[m_MatrixStackPos] = Matrix;
	m_MatrixStackPos++;
}

void CGeometry::PopMatrix(D3DMATRIX &Matrix)
{
	ASSERT(m_MatrixStackPos > 0);

	m_MatrixStackPos--;
	Matrix = m_MatrixStack[m_MatrixStackPos];
}


// Rotate an array of 2d vectors about a given angle, also translates them after rotating.
//
void CGeometry::RotateVector2D(VECTOR2D *Vector,VECTOR2D *TVector,VECTOR2D *Pos,float Angle,int Count)
{
	float Rot = (float)((Angle/360) * M_PI * 2.0);
	float Cos = cos(Rot);
	float Sin = sin(Rot);
	float ox = 0.0F;
	float oy = 0.0F;

	if(Pos) {
		ox = Pos->x;
		oy = Pos->y;
	}


	VECTOR2D *Vec = Vector;
	VECTOR2D *TVec = TVector;
	for(int i=0; i<Count; i++) {
		TVec->x = ( Vec->x*Cos + Vec->y*Sin ) + ox;
		TVec->y = ( Vec->y*Cos - Vec->x*Sin ) + oy;
		Vec++;
		TVec++;
	}
}


// Calculates a normalized direction vector pointing in the specified direction.
//
void CGeometry::DirectionVector(D3DVECTOR &Rotation,D3DVECTOR &Direction)
{
	Direction.x = 0.0F;
	Direction.y = 0.0F;
	Direction.z = 0.0F;
	Motion(Direction,Rotation,1.0F);
}


//typedef struct _D3DMATRIX {
//    D3DVALUE        _11, _12, _13, _14;
//    D3DVALUE        _21, _22, _23, _24;
//    D3DVALUE        _31, _32, _33, _34;
//    D3DVALUE        _41, _42, _43, _44;
//} D3DMATRIX, *LPD3DMATRIX;

// Multiply a vector by the world matrix.
//
void CGeometry::RotateVector(D3DVECTOR *Vector,D3DVECTOR *Result)
{
	float x,y,z;

	x = Vector->x*m_world._11 + Vector->y*m_world._21 + Vector->z*m_world._31;
	y = Vector->x*m_world._12 + Vector->y*m_world._22 + Vector->z*m_world._32;
	z = Vector->x*m_world._13 + Vector->y*m_world._23 + Vector->z*m_world._33;
	Result->x = x;
	Result->y = y;
	Result->z = z;
}

void CGeometry::SetObjectMatrix(D3DVECTOR *ObjectRotation,D3DVECTOR *ObjectPosition,D3DVECTOR *CameraPosition)
{
	D3DVECTOR RelVec;

	m_spin = m_identity;
	ConcatenateXRotation(&m_spin, ObjectRotation->x);
	ConcatenateZRotation(&m_spin, ObjectRotation->z);
	ConcatenateYRotation(&m_spin, ObjectRotation->y);

// Calculate position of the object relative to the camera.
	RelVec.x= ObjectPosition->x-CameraPosition->x;
	RelVec.y= ObjectPosition->y-CameraPosition->y;
	RelVec.z= ObjectPosition->z-CameraPosition->z;

// Set the translation in the object matrix.
	SetTranslation(&m_spin,&RelVec);
}


void CGeometry::SetObjectScale(D3DVECTOR *Vector)
{
	SetScale(&m_spin,Vector);
}


void CGeometry::MulObjectMatrix(D3DMATRIX *Matrix)
{
	MultiplyD3DMATRIX(&m_spin, Matrix, &m_spin);
}

BOOL CGeometry::SetTransformation(void)
{
	HRESULT ddrval;

	MultiplyD3DMATRIX(&m_world, &m_spin, &m_world);
	DXCALL(m_3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &m_proj));
	DXCALL(m_3DDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &m_view));      
	DXCALL(m_3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &m_world));    

	return TRUE;
}


void CGeometry::SetTranslation(LPD3DMATRIX Matrix,D3DVECTOR *Vector)
{
	Matrix->_41=Vector->x;
	Matrix->_42=Vector->y;
	Matrix->_43=Vector->z;
}

void CGeometry::SetScale(LPD3DMATRIX Matrix,D3DVECTOR *Vector)
{
	Matrix->_11*=Vector->x;
	Matrix->_22*=Vector->y;
	Matrix->_33*=Vector->z;
}


#define FARCLIP (PATCHSIZE*3)
BOOL CGeometry::TransformVertex(D3DVERTEX *Dest,D3DVERTEX *Source,DWORD NumVertices,D3DHVERTEX *HVertex,BOOL Clipped)
{

	D3DTRANSFORMDATA	TransformData;
	DWORD	OffScreen;
	HRESULT	ddrval;
	BOOL	FreeHV=FALSE;

	if(HVertex==NULL) {
		HVertex = new D3DHVERTEX[NumVertices];
		FreeHV=TRUE;
	}

	ASSERT(NumVertices <= 8);

	memset(&TransformData,0,sizeof(TransformData));
	TransformData.dwSize=sizeof(TransformData);
	TransformData.lpIn=Source;
	TransformData.dwInSize=sizeof(D3DVERTEX);
	TransformData.lpOut=Dest;
	TransformData.dwOutSize=sizeof(D3DVERTEX);
	TransformData.lpHOut=HVertex;
	TransformData.dwClip=0;

	TransformData.dwClipIntersection = 0;

	if(Clipped) {
		DXCALL(m_Viewport->TransformVertices(NumVertices,&TransformData,D3DTRANSFORM_CLIPPED,&OffScreen));
	} else {
		DXCALL(m_Viewport->TransformVertices(NumVertices,&TransformData,D3DTRANSFORM_UNCLIPPED,&OffScreen));
	}

	if(FreeHV) {
		delete HVertex;
	}

	return(OffScreen ? FALSE : TRUE);
}


void CGeometry::Motion(D3DVECTOR &Position,D3DVECTOR &Angle,float Speed)
{
	D3DVECTOR	RadAngle;

	RadAngle.x=RADIANS(Angle.x);
	RadAngle.y=RADIANS(Angle.y);
	RadAngle.z=RADIANS(Angle.z);
	
	Position.z+=( ((float)cos(RadAngle.y)) * ((float)cos(RadAngle.x)) ) * Speed;
	Position.x+=( ((float)sin(RadAngle.y)) * ((float)cos(RadAngle.x)) ) * Speed;
	Position.y+=-((float)sin(RadAngle.x)) * Speed;
}


void CGeometry::LookAt(D3DVECTOR &Target,D3DVECTOR &Position,D3DVECTOR &Rotation)
{
	Rotation.y = DEGREES(atan2(Position.x-Target.x ,Position.z-Target.z)+M_PI);
	Rotation.x = DEGREES(atan2(Position.y-Target.y ,Distance2D(Position.x-Target.x,Position.z-Target.z))+M_PI) - 180.0F;
}


float CGeometry::Orbit(D3DVECTOR &Position,D3DVECTOR &Rotation,D3DVECTOR &Center,float Angle,float Radius,float Step)
{
	float Rads = RADIANS(Angle);

	Position.x = -sin(Rads) * Radius;
	Position.z = -cos(Rads) * Radius;
	Position.x += Center.x;
	Position.z += Center.z;

	LookAt(Center,Position,Rotation);

	Angle += Step;

	return Angle;
}

/*
 * Calculates a point along a B-Spline curve defined by four points. p
 * n output, contain the point. t                                Position
 * along the curve between p2 and p3.  This position is a float between 0
 * and 1. p1, p2, p3, p4    Points defining spline curve. p, at parameter
 * t along the spline curve
 */
void CGeometry::spline(LPD3DVECTOR p, float t, LPD3DVECTOR p1, LPD3DVECTOR p2,
       LPD3DVECTOR p3, LPD3DVECTOR p4)
{
    double t2, t3;
    float m1, m2, m3, m4;

    t2 = (double)(t * t);
    t3 = t2 * (double)t;

    m1 = (float)((-1.0 * t3) + (2.0 * t2) + (-1.0 * (double)t));
    m2 = (float)((3.0 * t3) + (-5.0 * t2) + (0.0 * (double)t) + 2.0);
    m3 = (float)((-3.0 * t3) + (4.0 * t2) + (1.0 * (double)t));
    m4 = (float)((1.0 * t3) + (-1.0 * t2) + (0.0 * (double)t));

    m1 /= (float)2.0;
    m2 /= (float)2.0;
    m3 /= (float)2.0;
    m4 /= (float)2.0;

    p->x = p1->x * m1 + p2->x * m2 + p3->x * m3 + p4->x * m4;
    p->y = p1->y * m1 + p2->y * m2 + p3->y * m3 + p4->y * m4;
    p->z = p1->z * m1 + p2->z * m2 + p3->z * m3 + p4->z * m4;
}


void CGeometry::FindAverageVec(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Average)
{
	float ax=v0->x;
	float ay=v0->y;
	float az=v0->z;

	ax+=v1->x; ay+=v1->y; az+=v1->z;
	ax+=v2->x; ay+=v2->y; az+=v2->z;
	ax/=3;	ay/=3;	az/=3;

	Average->x=ax;
	Average->y=ay;
	Average->z=az;
}


BOOL CGeometry::PointInFace(float XPos,float YPos,D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2)
{
	DWORD l=0;

	l=CheckEdge(v0,v1,XPos,YPos,l);
	l=CheckEdge(v1,v2,XPos,YPos,l);
	l=CheckEdge(v2,v0,XPos,YPos,l);
	if(l&1) {
		return TRUE;
	}
	return FALSE;
}

/*
  UWORD	CheckEdge(SVEC2D *v0,SVEC2D *v1,SLONG x,SLONG z,UWORD l)
 
   AUTHOR:		Paul Dunning

   DESCRIPTION:	Check polygon edge against point. Compares the vector of the specified point
				and the specified edge, if the point vector crosses the edge then returns l+1
				else returns l.

   PARAMETERS:	SVEC2D	*v0;	Pointer to edge vertex 0.
				SVEC2D	*v1;	Pointer to edge vertex 1.
				SLONG	x;		X coord of point to test.
				SLONG	z;		Z coord of point to test.
				UWORD	l;		Edge count.
 
   RETURNS:		UWORD	l;		Updated edge count.

*/
DWORD CGeometry::CheckEdge(D3DVECTOR *v0,D3DVECTOR *v1,float x,float y,DWORD l)
{
	float md,m,b;
	DWORD fa,fb;

	if(v0->y == v1->y) return(l);

	fa=fb=0;

	if(v0->y < y) fa=1;
	if(v1->y < y) fb=1;

	if(fa==fb) return(l);

	fa=fb=0;

	if(v0->x < x) fa=1;
	if(v1->x < x) fb=1;

	if(fa & fb) {
		l++;
		return(l);
	}

	if((fa|fb)==0) return(l);

	md = v1->x - v0->x;
	if(md!=0) {
		m = (v1->y - v0->y) / md;
		b = (v1->y - y) - m*(v1->x - x);
		if((b/m) > 0) {
			l++;
		}
	}

	return(l);
}

int CGeometry::AddLight(DWORD Type)
{
	for(int i=0; i<MAXLIGHTS; i++) {
		if(m_Lights[i].InUse == 0) {
			memset(&m_Lights[i].LightDesc, 0, sizeof(D3DLIGHT));
			m_Lights[i].LightDesc.dwSize = sizeof(D3DLIGHT);
			m_Lights[i].LightDesc.dltType = (D3DLIGHTTYPE)Type;
			m_Lights[i].LightDesc.dcvColor.r = D3DVAL(1.0);
			m_Lights[i].LightDesc.dcvColor.g = D3DVAL(1.0);
			m_Lights[i].LightDesc.dcvColor.b = D3DVAL(1.0);
			m_Lights[i].LightDesc.dcvColor.a = D3DVAL(1.0);
			m_Lights[i].LightDesc.dvPosition.x = D3DVAL(0);
			m_Lights[i].LightDesc.dvPosition.y = D3DVAL(5000);
			m_Lights[i].LightDesc.dvPosition.z = D3DVAL(0);
			m_Lights[i].LightDesc.dvDirection.x = D3DVAL(0);
			m_Lights[i].LightDesc.dvDirection.y = D3DVAL(-1);
			m_Lights[i].LightDesc.dvDirection.z = D3DVAL(0);
			m_Lights[i].LightDesc.dvRange = D3DVAL(4000);
			m_Lights[i].LightDesc.dvFalloff = D3DVAL(0);
			m_Lights[i].LightDesc.dvTheta = D3DVAL(1);
			m_Lights[i].LightDesc.dvPhi = D3DVAL(10);
			m_Lights[i].LightDesc.dvAttenuation0 = D3DVAL(0.5);
			m_Lights[i].LightDesc.dvAttenuation1 = D3DVAL(0.0);
			m_Lights[i].LightDesc.dvAttenuation2 = D3DVAL(0.0);
			m_Lights[i].Position.x = D3DVAL(0);
			m_Lights[i].Position.y = D3DVAL(5000);
			m_Lights[i].Position.z = D3DVAL(0);
			m_Lights[i].Direction.x = D3DVAL(0);
			m_Lights[i].Direction.y = D3DVAL(-1);
			m_Lights[i].Direction.z = D3DVAL(0);
			m_Lights[i].Transform = TRUE;
			m_Lights[i].InUse++;

			HRESULT	ddrval;
			DXCALL_INT(m_Direct3D->CreateLight(&m_Lights[i].Light, NULL));
			DXCALL_INT(m_Lights[i].Light->SetLight(&m_Lights[i].LightDesc));
			DXCALL_INT(m_Viewport->AddLight(m_Lights[i].Light));

			return i;
		}
	}

	return -1;
}

BOOL CGeometry::RemoveLight(int LightID)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		HRESULT	ddrval;
		DXCALL(m_Viewport->DeleteLight(m_Lights[LightID].Light));
		DXCALL(m_Lights[LightID].Light->Release());

		m_Lights[LightID].InUse=0;
	}

	return TRUE;
}

void CGeometry::SetTransform(int LightID,BOOL Transform)
{
	m_Lights[LightID].Transform = Transform;
}

void CGeometry::SetLightPosition(int LightID,D3DVECTOR *Position)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		m_Lights[LightID].Position = *Position;
	}
}

void CGeometry::SetLightDirection(int LightID,D3DVECTOR *Direction)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		m_Lights[LightID].Direction = *Direction;
	}
}

void CGeometry::SetLightColour(int LightID,D3DCOLORVALUE *Colour)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		m_Lights[LightID].LightDesc.dcvColor = *Colour;
	}
}

void CGeometry::SetLightRange(int LightID,float Range)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		m_Lights[LightID].LightDesc.dvRange = D3DVAL(Range);
	}
}

void CGeometry::SetLightFalloff(int LightID,float Falloff)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		m_Lights[LightID].LightDesc.dvFalloff = D3DVAL(Falloff);
	}
}

void CGeometry::SetLightCone(int LightID,float Umbra,float Penumbra)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		m_Lights[LightID].LightDesc.dvTheta = D3DVAL(Umbra);
		m_Lights[LightID].LightDesc.dvPhi = D3DVAL(Penumbra);
	}
}


void CGeometry::SetLightAttenuation(int LightID,float Att0,float Att1,float Att2)
{
	ASSERT((LightID < MAXLIGHTS) && (LightID >= 0));

	if(m_Lights[LightID].InUse) {
		m_Lights[LightID].LightDesc.dvAttenuation0 = D3DVAL(Att0);
		m_Lights[LightID].LightDesc.dvAttenuation1 = D3DVAL(Att1);
		m_Lights[LightID].LightDesc.dvAttenuation2 = D3DVAL(Att2);
	}
}

BOOL CGeometry::TransformLights(D3DVECTOR *CameraRotation,D3DVECTOR *CameraPosition)
{
	HRESULT	ddrval;

	for(int i=0; i<MAXLIGHTS; i++) {
		if(m_Lights[i].InUse) {
				D3DVECTOR RelVec = m_Lights[i].Position;
				RelVec.x -= CameraPosition->x;
				RelVec.y -= CameraPosition->y;
				RelVec.z -= CameraPosition->z;
				SetWorldMatrix(CameraRotation);
				RotateVector(&RelVec,&m_Lights[i].LightDesc.dvPosition);

				if(m_Lights[i].Transform) {
					RelVec = m_Lights[i].Direction;
					RotateVector(&RelVec,&m_Lights[i].LightDesc.dvDirection);
				}

				DXCALL(m_Lights[i].Light->SetLight(&m_Lights[i].LightDesc));
		}
	}

	return TRUE;
}

