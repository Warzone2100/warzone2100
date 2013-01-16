/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef VECTOR_H_
#define VECTOR_H_

#include <stdbool.h>

typedef struct _vector vector;
typedef void (*mapCallback) (void *object);

/**
 * Creates a new vector.
 * 
 * @return A pointer to the newly created vector on success; otherwise NULL.
 */
vector *vectorCreate(void);

/**
 * Destroys the vector v. This is done by first calling the destroy callback
 * function on each element in the vector then free'ing the vector itself.
 * 
 * @param v The vector to destroy.
 */ 
void vectorDestroy(vector *v);

/**
 * Adds object to the vector v.
 * 
 * @param v The vector to add the object onto.
 * @param object    The object to add.
 * @return A pointer to object.
 */
void *vectorAdd(vector *v, void *object);

/**
 * Returns a pointer to the object at offset index for the vector v.
 *
 * @param v The vector to return the item from.
 * @param index The index of the desired item.
 * @return A pointer to the object/item, or NULL if no such item exists.
 */
void *vectorAt(vector *v, int index);

/**
 * A convenience method that returns the most recently added item to the vector
 * v. It is functionally equivalent to: vectorAt(v, vectorSize(v)).
 *
 * @param v The vector to return the last item of.
 * @return The last item in the vector.
 */
void *vectorHead(vector *v);

/**
 * Sets the item at offset index of the vector, v, to object.
 *
 * @param v The vector to set.
 * @param index The index of the item to be set.
 * @param object    The item to set v[index] to.
 * @return The current item at index.
 */
void *vectorSetAt(vector *v, int index, void *object);

/**
 * Removes the item at index from the vector v.
 *
 * @param v The vector to remove the item from.
 * @param index The index of the item to remove.
 */
void vectorRemoveAt(vector *v, int index);

/**
 * Iterates through each item in the vector v calling cb(vectorAt(v, index)).
 *
 * @param v The vector to map/iterate over.
 * @param cb    The function to call.
 */
void vectorMap(vector *v, mapCallback cb);

/**
 * Invokes vectorMap(v, cb) followed by vectorDestroy(v). This allows for the
 * elements in the vector to be free'ed/destroyed in order to prevent memory
 * leaks.
 *
 * @param v The vector to map over and destroy.
 * @param cb    The function to call.
 */
void vectorMapAndDestroy(vector *v, mapCallback cb);

/**
 * Returns the number of elements in the vector v.
 *
 * @param v The vector to get the number of elements of.
 * @return The number of elements in the vector.
 */
int vectorSize(vector *v);

/**
 * Re-winds the internal index pointer of the vector, v, such that it is at the
 * start of the vector.
 *
 * @param v The vector to rewind.
 */
void vectorRewind(vector *v);

/**
 * Returns a pointer to the next element in the vector, incrementing the
 * internal index by 1. If the end of the vector has been reached then NULL is
 * returned and the vector is rewound.
 *
 * @param v The vector to return the next element of.
 * @return The next element of the vector, or NULL if we have reached the end of
 *         the vector.
 */
void *vectorNext(vector *v);

/**
 * Returns true if the internal iterator index is not yet at the end of the
 * vector. Otherwise, false is returned.
 *
 * @param v The vector to check.
 * @return True if thd end of the vector has not yet been reached, false
 *         otherwise.
 */
bool vectorHasNext(vector *v);

#endif /*VECTOR_H_*/
