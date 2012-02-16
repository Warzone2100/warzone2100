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

#ifndef __INCLUDED_CHNKIO_H__
#define __INCLUDED_CHNKIO_H__

#define	CHECKTRUE(func) if((func)==FALSE) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }
#define	CHECKZERO(func) if((func)!=0) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }
#define	CHECKONE(func) if((func)!=1) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }
#define	CHECKNOTZERO(func) if((func)==0) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }


class CChnkIO {
public:
	void SceneDebugPrint(const TCHAR *format, ...);
	BOOL StartChunk(FILE *Stream,char *Name);
	BOOL EndChunk(FILE *Stream);
	BOOL ReadLong(FILE *Stream,char *Name,LONG *Long);
	BOOL ReadFloat(FILE *Stream,char *Name,float *Float);
	BOOL ReadString(FILE *Stream,char *Name,char *String);
	BOOL ReadQuotedString(FILE *Stream,char *Name,char *String);
	BOOL ReadStringAlloc(FILE *Stream,char *Name,char **String);
	BOOL ReadQuotedStringAlloc(FILE *Stream,char *Name,char **String);
};

#endif // __INCLUDED_CHNKIO_H__
