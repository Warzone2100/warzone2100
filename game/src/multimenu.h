/*
 * MultiMenu.h
 *
 * Definition for in game,multiplayer, interface.
 */
// 
#ifndef __INCLUDED_MULTIMENU__
#define __INCLUDED_MULTIMENU__

// requester
extern VOID		addMultiRequest(STRING *ToFind, UDWORD id,UBYTE mapCam);
extern BOOL		multiRequestUp;
extern W_SCREEN *psRScreen;			// requester stuff.
extern BOOL		runMultiRequester(UDWORD id,UDWORD *contextmode, STRING *chosen,UDWORD *chosenValue);
extern void		displayRequestOption(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

// multimenu
extern VOID		intProcessMultiMenu		(UDWORD id);
extern BOOL		intRunMultiMenu			(VOID);
extern BOOL		intCloseMultiMenu		(VOID);
extern VOID		intCloseMultiMenuNoAnim	(VOID);
extern BOOL		intAddMultiMenu			(VOID);

extern BOOL		MultiMenuUp;
extern BOOL		ClosingMultiMenu;

//extern VOID		intDisplayMiniMultiMenu		(VOID);

#define MULTIMENU			10600
#define MULTIMENU_FORM		MULTIMENU

#endif
