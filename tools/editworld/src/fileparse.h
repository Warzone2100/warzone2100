
#ifndef __INCLUDED_FILEPARSER__
#define __INCLUDED_FILEPARSER__

// Valid values for Flags.

#define FP_SKIPCOMMENTS		1	// Comments are skipped.
#define FP_QUOTES			2	// Strings in quotes are parsed as one word.
#define FP_UPPERCASE		4	// Strings are converted to upper case.
#define FP_LOWERCASE		8	// Strings are converted to lower case.
#define FP_ASMCOMMENTS      16  // Comments use assembler form ie ; ( defaults to cpp form ie // )

#define MAXBREAKCHARACTERS	16	// Maximum number of break characters.
#define MAXTOKENSIZE		512

struct TokenID {
	char *Token;
	int ID;
};

class CFileParser {

public:
	CFileParser(void);
	BOOL Create(char *FileName,short Flags);
	CFileParser(FILE *Stream,short Flags);
	CFileParser(char *Buffer,long BufferSize,short Flags);
	~CFileParser(void);
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

protected:
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
	BOOL m_WasAllocated;
};

#endif
