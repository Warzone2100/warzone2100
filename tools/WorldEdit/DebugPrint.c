#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "tchar.h"

#ifdef _DEBUG

extern void DebugWinPrint(char *String);

FILE* DebugStream=NULL;

void DebugOpen(char* LogName)
{
	if(DebugStream) {
		fclose(DebugStream);
		DebugStream = NULL;
	}

	DebugStream = fopen(LogName,"wb");
	
	fprintf(DebugStream,"This build : %s %s\n\n",__DATE__,__TIME__);
}

void DebugClose(void)
{
	if(DebugStream) {
		fclose(DebugStream);
	}
	DebugStream=NULL;
}

void DebugPrint(const TCHAR *format, ...)
{
	TCHAR buf[4096];

	va_list args;
	va_start(args,format);
	_vsntprintf(buf,4096,format,args);
	va_end(args);
	OutputDebugString(buf);
	if(DebugStream!=NULL) {
		fprintf(DebugStream,"%s",buf);
	}

//	DebugWinPrint(buf);
}

#else

void DebugOpen(char* LogName)
{
}

void DebugClose(void)
{
}


#endif
