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
/***************************************************************************/
/*
 * pieTypes.h
 *
 * type defines for simple pies.
 *
 */
/***************************************************************************/

#ifndef _pieTypes_h
#define _pieTypes_h

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "gfx_api_formats_def.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

/***************************************************************************/
/*
 *	Global Definitions (CONSTANTS)
 */
/***************************************************************************/
#define LONG_WAY			(1<<15)
#define BUTTON_DEPTH		2000 // will be stretched to 16000

#define OLD_TEXTURE_SIZE_FIX 256.0f

//Render style flags for all pie draw functions
#define pie_ECM                 0x1
#define pie_TRANSLUCENT         0x2
#define pie_ADDITIVE            0x4
#define pie_FORCE_FOG           0x8
#define pie_HEIGHT_SCALED       0x10
#define pie_RAISE               0x20
#define pie_BUTTON              0x40
#define pie_SHADOW              0x80
#define pie_STATIC_SHADOW       0x100
#define pie_PREMULTIPLIED       0x200
#define pie_NODEPTHWRITE        0x400

#define pie_RAISE_SCALE			256

enum LIGHTING_TYPE
{
	LIGHT_EMISSIVE,
	LIGHT_AMBIENT,
	LIGHT_DIFFUSE,
	LIGHT_SPECULAR,
	LIGHT_MAX
};

enum REND_MODE
{
	REND_ALPHA,
	REND_ADDITIVE,
	REND_OPAQUE,
	REND_MULTIPLICATIVE,
	REND_PREMULTIPLIED,
	REND_TEXT,
};

enum DEPTH_MODE
{
	DEPTH_CMP_LEQ_WRT_ON,
	DEPTH_CMP_LEQ_WRT_OFF,
	DEPTH_CMP_ALWAYS_WRT_ON,
	DEPTH_CMP_ALWAYS_WRT_OFF
};

enum TEXPAGE_TYPE
{
	TEXPAGE_NONE = -1,
	TEXPAGE_EXTERN = -2
};

enum SHADER_MODE
{
	SHADER_NONE,
	SHADER_COMPONENT,
	SHADER_COMPONENT_INSTANCED,
	SHADER_NOLIGHT,
	SHADER_NOLIGHT_INSTANCED,
	SHADER_TERRAIN,
	SHADER_TERRAIN_DEPTH,
	SHADER_DECALS,
	SHADER_WATER,
	SHADER_RECT,
	SHADER_RECT_INSTANCED,
	SHADER_TEXRECT,
	SHADER_GFX_COLOUR,
	SHADER_GFX_TEXT,
	SHADER_SKYBOX,
	SHADER_GENERIC_COLOR,
	SHADER_LINE,
	SHADER_TEXT,
	SHADER_MAX
};

//*************************************************************************
//
// Simple derived types
//
//*************************************************************************

class iV_BaseImage
{
public:
	virtual ~iV_BaseImage() { }
public:
	virtual unsigned int width() const = 0;
	virtual unsigned int height() const = 0;
	virtual gfx_api::pixel_format pixel_format() const = 0;

	// Get a pointer to the image data that can be read
	virtual const unsigned char* data() const = 0;
	virtual size_t data_size() const = 0;
	virtual unsigned int bufferRowLength() const = 0;
	virtual unsigned int bufferImageHeight() const = 0;
};

// An uncompressed bitmap (R, RG, RGB, or RGBA)
// Not thread-safe
class iV_Image final : public iV_BaseImage
{
public:
	enum class ColorOrder {
		RGB,
		BGR
	};
private:
	unsigned int m_width = 0, m_height = 0, m_channels = 0;
	unsigned char *m_bmp = nullptr;
	ColorOrder m_colorOrder = ColorOrder::RGB;

public:
	// iV_BaseImage
	unsigned int width() const override { return m_width; }
	unsigned int height() const override { return m_height; }

	unsigned int channels() const  { return m_channels; }
	unsigned int depth() const { return m_channels * 8; }

	// Get the current bitmap pixel format
	gfx_api::pixel_format pixel_format() const override;

	const unsigned char* data() const override { return bmp(); }
	size_t data_size() const override { return size_in_bytes(); }
	unsigned int bufferRowLength() const override { return m_width; }
	unsigned int bufferImageHeight() const override { return m_height; }

public:
	static gfx_api::pixel_format pixel_format_for_channels(unsigned int channels);

public:
	// Allocate a new iV_Image buffer
	bool allocate(unsigned int width = 0, unsigned int height = 0, unsigned int channels = 0, bool zeroMemory = false);

	// Duplicate an iV_Image (makes a deep copy)
	bool duplicate(const iV_Image& other);

	// Clear (free) the image contents / data
	void clear();

	// Get a pointer to the bitmap data that can be read
	const unsigned char* bmp() const;

	// Get a pointer to the bitmap data that can be written to
	unsigned char* bmp_w();

public:
	// Get bmp size (in bytes)
	inline size_t size_in_bytes() const
	{
		if (!bmp()) { return 0; }
		return static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * static_cast<size_t>(m_channels);
	}

	// Convert and N-component bitmap to an N+1-component bitmap (as long as N < 4)
	// If the channel that's added is the 4th (alpha) channel, it's set to all opaque
	bool expand_channels_towards_rgba();

	// Convert any 1, 2, or 3 component bitmap to 4-component (RGBA)
	bool convert_to_rgba();

	// Convert a 2 (or more) component image to a 4-component (RGBA) image where the G channel is stored in the last component (A)
	bool convert_rg_to_ra_rgba();

	// Converts a 3 or 4 component (RGB/RGBA) image to a 1-component (luma)
	bool convert_to_luma();

	// Converts an image to a single-component image of the selected component/channel
	bool convert_to_single_channel(unsigned int channel = 0);

	bool resize(int newWidth, int newHeight, optional<int> alphaChannelOverride = nullopt);
	bool resizedFromOther(const iV_Image& other, int output_w, int output_h, optional<int> alphaChannelOverride = nullopt);
	bool scale_image_max_size(int maxWidth, int maxHeight);

	bool pad_image(unsigned int newWidth, unsigned int newHeight, bool useSmearing);

	bool convert_color_order(ColorOrder newOrder);

private:
	bool resizeInternal(const iV_Image& source, int output_w, int output_h, optional<int> alphaChannelOverride = nullopt);

public:
	// iV_Image is non-copyable
	iV_Image(const iV_Image&) = delete;
	iV_Image& operator= (const iV_Image&) = delete;

	// iV_Image is movable
	iV_Image(iV_Image&& other);
	iV_Image& operator=(iV_Image&& other);

public:
	iV_Image() = default;
	~iV_Image();
};

struct PIELIGHTBYTES
{
        uint8_t r, g, b, a;
};

/** Our basic colour type. Use whenever you want to define a colour.
 *  Set bytes separetely, and do not assume a byte order between the components. */
union PIELIGHT
{
        PIELIGHTBYTES byte;
        UDWORD rgba;
        UBYTE vector[4];
};

#endif // _pieTypes_h
