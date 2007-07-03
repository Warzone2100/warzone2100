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
// AutoFlagDialog.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "autoflagdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutoFlagDialog dialog


CAutoFlagDialog::CAutoFlagDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoFlagDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAutoFlagDialog)
	//}}AFX_DATA_INIT
}


void CAutoFlagDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoFlagDialog)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAutoFlagDialog, CDialog)
	//{{AFX_MSG_MAP(CAutoFlagDialog)
	ON_BN_CLICKED(IDC_CHKRANDROTATE, OnChkrandrotate)
	ON_BN_CLICKED(IDC_CHKRANDXFLIP, OnChkrandxflip)
	ON_BN_CLICKED(IDC_CHKRANDYFLIP, OnChkrandyflip)
	ON_BN_CLICKED(IDC_ROT0, OnRot0)
	ON_BN_CLICKED(IDC_ROT180, OnRot180)
	ON_BN_CLICKED(IDC_ROT270, OnRot270)
	ON_BN_CLICKED(IDC_ROT90, OnRot90)
	ON_BN_CLICKED(IDC_XFLIP, OnXflip)
	ON_BN_CLICKED(IDC_YFLIP, OnYflip)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoFlagDialog message handlers

//int GetCheck( ) const;

BOOL CAutoFlagDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
//	((CButton*)GetDlgItem(IDC_CHKINCROTATE))->SetCheck(m_IncRotate);
//	((CButton*)GetDlgItem(IDC_CHKTOGXFLIP))->SetCheck(m_TogXFlip);
//	((CButton*)GetDlgItem(IDC_CHKTOGYFLIP))->SetCheck(m_TogYFlip);
	((CButton*)GetDlgItem(IDC_XFLIP))->SetCheck(m_XFlip);
	((CButton*)GetDlgItem(IDC_YFLIP))->SetCheck(m_YFlip);
	((CButton*)GetDlgItem(IDC_CHKRANDROTATE))->SetCheck(m_RandRotate);
	((CButton*)GetDlgItem(IDC_CHKRANDXFLIP))->SetCheck(m_RandXFlip);
	((CButton*)GetDlgItem(IDC_CHKRANDYFLIP))->SetCheck(m_RandYFlip);

	if(m_RandRotate == FALSE) {
		switch(m_Rotate) {
			case 0:
				((CButton*)GetDlgItem(IDC_ROT0))->SetCheck(1);
				break;

			case 1:
				((CButton*)GetDlgItem(IDC_ROT90))->SetCheck(1);
				break;

			case 2:
				((CButton*)GetDlgItem(IDC_ROT180))->SetCheck(1);
				break;

			case 3:
				((CButton*)GetDlgItem(IDC_ROT270))->SetCheck(1);
				break;
		}
	} else {
		((CButton*)GetDlgItem(IDC_ROT0))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_ROT90))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_ROT180))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_ROT270))->SetCheck(0);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAutoFlagDialog::OnOK() 
{
	m_IncRotate = 0;	//((CButton*)GetDlgItem(IDC_CHKINCROTATE))->GetCheck();
	m_TogXFlip = 0;	//((CButton*)GetDlgItem(IDC_CHKTOGXFLIP))->GetCheck();
	m_TogYFlip = 0;	//((CButton*)GetDlgItem(IDC_CHKTOGYFLIP))->GetCheck();
	m_XFlip = ((CButton*)GetDlgItem(IDC_XFLIP))->GetCheck();
	m_YFlip = ((CButton*)GetDlgItem(IDC_YFLIP))->GetCheck();
	m_RandRotate = ((CButton*)GetDlgItem(IDC_CHKRANDROTATE))->GetCheck();
	m_RandXFlip = ((CButton*)GetDlgItem(IDC_CHKRANDXFLIP))->GetCheck();
	m_RandYFlip = ((CButton*)GetDlgItem(IDC_CHKRANDYFLIP))->GetCheck();

	if(((CButton*)GetDlgItem(IDC_ROT0))->GetCheck()) {
		m_Rotate = 0;
	} else if(((CButton*)GetDlgItem(IDC_ROT90))->GetCheck()) {
		m_Rotate = 1;
	} else if(((CButton*)GetDlgItem(IDC_ROT180))->GetCheck()) {
		m_Rotate = 2;
	} else if(((CButton*)GetDlgItem(IDC_ROT270))->GetCheck()) {
		m_Rotate = 3;
	}


	CDialog::OnOK();
}

void CAutoFlagDialog::OnChkrandrotate() 
{
	((CButton*)GetDlgItem(IDC_ROT0))->SetCheck( 0 );
	((CButton*)GetDlgItem(IDC_ROT90))->SetCheck( 0 );
	((CButton*)GetDlgItem(IDC_ROT180))->SetCheck( 0 );
	((CButton*)GetDlgItem(IDC_ROT270))->SetCheck( 0 );
}

void CAutoFlagDialog::OnChkrandxflip() 
{
	((CButton*)GetDlgItem(IDC_XFLIP))->SetCheck( 0 );
}

void CAutoFlagDialog::OnChkrandyflip() 
{
	((CButton*)GetDlgItem(IDC_YFLIP))->SetCheck( 0 );
}


void CAutoFlagDialog::OnRot0() 
{
	((CButton*)GetDlgItem(IDC_CHKRANDROTATE))->SetCheck( 0 );
}

void CAutoFlagDialog::OnRot180() 
{
	((CButton*)GetDlgItem(IDC_CHKRANDROTATE))->SetCheck( 0 );
}

void CAutoFlagDialog::OnRot270() 
{
	((CButton*)GetDlgItem(IDC_CHKRANDROTATE))->SetCheck( 0 );
}

void CAutoFlagDialog::OnRot90() 
{
	((CButton*)GetDlgItem(IDC_CHKRANDROTATE))->SetCheck( 0 );
}

void CAutoFlagDialog::OnXflip() 
{
	((CButton*)GetDlgItem(IDC_CHKRANDXFLIP))->SetCheck( 0 );
}

void CAutoFlagDialog::OnYflip() 
{
	((CButton*)GetDlgItem(IDC_CHKRANDYFLIP))->SetCheck( 0 );
}
