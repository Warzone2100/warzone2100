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
 * defines the object __GLCarray which is an array which size can grow as some
 * new elements are added to it.
 */

/* This object heavily uses the realloc() which means that it must not be
 * assumed that the data are always stored at the same address. The safer way
 * to handle that is to *always* assume the address of the data has changed
 * *every* time a method of __GLCarray is called ; whatever the method is.
 */

#include "internal.h"

#define GLC_ARRAY_BLOCK_SIZE 16



/* Constructor of the object : it allocates memory and initializes the member
 * of the new object.
 * The user must give the size of an element of the array.
 */
__GLCarray* __glcArrayCreate(const int inElementSize)
{
  __GLCarray* This = NULL;

  This = (__GLCarray*)__glcMalloc(sizeof(__GLCarray));
  if (!This) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  memset(This, 0, sizeof(__GLCarray));

  This->data = (char*)__glcMalloc(GLC_ARRAY_BLOCK_SIZE * inElementSize);
  if (!This->data) {
    __glcFree(This);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  This->allocated = GLC_ARRAY_BLOCK_SIZE;
  This->elementSize = inElementSize;

  return This;
}



/* Destructor of the object */
void __glcArrayDestroy(__GLCarray* This)
{
  if (This->data) {
    assert(This->allocated);
    __glcFree(This->data);
  }

  __glcFree(This);
}



/* Allocate a new block of elements in the array 'This'. The function returns
 * NULL if it fails and raises an error accordingly. However the original
 * array is not lost and is kept untouched.
 */
static __GLCarray* __glcArrayUpdateSize(__GLCarray* This)
{
  char* data = NULL;

  data = (char*)__glcRealloc(This->data,
	(This->allocated + GLC_ARRAY_BLOCK_SIZE) * This->elementSize);
  if (!data) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }
  This->data = data;
  This->allocated += GLC_ARRAY_BLOCK_SIZE;

  return This;
}



/* Append a value to the array. The function may allocate some more room if
 * necessary
 */
__GLCarray* __glcArrayAppend(__GLCarray* This, const void* inValue)
{
  /* Update the room if needed */
  if (This->length == This->allocated) {
    if (!__glcArrayUpdateSize(This))
      return NULL;
  }

  /* Append the new element */
  memcpy(This->data + This->length*This->elementSize, inValue,
	 This->elementSize);
  This->length++;

  return This;
}



/* Insert a value in the array at the rank inRank. The function may allocate
 * some more room if necessary
 */
__GLCarray* __glcArrayInsert(__GLCarray* This, const int inRank,
			     const void* inValue)
{
  /* Update the room if needed */
  if (This->length == This->allocated) {
    if (!__glcArrayUpdateSize(This))
      return NULL;
  }

  /* Insert the new element */
  if (This->length > inRank)
    memmove(This->data + (inRank+1) * This->elementSize,
	   This->data + inRank * This->elementSize,
	   (This->length - inRank) * This->elementSize);

  memcpy(This->data + inRank*This->elementSize, inValue, This->elementSize);
  This->length++;

  return This;
}



/* Remove an element from the array. For performance reasons, this function
 * does not release memory.
 */
void __glcArrayRemove(__GLCarray* This, const int inRank)
{
  if (inRank < This->length-1)
    memmove(This->data + inRank * This->elementSize,
	    This->data + (inRank+1) * This->elementSize,
	    (This->length - inRank - 1) * This->elementSize);
  This->length--;
}



/* Insert some room in the array at rank 'inRank' and leave it as is.
 * The difference between __glcArrayInsertCell() and __glcArrayInsert() is that
 * __glcArrayInsert() copy a value in the new element array while
 * __glcArrayInsertCell() does not. Moreover __glcArrayInsertCell() can insert
 * several cells in a row which is faster than calling __glcArrayInsert()
 * several times in a row.
 * This function is used to optimize performance in certain configurations.
 */
void* __glcArrayInsertCell(__GLCarray* This, const int inRank,
			   const int inCells)
{
  char* newCell = NULL;

  assert(inCells < GLC_ARRAY_BLOCK_SIZE);

  if ((This->length + inCells) > This->allocated) {
    if (!__glcArrayUpdateSize(This))
      return NULL;
  }

  newCell = This->data + inRank * This->elementSize;

  if (This->length > inRank)
    memmove(newCell + inCells * This->elementSize, newCell,
	   (This->length - inRank) * This->elementSize);

  This->length += inCells;

  return (void*)newCell;
}



/* Duplicate an array */
__GLCarray* __glcArrayDuplicate(__GLCarray* This)
{
  __GLCarray* duplicate = NULL;

  duplicate = (__GLCarray*)__glcMalloc(sizeof(__GLCarray));
  if (!duplicate) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  memcpy(duplicate, This, sizeof(__GLCarray));

  duplicate->data = (char*)__glcMalloc(This->allocated * This->elementSize);
  if (!duplicate->data) {
    __glcFree(duplicate);
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  memcpy(duplicate->data, This->data, This->allocated * This->elementSize);

  return duplicate;
}
