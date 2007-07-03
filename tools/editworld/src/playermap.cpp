// PlayerMap.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "playermap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlayerMap dialog


CPlayerMap::CPlayerMap(CWnd* pParent /*=NULL*/)
	: CDialog(CPlayerMap::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPlayerMap)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPlayerMap::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlayerMap)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlayerMap, CDialog)
	//{{AFX_MSG_MAP(CPlayerMap)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlayerMap message handlers


#define NUM_CLANTYPES	8

char *ClanNames[NUM_CLANTYPES]={
	"Clan 1","Clan 2","Clan 3","Clan 4","Clan 5","Clan 6","Clan 7","Barbarian"
};

BOOL CPlayerMap::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_PLAYER1CLAN);
	int ListSize = 0;
	char *FirstString;

	// Add the strings to the list box.
	for(int i=0; i<NUM_CLANTYPES; i++) {
		List->AddString(ClanNames[i]);
		if(ListSize == 0) {
			FirstString = ClanNames[i];
		}
		ListSize++;
	}

	// Set the default selection.
	List->SelectString(-1, FirstString);
	m_Selected = 0;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
