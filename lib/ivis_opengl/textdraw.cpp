/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
#include "lib/framework/opengl.h"
#include <stdlib.h>
#include <string.h>
#include "lib/framework/string_ext.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/bitimage.h"
#include "src/multiplay.h"

#define ASCII_SPACE			(32)
#define ASCII_NEWLINE			('@')
#define ASCII_COLOURMODE		('#')

#ifdef WZ_OS_MAC
# include <CoreFoundation/CoreFoundation.h>
# include <CoreFoundation/CFURL.h>
# include <QuesoGLC/glc.h>
#else
# include <GL/glc.h>
#endif

static char font_family[128];
static char font_face_regular[128];
static char font_face_bold[128];

static float font_size = 12.f;
// Contains the font color in the following order: red, green, blue, alpha
static float font_colour[4] = {1.f, 1.f, 1.f, 1.f};

static GLint _glcContext = 0;
static GLint _glcFont_Regular = 0;
static GLint _glcFont_Bold = 0;

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

void iV_font(const char *fontName, const char *fontFace, const char *fontFaceBold)
{
	if (_glcContext)
	{
		debug(LOG_ERROR, "Cannot change font in running game, yet.");
		return;
	}
	if (fontName)
	{
		sstrcpy(font_family, fontName);
	}
	if (fontFace)
	{
		sstrcpy(font_face_regular, fontFace);
	}
	if (fontFaceBold)
	{
		sstrcpy(font_face_bold, fontFaceBold);
	}
}

static inline void iV_printFontList(void)
{
	unsigned int i;
	unsigned int font_count = glcGeti(GLC_CURRENT_FONT_COUNT);
	debug(LOG_NEVER, "GLC_CURRENT_FONT_COUNT = %d", font_count);

	if (font_count == 0)
	{
		debug(LOG_ERROR, "The required font (%s) isn't loaded", font_family);

		// Fall back to unselected fonts since the requested font apparently
		// isn't available.
		glcEnable(GLC_AUTO_FONT);
	}

	for (i = 0; i < font_count; ++i)
	{
		GLint font = glcGetListi(GLC_CURRENT_FONT_LIST, i);
		/* The output of the family name and the face is printed using 2 steps
		 * because glcGetFontc and glcGetFontFace return their result in the
		 * same buffer (according to GLC specs).
		 */
		char prBuffer[1024];
		snprintf(prBuffer, sizeof(prBuffer), "Font #%d : %s ", (int)font, (const char*)glcGetFontc(font, GLC_FAMILY));
		prBuffer[sizeof(prBuffer) - 1] = 0;
		sstrcat(prBuffer, (char const *)glcGetFontFace(font));
		debug(LOG_NEVER, "%s", prBuffer);
	}
}

static void iV_initializeGLC(void)
{
	if (_glcContext)
	{
		return;
	}

	_glcContext = glcGenContext();
	if (!_glcContext)
	{
		debug(LOG_ERROR, "Failed to initialize");
	}
	else
	{
		debug(LOG_NEVER, "Successfully initialized. _glcContext = %d", (int)_glcContext);
	}

	glcContext(_glcContext);

	glcEnable(GLC_AUTO_FONT);		// We *do* want font fall-backs
	glcRenderStyle(GLC_TEXTURE);
	glcStringType(GLC_UTF8_QSO); // Set GLC's string type to UTF-8 FIXME should be UCS4 to avoid conversions

	#ifdef WZ_OS_MAC
	{
		char resourcePath[PATH_MAX];
		CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		if (CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8 *) resourcePath, PATH_MAX))
		{
			sstrcat(resourcePath, "/Fonts");
			glcAppendCatalog(resourcePath);
		}
		else
		{
			debug(LOG_ERROR, "Could not change to resources directory.");
		}

		if (resourceURL != NULL)
		{
			CFRelease(resourceURL);
		}
	}
	#endif

	_glcFont_Regular = glcGenFontID();
	_glcFont_Bold = glcGenFontID();

	if (!glcNewFontFromFamily(_glcFont_Regular, font_family))
	{
		debug(LOG_ERROR, "Failed to select font family %s as regular font", font_family);
	}
	else
	{
		debug(LOG_NEVER, "Successfully selected font family %s as regular font", font_family);
	}

	if (!glcFontFace(_glcFont_Regular, font_face_regular))
	{
		debug(LOG_WARNING, "Failed to select the \"%s\" font face of font family %s", font_face_regular, font_family);
	}
	else
	{
		debug(LOG_NEVER, "Successfully selected the \"%s\" font face of font family %s", font_face_regular, font_family);
	}

	if (!glcNewFontFromFamily(_glcFont_Bold, font_family))
	{
		debug(LOG_ERROR, "iV_initializeGLC: Failed to select font family %s for the bold font", font_family);
	}
	else
	{
		debug(LOG_NEVER, "Successfully selected font family %s for the bold font", font_family);
	}

	if (!glcFontFace(_glcFont_Bold, font_face_bold))
	{
		debug(LOG_WARNING, "Failed to select the \"%s\" font face of font family %s", font_face_bold, font_family);
	}
	else
	{
		debug(LOG_NEVER, "Successfully selected the \"%s\" font face of font family %s", font_face_bold, font_family);
	}

	debug(LOG_NEVER, "Finished initializing GLC");
}

void iV_TextInit()
{
	iV_initializeGLC();
	iV_SetFont(font_regular);

#ifdef DEBUG
	iV_printFontList();
#endif
}

void iV_TextShutdown()
{
	if (_glcFont_Regular)
	{
		glcDeleteFont(_glcFont_Regular);
	}

	if (_glcFont_Bold)
	{
		glcDeleteFont(_glcFont_Bold);
	}

	glcContext(0);

	if (_glcContext)
	{
		glcDeleteContext(_glcContext);
	}
}

void iV_SetFont(enum iV_fonts FontID)
{
	switch (FontID)
	{
		case font_scaled:
			iV_SetTextSize(12.f * pie_GetVideoBufferHeight() / 480);
			glcFont(_glcFont_Regular);
			break;

		default:
		case font_regular:
			iV_SetTextSize(12.f);
			glcFont(_glcFont_Regular);
			break;

		case font_large:
			iV_SetTextSize(21.f);
			glcFont(_glcFont_Bold);
			break;

		case font_small:
			iV_SetTextSize(9.f);
			glcFont(_glcFont_Regular);
			break;
	}
}

static inline float getGLCResolution(void)
{
	float resolution = glcGetf(GLC_RESOLUTION);

	// The default resolution as used by OpenGLC is 72 dpi
	if (resolution == 0.f)
	{
		return 72.f;
	}

	return resolution;
}

static inline float getGLCPixelSize(void)
{
	float pixel_size = font_size * getGLCResolution() / 72.f;
	return pixel_size;
}

static inline float getGLCPointWidth(const float* boundingbox)
{
	// boundingbox contains: [ xlb ylb xrb yrb xrt yrt xlt ylt ]
	// l = left; r = right; b = bottom; t = top;
	float rightTopX = boundingbox[4];
	float leftTopX = boundingbox[6];

	float point_width = rightTopX - leftTopX;

	return point_width;
}

static inline float getGLCPointHeight(const float* boundingbox)
{
	// boundingbox contains: [ xlb ylb xrb yrb xrt yrt xlt ylt ]
	// l = left; r = right; b = bottom; t = top;
	float leftBottomY = boundingbox[1];
	float leftTopY = boundingbox[7];

	float point_height = fabsf(leftTopY - leftBottomY);

	return point_height;
}

static inline float getGLCPointToPixel(float point_width)
{
	float pixel_width = point_width * getGLCPixelSize();

	return pixel_width;
}

unsigned int iV_GetTextWidth(const char* string)
{
	float boundingbox[8];
	float pixel_width, point_width;

	glcMeasureString(GL_FALSE, string);
	if (!glcGetStringMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the string \"%s\"", string);
		return 0;
	}

	point_width = getGLCPointWidth(boundingbox);
	pixel_width = getGLCPointToPixel(point_width);
	return (unsigned int)pixel_width;
}

unsigned int iV_GetCountedTextWidth(const char* string, size_t string_length)
{
	float boundingbox[8];
	float pixel_width, point_width;

	glcMeasureCountedString(GL_FALSE, string_length, string);
	if (!glcGetStringMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the string \"%s\" of length %u", string, (unsigned int)string_length);
		return 0;
	}

	point_width = getGLCPointWidth(boundingbox);
	pixel_width = getGLCPointToPixel(point_width);
	return (unsigned int)pixel_width;
}

unsigned int iV_GetTextHeight(const char* string)
{
	float boundingbox[8];
	float pixel_height, point_height;

	glcMeasureString(GL_FALSE, string);
	if (!glcGetStringMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the string \"%s\"", string);
		return 0;
	}

	point_height = getGLCPointHeight(boundingbox);
	pixel_height = getGLCPointToPixel(point_height);
	return (unsigned int)pixel_height;
}

unsigned int iV_GetCharWidth(uint32_t charCode)
{
	float boundingbox[8];
	float pixel_width, point_width;

	if (!glcGetCharMetric(charCode, GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character code %u", charCode);
		return 0;
	}

	point_width = getGLCPointWidth(boundingbox);
	pixel_width = getGLCPointToPixel(point_width);
	return (unsigned int)pixel_width;
}

int iV_GetTextLineSize()
{
	float boundingbox[8];
	float pixel_height, point_height;

	if (!glcGetMaxCharMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character");
		return 0;
	}

	point_height = getGLCPointHeight(boundingbox);
	pixel_height = getGLCPointToPixel(point_height);
	return (unsigned int)pixel_height;
}

static float iV_GetMaxCharBaseY(void)
{
	float base_line[4]; // [ xl yl xr yr ]

	if (!glcGetMaxCharMetric(GLC_BASELINE, base_line))
	{
		debug(LOG_ERROR, "Couldn't retrieve the baseline for the character");
		return 0;
	}

	return base_line[1];
}

int iV_GetTextAboveBase(void)
{
	float point_base_y = iV_GetMaxCharBaseY();
	float point_top_y;
	float boundingbox[8];
	float pixel_height, point_height;

	if (!glcGetMaxCharMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character");
		return 0;
	}

	point_top_y = boundingbox[7];
	point_height = point_base_y - point_top_y;
	pixel_height = getGLCPointToPixel(point_height);
	return (int)pixel_height;
}

int iV_GetTextBelowBase(void)
{
	float point_base_y = iV_GetMaxCharBaseY();
	float point_bottom_y;
	float boundingbox[8];
	float pixel_height, point_height;

	if (!glcGetMaxCharMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character");
		return 0;
	}

	point_bottom_y = boundingbox[1];
	point_height = point_bottom_y - point_base_y;
	pixel_height = getGLCPointToPixel(point_height);
	return (int)pixel_height;
}

void iV_SetTextColour(PIELIGHT colour)
{
	font_colour[0] = colour.byte.r / 255.0f;
	font_colour[1] = colour.byte.g / 255.0f;
	font_colour[2] = colour.byte.b / 255.0f;
	font_colour[3] = colour.byte.a / 255.0f;
}

/** Draws formatted text with word wrap, long word splitting, embedded newlines
 *  (uses '@' rather than '\n') and colour toggle mode ('#') which enables or
 *  disables font colouring.
 *
 *  @param String   the string to display.
 *  @param x,y      X and Y coordinates of top left of formatted text.
 *  @param width    the maximum width of the formatted text (beyond which line
 *                  wrapping is used).
 *  @param justify  The alignment style to use, which is one of the following:
 *                  FTEXT_LEFTJUSTIFY, FTEXT_CENTRE or FTEXT_RIGHTJUSTIFY.
 *  @return the Y coordinate for the next text line.
 */
int iV_DrawFormattedText(const char* String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify)
{
	std::string FString;
	std::string FWord;
	int i;
	int jx = x;		// Default to left justify.
	int jy = y;
	UDWORD WWidth;
	int TWidth;
	const char* curChar = String;

	while (*curChar != 0)
	{
		bool GotSpace = false;
		bool NewLine = false;

		// Reset text draw buffer
		FString.clear();

		WWidth = 0;

		// Parse through the string, adding words until width is achieved.
		while (*curChar != 0 && WWidth < Width && !NewLine)
		{
			const char* startOfWord = curChar;
			const unsigned int FStringWidth = iV_GetTextWidth(FString.c_str());

			// Get the next word.
			i = 0;
			FWord.clear();
			for (; *curChar != 0
			    && *curChar != ASCII_SPACE
			    && *curChar != ASCII_NEWLINE
			    && *curChar != '\n';
			     ++i, ++curChar)
			{
				if (*curChar == ASCII_COLOURMODE) // If it's a colour mode toggle char then just add it to the word.
				{
					FWord.push_back(*curChar);

					// this character won't be drawn so don't deal with its width
					continue;
				}

				// Update this line's pixel width.
				//WWidth = FStringWidth + iV_GetCountedTextWidth(FWord.c_str(), i + 1);  // This triggers tonnes of valgrind warnings, if the string contains unicode. Adding lots of trailing garbage didn't help... Using iV_GetTextWidth with a null-terminated string, instead.
				WWidth = FStringWidth + iV_GetTextWidth(FWord.c_str());

				// If this word doesn't fit on the current line then break out
				if (WWidth > Width)
				{
					break;
				}

				// If width ok then add this character to the current word.
				FWord.push_back(*curChar);
			}

			// Don't forget the space.
			if (*curChar == ASCII_SPACE)
			{
				// Should be a space below, not '-', but need to work around bug in QuesoGLC
				// which was fixed in CVS snapshot as of 2007/10/26, same day as I reported it :) - Per
				WWidth += iV_GetCharWidth('-');
				if (WWidth <= Width)
				{
					FWord.push_back(' ');
					++i;
					++curChar;
					GotSpace = true;
				}
			}
			// Check for new line character.
			else if (*curChar == ASCII_NEWLINE
			      || *curChar == '\n')
			{
				if (!bMultiPlayer)
				{
					NewLine = true;
				}
				++curChar;
			}

			// If we've passed a space on this line and the word goes past the
			// maximum width and this isn't caused by the appended space then
			// rewind to the start of this word and finish this line.
			if (GotSpace
			 && WWidth > Width
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

		TWidth = iV_GetTextWidth(FString.c_str());

		// Do justify.
		switch (Justify)
		{
			case FTEXT_CENTRE:
				jx = x + (Width - TWidth) / 2;
				break;

			case FTEXT_RIGHTJUSTIFY:
				jx = x + Width - TWidth;
				break;

			case FTEXT_LEFTJUSTIFY:
				jx = x;
				break;
		}

		// draw the text.
		iV_DrawText(FString.c_str(), jx, jy);

		// and move down a line.
		jy += iV_GetTextLineSize();
	}

	return jy;
}

void iV_DrawTextRotated(const char* string, float XPos, float YPos, float rotation)
{
	GLint matrix_mode = 0;
	ASSERT_OR_RETURN( , string, "Couldn't render string!");
	pie_SetTexturePage(TEXPAGE_EXTERN);

	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if (rotation != 0.f)
	{
		rotation = 360.f - rotation;
	}

	glTranslatef(XPos, YPos, 0.f);
	glRotatef(180.f, 1.f, 0.f, 0.f);
	glRotatef(rotation, 0.f, 0.f, 1.f);
	glScalef(font_size, font_size, 0.f);

	glColor4fv(font_colour);

	glFrontFace(GL_CW);
	glcRenderString(string);
	glFrontFace(GL_CCW);

	glPopMatrix();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(matrix_mode);

	// Reset the current model view matrix
	glLoadIdentity();
}

static void iV_DrawTextRotatedFv(float x, float y, float rotation, const char* format, va_list ap)
{
	va_list aq;
	size_t size;
	char* str;

	/* Required because we're using the va_list ap twice otherwise, which
	 * results in undefined behaviour. See stdarg(3) for details.
	 */
	va_copy(aq, ap);

	// Allocate a buffer large enough to hold our string on the stack
	size = vsnprintf(NULL, 0, format, ap);
	str = (char *)alloca(size + 1);

	// Print into our newly created string buffer
	vsprintf(str, format, aq);

	va_end(aq);

	// Draw the produced string to the screen at the given position and rotation
	iV_DrawTextRotated(str, x, y, rotation);
}

void iV_DrawTextF(float x, float y, const char* format, ...)
{
	va_list ap;

	va_start(ap, format);
		iV_DrawTextRotatedFv(x, y, 0.f, format, ap);
	va_end(ap);
}

void iV_SetTextSize(float size)
{
	font_size = size;
}