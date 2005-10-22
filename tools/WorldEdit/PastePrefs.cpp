// PastePrefs.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "PastePrefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
