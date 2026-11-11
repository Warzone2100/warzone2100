// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file
 *  The About overlay screen
 */

#include "aboutscreen.h"

#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "../hci.h"
#include "../titleui/widgets/frontendimagebutton.h"
#include "../urlhelpers.h"
#include "../version.h"

constexpr int ABOUT_PANEL_W = 400;
constexpr int ABOUT_PADDING = 20;
constexpr int ABOUT_LINE_GAP = 4;
constexpr int ABOUT_SECTION_GAP = 14;
constexpr int ABOUT_BUTTON_GAP = 10;

static std::shared_ptr<W_SCREEN> aboutScreen;

class AboutPanel : public WIDGET
{
public:
	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		int x1 = x0 + width();
		int y1 = y0 + height();

		PIELIGHT backColor = WZCOL_TRANSPARENT_BOX;
		backColor.byte.a = std::max<UBYTE>(220, backColor.byte.a);
		pie_UniTransBoxFill(x0, y0, x1, y1, backColor);

		iV_Box(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 175));
	}
};

static std::string licenseOverviewURL()
{
	return "https://github.com/Warzone2100/warzone2100/blob/" + version_getUpstreamSourceRef() + "/README-LICENSING.md";
}

static void scheduleCloseAboutScreen()
{
	widgScheduleTask([]() {
		closeAboutScreen();
	});
}

static std::shared_ptr<W_LABEL> makeCenteredText(const WzString &text, iV_fonts font, PIELIGHT colour, int width)
{
	auto label = std::make_shared<W_LABEL>();
	label->setFontColour(colour);
	int height = label->setFormattedString(text, width, font);
	label->setGeometry(0, 0, width, height);
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setTransparentToMouse(true);
	return label;
}

static std::shared_ptr<WzFrontendImageButton> makeButton(const WzString &text)
{
	auto button = WzFrontendImageButton::make(nullopt);
	button->setString(text);
	button->FontID = font_regular;
	button->setPadding(10, 4);
	return button;
}

void showAboutScreen()
{
	if (aboutScreen)
	{
		return; // already showing
	}

	auto screen = W_SCREEN::make();

	auto rootForm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	rootForm->onClickedFunc = scheduleCloseAboutScreen;
	rootForm->onCancelPressed = scheduleCloseAboutScreen;
	screen->psForm->attach(rootForm);

	auto panel = std::make_shared<AboutPanel>();
	rootForm->attach(panel);

	const int contentWidth = ABOUT_PANEL_W - (ABOUT_PADDING * 2);
	int nextY = ABOUT_PADDING;

	// Attach a widget, horizontally-centered on the next line
	auto attachAt = [&](const std::shared_ptr<WIDGET> &widget, int gapAfter) {
		panel->attach(widget);
		widget->move((ABOUT_PANEL_W - widget->width()) / 2, nextY);
		nextY += widget->height() + gapAfter;
	};

	attachAt(makeCenteredText("Warzone 2100", font_medium_bold, WZCOL_TEXT_BRIGHT, contentWidth), ABOUT_LINE_GAP);

	attachAt(makeCenteredText(version_getFormattedVersionString(), font_regular, WZCOL_TEXT_MEDIUM, contentWidth), ABOUT_SECTION_GAP);

	auto siteLink = WzFrontendImageButton::make(nullopt);
	siteLink->setString("https://wz2100.net");
	siteLink->FontID = font_regular;
	siteLink->setPadding(0, 0);
	siteLink->setBorderDrawMode(WzFrontendImageButton::BorderDrawMode::Never);
	siteLink->setCustomTextColors(WZCOL_YELLOW, WZCOL_TEXT_BRIGHT);
	siteLink->setGeometry(0, 0, siteLink->idealWidth(), siteLink->idealHeight());
	siteLink->addOnClickHandler([](W_BUTTON&) {
		openURLInBrowser("https://wz2100.net/");
	});
	attachAt(siteLink, ABOUT_SECTION_GAP);

	// The copyright notices are not translated
	WzString copyrightNotices = WzString::fromUtf8(version_getProjectCopyrightNotice()) + ", " + WzString::fromUtf8(version_getOriginalReleaseCopyrightNotice());
	attachAt(makeCenteredText(copyrightNotices, font_regular, WZCOL_TEXT_BRIGHT, contentWidth), ABOUT_SECTION_GAP);

	WzString warranty = WzString::fromUtf8(_("This program comes with ABSOLUTELY NO WARRANTY."));
	warranty += "\n";
	warranty += WzString::fromUtf8(_("This is free software, and you are welcome to redistribute it under certain conditions:"));
	attachAt(makeCenteredText(warranty, font_regular, WZCOL_TEXT_MEDIUM, contentWidth), ABOUT_SECTION_GAP);

	// "View License" and "Close" buttons (centered)
	auto licenseButton = makeButton(_("View License"));
	auto closeButton = makeButton(_("Close"));
	licenseButton->addOnClickHandler([](W_BUTTON&) {
		openURLInBrowser(licenseOverviewURL().c_str());
	});
	closeButton->addOnClickHandler([](W_BUTTON&) {
		scheduleCloseAboutScreen();
	});
	const int buttonHeight = std::max(closeButton->idealHeight(), licenseButton->idealHeight());
	int buttonX = (ABOUT_PANEL_W - (licenseButton->idealWidth() + ABOUT_BUTTON_GAP + closeButton->idealWidth())) / 2;
	for (const auto &button : {licenseButton, closeButton})
	{
		panel->attach(button);
		button->setGeometry(buttonX, nextY, button->idealWidth(), buttonHeight);
		buttonX += button->idealWidth() + ABOUT_BUTTON_GAP;
	}
	nextY += buttonHeight + ABOUT_PADDING;

	const int panelHeight = nextY;
	panel->setCalcLayout([panelHeight](WIDGET *psWidget) {
		psWidget->setGeometry((static_cast<int>(screenWidth) - ABOUT_PANEL_W) / 2, (static_cast<int>(screenHeight) - panelHeight) / 2, ABOUT_PANEL_W, panelHeight);
	});

	widgRegisterOverlayScreenOnTopOfScreen(screen, psWScreen);
	aboutScreen = screen;
}

void closeAboutScreen()
{
	if (aboutScreen)
	{
		widgRemoveOverlayScreen(aboutScreen);
		aboutScreen = nullptr;
	}
}
