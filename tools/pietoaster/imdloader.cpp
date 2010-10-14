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

/*!
 * \file imdload.c
 *
 * Load IMD (.pie) files
 *
 * - Changes at version 4:
 *   - pcx name as string
 *   - pcx filepath
 *   - cut down vertex list
 *
 * - Changes at version 5_pre:
 *   - float coordinate support
 *   - ...
 */

#include "wzglobal.h"
#include "macros.h"
#include "pie_types.h"
#include "imdloader.h" // for imd structures
#include "texture.h"

//#include "tex.h" // texture page loading


// Static variables
static VERTEXID vertexTable[iV_IMD_MAX_POINTS];

/*!
 * returns true if both vectors are equal
 */
static inline bool Vector3i_compare(const Vector3i *a, const Vector3i *b)
{
	return a->x == b->x && a->y == b->y && a->z == b->z;
}


/*!
 * returns true if both vectors are equal
 */
static inline bool Vector3f_compare(const Vector3f *a, const Vector3f *b)
{
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

/*!
 * Set the vector field by field, same as v = (Vector3f){x, y, z};
 * Needed for MSVC which doesn't support C99 struct assignments.
 * \param[out] v Vector to set
 * \param[in] x,y,z Values to set to
 */
static inline void Vector3f_Set(Vector3f* v, const float x, const float y, const float z)
{
	v->x = x;
	v->y = y;
	v->z = z;
}

// Begin of weird Vector3 normalize

static void pie_VectorNormalise3iv(Vector3i *v)
{
	Sint32 size;
	Vector3i av = {abs(v->x), abs(v->y), abs(v->z)};

	if (av.x >= av.y)
	{
		if (av.x > av.z)
			size = av.x + av.z/4 + av.y/4;
		else
			size = av.z + av.x/4 + av.y/4;
	}
	else
	{
		if (av.y > av.z)
			size = av.y + av.z/4 + av.x/4;
		else
			size = av.z + av.y/4 + av.x/4;
	}

	if (size > 0)
	{
		v->x = (v->x * FP12_MULTIPLIER) / size;
		v->y = (v->y * FP12_MULTIPLIER) / size;
		v->z = (v->z * FP12_MULTIPLIER) / size;
	}
}

static void pie_VectorNormalise3fv(Vector3f *v)
{
	float size;
	Vector3f av = {fabs(v->x), fabs(v->y), fabs(v->z)};

	if (av.x >= av.y) {
		if (av.x > av.z)
			size = av.x + av.z/4 + av.y/4;
		else
			size = av.z + av.x/4 + av.y/4;
	} else {
		if (av.y > av.z)
			size = av.y + av.z/4 + av.x/4;
		else
			size = av.z + av.y/4 + av.x/4;
	}

	if (size > 0) {
		v->x = (v->x * FP12_MULTIPLIER) / size;
		v->y = (v->y * FP12_MULTIPLIER) / size;
		v->z = (v->z * FP12_MULTIPLIER) / size;
	}
}

/*!
 * Calculate surface normal
 * Eg. if a polygon (with n points in clockwise order) normal is required,
 * p1 = point 0, p2 = point 1, p3 = point n-1
 * \param[in] p1,p1,p3 points for forming 2 vector for cross product
 * \param[out] v normal vector returned << FP12_SHIFT
 */
static void pie_SurfaceNormal3fv(Vector3f *p1, Vector3f *p2, Vector3f *p3, Vector3f *v)
{
	Vector3f a = {p3->x - p1->x,
				p3->y - p1->y,
				p3->z - p1->z},
			b ={p2->x - p1->x,
				p2->y - p1->y,
				p2->z - p1->z};

	pie_VectorNormalise3fv(&a);
	pie_VectorNormalise3fv(&b);

	v->x = ((a.y * b.z) - (a.z * b.y)) / FP12_MULTIPLIER;
	v->y = ((a.z * b.x) - (a.x * b.z)) / FP12_MULTIPLIER;
	v->z = ((a.x * b.y) - (a.y * b.x)) / FP12_MULTIPLIER;
	pie_VectorNormalise3fv(v);
}

// End of weird Vector3 normalize

static bool AtEndOfFile(const char *CurPos, const char *EndOfFile)
{
	while ( *CurPos == 0x00 || *CurPos == 0x09 || *CurPos == 0x0a || *CurPos == 0x0d || *CurPos == 0x20 )
	{
		CurPos++;
		if (CurPos >= EndOfFile)
		{
			return true;
		}
	}

	if (CurPos >= EndOfFile)
	{
		return true;
	}
	return false;
}


/*!
 * Load shape level polygons
 * \param ppFileData Pointer to the data (usualy read from a file)
 * \param s Pointer to shape level
 * \return false on error (memory allocation failure/bad file format), true otherwise
 * \pre ppFileData loaded
 * \pre s allocated
 * \pre s->npolys set
 * \post s->polys allocated (iFSDPoly * s->npolys)
 * \post s->pindex allocated for each poly
 */
static bool _imd_load_polys( const char **ppFileData, iIMDShape *s )
{
	const char *pFileData = *ppFileData;
	int i, j, cnt;
	iIMDPoly *poly;

	s->numFrames = 0;
	s->animInterval = 0;

	s->polys = (iIMDPoly*)malloc(sizeof(iIMDPoly) * s->npolys);
	if (s->polys == NULL)
	{
		fprintf(stderr, "(_load_polys) Out of memory (polys)\n");
		return false;
	}

	for (i = 0, poly = s->polys; i < s->npolys; i++, poly++)
	{
		Uint32 flags, npnts;

		if (sscanf(pFileData, "%x %d%n", &flags, &npnts, &cnt) != 2)
		{
			fprintf(stderr, "(_load_polys) [poly %d] error loading flags and npoints", i);
		}
		pFileData += cnt;

		poly->flags = flags;
		poly->npnts = npnts;

		poly->pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * poly->npnts);
		if (poly->pindex == NULL)
		{
			fprintf(stderr, "(_load_polys) [poly %d] memory alloc fail (poly indices)", i);
			return false;
		}

		for (j = 0; j < poly->npnts; j++)
		{
			int newID;

			if (sscanf(pFileData, "%d%n", &newID, &cnt) != 1)
			{
				fprintf(stderr, "failed poly %d. point %d", i, j);
				return false;
			}
			pFileData += cnt;
			poly->pindex[j] = vertexTable[newID];
		}

		// calc poly normal
		if (poly->npnts > 2)
		{
			Vector3f p0, p1, p2;

			//assumes points already set
			p0.x = s->points[poly->pindex[0]].x;
			p0.y = s->points[poly->pindex[0]].y;
			p0.z = s->points[poly->pindex[0]].z;

			p1.x = s->points[poly->pindex[1]].x;
			p1.y = s->points[poly->pindex[1]].y;
			p1.z = s->points[poly->pindex[1]].z;

			p2.x = s->points[poly->pindex[poly->npnts-1]].x;
			p2.y = s->points[poly->pindex[poly->npnts-1]].y;
			p2.z = s->points[poly->pindex[poly->npnts-1]].z;

			pie_SurfaceNormal3fv(&p0, &p1, &p2, &poly->normal);
		}
		else
		{
			Vector3f_Set(&poly->normal, 0.0f, 0.0f, 0.0f);
		}

		if (poly->flags & iV_IMD_TEXANIM)
		{
			unsigned int nFrames, pbRate, tWidth, tHeight;

			poly->pTexAnim = (iTexAnim*)malloc(sizeof(iTexAnim));
			if (poly->pTexAnim == NULL)
			{
				fprintf(stderr, "(_load_polys) [poly %d] memory alloc fail (iTexAnim struct)", i);
				return false;
			}

			// even the psx needs to skip the data
			if (sscanf(pFileData, "%d %d %d %d%n", &nFrames, &pbRate, &tWidth, &tHeight, &cnt) != 4)
			{
				fprintf(stderr, "(_load_polys) [poly %d] error reading texanim data", i);
				return false;
			}
			pFileData += cnt;

			//ASSERT( tWidth > 0, "_imd_load_polys: texture width = %i", tWidth );
			//ASSERT( tHeight > 0, "_imd_load_polys: texture height = %i", tHeight );

			/* Assumes same number of frames per poly */
			s->numFrames = nFrames;
			poly->pTexAnim->nFrames = nFrames;
			poly->pTexAnim->playbackRate =pbRate;

			/* Uses Max metric playback rate */
			s->animInterval = pbRate;
			poly->pTexAnim->textureWidth = tWidth;
			poly->pTexAnim->textureHeight = tHeight;
		}
		else
		{
			poly->pTexAnim = NULL;
		}

		// PC texture coord routine
		if (poly->flags & iV_IMD_TEX)
		{
			poly->texCoord = (Vector2f*)malloc(sizeof(Vector2f) * poly->npnts);
			if (poly->texCoord == NULL)
			{
				fprintf(stderr, "(_load_polys) [poly %d] memory alloc fail (vertex struct)", i);
				return false;
			}

			for (j = 0; j < poly->npnts; j++)
			{
				float VertexU, VertexV;
				if (sscanf(pFileData, "%f %f%n", &VertexU, &VertexV, &cnt) != 2)
				{
					fprintf(stderr, "(_load_polys) [poly %d] error reading tex outline", i);
					return false;
				}
				pFileData += cnt;

				poly->texCoord[j].x = VertexU;
				poly->texCoord[j].y = VertexV;
			}
		}
		else
		{
			poly->texCoord = NULL;
		}
	}

	*ppFileData = pFileData;

	return true;
}


static bool ReadPoints( const char **ppFileData, iIMDShape *s )
{
	const char *pFileData = *ppFileData;
	int cnt, i, j, lastPoint = 0, match = -1;
	Vector3f newVector = {0.0f, 0.0f, 0.0f};

	for (i = 0; i < s->npoints; i++)
	{
		if (sscanf(pFileData, "%f %f %f%n", &newVector.x, &newVector.y, &newVector.z, &cnt) != 3)
		{
			fprintf(stderr, "(_load_points) file corrupt -K\n");
			return false;
		}
		pFileData += cnt;

		//check for duplicate points
		match = -1;

		// scan through list upto the number of points added (lastPoint) ... not up to the number of points scanned in (i)  (which will include duplicates)
		for (j = 0; j < lastPoint; j++)
		{
			if (Vector3f_compare(&newVector, &s->points[j]))
			{
				//HACKS!!! always -1 for now this duplicated point removal code(optimization in renderer) screws up imd save
				//match = j;
				match = -1;
				break;
			}
		}

		//check for duplicate points
		if (match == -1)
		{
			// new point
			s->points[lastPoint].x = newVector.x;
			s->points[lastPoint].y = newVector.y;
			s->points[lastPoint].z = newVector.z;
			vertexTable[i] = lastPoint;
			lastPoint++;
		}
		else
		{
			vertexTable[i] = match;
		}
	}

	//clear remaining table
	for (i = s->npoints; i < iV_IMD_MAX_POINTS; i++)
	{
		vertexTable[i] = -1;
	}

	s->npoints = lastPoint;

	*ppFileData = pFileData;

	return true;
}


static bool _imd_load_points( const char **ppFileData, iIMDShape *s )
{
	Vector3f *p = NULL;
	float tempXMax, tempXMin, tempZMax, tempZMin;
	float xmax, ymax, zmax;
	double dx, dy, dz, rad_sq, rad, old_to_p_sq, old_to_p, old_to_new;
	double xspan, yspan, zspan, maxspan;
	Vector3f dia1, dia2, cen;
	Vector3f vxmin = { 0, 0, 0 }, vymin = { 0, 0, 0 }, vzmin = { 0, 0, 0 },
	         vxmax = { 0, 0, 0 }, vymax = { 0, 0, 0 }, vzmax = { 0, 0, 0 };

	//load the points then pass through a second time to setup bounding datavalues
	s->points = (Vector3f*)malloc(sizeof(Vector3f) * s->npoints);
	if (s->points == NULL)
	{
		return false;
	}

	// Read in points and remove duplicates (!)
	if ( ReadPoints( ppFileData, s ) == false )
	{
		return false;
	}

	s->xmax = s->ymax = s->zmax = tempXMax = tempZMax = -FP12_MULTIPLIER;
	s->xmin = s->ymin = s->zmin = tempXMin = tempZMin = FP12_MULTIPLIER;

	vxmax.x = vymax.y = vzmax.z = -FP12_MULTIPLIER;
	vxmin.x = vymin.y = vzmin.z = FP12_MULTIPLIER;

	// set up bounding data for minimum number of vertices
	for (p = s->points; p < s->points + s->npoints; p++)
	{
		if (p->x > s->xmax)
			s->xmax = p->x;
		if (p->x < s->xmin)
			s->xmin = p->x;

		/* Biggest x coord so far within our height window? */
		if( p->x > tempXMax && p->y > DROID_VIS_LOWER && p->y < DROID_VIS_UPPER )
		{
			tempXMax = p->x;
		}

		/* Smallest x coord so far within our height window? */
		if( p->x < tempXMin && p->y > DROID_VIS_LOWER && p->y < DROID_VIS_UPPER )
		{
			tempXMin = p->x;
		}

		if (p->y > s->ymax)
			s->ymax = p->y;
		if (p->y < s->ymin)
			s->ymin = p->y;

		if (p->z > s->zmax)
			s->zmax = p->z;
		if (p->z < s->zmin)
			s->zmin = p->z;

		/* Biggest z coord so far within our height window? */
		if( p->z > tempZMax && p->y > DROID_VIS_LOWER && p->y < DROID_VIS_UPPER )
		{
			tempZMax = p->z;
		}

		/* Smallest z coord so far within our height window? */
		if( p->z < tempZMax && p->y > DROID_VIS_LOWER && p->y < DROID_VIS_UPPER )
		{
			tempZMin = p->z;
		}

		// for tight sphere calculations
		if (p->x < vxmin.x)
		{
			vxmin.x = p->x;
			vxmin.y = p->y;
			vxmin.z = p->z;
		}

		if (p->x > vxmax.x)
		{
			vxmax.x = p->x;
			vxmax.y = p->y;
			vxmax.z = p->z;
		}

		if (p->y < vymin.y)
		{
			vymin.x = p->x;
			vymin.y = p->y;
			vymin.z = p->z;
		}

		if (p->y > vymax.y)
		{
			vymax.x = p->x;
			vymax.y = p->y;
			vymax.z = p->z;
		}

		if (p->z < vzmin.z)
		{
			vzmin.x = p->x;
			vzmin.y = p->y;
			vzmin.z = p->z;
		}

		if (p->z > vzmax.z)
		{
			vzmax.x = p->x;
			vzmax.y = p->y;
			vzmax.z = p->z;
		}
	}

	// no need to scale an IMD shape (only FSD)
	xmax = MAX(s->xmax, -s->xmin);
	ymax = MAX(s->ymax, -s->ymin);
	zmax = MAX(s->zmax, -s->zmin);

	s->radius = MAX(xmax, (MAX(ymax, zmax)));
	s->sradius = sqrtf(xmax*xmax + ymax*ymax + zmax*zmax);

// START: tight bounding sphere

	// set xspan = distance between 2 points xmin & xmax (squared)
	dx = vxmax.x - vxmin.x;
	dy = vxmax.y - vxmin.y;
	dz = vxmax.z - vxmin.z;
	xspan = dx*dx + dy*dy + dz*dz;

	// same for yspan
	dx = vymax.x - vymin.x;
	dy = vymax.y - vymin.y;
	dz = vymax.z - vymin.z;
	yspan = dx*dx + dy*dy + dz*dz;

	// and ofcourse zspan
	dx = vzmax.x - vzmin.x;
	dy = vzmax.y - vzmin.y;
	dz = vzmax.z - vzmin.z;
	zspan = dx*dx + dy*dy + dz*dz;

	// set points dia1 & dia2 to maximally seperated pair
	// assume xspan biggest
	dia1 = vxmin;
	dia2 = vxmax;
	maxspan = xspan;

	if (yspan > maxspan)
	{
		maxspan = yspan;
		dia1 = vymin;
		dia2 = vymax;
	}

	if (zspan > maxspan)
	{
		maxspan = zspan;
		dia1 = vzmin;
		dia2 = vzmax;
	}

	// dia1, dia2 diameter of initial sphere
	cen.x = (dia1.x + dia2.x) / 2.;
	cen.y = (dia1.y + dia2.y) / 2.;
	cen.z = (dia1.z + dia2.z) / 2.;

	// calc initial radius
	dx = dia2.x - cen.x;
	dy = dia2.y - cen.y;
	dz = dia2.z - cen.z;

	rad_sq = dx*dx + dy*dy + dz*dz;
	rad = sqrt(rad_sq);

	// second pass (find tight sphere)
	for (p = s->points; p < s->points + s->npoints; p++)
	{
		dx = p->x - cen.x;
		dy = p->y - cen.y;
		dz = p->z - cen.z;
		old_to_p_sq = dx*dx + dy*dy + dz*dz;

		// do r**2 first
		if (old_to_p_sq>rad_sq)
		{
			// this point outside current sphere
			old_to_p = sqrt(old_to_p_sq);
			// radius of new sphere
			rad = (rad + old_to_p) / 2.;
			// rad**2 for next compare
			rad_sq = rad*rad;
			old_to_new = old_to_p - rad;
			// centre of new sphere
			cen.x = (rad*cen.x + old_to_new*p->x) / old_to_p;
			cen.y = (rad*cen.y + old_to_new*p->y) / old_to_p;
			cen.z = (rad*cen.z + old_to_new*p->z) / old_to_p;
			fprintf(stderr, "NEW SPHERE: cen,rad = %f %f %f, %f\n", cen.x, cen.y, cen.z, rad);
		}
	}

	s->ocen = cen;
	fprintf(stderr, "radius, sradius, %d, %d\n", s->radius, s->sradius);
	fprintf(stderr, "SPHERE: cen,rad = %f %f %f\n", s->ocen.x, s->ocen.y, s->ocen.z);

// END: tight bounding sphere

	return true;
}


/*!
 * Load shape level connectors
 * \param ppFileData Pointer to the data (usualy read from a file)
 * \param s Pointer to shape level
 * \return false on error (memory allocation failure/bad file format), true otherwise
 * \pre ppFileData loaded
 * \pre s allocated
 * \pre s->nconnectors set
 * \post s->connectors allocated
 */
static bool _imd_load_connectors(const char **ppFileData, iIMDShape *s)
{
	const char *pFileData = *ppFileData;
	int cnt;
	Vector3f *p = NULL, newVector = {0.0f, 0.0f, 0.0f};

	s->connectors = (Vector3f*)malloc(sizeof(Vector3f) * s->nconnectors);
	if (s->connectors == NULL)
	{
		fprintf(stderr, "(_load_connectors) MALLOC fail\n");
		return false;
	}

	for (p = s->connectors; p < s->connectors + s->nconnectors; p++)
	{
		if (sscanf(pFileData, "%f %f %f%n", &newVector.x, &newVector.y, &newVector.z, &cnt) != 3)
		{
			fprintf(stderr, "(_load_connectors) file corrupt -M\n");
			return false;
		}
		pFileData += cnt;
		*p = newVector;
	}

	*ppFileData = pFileData;

	return true;
}


/*!
 * Load shape levels recursively
 * \param ppFileData Pointer to the data (usualy read from a file)
 * \param FileDataEnd ???
 * \param nlevels Number of levels to load
 * \return pointer to iFSDShape structure (or NULL on error)
 * \pre ppFileData loaded
 * \post s allocated
 */
static iIMDShape *_imd_load_level(const char **ppFileData, const char *FileDataEnd, int nlevels)
{
	const char *pFileData = *ppFileData;
	char buffer[MAX_PATH] = {'\0'};
	int cnt = 0, n = 0;
	iIMDShape *s = NULL;

	if (nlevels == 0)
		return NULL;

	s = (iIMDShape*)malloc(sizeof(iIMDShape));
	if (s == NULL)
	{
		/* Failed to allocate memory for s */
		fprintf(stderr, "_imd_load_level: Memory allocation error\n");
		return NULL;
	}

	s->nconnectors = 0; // Default number of connectors must be 0
	s->npoints = 0;
	s->npolys = 0;

	s->points = NULL;
	s->polys = NULL;
	s->connectors = NULL;
	s->next = NULL;

	s->shadowEdgeList = NULL;
	s->nShadowEdges = 0;
	s->texpage = iV_TEX_INVALID;


	if (sscanf(pFileData, "%s %d%n", buffer, &s->npoints, &cnt) != 2)
	{
		fprintf(stderr, "_imd_load_level(2): file corrupt\n");
		return NULL;
	}
	pFileData += cnt;

	// load points
	if (strcmp(buffer, "POINTS") != 0)
	{
		fprintf(stderr, "_imd_load_level: expecting 'POINTS' directive, got: %s", buffer);
		return NULL;
	}

	if (s->npoints > iV_IMD_MAX_POINTS)
	{
		fprintf(stderr, "_imd_load_level: too many points in IMD\n");
		return NULL;
	}

	_imd_load_points( &pFileData, s );


	if (sscanf(pFileData, "%s %d%n", buffer, &s->npolys, &cnt) != 2)
	{
		fprintf(stderr, "_imd_load_level(3): file corrupt\n");
		return NULL;
	}
	pFileData += cnt;

	if (strcmp(buffer, "POLYGONS") != 0)
	{
		fprintf(stderr,"_imd_load_level: expecting 'POLYGONS' directive\n");
		return NULL;
	}

	_imd_load_polys( &pFileData, s );


	// NOW load optional stuff
	while (!AtEndOfFile(pFileData, FileDataEnd)) // check for end of file (give or take white space)
	{
		// Scans in the line ... if we don't get 2 parameters then quit
		if (sscanf(pFileData, "%s %d%n", buffer, &n, &cnt) != 2)
		{
			break;
		}
		pFileData += cnt;

		// check for next level ... or might be a BSP - This should handle an imd if it has a BSP tree attached to it
		// might be "BSP" or "LEVEL"
		if (strcmp(buffer, "LEVEL") == 0)
		{
			fprintf(stderr, "imd[_load_level] = npoints %d, npolys %d\n", s->npoints, s->npolys);
			s->next = _imd_load_level(&pFileData, FileDataEnd, nlevels - 1);
		}
		else if (strcmp(buffer, "CONNECTORS") == 0)
		{
			//load connector stuff
			s->nconnectors = n;
			_imd_load_connectors( &pFileData, s );
		}
		else
		{
			fprintf(stderr, "(_load_level) unexpected directive %s %d", buffer, n);
			break;
		}
	}

	*ppFileData = pFileData;

	return s;
}


/*!
 * Load ppFileData into a shape
 * \param ppFileData Data from the IMD file
 * \param FileDataEnd Endpointer
 * \return The shape, constructed from the data read
 */
// ppFileData is incremented to the end of the file on exit!
iIMDShape *iV_ProcessIMD( const char *filename )
{
	const char *pFileData, *pFileDataEnd;
	char buffer[MAX_PATH] = "-_-", texfile[MAX_PATH];
	int cnt, nlevels, filelength;
	iIMDShape *shape, *psShape;
	Uint32 level;
	Sint32 imd_version;
	Uint32 imd_flags; // FIXME UNUSED
	bool bTextured = false;
	FILE *FileToOpen = NULL;

	fprintf(stderr, "filename: %s", filename);
	printf("filename: %s", filename);

	FileToOpen = fopen(filename, "rb");
	if (FileToOpen == NULL)
	{
		fprintf(stderr, "iV_ProcessIMD %s file doesn't exist: (%s)\n", filename);
		return NULL;
	}

	pFileData = (char*)malloc(1024*1024);

	filelength = fread((void*)pFileData, 1, 1024*1024,FileToOpen);

	pFileDataEnd = pFileData + filelength;

	if (sscanf(pFileData, "%s %d%n", buffer, &imd_version, &cnt) != 2)
	{
		fprintf(stderr, "iV_ProcessIMD %s bad version: (%s)\n", filename, buffer);
		return NULL;
	}
	pFileData += cnt;

	if (strcmp(IMD_NAME, buffer) != 0 && strcmp(PIE_NAME, buffer) !=0 )
	{
		fprintf(stderr, "iV_ProcessIMD %s not an IMD file (%s %d)\n", filename, buffer, imd_version);
		return NULL;
	}

	//Now supporting version 4 files
	if (imd_version < 2 || imd_version > 5)
	{
		fprintf(stderr, "iV_ProcessIMD %s version %d not supported\n", filename, imd_version);
		return NULL;
	}

	/* Flags are ignored now. Reading them in just to pass the buffer. */
	if (sscanf(pFileData, "%s %x%n", buffer, &imd_flags, &cnt) != 2)
	{
		fprintf(stderr, "iV_ProcessIMD %s bad flags: %s\n", filename, buffer);
		return NULL;
	}
	pFileData += cnt;

	/* This can be either texture or levels */
	if (sscanf(pFileData, "%s %d%n", buffer, &nlevels, &cnt) != 2)
	{
		fprintf(stderr, "iV_ProcessIMD %s expecting TEXTURE or LEVELS: %s\n", filename, buffer);
		return NULL;
	}
	pFileData += cnt;

	// get texture page if specified
	if (strncmp(buffer, "TEXTURE", 7) == 0)
	{
		int i, pwidth, pheight;
		char ch, texType[MAX_PATH];

		/* the first parameter for textures is always ignored; which is why we ignore
		 * nlevels read in above */
		ch = *pFileData++;

		// Run up to the dot or till the buffer is filled. Leave room for the extension.
		for( i = 0; i < MAX_PATH-5 && (ch = *pFileData++) != EOF && ch != '.'; i++ )
		{
 			texfile[i] = (char)ch;
		}
		texfile[i] = '\0';

		if (sscanf(pFileData, "%s%n", texType, &cnt) != 1)
		{
			fprintf(stderr, "iV_ProcessIMD %s texture info corrupt: %s\n", filename, buffer);
			return NULL;
		}
		pFileData += cnt;

		//more texture types using SDL_Image
		if (!strcmp(texType, "png"))
		{
			strcat(texfile, ".png");
		}
		else if (!strcmp(texType, "bmp"))
		{
			strcat(texfile, ".bmp");
		}
		else if (!strcmp(texType, "pcx"))
		{
			strcat(texfile, ".pcx");
		}
		else if (!strcmp(texType, "jpg"))
		{
			strcat(texfile, ".jpg");
		}
		else if (!strcmp(texType, "gif"))
		{
			strcat(texfile, ".gif");
		}
		else
		{
			fprintf(stderr, "iV_ProcessIMD %s: only png bmp pcx jpg gif textures supported\n", filename);
			return NULL;
		}

		//pie_MakeTexPageName(texfile);

		if (sscanf(pFileData, "%d %d%n", &pwidth, &pheight, &cnt) != 2)
		{
			fprintf(stderr, "iV_ProcessIMD %s bad texture size: %s\n", filename, buffer);
			return NULL;
		}
		pFileData += cnt;

		/* Now read in LEVELS directive */
		if (sscanf(pFileData, "%s %d%n", buffer, &nlevels, &cnt) != 2)
		{
			fprintf(stderr, "iV_ProcessIMD %s bad levels info: %s\n", filename, buffer);
			return NULL;
		}
		pFileData += cnt;

		bTextured = true;
	}

	if (strncmp(buffer, "LEVELS", 6) != 0)
	{
		fprintf(stderr, "iV_ProcessIMD: expecting 'LEVELS' directive (%s)", buffer);
		return NULL;
	}

	/* Read first LEVEL directive */
	if (sscanf(pFileData, "%s %d%n", buffer, &level, &cnt) != 2)
	{
		fprintf(stderr, "(_load_level) file corrupt -J\n");
		return NULL;
	}
	pFileData += cnt;

	if (strncmp(buffer, "LEVEL", 5) != 0)
	{
		fprintf(stderr, "iV_ProcessIMD(2): expecting 'LEVELS' directive (%s)", buffer);
		return NULL;
	}

	shape = _imd_load_level(&pFileData, pFileDataEnd, nlevels);
	if (shape == NULL)
	{
		fprintf(stderr, "iV_ProcessIMD %s unsuccessful\n", filename);
		return NULL;
	}

	// load texture page if specified
	if (bTextured)
	{
		int texpage = iV_GetTexture(texfile);

		if (texpage < 0)
		{
			fprintf(stderr, "iV_ProcessIMD %s could not load tex page %s\n", filename, texfile);
			return NULL;
		}
		/* assign tex page to levels */
		for (psShape = shape; psShape != NULL; psShape = psShape->next)
		{
			psShape->texpage = texpage;
		}
	}

	return shape;
}
