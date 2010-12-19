/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
// Config.c saves your favourite options to the Registry.
#include "frame.h"
#include "configfile.h"

#include <string.h>
#include "string_ext.h"

#include "file.h"

#define REGISTRY_HASH_SIZE	32
#define MAXLINESIZE 255

typedef struct	regkey_t
{
	char			*key;
	char			*value;
	struct regkey_t *next;
} regkey_t;
static regkey_t* registry[REGISTRY_HASH_SIZE] = { NULL };
static char      RegFilePath[PATH_MAX];

void registry_clear()
{
	unsigned int	i;

	for (i = 0; i < ARRAY_SIZE(registry); ++i)
	{
		regkey_t* j = registry[i];

		while (j)
		{
			regkey_t* const toDelete = j;
			j = j->next;

			free(toDelete->key);
			free(toDelete->value);
			free(toDelete);
		}

		registry[i] = NULL;
	}
}

static unsigned int registry_hash(const char* s)
{
	unsigned int h = 0;

	while (s && *s)
		h += *s++;

	return h % ARRAY_SIZE(registry);
}

static regkey_t* registry_find_key(const char* key)
{
	regkey_t* i;

	for (i = registry[registry_hash(key)]; i != NULL; i = i->next)
	{
		if (!strcmp(key, i->key))
		{
			return i;
		}
	}

	return NULL;
}

static char* registry_get_by_key(const char* key)
{
	regkey_t* regkey = registry_find_key(key);

	if (regkey == NULL)
	{
#if 0
		debug(LOG_NEVER, "Key \"%s\" not found", key);
#endif
		return NULL;
	}
	else
	{
#if 0
		debug(LOG_NEVER, "Key \"%s\" has value \"%s\"", key, regkey->value);
#endif
		return regkey->value;
	}
}

static void registry_set_key(const char* key, const char* value)
{
	regkey_t* regkey = registry_find_key(key);

#if 0
	debug(LOG_NEVER, "Set key \"%s\" to value \"%s\"", key, value);
#endif
	if (regkey == NULL)
	{
		// Add a new bucked to the hashtable for the given key
		const unsigned int h = registry_hash(key);

		regkey = (regkey_t *)malloc(sizeof(*regkey));
		regkey->key = strdup(key);
		regkey->next = registry[h];
		registry[h] = regkey;
	}
	else
	{
		free(regkey->value);
	}

	regkey->value = strdup(value);
}

static bool registry_load(const char* filename)
{
	char buffer[MAXLINESIZE + 1];
	char *bptr = NULL, *bufstart = NULL;
	char key[MAXLINESIZE];
	int l; // sscanf expects an int to receive %n, not an unsigned int
	UDWORD filesize;

	debug(LOG_WZ, "Loading the registry from [directory: %s] %s", PHYSFS_getRealDir(filename), filename);
	if (PHYSFS_exists(filename)) {
		if (!loadFile(filename, &bptr, &filesize)) {
			return false;           // happens only in NDEBUG case
		}
	} else {
		// Registry does not exist. Caller will write a new one.
		return false;
	}

	debug(LOG_WZ, "Parsing the registry from [directory: %s] %s", PHYSFS_getRealDir(filename) , filename);
	if (filesize == 0 || strlen(bptr) == 0) {
		debug(LOG_WARNING, "Registry file %s is empty!", filename);
		return false;
	}
	bufstart = bptr;
	bptr[filesize - 1] = '\0'; // make sure it is terminated
	while (*bptr != '\0') {
		int count = 0;

		/* Put a line into buffer */
		while (*bptr != '\0' && *bptr != '\n' && count < MAXLINESIZE)
		{
			buffer[count] = *bptr;
			bptr++;
			count++;
		}
		if (*bptr != '\0') {
			bptr++;	// skip EOL
		}
		buffer[count] = '\0';
		if (sscanf(buffer, " %[^=] = %n", key, &l) == 1) {
			unsigned int i;

			for (i = l;; ++i) {
				if (buffer[i] == '\0') {
					break;
				} else if (buffer[i] < ' ') {
					buffer[i] = '\0';
					break;
				}
			}
			registry_set_key(key, buffer + l);
		}
	}
	free(bufstart);
	return true;
}

static bool registry_save(const char* filename)
{
	char buffer[MAXLINESIZE * ARRAY_SIZE(registry)];
	unsigned int i;
	int count = 0;

	debug(LOG_WZ, "Saving the registry to [directory: %s] %s", PHYSFS_getRealDir(filename), filename);
	for (i = 0; i < ARRAY_SIZE(registry); ++i)
	{
		regkey_t *j;

		for (j = registry[i]; j != NULL; j = j->next) {
			char linebuf[MAXLINESIZE];

			snprintf(linebuf, sizeof(linebuf), "%s=%s\n", j->key, j->value);
			assert(strlen(linebuf) > 0 && strlen(linebuf) < MAXLINESIZE);
			assert(count + strlen(linebuf) < MAXLINESIZE * ARRAY_SIZE(registry));
			memcpy(buffer + count, linebuf, strlen(linebuf));
			count += strlen(linebuf);
		}
	}

	return saveFile(filename, buffer, count);
}

void setRegistryFilePath(const char* fileName)
{
	sstrcpy(RegFilePath, fileName);
}

bool openWarzoneKey()
{
	static bool done = false;

	if (done == false)
	{
		done = registry_load(RegFilePath);
	}

	return true;
}

bool closeWarzoneKey()
{
	return registry_save(RegFilePath);
}

/**
 * Read config setting \c key into val
 * \param	*key	Config setting
 * \param	*val	Place where to store the setting
 * \return	Whether we succeed to find the setting
 */
bool getWarzoneKeyNumeric(const char* key, int* val)
{
	const char* const value = registry_get_by_key(key);

	return (value != NULL
	     && sscanf(value, "%i", val) == 1);
}

bool getWarzoneKeyString(const char* key, char* val)
{
	const char* const value = registry_get_by_key(key);

	if (value == NULL)
	{
		return false;
	}

	strcpy(val, value);
	return true;
}

bool setWarzoneKeyNumeric(const char* key, int val)
{
	char* numBuf;
	sasprintf(&numBuf, "%i", val);

	registry_set_key(key, numBuf);
	return true;
}

bool setWarzoneKeyString(const char* key, const char* val)
{
	registry_set_key(key, val);
	return true;
}
