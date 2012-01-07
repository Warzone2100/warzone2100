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
 * header of the object __GLCfont which manage the fonts
 */

#ifndef __glc_ofont_h
#define __glc_ofont_h

#include "ofacedesc.h"

/* It seems that Visual C++ does not recognize the inline keyword !?! */
#ifdef _MSC_VER
#define inline
#endif

struct __GLCfontRec {
  GLint id;
  __GLCfaceDescriptor* faceDesc;
  GLint parentMasterID;
  __GLCcharMap* charMap;
  GLfloat maxMetric[6];
  GLboolean maxMetricCached;
};

__GLCfont*  __glcFontCreate(GLint id, __GLCmaster* inMaster,
			    __GLCcontext* inContext, GLint inCode);
void __glcFontDestroy(__GLCfont *This, __GLCcontext* inContext);
__GLCglyph* __glcFontGetGlyph(const __GLCfont *This, const GLint inCode,
			      const __GLCcontext* inContext);
GLfloat* __glcFontGetBoundingBox(const __GLCfont *This, const GLint inCode,
				 GLfloat* outVec, const __GLCcontext* inContext,
				 const GLfloat inScaleX,
				 const GLfloat inScaleY);
GLfloat* __glcFontGetAdvance(const __GLCfont *This, const GLint inCode,
			     GLfloat* outVec, const __GLCcontext* inContext,
			     const GLfloat inScaleX, const GLfloat inScaleY);
GLfloat* __glcFontGetKerning(const __GLCfont* This, const GLint inCode,
			     const GLint inPrevCode, GLfloat* outVec,
			     const __GLCcontext* inContext,
			     const GLfloat inScaleX, const GLfloat inScaleY);
GLboolean __glcFontPrepareGlyph(const __GLCfont* This,
				const __GLCcontext* inContext,
				const GLfloat inScaleX, const GLfloat inScaleY,
				const GLCulong inGlyphIndex);
GLfloat* __glcFontGetMaxMetric(__GLCfont* This, GLfloat* outVec,
			       const __GLCcontext* inContext,
			       const GLfloat inScaleX, const GLfloat inScaleY);

/* Inline functions definitions */

#ifndef GLC_FT_CACHE
/* Open the font file */
static inline void* __glcFontOpen(__GLCfont* This, __GLCcontext* inContext)
{
  return __glcFaceDescOpen(This->faceDesc, inContext);
}

/* Close the font file */
static inline void __glcFontClose(__GLCfont* This)
{
   __glcFaceDescClose(This->faceDesc);
}
#endif

/* Get the size of the bitmap in which the glyph will be rendered */
static inline GLboolean __glcFontGetBitmapSize(const __GLCfont* This,
					       GLint* outWidth,
					       GLint* outHeight,
					       const GLfloat inScaleX,
					       const GLfloat inScaleY,
					       const int inFactor,
					       GLint* outPixBoundingBox,
					       const __GLCcontext* inContext)
{
  return __glcFaceDescGetBitmapSize(This->faceDesc, outWidth, outHeight,
				    inScaleX, inScaleY, outPixBoundingBox,
				    inFactor, inContext);
}

/* Decompose the outline of a glyph */
static inline GLboolean __glcFontOutlineDecompose(const __GLCfont* This,
						  __GLCrendererData* inData,
						  const __GLCcontext* inContext)
{
  return __glcFaceDescOutlineDecompose(This->faceDesc, inData, inContext);
}

/* Render the glyph in a bitmap */
static inline GLboolean __glcFontGetBitmap(const __GLCfont* This,
					   const GLint inWidth,
					   const GLint inHeight,
					   const void* inBuffer,
					   const __GLCcontext* inContext)
{
  return __glcFaceDescGetBitmap(This->faceDesc, inWidth, inHeight, inBuffer,
				inContext);
}

/* Chek if the outline of the glyph is empty (which means it is a spacing
 * character).
 */
static inline GLboolean __glcFontOutlineEmpty(const __GLCfont* This)
{
  return __glcFaceDescOutlineEmpty(This->faceDesc);
}

static inline GLboolean __glcFontHasChar(const __GLCfont* This,
					 const GLint inCode)
{
  return __glcCharMapHasChar(This->charMap, inCode);
}

#endif /* __glc_ofont_h */
