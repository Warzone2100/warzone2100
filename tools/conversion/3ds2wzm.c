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

// gcc -o 3ds2wzm 3ds2wzm.c -Wall -g -O0 `pkg-config --cflags --libs lib3ds` -Wshadow

// Make a string lower case
static void resToLower(char *pStr)
{
	while (*pStr != 0)
	{
		*pStr = tolower(*pStr);
		pStr += 1;
	}
}

void dump_wzm_file(Lib3dsFile *f, FILE *ctl, bool swapYZ, bool invertUV, bool reverseWinding, unsigned int baseTexFlags, float scaleFactor)
{
	Lib3dsMesh *m;
	Lib3dsMaterial *material;
	int meshIdx, j;

	fprintf(ctl, "WZM 1\n");

	for (j = 0, material = f->materials; material; material = material->next, j++)
	{
		Lib3dsTextureMap *texture = &material->texture1_map;

		resToLower(texture->name);
		if (j > 0)
		{
			fprintf(stderr, "Texture %d %s: More than one texture currently not supported!\n", j, texture->name);
			continue;
		}
		fprintf(ctl, "TEXTURE %s\n", texture->name);
	}

	for (j = 0, m = f->meshes; m; m = m->next, j++);
	fprintf(ctl, "MESHES %d\n", j);

	for (meshIdx = 0, m = f->meshes; m; m = m->next, meshIdx++)
	{
		unsigned int i;

		fprintf(ctl, "MESH %d\n", meshIdx);
		fprintf(ctl, "TEAMCOLOURS 0\n");
		fprintf(ctl, "VERTICES %d\n", m->points);
		fprintf(ctl, "FACES %d\n", m->faces);
		fprintf(ctl, "VERTEXARRAY\n");
		for (i = 0; i < m->points; i++)
		{
			Lib3dsVector pos;

			lib3ds_vector_copy(pos, m->pointL[i].pos);

			if (swapYZ)
			{
				fprintf(ctl, "\t%f %f %f\n", pos[0] * scaleFactor, pos[2] * scaleFactor, pos[1] * scaleFactor);
			}
			else
			{
				fprintf(ctl, "\t%f %f %f\n", pos[0] * scaleFactor, pos[1] * scaleFactor, pos[2] * scaleFactor);
			}
		}

		fprintf(ctl, "TEXTUREARRAYS 1\n");
		fprintf(ctl, "TEXTUREARRAY 0\n");
		for (i = 0; i < m->points; i++)
		{
			if (invertUV)
			{
				fprintf(ctl, "\t%f %f\n", m->texelL[i][0], 1.0f - m->texelL[i][1]);
			}
			else
			{
				fprintf(ctl, "\t%f %f\n", m->texelL[i][0], m->texelL[i][1]);
			}
		}

		fprintf(ctl, "INDEXARRAY\n");
		for (i = 0; i < m->faces; ++i)
		{
			Lib3dsFace *face = &m->faceL[i];

			if (reverseWinding)
			{
				fprintf(ctl, "\t%d %d %d\n", (int)face->points[2], (int)face->points[1], (int)face->points[0]);
			}
			else
			{
				fprintf(ctl, "\t%d %d %d\n", (int)face->points[0], (int)face->points[1], (int)face->points[2]);
			}
		}
		fprintf(ctl, "FRAMES 0\n");
		fprintf(ctl, "CONNECTORS 0\n");
	}
}


#if !defined(WZ_3DS2WZM_GUI)
static char *input_file = "";
static char *output_file = "";
static bool swapYZ = true;
static bool reverseWinding = true;
static bool invertUV = true;
static unsigned int baseTexFlags = 200;
static float scaleFactor = 1.0f;

static void parse_args(int argc, char **argv)
{
	unsigned int i = 1;

	for (i = 1; argc >= 2 + i && argv[i][0] == '-'; i++)
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
	if (argc < 2 + i)
	{
		fprintf(stderr, "Syntax: 3ds2wzm [-y] [-r] [-i] [-t] input_filename output_filename\n");
		fprintf(stderr, "  -y    Do not swap Y and Z axis. Exporter uses Y-axis as \"up\".\n");
		fprintf(stderr, "  -r    Do not reverse winding of all polygons.\n");
		fprintf(stderr, "  -i    Do not invert the vertical texture coordinates.\n");
		fprintf(stderr, "  -t    Use two sided polygons (slower; deprecated).\n");
		fprintf(stderr, "  -s N  Scale model points by N before converting.\n");
		exit(1);
	}
	input_file = argv[i++];
	output_file = argv[i++];
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
	dump_wzm_file(f, o, swapYZ, invertUV, reverseWinding, baseTexFlags, scaleFactor);
	fclose(o);

	lib3ds_file_free(f);

	return 0;
}
#endif
