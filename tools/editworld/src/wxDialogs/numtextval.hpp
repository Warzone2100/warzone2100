/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2009  Warzone Resurrection Project

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

#ifndef __INCLUDE_NUMTEXTVAL_HPP__
#define __INCLUDE_NUMTEXTVAL_HPP__

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
  #include <wx/textctrl.h>
  #include <wx/validate.h>
#endif

class wxNumericTextValidator : public wxValidator
{
DECLARE_DYNAMIC_CLASS(wxNumericTextValidator)
    public:
        wxNumericTextValidator();
        wxNumericTextValidator(long* val);
        wxNumericTextValidator(unsigned long* val);
        wxNumericTextValidator(const wxNumericTextValidator& rhs);
        const wxNumericTextValidator& operator=(const wxNumericTextValidator& rhs);

        virtual wxObject* Clone() const;

        // Called when the value in the window must be validated.
        // This function can pop up an error message.
        virtual bool Validate(wxWindow *parent);

        // Called to transfer data to the window
        virtual bool TransferToWindow();

        // Called to transfer data from the window
        virtual bool TransferFromWindow();

        // Filter keystrokes
        void OnChar(wxKeyEvent& event);

    protected:
        inline bool CheckValidator() const
        {
            wxCHECK_MSG( m_validatorWindow, false,
                         _T("No window associated with validator") );
            wxCHECK_MSG( dynamic_cast<wxTextCtrl*>(m_validatorWindow) != NULL, false,
                         _T("wxNumericTextValidator is only for wxTextCtrl's") );

            return true;
        }

    private:
        DECLARE_EVENT_TABLE()

    private:
        long*          _longValue;
        unsigned long* _UlongValue;
};

#endif // __INCLUDE_NUMTEXTVAL_HPP__
