/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
/***************************************************************************/
/*
 * pieimage.cpp
 *
 */
/***************************************************************************/

#include "gfx_api.h"

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-align"
#endif
#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#include "3rdparty/stb_image_resize.h"

#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#include <algorithm>

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

iV_Image::~iV_Image()
{
	clear();
}

iV_Image::iV_Image(iV_Image&& other)
: m_width(0)
, m_height(0)
, m_channels(0)
, m_bmp(nullptr)
, m_colorOrder(iV_Image::ColorOrder::RGB)
{
	std::swap(m_width, other.m_width);
	std::swap(m_height, other.m_height);
	std::swap(m_channels, other.m_channels);
	std::swap(m_bmp, other.m_bmp);
	std::swap(m_colorOrder, other.m_colorOrder);
}

iV_Image& iV_Image::operator=(iV_Image&& other)
{
	if(&other != this)
	{
		clear();
		std::swap(m_width, other.m_width);
		std::swap(m_height, other.m_height);
		std::swap(m_channels, other.m_channels);
		std::swap(m_bmp, other.m_bmp);
		std::swap(m_colorOrder, other.m_colorOrder);
	}
	return *this;
}

// Allocate a new iV_Image buffer
bool iV_Image::allocate(unsigned int newWidth, unsigned int newHeight, unsigned int newChannels, bool zeroMemory)
{
	if (m_bmp)
	{
		free(m_bmp);
		m_bmp = nullptr;
	}
	if (newWidth > 0 && newHeight > 0 && newChannels > 0)
	{
		size_t sizeOfBuffer = sizeof(unsigned char) * newWidth * newHeight * newChannels;
		m_bmp = (unsigned char*)malloc(sizeOfBuffer);
		if (!m_bmp)
		{
			ASSERT(false, "Failed to allocate memory buffer of size: %zu", sizeOfBuffer);
			return false;
		}
		if (zeroMemory)
		{
			memset(m_bmp, 0, sizeOfBuffer);
		}
	}
	m_width = newWidth;
	m_height = newHeight;
	m_channels = newChannels;
	return true;
}

bool iV_Image::duplicate(const iV_Image& other)
{
	if (!allocate(other.width(), other.height(), other.channels(), false))
	{
		return false;
	}
	memcpy(bmp_w(), other.bmp(), other.size_in_bytes());
	return true;
}

void iV_Image::clear()
{
	allocate(0,0,0);
}

// Get a pointer to the bitmap data that can be read
const unsigned char* iV_Image::bmp() const
{
	return m_bmp;
}

// Get a pointer to the bitmap data that can be written to
unsigned char* iV_Image::bmp_w()
{
	return m_bmp;
}

gfx_api::pixel_format iV_Image::pixel_format_for_channels(unsigned int channels)
{
	switch (channels)
	{
	case 1:
		return gfx_api::pixel_format::FORMAT_R8_UNORM;
	case 2:
		return gfx_api::pixel_format::FORMAT_RG8_UNORM;
	case 3:
		return gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8;
	case 4:
		return gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8;
	default:
		debug(LOG_FATAL, "iV_getPixelFormat: Unsupported image channels: %u", channels);
		return gfx_api::pixel_format::invalid;
	}
}

gfx_api::pixel_format iV_Image::pixel_format() const
{
	if (m_colorOrder == ColorOrder::BGR)
	{
		switch (m_channels)
		{
		case 4:
			return gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8;
		default:
			ASSERT(false, "BGR color order, but unexpected number of channels: %u", m_channels);
			// fall-through
		}
	}
	return iV_Image::pixel_format_for_channels(m_channels);
}

template <size_t readChannels, size_t writeChannels, int writeAlphaChannel = -1, unsigned char defaultAlphaValue = 255>
static void internal_convert_image_contents(unsigned int width, unsigned int height, const unsigned char *pSrc, unsigned char *pDst)
{
	static_assert(readChannels <= 4, "Only supports up to 4 channels (read)");
	static_assert(writeChannels <= 4, "Only supports up to 4 channels (write)");

	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			uint8_t rgba[4] = {0,0,0,defaultAlphaValue};
			const unsigned char *pSrcPixel = &pSrc[((y * width) + x) * readChannels];
			for (size_t i = 0; i < readChannels; i++)
			{
				rgba[i] = pSrcPixel[i];
			}

			unsigned char *pDstPixel = &pDst[((y * width) + x) * writeChannels];
			for (size_t i = 0; i < writeChannels; i++)
			{
				pDstPixel[i] = rgba[i];
			}
			if (writeAlphaChannel >= 0)
			{
				pDstPixel[writeAlphaChannel] = rgba[3];
			}
		}
	}
}

bool iV_Image::expand_channels_towards_rgba()
{
	ASSERT_OR_RETURN(false, bmp() != nullptr, "Not initialized");
	ASSERT_OR_RETURN(false, m_width > 0 && m_height > 0, "No size");
	if (m_channels >= 4)
	{
		return true;
	}
	auto originalBmpData = m_bmp; // TODO: Make this bmp()?
	auto originalChannels = m_channels;
	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	m_channels = originalChannels + 1;
	m_bmp = (unsigned char *)malloc(numPixels * m_channels);
	if (!m_bmp)
	{
		m_bmp = originalBmpData;
		m_channels = originalChannels;
		return false;
	}
	switch (originalChannels)
	{
		case 1:
			internal_convert_image_contents<1, 2, -1>(m_width, m_height, originalBmpData, m_bmp);
			break;
		case 2:
			internal_convert_image_contents<2, 3, -1>(m_width, m_height, originalBmpData, m_bmp);
			break;
		case 3:
			internal_convert_image_contents<3, 4, -1>(m_width, m_height, originalBmpData, m_bmp);
			break;
		default:
			ASSERT(false, "Invalid depth: %u", m_channels);
			free(m_bmp);
			m_bmp = originalBmpData;
			m_channels = originalChannels;
			return false;
	}
	// free the original bitmap data
	free(originalBmpData);
	return true;
}

bool iV_Image::convert_to_rgba()
{
	ASSERT_OR_RETURN(false, m_bmp != nullptr, "Not initialized");
	ASSERT_OR_RETURN(false, m_width > 0 && m_height > 0, "No size");
	if (m_channels == 4)
	{
		return true;
	}
	auto originalBmpData = m_bmp;
	auto originalChannels = m_channels;
	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	m_channels = 4;
	m_bmp = (unsigned char *)malloc(numPixels * m_channels);
	switch (originalChannels)
	{
		case 1:
			internal_convert_image_contents<1, 4, -1>(m_width, m_height, originalBmpData, m_bmp);
			break;
		case 2:
			internal_convert_image_contents<2, 4, -1>(m_width, m_height, originalBmpData, m_bmp);
			break;
		case 3:
			internal_convert_image_contents<3, 4, -1>(m_width, m_height, originalBmpData, m_bmp);
			break;
		default:
			ASSERT(false, "Invalid depth: %u", m_channels);
			free(m_bmp);
			m_bmp = originalBmpData;
			m_channels = originalChannels;
			return false;
	}
	// free the original bitmap data
	free(originalBmpData);
	return true;
}

bool iV_Image::convert_rg_to_ra_rgba()
{
	ASSERT_OR_RETURN(false, m_bmp != nullptr, "Not initialized");
	ASSERT_OR_RETURN(false, m_width > 0 && m_height > 0, "No size");
	ASSERT_OR_RETURN(false, m_channels >= 2, "Must have at least 2 channels");
	ASSERT_OR_RETURN(false, convert_to_rgba(), "Format must be RGBA");

	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	for (size_t pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
	{
		const size_t pixelStartPos = (pixelIdx * m_channels);
		m_bmp[pixelStartPos + 3] = m_bmp[pixelStartPos + 1];
		m_bmp[pixelStartPos + 2] = 0;
		m_bmp[pixelStartPos + 1] = 0;
	}
	return true;
}

bool iV_Image::convert_to_luma()
{
	if (m_channels == 1) { return true; }
	// Otherwise, expecting 3 or 4-channel (RGB/RGBA) (but handle 2 channels by taking only the first channel)
	ASSERT_OR_RETURN(false, m_channels <= 4, "Does not have <= 4 channels");
	auto originalBmpData = m_bmp;
	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	m_bmp = (unsigned char *)malloc(numPixels);
	if (m_channels >= 3)
	{
		for (size_t pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
		{
			uint32_t red = originalBmpData[(pixelIdx * m_channels)];
			uint32_t green = originalBmpData[(pixelIdx * m_channels) + 1];
			uint32_t blue = originalBmpData[(pixelIdx * m_channels) + 2];
			if (red == green && red == blue)
			{
				// all channels are the same - just use the first channel
				m_bmp[pixelIdx] = static_cast<unsigned char>(red);
			}
			else
			{
				// quick approximation of a weighted RGB -> Luma method
				// (R+R+B+G+G+G)/6
				uint32_t lum = (red+red+blue+green+green+green) / 6;
				m_bmp[pixelIdx] = static_cast<unsigned char>(std::min<uint32_t>(lum, 255));
			}
		}
	}
	else
	{
		ASSERT(m_channels == 2, "Expecting 2 channels??");
		// For two channels, just take the first, ignore the second
		internal_convert_image_contents<2, 1, -1>(m_width, m_height, originalBmpData, m_bmp);
	}
	m_channels = 1;
	// free the original bitmap data
	free(originalBmpData);
	return true;
}

bool iV_Image::resizeInternal(const iV_Image& source, int output_w, int output_h, optional<int> alphaChannelOverride /*= nullopt*/)
{
	stbir_filter filter = STBIR_FILTER_DEFAULT;
	if (output_w < source.m_width && output_h < source.m_height)
	{
		filter = STBIR_FILTER_MITCHELL;
	}

	int alphaChannel = source.m_channels == 4 ? 3 : STBIR_ALPHA_CHANNEL_NONE;
	int flags = STBIR_FLAG_ALPHA_PREMULTIPLIED;
	if (alphaChannelOverride.has_value())
	{
		alphaChannel = alphaChannelOverride.value();
	}

	unsigned char *output_pixels = (unsigned char *)malloc(static_cast<size_t>(output_w) * static_cast<size_t>(output_h) * source.m_channels);
	if (!output_pixels)
	{
		return false;
	}
	stbir_resize_uint8_generic(source.m_bmp, source.m_width, source.m_height, 0,
							   output_pixels, output_w, output_h, 0,
							   source.m_channels, alphaChannel, flags,
							   STBIR_EDGE_CLAMP,
							   filter,
							   STBIR_COLORSPACE_LINEAR,
							   nullptr);

	if (m_bmp)
	{
		free(m_bmp);
	}
	m_width = output_w;
	m_height = output_h;
	m_bmp = output_pixels;
	m_channels = source.m_channels;
	m_colorOrder = source.m_colorOrder;

	return true;
}

bool iV_Image::resize(int output_w, int output_h, optional<int> alphaChannelOverride /*= nullopt*/)
{
	if (output_w == m_width && output_h == m_height)
	{
		return true;
	}

	return resizeInternal(*this, output_w, output_h, alphaChannelOverride);
}

bool iV_Image::resizedFromOther(const iV_Image& other, int output_w, int output_h, optional<int> alphaChannelOverride /*= nullopt*/)
{
	ASSERT_OR_RETURN(false, &other != this, "Other is this");
	return resizeInternal(other, output_w, output_h, alphaChannelOverride);
}

// If resizing succeeds (or is not required), returns true. Returns false if resizing failed (memory allocation failure).
bool iV_Image::scale_image_max_size(int maxWidth, int maxHeight)
{
	if ((maxWidth <= 0 || m_width <= maxWidth) && (maxHeight <= 0 || m_height <= maxHeight))
	{
		return true;
	}

	double scalingRatio;
	double widthRatio = (double)maxWidth / (double)m_width;
	double heightRatio = (double)maxHeight / (double)m_height;
	if (maxWidth > 0 && maxHeight > 0)
	{
		scalingRatio = std::min<double>(widthRatio, heightRatio);
	}
	else
	{
		scalingRatio = (maxWidth > 0) ? widthRatio : heightRatio;
	}

	int output_w = static_cast<int>(m_width * scalingRatio);
	int output_h = static_cast<int>(m_height * scalingRatio);

	return resize(output_w, output_h);
}

bool iV_Image::convert_channels(const std::vector<unsigned int>& channelMap)
{
	ASSERT_OR_RETURN(false, !channelMap.empty(), "No channels supplied to extract?");
	ASSERT_OR_RETURN(false, channelMap.size() <= 4, "iV_Image does not support > 4 channel textures (channelMap has %zu entries)", channelMap.size());
	auto originalChannels = m_channels;
	ASSERT_OR_RETURN(false, std::all_of(channelMap.begin(), channelMap.end(), [originalChannels](unsigned int srcChannel){ return srcChannel < originalChannels; }), "Channel swizzle contains channel > originalChannels (%u)", m_channels);
	ASSERT_OR_RETURN(false, m_width > 0 && m_height > 0, "No size");

	unsigned int newChannels = static_cast<unsigned int>(channelMap.size());

	auto originalBmpData = m_bmp;
	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	m_channels = newChannels;
	m_bmp = (unsigned char *)malloc(static_cast<size_t>(m_height) * static_cast<size_t>(m_width) * static_cast<size_t>(newChannels));
	for (size_t pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
	{
		for (unsigned int i = 0; i < newChannels; ++i)
		{
			m_bmp[(pixelIdx * m_channels) + i] = originalBmpData[(pixelIdx * originalChannels) + channelMap[i]];
		}
	}
	// free the original bitmap data
	free(originalBmpData);
	return true;
}

bool iV_Image::convert_to_single_channel(unsigned int channel /*= 0*/)
{
	ASSERT_OR_RETURN(false, channel < m_channels, "Cannot extract channel %u from image with %u channels", channel, m_channels);
	ASSERT_OR_RETURN(false, m_width > 0 && m_height > 0, "No size");
	if (channel == 0 && m_channels == 1)
	{
		// nothing to do
		return true;
	}

	auto originalBmpData = m_bmp;
	auto originalChannels = m_channels;
	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	// copy just the desired channel over
	m_channels = 1;
	m_bmp = (unsigned char *)malloc(numPixels);
	for (size_t pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
	{
		m_bmp[pixelIdx] = originalBmpData[(pixelIdx * originalChannels) + channel];
	}
	// free the original bitmap data
	free(originalBmpData);

	return true;
}

bool iV_Image::pad_image(unsigned int newWidth, unsigned int newHeight, bool useSmearing)
{
	if (newWidth == m_width && newHeight == m_height)
	{
		// no padding needed
		return true;
	}
	if (newWidth < m_width || newHeight < m_height)
	{
		return false;
	}

	auto originalBmpData = m_bmp;
	auto originalWidth = m_width;
	auto originalHeight = m_height;
	const size_t sizeOfBuffer = sizeof(unsigned char) * newWidth * newHeight * m_channels;
	m_height = newHeight;
	m_width = newWidth;
	m_bmp = (unsigned char *)malloc(sizeOfBuffer);
	if (originalWidth > 0 && originalHeight > 0)
	{
		for (unsigned int y = 0; y < originalHeight; ++y)
		{
			// Copy the original image row
			memcpy(&m_bmp[newWidth * y * m_channels], &originalBmpData[originalWidth * y * m_channels], (m_channels * originalWidth));
			if (useSmearing)
			{
				// Smear extra in x direction
				for (unsigned int x = originalWidth; x < newWidth; ++x)
				{
					memcpy(&m_bmp[(newWidth * y + x) * m_channels], &m_bmp[(newWidth * y + x - 1) * m_channels], m_channels);
				}
			}
		}
		if (useSmearing)
		{
			// Smear extra in y direction
			for (unsigned int y = originalHeight; y < newHeight; ++y)
			{
				// Copy the entire prior row (above) in the y direction
				memcpy(&m_bmp[newWidth * y * m_channels], &m_bmp[(newWidth * (y - 1)) * m_channels], (m_channels * newWidth));
			}
		}
	}
	// free the original bitmap data
	free(originalBmpData);

	return true;
}

bool iV_Image::blit_image(const iV_Image& src, unsigned int xOffset, unsigned int yOffset)
{
	ASSERT_OR_RETURN(false, xOffset < m_width, "xOffset (%u) >= destination width (%u)", xOffset, m_width);
	ASSERT_OR_RETURN(false, yOffset < m_height, "yOffset (%u) >= destination height (%u)", yOffset, m_height);

	// For now, the source and destination images must share the same format / # of channels
	ASSERT_OR_RETURN(false, src.channels() == channels() && src.pixel_format() == pixel_format(), "Source and destination must share the same number of channels / format");

	auto sourceImageHeight = src.height();
	auto sourceImageWidth = src.width();

	unsigned int rowWidthToCopy = sourceImageWidth;
	if (xOffset + sourceImageWidth > m_width)
	{
		debug(LOG_ERROR, "Cropping source image, as it would extend beyond the width of the destination image.");
		rowWidthToCopy = m_width - xOffset;
	}
	unsigned int rowsToCopy = sourceImageHeight;
	if (yOffset + sourceImageHeight > m_height)
	{
		debug(LOG_ERROR, "Cropping source image, as it would extend beyond the height of the destination image.");
		rowsToCopy = m_height - yOffset;
	}

	for (unsigned int y = 0; y < rowsToCopy; ++y)
	{
		// Copy the source image row
		size_t copyOffset = (((m_width * (yOffset + y)) + xOffset) * 4);
		memcpy(&m_bmp[copyOffset], &src.m_bmp[sourceImageWidth * y * m_channels], (m_channels * rowWidthToCopy));
	}

	return true;
}

bool iV_Image::convert_color_order(ColorOrder newOrder)
{
	if (m_colorOrder == newOrder)
	{
		return true;
	}
	ASSERT_OR_RETURN(false, m_channels == 4, "Does not have 4 channels - currently only supported for 4 channels");
	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	for (size_t pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
	{
		std::swap(m_bmp[(pixelIdx * m_channels)], m_bmp[(pixelIdx * m_channels) + 2]);
	}
	m_colorOrder = newOrder;
	return true;
}

bool iV_Image::compare_equal(const iV_Image& other)
{
	if (m_width != other.m_width || m_height != other.m_height || m_channels != other.m_channels)
	{
		return false;
	}
	if (m_colorOrder != other.m_colorOrder)
	{
		return false;
	}
	if ((m_bmp == nullptr || other.m_bmp == nullptr) && (m_bmp != other.m_bmp))
	{
		return false;
	}

	const size_t sizeOfBuffers = sizeof(unsigned char) * m_width * m_height * m_channels;
	return memcmp(m_bmp, other.m_bmp, sizeOfBuffers) == 0;
}
