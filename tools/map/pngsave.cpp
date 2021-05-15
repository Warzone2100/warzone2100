/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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

#include "pngsave.h"
#include <wzmaplib/map_debug.h>
#include <png.h>
#include <cstdlib>
#include <cstdarg>

template <unsigned N>
static inline int vssprintf(char (&dest)[N], char const *format, va_list params) { return vsnprintf(dest, N, format, params); }

static inline void PNGWriteCleanup(png_infop *info_ptr, png_structp *png_ptr, FILE* fileHandle)
{
	if (*info_ptr != NULL)
		png_destroy_info_struct(*png_ptr, info_ptr);
	if (*png_ptr != NULL)
		png_destroy_write_struct(png_ptr, NULL);
	if (fileHandle != NULL)
		fclose(fileHandle);
}

#define debug_error(...) do { \
	fprintf(stderr, __VA_ARGS__); \
} while(0)

#if defined(_MSC_VER)
// FIXME?: disable MSVC warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
__pragma(warning( push )) // see matching "pop" below
__pragma(warning( disable : 4611 ))
#endif

static bool savePngInternal(const char *fileName, uint8_t *pixels, unsigned w, unsigned h, int bitdepth, int color_type)
{
	uint8_t **scanlines = NULL;
	png_infop info_ptr = NULL;
	png_structp png_ptr = NULL;
	FILE *fp;
	
	if (fileName == NULL || *fileName == '\0' || pixels == NULL)
	{
		return false;
	}
	if (w <= 0 || h <= 0)
	{
		debug_error("savePng: Unsupported image dimensions: %d x %d", w, h);
		return false;
	}
	if (bitdepth <= 0)
	{
		debug_error("savePng: Unsupported bit depth: %d", bitdepth);
		return false;
	}
	if (!(fp = fopen(fileName, "wb")))
	{
		debug_error("savePng: %s won't open for writing!", fileName);
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		debug_error("savePng: Unable to create png struct\n");
		PNGWriteCleanup(&info_ptr, &png_ptr, fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		debug_error("savePng: Unable to create png info struct\n");
		PNGWriteCleanup(&info_ptr, &png_ptr, fp);
		return false;
	}

	// If libpng encounters an error, it will jump into this if-branch
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		debug_error("savePng: Error encoding PNG data\n");
		PNGWriteCleanup(&info_ptr, &png_ptr, fp);
		return false;
	}
	else
	{
		unsigned currentRow, row_stride;
		unsigned channelsPerPixel;

		switch (color_type)
		{
			case PNG_COLOR_TYPE_GRAY:
				channelsPerPixel = 1;
				break;
			case PNG_COLOR_TYPE_RGB:
				channelsPerPixel = 3;
				break;
			case PNG_COLOR_TYPE_RGBA:
				channelsPerPixel = 4;
				break;
			default:
				debug_error("savePng: Unsupported pixel format.\n");
				PNGWriteCleanup(&info_ptr, &png_ptr, fp);
				return false;
		}

		row_stride = w * channelsPerPixel * bitdepth / 8;

		scanlines = (uint8_t **)malloc(sizeof(uint8_t *) * h);
		if (scanlines == NULL)
		{
			debug_error("savePng: Couldn't allocate memory\n");
			PNGWriteCleanup(&info_ptr, &png_ptr, fp);
			return false;
		}

		png_init_io(png_ptr, fp);
		
		//png_set_write_fn(png_ptr, fp, wzpng_write_data, wzpng_flush_data);

		// Set the compression level of ZLIB
		// Right now we stick with the default, since that one is the
		// fastest which still produces acceptable filesizes.
		// The highest compression level hardly produces smaller files than default.
		//
		// Below are some benchmarks done while taking screenshots at 1280x1024
		// Z_NO_COMPRESSION:
		// black (except for GUI): 398 msec
		// 381, 391, 404, 360 msec
		//
		// Z_BEST_SPEED:
		// black (except for GUI): 325 msec
		// 611, 406, 461, 608 msec
		//
		// Z_DEFAULT_COMPRESSION:
		// black (except for GUI): 374 msec
		// 1154, 1121, 627, 790 msec
		//
		// Z_BEST_COMPRESSION:
		// black (except for GUI): 439 msec
		// 1600, 1078, 1613, 1700 msec

		// Not calling this function is equal to using the default
		// so to spare some CPU cycles we comment this out.
		png_set_compression_level(png_ptr, 9);
		png_set_IHDR(png_ptr, info_ptr, w, h, bitdepth,
					 color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		// Create an array of scanlines
		for (currentRow = 0; currentRow < h; ++currentRow)
		{
			// png scanlines are ordered from top-to-bottom
			// so we fill the scanline from the top to the bottom here
			// otherwise we'd have a vertically mirrored image.
			scanlines[currentRow] = pixels + (row_stride * (h - currentRow - 1));
		}

		png_set_rows(png_ptr, info_ptr, (png_bytepp)scanlines);

		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	}

	free(scanlines);
	PNGWriteCleanup(&info_ptr, &png_ptr, fp);

	return true;
}

#if defined(_MSC_VER)
__pragma(warning( pop )) // FIXME?: re-enable MSVC warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#endif

/**************************************************************************
  Save an RGB888 image buffer to a PNG file.
**************************************************************************/
bool savePng(const char *filename, uint8_t *pixels, unsigned w, unsigned h)
{
	return savePngInternal(filename, pixels, w, h, 8, PNG_COLOR_TYPE_RGB);
}

/**************************************************************************
  Save an ARGB8888 image buffer to a PNG file.
**************************************************************************/
bool savePngARGB32(const char *filename, uint8_t *pixels, unsigned w, unsigned h)
{
	return savePngInternal(filename, pixels, w, h, 8, PNG_COLOR_TYPE_RGBA);
}

/**************************************************************************
  Save an I16 image buffer to a PNG file.
**************************************************************************/
bool savePngI16(const char *filename, uint16_t *pixels, unsigned w, unsigned h)
{
	return savePngInternal(filename, (uint8_t *)pixels, w, h, 16, PNG_COLOR_TYPE_GRAY);
}

/**************************************************************************
  Save an I8 image buffer to a PNG file.
**************************************************************************/
bool savePngI8(const char *filename, uint8_t *pixels, int w, int h)
{
	return savePngInternal(filename, pixels, w, h, 8, PNG_COLOR_TYPE_GRAY);
}
