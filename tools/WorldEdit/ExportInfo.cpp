// ExportInfo.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "ExportInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
