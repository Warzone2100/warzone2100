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
#ifndef __INCLUDED_INTIMAGE__
#define __INCLUDED_INTIMAGE__

#include "intfac.h" // Interface image id's.
#include "lib/widget/listwidget.h"
#include "lib/ivis_opengl/pieblitfunc.h"

#define FILLRED 16
#define FILLGREEN 16
#define FILLBLUE 128
#define FILLTRANS 128

/** Frame type */
enum FRAMETYPE
{
	FRAME_NORMAL, FRAME_RADAR
};

struct TABDEF
{
	SWORD MajorUp;			//< Index of image to use for tab not pressed.
	SWORD MajorDown;		//< Index of image to use for tab pressed.
	SWORD MajorHilight;		//< Index of image to use for tab hilighted by mouse.
	SWORD MajorSelected;		//< Index of image to use for tab selected (same as pressed).
};

/// A list's page tabs as in the game's panels: the tab images in three sizes (the smallest with scroll buttons), drawn
/// by the HUD skin
void intSetupPageTabs(TabSelectionWidget &tabs, TabAlignment tabAlignment);
/// Page tabs drawn by the HUD skin (the HUD's, others keep the Classic tabs)
void intSetHudPageTabs(TabSelectionWidget &tabs);

class IntListTabWidget : public ListTabWidget
{
protected:
	IntListTabWidget(): ListTabWidget() {}
	virtual void initialize() override;

public:
	static std::shared_ptr<IntListTabWidget> make(TabAlignment tabAlignment = TabAlignment::LeftAligned)
	{
		class make_shared_enabler: public IntListTabWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->tabAlignment = tabAlignment;
		widget->initialize();
		return widget;
	}

	/// Its page tabs drawn by the HUD skin (the HUD's lists, others keep the Classic tabs)
	void setHudSkinned();
	/// No row for the page tabs: the list takes all of its height (something else shows the pages)
	void setTabRowHidden();
	void displayRecursive(WidgetGraphicsContext const &context) override;
	void geometryChanged() override;

protected:
	void pageTurnedByTouch(size_t from, size_t to) override;

private:
	TabAlignment tabAlignment = TabAlignment::LeftAligned;
	bool hudSkinned = false;
	bool tabRowHidden = false;
	nonstd::optional<uint32_t> pageTurnedAt;  ///< (a swipe's page, coming in)
};

extern IMAGEFILE *IntImages;	//< All the 2d graphics for the user interface.

bool imageInitBitmaps();

/** Draws a transparent window. */
void RenderWindowFrame(FRAMETYPE frame, uint32_t x, uint32_t y, uint32_t Width, uint32_t Heig, const glm::mat4 &modelViewProjection = defaultProjectionMatrix());

#endif
