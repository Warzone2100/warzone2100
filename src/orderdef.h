//
// orderdef.h 
//
// order releated structures.

#ifndef _orderdef_h
#define _orderdef_h

// data for barbarians retreating
typedef struct _run_data
{
	POINT		sPos;		 // position to retreat to
	UBYTE		forceLevel;	 // number of units below which might run
    UBYTE       healthLevel; // %health value below which to turn and run - FOR GROUPS ONLY
	UBYTE		leadership;	 // basic chance to run
} RUN_DATA;

typedef struct _droid_order_data
{
	SDWORD			order;
	//UDWORD			x,y;
	//UDWORD			x2,y2;
	UWORD			x,y;
	UWORD			x2,y2;
	//Watermelon:multiple target info;
	BASE_OBJECT		*psObj[DROID_MAXWEAPS];
	BASE_STATS		*psStats;
} DROID_ORDER_DATA;


extern RUN_DATA	asRunData[MAX_PLAYERS];	// retreat positions for the players
extern void orderDroidBase(DROID *psDroid, DROID_ORDER_DATA *psOrder);

#endif
