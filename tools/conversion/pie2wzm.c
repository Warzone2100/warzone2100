/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2015  Warzone 2100 Project

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
#include <stdio.h>
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

// The WZM format is a proposed successor to the PIE format used by Warzone.
// For an explanation of the WZM format, see http://wiki.wz2100.net/WZM_format

// To compile: gcc -o pie2wzm pie2wzm.c -Wall -g -O0 -Wshadow

#define iV_IMD_TEX	0x00000200
#define iV_IMD_XNOCUL	0x00002000
#define iV_IMD_TEXANIM	0x00004000
#define MAX_POLYGON_SIZE 16

static bool swapYZ = false;
static bool invertUV = false;
static bool reverseWinding = false;
static bool assumeAnimation = false;
static char *input = "";
static char output[PATH_MAX];
static bool verbose = false;

typedef struct {
	int index[MAX_POLYGON_SIZE];
	int texCoord[MAX_POLYGON_SIZE][2];
	int vertices;
	int frames, rate, width, height; // animation data
	bool cull;
} WZ_FACE;

typedef struct {
	int x, y, z;
} WZ_POSITION;

static void parse_args(int argc, char **argv)
{
	unsigned int i = 1;
	char *dot;

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
		if (argv[i][1] == 'v')
		{
			verbose = true;
		}
		if (argv[i][1] == 'r')
		{
			reverseWinding = true;
		}
		if (argv[i][1] == 'a')
		{
			assumeAnimation = true;
		}
	}
	if (argc < 1 + i)
	{
		fprintf(stderr, "Syntax: pie2wzm [-y|-r|-t] input_filename\n");
		fprintf(stderr, "  -y  Swap the Y and Z axis.\n");
		fprintf(stderr, "  -r  Reverse winding of all polygons.\n");
		fprintf(stderr, "  -i  Invert the vertical texture coordinates (for 3DS MAX etc.).\n");
		fprintf(stderr, "  -a  Do not make a guess about team colour usage. Assume animation.\n");
		fprintf(stderr, "  -v  Verbose mode.\n");
		exit(1);
	}
	input = argv[i++];
	strncpy(output, input, PATH_MAX);
	dot = strrchr(output, '.');
	*dot = '\0';
	strcat(output, ".wzm");
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
	fprintf(ctl, "TEXTURE %s\n", s);

	num = fscanf(fp, "LEVELS %d\n", &levels);
	if (num != 1)
	{
		fprintf(stderr, "Bad LEVELS directive in %s\n", input);
		exit(1);
	}
	fprintf(ctl, "MESHES %d\n", levels);
	if (verbose) printf("PIE file %s with %d levels is being parsed\n", input, levels);

	for (level = 0; level < levels; level++)
	{
		int j, points, faces, facesWZM, faceCount, pointsWZM, pointCount, textureArrays = 1;
		WZ_FACE *faceList;
		WZ_POSITION *posList;

		num = fscanf(fp, "\nLEVEL %d\n", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad LEVEL directive in %s.\n", input);
			exit(1);
		}
		if (level + 1 != x)
		{
			fprintf(stderr, "LEVEL directive in %s was %d should be %d.\n", input, x, level + 1);
			exit(1);
		}
		if (verbose) printf("Parsing level %d\n", x);
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

			if (flags & iV_IMD_XNOCUL)
			{
				faceList[j].cull = true;
				facesWZM++;	// must add additional face that is faced in the opposite direction
			}
			else
			{
				faceList[j].cull = false;
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
				if (faceList[j].cull)
				{
					facesWZM += (faceList[j].vertices - 3); // double the number of extra faces needed
				}
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
					fprintf(stderr, "File %slevel %d, polygon %d has a single animation frame. That makes no sense.\n", input, level, j);
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
				faceList[j].width = 1; // to avoid division by zero
				faceList[j].height = 1;
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
		if (textureArrays == 8 && !assumeAnimation)	// guesswork
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
					// This works because wrap around is only permitted if you start the animation at the
					// left border of the texture. What a horrible hack this was.
					int framesPerLine = 256 / faceList[j].width;
					int frameH = z % framesPerLine;
					int frameV = z / framesPerLine;
					double width = faceList[j].texCoord[k][0] + faceList[j].width * frameH;
					double height = faceList[j].texCoord[k][1] + faceList[j].height * frameV;
					double u = width / 256.0f;
					double v = height / 256.0f;

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
		for (j = 0; j < faces; j++)
		{
			int k, key, previous;

			key = faceList[j].index[0];
			previous = faceList[j].index[2];
			faceCount++;
			if (reverseWinding || faceList[j].cull)
			{
				fprintf(ctl, "\n\t%d %d %d", key, previous, faceList[j].index[1]);
			}
			if (!reverseWinding || faceList[j].cull)
			{
				fprintf(ctl, "\n\t%d %d %d", key, faceList[j].index[1], previous);
			}

			// Generate triangles from the Warzone triangle fans (PIEs, get it?)
			for (k = 3; k < faceList[j].vertices; k++)
			{
				if (reverseWinding || faceList[j].cull)
				{
					fprintf(ctl, "\n\t%d %d %d", key, faceList[j].index[k], previous);
				}
				if (!reverseWinding || faceList[j].cull)
				{
					fprintf(ctl, "\n\t%d %d %d", key, previous, faceList[j].index[k]);
				}
				previous = faceList[j].index[k];
			}
		}

		// We only handle texture animation here, so leave bone heap animation out of it for now.
		if (textureArrays == 1 || (textureArrays == 8 && !assumeAnimation))
		{
			// none is just as good as one
			fprintf(ctl, "\nFRAMES 0");
		}
		else
		{
			fprintf(ctl, "\nFRAMES %d", textureArrays);
			for (j = 0; j < textureArrays; j++)
			{
				fprintf(ctl, "\n\t%f %d 0 0 0 0 0 0", (float)faceList[j].rate, j);
			}
		}

		num = fscanf(fp, "\nCONNECTORS %d", &x);
		if (num == 1)
		{
			if (x < 0)
			{
				fprintf(stderr, "Bad CONNNECTORS directive in %s, level %d\n", input, level);
				exit(EXIT_FAILURE);
			}
			if (verbose) printf("Read %d connectors\n", x);
			fprintf(ctl, "\nCONNECTORS %d", x);
			for (j = 0; num == 1 && j < x; ++j)
			{
				int a, b, c;

				num = fscanf(fp, "\n%d %d %d", &a, &b, &c);
				if (num != 3)
				{
					fprintf(stderr, "Bad CONNECTORS directive entry level %d, number %d\n", level, j);
					exit(1);
				}
				fprintf(ctl, "\n\t%d %d %d 0", a, b, c);
			}
			fprintf(ctl, "\n");
		}

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
		fprintf(stderr, "Cannot open \"%s\" for reading: %s\n", input, strerror(errno));
		exit(1);
	}
	f = fopen(output, "w");
	if (!f)
	{
		fprintf(stderr, "Cannot open \"%s\" for reading: %s\n", output, strerror(errno));
		exit(1);
	}
	dump_to_wzm(f, p);
	fclose(f);
	fclose(p);

	return 0;
}
