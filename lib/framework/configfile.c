/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

//
// =======================================================================================================================
// =======================================================================================================================
//
void registry_clear( void )
{
	//~~~~~~~~~~~~~~
	unsigned int	i;
	//~~~~~~~~~~~~~~

	for ( i = 0; i < REGISTRY_HASH_SIZE; ++i )
	{
		//~~~~~~~~~~~~~
		regkey_t	*j;
		regkey_t	*tmp;
		//~~~~~~~~~~~~~

		for (j = registry[i]; j != NULL; j = tmp)
		{
			tmp = j->next;
			free(j->key);
			free(j->value);
			free(j);
		}

		registry[i] = NULL;
	}
}

//
// =======================================================================================================================
// =======================================================================================================================
//
static unsigned int registry_hash( const char *s )
{
	//~~~~~~~~~~~~~~~~~~
	unsigned int	i;
	unsigned int	h = 0;
	//~~~~~~~~~~~~~~~~~~

	if ( s != NULL )
	{
		for ( i = 0; s[i] != '\0'; ++i )
		{
			h += s[i];
		}
	}

	return h % REGISTRY_HASH_SIZE;
}

//
// =======================================================================================================================
// =======================================================================================================================
//
static regkey_t *registry_find_key( const char *k )
{
	//~~~~~~~~~~~
	regkey_t	*i;
	//~~~~~~~~~~~

	for ( i = registry[registry_hash(k)]; i != NULL; i = i->next )
	{
		if ( !strcmp(k, i->key) )
		{
			return i;
		}
	}

	return NULL;
}

//
// =======================================================================================================================
// =======================================================================================================================
//
static char *registry_get_key( const char *k )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	regkey_t	*key = registry_find_key( k );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ( key == NULL )
	{
		//
		// printf("registry_get_key(%s) -> key not found\n", k);
		//
		return NULL;
	}
	else
	{
		//
		// printf("registry_get_key(%s) -> %s\n", k, key->value);
		//
		return key->value;
	}
}

//
// =======================================================================================================================
// =======================================================================================================================
//
static void registry_set_key( const char *k, const char *v )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	regkey_t	*key = registry_find_key( k );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	//
	// printf("registry_set_key(%s, %s)\n", k, v);
	//
	if ( key == NULL )
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		unsigned int	h = registry_hash( k );
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		key = (regkey_t *) malloc( sizeof(regkey_t) );
		key->key = strdup( k );
		key->next = registry[h];
		registry[h] = key;
	}
	else
	{
		free( key->value );
	}

	key->value = strdup( v );
}

//
// =======================================================================================================================
// =======================================================================================================================
//
static BOOL registry_load( const char *filename )
{
	char buffer[MAXLINESIZE];
	char *bptr = NULL, *bufstart = NULL;
	char key[32];
	int l; // sscanf expects an int to receive %n, not an unsigned int
	UDWORD filesize;

	debug(LOG_WZ, "Loading the registry from %s", filename);
	if (PHYSFS_exists(filename)) {
		if (!loadFile(filename, &bptr, &filesize)) {
			return false;           // happens only in NDEBUG case
		}
	} else {
		// Registry does not exist. Caller will write a new one.
		return false;
	}

	debug(LOG_WZ, "Parsing the registry from %s", filename);
	if (filesize == 0 || strlen(bptr) == 0) {
		debug(LOG_WARNING, "Registry file %s is empty!", filename);
		return false;
	}
	bufstart = bptr;
	bptr[filesize - 1] = '\0'; // make sure it is terminated
	while (*bptr != '\0') {
		int count = 0;

		/* Put a line into buffer */
		while (*bptr != '\0' && *bptr != '\n' && count < MAXLINESIZE) {
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

//
// =======================================================================================================================
// =======================================================================================================================
//
static BOOL registry_save( const char *filename )
{
	char buffer[MAXLINESIZE * REGISTRY_HASH_SIZE];
	unsigned int i;
	int count = 0;

	debug(LOG_WZ, "Saving the registry to %s", filename);
	for (i = 0; i < REGISTRY_HASH_SIZE; ++i) {
		regkey_t *j;

		for (j = registry[i]; j != NULL; j = j->next) {
			char linebuf[MAXLINESIZE];

			snprintf(linebuf, sizeof(linebuf), "%s=%s\n", j->key, j->value);
			assert(strlen(linebuf) > 0 && strlen(linebuf) < MAXLINESIZE);
			memcpy(buffer + count, linebuf, strlen(linebuf));
			count += strlen(linebuf);
			assert(count < MAXLINESIZE * REGISTRY_HASH_SIZE);
		}
	}

	if (!saveFile(filename, buffer, count)) {
		return false; // only in NDEBUG case
	}
	return true;
}

void setRegistryFilePath(const char* fileName)
{
	sstrcpy(RegFilePath, fileName);
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL openWarzoneKey( void )
{
	//~~~~~~~~~~~~~~~~~~~~~
	static BOOL done = false;
	//~~~~~~~~~~~~~~~~~~~~~

	if ( done == false )
	{
		registry_load( RegFilePath );
		done = true;
	}

	return true;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL closeWarzoneKey( void )
{
	registry_save( RegFilePath );
	return true;
}

/**
 * Read config setting pName into val
 * \param	*pName	Config setting
 * \param	*val	Place where to store the setting
 * \return	Whether we succeed to find the setting
 */
BOOL getWarzoneKeyNumeric( const char *pName, SDWORD *val )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char	*value = registry_get_key( pName );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ( value == NULL || sscanf(value, "%i", val) != 1 )
	{
		return false;
	}
	else
	{
		return true;
	}
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL getWarzoneKeyString( const char *pName, char *pString )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char	*value = registry_get_key( pName );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ( value == NULL )
	{
		return false;
	}
	else
	{
		strcpy( pString, value );
	}

	return true;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyNumeric( const char *pName, SDWORD val )
{
	//~~~~~~~~~~~~
	char	buf[32];
	//~~~~~~~~~~~~

	snprintf(buf, sizeof(buf), "%i", val);

	registry_set_key( pName, buf );
	return true;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyString( const char *pName, const char *pString )
{
	registry_set_key( pName, pString );
	return true;
}
