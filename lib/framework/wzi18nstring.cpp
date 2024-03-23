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

#include "wzi18nstring.h"

#include "i18n.h"
#include "wzglobal.h"

WzI18nString::WzI18nString()
{
	_defaultLocale = DEFAULT_LOCALE;
	_strings[_defaultLocale] = WzString();
}


WzI18nString::WzI18nString(WzString string)
{
	_defaultLocale = DEFAULT_LOCALE;
	_strings[_defaultLocale] = string;
}

WzI18nString::WzI18nString(const WzI18nString& other)
{
	_defaultLocale = other._defaultLocale;
	for (const auto &locale : other.listLocales())
	{
		_strings[std::string(locale)] = WzString(other.getLocaleString(locale));
	}
}

WzI18nString::WzI18nString(WzConfig &ini, const WzString &key, const int maxChars)
{
	if (!ini.contains(key))
	{
		reset();
		return;
	}
	json_variant value = ini.value(key);
	if (value.jsonValue().is_string())
	{
		WzString string = value.toWzString();
		if (string.length() > maxChars)
		{
			string.truncate(maxChars);
			debug(LOG_WARNING, "%s: value for %s was truncated to fit into %d characters.", ini.fileName().toUtf8().c_str(), key.toUtf8().c_str(), maxChars);
		}
		_defaultLocale = DEFAULT_LOCALE;
		_strings[_defaultLocale] = string;
		return;
	}
	if (ini.beginGroup(key))
	{
		for (auto locale : getLocales())
		{
			std::string localeCode = std::string(locale.code);
			WzString string = ini.string(WzString::fromUtf8(localeCode));
			if (string.length() > maxChars)
			{
				string.truncate(maxChars);
				debug(LOG_WARNING, "%s: value for %s (%s) was truncated to fit into %d characters.", ini.fileName().toUtf8().c_str(), key.toUtf8().c_str(), localeCode.c_str(), maxChars);
			}
			if (!string.isEmpty())
			{
				_strings[localeCode] = string;
			}
		}
		_defaultLocale = DEFAULT_LOCALE;
		WzString altDefault = ini.string(WzString::fromUtf8("default"));
		if (!altDefault.isEmpty())
		{
			_defaultLocale = altDefault.toUtf8();
		}
		if (!this->contains(_defaultLocale))
		{
			debug(LOG_ERROR, "%s: no entry for %s in default language %s.", ini.fileName().toUtf8().c_str(), key.toUtf8().c_str(), _defaultLocale.c_str());
			reset();
		}
		ini.endGroup();
	}
}

bool WzI18nString::contains(std::string localeCode) const
{
	return _strings.find(localeCode) != _strings.end();
}

bool WzI18nString::isEmpty() const
{
	return this->getDefaultString().isEmpty();
}

const std::string& WzI18nString::getDefaultLocaleCode() const
{
	return _defaultLocale;
}

const WzString& WzI18nString::getDefaultString() const
{
	return _strings.find(_defaultLocale)->second;
}

const WzString& WzI18nString::getLocaleString() const
{
	return this->getLocaleString(getLanguage());
}

const WzString& WzI18nString::getLocaleString(const char* localeCode) const
{
	return this->getLocaleString(std::string(localeCode));
}

const WzString& WzI18nString::getLocaleString(std::string localeCode) const
{
	auto it = _strings.find(localeCode);
	if (it != _strings.end())
	{
		return it->second;
	}
	return _strings.find(_defaultLocale)->second;
}

std::vector<std::string> WzI18nString::listLocales() const
{
	std::vector<std::string> locales;
	for (auto it = _strings.begin(); it != _strings.end(); ++it)
	{
		locales.push_back(it->first);
	}
	return locales;
}

void WzI18nString::reset()
{
	_defaultLocale = DEFAULT_LOCALE;
	_strings.clear();
	_strings[_defaultLocale] = WzString();
}

void WzI18nString::reset(std::string &defaultLocale)
{
	_strings.clear();
	_defaultLocale = defaultLocale;
	_strings[_defaultLocale] = WzString();
}

void WzI18nString::reset(std::string &defaultLocale, WzString &text)
{
	_strings.clear();
	_defaultLocale = defaultLocale;
	_strings[_defaultLocale] = text;
}

void WzI18nString::setLocaleString(const char* localeCode, WzString text)
{
	_strings[localeCode] = text;
}

WzI18nString& WzI18nString::operator=(const WzI18nString& other)
{
	_defaultLocale = other._defaultLocale;
	for (const auto &locale : other.listLocales())
	{
		_strings[std::string(locale)] = WzString(other.getLocaleString(locale));
	}
	return *this;
}
