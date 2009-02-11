/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#ifndef __INCLUDED_INITIALLIMITSDIALOG_HPP__
#define __INCLUDED_INITIALLIMITSDIALOG_HPP__

#include "resource.h"

#include <deque>
#include <string>

/////////////////////////////////////////////////////////////////////////////
// InitialLimitsDlg dialog

class InitialLimitsDlg : public CDialog
{
	// Construction
	public:
		template<typename InputIterator>
		InitialLimitsDlg(InputIterator first, InputIterator last, CWnd* parent = NULL) :
			CDialog(InitialLimitsDlg::IDD, parent),
			_selected(-1),
			InitialLimits_Choice(NULL)
		{
			for (; first != last; ++first)
			{
				_stringList.push_back(*first);
			}
			//{{AFX_DATA_INIT(InitialLimitsDlg)
				// NOTE: the ClassWizard will add member initialization here
			//}}AFX_DATA_INIT
		}

		int Selected() const;
		std::string SelectedString() const;

	// Dialog Data
		//{{AFX_DATA(InitialLimitsDlg)
		enum { IDD = IDD_INITIALLIMITS };
			// NOTE: the ClassWizard will add data members here
		//}}AFX_DATA


	// Overrides
	protected:
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(InitialLimitsDlg)
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	private:
		// Generated message map functions
		//{{AFX_MSG(InitialLimitsDlg)
		virtual BOOL OnInitDialog();
		afx_msg void OnSelchangeInitiallimits();
		//}}AFX_MSG

		CComboBox* InitialLimits_Choice;

	private:
		DECLARE_MESSAGE_MAP()

	private:
		int	_selected;
		std::deque<std::string> _stringList;
};


#endif // __INCLUDED_INITIALLIMITSDIALOG_HPP__
