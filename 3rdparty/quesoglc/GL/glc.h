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

#if !defined(__glc_h_)
#define __glc_h_

/************************************************************************
 * Begin system-specific stuff
 * from Mesa 3-D graphics library
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* __WIN32__ */
#if !defined(__WIN32__) && (defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__))
#  define __WIN32__
#endif

#ifdef __WIN32__
#  include <windows.h>
#endif

/* GLCAPI, part 1 (use WINGDIAPI, if defined) */
#if defined(__WIN32__) && defined(WINGDIAPI) && !defined(GLCAPI)
#  define GLCAPI WINGDIAPI
#endif

/* GLCAPI, part 2 */
#if !defined(GLCAPI)
#  if defined(_MSC_VER)                        /* Microsoft Visual C++ */
#    define GLCAPI __declspec(dllimport)
#  elif defined(__LCC__) && defined(__WIN32__) /* LCC-Win32 */
#    define GLCAPI __stdcall
#  elif defined(__GNUC__) && __GNUC__ >= 4
#    define GLCAPI extern __attribute__ ((visibility("default")))
#  else                                        /* Others (e.g. MinGW, Cygwin, non-win32) */
#    define GLCAPI extern
#  endif
#endif

/* APIENTRY */
#if !defined(APIENTRY)
#  if defined(__WIN32__)
#    define APIENTRY __stdcall
#  else
#    define APIENTRY
#  endif
#endif

/* CALLBACK */
#if !defined(CALLBACK)
#  if defined(__WIN32__)
#    define CALLBACK __stdcall
#  else
#    define CALLBACK
#  endif
#endif
/*
 * End system-specific stuff.
 ************************************************************************/



#if defined __APPLE__ && defined __MACH__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#if defined(__cplusplus)
    extern "C" {
#endif

typedef void GLCchar;
typedef GLint GLCenum;

#if defined(__cplusplus)
typedef GLboolean (CALLBACK *GLCfunc)(GLint);
#else
typedef GLboolean (CALLBACK *GLCfunc)(GLint);
#endif

/*************************************************************/

#define GLC_NONE                                  0x0000

#define GLC_AUTO_FONT                             0x0010
#define GLC_GL_OBJECTS                            0x0011
#define GLC_MIPMAP                                0x0012

#define GLC_OP_glcUnmappedCode                    0x0020

#define GLC_BASELINE                              0x0030
#define GLC_BOUNDS                                0x0031

#define GLC_PARAMETER_ERROR                       0x0040
#define GLC_RESOURCE_ERROR                        0x0041
#define GLC_STATE_ERROR                           0x0042

#define GLC_CHAR_LIST                             0x0050
#define GLC_FACE_LIST                             0x0051

#define GLC_FAMILY                                0x0060
#define GLC_MASTER_FORMAT                         0x0061
#define GLC_VENDOR                                0x0062
#define GLC_VERSION                               0x0063

#define GLC_CHAR_COUNT                            0x0070
#define GLC_FACE_COUNT                            0x0071
#define GLC_IS_FIXED_PITCH                        0x0072
#define GLC_MAX_MAPPED_CODE                       0x0073
#define GLC_MIN_MAPPED_CODE                       0x0074
#define GLC_IS_OUTLINE                            0x0075

#define GLC_CATALOG_LIST                          0x0080

#define GLC_CURRENT_FONT_LIST                     0x0090
#define GLC_FONT_LIST                             0x0091
#define GLC_LIST_OBJECT_LIST                      0x0092
#define GLC_TEXTURE_OBJECT_LIST                   0x0093

#define GLC_DATA_POINTER                          0x00A0

#define GLC_EXTENSIONS                            0x00B0
#define GLC_RELEASE                               0x00B1

#define GLC_RESOLUTION                            0x00C0

#define GLC_BITMAP_MATRIX                         0x00D0

#define GLC_CATALOG_COUNT                         0x00E0
#define GLC_CURRENT_FONT_COUNT                    0x00E1
#define GLC_FONT_COUNT                            0x00E2
#define GLC_LIST_OBJECT_COUNT                     0x00E3
#define GLC_MASTER_COUNT                          0x00E4
#define GLC_MEASURED_CHAR_COUNT                   0x00E5
#define GLC_RENDER_STYLE                          0x00E6
#define GLC_REPLACEMENT_CODE                      0x00E7
#define GLC_STRING_TYPE                           0x00E8
#define GLC_TEXTURE_OBJECT_COUNT                  0x00E9
#define GLC_VERSION_MAJOR                         0x00EA
#define GLC_VERSION_MINOR                         0x00EB

#define GLC_BITMAP                                0x0100
#define GLC_LINE                                  0x0101
#define GLC_TEXTURE                               0x0102
#define GLC_TRIANGLE                              0x0103

#define GLC_UCS1                                  0x0110
#define GLC_UCS2                                  0x0111
#define GLC_UCS4                                  0x0112

/*************************************************************/

GLCAPI void APIENTRY glcContext (GLint inContext);
GLCAPI void APIENTRY glcDeleteContext (GLint inContext);
GLCAPI GLint APIENTRY glcGenContext (void);
GLCAPI GLint* APIENTRY glcGetAllContexts (void);
GLCAPI GLint APIENTRY glcGetCurrentContext (void);
GLCAPI GLCenum APIENTRY glcGetError (void);
GLCAPI GLboolean APIENTRY glcIsContext (GLint inContext);

GLCAPI void APIENTRY glcCallbackFunc (GLCenum inOpcode, GLCfunc inFunc);
GLCAPI void APIENTRY glcDataPointer (GLvoid *inPointer);
GLCAPI void APIENTRY glcDeleteGLObjects (void);
GLCAPI void APIENTRY glcDisable (GLCenum inAttrib);
GLCAPI void APIENTRY glcEnable (GLCenum inAttrib);
GLCAPI GLCfunc APIENTRY glcGetCallbackFunc (GLCenum inOpcode);
GLCAPI const GLCchar* APIENTRY glcGetListc (GLCenum inAttrib, GLint inIndex);
GLCAPI GLint APIENTRY glcGetListi (GLCenum inAttrib, GLint inIndex);
GLCAPI GLvoid* APIENTRY glcGetPointer (GLCenum inAttrib);
GLCAPI const GLCchar* APIENTRY glcGetc (GLCenum inAttrib);
GLCAPI GLfloat APIENTRY glcGetf (GLCenum inAttrib);
GLCAPI GLfloat* APIENTRY glcGetfv (GLCenum inAttrib, GLfloat *outVec);
GLCAPI GLint APIENTRY glcGeti (GLCenum inAttrib);
GLCAPI GLboolean APIENTRY glcIsEnabled (GLCenum inAttrib);
GLCAPI void APIENTRY glcStringType (GLCenum inStringType);

GLCAPI void APIENTRY glcAppendCatalog (const GLCchar *inCatalog);
GLCAPI const GLCchar* APIENTRY glcGetMasterListc (GLint inMaster,
						  GLCenum inAttrib,
						  GLint inIndex);
GLCAPI const GLCchar* APIENTRY glcGetMasterMap (GLint inMaster, GLint inCode);
GLCAPI const GLCchar* APIENTRY glcGetMasterc (GLint inMaster, GLCenum inAttrib);
GLCAPI GLint APIENTRY glcGetMasteri (GLint inMaster, GLCenum inAttrib);
GLCAPI void APIENTRY glcPrependCatalog (const GLCchar *inCatalog);
GLCAPI void APIENTRY glcRemoveCatalog (GLint inIndex);

GLCAPI void APIENTRY glcAppendFont (GLint inFont);
GLCAPI void APIENTRY glcDeleteFont (GLint inFont);
GLCAPI void APIENTRY glcFont (GLint inFont);
GLCAPI GLboolean APIENTRY glcFontFace (GLint inFont, const GLCchar *inFace);
GLCAPI void APIENTRY glcFontMap (GLint inFont, GLint inCode,
				 const GLCchar *inCharName);
GLCAPI GLint APIENTRY glcGenFontID (void);
GLCAPI const GLCchar* APIENTRY glcGetFontFace (GLint inFont);
GLCAPI const GLCchar* APIENTRY glcGetFontListc (GLint inFont,
						GLCenum inAttrib,
						GLint inIndex);
GLCAPI const GLCchar* APIENTRY glcGetFontMap (GLint inFont, GLint inCode);
GLCAPI const GLbyte* APIENTRY glcGetFontMasterArray (GLint inFont,
						     GLboolean inFull,
						     GLint *outCount);
GLCAPI const GLCchar* APIENTRY glcGetFontc (GLint inFont, GLCenum inAttrib);
GLCAPI GLint APIENTRY glcGetFonti (GLint inFont, GLCenum inAttrib);
GLCAPI GLboolean APIENTRY glcIsFont (GLint inFont);
GLCAPI GLint APIENTRY glcNewFontFromFamily (GLint inFont,
					    const GLCchar *inFamily);
GLCAPI GLint APIENTRY glcNewFontFromMaster (GLint inFont, GLint inMaster);

GLCAPI void APIENTRY glcLoadIdentity (void);
GLCAPI void APIENTRY glcLoadMatrix (const GLfloat *inMatrix);
GLCAPI void APIENTRY glcMultMatrix (const GLfloat *inMatrix);
GLCAPI void APIENTRY glcRotate (GLfloat inAngle);
GLCAPI void APIENTRY glcScale (GLfloat inX, GLfloat inY);

GLCAPI void APIENTRY glcRenderChar (GLint inCode);
GLCAPI void APIENTRY glcRenderCountedString (GLint inCount,
					     const GLCchar *inString);
GLCAPI void APIENTRY glcRenderString (const GLCchar *inString);
GLCAPI void APIENTRY glcRenderStyle (GLCenum inStyle);
GLCAPI void APIENTRY glcReplacementCode (GLint inCode);
GLCAPI void APIENTRY glcResolution (GLfloat inVal);

GLCAPI GLfloat* APIENTRY glcGetCharMetric (GLint inCode, GLCenum inMetric,
					   GLfloat *outVec);
GLCAPI GLfloat* APIENTRY glcGetMaxCharMetric (GLCenum inMetric,
					      GLfloat *outVec);
GLCAPI GLfloat* APIENTRY glcGetStringCharMetric (GLint inIndex,
						 GLCenum inMetric,
						 GLfloat *outVec);
GLCAPI GLfloat* APIENTRY glcGetStringMetric (GLCenum inMetric, GLfloat *outVec);
GLCAPI GLint APIENTRY glcMeasureCountedString (GLboolean inMeasureChars,
					       GLint inCount,
					       const GLCchar *inString);
GLCAPI GLint APIENTRY glcMeasureString (GLboolean inMeasureChars,
					const GLCchar *inString);

/*************************************************************/

#define GLC_SGI_ufm_typeface_handle               1
#define GLC_UFM_TYPEFACE_HANDLE_SGI               0x8001
#define GLC_UFM_TYPEFACE_HANDLE_COUNT_SGI         0x8003
GLCAPI GLint APIENTRY glcGetMasterListiSGI(GLint inMaster, GLCenum inAttrib,
					   GLint inIndex);
GLCAPI GLint APIENTRY glcGetFontListiSGI(GLint inFont, GLCenum inAttrib,
					 GLint inIndex);

#define GLC_SGI_full_name                         1
#define GLC_FULL_NAME_SGI                         0x8002

#define GLC_QSO_utf8                              1
#define GLC_UTF8_QSO                              0x8004

#define GLC_QSO_hinting                           1
#define GLC_HINTING_QSO                           0x8005

#define GLC_QSO_extrude                           1
#define GLC_EXTRUDE_QSO                           0x8006

#define GLC_QSO_kerning                           1
#define GLC_KERNING_QSO                           0x8007

#define GLC_QSO_matrix_stack                      1
#define GLC_MATRIX_STACK_DEPTH_QSO                0x8008
#define GLC_MAX_MATRIX_STACK_DEPTH_QSO            0x8009
#define GLC_STACK_OVERFLOW_QSO                    0x800A
#define GLC_STACK_UNDERFLOW_QSO                   0x800B
GLCAPI void APIENTRY glcPushMatrixQSO(void);
GLCAPI void APIENTRY glcPopMatrixQSO(void);

#define GLC_QSO_attrib_stack                      1
#define GLC_ENABLE_BIT_QSO                        0x00000001
#define GLC_RENDER_BIT_QSO                        0x00000002
#define GLC_STRING_BIT_QSO                        0x00000004
#define GLC_GL_ATTRIB_BIT_QSO                     0x00000008
#define GLC_ALL_ATTRIB_BITS_QSO                   0x0000FFFF
#define GLC_ATTRIB_STACK_DEPTH_QSO                0x800C
#define GLC_MAX_ATTRIB_STACK_DEPTH_QSO            0x800D
GLCAPI void APIENTRY glcPushAttribQSO(GLbitfield inMask);
GLCAPI void APIENTRY glcPopAttribQSO(void);

#define GLC_QSO_buffer_object                     1
#define GLC_BUFFER_OBJECT_COUNT_QSO               0x800E
#define GLC_BUFFER_OBJECT_LIST_QSO                0x800F

#define GLC_QSO_render_parameter                  1
#define GLC_PARAMETRIC_TOLERANCE_QSO              0x8010
GLCAPI void APIENTRY glcRenderParameteriQSO(GLenum inAttrib, GLint inVal);
GLCAPI void APIENTRY glcRenderParameterfQSO(GLenum inAttrib, GLfloat inVal);

#define GLC_QSO_render_pixmap
#define GLC_PIXMAP_QSO                            0x8011

#if defined (__cplusplus)
}
#endif

#endif /* defined (__glc_h_) */
