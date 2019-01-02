/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2019  Warzone 2100 Project

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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifndef WIN32
#include <stdbool.h>
#include <limits.h>
#else
typedef int bool;
#define PATH_MAX 255
#define true 1
#define false 0
#endif

#include <lib3ds/file.h>
#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>

// gcc -o pie23ds pie23ds.c -Wall -g -O0 `pkg-config --cflags --libs lib3ds`

#define iV_IMD_TEX 0x00000200
#define iV_IMD_TEXANIM 0x00004000
#define MAX_POLYGON_SIZE 16

static bool swapYZ = true;
static bool invertUV = true;
static bool reverseWinding = true;
static char *input = "";
static char *output = "";

typedef struct {
	int index[MAX_POLYGON_SIZE];
	int texCoord[MAX_POLYGON_SIZE][2];
	int vertices;
} WZ_FACE;

typedef struct {
	int x, y, z;
} WZ_POSITION;

static void parse_args(int argc, char **argv)
{
	unsigned int i = 1;

	for (i = 1; argc >= 2 + i && argv[i][0] == '-'; i++)
	{
		if (argv[i][1] == 'y')
		{
			swapYZ = false; // exporting program used Y-axis as "up", like we do, don't switch
		}
		if (argv[i][1] == 'i')
		{
			invertUV = true;
		}
		if (argv[i][1] == 'r')
		{
			reverseWinding = true;
		}
	}
	if (argc < 2 + i)
	{
		fprintf(stderr, "Syntax: pie23ds [-y|-r|-t] input_filename output_filename\n");
		fprintf(stderr, "  -y  Exporter uses Y-axis as \"up\", so do not switch Y and Z axis.\n");
		fprintf(stderr, "  -r  Reverse winding of all polygons.\n");
		fprintf(stderr, "  -i  Invert the vertical texture coordinates (for 3DS MAX etc.).\n");
		exit(1);
	}
	input = argv[i++];
	output = argv[i++];
}

static void dump_to_3ds(Lib3dsFile *f, FILE *fp)
{
	Lib3dsMesh	*m = lib3ds_mesh_new("Warzone Mesh");
	Lib3dsMaterial	*material = lib3ds_material_new("Warzone texture");
	Lib3dsTextureMap *texture = &material->texture1_map;
	int i, num, x, y, levels;
	char s[200];

	num = fscanf(fp, "PIE %d\n", &i);
	if (num != 1)
	{
		fprintf(stderr, "Bad PIE file %s\n", input);
		exit(1);
	}
	fprintf(stdout, "PIE version %d\n", i);

	num = fscanf(fp, "TYPE %d\n", &i); // ignore
	if (num != 1)
	{
		fprintf(stderr, "Bad TYPE directive in %s\n", input);
		exit(1);
	}

	num = fscanf(fp, "TEXTURE %d %s %d %d\n", &i, s, &x, &y);
	if (num != 4)
	{
		fprintf(stderr, "Bad TEXTURE directive in %s\n", input);
		exit(1);
	}
	strcpy(texture->name, s);

	num = fscanf(fp, "LEVELS %d\n", &levels);
	if (num != 1)
	{
		fprintf(stderr, "Bad LEVELS directive in %s\n", input);
		exit(1);
	}

	f->frames = levels;
	f->meshes = m;
	f->materials = material;
	for (i = 0; i < levels; i++)
	{
		int j, points, faces, faces3DS, faceCount, points3DS, pointCount;
		WZ_FACE *faceList;
		WZ_POSITION *posList;

		num = fscanf(fp, "LEVEL %d\n", &x);
		if (num != 1 || (i + 1) != x)
		{
			fprintf(stderr, "Bad LEVEL directive in %s, was %d should be %d\n", input, x, i + 1);
			exit(1);
		}

		num = fscanf(fp, "POINTS %d\n", &points);
		if (num != 1)
		{
			fprintf(stderr, "Bad POINTS directive in %s frame %d\n", input, i);
			exit(1);
		}
		posList = malloc(sizeof(WZ_POSITION) * points);
		for (j = 0; j < points; j++)
		{
			if (swapYZ)
			{
				num = fscanf(fp, "%d %d %d\n", &posList[j].x, &posList[j].z, &posList[j].y);
			}
			else
			{
				num = fscanf(fp, "%d %d %d\n", &posList[j].x, &posList[j].y, &posList[j].z);
			}
			if (num != 3)
			{
				fprintf(stderr, "Bad POINTS entry frame %d, number %d\n", i, j);
				exit(1);
			}
		}

		num = fscanf(fp, "POLYGONS %d", &faces);
		if (num != 1)
		{
			fprintf(stderr, "Bad POLYGONS directive in %s frame %d\n", input, i);
			exit(1);			
		}
		faces3DS = faces;	// for starters
		faceList = malloc(sizeof(WZ_FACE) * faces);
		points3DS = 0;
		for (j = 0; j < faces; ++j)
		{
			int k;
			unsigned int flags;

			num = fscanf(fp, "\n%x", &flags);
			if (num != 1)
			{
				fprintf(stderr, "Bad POLYGONS texture flag entry frame %d, number %d\n", i, j);
				exit(1);
			}
			if (!(flags & iV_IMD_TEX))
			{
				fprintf(stderr, "Bad texture flag entry frame %d, number %d - no texture flag!\n", i, j);
				exit(1);
			}

			num = fscanf(fp, "%d", &faceList[j].vertices);
			if (num != 1)
			{
				fprintf(stderr, "Bad POLYGONS vertices entry frame %d, number %d\n", i, j);
				exit(1);
			}
			assert(faceList[j].vertices <= MAX_POLYGON_SIZE); // larger polygons not supported
			assert(faceList[j].vertices >= 3); // points and lines not supported
			if (faceList[j].vertices > 3)
			{
				// since they are triangle fans already, we get to do easy tessellation
				faces3DS += (faceList[j].vertices - 3);
			}
			points3DS += faceList[j].vertices;

			// Read in vertex indices and texture coordinates
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%d", &faceList[j].index[k]);
				if (num != 1)
				{
					fprintf(stderr, "Bad vertex position entry frame %d, number %d\n", i, j);
					exit(1);
				}
			}
			if (flags & iV_IMD_TEXANIM)
			{
				// read in and discard animation values for now
				int frames, rate, width, height;

				num = fscanf(fp, "%d %d %d %d", &frames, &rate, &width, &height);
				if (num != 4)
				{
					fprintf(stderr, "Bad texture animation entry frame %d, number %d\n", i, j);
					exit(1);
				}
			}
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%d %d", &faceList[j].texCoord[k][0], &faceList[j].texCoord[k][1]);
				if (num != 2)
				{
					fprintf(stderr, "Bad texture coordinate entry frame %d, number %d\n", i, j);
					exit(1);
				}
			}
		}

		// Calculate position list. Since positions hold texture coordinates in 3DS, unlike in Warzone,
		// we need to use some black magic to transfer them over here.
		lib3ds_mesh_new_point_list(m, points3DS);
		lib3ds_mesh_new_texel_list(m, points3DS);
		pointCount = 0;
		for (j = 0; j < faces; j++)
		{
			int k;

			for (k = 0; k < faceList[j].vertices; k++)
			{
				Lib3dsVector pos;

				pos[0] = posList[faceList[j].index[k]].x;
				pos[1] = posList[faceList[j].index[k]].y;
				pos[2] = posList[faceList[j].index[k]].z;

				lib3ds_vector_copy(m->pointL[pointCount].pos, pos);
				faceList[j].index[k] = pointCount; // use new position list

				if (invertUV)
				{
					m->texelL[pointCount][0] = ((float)faceList[j].texCoord[k][0] / 256.0f);
					m->texelL[pointCount][1] = 1.0f - ((float)faceList[j].texCoord[k][1] / 256.0f);
				}
				else
				{
					m->texelL[pointCount][0] = ((float)faceList[j].texCoord[k][0] / 256.0f);
					m->texelL[pointCount][1] = ((float)faceList[j].texCoord[k][1] / 256.0f);
				}
				pointCount++;
			}
		}

		lib3ds_mesh_new_face_list(m, faces3DS);
		faceCount = 0;

		// TODO reverse winding afterwards
		for (j = 0; j < faces; j++)
		{
			int k, key, previous;

			key = m->faceL[faceCount].points[0] = faceList[j].index[0];
			m->faceL[faceCount].points[1] = faceList[j].index[1];
			previous = m->faceL[faceCount].points[2] = faceList[j].index[2];
			faceCount++;

			// Generate triangles from the Warzone triangle fans (PIEs, get it?)
			for (k = 3; k < faceList[j].vertices; k++)
			{
				m->faceL[faceCount].points[0] = key;
				m->faceL[faceCount].points[1] = previous;
				previous = m->faceL[faceCount].points[0] = faceList[j].index[k];
			}

			// Since texture coordinates are properties of positions, not indices,
			// we do not need a similar expansion of these
		}
		free(faceList);
		free(posList);
	}
	if (!lib3ds_file_save(f, output))
	{
		fprintf(stderr, "Cannot open \"%s\" for writing: %s", output, strerror(errno));
		exit(1);
	}
}

int main(int argc, char **argv)
{
	Lib3dsFile *f = NULL;
	FILE *p;

	parse_args(argc, argv);
	f = lib3ds_file_new();
	
	p = fopen(input, "r");
	if (!p)
	{
		fprintf(stderr, "Cannot open \"%s\" for reading: %s", output, strerror(errno));
		exit(1);
	}
	dump_to_3ds(f, p);
	fclose(p);
	lib3ds_file_free(f);

	return 0;
}
