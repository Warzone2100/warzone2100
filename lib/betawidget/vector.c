#include <stdlib.h>
#include <string.h>

#include "vector.h"

struct _vector
{
	void            **mem;
	int             size;
	int             head;
	destroyCallback destroy;
};

static const int defaultSize = 4;

vector *vectorCreate(destroyCallback cb)
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
	v->destroy = cb;

	return v;
}

void vectorDestroy(vector *v)
{
	int i;

	for (i = 0; i < vectorSize(v); i++)
	{
		v->destroy(v->mem[i]);
	}

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
	return (index <= vectorSize(v)) ? v->mem[index] : NULL;
}

void *vectorSetAt(vector *v, int index, void *object)
{
	if (index >= v->head)
	{
		return NULL;
	}

	// Free the current element at index
	v->destroy(v->mem[index]);

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

	// Free the element using the provided callback
	v->destroy(v->mem[index]);

	memmove(&v->mem[index], &v->mem[index + 1],
	        (v->head - index) * sizeof(void *));

	v->head--;
}

int vectorSize(vector *v)
{
	return v->head;
}
