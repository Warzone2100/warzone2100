/*
 * The 3D Studio File Format Library
 * Copyright (C) 1996-2001 by J.E. Hoffmann <je-h@gmx.net>
 * All rights reserved.
 *
 * This program is  free  software;  you can redistribute it and/or modify it
 * under the terms of the  GNU Lesser General Public License  as published by
 * the  Free Software Foundation;  either version 2.1 of the License,  or (at
 * your option) any later version.
 *
 * This  program  is  distributed in  the  hope that it will  be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A  PARTICULAR PURPOSE.  See the  GNU Lesser General Public
 * License for more details.
 *
 * You should  have received  a copy of the GNU Lesser General Public License
 * along with  this program;  if not, write to the  Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "3ds2pie.h"

#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef WIN32
#include <stdbool.h>
#include <limits.h>
#endif

struct WZ_FACE_PROXY {
	unsigned int index[POLYGON_SIZE_3DS];
	float texCoord[POLYGON_SIZE_3DS][2];
	bool cull;
};

struct WZ_POSITION_PROXY {
	float x, y, z;
	unsigned int reindex;
	bool dupe;
};

// Make a string lower case
static void resToLower(char *pStr)
{
	while (*pStr != 0)
	{
		*pStr = tolower(*pStr);
		pStr += 1;
	}
}

void dump_3ds_to_pie(Lib3dsFile *f, WZ_PIE_LEVEL *pie, PIE_OPTIONS options)
{
	Lib3dsMesh *m;
	Lib3dsMaterial *material;
	unsigned int meshIdx, i, j;

	/* Materials */

	for (j = 0, material = f->materials; material; material = material->next, ++j)
	{
		Lib3dsTextureMap *texture = &material->texture1_map;

		if (j > 0)
		{
			fprintf(stderr, "Texture %d %s-%s: More than one texture currently not supported!\n", j, options.page, texture->name);
			continue;
		}

		resToLower(texture->name);
		pie->texName = strdup(texture->name);
	}

	/* Vertexes and polygons from meshes */

	for (meshIdx = 0, m = f->meshes; m; m = m->next, ++meshIdx)
	{
		unsigned int facesCount;
		WZ_FACE_PROXY *faceList;
		WZ_POSITION_PROXY *posList;

		if (meshIdx > 0)
		{
			fprintf(stderr, "Mesh %d %s: More than one frame currently not supported!\n", meshIdx, m->name);
			continue;
		}

		/* Vertexes */

		posList = (WZ_POSITION_PROXY *) malloc(sizeof(WZ_POSITION_PROXY) * m->points);

		for (j = 0; j < m->points; ++j)
		{
			posList[j].dupe = false;

			if (options.swapYZ)
			{
				posList[j].x = m->pointL[j].pos[0];
				posList[j].y = m->pointL[j].pos[2];
				posList[j].z = m->pointL[j].pos[1];
			}
			else
			{
				posList[j].x = m->pointL[j].pos[0];
				posList[j].y = m->pointL[j].pos[1];
				posList[j].z = m->pointL[j].pos[2];
			}
		}

		// Remove duplicate points
		for (i = 0, j = 0; j < m->points; ++j)
		{
			unsigned int k;

			for (k = j + 1; k < m->points; ++k)
			{
				// if points in k are equal to points in j, replace all k with j in face list
				if (!posList[k].dupe &&
					posList[k].x == posList[j].x &&
					posList[k].y == posList[j].y &&
					posList[k].z == posList[j].z)
				{
					posList[k].dupe	= true;	// oh noes, a dupe! let's skip it when we write them out again.
					posList[k].reindex = i;	// rewrite face list to point here
				}
			}
			if (!posList[j].dupe)
			{
				posList[j].reindex = i;
				i++;
			}
		}

		pie->posL = (WZ_POSITION *) malloc(sizeof(WZ_POSITION) * i);
		pie->numVertexes = i;
		
		for (j = 0; j < m->points; ++j)
		{
			if (!posList[j].dupe)
			{
				pie->posL[posList[j].reindex].x = posList[j].x;
				pie->posL[posList[j].reindex].y = posList[j].y;
				pie->posL[posList[j].reindex].z = posList[j].z;
			}
		}

		/* Polygons */

		faceList = (WZ_FACE_PROXY *) malloc(sizeof(WZ_FACE_PROXY) * m->faces);

		// i controls reverseWinding dynamically in form of (i,1,2-i), do notice 1!
		i = 2 * (int)options.reverseWinding;
		for (j = 0; j < m->faces; ++j)
		{
			Lib3dsFace *face = &m->faceL[j];

			if (!options.invertUV)
			{
				faceList[j].texCoord[i][0] = m->texelL[face->points[0]][0];
				faceList[j].texCoord[i][1] = m->texelL[face->points[0]][1];
				faceList[j].texCoord[1][0] = m->texelL[face->points[1]][0];
				faceList[j].texCoord[1][1] = m->texelL[face->points[1]][1];
				faceList[j].texCoord[2-i][0] = m->texelL[face->points[2]][0];
				faceList[j].texCoord[2-i][1] = m->texelL[face->points[2]][1];
			}
			else
			{
				faceList[j].texCoord[i][0] = m->texelL[face->points[0]][0];
				faceList[j].texCoord[i][1] = 1.0f - m->texelL[face->points[0]][1];
				faceList[j].texCoord[1][0] = m->texelL[face->points[1]][0];
				faceList[j].texCoord[1][1] = 1.0f - m->texelL[face->points[1]][1];
				faceList[j].texCoord[2-i][0] = m->texelL[face->points[2]][0];
				faceList[j].texCoord[2-i][1] = 1.0f - m->texelL[face->points[2]][1];
			}

			faceList[j].index[i] = face->points[0];
			faceList[j].index[1] = face->points[1];
			faceList[j].index[2-i] = face->points[2];

			if (face->material[0]) {
				material=lib3ds_file_material_by_name(f, face->material);
				faceList[j].cull = !material->two_sided;
			}
			else {
				faceList[j].cull = true;
			}			 
		}

		// Rewrite face table with reindexed vertexes
		facesCount = m->faces;
		for (j = 0; j < m->faces; ++j)
		{
			faceList[j].index[0] = posList[faceList[j].index[0]].reindex;
			faceList[j].index[1] = posList[faceList[j].index[1]].reindex;
			faceList[j].index[2] = posList[faceList[j].index[2]].reindex;

			if (options.twoSidedPolys && !faceList[j].cull)
				++facesCount;
		}

		// Free vertex proxy
		if (posList)
			free(posList);

		pie->faceL = (WZ_FACE *) malloc(sizeof(WZ_FACE) * facesCount);
		pie->numFaces = facesCount;

		for (i = 0, j = 0; j < m->faces; ++i, ++j)
		{
			pie->faceL[i].index[0] = faceList[j].index[0];
			pie->faceL[i].index[1] = faceList[j].index[1];
			pie->faceL[i].index[2] = faceList[j].index[2];

			pie->faceL[i].texCoord[0][0] = faceList[j].texCoord[0][0];
			pie->faceL[i].texCoord[0][1] = faceList[j].texCoord[0][1];
			pie->faceL[i].texCoord[1][0] = faceList[j].texCoord[1][0];
			pie->faceL[i].texCoord[1][1] = faceList[j].texCoord[1][1];
			pie->faceL[i].texCoord[2][0] = faceList[j].texCoord[2][0];
			pie->faceL[i].texCoord[2][1] = faceList[j].texCoord[2][1];

			if (!faceList[j].cull)
			{
				++i;
				pie->faceL[i].index[0] = faceList[j].index[2];
				pie->faceL[i].index[1] = faceList[j].index[1];
				pie->faceL[i].index[2] = faceList[j].index[0];

				pie->faceL[i].texCoord[0][0] = faceList[j].texCoord[2][0];
				pie->faceL[i].texCoord[0][1] = faceList[j].texCoord[2][1];
				pie->faceL[i].texCoord[1][0] = faceList[j].texCoord[1][0];
				pie->faceL[i].texCoord[1][1] = faceList[j].texCoord[1][1];
				pie->faceL[i].texCoord[2][0] = faceList[j].texCoord[0][0];
				pie->faceL[i].texCoord[2][1] = faceList[j].texCoord[0][1];
			}
		}

		// Free face proxy
		if (faceList)
			free(faceList);
	}
}


void dump_pie_file(WZ_PIE_LEVEL *pie, FILE *o, PIE_OPTIONS options)
{
	unsigned int i, pietype = 0, levelIdx;

	if (options.exportPIE3)
	{
		fprintf(o, "PIE 3\n");
	}
	else
	{
		fprintf(o, "PIE 2\n");
		pietype |= 0x200;
	}

	if (options.useTCMask)
	{
		pietype |= 0x10000;
	}

	fprintf(o, "TYPE %x\n", pietype);

	if (options.exportPIE3)
	{
		fprintf(o, "TEXTURE 0 %s-%s 0 0\n", options.page, pie->texName);
	}
	else
	{
		fprintf(o, "TEXTURE 0 %s-%s 256 256\n", options.page, pie->texName);
	}

	fprintf(o, "LEVELS %d\n", 1); // Should be pie.levels

	for (levelIdx = 0; levelIdx < 1; ++levelIdx)
	{
		fprintf(o, "LEVEL %d\n", levelIdx + 1);
		fprintf(o, "POINTS %d\n", pie->numVertexes);

		for (i = 0; i < pie->numVertexes; ++i)
		{
			if (options.exportPIE3)
			{
				fprintf(o, "\t%f %f %f\n",
					pie->posL[i].x * options.scaleFactor,
					pie->posL[i].y * options.scaleFactor,
					pie->posL[i].z * options.scaleFactor);
			}
			else
			{
				fprintf(o, "\t%d %d %d\n",
					(int)(pie->posL[i].x * options.scaleFactor),
					(int)(pie->posL[i].y * options.scaleFactor),
					(int)(pie->posL[i].z * options.scaleFactor));
			}
		}

		fprintf(o, "POLYGONS %d\n", pie->numFaces);

		for (i = 0; i < pie->numFaces; ++i)
		{
			fprintf(o, "\t%X 3 %u %u %u", 0x200,
				pie->faceL[i].index[0],	pie->faceL[i].index[1],	pie->faceL[i].index[2]);
			if (options.exportPIE3)
			{
				fprintf(o, " %f %f %f %f %f %f\n",
					pie->faceL[i].texCoord[0][0], pie->faceL[i].texCoord[0][1],
					pie->faceL[i].texCoord[1][0], pie->faceL[i].texCoord[1][1],
					pie->faceL[i].texCoord[2][0], pie->faceL[i].texCoord[2][1]);
			}
			else
			{
				fprintf(o, " %d %d %d %d %d %d\n",
					(int)(pie->faceL[i].texCoord[0][0] * 256),
					(int)(pie->faceL[i].texCoord[0][1] * 256),
					(int)(pie->faceL[i].texCoord[1][0] * 256),
					(int)(pie->faceL[i].texCoord[1][1] * 256),
					(int)(pie->faceL[i].texCoord[2][0] * 256),
					(int)(pie->faceL[i].texCoord[2][1] * 256));
			}
		}
	}
}

// FIXME!!! Most likely it's broken...
#if !defined(WZ_3DS2PIE_GUI)
static char *input_file = "";
static char *output_file = "";
static char *page = "";
static bool swapYZ = true;
static bool reverseWinding = true;
static bool invertUV = true;
static unsigned int baseTexFlags = 200;
static float scaleFactor = 1.0f;

static void parse_args(int argc, char **argv)
{
	unsigned int i = 1;

	for (i = 1; argc >= 3 + i && argv[i][0] == '-'; i++)
	{
		if (argv[i][1] == 'y')
		{
			swapYZ = false; // exporting program used Y-axis as "up", like we do, don't switch
		}
		else if (argv[i][1] == 'i')
		{
			invertUV = false;
		}
		else if (argv[i][1] == 't')
		{
			baseTexFlags = 2200;
		}
		else if (argv[i][1] == 'r')
		{
			reverseWinding = false;
		}
		else if (argv[i][1] == 's')
		{
			int ret;

			i++;
			if (argc < i)
			{
				fprintf(stderr, "Missing parameter to scale option.\n");
				exit(1);
			}
			ret = sscanf(argv[i], "%f", &scaleFactor);
			if (ret != 1)
			{
				fprintf(stderr, "Bad parameter to scale option.\n");
				exit(1);
			}
		}
		else
		{
			fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
			exit(1);
		}
	}
	if (argc < 3 + i)
	{
		fprintf(stderr, "Syntax: 3ds2m [-s] [-y] [-r] [-i] [-t] input_filename output_filename page_number\n");
		fprintf(stderr, "  -y    Do not swap Y and Z axis. Exporter uses Y-axis as \"up\".\n");
		fprintf(stderr, "  -r    Do not reverse winding of all polygons.\n");
		fprintf(stderr, "  -i    Do not invert the vertical texture coordinates.\n");
		fprintf(stderr, "  -s N  Scale model points by N before converting.\n");
		fprintf(stderr, "  -t    Use two sided polygons (slower; deprecated).\n");
		exit(1);
	}
	input_file = argv[i++];
	output_file = argv[i++];
	page = argv[i++];
}


int main(int argc, char **argv)
{
	Lib3dsFile *f = NULL;
	FILE *o = NULL;

	parse_args(argc, argv);
	f = lib3ds_file_load(input_file);
	if (!f)
	{
		fprintf(stderr, "***ERROR***\nLoading file %s failed\n", input_file);
		exit(1);
	}
	o = fopen(output_file, "w+");
	if (!o)
	{
		fprintf(stderr, "***ERROR***\nCan't open %s for writing\n", output_file);
		exit(1);
	}
	dump_pie_file(f, o, page, swapYZ, invertUV, reverseWinding, baseTexFlags, scaleFactor);
	fclose(o);

	lib3ds_file_free(f);

	return 0;
}
#endif
