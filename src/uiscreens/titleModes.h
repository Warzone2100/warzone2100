#pragma once

// determines which option screen to use. when in GS_TITLE_SCREEN mode.
enum tMode
{
	TITLE,			// 0 intro mode
	SINGLE,			// 1 single player menu
	MULTI,			// 2 multiplayer menu
	OPTIONS,		// 3 options menu
	GAME,			// 4
	TUTORIAL,		// 5  tutorial/fastplay
	TITLE_UNUSED,	// 6
	FORCESELECT,	// 7 MULTIPLAYER, Force design screen
	STARTGAME,		// 8 Fire up the game
	SHOWINTRO,		// 9 reshow the intro
	QUIT,			// 10 leaving game
	LOADSAVEGAME,	// 11 loading a save game
	KEYMAP,			// 12 keymap editor
	GRAPHICS_OPTIONS,       // 13 graphics options menu
	AUDIO_AND_ZOOM_OPTIONS, // 14 audio and zoom options menu
	VIDEO_OPTIONS,          // 15 video options menu
	MOUSE_OPTIONS,          // 16 mouse options menu
	CAMPAIGNS,              // 17 campaign selector
	MUSIC_MANAGER,			// 18 music manager
};
