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
#include "lib/framework/file.h"
#include <stdlib.h>
#include <string.h>
#include "lib/framework/string_ext.h"
#include "lib/framework/geometry.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/bitimage.h"
#include "src/multiplay.h"
#include <algorithm>
#include <numeric>
#include <array>
#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include <LaunchInfo.h>

#define ASCII_SPACE			(32)
#define ASCII_NEWLINE			('@')
#define ASCII_COLOURMODE		('#')

// Contains the font color in the following order: red, green, blue, alpha
static float font_colour[4] = {1.f, 1.f, 1.f, 1.f};

#include "hb.h"
#include "hb-ft.h"
#include "ft2build.h"
#if defined(FT_MULTIPLE_MASTERS_H)
#include FT_MULTIPLE_MASTERS_H
#endif
#include FT_GLYPH_H
#if defined(FT_LCD_FILTER_H)
#include FT_LCD_FILTER_H
#endif
#include <unordered_map>
#include <memory>
#include <limits>
#include <climits>

#if defined(HB_VERSION_ATLEAST) && HB_VERSION_ATLEAST(1,0,5)
//	#define WZ_FT_LOAD_FLAGS (FT_LOAD_DEFAULT | FT_LOAD_TARGET_LCD) // Needs further testing on low-DPI displays
	#define WZ_FT_LOAD_FLAGS (FT_LOAD_NO_HINTING | FT_LOAD_TARGET_LCD)
#else
	// Without `hb_ft_font_set_load_flags` (which requires Harfbuzz 1.0.5+),
	// must default FreeType to the same flags that Harfbuzz internally uses
	// (by default hb loads fonts without hinting)
	#define WZ_FT_LOAD_FLAGS FT_LOAD_NO_HINTING
#endif
#define WZ_FT_RENDER_MODE FT_RENDER_MODE_LCD

#if defined(WZ_FRIBIDI_ENABLED)
# include "fribidi.h"
# define USE_NEW_FRIBIDI_API (FRIBIDI_MAJOR_VERSION >= 1)
#endif // defined(WZ_FRIBIDI_ENABLED)

#include "3rdparty/LRUCache11.hpp"

float _horizScaleFactor = 1.0f;
float _vertScaleFactor = 1.0f;

/***************************************************************************
 *
 *	Internal classes
 *
 ***************************************************************************/

namespace HBFeature
{
	const hb_tag_t KernTag = HB_TAG('k', 'e', 'r', 'n'); // kerning operations
	const hb_tag_t LigaTag = HB_TAG('l', 'i', 'g', 'a'); // standard ligature substitution
	const hb_tag_t CligTag = HB_TAG('c', 'l', 'i', 'g'); // contextual ligature substitution

	static hb_feature_t LigatureOn = { LigaTag, 1, 0, std::numeric_limits<unsigned int>::max() };
	static hb_feature_t KerningOn = { KernTag, 1, 0, std::numeric_limits<unsigned int>::max() };
	static hb_feature_t CligOn = { CligTag, 1, 0, std::numeric_limits<unsigned int>::max() };
}

struct RasterizedGlyph
{
	std::unique_ptr<unsigned char[]> buffer;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	int32_t bearing_x;
	int32_t bearing_y;
};

struct GlyphMetrics
{
	uint32_t width;
	uint32_t height;
	int32_t bearing_x;
	int32_t bearing_y;
};

static bool bLoadedTextSystem = false;
static std::unordered_map<std::string, std::shared_ptr<std::vector<char>>> m_loadedFontDataCache;

static std::shared_ptr<std::vector<char>> loadFontData(const std::string &fileName)
{
	auto it = m_loadedFontDataCache.find(fileName);
	if (it == m_loadedFontDataCache.end())
	{
		auto loadedFontDataPtr = std::make_shared<std::vector<char>>();
		debug(LOG_WZ, "Loading font data: %s", fileName.c_str());
		if (!loadFileToBufferVector(fileName.c_str(), *loadedFontDataPtr, true, false))
		{
			debug(LOG_WZ, "Failed to load font file: %s", fileName.c_str());
			return nullptr;
		}
		auto result = m_loadedFontDataCache.insert({fileName, loadedFontDataPtr});
		it = result.first;
	}
	return it->second;
}

static void clearFontDataCache()
{
	m_loadedFontDataCache.clear();
}

static const char* FallbackGetFTErrorStr(FT_Error error_code)
{
	#undef FTERRORS_H_
	#undef __FTERRORS_H__ // for older FreeType headers
#if defined(FT_CONFIG_OPTION_USE_MODULE_ERRORS)
	#define FT_ERROR_START_LIST     switch ( FT_ERROR_BASE(error_code) ) {
#else
	#define FT_ERROR_START_LIST     switch ( error_code ) {
#endif
	#define FT_ERRORDEF( e, v, s )    case v: return s;
	#define FT_ERROR_END_LIST       default: return nullptr; }
	#include FT_ERRORS_H
	return nullptr; // silence warnings
}

static const char* WZGetFTErrorStr(FT_Error error)
{
#if (FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && (FREETYPE_MINOR > 10 || (FREETYPE_MINOR == 10 && FREETYPE_PATCH >= 1) ))) // FreeType 2.10.1+ needed for FT_Error_String
	const char* pFtErrorStr = FT_Error_String(error); // FT_Error_String only returns a value if FreeType is compiled with the appropriate option(s)
	if (pFtErrorStr != nullptr)
	{
		return pFtErrorStr;
	}
#endif
	const char* pFTErrorStrFallback = FallbackGetFTErrorStr(error);
	return pFTErrorStrFallback;
}

struct FTFace
{
	FTFace(FT_Library &lib, const std::string &fileName, int32_t charSize, uint32_t horizDPI, uint32_t vertDPI, optional<uint16_t> fontWeight = nullopt)
	{
		pFileData = loadFontData(fileName);
		if (!pFileData)
		{
			throw std::runtime_error(astringf("Failed to load font file: %s", fileName.c_str()));
		}
#if SIZE_MAX > LONG_MAX
		if (pFileData->size() > static_cast<size_t>(std::numeric_limits<FT_Long>::max()))
		{
			throw std::runtime_error(astringf("Font file size (%zu) is too big: %s", pFileData->size(), fileName.c_str()));
		}
#endif
		FT_Error error = FT_New_Memory_Face(lib, (const FT_Byte*)pFileData->data(), static_cast<FT_Long>(pFileData->size()), 0, &m_face);
		if (error == FT_Err_Unknown_File_Format)
		{
			throw std::runtime_error(astringf("Unknown font file format for %s", fileName.c_str()));
		}
		else if (error != FT_Err_Ok)
		{
			const char* pFtErrorStr = WZGetFTErrorStr(error);
			if (pFtErrorStr != nullptr)
			{
				throw std::runtime_error(astringf("Failed to load font file %s, with error: %s", fileName.c_str(), pFtErrorStr));
			}
			throw std::runtime_error(astringf("Failed to load font file %s, with error: %d", fileName.c_str(), static_cast<int>(error)));
		}
		error = FT_Set_Char_Size(m_face, 0, charSize, horizDPI, vertDPI);
		if (error != FT_Err_Ok)
		{
			throw std::runtime_error("Could not set character size");
		}
#if defined(FT_HAS_MULTIPLE_MASTERS) && (FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && (FREETYPE_MINOR >= 7))) // FreeType 2.7+ needed for FT_Get_Var_Design_Coordinates
		if (fontWeight.has_value() && FT_HAS_MULTIPLE_MASTERS(m_face))
		{
			FT_MM_Var *amaster;
			error = FT_Get_MM_Var(m_face, &amaster);
			if (error == FT_Err_Ok)
			{
				// find the "weight"-tagged axis
				optional<FT_UInt> weight_axis_idx;
				auto widthTag = FT_MAKE_TAG('w','g','h','t');
				for (FT_UInt i = 0; i < amaster->num_axis; ++i)
				{
					if (amaster->axis[i].tag == widthTag && !weight_axis_idx.has_value())
					{
						weight_axis_idx = i;
					}
				}
				if (weight_axis_idx.has_value())
				{
					std::vector<FT_Fixed> variations(amaster->num_axis, 0);
					error = FT_Get_Var_Design_Coordinates(m_face, amaster->num_axis, variations.data());
					if (error == FT_Err_Ok)
					{
						// set the desired font weight
						variations[weight_axis_idx.value()] = static_cast<FT_Fixed>(fontWeight.value()) << 16;
						error = FT_Set_Var_Design_Coordinates(m_face, amaster->num_axis, variations.data());
						if (error != FT_Err_Ok)
						{
							debug(LOG_WZ, "Failed to set the font weight axis (%d): %s", (int)error, fileName.c_str());
						}
					}
					else
					{
						debug(LOG_WZ, "FT_Get_Var_Design_Coordinates failed (%d): %s", (int)error, fileName.c_str());
					}
				}
				else
				{
					debug(LOG_WZ, "Font has multiple masters, but unable to find 'weight' axis: %s", fileName.c_str());
				}

#if (FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && (FREETYPE_MINOR >= 9)))
				FT_Done_MM_Var(lib, amaster);
#else
				// FreeType < 2.9 lacks FT_Done_MM_Var - docs say to call `free` on the data structure
				free(amaster);
#endif
				amaster = nullptr;
			}
			else
			{
				debug(LOG_WZ, "Font claims to have multiple masters, but FT_Get_MM_Var failed (%d): %s", (int)error, fileName.c_str());
			}
		}
#else
		if (fontWeight.has_value())
		{
			debug(LOG_WARNING, "FreeType does not appear to have multiple masters support - using font weight axis modifications will fail. Try upgrading FreeType");
		}
#endif
		m_font = hb_ft_font_create(m_face, nullptr);
#if defined(HB_VERSION_ATLEAST) && HB_VERSION_ATLEAST(1,0,5)
		hb_ft_font_set_load_flags(m_font, WZ_FT_LOAD_FLAGS);
#endif
	}

	~FTFace()
	{
		hb_font_destroy(m_font);
		FT_Done_Face(m_face);
	}

	uint32_t getGlyphWidth(uint32_t codePoint)
	{
		FT_Error error = FT_Load_Glyph(m_face,
			codePoint, // the glyph_index in the font file
			WZ_FT_LOAD_FLAGS
		);
		ASSERT(error == FT_Err_Ok, "Unable to load glyph for %u", codePoint);
		return m_face->glyph->metrics.width;
	}

	FT_Glyph getGlyph(uint32_t codePoint)
	{
		FT_Error error = FT_Load_Glyph(m_face,
			codePoint, // the glyph_index in the font file
			WZ_FT_LOAD_FLAGS
		);
		ASSERT_OR_RETURN(nullptr, error == FT_Err_Ok, "Unable to load glyph %u", codePoint);

		FT_Glyph result;
		error = FT_Get_Glyph(m_face->glyph, &result);
		ASSERT_OR_RETURN(nullptr, error == FT_Err_Ok, "Unable to get glyph %u from slot", codePoint);

		return result;
	}

	operator FT_Face()
	{
		return m_face;
	}

	FT_Face &face() { return m_face; }

	hb_font_t *m_font;
	std::shared_ptr<std::vector<char>> pFileData;

private:
	FT_Face m_face;
};

static FTFace &getFTFace(iV_fonts FontID, hb_script_t script); // forward-declare

struct FTlib
{
	FTlib()
	{
		FT_Init_FreeType(&lib);
#if defined(FT_LCD_FILTER_H)
		FT_Error error = FT_Library_SetLcdFilter(lib, FT_LCD_FILTER_DEFAULT);
		if (error == 0)
		{
			debug(LOG_WZ, "Enabled FT_LCD_FILTER_DEFAULT");
		}
		else
		{
			const char* pFtErrorStr = WZGetFTErrorStr(error);
			debug(LOG_WZ, "Could not enable FT_LCD_FILTER_DEFAULT: %s", (pFtErrorStr) ? pFtErrorStr : "unknown");
		}
#endif
	}

	~FTlib()
	{
		FT_Done_FreeType(lib);
	}

	FT_Library lib;
};

struct FTGlyphCacheKey
{
	FTFace* face;
	uint32_t codepoint;

	FTGlyphCacheKey(FTFace& face, uint32_t codepoint)
	: face(&face), codepoint(codepoint)
	{ }

	bool operator==(const FTGlyphCacheKey& other) const
	{
		return face == other.face && codepoint == other.codepoint;
	}
};

namespace std {

	template <>
	struct hash<FTGlyphCacheKey>
	{
		std::size_t operator()(const FTGlyphCacheKey& k) const
		{
			return std::hash<FTFace*>()(k.face)
				 ^ (std::hash<int>()(k.codepoint) << 1);
		}
	};

}

struct FTCache
{
	FTCache()
	: m_glyphCache(256, 16)
	{ }

	RasterizedGlyph get(FTFace& face, uint32_t codePoint, Vector2i subpixeloffset64)
	{
		FT_Glyph glyph = getGlyph(face, codePoint);
		ASSERT_OR_RETURN({}, glyph != nullptr, "Failed to get glyph: %" PRIu32, codePoint);

		FT_Vector delta;
		delta.x = subpixeloffset64.x;
		delta.y = subpixeloffset64.y;

		FT_Error error = FT_Glyph_To_Bitmap(&glyph, WZ_FT_RENDER_MODE, &delta, 0);
		ASSERT_OR_RETURN({}, error == FT_Err_Ok, "Failed to render glyph: %" PRIu32, codePoint);
		// After this point, glyph is actually a new FT_BitmapGlyph (which must be released when done)

		FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyph;
		FT_Bitmap ftBitmap = glyph_bitmap->bitmap;

		RasterizedGlyph g;
		g.buffer.reset(new unsigned char[ftBitmap.pitch * ftBitmap.rows]);
		if (ftBitmap.buffer != nullptr)
		{
			memcpy(g.buffer.get(), ftBitmap.buffer, ftBitmap.pitch * ftBitmap.rows);
		}
		else
		{
			ASSERT(ftBitmap.pitch == 0 || ftBitmap.rows == 0, "Glyph buffer missing (%d and %d)", ftBitmap.pitch, ftBitmap.rows);
		}
		g.width = ftBitmap.width / 3;
		g.height = ftBitmap.rows;
		g.bearing_x = glyph_bitmap->left;
		g.bearing_y = glyph_bitmap->top;
		g.pitch = ftBitmap.pitch;

		FT_Done_Glyph(glyph);
		return g;
	}

	GlyphMetrics getGlyphMetrics(FTFace& face, uint32_t codePoint, Vector2i subpixeloffset64)
	{
		FT_Glyph glyph = getGlyph(face, codePoint);
		ASSERT_OR_RETURN({}, glyph != nullptr, "Failed to get glyph: %" PRIu32, codePoint);

		FT_Vector delta;
		delta.x = subpixeloffset64.x;
		delta.y = subpixeloffset64.y;

		// FUTURE FIXME: Surely there is a better way of getting the metrics we need? ....
		FT_Error error = FT_Glyph_To_Bitmap(&glyph, WZ_FT_RENDER_MODE, &delta, 0);
		ASSERT_OR_RETURN({}, error == FT_Err_Ok, "Failed to render glyph: %" PRIu32, codePoint);
		// After this point, glyph is actually a new FT_BitmapGlyph (which must be released when done)

		FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyph;
		FT_Bitmap ftBitmap = glyph_bitmap->bitmap;

		GlyphMetrics result {
			ftBitmap.width / 3,
			ftBitmap.rows,
			glyph_bitmap->left, glyph_bitmap->top
		};

		FT_Done_Glyph(glyph);

		return result;
	}

public:
	void clear()
	{
		m_glyphCache.clear();
	}

private:
	// The glyph is owned by the cache - if transforms are needed, the caller should use FT_Glyph_Copy to make a copy and modify the copy!
	FT_Glyph getGlyph(FTFace& face, uint32_t codepoint)
	{
		FT_Glyph glyph = nullptr;
		WZOwnedFTGlyph *pCachedGlyph = m_glyphCache.tryGetPt(FTGlyphCacheKey(face, codepoint));
		if (pCachedGlyph)
		{
			glyph = pCachedGlyph->glyph;
		}
		if (!glyph)
		{
			// not cached - load fresh
			glyph = face.getGlyph(codepoint);
			if (glyph)
			{
				// cache it
				m_glyphCache.insert(FTGlyphCacheKey(face, codepoint), WZOwnedFTGlyph(glyph));
			}
		}
		return glyph;
	}

private:
	struct WZOwnedFTGlyph
	{
		WZOwnedFTGlyph(FT_Glyph glyph)
		: glyph(glyph)
		{ }
		~WZOwnedFTGlyph() { if (glyph) { FT_Done_Glyph(glyph); } }

		// Prevent copies
		WZOwnedFTGlyph(const WZOwnedFTGlyph&) = delete;
		void operator=(const WZOwnedFTGlyph&) = delete;

		// Allow move semantics
		WZOwnedFTGlyph& operator=(WZOwnedFTGlyph&& other)
		{
			if (this != &other)
			{
				glyph = other.glyph;

				// Reset other
				other.glyph = nullptr;
			}
			return *this;
		}

		WZOwnedFTGlyph(WZOwnedFTGlyph&& other)
		{
			*this = std::move(other);
		}

	public:
		FT_Glyph glyph;
	};

	lru11::Cache<FTGlyphCacheKey, WZOwnedFTGlyph> m_glyphCache;
};

static FTCache* glyphCache = nullptr;

struct TextRun
{
	int startOffset;
	int endOffset;
	hb_script_t script;
	hb_direction_t direction;
	hb_language_t language;
	FTFace* fontFace;

	hb_buffer_t* buffer = nullptr; // owned
	unsigned int glyphCount;
	hb_glyph_info_t* glyphInfos = nullptr;
	hb_glyph_position_t* glyphPositions = nullptr;
	const uint32_t* codePoints = nullptr;

public:

	TextRun(const uint32_t* codePoints, int startOffset, int endOffset, hb_script_t script, hb_direction_t direction, hb_language_t language, FTFace& face)
	: startOffset(startOffset), endOffset(endOffset), script(script), direction(direction), language(language), fontFace(&face), codePoints(codePoints)
	{ }

	~TextRun()
	{
		if (buffer)
		{
			hb_buffer_destroy(buffer);
		}
	}

public:
	// Prevent copies
	TextRun(const TextRun&) = delete;
	void operator=(const TextRun&) = delete;

	// Allow move semantics
	TextRun& operator=(TextRun&& other)
	{
		if (this != &other)
		{
			// Free the existing (owned) buffer, if any
			if (buffer)
			{
				hb_buffer_destroy(buffer);
			}

			// Get the other data
			startOffset = other.startOffset;
			endOffset = other.endOffset;
			script = other.script;
			direction = other.direction;
			language = other.language;
			fontFace = other.fontFace;

			buffer = other.buffer; // owned
			glyphCount = other.glyphCount;
			glyphInfos = other.glyphInfos;
			glyphPositions = other.glyphPositions;
			codePoints = other.codePoints;

			// Reset other's pointer types
			other.buffer = nullptr;
			other.glyphInfos = nullptr;
			other.glyphPositions = nullptr;
		}
		return *this;
	}

	TextRun(TextRun&& other)
	{
		*this = std::move(other);
	}
};

struct TextLayoutMetrics
{
	TextLayoutMetrics(uint32_t _width, uint32_t _height) : width(_width), height(_height) { }
	TextLayoutMetrics() : width(0), height(0) { }
	uint32_t width;
	uint32_t height;
};

struct RenderedText
{
	RenderedText(std::unique_ptr<iV_Image> &&_bitmap, int32_t _offset_x, int32_t _offset_y)
	: bitmap(std::move(_bitmap)), offset_x(_offset_x), offset_y(_offset_y)
	{ }

	RenderedText() : bitmap(nullptr), offset_x(0), offset_y(0)
	{ }

	std::unique_ptr<iV_Image> bitmap;
	int32_t offset_x;
	int32_t offset_y;
};

struct DrawTextResult
{
	DrawTextResult(RenderedText &&_text, TextLayoutMetrics _layoutMetrics) : text(std::move(_text)), layoutMetrics(_layoutMetrics)
	{ }

	DrawTextResult()
	{ }

	RenderedText text;
	TextLayoutMetrics layoutMetrics;
};

// Note:
// Technically glyph antialiasing is dependent of text rotation.
// Rotated text needs to set transform inside freetype2.
// However there is few rotated text in wz2100 and it's likely to make
// only minimal visual difference.
struct TextShaper
{
	struct HarfbuzzPosition
	{
		hb_codepoint_t codepoint;
		Vector2i penPosition;
		FTFace& face;

		HarfbuzzPosition(hb_codepoint_t c, Vector2i &&p, FTFace& f) : codepoint(c), penPosition(p), face(f) {}
	};

	struct ShapingResult
	{
		std::vector<HarfbuzzPosition> glyphes;
		int32_t x_advance = 0;
		int32_t y_advance = 0;
	};

	TextShaper()
	{ }

	~TextShaper()
	{ }

	// Returns the text width and height *IN PIXELS*
	TextLayoutMetrics getTextMetrics(const WzString& text, iV_fonts fontID)
	{
		const ShapingResult& shapingResult = shapeText(text, fontID);

		if (shapingResult.glyphes.empty())
		{
			return TextLayoutMetrics(shapingResult.x_advance / 64, shapingResult.y_advance / 64);
		}

		int32_t min_x;
		int32_t max_x;
		int32_t min_y;
		int32_t max_y;

		std::tie(min_x, max_x, min_y, max_y) = std::accumulate(shapingResult.glyphes.begin(), shapingResult.glyphes.end(), std::make_tuple(1000, -1000, 1000, -1000),
			[] (const std::tuple<int32_t, int32_t, int32_t, int32_t> &bounds, const HarfbuzzPosition &g) {
			GlyphMetrics glyph = glyphCache->getGlyphMetrics(g.face, g.codepoint, g.penPosition % 64);
			int32_t x0 = g.penPosition.x / 64 + glyph.bearing_x;
			int32_t y0 = g.penPosition.y / 64 - glyph.bearing_y;
			return std::make_tuple(
				std::min(x0, std::get<0>(bounds)),
				std::max(static_cast<int32_t>(x0 + glyph.width), std::get<1>(bounds)),
				std::min(y0, std::get<2>(bounds)),
				std::max(static_cast<int32_t>(y0 + glyph.height), std::get<3>(bounds))
				);
			});

		const uint32_t texture_width = max_x - min_x + 1;
		const uint32_t texture_height = max_y - min_y + 1;
		const uint32_t x_advance = (shapingResult.x_advance / 64);
		const uint32_t y_advance = (shapingResult.y_advance / 64);

		// return the maximum of the x_advance / y_advance (converted from harfbuzz units) and the texture dimensions
		return TextLayoutMetrics(std::max(texture_width, x_advance), std::max(texture_height, y_advance));
	}

#if defined(WZ_FRIBIDI_ENABLED)
	FriBidiParType getBaseDirection()
	{
		std::string language = getLanguage();

		if (language == "ar_SA")
		{
			return HB_DIRECTION_RTL;
		}
		else
		{
			return HB_DIRECTION_LTR;
		}
	}
#endif

	// Draws the text and returns the text buffer, width and height, etc *IN PIXELS*
	DrawTextResult drawText(const WzString& text, iV_fonts fontID)
	{
		ShapingResult shapingResult = shapeText(text, fontID);

		if (shapingResult.glyphes.empty())
		{
			return DrawTextResult(RenderedText(), TextLayoutMetrics(shapingResult.x_advance / 64, shapingResult.y_advance / 64));
		}

		int32_t min_x = 1000;
		int32_t max_x = -1000;
		int32_t min_y = 1000;
		int32_t max_y = -1000;

		// build glyphes
		struct glyphRaster
		{
			std::unique_ptr<unsigned char[]> buffer;
			Vector2i pixelPosition;
			Vector2i size;
			uint32_t pitch;

			glyphRaster(std::unique_ptr<unsigned char[]> &&b, Vector2i &&p, Vector2i &&s, uint32_t _pitch)
				: buffer(std::move(b)), pixelPosition(p), size(s), pitch(_pitch) {}
		};

		std::vector<glyphRaster> glyphs;
		std::transform(shapingResult.glyphes.begin(), shapingResult.glyphes.end(), std::back_inserter(glyphs),
			[&] (const HarfbuzzPosition &g) {
			RasterizedGlyph glyph = glyphCache->get(g.face, g.codepoint, g.penPosition % 64);
			int32_t x0 = g.penPosition.x / 64 + glyph.bearing_x;
			int32_t y0 = g.penPosition.y / 64 - glyph.bearing_y;
			min_x = std::min(x0, min_x);
			max_x = std::max(static_cast<int32_t>(x0 + glyph.width), max_x);
			min_y = std::min(y0, min_y);
			max_y = std::max(static_cast<int32_t>(y0 + glyph.height), max_y);
			return glyphRaster(std::move(glyph.buffer), Vector2i(x0, y0), Vector2i(glyph.width, glyph.height), glyph.pitch);
			});

		const uint32_t texture_width = max_x - min_x + 1;
		const uint32_t texture_height = max_y - min_y + 1;
		const uint32_t x_advance = (shapingResult.x_advance / 64);
		const uint32_t y_advance = (shapingResult.y_advance / 64);

		if (texture_width == 0 || texture_height == 0)
		{
			return DrawTextResult(RenderedText(), TextLayoutMetrics(x_advance, y_advance));
		}

		std::unique_ptr<iV_Image> stringBitmap(new iV_Image());
		stringBitmap->allocate(texture_width, texture_height, 4, true);
		unsigned char* stringTexture = stringBitmap->bmp_w();
		const size_t stringTextureSize = stringBitmap->data_size();

		// TODO: Someone should document this piece.
		size_t glyphNum = 0;
		std::for_each(glyphs.begin(), glyphs.end(),
			[&](const glyphRaster &g)
			{
				const auto glyphBufferSize = g.pitch * g.size.y;
				for (int i = 0; i < g.size.y; ++i)
				{
					uint32_t i0 = g.pixelPosition.y - min_y;
					for (int j = 0; j < g.size.x; ++j)
					{
						uint32_t j0 = g.pixelPosition.x - min_x;
						const auto srcBufferPos = i * g.pitch + 3 * j;
						ASSERT(srcBufferPos + 2 < glyphBufferSize, "Invalid source (%" PRIu32" / %" PRIu32") reading glyph %zu for string \"%s\"; (%d, %d, %d, %d, %" PRIu32 ", %d, %d, %d, %" PRIu32 ", %" PRIu32 ")", srcBufferPos, glyphBufferSize, glyphNum, text.toUtf8().c_str(), i, g.size.y, g.pixelPosition.y, min_y, i0, j, g.pixelPosition.x, min_x, j0, g.pitch);
						uint8_t const *src = &g.buffer[srcBufferPos];
						const auto stringTexturePos = 4 * ((i0 + i) * texture_width + j + j0);
						ASSERT(stringTexturePos + 3 < stringTextureSize, "Invalid destination (%" PRIu32" / %zu) writing glyph %zu for string \"%s\"; (%d, %d, %d, %d, %" PRIu32 ", %d, %d, %d, %" PRIu32 ", %" PRIu32 ")", stringTexturePos, stringTextureSize, glyphNum, text.toUtf8().c_str(), i, g.size.y, g.pixelPosition.y, min_y, i0, j, g.pixelPosition.x, min_x, j0, texture_width);
						uint8_t *dst = &stringTexture[stringTexturePos];
						dst[0] = std::min(dst[0] + src[0], 255);
						dst[1] = std::min(dst[1] + src[1], 255);
						dst[2] = std::min(dst[2] + src[2], 255);
						dst[3] = std::min(dst[3] + ((src[0] * 77 + src[1] * 150 + src[2] * 29) >> 8), 255);
					}
				}
				++glyphNum;
			});

		return DrawTextResult(
				RenderedText(std::move(stringBitmap), min_x, min_y),
				TextLayoutMetrics(std::max(texture_width, x_advance), std::max(texture_height, y_advance))
		);
	}

	ShapingResult shapeText(const WzString& text, iV_fonts fontID)
	{
		/* Fribidi assumes that the text is encoded in UTF-32, so we have to
		   convert from UTF-8 to UTF-32, assuming that the string is indeed in UTF-8.*/
		std::vector<uint32_t> codePoints = text.toUtf32();
		if (codePoints.empty())
		{
			return ShapingResult();
		}
		int codePoints_size = static_cast<int>(codePoints.size());
#if SIZE_MAX > INT32_MAX
		if (codePoints.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
		{
			ASSERT(codePoints.size() <= static_cast<size_t>(std::numeric_limits<int>::max()), "text codePoints.size (%zu) exceeds int_max!", codePoints.size());
			codePoints_size = std::numeric_limits<int>::max(); // truncate
		}
#endif

		std::vector<hb_script_t> scripts(codePoints_size);

#if defined(WZ_FRIBIDI_ENABLED)
		// Step 1: Initialize fribidi variables.

		FriBidiParType baseDirection = getBaseDirection();
		FriBidiStrIndex size = static_cast<FriBidiStrIndex>(codePoints_size);
		std::vector<FriBidiCharType> types(size, static_cast<FriBidiCharType>(0));
		std::vector<FriBidiLevel> levels(size, static_cast<FriBidiLevel>(0));
# if USE_NEW_FRIBIDI_API
		std::vector<FriBidiBracketType> bracketedTypes(size, static_cast<FriBidiBracketType>(0));
# endif // USE_NEW_FRIBIDI_API

#else // !defined(WZ_FRIBIDI_ENABLED)

		std::vector<bool> levels(codePoints_size, 0);

#endif // defined(WZ_FRIBIDI_ENABLED)



#if defined(WZ_FRIBIDI_ENABLED)
		// Step 2: Run fribidi.

		/* Get the bidi type of each character in the string.*/
		fribidi_get_bidi_types(codePoints.data(), size, types.data());

# if USE_NEW_FRIBIDI_API
		fribidi_get_bracket_types(codePoints.data(), size, types.data(), bracketedTypes.data());

		FriBidiLevel maxLevel = fribidi_get_par_embedding_levels_ex(types.data(), bracketedTypes.data(), size, &baseDirection, levels.data());
		ASSERT(maxLevel != 0, "Error in fribidi_get_par_embedding_levels_ex!");
# else
		FriBidiLevel maxLevel = fribidi_get_par_embedding_levels(types.data(), size, &baseDirection, levels.data());
		ASSERT(maxLevel != 0, "Error in fribidi_get_par_embedding_levels_ex!");
# endif // USE_NEW_FRIBIDI_API

#endif // defined(WZ_FRIBIDI_ENABLED)
		
		/* Fill the array of scripts with scripts of each character */
		hb_unicode_funcs_t* funcs = hb_unicode_funcs_get_default();
		for (int i = 0; i < codePoints_size; ++i)
			scripts[i] = hb_unicode_script(funcs, codePoints[i]);


		// Step 3: Resolve common or inherited scripts.

		hb_script_t lastScriptValue = HB_SCRIPT_UNKNOWN;
		int lastScriptIndex = -1;
		int lastSetIndex = -1;

		for (int i = 0; i < codePoints_size; ++i)
		{
			if (scripts[i] == HB_SCRIPT_COMMON || scripts[i] == HB_SCRIPT_INHERITED)
			{
				if (lastScriptIndex != -1)
				{
					scripts[i] = lastScriptValue;
					lastSetIndex = i;
				}
			}
			else
			{
				for (int j = lastSetIndex + 1; j < i; ++j)
				{
					scripts[j] = scripts[i];
				}
				lastScriptValue = scripts[i];
				lastScriptIndex = i;
				lastSetIndex = i;
			}
		}


		// Step 4: Create the different runs

		hb_language_t language = hb_language_get_default(); // Future TODO: We could probably be smarter about this, but this replicates the behavior of hb_buffer_guess_segment_properties()

		std::vector<TextRun> textRuns;
		hb_script_t lastScript = scripts[0];
		auto lastLevel = levels[0];
		int lastRunStart = 0; // where the last run started

		/* i == size means that we've reached the end of the string,
		   and that the last run should be created.*/
		for (int i = 0; i <= codePoints_size; ++i)
		{
			/* If the script or level is of the current point is the same as the previous one,
			   then this means that the we have not reached the end of the current run.
			   If there's change, create a new run.*/
			if (i == codePoints_size || (scripts[i] != lastScript) || (levels[i] != lastLevel))
			{
				int startOffset = lastRunStart;
				int endOffset = i;
				hb_script_t script = lastScript;

#if defined(WZ_FRIBIDI_ENABLED)
				/* "lastLevel & 1" yields either 1 or 0, depending on the least significant bit of lastLevel.*/
				hb_direction_t direction = lastLevel & 1 ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
#else // !defined(WZ_FRIBIDI_ENABLED)
				hb_direction_t direction = hb_script_get_horizontal_direction(script);
#endif // defined(WZ_FRIBIDI_ENABLED)

				FTFace& face = getFTFace(fontID, script);

				textRuns.emplace_back(codePoints.data(), startOffset, endOffset, script, direction, language, face);

				if (i < codePoints_size)
				{
					lastScript = scripts[i];
					lastLevel = levels[i];
					lastRunStart = i;
				}
				else
				{
					break;
				}
			}
		}


		// Step 6: Shape each run using harfbuzz.

		ShapingResult shapingResult;

		for (int i = 0; i < textRuns.size(); ++i)
		{
			shapeHarfbuzz(textRuns[i], *textRuns[i].fontFace);
		}

		int32_t x = 0;
		int32_t y = 0;

		auto processTextRunGlyphs = [&](const TextRun& run) {
			for (unsigned int glyphIndex = 0; glyphIndex < run.glyphCount; ++glyphIndex)
			{
				hb_glyph_position_t& current_glyphPos = run.glyphPositions[glyphIndex];

				shapingResult.glyphes.emplace_back(run.glyphInfos[glyphIndex].codepoint, Vector2i(x + current_glyphPos.x_offset, y + current_glyphPos.y_offset), *run.fontFace);

				x += run.glyphPositions[glyphIndex].x_advance;
				y += run.glyphPositions[glyphIndex].y_advance;
			}
		};

#if defined(WZ_FRIBIDI_ENABLED)
		// The direction of the loop must change depending on the base direction
		if (!(FRIBIDI_IS_RTL(baseDirection)))
		{
#endif // defined(WZ_FRIBIDI_ENABLED)
			std::for_each(textRuns.cbegin(), textRuns.cend(), processTextRunGlyphs);
#if defined(WZ_FRIBIDI_ENABLED)
		}
		else
		{
			std::for_each(textRuns.crbegin(), textRuns.crend(), processTextRunGlyphs);
		}
#endif // defined(WZ_FRIBIDI_ENABLED)

		shapingResult.x_advance += x;
		shapingResult.y_advance += y;

		// Step 7: Finalize.

		return shapingResult;
	}

	inline void shapeHarfbuzz(TextRun& run, FTFace& face)
	{
		run.buffer = hb_buffer_create();
		hb_buffer_set_direction(run.buffer, run.direction);
		hb_buffer_set_script(run.buffer, run.script);
		hb_buffer_set_language(run.buffer, run.language);
		hb_buffer_add_utf32(run.buffer, run.codePoints + run.startOffset,
                            run.endOffset - run.startOffset, 0,
                            run.endOffset - run.startOffset);
		hb_buffer_set_flags(run.buffer, (hb_buffer_flags_t)(HB_BUFFER_FLAG_BOT | HB_BUFFER_FLAG_EOT));
		std::array<hb_feature_t, 3> features = { {HBFeature::KerningOn, HBFeature::LigatureOn, HBFeature::CligOn} };
        
		hb_shape(face.m_font, run.buffer, features.data(), static_cast<unsigned int>(features.size()));

		run.glyphInfos = hb_buffer_get_glyph_infos(run.buffer, &run.glyphCount);
		run.glyphPositions = hb_buffer_get_glyph_positions(run.buffer, &run.glyphCount);
	}
};

/***************************************************************************/
/*
 *	Main source
 */
/***************************************************************************/

void iV_font(const char *fontName, const char *fontFace, const char *fontFaceBold)
{
}

FTlib &getGlobalFTlib()
{
	static FTlib globalFT;
	return globalFT;
}

TextShaper &getShaper()
{
	static TextShaper shaper;
	return shaper;
}

inline float iV_GetHorizScaleFactor()
{
	return _horizScaleFactor;
}

inline float iV_GetVertScaleFactor()
{
	return _vertScaleFactor;
}

// The base DPI used internally.
// Do not change this, or various layout in the game interface & menus will break.
#define DEFAULT_DPI 72.0f

static hb_unicode_funcs_t* m_unicode_funcs_hb = nullptr;

struct WZFontCollection
{
public:
	std::unique_ptr<FTFace> regular;
	std::unique_ptr<FTFace> regularBold;
	std::unique_ptr<FTFace> bold;
	std::unique_ptr<FTFace> medium;
	std::unique_ptr<FTFace> mediumBold;
	std::unique_ptr<FTFace> small;
	std::unique_ptr<FTFace> smallBold;
};
static WZFontCollection* baseFonts = nullptr;
static WZFontCollection* cjkFonts = nullptr;
static bool failedToLoadCJKFonts = false;

struct iVFontsHash
{
    std::size_t operator()(iV_fonts FontID) const
    {
        return static_cast<std::size_t>(FontID);
    }
};
typedef std::unordered_map<iV_fonts, WzText, iVFontsHash> FontToEllipsisMapType;
static FontToEllipsisMapType fontToEllipsisMap;

#define CJK_FONT_PATH "fonts/NotoSansCJK-VF.otf.ttc"

static bool inline initializeCJKFontsIfNeeded()
{
	if (cjkFonts) { return true; }
	if (failedToLoadCJKFonts) { return false; }
	cjkFonts = new WZFontCollection();
	uint32_t horizDPI = static_cast<uint32_t>(DEFAULT_DPI * _horizScaleFactor);
	uint32_t vertDPI = static_cast<uint32_t>(DEFAULT_DPI * _vertScaleFactor);
	try {
		cjkFonts->regular = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, CJK_FONT_PATH, 12 * 64, horizDPI, vertDPI, 400));
		cjkFonts->regularBold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, CJK_FONT_PATH, 12 * 64, horizDPI, vertDPI, 700));
		cjkFonts->bold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, CJK_FONT_PATH, 21 * 64, horizDPI, vertDPI, 400));
		cjkFonts->medium = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, CJK_FONT_PATH, 16 * 64, horizDPI, vertDPI, 400));
		cjkFonts->mediumBold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, CJK_FONT_PATH, 16 * 64, horizDPI, vertDPI, 700));
		cjkFonts->small = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, CJK_FONT_PATH, 9 * 64, horizDPI, vertDPI, 400));
		cjkFonts->smallBold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, CJK_FONT_PATH, 9 * 64, horizDPI, vertDPI, 700));
	}
	catch (const std::exception &e) {
		debug(LOG_ERROR, "Failed to load font:\n%s", e.what());
		delete cjkFonts;
		cjkFonts = nullptr;
		failedToLoadCJKFonts = true;
		return false;
	}

	return true;
}

static FTFace &getFTFace(iV_fonts FontID, hb_script_t script)
{
	switch (script)
	{
		case HB_SCRIPT_HAN:
		case HB_SCRIPT_BOPOMOFO:
		case HB_SCRIPT_HANGUL:
		case HB_SCRIPT_HIRAGANA:
		case HB_SCRIPT_KATAKANA:
			if (initializeCJKFontsIfNeeded())
			{
				switch (FontID)
				{
				default:
				case font_regular:
					return *(cjkFonts->regular);
				case font_regular_bold:
					return *(cjkFonts->regularBold);
				case font_large:
					return *(cjkFonts->bold);
				case font_medium:
					return *(cjkFonts->medium);
				case font_medium_bold:
					return *(cjkFonts->mediumBold);
				case font_small:
					return *(cjkFonts->small);
				case font_bar:
					return *(cjkFonts->smallBold);
				}
			}
			break;
		default:
			break;
	}
	switch (FontID)
	{
	default:
	case font_regular:
		return *(baseFonts->regular);
	case font_regular_bold:
		return *(baseFonts->regularBold);
	case font_large:
		return *(baseFonts->bold);
	case font_medium:
		return *(baseFonts->medium);
	case font_medium_bold:
		return *(baseFonts->mediumBold);
	case font_small:
		return *(baseFonts->small);
	case font_bar:
		return *(baseFonts->smallBold);
	}
}

static gfx_api::texture* textureID = nullptr;

void iV_TextInit(unsigned int horizScalePercentage, unsigned int vertScalePercentage)
{
	if (horizScalePercentage > 100 && horizScalePercentage < 200)
	{
		horizScalePercentage *= 2;
	}
	if (vertScalePercentage > 100 && vertScalePercentage < 200)
	{
		vertScalePercentage *= 2;
	}
	float horizScaleFactor = horizScalePercentage / 100.f;
	float vertScaleFactor = vertScalePercentage / 100.f;
	assert(horizScaleFactor >= 1.0f);
	assert(vertScaleFactor >= 1.0f);

	// Use the scaling factors to multiply the default DPI (72) to determine the desired internal font rendering DPI.
	_horizScaleFactor = horizScaleFactor;
	_vertScaleFactor = vertScaleFactor;
	uint32_t horizDPI = static_cast<uint32_t>(DEFAULT_DPI * horizScaleFactor);
	uint32_t vertDPI = static_cast<uint32_t>(DEFAULT_DPI * vertScaleFactor);
	debug(LOG_WZ, "Text-Rendering Scaling Factor: %f x %f; Internal Font DPI: %" PRIu32 " x %" PRIu32 "", _horizScaleFactor, _vertScaleFactor, horizDPI, vertDPI);

	if (glyphCache == nullptr)
	{
		glyphCache = new FTCache();
	}

	if (baseFonts == nullptr)
	{
		baseFonts = new WZFontCollection();
	}

	try {
		baseFonts->regular = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, "fonts/DejaVuSans.ttf", 12 * 64, horizDPI, vertDPI));
		baseFonts->regularBold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, "fonts/DejaVuSans-Bold.ttf", 12 * 64, horizDPI, vertDPI));
		baseFonts->bold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, "fonts/DejaVuSans-Bold.ttf", 21 * 64, horizDPI, vertDPI));
		baseFonts->medium = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, "fonts/DejaVuSans.ttf", 16 * 64, horizDPI, vertDPI));
		baseFonts->mediumBold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, "fonts/DejaVuSans-Bold.ttf", 16 * 64, horizDPI, vertDPI));
		baseFonts->small = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, "fonts/DejaVuSans.ttf", 9 * 64, horizDPI, vertDPI));
		baseFonts->smallBold = std::unique_ptr<FTFace>(new FTFace(getGlobalFTlib().lib, "fonts/DejaVuSans-Bold.ttf", 9 * 64, horizDPI, vertDPI));
	}
	catch (const std::exception &e) {
		// Log lots of details:
		debugOutputSearchPaths();
		debug(LOG_INFO, "Virtual filesystem:");
		WZ_PHYSFS_enumerateFiles("", [&](const char *file) -> bool {
			if (!file) { return true; }
			debug(LOG_INFO, " - %s", file);
			return true; // continue enumeration
		});
		debug(LOG_INFO, "Path to executable: %s", LaunchInfo::getCurrentProcessDetails().imageFileName.fullPath().c_str());
		debugOutputSearchPathMountErrors();
		// then...
		debug(LOG_FATAL, "Failed to load base font:\n%s\n", e.what());
		abort();
	}

	// Do a sanity-check here to make sure the CJK font exists
	// (since it's only loaded on-demand, and thus might fail with a fatal error later if missing)
	if (PHYSFS_exists(CJK_FONT_PATH) == 0)
	{
		debug(LOG_FATAL, "Missing data file: %s", CJK_FONT_PATH);
	}

	m_unicode_funcs_hb = hb_unicode_funcs_get_default();

	// hb_language_get_default: "To avoid problems, call this function once before multiple threads can call it."
	hb_language_get_default();

	bLoadedTextSystem = true;
}

void iV_TextShutdown()
{
	glyphCache->clear();
	delete glyphCache;
	glyphCache = nullptr;
	delete baseFonts;
	baseFonts = nullptr;
	delete cjkFonts;
	cjkFonts = nullptr;
	delete textureID;
	textureID = nullptr;
	fontToEllipsisMap.clear();
	clearFontDataCache();
	bLoadedTextSystem = false;
}

void iV_TextUpdateScaleFactor(unsigned int horizScalePercentage, unsigned int vertScalePercentage)
{
	if (!bLoadedTextSystem)
	{
		return;
	}
	iV_TextShutdown();
	iV_TextInit(horizScalePercentage, vertScalePercentage);
}

static WzText& iV_Internal_GetEllipsis(iV_fonts fontID)
{
	auto it = fontToEllipsisMap.find(fontID);
	if (it == fontToEllipsisMap.end())
	{
		// We must create + cache an ellipsis for this fontID
		it = fontToEllipsisMap.insert(FontToEllipsisMapType::value_type(fontID, WzText("\u2026", fontID))).first;
	}
	return it->second;
}

int iV_GetEllipsisWidth(iV_fonts fontID)
{
	return iV_Internal_GetEllipsis(fontID).width();
}

void iV_DrawEllipsis(iV_fonts fontID, Vector2f position, PIELIGHT colour)
{
	iV_Internal_GetEllipsis(fontID).render(position, colour);
}

unsigned int width_pixelsToPoints(unsigned int widthInPixels)
{
	return static_cast<int>(ceil((float)widthInPixels / _horizScaleFactor));
}
unsigned int height_pixelsToPoints(unsigned int heightInPixels)
{
	return static_cast<int>(ceil((float)heightInPixels / _vertScaleFactor));
}

// Returns the text width *in points*
unsigned int iV_GetTextWidth(const WzString& string, iV_fonts fontID)
{
	TextLayoutMetrics metrics = getShaper().getTextMetrics(string, fontID);
	return width_pixelsToPoints(metrics.width);
}

// Returns the counted text width *in points*
unsigned int iV_GetCountedTextWidth(const char *string, size_t string_length, iV_fonts fontID)
{
	return iV_GetTextWidth(string, fontID);
}

// Returns the text height *in points*
unsigned int iV_GetTextHeight(const char* string, iV_fonts fontID)
{
	TextLayoutMetrics metrics = getShaper().getTextMetrics(string, fontID);
	return height_pixelsToPoints(metrics.height);
}

// Returns the character width *in points*
unsigned int iV_GetCharWidth(uint32_t charCode, iV_fonts fontID)
{
	return width_pixelsToPoints(getFTFace(fontID, hb_unicode_script(m_unicode_funcs_hb, charCode)).getGlyphWidth(charCode) >> 6);
}

int metricsHeight_PixelsToPoints(int heightMetric)
{
	float ptMetric = (float)heightMetric / _vertScaleFactor;
	return (ptMetric < 0) ? static_cast<int>(floor(ptMetric)) : static_cast<int>(ceil(ptMetric));
}

int iV_GetTextLineSize(iV_fonts fontID)
{
	FT_Face face = getFTFace(fontID, HB_SCRIPT_COMMON); // TODO: Better handling of script-specific font faces?
	return metricsHeight_PixelsToPoints((face->size->metrics.ascender - face->size->metrics.descender) >> 6);
}

int iV_GetTextAboveBase(iV_fonts fontID)
{
	FT_Face face = getFTFace(fontID, HB_SCRIPT_COMMON); // TODO: Better handling of script-specific font faces?
	return metricsHeight_PixelsToPoints(-(face->size->metrics.ascender >> 6));
}

int iV_GetTextBelowBase(iV_fonts fontID)
{
	FT_Face face = getFTFace(fontID, HB_SCRIPT_COMMON); // TODO: Better handling of script-specific font faces?
	return metricsHeight_PixelsToPoints(face->size->metrics.descender >> 6);
}

void iV_SetTextColour(PIELIGHT colour)
{
	font_colour[0] = colour.byte.r / 255.0f;
	font_colour[1] = colour.byte.g / 255.0f;
	font_colour[2] = colour.byte.b / 255.0f;
	font_colour[3] = colour.byte.a / 255.0f;
}

optional<iV_fonts> iV_ShrinkFont(iV_fonts fontID)
{
	switch (fontID)
	{
		// bold fonts
		case font_large: // is actually bold
			return font_medium_bold;
		case font_medium_bold:
			return font_regular_bold;
		case font_regular_bold:
			return font_bar; // small_bold
		case font_bar:	// small_bold
			return nullopt;

		// regular fonts
		case font_medium:
			return font_regular;
		case font_scaled: // treated the same as font_regular
		case font_regular:
			return font_small;
		case font_small:
			return nullopt;

		case font_count:
			return nullopt;
	}

	return nullopt; // silence compiler warning
}

static bool breaksLine(char const c)
{
	return c == ASCII_NEWLINE || c == '\n';
}

static bool breaksWord(char const c)
{
	return c == ASCII_SPACE || breaksLine(c);
}

inline size_t utf8SequenceLength(const char* pChar)
{
	uint8_t first = *pChar;

	if (first < 0x80)
		return 1;
	else if ((first >> 5) == 0x6)
		return 2;
	else if ((first >> 4) == 0xe)
		return 3;
	else if ((first >> 3) == 0x1e)
		return 4;

	return 0;
}

std::vector<TextLine> iV_FormatText(const WzString& String, UDWORD MaxWidth, UDWORD Justify, iV_fonts fontID, bool ignoreNewlines /*= false*/)
{
	std::vector<TextLine> lineDrawResults;

	std::string FString;
	std::string FWord;
	const int x = 0;
	const int y = 0;
	size_t i;
	int jx = x;		// Default to left justify.
	int jy = y;
	UDWORD WWidth;
	int TWidth;
	const char *curChar = String.toUtf8().c_str();
	const char *charEnd = curChar + strlen(curChar);
	size_t sequenceLen = 1;

	while (*curChar != 0)
	{
		bool GotSpace = false;
		bool NewLine = false;

		// Reset text draw buffer
		FString.clear();

		WWidth = 0;

		size_t indexWithinLine = 0;

		// Parse through the string, adding words until width is achieved.
		while (*curChar != 0 && (WWidth == 0 || WWidth < MaxWidth) && !NewLine)
		{
			const char *startOfWord = curChar;
			const unsigned int FStringWidth = iV_GetTextWidth(FString.c_str(), fontID);

			// Get the next word.
			i = 0;
			FWord.clear();
			for (
				;
				*curChar && ((indexWithinLine == 0 && !breaksLine(*curChar)) || !breaksWord(*curChar));
				i += sequenceLen, curChar += sequenceLen, indexWithinLine += sequenceLen
			)
			{
				sequenceLen = utf8SequenceLength(curChar);
				if (sequenceLen == 0)
				{
					ASSERT(false, "curr_sequence_length is 0?? for string: %s", String.toUtf8().c_str());
					sequenceLen = 1;
				}
				if (curChar + sequenceLen > charEnd)
				{
					sequenceLen = (charEnd - curChar);
				}

				if (*curChar == ASCII_COLOURMODE) // If it's a colour mode toggle char then just add it to the word.
				{
					FWord.push_back(*curChar);

					// this character won't be drawn so don't deal with its width
					continue;
				}

				// Get the full (possibly multi-byte) sequence for this codepoint
				for (size_t seqIdx = 0; seqIdx < sequenceLen; ++seqIdx)
				{
					FWord.push_back(*(curChar + seqIdx));
				}

				// Update this line's pixel width.
				//WWidth = FStringWidth + iV_GetCountedTextWidth(FWord.c_str(), i + 1, fontID);  // This triggers tonnes of valgrind warnings, if the string contains unicode. Adding lots of trailing garbage didn't help... Using iV_GetTextWidth with a null-terminated string, instead.
				WWidth = FStringWidth + iV_GetTextWidth(FWord.c_str(), fontID);

				// If this word doesn't fit on the current line then break out
				if (indexWithinLine != 0 && WWidth > MaxWidth)
				{
					FWord.erase(FWord.size() - 1);
					break;
				}
			}

			// Don't forget the space.
			if (*curChar == ASCII_SPACE)
			{
				FWord.push_back(' ');
				++i;
				++curChar;
				GotSpace = true;
				auto spaceWidth = iV_GetCharWidth(' ', fontID);
				if (WWidth + spaceWidth <= MaxWidth)
				{
					WWidth += spaceWidth;
				}
			}
			// Check for new line character.
			else if (breaksLine(*curChar))
			{
				if (!ignoreNewlines)
				{
					NewLine = true;
				}
				++curChar;
			}

			// If we've passed a space on this line and the word goes past the
			// maximum width and this isn't caused by the appended space then
			// rewind to the start of this word and finish this line.
			if (GotSpace
			    && i != 0
			    && WWidth > MaxWidth
			    && FWord[i - 1] != ' ')
			{
				// Skip back to the beginning of this
				// word and draw it on the next line
				curChar = startOfWord;
				break;
			}

			// And add it to the output string.
			FString.append(FWord);
		}


		// Remove trailing spaces, useful when doing center alignment.
		while (!FString.empty() && FString[FString.size() - 1] == ' ')
		{
			FString.erase(FString.size() - 1);  // std::string has no pop_back().
		}

		TWidth = iV_GetTextWidth(FString.c_str(), fontID);

		// Do justify.
		switch (Justify)
		{
		case FTEXT_CENTRE:
			jx = x + (MaxWidth - TWidth) / 2;
			break;

		case FTEXT_RIGHTJUSTIFY:
			jx = x + MaxWidth - TWidth;
			break;

		case FTEXT_LEFTJUSTIFY:
			jx = x;
			break;
		}

		// Store the line of text and its position in the bounding rect
		lineDrawResults.push_back({FString, Vector2i(TWidth, iV_GetTextLineSize(fontID)), Vector2i(jx, jy)});

		// and move down a line.
		jy += iV_GetTextLineSize(fontID);
	}

	return lineDrawResults;
}

// Needs modification
void iV_DrawTextRotated(const char* string, float XPos, float YPos, float rotation, iV_fonts fontID)
{
	ASSERT_OR_RETURN(, string, "Couldn't render string!");

	if (rotation != 0.f)
	{
		rotation = 180.f - rotation;
	}

	PIELIGHT color;
	color.vector[0] = static_cast<UBYTE>(font_colour[0] * 255.f);
	color.vector[1] = static_cast<UBYTE>(font_colour[1] * 255.f);
	color.vector[2] = static_cast<UBYTE>(font_colour[2] * 255.f);
	color.vector[3] = static_cast<UBYTE>(font_colour[3] * 255.f);

	DrawTextResult drawResult = getShaper().drawText(string, fontID);

	if (drawResult.text.bitmap && drawResult.text.bitmap->width() > 0 && drawResult.text.bitmap->height() > 0)
	{
		if (textureID)
			delete textureID;
		textureID = gfx_api::context::get().createTextureForCompatibleImageUploads(1, *(drawResult.text.bitmap.get()), std::string("text::") + string);
		textureID->upload(0u, *(drawResult.text.bitmap.get()));
		iV_DrawImageText(*textureID, Vector2f(XPos, YPos), Vector2f((float)drawResult.text.offset_x / _horizScaleFactor, (float)drawResult.text.offset_y / _vertScaleFactor), Vector2f((float)drawResult.text.bitmap->width() / _horizScaleFactor, (float)drawResult.text.bitmap->height() / _vertScaleFactor), rotation, color);
	}
}

int WzText::width()
{
	updateCacheIfNecessary();
	return width_pixelsToPoints(layoutMetrics.x);
}
int WzText::height()
{
	updateCacheIfNecessary();
	return height_pixelsToPoints(layoutMetrics.y);
}
int WzText::aboveBase()
{
	updateCacheIfNecessary();
	return mPtsAboveBase;
}
int WzText::belowBase()
{
	updateCacheIfNecessary();
	return mPtsBelowBase;
}
int WzText::lineSize()
{
	updateCacheIfNecessary();
	return mPtsLineSize;
}

void WzText::setText(const WzString &text, iV_fonts fontID/*, bool delayRender*/)
{
	if (mText == text && fontID == mFontID)
	{
		return; // cached
	}
	drawAndCacheText(text, fontID);
}

void WzText::drawAndCacheText(const WzString& string, iV_fonts fontID)
{
	mFontID = fontID;
	mText = string;
	mRenderingHorizScaleFactor = iV_GetHorizScaleFactor();
	mRenderingVertScaleFactor = iV_GetVertScaleFactor();

	FTFace &face = getFTFace(fontID, HB_SCRIPT_COMMON);
	FT_Face &type = face.face();

	// TODO: Better handling here of multiple font faces?
	mPtsAboveBase = metricsHeight_PixelsToPoints(-(type->size->metrics.ascender >> 6));
	mPtsLineSize = metricsHeight_PixelsToPoints((type->size->metrics.ascender - type->size->metrics.descender) >> 6);
	mPtsBelowBase = metricsHeight_PixelsToPoints(type->size->metrics.descender >> 6);

	DrawTextResult drawResult = getShaper().drawText(string, fontID);
	dimensions = (drawResult.text.bitmap) ? Vector2i(drawResult.text.bitmap->width(), drawResult.text.bitmap->height()) : Vector2i(0,0);
	offsets = Vector2i(drawResult.text.offset_x, drawResult.text.offset_y);
	layoutMetrics = Vector2i(drawResult.layoutMetrics.width, drawResult.layoutMetrics.height);

	if (texture)
	{
		delete texture;
		texture = nullptr;
	}

	if (dimensions.x > 0 && dimensions.y > 0)
	{
		texture = gfx_api::context::get().createTextureForCompatibleImageUploads(1, *(drawResult.text.bitmap.get()), std::string("text::") + string.toUtf8());
		texture->upload(0u, *(drawResult.text.bitmap.get()));
	}
}

void WzText::redrawAndCacheText()
{
	drawAndCacheText(mText, mFontID);
}

WzText::WzText(const WzString &string, iV_fonts fontID)
{
	setText(string, fontID);
}

WzText::~WzText()
{
	if (texture)
		delete texture;
}

WzText& WzText::operator=(WzText&& other)
{
	if (this != &other)
	{
		// Free the existing texture, if any.
		if (texture)
		{
			delete texture;
		}

		// Get the other data
		texture = other.texture;
		mFontID = other.mFontID;
		mText = std::move(other.mText);
		mPtsAboveBase = other.mPtsAboveBase;
		mPtsBelowBase = other.mPtsBelowBase;
		mPtsLineSize = other.mPtsLineSize;
		offsets = other.offsets;
		dimensions = other.dimensions;
		mRenderingHorizScaleFactor = other.mRenderingHorizScaleFactor;
		mRenderingVertScaleFactor = other.mRenderingVertScaleFactor;
		layoutMetrics = other.layoutMetrics;

		// Reset other's texture
		other.texture = nullptr;
	}
	return *this;
}

WzText::WzText(WzText&& other)
{
	*this = std::move(other);
}

inline void WzText::updateCacheIfNecessary()
{
	if (mText.isEmpty())
	{
		return; // string is empty (or hasn't yet been set), thus changes have no effect
	}
	if (mRenderingHorizScaleFactor != iV_GetHorizScaleFactor() || mRenderingVertScaleFactor != iV_GetVertScaleFactor())
	{
		// The text rendering subsystem's scale factor has changed, so the rendered (cached) text must be re-rendered.
		redrawAndCacheText();
		// debug(LOG_WZ, "Redrawing / re-calculating WzText text - scale factor has changed.");
	}
}

void WzText::render(Vector2f position, PIELIGHT colour, float rotation, int maxWidth, int maxHeight)
{
	updateCacheIfNecessary();

	if (texture == nullptr)
	{
		// A texture will not always be created. (For example, if the rendered text is empty.)
		// No need to render if there's nothing to render.
		return;
	}

	if (rotation != 0.f)
	{
		rotation = 180.f - rotation;
	}

	if (maxWidth <= 0 && maxHeight <= 0)
	{
		iV_DrawImageText(*texture, position, Vector2f(offsets.x / mRenderingHorizScaleFactor, offsets.y / mRenderingVertScaleFactor), Vector2f(dimensions.x / mRenderingHorizScaleFactor, dimensions.y / mRenderingVertScaleFactor), rotation, colour);
	}
	else
	{
		WzRect clippingRectInPixels;
		clippingRectInPixels.setWidth((maxWidth > 0) ? static_cast<int>((float)maxWidth * mRenderingHorizScaleFactor) : dimensions.x);
		clippingRectInPixels.setHeight((maxHeight > 0) ? static_cast<int>((float)maxHeight * mRenderingVertScaleFactor) : dimensions.y);
		iV_DrawImageTextClipped(*texture, dimensions, position, Vector2f(offsets.x / mRenderingHorizScaleFactor, offsets.y / mRenderingVertScaleFactor), Vector2f((maxWidth > 0) ? maxWidth : dimensions.x / mRenderingHorizScaleFactor, (maxHeight > 0) ? maxHeight : dimensions.y / mRenderingVertScaleFactor), rotation, colour, clippingRectInPixels);
	}
}

void WzText::renderOutlined(int x, int y, PIELIGHT colour, PIELIGHT outlineColour)
{
	for (auto i = -1; i <= 1; i++)
	{
		for (auto j = -1; j <= 1; j++)
		{
			render(x + i, y + j, outlineColour);
		}
	}
	render(x, y, colour);
}

// Sets the text, truncating to a desired width limit (in *points*) if needed
// returns: the length of the string that will be drawn (may be less than the input text.length() if truncated)
size_t WidthLimitedWzText::setTruncatableText(const WzString &text, iV_fonts fontID, size_t limitWidthInPoints)
{
	if ((mFullText == text) && (mLimitWidthPts == limitWidthInPoints) && (getFontID() == fontID))
	{
		return getText().length(); // skip; no change
	}

	mFullText = text;
	mLimitWidthPts = limitWidthInPoints;

	WzString truncatedText = text;
	while ((truncatedText.length() > 0) && (iV_GetTextWidth(truncatedText, fontID) > limitWidthInPoints))
	{
		if (!truncatedText.pop_back())
		{
			ASSERT(false, "WzString::pop_back() failed??");
			break;
		}
	}

	WzText::setText(truncatedText, fontID);
	return truncatedText.length();
}
