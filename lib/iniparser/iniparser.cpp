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

#include <ctype.h>
#include <vector>

#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/stdio_ext.h"

#include "iniparser.h"
#include <search.h>

struct InifileEntry
{
	/// Section pointer for entry
	char *sec;
	/// Key of the entry
	char *key;
	/// Value of the entry
	char *value;
};

struct Inifile
{
	/// The names of the sections in the inifile
	std::vector<char *> sec;

	// The current section number
	char *currsec;

	/// Entries
	std::vector<InifileEntry> entry;

	/// Path of the inifile
	char *path;
    int size;
};

/**
 * This enum stores the status for each parsed line (internal use only).
 */
enum LINE_STATUS
{
	LINE_UNPROCESSED,
	LINE_ERROR,
	LINE_EMPTY,
	LINE_COMMENT,
	LINE_SECTION,
	LINE_VALUE
};


#define ASCIILINESZ         (1024)

// Utility function prototypes
static LINE_STATUS parse_line(const char *input_line,
                              char *section,
                              char *key,
                              char *value);

static char *strstrip(const char *s);
static char *strlwc(const char *s);

inifile *inifile_new()
{
	inifile *inif = new inifile;

	inif->currsec = NULL;

	inif->path = NULL;

	return inif;
}

inifile *inifile_load(const char *path)
{
	PHYSFS_File *in;

	char line[ASCIILINESZ + 1];
	char section[ASCIILINESZ + 1];
	char key[ASCIILINESZ + 1];
	char val[ASCIILINESZ + 1];

	int last = 0;
	int len;
	int lineno = 0;
	int errors = 0;

	inifile *inif;

	if ((in = PHYSFS_openRead(path)) == NULL)
	{
		return NULL;
	}

	inif = inifile_new();
	inif->path = strdup(path);

	ASSERT(inif->path, "Failed to allocate inifile memory.");

	memset(line, 0, ASCIILINESZ + 1);
	memset(section, 0, ASCIILINESZ + 1);
	memset(key, 0, ASCIILINESZ + 1);
	memset(val, 0, ASCIILINESZ + 1);
	last = 0 ;

	// Loop over each line in the file
	while (PHYSFS_fgets(line + last, ASCIILINESZ - last, in) != NULL)
	{
		lineno++;
		len = strlen(line) - 1;

		// Ensure that we read the entire line
		if (line[len] != '\n')
		{
			fprintf(stderr,
			        "iniparser: input line too long in %s (%d)\n",
			        path,
			        lineno);

			inifile_delete(inif);
			inif = NULL;

			break;
		}
		// Get rid of \n and spaces at end of line
		while (len >= 0
		    && (line[len] == '\n'
		     || isspace(line[len])))
		{
			line[len] = 0;
			len--;
		}
		// Detect multi-line
		if (line[len] == '\\')
		{
			last = len;
			continue;
		}
		else
		{
			last = 0;
		}

		switch (parse_line(line, section, key, val))
		{
			case LINE_EMPTY:
			case LINE_COMMENT:
				break;
			case LINE_SECTION:
				inifile_set_current_section(inif, section);
				break;
			case LINE_VALUE:
				inifile_set(inif, key, val);
				break;
			case LINE_ERROR:
				fprintf(stderr, "inifile: syntax error in %s (%d):\n",
						path,
						lineno);
				fprintf(stderr, "-> %s\n", line);
				errors++;
				break;
			default:
				break;
		}

		if (errors)
		{
			inifile_delete(inif);
			inif = NULL;
			break;
		}

		memset(line, 0, ASCIILINESZ + 1);
		last = 0;
	}

	PHYSFS_close(in);

	return inif;
}

int inifile_get_section_count(inifile *inif)
{
	return inif->sec.size();
}

const char *inifile_get_section(inifile *inif, int n)
{
	return inif->sec[n];
}

const char *inifile_get_current_section(inifile *inif)
{
	return inif->currsec;
}

void inifile_set_current_section(inifile *inif, const char *sec)
{
	// See if the section exists in the file already
	for (unsigned i = 0; i < inif->sec.size(); i++)
	{
		// Found it
		if (strcmp(strlwc(sec), inif->sec[i]) == 0)
		{
			inif->currsec = inif->sec[i];
			return;
		}
	}

	inif->sec.push_back(strdup(sec));

	// Make this new section entry current
	inif->currsec = inif->sec.back();

	ASSERT(inif->currsec, "Failed  to allocate inifile memory.");
}

BOOL inifile_key_exists(inifile *inif, const char *key)
{
	for (unsigned i = 0; i < inif->entry.size(); ++i)
	{
		if (strcmp(inif->entry[i].key, key) == 0)
		{
			return true;
		}
	}

	// Not found, return false
	return false;
}

const char *inifile_get(inifile *inif, const char *key, const char *dflt)
{
	// Search for the entry
	for (unsigned i = 0; i < inif->entry.size(); ++i)
	{
		if (inif->entry[i].sec == inif->currsec
		 && strcmp(inif->entry[i].key, strlwc(key)) == 0)
		{
			return inif->entry[i].value;
		}
	}

	// Not found
	return dflt;
}

void inifile_set(inifile *inif, const char *key, const char *value)
{
	// Ensure that key is valid
	ASSERT(key, "Attempting to set a NULL key");

	// If value is NULL then replace it with an empty string
	value = (value == NULL) ? "" : value;

	// If there is no active section we need to create one
	if (!inif->currsec)
	{
		inifile_set_current_section(inif, "main");
	}
	// If there is an active section then key may already exist
	else
	{
		for (unsigned i = 0; i < inif->entry.size(); ++i)
		{
			if (inif->entry[i].sec == inif->currsec
			 && strcmp(inif->entry[i].key, key) == 0)
			{
				// Update the current value for the key
				free(inif->entry[i].value);
				inif->entry[i].value = strdup(value);
				return;
			}
		}
	}

	// Insert
	InifileEntry entry;
	entry.sec = inif->currsec;
	entry.key = strdup(key);
	entry.value = strdup(value);
	inif->entry.push_back(entry);

	ASSERT(entry.key && entry.value, "Failed to allocate inifile memory.");
}

int inifile_get_as_int(inifile *inif, const char *key, int dflt)
{
	const char *strint = inifile_get(inif, key, NULL);

	if (strint == NULL)
	{
		return dflt;
	}
	else
	{
		return atoi(strint);
	}
}

void inifile_set_as_int(inifile *inif, const char *key, int value)
{
	char *strval;
	sasprintf(&strval, "%d", value);

	inifile_set(inif, key, strval);
}

int inifile_get_as_bool(inifile *inif, const char *key, int dflt)
{
	const char *strbool = inifile_get(inif, key, NULL);

	if (strbool == NULL)
	{
		return dflt;
	}
	else
	{
		switch (tolower(strbool[0]))
		{
			case 'y':
			case '1':
			case 't':
				return 1;
			case 'n':
			case '0':
			case 'f':
				return 0;
			default:
				return dflt;
		}
	}
}

void inifile_unset(inifile *inif, const char *sec, const char *key)
{
	int secoffset = -1;

	// First get the section offset
	for (unsigned i = 0; i < inif->sec.size(); ++i)
	{
		if (strcmp(inif->sec[i], sec) == 0)
		{
			secoffset = i;
		}
	}

	// If we didn't find the section then give up
	if (secoffset == -1)
	{
		return;
	}

	// Look for the key in that section
	for (unsigned i = 0; i < inif->entry.size(); ++i)
	{
		if (inif->entry[i].sec == inif->sec[secoffset]
		 && strcmp(inif->entry[i].key, key) == 0)
		{
			// Free the memory allocated by the entry
			free(inif->entry[i].key);
			free(inif->entry[i].value);

			// Shift all subsequent entries back a slot
			inif->entry.erase(inif->entry.begin() + i);

			break;
		}
	}

	/*
	 * If the section is not currently active then we need to see if any other
	 * entries are in the section and, if not, to delete the section.
	 */
	if (inif->currsec != inif->sec[secoffset])
	{
		// Look for other entries in the section
		for (unsigned i = 0; i < inif->entry.size(); ++i)
		{
			if (inif->entry[i].sec == inif->sec[secoffset])
			{
				return;
			}
		}

		// Otherwise, if there are none, get rid of the section
		free(inif->sec[secoffset]);
		inif->sec.erase(inif->sec.begin() + secoffset);
	}
}

void inifile_save(inifile *inif)
{
	ASSERT(inif->path, "Attempt to save an inifile with no valid (internal) path");

	inifile_save_as(inif, inif->path);
}

void inifile_save_as(inifile *inif, const char *path)
{
	PHYSFS_file *f = PHYSFS_openWrite(path);

	// Loop over each section of the inifile
	for (unsigned i = 0; i < inif->sec.size(); ++i)
	{
		const char *currsec = inifile_get_section(inif, i);

		/*
		 * Print the section title if there are either multiple sections in the
		 * file or if the singular section has been explicitly named.
		 */
		if (inif->sec.size() > 1 || strcmp("main", currsec) != 0)
		{
			// Print out the section title
			PHYSFS_printf(f, "[%s]\n", currsec);
		}

		// Then the key => value pairs
		for (unsigned j = 0; j < inif->entry.size(); ++j)
		{
			if (inif->entry[j].sec == currsec)
			{
				PHYSFS_printf(f, "%-30s = %s\n", inif->entry[j].key,
				                                 inif->entry[j].value);
			}
		}

		PHYSFS_printf(f, "\n");
	}

	PHYSFS_close(f);
}

void inifile_delete(inifile *inif)
{
	// Free the sections
	for (unsigned i = 0; i < inif->sec.size(); ++i)
	{
		free(inif->sec[i]);
	}

	// Then the list of sections

	// Free the entries
	for (unsigned i = 0; i < inif->entry.size(); ++i)
	{
		free(inif->entry[i].key);
		free(inif->entry[i].value);
	}

	// Then the list of entries

	// Finally the original file path (if given)
	free(inif->path);

	delete inif;
}

void inifile_test()
{
	// Internal consistency check
	inifile *inif = inifile_new();

	inifile_set(inif, "string", "a string");
	inifile_set_as_int(inif, "int", 31415);

	ASSERT(inifile_get_section_count(inif) == 1,
	       "Section count unit test failed");
	ASSERT(strcmp(inifile_get_current_section(inif), "main") == 0,
	       "Default section unit test failed");

	ASSERT(strcmp(inifile_get(inif, "string", ""), "a string") == 0,
	       "String retrieval failed");
	ASSERT(inifile_get_as_int(inif, "int", -1) == 31415,
	       "Int retrieval failed");

	ASSERT(strcmp(inifile_get(inif, "invalid key", "dflt"), "dflt") == 0,
	       "Default value retrieval failed");

	inifile_set_current_section(inif, "Casetest");
	inifile_set(inif, "Upperkey", "Any String");

	inifile_set_current_section(inif, "newsec");
	inifile_set(inif, "third", "a third entry");

	// Save the current inifile; we'll load it up again later
	inifile_save_as(inif, "selftest.ini");

	ASSERT(inifile_get_section_count(inif) == 3,
	       "Section addition test failed");

	inifile_unset(inif, "main", "string");
	inifile_unset(inif, "main", "int");

	ASSERT(inifile_get_section_count(inif) == 2,
	       "Automatic section removal test failed");
	ASSERT(inif->entry.size() == 2,
	       "Key deletion test failed");
	ASSERT(strcmp(inifile_get(inif, "third", ""), "a third entry") == 0,
	       "Key deletion test failed");

	inifile_delete(inif);

	// Load up the file we saved earlier
	inif = inifile_load("selftest.ini");

	ASSERT(inifile_get_section_count(inif) == 3,
	       "Save/Load test failed");

	inifile_set_current_section(inif, "newsec");

	ASSERT(strcmp(inifile_get(inif, "third", ""), "a third entry") == 0,
	       "Save/Load test failed");

	inifile_set_current_section(inif, "main");
	inifile_unset(inif, "newsec", "third");

	inifile_set_current_section(inif, "casetest");
	ASSERT(strcmp(inifile_get(inif, "Upperkey", "1"), inifile_get(inif, "upperkey", "2")) == 0,
	       "Case insensitivity test failed.");
	inifile_unset(inif, "Casetest", "Upperkey");

	// Save again, this time there should be no section titles
	inifile_save(inif);
	inifile_delete(inif);

	// Delete the saved file
	PHYSFS_delete("selftest.ini");

	fprintf(stdout, "\tinifile self-test: PASSED\n");
}

/**
  @brief	Load a single line from an INI file
  @param    input_line  Input line, may be concatenated multi-line input
  @param    section     Output space to store section
  @param    key         Output space to store key
  @param    value       Output space to store value
  @return   LINE_STATUS value
 */
LINE_STATUS parse_line(const char *input_line,
                       char *section, char *key, char *value)
{
	LINE_STATUS status = LINE_UNPROCESSED;
	char        line[ASCIILINESZ+1];
	int         len;

	// Strip the line and copy it into line
	strcpy(line, strstrip(input_line));
	len = strlen(line);

	// Empty line
	if (len < 1)
	{
		status = LINE_EMPTY;
	}
	// Starts with a # and therefore a comment
	else if (line[0] == '#')
	{
		status = LINE_COMMENT;
	}
	// Section identifier
	else if (line[0] == '['
	      && line[len-1] == ']')
	{
		sscanf(line, "[%[^]]", section);
		strcpy(section, strstrip(section));
		strcpy(section, strlwc(section));
		status = LINE_SECTION;
	}
	// Of the form key=value # optional comment
	else if (sscanf(line, "%[^=] = \"%[^\"]\"", key, value) == 2
	      || sscanf(line, "%[^=] = '%[^\']'",   key, value) == 2
	      || sscanf(line, "%[^=] = %[^;#]",     key, value) == 2)
	{
		strcpy(key, strstrip(key));
		strcpy(key, strlwc(key));
		strcpy(value, strstrip(value));
		/*
		 * sscanf cannot handle '' or "" as empty values
		 * this is done here
		 */
		if (!strcmp(value, "\"\"") || (!strcmp(value, "''")))
		{
			value[0] = '\0';
		}

		status = LINE_VALUE;
	}
	// Annoying special cases
	else if (sscanf(line, "%[^=] = %[;#]", key, value) == 2
	     ||  sscanf(line, "%[^=] %[=]", key, value) == 2)
	{
		/*
		 * Special cases:
		 * key=
		 * key=;
		 * key=#
		 */
		strcpy(key, strstrip(key));
		strcpy(key, strlwc(key));
		value[0] = 0;
		status = LINE_VALUE;
	}
	// Otherwise it is invalid
	else
	{
		status = LINE_ERROR;
	}

	return status;
}

/**
  @brief	Convert a string to lowercase.
  @param	s	String to convert.
  @return	ptr to statically allocated string.

  This function returns a pointer to a statically allocated string
  containing a lowercased version of the input string. Do not free
  or modify the returned string! Since the returned string is statically
  allocated, it will be modified at each function call (not re-entrant).
 */
char *strlwc(const char *s)
{
	static char l[ASCIILINESZ+1];
	int i;

	if (s == NULL) return NULL;
	memset(l, 0, ASCIILINESZ + 1);
	i = 0;
	while (s[i] && i < ASCIILINESZ)
	{
		l[i] = (char)tolower((int)s[i]);
		i++;
	}
	l[ASCIILINESZ] = (char)0;
	return l;
}

/**
  @brief	Remove blanks at the beginning and the end of a string.
  @param	s	String to parse.
  @return	ptr to statically allocated string.

  This function returns a pointer to a statically allocated string,
  which is identical to the input string, except that all blank
  characters at the end and the beg. of the string have been removed.
  Do not free or modify the returned string! Since the returned string
  is statically allocated, it will be modified at each function call
  (not re-entrant).
 */
char *strstrip(const char *s)
{
	static char l[ASCIILINESZ+1];
	char *last;

	if (s == NULL)
	{
		return NULL;
	}

	while (isspace(*s) && *s) s++;

	memset(l, 0, ASCIILINESZ + 1);
	strncpy(l, s, sizeof(l));
	last = l + strlen(l);
	while (last > l)
	{
		if (!isspace(*(last - 1)))
		{
			break;
		}

		last--;
	}

	*last = '\0';

	return l;
}
