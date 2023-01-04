/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/*
 * keyedit.c
 * keymap editor
 * alexl.
 */

// ////////////////////////////////////////////////////////////////////////////
// includes
#include <string.h>
#include <physfs.h>
#include <unordered_map>
#include <nonstd/optional.hpp>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/scrollablelist.h"

#include "input/mapping.h"
#include "input/manager.h"
#include "frend.h"
#include "frontend.h"
#include "hci.h"
#include "init.h"
#include "intdisplay.h"
#include "keybind.h"
#include "keyedit.h"
#include "loadsave.h"
#include "main.h"
#include "multiint.h"
#include "multiplay.h"
#include "ingameop.h"

static unsigned int getMaxKeyMapNameWidth(InputManager& inputManager);

// ////////////////////////////////////////////////////////////////////////////
// defines

#define	KM_START			10204
#define	KM_END				10399

#define KM_W				FRONTEND_BOTFORMW
#define KM_H				440
#define KM_X				FRONTEND_BOTFORMX
#define KM_Y				20
#define KM_SX				FRONTEND_SIDEX


#define KM_ENTRYW			(FRONTEND_BOTFORMW - 80)
#define KM_ENTRYH			(16)

struct DisplayKeyMapData {
	explicit DisplayKeyMapData(InputManager& inputManager, const KeyFunctionInfo& info)
		: inputManager(inputManager)
		, mappings(std::vector<nonstd::optional<std::reference_wrapper<KeyMapping>>>(static_cast<unsigned int>(KeyMappingSlot::LAST), nonstd::nullopt))
		, info(info)
	{
	}

	InputManager& inputManager;
	std::vector<nonstd::optional<std::reference_wrapper<KeyMapping>>> mappings;
	const KeyFunctionInfo& info;
};

class KeyMapForm : public IntFormAnimated
{
protected:
	KeyMapForm(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig)
		: IntFormAnimated(false)
		, inputManager(inputManager)
		, keyFuncConfig(keyFuncConfig)
	{
	}

	void initialize(bool isInGame);

public:
	static std::shared_ptr<KeyMapForm> make(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig, const bool isInGame)
	{
		class make_shared_enabler: public KeyMapForm
		{
		public:
			make_shared_enabler(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig)
				: KeyMapForm(inputManager, keyFuncConfig)
			{
			}
		};
		auto widget = std::make_shared<make_shared_enabler>(inputManager, keyFuncConfig);
		widget->initialize(isInGame);
		return widget;
	}

	void checkPushedKeyCombo();
	bool pushedKeyCombo(const KeyMappingInput input);

private:
	friend class KeyMapButton;

	InputManager& inputManager;
	const KeyFunctionConfiguration& keyFuncConfig;
	std::shared_ptr<ScrollableListWidget> keyMapList;
	std::unordered_map<std::string, DisplayKeyMapData*> displayDataPerInfo;

	std::shared_ptr<W_BUTTON> createKeyMapButton(const unsigned int id, const KeyMappingSlot slot, struct DisplayKeyMapData& targetFunctionData);
	void unhighlightSelected();
	void addButton(int buttonId, int y, const char *text);
};


// ////////////////////////////////////////////////////////////////////////////
// variables

struct KeyMappingSelection
{
	bool                   hasActiveSelection;
	KeyMappingSlot         slot;
	const KeyFunctionInfo* info;

	bool isSelected(const KeyFunctionInfo& otherInfo, const KeyMappingSlot otherSlot) const
	{
		return hasActiveSelection && info == &otherInfo && slot == otherSlot;
	}

	void select(const KeyFunctionInfo& newInfo, const KeyMappingSlot newSlot)
	{
		hasActiveSelection = true;
		slot = newSlot;
		info = &newInfo;
	}

	void clearSelection()
	{
		hasActiveSelection = false;
		info = nullptr;
		slot = KeyMappingSlot::LAST;
	}
};

static std::string getNotBoundLabel()
{
	return _("<not bound>");
}

static std::string getFixedKeyLabel()
{
	return _("<Fixed Key>");
}

static KeyMappingSelection keyMapSelection;
static bool maxKeyMapNameWidthDirty = true;


// ////////////////////////////////////////////////////////////////////////////
// Widgets

class KeyMapButton : public W_BUTTON
{
protected:
	KeyMapButton(
		InputManager& inputManager,
		KeyMappingSlot slot,
		DisplayKeyMapData& targetFunctionData,
		std::weak_ptr<KeyMapForm> parentForm
	)
		: inputManager(inputManager)
		, slot(slot)
		, targetFunctionData(targetFunctionData)
		, parentForm(parentForm)
	{
	}

public:
	static std::shared_ptr<KeyMapButton> make(
		InputManager& inputManager,
		KeyMappingSlot slot,
		DisplayKeyMapData& targetFunctionData,
		std::weak_ptr<KeyMapForm> parentForm
	) {
		class make_shared_enabler : public KeyMapButton
		{
		public:
			make_shared_enabler(
				InputManager& inputManager,
				KeyMappingSlot slot,
				DisplayKeyMapData& targetFunctionData,
				std::weak_ptr<KeyMapForm> parentForm
			)
				: KeyMapButton(inputManager, slot, targetFunctionData, parentForm)
			{
			}
		};
		return std::make_shared<make_shared_enabler>(inputManager, slot, targetFunctionData, parentForm);
	}

public:
	virtual void clicked(W_CONTEXT* psContext, WIDGET_KEY key) override
	{
		const auto psParentForm = parentForm.lock();
		ASSERT_OR_RETURN(, psParentForm != nullptr, "Cannot handle KeyMapButton::clicked: parent form was nullptr!");

		const KeyFunctionInfo& info = targetFunctionData.info;
		if (info.type != KeyMappingType::ASSIGNABLE)
		{
			audio_PlayBuildFailedOnce();
			psParentForm->unhighlightSelected();
			return;
		}

		if (keyMapSelection.isSelected(info, slot))
		{
			psParentForm->unhighlightSelected();
			return;
		}

		psParentForm->keyMapList->disableScroll();
		keyMapSelection.select(targetFunctionData.info, slot);
	}

	virtual void display(int xOffset, int yOffset) override
	{
		const std::shared_ptr<WIDGET> pParent = this->parent();
		ASSERT_OR_RETURN(, pParent != nullptr, "Keymap buttons should have a parent container!");

		// Update layout
		const int numSlots = static_cast<int>(KeyMappingSlot::LAST);
		const int buttonHeight = (pParent->height() / numSlots);
		const int layoutYOffset = buttonHeight * static_cast<int>(slot);
		const int buttonWidth = getMaxKeyMapNameWidth(inputManager);
		setGeometry(
			pParent->width() - buttonWidth,
			layoutYOffset,
			buttonWidth,
			buttonHeight
		);

		int xPos = xOffset + x();
		int yPos = yOffset + y();
		int h = height();
		int w = width();

		const KeyFunctionInfo& info = targetFunctionData.info;

		// Draw base
		if (keyMapSelection.isSelected(info, slot))
		{
			pie_BoxFill(xPos, yPos, xPos + w, yPos + h, WZCOL_KEYMAP_ACTIVE);
		}
		else if (info.type != KeyMappingType::ASSIGNABLE)
		{
			pie_BoxFill(xPos, yPos, xPos + w, yPos + h, WZCOL_MENU_BORDER);
			pie_BoxFill(xPos + 1, yPos + 1, xPos + w - 1, yPos + h - 1, WZCOL_KEYMAP_FIXED);
		}
		else
		{
			drawBlueBoxInset(xPos, yPos, w, h);
		}

		// Select label text and color
		PIELIGHT bindingTextColor = WZCOL_FORM_TEXT;
		char sPrimaryKey[MAX_STR_LENGTH];
		sPrimaryKey[0] = '\0';
		const nonstd::optional<KeyMapping> mapping = targetFunctionData.mappings[static_cast<unsigned int>(slot)];
		if (mapping && !mapping->keys.input.isCleared())
		{
			// Check to see if key is on the numpad, if so tell user and change color
			const bool isBoundToNumpad = mapping->keys.input.source == KeyMappingInputSource::KEY_CODE && mapping->keys.input.value.keyCode >= KEY_KP_0 && mapping->keys.input.value.keyCode <= KEY_KPENTER;
			if (isBoundToNumpad)
			{
				bindingTextColor = WZCOL_YELLOW;
			}
			mapping->toString(sPrimaryKey);
		}
		else
		{
			sstrcpy(sPrimaryKey, info.type == KeyMappingType::ASSIGNABLE ? getNotBoundLabel().c_str() : getFixedKeyLabel().c_str());
		}

		wzBindingText.setText(sPrimaryKey, iV_fonts::font_regular);
		wzBindingText.render(xPos, yPos + (h / 2) + 3, bindingTextColor);
	}

private:
	InputManager& inputManager;

	WzText wzBindingText;
	KeyMappingSlot slot;
	DisplayKeyMapData& targetFunctionData;

	std::weak_ptr<KeyMapForm> parentForm;
};

class KeyMapLabel : public WIDGET
{
protected:
	KeyMapLabel(InputManager& inputManager, DisplayKeyMapData& targetFunctionData)
		: inputManager(inputManager)
		, targetFunctionData(targetFunctionData)
	{
	}

public:
	static std::shared_ptr<KeyMapLabel> make(InputManager& inputManager, DisplayKeyMapData& targetFunctionData) {
		class make_shared_enabler : public KeyMapLabel
		{
		public:
			make_shared_enabler(InputManager& inputManager, DisplayKeyMapData& targetFunctionData)
				: KeyMapLabel(inputManager, targetFunctionData)
			{
			}
		};
		return std::make_shared<make_shared_enabler>(inputManager, targetFunctionData);
	}

public:
	virtual void display(int xOffset, int yOffset) override
	{
		const std::shared_ptr<WIDGET> pParent = parent();
		ASSERT_OR_RETURN(, pParent != nullptr, "Keymap labels should have a parent container!");

		// Update layout
		const int buttonWidth = getMaxKeyMapNameWidth(inputManager);
		setGeometry(
			0,
			0,
			pParent->width() - buttonWidth,
			pParent->height()
		);

		const int xPos = xOffset + x();
		const int yPos = yOffset + y();
		const int w = width();
		const int h = height();
		drawBlueBoxInset(xPos, yPos, w, h);
		wzNameText.setText(_(targetFunctionData.info.displayName.c_str()), iV_fonts::font_regular);
		wzNameText.render(xPos + 2, yPos + (h / 2) + 3, WZCOL_FORM_TEXT);
	}

private:
	InputManager& inputManager;

	WzText wzNameText;

	DisplayKeyMapData& targetFunctionData;
};



// ////////////////////////////////////////////////////////////////////////////
// funcs
// ////////////////////////////////////////////////////////////////////////////
static nonstd::optional<KEY_CODE> scanKeyBoardForPressedBindableKey()
{
	for (unsigned int i = 0; i < KEY_MAXSCAN; i++)
	{
		if (keyPressed((KEY_CODE)i))
		{
			if (i !=	KEY_RALT			// exceptions
			    && i !=	KEY_LALT
			    && i != KEY_RCTRL
			    && i != KEY_LCTRL
			    && i != KEY_RSHIFT
			    && i != KEY_LSHIFT
			    && i != KEY_LMETA
			    && i != KEY_RMETA
			   )
			{
				return (KEY_CODE)i;             // top row key pressed
			}
		}
	}
	return nonstd::nullopt;
}

static nonstd::optional<MOUSE_KEY_CODE> scanMouseForPressedBindableKey()
{
	for (unsigned int i = 0; i < MOUSE_KEY_CODE::MOUSE_END; i++)
	{
		const MOUSE_KEY_CODE mouseKeyCode = (MOUSE_KEY_CODE)i;
		if (mousePressed(mouseKeyCode))
		{
			if (   mouseKeyCode != MOUSE_KEY_CODE::MOUSE_LMB // exceptions
				&& mouseKeyCode != MOUSE_KEY_CODE::MOUSE_RMB
				&& mouseKeyCode != MOUSE_KEY_CODE::MOUSE_MMB)
			{
				return mouseKeyCode; // Bindable mouse key pressed
			}
		}
	}
	return nonstd::nullopt;
}

bool runInGameKeyMapEditor(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig, unsigned id)
{
	if (id == KM_RETURN || id == KM_GO_BACK)			// return
	{
		inputManager.cmappings().save(KeyMapPath);
		widgDelete(psWScreen, KM_FORM);
		inputLoseFocus();
		bAllowOtherKeyPresses = true;
		if (id == KM_GO_BACK)
		{
			intReopenMenuWithoutUnPausing();
			return false;
		}
		return true;
	}
	if (id == KM_DEFAULT)
	{
		// reinitialise key mappings
		inputManager.resetMappings(true, keyFuncConfig);
		widgDelete(psWScreen, KM_FORM); // readd the widgets
		maxKeyMapNameWidthDirty = true;
		startInGameKeyMapEditor(inputManager, keyFuncConfig, false);
	}

	if (auto kmForm = (KeyMapForm *)widgGetFromID(psWScreen, KM_FORM))
	{
		kmForm->checkPushedKeyCombo();
	}
	return false;
}

// ////////////////////////////////////////////////////////////////////////////
bool runKeyMapEditor(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig)
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	if (id == KM_RETURN)			// return
	{
		inputManager.cmappings().save(KeyMapPath);
		changeTitleMode(OPTIONS);
	}
	if (id == KM_DEFAULT)
	{
		// reinitialise key mappings
		inputManager.resetMappings(true, keyFuncConfig);
		widgDelete(psWScreen, FRONTEND_BACKDROP); // readd the widgets
		startKeyMapEditor(inputManager, keyFuncConfig, false);
	}

	if (auto kmForm = (KeyMapForm *)widgGetFromID(psWScreen, KM_FORM))
	{
		kmForm->checkPushedKeyCombo();
	}

	widgDisplayScreen(psWScreen);				// show the widgets currently running

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
KeyFunctionEntries getVisibleKeyFunctionEntries(const KeyFunctionConfiguration& keyFuncConfig)
{
	KeyFunctionEntries visible;
	for (const KeyFunctionInfo& info : keyFuncConfig.allKeyFunctionEntries())
	{
		if (info.type != KeyMappingType::HIDDEN)
		{
			visible.push_back(info);
		}
	}

	return visible;
}

static std::vector<std::reference_wrapper<const KeyMapping>> getVisibleMappings(const InputManager& inputManager)
{
	std::vector<std::reference_wrapper<const KeyMapping>> visibleMappings;
	for (const KeyMapping& mapping : inputManager.cmappings())
	{
		if (mapping.info.type != KeyMappingType::HIDDEN)
		{
			visibleMappings.push_back(mapping);
		}
	}

	return visibleMappings;
}

static unsigned int getMaxKeyMapNameWidth(InputManager& inputManager)
{
	static unsigned int max = 0;

	if (maxKeyMapNameWidthDirty) {
		max = static_cast<int>(iV_GetTextWidth(getNotBoundLabel().c_str(), iV_fonts::font_regular));

		char sKey[MAX_STR_LENGTH];
		for (const KeyMapping& mapping : getVisibleMappings(inputManager)) {
			mapping.toString(sKey);
			max = MAX(max, static_cast<int>(iV_GetTextWidth(sKey, iV_fonts::font_regular)));
		}

		maxKeyMapNameWidthDirty = false;
	}

	return max;
}

// ////////////////////////////////////////////////////////////////////////////
static bool keyMapEditor(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig, const bool first, WIDGET* parent, const bool isInGame)
{
	if (first)
	{
		inputManager.mappings().load(KeyMapPath, keyFuncConfig);
	}

	parent->attach(KeyMapForm::make(inputManager, keyFuncConfig, isInGame));
	return true;
}

bool startInGameKeyMapEditor(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig, bool first)
{
	bAllowOtherKeyPresses = false;
	return keyMapEditor(inputManager, keyFuncConfig, first, psWScreen->psForm.get(), true);
}

bool startKeyMapEditor(InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig, bool first)
{
	addBackdrop();
	addSideText(FRONTEND_SIDETEXT, KM_SX, KM_Y, _("KEY MAPPING"));
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	return keyMapEditor(inputManager, keyFuncConfig, first, parent, false);
}

// ////////////////////////////////////////////////////////////////////////////

std::shared_ptr<W_BUTTON> KeyMapForm::createKeyMapButton(const unsigned int buttonId, const KeyMappingSlot slot, DisplayKeyMapData& targetFunctionData)
{
	auto button = KeyMapButton::make(inputManager, slot, targetFunctionData, std::static_pointer_cast<KeyMapForm>(shared_from_this()));
	button->setGeometry(0, 0, KM_ENTRYW / 3, KM_ENTRYH); // Initially set to occupy 1/3 of the width. Display func will determine and update the actual size
	button->id = buttonId;
	return button;
}

void KeyMapForm::initialize(bool isInGame)
{
	id = KM_FORM;

	attach(keyMapList = ScrollableListWidget::make());
	if (!isInGame)
	{
		setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(KM_X, KM_Y, KM_W, KM_H);
		}));
		keyMapList->setGeometry(52, 10, KM_ENTRYW, 26 * KM_ENTRYH);

		addMultiBut(*this, KM_RETURN,			// return button.
				8, 5,
				iV_GetImageWidth(FrontImages, IMAGE_RETURN),
				iV_GetImageHeight(FrontImages, IMAGE_RETURN),
				_("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

		addMultiBut(*this, KM_DEFAULT,
				11, 45,
				iV_GetImageWidth(FrontImages, IMAGE_KEYMAP_DEFAULT),
				iV_GetImageHeight(FrontImages, IMAGE_KEYMAP_DEFAULT),
				_("Select Default"),
				IMAGE_KEYMAP_DEFAULT, IMAGE_KEYMAP_DEFAULT_HI, IMAGE_KEYMAP_DEFAULT_HI);	// default.
	}
	else
	{
		// Text versions for in-game where image resources are not available
		setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(((300-(KM_W/2))+D_W), ((240-(KM_H/2))+D_H), KM_W, KM_H + 10);
		}));
		keyMapList->setGeometry(52, 10, KM_ENTRYW, 24 * KM_ENTRYH);

		addButton(KM_GO_BACK, KM_H - 40, _("Go Back"));

		addButton(KM_RETURN, KM_H - 24, _("Resume Game"));

		if (!bMultiPlayer || !NetPlay.bComms) // no editing in true multiplayer
		{
			addButton(KM_DEFAULT, KM_H - 8, _("Select Default"));
		}
	}

	auto infos = getVisibleKeyFunctionEntries(keyFuncConfig);
	std::sort(infos.begin(), infos.end(), [&](const KeyFunctionInfo& a, const KeyFunctionInfo& b) {
		const bool bContextsAreSame = a.context == b.context;
		const InputContext& contextA = inputManager.contexts().get(a.context);
		const InputContext& contextB = inputManager.contexts().get(b.context);
		return bContextsAreSame
			? a.displayName < b.displayName
			: contextA.getDisplayName() < contextB.getDisplayName();
	});

	/* Add key mappings to the form */
	displayDataPerInfo.clear();
	for (KeyFunctionEntries::const_iterator i = infos.begin(); i != infos.end(); ++i)
	{
		const KeyFunctionInfo& info = *i;

		/* Add separator if changing categories */
		const bool bShouldAddSeparator = i == infos.begin() || std::prev(i)->get().context != info.context;
		if (bShouldAddSeparator)
		{
			auto separator = std::make_shared<W_LABEL>();
			separator->setGeometry(0, 0, KM_ENTRYW, KM_ENTRYH * 2);
			separator->setTextAlignment(WzTextAlignment::WLAB_ALIGNBOTTOMLEFT);
			separator->setFormattedString(_(inputManager.contexts().get(info.context).getDisplayName().c_str()), KM_ENTRYW, iV_fonts::font_large);
			keyMapList->addItem(separator);
		}

		DisplayKeyMapData* data = new DisplayKeyMapData(inputManager, info);
		displayDataPerInfo.insert({ info.name, data });

		const unsigned int numSlots = static_cast<unsigned int>(KeyMappingSlot::LAST);

		const unsigned int index = i - infos.begin();
		const unsigned int containerId = KM_START + index * (numSlots + 2);
		const unsigned int labelId = KM_START + index * (numSlots + 2) + 1;

		auto label = KeyMapLabel::make(inputManager, *data);
		label->setGeometry(0, 0, KM_ENTRYW / 3, KM_ENTRYH);
		label->id = labelId;

		auto container = std::make_shared<WIDGET>();
		container->setGeometry(0, 0, KM_ENTRYW, KM_ENTRYH * numSlots);
		container->id = containerId;
		container->attach(label);

		for (unsigned int slotIndex = 0; slotIndex < numSlots; ++slotIndex)
		{
			const auto slot = static_cast<KeyMappingSlot>(slotIndex);
			const auto buttonId = KM_START + index * (numSlots + 2) + 2 + slotIndex;
			const auto button = createKeyMapButton(buttonId, slot, *data);
			container->attach(button);

			data->mappings[slotIndex] = inputManager.mappings().get(info, slot);
		}

		keyMapList->addItem(container);
	}
}

void KeyMapForm::addButton(int buttonId, int y, const char *text)
{
	W_BUTINIT sButInit;

	sButInit.formID		= KM_FORM;
	sButInit.style		= WBUT_PLAIN | WBUT_TXTCENTRE;
	sButInit.width		= KM_W;
	sButInit.FontID		= font_regular;
	sButInit.x			= 0;
	sButInit.height		= 10;
	sButInit.pDisplay	= displayTextOption;
	sButInit.initPUserDataFunc = []() -> void * { return new DisplayTextOptionCache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	sButInit.id			= buttonId;
	sButInit.y			= y;
	sButInit.pText		= text;

	attach(std::make_shared<W_BUTTON>(&sButInit));
}

void KeyMapForm::checkPushedKeyCombo()
{
	if (keyMapSelection.hasActiveSelection)
	{
		if (const nonstd::optional<KEY_CODE> kc = scanKeyBoardForPressedBindableKey())
		{
			if (kc.value() == KEY_ESC)
			{
				// Special-handling for ESC - unhighlight the selection
				inputLoseFocus();	// clear the input buffer.
				unhighlightSelected();
				return;
			}
			pushedKeyCombo(*kc);
		}

		if (const nonstd::optional<MOUSE_KEY_CODE> mkc = scanMouseForPressedBindableKey())
		{
			pushedKeyCombo(*mkc);
		}
	}
}

bool KeyMapForm::pushedKeyCombo(const KeyMappingInput input)
{
	KEY_CODE metakey = KEY_IGNORE;

	if (keyDown(KEY_RALT) || keyDown(KEY_LALT))
	{
		metakey = KEY_LALT;
	}
	else if (keyDown(KEY_RCTRL) || keyDown(KEY_LCTRL))
	{
		metakey = KEY_LCTRL;
	}
	else if (keyDown(KEY_RSHIFT) || keyDown(KEY_LSHIFT))
	{
		metakey = KEY_LSHIFT;
	}
	else if (keyDown(KEY_RMETA) || keyDown(KEY_LMETA))
	{
		metakey = KEY_LMETA;
	}

	const auto selectedInfo = keyMapSelection.info;
	/* Disallow modifying non-assignable mappings. (Null-check the `info` in case assertions are disabled) */
	if (!selectedInfo || selectedInfo->type != KeyMappingType::ASSIGNABLE)
	{
		unhighlightSelected();
		return false;
	}

	/* Disallow conflicts with ALWAYS_ACTIVE keybinds, as that context always has max priority, preventing
	   any conflicting keys from triggering. */
	for (const KeyMapping& mapping : inputManager.mappings().findConflicting(metakey, input, selectedInfo->context, inputManager.contexts()))
	{
		const InputContext context = inputManager.contexts().get(mapping.info.context);
		if (context.isAlwaysActive())
		{
			audio_PlayBuildFailedOnce();
			unhighlightSelected();
			return false;
		}
	}

	/* Clear conflicting mappings using these keys */
	const auto conflicts = inputManager.mappings().removeConflicting(metakey, input, selectedInfo->context, inputManager.contexts());
	for (auto& conflict : conflicts)
	{
		// Update conflicting mappings' display data
		if (auto conflictData = displayDataPerInfo[conflict.info.name])
		{
			const unsigned int slotIndex = static_cast<unsigned int>(conflict.slot);
			conflictData->mappings[slotIndex] = nonstd::nullopt;
		}
	}

	/* Try and see if the mapping already exists. Remove the old mapping if one does exist */
	const auto maybeOld = inputManager.mappings().get(*selectedInfo, keyMapSelection.slot);
	if (maybeOld.has_value())
	{
		inputManager.mappings().remove(*maybeOld);
	}

	/* Figure out which `KeyAction` the mapping should use */
	const auto foundPrimary = std::find_if(selectedInfo->defaultMappings.cbegin(), selectedInfo->defaultMappings.cend(), [](const std::pair<KeyMappingSlot, KeyCombination>& defaultMapping) {
		return defaultMapping.first == KeyMappingSlot::PRIMARY;
	});

	const auto foundMatch = std::find_if(selectedInfo->defaultMappings.cbegin(), selectedInfo->defaultMappings.cend(), [&input](const std::pair<KeyMappingSlot, KeyCombination>& defaultMapping) {
		const KeyCombination combination = defaultMapping.second;
		return combination.input == input;
	});

	KeyAction action = KeyAction::PRESSED;
	if (foundMatch != selectedInfo->defaultMappings.cend())
	{
		action = foundMatch->second.action;
	}
	else if (foundPrimary != selectedInfo->defaultMappings.cend())
	{
		action = foundPrimary->second.action;
	}


	/* Finally, create the new mapping */
	KeyMapping& newMapping = inputManager.mappings().add({ metakey, input, action }, *selectedInfo, keyMapSelection.slot);

	// Update display data for the new mapping
	if (auto displayData = displayDataPerInfo[selectedInfo->name])
	{
		const unsigned int slotIndex = static_cast<unsigned int>(keyMapSelection.slot);
		displayData->mappings[slotIndex] = newMapping;
	}
	maxKeyMapNameWidthDirty = true;
	unhighlightSelected();
	return true;
}

void KeyMapForm::unhighlightSelected()
{
	keyMapList->enableScroll();
	keyMapSelection.clearSelection();
}
