#ifndef VECTOR_H_
#define VECTOR_H_

typedef struct _vector vector;
typedef void (*destroyCallback) (void *object);

/**
 * Creates a new vector.
 * 
 * @param destroyCb The callback function to call whenever an element is deleted
 * 	                or replaced.
 * @return A pointer to the newly created vector on success; otherwise NULL.
 */
vector *vectorCreate(destroyCallback destroyCb);

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
 * 
 */
void *vectorAt(vector *v, int index);

/**
 * 
 */
void *vectorSetAt(vector *v, int index, void *object);

/**
 * 
 */
void vectorRemoveAt(vector *v, int index);

/**
 * 
 */
int vectorSize(vector *v);

#endif /*VECTOR_H_*/
