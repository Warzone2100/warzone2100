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

#include "savesegmentdialog.hpp"

//(*InternalHeaders(SaveSegmentDialog)
#include <wx/xrc/xmlres.h>
//*)

#include "numtextval.hpp"

//(*IdInit(SaveSegmentDialog)
//*)

BEGIN_EVENT_TABLE(SaveSegmentDialog,wxDialog)
	//(*EventTable(SaveSegmentDialog)
	//*)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////
// SaveSegmentDialog dialog


SaveSegmentDialog::SaveSegmentDialog(wxWindow* parent,
                                     unsigned int StartX_,
                                     unsigned int StartY_,
                                     unsigned int Width_,
                                     unsigned int Height_) :
	_StartX(StartX_),
	_StartY(StartY_),
	_Width(Width_),
	_Height(Height_)
{
	//(*Initialize(SaveSegmentDialog)
	wxXmlResource::Get()->LoadObject(this,parent,_T("SaveSegmentDialog"),_T("wxDialog"));
	StartX_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXT_STARTX")));
	StartY_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXT_STARTY")));
	Width_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXT_WIDTH")));
	Height_TextCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(XRCID("ID_TEXT_HEIGHT")));
	//*)

	// Make sure all characters entered into these text controls
	// are numeric and connected to our private member number variables
	StartX_TextCtrl->SetValidator(wxNumericTextValidator(&_StartX));
	StartY_TextCtrl->SetValidator(wxNumericTextValidator(&_StartY));
	Width_TextCtrl->SetValidator(wxNumericTextValidator(&_Width));
	Height_TextCtrl->SetValidator(wxNumericTextValidator(&_Height));
}

void SaveSegmentDialog::StartX(unsigned long StartX_)
{
	_StartX = StartX_;
}

void SaveSegmentDialog::StartY(unsigned long StartY_)
{
	_StartY = StartY_;
}

void SaveSegmentDialog::Width(unsigned long Width_)
{
	_Width = Width_;
}

void SaveSegmentDialog::Height(unsigned long Height_)
{
	_Height = Height_;
}

unsigned long SaveSegmentDialog::StartX() const
{
	return _StartX;
}

unsigned long SaveSegmentDialog::StartY() const
{
	return _StartY;
}

unsigned long SaveSegmentDialog::Width() const
{
	return _Width;
}

unsigned long SaveSegmentDialog::Height() const
{
	return _Height;
}
