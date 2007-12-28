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

#include "mainframe.hpp"

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    //(*InternalHeaders(MainFrame)
    #include <wx/artprov.h>
    #include <wx/bitmap.h>
    #include <wx/intl.h>
    #include <wx/image.h>
    #include <wx/string.h>
    //*)
#endif

#include "aboutdialog.hpp"

//(*IdInit(MainFrame)
const long MainFrame::ID_TOOLBARITEM_NEW = wxNewId();
const long MainFrame::ID_TOOLBARITEM_OPEN = wxNewId();
const long MainFrame::ID_TOOLBARITEM_SAVE = wxNewId();
const long MainFrame::ID_TOOLBARITEM_UNDO = wxNewId();
const long MainFrame::ID_TOOLBARITEM_REDO = wxNewId();
const long MainFrame::ID_TOOLBARITEM_POINT = wxNewId();
const long MainFrame::ID_TOOLBARITEM_MOVE = wxNewId();
const long MainFrame::ID_TOOLBARITEM_OBJECTROTATE = wxNewId();
const long MainFrame::ID_TOOLBARITEM_DRAW_GET = wxNewId();
const long MainFrame::ID_TOOLBARITEM_DRAW_PAINT = wxNewId();
const long MainFrame::ID_TOOLBARITEM_DRAW_FILL = wxNewId();
const long MainFrame::ID_TOOLBARITEM_DRAW_EDGEPAINT = wxNewId();
const long MainFrame::ID_TOOLBARITEM_DRAWBRUSHFILL = wxNewId();
const long MainFrame::ID_TOOLBARITEM_GATEWAY = wxNewId();
const long MainFrame::ID_TOOLBARITEM_HEIGHT_TILEMODE = wxNewId();
const long MainFrame::ID_TOOLBARITEM_HEIGHT_VERTEXMODE = wxNewId();
const long MainFrame::ID_TOOLBARITEM_HEIGHT_PICK = wxNewId();
const long MainFrame::ID_TOOLBARITEM_HEIGHT_PAINT = wxNewId();
const long MainFrame::ID_TOOLBARITEM_PAINTATSEALEVEL = wxNewId();
const long MainFrame::ID_TOOLBARITEM_HEIGHT_DRAG = wxNewId();
const long MainFrame::ID_TOOLBARITEM_MARKRECT = wxNewId();
const long MainFrame::ID_TOOLBARITEM_XFLIPMARKED = wxNewId();
const long MainFrame::ID_TOOLBARITEM_YFLIPMARKED = wxNewId();
const long MainFrame::ID_TOOLBARITEM_COPYMARKED = wxNewId();
const long MainFrame::ID_TOOLBARITEM_PASTE = wxNewId();
const long MainFrame::ID_TOOLBARITEM_PASTEPREFS = wxNewId();
const long MainFrame::ID_TOOLBARITEM_FILE_EXPORTCLIPBOARD = wxNewId();
const long MainFrame::ID_TOOLBARITEM_FILE_IMPORTCLIPBOARD = wxNewId();
const long MainFrame::ID_TOOLBAR_MAIN = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_NEW = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_LOADFEATURESET = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_OPEN = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_SAVE = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_SAVE_AS = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_SAVEMAPSEGMENT = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_EXPORTCLIPBOARD = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_IMPORTCLIPBOARD = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_EXPORTWARZONESCENARIO = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_EXPORTWARZONESCENARIOEXPANSION = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_EXPORTWARZONESCENARIOMISSION = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_SAVEEDGEBRUSHES = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_LOADEDGEBRUSHES = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_SAVETILETYPES = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_LOADTILETYPES = wxNewId();
const long MainFrame::ID_MENUITEM_FILE_MOSTRECENTFILES = wxNewId();
const long MainFrame::ID_QUIT = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_PREFERENCES = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_IMPORTHEIGHTMAP = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_EXPORTHEIGHTMAP = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_EXPORTASBITMAP = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_IMPORTTILEMAP = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_EXPORTTILEMAP = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_RESETTEXTUREFLAGS = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_EDITSCROLLLIMITS = wxNewId();
const long MainFrame::ID_MENUITEM_MAP_REFRESHZONES = wxNewId();
const long MainFrame::ID_MENUITEM_VIEW_ZEROCAMERA = wxNewId();
const long MainFrame::ID_MENUITEM_VIEW_ZEROCAMERAPOSITION = wxNewId();
const long MainFrame::ID_MENUITEM_VIEW_LOCKCAMERA = wxNewId();
const long MainFrame::ID_MENUITEM_VIEW_FREECAMERA = wxNewId();
const long MainFrame::ID_MENUITEM_VIEW_WORLD = wxNewId();
const long MainFrame::ID_MENUITEM_VIEW_EDGEBRUSHES = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_TEXTURED = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_WIREFRAME = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_GOURADSHADING = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_AUTOHEIGHSET = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_SEALEVEL = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_TERRAINTYPES = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_OBJECTS = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_BOUNDINGSPHERES = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_ENABLEAUTOSNAP = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_USEINGAMENAMES = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_LOCATERMAPS = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_SHOWHEIGHTS = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_SHOWTEXTURES = wxNewId();
const long MainFrame::ID_MENUITEM_OPTIONS_LARGESCALE = wxNewId();
const long MainFrame::ID_APP_ABOUT = wxNewId();
//*)

BEGIN_EVENT_TABLE(MainFrame,wxFrame)
    //(*EventTable(MainFrame)
    //*)
END_EVENT_TABLE()

MainFrame::MainFrame(wxWindow* parent)
{
    //(*Initialize(MainFrame)
    wxToolBarToolBase* ToolBarItem4;
    wxMenuItem* MenuItem31;
    wxMenuItem* MenuItem8;
    wxMenuItem* MenuItem33;
    wxMenuItem* MenuItem7;
    wxMenuItem* MenuItem26;
    wxToolBarToolBase* ToolBarItem9;
    wxMenuItem* MenuItem25;
    wxToolBarToolBase* ToolBarItem21;
    wxToolBarToolBase* ToolBarItem19;
    wxMenuItem* MenuItem40;
    wxToolBarToolBase* ToolBarItem26;
    wxToolBarToolBase* ToolBarItem3;
    wxToolBarToolBase* ToolBarItem12;
    wxMenuItem* MenuItem5;
    wxToolBarToolBase* ToolBarItem11;
    wxMenuItem* MenuItem2;
    wxMenu* Menu3;
    wxToolBarToolBase* ToolBarItem10;
    wxToolBarToolBase* ToolBarItem20;
    wxMenuItem* MenuItem46;
    wxToolBarToolBase* ToolBarItem15;
    wxMenuItem* MenuItem1;
    wxMenuItem* MenuItem4;
    wxMenuItem* MenuItem14;
    wxMenuItem* MenuItem36;
    wxMenuItem* MenuItem11;
    wxMenuItem* MenuItem29;
    wxToolBarToolBase* ToolBarItem23;
    wxMenuItem* MenuItem15;
    wxMenuItem* MenuItem22;
    wxMenuItem* MenuItem37;
    wxMenuItem* MenuItem32;
    wxMenuItem* MenuItem17;
    wxToolBarToolBase* ToolBarItem24;
    wxMenuItem* MenuItem13;
    wxMenu* Menu1;
    wxToolBarToolBase* ToolBarItem16;
    wxMenuItem* MenuItem42;
    wxMenuItem* MenuItem10;
    wxToolBarToolBase* ToolBarItem6;
    wxToolBarToolBase* ToolBarItem13;
    wxMenuItem* MenuItem12;
    wxMenuItem* MenuItem24;
    wxMenuItem* MenuItem27;
    wxMenuItem* MenuItem44;
    wxToolBarToolBase* ToolBarItem1;
    wxMenuItem* MenuItem39;
    wxMenuItem* MenuItem38;
    wxMenuBar* Main_MenuBar;
    wxMenuItem* MenuItem3;
    wxToolBar* Main_ToolBar;
    wxMenuItem* MenuItem20;
    wxMenuItem* MenuItem28;
    wxToolBarToolBase* ToolBarItem14;
    wxMenuItem* MenuItem6;
    wxToolBarToolBase* ToolBarItem22;
    wxMenuItem* MenuItem35;
    wxMenuItem* MenuItem23;
    wxToolBarToolBase* ToolBarItem5;
    wxToolBarToolBase* ToolBarItem28;
    wxMenuItem* MenuItem41;
    wxToolBarToolBase* ToolBarItem8;
    wxMenuItem* MenuItem21;
    wxToolBarToolBase* ToolBarItem27;
    wxToolBarToolBase* ToolBarItem25;
    wxMenuItem* MenuItem34;
    wxMenuItem* MenuItem16;
    wxMenuItem* MenuItem43;
    wxMenu* Menu2;
    wxMenuItem* MenuItem9;
    wxToolBarToolBase* ToolBarItem17;
    wxMenuItem* MenuItem45;
    wxMenuItem* MenuItem18;
    wxMenuItem* MenuItem47;
    wxMenuItem* MenuItem30;
    wxToolBarToolBase* ToolBarItem2;
    wxMenu* Menu5;
    wxMenu* Menu4;
    wxMenuItem* MenuItem19;
    wxToolBarToolBase* ToolBarItem18;
    wxToolBarToolBase* ToolBarItem7;

    Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("wxID_ANY"));
    SetClientSize(wxSize(716,488));
    Main_ToolBar = new wxToolBar(this, ID_TOOLBAR_MAIN, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxNO_BORDER, _T("ID_TOOLBAR_MAIN"));
    ToolBarItem1 = Main_ToolBar->AddTool(ID_TOOLBARITEM_NEW, _("New file"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_NEW")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem2 = Main_ToolBar->AddTool(ID_TOOLBARITEM_OPEN, _("Open file"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_FILE_OPEN")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem3 = Main_ToolBar->AddTool(ID_TOOLBARITEM_SAVE, _("Save file"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_FILE_SAVE")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    Main_ToolBar->AddSeparator();
    ToolBarItem4 = Main_ToolBar->AddTool(ID_TOOLBARITEM_UNDO, _("Undo"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_UNDO")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem5 = Main_ToolBar->AddTool(ID_TOOLBARITEM_REDO, _("Redo"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_REDO")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    Main_ToolBar->AddSeparator();
    ToolBarItem6 = Main_ToolBar->AddTool(ID_TOOLBARITEM_POINT, _("Point"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem7 = Main_ToolBar->AddTool(ID_TOOLBARITEM_MOVE, _("Move"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem8 = Main_ToolBar->AddTool(ID_TOOLBARITEM_OBJECTROTATE, _("Rotate Object"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    Main_ToolBar->AddSeparator();
    ToolBarItem9 = Main_ToolBar->AddTool(ID_TOOLBARITEM_DRAW_GET, _("Get"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem10 = Main_ToolBar->AddTool(ID_TOOLBARITEM_DRAW_PAINT, _("Paint"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem11 = Main_ToolBar->AddTool(ID_TOOLBARITEM_DRAW_FILL, _("Fill"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem12 = Main_ToolBar->AddTool(ID_TOOLBARITEM_DRAW_EDGEPAINT, _("Edge Paint"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem13 = Main_ToolBar->AddTool(ID_TOOLBARITEM_DRAWBRUSHFILL, _("Brush Fill"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem14 = Main_ToolBar->AddTool(ID_TOOLBARITEM_GATEWAY, _("Gateway"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    Main_ToolBar->AddSeparator();
    ToolBarItem15 = Main_ToolBar->AddTool(ID_TOOLBARITEM_HEIGHT_TILEMODE, _("Tilemode"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem16 = Main_ToolBar->AddTool(ID_TOOLBARITEM_HEIGHT_VERTEXMODE, _("Vertexmode"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem17 = Main_ToolBar->AddTool(ID_TOOLBARITEM_HEIGHT_PICK, _("Pick"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem18 = Main_ToolBar->AddTool(ID_TOOLBARITEM_HEIGHT_PAINT, _("Paint"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem19 = Main_ToolBar->AddTool(ID_TOOLBARITEM_PAINTATSEALEVEL, _("Paint at sealevel"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem20 = Main_ToolBar->AddTool(ID_TOOLBARITEM_HEIGHT_DRAG, _("Drag"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    Main_ToolBar->AddSeparator();
    ToolBarItem21 = Main_ToolBar->AddTool(ID_TOOLBARITEM_MARKRECT, _("Mark rect"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem22 = Main_ToolBar->AddTool(ID_TOOLBARITEM_XFLIPMARKED, _("X Flip Marked"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem23 = Main_ToolBar->AddTool(ID_TOOLBARITEM_YFLIPMARKED, _("Y Flip Marked"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem24 = Main_ToolBar->AddTool(ID_TOOLBARITEM_COPYMARKED, _("Copy Marked"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_COPY")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem25 = Main_ToolBar->AddTool(ID_TOOLBARITEM_PASTE, _("Paste"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_PASTE")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem26 = Main_ToolBar->AddTool(ID_TOOLBARITEM_PASTEPREFS, _("Paste Preferences"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem27 = Main_ToolBar->AddTool(ID_TOOLBARITEM_FILE_EXPORTCLIPBOARD, _("Export Clipboard"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    ToolBarItem28 = Main_ToolBar->AddTool(ID_TOOLBARITEM_FILE_IMPORTCLIPBOARD, _("Import Clipboard"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString);
    Main_ToolBar->Realize();
    SetToolBar(Main_ToolBar);
    Main_MenuBar = new wxMenuBar();
    Menu1 = new wxMenu();
    MenuItem8 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_NEW, _("&New"), wxEmptyString, wxITEM_NORMAL);
    MenuItem8->SetBitmap(wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_NEW")),wxART_OTHER));
    Menu1->Append(MenuItem8);
    MenuItem9 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_LOADFEATURESET, _("Load Data Set"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem9);
    Menu1->AppendSeparator();
    MenuItem10 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_OPEN, _("&Open Project...\tCTRL+O"), wxEmptyString, wxITEM_NORMAL);
    MenuItem10->SetBitmap(wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_FILE_OPEN")),wxART_OTHER));
    Menu1->Append(MenuItem10);
    MenuItem11 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_SAVE, _("&Save Project\tCTRL+S"), wxEmptyString, wxITEM_NORMAL);
    MenuItem11->SetBitmap(wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_FILE_SAVE")),wxART_OTHER));
    Menu1->Append(MenuItem11);
    MenuItem12 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_SAVE_AS, _("Save Project &As..."), wxEmptyString, wxITEM_NORMAL);
    MenuItem12->SetBitmap(wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_FILE_SAVE_AS")),wxART_OTHER));
    Menu1->Append(MenuItem12);
    MenuItem13 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_SAVEMAPSEGMENT, _("Save Map Segment"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem13);
    Menu1->AppendSeparator();
    MenuItem14 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_EXPORTCLIPBOARD, _("Export Clipboard"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem14);
    MenuItem15 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_IMPORTCLIPBOARD, _("Import Clipboard"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem15);
    Menu1->AppendSeparator();
    MenuItem16 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_EXPORTWARZONESCENARIO, _("Export Warzone Scenario Start"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem16);
    MenuItem17 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_EXPORTWARZONESCENARIOEXPANSION, _("Export Warzone Scenario Expansion"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem17);
    MenuItem18 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_EXPORTWARZONESCENARIOMISSION, _("Export Warzone Scenario Mission"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem18);
    Menu1->AppendSeparator();
    MenuItem19 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_SAVEEDGEBRUSHES, _("Export Edge Brushes"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem19);
    MenuItem20 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_LOADEDGEBRUSHES, _("Import Edge Brushes"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem20);
    MenuItem21 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_SAVETILETYPES, _("Export Tile Types"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem21);
    MenuItem22 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_LOADTILETYPES, _("Import Tile Types"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem22);
    Menu1->AppendSeparator();
    MenuItem23 = new wxMenuItem(Menu1, ID_MENUITEM_FILE_MOSTRECENTFILES, _("Most Recent Files"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem23);
    MenuItem23->Enable(false);
    Menu1->AppendSeparator();
    MenuItem24 = new wxMenuItem(Menu1, ID_QUIT, _("E&xit"), wxEmptyString, wxITEM_NORMAL);
    MenuItem24->SetBitmap(wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_QUIT")),wxART_OTHER));
    Menu1->Append(MenuItem24);
    Main_MenuBar->Append(Menu1, _("&File"));
    Menu2 = new wxMenu();
    MenuItem25 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_PREFERENCES, _("Map Preferences"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem25);
    Menu2->AppendSeparator();
    MenuItem26 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_IMPORTHEIGHTMAP, _("Import Height Map"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem26);
    MenuItem27 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_EXPORTHEIGHTMAP, _("Export Height Map"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem27);
    MenuItem28 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_EXPORTASBITMAP, _("Export Map As Bitmap"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem28);
    Menu2->AppendSeparator();
    MenuItem29 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_IMPORTTILEMAP, _("Import Tile Map"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem29);
    MenuItem30 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_EXPORTTILEMAP, _("Export Tile Map"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem30);
    Menu2->AppendSeparator();
    MenuItem31 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_RESETTEXTUREFLAGS, _("Reset Texture Flags"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem31);
    Menu2->AppendSeparator();
    MenuItem32 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_EDITSCROLLLIMITS, _("Edit Scroll Limits"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem32);
    Menu2->AppendSeparator();
    MenuItem33 = new wxMenuItem(Menu2, ID_MENUITEM_MAP_REFRESHZONES, _("Refresh Zones"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem33);
    Main_MenuBar->Append(Menu2, _("&Map"));
    Menu3 = new wxMenu();
    MenuItem2 = new wxMenuItem(Menu3, ID_MENUITEM_VIEW_ZEROCAMERA, _("Look North\tF1"), wxEmptyString, wxITEM_NORMAL);
    Menu3->Append(MenuItem2);
    MenuItem3 = new wxMenuItem(Menu3, ID_MENUITEM_VIEW_ZEROCAMERAPOSITION, _("Zero Camera Position"), wxEmptyString, wxITEM_NORMAL);
    Menu3->Append(MenuItem3);
    Menu3->AppendSeparator();
    MenuItem4 = new wxMenuItem(Menu3, ID_MENUITEM_VIEW_LOCKCAMERA, _("Lock Camera"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(MenuItem4);
    MenuItem5 = new wxMenuItem(Menu3, ID_MENUITEM_VIEW_FREECAMERA, _("Free Camera"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(MenuItem5);
    Menu3->AppendSeparator();
    MenuItem6 = new wxMenuItem(Menu3, ID_MENUITEM_VIEW_WORLD, _("World\tF2"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(MenuItem6);
    MenuItem7 = new wxMenuItem(Menu3, ID_MENUITEM_VIEW_EDGEBRUSHES, _("Edge Brushes\tF3"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(MenuItem7);
    Main_MenuBar->Append(Menu3, _("&View"));
    Menu4 = new wxMenu();
    MenuItem34 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_TEXTURED, _("Textured"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem34);
    MenuItem35 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_WIREFRAME, _("Wire Frame"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem35);
    MenuItem36 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_GOURADSHADING, _("Gouraud Shading"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem36);
    Menu4->AppendSeparator();
    MenuItem37 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_AUTOHEIGHSET, _("Auto Height Set"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem37);
    Menu4->AppendSeparator();
    MenuItem38 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_SEALEVEL, _("Sea Level"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem38);
    MenuItem39 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_TERRAINTYPES, _("Terrain Types"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem39);
    MenuItem40 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_OBJECTS, _("Objects"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem40);
    MenuItem41 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_BOUNDINGSPHERES, _("Bounding spheres"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem41);
    MenuItem42 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_ENABLEAUTOSNAP, _("Enable Auto Snap"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem42);
    MenuItem43 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_USEINGAMENAMES, _("Use In Game Names"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem43);
    Menu4->AppendSeparator();
    MenuItem44 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_LOCATERMAPS, _("Locator Maps"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem44);
    MenuItem45 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_SHOWHEIGHTS, _("Show Heights"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem45);
    MenuItem46 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_SHOWTEXTURES, _("Show Textures"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem46);
    MenuItem47 = new wxMenuItem(Menu4, ID_MENUITEM_OPTIONS_LARGESCALE, _("Large Scale"), wxEmptyString, wxITEM_CHECK);
    Menu4->Append(MenuItem47);
    Main_MenuBar->Append(Menu4, _("&Options"));
    Menu5 = new wxMenu();
    MenuItem1 = new wxMenuItem(Menu5, ID_APP_ABOUT, _("&About"), wxEmptyString, wxITEM_NORMAL);
    Menu5->Append(MenuItem1);
    Main_MenuBar->Append(Menu5, _("&Help"));
    SetMenuBar(Main_MenuBar);

    Connect(ID_APP_ABOUT,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&MainFrame::OnAbout);
    //*)
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    AboutDialog().ShowModal();
}
