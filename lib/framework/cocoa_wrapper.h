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

#ifndef __INCLUDED_LIB_FRAMEWORK_COCOA_WRAPPER_H__
#define __INCLUDED_LIB_FRAMEWORK_COCOA_WRAPPER_H__

#include "wzglobal.h"

#if defined(WZ_OS_MAC) && !defined(WZ_OS_IOS)

bool cocoaSelectFileInFinder(const char *filename);
bool cocoaSelectFolderInFinder(const char* path);
bool cocoaOpenURL(const char *url);
bool cocoaOpenUserCrashReportFolder();

bool cocoaGetApplicationSupportDir(char *const tmpstr, size_t const size);

bool cocoaSetFileQuarantineAttribute(const char *path);

bool cocoaTransformToBackgroundApplication();

bool cocoaIsRunningOnMacOSAtLeastVersion(unsigned major, unsigned minor);

#endif // WZ_OS_MAC && !WZ_OS_IOS

#if defined(WZ_OS_IOS)

bool cocoaGetIOSDocumentsDir(char *const tmpstr, size_t const size);
bool cocoaOpenFolderInFiles(const char* path);
bool cocoaIOSIsPhone();

#endif // WZ_OS_IOS

#endif // __INCLUDED_LIB_FRAMEWORK_COCOA_WRAPPER_H__
