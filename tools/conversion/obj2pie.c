/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2009  Warzone Resurrection Project

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <stdbool.h>
#include <limits.h>
#else
typedef int bool;
#define PATH_MAX 255
#define true 1
#define false 0
#endif

static char *input_file = "";
static char *output_file = "";
static char *texture_file;
static bool swapYZ = true;
static bool reverseWinding = true;
static bool invertUV = true;
static unsigned int baseTexFlags = 200;
static float scaleFactor = 1.0f;

typedef struct _vector3i
{
	int x, y, z;
} vector3i;

typedef struct _vector2i
{
	int u, v;
} vector2i;

typedef struct _face
{
	int index_a_vertex, index_a_texture;
	int index_b_vertex, index_b_texture;
	int index_c_vertex, index_c_texture;
} face;

static vector3i *points;
static int count_points;

static vector2i *uvcoords;
static int count_uvcoords;

static face *faces;
static int count_faces;

void addpoint(int x, int y, int z)
{
	if (points == NULL)
	{
		points = (vector3i*) malloc(sizeof(vector3i));
		count_points = 1;
	}
	else
	{
		count_points++;
		points = (vector3i*)realloc(points, count_points * sizeof(vector3i));
	}

	points[count_points-1].x = x;
	points[count_points-1].y = y;
	points[count_points-1].z = z;
}

void adduvcoord(int u, int v)
{
	if (uvcoords == NULL)
	{
		uvcoords = (vector2i*) malloc(sizeof(vector2i));
		count_uvcoords = 1;
	}
	else
	{
		count_uvcoords++;
		uvcoords = (vector2i*)realloc(uvcoords, count_uvcoords * sizeof(vector2i));
	}

	if (u > 255)
	{
		u = 255;
	}
	if (v > 255)
	{
		v = 255;
	}
	uvcoords[count_uvcoords-1].u = u;
	uvcoords[count_uvcoords-1].v = v;
}

void addface(int index_a_vertex, int index_a_texture, int index_b_vertex, int index_b_texture, int index_c_vertex, int index_c_texture)
{
	if (faces == NULL)
	{
		faces = (face*) malloc(sizeof(face));
		count_faces = 1;
	}
	else
	{
		count_faces++;
		faces = (face*) realloc(faces, count_faces * sizeof(face));
	}

	faces[count_faces-1].index_a_vertex = index_a_vertex;
	faces[count_faces-1].index_a_texture = index_a_texture;
	faces[count_faces-1].index_b_vertex = index_b_vertex;
	faces[count_faces-1].index_b_texture = index_b_texture;
	faces[count_faces-1].index_c_vertex = index_c_vertex;
	faces[count_faces-1].index_c_texture = index_c_texture;
}

void readobj(FILE *input)
{
	char buffer[256];
	int index_a_vertex, index_a_texture, index_b_vertex, index_b_texture, index_c_vertex, index_c_texture;
	float u, v;
	float x, y, z;

	if (input == NULL)
	{
		return;
	}

	points = NULL;
	count_points = 0;

	uvcoords = NULL;
	count_uvcoords = 0;

	faces = NULL;
	count_faces = 0;

	while (!feof(input))
	{
		fgets(buffer, 256, input);

		if (buffer[0] == 'v' && buffer[1] != 't')
		{
			sscanf(&buffer[2], "%f %f %f", &x, &y, &z);
			addpoint((int)(x*scaleFactor), (int)(y*scaleFactor), (int)(z*scaleFactor));
		}
		else if (buffer[0] == 'v' && buffer[1] == 't')
		{
			sscanf(&buffer[3], "%f %f", &u, &v);
			if (invertUV)
				v = 1.0f - v;
			adduvcoord((int)(u*256.f), (int)(v*256.f));
		}
		else if (buffer[0] == 'f')
		{
			sscanf(&buffer[2], "%d/%d %d/%d %d/%d", &index_a_vertex, &index_a_texture, &index_b_vertex, &index_b_texture, &index_c_vertex, &index_c_texture);
			addface(index_a_vertex - 1, index_a_texture - 1, index_b_vertex - 1, index_b_texture - 1, index_c_vertex - 1, index_c_texture - 1);
		}
	}
}


void writepie(FILE *output)
{
	size_t i;

	if (output == NULL)
	{
		return;
	}

	fprintf(output, "PIE 2\nTYPE 200\n");
	fprintf(output, "TEXTURE 0 %s 256 256\n", texture_file);
	fprintf(output, "LEVELS 1\nLEVEL 1\n");

	fprintf(output, "POINTS %d\n", count_points);
	for (i = 0;i < count_points;i++)
	{
		if (swapYZ)
		{
			fprintf(output, "\t%d %d %d\n", points[i].x, points[i].z, points[i].y);
		}
		else
		{
			fprintf(output, "\t%d %d %d\n", points[i].x, points[i].y, points[i].z);
		}
	}

	fprintf(output, "POLYGONS %d\n", count_faces);
	for (i = 0;i < count_faces;i++)
	{
		if (reverseWinding)
		{
			fprintf(output, "\t%d 3 %d %d %d %d %d %d %d %d %d\n",
			        baseTexFlags,
			        faces[i].index_c_vertex,
			        faces[i].index_b_vertex,
			        faces[i].index_a_vertex,
			        uvcoords[faces[i].index_c_texture].u,
			        uvcoords[faces[i].index_c_texture].v,
			        uvcoords[faces[i].index_b_texture].u,
			        uvcoords[faces[i].index_b_texture].v,
			        uvcoords[faces[i].index_a_texture].u,
			        uvcoords[faces[i].index_a_texture].v);
		}
		else
		{
			fprintf(output, "\t%d 3 %d %d %d %d %d %d %d %d %d\n",
			        baseTexFlags,
			        faces[i].index_a_vertex,
			        faces[i].index_b_vertex,
			        faces[i].index_c_vertex,
			        uvcoords[faces[i].index_a_texture].u,
			        uvcoords[faces[i].index_a_texture].v,
			        uvcoords[faces[i].index_b_texture].u,
			        uvcoords[faces[i].index_b_texture].v,
			        uvcoords[faces[i].index_c_texture].u,
			        uvcoords[faces[i].index_c_texture].v);
		}
	}
}

void objtopie()
{
	FILE *input = NULL;
	FILE *output = NULL;

	input = fopen(input_file, "rt");
	if (input == NULL)
	{
		perror("Couldn't open input file");
		return;
	}

	output = fopen(output_file, "wt");
	if (output == NULL)
	{
		perror("Couldn't open output file");
		fclose(input);
		return;
	}

	readobj(input);
	writepie(output);

	fclose(input);
	fclose(output);
}

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
		fprintf(stderr, "Syntax: obj2pie [-s] [-y] [-r] [-i] [-t] input_filename output_filename texture_filename\n");
		fprintf(stderr, "  -y    Do not swap Y and Z axis. Exporter uses Y-axis as \"up\".\n");
		fprintf(stderr, "  -r    Do not reverse winding of all polygons.\n");
		fprintf(stderr, "  -i    Do not invert the vertical texture coordinates.\n");
		fprintf(stderr, "  -s N  Scale model points by N before converting.\n");
		fprintf(stderr, "  -t    Use two sided polygons (slower; deprecated).\n");
		exit(1);
	}
	input_file = argv[i++];
	output_file = argv[i++];
	texture_file = argv[i++];
}

int main(int argc, char *argv[])
{
	parse_args(argc, argv);
	objtopie();
	return 0;
}
