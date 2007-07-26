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
// SaveSegmentDialog.cpp : implementation file
//

#include "stdafx.h"
#include "BTEdit.h"
#include "savesegmentdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaveSegmentDialog dialog


CSaveSegmentDialog::CSaveSegmentDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSaveSegmentDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSaveSegmentDialog)
	m_Height = 0;
	m_Width = 0;
	m_StartX = 0;
	m_StartY = 0;
	//}}AFX_DATA_INIT
}


void CSaveSegmentDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveSegmentDialog)
	DDX_Text(pDX, IDC_SSEG_HEIGHT, m_Height);
	DDV_MinMaxUInt(pDX, m_Height, 1, 512);
	DDX_Text(pDX, IDC_SSEG_WIDTH, m_Width);
	DDV_MinMaxUInt(pDX, m_Width, 1, 512);
	DDX_Text(pDX, IDC_SSEG_X, m_StartX);
	DDV_MinMaxUInt(pDX, m_StartX, 0, 512);
	DDX_Text(pDX, IDC_SSEG_Y, m_StartY);
	DDV_MinMaxUInt(pDX, m_StartY, 0, 512);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveSegmentDialog, CDialog)
	//{{AFX_MSG_MAP(CSaveSegmentDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveSegmentDialog message handlers
