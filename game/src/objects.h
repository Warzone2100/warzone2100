/*
 * Objects.h
 *
 * A header file that groups together all the object header files
 *
 */
#ifndef _objects_h
#define _objects_h

#include "ObjectDef.h"
#include "Droid.h"
#include "Structure.h"
#include "Feature.h"
#include "Bullet.h"
#include "ObjMem.h"

#include "AI.h"
#include "Move.h"
#include "Function.h"
#include "Stats.h"

/* Initialise the object system */
extern BOOL objInitialise(void);

/* Shutdown the object system */
extern BOOL objShutdown(void);

/*goes thru' the list passed in reversing the order so the first entry becomes 
the last and the last entry becomes the first!*/
extern void reverseObjectList(BASE_OBJECT **ppsList);

#endif


