// BrushProp.h : header file
//

#ifndef __INCLUDED_BRUSHPROP__
#define __INCLUDED_BRUSHPROP__


/////////////////////////////////////////////////////////////////////////////
// CBrushProp dialog

// Valid values for m_BrushSize.
enum {
	EBP_BS_SMALL,	// Small brush
	EBP_BS_LARGE,	// Large brush
};

// Valid values for m_HeightSetting.
enum {
	EBP_HS_NOSET,	// Don't set heights
	EBP_HS_SETABB,	// Set absolute height
	EBP_HS_SEALEVEL,// Use sea level
};


#define WM_CLOSE_BRUSHPROP	(WM_USER+10)

class CBrushProp : public CDialog
{
// Construction
public:
	CBrushProp(CWnd* pParent = NULL);   // standard constructor
	CBrushProp(CView *pView);
	BOOL Create(void);
	void GetBrushData(void);
	void SetBrushData(void);

	CView *m_pView;
	int m_OldHeight;

// Dialog Data
	//{{AFX_DATA(CBrushProp)
	enum { IDD = IDD_EDGEBRUSH };
	CSliderCtrl	m_RandomSlider;
	CSliderCtrl	m_HeightSlider;
	int		m_Height;
	int		m_RandomRange;
	int		m_BrushID;
	int		m_BrushSize;
	int		m_HeightSetting;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrushProp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBrushProp)
	virtual void OnCancel();
	afx_msg void OnChangeHeightedit();
	afx_msg void OnChangeRandedit();
	afx_msg void OnChangeBrushid();
	afx_msg void OnBrushidnext();
	afx_msg void OnBrushidprev();
	afx_msg void OnBrushlarge();
	afx_msg void OnBrushsize();
	afx_msg void OnAbheight();
	afx_msg void OnSetheight();
	afx_msg void OnWaterheight();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
