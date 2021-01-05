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
 *  Functions for the button widget
 */

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "button.h"
#include "form.h"
#include "tip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/gamelib/gtime.h"


W_BUTINIT::W_BUTINIT()
	: pText(nullptr)
	, FontID(font_regular)
{}

W_BUTTON::W_BUTTON(W_BUTINIT const *init)
	: WIDGET(init, WIDG_BUTTON)
	, state(WBUT_PLAIN)
	, pText(WzString::fromUtf8(init->pText))
	, pTip(init->pTip)
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
	, FontID(init->FontID)
{
	ASSERT((init->style & ~(WBUT_PLAIN | WIDG_HIDDEN | WBUT_NOPRIMARY | WBUT_SECONDARY | WBUT_TXTCENTRE)) == 0, "unknown button style");
}

W_BUTTON::W_BUTTON()
	: WIDGET(WIDG_BUTTON)
	, state(WBUT_PLAIN)
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
	, FontID(font_regular)
{}

unsigned W_BUTTON::getState()
{
	return state & (WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK | WBUT_FLASH | WBUT_DOWN | WBUT_HIGHLIGHT);
}

void W_BUTTON::setFlash(bool enable)
{
	dirty = true;
	if (enable)
	{
		state |= WBUT_FLASH;
	}
	else
	{
		state &= ~WBUT_FLASH;
	}
}

void W_BUTTON::unlock()
{
	dirty = true;
	state &= ~(WBUT_LOCK | WBUT_CLICKLOCK);
}

void W_BUTTON::setState(unsigned newState)
{
	ASSERT(!((newState & WBUT_LOCK) && (newState & WBUT_CLICKLOCK)), "Cannot have both WBUT_LOCK and WBUT_CLICKLOCK");

	unsigned mask = WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK;
	state = (state & ~mask) | (newState & mask);
	dirty = true;
}

WzString W_BUTTON::getString() const
{
	return pText;
}

void W_BUTTON::setString(WzString string)
{
	pText = string;
	dirty = true;
}

void W_BUTTON::setTip(std::string string)
{
	pTip = string;
}

void W_BUTTON::clicked(W_CONTEXT *, WIDGET_KEY key)
{
	if ((minClickInterval > 0) && (realTime - lastClickTime < minClickInterval))
	{
		return;
	}
	lastClickTime = realTime;

	dirty = true;

	/* Can't click a button if it is disabled or locked down */
	if ((state & (WBUT_DISABLE | WBUT_LOCK)) == 0)
	{
		// Check this is the correct key
		if ((!(style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			if (AudioCallback)
			{
				AudioCallback(ClickedAudioID);
			}
			state &= ~WBUT_FLASH;	// Stop it flashing
			state |= WBUT_DOWN;
		}
	}

	/* Kill the tip if there is one */
	if (!pTip.empty())
	{
		tipStop(this);
	}
}

/* Respond to a mouse button up */
void W_BUTTON::released(W_CONTEXT *, WIDGET_KEY key)
{
	if (state & WBUT_DOWN)
	{
		// Check this is the correct key
		if ((!(style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			/* Call all onClick event handlers */
			for (auto it = onClickHandlers.begin(); it != onClickHandlers.end(); it++)
			{
				auto onClickHandler = *it;
				if (onClickHandler)
				{
					onClickHandler(*this);
				}
			}

			if (auto lockedScreen = screenPointer.lock())
			{
				lockedScreen->setReturn(shared_from_this());
			}
			state &= ~WBUT_DOWN;
			dirty = true;
		}
	}
}

bool W_BUTTON::isHighlighted() const
{
	return (state & WBUT_HIGHLIGHT);
}

/* Respond to a mouse moving over a button */
void W_BUTTON::highlight(W_CONTEXT *psContext)
{
	if ((state & WBUT_HIGHLIGHT) == 0)
	{
		state |= WBUT_HIGHLIGHT;
		dirty = true;
	}
	if (AudioCallback)
	{
		AudioCallback(HilightAudioID);
	}

	/* If there is a tip string start the tool tip */
	if (!pTip.empty())
	{
		if (auto lockedScreen = screenPointer.lock())
		{
			tipStart(this, pTip, lockedScreen->TipFontID, x() + psContext->xOffset, y() + psContext->yOffset, width(), height());
		}
	}
}


/* Respond to the mouse moving off a button */
void W_BUTTON::highlightLost()
{
	state &= ~(WBUT_DOWN | WBUT_HIGHLIGHT);
	dirty = true;
	if (!pTip.empty())
	{
		tipStop(this);
	}
}

void W_BUTTON::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (state & WBUT_DISABLE) != 0;
	bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

	// Display the button.
	if (!images.normal.isNull())
	{
		iV_DrawImage(images.normal, x0, y0);
		if (isDown && !images.down.isNull())
		{
			iV_DrawImage(images.down, x0, y0);
		}
		if (isDisabled && !images.disabled.isNull())
		{
			iV_DrawImage(images.disabled, x0, y0);
		}
		if (isHighlight && !images.highlighted.isNull())
		{
			iV_DrawImage(images.highlighted, x0, y0);
		}
	}
	else
	{
		iV_ShadowBox(x0, y0, x1, y1, 0, WZCOL_FORM_LIGHT, isDisabled ? WZCOL_FORM_LIGHT : WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
		if (isHighlight)
		{
			iV_Box(x0 + 2, y0 + 2, x1 - 3, y1 - 3, WZCOL_FORM_HILITE);
		}
	}

	if (haveText)
	{
		int fw = iV_GetTextWidth(pText.toUtf8().c_str(), FontID);
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - iV_GetTextLineSize(FontID)) / 2 - iV_GetTextAboveBase(FontID);
		if (isDisabled)
		{
			iV_SetTextColour(WZCOL_FORM_LIGHT);
			iV_DrawText(pText.toUtf8().c_str(), fx + 1, fy + 1, FontID);
			iV_SetTextColour(WZCOL_FORM_DISABLE);
		}
		else
		{
			iV_SetTextColour(WZCOL_FORM_TEXT);
		}
		iV_DrawText(pText.toUtf8().c_str(), fx, fy, FontID);
	}

	if (isDisabled && !images.normal.isNull() && images.disabled.isNull())
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
	}
}

void W_BUTTON::displayRecursive(WidgetGraphicsContext const &context)
{
	// call parent displayRecursive
	WIDGET::displayRecursive(context);

	if (progressBorder.has_value())
	{
		// "over-draw" with the progress border
		drawProgressBorder(context.getXOffset(), context.getYOffset());
	}
}

void W_BUTTON::drawProgressBorder(int xOffset, int yOffset)
{
	ASSERT_OR_RETURN(, progressBorder.has_value(), "progressBorder is nullopt");
	auto& config = progressBorder.value();

	// Determine total rect to be used for the progress border
	int x0 = geometry().x() + xOffset;
	int y0 = geometry().y() + yOffset;
	int widthReduction = config.inset().left + config.inset().right;
	int heightReduction = config.inset().top + config.inset().bottom;
	WzRect progressBorderRect = WzRect(x0 + config.inset().left, y0 + config.inset().top, geometry().width() - widthReduction, geometry().height() - heightReduction);

	// Determine border lines
	// THERE. ARE. 5. LINES!!! (because the "top" line is split in two - the starting point is the middle of the top line, and proceeds clockwise)
	std::vector<glm::ivec4> borderLines; // x, y, z, w
	std::vector<int> lengthOfEachLine;
	borderLines.push_back({progressBorderRect.x() + (progressBorderRect.width() / 2), progressBorderRect.y(), progressBorderRect.x() + progressBorderRect.width(), progressBorderRect.y()});
	lengthOfEachLine.push_back(borderLines.back().z - borderLines.back().x);
	borderLines.push_back({progressBorderRect.x() + progressBorderRect.width(), progressBorderRect.y(), progressBorderRect.x() + progressBorderRect.width(), progressBorderRect.y() + progressBorderRect.height()});
	lengthOfEachLine.push_back(borderLines.back().w - borderLines.back().y);
	borderLines.push_back({progressBorderRect.x() + progressBorderRect.width(), progressBorderRect.y() + progressBorderRect.height(), progressBorderRect.x(), progressBorderRect.y() + progressBorderRect.height()});
	lengthOfEachLine.push_back(progressBorderRect.width());
	borderLines.push_back({progressBorderRect.x(), progressBorderRect.y() + progressBorderRect.height(), progressBorderRect.x(), progressBorderRect.y()});
	lengthOfEachLine.push_back(lengthOfEachLine[1]);
	borderLines.push_back({progressBorderRect.x(), progressBorderRect.y(), progressBorderRect.x() + (progressBorderRect.width() / 2) + 1, progressBorderRect.y()});
	lengthOfEachLine.push_back(borderLines.back().z - borderLines.back().x);
	size_t realLineCount = borderLines.size();
	int totalLength = 0;
	for (const auto& lineLength : lengthOfEachLine)
	{
		totalLength += lineLength;
	}
	const int indeterminateMaxLineLength = static_cast<int>((float)totalLength / 4.f);
	if (config.isIndeterminate())
	{
		// "Repeat" indeterminate mode (progress goes all the way to 0 and restarts)
		// Increase totalLength by indeterminateMaxLineLength, and add a "fake" line at the end of the borderLine list that we don't actually draw
		totalLength += indeterminateMaxLineLength;
		borderLines.push_back({borderLines[0].x, borderLines[0].y, borderLines[0].x + indeterminateMaxLineLength, borderLines[0].w});
		lengthOfEachLine.push_back(indeterminateMaxLineLength);
	}
	UDWORD timeElapsed = (realTime - config.startingTime());

	float factor = 0.0f;
	if (config.factor().has_value())
	{
		factor = config.factor().value();
	}
	else if (config.interval() != 0)
	{
		factor = (float)timeElapsed / (float)config.interval();
	}
	else
	{
		// Indeterminate mode
		const int FIXED_INDETERMINATE_MODE_INTERVAL = 4 * GAME_TICKS_PER_SEC;
		factor = (float)timeElapsed / (float)FIXED_INDETERMINATE_MODE_INTERVAL;
	}

	// Get length of progress
	int progressLength = static_cast<int>(factor * (float)totalLength) % totalLength;

	if (!config.factor().has_value() && !config.repeated() && timeElapsed >= config.interval())
	{
		// Done - draw once at full, and then stop drawing in the future
		progressLength = totalLength;
		progressBorder = nullopt;
	}

	// Decide what parts of what lines to draw
	int remainingLength = progressLength;
	std::vector<glm::ivec4> linesToDraw;
	for (size_t lineIdx = 0; lineIdx < borderLines.size() && remainingLength > 0; lineIdx++)
	{
		int lineLength = lengthOfEachLine[lineIdx];
		if (remainingLength >= lineLength)
		{
			linesToDraw.push_back(borderLines[lineIdx]);
			remainingLength -= lineLength;
		}
		else
		{
			// Last line - partial line - reduce end-point
			auto v = borderLines[lineIdx];
			bool isVerticalLine = lineIdx % 2;
			int extraLength = lineLength - remainingLength;
			if (!isVerticalLine)
			{
				if (v[2] > v[0])
				{
					v[2] -= extraLength;
				}
				else
				{
					v[2] += extraLength;
				}
			}
			else
			{
				if (v[3] > v[1])
				{
					v[3] -= extraLength;
				}
				else
				{
					v[3] += extraLength;
				}
			}
			linesToDraw.push_back(v);
			lengthOfEachLine[lineIdx] = remainingLength; // update line length (for indeterminate mode case, below)
			remainingLength = 0;
		}
	}

	if (config.isIndeterminate() && !linesToDraw.empty())
	{
		// In indeterminate mode, start from the end and remove any lines beyond the indeterminateMaxLineLength
		remainingLength = indeterminateMaxLineLength;
		int lineIdx = static_cast<int>(linesToDraw.size() - 1);
		for (; lineIdx >= 0; lineIdx--)
		{
			int lineLength = lengthOfEachLine[lineIdx];
			if (remainingLength >= lineLength)
			{
				remainingLength -= lineLength;
			}
			else
			{
				// Shorten this line by adjusting its *starting point*
				auto v = linesToDraw[lineIdx];
				bool isVerticalLine = lineIdx % 2;
				int extraLength = lineLength - remainingLength;
				if (!isVerticalLine)
				{
					if (v[2] > v[0])
					{
						v[0] += extraLength;
					}
					else
					{
						v[0] -= extraLength;
					}
				}
				else
				{
					if (v[3] > v[1])
					{
						v[1] += extraLength;
					}
					else
					{
						v[1] -= extraLength;
					}
				}
				linesToDraw[lineIdx] = v;
				remainingLength = 0;
				break;
			}
		}
		// "Repeat" indeterminate mode: remove the last line *if* if it's the "fake" line for indeterminate mode calculations)
		if (linesToDraw.size() > realLineCount)
		{
			linesToDraw.pop_back();
		}
		// Remove any lines earlier in the list than lineIdx
		if (lineIdx > 0)
		{
			linesToDraw.erase(linesToDraw.begin(), linesToDraw.begin() + lineIdx);
		}
	}

	if (!linesToDraw.empty())
	{
		// Draw the lines
		iV_Lines(linesToDraw, progressBorderColour);
	}
}

void W_BUTTON::ProgressBorder::resetStartingTime()
{
	m_startingTime = realTime;
}

void W_BUTTON::setImages(Images const &images_)
{
	images = images_;
	dirty = true;
	if (!images.normal.isNull())
	{
		setGeometry(x(), y(), images.normal.width(), images.normal.height());
	}
}

void W_BUTTON::setImages(Image image, Image imageDown, Image imageHighlight, Image imageDisabled)
{
	dirty = true;
	setImages(Images(image, imageDown, imageHighlight, imageDisabled));
}

void W_BUTTON::addOnClickHandler(const W_BUTTON_ONCLICK_FUNC& onClickFunc)
{
	onClickHandlers.push_back(onClickFunc);
}

void MultipleChoiceButton::setChoice(unsigned newChoice)
{
	if (choice == newChoice)
	{
		return;
	}
	dirty = true;
	choice = newChoice;
	std::map<int, Images>::const_iterator image = imageSets.find(choice);
	if (image != imageSets.end())
	{
		W_BUTTON::setImages(image->second);
	}
	std::map<int, std::string>::const_iterator tip = tips.find(choice);
	if (tip != tips.end())
	{
		W_BUTTON::setTip(tip->second);
	}
}

void MultipleChoiceButton::setTip(unsigned choiceValue, const std::string& string)
{
	tips[choiceValue] = string;
	if (choice == choiceValue)
	{
		W_BUTTON::setTip(string);
	}
}

void MultipleChoiceButton::setTip(unsigned choiceValue, char const *stringUtf8)
{
	setTip(choiceValue, stringUtf8 != nullptr? std::string(stringUtf8) : std::string());
}

void MultipleChoiceButton::setImages(unsigned choiceValue, Images const &stateImages)
{
	imageSets[choiceValue] = stateImages;
	dirty = true;
	if (choice == choiceValue)
	{
		W_BUTTON::setImages(stateImages);
	}
}

void W_BUTTON::setProgressBorder(optional<ProgressBorder> _progressBorder, optional<PIELIGHT> _borderColour)
{
	progressBorder = _progressBorder;
	progressBorderColour = (_borderColour.has_value()) ? _borderColour.value() : WZCOL_FORM_HILITE;
}
