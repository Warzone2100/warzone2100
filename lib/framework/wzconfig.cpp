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

/** @file wzconfig.cpp
 *  Ini related functions.
 */

#include <QtCore/QJsonParseError>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonDocument>

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "wzconfig.h"
#include "file.h"

WzConfig::~WzConfig()
{
	if (mWarning == ReadAndWrite)
	{
		ASSERT(mObjStack.size() == 0, "Some json groups have not been closed, stack size %d.", mObjStack.size());
		QJsonDocument doc(mObj);
		QByteArray json = doc.toJson();
		saveFile(mFilename.toUtf8().constData(), json.constData(), json.size());
	}
}

static QJsonObject jsonMerge(QJsonObject original, const QJsonObject override)
{
	for (const QString &key : override.keys())
	{
		if (override[key].isObject() && original.contains(key))
		{
			original[key] = jsonMerge(original[key].toObject(), override[key].toObject());
		}
		else if (override[key].isNull())
		{
			original.remove(key);
		}
		else
		{
			original[key] = override[key];
		}
	}
	return original;
}

WzConfig::WzConfig(const QString &name, WzConfig::warning warning, QObject *parent)
{
	UDWORD size;
	char *data;
	QJsonParseError error;

	mFilename = name;
	mStatus = true;
	mWarning = warning;

	if (!PHYSFS_exists(name.toUtf8().constData()))
	{
		if (warning == ReadOnly)
		{
			mStatus = false;
			return;
		}
		else if (warning == ReadOnlyAndRequired)
		{
			debug(LOG_FATAL, "Missing required file %s", name.toUtf8().constData());
			abort();
		}
		else if (warning == ReadAndWrite)
		{
			return;
		}
	}
	if (!loadFile(name.toUtf8().constData(), &data, &size))
	{
		debug(LOG_FATAL, "Could not open \"%s\"", name.toUtf8().constData());
	}
	QJsonDocument mJson = QJsonDocument::fromJson(QByteArray(data, size), &error);
	ASSERT(!mJson.isNull(), "JSON document from %s is invalid: %s", name.toUtf8().constData(), error.errorString().toUtf8().constData());
	ASSERT(mJson.isObject(), "JSON document from %s is not an object. Read: \n%s", name.toUtf8().constData(), data);
	mObj = mJson.object();
	free(data);
	char **diffList = PHYSFS_enumerateFiles("diffs");
	for (char **i = diffList; *i != NULL; i++)
	{
		std::string str(std::string("diffs/") + *i + std::string("/") + name.toUtf8().constData());
		if (!PHYSFS_exists(str.c_str()))
		{
			continue;
		}
		if (!loadFile(str.c_str(), &data, &size))
		{
			debug(LOG_FATAL, "jsondiff file \"%s\" could not be opened!", name.toUtf8().constData());
		}
		QJsonDocument tmpJson = QJsonDocument::fromJson(QByteArray(data, size), &error);
		ASSERT(!tmpJson.isNull(), "JSON diff from %s is invalid: %s", name.toUtf8().constData(), error.errorString().toUtf8().constData());
		ASSERT(tmpJson.isObject(), "JSON diff from %s is not an object. Read: \n%s", name.toUtf8().constData(), data);
		QJsonObject tmpObj = tmpJson.object();
		mObj = jsonMerge(mObj, tmpObj);
		free(data);
		debug(LOG_INFO, "jsondiff \"%s\" loaded and merged", str.c_str());
	}
	PHYSFS_freeList(diffList);
}

QStringList WzConfig::childGroups() const
{
	return mObj.keys();
}

QStringList WzConfig::childKeys() const
{
	return mObj.keys();
}

bool WzConfig::contains(const QString &key) const
{
	return mObj.contains(key);
}

QVariant WzConfig::value(const QString &key, const QVariant &defaultValue) const
{
	if (!contains(key))
	{
		return defaultValue;
	}
	else
	{
		return mObj.value(key).toVariant();
	}
}

QJsonValue WzConfig::json(const QString &key, const QJsonValue &defaultValue) const
{
	if (!contains(key))
	{
		return defaultValue;
	}
	else
	{
		return mObj.value(key);
	}
}

void WzConfig::setVector3f(const QString &name, const Vector3f &v)
{
	QStringList l;
	l.push_back(QString::number(v.x));
	l.push_back(QString::number(v.y));
	l.push_back(QString::number(v.z));
	setValue(name, l);
}

Vector3f WzConfig::vector3f(const QString &name)
{
	Vector3f r(0.0, 0.0, 0.0);
	if (!contains(name))
	{
		return r;
	}
	QStringList v = value(name).toStringList();
	ASSERT(v.size() == 3, "Bad list of %s", name.toUtf8().constData());
	r.x = v[0].toDouble();
	r.y = v[1].toDouble();
	r.z = v[2].toDouble();
	return r;
}

void WzConfig::setVector3i(const QString &name, const Vector3i &v)
{
	QStringList l;
	l.push_back(QString::number(v.x));
	l.push_back(QString::number(v.y));
	l.push_back(QString::number(v.z));
	setValue(name, l);
}

Vector3i WzConfig::vector3i(const QString &name)
{
	Vector3i r(0, 0, 0);
	if (!contains(name))
	{
		return r;
	}
	QStringList v = value(name).toStringList();
	ASSERT(v.size() == 3, "Bad list of %s", name.toUtf8().constData());
	r.x = v[0].toInt();
	r.y = v[1].toInt();
	r.z = v[2].toInt();
	return r;
}

void WzConfig::setVector2i(const QString &name, const Vector2i &v)
{
	QStringList l;
	l.push_back(QString::number(v.x));
	l.push_back(QString::number(v.y));
	setValue(name, l);
}

Vector2i WzConfig::vector2i(const QString &name)
{
	Vector2i r(0, 0);
	if (!contains(name))
	{
		return r;
	}
	QStringList v = value(name).toStringList();
	ASSERT(v.size() == 2, "Bad list of %s", name.toUtf8().constData());
	r.x = v[0].toInt();
	r.y = v[1].toInt();
	return r;
}

bool WzConfig::beginGroup(const QString &prefix)
{
	mObjNameStack.append(mName);
	mObjStack.append(mObj);
	mName = prefix;
	if (mWarning == ReadAndWrite)
	{
		mObj = QJsonObject();
	}
	else
	{
		if (!contains(prefix)) // handled in this way for backwards compatibility
		{
			mObj = QJsonObject();
			return false;
		}
		QJsonValue value = mObj.value(prefix);
		ASSERT(value.isObject(), "beginGroup() on non-object key \"%s\"", prefix.toUtf8().constData());
		mObj = value.toObject();
	}
	return true;
}

void WzConfig::endGroup()
{
	ASSERT(mObjStack.size() > 0, "An endGroup() too much!");
	if (mWarning == ReadAndWrite)
	{
		QJsonObject latestObj = mObj;
		mObj = mObjStack.takeLast();
		mObj[mName] = latestObj;
		mName = mObjNameStack.takeLast();
	}
	else
	{
		mName = mObjNameStack.takeLast();
		mObj = mObjStack.takeLast();
	}
}

void WzConfig::setValue(const QString &key, const QVariant &value)
{
	mObj.insert(key, QJsonValue::fromVariant(value));
}
