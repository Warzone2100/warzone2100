/*
 * Label.h
 *
 * Definitions for the label widget.
 */
#ifndef _label_h
#define _label_h

/* The widget heaps */
extern OBJ_HEAP	*psLabHeap;

// label states.
#define WLABEL_HILITE	0x0004		// label is hilited

typedef struct _w_label
{
	/* The common widget data */
	WIDGET_BASE;

	UDWORD		state;					// The current button state
	STRING		aText[WIDG_MAXSTR];		// Text on the label
//	PROP_FONT	*psFont;				// Font for the label
	int FontID;
	STRING		*pTip;					// The tool tip for the button
} W_LABEL;

/* Create a button widget data structure */
extern BOOL labelCreate(W_LABEL **ppsWidget, W_LABINIT *psInit);

/* Free the memory used by a button */
extern void labelFree(W_LABEL *psWidget);

/* label display function */
extern void labelDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

/* Respond to a mouse moving over a label */
extern void labelHiLite(W_LABEL *psWidget, W_CONTEXT *psContext);

/* Respond to the mouse moving off a label */
extern void labelHiLiteLost(W_LABEL *psWidget);

#endif

