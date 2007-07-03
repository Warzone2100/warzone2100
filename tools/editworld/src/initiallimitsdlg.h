// InitialLimitsDlg.h : header file
//

#include "directx.h"
#include "geometry.h"
#include "ddimage.h"
#include "heightmap.h"

/////////////////////////////////////////////////////////////////////////////
// CInitialLimitsDlg dialog

class CInitialLimitsDlg : public CDialog
{
// Construction
public:
	CInitialLimitsDlg(CHeightMap *World,CWnd* pParent = NULL);   // standard constructor
	int GetSelected(void) { return m_Selected; }

// Dialog Data
	//{{AFX_DATA(CInitialLimitsDlg)
	enum { IDD = IDD_INITIALLIMITS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInitialLimitsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CHeightMap *m_World;
	int	m_Selected;

	// Generated message map functions
	//{{AFX_MSG(CInitialLimitsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeInitiallimits();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
