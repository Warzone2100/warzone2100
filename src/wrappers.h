/* Wrappers.h */

#ifndef _wrappers_h
#define _wrappers_h

typedef enum {
	TITLECODE_CONTINUE,
	TITLECODE_STARTGAME,
	TITLECODE_QUITGAME,
	TITLECODE_SHOWINTRO,
	TITLECODE_SAVEGAMELOAD,
} TITLECODE;

//used to set the scriptWinLoseVideo variable
#define PLAY_NONE   0
#define PLAY_WIN    1
#define PLAY_LOSE   2


extern BOOL			frontendInitVars	    ( void );
extern TITLECODE	titleLoop			    ( void );

extern void			clearTitle	 		    ( void );
extern void			displayTitleScreen 	    ( void );

extern void			initLoadingScreen		( BOOL drawbdrop );
extern void			closeLoadingScreen	    ( void );
extern void			loadingScreenCallback   ( void );

extern void			startCreditsScreen	    ( void );

extern BOOL			displayGameOver		    ( BOOL success);
extern void			setPlayerHasLost	    ( BOOL val );
extern BOOL			testPlayerHasLost	    ( void );
extern BOOL			testPlayerHasWon  	    ( void );
extern void			setPlayerHasWon		    ( BOOL val );
extern void         setScriptWinLoseVideo   ( UBYTE val );
extern UBYTE        getScriptWinLoseVideo   ( void );


// PC version calls the loading bar code directly.
//#define LOADBARCALLBACK() loadingScreenCallback()
#define LOADBARCALLBACK()


#endif
