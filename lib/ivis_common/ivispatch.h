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
*/
/***************************************************************************/
/*
 * ivispatch.h
 *
 * patches old ivis defines to new pie defines allows both definitions to run concurrently .
 *
 */
/***************************************************************************/

#ifndef _ivispatch_h
#define _ivispatch_h

#include "lib/framework/frame.h"
#include "pietypes.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

//matrixstuff
#define iV_MatrixBegin				pie_MatBegin
#define iV_MatrixEnd				pie_MatEnd
#define iV_MatrixRotateX			pie_MatRotX
#define iV_MatrixRotateY			pie_MatRotY
#define iV_MatrixRotateZ			pie_MatRotZ
#define iV_TRANSLATE				pie_TRANSLATE

#endif // _ivispatch_h
