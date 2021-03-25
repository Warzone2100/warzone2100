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

#include "objiter.h"
#include "droiddef.h"

template class ObjectIterator<DROID>;
template class PlayerObjectIterator<DROID>;

template<typename T>
ObjectIterator<T>::ObjectIterator(
	const bool bSelectedOnly,
	T* firstObject
)
	: currentObject(firstObject)
	, bSelectedOnly(bSelectedOnly)
{
	// Ensure the first object matches the selection filter
	if (bSelectedOnly && (currentObject && !currentObject->selected))
	{
		ObjectIterator<T>& iter = *this;
		++iter;
	}
}

template<typename T>
ObjectIterator<T>& ObjectIterator<T>::operator++()
{
	// Loop through the objects via next ptrs, picking only those matching selection filter
	while (currentObject)
	{
		currentObject = currentObject->psNext;
		if (!bSelectedOnly || (currentObject && currentObject->selected))
		{
			return *this;
		}
	}

	// If the current player has no more suitable objects, set the current object to nullptr to signify we are at the end
	currentObject = nullptr;
	return *this;
}

template<typename T>
ObjectIterator<T> ObjectIterator<T>::operator++(int)
{
	ObjectIterator<T> iter = *this;
	++(*this);
	return iter;
}

template<typename T>
bool ObjectIterator<T>::operator==(ObjectIterator<T> other) const
{
	return currentObject == other.currentObject;
}

template<typename T>
bool ObjectIterator<T>::operator!=(ObjectIterator<T> other) const
{
	return !(*this == other);
}

template<typename T>
typename ObjectIterator<T>::reference ObjectIterator<T>::operator*() const
{
	ASSERT(currentObject != nullptr, "Attempt to dereference a past-the-end iterator");
	return *currentObject;
}

// Verifies that objIter is valid to be used in PlayerObjectIterator
template<typename T>
bool PlayerObjectIterator<T>::isObjIterValid()
{
	// ObjectIterator is valid if it has an object and the object matches the selection filter.
	return objIter.currentObject && (!bSelectedOnly || objIter.currentObject->selected);
}

template<typename T>
PlayerObjectIterator<T>::PlayerObjectIterator(
	const unsigned int playerCursor,
	const std::vector<unsigned int> playerIndices,
	const bool bSelectedOnly,
	T** objectList
)
	: playerCursor(playerCursor)
	, playerIndices(playerIndices)
	, bSelectedOnly(bSelectedOnly)
	, objectList(objectList)
	, objIter(playerCursor >= playerIndices.size()
		? ObjectIterator<T>(bSelectedOnly, nullptr)
		: ObjectIterator<T>(bSelectedOnly, objectList[playerIndices[playerCursor]]))
{
	// Make sure the current player has any objects and those objects match the selection filter.
	// Iterate players until a player with available objects is found. If none are found, the iterator
	// becomes past-the-end iterator (playerCursor is past the end and objIter.currentObject is null).
	while (this->playerCursor < playerIndices.size() && !isObjIterValid())
	{
		PlayerObjectIterator<T>& iter = *this;
		++iter;
	}
}

template<typename T>
PlayerObjectIterator<T>& PlayerObjectIterator<T>::operator++()
{
	// Select the next object via the child iterator
	++objIter;

	// If the child iterator ran out of objects, find next player with more objects.
	while (!objIter.currentObject && playerCursor < playerIndices.size())
	{
		++playerCursor;
		objIter = playerCursor >= playerIndices.size()
			? ObjectIterator<T>(bSelectedOnly, nullptr)
			: ObjectIterator<T>(bSelectedOnly, objectList[playerIndices[playerCursor]]);
	}

	return *this;
}

template<typename T>
PlayerObjectIterator<T> PlayerObjectIterator<T>::operator++(int)
{
	PlayerObjectIterator<T> iter = *this;
	++(*this);
	return iter;
}

template<typename T>
bool PlayerObjectIterator<T>::operator==(PlayerObjectIterator<T> other) const
{
	return bSelectedOnly == other.bSelectedOnly
		&& objectList == other.objectList
		&& playerCursor == other.playerCursor
		&& objIter.currentObject == other.objIter.currentObject;
}

template<typename T>
bool PlayerObjectIterator<T>::operator!=(PlayerObjectIterator<T> other) const
{
	return !(*this == other);
}

template<typename T>
typename PlayerObjectIterator<T>::reference PlayerObjectIterator<T>::operator*() const
{
	ASSERT(playerCursor < playerIndices.size() && objIter.currentObject, "Attempt to dereference a past-the-end iterator");
	return *objIter.currentObject;
}

