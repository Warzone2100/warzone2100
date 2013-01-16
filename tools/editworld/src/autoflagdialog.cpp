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

#include "stdafx.h"
#include "btedit.h"
#include "autoflagdialog.hpp"

/////////////////////////////////////////////////////////////////////////////
// AutoFlagDialog dialog


AutoFlagDialog::AutoFlagDialog(CWnd* parent,
                               bool RandRotate,
                               bool RandXFlip,
                               bool RandYFlip,
                               bool XFlip,
                               bool YFlip,
                               unsigned int Rotate,
                               bool IncRotate,
                               bool ToggleXFlip,
                               bool ToggleYFlip) :
	CDialog(AutoFlagDialog::IDD, parent),
	_RandRotate(RandRotate),
	_RandXFlip(RandXFlip),
	_RandYFlip(RandYFlip),
	_XFlip(XFlip),
	_YFlip(YFlip),
	_Rotate(Rotate),
	_IncRotate(IncRotate),
	_ToggleXFlip(ToggleXFlip),
	_ToggleYFlip(ToggleYFlip)
{
	//{{AFX_DATA_INIT(AutoFlagDialog)
	//}}AFX_DATA_INIT
}

bool AutoFlagDialog::RandRotate() const
{
    return _RandRotate;
}

bool AutoFlagDialog::RandXFlip() const
{
    return _RandXFlip;
}

bool AutoFlagDialog::RandYFlip() const
{
    return _RandYFlip;
}

bool AutoFlagDialog::XFlip() const
{
    return _XFlip;
}

bool AutoFlagDialog::YFlip() const
{
    return _YFlip;
}

unsigned int AutoFlagDialog::Rotate() const
{
    return _Rotate;
}

bool AutoFlagDialog::IncRotate() const
{
    return _IncRotate;
}

bool AutoFlagDialog::ToggleXFlip() const
{
    return _ToggleXFlip;
}

bool AutoFlagDialog::ToggleYFlip() const
{
    return _ToggleYFlip;
}

void AutoFlagDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AutoFlagDialog)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AutoFlagDialog, CDialog)
	//{{AFX_MSG_MAP(AutoFlagDialog)
	ON_BN_CLICKED(IDC_CHKRANDROTATE, OnRandRotate)
	ON_BN_CLICKED(IDC_CHKRANDXFLIP, OnRandXFlip)
	ON_BN_CLICKED(IDC_CHKRANDYFLIP, OnRandYFlip)
	ON_BN_CLICKED(IDC_ROT0, OnRotate)
	ON_BN_CLICKED(IDC_ROT180, OnRotate)
	ON_BN_CLICKED(IDC_ROT270, OnRotate)
	ON_BN_CLICKED(IDC_ROT90, OnRotate)
	ON_BN_CLICKED(IDC_XFLIP, OnXFlip)
	ON_BN_CLICKED(IDC_YFLIP, OnYFlip)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AutoFlagDialog message handlers

BOOL AutoFlagDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	Degree0_RadioButton = (CButton*)GetDlgItem(IDC_ROT0);
	Degree90_RadioButton = (CButton*)GetDlgItem(IDC_ROT90);
	Degree180_RadioButton = (CButton*)GetDlgItem(IDC_ROT180);
	Degree270_RadioButton = (CButton*)GetDlgItem(IDC_ROT270);
	XFlip_CheckBox = (CButton*)GetDlgItem(IDC_XFLIP);
	YFlip_CheckBox = (CButton*)GetDlgItem(IDC_YFLIP);
	RandRotate_CheckBox = (CButton*)GetDlgItem(IDC_CHKRANDROTATE);
	RandXFlip_CheckBox = (CButton*)GetDlgItem(IDC_CHKRANDXFLIP);
	RandYFlip_CheckBox = (CButton*)GetDlgItem(IDC_CHKRANDYFLIP);
	//IncrementRotate_CheckBox = (CButton*)GetDlgItem(IDC_CHKINCROTATE);
	//ToggleXFlip_CheckBox = (CButton*)GetDlgItem(IDC_CHKTOGXFLIP);
	//ToggleYFlip_CheckBox = (CButton*)GetDlgItem(IDC_CHKTOGYFLIP);

//	IncrementRotate_CheckBox->setCheck(_IncRotate);
//	ToggleXFlip_CheckBox->setCheck(_ToggleXFlip);
//	ToggleYFlip_CheckBox->setCheck(_ToggleYFlip);
	XFlip_CheckBox->SetCheck(_XFlip);
	YFlip_CheckBox->SetCheck(_YFlip);
	RandRotate_CheckBox->SetCheck(_RandRotate);
	RandXFlip_CheckBox->SetCheck(_RandXFlip);
	RandYFlip_CheckBox->SetCheck(_RandYFlip);

	if(!_RandRotate)
	{
		switch(_Rotate)
		{
			case 0:
				Degree0_RadioButton->SetCheck(1);
				break;

			case 1:
				Degree90_RadioButton->SetCheck(1);
				break;

			case 2:
				Degree180_RadioButton->SetCheck(1);
				break;

			case 3:
				Degree270_RadioButton->SetCheck(1);
				break;
		}
	}
	else
	{
		Degree0_RadioButton->SetCheck(0);
		Degree90_RadioButton->SetCheck(0);
		Degree180_RadioButton->SetCheck(0);
		Degree270_RadioButton->SetCheck(0);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void AutoFlagDialog::OnOK()
{
	_IncRotate = false;	  //IncrementRotate_CheckBox->GetCheck();
	_ToggleXFlip = false; //ToggleXFlip_CheckBox->GetCheck();
	_ToggleYFlip = false; //ToggleYFlip_CheckBox->GetCheck();
	_XFlip = XFlip_CheckBox->GetCheck();
	_YFlip = XFlip_CheckBox->GetCheck();
	_RandRotate = RandRotate_CheckBox->GetCheck();
	_RandXFlip = RandXFlip_CheckBox->GetCheck();
	_RandYFlip = RandYFlip_CheckBox->GetCheck();

	if      (Degree0_RadioButton->GetCheck())
		_Rotate = 0;
	else if (Degree90_RadioButton->GetCheck())
		_Rotate = 1;
	else if (Degree180_RadioButton->GetCheck())
		_Rotate = 2;
	else if (Degree270_RadioButton->GetCheck())
		_Rotate = 3;

	CDialog::OnOK();
}

void AutoFlagDialog::OnRandRotate()
{
	Degree0_RadioButton->SetCheck(0);
	Degree90_RadioButton->SetCheck(0);
	Degree180_RadioButton->SetCheck(0);
	Degree270_RadioButton->SetCheck(0);
}

void AutoFlagDialog::OnRandXFlip()
{
	XFlip_CheckBox->SetCheck(0);
}

void AutoFlagDialog::OnRandYFlip()
{
	YFlip_CheckBox->SetCheck(0);
}

void AutoFlagDialog::OnRotate()
{
	RandRotate_CheckBox->SetCheck(0);
}

void AutoFlagDialog::OnXFlip()
{
	RandXFlip_CheckBox->SetCheck(0);
}

void AutoFlagDialog::OnYFlip()
{
	RandYFlip_CheckBox->SetCheck(0);
}
