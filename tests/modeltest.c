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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef WIN32
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#else
typedef int bool;
#define PATH_MAX 255
#define true 1
#define false 0
#endif

#define iV_IMD_TEX	0x00000200
#define iV_IMD_XNOCUL	0x00002000
#define iV_IMD_TEXANIM	0x00004000
#define iV_IMD_TCMASK	0x00010000
#define MAX_POLYGON_SIZE 3 // the game can't handle more

typedef struct {
	int index[MAX_POLYGON_SIZE];
	int texCoord[MAX_POLYGON_SIZE][2];
	int vertices;
	int frames, rate, width, height; // animation data
	bool cull;
} WZ_FACE;

typedef struct {
	int x, y, z, reindex;
	bool dupe;
} WZ_POSITION;

static void check_pie(const char *input)
{
	int num, x, y, z, levels, level;
	FILE *fp = fopen(input, "r");
	char s[200];

	if (!fp)
	{
		fprintf(stderr, "Unable to open %s from %s: %s\n", input, getcwd(s, sizeof(s) - 1), strerror(errno));
		exit(1);
	}
	num = fscanf(fp, "PIE %d\n", &x);
	if (num != 1)
	{
		fprintf(stderr, "Bad PIE file %s\n", input);
		exit(1);
	}
 	if (x != 2 && x != 3)
	{
		fprintf(stderr, "Unknown PIE version %d in %s\n", x, input);
		exit(1);
	}
	num = fscanf(fp, "TYPE %x\n", &x); // ignore
	if (num != 1)
	{
		fprintf(stderr, "Bad TYPE directive in %s\n", input);
		exit(1);
	}
	if (!(x & iV_IMD_TEX))
	{
		fprintf(stderr, "Missing texture flag in %s\n", input);
		exit(1);
	}
	num = fscanf(fp, "TEXTURE %d %s %d %d\n", &z, s, &x, &y);
	if (num != 4)
	{
		fprintf(stderr, "Bad TEXTURE directive in %s\n", input);
		exit(1);
	}
	num = fscanf(fp, "LEVELS %d\n", &levels);
	if (num != 1)
	{
		fprintf(stderr, "Bad LEVELS directive in %s\n", input);
		exit(1);
	}
	for (level = 0; level < levels; level++)
	{
		int j, points, faces;

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
		num = fscanf(fp, "POINTS %d\n", &points);
		if (num != 1)
		{
			fprintf(stderr, "Bad POINTS directive in %s, level %d.\n", input, level);
			exit(1);
		}
		for (j = 0; j < points; j++)
		{
			double a, b, c;
			num = fscanf(fp, "%f %f %f\n", &a, &b, &c);
			if (num != 3)
			{
				fprintf(stderr, "File %s. Bad POINTS entry level %d, number %d.\n", input, level, j);
				exit(1);
			}
		}
		num = fscanf(fp, "POLYGONS %d", &faces);
		if (num != 1)
		{
			fprintf(stderr, "Bad POLYGONS directive in %s, level %d.\n", input, level);
			exit(1);			
		}
		for (j = 0; j < faces; ++j)
		{
			int k;
			unsigned int flags;

			num = fscanf(fp, "\n%x", &flags);
			if (num != 1)
			{
				fprintf(stderr, "File %s. Bad POLYGONS flag entry level %d, number %d\n", input, level, j);
				exit(1);
			}
			if (!(flags & iV_IMD_TEX))
			{
				fprintf(stderr, "File %s. Bad polygon flag entry level %d, number %d - no texture flag!\n", input, level, j);
				exit(1);
			}
			if (flags & iV_IMD_XNOCUL)
			{
				fprintf(stderr, "File %s. Bad polygon flag entry level %d, number %d - face culling not supported anymore!\n", input, level, j);
				exit(1);
			}
			num = fscanf(fp, "%d", &k);
			if (num != 1)
			{
				fprintf(stderr, "File %s. Bad POLYGONS vertices entry level %d, number %d\n", input, level, j);
				exit(1);
			}
			if (k != 3)
			{
				fprintf(stderr, "File %s. Bad POLYGONS vertices entry level %d, number %d -- non-triangle polygon found\n", input, level, j);
				exit(1);
			}
			// Read in vertex indices and texture coordinates
			for (k = 0; k < 3; k++)
			{
				int l;
				num = fscanf(fp, "%d", &l);
				if (num != 1)
				{
					fprintf(stderr, "File %s. Bad vertex position entry level %d, number %d\n", input, level, j);
					exit(1);
				}
			}
			if (flags & iV_IMD_TEXANIM)
			{
				int frames, rate, width, height;
				num = fscanf(fp, "%d %d %d %d", &frames, &rate, &width, &height);
				if (num != 4)
				{
					fprintf(stderr, "File %s - Bad texture animation entry level %d, number %d.\n", input, level, j);
					exit(1);
				}
				if (frames <= 1)
				{
					fprintf(stderr, "File %s - Level %d, polygon %d has a single animation frame.\n", input, level, j);
					exit(1);
				}
			}
			for (k = 0; k < 3; k++)
			{
				double t, u;
				num = fscanf(fp, "%f %f", &t, &u);
				if (num != 2)
				{
					fprintf(stderr, "File %s. Bad texture coordinate entry level %d, number %d\n", input, level, j);
					exit(1);
				}
			}
		}
		num = fscanf(fp, "\nCONNECTORS %d", &x);
		if (num == 1)
		{
			if (x < 0)
			{
				fprintf(stderr, "Bad CONNECTORS directive in %s, level %d\n", input, level);
				exit(EXIT_FAILURE);
			}
			for (j = 0; j < x; ++j)
			{
				int a, b, c;

				num = fscanf(fp, "\n%d %d %d", &a, &b, &c);
				if (num != 3)
				{
					fprintf(stderr, "File %s. Bad CONNECTORS directive entry level %d, number %d\n", input, level, j);
					exit(1);
				}
			}
		}
	}
	fclose(fp);
}

int main(int argc, char **argv)
{
	char datapath[PATH_MAX], fullpath[PATH_MAX];
	FILE *fp = fopen("modellist.txt", "r");
	if (!fp)
	{
		fprintf(stderr, "%s: Failed to open list file\n", argv[0]);
		return -1;
	}
	strcpy(datapath, getenv("srcdir"));
	strcat(datapath, "/../data/");
	while (!feof(fp))
	{
		char filename[PATH_MAX];

		fscanf(fp, "%s\n", &filename);
		printf("Testing model: %s\n", filename);
		strcpy(fullpath, datapath);
		strcat(fullpath, filename);
		check_pie(fullpath);
	}
	fclose(fp);
	return 0;
}
