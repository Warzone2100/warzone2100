/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2012  Warzone 2100 Project

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

#include "numtextval.hpp"

#ifndef WX_PRECOMP
  #include <wx/utils.h>
  #include <wx/msgdlg.h>
  #include <wx/intl.h>
#endif

IMPLEMENT_DYNAMIC_CLASS(wxNumericTextValidator, wxValidator)

BEGIN_EVENT_TABLE(wxNumericTextValidator, wxValidator)
    EVT_CHAR(wxNumericTextValidator::OnChar)
END_EVENT_TABLE()

wxNumericTextValidator::wxNumericTextValidator() :
    _longValue(NULL),
    _UlongValue(NULL)
{
}

wxNumericTextValidator::wxNumericTextValidator(long* val) :
    _longValue(val),
    _UlongValue(NULL)
{
}

wxNumericTextValidator::wxNumericTextValidator(unsigned long* val) :
    _longValue(NULL),
    _UlongValue(val)
{
}

wxNumericTextValidator::wxNumericTextValidator(const wxNumericTextValidator& rhs) :
    _longValue(rhs._longValue),
    _UlongValue(rhs._UlongValue)
{
    wxValidator::Copy(rhs);
}

const wxNumericTextValidator& wxNumericTextValidator::operator=(const wxNumericTextValidator& rhs)
{
    wxValidator::Copy(rhs);
    _longValue = rhs._longValue;
    _UlongValue = rhs._UlongValue;

    return *this;
}

wxObject* wxNumericTextValidator::Clone() const
{
    return new wxNumericTextValidator(*this);
}

static inline bool wxIsNumeric(int keycode)
{
    return keycode == wxT('.')
        || keycode == wxT(',')
        || keycode == wxT('e')
        || keycode == wxT('E')
        || keycode == wxT('+')
        || keycode == wxT('-')
        || wxIsdigit(keycode);
}

static bool wxIsNumeric(const wxString& val)
{
    for (unsigned int i = 0; i < val.Length(); ++i)
    {
        // Allow for "," (French) as well as "." -- in future we should
        // use wxSystemSettings or other to do better localisation
        if (!wxIsNumeric(val[i]))
            return false;
    }

    return true;
}

// Called when the value in the window must be validated.
// This function can pop up an error message.
bool wxNumericTextValidator::Validate(wxWindow *parent)
{
    if (!CheckValidator())
        return false;

    wxTextCtrl* control = dynamic_cast<wxTextCtrl*>(m_validatorWindow);

    if (!control)
        return false;

    // If window is disabled, simply return
    if ( !control->IsEnabled() )
        return true;

    if (!wxIsNumeric(control->GetValue()))
    {
        m_validatorWindow->SetFocus();

        wxString buf;
        buf.Printf(_("'%s' should be numeric."), control->GetValue().c_str());

        wxMessageBox(buf, _("Validation conflict"),
                     wxOK | wxICON_EXCLAMATION, parent);

        return false;
    }

    return true;
}

// Called to transfer data to the window
bool wxNumericTextValidator::TransferToWindow()
{
    if (!CheckValidator())
        return false;

    if (!_longValue
     && !_UlongValue)
        return true;

    wxTextCtrl* control = dynamic_cast<wxTextCtrl*>(m_validatorWindow);

    if (!control)
        return false;

    if (_longValue)
        control->SetValue(wxString::Format(_T("%d"), *_longValue));
    else
        control->SetValue(wxString::Format(_T("%u"), *_UlongValue));

    return true;
}

// Called to transfer data from the window
bool wxNumericTextValidator::TransferFromWindow()
{
    if (!CheckValidator())
        return false;

    if (!_longValue
     && !_UlongValue)
        return true;

    wxTextCtrl* control = dynamic_cast<wxTextCtrl*>(m_validatorWindow);

    if (!control)
        return false;

    if (_longValue)
        control->GetValue().ToLong(_longValue);
    else
        control->GetValue().ToULong(_UlongValue);

    return true;
}

// Filter keystrokes
void wxNumericTextValidator::OnChar(wxKeyEvent& event)
{
    if (!m_validatorWindow)
        return;

    int keycode = event.GetKeyCode();
    // we don't filter special keys and Delete
    if (keycode < WXK_SPACE || keycode == WXK_DELETE || keycode > WXK_START)
    {
        // Don't disable following event handlers in the chain (i.e. use the key)
        event.Skip();
        return;
    }

    if (!wxIsNumeric(keycode))
    {
        if (!wxValidator::IsSilent())
            wxBell();

        // eat message
        return;
    }

    // Don't disable following event handlers in the chain (i.e. use the key)
    event.Skip();
}
