/*
 * Message.h
 *
 * Functions for the messages shown in the Intelligence Map View
 */
#ifndef _message_h
#define _message_h

#include "structure.h"
#include "messagedef.h"
#include "deliverance.h"

#define NO_AUDIO_MSG		-1

/* The lists of messages allocated */
extern MESSAGE		*apsMessages[MAX_PLAYERS];
/* The current tutorial message - there is only ever one at a time. They are displayed 
when called by the script. They are not to be re-displayed*/
//extern MESSAGE		tutorialMessage;
/* The IMD to use for the proximity messages */
extern struct iIMDShape	*pProximityMsgIMD;

/* The list of proximity displays allocated */
extern PROXIMITY_DISPLAY *apsProxDisp[MAX_PLAYERS];

//allocates the viewdata heap
extern BOOL initViewData(void);

/* Initialise the message heaps */
extern BOOL initMessage(void);
/* Release the message heaps */
extern BOOL messageShutdown(void);

//destroys the viewdata heap
extern void viewDataHeapShutDown(void);

/*Add a messgae to the list */
extern MESSAGE * addMessage(UDWORD msgType, BOOL proxPos, UDWORD player);
 /*remove a message */
extern void removeMessage(MESSAGE *psDel, UDWORD player);
/* Remove all Messages*/
extern void freeMessages(void);
/* removes all the proximity displays */
extern void releaseAllProxDisp(void);

/*load the view data for the messages from the file exported from the world editor*/
extern VIEWDATA* loadViewData(char *pViewMsgData, UDWORD bufferSize);
/*get the view data that contains the text message pointer passed in */
extern VIEWDATA* getViewData(char *pTextMsg);
/* Release the viewdata memory */
extern void viewDataShutDown(VIEWDATA *psViewData);

//extern void storeProximityScreenCoords(MESSAGE *psMessage, SDWORD x, SDWORD y);
extern PROXIMITY_DISPLAY * getProximityDisplay(MESSAGE *psMessage);

/* Looks through the players list of messages to find one with the same viewData 
pointer and which is the same type of message - used in scriptFuncs */
extern MESSAGE* findMessage(MSG_VIEWDATA *pViewdata, MESSAGE_TYPE type, UDWORD player);

/*'displays' a proximity display*/
extern void displayProximityMessage(PROXIMITY_DISPLAY *psProxDisp);

//add proximity messages for all untapped VISIBLE oil resources
extern void addOilResourceProximities(void);

#endif // message_h
