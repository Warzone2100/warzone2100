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
#include "../difficulty.h"
#include "../intdisplay.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/file.h"
#include "lib/framework/file_ext.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/widget/dropdown.h"
#include "lib/widget/margin.h"
#include "lib/widget/gridlayout.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/paragraph.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "../modding.h"
#include "../version.h"
#include "widgets/infobutton.h"
#include "widgets/frontendimagebutton.h"
#include "widgets/advcheckbox.h"

typedef WzAdvCheckbox WzCampaignTweakOptionToggle;

#define WZ2100_CORE_CLASSIC_BALANCE_MOD_FILENAME "wz2100_camclassic.wz"

#define FRONTEND_CAMPAIGNUI_BUTHEIGHT 28

static std::vector<WzCampaignModInfo> cached_availableCampaignMods;

typedef std::function<void (std::vector<WzCampaignModInfo> availableCampaignMods)> LoadCampaignModsCompletion;

struct LoadCampaingModsRequest
{
	std::string versionedCampaignModsPath;
	std::string versionedCampaignModsPath_platformDependent;
	LoadCampaignModsCompletion completionHandler;
};

static bool validateCampaignMod(WzCampaignModInfo& modInfo, const std::string& baseDir, const std::string& realModFilePathAndName)
{
	// Verify the name is not empty
	if (modInfo.name.empty())
	{
		debug(LOG_ERROR, "Campaign mod missing valid name field: %s", realModFilePathAndName.c_str());
		return false;
	}
	// Verify the author is not empty
	if (modInfo.author.empty())
	{
		debug(LOG_ERROR, "Campaign mod missing valid author field: %s", realModFilePathAndName.c_str());
		return false;
	}
	// Verify the license is not empty
	if (modInfo.license.empty())
	{
		debug(LOG_ERROR, "Campaign mod missing valid license field: %s", realModFilePathAndName.c_str());
		return false;
	}
	// Warn if the license isn't recognized (from the list of suggested examples)
	const std::unordered_set<std::string> RecognizedSPDXLicenseExpressions = {
		"CC-BY-4.0 OR GPL-2.0-or-later",
		"CC-BY-3.0 OR GPL-2.0-or-later",
		"GPL-2.0-or-later",
		"CC0-1.0"
	};
	if (RecognizedSPDXLicenseExpressions.count(modInfo.license) == 0)
	{
		debug(LOG_INFO, "Campaign mod has license \"%s\", consider one of the recommended options (ex. \"GPL-2.0-or-later\"): %s", modInfo.license.c_str(), realModFilePathAndName.c_str());
		// not fatal validation failure - continue
	}

	// Verify original campaign balance mods
	if (modInfo.type == WzModType::CampaignBalanceMod)
	{
		// It is expected that original campaign balance mods only modify the following directories:
		// - stats/
		// - script/
		// Anything else should be packaged as an AlternateCampaign

		bool invalidCampaignBalanceModContents = false;
		WZ_PHYSFS_enumerateFiles(baseDir.c_str(), [&](const char* file) -> bool {
			if (file == nullptr) { return true; }
			if (*file == '\0') { return true; }
			std::string realFileName_platformIndependent = baseDir + "/" + file;
			bool isDirectory = WZ_PHYSFS_isDirectory(realFileName_platformIndependent.c_str()) != 0;
			if (isDirectory)
			{
				if (strcmp(file, "stats") == 0)
				{
					return true; // allowed - continue enumeration
				}
				else if (strcmp(file, "script") == 0)
				{
					return true; // allowed - continue enumeration
				}
				else
				{
					// overriding other directories is not allowed
					invalidCampaignBalanceModContents = true;
					return false;
				}
			}
			return true; // continue enumeration
		});

		if (invalidCampaignBalanceModContents)
		{
			debug(LOG_INFO, "Campaign balance mod contains unsupported contents - treating as alternate campaign: %s", realModFilePathAndName.c_str());
			modInfo.type = WzModType::AlternateCampaign;
		}
	}

	// Verify alternate campaign has at least one campaign file listed
	if (modInfo.type == WzModType::AlternateCampaign)
	{
		if (modInfo.campaignFiles.empty())
		{
			debug(LOG_ERROR, "Alternate campaign mod must list one or more valid campaign JSON files in \"campaigns\": %s", realModFilePathAndName.c_str());
			return false;
		}
	}

	return true;
}

static int loadCampaignModsAsyncImpl(void* data)
{
	LoadCampaingModsRequest* pRequestInfo = (LoadCampaingModsRequest*)data;
	if (!pRequestInfo)
	{
		return 1;
	}

	// 1. Enumerate mods/campaign
	std::vector<WzCampaignModInfo> availableMods;
	std::string basePathFull = pRequestInfo->versionedCampaignModsPath;
	std::string basePathFull_PlatformDependent = pRequestInfo->versionedCampaignModsPath_platformDependent;
	auto enumSuccess = WZ_PHYSFS_enumerateFiles(basePathFull.c_str(), [&availableMods, basePathFull, basePathFull_PlatformDependent](const char* file) -> bool {
		if (file == nullptr) { return true; }
		if (*file == '\0') { return true; }
		std::string realFileName_platformIndependent = basePathFull + "/" + file;
		std::string realFileName_platformDependent = basePathFull_PlatformDependent + PHYSFS_getDirSeparator() + file;
		bool isDirectory = WZ_PHYSFS_isDirectory(realFileName_platformIndependent.c_str()) != 0;
		if (isDirectory)
		{
			return true;
		}

		if (file[0] == '.' || realFileName_platformIndependent.substr(realFileName_platformIndependent.find_last_of('.') + 1) != "wz")
		{
			return true; // skip and continue
		}

		// if it's a .wz file, mount it in a temporary path
		const char * pRealDirStr = PHYSFS_getRealDir(realFileName_platformIndependent.c_str());
		if (!pRealDirStr)
		{
			debug(LOG_ERROR, "Failed to find realdir for: %s", realFileName_platformIndependent.c_str());
			return true; // skip and continue
		}
		std::string realFilePathAndName = pRealDirStr + realFileName_platformDependent;

		if (PHYSFS_mount(realFilePathAndName.c_str(), "WZTempProcessCamMod", PHYSFS_APPEND) == 0)
		{
			debug(LOG_ERROR, "Could not mount %s, because: %s.\nPlease delete or move the file specified.", realFilePathAndName.c_str(), WZ_PHYSFS_getLastError());
			return true; // skip and continue
		}

		// Process the mod .wz file (using the mounted path in WZTempProcessCamMod)
		std::string modInfoPath = "WZTempProcessCamMod/mod-info.json";
		auto modInfo = loadCampaignModInfoFromFile(modInfoPath, "WZTempProcessCamMod");
		if (modInfo.has_value())
		{
			// Add the modFilename and modPath
			modInfo.value().modFilename = file;
			modInfo.value().modPath = std::string(pRealDirStr) + basePathFull_PlatformDependent;

			// attempt to load the mod_banner.png data, if it exists
			std::string modBannerPath = "WZTempProcessCamMod/mod-banner.png";
			if (PHYSFS_exists(modBannerPath.c_str()) != 0)
			{
				loadFileToBufferVectorT<unsigned char>(modBannerPath.c_str(), modInfo.value().modBannerPNGData, false, false);
			}
		}

		// Validate the mod info
		if (modInfo.has_value())
		{
			if (!validateCampaignMod(modInfo.value(), "WZTempProcessCamMod", realFilePathAndName))
			{
				modInfo.reset(); // not loadable
			}
		}

		if (WZ_PHYSFS_unmount(realFilePathAndName.c_str()) == 0)
		{
			debug(LOG_ERROR, "Could not unmount %s, %s", realFilePathAndName.c_str(), WZ_PHYSFS_getLastError());
		}

		if (modInfo.has_value())
		{
			availableMods.push_back(modInfo.value());
		}

		return true; // continue enumeration
	});
	if (!enumSuccess)
	{
		debug(LOG_INFO, "Failed to enumerate available campaign mods?");
	}
	pRequestInfo->completionHandler(std::move(availableMods));
	delete pRequestInfo;
	return 0;
}

static void loadCampaignModsAsync(LoadCampaignModsCompletion completionHandler)
{
	// Build the configuration data
	auto pRequest = new LoadCampaingModsRequest();
	pRequest->versionedCampaignModsPath = versionedModsPath(MODS_CAMPAIGN);
	pRequest->versionedCampaignModsPath_platformDependent = convertToPlatformDependentPath(pRequest->versionedCampaignModsPath.c_str()).toUtf8();
	pRequest->completionHandler = completionHandler;

	WZ_THREAD * pLoadModsThread = wzThreadCreate(loadCampaignModsAsyncImpl, pRequest);
	if (pLoadModsThread == nullptr)
	{
		debug(LOG_ERROR, "Failed to create thread for loading campaign mods");
		delete pRequest;
	}
	else
	{
		wzThreadDetach(pLoadModsThread);
		// the thread handles deleting pRequest
	}
}

// MARK: -

class WzCampaignSelectorForm;

class WzCampaignModButton : public W_BUTTON
{
protected:
	WzCampaignModButton() {}

	void initialize(const WzString& text)
	{
		FontID = font_regular_bold;
		setString(text);
		int minButtonWidthForText = iV_GetTextWidth(pText, FontID);
		setGeometry(0, 0, minButtonWidthForText + 20, iV_GetTextLineSize(FontID) + 16);
	}
public:
	static std::shared_ptr<WzCampaignModButton> make(const WzString& text)
	{
		class make_shared_enabler: public WzCampaignModButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(text);
		return widget;
	}
	void setIsSelected(bool val)
	{
		isSelected = val;
	}
	bool getIsSelected() const { return isSelected; }
protected:
	void display(int xOffset, int yOffset) override;
private:
	WzText wzText;
	bool isSelected = false;
	static constexpr int leftPadding = 25;
	static constexpr int rightPadding = 10;
};

void WzCampaignModButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	optional<PIELIGHT> backColor = nullopt;
	if (isSelected || isDown)
	{
		backColor = WZCOL_MENU_SCORE_BUILT;
	}
	if (backColor.has_value())
	{
		pie_UniTransBoxFill(x0, y0, x1, y1, backColor.value());
	}
	if (isHighlight)
	{
		// do something?
	}

	if (haveText)
	{
		wzText.setText(pText, FontID);

		PIELIGHT textColor = (isDown) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}

		int textX0 = x0 + leftPadding;
		int textY0 = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();

		int maxTextDisplayableWidth = width() - (leftPadding + rightPadding);
		bool isTruncated = maxTextDisplayableWidth < wzText.width();
		if (isTruncated)
		{
			maxTextDisplayableWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
		}
		wzText.render(textX0, textY0, textColor, 0.0f, maxTextDisplayableWidth);
		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxTextDisplayableWidth + 2, textY0), textColor);
		}
	}
}

class WzCampaignListSection : public WIDGET
{
protected:
	WzCampaignListSection() {}

	void initialize();
public:
	static std::shared_ptr<WzCampaignListSection> make()
	{
		class make_shared_enabler: public WzCampaignListSection {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}
public:
	virtual int32_t idealHeight() override;
	virtual void geometryChanged() override;

	void updateList(const std::vector<std::shared_ptr<WzCampaignModButton>>& modButtons);
	void clearAllIsSelected();
public:
	std::shared_ptr<W_LABEL> sectionTitle;
	std::vector<std::shared_ptr<WzCampaignModButton>> modButtons;
	std::shared_ptr<ScrollableListWidget> sectionList;
};

void WzCampaignListSection::updateList(const std::vector<std::shared_ptr<WzCampaignModButton>>& _modButtons)
{
	modButtons = _modButtons;
	sectionList->clear();

	for (auto& but : modButtons)
	{
		sectionList->addItem(but);
	}

	geometryChanged();
}

void WzCampaignListSection::geometryChanged()
{
	sectionTitle->callCalcLayout();
	sectionList->callCalcLayout();
}

void WzCampaignListSection::initialize()
{
	sectionTitle = std::make_shared<W_LABEL>();
	sectionTitle->setFont(font_regular_bold, WZCOL_TEXT_DARK);
	sectionTitle->setString(_("Additional Campaigns"));
	sectionTitle->setCanTruncate(true);
	attach(sectionTitle);
	sectionTitle->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psLabel = (W_LABEL*)psWidget;
		auto parent = std::dynamic_pointer_cast<WzCampaignListSection>(psWidget->parent());
		ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
		int x0 = 15;
		int w = std::min(psLabel->getMaxLineWidth(), parent->width() - x0);
		psWidget->setGeometry(x0, 0, w, iV_GetTextLineSize(font_regular_bold));
	}));

	sectionList = ScrollableListWidget::make();
	attach(sectionList);
	sectionList->setCalcLayout([](WIDGET* psWidget) {
		auto parent = std::dynamic_pointer_cast<WzCampaignListSection>(psWidget->parent());
		ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
		int y0 = parent->sectionTitle->y() + parent->sectionTitle->height() + 10;
		int h = parent->height() - y0;
		psWidget->setGeometry(0, y0, parent->width(), h);
	});
}

int32_t WzCampaignListSection::idealHeight()
{
	return sectionTitle->idealHeight() + 10 + sectionList->idealHeight();
}

void WzCampaignListSection::clearAllIsSelected()
{
	for (const auto& but : modButtons)
	{
		but->setIsSelected(false);
	}
}

class WzCampaignSelectorForm : public WIDGET
{
protected:
	WzCampaignSelectorForm() {}

	void initialize(const std::shared_ptr<WzCampaignSelectorTitleUI>& parent);
public:
	static std::shared_ptr<WzCampaignSelectorForm> make(const std::shared_ptr<WzCampaignSelectorTitleUI>& parent)
	{
		class make_shared_enabler: public WzCampaignSelectorForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(parent);
		return widget;
	}
public:
	void updateList(const std::vector<WzCampaignModInfo>& availableMods);
	void changeSelectedItem(optional<WzCampaignModInfo> modInfo);
protected:
	virtual void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
private:
	std::shared_ptr<WzCampaignModButton> makeCampaignModButton(const WzString& title, optional<WzCampaignModInfo> modInfo)
	{
		auto button = WzCampaignModButton::make(title);
		auto weakParent = std::weak_ptr<WzCampaignSelectorForm>(std::dynamic_pointer_cast<WzCampaignSelectorForm>(shared_from_this()));
		button->addOnClickHandler([weakParent, modInfo](W_BUTTON& but) {
			auto psSelf = std::dynamic_pointer_cast<WzCampaignModButton>(but.shared_from_this());
			auto strongParent = weakParent.lock();
			if (!psSelf->getIsSelected())
			{
				strongParent->changeSelectedItem(modInfo);
				psSelf->setIsSelected(true);
			}
		});
		return button;
	}
private:
	std::weak_ptr<WzCampaignSelectorTitleUI> parentCampaignSelectorUI;
	std::shared_ptr<WzCampaignModButton> originalCampaignButton;
	std::shared_ptr<WzCampaignListSection> alternateCampaigns;
};

void WzCampaignSelectorForm::geometryChanged()
{
	originalCampaignButton->callCalcLayout();
	alternateCampaigns->callCalcLayout();
}

void WzCampaignSelectorForm::display(int xOffset, int yOffset)
{ }

void WzCampaignSelectorForm::initialize(const std::shared_ptr<WzCampaignSelectorTitleUI>& parent)
{
	parentCampaignSelectorUI = parent;

	originalCampaignButton = makeCampaignModButton(_("Original Campaign"), nullopt);
	attach(originalCampaignButton);
	originalCampaignButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WzCampaignSelectorForm>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		psWidget->setGeometry(0, 0, psParent->width(), psWidget->height());
	}));
	originalCampaignButton->setIsSelected(true);

	alternateCampaigns = WzCampaignListSection::make();
	attach(alternateCampaigns);
	alternateCampaigns->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WzCampaignSelectorForm>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int y0 = psParent->originalCampaignButton->y() + psParent->originalCampaignButton->height() + 20;
		psWidget->setGeometry(0, y0, psParent->width(), psParent->height() - y0);
	}));
}

void WzCampaignSelectorForm::updateList(const std::vector<WzCampaignModInfo>& availableMods)
{
	std::vector<WzCampaignModInfo> sortedAlternateCampaigns;
	for (auto& mod : availableMods)
	{
		if (mod.type != WzModType::AlternateCampaign)
		{
			continue;
		}
		sortedAlternateCampaigns.push_back(mod);
	}
	std::sort(sortedAlternateCampaigns.begin(), sortedAlternateCampaigns.end(), [](const WzCampaignModInfo& a, const WzCampaignModInfo& b) -> bool {
		return a.name < b.name;
	});

	std::vector<std::shared_ptr<WzCampaignModButton>> modButtons;
	for (auto& mod : sortedAlternateCampaigns)
	{
		modButtons.push_back(makeCampaignModButton(WzString::fromUtf8(mod.name), mod));
	}

	alternateCampaigns->updateList(modButtons);
}

void WzCampaignSelectorForm::changeSelectedItem(optional<WzCampaignModInfo> modInfo)
{
	auto strongParent = parentCampaignSelectorUI.lock();
	if (!modInfo.has_value())
	{
		// original campaign
		originalCampaignButton->setIsSelected(true);
		alternateCampaigns->clearAllIsSelected();
	}
	else
	{
		originalCampaignButton->setIsSelected(false);
		alternateCampaigns->clearAllIsSelected(); // specific item is selected later by caller
	}

	strongParent->displayStartOptionsForm(modInfo);
}

// MARK: -

class CampaignStartButton : public W_BUTTON
{
protected:
	CampaignStartButton() {}
	void initialize();
public:
	static std::shared_ptr<CampaignStartButton> make()
	{
		class make_shared_enabler: public CampaignStartButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}
protected:
	void display(int xOffset, int yOffset) override;
private:
	WzText wzText;
};

void CampaignStartButton::initialize()
{
	setString(_("Start Game"));
	FontID = font_large;
	int minButtonWidthForText = iV_GetTextWidth(pText, FontID);
	setGeometry(0, 0, minButtonWidthForText + 10, iV_GetTextLineSize(FontID) + 10);
}

void CampaignStartButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	// Display the button.
	auto border_color = (isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	auto background_color = WZCOL_TRANSPARENT_BOX;
	if (isDisabled)
	{
		border_color = WZCOL_TEXT_DARK;
		background_color.byte.a = (background_color.byte.a / 2);
	}
	else if (isDown)
	{
		background_color = pal_RGBA(0,0,0,150);
	}
	pie_UniTransBoxFill(x0, y0, x1, y1, background_color);
	iV_Box2(x0, y0, x1, y1, border_color, border_color);

	if (haveText)
	{
		wzText.setText(pText, FontID);
		int fw = wzText.width();
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
		if (isDisabled)
		{
			textColor = WZCOL_TEXT_DARK;
			textColor.byte.a = (textColor.byte.a / 2);
		}
		wzText.render(fx, fy, textColor);
	}
}

class WzCampaignOptionButton : public W_BUTTON
{
protected:
	WzCampaignOptionButton() {}

	void initialize()
	{
		FontID = font_regular_bold;
		// Enable right clicks
		style |= WBUT_SECONDARY;
	}
public:
	static std::shared_ptr<WzCampaignOptionButton> make()
	{
		class make_shared_enabler: public WzCampaignOptionButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}
	void setDescriptionText(const WzString& description)
	{
		wzDescriptionText.setText(description, descriptionFontID);
		recalcIdealWidth();
	}

	void setString(WzString string) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
protected:
	void display(int xOffset, int yOffset) override;
private:
	void recalcIdealWidth()
	{
		cachedIdealWidth = (horizontalPadding * 2) + std::max<int>(iV_GetTextWidth(pText, FontID), wzDescriptionText.width());
	}
private:
	WzText wzText;
	WzText wzDescriptionText;
	iV_fonts descriptionFontID = font_small;
	int32_t cachedIdealWidth = 0;
	int lastWidgetWidth = 0;
	int horizontalPadding = 10;
	int verticalPadding = 5;
	int internalPadding = 3;
};

int32_t WzCampaignOptionButton::idealWidth()
{
	return cachedIdealWidth;
}

int32_t WzCampaignOptionButton::idealHeight()
{
	return (verticalPadding * 2) + iV_GetTextLineSize(FontID) + internalPadding + iV_GetTextLineSize(descriptionFontID);
}

void WzCampaignOptionButton::setString(WzString string)
{
	W_BUTTON::setString(string);
	recalcIdealWidth();
}

void WzCampaignOptionButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();

	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && isMouseOverWidget();

	iV_fonts fontID = wzText.getFontID();
	if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != pText)
	{
		fontID = FontID;
	}
	wzText.setText(pText, fontID);

	int availableButtonTextWidth = w - (horizontalPadding * 2);
	if (wzText.width() > availableButtonTextWidth)
	{
		// text would exceed the bounds of the button area
		// try to shrink font so it fits
		do {
			if (fontID == font_small || fontID == font_bar)
			{
				break;
			}
			auto result = iV_ShrinkFont(fontID);
			if (!result.has_value())
			{
				break;
			}
			fontID = result.value();
			wzText.setText(pText, fontID);
		} while (wzText.width() > availableButtonTextWidth);
	}
	lastWidgetWidth = width();

	// Draw the main text
	PIELIGHT textColor = (isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	int textX0 = x0 + horizontalPadding;
	int textY0 = y0 + verticalPadding - wzText.aboveBase();

	const int maxTextDisplayableWidth = w - (horizontalPadding * 2);
	int maxDisplayableMainTextWidth = maxTextDisplayableWidth;
	bool isTruncated = maxDisplayableMainTextWidth < wzText.width();
	if (isTruncated)
	{
		maxDisplayableMainTextWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
	}
	wzText.render(textX0, textY0, textColor, 0.0f, maxDisplayableMainTextWidth);
	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxDisplayableMainTextWidth + 2, textY0), textColor);
	}

	// Draw the description (truncate if needed)
	PIELIGHT descriptionColor = (isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		descriptionColor.byte.a = (descriptionColor.byte.a / 2);
	}
	int descriptionX0 = textX0;
	int descriptionY0 = y0 + verticalPadding + wzText.lineSize() + internalPadding - wzDescriptionText.aboveBase();
	int maxDisplayableDescriptionTextWidth = maxTextDisplayableWidth;
	isTruncated = maxDisplayableDescriptionTextWidth < wzDescriptionText.width();
	if (isTruncated)
	{
		maxDisplayableDescriptionTextWidth -= (iV_GetEllipsisWidth(wzDescriptionText.getFontID()) + 2);
	}
	wzDescriptionText.render(descriptionX0, descriptionY0, descriptionColor, 0.0f, maxDisplayableDescriptionTextWidth);
	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzDescriptionText.getFontID(), Vector2f(descriptionX0 + maxDisplayableDescriptionTextWidth + 2, descriptionY0), descriptionColor);
	}
}

class WzCampaignTweakOptionSetting
{
public:
	WzCampaignTweakOptionSetting(const std::string& uniqueIdentifier, const WzString& displayName, const WzString& description, bool defaultValue, bool editable)
	: uniqueIdentifier(uniqueIdentifier)
	, displayName(displayName)
	, description(description)
	, defaultValue(defaultValue)
	, currentValue(defaultValue)
	, editable(editable)
	{ }
public:
	std::string uniqueIdentifier;
	WzString displayName;
	WzString description;
	bool defaultValue = false;
	bool currentValue = false;
	bool editable = true;
};

class WzCampaignTweakOptionsEditForm : public WIDGET
{
protected:
	WzCampaignTweakOptionsEditForm()
	{}
	void initialize(const std::shared_ptr<std::vector<WzCampaignTweakOptionSetting>>& tweakOptions, const std::function<void()>& closeHandlerFunc);
public:
	static std::shared_ptr<WzCampaignTweakOptionsEditForm> make(const std::shared_ptr<std::vector<WzCampaignTweakOptionSetting>>& tweakOptions, const std::function<void()>& closeHandlerFunc)
	{
		class make_shared_enabler: public WzCampaignTweakOptionsEditForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(tweakOptions, closeHandlerFunc);
		return widget;
	}
	int32_t idealHeight() override;
protected:
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
private:
	void resetToDefaults();
	void updateResetButtonStatus();
private:
	std::shared_ptr<W_LABEL> formTitle;
	std::shared_ptr<WzFrontendImageButton> closeButton;
	std::shared_ptr<WzFrontendImageButton> resetButton;
	std::shared_ptr<ScrollableListWidget> tweaksDisplayList;
	std::vector<std::shared_ptr<WzCampaignTweakOptionToggle>> toggleWidgets;
	std::unordered_map<size_t, size_t> tweakOptionIdxToToggleWidgetIdxMap;
	std::shared_ptr<std::vector<WzCampaignTweakOptionSetting>> tweakOptions;
	int innerPadding = 20;
	int betweenButtonPadding = 15;
};

void WzCampaignTweakOptionsEditForm::initialize(const std::shared_ptr<std::vector<WzCampaignTweakOptionSetting>>& _tweakOptions, const std::function<void()>& closeHandlerFunc)
{
	tweakOptions = _tweakOptions;

	formTitle = std::make_shared<W_LABEL>();
	formTitle->setFont(font_regular_bold, WZCOL_TEXT_MEDIUM);
	formTitle->setString(_("Tweak Options:"));
	formTitle->setCanTruncate(true);
	formTitle->setTransparentToMouse(true);
	formTitle->setGeometry(innerPadding, innerPadding, formTitle->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
	attach(formTitle);

	auto weakEditForm = std::weak_ptr<WzCampaignTweakOptionsEditForm>(std::dynamic_pointer_cast<WzCampaignTweakOptionsEditForm>(shared_from_this()));

	if (closeHandlerFunc)
	{
		// Add "Close" button
		closeButton = WzFrontendImageButton::make(nullopt);
		closeButton->setString(_("Close"));
		closeButton->addOnClickHandler([closeHandlerFunc](W_BUTTON&) {
			widgScheduleTask([closeHandlerFunc]() {
				closeHandlerFunc();
			});
		});
		attach(closeButton);
	}

	// Add "Reset to Defaults" button
	resetButton = WzFrontendImageButton::make(IMAGE_ARROW_UNDO);
	resetButton->setImageDimensions(12);
	resetButton->setPadding(6, 4);
	resetButton->setTip(_("Reset to Defaults"));
	resetButton->addOnClickHandler([weakEditForm](W_BUTTON&) {
		widgScheduleTask([weakEditForm]() {
			auto strongEditForm = weakEditForm.lock();
			ASSERT_OR_RETURN(, strongEditForm != nullptr, "No parent?");
			strongEditForm->resetToDefaults();
		});
	});
	attach(resetButton);
	updateResetButtonStatus();

	tweaksDisplayList = ScrollableListWidget::make();
	tweaksDisplayList->setItemSpacing(10);
	tweaksDisplayList->setExpandWhenScrollbarInvisible(false);

	for (size_t i = 0; i < tweakOptions->size(); ++i)
	{
		const auto& o = (*tweakOptions)[i];

		if (!o.editable)
		{
			continue;	// skip non-editable settings
		}

		auto toggleWidget = WzCampaignTweakOptionToggle::make(o.displayName, o.description);
		toggleWidget->setIsChecked(o.currentValue);
		toggleWidget->addOnClickHandler([weakEditForm, i](W_BUTTON& but) {
			auto strongEditForm = weakEditForm.lock();
			ASSERT_OR_RETURN(, strongEditForm != nullptr, "No parent?");
			auto pTweakOptionsToggle = std::dynamic_pointer_cast<WzCampaignTweakOptionToggle>(but.shared_from_this());
			ASSERT_OR_RETURN(, pTweakOptionsToggle != nullptr, "Invalid button type");
			strongEditForm->tweakOptions->at(i).currentValue = pTweakOptionsToggle->isChecked();
			strongEditForm->updateResetButtonStatus();
		});

		toggleWidgets.push_back(toggleWidget);
		tweaksDisplayList->addItem(toggleWidget);
		tweakOptionIdxToToggleWidgetIdxMap.insert({i, toggleWidgets.size() - 1});
	}

	int listY0 = formTitle->y() + formTitle->height() + innerPadding;
	tweaksDisplayList->setGeometry(innerPadding, listY0, tweaksDisplayList->idealWidth(), 100);
	attach(tweaksDisplayList);
}

void WzCampaignTweakOptionsEditForm::geometryChanged()
{
	if (width() <= 1 || height() <= 1)
	{
		return;
	}

	// before sending the event to all children, resize all of the toggleWidgets to the new width (which causes them to recalculate their new needed height)
	int childWidth = width() - (innerPadding * 2) - tweaksDisplayList->getScrollbarWidth();
	for (const auto& widg : toggleWidgets)
	{
		// first call sets the width, and causes the internal paragraph to recalculate itself and its needed height
		widg->setGeometry(widg->x(), widg->y(), childWidth, widg->height());
		// second call sets the height to the needed height (including the recalculated paragraph)
		widg->setGeometry(widg->x(), widg->y(), childWidth, widg->idealHeight());
	}

	// recalculate position of tweaks list
	int listY0 = tweaksDisplayList->y();
	int listH = height() - listY0 - innerPadding;
	int listW = width() - (innerPadding * 2);
	if (listW > 1 && listH > 1)
	{
		tweaksDisplayList->setGeometry(innerPadding, listY0, listW, listH);

		// truncate list height at needed height (if scrollable height is less)
		auto listIdealHeight = tweaksDisplayList->idealHeight();
		if (listIdealHeight < listH)
		{
			tweaksDisplayList->setGeometry(innerPadding, listY0, listW, listIdealHeight);
		}
	}

	// position top buttons
	int lastButtonX0 = width() - innerPadding;
	int buttonY0 = innerPadding;
	int topButtonHeight = std::max<int>(closeButton->idealHeight(), resetButton->idealHeight());
	if (closeButton)
	{
		int buttonX0 = lastButtonX0 - closeButton->idealWidth();
		closeButton->setGeometry(buttonX0, buttonY0, closeButton->idealWidth(), topButtonHeight);
		lastButtonX0 = buttonX0 - betweenButtonPadding;
	}
	if (resetButton)
	{
		int buttonX0 = lastButtonX0 - resetButton->idealWidth();
		resetButton->setGeometry(buttonX0, buttonY0, resetButton->idealWidth(), topButtonHeight);
		lastButtonX0 = buttonX0;
	}
}

// ideal height *for the current width*
int32_t WzCampaignTweakOptionsEditForm::idealHeight()
{
	return tweaksDisplayList->y() + tweaksDisplayList->height() + innerPadding;
}

void WzCampaignTweakOptionsEditForm::display(int xOffset, int yOffset)
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

void WzCampaignTweakOptionsEditForm::resetToDefaults()
{
	for (size_t i = 0; i < tweakOptions->size(); ++i)
	{
		auto& o = (*tweakOptions)[i];
		o.currentValue = o.defaultValue;
		auto it = tweakOptionIdxToToggleWidgetIdxMap.find(i);
		if (it != tweakOptionIdxToToggleWidgetIdxMap.end())
		{
			ASSERT(it->second < toggleWidgets.size(), "Invalid index: %zu", it->second);
			toggleWidgets[it->second]->setIsChecked(o.defaultValue);
		}
	}
	updateResetButtonStatus();
}

void WzCampaignTweakOptionsEditForm::updateResetButtonStatus()
{
	bool hasSettingThatDiffersFromDefaults = std::any_of(tweakOptions->begin(), tweakOptions->end(), [](const WzCampaignTweakOptionSetting& o) -> bool {
		return o.currentValue != o.defaultValue;
	});
	resetButton->setState((hasSettingThatDiffersFromDefaults) ? 0 : WBUT_DISABLE);
}

class WzCampaignTweakOptionsSummaryWidget : public WIDGET
{
protected:
	WzCampaignTweakOptionsSummaryWidget() {}

	void initialize();
public:
	static std::shared_ptr<WzCampaignTweakOptionsSummaryWidget> make()
	{
		class make_shared_enabler: public WzCampaignTweakOptionsSummaryWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	int32_t idealHeight() override;
public:
	typedef std::function<void ()> InfoClickHandler;
	void setInfoClickHandler(const InfoClickHandler& func);
	void update(const std::vector<WzCampaignTweakOptionSetting>& tweakOptions);

protected:
	void geometryChanged() override;
private:
	std::shared_ptr<ClickableScrollableList> tweakOptionsList;
	std::vector<std::shared_ptr<W_LABEL>> tweakOptionsLabels;
	std::shared_ptr<WzInfoButton> infoButton;
	InfoClickHandler infoClickHandler;
	int betweenRowPadding = 0;
	int topPadding = 8;
	int bottomPadding = 5;
	int internalHorizontalPadding = 10;
};

int32_t WzCampaignTweakOptionsSummaryWidget::idealHeight()
{
	// the height for two rows
	int32_t rowHeight = iV_GetTextLineSize(font_small);
	return (topPadding + bottomPadding) + (rowHeight * 2) + betweenRowPadding;
}

void WzCampaignTweakOptionsSummaryWidget::geometryChanged()
{
	for (const auto& child : children())
	{
		child->callCalcLayout();
	}
}

void WzCampaignTweakOptionsSummaryWidget::setInfoClickHandler(const InfoClickHandler& func)
{
	infoClickHandler = func;
	if (infoClickHandler)
	{
		infoButton->show();
	}
	else
	{
		infoButton->hide();
	}
}

class FixedIdealWidthLabel : public W_LABEL
{
public:
	FixedIdealWidthLabel(int32_t fixedIdealWidth = -1)
	: W_LABEL()
	, m_fixedIdealWidth(fixedIdealWidth)
	{ }

	virtual int32_t idealWidth() override
	{
		return (m_fixedIdealWidth >= 0) ? m_fixedIdealWidth : W_LABEL::idealWidth();
	}
private:
	int32_t m_fixedIdealWidth = -1;
};

void WzCampaignTweakOptionsSummaryWidget::update(const std::vector<WzCampaignTweakOptionSetting>& tweakOptionSettings)
{
	auto makeTweakOptionSmallLabel = [](const WzString& str, int32_t fixedIdealWidth = -1) {
		auto label = std::make_shared<FixedIdealWidthLabel>(fixedIdealWidth);
		label->setFont(font_small, WZCOL_TEXT_MEDIUM);
		label->setString(str);
		label->setCanTruncate(true);
		label->setTransparentToMouse(true);
		label->setGeometry(0, 0, label->getMaxLineWidth(), iV_GetTextLineSize(font_small));
		return label;
	};

	tweakOptionsLabels.clear();

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);
	uint32_t col = 0;

	int32_t statusLabelWidth = std::max<int32_t>(iV_GetTextWidth("✓", font_small), iV_GetTextWidth("x", font_small));

	// Add list of tweak options (2 columns)
	for (const auto& setting : tweakOptionSettings)
	{
		if (!setting.editable)
		{
			continue;	// skip non-editable settings
		}
		auto newStatusLabel = makeTweakOptionSmallLabel((setting.currentValue) ? "✓" : "x", statusLabelWidth);
		auto newTweakLabel = makeTweakOptionSmallLabel(setting.displayName);
		grid->place({col++, 1, false}, row, Margin(0, 3, 0, 0).wrap(newStatusLabel));
		grid->place({col++, 1, true}, row, newTweakLabel);
		if (col >= 3)
		{
			row.start++;
			col = 0;
		}
		tweakOptionsLabels.push_back(newStatusLabel);
		tweakOptionsLabels.push_back(newTweakLabel);
	}

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());
	grid->setTransparentToMouse(true);

	tweakOptionsList->clear();
	tweakOptionsList->addItem(grid);
}

void WzCampaignTweakOptionsSummaryWidget::initialize()
{
	infoButton = WzInfoButton::make();
	infoButton->setImageDimensions(16);
	attach(infoButton);
	infoButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WzCampaignTweakOptionsSummaryWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int w = 16 + psParent->internalHorizontalPadding;
		int x0 = psParent->width() - w;
		int h = psParent->height() - (psParent->topPadding + psParent->bottomPadding);
		psWidget->setGeometry(x0, psParent->topPadding, w, h);
	}));
	infoButton->hide();
	auto weakSelf = std::weak_ptr<WzCampaignTweakOptionsSummaryWidget>(std::dynamic_pointer_cast<WzCampaignTweakOptionsSummaryWidget>(shared_from_this()));
	infoButton->addOnClickHandler([weakSelf](W_BUTTON&) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		if (strongSelf->infoClickHandler)
		{
			strongSelf->infoClickHandler();
		}
	});

	tweakOptionsList = ClickableScrollableList::make();
	attach(tweakOptionsList);
	tweakOptionsList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WzCampaignTweakOptionsSummaryWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int w = psParent->width() - psParent->infoButton->width() - (psParent->internalHorizontalPadding * 2);
		int h = psParent->height() - (psParent->topPadding + psParent->bottomPadding);
		psWidget->setGeometry(psParent->internalHorizontalPadding, psParent->topPadding, w, h);
	}));
	tweakOptionsList->setOnClickHandler([weakSelf](ClickableScrollableList&){
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		if (strongSelf->infoClickHandler)
		{
			strongSelf->infoClickHandler();
		}
	});
	tweakOptionsList->setOnHighlightHandler([weakSelf](ClickableScrollableList&, bool isHighlighted) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		PIELIGHT labelFontColor = (isHighlighted) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
		for (auto& label : strongSelf->tweakOptionsLabels)
		{
			label->setFontColour(labelFontColor);
		}
	});
}

// MARK: - WzCampaignTitleWidget

class WzCampaignTitleWidget : public WIDGET
{
protected:
	WzCampaignTitleWidget() {}
	void initialize(optional<WzCampaignModInfo> modInfo)
	{
		WzString displayName;
		WzString author;
		WzString description;
		WzString version;
		if (modInfo.has_value())
		{
			// load details from mod
			displayName = WzString::fromUtf8(modInfo.value().name);
			author = WzString::fromUtf8(modInfo.value().author);
			description = WzString::fromUtf8(modInfo.value().description.getLocalizedString());
			if (!modInfo.value().modBannerPNGData.empty())
			{
				iV_Image ivImage;
				auto result = iV_loadImage_PNG(modInfo.value().modBannerPNGData, &ivImage);
				if (result.noError())
				{
					pBackgroundTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(ivImage), gfx_api::texture_type::user_interface, "mem::mod_banner_png");
				}
				else
				{
					debug(LOG_ERROR, "Failed to load image from memory buffer: %s", result.text.c_str());
				}
			}
			version = WzString::fromUtf8(modInfo.value().version.value_or(""));
		}
		else
		{
			// load built-in details for original campaign
			displayName = _("Original Campaign");
			author = "Warzone 2100";
			description = _("Command the forces of The Project in a battle to rebuild the world");
			version = version_getVersionString();
			pBackgroundTexture = gfx_api::context::get().loadTextureFromFile("images/wz_original_campaign_banner.png", gfx_api::texture_type::user_interface, -1, -1, true);
		}

		// Create all the widgets and position them

		nameLabel = std::make_shared<W_LABEL>();
		nameLabel->setFont(font_large, WZCOL_TEXT_BRIGHT);
		nameLabel->setCanTruncate(true);
		nameLabel->setString(displayName);
		nameLabel->setGeometry(leftPadding, topPadding, nameLabel->getMaxLineWidth(), iV_GetTextLineSize(font_large));
		attach(nameLabel);
		nameLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto parent = psWidget->parent();
			ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
			int w = std::max(parent->width() - (psWidget->x() * 2), 0);
			psWidget->setGeometry(psWidget->x(), psWidget->y(), w, psWidget->height());
		}));

		int nextLineY0 = nameLabel->y() + nameLabel->height() + titleAuthorPadding;
		auto authorLabel = std::make_shared<W_LABEL>();
		authorLabel->setFont(font_regular_bold, WZCOL_TEXT_BRIGHT);
		authorLabel->setCanTruncate(true);
		authorLabel->setString(author);
		authorLabel->setGeometry(leftPadding, nextLineY0, authorLabel->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
		attach(authorLabel);
		authorLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto parent = psWidget->parent();
			ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
			int w = std::max(parent->width() - (psWidget->x() * 2), 0);
			psWidget->setGeometry(psWidget->x(), psWidget->y(), w, psWidget->height());
		}));

		nextLineY0 = authorLabel->y() + authorLabel->height() + 20;
		auto descriptionWidget = std::make_shared<Paragraph>();
		descriptionWidget->setFont(font_regular);
		descriptionWidget->setFontColour(WZCOL_TEXT_BRIGHT);
		if (pBackgroundTexture)
		{
			descriptionWidget->setShadeColour(pal_RGBA(0,0,0,80));
		}
		descriptionWidget->setGeometry(leftPadding, nextLineY0, 400, 40);
		descriptionWidget->addText(description);

		auto scrollableDescriptionWidget = ScrollableListWidget::make();
		scrollableDescriptionWidget->addItem(descriptionWidget);
		scrollableDescriptionWidget->setGeometry(leftPadding, nextLineY0, 400, 45);
		attach(scrollableDescriptionWidget);

		scrollableDescriptionWidget->setCalcLayout([descriptionWidget](WIDGET *psWidget){
			auto parent = psWidget->parent();
			ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
			if (parent->width() <= 1 || parent->height() <= 1)
			{
				return;
			}
			int w = std::max(parent->width() - (psWidget->x() * 2), 0);
			if (w == 0)
			{
				return;
			}
			psWidget->setGeometry(psWidget->x(), psWidget->y(), w, 45);
		});
	}
public:
	~WzCampaignTitleWidget()
	{
		if (pBackgroundTexture) { delete pBackgroundTexture; pBackgroundTexture = nullptr; }
	}
public:
	static std::shared_ptr<WzCampaignTitleWidget> make(optional<WzCampaignModInfo> modInfo)
	{
		class make_shared_enabler: public WzCampaignTitleWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(modInfo);
		return widget;
	}
protected:
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
private:
	int leftPadding = 40;
	int topPadding = 20;
	int titleAuthorPadding = 3;
	gfx_api::texture* pBackgroundTexture = nullptr;
	std::shared_ptr<W_LABEL> nameLabel;
};

void WzCampaignTitleWidget::geometryChanged()
{
	for (const auto& child : children())
	{
		child->callCalcLayout();
	}
}

void WzCampaignTitleWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	// Draw the background
	if (pBackgroundTexture)
	{
		iV_DrawImageAnisotropic(*pBackgroundTexture, Vector2i(x0, y0), Vector2f(0.f,0.f), Vector2f(width(), height()), 0.f, pal_RGBA(255, 255, 255, 255));
	}
	else
	{
		// Draw a static background
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), WZCOL_TRANSPARENT_BOX);
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0,0,0,80));
	}
}

// MARK: - WzCampaignSubInfoBox

class WzCampaignSubInfoBox : public WIDGET
{
protected:
	WzCampaignSubInfoBox() {}
	void initialize(optional<WzCampaignModInfo> modInfo)
	{
		WzCampaignUniverse universe;
		if (modInfo.has_value())
		{
			// load details from mod
			universe = modInfo.value().universe;
		}
		else
		{
			// load built-in details for original campaign
			universe = WzCampaignUniverse::WZ2100;
		}

		// Create the "universe" label
		WzString universeDisplayStr;
		switch (universe)
		{
			case WzCampaignUniverse::WZ2100:
				universeDisplayStr = _("Warzone 2100 Universe");
				pUniverseTexture = gfx_api::context::get().loadTextureFromFile("images/warzone2100.png", gfx_api::texture_type::game_texture, -1, -1, true);
				universeTextureColor = pal_RGBA(255,255,255,255);
				break;
			case WzCampaignUniverse::WZ2100Extended:
				universeDisplayStr = _("WZ2100 Extended Universe");
				pUniverseTexture = gfx_api::context::get().loadTextureFromFile("images/warzone2100.png", gfx_api::texture_type::game_texture, -1, -1, true);
				universeTextureColor = pal_RGBA(255,99,0,255);
				break;
			case WzCampaignUniverse::Unique:
				universeDisplayStr = _("Unique Universe");
				break;
		}

		universeLabel = std::make_shared<W_LABEL>();
		universeLabel->setFont(font_small, WZCOL_TEXT_BRIGHT);
		universeLabel->setCanTruncate(true);
		universeLabel->setString(universeDisplayStr);
		universeLabel->setGeometry(leftPadding, topPadding, universeLabel->getMaxLineWidth(), iV_GetTextLineSize(font_small));
		attach(universeLabel);
		universeLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto parent = psWidget->parent();
			ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
			int w = psWidget->width();
			int x = parent->width() - 40 - w;
			psWidget->setGeometry(x, 0, w, parent->height());
		}));

		if (modInfo.has_value())
		{
			// Also display compatibility info
			compatibilityResult = modInfo.value().getModCompatibility();
			compatibilityLabel = std::make_shared<W_LABEL>();
			compatibilityLabel->setFont(font_small, WZCOL_TEXT_BRIGHT);
			compatibilityLabel->setCanTruncate(true);
			std::string compatibilityStr;
			switch (compatibilityResult)
			{
				case WzModCompatibilityResult::NOT_COMPATIBLE:
				{
					std::string minVerString = "≥ ";
					minVerString += modInfo.value().minVersionSupported;
					compatibilityStr = astringf(_("Requires %s"), minVerString.c_str());
					break;
				}
				case WzModCompatibilityResult::OK:
					compatibilityStr = _("Compatible");
					break;
				case WzModCompatibilityResult::MAYBE_COMPATIBLE:
				{
					std::string verCompatRangeStr;
					if (!modInfo.value().maxVersionTested.empty())
					{
						verCompatRangeStr = modInfo.value().minVersionSupported + " - " + modInfo.value().maxVersionTested;
					}
					else
					{
						verCompatRangeStr = "≥ " + modInfo.value().minVersionSupported;
					}
					compatibilityStr = astringf(_("Compatible %s"), verCompatRangeStr.c_str());
					break;
				}
			}
			compatibilityLabel->setString(WzString::fromUtf8(compatibilityStr));
			compatibilityLabel->setGeometry(leftPadding, topPadding, compatibilityLabel->getMaxLineWidth(), iV_GetTextLineSize(font_small));
			attach(compatibilityLabel);
			compatibilityLabel->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
				auto parent = std::dynamic_pointer_cast<WzCampaignSubInfoBox>(psWidget->parent());
				ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
				int w = psWidget->width();
				int x = 40 + parent->imgDimensions + parent->imgTextPadding;
				psWidget->setGeometry(x, 0, w, parent->height());
			}));
		}
	}
public:
	static std::shared_ptr<WzCampaignSubInfoBox> make(optional<WzCampaignModInfo> modInfo)
	{
		class make_shared_enabler: public WzCampaignSubInfoBox {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(modInfo);
		return widget;
	}
public:
	~WzCampaignSubInfoBox()
	{
		if (pUniverseTexture) { delete pUniverseTexture; pUniverseTexture = nullptr; }
	}
protected:
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
private:
	int leftPadding = 40;
	int topPadding = 20;
	int imgDimensions = 16;
	int imgTextPadding = 5;
	gfx_api::texture* pUniverseTexture = nullptr;
	PIELIGHT universeTextureColor = pal_RGBA(255,255,255,255);
	std::shared_ptr<W_LABEL> universeLabel;
	WzModCompatibilityResult compatibilityResult = WzModCompatibilityResult::OK;
	std::shared_ptr<W_LABEL> compatibilityLabel;
};

void WzCampaignSubInfoBox::geometryChanged()
{
	imgDimensions = std::max<int>(std::min<int>({width(), height() - 2, 14}), 0);
	for (const auto& child : children())
	{
		child->callCalcLayout();
	}
}

void WzCampaignSubInfoBox::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0,0,0,128));

	int imgPosY0 = y0 + (height() - imgDimensions) / 2;

	// draw universe image
	int imgPosX0 = x0 + (universeLabel->x() - (imgDimensions + imgTextPadding));
	if (pUniverseTexture)
	{
		iV_DrawImageAnisotropic(*pUniverseTexture, Vector2i(imgPosX0, imgPosY0), Vector2f(0.f,0.f), Vector2f(imgDimensions + 1, imgDimensions + 1), 0.f, universeTextureColor);
	}
	else
	{
		// draw "other" universe image
		iV_DrawImageFileAnisotropicTint(FrontImages, IMAGE_CAM_UNIVERSE_STARS, imgPosX0, imgPosY0, Vector2f(imgDimensions, imgDimensions), pal_RGBA(255,255,255,200));
	}

	if (compatibilityLabel)
	{
		// draw compatibility image
		int compatImgPosX0 = x0 + (compatibilityLabel->x() - (imgDimensions + imgTextPadding));
		UWORD img = 0;
		PIELIGHT imgColor;
		switch (compatibilityResult)
		{
			case WzModCompatibilityResult::NOT_COMPATIBLE:
				img = IMAGE_BAN_CIRCLE;
				imgColor = pal_RGBA(191, 2, 2, 255);
				break;
			case WzModCompatibilityResult::OK:
				img = IMAGE_CHECK_CIRCLE;
				imgColor = pal_RGBA(0, 168, 10, 255);
				break;
			case WzModCompatibilityResult::MAYBE_COMPATIBLE:
				img = IMAGE_QUESTION_CIRCLE;
				imgColor = pal_RGBA(150, 150, 150, 255);
				break;
		}
		iV_DrawImageFileAnisotropicTint(FrontImages, img, compatImgPosX0, imgPosY0, Vector2f(imgDimensions, imgDimensions), imgColor);
	}
}

// MARK: - CampaignStartOptionsForm

static std::vector<WzCampaignTweakOptionSetting> buildTweakOptionSettings(optional<WzCampaignModInfo> modInfo)
{
	std::vector<WzCampaignTweakOptionSetting> results;

	// built-in, camTweakOptions

	results.emplace_back(
		"timerPowerBonus",
		_("Timer Power Bonus"),
		_("Grant power based on the remaining level timer, so there's no need to wait out the clock."),
		true, true
	);

	results.emplace_back(
		"classicTimers",
		_("Classic Timers"),
		_("Alter / disable timers on specific levels, like the original release. (Allows power-cheating.)"),
		false, true
	);

	results.emplace_back(
		"playerUnitCap40",
		_("40 Unit Limit"),
		_("Lower the player's unit limit to 40, matching the original PS1 release."),
		false, true
	);

	results.emplace_back(
		"autosavesOnly",
		_("Autosaves-Only"),
		_("Disable the ability to manually save / quick-save, limit autosaves to the beginning of each level."),
		false, true
	);

	results.emplace_back(
		"ps1Modifiers",
		_("PS1 Modifiers"),
		_("Reduces the damage the enemy deals to a third of the current difficulty modifier."),
		false, true
	);

	results.emplace_back(
		"infiniteTime",
		_("Infinite Time"),
		_("Missions have no timer where possible (Timer Power Bonus replaced with a small power reward if enabled)."),
		false, true
	);

	results.emplace_back(
		"victoryHints",
		_("Victory Hints"),
		_("Displays a console message every few minutes showing victory related information."),
		true, true
	);

	results.emplace_back(
		"gammaEndBonus",
		_("Finale Fun"),
		_("Activate Final Gamma mission bonus content."),
		true, true
	);

	results.emplace_back(
		"gammaBonusLevel",
		_("Gamma Bonus"),
		_("Extra Gamma mission making use of a map created by the original developement team (map updated by: DARwins)."),
		false, true
	);

	results.emplace_back(
		"insanePlus",
		_("Additional Spawns (Insane difficulty)"),
		_("Enables additional enemy spawns and behavior for Insane difficulty (or higher)."),
		true, true
	);

	results.emplace_back(
		"insanePlusLowDiff",
		_("Additional Spawns (Lower difficulties than Insane)"),
		_("Enables additional enemy spawns and behavior for Hard difficulty or lower."),
		false, true
	);

	results.emplace_back(
		"fastExp",
		_("Fast EXP gain"),
		_("Increases unit experience point gain by 2x."),
		false, true
	);

	results.emplace_back(
		"towerWars",
		_("Tower Wars"),
		_("Player gets significantly stronger structures."),
		false, true
	);

	if (modInfo.has_value())
	{
		for (auto it = results.begin(); it != results.end(); )
		{
			const auto& modCamTweakOptions = modInfo.value().camTweakOptions;
			const auto& uniqueIdentifier = it->uniqueIdentifier;
			auto modCamIt = std::find_if(modCamTweakOptions.begin(), modCamTweakOptions.end(), [uniqueIdentifier](const WzCampaignTweakOption& opt) -> bool {
				return opt.uniqueIdentifier == uniqueIdentifier;
			});
			if (modCamIt == modCamTweakOptions.end())
			{
				// builtin option not found in mod's camTweakOptions - remove it from list
				it = results.erase(it);
			}
			else
			{
				it->defaultValue = modCamIt->enabled;
				it->currentValue = modCamIt->enabled;
				it->editable = modCamIt->userEditable;
				++it;
			}
		}

		// append any customTweakOptions
		for (const auto& customOpt : modInfo.value().customTweakOptions)
		{
			auto localizedDisplayName = WzString::fromUtf8(customOpt.displayName.getLocalizedString());
			auto localizedDescription = WzString::fromUtf8(customOpt.description.getLocalizedString());
			if (localizedDisplayName.isEmpty())
			{
				continue;
			}
			if (customOpt.type != WzCampaignTweakOption::Type::Bool)
			{
				// this doesn't currently support anything else
				continue;
			}
			results.emplace_back(
				customOpt.uniqueIdentifier,
				localizedDisplayName,
				localizedDescription,
				customOpt.enabled, customOpt.userEditable
			);
		}
	}

	return results;
}

class CampaignStartOptionsForm : public WIDGET
{
protected:
	CampaignStartOptionsForm() {}

	void initialize(const std::shared_ptr<WzCampaignSelectorTitleUI>& parent);
public:
	static std::shared_ptr<CampaignStartOptionsForm> make(const std::shared_ptr<WzCampaignSelectorTitleUI>& parent, optional<WzCampaignModInfo> modInfo)
	{
		class make_shared_enabler: public CampaignStartOptionsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->modInfo = modInfo;
		widget->initialize(parent);
		return widget;
	}
	bool updateCampaignBalanceDropdown(const std::vector<WzCampaignModInfo>& availableCampaignMods);
protected:
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
private:
	std::shared_ptr<WIDGET> makeDifficultySelectorDropdown(const std::vector<DIFFICULTY_LEVEL>& availableDifficultyLevels);
	std::shared_ptr<WIDGET> makeCampaignSelectorDropdown();
	std::shared_ptr<WIDGET> makeCampaignBalanceDropdown();
	void openOverlay(const std::shared_ptr<WIDGET>& popoverForm, const std::shared_ptr<WIDGET>& psTarget, const std::function<void ()>& onCloseFunc);
	void closeOverlay();

	void updateTweakOptions();
	void openTweakOptionsEditFormOverlay();

	optional<WzCampaignModInfo> getCurrentBalanceModInfo();

	void handleStartGame();
private:
	std::shared_ptr<W_SCREEN> overlayScreen;
	std::function<void ()> overlayScreenOnCloseFunc;

	std::shared_ptr<WzCampaignTitleWidget> titleArea;
	std::shared_ptr<WzCampaignSubInfoBox> subInfoBox;

	optional<WzCampaignModInfo> modInfo;
	std::shared_ptr<DropdownWidget> campaignBalanceDropdown;
	std::shared_ptr<DropdownWidget> campaignSelectorDropdown;
	std::shared_ptr<WzCampaignTweakOptionsSummaryWidget> tweakOptionsSummary;

	std::vector<CAMPAIGN_FILE> campaignList;
	struct OriginalCampaignBalanceChoice
	{
		WzString displayName;
		WzString description;
		optional<WzCampaignModInfo> modInfo;
	};
	std::vector<OriginalCampaignBalanceChoice> campaignBalanceChoices;

	std::shared_ptr<std::vector<WzCampaignTweakOptionSetting>> tweakOptionSettings = std::make_shared<std::vector<WzCampaignTweakOptionSetting>>();
};

optional<WzCampaignModInfo> CampaignStartOptionsForm::getCurrentBalanceModInfo()
{
	if (campaignBalanceDropdown && !campaignBalanceChoices.empty())
	{
		// If an alternative balance is selected, get the balance mod
		const auto& balanceChoice = campaignBalanceChoices[campaignBalanceDropdown->getSelectedIndex().value_or(0)];
		return balanceChoice.modInfo;
	}
	return nullopt;
}

void CampaignStartOptionsForm::handleStartGame()
{
	ASSERT_OR_RETURN(, campaignSelectorDropdown != nullptr, "No campaign selector dropdown?");

	campaign_mods.clear();
	if (modInfo.has_value())
	{
		// Set campaign_mods
		campaign_mods.push_back(modInfo.value().modFilename);
		// Clear existing level list
		levShutDown();
		levInitialise();
		rebuildSearchPath(mod_campaign, true);
		pal_Init(); // Update palettes.
		// must reload campaign levels, as the campaign mod may provide its own levels
		if (!buildMapList(true))
		{
			debug(LOG_FATAL, "Failed to build map list");
			return;
		}
	}

	if (campaignBalanceDropdown && !campaignBalanceChoices.empty())
	{
		// If an alternative balance is selected, load the balance mod
		const auto& balanceChoice = campaignBalanceChoices[campaignBalanceDropdown->getSelectedIndex().value_or(0)];
		if (balanceChoice.modInfo.has_value())
		{
			// Load campaign balance mod
			campaign_mods.push_back(balanceChoice.modInfo.value().modFilename);
			// NOTE: Campaign balance mods shouldn't provide alternate levels, so there shouldn't be a need to reload levels
		}
	}

	SPinit(LEVEL_TYPE::CAMPAIGN);

	const auto& campaignFile = campaignList[campaignSelectorDropdown->getSelectedIndex().value_or(0)];

	sstrcpy(aLevelName, campaignFile.level.toUtf8().c_str());
	setCampaignName(campaignFile.name.toStdString());

	// show video
	if (!campaignFile.video.isEmpty())
	{
		seq_ClearSeqList();
		seq_AddSeqToList(campaignFile.video.toUtf8().c_str(), nullptr, campaignFile.captions.toUtf8().c_str(), false);
		seq_StartNextFullScreenVideo();
	}

	// set tweak options
	std::unordered_map<std::string, nlohmann::json> tweakOptionsJSONMap;
	for (const auto& o : *tweakOptionSettings)
	{
		tweakOptionsJSONMap[o.uniqueIdentifier] = nlohmann::json(o.currentValue);
	}
	setCamTweakOptions(tweakOptionsJSONMap);

	debug(LOG_WZ, "Loading campaign level -- %s", aLevelName);
	changeTitleMode(STARTGAME);
}

void CampaignStartOptionsForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), WZCOL_TRANSPARENT_BOX);

	int leftLineX0 = x0;
	int y1 = y0 + height();
	iV_Line(leftLineX0, y0, leftLineX0, y1, pal_RGBA(0,0,0,255));
}

const char* difficultyLevelToString(DIFFICULTY_LEVEL difficulty)
{
	switch (difficulty)
	{
	case DL_SUPER_EASY: return _("Super Easy");
	case DL_EASY: return _("Easy");
	case DL_NORMAL: return _("Normal");
	case DL_HARD: return _("Hard");
	case DL_INSANE: return _("Insane");
	}
	return _("Unsupported");
}

const char* getCampaignDifficultyDescriptionString(DIFFICULTY_LEVEL difficulty)
{
	switch (difficulty)
	{
	case DL_SUPER_EASY: return _("A more relaxed playthrough - easier than easy");
	case DL_EASY: return _("A slightly easier challenge than Normal");
	case DL_NORMAL: return _("A fun & challenging experience");
	case DL_HARD: return _("A challenge for players who have beaten the game");
	case DL_INSANE: return _("An unforgiving challenge for experienced players only");
	}
	return _("Unsupported");
}

std::shared_ptr<W_BUTTON> makeCampaignUITextButton(UDWORD id, const std::string &txt, unsigned int style, optional<unsigned int> minimumWidth = nullopt)
{
	W_BUTINIT sButInit;

	// Align
	sButInit.FontID = font_regular_bold;
	if (!(style & WBUT_TXTCENTRE))
	{
		auto textWidth = iV_GetTextWidth(txt.c_str(), sButInit.FontID);
		sButInit.width = (short)std::max<unsigned int>(minimumWidth.value_or(0), textWidth);
	}
	else
	{
		sButInit.style |= WBUT_TXTCENTRE;
	}

	// Enable right clicks
	if (style & WBUT_SECONDARY)
	{
		sButInit.style |= WBUT_SECONDARY;
	}

	sButInit.UserData = (style & WBUT_DISABLE); // store disable state
	auto pUserData = new DisplayTextOptionCache();
	pUserData->overflowBehavior = DisplayTextOptionCache::OverflowBehavior::ShrinkFont;
	sButInit.pUserData = pUserData;
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	sButInit.height = FRONTEND_CAMPAIGNUI_BUTHEIGHT;
	sButInit.pDisplay = displayTextOption;
	sButInit.pText = txt.c_str();

	auto button = std::make_shared<W_BUTTON>(&sButInit);
	// Disable button
	if (style & WBUT_DISABLE)
	{
		button->setState(WBUT_DISABLE);
	}

	return button;
}

std::shared_ptr<WIDGET> CampaignStartOptionsForm::makeDifficultySelectorDropdown(const std::vector<DIFFICULTY_LEVEL>& availableDifficultyLevels)
{
	std::vector<std::tuple<WzString, DIFFICULTY_LEVEL, WzString>> dropDownChoices;
	for (const auto& level : availableDifficultyLevels)
	{
		dropDownChoices.push_back({ difficultyLevelToString(level), level, getCampaignDifficultyDescriptionString(level) });
	}

	// If current value (from config) is not one of the presets in dropDownChoices, add a "Custom" entry
	size_t currentSettingIdx = 0;
	DIFFICULTY_LEVEL currValue = getDifficultyLevel();
	auto it = std::find_if(dropDownChoices.begin(), dropDownChoices.end(), [currValue](const std::tuple<WzString, DIFFICULTY_LEVEL, WzString>& item) -> bool {
		return std::get<1>(item) == currValue;
	});
	if (it != dropDownChoices.end())
	{
		currentSettingIdx = it - dropDownChoices.begin();
	}
	else
	{
		// Future TODO: If the current difficulty setting doesn't exist in availableDifficultyLevels, find the nearest available option
		currentSettingIdx = 0;
	}

	auto dropdown = std::make_shared<DropdownWidget>();

	int maxItemHeight = 0;
	for (const auto& option : dropDownChoices)
	{
		auto item = WzCampaignOptionButton::make();
		item->setString(std::get<0>(option));
		item->setDescriptionText(std::get<2>(option));
		item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
		maxItemHeight = std::max(maxItemHeight, item->idealHeight());
		dropdown->addItem(item);
	}

	dropdown->setListHeight(maxItemHeight * std::min<uint32_t>(4, dropDownChoices.size()));

	dropdown->setSelectedIndex(currentSettingIdx);

	dropdown->setCanChange([dropDownChoices](DropdownWidget &widget, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget) -> bool {
		ASSERT_OR_RETURN(false, newIndex < dropDownChoices.size(), "Invalid index");
		auto newDifficultyLevel = std::get<1>(dropDownChoices.at(newIndex));
		setDifficultyLevel(newDifficultyLevel);
		return true;
	});

	return Margin(0, 10).wrap(dropdown);
}

bool CampaignStartOptionsForm::updateCampaignBalanceDropdown(const std::vector<WzCampaignModInfo>& availableCampaignMods)
{
	ASSERT_OR_RETURN(false, campaignBalanceDropdown != nullptr, "No campaign balance dropdown?");

	if (campaignBalanceChoices.size() > 1)
	{
		campaignBalanceChoices.resize(1); // erase all but first element, and repopulate
	}

	campaignBalanceChoices.push_back({_("Classic"), _("\"Classic\" campaign balance"), nullopt});
	constexpr size_t classicModIdx = 1;

	for (const auto& mod : availableCampaignMods)
	{
		if (mod.type != WzModType::CampaignBalanceMod)
		{
			continue;
		}
		if (mod.modFilename == WZ2100_CORE_CLASSIC_BALANCE_MOD_FILENAME)
		{
			// already added above in all cases - just update the modinfo
			campaignBalanceChoices[classicModIdx].modInfo = mod;
			continue;
		}
		campaignBalanceChoices.push_back({WzString::fromUtf8(mod.name), WzString::fromUtf8(mod.description.getLocalizedString()), mod});
	}

	campaignBalanceDropdown->clear();

	int maxItemHeight = 0;
	for (size_t i = 0; i < campaignBalanceChoices.size(); ++i)
	{
		auto& option = campaignBalanceChoices[i];
		auto item = WzCampaignOptionButton::make();
		item->setString(option.displayName);
		item->setDescriptionText(option.description);
		item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
		maxItemHeight = std::max(maxItemHeight, item->idealHeight());

		if (i == classicModIdx && (!option.modInfo.has_value()))
		{
			// disable classic balance mod, which is always displayed, if unavailable
			item->setState(WBUT_DISABLE);
		}

		if (option.modInfo.has_value() && option.modInfo.value().getModCompatibility() == WzModCompatibilityResult::NOT_COMPATIBLE)
		{
			// disable incompatible mods
			item->setState(WBUT_DISABLE);
		}

		campaignBalanceDropdown->addItem(item);
	}

	campaignBalanceDropdown->setListHeight(maxItemHeight * std::min<uint32_t>(4, campaignBalanceChoices.size()));

	campaignBalanceDropdown->setSelectedIndex(0);

	return true;
}

std::shared_ptr<WIDGET> CampaignStartOptionsForm::makeCampaignBalanceDropdown()
{
	campaignBalanceChoices = {
		{_("Remastered"), _("The remastered campaign experience"), nullopt},
		{_("Classic"), _("\"Classic\" campaign balance"), nullopt},
	};
	constexpr size_t classicModIdx = 1;

	campaignBalanceDropdown = std::make_shared<DropdownWidget>();

	int maxItemHeight = 0;
	for (size_t i = 0; i < campaignBalanceChoices.size(); ++i)
	{
		auto& option = campaignBalanceChoices[i];
		auto item = WzCampaignOptionButton::make();
		item->setString(option.displayName);
		item->setDescriptionText(option.description);
		item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
		maxItemHeight = std::max(maxItemHeight, item->idealHeight());

		if (i == classicModIdx && (!option.modInfo.has_value()))
		{
			// disable classic balance mod (which is always displayed) by default - updateCampaignBalanceDropdown handles enabling if available
			item->setState(WBUT_DISABLE);
		}

		campaignBalanceDropdown->addItem(item);
	}

	campaignBalanceDropdown->setListHeight(maxItemHeight * std::min<uint32_t>(4, campaignBalanceChoices.size()));

	campaignBalanceDropdown->setSelectedIndex(0);

	campaignBalanceDropdown->setCanChange([](DropdownWidget &widget, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget) -> bool {
		ASSERT_OR_RETURN(false, newSelectedWidget != nullptr, "Nothing selected?");
		bool isDisabled = (newSelectedWidget->getState() & WBUT_DISABLE) != 0;
		return !isDisabled;
	});

	auto weakSelf = std::weak_ptr<CampaignStartOptionsForm>(std::dynamic_pointer_cast<CampaignStartOptionsForm>(shared_from_this()));
	campaignBalanceDropdown->setOnChange([weakSelf](DropdownWidget &widget) {
		widgScheduleTask([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
			strongSelf->updateTweakOptions();
		});
	});

	return Margin(0, 10).wrap(campaignBalanceDropdown);
}

std::shared_ptr<WIDGET> CampaignStartOptionsForm::makeCampaignSelectorDropdown()
{
	campaignSelectorDropdown = std::make_shared<DropdownWidget>();
	campaignSelectorDropdown->setListHeight(FRONTEND_CAMPAIGNUI_BUTHEIGHT * std::min<uint32_t>(5, campaignList.size()));
	const auto paddingSize = 10;

	for (const auto& option : campaignList)
	{
		auto item = makeCampaignUITextButton(0, option.name.toUtf8(), 0);
		campaignSelectorDropdown->addItem(Margin(0, paddingSize).wrap(item));
	}

	if (!campaignList.empty())
	{
		campaignSelectorDropdown->setSelectedIndex(0);
	}

	return Margin(0, 10).wrap(campaignSelectorDropdown);
}

void CampaignStartOptionsForm::updateTweakOptions()
{
	if (tweakOptionsSummary)
	{
		if (modInfo.has_value())
		{
			*tweakOptionSettings = buildTweakOptionSettings(modInfo.value());
		}
		else
		{
			*tweakOptionSettings = buildTweakOptionSettings(getCurrentBalanceModInfo());
		}
		tweakOptionsSummary->update(*tweakOptionSettings);
	}
}

void CampaignStartOptionsForm::closeOverlay()
{
	if (!overlayScreen)
	{
		return;
	}
	widgRemoveOverlayScreen(overlayScreen);
	overlayScreen.reset();
	if (overlayScreenOnCloseFunc)
	{
		overlayScreenOnCloseFunc();
	}
}

void CampaignStartOptionsForm::openOverlay(const std::shared_ptr<WIDGET>& popoverForm, const std::shared_ptr<WIDGET>& psTarget, const std::function<void ()>& onCloseFunc)
{
	ASSERT_OR_RETURN(, overlayScreen == nullptr, "Overlay already open?");

	auto lockedScreen = screenPointer.lock();
	ASSERT_OR_RETURN(, lockedScreen != nullptr, "The CampaignStartOptionsForm does not have an associated screen pointer?");

	// Initialize the options overlay screen
	overlayScreen = W_SCREEN::make();
	overlayScreenOnCloseFunc = onCloseFunc;
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	std::weak_ptr<W_SCREEN> psWeakOverlayScreen(overlayScreen);
	std::weak_ptr<CampaignStartOptionsForm> psWeakSelf(std::dynamic_pointer_cast<CampaignStartOptionsForm>(shared_from_this()));
	newRootFrm->onClickedFunc = [psWeakOverlayScreen, psWeakSelf]() {
		// Destroy overlay screen
		if (auto strongSelf = psWeakSelf.lock())
		{
			strongSelf->closeOverlay();
		}
		else
		{
			// backup in case there's a lingering overlay screen
			if (auto psOverlayScreen = psWeakOverlayScreen.lock())
			{
				widgRemoveOverlayScreen(psOverlayScreen);
			}
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	overlayScreen->psForm->attach(newRootFrm);

	// Attach the pop-over form
	newRootFrm->attach(popoverForm);

	// Position the pop-over form above the target widget
	popoverForm->setCalcLayout([psWeakSelf](WIDGET *psWidget) {
		auto strongSelf = psWeakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "parent is null");
		// (Ideally, with its left aligned with the left side of the "CampaignStartOptionsForm" widget, but ensure full visibility on-screen)
		int popOverX0 = strongSelf->screenPosX() + (strongSelf->width() - psWidget->width()) / 2;
		if (popOverX0 + psWidget->width() > screenWidth)
		{
			popOverX0 = screenWidth - psWidget->width();
		}
		// (Ideally, with its middle centered with the middle of the "CampaignStartOptionsForm" widget, but ensure full visibility on-screen)
		int popOverY0 = std::max<int>(strongSelf->screenPosY() + (strongSelf->height() - psWidget->height()) / 2, 0);
		if (popOverY0 + psWidget->height() > screenHeight)
		{
			popOverY0 = screenHeight - psWidget->height();
		}
		psWidget->move(popOverX0, popOverY0);
	});

	widgRegisterOverlayScreenOnTopOfScreen(overlayScreen, lockedScreen);
}

void CampaignStartOptionsForm::openTweakOptionsEditFormOverlay()
{
	ASSERT_OR_RETURN(, overlayScreen == nullptr, "Overlay already open?");

	std::weak_ptr<CampaignStartOptionsForm> psWeakSelf(std::dynamic_pointer_cast<CampaignStartOptionsForm>(shared_from_this()));
	auto optionsPopOver = WzCampaignTweakOptionsEditForm::make(tweakOptionSettings, [psWeakSelf](){
		if (auto strongSelf = psWeakSelf.lock())
		{
			strongSelf->closeOverlay();
		}
	});
	auto maxPopoverHeight = height() - 50;
	optionsPopOver->setGeometry(0, 0, width() - 20, maxPopoverHeight); // set once to set width
	optionsPopOver->setGeometry(0, 0, optionsPopOver->width(), std::min<int32_t>(maxPopoverHeight, optionsPopOver->idealHeight())); // set again to set height based on idealHeight for that width (if smaller)

	auto psTarget = tweakOptionsSummary;
	std::weak_ptr<CampaignStartOptionsForm> weakSelf(std::dynamic_pointer_cast<CampaignStartOptionsForm>(shared_from_this()));
	openOverlay(optionsPopOver, psTarget, [weakSelf]() {
		if (auto strongSelf = weakSelf.lock())
		{
			// refresh any possibly-edited data
			strongSelf->tweakOptionsSummary->update(*(strongSelf->tweakOptionSettings));
		}
	});
}

static std::shared_ptr<WIDGET> addCampaignUIMargin(std::shared_ptr<WIDGET> widget)
{
	return Margin(0, 10).wrap(widget);
}

class WZOptionTitleLabel : public WIDGET
{
protected:
	void display(int xOffset, int yOffset) override;
public:
	WzString getString() const override;
	void setString(WzString string) override;
	std::string getTip() override;
	void setFont(iV_fonts _newFont) { FontID = _newFont; }
	int32_t idealWidth() override;
private:
	WzText wzText;
	iV_fonts FontID = font_regular;
	int verticalPadding = 5;
	bool isTruncated = false;
};

int32_t WZOptionTitleLabel::idealWidth()
{
	return wzText.width();
}

WzString WZOptionTitleLabel::getString() const
{
	return wzText.getText();
}

void WZOptionTitleLabel::setString(WzString string)
{
	wzText.setText(string, font_regular);
}

std::string WZOptionTitleLabel::getTip()
{
	if (!isTruncated)
	{
		return "";
	}
	return wzText.getText().toUtf8();
}

void WZOptionTitleLabel::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	PIELIGHT textColor = WZCOL_TEXT_MEDIUM;

	int textX0 = x0;
	int textY0 = y0 + verticalPadding - wzText.aboveBase();

	int maxTextDisplayableWidth = width();
	isTruncated = maxTextDisplayableWidth < wzText.width();
	if (isTruncated)
	{
		maxTextDisplayableWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
	}
	wzText.render(textX0, textY0, textColor, 0.0f, maxTextDisplayableWidth);
	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxTextDisplayableWidth + 2, textY0), textColor);
	}
}

static std::shared_ptr<WZOptionTitleLabel> makeCampaignUIOptionTitleLabel(const WzString& text, iV_fonts font)
{
	auto psLabel = std::make_shared<WZOptionTitleLabel>();
	psLabel->setFont(font);
	psLabel->setString(text);
	psLabel->setGeometry(0, 0, psLabel->idealWidth(), iV_GetTextLineSize(font));
	return psLabel;
}

void CampaignStartOptionsForm::initialize(const std::shared_ptr<WzCampaignSelectorTitleUI>& parent)
{
	auto weakParent = std::weak_ptr<WzCampaignSelectorTitleUI>(parent);
	auto weakSelf = std::weak_ptr<CampaignStartOptionsForm>(std::dynamic_pointer_cast<CampaignStartOptionsForm>(shared_from_this()));

	titleArea = WzCampaignTitleWidget::make(modInfo);
	attach(titleArea);
	titleArea->setCalcLayout([](WIDGET *psWidget) {
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		psWidget->setGeometry(0, 0, psParent->width(), 155);
	});

	subInfoBox = WzCampaignSubInfoBox::make(modInfo);
	attach(subInfoBox);
	subInfoBox->setCalcLayout([](WIDGET *psWidget) {
		auto psParent = std::dynamic_pointer_cast<CampaignStartOptionsForm>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int y0 = psParent->titleArea->y() + psParent->titleArea->height();
		psWidget->setGeometry(0, y0, psParent->width(), 30);
	});

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	// Add Difficulty selector widget
	std::vector<DIFFICULTY_LEVEL> availableDifficultyLevels = { DL_SUPER_EASY, DL_EASY, DL_NORMAL, DL_HARD, DL_INSANE };
	if (modInfo.has_value() && !modInfo.value().difficultyLevels.empty())
	{
		std::vector<DIFFICULTY_LEVEL> filtered;
		const auto& modInfoValueRef = modInfo.value();
		std::copy_if(availableDifficultyLevels.begin(), availableDifficultyLevels.end(), std::back_inserter(filtered), [modInfoValueRef](DIFFICULTY_LEVEL lvl){ return modInfoValueRef.difficultyLevels.count(lvl) > 0; } );
		availableDifficultyLevels = std::move(filtered);
	}
	grid->place({0, 1, false}, row, addCampaignUIMargin(makeCampaignUIOptionTitleLabel(_("Difficulty:"), font_regular_bold)));
	grid->place({1, 1, true}, row, makeDifficultySelectorDropdown(availableDifficultyLevels));
	row.start++;

	// Only display Balance Choice: Remastered / Classic / etc if !modInfo.has_value() (i.e. if original campaign)
	if (!modInfo.has_value())
	{
		// Original Campaign - add balance dropdown
		grid->place({0, 1, false}, row, addCampaignUIMargin(makeCampaignUIOptionTitleLabel(_("Balance:"), font_regular_bold)));
		grid->place({1, 1, true}, row, makeCampaignBalanceDropdown());
		row.start++;
		updateCampaignBalanceDropdown(cached_availableCampaignMods);
	}

	// Add Campaign selector widget
	if (modInfo.has_value())
	{
		campaignList = modInfo.value().campaignFiles;
	}
	else
	{
		// read the base campaign list
		campaignList = readCampaignFiles();
	}
	grid->place({0, 1, false}, row, addCampaignUIMargin(makeCampaignUIOptionTitleLabel(_("Start at:"), font_regular_bold)));
	grid->place({1, 1, true}, row, makeCampaignSelectorDropdown());
	row.start++;

	// Add Tweak Options widget (optional)
	*tweakOptionSettings = buildTweakOptionSettings(modInfo);
	if (std::any_of(tweakOptionSettings->begin(), tweakOptionSettings->end(), [](const WzCampaignTweakOptionSetting& o) { return o.editable; }))
	{
		tweakOptionsSummary = WzCampaignTweakOptionsSummaryWidget::make();
		tweakOptionsSummary->update(*tweakOptionSettings);
		tweakOptionsSummary->setInfoClickHandler([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
			strongSelf->openTweakOptionsEditFormOverlay();
		});
		grid->place({0, 1, false}, row, addCampaignUIMargin(makeCampaignUIOptionTitleLabel(_("Tweaks:"), font_regular_bold)));
		grid->place({1, 1, true}, row, Margin(0, 10).wrap(tweakOptionsSummary));
		row.start++;
	}

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->addItem(grid);
	scrollableList->setPadding({4, 4, 4, 4});
	attach(scrollableList);
	scrollableList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<CampaignStartOptionsForm>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int y0 = psParent->titleArea->y() + psParent->titleArea->height() + 40;
		psWidget->setGeometry(5, y0, psParent->width() - 10, 160);
	}));

	auto psStartNewGameButton = CampaignStartButton::make();
	attach(psStartNewGameButton);
	psStartNewGameButton->setCalcLayout([](WIDGET *psWidget) {
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int h = psWidget->height();
		int y0 = psParent->height() - h - 20;
		psWidget->setGeometry(30, y0, std::max<int>(psParent->width() - 60, 50), psWidget->height());
	});
	psStartNewGameButton->addOnClickHandler([weakSelf](W_BUTTON&) {
		widgScheduleTask([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
			strongSelf->handleStartGame();
		});
	});
	if (modInfo.has_value())
	{
		if (modInfo.value().getModCompatibility() == WzModCompatibilityResult::NOT_COMPATIBLE)
		{
			psStartNewGameButton->setState(WBUT_DISABLE);
		}
	}
}

void CampaignStartOptionsForm::geometryChanged()
{
	for (const auto& child : children())
	{
		child->callCalcLayout();
	}
}

WzCampaignSelectorTitleUI::WzCampaignSelectorTitleUI(std::shared_ptr<WzTitleUI> parent)
	: parent(parent)
{
}

WzCampaignSelectorTitleUI::~WzCampaignSelectorTitleUI()
{
	cached_availableCampaignMods.clear();
}

void WzCampaignSelectorTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
}

void addCampaignBaseForm()
{
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	auto botForm = std::make_shared<IntFormAnimated>();
	parent->attach(botForm);
	botForm->id = FRONTEND_BOTFORM;

	botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(FRONTEND_BOTFORM_WIDEX, FRONTEND_TOPFORM_WIDEY, 590, 460);
	}));
}

void WzCampaignSelectorTitleUI::start()
{
	addBackdrop();
	addCampaignBaseForm();

	WIDGET *psBotForm = widgGetFromID(psWScreen, FRONTEND_BOTFORM);
	ASSERT(psBotForm != nullptr, "Unable to find FRONTEND_BOTFORM?");

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_TOPFORM_WIDEY, _("CAMPAIGNS"));

	auto weakSelf = std::weak_ptr<WzCampaignSelectorTitleUI>(std::dynamic_pointer_cast<WzCampaignSelectorTitleUI>(shared_from_this()));

	auto psQuitButton = addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	psQuitButton->addOnClickHandler([weakSelf](W_BUTTON& button) {
		widgScheduleTask([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent");
			changeTitleUI(strongSelf->parent); // go back
		});
	});
	int campaignSelectorY0 = psQuitButton->y() + psQuitButton->height() + 10;

	campaignSelector = WzCampaignSelectorForm::make(std::dynamic_pointer_cast<WzCampaignSelectorTitleUI>(shared_from_this()));
	psBotForm->attach(campaignSelector);
	campaignSelector->setCalcLayout([campaignSelectorY0](WIDGET *psWidget){
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int h = psParent->height() - campaignSelectorY0;
		psWidget->setGeometry(1, campaignSelectorY0, 180, h);
	});

	displayStartOptionsForm(nullopt);

	// Call loadCampaignModsAsync
	loadCampaignModsAsync([weakSelf](std::vector<WzCampaignModInfo> campaignMods) {
		wzAsyncExecOnMainThread([campaignMods, weakSelf]() {
			debug(LOG_INFO, "Found %zu campaign mods", campaignMods.size());

			auto strongSelf = weakSelf.lock();
			if (strongSelf == nullptr)
			{
				// nothing to do - the titleUI instance that triggered this async method not longer exists
				return;
			}

			cached_availableCampaignMods = std::move(campaignMods);

			if (!cached_availableCampaignMods.empty())
			{
				strongSelf->campaignSelector->updateList(cached_availableCampaignMods);
				// Update the original campaign panel's list of balance options (which is currently being displayed)
				auto pDisplayCampaignStartOptions = std::dynamic_pointer_cast<CampaignStartOptionsForm>(strongSelf->displayedPanel);
				if (pDisplayCampaignStartOptions)
				{
					pDisplayCampaignStartOptions->updateCampaignBalanceDropdown(cached_availableCampaignMods);
				}
			}
		});
	});

	// TODO: Display loading spinner?

	// show this only when the video sequences are not installed
	if (!seq_hasVideos())
	{
		notifyAboutMissingVideos();
	}
}

void WzCampaignSelectorTitleUI::displayStartOptionsForm(optional<WzCampaignModInfo> modInfo)
{
	WIDGET *psBotForm = widgGetFromID(psWScreen, FRONTEND_BOTFORM);
	ASSERT(psBotForm != nullptr, "Unable to find FRONTEND_BOTFORM?");

	if (displayedPanel)
	{
		psBotForm->detach(displayedPanel);
	}

	auto weakSelf = std::weak_ptr<WzCampaignSelectorTitleUI>(std::dynamic_pointer_cast<WzCampaignSelectorTitleUI>(shared_from_this()));

	displayedPanel = CampaignStartOptionsForm::make(std::dynamic_pointer_cast<WzCampaignSelectorTitleUI>(shared_from_this()), modInfo);
	psBotForm->attach(displayedPanel);
	displayedPanel->setCalcLayout([weakSelf](WIDGET *psWidget) {
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		auto strongTitleUI = weakSelf.lock();
		auto pLeftPanel = strongTitleUI->campaignSelector;
		int x = pLeftPanel->x() + pLeftPanel->width() + 1;
		int w = psParent->width() - x - 1;
		psWidget->setGeometry(x, 1, w, psParent->height() - 2);
	});
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
