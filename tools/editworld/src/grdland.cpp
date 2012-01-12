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

//#include "stdafx.h"
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "typedefs.h"
#include "debugprint.hpp"

#include "grdland.h"

#define GRDLANDVERSION 4

DWORD Version;

CGrdTileIO::CGrdTileIO(void)
{
}

CGrdTileIO::~CGrdTileIO(void)
{
}

void CGrdTileIO::SetTextureID(DWORD TMapID)
{
	m_TMapID = TMapID;
}

void CGrdTileIO::SetVertexFlip(DWORD VertexFlip)
{
	m_VertexFlip = VertexFlip;
}

void CGrdTileIO::SetTextureFlip(DWORD TextureFlip)
{
	m_TextureFlip = TextureFlip;
}

void CGrdTileIO::SetVertexHeight(DWORD Index,float y)
{
	ASSERT(Index<4);
	m_Height[Index] = y;
}

BOOL CGrdTileIO::Write(FILE *Stream)
{
	fprintf(Stream,"        TID %d VF %d TF %d ",m_TMapID,m_VertexFlip,m_TextureFlip);
	fprintf(Stream,"F %d ",m_Flags);
	fprintf(Stream,"VH %d %d %d %d\n",(DWORD)m_Height[0],(DWORD)m_Height[1],
									(DWORD)m_Height[2],(DWORD)m_Height[3]);

	return	TRUE;
}

DWORD CGrdTileIO::GetTextureID(void)
{
	return m_TMapID;
}

DWORD CGrdTileIO::GetVertexFlip(void)
{
	return m_VertexFlip;
}

DWORD CGrdTileIO::GetTextureFlip(void)
{
	return m_TextureFlip;
}

float CGrdTileIO::GetVertexHeight(DWORD Index)
{
	ASSERT(Index < 4);

	return m_Height[Index];
}

BOOL CGrdTileIO::Read(FILE *Stream)
{
	SLONG tmp;

	CHECKTRUE(ReadLong(Stream,"TID",(LONG*)&m_TMapID));
	CHECKTRUE(ReadLong(Stream,"VF",(LONG*)&m_VertexFlip));
	CHECKTRUE(ReadLong(Stream,"TF",(LONG*)&m_TextureFlip));
	if(Version >= 1) {
		CHECKTRUE(ReadLong(Stream,"F",(LONG*)&m_Flags));
	} else {
		m_Flags = 0;
	}
	CHECKTRUE(ReadLong(Stream,"VH",&tmp));
	m_Height[0]=(float)tmp;
	CHECKTRUE(ReadLong(Stream,NULL,&tmp));
	m_Height[1]=(float)tmp;
	CHECKTRUE(ReadLong(Stream,NULL,&tmp));
	m_Height[2]=(float)tmp;
	CHECKTRUE(ReadLong(Stream,NULL,&tmp));
	m_Height[3]=(float)tmp;
	
	return	TRUE;
}


CGrdLandIO::CGrdLandIO(void)
{
	m_TextureNames = NULL;
	m_Tiles = NULL;
	Version = GRDLANDVERSION;
	m_Version = Version;
}

CGrdLandIO::~CGrdLandIO(void)
{
	DWORD i;

	if(m_TextureNames) {
		for(i=0; i<m_NumTextures; i++) {
			if(m_TextureNames[i]) delete m_TextureNames[i];
		}
		delete m_TextureNames;
	}

	if(m_Tiles) {
		for(i=0; i<m_NumTiles; i++) {
			if(m_Tiles[i]) delete m_Tiles[i];
		}
		delete m_Tiles;
	}
}

void CGrdLandIO::SetTextureSize(DWORD TextureWidth,DWORD TextureHeight)
{
	m_TextureWidth = TextureWidth;
	m_TextureHeight = TextureHeight;
}

void CGrdLandIO::SetCameraPosition(float x,float y,float z)
{
	m_CameraXPos = x;
	m_CameraYPos = y;
	m_CameraZPos = z;
}

void CGrdLandIO::SetCameraRotation(float x,float y,float z)
{
	m_CameraXRot = x;
	m_CameraYRot = y;
	m_CameraZRot = z;
}

void CGrdLandIO::Set2DPosition(SLONG ScrollX,SLONG ScrollY)
{
	m_ScrollX = ScrollX;
	m_ScrollY = ScrollY;
}

void CGrdLandIO::SetHeightScale(DWORD HeightScale)
{
	m_HeightScale = HeightScale;
}

void CGrdLandIO::SetMapSize(DWORD MapWidth,DWORD MapHeight)
{
	DWORD	i;

	m_MapWidth = MapWidth;
	m_MapHeight = MapHeight;
	m_NumTiles = m_MapWidth*m_MapHeight;
	m_Tiles = new CGrdTileIO*[m_NumTiles];
	for(i=0; i<m_NumTiles; i++) {
		m_Tiles[i] = new CGrdTileIO;
	}
}

void CGrdLandIO::SetTileSize(DWORD TileWidth,DWORD TileHeight)
{
	m_TileWidth = TileWidth;
	m_TileHeight = TileHeight;
}

CGrdTileIO *CGrdLandIO::GetTile(DWORD Index)
{
	ASSERT(Index < m_NumTiles);

	return m_Tiles[Index];
}

void CGrdLandIO::SetNumTextures(DWORD NumTextures)
{
	DWORD i;

	m_NumTextures = NumTextures;
	m_TextureNames = new char*[m_NumTextures];

	for(i=0; i<m_NumTextures; i++) {
		m_TextureNames[i] = NULL;
	}
}

void CGrdLandIO::SetTextureName(DWORD NameIndex,char *TextureName)
{
	ASSERT(NameIndex < m_NumTextures);

	m_TextureNames[NameIndex] = new char[strlen(TextureName)+1];
	strcpy(m_TextureNames[NameIndex],TextureName);
}

BOOL CGrdLandIO::Write(FILE *Stream)
{
	DWORD i;

	Version = GRDLANDVERSION;
	m_Version = Version;
	fprintf(Stream,"GrdLand {\n");
	fprintf(Stream,"    Version %d\n",Version);
	fprintf(Stream,"    3DPosition %f %f %f\n",m_CameraXPos,m_CameraYPos,m_CameraZPos);
	fprintf(Stream,"    3DRotation %f %f %f\n",m_CameraXRot,m_CameraYRot,m_CameraZRot);
	fprintf(Stream,"    2DPosition %d %d\n",m_ScrollX,m_ScrollY);
	fprintf(Stream,"    CustomSnap %d %d\n",m_SnapX,m_SnapZ);
	fprintf(Stream,"    SnapMode %d\n",m_SnapMode);
	fprintf(Stream,"    Gravity %d\n",m_EnableGravity);
	fprintf(Stream,"    HeightScale %d\n",m_HeightScale);
	fprintf(Stream,"    MapWidth %d\n",m_MapWidth);
	fprintf(Stream,"    MapHeight %d\n",m_MapHeight);
	fprintf(Stream,"    TileWidth %d\n",m_TileWidth);
	fprintf(Stream,"    TileHeight %d\n",m_TileHeight);
	fprintf(Stream,"    SeaLevel %d\n",m_SeaLevel);
	fprintf(Stream,"    TextureWidth %d\n",m_TextureWidth);
	fprintf(Stream,"    TextureHeight %d\n",m_TextureHeight);
	fprintf(Stream,"    NumTextures %d\n",m_NumTextures);
	fprintf(Stream,"    Textures {\n");
	for(i=0; i<m_NumTextures; i++) {
		fprintf(Stream,"        %s\n",m_TextureNames[i]);
	}
	fprintf(Stream,"    }\n");
	fprintf(Stream,"    NumTiles %d\n",m_NumTiles);
	fprintf(Stream,"    Tiles {\n");
	for(i=0; i<m_NumTiles; i++) {
		m_Tiles[i]->Write(Stream);
	}
	fprintf(Stream,"    }\n");
	fprintf(Stream,"}\n");
	return	TRUE;
}


void CGrdLandIO::GetCameraPosition(float *x,float *y,float *z)
{
	 *x = m_CameraXPos;
	 *y = m_CameraYPos;
	 *z = m_CameraZPos;
}

void CGrdLandIO::GetCameraRotation(float *x,float *y,float *z)
{
	 *x = m_CameraXRot;
	 *y = m_CameraYRot;
	 *z = m_CameraZRot;
}

void CGrdLandIO::Get2DPosition(SLONG *ScrollX,SLONG *ScrollY)
{
	*ScrollX = m_ScrollX;
	*ScrollY = m_ScrollY;
}

DWORD CGrdLandIO::GetHeightScale(void)
{
	return m_HeightScale;
}

void CGrdLandIO::GetTextureSize(DWORD *TextureWidth,DWORD *TextureHeight)
{
	*TextureWidth = m_TextureWidth;
	*TextureHeight = m_TextureHeight;
}

void CGrdLandIO::GetMapSize(DWORD *MapWidth,DWORD *MapHeight)
{
	*MapWidth = m_MapWidth;
	*MapHeight = m_MapHeight;
}

void CGrdLandIO::GetTileSize(DWORD *TileWidth,DWORD *TileHeight)
{
	*TileWidth = m_TileWidth;
	*TileHeight = m_TileHeight;
}

DWORD CGrdLandIO::GetNumTextures(void)
{
	return m_NumTextures;
}

char *CGrdLandIO::GetTextureName(DWORD NameIndex)
{
	ASSERT(NameIndex < m_NumTextures);

	return m_TextureNames[NameIndex];
}


BOOL CGrdLandIO::Read(FILE *Stream)
{
	CHECKTRUE(StartChunk(Stream,"GrdLand"));
	fpos_t Pos;
	int PosOk;
	PosOk = fgetpos(Stream, &Pos );
	if(!ReadLong(Stream,"Version",(LONG*)&Version)) {
		PosOk = fsetpos(Stream, &Pos );
		DebugPrint("No Version Number\n");
		Version = 0;
	}
	m_Version = Version;
	CHECKTRUE(ReadFloat(Stream,"3DPosition",&m_CameraXPos));
	CHECKTRUE(ReadFloat(Stream,NULL,&m_CameraYPos));
	CHECKTRUE(ReadFloat(Stream,NULL,&m_CameraZPos));
	CHECKTRUE(ReadFloat(Stream,"3DRotation",&m_CameraXRot));
	CHECKTRUE(ReadFloat(Stream,NULL,&m_CameraYRot));
	CHECKTRUE(ReadFloat(Stream,NULL,&m_CameraZRot));
	CHECKTRUE(ReadLong(Stream,"2DPosition",(LONG*)&m_ScrollX));
	CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&m_ScrollY));
	if(Version >= 1) {
		CHECKTRUE(ReadLong(Stream,"CustomSnap",(LONG*)&m_SnapX));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&m_SnapZ));
		CHECKTRUE(ReadLong(Stream,"SnapMode",(LONG*)&m_SnapMode));
		CHECKTRUE(ReadLong(Stream,"Gravity",(LONG*)&m_EnableGravity));
	} else {
		m_SnapMode = 0;
		m_SnapX = 256;
		m_SnapZ = 256;
		m_EnableGravity = 1;
	}
	CHECKTRUE(ReadLong(Stream,"HeightScale",(LONG*)&m_HeightScale));
	CHECKTRUE(ReadLong(Stream,"MapWidth",(LONG*)&m_MapWidth));
	CHECKTRUE(ReadLong(Stream,"MapHeight",(LONG*)&m_MapHeight));
	CHECKTRUE(ReadLong(Stream,"TileWidth",(LONG*)&m_TileWidth));
	CHECKTRUE(ReadLong(Stream,"TileHeight",(LONG*)&m_TileHeight));
	if(Version >= 3) {
		CHECKTRUE(ReadLong(Stream,"SeaLevel",(LONG*)&m_SeaLevel));
	} else {
		m_SeaLevel = 100;
	}
	CHECKTRUE(ReadLong(Stream,"TextureWidth",(LONG*)&m_TextureWidth));
	CHECKTRUE(ReadLong(Stream,"TextureHeight",(LONG*)&m_TextureHeight));

	DWORD NumTextures;
	DWORD i;

	CHECKTRUE(ReadLong(Stream,"NumTextures",(LONG*)&NumTextures));
	SetNumTextures(NumTextures);

	CHECKTRUE(StartChunk(Stream,"Textures"));
	for(i=0; i<NumTextures; i++) {
		CHECKTRUE(ReadStringAlloc(Stream,NULL,&m_TextureNames[i]));
	}
	CHECKTRUE(EndChunk(Stream));

	CHECKTRUE(ReadLong(Stream,"NumTiles",(LONG*)&m_NumTiles));
	m_Tiles = new CGrdTileIO*[m_NumTiles];

	CHECKTRUE(StartChunk(Stream,"Tiles"));
	for(i=0; i<m_NumTiles; i++) {
		m_Tiles[i] = new CGrdTileIO;
		m_Tiles[i]->Read(Stream);
	}
	CHECKTRUE(EndChunk(Stream));

	CHECKTRUE(EndChunk(Stream));

	return	TRUE;
}


