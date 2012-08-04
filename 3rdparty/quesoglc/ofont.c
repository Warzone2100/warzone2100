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

/* Defines the methods of an object that is intended to managed fonts */

/** \file
 * defines the object __GLCfont which manage the fonts
 */

#include <math.h>
#include "internal.h"



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the master 'inParent' which the font will instantiate.
 */
__GLCfont* __glcFontCreate(GLint inID, __GLCmaster* inMaster,
			   __GLCcontext* inContext, GLint inCode)
{
  __GLCfont *This = NULL;

  assert(inContext);

  This = (__GLCfont*)__glcMalloc(sizeof(__GLCfont));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCfont));

  if (inMaster) {
    /* At font creation, the default face is the first one.
     * glcFontFace() can change the face.
     */
    This->faceDesc = __glcFaceDescCreate(inMaster, NULL, inContext, inCode);
    if (!This->faceDesc) {
      __glcFree(This);
      return NULL;
    }

    This->charMap = __glcFaceDescGetCharMap(This->faceDesc, inContext);
    if (!This->charMap) {
      __glcFaceDescDestroy(This->faceDesc, inContext);
      __glcFree(This);
      return NULL;
    }

    This->parentMasterID = __glcMasterGetID(inMaster, inContext);
  }
  else {
    /* Creates an empty font (used by glcGenFontID() to reserve font IDs) */
    This->faceDesc = NULL;
    This->charMap = NULL;
    This->parentMasterID = 0;
  }

  This->id = inID;
  This->maxMetricCached = GL_FALSE;
  memset(This->maxMetric, 0, 6 * sizeof(GLfloat));

  return This;
}



/* Destructor of the object */
void __glcFontDestroy(__GLCfont *This, __GLCcontext* inContext)
{
  if (This->charMap)
    __glcCharMapDestroy(This->charMap);

  if (This->faceDesc)
    __glcFaceDescDestroy(This->faceDesc, inContext);

  __glcFree(This);
}



/* Extract from the font the glyph which corresponds to the character code
 * 'inCode'.
 */
__GLCglyph* __glcFontGetGlyph(const __GLCfont *This, const GLint inCode,
			      const __GLCcontext* inContext)
{
  /* Try to get the glyph from the character map */
  __GLCglyph* glyph = __glcCharMapGetGlyph(This->charMap, inCode);

  if (!glyph) {
    /* If it fails, we must extract the glyph from the face */
    glyph = __glcFaceDescGetGlyph(This->faceDesc, inCode, inContext);
    if (!glyph)
      return NULL;

    /* Update the character map so that the glyph will be cached */
    __glcCharMapAddChar(This->charMap, inCode, glyph);
  }

  return glyph;
}



/* Get the bounding box of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inCode' contains the character
 * code for which the bounding box is requested.
 */
GLfloat* __glcFontGetBoundingBox(const __GLCfont *This, const GLint inCode,
				 GLfloat* outVec, const __GLCcontext* inContext,
				 const GLfloat inScaleX, const GLfloat inScaleY)
{
  /* Get the glyph from the font */
  __GLCglyph* glyph = __glcFontGetGlyph(This, inCode, inContext);

  assert(outVec);

  if (!glyph)
    return NULL;

  /* If the bounding box of the glyph is cached then copy it to outVec and
   * return.
   */
  if (glyph->boundingBoxCached && inContext->enableState.glObjects) {
    memcpy(outVec, glyph->boundingBox, 4 * sizeof(GLfloat));
    return outVec;
  }

  /* Otherwise, we must extract the bounding box from the face file */
  if (!__glcFaceDescGetBoundingBox(This->faceDesc, glyph->index, outVec,
				   inScaleX, inScaleY, inContext))
    return NULL;

  /* Special case for glyphes which have no bounding box (i.e. spaces) */
  if ((fabs(outVec[0] - outVec[2]) < GLC_EPSILON)
      || (fabs(outVec[1] - outVec[3]) < GLC_EPSILON)) {
    GLfloat advance[2] = {0.f, 0.f};

    if (__glcFontGetAdvance(This, inCode, advance, inContext, inScaleX,
			    inScaleY)) {
      if (fabs(outVec[0] - outVec[2]) < GLC_EPSILON)
	outVec[2] += advance[0];

      if (fabs(outVec[1] - outVec[3]) < GLC_EPSILON)
	outVec[3] += advance[1];
    }
  }

  /* Copy the result to outVec and return */
  if (inContext->enableState.glObjects) {
    memcpy(glyph->boundingBox, outVec, 4 * sizeof(GLfloat));
    glyph->boundingBoxCached = GL_TRUE;
  }

  return outVec;
}



/* Get the advance of a glyph according to the size given by inScaleX and
 * inScaleY. The result is returned in outVec. 'inCode' contains the character
 * code for which the advance is requested.
 */
GLfloat* __glcFontGetAdvance(const __GLCfont* This, const GLint inCode,
			     GLfloat* outVec, const __GLCcontext* inContext,
			     const GLfloat inScaleX, const GLfloat inScaleY)
{
  /* Get the glyph from the font */
  __GLCglyph* glyph = __glcFontGetGlyph(This, inCode, inContext);

  assert(outVec);

  if (!glyph)
    return NULL;

  /* If the advance of the glyph is cached then copy it to outVec and
   * return.
   */
  if (glyph->advanceCached && inContext->enableState.glObjects) {
    memcpy(outVec, glyph->advance, 2 * sizeof(GLfloat));
    return outVec;
  }

  /* Otherwise, we must extract the advance from the face file */
  if (!__glcFaceDescGetAdvance(This->faceDesc, glyph->index, outVec, inScaleX,
			       inScaleY, inContext))
    return NULL;

  /* Copy the result to outVec and return */
  if (inContext->enableState.glObjects) {
    memcpy(glyph->advance, outVec, 2 * sizeof(GLfloat));
    glyph->advanceCached = GL_TRUE;
  }

  return outVec;
}



/* Get the maximum metrics of a face that is the bounding box that encloses
 * every glyph of the face, and the maximum advance of the face.
 */
GLfloat* __glcFontGetMaxMetric(__GLCfont* This, GLfloat* outVec,
			       const __GLCcontext* inContext,
			       const GLfloat inScaleX, const GLfloat inScaleY)
{
  assert(outVec);

  if (This->maxMetricCached && inContext->enableState.glObjects) {
    memcpy(outVec, This->maxMetric, 6 * sizeof(GLfloat));
    return outVec;
  }

  if (!__glcFaceDescGetMaxMetric(This->faceDesc, outVec, inContext, inScaleX,
				 inScaleY))
    return NULL;

  /* Copy the result to outVec and return */
  if (inContext->enableState.glObjects) {
    memcpy(This->maxMetric, outVec, 6 * sizeof(GLfloat));
    This->maxMetricCached = GL_TRUE;
  }

  return outVec;
}



/* Get the kerning information of a pair of glyph according to the size given by
 * inScaleX and inScaleY. The result is returned in outVec. 'inCode' contains
 * the current character code and 'inPrevCode' the character code of the
 * previously displayed character.
 */
GLfloat* __glcFontGetKerning(const __GLCfont* This, const GLint inCode,
			     const GLint inPrevCode, GLfloat* outVec,
			     const __GLCcontext* inContext,
			     const GLfloat inScaleX, const GLfloat inScaleY)
{
  __GLCglyph* glyph = __glcFontGetGlyph(This, inCode, inContext);
  __GLCglyph* prevGlyph = __glcFontGetGlyph(This, inPrevCode, inContext);

  if (!glyph || !prevGlyph)
    return NULL;

  return __glcFaceDescGetKerning(This->faceDesc, glyph->index, prevGlyph->index,
				 inScaleX, inScaleY, outVec, inContext);
}



/* This internal function tries to open the face file which name is identified
 * by 'inFace'. If it succeeds, it closes the previous face and stores the new
 * face attributes in the __GLCfont object "inFont". Otherwise, it leaves the
 * font unchanged. GL_TRUE or GL_FALSE are returned to indicate if the function
 * succeeded or not.
 */
GLboolean __glcFontFace(__GLCfont* This, const GLCchar8* inFace,
			__GLCcontext *inContext)
{
  __GLCfaceDescriptor *faceDesc = NULL;
  __GLCmaster *master = NULL;
  __GLCcharMap* newCharMap = NULL;

  /* TODO : Verify if the font has already the required face activated */

  master = __glcMasterCreate(This->parentMasterID, inContext);
  if (!master)
    return GL_FALSE;

  /* Get the face descriptor of the face identified by the string inFace */
  faceDesc = __glcFaceDescCreate(master, inFace, inContext, 0);
  if (!faceDesc) {
    __glcMasterDestroy(master);
    return GL_FALSE;
  }

  newCharMap = __glcFaceDescGetCharMap(faceDesc, inContext);
  if (!newCharMap) {
    __glcFaceDescDestroy(faceDesc, inContext);
    __glcMasterDestroy(master);
    return GL_FALSE;
  }

  __glcMasterDestroy(master);

#ifndef GLC_FT_CACHE
  /* If the font belongs to GLC_CURRENT_FONT_LIST then open the font file */
  if (FT_List_Find(&inContext->currentFontList, This)) {

    /* Open the new face */
    if (!__glcFaceDescOpen(faceDesc, inContext)) {
      __glcFaceDescDestroy(faceDesc, inContext);
      __glcCharMapDestroy(newCharMap);
      return GL_FALSE;
    }

    /* Close the current face */
    __glcFontClose(This);
  }
#endif

  /* Destroy the current charmap */
  if (This->charMap)
    __glcCharMapDestroy(This->charMap);

  This->charMap = newCharMap;

  __glcFaceDescDestroy(This->faceDesc, inContext);
  This->faceDesc = faceDesc;
  This->maxMetricCached = GL_FALSE;
  memset(This->maxMetric, 0, 6 * sizeof(GLfloat));

  return GL_TRUE;
}



/* Load a glyph of the current font face and stores the corresponding data in
 * the corresponding face. The size of the glyph is given by inScaleX and
 * inScaleY. 'inGlyphIndex' contains the index of the glyph in the font file.
 */
GLboolean __glcFontPrepareGlyph(const __GLCfont* This,
				const __GLCcontext* inContext,
				const GLfloat inScaleX, const GLfloat inScaleY,
				const GLCulong inGlyphIndex)
{
  GLboolean result = __glcFaceDescPrepareGlyph(This->faceDesc, inContext,
					       inScaleX, inScaleY,
					       inGlyphIndex);
#ifndef GLC_FT_CACHE
  __glcFaceDescClose(This->faceDesc);
#endif

  return result;
}
