/*
	This file is part of Warzone 2100.
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
#ifndef _i18n_h
#define _i18n_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#  error Framework header files MUST be included from frame.h ONLY.
#endif


#include "gettext.h"

// Enable NLS for our parsers when it's enabled for us
#define YYENABLE_NLS ENABLE_NLS


// MSVC doesn't have it, and gettext's wonderful design needs it.
#if !defined(LC_MESSAGES)
#  define LC_MESSAGES 0
#endif


#define _(String) gettext(String)
#define N_(String) gettext_noop(String)

// Context sensitive strings
#define P_(Context, String) pgettext(Context, String)
// Non literal context sensitive strings
#define PE_(Context, String) pgettext_expr(Context, String)
// Make xgettext recognize the context
#define NP_(Context, String) gettext_noop(String)

WZ_DECL_PURE const char *getLanguage();
WZ_DECL_PURE const char *getLanguageName();
WZ_DECL_NONNULL(1) bool setLanguage(const char *name);
void setNextLanguage();
void initI18n();

#endif // _i18n_h
