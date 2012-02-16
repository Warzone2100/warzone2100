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

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDED_MACROS_H__
#define __INCLUDED_MACROS_H__

#define DXCALL(func) if( (ddrval = func) != DD_OK) { DisplayError(ddrval,__FILE__,__LINE__,0); DebugClose(); return FALSE; }
#define DXCALL_INT(func) if( (ddrval = func) != DD_OK) { DisplayError(ddrval,__FILE__,__LINE__,0); DebugClose(); return -1; }
#define DXCALL_HRESULT(func) if( (ddrval = func) != DD_OK) { DisplayError(ddrval,__FILE__,__LINE__,0); DebugClose(); return ddrval; }

#define DXCALL_RESTORE(func) if( (ddrval = func) != DD_OK) \
{ \
	if(ddrval == DDERR_SURFACELOST) { \
		RestoreSurfaces(); \
	} else { \
		DisplayError(ddrval,__FILE__,__LINE__,0); \
		DebugClose(); \
		return FALSE; \
	} \
}

#define RELEASE(x) if (x) { x->Release(); x = NULL; }

#define DELOBJ(x) if (x) { delete x; x = NULL; }

#endif // __INCLUDED_MACROS_H__
