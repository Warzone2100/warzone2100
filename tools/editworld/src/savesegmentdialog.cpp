// SaveSegmentDialog.cpp : implementation file
//

#include "stdafx.h"
#include "BTEdit.h"
#include "savesegmentdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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
