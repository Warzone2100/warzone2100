/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 * clParse.c
 *
 * Parse command line arguments
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_opengl/pieclip.h"

#include "levels.h"
#include "clparse.h"
#include "display3d.h"
#include "frontend.h"
#include "flowfield.h"
#include "keybind.h"
#include "loadsave.h"
#include "main.h"
#include "modding.h"
#include "multiplay.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"
#include "multilobbycommands.h"

#include <cwchar>

//////
// Our fine replacement for the popt abomination follows

#define POPT_ARG_STRING true
#define POPT_ARG_NONE false
#define POPT_ERROR_BADOPT -1
#define POPT_SKIP_MAC_PSN 666

struct poptOption
{
	const char *string;
	bool argument;
	int enumeration;
	const char *descrip;
	const char *argDescrip;
};

typedef struct _poptContext
{
	int argc = 0;
	int current = 0;
	int size = 0;
	const char * const *argv = nullptr;
	const char *parameter = nullptr;
	const char *bad = nullptr;
	const struct poptOption *table = nullptr;
} *poptContext;

/// TODO: Find a way to use the real qFatal from Qt
#undef qFatal
#define qFatal(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }

/// Enable automatic test games
static bool wz_autogame = false;
static std::string wz_saveandquit;
static std::string wz_test;
static std::string wz_autoratingUrl;
static bool wz_autoratingEnable = false;
static bool wz_cli_headless = false;
static bool wz_streamer_spectator_mode = false;
static bool wz_lobby_slashcommands = false;
static WZ_Command_Interface wz_cmd_interface = WZ_Command_Interface::None;
static int wz_min_autostart_players = -1;

#if defined(WZ_OS_WIN)

#include <ntverp.h>				// Windows SDK - include for access to VER_PRODUCTBUILD
#if VER_PRODUCTBUILD >= 9200
	// 9200 is the Windows SDK 8.0 (which introduced family support)
	#include <winapifamily.h>	// Windows SDK
#else
	// Earlier SDKs don't have the concept of families - provide simple implementation
	// that treats everything as "desktop"
	#if !defined(WINAPI_PARTITION_DESKTOP)
		#define WINAPI_PARTITION_DESKTOP			0x00000001
	#endif
	#if !defined(WINAPI_FAMILY_PARTITION)
		#define WINAPI_FAMILY_PARTITION(Partition)	((WINAPI_PARTITION_DESKTOP & Partition) == Partition)
	#endif
#endif

#include <fcntl.h>
#include <io.h>
#include <iostream>

void SetStdOutToConsole_Win()
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	static bool has_setup_console = false;
	const WORD MAX_CONSOLE_LINES = 9999;

	if (has_setup_console) { return; }

	if (AttachConsole(ATTACH_PARENT_PROCESS) == 0)
	{
		// failed to attach to parent process console
		// allocate a console for this app
		if (AllocConsole() == 0)
		{
			// failed to allocate a console
			return;
		}

		// give the new console window a nicer title
		SetConsoleTitleW(L"Warzone 2100");
	}

	SetConsoleOutputCP(CP_UTF8);

	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	// give the console window a bigger buffer size / scroll-back
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(consoleHandle, &csbi))
	{
		csbi.dwSize.Y = MAX_CONSOLE_LINES;
		SetConsoleScreenBufferSize(consoleHandle, csbi.dwSize);
	}

	FILE* fi = 0;
	freopen_s(&fi, "CONOUT$", "w", stdout);
	fi = 0;
	freopen_s(&fi, "CONOUT$", "w", stderr);

	std::ios::sync_with_stdio();

	has_setup_console = true;
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
}

#endif /* defined(WZ_OS_WIN) */

static void poptPrintHelp(poptContext ctx, FILE *output)
{
	// TRANSLATORS: Summary of commandline option syntax
	fprintf(output, _("Usage: %s [OPTION...]\n"), ctx->argv[0]);
	for (int i = 0; i < ctx->size; i++)
	{
		char txt[128];
		ssprintf(txt, "  --%s", ctx->table[i].string);

		if (ctx->table[i].argument)
		{
			sstrcat(txt, "=");
			if (ctx->table[i].argDescrip)
			{
				sstrcat(txt, ctx->table[i].argDescrip);
			}
		}

		// calculate number of terminal columns required to print
		// for languages with multibyte characters
		const size_t txtSize = strlen(txt) + 1;
		std::mbstate_t state = std::mbstate_t();
		const char *pTxt = txt;
		size_t txtOffset = std::mbsrtowcs(nullptr, &pTxt, 0, &state) + 1 - txtSize;
		// CJK characters take up two columns
		char language[3]; // stores ISO 639-1 code
		strlcpy(language, setlocale(LC_MESSAGES, nullptr), sizeof(language));
		if (strcmp(language, "zh") == 0 || strcmp(language, "ko") == 0)
		{
			txtOffset /= 2;
		}

		fprintf(output, "%-*s", static_cast<int>(40 - txtOffset), txt);

		if (ctx->table[i].descrip)
		{
			fprintf(output, "%s", ctx->table[i].descrip);
		}
		fprintf(output, "\n");
	}
}

static const char *poptBadOption(poptContext ctx, WZ_DECL_UNUSED int unused)
{
	return ctx->bad;
}

static const char *poptGetOptArg(poptContext ctx)
{
	return ctx->parameter;
}

static int poptGetNextOpt(poptContext ctx)
{
	static char match[PATH_MAX];		// static for bad function
	static char parameter[PATH_MAX];	// static for arg function
	char *pparam;
	int i;

	ctx->bad = nullptr;
	ctx->parameter = nullptr;
	parameter[0] = '\0';
	match[0] = '\0';

	if (ctx->current >= ctx->argc)	// counts from 1
	{
		return 0;
	}

	if (strstr(ctx->argv[ctx->current], "-psn_"))
	{
		ctx->current++;	// skip mac -psn_*  Yum!
		return POPT_SKIP_MAC_PSN;
	}

	sstrcpy(match, ctx->argv[ctx->current]);
	ctx->current++;
	pparam = strchr(match, '=');
	if (pparam)									// option's got a parameter
	{
		*pparam++ = '\0';							// split option from parameter and increment past '='
		if (pparam[0] == '"')							// found scary quotes
		{
			pparam++;							// skip start quote
			sstrcpy(parameter, pparam);					// copy first parameter
			if (!strrchr(pparam, '"'))					// if no end quote, then find it
			{
				while (!strrchr(parameter, '"') && ctx->current < ctx->argc)
				{
					sstrcat(parameter, " ");			// insert space
					sstrcat(parameter, ctx->argv[ctx->current]);
					ctx->current++;					// next part, please!
				}
			}
			if (strrchr(parameter, '"'))					// its not an else for above!
			{
				*strrchr(parameter, '"') = '\0';			// remove end qoute
			}
		}
		else
		{
			sstrcpy(parameter, pparam);					// copy parameter
		}
	}

	for (i = 0; i < ctx->size; i++)
	{
		char slong[64];

		ssprintf(slong, "--%s", ctx->table[i].string);
		if (strcmp(slong, match) == 0)
		{
			if (ctx->table[i].argument && pparam)
			{
				ctx->parameter = parameter;
			}
			return ctx->table[i].enumeration;
		}
	}
	ctx->bad = match;
	return POPT_ERROR_BADOPT;
}

static poptContext poptGetContext(WZ_DECL_UNUSED void *unused, int argc, const char * const *argv, const struct poptOption *table, WZ_DECL_UNUSED int none)
{
	static struct _poptContext ctx;

	ctx.argc = argc;
	ctx.argv = argv;
	ctx.table = table;
	ctx.current = 1;
	ctx.parameter = nullptr;

	for (ctx.size = 0; table[ctx.size].string; ctx.size++) ;	// count table size

	return &ctx;
}


typedef enum
{
	// We don't want to use zero, so start at one (1)
	CLI_CONFIGDIR = 1,
	CLI_DATADIR,
	CLI_DEBUG,
	CLI_DEBUGFILE,
	CLI_FLUSHDEBUGSTDERR,
	CLI_FULLSCREEN,
	CLI_GAME,
	CLI_HELP,
	CLI_MOD_GLOB,
	CLI_MOD_CA,
	CLI_MOD_MP,
	CLI_LOADSKIRMISH,
	CLI_LOADCAMPAIGN,
	CLI_LOADREPLAY,
	CLI_WINDOW,
	CLI_VERSION,
	CLI_RESOLUTION,
	CLI_SHADOWS,
	CLI_NOSHADOWS,
	CLI_SOUND,
	CLI_NOSOUND,
	CLI_CONNECTTOIP,
	CLI_CONNECTTOIP_SPECTATE,
	CLI_HOSTLAUNCH,
	CLI_NOASSERT,
	CLI_CRASH,
	CLI_TEXTURECOMPRESSION,
	CLI_NOTEXTURECOMPRESSION,
	CLI_GFXBACKEND,
	CLI_GFXDEBUG,
	CLI_JSBACKEND,
	CLI_AUTOGAME,
	CLI_SAVEANDQUIT,
	CLI_SKIRMISH,
	CLI_CONTINUE,
	CLI_AUTOHOST,
	CLI_AUTORATING,
	CLI_FLOWFIELD,
	CLI_AUTOHEADLESS,
#if defined(WZ_OS_WIN)
	CLI_WIN_ENABLE_CONSOLE,
#endif
	CLI_GAMEPORT,
	CLI_WZ_CRASH_RPT,
	CLI_WZ_DEBUG_CRASH_HANDLER,
	CLI_STREAMER_SPECTATOR,
	CLI_LOBBY_SLASHCOMMANDS,
	CLI_ADD_LOBBY_ADMINHASH,
	CLI_ADD_LOBBY_ADMINPUBLICKEY,
	CLI_COMMAND_INTERFACE,
	CLI_STARTPLAYERS,
} CLI_OPTIONS;

// Separate table that avoids *any* translated strings, to avoid any risk of gettext / libintl function calls
static const struct poptOption debugOptionsTable[] =
{
	{ "debug", POPT_ARG_STRING, CLI_DEBUG, nullptr, nullptr },
	{ "debugfile", POPT_ARG_STRING, CLI_DEBUGFILE, nullptr, nullptr },
	{ "flush-debug-stderr", POPT_ARG_NONE, CLI_FLUSHDEBUGSTDERR, nullptr, nullptr },
	// Terminating entry
	{ nullptr, 0, 0,              nullptr,                                    nullptr },
};

static const struct poptOption *getOptionsTable()
{
	static const struct poptOption optionsTable[] =
	{
		{ "configdir", POPT_ARG_STRING, CLI_CONFIGDIR,  N_("Set configuration directory"),       N_("configuration directory") },
		{ "datadir", POPT_ARG_STRING, CLI_DATADIR,    N_("Add data directory"),                N_("data directory") },
		{ "debug", POPT_ARG_STRING, CLI_DEBUG,      N_("Show debug for given level"),        N_("debug level") },
		{ "debugfile", POPT_ARG_STRING, CLI_DEBUGFILE,  N_("Log debug output to file"),          N_("file") },
		{ "flush-debug-stderr", POPT_ARG_NONE, CLI_FLUSHDEBUGSTDERR, N_("Flush all debug output written to stderr"), nullptr },
		{ "fullscreen", POPT_ARG_NONE, CLI_FULLSCREEN, N_("Play in fullscreen mode"),           nullptr },
		{ "game", POPT_ARG_STRING, CLI_GAME,       N_("Load a specific game mode"),         N_("level name") },
		{ "help",  POPT_ARG_NONE, CLI_HELP,       N_("Show options and exit"),             nullptr },
		{ "mod", POPT_ARG_STRING, CLI_MOD_GLOB,   N_("Enable a global mod"),               N_("mod") },
		{ "mod_ca", POPT_ARG_STRING, CLI_MOD_CA,     N_("Enable a campaign only mod"),        N_("mod") },
		{ "mod_mp", POPT_ARG_STRING, CLI_MOD_MP,     N_("Enable a multiplay only mod"),       N_("mod") },
		{ "noassert", POPT_ARG_NONE, CLI_NOASSERT,   N_("Disable asserts"),                   nullptr },
		{ "crash", POPT_ARG_NONE, CLI_CRASH,      N_("Causes a crash to test the crash handler"), nullptr },
		{ "loadskirmish", POPT_ARG_STRING, CLI_LOADSKIRMISH, N_("Load a saved skirmish game"),     N_("savegame") },
		{ "loadcampaign", POPT_ARG_STRING, CLI_LOADCAMPAIGN, N_("Load a saved campaign game"),     N_("savegame") },
		{ "loadreplay", POPT_ARG_STRING, CLI_LOADREPLAY, N_("Load a replay"),     N_("replay file") },
		{ "window", POPT_ARG_NONE, CLI_WINDOW,     N_("Play in windowed mode"),             nullptr },
		{ "version", POPT_ARG_NONE, CLI_VERSION,    N_("Show version information and exit"), nullptr },
		{ "resolution", POPT_ARG_STRING, CLI_RESOLUTION, N_("Set the resolution to use"),         N_("WIDTHxHEIGHT") },
		{ "shadows", POPT_ARG_NONE, CLI_SHADOWS,    N_("Enable shadows"),                    nullptr },
		{ "noshadows", POPT_ARG_NONE, CLI_NOSHADOWS,  N_("Disable shadows"),                   nullptr },
		{ "sound", POPT_ARG_NONE, CLI_SOUND,      N_("Enable sound"),                      nullptr },
		{ "nosound", POPT_ARG_NONE, CLI_NOSOUND,    N_("Disable sound"),                     nullptr },
		{ "join", POPT_ARG_STRING, CLI_CONNECTTOIP, N_("Connect directly to IP/hostname"),  N_("host") },
		{ "spectate", POPT_ARG_STRING, CLI_CONNECTTOIP_SPECTATE, N_("Connect directly to IP/hostname as a spectator"),  N_("host") },
		{ "host", POPT_ARG_NONE, CLI_HOSTLAUNCH, N_("Go directly to host screen"),        nullptr },
		{ "texturecompression", POPT_ARG_NONE, CLI_TEXTURECOMPRESSION, N_("Enable texture compression"), nullptr },
		{ "notexturecompression", POPT_ARG_NONE, CLI_NOTEXTURECOMPRESSION, N_("Disable texture compression"), nullptr },
		{ "gfxbackend", POPT_ARG_STRING, CLI_GFXBACKEND, N_("Set gfx backend"),
			"(opengl, opengles"
#if defined(WZ_VULKAN_ENABLED)
			", vulkan"
#endif
#if defined(WZ_BACKEND_DIRECTX)
			", directx"
#endif
			")"
		},
		{ "gfxdebug", POPT_ARG_NONE, CLI_GFXDEBUG, N_("Use gfx backend debug"), nullptr },
		{ "jsbackend", POPT_ARG_STRING, CLI_JSBACKEND, N_("Set JS backend"),
					"("
					"quickjs"
					")"
		},
		{ "autogame", POPT_ARG_NONE, CLI_AUTOGAME,   N_("Run games automatically for testing"), nullptr },
		{ "headless", POPT_ARG_NONE, CLI_AUTOHEADLESS,   N_("Headless mode (only supported when also specifying --autogame, --autohost, --skirmish)"), nullptr },
		{ "saveandquit", POPT_ARG_STRING, CLI_SAVEANDQUIT, N_("Immediately save game and quit"), N_("save name") },
		{ "skirmish", POPT_ARG_STRING, CLI_SKIRMISH,   N_("Start skirmish game with given settings file"), N_("test") },
		{ "continue", POPT_ARG_NONE, CLI_CONTINUE,   N_("Continue the last saved game"), nullptr },
		{ "autohost", POPT_ARG_STRING, CLI_AUTOHOST,   N_("Start host game with given settings file"), N_("autohost") },
		{ "autorating", POPT_ARG_STRING, CLI_AUTORATING,   N_("Query ratings from given server url (containing \"{HASH}\"), when hosting"), N_("autorating") },
        { "flowfield", POPT_ARG_NONE, CLI_FLOWFIELD, N_("Use advanced Flow fields method for path finding (experimental)"), nullptr },
		{ "autorating", POPT_ARG_STRING, CLI_AUTORATING,   N_("Query ratings from given server url, when hosting"), N_("autorating") },
#if defined(WZ_OS_WIN)
		{ "enableconsole", POPT_ARG_NONE, CLI_WIN_ENABLE_CONSOLE,   N_("Attach or create a console window and display console output (Windows only)"), nullptr },
#endif
		{ "gameport", POPT_ARG_STRING, CLI_GAMEPORT,   N_("Set game server port"), N_("port") },
		{ "wz-crash-rpt", POPT_ARG_NONE, CLI_WZ_CRASH_RPT, nullptr, nullptr },
		{ "wz-debug-crash-handler", POPT_ARG_NONE, CLI_WZ_DEBUG_CRASH_HANDLER, nullptr, nullptr },
		{ "spectator-min-ui", POPT_ARG_NONE, CLI_STREAMER_SPECTATOR, nullptr, nullptr},
		{ "enablelobbyslashcmd", POPT_ARG_NONE, CLI_LOBBY_SLASHCOMMANDS, N_("Enable lobby slash commands (for connecting clients)"), nullptr},
		{ "addlobbyadminhash", POPT_ARG_STRING, CLI_ADD_LOBBY_ADMINHASH, N_("Add a lobby admin identity hash (for slash commands)"), _("hash string")},
		{ "addlobbyadminpublickey", POPT_ARG_STRING, CLI_ADD_LOBBY_ADMINPUBLICKEY, N_("Add a lobby admin public key (for slash commands)"), N_("b64-pub-key")},
		{ "enablecmdinterface", POPT_ARG_STRING, CLI_COMMAND_INTERFACE, N_("Enable command interface"), N_("(stdin)")},
		{ "startplayers", POPT_ARG_STRING, CLI_STARTPLAYERS, N_("Minimum required players to auto-start game"), N_("startplayers")},
		// Terminating entry
		{ nullptr, 0, 0,              nullptr,                                    nullptr },
	};

	static struct poptOption TranslatedOptionsTable[sizeof(optionsTable) / sizeof(struct poptOption)];
	static bool translated = false;

	if (translated == false)
	{
		unsigned int table_size = sizeof(optionsTable) / sizeof(struct poptOption) - 1;
		unsigned int i;

		for (i = 0; i < table_size; ++i)
		{
			TranslatedOptionsTable[i] = optionsTable[i];

			// If there is a description, make sure to translate it with gettext
			if (TranslatedOptionsTable[i].descrip != nullptr)
			{
				TranslatedOptionsTable[i].descrip = gettext(TranslatedOptionsTable[i].descrip);
			}

			if (TranslatedOptionsTable[i].argDescrip != nullptr)
			{
				TranslatedOptionsTable[i].argDescrip = gettext(TranslatedOptionsTable[i].argDescrip);
			}
		}

		translated = true;
	}

	return TranslatedOptionsTable;
}

// Must not trigger or call any gettext / libintl routines!
bool ParseCommandLineDebugFlags(int argc, const char * const *argv)
{
	poptContext poptCon = poptGetContext(nullptr, argc, argv, debugOptionsTable, 0);
	int iOption;

#if defined(WZ_OS_MAC) && defined(DEBUG)
	debug_enable_switch("all");
#endif /* WZ_OS_MAC && DEBUG */

	/* loop through command line */
	while ((iOption = poptGetNextOpt(poptCon)) > 0 || iOption == POPT_ERROR_BADOPT)
	{
		CLI_OPTIONS option = (CLI_OPTIONS)iOption;
		const char *token;

		if (iOption == POPT_ERROR_BADOPT)
		{
			continue;
		}

		switch (option)
		{
		case CLI_DEBUG:
			// retrieve the debug section name
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Usage: --debug=<flag>");
			}

			// Attempt to enable the given debug section
			if (!debug_enable_switch(token))
			{
				qFatal("Debug flag \"%s\" not found!", token);
			}
			break;

		case CLI_DEBUGFILE:
		{
			// find the file name
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Missing debugfile filename?");
			}
			WzString debug_filename = token;
			debug_register_callback(debug_callback_file, debug_callback_file_init, debug_callback_file_exit, &debug_filename); // note: by the time this function returns, all use of debug_filename has completed
			customDebugfile = true;
			break;
		}

		case CLI_FLUSHDEBUGSTDERR:
			// Tell the debug stderr output callback to always flush its output
			debugFlushStderr();
			break;

		default:
			break;
		};
	}

	return true;
}

//! Early parsing of the commandline
/**
 * First half of the command line parsing. Also see ParseCommandLine()
 * below. The parameters here are needed early in the boot process,
 * while the ones in ParseCommandLine can benefit from debugging being
 * set up first.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns true on success, false on error */
bool ParseCommandLineEarly(int argc, const char * const *argv)
{
	poptContext poptCon = poptGetContext(nullptr, argc, argv, getOptionsTable(), 0);
	int iOption;

#if defined(WZ_OS_MAC) && defined(DEBUG)
	debug_enable_switch("all");
#endif /* WZ_OS_MAC && DEBUG */

	/* loop through command line */
	while ((iOption = poptGetNextOpt(poptCon)) > 0 || iOption == POPT_ERROR_BADOPT)
	{
		CLI_OPTIONS option = (CLI_OPTIONS)iOption;
		const char *token;

		if (iOption == POPT_ERROR_BADOPT)
		{
			qFatal("Unrecognized option: %s", poptBadOption(poptCon, 0));
		}

		switch (option)
		{

		case CLI_CONFIGDIR:
			// retrieve the configuration directory
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Unrecognised configuration directory");
			}
			if (strlen(token) >= (sizeof(configdir) / sizeof(configdir[0])))
			{
				qFatal("Configuration directory exceeds maximum supported length on this platform");
			}
			sstrcpy(configdir, token);
			break;

		case CLI_HELP:
			poptPrintHelp(poptCon, stdout);
			return false;

		case CLI_VERSION:
			printf("Warzone 2100 - %s\n", version_getFormattedVersionString());
			return false;

#if defined(WZ_OS_WIN)
		case CLI_WIN_ENABLE_CONSOLE:
			SetStdOutToConsole_Win();
			break;
#endif
		case CLI_WZ_CRASH_RPT:
		case CLI_WZ_DEBUG_CRASH_HANDLER:
			// this is currently a no-op because it must be parsed even earlier than ParseCommandLineEarly
			break;
		default:
			break;
		};
	}

	return true;
}

//! second half of parsing the commandline
/**
 * Second half of command line parsing. See ParseCommandLineEarly() for
 * the first half. Note that render mode must come before resolution flag.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns true on success, false on error */
bool ParseCommandLine(int argc, const char * const *argv)
{
	poptContext poptCon = poptGetContext(nullptr, argc, argv, getOptionsTable(), 0);
	int iOption;

	/* loop through command line */
	while ((iOption = poptGetNextOpt(poptCon)) > 0)
	{
		const char *token;
		CLI_OPTIONS option = (CLI_OPTIONS)iOption;

		switch (option)
		{
		case CLI_DEBUG:
		case CLI_DEBUGFILE:
		case CLI_FLUSHDEBUGSTDERR:
		case CLI_CONFIGDIR:
		case CLI_HELP:
		case CLI_VERSION:
#if defined(WZ_OS_WIN)
		case CLI_WIN_ENABLE_CONSOLE:
#endif
		case CLI_WZ_CRASH_RPT:
		case CLI_WZ_DEBUG_CRASH_HANDLER:
			// These options are parsed in ParseCommandLineEarly() already, so ignore them
			break;

		case CLI_NOASSERT:
			kf_NoAssert();
			break;

		// NOTE: The sole purpose of this is to test the crash handler.
		case CLI_CRASH:
			CauseCrash = true;
			NetPlay.bComms = false;
			sstrcpy(aLevelName, "CAM_3A");
			SetGameMode(GS_NORMAL);
			break;

		case CLI_DATADIR:
			// retrieve the quoted path name
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Unrecognised datadir");
			}
			sstrcpy(datadir, token);
			break;

		case CLI_FULLSCREEN:
			war_setWindowMode(WINDOW_MODE::fullscreen);
			break;
		case CLI_CONNECTTOIP:
		case CLI_CONNECTTOIP_SPECTATE:
			//get the ip we want to connect with, and go directly to join screen.
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("No IP/hostname given");
			}
			sstrcpy(iptoconnect, token);
			// also set spectate flag
			cliConnectToIpAsSpectator = (option == CLI_CONNECTTOIP_SPECTATE);
			break;
		case CLI_HOSTLAUNCH:
			// go directly to host screen, bypass all others.
			setHostLaunch(HostLaunch::Host);
			break;
		case CLI_GAME:
			// retrieve the game name
			token = poptGetOptArg(poptCon);
			if (token == nullptr
			    || (strcmp(token, "CAM_1A") && strcmp(token, "CAM_2A") && strcmp(token, "CAM_3A")
			        && strcmp(token, "TUTORIAL3") && strcmp(token, "FASTPLAY")))
			{
				qFatal("The game parameter requires one of the following keywords:"
				       "CAM_1A, CAM_2A, CAM_3A, TUTORIAL3, or FASTPLAY.");
			}
			NetPlay.bComms = false;
			bMultiPlayer = false;
			bMultiMessages = false;
			for (unsigned int i = 0; i < MAX_PLAYERS; i++)
			{
				NET_InitPlayer(i, true);
			}

			//NET_InitPlayer deallocates Player 0, who must be allocated so that a later invocation of processDebugMappings does not trigger DEBUG mode
			NetPlay.players[0].allocated = true;

			if (!strcmp(token, "CAM_1A") || !strcmp(token, "CAM_2A") || !strcmp(token, "CAM_3A"))
			{
				game.type = LEVEL_TYPE::CAMPAIGN;
			}
			else
			{
				game.type = LEVEL_TYPE::SKIRMISH; // tutorial is skirmish for some reason
			}
			sstrcpy(aLevelName, token);
			SetGameMode(GS_NORMAL);
			break;
		case CLI_MOD_GLOB:
			{
				// retrieve the file name
				token = poptGetOptArg(poptCon);
				if (token == nullptr)
				{
					qFatal("Missing mod name?");
				}

				global_mods.push_back(token);
				break;
			}
		case CLI_MOD_CA:
			{
				// retrieve the file name
				token = poptGetOptArg(poptCon);
				if (token == nullptr)
				{
					qFatal("Missing mod name?");
				}

				campaign_mods.push_back(token);
				break;
			}
		case CLI_MOD_MP:
			{
				// retrieve the file name
				token = poptGetOptArg(poptCon);
				if (token == nullptr)
				{
					qFatal("Missing mod name?");
				}

				multiplay_mods.push_back(token);
				break;
			}
		case CLI_RESOLUTION:
			{
				unsigned int width, height;

				token = poptGetOptArg(poptCon);
				if (sscanf(token, "%ux%u", &width, &height) != 2)
				{
					qFatal("Invalid parameter specified (format is WIDTHxHEIGHT, e.g. 800x600)");
				}
				if (width < 640)
				{
					debug(LOG_ERROR, "Screen width < 640 unsupported, using 640");
					width = 640;
				}
				if (height < 480)
				{
					debug(LOG_ERROR, "Screen height < 480 unsupported, using 480");
					height = 480;
				}
				// tell the display system of the desired resolution
				pie_SetVideoBufferWidth(width);
				pie_SetVideoBufferHeight(height);
				// and update the configuration
				war_SetWidth(width);
				war_SetHeight(height);
				break;
			}
		case CLI_LOADSKIRMISH:
			// retrieve the game name
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Unrecognised skirmish savegame name");
			}
			snprintf(saveGameName, sizeof(saveGameName), "%s/skirmish/%s.gam", SaveGamePath, token);
			sstrcpy(sRequestResult, saveGameName); // hack to avoid crashes
			SPinit(LEVEL_TYPE::SKIRMISH);
			bMultiPlayer = true;
			SetGameMode(GS_SAVEGAMELOAD);
			break;
		case CLI_LOADCAMPAIGN:
			// retrieve the game name
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Unrecognised campaign savegame name");
			}
			snprintf(saveGameName, sizeof(saveGameName), "%s/campaign/%s.gam", SaveGamePath, token);
			SPinit(LEVEL_TYPE::CAMPAIGN);
			SetGameMode(GS_SAVEGAMELOAD);
			break;
		case CLI_LOADREPLAY:
		{
			// retrieve the replay name
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Unrecognised replay name");
			}
			std::string extension;
			if (!strEndsWith(token, ".wzrp"))
			{
				extension = ".wzrp";
			}
			// check if we have a full path (relative to the replay dir)
			snprintf(saveGameName, sizeof(saveGameName), "%s/%s%s", ReplayPath, token, extension.c_str());
			bool foundReplayFile = PHYSFS_exists(saveGameName) != 0;
			if (!foundReplayFile)
			{
				// look in all possible replay subdirs (maybe we just have a filename)
				std::vector<std::string> replaySubdirs = {"skirmish", "multiplay"};
				for (auto& replaySubdir : replaySubdirs)
				{
					snprintf(saveGameName, sizeof(saveGameName), "%s/%s/%s%s", ReplayPath, replaySubdir.c_str(), token, extension.c_str());
					if (PHYSFS_exists(saveGameName))
					{
						foundReplayFile = true;
						break;
					}
				}
			}
			if (!foundReplayFile)
			{
				qFatal("Unable to find specified replay");
			}
			setHostLaunch(HostLaunch::LoadReplay);
			sstrcpy(sRequestResult, saveGameName); // hack to avoid crashes
			SPinit(LEVEL_TYPE::SKIRMISH);
			bMultiPlayer = true;
			game.maxPlayers = 4; //DEFAULTSKIRMISHMAPMAXPLAYERS;
			SetGameMode(GS_SAVEGAMELOAD);
			break;
		}
		case CLI_CONTINUE:
			if (findLastSave())
			{
				runContinue();
				SetGameMode(GS_SAVEGAMELOAD);
			}
			break;

		case CLI_WINDOW:
			war_setWindowMode(WINDOW_MODE::windowed);
			break;

		case CLI_SHADOWS:
			setDrawShadows(true);
			break;

		case CLI_NOSHADOWS:
			setDrawShadows(false);
			break;

		case CLI_SOUND:
			war_setSoundEnabled(true);
			break;

		case CLI_NOSOUND:
			war_setSoundEnabled(false);
			break;

		case CLI_TEXTURECOMPRESSION:
			wz_texture_compression = true;
			break;

		case CLI_NOTEXTURECOMPRESSION:
			wz_texture_compression = false;
			break;

		case CLI_GFXBACKEND:
			{
				// retrieve the backend
				token = poptGetOptArg(poptCon);
				if (token == nullptr)
				{
					qFatal("Unrecognised backend");
				}
				video_backend gfxBackend;
				if (video_backend_from_str(token, gfxBackend))
				{
					war_setGfxBackend(gfxBackend);
				}
				else
				{
					qFatal("Unsupported / invalid gfxbackend value");
				}
				break;
			}

		case CLI_GFXDEBUG:
			uses_gfx_debug = true;
			break;

		case CLI_JSBACKEND:
			{
				// retrieve the backend
				token = poptGetOptArg(poptCon);
				if (token == nullptr)
				{
					qFatal("Unrecognised backend");
				}
				JS_BACKEND jsBackend;
				if (js_backend_from_str(token, jsBackend))
				{
					war_setJSBackend(jsBackend);
				}
				else
				{
					qFatal("Unsupported / invalid jsbackend value");
				}
				break;
			}

		case CLI_AUTOGAME:
			wz_autogame = true;
			// need to cause wrappers.cpp to update calculated effective headless mode
			setHeadlessGameMode(wz_cli_headless);
			break;

		case CLI_SAVEANDQUIT:
			token = poptGetOptArg(poptCon);
			if (token == nullptr || !strchr(token, '/'))
			{
				qFatal("Bad savegame name (needs to be a full path)");
			}
			wz_saveandquit = token;
			break;

		case CLI_SKIRMISH:
			setHostLaunch(HostLaunch::Skirmish);
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Bad test key");
			}
			wz_test = token;
			break;

		case CLI_AUTOHOST:
			setHostLaunch(HostLaunch::Autohost);
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Bad test key");
			}
			wz_test = token;
			break;

		case CLI_AUTORATING:
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Bad autorating server");
			}
			wz_autoratingUrl = token;
			debug(LOG_INFO, "Using \"%s\" for ratings.", wz_autoratingUrl.c_str());
			break;
		case CLI_FLOWFIELD:
			flowfieldEnable();
			break;
		case CLI_AUTOHEADLESS:
			wz_cli_headless = true;
			setHeadlessGameMode(true);
			break;

		case CLI_GAMEPORT:
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Bad game server port");
			}
			NETsetGameserverPort(atoi(token));
			netGameserverPortOverride = true;
			debug(LOG_INFO, "Games will be hosted on port [%d]", NETgetGameserverPort());
			break;

		case CLI_STREAMER_SPECTATOR:
			wz_streamer_spectator_mode = true;
			break;

		case CLI_LOBBY_SLASHCOMMANDS:
			wz_lobby_slashcommands = true;
			break;

		case CLI_ADD_LOBBY_ADMINHASH:
			token = poptGetOptArg(poptCon);
			if (token == nullptr || strlen(token) == 0)
			{
				qFatal("Bad admin hash");
			}
			addLobbyAdminIdentityHash(token);
			break;

		case CLI_ADD_LOBBY_ADMINPUBLICKEY:
			token = poptGetOptArg(poptCon);
			if (token == nullptr || strlen(token) == 0)
			{
				qFatal("Bad admin public key");
			}
			addLobbyAdminPublicKey(token);
			break;

		case CLI_COMMAND_INTERFACE:
			token = poptGetOptArg(poptCon);
			if (token == nullptr || strlen(token) == 0)
			{
				// use default, which is currently "stdin"
				token = "stdin";
			}
			if (strcmp(token, "stdin") == 0)
			{
				// enable stdin
				wz_cmd_interface = WZ_Command_Interface::StdIn_Interface;
			}
			else
			{
				qFatal("Unsupported / invalid enablecmdinterface value");
			}
			break;
		case CLI_STARTPLAYERS:
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Bad start players count");
			}
			wz_min_autostart_players = atoi(token);
			if (wz_min_autostart_players < 0)
			{
				qFatal("Invalid start players count");
			}
			debug(LOG_INFO, "Games will automatically start with [%d] players (when ready)", wz_min_autostart_players);
			break;
		};
	}

	return true;
}

bool autogame_enabled()
{
	return wz_autogame;
}

const std::string &saveandquit_enabled()
{
	return wz_saveandquit;
}

const std::string &wz_skirmish_test()
{
	return wz_test;
}

void setAutoratingUrl(std::string url) {
	wz_autoratingUrl = url;
}

std::string getAutoratingUrl() {
	return wz_autoratingUrl;
}

void setAutoratingEnable(bool e)
{
	wz_autoratingEnable = e;
}

bool getAutoratingEnable() {
	return wz_autoratingEnable;
}

bool streamer_spectator_mode()
{
	return wz_streamer_spectator_mode;
}

bool lobby_slashcommands_enabled()
{
	return wz_lobby_slashcommands;
}

WZ_Command_Interface wz_command_interface()
{
	return wz_cmd_interface;
}

int min_autostart_player_count()
{
	return wz_min_autostart_players;
}
