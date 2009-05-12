/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2009  Giel van Schijndel
	Copyright (C) 2009       Warzone Resurrection Project

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

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "json_value.h"
#include "debug.h"
#include "stdio_ext.h"
#include "string_ext.h"

void freeJsonValuePair(json_value_pair* p)
{
	// See the definition of createJsonValuePair to understand why we don't
	// remove p->name here.
	freeJsonValue(p->value);
	free(p);
}

void freeJsonValue(json_value* v)
{
	switch (v->type)
	{
		case JSON_NUMBER_INT:
		case JSON_NUMBER_FLOAT:
		case JSON_BOOL:
		case JSON_NULL:
			break;

		// See the definition of createJsonString below to understand
		// why we don't remove v->value.string here.
		case JSON_STRING:
			break;

		case JSON_OBJECT:
		{
			while (v->value.members)
			{
				// Get a temporary pointer to the object we're deleting
				json_value_pair * const toDelete = v->value.members;

				// Increment our iteration pointer
				v->value.members = v->value.members->next;

				// Delete the object
				freeJsonValuePair(toDelete);
			}

			break;
		}
		case JSON_ARRAY:
		{
			size_t i;
			for (i = 0; i < v->value.array.size; ++i)
			{
				freeJsonValue(v->value.array.a[i]);
			}

			if (v->value.array.size)
				free(v->value.array.a);

			break;
		}
	}

	free(v);
}

json_value* createJsonString(const char* s)
{
	json_value* v = malloc(sizeof(*v) + strlen(s) + 1);
	if (v == NULL)
	{
		debug(LOG_ERROR, "Out of memory!");
		abort();
		return NULL;
	}

	v->type = JSON_STRING;
	v->value.string = (char*)(v + 1);
	strcpy(v->value.string, s);

	return v;
}

json_value_pair* createJsonValuePair(const char* name, json_value* value)
{
	json_value_pair* p = malloc(sizeof(*p) + strlen(name) + 1);
	if (p == NULL)
	{
		debug(LOG_ERROR, "Out of memory!");
		abort();
		return NULL;
	}

	p->name = (char*)(p + 1);
	strcpy(p->name, name);
	p->value = value;

	return p;
}

static char* serializeJsonString(char* out, const char* s)
{
	char* dptr;
	/* Allocate for worst case scenario; i.e. all characters need to be
	 * escaped, 2 quotes need to be added and a single terminating NUL
	 * character.
	 */
	char* const dest = malloc(strlen(s) * 2 + 2 + 1);
	if (dest == NULL)
	{
		debug(LOG_ERROR, "Out of memory");
		abort();
		return NULL;
	}

	dptr = dest;
	*dptr++ = '\"';

	for (; *s; ++s)
	{
		switch (*s)
		{
			case '\"':
				*dptr++ = '\\';
				*dptr++ = '"';
				break;

			case '\\':
				*dptr++ = '\\';
				*dptr++ = '\\';
				break;

			case '\b':
				*dptr++ = '\\';
				*dptr++ = 'b';
				break;

			case '\f':
				*dptr++ = '\\';
				*dptr++ = 'f';
				break;

			case '\n':
				*dptr++ = '\\';
				*dptr++ = 'n';
				break;

			case '\r':
				*dptr++ = '\\';
				*dptr++ = 'r';
				break;

			case '\t':
				*dptr++ = '\\';
				*dptr++ = 't';
				break;

			default:
				*dptr++ = *s;
				break;
		}
	}

	*dptr++ = '\"';
	*dptr++ = '\0';

	out = strrealloccat(out, dest);

	free(dest);

	return out;
}

static char* serializeJsonIndentation(char* out, unsigned int recursion)
{
	static const unsigned int shiftwidth = 2;

	char* const spaces = malloc(recursion * shiftwidth + 1);
	if (spaces == NULL)
	{
		debug(LOG_ERROR, "Out of memory");
		abort();
		return NULL;
	}

	memset(spaces, ' ', recursion * shiftwidth);
	spaces[recursion * shiftwidth] = '\0';

	out = strrealloccat(out, spaces);

	free(spaces);

	return out;
}

static char* serializeJsonValueIndented(char* out, const json_value* v, unsigned int recursion);

static char* serializeJsonValuePairIndented(char* out, const json_value_pair* p, unsigned int recursion)
{
	out = serializeJsonIndentation(out, recursion);
	out = serializeJsonString(out, p->name);

	out = strrealloccat(out, ": ");

	return serializeJsonValueIndented(out, p->value, recursion);
}

static char* serializeJsonValueIndented(char* out, const json_value* v, unsigned int recursion)
{
	switch (v->type)
	{
		case JSON_NUMBER_INT:
		{
			char* buf;
			sasprintf(&buf, "%ld", v->value.num_int);
			return strrealloccat(out, buf);
		}
		case JSON_NUMBER_FLOAT:
		{
			char* buf;
			sasprintf(&buf, "%lf", v->value.num_float);
			return strrealloccat(out, buf);
		}
		case JSON_BOOL:
			return strrealloccat(out, v->value.boolean ? "true" : "false");

		case JSON_NULL:
			return strrealloccat(out, "null");

		case JSON_STRING:
			return serializeJsonString(out, v->value.string);

		case JSON_OBJECT:
		{
			json_value_pair * cur = v->value.members;

			out = strrealloccat(out, "{\n");

			for (cur = v->value.members; cur != NULL; cur = cur->next)
			{
				// Write the member itself
				out = serializeJsonValuePairIndented(out, cur, recursion + 1);

				// Some structure
				if (cur->next)
				{
					out = strrealloccat(out, ",\n");
				}
			}

			out = strrealloccat(out, "\n");
			out = serializeJsonIndentation(out, recursion);
			out = strrealloccat(out, "}");

			break;
		}

		case JSON_ARRAY:
		{
			size_t i;

			out = strrealloccat(out, "[\n");

			for (i = 0; i < v->value.array.size; ++i)
			{
				// Write the member itself
				out = serializeJsonIndentation(out, recursion + 1);
				out = serializeJsonValueIndented(out, v->value.array.a[i], recursion + 1);

				// Some structure
				if (i + 1 < v->value.array.size)
				{
					out = strrealloccat(out, ",\n");
				}
			}

			out = strrealloccat(out, "\n");
			out = serializeJsonIndentation(out, recursion);
			out = strrealloccat(out, "]");

			break;
		}

		default:
			ASSERT(!"Unknown JSON type", "Unknown JSON type: %d", v->type);
			return NULL;
	}

	return out;
}

char* serializeJsonValuePair(const json_value_pair* p)
{
	return serializeJsonValuePairIndented(NULL, p, 0);
}

char* serializeJsonValue(const json_value* v)
{
	return serializeJsonValueIndented(NULL, v, 0);
}
