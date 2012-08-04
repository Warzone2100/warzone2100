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

/* Defines the methods of an object that is intended to managed glyphs */

/** \file
 * defines the object __GLCglyph which caches all the data needed for a given
 * glyph : display list, texture, bounding box, advance, index in the font
 * file, etc.
 */

#include "internal.h"
#include "texture.h"



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the index of the glyph in the font file and its Unicode
 * codepoint.
 */
__GLCglyph* __glcGlyphCreate(const GLCulong inIndex, const GLCulong inCode)
{
  __GLCglyph* This = NULL;

  This = (__GLCglyph*)__glcMalloc(sizeof(__GLCglyph));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCglyph));

  This->node.data = This;
  This->index = inIndex;
  This->codepoint = inCode;
  This->isSpacingChar = GL_FALSE;
  This->advanceCached = GL_FALSE;
  This->boundingBoxCached = GL_FALSE;

  return This;
}



/* Destructor of the object */
void __glcGlyphDestroy(__GLCglyph* This, __GLCcontext* inContext)
{
  __glcGlyphDestroyGLObjects(This, inContext);
  __glcFree(This);
}



/* Remove all GL objects related to the texture of the glyph */
void __glcGlyphDestroyTexture(__GLCglyph* This, const __GLCcontext* inContext)
{
  if (!inContext->isInGlobalCommand && !GLEW_ARB_vertex_buffer_object)
    glDeleteLists(This->glObject[1], 1);
  This->glObject[1] = 0;
  This->textureObject = NULL;
}



/* This function destroys the display lists and the texture objects that
 * are associated with a glyph.
 */
void __glcGlyphDestroyGLObjects(__GLCglyph* This, __GLCcontext* inContext)
{
  if (This->glObject[1]) {
    __glcReleaseAtlasElement(This->textureObject, inContext);
    __glcGlyphDestroyTexture(This, inContext);
  }

  if (!inContext->isInGlobalCommand) {
    if (This->glObject[0]) {
      if (GLEW_ARB_vertex_buffer_object) {
	glDeleteBuffersARB(1, &This->glObject[0]);
	if (This->contours)
	  __glcFree(This->contours);
	This->nContour = 0;
	This->contours = NULL;
      }
      else
	glDeleteLists(This->glObject[0], 1);
    }

    if (This->glObject[2]) {
      if (GLEW_ARB_vertex_buffer_object) {
	glDeleteBuffersARB(1, &This->glObject[2]);
	if (This->geomBatches)
	  __glcFree(This->geomBatches);
	This->nGeomBatch = 0;
	This->geomBatches = NULL;
      }
      else
	glDeleteLists(This->glObject[2], 1);
    }

    if (This->glObject[3]) {
      if (GLEW_ARB_vertex_buffer_object)
	glDeleteBuffersARB(1, &This->glObject[3]);
      else
	glDeleteLists(This->glObject[3], 1);
    }

    memset(This->glObject, 0, 4 * sizeof(GLuint));
  }
}



/* Returns the number of display that has been built for a glyph */
int __glcGlyphGetDisplayListCount(const __GLCglyph* This)
{
  int i = 0;
  int count = 0;

  if (GLEW_ARB_vertex_buffer_object)
    return 0;

  for (i = 0; i < 4; i++) {
    if (This->glObject[i])
      count++;
  }

  return count;
}



/* Returns the ID of the inCount-th display list that has been built for a
 * glyph.
 */
GLuint __glcGlyphGetDisplayList(const __GLCglyph* This, const int inCount)
{
  int i = 0;
  int count = inCount;

  assert(inCount >= 0);
  assert(inCount < __glcGlyphGetDisplayListCount(This));

  if (GLEW_ARB_vertex_buffer_object)
    return 0;

  for (i = 0; i < 4; i++) {
    GLuint displayList = This->glObject[i];

    if (displayList) {
      if (!count)
	return displayList;
      count--;
    }
  }

  /* The program is not supposed to reach the end of the function.
   * The following return is there to prevent the compiler to issue
   * a warning about 'control reaching the end of a non-void function'.
   */
  return 0xdeadbeef;
}



/* Returns the number of buffer objects that has been built for a glyph */
int __glcGlyphGetBufferObjectCount(const __GLCglyph* This)
{
  int i = 0;
  int count = 0;

  assert(GLEW_ARB_vertex_buffer_object);

  for (i = 0; i < 4; i++) {
    if (i == 1)
      continue;

    if (This->glObject[i])
      count++;
  }

  return count;
}



/* Returns the ID of the inCount-th buffer object that has been built for a
 * glyph.
 */
GLuint __glcGlyphGetBufferObject(const __GLCglyph* This, const int inCount)
{
  int i = 0;
  int count = inCount;

  assert(GLEW_ARB_vertex_buffer_object);
  assert(inCount >= 0);
  assert(inCount < __glcGlyphGetBufferObjectCount(This));

  for (i = 0; i < 4; i++) {
    GLuint bufferObject = This->glObject[i];

    if (i == 1)
      continue;

    if (bufferObject) {
      if (!count)
	return bufferObject;
      count--;
    }
  }

  /* The program is not supposed to reach the end of the function.
   * The following return is there to prevent the compiler to issue
   * a warning about 'control reaching the end of a non-void function'.
   */
  return 0xdeadbeef;
}
