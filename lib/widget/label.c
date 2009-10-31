/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
/** @file
 *  Functions for the label widget.
 */

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "label.h"
#include "form.h"
#include "tip.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/textdraw.h"

/* Create a button widget data structure */
W_LABEL* labelCreate(const W_LABINIT* psInit)
{
	W_LABEL* psWidget;

	/* Do some validation on the initialisation struct */
	if (psInit->style & ~(WLAB_PLAIN | WLAB_ALIGNLEFT |
						   WLAB_ALIGNRIGHT | WLAB_ALIGNCENTRE | WIDG_HIDDEN))
	{
		ASSERT( false, "Unknown button style" );
		return NULL;
	}

	/* Allocate the required memory */
	psWidget = (W_LABEL *)malloc(sizeof(W_LABEL));
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "labelCreate: Out of memory");
		abort();
		return NULL;
	}
	/* Allocate the memory for the tip and copy it if necessary */
	psWidget->pTip = psInit->pTip;

	/* Initialise the structure */
	psWidget->type = WIDG_LABEL;
	psWidget->id = psInit->id;
	psWidget->formID = psInit->formID;
	psWidget->style = psInit->style;
	psWidget->x = psInit->x;
	psWidget->y = psInit->y;
	psWidget->width = psInit->width;
	psWidget->height = psInit->height;

	if (psInit->pDisplay)
	{
		psWidget->display = psInit->pDisplay;
	}
	else
	{
		psWidget->display = labelDisplay;
	}
	psWidget->callback = psInit->pCallback;
	psWidget->pUserData = psInit->pUserData;
	psWidget->UserData = psInit->UserData;
	psWidget->FontID = psInit->FontID;

	if (psInit->pText)
	{
		sstrcpy(psWidget->aText, psInit->pText);
	}
	else
	{
		psWidget->aText[0] = 0;
	}

	return psWidget;
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
	enum iV_fonts FontID;

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
