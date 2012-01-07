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
 *  header of all private functions that are used throughout the library.
 */

#ifndef __glc_internal_h
#define __glc_internal_h

#include <stdlib.h>

#if !defined(DEBUGMODE) && !defined(NDEBUG)
#define NDEBUG
#endif
#include <assert.h>

#ifdef HAVE_LIBGLEW
#include <GL/glew.h>
#else
#include "GL/glew.h"
#endif
#define GLCAPI GLEWAPI
#include "GL/glc.h"
#include "qglc_config.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#if defined(HAVE_FT_CACHE) && defined(FT_CACHE_H)
#define GLC_FT_CACHE
#endif

#define GLCchar8  FcChar8
#define GLCchar16 FcChar16
#define GLCchar32 FcChar32
#define GLCuint   FT_UInt
#define GLClong   FT_Long
#define GLCulong  FT_ULong

#include "ofont.h"

#define GLC_OUT_OF_RANGE_LEN	11
#define GLC_EPSILON		1E-6
#define GLC_POINT_SIZE		128

#if defined(__GNUC__)
# define GLC_UNUSED_ARG(_arg) GLC_UNUSED_ ## _arg __attribute__((unused))
#elif defined(__LCLINT__)
# define GLC_UNUSED_ARG(_arg) /*@unused@*/ GLC_UNUSED_ ## _arg
#else
# define GLC_UNUSED_ARG(_arg) GLC_UNUSED_ ## _arg
#endif

/* Definition of the GLC_INIT_THREAD macro : it is some sort of an equivalent to
 * XInitThreads(). It allows to get rid of pthread_get_specific()/TlsGetValue()
 * when only one thread is used and to fallback to the usual thread management
 * if more than one thread is used.
 * If Thread Local Storage is used the macro does nothing.
 */
#ifdef __WIN32__
# define GLC_INIT_THREAD() \
  if (!InterlockedCompareExchange(&__glcCommonArea.__glcInitThreadOnce, 1, 0)) \
    __glcInitThread();
#elif !defined(HAVE_TLS)
# define GLC_INIT_THREAD() \
  pthread_once(&__glcCommonArea.__glcInitThreadOnce, __glcInitThread);
#else
#define GLC_INIT_THREAD()
#endif

/* Definition of the GLC_GET_THREAD_AREA macro */
#ifdef __WIN32__
# define GLC_GET_THREAD_AREA() \
  (((__glcCommonArea.threadID == GetCurrentThreadId()) && __glcThreadArea) ? \
    __glcThreadArea : __glcGetThreadArea())
#elif !defined(HAVE_TLS)
# define GLC_GET_THREAD_AREA() \
  ((pthread_equal(__glcCommonArea.threadID, pthread_self()) && __glcThreadArea) ? \
    __glcThreadArea : __glcGetThreadArea())
#else
# define GLC_GET_THREAD_AREA() &__glcTlsThreadArea
#endif

/* Definition of the GLC_GET_CURRENT_CONTEXT macro */
#ifdef __WIN32__
# define GLC_GET_CURRENT_CONTEXT() \
  (((__glcCommonArea.threadID == GetCurrentThreadId()) && __glcThreadArea) ? \
    __glcThreadArea->currentContext : __glcGetCurrent())
#elif !defined(HAVE_TLS)
# define GLC_GET_CURRENT_CONTEXT() \
  ((pthread_equal(__glcCommonArea.threadID, pthread_self()) && __glcThreadArea) ? \
    __glcThreadArea->currentContext : __glcGetCurrent())
#else
#define GLC_GET_CURRENT_CONTEXT() __glcTlsThreadArea.currentContext
#endif

/* ceil() and floor() macros for 26.6 fixed integers */
#define GLC_CEIL_26_6(x) (((x) < 0) ? ((x) & -64) : ((x) + 63) & -64)
#define GLC_FLOOR_26_6(x) (((x) < 0) ? (((x) - 63) & -64) : ((x) & -64))

typedef struct __GLCdataCodeFromNameRec __GLCdataCodeFromName;
typedef struct __GLCcharacterRec __GLCcharacter;

struct __GLCrendererDataRec {
  GLfloat vector[8];			/* Current coordinates */
  GLfloat tolerance;			/* Chordal tolerance */
  __GLCarray* vertexArray;		/* Array of vertices */
  __GLCarray* controlPoints;		/* Array of control points */
  __GLCarray* endContour;		/* Array of contour limits */
  __GLCarray* vertexIndices;		/* Array of vertex indices */
  __GLCarray* geomBatches;		/* Array of geometric batches */
  GLfloat* transformMatrix;		/* Transformation matrix from the
					   object space to the viewport */
  GLfloat halfWidth;
  GLfloat halfHeight;
};

struct __GLCdataCodeFromNameRec {
  GLint code;
  const char* name;
};

struct __GLCgeomBatchRec {
  GLenum mode;
  GLint length;
  GLuint start;
  GLuint end;
};

struct __GLCcharacterRec {
  GLint code;
  __GLCfont* font;
  __GLCglyph* glyph;
  GLfloat advance[2];
};

/* Those functions are used to protect against race conditions whenever we try
 * to access the common area or functions which are not multi-threaded.
 */
void __glcLock(void);
void __glcUnlock(void);

/* Callback function type that is called by __glcProcessChar().
 * It allows to unify the character processing before the rendering or the
 * measurement of a character : __glcProcessChar() is called first (see below)
 * then the callback function of type __glcProcessCharFunc is called by
 * __glcProcessChar(). Two functions are defined according to this type :
 * __glcRenderChar() for rendering and __glcGetCharMetric() for measurement.
 */
typedef void* (*__glcProcessCharFunc)(const GLint inCode,
				      const GLint inPrevCode,
				      const GLboolean inIsRTL,
				      const __GLCfont* inFont,
				      __GLCcontext* inContext,
				      const void* inProcessCharData,
				      const GLboolean inMultipleChars);

/* Process the character in order to find a font that maps the code and to
 * render the corresponding glyph. Replacement code or the '\<hexcode>'
 * character sequence is issued if necessary.
 * 'inCode' must be given in UCS-4 format
 */
extern void* __glcProcessChar(__GLCcontext *inContext, const GLint inCode,
			      __GLCcharacter* inPrevCode,
			      const GLboolean inIsRTL,
			      const __glcProcessCharFunc inProcessCharFunc,
			      const void* inProcessCharData);

/* Render scalable characters using either the GLC_LINE style or the
 * GLC_TRIANGLE style
 */
extern void __glcRenderCharScalable(const __GLCfont* inFont,
				    const __GLCcontext* inContext,
				    GLfloat* inTransformMatrix,
				    const GLfloat inScaleX,
				    const GLfloat inScaleY,
				    __GLCglyph* inGlyph);

/* QuesoGLC own allocation and memory management routines */
#ifdef DEBUGMODE
extern void* __glcMalloc(size_t size);
extern void __glcFree(void* ptr);
extern void* __glcRealloc(void* ptr, size_t size);
#else
static inline void* __glcMalloc(size_t size)
{
  return malloc(size);
}
static inline void __glcFree(void *ptr)
{
  free(ptr);
}
static inline void* __glcRealloc(void *ptr, size_t size)
{
  return realloc(ptr, size);
}
#endif

/* Arrays that contain the Unicode name of characters */
extern const __GLCdataCodeFromName __glcCodeFromNameArray[];
extern const GLint __glcNameFromCodeArray[];
extern const GLint __glcMaxCode;
extern const GLint __glcCodeFromNameSize;

/* Find a Unicode name from its code */
extern const GLCchar8* __glcNameFromCode(const GLint code);

/* Find a Unicode code from its name */
extern GLint __glcCodeFromName(const GLCchar8* name);

/* Duplicate a string and convert if from any Unicode format to UTF-8 format */
extern GLCchar8* __glcConvertToUtf8(const GLCchar* inString,
				    const GLint inStringType);

/* Duplicate a string to the context buffer and convert it from UTF-8 format to
 * any Unicode format.
 */
extern GLCchar* __glcConvertFromUtf8ToBuffer(__GLCcontext* This,
					     const GLCchar8* inString);

/* Duplicate a counted string to the context buffer and convert it from any
 * Unicode format to UTF-8 format.
 */
extern GLCchar8* __glcConvertCountedStringToUtf8(const GLint inCount,
						 const GLCchar* inString,
						 const GLint inStringType);

/* Convert a UCS-4 character code into the current string type. The result is
 * stored in a GLint. This function is needed since the GLC specs store
 * individual character codes in GLint whatever is their string type.
 */
extern GLint __glcConvertUcs4ToGLint(__GLCcontext *inContext, GLint inCode);

/* Convert a character encoded in the current string type to the UCS-4 format.
 * This function is needed since the GLC specs store individual character
 * codes in GLint whatever is their string type.
 */
extern GLint __glcConvertGLintToUcs4(const __GLCcontext *inContext,
				     GLint inCode);

/* Verify that the thread has a current context and that the master identified
 * by 'inMaster' exists. Returns the master object corresponding to the master
 * ID 'inMaster'.
 */
extern __GLCmaster* __glcVerifyMasterParameters(const GLint inMaster);

/* Verify that the thread has a current context and that the font identified
 * by 'inFont' exists.
 */
extern __GLCfont* __glcVerifyFontParameters(GLint inFont);

/* Do the actual job of glcAppendFont(). This function can be called as an
 * internal version of glcAppendFont() where the current GLC context is already
 * determined and the font ID has been resolved in its corresponding __GLCfont
 * object.
 */
extern void __glcAppendFont(__GLCcontext* inContext, __GLCfont* inFont);

/* This internal function deletes the font identified by inFont (if any) and
 * creates a new font based on the pattern 'inPattern'. The resulting font is
 * added to the list GLC_FONT_LIST.
 */
extern __GLCfont* __glcNewFontFromMaster(GLint inFontID, __GLCmaster* inMaster,
					 __GLCcontext *inContext, GLint inCode);

/* This internal function tries to open the face file which name is identified
 * by 'inFace'. If it succeeds, it closes the previous face and stores the new
 * face attributes in the __GLCfont object "inFont". Otherwise, it leaves the
 * font unchanged. GL_TRUE or GL_FALSE are returned to indicate if the function
 * succeeded or not.
 */
extern GLboolean __glcFontFace(__GLCfont* inFont, const GLCchar8* inFace,
			       __GLCcontext *inContext);

/* Allocate a new ID for a font and store it in a special list so that the same
 * ID is not allocated twice.
 */
GLint __glcGenFontID(__GLCcontext* inContext);

#ifndef HAVE_TLS
/* Return a struct which contains thread specific info. If the platform supports
 * pointers for thread-local storage (TLS) then __glcGetThreadArea is replaced
 * by a macro that returns a thread-local pointer. Otherwise, a function is
 * called to return the structure using pthread_get_specific (POSIX) or
 * TlsGetValue (WIN32) which are much slower.
 */
extern __GLCthreadArea* __glcGetThreadArea(void);
#endif

/* Raise an error.
 * See also remarks above about TLS pointers.
 */
#ifdef HAVE_TLS
#define __glcRaiseError(inError) \
if (!__glcTlsThreadArea.errorState || ! (inError)) \
  __glcTlsThreadArea.errorState = (inError)
#else
extern void __glcRaiseError(GLCenum inError);
#endif

#ifndef HAVE_TLS
/* Return the current context state.
 * See also remarks above about TLS pointers.
 */
extern __GLCcontext* __glcGetCurrent(void);
#endif

/* Compute an optimal size for the glyph to be rendered on the screen (if no
 * display list is currently building).
 */
extern void __glcGetScale(const __GLCcontext* inContext,
			  GLfloat* outTransformMatrix,
			  GLfloat* outScaleX, GLfloat* outScaleY);

/* Convert 'inString' (stored in logical order) to UCS4 format and return a
 * copy of the converted string in visual order.
 */
extern GLCchar32* __glcConvertToVisualUcs4(__GLCcontext* inContext,
					   GLboolean *outIsRTL,
					   GLint *outLength,
					   const GLCchar* inString);

/* Convert 'inCount' characters of 'inString' (stored in logical order) to UCS4
 * format and return a copy of the converted string in visual order.
 */
extern GLCchar32* __glcConvertCountedStringToVisualUcs4(__GLCcontext* inContext,
							GLboolean *outIsRTL,
							const GLCchar* inString,
							const GLint inCount);

#ifdef GLC_FT_CACHE
/* Callback function used by the FreeType cache manager to open a given face */
extern FT_Error __glcFileOpen(FTC_FaceID inFile, FT_Library inLibrary,
			      FT_Pointer inData, FT_Face* outFace);

/* Rename FTC_Manager_LookupFace for old freetype versions */
# if FREETYPE_MAJOR == 2 \
     && (FREETYPE_MINOR < 1 \
         || (FREETYPE_MINOR == 1 && FREETYPE_PATCH < 8))
#   define FTC_Manager_LookupFace FTC_Manager_Lookup_Face
# endif
#endif

/* Save the GL State in a structure */
extern void __glcSaveGLState(__GLCglState* inGLState,
			     const __GLCcontext* inContext,
			     const GLboolean inAll);

/* Restore the GL State from a structure */
extern void __glcRestoreGLState(const __GLCglState* inGLState,
				const __GLCcontext* inContext,
				const GLboolean inAll);

#ifdef GLEW_MX
/* Macro/function for GLEW so that it can get a context */
GLEWAPI GLEWContext* __glcGetGlewContext(void);
#define glewGetContext() __glcGetGlewContext()
#endif

#ifndef HAVE_TLS
/* This function initializes the thread management of QuesoGLC when TLS is not
 * available. It must be called once (see the macro GLC_INIT_THREAD)
 */
extern void __glcInitThread(void);
#endif

extern int __glcdeCasteljauConic(void *inUserData);
extern int __glcdeCasteljauCubic(void *inUserData);

#endif /* __glc_internal_h */
