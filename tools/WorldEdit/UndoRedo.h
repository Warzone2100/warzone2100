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
