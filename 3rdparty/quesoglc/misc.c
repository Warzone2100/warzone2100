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
 * defines miscellaneous utility routines used throughout QuesoGLC.
 */

#include <math.h>

#include "internal.h"


#ifdef DEBUGMODE
GLuint __glcMemAllocCount = 0;
GLuint __glcMemAllocTrigger = 0;
GLboolean __glcMemAllocFailOnce = GL_TRUE;


/* QuesoGLC own allocation and memory management routines */
void* __glcMalloc(size_t size)
{
  __glcMemAllocCount++;

  if (__glcMemAllocFailOnce) {
    if (__glcMemAllocCount == __glcMemAllocTrigger)
      return NULL;
  }
  else if (__glcMemAllocCount >= __glcMemAllocTrigger)
    return NULL;

  return malloc(size);
}

void __glcFree(void *ptr)
{
  /* Not all implementations of free() accept NULL. Moreover this allows to
   * detect useless calls.
   */
  assert(ptr);

  free(ptr);
}

void* __glcRealloc(void *ptr, size_t size)
{
  __glcMemAllocCount++;

  if (__glcMemAllocFailOnce) {
    if (__glcMemAllocCount == __glcMemAllocTrigger)
      return NULL;
  }
  else if (__glcMemAllocCount >= __glcMemAllocTrigger)
    return NULL;

  return realloc(ptr, size);
}
#endif



#ifndef HAVE_TLS
/* Each thread has to store specific informations so they can be retrieved
 * later. __glcGetThreadArea() returns a struct which contains thread specific
 * info for GLC.
 * If the '__GLCthreadArea' of the current thread does not exist, it is created
 * and initialized.
 * IMPORTANT NOTE : __glcGetThreadArea() must never use __glcMalloc() and
 *    __glcFree() since those functions could use the exceptContextStack
 *    before it is initialized.
 */
__GLCthreadArea* __glcGetThreadArea(void)
{
  __GLCthreadArea *area = NULL;

#ifdef __WIN32__
  area = (__GLCthreadArea*)TlsGetValue(__glcCommonArea.threadKey);
#else
  area = (__GLCthreadArea*)pthread_getspecific(__glcCommonArea.threadKey);
#endif
  if (area)
    return area;

  area = (__GLCthreadArea*)malloc(sizeof(__GLCthreadArea));
  if (!area)
    return NULL;

  area->currentContext = NULL;
  area->errorState = GLC_NONE;
  area->lockState = 0;
  area->exceptionStack.head = NULL;
  area->exceptionStack.tail = NULL;
  area->failedTry = GLC_NO_EXC;
#ifdef __WIN32__
  if (!TlsSetValue(__glcCommonArea.threadKey, (LPVOID)area)) {
    free(area);
    return NULL;
  }
#else
  pthread_setspecific(__glcCommonArea.threadKey, (void*)area);
#endif

#ifdef __WIN32__
  if (__glcCommonArea.threadID == GetCurrentThreadId())
#else
  if (pthread_equal(__glcCommonArea.threadID, pthread_self()))
#endif
    __glcThreadArea = area;
  return area;
}



/* Raise an error. This function must be called each time the current error
 * of the issuing thread must be set
 */
void __glcRaiseError(GLCenum inError)
{
  GLCenum error = GLC_NONE;
  __GLCthreadArea *area = NULL;

  area = GLC_GET_THREAD_AREA();
  assert(area);

  /* An error can only be raised if the current error value is GLC_NONE.
   * However, when inError == GLC_NONE then we must force the current error
   * value to GLC_NONE whatever its previous value was.
   */
  error = area->errorState;
  if (!error || !inError)
    area->errorState = inError;
}



/* Get the current context of the issuing thread */
__GLCcontext* __glcGetCurrent(void)
{
  __GLCthreadArea *area = NULL;

  area = __glcGetThreadArea(); /* Don't use GLC_GET_THREAD_AREA */
  assert(area);

  return area->currentContext;
}
#endif /* HAVE_TLS */



/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code and '<hexcode>' format
 * are issued if necessary. The previous code is updated accordingly.
 * 'inCode' must be given in UCS-4 format
 */
void* __glcProcessChar(__GLCcontext *inContext, const GLint inCode,
		       __GLCcharacter* inPrevCode, const GLboolean inIsRTL,
		       const __glcProcessCharFunc inProcessCharFunc,
		       const void* inProcessCharData)
{
  GLint repCode = 0;
  __GLCfont* font = NULL;
  void* ret = NULL;

  if (!inCode)
    return NULL;

  /* Get a font that maps inCode */
  font = __glcContextGetFont(inContext, inCode);
  if (font) {
    /* A font has been found */
    if (font != inPrevCode->font)
      inPrevCode->code = 0; /* The font has changed, kerning must be disabled */
    ret = inProcessCharFunc(inCode, inPrevCode->code, inIsRTL, font, inContext,
			    inProcessCharData, GL_FALSE);
    inPrevCode->code = inCode;
    inPrevCode->font = font;
    return ret;
  }

  /* __glcContextGetFont() can not find a font that maps inCode, we then attempt
   * to produce an alternate rendering.
   */

  /* If the variable GLC_REPLACEMENT_CODE is nonzero, and __glcContextGetFont()
   * finds a font that maps the replacement code, we now render the character
   * that the replacement code is mapped to
   */
  repCode = inContext->stringState.replacementCode;
  font = __glcContextGetFont(inContext, repCode);
  if (repCode && font) {
    if (font != inPrevCode->font)
      inPrevCode->code = 0; /* The font has changed, kerning must be disabled */
    ret = inProcessCharFunc(repCode, inPrevCode->code, inIsRTL, font, inContext,
			    inProcessCharData, GL_FALSE);
    inPrevCode->code = repCode;
    inPrevCode->font = font;
    return ret; 
  }
  else {
    /* If we get there, we failed to render both the character that inCode maps
     * to and the replacement code. Now, we will try to render the character
     * sequence "\<hexcode>", where '\' is the character REVERSE SOLIDUS 
     * (U+5C), '<' is the character LESS-THAN SIGN (U+3C), '>' is the character
     * GREATER-THAN SIGN (U+3E), and 'hexcode' is inCode represented as a
     * sequence of hexadecimal digits. The sequence has no leading zeros, and
     * alphabetic digits are in upper case. The GLC measurement commands treat
     * the sequence as a single character.
     */
    char buf[11];
    GLint i = 0;
    GLint n = 0;

    /* Check if a font maps hexadecimal digits */
#ifdef _MSC_VER
    n = sprintf_s(buf, 11, "\\<%X>", (int)inCode);
    if (n < 0) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }
#else
    n = snprintf(buf, 11, "\\<%X>", (int)inCode);
#endif
    for (i = 0; i < n; i++) {
      if (!__glcContextGetFont(inContext, buf[i]))
	/* The code is not rendered, the previous code is thus left unchanged */
	return NULL;
    }

    /* Render the '\<hexcode>' sequence */
    for (i = 0; i < n; i++) {
      GLint pos = inIsRTL ? n-i-1 : i;

      font = __glcContextGetFont(inContext, buf[pos]);
      if (font != inPrevCode->font)
	inPrevCode->code = 0; /*The font has changed, kerning must be disabled*/
      ret = inProcessCharFunc(buf[pos], inPrevCode->code, inIsRTL, font,
			      inContext, inProcessCharData, GL_TRUE);
      inPrevCode->code = buf[pos];
      inPrevCode->font = font;
    }
    return ret;
  }
}



/* Store an 4x4 identity matrix in 'm' */
static void __glcMakeIdentity(GLfloat* m)
{
  memset(m, 0, 16 * sizeof(GLfloat));
  m[0] = 1.f;
  m[5] = 1.f;
  m[10] = 1.f;
  m[15] = 1.f;
}



/* Invert a 4x4 matrix stored in inMatrix. The result is stored in outMatrix
 * It uses the Gauss-Jordan elimination method
 */
static GLboolean __glcInvertMatrix(const GLfloat* inMatrix, GLfloat* outMatrix)
{
  int i, j, k, swap;
  GLfloat t;
  GLfloat temp[4][4];

  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) {
      temp[i][j] = inMatrix[i*4+j];
    }
  }
  __glcMakeIdentity(outMatrix);

  for (i = 0; i < 4; i++) {
    /* Look for largest element in column */
    swap = i;
    for (j = i + 1; j < 4; j++) {
      if (fabs(temp[j][i]) > fabs(temp[i][i])) {
	swap = j;
      }
    }

    if (swap != i) {
      /* Swap rows */
      for (k = 0; k < 4; k++) {
	t = temp[i][k];
	temp[i][k] = temp[swap][k];
	temp[swap][k] = t;

	t = outMatrix[i*4+k];
	outMatrix[i*4+k] = outMatrix[swap*4+k];
	outMatrix[swap*4+k] = t;
      }
    }

    if (fabs(temp[i][i]) < GLC_EPSILON) {
      /* No non-zero pivot. The matrix is singular, which shouldn't
       * happen. This means the user gave us a bad matrix.
       */
      return GL_FALSE;
    }

    t = temp[i][i];
    for (k = 0; k < 4; k++) {
      temp[i][k] /= t;
      outMatrix[i*4+k] /= t;
    }
    for (j = 0; j < 4; j++) {
      if (j != i) {
	t = temp[j][i];
	for (k = 0; k < 4; k++) {
	  temp[j][k] -= temp[i][k]*t;
	  outMatrix[j*4+k] -= outMatrix[i*4+k]*t;
	}
      }
    }
  }
  return GL_TRUE;
}



/* Mutiply two 4x4 matrices, the operands are stored in inMatrix1 and inMatrix2
 * The result is stored in outMatrix which can be neither inMatrix1 nor
 * inMatrix2.
 */
static void __glcMultMatrices(const GLfloat* inMatrix1,
			      const GLfloat* inMatrix2, GLfloat* outMatrix)
{
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      outMatrix[i*4+j] = 
	inMatrix1[i*4+0]*inMatrix2[0*4+j] +
	inMatrix1[i*4+1]*inMatrix2[1*4+j] +
	inMatrix1[i*4+2]*inMatrix2[2*4+j] +
	inMatrix1[i*4+3]*inMatrix2[3*4+j];
    }
  }
}



/* Compute an optimal size for the glyph to be rendered on the screen if no
 * display list is planned to be built.
 */
void __glcGetScale(const __GLCcontext* inContext, GLfloat* outTransformMatrix,
		   GLfloat* outScaleX, GLfloat* outScaleY)
{
  int i = 0;

  if ((inContext->renderState.renderStyle != GLC_BITMAP)
      && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)) {
    /* Compute the matrix that transforms object space coordinates to viewport
     * coordinates. If we plan to use object space coordinates, this matrix is
     * set to identity.
     */
    GLfloat projectionMatrix[16];
    GLfloat modelviewMatrix[16];
    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelviewMatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);

    __glcMultMatrices(modelviewMatrix, projectionMatrix, outTransformMatrix);

    if (!inContext->enableState.glObjects && inContext->enableState.hinting) {
      GLfloat rs[16], m[16];
      /* Get the scale factors in each X, Y and Z direction */
      GLfloat sx = sqrt(outTransformMatrix[0] * outTransformMatrix[0]
			+outTransformMatrix[1] * outTransformMatrix[1]
			+outTransformMatrix[2] * outTransformMatrix[2]);
      GLfloat sy = sqrt(outTransformMatrix[4] * outTransformMatrix[4]
			+outTransformMatrix[5] * outTransformMatrix[5]
			+outTransformMatrix[6] * outTransformMatrix[6]);
      GLfloat sz = sqrt(outTransformMatrix[8] * outTransformMatrix[8]
			+outTransformMatrix[9] * outTransformMatrix[9]
			+outTransformMatrix[10] * outTransformMatrix[10]);
      GLfloat x = 0., y = 0.;

      memset(rs, 0, 16 * sizeof(GLfloat));
      rs[15] = 1.;
      for (i = 0; i < 3; i++) {
	rs[0+4*i] = outTransformMatrix[0+4*i] / sx;
	rs[1+4*i] = outTransformMatrix[1+4*i] / sy;
	rs[2+4*i] = outTransformMatrix[2+4*i] / sz;
      }
      if (!__glcInvertMatrix(rs, rs)) {
	*outScaleX = 0.f;
	*outScaleY = 0.f;
	return;
      }

      __glcMultMatrices(rs, outTransformMatrix, m);
      x = ((m[0] + m[12])/(m[3] + m[15]) - m[12]/m[15]) * viewport[2] * 0.5;
      y = ((m[1] + m[13])/(m[3] + m[15]) - m[13]/m[15]) * viewport[3] * 0.5;
      *outScaleX = sqrt(x*x+y*y);
      x = ((m[4] + m[12])/(m[7] + m[15]) - m[12]/m[15]) * viewport[2] * 0.5;
      y = ((m[5] + m[13])/(m[7] + m[15]) - m[13]/m[15]) * viewport[3] * 0.5;
      *outScaleY = sqrt(x*x+y*y);
    }
    else {
      *outScaleX = GLC_POINT_SIZE;
      *outScaleY = GLC_POINT_SIZE;
    }
  }
  else {
    GLfloat determinant = 0., norm = 0.;
    GLfloat *transform = inContext->bitmapMatrix;

    /* Compute the norm of the transformation matrix */
    for (i = 0; i < 4; i++) {
      if (fabsf(transform[i]) > norm)
	norm = fabsf(transform[i]);
    }

    determinant = transform[0] * transform[3] - transform[1] * transform[2];

    /* If the transformation is degenerated, nothing needs to be rendered */
    if (fabsf(determinant) < norm * GLC_EPSILON) {
      *outScaleX = 0.f;
      *outScaleY = 0.f;
      return;
    }

    if (inContext->enableState.hinting) {
      *outScaleX = sqrt(transform[0]*transform[0]+transform[1]*transform[1]);
      *outScaleY = sqrt(transform[2]*transform[2]+transform[3]*transform[3]);
    }
    else {
      *outScaleX = GLC_POINT_SIZE;
      *outScaleY = GLC_POINT_SIZE;
    }
  }
}



/* Save the GL State in a structure */
void __glcSaveGLState(__GLCglState* inGLState, const __GLCcontext* inContext,
		      const GLboolean inAll)
{
  if (inAll || inContext->renderState.renderStyle == GLC_TEXTURE) {
    inGLState->blend = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC, &inGLState->blendSrc);
    glGetIntegerv(GL_BLEND_DST, &inGLState->blendDst);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &inGLState->textureID);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
		  &inGLState->textureEnvMode);
    if ((inAll || !inContext->enableState.glObjects)
	&& GLEW_ARB_pixel_buffer_object)
      glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB,
		    &inGLState->pixelBufferObjectID);
  }

  if (inAll || (inContext->renderState.renderStyle == GLC_BITMAP)
      || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO)) {
    glGetIntegerv(GL_UNPACK_LSB_FIRST, &inGLState->unpackLsbFirst);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &inGLState->unpackRowLength);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &inGLState->unpackSkipPixels);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &inGLState->unpackSkipRows);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &inGLState->unpackAlignment);

    if (inAll || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO)) {
      if (!inAll) { /* if inAll, already saved in GLC_TEXTURE's block */
        inGLState->blend = glIsEnabled(GL_BLEND);
        glGetIntegerv(GL_BLEND_SRC, &inGLState->blendSrc);
        glGetIntegerv(GL_BLEND_DST, &inGLState->blendDst);
      }
      glGetFloatv(GL_RED_BIAS, &inGLState->colorBias[0]);
      glGetFloatv(GL_GREEN_BIAS, &inGLState->colorBias[1]);
      glGetFloatv(GL_BLUE_BIAS, &inGLState->colorBias[2]);
      glGetFloatv(GL_ALPHA_BIAS, &inGLState->colorBias[3]);
      glGetFloatv(GL_RED_SCALE, &inGLState->colorScale[0]);
      glGetFloatv(GL_GREEN_SCALE, &inGLState->colorScale[1]);
      glGetFloatv(GL_BLUE_SCALE, &inGLState->colorScale[2]);
      glGetFloatv(GL_ALPHA_SCALE, &inGLState->colorScale[3]);
    }
  }

  if ((inAll || (inContext->enableState.glObjects
		 && (inContext->renderState.renderStyle != GLC_BITMAP)
		 && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)))
      && GLEW_ARB_vertex_buffer_object) {
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB,
		  &inGLState->vertexBufferObjectID);
    if (inAll || (inContext->renderState.renderStyle == GLC_TRIANGLE))
      glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB,
		    &inGLState->elementBufferObjectID);
  }

  if (inAll || (inContext->renderState.renderStyle == GLC_TRIANGLE
		&& inContext->enableState.glObjects
		&& inContext->enableState.extrude))
    inGLState->normalize = glIsEnabled(GL_NORMALIZE);

  if (inAll || inContext->renderState.renderStyle == GLC_LINE
      || inContext->renderState.renderStyle == GLC_TRIANGLE
      || (inContext->renderState.renderStyle == GLC_TEXTURE
	  && inContext->enableState.glObjects
	  && GLEW_ARB_vertex_buffer_object)) {
    inGLState->vertexArray = glIsEnabled(GL_VERTEX_ARRAY);
    if (inGLState->vertexArray == GL_TRUE) {
      glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &inGLState->vertexArraySize);
      glGetIntegerv(GL_VERTEX_ARRAY_TYPE, &inGLState->vertexArrayType);
      glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &inGLState->vertexArrayStride);
      glGetPointerv(GL_VERTEX_ARRAY_POINTER, &inGLState->vertexArrayPointer);
    }
    inGLState->normalArray = glIsEnabled(GL_NORMAL_ARRAY);
    inGLState->colorArray = glIsEnabled(GL_COLOR_ARRAY);
    inGLState->indexArray = glIsEnabled(GL_INDEX_ARRAY);
    inGLState->texCoordArray = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
    if ((inAll || inContext->renderState.renderStyle == GLC_TEXTURE) && inGLState->texCoordArray == GL_TRUE) {
      glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &inGLState->texCoordArraySize);
      glGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE, &inGLState->texCoordArrayType);
      glGetIntegerv(GL_TEXTURE_COORD_ARRAY_STRIDE,
		    &inGLState->texCoordArrayStride);
      glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER,
		    &inGLState->texCoordArrayPointer);
    }
    inGLState->edgeFlagArray = glIsEnabled(GL_EDGE_FLAG_ARRAY);
  }
}



/* Restore the GL State from a structure */
void __glcRestoreGLState(const __GLCglState* inGLState,
			 const __GLCcontext* inContext, const GLboolean inAll)
{
  if (inAll || inContext->renderState.renderStyle == GLC_TEXTURE) {
    if (!inGLState->blend)
      glDisable(GL_BLEND);
    glBlendFunc(inGLState->blendSrc, inGLState->blendDst);
    glBindTexture(GL_TEXTURE_2D, inGLState->textureID);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, inGLState->textureEnvMode);
    if ((inAll || !inContext->enableState.glObjects)
	&& GLEW_ARB_pixel_buffer_object)
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
		      inGLState->pixelBufferObjectID);
  }

  if ((inAll || (inContext->renderState.renderStyle == GLC_BITMAP)
       || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO))) {
    glPixelStorei(GL_UNPACK_LSB_FIRST, inGLState->unpackLsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, inGLState->unpackRowLength);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, inGLState->unpackSkipPixels);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, inGLState->unpackSkipRows);
    glPixelStorei(GL_UNPACK_ALIGNMENT, inGLState->unpackAlignment);

    if (inAll || (inContext->renderState.renderStyle == GLC_PIXMAP_QSO)) {
      if (!inAll) { /* if inAll, already restored in GLC_TEXTURE's block */
        if (!inGLState->blend)
          glDisable(GL_BLEND);
        glBlendFunc(inGLState->blendSrc, inGLState->blendDst);
      }
      glPixelTransferf(GL_RED_BIAS, inGLState->colorBias[0]);
      glPixelTransferf(GL_GREEN_BIAS, inGLState->colorBias[1]);
      glPixelTransferf(GL_BLUE_BIAS, inGLState->colorBias[2]);
      glPixelTransferf(GL_ALPHA_BIAS, inGLState->colorBias[3]);
      glPixelTransferf(GL_RED_SCALE, inGLState->colorScale[0]);
      glPixelTransferf(GL_GREEN_SCALE, inGLState->colorScale[1]);
      glPixelTransferf(GL_BLUE_SCALE, inGLState->colorScale[2]);
      glPixelTransferf(GL_ALPHA_SCALE, inGLState->colorScale[3]);
    }
  }

  if ((inAll || (inContext->enableState.glObjects
		 && (inContext->renderState.renderStyle != GLC_BITMAP)
		 && (inContext->renderState.renderStyle != GLC_PIXMAP_QSO)))
      && GLEW_ARB_vertex_buffer_object) {
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, inGLState->vertexBufferObjectID);
    if (inAll || (inContext->renderState.renderStyle == GLC_TRIANGLE))
      glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
		      inGLState->elementBufferObjectID);
  }

  if (inAll || (inContext->renderState.renderStyle == GLC_TRIANGLE
		&& inContext->enableState.glObjects
		&& inContext->enableState.extrude))
    if (!inGLState->normalize)
      glDisable(GL_NORMALIZE);

  if (inAll || inContext->renderState.renderStyle == GLC_LINE
      || inContext->renderState.renderStyle == GLC_TRIANGLE
      || (inContext->renderState.renderStyle == GLC_TEXTURE
	  && inContext->enableState.glObjects
	  && GLEW_ARB_vertex_buffer_object)) {
    if (!inGLState->vertexArray)
      glDisableClientState(GL_VERTEX_ARRAY);
    if (inGLState->vertexArray == GL_TRUE) {
      glVertexPointer(inGLState->vertexArraySize, inGLState->vertexArrayType,
		      inGLState->vertexArrayStride,
		      inGLState->vertexArrayPointer);
    }
    if (!inGLState->normalArray)
      glDisableClientState(GL_NORMAL_ARRAY);
    if (!inGLState->colorArray)
      glDisableClientState(GL_COLOR_ARRAY);
    if (!inGLState->indexArray)
      glDisableClientState(GL_INDEX_ARRAY);
    if (!inGLState->texCoordArray)
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if ((inAll || inContext->renderState.renderStyle == GLC_TEXTURE) && inGLState->texCoordArray == GL_TRUE)
      glTexCoordPointer(inGLState->texCoordArraySize,
			inGLState->texCoordArrayType,
			inGLState->texCoordArrayStride,
			inGLState->texCoordArrayPointer);
    if (!inGLState->edgeFlagArray)
      glDisableClientState(GL_EDGE_FLAG_ARRAY);
  }
}



#ifdef GLEW_MX
/* Function for GLEW so that it can get a context */
GLEWContext* __glcGetGlewContext(void)
{
  __GLCcontext* ctx = GLC_GET_CURRENT_CONTEXT();

  if (!ctx) {
    __glcRaiseError(GLC_STATE_ERROR);
    return NULL;
  }

  return &ctx->glewContext;
}
#endif



/* This function initializes the thread management of QuesoGLC when TLS is not
 * available. It must be called once (see the macro GLC_INIT_THREAD)
 */
#ifndef HAVE_TLS
void __glcInitThread(void) {
#ifdef __WIN32__
  __glcCommonArea.threadID = GetCurrentThreadId();
#else
  __glcCommonArea.threadID = pthread_self();
#endif /* __WIN32__ */
}
#endif /* HAVE_TLS */
