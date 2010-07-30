/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
/** \file
 *  Matrix manipulation functions.
 */

#include <GLee.h>
#include "lib/framework/frame.h"

#include "lib/framework/fixedpoint.h"
#include "lib/ivis_common/pieclip.h"
#include "piematrix.h"
#include "lib/ivis_common/rendmode.h"

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

#define MATRIX_MAX 8

typedef struct { SDWORD a, b, c,  d, e, f,  g, h, i,  j, k, l; } SDMATRIX;
static SDMATRIX	aMatrixStack[MATRIX_MAX];
static SDMATRIX *psMatrix = &aMatrixStack[0];

BOOL drawing_interface = true;

//*************************************************************************

// We use FP12_MULTIPLIER.
static SDMATRIX _MATRIX_ID = {FP12_MULTIPLIER, 0, 0, 0, FP12_MULTIPLIER, 0, 0, 0, FP12_MULTIPLIER, 0L, 0L, 0L};
static SDWORD _MATRIX_INDEX;

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
	glLoadMatrixf(matrix);
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
void pie_MatScale( unsigned int percent )
{
	if (percent == 100)
	{
		return;
	}

	psMatrix->a = (psMatrix->a * percent) / 100;
	psMatrix->b = (psMatrix->b * percent) / 100;
	psMatrix->c = (psMatrix->c * percent) / 100;

	psMatrix->d = (psMatrix->d * percent) / 100;
	psMatrix->e = (psMatrix->e * percent) / 100;
	psMatrix->f = (psMatrix->f * percent) / 100;

	psMatrix->g = (psMatrix->g * percent) / 100;
	psMatrix->h = (psMatrix->h * percent) / 100;
	psMatrix->i = (psMatrix->i * percent) / 100;

	glScalef(0.01f*percent, 0.01f*percent, 0.01f*percent);
}


//*************************************************************************
//*** matrix rotate y (yaw) current transformation matrix
//*
//******

void pie_MatRotY(int y)
{
	if (y != 0)
	{
		int t;
		int64_t cra = iCos(y), sra = iSin(y);

		t = (cra*psMatrix->a - sra*psMatrix->g)>>16;
		psMatrix->g = (sra*psMatrix->a + cra*psMatrix->g)>>16;
		psMatrix->a = t;

		t = (cra*psMatrix->b - sra*psMatrix->h)>>16;
		psMatrix->h = (sra*psMatrix->b + cra*psMatrix->h)>>16;
		psMatrix->b = t;

		t = (cra*psMatrix->c - sra*psMatrix->i)>>16;
		psMatrix->i = (sra*psMatrix->c + cra*psMatrix->i)>>16;
		psMatrix->c = t;

		glRotatef(y*(360.f / 65536), 0.0f, 1.0f, 0.0f);
	}
}


//*************************************************************************
//*** matrix rotate z (roll) current transformation matrix
//*
//******

void pie_MatRotZ(int z)
{
	if (z != 0)
	{
		int t;
		int64_t cra = iCos(z), sra = iSin(z);

		t = (cra*psMatrix->a + sra*psMatrix->d)>>16;
		psMatrix->d = (cra*psMatrix->d - sra*psMatrix->a)>>16;
		psMatrix->a = t;

		t = (cra*psMatrix->b + sra*psMatrix->e)>>16;
		psMatrix->e = (cra*psMatrix->e - sra*psMatrix->b)>>16;
		psMatrix->b = t;

		t = (cra*psMatrix->c + sra*psMatrix->f)>>16;
		psMatrix->f = (cra*psMatrix->f - sra*psMatrix->c)>>16;
		psMatrix->c = t;

		glRotatef(z*(360.f / 65536), 0.0f, 0.0f, 1.0f);
	}
}


//*************************************************************************
//*** matrix rotate x (pitch) current transformation matrix
//*
//******

void pie_MatRotX(int x)
{
	if (x != 0)
	{
		int t;
		int64_t cra = iCos(x), sra = iSin(x);

		t = (cra*psMatrix->d + sra*psMatrix->g)>>16;
		psMatrix->g = (cra*psMatrix->g - sra*psMatrix->d)>>16;
		psMatrix->d = t;

		t = (cra*psMatrix->e + sra*psMatrix->h)>>16;
		psMatrix->h = (cra*psMatrix->h - sra*psMatrix->e)>>16;
		psMatrix->e = t;

		t = (cra*psMatrix->f + sra*psMatrix->i)>>16;
		psMatrix->i = (cra*psMatrix->i - sra*psMatrix->f)>>16;
		psMatrix->f = t;

		glRotatef(x*(360.f / 65536), 1.0f, 0.0f, 0.0f);
	}
}


/*!
 * 3D vector perspective projection
 * Projects 3D vector into 2D screen space
 * \param v3d       3D vector to project
 * \param[out] v2d  resulting 2D vector
 * \return projected z component of v2d
 */
int32_t pie_RotateProject(const Vector3i *v3d, Vector2i *v2d)
{
	Vector3i v = {
		v3d->x * psMatrix->a + v3d->y * psMatrix->d + v3d->z * psMatrix->g + psMatrix->j,
		v3d->x * psMatrix->b + v3d->y * psMatrix->e + v3d->z * psMatrix->h + psMatrix->k,
		v3d->x * psMatrix->c + v3d->y * psMatrix->f + v3d->z * psMatrix->i + psMatrix->l
	};

	int zz = v.z >> STRETCHED_Z_SHIFT;
	int zfx = v.z >> psRendSurface->xpshift;
	int zfy = v.z >> psRendSurface->ypshift;

	if (zfx <= 0 || zfy <= 0 || zz < MIN_STRETCHED_Z)
	{
		v2d->x = LONG_WAY; //just along way off screen
		v2d->y = LONG_WAY;
	}
	else
	{
		v2d->x = psRendSurface->xcentre + (v.x / zfx);
		v2d->y = psRendSurface->ycentre - (v.y / zfy);
	}

	return zz;
}

void pie_PerspectiveBegin(void)
{
	const float width = pie_GetVideoBufferWidth();
	const float height = pie_GetVideoBufferHeight();
	const float xangle = width/6.0f;
	const float yangle = height/6.0f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef(
		(2 * psRendSurface->xcentre-width) / width,
		(height - 2 * psRendSurface->ycentre) / height,
		0);
	glFrustum(-xangle, xangle, -yangle, yangle, 330, 100000);
	glScalef(1, 1, -1);
	glMatrixMode(GL_MODELVIEW);
}

void pie_PerspectiveEnd(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, (double) pie_GetVideoBufferWidth(), (double) pie_GetVideoBufferHeight(), 0.0f, 1.0f, -1.0f);
	glMatrixMode(GL_MODELVIEW);
}

void pie_Begin3DScene(void)
{
	glDepthRange(0.1, 1);
	drawing_interface = false;
}

void pie_BeginInterface(void)
{
	glDepthRange(0, 0.1);
	drawing_interface = true;
}

void pie_SetGeometricOffset(int x, int y)
{
	psRendSurface->xcentre = x;
	psRendSurface->ycentre = y;
}


/** Inverse rotate 3D vector with current rotation matrix.
 *  @param v1       3D vector to rotate
 *  @param[out] v2  inverse rotated 3D vector
 */
void pie_VectorInverseRotate0(const Vector3i *v1, Vector3i *v2)
{
	unsigned int x = v1->x, y = v1->y, z = v1->z;

	v2->x = (x * psMatrix->a + y * psMatrix->b + z * psMatrix->c) >> FP12_SHIFT;
	v2->y = (x * psMatrix->d + y * psMatrix->e + z * psMatrix->f) >> FP12_SHIFT;
	v2->z = (x * psMatrix->g + y * psMatrix->h + z * psMatrix->i) >> FP12_SHIFT;
}

/** Sets up transformation matrices/quaternions and trig tables
 */
void pie_MatInit(void)
{
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


void pie_RotateTranslate3f(const Vector3f *v, Vector3f *s)
{
	s->x = ( v->x * psMatrix->a + v->z * psMatrix->d + v->y * psMatrix->g
			+ psMatrix->j ) / FP12_MULTIPLIER;
	s->z = ( v->x * psMatrix->b + v->z * psMatrix->e + v->y * psMatrix->h
			+ psMatrix->k ) / FP12_MULTIPLIER;
	s->y = ( v->x * psMatrix->c + v->z * psMatrix->f + v->y * psMatrix->i
			+ psMatrix->l ) / FP12_MULTIPLIER;
}
