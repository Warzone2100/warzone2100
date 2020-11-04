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
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_opengl/pieclip.h"

#include "levels.h"
#include "clparse.h"
#include "display3d.h"
#include "frontend.h"
#include "keybind.h"
#include "loadsave.h"
#include "main.h"
#include "modding.h"
#include "multiplay.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"

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
			sstrcat(txt, ctx->table[i].argDescrip);
		}

		// calculate number of terminal columns required to print
		// for languages with multibyte characters
		const size_t txtSize = strlen(txt) + 1;
		std::mbstate_t state = std::mbstate_t();
		const char *pTxt = txt;
		int txtOffset = (int) std::mbsrtowcs(nullptr, &pTxt, 0, &state) + 1 - txtSize;
		// CJK characters take up two columns
		char language[3]; // stores ISO 639-1 code
		strlcpy(language, setlocale(LC_MESSAGES, nullptr), sizeof(language));
		if (strcmp(language, "zh") == 0 || strcmp(language, "ko") == 0)
		{
			txtOffset /= 2;
		}

		fprintf(output, "%-*s", 40 - txtOffset, txt);

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
	ctx->current++;
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
	CLI_WINDOW,
	CLI_VERSION,
	CLI_RESOLUTION,
	CLI_SHADOWS,
	CLI_NOSHADOWS,
	CLI_SOUND,
	CLI_NOSOUND,
	CLI_CONNECTTOIP,
	CLI_HOSTLAUNCH,
	CLI_NOASSERT,
	CLI_CRASH,
	CLI_TEXTURECOMPRESSION,
	CLI_NOTEXTURECOMPRESSION,
	CLI_GFXBACKEND,
	CLI_GFXDEBUG,
	CLI_AUTOGAME,
	CLI_SAVEANDQUIT,
	CLI_SKIRMISH,
	CLI_CONTINUE,
	CLI_AUTOHOST,
	CLI_AUTORATING,
} CLI_OPTIONS;

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
		{ "window", POPT_ARG_NONE, CLI_WINDOW,     N_("Play in windowed mode"),             nullptr },
		{ "version", POPT_ARG_NONE, CLI_VERSION,    N_("Show version information and exit"), nullptr },
		{ "resolution", POPT_ARG_STRING, CLI_RESOLUTION, N_("Set the resolution to use"),         N_("WIDTHxHEIGHT") },
		{ "shadows", POPT_ARG_NONE, CLI_SHADOWS,    N_("Enable shadows"),                    nullptr },
		{ "noshadows", POPT_ARG_NONE, CLI_NOSHADOWS,  N_("Disable shadows"),                   nullptr },
		{ "sound", POPT_ARG_NONE, CLI_SOUND,      N_("Enable sound"),                      nullptr },
		{ "nosound", POPT_ARG_NONE, CLI_NOSOUND,    N_("Disable sound"),                     nullptr },
		{ "join", POPT_ARG_STRING, CLI_CONNECTTOIP, N_("Connect directly to IP/hostname"),  N_("host") },
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
		{ "autogame", POPT_ARG_NONE, CLI_AUTOGAME,   N_("Run games automatically for testing"), nullptr },
		{ "saveandquit", POPT_ARG_STRING, CLI_SAVEANDQUIT, N_("Immediately save game and quit"), N_("save name") },
		{ "skirmish", POPT_ARG_STRING, CLI_SKIRMISH,   N_("Start skirmish game with given settings file"), N_("test") },
		{ "continue", POPT_ARG_NONE, CLI_CONTINUE,   N_("Continue the last saved game"), nullptr },
		{ "autohost", POPT_ARG_STRING, CLI_AUTOHOST,   N_("Start host game with given settings file"), N_("autohost") },
		{ "autorating", POPT_ARG_STRING, CLI_AUTORATING,   N_("Query ratings from given server url (containing \"{HASH}\"), when hosting"), N_("autorating") },
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
			war_setFullscreen(true);
			break;
		case CLI_CONNECTTOIP:
			//get the ip we want to connect with, and go directly to join screen.
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("No IP/hostname given");
			}
			sstrcpy(iptoconnect, token);
			break;
		case CLI_HOSTLAUNCH:
			// go directly to host screen, bypass all others.
			hostlaunch = HostLaunch::Host;
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
			for (int i = 0; i < MAX_PLAYERS; i++)
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
		case CLI_CONTINUE:
			if (findLastSave())
			{
				runContinue();
				SetGameMode(GS_SAVEGAMELOAD);
			}
			break;

		case CLI_WINDOW:
			war_setFullscreen(false);
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

		case CLI_AUTOGAME:
			wz_autogame = true;
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
			hostlaunch = HostLaunch::Skirmish;
			token = poptGetOptArg(poptCon);
			if (token == nullptr)
			{
				qFatal("Bad test key");
			}
			wz_test = token;
			break;

		case CLI_AUTOHOST:
			hostlaunch = HostLaunch::Autohost;
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

std::string autoratingUrl(std::string const &hash) {
	auto url = wz_autoratingUrl;
	auto h = wz_autoratingUrl.find_first_of("{HASH}");
	if (h != std::string::npos)
	{
		url.replace(h, 6, hash);
	}
	return url;
}
