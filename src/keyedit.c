/*
 * keyedit.c
 * keymap editor
 * alexl.
 */

// ////////////////////////////////////////////////////////////////////////////
// includes 
#include "frame.h"
#include "widget.h"
#include "frontend.h"
#include "frend.h"
#include "text.h"
#include "textdraw.h"
#include "hci.h"
#include "loadsave.h"
#include "keymap.h"
#include "csnap.h"
#include "intimage.h"
#include "bitimage.h"
#include "IntDisplay.h"
#include "Audio_id.h"

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
extern IMAGEFILE	*FrontImages;
extern CURSORSNAP	InterfaceSnap;

static KEY_MAPPING	*selectedKeyMap;
// ////////////////////////////////////////////////////////////////////////////
// protos

BOOL		runKeyMapEditor		(void);
static BOOL keyMapToString		(STRING *pStr, KEY_MAPPING *psMapping);
VOID		displayKeyMap		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
BOOL		startKeyMapEditor	(BOOL first);
BOOL		saveKeyMap			(void);
BOOL		loadKeyMap			(void);
static BOOL	pushedKeyMap		(UDWORD key);

extern char	buildTime[8];

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
static BOOL pushedKeyCombo(UDWORD subkey)
{
	KEY_CODE	metakey=KEY_IGNORE;
	KEY_MAPPING	*pExist;
   	KEY_MAPPING	*psMapping;
	KEY_CODE	alt;
   //	void (*function)(void);
   //	KEY_ACTION	action;
   //	KEY_STATUS	status;
   //	STRING	name[255];

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
	if(pExist && (pExist->status == KEYMAP_ALWAYS OR pExist->status == KEYMAP_ALWAYS_PROCESS))
	{
		selectedKeyMap = NULL;	// unhighlght selected.
		return FALSE;
	}

	/* Clear down mappings using these keys... But only if it isn't unassigned */
	keyReAssignMapping( metakey, subkey, KEY_IGNORE, KEY_MAXSCAN );

	/* Try and see if its there already - damn well should be! */
	psMapping = keyGetMappingFromFunction(selectedKeyMap->function);

	/* Cough if it's not there */
	ASSERT((psMapping!=NULL,"Trying to patch a non-existant function mapping - whoop whoop!!!"));

	/* Now alter it to the new values */
	psMapping->metaKeyCode = metakey;
	psMapping->subKeyCode = subkey;
	psMapping->status == KEYMAP_ASSIGNABLE; //must be
	if(alt)
	{
		psMapping->altMetaKeyCode = alt;
	}



	/*



	// unbind old mapping with this combo.
//	function = selectedKeyMap->function;
//	action = selectedKeyMap->action;
//	status = selectedKeyMap->status;
//	strcpy(name,selectedKeyMap->pName);
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
static UDWORD scanKeyBoardForBinding()
{
	UDWORD i;
	for(i = KEY_1; i <= KEY_DELETE; i++)
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
				return(i);	// top row key pressed
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
	
	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// returns key to press given a mapping.
static BOOL keyMapToString(STRING *pStr, KEY_MAPPING *psMapping)
{
	BOOL	onlySub = TRUE;
	STRING	asciiSub[20],asciiMeta[20];

	if(psMapping->metaKeyCode!=KEY_IGNORE)
	{
		keyScanToString(psMapping->metaKeyCode,(STRING *)&asciiMeta,20);
		onlySub = FALSE;
	}
	keyScanToString(psMapping->subKeyCode,(STRING *)&asciiSub,20);

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
VOID displayKeyMap(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD		x = xOffset+psWidget->x;
	UDWORD		y = yOffset+psWidget->y;
	UDWORD		w = psWidget->width;
	UDWORD		h = psWidget->height; 
	KEY_MAPPING *psMapping = (KEY_MAPPING*)psWidget->pUserData;
	STRING		sKey[MAX_NAME_SIZE];// was just 40
	UNUSEDPARAMETER(pColours);
	
	
	if(psMapping == selectedKeyMap)
	{
		pie_BoxFillIndex(x,y,x+w,y+h,COL_GREEN);
	}
	else if(psMapping->status == KEYMAP_ALWAYS OR psMapping->status == KEYMAP_ALWAYS_PROCESS)
	{
		pie_BoxFillIndex(x,y,x+w,y+h,COL_RED);
	}
	else
	{
		drawBlueBox(x,y,w,h);
	}

	// draw name
	iV_SetFont(WFont);											// font
	iV_SetTextColour(-1);										//colour

	pie_DrawText((UCHAR*)psMapping->pName, x+2, y+(psWidget->height/2)+3);

	// draw binding
	keyMapToString(sKey,psMapping);
	pie_DrawText((UCHAR*)sKey, x+370, y+(psWidget->height/2)+3);

	return;
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
	KEY_MAPPING	*psPresent,*psNext;
	char		test[255];
	addBackdrop();
	addSideText	(FRONTEND_SIDETEXT ,KM_X-2,KM_Y,strresGetString(psStringRes, STR_KM_KEYMAP_SIDE));

	if(first)
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
					STR_MUL_CANCEL,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	addMultiBut(psWScreen,KM_FORM,KM_DEFAULT,
				11,45,
				56,38,
				STR_MUL_DEFAULT,
				IMAGE_KEYMAP_DEFAULT,IMAGE_KEYMAP_DEFAULT,TRUE);	// default.


	/* Better be none that come after this...! */
	strcpy(test,"zzzzzzzzzzzzzzzzzzzzz");
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
				strcpy(test,psMapping->pName);
				psPresent = psMapping;
			}
		}
	}

	// add tab form..
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID			= KM_FORM;	
	sFormInit.id				= KM_FORM_TABS;
	sFormInit.style				= WFORM_TABBED;
	sFormInit.x					= 50;
	sFormInit.y					= 10;
	sFormInit.width				= KM_W- 100;	
	sFormInit.height			= KM_H- 4;	
	sFormInit.numMajor			= numForms(mapcount, BUTTONSPERKEYMAPPAGE);
	sFormInit.majorPos			= WFORM_TABTOP;
	sFormInit.minorPos			= WFORM_TABNONE;
	sFormInit.majorSize			= OBJ_TABWIDTH+3; 
	sFormInit.majorOffset		= OBJ_TABOFFSET;
	sFormInit.tabVertOffset		= (OBJ_TABHEIGHT/2);
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;
	sFormInit.pFormDisplay		= intDisplayObjectForm;	
	sFormInit.pUserData			= (void*)&StandardTab;
	sFormInit.pTabDisplay		= intDisplayTab;
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
	sButInit.x		  = 2;
	sButInit.y		  = 16;
	sButInit.id	 	  = KM_START;


	/* Add our first mapping to the form */
	sButInit.pUserData= (VOID*)psPresent;
	widgAddButton(psWScreen, &sButInit);
	sButInit.id++;

	/* Now add the others... */
	bubbleCount = 0;
	bAtEnd = FALSE;
	/* Stop when the right number or when aphabetically last - not sure...! */
	while(bubbleCount<mapcount-1 AND !bAtEnd)
	{
		/* Same test as before for upper limit */
	 	strcpy(test,"zzzzzzzzzzzzzzzzzzzzz");
		for(psMapping = keyMappings,psNext = NULL,bGotOne = FALSE; psMapping; psMapping = psMapping->psNext)
		{
			/* Only certain mappings get displayed */
			if( (psMapping->status!=KEYMAP__DEBUG)&&(psMapping->status!=KEYMAP___HIDE))		// if it's not a debug mapping..
			{
				/* If it's alphabetically good but better then next one */
				if(strcmp(psMapping->pName,test) < 0 AND strcmp(psMapping->pName,psPresent->pName) > 0)
				{
					/* Keep a record of it */
					strcpy(test,psMapping->pName);
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
	 		sButInit.pUserData= (VOID*)psNext;
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
BOOL saveKeyMap(void)
{
	KEY_MAPPING	*psMapping;
	FILE		*pFileHandle;
	SDWORD		count;
	STRING		name[128];

	pFileHandle = fopen("keymap.map", "wb");								// open the file
	if (!pFileHandle)
	{
		DBERROR(("Couldn't open keymap file"));
		return FALSE;
	}
	
	// write out number of entries.
	count =0;
	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		count++;
	}
	if (fwrite(&count, sizeof(SDWORD), 1, pFileHandle) != 1)		
	{
		DBERROR(("Keymap Write failed"));
		return FALSE;
	}

	// version data
	if (fwrite(&buildTime, 8, 1, pFileHandle) != 1)		
	{
		DBERROR(("version Write failed"));
		return FALSE;
	}

	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		// save this map.
		// name
		strcpy(name,psMapping->pName);
		if (fwrite(&name, 128 , 1, pFileHandle) != 1)		
		{
			DBERROR(("Keymap Write failed"));
			return FALSE;
		}
		
		// status
		if (fwrite(&psMapping->status, sizeof(KEY_STATUS), 1, pFileHandle) != 1)		
		{
			DBERROR(("Keymap Write failed"));
			return FALSE;
		}

		// metakey
		if (fwrite(&psMapping->metaKeyCode, sizeof(KEY_CODE), 1, pFileHandle) != 1)		
		{
			DBERROR(("Keymap Write failed"));
			return FALSE;
		}

		// subkey
		if (fwrite(&psMapping->subKeyCode, sizeof(KEY_CODE), 1, pFileHandle) != 1)		
		{
			DBERROR(("Keymap Write failed"));
			return FALSE;
		}

		// action
		if (fwrite(&psMapping->action, sizeof(KEY_ACTION), 1, pFileHandle) != 1)		
		{
			DBERROR(("Keymap Write failed"));
			return FALSE;
		}

		// function to map to!
		for(count = 0;  keyMapSaveTable[count] != NULL
					 && keyMapSaveTable[count] != psMapping->function;
			count++);
		if(keyMapSaveTable[count] == NULL)
		{
			DBERROR(("can't find keymapped function in the keymap save table!!"));
		}
	
		if (fwrite(&count, sizeof(UDWORD), 1, pFileHandle) != 1)		
		{
			DBERROR(("Keymap Write failed"));
			return FALSE;
		}
	}

	if (fclose(pFileHandle) != 0)
	{
		DBERROR(("Close failed for Keymap save"));
		return FALSE;
	}

	DBPRINTF(("Keymap written ok.\n"));
	return TRUE;	// saved ok.
}

// ////////////////////////////////////////////////////////////////////////////
// load keymaps from registry.
BOOL loadKeyMap(void)
{
	KEY_STATUS	status;
	KEY_CODE	metaCode;
	KEY_CODE	subCode;
	KEY_ACTION	action;
	STRING		name[128];
	FILE		*pFileHandle;
	SDWORD		count;
	UDWORD		funcmap;
	char		ver[8];

	// throw away any keymaps!!
	keyClearMappings();
	
	pFileHandle = fopen("keymap.map", "rb");								// check file exists
	if (pFileHandle == NULL)
	{
		return FALSE;														// failed
	}

	if (fread(&count, sizeof(UDWORD), 1, pFileHandle) != 1)			
	{
		DBERROR(("Read failed for keymap load"));
		fclose(pFileHandle);
		return FALSE;
	}

	// get version number.
	// if not from current version, create a new one..
	if(fread(&ver,8,1,pFileHandle) !=1)
	{
		fclose(pFileHandle);
		return FALSE;
	}
	if(strncmp(&ver,buildTime,8) != 0)	// check 
	{
		fclose(pFileHandle);
		return FALSE;	
	}

	for(; count>0;count--)
	{
		// name
		if (fread(&name, 128, 1, pFileHandle) != 1)					
		{
			DBERROR(("Read failed for keymap load"));
			fclose(pFileHandle);
			return FALSE;
		}

		// status
		if (fread(&status, sizeof(KEY_STATUS), 1, pFileHandle) != 1)					
		{
			DBERROR(("Read failed for keymap load"));
			fclose(pFileHandle);
			return FALSE;
		}
		// metakey
		if (fread(&metaCode, sizeof(KEY_CODE), 1, pFileHandle) != 1)					
		{
			DBERROR(("Read failed for keymap load"));
			fclose(pFileHandle);
			return FALSE;
		}
		// subkey
		if (fread(&subCode, sizeof(KEY_CODE), 1, pFileHandle) != 1)				
		{
			DBERROR(("Read failed for keymap load"));
			fclose(pFileHandle);
			return FALSE;
		}
		// action
		if (fread(&action, sizeof(KEY_ACTION), 1, pFileHandle) != 1)				
		{
			DBERROR(("Read failed for keymap load"));
			fclose(pFileHandle);
			return FALSE;
		}
		// function
		if (fread(&funcmap, sizeof(UDWORD), 1, pFileHandle) != 1)				
		{
			DBERROR(("Read failed for keymap load"));
			fclose(pFileHandle);
			return FALSE;
		}

		// add mapping
		keyAddMapping( status, metaCode, subCode, action, keyMapSaveTable[funcmap],(char*)&name);
	}

	if (fclose(pFileHandle) != 0)
	{
		DBERROR(("Close failed for load key map."));
		return FALSE;
	}
	return TRUE;

}