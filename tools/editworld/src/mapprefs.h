/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDED_MAPPREFS_H__
#define __INCLUDED_MAPPREFS_H__

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

#endif // __INCLUDED_MAPPREFS_H__
