/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/form.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include <unordered_map>
#include <set>

class HelpTriggerWidget;

class W_HELPSCREEN_CLICKFORM : public W_CLICKFORM
{
protected:
	W_HELPSCREEN_CLICKFORM(W_FORMINIT const *init);
	W_HELPSCREEN_CLICKFORM();
public:
	static std::shared_ptr<W_HELPSCREEN_CLICKFORM> make(UDWORD formID = 0);
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;

public:
	void setHelpForWidget(const std::shared_ptr<WIDGET>& widget, const WidgetHelp& help);
	void removeHelpForWidget(const std::shared_ptr<WIDGET>& widget);
	const WidgetHelp* getHelpForWidget(const std::shared_ptr<WIDGET>& widget);

protected:
	friend class HelpTriggerWidget;
	void setCurrentHelpWidget(const std::shared_ptr<HelpTriggerWidget>& triggerWidget, const std::shared_ptr<W_FORM>& helpPanel);
	void unsetCurrentHelpWidget(const std::shared_ptr<HelpTriggerWidget>& triggerWidget);
	bool helperWidgetHasFocusLock();

public:
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 125);
	std::function<void ()> onClickedFunc;
	std::function<void ()> onCancelPressed;
private:
	struct WidgetInfo
	{
		WidgetHelp help;
		std::shared_ptr<WIDGET> helpTriggerWidget;

		WidgetInfo() { }
		WidgetInfo(const WidgetHelp& help)
		: help(help)
		{ }
	};
	typedef std::unordered_map<std::shared_ptr<WIDGET>, WidgetInfo>  RegisteredHelpWidgetsMap;
	RegisteredHelpWidgetsMap registeredWidgets;
	std::weak_ptr<WIDGET> currentHelpWidget;
	std::shared_ptr<W_FORM> currentHelpPanel;
};

struct W_HELP_OVERLAY_SCREEN: public W_SCREEN
{
protected:
	W_HELP_OVERLAY_SCREEN(): W_SCREEN() {}

public:
	typedef std::function<void (std::shared_ptr<W_HELP_OVERLAY_SCREEN>)> OnCloseFunc;
	static std::shared_ptr<W_HELP_OVERLAY_SCREEN> make(const OnCloseFunc& onCloseFunction);

public:
	void setHelpFromWidgets(const std::shared_ptr<WIDGET>& root);
	void setHelpForWidget(const std::shared_ptr<WIDGET>& widget, const WidgetHelp& help);
	void removeHelpForWidget(const std::shared_ptr<WIDGET>& widget);
	const WidgetHelp* getHelpForWidget(const std::shared_ptr<WIDGET>& widget);

private:
	void closeHelpOverlayScreen();

private:
	std::shared_ptr<W_HELPSCREEN_CLICKFORM> rootHelpScreenForm;
	OnCloseFunc onCloseFunc;

private:
	W_HELP_OVERLAY_SCREEN(W_HELP_OVERLAY_SCREEN const &) = delete;
	W_HELP_OVERLAY_SCREEN &operator =(W_HELP_OVERLAY_SCREEN const &) = delete;
};
