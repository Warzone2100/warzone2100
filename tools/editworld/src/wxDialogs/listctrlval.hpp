/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2009  Warzone Resurrection Project
	Copyright (C) 1999  Julian Smart (assigned from Kevin Smith)
	Copyright (C) 1999  Kevin Smith

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

#ifndef __INCLUDE_LISTCTRLVAL_HPP__
#define __INCLUDE_LISTCTRLVAL_HPP__

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
  #include <wx/dynarray.h>
  #include <wx/listctrl.h>
  #include <wx/validate.h>
  #include <wx/utils.h>
#endif

#include <typeinfo>

template <class Container>
class wxListCtrlValidator : public wxValidator
{
// Macro DECLARE_CLASS and IMPLEMENT_CLASS expanded so that we can use templates:
    public:
        static wxClassInfo ms_classInfo;
        virtual wxClassInfo *GetClassInfo() const
        {
            return &ms_classInfo;
        }

        //static wxObject* wxCreateObject();

    public:
        wxListCtrlValidator(Container* val) :
            _SelectionContainer(val)
        {}

        wxListCtrlValidator(const wxListCtrlValidator& rhs) :
            _SelectionContainer(rhs._SelectionContainer)
        {
            wxValidator::Copy(rhs);
        }

        wxListCtrlValidator& operator=(const wxListCtrlValidator& rhs)
        {
            wxValidator::Copy(rhs);
            _SelectionContainer = rhs._SelectionContainer;
        }

        // Make a clone of this validator (or return NULL) - currently necessary
        // if you're passing a reference to a validator.
        // Another possibility is to always pass a pointer to a new validator
        // (so the calling code can use a copy constructor of the relevant class).
        virtual wxObject* Clone() const
        {
            return new wxListCtrlValidator(*this);
        }

        // Called when the value in the window must be validated.
        // This function can pop up an error message.
        virtual bool Validate(wxWindow* parent)
        {
            return CheckValidator();
        }

        // Called to transfer data to the window
        virtual bool TransferToWindow()
        {
            if (!CheckValidator())
                return false;

            wxListCtrl* control = dynamic_cast<wxListCtrl*>(m_validatorWindow);

            if (!control)
                return false;

            size_t count = control->GetItemCount();

            // clear all selections
            for (size_t i = 0; i < count; ++i)
                control->SetItemState(i, 0, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);

            // select each item in our array
            for (typename Container::const_iterator i = _SelectionContainer->begin(); i != _SelectionContainer->end(); ++i)
                control->SetItemState(*i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

            return true;
        }

        // Called to transfer data to the window
        virtual bool TransferFromWindow()
        {
            if (!CheckValidator())
                return false;

            wxListCtrl* control = dynamic_cast<wxListCtrl*>(m_validatorWindow);

            if (!control)
                return false;

            // clear our array
            _SelectionContainer->clear();

            // add each selected item to our array
            const size_t count = control->GetItemCount();
            for (size_t i = 0; i < count; ++i)
            {
                if (control->GetItemState(i, wxLIST_STATE_SELECTED))
                    _SelectionContainer->push_back(i);
            }

            return true;
        }

    protected:
        inline bool CheckValidator() const
        {
            wxCHECK_MSG( m_validatorWindow, false,
                         _T("No window associated with validator") );
            wxCHECK_MSG( dynamic_cast<wxListCtrl*>(m_validatorWindow) != NULL, false,
                         _T("wxListCtrlValidator is only for wxListCtrl's") );
            wxCHECK_MSG( _SelectionContainer, false,
                         _T("wxListCtrlValidator needs a valid pointer to a container (as per the C++ standard's specs) of integers") );

            return true;
        }

    private:
        Container* _SelectionContainer;
};

template <typename Container>
wxClassInfo wxListCtrlValidator<Container>::ms_classInfo(wxString::Format(_T("wxListCtrlValidator<%s>"), typeid(Container).name()),
                                                         &wxValidator::ms_classInfo,
                                                         NULL,
                                                         static_cast<int>(sizeof(wxListCtrlValidator<Container>)),
                                                         reinterpret_cast<wxObjectConstructorFn>(NULL));

#endif // __INCLUDE_LISTCTRLVAL_HPP__
