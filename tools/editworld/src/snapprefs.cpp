// SnapPrefs.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "snapprefs.h"
//#include "datatypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSnapPrefs dialog


CSnapPrefs::CSnapPrefs(CWnd* pParent /*=NULL*/)
	: CDialog(CSnapPrefs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSnapPrefs)
	m_EnableGravity = FALSE;
	m_SnapX = 0;
	m_SnapZ = 0;
	m_SnapMode = 0;
	m_RotSnap = 0;
	//}}AFX_DATA_INIT
}


void CSnapPrefs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

//	Check = m_SnapMode==SNAP_NONE ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPNONE, Check);
//	Check = m_SnapMode==SNAP_CENTER ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPTCEN, Check);
//	Check = m_SnapMode==SNAP_TOPLEFT ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPTL, Check);
//	Check = m_SnapMode==SNAP_TOPRIGHT ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPTR, Check);
//	Check = m_SnapMode==SNAP_BOTTOMLEFT ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPBL, Check);
//	Check = m_SnapMode==SNAP_BOTTOMRIGHT ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPBR, Check);
//	Check = m_SnapMode==SNAP_CUSTOM ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPCUSTOM, Check);
//	Check = m_SnapMode==SNAP_QUAD ? TRUE : FALSE;
//	DDX_Check(pDX, IDC_SNAPQUAD, Check);


	//{{AFX_DATA_MAP(CSnapPrefs)
	DDX_Check(pDX, IDC_ENABLEDGRAVITY, m_EnableGravity);
	DDX_Text(pDX, IDC_SNAPCUSTOMX, m_SnapX);
	DDV_MinMaxDWord(pDX, m_SnapX, 1, 9999);
	DDX_Text(pDX, IDC_SNAPCUSTOMZ, m_SnapZ);
	DDV_MinMaxDWord(pDX, m_SnapZ, 1, 9999);
	DDX_Text(pDX, IDC_ROTSNAP, m_RotSnap);
	DDV_MinMaxDWord(pDX, m_RotSnap, 1, 360);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSnapPrefs, CDialog)
	//{{AFX_MSG_MAP(CSnapPrefs)
	ON_BN_CLICKED(IDC_SNAPNONE, OnSnapnone)
	ON_BN_CLICKED(IDC_SNAPTCEN, OnSnaptcen)
	ON_BN_CLICKED(IDC_SNAPTL, OnSnaptl)
	ON_BN_CLICKED(IDC_SNAPTR, OnSnaptr)
	ON_BN_CLICKED(IDC_SNAPBL, OnSnapbl)
	ON_BN_CLICKED(IDC_SNAPBR, OnSnapbr)
	ON_BN_CLICKED(IDC_SNAPCUSTOM, OnSnapcustom)
	ON_BN_CLICKED(IDC_SNAPQUAD, OnSnapquad)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSnapPrefs message handlers

void CSnapPrefs::OnSnapnone() 
{
//	m_SnapMode = SNAP_NONE;
}

void CSnapPrefs::OnSnaptcen() 
{
//	m_SnapMode = SNAP_CENTER;
}

void CSnapPrefs::OnSnaptl() 
{
//	m_SnapMode = SNAP_TOPLEFT;
}

void CSnapPrefs::OnSnaptr() 
{
//	m_SnapMode = SNAP_TOPRIGHT;
}

void CSnapPrefs::OnSnapbl() 
{
//	m_SnapMode = SNAP_BOTTOMLEFT;
}

void CSnapPrefs::OnSnapbr() 
{
//	m_SnapMode = SNAP_BOTTOMRIGHT;
}

void CSnapPrefs::OnSnapcustom() 
{
//	m_SnapMode = SNAP_CUSTOM;
}

void CSnapPrefs::OnSnapquad() 
{
//	m_SnapMode = SNAP_QUAD;
}
