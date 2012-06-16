/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 */

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/piematrix.h"

#include "ivisdef.h" // for imd structures
#include "imd.h" // for imd structures
#include "tex.h" // texture page loading

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
static bool _imd_load_polys( const char **ppFileData, iIMDShape *s, int pieVersion)
{
	const char *pFileData = *ppFileData;
	unsigned int i, j;
	iIMDPoly *poly;

	s->numFrames = 0;
	s->animInterval = 0;

	s->polys = (iIMDPoly*)malloc(sizeof(iIMDPoly) * s->npolys);
	if (s->polys == NULL)
	{
		debug(LOG_ERROR, "(_load_polys) Out of memory (polys)");
		return false;
	}

	for (i = 0, poly = s->polys; i < s->npolys; i++, poly++)
	{
		unsigned int flags, npnts;
		int cnt;

		if (sscanf(pFileData, "%x %u%n", &flags, &npnts, &cnt) != 2)
		{
			debug(LOG_ERROR, "(_load_polys) [poly %u] error loading flags and npoints", i);
		}
		pFileData += cnt;

		poly->flags = flags;
		poly->npnts = npnts;
		ASSERT_OR_RETURN(false, npnts == 3, "Invalid polygon size (%d)", npnts);
		if (sscanf(pFileData, "%d %d %d%n", &poly->pindex[0], &poly->pindex[1], &poly->pindex[2], &cnt) != 3)
		{
			debug(LOG_ERROR, "failed reading triangle, point %d", i);
			return false;
		}
		pFileData += cnt;

		// calc poly normal
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

			poly->normal = pie_SurfaceNormal3fv(p0, p1, p2);
		}

		if (poly->flags & iV_IMD_TEXANIM)
		{
			int nFrames, pbRate, tWidth, tHeight;

			// even the psx needs to skip the data
			if (sscanf(pFileData, "%d %d %d %d%n", &nFrames, &pbRate, &tWidth, &tHeight, &cnt) != 4)
			{
				debug(LOG_ERROR, "(_load_polys) [poly %u] error reading texanim data", i);
				return false;
			}
			pFileData += cnt;

			ASSERT(tWidth > 0, "%s: texture width = %d", GetLastResourceFilename(), tWidth);
			ASSERT(tHeight > 0, "%s: texture height = %d (width=%d)", GetLastResourceFilename(), tHeight, tWidth);

			/* Must have same number of frames and same playback rate for all polygons */
			s->numFrames = nFrames;
			s->animInterval = pbRate;

			poly->texAnim.x = tWidth / OLD_TEXTURE_SIZE_FIX;
			poly->texAnim.y = tHeight / OLD_TEXTURE_SIZE_FIX;
		}
		else
		{
			poly->texAnim.x = 0;
			poly->texAnim.y = 0;
		}

		// PC texture coord routine
		if (poly->flags & iV_IMD_TEX)
		{
			int nFrames, framesPerLine, frame;

			nFrames = MAX(1, s->numFrames);
			poly->texCoord = (Vector2f *)malloc(sizeof(*poly->texCoord) * nFrames * poly->npnts);
			ASSERT_OR_RETURN(false, poly->texCoord, "Out of memory allocating texture coordinates");
			framesPerLine = OLD_TEXTURE_SIZE_FIX / (poly->texAnim.x * OLD_TEXTURE_SIZE_FIX);
			for (j = 0; j < poly->npnts; j++)
			{
				float VertexU, VertexV;

				if (sscanf(pFileData, "%f %f%n", &VertexU, &VertexV, &cnt) != 2)
				{
					debug(LOG_ERROR, "(_load_polys) [poly %u] error reading tex outline", i);
					return false;
				}
				pFileData += cnt;

				if (pieVersion != PIE_FLOAT_VER)
				{
					VertexU /= OLD_TEXTURE_SIZE_FIX;
					VertexV /= OLD_TEXTURE_SIZE_FIX;
				}

				for (frame = 0; frame < nFrames; frame++)
				{
					const int uFrame = (frame % framesPerLine) * (poly->texAnim.x * OLD_TEXTURE_SIZE_FIX);
					const int vFrame = (frame / framesPerLine) * (poly->texAnim.y * OLD_TEXTURE_SIZE_FIX);
					Vector2f *c = &poly->texCoord[frame * poly->npnts + j];

					c->x = VertexU + uFrame / OLD_TEXTURE_SIZE_FIX;
					c->y = VertexV + vFrame / OLD_TEXTURE_SIZE_FIX;
				}
			}
		}
		else
		{
			ASSERT_OR_RETURN(false, !(poly->flags & iV_IMD_TEXANIM), "Polygons with texture animation must have textures!");
			poly->texCoord = NULL;
		}
	}

	*ppFileData = pFileData;

	return true;
}


static bool ReadPoints( const char **ppFileData, iIMDShape *s )
{
	const char *pFileData = *ppFileData;
	unsigned int i;
	int cnt;

	for (i = 0; i < s->npoints; i++)
	{
		if (sscanf(pFileData, "%f %f %f%n", &s->points[i].x, &s->points[i].y, &s->points[i].z, &cnt) != 3)
		{
			debug(LOG_ERROR, "(_load_points) file corrupt -K");
			return false;
		}
		pFileData += cnt;
	}

	*ppFileData = pFileData;

	return true;
}


static bool _imd_load_points( const char **ppFileData, iIMDShape *s )
{
	Vector3f *p = NULL;
	int32_t xmax, ymax, zmax;
	double dx, dy, dz, rad_sq, rad, old_to_p_sq, old_to_p, old_to_new;
	double xspan, yspan, zspan, maxspan;
	Vector3f dia1, dia2, cen;
	Vector3f vxmin(0, 0, 0), vymin(0, 0, 0), vzmin(0, 0, 0),
	         vxmax(0, 0, 0), vymax(0, 0, 0), vzmax(0, 0, 0);

	//load the points then pass through a second time to setup bounding datavalues
	s->points = (Vector3f*)malloc(sizeof(Vector3f) * s->npoints);
	if (s->points == NULL)
	{
		return false;
	}

	// Read in points and remove duplicates (!)
	if ( ReadPoints( ppFileData, s ) == false )
	{
		free(s->points);
		s->points = NULL;
		return false;
	}

	s->max.x = s->max.y = s->max.z = -FP12_MULTIPLIER;
	s->min.x = s->min.y = s->min.z = FP12_MULTIPLIER;

	vxmax.x = vymax.y = vzmax.z = -FP12_MULTIPLIER;
	vxmin.x = vymin.y = vzmin.z = FP12_MULTIPLIER;

	// set up bounding data for minimum number of vertices
	for (p = s->points; p < s->points + s->npoints; p++)
	{
		if (p->x > s->max.x)
		{
			s->max.x = p->x;
		}
		if (p->x < s->min.x)
		{
			s->min.x = p->x;
		}

		if (p->y > s->max.y)
		{
			s->max.y = p->y;
		}
		if (p->y < s->min.y)
		{
			s->min.y = p->y;
		}

		if (p->z > s->max.z)
		{
			s->max.z = p->z;
		}
		if (p->z < s->min.z)
		{
			s->min.z = p->z;
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
	xmax = MAX(s->max.x, -s->min.x);
	ymax = MAX(s->max.y, -s->min.y);
	zmax = MAX(s->max.z, -s->min.z);

	s->radius = MAX(xmax, MAX(ymax, zmax));
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
	rad = sqrt((double)rad_sq);

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
			old_to_p = sqrt((double)old_to_p_sq);
			// radius of new sphere
			rad = (rad + old_to_p) / 2.;
			// rad**2 for next compare
			rad_sq = rad*rad;
			old_to_new = old_to_p - rad;
			// centre of new sphere
			cen.x = (rad*cen.x + old_to_new*p->x) / old_to_p;
			cen.y = (rad*cen.y + old_to_new*p->y) / old_to_p;
			cen.z = (rad*cen.z + old_to_new*p->z) / old_to_p;
		}
	}

	s->ocen = cen;

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
	Vector3i *p = NULL, newVector(0, 0, 0);

	s->connectors = (Vector3i *)malloc(sizeof(Vector3i) * s->nconnectors);
	if (s->connectors == NULL)
	{
		debug(LOG_ERROR, "(_load_connectors) MALLOC fail");
		return false;
	}

	for (p = s->connectors; p < s->connectors + s->nconnectors; p++)
	{
		if (sscanf(pFileData, "%d %d %d%n",                         &newVector.x, &newVector.y, &newVector.z, &cnt) != 3 &&
		    sscanf(pFileData, "%d%*[.0-9] %d%*[.0-9] %d%*[.0-9]%n", &newVector.x, &newVector.y, &newVector.z, &cnt) != 3)
		{
			debug(LOG_ERROR, "(_load_connectors) file corrupt -M");
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
static iIMDShape *_imd_load_level(const char **ppFileData, const char *FileDataEnd, int nlevels, int pieVersion)
{
	const char *pTmp, *pFileData = *ppFileData;
	char buffer[PATH_MAX] = {'\0'};
	int cnt = 0, n = 0, i;
	iIMDShape *s = NULL;

	if (nlevels == 0)
	{
		return NULL;
	}

	// Load optional MATERIALS directive
	pTmp = pFileData;	// remember position
	i = sscanf(pFileData, "%255s %n", buffer, &cnt);
	ASSERT_OR_RETURN(NULL, i == 1, "Bad directive following LEVEL");

	s = (iIMDShape*)malloc(sizeof(iIMDShape));
	if (s == NULL)
	{
		/* Failed to allocate memory for s */
		debug(LOG_ERROR, "_imd_load_level: Memory allocation error");
		return NULL;
	}
	s->flags = 0;
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
	s->tcmaskpage = iV_TEX_INVALID;
	s->normalpage = iV_TEX_INVALID;
	memset(s->material, 0, sizeof(s->material));
	s->material[LIGHT_AMBIENT][3] = 1.0f;
	s->material[LIGHT_DIFFUSE][3] = 1.0f;
	s->material[LIGHT_SPECULAR][3] = 1.0f;
	if (strcmp(buffer, "MATERIALS") == 0)
	{
		i = sscanf(pFileData, "%255s %f %f %f %f %f %f %f %f %f %f%n", buffer,
		           &s->material[LIGHT_AMBIENT][0], &s->material[LIGHT_AMBIENT][1], &s->material[LIGHT_AMBIENT][2],
		           &s->material[LIGHT_DIFFUSE][0], &s->material[LIGHT_DIFFUSE][1], &s->material[LIGHT_DIFFUSE][2],
	                   &s->material[LIGHT_SPECULAR][0], &s->material[LIGHT_SPECULAR][1], &s->material[LIGHT_SPECULAR][2],
		           &s->shininess, &cnt);
		ASSERT_OR_RETURN(NULL, i == 11, "Bad MATERIALS directive");
		pFileData += cnt;
	}
	else
	{
		// Set default values
		s->material[LIGHT_AMBIENT][0] = 1.0f;
		s->material[LIGHT_AMBIENT][1] = 1.0f;
		s->material[LIGHT_AMBIENT][2] = 1.0f;
		s->material[LIGHT_DIFFUSE][0] = 1.0f;
		s->material[LIGHT_DIFFUSE][1] = 1.0f;
		s->material[LIGHT_DIFFUSE][2] = 1.0f;
		s->material[LIGHT_SPECULAR][0] = 1.0f;
		s->material[LIGHT_SPECULAR][1] = 1.0f;
		s->material[LIGHT_SPECULAR][2] = 1.0f;
		s->shininess = 10;
		pFileData = pTmp;
	}

	if (sscanf(pFileData, "%255s %d%n", buffer, &s->npoints, &cnt) != 2)
	{
		debug(LOG_ERROR, "_imd_load_level(2): file corrupt");
		return NULL;
	}
	pFileData += cnt;

	// load points

	ASSERT_OR_RETURN(NULL, strcmp(buffer, "POINTS") == 0, "Expecting 'POINTS' directive, got: %s", buffer);

	_imd_load_points( &pFileData, s );

	if (sscanf(pFileData, "%255s %d%n", buffer, &s->npolys, &cnt) != 2)
	{
		debug(LOG_ERROR, "_imd_load_level(3): file corrupt");
		return NULL;
	}
	pFileData += cnt;

	ASSERT_OR_RETURN(NULL, strcmp(buffer, "POLYGONS") == 0, "Expecting 'POLYGONS' directive, got: %s", buffer);

	_imd_load_polys( &pFileData, s, pieVersion);


	// NOW load optional stuff
	while (!AtEndOfFile(pFileData, FileDataEnd)) // check for end of file (give or take white space)
	{
		// Scans in the line ... if we don't get 2 parameters then quit
		if (sscanf(pFileData, "%255s %d%n", buffer, &n, &cnt) != 2)
		{
			break;
		}
		pFileData += cnt;

		// check for next level ... or might be a BSP - This should handle an imd if it has a BSP tree attached to it
		// might be "BSP" or "LEVEL"
		if (strcmp(buffer, "LEVEL") == 0)
		{
			debug(LOG_3D, "imd[_load_level] = npoints %d, npolys %d", s->npoints, s->npolys);
			s->next = _imd_load_level(&pFileData, FileDataEnd, nlevels - 1, pieVersion);
		}
		else if (strcmp(buffer, "CONNECTORS") == 0)
		{
			//load connector stuff
			s->nconnectors = n;
			_imd_load_connectors( &pFileData, s );
		}
		else
		{
			debug(LOG_ERROR, "(_load_level) unexpected directive %s %d", buffer, n);
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
iIMDShape *iV_ProcessIMD( const char **ppFileData, const char *FileDataEnd )
{
	const char *pFileName = GetLastResourceFilename(); // Last loaded texture page filename
	const char *pFileData = *ppFileData;
	char buffer[PATH_MAX], texfile[PATH_MAX], normalfile[PATH_MAX];
	int cnt, nlevels;
	iIMDShape *shape, *psShape;
	UDWORD level;
	int32_t imd_version;
	uint32_t imd_flags;
	bool bTextured = false;

	memset(normalfile, 0, sizeof(normalfile));

	if (sscanf(pFileData, "%255s %d%n", buffer, &imd_version, &cnt) != 2)
	{
		debug(LOG_ERROR, "iV_ProcessIMD %s bad version: (%s)", pFileName, buffer);
		assert(false);
		return NULL;
	}
	pFileData += cnt;

	if (strcmp(PIE_NAME, buffer) != 0)
	{
		debug(LOG_ERROR, "iV_ProcessIMD %s not an IMD file (%s %d)", pFileName, buffer, imd_version);
		return NULL;
	}

	//Now supporting version PIE_VER and PIE_FLOAT_VER files
	if (imd_version != PIE_VER && imd_version != PIE_FLOAT_VER)
	{
		debug(LOG_ERROR, "iV_ProcessIMD %s version %d not supported", pFileName, imd_version);
		return NULL;
	}

	// Read flag
	if (sscanf(pFileData, "%255s %x%n", buffer, &imd_flags, &cnt) != 2)
	{
		debug(LOG_ERROR, "iV_ProcessIMD %s bad flags: %s", pFileName, buffer);
		return NULL;
	}
	pFileData += cnt;

	/* This can be either texture or levels */
	if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
	{
		debug(LOG_ERROR, "iV_ProcessIMD %s expecting TEXTURE or LEVELS: %s", pFileName, buffer);
		return NULL;
	}
	pFileData += cnt;

	// get texture page if specified
	if (strncmp(buffer, "TEXTURE", 7) == 0)
	{
		int i, pwidth, pheight;
		char ch, texType[PATH_MAX];

		/* the first parameter for textures is always ignored; which is why we ignore
		 * nlevels read in above */
		ch = *pFileData++;

		// Run up to the dot or till the buffer is filled. Leave room for the extension.
		for (i = 0; i < PATH_MAX-5 && (ch = *pFileData++) != '\0' && ch != '.'; ++i)
		{
 			texfile[i] = ch;
		}
		texfile[i] = '\0';

		if (sscanf(pFileData, "%255s%n", texType, &cnt) != 1)
		{
			debug(LOG_ERROR, "iV_ProcessIMD %s texture info corrupt: %s", pFileName, buffer);
			return NULL;
		}
		pFileData += cnt;

		if (strcmp(texType, "png") != 0)
		{
			debug(LOG_ERROR, "iV_ProcessIMD %s: only png textures supported", pFileName);
			return NULL;
		}
		sstrcat(texfile, ".png");

		if (sscanf(pFileData, "%d %d%n", &pwidth, &pheight, &cnt) != 2)
		{
			debug(LOG_ERROR, "iV_ProcessIMD %s bad texture size: %s", pFileName, buffer);
			return NULL;
		}
		pFileData += cnt;

		/* Now read in LEVELS directive */
		if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
		{
			debug(LOG_ERROR, "iV_ProcessIMD %s bad levels info: %s", pFileName, buffer);
			return NULL;
		}
		pFileData += cnt;

		bTextured = true;
	}

	if (strncmp(buffer, "NORMALMAP", 9) == 0)
	{
		char ch, texType[PATH_MAX];
		int i;

		/* the first parameter for textures is always ignored; which is why we ignore
		 * nlevels read in above */
		ch = *pFileData++;

		// Run up to the dot or till the buffer is filled. Leave room for the extension.
		for (i = 0; i < PATH_MAX-5 && (ch = *pFileData++) != '\0' && ch != '.'; ++i)
		{
 			normalfile[i] = ch;
		}
		normalfile[i] = '\0';

		if (sscanf(pFileData, "%255s%n", texType, &cnt) != 1)
		{
			debug(LOG_ERROR, "iV_ProcessIMD %s normal map info corrupt: %s", pFileName, buffer);
			return NULL;
		}
		pFileData += cnt;

		if (strcmp(texType, "png") != 0)
		{
			debug(LOG_ERROR, "iV_ProcessIMD %s: only png normal maps supported", pFileName);
			return NULL;
		}
		sstrcat(normalfile, ".png");

		/* -Now- read in LEVELS directive */
		if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
		{
			debug(LOG_ERROR, "iV_ProcessIMD %s bad levels info: %s", pFileName, buffer);
			return NULL;
		}
		pFileData += cnt;
	}

	if (strncmp(buffer, "LEVELS", 6) != 0)
	{
		debug(LOG_ERROR, "iV_ProcessIMD: expecting 'LEVELS' directive (%s)", buffer);
		return NULL;
	}

	/* Read first LEVEL directive */
	if (sscanf(pFileData, "%255s %d%n", buffer, &level, &cnt) != 2)
	{
		debug(LOG_ERROR, "(_load_level) file corrupt -J");
		return NULL;
	}
	pFileData += cnt;

	if (strncmp(buffer, "LEVEL", 5) != 0)
	{
		debug(LOG_ERROR, "iV_ProcessIMD(2): expecting 'LEVEL' directive (%s)", buffer);
		return NULL;
	}

	shape = _imd_load_level(&pFileData, FileDataEnd, nlevels, imd_version);
	if (shape == NULL)
	{
		debug(LOG_ERROR, "iV_ProcessIMD %s unsuccessful", pFileName);
		return NULL;
	}

	// load texture page if specified
	if (bTextured)
	{
		int texpage = iV_GetTexture(texfile);
		int normalpage = iV_TEX_INVALID;

		if (normalfile[0] != '\0')
		{
			debug(LOG_WARNING, "Loading normal map %s for %s", normalfile, pFileName);
			normalpage = iV_GetTexture(normalfile);
		}

		ASSERT_OR_RETURN(NULL, texpage >= 0, "%s could not load tex page %s", pFileName, texfile);

		// assign tex pages and flags to all levels
		for (psShape = shape; psShape != NULL; psShape = psShape->next)
		{
			psShape->texpage = texpage;
			psShape->normalpage = normalpage;
			psShape->flags = imd_flags;
		}

		// check if model should use team colour mask
		if (imd_flags & iV_IMD_TCMASK)
		{
			int texpage_mask;

			pie_MakeTexPageTCMaskName(texfile);
			sstrcat(texfile, ".png");
			texpage_mask = iV_GetTexture(texfile);

			ASSERT_OR_RETURN(shape, texpage_mask >= 0, "%s could not load tcmask %s", pFileName, texfile);

			// Propagate settings through levels
			for (psShape = shape; psShape != NULL; psShape = psShape->next)
			{
				psShape->tcmaskpage = texpage_mask;
			}
		}
	}

	*ppFileData = pFileData;
	return shape;
}
