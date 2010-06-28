/*-------------------------------------------------------------------------*/
/**
   @file	dictionary.c
   @author	N. Devillard
   @date	Sep 2007
   @version	$Revision: 1.27 $
   @brief	Implements a dictionary for string variables.

   This module implements a simple dictionary object, i.e. a list
   of string/string associations. This object is useful to store e.g.
   informations retrieved from a configuration file (ini files).
*/
/*--------------------------------------------------------------------------*/

/*
	$Id: dictionary.c,v 1.27 2007-11-23 21:39:18 ndevilla Exp $
	$Revision: 1.27 $
*/
/*---------------------------------------------------------------------------
   								Includes
 ---------------------------------------------------------------------------*/
#include "lib/framework/wzglobal.h"
#include "lib/framework/frame.h"
#include "dictionary.h"

#include <stdlib.h>
#include <string.h>
#if defined(WZ_OS_UNIX)
#include <unistd.h>
#endif

/** Maximum value size for integers and doubles. */
#define MAXVALSZ	1024

/** Minimal allocated number of entries in a dictionary */
#define DICTMINSZ	128

/** Invalid key token */
#define DICT_INVALID_KEY    ((char*)-1)

/*---------------------------------------------------------------------------
  							Private functions
 ---------------------------------------------------------------------------*/

/**
 * Doubles the allocated size associated to a pointer.
 * @param size The current allocated size. 
 */
static void * mem_double(void * ptr, int size)
{
	void * newptr ;

	newptr = calloc(2 * size, 1);
	if (newptr == NULL)
	{
		return NULL;
	}
	memcpy(newptr, ptr, size);
	free(ptr);
	return newptr;
}

/*---------------------------------------------------------------------------
  							Function codes
 ---------------------------------------------------------------------------*/
unsigned dictionary_hash(const char *key)
{
	int		len;
	unsigned	hash;
	int		i;

	len = strlen(key);
	for (hash = 0, i = 0 ; i < len ; i++)
	{
		hash += (unsigned)key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

dictionary *dictionary_new(int size)
{
	dictionary	*d;

	/* If no size was specified, allocate space for DICTMINSZ */
	if (size < DICTMINSZ) size = DICTMINSZ ;

	if (!(d = (dictionary *)calloc(1, sizeof(dictionary))))
	{
		return NULL;
	}
	d->size = size ;
	d->val  = (char **)calloc(size, sizeof(char*));
	d->key  = (char **)calloc(size, sizeof(char*));
	d->hash = (unsigned int *)calloc(size, sizeof(unsigned));
	return d;
}

void dictionary_del(dictionary * d)
{
	int	i;

	if (d == NULL) return;
	for (i = 0 ; i < d->size ; i++)
	{
		if (d->key[i] != NULL)
			free(d->key[i]);
		if (d->val[i] != NULL)
			free(d->val[i]);
	}
	free(d->val);
	free(d->key);
	free(d->hash);
	free(d);
}

const char *dictionary_get(dictionary *d, const char *key, const char *def)
{
	unsigned	hash;
	int		i;

	hash = dictionary_hash(key);
	for (i = 0 ; i < d->size ; i++)
	{
		if (d->key[i] == NULL)
			continue;
		/* Compare hash */
		if (hash == d->hash[i])
		{
			/* Compare string, to avoid hash collisions */
			if (!strcmp(key, d->key[i]))
			{
				return d->val[i];
			}
		}
	}
	return def;
}

int dictionary_set(dictionary *d, const char *key, const char *val)
{
	int		i;
	unsigned	hash;

	if (d == NULL || key == NULL) return -1;

	/* Compute hash for this key */
	hash = dictionary_hash(key) ;
	/* Find if value is already in dictionary */
	if (d->n > 0)
	{
		for (i = 0 ; i < d->size ; i++)
		{
			if (d->key[i] == NULL)
				continue;
			if (hash == d->hash[i])		/* Same hash value */
			{
				if (!strcmp(key, d->key[i]))	 /* Same key */
				{
					/* Found a value: modify and return */
					if (d->val[i] != NULL)
						free(d->val[i]);
					d->val[i] = val ? strdup(val) : NULL ;
					/* Value has been modified: return */
					return 0;
				}
			}
		}
	}
	/* Add a new value */
	/* See if dictionary needs to grow */
	if (d->n == d->size)
	{

		/* Reached maximum size: reallocate dictionary */
		d->val  = (char **)mem_double(d->val,  d->size * sizeof(char*));
		d->key  = (char **)mem_double(d->key,  d->size * sizeof(char*));
		d->hash = (unsigned int *)mem_double(d->hash, d->size * sizeof(unsigned));
		if ((d->val == NULL) || (d->key == NULL) || (d->hash == NULL))
		{
			/* Cannot grow dictionary */
			return -1;
		}
		/* Double size */
		d->size *= 2;
	}

	/* Insert key in the first empty slot */
	for (i = 0 ; i < d->size ; i++)
	{
		if (d->key[i] == NULL)
		{
			/* Add key here */
			break;
		}
	}
	/* Copy key */
	d->key[i]  = strdup(key);
	d->val[i]  = val ? strdup(val) : NULL;
	d->hash[i] = hash;
	d->n++;
	return 0;
}

int dictionary_set_int(dictionary *d, const char *key, int val)
{
	char *strval;
	sasprintf(&strval, "%d", val);

	return dictionary_set(d, key, strval);
}

void dictionary_unset(dictionary *d, char *key)
{
	unsigned	hash;
	int		i;

	if (key == NULL)
	{
		return;
	}

	hash = dictionary_hash(key);
	for (i = 0 ; i < d->size ; i++)
	{
		if (d->key[i] == NULL)
			continue ;
		/* Compare hash */
		if (hash == d->hash[i])
		{
			/* Compare string, to avoid hash collisions */
			if (!strcmp(key, d->key[i]))
			{
				/* Found key */
				break;
			}
		}
	}
	if (i >= d->size)
		/* Key not found */
		return ;

	free(d->key[i]);
	d->key[i] = NULL;
	if (d->val[i] != NULL)
	{
		free(d->val[i]);
		d->val[i] = NULL;
	}
	d->hash[i] = 0;
	d->n--;
}

void dictionary_dump(dictionary * d, FILE * out)
{
	int		i;

	if (d == NULL || out == NULL) return;
	if (d->n < 1)
	{
		fprintf(out, "empty dictionary\n");
		return;
	}
	for (i = 0 ; i < d->size ; i++)
	{
		if (d->key[i])
		{
			fprintf(out, "%20s\t[%s]\n",
			        d->key[i],
			        d->val[i] ? d->val[i] : "UNDEF");
		}
	}
}
