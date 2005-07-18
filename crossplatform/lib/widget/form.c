/*
 * Form.c
 *
 * Functionality for the form widget.
 */

#include "frame.h"
#include "widget.h"
#include "widgint.h"
#include "form.h"
#include "tip.h"
#include "vid.h"
#include "piepalette.h"

/* The widget heaps */
OBJ_HEAP	*psFormHeap;
OBJ_HEAP	*psCFormHeap;
OBJ_HEAP	*psTFormHeap;

/* Control whether single tabs are displayed */
#define NO_DISPLAY_SINGLE_TABS 1

static void formFreeTips(W_TABFORM *psForm);

/* Store the position of a tab */
typedef struct _tab_pos
{
	SDWORD		index;
	SDWORD		x,y;
	UDWORD		width,height;
} TAB_POS;

/* Set default colours for a form */
static void formSetDefaultColours(W_FORM *psForm)
{
	static BOOL bDefaultsSet = FALSE;
	static UBYTE wcol_bkgrnd;    
	static SDWORD wcol_text;      
	static UBYTE wcol_light;     
	static UBYTE wcol_dark;      
	static UBYTE wcol_hilite;    
	static UBYTE wcol_cursor;    
	static UBYTE wcol_tipbkgrnd; 
	static UBYTE wcol_disable;   

	if (bDefaultsSet)
	{
		psForm->aColours[WCOL_BKGRND]    = wcol_bkgrnd;
		psForm->aColours[WCOL_TEXT]      = wcol_text;
		psForm->aColours[WCOL_LIGHT]     = wcol_light;
		psForm->aColours[WCOL_DARK]      = wcol_dark;    
		psForm->aColours[WCOL_HILITE]    = wcol_hilite;
		psForm->aColours[WCOL_CURSOR]    = wcol_cursor;
		psForm->aColours[WCOL_TIPBKGRND] = wcol_tipbkgrnd;
		psForm->aColours[WCOL_DISABLE]   = wcol_disable;
	}
	else
	{
		wcol_bkgrnd    = (UBYTE)pal_GetNearestColour(0x7f,0x7f,0x7f);
		wcol_text      = -1;
		wcol_light     = (UBYTE)pal_GetNearestColour(0xff,0xff,0xff);
		wcol_dark      = (UBYTE)pal_GetNearestColour(0,0,0);
		wcol_hilite    = (UBYTE)pal_GetNearestColour(0x40,0x40,0x40);
		wcol_cursor    = (UBYTE)pal_GetNearestColour(0xff,0x00,0x00);
		wcol_tipbkgrnd = (UBYTE)pal_GetNearestColour(0x30,0x30,0x60);
		wcol_disable   = (UBYTE)pal_GetNearestColour(0xbf,0xbf,0xbf);

		bDefaultsSet   = TRUE;
		
		psForm->aColours[WCOL_BKGRND]    = wcol_bkgrnd;
		psForm->aColours[WCOL_TEXT]      = wcol_text;
		psForm->aColours[WCOL_LIGHT]     = wcol_light;
		psForm->aColours[WCOL_DARK]      = wcol_dark;    
		psForm->aColours[WCOL_HILITE]    = wcol_hilite;
		psForm->aColours[WCOL_CURSOR]    = wcol_cursor;
		psForm->aColours[WCOL_TIPBKGRND] = wcol_tipbkgrnd;
		psForm->aColours[WCOL_DISABLE]   = wcol_disable;

	}
//old colours ??
//	psForm->aColours[WCOL_BKGRND] = screenGetCacheColour(0xbf,0xbf,0xbf);
//	psForm->aColours[WCOL_TEXT] = -1;	// use bmp colours  was screenGetCacheColour(0,0,0);
//	psForm->aColours[WCOL_LIGHT] = screenGetCacheColour(0xff,0xff,0xff);
//	psForm->aColours[WCOL_DARK] = screenGetCacheColour(0,0,0);
//	psForm->aColours[WCOL_HILITE] = screenGetCacheColour(0x44,0x44,0x44);
//	psForm->aColours[WCOL_CURSOR] = screenGetCacheColour(0xff,0,0);
//	psForm->aColours[WCOL_TIPBKGRND] = screenGetCacheColour(50,50,100); //was screenGetCacheColour(0xcc,0xcc,0xcc);
//	psForm->aColours[WCOL_DISABLE] = screenGetCacheColour(0x7f,0x7f,0x7f);
}

/* Create a plain form widget */
static BOOL formCreatePlain(W_FORM **ppsWidget, W_FORMINIT *psInit)
{
	/* Allocate the required memory */
#if W_USE_MALLOC
	*ppsWidget = (W_FORM *)MALLOC(sizeof(W_FORM));
	if (*ppsWidget == NULL)
#else
	if (!HEAP_ALLOC(psFormHeap, (void*) ppsWidget))
#endif
	{
		ASSERT((FALSE, "formCreatePlain: Out of memory"));
		return FALSE;
	}

	/* Initialise the structure */
	memset(*ppsWidget, 0, sizeof(W_FORM));
	(*ppsWidget)->type = WIDG_FORM;
	(*ppsWidget)->id = psInit->id;
	(*ppsWidget)->formID = psInit->formID;
	(*ppsWidget)->style = psInit->style;
	(*ppsWidget)->disableChildren = psInit->disableChildren;
	(*ppsWidget)->animCount = 0;
	(*ppsWidget)->x = psInit->x;
	(*ppsWidget)->y = psInit->y;
	(*ppsWidget)->width = psInit->width;
	(*ppsWidget)->height = psInit->height;

	if (psInit->pDisplay)
	{
		(*ppsWidget)->display = psInit->pDisplay;
	}
	else
	{
		(*ppsWidget)->display = formDisplay;
	}
	(*ppsWidget)->callback = psInit->pCallback;
	(*ppsWidget)->pUserData = psInit->pUserData;
	(*ppsWidget)->UserData = psInit->UserData;
	(*ppsWidget)->psWidgets = NULL;
	(*ppsWidget)->psLastHiLite = NULL;
	formSetDefaultColours(*ppsWidget);

	formInitialise(*ppsWidget);
	
	return TRUE;
}


/* Free a plain form widget */
static void formFreePlain(W_FORM *psWidget)
{
	ASSERT((PTRVALID(psWidget, sizeof(W_FORM)),
		"formFreePlain: Invalid form pointer"));

	widgReleaseWidgetList(psWidget->psWidgets);
#if W_USE_MALLOC
	FREE(psWidget);
#else
	HEAP_FREE(psFormHeap, psWidget);
#endif
}


/* Create a plain form widget */
static BOOL formCreateClickable(W_CLICKFORM **ppsWidget, W_FORMINIT *psInit)
{
	/* Allocate the required memory */
#if W_USE_MALLOC
	*ppsWidget = (W_CLICKFORM *)MALLOC(sizeof(W_CLICKFORM));
	if (*ppsWidget == NULL)
#else
	if (!HEAP_ALLOC(psCFormHeap, (void*) ppsWidget))
#endif
	{
		ASSERT((FALSE, "formCreateClickable: Out of memory"));
		return FALSE;
	}

	/* Initialise the structure */
	memset(*ppsWidget, 0, sizeof(W_CLICKFORM));
	(*ppsWidget)->type = WIDG_FORM;
	(*ppsWidget)->id = psInit->id;
	(*ppsWidget)->formID = psInit->formID;
	(*ppsWidget)->style = psInit->style;
	(*ppsWidget)->disableChildren = psInit->disableChildren;
	(*ppsWidget)->animCount = 0;
	(*ppsWidget)->x = psInit->x;
	(*ppsWidget)->y = psInit->y;
	(*ppsWidget)->width = psInit->width;
	(*ppsWidget)->height = psInit->height;
	(*ppsWidget)->callback = psInit->pCallback;
	(*ppsWidget)->pUserData = psInit->pUserData;
	(*ppsWidget)->UserData = psInit->UserData;

	(*ppsWidget)->AudioCallback = WidgGetAudioCallback();
	(*ppsWidget)->HilightAudioID = WidgGetHilightAudioID();
	(*ppsWidget)->ClickedAudioID = WidgGetClickedAudioID();

	if (psInit->pDisplay)
	{
		(*ppsWidget)->display = psInit->pDisplay;
	}
	else
	{
		(*ppsWidget)->display = formDisplayClickable;
	}
	(*ppsWidget)->psWidgets = NULL;
	(*ppsWidget)->psLastHiLite = NULL;
	if (psInit->pTip)
	{
#if W_USE_STRHEAP
		if (!widgAllocCopyString(&(*ppsWidget)->pTip, psInit->pTip))
		{
			ASSERT((FALSE, "formCreateClickable: Out of string memory"));
#if W_USE_MALLOC
			FREE(*ppsWidget);
#else
			HEAP_FREE(psCFormHeap, *ppsWidget);
#endif
			return FALSE;
		}
#endif
		(*ppsWidget)->pTip = psInit->pTip;
	}
	else
	{
		(*ppsWidget)->pTip = NULL;
	}
	formSetDefaultColours((W_FORM *)*ppsWidget);

	formInitialise((W_FORM *)*ppsWidget);
	
	return TRUE;
}


/* Free a plain form widget */
static void formFreeClickable(W_CLICKFORM *psWidget)
{
	ASSERT((PTRVALID(psWidget, sizeof(W_FORM)),
		"formFreePlain: Invalid form pointer"));

	widgReleaseWidgetList(psWidget->psWidgets);
#if W_USE_STRHEAP
	if (psWidget->pTip)
	{
		widgFreeString(psWidget->pTip);
	}
#endif

#if W_USE_MALLOC
	FREE(psWidget);
#else
	HEAP_FREE(psCFormHeap, psWidget);
#endif
}


/* Create a tabbed form widget */
static BOOL formCreateTabbed(W_TABFORM **ppsWidget, W_FORMINIT *psInit)
{
	UDWORD		major,minor;
	W_MAJORTAB	*psMajor;

	if (psInit->numMajor == 0)
	{
		ASSERT((FALSE, "formCreateTabbed: Must have at least one major tab on a tabbed form"));
		return FALSE;
	}
	if (psInit->majorPos != 0 && psInit->majorPos == psInit->minorPos)
	{
		ASSERT((FALSE, "formCreateTabbed: Cannot have major and minor tabs on same side"));
		return FALSE;
	}
	if (psInit->numMajor >= WFORM_MAXMAJOR)
	{
		ASSERT((FALSE, "formCreateTabbed: Too many Major tabs"));
		return FALSE;
	}
	for(major=0; major<psInit->numMajor; major++)
	{
		if (psInit->aNumMinors[major] >= WFORM_MAXMINOR)
		{
			ASSERT((FALSE, "formCreateTabbed: Too many Minor tabs for Major %d", major));
			return FALSE;
		}
		if (psInit->aNumMinors[major] == 0)
		{
			ASSERT((FALSE, "formCreateTabbed: Must have at least one Minor tab for each major"));
			return FALSE;
		}
	}

	/* Allocate the required memory */
#if W_USE_MALLOC
	*ppsWidget = (W_TABFORM *)MALLOC(sizeof(W_TABFORM));
	if (*ppsWidget == NULL)
#else
	if (!HEAP_ALLOC(psTFormHeap, (void*) ppsWidget))
#endif
	{
		ASSERT((FALSE, "formCreateTabbed: Out of memory"));
		return FALSE;
	}
	memset(*ppsWidget, 0, sizeof(W_TABFORM));

	/* Allocate the memory for tool tips and copy them in */
	psMajor = (*ppsWidget)->asMajor;
	for(major=0; major<psInit->numMajor; major++)
	{
		/* Check for a tip for the major tab */
		if (psInit->apMajorTips[major])
		{
#if W_USE_STRHEAP
			(void)widgAllocCopyString(&psMajor->pTip, psInit->apMajorTips[major]);
#else
			psMajor->pTip = psInit->apMajorTips[major];
#endif
		}
		/* Check for tips for the minor tab */
		for(minor=0; minor<psInit->aNumMinors[major]; minor++)
		{
			if (psInit->apMinorTips[major][minor])
			{
#if W_USE_STRHEAP
				(void)widgAllocCopyString(&(psMajor->asMinor[minor].pTip),
										  psInit->apMinorTips[major][minor]);
#else
				psMajor->asMinor[minor].pTip = psInit->apMinorTips[major][minor];
#endif
			}
		}
		psMajor++;
	}

	/* Initialise the structure */
	(*ppsWidget)->type = WIDG_FORM;
	(*ppsWidget)->id = psInit->id;
	(*ppsWidget)->formID = psInit->formID;
	(*ppsWidget)->style = psInit->style;
	(*ppsWidget)->disableChildren = psInit->disableChildren;
	(*ppsWidget)->animCount = 0;
	(*ppsWidget)->x = psInit->x;
	(*ppsWidget)->y = psInit->y;
	(*ppsWidget)->width = psInit->width;
	(*ppsWidget)->height = psInit->height;
	if (psInit->pDisplay)
	{
		(*ppsWidget)->display = psInit->pDisplay;
	}
	else
	{
		(*ppsWidget)->display = formDisplayTabbed;
	}
	(*ppsWidget)->callback = psInit->pCallback;
	(*ppsWidget)->pUserData = psInit->pUserData;
	(*ppsWidget)->UserData = psInit->UserData;
	(*ppsWidget)->psLastHiLite = NULL;
	(*ppsWidget)->majorSize = psInit->majorSize;
	(*ppsWidget)->minorSize = psInit->minorSize;
	(*ppsWidget)->tabMajorThickness = psInit->tabMajorThickness;
	(*ppsWidget)->tabMinorThickness = psInit->tabMinorThickness;
	(*ppsWidget)->tabMajorGap = psInit->tabMajorGap;
	(*ppsWidget)->tabMinorGap = psInit->tabMinorGap;
	(*ppsWidget)->tabVertOffset = psInit->tabVertOffset;
	(*ppsWidget)->tabHorzOffset = psInit->tabHorzOffset;
	(*ppsWidget)->majorOffset = psInit->majorOffset;
	(*ppsWidget)->minorOffset = psInit->minorOffset;
	(*ppsWidget)->majorPos = psInit->majorPos;
	(*ppsWidget)->minorPos = psInit->minorPos;
	(*ppsWidget)->pTabDisplay = psInit->pTabDisplay;
	(*ppsWidget)->pFormDisplay = psInit->pFormDisplay;

	formSetDefaultColours((W_FORM *)(*ppsWidget));

	/* Set up the tab data.
	 * All widget pointers have been zeroed by the memset above.
	 */
	(*ppsWidget)->numMajor = psInit->numMajor;
	for (major=0; major<psInit->numMajor; major++)
	{
		(*ppsWidget)->asMajor[major].numMinor = psInit->aNumMinors[major];
	}

	formInitialise((W_FORM *)*ppsWidget);

	return TRUE;
}

/* Free the tips strings for a tabbed form */
static void formFreeTips(W_TABFORM *psForm)
{
#if W_USE_STRHEAP
	UDWORD		minor,major;
	W_MAJORTAB	*psMajor;

	psMajor = psForm->asMajor;
	for(major = 0; major < psForm->numMajor; major++)
	{
		if (psMajor->pTip)
		{
			widgFreeString(psMajor->pTip);
		}
		for(minor = 0; minor < psMajor->numMinor; minor++)
		{
			if (psMajor->asMinor[minor].pTip)
			{
				widgFreeString(psMajor->asMinor[minor].pTip);
			}
		}
		psMajor++;
	}
#else
	psForm = psForm;
#endif
}

/* Free a tabbed form widget */
static void formFreeTabbed(W_TABFORM *psWidget)
{
	WIDGET			*psCurr;
	W_FORMGETALL	sGetAll;

	ASSERT((PTRVALID(psWidget, sizeof(W_TABFORM)),
		"formFreeTabbed: Invalid form pointer"));

	formFreeTips(psWidget);

	formInitGetAllWidgets((W_FORM *)psWidget,&sGetAll);
	psCurr = formGetAllWidgets(&sGetAll);
	while (psCurr)
	{
		widgReleaseWidgetList(psCurr);
		psCurr = formGetAllWidgets(&sGetAll);
	}
#if W_USE_MALLOC
	FREE(psWidget);
#else
	HEAP_FREE(psTFormHeap, psWidget);
#endif
}


/* Create a form widget data structure */
BOOL formCreate(W_FORM **ppsWidget, W_FORMINIT *psInit)
{
	/* Check the style bits are OK */
	if (psInit->style & ~(WFORM_TABBED | WFORM_INVISIBLE | WFORM_CLICKABLE |
						  WFORM_NOCLICKMOVE | WFORM_NOPRIMARY | WFORM_SECONDARY |
						  WIDG_HIDDEN))
	{
		ASSERT((FALSE, "formCreate: Unknown style bit"));
		return FALSE;
	}
	if ((psInit->style & WFORM_TABBED) &&
		(psInit->style & (WFORM_INVISIBLE | WFORM_CLICKABLE)))
	{
		ASSERT((FALSE, "formCreate: Tabbed form cannot be invisible or clickable"));
		return FALSE;
	}
	if ((psInit->style & WFORM_INVISIBLE) &&
		(psInit->style & WFORM_CLICKABLE))
	{
		ASSERT((FALSE, "formCreate: Cannot have an invisible clickable form"));
		return FALSE;
	}
	if (!(psInit->style & WFORM_CLICKABLE) &&
		((psInit->style & WFORM_NOPRIMARY) || (psInit->style & WFORM_SECONDARY)))
	{
		ASSERT((FALSE, "formCreate: Cannot set keys if the form isn't clickable"));
		return FALSE;
	}

	/* Create the correct type of form */
	if (psInit->style & WFORM_TABBED)
	{
		return formCreateTabbed((W_TABFORM **)ppsWidget, psInit);
	}
	else if (psInit->style & WFORM_CLICKABLE)
	{
		return formCreateClickable((W_CLICKFORM **)ppsWidget, psInit);
	}
	else
	{
		return formCreatePlain(ppsWidget, psInit);
	}

	return FALSE;
}


/* Free the memory used by a form */
void formFree(W_FORM *psWidget)
{
	if (psWidget->style & WFORM_TABBED)
	{
		formFreeTabbed((W_TABFORM *)psWidget);
	}
	else if (psWidget->style & WFORM_CLICKABLE)
	{
		formFreeClickable((W_CLICKFORM *)psWidget);
	}
	else
	{
		formFreePlain(psWidget);
	}
}


/* Add a widget to a form */
BOOL formAddWidget(W_FORM *psForm, WIDGET *psWidget, W_INIT *psInit)
{
	W_TABFORM	*psTabForm;
	WIDGET		**ppsList;
	W_MAJORTAB	*psMajor;

	ASSERT((PTRVALID(psWidget, sizeof(WIDGET)),
		"formAddWidget: Invalid widget pointer"));

	if (psForm->style & WFORM_TABBED)
	{
		ASSERT((PTRVALID(psForm, sizeof(W_TABFORM)),
			"formAddWidget: Invalid tab form pointer"));
		psTabForm = (W_TABFORM *)psForm;
		if (psInit->majorID >= psTabForm->numMajor)
		{
			ASSERT((FALSE, "formAddWidget: Major tab does not exist"));
			return FALSE;
		}
		psMajor = psTabForm->asMajor + psInit->majorID;
		if (psInit->minorID >= psMajor->numMinor)
		{
			ASSERT((FALSE, "formAddWidget: Minor tab does not exist"));
			return FALSE;
		}
		ppsList = &(psMajor->asMinor[psInit->minorID].psWidgets);
		psWidget->psNext = *ppsList;
		*ppsList = psWidget;
	}
	else
	{
		ASSERT((PTRVALID(psForm, sizeof(W_FORM)),
			"formAddWidget: Invalid form pointer"));
		psWidget->psNext = psForm->psWidgets;
		psForm->psWidgets = psWidget;
	}

	return TRUE;
}


/* Get the button state of a click form */
UDWORD formGetClickState(W_CLICKFORM *psForm)
{
	UDWORD State = 0;

	if (psForm->state & WCLICK_GREY)
	{
		State |= WBUT_DISABLE;
	}

	if (psForm->state & WCLICK_LOCKED)
	{
		State |= WBUT_LOCK;
	}

	if (psForm->state & WCLICK_CLICKLOCK)
	{
		State |= WBUT_CLICKLOCK;
	}

	return State;
}


/* Set the button state of a click form */
void formSetClickState(W_CLICKFORM *psForm, UDWORD state)
{
	ASSERT((!((state & WBUT_LOCK) && (state & WBUT_CLICKLOCK)),
		"widgSetButtonState: Cannot have WBUT_LOCK and WBUT_CLICKLOCK"));

	if (state & WBUT_DISABLE)
	{
		psForm->state |= WCLICK_GREY;
	}
	else
	{
		psForm->state &= ~WCLICK_GREY;
	}
	if (state & WBUT_LOCK)
	{
		psForm->state |= WCLICK_LOCKED;
	}
	else
	{
		psForm->state &= ~WCLICK_LOCKED;
	}
	if (state & WBUT_CLICKLOCK)
	{
		psForm->state |= WCLICK_CLICKLOCK;
	}
	else
	{
		psForm->state &= ~WCLICK_CLICKLOCK;
	}
}

/* Return the widgets currently displayed by a form */
WIDGET *formGetWidgets(W_FORM *psWidget)
{
	W_TABFORM	*psTabForm;
	W_MAJORTAB	*psMajor;

	if (psWidget->style & WFORM_TABBED)
	{
		psTabForm = (W_TABFORM *)psWidget;
		psMajor = psTabForm->asMajor + psTabForm->majorT;
		return psMajor->asMinor[psTabForm->minorT].psWidgets;
	}
	else
	{
		return psWidget->psWidgets;
	}
}


/* Initialise the formGetAllWidgets function */
void formInitGetAllWidgets(W_FORM *psWidget, W_FORMGETALL *psCtrl)
{
	if (psWidget->style & WFORM_TABBED)
	{
		psCtrl->psGAWList = NULL;
		psCtrl->psGAWForm = (W_TABFORM *)psWidget;
		psCtrl->psGAWMajor = psCtrl->psGAWForm->asMajor;
		psCtrl->GAWMajor = 0;
		psCtrl->GAWMinor = 0;
	}
	else
	{
		psCtrl->psGAWList = psWidget->psWidgets;
		psCtrl->psGAWForm = NULL;
	}
}


/* Repeated calls to this function will return widget lists
 * until all widgets in a form have been returned.
 * When a NULL list is returned, all widgets have been seen.
 */
WIDGET *formGetAllWidgets(W_FORMGETALL *psCtrl)
{
	WIDGET	*psRetList;

	if (psCtrl->psGAWForm == NULL)
	{
		/* Not a tabbed form, return the list */
		psRetList = psCtrl->psGAWList;
		psCtrl->psGAWList = NULL;
	}
	else
	{
		/* Working with a tabbed form - search for the first widget list */
		psRetList = NULL;
		while (psRetList == NULL && psCtrl->GAWMajor < psCtrl->psGAWForm->numMajor)
		{
			psRetList = psCtrl->psGAWMajor->asMinor[psCtrl->GAWMinor++].psWidgets;
			if (psCtrl->GAWMinor >= psCtrl->psGAWMajor->numMinor)
			{
				psCtrl->GAWMinor = 0;
				psCtrl->GAWMajor ++;
				psCtrl->psGAWMajor ++;
			}
		}
	}

	return psRetList;
}


/* Set the current tabs for a tab form */
void widgSetTabs(W_SCREEN *psScreen, UDWORD id, UWORD major, UWORD minor)
{
	W_TABFORM	*psForm;

	psForm = (W_TABFORM *)widgGetFromID(psScreen, id);
	if (psForm == NULL || !(psForm->style & WFORM_TABBED))
	{
		ASSERT((FALSE,"widgSetTabs: couldn't find tabbed form from id"));
		return;
	}
	ASSERT((PTRVALID(psForm, sizeof(W_TABFORM)),
		"widgSetTabs: Invalid tab form pointer"));

	if (major >= psForm->numMajor ||
		minor >= psForm->asMajor[major].numMinor)
	{
		ASSERT((FALSE, "widgSetTabs: invalid major or minor id"));
		return;
	}

	psForm->majorT = major;
	psForm->minorT = minor;
	psForm->asMajor[major].lastMinor = minor;
}


/* Get the current tabs for a tab form */
void widgGetTabs(W_SCREEN *psScreen, UDWORD id, UWORD *pMajor, UWORD *pMinor)
{
	W_TABFORM	*psForm;

	psForm = (W_TABFORM *)widgGetFromID(psScreen, id);
	if (psForm == NULL || psForm->type != WIDG_FORM ||
		!(psForm->style & WFORM_TABBED))
	{
		ASSERT((FALSE,"widgGetTabs: couldn't find tabbed form from id"));
		return;
	}
	ASSERT((PTRVALID(psForm, sizeof(W_TABFORM)),
		"widgGetTabs: Invalid tab form pointer"));

	*pMajor = psForm->majorT;
	*pMinor = psForm->minorT;
}


/* Set a colour on a form */
void widgSetColour(W_SCREEN *psScreen, UDWORD id, UDWORD colour,
				   UBYTE red, UBYTE green, UBYTE blue)
{
	W_TABFORM	*psForm;

	psForm = (W_TABFORM *)widgGetFromID(psScreen, id);
	if (psForm == NULL || psForm->type != WIDG_FORM)
	{
		ASSERT((FALSE,"widgSetColour: couldn't find form from id"));
		return;
	}
	ASSERT((PTRVALID(psForm, sizeof(W_FORM)),
		"widgSetColour: Invalid tab form pointer"));

	if (colour >= WCOL_MAX)
	{
		ASSERT((FALSE, "widgSetColour: Colour id out of range"));
		return;
	}
//	psForm->aColours[colour] = screenGetCacheColour(red,green,blue);
	psForm->aColours[colour] = (UBYTE)pal_GetNearestColour(red,green,blue);
}


/* Return the origin on the form from which button locations are calculated */
void formGetOrigin(W_FORM *psWidget, SDWORD *pXOrigin, SDWORD *pYOrigin)
{
	W_TABFORM	*psTabForm;

	ASSERT((PTRVALID(psWidget, sizeof(W_FORM)),
		"formGetOrigin: Invalid form pointer"));

	if (psWidget->style & WFORM_TABBED)
	{
		psTabForm = (W_TABFORM *)psWidget;
		if(psTabForm->majorPos == WFORM_TABTOP) {
			*pYOrigin = psTabForm->tabMajorThickness;
		} else if(psTabForm->minorPos == WFORM_TABTOP) {
			*pYOrigin = psTabForm->tabMinorThickness;
		} else {
			*pYOrigin = 0;
		}
		if(psTabForm->majorPos == WFORM_TABLEFT) {
			*pXOrigin = psTabForm->tabMajorThickness;
		} else if(psTabForm->minorPos == WFORM_TABLEFT) {
			*pXOrigin = psTabForm->tabMinorThickness;
		} else {
			*pXOrigin = 0;
		}
//		if ((psTabForm->majorPos == WFORM_TABTOP) ||
//			(psTabForm->minorPos == WFORM_TABTOP))
//		{
//			*pYOrigin = psTabForm->tabThickness;
//		}
//		else
//		{
//			*pYOrigin = 0;
//		}
//		if ((psTabForm->majorPos == WFORM_TABLEFT) ||
//			(psTabForm->minorPos == WFORM_TABLEFT))
//		{
//			*pXOrigin = psTabForm->tabThickness;
//		}
//		else
//		{
//			*pXOrigin = 0;
//		}
	}
	else
	{
		*pXOrigin = 0;
		*pYOrigin = 0;
	}
}


/* Initialise a form widget before running it */
void formInitialise(W_FORM *psWidget)
{
	W_TABFORM	*psTabForm;
	W_CLICKFORM	*psClickForm;
	UDWORD i;

	if (psWidget->style & WFORM_TABBED)
	{
		ASSERT((PTRVALID(psWidget, sizeof(W_TABFORM)),
			"formInitialise: invalid tab form pointer"));
		psTabForm = (W_TABFORM *)psWidget;
		psTabForm->majorT = 0;
		psTabForm->minorT = 0;
		psTabForm->tabHiLite = (UWORD)(-1);
		for (i=0; i<psTabForm->numMajor; i++)
		{
			psTabForm->asMajor[i].lastMinor = 0;
		}
	}
	else if (psWidget->style & WFORM_CLICKABLE)
	{
		ASSERT((PTRVALID(psWidget, sizeof(W_CLICKFORM)),
			"formInitialise: invalid clickable form pointer"));
		psClickForm = (W_CLICKFORM *)psWidget;
		psClickForm->state = WCLICK_NORMAL;
	}
	else
	{
		ASSERT((PTRVALID(psWidget, sizeof(W_FORM)),
			"formInitialise: invalid form pointer"));
	}

	psWidget->psLastHiLite = NULL;
}


/* Choose a horizontal tab from a coordinate */
static BOOL formPickHTab(TAB_POS *psTabPos,
						 SDWORD x0, SDWORD y0,
						 UDWORD width, UDWORD height, UDWORD gap,
						 UDWORD number, SDWORD fx, SDWORD fy)
{
	SDWORD	x, y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS 
	if (number == 1)
	{
		/* Don't have single tabs */
		return FALSE;
	}
#endif

	x = x0;
	y1 = y0 + height;
	for (i=0; i < number; i++)
	{
//		if (fx >= x && fx <= x + (SDWORD)(width - gap) &&
		if (fx >= x && fx <= x + (SDWORD)(width) &&
			fy >= y0 && fy <= y1)
		{
			/* found a tab under the coordinate */
			psTabPos->index = i;
			psTabPos->x = x;
			psTabPos->y = y0;
			psTabPos->width = width;
			psTabPos->height = height;
			return TRUE;
		}

		x += width + gap;
	}

	/* Didn't find any  */
	return FALSE;
}


/* Choose a vertical tab from a coordinate */
static BOOL formPickVTab(TAB_POS *psTabPos,
						 SDWORD x0, SDWORD y0,
						 UDWORD width, UDWORD height, UDWORD gap,
						 UDWORD number, SDWORD fx, SDWORD fy)
{
	SDWORD	x1, y;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS 
	if (number == 1)
	{
		/* Don't have single tabs */
		return FALSE;
	}
#endif

	x1 = x0 + width;
	y = y0;
	for (i=0; i < (SDWORD)number; i++)
	{
		if (fx >= x0 && fx <= x1 &&
			fy >= y && fy <= y + (SDWORD)(height))
//			fy >= y && fy <= y + (SDWORD)(height - gap))
		{
			/* found a tab under the coordinate */
			psTabPos->index = i;
			psTabPos->x = x0;
			psTabPos->y = y;
			psTabPos->width = width;
			psTabPos->height = height;
			return TRUE;
		}

		y += height + gap;
	}

	/* Didn't find any */
	return FALSE;
}


/* Find which tab is under a form coordinate */
static BOOL formPickTab(W_TABFORM *psForm, UDWORD fx, UDWORD fy,
						TAB_POS *psTabPos)
{
	SDWORD		x0,y0, x1,y1;
	W_MAJORTAB	*psMajor;
	SDWORD	xOffset,yOffset;
	SDWORD	xOffset2,yOffset2;

	/* Get the basic position of the form */
	x0 = 0;
	y0 = 0;
	x1 = psForm->width;
	y1 = psForm->height;

	/* Adjust for where the tabs are */
	if(psForm->majorPos == WFORM_TABLEFT) {
		x0 += psForm->tabMajorThickness;
	} else if(psForm->minorPos == WFORM_TABLEFT) {
		x0 += psForm->tabMinorThickness;
	}
	if(psForm->majorPos == WFORM_TABRIGHT) {
		x1 -= psForm->tabMajorThickness;
	} else if (psForm->minorPos == WFORM_TABRIGHT) {
		x1 -= psForm->tabMinorThickness;
	}
	if(psForm->majorPos == WFORM_TABTOP) {
		y0 += psForm->tabMajorThickness;
	} else if(psForm->minorPos == WFORM_TABTOP) {
		y0 += psForm->tabMinorThickness;
	}
	if(psForm->majorPos == WFORM_TABBOTTOM) {
		y1 -= psForm->tabMajorThickness;
	} else if(psForm->minorPos == WFORM_TABBOTTOM) {
		y1 -= psForm->tabMinorThickness;
	}

//		/* Adjust for where the tabs are */
//	if (psForm->majorPos == WFORM_TABLEFT || psForm->minorPos == WFORM_TABLEFT)
//	{
//		x0 += psForm->tabThickness;
//	}
//	if (psForm->majorPos == WFORM_TABRIGHT || psForm->minorPos == WFORM_TABRIGHT)
//	{
//		x1 -= psForm->tabThickness;
//	}
//	if (psForm->majorPos == WFORM_TABTOP || psForm->minorPos == WFORM_TABTOP)
//	{
//		y0 += psForm->tabThickness;
//	}
//	if (psForm->majorPos == WFORM_TABBOTTOM || psForm->minorPos == WFORM_TABBOTTOM)
//	{
//		y1 -= psForm->tabThickness;
//	}


	xOffset = yOffset = 0;
	switch (psForm->minorPos)
	{
		case WFORM_TABTOP:
			yOffset = psForm->tabVertOffset;
			break;

		case WFORM_TABLEFT:
			xOffset = psForm->tabHorzOffset;
			break;

		case WFORM_TABBOTTOM:
			yOffset = psForm->tabVertOffset;
			break;

		case WFORM_TABRIGHT:
			xOffset = psForm->tabHorzOffset;
			break;
	}

	xOffset2 = yOffset2 = 0;
	/* Check the major tabs */
	switch (psForm->majorPos)
	{
	case WFORM_TABTOP:
		if (formPickHTab(psTabPos, x0+psForm->majorOffset - xOffset, y0 - psForm->tabMajorThickness,
					 psForm->majorSize, psForm->tabMajorThickness, psForm->tabMajorGap,
					 psForm->numMajor, fx, fy))
		{
			return TRUE;
		}
		yOffset2 = -psForm->tabVertOffset;
		break;
	case WFORM_TABBOTTOM:
		if (formPickHTab(psTabPos, x0+psForm->majorOffset - xOffset, y1,
					 psForm->majorSize, psForm->tabMajorThickness, psForm->tabMajorGap,
					 psForm->numMajor, fx, fy))
		{
			return TRUE;
		}
		break;
	case WFORM_TABLEFT:
		if (formPickVTab(psTabPos, x0 - psForm->tabMajorThickness, y0+psForm->majorOffset - yOffset,
					 psForm->tabMajorThickness, psForm->majorSize, psForm->tabMajorGap,
					 psForm->numMajor, fx, fy))
		{
			return TRUE;
		}
		xOffset2 = psForm->tabHorzOffset;
		break;
	case WFORM_TABRIGHT:
		if (formPickVTab(psTabPos, x1, y0+psForm->majorOffset - yOffset,
					 psForm->tabMajorThickness, psForm->majorSize, psForm->tabMajorGap,
					 psForm->numMajor, fx, fy))
		{
			return TRUE;
		}
		break;
	case WFORM_TABNONE:
		ASSERT((FALSE, "formDisplayTabbed: Cannot have a tabbed form with no major tabs"));
		break;
	}

	/* Draw the minor tabs */
	psMajor = psForm->asMajor + psForm->majorT;
	switch (psForm->minorPos)
	{
	case WFORM_TABTOP:
		if (formPickHTab(psTabPos, x0+psForm->minorOffset - xOffset2, y0 - psForm->tabMinorThickness,
					 psForm->minorSize, psForm->tabMinorThickness, psForm->tabMinorGap,
					 psMajor->numMinor, fx, fy))
		{
			psTabPos->index += psForm->numMajor;
			return TRUE;
		}
		break;
	case WFORM_TABBOTTOM:
		if (formPickHTab(psTabPos, x0+psForm->minorOffset - xOffset2, y1,
					 psForm->minorSize, psForm->tabMinorThickness, psForm->tabMinorGap,
					 psMajor->numMinor, fx, fy))
		{
			psTabPos->index += psForm->numMajor;
			return TRUE;
		}
		break;
	case WFORM_TABLEFT:
		if (formPickVTab(psTabPos, x0+xOffset - psForm->tabMinorThickness, y0+psForm->minorOffset - yOffset2,
					 psForm->tabMinorThickness, psForm->minorSize, psForm->tabMinorGap,
					 psMajor->numMinor, fx, fy))
		{
			psTabPos->index += psForm->numMajor;
			return TRUE;
		}
		break;
	case WFORM_TABRIGHT:
		if (formPickVTab(psTabPos, x1+xOffset, y0+psForm->minorOffset - yOffset2,
					 psForm->tabMinorThickness, psForm->minorSize, psForm->tabMinorGap,
					 psMajor->numMinor, fx, fy))
		{
			psTabPos->index += psForm->numMajor;
			return TRUE;
		}
		break;
	/* case WFORM_TABNONE - no minor tabs so nothing to display */
	}

	return FALSE;
}

extern UDWORD gameTime2;

/* Run a form widget */
void formRun(W_FORM *psWidget, W_CONTEXT *psContext)
{
	SDWORD		mx,my;
	TAB_POS		sTabPos;
	STRING		*pTip;
	W_TABFORM	*psTabForm;

	if(psWidget->style & WFORM_CLICKABLE) {
		if(((W_CLICKFORM *)psWidget)->state & WCLICK_FLASH) {
			if (((gameTime2/250) % 2) == 0) {
				((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASHON;
			} else {
				((W_CLICKFORM *)psWidget)->state |= WCLICK_FLASHON;
			}
		}
	}

	if (psWidget->style & WFORM_TABBED)
	{
		mx = psContext->mx;
		my = psContext->my;
		psTabForm = (W_TABFORM *)psWidget;

		/* If the mouse is over the form, see if any tabs need to be hilited */
		if (mx >= 0 && mx <= psTabForm->width &&
			my >= 0 && my <= psTabForm->height)
		{
			if (formPickTab(psTabForm, mx,my, &sTabPos))
			{
				if (psTabForm->tabHiLite != (UWORD)sTabPos.index)
				{
					/* Got a new tab - start the tool tip if there is one */
					psTabForm->tabHiLite = (UWORD)sTabPos.index;
					if (sTabPos.index >= psTabForm->numMajor)
					{
						pTip = psTabForm->asMajor[psTabForm->majorT].
									asMinor[sTabPos.index - psTabForm->numMajor].pTip;
					}
					else
					{
						pTip = psTabForm->asMajor[sTabPos.index].pTip;
					}
					if (pTip)
					{
						/* Got a tip - start it off */
						tipStart((WIDGET *)psTabForm, pTip,
								 psContext->psScreen->TipFontID, psTabForm->aColours,
								 sTabPos.x + psContext->xOffset,
								 sTabPos.y + psContext->yOffset,
								 sTabPos.width,sTabPos.height);
					}
					else
					{
						/* No tip - clear any old tip */
						tipStop((WIDGET *)psWidget);
					}
				}
			}
			else
			{
				/* No tab - clear the tool tip */
				tipStop((WIDGET *)psWidget);
				/* And clear the hilite */
				psTabForm->tabHiLite = (UWORD)(-1);
			}
		}
	}
}


void formSetFlash(W_FORM *psWidget)
{
	if(psWidget->style & WFORM_CLICKABLE) {
		((W_CLICKFORM *)psWidget)->state |= WCLICK_FLASH;
	}
}


void formClearFlash(W_FORM *psWidget)
{
	if(psWidget->style & WFORM_CLICKABLE) {
		((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASH;
		((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASHON;
	}
}


/* Respond to a mouse click */
void formClicked(W_FORM *psWidget, UDWORD key)
{
	W_CLICKFORM		*psClickForm;

	/* Stop the tip if there is one */
	tipStop((WIDGET *)psWidget);

	if (psWidget->style & WFORM_CLICKABLE)
	{
		/* Can't click a button if it is disabled or locked down */
		if (!(((W_CLICKFORM *)psWidget)->state & (WCLICK_GREY | WCLICK_LOCKED)))
		{
			// Check this is the correct key
			if ((!(psWidget->style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
				((psWidget->style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
			{
				((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASH;	// Stop it flashing
				((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASHON;
				((W_CLICKFORM *)psWidget)->state |= WCLICK_DOWN;

				psClickForm = (W_CLICKFORM *)psWidget;

				if(psClickForm->AudioCallback) {
					psClickForm->AudioCallback(psClickForm->ClickedAudioID);
				}
			}
		}
	}
}


/* Respond to a mouse form up */
void formReleased(W_FORM *psWidget, UDWORD key, W_CONTEXT *psContext)
{
	W_TABFORM	*psTabForm;
	W_CLICKFORM	*psClickForm;
	TAB_POS		sTabPos;

	if (psWidget->style & WFORM_TABBED)
	{
		psTabForm = (W_TABFORM *)psWidget;
		/* See if a tab has been clicked on */
		if (formPickTab(psTabForm, psContext->mx,psContext->my, &sTabPos))
		{
			if (sTabPos.index >= psTabForm->numMajor)
			{
				/* Clicked on a minor tab */
				psTabForm->minorT = (UWORD)(sTabPos.index - psTabForm->numMajor);
				psTabForm->asMajor[psTabForm->majorT].lastMinor = psTabForm->minorT;
				widgSetReturn((WIDGET *)psWidget);
			}
			else
			{
				/* Clicked on a major tab */
				psTabForm->majorT = (UWORD)sTabPos.index;
				psTabForm->minorT = psTabForm->asMajor[sTabPos.index].lastMinor;
				widgSetReturn((WIDGET *)psWidget);
			}
		}
	}
	else if (psWidget->style & WFORM_CLICKABLE)
	{
		psClickForm = (W_CLICKFORM *)psWidget;
		if (psClickForm->state & WCLICK_DOWN)
		{
			// Check this is the correct key
			if ((!(psWidget->style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
				((psWidget->style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
			{
				widgSetReturn((WIDGET *)psClickForm);
				psClickForm->state &= ~WCLICK_DOWN;
			}
		}
	}
}


/* Respond to a mouse moving over a form */
void formHiLite(W_FORM *psWidget, W_CONTEXT *psContext)
{
	W_CLICKFORM		*psClickForm;

	if (psWidget->style & WFORM_CLICKABLE)
	{
		psClickForm = (W_CLICKFORM *)psWidget;

		psClickForm->state |= WCLICK_HILITE;

		/* If there is a tip string start the tool tip */
		if (psClickForm->pTip)
		{
			tipStart((WIDGET *)psClickForm, psClickForm->pTip,
					 psContext->psScreen->TipFontID, psContext->psForm->aColours,
					 psWidget->x + psContext->xOffset, psWidget->y + psContext->yOffset,
					 psWidget->width, psWidget->height);
		}

		if(psClickForm->AudioCallback) {
			psClickForm->AudioCallback(psClickForm->HilightAudioID);
		}
	}
}


/* Respond to the mouse moving off a form */
void formHiLiteLost(W_FORM *psWidget, W_CONTEXT *psContext)
{
	/* If one of the widgets were hilited that has to loose it as well */
	if (psWidget->psLastHiLite != NULL)
	{
		widgHiLiteLost(psWidget->psLastHiLite, psContext);
	}
	if (psWidget->style & WFORM_TABBED)
	{
		((W_TABFORM *)psWidget)->tabHiLite = (UWORD)(-1);
	}
	if (psWidget->style & WFORM_CLICKABLE)
	{
		((W_CLICKFORM *)psWidget)->state &= ~(WCLICK_DOWN | WCLICK_HILITE);
	}
	/* Clear the tool tip if there is one */
	tipStop((WIDGET *)psWidget);
}

/* Display a form */
void formDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD	x0,y0,x1,y1;

	if (!(psWidget->style & WFORM_INVISIBLE))
	{
		x0 = psWidget->x + xOffset;
		y0 = psWidget->y + yOffset;
		x1 = x0 + psWidget->width;
		y1 = y0 + psWidget->height;

		pie_BoxFillIndex(x0+1,y0+1, x1-1,y1-1,WCOL_BKGRND);
		iV_Line(x0,y1,x0,y0,*(pColours + WCOL_LIGHT));
		iV_Line(x0,y0,x1,y0,*(pColours + WCOL_LIGHT));
		iV_Line(x1,y0,x1,y1,*(pColours + WCOL_DARK));
		iV_Line(x1,y1,x0,y1,*(pColours + WCOL_DARK));
	}
}


/* Display a clickable form */
void formDisplayClickable(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD			x0,y0,x1,y1;
	W_CLICKFORM		*psForm;

	psForm = (W_CLICKFORM *)psWidget;
	x0 = psWidget->x + xOffset;
	y0 = psWidget->y + yOffset;
	x1 = x0 + psWidget->width;
	y1 = y0 + psWidget->height;

	/* Fill the background */
	pie_BoxFillIndex(x0+1,y0+1, x1-1,y1-1,WCOL_BKGRND);

	/* Display the border */
	if (psForm->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)) 
	{
		/* Form down */
		iV_Line(x0,y1,x0,y0,*(pColours + WCOL_DARK));
		iV_Line(x0,y0,x1,y0,*(pColours + WCOL_DARK));
		iV_Line(x1,y0,x1,y1,*(pColours + WCOL_LIGHT));
		iV_Line(x1,y1,x0,y1,*(pColours + WCOL_LIGHT));
	}
	else
	{
		/* Form up */
		iV_Line(x0,y1,x0,y0,*(pColours + WCOL_LIGHT));
		iV_Line(x0,y0,x1,y0,*(pColours + WCOL_LIGHT));
		iV_Line(x1,y0,x1,y1,*(pColours + WCOL_DARK));
		iV_Line(x1,y1,x0,y1,*(pColours + WCOL_DARK));
	}
}


/* Draw top tabs */
static void formDisplayTTabs(W_TABFORM *psForm,SDWORD x0, SDWORD y0,
							 UDWORD width, UDWORD height,
							 UDWORD number, UDWORD selected, UDWORD hilite,
							 UDWORD *pColours,UDWORD TabType,UDWORD TabGap)
{
	SDWORD	x,x1, y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x = x0 + 2;
	x1 = x + width - 2;
	y1 = y0 + height;
	for (i=0; i < number; i++)
	{
		if(psForm->pTabDisplay) {
			psForm->pTabDisplay((WIDGET*)psForm,TabType,WFORM_TABTOP,i,i==selected,i==hilite,x,y0,width,height);
		} else {
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x+1,y0+1, x1-1,y1,WCOL_BKGRND);
				/* Draw the outline */
				iV_Line(x,y0+2, x,y1-1,*(pColours + WCOL_LIGHT));
				iV_Line(x,y0+2, x+2,y0,*(pColours + WCOL_LIGHT));
				iV_Line(x+2,y0, x1-1,y0,*(pColours + WCOL_LIGHT));
				iV_Line(x1,y0+1, x1,y1,*(pColours + WCOL_DARK));
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x+1,y0+2, x1-1,y1-1,WCOL_BKGRND);
				/* Draw the outline */
				iV_Line(x,y0+3, x,y1-1,*(pColours + WCOL_LIGHT));
				iV_Line(x,y0+3, x+2,y0+1,*(pColours + WCOL_LIGHT));
				iV_Line(x+2,y0+1, x1-1,y0+1,*(pColours + WCOL_LIGHT));
				iV_Line(x1,y0+2, x1,y1-1,*(pColours + WCOL_DARK));
			}
			if (i == hilite)
			{
				/* Draw the hilite box */
				iV_Box(x+2,y0+4, x1-3, y1-3,*(pColours + WCOL_HILITE));
			}
		}
		x += width + TabGap;
		x1 += width + TabGap;
	}
}


/* Draw bottom tabs */
static void formDisplayBTabs(W_TABFORM *psForm,SDWORD x0, SDWORD y0,
							 UDWORD width, UDWORD height,
							 UDWORD number, UDWORD selected, UDWORD hilite,
							 UDWORD *pColours,UDWORD TabType,UDWORD TabGap)
{
	SDWORD	x,x1, y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x = x0 + 2;
	x1 = x + width - 2;
	y1 = y0 + height;
	for (i=0; i < number; i++)
	{
		if(psForm->pTabDisplay) {
			psForm->pTabDisplay((WIDGET*)psForm,TabType,WFORM_TABBOTTOM,i,i==selected,i==hilite,x,y0,width,height);
		} else {
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x+1,y0, x1-1,y1-1,WCOL_BKGRND);
				/* Draw the outline */
				iV_Line(x,y0, x,y1-1,*(pColours + WCOL_LIGHT));
				iV_Line(x,y1, x1-3,y1,*(pColours + WCOL_DARK));
				iV_Line(x1-2,y1, x1,y1-2,*(pColours + WCOL_DARK));
				iV_Line(x1,y1-3, x1,y0+1,*(pColours + WCOL_DARK));
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x+1,y0+1, x1-1,y1-2,WCOL_BKGRND);
				/* Draw the outline */
				iV_Line(x,y0+1, x,y1-1,*(pColours + WCOL_LIGHT));
				iV_Line(x+1,y1-1, x1-3,y1-1,*(pColours + WCOL_DARK));
				iV_Line(x1-2,y1-1, x1,y1-3,*(pColours + WCOL_DARK));
				iV_Line(x1,y1-4, x1,y0+1,*(pColours + WCOL_DARK));
			}
			if (i == hilite)
			{
				/* Draw the hilite box */
				iV_Box(x+2,y0+3, x1-3, y1-4,*(pColours + WCOL_HILITE));
			}
		}
		x += width + TabGap;
		x1 += width + TabGap;
	}
}


/* Draw left tabs */
static void formDisplayLTabs(W_TABFORM *psForm,SDWORD x0, SDWORD y0,
							 UDWORD width, UDWORD height,
							 UDWORD number, UDWORD selected, UDWORD hilite,
							 UDWORD *pColours,UDWORD TabType,UDWORD TabGap)
{
	SDWORD	x1, y,y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x1 = x0 + width;
	y = y0+2;
	y1 = y + height - 2;
	for (i=0; i < (SDWORD)number; i++)
	{
		if(psForm->pTabDisplay) {
			psForm->pTabDisplay((WIDGET*)psForm,TabType,WFORM_TABLEFT,i,i==selected,i==hilite,x0,y,width,height);
		} else {
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x0+1,y+1, x1,y1-1,WCOL_BKGRND);
				/* Draw the outline */
				iV_Line(x0,y, x1-1,y,*(pColours + WCOL_LIGHT));
				iV_Line(x0,y+1, x0,y1-2,*(pColours + WCOL_LIGHT));
				iV_Line(x0+1,y1-1, x0+2,y1,*(pColours + WCOL_DARK));
				iV_Line(x0+3,y1, x1,y1,*(pColours + WCOL_DARK));
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x0+2,y+1, x1-1,y1-1,WCOL_BKGRND);
				/* Draw the outline */
				iV_Line(x0+1,y, x1-1,y,*(pColours + WCOL_LIGHT));
				iV_Line(x0+1,y+1, x0+1,y1-2,*(pColours + WCOL_LIGHT));
				iV_Line(x0+2,y1-1, x0+3,y1,*(pColours + WCOL_DARK));
				iV_Line(x0+4,y1, x1-1,y1,*(pColours + WCOL_DARK));
			}
			if (i == hilite)
			{
				iV_Box(x0+4,y+2, x1-2, y1-3,*(pColours + WCOL_HILITE));
			}
		}
		y += height + TabGap;
		y1 += height + TabGap;
	}
}


/* Draw right tabs */
static void formDisplayRTabs(W_TABFORM *psForm,SDWORD x0, SDWORD y0,
							 UDWORD width, UDWORD height,
							 UDWORD number, UDWORD selected, UDWORD hilite,
							 UDWORD *pColours,UDWORD TabType,UDWORD TabGap)
{
	SDWORD	x1, y,y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x1 = x0 + width;
	y = y0+2;
	y1 = y + height - 2;
	for (i=0; i < (SDWORD)number; i++)
	{
		if(psForm->pTabDisplay) {
			psForm->pTabDisplay((WIDGET*)psForm,TabType,WFORM_TABRIGHT,i,i==selected,i==hilite,x0,y,width,height);
		} else {
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x0,y+1, x1-1,y1-1,(UBYTE)*(pColours + WCOL_BKGRND));
				/* Draw the outline */
				iV_Line(x0,y, x1-1,y,*(pColours + WCOL_LIGHT));
				iV_Line(x1,y, x1,y1-2,*(pColours + WCOL_DARK));
				iV_Line(x1-1,y1-1, x1-2,y1,*(pColours + WCOL_DARK));
				iV_Line(x1-3,y1, x0,y1,*(pColours + WCOL_DARK));
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFillIndex(x0+1,y+1, x1-2,y1-1,(UBYTE)*(pColours + WCOL_BKGRND));
				/* Draw the outline */
				iV_Line(x0+1,y, x1-1,y,*(pColours + WCOL_LIGHT));
				iV_Line(x1-1,y, x1-1,y1-2,*(pColours + WCOL_DARK));
				iV_Line(x1-2,y1-1, x1-3,y1,*(pColours + WCOL_DARK));
				iV_Line(x1-4,y1, x0+1,y1,*(pColours + WCOL_DARK));
			}
			if (i == hilite)
			{
				iV_Box(x0+2,y+2, x1-4, y1-3,*(pColours + WCOL_HILITE));
			}
		}
		y += height + TabGap;
		y1 += height + TabGap;
	}
}


/* Display a tabbed form */
void formDisplayTabbed(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD		x0,y0,x1,y1;
	W_TABFORM	*psForm;
	W_MAJORTAB	*psMajor;

	psForm = (W_TABFORM *)psWidget;

	/* Get the basic position of the form */
	x0 = psForm->x + xOffset;
	y0 = psForm->y + yOffset;
	x1 = x0 + psForm->width;
	y1 = y0 + psForm->height;

	/* Adjust for where the tabs are */
	if(psForm->majorPos == WFORM_TABLEFT) {
		x0 += psForm->tabMajorThickness - psForm->tabHorzOffset;
	} else if(psForm->minorPos == WFORM_TABLEFT) {
		x0 += psForm->tabMinorThickness - psForm->tabHorzOffset;
	}
	if(psForm->majorPos == WFORM_TABRIGHT) {
		x1 -= psForm->tabMajorThickness - psForm->tabHorzOffset;
	} else if(psForm->minorPos == WFORM_TABRIGHT) {
		x1 -= psForm->tabMinorThickness - psForm->tabHorzOffset;
	}
	if(psForm->majorPos == WFORM_TABTOP) {
		y0 += psForm->tabMajorThickness - psForm->tabVertOffset;
	} else if(psForm->minorPos == WFORM_TABTOP) {
		y0 += psForm->tabMinorThickness - psForm->tabVertOffset;
	}
	if(psForm->majorPos == WFORM_TABBOTTOM) {
		y1 -= psForm->tabMajorThickness - psForm->tabVertOffset;
	} else if(psForm->minorPos == WFORM_TABBOTTOM) {
		y1 -= psForm->tabMinorThickness - psForm->tabVertOffset;
	}

	/* Adjust for where the tabs are */
//	if (psForm->majorPos == WFORM_TABLEFT || psForm->minorPos == WFORM_TABLEFT)
//	{
//		x0 += psForm->tabThickness - psForm->tabHorzOffset;
//	}
//	if (psForm->majorPos == WFORM_TABRIGHT || psForm->minorPos == WFORM_TABRIGHT)
//	{
//		x1 -= psForm->tabThickness - psForm->tabHorzOffset;
//	}
//	if (psForm->majorPos == WFORM_TABTOP || psForm->minorPos == WFORM_TABTOP)
//	{
//		y0 += psForm->tabThickness - psForm->tabVertOffset;
//	}
//	if (psForm->majorPos == WFORM_TABBOTTOM || psForm->minorPos == WFORM_TABBOTTOM)
//	{
//		y1 -= psForm->tabThickness - psForm->tabVertOffset;
//	}

	if(psForm->pFormDisplay) {
		psForm->pFormDisplay((WIDGET *)psForm, xOffset, yOffset, psForm->aColours);
	} else {
	//	screenSetLineColour(0,0,0);
		/* Draw the form outline */
		if (!(psForm->style & WFORM_INVISIBLE))
		{
			pie_BoxFillIndex(x0,y0,x1,y1,WCOL_BKGRND);
			iV_Line(x0,y1,x0,y0,*(pColours + WCOL_LIGHT));
			iV_Line(x0,y0,x1,y0,*(pColours + WCOL_LIGHT));
			iV_Line(x1,y0,x1,y1,*(pColours + WCOL_DARK));
			iV_Line(x1,y1,x0,y1,*(pColours + WCOL_DARK));
		}
	}

	/* Draw the major tabs */
	switch (psForm->majorPos)
	{
	case WFORM_TABTOP:
		formDisplayTTabs(psForm,x0+psForm->majorOffset, y0 - psForm->tabMajorThickness + psForm->tabVertOffset,
						 psForm->majorSize, psForm->tabMajorThickness,
						 psForm->numMajor, psForm->majorT, psForm->tabHiLite,
						 pColours,TAB_MAJOR,psForm->tabMajorGap);
		break;
	case WFORM_TABBOTTOM:
		formDisplayBTabs(psForm,x0+psForm->majorOffset, y1 + psForm->tabVertOffset,
						 psForm->majorSize, psForm->tabMajorThickness,
						 psForm->numMajor, psForm->majorT, psForm->tabHiLite,
						 pColours,TAB_MAJOR,psForm->tabMajorGap);
		break;
	case WFORM_TABLEFT:
		formDisplayLTabs(psForm,x0 - psForm->tabMajorThickness + psForm->tabHorzOffset, y0+psForm->majorOffset,
						 psForm->tabMajorThickness, psForm->majorSize,
						 psForm->numMajor, psForm->majorT, psForm->tabHiLite,
						 pColours,TAB_MAJOR,psForm->tabMajorGap);
		break;
	case WFORM_TABRIGHT:
		formDisplayRTabs(psForm,x1 - psForm->tabHorzOffset, y0+psForm->majorOffset,
						 psForm->tabMajorThickness, psForm->majorSize,
						 psForm->numMajor, psForm->majorT, psForm->tabHiLite,
						 pColours,TAB_MAJOR,psForm->tabMajorGap);
		break;
	case WFORM_TABNONE:
		ASSERT((FALSE, "formDisplayTabbed: Cannot have a tabbed form with no major tabs"));
		break;
	}

	/* Draw the minor tabs */
	psMajor = psForm->asMajor + psForm->majorT;
	switch (psForm->minorPos)
	{
	case WFORM_TABTOP:
		formDisplayTTabs(psForm,x0 + psForm->minorOffset, y0 - psForm->tabMinorThickness + psForm->tabVertOffset,
						 psForm->minorSize, psForm->tabMinorThickness,
						 psMajor->numMinor, psForm->minorT,
						 psForm->tabHiLite - psForm->numMajor, pColours,TAB_MINOR,psForm->tabMinorGap);
		break;
	case WFORM_TABBOTTOM:
		formDisplayBTabs(psForm,x0 + psForm->minorOffset, y1 + psForm->tabVertOffset,
						 psForm->minorSize, psForm->tabMinorThickness,
						 psMajor->numMinor, psForm->minorT,
						 psForm->tabHiLite - psForm->numMajor, pColours,TAB_MINOR,psForm->tabMinorGap);
		break;
	case WFORM_TABLEFT:
		formDisplayLTabs(psForm,x0 - psForm->tabMinorThickness + psForm->tabHorzOffset + psForm->minorOffset, 
						 y0+psForm->minorOffset,
						 psForm->tabMinorThickness, psForm->minorSize,
						 psMajor->numMinor, psForm->minorT,
						 psForm->tabHiLite - psForm->numMajor, pColours,TAB_MINOR,psForm->tabMinorGap);
		break;
	case WFORM_TABRIGHT:
		formDisplayRTabs(psForm,x1 + psForm->tabHorzOffset, y0+psForm->minorOffset,
						 psForm->tabMinorThickness, psForm->minorSize,
						 psMajor->numMinor, psForm->minorT,
						 psForm->tabHiLite - psForm->numMajor, pColours,TAB_MINOR,psForm->tabMinorGap);
		break;
	/* case WFORM_TABNONE - no minor tabs so nothing to display */
	}

}
