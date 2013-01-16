/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

	$Revision$
	$Id$
	$HeadURL$
*/

#include "undoredo.h"
#include "debugprint.hpp"

CUndoRedo::CUndoRedo(CHeightMap* HeightMap, UDWORD StackSize) :
	_HeightMap(HeightMap),
	_StackSize(StackSize),
	_StackPointer(-1),
	_Stack(new UndoRecord[StackSize]),
	_GroupRefCount(0),
	_GroupActive(false),
	_GroupCounter(0)
{
	memset(_Stack, 0, sizeof(UndoRecord) * StackSize);
}


CUndoRedo::~CUndoRedo()
{
	for(UDWORD i = 0; i < _StackSize; ++i)
	{
		CleanUndoRecord(_Stack[i]);
	}

	delete [] _Stack;
}


bool CUndoRedo::PushUndo(UndoRecord* UndoRec)
{
	// Get the next record.
	++_StackPointer;
	if(_StackPointer >= _StackSize)
	{
		_StackPointer = 0;
	}

	// If it's already in use then clean it.
	if(_Stack[_StackPointer].Flags)
	{
		CleanUndoRecord(_Stack[_StackPointer]);
	}

	// Copy the undo data into it.
	_Stack[_StackPointer] = *UndoRec;

	// And mark it as new.
	_Stack[_StackPointer].Flags = UF_DONE;

	if (_GroupActive)
	{
		if(_GroupCounter > 0)
		{
			_Stack[_StackPointer].Group = UF_INGROUP;
		}
		else
		{
			_Stack[_StackPointer].Group = UF_BEGINGROUP;
		}
	}
	else
	{
		_Stack[_StackPointer].Group = UF_NOGROUP;
	}

	if (_GroupActive)
		_GroupCounter++;

	return true;
}


void CUndoRedo::BeginGroup()
{
	if (_GroupRefCount == 0)
	{
		_GroupActive = true;
		_GroupCounter = 0;
	}

	_GroupRefCount++;
}


void CUndoRedo::EndGroup()
{
	--_GroupRefCount;

	if (_GroupRefCount == 0)
	{
		_Stack[_StackPointer].Group = UF_ENDGROUP;
		_GroupActive = false;
		_GroupCounter = 0;
	}
}


bool CUndoRedo::PopUndo(UndoRecord* UndoRec, bool* IsGroup)
{
	// If the current record is new.
	if (_Stack[_StackPointer].Flags == UF_DONE)
	{
		*IsGroup = (_Stack[_StackPointer].Group == UF_INGROUP) || (_Stack[_StackPointer].Group == UF_ENDGROUP);

		// Return the record.
		*UndoRec = _Stack[_StackPointer];

		// Mark it as undone.
		_Stack[_StackPointer].Flags = UF_UNDONE;

		// Get previous record.
		--_StackPointer;

		if (_StackPointer < 0)
		{
			_StackPointer = _StackSize - 1;
		}

		return true;
	}

	return false;
}


bool CUndoRedo::GetRedo(UndoRecord* UndoRec, bool* IsGroup)
{
	SWORD Tmp = _StackPointer;

	// Get next record.
	++_StackPointer;

	if (_StackPointer >= _StackSize)
	{
		_StackPointer = 0;
	}

	if (_Stack[_StackPointer].Flags == UF_UNDONE)
	{
		*IsGroup = (_Stack[_StackPointer].Group == UF_INGROUP) || (_Stack[_StackPointer].Group == UF_BEGINGROUP);

		// Return the record.
		*UndoRec = _Stack[_StackPointer];

		// Mark it as done.
		_Stack[_StackPointer].Flags = UF_DONE;

		return true;
	}

	_StackPointer = Tmp;
	return false;
}


void CUndoRedo::CleanUndoRecord(UndoRecord& UndoRec)
{
	if (UndoRec.Tile)
		delete UndoRec.Tile;

	memset(&UndoRec, 0, sizeof(UndoRecord));
}


bool CUndoRedo::AddUndo(CTile *Tile)
{
	UndoRecord UndoRec;

	memset(&UndoRec,0,sizeof(UndoRecord));
	UndoRec.TilePointer = Tile;
	UndoRec.Tile = new CTile;
	*UndoRec.Tile = *Tile;
	PushUndo(&UndoRec);

	return true;
}


void CUndoRedo::DumpRedo()
{
	static char *FlagNames[]={
				"UF_FREE",
				"UF_DONE",
				"UF_UNDONE",
				"UF_NOGROUP",
				"UF_BEGINGROUP",
				"UF_INGROUP",
				"UF_ENDGROUP",
				};

	SWORD StackPointer = _StackPointer;

	++StackPointer;
	if (StackPointer >= _StackSize)
		StackPointer = 0;


	while (_Stack[StackPointer].Flags == UF_UNDONE)
	{
		DebugPrint("sp %d : Flags %s Group %s\n",StackPointer,
												FlagNames[_Stack[StackPointer].Flags],
												FlagNames[_Stack[StackPointer].Group]);

		// Get the next record.
		++StackPointer;
		if (StackPointer >= _StackSize)
			StackPointer = 0;
	}
}


void CUndoRedo::DumpUndo()
{
	static char *FlagNames[]={
				"UF_FREE",
				"UF_DONE",
				"UF_UNDONE",
				"UF_NOGROUP",
				"UF_BEGINGROUP",
				"UF_INGROUP",
				"UF_ENDGROUP",
				};

	SWORD StackPointer = _StackPointer;

	while (_Stack[StackPointer].Flags == UF_DONE)
	{
		DebugPrint("sp %d : Flags %s Group %s\n",StackPointer,
												FlagNames[_Stack[StackPointer].Flags],
												FlagNames[_Stack[StackPointer].Group]);

		// Get previous record.
		--StackPointer;
		if (StackPointer < 0)
			StackPointer = _StackSize - 1;
	}
}


bool CUndoRedo::Undo()
{
	UndoRecord UndoRec;
	bool IsGroup;

	do {
		if(!PopUndo(&UndoRec,&IsGroup)) {
			return false;
		}

		if(UndoRec.TilePointer) {
			CTile Temp = *UndoRec.TilePointer;
			*UndoRec.TilePointer = *UndoRec.Tile;
			*UndoRec.Tile = Temp;
		}
	} while(IsGroup);

	return true;
}


bool CUndoRedo::Redo()
{
	UndoRecord UndoRec;
	bool IsGroup;

	do {
		if(!GetRedo(&UndoRec,&IsGroup)) {
			return false;
		}

		if(UndoRec.TilePointer) {
			CTile Temp = *UndoRec.TilePointer;
			*UndoRec.TilePointer = *UndoRec.Tile;
			*UndoRec.Tile = Temp;
		}
	} while(IsGroup);

	return true;
}
