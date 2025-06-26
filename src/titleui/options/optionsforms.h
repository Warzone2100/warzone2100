// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
 *  Widgets for displaying a list of changeable options
 */

#pragma once

#include "../widgets/optionsform.h"
#include <vector>
#include <functional>

std::shared_ptr<OptionsForm> makeInterfaceOptionsForm(const std::function<void()> languageDidChangeHandler = nullptr);
std::shared_ptr<OptionsForm> makeDefaultsOptionsForm();
std::shared_ptr<OptionsForm> makeGraphicsOptionsForm();
std::shared_ptr<OptionsForm> makeAudioOptionsForm();
std::shared_ptr<OptionsForm> makeControlsOptionsForm();
std::shared_ptr<OptionsForm> makeWindowOptionsForm();

class OptionsBrowserForm : public WIDGET
{
protected:
	OptionsBrowserForm();
	void initialize(const std::shared_ptr<WIDGET>& optionalBackButton);
public:
	static std::shared_ptr<OptionsBrowserForm> make(const std::shared_ptr<WIDGET>& optionalBackButton = nullptr);

	enum class Modes
	{
		Interface,
		Defaults,
		Graphics,
		Audio,
		Controls,
		Window
	};

	typedef std::function<std::shared_ptr<OptionsForm>(void)> CreateOptionsFormFunc;
	void addOptionsForm(Modes mode, const CreateOptionsFormFunc& createFormFunc, const WzString& untranslatedTitle);
	void showOpenConfigDirLink(bool show);
	bool switchToOptionsForm(Modes mode);
	void informLanguageDidChange();

protected:
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	void displayOptionsForm(size_t idx);
	bool displayOptionsForm(const WzString& untranslatedTitle);
	bool displayOptionsFormSection(const WzString& sectionId);
	void positionCurrentOptionsForm();
	void updateSectionSwitcher();
private:
	struct OptionsFormDetails
	{
		Modes mode;
		WzString untranslatedTitle;
		CreateOptionsFormFunc createFormFunc;
		std::shared_ptr<OptionsForm> optionsForm;
	};
	std::vector<OptionsFormDetails> optionsForms;
	optional<size_t> currentOptionsForm = nullopt;
	std::shared_ptr<WIDGET> optionalBackButton;
	std::shared_ptr<WIDGET> optionsFormSwitcher;
	std::shared_ptr<WIDGET> optionsFormSectionSwitcher;
	std::shared_ptr<WIDGET> openConfigurationDirectoryLink;
	int32_t maxOptionsFormWidth = 450;
	const int32_t paddingBeforeOptionsForm = 10;
	const int32_t topPadding = 0;
	const int32_t leftOpenConfigDirLinkPadding = 10;
	const int32_t bottomPadding = 20;
};

std::shared_ptr<OptionsBrowserForm> createOptionsBrowser(bool inGame, const std::shared_ptr<WIDGET>& optionalBackButton = nullptr);
