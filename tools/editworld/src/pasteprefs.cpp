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
// PastePrefs.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "pasteprefs.h"

/////////////////////////////////////////////////////////////////////////////
// CPastePrefs dialog


CPastePrefs::CPastePrefs(CWnd* pParent /*=NULL*/)
	: CDialog(CPastePrefs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPastePrefs)
	m_Heights = FALSE;
	m_Objects = FALSE;
	m_Textures = FALSE;
	//}}AFX_DATA_INIT
}


void CPastePrefs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPastePrefs)
	DDX_Check(pDX, IDC_PASTEHEIGHTS, m_Heights);
	DDX_Check(pDX, IDC_PASTEOBJECTS, m_Objects);
	DDX_Check(pDX, IDC_PASTETEXTURES, m_Textures);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPastePrefs, CDialog)
	//{{AFX_MSG_MAP(CPastePrefs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPastePrefs message handlers
