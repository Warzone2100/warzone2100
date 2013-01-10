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

/** @file wzconfig.cpp
 *  Ini related functions.
 */

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "wzconfig.h"

WzConfig::WzConfig(const QString &name, WzConfig::warning warning, QObject *parent)
	: WzConfigHack(name, (int)warning), m_settings(QString("wz::") + name, QSettings::IniFormat, parent), m_overrides()
{
	if (m_settings.status() != QSettings::NoError && (warning != ReadOnly || PHYSFS_exists(name.toUtf8().constData())))
	{
		debug(LOG_FATAL, "Could not open \"%s\"", name.toUtf8().constData());
	}
	char **diffList = PHYSFS_enumerateFiles("diffs");
	char **i;
	int j;
	for (i = diffList; *i != NULL; i++) 
	{
		std::string str(std::string("diffs/") + *i + std::string("/") + name.toUtf8().constData());
		if (!PHYSFS_exists(str.c_str())) 
			continue;
		QSettings tmp(QString("wz::") + str.c_str(), QSettings::IniFormat);
		if (tmp.status() != QSettings::NoError)
		{
			debug(LOG_FATAL, "Could not read an override for \"%s\"", name.toUtf8().constData());
		}
		QStringList keys(tmp.allKeys());
		for (j = 0; j < keys.length(); j++)
		{
			m_overrides.insert(keys[j],tmp.value(keys[j]));
		}
	}
	PHYSFS_freeList(diffList);
}

QStringList WzConfig::childGroups() const
{
	QStringList ret(m_settings.childGroups());
	int i,j;
	QStringList keys(m_overrides.keys());
	QString group = slashedGroup();
	for (i = 0; i < keys.length(); i++)
	{
		if (!keys[i].startsWith(group)) 
			continue;
		keys[i].remove(0, group.length());
		j = keys[i].indexOf("/");
		if (j == -1)
			continue;
		keys[i].truncate(j);
		if (ret.contains(keys[i]))
			continue;
		ret.append(keys[i]);
	}
	return ret;
}

QStringList WzConfig::childKeys() const
{
	QStringList ret(m_settings.childKeys());
	int i;
	QStringList keys(m_overrides.keys());
	QString group = slashedGroup();
	for (i = 0; i < keys.length(); i++)
	{
		if (!keys[i].startsWith(group)) 
			continue;
		keys[i].remove(0, group.length());
		if (keys[i].indexOf("/") > -1)
			continue;
		if (ret.contains(keys[i]))
			continue;
		ret.append(keys[i]);
	}
	return ret;
}

bool WzConfig::contains(const QString &key) const
{
	if (m_overrides.contains(slashedGroup() + key))
	{
		return true;
	}
	return m_settings.contains(key);
}

QVariant WzConfig::value(const QString &key, const QVariant &defaultValue) const
{
	if (m_overrides.contains(slashedGroup() + key))
	{
		return m_overrides.value(slashedGroup() + key);
	}
	return m_settings.value(key,defaultValue);
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
	if (!contains(name)) return r;
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
	if (!contains(name)) return r;
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
	if (!contains(name)) return r;
	QStringList v = value(name).toStringList();
	ASSERT(v.size() == 2, "Bad list of %s", name.toUtf8().constData());
	r.x = v[0].toInt();
	r.y = v[1].toInt();
	return r;
}
