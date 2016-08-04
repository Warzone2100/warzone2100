/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"

#include "clparse.h"
#include "display3d.h"
#include "frontend.h"
#include "keybind.h"
#include "loadsave.h"
#include "main.h"
#include "multiplay.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"

//////
// Our fine replacement for the popt abomination follows

#define POPT_ARG_STRING true
#define POPT_ARG_NONE false
#define POPT_ERROR_BADOPT -1
#define POPT_SKIP_MAC_PSN 666

struct poptOption
{
	const char *string;
	char short_form;
	bool argument;
	void *nullpointer;		// unused
	int enumeration;
	const char *descrip;
	const char *argDescrip;
};

typedef struct _poptContext
{
	int argc, current, size;
	const char **argv;
	const char *parameter;
	const char *bad;
	const struct poptOption *table;
} *poptContext;

/// TODO: Find a way to use the real qFatal from Qt
#undef qFatal
#define qFatal(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }

/// Enable automatic test games
static bool wz_autogame = false;

static void poptPrintHelp(poptContext ctx, FILE *output, WZ_DECL_UNUSED int unused)
{
	int i;

	fprintf(output, "Usage: %s [OPTION...]\n", ctx->argv[0]);
	for (i = 0; i < ctx->size; i++)
	{
		char txt[128];

		if (ctx->table[i].short_form != '\0')
		{
			ssprintf(txt, "  -%c, --%s", ctx->table[i].short_form, ctx->table[i].string);
		}
		else
		{
			ssprintf(txt, "  --%s", ctx->table[i].string);
		}

		if (ctx->table[i].argument)
		{
			sstrcat(txt, "=");
			sstrcat(txt, ctx->table[i].argDescrip);
		}

		fprintf(output, "%-40s", txt);
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

	ctx->bad = NULL;
	ctx->parameter = NULL;
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
	pparam = strrchr(match, '=');
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
		char sshort[3];
		char slong[64];

		ssprintf(sshort, "-%c", ctx->table[i].short_form);
		ssprintf(slong, "--%s", ctx->table[i].string);
		if ((strcmp(sshort, match) == 0 && ctx->table[i].short_form != '\0') || strcmp(slong, match) == 0)
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

static poptContext poptGetContext(WZ_DECL_UNUSED void *unused, int argc, const char **argv, const struct poptOption *table, WZ_DECL_UNUSED int none)
{
	static struct _poptContext ctx;

	ctx.argc = argc;
	ctx.argv = argv;
	ctx.table = table;
	ctx.current = 1;
	ctx.parameter = NULL;

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
	CLI_AUTOGAME,
} CLI_OPTIONS;

static const struct poptOption *getOptionsTable(void)
{
	static const struct poptOption optionsTable[] =
	{
		{ "configdir",  '\0', POPT_ARG_STRING, NULL, CLI_CONFIGDIR,  N_("Set configuration directory"),       N_("configuration directory") },
		{ "datadir",    '\0', POPT_ARG_STRING, NULL, CLI_DATADIR,    N_("Set default data directory"),        N_("data directory") },
		{ "debug",      '\0', POPT_ARG_STRING, NULL, CLI_DEBUG,      N_("Show debug for given level"),        N_("debug level") },
		{ "debugfile",  '\0', POPT_ARG_STRING, NULL, CLI_DEBUGFILE,  N_("Log debug output to file"),          N_("file") },
		{ "flush-debug-stderr", '\0', POPT_ARG_NONE, NULL, CLI_FLUSHDEBUGSTDERR, N_("Flush all debug output written to stderr"), NULL },
		{ "fullscreen", '\0', POPT_ARG_NONE,   NULL, CLI_FULLSCREEN, N_("Play in fullscreen mode"),           NULL },
		{ "game",       '\0', POPT_ARG_STRING, NULL, CLI_GAME,       N_("Load a specific game"),              N_("game-name") },
		{ "help",       'h',  POPT_ARG_NONE,   NULL, CLI_HELP,       N_("Show this help message and exit"),   NULL },
		{ "mod",        '\0', POPT_ARG_STRING, NULL, CLI_MOD_GLOB,   N_("Enable a global mod"),               N_("mod") },
		{ "mod_ca",     '\0', POPT_ARG_STRING, NULL, CLI_MOD_CA,     N_("Enable a campaign only mod"),        N_("mod") },
		{ "mod_mp",     '\0', POPT_ARG_STRING, NULL, CLI_MOD_MP,     N_("Enable a multiplay only mod"),       N_("mod") },
		{ "noassert",	'\0', POPT_ARG_NONE,   NULL, CLI_NOASSERT,   N_("Disable asserts"),                   NULL },
		{ "crash",		'\0', POPT_ARG_NONE,   NULL, CLI_CRASH,      N_("Causes a crash to test the crash handler"), NULL },
		{ "loadskirmish",'\0', POPT_ARG_STRING, NULL, CLI_LOADSKIRMISH, N_("Load a saved skirmish game"),     N_("savegame") },
		{ "loadcampaign",'\0', POPT_ARG_STRING, NULL, CLI_LOADCAMPAIGN, N_("Load a saved campaign game"),     N_("savegame") },
		{ "window",     '\0', POPT_ARG_NONE,   NULL, CLI_WINDOW,     N_("Play in windowed mode"),             NULL },
		{ "version",    '\0', POPT_ARG_NONE,   NULL, CLI_VERSION,    N_("Show version information and exit"), NULL },
		{ "resolution", '\0', POPT_ARG_STRING, NULL, CLI_RESOLUTION, N_("Set the resolution to use"),         N_("WIDTHxHEIGHT") },
		{ "shadows",    '\0', POPT_ARG_NONE,   NULL, CLI_SHADOWS,    N_("Enable shadows"),                    NULL },
		{ "noshadows",  '\0', POPT_ARG_NONE,   NULL, CLI_NOSHADOWS,  N_("Disable shadows"),                   NULL },
		{ "sound",      '\0', POPT_ARG_NONE,   NULL, CLI_SOUND,      N_("Enable sound"),                      NULL },
		{ "nosound",    '\0', POPT_ARG_NONE,   NULL, CLI_NOSOUND,    N_("Disable sound"),                     NULL },
		{ "join",       '\0', POPT_ARG_STRING, NULL, CLI_CONNECTTOIP, N_("Connect directly to IP/hostname"),   N_("host") },
		{ "host",       '\0', POPT_ARG_NONE,   NULL, CLI_HOSTLAUNCH, N_("Go directly to host screen"),        NULL },
		{ "texturecompression", '\0', POPT_ARG_NONE, NULL, CLI_TEXTURECOMPRESSION, N_("Enable texture compression"), NULL },
		{ "notexturecompression", '\0', POPT_ARG_NONE, NULL, CLI_NOTEXTURECOMPRESSION, N_("Disable texture compression"), NULL },
		{ "autogame",   '\0', POPT_ARG_NONE,   NULL, CLI_AUTOGAME,   N_("Run games automatically for testing"), NULL },
		// Terminating entry
		{ NULL,         '\0', 0,               NULL, 0,              NULL,                                    NULL },
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
			if (TranslatedOptionsTable[i].descrip != NULL)
			{
				TranslatedOptionsTable[i].descrip = gettext(TranslatedOptionsTable[i].descrip);
			}

			if (TranslatedOptionsTable[i].argDescrip != NULL)
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
bool ParseCommandLineEarly(int argc, const char **argv)
{
	poptContext poptCon = poptGetContext(NULL, argc, argv, getOptionsTable(), 0);
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
			if (token == NULL)
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
			// find the file name
			token = poptGetOptArg(poptCon);
			if (token == NULL)
			{
				qFatal("Missing debugfile filename?");
			}
			debug_register_callback(debug_callback_file, debug_callback_file_init, debug_callback_file_exit, (void *)token);
			customDebugfile = true;
			break;

		case CLI_FLUSHDEBUGSTDERR:
			// Tell the debug stderr output callback to always flush its output
			debugFlushStderr();
			break;

		case CLI_CONFIGDIR:
			// retrieve the configuration directory
			token = poptGetOptArg(poptCon);
			if (token == NULL)
			{
				qFatal("Unrecognised configuration directory");
			}
			sstrcpy(configdir, token);
			break;

		case CLI_HELP:
			poptPrintHelp(poptCon, stdout, 0);
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
bool ParseCommandLine(int argc, const char **argv)
{
	poptContext poptCon = poptGetContext(NULL, argc, argv, getOptionsTable(), 0);
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
			if (token == NULL)
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
			if (token == NULL)
			{
				qFatal("No IP/hostname given");
			}
			sstrcpy(iptoconnect, token);
			break;
		case CLI_HOSTLAUNCH:
			// go directly to host screen, bypass all others.
			hostlaunch = true;
			break;
		case CLI_GAME:
			// retrieve the game name
			token = poptGetOptArg(poptCon);
			if (token == NULL
			    || (strcmp(token, "CAM_1A") && strcmp(token, "CAM_2A") && strcmp(token, "CAM_3A")
			        && strcmp(token, "TUTORIAL3") && strcmp(token, "FASTPLAY")))
			{
				qFatal("The game parameter requires one of the following keywords:"
				       "CAM_1A, CAM_2A, CAM_3A, TUTORIAL3, or FASTPLAY.");
			}
			NetPlay.bComms = false;
			bMultiPlayer = false;
			bMultiMessages = false;
			NetPlay.players[0].allocated = true;
			if (!strcmp(token, "CAM_1A") || !strcmp(token, "CAM_2A") || !strcmp(token, "CAM_3A"))
			{
				game.type = CAMPAIGN;
			}
			else
			{
				game.type = SKIRMISH; // tutorial is skirmish for some reason
			}
			sstrcpy(aLevelName, token);
			SetGameMode(GS_NORMAL);
			break;
		case CLI_MOD_GLOB:
			{
				unsigned int i;

				// retrieve the file name
				token = poptGetOptArg(poptCon);
				if (token == NULL)
				{
					qFatal("Missing mod name?");
				}

				// Find an empty place in the global_mods list
				for (i = 0; i < 100 && global_mods[i] != NULL; ++i) {}
				if (i >= 100 || global_mods[i] != NULL)
				{
					qFatal("Too many mods registered! Aborting!");
				}
				global_mods[i] = strdup(token);
				break;
			}
		case CLI_MOD_CA:
			{
				unsigned int i;

				// retrieve the file name
				token = poptGetOptArg(poptCon);
				if (token == NULL)
				{
					qFatal("Missing mod name?");
				}

				// Find an empty place in the campaign_mods list
				for (i = 0; i < 100 && campaign_mods[i] != NULL; ++i) {}
				if (i >= 100 || campaign_mods[i] != NULL)
				{
					qFatal("Too many mods registered! Aborting!");
				}
				campaign_mods[i] = strdup(token);
				break;
			}
		case CLI_MOD_MP:
			{
				unsigned int i;

				// retrieve the file name
				token = poptGetOptArg(poptCon);
				if (token == NULL)
				{
					qFatal("Missing mod name?");
				}

				for (i = 0; i < 100 && multiplay_mods[i] != NULL; ++i) {}
				if (i >= 100 || multiplay_mods[i] != NULL)
				{
					qFatal("Too many mods registered! Aborting!");
				}
				multiplay_mods[i] = strdup(token);
				break;
			}
		case CLI_RESOLUTION:
			{
				unsigned int width, height;

				token = poptGetOptArg(poptCon);
				if (sscanf(token, "%ix%i", &width, &height) != 2)
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
			if (token == NULL)
			{
				qFatal("Unrecognised savegame name");
			}
			snprintf(saveGameName, sizeof(saveGameName), "%s/skirmish/%s.gam", SaveGamePath, token);
			SPinit();
			bMultiPlayer = true;
			game.type = SKIRMISH; // tutorial is skirmish for some reason
			SetGameMode(GS_SAVEGAMELOAD);
			break;
		case CLI_LOADCAMPAIGN:
			// retrieve the game name
			token = poptGetOptArg(poptCon);
			if (token == NULL)
			{
				qFatal("Unrecognised savegame name");
			}
			snprintf(saveGameName, sizeof(saveGameName), "%s/campaign/%s.gam", SaveGamePath, token);
			SPinit();
			SetGameMode(GS_SAVEGAMELOAD);
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
			wz_texture_compression = GL_COMPRESSED_RGBA_ARB;
			break;

		case CLI_NOTEXTURECOMPRESSION:
			wz_texture_compression = GL_RGBA;
			break;

		case CLI_AUTOGAME:
			wz_autogame = true;
			break;
		};
	}

	return true;
}

bool autogame_enabled()
{
	return wz_autogame;
}
