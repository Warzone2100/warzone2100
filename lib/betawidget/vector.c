#include <stdlib.h>
#include <string.h>

#include "vector.h"

struct _vector
{
	void            **mem;
	int             size;
	int             head;
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
	return (index <= v->size) ? v->mem[index] : NULL;
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
}

void vectorMap(vector *v, mapCallback cb)
{
	int i;
	
	for (i = 0; i < v->size; i++)
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
