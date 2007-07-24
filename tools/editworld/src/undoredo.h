/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#ifndef _INCLUDE_UNDOREDO_
#define _INCLUDE_UNDOREDO_

#define UNDO_STACKSIZE 8192

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
	CTile *TilePointer;	// Pointer to undo location.
	CTile *Tile;		// Undo data.
};

class CUndoRedo {
protected:
	CHeightMap *m_HeightMap;
	UWORD m_StackSize;
	SWORD m_StackPointer;
	UndoRecord *m_Stack;
	SWORD m_GroupRefCount;
	BOOL m_GroupActive;
	SWORD m_GroupCounter;

public:
	CUndoRedo(CHeightMap *HeightMap,UDWORD StackSize);
	~CUndoRedo(void);
	void BeginGroup(void);
	BOOL AddUndo(CTile *Tile);
	BOOL AddUndo(DWORD ToolType,int Selected);
	void EndGroup(void);
	BOOL Undo(void);
	BOOL Redo(void);
	void DumpUndo(void);
	void DumpRedo(void);

protected:
	BOOL PushUndo(UndoRecord *UndoRec);
	BOOL PopUndo(UndoRecord *UndoRec,BOOL *IsGroup);
	BOOL GetRedo(UndoRecord *UndoRec,BOOL *IsGroup);
	void CleanUndoRecord(UndoRecord *UndoRec);
};

#endif
