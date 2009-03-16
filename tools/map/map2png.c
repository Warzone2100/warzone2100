// gcc -o ~/bin/map2png map2png.c mapload.c -I. -lphysfs -lpng -I../../lib/framework

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "mapload.h"

#include <png.h>

#define debug(x, ...) printf(__VA_ARGS__); printf("\n");

/**************************************************************************
  Save an RGBA8888 image buffer to a PNG file.
**************************************************************************/
bool savePng(const char *filename, char *pixels, int w, int h)
{
	png_structp pngp;
	png_infop infop;
	int y;
	FILE *fp;
	png_bytep *row_pointers;

	if (filename == NULL || *filename == '\0' || pixels == NULL)
	{
		return false;
	}
	if (!(fp = fopen(filename, "wb")))
	{
		debug(LOG_ERROR, "%s won't open for writing!", filename);
		return false;
	}
	pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngp)
	{
		debug(LOG_ERROR, "fgl_save_pixmap: Failed to create PNG write struct");
		fclose(fp);
		return false;
	}

	infop = png_create_info_struct(pngp);
	if (!infop)
	{
		debug(LOG_ERROR, "fgl_save_pixmap: Failed to create PNG info struct");
		fclose(fp);
		png_destroy_write_struct(&pngp, NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(pngp)))
	{
		debug(LOG_ERROR, "fgl_save_pixmap: Failed to initialize IO during PNG save");
		fclose(fp);
		png_destroy_write_struct(&pngp, NULL);
		return false;
	}
	png_init_io(pngp, fp);

	/* write header */
	if (setjmp(png_jmpbuf(pngp)))
	{
		debug(LOG_ERROR, "fgl_save_pixmap: Failed to write PNG header");
		fclose(fp);
		png_destroy_write_struct(&pngp, NULL);
		return false;
	}
	png_set_IHDR(pngp, infop, w, h, 8, PNG_COLOR_TYPE_RGBA,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
	             PNG_FILTER_TYPE_BASE);
	png_write_info(pngp, infop);

	/* Create pointers to each row of the data, as libpng wants it */
	row_pointers = malloc(sizeof(png_bytep) * h);
	for (y = 0; y < h; y++)
	{
		row_pointers[y] = pixels + (y * w * 4);
	}

	if (setjmp(png_jmpbuf(pngp)))
	{
		debug(LOG_ERROR, "fgl_save_pixmap: Failed to write PNG body");
		fclose(fp);
		png_destroy_write_struct(&pngp, NULL);
		return false;
	}
	png_write_image(pngp, row_pointers);

	if (setjmp(png_jmpbuf(pngp)))
	{
		debug(LOG_ERROR, "fgl_save_pixmap: Failed to finalize PNG");
		fclose(fp);
		png_destroy_write_struct(&pngp, NULL);
		return false;
	}
	png_write_end(pngp, NULL);

	free(row_pointers);
	fclose(fp);
	png_destroy_write_struct(&pngp, NULL);

	return true;
}

int main(int argc, char **argv)
{
	char filename[PATH_MAX];
	char path[PATH_MAX], *delim;
	GAMEMAP *map;

	if (argc != 2)
	{
		printf("Usage: %s <map>\n", argv[0]);
		return -1;
	}
	strcpy(path, argv[1]);
	delim = strrchr(path, '/');
	if (delim)
	{
		*delim = '\0';
		delim++;
		strcpy(filename, delim);
	}
	else
	{
		path[1] = '.';
		path[1] = '\0';
		strcpy(filename, argv[1]);
	}
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath(path, 1);

	map = mapLoad(filename);
	if (map)
	{
		int x, y;
		char *pixels = malloc(4 * map->width * map->height);	// RGBA 32 bits

		for (x = 0; x < map->width; x++)
		{
			for (y = 0; y < map->height; y++)
			{
				MAPTILE *psTile = mapTile(map, x, y);
				int pixpos = y * map->width * 4 + x * 4;

				pixels[pixpos++] = psTile->height;
				pixels[pixpos++] = psTile->height;
				pixels[pixpos++] = psTile->height;
				pixels[pixpos] = 255;	// alpha
			}
		}
		strcat(filename, ".png");
		savePng(filename, pixels, map->width, map->height);
		free(pixels);
	}
	mapFree(map);

	return 0;
}
