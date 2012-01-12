/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

	$Revision$
	$Id$
	$HeadURL$
*/

#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "typedefs.h"
#include "debugprint.hpp"


#include "chnkio.h"

BOOL CChnkIO::StartChunk(FILE *Stream,char *Name)
{
	char	Tmp[256];

	if(Name) {
//		DebugPrint("%s\n",Name);
		CHECKONE(fscanf(Stream,"%s",Tmp));
		CHECKZERO(strcmp(Tmp,Name));
	}
	CHECKONE(fscanf(Stream,"%s",Tmp));
	CHECKZERO(strcmp(Tmp,"{"));

	return TRUE;
}

BOOL CChnkIO::EndChunk(FILE *Stream)
{
	char	Tmp[256];

//	DebugPrint("EndChunk\n");
	CHECKONE(fscanf(Stream,"%s",Tmp));
	CHECKZERO(strcmp(Tmp,"}"));

	return TRUE;
}

BOOL CChnkIO::ReadLong(FILE *Stream,char *Name,LONG *Long)
{
	char	Tmp[256];

	if(Name) {
//		DebugPrint("%s\n",Name);
		CHECKONE(fscanf(Stream,"%s",Tmp));
		CHECKZERO(strcmp(Tmp,Name));
	}
	CHECKONE(fscanf(Stream,"%ld",Long));

	return TRUE;
}

BOOL CChnkIO::ReadFloat(FILE *Stream,char *Name,float *Float)
{
	char	Tmp[256];

	if(Name) {
//		DebugPrint("%s\n",Name);
		CHECKONE(fscanf(Stream,"%s",Tmp));
		CHECKZERO(strcmp(Tmp,Name));
	}
	CHECKONE(fscanf(Stream,"%f",Float));

	return TRUE;
}

BOOL CChnkIO::ReadString(FILE *Stream,char *Name,char *String)
{
	char	Tmp[256];

	if(Name) {
//		DebugPrint("%s\n",Name);
		CHECKONE(fscanf(Stream,"%s",Tmp));
		CHECKZERO(strcmp(Tmp,Name));
	}
	CHECKONE(fscanf(Stream,"%s",String));

	return TRUE;
}

BOOL CChnkIO::ReadQuotedString(FILE *Stream,char *Name,char *String)
{
	char	Tmp[256];

	if(Name) {
		CHECKONE(fscanf(Stream,"%s",Tmp));
		CHECKZERO(strcmp(Tmp,Name));
	}

	char c;
	BOOL IsQuoted = TRUE;

	do {
		c = fgetc(Stream);
		if( (c!=' ') && (c!='\t') && (c!='"') && (c!=10) && (c!=13) ) {
			IsQuoted = FALSE;
			break;
		}
	} while(c!='"');

	int i=0;
	if(IsQuoted) {
		do {
			c = fgetc(Stream);
			if(c!='"') {
				String[i] = c;
	// Need to max length check here
				i++;
			}
		} while(c!='"');
		String[i] = 0;
	} else {
		String[i] = c;
		i++;
		do {
			c = fgetc(Stream);
			if((c!=' ') && (c!='\t') && (c!=10) && (c!=13)) {
				String[i] = c;
	// Need to max length check here
				i++;
			}
		} while((c!=' ') && (c!='\t') && (c!=10) && (c!=13));
		String[i] = 0;
	}

//	char c;
//
//	do {
//		c = fgetc(Stream);
//	} while(c!='"');
//
//	int i=0;
//	do {
//		c = fgetc(Stream);
//		if(c!='"') {
//			String[i] = c;
//// Need to max length check here
//			i++;
//		}
//	} while(c!='"');
//	String[i] = 0;

	return TRUE;
}

BOOL CChnkIO::ReadStringAlloc(FILE *Stream,char *Name,char **String)
{
	char	Tmp[256];

	if(Name) {
//		DebugPrint("%s\n",Name);
		CHECKONE(fscanf(Stream,"%s",Tmp));
		CHECKZERO(strcmp(Tmp,Name));
	}
	CHECKONE(fscanf(Stream,"%s",Tmp));

	*String = new char[strlen(Tmp)+1];
	strcpy(*String,Tmp);

	return TRUE;
}

BOOL CChnkIO::ReadQuotedStringAlloc(FILE *Stream,char *Name,char **String)
{
	char	Tmp[256];

	if(Name) {
		CHECKONE(fscanf(Stream,"%s",Tmp));
		CHECKZERO(strcmp(Tmp,Name));
	}

	char c;
	BOOL IsQuoted = TRUE;

	do {
		c = fgetc(Stream);
		if( (c!=' ') && (c!='\t') && (c!='"') ) {
			IsQuoted = FALSE;
			break;
		}
	} while(c!='"');

	int i=0;
	if(IsQuoted) {
		do {
			c = fgetc(Stream);
			if(c!='"') {
				Tmp[i] = c;
	// Need to max length check here
				i++;
			}
		} while(c!='"');
		Tmp[i] = 0;
	} else {
		Tmp[i] = c;
		i++;
		do {
			c = fgetc(Stream);
			if((c!=' ') && (c!='\t') && (c!=10) && (c!=13)) {
				Tmp[i] = c;
	// Need to max length check here
				i++;
			}
		} while((c!=' ') && (c!='\t') && (c!=10) && (c!=13));
		Tmp[i] = 0;
	}

	*String = new char[strlen(Tmp)+1];
	strcpy(*String,Tmp);

	return TRUE;
}

