/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2013  Warzone 2100 Project

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

#include "mainframe.hpp"

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    //(*InternalHeaders(MainFrame)
    #include <wx/xrc/xmlres.h>
    //*)
#endif

#include "aboutdialog.hpp"

//(*IdInit(MainFrame)
//*)

BEGIN_EVENT_TABLE(MainFrame,wxFrame)
    //(*EventTable(MainFrame)
    //*)
END_EVENT_TABLE()

MainFrame::MainFrame(wxWindow* parent)
{
    //(*Initialize(MainFrame)
    wxXmlResource::Get()->LoadObject(this,parent,_T("MainFrame"),_T("wxFrame"));

    Connect(XRCID("ID_APP_ABOUT"),wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&MainFrame::OnAbout);
    //*)
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    AboutDialog().ShowModal();
}
