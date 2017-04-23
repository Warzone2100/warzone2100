/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2017  Warzone 2100 Project

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
/**
 * @file macosx_screen_resolutions.h
 *
 *
 */
#ifndef QTGAME_MACOSX_SCREEN_RESOLUTIONS_H
#define QTGAME_MACOSX_SCREEN_RESOLUTIONS_H

#include "lib/framework/wzglobal.h"

#ifdef WZ_WS_MAC
#include <QtCore/QList>
#include <QtCore/QSize>
#include <QtCore/QPoint>

/*!
 * Appends available screen resolutions for the main window's screen
 * to the given list. Resolutions that are smaller than the given
 * minimum size or are not available for the current bit depth and
 * refresh rate are omitted.
 *
 * \param resolutions A list to append resolution sizes to
 * \param minSize Filter out resolutions smaller than this size
 * \return true on success, false otherwise
 */
bool macosxAppendAvailableScreenResolutions(QList<QSize> &resolutions, QSize minSize, QPoint screenPoint);

/*!
 * Sets the resolution of the main window's screen.
 *
 * \param resolution The target resolution size
 * \return true on success, false otherwise
 */
bool macosxSetScreenResolution(QSize resolution, QPoint screenPoint);

#endif // WZ_WS_MAC

#endif // QTGAME_MACOSX_SCREEN_RESOLUTIONS_H
