/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
#ifndef _point_tree_h
#define _point_tree_h

#include "lib/framework/types.h"

#include <vector>

class PointTree
{
public:
	typedef std::vector<void *> ResultVector;
	typedef std::vector<unsigned> IndexVector;
	class Filter  ///< Filters are invalidated when modifying the PointTree.
	{
	public:
		Filter() : data(1) {}  ///< Must be reset before use.
		Filter(PointTree const &pointTree) : data(pointTree.points.size() + 1) {}
		void reset(PointTree const &pointTree)
		{
			data.assign(pointTree.points.size() + 1, 0);
		}
		void erase(unsigned index)
		{
			data[index] = std::max(data[index], 1u);    ///< Erases the point from query results using the filter.
		}

	private:
		friend class PointTree;

		typedef std::vector<unsigned> Data;

		Data data;
	};

	void insert(void *pointData, int32_t x, int32_t y);                       ///< Inserts a point into the point tree.
	void clear();                                                             ///< Clears the PointTree.
	void sort();                                                              ///< Must be done between inserting and querying, to get meaningful results.
	/// Returns all points less than or equal to radius from (x, y), possibly plus some extra nearby points.
	/// (More specifically, returns all objects in a square with edge length 2*radius.)
	/// Note: Not thread safe, because it modifies lastQueryResults.
	ResultVector &query(int32_t x, int32_t y, uint32_t radius);
	/// Returns all points which have not been filtered away, less than or equal to radius from (x, y), possibly plus some extra nearby points.
	/// (More specifically, returns objects in a square with edge length 2*radius.)
	/// Note: Not thread safe, because it modifies lastQueryResults, lastFilteredQueryIndices and the internal filter representation for faster lookups.
	ResultVector &query(Filter &filter, int32_t x, int32_t y, uint32_t radius);
	/// Returns all points which have not been filtered away within given rectangle. See function above on thread safety.
	ResultVector &query(int32_t x, int32_t y, uint32_t x2, uint32_t y2);

	ResultVector lastQueryResults;
	IndexVector lastFilteredQueryIndices;

private:
	typedef std::pair<uint64_t, void *> Point;
	typedef std::vector<Point> Vector;

	template<bool IsFiltered>
	ResultVector &queryMaybeFilter(Filter &filter, int32_t minXo, int32_t maxXo, int32_t minYo, int32_t maxYo);

	Vector points;
};

#endif //_point_tree_h
