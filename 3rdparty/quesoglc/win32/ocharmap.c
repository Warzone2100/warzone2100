/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2007, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id: ocharmap.c,v 1.17 2007/02/25 13:30:48 bcoconni Exp $ */

/** \file
 * defines the object __GLCcharMap which manage the charmaps of both the fonts
 * and the masters. One of the purpose of this object is to encapsulate the
 * GLYPHSET structure from Win32 GDI and to add it some more functionalities.
 * It also allows to centralize the character map management for easier
 * maintenance.
 */

/* QuesoGLC needs Windows 2000 or newer */
#define _WIN32_WINNT 0x0500
#include "internal.h"



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the initial GLYPHSET of the font or the master (which
 * may be NULL) in which case the character map will be empty.
 */
__GLCcharMap* __glcCharMapCreate(__GLCmaster* inMaster)
{
  __GLCcharMap* This = NULL;

  This = (__GLCcharMap*)__glcMalloc(sizeof(__GLCcharMap));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (inMaster) {
    HFONT font;
    DWORD size;
    HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    if (FAILED(dc)) {
      __glcFree(This);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    font = CreateFontIndirect(&inMaster->pattern->elfLogFont);
    if (FAILED(font)) {
      DeleteDC(dc);
      __glcFree(This);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    if (FAILED(SelectObject(dc, font))) {
      DeleteDC(dc);
      DeleteObject(font);
      __glcFree(This);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    size = GetFontUnicodeRanges(dc, NULL);
    if (!size) {
      DeleteDC(dc);
      DeleteObject(font);
      __glcFree(This);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    This->charSet = (LPGLYPHSET)__glcMalloc(size);
    if (!This->charSet) {
      DeleteDC(dc);
      DeleteObject(font);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcFree(This);
      return NULL;
    }
    if (!GetFontUnicodeRanges(dc, This->charSet)) {
      DeleteDC(dc);
      DeleteObject(font);
      __glcFree(This->charSet);
      __glcFree(This);
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
    DeleteDC(dc);
    DeleteObject(font);
  }
  else {
    This->charSet = (LPGLYPHSET)__glcMalloc(sizeof(GLYPHSET));
    if (!This->charSet) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      __glcFree(This);
      return NULL;
    }
  }

  /* The array 'map' will contain the actual character map */
  This->map = __glcArrayCreate(sizeof(__GLCcharMapElement));
  if (!This->map) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This->charSet);
    __glcFree(This);
    return NULL;
  }

  return This;
}



/* Destructor of the object */
void __glcCharMapDestroy(__GLCcharMap* This)
{
  if (This->map)
    __glcArrayDestroy(This->map);

  __glcFree(This->charSet);
  __glcFree(This);
}



/* Add a given character to the character map. Afterwards, the character map
 * will associate the glyph 'inGlyph' to the Unicode codepoint 'inCode'.
 */
void __glcCharMapAddChar(__GLCcharMap* This, GLint inCode, __GLCglyph* inGlyph)
{
  __GLCcharMapElement* element = NULL;
  __GLCcharMapElement* newElement = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));
  assert(inCode >= 0);

  /* Characters are stored by ascending order of their mapped code */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Parse the array by dichotomy to look for the place where to add the new
   * character.
   */
  while (start <= end) {
    middle = (start + end) >> 1;
    /* If the character map already contains the new character then update the
     * glyph then return.
     */
    if (element[middle].mappedCode == (GLCulong)inCode) {
      element[middle].glyph = inGlyph;
      return;
    }
    else if (element[middle].mappedCode > (GLCulong)inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  /* If we have reached the end of the array then updated the rank 'middle'
   * accordingly.
   */
  if ((end >= 0) && (element[middle].mappedCode < (GLCulong)inCode))
    middle++;

  /* Insert the new character in the character map */
  newElement = (__GLCcharMapElement*)__glcArrayInsertCell(This->map, middle, 1);
  if (!newElement)
    return;

  newElement->mappedCode = inCode;
  newElement->glyph = inGlyph;
  return;
}



/* Remove a character from the character map */
void __glcCharMapRemoveChar(__GLCcharMap* This, GLint inCode)
{
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));
  assert(inCode >= 0);

  /* Characters are stored by ascending order of their mapped code */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Parse the array by dichotomy to look for the place where to add the new
   * character.
   */
  while (start <= end) {
    middle = (start + end) >> 1;
    /* When the character is found remove it from the array and return */
    if (element[middle].mappedCode == (GLCulong)inCode) {
      __glcArrayRemove(This->map, middle);
      break;
    }
    else if (element[middle].mappedCode > (GLCulong)inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }
}



/* Get the Unicode character name of the character which codepoint is inCode.
 * Note : since the character maps of the fonts can be altered, this function
 * can return 'LATIN CAPITAL LETTER B' whereas inCode contained 65 (which is
 * the Unicode code point of 'LATIN CAPITAL LETTER A'.
 */
GLCchar* __glcCharMapGetCharName(__GLCcharMap* This, GLint inCode,
				 __GLCcontext* inContext)
{
  GLCchar *buffer = NULL;
  GLCchar8* name = NULL;
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));
  assert(inCode >= 0);

  /* Characters are stored by ascending order of their mapped code */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Parse the array by dichotomy to look for the Unicode codepoint that the
   * request character maps to.
   */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (element[middle].mappedCode == (GLCulong)inCode) {
      inCode = element[middle].glyph->codepoint;
      break;
    }
    else if (element[middle].mappedCode > (GLCulong)inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  /* If we have not found the character in the character map, it means that
   * the mapped code is equal to 'inCode' otherwise inCode is modified to
   * contain the mapped code.
   */
  name = __glcNameFromCode(inCode);
  if (!name) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  /* Convert the Unicode to the current string type */
  buffer = __glcConvertFromUtf8ToBuffer(inContext, name,
					inContext->stringState.stringType);
  if (!buffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GLC_NONE;
  }

  return buffer;
}



/* Get the glyph corresponding to codepoint 'inCode' */
__GLCglyph* __glcCharMapGetGlyph(__GLCcharMap* This, GLint inCode)
{
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));
  assert(inCode >= 0);

  /* Characters are stored by ascending order of their mapped code */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Parse the array by dichotomy to find the glyph of the requested
   * character.
   */
  while (start <= end) {
    middle = (start + end) >> 1;
    if (element[middle].mappedCode == (GLCulong)inCode)
      /* When the character is found return the corresponding glyph */
      return element[middle].glyph;
    else if (element[middle].mappedCode > (GLCulong)inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  /* No glyph has been defined yet for the requested character */
  return NULL;
}



/* Check if a character is in the character map */
GLboolean __glcCharMapHasChar(__GLCcharMap* This, GLint inCode)
{
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));
  assert(inCode >= 0);

  /* Characters are stored by ascending order of their mapped code */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);

  end = GLC_ARRAY_LENGTH(This->map) - 1;

  /* Parse the array by dichotomy to find the requested character. */
  while (start <= end) {
    middle = (start + end) >> 1;
    /* The character has been found : return GL_TRUE */
    if (element[middle].mappedCode == (GLCulong)inCode)
      return GL_TRUE;
    else if (element[middle].mappedCode > (GLCulong)inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  /* Check if the character identified by inCode exists in the font */
  if (This->charSet->cGlyphsSupported) {
    DWORD i = 0;
    LPGLYPHSET charSet = This->charSet;
    LPWCRANGE range = charSet->ranges;

    for (; i < charSet->cRanges; i++, range++) {
      if (inCode > (range->wcLow + range->cGlyphs - 1))
	continue;
      if (inCode >= range->wcLow)
	return GL_TRUE;
    }
  }
  return GL_FALSE;
}



/* Get the name of the character which is stored at rank 'inIndex' in the
 * GLYPHSET of the face.
 */
GLCchar* __glcCharMapGetCharNameByIndex(__GLCcharMap* This, GLint inIndex,
					__GLCcontext* inContext)
{
  LPGLYPHSET charSet = This->charSet;

  assert(inIndex >= 0);

  if (charSet->cGlyphsSupported) {
    DWORD i = 0;
    LPWCRANGE range = charSet->ranges;
    GLCchar8* name = NULL;
    GLCchar* buffer = NULL;

    for (; i < charSet->cRanges; i++, range++) {
      if (inIndex > range->cGlyphs) {
	inIndex -= range->cGlyphs;
	continue;
      }

      /* Get the character name */
      name = __glcNameFromCode(range->wcLow + inIndex);

      if (!name) {
        __glcRaiseError(GLC_PARAMETER_ERROR);
        return GLC_NONE;
      }

      /* Performs the conversion to the current string type */
      buffer = __glcConvertFromUtf8ToBuffer(inContext, name,
					    inContext->stringState.stringType);
      if (!buffer) {
        __glcRaiseError(GLC_RESOURCE_ERROR);
        return GLC_NONE;
      }

      return buffer;
    }
  }

  /* The character has not been found */
  __glcRaiseError(GLC_PARAMETER_ERROR);
  return GLC_NONE;
}



/* Return the number of characters in the character map */
GLint __glcCharMapGetCount(__GLCcharMap* This)
{
  return This->charSet->cGlyphsSupported;
}



/* Get the maximum mapped code of a character set */
GLint __glcCharMapGetMaxMappedCode(__GLCcharMap* This)
{
  DWORD i = 0;
  LPGLYPHSET charSet = This->charSet;
  LPWCRANGE range = charSet->ranges;
  GLCulong maxMappedCode = 0;
  __GLCcharMapElement* element = NULL;
  int length = 0;

  assert(charSet->cGlyphsSupported);
  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  for (; i < charSet->cRanges; i++, range++) {
    WCHAR maxCode = range->wcLow + range->cGlyphs - 1;

    maxMappedCode < maxCode ? maxCode : maxMappedCode;
  }

  /* Check that a code greater than the one found in the GLYPHSET is not
   * stored in the array 'map'.
   */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);
  length = GLC_ARRAY_LENGTH(This->map);

  /* Return the greater of the code of both the GLYPHSET and the array 'map'*/
  if (length)
    return element[length-1].mappedCode > maxMappedCode ?
      element[length-1].mappedCode : maxMappedCode;
  else
    return maxMappedCode;
}



/* Get the minimum mapped code of a character set */
GLint __glcCharMapGetMinMappedCode(__GLCcharMap* This)
{
  DWORD i = 0;
  LPGLYPHSET charSet = This->charSet;
  LPWCRANGE range = charSet->ranges;
  GLCulong minMappedCode = 0xffffffff;
  __GLCcharMapElement* element = NULL;
  int length = 0;

  assert(charSet->cGlyphsSupported);
  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  for (; i < charSet->cRanges; i++, range++)
    minMappedCode > range->wcLow ? range->wcLow : minMappedCode;

  /* Check that a code lower than the one found in the GLYPHSET is not
   * stored in the array 'map'.
   */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);
  length = GLC_ARRAY_LENGTH(This->map);

  /* Return the lower of the code of both the GLYPHSET and the array 'map' */
  if (length > 0)
    return element[0].mappedCode < minMappedCode ?
      element[0].mappedCode : minMappedCode;
  else
    return minMappedCode;
}
