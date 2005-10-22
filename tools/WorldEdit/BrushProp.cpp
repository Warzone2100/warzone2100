// BrushProp.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "BTEditDoc.h"
#include "WFView.h"
#include "DebugPrint.h"

#include "BrushProp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrushProp dialog


static CBTEditDoc *m_pDoc;

CBrushProp::CBrushProp(CWnd* pParent /*=NULL*/)
	: CDialog(CBrushProp::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBrushProp)
	m_Height = 0;
	m_RandomRange = 0;
	m_BrushID = 0;
	m_BrushSize = -1;
	m_HeightSetting = -1;
	//}}AFX_DATA_INIT

	m_pView = NULL;
}


CBrushProp::CBrushProp(CView *pView)
{
	m_Height = 0;
	m_RandomRange = 0;
	m_BrushID = 0;
	m_BrushSize = 1;
	m_HeightSetting = 0;

	m_OldHeight = (255-m_Height);

	m_pView = pView;
	m_pDoc = ((CWFView*)m_pView)->GetDocument();
	ASSERT_VALID(m_pDoc);
}


BOOL CBrushProp::Create(void)
{
	BOOL Ok = CDialog::Create(CBrushProp::IDD);

	if(Ok) {
		m_HeightSlider.SetRange( 0, 255, TRUE );
		m_HeightSlider.SetTicFreq( 16 );
		m_RandomSlider.SetRange( 0, 64, TRUE );
		m_RandomSlider.SetTicFreq( 8 );
		GetBrushData();
	}

	return Ok;
}


void CBrushProp::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrushProp)
	DDX_Control(pDX, IDC_RANDOMRANGE, m_RandomSlider);
	DDX_Control(pDX, IDC_HEIGHTSLIDER, m_HeightSlider);
	DDX_Text(pDX, IDC_HEIGHTEDIT, m_Height);
	DDV_MinMaxInt(pDX, m_Height, 0, 255);
	DDX_Text(pDX, IDC_RANDEDIT, m_RandomRange);
	DDV_MinMaxInt(pDX, m_RandomRange, 0, 64);
	DDX_Text(pDX, IDC_BRUSHID, m_BrushID);
	DDV_MinMaxInt(pDX, m_BrushID, 0, 15);
	DDX_Radio(pDX, IDC_BRUSHSIZE, m_BrushSize);
	DDX_Radio(pDX, IDC_SETHEIGHT, m_HeightSetting);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrushProp, CDialog)
	//{{AFX_MSG_MAP(CBrushProp)
	ON_EN_CHANGE(IDC_HEIGHTEDIT, OnChangeHeightedit)
	ON_EN_CHANGE(IDC_RANDEDIT, OnChangeRandedit)
	ON_EN_CHANGE(IDC_BRUSHID, OnChangeBrushid)
	ON_BN_CLICKED(IDC_BRUSHIDNEXT, OnBrushidnext)
	ON_BN_CLICKED(IDC_BRUSHIDPREV, OnBrushidprev)
	ON_BN_CLICKED(IDC_BRUSHLARGE, OnBrushlarge)
	ON_BN_CLICKED(IDC_BRUSHSIZE, OnBrushsize)
	ON_BN_CLICKED(IDC_ABHEIGHT, OnAbheight)
	ON_BN_CLICKED(IDC_SETHEIGHT, OnSetheight)
	ON_BN_CLICKED(IDC_WATERHEIGHT, OnWaterheight)
	ON_WM_KILLFOCUS()
	ON_WM_KEYUP()
	ON_WM_DRAWITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrushProp message handlers

void CBrushProp::OnCancel() 
{
	DestroyWindow();
//	CDialog::OnCancel();
}


void CBrushProp::GetBrushData(void)
{
	CEdgeBrush *EdgeBrush = m_pDoc->GetEdgeBrush();

	m_BrushID = m_pDoc->GetEdgeBrushID();

	m_Height = EdgeBrush->m_Height;
	m_OldHeight = (255-m_Height);
	m_HeightSlider.SetPos(m_OldHeight);

	m_RandomRange = EdgeBrush->m_RandomRange;
	m_RandomSlider.SetPos(m_RandomRange);

	if(EdgeBrush->GetLargeBrush()) {
		m_BrushSize = EBP_BS_LARGE;
	} else {
		m_BrushSize = EBP_BS_SMALL;
	}

	m_HeightSetting = EdgeBrush->m_HeightMode;

	// Set dialog's controls from brush data.
	UpdateData(FALSE);

	GetDlgItem(IDC_TILEBUT1)->InvalidateRect(NULL,FALSE);
}


void CBrushProp::SetBrushData(void)
{
	// Get data from the dialog's controls.
	UpdateData(TRUE);

	CEdgeBrush *EdgeBrush = m_pDoc->GetEdgeBrush();

	EdgeBrush->m_Height = m_Height;
	EdgeBrush->m_RandomRange = m_RandomRange;

	if(m_BrushSize == EBP_BS_SMALL) {
		EdgeBrush->SetLargeBrush(FALSE);
	} else {
		EdgeBrush->SetLargeBrush(TRUE);
	}

	EdgeBrush->m_HeightMode = m_HeightSetting;
}


BOOL CBrushProp::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR *Message = (NMHDR*)lParam;

	if(wParam == IDC_HEIGHTSLIDER) {
//		DebugPrint("%d %d\n",Message->idFrom,Message->code);
   		if(m_OldHeight != m_HeightSlider.GetPos()) {
   			m_OldHeight = m_HeightSlider.GetPos();
   			m_Height = 255-m_HeightSlider.GetPos();
   			UpdateData(FALSE);
   		}
		if(Message->code != NM_CUSTOMDRAW) {
			SetBrushData();
		}
	}

	if(wParam == IDC_RANDOMRANGE) {
//		DebugPrint("%d %d\n",Message->idFrom,Message->code);
		if(m_RandomRange != m_RandomSlider.GetPos()) {
			m_RandomRange = m_RandomSlider.GetPos();
			UpdateData(FALSE);
		}
		if(Message->code != NM_CUSTOMDRAW) {
			SetBrushData();
		}
	}

	return CDialog::OnNotify(wParam, lParam, pResult);
}


void CBrushProp::OnChangeHeightedit() 
{
	UpdateData();
	m_OldHeight = (255-m_Height);
	m_HeightSlider.SetPos(255-m_Height);
	SetBrushData();
}



void CBrushProp::OnChangeRandedit() 
{
	UpdateData();
	m_RandomSlider.SetPos(m_RandomRange);
	SetBrushData();
}



void CBrushProp::OnChangeBrushid() 
{
	GetBrushData();
}

void CBrushProp::OnBrushidnext() 
{
	SetBrushData();

	m_BrushID ++;
	if(m_BrushID > 15) {
		m_BrushID = 0;
	}

	m_pDoc->SetEdgeBrush(m_BrushID);
	GetBrushData();
}

void CBrushProp::OnBrushidprev() 
{
	SetBrushData();

	m_BrushID --;
	if(m_BrushID < 0) {
		m_BrushID = 15;
	}

	m_pDoc->SetEdgeBrush(m_BrushID);
	GetBrushData();
}




void CBrushProp::OnBrushlarge() 
{
	SetBrushData();
}

void CBrushProp::OnBrushsize() 
{
	SetBrushData();
}




void CBrushProp::OnAbheight() 
{
	SetBrushData();
}


void CBrushProp::OnSetheight() 
{
	SetBrushData();
}
 
void CBrushProp::OnWaterheight() 
{
	SetBrushData();
}



void CBrushProp::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);
}

void CBrushProp::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}

/*
typedef struct tagDRAWITEMSTRUCT {
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    UINT        itemAction;
    UINT        itemState;
    HWND        hwndItem;
    HDC         hDC;
    RECT        rcItem;
    DWORD       itemData;
} DRAWITEMSTRUCT, NEAR *PDRAWITEMSTRUCT, FAR *LPDRAWITEMSTRUCT;
*/

void CBrushProp::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	int ButWidth = lpDrawItemStruct->rcItem.right&0xfffc;
	int ButHeight = lpDrawItemStruct->rcItem.bottom;
	
	CDIBDraw *Dib = new CDIBDraw(lpDrawItemStruct->hwndItem,
								ButWidth,ButHeight,
								g_WindowsIs555);
	Dib->ClearDIB();

	CEdgeBrush *EdgeBrush = m_pDoc->GetEdgeBrush();
	CTextureSelector *Textures = m_pDoc->GetTextureSelector();

	int x = (ButWidth-m_pDoc->GetTextureWidth()*3)/2;
	int y = (ButHeight-m_pDoc->GetTextureHeight())/2;

	EdgeBrush->DrawEdgeBrushIcon(Dib,Textures->GetImages(),x,y);

	Dib->SwitchBuffers();

//	CDC	*dc=dc->FromHandle(lpDrawItemStruct->hDC);
//	CPen Pen;
//	Pen.CreatePen( PS_SOLID, 2, RGB(255,0,255) );
//	dc->SelectObject( &Pen );
//
//  int x0 = lpDrawItemStruct->rcItem.left;
//  int y0 = lpDrawItemStruct->rcItem.top;
//  int x1 = lpDrawItemStruct->rcItem.right;
//  int y1 = lpDrawItemStruct->rcItem.bottom;
//
//	dc->MoveTo(x0,y0);
//	dc->LineTo(x1,y0);
//	dc->LineTo(x1,y1);
//	dc->LineTo(x0,y1);
//	dc->LineTo(x0,y0);
//
	delete Dib;

	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
