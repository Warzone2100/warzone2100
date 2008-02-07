/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2008  Warzone Resurrection Project

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
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

// The WZM format is a proposed successor to the PIE format used by Warzone.
// For an explanation of the WZM format, see http://wiki.wz2100.net/WZM_format

// To compile: gcc -o pie2wzm pie2wzm.c -Wall -g -O0 -Wshadow

#define iV_IMD_TEX 0x00000200
#define iV_IMD_TEXANIM 0x00004000
#define MAX_POLYGON_SIZE 16

static bool swapYZ = false;
static bool invertUV = false;
static bool reverseWinding = false;
static char *input = "";
static char *output = "";

typedef struct {
	int index[MAX_POLYGON_SIZE];
	int texCoord[MAX_POLYGON_SIZE][2];
	int vertices;
	int frames, rate, width, height; // animation data
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
			swapYZ = true; // exporting program used Y-axis as "up", like we do, don't switch
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
		fprintf(stderr, "Syntax: pie2wzm [-y|-r|-t] input_filename output_filename\n");
		fprintf(stderr, "  -y  Swap the Y and Z axis.\n");
		fprintf(stderr, "  -r  Reverse winding of all polygons. (DOES NOT WORK YET.)\n");
		fprintf(stderr, "  -i  Invert the vertical texture coordinates (for 3DS MAX etc.).\n");
		exit(1);
	}
	input = argv[i++];
	output = argv[i++];
}

static void dump_to_wzm(FILE *ctl, FILE *fp)
{
	int num, x, y, z, levels, level;
	char s[200];

	num = fscanf(fp, "PIE %d\n", &x);
	if (num != 1)
	{
		fprintf(stderr, "Bad PIE file %s\n", input);
		exit(1);
	}
	fprintf(ctl, "WZM 1\n");

	num = fscanf(fp, "TYPE %d\n", &x); // ignore
	if (num != 1)
	{
		fprintf(stderr, "Bad TYPE directive in %s\n", input);
		exit(1);
	}

	num = fscanf(fp, "TEXTURE %d %s %d %d\n", &z, s, &x, &y);
	if (num != 4)
	{
		fprintf(stderr, "Bad TEXTURE directive in %s\n", input);
		exit(1);
	}
	fprintf(ctl, "TEXTURE 0 %s\n", s);

	num = fscanf(fp, "LEVELS %d\n", &levels);
	if (num != 1)
	{
		fprintf(stderr, "Bad LEVELS directive in %s\n", input);
		exit(1);
	}
	fprintf(ctl, "MESHES %d\n", levels);

	// WZM does not support multiple meshes, nor importing them from PIE
	for (level = 0; level < levels; level++)
	{
		int j, points, faces, facesWZM, faceCount, pointsWZM, pointCount, textureArrays = 1;
		WZ_FACE *faceList;
		WZ_POSITION *posList;

		num = fscanf(fp, "LEVEL %d\n", &x);
		if (num != 1 || level + 1 != x)
		{
			fprintf(stderr, "Bad LEVEL directive in %s, was %d should be %d.\n", input, x, level + 1);
			exit(1);
		}
		fprintf(ctl, "MESH %d\n", level);

		num = fscanf(fp, "POINTS %d\n", &points);
		if (num != 1)
		{
			fprintf(stderr, "Bad POINTS directive in %s, level %d.\n", input, level);
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
				fprintf(stderr, "Bad POINTS entry level %d, number %d\n", level, j);
				exit(1);
			}
		}

		num = fscanf(fp, "POLYGONS %d", &faces);
		if (num != 1)
		{
			fprintf(stderr, "Bad POLYGONS directive in %s, level %d.\n", input, level);
			exit(1);			
		}
		facesWZM = faces;	// for starters
		faceList = malloc(sizeof(WZ_FACE) * faces);
		pointsWZM = 0;
		for (j = 0; j < faces; ++j)
		{
			int k;
			unsigned int flags;

			num = fscanf(fp, "\n%x", &flags);
			if (num != 1)
			{
				fprintf(stderr, "Bad POLYGONS texture flag entry level %d, number %d\n", level, j);
				exit(1);
			}
			if (!(flags & iV_IMD_TEX))
			{
				fprintf(stderr, "Bad texture flag entry level %d, number %d - no texture flag!\n", level, j);
				exit(1);
			}

			num = fscanf(fp, "%d", &faceList[j].vertices);
			if (num != 1)
			{
				fprintf(stderr, "Bad POLYGONS vertices entry level %d, number %d\n", level, j);
				exit(1);
			}
			assert(faceList[j].vertices <= MAX_POLYGON_SIZE); // larger polygons not supported
			assert(faceList[j].vertices >= 3); // points and lines not supported
			if (faceList[j].vertices > 3)
			{
				// since they are triangle fans already, we get to do easy tessellation
				facesWZM += (faceList[j].vertices - 3);
			}
			pointsWZM += faceList[j].vertices;

			// Read in vertex indices and texture coordinates
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%d", &faceList[j].index[k]);
				if (num != 1)
				{
					fprintf(stderr, "Bad vertex position entry level %d, number %d\n", level, j);
					exit(1);
				}
			}
			if (flags & iV_IMD_TEXANIM)
			{
				num = fscanf(fp, "%d %d %d %d", &faceList[j].frames, &faceList[j].rate, &faceList[j].width, &faceList[j].height);
				if (num != 4)
				{
					fprintf(stderr, "Bad texture animation entry level %d, number %d.\n", level, j);
					exit(1);
				}
				if (faceList[j].frames <= 1)
				{
					fprintf(stderr, "Level %d, polygon %d has a single animation frame. That makes no sense.\n", level, j);
				}
				if (textureArrays < faceList[j].frames)
				{
					textureArrays = faceList[j].frames;
				}
			}
			else
			{
				faceList[j].frames = 0;
				faceList[j].rate = 0;
				faceList[j].width = 0;
				faceList[j].height = 0;
			}
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%d %d", &faceList[j].texCoord[k][0], &faceList[j].texCoord[k][1]);
				if (num != 2)
				{
					fprintf(stderr, "Bad texture coordinate entry level %d, number %d\n", level, j);
					exit(1);
				}
			}
		}
		if (textureArrays == 8)	// guesswork
		{
			fprintf(ctl, "TEAMCOLOURS 1\n");
		}
		else
		{
			fprintf(ctl, "TEAMCOLOURS 0\n");
		}

		// Calculate position list. Since positions hold texture coordinates in WZM, unlike in Warzone,
		// we need to use some black magic to transfer them over here.
		pointCount = 0;
		fprintf(ctl, "VERTICES %d\n", pointsWZM);
		fprintf(ctl, "FACES %d\n", facesWZM);
		fprintf(ctl, "VERTEXARRAY");
		for (j = 0; j < faces; j++)
		{
			int k;

			for (k = 0; k < faceList[j].vertices; k++, pointCount++)
			{
				int pos = faceList[j].index[k];

				// Generate new position
				fprintf(ctl, " \n\t%d %d %d", posList[pos].x, posList[pos].y, posList[pos].z);

				// Use the new position
				faceList[j].index[k] = pointCount;
			}
		}

		fprintf(ctl, "\nTEXTUREARRAYS %d", textureArrays);
		// Handle texture animation or team colours. In either case, add multiple texture arrays.
		for (z = 0; z < textureArrays; z++)
		{
			fprintf(ctl, "\nTEXTUREARRAY %d", z);
			for (j = 0; j < faces; j++)
			{
				int k;

				for (k = 0; k < faceList[j].vertices; k++)
				{
					double u = (double)(faceList[j].texCoord[k][0] + faceList[j].width * z) / 256.0f;
					double v = (double)(faceList[j].texCoord[k][1] + faceList[j].height * z) / 256.0f;

					if (invertUV)
					{
						v = 1.0f - v;
					}
					fprintf(ctl, " \n\t%f %f", u, v);
				}
			}
		}

		faceCount = 0;

		fprintf(ctl, "\nINDEXARRAY");
		// TODO support reverse winding option?
		for (j = 0; j < faces; j++)
		{
			int k, key, previous;

			key = faceList[j].index[0];
			previous = faceList[j].index[2];
			faceCount++;
			fprintf(ctl, "\n\t%d %d %d", key, faceList[j].index[1], previous);

			// Generate triangles from the Warzone triangle fans (PIEs, get it?)
			for (k = 3; k < faceList[j].vertices; k++)
			{
				previous = faceList[j].index[k];
				fprintf(ctl, "\n\t%d %d %d", key, previous, faceList[j].index[k]);
			}
		}

		// We only handle texture animation here, so leave bone heap animation out of it for now.
		fprintf(ctl, "\nFRAMES %d", textureArrays);
		for (j = 0; j < textureArrays; j++)
		{
			fprintf(ctl, "\n\t0 %d %d", j, faceList[j].rate);
		}

		fprintf(ctl, "\nCONNECTORS 0\n"); // TODO

		free(faceList);
		free(posList);
	}
}

int main(int argc, char **argv)
{
	FILE *p, *f;

	parse_args(argc, argv);
	
	p = fopen(input, "r");
	if (!p)
	{
		fprintf(stderr, "Cannot open \"%s\" for reading: %s", input, strerror(errno));
		exit(1);
	}
	f = fopen(output, "w");
	if (!f)
	{
		fprintf(stderr, "Cannot open \"%s\" for reading: %s", output, strerror(errno));
		exit(1);
	}
	dump_to_wzm(f, p);
	fclose(f);
	fclose(p);

	return 0;
}
