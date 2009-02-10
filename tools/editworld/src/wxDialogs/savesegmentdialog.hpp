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

#ifndef __INCLUDE_SAVESEGMENTDIALOG_HPP__
#define __INCLUDE_SAVESEGMENTDIALOG_HPP__

#include <wx/wxprec.h>

//(*Headers(SaveSegmentDialog)
#include <wx/textctrl.h>
#include <wx/dialog.h>
//*)

/////////////////////////////////////////////////////////////////////////////
// SaveSegmentDialog dialog

class SaveSegmentDialog: public wxDialog
{
	// Construction
	public:
		SaveSegmentDialog(wxWindow* parent = NULL,
		                  unsigned int StartX_ = 0,
		                  unsigned int StartY_ = 0,
		                  unsigned int Width_ = 0,
		                  unsigned int Height_ = 0);   // default constructor

		//(*Identifiers(SaveSegmentDialog)
		//*)

		void StartX(unsigned long StartX_);
		void StartY(unsigned long StartY_);
		void Width(unsigned long Width_);
		void Height(unsigned long Height_);

		unsigned long StartX() const;
		unsigned long StartY() const;
		unsigned long Width() const;
		unsigned long Height() const;

	// Implementation
	private:
		//(*Handlers(SaveSegmentDialog)
		//*)

		//(*Declarations(SaveSegmentDialog)
		wxTextCtrl*  StartX_TextCtrl;
		wxTextCtrl*  StartY_TextCtrl;
		wxTextCtrl*  Width_TextCtrl;
		wxTextCtrl*  Height_TextCtrl;
		//*)

	private:
		DECLARE_EVENT_TABLE()

	private:
		unsigned long _StartX;
		unsigned long _StartY;
		unsigned long _Width;
		unsigned long _Height;
};

#endif // __INCLUDE_SAVESEGMENTDIALOG_HPP__
