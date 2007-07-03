// TDView.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "bteditdoc.h"
#include "tdview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTDView

IMPLEMENT_DYNCREATE(CTDView, CView)

CTDView::CTDView()
{
}

CTDView::~CTDView()
{
}


BEGIN_MESSAGE_MAP(CTDView, CView)
	//{{AFX_MSG_MAP(CTDView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDView drawing

void CTDView::OnDraw(CDC* pDC)
{
	CBTEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code here

//	pDC->TextOut(16,16,"Splitter View 2 (CTDView)");


//	CListCtrl* List=(CListCtrl*)GetDlgItem(IDC_LISTVIEW);
	CListCtrl List;

	RECT ClientRect;
	GetClientRect(&ClientRect);
	List.Create(LVS_LIST,ClientRect,this,1);
}

/////////////////////////////////////////////////////////////////////////////
// CTDView diagnostics

#ifdef _DEBUG
void CTDView::AssertValid() const
{
	CView::AssertValid();
}

void CTDView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CBTEditDoc* CTDView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBTEditDoc)));
	return (CBTEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTDView message handlers
