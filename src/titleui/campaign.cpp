/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
/** \file
 *  Campaign Selector / Configuration / Startup Title UI.
 */

#include "campaign.h"
#include "../frontend.h"
#include "../multiint.h"
#include "../campaigninfo.h"
#include "../seqdisp.h"
#include "../hci.h"
#include "../urlhelpers.h"
#include "../frend.h"
#include "lib/widget/widget.h"

static void frontEndNewGame(int which)
{
	SPinit(LEVEL_TYPE::CAMPAIGN);

	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();
	sstrcpy(aLevelName, list[which].level.toUtf8().c_str());
	setCampaignName(list[which].name.toStdString());

	// show this only when the video sequences are installed
	if (seq_hasVideos())
	{
		if (!list[which].video.isEmpty())
		{
			seq_ClearSeqList();
			seq_AddSeqToList(list[which].video.toUtf8().c_str(), nullptr, list[which].captions.toUtf8().c_str(), false);
			seq_StartNextFullScreenVideo();
		}
	}
	debug(LOG_WZ, "Loading campaign mod -- %s", aLevelName);
	changeTitleMode(STARTGAME);
}

WzCampaignSelectorTitleUI::WzCampaignSelectorTitleUI(std::shared_ptr<WzTitleUI> parent)
	: parent(parent)
{
}

WzCampaignSelectorTitleUI::~WzCampaignSelectorTitleUI()
{
}

void WzCampaignSelectorTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
}

void WzCampaignSelectorTitleUI::start()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	auto weakSelf = std::weak_ptr<WzCampaignSelectorTitleUI>(std::dynamic_pointer_cast<WzCampaignSelectorTitleUI>(shared_from_this()));

	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();
	ASSERT(list.size() <= static_cast<size_t>(std::numeric_limits<UDWORD>::max()), "list.size() (%zu) exceeds UDWORD max", list.size());
	for (UDWORD i = 0; i < static_cast<UDWORD>(list.size()); i++)
	{
		auto psCampaignButton = addTextButton(FRONTEND_CAMPAIGN_1 + i, FRONTEND_POS1X, FRONTEND_POS2Y + FRONTEND_BUTHEIGHT * i, gettext(list[i].name.toUtf8().c_str()), WBUT_TXTCENTRE);
		psCampaignButton->addOnClickHandler([i](W_BUTTON& button) {
			widgScheduleTask([i]() {
				frontEndNewGame(i);
			});
		});
	}
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("CAMPAIGNS"));

	auto psQuitButton = addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	psQuitButton->addOnClickHandler([weakSelf](W_BUTTON& button) {
		widgScheduleTask([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent");
			changeTitleUI(strongSelf->parent); // go back
		});
	});

	// show this only when the video sequences are not installed
	if (!seq_hasVideos())
	{
		auto psHyperlink = addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
		psHyperlink->addOnClickHandler([](W_BUTTON& button) {
			widgScheduleTask([]() {
				openURLInBrowser("https://wz2100.net/");
			});
		});
		notifyAboutMissingVideos();
	}
}

std::shared_ptr<WzTitleUI> WzCampaignSelectorTitleUI::getParentTitleUI()
{
	return parent;
}

TITLECODE WzCampaignSelectorTitleUI::run()
{
	widgRunScreen(psWScreen);

	widgDisplayScreen(psWScreen); // show the widgets currently running

	if (CancelPressed())
	{
		changeTitleUI(parent);
	}

	return TITLECODE_CONTINUE;
}
