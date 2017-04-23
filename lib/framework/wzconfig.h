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

#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <physfs.h>
#include <stdbool.h>

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "lib/framework/frame.h"
#include "lib/framework/vector.h"

class WzConfig
{
public:
	enum warning { ReadAndWrite, ReadOnly, ReadOnlyAndRequired };

private:
	QJsonObject mObj;
	QJsonArray mArray;
	QString mName;
	QList<QJsonObject> mObjStack;
	QStringList mObjNameStack;
	QString mFilename;
	bool mStatus;
	warning mWarning;

public:
	WzConfig(const QString &name, WzConfig::warning warning, QObject *parent = 0);
	~WzConfig();

	Vector3f vector3f(const QString &name);
	void setVector3f(const QString &name, const Vector3f &v);
	Vector3i vector3i(const QString &name);
	void setVector3i(const QString &name, const Vector3i &v);
	Vector2i vector2i(const QString &name);
	void setVector2i(const QString &name, const Vector2i &v);

	QStringList childGroups() const;
	QStringList childKeys() const;
	bool contains(const QString &key) const;
	QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
	QJsonValue json(const QString &key, const QJsonValue &defaultValue = QJsonValue()) const;

	void beginArray(const QString &name);
	void nextArrayItem();
	void endArray();
	int remainingArrayItems();

	bool beginGroup(const QString &name);
	void endGroup();

	QString fileName() const
	{
		return mFilename;
	}

	bool isWritable() const
	{
		return mWarning == ReadAndWrite && mStatus;
	}

	void setValue(const QString &key, const QVariant &value);

	QString group()
	{
		return mName;
	}

	bool status()
	{
		return mStatus;
	}
};

#endif
