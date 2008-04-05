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
#include <lib3ds/file.h>
#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef WIN32
#include <stdbool.h>
#include <limits.h>
#else
typedef int bool;
#define PATH_MAX 255
#define true 1
#define false 0
#endif

// Based on 3ds2m.c from lib3ds

// gcc -o 3ds2pie 3ds2pie.c -Wall -g -O0 `pkg-config --cflags --libs lib3ds` -Wshadow

// Make a string lower case
static void resToLower(char *pStr)
{
	while (*pStr != 0)
	{
		*pStr = tolower(*pStr);
		pStr += 1;
	}
}


void dump_pie_file(Lib3dsFile *f, FILE *o, const char *page, bool swapYZ, bool invertUV, bool reverseWinding, unsigned int baseTexFlags, float scaleFactor)
{
	Lib3dsMesh *m;
	Lib3dsMaterial *material;
	int meshIdx, j;

	fprintf(o, "PIE 2\n");
	fprintf(o, "TYPE 200\n");

	for (j = 0, material = f->materials; material; material = material->next, j++)
	{
		Lib3dsTextureMap *texture = &material->texture1_map;

		resToLower(texture->name);
		if (j > 0)
		{
			fprintf(stderr, "Texture %d %s-%s: More than one texture currently not supported!\n", j, page, texture->name);
			continue;
		}
		fprintf(o, "TEXTURE %d %s-%s 256 256\n", j, page, texture->name);
	}

	for (j = 0, m = f->meshes; m; m = m->next, j++);
	fprintf(o, "LEVELS %d\n", j);

	for (meshIdx = 0, m = f->meshes; m; m = m->next, meshIdx++)
	{
		unsigned int i;

		if (meshIdx > 0)
		{
			fprintf(stderr, "Mesh %d %s: More than one frame currently not supported!\n", meshIdx, m->name);
			continue;
		}

		fprintf(o, "LEVEL %d\n", meshIdx + 1);
		fprintf(o, "POINTS %d\n", m->points);
		for (i = 0; i < m->points; i++)
		{
			Lib3dsVector pos;

			lib3ds_vector_copy(pos, m->pointL[i].pos);

			if (swapYZ)
			{
				fprintf(o, "\t%d %d %d\n", (int)(pos[0] * scaleFactor), (int)(pos[2] * scaleFactor), (int)(pos[1] * scaleFactor));
			}
			else
			{
				fprintf(o, "\t%d %d %d\n", (int)(pos[0] * scaleFactor), (int)(pos[1] * scaleFactor), (int)(pos[2] * scaleFactor));
			}
		}

		fprintf(o, "POLYGONS %d\n", m->faces);
		for (i = 0; i < m->faces; ++i)
		{
			Lib3dsFace *face = &m->faceL[i];
			int texel[3][2];

			if (!invertUV)
			{
				texel[0][0] = m->texelL[face->points[0]][0] * 256.0f;
				texel[0][1] = m->texelL[face->points[0]][1] * 256.0f;
				texel[1][0] = m->texelL[face->points[1]][0] * 256.0f;
				texel[1][1] = m->texelL[face->points[1]][1] * 256.0f;
				texel[2][0] = m->texelL[face->points[2]][0] * 256.0f;
				texel[2][1] = m->texelL[face->points[2]][1] * 256.0f;
			}
			else
			{
				texel[0][0] = m->texelL[face->points[0]][0] * 256.0f;
				texel[0][1] = (1.0f - m->texelL[face->points[0]][1]) * 256.0f;
				texel[1][0] = m->texelL[face->points[1]][0] * 256.0f;
				texel[1][1] = (1.0f - m->texelL[face->points[1]][1]) * 256.0f;
				texel[2][0] = m->texelL[face->points[2]][0] * 256.0f;
				texel[2][1] = (1.0f - m->texelL[face->points[2]][1]) * 256.0f;
			}

			if (reverseWinding)
			{
				fprintf(o, "\t%d 3 %d %d %d", baseTexFlags, (int)face->points[2], (int)face->points[1], (int)face->points[0]);
				fprintf(o, " %d %d %d %d %d %d\n", texel[2][0], texel[2][1], texel[1][0], texel[1][1], texel[0][0], texel[0][1]);
			}
			else
			{
				fprintf(o, "\t%d 3 %d %d %d", baseTexFlags, (int)face->points[0], (int)face->points[1], (int)face->points[2]);
				fprintf(o, " %d %d %d %d %d %d\n", texel[0][0], texel[0][1], texel[1][0], texel[1][1], texel[2][0], texel[2][1]);
			}
		}
	}
}


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
