/*
 This file is part of Warzone 2100.
 Copyright (C) 1999-2004  Eidos Interactive
 Copyright (C) 2005-2011  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_FRAMEWORK_COCOA_WRAPPER_H__
#define __INCLUDED_LIB_FRAMEWORK_COCOA_WRAPPER_H__

#include "wzglobal.h"

#ifdef WZ_OS_MAC

void cocoaInit(void);

/*!
 * Display an alert dialog.
 * This blocks until the dialog is dismissed.
 * \param message Summary of the issue
 * \param information A more detailed explanation of the issue
 * \param style 0 is a warning, 1 is informational, and 2 is critical. (NSAlertStyle)
 */
void cocoaShowAlert(const char *message, const char *information, unsigned style);

#endif // WZ_OS_MAC

#ifdef WZ_WS_MAC
#include <QtCore/QList>
#include <QtCore/QSize>

/*!
 * Appends available screen resolutions from every connected screen to
 * the given list that are larger than the given minimum size and match
 * a screen's current bit depth and refresh rate.
 *
 * \param resolutions A list to append resolution sizes to
 * \param minWidth
 * \param minHeight
 * \return true on success, false otherwise
 */
bool cocoaAppendAvailableScreenResolutions(QList<QSize> &resolutions, int minWidth, int minHeight);

#endif // WZ_WS_MAC

#endif // __INCLUDED_LIB_FRAMEWORK_COCOA_WRAPPER_H__
