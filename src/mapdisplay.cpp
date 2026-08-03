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
	MapDisplay - Renders the world view necessary for the intelligence map
	Alex McLean, Pumpkin Studios, EIDOS Interactive, 1997

	Makes heavy use of the functions available in display3d.c. Could have
	messed about with display3d.c to make to world render dual purpose, but
	it's neater as a separate file, as the intelligence map has special requirements
	and overlays and needs to render to a specified buffer for later use.
*/

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piematrix.h"

#include "component.h"
#include "intdisplay.h"
#include "mapdisplay.h"

#define ROTATE_TIME	(2*GAME_TICKS_PER_SEC)
#define PREVIEW_ROTATE_TIME	(3*GAME_TICKS_PER_SEC)

/* renders the Research IMDs into the surface - used by message display in
Intelligence Map */
void renderResearchToBuffer(RESEARCH *psResearch, UDWORD OriginX, UDWORD OriginY, bool rotate)
{
	BASE_STATS      *psResGraphic;
	UDWORD          IMDType;
	SDWORD          scale = 0;

	if (psResearch == nullptr)
	{
		return;
	}

	pie_SetGeometricOffset(OriginX, OriginY + 10);

	// Rotate round
	// full rotation once every 2 seconds..
	const int angle = (rotate) ? ((realTime % ROTATE_TIME) * 360 / ROTATE_TIME) : 45;
	Vector3i Position = Vector3i( 0, 0, BUTTON_DEPTH );
	Vector3i Rotation = Vector3i( -30, angle, 0 );

	//draw the IMD for the research
	if (psResearch->psStat)
	{
		//we have a Stat associated with this research topic
		if (StatIsStructure(psResearch->psStat))
		{
			//this defines how the button is drawn
			IMDType = IMDTYPE_STRUCTURESTAT;
			psResGraphic = psResearch->psStat;
			//set up the scale
			unsigned basePlateSize = getStructureStatSizeMax((STRUCTURE_STATS *)psResearch->psStat);
			if (basePlateSize == 1)
			{
				scale = RESEARCH_COMPONENT_SCALE / 2;
				/*HACK HACK HACK!
				if its a 'tall thin (ie tower)' structure stat with something on
				the top - offset the position to show the object on top*/
				const iIMDShape *pDisplayModel = ((STRUCTURE_STATS *)psResearch->psStat)->pIMD[0]->displayModel();
				if (!pDisplayModel->connectors.empty() &&
				    getStructureStatHeight((STRUCTURE_STATS *)psResearch->psStat) > TOWER_HEIGHT)
				{
					Position.y -= 30;
				}
			}
			else if (basePlateSize == 2)
			{
				scale = RESEARCH_COMPONENT_SCALE / 4;
			}
			else
			{
				scale = RESEARCH_COMPONENT_SCALE / 5;
			}
		}
		else
		{
			unsigned compID = StatIsComponent(psResearch->psStat);
			if (compID != COMP_NUMCOMPONENTS)
			{
				//this defines how the button is drawn
				IMDType = IMDTYPE_COMPONENT;
				psResGraphic = psResearch->psStat;
				// FIXME: Another kludge to deal with the superTransport to make it "fit" the display.
				if (psResearch->id.compare("R-SuperTransport") == 0)
				{
					scale = RESEARCH_COMPONENT_SCALE / 3;
				}
				else if (psResearch->id.compare("R-Cyborg-Transport") == 0)
				{
					scale = RESEARCH_COMPONENT_SCALE / 2;
				}
				else
				{
					scale = RESEARCH_COMPONENT_SCALE;
				}
			}
			else
			{
				ASSERT(false, "intDisplayMessageButton: invalid stat");
				IMDType = IMDTYPE_RESEARCH;
				psResGraphic = (BASE_STATS *)psResearch;
			}
		}
	}
	else
	{
		//no Stat for this research topic so use the research topic to define what is drawn
		psResGraphic = (BASE_STATS *)psResearch;
		IMDType = IMDTYPE_RESEARCH;
	}

	//scale the research according to size of IMD
	if (IMDType == IMDTYPE_RESEARCH)
	{
		unsigned Radius = getResearchRadius((BASE_STATS *)psResGraphic);
		if (Radius <= 100)
		{
			scale = RESEARCH_COMPONENT_SCALE / 2;
		}
		else if (Radius <= 128)
		{
			scale = RESEARCH_COMPONENT_SCALE / 3;
		}
		else if (Radius <= 256)
		{
			scale = RESEARCH_COMPONENT_SCALE / 4;
		}
		else
		{
			scale = RESEARCH_COMPONENT_SCALE / 5;
		}
	}

	/* display the IMDs */
	if (IMDType == IMDTYPE_COMPONENT)
	{
		displayComponentButton(psResGraphic, &Rotation, &Position, scale);
	}
	else if (IMDType == IMDTYPE_RESEARCH)
	{
		displayResearchButton(psResGraphic, &Rotation, &Position, scale);
	}
	else if (IMDType == IMDTYPE_STRUCTURESTAT)
	{
		displayStructureStatButton((STRUCTURE_STATS *)psResGraphic, &Rotation, &Position, scale);
	}
	else
	{
		ASSERT(false, "Unknown PIEType");
	}
}

// Buttons fit a radius-64 model into roughly 60 pixels at scale 100
#define RESEARCH_FIT_REFERENCE_BOX 60
// The radius below which the buttons treat a research model like a component
#define SMALL_RESEARCH_RADIUS 100

static void resolveResearchFit(RESEARCH *psResearch, UDWORD boxSize, BASE_STATS **psResGraphicOut, UDWORD *IMDTypeOut, UDWORD *radiusOut, SDWORD *scaleOut)
{
	BASE_STATS *psResGraphic = nullptr;
	UDWORD IMDType = IMDTYPE_RESEARCH;
	UDWORD radius = COMPONENT_RADIUS;

	if (psResearch->psStat != nullptr)
	{
		psResGraphic = psResearch->psStat;
		if (StatIsStructure(psResearch->psStat))
		{
			IMDType = IMDTYPE_STRUCTURESTAT;
			const iIMDShape *pDisplayModel = safeGetDisplayModelFromBase(((STRUCTURE_STATS *)psResearch->psStat)->pIMD[0]);
			radius = (pDisplayModel != nullptr) ? static_cast<UDWORD>(pDisplayModel->sradius) : OBJECT_RADIUS;
		}
		else if (StatIsComponent(psResearch->psStat) != COMP_NUMCOMPONENTS)
		{
			IMDType = IMDTYPE_COMPONENT;
			radius = getComponentRadius(psResearch->psStat);
		}
		else
		{
			psResGraphic = (BASE_STATS *)psResearch;
			radius = getResearchRadius((BASE_STATS *)psResearch);
		}
	}
	else
	{
		psResGraphic = (BASE_STATS *)psResearch;
		radius = getResearchRadius((BASE_STATS *)psResearch);
	}

	if (radius == 0)
	{
		radius = COMPONENT_RADIUS;
	}
	const SDWORD targetRadius = static_cast<SDWORD>(COMPONENT_RADIUS * boxSize / RESEARCH_FIT_REFERENCE_BOX);

	// Averaging against a flat scale keeps a small model from being blown up to
	// fill the box, but it also puts a floor under the result. So it applies to a
	// model small enough to need lifting and to nothing else.
	//
	// Anything bigger has to be allowed under that floor or it comes out oversized
	// however small the box: a structure at a radius of 285 against a component's
	// 50, and a transport body at more again.
	const bool softenAgainstFlatScale = (radius <= SMALL_RESEARCH_RADIUS);
	const SDWORD flatScale = softenAgainstFlatScale
		? static_cast<SDWORD>(COMP_BUT_SCALE * boxSize / RESEARCH_FIT_REFERENCE_BOX)
		: 0;

	*psResGraphicOut = psResGraphic;
	*IMDTypeOut = IMDType;
	*radiusOut = radius;
	*scaleOut = rescaleButtonObject(static_cast<SDWORD>(radius), flatScale, targetRadius);
}

void renderResearchFittedToBuffer(RESEARCH *psResearch, UDWORD OriginX, UDWORD OriginY, UDWORD boxSize, bool rotate)
{
	if (psResearch == nullptr || boxSize == 0)
	{
		return;
	}

	pie_SetGeometricOffset(OriginX, OriginY + static_cast<int>(boxSize) / 8);

	const int angle = (rotate) ? static_cast<int>((realTime % PREVIEW_ROTATE_TIME) * 360 / PREVIEW_ROTATE_TIME) : 45;
	Vector3i Position = Vector3i(0, 0, BUTTON_DEPTH);
	Vector3i Rotation = Vector3i(-30, angle, 0);

	BASE_STATS *psResGraphic = nullptr;
	UDWORD IMDType = IMDTYPE_RESEARCH;
	UDWORD radius = 0;
	SDWORD scale = 0;
	resolveResearchFit(psResearch, boxSize, &psResGraphic, &IMDType, &radius, &scale);

	if (IMDType == IMDTYPE_COMPONENT)
	{
		displayComponentButton(psResGraphic, &Rotation, &Position, scale);
	}
	else if (IMDType == IMDTYPE_RESEARCH)
	{
		displayResearchButton(psResGraphic, &Rotation, &Position, scale);
	}
	else
	{
		displayStructureStatButton((STRUCTURE_STATS *)psResGraphic, &Rotation, &Position, scale);
	}
}
