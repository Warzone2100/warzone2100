// InfoDialog.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "InfoDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfoDialog dialog


CInfoDialog::CInfoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CInfoDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInfoDialog)
	m_Info = _T("");
	//}}AFX_DATA_INIT
	m_pView = NULL;
}


CInfoDialog::CInfoDialog(CView *pView)
{
	m_pView = pView;
}


BOOL CInfoDialog::Create(void)
{
	return CDialog::Create(CInfoDialog::IDD);	//_DEBUGINFO);
}


void CInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInfoDialog)
	DDX_Text(pDX, IDC_DEBUGINFO, m_Info);
	DDV_MaxChars(pDX, m_Info, 65535);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInfoDialog, CDialog)
	//{{AFX_MSG_MAP(CInfoDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInfoDialog message handlers

void CInfoDialog::OnCancel() 
{
	if(m_pView != NULL) {
		m_pView->PostMessage(WM_GOODBYE, IDCANCEL);
	}
}

void CInfoDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_pView != NULL) {
		m_pView->PostMessage(WM_GOODBYE, IDOK);
	}
}
