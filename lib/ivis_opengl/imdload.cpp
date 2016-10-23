/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#include <QtCore/QMap>
#include <QtCore/QString>

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piestate.h"

#include "ivisdef.h" // for imd structures
#include "imd.h" // for imd structures
#include "tex.h" // texture page loading

// Scale animation numbers from int to float
#define INT_SCALE       1000

typedef QMap<QString, iIMDShape *> MODELMAP;
static MODELMAP models;

static iIMDShape *iV_ProcessIMD(const QString &filename, const char **ppFileData, const char *FileDataEnd);

iIMDShape::iIMDShape()
{
	flags = 0;
	nconnectors = 0; // Default number of connectors must be 0
	npoints = 0;
	npolys = 0;
	points = NULL;
	polys = NULL;
	connectors = NULL;
	next = NULL;
	shadowEdgeList = NULL;
	nShadowEdges = 0;
	texpage = iV_TEX_INVALID;
	tcmaskpage = iV_TEX_INVALID;
	normalpage = iV_TEX_INVALID;
	specularpage = iV_TEX_INVALID;
	numFrames = 0;
	shaderProgram = SHADER_NONE;
	objanimtime = 0;
	objanimcycles = 0;
}

static void iV_IMDRelease(iIMDShape *s)
{
	unsigned int i;
	iIMDShape *d;

	if (s)
	{
		if (s->points)
		{
			free(s->points);
		}
		if (s->connectors)
		{
			free(s->connectors);
		}
		if (s->polys)
		{
			for (i = 0; i < s->npolys; i++)
			{
				if (s->polys[i].texCoord)
				{
					free(s->polys[i].texCoord);
				}
			}
			free(s->polys);
		}
		if (s->shadowEdgeList)
		{
			free(s->shadowEdgeList);
			s->shadowEdgeList = NULL;
		}
		glDeleteBuffers(VBO_COUNT, s->buffers);
		// shader deleted later, if any
		d = s->next;
		delete s;
		iV_IMDRelease(d);
	}
}

void modelShutdown()
{
	for (MODELMAP::iterator i = models.begin(); i != models.end(); i = models.erase(i))
	{
		iV_IMDRelease(i.value());
	}
	models.clear();
}

static bool tryLoad(const QString &path, const QString &filename)
{
	if (PHYSFS_exists(path + filename))
	{
		char *pFileData = NULL, *fileEnd;
		UDWORD size = 0;
		if (!loadFile(QString(path + filename).toUtf8().constData(), &pFileData, &size))
		{
			debug(LOG_ERROR, "Failed to load model file: %s", QString(path + filename).toUtf8().constData());
			return false;
		}
		fileEnd = pFileData + size;
		iIMDShape *s = iV_ProcessIMD(filename, (const char **)&pFileData, fileEnd);
		if (s)
		{
			models.insert(filename, s);
		}
		return true;
	}
	return false;
}

const QString &modelName(iIMDShape *model)
{
	for (MODELMAP::iterator i = models.begin(); i != models.end(); ++i)
	{
		if (i.value() == model)
		{
			return i.key();
		}
	}
	ASSERT(false, "An IMD pointer could not be backtraced to a filename!");
	static QString error;
	return error;
}

iIMDShape *modelGet(const QString &filename)
{
	QString name(filename.toLower());
	if (models.contains(name))
	{
		return models[name]; // cached
	}
	else if (tryLoad("structs/", name) || tryLoad("misc/", name) || tryLoad("effects/", name)
	         || tryLoad("components/prop/", name) || tryLoad("components/weapons/", name)
	         || tryLoad("components/bodies/", name) || tryLoad("features/", name)
	         || tryLoad("misc/micnum/", name) || tryLoad("misc/minum/", name) || tryLoad("misc/mivnum/", name) || tryLoad("misc/researchimds/", name))
	{
		return models[name];
	}
	debug(LOG_ERROR, "Could not find: %s", name.toUtf8().constData());
	return NULL;
}

static bool AtEndOfFile(const char *CurPos, const char *EndOfFile)
{
	while (*CurPos == 0x00 || *CurPos == 0x09 || *CurPos == 0x0a || *CurPos == 0x0d || *CurPos == 0x20)
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
static bool _imd_load_polys(const QString &filename, const char **ppFileData, iIMDShape *s, int pieVersion)
{
	const char *pFileData = *ppFileData;
	unsigned int i, j;
	iIMDPoly *poly;

	s->numFrames = 0;
	s->animInterval = 0;

	s->polys = (iIMDPoly *)malloc(sizeof(iIMDPoly) * s->npolys);
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

			p2.x = s->points[poly->pindex[2]].x;
			p2.y = s->points[poly->pindex[2]].y;
			p2.z = s->points[poly->pindex[2]].z;

			poly->normal = pie_SurfaceNormal3fv(p0, p1, p2);
		}

		// texture coord routine
		if (poly->flags & iV_IMD_TEX)
		{
			int nFrames, framesPerLine, frame, pbRate;
			float tWidth, tHeight;

			if (poly->flags & iV_IMD_TEXANIM)
			{
				if (sscanf(pFileData, "%d %d %f %f%n", &nFrames, &pbRate, &tWidth, &tHeight, &cnt) != 4)
				{
					debug(LOG_ERROR, "(_load_polys) [poly %u] error reading texanim data", i);
					return false;
				}
				pFileData += cnt;

				ASSERT(tWidth > 0.0001f, "%s: texture width = %f", filename.toUtf8().constData(), tWidth);
				ASSERT(tHeight > 0.f, "%s: texture height = %f (width=%f)", filename.toUtf8().constData(), tHeight, tWidth);
				ASSERT(nFrames > 1, "%s: animation frames = %d", filename.toUtf8().constData(), nFrames);
				ASSERT(pbRate > 0, "%s: animation interval = %d ms", filename.toUtf8().constData(), pbRate);

				/* Must have same number of frames and same playback rate for all polygons */
				if (s->numFrames == 0)
				{
					s->numFrames = nFrames;
					s->animInterval = pbRate;
				}
				else
				{
					ASSERT(s->numFrames == nFrames,
					       "%s: varying number of frames within one PIE level: %d != %d",
					       filename.toUtf8().constData(), nFrames, s->numFrames);
					ASSERT(s->animInterval == pbRate,
					       "%s: varying animation intervals within one PIE level: %d != %d",
					       filename.toUtf8().constData(), pbRate, s->animInterval);
				}

				poly->texAnim.x = tWidth;
				poly->texAnim.y = tHeight;

				if (pieVersion != PIE_FLOAT_VER)
				{
					poly->texAnim.x /= OLD_TEXTURE_SIZE_FIX;
					poly->texAnim.y /= OLD_TEXTURE_SIZE_FIX;
				}
				framesPerLine = 1 / poly->texAnim.x;
			}
			else
			{
				nFrames = 1;
				framesPerLine = 1;
				pbRate = 1;
				tWidth = 0.f;
				tHeight = 0.f;
				poly->texAnim.x = 0;
				poly->texAnim.y = 0;
			}

			poly->texCoord = (Vector2f *)malloc(sizeof(*poly->texCoord) * nFrames * 3);
			ASSERT_OR_RETURN(false, poly->texCoord, "Out of memory allocating texture coordinates");
			for (j = 0; j < 3; j++)
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
					const float uFrame = (frame % framesPerLine) * poly->texAnim.x;
					const float vFrame = (frame / framesPerLine) * poly->texAnim.y;
					Vector2f *c = &poly->texCoord[frame * 3 + j];

					c->x = VertexU + uFrame;
					c->y = VertexV + vFrame;
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


static bool ReadPoints(const char **ppFileData, iIMDShape *s)
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

void _imd_calc_bounds(iIMDShape *s, Vector3f *p, int size)
{
	int32_t xmax, ymax, zmax, count;
	double dx, dy, dz, rad_sq, rad, old_to_p_sq, old_to_p, old_to_new;
	double xspan, yspan, zspan, maxspan;
	Vector3f dia1, dia2, cen;
	Vector3f vxmin(0, 0, 0), vymin(0, 0, 0), vzmin(0, 0, 0),
	         vxmax(0, 0, 0), vymax(0, 0, 0), vzmax(0, 0, 0);

	s->max.x = s->max.y = s->max.z = -FP12_MULTIPLIER;
	s->min.x = s->min.y = s->min.z = FP12_MULTIPLIER;

	vxmax.x = vymax.y = vzmax.z = -FP12_MULTIPLIER;
	vxmin.x = vymin.y = vzmin.z = FP12_MULTIPLIER;

	// set up bounding data for minimum number of vertices
	for (count = 0; count < size; p++, count++)
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
	s->sradius = sqrtf(xmax * xmax + ymax * ymax + zmax * zmax);

// START: tight bounding sphere

	// set xspan = distance between 2 points xmin & xmax (squared)
	dx = vxmax.x - vxmin.x;
	dy = vxmax.y - vxmin.y;
	dz = vxmax.z - vxmin.z;
	xspan = dx * dx + dy * dy + dz * dz;

	// same for yspan
	dx = vymax.x - vymin.x;
	dy = vymax.y - vymin.y;
	dz = vymax.z - vymin.z;
	yspan = dx * dx + dy * dy + dz * dz;

	// and ofcourse zspan
	dx = vzmax.x - vzmin.x;
	dy = vzmax.y - vzmin.y;
	dz = vzmax.z - vzmin.z;
	zspan = dx * dx + dy * dy + dz * dz;

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

	rad_sq = dx * dx + dy * dy + dz * dz;
	rad = sqrt((double)rad_sq);

	// second pass (find tight sphere)
	for (p = s->points; p < s->points + s->npoints; p++)
	{
		dx = p->x - cen.x;
		dy = p->y - cen.y;
		dz = p->z - cen.z;
		old_to_p_sq = dx * dx + dy * dy + dz * dz;

		// do r**2 first
		if (old_to_p_sq > rad_sq)
		{
			// this point outside current sphere
			old_to_p = sqrt((double)old_to_p_sq);
			// radius of new sphere
			rad = (rad + old_to_p) / 2.;
			// rad**2 for next compare
			rad_sq = rad * rad;
			old_to_new = old_to_p - rad;
			// centre of new sphere
			cen.x = (rad * cen.x + old_to_new * p->x) / old_to_p;
			cen.y = (rad * cen.y + old_to_new * p->y) / old_to_p;
			cen.z = (rad * cen.z + old_to_new * p->z) / old_to_p;
		}
	}

	s->ocen = cen;

// END: tight bounding sphere
}

static bool _imd_load_points(const char **ppFileData, iIMDShape *s)
{
	//load the points then pass through a second time to setup bounding datavalues
	s->points = (Vector3f *)malloc(sizeof(Vector3f) * s->npoints);
	if (s->points == NULL)
	{
		return false;
	}

	// Read in points and remove duplicates (!)
	if (ReadPoints(ppFileData, s) == false)
	{
		free(s->points);
		s->points = NULL;
		return false;
	}

	_imd_calc_bounds(s, s->points, s->npoints);

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
bool _imd_load_connectors(const char **ppFileData, iIMDShape *s)
{
	const char *pFileData = *ppFileData;
	int cnt;
	Vector3i *p = NULL, newVector(0, 0, 0);

	s->connectors = (Vector3i *)malloc(sizeof(Vector3i) * s->nconnectors);
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

// performance hack
static QVector<GLfloat> vertices;
static QVector<GLfloat> normals;
static QVector<GLfloat> texcoords;
static QVector<GLfloat> tangents;
static QVector<uint16_t> indices; // size is npolys * 3 * numFrames
static int vertexCount;

static inline int addVertex(iIMDShape *s, int i, const iIMDPoly *p, int frameidx)
{
	// if texture animation flag is present, fetch animation coordinates for this polygon
	// otherwise just show the first set of texel coordinates
	int frame = (p->flags & iV_IMD_TEXANIM) ? frameidx : 0;

	// See if we already have this defined, if so, return reference to it.
	for (int j = 0; j < vertexCount; j++)
	{
		if (texcoords[j * 2 + 0] == p->texCoord[frame * 3 + i].x
		    && texcoords[j * 2 + 1] == p->texCoord[frame * 3 + i].y
		    && vertices[j * 3 + 0] == s->points[p->pindex[i]].x
		    && vertices[j * 3 + 1] == s->points[p->pindex[i]].y
		    && vertices[j * 3 + 2] == s->points[p->pindex[i]].z
		    && normals[j * 3 + 0] == p->normal.x
		    && normals[j * 3 + 1] == p->normal.y
		    && normals[j * 3 + 2] == p->normal.z)
		{
			return j;
		}
	}
	// We don't have it, add it.
	normals.append(p->normal.x);
	normals.append(p->normal.y);
	normals.append(p->normal.z);
	texcoords.append(p->texCoord[frame * 3 + i].x);
	texcoords.append(p->texCoord[frame * 3 + i].y);
	vertices.append(s->points[p->pindex[i]].x);
	vertices.append(s->points[p->pindex[i]].y);
	vertices.append(s->points[p->pindex[i]].z);
	vertexCount++;
	return vertexCount - 1;
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
static iIMDShape *_imd_load_level(const QString &filename, const char **ppFileData, const char *FileDataEnd, int nlevels, int pieVersion)
{
	const char *pFileData = *ppFileData;
	char buffer[PATH_MAX] = {'\0'};
	int cnt = 0, n = 0, i;
	iIMDShape *s = NULL;
	float dummy;

	if (nlevels == 0)
	{
		return NULL;
	}

	i = sscanf(pFileData, "%255s %n", buffer, &cnt);
	ASSERT_OR_RETURN(NULL, i == 1, "Bad directive following LEVEL");

	s = new iIMDShape;

	// Optionally load and ignore deprecated MATERIALS directive
	if (strcmp(buffer, "MATERIALS") == 0)
	{
		i = sscanf(pFileData, "%255s %f %f %f %f %f %f %f %f %f %f%n", buffer, &dummy, &dummy, &dummy, &dummy,
		           &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &cnt);
		ASSERT_OR_RETURN(NULL, i == 11, "Bad MATERIALS directive");
		debug(LOG_WARNING, "MATERIALS directive no longer supported!");
		pFileData += cnt;
	}
	else if (strcmp(buffer, "SHADERS") == 0)
	{
		char vertex[PATH_MAX], fragment[PATH_MAX];

		if (sscanf(pFileData, "%255s %255s %255s%n", buffer, vertex, fragment, &cnt) != 3)
		{
			debug(LOG_ERROR, "%s shader corrupt: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		std::vector<std::string> uniform_names { "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap",
		                                         "specularmap", "ecmEffect", "alphaTest", "graphicsCycle", "ModelViewProjectionMatrix" };
		s->shaderProgram = pie_LoadShader(filename.toUtf8().constData(), vertex, fragment, uniform_names);
		pFileData += cnt;
	}

	if (sscanf(pFileData, "%255s %d%n", buffer, &s->npoints, &cnt) != 2)
	{
		debug(LOG_ERROR, "_imd_load_level(2): file corrupt");
		return NULL;
	}
	pFileData += cnt;

	// load points

	ASSERT_OR_RETURN(NULL, strcmp(buffer, "POINTS") == 0, "Expecting 'POINTS' directive, got: %s", buffer);

	_imd_load_points(&pFileData, s);

	if (sscanf(pFileData, "%255s %d%n", buffer, &s->npolys, &cnt) != 2)
	{
		debug(LOG_ERROR, "_imd_load_level(3): file corrupt");
		return NULL;
	}
	pFileData += cnt;

	ASSERT_OR_RETURN(NULL, strcmp(buffer, "POLYGONS") == 0, "Expecting 'POLYGONS' directive, got: %s", buffer);

	_imd_load_polys(filename, &pFileData, s, pieVersion);

	// optional stuff : levels, object animations, connectors
	s->objanimframes = 0;
	while (!AtEndOfFile(pFileData, FileDataEnd)) // check for end of file (give or take white space)
	{
		// Scans in the line ... if we don't get 2 parameters then quit
		if (sscanf(pFileData, "%255s %d%n", buffer, &n, &cnt) != 2)
		{
			break;
		}
		pFileData += cnt;

		if (strcmp(buffer, "LEVEL") == 0)	// check for next level
		{
			debug(LOG_3D, "imd[_load_level] = npoints %d, npolys %d", s->npoints, s->npolys);
			s->next = _imd_load_level(filename, &pFileData, FileDataEnd, nlevels - 1, pieVersion);
		}
		else if (strcmp(buffer, "CONNECTORS") == 0)
		{
			//load connector stuff
			s->nconnectors = n;
			_imd_load_connectors(&pFileData, s);
		}
		else if (strcmp(buffer, "ANIMOBJECT") == 0)
		{
			s->objanimtime = n;
			if (sscanf(pFileData, "%d %d%n", &s->objanimcycles, &s->objanimframes, &cnt) != 2)
			{
				debug(LOG_ERROR, "%s bad ANIMOBJ: %s", filename.toUtf8().constData(), pFileData);
				return NULL;
			}
			pFileData += cnt;
			s->objanimdata.resize(s->objanimframes);
			for (int i = 0; i < s->objanimframes; i++)
			{
				int frame;
				Vector3i pos, rot, scale;

				if (sscanf(pFileData, "%d %d %d %d %d %d %d %d %d %d%n",
				           &frame, &pos.x, &pos.y, &pos.z, &rot.x, &rot.y, &rot.z, &scale.x, &scale.y, &scale.z, &cnt) != 10)
				{
					debug(LOG_ERROR, "%s: Invalid object animation line frame %d", filename.toUtf8().constData(), frame);
				}
				ASSERT(frame == i, "%s: Invalid frame enumeration object animation %d: %d", filename.toUtf8().constData(), i, frame);
				s->objanimdata[i].pos.x = pos.x / INT_SCALE;
				s->objanimdata[i].pos.y = pos.z / INT_SCALE;
				s->objanimdata[i].pos.z = pos.y / INT_SCALE;
				s->objanimdata[i].rot.pitch = -(rot.x * DEG_1 / INT_SCALE);
				s->objanimdata[i].rot.direction = -(rot.z * DEG_1 / INT_SCALE);
				s->objanimdata[i].rot.roll = -(rot.y * DEG_1 / INT_SCALE);
				s->objanimdata[i].scale.x = scale.x / 1000.0f;
				s->objanimdata[i].scale.y = scale.z / 1000.0f;
				s->objanimdata[i].scale.z = scale.y / 1000.0f;
				pFileData += cnt;
			}
		}
		else
		{
			debug(LOG_ERROR, "(_load_level) unexpected directive %s %d", buffer, n);
			break;
		}
	}

	// FINALLY, massage the data into what can stream directly to OpenGL
	glGenBuffers(VBO_COUNT, s->buffers);
	vertexCount = 0;
	for (int k = 0; k < MAX(1, s->numFrames); k++)
	{
		// Go through all polygons for each frame
		for (unsigned i = 0; i < s->npolys; i++)
		{
			const iIMDPoly *pPolys = &s->polys[i];

			// Do we already have the vertex data for this polygon?
			indices.append(addVertex(s, 0, pPolys, k));
			indices.append(addVertex(s, 1, pPolys, k));
			indices.append(addVertex(s, 2, pPolys, k));
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, s->buffers[VBO_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, s->buffers[VBO_NORMAL]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->buffers[VBO_INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, s->buffers[VBO_TEXCOORD]);
	glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat), texcoords.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

	indices.resize(0);
	vertices.resize(0);
	texcoords.resize(0);
	normals.resize(0);

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
static iIMDShape *iV_ProcessIMD(const QString &filename, const char **ppFileData, const char *FileDataEnd)
{
	const char *pFileData = *ppFileData;
	char buffer[PATH_MAX], texfile[PATH_MAX], normalfile[PATH_MAX], specfile[PATH_MAX];
	int cnt, nlevels;
	iIMDShape *shape;
	UDWORD level;
	int32_t imd_version;
	uint32_t imd_flags;
	bool bTextured = false;
	iIMDShape *objanimpie[ANIM_EVENT_COUNT];

	memset(normalfile, 0, sizeof(normalfile));
	memset(specfile, 0, sizeof(specfile));

	if (sscanf(pFileData, "%255s %d%n", buffer, &imd_version, &cnt) != 2)
	{
		debug(LOG_ERROR, "%s: bad PIE version: (%s)", filename.toUtf8().constData(), buffer);
		assert(false);
		return NULL;
	}
	pFileData += cnt;

	if (strcmp(PIE_NAME, buffer) != 0)
	{
		debug(LOG_ERROR, "%s: Not an IMD file (%s %d)", filename.toUtf8().constData(), buffer, imd_version);
		return NULL;
	}

	//Now supporting version PIE_VER and PIE_FLOAT_VER files
	if (imd_version != PIE_VER && imd_version != PIE_FLOAT_VER)
	{
		debug(LOG_ERROR, "%s: Version %d not supported", filename.toUtf8().constData(), imd_version);
		return NULL;
	}

	// Read flag
	if (sscanf(pFileData, "%255s %x%n", buffer, &imd_flags, &cnt) != 2)
	{
		debug(LOG_ERROR, "%s: bad flags: %s", filename.toUtf8().constData(), buffer);
		return NULL;
	}
	pFileData += cnt;

	/* This can be either texture or levels */
	if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
	{
		debug(LOG_ERROR, "%s: Expecting TEXTURE or LEVELS: %s", filename.toUtf8().constData(), buffer);
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
		for (i = 0; i < PATH_MAX - 5 && (ch = *pFileData++) != '\0' && ch != '.'; ++i)
		{
			texfile[i] = ch;
		}
		texfile[i] = '\0';

		if (sscanf(pFileData, "%255s%n", texType, &cnt) != 1)
		{
			debug(LOG_ERROR, "%s: Texture info corrupt: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;

		if (strcmp(texType, "png") != 0)
		{
			debug(LOG_ERROR, "%s: Only png textures supported", filename.toUtf8().constData());
			return NULL;
		}
		sstrcat(texfile, ".png");

		if (sscanf(pFileData, "%d %d%n", &pwidth, &pheight, &cnt) != 2)
		{
			debug(LOG_ERROR, "%s: Bad texture size: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;

		/* Now read in LEVELS directive */
		if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
		{
			debug(LOG_ERROR, "%s: Bad levels info: %s", filename.toUtf8().constData(), buffer);
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
		for (i = 0; i < PATH_MAX - 5 && (ch = *pFileData++) != '\0' && ch != '.'; ++i)
		{
			normalfile[i] = ch;
		}
		normalfile[i] = '\0';

		if (sscanf(pFileData, "%255s%n", texType, &cnt) != 1)
		{
			debug(LOG_ERROR, "%s: Normal map info corrupt: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;

		if (strcmp(texType, "png") != 0)
		{
			debug(LOG_ERROR, "%s: Only png normal maps supported", filename.toUtf8().constData());
			return NULL;
		}
		sstrcat(normalfile, ".png");

		/* Now read in LEVELS directive */
		if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
		{
			debug(LOG_ERROR, "%s: Bad levels info: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;
	}

	if (strncmp(buffer, "SPECULARMAP", 11) == 0)
	{
		char ch, texType[PATH_MAX];
		int i;

		/* the first parameter for textures is always ignored; which is why we ignore nlevels read in above */
		ch = *pFileData++;

		// Run up to the dot or till the buffer is filled. Leave room for the extension.
		for (i = 0; i < PATH_MAX - 5 && (ch = *pFileData++) != '\0' && ch != '.'; ++i)
		{
			specfile[i] = ch;
		}
		specfile[i] = '\0';

		if (sscanf(pFileData, "%255s%n", texType, &cnt) != 1)
		{
			debug(LOG_ERROR, "%s specular map info corrupt: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;

		if (strcmp(texType, "png") != 0)
		{
			debug(LOG_ERROR, "%s: only png specular maps supported", filename.toUtf8().constData());
			return NULL;
		}
		sstrcat(specfile, ".png");

		/* Try -again- to read in LEVELS directive */
		if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
		{
			debug(LOG_ERROR, "%s: Bad levels info: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;
	}

	for (int i = 0; i < ANIM_EVENT_COUNT; i++)
	{
		objanimpie[i] = NULL;
	}
	while (strncmp(buffer, "EVENT", 5) == 0)
	{
		char animpie[PATH_MAX];

		ASSERT(nlevels < ANIM_EVENT_COUNT && nlevels >= 0, "Invalid event type %d", nlevels);
		pFileData++;
		if (sscanf(pFileData, "%255s%n", animpie, &cnt) != 1)
		{
			debug(LOG_ERROR, "%s animation model corrupt: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;
		objanimpie[nlevels] = modelGet(animpie);

		/* Try -yet again- to read in LEVELS directive */
		if (sscanf(pFileData, "%255s %d%n", buffer, &nlevels, &cnt) != 2)
		{
			debug(LOG_ERROR, "%s: Bad levels info: %s", filename.toUtf8().constData(), buffer);
			return NULL;
		}
		pFileData += cnt;
	}

	if (strncmp(buffer, "LEVELS", 6) != 0)
	{
		debug(LOG_ERROR, "%s: Expecting 'LEVELS' directive (%s)", filename.toUtf8().constData(), buffer);
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
		debug(LOG_ERROR, "%s: Expecting 'LEVEL' directive (%s)", filename.toUtf8().constData(), buffer);
		return NULL;
	}

	shape = _imd_load_level(filename, &pFileData, FileDataEnd, nlevels, imd_version);
	if (shape == NULL)
	{
		debug(LOG_ERROR, "%s: Unsuccessful", filename.toUtf8().constData());
		return NULL;
	}

	// load texture page if specified
	if (bTextured)
	{
		int texpage = iV_GetTexture(texfile);
		int normalpage = iV_TEX_INVALID;
		int specpage = iV_TEX_INVALID;

		ASSERT_OR_RETURN(NULL, texpage >= 0, "%s could not load tex page %s", filename.toUtf8().constData(), texfile);

		if (normalfile[0] != '\0')
		{
			debug(LOG_TEXTURE, "Loading normal map %s for %s", normalfile, filename.toUtf8().constData());
			normalpage = iV_GetTexture(normalfile, false);
			ASSERT_OR_RETURN(NULL, normalpage >= 0, "%s could not load tex page %s", filename.toUtf8().constData(), normalfile);
		}

		if (specfile[0] != '\0')
		{
			debug(LOG_TEXTURE, "Loading specular map %s for %s", specfile, filename.toUtf8().constData());
			specpage = iV_GetTexture(specfile, false);
			ASSERT_OR_RETURN(NULL, specpage >= 0, "%s could not load tex page %s", filename.toUtf8().constData(), specfile);
		}

		// assign tex pages and flags to all levels
		for (iIMDShape *psShape = shape; psShape != NULL; psShape = psShape->next)
		{
			psShape->texpage = texpage;
			psShape->normalpage = normalpage;
			psShape->specularpage = specpage;
			psShape->flags = imd_flags;
		}

		// check if model should use team colour mask
		if (imd_flags & iV_IMD_TCMASK)
		{
			int texpage_mask;

			pie_MakeTexPageTCMaskName(texfile);
			sstrcat(texfile, ".png");
			texpage_mask = iV_GetTexture(texfile);

			ASSERT_OR_RETURN(shape, texpage_mask >= 0, "%s could not load tcmask %s", filename.toUtf8().constData(), texfile);

			// Propagate settings through levels
			for (iIMDShape *psShape = shape; psShape != NULL; psShape = psShape->next)
			{
				psShape->tcmaskpage = texpage_mask;
			}
		}
	}

	// copy over model-wide animation information, stored only in the first level
	for (int i = 0; i < ANIM_EVENT_COUNT; i++)
	{
		shape->objanimpie[i] = objanimpie[i];
	}

	*ppFileData = pFileData;
	return shape;
}
