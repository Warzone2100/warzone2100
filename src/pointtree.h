#ifndef _point_tree_h
#define _point_tree_h

#include "lib/framework/types.h"

struct _pointTree;
typedef struct _pointTree POINT_TREE;

#ifdef __cplusplus
extern "C"
{
#endif

POINT_TREE *pointTreeCreate(void);
void pointTreeDestroy(POINT_TREE *pointTree);

void pointTreeInsert(POINT_TREE *pointTree, void *point, int32_t x, int32_t y);
void pointTreeClear();

// Must sort after inserting, and before querying.
void pointTreeSort(POINT_TREE *pointTree);

// Returns all points less than or equal to radius from (x, y), possibly plus some extra nearby points.
void **pointTreeQuery(POINT_TREE *pointTree, int32_t x, int32_t y, int32_t radius);

#ifdef __cplusplus
}
#endif

#endif //_point_tree_h
