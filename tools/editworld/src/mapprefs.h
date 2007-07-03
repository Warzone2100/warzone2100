// MapPrefs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapPrefs dialog

class CMapPrefs : public CDialog
{
// Construction
public:
	CMapPrefs(CWnd* pParent = NULL);   // standard constructor
	void SetMapWidth(DWORD MapWidth);
	DWORD GetMapWidth(void);
	void SetMapHeight(DWORD MapHeight);
	DWORD GetMapHeight(void);
	void SetMapSpacing(DWORD MapSpacing);
	DWORD GetMapSpacing(void);
	void SetTextureSize(DWORD TextureSize);
	DWORD GetTextureSize(void);
	void SetSeaLevel(DWORD SeaLevel);
	DWORD GetSeaLevel(void);
	void SetHeightScale(DWORD HeightScale);
	DWORD GetHeightScale(void);



// Dialog Data
	//{{AFX_DATA(CMapPrefs)
	enum { IDD = IDD_MAPPREFS };
	DWORD	m_MapHeight;
	DWORD	m_MapWidth;
	DWORD	m_MapSpacing;
	DWORD	m_SeaLevel;
	DWORD	m_TextureSize;
	DWORD	m_HeightScale;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapPrefs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMapPrefs)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
