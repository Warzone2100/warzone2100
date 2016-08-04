/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
static char keymapVersion[8] = "KM_0002";

// ////////////////////////////////////////////////////////////////////////////
// funcs

static bool pushedKeyMap(UDWORD key)
{
	selectedKeyMap = (KEY_MAPPING *)widgGetFromID(psWScreen, key)->pUserData;
	if (selectedKeyMap && selectedKeyMap->status != KEYMAP_ASSIGNABLE)
	{
		selectedKeyMap = NULL;
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
		selectedKeyMap = NULL;	// unhighlght selected.
		return false;
	}

	/* Clear down mappings using these keys... But only if it isn't unassigned */
	keyReAssignMapping(metakey, subkey, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN);

	/* Try and see if its there already - damn well should be! */
	psMapping = keyGetMappingFromFunction((void *)selectedKeyMap->function);

	/* Cough if it's not there */
	ASSERT_OR_RETURN(false, psMapping != NULL, "Trying to patch a non-existant function mapping - whoop whoop!!!");

	/* Now alter it to the new values */
	psMapping->metaKeyCode = metakey;
	psMapping->subKeyCode = subkey;
	// was "=="
	psMapping->status = KEYMAP_ASSIGNABLE; //must be
	if (alt)
	{
		psMapping->altMetaKeyCode = alt;
	}
	selectedKeyMap = NULL;	// unhighlght selected .
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
static KEY_CODE scanKeyBoardForBinding(void)
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
bool runKeyMapEditor(void)
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
		keyClearMappings();
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
	char		sKey[MAX_STR_LENGTH];

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
	iV_SetFont(font_regular);											// font type
	iV_SetTextColour(WZCOL_FORM_TEXT);

	iV_DrawText(_(psMapping->pName), x + 2, y + (psWidget->height() / 2) + 3);

	// draw binding
	keyMapToString(sKey, psMapping);
	// Check to see if key is on the numpad, if so tell user and change color
	if (psMapping->subKeyCode >= KEY_KP_0 && psMapping->subKeyCode <= KEY_KPENTER)
	{
		iV_SetTextColour(WZCOL_YELLOW);
		sstrcat(sKey, " (numpad)");
	}
	iV_DrawText(sKey, x + 364, y + (psWidget->height() / 2) + 3);
}

static bool keyMappingSort(KEY_MAPPING const *a, KEY_MAPPING const *b)
{
	return strcmp(a->pName, b->pName) < 0;
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
	for (KEY_MAPPING *m = keyMappings; m != nullptr; m = m->psNext)
	{
		if (m->status != KEYMAP__DEBUG && m->status != KEYMAP___HIDE)  // if it's not a debug mapping..
		{
			mappings.push_back(m);
		}
	}
	std::sort(mappings.begin(), mappings.end(), keyMappingSort);
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

	/* Stop when the right number or when aphabetically last - not sure...! */
	/* Go home... */
	return true;
}



// ////////////////////////////////////////////////////////////////////////////
// save current keymaps to registry
// FIXME: Use the endian-safe physfs functions.
bool saveKeyMap(void)
{
	KEY_MAPPING	*psMapping;
	SDWORD		count;
	char		name[128];
	PHYSFS_file *pfile;

	// KeyMapPath
	debug(LOG_WZ, "We are to write %s for keymap info", KeyMapPath);
	pfile = PHYSFS_openWrite(KeyMapPath);
	if (!pfile)
	{
		// NOTE: Changed to LOG_FATAL, since we want to inform user via pop-up (windows only)
		debug(LOG_FATAL, "saveKeyMap: %s could not be created: %s", KeyMapPath, PHYSFS_getLastError());
		assert(false);
		return false;
	}

#define WRITE(var, size)                                               \
	if (PHYSFS_write(pfile, var, 1, size) != size) {                     \
		debug(LOG_FATAL, "saveKeyMap: could not write to %s %d bytes: %s", \
		      KeyMapPath, (int)size, PHYSFS_getLastError());                    \
		assert(false);                                                     \
		return false;                                                      \
	}

	// write out number of entries.
	count = 0;
	for (psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		count++;
	}
	WRITE(&count, sizeof(count));
	WRITE(&keymapVersion, 8);

	for (psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		// save this map.
		// name
		sstrcpy(name, psMapping->pName);
		WRITE(&name, 128);

		WRITE(&psMapping->status, sizeof(KEY_STATUS));	// status
		WRITE(&psMapping->metaKeyCode, sizeof(KEY_CODE));	// metakey
		WRITE(&psMapping->subKeyCode, sizeof(KEY_CODE)); // subkey
		WRITE(&psMapping->action, sizeof(KEY_ACTION)); // action

		// function to map to!
		for (count = 0;  keyMapSaveTable[count] != NULL && keyMapSaveTable[count] != psMapping->function; count++) {}
		if (keyMapSaveTable[count] == NULL)
		{
			debug(LOG_FATAL, "can't find keymapped function %s in the keymap save table at %d!", name, count);
			abort();
		}
		WRITE(&count, sizeof(count));
	}
	if (!PHYSFS_close(pfile))
	{
		debug(LOG_FATAL, "Error closing %s: %s", KeyMapPath, PHYSFS_getLastError());
		assert(false);
		return false;
	}
	debug(LOG_WZ, "Keymap written ok to %s.", KeyMapPath);
	return true;	// saved ok.
#undef WRITE
}

// ////////////////////////////////////////////////////////////////////////////
// load keymaps from registry.
bool loadKeyMap(void)
{
	KEY_STATUS	status;
	KEY_CODE	metaCode;
	KEY_CODE	subCode;
	KEY_ACTION	action;
	char		name[128];
	SDWORD		count;
	UDWORD		funcmap;
	char		ver[8];
	PHYSFS_file *pfile;
	PHYSFS_sint64 filesize;
	PHYSFS_sint64 countsize = 0;
	PHYSFS_sint64 length_read;

	// throw away any keymaps!!
	keyClearMappings();

	if (!PHYSFS_exists(KeyMapPath))
	{
		debug(LOG_WZ, "%s not found", KeyMapPath);
		return false;
	}
	pfile = PHYSFS_openRead(KeyMapPath);
	if (!pfile)
	{
		// NOTE: Changed to LOG_FATAL, since we want to inform user via pop-up (windows only)
		debug(LOG_FATAL, "loadKeyMap: [directory: %s] %s could not be opened: %s", PHYSFS_getRealDir(KeyMapPath),
		      KeyMapPath, PHYSFS_getLastError());
		assert(false);
		return false;
	}
	filesize = PHYSFS_fileLength(pfile);

#define READ(var, size)                                       \
	length_read = PHYSFS_read(pfile, var, 1, size);             \
	countsize += length_read;                                   \
	if (length_read != size) {                                  \
		debug(LOG_FATAL, "loadKeyMap: Reading %s short: %s",      \
		      KeyMapPath, PHYSFS_getLastError());                 \
		assert(false);                                            \
		(void) PHYSFS_close(pfile);                               \
		return false;                                             \
	}

	READ(&count, sizeof(count));
	READ(&ver, 8);	// get version number.

	if (strncmp(ver, keymapVersion, 8) != 0)
	{
		/* If wrong version, create a new one instead. */
		PHYSFS_close(pfile);
		return false;
	}

	for (; count > 0; count--)
	{
		READ(&name, 128);	// name
		READ(&status, sizeof(KEY_STATUS));	// status
		READ(&metaCode, sizeof(KEY_CODE));	// metakey
		READ(&subCode, sizeof(KEY_CODE));	// subkey
		READ(&action, sizeof(KEY_ACTION));	// action
		READ(&funcmap, sizeof(funcmap));	// function

		// add mapping
		keyAddMapping(status, metaCode, subCode, action, keyMapSaveTable[funcmap], (char *)&name);
	}

	if (!PHYSFS_close(pfile))
	{
		debug(LOG_ERROR, "Error closing %s: %s", KeyMapPath, PHYSFS_getLastError());
		assert(false);
		return false;
	}
	if (countsize != filesize)
	{
		debug(LOG_FATAL, "File size different from bytes read!");
		assert(false);
	}
	return true;
}
