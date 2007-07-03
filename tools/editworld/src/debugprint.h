
#ifdef __cplusplus
extern "C" {
#endif

//#define DEBUG0
//#define DEBUG1
//#define DEBUG2
//#define DEBUG3
//#define DEBUG4
#define DEBUG5
	
void DebugOpen(char* LogName);
void DebugClose(void);

#ifndef _TCHAR_DEFINED
typedef char TCHAR, *PTCHAR;
typedef unsigned char TBYTE , *PTBYTE ;
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */


#ifdef _DEBUG
void DebugPrint(const TCHAR *format, ...);
#else
#define DebugPrint
#endif

#ifdef __cplusplus
	#ifndef ASSERT

	#define ASSERT

	#endif
#else
	#ifndef ASSERT

		#define DBG_ASSERT(a,b) if(!(a)) { DebugPrint("Assertion Failure in %s, line : %d\n",__FILE__, __LINE__); \
								DebugPrint(b); \
								DebugPrint("\n"); }
		#define ASSERT(a) DBG_ASSERT a

	#endif
#endif

#define DBERROR(a) DebugPrint a;

#define DBPRINTF(a) DebugPrint a;

#ifdef DEBUG0
#define DBP0(a) DebugPrint a;
#else
#define DBP0
#endif

#ifdef DEBUG1
#define DBP1(a) DebugPrint a;
#else
#define DBP1
#endif

#ifdef DEBUG2
#define DBP2(a) DebugPrint a;
#else
#define DBP2
#endif

#ifdef DEBUG3
#define DBP3(a) DebugPrint a;
#else
#define DBP3
#endif

#ifdef DEBUG4
#define DBP4(a) DebugPrint a;
#else
#define DBP4
#endif

#ifdef DEBUG5
#define DBP5(a) DebugPrint a;
#else
#define DBP5
#endif

#ifdef __cplusplus
}
#endif

