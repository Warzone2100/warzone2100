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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
# include <wx/app.h>
# include <wx/xrc/xmlres.h>
# include <wx/fs_arc.h>
# include <wx/image.h>
# include <wx/imagpng.h>
#endif

#include "wx/hyperlink.h"
#include "mainframe.hpp"

class EditWorldApp : public wxApp
{
    public:
        virtual bool OnInit()
        {
            wxFileSystem::AddHandler(new wxArchiveFSHandler);
            wxImage::AddHandler(new wxPNGHandler);
            wxXmlResource::Get()->InitAllHandlers();
            wxXmlResource::Get()->AddHandler(new wxHyperLink::XmlHandler);

            // Load the resource file containing all the windows & dialogs
            wxXmlResource::Get()->Load(_T("windows.xrs"));

            bool wxsOK = true;

            if (wxsOK)
            {
                // Constructing and showing our main window/frame
                MainFrame* frame = new MainFrame;
                frame->Show();
                SetTopWindow(frame);
            }

            return wxsOK;
        }
};

IMPLEMENT_APP(EditWorldApp);
