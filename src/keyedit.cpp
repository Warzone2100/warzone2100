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
	bool pushedKeyCombo(const KeyMappingInput input);

private:
	std::shared_ptr<ScrollableListWidget> keyMapList;
	void unhighlightSelected();
	void addButton(int buttonId, int y, const char *text);
};

struct DisplayKeyMapCache {
	WzText wzNameText;
	WzText wzPrimaryBindingText;
	WzText wzSecondaryBindingText;
};

struct DisplayKeyMapData {
	explicit DisplayKeyMapData(KEY_MAPPING *psMapping)
		: mappings(std::vector<KEY_MAPPING*>(static_cast<unsigned int>(KeyMappingPriority::LAST), nullptr))
	{
	}

	std::vector<KEY_MAPPING*> mappings;
	DisplayKeyMapCache cache;
};

// ////////////////////////////////////////////////////////////////////////////
// variables

static KEY_MAPPING	*selectedKeyMap;
static std::vector<bool> maxKeyMapNameWidthDirty(static_cast<unsigned int>(KeyMappingPriority::LAST), true);
static const std::string NOT_BOUND_LABEL = "<not bound>";

// ////////////////////////////////////////////////////////////////////////////
// funcs
// ////////////////////////////////////////////////////////////////////////////
static KEY_CODE scanKeyBoardForPressedBindableKey()
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

static MOUSE_KEY_CODE scanMouseForPressedBindableKey()
{
	UDWORD i;
	for (i = 0; i < MOUSE_KEY_CODE::MOUSE_END; i++)
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
	return (MOUSE_KEY_CODE)0;
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
		std::fill(maxKeyMapNameWidthDirty.begin(), maxKeyMapNameWidthDirty.end(), true);
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

static void mouseKeyCodeToString(MOUSE_KEY_CODE code, char* pResult, int maxStringLength)
{
	switch (code)
	{
	case MOUSE_KEY_CODE::MOUSE_LMB:
		strcpy(pResult, "Mouse Left");
		break;
	case MOUSE_KEY_CODE::MOUSE_MMB:
		strcpy(pResult, "Mouse Middle");
		break;
	case MOUSE_KEY_CODE::MOUSE_RMB:
		strcpy(pResult, "Mouse Right");
		break;
	case MOUSE_KEY_CODE::MOUSE_X1:
		strcpy(pResult, "Mouse 4");
		break;
	case MOUSE_KEY_CODE::MOUSE_X2:
		strcpy(pResult, "Mouse 5");
		break;
	case MOUSE_KEY_CODE::MOUSE_WUP:
		strcpy(pResult, "Mouse Wheel Up");
		break;
	case MOUSE_KEY_CODE::MOUSE_WDN:
		strcpy(pResult, "Mouse Wheel Down");
		break;
	default:
		strcpy(pResult, "Mouse ???");
		break;
	}
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

	// Figure out if the keycode is for mouse or keyboard. Default to assuming it 
	switch (psMapping->input.source)
	{
	case KeyMappingInputSource::KEY_CODE:
		keyScanToString(psMapping->input.value.keyCode, (char*)&asciiSub, 20);
		break;
	case KeyMappingInputSource::MOUSE_KEY_CODE:
		mouseKeyCodeToString(psMapping->input.value.mouseKeyCode, (char*)&asciiSub, 20);
		break;
	default:
		strcpy(asciiSub, "NOT VALID");
		debug(LOG_WZ, "Encountered invalid key mapping source %u while converting mapping to string!", static_cast<unsigned int>(psMapping->input.source));
		return true;
	}

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

static uint16_t getMaxKeyMapNameWidth(const KeyMappingPriority priority)
{
	static std::vector<int> max(static_cast<unsigned int>(KeyMappingPriority::LAST));

	const unsigned int priorityIndex = static_cast<unsigned int>(priority);
	if (maxKeyMapNameWidthDirty[priorityIndex]) {
		debug(LOG_WZ, "Calculating max width for %u (pIdx = %u)", static_cast<unsigned int>(priority), priorityIndex);
		WzText displayText;
		displayText.setText(NOT_BOUND_LABEL, font_regular);
		max[priorityIndex] = displayText.width();

		char sKey[MAX_STR_LENGTH];
		for (auto mapping: getVisibleMappings()) {
			if (mapping->priority == priority) {
				keyMapToString(sKey, mapping);
				displayText.setText(sKey, font_regular);
				max[priorityIndex] = MAX(max[priorityIndex], displayText.width());
			}
		}
		debug(LOG_WZ, "\tresult: %d", max[priorityIndex]);

		maxKeyMapNameWidthDirty[priorityIndex] = false;
	}

	return max[priorityIndex];
}

// ////////////////////////////////////////////////////////////////////////////
static void drawKeyMapping(const int x, const int y, const int w, const int h, const int boxX, const int boxY, const int boxW, const int boxH, const int textOffset, KEY_MAPPING* mapping, DisplayKeyMapData& data)
{
	PIELIGHT bindingTextColor = WZCOL_FORM_TEXT;
	char sPrimaryKey[MAX_STR_LENGTH];

	if (mapping)
	{
		if (mapping == selectedKeyMap)
		{
			pie_BoxFill(boxX, boxY, boxX + boxW, boxY + boxH, WZCOL_KEYMAP_ACTIVE);
		}
		else if (mapping->status == KEYMAP_ALWAYS || mapping->status == KEYMAP_ALWAYS_PROCESS)
		{
			pie_BoxFill(boxX, boxY, boxX + boxW, boxY + boxH, WZCOL_KEYMAP_FIXED);
		}
		else
		{
			drawBlueBoxInset(boxX, boxY, boxW, boxH);
		}

		// Check to see if key is on the numpad, if so tell user and change color
		if (mapping->input.source == KeyMappingInputSource::KEY_CODE && mapping->input.value.keyCode >= KEY_KP_0 && mapping->input.value.keyCode <= KEY_KPENTER)
		{
			bindingTextColor = WZCOL_YELLOW;
		}
		keyMapToString(sPrimaryKey, mapping);
	}
	else
	{
		drawBlueBoxInset(boxX, boxY, boxW, boxH);
		strcpy(sPrimaryKey, NOT_BOUND_LABEL.c_str());
	}

	data.cache.wzPrimaryBindingText.setText(sPrimaryKey, font_regular);
	data.cache.wzPrimaryBindingText.render(boxX, y + (h / 2) + 3, bindingTextColor);
}

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
	int primaryW = getMaxKeyMapNameWidth(KeyMappingPriority::PRIMARY);
	int secondaryW = getMaxKeyMapNameWidth(KeyMappingPriority::SECONDARY);
	KEY_MAPPING *primaryMapping = data.mappings[static_cast<unsigned int>(KeyMappingPriority::PRIMARY)];
	KEY_MAPPING *secondaryMapping = data.mappings[static_cast<unsigned int>(KeyMappingPriority::SECONDARY)];	

	drawBlueBoxInset(x, y, w - primaryW - secondaryW, h);

	// Draw name. Try to use primary mapping for the name, but if it for some reason does not exist, fall back to secondary.
	// If both fail, fall back to "<unknown>"
	const char* name = primaryMapping
		? primaryMapping->name.c_str()
		: secondaryMapping ? secondaryMapping->name.c_str() : "<unknown>";
	data.cache.wzNameText.setText(_(name), font_regular);
	data.cache.wzNameText.render(x + 2, y + (psWidget->height() / 2) + 3, WZCOL_FORM_TEXT);

	// draw binding(s)
	drawKeyMapping(
		x, y, w, h,
		x + w - primaryW - secondaryW,
		y,
		primaryW,
		h,
		-primaryW - 2 - secondaryW - 2,
		primaryMapping, data);

	drawKeyMapping(
		x, y, w, h,
		x + w - secondaryW,
		y,
		secondaryW,
		h,
		-secondaryW - 2,
		secondaryMapping, data);
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

		switch (mapping.input.source) {
		case KeyMappingInputSource::KEY_CODE:
			ini.setValue("source", "default");
			ini.setValue("sub", mapping.input.value.keyCode);
			break;
		case KeyMappingInputSource::MOUSE_KEY_CODE:
			ini.setValue("source", "mouse_key");
			ini.setValue("sub", mapping.input.value.mouseKeyCode);
			break;
		default:
			debug(LOG_WZ, "Encountered invalid key mapping source %u while saving keymap!", static_cast<unsigned int>(mapping.input.source));
			break;
		}
		switch (mapping.priority)
		{
		case KeyMappingPriority::PRIMARY:
			ini.setValue("priority", "primary");
			break;
		case KeyMappingPriority::SECONDARY:
			ini.setValue("priority", "secondary");
			break;
		default:
			debug(LOG_WZ, "Encountered invalid key mapping priority %u while saving keymap!", static_cast<unsigned int>(mapping.priority));
			break;
		}

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
		auto sub = ini.value("sub", 0).toInt();
		auto action = (KEY_ACTION)ini.value("action", 0).toInt();
		auto functionName = ini.value("function", "").toWzString();
		auto function = keymapEntryByName(functionName.toUtf8());
		if (function == nullptr)
		{
			debug(LOG_WARNING, "Skipping unknown keymap function \"%s\".", functionName.toUtf8().c_str());
			continue;
		}

		const WzString sourceName = ini.value("source", "default").toWzString();
		const KeyMappingInputSource source = keyMappingSourceByName(sourceName.toUtf8().c_str());

		// add mapping
		KeyMappingInput input;
		input.source = source;
		switch (source) {
		case KeyMappingInputSource::KEY_CODE:
			input.value.keyCode = (KEY_CODE)sub;
			break;
		case KeyMappingInputSource::MOUSE_KEY_CODE:
			input.value.mouseKeyCode = (MOUSE_KEY_CODE)sub;
			break;
		default:
			debug(LOG_WZ, "Encountered invalid key mapping source %u while loading keymap!", static_cast<unsigned int>(source));
			break;
		}

		const WzString priorityName = ini.value("priority", "primary").toWzString();
		const KeyMappingPriority priority = keyMappingPriorityByName(priorityName.toUtf8().c_str());

		keyAddMapping(status, meta, input, action, function->function, name.toUtf8().c_str(), priority);
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

	/* Add key mappings to the form */
	std::unordered_map<void(*)(), DisplayKeyMapData*> displayDataLookup;
	for (std::vector<KEY_MAPPING *>::const_iterator i = mappings.begin(); i != mappings.end(); ++i)
	{
		// Insert the mapping to correct priority slot in the display data. Create new data if no entry
		// for this keybind is available.
		KEY_MAPPING *mapping = *i;
		DisplayKeyMapData* data;

		if (!(data = displayDataLookup[mapping->function])) {
			data = new DisplayKeyMapData(*i);
			displayDataLookup[mapping->function] = data;

			// Only create the button if it does not exist yet
			// TODO 1309: separate buttons for primary/secondary keybinds?
			W_BUTINIT emptyInit;
			auto button = std::make_shared<W_BUTTON>(&emptyInit);
			button->setGeometry(0, 0, KM_ENTRYW, KM_ENTRYH);
			button->id = KM_START + (i - mappings.begin());
			button->displayFunction = displayKeyMap;
			button->pUserData = data;
			button->setOnDelete([](WIDGET* psWidget) {
				assert(psWidget->pUserData != nullptr);
				delete static_cast<DisplayKeyMapData*>(psWidget->pUserData);
				psWidget->pUserData = nullptr;
			});
			button->addOnClickHandler([=](W_BUTTON& clickedButton) {
				// TODO: Allow clicking the secondary key
				const int priorityIndex = static_cast<unsigned int>(KeyMappingPriority::PRIMARY);
				auto clickedMapping = static_cast<DisplayKeyMapData*>(clickedButton.pUserData)->mappings[priorityIndex];
				if (!clickedMapping || clickedMapping->status != KEYMAP_ASSIGNABLE)
				{
					audio_PlayTrack(ID_SOUND_BUILD_FAIL);
				}
				else if (selectedKeyMap == clickedMapping) {
					unhighlightSelected();
				}
				else {
					keyMapList->disableScroll();
					selectedKeyMap = clickedMapping;
				}
			});
			keyMapList->addItem(button);
		}

		data->mappings[static_cast<unsigned int>(mapping->priority)] = mapping;
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
		const KEY_CODE kc = scanKeyBoardForPressedBindableKey();
		if (kc)
		{
			pushedKeyCombo(KeyMappingInput((KEY_CODE)kc)); // TODO 1309
		}
		const MOUSE_KEY_CODE mkc = scanMouseForPressedBindableKey();
		if (mkc)
		{
			pushedKeyCombo(KeyMappingInput((MOUSE_KEY_CODE)mkc)); // TODO 1309
		}
	}
}

bool KeyMapForm::pushedKeyCombo(const KeyMappingInput input)
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
	pExist = keyFindMapping(metakey, input);
	if (pExist && (pExist->status == KEYMAP_ALWAYS || pExist->status == KEYMAP_ALWAYS_PROCESS))
	{
		unhighlightSelected();
		return false;
	}

	/* Clear down mappings using these keys... But only if it isn't unassigned */
	clearKeyMappingIfExists(metakey, input);

	/* Try and see if its there already - damn well should be! */
	// TODO 1309: this might not always be the case in case of secondary keybinds. Might be nullable for primaries now, too
	psMapping = keyGetMappingFromFunction(selectedKeyMap->function, KeyMappingPriority::PRIMARY);

	/* Cough if it's not there */
	ASSERT_OR_RETURN(false, psMapping != nullptr, "Trying to patch a non-existent function mapping - whoop whoop!!!");

	/* Now alter it to the new values */
	psMapping->metaKeyCode = metakey;
	psMapping->input = input;

	psMapping->status = KEYMAP_ASSIGNABLE;
	if (alt)
	{
		psMapping->altMetaKeyCode = alt;
	}
	unhighlightSelected();

	// TODO 1309: handle priorities here
	maxKeyMapNameWidthDirty[static_cast<unsigned int>(KeyMappingPriority::PRIMARY)] = true;
	return true;
}

void KeyMapForm::unhighlightSelected()
{
	keyMapList->enableScroll();
	selectedKeyMap = nullptr;
}
