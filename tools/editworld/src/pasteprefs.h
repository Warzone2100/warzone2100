// PastePrefs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPastePrefs dialog

class CPastePrefs : public CDialog
{
// Construction
public:
	CPastePrefs(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPastePrefs)
	enum { IDD = IDD_PASTEPREFS };
	BOOL	m_Heights;
	BOOL	m_Objects;
	BOOL	m_Textures;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPastePrefs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPastePrefs)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
