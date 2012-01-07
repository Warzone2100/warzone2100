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
/* $Id$ */

/** \file
 * header of the functions needed for cleanup stack exception handling (CSEH)
 */

#ifndef __glc_except_h
#define __glc_except_h

#include <setjmp.h>

#ifdef __cpluplus
extern "C" {
#endif

typedef unsigned int __glcException;

#define GLC_NO_EXC (__glcException)0
#define GLC_MEMORY_EXC (__glcException)1

jmp_buf* __glcExceptionCreateContext(void);
void __glcExceptionReleaseContext(void);
void __glcExceptionPush(void (*destructor)(void*), void *data);
void __glcExceptionPop(int destroy);
void __glcExceptionUnwind(int destroy);
jmp_buf* __glcExceptionThrow(__glcException exception);
__glcException __glcExceptionCatch(void);

#define TRY \
do { \
  jmp_buf* __glcEnv = __glcExceptionCreateContext(); \
  if (__glcEnv && (setjmp(*__glcEnv) == 0)) {

#define CATCH(__e__) \
    __glcExceptionUnwind(0); \
  } \
  else { \
    __e__ = __glcExceptionCatch();

#define END_CATCH \
  } \
  __glcExceptionReleaseContext(); \
} while (0);

#define THROW(__e__) \
do { \
  jmp_buf* __glcEnv; \
  __glcExceptionUnwind(1); \
  __glcEnv = __glcExceptionThrow(__e__); \
  longjmp(*__glcEnv, 1); \
} while (0);

#define RETHROW(__e__) \
do { \
  jmp_buf* __glcEnv; \
  __glcExceptionReleaseContext(); \
  __glcExceptionUnwind(1); \
  __glcEnv = __glcExceptionThrow(__e__); \
  longjmp(*__glcEnv, 1); \
} while (0);

#define RETURN(__value__) \
do { \
  __glcExceptionUnwind(0); \
  __glcExceptionReleaseContext(); \
  return __value__; \
} while(0);

#ifdef __cplusplus
}
#endif

#endif
