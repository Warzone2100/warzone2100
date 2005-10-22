// TexturePrefs.cpp : implementation file
//

#include "stdafx.h"
#include "BTEdit.h"
#include "TexturePrefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTexturePrefs dialog


CTexturePrefs::CTexturePrefs(CWnd* pParent /*=NULL*/)
	: CDialog(CTexturePrefs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTexturePrefs)
	m_TextureWidth = 64;
	m_TextureHeight = 64;
	//}}AFX_DATA_INIT
}


void CTexturePrefs::SetTextureSize(DWORD TextureWidth,DWORD TextureHeight)
{
	m_TextureWidth = TextureWidth;
	m_TextureHeight  = TextureHeight;
}

void CTexturePrefs::GetTextureSize(DWORD *TextureWidth,DWORD *TextureHeight)
{
	*TextureWidth = m_TextureWidth;
	*TextureHeight  = m_TextureHeight;
}

void CTexturePrefs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTexturePrefs)
	DDX_Text(pDX, IDC_TEXTUREHEIGHT, m_TextureWidth);
	DDV_MinMaxDWord(pDX, m_TextureWidth, 1, 256);
	DDX_Text(pDX, IDC_TEXTUREWIDTH, m_TextureHeight);
	DDV_MinMaxDWord(pDX, m_TextureHeight, 1, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTexturePrefs, CDialog)
	//{{AFX_MSG_MAP(CTexturePrefs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTexturePrefs message handlers
