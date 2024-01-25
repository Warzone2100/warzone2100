/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2024  Warzone 2100 Project

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
#include "png_util.h"
#include <spng.h>
#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include <algorithm>

IMGSaveError IMGSaveError::None = IMGSaveError();

// SPNG callbacks
static int wzspng_read_data(spng_ctx *ctx, void *user, void *dest, size_t length)
{
	PHYSFS_file *fileHandle = (PHYSFS_file *)user;
	(void)ctx;

	if (fileHandle == nullptr)
	{
		return SPNG_IO_ERROR;
	}

	PHYSFS_sint64 result = WZ_PHYSFS_readBytes(fileHandle, dest, static_cast<PHYSFS_uint32>(length));
	if (result > -1)
	{
		size_t byteCountRead = static_cast<size_t>(result);
		if (byteCountRead == length)
		{
			return 0; // success
		}
		return SPNG_IO_EOF;
	}

	return SPNG_IO_ERROR;
}
// End of SPNG callbacks

bool iV_loadImage_PNG(const char *fileName, iV_Image *image)
{
	// Open file
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	ASSERT_OR_RETURN(false, fileHandle != nullptr, "Could not open %s: %s", fileName, WZ_PHYSFS_getLastError());
	WZ_PHYSFS_SETBUFFER(fileHandle, 4096)//;

	// Create SPNG context
	spng_ctx *ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);
	if (ctx == NULL) return false;

	// Set various limits for handling untrusted files
	spng_set_image_limits(ctx, WZ_PNG_MAX_IMAGE_DIMENSIONS, WZ_PNG_MAX_IMAGE_DIMENSIONS);
	spng_set_chunk_limits(ctx, WZ_PNG_MAX_CHUNKSIZE_LIMIT, WZ_PNG_MAX_CHUNKSIZE_LIMIT);

	int ret = 0;
	// Set read stream function
	ret = spng_set_png_stream(ctx, wzspng_read_data, fileHandle);
	if (ret) goto err;

	struct spng_ihdr ihdr;
	ret = spng_get_ihdr(ctx, &ihdr);
	if (ret) goto err;

	/* Determine output image size */
	size_t image_size;
	ret = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &image_size);
	if (ret) goto err;

	if (image_size > WZ_PNG_MAX_DECODED_MEMSIZE)
	{
		ret = -1;
		goto err;
	}

	if (!image->allocate(ihdr.width, ihdr.height, 4))
	{
		ret = -1;
		goto err;
	}
	if (image->data_size() != image_size)
	{
		ret = -1;
		goto err;
	}

	/* Decode to 8-bit RGBA */
	ret = spng_decode_image(ctx, image->bmp_w(), image->data_size(), SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);
	if (ret) goto err;

err:
	spng_ctx_free(ctx);

	if (fileHandle != nullptr)
	{
		PHYSFS_close(fileHandle);
	}

	return ret == 0;
}

bool iV_loadImage_PNG2(const char *fileName, iV_Image& image, bool forceRGBA8 /*= false*/, bool quietOnOpenFail /*= false*/)
{
	spng_format fmt = SPNG_FMT_RGBA8;
	int decode_flags = SPNG_DECODE_TRNS;
	unsigned int channels = 4;
	struct spng_ihdr ihdr;
	struct spng_trns trns = {};
	int have_trns = 0;

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

	// Create SPNG context
	spng_ctx *ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);
	if (ctx == NULL) return false;

	// Set various limits for handling untrusted files
	spng_set_image_limits(ctx, WZ_PNG_MAX_IMAGE_DIMENSIONS, WZ_PNG_MAX_IMAGE_DIMENSIONS);
	spng_set_chunk_limits(ctx, WZ_PNG_MAX_CHUNKSIZE_LIMIT, WZ_PNG_MAX_CHUNKSIZE_LIMIT);

	int ret = 0;
	// Set read stream function
	ret = spng_set_png_stream(ctx, wzspng_read_data, fileHandle);
	if (ret) goto err;

	ret = spng_get_ihdr(ctx, &ihdr);
	if (ret) goto err;

	have_trns = !spng_get_trns(ctx, &trns);

	/* Determine output image format */
	switch (ihdr.color_type)
	{
		case SPNG_COLOR_TYPE_GRAYSCALE:
			// WZ doesn't support 16-bit color, and libspng doesn't support converting 16-bit grayscale to anything but SPNG_FMT_G(A)16 or SPNG_FMT_RGB(A)8
			if (!have_trns)
			{
				fmt = (ihdr.bit_depth <= 8) ? SPNG_FMT_G8 : SPNG_FMT_RGB8;
			}
			else
			{
				fmt = (ihdr.bit_depth <= 8) ? SPNG_FMT_GA8 : SPNG_FMT_RGBA8;
			}
			break;
		case SPNG_COLOR_TYPE_TRUECOLOR:
			fmt = (have_trns) ? SPNG_FMT_RGBA8 : SPNG_FMT_RGB8;
			break;
		case SPNG_COLOR_TYPE_INDEXED:
			fmt = (have_trns) ? SPNG_FMT_RGBA8 : SPNG_FMT_RGB8;
			break;
		case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
			// WZ doesn't support 16-bit color, and libspng doesn't support converting 16-bit grayscale-alpha to anything but SPNG_FMT_GA16 or SPNG_FMT_RGB(A)8
			fmt = (ihdr.bit_depth <= 8) ? SPNG_FMT_GA8 : SPNG_FMT_RGBA8;
			break;
		case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
			fmt = SPNG_FMT_RGBA8;
			break;
	}

	if (forceRGBA8)
	{
		/* Must end up with 32bpp, 4 channel RGBA */
		fmt = SPNG_FMT_RGBA8;
	}

	switch (fmt)
	{
		case SPNG_FMT_RGBA8:
			channels = 4;
			break;
		case SPNG_FMT_RGB8:
			channels = 3;
			break;
		case SPNG_FMT_GA8:
			channels = 2;
			break;
		case SPNG_FMT_G8:
			channels = 1;
			break;
		default:
			ret = -1;
			goto err;
	}

	/* Determine output image size */
	size_t image_size;
	ret = spng_decoded_image_size(ctx, fmt, &image_size);
	if (ret) goto err;

	if (image_size > WZ_PNG_MAX_DECODED_MEMSIZE)
	{
		ret = -1;
		goto err;
	}

	if (!image.allocate(ihdr.width, ihdr.height, channels))
	{
		ret = -1;
		goto err;
	}
	if (image.data_size() != image_size)
	{
		ret = -1;
		goto err;
	}

	/* Decode to 8-bit RGBA */
	ret = spng_decode_image(ctx, image.bmp_w(), image.data_size(), fmt, decode_flags);
	if (ret) goto err;

err:
	spng_ctx_free(ctx);

	if (fileHandle != nullptr)
	{
		PHYSFS_close(fileHandle);
	}

	return ret == 0;
}

struct MemoryBufferInputStream
{
public:
	MemoryBufferInputStream(const std::vector<unsigned char> *pMemoryBuffer, size_t currentPos = 0)
	: pMemoryBuffer(pMemoryBuffer)
	, currentPos(currentPos)
	{ }

	size_t readBytes(void *dest, size_t byteCountToRead)
	{
		const size_t remainingBytes = pMemoryBuffer->size() - currentPos;
		size_t bytesToActuallyRead = std::min(byteCountToRead, remainingBytes);
		if (bytesToActuallyRead > 0)
		{
			memcpy(dest, pMemoryBuffer->data() + currentPos, bytesToActuallyRead);
		}
		currentPos += bytesToActuallyRead;
		return bytesToActuallyRead;
	}

private:
	const std::vector<unsigned char> *pMemoryBuffer;
	size_t currentPos = 0;
};

static int wzspng_read_data_from_buffer(spng_ctx *ctx, void *user, void *dest, size_t length)
{
	MemoryBufferInputStream *pMemoryStream = (MemoryBufferInputStream *)user;
	(void)ctx;

	if (pMemoryStream == nullptr)
	{
		return SPNG_IO_ERROR;
	}

	size_t byteCountRead = pMemoryStream->readBytes(dest, length);
	if (byteCountRead == length)
	{
		return 0; // success
	}
	return SPNG_IO_EOF;
}

// Note: This function must be thread-safe.
//       It does not call the debug() macro directly, but instead returns an IMGSaveError structure with the text of any error.
IMGSaveError iV_loadImage_PNG(const std::vector<unsigned char>& memoryBuffer, iV_Image *image)
{
	MemoryBufferInputStream inputStream = MemoryBufferInputStream(&memoryBuffer, 0);

	// Create SPNG context
	spng_ctx *ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);
	if (ctx == NULL)
	{
		return IMGSaveError("iV_loadImage_PNG: Failed to create spng context");
	}

	// Set various limits for handling untrusted files
	spng_set_image_limits(ctx, WZ_PNG_MAX_IMAGE_DIMENSIONS, WZ_PNG_MAX_IMAGE_DIMENSIONS);
	spng_set_chunk_limits(ctx, WZ_PNG_MAX_CHUNKSIZE_LIMIT, WZ_PNG_MAX_CHUNKSIZE_LIMIT);

	int ret = 0;
	// Set read stream function
	ret = spng_set_png_stream(ctx, wzspng_read_data_from_buffer, &inputStream);
	if (ret) goto err;

	struct spng_ihdr ihdr;
	ret = spng_get_ihdr(ctx, &ihdr);
	if (ret) goto err;

	/* Determine output image size */
	size_t image_size;
	ret = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &image_size);
	if (ret) goto err;

	if (image_size > WZ_PNG_MAX_DECODED_MEMSIZE)
	{
		ret = -1;
		goto err;
	}

	if (!image->allocate(ihdr.width, ihdr.height, 4))
	{
		ret = -1;
		goto err;
	}
	if (image->data_size() != image_size)
	{
		ret = -1;
		goto err;
	}

	/* Decode to 8-bit RGBA */
	ret = spng_decode_image(ctx, image->bmp_w(), image->data_size(), SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);
	if (ret) goto err;

err:
	spng_ctx_free(ctx);

	if (ret != 0)
	{
		IMGSaveError error;
		error.text = "Failed to decode image";
		return error;
	}

	return IMGSaveError::None;
}

// Note: This function must be thread-safe.
//       It does not call the debug() macro directly, but instead returns an IMGSaveError structure with the text of any error.
static IMGSaveError internal_saveImage_PNG(const char *fileName, const iV_Image *image, bool bottom_up_write = true)
{
	IMGSaveError errorResult;

	spng_color_type color_type;
	switch (image->pixel_format())
	{
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
			color_type = SPNG_COLOR_TYPE_GRAYSCALE;
			break;
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
			color_type = SPNG_COLOR_TYPE_GRAYSCALE_ALPHA;
			break;
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			color_type = SPNG_COLOR_TYPE_TRUECOLOR;
			break;
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
			break;
		default:
			// not supported
			IMGSaveError error;
			error.text = "internal_saveImage_PNG: Unsupported input image format: ";
			error.text += gfx_api::format_to_str(image->pixel_format());
			return error;
	}

	// Create an encoder context
	spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);

	// Encode to internal buffer managed by libspng
	spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1);

	// Ensure high compression
	spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, 8);
	spng_set_option(ctx, SPNG_IMG_WINDOW_BITS, 15);
	spng_set_option(ctx, SPNG_FILTER_CHOICE, SPNG_FILTER_CHOICE_ALL);

	// Set image properties (which determines the destination image format)
	struct spng_ihdr ihdr = {}; /* zero-initialize to set valid defaults */
	ihdr.width = image->width();
	ihdr.height = image->height();
	ihdr.color_type = color_type;
	ihdr.bit_depth = image->depth() / image->channels();
	spng_set_ihdr(ctx, &ihdr);

	// SPNG_FMT_PNG is a special value that matches the format in ihdr
	int fmt = SPNG_FMT_PNG;

	int ret = spng_encode_image(ctx, nullptr, 0, fmt, SPNG_ENCODE_PROGRESSIVE | SPNG_ENCODE_FINALIZE);
	if (ret)
	{
		errorResult.text = "internal_saveImage_PNG: spng_encode_image error: ";
		const char* pErrStr = spng_strerror(ret);
		if (pErrStr)
		{
			errorResult.text += pErrStr;
		}
		spng_ctx_free(ctx);
		return errorResult;
	}

	// encode scanlines
	unsigned char* bitmap_ptr = const_cast<unsigned char*>(image->bmp());
	unsigned int channelsPerPixel = image->channels();
	unsigned int row_stride = image->width() * channelsPerPixel;
	for (unsigned int currentRow = 0; currentRow < image->height(); ++currentRow)
	{
		unsigned char * pCurrentScanline = nullptr;
		if (bottom_up_write)
		{
			// We're filling the scanline from the bottom up here,
			// otherwise we'd have a vertically mirrored image.
			pCurrentScanline = &bitmap_ptr[row_stride * (image->height() - currentRow - 1)];
		}
		else
		{
			pCurrentScanline = &bitmap_ptr[row_stride * currentRow];
		}

		const int err = spng_encode_scanline(ctx, pCurrentScanline, row_stride);
		if (err == SPNG_EOI)
			break;
		if (err)
		{
			errorResult.text = "internal_saveImage_PNG: spng_encode_scanline(row=" + std::to_string(currentRow) + ") error: ";
			const char* pErrStr = spng_strerror(ret);
			if (pErrStr)
			{
				errorResult.text += pErrStr;
			}
			spng_ctx_free(ctx);
			return errorResult;
		}
	}

	// Get the encoded buffer
	size_t output_size = 0;
	void *png_buf = spng_get_png_buffer(ctx, &output_size, &ret);
	if (!png_buf)
	{
		errorResult.text = "internal_saveImage_PNG: spng_get_png_buffer error: ";
		const char* pErrStr = spng_strerror(ret);
		if (pErrStr)
		{
			errorResult.text += pErrStr;
		}
		spng_ctx_free(ctx);
		return errorResult;
	}

	// Write out the file
	PHYSFS_file *fileHandle = PHYSFS_openWrite(fileName);
	if (fileHandle == nullptr)
	{
		errorResult.text = "internal_saveImage_PNG: PHYSFS_openWrite failed (while opening file ";
		errorResult.text += fileName;
		errorResult.text += ") with error: ";
		errorResult.text += WZ_PHYSFS_getLastError();
		free(png_buf);
		spng_ctx_free(ctx);
		return errorResult;
	}

	WZ_PHYSFS_writeBytes(fileHandle, png_buf, static_cast<PHYSFS_uint32>(output_size));

	if (WZ_PHYSFS_writeBytes(fileHandle, png_buf, static_cast<PHYSFS_uint32>(output_size)) != static_cast<PHYSFS_sint64>(output_size))
	{
		// Failed to write png data to file
		errorResult.text = "internal_saveImage_PNG: could not write data to ";
		errorResult.text += fileName;
		errorResult.text += ") with error: ";
		errorResult.text += WZ_PHYSFS_getLastError();
		// just fall through to ensure proper cleanup and handling
	}

	PHYSFS_close(fileHandle);
	free(png_buf);

	spng_ctx_free(ctx);
	return errorResult;
}

// Note: This function must be thread-safe.
IMGSaveError iV_saveImage_PNG(const char *fileName, const iV_Image *image)
{
	return internal_saveImage_PNG(fileName, image); //PNG_COLOR_TYPE_RGB);
}

// Note: This function must be thread-safe.
IMGSaveError iV_saveImage_PNG_Gray(const char *fileName, const iV_Image *image)
{
	if (image->channels() == 1)
	{
		return internal_saveImage_PNG(fileName, image);
	}
	else
	{
		// must create a copy that's grayscale
		iV_Image grayscaleCopy;
		grayscaleCopy.duplicate(*image);
		if (!grayscaleCopy.convert_to_luma())
		{
			IMGSaveError error;
			error.text = "iV_saveImage_PNG_Gray: Unsupported input image format: ";
			error.text += gfx_api::format_to_str(image->pixel_format());
			return error;
		}
		return internal_saveImage_PNG(fileName, &grayscaleCopy);
	}
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

	auto saveResult = internal_saveImage_PNG(outputFilename.c_str(), &image, false);
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
