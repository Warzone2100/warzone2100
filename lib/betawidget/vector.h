#ifndef VECTOR_H_
#define VECTOR_H_

typedef struct _vector vector;
typedef void (*destroyCallback) (void *object);

struct _vector
{
	void			**mem;
	int				size;
	int				head;
	destroyCallback	destroy;
};

vector *vectorCreate(destroyCallback destroyCb);

void vectorDestroy(vector *v);

void *vectorAdd(vector *v, void *object);

void *vectorAt(vector *v, int index);

void *vectorSetAt(vector *v, int index, void *object);

void vectorRemoveAt(vector *v, int index);

int vectorSize(vector *v);

#endif /*VECTOR_H_*/
