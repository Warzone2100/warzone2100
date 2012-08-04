/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2012  Warzone 2100 Project

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

#include <stdlib.h>
#include <string.h>

#include "vector.h"

struct _vector
{
	void            **mem;
	int             size;
	int             head;
	int             iter;
};

static const int defaultSize = 4;

vector *vectorCreate()
{
	vector *v = malloc(sizeof(vector));

	if (v == NULL)
	{
		return NULL;
	}

	v->mem = calloc(defaultSize, sizeof(void *));

	if (v->mem == NULL)
	{
		free(v);
		return NULL;
	}

	v->size = defaultSize;
	v->head = 0;
	
	// Rewind the vector to the start
	vectorRewind(v);

	return v;
}

void vectorDestroy(vector *v)
{
	free(v->mem);
	free(v);
}

void *vectorAdd(vector *v, void *object)
{
	if (v->head + 1 > v->size)
	{
		void **newMem = realloc(v->mem, 2 * v->size * sizeof(void *));

		if (newMem == NULL)
		{
			return NULL;
		}

		v->mem = newMem;
		v->size *= 2;
	}

	v->mem[v->head++] = object;

	return object;
}

void *vectorAt(vector *v, int index)
{
	return (index < v->head && index >= 0) ? v->mem[index] : NULL;
}

void *vectorHead(vector *v)
{
	return vectorAt(v, v->head - 1);
}

void *vectorSetAt(vector *v, int index, void *object)
{
	if (index >= v->head)
	{
		return NULL;
	}

	// Replace the item
	v->mem[index] = object;

	return object;
}

void vectorRemoveAt(vector *v, int index)
{
	if (index >= v->head)
	{
		return;
	}

	memmove(&v->mem[index], &v->mem[index + 1],
	        (v->head - index) * sizeof(void *));

	v->head--;
	v->iter--;
}

void vectorMap(vector *v, mapCallback cb)
{
	int i;
	
	for (i = 0; i < v->head; i++)
	{
		cb(v->mem[i]);
	}
}

void vectorMapAndDestroy(vector *v, mapCallback cb)
{
	vectorMap(v, cb);
	vectorDestroy(v);
}

int vectorSize(vector *v)
{
	return v->head;
}

void vectorRewind(vector *v)
{
	v->iter = -1;
}

void *vectorNext(vector *v)
{
	if (v->iter >= v->head-1)
	{
		vectorRewind(v);
		return NULL;
	}
	else
	{
		return v->mem[++v->iter];
	}
}

bool vectorHasNext(vector *v)
{
	return (v->iter < v->head) ? true : false;
}
