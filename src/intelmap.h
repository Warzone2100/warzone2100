/*
 * IntelMap.h
 *
 * Functions for the display of the Intelligence Map
 */

#ifndef _intelmap_h
#define _intelmap_h

/* Intelligence Map screen IDs */
#define IDINTMAP_FORM			6000	//The intelligence map base form
#define IDINTMAP_MSGVIEW		6002	//The message 3D view for the intelligence screen

/*dimensions for PIE view section relative to IDINTMAP_MSGVIEW*/

#define	INTMAP_PIEWIDTH			238
#define INTMAP_PIEHEIGHT		169


#define	INTMAP_TEXTWINDOWHEIGHT	(30)

// The current message being displayed
extern MESSAGE			*psCurrentMsg;
// The display stats for the current messages' text
extern TEXT_DISPLAY		currentTextDisplay;

/* Add the Intelligence Map widgets to the widget screen */
//extern BOOL intAddIntelMap(BOOL playCurrent);
extern BOOL intAddIntelMap(void);
/*Add the 3D world view for the current message */
extern BOOL intAddMessageView(MESSAGE *psMessage);
/* Remove the Message View from the Intelligence screen */
extern void intRemoveMessageView(BOOL animated);

/* Process return codes from the Intelligence Map */
extern void intProcessIntelMap(UDWORD id);
/* Process return code from the Message View for Tutorial Mode*/
//extern void intProcessMessageView(UDWORD id);

/* rotate the view so looking directly down if forward = TRUE or
 back to previous view if forward = FALSE */
//extern void intelMapView(BOOL forward);

/* Remove the Intelligence Map widgets from the screen */
extern void intRemoveIntelMap(void);

/* Remove the Intelligence Map widgets from the screen without animation*/
extern void intRemoveIntelMapNoAnim(void);

//initialise the text display stats for the current message
//extern void initTextDisplay(MESSAGE *psMessage, UDWORD fontID, UWORD fontColour);

/* scroll the text message from left to right - aka tickertape messages */
//extern void scrollMessage(char *pText, UDWORD startX, UDWORD endX, UDWORD y, UDWORD gap);

/*sets psCurrentMsg for the Intelligence screen*/
extern void setCurrentMsg(void);

/*sets which states need to be paused when the intelligence screen is up*/
extern void setIntelligencePauseState(void);
/*resets the pause states */
extern void resetIntelligencePauseState(void);

// tell the intelligence screen to play this message immediately
extern void displayImmediateMessage(MESSAGE *psMessage);

// return whether a message is immediate
extern BOOL messageIsImmediate(void);
/*sets the flag*/
extern void setMessageImmediate(BOOL state);

#endif	//intelmap.h


