/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * IntelMap.c		(Intelligence Map)
 *
 * Functions for the display of the Intelligence Map
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"
#include "lib/widget/button.h"
/* Includes direct access to render library */
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/piemode.h"

#include "display3d.h"
#include "lib/framework/cursors.h"
#include "map.h"
#include "intdisplay.h"
#include "objects.h"
#include "display.h"
#include "design.h"
#include "message.h"
#include "hci.h"
#include "intelmap.h"
#include "mapdisplay.h"
#include "lib/sound/audio.h"
#include "console.h"
#include "research.h"
#include "lib/gamelib/gtime.h"
#include "loop.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "warzoneconfig.h"
#include "seqdisp.h"
#include "mission.h"

#include "multiplay.h"
#include "lib/sound/cdaudio.h"
#include "lib/sequence/sequence.h"
#include "lib/sound/track.h"

#include "scriptextern.h"

#include "multimenu.h"

//#define NO_VIDEO

/* Intelligence Map screen IDs */
#define IDINTMAP_MSGFORM		6001	//The intelligence map tabbed form
#define IDINTMAP_CLOSE			6004	//The close button icon for the 3D view
#define	IDINTMAP_PAUSELABEL		6005	//The paused message
#define	IDINITMAP_PIEVIEW		6007	//The PIE view part of MSGVIEW
#define IDINTMAP_FLICVIEW		6008	//The Flic View part of MSGVIEW
#define IDINTMAP_TEXTVIEW		6009	//The Text area of MSGVIEW
#define	IDINTMAP_TITLELABEL		6010	//The title text
#define IDINTMAP_SEQTEXT		6011	//Sequence subtitle text

#define IDINTMAP_MSGSTART		6100	//The first button on the intelligence form
#define	IDINTMAP_MSGEND			6139	//The last button on the intelligence form (40 MAX)

#define IDINTMAP_SEQTEXTSTART		6200	//Sequence subtitle text tabs

/* Intelligence Map screen positions */
#define INTMAP_X				OBJ_BACKX
#define INTMAP_Y				OBJ_BACKY
#define INTMAP_WIDTH			OBJ_BACKWIDTH
#define INTMAP_HEIGHT			OBJ_BACKHEIGHT

#define INTMAP_LABELX			RET_X
#define INTMAP_LABELY			10
#define INTMAP_LABELWIDTH		60
#define INTMAP_LABELHEIGHT		20

/*tabbed message form screen positions */
#define INTMAP_MSGX				OBJ_TABX
#define INTMAP_MSGY				OBJ_TABY
#define INTMAP_MSGWIDTH			OBJ_WIDTH
#define INTMAP_MSGHEIGHT		OBJ_HEIGHT

//define the 3D View sizes and positions that are required - relative to INTMAP_FORM
#define	INTMAP_RESEARCHX		(100 + D_W)
#define INTMAP_RESEARCHY		(30	+ D_H)

#define	INTMAP_RESEARCHWIDTH	440
#define INTMAP_RESEARCHHEIGHT	288

/*dimensions for Title view section relative to IDINTMAP_MSGVIEW*/
/*dimensions for PIE view section relative to IDINTMAP_MSGVIEW*/

#define	INTMAP_TITLEX			0
#define INTMAP_TITLEY			0
#define	INTMAP_TITLEWIDTH		INTMAP_RESEARCHWIDTH
#define INTMAP_TITLEHEIGHT		18
#define	INTMAP_PIEX				3
#define INTMAP_PIEY				24

/*dimensions for FLIC view section relative to IDINTMAP_MSGVIEW*/
#define	INTMAP_FLICX			245
#define INTMAP_FLICY			24
#define	INTMAP_FLICWIDTH		192
#define INTMAP_FLICHEIGHT		170
/*dimensions for TEXT view section relative to IDINTMAP_MSGVIEW*/

#define	INTMAP_TEXTX			0
#define INTMAP_TEXTY			200
#define	INTMAP_TEXTWIDTH		INTMAP_RESEARCHWIDTH
#define INTMAP_TEXTHEIGHT		88
#define TEXT_XINDENT				5
#define TEXT_YINDENT				5

/*dimensions for SEQTEXT view relative to IDINTMAP_MSGVIEW*/
#define INTMAP_SEQTEXTX			0
#define INTMAP_SEQTEXTY			0
#define INTMAP_SEQTEXTWIDTH		INTMAP_RESEARCHWIDTH
#define INTMAP_SEQTEXTHEIGHT		INTMAP_RESEARCHHEIGHT

/*dimensions for SEQTEXT tab view relative to IDINTMAP_SEQTEXT*/
#define INTMAP_SEQTEXTTABX		0
#define INTMAP_SEQTEXTTABY		0
#define INTMAP_SEQTEXTTABWIDTH		INTMAP_SEQTEXTWIDTH
#define INTMAP_SEQTEXTTABHEIGHT		INTMAP_SEQTEXTHEIGHT

//position for text on full screen video
#define VIDEO_TEXT_TOP_X				20
#define VIDEO_TEXT_TOP_Y				20
#define VIDEO_TEXT_BOTTOM_X				20
#define VIDEO_TEXT_BOTTOM_Y				444

#define TEXT_START_FRAME	0
#define TEXT_END_FRAME			9999

/* the widget screen */
extern W_SCREEN		*psWScreen;

static UDWORD			messageID;
static bool				immediateMessage = false;

//flags whether to open the Intel Screen with a message
static bool				playCurrent;

/* functions declarations ****************/
static bool intAddMessageForm(bool playCurrent);
/*Displays the buttons used on the intelligence map */
static void intDisplayMessageButton(WIDGET *psWidget, UDWORD xOffset,
							  UDWORD yOffset, PIELIGHT *pColours);

/*deal with the actual button press - proxMsg is set to true if a proximity
  button has been pressed*/
static void intIntelButtonPressed(bool proxMsg, UDWORD id);

static void intDisplayPIEView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
#ifndef NO_VIDEO
static void intDisplayFLICView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
#endif
static void intDisplayTEXTView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void addVideoText(SEQ_DISPLAY *psSeqDisplay, UDWORD sequence);

static void intDisplaySeqTextView(WIDGET *psWidget,
				  UDWORD xOffset, UDWORD yOffset,
				  PIELIGHT *pColours);
static bool intDisplaySeqTextViewPage(VIEW_REPLAY *psViewReplay,
				      UDWORD x0, UDWORD y0,
				      UDWORD width, UDWORD height,
				      bool render,
				      size_t *major, size_t *minor);


/*********************** VARIABLES ****************************/
// The current message being displayed
MESSAGE			*psCurrentMsg = NULL;

#define PAUSE_DISPLAY_CONDITION (!bMultiPlayer)
#define PAUSEMESSAGE_YOFFSET (0)


/* Add the Intelligence Map widgets to the widget screen */
bool intAddIntelMap(void)
{
	bool			Animate = true;

	//check playCurrent with psCurrentMsg
	if (psCurrentMsg == NULL)
	{
		playCurrent = false;
	}
	else
	{
		playCurrent = true;
	}

	// Is the form already up?
	if(widgGetFromID(psWScreen,IDINTMAP_FORM) != NULL)
	{
		intRemoveIntelMapNoAnim();
		Animate = false;
	}
	else
	{
		audio_StopAll();
	}

	cdAudio_Pause();

	//add message to indicate game is paused - single player mode
	if(PAUSE_DISPLAY_CONDITION)
	{
		if(widgGetFromID(psWScreen,IDINTMAP_PAUSELABEL) == NULL)
		{
			W_LABINIT sLabInit;
			sLabInit.id = IDINTMAP_PAUSELABEL;
			sLabInit.formID = 0;
			sLabInit.x = INTMAP_LABELX;
			sLabInit.y = INTMAP_LABELY+PAUSEMESSAGE_YOFFSET;
			sLabInit.width = INTMAP_LABELWIDTH;
			sLabInit.height = INTMAP_LABELHEIGHT;
			sLabInit.pText = _("PAUSED");
			if (!widgAddLabel(psWScreen, &sLabInit))
			{
				return false;
			}
		}
	}

	//set pause states before putting the interface up
	setIntelligencePauseState();

	W_FORMINIT sFormInit;

	// Add the main Intelligence Map form

	sFormInit.formID = 0;
	sFormInit.id = IDINTMAP_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)INTMAP_X;
	sFormInit.y = (SWORD)INTMAP_Y;
	sFormInit.width = INTMAP_WIDTH;
	sFormInit.height = INTMAP_HEIGHT;

	// If the window was closed then do open animation.
	if(Animate)
	{
		sFormInit.pDisplay = intOpenPlainForm;
		sFormInit.disableChildren = true;
	}
	else
	{
		// otherwise just recreate it.
		sFormInit.pDisplay = intDisplayPlainForm;
	}

	//sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	if (!intAddMessageForm(playCurrent))
	{
		return false;
	}

	if (bMultiPlayer && !MultiMenuUp && !playCurrent)
	{
		intAddMultiMenu();
	}

	return true;
}

/* Add the Message sub form */
static bool intAddMessageForm(bool playCurrent)
{
	UDWORD			numButtons, i;
	MESSAGE			*psMessage;
	RESEARCH		*psResearch;
	SDWORD			BufferID;

	/* Add the Message form */
	W_FORMINIT sFormInit;
	sFormInit.formID = IDINTMAP_FORM;
	sFormInit.id = IDINTMAP_MSGFORM;
	sFormInit.style = WFORM_TABBED;
	sFormInit.width = INTMAP_MSGWIDTH;
	sFormInit.height = INTMAP_MSGHEIGHT;
	sFormInit.x = INTMAP_MSGX;
	sFormInit.y = INTMAP_MSGY;

	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = OBJ_TABWIDTH;
	sFormInit.majorOffset = OBJ_TABOFFSET;
	sFormInit.tabVertOffset = (OBJ_TABHEIGHT/2);
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;

	numButtons = 0;
	/*work out the number of buttons */
	for(psMessage = apsMessages[selectedPlayer]; psMessage; psMessage =
		psMessage->psNext)
	{
		//ignore proximity messages here
		if (psMessage->type != MSG_PROXIMITY)
		{
			numButtons++;
		}

		// stop adding the buttons once max has been reached
		if (numButtons > (IDINTMAP_MSGEND - IDINTMAP_MSGSTART))
		{
			break;
		}
	}

	//set the number of tabs required
	sFormInit.numMajor = numForms((OBJ_BUTWIDTH + OBJ_GAP) * numButtons,
								  (OBJ_WIDTH - OBJ_GAP)*2);

	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;

	if (sFormInit.numMajor > MAX_TAB_STD_SHOWN)
	{	// we do NOT use smallTab icons here, so be safe and only display max # of
		// standard sized tab icons.
		sFormInit.numMajor = MAX_TAB_STD_SHOWN;
	}
	//set minor tabs to 1
	for (i=0; i< sFormInit.numMajor; i++)
	{
		sFormInit.aNumMinors[i] = 1;
	}

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}


	/* Add the message buttons */
	W_FORMINIT sBFormInit;
	sBFormInit.formID = IDINTMAP_MSGFORM;
	sBFormInit.id = IDINTMAP_MSGSTART;
	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	sBFormInit.x = OBJ_STARTX;
	sBFormInit.y = OBJ_STATSTARTY;
	sBFormInit.width = OBJ_BUTWIDTH;
	sBFormInit.height = OBJ_BUTHEIGHT;

	ClearObjectBuffers();

	//add each button
	messageID = 0;
	for(psMessage = apsMessages[selectedPlayer]; psMessage; psMessage =
		psMessage->psNext)
	{
		/*if (psMessage->type == MSG_TUTORIAL)
		{
			//tutorial cases should never happen
			ASSERT( false, "Tutorial message in Intelligence screen!" );
			continue;
		}*/
		if (psMessage->type == MSG_PROXIMITY)
		{
			//ignore proximity messages here
			continue;
		}

		/* Set the tip and add the button */
		switch (psMessage->type)
		{
			case MSG_RESEARCH:
				psResearch =  getResearchForMsg((VIEWDATA *)psMessage->pViewData);
				if (psResearch)
				{
					sBFormInit.pTip = getStatName(psResearch);;
				}
				else
				{
					sBFormInit.pTip = _("Research Update");
				}
				break;
			case MSG_CAMPAIGN:
				sBFormInit.pTip = _("Project Goals");
				break;
			case MSG_MISSION:
				sBFormInit.pTip = _("Current Objective");
				break;
			default:
				break;
		}

		BufferID = GetObjectBuffer();
		ASSERT( BufferID >= 0,"Unable to acquire object buffer." );
		RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
		ObjectBuffers[BufferID].Data = (void*)psMessage;
		sBFormInit.pUserData = &ObjectBuffers[BufferID];
		sBFormInit.pDisplay = intDisplayMessageButton;

		if (!widgAddForm(psWScreen, &sBFormInit))
		{
			return false;
		}

		/* if the current message matches psSelected lock the button */
		if (psMessage == psCurrentMsg)
		{
			messageID = sBFormInit.id;
			widgSetButtonState(psWScreen, messageID, WBUT_LOCK);
			widgSetTabs(psWScreen, IDINTMAP_MSGFORM, sBFormInit.majorID, 0);
		}

		/* Update the init struct for the next button */
		sBFormInit.id += 1;

		// stop adding the buttons when at max
		if (sBFormInit.id > IDINTMAP_MSGEND)
		{
			break;
		}

		ASSERT( sBFormInit.id < (IDINTMAP_MSGEND+1),"Too many message buttons" );

		sBFormInit.x += OBJ_BUTWIDTH + OBJ_GAP;
		if (sBFormInit.x + OBJ_BUTWIDTH + OBJ_GAP > INTMAP_MSGWIDTH)
		{
			sBFormInit.x = OBJ_STARTX;
			sBFormInit.y += OBJ_BUTHEIGHT + OBJ_GAP;
		}

		if (sBFormInit.y + OBJ_BUTHEIGHT + OBJ_GAP > INTMAP_MSGHEIGHT)
		{
			sBFormInit.y = OBJ_STATSTARTY;
			sBFormInit.majorID += 1;
		}
	}
	//check to play current message instantly
	if (playCurrent)
	{
		//is it a proximity message?
		if (psCurrentMsg->type == MSG_PROXIMITY)
		{
			//intIntelButtonPressed(true, messageID);
		}
		else
		{
			intIntelButtonPressed(false, messageID);
		}
	}
	return true;
}

/*Add the 3D world view for the particular message (only research nmessages now) */
bool intAddMessageView(MESSAGE * psMessage)
{
	bool			Animate = true;
	RESEARCH		*psResearch;

	// Is the form already up?
	if(widgGetFromID(psWScreen,IDINTMAP_MSGVIEW) != NULL)
	{
		intRemoveMessageView(false);
		Animate = false;
	}
	if (MultiMenuUp)
	{
		intCloseMultiMenuNoAnim();
	}

	/* Add the base form */
	W_FORMINIT sFormInit;
	sFormInit.formID = 0;
	sFormInit.id = IDINTMAP_MSGVIEW;
	sFormInit.style = WFORM_PLAIN;
	//size and position depends on the type of message - ONLY RESEARCH now
	sFormInit.width = INTMAP_RESEARCHWIDTH;
	sFormInit.height = INTMAP_RESEARCHHEIGHT;
	sFormInit.x = (SWORD)INTMAP_RESEARCHX;
	sFormInit.y = (SWORD)INTMAP_RESEARCHY;

	// If the window was closed then do open animation.
	if(Animate)
	{
		sFormInit.pDisplay = intOpenPlainForm;
		sFormInit.disableChildren = true;
	}
	else
	{
		// otherwise just display it.
		sFormInit.pDisplay = intDisplayPlainForm;
	}

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the close box */
	W_BUTINIT sButInit;
	sButInit.formID = IDINTMAP_MSGVIEW;
	sButInit.id = IDINTMAP_CLOSE;
	sButInit.x = (SWORD)(sFormInit.width - OPT_GAP - CLOSE_SIZE);
	sButInit.y = OPT_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	if (psMessage->type != MSG_RESEARCH &&
	    ((VIEWDATA*)psMessage->pViewData)->type == VIEW_RPL)
	{
		VIEW_REPLAY	*psViewReplay;
		size_t		i, cur_seq, cur_seqpage;

		psViewReplay = (VIEW_REPLAY *)((VIEWDATA *)psMessage->pViewData)->pData;

		/* Add a big tabbed text box for the subtitle text */
		sFormInit = W_FORMINIT();

		sFormInit.id = IDINTMAP_SEQTEXT;
		sFormInit.formID = IDINTMAP_MSGVIEW;
		sFormInit.style = WFORM_TABBED;
		sFormInit.x = INTMAP_SEQTEXTX;
		sFormInit.y = INTMAP_SEQTEXTY;
		sFormInit.width = INTMAP_SEQTEXTWIDTH;
		sFormInit.height = INTMAP_SEQTEXTHEIGHT;

		sFormInit.majorPos = WFORM_TABBOTTOM;
		sFormInit.minorPos = WFORM_TABNONE;
		sFormInit.majorSize = OBJ_TABWIDTH;
		sFormInit.majorOffset = OBJ_TABOFFSET;
		sFormInit.tabVertOffset = (OBJ_TABHEIGHT/2);
		sFormInit.tabMajorThickness = OBJ_TABHEIGHT;

		sFormInit.numMajor = 0;

		cur_seq = cur_seqpage = 0;
		do {
			sFormInit.aNumMinors[sFormInit.numMajor] = 1;
			sFormInit.numMajor++;
		}
		while (!intDisplaySeqTextViewPage(psViewReplay, 0, 0,
						  sFormInit.width, sFormInit.height,
						  false, &cur_seq, &cur_seqpage));

		sFormInit.pUserData = &StandardTab;
		sFormInit.pTabDisplay = intDisplayTab;

		if (!widgAddForm(psWScreen, &sFormInit))
		{
			return false;
		}

		W_FORMINIT sTabForm;
		sTabForm.formID = IDINTMAP_SEQTEXT;
		sTabForm.id = IDINTMAP_SEQTEXTSTART;
		sTabForm.majorID = 0;
		sTabForm.minorID = 0;
		sTabForm.style = WFORM_PLAIN;
		sTabForm.x = INTMAP_SEQTEXTTABX;
		sTabForm.y = INTMAP_SEQTEXTTABY;
		sTabForm.width = INTMAP_SEQTEXTTABWIDTH;
		sTabForm.height = INTMAP_SEQTEXTTABHEIGHT;
		sTabForm.pDisplay = intDisplaySeqTextView;
		sTabForm.pUserData = psViewReplay;

		for (i = 0; i < sFormInit.numMajor; i++)
		{
			sTabForm.id = IDINTMAP_SEQTEXTSTART + i;
			sTabForm.majorID = i;
			if (!widgAddForm(psWScreen, &sTabForm))
			{
				return false;
			}
		}

		return true;
	}

	/*add the Label for the title box*/
	W_LABINIT sLabInit;
	sLabInit.id = IDINTMAP_TITLELABEL;
	sLabInit.formID = IDINTMAP_MSGVIEW;
	sLabInit.x = INTMAP_TITLEX + TEXT_XINDENT;
	sLabInit.y = INTMAP_TITLEY + TEXT_YINDENT;
	sLabInit.width = INTMAP_TITLEWIDTH;
	sLabInit.height = INTMAP_TITLEHEIGHT;
	//print research name in title bar

	ASSERT( psMessage->type != MSG_PROXIMITY,
		"intAddMessageView:Invalid message type for research" );

	psResearch = getResearchForMsg((VIEWDATA *)psMessage->pViewData);

	ASSERT( psResearch!=NULL,"Research not found" );
	//sLabInit.pText=psResearch->pName;
	sLabInit.pText = getStatName(psResearch);

	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	/*Add the PIE box*/

	sFormInit = W_FORMINIT();
	sFormInit.formID = IDINTMAP_MSGVIEW;
	sFormInit.id = IDINITMAP_PIEVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = INTMAP_PIEX;
	sFormInit.y = INTMAP_PIEY;
	sFormInit.width = INTMAP_PIEWIDTH;
	sFormInit.height = INTMAP_PIEHEIGHT;
	sFormInit.pDisplay = intDisplayPIEView;
	sFormInit.pUserData = psMessage;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

#ifndef NO_VIDEO
	/*Add the Flic box */
	sFormInit = W_FORMINIT();
	sFormInit.formID = IDINTMAP_MSGVIEW;
	sFormInit.id = IDINTMAP_FLICVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = INTMAP_FLICX;
	sFormInit.y = INTMAP_FLICY;
	sFormInit.width = INTMAP_FLICWIDTH;
	sFormInit.height = INTMAP_FLICHEIGHT;
	sFormInit.pDisplay = intDisplayFLICView;
	sFormInit.pUserData = psMessage;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}
#endif

	/*Add the text box*/
	sFormInit = W_FORMINIT();

	sFormInit.formID = IDINTMAP_MSGVIEW;

	sFormInit.id = IDINTMAP_TEXTVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = INTMAP_TEXTX;
	sFormInit.y = INTMAP_TEXTY;
	sFormInit.width = INTMAP_TEXTWIDTH;
	sFormInit.height = INTMAP_TEXTHEIGHT;
	sFormInit.pDisplay = intDisplayTEXTView;
	sFormInit.pUserData = psMessage;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	return true;
}

/* Process return codes from the Intelligence Map */
void intProcessIntelMap(UDWORD id)
{

	if (id >= IDINTMAP_MSGSTART && id <= IDINTMAP_MSGEND)
	{
		intIntelButtonPressed(false, id);
	}
	else if (id == IDINTMAP_CLOSE)
	{
		//if close button pressed on 3D View then close the view only
		psCurrentMsg = NULL;
		intRemoveMessageView(true);
		if (bMultiPlayer && !MultiMenuUp)
		{
			intAddMultiMenu();
		}
	}
	else if (MultiMenuUp)
	{
		intProcessMultiMenu(id);
	}
}

/**
 * Draws the text for the intelligence display window.
 */
static bool intDisplaySeqTextViewPage(VIEW_REPLAY *psViewReplay,
				      UDWORD x0, UDWORD y0,
				      UDWORD width, UDWORD height,
				      bool render,
				      size_t *cur_seq, size_t *cur_seqpage)
{
	UDWORD i, cur_y;
	UDWORD sequence;

	if (!psViewReplay)
	{
		return true;	/* nothing to do */
	}

	iV_SetFont(font_regular);

	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	cur_y = y0 + iV_GetTextLineSize()/2 + 2*TEXT_YINDENT;

	/* add each message */
	for (sequence = *cur_seq, i = *cur_seqpage; sequence < psViewReplay->numSeq; sequence++)
	{
		SEQ_DISPLAY *psSeqDisplay = &psViewReplay->pSeqList[sequence];
		for (; i < psSeqDisplay->textMsg.size(); i++)
		{
			if (render)
			{
				cur_y = iV_DrawFormattedText(psSeqDisplay->textMsg[i].toUtf8().constData(),
					    x0 + TEXT_XINDENT,
					    cur_y, width, false);
			}
			else
			{
				cur_y += iV_GetTextLineSize();
			}
			if (cur_y > y0 + height)
			{
				/* run out of room - need to make new tab */
				*cur_seq = sequence;
				*cur_seqpage = i;
				return false;
			}
		}
		i = 0;
	}

	return true;		/* done */
}

/**
 * Draw the text window for the intelligence display
 */
static void intDisplaySeqTextView(WIDGET *psWidget,
				  UDWORD xOffset, UDWORD yOffset,
				  WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	VIEW_REPLAY *psViewReplay = (VIEW_REPLAY*)Form->pUserData;
	size_t cur_seq, cur_seqpage;
	UDWORD x0, y0, page;

	x0 = xOffset + Form->x;
	y0 = yOffset + Form->y;

	RenderWindowFrame(FRAME_NORMAL, x0, y0, Form->width, Form->height);

	/* work out where we're up to in the text */
	cur_seq = cur_seqpage = 0;
	if(Form->style & WFORM_TABBED)
	{
		// Gerard 2007-04-07: dead code?
		ASSERT(!"the form is tabbed", "intDisplaySeqTextView: the form is tabbed");
		for (page = 0; page < Form->majorT; page++)
		{
			intDisplaySeqTextViewPage(psViewReplay, x0, y0,
						  Form->width, Form->height,
						  false, &cur_seq, &cur_seqpage);
		}
	}

	intDisplaySeqTextViewPage(psViewReplay, x0, y0,
				  Form->width-40, Form->height,
				  true, &cur_seq, &cur_seqpage);
}


// Add all the Video Sequences for a message
static void StartMessageSequences(MESSAGE *psMessage, bool Start)
{
	bool bLoop = false;

	debug(LOG_GUI, "StartMessageSequences: start message sequence");

	//should never have a proximity message here
	if (psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	ASSERT( psMessage->pViewData != NULL,
		"StartMessageSequences: invalid ViewData pointer" );

	if (((VIEWDATA *)psMessage->pViewData)->type == VIEW_RPL)
	{
		VIEW_REPLAY		*psViewReplay;
		UDWORD Sequence;

	// Surely we don't need to set up psCurrentMsg when we pass the message into this routine ... tim
		psViewReplay = (VIEW_REPLAY *)((VIEWDATA *)psMessage->pViewData)->pData;

		seq_ClearSeqList();

		//add any sequences to the list to be played when the first one is finished
		for (Sequence = 0; Sequence < psViewReplay->numSeq; Sequence++)

		{
			if (psViewReplay->pSeqList[Sequence].flag == 1)
			{
				bLoop = true;
			}
			else
			{
				bLoop = false;
			}


			seq_AddSeqToList(psViewReplay->pSeqList[Sequence].sequenceName, psViewReplay->pSeqList[Sequence].pAudio, NULL, bLoop);

			debug(LOG_GUI, "StartMessageSequences: sequence=%d", Sequence);
			addVideoText(&psViewReplay->pSeqList[Sequence],Sequence);
		}
		//play first full screen video
		if (Start==true)
		{
		 	seq_StartNextFullScreenVideo();
		}
	}

	else if (((VIEWDATA *)psMessage->pViewData)->type == VIEW_RES)
	{
		VIEW_RESEARCH		*psViewReplay;
		//UDWORD Sequence;

		psViewReplay = (VIEW_RESEARCH *)((VIEWDATA *)psCurrentMsg->pViewData)->pData;

		seq_ClearSeqList();
		seq_AddSeqToList(psViewReplay->sequenceName, psViewReplay->pAudio, NULL, false);
		//play first full screen video
		if (Start==true)
		{
			seq_StartNextFullScreenVideo();
		}
	}

}

/*
deal with the actual button press - proxMsg is set to true if a proximity
button has been pressed
*/
void intIntelButtonPressed(bool proxMsg, UDWORD id)
{
	MESSAGE			*psMessage;
	UDWORD			currID;//, i;
	RESEARCH		*psResearch;

	ASSERT( proxMsg != true,
		"intIntelButtonPressed: Shouldn't be able to get a proximity message!" );

	if(id == 0)
	{
		intRemoveIntelMap();
		return;
	}

	/* message button has been pressed - clear the old button and messageView*/
	if (messageID != 0)
	{
		widgSetButtonState(psWScreen, messageID, 0);
		intRemoveMessageView(false);
		psCurrentMsg = NULL;
	}

	/* Lock the new button */
	// This means we can't click on the same movie button twice.
	widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
	messageID = id;

	//Find the message for the new button */
	currID = IDINTMAP_MSGSTART;
	for(psMessage = apsMessages[selectedPlayer]; psMessage; psMessage =
		psMessage->psNext)
	{
		if (psMessage->type != MSG_PROXIMITY)
		{
			if (currID == id)
			{
				break;
			}
			currID++;
		}
	}

	//deal with the message if one
	if (psMessage)
	{
		//set the current message
		psCurrentMsg = psMessage;

		//set the read flag
		psCurrentMsg->read = true;

		debug(LOG_GUI, "intIntelButtonPressed: Dealing with a new message type=%d",
		      psMessage->type);

		//should never have a proximity message
		if (psMessage->type == MSG_PROXIMITY)
		{
			return;
		}
		// If its a video sequence then play it anyway
		if (((VIEWDATA *)psMessage->pViewData)->type == VIEW_RPL)
		{

			if (psMessage->pViewData)
			{
				intAddMessageView(psMessage);
			}

			StartMessageSequences(psMessage,true);

		}
		else if (((VIEWDATA *)psMessage->pViewData)->type == VIEW_RES)
		{
			psResearch = getResearchForMsg((VIEWDATA *)psMessage->pViewData);
			if (psResearch != NULL)
			{
				static const float maxVolume = 1.f;
				static AUDIO_STREAM *playing = NULL;

				// only play the sample once, otherwise, they tend to overlap each other
				if (sound_isStreamPlaying(playing))
				{
					sound_StopStream(playing);
				}

				switch(psResearch->iconID)
				{
				case IMAGE_RES_DROIDTECH:
					playing = audio_PlayStream("sequenceaudio/res_droid.ogg", maxVolume, NULL, NULL);
						break;
				case IMAGE_RES_WEAPONTECH:
					playing = audio_PlayStream("sequenceaudio/res_weapons.ogg", maxVolume, NULL, NULL);
						break;
				case IMAGE_RES_COMPUTERTECH:
					playing = audio_PlayStream("sequenceaudio/res_com.ogg", maxVolume, NULL, NULL);
						break;
				case IMAGE_RES_POWERTECH:
					playing = audio_PlayStream("sequenceaudio/res_pow.ogg", maxVolume, NULL, NULL);
						break;
				case IMAGE_RES_SYSTEMTECH:
					playing = audio_PlayStream("sequenceaudio/res_systech.ogg", maxVolume, NULL, NULL);
						break;
				case IMAGE_RES_STRUCTURETECH:
					playing = audio_PlayStream("sequenceaudio/res_strutech.ogg", maxVolume, NULL, NULL);
						break;
				case IMAGE_RES_CYBORGTECH:
					playing = audio_PlayStream("sequenceaudio/res_droid.ogg", maxVolume, NULL, NULL);
						break;
				case IMAGE_RES_DEFENCE:
					playing = audio_PlayStream("sequenceaudio/res_strutech.ogg", maxVolume, NULL, NULL);
						break;
				}
			}

			//and finally for the dumb?
			if (psMessage->pViewData)
			{
				intAddMessageView(psMessage);
			}
		}
	}
}


static void intCleanUpIntelMap(void)
{
	MESSAGE		*psMessage, *psNext;

	//remove any research messages that have been read
	for (psMessage = apsMessages[selectedPlayer]; psMessage != NULL; psMessage =
		psNext)
	{
		psNext = psMessage->psNext;
		if (psMessage->type == MSG_RESEARCH && psMessage->read)
		{
			removeMessage(psMessage, selectedPlayer);
		}
	}
	resetIntelligencePauseState();
	immediateMessage = false;

	cdAudio_Resume();

	if (interpProcessorActive())
	{
		debug(LOG_SCRIPT, "intCleanUpIntelMap: interpreter running, storing CALL_VIDEO_QUIT");
		if(!msgStackPush(CALL_VIDEO_QUIT,-1,-1,"\0",-1,-1,NULL))
		{
			debug(LOG_ERROR, "intCleanUpIntelMap() - msgStackPush - stack failed");
			return;
		}
	}
	else
	{
		debug(LOG_SCRIPT, "intCleanUpIntelMap: not running");
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
	}
}


/* Remove the Intelligence Map widgets from the screen */
void intRemoveIntelMap(void)
{
	WIDGET		*Widg;
	W_TABFORM	*Form;

	//remove 3dView if still there
	Widg = widgGetFromID(psWScreen,IDINTMAP_MSGVIEW);
	if(Widg)
	{
		intRemoveMessageView(false);
	}

	// Start the window close animation.
	Form = (W_TABFORM*)widgGetFromID(psWScreen,IDINTMAP_FORM);
	if(Form)
	{
		Form->display = intClosePlainForm;
		Form->disableChildren = true;
		Form->pUserData = NULL; // Used to signal when the close anim has finished.
	}
	ClosingIntelMap = true;
	//remove the text label
	widgDelete(psWScreen, IDINTMAP_PAUSELABEL);

	if (bMultiPlayer && MultiMenuUp)
	{
		intCloseMultiMenu();
	}
	
	intCleanUpIntelMap();
}

/* Remove the Intelligence Map widgets from the screen */
void intRemoveIntelMapNoAnim(void)
{
	WIDGET *Widg;

	//remove 3dView if still there
	Widg = widgGetFromID(psWScreen,IDINTMAP_MSGVIEW);
	if(Widg)
	{
		intRemoveMessageView(false);
	}
	//remove main Intelligence screen
	widgDelete(psWScreen, IDINTMAP_FORM);
	//remove the text label
	widgDelete(psWScreen, IDINTMAP_PAUSELABEL);

	if (bMultiPlayer && MultiMenuUp)
	{
		intCloseMultiMenuNoAnim();
	}
	
	intCleanUpIntelMap();
}

/* Remove the Message View from the Intelligence screen */
void intRemoveMessageView(bool animated)
{
	W_TABFORM		*Form;
	VIEW_RESEARCH	*psViewResearch;

	//remove 3dView if still there
	Form = (W_TABFORM*)widgGetFromID(psWScreen,IDINTMAP_MSGVIEW);
	if(Form)
	{

		//stop the video
		psViewResearch = (VIEW_RESEARCH *)Form->pUserData;
		seq_RenderVideoToBuffer(psViewResearch->sequenceName, SEQUENCE_KILL);

		if (animated)
		{

			widgDelete(psWScreen, IDINTMAP_CLOSE);


			// Start the window close animation.
			Form->display = intClosePlainForm;
			Form->disableChildren = true;
			Form->pUserData = NULL; // Used to signal when the close anim has finished.
			ClosingMessageView = true;
		}
		else
		{
			//remove without the animating close window
			widgDelete(psWScreen, IDINTMAP_MSGVIEW);

		}
	}
}


/*Displays the buttons used on the intelligence map */
void intDisplayMessageButton(WIDGET *psWidget, UDWORD xOffset,
							  UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_CLICKFORM		*psButton = (W_CLICKFORM*)psWidget;
	RENDERED_BUTTON *psBuffer = (RENDERED_BUTTON*)psButton->pUserData;
	MESSAGE			*psMsg;
	bool			Hilight = false;
	UDWORD			Down = 0, IMDType = 0, compID;
	SDWORD			image = -1;
	RESEARCH		*pResearch = NULL;
    BASE_STATS      *psResGraphic = NULL;
	bool MovieButton = false;

	OpenButtonRender((UWORD)(xOffset+psButton->x), (UWORD)(yOffset+psButton->y),
		psButton->width, psButton->height);

	Down = psButton->state & (WBUTS_DOWN | WBUTS_CLICKLOCK);
	Hilight = psButton->state & WBUTS_HILITE;

	// Get the object associated with this widget.
	psMsg = (MESSAGE *)psBuffer->Data;
	ASSERT_OR_RETURN( , psMsg != NULL, "psBuffer->Data empty. Why?" );
	//shouldn't have any proximity messages here...
	if (psMsg->type == MSG_PROXIMITY)
	{
		return;
	}
	//set the graphic for the button
	switch (psMsg->type)
	{
	case MSG_RESEARCH:
		pResearch = getResearchForMsg((VIEWDATA *)psMsg->pViewData);
		//IMDType = IMDTYPE_RESEARCH;
        	//set the IMDType depending on what stat is associated with the research
		if (pResearch && pResearch->psStat)
        	{
			//we have a Stat associated with this research topic
			if  (StatIsStructure(pResearch->psStat))
			{
				//this defines how the button is drawn
				IMDType = IMDTYPE_STRUCTURESTAT;
				psResGraphic = pResearch->psStat;
			}
			else
			{
				compID = StatIsComponent(pResearch->psStat);
				if (compID != COMP_UNKNOWN)
				{
					//this defines how the button is drawn
					IMDType = IMDTYPE_COMPONENT;
					psResGraphic = pResearch->psStat;
				}
				else
				{
					ASSERT( false, "intDisplayMessageButton: invalid stat" );
					IMDType = IMDTYPE_RESEARCH;
					psResGraphic = (BASE_STATS *)pResearch;
				}
			}
		}
		else
		{
			//no Stat for this research topic so use the research topic to define what is drawn
			psResGraphic = (BASE_STATS *)pResearch;
			IMDType = IMDTYPE_RESEARCH;
		}
		break;
	case MSG_CAMPAIGN:
		image = IMAGE_INTEL_CAMPAIGN;
		MovieButton = true;
		break;
	case MSG_MISSION:
		image = IMAGE_INTEL_MISSION;
		MovieButton = true;
		break;
	default:
		debug( LOG_ERROR, "Unknown message type: %i", psMsg->type );
		return;
	}

	//if research message
	if (pResearch)
	{
		if (pResearch->iconID != NO_RESEARCH_ICON)
		{
			image = pResearch->iconID;
		}

		//do we have the same icon for the top right hand corner?
		if (image > 0)
		{
			RenderToButton(IntImages, (UWORD)image, psResGraphic, selectedPlayer, psBuffer,Down, IMDType, TOPBUTTON);
		}
		else
		{
			RenderToButton(NULL,0,pResearch,selectedPlayer,psBuffer,Down,IMDType,TOPBUTTON); //ajl, changed from 0 to selectedPlayer
		}
	}
	else
	//draw buttons for mission and general messages
	{
		if (image > 0)
		{

			if(MovieButton) {
				// draw the button with the relevant image, don't add Down to the image ID if it's
				// a movie button.
				RenderImageToButton(IntImages,(UWORD)(image),psBuffer,Down,TOPBUTTON);
			} else {
				//draw the button with the relevant image
				RenderImageToButton(IntImages,(UWORD)(image+Down),psBuffer,Down,TOPBUTTON);
			}

		}
	}

	CloseButtonRender();

	if (Hilight)
	{

		iV_DrawImage(IntImages,IMAGE_BUT_HILITE,xOffset+psButton->x,
			yOffset+psButton->y);
	}
}


/* displays the PIE view for the current message */
void intDisplayPIEView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM		*Form = (W_TABFORM*)psWidget;
	MESSAGE			*psMessage = (MESSAGE *)Form->pUserData;
	UDWORD			x0,y0,x1,y1;
	SWORD			image = -1;
	RESEARCH        *psResearch;

	// Should not have any proximity messages here...
	if (!psMessage || psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	if (psMessage->pViewData)
	{
		x0 = xOffset+Form->x;
		y0 = yOffset+Form->y;
		x1 = x0 + Form->width;
		y1 = y0 + Form->height;

		//moved from after close render
		RenderWindowFrame(FRAME_NORMAL, x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2);

		OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),
			Form->width, Form->height);
		//OpenButtonRender(Form->x, Form->y,Form->width, Form->height);

		if (((VIEWDATA *)psMessage->pViewData)->type != VIEW_RES)
		{
			ASSERT( false, "intDisplayPIEView: Invalid message type" );
			return;
		}

		//render an object
		psResearch = getResearchForMsg((VIEWDATA *)psCurrentMsg->pViewData);
		renderResearchToBuffer(psResearch, x0+(x1-x0)/2, y0+(y1-y0)/2);

		CloseButtonRender();

		//draw image icon in top left of window
		image = (SWORD)getResearchForMsg((VIEWDATA *)psMessage->pViewData)->iconID;
		if (image > 0)
		{
			iV_DrawImage(IntImages,image,x0,y0);
		}
	}
}

#ifndef NO_VIDEO
/* displays the FLIC view for the current message */
void intDisplayFLICView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{

	W_TABFORM		*Form = (W_TABFORM*)psWidget;
	MESSAGE			*psMessage = (MESSAGE *)Form->pUserData;
	UDWORD			x0,y0,x1,y1;
	VIEW_RESEARCH	*psViewResearch;

	//shouldn't have any proximity messages here...
	if (!psMessage || psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	if (psMessage->pViewData)
	{
		OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),
			Form->width, Form->height);

		x0 = xOffset+Form->x;
		y0 = yOffset+Form->y;
		x1 = x0 + Form->width;
		y1 = y0 + Form->height;

		if (((VIEWDATA *)psMessage->pViewData)->type != VIEW_RES)
		{
			ASSERT( false, "intDisplayFLICView: Invalid message type" );
			return;
		}

		RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
		psViewResearch = (VIEW_RESEARCH *)((VIEWDATA *)psCurrentMsg->pViewData)->pData;
		// set the dimensions to window size & position
		seq_SetDisplaySize(192, 168, x0, y0);
		//render a frame of the current movie *must* force above resolution!
		seq_RenderVideoToBuffer(psViewResearch->sequenceName, SEQUENCE_HOLD);
		CloseButtonRender();
	}
}
#endif

/**
 * Displays the TEXT view for the current message.
 * If this function breaks, please merge it with intDisplaySeqTextViewPage
 * which presumably does almost the same.
 */
void intDisplayTEXTView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM		*Form = (W_TABFORM*)psWidget;
	MESSAGE			*psMessage = (MESSAGE *)Form->pUserData;
	UDWORD			x0, y0, x1, y1, i, linePitch;
	UDWORD			ty;

	x0 = xOffset+Form->x;
	y0 = yOffset+Form->y;
	x1 = x0 + Form->width;
	y1 = y0 + Form->height;
	ty = y0;

	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);

	if (psMessage)
	{
		iV_SetFont(font_regular);
		/* Get the travel to the next line */
		linePitch = iV_GetTextLineSize();
		/* Fix for spacing.... */
		linePitch+=3;
		ty+=3;
		/* Fix for spacing.... */


		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		//add each message
		for (i = 0; i < ((VIEWDATA *)psMessage->pViewData)->textMsg.size(); i++)
		{
			//check haven't run out of room first!
			if (i * linePitch > Form->height)
			{
				ASSERT( false, "intDisplayTEXTView: Run out of room!" );
				return;
			}
			//need to check the string will fit!
			iV_DrawText(_(((VIEWDATA *)psMessage->pViewData)->textMsg[i].toUtf8().constData()), x0 + TEXT_XINDENT,
				(ty + TEXT_YINDENT*3) + (i * linePitch));
		}
	}
}


//adds text to full screen video
void addVideoText(SEQ_DISPLAY *psSeqDisplay, UDWORD sequence)
{
	UDWORD	i, x, y;

	if (psSeqDisplay->textMsg.size() > 0)
	{
		//add each message, first at the top
		// FIXME We should perhaps get font size, and use that to calculate offset(s) ?
		x = VIDEO_TEXT_TOP_X;
		y = VIDEO_TEXT_TOP_Y;

		seq_AddTextForVideo(psSeqDisplay->textMsg[0].toUtf8().constData(), x, y, TEXT_START_FRAME, TEXT_END_FRAME, SEQ_TEXT_POSITION); //startframe endFrame

		//add each message, the rest at the bottom
		x = VIDEO_TEXT_BOTTOM_X;
		// calculate the real bottom... NOTE, game assumes all videos to be 640x480
		y = (double)pie_GetVideoBufferHeight() / 480. * (double)VIDEO_TEXT_BOTTOM_Y;
		i = 1;
		while (i < psSeqDisplay->textMsg.size())
		{
			seq_AddTextForVideo(psSeqDisplay->textMsg[i].toUtf8().constData(), x, y, TEXT_START_FRAME, TEXT_END_FRAME, SEQ_TEXT_POSITION); //startframe endFrame
			//initialise after the first setting
			x = y = 0;
			i++;
		}
	}
}

/*sets psCurrentMsg for the Intelligence screen*/
void setCurrentMsg(void)
{
	MESSAGE *psMsg, *psLastMsg;

	psLastMsg = NULL;
	for (psMsg = apsMessages[selectedPlayer]; psMsg != NULL; psMsg =
		psMsg->psNext)
	{
		if (psMsg->type != MSG_PROXIMITY)
		{
			psLastMsg = psMsg;
		}
	}
	psCurrentMsg = psLastMsg;
}

/*sets which states need to be paused when the intelligence screen is up*/
void setIntelligencePauseState(void)
{
	if (!bMultiPlayer)
	{
		//need to clear mission widgets from being shown on intel screen
		clearMissionWidgets();
		gameTimeStop();
		setGameUpdatePause(true);
		if(!bInTutorial)
		{	// Don't pause the scripts or the console if the tutorial is running.
			setScriptPause(true);
			setConsolePause(true);
		}
		setScrollPause(true);
		screen_RestartBackDrop();
	}
}

/*resets the pause states */
void resetIntelligencePauseState(void)
{
	if (!bMultiPlayer)
	{
		//put any widgets back on for the missions
		resetMissionWidgets();
		setGameUpdatePause(false);
		if(!bInTutorial)
		{
			setScriptPause(false);
		}
		setScrollPause(false);
		setConsolePause(false);
		gameTimeStart();
		screen_StopBackDrop();
		pie_ScreenFlip(CLEAR_BLACK);
	}
}

/** Play an intelligence message.
 * This function is used from scripts to give updates to the mission.
 */
void displayImmediateMessage(MESSAGE *psMessage)
{
	/*
		This has to be changed to support a script calling a message in the intellegence screen
	*/

	psCurrentMsg = psMessage;
	StartMessageSequences(psMessage,true);
	// remind the player that the message can be seen again from
	// the intelligence screen
	addConsoleMessage(_("New Intelligence Report"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
	flashReticuleButton(IDRET_INTEL_MAP);
}


// return whether a message is immediate
bool messageIsImmediate(void)
{
	return immediateMessage;
}

/*sets the flag*/
void setMessageImmediate(bool state)
{
	immediateMessage = state;
}
