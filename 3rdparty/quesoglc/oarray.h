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
 * header of the object __GLCarray which is an array which size can grow as
 * some new elements are added to it.
 */

#ifndef __glc_oarray_h
#define __glc_oarray_h

#define GLC_ARRAY_DATA(array) ((array)->data)
#define GLC_ARRAY_LENGTH(array) ((array)->length)
#define GLC_ARRAY_SIZE(array) (((array)->length) * ((array)->elementSize))

typedef struct __GLCarrayRec __GLCarray;

struct __GLCarrayRec {
  char* data;
  int allocated;
  int length;
  int elementSize;
};

__GLCarray* __glcArrayCreate(int inElementSize);
void __glcArrayDestroy(__GLCarray* This);
__GLCarray* __glcArrayAppend(__GLCarray* This, const void* inValue);
__GLCarray* __glcArrayInsert(__GLCarray* This, const int inRank,
			     const void* inValue);
void __glcArrayRemove(__GLCarray* This, const int inRank);
void* __glcArrayInsertCell(__GLCarray* This, const int inRank,
			   const int inCells);
__GLCarray* __glcArrayDuplicate(__GLCarray* This);
#endif
