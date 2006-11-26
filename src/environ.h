#ifndef _environ_h
#define _environ_h

// -------------------------------------------------------------------------------
extern BOOL	waterOnMap					( void );
extern BOOL environInit					( void );
extern void	environUpdate				( void );
extern UDWORD	environGetValue			( UDWORD x, UDWORD y );
extern UDWORD	environGetData			( UDWORD x, UDWORD y );
extern UDWORD map_MistValue				( UDWORD x, UDWORD y );
extern UDWORD map_TileMistValue(UDWORD x, UDWORD y);
extern void	environShutDown				( void );
//this function is called whenever the map changes - load new level or return from an offWorld map
extern void environReset(void);

// -------------------------------------------------------------------------------
#endif
