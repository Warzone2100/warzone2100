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

#include "initiallimitsdialog.hpp"

//(*InternalHeaders(InitialLimitsDialog)
#include <wx/xrc/xmlres.h>
//*)

#include <wx/valgen.h>
#include "wxstringconv.hpp"

/////////////////////////////////////////////////////////////////////////////
// InitialLimitsDialog dialog

//(*IdInit(InitialLimitsDialog)
//*)

BEGIN_EVENT_TABLE(InitialLimitsDialog,wxDialog)
	//(*EventTable(InitialLimitsDialog)
	//*)
END_EVENT_TABLE()

void InitialLimitsDialog::initialize(wxWindow* parent)
{
	//(*Initialize(InitialLimitsDialog)
	wxXmlResource::Get()->LoadObject(this,parent,_T("InitialLimitsDialog"),_T("wxDialog"));
	InitialLimits_Choice = dynamic_cast<wxChoice*>(FindWindow(XRCID("ID_CHOICE_INITIALLIMITS")));
	//*)

	// Add the strings to the list box.
	for (std::deque<std::string>::const_iterator curString = _stringList.begin(); curString != _stringList.end(); ++curString)
	{
		InitialLimits_Choice->Append(std2wxString(*curString));
	}

	// Set the default selection, and make sure the selection is returned into _selected
	InitialLimits_Choice->SetValidator(wxGenericValidator(&_selected));
}

int InitialLimitsDialog::Selected() const
{
	return _selected;
}

std::string InitialLimitsDialog::SelectedString() const
{
	if (_selected == wxNOT_FOUND)
		return std::string();

	return _stringList.at(_selected);
}
