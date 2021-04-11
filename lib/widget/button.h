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
 *  Definitions for edit box functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_BUTTON_H__
#define __INCLUDED_LIB_WIDGET_BUTTON_H__

#include "lib/ivis_opengl/ivisdef.h"

#include "widget.h"
#include "widgbase.h"
#include <map>
#include <functional>
#include <string>


class W_BUTTON : public WIDGET
{

public:
	struct Images
	{
		Images() {}
		Images(Image normal, Image down, Image highlighted, Image disabled = Image()) : normal(normal), down(down), highlighted(highlighted), disabled(disabled) {}

		Image normal;       ///< The image for the button.
		Image down;         ///< The image for the button, when down. Is overlaid over image.
		Image highlighted;  ///< The image for the button, when highlighted. Is overlaid over image.
		Image disabled;     ///< The image for the button, when disabled. Is overlaid over image.
	};

public:
	W_BUTTON(W_BUTINIT const *init);
	W_BUTTON();

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;
	void display(int xOffset, int yOffset) override;
	void displayRecursive(WidgetGraphicsContext const &context) override; // for handling progress border overlay

	unsigned getState() override;
	void setState(unsigned state) override;
	void setFlash(bool enable) override;
	WzString getString() const override;
	void setString(WzString string) override;
	void setTip(std::string string) override;
	void unlock();

	void setImages(Images const &images);
	void setImages(Image image, Image imageDown, Image imageHighlight, Image imageDisabled = Image());

	using WIDGET::setString;
	using WIDGET::setTip;

	std::string getTip() override
	{
		return pTip;
	}

	/* The optional "onClick" callback function */
	typedef std::function<void (W_BUTTON& button)> W_BUTTON_ONCLICK_FUNC;

	void addOnClickHandler(const W_BUTTON_ONCLICK_FUNC& onClickFunc);

	struct ProgressBorder
	{
	public:
		struct BorderInset
		{
			BorderInset() { }
			BorderInset(int left, int top, int right, int bottom)
			: left(left), top(top), right(right), bottom(bottom)
			{ }
			int left = 0;
			int top = 0;
			int right = 0;
			int bottom = 0;
		};
	private:
		ProgressBorder(UDWORD interval, optional<float> factor, bool repeated, BorderInset inset = BorderInset()): m_inset(inset), m_interval(interval), m_factor(factor), m_repeated(repeated) { resetStartingTime(); }
	public:
		// Create an indeterminate ProgressBorder
		static ProgressBorder indeterminate(BorderInset inset = BorderInset()) { return ProgressBorder(0, nullopt, true, inset); }
		// Create a timed ProgressBorder which proceeds from 0 to 100% over the interval
		static ProgressBorder timed(UDWORD interval, bool repeated = false, BorderInset inset = BorderInset()) { return ProgressBorder(interval, nullopt, repeated, inset); }
		// Create a fixed factor ProgressBorder, where `factor` is expected to be a float from 0.0 to 1.0 (which corresponds to the percentage progress)
		static ProgressBorder fixedFactor(float factor, BorderInset inset = BorderInset()) { return ProgressBorder(0, factor, false, inset); }
	public:
		void setInset(const BorderInset& newInset) { m_inset = newInset; }
		void resetStartingTime();
		const BorderInset& inset() const { return m_inset; }
		UDWORD interval() const { return m_interval; }
		UDWORD startingTime() const { return m_startingTime; }
		optional<float> factor() const { return m_factor; }
		bool repeated() const { return m_repeated; }

		bool isIndeterminate() const { return !m_factor.has_value() && m_interval == 0; }
	private:
		BorderInset m_inset;
		UDWORD m_interval = 0;
		UDWORD m_startingTime = 0;
		optional<float> m_factor = 0.0f;
		bool m_repeated = false;
	};

	// Set the current ProgressBorder on the button, optionally specifying a color
	// To disable a ProgressBorder, use `setProgressBorder(nullopt)`
	void setProgressBorder(optional<ProgressBorder> progressBorder, optional<PIELIGHT> borderColour = nullopt);

public:
	bool isHighlighted() const;

private:
	void drawProgressBorder(int xOffset, int yOffset);

public:
	unsigned        state;                          // The current button state
	WzString        pText;                          // The text for the button
	Images          images;                         ///< The images for the button.
	std::string     pTip;                           // The tool tip for the button
	SWORD HilightAudioID;				// Audio ID for form clicked sound
	SWORD ClickedAudioID;				// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;	// Pointer to audio callback function
	iV_fonts        FontID;
	UDWORD minClickInterval = 0;
private:
	UDWORD lastClickTime = 0;
	std::vector<W_BUTTON_ONCLICK_FUNC> onClickHandlers;
	optional<ProgressBorder> progressBorder;
	PIELIGHT				 progressBorderColour;
};

class MultipleChoiceButton : public W_BUTTON
{

public:
	MultipleChoiceButton() : W_BUTTON(), choice(0) {}
	void setChoice(unsigned newChoice);
	void setTip(unsigned stateValue, std::string const &string);
	void setTip(unsigned stateValue, char const *stringUtf8);
	void setImages(unsigned stateValue, Images const &stateImages);
	unsigned getChoice()
	{
		return choice;
	}

	using WIDGET::setTip;

private:
	unsigned choice;
	std::map<int, std::string> tips;
	std::map<int, Images> imageSets;
};

#endif // __INCLUDED_LIB_WIDGET_BUTTON_H__
