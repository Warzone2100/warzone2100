/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <physfs.h>

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "lib/framework/frame.h"
#include "lib/framework/vector.h"

// QSettings is totally the wrong class to use for this, but it is so shiny!
// The amount of hacks needed are escalating. So clearly Something Needs To Be Done.
class WzConfigHack
{
public:
	WzConfigHack(const QString &fileName, int readOnly)
	{
		if (readOnly == 1 || PHYSFS_exists(fileName.toUtf8().constData())) return;
		if (readOnly == 0)
		{
			PHYSFS_file *fileHandle = PHYSFS_openWrite(fileName.toUtf8().constData());
			if (!fileHandle) debug(LOG_ERROR, "%s could not be created: %s", fileName.toUtf8().constData(), PHYSFS_getLastError());
			PHYSFS_close(fileHandle);
		}
		else if (readOnly == 2 && !PHYSFS_exists(fileName.toUtf8().constData()))
		{
			debug(LOG_FATAL, "Could not find required file \"%s\"", fileName.toUtf8().constData());
		}
	}
};

class WzConfig : private WzConfigHack
{
private:
	QSettings m_settings;
	QMap<QString,QVariant> m_overrides;
	
	QString slashedGroup() const 
	{
		if (m_settings.group() == "") 
			return "";
		return m_settings.group()+"/";
	}

public:
	enum warning { ReadAndWrite, ReadOnly, ReadOnlyAndRequired };
	WzConfig(const QString &name, WzConfig::warning warning = ReadAndWrite, QObject *parent = 0);

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

	void beginGroup(const QString &prefix)
	{
		m_settings.beginGroup(prefix);
	}
	void endGroup()
	{
		m_settings.endGroup();
	}
	QString fileName() const
	{
		return m_settings.fileName();
	}
	bool isWritable() const
	{
		return m_settings.isWritable();
	}
	void setValue(const QString &key, const QVariant &value) 
	{
		m_settings.setValue(key,value);
	}
	QSettings::Status status() const
	{
		return m_settings.status();
	}
	
};

#endif
