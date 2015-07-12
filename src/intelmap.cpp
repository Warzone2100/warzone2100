/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

/* Intelligence Map screen IDs */
#define IDINTMAP_MSGFORM		6001	//The intelligence map tabbed form
#define IDINTMAP_CLOSE			6004	//The close button icon for the 3D view
#define	IDINTMAP_PAUSELABEL		6005	//The paused message
#define	IDINITMAP_PIEVIEW		6007	//The PIE view part of MSGVIEW
#define IDINTMAP_FLICVIEW		6008	//The Flic View part of MSGVIEW
#define IDINTMAP_TEXTVIEW		6009	//The Text area of MSGVIEW
#define	IDINTMAP_TITLELABEL		6010	//The title text

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
#define INTMAP_MSGY				OBJ_TABY
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
class IntMessageButton : public IntFancyButton
{
public:
	IntMessageButton(WIDGET *parent);

	virtual void display(int xOffset, int yOffset);

	void setMessage(MESSAGE *msg)
	{
		psMsg = msg;
	}

protected:
	MESSAGE *psMsg;
};

/*deal with the actual button press - proxMsg is set to true if a proximity
  button has been pressed*/
static void intIntelButtonPressed(bool proxMsg, UDWORD id);

static void intDisplayPIEView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void intDisplayFLICView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void intDisplayTEXTView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void addVideoText(SEQ_DISPLAY *psSeqDisplay, UDWORD sequence);

static void intDisplaySeqTextView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
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
	if (widgGetFromID(psWScreen, IDINTMAP_FORM) != NULL)
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
	if (PAUSE_DISPLAY_CONDITION)
	{
		if (widgGetFromID(psWScreen, IDINTMAP_PAUSELABEL) == NULL)
		{
			W_LABINIT sLabInit;
			sLabInit.id = IDINTMAP_PAUSELABEL;
			sLabInit.formID = 0;
			sLabInit.x = INTMAP_LABELX;
			sLabInit.y = INTMAP_LABELY + PAUSEMESSAGE_YOFFSET;
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

	WIDGET *parent = psWScreen->psForm;

	// Add the main Intelligence Map form
	IntFormAnimated *intMapForm = new IntFormAnimated(parent, Animate);  // Do not animate the opening, if the window was already open.
	intMapForm->id = IDINTMAP_FORM;
	intMapForm->setGeometry(INTMAP_X, INTMAP_Y, INTMAP_WIDTH, INTMAP_HEIGHT);

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
	WIDGET *msgForm = widgGetFromID(psWScreen, IDINTMAP_FORM);

	/* Add the Message form */
	IntListTabWidget *msgList = new IntListTabWidget(msgForm);
	msgList->id = IDINTMAP_MSGFORM;
	msgList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
	msgList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int msgListWidth = OBJ_BUTWIDTH * 5 + OBJ_GAP * 4;
	msgList->setGeometry((msgForm->width() - msgListWidth) / 2, INTMAP_MSGY, msgListWidth, msgForm->height() - INTMAP_MSGY);

	/* Add the message buttons */
	int nextButtonId = IDINTMAP_MSGSTART;

	//add each button
	messageID = 0;
	for (MESSAGE *psMessage = apsMessages[selectedPlayer]; psMessage != nullptr; psMessage = psMessage->psNext)
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

		IntMessageButton *button = new IntMessageButton(msgList);
		button->id = nextButtonId;
		button->setMessage(psMessage);
		msgList->addWidgetToLayout(button);

		/* Set the tip and add the button */
		RESEARCH *psResearch;
		switch (psMessage->type)
		{
		case MSG_RESEARCH:
			psResearch = getResearchForMsg((VIEWDATA *)psMessage->pViewData);
			if (psResearch)
			{
				button->setTip(psResearch->name);
			}
			else
			{
				button->setTip(_("Research Update"));
			}
			break;
		case MSG_CAMPAIGN:
			button->setTip(_("Project Goals"));
			break;
		case MSG_MISSION:
			button->setTip(_("Current Objective"));
			break;
		default:
			break;
		}

		/* if the current message matches psSelected lock the button */
		if (psMessage == psCurrentMsg)
		{
			messageID = nextButtonId;
			button->setState(WBUT_LOCK);
			msgList->setCurrentPage(msgList->pages() - 1);
		}

		/* Update the init struct for the next button */
		++nextButtonId;

		// stop adding the buttons when at max
		if (nextButtonId > IDINTMAP_MSGEND)
		{
			break;
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
bool intAddMessageView(MESSAGE *psMessage)
{
	bool			Animate = true;
	RESEARCH		*psResearch;

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDINTMAP_MSGVIEW) != NULL)
	{
		intRemoveMessageView(false);
		Animate = false;
	}
	if (MultiMenuUp)
	{
		intCloseMultiMenuNoAnim();
	}

	WIDGET *parent = psWScreen->psForm;

	IntFormAnimated *intMapMsgView = new IntFormAnimated(parent, Animate);  // Do not animate the opening, if the window was already open.
	intMapMsgView->id = IDINTMAP_MSGVIEW;
	intMapMsgView->setGeometry(INTMAP_RESEARCHX, INTMAP_RESEARCHY, INTMAP_RESEARCHWIDTH, INTMAP_RESEARCHHEIGHT);

	/* Add the close box */
	W_BUTINIT sButInit;
	sButInit.formID = IDINTMAP_MSGVIEW;
	sButInit.id = IDINTMAP_CLOSE;
	sButInit.x = intMapMsgView->width() - OPT_GAP - CLOSE_SIZE;
	sButInit.y = OPT_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	if (psMessage->type != MSG_RESEARCH &&
	    ((VIEWDATA *)psMessage->pViewData)->type == VIEW_RPL)
	{
		VIEW_REPLAY	*psViewReplay;

		psViewReplay = (VIEW_REPLAY *)((VIEWDATA *)psMessage->pViewData)->pData;

		/* Add a big tabbed text box for the subtitle text */
		IntListTabWidget *seqList = new IntListTabWidget(intMapMsgView);
		seqList->setChildSize(INTMAP_SEQTEXTTABWIDTH, INTMAP_SEQTEXTTABHEIGHT);
		seqList->setChildSpacing(2, 2);
		seqList->setGeometry(INTMAP_SEQTEXTX, INTMAP_SEQTEXTY, INTMAP_SEQTEXTWIDTH, INTMAP_SEQTEXTHEIGHT);
		seqList->setTabPosition(ListTabWidget::Bottom);
		// Don't think the tabs are actually ever used...

		size_t cur_seq = 0, cur_seqpage = 0;
		int nextPageId = IDINTMAP_SEQTEXTSTART;
		do
		{
			W_FORM *page = new W_FORM(seqList);
			page->id = nextPageId++;
			page->displayFunction = intDisplaySeqTextView;
			page->pUserData = psViewReplay;
			seqList->addWidgetToLayout(page);
		}
		while (!intDisplaySeqTextViewPage(psViewReplay, 0, 0, INTMAP_SEQTEXTTABWIDTH, INTMAP_SEQTEXTHEIGHT, false, &cur_seq, &cur_seqpage));

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

	ASSERT(psMessage->type != MSG_PROXIMITY,
	       "intAddMessageView:Invalid message type for research");

	psResearch = getResearchForMsg((VIEWDATA *)psMessage->pViewData);

	ASSERT(psResearch != NULL, "Research not found");
	//sLabInit.pText=psResearch->pName;
	sLabInit.pText = psResearch->name;

	sLabInit.FontID = font_regular;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	/*Add the PIE box*/
	W_FORMINIT sFormInit;
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

	cur_y = y0 + iV_GetTextLineSize() / 2 + 2 * TEXT_YINDENT;

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
static void intDisplaySeqTextView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	VIEW_REPLAY *psViewReplay = (VIEW_REPLAY *)psWidget->pUserData;
	size_t cur_seq, cur_seqpage;

	int x0 = xOffset + psWidget->x();
	int y0 = yOffset + psWidget->y();

	RenderWindowFrame(FRAME_NORMAL, x0, y0, psWidget->width(), psWidget->height());

	/* work out where we're up to in the text */
	cur_seq = cur_seqpage = 0;

	intDisplaySeqTextViewPage(psViewReplay, x0, y0, psWidget->width() - 40, psWidget->height(), true, &cur_seq, &cur_seqpage);
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

	ASSERT(psMessage->pViewData != NULL,
	       "StartMessageSequences: invalid ViewData pointer");

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
			addVideoText(&psViewReplay->pSeqList[Sequence], Sequence);
		}
		//play first full screen video
		if (Start == true)
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
		if (Start == true)
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
	UDWORD			currID;
	RESEARCH		*psResearch;

	ASSERT_OR_RETURN(, proxMsg != true, "Shouldn't be able to get a proximity message!");

	if (id == 0)
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
	for (psMessage = apsMessages[selectedPlayer]; psMessage; psMessage =
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

		if ((VIEWDATA *)psMessage->pViewData)
		{		// If its a video sequence then play it anyway
			if (((VIEWDATA *)psMessage->pViewData)->type == VIEW_RPL)
			{

				if (psMessage->pViewData)
				{
					intAddMessageView(psMessage);
				}

				StartMessageSequences(psMessage, true);

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

					switch (psResearch->iconID)
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
		if (!msgStackPush(CALL_VIDEO_QUIT, -1, -1, "\0", -1, -1, NULL))
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
	//remove 3dView if still there
	WIDGET *Widg = widgGetFromID(psWScreen, IDINTMAP_MSGVIEW);
	if (Widg)
	{
		intRemoveMessageView(false);
	}

	// Start the window close animation.
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDINTMAP_FORM);
	if (form)
	{
		form->closeAnimateDelete();
	}
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
	Widg = widgGetFromID(psWScreen, IDINTMAP_MSGVIEW);
	if (Widg)
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
	//remove 3dView if still there
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDINTMAP_MSGVIEW);
	if (form == nullptr)
	{
		return;
	}

	//stop the video
	VIEW_RESEARCH *psViewResearch = (VIEW_RESEARCH *)form->pUserData;
	seq_RenderVideoToBuffer(psViewResearch->sequenceName, SEQUENCE_KILL);

	if (animated)
	{
		// Start the window close animation.
		form->closeAnimateDelete();
	}
	else
	{
		//remove without the animating close window
		delete form;
	}
}

IntMessageButton::IntMessageButton(WIDGET *parent)
	: IntFancyButton(parent)
	, psMsg(nullptr)
{}

/*Displays the buttons used on the intelligence map */
void IntMessageButton::display(int xOffset, int yOffset)
{
	RESEARCH		*pResearch = NULL;
	bool MovieButton = false;
	ImdObject object;
	Image image;

	initDisplay();

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
			if (StatIsStructure(pResearch->psStat))
			{
				//this defines how the button is drawn
				object = ImdObject::StructureStat(pResearch->psStat);
			}
			else
			{
				int compID = StatIsComponent(pResearch->psStat);
				if (compID != COMP_NUMCOMPONENTS)
				{
					//this defines how the button is drawn
					object = ImdObject::Component(pResearch->psStat);
				}
				else
				{
					ASSERT(false, "intDisplayMessageButton: invalid stat");
					object = ImdObject::Research(pResearch);
				}
			}
		}
		else
		{
			//no Stat for this research topic so use the research topic to define what is drawn
			object = ImdObject::Research(pResearch);
		}
		break;
	case MSG_CAMPAIGN:
		image = Image(IntImages, IMAGE_INTEL_CAMPAIGN);
		MovieButton = true;
		break;
	case MSG_MISSION:
		image = Image(IntImages, IMAGE_INTEL_MISSION);
		MovieButton = true;
		break;
	default:
		debug(LOG_ERROR, "Unknown message type: %i", psMsg->type);
		return;
	}

	//if research message
	if (pResearch)
	{
		if (pResearch->iconID != NO_RESEARCH_ICON)
		{
			image = Image(IntImages, pResearch->iconID);
		}

		//do we have the same icon for the top right hand corner?
		displayIMD(image, object, xOffset, yOffset);
	}
	else
		//draw buttons for mission and general messages
	{
		// Draw the button with the relevant image, don't add isDown() to the image ID if it's a movie button.
		displayImage(MovieButton ? image : Image(image.images, image.id + isDown()), xOffset, yOffset);
	}
	displayIfHighlight(xOffset, yOffset);
	doneDisplay();
}


/* displays the PIE view for the current message */
void intDisplayPIEView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	MESSAGE *psMessage = (MESSAGE *)psWidget->pUserData;
	SWORD			image = -1;
	RESEARCH        *psResearch;

	// Should not have any proximity messages here...
	if (!psMessage || psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	if (psMessage->pViewData)
	{
		int x0 = xOffset + psWidget->x();
		int y0 = yOffset + psWidget->y();
		int x1 = x0 + psWidget->width();
		int y1 = y0 + psWidget->height();

		//moved from after close render
		RenderWindowFrame(FRAME_NORMAL, x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2);

		if (((VIEWDATA *)psMessage->pViewData)->type != VIEW_RES)
		{
			ASSERT(false, "intDisplayPIEView: Invalid message type");
			return;
		}

		//render an object
		psResearch = getResearchForMsg((VIEWDATA *)psCurrentMsg->pViewData);
		renderResearchToBuffer(psResearch, x0 + (x1 - x0) / 2, y0 + (y1 - y0) / 2);

		//draw image icon in top left of window
		image = (SWORD)getResearchForMsg((VIEWDATA *)psMessage->pViewData)->iconID;
		if (image > 0)
		{
			iV_DrawImage(IntImages, image, x0, y0);
		}
	}
}

/* displays the FLIC view for the current message */
void intDisplayFLICView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	MESSAGE *psMessage = (MESSAGE *)psWidget->pUserData;
	VIEW_RESEARCH	*psViewResearch;

	//shouldn't have any proximity messages here...
	if (!psMessage || psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	if (psMessage->pViewData)
	{
		int x0 = xOffset + psWidget->x();
		int y0 = yOffset + psWidget->y();
		int x1 = x0 + psWidget->width();
		int y1 = y0 + psWidget->height();

		if (((VIEWDATA *)psMessage->pViewData)->type != VIEW_RES)
		{
			ASSERT(false, "intDisplayFLICView: Invalid message type");
			return;
		}

		RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
		psViewResearch = (VIEW_RESEARCH *)((VIEWDATA *)psCurrentMsg->pViewData)->pData;
		// set the dimensions to window size & position
		seq_SetDisplaySize(192, 168, x0, y0);
		//render a frame of the current movie *must* force above resolution!
		seq_RenderVideoToBuffer(psViewResearch->sequenceName, SEQUENCE_HOLD);
	}
}

/**
 * Displays the TEXT view for the current message.
 * If this function breaks, please merge it with intDisplaySeqTextViewPage
 * which presumably does almost the same.
 */
void intDisplayTEXTView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	MESSAGE *psMessage = (MESSAGE *)psWidget->pUserData;

	int x0 = xOffset + psWidget->x();
	int y0 = yOffset + psWidget->y();
	int x1 = x0 + psWidget->width();
	int y1 = y0 + psWidget->height();
	int ty = y0;

	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);

	if (psMessage)
	{
		iV_SetFont(font_regular);
		/* Get the travel to the next line */
		int linePitch = iV_GetTextLineSize();
		/* Fix for spacing.... */
		linePitch += 3;
		ty += 3;
		/* Fix for spacing.... */


		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		//add each message
		for (unsigned i = 0; i < ((VIEWDATA *)psMessage->pViewData)->textMsg.size(); i++)
		{
			//check haven't run out of room first!
			if (i * linePitch > psWidget->height())
			{
				ASSERT(false, "intDisplayTEXTView: Run out of room!");
				return;
			}
			//need to check the string will fit!
			iV_DrawText(_(((VIEWDATA *)psMessage->pViewData)->textMsg[i].toUtf8().constData()), x0 + TEXT_XINDENT,
			            (ty + TEXT_YINDENT * 3) + (i * linePitch));
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
		if (!bInTutorial)
		{
			// Don't pause the scripts or the console if the tutorial is running.
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
		if (!bInTutorial)
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
	StartMessageSequences(psMessage, true);
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
