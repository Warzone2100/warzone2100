// ObjectProperties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectProperties dialog

class CObjectProperties : public CDialog
{
// Construction
public:
	CObjectProperties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CObjectProperties)
	enum { IDD = IDD_OBJECTPROPERTIES };
	CString	m_Class;
	CString	m_Name;
	int		m_PlayerID;
	float	m_PosX;
	float	m_PosY;
	float	m_PosZ;
	int		m_RotX;
	int		m_RotY;
	int		m_RotZ;
	int		m_UniqueID;
	DWORD	m_GPosX;
	DWORD	m_GPosY;
	DWORD	m_GPosZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjectProperties)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
