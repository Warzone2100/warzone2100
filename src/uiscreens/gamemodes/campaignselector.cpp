#include "campaignselector.h"

#include "lib/framework/physfs_ext.h"

#include "../../mission.h" // CAMPAIGN_FILE
#include "../../seqdisp.h" // seq_*

void startCampaignSelector()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();
	ASSERT(list.size() <= static_cast<size_t>(std::numeric_limits<UDWORD>::max()), "list.size() (%zu) exceeds UDWORD max", list.size());
	for (UDWORD i = 0; i < static_cast<UDWORD>(list.size()); i++)
	{
		addTextButton(FRONTEND_CAMPAIGN_1 + i, FRONTEND_POS1X, FRONTEND_POS2Y + FRONTEND_BUTHEIGHT * i, gettext(list[i].name.toUtf8().c_str()), WBUT_TXTCENTRE);
	}
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("CAMPAIGNS"));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	// show this only when the video sequences are not installed
	if (!PHYSFS_exists("sequences/devastation.ogg"))
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
	}
}

static void frontEndNewGame(int which)
{
	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();
	sstrcpy(aLevelName, list[which].level.toUtf8().c_str());
	// show this only when the video sequences are installed
	if (PHYSFS_exists("sequences/devastation.ogg"))
	{
		if (!list[which].video.isEmpty())
		{
			seq_ClearSeqList();
			seq_AddSeqToList(list[which].video.toUtf8().c_str(), nullptr, list[which].captions.toUtf8().c_str(), false);
			seq_StartNextFullScreenVideo();
		}
	}
	if (!list[which].package.isEmpty())
	{
		WzString path;
		path += PHYSFS_getWriteDir();
		path += PHYSFS_getDirSeparator();
		path += "campaigns";
		path += PHYSFS_getDirSeparator();
		path += list[which].package;
		if (!PHYSFS_mount(path.toUtf8().c_str(), NULL, PHYSFS_APPEND))
		{
			debug(LOG_ERROR, "Failed to load campaign mod \"%s\": %s",
				path.toUtf8().c_str(), WZ_PHYSFS_getLastError());
		}
	}
	if (!list[which].loading.isEmpty())
	{
		debug(LOG_WZ, "Adding campaign mod level \"%s\"", list[which].loading.toUtf8().c_str());
		if (!loadLevFile(list[which].loading.toUtf8().c_str(), mod_campaign, false, nullptr))
		{
			debug(LOG_ERROR, "Failed to load %s", list[which].loading.toUtf8().c_str());
			return;
		}
	}
	debug(LOG_WZ, "Loading campaign mod -- %s", aLevelName);
	changeTitleMode(STARTGAME);
}



bool runCampaignSelector()
{
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.
	if (id == FRONTEND_QUIT)
	{
		changeTitleMode(SINGLE); // go back
	}
	else if (id >= FRONTEND_CAMPAIGN_1 && id <= FRONTEND_CAMPAIGN_6) // chose a campaign
	{
		SPinit(LEVEL_TYPE::CAMPAIGN);
		frontEndNewGame(id - FRONTEND_CAMPAIGN_1);
	}
	else if (id == FRONTEND_HYPERLINK)
	{
		runHyperlink();
	}

	widgDisplayScreen(psWScreen); // show the widgets currently running

	if (CancelPressed())
	{
		changeTitleMode(SINGLE);
	}

	return true;
}
