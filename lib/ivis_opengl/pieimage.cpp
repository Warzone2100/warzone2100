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
{
	std::swap(m_width, other.m_width);
	std::swap(m_height, other.m_height);
	std::swap(m_channels, other.m_channels);
	std::swap(m_bmp, other.m_bmp);
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
			// TODO: ASSERT FAILURE TO ALLOCATE MEMORY!!
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

	const size_t numBytes = static_cast<size_t>(m_height) * static_cast<size_t>(m_width) * static_cast<size_t>(m_channels);
	for (auto pPixelPos = bmp_w(); pPixelPos < (pPixelPos + numBytes); pPixelPos += 4)
	{
		pPixelPos[3] = pPixelPos[1];
		pPixelPos[2] = 0;
		pPixelPos[1] = 0;
	}
	return true;
}

bool iV_Image::convert_to_luma()
{
	if (m_channels == 1) { return true; }
	// Otherwise, expecting 3 or 4-channel (RGB/RGBA)
	ASSERT_OR_RETURN(false, m_channels == 3 || m_channels == 4, "Does not have 1, 3 or 4 channels");
	auto originalBmpData = m_bmp;
	const size_t numPixels = static_cast<size_t>(m_height) * static_cast<size_t>(m_width);
	m_bmp = (unsigned char *)malloc(numPixels);
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
	m_channels = 1;
	// free the original bitmap data
	free(originalBmpData);
	return true;
}

bool iV_Image::resize(int output_w, int output_h)
{
	unsigned char *output_pixels = (unsigned char *)malloc(static_cast<size_t>(output_w) * static_cast<size_t>(output_h) * m_channels);
	stbir_resize_uint8_generic(m_bmp, m_width, m_height, 0,
							   output_pixels, output_w, output_h, 0,
							   m_channels, m_channels == 4 ? 3 : STBIR_ALPHA_CHANNEL_NONE, 0,
							   STBIR_EDGE_CLAMP,
							   STBIR_FILTER_MITCHELL,
							   STBIR_COLORSPACE_LINEAR,
							   nullptr);
	free(m_bmp);
	m_width = output_w;
	m_height = output_h;
	m_bmp = output_pixels;

	return true;
}

bool iV_Image::scale_image_max_size(int maxWidth, int maxHeight)
{
	if ((maxWidth <= 0 || m_width <= maxWidth) && (maxHeight <= 0 || m_height <= maxHeight))
	{
		return false;
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

bool iV_Image::convert_to_single_channel(unsigned int channel /*= 0*/)
{
	ASSERT_OR_RETURN(false, channel < m_channels, "Cannot extract channel %u from image with %u channels", channel, m_channels);
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
