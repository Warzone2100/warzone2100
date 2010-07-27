/*
	This file is part of Warzone 2100.
	Copyright (C) 2010  Giel van Schijndel
	Copyright (C) 2010  Warzone 2100 Project

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

#ifndef __INCLUDED_X_MOTIF_MESSAGEBOX_H__
#define __INCLUDED_X_MOTIF_MESSAGEBOX_H__

#include "wzglobal.h"

#if defined(WZ_HAVE_MOTIF)
# define FRAME_LIB_INCLUDE
# include "types.h" 
# include "debug.h" 

extern void XmMessageBox(code_part part, const char* part_name, const char* msg);
#endif

#endif // __INCLUDED_X_MOTIF_MESSAGEBOX_H__
