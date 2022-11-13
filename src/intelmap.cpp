/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/widget/gridlayout.h"
#include "lib/widget/button.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/label.h"
#include "lib/widget/scrollablelist.h"
#include "lib/sound/mixer.h" //for sound_GetUIVolume()
/* Includes direct access to render library */
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/piemode.h"

#include "display3d.h"
#include "lib/framework/cursors.h"
#include "lib/framework/input.h"
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
#include "warzoneconfig.h"
#include "seqdisp.h"
#include "mission.h"

#include "multiplay.h"
#include "lib/sequence/sequence.h"
#include "lib/sound/track.h"

#include "multimenu.h"
#include "qtscript.h"

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
#define INTMAP_TITLEHEIGHT		25

/*dimensions for FLIC view section relative to IDINTMAP_MSGVIEW*/
#define	INTMAP_FLICWIDTH		192
#define INTMAP_FLICHEIGHT		168

/*dimensions for PIE view section relative to IDINTMAP_MSGVIEW*/
#define	INTMAP_PIEWIDTH			238
#define INTMAP_PIEHEIGHT		INTMAP_FLICHEIGHT

//position for text on full screen video
#define VIDEO_TEXT_TOP_X				20
#define VIDEO_TEXT_TOP_Y				20
#define VIDEO_TEXT_BOTTOM_X				20
#define VIDEO_TEXT_BOTTOM_Y				444

#define TEXT_START_FRAME	0
#define TEXT_END_FRAME			9999

/* the widget screen */
extern std::shared_ptr<W_SCREEN> psWScreen;

static UDWORD			messageID;
static bool				immediateMessage = false;

//flags whether to open the Intel Screen with a message
static bool				playCurrent;

/* functions declarations ****************/
static bool intAddMessageForm(bool playCurrent);
static const char* getMessageTitle(const MESSAGE& message);
/*Displays the buttons used on the intelligence map */
class IntMessageButton : public IntFancyButton
{
public:
	IntMessageButton();

	virtual void display(int xOffset, int yOffset) override;

	void setMessage(MESSAGE *msg)
	{
		psMsg = msg;
	}

	std::string getTip() override
	{
		ASSERT_OR_RETURN("", psMsg != nullptr, "Null message?");
		const char* pMessageTitle = getMessageTitle(*psMsg);
		return (pMessageTitle != nullptr) ? pMessageTitle : "";
	}

protected:
	MESSAGE *psMsg;
};

/*deal with the actual button press - proxMsg is set to true if a proximity
  button has been pressed*/
static void intIntelButtonPressed(bool proxMsg, UDWORD id);

static void intDisplayPIEView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void intDisplayFLICView(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void addVideoText(SEQ_DISPLAY *psSeqDisplay, UDWORD sequence);

/*********************** VARIABLES ****************************/
// The current message being displayed
MESSAGE			*psCurrentMsg = nullptr;

#define PAUSE_DISPLAY_CONDITION (!bMultiPlayer)
#define PAUSEMESSAGE_YOFFSET (0)


/* Add the Intelligence Map widgets to the widget screen */
bool intAddIntelMap()
{
	bool			Animate = true;

	//check playCurrent with psCurrentMsg
	if (psCurrentMsg == nullptr)
	{
		playCurrent = false;
	}
	else
	{
		playCurrent = true;
	}

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDINTMAP_FORM) != nullptr)
	{
		intRemoveIntelMapNoAnim();
		Animate = false;
	}
	else
	{
		audio_StopAll();
	}

	//add message to indicate game is paused - single player mode
	if (PAUSE_DISPLAY_CONDITION)
	{
		if (widgGetFromID(psWScreen, IDINTMAP_PAUSELABEL) == nullptr)
		{
			W_LABINIT sLabInit;
			sLabInit.id = IDINTMAP_PAUSELABEL;
			sLabInit.formID = 0;
			sLabInit.x = INTMAP_LABELX;
			sLabInit.y = INTMAP_LABELY + PAUSEMESSAGE_YOFFSET;
			sLabInit.width = INTMAP_LABELWIDTH;
			sLabInit.height = INTMAP_LABELHEIGHT;
			sLabInit.pText = WzString::fromUtf8(_("PAUSED"));
			if (!widgAddLabel(psWScreen, &sLabInit))
			{
				return false;
			}
		}
	}

	//set pause states before putting the interface up
	setIntelligencePauseState();

	auto const &parent = psWScreen->psForm;

	// Add the main Intelligence Map form
	auto intMapForm = std::make_shared<IntFormAnimated>(Animate);  // Do not animate the opening, if the window was already open.
	parent->attach(intMapForm);
	intMapForm->id = IDINTMAP_FORM;
	intMapForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(INTMAP_X, INTMAP_Y, INTMAP_WIDTH, INTMAP_HEIGHT);
	}));

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
static bool intAddMessageForm(bool _playCurrent)
{
	if (selectedPlayer >= MAX_PLAYERS) { return true; }

	WIDGET *msgForm = widgGetFromID(psWScreen, IDINTMAP_FORM);

	/* Add the Message form */
	auto msgList = IntListTabWidget::make();
	msgForm->attach(msgList);
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

		auto button = std::make_shared<IntMessageButton>();
		msgList->attach(button);
		button->id = nextButtonId;
		button->setMessage(psMessage);
		msgList->addWidgetToLayout(button);

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
	if (_playCurrent)
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

/*Add the 3D world view for the particular message */
bool intAddMessageView(MESSAGE *psMessage)
{
	bool Animate = true;

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDINTMAP_MSGVIEW) != nullptr)
	{
		intRemoveMessageView(false);
		Animate = false;
	}
	if (MultiMenuUp)
	{
		intCloseMultiMenuNoAnim();
	}

	auto const &parent = psWScreen->psForm;

	auto intMapMsgView = std::make_shared<IntFormAnimated>(Animate);  // Do not animate the opening, if the window was already open.
	parent->attach(intMapMsgView);
	intMapMsgView->id = IDINTMAP_MSGVIEW;
	intMapMsgView->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(INTMAP_RESEARCHX, INTMAP_RESEARCHY, INTMAP_RESEARCHWIDTH, INTMAP_RESEARCHHEIGHT);
	}));

	/* Add the close box */
	W_BUTINIT sButInit;
	sButInit.id = IDINTMAP_CLOSE;
	sButInit.x = intMapMsgView->width() - OPT_GAP - CLOSE_SIZE;
	sButInit.y = OPT_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	auto closeButton = std::make_shared<W_BUTTON>(&sButInit);
	intMapMsgView->attach(closeButton);

	auto title = std::make_shared<W_LABEL>();
	title->setGeometry(0, 0, INTMAP_RESEARCHWIDTH, INTMAP_TITLEHEIGHT);
	title->setFontColour(WZCOL_YELLOW);
	title->setTextAlignment(WLAB_ALIGNCENTRE);
	title->setFont(font_regular, WZCOL_YELLOW);
	title->setString(getMessageTitle(*psMessage));
	intMapMsgView->attach(title);

	auto grid = std::make_shared<GridLayout>();
	intMapMsgView->attach(grid);
	grid->setGeometry(1, INTMAP_TITLEHEIGHT, INTMAP_RESEARCHWIDTH - 2, INTMAP_RESEARCHHEIGHT - 1 - INTMAP_TITLEHEIGHT);

	auto messages = ScrollableListWidget::make();
	messages->setItemSpacing(3);
	messages->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
	messages->setPadding({10, 10, 10, 10});

	if (psMessage->type != MSG_RESEARCH && psMessage->pViewData->type == VIEW_RPL)
	{
		auto psViewReplay = (VIEW_REPLAY *)psMessage->pViewData->pData;
		for (const auto &seq: psViewReplay->seqList)
		{
			for (const auto &msg: seq.textMsg)
			{
				auto message = std::make_shared<Paragraph>();
				message->setFontColour(WZCOL_TEXT_BRIGHT);
				message->addText(msg.toUtf8());
				messages->addItem(message);
			}
		}

		grid->place({0, 1, false}, {0, 1, true}, messages);

		return true;
	}

	ASSERT_OR_RETURN(false, psMessage->type != MSG_PROXIMITY, "Invalid message type for research");

	/*Add the PIE box*/
	W_FORMINIT sFormInit;
	sFormInit.id = IDINITMAP_PIEVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.width = INTMAP_PIEWIDTH;
	sFormInit.height = INTMAP_PIEHEIGHT;
	sFormInit.pDisplay = intDisplayPIEView;
	sFormInit.pUserData = psMessage;
	auto form3dView = std::make_shared<W_CLICKFORM>(&sFormInit);

	/*Add the Flic box if videos are installed, or on-demand video streaming is available */
	if (seq_hasVideos())
	{
		sFormInit = W_FORMINIT();
		sFormInit.id = IDINTMAP_FLICVIEW;
		sFormInit.style = WFORM_PLAIN;
		sFormInit.width = INTMAP_FLICWIDTH;
		sFormInit.height = INTMAP_FLICHEIGHT;
		sFormInit.pDisplay = intDisplayFLICView;
		sFormInit.pUserData = psMessage;
		auto flicView = std::make_shared<W_FORM>(&sFormInit);

		grid->place({0, 2, true}, {0, 1, false}, form3dView);
		grid->place({2, 1, false}, {0, 1, false}, flicView);
	}
	else
	{
		grid->place({1, 1, false}, {0, 1, false}, form3dView);
	}

	/*Add the text box*/
	for (const auto &msg: psMessage->pViewData->textMsg)
	{
		auto message = std::make_shared<Paragraph>();
		message->setFontColour(WZCOL_TEXT_BRIGHT);
		message->addText(msg.toUtf8());
		messages->addItem(message);
	}

	grid->place({0, 3, true}, {1, 1, true}, messages);

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
		psCurrentMsg = nullptr;
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

	ASSERT_OR_RETURN(, psMessage->pViewData != nullptr, "Invalid ViewData pointer");

	if (psMessage->pViewData->type == VIEW_RPL)
	{
		VIEW_REPLAY		*psViewReplay;
		UDWORD Sequence;

		// Surely we don't need to set up psCurrentMsg when we pass the message into this routine ... tim
		psViewReplay = (VIEW_REPLAY *)psMessage->pViewData->pData;

		seq_ClearSeqList();

		//add any sequences to the list to be played when the first one is finished
		for (Sequence = 0; Sequence < psViewReplay->seqList.size(); Sequence++)
		{
			if (psViewReplay->seqList.at(Sequence).flag == 1)
			{
				bLoop = true;
			}
			else
			{
				bLoop = false;
			}

			seq_AddSeqToList(psViewReplay->seqList.at(Sequence).sequenceName, psViewReplay->seqList.at(Sequence).audio, nullptr, bLoop);

			debug(LOG_GUI, "StartMessageSequences: sequence=%d", Sequence);
			addVideoText(&psViewReplay->seqList.at(Sequence), Sequence);
		}
		//play first full screen video
		if (Start == true)
		{
			seq_StartNextFullScreenVideo();
		}
	}

	else if (psMessage->pViewData->type == VIEW_RES)
	{
		VIEW_RESEARCH		*psViewReplay;
		//UDWORD Sequence;

		psViewReplay = (VIEW_RESEARCH *)psCurrentMsg->pViewData->pData;

		seq_ClearSeqList();
		seq_AddSeqToList(psViewReplay->sequenceName, psViewReplay->audio, nullptr, false);
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
		psCurrentMsg = nullptr;
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

		if (psMessage->pViewData)
		{
			// If it's a video sequence then play it anyway
			if (psMessage->pViewData->type == VIEW_RPL)
			{
				if (psMessage->pViewData)
				{
					intAddMessageView(psMessage);
				}
				// only attempt to show videos if they are installed
				if (seq_hasVideos())
				{
					StartMessageSequences(psMessage, true);
				}
			}
			else if (psMessage->pViewData->type == VIEW_RES)
			{
				psResearch = getResearchForMsg(psMessage->pViewData);
				if (psResearch != nullptr)
				{
					static AUDIO_STREAM *playing = nullptr;

					// only play the sample once, otherwise, they tend to overlap each other
					if (sound_isStreamPlaying(playing))
					{
						sound_StopStream(playing);
					}

					char const *audio = nullptr;
					switch (psResearch->iconID)
					{
					case IMAGE_RES_DROIDTECH:
					case IMAGE_RES_CYBORGTECH:
						audio = "sequenceaudio/res_droid.ogg";
						break;
					case IMAGE_RES_WEAPONTECH:
						audio = "sequenceaudio/res_weapons.ogg";
						break;
					case IMAGE_RES_COMPUTERTECH:
						audio = "sequenceaudio/res_com.ogg";
						break;
					case IMAGE_RES_POWERTECH:
						audio = "sequenceaudio/res_pow.ogg";
						break;
					case IMAGE_RES_SYSTEMTECH:
						audio = "sequenceaudio/res_systech.ogg";
						break;
					case IMAGE_RES_STRUCTURETECH:
					case IMAGE_RES_DEFENCE:
						audio = "sequenceaudio/res_strutech.ogg";
						break;
					}

					if (audio != nullptr)
					{
						playing = audio_PlayStream(audio, sound_GetUIVolume(), [](const AUDIO_STREAM *stream, const void *) {
							if (stream == playing)
							{
								playing = nullptr;
							}
						}, nullptr);
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


static void intCleanUpIntelMap()
{
	MESSAGE		*psMessage, *psNext;
	bool removedAMessage = false;

	if (selectedPlayer < MAX_PLAYERS)
	{
		//remove any research messages that have been read
		for (psMessage = apsMessages[selectedPlayer]; psMessage != nullptr; psMessage =
				 psNext)
		{
			psNext = psMessage->psNext;
			if (psMessage->type == MSG_RESEARCH && psMessage->read)
			{
				removeMessage(psMessage, selectedPlayer);
				removedAMessage = true;
			}
		}
	}
	if (removedAMessage)
	{
		jsDebugMessageUpdate();
	}
	resetIntelligencePauseState();
	immediateMessage = false;
}


/* Remove the Intelligence Map widgets from the screen */
void intRemoveIntelMap()
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
void intRemoveIntelMapNoAnim()
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

	if (psViewResearch != nullptr)
	{
		seq_RenderVideoToBuffer(psViewResearch->sequenceName, SEQUENCE_KILL);
	}
	else
	{
		const WzString dummy = "";
		seq_RenderVideoToBuffer(dummy, SEQUENCE_KILL);
	}

	if (animated)
	{
		// Start the window close animation.
		form->closeAnimateDelete();
	}
	else
	{
		//remove without the animating close window
		widgDelete(form);
	}
}

IntMessageButton::IntMessageButton()
	: IntFancyButton()
	, psMsg(nullptr)
{}

/*Displays the buttons used on the intelligence map */
void IntMessageButton::display(int xOffset, int yOffset)
{
	RESEARCH		*pResearch = nullptr;
	bool MovieButton = false;
	ImdObject object;
	AtlasImage image;

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
		pResearch = getResearchForMsg(psMsg->pViewData);
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
		image = AtlasImage(IntImages, IMAGE_INTEL_CAMPAIGN);
		MovieButton = true;
		break;
	case MSG_MISSION:
		image = AtlasImage(IntImages, IMAGE_INTEL_MISSION);
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
			image = AtlasImage(IntImages, pResearch->iconID);
		}

		//do we have the same icon for the top right hand corner?
		displayIMD(image, object, xOffset, yOffset);
	}
	else
		//draw buttons for mission and general messages
	{
		// Draw the button with the relevant image, don't add isDown() to the image ID if it's a movie button.
		displayImage(MovieButton ? image : AtlasImage(image.images, image.id + isDown()), xOffset, yOffset);
	}
	displayIfHighlight(xOffset, yOffset);
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
		int width = psWidget->width();
		int height = psWidget->height();

		//moved from after close render
		RenderWindowFrame(FRAME_NORMAL, x0, y0, width, height);

		if (psMessage->pViewData->type != VIEW_RES)
		{
			ASSERT(false, "intDisplayPIEView: Invalid message type");
			return;
		}

		//render an object
		psResearch = getResearchForMsg(psCurrentMsg->pViewData);
		if (psResearch)
		{
			renderResearchToBuffer(psResearch, x0 + width / 2, y0 + height / 2);
		}

		//draw image icon in top left of window
		psResearch = getResearchForMsg(psMessage->pViewData);
		if (psResearch)
		{
			image = (SWORD)(psResearch->iconID);
			if (image > 0)
			{
				iV_DrawImage(IntImages, image, x0 + 3, y0 + 3);
			}
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

		if (psMessage->pViewData->type != VIEW_RES)
		{
			ASSERT(false, "intDisplayFLICView: Invalid message type");
			return;
		}

		RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
		psViewResearch = (VIEW_RESEARCH *)psCurrentMsg->pViewData->pData;
		// set the dimensions to window size & position
		seq_SetDisplaySize(INTMAP_FLICWIDTH, INTMAP_FLICHEIGHT, x0, y0);
		//render a frame of the current movie *must* force above resolution!
		seq_RenderVideoToBuffer(psViewResearch->sequenceName, SEQUENCE_HOLD);
	}
}


//adds text to full screen video
void addVideoText(SEQ_DISPLAY *psSeqDisplay, UDWORD sequence)
{
	UDWORD	i, x, y;

	if (!psSeqDisplay->textMsg.empty())
	{
		//add each message, first at the top
		// FIXME We should perhaps get font size, and use that to calculate offset(s) ?
		x = VIDEO_TEXT_TOP_X;
		y = VIDEO_TEXT_TOP_Y;

		seq_AddTextForVideo(psSeqDisplay->textMsg[0].toUtf8().c_str(), x, y, TEXT_START_FRAME, TEXT_END_FRAME, SEQ_TEXT_POSITION); //startframe endFrame

		//add each message, the rest at the bottom
		x = VIDEO_TEXT_BOTTOM_X;
		// calculate the real bottom... NOTE, game assumes all videos to be 640x480
		y = static_cast<UDWORD>((double)pie_GetVideoBufferHeight() / 480. * (double)VIDEO_TEXT_BOTTOM_Y);
		i = 1;
		while (i < psSeqDisplay->textMsg.size())
		{
			seq_AddTextForVideo(psSeqDisplay->textMsg[i].toUtf8().c_str(), x, y, TEXT_START_FRAME, TEXT_END_FRAME, SEQ_TEXT_POSITION); //startframe endFrame
			//initialise after the first setting
			x = y = 0;
			i++;
		}
	}
}

/*sets psCurrentMsg for the Intelligence screen*/
void setCurrentMsg()
{
	MESSAGE *psMsg, *psLastMsg;

	ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "Unsupported selectedPlayer: %" PRIu32 "", selectedPlayer);

	psLastMsg = nullptr;
	for (psMsg = apsMessages[selectedPlayer]; psMsg != nullptr; psMsg =
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
void setIntelligencePauseState()
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
void resetIntelligencePauseState()
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
	}
}

/** Play an intelligence message.
 * This function is used from scripts to give updates to the mission.
 */
void displayImmediateMessage(MESSAGE *psMessage)
{
	/*
		This has to be changed to support a script calling a message in the intelligence screen
	*/

	// only attempt to show videos if they are installed
	if (seq_hasVideos())
	{
	    psCurrentMsg = psMessage;
	    StartMessageSequences(psMessage, true);
	}
	// remind the player that the message can be seen again from
	// the intelligence screen
	addConsoleMessage(_("New Intelligence Report"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
	flashReticuleButton(IDRET_INTEL_MAP);
}


// return whether a message is immediate
bool messageIsImmediate()
{
	return immediateMessage;
}

/*sets the flag*/
void setMessageImmediate(bool state)
{
	immediateMessage = state;
}

/* run intel map (in the game loop) */
void intRunIntelMap()
{
	if (keyPressed(KEY_ESC))
	{
		intResetScreen(false);
		// clear key press so we don't enter in-game options
		inputLoseFocus();
	}
}

static const char* getMessageTitle(const MESSAGE& message)
{
	RESEARCH* research;

	switch (message.type)
	{
	case MSG_RESEARCH:
		research = getResearchForMsg(message.pViewData);
		return research ? _(research->name.toUtf8().c_str()) : _("Research Update");
	case MSG_CAMPAIGN:
		return _("Project Goals and Updates");
	case MSG_MISSION:
		return _("Current Objective");
	default:
		return nullptr;
	}

	return nullptr;
}
