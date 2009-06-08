#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <png.h>

#include "pngsave.h"

#define debug(x, ...) printf(__VA_ARGS__); printf("\n");

/**************************************************************************
  Save given buffer type to file.
**************************************************************************/
bool savePngInternal(const char *filename, unsigned char *pixels, int w, int h, int bitdepth, int color_type)
{
	png_structp pngp;
	png_infop infop;
	int y;
	FILE *fp;
	png_bytep *row_pointers;
	int const bytes = bitdepth / 8;

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
	png_set_IHDR(pngp, infop, w, h, bitdepth, color_type,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
	             PNG_FILTER_TYPE_BASE);
	png_write_info(pngp, infop);

	/* Create pointers to each row of the data, as libpng wants it */
	row_pointers = malloc(sizeof(png_bytep) * h);
	for (y = 0; y < h; y++)
	{
		row_pointers[y] = pixels + (y * w * bytes);
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


/**************************************************************************
  Save an RGBA8888 image buffer to a PNG file.
**************************************************************************/
bool savePng(const char *filename, char *pixels, int w, int h)
{
	return savePngInternal(filename, (unsigned char *)pixels, w, h, 8, PNG_COLOR_TYPE_RGBA);
}

/**************************************************************************
  Save an I16 image buffer to a PNG file.
**************************************************************************/
bool savePngI16(const char *filename, uint16_t *pixels, int w, int h)
{
	return savePngInternal(filename, (unsigned char *)pixels, w, h, 16, PNG_COLOR_TYPE_GRAY);
}

/**************************************************************************
  Save an I8 image buffer to a PNG file.
**************************************************************************/
bool savePngI8(const char *filename, uint8_t *pixels, int w, int h)
{
	return savePngInternal(filename, (unsigned char *)pixels, w, h, 8, PNG_COLOR_TYPE_GRAY);
}
