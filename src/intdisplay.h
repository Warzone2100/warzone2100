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

#ifndef __INCLUDED_SRC_INTDISPLAY_H__
#define __INCLUDED_SRC_INTDISPLAY_H__

#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "intimage.h"
#include "droid.h"
#include "template.h"


/* Power levels are divided by this for power bar display. The extra factor has
been included so that the levels appear the same for the power bar as for the
power values in the buttons */
#define POWERBAR_SCALE			(5 * WBAR_SCALE/STAT_PROGBARWIDTH)

#define BUTTONOBJ_ROTSPEED		90	// Speed to rotate objects rendered in
									// buttons ( degrees per second )

enum ImdType
{
	IMDTYPE_NONE,
	IMDTYPE_DROID,
	IMDTYPE_DROIDTEMPLATE,
	IMDTYPE_COMPONENT,
	IMDTYPE_STRUCTURE,
	IMDTYPE_RESEARCH,
	IMDTYPE_STRUCTURESTAT,
	IMDTYPE_FEATURE,
};

struct ImdObject
{
	ImdObject() : ptr(nullptr), type(IMDTYPE_NONE) {}
	static ImdObject Droid(BASE_OBJECT *p) { return ImdObject(p, IMDTYPE_DROID); }
	static ImdObject DroidTemplate(BASE_STATS *p) { return ImdObject(p, IMDTYPE_DROIDTEMPLATE); }
	static ImdObject Component(BASE_STATS *p) { return ImdObject(p, IMDTYPE_COMPONENT); }
	static ImdObject Structure(BASE_OBJECT *p) { return ImdObject(p, IMDTYPE_STRUCTURE); }
	static ImdObject Research(BASE_STATS *p) { return ImdObject(p, IMDTYPE_RESEARCH); }
	static ImdObject StructureStat(BASE_STATS *p) { return ImdObject(p, IMDTYPE_STRUCTURESTAT); }
	static ImdObject Feature(BASE_STATS *p) { FEATURE_STATS *fStat = (FEATURE_STATS *)p; return ImdObject(fStat->psImd, IMDTYPE_FEATURE); }

	bool empty() const { return ptr == nullptr; }

	void *ptr;
	ImdType type;

private:
	ImdObject(void *ptr, ImdType type) : ptr(ptr), type(type) {}
};

// Set audio IDs for form opening/closing anims.
void SetFormAudioIDs(int OpenID,int CloseID);

// Initialise interface graphics.
void intInitialiseGraphics(void);

// callback to update the command droid size label
void intUpdateCommandSize(WIDGET *psWidget, W_CONTEXT *psContext);

// callback to update the command droid experience
void intUpdateCommandExp(WIDGET *psWidget, W_CONTEXT *psContext);

// callback to update the command droid factories
void intUpdateCommandFact(WIDGET *psWidget, W_CONTEXT *psContext);

void intUpdateProgressBar(WIDGET *psWidget, W_CONTEXT *psContext);

void intUpdateQuantity(WIDGET *psWidget, W_CONTEXT *psContext);
//callback to display the factory number
extern void intAddFactoryInc(WIDGET *psWidget, W_CONTEXT *psContext);
//callback to display the production quantity number for a template
extern void intAddProdQuantity(WIDGET *psWidget, W_CONTEXT *psContext);

void intDisplayPowerBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

class IntFancyButton : public W_CLICKFORM
{
public:
	IntFancyButton(WIDGET *parent);

protected:
	//the two types of button used in the object display (bottom bar)
	enum ButtonType {TOPBUTTON = 0, BTMBUTTON = 1};

	void initDisplay();
	void doneDisplay();
	void displayClear(int xOffset, int yOffset);
	void displayIMD(Image image, ImdObject imdObject, int xOffset, int yOffset);
	void displayImage(Image image, int xOffset, int yOffset);
	void displayBlank(int xOffset, int yOffset);
	void displayIfHighlight(int xOffset, int yOffset);

	struct
	{
		Vector3i position;
		Vector3i rotation;
		int scale;
		int rate;
	} model;
	ButtonType buttonType;  // TOPBUTTON is square, BTMBUTTON has a little up arrow.
};

class IntObjectButton : public IntFancyButton
{
public:
	IntObjectButton(WIDGET *parent);

	virtual void display(int xOffset, int yOffset);

	void setObject(BASE_OBJECT *object) { psObj = object; }
	bool clearData() { bool ret = psObj != nullptr; psObj = nullptr; return ret; }

protected:
	BASE_OBJECT *psObj;
};

class IntStatusButton : public IntObjectButton
{
public:
	IntStatusButton(WIDGET *parent);

	void setObject(BASE_OBJECT *object) { psObj = object; theStats = nullptr; }
	void setObjectAndStats(BASE_OBJECT *object, BASE_STATS *stats) { psObj = object; theStats = stats; }

	virtual void display(int xOffset, int yOffset);

protected:
	BASE_STATS *theStats;
};

class IntStatsButton : public IntFancyButton
{
public:
	IntStatsButton(WIDGET *parent);

	virtual void display(int xOffset, int yOffset);

	void setStats(BASE_STATS *stats) { Stat = stats; }
	void setStatsAndTip(BASE_STATS *stats) { setStats(stats); setTip(stats->name); }

protected:
	BASE_STATS *Stat;
};

/// Form which animates opening/closing.
class IntFormAnimated : public W_FORM
{
public:
	IntFormAnimated(WIDGET *parent, bool openAnimate = true);

	virtual void display(int xOffset, int yOffset);

	void closeAnimateDelete();              ///< Animates the form closing, and deletes itself when done.

private:
	unsigned        startTime;              ///< Animation start time
	int             currentAction;          ///< Opening/open/closing/closed.
};

void intDisplayImage(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

void intDisplayImageHilight(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

void intDisplayButtonHilight(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

void intDisplayButtonFlash(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

void intAddLoopQuantity(WIDGET *psWidget, W_CONTEXT *psContext);

void intDisplayEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

bool DroidIsBuilding(DROID *Droid);
STRUCTURE *DroidGetBuildStructure(DROID *Droid);
bool DroidGoingToBuild(DROID *Droid);
BASE_STATS *DroidGetBuildStats(DROID *Droid);
iIMDShape *DroidGetIMD(DROID *Droid);

bool StructureIsManufacturingPending(STRUCTURE *structure);   ///< Returns true iff the structure is either manufacturing or on hold (even if not yet synchronised). (But ignores research.)
bool structureIsResearchingPending(STRUCTURE *structure);     ///< Returns true iff the structure is either researching or on hold (even if not yet synchronised). (But ignores manufacturing.)
bool StructureIsOnHoldPending(STRUCTURE *structure);          ///< Returns true iff the structure is on hold (even if not yet synchronised).
DROID_TEMPLATE *FactoryGetTemplate(FACTORY *Factory);

RESEARCH_FACILITY *StructureGetResearch(STRUCTURE *Structure);
FACTORY *StructureGetFactory(STRUCTURE *Structure);

bool StatIsStructure(BASE_STATS const *Stat);
iIMDShape *StatGetStructureIMD(BASE_STATS *Stat,UDWORD Player);
bool StatIsTemplate(BASE_STATS *Stat);
bool StatIsFeature(BASE_STATS const *Stat);

SDWORD StatIsComponent(BASE_STATS *Stat);
bool StatGetComponentIMD(BASE_STATS *Stat, SDWORD compID,iIMDShape **CompIMD,iIMDShape **MountIMD);

bool StatIsResearch(BASE_STATS *Stat);

/* Draws a stats bar for the design screen */
void intDisplayStatsBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
/* Draws a Template Power Bar for the Design Screen */
void intDisplayDesignPowerBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

// Widget callback function to play an audio track.
extern void WidgetAudioCallback(int AudioID);

//void intDisplayTransportButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
class IntTransportButton : public IntFancyButton
{
public:
	IntTransportButton(WIDGET *parent);

	virtual void display(int xOffset, int yOffset);

	void setObject(DROID *object) { psDroid = object; }

protected:
	DROID *psDroid;
};

/*draws blips on radar to represent Proximity Display*/
extern void drawRadarBlips(int radarX, int radarY, float pixSizeH, float pixSizeV);

/*Displays the proximity messages blips over the world*/
 void intDisplayProximityBlips(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

extern void intUpdateQuantitySlider(WIDGET *psWidget, W_CONTEXT *psContext);

void intDisplayResSubGroup(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

void intDisplayMissionClock(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

void intDisplayAllyIcon(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
void intDisplayAllyBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

void intSetShadowPower(int quantity);

#endif // __INCLUDED_SRC_INTDISPLAY_H__
