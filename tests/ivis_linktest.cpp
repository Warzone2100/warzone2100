#include "lib/framework/wzglobal.h"
#include "lib/framework/types.h"
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/pieclip.h"

#include "src/console.h" // HACK

// --- console dummy implementations ---
// this should be implemented within ivis/gfxqueue

#define MAX_CONSOLE_TMP_STRING_LENGTH	(255)
char ConsoleString[MAX_CONSOLE_TMP_STRING_LENGTH];

bool addConsoleMessage(const char *Text, CONSOLE_TEXT_JUSTIFICATION jusType, SDWORD player)
{
	return true;
}

// --- misc dummy implementations ---

UDWORD realTime; // from gtime

bool bMultiPlayer; // FIXME, really should not access this from ivis lib

void addDumpInfo(const char *inbuffer)
{
}

// --- end linking hacks ---

void mainLoop(void)
{
	pie_ScreenFlip(CLEAR_BLACK);
	frameUpdate(); // General housekeeping

	if (keyPressed(KEY_ESC))
	{
		inputLoseFocus();               // remove it from input stream
		exit(1);	// FIXME, insert SDL_QUIT event instead
	}
}

int realmain(int argc, char **argv)
{
	wzMain(argc, argv);
	debug_init();
	debug_register_callback(debug_callback_stderr, NULL, NULL, NULL);
	pie_SetVideoBufferWidth(800);
	pie_SetVideoBufferHeight(600);
	if (!wzMain2(0, false, true))
	{
		fprintf(stderr, "Failed to initialize graphics\n");
		return EXIT_FAILURE;
	}
	frameInitialise();
	screenInitialise();

	wzMain3(); // enter main loop

	frameShutDown();
	screenShutDown();
	wzShutdown();
	return EXIT_SUCCESS;
}
