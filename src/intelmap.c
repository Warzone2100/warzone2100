/*
 * IntelMap.c		(Intelligence Map)
 *
 * Functions for the display of the Intelligence Map
 *
 */
#include <time.h>
#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
/* Includes direct access to render library */
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"

//#include "geo.h"
#include "display3d.h"
#include "resource.h"
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
#include "text.h"
#include "console.h"
#include "research.h"
#include "lib/gamelib/gtime.h"
#include "loop.h"
#include "lib/script/script.h"
#include "scripttabs.h"

#include "seqdisp.h"


#include "multiplay.h"
#include "lib/sound/cdaudio.h"

#include "scriptextern.h"



#include "csnap.h"
extern CURSORSNAP InterfaceSnap;

#define NO_VIDEO

// See research.txt for research entry to be displayed
// By defined this we jump straight to the research entry when clicking on any button in the intelmap screen
//#define DBG_RESEARCH_ENTRY (28)			// Weapon research test
//#define DBG_RESEARCH_ENTRY (100)			// Engineering research test
//#define DBG_RESEARCH_ENTRY (65)				// Structure Tech


//Height to view the world from in Intelligence Screen
/*#ifndef PSX
#define INTELMAP_VIEWHEIGHT		2250
#else
#define INTELMAP_VIEWHEIGHT		(2250)
#endif*/

/* Intelligence Map screen IDs */
//#define IDINTMAP_FORM			6000	//The intelligence map base form
#define IDINTMAP_MSGFORM		6001	//The intelligence map tabbed form
//#define IDINTMAP_MSGVIEW		6002	//The message 3D view for the intelligence screen
#define IDINTMAP_CLOSE			6004	//The close button icon for the 3D view
#define	IDINTMAP_PAUSELABEL		6005	//The paused message
#define	IDINITMAP_TITLEVIEW		6006	//The Title view part of MSGVIEW
#define	IDINITMAP_PIEVIEW		6007	//The PIE view part of MSGVIEW
#define IDINTMAP_FLICVIEW		6008	//The Flic View part of MSGVIEW
#define IDINTMAP_TEXTVIEW		6009	//The Text area of MSGVIEW
#define	IDINTMAP_TITLELABEL		6010	//The title text
#define IDINTMAP_SEQTEXT		6011	//Sequence subtitle text

#define IDINTMAP_MSGSTART		6100	//The first button on the intelligence form
#define	IDINTMAP_MSGEND			6139	//The last button on the intelligence form (40 MAX)

#define IDINTMAP_SEQTEXTSTART		6200	//Sequence subtitle text tabs

//Proximity Messages no longer displayed in Intel Screen
//#define IDINTMAP_PROXSTART		6200	//The first proximity button
//#define IDINTMAP_PROXEND		6299	//The last proximity button

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

/*3D View message form screen positions relative to INTMAP_FORM */
//#define INTMAP_VIEWX			211
//#define INTMAP_VIEWY			(-260)
//#define INTMAP_VIEWWIDTH		MSG_BUFFER_WIDTH
//#define INTMAP_VIEWHEIGHT		MSG_BUFFER_HEIGHT

/* Length of time the message stays on the bottom of the screen for in milliseconds */
#define	INTEL_TXT_LIFE			2000

//define the 3D View sizes and positions that are required - relative to INTMAP_FORM
//#ifndef PSX
#define	INTMAP_RESEARCHX		(100 + D_W)
#define INTMAP_RESEARCHY		(30	+ D_H)

#define	INTMAP_RESEARCHWIDTH	440
#define INTMAP_RESEARCHHEIGHT	288

//#else
//// PSX versions need to be a multiple of 16 pixels wide and high as thats the
//// clipping rectangle limitations ( I Think? ). So far only tested for CAMPAIGN windows.
//#define	INTMAP_RESEARCHX		(32*2)
//#define INTMAP_RESEARCHY		(32)
//#define	INTMAP_RESEARCHWIDTH	(320-(32*2))
//#define INTMAP_RESEARCHHEIGHT	(16*11)
//#endif

//define the 3D View sizes and positions that are required - relative to INTMAP_FORM
/*#define INTMAP_MISSIONX			(OBJ_BACKX)
#define INTMAP_MISSIONY			(46)
#define INTMAP_MISSIONWIDTH		(OBJ_WIDTH)
#define INTMAP_MISSIONHEIGHT	(215)
#define INTMAP_PROXIMITYX		(350)
#define INTMAP_PROXIMITYY		(70)
#define INTMAP_PROXIMITYWIDTH	(200)
#define INTMAP_PROXIMITYHEIGHT	(175)*/
//#else
// PSX versions need to be a multiple of 16 pixels wide and high as thats the
// clipping rectangle limitations ( I Think? ). So far only tested for CAMPAIGN windows.
/*#define INTMAP_MISSIONX			(OBJ_BACKX)
#define INTMAP_MISSIONY			(40)
#define INTMAP_MISSIONWIDTH		(OBJ_WIDTH)
#define INTMAP_MISSIONHEIGHT	(240)
#define INTMAP_PROXIMITYX		(350)
#define INTMAP_PROXIMITYY		(70)
#define INTMAP_PROXIMITYWIDTH	(200)
#define INTMAP_PROXIMITYHEIGHT	(200)*/
//#endif

/*dimensions for Title view section relative to IDINTMAP_MSGVIEW*/
/*dimensions for PIE view section relative to IDINTMAP_MSGVIEW*/

#define	INTMAP_TITLEX			0
#define INTMAP_TITLEY			0
#define	INTMAP_TITLEWIDTH		INTMAP_RESEARCHWIDTH
#define INTMAP_TITLEHEIGHT		18
#define	INTMAP_PIEX				3
#define INTMAP_PIEY				24

//#define INTMAP_PIEWIDTH		240
//#define INTMAP_PIEHEIGHT		169
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

/* Static variables ********************/
//static SDWORD			viewAngle;
//static SDWORD			viewHeight;
static UDWORD			messageID;
static BOOL				immediateMessage = FALSE;


//How many proximity messages are currently being displayed
//static UDWORD			numProxMsg;

//flags whether to open the Intel Screen with a message
static BOOL				playCurrent;

/* functions declarations ****************/
static BOOL intAddMessageForm(BOOL playCurrent);
/*Displays the buttons used on the intelligence map */
static void intDisplayMessageButton(struct _widget *psWidget, UDWORD xOffset,
							  UDWORD yOffset, UDWORD *pColours);
/* displays the 3D view for the current message */
//static void intDisplayMessageView(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
//					  UDWORD *pColours);

/* Add the Proximity message buttons */
//static BOOL intAddProximityButton(MESSAGE *pMessage, UDWORD inc);

/*Displays the proximity messages used on the intelligence map */
//static void intDisplayProximityButton(struct _widget *psWidget, UDWORD xOffset,
//							  UDWORD yOffset, UDWORD *pColours);

/*deal with the actual button press - proxMsg is set to true if a proximity
  button has been pressed*/
static void intIntelButtonPressed(BOOL proxMsg, UDWORD id);

/*this sets the width and height for the Intel map surface so that it fill the
appropriate sized image for the view*/
//static void setIntelBufferSize(UDWORD type);

/*sets the intel map surface back to the size it was created with */
//static void resetIntelBufferSize(void);

//static BOOL checkMessageOverlap(MESSAGE *psMessage, SWORD x, SWORD y);

/* draws the text message in the message window - only allows for one at the moment!*/
//static void displayIntelligenceMessage(MESSAGE *psMessage);

/* Remove the Message View from the Intelligence screen without animation*/
//static void intRemoveMessageViewNoAnim(BOOL animated);

static void intDisplayPIEView(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
					  UDWORD *pColours);
#ifndef NO_VIDEO
static void intDisplayFLICView(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
					  UDWORD *pColours);
#endif
static void intDisplayTEXTView(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
					  UDWORD *pColours);
static void addVideoText(SEQ_DISPLAY *psSeqDisplay, UDWORD sequence);

static void intDisplaySeqTextView(struct _widget *psWidget,
				  UDWORD xOffset, UDWORD yOffset,
				  UDWORD *pColours);
static BOOL intDisplaySeqTextViewPage(VIEW_REPLAY *psViewReplay,
				      UDWORD x0, UDWORD y0,
				      UDWORD width, UDWORD height,
				      BOOL render,
				      size_t *major, size_t *minor);



/*********************** VARIABLES ****************************/
// The current message being displayed
MESSAGE			*psCurrentMsg = NULL;

// The display stats for the current messages' text
TEXT_DISPLAY	currentTextDisplay;




#define PAUSE_DISPLAY_CONDITION (!bMultiPlayer)
#define PAUSEMESSAGE_YOFFSET (0)




/* Add the Intelligence Map widgets to the widget screen */
//BOOL intAddIntelMap(BOOL playCurrent)
BOOL _intAddIntelMap(void)
{
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;
	BOOL			Animate = TRUE;



	//check playCurrent with psCurrentMsg
	if (psCurrentMsg == NULL)
	{
		playCurrent = FALSE;
	}
	else
	{
		playCurrent = TRUE;
	}

	// Is the form already up?
	if(widgGetFromID(psWScreen,IDINTMAP_FORM) != NULL)
	{
		intRemoveIntelMapNoAnim();
		Animate = FALSE;
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
			memset(&sLabInit,0,sizeof(W_LABINIT));
			sLabInit.id = IDINTMAP_PAUSELABEL;
			sLabInit.formID = 0;
			sLabInit.style = WLAB_PLAIN;
			sLabInit.x = INTMAP_LABELX;
			sLabInit.y = INTMAP_LABELY+PAUSEMESSAGE_YOFFSET;
			sLabInit.width = INTMAP_LABELWIDTH;
			sLabInit.height = INTMAP_LABELHEIGHT;
			sLabInit.pText = strresGetString(psStringRes, STR_MISC_PAUSED);
			sLabInit.FontID = WFont;
			if (!widgAddLabel(psWScreen, &sLabInit))
			{
				return FALSE;
			}
		}
	}

	//set pause states before putting the interface up
	setIntelligencePauseState();

	memset(&sFormInit, 0, sizeof(W_FORMINIT));

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
		sFormInit.disableChildren = TRUE;
	}
	else
	{
// otherwise just recreate it.
		sFormInit.pDisplay = intDisplayPlainForm;
	}

	//sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

//#ifdef PSX
//	SetCurrentSnapFormID(&InterfaceSnap,sFormInit.id);
////	SetMouseFormPosition(&sFormInit);
//#endif

	if (!intAddMessageForm(playCurrent))
	{
		return FALSE;
	}



	return TRUE;
}

/* Add the Message sub form */
static BOOL intAddMessageForm(BOOL playCurrent)
{
	W_FORMINIT		sFormInit;
	W_FORMINIT		sBFormInit;
	UDWORD			numButtons, i;
	MESSAGE			*psMessage;
	RESEARCH		*psResearch;
	SDWORD			BufferID;

	/* Add the Message form */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
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
	//numProxMsg = 0;
	/*work out the number of buttons */
	for(psMessage = apsMessages[selectedPlayer]; psMessage; psMessage =
		psMessage->psNext)
	{
		//ignore proximity messages here
		if (psMessage->type == MSG_PROXIMITY)
		{
			//intAddProximityButton(psMessage, numProxMsg);
			//numProxMsg++;
		}
		else
		{
			numButtons++;
		}

        //stop adding the buttons once max has been reached
		if (numButtons > (IDINTMAP_MSGEND - IDINTMAP_MSGSTART))
		{
			break;
		}

	}

	//set the number of tabs required
	sFormInit.numMajor = numForms((OBJ_BUTWIDTH + OBJ_GAP) * numButtons,
								  (OBJ_WIDTH - OBJ_GAP)*2);

	//set minor tabs to 1
	for (i=0; i< sFormInit.numMajor; i++)
	{
		sFormInit.aNumMinors[i] = 1;
	}

	sFormInit.pFormDisplay = intDisplayObjectForm;
	sFormInit.pUserData = (void*)&StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}


	/* Add the message buttons */
	memset(&sBFormInit, 0, sizeof(W_FORMINIT));
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
			ASSERT((FALSE, "Tutorial message in Intelligence screen!"));
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
					sBFormInit.pTip = strresGetString(psStringRes, STR_INT_RESMESSAGE);
				}
				break;
			case MSG_CAMPAIGN:
				sBFormInit.pTip = strresGetString(psStringRes, STR_INT_GENMESSAGE);
				break;
			case MSG_MISSION:
				sBFormInit.pTip = strresGetString(psStringRes, STR_INT_MISMESSAGE);
				break;
			default:
				break;
		}

		BufferID = GetObjectBuffer();
		ASSERT((BufferID >= 0,"Unable to acquire object buffer."));
		RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
		ObjectBuffers[BufferID].Data = (void*)psMessage;
		sBFormInit.pUserData = (void*)&ObjectBuffers[BufferID];
		sBFormInit.pDisplay = intDisplayMessageButton;

		if (!widgAddForm(psWScreen, &sBFormInit))
		{
			return FALSE;
		}

		intSetCurrentCursorPosition(&InterfaceSnap,sBFormInit.id);

		/* if the current message matches psSelected lock the button */
		if (psMessage == psCurrentMsg)
		{
			messageID = sBFormInit.id;
			widgSetButtonState(psWScreen, messageID, WBUT_LOCK);
			widgSetTabs(psWScreen, IDINTMAP_MSGFORM, sBFormInit.majorID, 0);
		}

		/* Update the init struct for the next button */
		sBFormInit.id += 1;

        //stop adding the buttons when at max
		if (sBFormInit.id > IDINTMAP_MSGEND)
		{
			break;
		}

		ASSERT((sBFormInit.id < (IDINTMAP_MSGEND+1),"Too many message buttons"));

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
			//intIntelButtonPressed(TRUE, messageID);
		}
		else
		{
			intIntelButtonPressed(FALSE, messageID);
		}
	}
	return TRUE;
}

/*Add the 3D world view for the particular message (only research nmessages now) */
BOOL intAddMessageView(MESSAGE * psMessage)
{
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	W_LABINIT		sLabInit;
	BOOL			Animate = TRUE;
	RESEARCH		*psResearch;

/*	ASSERT((psMessage->type == MSG_RESEARCH,
 *		"intAddMessageView: invalid message type"));
 *	had to comment out this check, since the 'Fast Play' tutorial triggered it
 *	with psMessage->type=MSG_MISSION and ((VIEWDATA)*psMessage->pViewData)->type=VIEW_RPL,
 * 	but which is probably using the wrong function. - Per
 */

	// Is the form already up?
	if(widgGetFromID(psWScreen,IDINTMAP_MSGVIEW) != NULL)
	{
		intRemoveMessageView(FALSE);
		Animate = FALSE;
	}


	/* Add the base form */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = IDINTMAP_MSGVIEW;
	sFormInit.style = WFORM_PLAIN;
	//size and position depends on the type of message - ONLY RESEARCH now
	sFormInit.width = INTMAP_RESEARCHWIDTH;
	sFormInit.height = INTMAP_RESEARCHHEIGHT;
	sFormInit.x = (SWORD)INTMAP_RESEARCHX;
	sFormInit.y = (SWORD)INTMAP_RESEARCHY;

	/*switch (type)
	{
	case MSG_RESEARCH:
		sFormInit.width = INTMAP_CAMPAIGNWIDTH;
		sFormInit.height = INTMAP_CAMPAIGNHEIGHT;
		sFormInit.x = INTMAP_CAMPAIGNX;
		sFormInit.y = INTMAP_CAMPAIGNY;
		break;
	//these are Full Screen FMV now
	case MSG_CAMPAIGN:
	case MSG_MISSION:
	//case MSG_TUTORIAL:
		sFormInit.width = INTMAP_MISSIONWIDTH;
		sFormInit.height = INTMAP_MISSIONHEIGHT;
		sFormInit.x = INTMAP_MISSIONX;
		sFormInit.y = INTMAP_MISSIONY;
		break;
	//these are no longer displayed in Intel Screen
	case MSG_PROXIMITY:
		sFormInit.width = INTMAP_PROXIMITYWIDTH;
		sFormInit.height = INTMAP_PROXIMITYHEIGHT;
		sFormInit.x = INTMAP_PROXIMITYX;
		sFormInit.y = INTMAP_PROXIMITYY;
		break;
	default:
		ASSERT((FALSE, "Unknown message type"));
		return FALSE;
	}*/

	// If the window was closed then do open animation.
	if(Animate)
	{
		sFormInit.pDisplay = intOpenPlainForm;
		sFormInit.disableChildren = TRUE;
	}
	else
	{
		// otherwise just display it.
		sFormInit.pDisplay = intDisplayPlainForm;
	}

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}



	/* Add the close box */
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = IDINTMAP_MSGVIEW;
	sButInit.id = IDINTMAP_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = (SWORD)(sFormInit.width - OPT_GAP - CLOSE_SIZE);
	sButInit.y = OPT_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.pTip = strresGetString(psStringRes, STR_MISC_CLOSE);
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.pUserData = (void*)PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return FALSE;
	}


	if (psMessage->type != MSG_RESEARCH &&
	    ((VIEWDATA*)psMessage->pViewData)->type == VIEW_RPL)
	{
		W_FORMINIT	sTabForm;
		VIEW_REPLAY	*psViewReplay;
		size_t		i, cur_seq, cur_seqpage;

		psViewReplay = (VIEW_REPLAY *)((VIEWDATA *)psMessage->pViewData)->pData;

		/* Add a big tabbed text box for the subtitle text */
		memset(&sFormInit, 0, sizeof(W_FORMINIT));

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
						  FALSE, &cur_seq, &cur_seqpage));


		sFormInit.pFormDisplay = intDisplayObjectForm;
		sFormInit.pUserData = (void*)&StandardTab;
		sFormInit.pTabDisplay = intDisplayTab;

		if (!widgAddForm(psWScreen, &sFormInit))
		{
			return FALSE;
		}


		memset(&sTabForm, 0, sizeof(W_FORMINIT));
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
				return FALSE;
			}
		}

		return TRUE;
	}


	/*Add the Title box*/
	/*memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDINTMAP_MSGVIEW;
	sFormInit.id = IDINITMAP_TITLEVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = INTMAP_TITLEX;
	sFormInit.y = INTMAP_TITLEY;
	sFormInit.width = INTMAP_TITLEWIDTH;
	sFormInit.height = INTMAP_TITLEHEIGHT;
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}*/


	/*add the Label for the title box*/
	memset(&sLabInit,0,sizeof(W_LABINIT));
	sLabInit.id = IDINTMAP_TITLELABEL;
	sLabInit.formID = IDINTMAP_MSGVIEW;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = INTMAP_TITLEX + TEXT_XINDENT;
	sLabInit.y = INTMAP_TITLEY + TEXT_YINDENT;
	sLabInit.width = INTMAP_TITLEWIDTH;
	sLabInit.height = INTMAP_TITLEHEIGHT;
	//print research name in title bar

	ASSERT((psMessage->type != MSG_PROXIMITY,
		"intAddMessageView:Invalid message type for research"));

	psResearch = getResearchForMsg((VIEWDATA *)psMessage->pViewData);

	ASSERT((psResearch!=NULL,"Research not found"));
	//sLabInit.pText=psResearch->pName;
	sLabInit.pText = getStatName(psResearch);

	sLabInit.FontID = WFont;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return FALSE;
	}


	/*Add the PIE box*/

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDINTMAP_MSGVIEW;
	sFormInit.id = IDINITMAP_PIEVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = INTMAP_PIEX;
	sFormInit.y = INTMAP_PIEY;
	sFormInit.width = INTMAP_PIEWIDTH;
	sFormInit.height = INTMAP_PIEHEIGHT;
	sFormInit.pDisplay = intDisplayPIEView;
	sFormInit.pUserData = (void *)psMessage;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}


#ifndef NO_VIDEO
	/*Add the Flic box */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDINTMAP_MSGVIEW;
	sFormInit.id = IDINTMAP_FLICVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = INTMAP_FLICX;
	sFormInit.y = INTMAP_FLICY;
	sFormInit.width = INTMAP_FLICWIDTH;
	sFormInit.height = INTMAP_FLICHEIGHT;
	sFormInit.pDisplay = intDisplayFLICView;
	sFormInit.pUserData = (void *)psMessage;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}
#endif


	/*Add the text box*/
	memset(&sFormInit, 0, sizeof(W_FORMINIT));

	sFormInit.formID = IDINTMAP_MSGVIEW;

	sFormInit.id = IDINTMAP_TEXTVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = INTMAP_TEXTX;
	sFormInit.y = INTMAP_TEXTY;
	sFormInit.width = INTMAP_TEXTWIDTH;
	sFormInit.height = INTMAP_TEXTHEIGHT;
	sFormInit.pDisplay = intDisplayTEXTView;
	sFormInit.pUserData = (void *)psMessage;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return FALSE;
	}

	return TRUE;
}

/* Process return codes from the Intelligence Map */
void intProcessIntelMap(UDWORD id)
{

	if (id >= IDINTMAP_MSGSTART && id <= IDINTMAP_MSGEND)
	{
		intIntelButtonPressed(FALSE, id);
	}
	/*else if (id >= IDINTMAP_PROXSTART && id <= IDINTMAP_PROXEND)
	{
		intIntelButtonPressed(TRUE, id);
	}*/

	else if (id == IDINTMAP_CLOSE)
	{
		//if close button pressed on 3D View then close the view only
		psCurrentMsg = NULL;
		//initTextDisplay(psCurrentMsg, WFont, 255);
		intRemoveMessageView(TRUE);
	}

}


static BOOL intDisplaySeqTextViewPage(VIEW_REPLAY *psViewReplay,
				      UDWORD x0, UDWORD y0,
				      UDWORD width, UDWORD height,
				      BOOL render,
				      size_t *cur_seq, size_t *cur_seqpage)
{
	UDWORD x1, y1, i, linePitch, cur_y;
	UDWORD ty;
	UDWORD sequence;

	if (!psViewReplay)
	{
		return TRUE;	/* nothing to do */
	}

	x1 = x0 + width;
	y1 = y0 + height;
	ty = y0;

	iV_SetFont(WFont);
	/* Get the travel to the next line */
	linePitch = iV_GetTextLineSize();
	/* Fix for spacing.... */
	linePitch += 6;
	ty += 3;

	iV_SetTextColour(iV_PaletteNearestColour(255, 255, 255));

	cur_y = 0;

	/* add each message */
	for (sequence = *cur_seq, i = *cur_seqpage; sequence < psViewReplay->numSeq; sequence++)
	{
		SEQ_DISPLAY *psSeqDisplay = &psViewReplay->pSeqList[sequence];
		for (; i < psSeqDisplay->numText; i++)
		{
			if (render)
			{
				iV_DrawText(psSeqDisplay->ppTextMsg[i],
					    x0 + TEXT_XINDENT,
					    (ty + TEXT_YINDENT*3) + cur_y);
			}
			cur_y += linePitch;
			if (cur_y > height)
			{
				/* run out of room - need to make new tab */
				*cur_seq = sequence;
				*cur_seqpage = i;
				return FALSE;
			}
		}
		i = 0;
	}

	return TRUE;		/* done */
}

static void intDisplaySeqTextView(struct _widget *psWidget,
				  UDWORD xOffset, UDWORD yOffset,
				  UDWORD *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	VIEW_REPLAY *psViewReplay = (VIEW_REPLAY*)Form->pUserData;
	size_t cur_seq, cur_seqpage;
	UDWORD x0, y0, page;

	x0 = xOffset + Form->x;
	y0 = yOffset + Form->y;

	RenderWindowFrame(&FrameNormal, x0, y0, Form->width, Form->height);

	/* work out where we're up to in the text */
	cur_seq = cur_seqpage = 0;
	for (page = 0; page < Form->majorT; page++)
	{
		intDisplaySeqTextViewPage(psViewReplay, x0, y0,
					  Form->width, Form->height,
					  FALSE, &cur_seq, &cur_seqpage);
	}

	intDisplaySeqTextViewPage(psViewReplay, x0, y0,
				  Form->width, Form->height,
				  TRUE, &cur_seq, &cur_seqpage);
}


// Add all the Video Sequences for a message ... works on PC &  PSX
void StartMessageSequences(MESSAGE *psMessage, BOOL Start)
{

	BOOL bLoop = FALSE;

//		printf("start message sequence\n");		//[testing if we hit this] -Q
	//should never have a proximity message here
	if (psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	ASSERT((PTRVALID(psMessage->pViewData, sizeof(VIEWDATA)),
		"StartMessageSequences: invalid ViewData pointer"));

	if (((VIEWDATA *)psMessage->pViewData)->type == VIEW_RPL)
	{
		VIEW_REPLAY		*psViewReplay;
		UDWORD Sequence;

	// Surely we don't need to set up psCurrentMsg when we pass the message into this routine ... tim
//		psViewReplay = (VIEW_REPLAY *)((VIEWDATA *)psCurrentMsg->pViewData)->pData;
		psViewReplay = (VIEW_REPLAY *)((VIEWDATA *)psMessage->pViewData)->pData;

		seq_ClearSeqList();

		//add any sequences to the list to be played when the first one is finished
		for (Sequence = 0; Sequence < psViewReplay->numSeq; Sequence++)

		{
			if (psViewReplay->pSeqList[Sequence].flag == 1)
			{
				bLoop = TRUE;
			}
			else
			{
				bLoop = FALSE;
			}


			seq_AddSeqToList(psViewReplay->pSeqList[Sequence].sequenceName,psViewReplay->pSeqList[Sequence].pAudio, NULL, bLoop,Sequence);


//	{
//		STRING String[256];
//		sprintf(String,"seqadded %d of %d [%s]\n",Sequence,psViewReplay->numSeq,psViewReplay->pSeqList[Sequence].sequenceName);
//		prnt(1,String,0,0);
//	}
			debug( LOG_NEVER, "sequence=%d\n", Sequence );
			addVideoText(&psViewReplay->pSeqList[Sequence],Sequence);
		}
		//play first full screen video
		if (Start==TRUE)
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
		seq_AddSeqToList(psViewReplay->sequenceName,psViewReplay->pAudio, NULL, FALSE,0);
		//play first full screen video
		if (Start==TRUE)
		{
			seq_StartNextFullScreenVideo();
		}
	}

}



UDWORD ButtonPresses=0;


#ifdef DBG_RESEARCH_ENTRY

MESSAGE TimsMessage=
{
	MSG_RESEARCH,NULL,NULL,FALSE,0
};
#endif


/*
deal with the actual button press - proxMsg is set to true if a proximity
button has been pressed
*/
void _intIntelButtonPressed(BOOL proxMsg, UDWORD id)
{
	MESSAGE			*psMessage;
	UDWORD			currID;//, i;
//	char aAudioName[MAX_STR_LENGTH];	// made static to reduce stack usage.
	RESEARCH		*psResearch;

	ASSERT((proxMsg != TRUE,
		"intIntelButtonPressed: Shouldn't be able to get a proximity message!"));

	/* message button has been pressed - clear the old button and messageView*/
	if (messageID != 0)
	{
		widgSetButtonState(psWScreen, messageID, 0);
		intRemoveMessageView(FALSE);
		psCurrentMsg = NULL;
		//initTextDisplay(psCurrentMsg, WFont, 255);
	}

	/* Lock the new button */
	// This means we can't click on the same movie button twice.
//	widgSetButtonState(psWScreen, id, WBUT_LOCK);
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

#ifdef DBG_RESEARCH_ENTRY
#warning DEBUG MESSAGE CODE ADDED ---- REMOVE BEFORE FINAL BUILD !!!!!!
	ButtonPresses++;
	if (ButtonPresses>=2)	// 2nd time only !!!
	{
		psMessage=&TimsMessage;
		TimsMessage.pViewData= asResearch[DBG_RESEARCH_ENTRY].pViewData;
	}
#endif

	//deal with the message if one
	if (psMessage)
	{

		//set the current message
		psCurrentMsg = psMessage;
		//initTextDisplay(psCurrentMsg, WFont, 255);

		//set the read flag
		psCurrentMsg->read = TRUE;

		//this is for the deaf! - done in intDisplayMessageView()
		/*if (psMessage->pViewData->pTextMsg)
		{
			addGameMessage(psMessage->pViewData->pTextMsg, INTEL_TXT_LIFE,
				TRUE);
		}*/

//DBPRINTF(("Dealing with a new message !!! type=%d\n",psMessage->pViewData->type);
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

			StartMessageSequences(psMessage,TRUE);

		}
		else if (((VIEWDATA *)psMessage->pViewData)->type == VIEW_RES)
		{

			//this must be for the blind
			//with forsight this information was removed from the meassage text
/*
			if (((VIEW_RESEARCH *)((VIEWDATA *)psMessage->pViewData)->pData)->pAudio != NULL)
			{
				ASSERT((strlen(((VIEW_RESEARCH *)((VIEWDATA *)psMessage->pViewData)->
					pData)->pAudio)<244,"sequence path+name greater than max string"));
				strcpy(aAudioName,"sequenceAudio\\");
				strcat(aAudioName,((VIEW_RESEARCH *)((VIEWDATA *)psMessage->
					pViewData)->pData)->pAudio);

				audio_PlayStream(aAudioName, AUDIO_VOL_MAX, NULL);
			}
*/
			//This hack replaces it
			psResearch = getResearchForMsg((VIEWDATA *)psMessage->pViewData);
			if (psResearch != NULL)
			{
				switch(psResearch->iconID)
				{
				case IMAGE_RES_DROIDTECH:
					audio_PlayStream("sequenceAudio\\Res_Droid.wav", AUDIO_VOL_MAX, NULL);
						break;
				case IMAGE_RES_WEAPONTECH:
					audio_PlayStream("sequenceAudio\\Res_Weapons.wav", AUDIO_VOL_MAX, NULL);
						break;
				case IMAGE_RES_COMPUTERTECH:
					audio_PlayStream("sequenceAudio\\Res_com.wav", AUDIO_VOL_MAX, NULL);
						break;
				case IMAGE_RES_POWERTECH:
					audio_PlayStream("sequenceAudio\\Res_Power.wav", AUDIO_VOL_MAX, NULL);
						break;
				case IMAGE_RES_SYSTEMTECH:
					audio_PlayStream("sequenceAudio\\Res_SysTech.wav", AUDIO_VOL_MAX, NULL);
						break;
				case IMAGE_RES_STRUCTURETECH:
					audio_PlayStream("sequenceAudio\\Res_StruTech.wav", AUDIO_VOL_MAX, NULL);
						break;
				case IMAGE_RES_CYBORGTECH:
					audio_PlayStream("sequenceAudio\\Res_Droid.wav", AUDIO_VOL_MAX, NULL);
						break;
				case IMAGE_RES_DEFENCE:
					audio_PlayStream("sequenceAudio\\Res_StruTech.wav", AUDIO_VOL_MAX, NULL);
						break;
	//				default:
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


void intCleanUpIntelMap(void)
{
	MESSAGE		*psMessage, *psNext;

	//remove any research messages that have been read
	for (psMessage = apsMessages[selectedPlayer]; psMessage != NULL; psMessage =
		psNext)
	{
		psNext = psMessage->psNext;
		if (psMessage->type == MSG_RESEARCH AND psMessage->read)
		{
			removeMessage(psMessage, selectedPlayer);
		}
	}
	resetIntelligencePauseState();
	immediateMessage = FALSE;


	cdAudio_Resume();

	// FIXME: NOT SURE IT'S CORRECT. this makes the transports come.
        eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
}


/* Remove the Intelligence Map widgets from the screen */
void intRemoveIntelMap(void)
{
	//UDWORD buttonID;
	WIDGET		*Widg;
	W_TABFORM	*Form;
	//MESSAGE		*psMessage, *psNext;


	//remove each proximity button
	/*for (buttonID = 0; buttonID < numProxMsg; buttonID++)
	{
		widgDelete(psWScreen, IDINTMAP_PROXSTART + buttonID);
	}*/
	//remove 3dView if still there
	Widg = widgGetFromID(psWScreen,IDINTMAP_MSGVIEW);
	if(Widg)
	{
		intRemoveMessageView(FALSE);
	}

	// Start the window close animation.
	Form = (W_TABFORM*)widgGetFromID(psWScreen,IDINTMAP_FORM);
	Form->display = intClosePlainForm;
	Form->disableChildren = TRUE;
	Form->pUserData = (void*)0;	// Used to signal when the close anim has finished.
	ClosingIntelMap = TRUE;
	//remove the text label
	widgDelete(psWScreen, IDINTMAP_PAUSELABEL);

	intCleanUpIntelMap();


//	//remove any research messages that have been read
//	for (psMessage = apsMessages[selectedPlayer]; psMessage != NULL; psMessage =
//		psNext)
//	{
//		psNext = psMessage->psNext;
//		if (psMessage->type == MSG_RESEARCH AND psMessage->read)
//		{
//			removeMessage(psMessage, selectedPlayer);
//		}
//	}
//	resetIntelligencePauseState();
//
//	immediateMessage = FALSE;
}

/* Remove the Intelligence Map widgets from the screen */
void intRemoveIntelMapNoAnim(void)
{
	//UDWORD buttonID;
	WIDGET *Widg;

	//remove each proximity button
	/*for (buttonID = 0; buttonID < numProxMsg; buttonID++)
	{
		widgDelete(psWScreen, IDINTMAP_PROXSTART + buttonID);
	}*/
	//remove 3dView if still there
	Widg = widgGetFromID(psWScreen,IDINTMAP_MSGVIEW);
	if(Widg)
	{
		intRemoveMessageView(FALSE);
	}
	//remove main Intelligence screen
	widgDelete(psWScreen, IDINTMAP_FORM);
	//remove the text label
	widgDelete(psWScreen, IDINTMAP_PAUSELABEL);

	intCleanUpIntelMap();


//	resetIntelligencePauseState();
//
//	immediateMessage = FALSE;
}

/* Remove the Message View from the Intelligence screen */
void intRemoveMessageView(BOOL animated)
{
	W_TABFORM		*Form;
	VIEW_RESEARCH	*psViewResearch;

	//remove 3dView if still there
	Form = (W_TABFORM*)widgGetFromID(psWScreen,IDINTMAP_MSGVIEW);
	if(Form)
	{

		//stop the video
		psViewResearch = (VIEW_RESEARCH *)Form->pUserData;
		seq_RenderVideoToBuffer(NULL, psViewResearch->sequenceName,
			gameTime2, SEQUENCE_KILL);


		if (animated)
		{

			widgDelete(psWScreen, IDINTMAP_CLOSE);


			// Start the window close animation.
			Form->display = intClosePlainForm;
			Form->disableChildren = TRUE;
			Form->pUserData = (void*)0;	// Used to signal when the close anim has finished.
			ClosingMessageView = TRUE;
		}
		else
		{
			//remove without the animating close window
			widgDelete(psWScreen, IDINTMAP_MSGVIEW);

		}
	}
}


/*Displays the buttons used on the intelligence map */
void intDisplayMessageButton(struct _widget *psWidget, UDWORD xOffset,
							  UDWORD yOffset, UDWORD *pColours)
{
	W_CLICKFORM		*psButton = (W_CLICKFORM*)psWidget;
	RENDERED_BUTTON *psBuffer = (RENDERED_BUTTON*)psButton->pUserData;
	MESSAGE			*psMsg;
	BOOL			Hilight = FALSE;
	UDWORD			Down = 0, IMDType = 0, compID;
	SDWORD			image = -1;
	RESEARCH		*pResearch = NULL;
    BASE_STATS      *psResGraphic = NULL;
	BOOL MovieButton = FALSE;

	OpenButtonRender((UWORD)(xOffset+psButton->x), (UWORD)(yOffset+psButton->y),
		psButton->width, psButton->height);

	Down = psButton->state & (WBUTS_DOWN | WBUTS_CLICKLOCK);
	Hilight = psButton->state & WBUTS_HILITE;

	// Get the object associated with this widget.
	psMsg = (MESSAGE *)psBuffer->Data;
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
        	if (pResearch->psStat)
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
					ASSERT((FALSE, "intDisplayMessageButton: invalid stat"));
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
		MovieButton = TRUE;
		break;
	case MSG_MISSION:
		image = IMAGE_INTEL_MISSION;
		MovieButton = TRUE;
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
			//RenderToButton(IntImages,(UWORD)image,pResearch,selectedPlayer,psBuffer,Down,
			//				IMDType,TOPBUTTON);											// ajl, changed from 0 to selectedPLayer
			RenderToButton(IntImages,(UWORD)image,psResGraphic,selectedPlayer,
                psBuffer,Down,IMDType,TOPBUTTON);
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

		iV_DrawTransImage(IntImages,IMAGE_BUT_HILITE,xOffset+psButton->x,
			yOffset+psButton->y);
	}
}


/* displays the PIE view for the current message */
void intDisplayPIEView(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
					  UDWORD *pColours)
{
	W_TABFORM		*Form = (W_TABFORM*)psWidget;
	MESSAGE			*psMessage = (MESSAGE *)Form->pUserData;
	UDWORD			x0,y0,x1,y1;
	VIEW_RESEARCH	*psViewResearch;
	SWORD			image = -1;
//#ifndef PSX
    RESEARCH        *psResearch;
//#endif


	//shouldn't have any proximity messages here...
	if (psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	if (psMessage AND psMessage->pViewData)
	{
		x0 = xOffset+Form->x;
		y0 = yOffset+Form->y;
		x1 = x0 + Form->width;
		y1 = y0 + Form->height;


		//moved from after close render
		RenderWindowFrame(&FrameNormal,x0-1,y0-1,x1-x0+2,y1-y0+2);


		OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),
			Form->width, Form->height);
		//OpenButtonRender(Form->x, Form->y,Form->width, Form->height);

		if (((VIEWDATA *)psMessage->pViewData)->type != VIEW_RES)
		{
			ASSERT((FALSE, "intDisplayPIEView: Invalid message type"));
			return;
		}

		//render an object
		psViewResearch = (VIEW_RESEARCH *)((VIEWDATA *)psCurrentMsg->pViewData)->pData;

		psResearch = getResearchForMsg((VIEWDATA *)psCurrentMsg->pViewData);
		renderResearchToBuffer(pIntelMapSurface, psResearch, x0+(x1-x0)/2, y0+(y1-y0)/2);

		CloseButtonRender();

		//draw image icon in top left of window
		image = (SWORD)getResearchForMsg((VIEWDATA *)psMessage->pViewData)->iconID;
		if (image > 0)
		{
			iV_DrawTransImage(IntImages,image,x0,y0);
		}
	}
}

#ifndef NO_VIDEO
/* displays the FLIC view for the current message */
void intDisplayFLICView(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
					  UDWORD *pColours)
{

	W_TABFORM		*Form = (W_TABFORM*)psWidget;
	MESSAGE			*psMessage = (MESSAGE *)Form->pUserData;
	UDWORD			x0,y0,x1,y1;
	VIEW_RESEARCH	*psViewResearch;
//#ifdef PSX
//	RECT			DrawArea;
//	UWORD			OTIndex;
//#endif

	//shouldn't have any proximity messages here...
	if (psMessage->type == MSG_PROXIMITY)
	{
		return;
	}

	if (psMessage AND psMessage->pViewData)
	{
		OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),
			Form->width, Form->height);

		x0 = xOffset+Form->x;
		y0 = yOffset+Form->y;
		x1 = x0 + Form->width;
		y1 = y0 + Form->height;


		if (((VIEWDATA *)psMessage->pViewData)->type != VIEW_RES)
		{
			ASSERT((FALSE, "intDisplayFLICView: Invalid message type"));
			return;
		}
		//render a frame of the current movie
		psViewResearch = (VIEW_RESEARCH *)((VIEWDATA *)psCurrentMsg->pViewData)->pData;
//#ifndef PSX
		seq_RenderVideoToBuffer(NULL, psViewResearch->sequenceName,
			gameTime2, SEQUENCE_HOLD);
		//download to screen now
		seq_BlitBufferToScreen((SBYTE *)rendSurface.buffer, rendSurface.scantable[1],
			x0, y0);
//#else
//	// PSXSequencesCountdown is the time until the playstation research seq. starts
//	// ... This gives the rest of the display a chance to have a head start.
//	//  ... avoiding screen flickers
//		if (PSXSequencesCountdown>0) PSXSequencesCountdown--;
//		if (PSXSequencesCountdown==1)
//		{
//			StartMessageSequences(psMessage,FALSE);	// PSX Version just starts the sequences
//			loop_SetVideoPlaybackMode();		// set so that the main loop plays the video !
//
//		}
//#endif
		CloseButtonRender();
	}

}
#endif

/* displays the TEXT view for the current message */
void intDisplayTEXTView(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
					  UDWORD *pColours)
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


	RenderWindowFrame(&FrameNormal,x0,y0,x1-x0,y1-y0);

	if (psMessage)
	{
		iV_SetFont(WFont);
		/* Get the travel to the next line */
		linePitch = iV_GetTextLineSize();
		/* Fix for spacing.... */
		linePitch+=6;
		ty+=3;
		/* Fix for spacing.... */


		iV_SetTextColour(iV_PaletteNearestColour(255, 255, 255));
		//add each message
		for (i = 0; i < ((VIEWDATA *)psMessage->pViewData)->numText; i++)
		{
			//displayIntelligenceMessage(psMessage);
			//check haven't run out of room first!
			if (i * linePitch > Form->height)
			{
				ASSERT((FALSE, "intDisplayTEXTView: Run out of room!"));
				return;
			}
			//need to check the string will fit!
			iV_DrawText(((VIEWDATA *)psMessage->pViewData)->ppTextMsg[i], x0 + TEXT_XINDENT,
				(ty + TEXT_YINDENT*3) + (i * linePitch));
		}

	}


}



//adds text to full screen video
void addVideoText(SEQ_DISPLAY *psSeqDisplay, UDWORD sequence)
{
	UDWORD	i, x, y;

	if (psSeqDisplay->numText > 0)
	{
		debug( LOG_NEVER, "avt seq=%d [%s]\n", sequence, psSeqDisplay->ppTextMsg[0] );
		//add each message, first at the top
		x = VIDEO_TEXT_TOP_X;
		y = VIDEO_TEXT_TOP_Y;


		seq_AddTextForVideo(psSeqDisplay->ppTextMsg[0], x, y, TEXT_START_FRAME, TEXT_END_FRAME, FALSE, sequence); //startframe endFrame

		//add each message, the rest at the bottom
		x = VIDEO_TEXT_BOTTOM_X;
		y = VIDEO_TEXT_BOTTOM_Y;
		i = 1;
		while (i < psSeqDisplay->numText)
		{
			seq_AddTextForVideo(psSeqDisplay->ppTextMsg[i], x, y, TEXT_START_FRAME, TEXT_END_FRAME, FALSE, sequence); //startframe endFrame
			//initialise after the first setting
			x = y = 0;
			i++;
		}
	}
}


/*rotate the view so looking directly down if forward = TRUE or
 back to previous view if forward = FALSE */
/*void intelMapView(BOOL forward)
{
	if (forward)
	{
		//save the current viewing angle
		viewAngle = player.r.x;
		viewHeight = player.p.y;
		//rotate to top down view
		player.r.x = DEG(-90);
#ifndef PSX
		player.p.y = INTELMAP_VIEWHEIGHT;
#else
		camera.p.y = INTELMAP_VIEWHEIGHT;
#endif
	}
	else
	{
		//rotate back to previous view angle
		player.r.x = viewAngle;
		player.p.y = viewHeight;
	}
}*/

/*this sets the width and height for the Intel map surface so that it fill the
appropriate sized image for the view*/
/*void setIntelBufferSize(UDWORD type)
{
	switch (type)
	{
	case MSG_CAMPAIGN:
	case MSG_RESEARCH:
		pIntelMapSurface->width = INTMAP_CAMPAIGNWIDTH;
		pIntelMapSurface->height = INTMAP_CAMPAIGNHEIGHT;
		break;
	case MSG_MISSION:
	//case MSG_TUTORIAL:
		pIntelMapSurface->width = INTMAP_MISSIONWIDTH;
		pIntelMapSurface->height = INTMAP_MISSIONHEIGHT;
		break;
	case MSG_PROXIMITY:
		pIntelMapSurface->width = INTMAP_PROXIMITYWIDTH;
		pIntelMapSurface->height = INTMAP_PROXIMITYHEIGHT;
		break;
	default:
		ASSERT((FALSE, "Invalid message type"));
	}
}
*/
/*sets the intel map surface back to the size it was created with */
/*void resetIntelBufferSize(void)
{
	pIntelMapSurface->width = MSG_BUFFER_WIDTH;
	pIntelMapSurface->height = MSG_BUFFER_HEIGHT;
}*/

/* draws the text message in the message window - only allows for one at the moment!*/
/*void displayIntelligenceMessage(MESSAGE *psMessage)
{
	UDWORD	x1, x2, y, indent = 10;


	x1 = INTMAP_TEXTX;
	x2 = INTMAP_TEXTX + INTMAP_TEXTWIDTH;
	y = INTMAP_TEXTY + INTMAP_TEXTHEIGHT + indent;

	//size and position depends on the type of message
	//switch (psMessage->type)
	{
	case MSG_RESEARCH:
		x1 = INTMAP_RESEARCHX;
		x2 = INTMAP_RESEARCHX + INTMAP_RESEARCHWIDTH;
		y = INTMAP_RESEARCHY + INTMAP_RESEARCHHEIGHT + indent;
		break;
	case MSG_MISSION:
      case MSG_CAMPAIGN:
	//case MSG_TUTORIAL:
		x1 = INTMAP_MISSIONX;
		x2 = INTMAP_MISSIONX + INTMAP_MISSIONWIDTH;
		y = INTMAP_MISSIONY + INTMAP_MISSIONHEIGHT + indent;// + INTMAP_TEXTWINDOWHEIGHT;// - indent;
		break;
	case MSG_PROXIMITY:
		x1 = INTMAP_PROXIMITYX;
		x2 = INTMAP_PROXIMITYX + INTMAP_PROXIMITYWIDTH;
		y = INTMAP_PROXIMITYY + INTMAP_PROXIMITYHEIGHT + indent;// + INTMAP_TEXTWINDOWHEIGHT;// - indent;
		break;
	default:
		ASSERT((FALSE, "Unknown message type"));
	}

#ifdef PSX
	DisplayControlDiag();
#endif

//#ifdef PSX
//	iV_DrawText(psMessage->pViewData->pTextMsg,x1,y);
//#else
//	screenSetTextColour(255,255,255);
//	screenTextOut(x+1,y+1,psMessage->pViewData->pTextMsg);
	//scrollMessage(psMessage->pViewData->pTextMsg, x2, x1, y, 5);

#ifdef PSX
	setConsoleSizePos(x1+2,y-32-iV_GetTextLineSize()*2,(x2-x1)-4);
#else
	setConsoleSizePos(x1, y, (x2-x1));
#endif
	setConsolePermanence(TRUE);
	addConsoleMessage(psMessage->pViewData->pTextMsg, LEFT_JUSTIFY);
}
*/
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

//initialise the text display stats for the current message
/*void initTextDisplay(MESSAGE *psMessage, UDWORD fontID, UWORD fontColour)
{
	UDWORD		width, currentLength, strLen, frames, inc;

	currentTextDisplay.font = fontID;
	currentTextDisplay.fontColour = fontColour;
	currentTextDisplay.startTime = 0;
	currentTextDisplay.text[0] = '\0';

	if (psMessage == NULL OR psMessage->pViewData->pTextMsg == NULL)
	{
		currentTextDisplay.totalFrames = 0;
		return;
	}

	//size of the text window depends on the message type
	switch (psMessage->type)
	{
	//case MSG_CAMPAIGN:
	case MSG_RESEARCH:
		width = INTMAP_RESEARCHWIDTH;
		break;
	//case MSG_MISSION:
	//case MSG_TUTORIAL:
	//	width = INTMAP_MISSIONWIDTH;
	//	break;
	//case MSG_PROXIMITY:
	//	width = INTMAP_PROXIMITYWIDTH;
	//	break;
	default:
		ASSERT((FALSE, "Unknown message type"));
		return;
	}

	currentLength = 0;
	strLen = strlen(psMessage->pViewData->pTextMsg);
	frames = 0;
	currentTextDisplay.totalFrames = 0;

	//get the length of the text message
	for (inc=0; inc < strLen; inc++)
	{
		currentLength += iV_GetCharWidth(psMessage->pViewData->pTextMsg[inc]);
	}

	width += currentLength;

	currentLength = 0;
	//how long for string to completely pass along width of view
	for (inc=0; inc < MAX_STR_LENGTH; inc++)
	{
		if (inc < strLen)
		{
			currentLength += iV_GetCharWidth(psMessage->pViewData->pTextMsg[inc]);
		}
		else
		{
			currentLength += iV_GetCharWidth(' ');
		}

		if (currentLength > width)
		{
			break;
		}
		currentTextDisplay.totalFrames++;
	}
	currentTextDisplay.startTime = gameTime2;
}*/

/* scroll the text message from right to left - aka tickertape messages */
/*void scrollMessage(STRING *pText, UDWORD startX, UDWORD endX, UDWORD y, UDWORD gap)
{
	UDWORD			frames, inc, strLen;
	SDWORD			position, startChar, currentLength;
	UDWORD			endChar, text;

	//work out current frame
	frames = 20 * (gameTime2 - currentTextDisplay.startTime)/GAME_TICKS_PER_SEC;

	//get the number of chars in the string
	strLen = strlen(pText);

	currentLength = 0;

	//work out position of the string
	for (inc = 0; inc < frames; inc++)
	{
		//framesMinus1 = frames - 1;
		if ((frames - 1 - inc) > (strLen-1))
		{
			//add a blank for 'characters' at the end of the sentence
			currentLength += iV_GetCharWidth(' ');
			//nothing to draw so go to next inc
			continue;
		}

		//increment the current amount drawn
		currentLength += iV_GetCharWidth(pText[frames - 1 - inc]);

		if (((SDWORD)startX - currentLength) < (SDWORD)endX)
		{
			//ignore this character since off the scale
			inc--;
			break;
		}
		position = startX - currentLength;
	}
	startChar = frames - 1 - inc;
	if (startChar < 0)
	{
		startChar = 0;
	}
	endChar = frames;
	if (endChar > strLen)
	{
		endChar = strLen;
	}
	text = 0;
	for (inc = startChar; inc != endChar AND inc < strLen; inc++)
	{
		currentTextDisplay.text[text++] = pText[inc];
	}
	currentTextDisplay.text[text] = '\0';
	iV_SetFont(currentTextDisplay.font);
	iV_SetTextColour(currentTextDisplay.fontColour);
	iV_DrawText(currentTextDisplay.text, position, y);

	//time to redo message
	if (frames > currentTextDisplay.totalFrames + gap)
	{
		//start again
		currentTextDisplay.startTime = gameTime2;
	}
}*/

/* Process return code from the Message View for Tutorial Mode*/
/*void intProcessMessageView(UDWORD id)
{
	if (id == IDINTMAP_CLOSE)
	{
		//if close button pressed on 3D View then close the view only
		psCurrentMsg = NULL;
		initTextDisplay(psCurrentMsg, WFont, 255);
		//intRemoveMessageView();
		intRemoveMessageViewNoAnim();
		intResetScreen(TRUE);
	}
}*/

/* Add the Proximity message buttons */
/*BOOL intAddProximityButton(MESSAGE *pMessage, UDWORD inc)
{
	W_FORMINIT		sBFormInit;
	VIEW_LOCATION	*pViewLocation = (VIEW_LOCATION*)pMessage->pViewData->pData;

	memset(&sBFormInit, 0, sizeof(W_FORMINIT));
	sBFormInit.formID = 0;
	sBFormInit.id = IDINTMAP_PROXSTART + inc;
	ASSERT((sBFormInit.id < IDINTMAP_PROXEND,"Too many message buttons"));
	sBFormInit.majorID = 0;
	sBFormInit.minorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	//width and height is dependant on the state of the message - see intDisplayProximityButton
	//the x and y need to be set up each time the button is drawn - see intDisplayProximityButton

	sBFormInit.pDisplay = intDisplayProximityButton;
	//set the data for this button
	sBFormInit.pUserData = pMessage;

	if (!widgAddForm(psWScreen, &sBFormInit))
	{
		return FALSE;
	}
	return TRUE;
}*/


/*Displays the proximity messages used on the intelligence map */
/*void intDisplayProximityButton(struct _widget *psWidget, UDWORD xOffset,
							  UDWORD yOffset, UDWORD *pColours)
{
	W_CLICKFORM			*psButton = (W_CLICKFORM*)psWidget;
	MESSAGE				*psMsg = (MESSAGE*)psButton->pUserData;
	PROXIMITY_DISPLAY	*psProximityDisplay;
	BOOL				Hilight = FALSE;
	VIEW_LOCATION		*psViewLocation;
	UBYTE				imageID;
	UDWORD				delay = 100;

	(void)pColours;
	(void)xOffset;
	(void)yOffset;

	// Get the object associated with this widget.
	psMsg = (MESSAGE *)psButton->pUserData;
	ASSERT((psMsg->type == MSG_PROXIMITY, "Invalid message type"));

	psViewLocation = (VIEW_LOCATION *)psMsg->pViewData->pData;

	//if not within view ignore message
	if (!clipXY(psViewLocation->location.x,	psViewLocation->location.z))
	{
		return;
	}

	psProximityDisplay = getProximityDisplay(psMsg);
	psButton->x = (SWORD)psProximityDisplay->screenX;
	psButton->y = (SWORD)psProximityDisplay->screenY;

	//get the screen coords for the message - check not 'off' the screen
	if (psButton->x < 0 OR psButton->x > DISP_WIDTH OR psButton->y < 0 OR
		psButton->y > DISP_HEIGHT)
	{
		return;
	}

	Hilight = psButton->state & WBUTS_HILITE;

	//if hilighted
	if (Hilight)
	{
		imageID = IMAGE_INTEL_PROXHILI;
		psButton->width = iV_GetImageWidth(IntImages,IMAGE_INTEL_PROXHILI);
		psButton->height = iV_GetImageHeight(IntImages,IMAGE_INTEL_PROXHILI);
	}
	else if (psMsg->read)
	{
		//if the message is read - don't animate
		imageID = IMAGE_INTEL_PROXREAD;
		psButton->width = iV_GetImageWidth(IntImages,IMAGE_INTEL_PROXREAD);
		psButton->height = iV_GetImageHeight(IntImages,IMAGE_INTEL_PROXREAD);
	}
	else
	{
		//draw animated
		if ((GetTickCount() - psProximityDisplay->timeLastDrawn) > delay)
		{
			psProximityDisplay->strobe++;
			if (psProximityDisplay->strobe > 2)
			{
				psProximityDisplay->strobe = 0;
			}
			psProximityDisplay->timeLastDrawn = GetTickCount();
		}
		imageID = (UBYTE)(IMAGE_INTEL_PROXIMITY + psProximityDisplay->strobe);
		psButton->width = iV_GetImageWidth(IntImages,IMAGE_INTEL_PROXIMITY);
		psButton->height = iV_GetImageHeight(IntImages,IMAGE_INTEL_PROXIMITY);
	}
	//adjust button x and y for width and height of button
	psButton->x = (SWORD)(psButton->x - psButton->width/(UWORD)2);
	psButton->y = (SWORD)(psButton->y - psButton->height/(UWORD)2);
	if (psButton->x < 0 OR psButton->x > DISP_WIDTH OR psButton->y < 0 OR
		psButton->y > DISP_HEIGHT)
	{
		return;
	}

	//if there is a message 3Dview up - don't draw the proximity messages underneath
	if (psCurrentMsg AND psCurrentMsg->pViewData)
	{
		if (!checkMessageOverlap(psCurrentMsg, psButton->x, psButton->y))
		{
			return;
		}
	}

	//draw the 'button'
	iV_DrawTransImage(IntImages,imageID, psButton->x, psButton->y);
}*/
/*check the x and y are within the messages 3D view if on screen */
/*BOOL checkMessageOverlap(MESSAGE *psMessage, SWORD x, SWORD y)
{
	SWORD		messageX, messageY, messageWidth, messageHeight;

	switch (psMessage->type)
	{
	case MSG_CAMPAIGN:
	case MSG_RESEARCH:
		messageX = INTMAP_CAMPAIGNX;
		messageY = INTMAP_CAMPAIGNY;
		messageWidth = INTMAP_CAMPAIGNWIDTH;
		messageHeight = INTMAP_CAMPAIGNHEIGHT;
		break;
	case MSG_MISSION:
		messageX = INTMAP_MISSIONX;
		messageY = INTMAP_MISSIONY;
		messageWidth = INTMAP_MISSIONWIDTH;
		messageHeight = INTMAP_MISSIONHEIGHT;
		break;
	case MSG_PROXIMITY:
		messageX = INTMAP_PROXIMITYX;
		messageY = INTMAP_PROXIMITYY;
		messageWidth = INTMAP_PROXIMITYWIDTH;
		messageHeight = INTMAP_PROXIMITYHEIGHT;
		break;
	default:
		ASSERT((FALSE, "Unknown message type"));
		return FALSE;
	}

	if ((x > messageX AND x < (messageX + messageWidth)) AND
		(y > messageY AND y < (messageY + messageHeight)))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}*/

/*sets which states need to be paused when the intelligence screen is up*/
void setIntelligencePauseState(void)
{


	if (!bMultiPlayer)
	{

		gameTimeStop();
		setGameUpdatePause(TRUE);
		if(!bInTutorial)
		{	// Don't pause the scripts or the console if the tutorial is running.
			setScriptPause(TRUE);
			setConsolePause(TRUE);
		}
		setScrollPause(TRUE);


	}


}

/*resets the pause states */
void resetIntelligencePauseState(void)
{

	if (!bMultiPlayer)
	{

		setGameUpdatePause(FALSE);
		if(!bInTutorial) {
			setScriptPause(FALSE);
		}
		setScrollPause(FALSE);
		setConsolePause(FALSE);
		gameTimeStart();


	}

}




// play this message immediately, but definitely donot tell the intelligence screen to start


void _displayImmediateMessage(MESSAGE *psMessage)
{

	/*
		This has to be changed to support a script calling a message in the intellegence screen

	*/

#ifdef NO_VIDEO
	psCurrentMsg = psMessage;
	/* so we lied about definately not starting the intelligence screen */
	addIntelScreen();
	/* reset mouse cursor, since addIntelScreen() doesn't do that */
	pie_SetMouse(IntImages,IMAGE_CURSOR_DEFAULT);
	frameSetCursorFromRes(IDC_DEFAULT);
	/* addIntelScreen() (via addIntelMap()) actually starts
	 * playing psCurrentMsg automatically */
	return;
#endif

	StartMessageSequences(psMessage,TRUE);
}


void displayImmediateMessage(MESSAGE *psMessage)
{


	_displayImmediateMessage(psMessage);
}


// return whether a message is immediate
BOOL messageIsImmediate(void)
{
	return immediateMessage;
}

/*sets the flag*/
void setMessageImmediate(BOOL state)
{
	immediateMessage = state;
}


BOOL intAddIntelMap(void)
{

	return _intAddIntelMap();
}


void intIntelButtonPressed(BOOL proxMsg, UDWORD id)
{


	_intIntelButtonPressed(proxMsg,id);
}





