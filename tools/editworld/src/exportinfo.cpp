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

	$Revision$
	$Id$
	$HeadURL$
*/
// ExportInfo.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "exportinfo.h"

/////////////////////////////////////////////////////////////////////////////
// CExportInfo dialog


CExportInfo::CExportInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CExportInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExportInfo)
	m_Text = _T("");
	//}}AFX_DATA_INIT
}


void CExportInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExportInfo)
	DDX_Text(pDX, IDC_INFOTEXT, m_Text);
	DDV_MaxChars(pDX, m_Text, 65536);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExportInfo, CDialog)
	//{{AFX_MSG_MAP(CExportInfo)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportInfo message handlers
