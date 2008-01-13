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
 *
 * $Id: 3ds2m.c,v 1.1 2001/07/18 12:17:49 jeh Exp $
 */
#include <lib3ds/file.h>
#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Based on 3ds2m.c from lib3ds

// gcc -o 3ds2pie 3ds2pie.c -Wall -g -O0 `pkg-config --cflags --libs lib3ds` -Wshadow

static char *filename = "";
static char *output = "";
static char *page = "";
static bool switchYZ = true;
static bool reversewinding = false;
static bool inverseUV = false;
static int baseTexFlags = 200;

static void parse_args(int argc, char **argv)
{
	int i = 1;

	while (argc >= 3 + i && argv[i][0] == '-')
	{
		if (argv[i][1] == 'y')
		{
			switchYZ = false; // exporting program used Y-axis as "up", like we do, don't switch
		}
		if (argv[i][1] == 'i')
		{
			inverseUV = true;
		}
		if (argv[i][1] == 't')
		{
			baseTexFlags = 2200;
		}
		if (argv[i][1] == 'r')
		{
			reversewinding = true;
		}
		i++;
	}
	if (argc < 3 + i)
	{
		fprintf(stderr, "Syntax: 3ds2m [-y|-r|-t] input_filename output_filename page_number\n");
		fprintf(stderr, "  -y  Exporter uses Y-axis as \"up\", so do not switch Y and Z axis.\n");
		fprintf(stderr, "  -r  Reverse winding of all polygons.\n");
		fprintf(stderr, "  -t  Use two sided polygons (slower; unused in WRP).\n");
		fprintf(stderr, "  -i  Invert the vertical texture coordinates (for 3DS MAX etc.).\n");
		exit(1);
	}
	filename = argv[i++];
	output = argv[i++];
	page = argv[i++];
}

// Make a string lower case
void resToLower(char *pStr)
{
	while (*pStr != 0)
	{
		if (isupper(*pStr))
		{
			*pStr = (char)(*pStr - (char)('A' - 'a'));
		}
		pStr += 1;
	}
}

static void dump_pie_file(Lib3dsFile *f, FILE *o)
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

	fprintf(o, "LEVELS %d\n", f->frames);
	for (meshIdx = 0, m = f->meshes; m; m = m->next, meshIdx++)
	{
		int i;

		if (meshIdx > 0)
		{
			fprintf(stderr, "Mesh %d %s: More than one frame currently not supported!\n", meshIdx, m->name);
			continue;
		}

		fprintf(o, "LEVEL %d\n", meshIdx); // I think this is correct? not sure how 3ds does animations
		fprintf(o, "POINTS %d\n", m->points);
		for (i = 0; i < m->points; i++)
		{
			Lib3dsVector pos;

			lib3ds_vector_copy(pos, m->pointL[i].pos);

			if (switchYZ)
			{
				fprintf(o, "\t%d %d %d\n", (int)pos[0], (int)pos[2], (int)pos[1]);
			}
			else
			{
				fprintf(o, "\t%d %d %d\n", (int)pos[0], (int)pos[1], (int)pos[2]);
			}
		}

		fprintf(o, "POLYGONS %d\n", m->faces);
		for (i = 0; i < m->faces; ++i)
		{
			Lib3dsFace *face = &m->faceL[i];
			int texel[3][2];

			if (!inverseUV)
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

			if (reversewinding)
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

int main(int argc, char **argv)
{
	Lib3dsFile *f = NULL;
	FILE *o;

	parse_args(argc, argv);
	f = lib3ds_file_load(filename);
	if (!f)
	{
		fprintf(stderr, "***ERROR***\nLoading file %s failed\n", filename);
		exit(1);
	}
	o = fopen(output, "w+");
	if (!o)
	{
		fprintf(stderr, "***ERROR***\nCan't open %s for writing\n", output);
		exit(1);
	}
	dump_pie_file(f, o);
	fclose(o);

	lib3ds_file_free(f);

	return 0;
}
