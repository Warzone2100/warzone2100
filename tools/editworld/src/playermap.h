// PlayerMap.h : header file
//

#include "directx.h"
#include "geometry.h"
#include "ddimage.h"
#include "heightmap.h"

/////////////////////////////////////////////////////////////////////////////
// CPlayerMap dialog

class CPlayerMap : public CDialog
{
// Construction
public:
	CPlayerMap(CWnd* pParent = NULL);   // standard constructor
	int m_Selected;

// Dialog Data
	//{{AFX_DATA(CPlayerMap)
	enum { IDD = IDD_PLAYERMAP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlayerMap)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPlayerMap)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
