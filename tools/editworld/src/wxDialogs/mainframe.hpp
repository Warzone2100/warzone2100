/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007  Warzone Resurrection Project

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

#ifndef __INCLUDED_MAINFRAME_HPP__
#define __INCLUDED_MAINFRAME_HPP__

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    //(*Headers(MainFrame)
    #include <wx/menu.h>
    #include <wx/toolbar.h>
    #include <wx/frame.h>
    //*)
#endif

class MainFrame: public wxFrame
{
    public:
        MainFrame(wxWindow* parent = NULL);

        //(*Declarations(MainFrame)
        //*)

    private:
        //(*Identifiers(MainFrame)
        static const long ID_TOOLBARITEM_NEW;
        static const long ID_TOOLBARITEM_OPEN;
        static const long ID_TOOLBARITEM_SAVE;
        static const long ID_TOOLBARITEM_UNDO;
        static const long ID_TOOLBARITEM_REDO;
        static const long ID_TOOLBARITEM_POINT;
        static const long ID_TOOLBARITEM_MOVE;
        static const long ID_TOOLBARITEM_OBJECTROTATE;
        static const long ID_TOOLBARITEM_DRAW_GET;
        static const long ID_TOOLBARITEM_DRAW_PAINT;
        static const long ID_TOOLBARITEM_DRAW_FILL;
        static const long ID_TOOLBARITEM_DRAW_EDGEPAINT;
        static const long ID_TOOLBARITEM_DRAWBRUSHFILL;
        static const long ID_TOOLBARITEM_GATEWAY;
        static const long ID_TOOLBARITEM_HEIGHT_TILEMODE;
        static const long ID_TOOLBARITEM_HEIGHT_VERTEXMODE;
        static const long ID_TOOLBARITEM_HEIGHT_PICK;
        static const long ID_TOOLBARITEM_HEIGHT_PAINT;
        static const long ID_TOOLBARITEM_PAINTATSEALEVEL;
        static const long ID_TOOLBARITEM_HEIGHT_DRAG;
        static const long ID_TOOLBARITEM_MARKRECT;
        static const long ID_TOOLBARITEM_XFLIPMARKED;
        static const long ID_TOOLBARITEM_YFLIPMARKED;
        static const long ID_TOOLBARITEM_COPYMARKED;
        static const long ID_TOOLBARITEM_PASTE;
        static const long ID_TOOLBARITEM_PASTEPREFS;
        static const long ID_TOOLBARITEM_FILE_EXPORTCLIPBOARD;
        static const long ID_TOOLBARITEM_FILE_IMPORTCLIPBOARD;
        static const long ID_TOOLBAR_MAIN;
        static const long ID_MENUITEM_FILE_NEW;
        static const long ID_MENUITEM_FILE_LOADFEATURESET;
        static const long ID_MENUITEM_FILE_OPEN;
        static const long ID_MENUITEM_FILE_SAVE;
        static const long ID_MENUITEM_FILE_SAVE_AS;
        static const long ID_MENUITEM_FILE_SAVEMAPSEGMENT;
        static const long ID_MENUITEM_FILE_EXPORTCLIPBOARD;
        static const long ID_MENUITEM_FILE_IMPORTCLIPBOARD;
        static const long ID_MENUITEM_FILE_EXPORTWARZONESCENARIO;
        static const long ID_MENUITEM_FILE_EXPORTWARZONESCENARIOEXPANSION;
        static const long ID_MENUITEM_FILE_EXPORTWARZONESCENARIOMISSION;
        static const long ID_MENUITEM_FILE_SAVEEDGEBRUSHES;
        static const long ID_MENUITEM_FILE_LOADEDGEBRUSHES;
        static const long ID_MENUITEM_FILE_SAVETILETYPES;
        static const long ID_MENUITEM_FILE_LOADTILETYPES;
        static const long ID_MENUITEM_FILE_MOSTRECENTFILES;
        static const long ID_QUIT;
        static const long ID_MENUITEM_MAP_PREFERENCES;
        static const long ID_MENUITEM_MAP_IMPORTHEIGHTMAP;
        static const long ID_MENUITEM_MAP_EXPORTHEIGHTMAP;
        static const long ID_MENUITEM_MAP_EXPORTASBITMAP;
        static const long ID_MENUITEM_MAP_IMPORTTILEMAP;
        static const long ID_MENUITEM_MAP_EXPORTTILEMAP;
        static const long ID_MENUITEM_MAP_RESETTEXTUREFLAGS;
        static const long ID_MENUITEM_MAP_EDITSCROLLLIMITS;
        static const long ID_MENUITEM_MAP_REFRESHZONES;
        static const long ID_MENUITEM_VIEW_ZEROCAMERA;
        static const long ID_MENUITEM_VIEW_ZEROCAMERAPOSITION;
        static const long ID_MENUITEM_VIEW_LOCKCAMERA;
        static const long ID_MENUITEM_VIEW_FREECAMERA;
        static const long ID_MENUITEM_VIEW_WORLD;
        static const long ID_MENUITEM_VIEW_EDGEBRUSHES;
        static const long ID_MENUITEM_OPTIONS_TEXTURED;
        static const long ID_MENUITEM_OPTIONS_WIREFRAME;
        static const long ID_MENUITEM_OPTIONS_GOURADSHADING;
        static const long ID_MENUITEM_OPTIONS_AUTOHEIGHSET;
        static const long ID_MENUITEM_OPTIONS_SEALEVEL;
        static const long ID_MENUITEM_OPTIONS_TERRAINTYPES;
        static const long ID_MENUITEM_OPTIONS_OBJECTS;
        static const long ID_MENUITEM_OPTIONS_BOUNDINGSPHERES;
        static const long ID_MENUITEM_OPTIONS_ENABLEAUTOSNAP;
        static const long ID_MENUITEM_OPTIONS_USEINGAMENAMES;
        static const long ID_MENUITEM_OPTIONS_LOCATERMAPS;
        static const long ID_MENUITEM_OPTIONS_SHOWHEIGHTS;
        static const long ID_MENUITEM_OPTIONS_SHOWTEXTURES;
        static const long ID_MENUITEM_OPTIONS_LARGESCALE;
        static const long ID_APP_ABOUT;
        //*)

    private:
        //(*Handlers(MainFrame)
        void OnAbout(wxCommandEvent& event);
        //*)

    private:
        DECLARE_EVENT_TABLE()
};

#endif // __INCLUDED_MAINFRAME_HPP__
