#ifndef _intdisplay_h
#define _intdisplay_h

#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/bar.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/slider.h"
#include "intimage.h"
#include "droid.h"

#define NUM_OBJECTSURFACES		(10)
#define NUM_TOPICSURFACES		(5)
#define NUM_STATSURFACES		(20)
#define NUM_SYSTEM0SURFACES		(10)
#define NUM_OBJECTBUFFERS		(NUM_OBJECTSURFACES*4)
#define NUM_STATBUFFERS			(NUM_STATSURFACES*4)
#define NUM_TOPICBUFFERS		(NUM_TOPICSURFACES*4)

#define NUM_SYSTEM0BUFFERS		(NUM_SYSTEM0SURFACES*8)


/* Power levels are divided by this for power bar display. The extra factor has
been included so that the levels appear the same for the power bar as for the
power values in the buttons */
#define POWERBAR_SCALE			(5 * WBAR_SCALE/STAT_PROGBARWIDTH)

#define BUTTONOBJ_ROTSPEED		90	// Speed to rotate objects rendered in
									// buttons ( degrees per second )

//the two types of button used in the object display (bottom bar)
#define		TOPBUTTON			0
#define		BTMBUTTON			1


enum {
	IMDTYPE_NONE,
	IMDTYPE_DROID,
	IMDTYPE_DROIDTEMPLATE,
	IMDTYPE_COMPONENT,
	IMDTYPE_STRUCTURE,
	IMDTYPE_RESEARCH,
	IMDTYPE_STRUCTURESTAT,
};

typedef struct {
	char *Token;
	SWORD ID;
} TOKENID;

typedef struct {
	char *Token;
	SWORD ID;
	SWORD IMD;
} RESEARCHICON;


typedef struct {
	UBYTE *Buffer;		// Bitmap buffer.
struct 	iSurface *Surface;	// Ivis surface definition.
} BUTTON_SURFACE;




// I tried to get the PC code working with the above PSX structure but it was having none of it
//  ... sorry about that ... TC
#define RENDERBUTTON_INUSE(x)  ((x)->InUse=TRUE)
#define RENDERBUTTON_NOTINUSE(x)  ((x)->InUse=FALSE)

#define RENDERBUTTON_INITIALISED(x)  ((x)->Initialised=TRUE)
#define RENDERBUTTON_NOTINITIALISED(x)  ((x)->Initialised=FALSE)

#define IsBufferInitialised(x) ((x)->Initialised)
#define IsBufferInUse(x) ((x)->InUse)

typedef struct {
	BOOL InUse;			// Is it in use.
	BOOL Initialised;	// Is it initialised.
	SDWORD ImdRotation;	// Rotation if button is an IMD.
	UDWORD State;		// Copy of widget's state so we know if state has changed.
	void *Data;			// Any data we want to attach.
	void *Data2;		// Any data we want to attach.
	BUTTON_SURFACE *ButSurf;	// Surface to render the button into.
//	uint8 *Buffer;		// Bitmap buffer.
//	iSurface *Surface;	// Ivis surface definition.
} RENDERED_BUTTON;






extern RENDERED_BUTTON TopicBuffers[NUM_TOPICBUFFERS];
extern RENDERED_BUTTON ObjectBuffers[NUM_OBJECTBUFFERS];
extern RENDERED_BUTTON StatBuffers[NUM_STATBUFFERS];
extern RENDERED_BUTTON System0Buffers[NUM_SYSTEM0BUFFERS];
//extern RENDERED_BUTTON System1Buffers[NUM_OBJECTBUFFERS];
//extern RENDERED_BUTTON System2Buffers[NUM_OBJECTBUFFERS];
extern BUTTON_SURFACE TopicSurfaces[NUM_TOPICSURFACES];
extern BUTTON_SURFACE ObjectSurfaces[NUM_OBJECTSURFACES];
extern BUTTON_SURFACE StatSurfaces[NUM_STATSURFACES];
extern BUTTON_SURFACE System0Surfaces[NUM_SYSTEM0SURFACES];


extern UDWORD ManuPower;		// Power required to manufacture the current item.
extern UWORD ButXPos;
extern UWORD ButYPos;
extern UWORD ButWidth,ButHeight;

extern BASE_STATS *CurrentStatsTemplate;


// Set audio IDs for form opening/closing anims.
void SetFormAudioIDs(int OpenID,int CloseID);

// Initialise interface graphics.
void intInitialiseGraphics(void);

// Free up interface graphics.
void intDeleteGraphics(void);

// Intialise button surfaces.
void InitialiseButtonData(void);

// Free up button surfaces.
void DeleteButtonData(void);

// Get a free RENDERED_BUTTON structure for an object window button.
SDWORD GetObjectBuffer(void);

// Clear ( make unused ) all RENDERED_BUTTON structures for the object window.
void ClearObjectBuffers(void);

// Clear ( make unused ) all RENDERED_BUTTON structures for the topic window.
void ClearTopicBuffers(void);

// Clear ( make unused ) a RENDERED_BUTTON structure.
void ClearObjectButtonBuffer(SDWORD BufferID);

// Clear ( make unused ) a RENDERED_BUTTON structure.
void ClearTopicButtonBuffer(SDWORD BufferID);

void RefreshObjectButtons(void);
void RefreshSystem0Buttons(void);
void RefreshTopicButtons(void);
void RefreshStatsButtons(void);


// Get a free RENDERED_BUTTON structure for a stat window button.
SDWORD GetStatBuffer(void);

// Clear ( make unused ) all RENDERED_BUTTON structures for the stat window.
void ClearStatBuffers(void);

/*these have been set up for the Transporter - the design screen DOESN'T use them*/
// Clear ( make unused ) *all* RENDERED_BUTTON structures.
void ClearSystem0Buffers(void);
// Clear ( make unused ) a RENDERED_BUTTON structure.
void ClearSystem0ButtonBuffer(SDWORD BufferID);
// Get a free RENDERED_BUTTON structure.
SDWORD GetSystem0Buffer(void);

// callback to update the command droid size label
void intUpdateCommandSize(struct _widget *psWidget, struct _w_context *psContext);

// callback to update the command droid experience
void intUpdateCommandExp(struct _widget *psWidget, struct _w_context *psContext);

// callback to update the command droid factories
void intUpdateCommandFact(struct _widget *psWidget, struct _w_context *psContext);

void intUpdateProgressBar(struct _widget *psWidget, struct _w_context *psContext);

void intUpdateOptionText(struct _widget *psWidget, struct _w_context *psContext);

void intUpdateQuantity(struct _widget *psWidget, struct _w_context *psContext);
//callback to display the factory number
extern void intAddFactoryInc(struct _widget *psWidget, struct _w_context *psContext);
//callback to display the production quantity number for a template
extern void intAddProdQuantity(struct _widget *psWidget, struct _w_context *psContext);

void intDisplayPowerBar(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayStatusButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayObjectButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayStatsButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void AdjustTabFormSize(W_TABFORM *Form,UDWORD *x0,UDWORD *y0,UDWORD *x1,UDWORD *y1);

void intDisplayObjectForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayStatsForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intOpenPlainForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intClosePlainForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayPlainForm(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayImage(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayImageHilight(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayButtonHilight(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
void intDisplayAltButtonHilight(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayButtonFlash(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayButtonPressed(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayReticuleButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayTab(struct _widget *psWidget,UDWORD TabType, UDWORD Position,
					UDWORD Number,BOOL Selected,BOOL Hilight,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height);
void intDisplaySystemTab(struct _widget *psWidget,UDWORD TabType, UDWORD Position,
					UDWORD Number,BOOL Selected,BOOL Hilight,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height);
void intDisplaySlider(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void intDisplayNumber(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
void intAddLoopQuantity(struct _widget *psWidget, struct _w_context *psContext);

void intDisplayEditBox(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

void OpenButtonRender(UWORD XPos,UWORD YPos,UWORD Width,UWORD Height);
void CloseButtonRender(void);

void ClearButton(BOOL Down,UDWORD Size, UDWORD buttonType);

void RenderToButton(IMAGEFILE *ImageFile,UWORD ImageID,void *Object,UDWORD Player,RENDERED_BUTTON *Buffer,
					BOOL Down,UDWORD IMDType, UDWORD buttonType);

void CreateIMDButton(IMAGEFILE *ImageFile,UWORD ImageID,void *Object,UDWORD Player,RENDERED_BUTTON *Buffer,
					BOOL Down,UDWORD IMDType,UDWORD buttonType);

void CreateImageButton(IMAGEFILE *ImageFile,UWORD ImageID,RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType);

void CreateBlankButton(RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType);

void RenderImageToButton(IMAGEFILE *ImageFile,UWORD ImageID,RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType);
void RenderBlankToButton(RENDERED_BUTTON *Buffer,BOOL Down, UDWORD buttonType);

//void RenderCompositeDroid(UDWORD Index,iVector *Rotation,iVector *Position);


extern BOOL DroidIsRepairing(DROID *Droid);

BOOL DroidIsBuilding(DROID *Droid);
STRUCTURE *DroidGetBuildStructure(DROID *Droid);
BOOL DroidGoingToBuild(DROID *Droid);
BASE_STATS *DroidGetBuildStats(DROID *Droid);
iIMDShape *DroidGetIMD(DROID *Droid);
UDWORD DroidGetIMDIndex(DROID *Droid);
BOOL DroidIsDemolishing(DROID *Droid);

BOOL StructureIsManufacturing(STRUCTURE *Structure);
RESEARCH_FACILITY *StructureGetResearch(STRUCTURE *Structure);
BOOL StructureIsResearching(STRUCTURE *Structure);
FACTORY *StructureGetFactory(STRUCTURE *Structure);
iIMDShape *StructureGetIMD(STRUCTURE *Structure);

DROID_TEMPLATE *FactoryGetTemplate(FACTORY *Factory);

//iIMDShape *TemplateGetIMD(DROID_TEMPLATE *DroidTemp,UDWORD Player);
//UDWORD TemplateGetIMDIndex(DROID_TEMPLATE *Template,UDWORD Player);

//SDWORD ResearchGetImage(RESEARCH_FACILITY *Research);

BOOL StatIsStructure(BASE_STATS *Stat);
iIMDShape *StatGetStructureIMD(BASE_STATS *Stat,UDWORD Player);
BOOL StatIsTemplate(BASE_STATS *Stat);
BOOL StatIsFeature(BASE_STATS *Stat);

//iIMDShape *StatGetTemplateIMD(BASE_STATS *Stat,UDWORD Player);
//UDWORD StatGetTemplateIMDIndex(BASE_STATS *Stat,UDWORD Player);

SDWORD StatIsComponent(BASE_STATS *Stat);
//iIMDShape *StatGetComponentIMD(BASE_STATS *Stat);
//iIMDShape *StatGetComponentIMD(BASE_STATS *Stat, SDWORD compID);
BOOL StatGetComponentIMD(BASE_STATS *Stat, SDWORD compID,iIMDShape **CompIMD,iIMDShape **MountIMD);

BOOL StatIsResearch(BASE_STATS *Stat);
//void StatGetResearchImage(BASE_STATS *Stat,SDWORD *Image,iIMDShape **Shape, BOOL drawTechIcon);
void StatGetResearchImage(BASE_STATS *psStat, SDWORD *Image, iIMDShape **Shape,
                          BASE_STATS **ppGraphicData, BOOL drawTechIcon);

//SWORD GetTokenID(TOKENID *Tok,char *Token);
//SWORD FindTokenID(TOKENID *Tok,char *Token);

//displays a border for a form
extern void intDisplayBorderForm(struct _widget *psWidget, UDWORD xOffset,
								 UDWORD yOffset, UDWORD *pColours);

/* Draws a stats bar for the design screen */
extern void intDisplayStatsBar(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset,
							   UDWORD *pColours);
/* Draws a Template Power Bar for the Design Screen */
void intDisplayDesignPowerBar(struct _widget *psWidget, UDWORD xOffset,
							  UDWORD yOffset, UDWORD *pColours);

// Widget callback function to play an audio track.
extern void WidgetAudioCallback(int AudioID);

// Widget callback to display a contents button for the Transporter
extern void intDisplayTransportButton(struct _widget *psWidget, UDWORD xOffset,
						  UDWORD yOffset, UDWORD *pColours);
/*draws blips on radar to represent Proximity Display*/
extern void drawRadarBlips(void);

/*Displays the proximity messages blips over the world*/
extern void intDisplayProximityBlips(struct _widget *psWidget, UDWORD xOffset,
					UDWORD yOffset, UDWORD *pColours);

extern void intUpdateQuantitySlider(struct _widget *psWidget, struct _w_context *psContext);



extern void intDisplayDPButton(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

extern void intDisplayTime(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
extern void intDisplayNum(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

extern void intDisplayResSubGroup(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

extern void intDisplayMissionClock(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

extern void intDisplayAllyIcon(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

#endif
