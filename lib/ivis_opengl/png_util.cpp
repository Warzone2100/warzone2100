/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "lib/framework/debug.h"
#include "jpeg_encoder.h"
#include "png_util.h"
#include <png.h>
#include <physfs.h>

#define PNG_BYTES_TO_CHECK 8

// PNG callbacks
static void wzpng_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
	PHYSFS_file *fileHandle = (PHYSFS_file *)png_get_io_ptr(ctx);

	PHYSFS_read(fileHandle, area, 1, size);
}

static void wzpng_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PHYSFS_file *fileHandle = (PHYSFS_file *)png_get_io_ptr(png_ptr);

	PHYSFS_write(fileHandle, data, length, 1);
}

static void wzpng_flush_data(png_structp png_ptr)
{
	PHYSFS_file *fileHandle = (PHYSFS_file *)png_get_io_ptr(png_ptr);

	PHYSFS_flush(fileHandle);
}
// End of PNG callbacks

static inline void PNGReadCleanup(png_infop *info_ptr, png_structp *png_ptr, PHYSFS_file *fileHandle)
{
	if (*info_ptr != NULL)
	{
		png_destroy_info_struct(*png_ptr, info_ptr);
	}
	if (*png_ptr != NULL)
	{
		png_destroy_read_struct(png_ptr, NULL, NULL);
	}
	if (fileHandle != NULL)
	{
		PHYSFS_close(fileHandle);
	}
}

static inline void PNGWriteCleanup(png_infop *info_ptr, png_structp *png_ptr, PHYSFS_file *fileHandle)
{
	if (*info_ptr != NULL)
	{
		png_destroy_info_struct(*png_ptr, info_ptr);
	}
	if (*png_ptr != NULL)
	{
		png_destroy_write_struct(png_ptr, NULL);
	}
	if (fileHandle != NULL)
	{
		PHYSFS_close(fileHandle);
	}
}

bool iV_loadImage_PNG(const char *fileName, iV_Image *image)
{
	unsigned char PNGheader[PNG_BYTES_TO_CHECK];
	PHYSFS_sint64 readSize;

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	// Open file
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	ASSERT_OR_RETURN(false, fileHandle != NULL, "Could not open %s: %s", fileName, PHYSFS_getLastError());

	// Read PNG header from file
	readSize = PHYSFS_read(fileHandle, PNGheader, 1, PNG_BYTES_TO_CHECK);
	if (readSize < PNG_BYTES_TO_CHECK)
	{
		debug(LOG_FATAL, "pie_PNGLoadFile: PHYSFS_read(%s) failed with error: %s\n", fileName, PHYSFS_getLastError());
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	// Verify the PNG header to be correct
	if (png_sig_cmp(PNGheader, 0, PNG_BYTES_TO_CHECK))
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Did not recognize PNG header in %s", fileName);
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Unable to create png struct");
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Unable to create png info struct");
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	// Set libpng's failure jump position to the if branch,
	// setjmp evaluates to false so the else branch will be executed at first
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Error decoding PNG data in %s", fileName);
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	// Tell libpng how many byte we already read
	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	/* Set up the input control */
	png_set_read_fn(png_ptr, fileHandle, wzpng_read_data);

	// Most of the following transformations are seemingly not needed
	// Filler is, however, for an unknown reason required for tertilesc[23]

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
// 	png_set_packing(png_ptr);

	/* More transformations to ensure we end up with 32bpp, 4 channel RGBA */
	png_set_gray_to_rgb(png_ptr);
	png_set_palette_to_rgb(png_ptr);
	png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
//	png_set_gray_1_2_4_to_8(png_ptr);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	image->width = png_get_image_width(png_ptr, info_ptr);
	image->height = png_get_image_height(png_ptr, info_ptr);
	image->depth = png_get_channels(png_ptr, info_ptr);
	image->bmp = (unsigned char *)malloc(image->height * png_get_rowbytes(png_ptr, info_ptr));

	{
		unsigned int i = 0;
		png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
		for (i = 0; i < png_get_image_height(png_ptr, info_ptr); i++)
		{
			memcpy(image->bmp + (png_get_rowbytes(png_ptr, info_ptr) * i), row_pointers[i], png_get_rowbytes(png_ptr, info_ptr));
		}
	}

	PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);

	ASSERT_OR_RETURN(false, image->depth > 3, "Unsupported image depth (%d) found.  We only support 3 (RGB) or 4 (ARGB)", image->depth);

	return true;
}

static void internal_saveImage_PNG(const char *fileName, const iV_Image *image, int color_type)
{
	static unsigned char **scanlines;
	png_infop info_ptr = NULL;
	png_structp png_ptr = NULL;
	PHYSFS_file *fileHandle;

	scanlines = NULL;
	ASSERT(image->depth != 0, "Bad depth");

	fileHandle = PHYSFS_openWrite(fileName);
	if (fileHandle == NULL)
	{
		debug(LOG_ERROR, "pie_PNGSaveFile: PHYSFS_openWrite failed (while opening file %s) with error: %s\n", fileName, PHYSFS_getLastError());
		return;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		debug(LOG_ERROR, "pie_PNGSaveFile: Unable to create png struct\n");
		PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		debug(LOG_ERROR, "pie_PNGSaveFile: Unable to create png info struct\n");
		PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
		return;
	}

	// If libpng encounters an error, it will jump into this if-branch
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		debug(LOG_ERROR, "pie_PNGSaveFile: Error encoding PNG data\n");
	}
	else
	{
		unsigned int channelsPerPixel = 3;
		unsigned int currentRow, row_stride;

		if (color_type == PNG_COLOR_TYPE_GRAY)
		{
			channelsPerPixel = 1;
		}
		row_stride = image->width * channelsPerPixel * image->depth / 8;

		scanlines = (unsigned char **)malloc(sizeof(unsigned char *) * image->height);
		if (scanlines == NULL)
		{
			debug(LOG_ERROR, "pie_PNGSaveFile: Couldn't allocate memory\n");
			PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
			return;
		}

		png_set_write_fn(png_ptr, fileHandle, wzpng_write_data, wzpng_flush_data);

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
		// png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);

		png_set_IHDR(png_ptr, info_ptr, image->width, image->height, image->depth,
		             color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		// Create an array of scanlines
		for (currentRow = 0; currentRow < image->height; ++currentRow)
		{
			// We're filling the scanline from the bottom up here,
			// otherwise we'd have a vertically mirrored image.
			scanlines[currentRow] = &image->bmp[row_stride * (image->height - currentRow - 1)];
		}

		png_set_rows(png_ptr, info_ptr, (png_bytepp)scanlines);

		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	}

	free(scanlines);
	PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
}

void iV_saveImage_PNG(const char *fileName, const iV_Image *image)
{
	internal_saveImage_PNG(fileName, image, PNG_COLOR_TYPE_RGB);
}

void iV_saveImage_PNG_Gray(const char *fileName, const iV_Image *image)
{
	internal_saveImage_PNG(fileName, image, PNG_COLOR_TYPE_GRAY);
}

void iV_saveImage_JPEG(const char *fileName, const iV_Image *image)
{
	unsigned char *buffer = NULL;
	unsigned char *jpeg = NULL;
	char newfilename[PATH_MAX];
	unsigned int currentRow;
	const unsigned int row_stride = image->width * 3; // 3 bytes per pixel
	PHYSFS_file *fileHandle;
	unsigned char *jpeg_end;

	sstrcpy(newfilename, fileName);
	memcpy(newfilename + strlen(newfilename) - 4, ".jpg", 4);
	fileHandle = PHYSFS_openWrite(newfilename);
	if (fileHandle == NULL)
	{
		debug(LOG_ERROR, "pie_JPEGSaveFile: PHYSFS_openWrite failed (while opening file %s) with error: %s\n", fileName, PHYSFS_getLastError());
		return;
	}

	buffer = (unsigned char *)malloc(sizeof(const char *) * image->height * image->width); // Suspect it should be sizeof(unsigned char)*3 == 3 here, not sizeof(const char *) == 8.
	if (buffer == NULL)
	{
		debug(LOG_ERROR, "pie_JPEGSaveFile: Couldn't allocate memory\n");
		return;
	}

	// Create an array of scanlines
	for (currentRow = 0; currentRow < image->height; ++currentRow)
	{
		// We're filling the scanline from the bottom up here,
		// otherwise we'd have a vertically mirrored image.
		memcpy(buffer + row_stride * currentRow, &image->bmp[row_stride * (image->height - currentRow - 1)], row_stride);
	}

	jpeg = (unsigned char *)malloc(sizeof(const char *) * image->height * image->width); // Suspect it should be something else here, but sizeof(const char *) == 8 is hopefully big enough...
	if (jpeg == NULL)
	{
		debug(LOG_ERROR, "pie_JPEGSaveFile: Couldn't allocate memory\n");
		free(buffer);
		return;
	}

	jpeg_end = jpeg_encode_image(buffer, jpeg, 1, JPEG_FORMAT_RGB, image->width, image->height);
	PHYSFS_write(fileHandle, jpeg, jpeg_end - jpeg, 1);

	free(buffer);
	free(jpeg);
	PHYSFS_close(fileHandle);
}

