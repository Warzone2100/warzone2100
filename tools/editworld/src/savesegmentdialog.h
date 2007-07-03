// SaveSegmentDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaveSegmentDialog dialog

class CSaveSegmentDialog : public CDialog
{
// Construction
public:
	CSaveSegmentDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveSegmentDialog)
	enum { IDD = IDD_SAVEMAPSEGMENT };
	UINT	m_Height;
	UINT	m_Width;
	UINT	m_StartX;
	UINT	m_StartY;
	//}}AFX_DATA

	void SetWidth(UINT Width) { m_Width = Width; }
	void SetHeight(UINT Height) { m_Height = Height; }
	void SetStartX(UINT StartX) { m_StartX = StartX; }
	void SetStartY(UINT StartY) { m_StartY = StartY; }
	UINT GetWidth(void) { return m_Width; }
	UINT GetHeight(void) { return m_Height; }
	UINT GetStartX(void) { return m_StartX; }
	UINT GetStartY(void) { return m_StartY; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveSegmentDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSaveSegmentDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
