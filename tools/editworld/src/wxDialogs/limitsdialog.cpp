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

#include "limitsdialog.hpp"

#ifndef WX_PRECOMP
//(*InternalHeaders(LimitsDialog)
#include <wx/xrc/xmlres.h>
//*)
#endif

#include "numtextval.hpp"
#include "listctrlval.hpp"
#include "wxstringconv.hpp"
#include <cassert>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// LimitsDialog dialog

//(*IdInit(LimitsDialog)
//*)

IMPLEMENT_DYNAMIC_CLASS(LimitsDialog::ListCtrl, wxListCtrl)
IMPLEMENT_DYNAMIC_CLASS(LimitsDialog::ListCtrl::XmlHandler, wxListCtrlXmlHandler)

std::deque<CScrollLimits>::const_iterator LimitsDialog::firstLimit()
{
	return _limitsList.begin();
}

std::deque<CScrollLimits>::const_iterator LimitsDialog::lastLimit()
{
	return _limitsList.end();
}

BEGIN_EVENT_TABLE(LimitsDialog,wxDialog)
	//(*EventTable(LimitsDialog)
	//*)
END_EVENT_TABLE()

void LimitsDialog::initialize(wxWindow* parent)
{
	wxXmlResource::Get()->AddHandler(new LimitsDialog::ListCtrl::XmlHandler);

	//(*Initialize(LimitsDialog)
	wxXmlResource::Get()->LoadObject(this, parent, _T("LimitsDialog"), _T("wxDialog"));
	Limits_ListCtrl = dynamic_cast<ListCtrl*>(FindWindow(XRCID("ID_LISTCTRL_LISTLIMITS")));
	Name_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXTCTRL_NAME")));
	MinX_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXTCTRL_MINX")));
	MinZ_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXTCTRL_MINZ")));
	MaxX_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXTCTRL_MAXX")));
	MaxZ_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXTCTRL_MAXZ")));
	Connect(XRCID("ID_LISTCTRL_LISTLIMITS"), wxEVT_COMMAND_LIST_ITEM_FOCUSED, (wxObjectEventFunction)&LimitsDialog::OnLimits_ListCtrlItemFocused);
	Limits_ListCtrl->Connect(XRCID("ID_LISTCTRL_LISTLIMITS"), wxEVT_KEY_DOWN, (wxObjectEventFunction)&LimitsDialog::OnLimits_ListCtrlKeyDown, NULL, this);
	Connect(XRCID("ID_BUTTON_MODIFY"), wxEVT_COMMAND_BUTTON_CLICKED, (wxObjectEventFunction)&LimitsDialog::OnModify_ButtonClick);
	Connect(XRCID("ID_BUTTON_ADDLIMITS"), wxEVT_COMMAND_BUTTON_CLICKED, (wxObjectEventFunction)&LimitsDialog::OnAddLimits_ButtonClick);
	//*)

	Limits_ListCtrl->SetValidator(wxListCtrlValidator<std::deque<unsigned int> >(&_SelectedItems));
	MinX_TextCtrl->SetValidator(wxNumericTextValidator(&_MinX));
	MinZ_TextCtrl->SetValidator(wxNumericTextValidator(&_MinZ));
	MaxX_TextCtrl->SetValidator(wxNumericTextValidator(&_MaxX));
	MaxZ_TextCtrl->SetValidator(wxNumericTextValidator(&_MaxZ));

	Limits_ListCtrl->InsertColumn(0, wxT("Name"),      wxLIST_FORMAT_LEFT, 128);
	Limits_ListCtrl->InsertColumn(1, wxT("Unique ID"), wxLIST_FORMAT_LEFT, 64);
	Limits_ListCtrl->InsertColumn(2, wxT("Min X"),     wxLIST_FORMAT_LEFT, 48);
	Limits_ListCtrl->InsertColumn(3, wxT("Min Z"),     wxLIST_FORMAT_LEFT, 48);
	Limits_ListCtrl->InsertColumn(4, wxT("Max X"),     wxLIST_FORMAT_LEFT, 48);
	Limits_ListCtrl->InsertColumn(5, wxT("Max Z"),     wxLIST_FORMAT_LEFT, 48);

	Limits_ListCtrl->SetItemCount(_limitsList.size());
}

wxString LimitsDialog::ListCtrl::OnGetItemText(long item, long column) const
{
	if (!_limitsDlg
	 || item < 0
	 || static_cast<unsigned long>(item) >= _limitsDlg->_limitsList.size())
		return wxString();

	std::deque<CScrollLimits>::const_iterator ScrollLimits = _limitsDlg->_limitsList.begin();
	std::advance(ScrollLimits, item);

	switch (column)
	{
		case	0:
			return wxString(ScrollLimits->ScriptName, wxConvUTF8);

		case	1:
			return wxString::Format(_T("%d"), int(ScrollLimits->UniqueID));

		case	2:
			return wxString::Format(_T("%d"), ScrollLimits->MinX);

		case	3:
			return wxString::Format(_T("%d"), ScrollLimits->MinZ);

		case	4:
			return wxString::Format(_T("%d"), ScrollLimits->MaxX);

		case	5:
			return wxString::Format(_T("%d"), ScrollLimits->MaxZ);
	}

	return wxString();
}

void LimitsDialog::OnLimits_ListCtrlKeyDown(wxKeyEvent& event)
{
	switch (event.GetKeyCode())
	{
		case WXK_DELETE:
		{
			if (!wxWindow::TransferDataFromWindow()
			 || _SelectedItems.empty())
				break;

			const unsigned int SelectedItemIndex = _SelectedItems.front();
			assert(SelectedItemIndex < _limitsList.size());

			std::deque<CScrollLimits>::iterator toErase = _limitsList.begin();
			std::advance(toErase, SelectedItemIndex);
			_limitsList.erase(toErase);
			Limits_ListCtrl->DeleteItem(SelectedItemIndex);

			break;
		}
	}
}

void LimitsDialog::OnLimits_ListCtrlItemFocused(wxListEvent& event)
{
	if (!wxWindow::TransferDataFromWindow())
		return;

	if (_SelectedItems.empty())
	{
		_MinX = 0;
		_MinZ = 0;
		_MaxX = 0;
		_MaxZ = 0;

		Name_TextCtrl->Clear();
		wxWindow::TransferDataToWindow();

		return;
	}

	const unsigned int SelectedItemIndex = _SelectedItems.front();
	assert(SelectedItemIndex < _limitsList.size());

	CScrollLimits& limits = _limitsList[SelectedItemIndex];

	_MinX = limits.MinX;
	_MinZ = limits.MinZ;
	_MaxX = limits.MaxX;
	_MaxZ = limits.MaxZ;

	Name_TextCtrl->SetValue(wxString(limits.ScriptName, wxConvUTF8));
	wxWindow::TransferDataToWindow();
}

void LimitsDialog::OnAddLimits_ButtonClick(wxCommandEvent& event)
{
	CScrollLimits limits;

	limits.UniqueID = _limitsList.back().UniqueID + 1;

	if(!GetEditFields())
		return;

	limits.MinX = _MinX;
	limits.MinZ = _MinZ;
	limits.MaxX = _MaxX;
	limits.MaxZ = _MaxZ;
	strncpy(limits.ScriptName, _ScriptName.c_str(), sizeof(limits.ScriptName));
	// Guarantee to NUL terminate
	limits.ScriptName[sizeof(limits.ScriptName) - 1] = '\0';

	_limitsList.push_back(limits);

	Limits_ListCtrl->SetItemCount(_limitsList.size());
}

void LimitsDialog::OnModify_ButtonClick(wxCommandEvent& event)
{
	if (!GetEditFields()
	 || _SelectedItems.empty())
		return;

	const unsigned int SelectedItemIndex = _SelectedItems.front();
	assert(SelectedItemIndex < _limitsList.size());

	CScrollLimits& limits = _limitsList[SelectedItemIndex];

	limits.MinX = _MinX;
	limits.MinZ = _MinZ;
	limits.MaxX = _MaxX;
	limits.MaxZ = _MaxZ;
	strncpy(limits.ScriptName, _ScriptName.c_str(), sizeof(limits.ScriptName));
	// Guarantee to NUL terminate
	limits.ScriptName[sizeof(limits.ScriptName) - 1] = '\0';

	Limits_ListCtrl->RefreshItem(SelectedItemIndex);
}

bool LimitsDialog::GetEditFields()
{
	_ScriptName = wx2stdString(Name_TextCtrl->GetValue());

	if (!wxWindow::TransferDataFromWindow())
		return false;

	if (_MaxX <= _MinX
	 || _MaxZ <= _MinZ)
		return false;

	if (_MinX > _MapWidth
	 || _MinZ > _MapHeight
	 || _MaxX > _MapWidth
	 || _MaxZ > _MapHeight)
		return false;

	return true;
}

LimitsDialog::ListCtrl::ListCtrl() :
	_limitsDlg(dynamic_cast<LimitsDialog*>(GetParent()))
{
}

bool LimitsDialog::ListCtrl::Create(wxWindow* parent,
                                    wxWindowID id,
                                    const wxPoint& pos,
                                    const wxSize& size,
                                    long style,
                                    const wxValidator& validator,
                                    const wxString& name)
{
	_limitsDlg = dynamic_cast<LimitsDialog*>(parent);

	return wxListCtrl::Create(parent, id, pos, size, style, validator, name);
}

void LimitsDialog::ListCtrl::setLimitsDialog(LimitsDialog* limitsDlg)
{
	_limitsDlg = limitsDlg;
}

wxObject* LimitsDialog::ListCtrl::XmlHandler::DoCreateResource()
{
	XRC_MAKE_INSTANCE(list, ListCtrl)

	list->Create(m_parentAsWindow,
	             GetID(),
	             GetPosition(), GetSize(),
	             GetStyle(),
	             wxDefaultValidator,
	             GetName());

	// FIXME: add columns definition

	SetupWindow(list);

	return list;
}

bool LimitsDialog::ListCtrl::XmlHandler::CanHandle(wxXmlNode* node)
{
	return IsOfClass(node, wxT("LimitsDialog::ListCtrl"));
}
