/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#define	TARGET_TYPE_NONE		0x0000
#define	TARGET_TYPE_THREAT		0x0001
#define	TARGET_TYPE_WALL		0x0002
#define	TARGET_TYPE_STRUCTURE	0x0004
#define	TARGET_TYPE_DROID		0x0008
#define	TARGET_TYPE_FEATURE		0x0010
#define	TARGET_TYPE_ARTIFACT	0x0020
#define	TARGET_TYPE_RESOURCE	0x0040
#define TARGET_TYPE_FRIEND		0x0080
#define	TARGET_TYPE_ANY			0xffff

void targetInitialise(void);
void targetOpenList(BASE_OBJECT *psTargeting);
void targetCloseList(void);
void targetSetTargetable(UWORD DroidType);
void targetAdd(BASE_OBJECT *psObj);
void targetKilledObject(BASE_OBJECT *psObj);
BASE_OBJECT *targetAquireNew(void);
BASE_OBJECT *targetAquireNearestScreen(SWORD x,SWORD y,UWORD TargetType);
BASE_OBJECT *targetAquireNearestView(SWORD x,SWORD y,UWORD TargetType);
BASE_OBJECT *targetAquireNearestObj(BASE_OBJECT *psObj,UWORD TargetType);
BASE_OBJECT *targetAquireNearestObjView(BASE_OBJECT *psObj,UWORD TargetType);
BASE_OBJECT *targetAquireNext(UWORD TargetType);
BASE_OBJECT *targetGetCurrent(void);
void targetStartAnim(void);
void targetMarkCurrent(void);
