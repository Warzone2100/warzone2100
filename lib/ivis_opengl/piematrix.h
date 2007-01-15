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
/***************************************************************************/
/*
 * pieMatrix.h
 *
 * matrix functions for pumpkin image library.
 *
 */
/***************************************************************************/
#ifndef _pieMatrix_h
#define _pieMatrix_h

#include "lib/ivis_common/piedef.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

typedef struct {SDWORD a, b, c,  d, e, f,  g, h, i,  j, k, l;} SDMATRIX;

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern SDMATRIX *psMatrix;
extern SDWORD	aSinTable[];

//*************************************************************************

#define SIN(X)					aSinTable[(uint16)(X) >> 4]
#define COS(X)					aSinTable[((uint16)(X) >> 4) + 1024]


//*************************************************************************

#define pie_SETUP_ROTATE_PROJECT

#define pie_ROTATE_PROJECT(x, y, z, sx, sy) pie_RotateProject(x, y, z, &(sx), &(sy))


//*************************************************************************

#define pie_ROTATE_TRANSLATE(x,y,z,xs,ys,zs)										\
{																			\
	xs = (x) * psMatrix->a + (y) * psMatrix->d + (z) * psMatrix->g +	\
				psMatrix->j;												\
	ys = (x) * psMatrix->b + (y) * psMatrix->e + (z) * psMatrix->h +	\
				psMatrix->k;												\
	zs = (x) * psMatrix->c + (y) * psMatrix->f + (z) * psMatrix->i +	\
				psMatrix->l;												\
	xs >>=FP12_SHIFT;												\
	ys >>=FP12_SHIFT;												\
	zs >>=FP12_SHIFT;												\
}

//*************************************************************************


//*************************************************************************

#define pie_CLOCKWISE(x0,y0,x1,y1,x2,y2) ((((y1)-(y0)) * ((x2)-(x1))) <= (((x1)-(x0)) * ((y2)-(y1))))

//*************************************************************************


extern void pie_MatInit(void);


//*************************************************************************

extern void pie_MatBegin(void);
extern void pie_MatEnd(void);
extern void pie_MATTRANS(int x,int y,int z);
extern void pie_TRANSLATE(int x, int y, int z);
extern void pie_MatScale( UDWORD percent );
extern void pie_MatRotX(int x);
extern void pie_MatRotY(int y);
extern void pie_MatRotZ(int z);
extern int32 pie_RotProj(iVector *v3d, iPoint *v2d);
extern int32 pie_RotateProject(SDWORD x, SDWORD y, SDWORD z, SDWORD* xs, SDWORD* ys);

//*************************************************************************

extern void pie_PerspectiveBegin(void);
extern void pie_PerspectiveEnd(void);

//*************************************************************************

extern void pie_VectorNormalise(iVector *v);
extern void pie_VectorInverseRotate0(iVector *v1, iVector *v2);
extern void pie_SurfaceNormal(iVector *p1, iVector *p2, iVector *p3, iVector *v);
extern BOOL pie_Clockwise(iVertex *s);
extern void pie_SetGeometricOffset(int x, int y);

extern BOOL pie_PieClockwise(PIEVERTEX *s);

void pie_Begin3DScene(void);
void pie_BeginInterface(void);

#endif
