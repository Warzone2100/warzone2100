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
/** @file
 *  Functionality for the guide screen overlay
 */

#include "guidescreen.h"
#include "../wzjsonlocalizedstring.h"

#include "lib/framework/wzglobal.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"
#include "lib/framework/wzpaths.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/math_ext.h"
#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/button.h"
#include "lib/widget/margin.h"
#include "lib/widget/checkbox.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/bitimage.h"

#include "../hci.h"
#include "../wzjsonhelpers.h"
#include "../keybind.h"
#include "../loop.h"
#include "../intdisplay.h"
#include "../intorder.h"
#include "../hci/objects_stats.h"
#include "../activity.h"
#include "../feature.h"
#include "../mission.h"

#include <unordered_map>
#include <locale>

struct WzGameGuideScreen;

#define WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN PACKAGE "_guide"

// MARK: - Guide Topic / Registry types

constexpr int GuideNavBarButtonHorizontalPadding = 5;

struct WzStatVisual
{
	std::string statName;

	enum class OnlyIfKnown
	{
		False,
		CampaignMode,
		True
	};
	OnlyIfKnown onlyIfKnown = OnlyIfKnown::False;
};

struct WzGuideVisual
{
	optional<std::string> imagePath;
	optional<WzStatVisual> statVisual;
	optional<int> reticuleButtonId;
	optional<SECONDARY_ORDER> secondaryUnitOrderButtons;
	optional<nlohmann::json> droidTemplateJSON;
};

struct WzGuideTopic
{
	std::string identifier;
	optional<std::string> sortKey;
	std::string version;
	WzJsonLocalizedString displayName;
	optional<std::unordered_set<ActivitySink::GameMode>> applicableModes;
	std::vector<WzGuideVisual> visuals;
	std::vector<WzJsonLocalizedString> contents;
	std::vector<std::string> linkedTopicIdentifiers;
};

class WzWrappedGuideTopic
{
public:
	WzWrappedGuideTopic(const std::shared_ptr<WzGuideTopic>& topic)
	: topic(topic)
	{ }
public:
	std::shared_ptr<WzGuideTopic> topic;
	std::weak_ptr<WzWrappedGuideTopic> parent;
	std::vector<std::shared_ptr<WzWrappedGuideTopic>> children;
private:
	friend class WzGuideTopicsRegistry;
	optional<ActivityDBProtocol::GuideTopicPrefs> prefs; // loaded on-demand, when needed for display - used directly by WzGuideTopicsRegistry only
};

class WzGuideTopicsRegistry
{
public:
	optional<std::string> getDisplayNameForTopic(const std::string& identifier) const;
	std::shared_ptr<WzWrappedGuideTopic> getTopic(const std::string& identifier) const;
	const std::vector<std::shared_ptr<WzWrappedGuideTopic>>& getTopics() const;
	bool hasTopics() const;
	bool hasTopic(const std::string& identifier) const;
	std::shared_ptr<WzWrappedGuideTopic> addTopic(const std::shared_ptr<WzGuideTopic>& topic);
	void clear();

	std::vector<std::string> saveLoadedTopics() const;
	bool restoreLoadedTopics(const std::vector<std::string>& topicIds);

	optional<ActivityDBProtocol::GuideTopicPrefs> getTopicPrefs(const std::shared_ptr<WzWrappedGuideTopic>& topic);
	void setTopicSeen(const std::shared_ptr<WzWrappedGuideTopic>& topic);

private:
	std::shared_ptr<WzWrappedGuideTopic> getCategoryTopic(const std::string& topicIdentifier);
	std::shared_ptr<WzWrappedGuideTopic> addTopicImpl(const std::shared_ptr<WzGuideTopic>& topic, bool restoringLoadedTopics = false);
	void enumerateLoadedTopics(const std::function<void (const std::shared_ptr<WzWrappedGuideTopic>&)>& enumFunc) const;
	void enumerateTopicsImpl(const std::vector<std::shared_ptr<WzWrappedGuideTopic>>& root, const std::function<void (const std::shared_ptr<WzWrappedGuideTopic>&)>& enumFunc) const;

private:
	std::vector<std::shared_ptr<WzWrappedGuideTopic>> topics;
	std::unordered_map<std::string, std::shared_ptr<WzWrappedGuideTopic>> identifierLookupMap;
	std::unordered_set<std::string> nonCategoryTopicIds;
	std::shared_ptr<ActivityDBProtocol> activityDB;
};

// MARK: - Globals

static std::shared_ptr<WzGameGuideScreen> psCurrentGameGuideScreen;
static std::shared_ptr<WzGuideTopicsRegistry> guideTopicsRegistry = std::make_shared<WzGuideTopicsRegistry>();
static bool disableTopicPopups = false;

// forward-declarations
std::shared_ptr<WzGuideTopic> loadWZGuideTopicByTopicId(const std::string& guideTopicId);

// MARK: - WzGuideTopicsRegistry

optional<std::string> WzGuideTopicsRegistry::getDisplayNameForTopic(const std::string& identifier) const
{
	auto it = identifierLookupMap.find(identifier);
	if (it == identifierLookupMap.end())
	{
		return nullopt;
	}
	return it->second->topic->displayName.getLocalizedString(WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN);
}

std::shared_ptr<WzWrappedGuideTopic> WzGuideTopicsRegistry::getTopic(const std::string& identifier) const
{
	auto it = identifierLookupMap.find(identifier);
	if (it == identifierLookupMap.end())
	{
		return nullptr;
	}
	return it->second;
}

const std::vector<std::shared_ptr<WzWrappedGuideTopic>>& WzGuideTopicsRegistry::getTopics() const
{
	return topics;
}

bool WzGuideTopicsRegistry::hasTopics() const
{
	return !topics.empty();
}

bool WzGuideTopicsRegistry::hasTopic(const std::string& identifier) const
{
	return identifierLookupMap.find(identifier) != identifierLookupMap.end();
}

std::shared_ptr<WzWrappedGuideTopic> WzGuideTopicsRegistry::getCategoryTopic(const std::string& topicIdentifier)
{
	if (topicIdentifier.empty())
	{
		return nullptr;
	}
	const std::string topicDelimeter = "::";
	auto lastComponentPos = std::string::npos;
	while (lastComponentPos > 0)
	{
		lastComponentPos = topicIdentifier.rfind(topicDelimeter, lastComponentPos - 1);
		if (lastComponentPos == std::string::npos)
		{
			break;
		}
		auto possibleParent = topicIdentifier.substr(0, lastComponentPos);
		auto parentIt = identifierLookupMap.find(possibleParent);
		if (parentIt != identifierLookupMap.end())
		{
			debug(LOG_WZ, "Found parent: %s", possibleParent.c_str());
			return parentIt->second;
		}
		else
		{
			// parent guide category topic not found
			if (nonCategoryTopicIds.count(possibleParent) > 0)
			{
				// already checked this - doesn't exist - continue
				continue;
			}
			else
			{
				// did not already check for an _index.json underneath this category - try to load
				auto categoryTopic = loadWZGuideTopicByTopicId(possibleParent);
				if (!categoryTopic)
				{
					debug(LOG_WZ, "No category: %s", possibleParent.c_str());
					nonCategoryTopicIds.insert(possibleParent);
					continue;
				}
				// add the category topic to the topics list, and return it
				debug(LOG_WZ, "Loaded category: %s", possibleParent.c_str());
				return addTopicImpl(categoryTopic);
			}
		}
	}

	return nullptr;
}

std::shared_ptr<WzWrappedGuideTopic> WzGuideTopicsRegistry::addTopicImpl(const std::shared_ptr<WzGuideTopic>& topic, bool restoringLoadedTopics)
{
	ASSERT_OR_RETURN(nullptr, topic != nullptr, "Null topic");

	if (identifierLookupMap.find(topic->identifier) != identifierLookupMap.end())
	{
		debug(LOG_WZ, "Guide topic with identifier \"%s\" was already added", topic->identifier.c_str());
		return nullptr;
	}

	if (topic->applicableModes.has_value() && !topic->applicableModes.value().empty() && !restoringLoadedTopics)
	{
		if (topic->applicableModes.value().count(ActivityManager::instance().getCurrentGameMode()) == 0)
		{
			// current game mode is not in applicableModes - skipping add of topic
			debug(LOG_WZ, "Skipping topic (not applicable to current game mode): %s", topic->identifier.c_str());
			return nullptr;
		}
	}

	auto wrappedTopic = std::make_shared<WzWrappedGuideTopic>(topic);

	// Determine if it should be inserted underneath a category
	const auto& topicIdentifier = topic->identifier;
	auto parentTopic = getCategoryTopic(topicIdentifier);
	if (parentTopic)
	{
		parentTopic->children.push_back(wrappedTopic);
		wrappedTopic->parent = parentTopic;
	}
	else
	{
		// root level
		topics.push_back(wrappedTopic);
	}

	identifierLookupMap[topic->identifier] = wrappedTopic;
	return wrappedTopic;
}

std::shared_ptr<WzWrappedGuideTopic> WzGuideTopicsRegistry::addTopic(const std::shared_ptr<WzGuideTopic>& topic)
{
	return addTopicImpl(topic);
}

void WzGuideTopicsRegistry::clear()
{
	topics.clear();
	identifierLookupMap.clear();
}

std::vector<std::string> WzGuideTopicsRegistry::saveLoadedTopics() const
{
	std::vector<std::string> results;
	enumerateLoadedTopics([&results](const std::shared_ptr<WzWrappedGuideTopic>& wrappedTopic) {
		results.push_back(wrappedTopic->topic->identifier);
	});
	return results;
}

void WzGuideTopicsRegistry::enumerateTopicsImpl(const std::vector<std::shared_ptr<WzWrappedGuideTopic>>& root, const std::function<void (const std::shared_ptr<WzWrappedGuideTopic>&)>& enumFunc) const
{
	for (const auto& wrappedTopic : root)
	{
		if (!wrappedTopic->children.empty())
		{
			enumerateTopicsImpl(wrappedTopic->children, enumFunc);
		}
		enumFunc(wrappedTopic);
	}
}

void WzGuideTopicsRegistry::enumerateLoadedTopics(const std::function<void (const std::shared_ptr<WzWrappedGuideTopic>&)>& enumFunc) const
{
	enumerateTopicsImpl(topics, enumFunc);
}

bool WzGuideTopicsRegistry::restoreLoadedTopics(const std::vector<std::string>& topicIds)
{
	clear();
	for (const auto& topicId : topicIds)
	{
		auto loadedTopic = loadWZGuideTopicByTopicId(topicId);
		if (!loadedTopic)
		{
			debug(LOG_INFO, "Failed to open / load guide topic: %s", topicId.c_str());
			continue;
		}
		addTopicImpl(loadedTopic, true);
	}
	return true;
}

optional<ActivityDBProtocol::GuideTopicPrefs> WzGuideTopicsRegistry::getTopicPrefs(const std::shared_ptr<WzWrappedGuideTopic>& topic)
{
	// find cached data first
	ASSERT_OR_RETURN(nullopt, topic != nullptr, "Null topic");
	if (topic->prefs.has_value())
	{
		return topic->prefs;
	}
	if (!activityDB)
	{
		activityDB = ActivityManager::instance().getRecord();
		if (!activityDB)
		{
			// database access isn't available, but store the prefs for this run at least
			topic->prefs = ActivityDBProtocol::GuideTopicPrefs();
			return topic->prefs;
		}
	}
	topic->prefs = activityDB->getGuideTopicPrefs(topic->topic->identifier);
	if (!topic->prefs.has_value())
	{
		// initialize to defaults
		topic->prefs = ActivityDBProtocol::GuideTopicPrefs();
	}
	return topic->prefs;
}

void WzGuideTopicsRegistry::setTopicSeen(const std::shared_ptr<WzWrappedGuideTopic>& topic)
{
	ASSERT_OR_RETURN(, topic != nullptr, "Null topic");
	if (!topic->prefs.has_value())
	{
		// populate it from db
		getTopicPrefs(topic);
		ASSERT_OR_RETURN(, topic->prefs.has_value(), "Failed to initialize topic prefs?");
	}

	if (topic->prefs.value().lastReadVersion != topic->topic->version)
	{
		topic->prefs.value().lastReadVersion = topic->topic->version;

		if (!activityDB)
		{
			activityDB = ActivityManager::instance().getRecord(); // might return nullptr if database access isn't available
		}
		if (activityDB)
		{
			activityDB->setGuideTopicPrefs(topic->topic->identifier, topic->prefs.value());
		}
	}
}

// MARK: - Guide Topic display widget classes

class WzGuideTopicsNavButton;
class WzGuideTopicDisplayWidget;

class WzGuideTopicsNavBar : public WIDGET
{
public:
	typedef std::function<void (const std::string& topicIdentifier)> OnPopStateFunction;
	typedef std::function<void ()> OnSidebarToggleFunc;
	typedef std::function<void ()> OnCloseFunc;
protected:
	WzGuideTopicsNavBar(): WIDGET() {}
	void initialize(int32_t topicDisplayWidth, const OnPopStateFunction& popStateHandler, const OnSidebarToggleFunc& onSidebarToggleHandler, const OnCloseFunc& onCloseHandler);
protected:
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
public:
	static std::shared_ptr<WzGuideTopicsNavBar> make(int32_t topicDisplayWidth, const OnPopStateFunction& popStateHandler, const OnSidebarToggleFunc& onSidebarToggleHandler, const OnCloseFunc& onCloseHandler);

	~WzGuideTopicsNavBar();

	virtual int32_t idealHeight() override;

	void pushState(const std::string& topicIdentifier);

private:
	bool navBack();
	bool navForward();
	void updateButtonStates();
	void updateLayout();
	void triggerCloseHandler();
	void triggerSidebarToggleHandler();
	std::shared_ptr<WIDGET> createOptionsPopoverForm();
	void displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent);
private:
	std::shared_ptr<W_LABEL> m_titleLabel;
	std::shared_ptr<WzGuideTopicsNavButton> m_backButton;
	std::shared_ptr<WzGuideTopicsNavButton> m_nextButton;
	std::shared_ptr<W_BUTTON> m_sidebarButton;
	std::shared_ptr<W_BUTTON> m_closeButton;
	std::shared_ptr<W_BUTTON> m_optionsButton;

	std::shared_ptr<W_SCREEN> optionsOverlayScreen;

	std::vector<std::string> m_stateHistory;
	size_t m_currentStateIndex = 0;
	OnPopStateFunction m_popStateHandler;
	OnSidebarToggleFunc m_onSidebarToggleHandler;
	OnCloseFunc m_onCloseHandler;

	int32_t m_topicDisplayWidth = 0;
};

class WzGuideForm;

class WzGuideSidebarTopicButton : public W_BUTTON
{
protected:
	WzGuideSidebarTopicButton() {}

	void initialize(const std::string& _topicIdentifier, const WzString& text, size_t _indentLevel)
	{
		topicIdentifier = _topicIdentifier;
		indentLevel = _indentLevel;
		FontID = (indentLevel > 0) ? font_regular : font_regular_bold;
		setString(text);
		int minButtonWidthForText = iV_GetTextWidth(pText, FontID);
		setGeometry(0, 0, minButtonWidthForText + (WzGuideSidebarTopicButton::leftPadding + WzGuideSidebarTopicButton::rightPadding), iV_GetTextLineSize(FontID) + 12);
	}
public:
	static std::shared_ptr<WzGuideSidebarTopicButton> make(const std::shared_ptr<WzGuideTopic>& topic, size_t indentLevel, optional<ActivityDBProtocol::GuideTopicPrefs> prefs)
	{
		ASSERT_OR_RETURN(nullptr, topic != nullptr, "Null topic?");
		class make_shared_enabler: public WzGuideSidebarTopicButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(topic->identifier, WzString::fromUtf8(topic->displayName.getLocalizedString(WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN)), indentLevel);
		if (prefs.has_value())
		{
			widget->setIsNew(prefs.value().lastReadVersion != topic->version);
		}
		return widget;
	}
	void setIsSelected(bool val)
	{
		isSelected = val;
	}
	void setIsNew(bool val)
	{
		isNew = val;
	}
	std::string getTip() override
	{
		return (isTruncated) ? pText.toUtf8() : "";
	}
	bool getIsSelected() const { return isSelected; }
	bool getIsNew() const { return isNew; }
	const std::string& getTopicIdentifier() const { return topicIdentifier; }
	size_t getIndentLevel() const { return indentLevel; }
	const WzString& getTopicDisplayName() const { return pText; }
protected:
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *) override;
private:
	std::string topicIdentifier;
	size_t indentLevel = 0;
	WzCachedText wzText;
	bool isSelected = false;
	bool isNew = true;
	bool isTruncated = false;
	static constexpr int leftPadding = 12;
	static constexpr int indentPadding = 12;
	static constexpr int rightPadding = 10;
};

class WzGuideSidebar : public WIDGET
{
protected:
	void initialize(const std::shared_ptr<WzGuideForm>& parentGuideForm);
public:
	static std::shared_ptr<WzGuideSidebar> make(const std::shared_ptr<WzGuideForm>& parentGuideForm, const std::shared_ptr<WzGuideTopicsRegistry>& registry);
	void changeSelectedItem(const std::string& topicIdentifier);
	optional<std::string> getItemTopicIdentifier(size_t idx);
	void updateList(const std::shared_ptr<WzGuideTopicsRegistry>& registry);
protected:
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
private:
	std::vector<std::shared_ptr<WzGuideSidebarTopicButton>> makeTopicWidgets(const std::vector<std::shared_ptr<WzWrappedGuideTopic>>& topics, const std::shared_ptr<WzGuideTopicsRegistry>& registry, size_t level = 0);
private:
	std::weak_ptr<WzGuideForm> m_parentGuideForm;
	std::vector<std::shared_ptr<WzGuideSidebarTopicButton>> m_topicButtons;
	std::shared_ptr<ScrollableListWidget> m_topicsList;
	std::unordered_map<std::string, size_t> m_topicIdentifierToListIdMap;
	std::string m_currentSelectedTopicIdentifier;
	const uint32_t listTopPadding = 10;
};

class WzGuideForm : public W_FORM
{
public:
	typedef WzGuideTopicsNavBar::OnCloseFunc OnCloseFunc;
	typedef std::function<void (const std::string& topicIdentifier)> OnTopicCloseFunc;
protected:
	void initialize(const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnCloseFunc& onCloseHandler);
	virtual void geometryChanged() override;
public:
	static std::shared_ptr<WzGuideForm> make(const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnCloseFunc& onCloseHandler);
	void toggleSidebar();
	void setSidebarVisible(bool visible);
	void showGuideTopic(const std::string& topicIdentifier, OnTopicCloseFunc onTopicClose = nullptr);
	void showFirstGuideTopicIfNeeded();
	void queueRefreshTopicsList();

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	virtual void display(int xOffset, int yOffset) override;
	virtual void run(W_CONTEXT *psContext) override;
private:
	bool displayGuideTopic(const std::string& topicIdentifier, OnTopicCloseFunc onTopicClose = nullptr);
	void resizeToIdeal();
	void updateSidebarIfNeeded();
private:
	std::shared_ptr<WzGuideTopicsNavBar> m_navBar;
	std::shared_ptr<WzGuideSidebar> m_sideBar;
	std::shared_ptr<WzGuideTopicDisplayWidget> m_guideTopic;
	bool m_sidebarVisible = false;
	bool m_queuedTopicsListRefresh = false;

	std::shared_ptr<WzGuideTopicsRegistry> m_registry;
	OnCloseFunc m_closeHandler;
	const int32_t topicsDisplayWidth = 400;
	const int32_t sidebarDisplayWidth = 220;
};

// MARK: - Linked Topic button

class WzGuideLinkedTopicButton: public W_BUTTON
{
protected:
	WzGuideLinkedTopicButton():
		font(font_regular),
		cachedText("", font)
	{ }

	void initialize(const std::string& localizedDisplayName)
	{
		displayName = WzString::fromUtf8(localizedDisplayName);
		cachedText = WzCachedText(displayName, font);
		ptsAboveBase = cachedText->aboveBase();
		setGeometry(x(), y(), idealWidth(), idealHeight());
	}

public:
	static std::shared_ptr<WzGuideLinkedTopicButton> make(const std::string& localizedDisplayName)
	{
		class make_shared_enabler: public WzGuideLinkedTopicButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(localizedDisplayName);
		return widget;
	}

	void display(int xOffset, int yOffset) override
	{
		auto left = xOffset + x();
		auto top = yOffset + y();
		auto marginLeft = left + leftMargin;
		auto textX = marginLeft + horizontalPadding;
		auto textY = top - cachedText->aboveBase();

		bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

		PIELIGHT textColor = WZCOL_FORM_TEXT;

		if (isHighlight || isDown)
		{
			PIELIGHT backColor = (isDown) ? pal_RGBA(50, 50, 50, 65) : pal_RGBA(255, 255, 255, 65);
			pie_UniTransBoxFill(left, top, left + width() - 1, top + height() - 1, backColor);

			PIELIGHT borderColor = (isHighlight) ? pal_RGBA(255, 255, 255, 180) : pal_RGBA(255, 255, 255, 75);
			iV_Box(left, top, left + width(), top + height(), borderColor);

			textColor = WZCOL_TEXT_BRIGHT;
		}

		cachedText->renderOutlined(textX, textY, textColor, {0, 0, 0, 128});
	}

	void run(W_CONTEXT *) override
	{
		cachedText.tick();
	}

	int aboveBase()
	{
		return ptsAboveBase;
	}

	virtual int32_t idealWidth() override
	{
		return cachedText->width() + leftMargin + 2 * horizontalPadding;
	}

	virtual int32_t idealHeight() override
	{
		return cachedText->lineSize();
	}

private:
	iV_fonts font;
	WzString displayName;
	WzCachedText cachedText;
	int32_t horizontalPadding = 3;
	int32_t leftMargin = 0;
	int32_t ptsAboveBase = 0;
};

// MARK: - Reticule Button image view

class WzGuideRetButtonView : public WIDGET
{
public:
	static std::shared_ptr<WzGuideRetButtonView> make(int retButId);
protected:
	void display(int xOffset, int yOffset) override;
public:
	virtual int32_t idealWidth() override { return imageWidth; }
	virtual int32_t idealHeight() override { return imageHeight; }
private:
	WzString retButtonDisplayFilename;
	int32_t imageWidth = 0;
	int32_t imageHeight = 0;
};

std::shared_ptr<WzGuideRetButtonView> WzGuideRetButtonView::make(int retButId)
{
	auto retButtonDisplayFilename = getReticuleButtonDisplayFilename(retButId);
	if (!retButtonDisplayFilename.has_value() || retButtonDisplayFilename.value().empty())
	{
		return nullptr;
	}
	class make_shared_enabler: public WzGuideRetButtonView {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->retButtonDisplayFilename = WzString::fromUtf8(retButtonDisplayFilename.value());

	AtlasImageDef *image = iV_GetImage(widget->retButtonDisplayFilename);
	if (image)
	{
		// set the button width/height based on the "normal" image dimensions
		widget->imageWidth = image->Width;
		widget->imageHeight = image->Height;
		widget->setGeometry(widget->x(), widget->y(), widget->imageWidth, widget->imageHeight);
	}

	return widget;
}

void WzGuideRetButtonView::display(int xOffset, int yOffset)
{
	if (retButtonDisplayFilename.isEmpty())
	{
		return;
	}
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	float w = width();
	float h = height();
	iV_DrawImage2(retButtonDisplayFilename, x0, y0, w, h);
}

// MARK: - Stat image view

class WzGuideStatPIEView : public DynamicIntFancyButton
{
protected:
	WzGuideStatPIEView()
	{
		setTransparentToClicks(true);
	}
public:
	static std::shared_ptr<WzGuideStatPIEView> make(const WzStatVisual& statVisual);
	static std::shared_ptr<WzGuideStatPIEView> makeFromDroidTemplateJSON(const nlohmann::json& droidTemplateJSON);
protected:
	void display(int xOffset, int yOffset) override;
	bool isHighlighted() const override
	{
		return false;
	}
public:
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	BASE_STATS *Stat = nullptr;
	DROID_TEMPLATE design;
};

size_t getComponentsNumByType(COMPONENT_TYPE type)
{
	switch (type)
	{
		case COMP_BODY:
			return asBodyStats.size();
		case COMP_BRAIN:
			return asBrainStats.size();
		case COMP_PROPULSION:
			return asPropulsionStats.size();
		case COMP_REPAIRUNIT:
			return asRepairStats.size();
		case COMP_ECM:
			return asECMStats.size();
		case COMP_SENSOR:
			return asSensorStats.size();
		case COMP_CONSTRUCT:
			return asConstructStats.size();
		case COMP_WEAPON:
			return asWeaponStats.size();
		case COMP_NUMCOMPONENTS:
			return 0;
	}
	return 0;
}

static optional<ItemAvailability> getStatPlayerAvailability(int player, BASE_STATS *Stat)
{
	ASSERT_OR_RETURN(nullopt, player < MAX_PLAYERS, "Invalid player: %d", player);
	ASSERT_OR_RETURN(nullopt, Stat != nullptr, "Stat is null");

	if (StatIsStructure(Stat))
	{
		ASSERT_OR_RETURN(nullopt, Stat->index < numStructureStats, "Invalid struct stat index: %zu (exceeds numStructureStats: %" PRIu32 ")", Stat->index, numStructureStats);
		ASSERT_OR_RETURN(nullopt, Stat == &asStructureStats[Stat->index], "Sanity check failed");
		return (ItemAvailability)apStructTypeLists[player][Stat->index];
	}
	else if (StatIsTemplate(Stat))
	{
		// not currently supported
		return nullopt;
	}
	else if (StatIsFeature(Stat))
	{
		// treat features as always available
		return (ItemAvailability)AVAILABLE;
	}
	else
	{
		COMPONENT_TYPE partIndex = StatIsComponent(Stat); // Old comment: "This fails for viper body." ... but does it?
		if (partIndex != COMP_NUMCOMPONENTS)
		{
			size_t numOfComponentsOfType = getComponentsNumByType(partIndex);
			ASSERT_OR_RETURN(nullopt, Stat->index < numOfComponentsOfType, "Invalid Stat index: %zu (exceeds component type num: %zu)", Stat->index, numOfComponentsOfType);
			return (ItemAvailability)apCompLists[player][partIndex][Stat->index];
		}
		else if (StatIsResearch(Stat))
		{
			ASSERT_OR_RETURN(nullopt, Stat->index < asResearch.size(), "Invalid Stat index: %zu (exceeds research num: %zu)", Stat->index, asResearch.size());
			PLAYER_RESEARCH *psRes = &asPlayerResList[player][Stat->index];
			if ((psRes->ResearchStatus & RESBITS_ALL) != 0)
			{
				return (ItemAvailability)AVAILABLE;
			}
			else if (IsResearchPossible(psRes))
			{
				return (ItemAvailability)FOUND;
			}
			else
			{
				return (ItemAvailability)UNAVAILABLE;
			}
		}
	}

	return nullopt;
}

std::shared_ptr<WzGuideStatPIEView> WzGuideStatPIEView::make(const WzStatVisual& statVisual)
{
	BASE_STATS *Stat = getBaseStatsFromName(WzString::fromUtf8(statVisual.statName));
	if (Stat == nullptr)
	{
		// could be a feature stat id (which getBaseStatsFromName doesn't currently handle)
		int featureStatId = getFeatureStatFromName(WzString::fromUtf8(statVisual.statName));
		if (featureStatId >= 0 && featureStatId < asFeatureStats.size())
		{
			Stat = &asFeatureStats[featureStatId];
		}
		else
		{
			// still didn't find this stat
			return nullptr;
		}
	}

	if (statVisual.onlyIfKnown == WzStatVisual::OnlyIfKnown::True || (statVisual.onlyIfKnown == WzStatVisual::OnlyIfKnown::CampaignMode && ActivityManager::instance().getCurrentGameMode() == ActivitySink::GameMode::CAMPAIGN))
	{
		auto availability = getStatPlayerAvailability(selectedPlayer, Stat);
		if (availability.has_value())
		{
			switch (availability.value())
			{
				case UNAVAILABLE:
					// skip displaying this item
					return nullptr;
				default:
					break;
			}
		}
	}

	class make_shared_enabler: public WzGuideStatPIEView {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->buttonType = TOPBUTTON;
	widget->Stat = Stat;
	return widget;
}

std::shared_ptr<WzGuideStatPIEView> WzGuideStatPIEView::makeFromDroidTemplateJSON(const nlohmann::json& droidTemplateJSON)
{
	class make_shared_enabler: public WzGuideStatPIEView {};
	auto widget = std::make_shared<make_shared_enabler>();
	if (!loadTemplateCommon(droidTemplateJSON, widget->design))
	{
		debug(LOG_ERROR, "Unable to parse template");
		return nullptr;
	}
	widget->buttonType = TOPBUTTON;
	widget->Stat = &widget->design; // point to the DROID_TEMPLATE that the widget owns
	return widget;
}

int32_t WzGuideStatPIEView::idealWidth()
{
	return OBJ_BUTWIDTH; // because DynamicIntFancyButton largely assumes this, and doesn't currently support resizing
}

int32_t WzGuideStatPIEView::idealHeight()
{
	return OBJ_BUTHEIGHT; // because DynamicIntFancyButton largely assumes this, and doesn't currently support resizing
}

void WzGuideStatPIEView::display(int xOffset, int yOffset)
{
	ImdObject object;
	AtlasImage image;

	if (Stat)
	{
		if (StatIsStructure(Stat))
		{
			object = ImdObject::StructureStat(Stat);
		}
		else if (StatIsTemplate(Stat))
		{
			object = ImdObject::DroidTemplate(Stat);
		}
		else if (StatIsFeature(Stat))
		{
			object = ImdObject::Feature(Stat);
		}
		else
		{
			SDWORD compID = StatIsComponent(Stat); // Old comment: "This fails for viper body." ... but does it?
			if (compID != COMP_NUMCOMPONENTS)
			{
				object = ImdObject::Component(Stat);
			}
			else if (StatIsResearch(Stat))
			{
				object = getResearchObjectImage(((RESEARCH *)Stat));
			}
		}
	}
	else
	{
		//BLANK button for now - AB 9/1/98
		object = ImdObject::Component(nullptr);
	}

	displayIMD(image, object, xOffset, yOffset);
}

// MARK: - Atlas IMAGEFILE image view

class W_ATLASIMAGE : public WIDGET
{
protected:
	W_ATLASIMAGE() {}
public:
	static std::shared_ptr<W_ATLASIMAGE> make(IMAGEFILE *ImageFile, std::vector<UWORD> ImageIDs)
	{
		class make_shared_enabler: public W_ATLASIMAGE {};
		auto widget = std::make_shared<make_shared_enabler>();
		ASSERT_OR_RETURN(nullptr, ImageFile != nullptr, "ImageFile is null");
		widget->ImageFile = ImageFile;
		widget->ImageIDs = ImageIDs;

		int32_t maxImageHeight = 0;
		int32_t totalIdealWidth = 0;
		for (auto ImageID : ImageIDs)
		{
			int32_t w = iV_GetImageWidth(ImageFile, ImageID);
			int32_t h = iV_GetImageHeight(ImageFile, ImageID);
			widget->imageWidths.push_back(w);
			widget->imageHeights.push_back(h);
			maxImageHeight = std::max(maxImageHeight, h);
			totalIdealWidth += w + 2;
		}
		widget->maxImageHeight = maxImageHeight;
		widget->totalIdealWidth = totalIdealWidth;

		return widget;
	}
protected:
	void display(int xOffset, int yOffset) override;
public:
	virtual int32_t idealWidth() override { return totalIdealWidth; }
	virtual int32_t idealHeight() override { return maxImageHeight; }
private:
	IMAGEFILE *ImageFile = nullptr;
	std::vector<UWORD> ImageIDs;
	std::vector<int32_t> imageWidths;
	std::vector<int32_t> imageHeights;
	int32_t maxImageHeight = 0;
	int32_t totalIdealWidth = 0;
};

void W_ATLASIMAGE::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int h = height();

	int currImageX0 = x0;
	for (size_t idx = 0; idx < ImageIDs.size(); ++idx)
	{
		auto imageWidth = imageWidths[idx];
		auto imageHeight = imageHeights[idx];
		auto imageID = ImageIDs[idx];
		int imageY0 = y0 + (h - imageHeight) / 2;
		iV_DrawImageTint(ImageFile, imageID, currImageX0, imageY0, pal_RGBA(255, 255, 255, 255));
		currImageX0 += imageWidth + 2;
	}
}

// MARK: - Image view

class W_IMAGE : public WIDGET
{
protected:
	W_IMAGE() {}
public:
	~W_IMAGE()
	{
		if (pOwnedBackgroundTexture) { delete pOwnedBackgroundTexture; pOwnedBackgroundTexture = nullptr; }
	}
public:
	static std::shared_ptr<W_IMAGE> make(const std::string& filename)
	{
		class make_shared_enabler: public W_IMAGE {};
		auto widget = std::make_shared<make_shared_enabler>();
		auto pOwnedBackgroundTexture = gfx_api::context::get().loadTextureFromFile(filename.c_str(), gfx_api::texture_type::user_interface, -1, -1, true);
		ASSERT_OR_RETURN(nullptr, pOwnedBackgroundTexture != nullptr, "Failed to load texture from file: %s", filename.c_str());
		widget->pOwnedBackgroundTexture = pOwnedBackgroundTexture;
		auto textureDimensions = pOwnedBackgroundTexture->get_dimensions();
		ASSERT_OR_RETURN(nullptr, textureDimensions.width <= 4096 && textureDimensions.height <= 4096, "Unexpectedly large texture dimensions?: %zu x %zu", textureDimensions.width, textureDimensions.height);
		widget->textureWidth = static_cast<int32_t>(textureDimensions.width);
		widget->textureHeight = static_cast<int32_t>(textureDimensions.height);
		return widget;
	}
protected:
	void display(int xOffset, int yOffset) override;
public:
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	gfx_api::texture* pOwnedBackgroundTexture = nullptr;
	int32_t textureWidth = 0;
	int32_t textureHeight = 0;
};

int32_t W_IMAGE::idealWidth()
{
	return textureWidth;
}

int32_t W_IMAGE::idealHeight()
{
	return textureHeight;
}

void W_IMAGE::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	// Draw the background
	if (pOwnedBackgroundTexture)
	{
		iV_DrawImageAnisotropic(*pOwnedBackgroundTexture, Vector2i(x0, y0), Vector2f(0.f,0.f), Vector2f(width(), height()), 0.f, pal_RGBA(255, 255, 255, 255));
	}
}

// MARK: - Linked Topics widget

class WzGuideLinkedTopicsWidget : public WIDGET
{
public:
	typedef std::function<void (const std::string& topicIdentifier)> OnClickLinkedTopicFunc;
protected:
	WzGuideLinkedTopicsWidget() { }
	void initialize(const std::vector<std::string>& linkedTopicIdentifiers, const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnClickLinkedTopicFunc& onClickLinkedTopicHandler);
public:
	static std::shared_ptr<WzGuideLinkedTopicsWidget> make(const std::vector<std::string>& linkedTopicIdentifiers, const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnClickLinkedTopicFunc& onClickLinkedTopicHandler);
	virtual int32_t idealHeight() override;
	int32_t rowHeight() const;
protected:
	virtual void geometryChanged() override;
private:
	std::shared_ptr<W_LABEL> m_seeAlsoLabel;
	std::shared_ptr<Paragraph> m_linkedTopics;
	std::shared_ptr<ScrollableListWidget> m_scrollableLinkedTopicsContainer;
	const int lineSpacing = 2;
	OnClickLinkedTopicFunc m_onClickLinkedTopicHandler;
};

int32_t WzGuideLinkedTopicsWidget::idealHeight()
{
	if (!m_scrollableLinkedTopicsContainer || !m_seeAlsoLabel) { return 0; }
	return std::max<int32_t>(m_scrollableLinkedTopicsContainer->idealHeight(), m_seeAlsoLabel->idealHeight());
}

int32_t WzGuideLinkedTopicsWidget::rowHeight() const
{
	return iV_GetTextLineSize(font_regular) + lineSpacing;
}

std::shared_ptr<WzGuideLinkedTopicsWidget> WzGuideLinkedTopicsWidget::make(const std::vector<std::string>& linkedTopicIdentifiers, const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnClickLinkedTopicFunc& onClickLinkedTopicHandler)
{
	class make_shared_enabler: public WzGuideLinkedTopicsWidget {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(linkedTopicIdentifiers, registry, onClickLinkedTopicHandler);
	return widget;
}

void WzGuideLinkedTopicsWidget::initialize(const std::vector<std::string>& linkedTopicIdentifiers, const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnClickLinkedTopicFunc& onClickLinkedTopicHandler)
{
	if (linkedTopicIdentifiers.empty())
	{
		return;
	}

	m_onClickLinkedTopicHandler = onClickLinkedTopicHandler;

	m_seeAlsoLabel = std::make_shared<W_LABEL>();
	attach(m_seeAlsoLabel);
	m_seeAlsoLabel->setFont(font_regular, WZCOL_FORM_TEXT);
	m_seeAlsoLabel->setString(_("See Also:"));
	m_seeAlsoLabel->setCanTruncate(true);
	m_seeAlsoLabel->setGeometry(0, 0, m_seeAlsoLabel->getMaxLineWidth(), iV_GetTextLineSize(font_regular));

	m_linkedTopics = std::make_shared<Paragraph>();
	m_linkedTopics->setFont(font_regular);
	m_linkedTopics->setFontColour(WZCOL_TEXT_BRIGHT);
	m_linkedTopics->setLineSpacing(lineSpacing);
	m_linkedTopics->setGeometry(0, 0, 400, 40);

	std::weak_ptr<WzGuideLinkedTopicsWidget> psWeakLinkedTopicsWidg = std::dynamic_pointer_cast<WzGuideLinkedTopicsWidget>(shared_from_this());

	for (const auto& topicID : linkedTopicIdentifiers)
	{
		auto topicDisplayName = registry->getDisplayNameForTopic(topicID);
		if (!topicDisplayName.has_value())
		{
			continue;
		}
		auto linkedTopicWidg = WzGuideLinkedTopicButton::make(topicDisplayName.value());
		std::string topicIdentifierCopy = topicID;
		linkedTopicWidg->addOnClickHandler([topicIdentifierCopy, psWeakLinkedTopicsWidg](W_BUTTON&) {
			auto strongParent = psWeakLinkedTopicsWidg.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
			if (strongParent->m_onClickLinkedTopicHandler)
			{
				strongParent->m_onClickLinkedTopicHandler(topicIdentifierCopy);
			}
		});
		auto marginWrappedWidget = Margin(0, 0, 0, 4).wrap(linkedTopicWidg);
		marginWrappedWidget->setGeometry(0, 0, marginWrappedWidget->idealWidth(), marginWrappedWidget->idealHeight());
		m_linkedTopics->addWidget(marginWrappedWidget, linkedTopicWidg->aboveBase());
	}

	m_scrollableLinkedTopicsContainer = ScrollableListWidget::make();
	m_scrollableLinkedTopicsContainer->setPadding({0, 0, 0, 0});
	m_scrollableLinkedTopicsContainer->addItem(m_linkedTopics);
	int topicsContainerX0 = m_seeAlsoLabel->x() + m_seeAlsoLabel->width() + 10;
	m_scrollableLinkedTopicsContainer->setGeometry(topicsContainerX0, 0, 400, 45);
	attach(m_scrollableLinkedTopicsContainer);
}

void WzGuideLinkedTopicsWidget::geometryChanged()
{
	int w = width();
	int h = height();
	if (w <= 1 && h <= 1)
	{
		return;
	}

	if (!m_seeAlsoLabel)
	{
		return;
	}

	int topicsContainerX0 = m_seeAlsoLabel->x() + m_seeAlsoLabel->width() + 10;
	int topicsContainerWidth = std::max(w - topicsContainerX0, 0);
	if (topicsContainerWidth <= 1)
	{
		return;
	}
	m_scrollableLinkedTopicsContainer->setGeometry(topicsContainerX0, 0, topicsContainerWidth, h);
}

// MARK: - WzGuideTopicDisplayWidget

class WzGuideTopicDisplayWidget : public WIDGET
{
protected:
	WzGuideTopicDisplayWidget(): WIDGET() {}
	void initialize(const WzWrappedGuideTopic& topic, const std::shared_ptr<WzGuideTopicsRegistry>& registry);
protected:
	virtual void geometryChanged() override;
public:
	static std::shared_ptr<WzGuideTopicDisplayWidget> make(const WzWrappedGuideTopic& topic, const std::shared_ptr<WzGuideTopicsRegistry>& registry);
	const std::string& getDisplayedTopicID() const;
private:
	std::shared_ptr<W_LABEL> constructParentNavPathWidget(const WzWrappedGuideTopic& topic);
private:
	std::string m_topicID;
	std::shared_ptr<W_LABEL> m_parentNavPath;
	std::shared_ptr<W_LABEL> m_displayName;
	std::vector<std::shared_ptr<WIDGET>> m_visuals;
	std::shared_ptr<Paragraph> m_contents;
	std::shared_ptr<ScrollableListWidget> m_scrollableContentsContainer;
	std::shared_ptr<WzGuideLinkedTopicsWidget> m_linkedTopicsWidget;
	int maxLinkedTopicsHeight = 28;
	const int internalPaddingY = 15;
	const int internalPaddingX = 15;
	const int internalMarginTop = 10;
	const int maxVisualHeight = OBJ_BUTHEIGHT; // for now, limited to the max height of a stat view button image
	const int visualBetweenPaddingY = 5;
};

std::shared_ptr<WzGuideTopicDisplayWidget> WzGuideTopicDisplayWidget::make(const WzWrappedGuideTopic& topic, const std::shared_ptr<WzGuideTopicsRegistry>& registry)
{
	const auto& psTopic = topic.topic;
	ASSERT_OR_RETURN(nullptr, psTopic != nullptr, "Null topic?");
	class make_shared_enabler: public WzGuideTopicDisplayWidget {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(topic, registry);
	return widget;
}

static int32_t widgScaledWidthForHeight(const std::shared_ptr<WIDGET>& widg, int32_t height)
{
	// returns the width for a given height, maintaining the original widget's aspect ratio (according to idealWidth/idealHeight)
	auto idealHeight = widg->idealHeight();
	auto idealWidth = widg->idealWidth();
	if (height == widg->idealHeight()) { return idealWidth; }
	if (idealHeight <= 0) { return 0; }
	double heightScaling = (double)height / (double)idealHeight;
	return static_cast<int32_t>(idealWidth * heightScaling);
}

void WzGuideTopicDisplayWidget::geometryChanged()
{
	int remainingHeight = height();
	int w = width();
	int usableWidth = std::max(w - (internalPaddingX * 2), 0);
	int maxUsableX1 = std::max(w - internalPaddingX, 0);

	if (usableWidth <= 1)
	{
		return;
	}

	int nextLineY0 = internalMarginTop + internalPaddingY;
	remainingHeight -= nextLineY0;

	if (m_parentNavPath)
	{
		// display nav path bar at top
		m_parentNavPath->setGeometry(internalPaddingX, nextLineY0, std::min<int>(m_parentNavPath->getMaxLineWidth(), usableWidth), m_parentNavPath->height());
		remainingHeight -= (m_parentNavPath->height() + internalPaddingY);
		nextLineY0 += m_parentNavPath->height() + internalPaddingY;
	}

	// display name is first
	m_displayName->setGeometry(internalPaddingX, nextLineY0, std::min<int>(m_displayName->getMaxLineWidth(), usableWidth), m_displayName->height());
	remainingHeight -= (m_displayName->height() + internalPaddingY);
	nextLineY0 += m_displayName->height() + internalPaddingY;

	// followed by the visual(s) (if present)
	if (!m_visuals.empty())
	{
		int visualX0 = internalPaddingX;
		int maxIdealVisualHeight = 0;
		for (auto& visual : m_visuals)
		{
			maxIdealVisualHeight = std::max<int>(maxIdealVisualHeight, visual->idealHeight());
		}
		maxIdealVisualHeight = std::min<int32_t>(maxVisualHeight, maxIdealVisualHeight);
		for (auto& visual : m_visuals)
		{
			int32_t visualIdealHeight = visual->idealHeight();
			int32_t visualHeight = std::min<int32_t>(maxVisualHeight, visualIdealHeight);
			int32_t visualWidth = widgScaledWidthForHeight(visual, visualHeight);
			int visualY0 = nextLineY0 + (maxIdealVisualHeight - visualHeight) / 2;
			int visualY1 = visualX0 + visualWidth;
			if (visualY1 > maxUsableX1)
			{
				visual->hide();
				continue;
			}
			visual->show();
			visual->setGeometry(visualX0, visualY0, visualWidth, visualHeight);
			visualX0 += visualWidth + visualBetweenPaddingY;
		}

		remainingHeight -= (maxIdealVisualHeight + internalPaddingY);
		nextLineY0 += maxIdealVisualHeight + internalPaddingY;
	}

	// the linked topics container takes up space at the bottom
	if (m_linkedTopicsWidget)
	{
		if (usableWidth > 1 && remainingHeight > 1)
		{
			m_linkedTopicsWidget->setGeometry(internalPaddingX, nextLineY0, usableWidth, maxLinkedTopicsHeight); // set once to set width
			// set again to set height based on idealHeight for that width (if smaller)
			int linkedTopicsHeight = std::min<int>(m_linkedTopicsWidget->idealHeight(), maxLinkedTopicsHeight);
			m_linkedTopicsWidget->setGeometry(internalPaddingX, height() - (linkedTopicsHeight + internalPaddingY), usableWidth, linkedTopicsHeight);
			remainingHeight -= (m_linkedTopicsWidget->height() + internalPaddingY);
		}
	}

	// the contents take up the rest of the space
	if (m_scrollableContentsContainer)
	{
		remainingHeight -= internalPaddingY;
		if (usableWidth > 1 && remainingHeight > 1)
		{
			m_scrollableContentsContainer->setGeometry(internalPaddingX, nextLineY0, usableWidth, remainingHeight);
		}
	}
}

std::shared_ptr<W_LABEL> WzGuideTopicDisplayWidget::constructParentNavPathWidget(const WzWrappedGuideTopic& topic)
{
	auto psParentTopic = topic.parent.lock();
	if (!psParentTopic)
	{
		return nullptr;
	}

	std::vector<std::string> pathComponents;
	while (psParentTopic != nullptr)
	{
		pathComponents.push_back(psParentTopic->topic->displayName.getLocalizedString(WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN));
		psParentTopic = psParentTopic->parent.lock();
	}

	if (pathComponents.size() <= 1) // don't display for first-level entries
	{
		return nullptr;
	}

	std::string pathStr;
	for (auto it = pathComponents.rbegin(); it != pathComponents.rend(); it++)
	{
		if (!pathStr.empty())
		{
			pathStr += " » ";
		}
		pathStr.append(*it);
	}

	auto result = std::make_shared<W_LABEL>();
	result->setFont(font_bar, WZCOL_TEXT_MEDIUM);
	result->setString(WzString::fromUtf8(pathStr));
	result->setCanTruncate(true);
	result->setGeometry(0, 0, result->getMaxLineWidth(), iV_GetTextLineSize(font_bar));

	return result;
}

void WzGuideTopicDisplayWidget::initialize(const WzWrappedGuideTopic& psWrappedTopic, const std::shared_ptr<WzGuideTopicsRegistry> &registry)
{
	const auto& topic = *(psWrappedTopic.topic);

	m_topicID = topic.identifier;

	// determine if a the parent navpath needs to be shown
	m_parentNavPath = constructParentNavPathWidget(psWrappedTopic);
	if (m_parentNavPath)
	{
		attach(m_parentNavPath);
	}

	m_displayName = std::make_shared<W_LABEL>();
	attach(m_displayName);
	m_displayName->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
	m_displayName->setString(WzString::fromUtf8(topic.displayName.getLocalizedString(WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN)));
	m_displayName->setCanTruncate(true);
	m_displayName->setGeometry(0, 0, m_displayName->getMaxLineWidth(), iV_GetTextLineSize(font_medium));

	// the visuals (reticuleButtonId, statId, imagePath, etc)
	for (const auto& visual : topic.visuals)
	{
		if (visual.reticuleButtonId.has_value())
		{
			auto retButtonView = WzGuideRetButtonView::make(visual.reticuleButtonId.value());
			if (retButtonView)
			{
				attach(retButtonView, ChildZPos::Back);
				retButtonView->setGeometry(0, 0, retButtonView->idealWidth(), retButtonView->idealHeight());
				m_visuals.push_back(retButtonView);
			}
		}
		else if (visual.statVisual.has_value())
		{
			auto statPieView = WzGuideStatPIEView::make(visual.statVisual.value());
			if (statPieView)
			{
				attach(statPieView, ChildZPos::Back);
				int32_t imageHeight = std::min<int32_t>(maxVisualHeight, statPieView->idealHeight());
				statPieView->setGeometry(0, 0, statPieView->idealWidth(), imageHeight);
				m_visuals.push_back(statPieView);
			}
		}
		else if (visual.droidTemplateJSON.has_value())
		{
			auto statPieView = WzGuideStatPIEView::makeFromDroidTemplateJSON(visual.droidTemplateJSON.value());
			if (statPieView)
			{
				attach(statPieView, ChildZPos::Back);
				int32_t imageHeight = std::min<int32_t>(maxVisualHeight, statPieView->idealHeight());
				statPieView->setGeometry(0, 0, statPieView->idealWidth(), imageHeight);
				m_visuals.push_back(statPieView);
			}
		}
		else if (visual.secondaryUnitOrderButtons.has_value())
		{
			auto orderButtonImages = intOrderGetButtonImageIDs(visual.secondaryUnitOrderButtons.value());
			if (!orderButtonImages.empty())
			{
				auto orderButtonImagesWidg = W_ATLASIMAGE::make(IntImages, orderButtonImages);
				if (orderButtonImagesWidg)
				{
					attach(orderButtonImagesWidg, ChildZPos::Back);
					int32_t imageHeight = std::min<int32_t>(maxVisualHeight, orderButtonImagesWidg->idealHeight());
					orderButtonImagesWidg->setGeometry(0, 0, orderButtonImagesWidg->idealWidth(), imageHeight);
					m_visuals.push_back(orderButtonImagesWidg);
				}
			}
		}
		else if (visual.imagePath.has_value())
		{
			auto image = W_IMAGE::make(visual.imagePath.value());
			if (image)
			{
				attach(image, ChildZPos::Back);
				int32_t imageHeight = std::min<int32_t>(maxVisualHeight, image->idealHeight());
				image->setGeometry(0, 0, image->width(), imageHeight);
				m_visuals.push_back(image);
			}
		}
	}

	m_contents = std::make_shared<Paragraph>();
	m_contents->setFont(font_regular);
	m_contents->setFontColour(WZCOL_TEXT_BRIGHT);
	m_contents->setGeometry(0, 0, 400, 400);
	m_contents->setLineSpacing(2);

	auto addFormattedTextLine = [&](std::string& line)
	{
		optional<iV_fonts> oldFont;
		if (!line.empty())
		{
			// If line starts with "#", treat as heading or subheading (i.e. changing font size)
			if (line.front() == '#')
			{
				oldFont = m_contents->getTextStyle().font;
				iV_fonts newFont = font_medium_bold;
				if (line.size() > 1 && line[1] == '#')
				{
					newFont = font_regular_bold;
				}
				m_contents->setFont(newFont);
			}
		}
		else
		{
			line = " ";
		}
		m_contents->addText(WzString::fromUtf8(line));
		if (oldFont.has_value())
		{
			m_contents->setFont(oldFont.value());
		}
	};

	for (size_t idx = 0; idx < topic.contents.size(); ++idx )
	{
		if (idx > 0)
		{
			m_contents->addText("\n");
		}
		const auto& localizedStr = topic.contents[idx];
		auto pDisplayString = localizedStr.getLocalizedString(WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN);
		std::string displayString = (pDisplayString) ? std::string(pDisplayString) : std::string();
		addFormattedTextLine(displayString);
	}

	m_scrollableContentsContainer = ScrollableListWidget::make();
	m_scrollableContentsContainer->addItem(m_contents);
	m_scrollableContentsContainer->setPadding({0, 4, 0, 0});
	m_scrollableContentsContainer->setGeometry(0, 0, 400, 405);
	attach(m_scrollableContentsContainer);

	// display linked topics that have already been added to the guide topics registry
	std::vector<std::string> foundLinkedTopicIdentifiers;
	std::copy_if(topic.linkedTopicIdentifiers.begin(), topic.linkedTopicIdentifiers.end(), std::back_inserter(foundLinkedTopicIdentifiers),
				 [&registry](const std::string& topicID)
	{
		 return registry->getTopic(topicID) != nullptr;
	});
	if (!foundLinkedTopicIdentifiers.empty())
	{
		std::weak_ptr<WzGuideTopicDisplayWidget> psWeakGuideTopicWidg = std::dynamic_pointer_cast<WzGuideTopicDisplayWidget>(shared_from_this());
		m_linkedTopicsWidget = WzGuideLinkedTopicsWidget::make(foundLinkedTopicIdentifiers, registry, [psWeakGuideTopicWidg](const std::string& linkedTopicIdentifier) {
			std::string linkedTopicIdentifierCopy = linkedTopicIdentifier;
			widgScheduleTask([linkedTopicIdentifierCopy, psWeakGuideTopicWidg]() {
				auto strongGuideTopicWidg = psWeakGuideTopicWidg.lock();
				ASSERT_OR_RETURN(, strongGuideTopicWidg != nullptr, "No parent?");
				// The parent of the strongGuideTopicWidg should be the WzGuideForm
				auto strongGuideForm = std::dynamic_pointer_cast<WzGuideForm>(strongGuideTopicWidg->parent());
				ASSERT_OR_RETURN(, strongGuideForm != nullptr, "No guide form?");
				strongGuideForm->showGuideTopic(linkedTopicIdentifierCopy);
			});
		});
		maxLinkedTopicsHeight = (m_linkedTopicsWidget->rowHeight() * 2);
		m_linkedTopicsWidget->setGeometry(0, 0, 400, maxLinkedTopicsHeight);
		attach(m_linkedTopicsWidget);
	}
}

const std::string& WzGuideTopicDisplayWidget::getDisplayedTopicID() const
{
	return m_topicID;
}

// MARK: - WzGuideTopicsNavBar implementation

class WzGuideTopicsNavButton : public W_BUTTON
{
public:
	static std::shared_ptr<WzGuideTopicsNavButton> make(const WzString& buttonText, iV_fonts FontID = font_regular)
	{
		auto result = std::make_shared<WzGuideTopicsNavButton>();
		result->FontID = FontID;
		result->pText = buttonText;
		return result;
	}
	static std::shared_ptr<WzGuideTopicsNavButton> make(UWORD intfacImageID, int imageDimensions = 16)
	{
		if (!assertValidImage(IntImages, intfacImageID))
		{
			return nullptr;
		}
		auto result = std::make_shared<WzGuideTopicsNavButton>();
		result->intfacImageID = intfacImageID;
		result->imageDimensions = imageDimensions;
		return result;
	}
public:
	virtual int32_t idealWidth() override
	{
		if (intfacImageID.has_value())
		{
			return imageDimensions + (GuideNavBarButtonHorizontalPadding * 2);
		}
		return iV_GetTextWidth(pText, FontID) + (GuideNavBarButtonHorizontalPadding * 2);
	}

	// Display the text, shrunk to fit, with a faint border around it (that gets bright when highlighted)
	virtual void display(int xOffset, int yOffset) override
	{
		const std::shared_ptr<WIDGET> pParent = this->parent();
		ASSERT_OR_RETURN(, pParent != nullptr, "Keymap buttons should have a parent container!");

		int xPos = xOffset + x();
		int yPos = yOffset + y();
		int h = height();
		int w = width();

		bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		bool isDisabled = (state & WBUT_DISABLE) != 0;
		bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

		// Draw background (if needed)
		optional<PIELIGHT> backColor;
		if (!isDisabled)
		{
			if (isDown)
			{
				backColor = WZCOL_MENU_SCORE_BUILT;
			}
			else if (isHighlight)
			{
				backColor = pal_RGBA(255,255,255,50);
			}
			if (backColor.has_value())
			{
				pie_UniTransBoxFill(xPos, yPos, xPos + w, yPos + h, backColor.value());
			}
		}

		// Select label text and color
		PIELIGHT msgColour = (!isDown) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			msgColour = WZCOL_TEXT_BRIGHT;
			msgColour.byte.a = msgColour.byte.a / 2;
		}

		if (intfacImageID.has_value())
		{
			// Display the image centered
			int imgPosX0 = xPos + (w - imageDimensions) / 2;
			int imgPosY0 = yPos + (h - imageDimensions) / 2;
			PIELIGHT imgColor = msgColour;
			iV_DrawImageFileAnisotropicTint(IntImages, intfacImageID.value(), imgPosX0, imgPosY0, Vector2f(imageDimensions, imageDimensions), imgColor);
			return; // shortcut the rest
		}

		iV_fonts fontID = wzMessageText.getFontID();
		if (fontID == font_count || lastWidgetWidth != width() || wzMessageText.getText() != pText)
		{
			fontID = FontID;
		}
		wzMessageText.setText(pText, fontID);

		int availableButtonTextWidth = w - (GuideNavBarButtonHorizontalPadding * 2);
		if (wzMessageText.width() > availableButtonTextWidth)
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
				wzMessageText.setText(pText, fontID);
			} while (wzMessageText.width() > availableButtonTextWidth);
		}
		lastWidgetWidth = width();

		int textX0 = std::max(xPos + GuideNavBarButtonHorizontalPadding, xPos + ((w - wzMessageText.width()) / 2));
		int textY0 = static_cast<int>(yPos + (h - wzMessageText.lineSize()) / 2 - float(wzMessageText.aboveBase()));

		int maxTextDisplayableWidth = w - (GuideNavBarButtonHorizontalPadding * 2);
		isTruncated = maxTextDisplayableWidth < wzMessageText.width();
		if (isTruncated)
		{
			maxTextDisplayableWidth -= (iV_GetEllipsisWidth(wzMessageText.getFontID()) + 2);
		}
		wzMessageText.render(textX0, textY0, msgColour, 0.0f, maxTextDisplayableWidth);

		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(wzMessageText.getFontID(), Vector2f(textX0 + maxTextDisplayableWidth + 2, textY0), msgColour);
		}
	}

	std::string getTip() override
	{
		if (!isTruncated) {
			return pTip;
		}

		return pText.toUtf8();
	}
private:
	WzText wzMessageText;
	optional<UWORD> intfacImageID = nullopt;
	int imageDimensions = 16;
	int lastWidgetWidth = 0;
	bool isTruncated = false;
};

WzGuideTopicsNavBar::~WzGuideTopicsNavBar()
{
	if (optionsOverlayScreen)
	{
		widgRemoveOverlayScreen(optionsOverlayScreen);
		optionsOverlayScreen.reset();
	}
}

int32_t WzGuideTopicsNavBar::idealHeight()
{
	return 30; // TODO: ??
}

std::shared_ptr<WzGuideTopicsNavBar> WzGuideTopicsNavBar::make(int32_t topicDisplayWidth, const OnPopStateFunction& popStateHandler, const OnSidebarToggleFunc& onSidebarToggleHandler, const OnCloseFunc& onCloseHandler)
{
	class make_shared_enabler: public WzGuideTopicsNavBar {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(topicDisplayWidth, popStateHandler, onSidebarToggleHandler, onCloseHandler);
	return widget;
}

void WzGuideTopicsNavBar::initialize(int32_t topicDisplayWidth, const OnPopStateFunction& popStateHandler, const OnSidebarToggleFunc& onSidebarToggleHandler, const OnCloseFunc& onCloseHandler)
{
	m_topicDisplayWidth = topicDisplayWidth;
	m_popStateHandler = popStateHandler;
	m_onSidebarToggleHandler = onSidebarToggleHandler;
	m_onCloseHandler = onCloseHandler;

	// Title
	m_titleLabel = std::make_shared<W_LABEL>();
	m_titleLabel->setFont(font_regular);
	m_titleLabel->setString(_("Game Guide"));
	m_titleLabel->setTransparentToMouse(true);
	attach(m_titleLabel);

	std::weak_ptr<WzGuideTopicsNavBar> psWeakNavBar(std::dynamic_pointer_cast<WzGuideTopicsNavBar>(shared_from_this()));

	// Close button
	m_closeButton = WzGuideTopicsNavButton::make(WzString::fromUtf8("\u2715"), font_regular); // ✕
	m_closeButton->setTip(_("Close"));
	attach(m_closeButton);
	m_closeButton->addOnClickHandler([psWeakNavBar](W_BUTTON&) {
		auto psStrongNavbar = psWeakNavBar.lock();
		ASSERT_OR_RETURN(, psStrongNavbar != nullptr, "No parent?");
		psStrongNavbar->triggerCloseHandler();
	});

	// Options button // "⚙"
	m_optionsButton = WzGuideTopicsNavButton::make(WzString::fromUtf8("\u2699"), font_regular);
	m_optionsButton->setTip(_("Options"));
	attach(m_optionsButton);
	m_optionsButton->addOnClickHandler([psWeakNavBar](W_BUTTON& button) {
		auto psStrongNavbar = psWeakNavBar.lock();
		ASSERT_OR_RETURN(, psStrongNavbar != nullptr, "No parent?");
		// Display a "pop-over" options menu
		psStrongNavbar->displayOptionsOverlay(psStrongNavbar);
		psStrongNavbar->m_optionsButton->setState(WBUT_CLICKLOCK);
	});

	// Sidebar button
	if (m_onSidebarToggleHandler)
	{
		m_sidebarButton = WzGuideTopicsNavButton::make(IMAGE_INTFAC_SIDEBAR_LIST, 12);
		if (!m_sidebarButton)
		{
			// failed to get the image - use an "s"
			m_sidebarButton = WzGuideTopicsNavButton::make("s", font_regular);
		}
		attach(m_sidebarButton);
		m_sidebarButton->addOnClickHandler([psWeakNavBar](W_BUTTON&) {
			auto psStrongNavbar = psWeakNavBar.lock();
			ASSERT_OR_RETURN(, psStrongNavbar != nullptr, "No parent?");
			psStrongNavbar->triggerSidebarToggleHandler();
		});
		m_sidebarButton->setTip(_("Toggle Sidebar"));
	}

	// Back / Forward buttons
	m_backButton = WzGuideTopicsNavButton::make("‹", font_medium);
	attach(m_backButton);
	m_backButton->setState(WBUT_DISABLE);
	m_backButton->addOnClickHandler([psWeakNavBar](W_BUTTON&) {
		auto psStrongNavbar = psWeakNavBar.lock();
		ASSERT_OR_RETURN(, psStrongNavbar != nullptr, "No parent?");
		psStrongNavbar->navBack();
	});

	m_nextButton = WzGuideTopicsNavButton::make("›", font_medium);
	attach(m_nextButton);
	m_nextButton->setState(WBUT_DISABLE);
	m_nextButton->addOnClickHandler([psWeakNavBar](W_BUTTON&) {
		auto psStrongNavbar = psWeakNavBar.lock();
		ASSERT_OR_RETURN(, psStrongNavbar != nullptr, "No parent?");
		psStrongNavbar->navForward();
	});

	updateLayout();
}

void WzGuideTopicsNavBar::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int h = height();
	int w = width();
	iV_Line(x0, y0 + h, x0 + w, y0 + h, pal_RGBA(255,255,255,255));
}

void WzGuideTopicsNavBar::geometryChanged()
{
	updateLayout();
}

void WzGuideTopicsNavBar::updateLayout()
{
	int x0 = x();
	int y0 = y();
	int w = width();
	int h = height();

	// The "m_topicDisplayWidth" area is a right-aligned area used for:
	// - the title (centered within it)
	// - the close button (right side of it)
	// - the action buttons (back / forward - left side of it*)

	// The sidebar button is always aligned with the overall left-side of the navbar.
	// If this is within the m_topicDisplayWidth area, the back/forward buttons are aligned next to it
	// If not, they are aligned with the left side of the m_topicDisplayWidth area

	int widgetHeight = h - (GuideNavBarButtonHorizontalPadding * 2);
	int widgetY0 = y0 + ((h - widgetHeight) / 2);

	int buttonWidth = std::max<int>({m_backButton->idealWidth(), m_nextButton->idealWidth(), m_closeButton->idealWidth(), (m_optionsButton) ? m_optionsButton->idealWidth() : 0, (m_sidebarButton) ? m_sidebarButton->idealWidth() : 0});

	// Start by left-aligning the sidebar button with the overall widget
	int sidebarButtonX1 = 0;
	if (m_sidebarButton)
	{
		m_sidebarButton->setGeometry(x0 + GuideNavBarButtonHorizontalPadding, widgetY0, buttonWidth, widgetHeight);
		sidebarButtonX1 = m_sidebarButton->x() + m_sidebarButton->width();
	}

	// Align the back & forward buttons
	int navButtonsX0 = std::max<int32_t>(sidebarButtonX1 + GuideNavBarButtonHorizontalPadding, ((w > m_topicDisplayWidth) ? w - m_topicDisplayWidth : 0) + GuideNavBarButtonHorizontalPadding);
	m_backButton->setGeometry(navButtonsX0, widgetY0, buttonWidth, widgetHeight);
	navButtonsX0 += m_backButton->width() + GuideNavBarButtonHorizontalPadding;
	m_nextButton->setGeometry(navButtonsX0, widgetY0, buttonWidth, widgetHeight);
	int navButtonsX1 = m_nextButton->x() + m_nextButton->width();

	// Center the title within the right-aligned topicDisplayWidth area
	int titleX0 = (std::min<int32_t>(m_topicDisplayWidth, w) - m_titleLabel->idealWidth()) / 2;
	titleX0 += ((w > m_topicDisplayWidth) ? w - m_topicDisplayWidth : 0);
	titleX0 = std::max<int32_t>(titleX0, navButtonsX1 + GuideNavBarButtonHorizontalPadding);
	m_titleLabel->setGeometry(titleX0, widgetY0, m_titleLabel->idealWidth(), widgetHeight);

	// Right-align the options and close buttons
	m_closeButton->setGeometry(w - (buttonWidth + GuideNavBarButtonHorizontalPadding), widgetY0, buttonWidth, widgetHeight);
	m_optionsButton->setGeometry(m_closeButton->x() - (buttonWidth + GuideNavBarButtonHorizontalPadding), widgetY0, buttonWidth, widgetHeight);
}

void WzGuideTopicsNavBar::triggerCloseHandler()
{
	ASSERT_OR_RETURN(, m_onCloseHandler != nullptr, "Missing onCloseHandler");
	m_onCloseHandler();
}

void WzGuideTopicsNavBar::triggerSidebarToggleHandler()
{
	ASSERT_OR_RETURN(, m_onSidebarToggleHandler != nullptr, "Missing onSidebarToggleHandler");
	m_onSidebarToggleHandler();
}

void WzGuideTopicsNavBar::updateButtonStates()
{
	m_backButton->setState((m_currentStateIndex > 0) ? 0 : WBUT_DISABLE);
	m_nextButton->setState((!m_stateHistory.empty() && (m_currentStateIndex < (m_stateHistory.size() - 1))) ? 0 : WBUT_DISABLE);
}

void WzGuideTopicsNavBar::pushState(const std::string &topicIdentifier)
{
	if (!m_stateHistory.empty() && m_currentStateIndex != (m_stateHistory.size() - 1))
	{
		// truncate state history
		m_stateHistory.resize(m_currentStateIndex + 1);
	}
	m_stateHistory.push_back(topicIdentifier);
	m_currentStateIndex = m_stateHistory.size() - 1;
	updateButtonStates();
}

bool WzGuideTopicsNavBar::navBack()
{
	if (m_currentStateIndex == 0)
	{
		return false;
	}
	--m_currentStateIndex;
	if (m_popStateHandler)
	{
		m_popStateHandler(m_stateHistory[m_currentStateIndex]);
	}
	updateButtonStates();
	return true;
}

bool WzGuideTopicsNavBar::navForward()
{
	if (m_currentStateIndex == (m_stateHistory.size() - 1))
	{
		return false;
	}
	++m_currentStateIndex;
	if (m_popStateHandler)
	{
		m_popStateHandler(m_stateHistory[m_currentStateIndex]);
	}
	updateButtonStates();
	return true;
}

std::shared_ptr<WIDGET> WzGuideTopicsNavBar::createOptionsPopoverForm()
{
	// create all the buttons / option rows
	auto createOptionsCheckbox = [](const WzString& text, bool isChecked, bool isDisabled, const std::function<void (WzCheckboxButton& button)>& onClickFunc) -> std::shared_ptr<WzCheckboxButton> {
		auto pCheckbox = std::make_shared<WzCheckboxButton>();
		pCheckbox->pText = text;
		pCheckbox->FontID = font_regular;
		pCheckbox->setIsChecked(isChecked);
		pCheckbox->setTextColor(WZCOL_TEXT_BRIGHT);
		if (isDisabled)
		{
			pCheckbox->setState(WBUT_DISABLE);
		}
		Vector2i minimumDimensions = pCheckbox->calculateDesiredDimensions();
		pCheckbox->setGeometry(0, 0, minimumDimensions.x, minimumDimensions.y);
		if (onClickFunc)
		{
			pCheckbox->addOnClickHandler([onClickFunc](W_BUTTON& button){
				auto checkBoxButton = std::dynamic_pointer_cast<WzCheckboxButton>(button.shared_from_this());
				ASSERT_OR_RETURN(, checkBoxButton != nullptr, "checkBoxButton is null");
				onClickFunc(*checkBoxButton);
			});
		}
		return pCheckbox;
	};

	std::vector<std::shared_ptr<WIDGET>> buttons;
	buttons.push_back(createOptionsCheckbox(_("Disable Topic Pop-ups"), disableTopicPopups, false, [](WzCheckboxButton& button){
		disableTopicPopups = button.getIsChecked();
	}));

	// determine required height for all buttons
	int totalButtonHeight = std::accumulate(buttons.begin(), buttons.end(), 0, [](int a, const std::shared_ptr<WIDGET>& b) {
		return a + b->height();
	});
	int maxButtonWidth = (*(std::max_element(buttons.begin(), buttons.end(), [](const std::shared_ptr<WIDGET>& a, const std::shared_ptr<WIDGET>& b){
		return a->width() < b->width();
	})))->width();

	auto itemsList = ScrollableListWidget::make();
	itemsList->setBackgroundColor(WZCOL_MENU_BACKGROUND);
	itemsList->setBorderColor(pal_RGBA(152, 151, 154, 255));
	Padding padding = {8, 8, 8, 8};
	itemsList->setPadding(padding);
	const int itemSpacing = 4;
	itemsList->setItemSpacing(itemSpacing);
	itemsList->setGeometry(itemsList->x(), itemsList->y(), maxButtonWidth + padding.left + padding.right, totalButtonHeight + ((static_cast<int>(buttons.size()) - 1) * itemSpacing) + padding.top + padding.bottom);
	for (auto& button : buttons)
	{
		itemsList->addItem(button);
	}

	return itemsList;
}

void WzGuideTopicsNavBar::displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent)
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The WzPlayerBoxTabs does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	newRootFrm->displayFunction = displayChildDropShadows;
	std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
	std::weak_ptr<WzGuideTopicsNavBar> psWeakNavBar = std::dynamic_pointer_cast<WzGuideTopicsNavBar>(shared_from_this());
	newRootFrm->onClickedFunc = [psWeakOptionsOverlayScreen, psWeakNavBar]() {
		if (auto psOverlayScreen = psWeakOptionsOverlayScreen.lock())
		{
			widgRemoveOverlayScreen(psOverlayScreen);
		}
		// Destroy Options overlay / overlay screen, reset options button state
		if (auto strongNavBar = psWeakNavBar.lock())
		{
			strongNavBar->optionsOverlayScreen.reset();
			strongNavBar->m_optionsButton->setState(0);
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	optionsOverlayScreen->psForm->attach(newRootFrm);

	// Create the pop-over form
	auto optionsPopOver = createOptionsPopoverForm();
	newRootFrm->attach(optionsPopOver);

	// Position the pop-over form
	std::weak_ptr<WIDGET> weakParent = psParent;
	optionsPopOver->setCalcLayout([weakParent](WIDGET *psWidget) {
		auto psParent = weakParent.lock();
		ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");
		// (Ideally, with its right aligned with the right side of the "parent" widget, but ensure full visibility on-screen)
		int popOverX0 = (psParent->screenPosX() + psParent->width()) - psWidget->width() - 4;
		if (popOverX0 + psWidget->width() > screenWidth)
		{
			popOverX0 = screenWidth - psWidget->width();
		}
		// (Ideally, with its top aligned with the bottom of the "parent" widget, but ensure full visibility on-screen)
		int popOverY0 = psParent->screenPosY() + psParent->height() - 4;
		if (popOverY0 + psWidget->height() > screenHeight)
		{
			popOverY0 = screenHeight - psWidget->height();
		}
		psWidget->move(popOverX0, popOverY0);
	});

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
}

// MARK: - WzGuideSidebar implementation

void WzGuideSidebarTopicButton::run(W_CONTEXT *)
{
	wzText.tick();
}

void WzGuideSidebarTopicButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();
	int x1 = x0 + w;
	int y1 = y0 + h;

	int maxTextDisplayableWidth = w - (leftPadding + rightPadding);

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	optional<PIELIGHT> backColor = nullopt;
	if (isSelected)
	{
		backColor = WZCOL_MENU_SCORE_BUILT;
	}
	else if (isHighlight)
	{
		backColor = pal_RGBA(255, 255, 255, 50);
	}
	if (backColor.has_value())
	{
		pie_UniTransBoxFill(x0, y0, x1, y1, backColor.value());
	}

	// Handle isNew (display a "new" blue indicator on the right side)
	if (isNew)
	{
		const int indicatorSize = 10;
		PIELIGHT indicatorColor = pal_RGBA(35, 82, 252, 255); //pal_RGBA(35,252,115,255);
		int newIndicatorX0 = x0 + (w - indicatorSize - rightPadding);
		int newIndicatorY0 = y0 + ((h - indicatorSize) / 2);
		if (IntImages)
		{
			iV_DrawImageTint(IntImages, IMAGE_INDICATOR_DOT, newIndicatorX0, newIndicatorY0, indicatorColor, Vector2f(indicatorSize, indicatorSize));
		}
		else
		{
			// Interface Images are not loaded - just use simple rectangles
			pie_UniTransBoxFill(newIndicatorX0, newIndicatorY0, newIndicatorX0 + indicatorSize, newIndicatorY0 + indicatorSize, indicatorColor);
		}

		maxTextDisplayableWidth -= (indicatorSize + rightPadding);
	}

	if (haveText)
	{
		wzText.setText(pText, FontID);

		PIELIGHT textColor = (isDown || isSelected || isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}

		int indentOffsetX = (indentPadding * indentLevel);
		int textX0 = x0 + leftPadding + indentOffsetX;
		int textY0 = y0 + (h - wzText->lineSize()) / 2 - wzText->aboveBase();
		maxTextDisplayableWidth -= indentOffsetX;

		isTruncated = maxTextDisplayableWidth < wzText->width();
		if (isTruncated)
		{
			maxTextDisplayableWidth -= (iV_GetEllipsisWidth(FontID) + 2);
		}
		wzText->render(textX0, textY0, textColor, 0.0f, maxTextDisplayableWidth);
		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(FontID, Vector2f(textX0 + maxTextDisplayableWidth + 2, textY0), textColor);
		}
	}
}

void WzGuideSidebar::changeSelectedItem(const std::string& topicIdentifier)
{
	auto it = m_topicIdentifierToListIdMap.find(topicIdentifier);
	if (it == m_topicIdentifierToListIdMap.end())
	{
		m_currentSelectedTopicIdentifier.clear();
		return;
	}
	ASSERT_OR_RETURN(, it->second < m_topicButtons.size(), "Invalid topic identifier mapping: %zu", it->second);
	for (const auto& but : m_topicButtons)
	{
		but->setIsSelected(false);
	}
	m_topicButtons[it->second]->setIsSelected(true);
	m_topicButtons[it->second]->setIsNew(false);
	m_currentSelectedTopicIdentifier = topicIdentifier;
	if (width() > 0 && height() > 0)
	{
		m_topicsList->scrollEnsureItemVisible(it->second);
	}
}

optional<std::string> WzGuideSidebar::getItemTopicIdentifier(size_t idx)
{
	if (idx >= m_topicButtons.size())
	{
		return nullopt;
	}
	return m_topicButtons[idx]->getTopicIdentifier();
}

static std::vector<std::shared_ptr<WzWrappedGuideTopic>> makeSortedNestedTopics(const std::vector<std::shared_ptr<WzWrappedGuideTopic>>& topics)
{
	std::vector<std::shared_ptr<WzWrappedGuideTopic>> sortedTopics;
	for (auto& wrappedTopic : topics)
	{
		if (wrappedTopic->children.empty())
		{
			sortedTopics.push_back(wrappedTopic);
		}
		else
		{
			auto childSortedWrappedTopicCopy = std::make_shared<WzWrappedGuideTopic>(*wrappedTopic);
			childSortedWrappedTopicCopy->children = makeSortedNestedTopics(childSortedWrappedTopicCopy->children);
			sortedTopics.push_back(childSortedWrappedTopicCopy);
		}
	}

	// sort the topics list at this level
	auto& f = std::use_facet<std::collate<char>>(std::locale());
	auto& f_c_locale = std::use_facet<std::collate<char>>(std::locale::classic());
	std::sort(sortedTopics.begin(), sortedTopics.end(), [&f, &f_c_locale](const std::shared_ptr<WzWrappedGuideTopic>& a, const std::shared_ptr<WzWrappedGuideTopic>& b) -> bool {
		bool aHasSortKey = a->topic->sortKey.has_value();
		bool bHasSortKey = b->topic->sortKey.has_value();
		if (aHasSortKey != bHasSortKey)
		{
			return aHasSortKey;
		}
		if (aHasSortKey)
		{
			const auto& sortKeyA = a->topic->sortKey.value();
			const auto& sortKeyB = b->topic->sortKey.value();
			const auto pStrA = sortKeyA.c_str();
			const auto pStrB = sortKeyB.c_str();
			return f_c_locale.compare(pStrA, pStrA + sortKeyA.length(), pStrB, pStrB + sortKeyB.length()) < 0;
		}
		else
		{
			const auto pStrA = a->topic->displayName.getLocalizedString(WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN);
			const auto pStrB = b->topic->displayName.getLocalizedString(WZ2100_GUIDE_MESSAGE_CATALOG_DOMAIN);
			return f.compare(pStrA, pStrA + strlen(pStrA), pStrB, pStrB + strlen(pStrB)) < 0;
		}
	});

	return sortedTopics;
}

std::vector<std::shared_ptr<WzGuideSidebarTopicButton>> WzGuideSidebar::makeTopicWidgets(const std::vector<std::shared_ptr<WzWrappedGuideTopic>>& topics, const std::shared_ptr<WzGuideTopicsRegistry>& registry, size_t level /*= 0*/)
{
	std::vector<std::shared_ptr<WzGuideSidebarTopicButton>> result;
	for (auto& wrappedTopic : topics)
	{
		if (!wrappedTopic->topic)
		{
			continue;
		}
		// create button for this topic
		auto button = WzGuideSidebarTopicButton::make(wrappedTopic->topic, level, registry->getTopicPrefs(wrappedTopic));
		if (!button)
		{
			ASSERT(false, "Failed to create sidebar topic button?");
			continue;
		}
		auto weakParent = std::weak_ptr<WzGuideSidebar>(std::dynamic_pointer_cast<WzGuideSidebar>(shared_from_this()));
		button->addOnClickHandler([weakParent](W_BUTTON& but) {
			auto psSelf = std::dynamic_pointer_cast<WzGuideSidebarTopicButton>(but.shared_from_this());
			ASSERT_OR_RETURN(, psSelf != nullptr, "Unexpected type?");
			if (!psSelf->getIsSelected())
			{
				std::string topicIdentifierCopy = psSelf->getTopicIdentifier();
				widgScheduleTask([weakParent, topicIdentifierCopy]() {
					auto strongParent = weakParent.lock();
					auto strongGuideForm = strongParent->m_parentGuideForm.lock();
					strongGuideForm->showGuideTopic(topicIdentifierCopy);
				});
			}
		});
		result.push_back(button);

		// add buttons for children
		if (!wrappedTopic->children.empty())
		{
			auto childButtons = makeTopicWidgets(wrappedTopic->children, registry, level+1);
			result.insert(std::end(result), std::begin(childButtons), std::end(childButtons));
		}
	}

	return result;
}

void WzGuideSidebar::updateList(const std::shared_ptr<WzGuideTopicsRegistry>& registry)
{
	m_topicsList->clear();
	m_topicIdentifierToListIdMap.clear();
	m_topicButtons.clear();

	// sort the topics at each level
	std::vector<std::shared_ptr<WzWrappedGuideTopic>> sortedTopics = makeSortedNestedTopics(registry->getTopics());

	m_topicButtons = makeTopicWidgets(sortedTopics, registry);

	size_t listIdx = 0;
	for (auto& topicButton : m_topicButtons)
	{
		m_topicIdentifierToListIdMap.insert({topicButton->getTopicIdentifier(), listIdx});
		m_topicsList->addItem(topicButton);
		++listIdx;
	}

	// re-select the previously-selected item
	if (!m_currentSelectedTopicIdentifier.empty())
	{
		auto previouslySelectedTopicId = m_currentSelectedTopicIdentifier;
		changeSelectedItem(previouslySelectedTopicId);
	}

	geometryChanged();
}

void WzGuideSidebar::geometryChanged()
{
	m_topicsList->callCalcLayout();
}

void WzGuideSidebar::initialize(const std::shared_ptr<WzGuideForm>& parentGuideForm)
{
	m_topicsList = ScrollableListWidget::make();
	m_topicsList->setPadding({listTopPadding, 0, 0, 1});
	attach(m_topicsList);
	m_topicsList->setCalcLayout([](WIDGET* psWidget) {
		auto parent = std::dynamic_pointer_cast<WzGuideSidebar>(psWidget->parent());
		ASSERT_OR_RETURN(, parent != nullptr, "No parent?");
		psWidget->setGeometry(0, 0, std::max<int>(parent->width()-1, 0), std::max<int>(parent->height()-1, 0));
	});
	m_parentGuideForm = parentGuideForm;
}

void WzGuideSidebar::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	iV_Line(x1, y0, x1, y1, pal_RGBA(255,255,255,100));
}

std::shared_ptr<WzGuideSidebar> WzGuideSidebar::make(const std::shared_ptr<WzGuideForm>& parentGuideForm, const std::shared_ptr<WzGuideTopicsRegistry>& registry)
{
	class make_shared_enabler: public WzGuideSidebar {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(parentGuideForm);
	widget->updateList(registry);
	return widget;
}

// MARK: - WzGuideForm implementation

int32_t WzGuideForm::idealWidth()
{
	int32_t w = topicsDisplayWidth;
	if (m_sidebarVisible)
	{
		w += sidebarDisplayWidth;
	}
	return w;
}

int32_t WzGuideForm::idealHeight()
{
	return std::max<int32_t>(480, 200); // TODO: Implement
}

std::shared_ptr<WzGuideForm> WzGuideForm::make(const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnCloseFunc& onCloseHandler)
{
	class make_shared_enabler: public WzGuideForm {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(registry, onCloseHandler);
	return widget;
}

void WzGuideForm::initialize(const std::shared_ptr<WzGuideTopicsRegistry>& registry, const OnCloseFunc &onCloseHandler)
{
	m_registry = registry;
	m_closeHandler = onCloseHandler;

	enableMinimizing(_("Game Guide"));
	userMovable = true;

	std::weak_ptr<WzGuideForm> psWeakGuideForm = std::dynamic_pointer_cast<WzGuideForm>(shared_from_this());

	m_navBar = WzGuideTopicsNavBar::make(topicsDisplayWidth, [psWeakGuideForm](const std::string& topicIdentifier){
		// nav pop state handler
		std::string topicIdentifierCopy = topicIdentifier;
		widgScheduleTask([psWeakGuideForm, topicIdentifierCopy]{
			auto psStrongGuideForm = psWeakGuideForm.lock();
			ASSERT_OR_RETURN(, psStrongGuideForm != nullptr, "No parent??");
			psStrongGuideForm->displayGuideTopic(topicIdentifierCopy);
		});
	}, [psWeakGuideForm](){
		// on sidebar toggle handler
		widgScheduleTask([psWeakGuideForm]{
			auto psStrongGuideForm = psWeakGuideForm.lock();
			ASSERT_OR_RETURN(, psStrongGuideForm != nullptr, "No parent??");
			psStrongGuideForm->toggleSidebar();
		});
	}, [psWeakGuideForm](){
		// close handler
		widgScheduleTask([psWeakGuideForm]{
			auto psStrongGuideForm = psWeakGuideForm.lock();
			ASSERT_OR_RETURN(, psStrongGuideForm != nullptr, "No parent??");
			psStrongGuideForm->m_closeHandler();
		});
	});
	m_navBar->setTransparentToClicks(true);
	attach(m_navBar);

	m_sideBar = WzGuideSidebar::make(std::dynamic_pointer_cast<WzGuideForm>(shared_from_this()), registry);
	attach(m_sideBar);
}

void WzGuideForm::toggleSidebar()
{
	m_sidebarVisible = !m_sidebarVisible;
	resizeToIdeal();
}

void WzGuideForm::setSidebarVisible(bool visible)
{
	m_sidebarVisible = visible;
	resizeToIdeal();
}

void WzGuideForm::resizeToIdeal()
{
	int guideFormHeight = idealHeight();
	int guideFormWidth = idealWidth();

	int x0 = x() + (width() - guideFormWidth);

	int y0 = y();
	int maxX1 = x0 + guideFormWidth;
	int maxY1 = y0 + guideFormHeight;
	auto psParent = parent();
	if (psParent)
	{
		maxX1 = psParent->width() - 10;
		maxY1 = psParent->height();
	}
	if (x0 + guideFormWidth > maxX1)
	{
		x0 = std::max<int>(maxX1 - guideFormWidth, 0);
	}
	if (y0 + guideFormHeight > maxY1)
	{
		y0 = std::max<int>(maxY1 - guideFormHeight, 0);
	}
	x0 = std::max<int>(x0, 0);
	y0 = std::max<int>(y0, 0);

	// set new width + height, adjusting x,y if needed to keep in bounds
	setGeometry(x0, y0, guideFormWidth, guideFormHeight);
}

void WzGuideForm::queueRefreshTopicsList()
{
	m_queuedTopicsListRefresh = true;
}

void WzGuideForm::geometryChanged()
{
	int w = width();
	int h = height();

	int32_t navbarHeight = m_navBar->idealHeight();
	m_navBar->setGeometry(0, 0, w, navbarHeight);

	int32_t heightMinusNavbar = std::max<int32_t>(h - navbarHeight, 0);
	if (heightMinusNavbar == 0)
	{
		return;
	}

	if (m_guideTopic)
	{
		int32_t guideTopicX0 = std::max<int32_t>(w - topicsDisplayWidth, 0);
		m_guideTopic->setGeometry(guideTopicX0, navbarHeight, std::min<int32_t>(topicsDisplayWidth, w), heightMinusNavbar);
	}

	if (m_sidebarVisible)
	{
		m_sideBar->show();
		int32_t sidebarWidth = std::max<int32_t>(w - topicsDisplayWidth, 0);
		m_sideBar->setGeometry(0, navbarHeight, sidebarWidth, heightMinusNavbar);
	}
	else
	{
		m_sideBar->hide();
	}
}

void WzGuideForm::run(W_CONTEXT *)
{
	updateSidebarIfNeeded();
}

void WzGuideForm::updateSidebarIfNeeded()
{
	if (m_queuedTopicsListRefresh)
	{
		if (m_sideBar)
		{
			m_sideBar->updateList(m_registry);
		}
		m_queuedTopicsListRefresh = false;
	}
}

void WzGuideForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	// draw "drop-shadow"
	int dropShadowX0 = std::max<int>(x0 - 6, 0);
	int dropShadowY0 = std::max<int>(y0 - 6, 0);
	int dropShadowX1 = std::min<int>(x1 + 6, pie_GetVideoBufferWidth());
	int dropShadowY1 = std::min<int>(y1 + 6, pie_GetVideoBufferHeight());
	pie_UniTransBoxFill((float)dropShadowX0, (float)dropShadowY0, (float)dropShadowX1, (float)dropShadowY1, pal_RGBA(0, 0, 0, 40));

	// draw background + border
	pie_UniTransBoxFill((float)x0, (float)y0, (float)x1, (float)y1, pal_RGBA(5, 0, 15, 170));
	iV_Box(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 150));
}


bool WzGuideForm::displayGuideTopic(const std::string& topicIdentifier, OnTopicCloseFunc onTopicClose)
{
	auto psTopic = m_registry->getTopic(topicIdentifier);
	ASSERT_OR_RETURN(false, psTopic != nullptr, "Topic not found: %s", topicIdentifier.c_str());
	ASSERT_OR_RETURN(false, psTopic->topic != nullptr, "Topic is null?: %s", topicIdentifier.c_str());

	m_registry->setTopicSeen(psTopic);

	if (m_guideTopic)
	{
		if (m_guideTopic->getDisplayedTopicID() == topicIdentifier)
		{
			// already displaying this topic
			return true;
		}
		detach(m_guideTopic);
		m_guideTopic.reset();
	}

	m_guideTopic = WzGuideTopicDisplayWidget::make(*psTopic, m_registry);
	ASSERT_OR_RETURN(false, m_guideTopic != nullptr, "Failed to create guide topic display for: %s", topicIdentifier.c_str());
	if (onTopicClose)
	{
		std::string topicIdentifierCopy = topicIdentifier;
		m_guideTopic->setOnDelete([onTopicClose, topicIdentifierCopy](WIDGET *) {
			widgScheduleTask([onTopicClose, topicIdentifierCopy]() {
				onTopicClose(topicIdentifierCopy);
			});
		});
	}
	attach(m_guideTopic);
	updateSidebarIfNeeded();
	m_sideBar->changeSelectedItem(topicIdentifier);
	geometryChanged();
	return true;
}

void WzGuideForm::showGuideTopic(const std::string& topicIdentifier, OnTopicCloseFunc onTopicClose)
{
	if (m_guideTopic && (m_guideTopic->getDisplayedTopicID() == topicIdentifier))
	{
		// already displaying this topic
		return;
	}

	if (displayGuideTopic(topicIdentifier, onTopicClose))
	{
		m_navBar->pushState(topicIdentifier);
	}
}

// shows the first guide topic if not topic is currently displayed
void WzGuideForm::showFirstGuideTopicIfNeeded()
{
	if (m_guideTopic)
	{
		// already displaying a guide topic
		return;
	}

	// otherwise, fetch the first guide topic in the sorted sidebar list
	updateSidebarIfNeeded();
	auto firstSortedTopicId = m_sideBar->getItemTopicIdentifier(0);
	if (firstSortedTopicId.has_value())
	{
		if (displayGuideTopic(firstSortedTopicId.value()))
		{
			m_navBar->pushState(firstSortedTopicId.value());
		}
	}
}

// MARK: -

class WzGameGuideScreen_CLICKFORM : public W_CLICKFORM
{
protected:
	static constexpr int32_t INTERNAL_PADDING = 15;
protected:
	WzGameGuideScreen_CLICKFORM(W_FORMINIT const *init);
	WzGameGuideScreen_CLICKFORM();
	~WzGameGuideScreen_CLICKFORM();
	void initialize();
public:
	static std::shared_ptr<WzGameGuideScreen_CLICKFORM> make(UDWORD formID = 0);
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override
	{
		recalcLayout();
	}
	void recalcLayout();

	std::shared_ptr<WzGuideForm>& getGuideForm() { return guideForm; }

private:
	void clearFocusAndEditing();
	void handleClickOnForm();

public:
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 80);
	std::function<void ()> onClickedFunc;
	std::function<void ()> onCancelPressed;
private:
	std::shared_ptr<WzGuideForm> guideForm;
	const int edgeMargin = 10;
};

struct WzGameGuideScreen: public W_SCREEN
{
protected:
	WzGameGuideScreen(): W_SCREEN() {}
	~WzGameGuideScreen()
	{
		setFocus(nullptr);
	}

public:
	typedef std::function<void ()> OnCloseFunc;
	static std::shared_ptr<WzGameGuideScreen> make(const OnCloseFunc& onCloseFunction, bool expandedSidebarByDefault);
	void reopeningScreen(const OnCloseFunc& onCloseFunction, bool expandedSidebarByDefault);

public:
	void closeScreen(bool force = false);
	std::shared_ptr<WzGuideForm>& getGuideForm();

private:
	void pauseGameIfNeeded();
	void unpauseGameIfNeeded();

private:
	OnCloseFunc onCloseFunc;
	std::chrono::steady_clock::time_point openTime;
	bool didStopGameTime = false;
	bool didSetPauseStates = false;

private:
	WzGameGuideScreen(WzGameGuideScreen const &) = delete;
	WzGameGuideScreen &operator =(WzGameGuideScreen const &) = delete;
};

// MARK: - WzGameGuideScreen implementation

WzGameGuideScreen_CLICKFORM::WzGameGuideScreen_CLICKFORM(W_FORMINIT const *init) : W_CLICKFORM(init) {}
WzGameGuideScreen_CLICKFORM::WzGameGuideScreen_CLICKFORM() : W_CLICKFORM() {}
WzGameGuideScreen_CLICKFORM::~WzGameGuideScreen_CLICKFORM()
{ }

std::shared_ptr<WzGameGuideScreen_CLICKFORM> WzGameGuideScreen_CLICKFORM::make(UDWORD formID)
{
	W_FORMINIT sInit;
	sInit.id = formID;
	sInit.style = WFORM_PLAIN | WFORM_CLICKABLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;
	sInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	});

	class make_shared_enabler: public WzGameGuideScreen_CLICKFORM
	{
	public:
		make_shared_enabler(W_FORMINIT const *init): WzGameGuideScreen_CLICKFORM(init) {}
	};
	auto widget = std::make_shared<make_shared_enabler>(&sInit);
	widget->initialize();
	return widget;
}

void WzGameGuideScreen_CLICKFORM::initialize()
{
	auto weakSelf = std::weak_ptr<WzGameGuideScreen_CLICKFORM>(std::dynamic_pointer_cast<WzGameGuideScreen_CLICKFORM>(shared_from_this()));

	guideForm = WzGuideForm::make(guideTopicsRegistry, [weakSelf]() {
		// on guide form close
		widgScheduleTask([weakSelf]() {
			// click on the form so we ultimately close the chat screen
			auto strongParentForm = weakSelf.lock();
			ASSERT_OR_RETURN(, strongParentForm != nullptr, "No parent form");
			auto parentScreen = std::dynamic_pointer_cast<WzGameGuideScreen>(strongParentForm->screenPointer.lock());
			ASSERT_OR_RETURN(, parentScreen != nullptr, "No screen?");
			parentScreen->closeScreen(true);
		});
	});
	attach(guideForm);
	// initially position right-aligned in parent (which takes up the whole screen)
	int guideFormHeight = guideForm->idealHeight();
	int guideFormWidth = guideForm->idealWidth();
	int x0 = (screenWidth - guideFormWidth - edgeMargin);
	int y0 = (screenHeight - guideFormHeight) / 2;
	guideForm->setGeometry(x0, y0, guideFormWidth, guideFormHeight);
}

void WzGameGuideScreen_CLICKFORM::recalcLayout()
{
	int w = width();
	int h = height();

	// ensure guideForm is within window bounds
	int guideFormHeight = guideForm->idealHeight();
	int guideFormWidth = guideForm->idealWidth();

	int x0 = clip<int>(guideForm->x(), edgeMargin, w - guideFormWidth - edgeMargin);
	int y0 = std::min<int>(guideForm->y(), h - guideFormHeight);

	guideForm->setGeometry(x0, y0, guideFormWidth, guideFormHeight);
}

void WzGameGuideScreen_CLICKFORM::clearFocusAndEditing()
{
	if (auto lockedScreen = screenPointer.lock())
	{
		lockedScreen->setFocus(nullptr);
	}
}

void WzGameGuideScreen_CLICKFORM::handleClickOnForm()
{
	clearFocusAndEditing();
	if (onClickedFunc)
	{
		onClickedFunc();
	}
}

void WzGameGuideScreen_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	handleClickOnForm();
}

void WzGameGuideScreen_CLICKFORM::display(int xOffset, int yOffset)
{
	if (!visible())
	{
		return; // skip if hidden
	}

	if (backgroundColor.isTransparent())
	{
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	// draw background over everything
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
}

void WzGameGuideScreen_CLICKFORM::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		clearFocusAndEditing();
		if (onCancelPressed)
		{
			onCancelPressed();
			inputLoseFocus();
		}
	}

	if (visible())
	{
		// while this is displayed, clear the input buffer
		// (ensuring that subsequent screens / the main screen do not get the input to handle)
		inputLoseFocus();
	}
}

std::shared_ptr<WzGameGuideScreen> WzGameGuideScreen::make(const OnCloseFunc& _onCloseFunc, bool expandedSidebarByDefault)
{
	class make_shared_enabler: public WzGameGuideScreen {};
	auto newRootFrm = WzGameGuideScreen_CLICKFORM::make();
	auto screen = std::make_shared<make_shared_enabler>();
	screen->initialize(newRootFrm);
	screen->onCloseFunc = _onCloseFunc;
	std::weak_ptr<WzGameGuideScreen> psWeakOverlayScreen(screen);
	newRootFrm->onClickedFunc = [psWeakOverlayScreen]() {
		auto psOverlayScreen = psWeakOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeScreen(false);
		}
	};
	newRootFrm->onCancelPressed = [psWeakOverlayScreen]() {
		auto psOverlayScreen = psWeakOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeScreen(true);
		}
	};

	if (expandedSidebarByDefault)
	{
		screen->getGuideForm()->setSidebarVisible(true);
	}

	screen->pauseGameIfNeeded();
	screen->openTime = std::chrono::steady_clock::now();

	return screen;
}

void WzGameGuideScreen::reopeningScreen(const OnCloseFunc& onCloseFunction, bool expandedSidebarByDefault)
{
	onCloseFunc = onCloseFunction;
	pauseGameIfNeeded();
	openTime = std::chrono::steady_clock::now();
	if (expandedSidebarByDefault)
	{
		getGuideForm()->setSidebarVisible(true);
	}
}

void WzGameGuideScreen::closeScreen(bool force)
{
	if (!force && (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - openTime) < std::chrono::milliseconds(2000)))
	{
		// ignore errant clicks that might otherwise close screen right after it opened
		return;
	}
	widgRemoveOverlayScreen(shared_from_this());
	unpauseGameIfNeeded();
	if (onCloseFunc)
	{
		onCloseFunc();
		// "consumes" the onCloseFunc
		onCloseFunc = nullptr;
	}
}

void WzGameGuideScreen::pauseGameIfNeeded()
{
	if (!runningMultiplayer() && !gameTimeIsStopped())
	{
		/* Stop the clock / simulation */
		gameTimeStop();
		addConsoleMessage(_("PAUSED"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);

		psForm->show(); // show the root form, which means it accepts all clicks on the screen and dims the background

		didStopGameTime = true;

		if (!gamePaused())
		{
			setGamePauseStatus(true);
			setConsolePause(true);
			setScriptPause(true);
			setAudioPause(true);
			setScrollPause(true);
			didSetPauseStates = true;
		}
	}
	else
	{
		psForm->hide(); // hide the root form - it won't accept clicks (but its children will be displayed)
	}
}

void WzGameGuideScreen::unpauseGameIfNeeded()
{
	if (didStopGameTime)
	{
		if (!runningMultiplayer() && gameTimeIsStopped())
		{
			clearActiveConsole();
			/* Start the clock again */
			gameTimeStart();

			//put any widgets back on for the missions
			resetMissionWidgets();
		}
		didStopGameTime = false;
	}

	if (didSetPauseStates)
	{
		setGamePauseStatus(false);
		setConsolePause(false);
		setScriptPause(false);
		setAudioPause(false);
		setScrollPause(false);
		didSetPauseStates = false;
	}
}

std::shared_ptr<WzGuideForm>& WzGameGuideScreen::getGuideForm()
{
	return std::dynamic_pointer_cast<WzGameGuideScreen_CLICKFORM>(psForm)->getGuideForm();
}

// MARK: - Loading topic from JSON

#define NUMRETBUTS	7 // Number of reticule buttons - from hci.cpp

void from_json(const nlohmann::json& j, WzStatVisual::OnlyIfKnown& v)
{
	if (j.is_boolean())
	{
		v = j.get<bool>() ? WzStatVisual::OnlyIfKnown::True : WzStatVisual::OnlyIfKnown::False;
	}
	else if (j.is_string())
	{
		auto valStr = j.get<std::string>();
		if (valStr.compare("false") == 0)
		{
			v = WzStatVisual::OnlyIfKnown::False;
		}
		else if (valStr.compare("true") == 0)
		{
			v = WzStatVisual::OnlyIfKnown::True;
		}
		else if (valStr.compare("campaign") == 0)
		{
			v = WzStatVisual::OnlyIfKnown::CampaignMode;
		}
		else
		{
			throw nlohmann::json::other_error::create(600, "Unknown value (\"" + valStr + "\")", &j);
		}
	}
	else
	{
		throw nlohmann::json::type_error::create(302, "invalid type of value: " + std::string(j.type_name()), &j);
	}
}

void from_json(const nlohmann::json& j, WzStatVisual& v)
{
	if (j.is_object())
	{
		v.statName = j.at("id").get<std::string>();
		auto it = j.find("onlyIfKnown");
		if (it != j.end())
		{
			v.onlyIfKnown = it.value().get<WzStatVisual::OnlyIfKnown>();
		}
	}
	else if (j.is_string())
	{
		// special-case: treat string value as just the id
		v.statName = j.get<std::string>();
	}
	else
	{
		throw nlohmann::json::type_error::create(302, "invalid type of value for \"stat\": " + std::string(j.type_name()), &j);
	}
}

const std::unordered_map<std::string, ActivitySink::GameMode> strToGameModeMap = {
	{ "campaign", ActivitySink::GameMode::CAMPAIGN },
	{ "challenge", ActivitySink::GameMode::CHALLENGE },
	{ "skirmish", ActivitySink::GameMode::SKIRMISH },
	{ "multiplayer", ActivitySink::GameMode::MULTIPLAYER }
};

void from_json(const nlohmann::json& j, ActivitySink::GameMode& v)
{
	if (!j.is_string())
	{
		throw nlohmann::json::type_error::create(302, "invalid type of value - expecting string, got: " + std::string(j.type_name()), &j);
	}

	auto strVal = j.get<std::string>();
	auto it = strToGameModeMap.find(strVal);
	if (it != strToGameModeMap.end())
	{
		v = it->second;
	}
	else
	{
		throw nlohmann::json::other_error::create(600, "Unknown value (\"" + strVal + "\")", &j);
	}
}

const std::unordered_map<std::string, SECONDARY_ORDER> strToSecondaryOrderMap = {
	{ "ATTACK_RANGE", DSO_ATTACK_RANGE },
	{ "REPAIR_LEVEL", DSO_REPAIR_LEVEL },
	{ "ATTACK_LEVEL", DSO_ATTACK_LEVEL },
	{ "ASSIGN_PRODUCTION", DSO_ASSIGN_PRODUCTION },
	{ "ASSIGN_CYBORG_PRODUCTION", DSO_ASSIGN_CYBORG_PRODUCTION },
	{ "CLEAR_PRODUCTION", DSO_CLEAR_PRODUCTION },
	{ "RECYCLE", DSO_RECYCLE },
	{ "PATROL", DSO_PATROL },
	{ "HALTTYPE", DSO_HALTTYPE },
	{ "RETURN_TO_LOC", DSO_RETURN_TO_LOC },
	{ "FIRE_DESIGNATOR", DSO_FIRE_DESIGNATOR },
	{ "ASSIGN_VTOL_PRODUCTION", DSO_ASSIGN_VTOL_PRODUCTION },
	{ "CIRCLE", DSO_CIRCLE },
	{ "ACCEPT_RETREP", DSO_ACCEPT_RETREP }
};

void from_json(const nlohmann::json& j, SECONDARY_ORDER& v)
{
	if (!j.is_string())
	{
		throw nlohmann::json::type_error::create(302, "invalid type of value - expecting string, got: " + std::string(j.type_name()), &j);
	}

	auto strVal = j.get<std::string>();
	auto it = strToSecondaryOrderMap.find(strVal);
	if (it != strToSecondaryOrderMap.end())
	{
		v = it->second;
	}
	else
	{
		throw nlohmann::json::other_error::create(600, "Unknown value (\"" + strVal + "\")", &j);
	}
}

void from_json(const nlohmann::json& j, WzGuideVisual& v)
{
	// A visual must be a single type, so only allow one of the top-level keys to be specified
	bool haveVisualType = false;
	auto findVisualKey = [&j, &haveVisualType](const std::string& key) {
		auto it = j.find(key);
		if (it != j.end())
		{
			if (haveVisualType)
			{
				throw nlohmann::json::other_error::create(600, "Visual must be a single type, but found additional key: (\"" + key + "\")", &j);
			}
			haveVisualType = true;
		}
		return it;
	};

	auto it = findVisualKey("stat");
	if (it != j.end())
	{
		v.statVisual = it.value().get<WzStatVisual>();
	}
	it = findVisualKey("droidTemplate");
	if (it != j.end())
	{
		v.droidTemplateJSON = it.value();
	}
	it = findVisualKey("retButId");
	if (it != j.end())
	{
		if (it.value().is_string())
		{
			auto buttonIdStr = it.value().get<std::string>();
			if (buttonIdStr.compare("CANCEL") == 0)
			{
				v.reticuleButtonId = RETBUT_CANCEL;
			}
			else if (buttonIdStr.compare("FACTORY") == 0)
			{
				v.reticuleButtonId = RETBUT_FACTORY;
			}
			else if (buttonIdStr.compare("RESEARCH") == 0)
			{
				v.reticuleButtonId = RETBUT_RESEARCH;
			}
			else if (buttonIdStr.compare("BUILD") == 0)
			{
				v.reticuleButtonId = RETBUT_BUILD;
			}
			else if (buttonIdStr.compare("DESIGN") == 0)
			{
				v.reticuleButtonId = RETBUT_DESIGN;
			}
			else if (buttonIdStr.compare("INTELMAP") == 0)
			{
				v.reticuleButtonId = RETBUT_INTELMAP;
			}
			else if (buttonIdStr.compare("COMMAND") == 0)
			{
				v.reticuleButtonId = RETBUT_COMMAND;
			}
			else
			{
				throw nlohmann::json::other_error::create(600, "Unknown value (\"" + buttonIdStr + "\")", &j);
			}
		}
		else
		{
			auto buttonId = it.value().get<int>();
			if (buttonId >= 0 && buttonId < NUMRETBUTS)
			{
				v.reticuleButtonId = buttonId;
			}
			else
			{
				throw nlohmann::json::other_error::create(600, "Unknown value (\"" + std::to_string(buttonId) + "\")", &j);
			}
		}
	}
	it = findVisualKey("imagePath");
	if (it != j.end())
	{
		v.imagePath = it.value().get<std::string>();
	}
	it = findVisualKey("secondaryUnitOrderButtons");
	if (it != j.end())
	{
		v.secondaryUnitOrderButtons = it.value().get<SECONDARY_ORDER>();
	}
}

static void fixupVisualPaths(WzGuideVisual& visual, const std::string& baseDir, const nlohmann::json& j)
{
	if (visual.imagePath.has_value() && !visual.imagePath.value().empty())
	{
		const auto& relativePath = visual.imagePath.value();

		std::string fullImagePath = baseDir;
		if (strEndsWith(fullImagePath, "/"))
		{
			fullImagePath.resize(fullImagePath.size() - 1);
		}

		// supporting "../"s at the beginning of the relativePath
		const std::string parentDir = "../";
		size_t relStart = 0;
		while (relativePath.compare(relStart, parentDir.size(), parentDir) == 0)
		{
			// remove the last path component from the baseDir
			if (fullImagePath.empty())
			{
				// no parent directory to traverse
				throw nlohmann::json::other_error::create(600, "Invalid imagePath (\"" + relativePath + "\"), has too many \"../\"", &j);
			}
			auto basePos = fullImagePath.rfind("/");
			if (basePos == std::string::npos || basePos == 0)
			{
				fullImagePath.clear();
			}
			else
			{
				fullImagePath = fullImagePath.substr(0, basePos);
			}
			relStart += 3;
		}

		if (!fullImagePath.empty() && !strEndsWith(fullImagePath, "/"))
		{
			fullImagePath += "/";
		}
		fullImagePath += relativePath.substr(relStart);
		visual.imagePath = std::move(fullImagePath);
	}
}

std::vector<WzGuideVisual> loadWZGuideVisualsFromJSON(const nlohmann::json& j, const std::string& baseDir)
{
	std::vector<WzGuideVisual> result;
	if (j.is_array())
	{
		result.reserve(j.size());
		for (size_t i = 0; i < j.size(); ++i)
		{
			result.push_back(j[i].get<WzGuideVisual>());
			fixupVisualPaths(result.back(), baseDir, j[i]);
		}
	}
	else if (j.is_object())
	{
		// a single visual
		result.reserve(1);
		result.push_back(j.get<WzGuideVisual>());
		fixupVisualPaths(result.back(), baseDir, j);
	}
	else
	{
		throw nlohmann::json::type_error::create(302, "type must be an object or array, but is " + std::string(j.type_name()), &j);
	}
	return result;
}

WzGuideTopic loadWZGuideTopicFromJSON(const std::string& expectedTopicId, const nlohmann::json& j, const std::string& baseDir)
{
	WzGuideTopic v;

	if (!j.is_object())
	{
		throw nlohmann::json::type_error::create(302, "type must be an object, but is " + std::string(j.type_name()), &j);
	}

	v.identifier = j.at("id").get<std::string>();
	if (v.identifier != expectedTopicId)
	{
		throw nlohmann::json::other_error::create(600, "\"id\" must match expected (\"" + expectedTopicId + "\"), but is \"" + v.identifier + "\"", &j);
	}
	auto it = j.find("version");
	if (it != j.end())
	{
		v.version = it.value().get<std::string>();
	}
	else
	{
		v.version = "0";
	}
	it = j.find("sort");
	if (it != j.end())
	{
		v.sortKey = it.value().get<std::string>();
	}
	v.displayName = j.at("title").get<WzJsonLocalizedString>();
	it = j.find("visual");
	if (it != j.end())
	{
		v.visuals = loadWZGuideVisualsFromJSON(it.value(), baseDir);
	}
	v.contents = j.at("contents").get<std::vector<WzJsonLocalizedString>>();
	it = j.find("links");
	if (it != j.end())
	{
		v.linkedTopicIdentifiers = it.value().get<std::vector<std::string>>();
	}
	it = j.find("applicableModes");
	if (it != j.end())
	{
		v.applicableModes = it.value().get<std::unordered_set<ActivitySink::GameMode>>();
	}

	return v;
}

optional<std::string> convertGuideTopicIdToExpectedJSONFilePath(const std::string& guideTopicId, bool appendJSONFileExtension = true)
{
	ASSERT_OR_RETURN(nullopt, !guideTopicId.empty(), "Empty guide topic id");
	std::string result = "guidetopics";
	std::string delim = "::";
	size_t start = 0;
	auto end = guideTopicId.find(delim);
	while (end != std::string::npos)
	{
		result += "/" + guideTopicId.substr(start, end - start);
		start = end + delim.length();
		end = guideTopicId.find(delim, start);
	}
	if (start < guideTopicId.length())
	{
		result += "/" + guideTopicId.substr(start);
	}
	if (appendJSONFileExtension)
	{
		result += ".json";
	}
	return result;
}

std::shared_ptr<WzGuideTopic> loadWZGuideTopicByTopicId(const std::string& guideTopicId)
{
	// Convert the guide topic id to an expected virtual filesystem path
	// Example: "wz2100::hq" should map to "guidetopics/wz2100/hq.json"
	auto filePath = convertGuideTopicIdToExpectedJSONFilePath(guideTopicId);
	if (!filePath.has_value())
	{
		return nullptr;
	}
	WzPathInfo pathInfo = WzPathInfo::fromPlatformIndependentPath(filePath.value());

	auto jsonObj = wzLoadJsonObjectFromFile(filePath.value());
	if (!jsonObj.has_value())
	{
		// try to load "_index.json" for this topic id (in case it's a category topic)
		filePath = convertGuideTopicIdToExpectedJSONFilePath(guideTopicId + "::_index");
		jsonObj = wzLoadJsonObjectFromFile(filePath.value());
		if (!jsonObj.has_value())
		{
			return nullptr;
		}
	}

	// from_json
	WzGuideTopic result;
	try {
		result = loadWZGuideTopicFromJSON(guideTopicId, jsonObj.value(), pathInfo.path());
	}
	catch (const std::exception &e) {
		// Failed to parse the JSON
		debug(LOG_ERROR, "JSON document from %s is invalid: %s", filePath.value().c_str(), e.what());
		return nullptr;
	}
	return std::make_shared<WzGuideTopic>(std::move(result));
}

// MARK: - Private API

std::shared_ptr<WzGameGuideScreen> createGuideScreen(std::function<void ()> onCloseFunc, bool expandedSidebarByDefault = false)
{
	auto screen = WzGameGuideScreen::make(onCloseFunc, expandedSidebarByDefault);
	ASSERT_OR_RETURN(nullptr, screen != nullptr, "Failed to create screen");
	widgRegisterOverlayScreenOnTopOfScreen(screen, psWScreen);
	psCurrentGameGuideScreen = screen;
	return screen;
}

std::shared_ptr<WzGameGuideScreen> createOrGetGuideScreen(std::function<void ()> onCloseFunc, bool expandedSidebarByDefault = false)
{
	if (!psCurrentGameGuideScreen)
	{
		psCurrentGameGuideScreen = createGuideScreen(onCloseFunc, expandedSidebarByDefault);
		ASSERT_OR_RETURN(nullptr, psCurrentGameGuideScreen != nullptr, "Failed to create game guide screen?");
	}
	else
	{
		if (!isRegisteredOverlayScreen(psCurrentGameGuideScreen))
		{
			widgRegisterOverlayScreenOnTopOfScreen(psCurrentGameGuideScreen, psWScreen);
			// manually trigger a screenSizeDidChange event to ensure the existing screen's form resizes if needed
			psCurrentGameGuideScreen->screenSizeDidChange(screenWidth - 1, screenHeight - 1, screenWidth, screenHeight);
		}
		psCurrentGameGuideScreen->reopeningScreen(onCloseFunc, expandedSidebarByDefault);
	}
	return psCurrentGameGuideScreen;
}

bool showGuideTopicImmediately(const std::string& guideTopicIdentifier, WzGuideForm::OnTopicCloseFunc onTopicClose = nullptr)
{
	auto strongGameGuideScreen = createOrGetGuideScreen([](){
		// no-op on screen close function
	});
	ASSERT_OR_RETURN(false, strongGameGuideScreen != nullptr, "Failed to open game guide screen?");
	auto psGuideForm = strongGameGuideScreen->getGuideForm();
	ASSERT_OR_RETURN(false, psGuideForm != nullptr, "No guide form?");
	psGuideForm->setSidebarVisible(false);
	psGuideForm->showGuideTopic(guideTopicIdentifier, onTopicClose);
	return true;
}

bool handleShowFlags(const std::shared_ptr<WzWrappedGuideTopic>& addedTopic, ShowTopicFlags showFlags, bool newlyAddedTopic, WzGuideForm::OnTopicCloseFunc onTopicClose)
{
	if (showFlags == ShowTopicFlags::None)
	{
		return true;
	}

	bool shouldShow = true;
	if (showFlags & ShowTopicFlags::FirstAdd)
	{
		if (!newlyAddedTopic)
		{
			shouldShow = false;
		}
	}
	if (showFlags & ShowTopicFlags::NeverViewed)
	{
		auto prefs = guideTopicsRegistry->getTopicPrefs(addedTopic);
		if (prefs.has_value() && prefs.value().lastReadVersion == addedTopic->topic->version)
		{
			shouldShow = false;
		}
	}

	if (shouldShow)
	{
		if (!disableTopicPopups)
		{
			showGuideTopicImmediately(addedTopic->topic->identifier, onTopicClose);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool addGuideTopic(const std::shared_ptr<WzGuideTopic>& topic, ShowTopicFlags showFlags, WzGuideForm::OnTopicCloseFunc onTopicClose = nullptr)
{
	auto newlyAddedWrappedTopic = guideTopicsRegistry->addTopic(topic);

	if (newlyAddedWrappedTopic && psCurrentGameGuideScreen)
	{
		psCurrentGameGuideScreen->getGuideForm()->queueRefreshTopicsList();
	}

	if (showFlags != ShowTopicFlags::None)
	{
		if (newlyAddedWrappedTopic)
		{
			handleShowFlags(newlyAddedWrappedTopic, showFlags, true, onTopicClose);
		}
		else
		{
			handleShowFlags(guideTopicsRegistry->getTopic(topic->identifier), showFlags, false, onTopicClose);
		}
	}

	return newlyAddedWrappedTopic != nullptr;
}

std::string guideTopicFilePathToTopicId(const WzPathInfo& pathInfo)
{
	auto components = pathInfo.pathComponents();
	ASSERT_OR_RETURN({}, !components.empty(), "Empty path?");
	std::string result;
	size_t idx = 0;
	if (components[0] == "guidetopics")
	{
		++idx;
	}
	for (; idx < components.size(); ++idx)
	{
		if (!result.empty())
		{
			result += "::";
		}
		result += components[idx];
	}
	if (strEndsWith(result, ".json"))
	{
		result = result.substr(0, result.size() - 5);
	}
	return result;
}

static bool loadWildcardTopicIds(const std::string& guideTopicId, ShowTopicFlags showFlags, WzGuideForm::OnTopicCloseFunc onTopicClose, const std::unordered_set<std::string>& excludedTopicIDs)
{
	if (guideTopicId.empty() || guideTopicId.back() != '*')
	{
		return false;
	}

	size_t numAsterisks = 1;
	if (guideTopicId.size() >= 2 && guideTopicId[guideTopicId.size() - 2] == '*')
	{
		numAsterisks = 2;
	}
	bool recurse = numAsterisks > 1;

	// otherwise, see if this is a proper wildcard guideTopicId, and try to enumerate
	// this currently only supports the asterisk in the last position, as in: "wz2100:*"
	auto guideTopicBaseId = guideTopicId.substr(0, guideTopicId.size() - numAsterisks);
	ASSERT_OR_RETURN(false, guideTopicBaseId.size() > 2 && strEndsWith(guideTopicBaseId, "::"), "Unexpected wildcard guide topic id: %s", guideTopicId.c_str());
	auto guideTopicBasePath = convertGuideTopicIdToExpectedJSONFilePath(guideTopicBaseId, false);
	if (!guideTopicBasePath.has_value())
	{
		return false;
	}
	std::string baseDir = guideTopicBasePath.value();
	if (!strEndsWith(baseDir, "/"))
	{
		baseDir += "/";
	}
	std::string fullPath;
	std::string expectedGuideTopicId;
	std::string jsonExtension = ".json";
	std::vector<std::shared_ptr<WzWrappedGuideTopic>> newlyAddedWrappedTopics;
	WZ_PHYSFS_enumerateFilesEx(baseDir.c_str(), [&baseDir, &fullPath, &expectedGuideTopicId, &jsonExtension, &newlyAddedWrappedTopics, &excludedTopicIDs](const char *i) -> bool {
		if (!i) { return true; }
		fullPath = baseDir + std::string(i);
		// check if ends with .json
		if (strEndsWith(fullPath, jsonExtension))
		{
			WzPathInfo pathInfo = WzPathInfo::fromPlatformIndependentPath(fullPath);
			// ignore category "_index.json"
			if (pathInfo.fileName().compare("_index.json") == 0)
			{
				return true;
			}
			expectedGuideTopicId = guideTopicFilePathToTopicId(pathInfo);
			if (excludedTopicIDs.count(expectedGuideTopicId) > 0)
			{
				// skip excluded guide topic
				return true;
			}
			// try to load it
			auto jsonObj = wzLoadJsonObjectFromFile(fullPath);
			if (!jsonObj.has_value())
			{
				return true;
			}
			// from_json
			WzGuideTopic loadedTopic;
			try {
				loadedTopic = loadWZGuideTopicFromJSON(expectedGuideTopicId, jsonObj.value(), pathInfo.path());
			}
			catch (const std::exception &e) {
				// Failed to parse the JSON
				debug(LOG_ERROR, "JSON document from %s is invalid: %s", fullPath.c_str(), e.what());
				return true;
			}
			// add to guide registry
			auto psNewGuideTopic = std::make_shared<WzGuideTopic>(std::move(loadedTopic));
			auto newlyAddedWrappedTopic = guideTopicsRegistry->addTopic(psNewGuideTopic);
			if (newlyAddedWrappedTopic != nullptr)
			{
				if (psCurrentGameGuideScreen)
				{
					psCurrentGameGuideScreen->getGuideForm()->queueRefreshTopicsList();
				}
				newlyAddedWrappedTopics.push_back(newlyAddedWrappedTopic);
			}
		}
		return true; // continue
	}, recurse);

	if (showFlags != ShowTopicFlags::None && !newlyAddedWrappedTopics.empty() && !disableTopicPopups)
	{
		// Just highlight the first new entry added (that matches the showFlags), and show it
		for (const auto& topic : newlyAddedWrappedTopics)
		{
			if (handleShowFlags(topic, showFlags, true, onTopicClose))
			{
				break;
			}
		}
	}
	return true;
}

bool addGuideTopicImpl(const std::string& guideTopicId, ShowTopicFlags showFlags, WzGuideForm::OnTopicCloseFunc onTopicClose, const std::unordered_set<std::string>& excludedTopicIDs)
{
	auto psExistingAddedTopic = guideTopicsRegistry->getTopic(guideTopicId);
	if (psExistingAddedTopic != nullptr)
	{
		debug(LOG_WZ, "Guide topic with identifier \"%s\" was already added", guideTopicId.c_str());
		handleShowFlags(psExistingAddedTopic, showFlags, false, onTopicClose);
		return false;
	}
	if (loadWildcardTopicIds(guideTopicId, showFlags, onTopicClose, excludedTopicIDs))
	{
		// already handled
		return true;
	}
	ASSERT(excludedTopicIDs.empty(), "excludedTopicIDs provided, but did not specify a wildcard topic ID to add");
	auto loadedTopic = loadWZGuideTopicByTopicId(guideTopicId);
	if (!loadedTopic)
	{
		debug(LOG_ERROR, "Failed to open / load guide topic: %s", guideTopicId.c_str());
		return false;
	}
	return addGuideTopic(loadedTopic, showFlags, onTopicClose);
}

// MARK: - Public API

bool hasGameGuideTopics()
{
	return guideTopicsRegistry->hasTopics();
}

bool showGuideScreen(std::function<void ()> onCloseFunc, bool expandedSidebarByDefault)
{
	auto screen = createOrGetGuideScreen(onCloseFunc, expandedSidebarByDefault);
	screen->getGuideForm()->showFirstGuideTopicIfNeeded();
	return screen != nullptr;
}

void closeGuideScreen()
{
	if (psCurrentGameGuideScreen)
	{
		psCurrentGameGuideScreen->closeScreen(true);
	}
}

void addGuideTopic(const std::string& guideTopicId, ShowTopicFlags showFlags, const std::unordered_set<std::string>& excludedTopicIDs)
{
	addGuideTopicImpl(guideTopicId, showFlags, nullptr, excludedTopicIDs);
}

void shutdownGameGuide()
{
	guideTopicsRegistry->clear();
	closeGuideScreen();
	psCurrentGameGuideScreen.reset();
	disableTopicPopups = false;
}

std::vector<std::string> saveLoadedGuideTopics()
{
	return guideTopicsRegistry->saveLoadedTopics();
}

bool restoreLoadedGuideTopics(const std::vector<std::string>& topicIds)
{
	return guideTopicsRegistry->restoreLoadedTopics(topicIds);
}

bool getGameGuideDisableTopicPopups()
{
	return disableTopicPopups;
}

void setGameGuideDisableTopicPopups(bool disablePopups)
{
	disableTopicPopups = disablePopups;
}
