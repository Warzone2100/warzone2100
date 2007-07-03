// AutoFlagDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAutoFlagDialog dialog

class CAutoFlagDialog : public CDialog
{
// Construction
public:
	CAutoFlagDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAutoFlagDialog)
	enum { IDD = IDD_AUTOFLAGDIALOG };
	//}}AFX_DATA
	BOOL m_IncRotate;
	BOOL m_TogXFlip;
	BOOL m_TogYFlip;
	BOOL m_RandRotate;
	BOOL m_RandXFlip;
	BOOL m_RandYFlip;
	BOOL m_XFlip;
	BOOL m_YFlip;
	int m_Rotate;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoFlagDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAutoFlagDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChkrandrotate();
	afx_msg void OnChkrandxflip();
	afx_msg void OnChkrandyflip();
	afx_msg void OnRot0();
	afx_msg void OnRot180();
	afx_msg void OnRot270();
	afx_msg void OnRot90();
	afx_msg void OnXflip();
	afx_msg void OnYflip();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
