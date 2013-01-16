/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#define	KM_FORM_TABS		10201
#define KM_RETURN			10202
#define KM_DEFAULT			10203

#define	KM_START			10204
#define	KM_END				10399

#define KM_W				FRONTEND_BOTFORMW
#define KM_H				440
#define KM_X				FRONTEND_BOTFORMX
#define KM_Y				20
#define KM_SX				FRONTEND_SIDEX

#define BUTTONSPERKEYMAPPAGE 20

#define KM_ENTRYW			(FRONTEND_BOTFORMW - 80)
#define KM_ENTRYH			(( (KM_H-50)/BUTTONSPERKEYMAPPAGE )-3 )


// ////////////////////////////////////////////////////////////////////////////
// variables

static KEY_MAPPING	*selectedKeyMap;
static char keymapVersion[8] = "KM_0002";

// ////////////////////////////////////////////////////////////////////////////
// funcs

static bool pushedKeyMap(UDWORD key)
{
	selectedKeyMap = (KEY_MAPPING *)widgGetFromID(psWScreen,key)->pUserData;
	if(selectedKeyMap && selectedKeyMap->status != KEYMAP_ASSIGNABLE)
	{
		selectedKeyMap = NULL;
	    audio_PlayTrack( ID_SOUND_BUILD_FAIL );

	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
static bool pushedKeyCombo(KEY_CODE subkey)
{
	KEY_CODE	metakey=KEY_IGNORE;
	KEY_MAPPING	*pExist;
   	KEY_MAPPING	*psMapping;
	KEY_CODE	alt;

	// check for
	// alt
	alt = (KEY_CODE)0;
	if( keyDown(KEY_RALT) || keyDown(KEY_LALT) )
	{
		metakey= KEY_LALT;
		alt = KEY_RALT;
	}
	// ctrl
	else if( keyDown(KEY_RCTRL) || keyDown(KEY_LCTRL) )
	{
		metakey = KEY_LCTRL;
		alt = KEY_RCTRL;
	}
	// shift
	else if( keyDown(KEY_RSHIFT) || keyDown(KEY_LSHIFT) )
	{
		metakey = KEY_LSHIFT;
		alt = KEY_RSHIFT;
	}
	// meta (cmd)
	else if( keyDown(KEY_RMETA) || keyDown(KEY_LMETA) )
	{
		metakey = KEY_LMETA;
		alt = KEY_RMETA;
	}

	// check if bound to a fixed combo.
	pExist = keyFindMapping(  metakey,  subkey );
	if(pExist && (pExist->status == KEYMAP_ALWAYS || pExist->status == KEYMAP_ALWAYS_PROCESS))
	{
		selectedKeyMap = NULL;	// unhighlght selected.
		return false;
	}

	/* Clear down mappings using these keys... But only if it isn't unassigned */
	keyReAssignMapping( metakey, subkey, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN );

	/* Try and see if its there already - damn well should be! */
	psMapping = keyGetMappingFromFunction((void*)selectedKeyMap->function);

	/* Cough if it's not there */
	ASSERT( psMapping!=NULL,"Trying to patch a non-existant function mapping - whoop whoop!!!" );

	/* Now alter it to the new values */
	psMapping->metaKeyCode = metakey;
	psMapping->subKeyCode = subkey;
	// was "=="
	psMapping->status = KEYMAP_ASSIGNABLE; //must be
	if(alt)
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
	for(i = 0; i < KEY_MAXSCAN; i++)
	{
		if(keyPressed((KEY_CODE)i))
		{
			if(i !=	KEY_RALT			// exceptions
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
	UDWORD id;

	id = widgRunScreen(psWScreen);						// Run the current set of widgets

	if(id == KM_RETURN)			// return
	{
		saveKeyMap();
		changeTitleMode(OPTIONS);
	}
	if(id == KM_DEFAULT)
	{
		keyClearMappings();
		keyInitMappings(true);
		widgDelete(psWScreen,FRONTEND_BACKDROP);// readd the widgets
		startKeyMapEditor(false);
	}
	else if( id>=KM_START && id<=KM_END)
	{
		 pushedKeyMap(id);
	}

	if(selectedKeyMap)
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
	char	asciiSub[20],asciiMeta[20];

	if(psMapping->metaKeyCode!=KEY_IGNORE)
	{
		keyScanToString(psMapping->metaKeyCode,(char *)&asciiMeta,20);
		onlySub = false;
	}
	keyScanToString(psMapping->subKeyCode,(char *)&asciiSub,20);

	if(onlySub)
	{
		sprintf(pStr,"%s",asciiSub);
	}
	else
	{
		sprintf(pStr,"%s %s", asciiMeta, asciiSub);
	}
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// display a keymap on the interface.
static void displayKeyMap(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	UDWORD		w = psWidget->width;
	UDWORD		h = psWidget->height;
	KEY_MAPPING *psMapping = (KEY_MAPPING*)psWidget->pUserData;
	char		sKey[MAX_STR_LENGTH];

	if(psMapping == selectedKeyMap)
	{
		pie_BoxFill(x, y, x + w, y + h, WZCOL_KEYMAP_ACTIVE);
	}
	else if(psMapping->status == KEYMAP_ALWAYS || psMapping->status == KEYMAP_ALWAYS_PROCESS)
	{
		// when user can't edit something...
		pie_BoxFill(x, y , x + w, y + h, WZCOL_KEYMAP_FIXED);
	}
	else
	{
		drawBlueBox(x,y,w,h);
	}

	// draw name
	iV_SetFont(font_regular);											// font type
	iV_SetTextColour(WZCOL_FORM_TEXT);

	iV_DrawText(_(psMapping->pName), x + 2, y + (psWidget->height / 2) + 3);

	// draw binding
	keyMapToString(sKey, psMapping);
	// Check to see if key is on the numpad, if so tell user and change color
	if (psMapping->subKeyCode >= KEY_KP_0 && psMapping->subKeyCode <= KEY_KPENTER)
	{
		iV_SetTextColour(WZCOL_YELLOW);
		sstrcat(sKey, " (numpad)");
	}
	iV_DrawText(sKey, x + 364, y + (psWidget->height / 2) + 3);
}

// ////////////////////////////////////////////////////////////////////////////
bool startKeyMapEditor(bool first)
{
	KEY_MAPPING	*psMapping;
	UDWORD		i,mapcount =0;
	UDWORD		bubbleCount;
	bool		bAtEnd,bGotOne;
	KEY_MAPPING	*psPresent = NULL, *psNext;
	char		test[255];

	addBackdrop();
	addSideText(FRONTEND_SIDETEXT, KM_SX, KM_Y, _("KEY MAPPING"));

	if (first)
	{
		loadKeyMap();									// get the current mappings.
	}
	W_FORMINIT sFormInit;
	sFormInit.formID	= FRONTEND_BACKDROP;
	sFormInit.id		= KM_FORM;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x			= KM_X;
	sFormInit.y			= KM_Y;
	sFormInit.width		= KM_W;
	sFormInit.height	= KM_H;
	sFormInit.pDisplay	= intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

	addMultiBut(psWScreen,KM_FORM,KM_RETURN,			// return button.
					8,5,
					iV_GetImageWidth(FrontImages,IMAGE_RETURN),
					iV_GetImageHeight(FrontImages,IMAGE_RETURN),
					_("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	addMultiBut(psWScreen,KM_FORM,KM_DEFAULT,
				11,45,
				56,38,
				_("Select Default"),
				IMAGE_KEYMAP_DEFAULT, IMAGE_KEYMAP_DEFAULT_HI, IMAGE_KEYMAP_DEFAULT_HI);	// default.

	/* Better be none that come after this...! */
	sstrcpy(test, "zzzzzzzzzzzzzzzzzzzzz");
	psMapping = NULL;

	//count mappings required.
	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		if( (psMapping->status!=KEYMAP__DEBUG)&&(psMapping->status!=KEYMAP___HIDE))		// if it's not a debug mapping..
		{
			mapcount++;
			if(strcmp(psMapping->pName,test) < 0)
			{
				/* Best one found so far */
				sstrcpy(test, psMapping->pName);
				psPresent = psMapping;
			}
		}
	}

	// add tab form..
	sFormInit = W_FORMINIT();
	sFormInit.formID		= KM_FORM;
	sFormInit.id			= KM_FORM_TABS;
	sFormInit.style			= WFORM_TABBED;
	sFormInit.x			= 50;
	sFormInit.y			= 10;
	sFormInit.width			= KM_W- 100;
	sFormInit.height		= KM_H- 4;
	sFormInit.numMajor		= numForms(mapcount, BUTTONSPERKEYMAPPAGE);
	sFormInit.majorPos		= WFORM_TABTOP;
	sFormInit.minorPos		= WFORM_TABNONE;
	sFormInit.majorSize		= OBJ_TABWIDTH+3;
	sFormInit.majorOffset		= OBJ_TABOFFSET;
	sFormInit.tabVertOffset		= (OBJ_TABHEIGHT/2);
	sFormInit.tabMajorThickness 	= OBJ_TABHEIGHT;
	sFormInit.pUserData		= &StandardTab;
	sFormInit.pTabDisplay		= intDisplayTab;

	// TABFIXME: Special case for tabs, since this one has whole screen to itself. No need to modify(?)
	for (i=0; i< sFormInit.numMajor; i++)
	{
		sFormInit.aNumMinors[i] = 1;
	}
	widgAddForm(psWScreen, &sFormInit);

	//Put the buttons on it
	W_BUTINIT sButInit;
	sButInit.formID   = KM_FORM_TABS;
	sButInit.width    = KM_ENTRYW;
	sButInit.height	  = KM_ENTRYH;
	sButInit.pDisplay = displayKeyMap;
	sButInit.x	  = 2;
	sButInit.y	  = 16;
	sButInit.id	  = KM_START;

	/* Add our first mapping to the form */
	sButInit.pUserData= psPresent;
	widgAddButton(psWScreen, &sButInit);
	sButInit.id++;
	sButInit.y +=  KM_ENTRYH +3;

	/* Now add the others... */
	bubbleCount = 0;
	bAtEnd = false;
	/* Stop when the right number or when aphabetically last - not sure...! */
	while(bubbleCount<mapcount-1 && !bAtEnd)
	{
		/* Same test as before for upper limit */
	 	sstrcpy(test, "zzzzzzzzzzzzzzzzzzzzz");
		for(psMapping = keyMappings,psNext = NULL,bGotOne = false; psMapping; psMapping = psMapping->psNext)
		{
			/* Only certain mappings get displayed */
			if( (psMapping->status!=KEYMAP__DEBUG)&&(psMapping->status!=KEYMAP___HIDE))		// if it's not a debug mapping..
			{
				/* If it's alphabetically good but better then next one */
				if(strcmp(psMapping->pName,test) < 0 && strcmp(psMapping->pName,psPresent->pName) > 0)
				{
					/* Keep a record of it */
					sstrcpy(test, psMapping->pName);
				   	psNext = psMapping;
					bGotOne = true;
				}
			}
		}
		/* We found one matching criteria */
		if(bGotOne)
		{
			psPresent = psNext;
			bubbleCount++;
			sButInit.pUserData= psNext;
	 		widgAddButton(psWScreen, &sButInit);
			 					// move on..
	 		sButInit.id++;
		  	/* Might be no more room on the page */
			if (  (sButInit.y + KM_ENTRYH+5 ) > (3+ (BUTTONSPERKEYMAPPAGE*(KM_ENTRYH+3))))
			{
				sButInit.y = 16;
				sButInit.majorID += 1;
			}
			else
			{
				sButInit.y +=  KM_ENTRYH +3;
			}
		}
		else
		{
			/* The previous one we found was alphabetically last - time to stop */
			bAtEnd = true;
		}
	}

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
	{	// NOTE: Changed to LOG_FATAL, since we want to inform user via pop-up (windows only)
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

	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
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
		if(keyMapSaveTable[count] == NULL)
		{
			debug( LOG_FATAL, "can't find keymapped function %s in the keymap save table at %d!", name, count );
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

	for(; count > 0; count--) {
		READ(&name, 128);	// name
		READ(&status, sizeof(KEY_STATUS));	// status
		READ(&metaCode, sizeof(KEY_CODE));	// metakey
		READ(&subCode, sizeof(KEY_CODE));	// subkey
		READ(&action, sizeof(KEY_ACTION));	// action
		READ(&funcmap, sizeof(funcmap));	// function

		// add mapping
		keyAddMapping( status, metaCode, subCode, action, keyMapSaveTable[funcmap],(char*)&name);
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
