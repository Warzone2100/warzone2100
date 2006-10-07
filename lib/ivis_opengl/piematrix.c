/***************************************************************************/
/*
 * pieMatrix.c
 *
 * matrix functions for pumpkin image library.
 *
 */
/***************************************************************************/

#include <stdio.h>
#ifdef _MSC_VER
#include <windows.h>  //needed for gl.h!  --Qamly
#endif
#include <SDL/SDL_opengl.h>

#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/pieclip.h"
#include "piematrix.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/bug.h"

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

#define MATRIX_MAX	8
#define	ONE_PERCENT	41	// 4096/100

static SDMATRIX	aMatrixStack[MATRIX_MAX];
SDMATRIX *psMatrix = &aMatrixStack[0];

BOOL drawing_interface = TRUE;

void pie_VectorNormalise(iVector *v)

{
	int32 size;
	iVector av;

	av.x = pie_ABS(v->x);
	av.y = pie_ABS(v->y);
	av.z = pie_ABS(v->z);
	if (av.x >= av.y) {
		if (av.x > av.z)
			size = av.x + (av.z >> 2) + (av.y >> 2);
		else
			size = av.z + (av.x >> 2) + (av.y >> 2);
	} else {
		if (av.y > av.z)
			size = av.y + (av.z >> 2) + (av.x >> 2);
		else
			size = av.z + (av.y >> 2) + (av.x >> 2);
	}

	if (size > 0) {
		v->x = (v->x << FP12_SHIFT) / size;
		v->y = (v->y << FP12_SHIFT) / size;
		v->z = (v->z << FP12_SHIFT) / size;
	}
}


//*************************************************************************
//*** calculate surface normal
//*
//* params	p1,p2,p3	= points for forming 2 vector for cross product
//*			v			= normal vector returned << FP12_SHIFT
//*
//* eg		if a polygon (with n points in clockwise order) normal
//*			is required, p1 = point 0, p2 = point 1, p3 = point n-1
//*
//******

void pie_SurfaceNormal(iVector *p1, iVector *p2, iVector *p3, iVector *v)

{
	iVector a, b;

	a.x = p3->x - p1->x;
	a.y = p3->y - p1->y;
	a.z = p3->z - p1->z;
	pie_VectorNormalise(&a);

 	b.x = p2->x - p1->x;
	b.y = p2->y - p1->y;
	b.z = p2->z - p1->z;
	pie_VectorNormalise(&b);

	v->x = ((a.y * b.z) - (a.z * b.y)) >> FP12_SHIFT;
	v->y = ((a.z * b.x) - (a.x * b.z)) >> FP12_SHIFT;
	v->z = ((a.x * b.y) - (a.y * b.x)) >> FP12_SHIFT;
	pie_VectorNormalise(v);
}









#define SC_TABLESIZE	4096

//*************************************************************************

#define X_INTERCEPT(pt1,pt2,yy)														\
	(pt2->x - (((pt2->y - yy)  * (pt1->x - pt2->x)) / (pt1->y - pt2->y)))

//*************************************************************************

static SDMATRIX	_MATRIX_ID = {FP12_MULTIPLIER,0,0, 0,FP12_MULTIPLIER,0, 0,0,FP12_MULTIPLIER, 0L,0L,0L};
static SDWORD	_MATRIX_INDEX;

//*************************************************************************

SDWORD	aSinTable[SC_TABLESIZE + (SC_TABLESIZE/4)];

//*************************************************************************
//*** reset transformation matrix stack and make current identity
//*
//******

static void pie_MatReset(void)

{
// printf("pie_MatReset\n");
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
	if (_MATRIX_INDEX > 3)
	{
		ASSERT( _MATRIX_INDEX < MATRIX_MAX,"pie_MatBegin past top of the stack" );
	}
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
	ASSERT( _MATRIX_INDEX >= 0,"pie_MatEnd of the bottom of the stack" );
	psMatrix--;

	glPopMatrix();
}


void pie_MATTRANS(int x, int y, int z) {
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

void pie_TRANSLATE(int x, int y, int z) {
	psMatrix->j += ((x) * psMatrix->a + (y) * psMatrix->d + (z) * psMatrix->g);
	psMatrix->k += ((x) * psMatrix->b + (y) * psMatrix->e + (z) * psMatrix->h);
	psMatrix->l += ((x) * psMatrix->c + (y) * psMatrix->f + (z) * psMatrix->i);

	glTranslatef(x, y, z);
}

//*************************************************************************
//*** matrix scale current transformation matrix
//*
//******
void	pie_MatScale( UDWORD percent )
{
SDWORD	scaleFactor;

	if(percent == 100)
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
// printf("pie_MatRotY %i\n", y);
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
// printf("pie_MatRotZ %i\n", z);
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
// printf("pie_MatRotX %i\n", x);
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


//*************************************************************************
//*** 3D vector perspective projection
//*
//* params	v1 = 3D vector to project
//* 			v2 = pointer to 2D resultant vector
//*
//* on exit	v2 = projected vector
//*
//* returns	rotated and translated z component of v1
//*
//******

int32 pie_RotateProject(SDWORD x, SDWORD y, SDWORD z, SDWORD* xs, SDWORD* ys)
{
	int32 zfx, zfy;
	int32 zz, _x, _y, _z;


	_x = x * psMatrix->a+y * psMatrix->d+z * psMatrix->g + psMatrix->j;
	_y = x * psMatrix->b+y * psMatrix->e+z * psMatrix->h + psMatrix->k;
	_z = x * psMatrix->c+y * psMatrix->f+z * psMatrix->i + psMatrix->l;

	zz = _z >> STRETCHED_Z_SHIFT;

	zfx = _z >> psRendSurface->xpshift;
	zfy = _z >> psRendSurface->ypshift;

	if ((zfx<=0) || (zfy<=0))
	{
		*xs = LONG_WAY; //just along way off screen
		*ys = LONG_WAY;
	}
	else if (zz < MIN_STRETCHED_Z)
	{
		*xs = LONG_WAY; //just along way off screen
		*ys = LONG_WAY;
	}
	else
	{
		*xs = psRendSurface->xcentre + (_x / zfx);
		*ys = psRendSurface->ycentre - (_y / zfy);
	}

	return zz;
}

int32 pie_RotProj(iVector *v3d, iPoint *v2d)
{
	return pie_RotateProject(v3d->x, v3d->y, v3d->z, &(v2d->x), &(v2d->y));
}

static int32 pie_Transform(iVector *v3d, iPoint *v2d)
{
	v2d->x = v3d->x;
	v2d->y = v3d->y;
	return v3d->z;
}

//*************************************************************************

void pie_PerspectiveBegin(void) {
	float width = pie_GetVideoBufferWidth();
	float height = pie_GetVideoBufferHeight();
	float xangle = width/6;
	float yangle = height/6;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef((2*psRendSurface->xcentre-width)/width,
		     (height-2*psRendSurface->ycentre)/height , 0);
	glFrustum(-xangle, xangle, -yangle, yangle, 330, 100000);
	glScalef(1, 1, -1);
	glMatrixMode(GL_MODELVIEW);
}

void pie_PerspectiveEnd(void) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight(), 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
}

void pie_Begin3DScene(void) {
	glDepthRange(0.1, 1);
	drawing_interface = FALSE;
}

void pie_BeginInterface(void) {
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

BOOL pie_Clockwise(iVertex *s)
{
	return (((s[1].y - s[0].y) * (s[2].x - s[1].x)) <=
			((s[1].x - s[0].x) * (s[2].y - s[1].y)));
}

BOOL pie_PieClockwise(PIEVERTEX *s)
{
	return (((s[1].sy - s[0].sy) * (s[2].sx - s[1].sx)) <=
			((s[1].sx - s[0].sx) * (s[2].sy - s[1].sy)));
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

void pie_VectorInverseRotate0(iVector *v1, iVector *v2)
{
	int32 x, y, z;

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
  	conv = (float)(PI / (0.5 * SC_TABLESIZE));

	for (i=0; i<scsize; i++) {
		v = (double) sin(i * conv) * FP12_MULTIPLIER;

		if (v >= 0.0)
			aSinTable[i] = (int32)(v + 0.5);
		else
			aSinTable[i] = (int32)(v - 0.5);
	}

	// init matrix/quat stack

	pie_MatReset();


	iV_DEBUG0("geo[_geo_setup] = setup successful\n");
}




