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
 * defines the object __GLCfaceDescriptor that contains the description of a
 * face. One of the purpose of this object is to encapsulate the FT_Face
 * structure from FreeType and to add it some more functionalities.
 * It also allows to centralize the character map management for easier
 * maintenance.
 */

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "internal.h"
#include "texture.h"
#include FT_GLYPH_H
#ifdef GLC_FT_CACHE
#include FT_CACHE_H
#endif
#include FT_OUTLINE_H

#include FT_TYPE1_TABLES_H
#ifdef FT_XFREE86_H
#include FT_XFREE86_H
#endif
#include FT_BDF_H
#ifdef FT_WINFONTS_H
#include FT_WINFONTS_H
#endif
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the name of the face, the character map, if it is a fixed
 * font or not, the file name and the index of the font in its file.
 */
__GLCfaceDescriptor* __glcFaceDescCreate(const __GLCmaster* inMaster,
					 const GLCchar8* inFace,
					 const __GLCcontext* inContext,
					 const GLint inCode)
{
  __GLCfaceDescriptor* This = NULL;
  FcObjectSet* objectSet = NULL;
  FcFontSet *fontSet = NULL;
  int i = 0;
  FcPattern* pattern = FcPatternCreate();

  if (!pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  objectSet = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_STYLE, FC_SPACING,
			       FC_FILE, FC_INDEX, FC_OUTLINE, FC_CHARSET, NULL);
  if (!objectSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    FcPatternDestroy(pattern);
    return NULL;
  }
  fontSet = FcFontList(inContext->config, pattern, objectSet);
  FcObjectSetDestroy(objectSet);
  FcPatternDestroy(pattern);
  if (!fontSet) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  for (i = 0; i < fontSet->nfont; i++) {
    FcChar8* family = NULL;
    int fixed = 0;
    FcChar8* foundry = NULL;
    FcChar8* style = NULL;
    FcBool outline = FcFalse;
    FcCharSet* charSet = NULL;
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
    result = FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &charSet);
    assert(result != FcResultTypeMismatch);
#else
    FcPatternGetCharSet(fontSet->fonts[i], FC_CHARSET, 0, &charSet);
#endif

    /* Check that the code is included in the font */
    if (inCode && !FcCharSetHasChar(charSet, inCode))
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
      FcFontSetDestroy(fontSet);
      return NULL;
    }

    equal = FcPatternEqual(pattern, inMaster->pattern);
    FcPatternDestroy(pattern);
    if (equal) {
      if (inFace) {
#ifdef DEBUGMODE
	result = FcPatternGetString(fontSet->fonts[i], FC_STYLE, 0, &style);
	assert(result != FcResultTypeMismatch);
#else
	FcPatternGetString(fontSet->fonts[i], FC_STYLE, 0, &style);
#endif
	if (strcmp((const char*)style, (const char*)inFace))
	  continue;
      }
      break;
    }
  }

  if (i == fontSet->nfont) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This = (__GLCfaceDescriptor*)__glcMalloc(sizeof(__GLCfaceDescriptor));
  if (!This) {
    FcFontSetDestroy(fontSet);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCfaceDescriptor));

  This->pattern = FcPatternDuplicate(fontSet->fonts[i]);
  FcFontSetDestroy(fontSet);
  if (!This->pattern) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    __glcFree(This);
    return NULL;
  }

  return This;
}



/* Destructor of the object */
void __glcFaceDescDestroy(__GLCfaceDescriptor* This, __GLCcontext* inContext)
{
  FT_ListNode node = NULL;
  FT_ListNode next = NULL;

#ifndef GLC_FT_CACHE
  assert(!This->faceRefCount && !This->face);
#endif

  /* Don't use FT_List_Finalize here, since __glcGlyphDestroy also destroys
   * the node itself.
   */
  node = This->glyphList.head;
  while (node) {
    next = node->next;
    __glcGlyphDestroy((__GLCglyph*)node, inContext);
    node = next;
  }

#if defined(GLC_FT_CACHE) \
  && (FREETYPE_MAJOR > 2 \
     || (FREETYPE_MAJOR == 2 \
         && (FREETYPE_MINOR > 1 \
             || (FREETYPE_MINOR == 1 && FREETYPE_PATCH >= 8))))
  /* In order to make sure its ID is removed from the FreeType cache */
  FTC_Manager_RemoveFaceID(inContext->cache, (FTC_FaceID)This);
#endif

  FcPatternDestroy(This->pattern);
  __glcFree(This);
}



#ifndef GLC_FT_CACHE
/* Open a face, select a Unicode charmap. __glcFaceDesc maintains a reference
 * count for each face so that the face is open only once.
 */
FT_Face __glcFaceDescOpen(__GLCfaceDescriptor* This,
			  __GLCcontext* inContext)
{
  if (!This->faceRefCount) {
    GLCchar8 *fileName = NULL;
    int index = 0;
#ifdef DEBUGMODE
    FcResult result = FcResultMatch;

    /* get the file name */
    result = FcPatternGetString(This->pattern, FC_FILE, 0, &fileName);
    assert(result != FcResultTypeMismatch);
    /* get the index of the font in font file */
    result = FcPatternGetInteger(This->pattern, FC_INDEX, 0, &index);
    assert(result != FcResultTypeMismatch);
#else
    /* get the file name */
    FcPatternGetString(This->pattern, FC_FILE, 0, &fileName);
    /* get the index of the font in font file */
    FcPatternGetInteger(This->pattern, FC_INDEX, 0, &index);
#endif

    if (FT_New_Face(inContext->library, (const char*)fileName, index,
		    &This->face)) {
      /* Unable to load the face file */
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }

    /* select a Unicode charmap */
    FT_Select_Charmap(This->face, ft_encoding_unicode);

    This->faceRefCount = 1;
  }
  else
    This->faceRefCount++;

  return This->face;
}



/* Close the face and update the reference counter accordingly */
void __glcFaceDescClose(__GLCfaceDescriptor* This)
{
  assert(This->faceRefCount > 0);

  This->faceRefCount--;

  if (!This->faceRefCount) {
    assert(This->face);

    FT_Done_Face(This->face);
    This->face = NULL;
  }
}



#else /* GLC_FT_CACHE */
/* Callback function used by the FreeType cache manager to open a given face */
FT_Error __glcFileOpen(FTC_FaceID inFile, FT_Library inLibrary,
		       FT_Pointer GLC_UNUSED_ARG(inData), FT_Face* outFace)
{
  __GLCfaceDescriptor* file = (__GLCfaceDescriptor*)inFile;
  GLCchar8 *fileName = NULL;
  int fileIndex = 0;
  FT_Error error;
#ifdef DEBUGMODE
  FcResult result = FcResultMatch;

  /* get the file name */
  result = FcPatternGetString(file->pattern, FC_FILE, 0, &fileName);
  assert(result != FcResultTypeMismatch);
  /* get the index of the font in font file */
  result = FcPatternGetInteger(file->pattern, FC_INDEX, 0, &fileIndex);
  assert(result != FcResultTypeMismatch);
#else
  /* get the file name */
  FcPatternGetString(file->pattern, FC_FILE, 0, &fileName);
  /* get the index of the font in font file */
  FcPatternGetInteger(file->pattern, FC_INDEX, 0, &fileIndex);
#endif

  error = FT_New_Face(inLibrary, (const char*)fileName, fileIndex, outFace);

  if (error) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return error;
  }

  /* select a Unicode charmap */
  FT_Select_Charmap(*outFace, ft_encoding_unicode);

  return error;
}
#endif /* GLC_FT_CACHE */



/* Return the glyph which corresponds to codepoint 'inCode' */
__GLCglyph* __glcFaceDescGetGlyph(__GLCfaceDescriptor* This,
				  const GLint inCode,
				  const __GLCcontext* inContext)
{
  FT_Face face = NULL;
  __GLCglyph* glyph = NULL;
  FT_ListNode node = NULL;
  FT_UInt index = 0;

  /* Check if the glyph has already been added to the glyph list */
  for (node = This->glyphList.head; node; node = node->next) {
    glyph = (__GLCglyph*)node;
    if (glyph->codepoint == (GLCulong)inCode)
      return glyph;
  }

  /* Open the face */
#ifdef GLC_FT_CACHE
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
#else
  face = __glcFaceDescOpen(This, inContext);
  if (!face) {
#endif
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  /* Create a new glyph */
#ifdef GLC_FT_CACHE
  index = FT_Get_Char_Index(face, inCode);
#else
  index = FcFreeTypeCharIndex(face, inCode);
#endif
  if (!index)
    return NULL;
  glyph = __glcGlyphCreate(index, inCode);
  if (!glyph) {
#ifndef GLC_FT_CACHE
    __glcFaceDescClose(This);
#endif
    return NULL;
  }
  /* Append the new glyph to the list of the glyphes of the face and close the
   * face.
   */
  FT_List_Add(&This->glyphList, (FT_ListNode)glyph);
#ifndef GLC_FT_CACHE
  __glcFaceDescClose(This);
#endif
  return glyph;
}



/* Prepare a font to obtain data for glyphes. Size is given via the parameters
 * "inScaleX" and "inScaleY".
 */
static GLboolean __glcFaceDescPrepareFont(__GLCfaceDescriptor* This,
					  const __GLCcontext* inContext,
					  const GLfloat inScaleX,
					  const GLfloat inScaleY)
{
#ifdef GLC_FT_CACHE
# if FREETYPE_MAJOR == 2 \
     && (FREETYPE_MINOR < 1 \
         || (FREETYPE_MINOR == 1 && FREETYPE_PATCH < 8))
  FTC_FontRec font;
# else
  FTC_ScalerRec scaler;
# endif
  FT_Size size = NULL;
#else
  FT_Error error;
#endif

  /* Open the face */
#ifdef GLC_FT_CACHE
# if FREETYPE_MAJOR == 2 \
     && (FREETYPE_MINOR < 1 \
         || (FREETYPE_MINOR == 1 && FREETYPE_PATCH < 8))
  font.face_id = (FTC_FaceID)This;

  if (inContext->enableState.glObjects) {
    font.pix_width = (FT_UShort) inScaleX;
    font.pix_height = (FT_UShort) inScaleY;
  }
  else {
    font.pix_width = (FT_UShort) (inScaleX * inContext->renderState.resolution
				  / 72.);
    font.pix_height = (FT_UShort) (inScaleY * inContext->renderState.resolution
				   / 72.);
  }

  if (FTC_Manager_Lookup_Size(inContext->cache, &font, &This->face, &size)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
# else
  scaler.face_id = (FTC_FaceID)This;
  scaler.width = (FT_UInt)(inScaleX * 64.);
  scaler.height = (FT_UInt)(inScaleY * 64.);
  scaler.pixel = (FT_Int)0;

  if (inContext->enableState.glObjects) {
    scaler.x_res = 72;
    scaler.y_res = 72;
  }
  else {
    scaler.x_res = (FT_UInt)inContext->renderState.resolution;
    scaler.y_res = (FT_UInt)inContext->renderState.resolution;
  }

  if (FTC_Manager_LookupSize(inContext->cache, &scaler, &size)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  This->face = size->face;
# endif /* FREETYPE_MAJOR */
#else
  if (!__glcFaceDescOpen(This, inContext))
    return GL_FALSE;

  /* Select the size of the glyph */
  if (inContext->enableState.glObjects) {
    error = FT_Set_Char_Size(This->face, (FT_F26Dot6)(inScaleX * 64.),
			     (FT_F26Dot6)(inScaleY * 64.), 0, 0);
  }
  else {
    error = FT_Set_Char_Size(This->face, (FT_F26Dot6)(inScaleX * 64.),
			     (FT_F26Dot6)(inScaleY * 64.),
			     (FT_UInt)inContext->renderState.resolution,
			     (FT_UInt)inContext->renderState.resolution);
  }
  if (error) {
    __glcFaceDescClose(This);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
#endif

  return GL_TRUE;
}



/* Load a glyph of the current font face and stores the corresponding data in
 * the corresponding face. The size of the glyph is given by inScaleX and
 * inScaleY. 'inGlyphIndex' contains the index of the glyph in the font file.
 */
GLboolean __glcFaceDescPrepareGlyph(__GLCfaceDescriptor* This,
				    const __GLCcontext* inContext,
				    const GLfloat inScaleX,
				    const GLfloat inScaleY,
				    const GLCulong inGlyphIndex)
{
  FT_Int32 loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM
                     | FT_LOAD_FORCE_AUTOHINT;

  if (!__glcFaceDescPrepareFont(This, inContext, inScaleX, inScaleY))
    return GL_FALSE;

  /* If GLC_HINTING_QSO is enabled then perform hinting on the glyph while
   * loading it.
   */
  if (!inContext->enableState.hinting && !inContext->enableState.glObjects)
    loadFlags |= FT_LOAD_NO_HINTING;

  /* Load the glyph */
  if (FT_Load_Glyph(This->face, inGlyphIndex, loadFlags)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
#ifndef GLC_FT_CACHE
    __glcFaceDescClose(This);
#endif
    return GL_FALSE;
  }

  return GL_TRUE;
}



/* Destroy the GL objects of every glyph of the face */
void __glcFaceDescDestroyGLObjects(const __GLCfaceDescriptor* This,
				   __GLCcontext* inContext)
{
  FT_ListNode node = NULL;

  for (node = This->glyphList.head; node; node = node->next) {
    __GLCglyph* glyph = (__GLCglyph*)node;

    __glcGlyphDestroyGLObjects(glyph, inContext);
  }
}



/* Get the bounding box of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inGlyphIndex' contains the
 * index of the glyph in the font file.
 */
GLfloat* __glcFaceDescGetBoundingBox(__GLCfaceDescriptor* This,
				     const GLCulong inGlyphIndex,
				     GLfloat* outVec, const GLfloat inScaleX,
				     const GLfloat inScaleY,
				     const __GLCcontext* inContext)
{
  FT_BBox boundBox;
  FT_Glyph glyph;

  assert(outVec);

  if (!__glcFaceDescPrepareGlyph(This, inContext, inScaleX, inScaleY,
				 inGlyphIndex))
    return NULL;

  /* Get the bounding box of the glyph */
  FT_Get_Glyph(This->face->glyph, &glyph);
  FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_unscaled, &boundBox);

  /* Transform the bounding box according to the conversion from FT_F26Dot6 to
   * GLfloat and the size in points of the glyph.
   */
  outVec[0] = (GLfloat) boundBox.xMin / 64. / inScaleX;
  outVec[2] = (GLfloat) boundBox.xMax / 64. / inScaleX;
  outVec[1] = (GLfloat) boundBox.yMin / 64. / inScaleY;
  outVec[3] = (GLfloat) boundBox.yMax / 64. / inScaleY;

  FT_Done_Glyph(glyph);
#ifndef GLC_FT_CACHE
  __glcFaceDescClose(This);
#endif
  return outVec;
}



/* Get the advance of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inGlyphIndex' contains the
 * index of the glyph in the font file.
 */
GLfloat* __glcFaceDescGetAdvance(__GLCfaceDescriptor* This,
				 const GLCulong inGlyphIndex, GLfloat* outVec,
				 const GLfloat inScaleX, const GLfloat inScaleY,
				 const __GLCcontext* inContext)
{
  assert(outVec);

  if (!__glcFaceDescPrepareGlyph(This, inContext, inScaleX, inScaleY,
				 inGlyphIndex))
    return NULL;

  /* Transform the advance according to the conversion from FT_F26Dot6 to
   * GLfloat.
   */
  outVec[0] = (GLfloat) This->face->glyph->advance.x / 64. / inScaleX;
  outVec[1] = (GLfloat) This->face->glyph->advance.y / 64. / inScaleY;

#ifndef GLC_FT_CACHE
  __glcFaceDescClose(This);
#endif
  return outVec;
}



/* Use FreeType to determine in which format the face is stored in its file :
 * Type1, TrueType, OpenType, ...
 */
const GLCchar8* __glcFaceDescGetFontFormat(const __GLCfaceDescriptor* This,
					   const __GLCcontext* inContext,
					   const GLCenum inAttrib)
{
  static GLCchar8 unknown[] = "Unknown";
#ifndef FT_XFREE86_H
  static GLCchar8 masterFormat1[] = "Type 1";
  static GLCchar8 masterFormat2[] = "BDF";
#  ifdef FT_WINFONTS_H
  static GLCchar8 masterFormat3[] = "Windows FNT";
#  endif /* FT_WINFONTS_H */
  static GLCchar8 masterFormat4[] = "TrueType/OpenType";
#endif /* FT_XFREE86_H */

  FT_Face face = NULL;
  PS_FontInfoRec afont_info;
  const char* acharset_encoding = NULL;
  const char* acharset_registry = NULL;
#ifdef FT_WINFONTS_H
  FT_WinFNT_HeaderRec aheader;
#endif
  GLCuint count = 0;
  const GLCchar8* result = NULL;

  /* Open the face */
#ifdef GLC_FT_CACHE
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
#else
  face = __glcFaceDescOpen(This, inContext);
  if (!face) {
#endif
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

#ifdef FT_XFREE86_H
  if (inAttrib == GLC_MASTER_FORMAT) {
    /* This function is undocumented until FreeType 2.3.0 where it has been
     * added to the public API. It can be safely used nonetheless as long as
     * the existence of FT_XFREE86_H is checked.
     */
    result = (const GLCchar8*)FT_Get_X11_Font_Format(face);
#  ifndef GLC_FT_CACHE
    __glcFaceDescClose(This);
#  endif /* GLC_FT_CACHE */
    return result;
  }
#endif /* FT_XFREE86_H */

  /* Is it Type 1 ? */
  if (!FT_Get_PS_Font_Info(face, &afont_info)) {
    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
#ifndef GLC_FT_CACHE
      __glcFaceDescClose(This);
#endif
      return masterFormat1;
#endif
    case GLC_FULL_NAME_SGI:
      if (afont_info.full_name)
	result = (GLCchar8*)afont_info.full_name;
      break;
    case GLC_VERSION:
      if (afont_info.version)
	result = (GLCchar8*)afont_info.version;
      break;
    }
  }
  /* Is it BDF ? */
  else if (!FT_Get_BDF_Charset_ID(face, &acharset_encoding,
				  &acharset_registry)) {
    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
      result = masterFormat2;
      break;
#endif
    case GLC_FULL_NAME_SGI:
      result = unknown;
      break;
    case GLC_VERSION:
      result = unknown;
      break;
    }
  }
#ifdef FT_WINFONTS_H
  /* Is it Windows FNT ? */
  else if (!FT_Get_WinFNT_Header(face, &aheader)) {
    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
      result = masterFormat3;
      break;
#endif
    case GLC_FULL_NAME_SGI:
      result = unknown;
      break;
    case GLC_VERSION:
      result = unknown;
      break;
    }
  }
#endif
  /* Is it TrueType/OpenType ? */
  else if ((count = FT_Get_Sfnt_Name_Count(face))) {
#if 0
    GLCuint i = 0;
    FT_SfntName aName;
#endif

    switch(inAttrib) {
#ifndef FT_XFREE86_H
    case GLC_MASTER_FORMAT:
      result = masterFormat4;
      break;
#endif
    case GLC_FULL_NAME_SGI:
      result = unknown;
      break;
    case GLC_VERSION:
      result = unknown;
      break;
    }

    /* TODO : decode the SFNT name tables in order to get full name
     * of the TrueType/OpenType fonts and their version
     */
#if 0
    for (i = 0; i < count; i++) {
      if (!FT_Get_Sfnt_Name(face, i, &aName)) {
        if ((aName.name_id != TT_NAME_ID_FULL_NAME)
		&& (aName.name_id != TT_NAME_ID_VERSION_STRING))
	  continue;

        switch (aName.platform_id) {
	case TT_PLATFORM_APPLE_UNICODE:
	  break;
	case TT_PLATFORM_MICROSOFT:
	  break;
	}
      }
    }
#endif
  }

#ifndef GLC_FT_CACHE
  /* Close the face */
  __glcFaceDescClose(This);
#endif
  return result;
}



/* Get the maximum metrics of a face that is the bounding box that encloses
 * every glyph of the face, and the maximum advance of the face.
 */
GLfloat* __glcFaceDescGetMaxMetric(__GLCfaceDescriptor* This, GLfloat* outVec,
				   const __GLCcontext* inContext,
				   const GLfloat inScaleX,
				   const GLfloat inScaleY)
{
  GLfloat scale = inContext->renderState.resolution / 72.;

  assert(outVec);

  if (!__glcFaceDescPrepareFont(This, inContext, inScaleX, inScaleY))
    return NULL;

  scale /= This->face->units_per_EM;

  /* Get the values and transform them according to the resolution */
  outVec[0] = (GLfloat)This->face->max_advance_width * scale;
  outVec[1] = (GLfloat)This->face->max_advance_height * scale;
  outVec[2] = (GLfloat)This->face->bbox.yMax * scale;
  outVec[3] = (GLfloat)This->face->bbox.yMin * scale;
  outVec[4] = (GLfloat)This->face->bbox.xMax * scale;
  outVec[5] = (GLfloat)This->face->bbox.xMin * scale;

#ifndef GLC_FT_CACHE
  __glcFaceDescClose(This);
#endif
  return outVec;
}



/* Get the kerning information of a pair of glyphes according to the size given
 * by inScaleX and inScaleY. The result is returned in outVec.
 */
GLfloat* __glcFaceDescGetKerning(__GLCfaceDescriptor* This,
				 const GLCuint inGlyphIndex,
				 const GLCuint inPrevGlyphIndex,
				 const GLfloat inScaleX, const GLfloat inScaleY,
				 GLfloat* outVec, const __GLCcontext* inContext)
{
  FT_Vector kerning;
  FT_Error error;

  assert(outVec);

  if (!__glcFaceDescPrepareGlyph(This, inContext, inScaleX, inScaleY,
				 inGlyphIndex))
    return NULL;

  if (!FT_HAS_KERNING(This->face)) {
    outVec[0] = 0.;
    outVec[1] = 0.;
    return outVec;
  }

  error = FT_Get_Kerning(This->face, inPrevGlyphIndex, inGlyphIndex,
			 FT_KERNING_DEFAULT, &kerning);

#ifndef GLC_FT_CACHE
  __glcFaceDescClose(This);
#endif

  if (error)
    return NULL;
  else {
    outVec[0] = (GLfloat) kerning.x / 64. / inScaleX;
    outVec[1] = (GLfloat) kerning.y / 64. / inScaleY;
    return outVec;
  }
}



/* Get the style name of the face descriptor */
GLCchar8* __glcFaceDescGetStyleName(__GLCfaceDescriptor* This)
{
  GLCchar8 *styleName = NULL;
#ifdef DEBUGMODE
  FcResult result = FcPatternGetString(This->pattern, FC_STYLE, 0, &styleName);

  assert(result != FcResultTypeMismatch);
#else
  FcPatternGetString(This->pattern, FC_STYLE, 0, &styleName);
#endif
  return styleName;
}



/* Determine if the face descriptor has a fixed pitch */
GLboolean __glcFaceDescIsFixedPitch(__GLCfaceDescriptor* This)
{
  int fixed = 0;
#ifdef DEBUGMODE
  FcResult result = FcPatternGetInteger(This->pattern, FC_SPACING, 0, &fixed);

  assert(result != FcResultTypeMismatch);
#else
  FcPatternGetInteger(This->pattern, FC_SPACING, 0, &fixed);
#endif
  return (fixed != FC_PROPORTIONAL);
}



/* Callback function that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * MoveTo is called when the pen move from one curve to another curve.
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcMoveTo(const FT_Vector *inVecTo, void* inUserData)
#else
static int __glcMoveTo(FT_Vector *inVecTo, void* inUserData)
#endif
{
  __GLCrendererData *data = (__GLCrendererData *) inUserData;

  /* We don't need to store the point where the pen is since glyphs are defined
   * by closed loops (i.e. the first point and the last point are the same) and
   * the first point will be stored by the next call to lineto/conicto/cubicto.
   */

  if (!__glcArrayAppend(data->endContour,
			&GLC_ARRAY_LENGTH(data->vertexArray)))
    return 1;

  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;
  return 0;
}



/* Callback function that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * LineTo is called when the pen draws a line between two points.
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcLineTo(const FT_Vector *inVecTo, void* inUserData)
#else
static int __glcLineTo(FT_Vector *inVecTo, void* inUserData)
#endif
{
  __GLCrendererData *data = (__GLCrendererData *) inUserData;

  if (!__glcArrayAppend(data->vertexArray, data->vector))
    return 1;

  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;
  return 0;
}



/* Callback function that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * ConicTo is called when the pen draws a conic between two points (and with
 * one control point).
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcConicTo(const FT_Vector *inVecControl,
			const FT_Vector *inVecTo, void* inUserData)
{
#else
static int __glcConicTo(FT_Vector *inVecControl, FT_Vector *inVecTo,
			void* inUserData)
{
#endif
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  int error = 0;

  data->vector[2] = (GLfloat)inVecControl->x;
  data->vector[3] = (GLfloat)inVecControl->y;
  data->vector[4] = (GLfloat)inVecTo->x;
  data->vector[5] = (GLfloat)inVecTo->y;
  error = __glcdeCasteljauConic(inUserData);
  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;

  return error;
}



/* Callback functions that is called by the FreeType function
 * FT_Outline_Decompose() when parsing an outline.
 * CubicTo is called when the pen draws a cubic between two points (and with
 * two control points).
 */
#if ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR >= 2))
static int __glcCubicTo(const FT_Vector *inVecControl1,
			const FT_Vector *inVecControl2,
			const FT_Vector *inVecTo, void* inUserData)
{
#else
static int __glcCubicTo(FT_Vector *inVecControl1, FT_Vector *inVecControl2,
			FT_Vector *inVecTo, void* inUserData)
{
#endif
  __GLCrendererData *data = (__GLCrendererData *) inUserData;
  int error = 0;

  data->vector[2] = (GLfloat)inVecControl1->x;
  data->vector[3] = (GLfloat)inVecControl1->y;
  data->vector[4] = (GLfloat)inVecControl2->x;
  data->vector[5] = (GLfloat)inVecControl2->y;
  data->vector[6] = (GLfloat)inVecTo->x;
  data->vector[7] = (GLfloat)inVecTo->y;
  error = __glcdeCasteljauCubic(inUserData);
  data->vector[0] = (GLfloat) inVecTo->x;
  data->vector[1] = (GLfloat) inVecTo->y;

  return error;
}



/* Decompose the outline of a glyph */
GLboolean __glcFaceDescOutlineDecompose(const __GLCfaceDescriptor* This,
                                        __GLCrendererData* inData,
                                        const __GLCcontext* inContext)
{
  FT_Outline *outline = NULL;
  FT_Outline_Funcs outlineInterface;
  FT_Face face = NULL;

#ifndef GLC_FT_CACHE
  face = This->face;
#else
  if (FTC_Manager_LookupFace(inContext->cache, (FTC_FaceID)This, &face)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }
#endif

  /* Initialize the data for FreeType to parse the outline */
  outline = &face->glyph->outline;
  outlineInterface.shift = 0;
  outlineInterface.delta = 0;
  outlineInterface.move_to = __glcMoveTo;
  outlineInterface.line_to = __glcLineTo;
  outlineInterface.conic_to = __glcConicTo;
  outlineInterface.cubic_to = __glcCubicTo;

  if (inContext->enableState.glObjects) {
    /* Distances are computed in object space, so is the tolerance of the
     * de Casteljau algorithm.
     */
    inData->tolerance *= face->units_per_EM;
  }

  /* Parse the outline of the glyph */
  if (FT_Outline_Decompose(outline, &outlineInterface, inData)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    GLC_ARRAY_LENGTH(inData->vertexArray) = 0;
    GLC_ARRAY_LENGTH(inData->endContour) = 0;
    GLC_ARRAY_LENGTH(inData->vertexIndices) = 0;
    GLC_ARRAY_LENGTH(inData->geomBatches) = 0;
    return GL_FALSE;
  }

  return GL_TRUE;
}



/* Compute the lower power of 2 that is greater than value. It is used to
 * determine the smaller texture than can contain a glyph.
 */
static int __glcNextPowerOf2(int value)
{
  int power = 0;

  for (power = 1; power < value; power <<= 1);

  return power;
}



/* Get the size of the bitmap in which the glyph will be rendered */
GLboolean __glcFaceDescGetBitmapSize(const __GLCfaceDescriptor* This,
				     GLint* outWidth, GLint *outHeight,
				     const GLfloat inScaleX,
				     const GLfloat inScaleY,
				     GLint* outPixBoundingBox,
				     const int inFactor,
				     const __GLCcontext* inContext)
{
  FT_Outline outline;
  FT_Matrix matrix;
  FT_BBox boundingBox;
  FT_Face face = This->face;

  assert(face);

  outline = face->glyph->outline;

  if ((inContext->renderState.renderStyle == GLC_BITMAP)
      || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO)) {
    GLfloat *transform = inContext->bitmapMatrix;

    /* compute glyph dimensions */
    matrix.xx = (FT_Fixed)(transform[0] * 65536. / inScaleX);
    matrix.xy = (FT_Fixed)(transform[2] * 65536. / inScaleY);
    matrix.yx = (FT_Fixed)(transform[1] * 65536. / inScaleX);
    matrix.yy = (FT_Fixed)(transform[3] * 65536. / inScaleY);
  }
  else {
    matrix.xy = 0;
    matrix.yx = 0;

    if (inContext->enableState.glObjects) {
      matrix.xx = (FT_Fixed)((GLC_TEXTURE_SIZE << 16) / inScaleX);
      matrix.yy = (FT_Fixed)((GLC_TEXTURE_SIZE << 16)/ inScaleY);
    }
    else {
      matrix.xx = 65536 >> inFactor;
      matrix.yy = 65536 >> inFactor;
    }
  }

  FT_Outline_Transform(&outline, &matrix);
  FT_Outline_Get_CBox(&outline, &boundingBox);

  if ((inContext->renderState.renderStyle == GLC_BITMAP)
      || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO)) {
    FT_Pos pitch = 0;

    outPixBoundingBox[0] = GLC_FLOOR_26_6(boundingBox.xMin);
    outPixBoundingBox[1] = GLC_FLOOR_26_6(boundingBox.yMin);
    outPixBoundingBox[2] = GLC_CEIL_26_6(boundingBox.xMax);
    outPixBoundingBox[3] = GLC_CEIL_26_6(boundingBox.yMax);

    /* Calculate pitch to upper 8 byte boundary for 1 bit/pixel, i.e. ceil() */
    pitch = (outPixBoundingBox[2] - outPixBoundingBox[0] + 511) >> 9;

    *outWidth = pitch << 3;
    *outHeight = (outPixBoundingBox[3] - outPixBoundingBox[1]) >> 6;
  }
  else {
    GLint width = 0;
    GLint height = 0;

    if (inContext->enableState.glObjects) {
      GLfloat ratioX = 0.f;
      GLfloat ratioY = 0.f;
      GLfloat ratio = 0.f;

      width = boundingBox.xMax - boundingBox.xMin;
      height = boundingBox.yMax - boundingBox.yMin;

      ratioX = width / (64.f * GLC_TEXTURE_SIZE);
      ratioY = height / (64.f * GLC_TEXTURE_SIZE);

      ratioX = (ratioX > 1.f) ? ratioX : 1.f;
      ratioY = (ratioY > 1.f) ? ratioY : 1.f;
      ratio = ((ratioX > ratioY) ? ratioX : ratioY);

      *outWidth = GLC_TEXTURE_SIZE;
      *outHeight = GLC_TEXTURE_SIZE;

      outline.flags |= FT_OUTLINE_HIGH_PRECISION;

      if (ratio > 1.f) {
	outPixBoundingBox[0] = boundingBox.xMin
	  - ((GLint)((GLC_TEXTURE_SIZE << 5) - (width * 0.5f)) * ratio);
	outPixBoundingBox[1] = boundingBox.yMin
	  - ((GLint)((GLC_TEXTURE_SIZE << 5) - (height * 0.5f)) * ratio);
	outPixBoundingBox[2] = outPixBoundingBox[0]
	  + ((GLint)((GLC_TEXTURE_SIZE << 6) * ratio));
	outPixBoundingBox[3] = outPixBoundingBox[1]
	  + ((GLint)((GLC_TEXTURE_SIZE << 6) * ratio));

	matrix.xx = (FT_Fixed)(65536.f / ratio);
	matrix.yy = matrix.xx;

	FT_Outline_Transform(&outline, &matrix);
	FT_Outline_Get_CBox(&outline, &boundingBox);
      }
      else {
	outPixBoundingBox[0] = boundingBox.xMin
	  - ((GLC_TEXTURE_SIZE << 5) - (width >> 1));
	outPixBoundingBox[1] = boundingBox.yMin
	  - ((GLC_TEXTURE_SIZE << 5) - (height >> 1));
	outPixBoundingBox[2] = outPixBoundingBox[0] + ((GLC_TEXTURE_SIZE - 1) << 6);
	outPixBoundingBox[3] = outPixBoundingBox[1] + ((GLC_TEXTURE_SIZE - 1) << 6);
      }
    }
    else {
      width = (GLC_CEIL_26_6(boundingBox.xMax)
	       - GLC_FLOOR_26_6(boundingBox.xMin)) >> 6;
      height = (GLC_CEIL_26_6(boundingBox.yMax)
		- GLC_FLOOR_26_6(boundingBox.yMin)) >> 6;
      *outWidth = __glcNextPowerOf2(width);
      *outHeight = __glcNextPowerOf2(height);

      *outWidth = (*outWidth > inContext->texture.width)
	? *outWidth : inContext->texture.width;
      *outHeight = (*outHeight > inContext->texture.height)
	? *outHeight : inContext->texture.height;

      if (*outWidth - width <= 1) *outWidth <<= 1;
      if (*outHeight - height <= 1) *outHeight <<= 1;

      /* If the texture size is too small then give up */
      if ((*outWidth < 4) || (*outHeight < 4))
        return GL_FALSE;

      outPixBoundingBox[0] = GLC_FLOOR_26_6(boundingBox.xMin)
	- (((*outWidth - width) >> 1 ) << 6);
      outPixBoundingBox[1] = GLC_FLOOR_26_6(boundingBox.yMin)
	- (((*outHeight - height) >> 1) << 6);
      outPixBoundingBox[2] = outPixBoundingBox[0] + ((*outWidth - 1) << 6);
      outPixBoundingBox[3] = outPixBoundingBox[1] + ((*outHeight - 1) << 6);
    }
  }

  return GL_TRUE;
}



/* Render the glyph in a bitmap */
GLboolean __glcFaceDescGetBitmap(const __GLCfaceDescriptor* This,
				 const GLint inWidth, const GLint inHeight,
				 const void* inBuffer,
				 const __GLCcontext* inContext)
{
  FT_Outline outline;
  FT_BBox boundingBox;
  FT_Bitmap pixmap;
  FT_Matrix matrix;
  FT_Pos dx = 0, dy = 0;
  FT_Face face = This->face;
  FT_Pos width = 0, height = 0;

  assert(face);

  outline = face->glyph->outline;
  FT_Outline_Get_CBox(&outline, &boundingBox);

  if ((inContext->renderState.renderStyle == GLC_BITMAP)
      || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO)
      || (!inContext->enableState.glObjects)) {
    dx = GLC_FLOOR_26_6(boundingBox.xMin);
    dy = GLC_FLOOR_26_6(boundingBox.yMin);
    if (inContext->renderState.renderStyle == GLC_TEXTURE) {
      width = (GLC_CEIL_26_6(boundingBox.xMax) - dx) >> 6;
      height = (GLC_CEIL_26_6(boundingBox.yMax) - dy) >> 6;
      dx -= (((inWidth - width) >> 1) << 6);
      dy -= (((inHeight - height) >> 1) << 6);
    }
  }
  else {
    dx = boundingBox.xMin;
    dy = boundingBox.yMin;
    width = boundingBox.xMax - dx;
    height = boundingBox.yMax - dy;
    dx -= (inWidth << 5) - (width >> 1);
    dy -= (inHeight << 5) - (height >> 1);
  }
  
  pixmap.width = inWidth;
  pixmap.rows = inHeight;
  pixmap.buffer = (unsigned char*)inBuffer;

  if (inContext->renderState.renderStyle == GLC_BITMAP) {
    /* Calculate pitch to upper 8 byte boundary for 1 bit/pixel, i.e. ceil() */
    pixmap.pitch = -(pixmap.width >> 3);

    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_mono;	/* Monochrome rendering */
  }
  else {
    /* Flip the picture */
    pixmap.pitch = -pixmap.width; /* 8 bits/pixel */

    /* Fill the pixmap descriptor and the pixmap buffer */
    pixmap.pixel_mode = ft_pixel_mode_grays; /* Anti-aliased rendering */
    pixmap.num_grays = 256;
  }

  /* fill the pixmap buffer with the background color */
  memset(pixmap.buffer, 0, - pixmap.rows * pixmap.pitch);

  /* translate the outline to match (0,0) with the glyph's lower left
   * corner
   */
  FT_Outline_Translate(&outline, -dx, -dy);

  /* render the glyph */
  if (FT_Outline_Get_Bitmap(inContext->library, &outline, &pixmap)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return GL_FALSE;
  }

  if ((inContext->renderState.renderStyle != GLC_BITMAP)
      && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)) {
    /* Prepare the outline for the next mipmap level :
     * a. Restore the outline initial position
     */
    FT_Outline_Translate(&outline, dx, dy);

    /* b. Divide the character size by 2. */
    matrix.xx = 32768; /* 0.5 in FT_Fixed type */
    matrix.xy = 0;
    matrix.yx = 0;
    matrix.yy = 32768;
    FT_Outline_Transform(&outline, &matrix);
  }

  return GL_TRUE;
}



/* Chek if the outline of the glyph is empty (which means it is a spacing
 * character).
 */
GLboolean __glcFaceDescOutlineEmpty(__GLCfaceDescriptor* This)
{
  FT_Outline outline = This->face->glyph->outline;

  return outline.n_points ? GL_TRUE : GL_FALSE;
}



/* Get the CharMap of the face descriptor */
__GLCcharMap* __glcFaceDescGetCharMap(__GLCfaceDescriptor* This,
				      __GLCcontext* inContext)
{
#ifdef DEBUGMODE
  FcResult result = FcResultMatch;
#endif
  FcCharSet* charSet = NULL;
  FcCharSet* newCharSet = NULL;
  __GLCcharMap* charMap = __glcCharMapCreate(NULL, inContext);

  if (!charMap)
    return NULL;

#ifdef DEBUGMODE
  result = FcPatternGetCharSet(This->pattern, FC_CHARSET, 0, &charSet);
  assert(result != FcResultTypeMismatch);
#else
  FcPatternGetCharSet(This->pattern, FC_CHARSET, 0, &charSet);
#endif

  newCharSet = FcCharSetCopy(charSet);
  if (!newCharSet) {
    __glcCharMapDestroy(charMap);
    return NULL;
  }

  FcCharSetDestroy(charMap->charSet);
  charMap->charSet = newCharSet;

  return charMap;
}
