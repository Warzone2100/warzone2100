#ifndef _bug_
#define _bug_

#include <assert.h>

#define iV_DEBUG_FILE	"IVI.LOG"

#ifdef _DEBUG
//	#define iV_DEBUG
#endif

#ifdef iV_DEBUG

#ifdef WIN32

#define iV_DEBUG0(S)										iV_Debug((S))
#define iV_DEBUG1(S,A)									iV_Debug((S),(A))
#define iV_DEBUG2(S,A,B) 								iV_Debug((S),(A),(B))
#define iV_DEBUG3(S,A,B,C)								iV_Debug((S),(A),(B),(C))
#define iV_DEBUG4(S,A,B,C,D)							iV_Debug((S),(A),(B),(C),(D))
#define iV_DEBUG5(S,A,B,C,D,E)						iV_Debug((S),(A),(B),(C),(D),(E))
#define iV_DEBUG6(S,A,B,C,D,E,F)						iV_Debug((S),(A),(B),(C),(D),(E),(F))
#define iV_DEBUG7(S,A,B,C,D,E,F,G)					iV_Debug((S),(A),(B),(C),(D),(E),(F),(G))
#define iV_DEBUG8(S,A,B,C,D,E,F,G,H)				iV_Debug((S),(A),(B),(C),(D),(E),(F),(G),(H))
#define iV_DEBUG9(S,A,B,C,D,E,F,G,H,I)				iV_Debug((S),(A),(B),(C),(D),(E),(F),(G),(H),(I))
#define iV_DEBUG10(S,A,B,C,D,E,F,G,H,I,J) 		iV_Debug((S),(A),(B),(C),(D),(E),(F),(G),(H),(I),(J))
#define iV_DEBUG11(S,A,B,C,D,E,F,G,H,I,J,K) 		iV_Debug((S),(A),(B),(C),(D),(E),(F),(G),(H),(I),(J),(K))
#define iV_DEBUG12(S,A,B,C,D,E,F,G,H,I,J,K,L)	iV_Debug((S),(A),(B),(C),(D),(E),(F),(G),(H),(I),(J),(K),(L))
#define iV_DEBUG_CREATE_LOG							_debug_create_log()
#define iV_ASSERT(A)										assert((A))

#else

#define MAXDBGSIZE (256)
extern char dbgbuffer[MAXDBGSIZE];

#define iV_DEBUG0(S)							iV_Debug((S))
#define iV_DEBUG1(S,A)  { sprintf(dbgbuffer,(S),(A)); iV_Debug(dbgbuffer); }
#define iV_DEBUG2(S,A,b)  { sprintf(dbgbuffer,(S),(A),(b)); iV_Debug(dbgbuffer); }
#define iV_DEBUG3(S,A,b,c)  { sprintf(dbgbuffer,(S),(A),(b),(c)); iV_Debug(dbgbuffer); }
#define iV_DEBUG4(S,A,b,c,d)  { sprintf(dbgbuffer,(S),(A),(b),(c),(d)); iV_Debug(dbgbuffer); }
#define iV_DEBUG5(S,A,b,c,d,e)  { sprintf(dbgbuffer,(S),(A),(b),(c),(d),(e)); iV_Debug(dbgbuffer); }
#define iV_DEBUG6(S,A,b,c,d,e,f)  { sprintf(dbgbuffer,(S),(A),(b),(c),(d),(e),(f)); iV_Debug(dbgbuffer); }
#define iV_DEBUG7(S,A,b,c,d,e,f,g)  { sprintf(dbgbuffer,(S),(A),(b),(c),(d),(e),(f),(g)); iV_Debug(dbgbuffer); }
#define iV_DEBUG12(S,A,b,c,d,e,f,g,h,i,j,k,l)  { sprintf(dbgbuffer,(S),(A),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l)); iV_Debug(dbgbuffer); }
#define iV_DEBUG_CREATE_LOG							_debug_create_log()
#define iV_ASSERT(A)										assert((A))

#endif

#else

#define iV_DEBUG0(S)
#define iV_DEBUG1(S,A)
#define iV_DEBUG2(S,A,B)
#define iV_DEBUG3(S,A,B,C)
#define iV_DEBUG4(S,A,B,C,D)
#define iV_DEBUG5(S,A,B,C,D,E)
#define iV_DEBUG6(S,A,B,C,D,E,F)
#define iV_DEBUG7(S,A,B,C,D,E,F,G)
#define iV_DEBUG8(S,A,B,C,D,E,F,G,H)
#define iV_DEBUG9(S,A,B,C,D,E,F,G,H,I)
#define iV_DEBUG10(S,A,B,C,D,E,F,G,H,I,J)
#define iV_DEBUG11(S,A,B,C,D,E,F,G,H,I,J,K)
#define iV_DEBUG12(S,A,B,C,D,E,F,G,H,I,J,K,L)
#define iV_DEBUG_CREATE_LOG
#define iV_ASSERT(A)

#endif

//*************************************************************************

extern void iV_Debug(char *string, ...);
extern void iV_DebugDisplayLog(void);

//*************************************************************************

extern void _debug_create_log(void);

#endif
