// InfoDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInfoDialog dialog

#define WM_GOODBYE (WM_USER+5)

class CInfoDialog : public CDialog
{
private:
	CView *m_pView;
	
// Construction
public:
	CInfoDialog(CWnd* pParent = NULL);   // standard constructor
	CInfoDialog(CView *pView);
	BOOL Create(void);

// Dialog Data
	//{{AFX_DATA(CInfoDialog)
	enum { IDD = IDD_DEBUGINFO };
	CString	m_Info;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInfoDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInfoDialog)
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
