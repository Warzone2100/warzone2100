/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** @file
 *  Utilities for iterating over collections of objects
 */


#ifndef __INCLUDED_SRC_OBJITER_H__
#define __INCLUDED_SRC_OBJITER_H__

#include <iterator>
#include <vector>


/* Abstract base class for iterators over object lists. */
template<typename T>
class ObjectIterator : public std::iterator<std::input_iterator_tag, T, T, T*, T&>
{
public:
	explicit ObjectIterator(
		const bool bSelectedOnly,
		T* firstObject
	);

private:
	T* currentObject;
	bool bSelectedOnly;

	// Iterator implementation
public:
	ObjectIterator<T>& operator++();
	ObjectIterator<T> operator++(int);
	bool operator==(ObjectIterator<T> other) const;
	bool operator!=(ObjectIterator<T> other) const;
	typename ObjectIterator<T>::reference operator*() const;

	template<typename U>
	friend class PlayerObjectIterator;
};

template <typename T>
class PlayerObjectIterator : public std::iterator<std::input_iterator_tag, T, T, T*, T&>
{
public:
	explicit PlayerObjectIterator(
		const unsigned int playerCursor,
		const std::vector<unsigned int> playerIndices,
		const bool bSelectedOnly,
		T** objectList
	);

private:
	unsigned int playerCursor;
	ObjectIterator<T> objIter;

	const std::vector<unsigned int> playerIndices;
	const bool bSelectedOnly;
	T** objectList;

	bool isObjIterValid();

	// Iterator implementation
public:
	PlayerObjectIterator<T>& operator++();
	PlayerObjectIterator<T> operator++(int);
	bool operator==(PlayerObjectIterator<T> other) const;
	bool operator!=(PlayerObjectIterator<T> other) const;
	typename PlayerObjectIterator<T>::reference operator*() const;
};

#endif // __INCLUDED_SRC_OBJITER_H__
