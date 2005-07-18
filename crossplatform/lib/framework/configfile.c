// Config.c saves your favourite options to the Registry.
#include "frame.h"
#include "configfile.h"

///
//
// * Windows version
#ifdef WIN32	// registry stuff IS used but it shouldn't be, I see no need, we can use same code linux does. --Qamly
static HKEY ghWarzoneKey = NULL;

//
// =======================================================================================================================
// =======================================================================================================================
//
BOOL openWarzoneKey( void )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	DWORD	result, ghGameDisp;			// key created or opened
	char	keyname[256] = "SOFTWARE\\Pumpkin Studios\\Warzone2100\\";
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	result = RegCreateKeyEx
		(								// create/open key.
			HKEY_LOCAL_MACHINE,			// handle of an open key
			keyname,					// address of subkey name
			0,							// reserved
			NULL,						// address of class string
			REG_OPTION_NON_VOLATILE,	// special options flag
			KEY_ALL_ACCESS,				// desired security access
			NULL,						// address of key security structure
			&ghWarzoneKey,				// address of buffer for opened handle
			&ghGameDisp					// address of disposition value buffer
		);
	if ( result == ERROR_SUCCESS )
	{
		return TRUE;
	}

	return FALSE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL closeWarzoneKey( void )
{
	if ( RegCloseKey(ghWarzoneKey) == ERROR_SUCCESS )
	{
		return TRUE;
	}

	return FALSE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL getWarzoneKeyNumeric( STRING *pName, DWORD *val )
{
	//~~~~~~~~~~~~~~~~~~~~~~
	UCHAR	result[16];			// buffer
	DWORD	resultsize = 16;	// buffersize
	DWORD	type;				// type of reg entry.
	//~~~~~~~~~~~~~~~~~~~~~~

	// check guid exists
	if ( RegQueryValueEx(ghWarzoneKey, pName, NULL, &type, (UCHAR *) &result, &resultsize) != ERROR_SUCCESS )
	{
		//
		// RegCloseKey(ghWarzoneKey);
		//
		return FALSE;
	}

	if ( type == REG_DWORD )
	{
		memcpy( val, &result[0], sizeof(DWORD) );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL getWarzoneKeyString( STRING *pName, STRING *pString )
{
	//~~~~~~~~~~~~~~~~~~~~~~~
	UCHAR	result[255];		// buffer
	DWORD	resultsize = 255;	// buffersize
	DWORD	type;				// type of reg entry.
	//~~~~~~~~~~~~~~~~~~~~~~~

	// check guid exists
	if ( RegQueryValueEx(ghWarzoneKey, pName, NULL, &type, (UCHAR *) &result, &resultsize) != ERROR_SUCCESS )
	{
		//
		// RegCloseKey(ghWarzoneKey);
		//
		return FALSE;
	}

	if ( type == REG_SZ )
	{
		strcpy( pString, (CHAR *) result );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL getWarzoneKeyBinary( STRING *pName, UCHAR *pData, UDWORD *pSize )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~
	UCHAR	result[2048];		// buffer
	DWORD	resultsize = 2048;	// buffersize
	DWORD	type;				// type of reg entry.
	//~~~~~~~~~~~~~~~~~~~~~~~~

	// check guid exists
	if ( RegQueryValueEx(ghWarzoneKey, pName, NULL, &type, (UCHAR *) &result, &resultsize) != ERROR_SUCCESS )
	{
		//
		// RegCloseKey(ghWarzoneKey);
		//
		*pSize = 0;
		return FALSE;
	}

	if ( type == REG_BINARY )
	{
		memcpy( pData, &result[0], resultsize );
		*pSize = resultsize;
		return TRUE;
	}
	else
	{
		pSize = 0;
		return FALSE;
	}
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyNumeric( STRING *pName, DWORD val )
{
	RegSetValueEx( ghWarzoneKey, pName, 0, REG_DWORD, (UBYTE *) &val, sizeof(val) );
	return TRUE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyString( STRING *pName, STRING *pString )
{
	RegSetValueEx( ghWarzoneKey, pName, 0, REG_SZ, (UBYTE *) pString, strlen(pString) );
	return TRUE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyBinary( STRING *pName, VOID *pData, UDWORD size )
{
	RegSetValueEx( ghWarzoneKey, pName, 0, REG_BINARY, pData, size );
	return TRUE;
}

#else // Linux version
	#define REGISTRY_HASH_SIZE	32
typedef struct	regkey_t
{
	char			*key;
	char			*value;
	struct regkey_t *next;
} regkey_t;
regkey_t	*registry[REGISTRY_HASH_SIZE] = { NULL };
char		UnixRegFilePath[255];

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

		for ( j = registry[i]; j != NULL; j = tmp )
		{
			tmp = j->next;
			free( j->key );
			free( j->value );
			free( j );
		}

		registry[i] = 0;
	}
}

//
// =======================================================================================================================
// =======================================================================================================================
//
unsigned int registry_hash( const char *s )
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
regkey_t *registry_find_key( const char *k )
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
char *registry_get_key( const char *k )
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
void registry_set_key( const char *k, const char *v )
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
BOOL registry_load( char *filename )
{
	//~~~~~~~
	FILE	*f;
	//~~~~~~~

	f = fopen( filename, "r" );
	if ( f != NULL )
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~
		unsigned int	l;
		char			buffer[256];
		char			key[32];
		//~~~~~~~~~~~~~~~~~~~~~~~~

		//
		// printf("Loading the registry from %s\n", filename);
		//
		while ( !feof(f) )
		{
			fgets( buffer, 255, f );
			if ( sscanf(buffer, " %[^=] = %n", key, &l) == 1 )
			{
				//~~~~~~~~~~~~~~
				unsigned int	i;
				//~~~~~~~~~~~~~~

				for ( i = l;; ++i )
				{
					if ( buffer[i] == '\0' )
					{
						break;
					}
					else if ( buffer[i] < ' ' )
					{
						buffer[i] = '\0';
						break;
					}
				}

				registry_set_key( key, buffer + l );
			}
		}

		fclose( f );
		return TRUE;
	}

	return FALSE;
}

//
// =======================================================================================================================
// =======================================================================================================================
//
BOOL registry_save( char *filename )
{
	//~~~~~~~
	FILE	*f;
	//~~~~~~~

	f = fopen( filename, "w" );
	if ( f != NULL )
	{
		//~~~~~~~~~~~~~~
		unsigned int	i;
		//~~~~~~~~~~~~~~

		//
		// printf("Saving the registry to %s\n", filename);
		//
		for ( i = 0; i < REGISTRY_HASH_SIZE; ++i )
		{
			//~~~~~~~~~~~
			regkey_t	*j;
			//~~~~~~~~~~~

			for ( j = registry[i]; j != NULL; j = j->next )
			{
				fprintf( f, "%s=%s\n", j->key, j->value );
			}
		}

		fclose( f );
		return TRUE;
	}

	return FALSE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL openWarzoneKey( void )
{
	//~~~~~~~~~~~~~~~~~~~~~
	static BOOL done = FALSE;
	//~~~~~~~~~~~~~~~~~~~~~

	if ( done == FALSE )
	{
		registry_load( UnixRegFilePath );
		done = TRUE;
	}

	return TRUE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL closeWarzoneKey( void )
{
	registry_save( UnixRegFilePath );
	return TRUE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL getWarzoneKeyNumeric( STRING *pName, DWORD *val )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char	*value = registry_get_key( pName );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ( value == NULL || sscanf(value, "%i", val) != 1 )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL getWarzoneKeyString( STRING *pName, STRING *pString )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char	*value = registry_get_key( pName );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ( value == NULL )
	{
		return FALSE;
	}
	else
	{
		strcpy( pString, value );
	}

	return TRUE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL getWarzoneKeyBinary( STRING *pName, UCHAR *pData, UDWORD *pSize )
{
	return FALSE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyNumeric( STRING *pName, DWORD val )
{
	//~~~~~~~~~~~~
	char	buf[32];
	//~~~~~~~~~~~~

	sprintf( buf, "%i", val );
	registry_set_key( pName, buf );
	return TRUE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyString( STRING *pName, STRING *pString )
{
	registry_set_key( pName, pString );
	return TRUE;
}

///
// =======================================================================================================================
// =======================================================================================================================
//
BOOL setWarzoneKeyBinary( STRING *pName, VOID *pData, UDWORD size )
{
	return FALSE;
}
#endif // linux
