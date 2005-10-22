#ifndef _INCLUDED_GEOMETRY_
#define _INCLUDED_GEOMETRY_

#include <ddraw.h>
#include <d3d.h>
#include "typedefs.h"
#include "macros.h"

#ifdef DLLEXPORT
#define EXPORT __declspec (dllexport)
#else
#define EXPORT
#endif

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
EXPORT	CGeometry(ID3D *Direct3D,ID3DDEVICE *Device,ID3DVIEPORT *Viewport);
EXPORT	~CGeometry(void);
EXPORT	void SetTransState(void);
EXPORT	void GetIdentity(D3DMATRIX *Matrix) { *Matrix = m_identity; }
EXPORT	void ConcatenateXRotation(LPD3DMATRIX lpM, float Degrees );
EXPORT	void ConcatenateYRotation(LPD3DMATRIX lpM, float Degrees );
EXPORT	void ConcatenateZRotation(LPD3DMATRIX lpM, float Degrees );
EXPORT	LPD3DVECTOR D3DVECTORNormalise(LPD3DVECTOR v);
EXPORT	LPD3DVECTOR D3DVECTORCrossProduct(LPD3DVECTOR lpd, LPD3DVECTOR lpa, LPD3DVECTOR lpb);
EXPORT	LPD3DMATRIX MultiplyD3DMATRIX(LPD3DMATRIX lpDst, LPD3DMATRIX lpSrc1, LPD3DMATRIX lpSrc2);
EXPORT	LPD3DMATRIX D3DMATRIXInvert(LPD3DMATRIX d, LPD3DMATRIX a);
EXPORT	LPD3DMATRIX D3DMATRIXSetRotation(LPD3DMATRIX lpM, LPD3DVECTOR lpD, LPD3DVECTOR lpU);
EXPORT	void DirectionVector(D3DVECTOR &Rotation,D3DVECTOR &Direction);
EXPORT	void RotateVector2D(VECTOR2D *Vector,VECTOR2D *TVector,VECTOR2D *Pos,float Angle,int Count);
EXPORT	BOOL RotateTranslateProject(D3DVECTOR *Vector,D3DVECTOR *Result);
EXPORT	void RotateVector(D3DVECTOR *Vector,D3DVECTOR *Result);
EXPORT	void spline(LPD3DVECTOR p, float t, LPD3DVECTOR p1, LPD3DVECTOR p2, LPD3DVECTOR p3, LPD3DVECTOR p4);
EXPORT	void CalcNormal(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Normal);
EXPORT	void CalcPlaneEquation(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Normal,float *Offset);
EXPORT	void SetWorldMatrix(D3DVECTOR *CameraRotation);
EXPORT	void PushWorldMatrix(void);
EXPORT	void PopWorldMatrix(void);
EXPORT	void PushMatrix(D3DMATRIX &Matrix);
EXPORT	void PopMatrix(D3DMATRIX &Matrix);
EXPORT	void SetObjectMatrix(D3DVECTOR *ObjectRotation,D3DVECTOR *ObjectPosition,D3DVECTOR *CameraPosition);
EXPORT	void SetObjectScale(D3DVECTOR *Vector);
EXPORT	void MulObjectMatrix(D3DMATRIX *Matrix);
EXPORT	BOOL SetTransformation(void);
EXPORT	void SetTranslation(LPD3DMATRIX Matrix,D3DVECTOR *Vector);
EXPORT	void SetScale(LPD3DMATRIX Matrix,D3DVECTOR *Vector);
EXPORT	BOOL TransformVertex(D3DVERTEX *Dest,D3DVERTEX *Source,DWORD NumVertices,D3DHVERTEX *HVertex=NULL,BOOL Clipped=TRUE);
EXPORT	void InverseTransformVertex(D3DVECTOR *Dest,D3DVECTOR *Source);
EXPORT	void Motion(D3DVECTOR &Position,D3DVECTOR &Angle,float Speed);
EXPORT	void LookAt(D3DVECTOR &Target,D3DVECTOR &Position,D3DVECTOR &Rotation);
EXPORT	float Orbit(D3DVECTOR &Position,D3DVECTOR &Rotation,D3DVECTOR &Center,float Angle,float Radius,float Step);
EXPORT	void FindAverageVec(D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2,D3DVECTOR *Average);
EXPORT	BOOL PointInFace(float Xpos,float YPos,D3DVECTOR *v0,D3DVECTOR *v1,D3DVECTOR *v2);
EXPORT	DWORD CheckEdge(D3DVECTOR *v0,D3DVECTOR *v1,float x,float y,DWORD l);

EXPORT	int AddLight(DWORD Type);
EXPORT	void SetTransform(int LightID,BOOL Transform);
EXPORT	void SetLightPosition(int LightID,D3DVECTOR *Position);
EXPORT	void SetLightDirection(int LightID,D3DVECTOR *Rotation);
EXPORT	void SetLightColour(int LightID,D3DCOLORVALUE *Colour);
EXPORT	void SetLightRange(int LightID,float Range);
EXPORT	void SetLightFalloff(int LightID,float Falloff);
EXPORT	void SetLightCone(int LightID,float Umbra,float Penumbra);
EXPORT	void SetLightAttenuation(int LightID,float Att0,float Att1,float Att2);
EXPORT	BOOL RemoveLight(int LightID);
EXPORT	BOOL TransformLights(D3DVECTOR *CameraRotation,D3DVECTOR *CameraPosition);

protected:
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

#endif