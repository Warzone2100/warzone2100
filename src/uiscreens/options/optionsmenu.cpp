#include "optionsmenu.h"

#include "lib/framework/wzapp.h" // Dialog_Warning, wzDisplayDialog
#include "../../urlhelpers.h" // openFolderInDefaultFileManager

// ////////////////////////////////////////////////////////////////////////////
// Options Menu
void startOptionsMenu()
{
	sliderEnableDrag(true);

	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("OPTIONS"));
	addTextButton(FRONTEND_GAMEOPTIONS, FRONTEND_POS2X, FRONTEND_POS2Y, _("Game Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_GRAPHICSOPTIONS, FRONTEND_POS3X, FRONTEND_POS3Y, _("Graphics Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_VIDEOOPTIONS, FRONTEND_POS4X, FRONTEND_POS4Y, _("Video Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_AUDIO_AND_ZOOMOPTIONS, FRONTEND_POS5X, FRONTEND_POS5Y, _("Audio / Zoom Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MOUSEOPTIONS, FRONTEND_POS6X, FRONTEND_POS6Y, _("Mouse Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_KEYMAP, FRONTEND_POS7X, FRONTEND_POS7Y, _("Key Mappings"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MUSICMANAGER, FRONTEND_POS8X, FRONTEND_POS8Y, _("Music Manager"), WBUT_TXTCENTRE);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Open Configuration Directory"), 0);
}

bool runOptionsMenu()
{
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_GAMEOPTIONS:
		changeTitleMode(GAME);
		break;
	case FRONTEND_GRAPHICSOPTIONS:
		changeTitleMode(GRAPHICS_OPTIONS);
		break;
	case FRONTEND_AUDIO_AND_ZOOMOPTIONS:
		changeTitleMode(AUDIO_AND_ZOOM_OPTIONS);
		break;
	case FRONTEND_VIDEOOPTIONS:
		changeTitleMode(VIDEO_OPTIONS);
		break;
	case FRONTEND_MOUSEOPTIONS:
		changeTitleMode(MOUSE_OPTIONS);
		break;
	case FRONTEND_KEYMAP:
		changeTitleMode(KEYMAP);
		break;
	case FRONTEND_MUSICMANAGER:
		changeTitleMode(MUSIC_MANAGER);
		break;
	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;
	case FRONTEND_HYPERLINK:
		if (!openFolderInDefaultFileManager(PHYSFS_getWriteDir()))
		{
			// Failed to open write dir in default filesystem browser
			std::string configErrorMessage = _("Failed to open configuration directory in system default file browser.");
			configErrorMessage += "\n\n";
			configErrorMessage += _("Configuration directory is reported as:");
			configErrorMessage += "\n";
			configErrorMessage += PHYSFS_getWriteDir();
			configErrorMessage += "\n\n";
			configErrorMessage += _("If running inside a container / isolated environment, this may differ from the actual path on disk.");
			configErrorMessage += "\n";
			configErrorMessage += _("Please see the documentation for more information on how to locate it manually.");
			wzDisplayDialog(Dialog_Warning, _("Failed to open configuration directory"), configErrorMessage.c_str());
		}
		break;
	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(TITLE);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}
