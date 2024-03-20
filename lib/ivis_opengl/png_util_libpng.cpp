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
			PHYSFS_sint64 result = WZ_PHYSFS_readBytes(fileHandle, area, static_cast<PHYSFS_uint32>(size));
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

	WZ_PHYSFS_writeBytes(fileHandle, data, static_cast<PHYSFS_uint32>(length));
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
	WZ_PHYSFS_SETBUFFER(fileHandle, 4096)//;

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

	// Set various limits for handling untrusted files
#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, WZ_PNG_MAX_IMAGE_DIMENSIONS, WZ_PNG_MAX_IMAGE_DIMENSIONS);
#endif
#if PNG_LIBPNG_VER >= 10401
	png_set_chunk_malloc_max(png_ptr, static_cast<png_alloc_size_t>(WZ_PNG_MAX_CHUNKSIZE_LIMIT));
#endif

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

	png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
	png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
	png_byte depth = png_get_channels(png_ptr, info_ptr);
	if (png_get_rowbytes(png_ptr, info_ptr) != (width * depth))
	{
		// something is wrong - unexpected row size
		png_error(png_ptr, "unexpected row bytes");
	}
	size_t decoded_image_size = sizeof(unsigned char) * static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth);
	if (decoded_image_size > WZ_PNG_MAX_DECODED_MEMSIZE)
	{
		png_error(png_ptr, "unexpectedly large decoded image size");
	}

	image->allocate(width, height, depth);

	{
		unsigned int i = 0;
		png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
		unsigned char* bmp_write_ptr = image->bmp_w();
		for (i = 0; i < png_get_image_height(png_ptr, info_ptr); i++)
		{
			memcpy(bmp_write_ptr + (png_get_rowbytes(png_ptr, info_ptr) * i), row_pointers[i], png_get_rowbytes(png_ptr, info_ptr));
		}
	}

	PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);

	ASSERT_OR_RETURN(false, image->channels() > 3, "Unsupported image channels (%d) found.  We only support 3 (RGB) or 4 (ARGB)", image->channels());

	return true;
}

bool iV_loadImage_PNG2(const char *fileName, iV_Image& image, bool forceRGBA8 /*= false*/, bool quietOnOpenFail /*= false*/)
{
	unsigned char PNGheader[PNG_BYTES_TO_CHECK];
	PHYSFS_sint64 readSize;

	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	png_uint_32 width = 0;
	png_uint_32 height = 0;
	png_byte color_type = 0;
	png_byte bit_depth = 0;
	png_byte channels = 0;
	png_byte interlace_type = 0;
	size_t row_bytes = 0;

	volatile png_bytepp row_pointers = nullptr;

	// Open file
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	if (fileHandle == nullptr)
	{
		if (!quietOnOpenFail)
		{
			ASSERT_OR_RETURN(false, fileHandle != nullptr, "Could not open %s: %s", fileName, WZ_PHYSFS_getLastError());
		}
		else
		{
			return false;
		}
	}
	WZ_PHYSFS_SETBUFFER(fileHandle, 4096)//;

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

	// Set various limits for handling untrusted files
#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, WZ_PNG_MAX_IMAGE_DIMENSIONS, WZ_PNG_MAX_IMAGE_DIMENSIONS);
#endif
#if PNG_LIBPNG_VER >= 10401
	png_set_chunk_malloc_max(png_ptr, static_cast<png_alloc_size_t>(WZ_PNG_MAX_CHUNKSIZE_LIMIT));
#endif

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
		png_bytepp rows_tmp_cleanup = row_pointers;
		if (rows_tmp_cleanup != nullptr)
		{
			row_pointers = nullptr;
			free(rows_tmp_cleanup);
		}
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	// Tell libpng how many byte we already read
	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	/* Set up the input control */
	png_set_read_fn(png_ptr, fileHandle, wzpng_read_data);

	/* Read the PNG info / header */
	png_read_info(png_ptr, info_ptr);
	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	channels = png_get_channels(png_ptr, info_ptr);
	interlace_type = png_get_interlace_type(png_ptr, info_ptr);

	if (interlace_type != PNG_INTERLACE_NONE)
	{
		png_error(png_ptr, "interlaced PNGs are not currently supported");
		return false;
	}

	// Most of the following transformations are seemingly not needed
	// Filler is, however, for an unknown reason required for tertilesc[23]

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	if (bit_depth == 16)
	{
		png_set_strip_16(png_ptr);
	}

	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(png_ptr);
	}

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(png_ptr);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	{
		png_set_expand_gray_1_2_4_to_8(png_ptr); // libPNG 1.2.9+
	}

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	if (bit_depth < 8)
	{
		png_set_packing(png_ptr);
	}

	if (forceRGBA8)
	{
		/* More transformations to ensure we end up with 32bpp, 4 channel RGBA */
		png_set_gray_to_rgb(png_ptr);

		// Fill alpha with 0xFF (if needed)
		png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	}

	png_read_update_info(png_ptr, info_ptr);

	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	channels = png_get_channels(png_ptr, info_ptr);
	row_bytes = png_get_rowbytes(png_ptr, info_ptr);

	if (bit_depth != 8)
	{
		png_error(png_ptr, "output bit_depth != 8");
		return false;
	}

	/* Guard against integer overflow */
	if (height > (PNG_SIZE_MAX / row_bytes))
	{
		png_error(png_ptr, "image_data buffer would be too large");
		return false;
	}

	// Figure out output image type
	auto destFormat = iV_Image::pixel_format_for_channels(channels);
	if (destFormat == gfx_api::pixel_format::invalid)
	{
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	if ((height * row_bytes) != (channels * height * width))
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Unexpected output size (%zu, expected: %zu): %s", size_t(height * row_bytes), size_t(channels * height * width), fileName);
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	size_t decoded_image_size = sizeof(unsigned char) * static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
	if (decoded_image_size > WZ_PNG_MAX_DECODED_MEMSIZE)
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Unexpectedly large decoded image size (%zu): %s", decoded_image_size, fileName);
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	// Allocate buffer
	if (!image.allocate(width, height, channels))
	{
		debug(LOG_FATAL, "pie_PNGLoadMem: Unable to allocate memory to load: %s", fileName);
		PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);
		return false;
	}

	unsigned char *pData = image.bmp_w();

	// construct row pointers
	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	png_bytepp rows_tmp = row_pointers; // Avoid volatile overhead, copy row_pointers to local - use row_pointers only in setjmp/longjmp handler above
	for (png_uint_32 i = 0; i < height; i++)
	{
		rows_tmp[i] = &(pData[i * row_bytes]);
	}

	// read image rows
	png_read_image(png_ptr, rows_tmp);

	if (rows_tmp != nullptr)
	{
		row_pointers = nullptr;
		free(rows_tmp);
	}
	PNGReadCleanup(&info_ptr, &png_ptr, fileHandle);

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

	// Set various limits for handling untrusted files
#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, WZ_PNG_MAX_IMAGE_DIMENSIONS, WZ_PNG_MAX_IMAGE_DIMENSIONS);
#endif
#if PNG_LIBPNG_VER >= 10401
	png_set_chunk_malloc_max(png_ptr, static_cast<png_alloc_size_t>(WZ_PNG_MAX_CHUNKSIZE_LIMIT));
#endif

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

	png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
	png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
	png_byte depth = png_get_channels(png_ptr, info_ptr);
	if (png_get_rowbytes(png_ptr, info_ptr) != (width * depth))
	{
		// something is wrong - unexpected row size
		png_error(png_ptr, "unexpected row bytes");
	}
	size_t decoded_image_size = sizeof(unsigned char) * static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth);
	if (decoded_image_size > WZ_PNG_MAX_DECODED_MEMSIZE)
	{
		png_error(png_ptr, "unexpectedly large decoded image size");
	}

	image->allocate(width, height, depth);

	{
		unsigned int i = 0;
		png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
		unsigned char* bmp_write_ptr = image->bmp_w();
		for (i = 0; i < png_get_image_height(png_ptr, info_ptr); i++)
		{
			memcpy(bmp_write_ptr + (png_get_rowbytes(png_ptr, info_ptr) * i), row_pointers[i], png_get_rowbytes(png_ptr, info_ptr));
		}
	}

	PNGReadCleanup(&info_ptr, &png_ptr, nullptr);

	if (image->channels() < 3 || image->channels() > 4)
	{
		IMGSaveError error;
		error.text = "Unsupported image channels (";
		error.text += std::to_string(image->channels());
		error.text += ") found.  We only support 3 (RGB) or 4 (ARGB)";
		return error;
	}

	return IMGSaveError::None;
}

// Note: This function must be thread-safe.
//       It does not call the debug() macro directly, but instead returns an IMGSaveError structure with the text of any error.
static IMGSaveError internal_saveImage_PNG(const char *fileName, const iV_Image *image, int color_type, bool bottom_up_write = true)
{
	unsigned char **volatile scanlines = nullptr;  // Must be volatile to reliably preserve value if modified between setjmp/longjmp.
	png_infop info_ptr = nullptr;
	png_structp png_ptr = nullptr;
	PHYSFS_file *fileHandle;

	scanlines = nullptr;
	ASSERT(image->channels() != 0, "Bad channels");

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
		unsigned int channelsPerPixel = image->channels();
		unsigned int currentRow, row_stride;

		row_stride = image->width() * channelsPerPixel;

		scanlines = (unsigned char **)malloc(sizeof(unsigned char *) * image->height());
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

		png_set_IHDR(png_ptr, info_ptr, image->width(), image->height(), 8,
		             color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		// Create an array of scanlines
		unsigned char* bitmap_ptr = const_cast<unsigned char*>(image->bmp());
		for (currentRow = 0; currentRow < image->height(); ++currentRow)
		{
			if (bottom_up_write)
			{
				// We're filling the scanline from the bottom up here,
				// otherwise we'd have a vertically mirrored image.
				scanlines[currentRow] = &bitmap_ptr[row_stride * (image->height() - currentRow - 1)];
			}
			else
			{
				scanlines[currentRow] = &bitmap_ptr[row_stride * currentRow];
			}
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
	int color_type = PNG_COLOR_TYPE_RGB;
	switch (image->channels())
	{
		case 4:
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		case 3:
			color_type = PNG_COLOR_TYPE_RGB;
			break;
	}
	return internal_saveImage_PNG(fileName, image, color_type);
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
	const unsigned int row_stride = image->width() * 3; // 3 bytes per pixel
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

	buffer = (unsigned char *)malloc(sizeof(const char *) * image->height() * image->width()); // Suspect it should be sizeof(unsigned char)*3 == 3 here, not sizeof(const char *) == 8.
	if (buffer == nullptr)
	{
		debug(LOG_ERROR, "pie_JPEGSaveFile: Couldn't allocate memory\n");
		return;
	}

	// Create an array of scanlines
	const unsigned char* bitmap_ptr = image->bmp();
	for (currentRow = 0; currentRow < image->height(); ++currentRow)
	{
		// We're filling the scanline from the bottom up here,
		// otherwise we'd have a vertically mirrored image.
		memcpy(buffer + row_stride * currentRow, &bitmap_ptr[row_stride * (image->height() - currentRow - 1)], row_stride);
	}

	jpeg = (unsigned char *)malloc(sizeof(const char *) * image->height() * image->width()); // Suspect it should be something else here, but sizeof(const char *) == 8 is hopefully big enough...
	if (jpeg == nullptr)
	{
		debug(LOG_ERROR, "pie_JPEGSaveFile: Couldn't allocate memory\n");
		free(buffer);
		return;
	}

	jpeg_end = jpeg_encode_image(buffer, jpeg, 1, JPEG_FORMAT_RGB, image->width(), image->height());
	WZ_PHYSFS_writeBytes(fileHandle, jpeg, jpeg_end - jpeg);

	free(buffer);
	free(jpeg);
	PHYSFS_close(fileHandle);
}

// Note: This function is *NOT* thread-safe.
bool iV_LoadAndSavePNG_AsLumaSingleChannel(const std::string &inputFilename, const std::string &outputFilename, bool check)
{
	iV_Image image;

	// 1.) Load the PNG into an iV_Image
	if (!iV_loadImage_PNG2(inputFilename.c_str(), image, false, false))
	{
		// Failed to load the input image
		return false;
	}

	bool result = image.convert_to_luma();
	ASSERT_OR_RETURN(false, result, "(%s): Failed to convert specular map", inputFilename.c_str());

	auto saveResult = internal_saveImage_PNG(outputFilename.c_str(), &image, PNG_COLOR_TYPE_GRAY, false);
	ASSERT_OR_RETURN(false, saveResult.noError(), "(%s): Failed to save specular map output, with error: %s", outputFilename.c_str(), saveResult.text.c_str());

	if (check)
	{
		// Test round-trip - does loading the written image yield the same iV_Image bitmap data?
		iV_Image outputImage;
		if (!iV_loadImage_PNG2(outputFilename.c_str(), outputImage, false, false))
		{
			// Failed to load the output image
			return false;
		}

		ASSERT_OR_RETURN(false, outputImage.channels() == 1, "Output image unexpectedly has %u channels", outputImage.channels());
		ASSERT_OR_RETURN(false, image.compare_equal(outputImage), "Output image doesn't seem to match expected");
		debug(LOG_INFO, "Validated that output image loads the same bitmap data that was saved.");
	}

	return true;
}
