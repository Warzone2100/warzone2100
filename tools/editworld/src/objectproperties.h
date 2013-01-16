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

#ifndef __INCLUDED_OBJECTPROPERTIES_H__
#define __INCLUDED_OBJECTPROPERTIES_H__

/////////////////////////////////////////////////////////////////////////////
// CObjectProperties dialog

class CObjectProperties : public CDialog
{
// Construction
public:
	CObjectProperties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CObjectProperties)
	enum { IDD = IDD_OBJECTPROPERTIES };
	CString	m_Class;
	CString	m_Name;
	int		m_PlayerID;
	float	m_PosX;
	float	m_PosY;
	float	m_PosZ;
	int		m_RotX;
	int		m_RotY;
	int		m_RotZ;
	int		m_UniqueID;
	DWORD	m_GPosX;
	DWORD	m_GPosY;
	DWORD	m_GPosZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjectProperties)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __INCLUDED_OBJECTPROPERTIES_H__
