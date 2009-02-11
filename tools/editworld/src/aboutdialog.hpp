/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#ifndef __INCLUDED_ABOUTDIALOG_HPP__
#define __INCLUDED_ABOUTDIALOG_HPP__

/////////////////////////////////////////////////////////////////////////////
// AboutDialog dialog used for App About

#include "resource.h"

class AboutDialog : public CDialog
{
	public:
		AboutDialog();

	// Dialog Data
		//{{AFX_DATA(AboutDialog)
		enum { IDD = IDD_ABOUTBOX };
		//}}AFX_DATA
};

#endif // __INCLUDED_ABOUTDIALOG_HPP__
