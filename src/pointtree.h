#ifndef _point_tree_h
#define _point_tree_h

#include "lib/framework/types.h"

#ifdef __cplusplus

#include <vector>

class PointTree
{
public:
	typedef std::vector<void *> ResultVector;

	void insert(void *pointData, int32_t x, int32_t y);  ///< Inserts a point into the point tree.
	void clear();  ///< Clears the PointTree.
	void sort();  ///< Must be done between inserting and querying, to get meaningful results.
	/// Returns all points less than or equal to radius from (x, y), possibly plus some extra nearby points.
	/// (More specifically, returns all objects in a square with edge length 2*radius.)
	ResultVector &query(int32_t x, int32_t y, uint32_t radius);

	ResultVector lastQueryResults;

private:
	typedef std::pair<uint64_t, void *> Point;
	typedef std::vector<Point> Vector;

	Vector points;
};


extern "C"
{
#endif //_cplusplus

struct PointTree;
typedef struct PointTree POINT_TREE;

POINT_TREE *pointTreeCreate(void);
void pointTreeDestroy(POINT_TREE *pointTree);

void pointTreeInsert(POINT_TREE *pointTree, void *point, int32_t x, int32_t y);
void pointTreeClear();

// Must sort after inserting, and before querying.
void pointTreeSort(POINT_TREE *pointTree);

// Returns all points less than or equal to radius from (x, y), possibly plus some extra nearby points.
void **pointTreeQuery(POINT_TREE *pointTree, int32_t x, int32_t y, uint32_t radius);

#ifdef __cplusplus
}
#endif //_cplusplus

#endif //_point_tree_h
