/*
 * Objects.h
 *
 * A header file that groups together all the object header files
 *
 */
#ifndef _objects_h
#define _objects_h

#include "objectdef.h"
#include "droid.h"
#include "structure.h"
#include "feature.h"
#include "bullet.h"
#include "objmem.h"

#include "ai.h"
#include "move.h"
#include "function.h"
#include "stats.h"

/* Initialise the object system */
extern BOOL objInitialise(void);

/* Shutdown the object system */
extern BOOL objShutdown(void);

/*goes thru' the list passed in reversing the order so the first entry becomes 
the last and the last entry becomes the first!*/
extern void reverseObjectList(BASE_OBJECT **ppsList);

#endif


