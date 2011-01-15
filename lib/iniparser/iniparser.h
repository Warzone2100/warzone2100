/*
	This file is part of Warzone 2100.
	Copyright (C) 2010  Freddie Witherden <freddie@witherden.org>
	Copyright (C) 2010  Warzone 2100 Project

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

#ifndef _INIPARSER_H_
#define _INIPARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Opaque handle to an inifile structure.
typedef struct _inifile inifile;

/**
 * Returns a handle to a new inifile instance. This should be used when creating
 * a new inifile as opposed to loading an existing one.
 *
 * @return A handle to a newly allocated inifile.
 */
inifile *inifile_new();

/**
 * Loads the inifile referenced by path. Should the inifile referenced by path
 * not exist or be invalid NULL is returned.
 *
 * @param path The path to the inifile to load.
 * @return A handle to the inifile.
 */
inifile *inifile_load(const char *path);

/**
 * Returns the number of sections in inif.
 *
 * @param inif The inifile handle.
 * @return The number of sections in inif.
 */
int inifile_get_section_count(inifile *inif);

/**
 * Returns the `n'-th section in the inifile inif. The section number is
 * subject to the constraint that 0 <= n < get_section_count().
 *
 * @param inif The inifile handle.
 * @param n The number of the section, indexed from 0.
 * @return A pointer to the section name. Valid until either the section or
 *         inifile is deleted.
 */
const char *inifile_get_section(inifile *inif, int n);

/**
 * Returns a pointer to the currently active section in the inifile inif.
 *
 * @param inif The inifile handle.
 * @return A pointer to the section name.
 */
const char *inifile_get_current_section(inifile *inif);

/**
 * Sets the currently active section in inif to sec. If the section does not
 * exist in the inifile then a new one is created.
 *
 * @param inif The inifile handle.
 * @param sec The section to use. This is duplicated before the function
 *            returns.
 */
void inifile_set_current_section(inifile *inif, const char *sec);

/**
 * Checks to see if the key, key, exists in the inifile inif.
 *
 * @param inif The inifile handle.
 * @param key The key to check the existence of.
 * @return True if the key exists, false otherwise.
 */
BOOL inifile_key_exists(inifile *inif, const char *key);

/**
 * Fetches the value of the key specified by key in the inifile inif. If the key
 * does not exist then dflt is returned. The pointer returned is valid until
 * either the key is set/unset or the inifile is deleted.
 *
 * It is necessary to specify the section to search beforehand using the
 * set_current_section method.
 *
 * @param inif The inifile handle.
 * @param key The key to fetch the value for.
 * @param dflt The default string to return if the key is not found.
 * @return The value associated with the key, dflt otherwise.
 */
const char *inifile_get(inifile *inif, const char *key, const char *dflt);

/**
 * Sets the entry \a key to be \a value.  If \a key already exists its value is
 * updated.
 *
 * @param inif The inifile handle.
 * @param key The key to set/updated.
 * @param value The value to set.
 */
void inifile_set(inifile *inif, const char *key, const char *value);

/**
 * A helper function provided for convenience. Automatically converts the
 * result returned by get method to an integer.
 *
 * @param inif The inifile handle.
 * @param key The key to fetch the value for.
 * @param dflt The default integer to return if the key is not found.
 * @return The integer value associated with the key, dflt otherwise.
 */
int inifile_get_as_int(inifile *inif, const char *key, int dflt);

/**
 * A helper function provided for convenience. Automatically converts the
 * integer to a string and sets the \a key entry in the current section to
 * be that value.
 *
 * @param inif The inifile handle.
 * @param key The key to set.
 * @param value The integer value to set the key to.
 */
void inifile_set_as_int(inifile *inif, const char *key, int value);

/**
 * A helper function provided for convenience.  As \a dflt is an integer it can
 * be used to help determine if \a key exists or not by passing say -1.
 *
 * @param inif The inifile handle.
 * @param key The key to get.
 * @param dflt Default value to return should the key not exist.
 * @return The value associated with the key converted to a boolean value, or
 *         \a dflt should the key not exist.
 */
int inifile_get_as_bool(inifile *inif, const char *key, int dflt);

/**
 * Removes \a key from the \a inif in the section \a section.
 *
 * @param inif The inifile handle.
 * @param section The section to remove the key from.
 * @param key The key to remove.
 */
void inifile_unset(inifile *inif, const char *section, const char *key);

/**
 * Saves the inifile back to the file which is was loaded from. This is only
 * meaningful if the file was loaded using inifile_load.
 *
 * @param inif The inifile handle.
 */
void inifile_save(inifile *inif);

/**
 * Saves \a inif to the file referenced by \a path.
 *
 * @param inif The inifile handle.
 * @param path The path to save the file as.
 */
void inifile_save_as(inifile *inif, const char *path);

/**
 * Frees the memory associated with the inifile inif. This must be called;
 * failure to do so may result in ones program spontaneously turning to custard
 * and other bad things.
 *
 * Calling this method does not affect the on-disk representation of the
 * inifile. Therefore the file must be saved, using the inifile_save* methods
 * first.
 *
 * @param inif The inifile handle.
 */
void inifile_delete(inifile *inif);

/**
 * Unit tests for the inifile module. Asserts should any test fail. The
 * implementation can also serve as basic API documentation.
 */
void inifile_test(void);

#endif
