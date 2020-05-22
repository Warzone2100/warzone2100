/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/framework/physfs_ext.h"
#include <algorithm>

#define PNG_BYTES_TO_CHECK 8

IMGSaveError IMGSaveError::None = IMGSaveError();

// PNG callbacks
static void wzpng_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
	if (ctx != nullptr)
	{
		PHYSFS_file *fileHandle = (PHYSFS_file *)png_get_io_ptr(ctx);
		if (fileHandle != nullptr)
		{
			PHYSFS_sint64 result = WZ_PHYSFS_readBytes(fileHandle, area, size);
			if (result > -1)
			{
				size_t byteCountRead = static_cast<size_t>(result);
				if (byteCountRead == size)
				{
					return;
				}
				png_error(ctx, "Attempt to read beyond end of data");
			}
			png_error(ctx, "readBytes failure");
		}
		png_error(ctx, "Invalid memory read");
	}
}

static void wzpng_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PHYSFS_file *fileHandle = (PHYSFS_file *)png_get_io_ptr(png_ptr);

	WZ_PHYSFS_writeBytes(fileHandle, data, length);
}

static void wzpng_flush_data(png_structp png_ptr)
{
	PHYSFS_file *fileHandle = (PHYSFS_file *)png_get_io_ptr(png_ptr);

	PHYSFS_flush(fileHandle);
}
// End of PNG callbacks

static inline void PNGReadCleanup(png_infop *info_ptr, png_structp *png_ptr, PHYSFS_file *fileHandle)
{
	if (*info_ptr != nullptr)
	{
		png_destroy_info_struct(*png_ptr, info_ptr);
	}
	if (*png_ptr != nullptr)
	{
		png_destroy_read_struct(png_ptr, nullptr, nullptr);
	}
	if (fileHandle != nullptr)
	{
		PHYSFS_close(fileHandle);
	}
}

static inline void PNGWriteCleanup(png_infop *info_ptr, png_structp *png_ptr, PHYSFS_file *fileHandle)
{
	if (*info_ptr != nullptr)
	{
		png_destroy_info_struct(*png_ptr, info_ptr);
	}
	if (*png_ptr != nullptr)
	{
		png_destroy_write_struct(png_ptr, nullptr);
	}
	if (fileHandle != nullptr)
	{
		PHYSFS_close(fileHandle);
	}
}

// FIXME?: disable MSVC warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
MSVC_PRAGMA(warning( push )) // see matching "pop" below
MSVC_PRAGMA(warning( disable : 4611 ))

bool iV_loadImage_PNG(const char *fileName, iV_Image *image)
{
	unsigned char PNGheader[PNG_BYTES_TO_CHECK];
	PHYSFS_sint64 readSize;

	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	// Open file
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	ASSERT_OR_RETURN(false, fileHandle != nullptr, "Could not open %s: %s", fileName, WZ_PHYSFS_getLastError());

	// Read PNG header from file
	readSize = WZ_PHYSFS_readBytes(fileHandle, PNGheader, PNG_BYTES_TO_CHECK);
	if (readSize < PNG_BYTES_TO_CHECK)
	{
		debug(LOG_FATAL, "pie_PNGLoadFile: WZ_WZ_PHYSFS_readBytes(%s) failed with error: %s\n", fileName, WZ_PHYSFS_getLastError());
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

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr)
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Unable to create png struct");
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr)
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

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

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

struct MemoryBufferInputStream
{
public:
	MemoryBufferInputStream(const std::vector<unsigned char> *pMemoryBuffer, size_t currentPos = 0)
	: pMemoryBuffer(pMemoryBuffer)
	, currentPos(currentPos)
	{ }

	size_t readBytes(png_bytep destBytes, size_t byteCountToRead)
	{
		const size_t remainingBytes = pMemoryBuffer->size() - currentPos;
		size_t bytesToActuallyRead = std::min(byteCountToRead, remainingBytes);
		if (bytesToActuallyRead > 0)
		{
			memcpy(destBytes, pMemoryBuffer->data() + currentPos, bytesToActuallyRead);
		}
		currentPos += bytesToActuallyRead;
		return bytesToActuallyRead;
	}

private:
	const std::vector<unsigned char> *pMemoryBuffer;
	size_t currentPos = 0;
};

static void wzpng_read_data_from_buffer(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
{
	if (png_ptr != nullptr)
	{
		MemoryBufferInputStream *pMemoryStream = (MemoryBufferInputStream *)png_get_io_ptr(png_ptr);
		if (pMemoryStream != nullptr)
		{
			size_t byteCountRead = pMemoryStream->readBytes(outBytes, byteCountToRead);
			if (byteCountRead == byteCountToRead)
			{
				return;
			}
			png_error(png_ptr, "Attempt to read beyond end of data");
		}
		png_error(png_ptr, "Invalid memory read");
	}
}

// Note: This function must be thread-safe.
//       It does not call the debug() macro directly, but instead returns an IMGSaveError structure with the text of any error.
IMGSaveError iV_loadImage_PNG(const std::vector<unsigned char>& memoryBuffer, iV_Image *image)
{
	unsigned char PNGheader[PNG_BYTES_TO_CHECK];
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	MemoryBufferInputStream inputStream = MemoryBufferInputStream(&memoryBuffer, 0);

	size_t readSize = inputStream.readBytes(PNGheader, PNG_BYTES_TO_CHECK);
	if (readSize < PNG_BYTES_TO_CHECK)
	{
		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
		return IMGSaveError("iV_loadImage_PNG: Insufficient length for PNG header");
	}

	// Verify the PNG header to be correct
	if (png_sig_cmp(PNGheader, 0, PNG_BYTES_TO_CHECK))
	{
		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
		return IMGSaveError("iV_loadImage_PNG: Did not recognize PNG header");
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr)
	{
		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
		return IMGSaveError("iV_loadImage_PNG: Unable to create png struct");
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr)
	{
		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
		return IMGSaveError("iV_loadImage_PNG: Unable to create png info struct");
	}

	// Set libpng's failure jump position to the if branch,
	// setjmp evaluates to false so the else branch will be executed at first
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		PNGReadCleanup(&info_ptr, &png_ptr, nullptr);
		return IMGSaveError("iV_loadImage_PNG: Error decoding PNG data");
	}

	// Tell libpng how many byte we already read
	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	/* Set up the input control */
	png_set_read_fn(png_ptr, &inputStream, wzpng_read_data_from_buffer);

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

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

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

	PNGReadCleanup(&info_ptr, &png_ptr, nullptr);

	if (image->depth < 3 || image->depth > 4)
	{
		IMGSaveError error;
		error.text = "Unsupported image depth (";
		error.text += std::to_string(image->depth);
		error.text += ") found.  We only support 3 (RGB) or 4 (ARGB)";
		return error;
	}

	return IMGSaveError::None;
}

// Note: This function must be thread-safe.
//       It does not call the debug() macro directly, but instead returns an IMGSaveError structure with the text of any error.
static IMGSaveError internal_saveImage_PNG(const char *fileName, const iV_Image *image, int color_type)
{
	unsigned char **volatile scanlines = nullptr;  // Must be volatile to reliably preserve value if modified between setjmp/longjmp.
	png_infop info_ptr = nullptr;
	png_structp png_ptr = nullptr;
	PHYSFS_file *fileHandle;

	scanlines = nullptr;
	ASSERT(image->depth != 0, "Bad depth");

	fileHandle = PHYSFS_openWrite(fileName);
	if (fileHandle == nullptr)
	{
		IMGSaveError error;
		error.text = "pie_PNGSaveFile: PHYSFS_openWrite failed (while opening file ";
		error.text += fileName;
		error.text += ") with error: ";
		error.text += WZ_PHYSFS_getLastError();
		return error;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr)
	{
		PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
		return IMGSaveError("pie_PNGSaveFile: Unable to create png struct");
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr)
	{
		PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
		return IMGSaveError("pie_PNGSaveFile: Unable to create png info struct");
	}

	// If libpng encounters an error, it will jump into this if-branch
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		free(scanlines);
		PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
		return IMGSaveError("pie_PNGSaveFile: Error encoding PNG data");
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
		if (scanlines == nullptr)
		{
			PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
			return IMGSaveError("pie_PNGSaveFile: Couldn't allocate memory");
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

		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
	}

	free(scanlines);
	PNGWriteCleanup(&info_ptr, &png_ptr, fileHandle);
	return IMGSaveError::None;
}

MSVC_PRAGMA(warning( pop )) // FIXME?: re-enable MSVC warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable

// Note: This function must be thread-safe.
IMGSaveError iV_saveImage_PNG(const char *fileName, const iV_Image *image)
{
	return internal_saveImage_PNG(fileName, image, PNG_COLOR_TYPE_RGB);
}

// Note: This function must be thread-safe.
IMGSaveError iV_saveImage_PNG_Gray(const char *fileName, const iV_Image *image)
{
	return internal_saveImage_PNG(fileName, image, PNG_COLOR_TYPE_GRAY);
}

// Note: This function is *NOT* thread-safe (jpeg_encode_image is not thread-safe).
void iV_saveImage_JPEG(const char *fileName, const iV_Image *image)
{
	unsigned char *buffer = nullptr;
	unsigned char *jpeg = nullptr;
	char newfilename[PATH_MAX];
	unsigned int currentRow;
	const unsigned int row_stride = image->width * 3; // 3 bytes per pixel
	PHYSFS_file *fileHandle;
	unsigned char *jpeg_end;

	sstrcpy(newfilename, fileName);
	memcpy(newfilename + strlen(newfilename) - 4, ".jpg", 4);
	fileHandle = PHYSFS_openWrite(newfilename);
	if (fileHandle == nullptr)
	{
		debug(LOG_ERROR, "pie_JPEGSaveFile: PHYSFS_openWrite failed (while opening file %s) with error: %s\n", fileName, WZ_PHYSFS_getLastError());
		return;
	}

	buffer = (unsigned char *)malloc(sizeof(const char *) * image->height * image->width); // Suspect it should be sizeof(unsigned char)*3 == 3 here, not sizeof(const char *) == 8.
	if (buffer == nullptr)
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
	if (jpeg == nullptr)
	{
		debug(LOG_ERROR, "pie_JPEGSaveFile: Couldn't allocate memory\n");
		free(buffer);
		return;
	}

	jpeg_end = jpeg_encode_image(buffer, jpeg, 1, JPEG_FORMAT_RGB, image->width, image->height);
	WZ_PHYSFS_writeBytes(fileHandle, jpeg, jpeg_end - jpeg);

	free(buffer);
	free(jpeg);
	PHYSFS_close(fileHandle);
}

