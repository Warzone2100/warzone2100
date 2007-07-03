// LimitsDialog.h : header file
//

#include "directx.h"
#include "geometry.h"
#include "ddimage.h"
#include "heightmap.h"

/////////////////////////////////////////////////////////////////////////////
// CLimitsDialog dialog

class CLimitsDialog : public CDialog
{
// Construction
public:
	CLimitsDialog(CHeightMap *World,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLimitsDialog)
	enum { IDD = IDD_SCROLLLIMITS };
	int		m_MaxX;
	int		m_MaxZ;
	int		m_MinX;
	int		m_MinZ;
	CString	m_ScriptName;
	//}}AFX_DATA

	CHeightMap *m_World;
	int m_SelectedItemIndex;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLimitsDialog)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void RebuildList(void);
	BOOL GetEditFields(BOOL CheckDup,int &MinX,int &MinZ,int &MaxX,int &MaxZ,char *ScriptName);

	// Generated message map functions
	//{{AFX_MSG(CLimitsDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnGetdispinfoListlimits(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownListlimits(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedListlimits(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAddlimits();
	afx_msg void OnModify();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
