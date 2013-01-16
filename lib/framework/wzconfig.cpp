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
