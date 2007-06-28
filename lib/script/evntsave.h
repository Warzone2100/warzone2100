/*
 * EvntSave.h
 *
 * Save the state of the event system.
 *
 */

#ifndef _evntsave_h
#define _evntsave_h

// Save the state of the event system
extern BOOL eventSaveState(SDWORD version, UBYTE **ppBuffer, UDWORD *pFileSize);

// Load the state of the event system
extern BOOL eventLoadState(UBYTE *pBuffer, UDWORD fileSize, BOOL bHashed);

#endif


