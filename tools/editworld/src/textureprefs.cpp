/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
// TexturePrefs.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "textureprefs.h"

/////////////////////////////////////////////////////////////////////////////
// CTexturePrefs dialog


CTexturePrefs::CTexturePrefs(CWnd* pParent /*=NULL*/)
	: CDialog(CTexturePrefs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTexturePrefs)
	m_TextureWidth = 64;
	m_TextureHeight = 64;
	//}}AFX_DATA_INIT
}


void CTexturePrefs::SetTextureSize(DWORD TextureWidth,DWORD TextureHeight)
{
	m_TextureWidth = TextureWidth;
	m_TextureHeight  = TextureHeight;
}

void CTexturePrefs::GetTextureSize(DWORD *TextureWidth,DWORD *TextureHeight)
{
	*TextureWidth = m_TextureWidth;
	*TextureHeight  = m_TextureHeight;
}

void CTexturePrefs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTexturePrefs)
	DDX_Text(pDX, IDC_TEXTUREHEIGHT, m_TextureWidth);
	DDV_MinMaxDWord(pDX, m_TextureWidth, 1, 256);
	DDX_Text(pDX, IDC_TEXTUREWIDTH, m_TextureHeight);
	DDV_MinMaxDWord(pDX, m_TextureHeight, 1, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTexturePrefs, CDialog)
	//{{AFX_MSG_MAP(CTexturePrefs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTexturePrefs message handlers
