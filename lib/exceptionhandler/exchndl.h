/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Giel van Schijndel
	Copyright (C) 2008-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_EXCEPTIONHANDLER_EXCHNDL_H__
#define __INCLUDED_LIB_EXCEPTIONHANDLER_EXCHNDL_H__

void ExchndlSetup(const char *packageVersion, const std::string &writeDir, bool portable_mode);
void ExchndlShutdown();
void ResetRPTDirectory(wchar_t *newPath);

#endif // __INCLUDED_LIB_EXCEPTIONHANDLER_EXCHNDL_H__
