// 
// IntOrder.h 
// definitions for order interface functions.
// 
#ifndef __INCLUDED_INTORDER__
#define __INCLUDED_INTORDER__


#define IDORDER_FORM	8000
#define IDORDER_CLOSE	8001

extern BOOL OrderUp;

BOOL intUpdateOrder(DROID *psDroid);	// update already open order form
//changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
//BOOL intAddOrder(DROID *Droid);			// create and open order form
BOOL intAddOrder(BASE_OBJECT *psObj);			// create and open order form
void intRunOrder(void);					
void intProcessOrder(UDWORD id);
void intRemoveOrder(void);
void intRemoveOrderNoAnim(void);
BOOL intRefreshOrder(void);

//new function added to bring up the RMB order form for Factories as well as droids
extern void intAddFactoryOrder(STRUCTURE *psStructure);



#endif
