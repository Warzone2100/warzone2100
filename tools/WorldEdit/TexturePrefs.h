// TexturePrefs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTexturePrefs dialog

class CTexturePrefs : public CDialog
{
// Construction
public:
	CTexturePrefs(CWnd* pParent = NULL);   // standard constructor
	void SetTextureSize(DWORD TextureWidth,DWORD TextureHeight);
	void GetTextureSize(DWORD *TextureWidth,DWORD *TextureHeight);

// Dialog Data
	//{{AFX_DATA(CTexturePrefs)
	enum { IDD = IDD_TEXTUREPREFS };
	DWORD	m_TextureWidth;
	DWORD	m_TextureHeight;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTexturePrefs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTexturePrefs)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
