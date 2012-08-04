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

#ifndef __INCLUDED_TILETYPES_H__
#define __INCLUDED_TILETYPES_H__

//#include "ddbiew.h"
//#include "ddimage.h"
#include "dibdraw.h"

/*
 Tile flags.

 11111100 00000000
 54321098 76543210
 tttttttt .grryxvh
// tttttttt grryxfvh

  rr	Texture rotation 0=0, 1=90, 2=180, 3=270.
  v		Vertex ( triangle direction ) flip.
  y		Y texture flip.
  x     X texture flip.
  f     Diagonal flip.
  h     Hide.
  g		Is gateway.
  t		Tile type ie Grass,Sand etc...
  .		Spare.
*/

#define TF_SHOW			0
#define	TF_HIDE			1
#define TF_VERTEXFLIP	2
#define TF_GEOMETRYMASK 0x0007		// Surely this is wrong..
#define TF_TEXTUREFLIPX	4

#define TF_TEXTUREFLIPY	8
#define TF_TEXTUREROTMASK	0x30
#define TF_TEXTUREROTSHIFT	4
#define TF_TEXTUREROT90		(1<<TF_TEXTUREROTSHIFT)	//16 -> 32
#define TF_TEXTUREROT180	(2<<TF_TEXTUREROTSHIFT)	//16 -> 32
#define	TF_TEXTUREROT270	(3<<TF_TEXTUREROTSHIFT)	//16 -> 32
#define TF_TEXTUREGATEWAY	64
#define TF_TEXTUREMASK      0x00f8

//0000 0001
//0000 1362
//1248 6248
//vfxy rrss

//#define TF_TYPEMASK			0xff00
//#define TF_TYPESHIFT        8
//#define TF_TYPEGRASS		256
//#define TF_TYPESTONE		512
//#define TF_TYPESAND			1024
//#define TF_TYPEWATER		2048
//#define TF_TYPESPARE1		4096
//#define TF_TYPESPARE2		8192
//#define TF_TYPESPARE3		16384
//#define TF_TYPEALL			0xff00

#define TF_TYPESAND			0
#define TF_TYPESANDYBRUSH	1
#define TF_TYPEBAKEDEARTH	2
#define TF_TYPEGREENMUD		3
#define TF_TYPEREDBRUSH		4
#define TF_TYPEPINKROCK		5
#define TF_TYPEROAD			6
#define TF_TYPEWATER		7
#define TF_TYPECLIFFFACE	8
#define TF_TYPERUBBLE		9
#define TF_TYPESHEETICE		10
#define TF_TYPESLUSH 		11

#define TF_NUMTYPES			12
#define TF_DEFAULTTYPE		TF_TYPEBAKEDEARTH

void DisplayTerrainType(CDIBDraw *DIBDraw,int XPos,int YPos,int Width,int Height,DWORD Flags);

#endif // __INCLUDED_TILETYPES_H__
