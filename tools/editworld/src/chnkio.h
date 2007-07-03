#ifndef __CHNKIO_INCLUDED__
#define __CHNKIO_INCLUDED__

#define	CHECKTRUE(func) if((func)==FALSE) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }
#define	CHECKZERO(func) if((func)!=0) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }
#define	CHECKONE(func) if((func)!=1) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }
#define	CHECKNOTZERO(func) if((func)==0) { DebugPrint("Check Failed %s %d\n",__FILE__,__LINE__); return FALSE; }


class CChnkIO {
public:
	void SceneDebugPrint(const TCHAR *format, ...);
	BOOL StartChunk(FILE *Stream,char *Name);
	BOOL EndChunk(FILE *Stream);
	BOOL ReadLong(FILE *Stream,char *Name,LONG *Long);
	BOOL ReadFloat(FILE *Stream,char *Name,float *Float);
	BOOL ReadString(FILE *Stream,char *Name,char *String);
	BOOL ReadQuotedString(FILE *Stream,char *Name,char *String);
	BOOL ReadStringAlloc(FILE *Stream,char *Name,char **String);
	BOOL ReadQuotedStringAlloc(FILE *Stream,char *Name,char **String);
};

#endif
