//////////////////////////////////////////////////////////////////////////////
// File:        hyperlink.h
// Purpose:     wxHyperLink control
// Maintainer:  Wyo
// Created:     2003-04-07
// RCS-ID:      $Id$
// Copyright:   (c) 2004 wxCode
// Licence:     wxWindows
//////////////////////////////////////////////////////////////////////////////

#ifndef _MY_HYPERLINK_H_
#define _MY_HYPERLINK_H_

#ifdef __GNUG__
    #pragma implementation "hyperlink.h"
#endif

//----------------------------------------------------------------------------
// information
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------
//! wxWidgets headers


//============================================================================
// declarations
//============================================================================

//----------------------------------------------------------------------------
//!


//----------------------------------------------------------------------------
//! wxHyperLink
class wxHyperLink: public wxControl {

DECLARE_DYNAMIC_CLASS (wxHyperLink)

public:

    //! default constructor
    wxHyperLink () {}

    //! create constructor
    wxHyperLink (wxWindow *parent,
                 wxWindowID id,
                 const wxString &label,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 long style = 0,
                 const wxString &name = _T("wxHyperLink")) {
        Create (parent, id, label, pos, size, style, name);
    }

    // function create
    bool Create (wxWindow *parent,
                 wxWindowID id,
                 const wxString &label,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 long style = 0,
                 const wxString &name = _T("wxHyperLink"));

    // event handlers
    void OnLinkActivate (wxMouseEvent &event);
    void OnPaint (wxPaintEvent &event);
    void OnWindowEnter (wxMouseEvent &event);
    void OnWindowLeave (wxMouseEvent &event);

    // size functions
    wxSize DoGetBestSize() const;

    // get/set settings
    wxCursor GetHoverCursor ();
    void SetHoverCursor (wxCursor cursor);
    wxColour GetHoverColour ();
    void SetHoverColour (wxColour colour);
    wxColour GetNormalColour ();
    void SetNormalColour (wxColour colour);
    wxColour GetVisitedColour ();
    void SetVisitedColour (wxColour colour);
    wxString GetURL ();
    void SetURL (const wxString &url);

private:

    //! hypertext variables
    wxString m_URL;
    bool m_Visited;

    //! style settings
    wxCursor m_HoverCursor;
    wxColour m_HoverColour;
    wxColour m_NormalColour;
    wxColour m_VisitedColour;
    wxColour m_BackgroundColour;

    // size variables
    wxCoord m_width;
    wxCoord m_height;

    //! execute according to mimetype
    void ExecuteLink (const wxString &link);

    DECLARE_EVENT_TABLE()
};

#endif // _MY_HYPERLINK_H_

