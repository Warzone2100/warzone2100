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
/* $Id$ */

/** \file
 * defines the object __GLCmaster which manage the masters
 */

/* QuesoGLC needs Windows 2000 or newer */
#define _WIN32_WINNT 0x0500
#include "internal.h"



typedef struct __GLCdataRec __GLCdata;

struct __GLCdataRec {
  __GLCarray* hashTable;
  __GLCmaster* master;
  __GLCcharMap* charMap;
  void* ptr;
  GLint index;
};



/* Hash function of strings adapted from "Compilers : Principles, Techniques and
 * Tools - Alfred V. Aho, Ravi Sethi and Jeffrey D. Ullman"
 */
GLCchar32 __glcHashValue(LPLOGFONT inLogFont)
{
  TCHAR* ptr = inLogFont->lfFaceName;
  GLCchar32 tmp = 0;
  GLCchar32 hashValue = 0;

  while(*ptr) {
    hashValue = (hashValue << 4) + (*ptr);
    tmp = hashValue & 0xf0000000;
    if (tmp) {
      hashValue = hashValue ^ (tmp >> 24);
      hashValue = hashValue ^ tmp;
    }
    ptr++;
  }

  return hashValue;
}



static int CALLBACK __glcEnumMasterCreate(ENUMLOGFONTEX* inElfe,
                                          NEWTEXTMETRICEX* inNtme,
                                          DWORD inFontType, LPARAM inData)
{
  __GLCarray* masterHashTable = ((__GLCdata*)inData)->hashTable;
  GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(masterHashTable);
  int length = GLC_ARRAY_LENGTH(masterHashTable);
  int i = 0;
  GLCchar32 hashValue = 0;

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  hashValue = __glcHashValue(&inElfe->elfLogFont);

  /* Check if the font is already registered in the hash table */
  for (i = 0; i < length; i++) {
    if (hashTable[i] == hashValue) {
      __GLCmaster* master = ((__GLCdata*)inData)->master;
      memcpy(master->pattern, &inElfe->elfLogFont, sizeof(ENUMLOGFONTEX));
      return 0;
    }
  }

  return 1;
}



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 */
__GLCmaster* __glcMasterCreate(GLint inMaster, __GLCcontext* inContext)
{
  __GLCmaster* This = NULL;
  __GLCdata data = {NULL, NULL, NULL, NULL, 0};
  LOGFONT lfont;
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    DeleteDC(dc);
    return NULL;
  }

  data.hashTable = inContext->masterHashTable;
  data.master = This;

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumMasterCreate,
                     (LPARAM)&data, 0);

  DeleteDC(dc);
  return This;
}



/* Destructor of the object */
void __glcMasterDestroy(__GLCmaster* This)
{
  __glcFree(This->pattern);
  __glcFree(This);
}



/* Merge the character map 'inCharMap' in the character map 'This' */
static GLboolean __glcCharMapUnion(__GLCcharMap* This, LPGLYPHSET inCharSet)
{
  LPGLYPHSET result = NULL;
  LPGLYPHSET charSet = This->charSet;
  DWORD i = 0;
  DWORD resultSize = 0;
  LPWCRANGE range = NULL;

  assert(charSet);
  assert(inCharSet);

  if (!inCharSet->cGlyphsSupported)
    return GL_TRUE;

  if (!charSet->cGlyphsSupported)
    resultSize = inCharSet->cbThis;
  else
    resultSize = charSet->cbThis + inCharSet->cRanges * sizeof(WCRANGE);

  result = (LPGLYPHSET)__glcMalloc(resultSize);
  if (!result) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  if (!charSet->cGlyphsSupported) {
    memcpy(result, inCharSet, inCharSet->cbThis);
    __glcFree(charSet);
    This->charSet = result;
    return GL_TRUE;
  }
  else {
    memcpy(result, charSet, charSet->cbThis);
    result->cbThis = resultSize;
  }

  for (; i < inCharSet->cRanges; i++) {
    DWORD j = 0;
    LPWCRANGE range2 = result->ranges;
    WCRANGE temp;

    range = inCharSet->ranges + i;

    for (; j < result->cRanges; j++, range2++) {
      if (range->wcLow > (range2->wcLow + range2->cGlyphs -1))
	continue;
      if ((range->wcLow + range->cGlyphs - 1) < range2->wcLow) {
	memcpy(range2+1, range2, (result->cRanges - j) * sizeof(WCRANGE));
	range2->wcLow = range->wcLow;
	range2->cGlyphs = range2->cGlyphs;
	result->cRanges++;
	break;
      }
      if ((range->wcLow >= range2->wcLow) && ((range->wcLow + range->cGlyphs) <= (range2->wcLow + range2->cGlyphs)))
	break;
      if ((range->wcLow < range2->wcLow) &&  ((range->wcLow + range->cGlyphs) <= (range2->wcLow + range2->cGlyphs))) {
	range2->cGlyphs = range2->wcLow + range2->cGlyphs - range->wcLow;
	range2->wcLow = range->wcLow;
	break;
      }
      temp.wcLow = (range->wcLow < range2->wcLow) ? range->wcLow : range2->wcLow;
      temp.cGlyphs = range2->wcLow + range2->cGlyphs - temp.wcLow;
      range = &temp;
      if (result->cRanges-j-1) {
	memcpy(range2-1, range2, (result->cRanges-j-1)*sizeof(WCRANGE));
	result->cRanges--;
        j--;
        range2--;
      }
    }

    if (j == result->cRanges) {
      range2->wcLow = range->wcLow;
      range2->cGlyphs = range->cGlyphs;
      result->cRanges++;
    }
  }

  range = result->ranges;
  result->cGlyphsSupported = 0;

  for (i = 0; i < result->cRanges; i++, range++)
    result->cGlyphsSupported += range->cGlyphs;

  /* Destroy the previous GLYPHSET and replace it by the new one */
  __glcFree(This->charSet);
  This->charSet = result;

  return GL_TRUE;
}



static int CALLBACK __glcEnumMasterGetCharMap(ENUMLOGFONTEX* inElfe,
                                              NEWTEXTMETRICEX* inNtme,
                                              DWORD inFontType, LPARAM inData)
{
  __GLCdata* data = (__GLCdata*)inData;
  GLCchar32 hashValue = GLC_MASTER_HASH_VALUE(data->master);
  HFONT font;
  DWORD size;
  HDC dc;
  LPGLYPHSET charSet = NULL;

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  if (hashValue != __glcHashValue(&inElfe->elfLogFont))
    return 1;

  dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  font = CreateFontIndirect(&inElfe->elfLogFont);
  if (FAILED(font)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    DeleteDC(dc);
    return 0;
  }

  if (FAILED(SelectObject(dc, font))) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  size = GetFontUnicodeRanges(dc, NULL);
  if (!size) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  charSet = (LPGLYPHSET)__glcMalloc(size);
  if (!charSet) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  if (!GetFontUnicodeRanges(dc, charSet)) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcFree(charSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }
  DeleteDC(dc);
  DeleteObject(font);
  __glcCharMapUnion(data->charMap, charSet);
  __glcFree(charSet);

  return 1;
}



/* Extract the charmap of the master */
__GLCcharMap* __glcMasterGetCharMap(__GLCmaster* This, __GLCcontext* inContext)
{
  LOGFONT lfont;
  __GLCdata data = {NULL, NULL, NULL, NULL, 0};
  __GLCcharMap* charMap = NULL;
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  charMap = __glcCharMapCreate(This);
  if (!charMap) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    DeleteDC(dc);
    return NULL;
  }

  data.master = This;
  data.charMap = charMap;

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumMasterGetCharMap,
                     (LPARAM)&data, 0);

  DeleteDC(dc);
  return charMap;
}



static int CALLBACK __glcEnumMasterGetFaceName(ENUMLOGFONTEX* inElfe,
                                               NEWTEXTMETRICEX* inNtme,
                                               DWORD inFontType, LPARAM inData)
{
  __GLCdata* data = (__GLCdata*)inData;
  GLCchar32 hashValue = GLC_MASTER_HASH_VALUE(data->master);

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  if (hashValue != __glcHashValue(&inElfe->elfLogFont))
    return 1;

  if (--data->index)
    return 1;

  data->ptr = __glcMalloc(LF_FACESIZE * sizeof(TCHAR));
  if (!data->ptr) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  memcpy(data->ptr, inElfe->elfStyle, LF_FACESIZE * sizeof(TCHAR));
  return 0;
}



/* Get the style name of the face identified by inIndex  */
GLCchar8* __glcMasterGetFaceName(__GLCmaster* This, __GLCcontext* inContext,
				 GLint inIndex)
{
  LOGFONT lfont;
  __GLCdata data = {NULL, NULL, NULL, NULL, 0};
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  data.master = This;
  data.index = inIndex;

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  if (EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumMasterGetFaceName,
                         (LPARAM)&data, 0)) {
    /* Could not find face identified by inIndex */
    assert(data.index);
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  assert(!data.index);

  DeleteDC(dc);
  return (GLCchar8*)data.ptr;
}



static int CALLBACK __glcEnumMasterIsFixedPitch(ENUMLOGFONTEX* inElfe,
                                                NEWTEXTMETRICEX* inNtme,
                                                DWORD inFontType, LPARAM inData)
{
  __GLCdata* data = (__GLCdata*)inData;
  GLCchar32 hashValue = GLC_MASTER_HASH_VALUE(data->master);

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  if (hashValue == __glcHashValue(&inElfe->elfLogFont))
    data->index += (inNtme->ntmTm.tmPitchAndFamily & 1);

  return 1;
}



/* Is this a fixed font ? */
GLboolean __glcMasterIsFixedPitch(__GLCmaster* This)
{
  LOGFONT lfont;
  __GLCdata data = {NULL, NULL, NULL, NULL, 0};
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  data.master = This;

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumMasterGetFaceName,
                     (LPARAM)&data, 0);
  DeleteDC(dc);

  return data.index ? GL_FALSE : GL_TRUE;
}



static int CALLBACK __glcEnumMasterFaceCount(ENUMLOGFONTEX* inElfe,
                                             NEWTEXTMETRICEX* inNtme,
                                             DWORD inFontType, LPARAM inData)
{
  __GLCdata* data = (__GLCdata*)inData;
  GLCchar32 hashValue = GLC_MASTER_HASH_VALUE(data->master);

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  if (hashValue == __glcHashValue(&inElfe->elfLogFont))
    data->index++;

  return 1;
}



/* Get the face count of the master */
GLint __glcMasterFaceCount(__GLCmaster* This, __GLCcontext* inContext)
{
  LOGFONT lfont;
  __GLCdata data = {NULL, NULL, NULL, NULL, 0};
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  data.master = This;

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumMasterGetFaceName,
                     (LPARAM)&data, 0);
  DeleteDC(dc);

  return data.index;
}



/* This subroutine is called whenever the user wants to access to informations
 * that have not been loaded from the font files yet. In order to reduce disk
 * accesses, informations such as the master format, full name or version are
 * read "just in time" i.e. only when the user requests them.
 */
GLCchar8* __glcMasterGetInfo(__GLCmaster* This, __GLCcontext* inContext,
			     GLCenum inAttrib)
{
  TCHAR* string = NULL;
  GLCchar *buffer = NULL;
  static TCHAR unknown[] = TEXT("Unknown");
#ifdef UNICODE
  int size = 0;
  size_t length = 0;
  LPSTR stringUTF8 = NULL;
#endif

  /* Get the Unicode string which corresponds to the requested attribute */
  switch(inAttrib) {
  case GLC_FAMILY:
    string = This->pattern->elfLogFont.lfFaceName;
    break;
  case GLC_FULL_NAME_SGI:
    string = This->pattern->elfFullName;
    break;
  case GLC_VERSION:
  case GLC_VENDOR:
  case GLC_MASTER_FORMAT:
    string = unknown;
    break;
  }

#ifdef UNICODE
  StringCchLength(string, STRSAFE_MAX_CCH, &length);
  size = WideCharToMultiByte(CP_UTF8, 0, string, length, NULL, 0, NULL, NULL);
  stringUTF8 = (LPSTR)__glcMalloc(size);
  if (!stringUTF8) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  WideCharToMultiByte(CP_UTF8, 0, string, length, stringUTF8, size, NULL, NULL);
  string = stringUTF8;
#endif

  if (string) {
    /* Convert the string and store it in the context buffer */
    buffer = __glcConvertFromUtf8ToBuffer(inContext, string,
					  inContext->stringState.stringType);

    if (!buffer)
      __glcRaiseError(GLC_RESOURCE_ERROR);
  }
  else
    __glcRaiseError(GLC_RESOURCE_ERROR);

  return buffer;
}



static int CALLBACK __glcEnumMasterFromFamily(ENUMLOGFONTEX* inElfe,
                                              NEWTEXTMETRICEX* inNtme,
                                              DWORD inFontType, LPARAM inData)
{
  __GLCdata* data = (__GLCdata*)inData;
  __GLCmaster* master = ((__GLCdata*)inData)->master;

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  if (lstrcmp(inElfe->elfLogFont.lfFaceName, data->ptr))
    return 1;

  memcpy(master->pattern, &inElfe->elfLogFont, sizeof(ENUMLOGFONTEX));

  return 0;
}



/* Create a master on the basis of the family name */
__GLCmaster* __glcMasterFromFamily(__GLCcontext* inContext, GLCchar8* inFamily)
{
  __GLCmaster* This = NULL;
  TCHAR* family = NULL;
  LOGFONT lfont;
  __GLCdata data = {NULL, NULL, NULL, NULL, 0};
#ifdef UNICODE
  int size = 0;
#endif
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    DeleteDC(dc);
    return NULL;
  }

#ifdef UNICODE
  int size = MultiByteToWideChar(CP_UTF8, 0, inFamily, -1, NULL, 0);

  family = (LPWSTR)__glcMalloc(size * sizeof(WCHAR));
  if (!family) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    DeleteDC(dc);
    __glcFree(This);
    return NULL;
  }
  MultiByteToWideChar(CP_UTF8, 0, inFamily, -1, family, size);
#else
  family = (TCHAR*)inFamily;
#endif

  data.master = This;
  data.ptr = family;

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  if (EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumMasterFromFamily,
                         (LPARAM)&data, 0)) {
    __glcFree(This);
    This = NULL;
    data.master = NULL;
  }
  DeleteDC(dc);

  return data.master;
}



static int CALLBACK __glcEnumMasterMatchCode(ENUMLOGFONTEX* inElfe,
                                             NEWTEXTMETRICEX* inNtme,
                                             DWORD inFontType, LPARAM inData)
{
  __GLCdata* data = (__GLCdata*)inData;
  GLint code = data->index;
  __GLCmaster* master = data->master;
  HFONT font;
  DWORD size;
  HDC dc;
  LPGLYPHSET charSet = NULL;

  if (!(inFontType & TRUETYPE_FONTTYPE))
    return 1;

  dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  font = CreateFontIndirect(&inElfe->elfLogFont);
  if (FAILED(font)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    DeleteDC(dc);
    return 0;
  }

  if (FAILED(SelectObject(dc, font))) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  size = GetFontUnicodeRanges(dc, NULL);
  if (!size) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  charSet = (LPGLYPHSET)__glcMalloc(size);
  if (!charSet) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }

  if (!GetFontUnicodeRanges(dc, charSet)) {
    DeleteDC(dc);
    DeleteObject(font);
    __glcFree(charSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return 0;
  }
  DeleteDC(dc);
  DeleteObject(font);


  /* Check if the character identified by inCode exists in the font */
  if (charSet->cGlyphsSupported) {
    DWORD i = 0;
    LPWCRANGE range = charSet->ranges;

    for (; i < charSet->cRanges; i++, range++) {
      if (code > (range->wcLow + range->cGlyphs - 1))
	continue;
      if (code >= range->wcLow) {
        memcpy(master->pattern, &inElfe->elfLogFont, sizeof(ENUMLOGFONTEX));
        __glcFree(charSet);
	return 0;
      }
    }
  }
  __glcFree(charSet);

  return 1;
}



/* Create a master which contains at least a font which match the character
 * identified by inCode.
 */
__GLCmaster* __glcMasterMatchCode(__GLCcontext* inContext, GLint inCode)
{
  __GLCmaster* This = NULL;
  LOGFONT lfont;
  __GLCdata data = {NULL, NULL, NULL, NULL, 0};
  HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

  if (FAILED(dc)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    DeleteDC(dc);
    return NULL;
  }

  data.master = This;
  data.index = inCode;

  /* Enumerates all fonts in all character sets */
  lfont.lfCharSet = DEFAULT_CHARSET;
  lfont.lfFaceName[0] = '\0';

  if (EnumFontFamiliesEx(dc, &lfont, (FONTENUMPROC)__glcEnumMasterMatchCode,
                         (LPARAM)&data, 0)) {
    __glcFree(This);
    This = NULL;
    data.master = NULL;
  }
  DeleteDC(dc);

  return data.master;
}



GLint __glcMasterGetID(__GLCmaster* This, __GLCcontext* inContext)
{
  GLCchar32 hashValue = GLC_MASTER_HASH_VALUE(This);
  GLint i = 0;
  GLCchar32* hashTable = (GLCchar32*)GLC_ARRAY_DATA(inContext->masterHashTable);

  for (i = 0; i < GLC_ARRAY_LENGTH(inContext->masterHashTable); i++) {
    if (hashValue == hashTable[i])
      break;
  }
  assert(i < GLC_ARRAY_LENGTH(inContext->masterHashTable));

  return i;
}
