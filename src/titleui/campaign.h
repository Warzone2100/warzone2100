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

#ifndef __INCLUDED_SRC_TITLEUI_CAMPAIGN_H__
#define __INCLUDED_SRC_TITLEUI_CAMPAIGN_H__

#include "titleui.h"
#include "../modinfo.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

class WzCampaignSelectorForm;

class WzCampaignSelectorTitleUI : public WzTitleUI
{
public:
	WzCampaignSelectorTitleUI(std::shared_ptr<WzTitleUI> parent);
	virtual ~WzCampaignSelectorTitleUI();
	virtual void start() override;
	virtual TITLECODE run() override;
	void screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight) override;

	std::shared_ptr<WzTitleUI> getParentTitleUI();

protected:
	friend class WzCampaignSelectorForm;
	void displayStartOptionsForm(optional<WzCampaignModInfo> modInfo);

private:
	std::shared_ptr<WzTitleUI> parent;
	std::shared_ptr<WzCampaignSelectorForm> campaignSelector;
	std::shared_ptr<WIDGET> displayedPanel;
};

#endif // __INCLUDED_SRC_TITLEUI_CAMPAIGN_H__
