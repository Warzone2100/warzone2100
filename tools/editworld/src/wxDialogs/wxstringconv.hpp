/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2009  Warzone Resurrection Project

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

	$Revision$
	$Id$
	$HeadURL$
*/

#include <string>
#include <wx/string.h>

/** Converts a wxWidgets' wxString to an STL std::string
 */
inline std::string wx2stdString(const wxString& rhs)
{
	return std::string(rhs.mb_str(wxConvUTF8));
}

/** Converts an STL's std::string to a wxWidgets wxString
 */
inline wxString std2wxString(const std::string& rhs)
{
	return wxString(rhs.c_str(), wxConvUTF8, rhs.length());
}
