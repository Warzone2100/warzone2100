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

// Based on 3ds2m.c from lib3ds

// gcc -o 3ds2pie 3ds2pie.c -Wall -g -O0 `pkg-config --cflags --libs lib3ds`

static char *filename = "";
static char *output = "";

static void parse_args(int argc, char **argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "Syntax: 3ds2m input_filename output_filename\n");
		exit(1);
	}
	filename = argv[1];
	output = argv[2];
}

static void dump_pie_file(Lib3dsFile *f, FILE *o)
{
	Lib3dsMesh *m;
	Lib3dsMaterial *material;
	int i;

	fprintf(o, "PIE 2\n");
	fprintf(o, "TYPE 200\n");

	for (i = 0, material = f->materials; material; material = material->next, i++)
	{
		Lib3dsTextureMap *texture = &material->texture1_map;

		if (i > 0)
		{
			fprintf(stderr, "Texture %d %s: More than one texture currently not supported!\n", i, texture->name);
			continue;
		}
		fprintf(o, "TEXTURE %d %s 256 256\n", i, texture->name);
	}

	fprintf(o, "LEVELS %d\n", f->frames);
	for (i = 0, m = f->meshes; m; m = m->next, i++)
	{
		if (i > 0)
		{
			fprintf(stderr, "Mesh %d %s: More than one texture currently not supported!\n", i, m->name);
			continue;
		}

		fprintf(o, "LEVEL %d\n", i); // I think this is correct? not sure how 3ds does animations
		fprintf(o, "POINTS %d\n", m->points);
		for (i = 0; i < m->points; ++i)
		{
			Lib3dsVector pos;

			lib3ds_vector_copy(pos, m->pointL[i].pos);
			fprintf(o, "\t%d %d %d\n", (int)pos[0], (int)pos[1], (int)pos[2]);
		}

		fprintf(o, "POLYGONS %d\n", m->faces);
		for (i = 0; i < m->faces; ++i)
		{
			Lib3dsFace *f = &m->faceL[i];

			fprintf(o, "\t200 3 %d %d %d", (int)f->points[0], (int)f->points[1], (int)f->points[2]);
			fprintf(o, "%d %d %d %d %d %d\n", (int)(m->texelL[f->points[0]][0] * 256), (int)(m->texelL[f->points[0]][1] * 256),
			                                  (int)(m->texelL[f->points[1]][0] * 256), (int)(m->texelL[f->points[1]][1] * 256),
			                                  (int)(m->texelL[f->points[2]][0] * 256), (int)(m->texelL[f->points[2]][1] * 256));
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
