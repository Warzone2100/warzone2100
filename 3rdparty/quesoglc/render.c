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
 * defines the so-called "Rendering commands" described in chapter 3.9 of the
 * GLC specs.
 */

/** \defgroup render Rendering commands
 * These are the commands that render characters to a GL render target. Those
 * commands gather glyph datas according to the parameters that has been set in
 * the state machine of GLC, and issue GL commands to render the characters
 * layout to the GL render target.
 *
 * When it renders a character, GLC finds a font that maps the character code
 * to a character such as LATIN CAPITAL LETTER A, then uses one or more glyphs
 * from the font to create a graphical layout that represents the character.
 * Finally, GLC issues a sequence of GL commands to draw the layout. Glyph
 * coordinates are defined in EM units and are transformed during rendering to
 * produce the desired mapping of the glyph shape into the GL window coordinate
 * system.
 *
 * If GLC cannot find a font that maps the character code in the list
 * \b GLC_CURRENT_FONT_LIST, it attemps to produce an alternate rendering. If
 * the value of the boolean variable \b GLC_AUTO_FONT is set to \b GL_TRUE, GLC
 * searches for a font that has the character that maps the character code. If
 * the search succeeds, the font's ID is appended to \b GLC_CURRENT_FONT_LIST
 * and the character is rendered.
 *
 * If there are fonts in the list \b GLC_CURRENT_FONT_LIST, but a match for
 * the character code cannot be found in any of those fonts, GLC goes through
 * the following steps :
 * -# If the value of the variable \b GLC_REPLACEMENT_CODE is nonzero,
 * GLC finds a font that maps the replacement code, and renders the character
 * that the replacement code is mapped to.
 * -# If the variable \b GLC_REPLACEMENT_CODE is zero, or if the replacement
 * code does not result in a match, GLC checks whether a callback function is
 * defined. If a callback function is defined for \b GLC_OP_glcUnmappedCode,
 * GLC calls the function. The callback function provides the character code to
 * the user and allows loading of the appropriate font. After the callback
 * returns, GLC tries to render the character code again.
 * -# Otherwise, the command attemps to render the character sequence
 * <em>\\\<hexcode\></em>, where \\ is the character REVERSE SOLIDUS (U+5C),
 * \< is the character LESS-THAN SIGN (U+3C), \> is the character GREATER-THAN
 * SIGN (U+3E), and \e hexcode is the character code represented as a sequence
 * of hexadecimal digits. The sequence has no leading zeros, and alphabetic
 * digits are in upper case. The GLC measurement commands treat the sequence
 * as a single character.
 *
 * \note The rendering commands may issue GL commands, hence a GL context must
 * be bound to the current thread such that the GLC commands produce the desired
 * result. It is the responsibility of the GLC client to set up the underlying
 * GL implementation.
 *
 * \note Some rendering commands create and/or use display lists and/or
 * textures. The IDs of those display lists and textures are stored in the
 * current GLC context but the display lists and the textures themselves are
 * managed by the current GL context. In order not to impact the performance of
 * error-free programs, QuesoGLC does not check if the current GL context is
 * the same than the one where the display lists and the textures were actually
 * created. If the current GL context has changed meanwhile, the result of
 * commands that refer to the corresponding display lists or textures is
 * undefined.
 */

#include "internal.h"

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <math.h>

#include "texture.h"



/* This internal function renders a glyph using the GLC_BITMAP format */
/* TODO : Render Bitmap fonts */
static void __glcRenderCharBitmap(const __GLCfont* inFont,
				  const __GLCcontext* inContext,
                                  const GLfloat inScaleX,
				  const GLfloat inScaleY,
				  const GLfloat* inAdvance,
				  const GLboolean inIsRTL)
{
  GLfloat *transform = inContext->bitmapMatrix;
  GLint pixWidth = 0, pixHeight = 0;
  GLubyte* pixBuffer = NULL;
  GLint pixBoundingBox[4] = {0, 0, 0, 0};

  __glcFontGetBitmapSize(inFont, &pixWidth, &pixHeight, inScaleX, inScaleY, 0,
			 pixBoundingBox, inContext);

  pixBuffer = (GLubyte *)__glcMalloc(pixWidth * pixHeight);
  if (!pixBuffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* render the glyph */
  if (!__glcFontGetBitmap(inFont, pixWidth, pixHeight, pixBuffer, inContext)) {
    __glcFree(pixBuffer);
    return;
  }

  /* Do the actual GL rendering */
  if (inIsRTL) {
    glBitmap(0, 0, 0, 0,
	     inAdvance[1] * transform[2] - inAdvance[0] * transform[0],
	     inAdvance[1] * transform[3] - inAdvance[0] * transform[1],
	     NULL);
    glBitmap(pixWidth, pixHeight, - pixBoundingBox[0] >> 6,
	     -pixBoundingBox[1] >> 6, 0., 0., pixBuffer);
  }
  else
    glBitmap(pixWidth, pixHeight, -pixBoundingBox[0] >> 6,
	     -pixBoundingBox[1] >> 6,
	     inAdvance[0] * transform[0] + inAdvance[1] * transform[2],
	     inAdvance[0] * transform[1] + inAdvance[1] * transform[3],
	     pixBuffer);

  __glcFree(pixBuffer);
}



/* This internal function renders a glyph using the GLC_PIXMAP_QSO format */
static void __glcRenderCharPixmap(const __GLCfont* inFont,
				  const __GLCcontext* inContext,
                                  const GLfloat scaleX, const GLfloat scaleY,
                                  const GLfloat* advance,
				  const GLboolean inIsRTL)
{
  GLfloat *transform = inContext->bitmapMatrix;
  GLint pixWidth = 0, pixHeight = 0;
  GLubyte* pixBuffer = NULL;
  GLint pixBoundingBox[4] = {0, 0, 0, 0};

  __glcFontGetBitmapSize(inFont, &pixWidth, &pixHeight, scaleX, scaleY, 0,
			 pixBoundingBox, inContext);

  pixBuffer = (GLubyte *)__glcMalloc(pixWidth * pixHeight);
  if (!pixBuffer) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return;
  }

  /* render the glyph */
  if (!__glcFontGetBitmap(inFont, pixWidth, pixHeight, pixBuffer, inContext)) {
    __glcFree(pixBuffer);
    return;
  }

  /* Do the actual GL rendering */
  if (inIsRTL) {
    glBitmap(0, 0, 0.f, 0.f,
	     advance[1] * transform[2] - advance[0] * transform[0] +
	     (pixBoundingBox[0] >> 6),
	     advance[1] * transform[3] - advance[0] * transform[1] +
	     (pixBoundingBox[1] >> 6),
	     NULL);

    glDrawPixels(pixWidth, pixHeight, GL_ALPHA, GL_UNSIGNED_BYTE, pixBuffer);

    glBitmap(0, 0, 0.f, 0.f,
	     -(pixBoundingBox[0] >> 6),
	     -(pixBoundingBox[1] >> 6),
	     NULL);
  }
  else {
    glBitmap(0, 0, 0.f, 0.f, 
	     pixBoundingBox[0] >> 6, 
	     pixBoundingBox[1] >> 6, 
	     NULL);

    glDrawPixels(pixWidth, pixHeight, GL_ALPHA, GL_UNSIGNED_BYTE, pixBuffer);

    glBitmap(0, 0, 0.f, 0.f,
	     advance[0] * transform[0] + advance[1] * transform[2] - 
	     (pixBoundingBox[0] >> 6),
	     advance[0] * transform[1] + advance[1] * transform[3] - 
	     (pixBoundingBox[1] >> 6),
	     NULL);
  }

  __glcFree(pixBuffer);
}



/* Internal function that is called to do the actual rendering :
 * 'inCode' must be given in UCS-4 format
 */
static void* __glcRenderChar(const GLint inCode, const GLint inPrevCode,
			     const GLboolean inIsRTL, const __GLCfont* inFont,
			     __GLCcontext* inContext,
			     const void* GLC_UNUSED_ARG(inData),
			     const GLboolean GLC_UNUSED_ARG(inMultipleChars))
{
  GLfloat transformMatrix[16];
  GLfloat scaleX = GLC_POINT_SIZE;
  GLfloat scaleY = GLC_POINT_SIZE;
  __GLCglyph* glyph = NULL;
  GLfloat sx64 = 0., sy64 = 0.;
  GLfloat advance[2] = {0., 0.};

  assert(inFont);

  __glcGetScale(inContext, transformMatrix, &scaleX, &scaleY);

  if ((fabs(scaleX) < GLC_EPSILON) || (fabs(scaleY) < GLC_EPSILON))
    return NULL;

#ifndef GLC_FT_CACHE
  if (!__glcFontOpen(inFont, inContext))
    return NULL;
#endif

  if (inPrevCode && inContext->enableState.kerning) {
    GLfloat kerning[2];
    GLint leftCode = inIsRTL ? inCode : inPrevCode;
    GLint rightCode = inIsRTL ? inPrevCode : inCode;

    if (__glcFontGetKerning(inFont, leftCode, rightCode, kerning, inContext,
			    scaleX, scaleY)) {
      if (inIsRTL)
	kerning[0] = -kerning[0];

      if ((inContext->renderState.renderStyle == GLC_BITMAP)
          || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO))
	glBitmap(0, 0, 0, 0,
		 kerning[0] * inContext->bitmapMatrix[0] 
		 + kerning[1] * inContext->bitmapMatrix[2],
		 kerning[0] * inContext->bitmapMatrix[1]
		 + kerning[1] * inContext->bitmapMatrix[3],
		 NULL);
      else
	glTranslatef(kerning[0], kerning[1], 0.f);
    }
  }

  if (!__glcFontGetAdvance(inFont, inCode, advance, inContext, scaleX,
			   scaleY)) {
#ifndef GLC_FT_CACHE
    __glcFontClose(inFont);
#endif
    return NULL;
  }

  /* Get and load the glyph which unicode code is identified by inCode */
  glyph = __glcFontGetGlyph(inFont, inCode, inContext);

  if (inContext->enableState.glObjects
      && !__glcFontPrepareGlyph(inFont, inContext, scaleX, scaleY,
				glyph->index)) {
#ifndef GLC_FT_CACHE
    __glcFontClose(inFont);
#endif
    return NULL;
  }

  sx64 = 64. * scaleX;
  sy64 = 64. * scaleY;

  if ((inContext->renderState.renderStyle != GLC_BITMAP)
      && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)) {
    if (inIsRTL)
      glTranslatef(-advance[0], advance[1], 0.f);

    /* If the outline contains no point then the glyph represents a space
     * character and there is no need to continue the process of rendering.
     */
    if (!__glcFontOutlineEmpty(inFont)) {
      /* Update the advance and return */
      if (!inIsRTL)
        glTranslatef(advance[0], advance[1], 0.f);
      if (inContext->enableState.glObjects)
	glyph->isSpacingChar = GL_TRUE;
#ifndef GLC_FT_CACHE
      __glcFontClose(inFont);
#endif
      return NULL;
    }

    /* coordinates are given in 26.6 fixed point integer hence we
     * divide the scale by 2^6
     */
    if (!inContext->enableState.glObjects)
      glScalef(1. / sx64, 1. / sy64, 1.f);
  }

  /* Call the appropriate function depending on the rendering mode */
  switch(inContext->renderState.renderStyle) {
  case GLC_BITMAP:
    __glcRenderCharBitmap(inFont, inContext, scaleX, scaleY, advance,
			  inIsRTL);
    break;
  case GLC_PIXMAP_QSO:
    __glcRenderCharPixmap(inFont, inContext, scaleX, scaleY, advance,
			  inIsRTL);
    break;
  case GLC_TEXTURE:
    __glcRenderCharTexture(inFont, inContext, scaleX, scaleY, glyph);
    break;
  case GLC_LINE:
    __glcRenderCharScalable(inFont, inContext, transformMatrix, scaleX,
			    scaleY, glyph);
    break;
  case GLC_TRIANGLE:
    __glcRenderCharScalable(inFont, inContext, transformMatrix, scaleX,
			    scaleY, glyph);
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
  }

  if ((inContext->renderState.renderStyle != GLC_BITMAP)
      && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)) {
    if (!inContext->enableState.glObjects)
      glScalef(sx64, sy64, 1.);
    if (!inIsRTL)
      glTranslatef(advance[0], advance[1], 0.f);
  }
#ifndef GLC_FT_CACHE
  __glcFontClose(inFont);
#endif
  return NULL;
}



/* This internal function is used by both glcRenderString() and
 * glcRenderCountedString(). The string 'inString' must be sorted in visual
 * order and stored using UCS4 format.
 */
static void __glcRenderCountedString(__GLCcontext* inContext,
				     const GLCchar32* inString,
				     const GLboolean inIsRightToLeft,
				     const GLint inCount)
{
  GLint listIndex = 0;
  GLint i = 0;
  const GLCchar32* ptr = NULL;
  __GLCglState GLState;
  __GLCcharacter prevCode = {0, NULL, NULL, {0.f, 0.f}};
  GLboolean saveGLObjects = GL_FALSE;
  GLint shift = 1;
  __GLCcharacter* chars = NULL;
  GLfloat pixmapColor[4];

  /* Disable the internal management of GL objects when the user is currently
   * building a display list.
   */
  glGetIntegerv(GL_LIST_INDEX, &listIndex);
  if (listIndex) {
    saveGLObjects = inContext->enableState.glObjects;
    inContext->enableState.glObjects = GL_FALSE;
  }


  /* Allocate a buffer to store the glyphes informations of the string to be
   * rendered.
   */
  if (inContext->enableState.glObjects
      && (inContext->renderState.renderStyle != GLC_BITMAP)
      && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)) {
    chars = (__GLCcharacter*)__glcMalloc(inCount * sizeof(__GLCcharacter));
    if (!chars) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return;
    }
  }

  /* Save the value of the GL parameters */
  __glcSaveGLState(&GLState, inContext, GL_FALSE);

  /* Set the vertex arrays parameters for GLC_LINE and GLC_TRIANGLE rendering
   * styles when GLC_GL_OBJECTS is enabled.
   */
  if (inContext->renderState.renderStyle == GLC_LINE ||
      (inContext->renderState.renderStyle == GLC_TRIANGLE
       && !(inContext->enableState.glObjects
	    && inContext->enableState.extrude))) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
  }

  if (inContext->renderState.renderStyle == GLC_TRIANGLE
      && inContext->enableState.glObjects && inContext->enableState.extrude)
    glEnable(GL_NORMALIZE);

  /* Set the texture environment if the render style is GLC_TEXTURE */
  if (inContext->renderState.renderStyle == GLC_TEXTURE) {
    /* Set the new values of the parameters */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (inContext->enableState.glObjects) {
      if (inContext->atlas.id)
	glBindTexture(GL_TEXTURE_2D, inContext->atlas.id);
      if (GLEW_ARB_vertex_buffer_object) {
	if (inContext->atlas.bufferObjectID) {
	  glBindBufferARB(GL_ARRAY_BUFFER_ARB, inContext->atlas.bufferObjectID);
	  glInterleavedArrays(GL_T2F_V3F, 0, NULL);
	}
      }
    }
    else if (inContext->texture.id) {
      glBindTexture(GL_TEXTURE_2D, inContext->texture.id);
      if (GLEW_ARB_pixel_buffer_object && inContext->texture.bufferObjectID)
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER,
			inContext->texture.bufferObjectID);
    }
  }

  if ((inContext->renderState.renderStyle == GLC_BITMAP)
      || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO)) {
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (inContext->renderState.renderStyle == GLC_PIXMAP_QSO) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glGetFloatv(GL_CURRENT_RASTER_COLOR, pixmapColor);
      glPixelTransferf(GL_RED_BIAS, pixmapColor[0]);
      glPixelTransferf(GL_GREEN_BIAS, pixmapColor[1]);
      glPixelTransferf(GL_BLUE_BIAS, pixmapColor[2]);
      glPixelTransferf(GL_ALPHA_BIAS, 0.f);
      glPixelTransferf(GL_RED_SCALE, 1.f);
      glPixelTransferf(GL_GREEN_SCALE, 1.f);
      glPixelTransferf(GL_BLUE_SCALE, 1.f);
      glPixelTransferf(GL_ALPHA_SCALE, pixmapColor[3]);
    }
  }

  /* Render the string */
  ptr = inString;
  if (inIsRightToLeft) {
    ptr += inCount - 1;
    shift = -1;
  }

  if (inContext->enableState.glObjects
      && (inContext->renderState.renderStyle != GLC_BITMAP)
      && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)) {
    __GLCfont* font = NULL;
    __GLCglyph* glyph = NULL;
    int length = 0;
    int j = 0;
    GLuint GLObjectIndex = inContext->renderState.renderStyle - 0x101;
    FT_ListNode node = NULL;
    float resolution = inContext->renderState.resolution / 72.;
    GLfloat orientation = 1.f;

    if (inContext->renderState.renderStyle == GLC_TRIANGLE
	&& inContext->enableState.extrude) {
      GLfloat transformMatrix[16];
      GLfloat scaleX = GLC_POINT_SIZE;
      GLfloat scaleY = GLC_POINT_SIZE;

      __glcGetScale(inContext, transformMatrix, &scaleX, &scaleY);

      if ((fabs(scaleX) < GLC_EPSILON) || (fabs(scaleY) < GLC_EPSILON))
	return;

      orientation = -transformMatrix[11];
      GLObjectIndex++;
    }

    glNormal3f(0.f, 0.f, 1.f / resolution);

    for (i = 0; i < inCount; i++) {
      if (*ptr >= 32) {
 	for (node = inContext->currentFontList.head; node ; node = node->next) {
 	  font = (__GLCfont*)node->data;
 	  glyph = __glcCharMapGetGlyph(font->charMap, *ptr);

 	  if (glyph) {
 	    if (!glyph->glObject[GLObjectIndex] && !glyph->isSpacingChar)
 	      continue;

	    if (!glyph->isSpacingChar
		&& (inContext->renderState.renderStyle == GLC_TEXTURE))
	      FT_List_Up(&inContext->atlasList,
			 (FT_ListNode)glyph->textureObject);

	    chars[length].glyph = glyph;
	    chars[length].advance[0] = glyph->advance[0];
	    chars[length].advance[1] = glyph->advance[1];

 	    if (inContext->enableState.kerning) {
 	      if (prevCode.code && prevCode.font == font) {
 		GLfloat kerning[2];
		GLint leftCode = inIsRightToLeft ? *ptr : prevCode.code;
		GLint rightCode = inIsRightToLeft ? prevCode.code : *ptr;

 		if (__glcFontGetKerning(font, leftCode, rightCode, kerning,
 					inContext, GLC_POINT_SIZE,
 					GLC_POINT_SIZE)) {
		  if (inIsRightToLeft)
		    kerning[0] = -kerning[0];

		  if (length) {
		    chars[length - 1].advance[0] += kerning[0];
		    chars[length - 1].advance[1] += kerning[1];
		  }
		  else
		    glTranslatef(kerning[0], kerning[1], 0.f);
 		}
 	      }
 	    }

	    prevCode.font = font;
	    prevCode.code = *ptr;

	    if (glyph->isSpacingChar)
	      chars[length].code = 32;
	    else
	      chars[length].code = *ptr;

 	    length++;
 	    break;
 	  }
 	}
      }

      if(!node || (i == inCount-1)) {
	glScalef(resolution, resolution, 1.f);

	for (j = 0; j < length; j++) {
	  if (inIsRightToLeft)
	    glTranslatef(-chars[j].advance[0], chars[j].advance[1], 0.);
	  if (chars[j].code != 32) {
	    glyph = chars[j].glyph;

	    switch(inContext->renderState.renderStyle) {
	    case GLC_TEXTURE:
	      if (GLEW_ARB_vertex_buffer_object)
		glDrawArrays(GL_QUADS, glyph->textureObject->position * 4, 4);
	      else
		glCallList(glyph->glObject[1]);
	      break;
	    case GLC_LINE:
	      if (GLEW_ARB_vertex_buffer_object) {
		int k = 0;

		glBindBufferARB(GL_ARRAY_BUFFER_ARB, glyph->glObject[0]);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		for (k = 0; k < glyph->nContour; k++)
		  glDrawArrays(GL_LINE_LOOP, glyph->contours[k],
			       glyph->contours[k+1] - glyph->contours[k]);
		break;
	      }
	      glCallList(glyph->glObject[0]);
	      break;
	    case GLC_TRIANGLE:
	      if (GLEW_ARB_vertex_buffer_object) {
		int k = 0;
		GLboolean extrude = GL_FALSE;

		glBindBufferARB(GL_ARRAY_BUFFER_ARB, glyph->glObject[0]);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				glyph->glObject[2]);
		glVertexPointer(2, GL_FLOAT, 0, NULL);

		do {
		  GLuint* vertexIndices = NULL;

		  if (orientation > 0.f) {
		    for (k = 0; k < glyph->nGeomBatch; k++) {
		      glDrawRangeElements(glyph->geomBatches[k].mode,
					  glyph->geomBatches[k].start,
					  glyph->geomBatches[k].end,
					  glyph->geomBatches[k].length,
					  GL_UNSIGNED_INT, vertexIndices);
		      vertexIndices += glyph->geomBatches[k].length;
		    }
		  }

		  if (inContext->enableState.extrude) {
		    if (extrude) {
		      glTranslatef(0.f, 0.f, 1.f);
		      glBindBufferARB(GL_ARRAY_BUFFER_ARB,
				      glyph->glObject[3]);
		      glInterleavedArrays(GL_N3F_V3F, 0, NULL);

		      for (k = 0; k < glyph->nContour; k++)
			glDrawArrays(GL_TRIANGLE_STRIP, glyph->contours[k] * 2,
				     (glyph->contours[k+1] - glyph->contours[k]
				      + 1) * 2);
		      glNormal3f(0.f, 0.f, 1.f / resolution);
		    }
		    else {
		      glNormal3f(0.f, 0.f, -1.f / resolution);
		      glTranslatef(0.f, 0.f, -1.f);
		      orientation = -orientation;
		    }
		    extrude = (!extrude);
		  }
		} while(extrude);
	      }
	      else
		glCallList(glyph->glObject[GLObjectIndex]);

	      break;
	    }
	  }
	  if (!inIsRightToLeft)
	    glTranslatef(chars[j].advance[0], chars[j].advance[1], 0.);
	}

	if (!node)
	  __glcProcessChar(inContext, *ptr, &prevCode, inIsRightToLeft,
			   __glcRenderChar, NULL);

	glScalef(1./resolution, 1./resolution, 1.f);
	length = 0;
      }

      ptr += shift;
    }
  }
  else {
    glNormal3f(0.f, 0.f, 1.f);

    for (i = 0; i < inCount; i++) {
      if (*ptr >= 32)
	__glcProcessChar(inContext, *ptr, &prevCode, inIsRightToLeft,
			 __glcRenderChar, NULL);
      ptr += shift;
    }
  }

  /* Restore the values of the GL state if needed */
  __glcRestoreGLState(&GLState, inContext, GL_FALSE);

  if ((inContext->renderState.renderStyle != GLC_BITMAP)
      && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)
      && inContext->enableState.glObjects)
      __glcFree(chars);

  if (listIndex)
    inContext->enableState.glObjects = saveGLObjects;
}



/** \ingroup render
 *  This command renders the character that \e inCode is mapped to.
 *  \param inCode The character to render
 *  \sa glcRenderString()
 *  \sa glcRenderCountedString()
 *  \sa glcReplacementCode()
 *  \sa glcRenderStyle()
 *  \sa glcCallbackFunc()
 */
void APIENTRY glcRenderChar(GLint inCode)
{
  __GLCcontext *ctx = NULL;
  GLint code = 0;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the character code converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(ctx, inCode);
  if (code < 32)
    return; /* Skip control characters and unknown characters */

  __glcRenderCountedString(ctx, (GLCchar32*)&code, GL_FALSE, 1);
}



/** \ingroup render
 *  This command is identical to the command glcRenderChar(), except that it
 *  renders a string of characters. The string comprises the first \e inCount
 *  elements of the array \e inString, which need not be followed by a zero
 *  element.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inCount is less than zero.
 *  \param inCount The number of elements in the string to be rendered
 *  \param inString The array of characters from which to render \e inCount
 *                  elements.
 *  \sa glcRenderChar()
 *  \sa glcRenderString()
 */
void APIENTRY glcRenderCountedString(GLint inCount, const GLCchar *inString)
{
  __GLCcontext *ctx = NULL;
  GLCchar32* UinString = NULL;
  GLboolean isRightToLeft = GL_FALSE;

  GLC_INIT_THREAD();

  /* Check if inCount is positive */
  if (inCount < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return;

  /* Creates a Unicode string based on the current string type. Basically,
   * that means that inString is read in the current string format.
   */
  UinString = __glcConvertCountedStringToVisualUcs4(ctx, &isRightToLeft,
						    inString, inCount);
  if (!UinString)
    return;


  __glcRenderCountedString(ctx, UinString, isRightToLeft, inCount);
}



/** \ingroup render
 *  This command is identical to the command glcRenderCountedString(), except
 *  that \e inString is zero terminated, not counted.
 *  \param inString A zero-terminated string of characters.
 *  \sa glcRenderChar()
 *  \sa glcRenderCountedString()
 */
void APIENTRY glcRenderString(const GLCchar *inString)
{
  __GLCcontext *ctx = NULL;
  GLCchar32* UinString = NULL;
  GLboolean isRightToLeft = GL_FALSE;
  GLint length = 0;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a context state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* If inString is NULL then there is no point in continuing */
  if (!inString)
    return;

  /* Creates a Unicode string based on the current string type. Basically,
   * that means that inString is read in the current string format.
   */
  UinString = __glcConvertToVisualUcs4(ctx, &isRightToLeft, &length, inString);
  if (!UinString)
    return;

  __glcRenderCountedString(ctx, UinString, isRightToLeft, length);
}



/** \ingroup render
 *  This command assigns the value \e inStyle to the variable
 *  \b GLC_RENDER_STYLE. Legal values for \e inStyle are defined in the table
 *  below :
 *  <center>
 *  <table>
 *  <caption>Rendering styles</caption>
 *    <tr>
 *      <td>Name</td> <td>Enumerant</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_BITMAP</b></td> <td>0x0100</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_LINE</b></td> <td>0x0101</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_TEXTURE</b></td> <td>0x0102</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_TRIANGLE</b></td> <td>0x0103</td>
 *    </tr>
 *    <tr>
 *      <td><b>GLC_PIXMAP_QSO</b></td> <td>0x8011</td>
 *    </tr>
 *  </table>
 *  </center>
 *  \param inStyle The value to assign to the variable \b GLC_RENDER_STYLE.
 *  \sa glcGeti() with argument \b GLC_RENDER_STYLE
 */
void APIENTRY glcRenderStyle(GLCenum inStyle)
{
  __GLCcontext *ctx = NULL;

  GLC_INIT_THREAD();

  /* Check if inStyle has a legal value */
  switch(inStyle) {
  case GLC_BITMAP:
  case GLC_LINE:
  case GLC_TEXTURE:
  case GLC_TRIANGLE:
  case GLC_PIXMAP_QSO:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a current state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the rendering style */
  ctx->renderState.renderStyle = inStyle;
  return;
}



/** \ingroup render
 *  This command assigns the value \e inCode to the variable
 *  \b GLC_REPLACEMENT_CODE. The replacement code is the code which is used
 *  whenever glcRenderChar() can not find a font that owns a character which
 *  the parameter \e inCode of glcRenderChar() maps to.
 *  \param inCode An integer to assign to \b GLC_REPLACEMENT_CODE.
 *  \sa glcGeti() with argument \b GLC_REPLACEMENT_CODE
 *  \sa glcRenderChar()
 */
void APIENTRY glcReplacementCode(GLint inCode)
{
  __GLCcontext *ctx = NULL;
  GLint code = 0;

  GLC_INIT_THREAD();

  /* Check if the current thread owns a current state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Get the replacement character converted to the UCS-4 format */
  code = __glcConvertGLintToUcs4(ctx, inCode);
  if (code < 0)
    return;

  /* Stores the replacement code */
  ctx->stringState.replacementCode = code;
  return;
}



/** \ingroup render
 *  This command assigns the value \e inVal to the variable \b GLC_RESOLUTION.
 *  It is used to compute the size of characters in pixels from the size in
 *  points.
 *
 *  The resolution is given in \e dpi (dots per inch). If \e inVal is zero, the
 *  resolution defaults to 72 dpi.
 *
 *  The command raises \b GLC_PARAMETER_ERROR if \e inVal is negative.
 *  \param inVal A floating point number to be used as resolution.
 *  \sa glcGetf() with argument GLC_RESOLUTION
 */
void APIENTRY glcResolution(GLfloat inVal)
{
  __GLCcontext *ctx = NULL;
  FT_ListNode node = NULL;

  GLC_INIT_THREAD();

  /* Negative resolutions are illegal */
  if (inVal < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a current state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the resolution */
  ctx->renderState.resolution = (inVal < GLC_EPSILON) ? 72. : inVal;

  /* Force the measurement caches to be updated */
  for (node = ctx->fontList.head; node; node = node->next) {
    __GLCfont* font = (__GLCfont*)node->data;
    __GLCfaceDescriptor* faceDesc = font->faceDesc;
    FT_ListNode glyphNode = NULL;

    font->maxMetricCached = GL_FALSE;

    for (glyphNode = faceDesc->glyphList.head; glyphNode;
	 glyphNode = glyphNode->next) {
      __GLCglyph* glyph = (__GLCglyph*)glyphNode->data;

      glyph->advanceCached = GL_FALSE;
      glyph->boundingBoxCached = GL_FALSE;
    }
  }

  return;
}



/** \ingroup render
 *  This command assigns the value \b inVal to the floating point variable
 *  identified by \e inAttrib which must be chosen in the table below.
 *
 *  - \b GLC_PARAMETRIC_TOLERANCE_QSO specifies the maximum distance, in object
 *    space, between the tesselation line contours and the curves they
 *    approximate. This parameter is only relevant for the \b GLC_LINE and
 *    \b GLC_TRIANGLE rendering types.
 *
 *  \param inAttrib A symbolic constant indicating a GLC attribute.
 *  \param inValue A floating point number to be used as tolerance.
 *  \sa glcGetf() with argument GLC_PARAMETRIC_TOLERANCE_QSO
 */
void APIENTRY glcRenderParameterfQSO(GLenum inAttrib, GLfloat inVal)
{
  __GLCcontext *ctx = NULL;

  GLC_INIT_THREAD();

  /* Check if inAttrib has a legal value */
  switch(inAttrib) {
  case GLC_PARAMETRIC_TOLERANCE_QSO:
    break;
  default:
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  if (inVal <= 0.f) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return;
  }

  /* Check if the current thread owns a current state */
  ctx = GLC_GET_CURRENT_CONTEXT();
  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return;
  }

  /* Stores the tolerance */
  ctx->renderState.tolerance = inVal;
  return;
}
