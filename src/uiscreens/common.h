#pragma once

#include <memory>

#include "lib/widget/button.h"
#include "lib/widget/slider.h"
#include "lib/widget/margin.h"
#include "lib/widget/label.h"

#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/bitimage.h"

#include "lib/framework/input.h"

#include "../intfac.h"
#include "../hci.h"
#include "../modding.h"
#include "../intimage.h"
#include "../intdisplay.h"
#include "../version.h"
#include "../init.h"
#include "../display.h" // HIDDEN_FRONTEND_WIDTH/HEIGHT
#include "../frend.h" // IMAGE_FE_LOGO
#include "../multiint.h" // MULTIOP_READY_WIDTH

// ////////////////////////////////////////////////////////////////////////////
// defines.

#define FRONTEND_TOPFORMX		50
#define FRONTEND_TOPFORMY		10
#define FRONTEND_TOPFORMW		540
#define FRONTEND_TOPFORMH		150


#define FRONTEND_TOPFORM_WIDEX	28
#define FRONTEND_TOPFORM_WIDEY	10
#define FRONTEND_TOPFORM_WIDEW	588
#define FRONTEND_TOPFORM_WIDEH	150


#define FRONTEND_BOTFORMX		FRONTEND_TOPFORMX
#define FRONTEND_BOTFORMY		170
#define FRONTEND_BOTFORMW		FRONTEND_TOPFORMW
#define FRONTEND_BOTFORMH		305				// keep Y+H < 480 (minimum display height)


#define FRONTEND_BUTWIDTH		FRONTEND_BOTFORMW-40 // text button sizes.
#define FRONTEND_BUTHEIGHT		35

#define FRONTEND_POS1X			20				// button positions
#define FRONTEND_POS1Y			(0*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS1M			340

#define FRONTEND_POS2X			20
#define FRONTEND_POS2Y			(1*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS2M			340

#define FRONTEND_POS3X			20
#define FRONTEND_POS3Y			(2*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS3M			340

#define FRONTEND_POS4X			20
#define FRONTEND_POS4Y			(3*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS4M			340

#define FRONTEND_POS5X			20
#define FRONTEND_POS5Y			(4*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS5M			340

#define FRONTEND_POS6X			20
#define FRONTEND_POS6Y			(5*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS6M			340

#define FRONTEND_POS7X			20
#define FRONTEND_POS7Y			(6*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS7M			340

#define FRONTEND_POS8X			20
#define FRONTEND_POS8Y			(7*FRONTEND_BUTHEIGHT)
#define FRONTEND_POS8M			340

#define FRONTEND_POS9X			-30				// special case for our hyperlink
#define FRONTEND_POS9Y			(8*FRONTEND_BUTHEIGHT)


#define FRONTEND_SIDEX			24
#define FRONTEND_SIDEY			FRONTEND_BOTFORMY

#define TRANSLATION_URL "https://translate.wz2100.net"

enum
{
	FRONTEND_BACKDROP = 20000,
	FRONTEND_TOPFORM,
	FRONTEND_BOTFORM,
	FRONTEND_LOGO,
	FRONTEND_SIDETEXT,					// sideways text
	FRONTEND_MULTILINE_SIDETEXT,				// sideways text
	FRONTEND_SIDETEXT1,					// sideways text
	FRONTEND_SIDETEXT2,					// sideways text
	FRONTEND_SIDETEXT3,					// sideways text
	FRONTEND_SIDETEXT4,					// sideways text
	FRONTEND_PASSWORDFORM,
	FRONTEND_HYPERLINK,
	FRONTEND_UPGRDLINK,
	FRONTEND_DONATELINK,
	FRONTEND_CHATLINK,
	// begin menu
	FRONTEND_SINGLEPLAYER = 20100,	// title screen
	FRONTEND_MULTIPLAYER,
	FRONTEND_TUTORIAL,
	FRONTEND_PLAYINTRO,
	FRONTEND_OPTIONS,
	FRONTEND_QUIT,
	FRONTEND_FASTPLAY,					//tutorial menu option
	FRONTEND_CONTINUE,
	FRONTEND_NEWGAME = 20200,	// single player (menu)
	FRONTEND_LOADGAME_MISSION,
	FRONTEND_LOADGAME_SKIRMISH,
	FRONTEND_REPLAY,
	FRONTEND_SKIRMISH,
	FRONTEND_CHALLENGES,
	FRONTEND_HOST = 20300,	//multiplayer menu options
	FRONTEND_JOIN,
	FE_P0,								// player 0 buton
	FE_P1,								// player 1 buton
	FE_P2,								// player 2 buton
	FE_P3,								// player 3 buton
	FE_P4,								// player 4 buton
	FE_P5,								// player 5 buton
	FE_P6,								// player 6 buton
	FE_P7,								// player 7 buton
	FE_MP_PR,  // Multiplayer player random button
	FE_MP_PMAX = FE_MP_PR + MAX_PLAYERS_IN_GUI,  // Multiplayer player blah button

	FRONTEND_CAMPAIGN_1,
	FRONTEND_CAMPAIGN_2,
	FRONTEND_CAMPAIGN_3,
	FRONTEND_CAMPAIGN_4,
	FRONTEND_CAMPAIGN_5,
	FRONTEND_CAMPAIGN_6,

	FRONTEND_GAMEOPTIONS = 21000,           // Game Options menu
	FRONTEND_LANGUAGE,
	FRONTEND_LANGUAGE_R,
	FRONTEND_COLOUR,
	FRONTEND_COLOUR_CAM,
	FRONTEND_COLOUR_MP,
	FRONTEND_DIFFICULTY,
	FRONTEND_DIFFICULTY_R,
	FRONTEND_CAMERASPEED,
	FRONTEND_CAMERASPEED_R,

	FRONTEND_GRAPHICSOPTIONS = 22000,       // Graphics Options Menu
	FRONTEND_FMVMODE,
	FRONTEND_FMVMODE_R,
	FRONTEND_SCANLINES,
	FRONTEND_SCANLINES_R,
	FRONTEND_SHADOWS,
	FRONTEND_SHADOWS_R,
	FRONTEND_FOG,
	FRONTEND_FOG_R,
	FRONTEND_RADAR,
	FRONTEND_RADAR_R,
	FRONTEND_RADAR_JUMP,
	FRONTEND_RADAR_JUMP_R,
	FRONTEND_SSHAKE,
	FRONTEND_SSHAKE_R,

	FRONTEND_AUDIO_AND_ZOOMOPTIONS = 23000,                 // Audio and Zoom Options Menu
	FRONTEND_3D_FX,						// 3d sound volume
	FRONTEND_FX,						// 2d (voice) sound volume
	FRONTEND_MUSIC,						// music volume
	FRONTEND_SUBTITLES,
	FRONTEND_SUBTITLES_R,
	FRONTEND_SOUND_HRTF,				// HRTF mode
	FRONTEND_MAP_ZOOM,					// map zoom
	FRONTEND_MAP_ZOOM_RATE,					// map zoom rate
	FRONTEND_RADAR_ZOOM,					// radar zoom rate
	FRONTEND_3D_FX_SL,
	FRONTEND_FX_SL,
	FRONTEND_MUSIC_SL,
	FRONTEND_SOUND_HRTF_R,
	FRONTEND_MAP_ZOOM_R,
	FRONTEND_MAP_ZOOM_RATE_R,
	FRONTEND_RADAR_ZOOM_R,

	FRONTEND_VIDEOOPTIONS = 24000,          // video Options Menu
	FRONTEND_WINDOWMODE,
	FRONTEND_WINDOWMODE_R,
	FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER,
	FRONTEND_RESOLUTION_READONLY_LABEL,
	FRONTEND_RESOLUTION_READONLY_CONTAINER,
	FRONTEND_RESOLUTION_READONLY,
	FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER,
	FRONTEND_RESOLUTION_DROPDOWN_LABEL,
	FRONTEND_RESOLUTION_DROPDOWN,
	FRONTEND_TEXTURESZ,
	FRONTEND_TEXTURESZ_R,
	FRONTEND_VSYNC,
	FRONTEND_VSYNC_R,
	FRONTEND_FSAA,
	FRONTEND_FSAA_R,
	FRONTEND_DISPLAYSCALE,
	FRONTEND_DISPLAYSCALE_R,
	FRONTEND_GFXBACKEND,
	FRONTEND_GFXBACKEND_R,

	FRONTEND_MOUSEOPTIONS = 25000,          // Mouse Options Menu
	FRONTEND_CURSORMODE,
	FRONTEND_CURSORMODE_R,
	FRONTEND_TRAP,
	FRONTEND_TRAP_R,
	FRONTEND_MFLIP,
	FRONTEND_MFLIP_R,
	FRONTEND_MBUTTONS,
	FRONTEND_MBUTTONS_R,
	FRONTEND_MMROTATE,
	FRONTEND_MMROTATE_R,
	FRONTEND_SCROLLEVENT,
	FRONTEND_SCROLLEVENT_R,

	FRONTEND_KEYMAP = 26000,	// Keymap menu

	FRONTEND_MUSICMANAGER = 27000,	// Music manager menu

	FRONTEND_NOGAMESAVAILABLE = 31666	// Used when no games are available in lobby

};

// ////////////////////////////////////////////////////////////////////////////
// FUNCTIONS

W_BUTTON* addSmallTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const char* txt, unsigned int style);
std::shared_ptr<W_BUTTON> makeTextButton(UDWORD id, const std::string& txt, unsigned int style);
std::shared_ptr<W_SLIDER> makeFESlider(UDWORD id, UDWORD parent, UDWORD stops, UDWORD pos);
std::shared_ptr<WIDGET> addMargin(std::shared_ptr<WIDGET> widget);
void moveToParentRightEdge(WIDGET* widget, int32_t right);
bool CancelPressed();
void displayTextOption(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset);
void displayTextAt270(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset);
void displayBigSlider(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset);
void displayLogo(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset);
void displayTitleBitmap(WZ_DECL_UNUSED WIDGET* psWidget, WZ_DECL_UNUSED UDWORD xOffset, WZ_DECL_UNUSED UDWORD yOffset);

void addTopForm(bool wide);
void addBottomForm();
W_FORM* addBackdrop();
W_FORM* addBackdrop(const std::shared_ptr<W_SCREEN>& screen);
void addTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const std::string& txt, unsigned int style);
W_LABEL* addSideText(UDWORD id, UDWORD PosX, UDWORD PosY, const char* txt);
W_LABEL* addSideText(const std::shared_ptr<W_SCREEN>& screen, UDWORD formId, UDWORD id, UDWORD PosX, UDWORD PosY, const char* txt);
void addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos);

void displayTextOption(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset);

bool CancelPressed();

// Cycle through options as in program seq(1) from coreutils
// The T cast is to cycle through enums.
template <typename T> static T seqCycle(T value, T min, int inc, T max)
{
	return widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_PRIMARY ?
		value < max ? T(value + inc) : min :  // Cycle forwards.
		min < value ? T(value - inc) : max;  // Cycle backwards.
}

// Cycle through options, which are powers of two, such as [128, 256, 512, 1024, 2048].
template <typename T> static T pow2Cycle(T value, T min, T max)
{
	return widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_PRIMARY ?
		value < max ? std::max<T>(1, value) * 2 : min :  // Cycle forwards.
		min < value ? (value / 2 > 1 ? value / 2 : 0) : max;  // Cycle backwards.
}

struct DisplayTextOptionCache
{
	WzText wzText;
};

struct TitleBitmapCache {
	WzText formattedVersionString;
	WzText modListText;
	WzText gfxBackend;
};
