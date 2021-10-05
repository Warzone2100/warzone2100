#pragma once

#include "videooptions.h"
#include "screenresolutionmodel.h"

#include "lib/ivis_opengl/piestate.h"

#include "lib/widget/dropdown.h"
#include "lib/widget/gridlayout.h"

#include "../../warzoneconfig.h" // war_Get*
#include "../../texture.h" // texture functions


// ////////////////////////////////////////////////////////////////////////////
// Video Options

void refreshCurrentVideoOptionsValues()
{
	widgSetString(psWScreen, FRONTEND_WINDOWMODE_R, videoOptionsWindowModeString());
	widgSetString(psWScreen, FRONTEND_FSAA_R, videoOptionsAntialiasingString().c_str());
	if (widgGetFromID(psWScreen, FRONTEND_RESOLUTION_READONLY)) // Resolution option may not be available
	{
		widgSetString(psWScreen, FRONTEND_RESOLUTION_READONLY, ScreenResolutionsModel::currentResolutionString().c_str());
		if (war_getWindowMode() != WINDOW_MODE::fullscreen)
		{
			// If live window resizing is supported & the current mode is "windowed", disable the Resolution option and add a tooltip
			// explaining the user can now resize the window normally.
			videoOptionsDisableResolutionButtons();
		}
		else
		{
			videoOptionsEnableResolutionButtons();
		}
	}
	widgSetString(psWScreen, FRONTEND_TEXTURESZ_R, videoOptionsTextureSizeString().c_str());
	widgSetString(psWScreen, FRONTEND_VSYNC_R, videoOptionsVsyncString());
	if (widgGetFromID(psWScreen, FRONTEND_DISPLAYSCALE_R)) // Display Scale option may not be available
	{
		widgSetString(psWScreen, FRONTEND_DISPLAYSCALE_R, videoOptionsDisplayScaleString().c_str());
	}
}

std::shared_ptr<WIDGET> makeResolutionDropdown()
{
	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_RESOLUTION_DROPDOWN;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * 5);
	const auto paddingSize = 10;

	ScreenResolutionsModel screenResolutionsModel;
	for (auto resolution : screenResolutionsModel)
	{
		auto item = makeTextButton(0, ScreenResolutionsModel::resolutionString(resolution), 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
	}

	auto closestResolution = screenResolutionsModel.findResolutionClosestToCurrent();
	if (closestResolution != screenResolutionsModel.end())
	{
		dropdown->setSelectedIndex(closestResolution - screenResolutionsModel.begin());
	}

	dropdown->setOnChange([screenResolutionsModel](DropdownWidget& dropdown) {
		if (auto selectedIndex = dropdown.getSelectedIndex())
		{
			screenResolutionsModel.selectAt(selectedIndex.value());
		}
		});

	return Margin(0, -paddingSize).wrap(dropdown);
}

void startVideoOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	// Add a note about changes taking effect on restart for certain options
	WIDGET* parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto label = std::make_shared<W_LABEL>();
	parent->attach(label);
	label->setGeometry(FRONTEND_POS1X + 48, FRONTEND_POS1Y, FRONTEND_BUTWIDTH - FRONTEND_POS1X - 48, FRONTEND_BUTHEIGHT);
	label->setFontColour(WZCOL_TEXT_BRIGHT);
	label->setString(_("* Takes effect on game restart"));
	label->setTextAlignment(WLAB_ALIGNBOTTOMLEFT);

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	// Fullscreen/windowed
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_WINDOWMODE, videoOptionsWindowModeLabel(), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_WINDOWMODE_R, videoOptionsWindowModeString(), WBUT_SECONDARY)));
	row.start++;

	// Resolution
	auto resolutionReadonlyLabel = makeTextButton(FRONTEND_RESOLUTION_READONLY_LABEL, videoOptionsResolutionLabel(), WBUT_SECONDARY | WBUT_DISABLE);
	resolutionReadonlyLabel->setTip(videoOptionsResolutionGetReadOnlyTooltip());
	auto resolutionReadonlyLabelContainer = addMargin(resolutionReadonlyLabel);
	resolutionReadonlyLabelContainer->id = FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER;

	auto resolutionReadonlyValue = makeTextButton(FRONTEND_RESOLUTION_READONLY, ScreenResolutionsModel::currentResolutionString(), WBUT_SECONDARY | WBUT_DISABLE);
	resolutionReadonlyValue->setTip(videoOptionsResolutionGetReadOnlyTooltip());
	auto resolutionReadonlyValueContainer = addMargin(resolutionReadonlyValue);
	resolutionReadonlyValueContainer->id = FRONTEND_RESOLUTION_READONLY_CONTAINER;

	grid->place({ 0 }, row, resolutionReadonlyLabelContainer);
	grid->place({ 1, 1, false }, row, resolutionReadonlyValueContainer);

	auto resolutionDropdownLabel = makeTextButton(FRONTEND_RESOLUTION_DROPDOWN_LABEL, videoOptionsResolutionLabel(), WBUT_SECONDARY);
	auto resolutionDropdownLabelContainer = addMargin(resolutionDropdownLabel);
	resolutionDropdownLabelContainer->id = FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER;
	grid->place({ 0 }, row, resolutionDropdownLabelContainer);
	grid->place({ 1, 1, false }, row, addMargin(makeResolutionDropdown()));
	row.start++;

	// Texture size
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_TEXTURESZ, _("Texture size"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_TEXTURESZ_R, videoOptionsTextureSizeString(), WBUT_SECONDARY)));
	row.start++;

	// Vsync
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_VSYNC, _("Vertical sync"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_VSYNC_R, videoOptionsVsyncString(), WBUT_SECONDARY)));
	row.start++;

	// Antialiasing
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_FSAA, _("Antialiasing*"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_FSAA_R, videoOptionsAntialiasingString(), WBUT_SECONDARY)));
	row.start++;

	auto antialiasing_label = std::make_shared<W_LABEL>();
	parent->attach(antialiasing_label);
	antialiasing_label->setGeometry(FRONTEND_POS1X + 48, FRONTEND_POS1Y - 18, FRONTEND_BUTWIDTH - FRONTEND_POS1X - 48, FRONTEND_BUTHEIGHT);
	antialiasing_label->setFontColour(WZCOL_YELLOW);
	antialiasing_label->setString(_("Warning: Antialiasing can cause crashes, especially with values > 16"));
	antialiasing_label->setTextAlignment(WLAB_ALIGNBOTTOMLEFT);

	// Display Scale
	const bool showDisplayScale = wzAvailableDisplayScales().size() > 1;
	if (showDisplayScale)
	{
		grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_DISPLAYSCALE, videoOptionsDisplayScaleLabel(), WBUT_SECONDARY)));
		grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_DISPLAYSCALE_R, videoOptionsDisplayScaleString(), WBUT_SECONDARY)));
		row.start++;
	}

	// Gfx Backend
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_GFXBACKEND, _("Graphics Backend*"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_GFXBACKEND_R, videoOptionsGfxBackendString(), WBUT_SECONDARY)));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("VIDEO OPTIONS"));

	// Quit/return
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	refreshCurrentVideoOptionsValues();
}

std::vector<unsigned int> availableDisplayScalesSorted()
{
	std::vector<unsigned int> displayScales = wzAvailableDisplayScales();
	std::sort(displayScales.begin(), displayScales.end());
	return displayScales;
}

void seqWindowMode()
{
	auto currentFullscreenMode = war_getWindowMode();
	bool success = false;

	auto supportedFullscreenModes = wzSupportedWindowModes();
	if (supportedFullscreenModes.empty()) { return; }
	size_t currentFullscreenModePos = 0;
	for (size_t i = 0; i < supportedFullscreenModes.size(); i++)
	{
		if (supportedFullscreenModes[i] == currentFullscreenMode)
		{
			currentFullscreenModePos = i;
			break;
		}
	}
	auto startingFullscreenModePos = currentFullscreenModePos;

	do
	{
		currentFullscreenModePos = seqCycle(currentFullscreenModePos, static_cast<size_t>(0), 1, supportedFullscreenModes.size() - 1);
		success = wzChangeWindowMode(supportedFullscreenModes[currentFullscreenModePos]);

	} while ((!success) && (currentFullscreenModePos != startingFullscreenModePos));

	if (currentFullscreenModePos == startingFullscreenModePos)
	{
		// Failed to change window mode - display messagebox
		wzDisplayDialog(Dialog_Warning, _("Unable to change Window Mode"), _("Warzone failed to change the Window mode.\nYour system / drivers may not support other modes."));
	}
	else if (success)
	{
		// succeeded changing vsync mode
		war_setWindowMode(supportedFullscreenModes[currentFullscreenModePos]);
	}
}

void seqVsyncMode()
{
	gfx_api::context::swap_interval_mode currentVsyncMode = getCurrentSwapMode();
	auto startingVsyncMode = currentVsyncMode;
	bool success = false;

	do
	{
		currentVsyncMode = static_cast<gfx_api::context::swap_interval_mode>(seqCycle(static_cast<std::underlying_type<gfx_api::context::swap_interval_mode>::type>(currentVsyncMode), static_cast<std::underlying_type<gfx_api::context::swap_interval_mode>::type>(gfx_api::context::min_swap_interval_mode), 1, static_cast<std::underlying_type<gfx_api::context::swap_interval_mode>::type>(gfx_api::context::max_swap_interval_mode)));

		success = gfx_api::context::get().setSwapInterval(currentVsyncMode);

	} while ((!success) && (currentVsyncMode != startingVsyncMode));

	if (currentVsyncMode == startingVsyncMode)
	{
		// Failed to change vsync mode - display messagebox
		wzDisplayDialog(Dialog_Warning, _("Unable to change Vertical Sync"), _("Warzone failed to change the Vertical Sync mode.\nYour system / drivers may not support other modes."));
	}
	else if (success)
	{
		// succeeded changing vsync mode
		saveCurrentSwapMode(currentVsyncMode);
	}
}

void seqDisplayScale()
{
	std::vector<unsigned int> displayScales = availableDisplayScalesSorted();
	assert(!displayScales.empty());

	// Get currently-configured display scale.
	unsigned int current_displayScale = war_GetDisplayScale();

	// Find current display scale in list.
	auto current = std::lower_bound(displayScales.begin(), displayScales.end(), current_displayScale);
	if (current == displayScales.end() || *current != current_displayScale)
	{
		--current;  // If current display scale doesn't exist, round down to next-highest one.
	}

	// Increment/decrement and loop if required.
	auto startingDisplayScale = current;
	bool successfulDisplayScaleChange = false;
	current = seqCycle(current, displayScales.begin(), 1, displayScales.end() - 1);

	while (current != startingDisplayScale)
	{
		// Attempt to change the display scale
		if (!wzChangeDisplayScale(*current))
		{
			debug(LOG_WARNING, "Failed to change display scale from: %d to: %d", current_displayScale, *current);

			// try the next display scale factor, and loop
			current = seqCycle(current, displayScales.begin(), 1, displayScales.end() - 1);
			continue;
		}
		else
		{
			successfulDisplayScaleChange = true;
			break;
		}
	}

	if (!successfulDisplayScaleChange)
		return;

	// Store the new display scale
	war_SetDisplayScale(*current);
}

bool runVideoOptionsMenu()
{
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_WINDOWMODE:
	case FRONTEND_WINDOWMODE_R:
	{
		seqWindowMode();

		// Update the widget(s)
		refreshCurrentVideoOptionsValues();
	}
	break;

	case FRONTEND_FSAA:
	case FRONTEND_FSAA_R:
	{
		war_setAntialiasing(pow2Cycle(war_getAntialiasing(), 0, pie_GetMaxAntialiasing()));
		widgSetString(psWScreen, FRONTEND_FSAA_R, videoOptionsAntialiasingString().c_str());
		break;
	}

	case FRONTEND_TEXTURESZ:
	case FRONTEND_TEXTURESZ_R:
	{
		int newTexSize = pow2Cycle(getTextureSize(), 128, 2048);

		// Set the new size
		setTextureSize(newTexSize);

		// Update the widget
		widgSetString(psWScreen, FRONTEND_TEXTURESZ_R, videoOptionsTextureSizeString().c_str());

		break;
	}

	case FRONTEND_VSYNC:
	case FRONTEND_VSYNC_R:
		seqVsyncMode();

		// Update the widget(s)
		refreshCurrentVideoOptionsValues();
		break;

	case FRONTEND_GFXBACKEND:
	case FRONTEND_GFXBACKEND_R:
	{
		const std::vector<video_backend> availableBackends = wzAvailableGfxBackends();
		if (availableBackends.size() >= 1)
		{
			auto current = std::find(availableBackends.begin(), availableBackends.end(), war_getGfxBackend());
			if (current == availableBackends.end())
			{
				current = availableBackends.begin();
			}
			current = seqCycle(current, availableBackends.begin(), 1, availableBackends.end() - 1);
			war_setGfxBackend(*current);
			widgSetString(psWScreen, FRONTEND_GFXBACKEND_R, videoOptionsGfxBackendString().c_str());
		}
		else
		{
			debug(LOG_ERROR, "There must be at least one valid backend");
		}
		break;
	}

	case FRONTEND_DISPLAYSCALE:
	case FRONTEND_DISPLAYSCALE_R:
		seqDisplayScale();

		// Update the widget(s)
		refreshCurrentVideoOptionsValues();
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);

	return true;
}


char const* videoOptionsVsyncString()
{
	switch (getCurrentSwapMode()) {
	case gfx_api::context::swap_interval_mode::immediate:
		return _("Off");
	case gfx_api::context::swap_interval_mode::vsync:
		return _("On");
	case gfx_api::context::swap_interval_mode::adaptive_vsync:
		return _("Adaptive");
	}
	return "n/a";
}

std::string videoOptionsDisplayScaleString()
{
	char resolution[100];
	ssprintf(resolution, "%d%%", war_GetDisplayScale());
	return resolution;
}

std::string videoOptionsGfxBackendString()
{
	return to_display_string(war_getGfxBackend());
}


char const* videoOptionsResolutionGetReadOnlyTooltip()
{
	switch (war_getWindowMode())
	{
	case WINDOW_MODE::desktop_fullscreen:
		return _("In Desktop Fullscreen mode, the resolution matches that of your desktop \n(or what the window manager allows).");
	case WINDOW_MODE::windowed:
		return _("You can change the resolution by resizing the window normally. (Try dragging a corner / edge.)");
	default:
		return "";
	}
}

void videoOptionsDisableResolutionButtons()
{
	widgReveal(psWScreen, FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER);
	widgReveal(psWScreen, FRONTEND_RESOLUTION_READONLY_CONTAINER);
	auto readonlyResolutionTooltip = videoOptionsResolutionGetReadOnlyTooltip();
	widgSetTip(psWScreen, FRONTEND_RESOLUTION_READONLY_LABEL, readonlyResolutionTooltip);
	widgSetTip(psWScreen, FRONTEND_RESOLUTION_READONLY, readonlyResolutionTooltip);
	widgHide(psWScreen, FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER);
	widgHide(psWScreen, FRONTEND_RESOLUTION_DROPDOWN);
}

void videoOptionsEnableResolutionButtons()
{
	widgHide(psWScreen, FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER);
	widgHide(psWScreen, FRONTEND_RESOLUTION_READONLY_CONTAINER);
	widgReveal(psWScreen, FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER);
	widgReveal(psWScreen, FRONTEND_RESOLUTION_DROPDOWN);
}

char const* videoOptionsWindowModeString()
{
	switch (war_getWindowMode())
	{
	case WINDOW_MODE::desktop_fullscreen:
		return _("Desktop Full");
	case WINDOW_MODE::windowed:
		return _("Windowed");
	case WINDOW_MODE::fullscreen:
		return _("Fullscreen");
	}
	return "n/a"; // silence warning
}

std::string videoOptionsAntialiasingString()
{
	if (war_getAntialiasing() == 0)
	{
		return _("Off");
	}
	else
	{
		return std::to_string(war_getAntialiasing()) + "×";
	}
}

char const* videoOptionsWindowModeLabel()
{
	return _("Graphics Mode");
}

char const* videoOptionsResolutionLabel()
{
	return _("Resolution");
}

char const* videoOptionsDisplayScaleLabel()
{
	return _("Display Scale");
}

std::string videoOptionsTextureSizeString()
{
	char textureSize[20];
	ssprintf(textureSize, "%d", getTextureSize());
	return textureSize;
}


gfx_api::context::swap_interval_mode getCurrentSwapMode()
{
	return to_swap_mode(war_GetVsync());
}

void saveCurrentSwapMode(gfx_api::context::swap_interval_mode mode)
{
	war_SetVsync(to_int(mode));
}

