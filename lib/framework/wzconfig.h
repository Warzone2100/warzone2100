/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#include <3rdparty/json/json.hpp>
using json = nlohmann::json;

#include <QtCore/QStringList>
#include <physfs.h>
#include <stdbool.h>
#include <vector>

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/framework/wzstring.h"


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

	json_variant(const QString & str);
	json_variant(const WzString & str);
public:
	// QVariant-like conversion methods
	int toInt(bool *ok = nullptr) const;
	uint toUInt(bool *ok = nullptr) const;
	bool toBool() const;
	double toDouble(bool *ok = nullptr) const;
	float toFloat(bool *ok = nullptr) const;
	WzString toWzString(bool *ok = nullptr) const;
	std::vector<WzString> toWzStringList() const;
	QString toString() const;
	QStringList toStringList() const;

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

	void beginArray(const WzString &name);
	void nextArrayItem();
	void endArray();
	int remainingArrayItems();

	bool beginGroup(const WzString &name);
	void endGroup();

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
};

// Enable JSON support for custom types

// QString
inline void to_json(json& j, const QString& p) {
	j = json(p.toUtf8().constData());
}

inline void from_json(const json& j, QString& p) {
	std::string str = j.get<std::string>();
	p = QString::fromUtf8(str.c_str());
}

// WzString
inline void to_json(json& j, const WzString& p) {
	j = json(p.toUtf8());
}

inline void from_json(const json& j, WzString& p) {
	std::string str = j.get<std::string>();
	p = WzString::fromUtf8(str.c_str());
}

// Convenience methods to retrieve a json_variant from any json object
json_variant json_getValue(const nlohmann::json& json, const WzString &key, const json_variant &defaultValue = json_variant());
json_variant json_getValue(const nlohmann::json& json, nlohmann::json::size_type idx, const json_variant &defaultValue = json_variant());

#endif
