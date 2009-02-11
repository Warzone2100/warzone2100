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

#ifndef __INCLUDED_SAVESEGMENTDIALOG_HPP__
#define __INCLUDED_SAVESEGMENTDIALOG_HPP__

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// SaveSegmentDialog dialog

class SaveSegmentDialog : public CDialog
{
	// Construction
	public:
		SaveSegmentDialog(CWnd* parent = NULL,
		                  unsigned int StartX_ = 0,
		                  unsigned int StartY_ = 0,
		                  unsigned int Width_ = 0,
		                  unsigned int Height_ = 0);   // default constructor


		void StartX(unsigned int StartX_);
		void StartY(unsigned int StartY_);
		void Width(unsigned int Width_);
		void Height(unsigned int Height_);

		unsigned int StartX() const;
		unsigned int StartY() const;
		unsigned int Width() const;
		unsigned int Height() const;

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(SaveSegmentDialog)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	private:

		// Generated message map functions
		//{{AFX_MSG(SaveSegmentDialog)
			// NOTE: the ClassWizard will add member functions here
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	private:
		// Dialog Data
		//{{AFX_DATA(SaveSegmentDialog)
		enum { IDD = IDD_SAVEMAPSEGMENT };
		unsigned int _StartX;
		unsigned int _StartY;
		unsigned int _Width;
		unsigned int _Height;
		//}}AFX_DATA
};

#endif // __INCLUDED_SAVESEGMENTDIALOG_HPP__
