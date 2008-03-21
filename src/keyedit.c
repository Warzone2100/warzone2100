/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
#include <SDL.h>
#include <physfs.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/input.h"
#include "lib/sound/audio.h"

#include "lib/widget/widget.h"
#include "frontend.h"
#include "frend.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/ivis_common/piepalette.h"
#include "hci.h"
#include "init.h"
#include "loadsave.h"
#include "keymap.h"
#include "intimage.h"
#include "lib/ivis_common/bitimage.h"
#include "intdisplay.h"
#include "lib/sound/audio_id.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "multiint.h"

// ////////////////////////////////////////////////////////////////////////////
// defines

#define KM_FORM				10200
#define	KM_FORM_TABS		10201
#define KM_RETURN			10202
#define KM_DEFAULT			10203

#define	KM_START			10204
#define	KM_END				10399

#define KM_W				580
#define KM_H				440
#define KM_X				30
#define KM_Y				20

#define KM_RETURNX			(KM_W-90)
#define KM_RETURNY			(KM_H-42)

#define BUTTONSPERKEYMAPPAGE 20

#define KM_ENTRYW			480
#define KM_ENTRYH			(( (KM_H-50)/BUTTONSPERKEYMAPPAGE )-3 )


// ////////////////////////////////////////////////////////////////////////////
// variables

static KEY_MAPPING	*selectedKeyMap;
// ////////////////////////////////////////////////////////////////////////////
// protos

BOOL		runKeyMapEditor		(void);
static BOOL keyMapToString		(char *pStr, KEY_MAPPING *psMapping);
void		displayKeyMap		(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
BOOL		startKeyMapEditor	(BOOL first);
BOOL		saveKeyMap		(void);
BOOL		loadKeyMap		(void);
static BOOL	pushedKeyMap		(UDWORD key);

char	keymapVersion[8] = "KM_0002";
extern char    KeyMapPath[];

// ////////////////////////////////////////////////////////////////////////////
// funcs

static BOOL pushedKeyMap(UDWORD key)
{
//	UDWORD count =0;
//	id-KM_START
//	for(selectedKeyMap = keyMappings;
//		selectedKeyMap->status != KEYMAP_ASSIGNABLE;
//		(selectedKeyMap->status= KEYMAP__DEBUG) && (selectedKeyMap->status==KEYMAP___HIDE);
//
//		selectedKeyMap = selectedKeyMap->psNext);
//
//	while(count!=key)
//	{
//		selectedKeyMap = selectedKeyMap->psNext;
//		if((selectedKeyMap->status!=KEYMAP__DEBUG)&&(selectedKeyMap->status!=KEYMAP___HIDE))		// if it's not a debug mapping..
//		if(selectedKeyMap->status == KEYMAP_ASSIGNABLE)
//		{
//			count++;
//		}
//	}
	selectedKeyMap = widgGetFromID(psWScreen,key)->pUserData;
	if(selectedKeyMap && selectedKeyMap->status != KEYMAP_ASSIGNABLE)
	{
		selectedKeyMap = NULL;
	    audio_PlayTrack( ID_SOUND_BUILD_FAIL );

	}

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
static BOOL pushedKeyCombo(KEY_CODE subkey)
{
	KEY_CODE	metakey=KEY_IGNORE;
	KEY_MAPPING	*pExist;
   	KEY_MAPPING	*psMapping;
	KEY_CODE	alt;
   //	void (*function)(void);
   //	KEY_ACTION	action;
   //	KEY_STATUS	status;
   //	char	name[255];

	// check for
	// alt
	alt = 0;
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

	// check if bound to a fixed combo.
	pExist = keyFindMapping(  metakey,  subkey );
	if(pExist && (pExist->status == KEYMAP_ALWAYS || pExist->status == KEYMAP_ALWAYS_PROCESS))
	{
		selectedKeyMap = NULL;	// unhighlght selected.
		return FALSE;
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



	/*



	// unbind old mapping with this combo.
//	function = selectedKeyMap->function;
//	action = selectedKeyMap->action;
//	status = selectedKeyMap->status;
//	strlcpy(name, selectedKeyMap->pName, sizeof(name));
//	keyRemoveMappingPt(selectedKeyMap);

	keyAddMapping(status,metakey,subkey,action,function,name);

	// add new binding.
//	keyReAssignMapping( selectedKeyMap->metaKeyCode, selectedKeyMap->subKeyCode, metakey, subkey);
  //	keyAddMapping(

	selectedKeyMap->metaKeyCode = metakey;
	selectedKeyMap->subKeyCode = subkey;

// readd the widgets.
//	widgDelete(psWScreen,FRONTEND_BACKDROP);
//	startKeyMapEditor(FALSE);

	*/
	selectedKeyMap = NULL;	// unhighlght selected .
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
static UDWORD scanKeyBoardForBinding(void)
{
	UDWORD i;
	for(i = 0; i < KEY_MAXSCAN; i++)
	{
		if(keyPressed(i))
		{
			if(i !=	KEY_RALT			// exceptions
			&& i !=	KEY_LALT
			&& i != KEY_RCTRL
			&& i != KEY_LCTRL
			&& i != KEY_RSHIFT
			&& i != KEY_LSHIFT
			)
			{
				return i;	// top row key pressed
			}
		}
	}
	return 0;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL runKeyMapEditor(void)
{
	UDWORD id;

	id = widgRunScreen(psWScreen);						// Run the current set of widgets

	if(id == KM_RETURN)			// return
	{
		saveKeyMap();
		changeTitleMode(TITLE);
	}
	if(id == KM_DEFAULT)
	{
		keyClearMappings();
		keyInitMappings(TRUE);
		widgDelete(psWScreen,FRONTEND_BACKDROP);// readd the widgets
		startKeyMapEditor(FALSE);
	}
	else if( id>=KM_START && id<=KM_END)
	{
		 pushedKeyMap(id);
	}

	if(selectedKeyMap)
	{
		id = scanKeyBoardForBinding();
		if(id)
		{
			pushedKeyCombo(id);
		}
	}

	widgDisplayScreen(psWScreen);				// show the widgets currently running

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// returns key to press given a mapping.
static BOOL keyMapToString(char *pStr, KEY_MAPPING *psMapping)
{
	BOOL	onlySub = TRUE;
	char	asciiSub[20],asciiMeta[20];

	if(psMapping->metaKeyCode!=KEY_IGNORE)
	{
		keyScanToString(psMapping->metaKeyCode,(char *)&asciiMeta,20);
		onlySub = FALSE;
	}
	keyScanToString(psMapping->subKeyCode,(char *)&asciiSub,20);

	if(onlySub)
	{
		sprintf(pStr,"%s",asciiSub);
	}
	else
	{
		sprintf(pStr,"%s - %s",asciiMeta,asciiSub);
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// display a keymap on the interface.
void displayKeyMap(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
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
		pie_BoxFill(x, y , x + w, y + h, WZCOL_KEYMAP_FIXED);
	}
	else
	{
		drawBlueBox(x,y,w,h);
	}

	// draw name
	iV_SetFont(font_regular);											// font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	iV_DrawText(psMapping->pName, x + 2, y + (psWidget->height / 2) + 3);

	// draw binding
	keyMapToString(sKey,psMapping);
	iV_DrawText(sKey, x + 370, y + (psWidget->height / 2) + 3);
}

// ////////////////////////////////////////////////////////////////////////////
BOOL startKeyMapEditor(BOOL first)
{
	W_BUTINIT	sButInit;
	W_FORMINIT	sFormInit;
	KEY_MAPPING	*psMapping;
	UDWORD		i,mapcount =0;
	UDWORD		bubbleCount;
	BOOL		bAtEnd,bGotOne;
	KEY_MAPPING	*psPresent = NULL, *psNext;
	char		test[255];
	addBackdrop();
	addSideText	(FRONTEND_SIDETEXT ,KM_X-2,KM_Y,_("KEY MAPPING"));

	if (first)
	{
		loadKeyMap();									// get the current mappings.
	}
	memset(&sFormInit, 0, sizeof(W_FORMINIT));			// draw blue form...
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
					_("Return To Previous Screen"),IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	addMultiBut(psWScreen,KM_FORM,KM_DEFAULT,
				11,45,
				56,38,
				_("Select Default"),
				IMAGE_KEYMAP_DEFAULT,IMAGE_KEYMAP_DEFAULT,TRUE);	// default.


	/* Better be none that come after this...! */
	strlcpy(test, "zzzzzzzzzzzzzzzzzzzzz", sizeof(test));
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
				strlcpy(test, psMapping->pName, sizeof(test));
				psPresent = psMapping;
			}
		}
	}

	// add tab form..
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
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
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID   = KM_FORM_TABS;
	sButInit.style	  = WFORM_PLAIN;
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
	bAtEnd = FALSE;
	/* Stop when the right number or when aphabetically last - not sure...! */
	while(bubbleCount<mapcount-1 && !bAtEnd)
	{
		/* Same test as before for upper limit */
	 	strlcpy(test, "zzzzzzzzzzzzzzzzzzzzz", sizeof(test));
		for(psMapping = keyMappings,psNext = NULL,bGotOne = FALSE; psMapping; psMapping = psMapping->psNext)
		{
			/* Only certain mappings get displayed */
			if( (psMapping->status!=KEYMAP__DEBUG)&&(psMapping->status!=KEYMAP___HIDE))		// if it's not a debug mapping..
			{
				/* If it's alphabetically good but better then next one */
				if(strcmp(psMapping->pName,test) < 0 && strcmp(psMapping->pName,psPresent->pName) > 0)
				{
					/* Keep a record of it */
					strlcpy(test, psMapping->pName, sizeof(test));
				   	psNext = psMapping;
					bGotOne = TRUE;
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
			bAtEnd = TRUE;
		}
	}

	/* Go home... */
	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////////
// save current keymaps to registry
// FIXME: Use the endian-safe physfs functions.
BOOL saveKeyMap(void)
{
	KEY_MAPPING	*psMapping;
	SDWORD		count;
	char		name[128];
	PHYSFS_file *pfile;

  // KeyMapPath
	debug(LOG_WZ, "We are to write %s for keymap info", KeyMapPath);
	pfile = PHYSFS_openWrite(KeyMapPath);
	if (!pfile) {
		debug(LOG_ERROR, "saveKeyMap: %s could not be created: %s", KeyMapPath,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}

#define WRITE(var, size)                                               \
	if (PHYSFS_write(pfile, var, 1, size) != size) {                     \
		debug(LOG_ERROR, "saveKeyMap: could not write to %s %d bytes: %s", \
		      KeyMapPath, (int)size, PHYSFS_getLastError());                    \
		assert(FALSE);                                                     \
		return FALSE;                                                      \
	}

	// write out number of entries.
	count = 0;
	for (psMapping = keyMappings; psMapping; psMapping = psMapping->psNext) {
		count++;
	}
	WRITE(&count, sizeof(count));
	WRITE(&keymapVersion, 8);

	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		// save this map.
		// name
		strlcpy(name, psMapping->pName, sizeof(name));
		WRITE(&name, 128);

		WRITE(&psMapping->status, sizeof(KEY_STATUS));	// status
		WRITE(&psMapping->metaKeyCode, sizeof(KEY_CODE));	// metakey
		WRITE(&psMapping->subKeyCode, sizeof(KEY_CODE)); // subkey
		WRITE(&psMapping->action, sizeof(KEY_ACTION)); // action

		// function to map to!
		for(count = 0;  keyMapSaveTable[count] != NULL
					 && keyMapSaveTable[count] != psMapping->function;
			count++);
		if(keyMapSaveTable[count] == NULL)
		{
			debug( LOG_ERROR, "can't find keymapped function %s in the keymap "
			       "save table at %d!", name, count );
			abort();
		}
		WRITE(&count, sizeof(count));
	}
	if (!PHYSFS_close(pfile)) {
		debug(LOG_ERROR, "saveKeyMap: Error closing %s: %s", KeyMapPath,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}
	debug(LOG_WZ, "Keymap written ok to %s.", KeyMapPath);
	return TRUE;	// saved ok.
#undef WRITE
}

// ////////////////////////////////////////////////////////////////////////////
// load keymaps from registry.
BOOL loadKeyMap(void)
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

	if (!PHYSFS_exists(KeyMapPath)) {
		debug(LOG_WZ, "loadKeyMap: %s not found", KeyMapPath);
		return FALSE;
	}
	pfile = PHYSFS_openRead(KeyMapPath);
	if (!pfile) {
		debug(LOG_ERROR, "loadKeyMap: %s could not be opened: %s", KeyMapPath,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}
	filesize = PHYSFS_fileLength(pfile);

#define READ(var, size)                                       \
	length_read = PHYSFS_read(pfile, var, 1, size);             \
	countsize += length_read;                                   \
	if (length_read != size) {                                  \
		debug(LOG_ERROR, "loadKeyMap: Reading %s short: %s",      \
		      KeyMapPath, PHYSFS_getLastError());                 \
		assert(FALSE);                                            \
		(void) PHYSFS_close(pfile);                               \
		return FALSE;                                             \
	}

	READ(&count, sizeof(count));
	READ(&ver, 8);	// get version number.

	if (strncmp(ver, keymapVersion, 8) != 0) {
		/* If wrong version, create a new one instead. */
		PHYSFS_close(pfile);
		return FALSE;
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

	if (!PHYSFS_close(pfile)) {
		debug(LOG_ERROR, "loadKeyMap: Error closing %s: %s", KeyMapPath,
		      PHYSFS_getLastError());
		assert(FALSE);
		return FALSE;
	}
	if (countsize != filesize) {
		debug(LOG_ERROR, "loadKeyMap: File size different from bytes read!");
		assert(FALSE);
	}
	return TRUE;
}
