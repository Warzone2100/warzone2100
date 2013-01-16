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

#include "autoflagdialog.hpp"

//(*InternalHeaders(AutoFlagDialog)
#include <wx/xrc/xmlres.h>
//*)

//(*IdInit(AutoFlagDialog)
//*)

BEGIN_EVENT_TABLE(AutoFlagDialog,wxDialog)
	//(*EventTable(AutoFlagDialog)
	//*)
END_EVENT_TABLE()

AutoFlagDialog::AutoFlagDialog(wxWindow* parent,
                               bool RandRotate,
                               bool RandXFlip,
                               bool RandYFlip,
                               bool XFlip,
                               bool YFlip,
                               unsigned int Rotate,
                               bool IncRotate,
                               bool ToggleXFlip,
                               bool ToggleYFlip) :
	_RandRotate(RandRotate),
	_RandXFlip(RandXFlip),
	_RandYFlip(RandYFlip),
	_XFlip(XFlip),
	_YFlip(YFlip),
	_Rotate(Rotate),
	_IncRotate(IncRotate),
	_ToggleXFlip(ToggleXFlip),
	_ToggleYFlip(ToggleYFlip)
{
	//(*Initialize(AutoFlagDialog)
	wxXmlResource::Get()->LoadObject(this,parent,_T("AutoFlagDialog"),_T("wxDialog"));
	Degree0_RadioButton = dynamic_cast<wxRadioButton*>(FindWindow(XRCID("ID_RADIO_0DEGREE")));
	Degree90_RadioButton = dynamic_cast<wxRadioButton*>(FindWindow(XRCID("ID_RADIO_90DEGREE")));
	Degree180_RadioButton = dynamic_cast<wxRadioButton*>(FindWindow(XRCID("ID_RADIO_180DEGREE")));
	Degree270_RadioButton = dynamic_cast<wxRadioButton*>(FindWindow(XRCID("ID_RADIO_270DEGREE")));
	XFlip_CheckBox = dynamic_cast<wxCheckBox*>(FindWindow(XRCID("ID_CHK_XFLIP")));
	YFlip_CheckBox = dynamic_cast<wxCheckBox*>(FindWindow(XRCID("ID_CHK_YFLIP")));
	RandRotate_CheckBox = dynamic_cast<wxCheckBox*>(FindWindow(XRCID("ID_CHK_RANDROTATE")));
	RandXFlip_CheckBox = dynamic_cast<wxCheckBox*>(FindWindow(XRCID("ID_CHK_RANDXFLIP")));
	RandYFlip_CheckBox = dynamic_cast<wxCheckBox*>(FindWindow(XRCID("ID_CHK_RANDYFLIP")));
	
	Connect(XRCID("ID_RADIO_0DEGREE"),wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&AutoFlagDialog::OnRotate);
	Connect(XRCID("ID_RADIO_90DEGREE"),wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&AutoFlagDialog::OnRotate);
	Connect(XRCID("ID_RADIO_180DEGREE"),wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&AutoFlagDialog::OnRotate);
	Connect(XRCID("ID_RADIO_270DEGREE"),wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&AutoFlagDialog::OnRotate);
	Connect(XRCID("ID_CHK_XFLIP"),wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&AutoFlagDialog::OnXFlip);
	Connect(XRCID("ID_CHK_YFLIP"),wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&AutoFlagDialog::OnYFlip);
	Connect(XRCID("ID_CHK_RANDROTATE"),wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&AutoFlagDialog::OnRandRotate);
	Connect(XRCID("ID_CHK_RANDXFLIP"),wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&AutoFlagDialog::OnRandXFlip);
	Connect(XRCID("ID_CHK_RANDYFLIP"),wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&AutoFlagDialog::OnRandYFlip);
	//*)

	Connect(XRCID("wxID_OK"),wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&AutoFlagDialog::OnOK);
}

bool AutoFlagDialog::RandRotate() const
{
    return _RandRotate;
}

bool AutoFlagDialog::RandXFlip() const
{
    return _RandXFlip;
}

bool AutoFlagDialog::RandYFlip() const
{
    return _RandYFlip;
}

bool AutoFlagDialog::XFlip() const
{
    return _XFlip;
}

bool AutoFlagDialog::YFlip() const
{
    return _YFlip;
}

unsigned int AutoFlagDialog::Rotate() const
{
    return _Rotate;
}

bool AutoFlagDialog::IncRotate() const
{
    return _IncRotate;
}

bool AutoFlagDialog::ToggleXFlip() const
{
    return _ToggleXFlip;
}

bool AutoFlagDialog::ToggleYFlip() const
{
    return _ToggleYFlip;
}

void AutoFlagDialog::OnOK(wxCommandEvent& event)
{
	_IncRotate = false; //IncrementRotate_CheckBox->GetValue();
	_ToggleXFlip = false;  //ToggleXFlip_CheckBox->GetValue();
	_ToggleYFlip = false;  //ToggleYFlip_CheckBox->GetValue();
	_XFlip = XFlip_CheckBox->GetValue();
	_YFlip = XFlip_CheckBox->GetValue();
	_RandRotate = RandRotate_CheckBox->GetValue();
	_RandXFlip = RandXFlip_CheckBox->GetValue();
	_RandYFlip = RandYFlip_CheckBox->GetValue();

	if      (Degree0_RadioButton->GetValue())
		_Rotate = 0;
	else if (Degree90_RadioButton->GetValue())
		_Rotate = 1;
	else if (Degree180_RadioButton->GetValue())
		_Rotate = 2;
	else if (Degree270_RadioButton->GetValue())
		_Rotate = 3;

	event.Skip();
}

void AutoFlagDialog::OnRandRotate(wxCommandEvent& event)
{
    Degree0_RadioButton->SetValue(false);
    Degree90_RadioButton->SetValue(false);
    Degree180_RadioButton->SetValue(false);
    Degree270_RadioButton->SetValue(false);
}

void AutoFlagDialog::OnRandXFlip(wxCommandEvent& event)
{
    XFlip_CheckBox->SetValue(false);
}

void AutoFlagDialog::OnRandYFlip(wxCommandEvent& event)
{
    YFlip_CheckBox->SetValue(false);
}

void AutoFlagDialog::OnRotate(wxCommandEvent& event)
{
    RandRotate_CheckBox->SetValue(false);
}

void AutoFlagDialog::OnXFlip(wxCommandEvent& event)
{
    RandXFlip_CheckBox->SetValue(false);
}

void AutoFlagDialog::OnYFlip(wxCommandEvent& event)
{
    RandYFlip_CheckBox->SetValue(false);
}
