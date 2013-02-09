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
*/
/** \file
 *  Matrix manipulation functions.
 */

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"

#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/pieclip.h"
#include "piematrix.h"
#include "lib/ivis_opengl/piemode.h"

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

#define MATRIX_MAX 8

/*
 * Matrices are of this form:
 * [ a d g j ]
 * [ b e h k ]
 * [ c f i l ]
 * [ 0 0 0 1 ]
 */
struct SDMATRIX
{
	SDWORD a, b, c,
	       d, e, f,
	       g, h, i,
	       j, k, l;

	SDMATRIX() : a(FP12_MULTIPLIER), b(0), c(0), d(0), e(FP12_MULTIPLIER), f(0), g(0), h(0), i(FP12_MULTIPLIER), j(0), k(0), l(0) {}
};
static SDMATRIX	aMatrixStack[MATRIX_MAX];
static SDMATRIX *psMatrix = &aMatrixStack[0];

//*************************************************************************

static SDWORD _MATRIX_INDEX;

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


void pie_TRANSLATE(int32_t x, int32_t y, int32_t z)
{
	/*
	 * curMatrix = curMatrix . translationMatrix(x, y, z)
	 *
	 *                         [ 1 0 0 x ]
	 *                         [ 0 1 0 y ]
	 * curMatrix = curMatrix . [ 0 0 1 z ]
	 *                         [ 0 0 0 1 ]
	 */
	psMatrix->j += x * psMatrix->a + y * psMatrix->d + z * psMatrix->g;
	psMatrix->k += x * psMatrix->b + y * psMatrix->e + z * psMatrix->h;
	psMatrix->l += x * psMatrix->c + y * psMatrix->f + z * psMatrix->i;

	glTranslatef(x, y, z);
}

//*************************************************************************
//*** matrix scale current transformation matrix
//*
//******
void pie_MatScale(float scale)
{
	/*
	 * s := scale
	 * curMatrix = curMatrix . scaleMatrix(s, s, s)
	 *
	 *                         [ s 0 0 0 ]
	 *                         [ 0 s 0 0 ]
	 * curMatrix = curMatrix . [ 0 0 s 0 ]
	 *                         [ 0 0 0 1 ]
	 *
	 * curMatrix = scale * curMatrix
	 */

	psMatrix->a = psMatrix->a * scale;
	psMatrix->b = psMatrix->b * scale;
	psMatrix->c = psMatrix->c * scale;

	psMatrix->d = psMatrix->d * scale;
	psMatrix->e = psMatrix->e * scale;
	psMatrix->f = psMatrix->f * scale;

	psMatrix->g = psMatrix->g * scale;
	psMatrix->h = psMatrix->h * scale;
	psMatrix->i = psMatrix->i * scale;

	glScalef(scale, scale, scale);
}


//*************************************************************************
//*** matrix rotate y (yaw) current transformation matrix
//*
//******
void pie_MatRotY(uint16_t y)
{
	/*
	 * a := angle
	 * c := cos(a)
	 * s := sin(a)
	 *
	 * curMatrix = curMatrix . rotationMatrix(a, 0, 1, 0)
	 *
	 *                         [  c  0  s  0 ]
	 *                         [  0  1  0  0 ]
	 * curMatrix = curMatrix . [ -s  0  c  0 ]
	 *                         [  0  0  0  1 ]
	 */
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

		glRotatef(UNDEG(y), 0.0f, 1.0f, 0.0f);
	}
}


//*************************************************************************
//*** matrix rotate z (roll) current transformation matrix
//*
//******
void pie_MatRotZ(uint16_t z)
{
	/*
	 * a := angle
	 * c := cos(a)
	 * s := sin(a)
	 *
	 * curMatrix = curMatrix . rotationMatrix(a, 0, 0, 1)
	 *
	 *                         [ c  -s  0  0 ]
	 *                         [ s   c  0  0 ]
	 * curMatrix = curMatrix . [ 0   0  1  0 ]
	 *                         [ 0   0  0  1 ]
	 */
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

		glRotatef(UNDEG(z), 0.0f, 0.0f, 1.0f);
	}
}


//*************************************************************************
//*** matrix rotate x (pitch) current transformation matrix
//*
//******
void pie_MatRotX(uint16_t x)
{
	/*
	 * a := angle
	 * c := cos(a)
	 * s := sin(a)
	 *
	 * curMatrix = curMatrix . rotationMatrix(a, 0, 0, 1)
	 *
	 *                         [ 1  0   0  0 ]
	 *                         [ 0  c  -s  0 ]
	 * curMatrix = curMatrix . [ 0  s   c  0 ]
	 *                         [ 0  0   0  1 ]
	 */
	if (x != 0.f)
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

		glRotatef(UNDEG(x), 1.0f, 0.0f, 0.0f);
	}
}

//*************************************************************************
//*** get current transformation matrix
//*
//******
void pie_GetMatrix(float *matrix)
{
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
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
	/*
	 * v = curMatrix . v3d
	 */
	Vector3i v(
		v3d->x * psMatrix->a + v3d->y * psMatrix->d + v3d->z * psMatrix->g + psMatrix->j,
		v3d->x * psMatrix->b + v3d->y * psMatrix->e + v3d->z * psMatrix->h + psMatrix->k,
		v3d->x * psMatrix->c + v3d->y * psMatrix->f + v3d->z * psMatrix->i + psMatrix->l
	);

	const int zz = v.z >> STRETCHED_Z_SHIFT;

	if (zz < MIN_STRETCHED_Z)
	{
		v2d->x = LONG_WAY; //just along way off screen
		v2d->y = LONG_WAY;
	}
	else
	{
		v2d->x = rendSurface.xcentre + (v.x / zz);
		v2d->y = rendSurface.ycentre - (v.y / zz);
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
		(2 * rendSurface.xcentre-width) / width,
		(height - 2 * rendSurface.ycentre) / height,
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
}

void pie_BeginInterface(void)
{
	glDepthRange(0, 0.1);
}

void pie_SetGeometricOffset(int x, int y)
{
	rendSurface.xcentre = x;
	rendSurface.ycentre = y;
}

/** Sets up transformation matrices */
void pie_MatInit(void)
{
	psMatrix = &aMatrixStack[0];
	glLoadIdentity();
}

