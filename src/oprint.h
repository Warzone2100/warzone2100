/*
 * oPrint.h
 *
 * Object information printing routines
 *
 */
#ifndef _oprint_h
#define _oprint_h

// print out information about a base object
extern void printBaseObjInfo(BASE_OBJECT *psObj);

// print out information about a general component
extern void printComponentInfo(COMP_BASE_STATS *psStats);

// print out weapon information
extern void printWeaponInfo(WEAPON_STATS *psStats);

// print out information about a droid and it's components
extern void printDroidInfo(DROID *psDroid);

#endif



