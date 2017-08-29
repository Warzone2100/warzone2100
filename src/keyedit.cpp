/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/widget/button.h"

#include "frend.h"
#include "frontend.h"
#include "hci.h"
#include "init.h"
#include "intdisplay.h"
#include "keyedit.h"
#include "keymap.h"
#include "loadsave.h"
#include "main.h"
#include "multiint.h"

// ////////////////////////////////////////////////////////////////////////////
// defines

#define KM_FORM				10200
#define KM_RETURN			10202
#define KM_DEFAULT			10203

#define	KM_START			10204
#define	KM_END				10399

#define KM_W				FRONTEND_BOTFORMW
#define KM_H				440
#define KM_X				FRONTEND_BOTFORMX
#define KM_Y				20
#define KM_SX				FRONTEND_SIDEX


#define KM_ENTRYW			(FRONTEND_BOTFORMW - 80)
#define KM_ENTRYH			(16)


// ////////////////////////////////////////////////////////////////////////////
// variables

static KEY_MAPPING	*selectedKeyMap;

// ////////////////////////////////////////////////////////////////////////////
// funcs

static bool pushedKeyMap(UDWORD key)
{
	selectedKeyMap = (KEY_MAPPING *)widgGetFromID(psWScreen, key)->pUserData;
	if (selectedKeyMap && selectedKeyMap->status != KEYMAP_ASSIGNABLE)
	{
		selectedKeyMap = nullptr;
		audio_PlayTrack(ID_SOUND_BUILD_FAIL);

	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
static bool pushedKeyCombo(KEY_CODE subkey)
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
		selectedKeyMap = nullptr;	// unhighlight selected.
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
	selectedKeyMap = nullptr;	// unhighlight selected .
	return true;
}

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
	else if (id >= KM_START && id <= KM_END)
	{
		pushedKeyMap(id);
	}

	if (selectedKeyMap)
	{
		KEY_CODE kc = scanKeyBoardForBinding();
		if (kc)
		{
			pushedKeyCombo(kc);
		}
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

// ////////////////////////////////////////////////////////////////////////////
// display a keymap on the interface.
static void displayKeyMap(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();
	KEY_MAPPING *psMapping = (KEY_MAPPING *)psWidget->pUserData;
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
		drawBlueBox(x, y, w, h);
	}

	// draw name
	iV_SetTextColour(WZCOL_FORM_TEXT);
	iV_DrawText(_(psMapping->name.c_str()), x + 2, y + (psWidget->height() / 2) + 3, font_regular);

	// draw binding
	keyMapToString(sKey, psMapping);
	// Check to see if key is on the numpad, if so tell user and change color
	if (psMapping->subKeyCode >= KEY_KP_0 && psMapping->subKeyCode <= KEY_KPENTER)
	{
		iV_SetTextColour(WZCOL_YELLOW);
		sstrcat(sKey, " (numpad)");
	}
	iV_DrawText(sKey, x + 364, y + (psWidget->height() / 2) + 3, font_regular);
}

// ////////////////////////////////////////////////////////////////////////////
bool startKeyMapEditor(bool first)
{
	addBackdrop();
	addSideText(FRONTEND_SIDETEXT, KM_SX, KM_Y, _("KEY MAPPING"));

	if (first)
	{
		loadKeyMap();									// get the current mappings.
	}

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *kmForm = new IntFormAnimated(parent, false);
	kmForm->id = KM_FORM;
	kmForm->setGeometry(KM_X, KM_Y, KM_W, KM_H);

	addMultiBut(psWScreen, KM_FORM, KM_RETURN,			// return button.
	            8, 5,
	            iV_GetImageWidth(FrontImages, IMAGE_RETURN),
	            iV_GetImageHeight(FrontImages, IMAGE_RETURN),
	            _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	addMultiBut(psWScreen, KM_FORM, KM_DEFAULT,
	            11, 45,
	            iV_GetImageWidth(FrontImages, IMAGE_KEYMAP_DEFAULT), iV_GetImageWidth(FrontImages, IMAGE_KEYMAP_DEFAULT),
	            _("Select Default"),
	            IMAGE_KEYMAP_DEFAULT, IMAGE_KEYMAP_DEFAULT_HI, IMAGE_KEYMAP_DEFAULT_HI);	// default.

	// add tab form..
	IntListTabWidget *kmList = new IntListTabWidget(kmForm);
	kmList->setChildSize(KM_ENTRYW, KM_ENTRYH);
	kmList->setChildSpacing(3, 3);
	kmList->setGeometry(52, 10, KM_ENTRYW, KM_H - 10 - 25);

	//Put the buttons on it
	std::vector<KEY_MAPPING *> mappings;
	for (KEY_MAPPING &m : keyMappings)
	{
		if (m.status != KEYMAP__DEBUG && m.status != KEYMAP___HIDE)  // if it's not a debug mapping..
		{
			mappings.push_back(&m);
		}
	}
	std::sort(mappings.begin(), mappings.end(), [](KEY_MAPPING *a, KEY_MAPPING *b) {
		return a->name < b->name;
	});
	/* Add our first mapping to the form */
	/* Now add the others... */
	for (std::vector<KEY_MAPPING *>::const_iterator i = mappings.begin(); i != mappings.end(); ++i)
	{
		W_BUTTON *button = new W_BUTTON(kmList);
		button->id = KM_START + (i - mappings.begin());
		button->pUserData = *i;
		button->displayFunction = displayKeyMap;
		kmList->addWidgetToLayout(button);
	}

	/* Stop when the right number or when alphabetically last - not sure...! */
	/* Go home... */
	return true;
}



// ////////////////////////////////////////////////////////////////////////////
// save current keymaps to registry
bool saveKeyMap()
{
	WzConfig ini(KeyMapPath, WzConfig::ReadAndWrite);
	if (!ini.status() || !ini.isWritable())
	{
		// NOTE: Changed to LOG_FATAL, since we want to inform user via pop-up (windows only)
		debug(LOG_FATAL, "Could not open %s", ini.fileName().toUtf8().constData());
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
		auto name = ini.value("name", "").toString();
		auto status = (KEY_STATUS)ini.value("status", 0).toInt();
		auto meta = (KEY_CODE)ini.value("meta", 0).toInt();
		auto sub = (KEY_CODE)ini.value("sub", 0).toInt();
		auto action = (KEY_ACTION)ini.value("action", 0).toInt();
		auto functionName = ini.value("function", "").toString().toUtf8();
		auto function = keymapEntryByName(functionName.constData());
		if (function == nullptr)
		{
			debug(LOG_WARNING, "Skipping unknown keymap function \"%s\".", functionName.constData());
			continue;
		}

		// add mapping
		keyAddMapping(status, meta, sub, action, function->function, name.toUtf8().constData());
	}
	ini.endArray();
	return true;
}
