/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2024  Warzone 2100 Project
 *
 *	Warzone 2100 is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Warzone 2100 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Warzone 2100; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _LIB_FRAMEWORK_WZI18NSTRING_H
#define _LIB_FRAMEWORK_WZI18NSTRING_H

#include <string>
#include <map>
#include <vector>
#include "lib/framework/wzstring.h"
#include "lib/framework/wzconfig.h"

/**
 * User provided multilingual string.
 */
class WzI18nString {
public:
	WzI18nString();
	WzI18nString(WzString string);
	WzI18nString(std::string defaultLocale, std::map<std::string, WzString> strings);
	WzI18nString(WzConfig &ini, const WzString &key, const int maxChars);
	WzI18nString(const WzI18nString& other);

	bool contains(std::string localeCode) const;
	bool isEmpty() const;
	const std::string& getDefaultLocaleCode() const;
	const WzString& getDefaultString() const;
	const WzString& getLocaleString() const;
	/** Get the string in the given locale or the default one if not set. */
	const WzString& getLocaleString(const char* localeCode) const;
	const WzString& getLocaleString(std::string localeCode) const;
	std::vector<std::string> listLocales() const;
public:
	void reset();
	void reset(std::string &defaultlocale);
	void reset(std::string &defaultLocale, WzString &text);
	void setLocaleString(const char* localeCode, WzString text);
public:
	WzI18nString& operator=(const WzI18nString& other);
private:
	std::string _defaultLocale;
	std::map<std::string, WzString> _strings;
};

#endif // _LIB_FRAMEWORK_WZI18NSTRING_H
