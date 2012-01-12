/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2012  Warzone 2100 Project

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

// compile command: gcc obj2pie.c -o obj2pie -lm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
static bool invertV = true;
static int pieVersion = 3;
static bool twoSided = false;
static unsigned int pieType = 0x200;
static float scaleFactor = 1.0f;

typedef struct _vector3f
{
	float x, y, z;
} vector3f;

typedef struct _vector2f
{
	float u, v;
} vector2f;

typedef struct _face
{
	int index_a_vertex, index_a_texture;
	int index_b_vertex, index_b_texture;
	int index_c_vertex, index_c_texture;
} face;

static vector3f *points;
static int count_points;

static vector2f *uvcoords;
static int count_uvcoords;

static face *faces;
static int count_faces;

void addpoint(float x, float y, float z)
{
	if (points == NULL)
	{
		points = (vector3f*) malloc(sizeof(vector3f));
		count_points = 1;
	}
	else
	{
		count_points++;
		points = (vector3f*)realloc(points, count_points * sizeof(vector3f));
	}

	if( pieVersion == 2)
	{
		points[count_points-1].x = rintf(x);
		points[count_points-1].y = rintf(y);
		points[count_points-1].z = rintf(z);
	}
	else // pieVersion == 3
	{
		points[count_points-1].x = x;
		points[count_points-1].y = y;
		points[count_points-1].z = z;
	}
}

void adduvcoord(float u, float v)
{
	if (uvcoords == NULL )
	{
		uvcoords = (vector2f*) malloc(sizeof(vector2f));
		count_uvcoords = 1;
	}
	else
	{
		count_uvcoords++;
		uvcoords = (vector2f*)realloc(uvcoords, count_uvcoords * sizeof(vector2f));
	}

	if( pieVersion == 2)
	{
		uvcoords[count_uvcoords-1].u = rintf(u*256.0f);
		uvcoords[count_uvcoords-1].v = rintf(v*256.0f);;
	}
	else // pieVersion == 3
	{
		uvcoords[count_uvcoords-1].u = u;
		uvcoords[count_uvcoords-1].v = v;
	}
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
	int ignored_a_normal, ignored_b_normal, ignored_c_normal;
	float u, v;
	float x, y, z;
	bool normalsDetected = false;

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

	while (fgets(buffer, 256, input) != NULL)
	{
		if (buffer[0] == 'v' && buffer[1] == 'n')
		{
			normalsDetected = true;
		}
		else if (buffer[0] == 'v' && buffer[1] == 't')
		{
			sscanf(&buffer[3], "%f %f", &u, &v);
			if (invertV)
				v = 1.0f - v;
			adduvcoord(u,v);
		}
		else if (buffer[0] == 'v')		// && buffer[1] != 't' && buffer[1] != 'n'
		{
			sscanf(&buffer[2], "%f %f %f", &x, &y, &z);
			addpoint(x*scaleFactor,
					 (swapYZ?z:y)*scaleFactor,
					 (swapYZ?y:z)*scaleFactor);
		}
		else if (buffer[0] == 'f' )
		{
			if(normalsDetected)
			{
				sscanf(&buffer[2], "%d/%d/%d %d/%d/%d %d/%d/%d",
					   &index_a_vertex, &index_a_texture, &ignored_a_normal,
					   &index_b_vertex, &index_b_texture, &ignored_b_normal,
					   &index_c_vertex, &index_c_texture, &ignored_c_normal);
			}
			else
			{
				sscanf(&buffer[2], "%d/%d %d/%d %d/%d",
					   &index_a_vertex, &index_a_texture,
					   &index_b_vertex, &index_b_texture,
					   &index_c_vertex, &index_c_texture);
			}

			if(reverseWinding || twoSided)
			{
				addface(index_c_vertex - 1, index_c_texture - 1,
						index_b_vertex - 1, index_b_texture - 1,
						index_a_vertex - 1, index_a_texture - 1);
			}
			if(!reverseWinding || twoSided)
			{
				addface(index_a_vertex - 1, index_a_texture - 1,
						index_b_vertex - 1, index_b_texture - 1,
						index_c_vertex - 1, index_c_texture - 1);
			}
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

	fprintf(output, "PIE %d\nTYPE %x\n", pieVersion, pieType);
	fprintf(output, "TEXTURE 0 %s 256 256\n", texture_file);
	fprintf(output, "LEVELS 1\nLEVEL 1\n");

	fprintf(output, "POINTS %d\n", count_points);
	for (i = 0;i < count_points;i++)
	{
		if( pieVersion == 2)
		{
			fprintf(output, "\t%d %d %d\n", (int)points[i].x, (int)points[i].y, (int)points[i].z);
		}
		else // pieVersion == 3
		{
			fprintf(output, "\t%f %f %f\n", points[i].x, points[i].y, points[i].z);
		}
	}

	fprintf(output, "POLYGONS %d\n", count_faces);
	for (i = 0;i < count_faces;i++)
	{
		if(pieVersion == 2)
		{
			fprintf(output, "\t200 3 %d %d %d %d %d %d %d %d %d\n",
					faces[i].index_a_vertex,
					faces[i].index_b_vertex,
					faces[i].index_c_vertex,
					(int)uvcoords[faces[i].index_a_texture].u,
					(int)uvcoords[faces[i].index_a_texture].v,
					(int)uvcoords[faces[i].index_b_texture].u,
					(int)uvcoords[faces[i].index_b_texture].v,
					(int)uvcoords[faces[i].index_c_texture].u,
					(int)uvcoords[faces[i].index_c_texture].v);
		}
		else // pieVersion == 3
		{
			fprintf(output, "\t200 3 %d %d %d %f %f %f %f %f %f\n",
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
			invertV = false;
		}
		else if (argv[i][1] == 't')
		{
			twoSided = true;
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
		else if (argv[i][1] == 'v')
		{
			int ret;

			i++;
			if (argc < i)
			{
				fprintf(stderr, "Missing parameter to pie version option.\n");
				exit(1);
			}
			ret = sscanf(argv[i], "%d", &pieVersion);
			if (ret != 1)
			{
				fprintf(stderr, "Bad parameter to pie version option.\n");
				exit(1);
			}
			if( pieVersion < 2 || pieVersion > 3)
			{
				fprintf(stderr, "Unsupported pie version %d.\n",pieVersion);
				exit(1);
			}
		}
		else if (argv[i][1] == 'm')
		{
			pieType |= 0x10000;
		}
		else
		{
			fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
			exit(1);
		}
	}
	if (argc < 3 + i)
	{
		fprintf(stderr, "Usage: obj2pie [options] input_filename output_filename texture_filename\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "  -y		Do not swap Y and Z axis. Exporter uses Y-axis as \"up\".\n");
		fprintf(stderr, "  -r		Do not reverse winding of all polygons.\n");
		fprintf(stderr, "  -i		Do not invert the vertical texture coordinates.\n");
		fprintf(stderr, "  -s N		Scale model points by N before converting.\n");
		fprintf(stderr, "  -t		Use two sided polygons (Create duplicate faces with reverse winding).\n");
		fprintf(stderr, "  -v N		Pie export version ( 2 and 3 supported, 3 is default).\n");
		fprintf(stderr, "  -m		Use tcmask pie type.\n");

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
