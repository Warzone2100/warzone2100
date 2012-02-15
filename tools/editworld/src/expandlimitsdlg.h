/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#ifndef __INCLUDED_EXPANDLIMITS_H__
#define __INCLUDED_EXPANDLIMITS_H__

// ExpandLimitsDlg.h : header file
//

#include "directx.h"
#include "geometry.h"
#include "ddimage.h"
#include "heightmap.h"

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

#endif // __INCLUDED_EXPANDLIMITS_H__
