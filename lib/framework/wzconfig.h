/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
	WzConfigHack(const QString &fileName)
	{
		if (PHYSFS_exists(fileName.toUtf8().constData())) return;
		PHYSFS_file *fileHandle = PHYSFS_openWrite(fileName.toUtf8().constData());
		if (!fileHandle) debug(LOG_ERROR, "%s could not be created: %s", fileName.toUtf8().constData(), PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
	}
};

class WzConfig : private WzConfigHack, public QSettings
{
public:
	WzConfig(const QString &name, QObject *parent = 0) : WzConfigHack(name), QSettings(QString("wz::") + name, QSettings::IniFormat, parent) {}
	Vector3f vector3f(const QString &name);
	void setVector3f(const QString &name, const Vector3f &v);
	Vector3i vector3i(const QString &name);
	void setVector3i(const QString &name, const Vector3i &v);
	Vector2i vector2i(const QString &name);
	void setVector2i(const QString &name, const Vector2i &v);
};

#endif
