/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2009, Bertrand Coconnier
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

#include "internal.h"
#include <string.h>

typedef GLboolean (*__glcPatternProcess)(FcFontSet* inFontSet,
					 FcPattern* inPattern, int inFont,
					 void* inData);
struct __GLCdataRec {
  const __GLCmaster* master;
  GLint* index;
};
typedef struct __GLCdataRec __GLCdata;


static int __glcScanFonts(const __GLCcontext* inContext, FcFontSet** outFontSet,
			  FcPattern** outPattern,
			  __glcPatternProcess inPatternProcess, void* inData)
{
  int font = 0;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  FcPattern *pattern = FcPatternCreate();

  /* Use Fontconfig to get the default font files */
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return -1;
  }
  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       FC_STYLE, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return -1;
  }
  fontSet = FcFontList(inContext->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  FcPatternDestroy(pattern);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return -1;
  }

  /* Parse the font set looking for a font with an outline and which hash value
   * matches the hash value of the master we are looking for.
   */
  for (font = 0; font < fontSet->nfont; font++) {
    FcBool outline = FcFalse;
    FcChar8* family = NULL;
    int fixed = 0;
    FcChar8* foundry = NULL;
#ifdef DEBUGMODE
    FcResult result = FcResultMatch;

    result = FcPatternGetBool(fontSet->fonts[font], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetBool(fontSet->fonts[font], FC_OUTLINE, 0, &outline);
#endif
    if (!outline)
      continue;

#ifdef DEBUGMODE
    result = FcPatternGetString(fontSet->fonts[font], FC_FAMILY, 0, &family);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetString(fontSet->fonts[font], FC_FOUNDRY, 0, &foundry);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetInteger(fontSet->fonts[font], FC_SPACING, 0, &fixed);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetString(fontSet->fonts[font], FC_FAMILY, 0, &family);
    FcPatternGetString(fontSet->fonts[font], FC_FOUNDRY, 0, &foundry);
    FcPatternGetInteger(fontSet->fonts[font], FC_SPACING, 0, &fixed);
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
      FcFontSetDestroy(fontSet);
      return -1;
    }

    if (inPatternProcess(fontSet, pattern, font, inData)) {
      *outPattern = pattern;
      *outFontSet = fontSet;
      return font;
    }

    FcPatternDestroy(pattern);
  }

  *outPattern = NULL;
  *outFontSet = fontSet;
  return font;
}



static GLboolean __glcCheckHashValue(FcFontSet* GLC_UNUSED_ARG(inFontSet),
				     FcPattern* inPattern,
				     int GLC_UNUSED_ARG(inFont), void* inData)
{
  GLCchar32 hashValue = *((GLCchar32*)inData);

  if (hashValue == FcPatternHash(inPattern))
    return GL_TRUE;

  return GL_FALSE;
}



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 */
__GLCmaster* __glcMasterCreate(const GLint inMaster,
			       const __GLCcontext* inContext)
{
  __GLCmaster* This = NULL;
  GLCchar32 hashValue =
	((GLCchar32*)GLC_ARRAY_DATA(inContext->masterHashTable))[inMaster];
  FcFontSet *fontSet = NULL;
  FcPattern *pattern = NULL;
  int font = __glcScanFonts(inContext, &fontSet, &pattern, __glcCheckHashValue,
			    &hashValue);

  assert(font < fontSet->nfont);

  FcFontSetDestroy(fontSet);
  if (font < 0)
    return NULL;

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCmaster));

  /* Duplicate the pattern of the found font (otherwise it will be deleted with
   * the font set).
   */
  This->pattern = pattern;
  return This;
}



/* Destructor of the object */
void __glcMasterDestroy(__GLCmaster* This)
{
  FcPatternDestroy(This->pattern);

  __glcFree(This);
}



static GLboolean __glcFindMasterIndex(FcFontSet* GLC_UNUSED_ARG(inFontSet),
				      FcPattern* inPattern,
				      int GLC_UNUSED_ARG(inFont), void* inData)
{
  const __GLCmaster* This = ((__GLCdata*)inData)->master;
  GLint *idx = ((__GLCdata*)inData)->index; 
  FcBool equal = FcPatternEqual(inPattern, This->pattern);

  if (equal) {
    if (*idx)
      (*idx)--;
    else
      return GL_TRUE;
  }

  return GL_FALSE;
}



/* Get the style name of the face identified by inIndex  */
GLCchar8* __glcMasterGetFaceName(const __GLCmaster* This,
				 const __GLCcontext* inContext,
				 const GLint inIndex)
{
  FcFontSet *fontSet = NULL;
  FcPattern *pattern = NULL;
  GLCchar8* string = NULL;
  GLCchar8* faceName;
  GLint idx = inIndex;
  __GLCdata data = {NULL, NULL};
  int font = 0;
#ifdef DEBUGMODE
  FcResult result = FcResultMatch;
#endif

  data.master = This;
  data.index = &idx;

  font = __glcScanFonts(inContext, &fontSet, &pattern, __glcFindMasterIndex,
			&data);

  FcPatternDestroy(pattern);

  if (font < 0) {
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  if (font == fontSet->nfont) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

#ifdef DEBUGMODE
  result = FcPatternGetString(fontSet->fonts[font], FC_STYLE, 0, &string);
  assert(result != FcResultTypeMismatch);
#else
  FcPatternGetString(fontSet->fonts[font], FC_STYLE, 0, &string);
#endif

#ifdef __WIN32__
  faceName = (GLCchar8*)_strdup((const char*)string);
#else
  faceName = (GLCchar8*)strdup((const char*)string);
#endif
  FcFontSetDestroy(fontSet);
  if (!faceName) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  return faceName;
}



/* Is this a fixed font ? */
GLboolean __glcMasterIsFixedPitch(const __GLCmaster* This)
{
  int fixed = 0;
#ifdef DEBUGMODE
  FcResult result = FcResultMatch;

  result = FcPatternGetInteger(This->pattern, FC_SPACING, 0, &fixed);
  assert(result != FcResultTypeMismatch);
#else
  FcPatternGetInteger(This->pattern, FC_SPACING, 0, &fixed);
#endif
  return fixed ? GL_TRUE: GL_FALSE;
}



static GLboolean __glcCountFaces(FcFontSet* GLC_UNUSED_ARG(inFontSet),
				 FcPattern* inPattern,
				 int GLC_UNUSED_ARG(inFont), void* inData)
{
  const __GLCmaster* This = ((__GLCdata*)inData)->master;
  GLint *count = ((__GLCdata*)inData)->index; 
  FcBool equal = FcPatternEqual(inPattern, This->pattern);

  if (equal)
    (*count)++;

  return GL_FALSE;
}



/* Get the face count of the master */
GLint __glcMasterFaceCount(const __GLCmaster* This,
			   const __GLCcontext* inContext)
{
  FcFontSet *fontSet = NULL;
  GLint count = 0;
  FcPattern* pattern = NULL;
  __GLCdata data = {NULL, NULL};
  int font = 0;

  data.master = This;
  data.index = &count;

  font = __glcScanFonts(inContext, &fontSet, &pattern, __glcCountFaces, &data);

  FcFontSetDestroy(fontSet);

  if (font < 0)
    return 0;

  return count;
}



/* This subroutine is called whenever the user wants to access to informations
 * that have not been loaded from the font files yet. In order to reduce disk
 * accesses, informations such as the master format, full name or version are
 * read "just in time" i.e. only when the user requests them.
 */
const GLCchar8* __glcMasterGetInfo(const __GLCmaster* This,
				   __GLCcontext* inContext,
				   const GLCenum inAttrib)
{
  __GLCfaceDescriptor* faceDesc = NULL;
#ifdef DEBUGMODE
  FcResult result = FcResultMatch;
#endif
  GLCchar8 *string = NULL;
  const GLCchar8* info = NULL;
  const GLCchar8 *buffer = NULL;

  /* Get the Unicode string which corresponds to the requested attribute */
  switch(inAttrib) {
  case GLC_FAMILY:
#ifdef DEBUGMODE
    result = FcPatternGetString(This->pattern, FC_FAMILY, 0, &string);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetString(This->pattern, FC_FAMILY, 0, &string);
#endif
    return string;
  case GLC_VENDOR:
#ifdef DEBUGMODE
    result = FcPatternGetString(This->pattern, FC_FOUNDRY, 0, &string);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetString(This->pattern, FC_FOUNDRY, 0, &string);
#endif
    return string;
  case GLC_VERSION:
  case GLC_FULL_NAME_SGI:
  case GLC_MASTER_FORMAT:
    faceDesc = __glcFaceDescCreate(This, NULL, inContext, 0);

    if (!faceDesc)
      return NULL;

#ifndef GLC_FT_CACHE
    if (!__glcFaceDescOpen(faceDesc, inContext)) {
      __glcFaceDescDestroy(faceDesc, inContext);
      return NULL;
    }
#endif

    info = __glcFaceDescGetFontFormat(faceDesc, inContext, inAttrib);
    if (info) {
      /* Convert the string and store it in the context buffer */
      buffer = (const GLCchar8*)__glcConvertFromUtf8ToBuffer(inContext, info);
    }
    else
      __glcRaiseError(GLC_RESOURCE_ERROR);

    if (faceDesc) {
#ifndef GLC_FT_CACHE
      __glcFaceDescClose(faceDesc);
#endif
      __glcFaceDescDestroy(faceDesc, inContext);
    }

    return buffer;
  }


  /* The program is not supposed to reach the end of the function.
   * The following return is there to prevent the compiler to issue
   * a warning about 'control reaching the end of a non-void function'.
   */
    return (const GLCchar8*)"";
}



static GLboolean __glcFindFamily(FcFontSet* inFontSet,
				 FcPattern* GLC_UNUSED_ARG(inPattern),
				 int inFont, void* inData)
{
  FcChar8* family = NULL;
#ifdef DEBUGMODE
  FcResult result = FcPatternGetString(inFontSet->fonts[inFont], FC_FAMILY, 0,
				       &family);
  assert(result != FcResultTypeMismatch);
#else
  FcPatternGetString(inFontSet->fonts[inFont], FC_FAMILY, 0, &family);
#endif

  if (strcmp((const char*)family, (const char*)inData))
    return GL_FALSE;

  return GL_TRUE;
}



/* Create a master on the basis of the family name */
__GLCmaster* __glcMasterFromFamily(const __GLCcontext* inContext,
				   const GLCchar8* inFamily)
{
  FcFontSet *fontSet = NULL;
  FcPattern* pattern = NULL;
  __GLCmaster* This = NULL;
  int font = __glcScanFonts(inContext, &fontSet, &pattern, __glcFindFamily,
			    (void*)inFamily);

  if (font < 0) {
    if (pattern)
      FcPatternDestroy(pattern);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  assert(fontSet);

  if (font == fontSet->nfont) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    if (pattern)
      FcPatternDestroy(pattern);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  assert(pattern);

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    FcPatternDestroy(pattern);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCmaster));

  This->pattern = pattern;
  FcFontSetDestroy(fontSet);
  return This;
}



/* Create a master which contains at least a font which math the character
 * identified by inCode.
 */
__GLCmaster* __glcMasterMatchCode(const __GLCcontext* inContext,
				  const GLint inCode)
{
  __GLCmaster* This = NULL;
  FcPattern* pattern = NULL;
  FcFontSet* fontSet = NULL;
  FcFontSet* fontSet2 = NULL;
  FcObjectSet* objectSet = NULL;
  FcResult result = FcResultMatch;
  int f = 0;
  FcChar8* family = NULL;
  int fixed = 0;
  FcChar8* foundry = NULL;
  FcCharSet* charSet = FcCharSetCreate();

  if (!charSet)
    return NULL;

  if (!FcCharSetAddChar(charSet, inCode)) {
    FcCharSetDestroy(charSet);
    return NULL;
  }
  pattern = FcPatternBuild(NULL, FC_CHARSET, FcTypeCharSet, charSet,
			   FC_OUTLINE, FcTypeBool, FcTrue, NULL);
  FcCharSetDestroy(charSet);
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  if (!FcConfigSubstitute(inContext->config, pattern, FcMatchPattern)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }
  FcDefaultSubstitute(pattern);
  fontSet = FcFontSort(inContext->config, pattern, FcFalse, NULL, &result);
  FcPatternDestroy(pattern);
  if ((!fontSet) || (result == FcResultTypeMismatch)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  for (f = 0; f < fontSet->nfont; f++) {
    FcBool outline = FcFalse;

#ifdef DEBUGMODE
    result = FcPatternGetBool(fontSet->fonts[f], FC_OUTLINE, 0, &outline);
    assert(result != FcResultTypeMismatch);
    result = FcPatternGetCharSet(fontSet->fonts[f], FC_CHARSET, 0, &charSet);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetBool(fontSet->fonts[f], FC_OUTLINE, 0, &outline);
    FcPatternGetCharSet(fontSet->fonts[f], FC_CHARSET, 0, &charSet);
#endif

    if (outline && FcCharSetHasChar(charSet, inCode))
      break;
  }

  if (f == fontSet->nfont) {
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  /* Ugly hack to extract a subset of the pattern fontSet->fonts[f]
   * (otherwise the hash value will not match any value of the hash table).
   */
  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_OUTLINE, FC_SPACING,
			       NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }
  fontSet2 = FcFontList(inContext->config, fontSet->fonts[f], objectSet);
  FcObjectSetDestroy(objectSet);
  if (!fontSet2) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    return NULL;
  }

  assert(fontSet2->nfont);

  This = (__GLCmaster*)__glcMalloc(sizeof(__GLCmaster));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcFontSetDestroy(fontSet);
    FcFontSetDestroy(fontSet2);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCmaster));

  /* Duplicate the pattern of the found font (otherwise it will be deleted with
   * the font set).
   */
#ifdef DEBUGMODE
  result = FcPatternGetString(fontSet2->fonts[0], FC_FAMILY, 0, &family);
  assert(result != FcResultTypeMismatch);
  result = FcPatternGetString(fontSet2->fonts[0], FC_FOUNDRY, 0, &foundry);
  assert(result != FcResultTypeMismatch);
  result = FcPatternGetInteger(fontSet2->fonts[0], FC_SPACING, 0, &fixed);
  assert(result != FcResultTypeMismatch);
#else
  FcPatternGetString(fontSet2->fonts[0], FC_FAMILY, 0, &family);
  FcPatternGetString(fontSet2->fonts[0], FC_FOUNDRY, 0, &foundry);
  FcPatternGetInteger(fontSet2->fonts[0], FC_SPACING, 0, &fixed);
#endif

  if (foundry)
    pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family, FC_FOUNDRY,
			     FcTypeString, foundry, FC_SPACING, FcTypeInteger,
			     fixed, NULL);
  else
    pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, family, FC_SPACING,
			     FcTypeInteger, fixed, NULL);

  FcFontSetDestroy(fontSet2);
  FcFontSetDestroy(fontSet);
  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  This->pattern = pattern;
  return This;
}



GLint __glcMasterGetID(const __GLCmaster* This, const __GLCcontext* inContext)
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
