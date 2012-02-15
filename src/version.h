/*
	This file is part of Warzone 2100.
	Copyright (C) 2004  Giel van Schijndel
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
*/

#ifndef __INCLUDED_VERSION_H__
#define __INCLUDED_VERSION_H__

#include "lib/framework/types.h"

/** Retrieve the low revision number
 *  \return the lowest revision number of the working copy from which we built
 */
extern unsigned int version_getLowRevision(void);

/** Retrieve the revision number
 *  \return the highest revision number of the working copy from which we built
 */
extern unsigned int version_getRevision(void);

/** Composes a simple version string.
 *
 *  If we compiled from a tag, i.e. the checkout URI started with "tags/%s/",
 *  the resulting string will be the "%s" portion of the URI.
 *
 *  When we compiled from a branch, i.e. the checkout URI started with
 *  "branches/%s/", the resulting string will be "%s branch <revision>".
 *
 *  If we compiled from trunk, i.e. the checkout URI started with "trunk/",
 *  the resulting string will be "TRUNK <revision>".
 *
 *  In all cases "<revision>" will be either "r<REVISION_NUMBER>" or
 *  "r<LOW_REVISION_NUMBER>:<HIGH_REVISION_NUMBER" in case of a mixed revision
 *  repository.
 */
extern const char* version_getVersionString(void);

/** Determines whether this version is compiled from a modified source tree.
 *  \return true if this version is compiled from modified sources, false when
 *          it is compiled from unmodified sources, or it cannot be determined
 *          whether changes have occurred.
 */
extern bool version_modified(void);

/** Retrieves the date at which this build was compiled.
 *  \return the date at which this build was made (uses __DATE__)
 */
extern const char* version_getBuildDate(void);

/** Retrieves the time at which this build was compiled.
 *  \return the time at which this build was made (uses __TIME__)
 */
extern const char* version_getBuildTime(void);

/** Retrieves the date at which the source of this build was committed.
 *  \return the date when this revision was committed to the subversion
 *          repository
 */
extern const char* version_getVcsDate(void);

/** Retrieves the time at which the source of this build was committed.
 *  \return the time when this revision was committed to the subversion
 *          repository
 */
extern const char* version_getVcsTime(void);

/** Composes a nicely formatted version string.
 *
 *  It is formatted as follows:
 *  "Version <version string> <working copy state> - Built <DATE><BUILD TYPE>"
 *
 *  - "<version string>" is the return value from version_getVersionString()
 *  - "<working copy state>" represents the modification and switch state
 *                           of the working copy from which this build was made.
 *  - "<DATE>" the date of building as returned by version_getBuildDate() or
 *             version_getVcsDate(); the latter is only used when the working
 *             copy has no local modifications.
 *  - "<BUILD TYPE>" the type of build produced (i.e. DEBUG or not)
 */
extern const char* version_getFormattedVersionString(void);

#endif // __INCLUDED_VERSION_H__
