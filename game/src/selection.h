#ifndef _selection_h
#define _selection_h

typedef enum _selection_class
{
DS_ALL_UNITS,
DS_BY_TYPE
} SELECTION_CLASS;

typedef enum _selectiontype
{
DST_UNUSED,
DST_VTOL,
DST_HOVER,
DST_WHEELED,
DST_TRACKED,
DST_HALF_TRACKED,
DST_ALL_COMBAT,
DST_ALL_DAMAGED,
DST_ALL_SAME
} SELECTIONTYPE;


// ---------------------------------------------------------------------
// EXTERNALLY REFERENCED FUNCTIONS
extern UDWORD	selDroidSelection( UDWORD	player, SELECTION_CLASS droidClass, 
						  SELECTIONTYPE droidType, BOOL bOnScreen );
extern UDWORD	selDroidDeselect		( UDWORD player );
extern UDWORD	selNumSelected			( UDWORD player );
extern void	selNextRepairUnit			( void );
extern void selNextUnassignedUnit		( void );
extern void	selNextSpecifiedBuilding	( UDWORD structType );
extern	void	selNextSpecifiedUnit	(UDWORD unitType);
// select the n'th command droid
extern void selCommander(SDWORD n);

// ---------------------------------------------------------------------

#endif
