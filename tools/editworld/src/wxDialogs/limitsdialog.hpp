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

#ifndef __INCLUDE__LIMITSDIALOG_HPP__
#define __INCLUDE__LIMITSDIALOG_HPP__

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
//(*Headers(LimitsDialog)
#include <wx/textctrl.h>
#include <wx/dialog.h>
//*)
  #include <wx/xrc/xmlres.h>
  #include <wx/xrc/xh_listc.h>
  #include <wx/listctrl.h>
  #include <wx/string.h>
#endif

#include "heightmap.h"
#include <deque>

/////////////////////////////////////////////////////////////////////////////
// LimitsDialog dialog

class LimitsDialog : public wxDialog
{
	// Construction
	public:
		template<typename InputIterator>
		LimitsDialog(InputIterator first, InputIterator last, unsigned int MapWidth, unsigned int MapHeight, wxWindow* parent = NULL) :
			_MaxX(0),
			_MaxZ(0),
			_MinX(0),
			_MinZ(0),
			_MapWidth(MapWidth),
			_MapHeight(MapHeight),
			_limitsList(first, last)
		{
			initialize(parent);
		}

	public:
		std::deque<CScrollLimits>::const_iterator firstLimit();
		std::deque<CScrollLimits>::const_iterator lastLimit();

		//(*Identifiers(LimitsDialog)
		//*)

	// Implementation
	private:
		void initialize(wxWindow* parent);
		bool GetEditFields();

		//(*Handlers(LimitsDialog)
		void OnAddLimits_ButtonClick(wxCommandEvent& event);
		void OnModify_ButtonClick(wxCommandEvent& event);
		void OnLimits_ListCtrlKeyDown(wxKeyEvent& event);
		void OnLimits_ListCtrlItemFocused(wxListEvent& event);
		//*)

		class ListCtrl : public wxListCtrl
		{
				DECLARE_DYNAMIC_CLASS(wxListCtrl)

			public:
				ListCtrl();
				bool Create(wxWindow* parent,
				            wxWindowID id,
				            const wxPoint& pos = wxDefaultPosition,
				            const wxSize& size = wxDefaultSize,
				            long style = wxLC_ICON,
				            const wxValidator& validator = wxDefaultValidator,
				            const wxString& name = wxListCtrlNameStr);

				void setLimitsDialog(LimitsDialog* limitsDlg);

				virtual wxString OnGetItemText(long item, long column) const;

				class XmlHandler : public wxListCtrlXmlHandler
				{
						DECLARE_DYNAMIC_CLASS(XmlHandler);

					public:
						virtual wxObject* DoCreateResource();
						virtual bool CanHandle(wxXmlNode* node);
				};

			private:
				LimitsDialog* _limitsDlg;
		};

		//(*Declarations(LimitsDialog)
		ListCtrl*    Limits_ListCtrl;
		wxTextCtrl*  Name_TextCtrl;
		wxTextCtrl*  MinX_TextCtrl;
		wxTextCtrl*  MinZ_TextCtrl;
		wxTextCtrl*  MaxX_TextCtrl;
		wxTextCtrl*  MaxZ_TextCtrl;
		//*)

	private:
		DECLARE_EVENT_TABLE()

	private:
		unsigned long _MaxX;
		unsigned long _MaxZ;
		unsigned long _MinX;
		unsigned long _MinZ;
		std::string   _ScriptName;

		const unsigned int _MapWidth;
		const unsigned int _MapHeight;

		std::deque<unsigned int> _SelectedItems;

		std::deque<CScrollLimits> _limitsList;
};

#endif // __INCLUDE__LIMITSDIALOG_HPP__
