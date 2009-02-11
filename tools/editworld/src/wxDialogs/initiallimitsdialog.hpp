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

#ifndef __INCLUDE__INITIALLIMITSDIALOG_HPP__
#define __INCLUDE__INITIALLIMITSDIALOG_HPP__

#include <wx/wxprec.h>

//(*Headers(InitialLimitsDialog)
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/dialog.h>
//*)

#include <deque>
#include <string>

/////////////////////////////////////////////////////////////////////////////
// InitialLimitsDialog dialog

class InitialLimitsDialog : public wxDialog
{
	// Construction
	public:
		template<typename InputIterator>
		InitialLimitsDialog(InputIterator first, InputIterator last, wxWindow* parent = NULL) :
			_selected(0),
			_stringList(first, last)
		{
			initialize(parent);
		}

		int Selected() const;
		std::string SelectedString() const;

		//(*Identifiers(InitialLimitsDialog)
		//*)


	// Implementation
	private:
		void initialize(wxWindow* parent);

		//(*Handlers(InitialLimitsDialog)
		//*)

		//(*Declarations(InitialLimitsDialog)
		wxChoice*  InitialLimits_Choice;
		//*)

	private:
		DECLARE_EVENT_TABLE()

	private:
		int _selected;
		std::deque<std::string> _stringList;
};

#endif // __INCLUDE__INITIALLIMITSDIALOG_HPP__
