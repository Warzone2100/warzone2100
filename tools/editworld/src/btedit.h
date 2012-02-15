/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#ifndef __INCLUDED_BTEDIT_H__
#define __INCLUDED_BTEDIT_H__

// BTEdit.h : main header file for the BTEDIT application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include <string>

#define snprintf _snprintf

class CBTEditCommandLineInfo : public CCommandLineInfo
{
public:
	CBTEditCommandLineInfo() {
		m_bForceEmulation = FALSE;
		m_bForceHardware = FALSE;
		m_bForcePrimary = FALSE;
		m_bForceSecondary = FALSE;
		m_bForceRamp = FALSE;
		m_bForceRGB = FALSE;
		m_bMapSizePower2 = TRUE;
	}
	BOOL m_bForceEmulation;
	BOOL m_bForceHardware;
	BOOL m_bForcePrimary;
	BOOL m_bForceSecondary;
	BOOL m_bForceRamp;
	BOOL m_bForceRGB;
	BOOL m_bMapSizePower2;
	virtual void ParseParam(const char* pszParam,BOOL bFlag,BOOL bLast);
};

extern CBTEditCommandLineInfo g_cmdInfo;

#endif // __INCLUDED_BTEDIT_H__
