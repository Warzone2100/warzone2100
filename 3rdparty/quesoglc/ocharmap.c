/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2008, Bertrand Coconnier
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
/* $Id$ */

/** \file
 * defines the object __GLCcharMap which manage the charmaps of both the fonts
 * and the masters. One of the purpose of this object is to encapsulate the
 * FcCharSet structure from Fontconfig and to add it some more functionalities.
 * It also allows to centralize the character map management for easier
 * maintenance.
 */

#include "internal.h"



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the FcPattern of the font or the master (which may be NULL
 * in which case the character map will be empty).
 */
__GLCcharMap* __glcCharMapCreate(const __GLCmaster* inMaster,
				 const __GLCcontext* inContext)
{
  __GLCcharMap* This = NULL;

  assert(inContext);

  This = (__GLCcharMap*)__glcMalloc(sizeof(__GLCcharMap));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCcharMap));

  This->charSet = FcCharSetCreate();
  if (!This->charSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  if (inMaster) {
    FcCharSet* charSet = NULL;
    FcFontSet* fontSet = NULL;
    int i = 0;
    FcObjectSet* objectSet = NULL;
    FcPattern* pattern = FcPatternCreate();

    if (!pattern) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcCharSetDestroy(This->charSet);
      __glcFree(This);
      return NULL;
    }

    objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_SPACING, FC_OUTLINE,
				 FC_CHARSET, NULL);
    if (!objectSet) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcPatternDestroy(pattern);
      FcCharSetDestroy(This->charSet);
      __glcFree(This);
      return NULL;
    }

    fontSet = FcFontList(inContext->config, pattern, objectSet);
    FcObjectSetDestroy(objectSet);
    FcPatternDestroy(pattern);
    if (!fontSet) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      FcCharSetDestroy(This->charSet);
      __glcFree(This);
      return NULL;
    }

    for (i = 0; i < fontSet->nfont; i++) {
      FcChar8* family = NULL;
      int fixed = 0;
      FcChar8* foundry = NULL;
      FcBool outline = FcFalse;
      FcBool equal = FcFalse;
#ifdef DEBUGMODE
      FcResult result = FcResultMatch;

      result = FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
      assert(result != FcResultTypeMismatch);
#else
      FcPatternGetBool(fontSet->fonts[i], FC_OUTLINE, 0, &outline);
#endif

      /* Check whether the glyphs are outlines */
      if (!outline)
	continue;

#ifdef DEBUGMODE
      result = FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &family);
      assert(result != FcResultTypeMismatch);
      result = FcPatternGetString(fontSet->fonts[i], FC_FOUNDRY, 0, &foundry);
      assert(result != FcResultTypeMismatch);
      result = FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed);
      assert(result != FcResultTypeMismatch);
#else
      FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &family);
      FcPatternGetString(fontSet->fonts[i], FC_FOUNDRY, 0, &foundry);
      FcPatternGetInteger(fontSet->fonts[i], FC_SPACING, 0, &fixed);
#endif

      if (foundry)
	pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family,
				 FC_FOUNDRY, FcTypeString, foundry, FC_SPACING,
				 FcTypeInteger, fixed, NULL);
      else
	pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family,
				 FC_SPACING, FcTypeInteger, fixed, NULL);

      if (!pattern) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	FcCharSetDestroy(This->charSet);
	FcFontSetDestroy(fontSet);
	__glcFree(This);
	return NULL;
      }

      equal = FcPatternEqual(pattern, inMaster->pattern);
      FcPatternDestroy(pattern);
      if (equal) {
        FcCharSet* newCharSet = NULL;

#ifdef DEBUGMODE
        result = FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0,
				     &charSet);
        assert(result != FcResultTypeMismatch);
#else
	FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &charSet);
#endif

        newCharSet = FcCharSetUnion(This->charSet, charSet);
	if (!newCharSet) {
          __glcRaiseError(GLC_RESOURCE_ERROR);
          FcCharSetDestroy(This->charSet);
          FcFontSetDestroy(fontSet);
          __glcFree(This);
          return NULL;
	}

	FcCharSetDestroy(This->charSet);
	This->charSet = newCharSet;
      }
    }

    FcFontSetDestroy(fontSet);
  }

  /* The array 'map' will contain the actual character map */
  This->map = __glcArrayCreate(sizeof(__GLCcharMapElement));
  if (!This->map) {
    FcCharSetDestroy(This->charSet);
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

  FcCharSetDestroy(This->charSet);

  __glcFree(This);
}



/* Add a given character to the character map. Afterwards, the character map
 * will associate the glyph 'inGlyph' to the Unicode codepoint 'inCode'.
 */
void __glcCharMapAddChar(__GLCcharMap* This, const GLint inCode,
			 __GLCglyph* inGlyph)
{
  __GLCcharMapElement* element = NULL;
  __GLCcharMapElement* newElement = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This);
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

  /* If we have reached the end of the array then update the rank 'middle'
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
void __glcCharMapRemoveChar(__GLCcharMap* This, const GLint inCode)
{
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This);
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
 * the Unicode code point of 'LATIN CAPITAL LETTER A').
 */
const GLCchar8* __glcCharMapGetCharName(const __GLCcharMap* This,
					const GLint inCode)
{
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;
  GLint code = 0;

  assert(This);
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
      code = element[middle].glyph->codepoint;
      break;
    }
    else if (element[middle].mappedCode > (GLCulong)inCode)
      end = middle - 1;
    else
      start = middle + 1;
  }

  if (!code) {
    if (FcCharSetHasChar(This->charSet, inCode))
      code = inCode;
    else
      return NULL;
  }

  return __glcNameFromCode(code);
}



/* Get the glyph corresponding to codepoint 'inCode' */
__GLCglyph* __glcCharMapGetGlyph(const __GLCcharMap* This, const GLint inCode)
{
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This);
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
GLboolean __glcCharMapHasChar(const __GLCcharMap* This, const GLint inCode)
{
  __GLCcharMapElement* element = NULL;
  int start = 0, middle = 0, end = 0;

  assert(This);
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
  return FcCharSetHasChar(This->charSet, inCode);
}



/* This function counts the number of bits that are set in c1 
 * Copied from Keith Packard's fontconfig
 */
static GLCchar32 __glcCharSetPopCount(const GLCchar32 c1)
{
  /* hackmem 169 */
  GLCchar32    c2 = (c1 >> 1) & 033333333333;
  c2 = c1 - c2 - ((c2 >> 1) & 033333333333);
  return (((c2 + (c2 >> 3)) & 030707070707) % 077);
}



/* Get the name of the character which is stored at rank 'inIndex' in the
 * FcCharSet of the face.
 */
const GLCchar8* __glcCharMapGetCharNameByIndex(const __GLCcharMap* This,
					       const GLint inIndex)
{
  int i = 0;
  int j = 0;

  /* In Fontconfig the map in FcCharSet is organized as an array of integers.
   * Each integer corresponds to a page of 32 characters (since it uses 32 bits
   * integer). If a bit is set then the corresponding character is in the
   * character map otherwise it is not.
   * In order not to store pages of 0's, the character map begins at the
   * character which codepoint is 'base'. 
   * Pages are also gathered in blocks of 'FC_CHARSET_MAP_SIZE' pages in order
   * to prevent Fontconfig to store heaps of 0's if the character codes are
   * sparsed.
   *
   * The codepoint of a character located at bit 'j' of page 'i' is :
   * 'base + (i << 5) + j'.
   */
  GLCchar32 map[FC_CHARSET_MAP_SIZE];
  GLCchar32 next = 0;
  GLCchar32 base = 0;
  GLCchar32 count = 1;
  GLCchar32 value = 0;

  assert(This);
  assert(inIndex >= 0);

  base = FcCharSetFirstPage(This->charSet, map, &next);

  do {
    /* Parse the pages in FcCharSet */
    for (i = 0; i < FC_CHARSET_MAP_SIZE; i++) {
      /* Get the number of character located in the current page */
      value = __glcCharSetPopCount(map[i]);

      /* Check if the character we are looking for is in the current page */
      if (count + value >= (GLCchar32)inIndex) {
	for (j = 0; j < 32; j++) {
	  /* Parse the page bit by bit */
	  if ((map[i] >> j) & 1) {
	    count++; /* A character is set at bit j */
	    /* Check if we have reached the rank inIndex */
	    if (count == (GLCchar32)inIndex) {
	      /* Get the character name */
	      return __glcNameFromCode(base + (i << 5) + j);
	    }
	  }
	}
      }
      /* Add the number of characters of the current page to the count and
       * check the next page.
       */
      count += value;
    }
    /* The current block is finished, check the next one */
    base = FcCharSetNextPage(This->charSet, map, &next);
  } while (base != FC_CHARSET_DONE);

  /* The character has not been found */
  __glcRaiseError(GLC_PARAMETER_ERROR);
  return GLC_NONE;
}



/* Get the maximum mapped code of a character set */
GLint __glcCharMapGetMaxMappedCode(const __GLCcharMap* This)
{
  GLCchar32 base = 0;
  GLCchar32 next = 0;
  GLCchar32 prev_base = 0;
  GLCchar32 map[FC_CHARSET_MAP_SIZE];
  int i = 0, j = 0;
  GLCulong maxMappedCode = 0;
  __GLCcharMapElement* element = NULL;
  int length = 0;

  assert(This);
  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  /* Look for the last block of pages of the FcCharSet structure */
  base = FcCharSetFirstPage(This->charSet, map, &next);
  assert(base != FC_CHARSET_DONE);

  do {
    prev_base = base;
    base = FcCharSetNextPage(This->charSet, map, &next);
  } while (base != FC_CHARSET_DONE);

  /* Parse the pages in descending order to find the last page that contains
   * one character.
   */
  for (i = FC_CHARSET_MAP_SIZE - 1; i >= 0; i--)
    if (map[i]) break;

  /* If the map contains no char then something went wrong... */
  assert(i >= 0); 

  /* Parse the bits of the last page in descending order to find the last
   * character of the page
   */
  for (j = 31; j >= 0; j--)
    if ((map[i] >> j) & 1) break;

  assert(j >= 0);

  /* Calculate the max mapped code */
  maxMappedCode = prev_base + (i << 5) + j;

  /* Check that a code greater than the one found in the FcCharSet is not
   * stored in the array 'map'.
   */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);
  length = GLC_ARRAY_LENGTH(This->map);

  /* Return the greater of the code of both the FcCharSet and the array 'map'*/
  if (length)
    return element[length-1].mappedCode > maxMappedCode ?
      element[length-1].mappedCode : maxMappedCode;
  else
    return maxMappedCode;
}



/* Get the minimum mapped code of a character set */
GLint __glcCharMapGetMinMappedCode(const __GLCcharMap* This)
{
  GLCchar32 base = 0;
  GLCchar32 next = 0;
  GLCchar32 map[FC_CHARSET_MAP_SIZE];
  int i = 0, j = 0;
  GLCulong minMappedCode = 0xffffffff;
  __GLCcharMapElement* element = NULL;
  int length = 0;

  assert(This);
  assert(This->map);
  assert(GLC_ARRAY_DATA(This->map));

  /* Get the first block of pages of the FcCharSet structure */
  base = FcCharSetFirstPage(This->charSet, map, &next);
  assert(base != FC_CHARSET_DONE);

  /* Parse the pages in ascending order to find the first page that contains
   * one character.
   */
  for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
    if (map[i]) break;

  /* If the map contains no char then something went wrong... */
  assert(i >= 0); 

  /* Parse the bits of the first page in ascending order to find the first
   * character of the page
   */
  for (j = 0; j < 32; j++)
    if ((map[i] >> j) & 1) break;

  assert(j < 32);
  minMappedCode = base + (i << 5) + j;

  /* Check that a code lower than the one found in the FcCharSet is not
   * stored in the array 'map'.
   */
  element = (__GLCcharMapElement*)GLC_ARRAY_DATA(This->map);
  length = GLC_ARRAY_LENGTH(This->map);

  /* Return the lower of the code of both the FcCharSet and the array 'map'*/
  if (length)
    return element[0].mappedCode < minMappedCode ?
      element[0].mappedCode : minMappedCode;
  else
    return minMappedCode;
}
