// ExpandLimitsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "debugprint.h"
#include "expandlimitsdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpandLimitsDlg dialog


CExpandLimitsDlg::CExpandLimitsDlg(CHeightMap *World,CWnd* pParent /*=NULL*/)
	: CDialog(CExpandLimitsDlg::IDD, pParent)
{
	m_World = World;
	m_ExcludeSelected = -1;
	m_IncludeSelected = -1;

	//{{AFX_DATA_INIT(CExpandLimitsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CExpandLimitsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpandLimitsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpandLimitsDlg, CDialog)
	//{{AFX_MSG_MAP(CExpandLimitsDlg)
	ON_CBN_SELCHANGE(IDC_EXCLUDELIST, OnSelchangeExcludelist)
	ON_CBN_SELCHANGE(IDC_INCLUDELIST, OnSelchangeIncludelist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpandLimitsDlg message handlers

BOOL CExpandLimitsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_EXCLUDELIST);

	ListNode<CScrollLimits> *TmpNode = m_World->GetScrollLimits();
	int ListSize = 0;
	char *FirstString;

	// Add the strings to the list box.
	while(TmpNode!=NULL) {
		List->AddString(TmpNode->GetData()->ScriptName);
		if(ListSize == 0) {
			FirstString = TmpNode->GetData()->ScriptName;
		}
		ListSize++;
		TmpNode = TmpNode->GetNextNode();
	}

	// Set the default selection.
	if(ListSize) {
		List->SelectString(-1, FirstString);
		m_ExcludeSelected = 0;
	}


	List = (CComboBox*)GetDlgItem(IDC_INCLUDELIST);

	TmpNode = m_World->GetScrollLimits();
	ListSize = 0;

	// Add the strings to the list box.
	while(TmpNode!=NULL) {
		List->AddString(TmpNode->GetData()->ScriptName);
		if(ListSize == 0) {
			FirstString = TmpNode->GetData()->ScriptName;
		}
		ListSize++;
		TmpNode = TmpNode->GetNextNode();
	}

	// Set the default selection.
	if(ListSize) {
		List->SelectString(-1, FirstString);
		m_IncludeSelected = 0;
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExpandLimitsDlg::OnSelchangeExcludelist() 
{
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_EXCLUDELIST);
	m_ExcludeSelected = List->GetCurSel();
}

void CExpandLimitsDlg::OnSelchangeIncludelist() 
{
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_INCLUDELIST);
	m_IncludeSelected = List->GetCurSel();
}
