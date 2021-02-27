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

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/widget/button.h"
#include "lib/widget/scrollablelist.h"

#include "frend.h"
#include "frontend.h"
#include "hci.h"
#include "init.h"
#include "intdisplay.h"
#include "keybind.h"
#include "keyedit.h"
#include "keymap.h"
#include "loadsave.h"
#include "main.h"
#include "multiint.h"
#include "multiplay.h"
#include "ingameop.h"

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

class KeyMapForm : public IntFormAnimated
{
protected:
	KeyMapForm(): IntFormAnimated(false) {}
	void initialize(bool isInGame);

public:
	static std::shared_ptr<KeyMapForm> make(bool isInGame)
	{
		class make_shared_enabler: public KeyMapForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(isInGame);
		return widget;
	}

	void checkPushedKeyCombo();
	bool pushedKeyCombo(KEY_CODE subkey);

private:
	std::shared_ptr<ScrollableListWidget> keyMapList;
	void unhighlightSelected();
	void addButton(int buttonId, int y, const char *text);
};

struct DisplayKeyMapCache {
	WzText wzNameText;
	WzText wzBindingText;
};

struct DisplayKeyMapData {
	explicit DisplayKeyMapData(KEY_MAPPING *psMapping)
	: psMapping(psMapping)
	{ }

	KEY_MAPPING *psMapping;
	DisplayKeyMapCache cache;
};

// ////////////////////////////////////////////////////////////////////////////
// variables

static KEY_MAPPING	*selectedKeyMap;
static bool maxKeyMapNameWidthDirty = true;

// ////////////////////////////////////////////////////////////////////////////
// funcs
// ////////////////////////////////////////////////////////////////////////////
static KEY_CODE scanKeyBoardForBinding()
{
	UDWORD i;
	for (i = 0; i < KEY_MAXSCAN; i++)
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
	return (KEY_CODE)0;
}

bool runInGameKeyMapEditor(unsigned id)
{
	if (id == KM_RETURN || id == KM_GO_BACK)			// return
	{
		saveKeyMap();
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
		keyInitMappings(true);
		widgDelete(psWScreen, KM_FORM); // readd the widgets
		startInGameKeyMapEditor(false);
		maxKeyMapNameWidthDirty = true;
	}

	if (auto kmForm = (KeyMapForm *)widgGetFromID(psWScreen, KM_FORM))
	{
		kmForm->checkPushedKeyCombo();
	}
	return false;
}

// ////////////////////////////////////////////////////////////////////////////
bool runKeyMapEditor()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	if (id == KM_RETURN)			// return
	{
		saveKeyMap();
		changeTitleMode(OPTIONS);
	}
	if (id == KM_DEFAULT)
	{
		// reinitialise key mappings
		keyInitMappings(true);
		widgDelete(psWScreen, FRONTEND_BACKDROP); // readd the widgets
		startKeyMapEditor(false);
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
// returns key to press given a mapping.
static bool keyMapToString(char *pStr, KEY_MAPPING *psMapping)
{
	bool	onlySub = true;
	char	asciiSub[20], asciiMeta[20];

	if (psMapping->metaKeyCode != KEY_IGNORE)
	{
		keyScanToString(psMapping->metaKeyCode, (char *)&asciiMeta, 20);
		onlySub = false;
	}
	keyScanToString(psMapping->subKeyCode, (char *)&asciiSub, 20);

	if (onlySub)
	{
		sprintf(pStr, "%s", asciiSub);
	}
	else
	{
		sprintf(pStr, "%s %s", asciiMeta, asciiSub);
	}
	return true;
}

std::vector<KEY_MAPPING *> getVisibleMappings()
{
	std::vector<KEY_MAPPING *> mappings;
	for (KEY_MAPPING &mapping : keyMappings)
	{
		if (mapping.status != KEYMAP__DEBUG && mapping.status != KEYMAP___HIDE)
		{
			mappings.push_back(&mapping);
		}
	}

	return mappings;
}

static uint16_t getMaxKeyMapNameWidth()
{
	static uint16_t max = 0;

	if (maxKeyMapNameWidthDirty) {
		max = 0;
		WzText displayText;
		char sKey[MAX_STR_LENGTH];

		for (auto mapping: getVisibleMappings()) {
			keyMapToString(sKey, mapping);
			displayText.setText(sKey, font_regular);
			max = MAX(max, displayText.width());
		}

		maxKeyMapNameWidthDirty = false;
	}

	return max;
}

// ////////////////////////////////////////////////////////////////////////////
// display a keymap on the interface.
static void displayKeyMap(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayKeyMap must have its pUserData initialized to a (DisplayKeyMapData*)
	assert(psWidget->pUserData != nullptr);
	DisplayKeyMapData& data = *static_cast<DisplayKeyMapData *>(psWidget->pUserData);

	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();
	KEY_MAPPING *psMapping = data.psMapping;
	char sKey[MAX_STR_LENGTH];

	if (psMapping == selectedKeyMap)
	{
		pie_BoxFill(x, y, x + w, y + h, WZCOL_KEYMAP_ACTIVE);
	}
	else if (psMapping->status == KEYMAP_ALWAYS || psMapping->status == KEYMAP_ALWAYS_PROCESS)
	{
		// when user can't edit something...
		pie_BoxFill(x, y , x + w, y + h, WZCOL_KEYMAP_FIXED);
	}
	else
	{
		drawBlueBoxInset(x, y, w, h);
	}

	// draw name
	data.cache.wzNameText.setText(_(psMapping->name.c_str()), font_regular);
	data.cache.wzNameText.render(x + 2, y + (psWidget->height() / 2) + 3, WZCOL_FORM_TEXT);

	// draw binding
	keyMapToString(sKey, psMapping);
	// Check to see if key is on the numpad, if so tell user and change color
	PIELIGHT bindingTextColor = WZCOL_FORM_TEXT;
	if (psMapping->subKeyCode >= KEY_KP_0 && psMapping->subKeyCode <= KEY_KPENTER)
	{
		bindingTextColor = WZCOL_YELLOW;
	}
	data.cache.wzBindingText.setText(sKey, font_regular);
	data.cache.wzBindingText.render(x + psWidget->width() - getMaxKeyMapNameWidth() - 2, y + (psWidget->height() / 2) + 3, bindingTextColor);
}

// ////////////////////////////////////////////////////////////////////////////
static bool keyMapEditor(bool first, WIDGET *parent, bool isInGame)
{
	if (first)
	{
		loadKeyMap();									// get the current mappings.
	}

	parent->attach(KeyMapForm::make(isInGame));

	/* Stop when the right number or when alphabetically last - not sure...! */
	/* Go home... */
	return true;
}

bool startInGameKeyMapEditor(bool first)
{
	bAllowOtherKeyPresses = false;
	return keyMapEditor(first, psWScreen->psForm.get(), true);
}

bool startKeyMapEditor(bool first)
{
	addBackdrop();
	addSideText(FRONTEND_SIDETEXT, KM_SX, KM_Y, _("KEY MAPPING"));
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	return keyMapEditor(first, parent, false);
}

// ////////////////////////////////////////////////////////////////////////////
// save current keymaps to registry
bool saveKeyMap()
{
	WzConfig ini(KeyMapPath, WzConfig::ReadAndWrite);
	if (!ini.status() || !ini.isWritable())
	{
		// NOTE: Changed to LOG_FATAL, since we want to inform user via pop-up (windows only)
		debug(LOG_FATAL, "Could not open %s", ini.fileName().toUtf8().c_str());
		return false;
	}

	ini.setValue("version", 1);

	ini.beginArray("mappings");
	for (auto const &mapping : keyMappings)
	{
		ini.setValue("name", mapping.name.c_str());
		ini.setValue("status", mapping.status);
		ini.setValue("meta", mapping.metaKeyCode);
		ini.setValue("sub", mapping.subKeyCode);
		ini.setValue("action", mapping.action);
		if (auto function = keymapEntryByFunction(mapping.function))
		{
			ini.setValue("function", function->name);
		}

		ini.nextArrayItem();
	}
	ini.endArray();

	debug(LOG_WZ, "Keymap written ok to %s.", KeyMapPath);
	return true;	// saved ok.
}

// ////////////////////////////////////////////////////////////////////////////
// load keymaps from registry.
bool loadKeyMap()
{
	// throw away any keymaps!!
	keyMappings.clear();

	WzConfig ini(KeyMapPath, WzConfig::ReadOnly);
	if (!ini.status())
	{
		debug(LOG_WZ, "%s not found", KeyMapPath);
		return false;
	}

	for (ini.beginArray("mappings"); ini.remainingArrayItems(); ini.nextArrayItem())
	{
		auto name = ini.value("name", "").toWzString();
		auto status = (KEY_STATUS)ini.value("status", 0).toInt();
		auto meta = (KEY_CODE)ini.value("meta", 0).toInt();
		auto sub = (KEY_CODE)ini.value("sub", 0).toInt();
		auto action = (KEY_ACTION)ini.value("action", 0).toInt();
		auto functionName = ini.value("function", "").toWzString();
		auto function = keymapEntryByName(functionName.toUtf8());
		if (function == nullptr)
		{
			debug(LOG_WARNING, "Skipping unknown keymap function \"%s\".", functionName.toUtf8().c_str());
			continue;
		}

		// add mapping
		keyAddMapping(status, meta, sub, action, function->function, name.toUtf8().c_str());
	}
	ini.endArray();
	return true;
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

		if (!(bMultiPlayer && NetPlay.bComms != 0)) // no editing in true multiplayer
		{
			addButton(KM_DEFAULT, KM_H - 8, _("Select Default"));
		}
	}

	//Put the buttons on it
	auto mappings = getVisibleMappings();

	std::sort(mappings.begin(), mappings.end(), [](KEY_MAPPING *a, KEY_MAPPING *b) {
		return a->name < b->name;
	});
	/* Add our first mapping to the form */
	/* Now add the others... */
	for (std::vector<KEY_MAPPING *>::const_iterator i = mappings.begin(); i != mappings.end(); ++i)
	{
		W_BUTINIT emptyInit;
		auto button = std::make_shared<W_BUTTON>(&emptyInit);
		button->setGeometry(0, 0, KM_ENTRYW, KM_ENTRYH);
		button->id = KM_START + (i - mappings.begin());
		button->displayFunction = displayKeyMap;
		button->pUserData = new DisplayKeyMapData(*i);
		button->setOnDelete([](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayKeyMapData *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		});
		button->addOnClickHandler([=](W_BUTTON& clickedButton) {
			auto clickedMapping = static_cast<DisplayKeyMapData *>(clickedButton.pUserData)->psMapping;
			if (!clickedMapping || clickedMapping->status != KEYMAP_ASSIGNABLE)
			{
				audio_PlayTrack(ID_SOUND_BUILD_FAIL);
			} else if (selectedKeyMap == clickedMapping) {
				unhighlightSelected();
			} else {
				keyMapList->disableScroll();
				selectedKeyMap = clickedMapping;
			}
		});
		keyMapList->addItem(button);
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
	if (selectedKeyMap)
	{
		KEY_CODE kc = scanKeyBoardForBinding();
		if (kc)
		{
			pushedKeyCombo(kc);
		}
	}
}

bool KeyMapForm::pushedKeyCombo(KEY_CODE subkey)
{
	KEY_CODE	metakey = KEY_IGNORE;
	KEY_MAPPING	*pExist;
	KEY_MAPPING	*psMapping;
	KEY_CODE	alt;

	// check for
	// alt
	alt = (KEY_CODE)0;
	if (keyDown(KEY_RALT) || keyDown(KEY_LALT))
	{
		metakey = KEY_LALT;
		alt = KEY_RALT;
	}
	// ctrl
	else if (keyDown(KEY_RCTRL) || keyDown(KEY_LCTRL))
	{
		metakey = KEY_LCTRL;
		alt = KEY_RCTRL;
	}
	// shift
	else if (keyDown(KEY_RSHIFT) || keyDown(KEY_LSHIFT))
	{
		metakey = KEY_LSHIFT;
		alt = KEY_RSHIFT;
	}
	// meta (cmd)
	else if (keyDown(KEY_RMETA) || keyDown(KEY_LMETA))
	{
		metakey = KEY_LMETA;
		alt = KEY_RMETA;
	}

	// check if bound to a fixed combo.
	pExist = keyFindMapping(metakey,  subkey);
	if (pExist && (pExist->status == KEYMAP_ALWAYS || pExist->status == KEYMAP_ALWAYS_PROCESS))
	{
		unhighlightSelected();
		return false;
	}

	/* Clear down mappings using these keys... But only if it isn't unassigned */
	keyReAssignMapping(metakey, subkey, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN);

	/* Try and see if its there already - damn well should be! */
	psMapping = keyGetMappingFromFunction(selectedKeyMap->function);

	/* Cough if it's not there */
	ASSERT_OR_RETURN(false, psMapping != nullptr, "Trying to patch a non-existent function mapping - whoop whoop!!!");

	/* Now alter it to the new values */
	psMapping->metaKeyCode = metakey;
	psMapping->subKeyCode = subkey;
	// was "=="
	psMapping->status = KEYMAP_ASSIGNABLE; //must be
	if (alt)
	{
		psMapping->altMetaKeyCode = alt;
	}
	unhighlightSelected();
	maxKeyMapNameWidthDirty = true;
	return true;
}

void KeyMapForm::unhighlightSelected()
{
	keyMapList->enableScroll();
	selectedKeyMap = nullptr;
}
