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
#ifndef WZCONFIG_H
#define WZCONFIG_H

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/framework/wzstring.h"

#if (defined(WZ_OS_WIN) && defined(WZ_CC_MINGW))
#  if (defined(snprintf) && !defined(_GL_STDIO_H) && defined(_LIBINTL_H))
// On mingw / MXE builds, libintl's define of snprintf breaks json.hpp
// So undef it here and restore it later (HACK)
#    define _wz_restore_libintl_snprintf
#    undef snprintf
#  endif
#endif

#if defined(__clang__) && defined(__has_cpp_attribute)
#  pragma clang diagnostic push
#  if __has_cpp_attribute(nodiscard)
#    // Work around json.hpp / Clang issue where Clang reports the nodiscard attribute is available in C++11 mode,
#    // but then `-Wc++17-extensions` warns that it is a C++17 feature
#    pragma clang diagnostic ignored "-Wc++1z-extensions" // -Wc++1z-extensions // -Wc++17-extensions
#  endif
#endif

#include <3rdparty/json/json.hpp>
using json = nlohmann::json;

#if defined(__clang__) && defined(__has_cpp_attribute)
#  pragma clang diagnostic pop
#endif

#if defined(_wz_restore_libintl_snprintf)
#  undef _wz_restore_libintl_snprintf
#  undef snprintf
#  define snprintf libintl_snprintf
#endif

#include <stdbool.h>
#include <vector>
#include <list>

class json_variant {
	// Wraps a json object and provides conversion methods that conform to older (QVariant) syntax and behavior
public:
	json_variant(const nlohmann::json& jObj)
	: mObj(jObj)
	{ }

	json_variant()
	// initializes to null json value
	{ }

	json_variant(int i);
	json_variant(const char * str);
	json_variant(const std::string & str);

	json_variant(const WzString & str);
public:
	// QVariant-like conversion methods
	int toInt(bool *ok = nullptr) const;
	unsigned int toUInt(bool *ok = nullptr) const;
	bool toBool() const;
	double toDouble(bool *ok = nullptr) const;
	float toFloat(bool *ok = nullptr) const;
	WzString toWzString(bool *ok = nullptr) const;
	std::vector<WzString> toWzStringList() const;

public:
	const nlohmann::json& jsonValue() const;

private:
	nlohmann::json mObj;
};

class WzConfig
{
public:
	enum warning { ReadAndWrite, ReadOnly, ReadOnlyAndRequired };

private:
	nlohmann::json mRoot = nlohmann::json::object();
	nlohmann::json *pCurrentObj = nullptr; // the "current" json object
	nlohmann::json mArray;
	WzString mName;
	std::list<nlohmann::json *> mObjStack;
	std::list<nlohmann::json> mNewObjStack; // stores newly-created objects (before they are added to root)
	std::list<WzString> mObjNameStack;
	WzString mFilename;
	bool mStatus;
	warning mWarning;

public:
	WzConfig(const WzString &name, WzConfig::warning warning);
	~WzConfig();

	Vector3f vector3f(const WzString &name);
	void setVector3f(const WzString &name, const Vector3f &v);
	Vector3i vector3i(const WzString &name);
	void setVector3i(const WzString &name, const Vector3i &v);
	Vector2i vector2i(const WzString &name);
	void setVector2i(const WzString &name, const Vector2i &v);

	std::vector<WzString> childGroups() const;
	std::vector<WzString> childKeys() const;
	bool contains(const WzString &key) const;
	json_variant value(const WzString &key, const json_variant &defaultValue = json_variant()) const;
	nlohmann::json json(const WzString &key, const nlohmann::json &defaultValue = nlohmann::json()) const;
	WzString string(const WzString &key, const WzString &defaultValue = WzString()) const;

	nlohmann::json& currentJsonValue() const;

	void beginArray(const WzString &name);
	void nextArrayItem();
	void endArray();
	int remainingArrayItems();

	bool beginGroup(const WzString &name);
	void endGroup();

	bool isAtDocumentRoot() const;

	WzString fileName() const
	{
		return mFilename;
	}

	bool isWritable() const
	{
		return mWarning == ReadAndWrite && mStatus;
	}

	void setValue(const WzString &key, const nlohmann::json &value);
	void set(const WzString &key, const nlohmann::json &value);

	WzString group()
	{
		return mName;
	}

	bool status()
	{
		return mStatus;
	}

	std::string compactStringRepresentation(const bool ensure_ascii = false) const;
};

// Enable JSON support for custom types

// WzString
inline void to_json(json& j, const WzString& p) {
	j = json(p.toUtf8());
}

inline void from_json(const json& j, WzString& p) {
	std::string str = j.get<std::string>();
	p = WzString::fromUtf8(str.c_str());
}

namespace glm
{
// Vector2i
inline void to_json(json& j, const ivec2 &v)
{
	j = nlohmann::json::array({ v.x, v.y });
}

inline void from_json(const json& j, ivec2& r)
{
	ASSERT(j.size() == 2, "Bad Vector2i list");
	try {
		r.x = j.at(0).get<int>();
		r.y = j.at(1).get<int>();
	}
	catch (const std::exception &e) {
		ASSERT(false, "Bad Vector2i list; exception: %s", e.what());
	}
	catch (...) {
		debug(LOG_FATAL, "Unexpected exception encountered reading Vector2i");
	}
}

// Vector3i
inline void to_json(json& j, const ivec3 &v)
{
	j = nlohmann::json::array({ v.x, v.y, v.z });
}

inline void from_json(const json& j, ivec3& r)
{
	ASSERT(j.size() == 3, "Bad Vector3i list");
	try {
		r.x = j.at(0).get<int>();
		r.y = j.at(1).get<int>();
		r.z = j.at(2).get<int>();
	}
	catch (const std::exception &e) {
		ASSERT(false, "Bad Vector3i list; exception: %s", e.what());
	}
	catch (...) {
		debug(LOG_FATAL, "Unexpected exception encountered in Vector3i");
	}
}
} // namespace glm

// Convenience methods to retrieve a json_variant from any json object
json_variant json_getValue(const nlohmann::json& json, const WzString &key, const json_variant &defaultValue = json_variant());
json_variant json_getValue(const nlohmann::json& json, nlohmann::json::size_type idx, const json_variant &defaultValue = json_variant());

#endif
