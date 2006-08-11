/*
 * Label.c
 *
 * Functions for the label widget.
 */


#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "label.h"
#include "form.h"
#include "tip.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"

/* The widget heaps */
OBJ_HEAP	*psLabHeap;

/* Create a button widget data structure */
BOOL labelCreate(W_LABEL **ppsWidget, W_LABINIT *psInit)
{
	/* Do some validation on the initialisation struct */
	if (psInit->style & ~(WLAB_PLAIN | WLAB_ALIGNLEFT |
						   WLAB_ALIGNRIGHT | WLAB_ALIGNCENTRE | WIDG_HIDDEN))
	{
		ASSERT((FALSE, "Unknown button style"));
		return FALSE;
	}

//	ASSERT((PTRVALID(psInit->psFont, sizeof(PROP_FONT)),
//		"labelCreate: Invalid font pointer"));

	/* Allocate the required memory */
#if W_USE_MALLOC
	*ppsWidget = (W_LABEL *)MALLOC(sizeof(W_LABEL));
	if (*ppsWidget == NULL)
#else
	if (!HEAP_ALLOC(psLabHeap, (void*) ppsWidget))
#endif
	{
		ASSERT((FALSE, "Out of memory"));
		return FALSE;
	}
	/* Allocate the memory for the tip and copy it if necessary */
	if (psInit->pTip)
	{
#if W_USE_STRHEAP
		if (!widgAllocCopyString(&(*ppsWidget)->pTip, psInit->pTip))
		{
			/* Out of memory - just carry on without the tip */
			ASSERT((FALSE, "buttonCreate: Out of memory"));
			(*ppsWidget)->pTip = NULL;
		}
#else
		(*ppsWidget)->pTip = psInit->pTip;
#endif
	}
	else
	{
		(*ppsWidget)->pTip = NULL;
	}

	/* Initialise the structure */
	(*ppsWidget)->type = WIDG_LABEL;
	(*ppsWidget)->id = psInit->id;
	(*ppsWidget)->formID = psInit->formID;
	(*ppsWidget)->style = psInit->style;
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
		(*ppsWidget)->display = labelDisplay;
	}
	(*ppsWidget)->callback = psInit->pCallback;
	(*ppsWidget)->pUserData = psInit->pUserData;
	(*ppsWidget)->UserData = psInit->UserData;
//	(*ppsWidget)->psFont = psInit->psFont;
	(*ppsWidget)->FontID = psInit->FontID;

	if (psInit->pText)
	{
		widgCopyString((*ppsWidget)->aText, psInit->pText);
	}
	else
	{
		*(*ppsWidget)->aText = 0;
	}

	return TRUE;
}


/* Free the memory used by a button */
void labelFree(W_LABEL *psWidget)
{
#if W_USE_STRHEAP
	if (psWidget->pTip)
	{
		widgFreeString(psWidget->pTip);
	}
#endif

	ASSERT((PTRVALID(psWidget, sizeof(W_LABEL)),
		"labelFree: Invalid label pointer"));

#if W_USE_MALLOC
	FREE(psWidget);
#else
	HEAP_FREE(psLabHeap, psWidget);
#endif
}


/* label display function */
void labelDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	SDWORD		fx,fy, fw;
	W_LABEL		*psLabel;
//	PROP_FONT	*psFont;
	int			FontID;

	psLabel = (W_LABEL *)psWidget;
//	psFont = psLabel->psFont;
	FontID = psLabel->FontID;

	iV_SetFont(FontID);
//	fontSetCacheColour(*(pColours + WCOL_TEXT));
	iV_SetTextColour((UWORD)*(pColours + WCOL_TEXT));
	if (psLabel->style & WLAB_ALIGNCENTRE)
	{
  		fw = iV_GetTextWidth(psLabel->aText);
		fx = xOffset + psLabel->x + (psLabel->width - fw) / 2;
	}
	else if (psLabel->style & WLAB_ALIGNRIGHT)
	{
  		fw = iV_GetTextWidth(psLabel->aText);
		fx = xOffset + psLabel->x + psLabel->width - fw;
	}
	else
	{
		fx = xOffset + psLabel->x;
	}
  	fy = yOffset + psLabel->y + (psLabel->height - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();
//	fy = yOffset + psLabel->y + (psLabel->height -
//			psFont->height + psFont->baseLine) / 2;
	iV_DrawText(psLabel->aText,fx,fy);
//	fontPrint(fx,fy, psLabel->aText);
}

/* Respond to a mouse moving over a label */
void labelHiLite(W_LABEL *psWidget, W_CONTEXT *psContext)
{
	psWidget->state |= WLABEL_HILITE;

	/* If there is a tip string start the tool tip */
	if (psWidget->pTip)
	{
		tipStart((WIDGET *)psWidget, psWidget->pTip, psContext->psScreen->TipFontID,
				 psContext->psForm->aColours,
				 psWidget->x + psContext->xOffset, psWidget->y + psContext->yOffset,
				 psWidget->width,psWidget->height);
	}
}


/* Respond to the mouse moving off a label */
void labelHiLiteLost(W_LABEL *psWidget)
{
	psWidget->state &= ~(WLABEL_HILITE);
	if (psWidget->pTip)
	{
		tipStop((WIDGET *)psWidget);
	}
}

