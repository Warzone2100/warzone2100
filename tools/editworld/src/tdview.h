// TDView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTDView view

class CTDView : public CView
{
protected:
	CTDView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTDView)

// Attributes
public:
	CBTEditDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTDView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTDView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in BTEditView.cpp
inline CBTEditDoc* CTDView::GetDocument()
   { return (CBTEditDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
