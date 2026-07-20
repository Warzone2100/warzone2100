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
/** @file
 *  Slider bar interface definitions.
 */

#ifndef __INCLUDED_LIB_WIDGET_SLIDER_H__
#define __INCLUDED_LIB_WIDGET_SLIDER_H__

#include "widget.h"
#include "widgbase.h"

class W_SLIDER : public WIDGET
{
public:
	typedef std::function<void(W_SLIDER &)> SliderOnChangeFunc;

public:
	W_SLIDER(W_SLDINIT const *init);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;
	void run(W_CONTEXT *psContext) override;
	void display(int xOffset, int yOffset) override;
	void setTip(std::string string) override;
	void setHelp(optional<WidgetHelp> help) override;
	void enable();
	void disable();
	bool isHighlighted() const;
	bool isEnabled() const;

	std::string getTip() override;
	WidgetHelp const * getHelp() const override;

	void addOnChange(const SliderOnChangeFunc& func);

	bool capturesMouseDrag(WIDGET_KEY) override;
	void mouseDragged(WIDGET_KEY, W_CONTEXT *start, W_CONTEXT *current) override;

	WSLD_ORIENTATION orientation;                   // The orientation of the slider
	UWORD		numStops;			// Number of stop positions on the slider
	UWORD		barSize;			// Thickness of slider bar
	UWORD		pos;				// Current stop position of the slider
	UWORD		state;				// Slider state
	std::string pTip;                           // Tool tip
private:
	optional<WidgetHelp> help;
	bool		isHandlingDrag = false;
	std::vector<SliderOnChangeFunc> onChangeFuncs;
private:
	void updateSliderFromMousePosition(W_CONTEXT*);
	void callOnChangeFuncs();
};

#endif // __INCLUDED_LIB_WIDGET_SLIDER_H__
