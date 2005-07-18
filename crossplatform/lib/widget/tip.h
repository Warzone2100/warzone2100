/*
 * Tip.h
 *
 * Interface to the tool tip display module
 *
 */
#ifndef _tip_h
#define _tip_h


/* Initialise the tool tip module */
extern void tipInitialise(void);

/*
 * Setup a tool tip.
 * The tip module will then wait until the correct points to
 * display and then remove the tool tip.
 * i.e. The tip will not be displayed immediately.
 * Calling this while another tip is being displayed will restart
 * the tip system.
 * psSource is the widget that started the tip.
 * x,y,width,height - specify the position of the button to place the
 * tip by.
 */
extern void tipStart(WIDGET *psSource, STRING *pTip, int NewFontID,
					 UDWORD *pColours, SDWORD x, SDWORD y, UDWORD width, UDWORD height);

/* Stop a tool tip (e.g. if the hilite is lost on a button).
 * psSource should be the same as the widget that started the tip.
 */
extern void tipStop(WIDGET *psSource);

/* Update and possibly display the tip */
extern void tipDisplay(void);

#endif

