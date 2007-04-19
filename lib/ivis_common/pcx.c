/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
#include <png.h>
#include <setjmp.h>
#include <string.h>
#include <physfs.h>

#include "lib/framework/frame.h"
#include "ivisdef.h"

#include "ivispatch.h"

static const size_t PNG_BYTES_TO_CHECK = 4;

static void wzpng_read_data(png_structp ctx, png_bytep area, png_size_t size)
{

	PHYSFS_file* fileHandle = (PHYSFS_file*)png_get_io_ptr(ctx);

	PHYSFS_read(fileHandle, area, 1, size);
}

static inline void PNGCleanup(png_infop *info_ptr, png_structp *png_ptr, PHYSFS_file* fileHandle)
{
	if (*info_ptr != NULL)
		png_destroy_info_struct(*png_ptr, info_ptr);
	if (*png_ptr != NULL)
		png_destroy_read_struct(png_ptr, NULL, NULL);
	if (fileHandle != NULL)
		PHYSFS_close(fileHandle);
}

BOOL pie_PNGLoadFile(const char *fileName, iTexture *s)
{
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	unsigned char PNGheader[PNG_BYTES_TO_CHECK];
	PHYSFS_sint64 readSize;

	// Open file
	PHYSFS_file* fileHandle = PHYSFS_openRead(fileName);
	if (fileHandle == NULL)
	{
		debug(LOG_ERROR, "pie_PNGLoadFile: PHYSFS_openRead(%s) failed with error: %s\n", fileName, PHYSFS_getLastError());
		PNGCleanup(&info_ptr, &png_ptr, fileHandle);
		return FALSE;
	}

	// Read PNG header from file
	readSize = PHYSFS_read(fileHandle, PNGheader, 1, PNG_BYTES_TO_CHECK);
	if (readSize < PNG_BYTES_TO_CHECK)
	{
		debug(LOG_ERROR, "pie_PNGLoadFile: PHYSFS_read(%s) failed with error: %s\n", fileName, PHYSFS_getLastError());
		PNGCleanup(&info_ptr, &png_ptr, fileHandle);
		return FALSE;
	}

	// Verify the PNG header to be correct
	if (png_sig_cmp(PNGheader, 0, PNG_BYTES_TO_CHECK)) {
		debug(LOG_3D, "pie_PNGLoadMem: Did not recognize PNG header in %s", fileName);
		PNGCleanup(&info_ptr, &png_ptr, fileHandle);
		return FALSE;
	}

	// Seek back to start of file, libpng needs this
	PHYSFS_seek(fileHandle, 0);


	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		debug(LOG_3D, "pie_PNGLoadMem: Unable to create png struct");
		PNGCleanup(&info_ptr, &png_ptr, fileHandle);
		return FALSE;
	}

	info_ptr = png_create_info_struct(png_ptr);

	if (info_ptr == NULL) {
		debug(LOG_3D, "pie_PNGLoadMem: Unable to create png info struct");
		PNGCleanup(&info_ptr, &png_ptr, fileHandle);
		return FALSE;
	}

	// Set libpng's failure jump position to the if branch,
	// setjmp evaluates to false so the else branch will be executed at first
	if (setjmp(png_jmpbuf(png_ptr))) {
		debug(LOG_3D, "pie_PNGLoadMem: Error decoding PNG data in %s", fileName);
		PNGCleanup(&info_ptr, &png_ptr, fileHandle);
		return FALSE;
	} else {
		int bit_depth, color_type, interlace_type;
		png_uint_32 width, height;

		/* Set up the input control */
		png_set_read_fn(png_ptr, fileHandle, wzpng_read_data);

		/* Read PNG header info */
		png_read_info(png_ptr, info_ptr);
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
			     &color_type, &interlace_type, NULL, NULL);

		/* tell libpng to strip 16 bit/color files down to 8 bits/color */
		png_set_strip_16(png_ptr) ;

		/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
		 * byte into separate bytes (useful for paletted and grayscale images).
		 */
		png_set_packing(png_ptr);

		/* More transformations to ensure we end up with 32bpp, 4 channel RGBA */
		png_set_gray_to_rgb(png_ptr);
		png_set_palette_to_rgb(png_ptr);
		png_set_tRNS_to_alpha(png_ptr);
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
		png_set_gray_1_2_4_to_8(png_ptr);

		/* scale greyscale values to the range 0..255 */
		if(color_type == PNG_COLOR_TYPE_GRAY)
			png_set_expand(png_ptr);

		png_read_update_info(png_ptr, info_ptr);

		{
			png_uint_32 w, h;

			png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)(&w),
				     (png_uint_32*)(&h), &bit_depth,
				     &color_type, &interlace_type, NULL, NULL);

			s->width = w;
			s->height = h;
			// Freeing s->bmp before allocating new mem would give a HEAP error on Windows (Invalid Address specified to RtlFreeHeap( x, x )).
			s->bmp = (iBitmap*)malloc(w*h*info_ptr->channels);
		}

		{
			png_bytep* row_pointers = (png_bytep*)malloc(s->height*sizeof(png_bytep));
			char* pdata;
			int i;
			const unsigned int line_size = s->width*info_ptr->channels;

			for (i = 0, pdata = s->bmp;
			     i < s->height;
			     ++i, pdata += line_size) {
				row_pointers[i] = (png_bytep)pdata;
			}

			/* Read the entire image in one go */
			png_read_image(png_ptr, row_pointers);

			free(row_pointers);

			/* read rest of file, get additional chunks in info_ptr - REQUIRED */
			png_read_end(png_ptr, info_ptr);
		}
	}

	PNGCleanup(&info_ptr, &png_ptr, fileHandle);
	return TRUE;
}
