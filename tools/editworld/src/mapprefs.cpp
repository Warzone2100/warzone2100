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
// MapPrefs.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "mapprefs.h"

/////////////////////////////////////////////////////////////////////////////
// CMapPrefs dialog


CMapPrefs::CMapPrefs(CWnd* pParent /*=NULL*/)
	: CDialog(CMapPrefs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapPrefs)
	m_MapHeight = 128;
	m_MapWidth = 128;
	m_MapSpacing = 64;
	m_SeaLevel = 0;
	m_TextureSize = 0;
	m_HeightScale = 0;
	//}}AFX_DATA_INIT
}

void CMapPrefs::SetMapWidth(DWORD MapWidth)
{
	m_MapWidth = MapWidth;
}

DWORD CMapPrefs::GetMapWidth(void)
{
	return m_MapWidth;
}

void CMapPrefs::SetMapHeight(DWORD MapHeight)
{
	m_MapHeight = MapHeight;
}

DWORD CMapPrefs::GetMapHeight(void)
{
	return m_MapHeight;
}

void CMapPrefs::SetMapSpacing(DWORD MapSpacing)
{
	m_MapSpacing = MapSpacing;
}

DWORD CMapPrefs::GetMapSpacing(void)
{
	return m_MapSpacing;
}

void CMapPrefs::SetTextureSize(DWORD TextureSize)
{
	m_TextureSize = TextureSize;
}

DWORD CMapPrefs::GetTextureSize(void)
{
	return m_TextureSize;
}

void CMapPrefs::SetSeaLevel(DWORD SeaLevel)
{
	m_SeaLevel = SeaLevel;
}

DWORD CMapPrefs::GetSeaLevel(void)
{
	return m_SeaLevel;
}

void CMapPrefs::SetHeightScale(DWORD HeightScale)
{
	m_HeightScale = HeightScale;
}

DWORD CMapPrefs::GetHeightScale(void)
{
	return m_HeightScale;
}


void CMapPrefs::DoDataExchange(CDataExchange* pDX)
{
	TRACE0("DoDataExchange\n");
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapPrefs)
	DDX_Text(pDX, IDC_MAPHEIGHT, m_MapHeight);
	DDV_MinMaxDWord(pDX, m_MapHeight, 1, 256);
	DDX_Text(pDX, IDC_MAPWIDTH, m_MapWidth);
	DDV_MinMaxDWord(pDX, m_MapWidth, 1, 256);
	DDX_Text(pDX, IDC_MAPSPACEING, m_MapSpacing);
	DDV_MinMaxDWord(pDX, m_MapSpacing, 16, 16384);
	DDX_Text(pDX, IDC_SEALEVEL, m_SeaLevel);
	DDV_MinMaxDWord(pDX, m_SeaLevel, 0, 255);
	DDX_Text(pDX, IDC_TEXTURESIZE, m_TextureSize);
	DDV_MinMaxDWord(pDX, m_TextureSize, 16, 256);
	DDX_Text(pDX, IDC_HIGHTSCALE, m_HeightScale);
	DDV_MinMaxDWord(pDX, m_HeightScale, 1, 16);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapPrefs, CDialog)
	//{{AFX_MSG_MAP(CMapPrefs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapPrefs message handlers
