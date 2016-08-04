#include "lib/framework/wzglobal.h"
#include "lib/framework/types.h"
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/textdraw.h"

#include "src/console.h" // HACK

// --- console dummy implementations ---
// this should be implemented within ivis lib

#define MAX_CONSOLE_TMP_STRING_LENGTH	(255)
char ConsoleString[MAX_CONSOLE_TMP_STRING_LENGTH];
int titleMode;

bool addConsoleMessage(const char *Text, CONSOLE_TEXT_JUSTIFICATION jusType, SDWORD player, bool team)
{
	return true;
}

// --- misc dummy implementations ---

UDWORD realTime; // from gtime

bool bMultiPlayer; // FIXME, really should not access this from ivis lib

void addDumpInfo(const char *inbuffer)
{
}

utf_32_char* UTF8toUTF32(char const*, unsigned long*)
{
	return NULL;
}


bool war_getFullscreen()
{
	return false;
}

bool war_GetColouredCursor()
{
	return false;
}

bool war_GetVsync()
{
	return false;
}

void war_SetColouredCursor(bool enabled)
{
}

int war_GetScreen()
{
	return 0;
}

// --- end linking hacks ---

void mainLoop()
{
	iV_DrawTextRotated("Press ESC to exit.", 100, 100, 0.0f);
	pie_ScreenFlip(CLEAR_BLACK);
	frameUpdate(); // General housekeeping

	if (keyPressed(KEY_ESC))
	{
		inputLoseFocus();               // remove it from input stream
		wzQuit();
	}
}

int realmain(int argc, char **argv)
{
	wzMain(argc, argv);
	debug_init();
	debug_register_callback(debug_callback_stderr, NULL, NULL, NULL);
	pie_SetVideoBufferWidth(800);
	pie_SetVideoBufferHeight(600);
	if (!wzMainScreenSetup(0, false, true))
	{
		fprintf(stderr, "Failed to initialize graphics\n");
		return EXIT_FAILURE;
	}
	frameInitialise();
	screenInitialise();
	iV_font("DejaVu Sans", "Book", "Bold");
	iV_TextInit();

	wzMainEventLoop(); // enter main loop

	iV_TextShutdown();
	frameShutDown();
	screenShutDown();
	wzShutdown();
	return EXIT_SUCCESS;
}
