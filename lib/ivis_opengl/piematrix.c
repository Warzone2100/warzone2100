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
 * pieMatrix.c
 *
 * matrix functions for pumpkin image library.
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"

#include <SDL_opengl.h>

#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/pieclip.h"
#include "piematrix.h"
#include "lib/ivis_common/rendmode.h"

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

#define MATRIX_MAX 8
#define ONE_PERCENT 4096/100

typedef struct { SDWORD a, b, c,  d, e, f,  g, h, i,  j, k, l; } SDMATRIX;
static SDMATRIX	aMatrixStack[MATRIX_MAX];
static SDMATRIX *psMatrix = &aMatrixStack[0];

BOOL drawing_interface = TRUE;


#define SC_TABLESIZE	4096

//*************************************************************************

// We use FP12_MULTIPLIER => This matrix should be float instead
static SDMATRIX _MATRIX_ID = {FP12_MULTIPLIER, 0, 0, 0, FP12_MULTIPLIER, 0, 0, 0, FP12_MULTIPLIER, 0L, 0L, 0L};
static SDWORD _MATRIX_INDEX;

//*************************************************************************

SDWORD aSinTable[SC_TABLESIZE + (SC_TABLESIZE/4)];

//*************************************************************************
//*** reset transformation matrix stack and make current identity
//*
//******

static void pie_MatReset(void)
{
	psMatrix = &aMatrixStack[0];

	// make 1st matrix identity
	*psMatrix = _MATRIX_ID;

	glLoadIdentity();
}


//*************************************************************************
//*** create new matrix from current transformation matrix and make current
//*
//******

void pie_MatBegin(void)
{
	_MATRIX_INDEX++;
	ASSERT( _MATRIX_INDEX < MATRIX_MAX, "pie_MatBegin past top of the stack" );

	psMatrix++;
	aMatrixStack[_MATRIX_INDEX] = aMatrixStack[_MATRIX_INDEX-1];

	glPushMatrix();
}


//*************************************************************************
//*** make current transformation matrix previous one on stack
//*
//******

void pie_MatEnd(void)
{
	_MATRIX_INDEX--;
	ASSERT( _MATRIX_INDEX >= 0, "pie_MatEnd of the bottom of the stack" );

	psMatrix--;

	glPopMatrix();
}


void pie_MATTRANS(int x, int y, int z)
{
	GLfloat matrix[16];

	psMatrix->j = x<<FP12_SHIFT;
	psMatrix->k = y<<FP12_SHIFT;
	psMatrix->l = z<<FP12_SHIFT;

	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	matrix[12] = x;
	matrix[13] = y;
	matrix[14] = z;
	glLoadIdentity();
	glMultMatrixf(matrix);
}

void pie_TRANSLATE(int x, int y, int z)
{
	psMatrix->j += ((x) * psMatrix->a + (y) * psMatrix->d + (z) * psMatrix->g);
	psMatrix->k += ((x) * psMatrix->b + (y) * psMatrix->e + (z) * psMatrix->h);
	psMatrix->l += ((x) * psMatrix->c + (y) * psMatrix->f + (z) * psMatrix->i);

	glTranslatef(x, y, z);
}

//*************************************************************************
//*** matrix scale current transformation matrix
//*
//******
void pie_MatScale( UDWORD percent )
{
	SDWORD scaleFactor;

	if (percent == 100)
	{
		return;
	}

	scaleFactor = percent * ONE_PERCENT;

	psMatrix->a = (psMatrix->a * scaleFactor) / 4096;
	psMatrix->b = (psMatrix->b * scaleFactor) / 4096;
	psMatrix->c = (psMatrix->c * scaleFactor) / 4096;

	psMatrix->d = (psMatrix->d * scaleFactor) / 4096;
	psMatrix->e = (psMatrix->e * scaleFactor) / 4096;
	psMatrix->f = (psMatrix->f * scaleFactor) / 4096;

	psMatrix->g = (psMatrix->g * scaleFactor) / 4096;
	psMatrix->h = (psMatrix->h * scaleFactor) / 4096;
	psMatrix->i = (psMatrix->i * scaleFactor) / 4096;

	glScalef(0.01f*percent, 0.01f*percent, 0.01f*percent);
}


//*************************************************************************
//*** matrix rotate y (yaw) current transformation matrix
//*
//******

void pie_MatRotY(int y)
{
	int t;
	int cra, sra;

	if (y != 0) {
   	cra = COS(y);
	sra = SIN(y);

		t = ((cra * psMatrix->a) - (sra * psMatrix->g))>>FP12_SHIFT;
		psMatrix->g = ((sra * psMatrix->a) + (cra * psMatrix->g))>>FP12_SHIFT;
		psMatrix->a = t;

		t = ((cra * psMatrix->b) - (sra * psMatrix->h))>>FP12_SHIFT;
		psMatrix->h = ((sra * psMatrix->b) + (cra * psMatrix->h))>>FP12_SHIFT;
		psMatrix->b = t;

		t = ((cra * psMatrix->c) - (sra * psMatrix->i))>>FP12_SHIFT;
		psMatrix->i = ((sra * psMatrix->c) + (cra * psMatrix->i))>>FP12_SHIFT;
		psMatrix->c = t;
	}

	glRotatef(y*22.5f/4096.0f, 0.0f, 1.0f, 0.0f);
}


//*************************************************************************
//*** matrix rotate z (roll) current transformation matrix
//*
//******

void pie_MatRotZ(int z)
{
	int t;
	int cra, sra;

	if (z != 0) {
		cra = COS(z);
		sra = SIN(z);

		t = ((cra * psMatrix->a) + (sra * psMatrix->d))>>FP12_SHIFT;
		psMatrix->d = ((cra * psMatrix->d) - (sra * psMatrix->a))>>FP12_SHIFT;
		psMatrix->a = t;

		t = ((cra * psMatrix->b) + (sra * psMatrix->e))>>FP12_SHIFT;
		psMatrix->e = ((cra * psMatrix->e) - (sra * psMatrix->b))>>FP12_SHIFT;
		psMatrix->b = t;

		t = ((cra * psMatrix->c) + (sra * psMatrix->f))>>FP12_SHIFT;
		psMatrix->f = ((cra * psMatrix->f) - (sra * psMatrix->c))>>FP12_SHIFT;
		psMatrix->c = t;
	}

	glRotatef(z*22.5f/4096.0f, 0.0f, 0.0f, 1.0f);
}


//*************************************************************************
//*** matrix rotate x (pitch) current transformation matrix
//*
//******

void pie_MatRotX(int x)
{
	int cra, sra;
	int t;

	if (x != 0) {
		cra = COS(x);
		sra = SIN(x);

		t = ((cra * psMatrix->d) + (sra * psMatrix->g))>>FP12_SHIFT;
		psMatrix->g = ((cra * psMatrix->g) - (sra * psMatrix->d))>>FP12_SHIFT;
		psMatrix->d = t;

		t = ((cra * psMatrix->e) + (sra * psMatrix->h))>>FP12_SHIFT;
		psMatrix->h = ((cra * psMatrix->h) - (sra * psMatrix->e))>>FP12_SHIFT;
		psMatrix->e = t;

		t = ((cra * psMatrix->f) + (sra * psMatrix->i))>>FP12_SHIFT;
		psMatrix->i = ((cra * psMatrix->i) - (sra * psMatrix->f))>>FP12_SHIFT;
		psMatrix->f = t;
	}

	glRotatef(x*22.5f/4096.0f, 1.0f, 0.0f, 0.0f);
}


/*!
 * 3D vector perspective projection
 * Projects 3D vector into 2D screen space
 * \param v3d 3D vector to project
 * \param v2d resulting 2D vector
 * \return projected z component of v2d
 */
Sint32 pie_RotateProject(const Vector3i *v3d, Vector2i *v2d)
{
	Sint32 zfx, zfy;
	Sint32 zz, _x, _y, _z;

	_x = v3d->x * psMatrix->a + v3d->y * psMatrix->d + v3d->z * psMatrix->g + psMatrix->j;
	_y = v3d->x * psMatrix->b + v3d->y * psMatrix->e + v3d->z * psMatrix->h + psMatrix->k;
	_z = v3d->x * psMatrix->c + v3d->y * psMatrix->f + v3d->z * psMatrix->i + psMatrix->l;

	zz = _z >> STRETCHED_Z_SHIFT;

	zfx = _z >> psRendSurface->xpshift;
	zfy = _z >> psRendSurface->ypshift;

	if (zfx <= 0 || zfy <= 0 || zz < MIN_STRETCHED_Z)
	{
		v2d->x = LONG_WAY; //just along way off screen
		v2d->y = LONG_WAY;
	}
	else
	{
		v2d->x = psRendSurface->xcentre + (_x / zfx);
		v2d->y = psRendSurface->ycentre - (_y / zfy);
	}

	return zz;
}


//*************************************************************************

void pie_PerspectiveBegin(void)
{
	const float width = pie_GetVideoBufferWidth();
	const float height = pie_GetVideoBufferHeight();
	const float xangle = width/6;
	const float yangle = height/6;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef((2*psRendSurface->xcentre-width)/width,
		     (height-2*psRendSurface->ycentre)/height , 0);
	glFrustum(-xangle, xangle, -yangle, yangle, 330, 100000);
	glScalef(1, 1, -1);
	glMatrixMode(GL_MODELVIEW);
}

void pie_PerspectiveEnd(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight(), 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
}


void pie_TranslateTextureBegin(const Vector2f offset)
{
	glMatrixMode(GL_TEXTURE);
	glTranslatef(offset.x, offset.y, 0.0f);
	glMatrixMode(GL_MODELVIEW);
}

void pie_TranslateTextureEnd(void)
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(1.0f/OLD_TEXTURE_SIZE_FIX, 1.0f/OLD_TEXTURE_SIZE_FIX, 1.0f); // FIXME Scaling texture coords to 256x256!
	glMatrixMode(GL_MODELVIEW);
}


void pie_Begin3DScene(void)
{
	glDepthRange(0.1, 1);
	drawing_interface = FALSE;
}

void pie_BeginInterface(void)
{
	glDepthRange(0, 0.1);
	drawing_interface = TRUE;
}

//*************************************************************************

void pie_SetGeometricOffset(int x, int y)
{
	psRendSurface->xcentre = x;
	psRendSurface->ycentre = y;
}


//*************************************************************************
//*** inverse rotate 3D vector with current rotation matrix
//*
//* params	v1 = pointer to 3D vector to rotate
//* 			v2 = pointer to 3D resultant vector
//*
//* on exit	v2 = inverse-rotated vector
//*
//******

void pie_VectorInverseRotate0(const Vector3i *v1, Vector3i *v2)
{
	Sint32 x, y, z;

	x = v1->x; y = v1->y; z = v1->z;

	v2->x = (x * psMatrix->a+y * psMatrix->b+z * psMatrix->c) >> FP12_SHIFT;
	v2->y = (x * psMatrix->d+y * psMatrix->e+z * psMatrix->f) >> FP12_SHIFT;
	v2->z = (x * psMatrix->g+y * psMatrix->h+z * psMatrix->i) >> FP12_SHIFT;
}

//*************************************************************************
//*** setup transformation matrices/quaternions and trig tables
//*
//******

void pie_MatInit(void)
{
	unsigned i, scsize;
	double conv, v;

	// sin/cos table

	scsize = SC_TABLESIZE + (SC_TABLESIZE / 4);
  	conv = (double)(M_PI / (0.5 * SC_TABLESIZE));

	for (i=0; i<scsize; i++) {
		v = (double) sin(i * conv) * FP12_MULTIPLIER;

		if (v >= 0.0)
			aSinTable[i] = (Sint32)(v + 0.5);
		else
			aSinTable[i] = (Sint32)(v - 0.5);
	}

	// init matrix/quat stack

	pie_MatReset();
}

void pie_RotateTranslate3iv(const Vector3i *v, Vector3i *s)
{
	s->x = ( v->x * psMatrix->a + v->z * psMatrix->d + v->y * psMatrix->g
			+ psMatrix->j ) / FP12_MULTIPLIER;
	s->z = ( v->x * psMatrix->b + v->z * psMatrix->e + v->y * psMatrix->h
			+ psMatrix->k ) / FP12_MULTIPLIER;
	s->y = ( v->x * psMatrix->c + v->z * psMatrix->f + v->y * psMatrix->i
			+ psMatrix->l ) / FP12_MULTIPLIER;
}
