// SnapPrefs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSnapPrefs dialog

// Possible values for SnapMode.
//enum {
//	SD_SNAPNONE,
//	SD_SNAPCENTER,
//	SD_SNAPTOPLEFT,
//	SD_SNAPTOPRIGHT,
//	SD_SNAPBOTTOMLEFT,
//	SD_SNAPBOTTOMRIGHT,
//	SD_SNAPQUAD,
//	SD_SNAPCUSTOM,
//};

class CSnapPrefs : public CDialog
{
// Construction
public:
	CSnapPrefs(CWnd* pParent = NULL);   // standard constructor

	void SetGravity(BOOL EnableGravity) { m_EnableGravity = EnableGravity; }
	void SetSnapMode(DWORD SnapMode) { m_SnapMode = SnapMode; }
	void SetSnapX(DWORD SnapX) { m_SnapX = SnapX; }
	void SetSnapZ(DWORD SnapZ) { m_SnapZ = SnapZ; }
	void SetRotSnap(DWORD RotSnap) { m_RotSnap = RotSnap; }

	BOOL GetGravity(void) { return m_EnableGravity; }
	DWORD GetSnapMode(void) { return m_SnapMode; }
	DWORD GetSnapX(void) { return m_SnapX; }
	DWORD GetSnapZ(void) { return m_SnapZ; }
	DWORD GetRotSnap(void) { return m_RotSnap; }

// Dialog Data
	//{{AFX_DATA(CSnapPrefs)
	enum { IDD = IDD_OBJECTPREFS };
	BOOL	m_EnableGravity;
	DWORD	m_SnapX;
	DWORD	m_SnapZ;
	DWORD	m_RotSnap;
	//}}AFX_DATA

	DWORD m_SnapMode;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSnapPrefs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSnapPrefs)
	afx_msg void OnSnapnone();
	afx_msg void OnSnaptcen();
	afx_msg void OnSnaptl();
	afx_msg void OnSnaptr();
	afx_msg void OnSnapbl();
	afx_msg void OnSnapbr();
	afx_msg void OnSnapcustom();
	afx_msg void OnSnapquad();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
