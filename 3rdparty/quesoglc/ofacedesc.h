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
 * header of the object __GLCfaceDescriptor that contains the description of a
 * face.
 */

#ifndef __glc_ofacedesc_h
#define __glc_ofacedesc_h

#include "omaster.h"

typedef struct __GLCrendererDataRec __GLCrendererData;
typedef struct __GLCfaceDescriptorRec __GLCfaceDescriptor;

struct __GLCfaceDescriptorRec {
  FT_ListNodeRec node;
  FcPattern* pattern;
  FT_Face face;
#ifndef GLC_FT_CACHE
  int faceRefCount;
#endif
  FT_ListRec glyphList;
};


__GLCfaceDescriptor* __glcFaceDescCreate(const __GLCmaster* inMaster,
					 const GLCchar8* inFace,
					 const __GLCcontext* inContext,
					 const GLint inCode);
void __glcFaceDescDestroy(__GLCfaceDescriptor* This, __GLCcontext* inContext);
#ifndef GLC_FT_CACHE
FT_Face __glcFaceDescOpen(__GLCfaceDescriptor* This,
			  __GLCcontext* inContext);
void __glcFaceDescClose(__GLCfaceDescriptor* This);
#endif
__GLCglyph* __glcFaceDescGetGlyph(__GLCfaceDescriptor* This,
				  const GLint inCode,
				  const __GLCcontext* inContext);
void __glcFaceDescDestroyGLObjects(const __GLCfaceDescriptor* This,
				   __GLCcontext* inContext);
GLboolean __glcFaceDescPrepareGlyph(__GLCfaceDescriptor* This,
				    const __GLCcontext* inContext,
				    const GLfloat inScaleX,
				    const GLfloat inScaleY,
				    const GLCulong inGlyphIndex);
GLfloat* __glcFaceDescGetBoundingBox(__GLCfaceDescriptor* This,
				     const GLCulong inGlyphIndex,
				     GLfloat* outVec, const GLfloat inScaleX,
				     const GLfloat inScaleY,
				     const __GLCcontext* inContext);
GLfloat* __glcFaceDescGetAdvance(__GLCfaceDescriptor* This,
				 const GLCulong inGlyphIndex, GLfloat* outVec,
				 const GLfloat inScaleX, const GLfloat inScaleY,
				 const __GLCcontext* inContext);
const GLCchar8* __glcFaceDescGetFontFormat(const __GLCfaceDescriptor* This,
					   const __GLCcontext* inContext,
					   const GLCenum inAttrib);
GLfloat* __glcFaceDescGetMaxMetric(__GLCfaceDescriptor* This, GLfloat* outVec,
				   const __GLCcontext* inContext,
				   const GLfloat inScaleX,
				   const GLfloat inScaleY);
GLfloat* __glcFaceDescGetKerning(__GLCfaceDescriptor* This,
				 const GLCuint inGlyphIndex,
				 const GLCuint inPrevGlyphIndex,
				 const GLfloat inScaleX, const GLfloat inScaleY,
				 GLfloat* outVec,
				 const __GLCcontext* inContext);
GLCchar8* __glcFaceDescGetStyleName(__GLCfaceDescriptor* This);
GLboolean __glcFaceDescIsFixedPitch(__GLCfaceDescriptor* This);
GLboolean __glcFaceDescOutlineDecompose(const __GLCfaceDescriptor* This,
                                        __GLCrendererData* inData,
                                        const __GLCcontext* inContext);
GLboolean __glcFaceDescGetBitmapSize(const __GLCfaceDescriptor* This,
				     GLint* outWidth, GLint *outHeight,
				     const GLfloat inScaleX,
				     const GLfloat inScaleY,
				     GLint* outPixBoundingBox,
				     const int inFactor,
				     const __GLCcontext* inContext);
GLboolean __glcFaceDescGetBitmap(const __GLCfaceDescriptor* This,
				 const GLint inWidth, const GLint inHeight,
				 const void* inBuffer,
				 const __GLCcontext* inContext);
GLboolean __glcFaceDescOutlineEmpty(__GLCfaceDescriptor* This);
__GLCcharMap* __glcFaceDescGetCharMap(__GLCfaceDescriptor* This,
				      __GLCcontext* inContext);
#endif /* __glc_ofacedesc_h */
