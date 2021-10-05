#include "screenresolutionmodel.h"

void ScreenResolutionsModel::selectAt(size_t index) const
{
	// Disable the ability to use the Video options menu to live-change the window size when in windowed mode.
	// Why?
	//	- Certain window managers don't report their own changes to window size through SDL in all circumstances.
	//	  (For example, attempting to set a window size of 800x600 might result in no actual change when using a
	//	   tiling window manager (ex. i3), but SDL thinks the window size has been set to 800x600. This obviously
	//     breaks things.)
	//  - Manual window resizing is supported (so there is no need for this functionality in the Video menu).
	ASSERT_OR_RETURN(, wzGetCurrentWindowMode() == WINDOW_MODE::fullscreen, "Attempt to change resolution in windowed-mode / desktop-fullscreen mode");

	ASSERT_OR_RETURN(, index < modes.size(), "Invalid mode index passed to ScreenResolutionsModel::selectAt");

	auto selectedResolution = modes.at(index);

	auto currentResolution = getCurrentResolution();
	// Attempt to change the resolution
	if (!wzChangeWindowResolution(selectedResolution.screen, selectedResolution.width, selectedResolution.height))
	{
		debug(
			LOG_WARNING,
			"Failed to change active resolution from: %s to: %s",
			ScreenResolutionsModel::resolutionString(currentResolution).c_str(),
			ScreenResolutionsModel::resolutionString(selectedResolution).c_str()
		);
	}

	// Store the new width and height
	war_SetScreen(selectedResolution.screen);
	war_SetWidth(selectedResolution.width);
	war_SetHeight(selectedResolution.height);

	// Update the widget(s)
	refreshCurrentVideoOptionsValues();
}
