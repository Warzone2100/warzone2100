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
// SaveSegmentDialog.cpp : implementation file
//

#include "stdafx.h"
#include "savesegmentdialog.hpp"

/////////////////////////////////////////////////////////////////////////////
// SaveSegmentDialog dialog


SaveSegmentDialog::SaveSegmentDialog(CWnd* parent,
                                     unsigned int StartX_,
                                     unsigned int StartY_,
                                     unsigned int Width_,
                                     unsigned int Height_) : 
	CDialog(SaveSegmentDialog::IDD, parent),
	_StartX(StartX_),
	_StartY(StartY_),
	_Width(Width_),
	_Height(Height_)
{
	//{{AFX_DATA_INIT(SaveSegmentDialog)
	//}}AFX_DATA_INIT
}


void SaveSegmentDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SaveSegmentDialog)
	DDX_Text(pDX, IDC_SSEG_HEIGHT, _Height);
	DDV_MinMaxUInt(pDX, _Height, 1, 512);
	DDX_Text(pDX, IDC_SSEG_WIDTH, _Width);
	DDV_MinMaxUInt(pDX, _Width, 1, 512);
	DDX_Text(pDX, IDC_SSEG_X, _StartX);
	DDV_MinMaxUInt(pDX, _StartX, 0, 512);
	DDX_Text(pDX, IDC_SSEG_Y, _StartY);
	DDV_MinMaxUInt(pDX, _StartY, 0, 512);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SaveSegmentDialog, CDialog)
	//{{AFX_MSG_MAP(SaveSegmentDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SaveSegmentDialog message handlers

void SaveSegmentDialog::StartX(unsigned int StartX_)
{
	_StartX = StartX_;
}

void SaveSegmentDialog::StartY(unsigned int StartY_)
{
	_StartY = StartY_;
}

void SaveSegmentDialog::Width(unsigned int Width_)
{
	_Width = Width_;
}

void SaveSegmentDialog::Height(unsigned int Height_)
{
	_Height = Height_;
}

unsigned int SaveSegmentDialog::StartX() const
{
	return _StartX;
}

unsigned int SaveSegmentDialog::StartY() const
{
	return _StartY;
}

unsigned int SaveSegmentDialog::Width() const
{
	return _Width;
}

unsigned int SaveSegmentDialog::Height() const
{
	return _Height;
}
