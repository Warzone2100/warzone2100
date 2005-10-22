// ExpandLimitsDlg.h : header file
//

#include "DirectX.h"
#include "Geometry.h"
#include "DDImage.h"
#include "HeightMap.h"

/////////////////////////////////////////////////////////////////////////////
// CExpandLimitsDlg dialog

class CExpandLimitsDlg : public CDialog
{
// Construction
public:
	CExpandLimitsDlg(CHeightMap *World,CWnd* pParent = NULL);   // standard constructor
	int GetExcludeSelected(void) { return m_ExcludeSelected; }
	int GetIncludeSelected(void) { return m_IncludeSelected; }

// Dialog Data
	//{{AFX_DATA(CExpandLimitsDlg)
	enum { IDD = IDD_EXPANSIONLIMITS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpandLimitsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CHeightMap *m_World;
	int	m_ExcludeSelected;
	int	m_IncludeSelected;

	// Generated message map functions
	//{{AFX_MSG(CExpandLimitsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeExcludelist();
	afx_msg void OnSelchangeIncludelist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
