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

#ifndef __INCLUDE_AUTOFLAGDIALOG_HPP__
#define __INCLUDE_AUTOFLAGDIALOG_HPP__

#include <wx/wxprec.h>

//(*Headers(AutoFlagDialog)
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/dialog.h>
//*)

/////////////////////////////////////////////////////////////////////////////
// AutoFlagDialog dialog

class AutoFlagDialog : public wxDialog
{
	public:
		// Construction
		AutoFlagDialog(wxWindow* parent = NULL,
		               bool RandRotate = false,
		               bool RandXFlip = false,
		               bool RandYFlip = false,
		               bool XFlip = false,
		               bool YFlip = false,
		               unsigned int Rotate = 0,
		               bool IncRotate = false,
		               bool ToggleXFlip = false,
		               bool ToggleYFlip = false);

		bool RandRotate() const;
		bool RandXFlip() const;
		bool RandYFlip() const;
		bool XFlip() const;
		bool YFlip() const;
		unsigned int Rotate() const;
		bool IncRotate() const;
		bool ToggleXFlip() const;
		bool ToggleYFlip() const;

	public:
		//(*Identifiers(AutoFlagDialog)
		//*)

	private:
		//(*Handlers(AutoFlagDialog)
		void OnOK(wxCommandEvent& event);
		void OnRotate(wxCommandEvent& event);
		void OnXFlip(wxCommandEvent& event);
		void OnYFlip(wxCommandEvent& event);
		void OnRandRotate(wxCommandEvent& event);
		void OnRandXFlip(wxCommandEvent& event);
		void OnRandYFlip(wxCommandEvent& event);
		//*)

		//(*Declarations(AutoFlagDialog)
		wxRadioButton*  Degree0_RadioButton;
		wxRadioButton*  Degree90_RadioButton;
		wxRadioButton*  Degree180_RadioButton;
		wxRadioButton*  Degree270_RadioButton;
		wxCheckBox*  XFlip_CheckBox;
		wxCheckBox*  YFlip_CheckBox;
		wxCheckBox*  RandRotate_CheckBox;
		wxCheckBox*  RandXFlip_CheckBox;
		wxCheckBox*  RandYFlip_CheckBox;
		//*)

	private:
		DECLARE_EVENT_TABLE()

	private:
		bool _RandRotate;
		bool _RandXFlip;
		bool _RandYFlip;
		bool _XFlip;
		bool _YFlip;
		unsigned int _Rotate;
		bool _IncRotate;
		bool _ToggleXFlip;
		bool _ToggleYFlip;
};

#endif // __INCLUDE_AUTOFLAGDIALOG_HPP__
