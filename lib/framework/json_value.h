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

#ifndef __INCLUDED_JSON_VALUE_H__
#define __INCLUDED_JSON_VALUE_H__

#include "types.h"

typedef struct __json_value_pair
{
	char*                           name;
	struct __json_value*            value;
	struct __json_value_pair*       next;
} json_value_pair;

typedef struct __json_value
{
	enum
	{
		JSON_STRING,
		JSON_NUMBER_INT,
		JSON_NUMBER_FLOAT,
		JSON_OBJECT,
		JSON_ARRAY,
		JSON_BOOL,
		JSON_NULL,
	} type;

	union
	{
		char*                   string;
		long int                num_int;
		double                  num_float;
		bool                    boolean;
		json_value_pair*        members;
		struct
		{
			size_t                  size;
			struct __json_value**   a;
		}                       array;
	} value;
} json_value;

extern void freeJsonValuePair(json_value_pair* p);
extern void freeJsonValue(json_value* v);

extern json_value* createJsonString(const char* s);
extern json_value_pair* createJsonValuePair(const char* name, json_value* value);
extern json_value* createJsonArray(size_t size);
extern json_value* createJsonInteger(long int val);
extern json_value* createJsonFloat(double val);
extern json_value* createJsonBool(bool val);
extern json_value* createJsonNull(void);

extern char* serializeJsonValuePair(const json_value_pair* p);
extern char* serializeJsonValue(const json_value* v);

#endif // __INCLUDED_JSON_VALUE_H__
