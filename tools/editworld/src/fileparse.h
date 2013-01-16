/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#ifndef __INCLUDED_FILEPARSE_H__
#define __INCLUDED_FILEPARSE_H__

// Valid values for Flags.

#define FP_SKIPCOMMENTS		1	// Comments are skipped.
#define FP_QUOTES			2	// Strings in quotes are parsed as one word.
#define FP_UPPERCASE		4	// Strings are converted to upper case.
#define FP_LOWERCASE		8	// Strings are converted to lower case.
#define FP_ASMCOMMENTS      16  // Comments use assembler form ie ; ( defaults to cpp form ie // )

#define MAXBREAKCHARACTERS	16	// Maximum number of break characters.
#define MAXTOKENSIZE		512

#include <istream>

struct TokenID
{
	char *Token;
	int ID;
};

class fileParser
{
	public:
		fileParser(std::istream& file, short flags);

		~fileParser();

		void SetFlags(short Flags) { m_Flags = Flags; }
		short GetFlags(void) { return m_Flags; }
		void SetBreakCharacters(char *brk);

		int CountTokens(void);
		void Rewind(void);
		void Parse(char *Word,short Size);
		void UnParse(void);
		BOOL ParseInt(char *Ident,int *Int);
		BOOL ParseFloat(char *Ident,float *Float);
		BOOL ParseDouble(char *Ident,double *Double);
		BOOL ParseString(char *Ident,char *Word,short Size);
		BOOL FindToken(char *Token);
		BOOL FindTokenList(char *Token, ...);
		char *GetBase(void) { return m_File; }
		char *GetPos(void) { return m_Pos; }
		void SetPos(char *Pos) { m_Pos = Pos; }

		TokenID *FindTokenID(char *Token,TokenID *IDLookup);
		int TokenToInt(char *Word);
		float TokenToFloat(char *Word);
		double TokenToDouble(char *Word);

	private:
		char *StringNextToken(char *s,char *tok,short toklen,char *brk);
		char StringFixCase(char chr);
		BOOL StringBreakChar(char chr,char *brk);
		char *StringSkipBlanks(char *p);
		char *StringSkipLine(char *p);

		char m_Brk[MAXBREAKCHARACTERS];
		short m_Flags;
		char *m_File;
		char *m_Pos;
		char *m_LastPos;
		long m_BufferSize;
};

#endif // __INCLUDED_FILEPARSE_H__
