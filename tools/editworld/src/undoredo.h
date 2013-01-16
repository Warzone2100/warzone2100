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

#ifndef __INCLUDED_UNDOREDO_H__
#define __INCLUDED_UNDOREDO_H__

#define UNDO_STACKSIZE 8192
#include "heightmap.h"

typedef enum {
	UF_FREE,
	UF_DONE,
	UF_UNDONE,
	UF_NOGROUP,
	UF_BEGINGROUP,
	UF_INGROUP,
	UF_ENDGROUP,
} UNDO_FLAGS;

struct UndoRecord {
	UNDO_FLAGS Flags;
	UNDO_FLAGS Group;
	CTile* TilePointer;	// Pointer to undo location.
	CTile* Tile;		// Undo data.
};

class CUndoRedo
{
	public:
		CUndoRedo(CHeightMap* HeightMap, UDWORD StackSize);
		~CUndoRedo();

		void BeginGroup();
		void EndGroup();

		bool AddUndo(CTile* Tile);
		bool AddUndo(DWORD ToolType, int Selected);

		bool Undo();
		bool Redo();

		void DumpUndo();
		void DumpRedo();

	private:
		bool PushUndo(UndoRecord* UndoRec);
		bool PopUndo(UndoRecord* UndoRec, bool* IsGroup);
		bool GetRedo(UndoRecord* UndoRec, bool* IsGroup);

		void CleanUndoRecord(UndoRecord& UndoRec);

	private:
		CHeightMap* _HeightMap;
		UWORD       _StackSize;
		SWORD       _StackPointer;
		UndoRecord* _Stack;
		SWORD       _GroupRefCount;
		bool        _GroupActive;
		SWORD       _GroupCounter;
};

#endif // __INCLUDED_UNDOREDO_H__
