/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
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

/* Create a button widget data structure */
BOOL labelCreate(W_LABEL **ppsWidget, W_LABINIT *psInit)
{
	/* Do some validation on the initialisation struct */
	if (psInit->style & ~(WLAB_PLAIN | WLAB_ALIGNLEFT |
						   WLAB_ALIGNRIGHT | WLAB_ALIGNCENTRE | WIDG_HIDDEN))
	{
		ASSERT( FALSE, "Unknown button style" );
		return FALSE;
	}

	/* Allocate the required memory */
	*ppsWidget = (W_LABEL *)malloc(sizeof(W_LABEL));
	if (*ppsWidget == NULL)
	{
		ASSERT( FALSE, "Out of memory" );
		return FALSE;
	}
	/* Allocate the memory for the tip and copy it if necessary */
	(*ppsWidget)->pTip = psInit->pTip;

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
	(*ppsWidget)->FontID = psInit->FontID;

	if (psInit->pText)
	{
		strlcpy((*ppsWidget)->aText, psInit->pText, sizeof((*ppsWidget)->aText));
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
	ASSERT( psWidget != NULL,
		"labelFree: Invalid label pointer" );

	free(psWidget);
}


/* label display function */
void labelDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	SDWORD		fx,fy, fw;
	W_LABEL		*psLabel;
	int			FontID;

	psLabel = (W_LABEL *)psWidget;
	FontID = psLabel->FontID;

	iV_SetFont(FontID);
	iV_SetTextColour(pColours[WCOL_TEXT]);
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
	iV_DrawText(psLabel->aText,fx,fy);
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



