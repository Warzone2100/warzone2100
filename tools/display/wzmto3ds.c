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

#include "wzmviewer.h"

#include <lib3ds/file.h>
#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>

// gcc -o wzmto3ds wzmto3ds.c wzmutils.c -Wall -g -O0 `pkg-config --cflags --libs lib3ds` `sdl-config --libs --cflags` -lpng

static bool swapYZ = true;
static bool invertUV = true;
static bool reverseWinding = true;
static char *input = "";
static char *output = "";

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
		fprintf(stderr, "Syntax: wzmto3ds [-y|-r|-t] input_filename output_filename\n");
		fprintf(stderr, "  -y  Exporter uses Y-axis as \"up\", so do not switch Y and Z axis.\n");
		fprintf(stderr, "  -r  Reverse winding of all polygons.\n");
		fprintf(stderr, "  -i  Invert the vertical texture coordinates (for 3DS MAX etc.).\n");
		exit(1);
	}
	input = argv[i++];
	output = argv[i++];
}

static void dump_to_3ds(MODEL *psModel, Lib3dsFile *f)
{
	Lib3dsMaterial	*material = lib3ds_material_new("Warzone texture");
	Lib3dsTextureMap *texture = &material->texture1_map;
	Lib3dsMesh	*iter = NULL;
	int		i;

	strcpy(texture->name, psModel->texPath);
	f->frames = psModel->meshes;
	f->materials = material;
	for (i = 0; i < psModel->meshes; i++)
	{
		Lib3dsMesh	*m = lib3ds_mesh_new("Warzone Mesh");
		MESH		*psMesh = &psModel->mesh[i];
		int		j;

		if (!iter)
		{
			f->meshes = m;
		}
		else
		{
			iter->next = m;
		}
		iter = m;

		lib3ds_mesh_new_point_list(m, psMesh->vertices);
		lib3ds_mesh_new_texel_list(m, psMesh->vertices);
		lib3ds_mesh_new_face_list(m, psMesh->faces);

		for (j = 0; j < psMesh->vertices; j++)
		{
			m->pointL[j].pos[0] = psMesh->vertexArray[j * 3 + 0];
			m->pointL[j].pos[1] = psMesh->vertexArray[j * 3 + 1];
			m->pointL[j].pos[2] = psMesh->vertexArray[j * 3 + 2];

			if (invertUV)
			{
				m->texelL[j][0] = psMesh->textureArray[0][j * 6 + 0];
				m->texelL[j][1] = 1.0f - psMesh->textureArray[0][j * 6 + 1];
			}
			else
			{
				m->texelL[j][0] = psMesh->textureArray[0][j * 6 + 0];
				m->texelL[j][1] = psMesh->textureArray[0][j * 6 + 1];
			}
		}

		for (j = 0; j < psMesh->faces; j++)
		{
			if (reverseWinding)
			{
				m->faceL[j].points[0] = psMesh->indexArray[j * 3 + 0];
				m->faceL[j].points[2] = psMesh->indexArray[j * 3 + 1];
				m->faceL[j].points[1] = psMesh->indexArray[j * 3 + 2];
			}
			else
			{
				m->faceL[j].points[0] = psMesh->indexArray[j * 3 + 0];
				m->faceL[j].points[1] = psMesh->indexArray[j * 3 + 1];
				m->faceL[j].points[2] = psMesh->indexArray[j * 3 + 2];
			}
		}
	}
}

int main(int argc, char **argv)
{
	Lib3dsFile *f = NULL;
	MODEL *psModel;

	parse_args(argc, argv);
	psModel = readModel(input, "");
	f = lib3ds_file_new();
	if (!f)
	{
		fprintf(stderr, "Cannot create new 3DS file!\n");
		exit(1);
	}
	dump_to_3ds(psModel, f);
	if (!lib3ds_file_save(f, output))
	{
		fprintf(stderr, "Cannot open \"%s\" for writing: %s", output, strerror(errno));
		exit(1);
	}
	lib3ds_file_free(f);

	return 0;
}
