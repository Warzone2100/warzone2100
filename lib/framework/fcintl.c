/*
	This file is part of Warzone 2100.
	Copyright (C) 1996  A Kjeldberg, L Gregersen, P Unold (Freeciv, revision 7638)
	Copyright (C) 2007  Warzone Resurrection Project

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

#include "frame.h"
#include <string.h>

/********************************************************************** 
Some strings are ambiguous for translation.  For example, "Game" is
something you play (like Freeciv!) or animals that can be hunted.
To distinguish strings for translation, we qualify them with a prefix
string of the form "?qualifier:".  So, the above two cases might be:
  "Game"           -- when used as meaning something you play
  "?animals:Game"  -- when used as animales to be hunted
Notice that only the second is qualified; the first is processed in
the normal gettext() manner (as at most one ambiguous string can be).

This function tests for, and removes if found, the qualifier prefix part
of a string.

This function should only be called by the Q_() macro.  This macro also
should, if NLS is enabled, have called gettext() to get the argument
to pass to this function.
***********************************************************************/
const char *skip_intl_qualifier_prefix(const char *str)
{
	const char *ptr;

	if (*str != '?')
	{
		return str;
	}
	else if ((ptr = strchr(str, ':')))
	{
		return (ptr + 1);
	}
	else
	{
		return str;  // may be something wrong
	}
}
